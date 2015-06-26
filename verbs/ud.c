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
 *
 * Intel(R) OPA Gen2 IB Driver
 */

#include <rdma/ib_smi.h>
#include "verbs.h"
#include "packet.h"

/**
 * ud_loopback - handle send on loopback QPs
 * @sqp: the sending QP
 * @swqe: the send work request
 *
 * This is called from opa_ib_make_ud_req() to forward a WQE addressed
 * to the same HCA.
 * Note that the receive interrupt handler may be calling opa_ib_ud_rcv()
 * while this is being called.
 */
static void ud_loopback(struct opa_ib_qp *sqp, struct opa_ib_swqe *swqe)
{
	struct ib_device *ibdev;
	struct opa_ib_portdata *ibp;
	struct opa_ib_qp *qp;
	struct ib_ah_attr *ah_attr;
	unsigned long flags;
	struct opa_ib_sge_state ssge;
	struct ib_sge *sge;
	struct ib_wc wc;
	u32 length;
	enum ib_qp_type sqptype, dqptype;

	ibdev = sqp->ibqp.device;
	ibp = to_opa_ibportdata(ibdev, sqp->port_num);

	qp = opa_ib_lookup_qpn(ibp, swqe->wr.wr.ud.remote_qpn);
	if (!qp) {
		ibp->n_pkt_drops++;
		return;
	}

	dev_info(&ibdev->dev,
		 "exercising UD loopback path, to/from lid %d\n", ibp->lid);

	sqptype = sqp->ibqp.qp_type == IB_QPT_GSI ?
			IB_QPT_UD : sqp->ibqp.qp_type;
	dqptype = qp->ibqp.qp_type == IB_QPT_GSI ?
			IB_QPT_UD : qp->ibqp.qp_type;

	if (dqptype != sqptype ||
	    !(ib_qp_state_ops[qp->state] & HFI1_PROCESS_RECV_OK)) {
		ibp->n_pkt_drops++;
		goto drop;
	}

	ah_attr = &to_opa_ibah(swqe->wr.wr.ud.ah)->attr;

#if 0 /* FXRTODO */
	if (qp->ibqp.qp_num > 1) {
		u16 pkey;
		u16 slid;
		u8 sc5 = ibp->sl_to_sc[ah_attr->sl];

		pkey = opa_ib_get_pkey(ibp, sqp->s_pkey_index);
		slid = ibp->lid | (ah_attr->src_path_bits &
				   ((1 << ibp->lmc) - 1));
		if (unlikely(ingress_pkey_check(ibp, pkey, sc5,
						qp->s_pkey_index, slid))) {
			opa_ib_bad_pqkey(ibp, IB_NOTICE_TRAP_BAD_PKEY, pkey,
					 ah_attr->sl,
					 sqp->ibqp.qp_num, qp->ibqp.qp_num,
					 cpu_to_be16(slid),
					 cpu_to_be16(ah_attr->dlid));
			goto drop;
		}
	}

	/*
	 * Check that the qkey matches (except for QP0, see 9.6.1.4.1).
	 * Qkeys with the high order bit set mean use the
	 * qkey from the QP context instead of the WR (see 10.2.5).
	 */
	if (qp->ibqp.qp_num) {
		u32 qkey;

		qkey = (int)swqe->wr.wr.ud.remote_qkey < 0 ?
			sqp->qkey : swqe->wr.wr.ud.remote_qkey;
		if (unlikely(qkey != qp->qkey)) {
			u16 lid;

			lid = ibp->lid | (ah_attr->src_path_bits &
					  ((1 << ibp->lmc) - 1));
			opa_ib_bad_pqkey(ibp, IB_NOTICE_TRAP_BAD_QKEY, qkey,
					 ah_attr->sl,
					 sqp->ibqp.qp_num, qp->ibqp.qp_num,
					 cpu_to_be16(lid),
					 cpu_to_be16(ah_attr->dlid));
			goto drop;
		}
	}
#endif

	/*
	 * A GRH is expected to precede the data even if not
	 * present on the wire.
	 */
	length = swqe->length;
	memset(&wc, 0, sizeof(wc));
	wc.byte_len = length + sizeof(struct ib_grh);

	if (swqe->wr.opcode == IB_WR_SEND_WITH_IMM) {
		wc.wc_flags = IB_WC_WITH_IMM;
		wc.ex.imm_data = swqe->wr.ex.imm_data;
	}

	spin_lock_irqsave(&qp->r_lock, flags);

	/*
	 * Get the next work request entry to find where to put the data.
	 */
	if (qp->r_flags & HFI1_R_REUSE_SGE)
		qp->r_flags &= ~HFI1_R_REUSE_SGE;
	else {
		int ret;

		ret = opa_ib_get_rwqe(qp, 0);
		if (ret < 0) {
			opa_ib_rc_error(qp, IB_WC_LOC_QP_OP_ERR);
			goto bail_unlock;
		}
		if (!ret) {
			if (qp->ibqp.qp_num == 0)
				ibp->n_vl15_dropped++;
			goto bail_unlock;
		}
	}
	/* Silently drop packets which are too big. */
	if (unlikely(wc.byte_len > qp->r_len)) {
		qp->r_flags |= HFI1_R_REUSE_SGE;
		ibp->n_pkt_drops++;
		goto bail_unlock;
	}

	if (ah_attr->ah_flags & IB_AH_GRH) {
		opa_ib_copy_sge(&qp->r_sge, &ah_attr->grh,
				sizeof(struct ib_grh), 1);
		wc.wc_flags |= IB_WC_GRH;
	} else
		opa_ib_skip_sge(&qp->r_sge, sizeof(struct ib_grh), 1);
	ssge.sg_list = swqe->sg_list + 1;
	ssge.sge = *swqe->sg_list;
	ssge.num_sge = swqe->wr.num_sge;
	sge = &ssge.sge;
	while (length) {
		u32 len = sge->length;

		if (len > length)
			len = length;
		BUG_ON(len == 0);
		opa_ib_copy_sge(&qp->r_sge, (void *)sge->addr, len, 1);
		sge->addr += len;
		sge->length -= len;
		if ((sge->length == 0) && (--ssge.num_sge))
			*sge = *ssge.sg_list++;
		/* FXRTODO - WFR has multi-segment stuff here */
		length -= len;
	}
	opa_ib_put_ss(&qp->r_sge);
	if (!test_and_clear_bit(HFI1_R_WRID_VALID, &qp->r_aflags))
		goto bail_unlock;
	wc.wr_id = qp->r_wr_id;
	wc.status = IB_WC_SUCCESS;
	wc.opcode = IB_WC_RECV;
	wc.qp = &qp->ibqp;
	wc.src_qp = sqp->ibqp.qp_num;
	if (qp->ibqp.qp_type == IB_QPT_GSI || qp->ibqp.qp_type == IB_QPT_SMI) {
		if (sqp->ibqp.qp_type == IB_QPT_GSI ||
		    sqp->ibqp.qp_type == IB_QPT_SMI)
			wc.pkey_index = swqe->wr.wr.ud.pkey_index;
		else
			wc.pkey_index = sqp->s_pkey_index;
	} else {
		wc.pkey_index = 0;
	}
	wc.slid = ibp->lid | (ah_attr->src_path_bits & ((1 << ibp->lmc) - 1));
	/* Check for loopback when the port lid is not set */
	if (wc.slid == 0 && sqp->ibqp.qp_type == IB_QPT_GSI)
		wc.slid = HFI1_PERMISSIVE_LID;
	wc.sl = ah_attr->sl;
	wc.dlid_path_bits = ah_attr->dlid & ((1 << ibp->lmc) - 1);
	wc.port_num = qp->port_num;
	/* Signal completion event if the solicited bit is set. */
	opa_ib_cq_enter(to_opa_ibcq(qp->ibqp.recv_cq), &wc,
			swqe->wr.send_flags & IB_SEND_SOLICITED);
	ibp->n_loop_pkts++;
bail_unlock:
	spin_unlock_irqrestore(&qp->r_lock, flags);
drop:
	if (atomic_dec_and_test(&qp->refcount))
		wake_up(&qp->wait);
}

