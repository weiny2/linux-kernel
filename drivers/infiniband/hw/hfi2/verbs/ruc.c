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
 * Intel(R) OPA Gen2 IB Driver
 */

#include "verbs.h"
#include "packet.h"

/*
 * Translate ib_wr_opcode into ib_wc_opcode.
 */
const enum ib_wc_opcode ib_hfi2_wc_opcode[IB_WR_RESERVED10 + 1] = {
	[IB_WR_RDMA_WRITE] = IB_WC_RDMA_WRITE,
	[IB_WR_RDMA_WRITE_WITH_IMM] = IB_WC_RDMA_WRITE,
	[IB_WR_SEND] = IB_WC_SEND,
	[IB_WR_SEND_WITH_IMM] = IB_WC_SEND,
	[IB_WR_RDMA_READ] = IB_WC_RDMA_READ,
	[IB_WR_ATOMIC_CMP_AND_SWP] = IB_WC_COMP_SWAP,
	[IB_WR_ATOMIC_FETCH_AND_ADD] = IB_WC_FETCH_ADD,
	[IB_WR_SEND_WITH_INV] = IB_WC_SEND,
	[IB_WR_LOCAL_INV] = IB_WC_LOCAL_INV,
	[IB_WR_REG_MR] = IB_WC_REG_MR
};

const struct rvt_operation_params hfi2_post_parms[RVT_OPERATION_MAX] = {
[IB_WR_RDMA_WRITE] = {
	.length = sizeof(struct ib_rdma_wr),
	.qpt_support = BIT(IB_QPT_UC) | BIT(IB_QPT_RC),
},

[IB_WR_RDMA_READ] = {
	.length = sizeof(struct ib_rdma_wr),
	.qpt_support = BIT(IB_QPT_RC),
	.flags = RVT_OPERATION_ATOMIC,
},

[IB_WR_ATOMIC_CMP_AND_SWP] = {
	.length = sizeof(struct ib_atomic_wr),
	.qpt_support = BIT(IB_QPT_RC),
	.flags = RVT_OPERATION_ATOMIC | RVT_OPERATION_ATOMIC_SGE,
},

[IB_WR_ATOMIC_FETCH_AND_ADD] = {
	.length = sizeof(struct ib_atomic_wr),
	.qpt_support = BIT(IB_QPT_RC),
	.flags = RVT_OPERATION_ATOMIC | RVT_OPERATION_ATOMIC_SGE,
},

[IB_WR_RDMA_WRITE_WITH_IMM] = {
	.length = sizeof(struct ib_rdma_wr),
	.qpt_support = BIT(IB_QPT_UC) | BIT(IB_QPT_RC),
},

[IB_WR_SEND] = {
	.length = sizeof(struct ib_send_wr),
	.qpt_support = BIT(IB_QPT_UD) | BIT(IB_QPT_SMI) | BIT(IB_QPT_GSI) |
		      BIT(IB_QPT_UC) | BIT(IB_QPT_RC),
},

[IB_WR_SEND_WITH_IMM] = {
	.length = sizeof(struct ib_send_wr),
	.qpt_support = BIT(IB_QPT_UD) | BIT(IB_QPT_SMI) | BIT(IB_QPT_GSI) |
		       BIT(IB_QPT_UC) | BIT(IB_QPT_RC),
},

[IB_WR_REG_MR] = {
	.length = sizeof(struct ib_reg_wr),
	.qpt_support = BIT(IB_QPT_UC) | BIT(IB_QPT_RC),
	.flags = RVT_OPERATION_LOCAL,
},

[IB_WR_LOCAL_INV] = {
	.length = sizeof(struct ib_send_wr),
	.qpt_support = BIT(IB_QPT_UC) | BIT(IB_QPT_RC),
	.flags = RVT_OPERATION_LOCAL,
},

[IB_WR_SEND_WITH_INV] = {
	.length = sizeof(struct ib_send_wr),
	.qpt_support = BIT(IB_QPT_RC),
},

};

/* when sending, force a reschedule every one of these periods */
#define SEND_RESCHED_TIMEOUT (5 * HZ)  /* 5s in jiffies */

/*
 * Send if not busy or waiting for I/O and either
 * a RC response is pending or we can process send work requests.
 */
