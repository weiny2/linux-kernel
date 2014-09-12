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
#include <linux/interrupt.h>
#include <linux/log2.h>
#include <linux/sched.h>
#include <rdma/opa_core.h>
#include <rdma/hfi_eq.h>
#include "opa_hfi.h"
#include <rdma/hfi_eq.h>

static uint cq_alloc_cyclic = 0;

static int hfi_pid_alloc(struct hfi_ctx *ctx, u16 *ptl_pid);
static void hfi_pid_free(struct hfi_devdata *dd, u16 ptl_pid);

/*
 * TODO - PID_ANY changed to 4095, as FXR PID is 12-bits.
 *        For temporary compatiblity with unit tests, also catch -1.
 */
#define IS_PID_ANY(x)	(x == HFI_PID_ANY || x == (u16)-1)

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

static int __hfi_cq_assign(struct hfi_ctx *ctx, u16 *cq_idx)
{
	struct hfi_devdata *dd = ctx->devdata;
	int ret;
	unsigned long flags;

	idr_preload(GFP_KERNEL);
	spin_lock_irqsave(&dd->cq_lock, flags);
	if (cq_alloc_cyclic)
		ret = idr_alloc_cyclic(&dd->cq_pair, ctx, 0, HFI_CQ_COUNT,
				       GFP_NOWAIT);
	else
		ret = idr_alloc(&dd->cq_pair, ctx, 0, HFI_CQ_COUNT, GFP_NOWAIT);
	spin_unlock_irqrestore(&dd->cq_lock, flags);
	idr_preload_end();
	if (ret < 0)
		return ret;

	*cq_idx = ret;
	dd_dev_info(dd, "CQ pair %u assigned\n", ret);

	/*
	 * TODO - this is a placeholder for tracking resource usage/limits.
	 *   Should be atomic or in locked region where compared against limit.
	 */
	ctx->cq_pair_num_assigned++;

	return 0;
}

int hfi_cq_assign(struct hfi_ctx *ctx, struct hfi_auth_tuple *auth_table, u16 *cq_idx)
{
	int ret;

	/* verify we are attached to Portals */
	if (ctx->pid == HFI_PID_NONE)
		return -EPERM;

	/*
	 * some kernel clients may not need to specify UID,SRANK
	 * if no auth_table, driver will set all to default
	 */
	if (auth_table) {
		ret = hfi_cq_validate_tuples(ctx, auth_table);
		if (ret)
			return ret;
	}

	ret = __hfi_cq_assign(ctx, cq_idx);
	if (!ret)
		hfi_cq_config(ctx, *cq_idx, auth_table, 1);
	return ret;
}

int hfi_cq_assign_privileged(struct hfi_ctx *ctx, u16 *cq_idx)
{
	int ret;

	/* verify system PID */
	if (ctx->pid != HFI_PID_SYSTEM)
		return -EPERM;

	ret = __hfi_cq_assign(ctx, cq_idx);
	if (!ret)
		hfi_cq_config(ctx, *cq_idx, NULL, 0);
	return ret;
}

int hfi_cq_update(struct hfi_ctx *ctx, u16 cq_idx, struct hfi_auth_tuple *auth_table)
{
	struct hfi_devdata *dd = ctx->devdata;
	struct hfi_ctx *cq_ctx;
	unsigned long flags;
	int ret = 0;

	/* verify we are attached to Portals */
	if (ctx->pid == HFI_PID_NONE)
		return -EPERM;

	/* verify we own specified CQ */
	spin_lock_irqsave(&dd->cq_lock, flags);
	cq_ctx = idr_find(&dd->cq_pair, cq_idx);
	spin_unlock_irqrestore(&dd->cq_lock, flags);
	if (cq_ctx != ctx)
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
	if (idr_find(&dd->cq_pair, cq_idx) != ctx) {
		ret = -EINVAL;
	} else {
		hfi_cq_disable(dd, cq_idx);
		idr_remove(&dd->cq_pair, cq_idx);
		ctx->cq_pair_num_assigned--;
		/* TODO - remove any CQ head mappings */
	}
	spin_unlock_irqrestore(&dd->cq_lock, flags);

	return ret;
}

