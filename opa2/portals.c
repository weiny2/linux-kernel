/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2014 - 2015 Intel Corporation.
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
 * Copyright (c) 2014 - 2015 Intel Corporation.
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
 *
 * Intel(R) Omni-Path Gen2 HFI PCIe Driver
 */

#include <linux/bitmap.h>
#include <linux/sched.h>
#include "opa_hfi.h"

/* TODO - temporary as FXR model has no IOMMU yet */
static int ptl_phys_pages = 1;

static int hfi_pid_alloc(struct hfi_ctx *ctx, u16 *ptl_pid);
static void hfi_pid_free(struct hfi_devdata *dd, u16 ptl_pid);

static int hfi_cq_assign_next(struct hfi_ctx *ctx,
			      u16 ptl_pid, int *out_cq_idx)
{
	struct hfi_devdata *dd = ctx->devdata;
	unsigned cq_idx, num_cqs = HFI_CQ_COUNT;

#if 0
	/* TODO - for now allow just one CQ pair.
	 * We can introduce HFI limits later.
	 */
	if (ctx->cq_pair_num_assigned > 0)
		return -ENOSPC;
#endif

	/* search the whole CQ array, starting with next_unused */
	while (num_cqs > 0) {
		cq_idx = dd->cq_pair_next_unused++ % HFI_CQ_COUNT;
		if (dd->cq_pair[cq_idx] == HFI_PID_NONE)
			break;
		num_cqs--;
	}
	if (num_cqs == 0) {
		/* all CQs are assigned */
		return -EBUSY;
	}

	*out_cq_idx = cq_idx;
	dd->cq_pair[cq_idx] = ptl_pid;
	ctx->cq_pair_num_assigned++;
	return 0;
}

static int hfi_cq_validate_tuples(struct hfi_ctx *ctx,
				  struct hfi_auth_tuple *auth_table)
{
	int i, j;
	u32 auth_uid, last_job_uid = HFI_UID_ANY;

	/* validate auth_tuples */
	/* TODO - some rework here when we fully understand UID management */
	for (i = 0; i < HFI_NUM_AUTH_TUPLES; i++) {
		auth_uid = auth_table[i].uid;

		/* user may request to let driver select UID */
		if (auth_uid == HFI_UID_ANY || auth_uid == 0)
			auth_uid = auth_table[i].uid = ctx->ptl_uid;

		/* if job_launcher didn't set UIDs, this must match default */
		if (ctx->auth_mask == 0) {
			if (auth_uid != ctx->ptl_uid)
				return -EINVAL;
			continue;
		}

		/*
		 * else look for match in job_launcher set UIDs,
		 * but try to short-circuit this search.
		 */
		if (auth_uid == last_job_uid)
			continue;
		for (j = 0; j < HFI_NUM_AUTH_TUPLES; j++) {
			if (auth_uid == ctx->auth_uid[j]) {
				last_job_uid = auth_uid;
				break;
			}
		}
		if (j == HFI_NUM_AUTH_TUPLES)
			return -EINVAL;
	}

	return 0;
}

int hfi_cq_assign(struct hfi_ctx *ctx, struct hfi_auth_tuple *auth_table, u16 *cq_idx)
{
	struct hfi_devdata *dd = ctx->devdata;
	int tmp_cq_idx, ret;
	unsigned long flags;

	/* verify we are attached to Portals */
	if (ctx->ptl_pid == HFI_PID_NONE)
		return -EPERM;

	ret = hfi_cq_validate_tuples(ctx, auth_table);
	if (ret)
		return ret;

	spin_lock_irqsave(&dd->cq_lock, flags);
	ret = hfi_cq_assign_next(ctx, ctx->ptl_pid, &tmp_cq_idx);
	spin_unlock_irqrestore(&dd->cq_lock, flags);
	if (ret)
		return ret;

	dd_dev_info(dd, "CQ pair %u assigned\n", tmp_cq_idx);
	*cq_idx = tmp_cq_idx;

	/* write CQ config in HFI CSRs */
	hfi_cq_config(ctx, tmp_cq_idx, dd->cq_head_base, auth_table);

	return 0;
}

int hfi_cq_update(struct hfi_ctx *ctx, u16 cq_idx, struct hfi_auth_tuple *auth_table)
{
	struct hfi_devdata *dd = ctx->devdata;
	int ret = 0;

	/* verify we are attached to Portals */
	if (ctx->ptl_pid == HFI_PID_NONE)
		return -EPERM;

	/* verify we own specified CQ */
	if (ctx->ptl_pid != dd->cq_pair[cq_idx])
		return -EINVAL;

	ret = hfi_cq_validate_tuples(ctx, auth_table);
	if (ret)
		return ret;

	/* write CQ tuple config in HFI CSRs */
	hfi_cq_config_tuples(ctx, cq_idx, auth_table);

	return 0;
}