static inline int send_ok(struct rvt_qp *qp)
{
	return !(qp->s_flags & (RVT_S_BUSY | RVT_S_ANY_WAIT_IO)) &&
		(qp->s_hdrwords || (qp->s_flags & RVT_S_RESP_PENDING) ||
		 !(qp->s_flags & RVT_S_ANY_WAIT_SEND));
}

/*
 * Validate a RWQE and fill in the receive SGE state.
 * Return 1 if OK.
 */
static int init_sge(struct rvt_qp *qp, struct rvt_rwqe *wqe)
{
	int i, j, ret;
	struct ib_wc wc;
	struct rvt_lkey_table *rkt;
	struct rvt_pd *pd;
	struct rvt_sge_state *ss;

	rkt = &ib_to_rvt(qp->ibqp.device)->lkey_table;
	pd = ibpd_to_rvtpd(qp->ibqp.srq ? qp->ibqp.srq->pd : qp->ibqp.pd);
	ss = &qp->r_sge;
	ss->sg_list = qp->r_sg_list;
	qp->r_len = 0;
	for (i = j = 0; i < wqe->num_sge; i++) {
		if (wqe->sg_list[i].length == 0)
			continue;
		/* Check LKEY */
		ret = rvt_lkey_ok(rkt, pd, j ? &ss->sg_list[j - 1] : &ss->sge,
				  NULL, &wqe->sg_list[i],
				  IB_ACCESS_LOCAL_WRITE);
		if (unlikely(ret <= 0))
			goto bad_lkey;
		qp->r_len += wqe->sg_list[i].length;
		j++;
	}
	ss->num_sge = j;
	ss->total_len = qp->r_len;
	ret = 1;
	goto bail;

bad_lkey:
	while (j) {
		struct rvt_sge *sge = --j ? &ss->sg_list[j - 1] : &ss->sge;

		rvt_put_mr(sge->mr);
	}
	ss->num_sge = 0;
	memset(&wc, 0, sizeof(wc));
	wc.wr_id = wqe->wr_id;
	wc.status = IB_WC_LOC_PROT_ERR;
	wc.opcode = IB_WC_RECV;
	wc.qp = &qp->ibqp;
	/* Signal solicited completion event. */
	rvt_cq_enter(ibcq_to_rvtcq(qp->ibqp.recv_cq), &wc, 1);
	ret = 0;
bail:
	return ret;
}

/**
 * hfi2_get_rwqe - copy the next RWQE into the QP's RWQE
 * @qp: the QP
 * @wr_id_only: update qp->r_wr_id only, not qp->r_sge
 *
 * Can be called from interrupt level.
 *
 * Return: -1 if there is a local error, 0 if no RWQE is available,
 * otherwise return 1.
 */
int hfi2_get_rwqe(struct rvt_qp *qp, int wr_id_only)
{
	unsigned long flags;
	struct rvt_rq *rq;
	struct rvt_rwq *wq;
	struct rvt_srq *srq;
	struct rvt_rwqe *wqe;

	void (*handler)(struct ib_event *, void *);
	u32 tail;
	int ret;

	if (qp->ibqp.srq) {
		srq = ibsrq_to_rvtsrq(qp->ibqp.srq);
		handler = srq->ibsrq.event_handler;
		rq = &srq->rq;
	} else {
		srq = NULL;
		handler = NULL;
		rq = &qp->r_rq;
	}

	spin_lock_irqsave(&rq->lock, flags);
	if (!(ib_rvt_state_ops[qp->state] & RVT_PROCESS_RECV_OK)) {
		ret = 0;
		goto unlock;
	}

	wq = rq->wq;
	tail = wq->tail;
	/* Validate tail before using it since it is user writable. */
	if (tail >= rq->size)
		tail = 0;
	if (unlikely(tail == wq->head)) {
		ret = 0;
		goto unlock;
	}
	/* Make sure entry is read after head index is read. */
	smp_rmb();
	wqe = rvt_get_rwqe_ptr(rq, tail);
	/*
	 * Even though we update the tail index in memory, the verbs
	 * consumer is not supposed to post more entries until a
	 * completion is generated.
	 */
	if (++tail >= rq->size)
		tail = 0;
	wq->tail = tail;
	if (!wr_id_only && !init_sge(qp, wqe)) {
		ret = -1;
		goto unlock;
	}
	qp->r_wr_id = wqe->wr_id;

	ret = 1;
	set_bit(RVT_R_WRID_VALID, &qp->r_aflags);
	if (handler) {
		u32 n;

		/*
		 * Validate head pointer value and compute
		 * the number of remaining WQEs.
		 */
		n = wq->head;
		if (n >= rq->size)
			n = 0;
		if (n < tail)
			n += rq->size - tail;
		else
			n -= tail;
		if (n < srq->limit) {
			struct ib_event ev;

			srq->limit = 0;
			spin_unlock_irqrestore(&rq->lock, flags);
			ev.device = qp->ibqp.device;
			ev.element.srq = qp->ibqp.srq;
			ev.event = IB_EVENT_SRQ_LIMIT_REACHED;
			handler(&ev, srq->ibsrq.srq_context);
			goto bail;
		}
	}
unlock:
	spin_unlock_irqrestore(&rq->lock, flags);
bail:
	return ret;
}