void hfi_cq_cleanup(struct hfi_ctx *ctx)
{
	struct hfi_devdata *dd = ctx->devdata;
	struct hfi_ctx *cq_ctx;
	int i;
	unsigned long flags;

	spin_lock_irqsave(&dd->cq_lock, flags);
	idr_for_each_entry(&dd->cq_pair, cq_ctx, i) {
		if (cq_ctx == ctx) {
			hfi_cq_disable(dd, i);
			idr_remove(&dd->cq_pair, i);
		}
	}
	ctx->cq_pair_num_assigned = 0;
	spin_unlock_irqrestore(&dd->cq_lock, flags);
}

int hfi_cq_map(struct hfi_ctx *ctx, u16 cq_idx,
	       struct hfi_cq *tx, struct hfi_cq *rx)
{
	ssize_t head_size;
	int rc;

	/* stash pointer to CQ HEAD */
	rc = hfi_ctxt_hw_addr(ctx, TOK_CQ_HEAD, cq_idx,
			      (void **)&tx->head_addr, &head_size);
	if (rc)
		goto err1;

	/* stash pointer to TX CQ */
	rc = hfi_ctxt_hw_addr(ctx, TOK_CQ_TX, cq_idx,
			      &tx->base, (ssize_t *)&tx->size);
	if (rc)
		goto err1;

	tx->base = ioremap((u64)tx->base, tx->size);
	if (!tx->base)
		goto err1;

	/* stash pointer to RX CQ */
	rc = hfi_ctxt_hw_addr(ctx, TOK_CQ_RX, cq_idx,
			      &rx->base, (ssize_t *)&rx->size);
	if (rc)
		goto err2;
	rx->base = ioremap((u64)rx->base, rx->size);
	if (!rx->base)
		goto err2;

	tx->cq_idx = cq_idx;
	tx->slots_total = HFI_CQ_TX_ENTRIES;
	tx->slots_avail = tx->slots_total - 1;
	tx->slot_idx = (*tx->head_addr);
	tx->sw_head_idx = tx->slot_idx;

	rx->cq_idx = cq_idx;
	rx->head_addr = tx->head_addr + 8;
	rx->slots_total = HFI_CQ_RX_ENTRIES;
	rx->slots_avail = rx->slots_total - 1;
	rx->slot_idx = (*rx->head_addr);
	rx->sw_head_idx = rx->slot_idx;

	return 0;

err2:
	iounmap(tx->base);
err1:
	return rc;
}

void hfi_cq_unmap(struct hfi_cq *tx, struct hfi_cq *rx)
{
	if (tx->base)
		iounmap(tx->base);
	if (rx->base)
		iounmap(rx->base);
	tx->base = NULL;
	rx->base = NULL;
}

static int hfi_ct_assign(struct hfi_ctx *ctx, struct opa_ev_assign *ev_assign)
{
	union ptl_ct_event *ct_desc_base = (void *)(ctx->ptl_state_base + HFI_PSB_CT_OFFSET);
	union ptl_ct_event ct_desc = {.val = {0}};
	u16 ct_base;
	int ct_idx;
	int num_cts = HFI_NUM_CT_ENTRIES;
	int ret = 0;

	ct_base = ev_assign->ni * num_cts;
	if (ev_assign->ni >= HFI_NUM_NIS)
		return -EINVAL;
	/* TODO blocking mode coming soon... */
	if (ev_assign->mode & OPA_EV_MODE_BLOCKING)
		return -EINVAL;

	idr_preload(GFP_KERNEL);
	spin_lock(&ctx->cteq_lock);
	ct_idx = idr_alloc(&ctx->ct_used, ctx, ct_base, ct_base + num_cts, GFP_NOWAIT);
	if (ct_idx < 0) {
		/* all EQs are assigned */
		ret = ct_idx;
		goto idr_end;
	}

	/*
	 * set CT descriptor in host memory;
	 * this memory is cache coherent with HFI, does not use RX CMD
	 */
	ct_desc.threshold = ev_assign->threshold;
	ct_desc.ni = ev_assign->ni;
	ct_desc.irq = 0;
	ct_desc.i = (ev_assign->mode & OPA_EV_MODE_BLOCKING);
	ct_desc.v = 1;
	ct_desc_base[ct_idx].val[0] = ct_desc.val[0];
	ct_desc_base[ct_idx].val[1] = ct_desc.val[1];
	ct_desc_base[ct_idx].val[2] = ct_desc.val[2];
	wmb();  /* barrier before writing Valid */
	ct_desc_base[ct_idx].val[3] = ct_desc.val[3];

	 /* return index to user */
	ev_assign->ev_idx = ct_idx;

idr_end:
	spin_unlock(&ctx->cteq_lock);
	idr_preload_end();

	return ret;
}

