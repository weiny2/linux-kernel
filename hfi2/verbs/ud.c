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

#include <rdma/ib_smi.h>
#include <rdma/opa_core_ib.h>
#include "verbs.h"
#include "packet.h"

/**
 * ud_loopback - handle send on loopback QPs
 * @sqp: the sending QP
 * @swqe: the send work request
 *
 * This is called from hfi2_make_ud_req() to forward a WQE addressed
 * to the same HFI.
 * Note that the receive logic may be calling hfi2_ud_rcv() while
 * this is being called.
 */
/* FXRTODO: Need to support 16B loopback */
static void ud_loopback(struct hfi2_qp *sqp, struct hfi2_swqe *swqe)
{
	struct ib_device *ibdev;
	struct hfi2_ibport *ibp;
	struct hfi_pportdata *ppd;
	struct hfi2_qp *qp;
	struct ib_ah_attr *ah_attr;
	unsigned long flags;
	struct hfi2_sge_state ssge;
	struct hfi2_sge *sge;
	struct ib_wc wc;
	u32 length;
	enum ib_qp_type sqptype, dqptype;

	ibdev = sqp->ibqp.device;
	ibp = to_hfi_ibp(ibdev, sqp->port_num);
	ppd = ibp->ppd;

	rcu_read_lock();

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)
	qp = hfi2_lookup_qpn(ibp, swqe->ud_wr.remote_qpn);
#else
	qp = hfi2_lookup_qpn(ibp, swqe->wr.wr.ud.remote_qpn);
#endif
	if (!qp) {
		ibp->n_pkt_drops++;
		rcu_read_unlock();
		return;
	}

	dev_dbg(ibp->dev,
		"exercising UD loopback path, to/from lid %d\n", ppd->lid);

	sqptype = sqp->ibqp.qp_type == IB_QPT_GSI ?
			IB_QPT_UD : sqp->ibqp.qp_type;
	dqptype = qp->ibqp.qp_type == IB_QPT_GSI ?
			IB_QPT_UD : qp->ibqp.qp_type;

	if (dqptype != sqptype ||
	    !(ib_qp_state_ops[qp->state] & HFI1_PROCESS_RECV_OK)) {
		ibp->n_pkt_drops++;
		goto drop;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)
	ah_attr = &to_hfi_ah(swqe->ud_wr.ah)->attr;
#else
	ah_attr = &to_hfi_ah(swqe->wr.wr.ud.ah)->attr;
#endif

