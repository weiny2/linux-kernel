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
const enum ib_wc_opcode ib_hfi1_wc_opcode[] = {
	[IB_WR_RDMA_WRITE] = IB_WC_RDMA_WRITE,
	[IB_WR_RDMA_WRITE_WITH_IMM] = IB_WC_RDMA_WRITE,
	[IB_WR_SEND] = IB_WC_SEND,
	[IB_WR_SEND_WITH_IMM] = IB_WC_SEND,
	[IB_WR_RDMA_READ] = IB_WC_RDMA_READ,
	[IB_WR_ATOMIC_CMP_AND_SWP] = IB_WC_COMP_SWAP,
	[IB_WR_ATOMIC_FETCH_AND_ADD] = IB_WC_FETCH_ADD
};

/*
 * Convert the AETH RNR timeout code into the number of microseconds.
 */
const u32 ib_hfi1_rnr_table[32] = {
	655360,	/* 00: 655.36 */
	10,	/* 01:    .01 */
	20,	/* 02     .02 */
	30,	/* 03:    .03 */
	40,	/* 04:    .04 */
	60,	/* 05:    .06 */
	80,	/* 06:    .08 */
	120,	/* 07:    .12 */
	160,	/* 08:    .16 */
	240,	/* 09:    .24 */
	320,	/* 0A:    .32 */
	480,	/* 0B:    .48 */
	640,	/* 0C:    .64 */
	960,	/* 0D:    .96 */
	1280,	/* 0E:   1.28 */
	1920,	/* 0F:   1.92 */
	2560,	/* 10:   2.56 */
	3840,	/* 11:   3.84 */
	5120,	/* 12:   5.12 */
	7680,	/* 13:   7.68 */
	10240,	/* 14:  10.24 */
	15360,	/* 15:  15.36 */
	20480,	/* 16:  20.48 */
	30720,	/* 17:  30.72 */
	40960,	/* 18:  40.96 */
	61440,	/* 19:  61.44 */
	81920,	/* 1A:  81.92 */
	122880,	/* 1B: 122.88 */
	163840,	/* 1C: 163.84 */
	245760,	/* 1D: 245.76 */
	327680,	/* 1E: 327.68 */
	491520	/* 1F: 491.52 */
};

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
		if (!rvt_lkey_ok(rkt, pd, j ? &ss->sg_list[j - 1] : &ss->sge,
				  &wqe->sg_list[i], IB_ACCESS_LOCAL_WRITE))
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

