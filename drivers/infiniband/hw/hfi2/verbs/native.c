/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2018 Intel Corporation.
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
 * Copyright (c) 2018 Intel Corporation.
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
 * Intel(R) OPA Gen2 Verbs over Native provider
 */

#include <linux/sched/signal.h>
#include "hfi_kclient.h"
#include "hfi_ct.h"
#include "hfi_tx_base.h"
#include "hfi_rx_vostlnp.h"
#include "native.h"

/* TODO - move this to header with HW defines */
#define	RKEY_PER_PID		4096
#define DEDICATED_RQ_POOL_SIZE  255

/* minimum size for EQs */
#define adjust_cqe(x) ((x) < HFI_MIN_EVENT_COUNT ? HFI_MIN_EVENT_COUNT : \
		       __roundup_pow_of_two(x))

/*
 * RKEY:
 * upper 24-bits are hardware index
 * lower 8-bits are salt bits or consumer key
 */
#define BUILD_RKEY(list_index, pid, salt) \
	(((u64)(pid) & 0xfff) << 20 | \
	 ((u64)(list_index) & 0xfff) << 8 | \
	 ((u64)(salt) & 0xff) << 0)

/*
 * LKEY:
 * upper 24-bits are private index
 * lower 8-bits are reserved for consumer key
 */
#define BUILD_LKEY(index) \
	(((u64)(index) & 0xffffff) << 8)

/*
 * Map HFI opcode for UD and RC to IB WC;
 * Only works for UD because UD_SEND overlaps with RC_SEND_FIRST/MIDDLE
 */
const u8 hfi_rc_opcode_wc[14] = {
	[PTL_UD_SEND] = IB_WC_SEND,
	[PTL_UD_SEND_IMM] = IB_WC_SEND,
	[PTL_RC_SEND_LAST] = IB_WC_SEND,
	[PTL_RC_SEND_LAST_IMM] = IB_WC_SEND,
	[PTL_RC_SEND_LAST_INV] = IB_WC_SEND,
	[PTL_RC_RDMA_WR] = IB_WC_RDMA_WRITE,
	[PTL_RC_RDMA_WR_LAST_IMM] = IB_WC_RDMA_WRITE,
	[PTL_RC_RDMA_RD] = IB_WC_RDMA_READ,
	[PTL_RC_RDMA_CMP_SWAP] = IB_WC_COMP_SWAP,
	[PTL_RC_RDMA_FETCH_ADD] = IB_WC_FETCH_ADD,
};

/* Map RX atomic_op to wc opcode */
const u8 hfi_rx_opcode_wc[14] = {
	[PTL_UD_SEND] = IB_WC_RECV,
	[PTL_UD_SEND_IMM] = IB_WC_RECV,
	[PTL_RC_SEND_LAST] = IB_WC_RECV,
	[PTL_RC_SEND_LAST_IMM] = IB_WC_RECV,
	[PTL_RC_SEND_LAST_INV] = IB_WC_RECV,
	[PTL_RC_RDMA_WR_LAST_IMM] = IB_WC_RECV_RDMA_WITH_IMM,
};

/* Map RX atomic_op to wc_flags */
const u8 hfi_rx_wc_flags[14] = {
	[PTL_UD_SEND] = 0,
	[PTL_UD_SEND_IMM] = IB_WC_WITH_IMM,
	[PTL_RC_SEND_LAST] = 0,
	[PTL_RC_SEND_LAST_IMM] = IB_WC_WITH_IMM,
	[PTL_RC_SEND_LAST_INV] = IB_WC_WITH_INVALIDATE,
	[PTL_RC_RDMA_WR_LAST_IMM] = IB_WC_WITH_IMM,
};

/* Map fail_type to IB status - TODO - scrub table, need one for RX, TX? */
const u8 hfi_ft_ib_status[16] = {
	[PTL_NI_OK] = IB_WC_SUCCESS,
	[PTL_NI_UNDELIVERABLE] = IB_WC_FATAL_ERR,
	[PTL_NI_DROPPED] = IB_WC_FATAL_ERR,
	[PTL_NI_INVALID_TARGET] = IB_WC_REM_INV_REQ_ERR,
	[PTL_NI_PT_DISABLED] = IB_WC_RNR_RETRY_EXC_ERR,
	[PTL_NI_PERM_VIOLATION] = IB_WC_REM_ACCESS_ERR,
	[PTL_NI_OP_VIOLATION] = IB_WC_REM_OP_ERR,
	[PTL_NI_SEGV] = IB_WC_REM_ABORT_ERR,
	[PTL_NI_UNSUPPORTED] = IB_WC_REM_OP_ERR,
};

/* TODO - implement QP to PID mapping algorithm for the two functions below.
 *        One idea is to just dynamically assign a new PID on-demand as
 *        we exhaust MEs that will be needed for the RQ/SRQ.
 */
static struct hfi_ctx *
hfi2_assign_hw_ctx_to_rq(struct hfi_ibcontext *ctx, struct rvt_qp *qp)
{
	if (hfi2_add_initial_hw_ctx(ctx))
		return NULL;
	return ctx->hw_ctx[0];
}

static struct hfi_ctx *
hfi2_assign_hw_ctx_to_srq(struct hfi_ibcontext *ctx, struct rvt_srq *srq)
{
	if (hfi2_add_initial_hw_ctx(ctx))
		return NULL;
	return ctx->hw_ctx[0];
}

static int hfi_ib_eq_setup(struct hfi_ctx *hw_ctx, struct rvt_cq *cq,
			   struct hfi_ibeq **hw_eq)
{
	struct opa_ev_assign eq_alloc = {0};
	struct hfi_ibeq *ibeq;
	int ret;
	unsigned long flags;

	eq_alloc.ni = NATIVE_NI;
	eq_alloc.count = adjust_cqe(cq->ibcq.cqe);
	ibeq = hfi_ibeq_alloc(hw_ctx, &eq_alloc);
	if (IS_ERR(ibeq)) {
		ret = PTR_ERR(ibeq);
		return ret;
	}

	spin_lock_irqsave(&cq->lock, flags);
	list_add_tail(&ibeq->hw_cq, &cq->hw_cq);

	if (hfi_ib_cq_armed(&cq->ibcq)) {
		/* arm the HW event queue for interrupts - CQ already armed */
		ibeq->hw_armed = 0;
		ibeq->hw_disarmed = 0;
		ret = hfi_ib_eq_arm(hw_ctx, &cq->ibcq, ibeq,
				    (cq->notify & IB_CQ_SOLICITED_MASK)
				    == IB_CQ_SOLICITED);
		spin_unlock_irqrestore(&cq->lock, flags);
		if (ret)
			goto arm_err;

		/* wait completion to turn on interrupt */
		ret = hfi_eq_poll_cmd_complete(hw_ctx, &ibeq->hw_armed);
		if (ret)
			goto arm_err;
	} else if (cq->notify & IB_CQ_SOLICITED_MASK) {
		/* arm the HW event queue for interrupts - first time arming */
		ibeq->hw_armed = 0;
		ibeq->hw_disarmed = 0;
		spin_unlock_irqrestore(&cq->lock, flags);
		ret = hfi_ib_cq_arm(hw_ctx, &cq->ibcq,
				    (cq->notify & IB_CQ_SOLICITED_MASK)
				    == IB_CQ_SOLICITED);
		if (ret)
			goto arm_err;

		/* wait completion to turn on interrupt */
		ret = hfi_eq_poll_cmd_complete(hw_ctx, &ibeq->hw_armed);
		if (ret)
			goto arm_err;
	} else {
		spin_unlock_irqrestore(&cq->lock, flags);
	}

	if (hw_eq)
		*hw_eq = ibeq;

	return 0;

arm_err:
	spin_lock_irqsave(&cq->lock, flags);
	list_del(&ibeq->hw_cq);
	spin_unlock_irqrestore(&cq->lock, flags);
	hfi_eq_free((struct hfi_eq *)ibeq);
	return ret;
}

static
int hfi2_create_qp_sync(struct hfi_ibcontext *ctx, struct hfi_ctx *hw_ctx)
{
	struct ib_cq *ibcq;
	int ret;
	struct ib_cq_init_attr attr;

	ctx->tx_qp_flow_ctl = kcalloc(HFI2_MAX_QPS_PER_PID,
				      sizeof(u64), GFP_KERNEL);
	if (!ctx->tx_qp_flow_ctl)
		return -ENOMEM;

	attr.cqe = 0x10000;
	attr.comp_vector = 0;
	attr.flags = 0;
	ibcq = ib_create_cq(ctx->ibuc.device, hfi2_qp_sync_comp_handler, NULL,
			    &ctx->ibuc, &attr);
	if (IS_ERR(ibcq)) {
		ret = PTR_ERR(ibcq);
		goto err_create_cq;
	}
	ret = hfi_ib_eq_setup(hw_ctx, ibcq_to_rvtcq(ibcq), NULL);
	if (ret)
		goto err_sync_cq;
	ret = ib_req_notify_cq(ibcq, IB_CQ_NEXT_COMP);
	if (ret)
		goto err_sync_cq;
	ctx->sync_cq = ibcq_to_rvtcq(ibcq);

	return 0;

err_sync_cq:
	ib_destroy_cq(ibcq);
err_create_cq:
	kfree(ctx->tx_qp_flow_ctl);
	ctx->tx_qp_flow_ctl = NULL;

	return ret;
}

