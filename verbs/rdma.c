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

#include <linux/sched.h>
#include "verbs.h"
#include "packet.h"

static void opa_ib_cnp_rcv(struct opa_ib_qp *qp, struct opa_ib_packet *packet);
typedef void (*opcode_handler)(struct opa_ib_qp *qp,
			       struct opa_ib_packet *packet);

static const opcode_handler opcode_handler_tbl[256] = {
	/* RC */
	[IB_OPCODE_RC_SEND_FIRST]                     = &opa_ib_rc_rcv,
	[IB_OPCODE_RC_SEND_MIDDLE]                    = &opa_ib_rc_rcv,
	[IB_OPCODE_RC_SEND_LAST]                      = &opa_ib_rc_rcv,
	[IB_OPCODE_RC_SEND_LAST_WITH_IMMEDIATE]       = &opa_ib_rc_rcv,
	[IB_OPCODE_RC_SEND_ONLY]                      = &opa_ib_rc_rcv,
	[IB_OPCODE_RC_SEND_ONLY_WITH_IMMEDIATE]       = &opa_ib_rc_rcv,
	[IB_OPCODE_RC_RDMA_WRITE_FIRST]               = &opa_ib_rc_rcv,
	[IB_OPCODE_RC_RDMA_WRITE_MIDDLE]              = &opa_ib_rc_rcv,
	[IB_OPCODE_RC_RDMA_WRITE_LAST]                = &opa_ib_rc_rcv,
	[IB_OPCODE_RC_RDMA_WRITE_LAST_WITH_IMMEDIATE] = &opa_ib_rc_rcv,
	[IB_OPCODE_RC_RDMA_WRITE_ONLY]                = &opa_ib_rc_rcv,
	[IB_OPCODE_RC_RDMA_WRITE_ONLY_WITH_IMMEDIATE] = &opa_ib_rc_rcv,
	[IB_OPCODE_RC_RDMA_READ_REQUEST]              = &opa_ib_rc_rcv,
	[IB_OPCODE_RC_RDMA_READ_RESPONSE_FIRST]       = &opa_ib_rc_rcv,
	[IB_OPCODE_RC_RDMA_READ_RESPONSE_MIDDLE]      = &opa_ib_rc_rcv,
	[IB_OPCODE_RC_RDMA_READ_RESPONSE_LAST]        = &opa_ib_rc_rcv,
	[IB_OPCODE_RC_RDMA_READ_RESPONSE_ONLY]        = &opa_ib_rc_rcv,
	[IB_OPCODE_RC_ACKNOWLEDGE]                    = &opa_ib_rc_rcv,
	[IB_OPCODE_RC_ATOMIC_ACKNOWLEDGE]             = &opa_ib_rc_rcv,
	[IB_OPCODE_RC_COMPARE_SWAP]                   = &opa_ib_rc_rcv,
	[IB_OPCODE_RC_FETCH_ADD]                      = &opa_ib_rc_rcv,
	/* UC */
	[IB_OPCODE_UC_SEND_FIRST]                     = &opa_ib_uc_rcv,
	[IB_OPCODE_UC_SEND_MIDDLE]                    = &opa_ib_uc_rcv,
	[IB_OPCODE_UC_SEND_LAST]                      = &opa_ib_uc_rcv,
	[IB_OPCODE_UC_SEND_LAST_WITH_IMMEDIATE]       = &opa_ib_uc_rcv,
	[IB_OPCODE_UC_SEND_ONLY]                      = &opa_ib_uc_rcv,
	[IB_OPCODE_UC_SEND_ONLY_WITH_IMMEDIATE]       = &opa_ib_uc_rcv,
	[IB_OPCODE_UC_RDMA_WRITE_FIRST]               = &opa_ib_uc_rcv,
	[IB_OPCODE_UC_RDMA_WRITE_MIDDLE]              = &opa_ib_uc_rcv,
	[IB_OPCODE_UC_RDMA_WRITE_LAST]                = &opa_ib_uc_rcv,
	[IB_OPCODE_UC_RDMA_WRITE_LAST_WITH_IMMEDIATE] = &opa_ib_uc_rcv,
	[IB_OPCODE_UC_RDMA_WRITE_ONLY]                = &opa_ib_uc_rcv,
	[IB_OPCODE_UC_RDMA_WRITE_ONLY_WITH_IMMEDIATE] = &opa_ib_uc_rcv,
	/* UD */
	[IB_OPCODE_UD_SEND_ONLY]                      = &opa_ib_ud_rcv,
	[IB_OPCODE_UD_SEND_ONLY_WITH_IMMEDIATE]       = &opa_ib_ud_rcv,
	/* CNP */
	[CNP_OPCODE]				      = &opa_ib_cnp_rcv
};