static int gid_ok(union ib_gid *gid, __be64 gid_prefix, __be64 id)
{
	return (gid->global.interface_id == id &&
		(gid->global.subnet_prefix == gid_prefix ||
		gid->global.subnet_prefix == IB_DEFAULT_GID_PREFIX));
}

/*
 * This should be called with the QP r_lock held.
 * The s_lock will be acquired around the hfi2_migrate_qp() call.
 */
int hfi2_ruc_check_hdr(struct hfi2_ibport *ibp, struct hfi2_ib_packet *packet)
{
	struct rvt_qp *qp = packet->qp;
	struct hfi2_qp_priv *priv = qp->priv;
	__be64 guid;
	unsigned long flags;
	struct hfi_pportdata *ppd = ibp->ppd;
	u8 sc5 = ppd->sl_to_sc[rdma_ah_get_sl(&qp->remote_ah_attr)];
	u32 dlid = packet->dlid;
	u32 slid = packet->slid;
	u32 sl = packet->sl;
	bool migrated = packet->migrated;
	u16 pkey = packet->pkey;

	priv->ibrcv = packet->rcv_ctx;

	if (qp->s_mig_state == IB_MIG_ARMED && (migrated)) {
		if (!packet->grh) {
			if ((rdma_ah_get_ah_flags(&qp->alt_ah_attr)
			     & IB_AH_GRH) &&
			    packet->etype != RHF_RCV_TYPE_BYPASS)
				return 1;
		} else {
			const struct ib_global_route *grh;

			if (!(rdma_ah_get_ah_flags(&qp->alt_ah_attr) &
			      IB_AH_GRH))
				return 1;
			grh = rdma_ah_read_grh(&qp->alt_ah_attr);
			guid = get_sguid(ppd, grh->sgid_index);
			if (!gid_ok(&packet->grh->dgid,
				    ibp->rvp.gid_prefix, guid))
				return 1;
			if (!gid_ok(
				&packet->grh->sgid,
				grh->dgid.global.subnet_prefix,
				grh->dgid.global.interface_id))
				return 1;
		}
		if (unlikely(hfi_rcv_pkey_check(ibp->ppd, pkey,
						sc5, slid))) {
			hfi2_bad_pkey(ibp, pkey, sl,
				      0, qp->ibqp.qp_num, slid, dlid);
			return 1;
		}
		/* Validate the SLID. See Ch. 9.6.1.5 and 17.2.8 */
		if ((slid != rdma_ah_get_dlid(&qp->alt_ah_attr)) ||
		    ibp->ppd->pnum != rdma_ah_get_port_num(&qp->alt_ah_attr))
			return 1;
		spin_lock_irqsave(&qp->s_lock, flags);
		hfi2_migrate_qp(qp);
		spin_unlock_irqrestore(&qp->s_lock, flags);
	} else {
		if (!packet->grh) {
			if ((rdma_ah_get_ah_flags(&qp->remote_ah_attr)
			     & IB_AH_GRH) &&
			    packet->etype != RHF_RCV_TYPE_BYPASS)
				return 1;
		} else {
			const struct ib_global_route *grh;

			if (!(rdma_ah_get_ah_flags(&qp->remote_ah_attr) &
			      IB_AH_GRH))
				return 1;
			grh = rdma_ah_read_grh(&qp->remote_ah_attr);
			guid = get_sguid(ppd, grh->sgid_index);
			if (!gid_ok(&packet->grh->dgid,
				    ibp->rvp.gid_prefix, guid))
				return 1;
			if (!gid_ok(
			     &packet->grh->sgid,
			     grh->dgid.global.subnet_prefix,
			     grh->dgid.global.interface_id))
				return 1;
		}
		if (unlikely(hfi_rcv_pkey_check(ibp->ppd, pkey,
						sc5, slid))) {
			hfi2_bad_pkey(ibp, pkey, sl,
				      0, qp->ibqp.qp_num, slid, dlid);
			return 1;
		}
		/* Validate the SLID. See Ch. 9.6.1.5 */
		if ((slid != rdma_ah_get_dlid(&qp->remote_ah_attr)) ||
		    ibp->ppd->pnum != qp->port_num)
			return 1;
		if (qp->s_mig_state == IB_MIG_REARM &&
		    !migrated)
			qp->s_mig_state = IB_MIG_ARMED;
	}

	return 0;
}