/* Append new set of keys to LKEY and RKEY stacks */
static int hfi2_add_keys(struct hfi_ibcontext *ctx, struct hfi_ctx *hw_ctx)
{
	int i;
	u32 key_idx, extra_keys, *new_keys;
	struct rvt_mregion **new_mr;

	/* TODO - driver managed RKEYs not fully implemented for legacy QPs */
	if (hw_ctx && !ctx->lkey_only) {
		/* Append new keys to RKEY Stack */
		mutex_lock(&ctx->rkey_ks.lock);
		new_keys = krealloc(ctx->rkey_ks.free_keys,
				    (ctx->rkey_ks.num_keys + RKEY_PER_PID) *
				    sizeof(u32), GFP_KERNEL);
		if (!new_keys) {
			mutex_unlock(&ctx->rkey_ks.lock);
			return -ENOMEM;
		}
		ctx->rkey_ks.free_keys = new_keys;

		for (i = 0; i < RKEY_PER_PID; ++i)
			ctx->rkey_ks.free_keys[ctx->rkey_ks.num_keys++] =
				BUILD_RKEY(i, hw_ctx->pid, 0);
		mutex_unlock(&ctx->rkey_ks.lock);
	}

	/* Append entries to LKEY Stack + MR Array */
	mutex_lock(&ctx->lkey_ks.lock);
	extra_keys = RKEY_PER_PID * 2;
	new_keys = krealloc(ctx->lkey_ks.free_keys,
			    (ctx->lkey_ks.num_keys + extra_keys) *
			    sizeof(u32), GFP_KERNEL);
	if (!new_keys) {
		mutex_unlock(&ctx->rkey_ks.lock);
		return -ENOMEM;
	}
	ctx->lkey_ks.free_keys = new_keys;

	new_mr = krealloc(ctx->lkey_mr, (ctx->lkey_ks.num_keys + extra_keys) *
			  sizeof(void *), GFP_KERNEL);
	if (!new_mr) {
		mutex_unlock(&ctx->lkey_ks.lock);
		return -ENOMEM;
	}
	ctx->lkey_mr = new_mr;

	/* hold back LKEY == 0, which is a reserved LKEY */
	if (ctx->lkey_ks.num_keys == 0) {
		ctx->lkey_ks.num_keys = 1;
		ctx->lkey_ks.key_head = 1;
		ctx->lkey_mr[0] = NULL;
		extra_keys--;
	}

	for (i = 0; i < extra_keys; ++i) {
		key_idx = ctx->lkey_ks.num_keys + i;
		ctx->lkey_ks.free_keys[key_idx] = BUILD_LKEY(key_idx);
		ctx->lkey_mr[key_idx] = NULL;
	}
	ctx->lkey_ks.num_keys += extra_keys;
	mutex_unlock(&ctx->lkey_ks.lock);

	return 0;
}

static void hfi2_free_all_keys(struct hfi_ibcontext *ctx)
{
	kfree(ctx->rkey_ks.free_keys);
	kfree(ctx->lkey_ks.free_keys);
	kfree(ctx->lkey_mr);
	ctx->rkey_ks.free_keys = NULL;
	ctx->lkey_ks.free_keys = NULL;
	ctx->lkey_mr = NULL;
}

static void hfi2_remove_hw_context(struct hfi_ctx *hw_ctx)
{
	kfree(hw_ctx->shr_me_ks);
	pr_info("native: Detach FXR PID %d\n", hw_ctx->pid);
	hfi_ctx_cleanup(hw_ctx);
	kfree(hw_ctx);
}

void hfi2_native_alloc_ucontext(struct hfi_ibcontext *ctx, void *udata,
				bool is_enabled)
{
	ctx->num_ctx = 0;
	mutex_init(&ctx->ctx_lock);
	spin_lock_init(&ctx->flow_ctl_lock);
	/* Init key stacks */
	ctx->rkey_ks.key_head = 0;
	ctx->lkey_ks.key_head = 0;
	mutex_init(&ctx->rkey_ks.lock);
	mutex_init(&ctx->lkey_ks.lock);

	ctx->is_user = !!udata;
	/* Only enable native Verbs for kernel clients */
	ctx->supports_native = is_enabled && !ctx->is_user;

	/* Use single key when not native Verbs */
	ctx->lkey_only = !ctx->supports_native;
	if (ctx->lkey_only)
		hfi2_add_keys(ctx, NULL);
}

/* Free all CMDQs, HW contexts, and KEY stacks. */
void hfi2_native_dealloc_ucontext(struct hfi_ibcontext *ctx)
{
	int i;

	if (ctx->cmdq) {
		hfi_cmdq_unmap(ctx->cmdq);
		hfi_cmdq_release(ctx->cmdq);
		kfree(ctx->cmdq);
		ctx->cmdq = NULL;
	}
	if (ctx->sync_cq) {
		ib_destroy_cq(&ctx->sync_cq->ibcq);
		ctx->sync_cq = NULL;
	}
	if (ctx->tx_qp_flow_ctl) {
		kfree(ctx->tx_qp_flow_ctl);
		ctx->tx_qp_flow_ctl = NULL;
	}
	hfi2_free_all_keys(ctx);
	for (i = 0; i < ctx->num_ctx; i++) {
		hfi2_remove_hw_context(ctx->hw_ctx[i]);
		ctx->hw_ctx[i] = NULL;
	}
	ctx->num_ctx = 0;
}

static int hfi2_create_me_pool(struct hfi_ctx *hw_ctx)
{
	struct hfi_ks *shr_me_ks;
	int i;

	/* Init ME Stack */
	shr_me_ks = kmalloc(sizeof(*shr_me_ks), GFP_KERNEL);
	if (!shr_me_ks)
		return -ENOMEM;
	shr_me_ks->num_keys = HFI_LE_ME_MAX_COUNT;
	shr_me_ks->key_head = 0;
	shr_me_ks->free_keys = kcalloc(shr_me_ks->num_keys,
				       sizeof(u32), GFP_KERNEL);
	if (!shr_me_ks->free_keys) {
		kfree(shr_me_ks);
		return -ENOMEM;
	}

	for (i = 0; i < shr_me_ks->num_keys; ++i)
		shr_me_ks->free_keys[i] = i + 1;

	mutex_init(&shr_me_ks->lock);
	hw_ctx->shr_me_ks = shr_me_ks;

	return 0;
}

static int hfi2_add_cmdq(struct hfi_ibcontext *ctx)
{
	int ret;
	struct hfi_cmdq_pair *cmdq;

	/* TODO - only support single command queue now */
	if (ctx->cmdq)
		return 0;

	cmdq = kzalloc(sizeof(*cmdq), GFP_KERNEL);
	if (!cmdq)
		return -ENOMEM;

	/* TODO - need PID_MASK integration to remove ctx->hw_ctx[0] usage */
	ret = hfi_cmdq_assign(cmdq, ctx->hw_ctx[0], NULL);
	if (ret < 0)
		goto err;
	ctx->cmdq = cmdq;

	ret = hfi_cmdq_map(ctx->cmdq);
	if (ret < 0)
		goto err;

	pr_info("native: Got CMDQ index %d\n", cmdq->idx);
	return 0;

err:
	if (ctx->cmdq) {
		hfi_cmdq_release(ctx->cmdq);
		ctx->cmdq = NULL;
	}
	kfree(cmdq);
	return ret;
}

/* Add new hardware context, CMDQ, and associated keys to the hfi_ibcontext */
static struct hfi_ctx *hfi2_add_hw_context(struct hfi_ibcontext *ctx)
{
	struct opa_ctx_assign attach = {0};
	struct hfi_ctx *hw_ctx = NULL;
	int ret;

	if (ctx->num_ctx >= HFI_MAX_IB_HW_CONTEXTS)
		return NULL;

	hw_ctx = kzalloc(sizeof(*hw_ctx), GFP_KERNEL);
	if (!hw_ctx)
		return ERR_PTR(-ENOMEM);

	ctx->ops->ctx_init(ctx->ibuc.device, hw_ctx);
	attach.pid = HFI_PID_ANY;
	attach.le_me_count = HFI_LE_ME_MAX_COUNT - 1;
	attach.unexpected_count = RKEY_PER_PID + RKEY_OFFSET;
	ret = hfi_ctx_attach(hw_ctx, &attach);
	if (ret < 0)
		goto err_attach;
	pr_info("native: Got FXR PID %d PASID %d\n", hw_ctx->pid,
		hw_ctx->pasid);

	ret = hfi2_create_me_pool(hw_ctx);
	if (ret < 0)
		goto err_post_attach;

	ret = hfi2_add_keys(ctx, hw_ctx);
	if (ret < 0)
		goto err_post_attach;

	/*
	 * Store pointer to the hardware context state,
	 * this should be last so that teardown works correctly.
	 * Callers currently hold ctx->ctx_lock.
	 */
	ctx->hw_ctx[ctx->num_ctx++] = hw_ctx;

	return hw_ctx;

err_post_attach:
	pr_info("native: Error %d after HW attach, detaching PID %d\n",
		ret, hw_ctx->pid);
	hfi_ctx_cleanup(hw_ctx);
err_attach:
	kfree(hw_ctx);
	return ERR_PTR(ret);
}

int hfi2_add_initial_hw_ctx(struct hfi_ibcontext *ctx)
{
	struct hfi_ctx *hw_ctx;
	int ret = 0;

	if (!ctx->num_ctx) {
		mutex_lock(&ctx->ctx_lock);
		if (!ctx->num_ctx) {
			hw_ctx = hfi2_add_hw_context(ctx);
			if (IS_ERR(hw_ctx)) {
				ret = PTR_ERR(hw_ctx);
			} else {
				ret = hfi2_add_cmdq(ctx);
				if (!ret)
					/* create QP Sync  */
					ret = hfi2_create_qp_sync(ctx, hw_ctx);
			}
		}
		mutex_unlock(&ctx->ctx_lock);
	}

	return ret;
}

/*
 * Release all hardware resources associated with the RQ
 * Any error during this teardown is likely a driver bug, so we handle
 * internally.  Callers expect this to return void so that the RQ is not
 * in some unknown state.
 */