/**
 * opa_ib_copy_sge - copy data to SGE memory
 * @ss: the SGE state
 * @data: the data to copy
 * @length: the length of the data
 */
void opa_ib_copy_sge(struct opa_ib_sge_state *ss, void *data, u32 length,
		     int release)
{
	struct ib_sge *sge = &ss->sge;

	while (length) {
		u32 len = sge->length;

		if (len > length)
			len = length;
		BUG_ON(len == 0);
		if (data) {
			memcpy((void *)sge->addr, data, len);
			data += len;
		}
		sge->addr += len;
		sge->length -= len;
		if (sge->length == 0) {
			/* FXRTODO - WFR has hfi1_put_mr() here */
			if (--ss->num_sge)
				*sge = *ss->sg_list++;
		}
		/* FXRTODO - WFR has multi-segment stuff here */
		length -= len;
	}
}

/**
 * opa_ib_skip_sge - skip over SGE memory
 * @ss: the SGE state
 * @length: the number of bytes to skip
 */
void opa_ib_skip_sge(struct opa_ib_sge_state *ss, u32 length, int release)
{
	opa_ib_copy_sge(ss, NULL, length, release);
}

/**
 * post_one_send - post one RC, UC, or UD send work request
 * @qp: the QP to post on
 * @wr: the work request to send
 *
 * Return: 0 on success, otherwise returns an errno.
 */