int hfi_cq_release(struct hfi_ctx *ctx, u16 cq_idx)
{
	struct hfi_devdata *dd = ctx->devdata;
	int ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&dd->cq_lock, flags);
	if (ctx->ptl_pid != dd->cq_pair[cq_idx]) {
		ret = -EINVAL;
	} else {
		hfi_cq_disable(dd, cq_idx);
		dd->cq_pair[cq_idx] = HFI_PID_NONE;
		ctx->cq_pair_num_assigned--;
		/* TODO - remove any CQ head mappings */
	}
	spin_unlock_irqrestore(&dd->cq_lock, flags);

	return ret;
}

static void hfi_cq_cleanup(struct hfi_ctx *ctx)
{
	struct hfi_devdata *dd = ctx->devdata;
	u64 ptl_pid = ctx->ptl_pid;
	int i;
	unsigned long flags;

	spin_lock_irqsave(&dd->cq_lock, flags);
	for (i = 0; i < HFI_CQ_COUNT; i++) {
		if (ptl_pid == dd->cq_pair[i]) {
			hfi_cq_disable(dd, i);
			dd->cq_pair[i] = HFI_PID_NONE;
		}
	}
	ctx->cq_pair_num_assigned = 0;
	spin_unlock_irqrestore(&dd->cq_lock, flags);
}

int hfi_eq_assign(struct hfi_ctx *ctx, struct hfi_eq_assign_args *eq_assign)
{
	union eqd *eq_desc_base = (void *)(ctx->ptl_state_base + HFI_PSB_EQ_DESC_OFFSET);
	union eqd *eq_desc;
	unsigned long flags;
	u16 eq_base;
	int eq_idx, msix_idx = 0;
	int num_eqs = HFI_NUM_EVENT_HANDLES;
	int ret = 0;

	eq_base = eq_assign->ni * num_eqs;
	if (eq_assign->ni >= HFI_NUM_NIS)
		return -EINVAL;
	/* TODO blocking mode coming soon... */
	if (eq_assign->mode & HFI_EQ_MODE_BLOCKING)
		return -EINVAL;
	/* TODO also validate queue base+count is mapped to user */
	if (eq_assign->base & (HFI_EQ_ALIGNMENT - 1))
		return -EFAULT;

	idr_preload(GFP_KERNEL);
	spin_lock_irqsave(&ctx->eq_lock, flags);
	eq_idx = idr_alloc(&ctx->eq_used, ctx, eq_base, eq_base + num_eqs, GFP_NOWAIT);
	if (eq_idx < 0) {
		/* all EQs are assigned */
		ret = -ENOSPC;
		goto idr_end;
	}

	/* set EQ descriptor in host memory. */
	eq_desc = &eq_desc_base[eq_idx];
	eq_desc->val[1] = 0; /* clear head/tail */
	eq_desc->order = eq_assign->order;
	eq_desc->start = (eq_assign->base >> 6);
	eq_desc->ni = eq_assign->ni;
	eq_desc->irq = msix_idx;
	eq_desc->i = (eq_assign->mode & HFI_EQ_MODE_BLOCKING);
	eq_desc->v = 1;
	/* TODO - need privileged CQ write to RX CmdQ to complete */

	 /* return index to user */
	eq_assign->eq_idx = eq_idx;

idr_end:
	spin_unlock_irqrestore(&ctx->eq_lock, flags);
	idr_preload_end();

	return ret;
}

int hfi_eq_release(struct hfi_ctx *ctx, u16 eq_idx)
{
	union eqd *eq_desc_base = (void *)(ctx->ptl_state_base + HFI_PSB_EQ_DESC_OFFSET);
	union eqd *eq_desc;
	void *eq_present;
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&ctx->eq_lock, flags);
	eq_present = idr_find(&ctx->eq_used, eq_idx);
	if (!eq_present) {
		ret = -EINVAL;
		goto idr_end;
	}

	eq_desc = &eq_desc_base[eq_idx];
	eq_desc->val[1] = 0; /* clear head/tail */
	eq_desc->val[0] = 0; /* clear valid */
	/* TODO - privileged CQ write to release EQ */

	idr_remove(&ctx->eq_used, eq_idx);
idr_end:
	spin_unlock_irqrestore(&ctx->eq_lock, flags);

	return ret;
}

static void hfi_eq_cleanup(struct hfi_ctx *ctx)
{
	unsigned long flags;

	spin_lock_irqsave(&ctx->eq_lock, flags);
	idr_destroy(&ctx->eq_used);
	spin_unlock_irqrestore(&ctx->eq_lock, flags);
}

/*
 * Reserves contiguous PIDs. Note, also used for orphan reservation which
 * do not touch ctx->[pid_base, pid_count].
 */