static inline
void hfi_deinit_hw_rq(struct hfi_ibcontext *ctx, struct hfi_rq *rq)
{
	int ret = 0;
	u32 list_handle;
	u64 result[2] = {0};
	struct hfi_rq_wc *hfi_wc;
	unsigned long flags;

	/* Return dedicated MEs to shared pool */
	list_handle = hfi2_pop_key(&rq->ded_me_ks);
	while (list_handle != INVALID_KEY) {
		ret = hfi2_push_key(rq->hw_ctx->shr_me_ks, list_handle);
		if (ret != 0)
			break;
		list_handle = hfi2_pop_key(&rq->ded_me_ks);
	}
	WARN_ON(ret != 0);

	/* Unlink MEs in RQ, return to shared pool */
	while (true) {
		result[0] = 0;

		/* Unlink head of list */
		spin_lock_irqsave(&ctx->cmdq->rx.lock, flags);
		ret = hfi_recvq_unlink(&ctx->cmdq->rx, NATIVE_NI,
				       rq->hw_ctx, rq->recvq_root,
				       (u64)result);
		spin_unlock_irqrestore(&ctx->cmdq->rx.lock, flags);
		if (ret != 0) {
			break;
		}

		ret = hfi_eq_poll_cmd_complete(rq->hw_ctx, result);
		if (ret != 0)
			break;

		/* Check fail type */
		if (result[0] & 0x7fffffffffffffffull)
			break;

		/* Reuse List Entry */
		hfi_wc = (struct hfi_rq_wc *)result[1];
		ret = hfi2_push_key(rq->hw_ctx->shr_me_ks,
				    hfi_wc->list_handle);
		if (ret != 0)
			break;
	}
	WARN_ON(ret != 0);

	/* Return RECVQ_ROOT to shared pool */
	ret = hfi2_push_key(rq->hw_ctx->shr_me_ks, rq->recvq_root);
	WARN_ON(ret != 0);

	/* Free memory */
	kfree(rq->ded_me_ks.free_keys);
	rq->ded_me_ks.free_keys = NULL;
	kfree(rq);
}

static inline
int hfi_init_hw_rq(struct hfi_ibcontext *ctx, struct rvt_rq *rvtrq,
		   struct hfi_ctx *hw_ctx, u32 pool_size)
{
	int i, ret;
	u32 list_handle, recvq_root;
	u64 done = 0;
	struct hfi_rq *rq;
	unsigned long flags;

	rq = kmalloc(sizeof(*rq), GFP_KERNEL);
	if (!rq)
		return -ENOMEM;
	rq->hw_ctx = hw_ctx;
	rq->ded_me_ks.free_keys = NULL;
	if (pool_size) {
		rq->ded_me_ks.free_keys = kcalloc(pool_size,
						  sizeof(u32),
						  GFP_KERNEL);
		if (!rq->ded_me_ks.free_keys) {
			kfree(rq);
			return -ENOMEM;
		}
	}

	recvq_root = hfi2_pop_key(rq->hw_ctx->shr_me_ks);
	if (recvq_root == INVALID_KEY) {
		ret = -ENOSPC;
		goto err;
	}

	rq->ded_me_ks.key_head = pool_size;
	rq->ded_me_ks.num_keys = pool_size;
	mutex_init(&rq->ded_me_ks.lock);
	for (i = 0; i < pool_size; ++i) {
		list_handle = hfi2_pop_key(rq->hw_ctx->shr_me_ks);
		if (list_handle == INVALID_KEY)
			break; /* TODO */
		hfi2_push_key(&rq->ded_me_ks, list_handle);
	}

	spin_lock_irqsave(&ctx->cmdq->rx.lock, flags);
	/* receive queue init */
	ret = hfi_recvq_init(&ctx->cmdq->rx, NATIVE_NI,
			     rq->hw_ctx, recvq_root,
			     (u64)&done);
	spin_unlock_irqrestore(&ctx->cmdq->rx.lock, flags);

	if (ret != 0) {
		goto err;
	}

	ret = hfi_eq_poll_cmd_complete(rq->hw_ctx, &done);
	if (ret != 0)
		goto err;
	rq->recvq_root = recvq_root;
	rvtrq->hw_rq = rq;

	return 0;

err:
	/* TODO - return ded free_keys to shared pool */
	kfree(rq->ded_me_ks.free_keys);
	rq->ded_me_ks.free_keys = NULL;
	hfi2_push_key(rq->hw_ctx->shr_me_ks, recvq_root);
	kfree(rq);
	return ret;
}

static inline
int hfi_format_qp_update_recv_eq(struct rvt_qp *qp, u64 user_ptr,
				 union hfi_rx_cq_update_command *cmd)
{
	struct ib_qp *ibqp = &qp->ibqp;
	union ptentry_verbs qp_state, qp_mask;
	struct hfi2_qp_priv *priv = qp->priv;

	qp_state.val[1] = 0;
	qp_state.eq_handle = priv->recv_eq->eq.idx;
	qp_mask.val[0] = 0;
	qp_mask.val[1] = 0;
	qp_mask.val[2] = 0;
	qp_mask.val[3] = 0;
	qp_mask.eq_handle = ~0x0;

	cmd->state_verbs.flit0.p0        = 0;
	cmd->state_verbs.flit0.p1	 = qp_state.val[1];
	cmd->state_verbs.flit0.c.ptl_idx = 0;
	cmd->state_verbs.flit0.c.qp_num  = ibqp->qp_num;
	cmd->state_verbs.flit0.d.ni      = NATIVE_NI;
	cmd->state_verbs.flit0.d.ct_handle = PTL_CT_NONE;
	cmd->state_verbs.flit0.d.ncc     = HFI_GEN_CC;
	cmd->state_verbs.flit0.d.command = QP_UPDATE;
	cmd->state_verbs.flit0.d.cmd_len = 3;
	cmd->state_verbs.flit1.e.cmd_pid = priv->rq_ctx->pid;
	cmd->state_verbs.flit1.p2        = 0;
	cmd->state_verbs.flit1.p3        = 0;
	cmd->state_verbs.flit1.user_ptr  = user_ptr;
	cmd->command[8]	 = qp_mask.val[0];
	cmd->command[9]	 = qp_mask.val[1];
	cmd->command[10] = qp_mask.val[2];
	cmd->command[11] = qp_mask.val[3];

	return 2;
}

/*
 * Format a QP_WRITE RX command to update QP_STATE table.
 * Most inputs for the QP_STATE entry come from the passed in RQ and QP.
 * If caller passes slid = zero, this formats command to clear the entry.
 */
int hfi_format_qp_write(struct rvt_qp *qp,
			u32 slid, u16 ipid, u64 user_ptr, u8 state, bool failed,
			union hfi_rx_cq_command *cmd)
{
	struct ib_qp *ibqp = &qp->ibqp;
	struct hfi2_qp_priv *qp_priv = qp->priv;
	union ptentry_verbs qp_state;
	u32 pd_handle;
	struct hfi_ibeq *eq = qp_priv->recv_eq;

	pd_handle = to_pd_handle(ibqp->pd);
	/* setup qp state */
	qp_state.val[0] = 0;
	qp_state.val[1] = 0;
	qp_state.val[2] = 0;
	qp_state.val[3] = 0;
	if (slid) {
		qp_state.nack = state;
		qp_state.pd = pd_handle;
		qp_state.slid = slid;
		qp_state.enable = 1;
		qp_state.ni = NATIVE_NI;
		qp_state.v = 1;
		qp_state.recvq_root = qp_priv->recvq_root;
		qp_state.tpid = qp_priv->rq_ctx->pid;
		qp_state.ipid = ipid;
		qp_state.eq_handle = eq ? eq->eq.idx : 0;
		qp_state.failed = failed;
		qp_state.ud = (ibqp->qp_type == IB_QPT_UD);
		qp_state.qp = 1;
		qp_state.user_id = qp_priv->rq_ctx->ptl_uid;
		qp_state.qkey = ibqp_to_rvtqp(ibqp)->qkey;
	}

	cmd->state_verbs.flit0.p0        = qp_state.val[0];
	cmd->state_verbs.flit0.p1	 = qp_state.val[1];
	cmd->state_verbs.flit0.c.ptl_idx = 0;
	cmd->state_verbs.flit0.c.qp_num  = ibqp->qp_num;
	cmd->state_verbs.flit0.d.ni      = NATIVE_NI;
	cmd->state_verbs.flit0.d.ct_handle = PTL_CT_NONE;
	cmd->state_verbs.flit0.d.ncc     = HFI_GEN_CC;
	cmd->state_verbs.flit0.d.command = QP_WRITE;
	cmd->state_verbs.flit0.d.cmd_len = (sizeof(cmd->state_verbs) >> 5) - 1;
	cmd->state_verbs.flit1.e.cmd_pid = qp_priv->rq_ctx->pid;
	cmd->state_verbs.flit1.p2        = qp_state.val[2];
	cmd->state_verbs.flit1.p3        = qp_state.val[3];
	cmd->state_verbs.flit1.user_ptr  = user_ptr;

	return 1;
}

int hfi_set_qp_state(struct hfi_cmdq *rx_cmdq,
		     struct rvt_qp *qp, u32 slid, u16 ipid,
		     u8 state, bool failed)
{
	struct hfi2_qp_priv *qp_priv = qp->priv;
	int ret, nslots;
	u64 done = 0;
	unsigned long flags;
	union hfi_rx_cq_command rx_cmd;

	if (!qp_priv->rq_ctx)
		return 0;

	nslots = hfi_format_qp_write(qp, slid, ipid, (u64)&done,
				     state, failed, &rx_cmd);
	spin_lock_irqsave(&rx_cmdq->lock, flags);
	do {
		ret = hfi_rx_command(rx_cmdq, (u64 *)&rx_cmd, nslots);
	} while (ret == -EAGAIN);
	spin_unlock_irqrestore(&rx_cmdq->lock, flags);
	/* If no error, process completion of above RX command */
	if (!ret)
		ret = hfi_eq_poll_cmd_complete(qp_priv->rq_ctx, &done);
	return ret;
}

