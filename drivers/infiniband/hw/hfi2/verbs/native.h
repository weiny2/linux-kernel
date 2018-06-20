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
 * Intel(R) OPA Gen2 IB Driver
 */

#ifndef NATIVE_VERBS_H
#define NATIVE_VERBS_H

#include <rdma/ib_verbs.h>
#include <rdma/rdma_vt.h>
#include "../core/hfi_core.h"
#include "verbs.h"

#define RESET_LKEY(key)	((key) & 0xFFFFFF00)
#define LKEY_INDEX(key)	(((key) >> 8) & 0xFFFFFF)
#define INVALID_KEY	0xffffffff
#define INVALID_KEY_IDX	0xffffff00
#define IS_INVALID_KEY(key) (RESET_LKEY(key) == INVALID_KEY_IDX)

#define RECV_CQ_FREE	(0x1<<30)
#define SEND_CQ_FREE	(0x1<<29)
#define IOVEC_VALID	0x80
#define NATIVE_NI	PTL_MATCHING_PHYSICAL
#define NATIVE_AUTH_IDX	0
#define NATIVE_RC	0
#define HFI2_MAX_QPS_PER_PID	64

/* These marcos are used for the TX WC reordering buffer. The valid bit
 * indicates the WC has been received for this command index. The solicit bit
 * indicaets that this WC needs to be delivered to the user. The fc (flow
 * control) bit indicates that command slot has seen a flow control event.
 */
#define INIT_QP_EQ_VALID_SIGNAL(qp_priv, cidx, signal) \
	(qp_priv->wc[cidx % qp->s_size].flags = (signal << 0x1))
#define SET_QP_EQ_VALID(qp_priv, cidx, qp) \
	(qp_priv->wc[cidx % qp->s_size].flags |= 0x1)
#define SET_QP_EQ_SIGNAL(qp_priv, cidx, qp) \
	(qp_priv->wc[cidx % qp->s_size].flags |= 0x2)
#define SET_QP_FC_SEEN(qp_priv, cidx, qp) \
	(qp_priv->wc[cidx % qp->s_size].flags |= 0x4)
#define CLR_QP_EQ_VALID(qp_priv, cidx, qp) \
	(qp_priv->wc[cidx % qp->s_size].flags &= ~0x1)
#define CLR_QP_EQ_SIGNAL(qp_priv, cidx, qp) \
	(qp_priv->wc[cidx % qp->s_size].flags &= ~0x2)
#define CLR_QP_FC_SEEN(qp_priv, cidx, qp) \
	(qp_priv->wc[cidx % qp->s_size].flags &= ~0x4)
#define QP_EQ_VALID(qp_priv, eidx)		(qp_priv->wc[eidx].flags & 0x1)
#define QP_EQ_SIGNAL(qp_priv, eidx)		(qp_priv->wc[eidx].flags & 0x2)
#define QP_EQ_FC_SEEN(qp_priv, eidx)		(qp_priv->wc[eidx].flags & 0x4)
#define IS_NON_IOVEC_SWQE(v) !(__virt_addr_valid((unsigned long)v))
#define NON_IOVEC_SWQE_CIDX(v, qp)      (((u64)v) % (qp->s_size * 2))
#define IS_FLOW_CTL(f, n)        (f[n / 64] & (0x1ull << (n % 64)))
#define CLEAR_FLOW_CTL(f, n)     (f[n / 64] &= ~(0x1ull << (n % 64)))
#define SET_FLOW_CTL(f, n)       (f[n / 64] |= (0x1ull << (n % 64)))

#define to_pd_handle(pd) \
	((pd)->uobject ? (pd)->uobject->user_handle : (u64)(pd))

#define obj_to_ibctx(obj) \
	((struct hfi_ibcontext *)			\
	  ((obj)->uobject ? (obj)->uobject->context :	\
			    to_hfi_ibd((obj)->device)->ibkc))

#define IS_CIDX_LESS_THAN(c0, c1, qp)\
	(((c0 < c1) && ((c1-c0) <= qp->s_size)) \
	|| ((c0 > c1) && ((c0-c1) > qp->s_size)))

#define QP_STATE_INIT           0
#define QP_STATE_PID_RECV       1
#define QP_STATE_PID_SENT       2
#define QP_STATE_RTR            3
#define QP_STATE_RTS            4

#define INITIATOR_ID_LID(i)     ((i) & 0xffffff)
#define INITIATOR_ID_PID(i)     ((i) >> 24)

union hfi_tx_cq_command;

struct hfi_iovec {
	u64	addr;
	u32	length;
	u8	padding[3];
	u8	flags;
};

struct hfi_rq {
	struct hfi_ctx *hw_ctx;
	u32	recvq_root;
	struct hfi_ks ded_me_ks;
};

struct hfi_rq_wc {
	u64	done;
	u64	wr_id;
	union {
		struct rvt_qp *qp;
		struct hfi_rq *rq;
	};
	u16	list_handle;
	u8	is_qp;
	u8	padding[5];
	struct hfi_iovec iov[0];
} __aligned(16);

struct hfi_swqe {
	struct rvt_qp *qp;
	u64	wr_id;
	u32	length;
	u32	cidx;
	u8	num_iov;
	u8	signal;
	u8	padding[6];
	struct hfi_iovec iov[0];
} __aligned(16);

static inline
bool hfi2_ks_full(struct hfi_ks *ks)
{
	return (ks->key_head == 0);
}

static inline
bool hfi2_ks_empty(struct hfi_ks *ks)
{
	return (ks->key_head >= ks->num_keys);
}