/**
 * opa_ib_make_ud_req - construct a UD request packet
 * @qp: the QP
 *
 * Return: 1 if constructed; otherwise 0.
 */
int opa_ib_make_ud_req(struct opa_ib_qp *qp)
{
	struct ib_l4_headers *ohdr;
	struct ib_ah_attr *ah_attr;
	struct opa_ib_portdata *ibp;
	struct opa_ib_swqe *wqe;
	unsigned long flags;
	u32 nwords;
	u32 extra_bytes;
	u32 bth0;
	u16 lrh0;
	u16 lid;
	int ret = 0;
	int next_cur;
	u8 sc5;

	spin_lock_irqsave(&qp->s_lock, flags);

	if (!(ib_qp_state_ops[qp->state] & HFI1_PROCESS_NEXT_SEND_OK)) {
		if (!(ib_qp_state_ops[qp->state] & HFI1_FLUSH_SEND))
			goto bail;
		/* We are in the error state, flush the work request. */
		if (qp->s_last == qp->s_head)
			goto bail;
		/* If DMAs are in progress, we can't flush immediately. */
		if (is_iowait_sdma_busy(&qp->s_iowait)) {
			qp->s_flags |= HFI1_S_WAIT_DMA;
			goto bail;
		}
		wqe = get_swqe_ptr(qp, qp->s_last);
		opa_ib_send_complete(qp, wqe, IB_WC_WR_FLUSH_ERR);
		goto done;
	}

	if (qp->s_cur == qp->s_head)
		goto bail;

	wqe = get_swqe_ptr(qp, qp->s_cur);
	next_cur = qp->s_cur + 1;
	if (next_cur >= qp->s_size)
		next_cur = 0;

	/* Construct the header. */
	ibp = to_opa_ibportdata(qp->ibqp.device, qp->port_num);
	/* TODO - review wr.wr.ud.ah */
	ah_attr = &to_opa_ibah(wqe->wr.wr.ud.ah)->attr;
	if (ah_attr->dlid < HFI1_MULTICAST_LID_BASE ||
	    ah_attr->dlid == HFI1_PERMISSIVE_LID) {
		lid = ah_attr->dlid & ~((1 << ibp->lmc) - 1);
		/*
		 * FXRTODO - WFR had additional test to skip when in link
		 * loopback, maybe to ensure datapath tested in that mode
		 */
		if (unlikely(lid == ibp->lid ||
		    (lid == HFI1_PERMISSIVE_LID &&
		     qp->ibqp.qp_type == IB_QPT_GSI))) {
			/*
			 * If DMAs are in progress, we can't generate
			 * a completion for the loopback packet since
			 * it would be out of order.
			 * Instead of waiting, we could queue a
			 * zero length descriptor so we get a callback.
			 */
			if (is_iowait_sdma_busy(&qp->s_iowait)) {
				qp->s_flags |= HFI1_S_WAIT_DMA;
				goto bail;
			}
			qp->s_cur = next_cur;
			spin_unlock_irqrestore(&qp->s_lock, flags);
			ud_loopback(qp, wqe);
			spin_lock_irqsave(&qp->s_lock, flags);
			opa_ib_send_complete(qp, wqe, IB_WC_SUCCESS);
			goto done;
		}
	}

#if 1
	/*
	 * FXRTODO - for now, have non-loopback return FATAL
	 * error to sender.
	 */
	opa_ib_send_complete(qp, wqe, IB_WC_FATAL_ERR);
	goto done;
#endif

	qp->s_cur = next_cur;
	extra_bytes = -wqe->length & 3;
	nwords = (wqe->length + extra_bytes) >> 2;

	/* header size in 32-bit words LRH+BTH+DETH = (8+12+8)/4. */
	qp->s_hdrwords = 7;
	qp->s_cur_size = wqe->length;
	qp->s_cur_sge = &qp->s_sge;
	qp->s_srate = ah_attr->static_rate;
	qp->srate_mbps = ib_rate_to_mbps(qp->s_srate);
	qp->s_wqe = wqe;
	qp->s_sge.sge = wqe->sg_list[0];
	qp->s_sge.sg_list = wqe->sg_list + 1;
	qp->s_sge.num_sge = wqe->wr.num_sge;
	qp->s_sge.total_len = wqe->length;

	if (ah_attr->ah_flags & IB_AH_GRH) {
		/* Header size in 32-bit words. */
		qp->s_hdrwords += opa_ib_make_grh(ibp, &qp->s_hdr->ibh.u.l.grh,
						  &ah_attr->grh,
						  qp->s_hdrwords, nwords);
		lrh0 = HFI1_LRH_GRH;
		ohdr = &qp->s_hdr->ibh.u.l.oth;
		/*
		 * Don't worry about sending to locally attached multicast
		 * QPs.  It is unspecified by the spec. what happens.
		 */
	} else {
		/* Header size in 32-bit words. */
		lrh0 = HFI1_LRH_BTH;
		ohdr = &qp->s_hdr->ibh.u.oth;
	}
	if (wqe->wr.opcode == IB_WR_SEND_WITH_IMM) {
		qp->s_hdrwords++;
		ohdr->u.ud.imm_data = wqe->wr.ex.imm_data;
		bth0 = IB_OPCODE_UD_SEND_ONLY_WITH_IMMEDIATE << 24;
	} else
		bth0 = IB_OPCODE_UD_SEND_ONLY << 24;
	sc5 = ibp->sl_to_sc[ah_attr->sl];
	lrh0 |= (ah_attr->sl & 0xf) << 4;
	if (qp->ibqp.qp_type == IB_QPT_SMI) {
		lrh0 |= 0xF000; /* Set VL (see ch. 13.5.3.1) */
		qp->s_sc = 0xf;
	} else {
		lrh0 |= (sc5 & 0xf) << 12;
		qp->s_sc = sc5;
	}
	qp->s_hdr->ibh.lrh[0] = cpu_to_be16(lrh0);
	qp->s_hdr->ibh.lrh[1] = cpu_to_be16(ah_attr->dlid);  /* DEST LID */
	qp->s_hdr->ibh.lrh[2] =
		cpu_to_be16(qp->s_hdrwords + nwords + SIZE_OF_CRC);
	if (ah_attr->dlid == IB_LID_PERMISSIVE)
		qp->s_hdr->ibh.lrh[3] = IB_LID_PERMISSIVE;
	else {
		lid = ibp->lid;
		if (lid) {
			lid |= ah_attr->src_path_bits & ((1 << ibp->lmc) - 1);
			qp->s_hdr->ibh.lrh[3] = cpu_to_be16(lid);
		} else
			qp->s_hdr->ibh.lrh[3] = IB_LID_PERMISSIVE;
	}
	if (wqe->wr.send_flags & IB_SEND_SOLICITED)
		bth0 |= IB_BTH_SOLICITED;
	bth0 |= extra_bytes << 20;
	if (qp->ibqp.qp_type == IB_QPT_GSI || qp->ibqp.qp_type == IB_QPT_SMI)
		bth0 |= opa_ib_get_pkey(ibp, wqe->wr.wr.ud.pkey_index);
	else
		bth0 |= opa_ib_get_pkey(ibp, qp->s_pkey_index);
	ohdr->bth[0] = cpu_to_be32(bth0);
	ohdr->bth[1] = cpu_to_be32(wqe->wr.wr.ud.remote_qpn);
	ohdr->bth[2] = cpu_to_be32(mask_psn(qp->s_next_psn++));
	/*
	 * Qkeys with the high order bit set mean use the
	 * qkey from the QP context instead of the WR (see 10.2.5).
	 */
	ohdr->u.ud.deth[0] = cpu_to_be32((int)wqe->wr.wr.ud.remote_qkey < 0 ?
					 qp->qkey : wqe->wr.wr.ud.remote_qkey);
	ohdr->u.ud.deth[1] = cpu_to_be32(qp->ibqp.qp_num);

done:
	ret = 1;
	goto unlock;

bail:
	qp->s_flags &= ~HFI1_S_BUSY;
unlock:
	spin_unlock_irqrestore(&qp->s_lock, flags);
	return ret;
}