#if 0 /* FXRTODO */
	if (qp->ibqp.qp_num > 1) {
		u16 pkey;
		u32 slid;
		u8 sc5 = ibp->sl_to_sc[ah_attr->sl];

		pkey = hfi2_get_pkey(ibp, sqp->s_pkey_index);
		slid = ppd->lid | (ah_attr->src_path_bits &
				   ((1 << ibp->lmc) - 1));
		if (unlikely(ingress_pkey_check(ibp, pkey, sc5,
						qp->s_pkey_index, slid))) {
			hfi2_bad_pqkey(ibp, IB_NOTICE_TRAP_BAD_PKEY, pkey,
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
			u32 lid;

			lid = ppd->lid | (ah_attr->src_path_bits &
					  ((1 << ibp->lmc) - 1));
			hfi2_bad_pqkey(ibp, IB_NOTICE_TRAP_BAD_QKEY, qkey,
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

		ret = hfi2_get_rwqe(qp, 0);
		if (ret < 0) {
			hfi2_rc_error(qp, IB_WC_LOC_QP_OP_ERR);
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
		struct ib_grh grh;

		hfi2_make_grh(ibp, &grh, &ah_attr->grh, 0, 0);
		hfi2_copy_sge(&qp->r_sge, &grh, sizeof(grh), 1);
		wc.wc_flags |= IB_WC_GRH;
	} else
		hfi2_skip_sge(&qp->r_sge, sizeof(struct ib_grh), 1);
	ssge.sg_list = swqe->sg_list + 1;
	ssge.sge = *swqe->sg_list;
	ssge.num_sge = swqe->wr.num_sge;
	sge = &ssge.sge;
	while (length) {
		u32 len = sge->length;

		if (len > length)
			len = length;
		if (len > sge->sge_length)
			len = sge->sge_length;
		BUG_ON(len == 0);
		hfi2_copy_sge(&qp->r_sge, sge->vaddr, len, 1);
		sge->vaddr += len;
		sge->length -= len;
		sge->sge_length -= len;
		if (sge->sge_length == 0) {
			if (--ssge.num_sge)
				*sge = *ssge.sg_list++;
		} else if (sge->length == 0 && sge->mr->lkey) {
			if (++sge->n >= HFI2_SEGSZ) {
				if (++sge->m >= sge->mr->mapsz)
					break;
				sge->n = 0;
			}
			sge->vaddr =
				sge->mr->map[sge->m]->segs[sge->n].vaddr;
			sge->length =
				sge->mr->map[sge->m]->segs[sge->n].length;
		}
		length -= len;
	}
	hfi2_put_ss(&qp->r_sge);
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
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)
			wc.pkey_index = swqe->ud_wr.pkey_index;
#else
			wc.pkey_index = swqe->wr.wr.ud.pkey_index;
#endif
		else
			wc.pkey_index = sqp->s_pkey_index;
	} else {
		wc.pkey_index = 0;
	}
	wc.slid = ppd->lid | (ah_attr->src_path_bits & ((1 << ppd->lmc) - 1));
	/* Check for loopback when the port lid is not set */
	if (wc.slid == 0 && sqp->ibqp.qp_type == IB_QPT_GSI)
		wc.slid = HFI1_PERMISSIVE_LID;
	wc.sl = ah_attr->sl;
	wc.dlid_path_bits = ah_attr->dlid & ((1 << ppd->lmc) - 1);
	wc.port_num = qp->port_num;
	/* Signal completion event if the solicited bit is set. */
	hfi2_cq_enter(to_hfi_cq(qp->ibqp.recv_cq), &wc,
			swqe->wr.send_flags & IB_SEND_SOLICITED);
	ibp->n_loop_pkts++;
bail_unlock:
	spin_unlock_irqrestore(&qp->r_lock, flags);
drop:
	rcu_read_unlock();
}

static void hfi2_make_ud_header(struct hfi2_qp *qp, struct hfi2_swqe *wqe,
				struct ib_l4_headers *ohdr, u8 opcode)
{
	struct hfi2_ibport *ibp = to_hfi_ibp(qp->ibqp.device, qp->port_num);
	struct hfi_pportdata *ppd = ibp->ppd;
	struct ib_ah_attr *ah_attr;
	u16 lrh0;
	u32 nwords;
	u32 extra_bytes;
	u32 bth0;
	u8 sc5;

	/* Construct the header. */
	extra_bytes = -wqe->length & 3;
	nwords = ((wqe->length + extra_bytes) >> 2) + SIZE_OF_CRC;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)
	ah_attr = &to_hfi_ah(wqe->ud_wr.ah)->attr;
#else
	ah_attr = &to_hfi_ah(wqe->wr.wr.ud.ah)->attr;
#endif
	if (ah_attr->ah_flags & IB_AH_GRH) {
		/* remove LRH size from s_hdrwords for GRH */
		qp->s_hdrwords += hfi2_make_grh(ibp, &qp->s_hdr->ph.ibh.u.l.grh,
						&ah_attr->grh,
						(qp->s_hdrwords - 2),
						nwords);
		lrh0 = HFI1_LRH_GRH;
	} else {
		lrh0 = HFI1_LRH_BTH;
	}
	wqe->lnh = lrh0;

	sc5 = ppd->sl_to_sc[ah_attr->sl];
	lrh0 |= (ah_attr->sl & 0xf) << 4;
	if (qp->ibqp.qp_type == IB_QPT_SMI) {
		lrh0 |= 0xF000; /* Set VL (see ch. 13.5.3.1) */
		qp->s_sc = 0xf;
		wqe->use_sc15 = true;
	} else {
		lrh0 |= (sc5 & 0xf) << 12;
		qp->s_sc = sc5;
		wqe->use_sc15 = false;
	}
	qp->s_hdr->ph.ibh.lrh[0] = cpu_to_be16(lrh0);
	qp->s_hdr->ph.ibh.lrh[1] = cpu_to_be16(ah_attr->dlid);  /* DEST LID */
	qp->s_hdr->ph.ibh.lrh[2] =
		cpu_to_be16(qp->s_hdrwords + nwords);
	if (ah_attr->dlid == IB_LID_PERMISSIVE) {
		qp->s_hdr->ph.ibh.lrh[3] = IB_LID_PERMISSIVE;
	} else {
		u16 lid = ppd->lid;

		if (lid) {
			lid |= ah_attr->src_path_bits & ((1 << ppd->lmc) - 1);
			qp->s_hdr->ph.ibh.lrh[3] = cpu_to_be16(lid);
		} else {
			qp->s_hdr->ph.ibh.lrh[3] = IB_LID_PERMISSIVE;
		}
	}

	bth0 = opcode << 24;
	if (wqe->wr.send_flags & IB_SEND_SOLICITED)
		bth0 |= IB_BTH_SOLICITED;
	bth0 |= extra_bytes << 20;
	if (qp->ibqp.qp_type == IB_QPT_GSI || qp->ibqp.qp_type == IB_QPT_SMI)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)
		bth0 |= hfi2_get_pkey(ibp, wqe->ud_wr.pkey_index);
