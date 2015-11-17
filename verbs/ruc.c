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
 * Send if not busy or waiting for I/O and either
 * a RC response is pending or we can process send work requests.
 */
static inline int send_ok(struct opa_ib_qp *qp)
{
	return !(qp->s_flags & (HFI1_S_BUSY | HFI1_S_ANY_WAIT_IO)) &&
		(qp->s_hdrwords || (qp->s_flags & HFI1_S_RESP_PENDING) ||
		 !(qp->s_flags & HFI1_S_ANY_WAIT_SEND));
}

/*
 * Validate a RWQE and fill in the receive SGE state.
 * Return 1 if OK.
 */
static int init_sge(struct opa_ib_qp *qp, struct opa_ib_rwqe *wqe)
{
	int i, j, ret;
	struct ib_wc wc;
	struct opa_ib_lkey_table *rkt;
	struct opa_ib_pd *pd;
	struct opa_ib_sge_state *ss;

	rkt = &to_opa_ibdata(qp->ibqp.device)->lk_table;
	pd = to_opa_ibpd(qp->ibqp.srq ? qp->ibqp.srq->pd : qp->ibqp.pd);
	ss = &qp->r_sge;
	ss->sg_list = qp->r_sg_list;
	qp->r_len = 0;
	for (i = j = 0; i < wqe->num_sge; i++) {
		if (wqe->sg_list[i].length == 0)
			continue;
		/* Check LKEY */
		if (!opa_ib_lkey_ok(rkt, pd, j ? &ss->sg_list[j - 1] : &ss->sge,
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
		struct hfi2_sge *sge = --j ? &ss->sg_list[j - 1] : &ss->sge;

		hfi2_put_mr(sge->mr);
	}
	ss->num_sge = 0;
	memset(&wc, 0, sizeof(wc));
	wc.wr_id = wqe->wr_id;
	wc.status = IB_WC_LOC_PROT_ERR;
	wc.opcode = IB_WC_RECV;
	wc.qp = &qp->ibqp;
	/* Signal solicited completion event. */
	opa_ib_cq_enter(to_opa_ibcq(qp->ibqp.recv_cq), &wc, 1);
	ret = 0;
bail:
	return ret;
}


/**
 * opa_ib_get_rwqe - copy the next RWQE into the QP's RWQE
 * @qp: the QP
 * @wr_id_only: update qp->r_wr_id only, not qp->r_sge
 *
 * Can be called from interrupt level.
 *
 * Return: -1 if there is a local error, 0 if no RWQE is available,
 * otherwise return 1.
 */
int opa_ib_get_rwqe(struct opa_ib_qp *qp, int wr_id_only)
{
	unsigned long flags;
	struct opa_ib_rq *rq;
	struct opa_ib_rwq *wq;
	struct opa_ib_srq *srq;
	struct opa_ib_rwqe *wqe;

	void (*handler)(struct ib_event *, void *);
	u32 tail;
	int ret;

	if (qp->ibqp.srq) {
		srq = to_opa_ibsrq(qp->ibqp.srq);
		handler = srq->ibsrq.event_handler;
		rq = &srq->rq;
	} else {
		srq = NULL;
		handler = NULL;
		rq = &qp->r_rq;
	}

	spin_lock_irqsave(&rq->lock, flags);
	if (!(ib_qp_state_ops[qp->state] & HFI1_PROCESS_RECV_OK)) {
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
	wqe = get_rwqe_ptr(rq, tail);
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
	set_bit(HFI1_R_WRID_VALID, &qp->r_aflags);
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

void opa_ib_make_ruc_header(struct opa_ib_qp *qp, struct ib_l4_headers *ohdr,
			    u32 bth0, u32 bth2, u16 *out_lrh0)
{
	struct opa_ib_portdata *ibp;
	u16 lrh0;
	u32 nwords;
	u32 extra_bytes;
	u8 sc5;
	u32 bth1;

	/* Construct the header. */
	ibp = to_opa_ibportdata(qp->ibqp.device, qp->port_num);
	extra_bytes = -qp->s_cur_size & 3;
	nwords = (qp->s_cur_size + extra_bytes) >> 2;
	lrh0 = HFI1_LRH_BTH;
	if (unlikely(qp->remote_ah_attr.ah_flags & IB_AH_GRH)) {
		qp->s_hdrwords += opa_ib_make_grh(ibp, &qp->s_hdr->ibh.u.l.grh,
					       &qp->remote_ah_attr.grh,
					       qp->s_hdrwords, nwords);
		lrh0 = HFI1_LRH_GRH;
	}
	sc5 = ibp->sl_to_sc[qp->remote_ah_attr.sl];
	lrh0 |= (sc5 & 0xf) << 12 | (qp->remote_ah_attr.sl & 0xf) << 4;
	qp->s_sc = sc5;
	if (qp->s_mig_state == IB_MIG_MIGRATED)
		bth0 |= IB_BTH_MIG_REQ;
	qp->s_hdr->ibh.lrh[0] = cpu_to_be16(lrh0);
	qp->s_hdr->ibh.lrh[1] = cpu_to_be16(qp->remote_ah_attr.dlid);
	qp->s_hdr->ibh.lrh[2] =
		cpu_to_be16(qp->s_hdrwords + nwords + SIZE_OF_CRC);
	qp->s_hdr->ibh.lrh[3] = cpu_to_be16(ibp->lid |
				       qp->remote_ah_attr.src_path_bits);
	bth0 |= opa_ib_get_pkey(ibp, qp->s_pkey_index);
	bth0 |= extra_bytes << 20;
	ohdr->bth[0] = cpu_to_be32(bth0);
	bth1 = qp->remote_qpn;
	if (qp->s_flags & HFI1_S_ECN) {
		qp->s_flags &= ~HFI1_S_ECN;
		/* we recently received a FECN, so return a BECN */
		bth1 |= (HFI1_BECN_MASK << HFI1_BECN_SHIFT);
	}
	ohdr->bth[1] = cpu_to_be32(bth1);
	ohdr->bth[2] = cpu_to_be32(bth2);
	*out_lrh0 = lrh0;
}

/**
 * send_wqe() - submit a WGE to the hardware
 * @ibp: outgoing port
 * @qp: the QP to send on
 * @wge: WGE to submit
 * TODO - delete below when STL-2554 implemented
 * @wait: wait structure to use when no slots (may be NULL)
 *
 * If a iowait structure is non-NULL the packet will be queued to the list
 * in wait.
 * TODO - see WFR's sdma_send_txreq() as reference.
 *
 * Return: TBD
 */
static int send_wqe(struct opa_ib_portdata *ibp, struct opa_ib_qp *qp,
		    struct opa_ib_swqe *wqe)
{
	int ret;

	ret = opa_ib_send_wqe(ibp, qp, wqe);
	if (ret < 0)
		opa_ib_send_complete(qp, wqe, IB_WC_FATAL_ERR);
	/* else send_complete issued upon DMA completion event */

	return ret;
}

/**
 * opa_ib_verbs_send - send a packet
 * @qp: the QP to send on
 * @hdr: the packet header
 * @hdrwords: the number of 32-bit words in the header
 * @ss: the SGE to send
 * @len: the length of the packet in bytes
 *
 * Return: zero if packet is sent or queued OK, non-zero
 * and clear qp->s_flags HFI1_S_BUSY otherwise.
 */
int opa_ib_verbs_send(struct opa_ib_qp *qp, struct opa_ib_dma_header *hdr,
		      u32 hdrwords, struct opa_ib_sge_state *ss, u32 len)
{
	struct opa_ib_portdata *ibp;
	int ret = 0;
	unsigned long flags = 0;

	ibp = to_opa_ibportdata(qp->ibqp.device, qp->port_num);

	//ret = egress_pkey_check(ibp, &hdr->ibh, qp);
	if (unlikely(ret)) {
		//hfi1_cdbg(PIO, "%s() Failed. Completing with err", __func__);
		spin_lock_irqsave(&qp->s_lock, flags);
		opa_ib_send_complete(qp, qp->s_wqe, IB_WC_GENERAL_ERR);
		spin_unlock_irqrestore(&qp->s_lock, flags);
		return -EINVAL;
	}

	/*
	 * Send from TX overflow list first.
	 * TODO - not fully integrated, we should reuse if we can. WFR
	 * enqueues to this list in sdma_check_progress via sleep callback.
	 */
	if (!list_empty(&qp->s_iowait.tx_head)) {
		struct opa_ib_swqe *wqe;

		wqe = list_first_entry(
			&qp->s_iowait.tx_head,
			struct opa_ib_swqe,
			pending_list);
		list_del_init(&wqe->pending_list);
		/* send queued pending WGE into fabric */
		/* TODO - STL-2554 should qp->s_wqe be updated? */
		ret = send_wqe(ibp, qp, wqe);
		/*
		 * TODO - why does WFR just return without enqueuing the
		 * WQE that would have been sent below (qp->s_wqe).
		 */
		return ret;
	}

	/* send WGE into fabric */
	return send_wqe(ibp, qp, qp->s_wqe);
}

/**
 * opa_ib_do_send - perform a send on a QP
 * @work: contains a pointer to the QP
 *
 * Process entries in the send work queue until credit or queue is
 * exhausted.  Only allow one CPU to send a packet per QP (tasklet).
 * Otherwise, two threads could send packets out of order.
 */
void opa_ib_do_send(struct work_struct *work)
{
	struct iowait *wait = container_of(work, struct iowait, iowork);
	struct opa_ib_qp *qp = container_of(wait, struct opa_ib_qp, s_iowait);
	int (*make_req)(struct opa_ib_qp *qp);
	unsigned long flags;

	/* FXRTODO - WFR performs RC/UC loopback here */

	if (qp->ibqp.qp_type == IB_QPT_RC)
		return; /* TODO - no RC support yet */
	else if (qp->ibqp.qp_type == IB_QPT_UC)
		make_req = opa_ib_make_uc_req;
	else
		make_req = opa_ib_make_ud_req;

	spin_lock_irqsave(&qp->s_lock, flags);
	/* Return if we are already busy processing a work request. */
	if (!send_ok(qp)) {
		spin_unlock_irqrestore(&qp->s_lock, flags);
		return;
	}
	qp->s_flags |= HFI1_S_BUSY;
	spin_unlock_irqrestore(&qp->s_lock, flags);

	do {
		/* Check for a constructed packet to be sent. */
		if (qp->s_hdrwords != 0) {
			/*
			 * If the packet cannot be sent now, return and
			 * the send tasklet will be woken up later.
			 */
			if (opa_ib_verbs_send(qp, qp->s_hdr, qp->s_hdrwords,
					      qp->s_cur_sge, qp->s_cur_size))
				break;

			/* Record that s_hdr is empty. */
			qp->s_hdrwords = 0;
		}
	} while (make_req(qp));
}

/*
 * This should be called with s_lock held.
 */
void opa_ib_send_complete(struct opa_ib_qp *qp, struct opa_ib_swqe *wqe,
			  enum ib_wc_status status)
{
	u32 old_last, last;
	unsigned i;

	if (!(ib_qp_state_ops[qp->state] & HFI1_PROCESS_OR_FLUSH_SEND))
		return;

	for (i = 0; i < wqe->wr.num_sge; i++) {
		struct hfi2_sge *sge = &wqe->sg_list[i];

		hfi2_put_mr(sge->mr);
	}
	if (qp->ibqp.qp_type == IB_QPT_UD ||
	    qp->ibqp.qp_type == IB_QPT_SMI ||
	    qp->ibqp.qp_type == IB_QPT_GSI)
		atomic_dec(&to_opa_ibah(wqe->wr.wr.ud.ah)->refcount);

	/* See ch. 11.2.4.1 and 10.7.3.1 */
	if (!(qp->s_flags & HFI1_S_SIGNAL_REQ_WR) ||
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
		opa_ib_cq_enter(to_opa_ibcq(qp->ibqp.send_cq), &wc,
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
void opa_ib_schedule_send(struct opa_ib_qp *qp)
{
	/* FXRTODO */
#if 0
	if (send_ok(qp)) {
		struct opa_ib_portdata *ibp;

		ibp = to_opa_ibportdata(qp->ibqp.device, qp->port_num);
		iowait_schedule(&qp->s_iowait, ibp->wq);
	}
#endif
}
