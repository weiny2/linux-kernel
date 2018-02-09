/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2017 Intel Corporation.
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
 * Copyright (c) 2017 Intel Corporation.
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

#define NATIVE_AUTH_IDX		0
#define NATIVE_RC		0

#define QKEY_USE_LOCAL		0x80000000

/* Map IB opcodes to native opcodes */
const u8 hfi_wr_ud_opcode[4] = {
	[IB_WR_SEND] = PTL_UD_SEND,
	[IB_WR_SEND_WITH_IMM] = PTL_UD_SEND_IMM,
};

const u8 hfi_wr_rc_opcode[10] = {
	[IB_WR_RDMA_WRITE] = PTL_RC_RDMA_WR,
	[IB_WR_RDMA_WRITE_WITH_IMM] = PTL_RC_RDMA_WR_LAST_IMM,
	[IB_WR_SEND] = PTL_RC_SEND_LAST,
	[IB_WR_SEND_WITH_IMM] = PTL_RC_SEND_LAST_IMM,
	[IB_WR_RDMA_READ] = PTL_RC_RDMA_RD,
	[IB_WR_ATOMIC_CMP_AND_SWP] = PTL_RC_RDMA_CMP_SWAP,
	[IB_WR_ATOMIC_FETCH_AND_ADD] = PTL_RC_RDMA_FETCH_ADD,
	[IB_WR_SEND_WITH_INV] = PTL_RC_SEND_LAST_INV
};

static inline
void hfi2_fence(struct rvt_qp *qp)
{
	struct hfi2_qp_priv *priv = qp->priv;
	struct hfi_rq *rq = qp->r_rq.hw_rq;
	u64 cnt;

	do {
		cnt = hfi_ct_get_success(rq->hw_ctx, priv->fence_ct) +
			hfi_ct_get_failure(rq->hw_ctx, priv->fence_ct);
		if (cnt == priv->fence_cnt)
			return;
		cpu_relax();
	} while (true);
}

static inline
void hfi2_nfence(struct rvt_qp *qp)
{
	struct hfi2_qp_priv *priv = qp->priv;
	struct hfi_rq *rq = qp->r_rq.hw_rq;
	u64 cnt;

	hfi2_fence(qp);
	do {
		cnt = hfi_ct_get_success(rq->hw_ctx, priv->nfence_ct) +
			hfi_ct_get_failure(rq->hw_ctx, priv->nfence_ct);
		if (cnt == priv->nfence_cnt)
			return;
		cpu_relax();
	} while (true);
}

/* Convert Send WR into state needed for send completion and hardware IOVEC */
static
struct hfi_swqe *hfi_prepare_swqe(struct rvt_qp *qp, struct ib_send_wr *wr,
				  bool signal, int *ret)
{
	int i;
	struct hfi_ibcontext *ctx = obj_to_ibctx(&qp->ibqp);
	struct hfi_swqe *swqe;
	struct hfi_iovec *iov;
	struct rvt_mregion *mr;

	swqe = kmalloc(sizeof(*swqe) + wr->num_sge * sizeof(*iov), GFP_KERNEL);
	if (!swqe) {
		*ret = -ENOMEM;
		return NULL;
	}
	iov = swqe->iov;

	for (i = 0; i < wr->num_sge; i++) {
		mr = hfi2_chk_mr_sge(ctx, &wr->sg_list[i]);
		if (!mr) {
			*ret = -EINVAL;
			goto err;
		}

		iov[i].addr = wr->sg_list[i].addr;
		iov[i].length = wr->sg_list[i].length;
		iov[i].flags = IOVEC_VALID;
		swqe->length += iov[i].length;
	}
	swqe->qp = qp;
	swqe->wr_id = wr->wr_id;
	swqe->num_iov = wr->num_sge;
	swqe->signal = signal;

	return swqe;

err:
	kfree(swqe);
	return NULL;
}

/*
 * UD SEND:
 * Fill in TX command from the user's WR and AH attributes.
 * Returns number of slots to be used or -errno on error.
 */