static int __hfi_ctxt_reserve(struct hfi_devdata *dd, u16 *base, u16 count)
{
	u16 start, n;
	int ret = 0;
	unsigned long flags;

	start = (*base == HFI_PID_ANY) ? 0 : *base;

	spin_lock_irqsave(&dd->ptl_lock, flags);
	n = bitmap_find_next_zero_area(dd->ptl_map, HFI_NUM_PIDS,
				       start, count, 0);
	if (n == -1)
		ret = -EBUSY;
	if (*base != HFI_PID_ANY && n != *base)
		ret = -EBUSY;
	if (ret) {
		spin_unlock_irqrestore(&dd->ptl_lock, flags);
		return ret;
	}
	bitmap_set(dd->ptl_map, n, count);
	spin_unlock_irqrestore(&dd->ptl_lock, flags);

	*base = n;
	return 0;
}

int hfi_ctxt_reserve(struct hfi_ctx *ctx, u16 *base, u16 count)
{
	struct hfi_devdata *dd = ctx->devdata;
	int ret;

	/* only one PID reservation */
	if (ctx->pid_count)
		return -EPERM;

	ret = __hfi_ctxt_reserve(dd, base, count);
	if (!ret) {
		ctx->pid_base = *base;
		ctx->pid_count = count;
	}
	return ret;
}

static void __hfi_ctxt_unreserve(struct hfi_devdata *dd, u16 base, u16 count)
{
	unsigned long flags;

	spin_lock_irqsave(&dd->ptl_lock, flags);
	bitmap_clear(dd->ptl_map, base, count);
	spin_unlock_irqrestore(&dd->ptl_lock, flags);
	return;
}

void hfi_ctxt_unreserve(struct hfi_ctx *ctx)
{
	struct hfi_devdata *dd = ctx->devdata;

	__hfi_ctxt_unreserve(dd, ctx->pid_base, ctx->pid_count);
	ctx->pid_base = -1;
	ctx->pid_count = 0;
}

/*
 * Associate this process with a Portals PID.
 * Note, hfi_open() sets:
 *   ctx->pid = current->pid
 *   ctx->ptl_pid = HFI_PID_NONE
 *   ctx->ptl_uid = current_uid()
 */
int hfi_ctxt_attach(struct hfi_ctx *ctx, struct opa_ctx_assign *ctx_assign)
{
	struct hfi_devdata *dd = ctx->devdata;
	u16 ptl_pid;
	u32 psb_size, trig_op_size, le_me_off, le_me_size, unexp_size;
	int ret;

	/* only one Portals PID allowed */
	if (ctx->ptl_pid != HFI_PID_NONE)
		return -EPERM;

	ptl_pid = ctx_assign->pid;
	ret = hfi_pid_alloc(ctx, &ptl_pid);
	if (ret)
		return ret;

	/* set ptl_pid, hfi_ctxt_cleanup() can handle all errors below */
	ctx->ptl_pid = ptl_pid;

	/* verify range of inputs */
	if (ctx_assign->trig_op_count > HFI_TRIG_OP_MAX_COUNT)
		ctx_assign->trig_op_count = HFI_TRIG_OP_MAX_COUNT;
	if (ctx_assign->le_me_count > HFI_LE_ME_MAX_COUNT)
		ctx_assign->le_me_count = HFI_LE_ME_MAX_COUNT;
	if (ctx_assign->unexpected_count > HFI_UNEXP_MAX_COUNT)
		ctx_assign->unexpected_count = HFI_UNEXP_MAX_COUNT;

	/* compute total Portals State Base size */
	trig_op_size = PAGE_ALIGN(ctx_assign->trig_op_count * HFI_TRIG_OP_SIZE);
	le_me_size = PAGE_ALIGN(ctx_assign->le_me_count * HFI_LE_ME_SIZE);
	unexp_size = PAGE_ALIGN(ctx_assign->unexpected_count * HFI_UNEXP_SIZE);
	le_me_off = HFI_PSB_FIXED_TOTAL_MEM + trig_op_size;
	psb_size = le_me_off + le_me_size + unexp_size;

	/* vmalloc Portals State memory, will store in PCB */
	if (!ptl_phys_pages)
		ctx->ptl_state_base = vmalloc_user(psb_size);
	else
		ctx->ptl_state_base = (void *)__get_free_pages(
							GFP_KERNEL | __GFP_ZERO,
							get_order(psb_size));
	if (!ctx->ptl_state_base) {
		ret = -ENOMEM;
		goto err_vmalloc;
	}
	ctx->ptl_state_size = psb_size;
	ctx->le_me_addr = (void *)(ctx->ptl_state_base + le_me_off);
	ctx->le_me_size = le_me_size;
	ctx->unexpected_size = unexp_size;
	ctx->trig_op_size = trig_op_size;
	idr_init(&ctx->eq_used);
	spin_lock_init(&ctx->eq_lock);

	dd_dev_info(dd, "Portals PID %u assigned PCB:[%d, %d, %d, %d]\n", ptl_pid,
		    psb_size, trig_op_size, le_me_size, unexp_size);

	/* set PASID entry for w/PASID translations */
	hfi_iommu_set_pasid(dd, current->mm, ptl_pid);

	/* write PCB (host memory) */
	hfi_pcb_write(ctx, ptl_pid, ptl_phys_pages);

	return 0;

err_vmalloc:
	hfi_ctxt_cleanup(ctx);
	return ret;
}