void hfi2_native_reset_qp(struct rvt_qp *qp)
{
	int ret;
	struct ib_qp *ibqp = &qp->ibqp;
	struct hfi_rq *rq = qp->r_rq.hw_rq;
	struct hfi_ibcontext *ctx;
	struct hfi2_qp_priv *priv = qp->priv;

	if (!rq)
		return;

	/* Clear the QPN to PID mapping */
	ctx = obj_to_ibctx(ibqp);
	ret = hfi_set_qp_state(&ctx->cmdq->rx, qp, 0, 0, VERBS_OK, false);
	/* TODO */
	WARN_ON(ret != 0);

	if (!list_empty(&priv->send_qp_ll))
		list_del(&priv->send_qp_ll);

	if (!list_empty(&priv->recv_qp_ll))
		list_del(&priv->recv_qp_ll);

	if (!ibqp->srq)
		hfi_deinit_hw_rq(ctx, rq);
	qp->r_rq.hw_rq = NULL;
}

static
int hfi2_flush_cq(struct rvt_qp *qp, struct ib_cq *ibcq)
{
	struct rvt_cq *cq = ibcq_to_rvtcq(ibcq);
	struct hfi_ibeq *ibeq;
	int kind, i, ret;
	u64 *eqe = NULL;
	union target_EQEntry *teq;
	union target_EQEntry *ieq;
	bool dropped;
	unsigned long flags;
	int qp_num;

	spin_lock_irqsave(&cq->lock, flags);
	list_for_each_entry(ibeq, &cq->hw_cq, hw_cq) {
		for (i = 0;; ++i) {
			ret = hfi_eq_peek_nth(&ibeq->eq, &eqe, i, &dropped);
			if (ret < 0) {
				spin_unlock_irqrestore(&cq->lock, flags);
				return ret;
			} else if (ret == HFI_EQ_EMPTY) {
				break;
			}
			kind = ((union target_EQEntry *)eqe)->event_kind;
			qp_num = qp->ibqp.qp_num;
			switch (kind) {
			case NON_PTL_EVENT_VERBS_RX:
				teq = (union target_EQEntry *)eqe;
				if (qp_num == EXTRACT_HD_DST_QP(teq->hdr_data))
					teq->match_bits = FMT_VOSTLNP_MB(MB_OC_QP_RESET,
									 qp_num, 0);
				break;
			case NON_PTL_EVENT_VERBS_TX:
				ieq = (union target_EQEntry *)eqe;
				if (qp_num == EXTRACT_MB_SRC_QP(ieq->match_bits))
					ieq->match_bits = FMT_VOSTLNP_MB(MB_OC_QP_RESET,
									 qp_num, 0);
				break;
			default:
				break;
			}
		}
	}
	spin_unlock_irqrestore(&cq->lock, flags);
	return 0;
}

static
int hfi2_sqd_async_event(struct hfi_ibcontext *ctx, struct rvt_qp *qp)
{
	struct hfi2_qp_priv *priv = qp->priv;

	/* TODO - Deliver Async Event */
	return hfi2_fence(ctx, qp, &priv->outstanding_cnt);
}

static inline
void *hfi2_recv_cq_find_eq(struct rvt_qp *qp, struct rvt_cq *cq)
{
	struct hfi_ibeq *ibeq;
	struct hfi_rq *rq = qp->r_rq.hw_rq;
	void *ret = NULL;
	unsigned long flags;

	spin_lock_irqsave(&cq->lock, flags);

	if (list_empty(&cq->hw_cq))
		goto exit;

	list_for_each_entry(ibeq, &cq->hw_cq, hw_cq) {
		if (ibeq != cq->hw_send && ibeq->eq.ctx == rq->hw_ctx) {
			ret = ibeq;
			goto exit;
		}
	}

exit:
	spin_unlock_irqrestore(&cq->lock, flags);

	return ret;
}

int hfi2_native_modify_kern_qp(struct rvt_qp *rvtqp, struct ib_qp_attr *attr,
			       int attr_mask)
{
	struct ib_qp *ibqp = &rvtqp->ibqp;
	struct hfi2_qp_priv *priv = rvtqp->priv;
	struct hfi_ibcontext *ctx = obj_to_ibctx(ibqp);
	struct rvt_cq *cq;
	u64 result[2] = {0};
	int i, ret;

	if (attr_mask & IB_QP_STATE &&
	    attr->qp_state == IB_QPS_INIT &&
	    !rvtqp->r_rq.hw_rq) {
		struct hfi_ctx *hw_ctx;
		struct hfi_rq *rq;

		priv->current_cidx = 0;
		priv->current_eidx = 0;
		priv->fc_cidx = 0;
		priv->fc_eidx = 0;
		priv->outstanding_cnt = 0;
		priv->outstanding_rd_cnt = 0;

		/* Malloc retransmit command / backpressure state */
		if (!priv->cmd) {
			priv->cmd = vmalloc(sizeof(union hfi_tx_cq_command) * rvtqp->s_size);
			if (!priv->cmd)
				goto qp_write_err;
		}
		if (!priv->wc) {
			priv->wc = vzalloc(rvtqp->s_size * sizeof(struct hfi_tx_wc));
			if (!priv->wc)
				goto qp_write_err;
		}

		/*
		 * Create initial HW resources if not done, and
		 * assign a PID to allocate RQ resources from.
		 */
		hw_ctx = hfi2_assign_hw_ctx_to_rq(ctx, rvtqp);
		if (!hw_ctx)
			goto qp_write_err;

		/* setup receive queue dedicated pool + recvq_root */
		if (!ibqp->srq) {
			ret = hfi_init_hw_rq(ctx, &rvtqp->r_rq, hw_ctx,
					     DEDICATED_RQ_POOL_SIZE);
			if (ret != 0)
				goto qp_write_err;
		} else {
			struct rvt_srq *srq = ibsrq_to_rvtsrq(ibqp->srq);

			/* setup srq */
			if (!srq->rq.hw_rq) {
				/* TODO - could use SRQ max_wr attribute */
				ret = hfi_init_hw_rq(ctx, &srq->rq, hw_ctx,
						     DEDICATED_RQ_POOL_SIZE);
				if (ret != 0)
					goto qp_write_err;
			}
			rvtqp->r_rq.hw_rq = srq->rq.hw_rq;
		}
		rq = rvtqp->r_rq.hw_rq;
		priv->recvq_root = rq->recvq_root;
		priv->rq_ctx = hw_ctx;
		/* Per IBTA, we can start enqueuing to RQ at INIT */

	} else if (attr_mask & IB_QP_STATE &&
		   attr->qp_state == IB_QPS_RTR &&
		   rvtqp->r_rq.hw_rq) {
		struct hfi2_ibport *ibp;
		struct hfi_e2e_conn conn;
		struct hfi_rq *rq = rvtqp->r_rq.hw_rq;

		/*
		 * Create send EQ if first use of CQ, need to create at RTR
		 * for responding to RNR with exception packet.
		 */
		cq = ibcq_to_rvtcq(ibqp->send_cq);
		if (cq && !cq->hw_send) {
			ret = hfi_ib_eq_setup(rq->hw_ctx, cq,
					      (struct hfi_ibeq **)&cq->hw_send);
			if (ret != 0)
				goto qp_write_err;
		}
		if (cq && list_empty(&priv->send_qp_ll))
			list_add_tail(&priv->send_qp_ll,
				      &((struct hfi_ibeq *)cq->hw_send)->qp_ll);

		/* create recv EQ if first use of CQ */
		cq = ibcq_to_rvtcq(ibqp->recv_cq);
		if (cq) {
			priv->recv_eq = hfi2_recv_cq_find_eq(rvtqp, cq);
			if (!priv->recv_eq) {
				ret = hfi_ib_eq_setup(rq->hw_ctx, cq,
						      &priv->recv_eq);
				if (ret != 0)
					goto qp_write_err;
			}
			if (list_empty(&priv->recv_qp_ll))
				list_add_tail(&priv->recv_qp_ll,
					      &((struct hfi_ibeq *)priv->recv_eq)->qp_ll);
		}

		/* store pkey for native_post_send */
		ibp = to_hfi_ibp(ibqp->device, rvtqp->port_num);
		priv->pkey = hfi2_get_pkey(ibp, rvtqp->s_pkey_index);

		/* Establish E2E connection for connected QPs */
		if (ibqp->qp_type == IB_QPT_UC ||
		    ibqp->qp_type == IB_QPT_RC) {
			conn.slid = ibp->ppd->lid;
			conn.dlid = rdma_ah_get_dlid(&rvtqp->remote_ah_attr);
			conn.port_num = rvtqp->port_num;
			conn.sl = rvtqp->remote_ah_attr.sl;
			conn.pkey = priv->pkey;
			ret = hfi_e2e_ctrl(ctx, &conn);
			pr_info("native: LID %d QPN %d E2E to AV:%d,%d,%d,%d ret %d\n",
				conn.slid, ibqp->qp_num,
				conn.dlid, conn.port_num,
				conn.sl, conn.pkey, ret);
			if (ret != 0)
				goto qp_write_err;
		}

		pr_info("native: QPN %d at RTR and enabled for Portals\n",
			ibqp->qp_num);
	} else if ((attr_mask & IB_QP_STATE) &&
		   (attr->qp_state == IB_QPS_RTS) &&
		   rvtqp->r_rq.hw_rq) {
		if (ibqp->qp_type != IB_QPT_UD) {
			struct rdma_ah_attr *ah_attr = &rvtqp->remote_ah_attr;
			struct hfi_eq *eq = ibcq_to_rvtcq(ibqp->send_cq)->hw_send;

			/* loop and preformat qp->cmd with connection state */
			for (i = 0; i < rvtqp->s_size; i++)
				hfi_preformat_rc(eq->ctx,
						 rdma_ah_get_dlid(ah_attr),
						 NATIVE_RC, ah_attr->sl,
						 priv->pkey,
						 rdma_ah_get_path_bits(ah_attr),
						 NATIVE_AUTH_IDX, eq,
						 ibqp->qp_num,
						 &priv->cmd[i]);
		}
	} else if ((attr_mask & IB_QP_STATE) &&
		   (attr->qp_state == IB_QPS_SQD) &&
		   rvtqp->r_rq.hw_rq) {
		/* TODO consider async worker thread */
		ret = hfi2_sqd_async_event(ctx, rvtqp);
		if (ret != 0)
			goto qp_write_err;
	} else if (attr_mask & IB_QP_STATE &&
		    (attr->qp_state == IB_QPS_ERR ||
		     attr->qp_state == IB_QPS_RESET) &&
		    rvtqp->r_rq.hw_rq) {
		struct hfi_rq_wc *hfi_wc;
		struct hfi_rq *rq = rvtqp->r_rq.hw_rq;
		unsigned long flags;

		while (!ibqp->srq) {
			result[0] = 0;
			/* Unlink head of list */
			spin_lock_irqsave(&ctx->cmdq->rx.lock, flags);
			ret = hfi_recvq_unlink(&ctx->cmdq->rx, NATIVE_NI,
					       rq->hw_ctx,
					       rq->recvq_root,
					       (u64)result);
			spin_unlock_irqrestore(&ctx->cmdq->rx.lock, flags);
			if (ret != 0) {
				goto qp_write_err;
			}

			ret = hfi_eq_poll_cmd_complete(rq->hw_ctx, result);
			if (ret != 0)
				goto qp_write_err;

			/* Check fail type */
			if (result[0] & 0x7fffffffffffffffull)
				break;

			/* Reuse List Entry */
			hfi_wc = (struct hfi_rq_wc *)result[1];

			if (!hfi_wc->is_qp) {
				struct hfi_ctx *hw_ctx;

				hw_ctx = hfi_wc->rq->hw_ctx;
				ret = hfi2_push_key(&hfi_wc->rq->ded_me_ks,
						    hfi_wc->list_handle);
				if (ret != 0)
					ret = hfi2_push_key(hw_ctx->shr_me_ks,
							    hfi_wc->list_handle);
				if (ret != 0)
					goto qp_write_err;
			}

			/* Generate Flush Event */
			if (attr->qp_state != IB_QPS_RESET) {
				struct ib_wc wc;

				memset(&wc, 0, sizeof(wc));
				wc.qp = ibqp;
				wc.opcode = IB_WC_RECV;
				wc.wr_id = hfi_wc->wr_id;
				wc.status = IB_WC_WR_FLUSH_ERR;
				rvt_cq_enter(ibcq_to_rvtcq(ibqp->recv_cq),
					     &wc, 1);
			}
			/* Free hfi_rq_wc */
			kfree(hfi_wc);
		};

		if (attr->qp_state == IB_QPS_RESET) {
			ret = hfi2_flush_cq(rvtqp, rvtqp->ibqp.send_cq);
			if (ret != 0)
				goto qp_write_err;

			if (rvtqp->ibqp.send_cq !=
			    rvtqp->ibqp.recv_cq) {
				ret = hfi2_flush_cq(rvtqp,
						    rvtqp->ibqp.recv_cq);
				if (ret != 0)
					goto qp_write_err;
			}
		}
	}

	/* TODO test if QKEY needs to update QP_STATE
	 * RTR -> RTR, RTR -> RTS and RTS -> RTS can modify QKEY
	 */

	return 0;

qp_write_err:
	/* TODO */
	ctx->supports_native = false;
	return -EIO;
}