static __be64 get_sguid(struct hfi2_ibport *ibp, unsigned index)
{
	if (!index) {
		struct hfi_pportdata *ppd = ibp->ppd;

		return cpu_to_be64(ppd->pguid);
	}
	return ibp->guids[index - 1];
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
int hfi2_ruc_check_hdr(struct hfi2_ibport *ibp, struct hfi2_ib_packet *packet,
			struct rvt_qp *qp)
{
	struct hfi2_qp_priv *priv = qp->priv;
	__be64 guid;
	unsigned long flags;
	struct hfi_pportdata *ppd = ibp->ppd;
	u8 sc5 = ppd->sl_to_sc[qp->remote_ah_attr.sl];
	struct hfi2_ib_header *hdr = NULL;
	struct hfi2_opa16b_header *hdr_16b = NULL;
	struct ib_l4_headers *ohdr = packet->ohdr;
	u32 dlid, slid, sl, opa_dlid;
	int migrated;
	u32 bth0, bth1;

	bth0 = be32_to_cpu(ohdr->bth[0]);
	bth1 = be32_to_cpu(ohdr->bth[1]);
	if (!(packet->etype == RHF_RCV_TYPE_BYPASS)) {
		hdr = (struct hfi2_ib_header *)packet->hdr;
		dlid = OPA_9B_GET_DLID(hdr);
		slid = OPA_9B_GET_SLID(hdr);
		sl = OPA_9B_GET_SL(hdr);
		migrated = bth1 & IB_BTH_MIG_REQ;
	} else {
		hdr_16b = (struct hfi2_opa16b_header *)packet->hdr;
		dlid = OPA_16B_GET_DLID(hdr_16b);
		slid = OPA_16B_GET_SLID(hdr_16b);
		sl = 0; /* 16B has no sl but hfi1_bad_pkqkey needs it */
		migrated = bth0 & IB_16B_BTH_MIG_REQ;
	}
	if (qp->s_mig_state == IB_MIG_ARMED && (migrated)) {
		if (!packet->grh) {
			if ((qp->alt_ah_attr.ah_flags & IB_AH_GRH) &&
			     !priv->use_16b)
				goto err;
		} else {
			if (!(qp->alt_ah_attr.ah_flags & IB_AH_GRH))
				goto err;
			guid = get_sguid(ibp, qp->alt_ah_attr.grh.sgid_index);
			if (!gid_ok(&packet->grh->dgid,
				    ibp->rvp.gid_prefix, guid))
				goto err;
			if (!gid_ok(
				&packet->grh->sgid,
				qp->alt_ah_attr.grh.dgid.global.subnet_prefix,
				qp->alt_ah_attr.grh.dgid.global.interface_id))
				goto err;
		}
		if (unlikely(hfi_rcv_pkey_check(ibp->ppd, (u16)bth0,
					    sc5, slid))) {
			hfi2_bad_pqkey(ibp, OPA_TRAP_BAD_P_KEY, (u16)bth0, sl,
				       0, qp->ibqp.qp_num, slid, dlid);
			goto err;
		}
		/* Validate the SLID. See Ch. 9.6.1.5 and 17.2.8 */
		opa_dlid = hfi2_get_dlid_from_ah(&qp->alt_ah_attr);
		if ((slid != opa_dlid) ||
		     ibp->ppd->pnum != qp->alt_ah_attr.port_num)
			goto err;
		spin_lock_irqsave(&qp->s_lock, flags);
		hfi2_migrate_qp(qp);
		spin_unlock_irqrestore(&qp->s_lock, flags);
	} else {
		if (!packet->grh) {
			if ((qp->remote_ah_attr.ah_flags & IB_AH_GRH) &&
			     !priv->use_16b)
				goto err;
		} else {
			if (!(qp->remote_ah_attr.ah_flags & IB_AH_GRH))
				goto err;
			guid = get_sguid(ibp,
					 qp->remote_ah_attr.grh.sgid_index);
			if (!gid_ok(&packet->grh->dgid,
				    ibp->rvp.gid_prefix, guid))
				goto err;
			 if (!gid_ok(
			     &packet->grh->sgid,
			     qp->remote_ah_attr.grh.dgid.global.subnet_prefix,
			     qp->remote_ah_attr.grh.dgid.global.interface_id))
				goto err;
		}
		if (unlikely(hfi_rcv_pkey_check(ibp->ppd, (u16)bth0,
					    sc5, slid))) {
			hfi2_bad_pqkey(ibp, OPA_TRAP_BAD_P_KEY, (u16)bth0, sl,
				       0, qp->ibqp.qp_num, slid, dlid);
			goto err;
		}
		/* Validate the SLID. See Ch. 9.6.1.5 */
		opa_dlid = hfi2_get_dlid_from_ah(&qp->remote_ah_attr);
		if ((slid != opa_dlid) ||
		    ibp->ppd->pnum != qp->port_num)
			goto err;
		 if (qp->s_mig_state == IB_MIG_REARM &&
		    !migrated)
		      qp->s_mig_state = IB_MIG_ARMED;
	}

	return 0;

err:
	return 1;
}

/*
 * Switch to alternate path.
 * The QP s_lock should be held and interrupts disabled.
 */
void hfi2_migrate_qp(struct rvt_qp *qp)
{
	struct ib_event ev;

	qp->s_mig_state = IB_MIG_MIGRATED;
	qp->remote_ah_attr = qp->alt_ah_attr;
	qp->port_num = qp->alt_ah_attr.port_num;
	qp->s_pkey_index = qp->s_alt_pkey_index;

	ev.device = qp->ibqp.device;
	ev.element.qp = &qp->ibqp;
	ev.event = IB_EVENT_PATH_MIG;
	qp->ibqp.event_handler(&ev, qp->ibqp.qp_context);
}

void hfi2_make_ruc_header(struct rvt_qp *qp, struct ib_l4_headers *ohdr,
			  u32 bth0, u32 bth2)
{
	struct hfi2_qp_priv *qp_priv = qp->priv;
	union hfi2_ib_dma_header *s_hdr = qp_priv->s_hdr;
	struct hfi2_ibport *ibp;
	struct hfi_pportdata *ppd;
	u16 lrh0;
	u32 nwords;
	u32 extra_bytes;
	u32 bth1;

	/* Construct the header. */
	ibp = to_hfi_ibp(qp->ibqp.device, qp->port_num);
	ppd = ibp->ppd;
	extra_bytes = -qp->s_cur_size & 3;
	nwords = ((qp->s_cur_size + extra_bytes) >> 2) + SIZE_OF_CRC;
	if (unlikely(qp->remote_ah_attr.ah_flags & IB_AH_GRH)) {
		/* remove LRH size from s_hdrwords for GRH */
		qp->s_hdrwords += hfi2_make_grh(ibp, &s_hdr->ph.ibh.u.l.grh,
						&qp->remote_ah_attr.grh,
						(qp->s_hdrwords - 2),
						nwords);
		lrh0 = HFI1_LRH_GRH;
	} else {
		lrh0 = HFI1_LRH_BTH;
	}

	qp_priv->s_sl = qp->remote_ah_attr.sl;
	qp_priv->s_sc = ppd->sl_to_sc[qp_priv->s_sl];
	lrh0 |= (qp_priv->s_sc & 0xf) << 12 | (qp_priv->s_sl & 0xf) << 4;
	if (qp->s_mig_state == IB_MIG_MIGRATED)
		bth0 |= IB_BTH_MIG_REQ;
	s_hdr->ph.ibh.lrh[0] = cpu_to_be16(lrh0);
	s_hdr->ph.ibh.lrh[1] = cpu_to_be16((u16)qp->remote_ah_attr.dlid);
	s_hdr->ph.ibh.lrh[2] =
		cpu_to_be16(qp->s_hdrwords + nwords);
	s_hdr->ph.ibh.lrh[3] = cpu_to_be16(ppd->lid |
				       qp->remote_ah_attr.src_path_bits);
	bth0 |= hfi2_get_pkey(ibp, qp->s_pkey_index);
	bth0 |= extra_bytes << 20;
	ohdr->bth[0] = cpu_to_be32(bth0);
	bth1 = qp->remote_qpn;
	if (qp->s_flags & RVT_S_ECN) {
		qp->s_flags &= ~RVT_S_ECN;
		/* we recently received a FECN, so return a BECN */
		bth1 |= (HFI1_BECN_MASK << HFI1_BECN_SHIFT);
	}
	ohdr->bth[1] = cpu_to_be32(bth1);
	ohdr->bth[2] = cpu_to_be32(bth2);
}

void hfi2_make_16b_ruc_header(struct rvt_qp *qp, struct ib_l4_headers *ohdr,
			      u32 bth0, u32 bth2)
{
	struct hfi2_qp_priv *qp_priv = qp->priv;
	struct hfi2_opa16b_header *opa16b = &qp_priv->s_hdr->opa16b;
	struct hfi2_ibport *ibp;
	struct hfi_pportdata *ppd;
	u32 nwords, extra_bytes;
	u32 bth1, qwords, slid, dlid;
	bool becn = false;
	u8 l4;
	u16 pkey;

	/* Construct the header. */
	ibp = to_hfi_ibp(qp->ibqp.device, qp->port_num);
	ppd = ibp->ppd;

	/* Whole packet (hdr, data, crc, tail byte) should be flit aligned */
	extra_bytes = -(qp->s_cur_size + (qp->s_hdrwords << 2) +
			(SIZE_OF_CRC << 2) + 1) & 7;
	nwords = (qp->s_cur_size + (SIZE_OF_CRC << 2) + 1 + extra_bytes) >> 2;

	/*
	 * FXRTODO: Later, we need to make grh for 16B packets
	 * going across the network based on hop_count.
	 */
	if (unlikely(qp->remote_ah_attr.ah_flags & IB_AH_GRH & 0)) {
		/* remove 16B HDR size from s_hdrwords for GRH */
		qp->s_hdrwords += hfi2_make_grh(ibp, &opa16b->u.l.grh,
						&qp->remote_ah_attr.grh,
						(qp->s_hdrwords - 4), nwords);
		l4 = HFI1_L4_IB_GLOBAL;
	} else {
		l4 = HFI1_L4_IB_LOCAL;
	}

	qp_priv->s_sl = qp->remote_ah_attr.sl;
	qp_priv->s_sc = ppd->sl_to_sc[qp_priv->s_sl];

	bth0 |= extra_bytes << 20;
	bth1 = qp->remote_qpn;
	if (qp->s_mig_state == IB_MIG_MIGRATED)
		bth1 |= IB_16B_BTH_MIG_REQ;
	if (qp->s_flags & RVT_S_ECN) {
		qp->s_flags &= ~RVT_S_ECN;
		/* we recently received a FECN, so return a BECN */
		becn = true;
	}

	dlid = hfi2_retrieve_dlid(qp);
	slid = ppd->lid | qp->remote_ah_attr.src_path_bits;
	pkey = hfi2_get_pkey(ibp, qp->s_pkey_index);
	qwords = (qp->s_hdrwords + nwords) >> 1;

	opa_make_16b_header((u32 *)opa16b, slid, dlid, qwords,
			    pkey, 0, qp_priv->s_sc, 0, 0, becn, 0, l4);

	ohdr->bth[0] = cpu_to_be32(bth0);
	ohdr->bth[1] = cpu_to_be32(bth1);
	ohdr->bth[2] = cpu_to_be32(bth2);
}

/**
 * send_wqe() - submit a WQE to the hardware
 * @ibp: outgoing port
 * @qp: the QP of the WQE to send
 * TODO - delete below when STL-2554 implemented
 * @wait: wait structure to use when no slots (may be NULL)
 *
 * If a iowait structure is non-NULL the packet will be queued to the list
 * in wait.
 * TODO - see WFR's sdma_send_txreq() as reference.
 *
 * Return: TBD
 */
static int send_wqe(struct hfi2_ibport *ibp, struct rvt_qp *qp)
{
	int ret;

	ret = ibp->ibd->send_wqe(ibp, qp->priv);
	if (ret < 0 && qp->s_wqe)
		hfi2_send_complete(qp, qp->s_wqe, IB_WC_FATAL_ERR);
	/* else send_complete issued upon DMA completion event */

	return ret;
}

/**
 * hfi2_verbs_send - send a packet
 * @qp: the QP to send on
 *
 * Return: zero if packet is sent or queued OK, non-zero
 * and clear qp->s_flags RVT_S_BUSY otherwise.
 */
static int hfi2_verbs_send(struct rvt_qp *qp)
{
	struct hfi2_ibport *ibp;
	int ret = 0;
	unsigned long flags = 0;

	ibp = to_hfi_ibp(qp->ibqp.device, qp->port_num);

	/* ret = egress_pkey_check(ibp, &hdr->ph.ibh, qp); */
	if (unlikely(ret)) {
		/*
		 * hfi1_cdbg(PIO, "%s() Failed. Completing with err",
		 * __func__);
		 */
		spin_lock_irqsave(&qp->s_lock, flags);
		hfi2_send_complete(qp, qp->s_wqe, IB_WC_GENERAL_ERR);
		spin_unlock_irqrestore(&qp->s_lock, flags);
		return -EINVAL;
	}

	/*
	 * Send from TX overflow list first.
	 * TODO - not fully integrated, we should reuse if we can. WFR
	 * enqueues to this list in sdma_check_progress via sleep callback.
	 */
#if 0
	if (!list_empty(&qp->s_iowait.tx_head)) {
		/* TODO - STL-2554 */
		struct rvt_swqe *wqe;

		wqe = list_first_entry(
			&qp->s_iowait.tx_head,
			struct rvt_swqe,
			pending_list);
		list_del_init(&wqe->pending_list);
		/* send queued pending WGE into fabric */
		ret = send_wqe(ibp, qp);
		/*
		 * TODO - why does WFR just return without enqueuing the
		 * WQE that would have been sent below (qp->s_wqe).
		 */
		ret = -EINVAL;
		return ret;
	}
#endif

	/* send WGE into fabric */
	return send_wqe(ibp, qp);
}

/**
 * hfi2_do_send - perform a send on a QP
 * @work: contains a pointer to the QP
 *
 * Process entries in the send work queue until credit or queue is
 * exhausted.  Only allow one CPU to send a packet per QP (tasklet).
 * Otherwise, two threads could send packets out of order.
 */
void hfi2_do_send(struct work_struct *work)
{
	struct iowait *wait = container_of(work, struct iowait, iowork);
	struct hfi2_qp_priv *qp_priv;
	struct rvt_qp *qp;
	unsigned long flags;
	int (*make_req)(struct rvt_qp *qp);

	qp_priv = container_of(wait, struct hfi2_qp_priv, s_iowait);
	qp = qp_priv->owner;

	/* FXRTODO - WFR performs RC/UC loopback here */

	if (qp->ibqp.qp_type == IB_QPT_RC)
		make_req = hfi2_make_rc_req;
	else if (qp->ibqp.qp_type == IB_QPT_UC)
		make_req = hfi2_make_uc_req;
	else
		make_req = hfi2_make_ud_req;

	spin_lock_irqsave(&qp->s_lock, flags);
	/* Return if we are already busy processing a work request. */
	if (!send_ok(qp)) {
		spin_unlock_irqrestore(&qp->s_lock, flags);
		return;
	}
	qp->s_flags |= RVT_S_BUSY;
	spin_unlock_irqrestore(&qp->s_lock, flags);

	do {
		/* Check for a constructed packet to be sent. */
		if (qp->s_hdrwords != 0) {
			/*
			 * If the packet cannot be sent now, return and
			 * the send tasklet will be woken up later.
			 */
			if (hfi2_verbs_send(qp))
				break;

			/* Record that s_hdr is empty. */
			qp->s_hdrwords = 0;
		}
	} while (make_req(qp));
}

/*
 * This should be called with s_lock held.
 */
void hfi2_send_complete(struct rvt_qp *qp, struct rvt_swqe *wqe,
			enum ib_wc_status status)
{
	u32 old_last, last;
	unsigned i;

	if (!(ib_rvt_state_ops[qp->state] & RVT_PROCESS_OR_FLUSH_SEND))
		return;

	for (i = 0; i < wqe->wr.num_sge; i++) {
		struct rvt_sge *sge = &wqe->sg_list[i];

		rvt_put_mr(sge->mr);
	}
	if (qp->ibqp.qp_type == IB_QPT_UD ||
	    qp->ibqp.qp_type == IB_QPT_SMI ||
	    qp->ibqp.qp_type == IB_QPT_GSI)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)
		atomic_dec(&ibah_to_rvtah(wqe->ud_wr.ah)->refcount);
#else
		atomic_dec(&ibah_to_rvtah(wqe->wr.wr.ud.ah)->refcount);
#endif

	/* See ch. 11.2.4.1 and 10.7.3.1 */
	if (!(qp->s_flags & RVT_S_SIGNAL_REQ_WR) ||
	    (wqe->wr.send_flags & IB_SEND_SIGNALED) ||
	    status != IB_WC_SUCCESS) {
		struct ib_wc wc;

		memset(&wc, 0, sizeof(wc));
		wc.wr_id = wqe->wr.wr_id;
		wc.status = status;
		wc.opcode = ib_hfi1_wc_opcode[wqe->wr.opcode];
		wc.qp = &qp->ibqp;
		if (status == IB_WC_SUCCESS)
			wc.byte_len = wqe->length;
		rvt_cq_enter(ibcq_to_rvtcq(qp->ibqp.send_cq), &wc,
				status != IB_WC_SUCCESS);
	}

	last = qp->s_last;
	old_last = last;
	if (++last >= qp->s_size)
		last = 0;
	qp->s_last = last;
	if (qp->s_acked == old_last)
		qp->s_acked = last;
	if (qp->s_cur == old_last)
		qp->s_cur = last;
	if (qp->s_tail == old_last)
		qp->s_tail = last;
	if (qp->state == IB_QPS_SQD && last == qp->s_cur)
		qp->s_draining = 0;
}

/*
 * This must be called with s_lock held.
 */
void hfi2_schedule_send(struct rvt_qp *qp)
{
	if (send_ok(qp)) {
		struct hfi2_qp_priv *qp_priv = qp->priv;
		struct hfi2_ibport *ibp;

		ibp = to_hfi_ibp(qp->ibqp.device, qp->port_num);
		iowait_schedule(&qp_priv->s_iowait, ibp->ppd->hfi_wq);
	}
}