static int hfi_pid_alloc(struct hfi_ctx *ctx, u16 *assigned_pid)
{
	unsigned long flags;
	int ret;
	struct hfi_devdata *dd = ctx->devdata;
	u16 ptl_pid = *assigned_pid;

	if (ctx->pid_count) {
		/* assign PID from Portals PID reservation */
		if ((ptl_pid != HFI_PID_ANY) &&
		    (ptl_pid >= ctx->pid_count))
			return -EINVAL;

		spin_lock_irqsave(&dd->ptl_lock, flags);
		if (ptl_pid == HFI_PID_ANY) {
			/* if PID_ANY, search reserved PIDs for unused one */
			for (ptl_pid = 0; ptl_pid < ctx->pid_count; ptl_pid++) {
				if (dd->ptl_user[ctx->pid_base + ptl_pid] == NULL)
					break;
			}
			if (ptl_pid >= ctx->pid_count) {
				spin_unlock_irqrestore(&dd->ptl_lock, flags);
				return -EBUSY;
			}
			ptl_pid += ctx->pid_base;
		} else {
			ptl_pid += ctx->pid_base;
			if (dd->ptl_user[ptl_pid] != NULL) {
				spin_unlock_irqrestore(&dd->ptl_lock, flags);
				return -EBUSY;
			}
		}

		/* store PID hfi_ctx pointer */
		dd->ptl_user[ptl_pid] = ctx;
		spin_unlock_irqrestore(&dd->ptl_lock, flags);
	} else {
		/* PID is user-specified, validate and acquire */
		if ((ptl_pid != HFI_PID_ANY) &&
		    (ptl_pid >= HFI_NUM_PIDS))
			return -EINVAL;

		ret = __hfi_ctxt_reserve(dd, &ptl_pid, 1);
		if (ret)
			return ret;
		dd_dev_info(dd, "acquired PID orphan [%u]\n", ptl_pid);

		/* store PID hfi_ctx pointer */
		BUG_ON(dd->ptl_user[ptl_pid]);
		dd->ptl_user[ptl_pid] = ctx;
	}

	*assigned_pid = ptl_pid;
	return 0;
}

static void hfi_pid_free(struct hfi_devdata *dd, u16 ptl_pid)
{
	unsigned long flags;

	spin_lock_irqsave(&dd->ptl_lock, flags);
	hfi_pcb_reset(dd, ptl_pid);
	hfi_iommu_clear_pasid(dd, ptl_pid);
	dd->ptl_user[ptl_pid] = NULL;
	spin_unlock_irqrestore(&dd->ptl_lock, flags);
	dd_dev_info(dd, "Portals PID %u released\n", ptl_pid);
}

/*
 * Release Portals PID resources.
 * Called during close() or explicitly via CMD_CTXT_DETACH.
 */
void hfi_ctxt_cleanup(struct hfi_ctx *ctx)
{
	struct hfi_devdata *dd = ctx->devdata;
	u16 ptl_pid = ctx->ptl_pid;

	if (ptl_pid == HFI_PID_NONE)
		/* no assigned PID */
		return;

	/* verify PID is in correct state */
	BUG_ON(dd->ptl_user[ptl_pid] != ctx);

	/* first release any assigned CQs */
	hfi_cq_cleanup(ctx);

	/* release assigned PID */
	hfi_pid_free(dd, ptl_pid);

	/* release per-EQ resources */
	hfi_eq_cleanup(ctx);

	if (ctx->ptl_state_base) {
		if (!ptl_phys_pages)
			vfree(ctx->ptl_state_base);
		else
			free_pages((unsigned long)ctx->ptl_state_base,
				   get_order(ctx->ptl_state_size));
	}
	ctx->ptl_state_base = NULL;

	if (ctx->pid_count == 0) {
		dd_dev_info(dd, "release PID orphan [%u]\n", ptl_pid);
		__hfi_ctxt_unreserve(dd, ptl_pid, 1);
	}

	/* clear last */
	ctx->ptl_pid = HFI_PID_NONE;
}