#else
		bth0 |= hfi2_get_pkey(ibp, wqe->wr.wr.ud.pkey_index);
#endif
	else
		bth0 |= hfi2_get_pkey(ibp, qp->s_pkey_index);

	ohdr->bth[0] = cpu_to_be32(bth0);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)
	ohdr->bth[1] = cpu_to_be32(wqe->ud_wr.remote_qpn);
#else
	ohdr->bth[1] = cpu_to_be32(wqe->wr.wr.ud.remote_qpn);
#endif
	ohdr->bth[2] = cpu_to_be32(mask_psn(qp->s_next_psn++));
	wqe->use_16b = false;
}

static void hfi2_make_16b_ud_header(struct hfi2_qp *qp, struct hfi2_swqe *wqe,
				    struct ib_l4_headers *ohdr, u8 opcode)
{
	struct hfi2_opa16b_header *opa16b = &qp->s_hdr->opa16b;
	struct hfi2_ibport *ibp = to_hfi_ibp(qp->ibqp.device, qp->port_num);
	struct hfi_pportdata *ppd = ibp->ppd;
	struct ib_ah_attr *ah_attr;
	u32 nwords, extra_bytes;
	u32 bth0, qwords, slid, dlid;
	u8 sc5, l4;
	u16 pkey;

	/* Construct the header. */
	/* Whole packet (hdr, data, crc, tail byte) should be flit aligned */
	extra_bytes = -(wqe->length + (qp->s_hdrwords << 2) +
			(SIZE_OF_CRC << 2) + 1) & 7;
	nwords = (wqe->length + (SIZE_OF_CRC << 2) + 1 + extra_bytes) >> 2;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)
	ah_attr = &to_hfi_ah(wqe->ud_wr.ah)->attr;
#else
	ah_attr = &to_hfi_ah(wqe->wr.wr.ud.ah)->attr;
#endif
	if (ah_attr->ah_flags & IB_AH_GRH) {
		/* remove 16B HDR size from s_hdrwords for GRH */
		qp->s_hdrwords += hfi2_make_grh(ibp, &opa16b->u.l.grh,
						&ah_attr->grh,
						(qp->s_hdrwords - 4), nwords);
		l4 = HFI1_L4_IB_GLOBAL;
		wqe->lnh = HFI1_LRH_GRH;
	} else {
		l4 = HFI1_L4_IB_LOCAL;
		wqe->lnh = HFI1_LRH_BTH;
	}

	sc5 = ppd->sl_to_sc[ah_attr->sl];
	if (qp->ibqp.qp_type == IB_QPT_SMI) {
		qp->s_sc = 0xf;
		wqe->use_sc15 = true;
	} else {
		qp->s_sc = sc5;
		wqe->use_sc15 = false;
	}

	/*
	 * FXRTODO: Translate 9B multicast and permissive LIDs
	 * to 16B for now, fix later.
	 * Also need to program FECN/BECN.
	 */
	if (ah_attr->dlid == IB_LID_PERMISSIVE) {
		slid = HFI1_16B_PERMISSIVE_LID;
	} else {
		slid = ppd->lid;
		if (slid)
			slid |= ah_attr->src_path_bits & ((1 << ppd->lmc) - 1);
		else
			slid = HFI1_16B_PERMISSIVE_LID;
	}

	if (ah_attr->dlid == IB_LID_PERMISSIVE) {
		dlid = HFI1_16B_PERMISSIVE_LID;
	} else if (ah_attr->dlid >= HFI1_MULTICAST_LID_BASE) {
		dlid = ah_attr->dlid - HFI1_MULTICAST_LID_BASE;
		dlid += HFI1_16B_MULTICAST_LID_BASE;
	} else {
		dlid = ah_attr->dlid;
	}

	if (qp->ibqp.qp_type == IB_QPT_GSI || qp->ibqp.qp_type == IB_QPT_SMI)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)
		pkey = hfi2_get_pkey(ibp, wqe->ud_wr.pkey_index);