static inline
u32 hfi2_pop_key(struct hfi_ks *ks)
{
	u32 key = INVALID_KEY;

	mutex_lock(&ks->lock);
	if (hfi2_ks_empty(ks))
		goto pop_exit;
	key = ks->free_keys[ks->key_head++];
pop_exit:
	mutex_unlock(&ks->lock);
	return key;
}

static inline
int hfi2_push_key(struct hfi_ks *ks, u32 key)
{
	int ret = 0;

	mutex_lock(&ks->lock);
	if (hfi2_ks_full(ks)) {
		ret = -EFAULT;
		goto push_exit;
	}
	ks->free_keys[--ks->key_head] = key;
push_exit:
	mutex_unlock(&ks->lock);
	return ret;
}

static inline
struct rvt_mregion *_hfi2_find_mr_from_lkey(struct hfi_ibcontext *ctx, u32 lkey,
					    bool inc)
{
	struct rvt_mregion *mr = NULL;
	u32 key_idx;

	if (lkey == 0) {
		/* return MR for the reserved LKEY */
		rcu_read_lock();
		mr = rcu_dereference(ib_to_rvt(ctx->ibuc.device)->dma_mr);
		if (mr && inc)
			rvt_get_mr(mr);
		rcu_read_unlock();
	} else {
		key_idx = LKEY_INDEX(lkey);
		if (key_idx >= ctx->lkey_ks.num_keys)
			return NULL;
		/* TODO - review locking of lkey_mr[] */
		mr = ctx->lkey_mr[key_idx];
		if (mr && inc)
			rvt_get_mr(mr);
	}

	return mr;
}

static inline
struct rvt_mregion *hfi2_chk_mr_sge(struct hfi_ibcontext *ctx,
				    struct ib_sge *sge, int acc)
{
	struct rvt_mregion *mr;

	/* TODO - pass false for last argument.
	 * Legacy QPs will reference count the MR, but this doesn't
	 * seem necessary for native transport.... revisit.
	 */
	mr = _hfi2_find_mr_from_lkey(ctx, sge->lkey, false);

	if (!mr || atomic_read(&mr->lkey_invalid) || (mr->lkey != sge->lkey) ||
	    (sge->lkey && ((sge->addr < (u64)mr->user_base) ||
	    (sge->addr + sge->length > ((u64)mr->user_base) + mr->length))) ||
	    ((mr->access_flags & acc) != acc))
		return NULL;
	return mr;
}

#ifndef CONFIG_HFI2_STLNP
#define hfi2_native_alloc_ucontext(ctx, udata, is_enabled)
#define hfi2_native_dealloc_ucontext(ctx)
#define hfi2_native_reg_mr(mr)	0
#else
void hfi2_native_alloc_ucontext(struct hfi_ibcontext *ctx, void *udata,
				bool is_enabled);
void hfi2_native_dealloc_ucontext(struct hfi_ibcontext *ctx);
void hfi_preformat_rc(struct hfi_ctx *ctx, u32 dlid, u8 rc,
		      u8 sl, u16 pkey, u8 slid_low, u8 auth_idx,
		      struct hfi_eq *eq_handle, u32 qpn,
		      union hfi_tx_cq_command *command);
int hfi2_alloc_lkey(struct rvt_mregion *mr, int acc_flags, bool dma_region);
int hfi2_free_lkey(struct rvt_mregion *mr);
struct rvt_mregion *hfi2_find_mr_from_lkey(struct rvt_pd *pd, u32 lkey);
struct rvt_mregion *hfi2_find_mr_from_rkey(struct rvt_pd *pd, u32 rkey);
struct rvt_mregion *_hfi2_find_mr_from_rkey(struct hfi_ibcontext *ctx,
					    u32 rkey);
struct hfi_ctx *hfi2_rkey_to_hw_ctx(struct hfi_ibcontext *ctx, uint32_t rkey);
int hfi2_native_reg_mr(struct rvt_mregion *mr);
int hfi2_native_modify_qp(struct rvt_qp *rvtqp,
			  struct ib_qp_attr *attr, int attr_mask,
			  struct ib_udata *udata);
void hfi2_native_reset_qp(struct rvt_qp *qp);
int hfi2_native_send(struct rvt_qp *qp, struct ib_send_wr *wr,
		     struct ib_send_wr **bad_wr);
int hfi2_native_recv(struct rvt_qp *qp, struct ib_recv_wr *wr,
		     struct ib_recv_wr **bad_wr);
int hfi2_native_srq_recv(struct rvt_srq *srq, struct ib_recv_wr *wr,
			 struct ib_recv_wr **bad_wr);
int hfi2_resize_cq(struct ib_cq *cq, int cqe, struct ib_udata *udata);
int hfi2_poll_cq(struct ib_cq *cq, int ne, struct ib_wc *wc);
int hfi2_req_notify_cq(struct ib_cq *cq, enum ib_cq_notify_flags flags);
int hfi2_destroy_srq(struct ib_srq *ibsrq);
void hfi2_qp_sync_comp_handler(struct ib_cq *cq, void *cq_context);
int hfi_set_qp_state(struct hfi_cmdq *rx_cmdq,
		     struct rvt_qp *qp, u32 slid, u16 ipid,
		     u8 state, bool failed);
int hfi2_fence(struct hfi_ibcontext *ctx, struct rvt_qp *qp, u32 *fence_value);
int hfi2_qp_exchange_pid(struct hfi_ibcontext *ctx, struct rvt_qp *qp);
int hfi2_destroy_cq(struct ib_cq *ibcq);
int hfi2_add_initial_hw_ctx(struct hfi_ibcontext *ctx);
#endif
#endif /* NATIVE_VERBS_H */
