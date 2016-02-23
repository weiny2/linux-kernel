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
#include <linux/interrupt.h>
#include <linux/log2.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/vmalloc.h>
#include <rdma/opa_core.h>
#include <rdma/hfi_eq.h>
#include "opa_hfi.h"
#include <rdma/hfi_eq.h>

static uint cq_alloc_cyclic;

static int hfi_pid_alloc(struct hfi_ctx *ctx, u16 *ptl_pid);
static void hfi_pid_free(struct hfi_devdata *dd, u16 ptl_pid);

/*
 * PID_ANY changed to 4095, as FXR PID is 12-bits.
 * We also consider -1 to mean PID_ANY assignment.
 */
#define IS_PID_ANY(x)	(x == HFI_PID_ANY || x == (u16)-1)

static int hfi_cq_validate_tuples(struct hfi_ctx *ctx,
				  struct hfi_auth_tuple *auth_table)
{
	int i, j;
	u32 auth_uid, last_job_uid = HFI_UID_ANY;

	/* validate auth_tuples */
	for (i = 0; i < HFI_NUM_AUTH_TUPLES; i++) {
		auth_uid = auth_table[i].uid;

		/* user may request to let driver select UID */
		if (auth_uid == HFI_UID_ANY || auth_uid == 0) {
			auth_table[i].uid = ctx->ptl_uid;
			auth_uid = ctx->ptl_uid;
		}

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
	if (ret >= 0)
		dd->cq_pair_num_assigned++;
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

int hfi_cq_assign(struct hfi_ctx *ctx, struct hfi_auth_tuple *auth_table,
		  u16 *cq_idx)
{
	int ret;
	bool priv;

	/* verify we are attached to Portals */
	if (ctx->pid == HFI_PID_NONE)
		return -EPERM;

	/* General and OFED DMA commands require privileged CQ */
	priv = (ctx->type == HFI_CTX_TYPE_KERNEL) &&
	       (ctx->mode & HFI_CTX_MODE_BYPASS_MASK);

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
		hfi_cq_config(ctx, *cq_idx, auth_table, !priv);
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

int hfi_cq_update(struct hfi_ctx *ctx, u16 cq_idx,
		  struct hfi_auth_tuple *auth_table)
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
		dd->cq_pair_num_assigned--;
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
			ctx->cq_pair_num_assigned--;
			dd->cq_pair_num_assigned--;
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
	union ptl_ct_event *ct_desc_base = (void *)(ctx->ptl_state_base
						    + HFI_PSB_CT_OFFSET);
	union ptl_ct_event ct_desc = {.val = {0} };
	u16 ct_base;
	int ct_idx;
	int num_cts = HFI_NUM_CT_ENTRIES;
	int ret = 0;

	ct_base = ev_assign->ni * num_cts;
	if (ev_assign->ni >= HFI_NUM_NIS)
		return -EINVAL;
	if (ev_assign->mode & OPA_EV_MODE_BLOCKING)
		return -EINVAL;

	idr_preload(GFP_KERNEL);
	spin_lock(&ctx->cteq_lock);
	ct_idx = idr_alloc(&ctx->ct_used, ctx, ct_base, ct_base + num_cts,
			   GFP_NOWAIT);
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
	union ptl_ct_event *ct_desc_base = (void *)(ctx->ptl_state_base
						    + HFI_PSB_CT_OFFSET);
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
	unsigned long sflags;
	int order, eq_idx, msix_idx = 0;
	int num_eqs = HFI_NUM_EVENT_HANDLES;
	int ret = 0;

	if (eq_assign->ni >= HFI_NUM_NIS)
		return -EINVAL;
	if (eq_assign->base & (HFI_EQ_ALIGNMENT - 1))
		return -EFAULT;

	/* FXR requires EQ size as power of 2 */
	if (is_power_of_2(eq_assign->count))
		order = __ilog2_u64(eq_assign->count);
	else
		return -EINVAL;
	if ((order < HFI_MIN_EVENT_ORDER) ||
	    (order > HFI_MAX_EVENT_ORDER))
		return -EINVAL;

	if (eq_assign->mode & OPA_EV_MODE_BLOCKING) {
		eq = kzalloc(sizeof(*eq), GFP_KERNEL);
		if (!eq)
			return -ENOMEM;
	}

	eq_base = eq_assign->ni * num_eqs;

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
		eq->isr_cb = eq_assign->isr_cb;
		eq->cookie = eq_assign->cookie;

		/* record info needed for hfi_eq_peek() */
		eq->desc.base = (void *)eq_assign->base;
		eq->desc.count = eq_assign->count;
		eq->desc.idx = eq_idx;

		/* add EQ to list of IRQ waiters */
		write_lock_irqsave(&me->irq_wait_lock, flags);
		list_add(&eq->irq_wait_chain, &me->irq_wait_head);
		write_unlock_irqrestore(&me->irq_wait_lock, flags);
	}

	/* set EQ descriptor in host memory */
	eq_desc.val[1] = 0; /* clear head/tail */
	eq_desc.order = order;
	eq_desc.start = (eq_assign->base >> PAGE_SHIFT);
	eq_desc.ni = eq_assign->ni;
	eq_desc.irq = msix_idx;
	eq_desc.i = (eq_assign->mode & OPA_EV_MODE_BLOCKING);
	eq_desc.v = 1;
	eq_desc_base[eq_idx].val[1] = eq_desc.val[1];
	wmb();  /* barrier before writing Valid */
	eq_desc_base[eq_idx].val[0] = eq_desc.val[0];

	spin_lock_irqsave(&dd->priv_rx_cq_lock, sflags);
	/* issue write to privileged CQ to complete */
	ret = _hfi_eq_update_desc_cmd(ctx, &dd->priv_rx_cq, eq_idx,
				      &eq_desc,
				      eq_assign->user_data);
	spin_unlock_irqrestore(&dd->priv_rx_cq_lock, sflags);
	if (ret < 0) {
		/*
		 * TODO - handle waiting for CQ slots,
		 * (deferred processing to wait for CQ slots?)
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

/* Block for an EQ zero event interrupt */
static int hfi_eq_zero_event_wait(struct hfi_ctx *ctx, u64 **eq_entry)
{
	int rc;

	rc = hfi_eq_wait_irq(ctx, &ctx->eq_zero[0], -1, eq_entry);
	if (rc == -EAGAIN || rc == -ERESTARTSYS)
		/* timeout or wait interrupted, not abnormal */
		rc = 0;
	else if (rc == HFI_EQ_DROPPED)
		/* driver bug with EQ sizing or IRQ logic */
		rc = -EIO;
	return rc;
}

static void hfi_e2e_cache_invalidate(struct ida *cache, u32 lid)
{
	ida_simple_get(cache, lid, lid + 1,
			    GFP_KERNEL);
	ida_simple_remove(cache, lid);

	return;
}

/* kernel thread handling EQ0 events for the system PID */
static int hfi_eq_zero_thread(void *data)
{
	struct hfi_ctx *ctx = data;
	struct hfi_devdata *dd = ctx->devdata;
	u64 *eq_entry;
	union target_EQEntry *rxe;
	int rc;
	u8 port, tc;
	u32 slid;
	struct hfi_ptcdata *ptc;
	struct ida *cache_rx;
	struct ida *cache_tx;

	allow_signal(SIGINT);

	while (!kthread_should_stop()) {
		eq_entry = NULL;
		rc = hfi_eq_zero_event_wait(ctx, &eq_entry);
		if (rc < 0) {
			/* only likely error is DROPPED event */
			dd_dev_err(dd, "%s unexpected EQ wait error %d\n",
				   __func__, rc);
			break;
		}
		if (!eq_entry)
			continue;

		rxe = (union target_EQEntry *)eq_entry;
		port = rxe->pt;
		tc = HFI_GET_TC(rxe->user_ptr);
		slid = rxe->initiator_id;

		switch (rxe->event_kind) {
		case PTL_EVENT_DISCONNECT:
			dd_dev_dbg(dd, "%s E2E disconnect request\n",
					__func__);

			if (tc >= HFI_MAX_TC) {
				dd_dev_err(dd, "%s unexpected tc %d\n",
				   __func__, tc);
				break;
			}
			ptc = &dd->pport[port].ptc[tc];
			cache_rx = &ptc->e2e_rx_state_cache;
			cache_tx = &ptc->e2e_tx_state_cache;

			hfi_e2e_cache_invalidate(cache_rx, slid);

			mutex_lock(&dd->e2e_lock);
			hfi_e2e_cache_invalidate(cache_tx, slid);
			mutex_unlock(&dd->e2e_lock);

			dd_dev_info(dd, "%s ev %d pt %d lid %lld tc %d\n",
				    __func__, rxe->event_kind, rxe->pt,
				    (u64)rxe->initiator_id, tc);
			break;

		case PTL_EVENT_TARGET_CONNECT:
			dd_dev_dbg(dd, "%s E2E connect request\n",
					__func__);

			if (tc >= HFI_MAX_TC) {
				dd_dev_err(dd, "%s unexpected tc %d\n",
				   __func__, tc);
				break;
			}
			ptc = &dd->pport[port].ptc[tc];
			cache_rx = &ptc->e2e_rx_state_cache;
			cache_tx = &ptc->e2e_tx_state_cache;

			rc = ida_simple_get(cache_rx, slid, slid + 1,
					    GFP_KERNEL);

			if (-ENOSPC == rc) {
				dd_dev_dbg(dd, "%s E2E Crashed node reconnect\n",
					   __func__);

				/*
				 * The node requesting this connect, went down
				 * without a disconnect request. Therfore the
				 * TX E2E connection has to be re established
				 * hence the TX cache invalidate.
				 */
				mutex_lock(&dd->e2e_lock);
				hfi_e2e_cache_invalidate(cache_tx, slid);
				mutex_unlock(&dd->e2e_lock);
			}

			dd_dev_info(dd, "%s ev %d pt %d lid %lld tc %d\n",
				    __func__, rxe->event_kind, rxe->pt,
				    (u64)rxe->initiator_id, tc);
			break;
		case PTL_CMD_COMPLETE:
			/* These events are handled synchronously */
			break;
		default:
			dd_dev_err(dd, "%s unexpected event %d port %d\n",
				   __func__, rxe->event_kind, rxe->pt);
			break;
		}

		if (rxe->event_kind != PTL_CMD_COMPLETE)
			hfi_eq_advance(ctx, &dd->priv_rx_cq,
				       &ctx->eq_zero[0], eq_entry);
	}
	return 0;
}

int hfi_eq_zero_assign(struct hfi_ctx *ctx)
{
	struct opa_ev_assign eq_assign = {0};
	int ni, ret;
	u32 *eq_head_array, *eq_head_addr;

	for (ni = 0; ni < HFI_NUM_NIS; ni++) {
		eq_assign.ni = ni;
		/* EQ is one page and meets 64B alignment */
		eq_assign.count = 64;
		eq_assign.base = (u64)kzalloc(eq_assign.count *
					      HFI_EQ_ENTRY_SIZE,
					      GFP_KERNEL);
		if (!eq_assign.base)
			return -ENOMEM;
		if ((ctx->pid == HFI_PID_SYSTEM) && !ni)
			/*
			 * system PID EQ 0 needs to handle
			 * E2E connect/destroy events
			 */
			eq_assign.mode = OPA_EV_MODE_BLOCKING;
		else
			eq_assign.mode = 0x0;
		ret = hfi_eq_assign(ctx, &eq_assign);
		if (ret)
			return ret;
		ctx->eq_zero[ni].base = (void *)eq_assign.base;
		ctx->eq_zero[ni].count = eq_assign.count;
		ctx->eq_zero[ni].idx = eq_assign.ev_idx;
		eq_head_array = ctx->eq_head_addr;
		/* Reset the EQ SW head */
		eq_head_addr = &eq_head_array[eq_assign.ev_idx];
		*eq_head_addr = 0;
	}

	/* TODO - ensure that EQ0 is created before events are delivered */
	mdelay(2);

	return 0;
}

int hfi_eq_zero_assign_privileged(struct hfi_ctx *ctx)
{
	int rc;
	struct hfi_devdata *dd = ctx->devdata;

	/* verify system PID */
	if (ctx->pid != HFI_PID_SYSTEM)
		return -EPERM;

	rc = hfi_eq_zero_assign(ctx);
	if (rc)
		return rc;

	dd->eq_zero_thread = kthread_run(hfi_eq_zero_thread, ctx,
					 "hfi_eq_zero%d", dd->unit);
	if (IS_ERR(dd->eq_zero_thread))
		rc = PTR_ERR(dd->eq_zero_thread);

	return rc;
}

int hfi_e2e_eq_assign(struct hfi_ctx *ctx)
{
	struct hfi_devdata *dd = ctx->devdata;
	struct opa_ev_assign eq_assign = {0};
	int ret;
	u32 *eq_head_array, *eq_head_addr;
	u64 *eq_entry = NULL;

	eq_assign.ni = PTL_NONMATCHING_PHYSICAL;
	/* EQ is one page and meets 64B alignment */
	eq_assign.count = 64;
	eq_assign.base = (u64)kzalloc(eq_assign.count *
				      HFI_EQ_ENTRY_SIZE,
				      GFP_KERNEL);
	if (!eq_assign.base)
		return -ENOMEM;
	ret = hfi_eq_assign(ctx, &eq_assign);
	if (ret)
		return ret;
	dd->e2e_eq.base = (void *)eq_assign.base;
	dd->e2e_eq.count = eq_assign.count;
	dd->e2e_eq.idx = eq_assign.ev_idx;
	eq_head_array = ctx->eq_head_addr;
	/* Reset the EQ SW head */
	eq_head_addr = &eq_head_array[eq_assign.ev_idx];
	*eq_head_addr = 0;

	/* Check on EQ 0 NI 0 for a PTL_CMD_COMPLETE event */
	hfi_eq_wait_timed(ctx, &ctx->eq_zero[0], HFI_EQ_WAIT_TIMEOUT_MS,
			  &eq_entry);
	if (eq_entry)
		hfi_eq_advance(ctx, &ctx->devdata->priv_rx_cq,
			       &ctx->eq_zero[0], eq_entry);
	else
		return -EIO;

	return 0;
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
	unsigned long sflags;
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

	spin_lock_irqsave(&dd->priv_rx_cq_lock, sflags);
	/* issue write to privileged CQ to complete EQ release */
	ret = _hfi_eq_update_desc_cmd(ctx, &dd->priv_rx_cq, eq_idx, &eq_desc,
				      user_data);
	spin_unlock_irqrestore(&dd->priv_rx_cq_lock, sflags);
	if (ret < 0) {
		/* TODO - revisit how to handle waiting for CQ slots */
		goto idr_end;
	}
	/* FXRTODO: Ensure the command above has completed by polling EQ 0 */
	mdelay(1);

	eq_desc_base[eq_idx].val[0] = 0; /* clear valid */

	idr_remove(&ctx->eq_used, eq_idx);
idr_end:
	spin_unlock(&ctx->cteq_lock);

	return ret;
}

void hfi_eq_zero_release(struct hfi_ctx *ctx)
{
	int ni, rc;
	struct hfi_devdata *dd = ctx->devdata;

	if (ctx == &dd->priv_ctx &&
	    !IS_ERR_OR_NULL(dd->eq_zero_thread)) {
		rc = send_sig(SIGINT, dd->eq_zero_thread, 0);
		if (rc) {
			dd_dev_err(dd, "send_sig failed %d\n", rc);
			return;
		}
		kthread_stop(dd->eq_zero_thread);
		dd->eq_zero_thread = NULL;
	}
	for (ni = 0; ni < HFI_NUM_NIS; ni++) {
		if (ctx->eq_zero[ni].base) {
			hfi_eq_release(ctx, ni * HFI_NUM_EVENT_HANDLES, 0);
			kfree(ctx->eq_zero[ni].base);
			ctx->eq_zero[ni].base = NULL;
		}
	}
}

void hfi_e2e_eq_release(struct hfi_ctx *ctx)
{
	struct hfi_devdata *dd = ctx->devdata;

	if (dd->e2e_eq.base) {
		hfi_eq_release(ctx, dd->e2e_eq.idx, 0x0);
		kfree(dd->e2e_eq.base);
		dd->e2e_eq.base = NULL;
	}
}

/* Returns true if there is a valid pending event and false otherwise */
static bool __hfi_eq_wait_condition(struct hfi_ctx *ctx, struct hfi_eq *eq)
{
	u64 *entry;
	int ret = hfi_eq_peek(ctx, eq, &entry);

	if (ret >= 0 && ret != HFI_EQ_EMPTY)
		return true;
	return false;
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
					__hfi_eq_wait_condition(ctx, &eq->desc),
					timeout);
		dd_dev_dbg(ctx->devdata, "wait_event returned %d\n", ret);
		if (ret == 0)	/* timeout */
			ret = -EAGAIN;
		else if (ret != -ERESTARTSYS)  /* success! */
			ret = 0;
		/* else interrupt, return -ERESTARTSYS */
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
 * Reserves contiguous PIDs. Note, also used for singleton reservation which
 * do not touch ctx->[pid_base, pid_count].
 */
static int __hfi_ctxt_reserve(struct hfi_devdata *dd, u16 *base, u16 count,
			      u16 align, u16 offset, u16 size)
{
	u16 start, n;
	int ret = 0;
	unsigned long align_mask = 0;

	if (size > HFI_NUM_USABLE_PIDS)
		return -EINVAL;

	if (align) {
		if (!is_power_of_2(align))
			return -EINVAL;
		align_mask = align - 1;
	}

	if (IS_PID_ANY(*base)) {
		start = offset;
	} else {
		/* honor request for base PID */
		start = offset + *base;
	}

	spin_lock(&dd->ptl_lock);
	n = bitmap_find_next_zero_area(dd->ptl_map, size,
				       start, count, align_mask);
	if (n >= size)
		ret = -EBUSY;
	else if (!IS_PID_ANY(*base) && n != start)
		ret = -EBUSY;
	else {
		bitmap_set(dd->ptl_map, n, count);
		*base = n;
	}
	spin_unlock(&dd->ptl_lock);

	return ret;
}

int hfi_ctxt_set_allowed_uids(struct hfi_ctx *ctx, u32 *auth_uid,
			      u8 num_uids)
{
	struct hfi_devdata *dd = ctx->devdata;
	struct hfi_ctx *tpid_ctx;
	int i, ret = 0;

	if (num_uids > HFI_NUM_AUTH_TUPLES)
		return -EINVAL;

	/*
	 * verify list of UIDs does not conflict with UIDs already in TPID_CAM
	 * table in order to prevent remapping into another job's PID range.
	 */
	spin_lock(&dd->ptl_lock);
	idr_for_each_entry(&dd->ptl_tpid, tpid_ctx, i) {
		for (i = 0; i < num_uids; i++) {
			if (auth_uid[i] &&
			    TPID_UID(tpid_ctx) == auth_uid[i]) {
				ret = -EACCES;
				break;
			}
		}
		if (ret < 0)
			break;
	}
	spin_unlock(&dd->ptl_lock);
	if (ret < 0)
		return ret;

	/* store AUTH UIDs in hfi_ctx */
	for (i = 0; i < num_uids; i++) {
		if (auth_uid[i]) {
			ctx->auth_mask |= (1 << i);
			ctx->auth_uid[i] = auth_uid[i];
		}
	}

	return 0;
}

static int hfi_ctxt_set_virtual_pid_range(struct hfi_ctx *ctx)
{
	struct hfi_devdata *dd = ctx->devdata;
	struct hfi_ctx *tpid_ctx;
	int tpid_idx, i, ret = 0;

	/* Find if we have available TPID_CAM entry, first verify is unique */
	idr_preload(GFP_KERNEL);
	spin_lock(&dd->ptl_lock);
	idr_for_each_entry(&dd->ptl_tpid, tpid_ctx, i) {
		if (TPID_UID(tpid_ctx) == TPID_UID(ctx)) {
			ret = -EACCES;
			goto idr_done;
		}
	}

	tpid_idx = idr_alloc(&dd->ptl_tpid, ctx, 0, HFI_TPID_ENTRIES, GFP_NOWAIT);
	if (tpid_idx < 0)
		ret = -EBUSY;
idr_done:
	spin_unlock(&dd->ptl_lock);
	idr_preload_end();
	if (ret)
		return ret;

	/* program TPID_CAM for our reserved PID range */
	hfi_tpid_enable(dd, tpid_idx, ctx->pid_base, TPID_UID(ctx));
	ctx->tpid_idx = tpid_idx;
	ctx->mode |= HFI_CTX_MODE_PID_VIRTUALIZED;
	return 0;
}

static void hfi_ctxt_unset_virtual_pid_range(struct hfi_ctx *ctx)
{
	struct hfi_devdata *dd = ctx->devdata;

	if (!IS_PID_VIRTUALIZED(ctx))
		return;

	hfi_tpid_disable(dd, ctx->tpid_idx);

	spin_lock(&dd->ptl_lock);
	idr_remove(&dd->ptl_tpid, ctx->tpid_idx);
	spin_unlock(&dd->ptl_lock);
	ctx->mode &= ~HFI_CTX_MODE_PID_VIRTUALIZED;
}

int hfi_ctxt_reserve(struct hfi_ctx *ctx, u16 *base, u16 count, u16 align,
		     u16 mode)
{
	struct hfi_devdata *dd = ctx->devdata;
	u16 offset = 0, size = HFI_NUM_USABLE_PIDS;
	int ret;

	/* only one PID reservation */
	if (ctx->pid_count)
		return -EPERM;

	/*
	 * TODO: not sure if this is correct, depends on how we
	 * document interface for PSM to reserve block of PIDs.
	 */
	if (ctx->mode & HFI_CTX_MODE_BYPASS_MASK) {
		offset = HFI_PID_BYPASS_BASE;
		size = offset + HFI_NUM_BYPASS_PIDS;
	}

	ret = __hfi_ctxt_reserve(dd, base, count, align, offset, size);
	if (!ret) {
		ctx->pid_base = *base;
		ctx->pid_count = count;
	}

	if (mode & HFI_CTX_MODE_PID_VIRTUALIZED) {
		ret = hfi_ctxt_set_virtual_pid_range(ctx);
		if (ret) {
			hfi_ctxt_unreserve(ctx);
		}
	}

	return ret;
}

static void __hfi_ctxt_unreserve(struct hfi_devdata *dd, u16 base, u16 count)
{
	spin_lock(&dd->ptl_lock);
	bitmap_clear(dd->ptl_map, base, count);
	spin_unlock(&dd->ptl_lock);
	return;
}

void hfi_ctxt_unreserve(struct hfi_ctx *ctx)
{
	struct hfi_devdata *dd = ctx->devdata;

	if (IS_PID_VIRTUALIZED(ctx))
		hfi_ctxt_unset_virtual_pid_range(ctx);

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

	/* if requested (by PSM), allocate PID from BYPASS range */
	if (ctx_assign->flags & HFI_CTX_MODE_USE_BYPASS)
		ctx->mode |= (HFI_CTX_MODE_USE_BYPASS | HFI_CTX_MODE_BYPASS_9B);

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
	ctx->ptl_state_base = vmalloc_user(psb_size);
	if (!ctx->ptl_state_base) {
		ret = -ENOMEM;
		goto err_vmalloc;
	}
	ctx->ptl_state_size = psb_size;
	ctx->le_me_addr = (void *)(ctx->ptl_state_base + le_me_off);
	ctx->le_me_size = le_me_size;
	ctx->le_me_count = ctx_assign->le_me_count;
	/* Initialize ME/LE pool used by hfi_me_alloc/free */
	ctx->le_me_free_index = 0;
	if (ctx->le_me_count && ctx->type == HFI_CTX_TYPE_KERNEL) {
		ctx->le_me_free_list = vzalloc(sizeof(uint16_t) *
					       (ctx->le_me_count - 1));
		if (!ctx->le_me_free_list)
			goto err_psb_vmalloc;

		for (i = 0; i < ctx->le_me_count - 1; i++)
			/* Entry 0 is reserved, so start from 1. */
			ctx->le_me_free_list[i] = i + 1;
	}

	ctx->unexpected_size = unexp_size;
	ctx->trig_op_size = trig_op_size;
	idr_init(&ctx->ct_used);
	idr_init(&ctx->eq_used);
	spin_lock_init(&ctx->cteq_lock);

	dd_dev_info(dd, "Portals PID %u assigned PCB:[%d, %d, %d, %d]\n", ptl_pid,
		    psb_size, trig_op_size, le_me_size, unexp_size);

	/* set PASID entry for w/PASID translations */
	hfi_iommu_set_pasid(ctx);

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
		/* EQ0 for system PID is assigned after the RX CQ is enabled */
		if (HFI_PID_SYSTEM != ctx->pid)
			ret = hfi_eq_zero_assign(ctx);
		/* no EQs assigned yet, so above cannot yield error */
		BUG_ON(ret != 0);

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

		if (ctx->mode & HFI_CTX_MODE_BYPASS_MASK)
			hfi_ctxt_set_bypass(ctx);
	}

	/* Initialize the Unexpected Free List */
	hfi_uh_init(ctx, ctx_assign->unexpected_count);

	return 0;

err_psb_vmalloc:
	vfree(ctx->ptl_state_base);
err_vmalloc:
	hfi_pid_free(dd, ptl_pid);
	return ret;
}

static int hfi_pid_alloc(struct hfi_ctx *ctx, u16 *assigned_pid)
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
		u16 offset = 0, size = HFI_NUM_USABLE_PIDS;

		if (ctx->mode & HFI_CTX_MODE_BYPASS_MASK) {
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
		ret = __hfi_ctxt_reserve(dd, &ptl_pid, 1, 0, offset, size);
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
void hfi_ctxt_cleanup(struct hfi_ctx *ctx)
{
	struct hfi_devdata *dd = ctx->devdata;
	u16 ptl_pid = ctx->pid;

	if (ptl_pid == HFI_PID_NONE)
		/* no assigned PID */
		return;

	/* verify PID state is not corrupted  */
	spin_lock(&dd->ptl_lock);
	BUG_ON(idr_find(&dd->ptl_user, ptl_pid) != ctx);
	spin_unlock(&dd->ptl_lock);

	/* release EQ 0 in each NI */
	hfi_eq_zero_release(ctx);

	/* first release any assigned CQs */
	hfi_cq_cleanup(ctx);

	/* reset PCB (host memory) */
	hfi_pcb_reset(dd, ptl_pid);

	/* stop pasid translation */
	hfi_iommu_clear_pasid(ctx);

	/* release assigned PID */
	hfi_pid_free(dd, ptl_pid);

	/* release per CT and EQ resources */
	hfi_cteq_cleanup(ctx);

	if (ctx->le_me_free_list) {
		vfree(ctx->le_me_free_list);
		ctx->le_me_free_list = NULL;
	}

	if (ctx->ptl_state_base) {
		vfree(ctx->ptl_state_base);
		ctx->ptl_state_base = NULL;
	}

	if (ctx->pid_count == 0) {
		dd_dev_info(dd, "release PID singleton [%u]\n", ptl_pid);
		__hfi_ctxt_unreserve(dd, ptl_pid, 1);
	}

	/* clear last */
	ctx->pid = HFI_PID_NONE;
}

/* returns number of hardware resources available to clients */
int hfi_get_hw_limits(struct hfi_ctx *ctx, struct hfi_hw_limit *hwl)
{
	struct hfi_devdata *dd = ctx->devdata;

	hwl->num_pids_avail = HFI_NUM_USABLE_PIDS - dd->pid_num_assigned;
	hwl->num_cq_pairs_avail = HFI_CQ_COUNT - dd->cq_pair_num_assigned;

	/* FXRTODO: Tune limits per system config and enforce them */
	hwl->num_pids_limit = hwl->num_pids_avail;
	hwl->num_cq_pairs_limit = hwl->num_cq_pairs_avail;
	hwl->le_me_count_limit = HFI_LE_ME_MAX_COUNT;
	hwl->unexpected_count_limit = HFI_UNEXP_MAX_COUNT;
	hwl->trig_op_count_limit = HFI_TRIG_OP_MAX_COUNT;
	return 0;
}