static int post_one_send(struct opa_ib_qp *qp, struct ib_send_wr *wr,
			 int *scheduled)
{
	struct opa_ib_swqe *wqe;
	u32 next;
	int i;
	int j;
	int acc;
	int ret;
	unsigned long flags;
	struct opa_ib_lkey_table *rkt;
	struct opa_ib_pd *pd;
	u8 sc5;
	struct opa_ib_portdata *ibp;

	spin_lock_irqsave(&qp->s_lock, flags);
	ibp = to_opa_ibportdata(qp->ibqp.device, qp->port_num);

	/* Check that state is OK to post send. */
	if (unlikely(!(ib_qp_state_ops[qp->state] & HFI1_POST_SEND_OK)))
		goto bail_inval;

	/* IB spec says that num_sge == 0 is OK. */
	if (wr->num_sge > qp->s_max_sge)
		goto bail_inval;

	/*
	 * Don't allow RDMA reads or atomic operations on UC or
	 * undefined operations.
	 * Make sure buffer is large enough to hold the result for atomics.
	 */
	if (wr->opcode == IB_WR_FAST_REG_MR) {
		if (opa_ib_fast_reg_mr(qp, wr))
			goto bail_inval;
	} else if (qp->ibqp.qp_type == IB_QPT_UC) {
		if ((unsigned) wr->opcode >= IB_WR_RDMA_READ)
			goto bail_inval;
	} else if (qp->ibqp.qp_type != IB_QPT_RC) {
		/* Check IB_QPT_SMI, IB_QPT_GSI, IB_QPT_UD opcode */
		if (wr->opcode != IB_WR_SEND &&
		    wr->opcode != IB_WR_SEND_WITH_IMM)
			goto bail_inval;
		/* Check UD destination address PD */
		if (qp->ibqp.pd != wr->wr.ud.ah->pd)
			goto bail_inval;
	} else if ((unsigned) wr->opcode > IB_WR_ATOMIC_FETCH_AND_ADD)
		goto bail_inval;
	else if (wr->opcode >= IB_WR_ATOMIC_CMP_AND_SWP &&
		   (wr->num_sge == 0 ||
		    wr->sg_list[0].length < sizeof(u64) ||
		    wr->sg_list[0].addr & (sizeof(u64) - 1)))
		goto bail_inval;
	else if (wr->opcode >= IB_WR_RDMA_READ && !qp->s_max_rd_atomic)
		goto bail_inval;

	next = qp->s_head + 1;
	if (next >= qp->s_size)
		next = 0;
	if (next == qp->s_last) {
		ret = -ENOMEM;
		goto bail;
	}

	rkt = &to_opa_ibdata(qp->ibqp.device)->lk_table;
	pd = to_opa_ibpd(qp->ibqp.pd);
	wqe = get_swqe_ptr(qp, qp->s_head);
	wqe->wr = *wr;
	wqe->length = 0;
	j = 0;
	if (wr->num_sge) {
		acc = wr->opcode >= IB_WR_RDMA_READ ?
			IB_ACCESS_LOCAL_WRITE : 0;
		for (i = 0; i < wr->num_sge; i++) {
			u32 length = wr->sg_list[i].length;
			int ok;

			if (length == 0)
				continue;
			ok = opa_ib_lkey_ok(rkt, pd, &wqe->sg_list[j],
					    &wr->sg_list[i], acc);
			if (!ok)
				goto bail_inval_free;
			wqe->length += length;
			j++;
		}
		wqe->wr.num_sge = j;
	}
	if (qp->ibqp.qp_type == IB_QPT_UC ||
	    qp->ibqp.qp_type == IB_QPT_RC) {
		if (wqe->length > 0x80000000U)
			goto bail_inval_free;
		sc5 = ibp->sl_to_sc[qp->remote_ah_attr.sl];
	} else {
		struct opa_ib_ah *ah = to_opa_ibah(wr->wr.ud.ah);
#if 0
		/* FXRTODO - Skip MTU check till FM is enabled */
		u8 vl;

		sc5 = ibp->sl_to_sc[ah->attr.sl];
		vl = ibp->sc_to_vl[sc5];
		if (vl < OPA_MAX_VLS)
			if (wqe->length > ibp->vl_mtu[vl])
				goto bail_inval_free;
#endif

		atomic_inc(&ah->refcount);
	}
	wqe->ssn = qp->s_ssn++;
	qp->s_head = next;

	ret = 0;
	goto bail;

bail_inval_free:
	/* FXRTODO - WFR has hfi1_put_mr() loop here */
bail_inval:
	ret = -EINVAL;
bail:
	if (!ret && !wr->next) {
		/*
		 * FXRTODO - revisit versus WFR code.
		 * This appears to kick the workqueue if other prior Verbs
		 * requests are still pending, versus returning which will
		 * execute the work queue function directly in this process.
		 * WFR will test if the single SDMA engine selected by
		 * the SC is busy, we currently use one TX Command Queue
		 * so cannot do the more narrow check..
		 */
		if (is_iowait_sdma_busy(&qp->s_iowait)) {
			opa_ib_schedule_send(qp);
			*scheduled = 1;
		}
	}

	spin_unlock_irqrestore(&qp->s_lock, flags);
	return ret;
}

/**
 * opa_ib_post_send - post a send on a QP
 * @ibqp: the QP to post the send on
 * @wr: the list of work requests to post
 * @bad_wr: the first bad WR is put here
 *
 * This may be called from interrupt context.
 *
 * Return: 0 on success, otherwise returns an errno.
 */