static inline void hfi2_make_ruc_bth(struct rvt_qp *qp,
				     struct ib_other_headers *ohdr,
				     u32 bth0, u32 bth1, u32 bth2)
{
	bth1 |= qp->remote_qpn;
	ohdr->bth[0] = cpu_to_be32(bth0);
	ohdr->bth[1] = cpu_to_be32(bth1);
	ohdr->bth[2] = cpu_to_be32(bth2);
}

static inline void hfi2_make_ruc_header_16B(struct hfi2_ibport *ibp,
					    struct rvt_qp *qp,
					    struct ib_other_headers *ohdr,
					    u32 bth0, u32 bth2)
{
	struct hfi2_qp_priv *priv = qp->priv;
	struct hfi_pportdata *ppd = ibp->ppd;
	u32 bth1 = 0;
	u32 slid;
	u32 dlid = rdma_ah_get_dlid(&qp->remote_ah_attr);
	u16 pkey = priv->pkey;
	u8 l4 = HFI1_L4_IB_LOCAL;

	u8 extra_bytes = hfi2_get_16b_padding((qp->s_hdrwords << 2),
					      qp->s_cur_size);
	u32 nwords = SIZE_OF_CRC + ((qp->s_cur_size +
				     extra_bytes + SIZE_OF_LT) >> 2);
	bool becn = false;

	if (unlikely(rdma_ah_get_ah_flags(&qp->remote_ah_attr) & IB_AH_GRH) &&
	    hfi2_check_mcast(rdma_ah_get_dlid(&qp->remote_ah_attr))) {
		struct ib_grh *grh;
		struct ib_global_route *grd =
			rdma_ah_retrieve_grh(&qp->remote_ah_attr);
		int hdrwords;

		/*
		 * Ensure OPA GIDs are transformed to IB gids
		 * before creating the GRH.
		 */
		if (grd->sgid_index == OPA_GID_INDEX)
			grd->sgid_index = 0;
		grh = &priv->s_hdr->opah.u.l.grh;
		l4 = HFI1_L4_IB_GLOBAL;
		hdrwords = qp->s_hdrwords - 4;
		qp->s_hdrwords += hfi2_make_grh(ibp, grh, grd,
						hdrwords, nwords);
	}

	if (qp->s_mig_state == IB_MIG_MIGRATED)
		bth1 |= IB_16B_BTH_MIG_REQ;

	bth0 |= pkey;
	bth0 |= extra_bytes << 20;
	if (qp->s_flags & RVT_S_ECN) {
		qp->s_flags &= ~RVT_S_ECN;
		/* we recently received a FECN, so return a BECN */
		becn = true;
	}
	hfi2_make_ruc_bth(qp, ohdr, bth0, bth1, bth2);

	if (!ppd->lid)
		slid = be32_to_cpu(OPA_LID_PERMISSIVE);
	else
		slid = ppd->lid | (rdma_ah_get_path_bits(&qp->remote_ah_attr) &
				   ((1 << ppd->lmc) - 1));

	hfi2_make_16b_hdr(&priv->s_hdr->opah, slid,
			  opa_get_lid(dlid, 16B),
			  (qp->s_hdrwords + nwords) >> 1,
			  pkey, becn, 0, l4, priv->s_sc);
}