int hfi2_native_modify_qp(struct rvt_qp *rvtqp, struct ib_qp_attr *attr,
			  int attr_mask, struct ib_udata *udata)
{
	struct ib_qp *ibqp = &rvtqp->ibqp;
	struct hfi2_qp_priv *qp_priv = rvtqp->priv;
	struct hfi_ibcontext *ctx = obj_to_ibctx(ibqp);
	struct hfi_cmdq *rx_cmdq = NULL;
	int ret = 0;

	/* kernel and user QPs use different RX CMDQ */
	if (!udata) {
		if (ibqp->qp_type == IB_QPT_SMI || ibqp->qp_type == IB_QPT_GSI)
			return 0;
		/* TODO UD needs multicast enabled, lower priority than RC */
		if (ibqp->qp_type == IB_QPT_UD)
			return 0;

		if (!ctx || !ctx->supports_native)
			return 0;
		rx_cmdq = ctx->cmdq ? &ctx->cmdq->rx : NULL;
	}
#if 0
	else
		rx_cmdq = &ibp->cmdq_rx;
#endif

	if (rx_cmdq && qp_priv->tpid &&
	    attr_mask & IB_QP_STATE &&
	    (attr->qp_state == IB_QPS_ERR ||
	     attr->qp_state == IB_QPS_RESET)) {
		ret = hfi_set_qp_state(rx_cmdq, rvtqp,
				       rdma_ah_get_dlid(&rvtqp->remote_ah_attr),
				       qp_priv->tpid, UNCORRECTABLE, true);
		if (ret)
			return ret;
	}

	/* For kernel provider, configure hardware QP and CQ resources */
	if (!udata) {
		ret = hfi2_native_modify_kern_qp(rvtqp, attr, attr_mask);
		if (ret)
			return ret;
		/* For kernel QPs entering INIT, CMDQ created above */
		rx_cmdq = ctx->cmdq ? &ctx->cmdq->rx : NULL;
	}

	if ((ibqp->qp_type == IB_QPT_RC || ibqp->qp_type == IB_QPT_UC) &&
	    rx_cmdq && attr_mask & IB_QP_STATE &&
	    attr->qp_state == IB_QPS_INIT) {
		/* Initialize QP to accept PID exchange */
		/* TODO - limit to IPID=0 */
		ret = hfi_set_qp_state(rx_cmdq, rvtqp, PTL_LID_ANY,
				       PTL_PID_ANY, VERBS_OK, false);
	} else if (rx_cmdq && attr_mask & IB_QP_STATE &&
		   attr->qp_state == IB_QPS_RTR) {
		/*
		 * Associate QP with hardware context and mark QP enabled.
		 * Per IBTA, QPs can process incoming messages at RTR.
		 */
		if (ibqp->qp_type == IB_QPT_UD)
			ret = hfi_set_qp_state(rx_cmdq, rvtqp, PTL_LID_ANY,
					       PTL_PID_ANY, VERBS_OK, false);
		else if (ibqp->qp_type == IB_QPT_RC ||
			 ibqp->qp_type == IB_QPT_UC)
			ret = hfi2_qp_exchange_pid(ctx, rvtqp);
		if (ret < 0)
			return ret;
	}
	return ret;
}

/*
 * Append an ME to receive queue (RECV_APPEND).
 * Provider limit on queue depth is supposed to prevent
 * exhausting MEs. But we may need to use software queue.
 * TODO If no MEs, schedule some async thread?
 */
static
int hfi2_do_rx_work(struct ib_pd *ibpd, struct rvt_rq *rq,
		    struct ib_recv_wr *wr, struct rvt_qp *qp,
		    bool last_wr)
{
	struct hfi_ibcontext *ctx = obj_to_ibctx(ibpd);
	struct rvt_mregion *mr;
	union hfi_process pt;
	struct hfi_rq *hw_rq = rq->hw_rq;
	struct hfi_rq_wc *rq_wc_p = NULL;
	u32 list_handle, pd_handle, me_options;
	u64 addr, length;
	int ret, i;
	unsigned long flags;
	int qp_err_flush = (qp && qp->state == IB_QPS_ERR) &&
			    !qp->ibqp.srq;

	pd_handle = to_pd_handle(ibpd);

	me_options = PTL_ME_NO_TRUNCATE | PTL_USE_ONCE | PTL_OP_PUT;
	if (wr->num_sge > rq->max_sge) {
		return -EINVAL;
	} else if (wr->num_sge == 1) {
		/* Compare MR to SGE */
		mr = hfi2_chk_mr_sge(ctx, &wr->sg_list[0],
				     IB_ACCESS_LOCAL_WRITE);
		if (!mr)
			return -EINVAL;

		/* Allocate USER_PTR */
		rq_wc_p = kmalloc(sizeof(*rq_wc_p), GFP_KERNEL);
		if (!rq_wc_p)
			return -ENOMEM;

		/* Setup Address/Length */
		addr = wr->sg_list[0].addr;
		length = wr->sg_list[0].length;
	} else if (wr->num_sge == 0) {
		/* Allocate USER_PTR */
		rq_wc_p = kmalloc(sizeof(*rq_wc_p), GFP_KERNEL);
		if (!rq_wc_p)
			return -ENOMEM;

		/* Setup Address/Length */
		addr = 0;
		length = 0;
	} else {
		/* Allocate USER_PTR + IOVEC Array */
		rq_wc_p = kmalloc(sizeof(*rq_wc_p) +
				  sizeof(struct hfi_iovec) * wr->num_sge,
				  GFP_KERNEL);
		if (!rq_wc_p)
			return -ENOMEM;

		/* Setup Address/Length */
		addr = (u64)rq_wc_p->iov;
		length = 0;
		me_options |= PTL_IOVEC;

		for (i = 0; i < wr->num_sge; ++i) {
			/* Compare MR to SGE */
			mr = hfi2_chk_mr_sge(ctx, &wr->sg_list[i],
					     IB_ACCESS_LOCAL_WRITE);
			if (!mr) {
				ret = -EINVAL;
				goto rq_wc_cleanup;
			}

			/* Update Length + Populate IOVEC Entry */
			length += wr->sg_list[i].length;
			rq_wc_p->iov[i].addr = wr->sg_list[i].addr;
			rq_wc_p->iov[i].length = wr->sg_list[i].length;
			rq_wc_p->iov[i].flags = IOVEC_VALID;
		}
	}

	if (unlikely(qp_err_flush)) {
		struct ib_wc wc;

		memset(&wc, 0, sizeof(wc));
		wc.qp = &qp->ibqp;
		wc.opcode = IB_WC_RECV;
		wc.wr_id = wr->wr_id;
		wc.status = IB_WC_WR_FLUSH_ERR;
		rvt_cq_enter(ibcq_to_rvtcq(qp->ibqp.recv_cq), &wc, 1);
		kfree(rq_wc_p);
		return 0;
	}

	/* Get Free ME */
	list_handle = hfi2_pop_key(&hw_rq->ded_me_ks);
	if (list_handle == INVALID_KEY) {
		list_handle = hfi2_pop_key(hw_rq->hw_ctx->shr_me_ks);
		if (list_handle == INVALID_KEY) {
			ret = -ENOMEM;		// TODO - async thread
			goto rq_wc_cleanup;
		}
	}

	/* Setup USER_PTR */
	rq_wc_p->done = 0;
	rq_wc_p->wr_id = wr->wr_id;
	rq_wc_p->list_handle = list_handle;
	rq_wc_p->rq = hw_rq;
	if (qp) {
		rq_wc_p->qp = qp;
		rq_wc_p->is_qp = 1;
	} else {
		rq_wc_p->rq = hw_rq;
		rq_wc_p->is_qp = 0;
	}

	/* Receive Queue Append */
	pt.phys.slid = PTL_LID_ANY;
	pt.phys.ipid = PTL_PID_ANY;

	spin_lock_irqsave(&ctx->cmdq->rx.lock, flags);
	ret = hfi_recvq_append(&ctx->cmdq->rx, NATIVE_NI,
			       (void *)addr, length,
			       hw_rq->recvq_root, pt, PTL_UID_ANY,
			       hw_rq->hw_ctx->pid, pd_handle, me_options,
			       PTL_CT_NONE, (u64)rq_wc_p, list_handle,
			       !last_wr);
	spin_unlock_irqrestore(&ctx->cmdq->rx.lock, flags);
	if (ret != 0)
		goto me_cleanup;

	/* process the CMD_COMPLETE if this was the last WR */
	if (last_wr) {
		ret = hfi_eq_poll_cmd_complete(hw_rq->hw_ctx, &rq_wc_p->done);
		if (ret != 0)
			goto me_cleanup;
	}
	if (qp)
		qp->r_flags &= ~RVT_S_WAIT_RNR;
	return 0;

me_cleanup:
	hfi2_push_key(&hw_rq->ded_me_ks, list_handle);

rq_wc_cleanup:
	kfree(rq_wc_p);
	return ret;
}

