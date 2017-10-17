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

#include "hfi_kclient.h"
#include "hfi_ct.h"
#include "hfi_tx_vostlnp.h"
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

static int hfi2_add_initial_hw_ctx(struct hfi_ibcontext *ctx);

/* Append new set of keys to LKEY and RKEY stacks */
static int hfi2_add_keys(struct hfi_ibcontext *ctx, struct hfi_ctx *hw_ctx)
{
	int i;
	u32 key_idx, extra_keys, *new_keys;
	struct rvt_mregion **new_mr;

	/* TODO - driver managed RKEYs not fully implemented yet */
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
	int ret;

	ctx->num_ctx = 0;
	mutex_init(&ctx->ctx_lock);
	/* Init key stacks */
	ctx->rkey_ks.key_head = 0;
	ctx->lkey_ks.key_head = 0;
	mutex_init(&ctx->rkey_ks.lock);
	mutex_init(&ctx->lkey_ks.lock);

	ctx->is_user = !!udata;
	/* Only enable native Verbs for kernel clients */
	ctx->supports_native = is_enabled && !ctx->is_user;
	if (ctx->supports_native) {
		/* Early memory registration needs HW context */
		ret = hfi2_add_initial_hw_ctx(ctx);
		/*
		 * If error, we have run out of HW contexts.
		 * So fallback to legacy transport.
		 */
		if (ret != 0)
			ctx->supports_native = false;
	}

	/* Use single key when not native Verbs */
	ctx->lkey_only = !ctx->supports_native;
	if (ctx->lkey_only)
		hfi2_add_keys(ctx, NULL);
}

/* Free all CMDQs, HW contexts, and KEY stacks. */
void hfi2_native_dealloc_ucontext(struct hfi_ibcontext *ctx)
{
	if (ctx->tx_cmdq) {
		hfi_cmdq_unmap(ctx->tx_cmdq, ctx->rx_cmdq);
		hfi_cmdq_release(ctx->hw_ctx, ctx->tx_cmdq->cmdq_idx);
		kfree(ctx->tx_cmdq);
		kfree(ctx->rx_cmdq);
		ctx->tx_cmdq = NULL;
		ctx->rx_cmdq = NULL;
	}
	hfi2_free_all_keys(ctx);
	if (ctx->hw_ctx) {
		hfi2_remove_hw_context(ctx->hw_ctx);
		ctx->hw_ctx = NULL;
	}
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
	struct hfi_cmdq *cmdq;
	u16 cmdq_idx;

	/* TODO - only support single command queue now */
	if (ctx->tx_cmdq)
		return 0;

	ret = hfi_cmdq_assign(ctx->hw_ctx, NULL, &cmdq_idx);
	if (ret < 0)
		return ret;

	cmdq = kmalloc(sizeof(*cmdq), GFP_KERNEL);
	if (!cmdq) {
		ret = -ENOMEM;
		goto err;
	}
	ctx->tx_cmdq = cmdq;

	cmdq = kmalloc(sizeof(*cmdq), GFP_KERNEL);
	if (!cmdq) {
		ret = -ENOMEM;
		goto err;
	}
	ctx->rx_cmdq = cmdq;

	ret = hfi_cmdq_map(ctx->hw_ctx, cmdq_idx, ctx->tx_cmdq, ctx->rx_cmdq);
	if (ret < 0)
		goto err;

	pr_info("native: Got CMDQ index %d\n", cmdq_idx);
	return 0;

err:
	kfree(ctx->tx_cmdq);
	ctx->tx_cmdq = NULL;
	kfree(ctx->rx_cmdq);
	ctx->rx_cmdq = NULL;
	hfi_cmdq_release(ctx->hw_ctx, cmdq_idx);
	return ret;
}

/* Add a new hardware context and associated keys to the hfi_ibcontext */
static struct hfi_ctx *hfi2_add_hw_context(struct hfi_ibcontext *ctx)
{
	struct opa_ctx_assign attach = {0};
	struct hfi_ctx *hw_ctx = NULL;
	int ret;

	/* TODO - only support single context now */
	if (ctx->num_ctx != 0)
		return NULL;

	/*
	 * TODO - will change for Verbs 2.0
	 * Assign PID and command queue using the 'old' interface.
	 * Try converting this to use the new Verbs interface where
	 * the hfi2_user.ko cdev is not used.
	 */
	hw_ctx = kzalloc(sizeof(*hw_ctx), GFP_KERNEL);
	if (!hw_ctx)
		return ERR_PTR(-ENOMEM);
	ctx->ops->ctx_init(ctx->ibuc.device, hw_ctx);
	attach.pid = HFI_PID_ANY;
	attach.le_me_count = HFI_LE_ME_MAX_COUNT - 1;
	attach.unexpected_count = RKEY_PER_PID;
	ret = hfi_ctx_attach(hw_ctx, &attach);
	if (ret < 0)
		goto err_attach;
	pr_info("native: Got FXR PID %d\n", hw_ctx->pid);