/*
 * Hardware can't check this so we do it here.
 *
 * This is a slightly different algorithm than the standard pkey check.  It
 * special cases the management keys and allows for 0x7fff and 0xffff to be in
 * the table at the same time.
 *
 * @returns the index found or -1 if not found
 */
int opa_ib_lookup_pkey_idx(struct opa_ib_portdata *ibp, u16 pkey)
{
	unsigned i;

	if (pkey == OPA_FULL_MGMT_P_KEY || pkey == OPA_LIM_MGMT_P_KEY) {
		unsigned lim_idx = -1;

		for (i = 0; i < ARRAY_SIZE(ibp->pkeys); ++i) {
			/* here we look for an exact match */
			if (ibp->pkeys[i] == pkey)
				return i;
			if (ibp->pkeys[i] == OPA_LIM_MGMT_P_KEY)
				lim_idx = i;
		}

		/* did not find 0xffff return 0x7fff idx if found */
		if (pkey == OPA_FULL_MGMT_P_KEY)
			return lim_idx;

		/* no match...  */
		return -1;
	}

	pkey &= 0x7fff; /* remove limited/full membership bit */

	for (i = 0; i < ARRAY_SIZE(ibp->pkeys); ++i)
		if ((ibp->pkeys[i] & 0x7fff) == pkey)
			return i;

	/*
	 * Should not get here, this means hardware failed to validate pkeys.
	 */
	return -1;
}