int opa_ib_post_send(struct ib_qp *ibqp, struct ib_send_wr *wr,
		     struct ib_send_wr **bad_wr)
{
	struct opa_ib_qp *qp = to_opa_ibqp(ibqp);
	int err = 0;
	int scheduled = 0;

	for (; wr; wr = wr->next) {
		err = post_one_send(qp, wr, &scheduled);
		if (err) {
			*bad_wr = wr;
			goto bail;
		}
	}

	/* Try to do the send work in the caller's context. */
	if (!scheduled)
		opa_ib_do_send(&qp->s_iowait.iowork);

bail:
	return err;
}

/**
 * opa_ib_post_receive - post a receive on a QP
 * @ibqp: the QP to post the receive on
 * @wr: the WR to post
 * @bad_wr: the first bad WR is put here
 *
 * This may be called from interrupt context.
 *
 * Return: 0 on success, otherwise returns an errno.
 */
int opa_ib_post_receive(struct ib_qp *ibqp, struct ib_recv_wr *wr,
			struct ib_recv_wr **bad_wr)
{
	struct opa_ib_qp *qp = to_opa_ibqp(ibqp);
	struct opa_ib_rwq *wq = qp->r_rq.wq;
	unsigned long flags;
	int ret;

	/* Check that state is OK to post receive. */
	if (!(ib_qp_state_ops[qp->state] & HFI1_POST_RECV_OK) || !wq) {
		*bad_wr = wr;
		ret = -EINVAL;
		goto bail;
	}

	for (; wr; wr = wr->next) {
		struct opa_ib_rwqe *wqe;
		u32 next;
		int i;

		if ((unsigned) wr->num_sge > qp->r_rq.max_sge) {
			*bad_wr = wr;
			ret = -EINVAL;
			goto bail;
		}

		spin_lock_irqsave(&qp->r_rq.lock, flags);
		next = wq->head + 1;
		if (next >= qp->r_rq.size)
			next = 0;
		if (next == wq->tail) {
			spin_unlock_irqrestore(&qp->r_rq.lock, flags);
			*bad_wr = wr;
			ret = -ENOMEM;
			goto bail;
		}

		wqe = get_rwqe_ptr(&qp->r_rq, wq->head);
		wqe->wr_id = wr->wr_id;
		wqe->num_sge = wr->num_sge;
		for (i = 0; i < wr->num_sge; i++)
			wqe->sg_list[i] = wr->sg_list[i];
		/* Make sure queue entry is written before the head index. */
		smp_wmb();
		wq->head = next;
		spin_unlock_irqrestore(&qp->r_rq.lock, flags);
	}
	ret = 0;

bail:
	return ret;
}

/*
 * Make sure the QP is ready and able to accept the given opcode.
 */
static inline bool is_qp_ok(struct opa_ib_portdata *ibp,
			    struct opa_ib_qp *qp, int opcode)
{
	if ((ib_qp_state_ops[qp->state] & HFI1_PROCESS_RECV_OK) &&
	    (((opcode & OPCODE_QP_MASK) == qp->allowed_ops) ||
	     (opcode == CNP_OPCODE))) {
		return true;
	} else {
		ibp->n_pkt_drops++;
		return false;
	}
}

/**
 * opa_ib_rcv - process an incoming packet
 * @packet: data packet information
 *
 * This is called to process an incoming packet at interrupt level.
 * FXRTODO: for FXR, this is called upon EQ event?
 * Tlen is the length of the header + data + CRC in bytes.
 */