static
int hfi_format_ud_send(struct rvt_qp *qp, struct ib_ud_wr *wr,
		       bool signal, bool in_line, bool solicit,
		       union hfi_tx_cq_command *cmd)
{
	int ret, nslots;
	struct rvt_ah *ah = ibah_to_rvtah(wr->ah);
	struct rdma_ah_attr *ah_attr = &ah->attr;
	struct ib_sge *sge = wr->wr.sg_list;
	struct hfi2_qp_priv *priv = qp->priv;
	struct hfi_rq *rq = qp->r_rq.hw_rq;
	struct rvt_cq *send_cq = ibcq_to_rvtcq(qp->ibqp.send_cq);
	struct hfi_eq *send_eq = send_cq->hw_cq;
	u8 op_req = hfi_wr_ud_opcode[wr->wr.opcode];
	u8 becn = 0;
	u32 qkey, md_opts = PTL_MD_EVENT_SUCCESS_DISABLE |
		PTL_MD_EVENT_CT_SEND;
	u16 tx_id = 0;
	union hfi_process tpid = {.phys.slid = rdma_ah_get_dlid(ah_attr)};
	struct hfi_swqe *swqe = NULL;

	if (signal || wr->wr.num_sge > 1) {
		/* validate SG list and build IOVEC array if multiple SGEs */
		md_opts = PTL_MD_EVENT_CT_SEND;
		swqe = hfi_prepare_swqe(qp, &wr->wr, signal, &ret);
		if (!swqe)
			return ret;
	} else if (wr->wr.num_sge) {
		/* validate single SGE */
		if (!in_line && !hfi2_chk_mr_sge(obj_to_ibctx(&qp->ibqp), sge))
			return -EINVAL;
	}

	qkey = !(wr->remote_qkey & QKEY_USE_LOCAL) ?
	       wr->remote_qkey : qp->qkey;

	if (wr->wr.num_sge > 1) {
		md_opts = PTL_IOVEC | PTL_MD_EVENT_CT_SEND;
		nslots = hfi_format_dma_iovec_ud(
					rq->hw_ctx, NATIVE_NI,
					(void *)swqe->iov, swqe->length,
					tpid, 1,
					NATIVE_RC, ah_attr->sl,
					becn, priv->pkey,
					rdma_ah_get_path_bits(ah_attr),
					NATIVE_AUTH_IDX,
					(u64)swqe,
					qp->ibqp.qp_num,
					wr->remote_qpn,
					wr->wr.ex.imm_data, qkey,
					md_opts, send_eq, priv->nfence_ct,
					tx_id, op_req, solicit,
					swqe->num_iov, 0,
					&cmd->dma_iovec);
	} else if (sge->length <= HFI_TX_MAX_BUFFERED) {
		nslots = hfi_format_buff_ud(
					rq->hw_ctx, NATIVE_NI,
					(void *)sge->addr, sge->length,
					tpid, 0,
					NATIVE_RC, ah_attr->sl,
					becn, priv->pkey,
					rdma_ah_get_path_bits(ah_attr),
					NATIVE_AUTH_IDX,
					(u64)swqe,
					qp->ibqp.qp_num,
					wr->remote_qpn,
					wr->wr.ex.imm_data, qkey,
					md_opts, send_eq, priv->nfence_ct,
					tx_id, op_req, solicit, MB_OC_OK,
					&cmd->buff_put_match);
	} else {
		nslots = hfi_format_dma_ud(
					rq->hw_ctx, NATIVE_NI,
					(void *)sge->addr, sge->length,
					tpid, 0,
					NATIVE_RC, ah_attr->sl,
					becn, priv->pkey,
					rdma_ah_get_path_bits(ah_attr),
					NATIVE_AUTH_IDX,
					(u64)swqe,
					qp->ibqp.qp_num,
					wr->remote_qpn,
					wr->wr.ex.imm_data, qkey,
					md_opts, send_eq, priv->nfence_ct,
					tx_id, op_req, solicit,
					&cmd->dma);
	}

	if (swqe && nslots <= 0)
		kfree(swqe);
	else
		++priv->nfence_cnt;

	return nslots;
}

/*
 * RC SEND:
 * Fill in TX command from the user's WR and QP/AH attributes.
 * Returns number of slots to be used or -errno on error.
 */