/**
 * opa_ib_ud_rcv - receive an incoming UD packet
 * @ibp: the port the packet came in on
 * @hdr: the packet header
 * @rcv_flags: flags relevant to rcv processing
 * @data: the packet data
 * @tlen: the packet length
 * @qp: the QP the packet came on
 *
 * This is called from opa_ib_qp_rcv() to process an incoming UD packet
 * for the given QP.
 * Called at interrupt level.
 */
void opa_ib_ud_rcv(struct opa_ib_portdata *ibp, struct opa_ib_header *hdr,
		   u32 rcv_flags, void *data, u32 tlen, struct opa_ib_qp *qp)
{
	struct ib_l4_headers *ohdr;
	int opcode;
	u32 hdrsize;
	u32 pad;
	struct ib_wc wc;
	u32 qkey;
	u32 src_qp;
	u16 dlid, slid, pkey;
	int mgmt_pkey_idx = -1;
	int has_grh = !!(rcv_flags & HFI1_HAS_GRH);
	int sc4_bit = (!!(rcv_flags & HFI1_SC4_BIT)) << 4;
	u8 sc5;
	int is_becn, is_fecn, is_mcast;
	struct ib_grh *grh = NULL;

	/* Check for GRH */
	if (!has_grh) {
		ohdr = &hdr->u.oth;
		hdrsize = 8 + 12 + 8;   /* LRH + BTH + DETH */
	} else {
		ohdr = &hdr->u.l.oth;
		hdrsize = 8 + 40 + 12 + 8; /* LRH + GRH + BTH + DETH */
		grh = &hdr->u.l.grh;
	}
	qkey = be32_to_cpu(ohdr->u.ud.deth[0]);
	src_qp = be32_to_cpu(ohdr->u.ud.deth[1]) & HFI1_QPN_MASK;
	dlid = be16_to_cpu(hdr->lrh[1]);
	slid = be16_to_cpu(hdr->lrh[3]);
	sc5 = (be16_to_cpu(hdr->lrh[0]) >> 12) & 0xf;
	sc5 |= sc4_bit;
	pkey = (u16)be32_to_cpu(ohdr->bth[0]);
	is_mcast = (dlid > HFI1_MULTICAST_LID_BASE) &&
			(dlid != HFI1_PERMISSIVE_LID);
	is_becn = (be32_to_cpu(ohdr->bth[1]) >> HFI1_BECN_SHIFT)
		& HFI1_BECN_MASK;
	is_fecn = (be32_to_cpu(ohdr->bth[1]) >> HFI1_FECN_SHIFT)
		& HFI1_FECN_MASK;

	/*
	 * The opcode is in the low byte when its in network order
	 * (top byte when in host order).
	 */
	opcode = be32_to_cpu(ohdr->bth[0]) >> 24;
	opcode &= 0xff;

#if 0	/* FXRTODO - becn/fecn */
	if (is_becn) {
		/*
		 * In pre-B0 h/w the CNP_OPCODE is handled via an
		 * error path (errata 291394).
		 */
		u32 lqpn =  be32_to_cpu(ohdr->bth[1]) & HFI1_QPN_MASK;
		u8 sl;

		sl = ibp->sc_to_sl[sc5];
		process_becn(ibp, sl, 0, lqpn, 0, IB_CC_SVCTYPE_UD);
	}

	if (!is_mcast && (opcode != CNP_OPCODE) && is_fecn)
		return_cnp(ibp, qp, src_qp, pkey, dlid, slid, sc5, grh);
#endif

	/*
	 * Get the number of bytes the message was padded by
	 * and drop incomplete packets.
	 */
	pad = (be32_to_cpu(ohdr->bth[0]) >> 20) & 3;
	if (unlikely(tlen < (hdrsize + pad + 4)))
		goto drop;

	tlen -= hdrsize + pad + 4;

	/*
	 * Check that the permissive LID is only used on QP0
	 * and the QKEY matches (see 9.6.1.4.1 and 9.6.1.5.1).
	 */
	if (qp->ibqp.qp_num) {
		//u8 sl = (be16_to_cpu(hdr->lrh[0]) >> 4) & 0xF;
		if (unlikely(hdr->lrh[1] == IB_LID_PERMISSIVE ||
			     hdr->lrh[3] == IB_LID_PERMISSIVE))
			goto drop;
		if (qp->ibqp.qp_num > 1) {
#if 0 /* FXRTODO */
			if (unlikely(ingress_pkey_check(ibp, pkey, sc5,
							qp->s_pkey_index,
							slid))) {
				opa_ib_bad_pqkey(ibp, IB_NOTICE_TRAP_BAD_PKEY,
						 pkey, sl,
						 src_qp, qp->ibqp.qp_num,
						 hdr->lrh[3], hdr->lrh[1]);
				return;
			}
#endif
		} else {
			/* GSI packet */
			mgmt_pkey_idx = opa_ib_lookup_pkey_idx(ibp, pkey);
			if (mgmt_pkey_idx < 0)
				goto drop;

		}
#if 0 /* FXRTODO */
		if (unlikely(qkey != qp->qkey)) {
			opa_ib_bad_pqkey(ibp, IB_NOTICE_TRAP_BAD_QKEY,
					 qkey, sl,
					 src_qp, qp->ibqp.qp_num,
					 hdr->lrh[3], hdr->lrh[1]);
			return;
		}
#endif
		/* Drop invalid MAD packets (see 13.5.3.1). */
		if (unlikely(qp->ibqp.qp_num == 1 &&
			     (tlen > 2048 ||
			      (be16_to_cpu(hdr->lrh[0]) >> 12) == 15)))
			goto drop;
	} else {
		/* Received on QP0, and so by definition, this is an SMP */
#if 1 /* FXRTODO, revisit this */
		struct ib_smp *smp = (struct ib_smp *) data;
#else
		struct opa_smp *smp = (struct opa_smp *)data;

		if (opa_smp_check(ibp, pkey, sc5, qp, slid, smp))
			goto drop;
#endif

		if (tlen > 2048)
			goto drop;
		if ((hdr->lrh[1] == IB_LID_PERMISSIVE ||
		     hdr->lrh[3] == IB_LID_PERMISSIVE) &&
		    smp->mgmt_class != IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE)
			goto drop;

		/* lookup SMI pkey */
		mgmt_pkey_idx = opa_ib_lookup_pkey_idx(ibp, pkey);
		if (mgmt_pkey_idx < 0)
			goto drop;

	}

	if (qp->ibqp.qp_num > 1 &&
	    opcode == IB_OPCODE_UD_SEND_ONLY_WITH_IMMEDIATE) {
		wc.ex.imm_data = ohdr->u.ud.imm_data;
		wc.wc_flags = IB_WC_WITH_IMM;
		tlen -= sizeof(u32);
	} else if (opcode == IB_OPCODE_UD_SEND_ONLY) {
		wc.ex.imm_data = 0;
		wc.wc_flags = 0;
	} else
		goto drop;

	/*
	 * A GRH is expected to precede the data even if not
	 * present on the wire.
	 */
	wc.byte_len = tlen + sizeof(struct ib_grh);

	/*
	 * Get the next work request entry to find where to put the data.
	 */
	if (qp->r_flags & HFI1_R_REUSE_SGE)
		qp->r_flags &= ~HFI1_R_REUSE_SGE;
	else {
		int ret;

		ret = opa_ib_get_rwqe(qp, 0);
		if (ret < 0) {
			opa_ib_rc_error(qp, IB_WC_LOC_QP_OP_ERR);
			return;
		}
		if (!ret) {
			if (qp->ibqp.qp_num == 0)
				ibp->n_vl15_dropped++;
			return;
		}
	}
	/* Silently drop packets which are too big. */
	if (unlikely(wc.byte_len > qp->r_len)) {
		qp->r_flags |= HFI1_R_REUSE_SGE;
		goto drop;
	}
	if (has_grh) {
		opa_ib_copy_sge(&qp->r_sge, &hdr->u.l.grh,
			     sizeof(struct ib_grh), 1);
		wc.wc_flags |= IB_WC_GRH;
	} else
		opa_ib_skip_sge(&qp->r_sge, sizeof(struct ib_grh), 1);
	opa_ib_copy_sge(&qp->r_sge, data, wc.byte_len - sizeof(struct ib_grh), 1);
	opa_ib_put_ss(&qp->r_sge);
	if (!test_and_clear_bit(HFI1_R_WRID_VALID, &qp->r_aflags))
		return;
	wc.wr_id = qp->r_wr_id;
	wc.status = IB_WC_SUCCESS;
	wc.opcode = IB_WC_RECV;
	wc.vendor_err = 0;
	wc.qp = &qp->ibqp;
	wc.src_qp = src_qp;

	if (qp->ibqp.qp_type == IB_QPT_GSI ||
	    qp->ibqp.qp_type == IB_QPT_SMI) {
		if (mgmt_pkey_idx < 0) {
			/* FXRTODO - WFR prints err */
			mgmt_pkey_idx = 0;
		}
		wc.pkey_index = (unsigned)mgmt_pkey_idx;
	} else
		wc.pkey_index = 0;

	wc.slid = slid;
	/*
	 * TODO - BUG existed here with computing SC,
	 *        (verified with WFR team and fixed here)
	 * Delete this comment after inspecting offical WFR driver fix.
	 */
	wc.sl = ibp->sc_to_sl[sc5];

	/*
	 * Save the LMC lower bits if the destination LID is a unicast LID.
	 */
	wc.dlid_path_bits = dlid >= HFI1_MULTICAST_LID_BASE ? 0 :
		dlid & ((1 << ibp->lmc) - 1);
	wc.port_num = qp->port_num;
	/* Signal completion event if the solicited bit is set. */
	opa_ib_cq_enter(to_opa_ibcq(qp->ibqp.recv_cq), &wc,
			(ohdr->bth[0] & cpu_to_be32(IB_BTH_SOLICITED)) != 0);
	return;

drop:
	ibp->n_pkt_drops++;
}