	ret = hfi2_create_me_pool(hw_ctx);
	if (ret < 0)
		goto err_post_attach;

	ret = hfi2_add_keys(ctx, hw_ctx);
	if (ret < 0)
		goto err_post_attach;

	/*
	 * Store pointer to the hardware context state
	 * TODO - update here for multiple PIDs
	 */
	ctx->num_ctx = 1;
	ctx->hw_ctx = hw_ctx;

	return hw_ctx;

err_post_attach:
	pr_info("native: Detach FXR PID %d\n", hw_ctx->pid);
	hfi_ctx_cleanup(hw_ctx);
err_attach:
	kfree(hw_ctx);
	return ERR_PTR(ret);
}

static int hfi2_add_initial_hw_ctx(struct hfi_ibcontext *ctx)
{
	struct hfi_ctx *hw_ctx;
	int ret = 0;

	if (!ctx->num_ctx) {
		mutex_lock(&ctx->ctx_lock);
		if (!ctx->num_ctx) {
			hw_ctx = hfi2_add_hw_context(ctx);
			if (!hw_ctx)
				ret = -EBUSY; /* TODO */
			else
				ret = hfi2_add_cmdq(ctx);
		}
		mutex_unlock(&ctx->ctx_lock);
	}

	return ret;
}

/* TODO - this memory is not yet freed on destroy_qp / destroy_srq */
static inline
int hfi_init_hw_rq(struct hfi_ibcontext *ctx, struct rvt_rq *rvtrq,
		   struct hfi_ctx *hw_ctx, u32 pool_size)
{
	int i, ret;
	u32 list_handle, recvq_root;
	u64 done = 0;
	struct hfi_rq *rq;

	rq = kmalloc(sizeof(*rq), GFP_KERNEL);
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