#else
		pkey = hfi2_get_pkey(ibp, wqe->wr.wr.ud.pkey_index);
#endif
	else
		pkey = hfi2_get_pkey(ibp, qp->s_pkey_index);

	qwords = (qp->s_hdrwords + nwords) >> 1;

	opa_make_16b_header((u32 *)&qp->s_hdr->opa16b,
			    slid, dlid, qwords, pkey,
			    0, qp->s_sc, 0, 0, 0, 0, l4);

	bth0 = opcode << 24;
	if (wqe->wr.send_flags & IB_SEND_SOLICITED)
		bth0 |= IB_BTH_SOLICITED;
	bth0 |= extra_bytes << 20;
	ohdr->bth[0] = cpu_to_be32(bth0);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)
	ohdr->bth[1] = cpu_to_be32(wqe->ud_wr.remote_qpn);
#else
	ohdr->bth[1] = cpu_to_be32(wqe->wr.wr.ud.remote_qpn);
#endif
	ohdr->bth[2] = cpu_to_be32(mask_psn(qp->s_next_psn++));
	wqe->use_16b = true;
}

/**
 * _hfi2_make_ud_req - construct a UD request packet
 * @qp: the QP
 * @is_16b: if set use 16B header, otherwise use LRH
 *
 * Return: 1 if constructed; otherwise 0.
 */
static int _hfi2_make_ud_req(struct hfi2_qp *qp, bool is_16b)
{
	struct ib_l4_headers *ohdr;
	struct ib_ah_attr *ah_attr;
	struct hfi2_ibport *ibp;
	struct hfi_pportdata *ppd;
	struct hfi2_swqe *wqe;
	unsigned long flags;
	u8 opcode;
	u16 lid;
	int ret = 0;
	int next_cur;

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
		hfi2_send_complete(qp, wqe, IB_WC_WR_FLUSH_ERR);
		goto done;
	}

	if (qp->s_cur == qp->s_head)
		goto bail;

	wqe = get_swqe_ptr(qp, qp->s_cur);
	next_cur = qp->s_cur + 1;
	if (next_cur >= qp->s_size)
		next_cur = 0;

	/* Construct the header. */
	ibp = to_hfi_ibp(qp->ibqp.device, qp->port_num);
	ppd = ibp->ppd;
	/* TODO - review wr.wr.ud.ah */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)
	ah_attr = &to_hfi_ah(wqe->ud_wr.ah)->attr;
#else
	ah_attr = &to_hfi_ah(wqe->wr.wr.ud.ah)->attr;
