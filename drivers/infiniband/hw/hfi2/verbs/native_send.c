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

#define QKEY_USE_LOCAL		0x80000000

/* Map IB WR opcodes to IB WC opcodes */
const u8 hfi_wr_wc_opcode[13] = {
	[IB_WR_RDMA_WRITE] = IB_WC_RDMA_WRITE,
	[IB_WR_RDMA_WRITE_WITH_IMM] = IB_WC_RDMA_WRITE,
	[IB_WR_SEND] = IB_WC_SEND,
	[IB_WR_SEND_WITH_IMM] = IB_WC_SEND,
	[IB_WR_RDMA_READ] = IB_WC_RDMA_READ,
	[IB_WR_ATOMIC_CMP_AND_SWP] = IB_WC_COMP_SWAP,
	[IB_WR_ATOMIC_FETCH_AND_ADD] = IB_WC_FETCH_ADD,
	[IB_WR_LOCAL_INV] = IB_WC_LOCAL_INV,
	[IB_WR_SEND_WITH_INV] = IB_WC_SEND,
	[IB_WR_REG_MR] = IB_WC_REG_MR,
};

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

void hfi_preformat_rc(struct hfi_ctx *ctx, u32 dlid, u8 rc,
		      u8 sl, u16 pkey, u8 slid_low,  u8 auth_idx,
		      struct hfi_eq *eq_handle, u32 qpn,
		      union hfi_tx_cq_command *command)
{
	hfi_cmd_t cmd = {.val = 0};
	union hfi_process target_id = {.phys.slid =  dlid};

	_hfi_format_verb_put_flit0(&command->dma.flit0,
				   cmd, (hfi_tx_ctype_t)VoNP_RC, 0,
				   target_id, 0, 0, sl,
				   pkey, slid_low, auth_idx, 0,
				   0, eq_handle,
				   PTL_CT_NONE,
				   0, 0, qpn);

	_hfi_format_put_flit_e1(ctx, &command->buff_put.flit1.e, (qpn >> 8),
				PTL_UINT64_T, 0);
}

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

static inline
void *mr_sge_addr(struct rvt_mregion *mr, struct ib_sge *sge)
{
	if (!mr->mapsz || !mr->lkey)
		return (void *)sge->addr;
	/* TODO - no support yet for potentially discontiguous mappings */
	BUG_ON(sge->length > mr->map[0]->segs[0].length);
	/* Should have been already checked */
	BUG_ON(mr->user_base > sge->addr);
	return (sge->addr - mr->user_base) + mr->map[0]->segs[0].vaddr;
}