static int hfi_ct_release(struct hfi_ctx *ctx, u16 ct_idx)
{
	union ptl_ct_event *ct_desc_base = (void *)(ctx->ptl_state_base + HFI_PSB_CT_OFFSET);
	void *ct_present;
	int ret = 0;

	spin_lock(&ctx->cteq_lock);
	ct_present = idr_find(&ctx->ct_used, ct_idx);
	if (!ct_present) {
		ret = -EINVAL;
		goto idr_end;
	}

	/* clear/invalidate CT */
	ct_desc_base[ct_idx].val[3] = 0;

	idr_remove(&ctx->ct_used, ct_idx);
idr_end:
	spin_unlock(&ctx->cteq_lock);

	return ret;
}

static int hfi_eq_assign(struct hfi_ctx *ctx, struct opa_ev_assign *eq_assign)
{
	struct hfi_devdata *dd = ctx->devdata;
	struct hfi_event_queue *eq = NULL;
	union eqd *eq_desc_base = (void *)(ctx->ptl_state_base +
				  HFI_PSB_EQ_DESC_OFFSET);
	union eqd eq_desc;
	u16 eq_base;
	int order, eq_idx, msix_idx = 0;
	int num_eqs = HFI_NUM_EVENT_HANDLES;
	int ret = 0;

	if (eq_assign->ni >= HFI_NUM_NIS)
		return -EINVAL;
	if (eq_assign->base & (HFI_EQ_ALIGNMENT - 1))
		return -EFAULT;
	if (!access_ok(VERIFY_WRITE, eq_assign->base,
		       eq_assign->size * HFI_EQ_ENTRY_SIZE))
		return -EFAULT;

	/* FXR requires EQ size as power of 2 */
	if (is_power_of_2(eq_assign->size))
		order = __ilog2_u64(eq_assign->size);
	else
		return -EINVAL;
	if (order > HFI_MAX_EVENT_ORDER)
		return -EINVAL;

	if (eq_assign->mode & OPA_EV_MODE_BLOCKING) {
		eq = kzalloc(sizeof(*eq), GFP_KERNEL);
		if (!eq)
			return -ENOMEM;
	}

	eq_base = eq_assign->ni * num_eqs;
	/*
	 * TODO - first EQ is reserved for result of EQ_DESC_WRITE.
	 * skip for now for kernel-clients until we copy user-client usage.
	 */
	if (ctx->type == HFI_CTX_TYPE_KERNEL)
		eq_base += 1;

	idr_preload(GFP_KERNEL);
	spin_lock(&ctx->cteq_lock);
	/* IDR contents differ based on blocking or non-blocking */
	eq_idx = idr_alloc(&ctx->eq_used, eq ? (void *)eq : (void *)ctx,
			   eq_base, eq_base + num_eqs, GFP_NOWAIT);
	if (eq_idx < 0) {
		/* all EQs are assigned */
		ret = eq_idx;
		kfree(eq);
		goto idr_end;
	}

	if (eq) {
		struct hfi_msix_entry *me;
		unsigned long flags;

		/* initialize EQ IRQ state */
		INIT_LIST_HEAD(&eq->irq_wait_chain);
		init_waitqueue_head(&eq->wq);

		/* for now just do round-robin assignment */
		msix_idx = atomic_inc_return(&dd->msix_eq_next) %
			   dd->num_eq_irqs;
		me = &dd->msix_entries[msix_idx];
		eq->irq_vector = msix_idx;
		/* add EQ to list of IRQ waiters */
		write_lock_irqsave(&me->irq_wait_lock, flags);
		list_add(&eq->irq_wait_chain, &me->irq_wait_head);
		write_unlock_irqrestore(&me->irq_wait_lock, flags);
	}

	/* set EQ descriptor in host memory */
	eq_desc.val[1] = 0; /* clear head/tail */
	eq_desc.order = order;
	eq_desc.start = (eq_assign->base >> 6);
	eq_desc.ni = eq_assign->ni;
	eq_desc.irq = msix_idx;
	eq_desc.i = (eq_assign->mode & OPA_EV_MODE_BLOCKING);
	eq_desc.v = 1;
	eq_desc_base[eq_idx].val[1] = eq_desc.val[1];
	wmb();  /* barrier before writing Valid */
	eq_desc_base[eq_idx].val[0] = eq_desc.val[0];

	/* issue write to privileged CQ to complete */
	ret = _hfi_eq_update_desc_cmd(ctx, &dd->priv_rx_cq, eq_idx, &eq_desc,
				      eq_assign->user_data);
	if (ret < 0) {
		/*
		 * TODO - need to define how to handle waiting for CQ slots,
		 *        (deferred processing to wait for CQ slots?).
		 */
		eq_desc_base[eq_idx].val[0] = 0; /* clear valid */
		idr_remove(&ctx->eq_used, eq_idx);
		goto idr_end;
	}

	 /* return index to user */
	eq_assign->ev_idx = eq_idx;

idr_end:
	spin_unlock(&ctx->cteq_lock);
	idr_preload_end();

	return ret;
}