int hfi2_native_recv(struct rvt_qp *qp, struct ib_recv_wr *wr,
		     struct ib_recv_wr **bad_wr)
{
	int ret;

	/* Refill RX hardware buffers */
	do {
		ret = hfi2_do_rx_work(qp->ibqp.pd, &qp->r_rq, wr, qp,
				      !wr->next);
		if (ret) {
			*bad_wr = wr;
			break;
		}
	} while ((wr = wr->next));

	return ret;
}

int hfi2_native_srq_recv(struct rvt_srq *srq, struct ib_recv_wr *wr,
			 struct ib_recv_wr **bad_wr)
{
	int ret;

	/*
	 * SRQ allows posting to SRQ immediately before even attached
	 * to any QPs. We usually defer allocating hardware resources
	 * until modify_qp(), but for SRQ have to do so here.
	 */
	if (!srq->rq.hw_rq) {
		struct hfi_ibcontext *ctx = obj_to_ibctx(&srq->ibsrq);
		struct hfi_ctx *hw_ctx;

		/* if context doesn't support native SRQ, use legacy SRQ */
		if (!ctx || !ctx->supports_native)
			return -EACCES;

		hw_ctx = hfi2_assign_hw_ctx_to_srq(ctx, srq);
		/* TODO - on error, fallback to legacy? */
		if (!hw_ctx)
			return -EBUSY;

		/* TODO - could use SRQ max_wr attribute */
		ret = hfi_init_hw_rq(ctx, &srq->rq, hw_ctx,
				     DEDICATED_RQ_POOL_SIZE);
		if (ret != 0)
			return ret;
	}

	/* Refill RX hardware buffers */
	do {
		ret = hfi2_do_rx_work(srq->ibsrq.pd, &srq->rq,
				      wr, NULL, !wr->next);
		if (ret) {
			*bad_wr = wr;
			break;
		}
	} while ((wr = wr->next));

	return ret;
}

static
int hfi2_process_tx_eq(struct hfi_ibcontext *ctx, struct hfi_ibeq *ibeq,
		       union initiator_EQEntry *eq, struct ib_wc *wc,
		       bool *qp_error, unsigned long *flags)
{
	struct hfi_swqe *swqe = (struct hfi_swqe *)eq->user_ptr;
	struct ib_qp *ibqp;
	struct rvt_qp *rvtqp;
	struct rvt_cq *cq;
	u8 mb_opcode = EXTRACT_MB_OPCODE(eq->match_bits);
	bool signal, suppress_free = false;
	bool in_order = true;
	struct hfi2_qp_priv *qp_priv;
	int cidx;
	int src_qpn = EXTRACT_MB_SRC_QP(eq->match_bits);
	struct hfi2_ibdev *ibd = to_hfi_ibd(ctx->ibuc.device);
	struct hfi2_ibport *ibp = ibd->pport;
	*qp_error = false;

	if (IS_NON_IOVEC_SWQE(swqe)) {
		rcu_read_lock();
		rvtqp = rvt_lookup_qpn(&ibd->rdi, &ibp->rvp, src_qpn);
		rcu_read_unlock();
	} else {
		rvtqp = swqe->qp;
	}
	ibqp = &rvtqp->ibqp;
	qp_priv = rvtqp->priv;
	cq = ibcq_to_rvtcq(ibqp->send_cq);

	cidx = IS_NON_IOVEC_SWQE(swqe) ? NON_IOVEC_SWQE_CIDX(swqe, rvtqp)
		: swqe->cidx;
	signal = (IS_NON_IOVEC_SWQE(swqe) ?
		  QP_EQ_SIGNAL(qp_priv, (cidx % rvtqp->s_size))
		  : swqe->signal) || eq->fail_type;

       /* Check Ordering */
	if (eq->fail_type != PTL_NI_PT_DISABLED &&
	    mb_opcode != MB_OC_QP_RESET) {
		spin_lock(&qp_priv->s_lock);
		if (wc && (cidx % rvtqp->s_size) == qp_priv->current_eidx) {
			qp_priv->current_eidx = ((qp_priv->current_eidx + 1) %
						 rvtqp->s_size);
			if (QP_EQ_VALID(qp_priv, qp_priv->current_eidx) &&
			    list_empty(&qp_priv->poll_qp)) {
				list_add_tail(&qp_priv->poll_qp, &cq->poll_qp);
			}
		} else {
			if (!wc &&
			    (cidx % rvtqp->s_size) == qp_priv->current_eidx &&
			    list_empty(&qp_priv->poll_qp)) {
				list_add_tail(&qp_priv->poll_qp, &cq->poll_qp);
			}
			if (signal)
				SET_QP_EQ_SIGNAL(qp_priv, cidx, rvtqp);
			SET_QP_EQ_VALID(qp_priv, cidx, rvtqp);
			wc = &qp_priv->wc[cidx % rvtqp->s_size].ib_wc;
			in_order = false;
		}
		if (eq->opcode >= PTL_RC_RDMA_RD)
			--qp_priv->outstanding_rd_cnt;
		--qp_priv->outstanding_cnt;
		spin_unlock(&qp_priv->s_lock);
	}
	if (!signal)
		goto done;

	/* Setup WC */
	if (IS_NON_IOVEC_SWQE(swqe))
		wc->wr_id = qp_priv->wc[cidx % rvtqp->s_size].ib_wc.wr_id;
	else
		wc->wr_id = swqe->wr_id;

	wc->status = hfi_ft_ib_status[eq->fail_type];
	wc->opcode = hfi_rc_opcode_wc[eq->opcode];
	wc->vendor_err = eq->fail_type;
	wc->qp = ibqp;
	/* always set byte_len, but only needed for RDMA READ/ATOMIC */
	wc->byte_len = eq->rlength;
	wc->wc_flags = 0;

	/* handle errors + flow control */
	if (unlikely(mb_opcode)) {
		switch (mb_opcode) {
		case MB_OC_LOCAL_INV:
			if (rvtqp->state >= IB_QPS_SQE)
				wc->status = IB_WC_WR_FLUSH_ERR;
			else
				wc->status = IB_WC_SUCCESS;
			wc->vendor_err = VERBS_OK;
			wc->opcode = IB_WC_LOCAL_INV;
			break;
		case MB_OC_REG_MR:
			if (rvtqp->state >= IB_QPS_SQE)
				wc->status = IB_WC_WR_FLUSH_ERR;
			else
				wc->status = IB_WC_SUCCESS;
			wc->vendor_err = VERBS_OK;
			wc->opcode = IB_WC_REG_MR;
			break;
		case MB_OC_QP_RESET:
		default:
			signal = false;
			break;
		}
	} else if (unlikely(rvtqp->state >= IB_QPS_SQE)) {
		wc->status = IB_WC_WR_FLUSH_ERR;
	} else if (unlikely(wc->vendor_err == PTL_NI_PT_DISABLED)) {
		spin_lock(&qp_priv->s_lock);
		/*
		 * Update command index flow control state - indicate to RNR
		 * thread this event has been consumed
		 */
		SET_QP_FC_SEEN(qp_priv, cidx, rvtqp);
		/* Enter flow control */
		if (!IS_FLOW_CTL(ctx->tx_qp_flow_ctl, ibqp->qp_num)) {
			qp_priv->fc_cidx = cidx;
			qp_priv->fc_eidx = (cidx + 1) % (rvtqp->s_size * 2);
			spin_lock(&ctx->flow_ctl_lock);
			SET_FLOW_CTL(ctx->tx_qp_flow_ctl, ibqp->qp_num);
			spin_unlock(&ctx->flow_ctl_lock);
		} else if (IS_FLOW_CTL(ctx->tx_qp_flow_ctl, ibqp->qp_num) &&
			IS_CIDX_LESS_THAN(cidx, qp_priv->fc_cidx, rvtqp)) {
			qp_priv->fc_cidx = cidx;
		} else if (IS_FLOW_CTL(ctx->tx_qp_flow_ctl, ibqp->qp_num) &&
			   IS_CIDX_LESS_THAN(qp_priv->fc_eidx,
					     ((cidx + 1) % (rvtqp->s_size * 2)),
					    rvtqp)) {
			qp_priv->fc_eidx = (cidx + 1) % (rvtqp->s_size * 2);
		}
		spin_unlock(&qp_priv->s_lock);

		/*
		 * Update match bits - indicate to RNR thread this event has
		 * been consumed
		 */
		eq->match_bits = FMT_VOSTLNP_MB(MB_OC_QP_RESET,
						ibqp->qp_num, 0);

		signal = false;
		suppress_free = true;
	} else if (unlikely(wc->vendor_err != VERBS_OK)) {
		*qp_error = true;
	}

done:
	if (!IS_NON_IOVEC_SWQE(swqe) && !suppress_free) {
		/* else we are only freeing IOVEC array */
		kfree(swqe);
	}

	hfi_eq_advance(&ibeq->eq, (u64 *)eq);

	/* Error handling - Continued must drop cq->lock */
	if (unlikely(*qp_error)) {
		struct ib_qp_attr attr = {0};
		if (ibqp->qp_type == IB_QPT_RC)
			attr.qp_state = IB_QPS_ERR;
		else
			attr.qp_state = IB_QPS_SQE;
		spin_unlock_irqrestore(&cq->lock, *flags);
		ib_modify_qp(ibqp, &attr, IB_QP_STATE);
		spin_lock_irqsave(&cq->lock, *flags);
	}

	return signal && in_order;
}