	/* receive queue init */
	ret = hfi_recvq_init(ctx->rx_cmdq, NATIVE_NI,
			     rq->hw_ctx, recvq_root,
			     (u64)&done);
	if (ret != 0)
		goto err;

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

int hfi2_native_modify_qp(struct rvt_qp *rvtqp, struct ib_qp_attr *attr,
			  int attr_mask, struct ib_udata *udata)
{
	struct ib_qp *ibqp = &rvtqp->ibqp;
	struct hfi2_qp_priv *priv = rvtqp->priv;
	struct hfi_ibcontext *ctx = obj_to_ibctx(ibqp);
	struct rvt_cq *cq;
	int ret;
	union ptentry_verbs qp_state;
	u64 done = 0;

	if (!ctx || !ctx->supports_native)
		return 0;

	if (attr_mask & IB_QP_STATE &&
	    attr->qp_state == IB_QPS_INIT &&
	    !rvtqp->r_rq.hw_rq) {
		struct hfi_ctx *hw_ctx;

		/* Create initial resources when first QP enters RTR */
		ret = hfi2_add_initial_hw_ctx(ctx);
		if (ret != 0)
			goto qp_write_err;

		/* TODO add multiple PID support here */
		hw_ctx = ctx->hw_ctx;

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
		/* Per IBTA, we can start enqueuing to RQ at INIT */

	} else if (attr_mask & IB_QP_STATE &&
		   attr->qp_state == IB_QPS_RTR) {
		struct hfi2_ibport *ibp;
		struct hfi_e2e_conn conn;
		struct opa_ev_assign eq_alloc = {0};
		struct hfi_rq *rq = rvtqp->r_rq.hw_rq;
		struct hfi_eq *eq;
		u32 pd_handle;

		/* TODO - how to set this? */
		pd_handle = ibqp->pd->uobject ?
			    ibqp->pd->uobject->user_handle : 0;

		/* create recv EQ if first use of CQ */
		cq = ibcq_to_rvtcq(ibqp->recv_cq);
		if (cq && !cq->hw_cq) {
			eq_alloc.ni = NATIVE_NI;
			eq_alloc.count = adjust_cqe(cq->ibcq.cqe);
			cq->hw_cq = hfi_eq_alloc(rq->hw_ctx, &eq_alloc);
			if (IS_ERR(cq->hw_cq)) {
				ret = PTR_ERR(cq->hw_cq);
				goto qp_write_err;
			}
		}
		eq = cq->hw_cq;

		/* alloc fence CT */
		ret = hfi_ct_alloc(rq->hw_ctx, NATIVE_NI, &priv->fence_ct);
		if (ret != 0)
			goto qp_write_err;
		priv->fence_cnt = 0;

		/* alloc non-fence CT */
		ret = hfi_ct_alloc(rq->hw_ctx, NATIVE_NI, &priv->nfence_ct);
		if (ret != 0)
			goto qp_write_err;
		priv->nfence_cnt = 0;

		/*
		 * setup qp state - TODO - move to driver
		 * Per IBTA, QPs cannot process incoming messages until RTR.
		 */
		qp_state.nack = 0;
		qp_state.pd = pd_handle;
		qp_state.slid = PTL_LID_ANY;
		qp_state.enable = 1;
		qp_state.ni = NATIVE_NI;
		qp_state.v = 1;
		qp_state.recvq_root = rq->recvq_root;
		qp_state.tpid = rq->hw_ctx->pid;
		qp_state.ipid = PTL_PID_ANY;
		qp_state.eq_handle = eq->idx;
		qp_state.failed = 0;
		qp_state.ud = (ibqp->qp_type == IB_QPT_UD);
		qp_state.qp = 1;
		qp_state.user_id = rq->hw_ctx->ptl_uid;
		qp_state.qkey = rvtqp->qkey;
		ret = hfi_qp_write(ctx->rx_cmdq, NATIVE_NI, rq->hw_ctx,
				   ibqp->qp_num, (u64 *)&qp_state,
				   (u64)&done);
		if (ret != 0)
			goto qp_write_err;

		ret = hfi_eq_poll_cmd_complete(rq->hw_ctx, &done);
		if (ret != 0)
			goto qp_write_err;

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

	} else if (attr_mask & IB_QP_STATE &&
		   attr->qp_state == IB_QPS_RTS) {
		/* create send EQ if first use of CQ */
		cq = ibcq_to_rvtcq(ibqp->send_cq);
		if (cq && !cq->hw_cq) {
			struct hfi_rq *rq = rvtqp->r_rq.hw_rq;
			struct opa_ev_assign eq_alloc = {0};

			eq_alloc.ni = NATIVE_NI;
			eq_alloc.count = adjust_cqe(cq->ibcq.cqe);
			cq->hw_cq = hfi_eq_alloc(rq->hw_ctx, &eq_alloc);
			if (IS_ERR(cq->hw_cq)) {
				ret = PTR_ERR(cq->hw_cq);
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
	return -EIO;
}

/*
 * Issue the appropriate TX command
 * TODO If no TX slots, schedule some async thread?
 */
static
int hfi2_do_tx_work(struct rvt_qp *qp, struct ib_send_wr *wr)
{
	bool signal;

	signal = !(qp->s_flags & RVT_S_SIGNAL_REQ_WR) ||
		 (wr->send_flags & IB_SEND_SIGNALED);

	/* TODO port from user provider */

	return 0;
}

/*
 * Append an ME to receive queue (RECV_APPEND).
 * Provider limit on queue depth is supposed to prevent
 * exhausting MEs. But we may need to use software queue.
 * TODO If no MEs, schedule some async thread?
 */
static
int hfi2_do_rx_work(struct ib_pd *ibpd, struct rvt_rq *rq,
		    struct ib_recv_wr *wr)
{
	/* TODO port from user provider */

	return 0;
}

int hfi2_native_send(struct rvt_qp *qp, struct ib_send_wr *wr,
		     struct ib_send_wr **bad_wr)
{
	int ret;

	/* Issue pending TX commands */
	do {
		ret = hfi2_do_tx_work(qp, wr);
		if (ret) {
			*bad_wr = wr;
			break;
		}
	} while ((wr = wr->next));

	return ret;
}

int hfi2_native_recv(struct rvt_qp *qp, struct ib_recv_wr *wr,
		     struct ib_recv_wr **bad_wr)
{
	int ret;

	/* Refill RX hardware buffers */
	do {
		ret = hfi2_do_rx_work(qp->ibqp.pd, &qp->r_rq, wr);
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

		/* if context doesn't support native SRQ, use legacy SRQ */
		if (!ctx || !ctx->supports_native)
			return -EACCES;

		ret = hfi2_add_initial_hw_ctx(ctx);
		if (ret != 0)
			return ret;

		/* TODO - could use SRQ max_wr attribute */
		ret = hfi_init_hw_rq(ctx, &srq->rq, ctx->hw_ctx,
				     DEDICATED_RQ_POOL_SIZE);
		if (ret != 0)
			return ret;
	}

	/* Refill RX hardware buffers */
	do {
		ret = hfi2_do_rx_work(srq->ibsrq.pd, &srq->rq,
				      wr);
		if (ret) {
			*bad_wr = wr;
			break;
		}
	} while ((wr = wr->next));

	return ret;
}

int hfi2_native_poll_cq(struct rvt_cq *cq, int ne, struct ib_wc *wc)
{
	/* TODO port from user provider */

	return 0;
}