/*
 * This function is called via EQ cleanup as a idr_for_each function, hence
 * the curious looking arguments.
 */
static int hfi_eq_irq_remove(int eq_idx, void *idr_ptr, void *idr_ctx)
{
	struct hfi_ctx *ctx = (struct hfi_ctx *)idr_ctx;

	/* test if blocking mode EQ */
	if (ctx != idr_ptr) {
		struct hfi_msix_entry *me;
		struct hfi_event_queue *eq = (struct hfi_event_queue *)idr_ptr;
		unsigned long flags;

		me = &ctx->devdata->msix_entries[eq->irq_vector];
		write_lock_irqsave(&me->irq_wait_lock, flags);
		if (!list_empty(&eq->irq_wait_chain))
			list_del(&eq->irq_wait_chain);
		write_unlock_irqrestore(&me->irq_wait_lock, flags);

		kfree(eq);
	}

	return 0;
}

static int hfi_eq_release(struct hfi_ctx *ctx, u16 eq_idx, u64 user_data)
{
	struct hfi_devdata *dd = ctx->devdata;
	union eqd *eq_desc_base = (void *)(ctx->ptl_state_base +
				  HFI_PSB_EQ_DESC_OFFSET);
	union eqd eq_desc;
	void *eq_present;
	int ret = 0;

	spin_lock(&ctx->cteq_lock);
	eq_present = idr_find(&ctx->eq_used, eq_idx);
	if (!eq_present) {
		ret = -EINVAL;
		goto idr_end;
	}

	/* remove any IRQ assignment */
	hfi_eq_irq_remove(eq_idx, eq_present, ctx);

	/* need cleared EQ desc to send via RX CQ */
	eq_desc.val[0] = 0;
	eq_desc.val[1] = 0;

	/* issue write to privileged CQ to complete EQ release */
	ret = _hfi_eq_update_desc_cmd(ctx, &dd->priv_rx_cq, eq_idx, &eq_desc,
				      user_data);
	if (ret < 0) {
		/* TODO - revisit how to handle waiting for CQ slots */
		goto idr_end;
	}

	eq_desc_base[eq_idx].val[0] = 0; /* clear valid */

	idr_remove(&ctx->eq_used, eq_idx);
idr_end:
	spin_unlock(&ctx->cteq_lock);

	return ret;
}