static inline void hfi2_make_ruc_header_9B(struct hfi2_ibport *ibp,
					   struct rvt_qp *qp,
					   struct ib_other_headers *ohdr,
					   u32 bth0, u32 bth2)
{
	struct hfi2_qp_priv *priv = qp->priv;
	struct hfi_pportdata *ppd = ibp->ppd;
	u32 bth1 = 0;
	u32 dlid = rdma_ah_get_dlid(&qp->remote_ah_attr);
	u16 pkey = priv->pkey;
	u16 lrh0 = HFI1_LRH_BTH;
	u16 slid;
	u8 extra_bytes = -qp->s_cur_size & 3;
	u32 nwords = SIZE_OF_CRC + ((qp->s_cur_size +
					 extra_bytes) >> 2);

	if (unlikely(rdma_ah_get_ah_flags(&qp->remote_ah_attr) & IB_AH_GRH)) {
		struct ib_grh *grh = &priv->s_hdr->ph.ibh.u.l.grh;
		int hdrwords = qp->s_hdrwords - 2;

		lrh0 = HFI1_LRH_GRH;
		qp->s_hdrwords +=
			hfi2_make_grh(ibp, grh,
				      rdma_ah_read_grh(&qp->remote_ah_attr),
				      hdrwords, nwords);
	} else {
		lrh0 = HFI1_LRH_BTH;
	}
	lrh0 |= (priv->s_sc & 0xf) << 12 |
		(rdma_ah_get_sl(&qp->remote_ah_attr) & 0xf) << 4;

	if (qp->s_mig_state == IB_MIG_MIGRATED)
		bth0 |= IB_BTH_MIG_REQ;

	bth0 |= pkey;
	bth0 |= extra_bytes << 20;
	if (qp->s_flags & RVT_S_ECN) {
		qp->s_flags &= ~RVT_S_ECN;
		/* we recently received a FECN, so return a BECN */
		bth1 |= (IB_BECN_MASK << IB_BECN_SHIFT);
	}
	hfi2_make_ruc_bth(qp, ohdr, bth0, bth1, bth2);

	if (!ppd->lid)
		slid = be16_to_cpu(IB_LID_PERMISSIVE);
	else
		slid = ppd->lid | (rdma_ah_get_path_bits(&qp->remote_ah_attr) &
				   ((1 << ppd->lmc) - 1));
	hfi2_make_ib_hdr(&priv->s_hdr->ph.ibh, lrh0,
			 qp->s_hdrwords + nwords,
			 opa_get_lid(dlid, 9B),
			 slid);
}

typedef void (*hfi2_make_ruc_hdr)(struct hfi2_ibport *ibp,
				  struct rvt_qp *qp,
				  struct ib_other_headers *ohdr,
				  u32 bth0, u32 bth2);

/* We support only two types - 9B and 16B for now */
static const hfi2_make_ruc_hdr hfi2_ruc_header_tbl[2] = {
	[HFI2_PKT_TYPE_9B] = &hfi2_make_ruc_header_9B,
	[HFI2_PKT_TYPE_16B] = &hfi2_make_ruc_header_16B
};

/*
 * Switch to alternate path.
 * The QP s_lock should be held and interrupts disabled.
 */
void hfi2_migrate_qp(struct rvt_qp *qp)
{
	struct ib_event ev;

	qp->s_mig_state = IB_MIG_MIGRATED;
	qp->remote_ah_attr = qp->alt_ah_attr;
	qp->port_num = rdma_ah_get_port_num(&qp->alt_ah_attr);
	qp->s_pkey_index = qp->s_alt_pkey_index;

	ev.device = qp->ibqp.device;
	ev.element.qp = &qp->ibqp;
	ev.event = IB_EVENT_PATH_MIG;
	qp->ibqp.event_handler(&ev, qp->ibqp.qp_context);
}

void hfi2_make_ruc_header(struct rvt_qp *qp, struct ib_other_headers *ohdr,
			  u32 bth0, u32 bth2, bool is_middle)
{
	struct hfi2_qp_priv *qp_priv = qp->priv;
	struct hfi2_ibport *ibp;
	struct hfi_pportdata *ppd;

	ibp = to_hfi_ibp(qp->ibqp.device, qp->port_num);
	ppd = ibp->ppd;

	qp_priv->s_sl = rdma_ah_get_sl(&qp->remote_ah_attr);
	qp_priv->s_sc = ppd->sl_to_sc[qp_priv->s_sl];
	qp_priv->opcode = (bth0 >> 24) & 0xFF;
	qp_priv->send_becn = false;
	qp_priv->pkey = hfi2_get_pkey(ibp, qp->s_pkey_index);
	hfi_set_middle_length(qp, bth2, is_middle);
	hfi2_ruc_header_tbl[qp_priv->hdr_type](ibp, qp, ohdr, bth0, bth2);
}