/* Convert Send WR into state needed for send completion and hardware IOVEC */
static
struct hfi_swqe *hfi_prepare_swqe(struct rvt_qp *qp, struct ib_send_wr *wr,
				  bool signal, int acc)
{
	int ret, i;
	struct hfi_ibcontext *ctx = obj_to_ibctx(&qp->ibqp);
	struct hfi_swqe *swqe;
	struct hfi_iovec *iov;
	struct rvt_mregion *mr;

	swqe = kmalloc(sizeof(*swqe) + wr->num_sge * sizeof(*iov), GFP_KERNEL);
	if (!swqe)
		return ERR_PTR(-ENOMEM);
	iov = swqe->iov;

	swqe->length = 0;
	for (i = 0; i < wr->num_sge; i++) {
		mr = hfi2_chk_mr_sge(ctx, &wr->sg_list[i], acc);
		if (!mr) {
			ret = -EINVAL;
			goto err;
		}

		iov[i].addr = (u64)mr_sge_addr(mr, &wr->sg_list[i]);
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
	return ERR_PTR(ret);
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
	int nslots;
	struct rvt_ah *ah = ibah_to_rvtah(wr->ah);
	struct rdma_ah_attr *ah_attr = &ah->attr;
	struct ib_sge *sge = wr->wr.sg_list;
	struct rvt_mregion *mr;
	struct hfi2_qp_priv *priv = qp->priv;
	struct hfi_rq *rq = qp->r_rq.hw_rq;
	struct rvt_cq *send_cq = ibcq_to_rvtcq(qp->ibqp.send_cq);
	struct hfi_eq *send_eq = send_cq->hw_cq;
	u8 op_req = hfi_wr_ud_opcode[wr->wr.opcode];
	u32 dlid = rdma_ah_get_dlid(ah_attr);
	u32 qkey, md_opts = PTL_MD_EVENT_SUCCESS_DISABLE |
		PTL_MD_EVENT_CT_SEND;
	u16 tx_id = 0;
	struct hfi_swqe *swqe = NULL;
	void *start = NULL;
	u32 length = 0;

	if (signal || wr->wr.num_sge > 1) {
		/* validate SG list and build IOVEC array if multiple SGEs */
		md_opts = PTL_MD_EVENT_CT_SEND;
		swqe = hfi_prepare_swqe(qp, &wr->wr, signal, 0);
		if (IS_ERR(swqe))
			return PTR_ERR(swqe);
		length = swqe->length;
		if (length)
			start = (void *)swqe->iov[0].addr;
	} else if (wr->wr.num_sge) {
		/* validate single SGE */
		mr = hfi2_chk_mr_sge(obj_to_ibctx(&qp->ibqp), sge, 0);
		if (!mr)
			return -EINVAL;
		start = mr_sge_addr(mr, sge);
		length = sge->length;
	}

	qkey = !(wr->remote_qkey & QKEY_USE_LOCAL) ?
	       wr->remote_qkey : qp->qkey;

	if (wr->wr.num_sge > 1) {
		WARN_ON(in_line);
		md_opts = PTL_IOVEC | PTL_MD_EVENT_CT_SEND;
		nslots = hfi_format_dma_iovec_ud(
					rq->hw_ctx,
					(void *)swqe->iov, swqe->length,
					dlid, 1,
					NATIVE_RC, ah_attr->sl,
					priv->pkey,
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
					rq->hw_ctx,
					start, length,
					dlid, 0,
					NATIVE_RC, ah_attr->sl,
					priv->pkey,
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
		WARN_ON(in_line);
		nslots = hfi_format_dma_ud(
					rq->hw_ctx,
					start, length,
					dlid, 0,
					NATIVE_RC, ah_attr->sl,
					priv->pkey,
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
	int nslots;
	struct ib_sge *sge = wr->sg_list;
	struct rvt_mregion *mr;
	struct hfi2_qp_priv *priv = qp->priv;
	struct hfi_rq *rq = qp->r_rq.hw_rq;
	u8 op_req = hfi_wr_rc_opcode[wr->opcode];
	u32 md_opts = PTL_MD_EVENT_SUCCESS_DISABLE | PTL_MD_EVENT_CT_SEND;
	u16 tx_id = 0;
	struct hfi_swqe *swqe = NULL;
	void *start = NULL;
	u32 length = 0;

	if (qp->ibqp.qp_type == IB_QPT_UC &&
	    wr->opcode == IB_WR_SEND_WITH_INV)
		return -EINVAL;

	if (signal || wr->num_sge > 1) {
		/* validate SG list and build IOVEC array if multiple SGEs */
		md_opts = PTL_MD_EVENT_CT_SEND;
		swqe = hfi_prepare_swqe(qp, wr, signal, 0);
		if (IS_ERR(swqe))
			return PTR_ERR(swqe);
		length = swqe->length;
		if (length)
			start = (void *)swqe->iov[0].addr;
	} else if (wr->num_sge) {
		/* validate single SGE */
		mr = hfi2_chk_mr_sge(obj_to_ibctx(&qp->ibqp), sge, 0);
		if (!mr)
			return -EINVAL;
		start = mr_sge_addr(mr, sge);
		length = sge->length;
	}

	if (wr->num_sge > 1) {
		WARN_ON(in_line);
		md_opts = PTL_IOVEC | PTL_MD_EVENT_CT_SEND;
		nslots = hfi_format_dma_iovec_rc_send(
					rq->hw_ctx,
					(void *)swqe->iov, swqe->length,
					1, NATIVE_RC,
					(u64)swqe,
					qp->ibqp.qp_num,
					qp->remote_qpn,
					wr->ex.imm_data,
					md_opts, priv->nfence_ct,
					tx_id, op_req, solicit,
					swqe->num_iov, 0,
					&cmd->dma_iovec);
	} else if (sge->length <= HFI_TX_MAX_BUFFERED) {
		nslots = hfi_format_buff_rc_send(
					rq->hw_ctx,
					start, length,
					0, NATIVE_RC,
					(u64)swqe,
					qp->ibqp.qp_num,
					qp->remote_qpn,
					wr->ex.imm_data,
					md_opts, priv->nfence_ct,
					tx_id, op_req, solicit,
					&cmd->buff_put_match);
	} else {
		WARN_ON(in_line);
		nslots = hfi_format_dma_rc_send(
					rq->hw_ctx,
					start, length,
					length > 4096, // FIXME - user MTU
					NATIVE_RC,
					(u64)swqe,
					qp->ibqp.qp_num,
					qp->remote_qpn,
					wr->ex.imm_data,
					md_opts, priv->nfence_ct,
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
	int nslots;
	struct ib_sge *sge = wr->wr.sg_list;
	struct rvt_mregion *mr;
	struct hfi2_qp_priv *priv = qp->priv;
	struct hfi_rq *rq = qp->r_rq.hw_rq;
	u8 op_req = hfi_wr_rc_opcode[wr->wr.opcode];
	u32 md_opts = PTL_MD_EVENT_SUCCESS_DISABLE | PTL_MD_EVENT_CT_SEND;
	u16 tx_id = 0;
	struct hfi_swqe *swqe = NULL;
	void *start = NULL;

	if (qp->ibqp.qp_type != IB_QPT_RC)
		return -EINVAL;

	if (!(wr->wr.num_sge == 1 && sge->length == 8))
		return -EINVAL;

	if (signal) {
		md_opts = PTL_MD_EVENT_CT_SEND;
		swqe = hfi_prepare_swqe(qp, &wr->wr, signal,
					IB_ACCESS_LOCAL_WRITE);
		if (IS_ERR(swqe))
			return PTR_ERR(swqe);
		start = (void *)swqe->iov[0].addr;
	} else {
		mr = hfi2_chk_mr_sge(obj_to_ibctx(&qp->ibqp), sge,
				     IB_ACCESS_LOCAL_WRITE);
		if (!mr)
			return -EINVAL;
		start = mr_sge_addr(mr, sge);
	}

	nslots = hfi_format_buff_rc_rdma_atomic(
				rq->hw_ctx,
				start,
				0, NATIVE_RC,
				(u64)swqe,
				qp->ibqp.qp_num,
				qp->remote_qpn,
				wr->wr.ex.imm_data, wr->rkey,
				md_opts, priv->fence_ct,
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
	int nslots;
	struct ib_sge *sge = wr->wr.sg_list;
	struct rvt_mregion *mr;
	struct hfi2_qp_priv *priv = qp->priv;
	struct hfi_rq *rq = qp->r_rq.hw_rq;
	u8 op_req = hfi_wr_rc_opcode[wr->wr.opcode];
	u32 md_opts = wr->wr.opcode != IB_WR_RDMA_READ ?
		PTL_MD_EVENT_SUCCESS_DISABLE | PTL_MD_EVENT_CT_SEND :
		PTL_MD_EVENT_SUCCESS_DISABLE | PTL_MD_EVENT_CT_REPLY;
	u16 tx_id = 0;
	struct hfi_swqe *swqe = NULL;
	void *start = NULL;
	u32 length = 0;
	int acc;
	hfi_ct_handle_t ct_handle = wr->wr.opcode != IB_WR_RDMA_READ ?
		priv->nfence_ct : priv->fence_ct;

	if (qp->ibqp.qp_type == IB_QPT_UD ||
	    (qp->ibqp.qp_type == IB_QPT_UC &&
	     wr->wr.opcode == IB_WR_RDMA_READ))
		return -EINVAL;

	acc = wr->wr.opcode >= IB_WR_RDMA_READ ?
		      IB_ACCESS_LOCAL_WRITE : 0;
	if (signal || wr->wr.num_sge > 1) {
		/* validate SG list and build IOVEC array if multiple SGEs */
		md_opts &= ~PTL_MD_EVENT_SUCCESS_DISABLE;
		swqe = hfi_prepare_swqe(qp, &wr->wr, signal, acc);
		if (IS_ERR(swqe))
			return PTR_ERR(swqe);
		length = swqe->length;
		if (length)
			start = (void *)swqe->iov[0].addr;
	} else if (wr->wr.num_sge) {
		/* validate single SGE */
		mr = hfi2_chk_mr_sge(obj_to_ibctx(&qp->ibqp), sge, acc);
		if (!mr)
			return -EINVAL;
		start = mr_sge_addr(mr, sge);
		length = sge->length;
	}

	if (wr->wr.num_sge > 1) {
		WARN_ON(in_line);
		md_opts &= ~PTL_MD_EVENT_SUCCESS_DISABLE;
		md_opts |= PTL_IOVEC;
		nslots = hfi_format_dma_iovec_rc_rdma(
					rq->hw_ctx,
					(void *)swqe->iov, swqe->length,
					1, NATIVE_RC,
					(u64)swqe,
					qp->ibqp.qp_num,
					qp->remote_qpn,
					wr->wr.ex.imm_data, wr->rkey,
					md_opts, ct_handle,
					wr->remote_addr,
					tx_id, op_req, solicit,
					swqe->num_iov, 0,
					&cmd->dma_iovec);
	} else if (wr->wr.opcode != IB_WR_RDMA_READ &&
		   length <= HFI_TX_MAX_BUFFERED) {
		nslots = hfi_format_buff_rc_rdma(
					rq->hw_ctx,
					start, length,
					0, NATIVE_RC,
					(u64)swqe,
					qp->ibqp.qp_num,
					qp->remote_qpn,
					wr->wr.ex.imm_data, wr->rkey,
					md_opts, ct_handle,
					wr->remote_addr,
					tx_id, op_req, solicit,
					&cmd->buff_put_match);
	} else {
		WARN_ON(in_line);
		nslots = hfi_format_dma_rc_rdma(
					rq->hw_ctx,
					start, length,
					length > 4096, //FIXME - use MTU
					NATIVE_RC,
					(u64)swqe,
					qp->ibqp.qp_num,
					qp->remote_qpn,
					wr->wr.ex.imm_data, wr->rkey,
					md_opts, ct_handle,
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

int hfi2_generate_wc(struct rvt_qp *qp, struct ib_send_wr *wr,
		     u8 mb_opcode, union hfi_tx_cq_command *cmd)
{
	struct hfi_ibcontext *ctx = obj_to_ibctx(&qp->ibqp);
	struct rvt_cq *cq = ibcq_to_rvtcq(qp->ibqp.send_cq);
	struct hfi_eq *eq = cq->hw_cq;
	int nslots;
	struct rdma_ah_attr *ah_attr;
	struct hfi_ctx *hw_ctx = ctx->hw_ctx;
	struct hfi_swqe *swqe = NULL;

	if (qp->ibqp.qp_type == IB_QPT_UD)
		ah_attr = &ibah_to_rvtah(ud_wr(wr)->ah)->attr;
	else
		ah_attr = &qp->remote_ah_attr;

	switch (mb_opcode) {
	case MB_OC_RX_FLUSH:
	case MB_OC_LOCAL_INV:
	case MB_OC_REG_MR:
		swqe = kmalloc(sizeof(*swqe), GFP_KERNEL);
		if (!swqe)
			return -ENOMEM;
		swqe->wr_id = wr->wr_id;
		swqe->num_iov = 0;
		swqe->signal = 1;
		swqe->qp = qp;
#if 0
		swqe->cidx = qp->current_cidx;
#endif
		break;
	default:
		/*TODO need to take some action */
		pr_warn("mb opcode %d not valid", mb_opcode);
		return -EINVAL;
	}

	nslots = hfi_format_buff_ud(
				hw_ctx,
				0, 0,
				0, ah_attr->port_num - 1,
				NATIVE_RC, ah_attr->sl,
				0, 0,
				NATIVE_AUTH_IDX,
				(u64)swqe,
				qp->ibqp.qp_num,
				0, 0, 0,
				PTL_MD_EVENT_CT_SEND, eq,
				PTL_CT_NONE,
				0, PTL_UD_SEND, 0, mb_opcode,
				&cmd->buff_put_match);

	return nslots;
}

static
inline int hfi2_do_reg_mr(struct rvt_qp *qp, struct ib_reg_wr *wr,
			       bool signal, union hfi_tx_cq_command *cmd)
{
	int ret;

	ret = rvt_fast_reg_mr(qp, wr->mr, wr->key,
			      wr->access);
	if (ret)
		return ret;
	ret = hfi2_native_reg_mr(rvt_ibmr_to_mregion(wr->mr));
	if (ret)
		return ret;
	/* returns err code or nslots required to post on tx_cmdq */
	if (signal)
		ret = hfi2_generate_wc(qp, &wr->wr, MB_OC_REG_MR,
				       cmd);
	return ret;
}

static
inline int hfi2_do_local_inv(struct rvt_qp *qp, struct ib_send_wr *wr,
			     bool signal, union hfi_tx_cq_command *cmd)
{
	struct hfi_ibcontext *ctx = obj_to_ibctx(&qp->ibqp);
	struct hfi_ctx  *hw_ctx;
	unsigned long flags;
	u64 done = 0;
	int ret;
	struct rvt_mregion *mr = NULL;
	u32 key = wr->ex.invalidate_rkey;

	if (!key || IS_INVALID_KEY(key))
		return 0;
	mr = _hfi2_find_mr_from_rkey(ctx, key);
	if (!mr)
		return 0;
	if (atomic_read(&mr->lkey_invalid)) {
		rvt_put_mr(mr);
		return 0;
	}
	atomic_set(&mr->lkey_invalid, 1);
	rvt_put_mr(mr);

	hw_ctx = ctx->hw_ctx;
	mutex_lock(&hw_ctx->rx_mutex);
	spin_lock_irqsave(&ctx->rx_cmdq->lock, flags);
	ret = hfi_rkey_invalidate(ctx->rx_cmdq, key, (u64)&done);
	spin_unlock_irqrestore(&ctx->rx_cmdq->lock, flags);
	if (ret != 0) {
		mutex_unlock(&hw_ctx->rx_mutex);
		return ret;
	}

	ret = hfi_eq_poll_cmd_complete(ctx->hw_ctx, &done);
	mutex_unlock(&hw_ctx->rx_mutex);
	if (ret != 0)
		return ret;
	/* returns err code or nslots required to post on tx_cmdq */
	if (signal)
		ret = hfi2_generate_wc(qp, wr, MB_OC_LOCAL_INV, cmd);
	// TODO - SEND_FLAGS /w SIGNALED + WR_ID

	return ret;
}

/*
 * Issue the appropriate TX command
 * TODO STL3 notes: If no TX slots, schedule some async thread?
 */
static
int hfi2_do_tx_work(struct rvt_qp *qp, struct ib_send_wr *wr)
{
	struct hfi_ibcontext *ctx = obj_to_ibctx(&qp->ibqp);
	struct hfi2_qp_priv *priv = (struct hfi2_qp_priv *)qp->priv;
	union hfi_tx_cq_command *cmd;
	int retries = 10;
	int ret, nslots = 0;
	bool signal, solicit, in_line;
	unsigned long flags;

	signal = !(qp->s_flags & RVT_S_SIGNAL_REQ_WR) ||
		 (wr->send_flags & IB_SEND_SIGNALED);
	solicit = !!(wr->send_flags & IB_SEND_SOLICITED);
	in_line = !!(wr->send_flags & IB_SEND_INLINE);

	if (wr->send_flags & IB_SEND_FENCE)
		hfi2_fence(qp);

	spin_lock_irqsave(&qp->s_lock, flags);
	cmd = &priv->cmd[priv->current_cidx % qp->s_size];

	switch (wr->opcode) {
	case IB_WR_SEND:
	case IB_WR_SEND_WITH_IMM:
	case IB_WR_SEND_WITH_INV:
		nslots = hfi_format_send_wr(qp, wr, signal, in_line,
					    solicit, cmd);
		break;
	case IB_WR_RDMA_WRITE:
	case IB_WR_RDMA_WRITE_WITH_IMM:
	case IB_WR_RDMA_READ:
		nslots = hfi_format_rc_rdma(qp, rdma_wr(wr), signal, in_line,
					    solicit, cmd);
		break;
	case IB_WR_ATOMIC_CMP_AND_SWP:
	case IB_WR_ATOMIC_FETCH_AND_ADD:
		nslots = hfi_format_rc_rdma_atomic(qp, atomic_wr(wr), signal,
						   in_line, solicit, cmd);
		break;
	case IB_WR_LOCAL_INV:
		hfi2_nfence(qp);
		nslots = hfi2_do_local_inv(qp, wr, signal, cmd);
		break;
	case IB_WR_REG_MR:
		nslots = hfi2_do_reg_mr(qp, reg_wr(wr), signal, cmd);
		break;
	default:
		nslots = -EINVAL;
		break;
	}

	if (nslots <= 0) {
		spin_unlock_irqrestore(&qp->s_lock, flags);
		return nslots;
	}
	priv->current_cidx = (priv->current_cidx + 1) % qp->s_size;
	spin_unlock_irqrestore(&qp->s_lock, flags);

retry:
	spin_lock_irqsave(&ctx->tx_cmdq->lock, flags);
	ret = hfi_tx_command(ctx->tx_cmdq, (u64 *)cmd, nslots);
	spin_unlock_irqrestore(&ctx->tx_cmdq->lock, flags);

	if (ret == -EAGAIN && retries) {
		/* TODO */
		msleep(2000);
		retries--;
		goto retry;
	}
	return ret;
}

static
int hfi2_flush_wr(struct rvt_qp *qp, struct ib_send_wr *wr,
		  struct ib_send_wr **bad_wr)
{
	unsigned long flags;

	spin_lock_irqsave(&qp->s_lock, flags);

	do {
		struct ib_wc wc;
		struct ib_qp *ibqp = &qp->ibqp;

		memset(&wc, 0, sizeof(wc));
		wc.qp = ibqp;
		wc.opcode = hfi_wr_wc_opcode[wr->opcode];
		wc.wr_id = wr->wr_id;
		wc.status = IB_WC_WR_FLUSH_ERR;
		rvt_cq_enter(ibcq_to_rvtcq(ibqp->send_cq), &wc, 1);
	} while ((wr = wr->next));

	spin_unlock_irqrestore(&qp->s_lock, flags);

	return 0;
}

static
int hfi2_queue_wr(struct rvt_qp *qp, struct ib_send_wr *wr,
		  struct ib_send_wr **bad_wr)
{
	/* TODO implement QP in SQD state */
	return 0;
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

	if (unlikely(qp->state != IB_QPS_RTS)) {
		if (qp->state >= IB_QPS_SQE)
			return hfi2_flush_wr(qp, wr, bad_wr);
		else if (qp->state == IB_QPS_SQD)
			return hfi2_queue_wr(qp, wr, bad_wr);
		*bad_wr = wr;
		return -EINVAL;
	}
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