static
int hfi_format_rc_send(struct rvt_qp *qp, struct ib_send_wr *wr,
		       bool signal, bool in_line, bool solicit,
		       union hfi_tx_cq_command *cmd)
{
	int ret, nslots;
	struct rdma_ah_attr *ah_attr = &qp->remote_ah_attr;
	struct ib_sge *sge = wr->sg_list;
	struct hfi2_qp_priv *priv = qp->priv;
	struct hfi_rq *rq = qp->r_rq.hw_rq;
	struct rvt_cq *send_cq = ibcq_to_rvtcq(qp->ibqp.send_cq);
	struct hfi_eq *send_eq = send_cq->hw_cq;
	u8 op_req = hfi_wr_rc_opcode[wr->opcode];
	u8 becn = 0;
	u32 md_opts = PTL_MD_EVENT_SUCCESS_DISABLE | PTL_MD_EVENT_CT_SEND;
	u16 tx_id = 0;
	union hfi_process tpid = {.phys.slid = rdma_ah_get_dlid(ah_attr)};
	struct hfi_swqe *swqe = NULL;

	if (qp->ibqp.qp_type == IB_QPT_UC &&
	    wr->opcode == IB_WR_SEND_WITH_INV)
		return -EINVAL;

	if (signal || wr->num_sge > 1) {
		/* validate SG list and build IOVEC array if multiple SGEs */
		md_opts = PTL_MD_EVENT_CT_SEND;
		swqe = hfi_prepare_swqe(qp, wr, signal, &ret);
		if (!swqe)
			return ret;
	} else if (wr->num_sge) {
		/* validate single SGE */
		if (!in_line && !hfi2_chk_mr_sge(obj_to_ibctx(&qp->ibqp), sge))
			return -EINVAL;
	}

	if (wr->num_sge > 1) {
		md_opts = PTL_IOVEC | PTL_MD_EVENT_CT_SEND;
		nslots = hfi_format_dma_iovec_rc_send(
					rq->hw_ctx, NATIVE_NI,
					(void *)swqe->iov, swqe->length,
					tpid, 1,
					NATIVE_RC, ah_attr->sl,
					becn, priv->pkey,
					rdma_ah_get_path_bits(ah_attr),
					NATIVE_AUTH_IDX,
					(u64)swqe,
					qp->ibqp.qp_num,
					qp->remote_qpn,
					wr->ex.imm_data,
					md_opts, send_eq, priv->nfence_ct,
					tx_id, op_req, solicit,
					swqe->num_iov, 0,
					&cmd->dma_iovec);
	} else if (sge->length <= HFI_TX_MAX_BUFFERED) {
		nslots = hfi_format_buff_rc_send(
					rq->hw_ctx, NATIVE_NI,
					(void *)sge->addr, sge->length,
					tpid, 0,
					NATIVE_RC, ah_attr->sl,
					becn, priv->pkey,
					rdma_ah_get_path_bits(ah_attr),
					NATIVE_AUTH_IDX,
					(u64)swqe,
					qp->ibqp.qp_num,
					qp->remote_qpn,
					wr->ex.imm_data,
					md_opts, send_eq, priv->nfence_ct,
					tx_id, op_req, solicit,
					&cmd->buff_put_match);
	} else {
		nslots = hfi_format_dma_rc_send(
					rq->hw_ctx, NATIVE_NI,
					(void *)sge->addr, sge->length,
					// FIXME - user MTU
					tpid, sge->length > 4096,
					NATIVE_RC, ah_attr->sl,
					becn, priv->pkey,
					rdma_ah_get_path_bits(ah_attr),
					NATIVE_AUTH_IDX,
					(u64)swqe,
					qp->ibqp.qp_num,
					qp->remote_qpn,
					wr->ex.imm_data,
					md_opts, send_eq, priv->nfence_ct,
					tx_id, op_req, solicit,
					&cmd->dma);
	}

	if (swqe && nslots <= 0)
		kfree(swqe);
	else
		++priv->nfence_cnt;

	return nslots;
}