#endif
	if (ah_attr->dlid < HFI1_MULTICAST_LID_BASE ||
	    ah_attr->dlid == HFI1_PERMISSIVE_LID) {
		lid = ah_attr->dlid & ~((1 << ppd->lmc) - 1);
		/*
		 * FXRTODO - WFR had additional test to skip when in link
		 * loopback, maybe to ensure datapath tested in that mode
		 */
		if (unlikely(lid == ppd->lid ||
		    (lid == HFI1_PERMISSIVE_LID &&
		    qp->ibqp.qp_type == IB_QPT_GSI) ||
		    (force_loopback && qp->ibqp.qp_type == IB_QPT_SMI))) {
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
			hfi2_send_complete(qp, wqe, IB_WC_SUCCESS);
			goto done;
		}
	}

	qp->s_cur = next_cur;

	/* 16B(4)/LRH(2) + BTH(3) + DETH(2) */
	qp->s_hdrwords = is_16b ? 9 : 7;
	qp->s_cur_size = wqe->length;
	qp->s_cur_sge = &qp->s_sge;
	qp->s_srate = ah_attr->static_rate;
	qp->srate_mbps = ib_rate_to_mbps(qp->s_srate);
	qp->s_wqe = wqe;
	qp->s_sge.sge = wqe->sg_list[0];
	qp->s_sge.sg_list = wqe->sg_list + 1;
	qp->s_sge.num_sge = wqe->wr.num_sge;
	qp->s_sge.total_len = wqe->length;

	if (ah_attr->ah_flags & IB_AH_GRH)
		/*
		 * Don't worry about sending to locally attached multicast
		 * QPs.  It is unspecified by the spec. what happens.
		 */
		ohdr = is_16b ? &qp->s_hdr->opa16b.u.l.oth :
				&qp->s_hdr->ph.ibh.u.l.oth;
	else
		ohdr = is_16b ? &qp->s_hdr->opa16b.u.oth :
				&qp->s_hdr->ph.ibh.u.oth;

	if (wqe->wr.opcode == IB_WR_SEND_WITH_IMM) {
		qp->s_hdrwords++;
		ohdr->u.ud.imm_data = wqe->wr.ex.imm_data;
		opcode = IB_OPCODE_UD_SEND_ONLY_WITH_IMMEDIATE;
	} else
		opcode = IB_OPCODE_UD_SEND_ONLY;

	if (is_16b)
		hfi2_make_16b_ud_header(qp, wqe, ohdr, opcode);
	else
		hfi2_make_ud_header(qp, wqe, ohdr, opcode);

	/*
	 * Qkeys with the high order bit set mean use the
	 * qkey from the QP context instead of the WR (see 10.2.5).
	 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)
	ohdr->u.ud.deth[0] = cpu_to_be32((int)wqe->ud_wr.remote_qkey < 0 ?
					 qp->qkey : wqe->ud_wr.remote_qkey);
#else
	ohdr->u.ud.deth[0] = cpu_to_be32((int)wqe->wr.wr.ud.remote_qkey < 0 ?
					 qp->qkey : wqe->wr.wr.ud.remote_qkey);
#endif
	ohdr->u.ud.deth[1] = cpu_to_be32(qp->ibqp.qp_num);

	/* TODO for now, WQE contains everything needed to perform the Send */
	wqe->s_qp = qp;
	wqe->s_sge = &qp->s_sge;
	wqe->s_hdr = qp->s_hdr;
	wqe->s_hdrwords = qp->s_hdrwords;
	wqe->s_ctx = qp->s_ctx;
	wqe->sl = ah_attr->sl;
	wqe->pkt_errors = 0;
	/*
	 * UD packets are not fragmented, set to max MTU as send_wqe()
	 * is transport agnostic.
	 */
	wqe->pmtu = opa_enum_to_mtu(OPA_MTU_10240);
done:
	ret = 1;
	goto unlock;

bail:
	qp->s_flags &= ~HFI1_S_BUSY;
unlock:
	spin_unlock_irqrestore(&qp->s_lock, flags);
	return ret;
}