/**
 * egress_pkey_check - check P_KEY of a packet
 * @ppd:  Physical IB port data
 * @pkey: PKEY for header
 * @sc5:  SC for packet
 *
 * It checks if hdr's pkey is valid.
 *
 * Return: 0 on success, otherwise, 1
 */
int egress_pkey_check(struct hfi_pportdata *ppd, u16 pkey,
		      u8 sc5)
{
	/* If SC15, pkey[0:14] must be 0x7fff */
	if ((sc5 == 0xf) && (HFI_PKEY_CAM(pkey) != HFI_PKEY_CAM_MASK))
		goto bad;

	/* Is the pkey = 0x0, or 0x8000? */
	if (HFI_PKEY_CAM(pkey) == 0)
		goto bad;

	return 0;
bad:
	incr_cntr64(&ppd->xmit_constraint_errors);

	if (!(ppd->err_info_xmit_constraint.status &
	     OPA_EI_STATUS_SMASK)) {
		ppd->err_info_xmit_constraint.status |=
			OPA_EI_STATUS_SMASK;
		ppd->err_info_xmit_constraint.slid = ppd->lid;
		ppd->err_info_xmit_constraint.pkey = pkey;
	}
	return 1;
}

/**
 * hfi2_verbs_send - send a packet
 * @qp: the QP to send on
 *
 * Return: zero if packet is sent or queued OK, non-zero
 * and clear qp->s_flags RVT_S_BUSY otherwise.
 */
static int hfi2_verbs_send(struct rvt_qp *qp, bool allow_sleep)
{
	struct hfi2_ibport *ibp;
	int ret = 0;
	unsigned long flags = 0;
	enum ib_wc_status wc_stat;
	struct hfi2_qp_priv *qp_priv = qp->priv;

	ibp = to_hfi_ibp(qp->ibqp.device, qp->port_num);

	ret = egress_pkey_check(ibp->ppd, qp_priv->pkey,
				qp_priv->s_sc);
	if (unlikely(ret)) {
		wc_stat = IB_WC_GENERAL_ERR;
		ret = -EINVAL;
		goto send_err;
	}

	/* send WGE into fabric */
	ret = ibp->ibd->send_wqe(ibp, qp->priv, allow_sleep);
	if (ret < 0) {
		wc_stat = IB_WC_FATAL_ERR;
		goto send_err;
	}
	/* else send_complete issued upon DMA completion event */
	return ret;

send_err:
	if (qp->s_wqe) {
		spin_lock_irqsave(&qp->s_lock, flags);
		hfi2_send_complete(qp, qp->s_wqe, wc_stat);
		spin_unlock_irqrestore(&qp->s_lock, flags);
	}
	return ret;
}

/**
 * hfi2_do_send - perform a send on a QP
 * @qp: pointer to the QP
 * @in_thread: true if in a workqueue thread
 *
 * Process entries in the send work queue until credit or queue is
 * exhausted.  Only allow one CPU to send a packet per QP (tasklet).
 * Otherwise, two threads could send packets out of order.
 */