/* TODO - can replace this with a function table lookup using qp_type */
static inline
int hfi_format_send_wr(struct rvt_qp *qp, struct ib_send_wr *wr,
		       bool signal, bool in_line, bool solicit,
		       union hfi_tx_cq_command *cmd)
{
	int nslots;

	if (qp->ibqp.qp_type == IB_QPT_RC ||
	    qp->ibqp.qp_type == IB_QPT_UC)
		nslots = hfi_format_rc_send(qp, wr, signal, in_line, solicit,
					    cmd);
	else if (qp->ibqp.qp_type == IB_QPT_UD)
		nslots = hfi_format_ud_send(qp, ud_wr(wr), signal, in_line,
					    solicit, cmd);
	else
		nslots = -EINVAL;

	return nslots;
}

/*
 * RDMA ATOMIC:
 * Fill in TX command from the user's WR and QP/AH attributes.
 * Returns number of slots to be used or -errno on error.
 */
static inline
int hfi_format_rc_rdma_atomic(struct rvt_qp *qp, struct ib_atomic_wr *wr,
			      bool signal, bool in_line, bool solicit,
			      union hfi_tx_cq_command *cmd)
{
	int ret, nslots;
	struct rdma_ah_attr *ah_attr = &qp->remote_ah_attr;
	struct ib_sge *sge = wr->wr.sg_list;
	struct hfi2_qp_priv *priv = qp->priv;
	struct hfi_rq *rq = qp->r_rq.hw_rq;
	struct rvt_cq *send_cq = ibcq_to_rvtcq(qp->ibqp.send_cq);
	struct hfi_eq *send_eq = send_cq->hw_cq;
	u8 op_req = hfi_wr_rc_opcode[wr->wr.opcode];
	u8 becn = 0;
	u32 md_opts = PTL_MD_EVENT_SUCCESS_DISABLE | PTL_MD_EVENT_CT_SEND;
	u16 tx_id = 0;
	union hfi_process tpid = {.phys.slid = rdma_ah_get_dlid(ah_attr)};
	struct hfi_swqe *swqe = NULL;

	if (qp->ibqp.qp_type != IB_QPT_RC)
		return -EINVAL;

	if (wr->wr.num_sge == 1 && sge->length == 8) {
		if (signal) {
			md_opts = PTL_MD_EVENT_CT_SEND;
			swqe = hfi_prepare_swqe(qp, &wr->wr, signal, &ret);
			if (!swqe)
				return ret;
		} else if (!in_line &&
			   !hfi2_chk_mr_sge(obj_to_ibctx(&qp->ibqp), sge)) {
			return -EINVAL;
		}

		nslots = hfi_format_buff_rc_rdma_atomic(
					rq->hw_ctx, NATIVE_NI,
					(void *)sge->addr,
					tpid, 0,
					NATIVE_RC, ah_attr->sl,
					becn, priv->pkey,
					rdma_ah_get_path_bits(ah_attr),
					NATIVE_AUTH_IDX,
					(u64)swqe,
					qp->ibqp.qp_num,
					qp->remote_qpn,
					wr->wr.ex.imm_data, wr->rkey,
					md_opts, send_eq, priv->fence_ct,
					wr->remote_addr,
					tx_id, op_req, solicit,
					&wr->compare_add,
					op_req == PTL_RC_RDMA_FETCH_ADD
					? NULL : &wr->swap,
					&cmd->buff_toa_fetch_match);
		if (swqe && nslots <= 0)
			kfree(swqe);
		else
			++priv->fence_cnt;
	} else {
		nslots = -EINVAL;
	}

	return nslots;
}

/*
 * RDMA WRITE/READ:
 * Fill in TX command from the user's WR and QP/AH attributes.
 * Returns number of slots to be used or -errno on error.
 */