int hfi2_make_ud_req(struct hfi2_qp *qp)
{
	/* FXRTODO: Dynamically determain encapsulation type */
	return _hfi2_make_ud_req(qp, is_16b_mode());
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
int hfi2_lookup_pkey_idx(struct hfi2_ibport *ibp, u16 pkey)
{
	unsigned i;
	struct hfi_pportdata *ppd = ibp->ppd;

	if (pkey == OPA_FULL_MGMT_PKEY || pkey == OPA_LIM_MGMT_PKEY) {
		unsigned lim_idx = -1;

		for (i = 0; i < HFI_MAX_PKEYS; ++i) {
			/* here we look for an exact match */
			if (ppd->pkeys[i] == pkey)
				return i;
			if (ppd->pkeys[i] == OPA_LIM_MGMT_PKEY)
				lim_idx = i;
		}

		/* did not find 0xffff return 0x7fff idx if found */
		if (pkey == OPA_FULL_MGMT_PKEY)
			return lim_idx;

		/* no match...  */
		return -1;
	}

	pkey &= 0x7fff; /* remove limited/full membership bit */

	for (i = 0; i < HFI_MAX_PKEYS; ++i)
		if ((ppd->pkeys[i] & 0x7fff) == pkey)
			return i;

	/*
	 * Should not get here, this means hardware failed to validate pkeys.
	 */
	return -1;
}

/**
 * hfi2_ud_rcv - receive an incoming UD packet
 * @qp: the QP the packet came on
 * @packet: incoming packet information
 *
 * This is called from hfi2_rcv() to process an incoming UD packet
 * for the given QP.
 */
void hfi2_ud_rcv(struct hfi2_qp *qp, struct hfi2_ib_packet *packet)
{
	struct ib_l4_headers *ohdr = packet->ohdr;
	int opcode;
	u32 hdrsize = packet->hlen;
	u32 pad;
	struct ib_wc wc;
	u32 qkey;
	u32 src_qp;
	int mgmt_pkey_idx = -1;
	struct hfi2_ibport *ibp = packet->ibp;
	struct hfi_pportdata *ppd = ibp->ppd;
	union hfi2_packet_header *ph = packet->hdr;
	void *data = packet->ebuf;
	u32 tlen = packet->tlen;
	u8 sc5, extra_bytes;
	bool is_becn, is_fecn, is_mcast;
	struct ib_grh *grh = packet->grh;
	u32 slid, dlid;
	u8 age, l4, rc;
	u16 entropy, len, pkey;
	bool is_16b = (packet->etype == RHF_RCV_TYPE_BYPASS);

	/* add DETH */
	hdrsize += 8;

	if (is_16b) {
		data = packet->ebuf + hdrsize;
		opa_parse_16b_header((u32 *)&ph->opa16b, &slid, &dlid, &len,
				     &pkey, &entropy, &sc5, &rc, &is_fecn,
				     &is_becn, &age, &l4);

		/*
		 * FXRTODO: Translate 16B multicast and permissive LIDs
		 * to 9B for now, fix later.
		 */
		if (slid == HFI1_16B_PERMISSIVE_LID)
			slid = HFI1_PERMISSIVE_LID;
		if (dlid == HFI1_16B_PERMISSIVE_LID) {
			dlid = HFI1_PERMISSIVE_LID;
		} else if (dlid >= HFI1_16B_MULTICAST_LID_BASE) {
			dlid -= HFI1_16B_MULTICAST_LID_BASE;
			dlid += HFI1_MULTICAST_LID_BASE;
		}

		/* Get the number of bytes the message was padded by. */
		pad = (be32_to_cpu(ohdr->bth[0]) >> 20) & 7;
		extra_bytes = 5;    /* ICRC + TAIL byte */
	} else {
		dlid = be16_to_cpu(ph->ibh.lrh[1]);
		slid = be16_to_cpu(ph->ibh.lrh[3]);
		sc5 = (be16_to_cpu(ph->ibh.lrh[0]) >> 12) & 0xf;
		sc5 |= packet->sc4_bit;
		pkey = (u16)be32_to_cpu(ohdr->bth[0]);

		is_becn = !!((be32_to_cpu(ohdr->bth[1]) >> HFI1_BECN_SHIFT)
			     & HFI1_BECN_MASK);
		is_fecn = !!((be32_to_cpu(ohdr->bth[1]) >> HFI1_FECN_SHIFT)
			     & HFI1_FECN_MASK);

		/* Get the number of bytes the message was padded by. */
		pad = (be32_to_cpu(ohdr->bth[0]) >> 20) & 3;
		extra_bytes = 4;    /* ICRC */
	}

	qkey = be32_to_cpu(ohdr->u.ud.deth[0]);
	src_qp = be32_to_cpu(ohdr->u.ud.deth[1]) & HFI1_QPN_MASK;
	is_mcast = (dlid > HFI1_MULTICAST_LID_BASE) &&
			(dlid != HFI1_PERMISSIVE_LID);

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

	/* Drop incomplete packets */
	if (unlikely(tlen < (hdrsize + pad + extra_bytes)))
		goto drop;
	tlen -= hdrsize + pad + extra_bytes;
	/*
	 * Check that the permissive LID is only used on QP0
	 * and the QKEY matches (see 9.6.1.4.1 and 9.6.1.5.1).
	 */
	if (qp->ibqp.qp_num) {
		if (unlikely(dlid == IB_LID_PERMISSIVE ||
			     slid == IB_LID_PERMISSIVE))
			goto drop;
		if (qp->ibqp.qp_num > 1) {
#if 0 /* FXRTODO */
			/*
			 * FXRTODO - WFR introduced this more light-weight pkey
			 * check.  Verify this is valid for 9B packets on FXR.
			 */
			if (unlikely(rcv_pkey_check(ibp, pkey, sc5, slid))) {
				/*
				 * Traps will not be sent for packets dropped
				 * by the HW. This is fine, as sending trap
				 * for invalid pkeys is optional according to
				 * IB spec (release 1.3, section 10.9.4)
				 */
				hfi2_bad_pqkey(ibp, IB_NOTICE_TRAP_BAD_PKEY,
					       pkey, sl,
					       src_qp, qp->ibqp.qp_num,
					       slid, dlid);
				return;
			}
#endif
		} else {
			/* GSI packet */
			mgmt_pkey_idx = hfi2_lookup_pkey_idx(ibp, pkey);
			if (mgmt_pkey_idx < 0)
				goto drop;

		}
#if 0 /* FXRTODO */
		if (unlikely(qkey != qp->qkey)) {
			hfi2_bad_pqkey(ibp, IB_NOTICE_TRAP_BAD_QKEY,
				       qkey, sl,
				       src_qp, qp->ibqp.qp_num,
				       slid, dlid);
			return;
		}
#endif
		/* Drop invalid MAD packets (see 13.5.3.1). */
		/* TODO - bug?  4-bit SC versus 5-bit SC here? */
		if (unlikely(qp->ibqp.qp_num == 1 &&
			     (tlen > 2048 ||
			      (sc5 == 15))))
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
		if ((dlid == IB_LID_PERMISSIVE ||
		     slid == IB_LID_PERMISSIVE) &&
		    smp->mgmt_class != IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE)
			goto drop;

		/* lookup SMI pkey */
		mgmt_pkey_idx = hfi2_lookup_pkey_idx(ibp, pkey);
		if (mgmt_pkey_idx < 0)
			goto drop;
	}

	if (qp->ibqp.qp_num > 1 &&
	    opcode == IB_OPCODE_UD_SEND_ONLY_WITH_IMMEDIATE) {
		wc.ex.imm_data = ohdr->u.ud.imm_data;
		wc.wc_flags = IB_WC_WITH_IMM;
		tlen -= sizeof(u32);
		data += (is_16b ? sizeof(u32) : 0);
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

		ret = hfi2_get_rwqe(qp, 0);
		if (ret < 0) {
			hfi2_rc_error(qp, IB_WC_LOC_QP_OP_ERR);
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
	if (grh) {
		hfi2_copy_sge(&qp->r_sge, grh,
			      sizeof(struct ib_grh), 1);
		wc.wc_flags |= IB_WC_GRH;
	} else
		hfi2_skip_sge(&qp->r_sge, sizeof(struct ib_grh), 1);
	hfi2_copy_sge(&qp->r_sge, data,
			wc.byte_len - sizeof(struct ib_grh), 1);
	hfi2_put_ss(&qp->r_sge);
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
	wc.sl = ppd->sc_to_sl[sc5];

	/*
	 * Save the LMC lower bits if the destination LID is a unicast LID.
	 */
	wc.dlid_path_bits = dlid >= HFI1_MULTICAST_LID_BASE ? 0 :
		dlid & ((1 << ppd->lmc) - 1);
	wc.port_num = qp->port_num;
	/* Signal completion event if the solicited bit is set. */
	hfi2_cq_enter(to_hfi_cq(qp->ibqp.recv_cq), &wc,
			(ohdr->bth[0] & cpu_to_be32(IB_BTH_SOLICITED)) != 0);
	return;

drop:
	dev_info(ibp->dev, "UD dropping packet\n");
	ibp->n_pkt_drops++;
}