void opa_ib_rcv(struct opa_ib_packet *packet)
{
	struct opa_ib_header *hdr = packet->hdr;
	u32 tlen = packet->tlen;
	struct opa_ib_portdata *ibp = packet->ibp;
	struct ib_l4_headers *ohdr;
	struct opa_ib_qp *qp;
	u32 qp_num;
	int lnh;
	u8 opcode;
	u16 lid;

	/* FXRTODO: delete me when opa2_ib is stable */
	BUG_ON(!ibp);

	/* 24 == LRH+BTH+CRC */
	if (unlikely(tlen < 24))
		goto drop;

	/* Check for a valid destination LID (see ch. 7.11.1). */
	lid = be16_to_cpu(hdr->lrh[1]);

	/* Check for GRH */
	lnh = be16_to_cpu(hdr->lrh[0]) & 3;
	if (lnh == HFI1_LRH_BTH)
		ohdr = &hdr->u.oth;
	else if (lnh == HFI1_LRH_GRH) {
		u32 vtf;

		ohdr = &hdr->u.l.oth;
		if (hdr->u.l.grh.next_hdr != IB_GRH_NEXT_HDR)
			goto drop;
		vtf = be32_to_cpu(hdr->u.l.grh.version_tclass_flow);
		if ((vtf >> IB_GRH_VERSION_SHIFT) != IB_GRH_VERSION)
			goto drop;
	} else
		goto drop;

	opcode = (be32_to_cpu(ohdr->bth[0]) >> 24) & 0x7f;
#if 0	/* TODO */
	trace_input_ibhdr(rcd->devdata, hdr);
	inc_opstats(tlen, &rcd->opstats->stats[opcode]);
#endif

	/* Get the destination QP number. */
	qp_num = be32_to_cpu(ohdr->bth[1]) & HFI1_QPN_MASK;
#if 0 /* FXRTODO - mcast */
	if ((lid >= HFI1_MULTICAST_LID_BASE) &&
	    (lid != HFI1_PERMISSIVE_LID)) {
		struct qib_mcast *mcast;
		struct qib_mcast_qp *p;

		if (lnh != HFI1_LRH_GRH)
			goto drop;
		mcast = opa_ib_mcast_find(ibp, &hdr->u.l.grh.dgid);
		if (mcast == NULL)
			goto drop;
		packet->rcv_flags |= HFI1_HAS_GRH;
		if (rhf_dc_info(packet->rhf))
			packet->rcv_flags |= HFI1_SC4_BIT;
		list_for_each_entry_rcu(p, &mcast->qp_list, list) {
			spin_lock(&qp->r_lock);
			if (likely((is_qp_ok(ibp, qp, opcode))))
				opcode_handler_tbl[opcode](qp, packet);
			spin_unlock(&qp->r_lock);
		}
		/*
		 * Notify multicast_detach() if it is waiting for us
		 * to finish.
		 */
		if (atomic_dec_return(&mcast->refcount) <= 1)
			wake_up(&mcast->wait);
	} else
#endif
	{
		rcu_read_lock();
		qp = opa_ib_lookup_qpn(ibp, qp_num);
		if (!qp) {
			rcu_read_unlock();
			goto drop;
		}

		if (lnh == HFI1_LRH_GRH)
			packet->rcv_flags |= HFI1_HAS_GRH;
#if 0
		if (rhf_dc_info(packet->rhf))
			packet->rcv_flags |= HFI1_SC4_BIT;
#endif
		spin_lock(&qp->r_lock);
		if (likely((is_qp_ok(ibp, qp, opcode))))
			opcode_handler_tbl[opcode](qp, packet);
		spin_unlock(&qp->r_lock);
		rcu_read_unlock();
	}
	return;

drop:
	ibp->n_pkt_drops++;
}

static void opa_ib_cnp_rcv(struct opa_ib_qp *qp, struct opa_ib_packet *packet)
{
	struct opa_ib_portdata *ibp = packet->ibp;

	if (qp->ibqp.qp_type == IB_QPT_UC)
		opa_ib_uc_rcv(qp, packet);
	else if (qp->ibqp.qp_type == IB_QPT_UD)
		opa_ib_ud_rcv(qp, packet);
	else
		ibp->n_pkt_drops++;
}