static void hfi2_do_send(struct rvt_qp *qp, bool in_thread)
{
	unsigned long flags;
	int (*make_req)(struct rvt_qp *qp);
	unsigned long timeout;
	unsigned long timeout_int;
	struct hfi2_ibport *ibp = to_hfi_ibp(qp->ibqp.device, qp->port_num);

	/* FXRTODO - WFR performs RC/UC loopback here */
	timeout_int = SEND_RESCHED_TIMEOUT;
	if (qp->ibqp.qp_type == IB_QPT_RC) {
		timeout_int = (qp->timeout_jiffies);
		make_req = hfi2_make_rc_req;
	} else if (qp->ibqp.qp_type == IB_QPT_UC) {
		make_req = hfi2_make_uc_req;
	} else {
		make_req = hfi2_make_ud_req;
	}

	spin_lock_irqsave(&qp->s_lock, flags);
	/* Return if we are already busy processing a work request. */
	if (!send_ok(qp)) {
		spin_unlock_irqrestore(&qp->s_lock, flags);
		return;
	}
	qp->s_flags |= RVT_S_BUSY;
	timeout = jiffies + (timeout_int) / 8;
	do {
		/* Check for a constructed packet to be sent. */
		if (qp->s_hdrwords != 0) {
			spin_unlock_irqrestore(&qp->s_lock, flags);
			/*
			 * If the packet cannot be sent now, return and
			 * the send tasklet will be woken up later.
			 */
			if (hfi2_verbs_send(qp, in_thread))
				return;

			/* Record that s_hdr is empty. */
			qp->s_hdrwords = 0;
		       /* allow other tasks to run */
			if (unlikely(time_after(jiffies, timeout))) {
				if (!in_thread ||
				    workqueue_congested(WORK_CPU_UNBOUND,
							ibp->ppd->hfi_wq)) {
					spin_lock_irqsave(&qp->s_lock, flags);
					qp->s_flags &= ~RVT_S_BUSY;
					hfi2_schedule_send(qp);
					spin_unlock_irqrestore(&qp->s_lock,
							       flags);
					return;
				}
				cond_resched();
				timeout = jiffies + (timeout_int) / 8;
			}
			spin_lock_irqsave(&qp->s_lock, flags);
		}
	} while (make_req(qp));
	spin_unlock_irqrestore(&qp->s_lock, flags);
}

void hfi2_do_send_from_rvt(struct rvt_qp *qp)
{
	hfi2_do_send(qp, false);
}

void hfi2_do_work(struct work_struct *work)
{
	struct iowait *wait = container_of(work, struct iowait, iowork);
	struct hfi2_qp_priv *priv;

	priv = container_of(wait, struct hfi2_qp_priv, s_iowait);
	hfi2_do_send(priv->owner, true);
}

/*
 * This should be called with s_lock held.
 */
void hfi2_send_complete(struct rvt_qp *qp, struct rvt_swqe *wqe,
			enum ib_wc_status status)
{
	u32 old_last, last;

	if (!(ib_rvt_state_ops[qp->state] & RVT_PROCESS_OR_FLUSH_SEND))
		return;
	last = qp->s_last;
	old_last = last;
	if (++last >= qp->s_size)
		last = 0;
	qp->s_last = last;
	/* See post_send() */
	barrier();
	rvt_put_swqe(wqe);
	if (qp->ibqp.qp_type == IB_QPT_UD ||
	    qp->ibqp.qp_type == IB_QPT_SMI ||
	    qp->ibqp.qp_type == IB_QPT_GSI)
		atomic_dec(&ibah_to_rvtah(wqe->ud_wr.ah)->refcount);

	rvt_qp_swqe_complete(qp, wqe,
			     ib_hfi2_wc_opcode[wqe->wr.opcode],
			     status);
	if (qp->s_acked == old_last)
		qp->s_acked = last;
	if (qp->s_cur == old_last)
		qp->s_cur = last;
	if (qp->s_tail == old_last)
		qp->s_tail = last;
	if (qp->state == IB_QPS_SQD && last == qp->s_cur)
		qp->s_draining = 0;
}

/**
 * hfi2_schedule_send_no_lock - schedule progress
 * @qp: the QP
 *
 * This schedules qp progress w/o regard to the s_flags.
 *
 * It is only used in the post send, which doesn't hold
 * the s_lock.
 */
void hfi2_schedule_send_no_lock(struct rvt_qp *qp)
{
	struct hfi2_qp_priv *qp_priv = qp->priv;
	struct hfi2_ibport *ibp;

	ibp = to_hfi_ibp(qp->ibqp.device, qp->port_num);
	iowait_schedule(&qp_priv->s_iowait, ibp->ppd->hfi_wq);
}

/**
 * hfi2_schedule_send - schedule progress
 * @qp: the QP
 *
 * This schedules qp progress and caller should hold
 * the s_lock.
 */
void hfi2_schedule_send(struct rvt_qp *qp)
{
	lockdep_assert_held(&qp->s_lock);
	if (send_ok(qp))
		hfi2_schedule_send_no_lock(qp);
}