static
int hfi2_process_rx_eq(struct hfi_ibeq *ibeq,
		       union target_EQEntry *eq, struct ib_wc *wc,
		       struct rvt_cq *cq, bool *qp_error,
		       unsigned long *flags)
{
	struct hfi_rq_wc *hfi_wc = (struct hfi_rq_wc *)eq->user_ptr;
	u8 mb_opcode = EXTRACT_MB_OPCODE(eq->match_bits);
	struct hfi_rq *rq;
	struct rvt_qp *qp;
	union hfi_process pt;
	int qp_num;
	struct hfi2_ibdev *ibd;
	struct hfi2_ibport *ibp;
	int ret, signal = 1;
	*qp_error = false;

	if (hfi_wc->is_qp) {
		qp = hfi_wc->qp;
		rq = qp->r_rq.hw_rq;
	} else {
		rq = hfi_wc->rq;
		ibd = to_hfi_ibd(cq->ibcq.device);
		ibp = ibd->pport;
		qp_num = EXTRACT_HD_DST_QP(eq->hdr_data);
		rcu_read_lock();
		qp = rvt_lookup_qpn(cq->rdi, &ibp->rvp, qp_num);
		rcu_read_unlock();
	}

	/* Setup WC */
	wc->wr_id = hfi_wc->wr_id;
	wc->status = hfi_ft_ib_status[eq->fail_type];
	wc->vendor_err = eq->fail_type;
	wc->byte_len = eq->mlength;
	wc->qp = &qp->ibqp;
	wc->src_qp = EXTRACT_UD_SRC_QP(eq->roffset);
	wc->opcode = hfi_rx_opcode_wc[eq->atomic_op];
	wc->wc_flags = hfi_rx_wc_flags[eq->atomic_op];
	wc->ex.imm_data = EXTRACT_HD_IMM_DATA(eq->hdr_data);
	wc->pkey_index = 0;  /* only QPN 0 and 1 set this */
	wc->sl = EXTRACT_UD_SL(eq->roffset);
	pt.phys.val = eq->initiator_id;
	wc->slid = pt.phys.slid;
	wc->dlid_path_bits = EXTRACT_UD_DLID(eq->roffset);
	WARN_ON(!wc->qp);

	/* Error handling */
	if (unlikely(mb_opcode == MB_OC_QP_RESET))
		signal = 0;
	else if (unlikely(qp->state == IB_QPS_ERR))
		wc->status = IB_WC_WR_FLUSH_ERR;
	else if (unlikely(wc->vendor_err && wc->vendor_err != MR_IN_USE))
		*qp_error = true;

	/* TODO - check for send /w invalidate that failed */

	/* Reuse List Entry */
	ret = hfi2_push_key(&rq->ded_me_ks, hfi_wc->list_handle);
	if (ret != 0) {
		ret = hfi2_push_key(rq->hw_ctx->shr_me_ks,
				    hfi_wc->list_handle);
		WARN_ON(ret != 0);
	}
	kfree(hfi_wc);

	hfi_eq_advance(&ibeq->eq, (u64 *)eq);

	/* Error handling - Continued must drop cq->lock */
	if (unlikely(*qp_error)) {
		struct ib_qp_attr attr = {0};
		attr.qp_state = IB_QPS_ERR;
		spin_unlock_irqrestore(&cq->lock, *flags);
		ib_modify_qp(&qp->ibqp, &attr, IB_QP_STATE);
		spin_lock_irqsave(&cq->lock, *flags);
	}

	return signal;
}

static inline
void hfi2_destroy_eq(struct rvt_cq *cq, struct hfi_ibeq *ibeq)
{
	list_del(&ibeq->hw_cq);
	hfi_eq_free((struct hfi_eq *)ibeq);
}

int hfi2_fence(struct hfi_ibcontext *ctx, struct rvt_qp *qp, u32 *fence_value)
{
	struct rvt_cq *cq = ibcq_to_rvtcq(qp->ibqp.recv_cq);
	struct hfi_ibeq *ibeq;
	u64 *eqe = NULL;
	int kind, ret;
	bool dropped;
	struct ib_wc wc;
	unsigned long flags;
	bool qp_error;

start:
	spin_lock_irqsave(&cq->lock, flags);

start_has_lock:
	if (*fence_value == 0)
		goto exit;

	list_for_each_entry(ibeq, &cq->hw_cq, hw_cq) {
		while (true) {
			ret = hfi_eq_peek_nth(&ibeq->eq, &eqe, 0, &dropped);
			if (ret < 0) {
				spin_unlock_irqrestore(&cq->lock, flags);
				return ret;
			} else if (ret == HFI_EQ_EMPTY) {
				if (unlikely(ibeq->eq.events_pending.counter >= RECV_CQ_FREE)) {
					hfi2_destroy_eq(cq, ibeq);
					goto start_has_lock;
				}
				if (!list_is_last(&ibeq->hw_cq, &cq->hw_cq))
					break;
				spin_unlock_irqrestore(&cq->lock, flags);
				cpu_relax();
				goto start;
			}
			kind = ((union target_EQEntry *)eqe)->event_kind;

			switch (kind) {
			case NON_PTL_EVENT_VERBS_RX:
				ret = hfi2_process_rx_eq(ibeq,
							 (union target_EQEntry *)eqe,
							 &wc, cq, &qp_error,
							 &flags);
				if (unlikely(ret < 0)) {
					spin_unlock_irqrestore(&cq->lock, flags);
					return ret;
				}
				spin_unlock_irqrestore(&cq->lock, flags);
				rvt_cq_enter(ibcq_to_rvtcq(qp->ibqp.send_cq), &wc, 1);
				goto start;
			case NON_PTL_EVENT_VERBS_TX:
				hfi2_process_tx_eq(ctx, ibeq,
						   (union initiator_EQEntry *)eqe,
						   NULL, &qp_error, &flags);
				if (unlikely(ibeq->eq.events_pending.counter == SEND_CQ_FREE)) {
					hfi2_destroy_eq(cq, ibeq);
					goto start_has_lock;
				}
				if (unlikely(qp_error))
					goto start_has_lock;
				break;
			default:
				pr_err("Unknown event kind %d\n", kind);
				spin_unlock_irqrestore(&cq->lock, flags);
				hfi_eq_advance(&ibeq->eq, eqe);
				return -EINVAL;
			}
			if (*fence_value == 0)
				goto exit;
		}
	}

exit:
	spin_unlock_irqrestore(&cq->lock, flags);

	return 0;
}

static
int hfi2_poll_cq_qp(struct hfi_ibcontext *ctx, struct rvt_cq *cq,
		    struct ib_wc *wc)
{
	struct rvt_qp *rvtqp;
	struct hfi2_qp_priv *qp_priv;

	qp_priv = list_first_entry(&cq->poll_qp, struct hfi2_qp_priv, poll_qp);
	rvtqp = qp_priv->owner;
	while (QP_EQ_VALID(qp_priv, qp_priv->current_eidx)) {
		if (QP_EQ_SIGNAL(qp_priv, qp_priv->current_eidx)) {
			memcpy(wc, &qp_priv->wc[qp_priv->current_eidx].ib_wc,
			       sizeof(*wc));
			qp_priv->wc[qp_priv->current_eidx].flags = 0;
			qp_priv->current_eidx = ((qp_priv->current_eidx + 1) %
						 rvtqp->s_size);
			return 1;
		}
		qp_priv->wc[qp_priv->current_eidx].flags = 0;
		qp_priv->current_eidx = ((qp_priv->current_eidx + 1) %
					 rvtqp->s_size);
	}

	list_del_init(&qp_priv->poll_qp);

	return 0;
}