static int __hfi_eq_wait_condition(struct hfi_ctx *ctx, u16 eq_idx)
{
	/*
	 * TODO - using refactored hfi_eq.h to test for event.
	 * We should pin this page to prevent potential page fault here?
	 */
	u64 *eq_entry = NULL;
	eq_entry = _hfi_eq_next_entry(ctx, eq_idx);
	return (*eq_entry & TARGET_EQENTRY_V_MASK);
}

static int hfi_eq_wait_single(struct hfi_ctx *ctx, u16 eq_idx, long timeout)
{
	struct hfi_event_queue *eq = NULL;
	void *eq_present;
	int ret = 0;

	if (timeout < 0)
		timeout = MAX_SCHEDULE_TIMEOUT;
	else
		timeout = msecs_to_jiffies(timeout);

	spin_lock(&ctx->cteq_lock);
	eq_present = idr_find(&ctx->eq_used, eq_idx);
	if (!eq_present || eq_present == ctx) {
		ret = -EINVAL;
		goto idr_end;
	}

	eq = (struct hfi_event_queue *)eq_present;
idr_end:
	spin_unlock(&ctx->cteq_lock);

	if (eq) {
		/* TODO - increment waiter count? */
		ret = wait_event_interruptible_timeout(eq->wq,
					__hfi_eq_wait_condition(ctx, eq_idx),
					timeout);
		dd_dev_dbg(ctx->devdata, "wait_event returned %d\n", ret);
		if (ret == 0)	/* timeout */
			ret = -EAGAIN;
		else if (ret < 0)  /* interrupt, TODO - restartable? */
			ret = (timeout > 0) ? -EINTR : -ERESTARTSYS;
		else		/* success */
			ret = 0;
	}

	return ret;
}

int hfi_cteq_assign(struct hfi_ctx *ctx, struct opa_ev_assign *ev_assign)
{
	if (ev_assign->mode & OPA_EV_MODE_COUNTER)
		return hfi_ct_assign(ctx, ev_assign);
	else
		return hfi_eq_assign(ctx, ev_assign);
}

int hfi_cteq_release(struct hfi_ctx *ctx, u16 ev_mode, u16 ev_idx, u64 user_data)
{
	if (ev_mode & OPA_EV_MODE_COUNTER)
		return hfi_ct_release(ctx, ev_idx);
	else
		return hfi_eq_release(ctx, ev_idx, user_data);
}

int hfi_cteq_wait_single(struct hfi_ctx *ctx, u16 ev_mode, u16 ev_idx,
			 long timeout)
{
	if (ev_mode & OPA_EV_MODE_COUNTER)
		return -ENOSYS;
	else
		return hfi_eq_wait_single(ctx, ev_idx, timeout);
}

static void hfi_cteq_cleanup(struct hfi_ctx *ctx)
{
	spin_lock(&ctx->cteq_lock);
	idr_destroy(&ctx->ct_used);
	idr_for_each(&ctx->eq_used, hfi_eq_irq_remove, ctx);
	idr_destroy(&ctx->eq_used);
	spin_unlock(&ctx->cteq_lock);
}

/*
 * Reserves contiguous PIDs. Note, also used for orphan reservation which
 * do not touch ctx->[pid_base, pid_count].
 */