static
int hfi_format_rc_rdma(struct rvt_qp *qp, struct ib_rdma_wr *wr,
		       bool signal, bool in_line, bool solicit,
		       union hfi_tx_cq_command *cmd)
{
	int ret, nslots;
	struct rdma_ah_attr *ah_attr = &qp->remote_ah_attr;
	struct ib_sge *sge = wr->wr.sg_list;
	struct hfi2_qp_priv *priv = qp->priv;
	struct hfi_rq *rq = qp->r_rq.hw_rq;
	struct rvt_cq *send_cq = ibcq_to_rvtcq(qp->ibqp.send_cq);
	struct hfi_eq *send_eq = send_cq->hw_cq;
	u8 op_req = hfi_wr_rc_opcode[wr->wr.opcode];
	u8 becn = 0;
	void *start = NULL;
	u32 length = 0;
	u32 md_opts = wr->wr.opcode != IB_WR_RDMA_READ ?
		PTL_MD_EVENT_SUCCESS_DISABLE | PTL_MD_EVENT_CT_SEND :
		PTL_MD_EVENT_SUCCESS_DISABLE | PTL_MD_EVENT_CT_REPLY;
	u16 tx_id = 0;
	union hfi_process tpid = {.phys.slid = rdma_ah_get_dlid(ah_attr)};
	struct hfi_swqe *swqe = NULL;
	hfi_ct_handle_t ct_handle = wr->wr.opcode != IB_WR_RDMA_READ ?
		priv->nfence_ct : priv->fence_ct;

	if (qp->ibqp.qp_type == IB_QPT_UD ||
	    (qp->ibqp.qp_type == IB_QPT_UC &&
	     wr->wr.opcode == IB_WR_RDMA_READ))
		return -EINVAL;

	if (signal || wr->wr.num_sge > 1) {
		/* validate SG list and build IOVEC array if multiple SGEs */
		md_opts &= ~PTL_MD_EVENT_SUCCESS_DISABLE;
		swqe = hfi_prepare_swqe(qp, &wr->wr, signal, &ret);
		if (!swqe)
			return ret;
	} else if (wr->wr.num_sge) {
		/* validate single SGE */
		if (!in_line && !hfi2_chk_mr_sge(obj_to_ibctx(&qp->ibqp), sge))
			return -EINVAL;
	}

	if (sge) {
		start = (void *)sge->addr;
		length = sge->length;
	}

	if (wr->wr.num_sge > 1) {
		md_opts &= ~PTL_MD_EVENT_SUCCESS_DISABLE;
		md_opts |= PTL_IOVEC;
		nslots = hfi_format_dma_iovec_rc_rdma(
					rq->hw_ctx, NATIVE_NI,
					(void *)swqe->iov, swqe->length,
					tpid, 1,
					NATIVE_RC, ah_attr->sl,
					becn, priv->pkey,
					rdma_ah_get_path_bits(ah_attr),
					NATIVE_AUTH_IDX,
					(u64)swqe,
					qp->ibqp.qp_num,
					qp->remote_qpn,
					wr->wr.ex.imm_data, wr->rkey,
					md_opts, send_eq, ct_handle,
					wr->remote_addr,
					tx_id, op_req, solicit,
					swqe->num_iov, 0,
					&cmd->dma_iovec);
	} else if (wr->wr.opcode != IB_WR_RDMA_READ &&
		   length <= HFI_TX_MAX_BUFFERED) {
		nslots = hfi_format_buff_rc_rdma(
					rq->hw_ctx, NATIVE_NI,
					start, length,
					tpid, 0,
					NATIVE_RC, ah_attr->sl,
					becn, priv->pkey,
					rdma_ah_get_path_bits(ah_attr),
					NATIVE_AUTH_IDX,
					(u64)swqe,
					qp->ibqp.qp_num,
					qp->remote_qpn,
					wr->wr.ex.imm_data, wr->rkey,
					md_opts, send_eq, ct_handle,
					wr->remote_addr,
					tx_id, op_req, solicit,
					&cmd->buff_put_match);
	} else {
		nslots = hfi_format_dma_rc_rdma(
					rq->hw_ctx, NATIVE_NI,
					start, length,
					tpid, length > 4096, //FIXME - use MTU
					NATIVE_RC, ah_attr->sl,
					becn, priv->pkey,
					rdma_ah_get_path_bits(ah_attr),
					NATIVE_AUTH_IDX,
					(u64)swqe,
					qp->ibqp.qp_num,
					qp->remote_qpn,
					wr->wr.ex.imm_data, wr->rkey,
					md_opts, send_eq, ct_handle,
					wr->remote_addr,
					tx_id, op_req, solicit,
					&cmd->dma);
	}