int hfi2_resize_cq(struct ib_cq *ibcq, int cqe, struct ib_udata *udata)
{
	struct hfi_ibcontext *ctx = obj_to_ibctx(ibcq);
	struct rvt_cq *cq = ibcq_to_rvtcq(ibcq);
	struct hfi2_qp_priv *priv;
	struct hfi_ibeq *ibeq, *new_ibeq, *next, *first_new = NULL;
	struct opa_ev_assign eq_alloc = {0};
	int ret = 0, nslots;
	unsigned long flags;
	uint64_t done;
	union hfi_rx_cq_update_command cmd;

	ret = rvt_resize_cq(ibcq, cqe, udata);
	if (ret || !ctx || !ctx->supports_native || list_empty(&cq->hw_cq))
		return ret;

	spin_lock_irqsave(&cq->lock, flags);

	/* This loop will allocate a new EQ, with the new CQ size, for every
	 * existing EQ assoicated with the IB CQ being resized. The existing
	 * EQs can either be send or recieve EQs. Send EQs loop through every QP
	 * assiocated with the EQ updating the QPs prepopulated command buffer
	 * EQ_HANDLE + IPID field. Recieve EQs loop through every QP assiocated
	 * with the EQ updating QP state in the HFI with the new EQ_HANDLE. The
	 * new EQ is appended linked list of EQs assoicated with the IB CQ and
	 * the loop is broken when the first new EQ is encountered.
	 */
	list_for_each_entry_safe(ibeq, next, &cq->hw_cq, hw_cq) {
		if (ibeq == first_new)
			break;

		eq_alloc.ni = NATIVE_NI;
		eq_alloc.count = adjust_cqe(cq->ibcq.cqe);
		new_ibeq = hfi_ibeq_alloc(ibeq->eq.ctx, &eq_alloc);
		if (IS_ERR(ibeq)) {
			ret = PTR_ERR(ibeq);
			break;
		}
		if (!first_new)
			first_new = new_ibeq;
		new_ibeq->qp_ll = ibeq->qp_ll;

		if (ibeq == cq->hw_send) {
			list_for_each_entry(priv, &ibeq->qp_ll, send_qp_ll)
				spin_lock(&priv->s_lock);

			/* events_pending field is overloaded, when counter
			 * reaches SEND_CQ_FREE the old EQ can be destroyed
			 * either below or when events are consumed.
			 */
			atomic_add(SEND_CQ_FREE,
				   &ibeq->eq.events_pending);
			cq->hw_send = new_ibeq;

			list_for_each_entry(priv, &ibeq->qp_ll, send_qp_ll)
				spin_unlock(&priv->s_lock);

			if (ibeq->eq.events_pending.counter == SEND_CQ_FREE)
				hfi2_destroy_eq(cq, ibeq);
		} else {
			list_for_each_entry(priv, &ibeq->qp_ll, recv_qp_ll) {
				priv->recv_eq = new_ibeq;
				ctx = obj_to_ibctx(&priv->owner->ibqp);

				nslots = hfi_format_qp_update_recv_eq(priv->owner,
								     (u64)&done,
								     &cmd);
				spin_lock(&ctx->cmdq->rx.lock);
				do {
					ret = hfi_rx_command(&ctx->cmdq->rx,
							     (u64 *)&cmd,
							     nslots);
				} while (ret == -EAGAIN);
				spin_unlock(&ctx->cmdq->rx.lock);
				if (unlikely(ret))
					goto exit;

				ret = hfi_eq_poll_cmd_complete(priv->rq_ctx,
							       &done);
				if (unlikely(ret))
					goto exit;
			}
			/* event_pending field is overload, when coutner is >=
			 * RECV_CQ_FREE and EQ has been fully drained the EQ can
			 * be destroyed.
			 */
			atomic_add(RECV_CQ_FREE+cqe,
				   &ibeq->eq.events_pending);
		}

		INIT_LIST_HEAD(&ibeq->qp_ll);
		list_add_tail(&new_ibeq->hw_cq, &cq->hw_cq);
	}

exit:
	spin_unlock_irqrestore(&cq->lock, flags);

	return ret;
}

int hfi2_poll_cq(struct ib_cq *ibcq, int ne, struct ib_wc *wc)
{
	struct hfi_ibcontext *ctx = obj_to_ibctx(ibcq);
	struct rvt_cq *cq = ibcq_to_rvtcq(ibcq);
	struct hfi_ibeq *ibeq;
	u64 *eqe = NULL;
	int kind, ret, i = 0;
	struct rvt_cq_wc *rvt_wc;
	bool dropped, qp_error;
	unsigned long flags;
	u32 head, tail;

	if (!ctx || !ctx->supports_native || list_empty(&cq->hw_cq))
		return rvt_poll_cq(ibcq, ne, wc);

start:
	while (i < ne) {
		spin_lock_irqsave(&cq->lock, flags);
		if (!list_empty(&cq->poll_qp)) {
			ret = hfi2_poll_cq_qp(ctx, cq, (wc + i));
			if (ret) {
				i += ret;
				spin_unlock_irqrestore(&cq->lock, flags);
				continue;
			}
		}
		rvt_wc = cq->queue;
		tail = rvt_wc->tail;
		head = rvt_wc->head;
		spin_unlock_irqrestore(&cq->lock, flags);
		if (tail < head) {
			i += rvt_poll_cq(ibcq, ne - i, (wc + i));
			if (i == ne)
				return i;
		} else {
			break;
		}
	}

	spin_lock_irqsave(&cq->lock, flags);

start_poll:
	list_for_each_entry(ibeq, &cq->hw_cq, hw_cq) {
		while (i < ne) {
			ret = hfi_eq_peek_nth(&ibeq->eq, &eqe, 0, &dropped);
			if (ret < 0) {
				spin_unlock_irqrestore(&cq->lock, flags);
				return ret;
			} else if (ret == HFI_EQ_EMPTY) {
				if (unlikely(ibeq->eq.events_pending.counter >= RECV_CQ_FREE)) {
					hfi2_destroy_eq(cq, ibeq);
					goto start_poll;
				}
				if (!list_is_last(&ibeq->hw_cq, &cq->hw_cq))
					break;
				spin_unlock_irqrestore(&cq->lock, flags);
				return i;
			}
			kind = ((union target_EQEntry *)eqe)->event_kind;

			switch (kind) {
			case NON_PTL_EVENT_VERBS_RX:
				i += hfi2_process_rx_eq(ibeq,
							(union target_EQEntry *)eqe,
							(wc + i), cq, &qp_error,
							&flags);
				if (unlikely(qp_error)) {
					spin_unlock_irqrestore(&cq->lock, flags);
					goto start;
				}
				break;
			case NON_PTL_EVENT_VERBS_TX:
				i += hfi2_process_tx_eq(ctx, ibeq,
							(union initiator_EQEntry *)eqe,
							(wc + i), &qp_error,
							&flags);
				if (unlikely(ibeq->eq.events_pending.counter == SEND_CQ_FREE)) {
					hfi2_destroy_eq(cq, ibeq);
					goto start_poll;
				}
				if (unlikely(qp_error)) {
					spin_unlock_irqrestore(&cq->lock, flags);
					goto start;
				}
				break;
			default:
				pr_err("Unknown event kind %d\n", kind);
				hfi_eq_advance(&ibeq->eq, eqe);
				spin_unlock_irqrestore(&cq->lock, flags);
				return -EINVAL;
			}
		}
	}

	// TODO - Rotate head

	spin_unlock_irqrestore(&cq->lock, flags);

	return i;
}

int hfi2_req_notify_cq(struct ib_cq *ibcq, enum ib_cq_notify_flags flags)
{
	struct hfi_ibcontext *ctx = obj_to_ibctx(ibcq);
	struct rvt_cq *cq = ibcq_to_rvtcq(ibcq);
	struct hfi_ibeq *ibeq;
	unsigned long lock_flags;
	int ret;

	ret = rvt_req_notify_cq(ibcq, flags);
	if (ret || !ctx || !ctx->supports_native || list_empty(&cq->hw_cq))
		return ret;

	/* check if already armed */
	spin_lock_irqsave(&cq->lock, lock_flags);
	if (hfi_ib_cq_armed(ibcq)) {
		spin_unlock_irqrestore(&cq->lock, lock_flags);
		return 0;
	}

	list_for_each_entry(ibeq, &cq->hw_cq, hw_cq) {
		/* check if this the first interrupt call */
		if (cq->eqm) {
			/* wait completion to turn off interrupt */
			ret = hfi_eq_poll_cmd_complete(ibeq->eq.ctx,
						       &ibeq->hw_disarmed);
			if (unlikely(ret)) {
				spin_unlock_irqrestore(&cq->lock, lock_flags);
				return ret;
			}
		}
		ibeq->hw_armed = 0;
		ibeq->hw_disarmed = 0;
	}
	spin_unlock_irqrestore(&cq->lock, lock_flags);

	/* arm the HW event queue for interrupts */
	ibeq = list_first_entry(&cq->hw_cq, struct hfi_ibeq, hw_cq);
	ret = hfi_ib_cq_arm(ibeq->eq.ctx, ibcq,
			    (flags & IB_CQ_SOLICITED_MASK) == IB_CQ_SOLICITED);
	if (!ret) {
		spin_lock_irqsave(&cq->lock, lock_flags);
		list_for_each_entry(ibeq, &cq->hw_cq, hw_cq) {
			/* wait completion to turn on interrupt */
			ret = hfi_eq_poll_cmd_complete(ibeq->eq.ctx,
						       &ibeq->hw_armed);
			if (unlikely(ret))
				break;
		}
		spin_unlock_irqrestore(&cq->lock, lock_flags);
	}

	return ret;
}

int hfi2_destroy_srq(struct ib_srq *ibsrq)
{
	struct rvt_srq *srq = ibsrq_to_rvtsrq(ibsrq);

	if (srq->rq.hw_rq) {
		hfi_deinit_hw_rq(obj_to_ibctx(ibsrq), srq->rq.hw_rq);
		srq->rq.hw_rq = NULL;
	}
	return rvt_destroy_srq(ibsrq);
}

int hfi2_destroy_cq(struct ib_cq *ibcq)
{
	struct rvt_cq *rcq = rcq = ibcq_to_rvtcq(ibcq);
	struct hfi_ibeq *ibeq, *next;
	int ret;
	unsigned long flags;

	ret = hfi_ib_cq_disarm_irq(ibcq);
	if (ret)
		return ret;

	spin_lock_irqsave(&rcq->lock, flags);
	if (!list_empty(&rcq->hw_cq)) {
		list_for_each_entry_safe(ibeq, next, &rcq->hw_cq, hw_cq) {
			if (!list_empty(&ibeq->qp_ll)) {
				spin_unlock_irqrestore(&rcq->lock, flags);
				return -EINVAL;
			}
			list_del(&ibeq->hw_cq);
			if (ibeq->eq.ctx->type == HFI_CTX_TYPE_KERNEL)
				hfi_eq_free((struct hfi_eq *)ibeq);
			else
				kfree(ibeq);
		}
	}
	spin_unlock_irqrestore(&rcq->lock, flags);

	return rvt_destroy_cq(ibcq);
}
