/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2015 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * BSD LICENSE
 *
 * Copyright (c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Intel(R) Omni-Path Gen2 HFI PCIe Driver
 */

#include <linux/bitmap.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>
#include "hfi2.h"
#include "chip/fxr_rx_dma_defs.h"

static int hfi_pid_alloc(struct hfi_ctx *ctx, u16 flags, u16 *ptl_pid);
static void hfi_pid_free(struct hfi_devdata *dd, u16 ptl_pid);

/*
 * PID_ANY changed to 4095, as FXR PID is 12-bits.
 * We also consider -1 to mean PID_ANY assignment.
 */
#define IS_PID_ANY(x)	((x) == HFI_PID_ANY || (x) == (u16)-1)

#define HFI_PID_TIMEOUT_MSECS	2000

void hfi_ctx_init(struct hfi_ctx *ctx, struct hfi_devdata *dd)
{
	ctx->devdata = dd;
	ctx->ops = dd->core_ops;
	ctx->type = HFI_CTX_TYPE_KERNEL;
	ctx->mode = 0;
	ctx->allow_phys_dlid = 1;
	ctx->pid = HFI_PID_NONE;
	ctx->pid_count = 0;
	/* UID=0 reserved for kernel clients */
	ctx->ptl_uid = HFI_DEFAULT_PTL_UID;
	idr_init(&ctx->ct_used);
	idr_init(&ctx->eq_used);
	idr_init(&ctx->ec_used);
	mutex_init(&ctx->dlid_mutex);
	mutex_init(&ctx->event_mutex);
	spin_lock_init(&ctx->eq_lock);
	init_rwsem(&ctx->ctx_rwsem);
}

/*
 * Reserves contiguous PIDs. Note, also used for singleton reservation which
 * do not touch ctx->[pid_base, pid_count].
 */
static int __hfi_ctx_reserve(struct hfi_devdata *dd, u16 *base, u16 count,
			     u16 align, u16 offset, u16 size, bool reuse)
{
	int ret = 0;
	unsigned long align_mask = 0, start, n;
	int ptl_pid = 0;
	unsigned long diff_jiffies, exit_jiffy;

	if (!count)
		return -EINVAL;
	if (size > HFI_NUM_USABLE_PIDS)
		return -EINVAL;

	if (align) {
		if (!is_power_of_2(align))
			return -EINVAL;
		align_mask = align - 1;
	}

	if (IS_PID_ANY(*base)) {
		start = offset;
		reuse = false;
	} else {
		/* honor request for base PID */
		start = offset + *base;
	}

	if (start + count > size)
		return -EINVAL;

	spin_lock(&dd->ptl_lock);
	/*
	 * Clear PIDs completing timed wait. We also clear if the caller
	 * passed in REUSE_ADDR and this is a single PID reservation.
	 */
	do {
		exit_jiffy = (unsigned long)idr_get_next(&dd->pid_wait,
							 &ptl_pid);
		if (!exit_jiffy)
			break;
		diff_jiffies = jiffies - exit_jiffy;

		if ((reuse && (count == 1) && (start == ptl_pid)) ||
		    (jiffies_to_msecs(diff_jiffies) > HFI_PID_TIMEOUT_MSECS)) {
			bitmap_clear(dd->ptl_map, ptl_pid, 1);
			idr_remove(&dd->pid_wait, ptl_pid);
		}
		ptl_pid++;
	} while (1);

	n = bitmap_find_next_zero_area(dd->ptl_map, size,
				       start, count, align_mask);
	if (n >= size) {
		ret = -EBUSY;
	} else if (!IS_PID_ANY(*base) && n != start) {
		ret = -EBUSY;
	} else {
		bitmap_set(dd->ptl_map, n, count);
		*base = n;
	}
	spin_unlock(&dd->ptl_lock);

	return ret;
}

int hfi_ctx_reserve(struct hfi_ctx *ctx, u16 *base, u16 count, u16 align,
		    u16 flags)
{
	struct hfi_devdata *dd = ctx->devdata;
	u16 offset = 0, size = HFI_NUM_USABLE_PIDS;
	int ret;

	/* only one PID reservation */
	if (ctx->pid_count)
		return -EPERM;

	/* If requested, use non-portals PID reservation (for PSM) */
	if (flags & HFI_CTX_FLAG_USE_BYPASS) {
		offset = HFI_PID_BYPASS_BASE;
		size = offset + HFI_NUM_BYPASS_PIDS;
	}

	ret = __hfi_ctx_reserve(dd, base, count, align, offset, size, false);
	if (ret)
		return ret;

	ctx->pid_base = *base;
	ctx->pid_count = count;

#ifdef CONFIG_HFI2_STLNP
	if (flags & HFI_CTX_FLAG_SET_VIRTUAL_PIDS) {
		ret = hfi_ctx_set_virtual_pid_range(ctx);
		if (ret) {
			hfi_ctx_unreserve(ctx);
			return ret;
		}
	}
#endif

	return 0;
}

static void __hfi_ctx_unreserve(struct hfi_devdata *dd, u16 base, u16 count)
{
	int pid_wait_idx, i;
	/* Allocate PID wait idr index and associate with jiffy time */
	idr_preload(GFP_KERNEL);
	spin_lock(&dd->ptl_lock);
	for (i = 0; i < count; i++) {
		pid_wait_idx = idr_alloc(&dd->pid_wait,
					 (unsigned long *)(jiffies &
					 ~RADIX_TREE_ENTRY_MASK),
					 base + i, base + i + 1,
					 GFP_NOWAIT);
		WARN_ON(pid_wait_idx < 0);
	}
	spin_unlock(&dd->ptl_lock);
	idr_preload_end();
}

void hfi_ctx_unreserve(struct hfi_ctx *ctx)
{
	struct hfi_devdata *dd = ctx->devdata;

#ifdef CONFIG_HFI2_STLNP
	if (IS_PID_VIRTUALIZED(ctx))
		hfi_ctx_unset_virtual_pid_range(ctx);
#endif

	if (ctx->pid_count)
		__hfi_ctx_unreserve(dd, ctx->pid_base, ctx->pid_count);
	ctx->pid_base = -1;
	ctx->pid_count = 0;
}

/* Initialize the Unexpected Free List */
static void hfi_uh_init(struct hfi_ctx *ctx, u16 unexpected_count)
{
	int idx;
	hfi_uh_t *p = ctx->le_me_addr + ctx->le_me_size;

	/*
	 * Add all the Unexpected Headers to the free list which begins in
	 * UH[0]
	 */
	for (idx = 0; idx < unexpected_count; idx++)
		p++->k.next = (idx + 1) % unexpected_count;
}

/*
 * Associate this process with a Portals PID.
 * Note, hfi_open() sets:
 *   ctx->pid = HFI_PID_NONE
 *   ctx->ptl_uid = current_uid()
 */
int hfi_ctx_attach(struct hfi_ctx *ctx, struct opa_ctx_assign *ctx_assign)
{
	struct hfi_devdata *dd = ctx->devdata;
	u16 ptl_pid;
	u32 psb_size, trig_op_size, le_me_off, le_me_size, unexp_size;
	int i, ni, ret;

	down_write(&ctx->ctx_rwsem);
	/* only one Portals PID allowed */
	if (ctx->pid != HFI_PID_NONE) {
		ret = -EPERM;
		goto err_unlock;
	}

	/* if requested (by PSM), allocate PID from BYPASS range */
	if (ctx_assign->flags & HFI_CTX_FLAG_USE_BYPASS)
		ctx->mode |= HFI_CTX_MODE_USE_BYPASS;

	ptl_pid = ctx_assign->pid;
	ret = hfi_pid_alloc(ctx, ctx_assign->flags, &ptl_pid);
	if (ret)
		goto err_unlock;

	/* set ptl_pid, hfi_ctx_cleanup() can handle all errors below */
	ctx->pid = ptl_pid;

	/* Attempt to claim bypass PID if requested */
	if (ctx->type == HFI_CTX_TYPE_KERNEL &&
	    ctx->mode & HFI_CTX_MODE_REQUEST_DEFAULT_BYPASS) {
		ret = hfi_ctx_set_bypass(ctx, 0);
		if (ret)
			goto err_bypass;
	}

	/* allocate pasid for this context */
	ret = hfi_at_set_pasid(ctx);
	if (ret)
		goto err_pasid;

	/* compute total Portals State size, HW requires minimum of one page */
	trig_op_size = ctx_assign->trig_op_count ?
		PAGE_ALIGN(ctx_assign->trig_op_count * HFI_TRIG_OP_SIZE) :
		PAGE_SIZE;
	le_me_size = ctx_assign->le_me_count ?
		PAGE_ALIGN(ctx_assign->le_me_count * HFI_LE_ME_SIZE) :
		PAGE_SIZE;
	unexp_size = ctx_assign->unexpected_count ?
		PAGE_ALIGN(ctx_assign->unexpected_count * HFI_UNEXP_SIZE) :
		PAGE_SIZE;
	le_me_off = HFI_PSB_FIXED_TOTAL_MEM + trig_op_size;
	psb_size = le_me_off + le_me_size + unexp_size;

	/* vmalloc Portals State memory, will store in PCB */
	/* FXRTODO: replace vmalloc_user with vmalloc_user_node */
	ctx->ptl_state_base = vmalloc_user(psb_size);
	if (!ctx->ptl_state_base) {
		ret = -ENOMEM;
		goto err_vmalloc;
	}
	dd_dev_dbg(dd, "%s %d trig_op_size 0x%x HFI_PSB_FIXED_TOTAL_MEM 0x%x le_me_size 0x%x unexp_size 0x%x psb_size 0x%x va %p\n",
		   __func__, __LINE__, trig_op_size, HFI_PSB_FIXED_TOTAL_MEM,
		   le_me_size, unexp_size, psb_size, ctx->ptl_state_base);
	ctx->ptl_state_size = psb_size;
	/* TODO: Take proper lock while updating priv_ctx SPT */
	hfi_at_reg_range(&dd->priv_ctx, ctx->ptl_state_base,
			 ctx->ptl_state_size, NULL, true);

	ctx->le_me_addr = (void *)(ctx->ptl_state_base + le_me_off);
	ctx->le_me_count = ctx_assign->le_me_count;
	ctx->le_me_size = le_me_size;
	ctx->unexpected_size = unexp_size;
	ctx->trig_op_size = trig_op_size;

	/* Initialize the Unexpected Free List */
	hfi_uh_init(ctx, ctx_assign->unexpected_count);

	/*
	 * Initialize ME/LE pool used by hfi_me_alloc/free,
	 * only needed for native transport.
	 */
	ctx->le_me_free_index = 0;
	ctx->le_me_free_list = NULL;
	if (ctx->type == HFI_CTX_TYPE_KERNEL &&
	    !(ctx->mode & HFI_CTX_MODE_USE_BYPASS)) {
		if (ctx->le_me_count > 1) {
			ctx->le_me_free_list = vzalloc(sizeof(u16) *
						       (ctx->le_me_count - 1));
			if (!ctx->le_me_free_list)
				goto err_le_me;

			for (i = 0; i < ctx->le_me_count - 1; i++)
				/* Entry 0 is reserved, so start from 1. */
				ctx->le_me_free_list[i] = i + 1;
		}

		/* Initialize PT pool used by hfi_pt_(fast_)alloc/free */
		for (ni = 0; ni < HFI_NUM_NIS; ni++) {
			ctx->pt_free_index[ni] = 0;
			/*
			 * Place high valued entries so they are allocated
			 * first by HFI_PT_ANY.
			 */
			for (i = 0; i < HFI_NUM_PT_ENTRIES; i++)
				ctx->pt_free_list[ni][i] =
					HFI_NUM_PT_ENTRIES - i - 1;
		}
	}

	dd_dev_info(dd, "Portals PID %u assigned PCB:[%d, %d, %d, %d]\n",
		    ptl_pid, psb_size, trig_op_size, le_me_size,
		    unexp_size);

	/* write PCB (host memory) */
	hfi_pcb_write(ctx, ptl_pid);

	/*
	 * stash pointers to PCB segments (for kernel clients),
	 * these tokens do not return any error
	 */
	hfi_ctx_hw_addr(ctx, TOK_EVENTS_CT, ctx->pid,
			&ctx->ct_addr, &ctx->ct_size);
	hfi_ctx_hw_addr(ctx, TOK_EVENTS_EQ_DESC, ctx->pid,
			&ctx->eq_addr, &ctx->eq_size);
	hfi_ctx_hw_addr(ctx, TOK_EVENTS_EQ_HEAD, ctx->pid,
			&ctx->eq_head_addr, &ctx->eq_head_size);
	hfi_ctx_hw_addr(ctx, TOK_PORTALS_TABLE, ctx->pid,
			&ctx->pt_addr, &ctx->pt_size);

	/* Initialize the erorr queue if requested */
	hfi_setup_errq(ctx, ctx_assign);

	up_write(&ctx->ctx_rwsem);

	/*
	 * The following code calls hfi_eq_assign()/hfi_ct_assign(), which
	 * acquires ctx_rwsem in shared mode, we release ctx_rwsem above to
	 * avoid deadlock. Also the code is only for trusted kernel clients,
	 * during the time window without ctx_rwsem, it is unlikely this
	 * context is detached to cause a multi-threaded race condition.
	 */
	if (ctx->type == HFI_CTX_TYPE_KERNEL) {
		/* assign Status Registers (first 16 CTs) */
		struct opa_ev_assign ct_assign = {0};

		for (ni = 0; ni < HFI_NUM_NIS; ni++) {
			ct_assign.ni = ni;
			ct_assign.mode = OPA_EV_MODE_COUNTER;
			for (i = 0; i < HFI_NUM_CT_RESERVED; i++) {
				ret = hfi_cteq_assign(ctx, &ct_assign);
				if (ret < 0)
					goto err_kern_ctx;
				ctx->status_reg[HFI_SR_INDEX(ni, i)] =
						ct_assign.ev_idx;
			}
		}
		/* EQ0 for system PID is assigned after RX CMDQ is enabled */
		if (ctx->pid != HFI_PID_SYSTEM) {
			ret = hfi_eq_zero_assign(ctx);
			if (ret < 0)
				goto err_kern_ctx;
		}
	}

	dd->stats.sps_ctxts++;

	return 0;

err_kern_ctx:
	hfi_errq_cleanup(ctx);
	hfi_pcb_reset(dd, ptl_pid);
	vfree(ctx->le_me_free_list);
err_le_me:
	hfi_at_dereg_range(&dd->priv_ctx, ctx->ptl_state_base,
			   ctx->ptl_state_size);
	vfree(ctx->ptl_state_base);
err_vmalloc:
	hfi_at_clear_pasid(ctx);
err_pasid:
	hfi_ctx_clear_bypass(ctx);
err_bypass:
	ctx->pid = HFI_PID_NONE;
	hfi_pid_free(dd, ptl_pid);
err_unlock:
	up_write(&ctx->ctx_rwsem);
	return ret;
}

static int hfi_pid_alloc(struct hfi_ctx *ctx, u16 flags, u16 *assigned_pid)
{
	int ret;
	struct hfi_devdata *dd = ctx->devdata;
	u16 start, end, ptl_pid = *assigned_pid;

	if (ctx->pid_count) {
		/*
		 * Here there is already a PID reservation set by the job
		 * launcher (contiguous set of PIDs) with ops->ctx_reserve.
		 * Assign PID from existing Portals PID reservation.
		 * User can ask for driver assignment (-1) from within the
		 * reservation or ask for a specific (logical) PID within the
		 * reserved set of PIDs.
		 */
		if (!IS_PID_ANY(ptl_pid) &&
		    (ptl_pid >= ctx->pid_count))
			return -EINVAL;

		if (IS_PID_ANY(ptl_pid)) {
			/* get first available PID within the reservation */
			start = ctx->pid_base;
			end = ctx->pid_base + ctx->pid_count;
		} else {
			/*
			 * attempt to assign specific PID offset in
			 * reservation
			 */
			start = ctx->pid_base + ptl_pid;
			end = ctx->pid_base + ptl_pid + 1;
		}
	} else {
		u16 offset = 0, size = HFI_NUM_USABLE_PIDS;

		if (ctx->mode & HFI_CTX_MODE_USE_BYPASS) {
			offset = HFI_PID_BYPASS_BASE;
			size = offset + HFI_NUM_BYPASS_PIDS;
		}

		/*
		 * Here PID is user-specified, there was not a job launcher
		 * supplied PID reservation.
		 * User can ask for driver assignment (-1) or ask for a
		 * specific PID.
		 */
		if (!IS_PID_ANY(ptl_pid) &&
		    (offset + ptl_pid >= size))
			return -EINVAL;

		/* No reservation, so must find unreserved PID first */
		ret = __hfi_ctx_reserve(dd, &ptl_pid, 1, 0, offset, size,
					!!(flags & HFI_CTX_FLAG_REUSE_ADDR));
		if (ret)
			return ret;

		dd_dev_info(dd, "acquired PID singleton [%u]\n", ptl_pid);

		start = ptl_pid;
		end = ptl_pid + 1;
	}

	/* Now assign the PID and associate with this context */
	idr_preload(GFP_KERNEL);
	spin_lock(&dd->ptl_lock);
	ret = idr_alloc(&dd->ptl_user, ctx, start, end, GFP_NOWAIT);
	if (ret >= 0)
		dd->pid_num_assigned++;
	spin_unlock(&dd->ptl_lock);
	idr_preload_end();

	if (ret < 0) {
		/*
		 * PID is already in use.
		 * For the case where there was not already a PID reservation,
		 * then we reserved a single PID above and are marking assigned
		 * with IDR. IDR allocation cannot fail in this case unless
		 * there is PID state corruption due to a driver bug, hence
		 * the WARN_ON here. Release reservation and return error.
		 */
		if (!ctx->pid_count) {
			WARN_ON(!ctx->pid_count);
			__hfi_ctx_unreserve(dd, ptl_pid, 1);
		}
		return ret;
	}
	*assigned_pid = ret;
	return 0;
}

static void hfi_pid_free(struct hfi_devdata *dd, u16 ptl_pid)
{
	spin_lock(&dd->ptl_lock);
	idr_remove(&dd->ptl_user, ptl_pid);
	dd->pid_num_assigned--;
	spin_unlock(&dd->ptl_lock);
	dd_dev_info(dd, "Portals PID %u released\n", ptl_pid);
}

/*
 * Release Portals PID resources.
 * Called during close() or explicitly via CMD_CTXT_DETACH.
 */
int hfi_ctx_cleanup(struct hfi_ctx *ctx)
{
	struct hfi_devdata *dd = ctx->devdata;
	struct hfi_ctx *idr_ctx;
	u16 ptl_pid;
	int ret = 0;

	down_write(&ctx->ctx_rwsem);
	ptl_pid = ctx->pid;
	if (ptl_pid == HFI_PID_NONE) {
		/* no assigned PID */
		goto unlock;
	}

	/* verify PID state is not corrupted  */
	spin_lock(&dd->ptl_lock);
	idr_ctx = idr_find(&dd->ptl_user, ptl_pid);
	spin_unlock(&dd->ptl_lock);
	if (idr_ctx != ctx) {
		/*
		 * driver is in corrupted state, so we can't safely take
		 * any action. Just return, however the PID is potentially
		 * now unusable due to the corrupted state.
		 */
		WARN_ON(idr_ctx != ctx);
		goto unlock;
	}

	/* User needs to clean up cmdqs before cleaning up the context */
	if (ctx->cmdq_pair_num_assigned) {
		ret = -EINVAL;
		goto unlock;
	}

	/* release event resources: ct/eq/ec */
	hfi_event_cleanup(ctx);

	/* free error queue */
	hfi_errq_cleanup(ctx);

	/* reset PCB, cancel OTR, flush caches */
	hfi_pcb_reset(dd, ptl_pid);

	if (ctx->le_me_free_list) {
		vfree(ctx->le_me_free_list);
		ctx->le_me_free_list = NULL;
	}

	if (ctx->ptl_state_base) {
		hfi_at_dereg_range(&dd->priv_ctx, ctx->ptl_state_base,
				   ctx->ptl_state_size);
		vfree(ctx->ptl_state_base);
		ctx->ptl_state_base = NULL;
	}

	/* stop pasid translation, but not for ZEBU: HSD 1209735086 */
	hfi_at_clear_pasid(ctx);

	/* disable default Bypass PID if used */
	hfi_ctx_clear_bypass(ctx);

	/* release assigned PID */
	hfi_pid_free(dd, ptl_pid);

	if (ctx->pid_count == 0) {
		dd_dev_info(dd, "release PID singleton [%u]\n",
			    ptl_pid);
		__hfi_ctx_unreserve(dd, ptl_pid, 1);
	}
	/* clear last */
	ctx->pid = HFI_PID_NONE;

	dd->stats.sps_ctxts--;
unlock:
	up_write(&ctx->ctx_rwsem);
	return ret;
}

int hfi_ctx_release(struct hfi_ctx *ctx)
{
	hfi_cmdq_cleanup(ctx);
	return hfi_ctx_cleanup(ctx);
}

/* returns number of hardware resources available to clients */
int hfi_get_hw_limits(struct hfi_ibcontext *uc, struct hfi_hw_limit *hwl)
{
	struct hfi_devdata *dd = uc->priv;

	hwl->num_pids_avail = HFI_NUM_USABLE_PIDS - dd->pid_num_assigned;
	hwl->num_cmdq_pairs_avail = HFI_CMDQ_COUNT - dd->cmdq_pair_num_assigned;

	/* FXRTODO: Tune limits per system config and enforce them */
	hwl->num_pids_limit = hwl->num_pids_avail;
	hwl->num_cmdq_pairs_limit = hwl->num_cmdq_pairs_avail;
	hwl->le_me_count_limit = HFI_LE_ME_MAX_COUNT;
	hwl->unexpected_count_limit = HFI_UNEXP_MAX_COUNT;
	hwl->trig_op_count_limit = HFI_TRIG_OP_MAX_COUNT;
	return 0;
}

static void __hfi_set_spill_area(struct hfi_devdata *dd, u8 spill_idx,
				 void *ptr, u64 size)
{
	u64 offset = spill_idx * 16;

	write_csr(dd, FXR_RXDMA_CFG_TO_BASE_TC0 + offset, (u64)ptr);
	write_csr(dd, FXR_RXDMA_CFG_TO_BOUNDS_TC0 + offset,
		  (u64)ptr + size);
}

int hfi_alloc_spill_area(struct hfi_devdata *dd)
{
	void *ptr;
	int i;
	u64 size = TRIGGER_OPS_SPILL_SIZE;

	ptr = vmalloc_node(size * 5, dd->node);

	if (!ptr)
		return -ENOMEM;

	hfi_at_reg_range(&dd->priv_ctx, ptr, size * 5, NULL, true);

	dd->trig_op_spill_area = ptr;

	for (i = 0; i < 5; i++)
		__hfi_set_spill_area(dd, i, ptr + size * i, size);
	return 0;
}

void hfi_free_spill_area(struct hfi_devdata *dd)
{
	int i;

	if (!dd->trig_op_spill_area)
		return;

	for (i = 0; i < 5; i++)
		__hfi_set_spill_area(dd, i, NULL, 0);

	hfi_at_dereg_range(&dd->priv_ctx, dd->trig_op_spill_area,
			   TRIGGER_OPS_SPILL_SIZE * 5);
	vfree(dd->trig_op_spill_area);
}