static int __hfi_ctxt_reserve(struct hfi_devdata *dd, u16 *base, u16 count,
			      u16 offset)
{
	u16 start, size, align, n;
	int ret = 0;
	unsigned long flags;

	if (IS_PID_ANY(*base)) {
		start = offset;
		/*
		 * user-space expectation is that multi-pid reservations
		 * have an even PID as the base of reservation
		 */
		align = (count > 1) ? 1 : 0;
	} else {
		/* honor request for base PID */
		start = offset + *base;
		align = 0;
	}
	/* last PID not usable as an RX context */
	size = HFI_NUM_PIDS - 1;

	spin_lock_irqsave(&dd->ptl_lock, flags);
	n = bitmap_find_next_zero_area(dd->ptl_map, size,
				       start, count, align);
	if (n >= size)
		ret = -EBUSY;
	if (!IS_PID_ANY(*base) && n != start)
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
	u16 offset;
	int ret;

	/* only one PID reservation */
	if (ctx->pid_count)
		return -EPERM;

	/*
	 * TODO: not sure if this is correct, depends on how we
	 * document interface for PSM to reserve block of PIDs.
	 */
	offset = (ctx->mode & HFI_CTX_MODE_BYPASS) ?
		 HFI_PID_BYPASS_BASE : 0;

	ret = __hfi_ctxt_reserve(dd, base, count, offset);
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

/* Initialize the Unexpected Free List */
static void hfi_uh_init(struct hfi_ctx *ctx, u16 unexpected_count)
{
	int idx;
	hfi_uh_t *p = ctx->le_me_addr + ctx->le_me_size;

	/* Add all the Unexpected Headers to the free list which begins in
	   UH[0] */
	for (idx = 0; idx < unexpected_count; idx++)
		p++->k.next = (idx+1) % unexpected_count;
}

/*
 * Associate this process with a Portals PID.
 * Note, hfi_open() sets:
 *   ctx->pid = HFI_PID_NONE
 *   ctx->ptl_uid = current_uid()
 */
int hfi_ctxt_attach(struct hfi_ctx *ctx, struct opa_ctx_assign *ctx_assign)
{
	struct hfi_devdata *dd = ctx->devdata;
	u16 ptl_pid;
	u32 psb_size, trig_op_size, le_me_off, le_me_size, unexp_size;
	int i, ni, ret;

	/* only one Portals PID allowed */
	if (ctx->pid != HFI_PID_NONE)
		return -EPERM;

	ptl_pid = ctx_assign->pid;
	ret = hfi_pid_alloc(ctx, &ptl_pid);
	if (ret)
		return ret;

	/* set ptl_pid, hfi_ctxt_cleanup() can handle all errors below */
	ctx->pid = ptl_pid;

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
	ctx->ptl_state_base = vmalloc_user(psb_size);
	if (!ctx->ptl_state_base) {
		ret = -ENOMEM;
		goto err_vmalloc;
	}
	ctx->ptl_state_size = psb_size;
	ctx->le_me_addr = (void *)(ctx->ptl_state_base + le_me_off);
	ctx->le_me_size = le_me_size;
	ctx->unexpected_size = unexp_size;
	ctx->trig_op_size = trig_op_size;
	idr_init(&ctx->ct_used);
	idr_init(&ctx->eq_used);
	spin_lock_init(&ctx->cteq_lock);

	dd_dev_info(dd, "Portals PID %u assigned PCB:[%d, %d, %d, %d]\n", ptl_pid,
		    psb_size, trig_op_size, le_me_size, unexp_size);

	/* set PASID entry for w/PASID translations */
	hfi_iommu_set_pasid(dd, (ctx->type == HFI_CTX_TYPE_USER) ?
			    current->mm : NULL, ptl_pid);

	/* write PCB (host memory) */
	hfi_pcb_write(ctx, ptl_pid);

	/* stash pointers to PCB segments (for kernel clients) */
	ret = hfi_ctxt_hw_addr(ctx, TOK_EVENTS_CT, ctx->pid,
			       &ctx->ct_addr, &ctx->ct_size);
	ret |= hfi_ctxt_hw_addr(ctx, TOK_EVENTS_EQ_DESC, ctx->pid,
				&ctx->eq_addr, &ctx->eq_size);
	ret |= hfi_ctxt_hw_addr(ctx, TOK_EVENTS_EQ_HEAD, ctx->pid,
				&ctx->eq_head_addr, &ctx->eq_head_size);
	ret |= hfi_ctxt_hw_addr(ctx, TOK_PORTALS_TABLE, ctx->pid,
				&ctx->pt_addr, &ctx->pt_size);
	/* above tokens do not return any error */
	BUG_ON(ret != 0);

	if (ctx->type == HFI_CTX_TYPE_KERNEL) {
		/* assign Status Registers (first 16 CTs) */
		struct opa_ev_assign ct_assign = {0};
		for (ni = 0; ni < HFI_NUM_NIS; ni++) {
			ct_assign.ni = ni;
			for (i = 0; i < HFI_NUM_CT_RESERVED; i++) {
				ret |= hfi_ct_assign(ctx, &ct_assign);
				ctx->status_reg[HFI_SR_INDEX(ni, i)] =
						ct_assign.ev_idx;
			}
		}
		/* no CTs assigned yet, so above cannot yield error */
		BUG_ON(ret != 0);
	}

	/* Initialize the Unexpected Free List */
	hfi_uh_init(ctx, ctx_assign->unexpected_count);

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
	u16 start, end, ptl_pid = *assigned_pid;

	if (ctx->pid_count) {
		/*
		 * Here there is already a PID reservation set by the job
		 * launcher (contiguous set of PIDs) with ops->ctx_reserve.
		 * Assign PID from existing Portals PID reservation.
		 * User can ask for driver assignment (-1) from within the
		 * reservation or ask for a specifc (logical) PID within the
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
			/* attempt to assign specific PID offset in reservation */
			start = ctx->pid_base + ptl_pid;
			end = ctx->pid_base + ptl_pid + 1;
		}
	} else {
		u16 offset = (ctx->mode & HFI_CTX_MODE_BYPASS) ?
			     HFI_PID_BYPASS_BASE : 0;

		/*
		 * Here PID is user-specified, there was not a job launcher
		 * supplied PID reservation.
		 * User can ask for driver assignment (-1) or ask for a
		 * specific PID.
		 */
		if (!IS_PID_ANY(ptl_pid) &&
		    (ptl_pid >= HFI_NUM_PIDS))
			return -EINVAL;

		/* No reservation, so must find unreserved PID first */
		ret = __hfi_ctxt_reserve(dd, &ptl_pid, 1, offset);
		if (ret)
			return ret;
		dd_dev_info(dd, "acquired PID orphan [%u]\n", ptl_pid);

		start = ptl_pid;
		end = ptl_pid + 1;
	}

	/* Now assign the PID and associate with this context */
	idr_preload(GFP_KERNEL);
	spin_lock_irqsave(&dd->ptl_lock, flags);
	ret = idr_alloc(&dd->ptl_user, ctx, start, end, GFP_NOWAIT);
	spin_unlock_irqrestore(&dd->ptl_lock, flags);
	idr_preload_end();

	if (ret < 0) {
		/*
		 * For the case where there was not already a PID reservation,
		 * then we reserved a single PID above and are marking assigned
		 * with IDR. IDR allocation cannot fail in this case unless
		 * there is PID state corruption due to a driver bug, hence
		 * the BUG_ON here.
		 */
		BUG_ON(!ctx->pid_count);
		return ret;
	}

	*assigned_pid = ret;
	return 0;
}