	if (swqe && nslots <= 0)
		kfree(swqe);
	else if (wr->wr.opcode != IB_WR_RDMA_READ)
		++priv->nfence_cnt;
	else
		++priv->fence_cnt;

	return nslots;
}

static
inline int hfi2_do_local_inv(struct rvt_qp *qp, struct ib_rdma_wr *wr)
{
	struct hfi_ibcontext *ctx = obj_to_ibctx(&qp->ibqp);
	u64 done = 0;
	int ret;

	ret = hfi_rkey_invalidate(ctx->rx_cmdq, wr->rkey,
				  (u64)&done);
	if (ret != 0)
		return ret;

	ret = hfi_eq_poll_cmd_complete(ctx->hw_ctx, &done);
	if (ret != 0)
		return ret;

	// TODO - SEND_FLAGS /w SIGNALED + WR_ID

	return 0;
}

/*
 * Issue the appropriate TX command
 * TODO STL3 notes: If no TX slots, schedule some async thread?
 */
static
int hfi2_do_tx_work(struct rvt_qp *qp, struct ib_send_wr *wr)
{
	struct hfi_ibcontext *ctx = obj_to_ibctx(&qp->ibqp);
	union hfi_tx_cq_command cmd;
	int retries = 10;
	int ret, nslots;
	bool signal, solicit, in_line;
	unsigned long flags;

	signal = !(qp->s_flags & RVT_S_SIGNAL_REQ_WR) ||
		 (wr->send_flags & IB_SEND_SIGNALED);
	solicit = !!(wr->send_flags & IB_SEND_SOLICITED);
	in_line = !!(wr->send_flags & IB_SEND_INLINE);

	if (wr->send_flags & IB_SEND_FENCE)
		hfi2_fence(qp);

	switch (wr->opcode) {
	case IB_WR_SEND:
	case IB_WR_SEND_WITH_IMM:
	case IB_WR_SEND_WITH_INV:
		nslots = hfi_format_send_wr(qp, wr, signal, in_line,
					    solicit, &cmd);
		break;
	case IB_WR_RDMA_WRITE:
	case IB_WR_RDMA_WRITE_WITH_IMM:
	case IB_WR_RDMA_READ:
		nslots = hfi_format_rc_rdma(qp, rdma_wr(wr), signal, in_line,
					    solicit, &cmd);
		break;
	case IB_WR_ATOMIC_CMP_AND_SWP:
	case IB_WR_ATOMIC_FETCH_AND_ADD:
		nslots = hfi_format_rc_rdma_atomic(qp, atomic_wr(wr), signal,
						   in_line, solicit, &cmd);
		break;
	case IB_WR_LOCAL_INV:
		hfi2_nfence(qp);
		nslots = hfi2_do_local_inv(qp, rdma_wr(wr));
		break;
	default:
		nslots = -EINVAL;
		break;
	}

	if (nslots <= 0)
		return nslots;

retry:
	spin_lock_irqsave(&ctx->tx_cmdq->lock, flags);
	ret = hfi_tx_command(ctx->tx_cmdq, (u64 *)&cmd, nslots);
	spin_unlock_irqrestore(&ctx->tx_cmdq->lock, flags);

	if (ret == -EAGAIN && retries) {
		/* TODO */
		msleep(2000);
		retries--;
		goto retry;
	}
	return ret;
}

static inline bool native_send_ok(struct rvt_qp *qp)
{
	if (unlikely(!(ib_rvt_state_ops[qp->state] & RVT_POST_SEND_OK)))
		return false;
	return true;
}

int hfi2_native_send(struct rvt_qp *qp, struct ib_send_wr *wr,
		     struct ib_send_wr **bad_wr)
{
	int ret;

	if (unlikely(qp->state != IB_QPS_RTS))
		return -EINVAL;
	/* TODO - need to implement SQD / SQE */
	if (!native_send_ok(qp))
		return -EINVAL;
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