static void hfi_pid_free(struct hfi_devdata *dd, u16 ptl_pid)
{
	unsigned long flags;

	spin_lock_irqsave(&dd->ptl_lock, flags);
	hfi_pcb_reset(dd, ptl_pid);
	hfi_iommu_clear_pasid(dd, ptl_pid);
	idr_remove(&dd->ptl_user, ptl_pid);
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
	u16 ptl_pid = ctx->pid;
	unsigned long flags;

	if (ptl_pid == HFI_PID_NONE)
		/* no assigned PID */
		return;

	/* verify PID state is not corrupted  */
	spin_lock_irqsave(&dd->ptl_lock, flags);
	BUG_ON(idr_find(&dd->ptl_user, ptl_pid) != ctx);
	spin_unlock_irqrestore(&dd->ptl_lock, flags);

	/* first release any assigned CQs */
	hfi_cq_cleanup(ctx);

	/* release assigned PID */
	hfi_pid_free(dd, ptl_pid);

	/* release per CT and EQ resources */
	hfi_cteq_cleanup(ctx);

	if (ctx->ptl_state_base) {
		vfree(ctx->ptl_state_base);
		ctx->ptl_state_base = NULL;
	}

	if (ctx->pid_count == 0) {
		dd_dev_info(dd, "release PID orphan [%u]\n", ptl_pid);
		__hfi_ctxt_unreserve(dd, ptl_pid, 1);
	}

	/* clear last */
	ctx->pid = HFI_PID_NONE;
}
