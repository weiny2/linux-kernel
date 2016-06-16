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

#include <linux/io.h>
#include "verbs.h"
#include "packet.h"

/* cut down ridiculously long IB macro names */
#define OP(x) IB_OPCODE_RC_##x

/* AETH NAK opcode values */
#define IB_RNR_NAK                      0x20
#define IB_NAK_PSN_ERROR                0x60
#define IB_NAK_INVALID_REQUEST          0x61
#define IB_NAK_REMOTE_ACCESS_ERROR      0x62
#define IB_NAK_REMOTE_OPERATIONAL_ERROR 0x63
#define IB_NAK_INVALID_RD_REQUEST       0x64

#define HFI1_PSN_CREDIT  16

#ifdef HFI_VERBS_TEST
#define _UC(x) IB_OPCODE_UC_##x
#define _UD(x) IB_OPCODE_UD_##x

static const char opcode_to_str[256][256] = {
	/* RC */
	[OP(SEND_FIRST)]                     = "RC_SEND_FIRST",
	[OP(SEND_MIDDLE)]                    = "RC_SEND_MIDDLE",
	[OP(SEND_LAST)]                      = "RC_SEND_LAST",
	[OP(SEND_LAST_WITH_IMMEDIATE)]       = "RC_SEND_LAST_WITH_IMMEDIATE",
	[OP(SEND_ONLY)]                      = "RC_SEND_ONLY",
	[OP(SEND_ONLY_WITH_IMMEDIATE)]       = "RC_SEND_ONLY_WITH_IMMEDIATE",
	[OP(RDMA_WRITE_FIRST)]               = "RC_RDMA_WRITE_FIRST",
	[OP(RDMA_WRITE_MIDDLE)]              = "RC_RDMA_WRITE_MIDDLE",
	[OP(RDMA_WRITE_LAST)]                = "RC_RDMA_WRITE_LAST",
	[OP(RDMA_WRITE_LAST_WITH_IMMEDIATE)] =
				"RC_RDMA_WRITE_LAST_WITH_IMMEDIATE",
	[OP(RDMA_WRITE_ONLY)]                = "RC_RDMA_WRITE_ONLY",
	[OP(RDMA_WRITE_ONLY_WITH_IMMEDIATE)] =
				"RC_RDMA_WRITE_ONLY_WITH_IMMEDIATE",
	[OP(RDMA_READ_REQUEST)]              = "RC_RDMA_READ_REQUEST",
	[OP(RDMA_READ_RESPONSE_FIRST)]       = "RC_RDMA_READ_RESPONSE_FIRST",
	[OP(RDMA_READ_RESPONSE_MIDDLE)]      = "RC_RDMA_READ_RESPONSE_MIDDLE",
	[OP(RDMA_READ_RESPONSE_LAST)]        = "RC_RDMA_READ_RESPONSE_LAST",
	[OP(RDMA_READ_RESPONSE_ONLY)]        = "RC_RDMA_READ_RESPONSE_ONLY",
	[OP(ACKNOWLEDGE)]                    = "RC_ACKNOWLEDGE",
	[OP(ATOMIC_ACKNOWLEDGE)]             = "RC_ATOMIC_ACKNOWLEDGE",
	[OP(COMPARE_SWAP)]                   = "RC_COMPARE_SWAP",
	[OP(FETCH_ADD)]                      = "RC_FETCH_ADD",
	/* UC */
	[_UC(SEND_FIRST)]                     = "UC_SEND_FIRST",
	[_UC(SEND_MIDDLE)]                    = "UC_SEND_MIDDLE",
	[_UC(SEND_LAST)]                      = "UC_SEND_LAST",
	[_UC(SEND_LAST_WITH_IMMEDIATE)]       = "UC_SEND_LAST_WITH_IMMEDIATE",
	[_UC(SEND_ONLY)]                      = "UC_SEND_ONLY",
	[_UC(SEND_ONLY_WITH_IMMEDIATE)]       = "UC_SEND_ONLY_WITH_IMMEDIATE",
	[_UC(RDMA_WRITE_FIRST)]               = "UC_RDMA_WRITE_FIRST",
	[_UC(RDMA_WRITE_MIDDLE)]              = "UC_RDMA_WRITE_MIDDLE",
	[_UC(RDMA_WRITE_LAST)]                = "UC_RDMA_WRITE_LAST",
	[_UC(RDMA_WRITE_LAST_WITH_IMMEDIATE)] =
					"UC_RDMA_WRITE_LAST_WITH_IMMEDIATE",
	[_UC(RDMA_WRITE_ONLY)]                = "UC_RDMA_WRITE_ONLY",
	[_UC(RDMA_WRITE_ONLY_WITH_IMMEDIATE)] =
					"UC_RDMA_WRITE_ONLY_WITH_IMMEDIATE",
	/* UD */
	[_UD(SEND_ONLY)]                      = "UD_SEND_ONLY",
	[_UD(SEND_ONLY_WITH_IMMEDIATE)]       = "UD_SEND_ONLY_WITH_IMMEDIATE",
	/* CNP */
	[CNP_OPCODE]			      = "CNP_OPCODE"
};
#endif

/*
 * Convert the AETH credit code into the number of credits.
 */
static const u16 credit_table[31] = {
	0,                      /* 0 */
	1,                      /* 1 */
	2,                      /* 2 */
	3,                      /* 3 */
	4,                      /* 4 */
	6,                      /* 5 */
	8,                      /* 6 */
	12,                     /* 7 */
	16,                     /* 8 */
	24,                     /* 9 */
	32,                     /* A */
	48,                     /* B */
	64,                     /* C */
	96,                     /* D */
	128,                    /* E */
	192,                    /* F */
	256,                    /* 10 */
	384,                    /* 11 */
	512,                    /* 12 */
	768,                    /* 13 */
	1024,                   /* 14 */
	1536,                   /* 15 */
	2048,                   /* 16 */
	3072,                   /* 17 */
	4096,                   /* 18 */
	6144,                   /* 19 */
	8192,                   /* 1A */
	12288,                  /* 1B */
	16384,                  /* 1C */
	24576,                  /* 1D */
	32768                   /* 1E */
};

static void rc_timeout(unsigned long arg);

/**
 * compute_aeth - compute the AETH (syndrome + MSN)
 * @qp: the queue pair to compute the AETH for
 *
 * Returns the AETH.
 */
static __be32 compute_aeth(struct rvt_qp *qp)
{
	u32 aeth = qp->r_msn & HFI1_MSN_MASK;

	if (qp->ibqp.srq) {
		/*
		 * Shared receive queues don't generate credits.
		 * Set the credit field to the invalid value.
		 */
		aeth |= HFI1_AETH_CREDIT_INVAL << HFI1_AETH_CREDIT_SHIFT;
	} else {
		u32 min, max, x;
		u32 credits;
		struct rvt_rwq *wq = qp->r_rq.wq;
		u32 head;
		u32 tail;

		/* sanity check pointers before trusting them */
		head = wq->head;
		if (head >= qp->r_rq.size)
			head = 0;
		tail = wq->tail;
		if (tail >= qp->r_rq.size)
			tail = 0;
		/*
		 * Compute the number of credits available (RWQEs).
		 * There is a small chance that the pair of reads are
		 * not atomic, which is OK, since the fuzziness is
		 * resolved as further ACKs go out.
		 */
		credits = head - tail;
		if ((int)credits < 0)
			credits += qp->r_rq.size;
		/*
		 * Binary search the credit table to find the code to
		 * use.
		 */
		min = 0;
		max = 31;
		for (;;) {
			x = (min + max) / 2;
			if (credit_table[x] == credits)
				break;
			if (credit_table[x] > credits)
				max = x;
			else if (min == x)
				break;
			else
				min = x;
		}
		aeth |= x << HFI1_AETH_CREDIT_SHIFT;
	}
	return cpu_to_be32(aeth);
}

/**
 * get_credit - flush the send work queue of a QP
 * @qp: the qp who's send work queue to flush
 * @aeth: the Acknowledge Extended Transport Header
 *
 * The QP s_lock should be held.
 */
static void get_credit(struct rvt_qp *qp, u32 aeth)
{
	u32 credit = (aeth >> HFI1_AETH_CREDIT_SHIFT) & HFI1_AETH_CREDIT_MASK;

	/*
	 * If the credit is invalid, we can send
	 * as many packets as we like.  Otherwise, we have to
	 * honor the credit field.
	 */
	if (credit == HFI1_AETH_CREDIT_INVAL) {
		if (!(qp->s_flags & RVT_S_UNLIMITED_CREDIT)) {
			qp->s_flags |= RVT_S_UNLIMITED_CREDIT;
			if (qp->s_flags & RVT_S_WAIT_SSN_CREDIT) {
				qp->s_flags &= ~RVT_S_WAIT_SSN_CREDIT;
				hfi2_schedule_send(qp);
			}
		}
	} else if (!(qp->s_flags & RVT_S_UNLIMITED_CREDIT)) {
		/* Compute new LSN (i.e., MSN + credit) */
		credit = (aeth + credit_table[credit]) & HFI1_MSN_MASK;
		if (cmp_msn(credit, qp->s_lsn) > 0) {
			qp->s_lsn = credit;
			if (qp->s_flags & RVT_S_WAIT_SSN_CREDIT) {
				qp->s_flags &= ~RVT_S_WAIT_SSN_CREDIT;
				hfi2_schedule_send(qp);
			}
		}
	}
}

static void qp_comm_est(struct rvt_qp *qp)
{
	qp->r_flags |= RVT_R_COMM_EST;
	if (qp->ibqp.event_handler) {
		struct ib_event ev;

		ev.device = qp->ibqp.device;
		ev.element.qp = &qp->ibqp;
		ev.event = IB_EVENT_COMM_EST;
		qp->ibqp.event_handler(&ev, qp->ibqp.qp_context);
	}
}

static u32 restart_sge(struct rvt_sge_state *ss, struct rvt_swqe *wqe,
		       u32 psn, u32 pmtu)
{
	u32 len;

	len = delta_psn(psn, wqe->psn) * pmtu;
	ss->sge = wqe->sg_list[0];
	ss->sg_list = wqe->sg_list + 1;
	ss->num_sge = wqe->wr.num_sge;
	ss->total_len = wqe->length;
	hfi2_skip_sge(ss, len, 0);
	return wqe->length - len;
}

static void start_timer(struct rvt_qp *qp)
{
	qp->s_flags |= RVT_S_TIMER;
	qp->s_timer.function = rc_timeout;
	/* 4.096 usec. * (1 << qp->timeout) */
	qp->s_timer.expires = jiffies + qp->timeout_jiffies;
	add_timer(&qp->s_timer);
}

/**
 * make_rc_ack - construct a response packet (ACK, NAK, or RDMA read)
 * @dev: the device for this QP
 * @qp: a pointer to the QP
 * @ohdr: a pointer to the IB header being constructed
 * @pmtu: the path MTU
 * @use_16b: is a 16B packet
 *
 * Return 1 if constructed; otherwise, return 0.
 * Note that we are in the responder's side of the QP context.
 * Note the QP s_lock must be held.
 */
static int make_rc_ack(struct hfi2_ibdev *dev, struct rvt_qp *qp,
		       struct ib_l4_headers *ohdr, u32 pmtu, bool use_16b)
{
	struct rvt_ack_entry *e;
	u32 hwords;
	u32 len;
	u32 bth0;
	u32 bth2;

	/* Don't send an ACK if we aren't supposed to. */
	if (!(ib_rvt_state_ops[qp->state] & RVT_PROCESS_RECV_OK))
		goto bail;

	/* 16B(4)/LRH(2) + BTH(3) */
	hwords = use_16b ? 7 : 5;

	switch (qp->s_ack_state) {
	case OP(RDMA_READ_RESPONSE_LAST):
	case OP(RDMA_READ_RESPONSE_ONLY):
		e = &qp->s_ack_queue[qp->s_tail_ack_queue];
		if (e->rdma_sge.mr) {
			rvt_put_mr(e->rdma_sge.mr);
			e->rdma_sge.mr = NULL;
		}
		/* FALLTHROUGH */
	case OP(ATOMIC_ACKNOWLEDGE):
		/*
		 * We can increment the tail pointer now that the last
		 * response has been sent instead of only being
		 * constructed.
		 */
		if (++qp->s_tail_ack_queue > OPA_IB_MAX_RDMA_ATOMIC)
			qp->s_tail_ack_queue = 0;
		/* FALLTHROUGH */
	case OP(SEND_ONLY):
	case OP(ACKNOWLEDGE):
		/* Check for no next entry in the queue. */
		if (qp->r_head_ack_queue == qp->s_tail_ack_queue) {
			if (qp->s_flags & RVT_S_ACK_PENDING)
				goto normal;
			goto bail;
		}

		e = &qp->s_ack_queue[qp->s_tail_ack_queue];
		if (e->opcode == OP(RDMA_READ_REQUEST)) {
			/*
			 * If a RDMA read response is being resent and
			 * we haven't seen the duplicate request yet,
			 * then stop sending the remaining responses the
			 * responder has seen until the requester re-sends it.
			 */
			len = e->rdma_sge.sge_length;
			if (len && !e->rdma_sge.mr) {
				qp->s_tail_ack_queue = qp->r_head_ack_queue;
				goto bail;
			}
			/* Copy SGE state in case we need to resend */
			qp->s_rdma_mr = e->rdma_sge.mr;
			if (qp->s_rdma_mr)
				rvt_get_mr(qp->s_rdma_mr);
			qp->s_ack_rdma_sge.sge = e->rdma_sge;
			qp->s_ack_rdma_sge.num_sge = 1;
			qp->s_cur_sge = &qp->s_ack_rdma_sge;
			if (len > pmtu) {
				len = pmtu;
				qp->s_ack_state = OP(RDMA_READ_RESPONSE_FIRST);
			} else {
				qp->s_ack_state = OP(RDMA_READ_RESPONSE_ONLY);
				e->sent = 1;
			}
			ohdr->u.aeth = compute_aeth(qp);
			hwords++;
			qp->s_ack_rdma_psn = e->psn;
			bth2 = mask_psn(qp->s_ack_rdma_psn++);
		} else {
			/* COMPARE_SWAP or FETCH_ADD */
			qp->s_cur_sge = NULL;
			len = 0;
			qp->s_ack_state = OP(ATOMIC_ACKNOWLEDGE);
			ohdr->u.at.aeth = compute_aeth(qp);
			ohdr->u.at.atomic_ack_eth[0] =
				cpu_to_be32(e->atomic_data >> 32);
			ohdr->u.at.atomic_ack_eth[1] =
				cpu_to_be32(e->atomic_data);
			hwords += sizeof(ohdr->u.at) / sizeof(u32);
			bth2 = mask_psn(e->psn);
			e->sent = 1;
		}
		bth0 = qp->s_ack_state << 24;
		break;

	case OP(RDMA_READ_RESPONSE_FIRST):
		qp->s_ack_state = OP(RDMA_READ_RESPONSE_MIDDLE);
		/* FALLTHROUGH */
	case OP(RDMA_READ_RESPONSE_MIDDLE):
		qp->s_cur_sge = &qp->s_ack_rdma_sge;
		qp->s_rdma_mr = qp->s_ack_rdma_sge.sge.mr;
		if (qp->s_rdma_mr)
			rvt_get_mr(qp->s_rdma_mr);
		len = qp->s_ack_rdma_sge.sge.sge_length;
		if (len > pmtu) {
			len = pmtu;
		} else {
			ohdr->u.aeth = compute_aeth(qp);
			hwords++;
			qp->s_ack_state = OP(RDMA_READ_RESPONSE_LAST);
			e = &qp->s_ack_queue[qp->s_tail_ack_queue];
			e->sent = 1;
		}
		bth0 = qp->s_ack_state << 24;
		bth2 = mask_psn(qp->s_ack_rdma_psn++);
		break;

	default:
normal:
		/*
		 * Send a regular ACK.
		 * Set the s_ack_state so we wait until after sending
		 * the ACK before setting s_ack_state to ACKNOWLEDGE
		 * (see above).
		 */
		qp->s_ack_state = OP(SEND_ONLY);
		qp->s_flags &= ~RVT_S_ACK_PENDING;
		qp->s_cur_sge = NULL;
		if (qp->s_nak_state)
			ohdr->u.aeth =
				cpu_to_be32((qp->r_msn & HFI1_MSN_MASK) |
					    (qp->s_nak_state <<
					     HFI1_AETH_CREDIT_SHIFT));
		else
			ohdr->u.aeth = compute_aeth(qp);
		hwords++;
		len = 0;
		bth0 = OP(ACKNOWLEDGE) << 24;
		bth2 = mask_psn(qp->s_ack_psn);
	}
	qp->s_rdma_ack_cnt++;
	qp->s_hdrwords = hwords;
	qp->s_cur_size = len;
	/* hfi2_send_wqe() requires this set to NULL for RC response */
	qp->s_wqe = NULL;
	if (use_16b)
		hfi2_make_16b_ruc_header(qp, ohdr, bth0, bth2);
	else
		hfi2_make_ruc_header(qp, ohdr, bth0, bth2);

	return 1;

bail:
	qp->s_ack_state = OP(ACKNOWLEDGE);
	/*
	 * Ensure s_rdma_ack_cnt changes are committed prior to resetting
	 * RVT_S_RESP_PENDING
	 */
	smp_wmb();
	qp->s_flags &= ~(RVT_S_RESP_PENDING
				| RVT_S_ACK_PENDING);
	return 0;
}

/**
 * hfi2_make_rc_req - construct a request packet (SEND, RDMA r/w, ATOMIC)
 * @qp: a pointer to the QP
 *
 * Return 1 if constructed; otherwise, return 0.
 */
int hfi2_make_rc_req(struct rvt_qp *qp)
{
	struct hfi2_qp_priv *qp_priv = qp->priv;
	struct hfi2_ibdev *dev = to_hfi_ibd(qp->ibqp.device);
	struct ib_l4_headers *ohdr;
	struct rvt_sge_state *ss;
	struct rvt_swqe *wqe;
	/* header size in 32-bit words LRH+BTH = (8+12)/4. */
	u32 hwords;
	u32 len;
	u32 bth0 = 0;
	u32 bth2;
	u32 pmtu = qp->pmtu;
	char newreq;
	unsigned long flags;
	int ret = 0;
	int delta;
	bool use_16b;

	use_16b = qp_priv->use_16b;
	/* 16B(4)/LRH(2) + BTH(3) */
	hwords = use_16b ? 7 : 5;

	/*
	 * FXRTODO: Later, we need to make grh for 16B packets
	 * going across the network based on hop_count.
	 */
	if (qp->remote_ah_attr.ah_flags & IB_AH_GRH)
		ohdr = use_16b ? &qp_priv->s_hdr->opa16b.u.oth :
				 &qp_priv->s_hdr->ph.ibh.u.l.oth;
	else
		ohdr = use_16b ? &qp_priv->s_hdr->opa16b.u.oth :
				 &qp_priv->s_hdr->ph.ibh.u.oth;

	/*
	 * The lock is needed to synchronize between the sending tasklet,
	 * the receive interrupt handler, and timeout re-sends.
	 */
	spin_lock_irqsave(&qp->s_lock, flags);

	/* Sending responses has higher priority over sending requests. */
	if ((qp->s_flags & RVT_S_RESP_PENDING) &&
	    make_rc_ack(dev, qp, ohdr, pmtu, use_16b))
		goto done;

	if (!(ib_rvt_state_ops[qp->state] & RVT_PROCESS_SEND_OK)) {
		if (!(ib_rvt_state_ops[qp->state] & RVT_FLUSH_SEND))
			goto bail;
		/* We are in the error state, flush the work request. */
		if (qp->s_last == qp->s_head)
			goto bail;
		/* If DMAs are in progress, we can't flush immediately. */
		if (atomic_read(&qp_priv->s_iowait.sdma_busy)) {
			qp->s_flags |= RVT_S_WAIT_DMA;
			goto bail;
		}
		wqe = rvt_get_swqe_ptr(qp, qp->s_last);
		hfi2_send_complete(qp, wqe, qp->s_last != qp->s_acked ?
			IB_WC_SUCCESS : IB_WC_WR_FLUSH_ERR);
		/* will get called again */
		goto done;
	}

	if (qp->s_flags & (RVT_S_WAIT_RNR | RVT_S_WAIT_ACK))
		goto bail;

	if (cmp_psn(qp->s_psn, qp->s_sending_hpsn) <= 0) {
		if (cmp_psn(qp->s_sending_psn, qp->s_sending_hpsn) <= 0) {
			qp->s_flags |= RVT_S_WAIT_PSN;
			goto bail;
		}
		qp->s_sending_psn = qp->s_psn;
		qp->s_sending_hpsn = qp->s_psn - 1;
	}

	/* Send a request. */
	wqe = rvt_get_swqe_ptr(qp, qp->s_cur);
	switch (qp->s_state) {
	default:
		if (!(ib_rvt_state_ops[qp->state] & RVT_PROCESS_NEXT_SEND_OK))
			goto bail;
		/*
		 * Resend an old request or start a new one.
		 *
		 * We keep track of the current SWQE so that
		 * we don't reset the "furthest progress" state
		 * if we need to back up.
		 */
		newreq = 0;
		if (qp->s_cur == qp->s_tail) {
			/* Check if send work queue is empty. */
			if (qp->s_tail == qp->s_head)
				goto bail;
			/*
			 * If a fence is requested, wait for previous
			 * RDMA read and atomic operations to finish.
			 */
			if ((wqe->wr.send_flags & IB_SEND_FENCE) &&
			    qp->s_num_rd_atomic) {
				qp->s_flags |= RVT_S_WAIT_FENCE;
				goto bail;
			}
			wqe->psn = qp->s_next_psn;
			newreq = 1;
		}
		/*
		 * Note that we have to be careful not to modify the
		 * original work request since we may need to resend
		 * it.
		 */
		len = wqe->length;
		ss = &qp->s_sge;
		bth2 = mask_psn(qp->s_psn);
		switch (wqe->wr.opcode) {
		case IB_WR_SEND:
		case IB_WR_SEND_WITH_IMM:
			/* If no credit, return. */
			if (!(qp->s_flags & RVT_S_UNLIMITED_CREDIT) &&
			    cmp_msn(wqe->ssn, qp->s_lsn + 1) > 0) {
				qp->s_flags |= RVT_S_WAIT_SSN_CREDIT;
				goto bail;
			}
			wqe->lpsn = wqe->psn;
			if (len > pmtu) {
				wqe->lpsn += (len - 1) / pmtu;
				qp->s_state = OP(SEND_FIRST);
				len = pmtu;
				break;
			}
			if (wqe->wr.opcode == IB_WR_SEND)
				qp->s_state = OP(SEND_ONLY);
			else {
				qp->s_state = OP(SEND_ONLY_WITH_IMMEDIATE);
				/* Immediate data comes after the BTH */
				ohdr->u.imm_data = wqe->wr.ex.imm_data;
				hwords += 1;
			}
			if (wqe->wr.send_flags & IB_SEND_SOLICITED)
				bth0 |= IB_BTH_SOLICITED;
			bth2 |= IB_BTH_REQ_ACK;
			if (++qp->s_cur == qp->s_size)
				qp->s_cur = 0;
			break;

		case IB_WR_RDMA_WRITE:
			if (newreq && !(qp->s_flags & RVT_S_UNLIMITED_CREDIT))
				qp->s_lsn++;
			/* FALLTHROUGH */
		case IB_WR_RDMA_WRITE_WITH_IMM:
			/* If no credit, return. */
			if (!(qp->s_flags & RVT_S_UNLIMITED_CREDIT) &&
			    cmp_msn(wqe->ssn, qp->s_lsn + 1) > 0) {
				qp->s_flags |= RVT_S_WAIT_SSN_CREDIT;
				goto bail;
			}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)
			ohdr->u.rc.reth.vaddr =
				cpu_to_be64(wqe->rdma_wr.remote_addr);
			ohdr->u.rc.reth.rkey =
				cpu_to_be32(wqe->rdma_wr.rkey);

#else
			ohdr->u.rc.reth.vaddr =
				cpu_to_be64(wqe->wr.wr.rdma.remote_addr);
			ohdr->u.rc.reth.rkey =
				cpu_to_be32(wqe->wr.wr.rdma.rkey);
#endif
			ohdr->u.rc.reth.length = cpu_to_be32(len);
			hwords += sizeof(struct ib_reth) / sizeof(u32);
			wqe->lpsn = wqe->psn;
			if (len > pmtu) {
				wqe->lpsn += (len - 1) / pmtu;
				qp->s_state = OP(RDMA_WRITE_FIRST);
				len = pmtu;
				break;
			}
			if (wqe->wr.opcode == IB_WR_RDMA_WRITE)
				qp->s_state = OP(RDMA_WRITE_ONLY);
			else {
				qp->s_state =
					OP(RDMA_WRITE_ONLY_WITH_IMMEDIATE);
				/* Immediate data comes after RETH */
				ohdr->u.rc.imm_data = wqe->wr.ex.imm_data;
				hwords += 1;
				if (wqe->wr.send_flags & IB_SEND_SOLICITED)
					bth0 |= IB_BTH_SOLICITED;
			}
			bth2 |= IB_BTH_REQ_ACK;
			if (++qp->s_cur == qp->s_size)
				qp->s_cur = 0;
			break;

		case IB_WR_RDMA_READ:
			/*
			 * Don't allow more operations to be started
			 * than the QP limits allow.
			 */
			if (newreq) {
				if (qp->s_num_rd_atomic >=
				    qp->s_max_rd_atomic) {
					qp->s_flags |= RVT_S_WAIT_RDMAR;
					goto bail;
				}
				qp->s_num_rd_atomic++;
				if (!(qp->s_flags & RVT_S_UNLIMITED_CREDIT))
					qp->s_lsn++;
				/*
				 * Adjust s_next_psn to count the
				 * expected number of responses.
				 */
				if (len > pmtu)
					qp->s_next_psn += (len - 1) / pmtu;
				wqe->lpsn = qp->s_next_psn++;
			}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)
			ohdr->u.rc.reth.vaddr =
				cpu_to_be64(wqe->rdma_wr.remote_addr);
			ohdr->u.rc.reth.rkey =
				cpu_to_be32(wqe->rdma_wr.rkey);
#else
			ohdr->u.rc.reth.vaddr =
				cpu_to_be64(wqe->wr.wr.rdma.remote_addr);
			ohdr->u.rc.reth.rkey =
				cpu_to_be32(wqe->wr.wr.rdma.rkey);
#endif
			ohdr->u.rc.reth.length = cpu_to_be32(len);
			qp->s_state = OP(RDMA_READ_REQUEST);
			hwords += sizeof(ohdr->u.rc.reth) / sizeof(u32);
			ss = NULL;
			len = 0;
			bth2 |= IB_BTH_REQ_ACK;
			if (++qp->s_cur == qp->s_size)
				qp->s_cur = 0;
			break;

		case IB_WR_ATOMIC_CMP_AND_SWP:
		case IB_WR_ATOMIC_FETCH_AND_ADD:
			/*
			 * Don't allow more operations to be started
			 * than the QP limits allow.
			 */
			if (newreq) {
				if (qp->s_num_rd_atomic >=
				    qp->s_max_rd_atomic) {
					qp->s_flags |= RVT_S_WAIT_RDMAR;
					goto bail;
				}
				qp->s_num_rd_atomic++;
				if (!(qp->s_flags & RVT_S_UNLIMITED_CREDIT))
					qp->s_lsn++;
				wqe->lpsn = wqe->psn;
			}
			if (wqe->wr.opcode == IB_WR_ATOMIC_CMP_AND_SWP) {
				qp->s_state = OP(COMPARE_SWAP);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)
				ohdr->u.atomic_eth.swap_data = cpu_to_be64(
					wqe->atomic_wr.swap);
				ohdr->u.atomic_eth.compare_data = cpu_to_be64(
					wqe->atomic_wr.compare_add);
#else
				ohdr->u.atomic_eth.swap_data = cpu_to_be64(
					wqe->wr.wr.atomic.swap);
				ohdr->u.atomic_eth.compare_data = cpu_to_be64(
					wqe->wr.wr.atomic.compare_add);
#endif
			} else {
				qp->s_state = OP(FETCH_ADD);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)
				ohdr->u.atomic_eth.swap_data = cpu_to_be64(
					wqe->atomic_wr.compare_add);
#else
				ohdr->u.atomic_eth.swap_data = cpu_to_be64(
					wqe->wr.wr.atomic.compare_add);
#endif
				ohdr->u.atomic_eth.compare_data = 0;
			}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)
			ohdr->u.atomic_eth.vaddr[0] = cpu_to_be32(
				wqe->atomic_wr.remote_addr >> 32);
			ohdr->u.atomic_eth.vaddr[1] = cpu_to_be32(
				wqe->atomic_wr.remote_addr);
			ohdr->u.atomic_eth.rkey = cpu_to_be32(
				wqe->atomic_wr.rkey);
#else
			ohdr->u.atomic_eth.vaddr[0] = cpu_to_be32(
				wqe->wr.wr.atomic.remote_addr >> 32);
			ohdr->u.atomic_eth.vaddr[1] = cpu_to_be32(
				wqe->wr.wr.atomic.remote_addr);
			ohdr->u.atomic_eth.rkey = cpu_to_be32(
				wqe->wr.wr.atomic.rkey);
#endif
			hwords += sizeof(struct ib_atomic_eth) / sizeof(u32);
			ss = NULL;
			len = 0;
			bth2 |= IB_BTH_REQ_ACK;
			if (++qp->s_cur == qp->s_size)
				qp->s_cur = 0;
			break;

		default:
			goto bail;
		}
		qp->s_sge.sge = wqe->sg_list[0];
		qp->s_sge.sg_list = wqe->sg_list + 1;
		qp->s_sge.num_sge = wqe->wr.num_sge;
		qp->s_sge.total_len = wqe->length;
		qp->s_len = wqe->length;
		if (newreq) {
			qp->s_tail++;
			if (qp->s_tail >= qp->s_size)
				qp->s_tail = 0;
		}
		if (wqe->wr.opcode == IB_WR_RDMA_READ)
			qp->s_psn = wqe->lpsn + 1;
		else {
			qp->s_psn++;
			if (cmp_psn(qp->s_psn, qp->s_next_psn) > 0)
				qp->s_next_psn = qp->s_psn;
		}
		break;

	case OP(RDMA_READ_RESPONSE_FIRST):
		/*
		 * qp->s_state is normally set to the opcode of the
		 * last packet constructed for new requests and therefore
		 * is never set to RDMA read response.
		 * RDMA_READ_RESPONSE_FIRST is used by the ACK processing
		 * thread to indicate a SEND needs to be restarted from an
		 * earlier PSN without interfering with the sending thread.
		 * See restart_rc().
		 */
		qp->s_len = restart_sge(&qp->s_sge, wqe, qp->s_psn, pmtu);
		/* FALLTHROUGH */
	case OP(SEND_FIRST):
		qp->s_state = OP(SEND_MIDDLE);
		/* FALLTHROUGH */
	case OP(SEND_MIDDLE):
		bth2 = mask_psn(qp->s_psn++);
		if (cmp_psn(qp->s_psn, qp->s_next_psn) > 0)
			qp->s_next_psn = qp->s_psn;
		ss = &qp->s_sge;
		len = qp->s_len;
		if (len > pmtu) {
			len = pmtu;
			break;
		}
		if (wqe->wr.opcode == IB_WR_SEND)
			qp->s_state = OP(SEND_LAST);
		else {
			qp->s_state = OP(SEND_LAST_WITH_IMMEDIATE);
			/* Immediate data comes after the BTH */
			ohdr->u.imm_data = wqe->wr.ex.imm_data;
			hwords += 1;
		}
		if (wqe->wr.send_flags & IB_SEND_SOLICITED)
			bth0 |= IB_BTH_SOLICITED;
		bth2 |= IB_BTH_REQ_ACK;
		qp->s_cur++;
		if (qp->s_cur >= qp->s_size)
			qp->s_cur = 0;
		break;

	case OP(RDMA_READ_RESPONSE_LAST):
		/*
		 * qp->s_state is normally set to the opcode of the
		 * last packet constructed for new requests and therefore
		 * is never set to RDMA read response.
		 * RDMA_READ_RESPONSE_LAST is used by the ACK processing
		 * thread to indicate a RDMA write needs to be restarted from
		 * an earlier PSN without interfering with the sending thread.
		 * See restart_rc().
		 */
		qp->s_len = restart_sge(&qp->s_sge, wqe, qp->s_psn, pmtu);
		/* FALLTHROUGH */
	case OP(RDMA_WRITE_FIRST):
		qp->s_state = OP(RDMA_WRITE_MIDDLE);
		/* FALLTHROUGH */
	case OP(RDMA_WRITE_MIDDLE):
		bth2 = mask_psn(qp->s_psn++);
		if (cmp_psn(qp->s_psn, qp->s_next_psn) > 0)
			qp->s_next_psn = qp->s_psn;
		ss = &qp->s_sge;
		len = qp->s_len;
		if (len > pmtu) {
			len = pmtu;
			break;
		}
		if (wqe->wr.opcode == IB_WR_RDMA_WRITE)
			qp->s_state = OP(RDMA_WRITE_LAST);
		else {
			qp->s_state = OP(RDMA_WRITE_LAST_WITH_IMMEDIATE);
			/* Immediate data comes after the BTH */
			ohdr->u.imm_data = wqe->wr.ex.imm_data;
			hwords += 1;
			if (wqe->wr.send_flags & IB_SEND_SOLICITED)
				bth0 |= IB_BTH_SOLICITED;
		}
		bth2 |= IB_BTH_REQ_ACK;
		qp->s_cur++;
		if (qp->s_cur >= qp->s_size)
			qp->s_cur = 0;
		break;

	case OP(RDMA_READ_RESPONSE_MIDDLE):
		/*
		 * qp->s_state is normally set to the opcode of the
		 * last packet constructed for new requests and therefore
		 * is never set to RDMA read response.
		 * RDMA_READ_RESPONSE_MIDDLE is used by the ACK processing
		 * thread to indicate a RDMA read needs to be restarted from
		 * an earlier PSN without interfering with the sending thread.
		 * See restart_rc().
		 */
		len = (delta_psn(qp->s_psn, wqe->psn)) * pmtu;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)
		ohdr->u.rc.reth.vaddr =
			cpu_to_be64(wqe->rdma_wr.remote_addr + len);
		ohdr->u.rc.reth.rkey =
			cpu_to_be32(wqe->rdma_wr.rkey);
#else
		ohdr->u.rc.reth.vaddr =
			cpu_to_be64(wqe->wr.wr.rdma.remote_addr + len);
		ohdr->u.rc.reth.rkey =
			cpu_to_be32(wqe->wr.wr.rdma.rkey);
#endif
		ohdr->u.rc.reth.length = cpu_to_be32(wqe->length - len);
		qp->s_state = OP(RDMA_READ_REQUEST);
		hwords += sizeof(ohdr->u.rc.reth) / sizeof(u32);
		bth2 = mask_psn(qp->s_psn) | IB_BTH_REQ_ACK;
		qp->s_psn = wqe->lpsn + 1;
		ss = NULL;
		len = 0;
		qp->s_cur++;
		if (qp->s_cur == qp->s_size)
			qp->s_cur = 0;
		break;
	}
	qp->s_sending_hpsn = bth2;
	delta = delta_psn(bth2, wqe->psn);
	if (delta && delta % HFI1_PSN_CREDIT == 0)
		bth2 |= IB_BTH_REQ_ACK;
	if (qp->s_flags & RVT_S_SEND_ONE) {
		qp->s_flags &= ~RVT_S_SEND_ONE;
		qp->s_flags |= RVT_S_WAIT_ACK;
		bth2 |= IB_BTH_REQ_ACK;
	}
	qp->s_len -= len;
	qp->s_hdrwords = hwords;
	qp->s_cur_sge = ss;
	qp->s_cur_size = len;
	/* hfi2_send_wqe() requires this set */
	qp->s_wqe = wqe;
	bth0 |= (qp->s_state << 24);
	if (use_16b)
		hfi2_make_16b_ruc_header(qp, ohdr, bth0, bth2);
	else
		hfi2_make_ruc_header(qp, ohdr, bth0, bth2);

done:
	ret = 1;
	goto unlock;

bail:
	qp->s_flags &= ~RVT_S_BUSY;
unlock:
	spin_unlock_irqrestore(&qp->s_lock, flags);
	return ret;
}

static u32 hfi2_make_rc_ack_header(struct rvt_qp *qp,
				   struct hfi2_ib_header *hdr,
				   struct ib_l4_headers *ohdr, bool is_fecn)
{
	struct hfi2_ibport *ibp = to_hfi_ibp(qp->ibqp.device, qp->port_num);
	u16 lrh0, sc5;
	u32 bth0, bth1;
	u32 hwords, nwords;

	/* header size in 32-bit words LRH+BTH+AETH = (8+12+4)/4 */
	hwords = 6;
	nwords = SIZE_OF_CRC;
	if (unlikely(qp->remote_ah_attr.ah_flags & IB_AH_GRH)) {
		/* remove LRH size from s_hdrwords for GRH */
		hwords += hfi2_make_grh(ibp, &hdr->u.l.grh,
					&qp->remote_ah_attr.grh,
					(hwords - 2), nwords);
		lrh0 = HFI1_LRH_GRH;
	} else {
		lrh0 = HFI1_LRH_BTH;
	}

	/* read pkey_index w/o lock (its atomic) */
	bth0 = hfi2_get_pkey(ibp, qp->s_pkey_index) | (OP(ACKNOWLEDGE) << 24);
	if (qp->s_mig_state == IB_MIG_MIGRATED)
		bth0 |= IB_BTH_MIG_REQ;

	sc5 = ibp->ppd->sl_to_sc[qp->remote_ah_attr.sl];
	/* TODO set SC4 bit in PTL flags
	 * pbc_flags |= ((!!(sc5 & 0x10)) << PBC_DC_INFO_SHIFT);
	 */
	lrh0 |= (sc5 & 0xf) << 12 | (qp->remote_ah_attr.sl & 0xf) << 4;
	hdr->lrh[0] = cpu_to_be16(lrh0);
	hdr->lrh[1] = cpu_to_be16((u16)qp->remote_ah_attr.dlid);
	hdr->lrh[2] = cpu_to_be16(hwords + nwords);
	hdr->lrh[3] = cpu_to_be16(ibp->ppd->lid |
				  qp->remote_ah_attr.src_path_bits);
	ohdr->bth[0] = cpu_to_be32(bth0);
	bth1 = qp->remote_qpn;
	if (is_fecn)
		bth1 |= (HFI1_BECN_MASK << HFI1_BECN_SHIFT);
	ohdr->bth[1] = cpu_to_be32(bth1);
	ohdr->bth[2] = cpu_to_be32(mask_psn(qp->r_ack_psn));
	return hwords;
}

static u32 hfi2_make_16b_rc_ack_header(struct rvt_qp *qp,
				       struct hfi2_opa16b_header *hdr,
				       struct ib_l4_headers *ohdr, bool is_fecn)
{
	struct hfi2_ibport *ibp = to_hfi_ibp(qp->ibqp.device, qp->port_num);
	u32 hwords, nwords, slid, dlid, qwords;
	u8 sc5, l4, extra_bytes;
	u32 bth0, bth1;
	u16 pkey;

	/* header size in 32-bit words LRH+BTH+AETH = (16+12+4)/4 */
	hwords = 8;

	/* Whole packet (hdr, data, crc, tail byte) should be flit aligned */
	extra_bytes = -((hwords << 2) +	(SIZE_OF_CRC << 2) + 1) & 7;
	nwords = ((SIZE_OF_CRC << 2) + 1 + extra_bytes) >> 2;

	/*
	 * FXRTODO: Later, we need to make grh for 16B packets
	 * going across the network based on hop_count.
	 */
	if (unlikely(qp->remote_ah_attr.ah_flags & IB_AH_GRH & 0)) {
		/* remove LRH size from hwords for GRH */
		hwords += hfi2_make_grh(ibp, &hdr->u.l.grh,
					&qp->remote_ah_attr.grh,
					(hwords - 2), nwords);
		l4 = HFI1_L4_IB_GLOBAL;
	} else {
		l4 = HFI1_L4_IB_LOCAL;
	}

	bth0 = (OP(ACKNOWLEDGE) << 24);
	bth0 |= extra_bytes << 20;
	bth1 = qp->remote_qpn;
	if (qp->s_mig_state == IB_MIG_MIGRATED)
		bth1 |= IB_16B_BTH_MIG_REQ;

	sc5 = ibp->ppd->sl_to_sc[qp->remote_ah_attr.sl];
	dlid = hfi2_retrieve_dlid(qp);
	slid = ibp->ppd->lid | qp->remote_ah_attr.src_path_bits;
	pkey = hfi2_get_pkey(ibp, qp->s_pkey_index);
	qwords = (hwords + nwords) >> 1;

	opa_make_16b_header((u32 *)hdr, slid, dlid, qwords,
			    pkey, 0, sc5, 0, 0, is_fecn, 0, l4);

	ohdr->bth[0] = cpu_to_be32(bth0);
	ohdr->bth[1] = cpu_to_be32(bth1);
	ohdr->bth[2] = cpu_to_be32(mask_psn(qp->r_ack_psn));
	return hwords;
}

/**
 * hfi2_send_rc_ack - Construct an ACK packet and send it
 * @qp: a pointer to the QP
 * @is_fecn: is fecn recived
 *
 * This is called from hfi2_rc_rcv().
 * TODO - HFI1 also calls from handle_receive_interrupt()
 * Note that RDMA reads and atomics are handled in the
 * send side QP state and tasklet.
 */
static void hfi2_send_rc_ack(struct rvt_qp *qp, bool is_fecn)
{
	struct hfi2_ibport *ibp = to_hfi_ibp(qp->ibqp.device, qp->port_num);
	struct hfi2_qp_priv *qp_priv = qp->priv;
	union hfi2_packet_header ph;
	struct ib_l4_headers *ohdr;
	bool use_16b;
	u32 hwords;

	/* Don't send ACK or NAK if a RDMA read or atomic is pending. */
	if (qp->s_flags & RVT_S_RESP_PENDING)
		goto queue_ack;

	/* Ensure s_rdma_ack_cnt changes are committed */
	smp_read_barrier_depends();
	if (qp->s_rdma_ack_cnt)
		goto queue_ack;

	use_16b = qp_priv->use_16b;
	/*
	 * FXRTODO: Later, we need to make grh for 16B packets
	 * going across the network based on hop_count.
	 */
	if (unlikely(qp->remote_ah_attr.ah_flags & IB_AH_GRH))
		ohdr = use_16b ? &ph.opa16b.u.oth : &ph.ibh.u.l.oth;
	else
		ohdr = use_16b ? &ph.opa16b.u.oth : &ph.ibh.u.oth;

	/* Construct the header */
	if (use_16b)
		hwords = hfi2_make_16b_rc_ack_header(qp, &ph.opa16b,
						     ohdr, is_fecn);
	else
		hwords = hfi2_make_rc_ack_header(qp, &ph.ibh, ohdr, is_fecn);

	if (qp->r_nak_state)
		ohdr->u.aeth = cpu_to_be32((qp->r_msn & HFI1_MSN_MASK) |
					    (qp->r_nak_state <<
					     HFI1_AETH_CREDIT_SHIFT));
	else
		ohdr->u.aeth = compute_aeth(qp);

	/* Don't try to send ACKs if the link isn't ACTIVE */
	if (ibp->ppd->lstate != IB_PORT_ACTIVE)
		return;

	dev_dbg(ibp->dev, "Send RC %s for PSN %u\n",
		(qp->r_nak_state) ? "NAK" : "ACK", mask_psn(qp->r_ack_psn));
	ibp->ibd->send_ack(ibp, qp->priv, &ph, hwords);
	return;

queue_ack:
	this_cpu_inc(*ibp->rc_qacks);
	spin_lock(&qp->s_lock);
	qp->s_flags |= RVT_S_ACK_PENDING | RVT_S_RESP_PENDING;
	qp->s_nak_state = qp->r_nak_state;
	qp->s_ack_psn = qp->r_ack_psn;
	if (is_fecn)
		qp->s_flags |= RVT_S_ECN;

	/* Schedule the send tasklet. */
	hfi2_schedule_send(qp);
	spin_unlock(&qp->s_lock);
}

/**
 * reset_psn - reset the QP state to send starting from PSN
 * @qp: the QP
 * @psn: the packet sequence number to restart at
 *
 * This is called from hfi2_rc_rcv() to process an incoming RC ACK
 * for the given QP.
 * Called at interrupt level with the QP s_lock held.
 */
static void reset_psn(struct rvt_qp *qp, u32 psn)
{
	u32 n = qp->s_acked;
	struct rvt_swqe *wqe = rvt_get_swqe_ptr(qp, n);
	u32 opcode;

	qp->s_cur = n;

	/*
	 * If we are starting the request from the beginning,
	 * let the normal send code handle initialization.
	 */
	if (cmp_psn(psn, wqe->psn) <= 0) {
		qp->s_state = OP(SEND_LAST);
		goto done;
	}

	/* Find the work request opcode corresponding to the given PSN. */
	opcode = wqe->wr.opcode;
	for (;;) {
		int diff;

		if (++n == qp->s_size)
			n = 0;
		if (n == qp->s_tail)
			break;
		wqe = rvt_get_swqe_ptr(qp, n);
		diff = cmp_psn(psn, wqe->psn);
		if (diff < 0)
			break;
		qp->s_cur = n;
		/*
		 * If we are starting the request from the beginning,
		 * let the normal send code handle initialization.
		 */
		if (diff == 0) {
			qp->s_state = OP(SEND_LAST);
			goto done;
		}
		opcode = wqe->wr.opcode;
	}

	/*
	 * Set the state to restart in the middle of a request.
	 * Don't change the s_sge, s_cur_sge, or s_cur_size.
	 * See hfi2_make_rc_req().
	 */
	switch (opcode) {
	case IB_WR_SEND:
	case IB_WR_SEND_WITH_IMM:
		qp->s_state = OP(RDMA_READ_RESPONSE_FIRST);
		break;

	case IB_WR_RDMA_WRITE:
	case IB_WR_RDMA_WRITE_WITH_IMM:
		qp->s_state = OP(RDMA_READ_RESPONSE_LAST);
		break;

	case IB_WR_RDMA_READ:
		qp->s_state = OP(RDMA_READ_RESPONSE_MIDDLE);
		break;

	default:
		/*
		 * This case shouldn't happen since its only
		 * one PSN per req.
		 */
		qp->s_state = OP(SEND_LAST);
	}
done:
	qp->s_psn = psn;
	/*
	 * Set RVT_S_WAIT_PSN as rc_complete() may start the timer
	 * asynchronously before the send tasklet can get scheduled.
	 * Doing it in hfi2_make_rc_req() is too late.
	 */
	if ((cmp_psn(qp->s_psn, qp->s_sending_hpsn) <= 0) &&
	    (cmp_psn(qp->s_sending_psn, qp->s_sending_hpsn) <= 0))
		qp->s_flags |= RVT_S_WAIT_PSN;
}

/*
 * Back up requester to resend the last un-ACKed request.
 * The QP r_lock and s_lock should be held and interrupts disabled.
 */
static void restart_rc(struct rvt_qp *qp, u32 psn, int wait)
{
	struct rvt_swqe *wqe = rvt_get_swqe_ptr(qp, qp->s_acked);
	struct hfi2_ibport *ibp;

	if (qp->s_retry == 0) {
		if (qp->s_mig_state == IB_MIG_ARMED) {
			hfi2_migrate_qp(qp);
			qp->s_retry = qp->s_retry_cnt;
		} else if (qp->s_last == qp->s_acked) {
			hfi2_send_complete(qp, wqe, IB_WC_RETRY_EXC_ERR);
			rvt_error_qp(qp, IB_WC_WR_FLUSH_ERR);
			return;
		} else /* need to handle delayed completion */
			return;
	} else
		qp->s_retry--;

	ibp = to_hfi_ibp(qp->ibqp.device, qp->port_num);
	if (wqe->wr.opcode == IB_WR_RDMA_READ)
		ibp->n_rc_resends++;
	else
		ibp->n_rc_resends += delta_psn(qp->s_psn, psn);

	qp->s_flags &= ~(RVT_S_WAIT_FENCE | RVT_S_WAIT_RDMAR |
			 RVT_S_WAIT_SSN_CREDIT | RVT_S_WAIT_PSN |
			 RVT_S_WAIT_ACK);
	if (wait)
		qp->s_flags |= RVT_S_SEND_ONE;
	reset_psn(qp, psn);
}

/*
 * This is called from s_timer for missing responses.
 */
static void rc_timeout(unsigned long arg)
{
	struct rvt_qp *qp = (struct rvt_qp *)arg;
	struct hfi2_ibport *ibp;
	unsigned long flags;

	spin_lock_irqsave(&qp->r_lock, flags);
	spin_lock(&qp->s_lock);
	if (qp->s_flags & RVT_S_TIMER) {
		ibp = to_hfi_ibp(qp->ibqp.device, qp->port_num);
		ibp->n_rc_timeouts++;
		qp->s_flags &= ~RVT_S_TIMER;
		del_timer(&qp->s_timer);
		restart_rc(qp, qp->s_last_psn + 1, 1);
		hfi2_schedule_send(qp);
	}
	spin_unlock(&qp->s_lock);
	spin_unlock_irqrestore(&qp->r_lock, flags);
}

/*
 * This is called from s_timer for RNR timeouts.
 */
static void rc_rnr_retry(unsigned long arg)
{
	struct rvt_qp *qp = (struct rvt_qp *)arg;
	unsigned long flags;

	spin_lock_irqsave(&qp->s_lock, flags);
	if (qp->s_flags & RVT_S_WAIT_RNR) {
		qp->s_flags &= ~RVT_S_WAIT_RNR;
		del_timer(&qp->s_timer);
		hfi2_schedule_send(qp);
	}
	spin_unlock_irqrestore(&qp->s_lock, flags);
}

/*
 * Set qp->s_sending_psn to the next PSN after the given one.
 * This would be psn+1 except when RDMA reads are present.
 */
static void reset_sending_psn(struct rvt_qp *qp, u32 psn)
{
	struct rvt_swqe *wqe;
	u32 n = qp->s_last;

	/* Find the work request corresponding to the given PSN. */
	for (;;) {
		wqe = rvt_get_swqe_ptr(qp, n);
		if (cmp_psn(psn, wqe->lpsn) <= 0) {
			if (wqe->wr.opcode == IB_WR_RDMA_READ)
				qp->s_sending_psn = wqe->lpsn + 1;
			else
				qp->s_sending_psn = psn + 1;
			break;
		}
		if (++n == qp->s_size)
			n = 0;
		if (n == qp->s_tail)
			break;
	}
}

/*
 * This should be called with the QP s_lock held and interrupts disabled.
 */
void hfi2_rc_send_complete(struct rvt_qp *qp, struct ib_l4_headers *ohdr)
{
	struct rvt_swqe *wqe;
	struct ib_wc wc;
	unsigned i;
	u32 opcode;
	u32 psn;

	if (!(ib_rvt_state_ops[qp->state] & RVT_PROCESS_OR_FLUSH_SEND))
		return;

	opcode = be32_to_cpu(ohdr->bth[0]) >> 24;
	if (opcode >= OP(RDMA_READ_RESPONSE_FIRST) &&
	    opcode <= OP(ATOMIC_ACKNOWLEDGE)) {
		WARN_ON(!qp->s_rdma_ack_cnt);
		qp->s_rdma_ack_cnt--;
		return;
	}

	psn = be32_to_cpu(ohdr->bth[2]);
	reset_sending_psn(qp, psn);

	/*
	 * Start timer after a packet requesting an ACK has been sent and
	 * there are still requests that haven't been acked.
	 */
	if ((psn & IB_BTH_REQ_ACK) && qp->s_acked != qp->s_tail &&
	    !(qp->s_flags &
		(RVT_S_TIMER | RVT_S_WAIT_RNR | RVT_S_WAIT_PSN)) &&
		(ib_rvt_state_ops[qp->state] & RVT_PROCESS_RECV_OK))
		start_timer(qp);

	while (qp->s_last != qp->s_acked) {
		wqe = rvt_get_swqe_ptr(qp, qp->s_last);
		if (cmp_psn(wqe->lpsn, qp->s_sending_psn) >= 0 &&
		    cmp_psn(qp->s_sending_psn, qp->s_sending_hpsn) <= 0)
			break;
		for (i = 0; i < wqe->wr.num_sge; i++) {
			struct rvt_sge *sge = &wqe->sg_list[i];

			rvt_put_mr(sge->mr);
		}
		/* Post a send completion queue entry if requested. */
		if (!(qp->s_flags & RVT_S_SIGNAL_REQ_WR) ||
		    (wqe->wr.send_flags & IB_SEND_SIGNALED)) {
			memset(&wc, 0, sizeof(wc));
			wc.wr_id = wqe->wr.wr_id;
			wc.status = IB_WC_SUCCESS;
			wc.opcode = ib_hfi1_wc_opcode[wqe->wr.opcode];
			wc.byte_len = wqe->length;
			wc.qp = &qp->ibqp;
			rvt_cq_enter(ibcq_to_rvtcq(qp->ibqp.send_cq), &wc, 0);
		}
		if (++qp->s_last >= qp->s_size)
			qp->s_last = 0;
	}
	/*
	 * If we were waiting for sends to complete before re-sending,
	 * and they are now complete, restart sending.
	 */
	if (qp->s_flags & RVT_S_WAIT_PSN &&
	    cmp_psn(qp->s_sending_psn, qp->s_sending_hpsn) > 0) {
		qp->s_flags &= ~RVT_S_WAIT_PSN;
		qp->s_sending_psn = qp->s_psn;
		qp->s_sending_hpsn = qp->s_psn - 1;
		hfi2_schedule_send(qp);
	}
}

static inline void update_last_psn(struct rvt_qp *qp, u32 psn)
{
	qp->s_last_psn = psn;
}

/*
 * Generate a SWQE completion.
 * This is similar to hfi2_send_complete but has to check to be sure
 * that the SGEs are not being referenced if the SWQE is being resent.
 */
static struct rvt_swqe *do_rc_completion(struct rvt_qp *qp,
					  struct rvt_swqe *wqe,
					  struct hfi2_ibport *ibp)
{
	struct ib_wc wc;
	unsigned i;

	/*
	 * Don't decrement refcount and don't generate a
	 * completion if the SWQE is being resent until the send
	 * is finished.
	 */
	if (cmp_psn(wqe->lpsn, qp->s_sending_psn) < 0 ||
	    cmp_psn(qp->s_sending_psn, qp->s_sending_hpsn) > 0) {
		for (i = 0; i < wqe->wr.num_sge; i++) {
			struct rvt_sge *sge = &wqe->sg_list[i];

			rvt_put_mr(sge->mr);
		}
		/* Post a send completion queue entry if requested. */
		if (!(qp->s_flags & RVT_S_SIGNAL_REQ_WR) ||
		    (wqe->wr.send_flags & IB_SEND_SIGNALED)) {
			memset(&wc, 0, sizeof(wc));
			wc.wr_id = wqe->wr.wr_id;
			wc.status = IB_WC_SUCCESS;
			wc.opcode = ib_hfi1_wc_opcode[wqe->wr.opcode];
			wc.byte_len = wqe->length;
			wc.qp = &qp->ibqp;
			rvt_cq_enter(ibcq_to_rvtcq(qp->ibqp.send_cq), &wc, 0);
		}
		if (++qp->s_last >= qp->s_size)
			qp->s_last = 0;
	} else {
		this_cpu_inc(*ibp->rc_delayed_comp);
		/*
		 * If send progress not running attempt to progress
		 * SDMA queue.
		 * FXRTODO
		 */
#if 0
		struct sdma_engine *engine;
		u8 sc5;

		/* For now use sc to find engine */
		sc5 = ibp->ppd->sl_to_sc[qp->remote_ah_attr.sl];
		engine = qp_to_sdma_engine(qp, sc5);
		sdma_engine_progress_schedule(engine);
#endif
	}

	qp->s_retry = qp->s_retry_cnt;
	update_last_psn(qp, wqe->lpsn);

	/*
	 * If we are completing a request which is in the process of
	 * being resent, we can stop re-sending it since we know the
	 * responder has already seen it.
	 */
	if (qp->s_acked == qp->s_cur) {
		if (++qp->s_cur >= qp->s_size)
			qp->s_cur = 0;
		qp->s_acked = qp->s_cur;
		wqe = rvt_get_swqe_ptr(qp, qp->s_cur);
		if (qp->s_acked != qp->s_tail) {
			qp->s_state = OP(SEND_LAST);
			qp->s_psn = wqe->psn;
		}
	} else {
		if (++qp->s_acked >= qp->s_size)
			qp->s_acked = 0;
		if (qp->state == IB_QPS_SQD && qp->s_acked == qp->s_cur)
			qp->s_draining = 0;
		wqe = rvt_get_swqe_ptr(qp, qp->s_acked);
	}
	return wqe;
}

static inline void rc_deferred_send(struct hfi_ctx *ctx, struct rvt_qp *qp)
{
	/* FXRTODO: Implement deferred send, but first verify is worthwhile */
#if 0
	if (list_empty(&qp->rspwait)) {
		qp->r_flags |= RVT_R_RSP_SEND;
		atomic_inc(&qp->refcount);
		list_add_tail(&qp->rspwait,
			      &ctx->wait_list);
	}
#else
	/* Schedule the send workqueue */
	hfi2_schedule_send(qp);
#endif
}

/**
 * do_rc_ack - process an incoming RC ACK
 * @qp: the QP the ACK came in on
 * @psn: the packet sequence number of the ACK
 * @opcode: the opcode of the request that resulted in the ACK
 *
 * This is called from rc_rcv_resp() to process an incoming RC ACK
 * for the given QP.
 * Called at interrupt level with the QP s_lock held.
 * Returns 1 if OK, 0 if current operation should be aborted (NAK).
 */
static int do_rc_ack(struct rvt_qp *qp, u32 aeth, u32 psn, int opcode,
		     u64 val, struct hfi_ctx *ctx)
{
	struct hfi2_ibport *ibp;
	enum ib_wc_status status;
	struct rvt_swqe *wqe;
	int ret = 0;
	u32 ack_psn;
	int diff;

	/* Remove QP from retry timer */
	if (qp->s_flags & (RVT_S_TIMER | RVT_S_WAIT_RNR)) {
		qp->s_flags &= ~(RVT_S_TIMER | RVT_S_WAIT_RNR);
		del_timer(&qp->s_timer);
	}

	/*
	 * Note that NAKs implicitly ACK outstanding SEND and RDMA write
	 * requests and implicitly NAK RDMA read and atomic requests issued
	 * before the NAK'ed request.  The MSN won't include the NAK'ed
	 * request but will include an ACK'ed request(s).
	 */
	ack_psn = psn;
	if (aeth >> 29)
		ack_psn--;
	wqe = rvt_get_swqe_ptr(qp, qp->s_acked);
	ibp = to_hfi_ibp(qp->ibqp.device, qp->port_num);

	/*
	 * The MSN might be for a later WQE than the PSN indicates so
	 * only complete WQEs that the PSN finishes.
	 */
	while ((diff = delta_psn(ack_psn, wqe->lpsn)) >= 0) {
		/*
		 * RDMA_READ_RESPONSE_ONLY is a special case since
		 * we want to generate completion events for everything
		 * before the RDMA read, copy the data, then generate
		 * the completion for the read.
		 */
		if (wqe->wr.opcode == IB_WR_RDMA_READ &&
		    opcode == OP(RDMA_READ_RESPONSE_ONLY) &&
		    diff == 0) {
			ret = 1;
			goto bail;
		}
		/*
		 * If this request is a RDMA read or atomic, and the ACK is
		 * for a later operation, this ACK NAKs the RDMA read or
		 * atomic.  In other words, only a RDMA_READ_LAST or ONLY
		 * can ACK a RDMA read and likewise for atomic ops.  Note
		 * that the NAK case can only happen if relaxed ordering is
		 * used and requests are sent after an RDMA read or atomic
		 * is sent but before the response is received.
		 */
		if ((wqe->wr.opcode == IB_WR_RDMA_READ &&
		     (opcode != OP(RDMA_READ_RESPONSE_LAST) || diff != 0)) ||
		    ((wqe->wr.opcode == IB_WR_ATOMIC_CMP_AND_SWP ||
		      wqe->wr.opcode == IB_WR_ATOMIC_FETCH_AND_ADD) &&
		     (opcode != OP(ATOMIC_ACKNOWLEDGE) || diff != 0))) {
			/* Retry this request. */
			if (!(qp->r_flags & RVT_R_RDMAR_SEQ)) {
				qp->r_flags |= RVT_R_RDMAR_SEQ;
				restart_rc(qp, qp->s_last_psn + 1, 0);
				rc_deferred_send(ctx, qp);
			}
			/*
			 * No need to process the ACK/NAK since we are
			 * restarting an earlier request.
			 */
			goto bail;
		}
		if (wqe->wr.opcode == IB_WR_ATOMIC_CMP_AND_SWP ||
		    wqe->wr.opcode == IB_WR_ATOMIC_FETCH_AND_ADD) {
			u64 *vaddr = wqe->sg_list[0].vaddr;
			*vaddr = val;
		}
		if (qp->s_num_rd_atomic &&
		    (wqe->wr.opcode == IB_WR_RDMA_READ ||
		     wqe->wr.opcode == IB_WR_ATOMIC_CMP_AND_SWP ||
		     wqe->wr.opcode == IB_WR_ATOMIC_FETCH_AND_ADD)) {
			qp->s_num_rd_atomic--;
			/* Restart sending task if fence is complete */
			if ((qp->s_flags & RVT_S_WAIT_FENCE) &&
			    !qp->s_num_rd_atomic) {
				qp->s_flags &= ~(RVT_S_WAIT_FENCE |
						 RVT_S_WAIT_ACK);
				hfi2_schedule_send(qp);
			} else if (qp->s_flags & RVT_S_WAIT_RDMAR) {
				qp->s_flags &= ~(RVT_S_WAIT_RDMAR |
						 RVT_S_WAIT_ACK);
				hfi2_schedule_send(qp);
			}
		}
		wqe = do_rc_completion(qp, wqe, ibp);
		if (qp->s_acked == qp->s_tail)
			break;
	}

	switch (aeth >> 29) {
	case 0:         /* ACK */
		this_cpu_inc(*ibp->rc_acks);
		if (qp->s_acked != qp->s_tail) {
			/*
			 * We are expecting more ACKs so
			 * reset the re-transmit timer.
			 */
			start_timer(qp);
			/*
			 * We can stop re-sending the earlier packets and
			 * continue with the next packet the receiver wants.
			 */
			if (cmp_psn(qp->s_psn, psn) <= 0)
				reset_psn(qp, psn + 1);
		} else if (cmp_psn(qp->s_psn, psn) <= 0) {
			qp->s_state = OP(SEND_LAST);
			qp->s_psn = psn + 1;
		}
		if (qp->s_flags & RVT_S_WAIT_ACK) {
			qp->s_flags &= ~RVT_S_WAIT_ACK;
			hfi2_schedule_send(qp);
		}
		get_credit(qp, aeth);
		qp->s_rnr_retry = qp->s_rnr_retry_cnt;
		qp->s_retry = qp->s_retry_cnt;
		update_last_psn(qp, psn);
		ret = 1;
		goto bail;

	case 1:         /* RNR NAK */
		ibp->n_rnr_naks++;
		if (qp->s_acked == qp->s_tail)
			goto bail;
		if (qp->s_flags & RVT_S_WAIT_RNR)
			goto bail;
		if (qp->s_rnr_retry == 0) {
			status = IB_WC_RNR_RETRY_EXC_ERR;
			goto class_b;
		}
		if (qp->s_rnr_retry_cnt < 7)
			qp->s_rnr_retry--;

		/* The last valid PSN is the previous PSN. */
		update_last_psn(qp, psn - 1);

		ibp->n_rc_resends += delta_psn(qp->s_psn, psn);

		reset_psn(qp, psn);

		qp->s_flags &= ~(RVT_S_WAIT_SSN_CREDIT | RVT_S_WAIT_ACK);
		qp->s_flags |= RVT_S_WAIT_RNR;
		qp->s_timer.function = rc_rnr_retry;
		qp->s_timer.expires = jiffies + usecs_to_jiffies(
			ib_hfi1_rnr_table[(aeth >> HFI1_AETH_CREDIT_SHIFT) &
					   HFI1_AETH_CREDIT_MASK]);
		add_timer(&qp->s_timer);
		goto bail;

	case 3:         /* NAK */
		if (qp->s_acked == qp->s_tail)
			goto bail;
		/* The last valid PSN is the previous PSN. */
		update_last_psn(qp, psn - 1);
		switch ((aeth >> HFI1_AETH_CREDIT_SHIFT) &
			HFI1_AETH_CREDIT_MASK) {
		case 0: /* PSN sequence error */
			ibp->n_seq_naks++;
			/*
			 * Back up to the responder's expected PSN.
			 * Note that we might get a NAK in the middle of an
			 * RDMA READ response which terminates the RDMA
			 * READ.
			 */
			restart_rc(qp, psn, 0);
			hfi2_schedule_send(qp);
			break;

		case 1: /* Invalid Request */
			status = IB_WC_REM_INV_REQ_ERR;
			ibp->n_other_naks++;
			goto class_b;

		case 2: /* Remote Access Error */
			status = IB_WC_REM_ACCESS_ERR;
			ibp->n_other_naks++;
			goto class_b;

		case 3: /* Remote Operation Error */
			status = IB_WC_REM_OP_ERR;
			ibp->n_other_naks++;
class_b:
			if (qp->s_last == qp->s_acked) {
				hfi2_send_complete(qp, wqe, status);
				rvt_error_qp(qp, IB_WC_WR_FLUSH_ERR);
			}
			break;

		default:
			/* Ignore other reserved NAK error codes */
			goto reserved;
		}
		qp->s_retry = qp->s_retry_cnt;
		qp->s_rnr_retry = qp->s_rnr_retry_cnt;
		goto bail;

	default:                /* 2: reserved */
reserved:
		/* Ignore reserved NAK codes. */
		goto bail;
	}

bail:
	return ret;
}

/*
 * We have seen an out of sequence RDMA read middle or last packet.
 * This ACKs SENDs and RDMA writes up to the first RDMA read or atomic SWQE.
 */
static void rdma_seq_err(struct rvt_qp *qp, struct hfi2_ibport *ibp,
			 u32 psn, struct hfi_ctx *ctx)
{
	struct rvt_swqe *wqe;

	/* Remove QP from retry timer */
	if (qp->s_flags & (RVT_S_TIMER | RVT_S_WAIT_RNR)) {
		qp->s_flags &= ~(RVT_S_TIMER | RVT_S_WAIT_RNR);
		del_timer(&qp->s_timer);
	}

	wqe = rvt_get_swqe_ptr(qp, qp->s_acked);

	while (cmp_psn(psn, wqe->lpsn) > 0) {
		if (wqe->wr.opcode == IB_WR_RDMA_READ ||
		    wqe->wr.opcode == IB_WR_ATOMIC_CMP_AND_SWP ||
		    wqe->wr.opcode == IB_WR_ATOMIC_FETCH_AND_ADD)
			break;
		wqe = do_rc_completion(qp, wqe, ibp);
	}

	ibp->n_rdma_seq++;
	qp->r_flags |= RVT_R_RDMAR_SEQ;
	restart_rc(qp, qp->s_last_psn + 1, 0);
	rc_deferred_send(ctx, qp);
}

/**
 * rc_rcv_resp - process an incoming RC response packet
 * @ibp: the port this packet came in on
 * @ohdr: the other headers for this packet
 * @data: the packet data
 * @tlen: the packet length
 * @qp: the QP for this packet
 * @opcode: the opcode for this packet
 * @psn: the packet sequence number for this packet
 * @hdrsize: the header length
 * @pmtu: the path MTU
 * @pad: pad length
 * @extra_bytes: length of ICRC and any TAIL bytes
 *
 * This is called from hfi2_rc_rcv() to process an incoming RC response
 * packet for the given QP.
 */
static void rc_rcv_resp(struct hfi2_ibport *ibp,
			struct ib_l4_headers *ohdr,
			void *data, u32 tlen, struct rvt_qp *qp,
			u32 opcode, u32 psn, u32 hdrsize, u32 pmtu,
			struct hfi_ctx *ctx, u32 pad, u8 extra_bytes)
{
	struct rvt_swqe *wqe;
	enum ib_wc_status status;
	unsigned long flags;
	int diff;
	u32 aeth;
	u64 val;

	spin_lock_irqsave(&qp->s_lock, flags);

	/* Ignore invalid responses. */
	if (cmp_psn(psn, qp->s_next_psn) >= 0)
		goto ack_done;

	/* Ignore duplicate responses. */
	diff = cmp_psn(psn, qp->s_last_psn);
	if (unlikely(diff <= 0)) {
		/* Update credits for "ghost" ACKs */
		if (diff == 0 && opcode == OP(ACKNOWLEDGE)) {
			aeth = be32_to_cpu(ohdr->u.aeth);
			if ((aeth >> 29) == 0)
				get_credit(qp, aeth);
		}
		goto ack_done;
	}

	/*
	 * Skip everything other than the PSN we expect, if we are waiting
	 * for a reply to a restarted RDMA read or atomic op.
	 */
	if (qp->r_flags & RVT_R_RDMAR_SEQ) {
		if (cmp_psn(psn, qp->s_last_psn + 1) != 0)
			goto ack_done;
		qp->r_flags &= ~RVT_R_RDMAR_SEQ;
	}

	if (unlikely(qp->s_acked == qp->s_tail))
		goto ack_done;
	wqe = rvt_get_swqe_ptr(qp, qp->s_acked);
	status = IB_WC_SUCCESS;

	switch (opcode) {
	case OP(ACKNOWLEDGE):
	case OP(ATOMIC_ACKNOWLEDGE):
	case OP(RDMA_READ_RESPONSE_FIRST):
		aeth = be32_to_cpu(ohdr->u.aeth);
		if (opcode == OP(ATOMIC_ACKNOWLEDGE)) {
			__be32 *p = ohdr->u.at.atomic_ack_eth;

			val = ((u64) be32_to_cpu(p[0]) << 32) |
				be32_to_cpu(p[1]);
		} else
			val = 0;
		if (!do_rc_ack(qp, aeth, psn, opcode, val, ctx) ||
		    opcode != OP(RDMA_READ_RESPONSE_FIRST))
			goto ack_done;
		wqe = rvt_get_swqe_ptr(qp, qp->s_acked);
		if (unlikely(wqe->wr.opcode != IB_WR_RDMA_READ))
			goto ack_op_err;
		/*
		 * If this is a response to a resent RDMA read, we
		 * have to be careful to copy the data to the right
		 * location.
		 */
		qp->s_rdma_read_len = restart_sge(&qp->s_rdma_read_sge,
						  wqe, psn, pmtu);
		goto read_middle;

	case OP(RDMA_READ_RESPONSE_MIDDLE):
		/* no AETH, no ACK */
		if (unlikely(cmp_psn(psn, qp->s_last_psn + 1)))
			goto ack_seq_err;
		if (unlikely(wqe->wr.opcode != IB_WR_RDMA_READ))
			goto ack_op_err;
read_middle:
		if (unlikely(tlen != (hdrsize + pmtu + pad + extra_bytes)))
			goto ack_len_err;
		if (unlikely(pmtu >= qp->s_rdma_read_len))
			goto ack_len_err;

		/*
		 * We got a response so update the timeout.
		 * 4.096 usec. * (1 << qp->timeout)
		 */
		qp->s_flags |= RVT_S_TIMER;
		mod_timer(&qp->s_timer, jiffies + qp->timeout_jiffies);
		if (qp->s_flags & RVT_S_WAIT_ACK) {
			qp->s_flags &= ~RVT_S_WAIT_ACK;
			hfi2_schedule_send(qp);
		}

		if (opcode == OP(RDMA_READ_RESPONSE_MIDDLE))
			qp->s_retry = qp->s_retry_cnt;

		/*
		 * Update the RDMA receive state but do the copy w/o
		 * holding the locks and blocking interrupts.
		 */
		qp->s_rdma_read_len -= pmtu;
		update_last_psn(qp, psn);
		spin_unlock_irqrestore(&qp->s_lock, flags);
		hfi2_copy_sge(&qp->s_rdma_read_sge, data, pmtu, 0);
		goto bail;

	case OP(RDMA_READ_RESPONSE_ONLY):
		aeth = be32_to_cpu(ohdr->u.aeth);
		if (!do_rc_ack(qp, aeth, psn, opcode, 0, ctx))
			goto ack_done;
		/*
		 * Check that the data size is >= 0 && <= pmtu.
		 * Remember to account for ICRC and any TAIL.
		 */
		if (unlikely(tlen < (hdrsize + pad + extra_bytes)))
			goto ack_len_err;
		/*
		 * If this is a response to a resent RDMA read, we
		 * have to be careful to copy the data to the right
		 * location.
		 */
		wqe = rvt_get_swqe_ptr(qp, qp->s_acked);
		qp->s_rdma_read_len = restart_sge(&qp->s_rdma_read_sge,
						  wqe, psn, pmtu);
		goto read_last;

	case OP(RDMA_READ_RESPONSE_LAST):
		/* ACKs READ req. */
		if (unlikely(cmp_psn(psn, qp->s_last_psn + 1)))
			goto ack_seq_err;
		if (unlikely(wqe->wr.opcode != IB_WR_RDMA_READ))
			goto ack_op_err;
		/*
		 * Check that the data size is >= 1 && <= pmtu.
		 * Remember to account for ICRC and any TAIL.
		 */
		if (unlikely(tlen <= (hdrsize + pad + extra_bytes)))
			goto ack_len_err;
read_last:
		tlen -= hdrsize + pad + extra_bytes;
		if (unlikely(tlen != qp->s_rdma_read_len))
			goto ack_len_err;
		aeth = be32_to_cpu(ohdr->u.aeth);
		hfi2_copy_sge(&qp->s_rdma_read_sge, data, tlen, 0);
		WARN_ON(qp->s_rdma_read_sge.num_sge);
		(void) do_rc_ack(qp, aeth, psn,
				 OP(RDMA_READ_RESPONSE_LAST), 0, ctx);
		goto ack_done;
	}

ack_op_err:
	status = IB_WC_LOC_QP_OP_ERR;
	goto ack_err;

ack_seq_err:
	rdma_seq_err(qp, ibp, psn, ctx);
	goto ack_done;

ack_len_err:
	status = IB_WC_LOC_LEN_ERR;
ack_err:
	if (qp->s_last == qp->s_acked) {
		hfi2_send_complete(qp, wqe, status);
		rvt_error_qp(qp, IB_WC_WR_FLUSH_ERR);
	}
ack_done:
	spin_unlock_irqrestore(&qp->s_lock, flags);
bail:
	return;
}

static inline void rc_defered_ack(struct hfi_ctx *ctx,
				  struct rvt_qp *qp)
{
	/*  FXRTODO: Implement deferrred ack */
#if 0
	if (list_empty(&qp->rspwait)) {
		qp->r_flags |= RVT_R_RSP_DEFERED_ACK;
		atomic_inc(&qp->refcount);
		list_add_tail(&qp->rspwait, &ctx->qp_wait_list);
	}
#else
	hfi2_send_rc_ack(qp, 0);
#endif
}

static inline void rc_cancel_ack(struct rvt_qp *qp)
{
	/*  FXRTODO: Implement deferrred ack */
#if 0
	qp->r_adefered = 0;
	if (list_empty(&qp->rspwait))
		return;
	list_del_init(&qp->rspwait);
	qp->r_flags &= ~RVT_R_RSP_DEFERED_ACK;
	if (atomic_dec_and_test(&qp->refcount))
		wake_up(&qp->wait);
#endif
}
/**
 * rc_rcv_error - process an incoming duplicate or error RC packet
 * @ohdr: the other headers for this packet
 * @data: the packet data
 * @qp: the QP for this packet
 * @opcode: the opcode for this packet
 * @psn: the packet sequence number for this packet
 * @diff: the difference between the PSN and the expected PSN
 * @ctx: context
 *
 * This is called from hfi2_rc_rcv() to process an unexpected
 * incoming RC packet for the given QP.
 * Called at interrupt level.
 * Return 1 if no more processing is needed; otherwise return 0 to
 * schedule a response to be sent.
 */
static noinline int rc_rcv_error(struct ib_l4_headers *ohdr, void *data,
			struct rvt_qp *qp, u32 opcode, u32 psn, int diff,
			struct hfi_ctx *ctx)
{
	struct hfi2_ibport *ibp = to_hfi_ibp(qp->ibqp.device, qp->port_num);
	struct rvt_ack_entry *e;
	unsigned long flags;
	u8 i, prev;
	int old_req;

	if (diff > 0) {
		/*
		 * Packet sequence error.
		 * A NAK will ACK earlier sends and RDMA writes.
		 * Don't queue the NAK if we already sent one.
		 */
		if (!qp->r_nak_state) {
			ibp->n_rc_seqnak++;
			qp->r_nak_state = IB_NAK_PSN_ERROR;
			/* Use the expected PSN. */
			qp->r_ack_psn = qp->r_psn;
			/*
			 * Wait to send the sequence NAK until all packets
			 * in the receive queue have been processed.
			 * Otherwise, we end up propagating congestion.
			 */
			rc_defered_ack(ctx, qp);
		}
		goto done;
	}

	/*
	 * Handle a duplicate request.  Don't re-execute SEND, RDMA
	 * write or atomic op.  Don't NAK errors, just silently drop
	 * the duplicate request.  Note that r_sge, r_len, and
	 * r_rcv_len may be in use so don't modify them.
	 *
	 * We are supposed to ACK the earliest duplicate PSN but we
	 * can coalesce an outstanding duplicate ACK.  We have to
	 * send the earliest so that RDMA reads can be restarted at
	 * the requester's expected PSN.
	 *
	 * First, find where this duplicate PSN falls within the
	 * ACKs previously sent.
	 * old_req is true if there is an older response that is scheduled
	 * to be sent before sending this one.
	 */
	e = NULL;
	old_req = 1;
	ibp->n_rc_dupreq++;

	spin_lock_irqsave(&qp->s_lock, flags);

	for (i = qp->r_head_ack_queue; ; i = prev) {
		if (i == qp->s_tail_ack_queue)
			old_req = 0;
		if (i)
			prev = i - 1;
		else
			prev = OPA_IB_MAX_RDMA_ATOMIC;
		if (prev == qp->r_head_ack_queue) {
			e = NULL;
			break;
		}
		e = &qp->s_ack_queue[prev];
		if (!e->opcode) {
			e = NULL;
			break;
		}
		if (cmp_psn(psn, e->psn) >= 0) {
			if (prev == qp->s_tail_ack_queue &&
			    cmp_psn(psn, e->lpsn) <= 0)
				old_req = 0;
			break;
		}
	}
	switch (opcode) {
	case OP(RDMA_READ_REQUEST): {
		struct ib_reth *reth;
		u32 offset;
		u32 len;

		/*
		 * If we didn't find the RDMA read request in the ack queue,
		 * we can ignore this request.
		 */
		if (!e || e->opcode != OP(RDMA_READ_REQUEST))
			goto unlock_done;
		/* RETH comes after BTH */
		reth = &ohdr->u.rc.reth;
		/*
		 * Address range must be a subset of the original
		 * request and start on pmtu boundaries.
		 * We reuse the old ack_queue slot since the requester
		 * should not back up and request an earlier PSN for the
		 * same request.
		 */
		offset = delta_psn(psn, e->psn) * qp->pmtu;
		len = be32_to_cpu(reth->length);
		if (unlikely(offset + len != e->rdma_sge.sge_length))
			goto unlock_done;
		if (e->rdma_sge.mr) {
			rvt_put_mr(e->rdma_sge.mr);
			e->rdma_sge.mr = NULL;
		}
		if (len != 0) {
			u32 rkey = be32_to_cpu(reth->rkey);
			u64 vaddr = be64_to_cpu(reth->vaddr);
			int ok;

			ok = rvt_rkey_ok(qp, &e->rdma_sge, len, vaddr, rkey,
					 IB_ACCESS_REMOTE_READ);
			if (unlikely(!ok))
				goto unlock_done;
		} else {
			e->rdma_sge.vaddr = NULL;
			e->rdma_sge.length = 0;
			e->rdma_sge.sge_length = 0;
		}
		e->psn = psn;
		if (old_req)
			goto unlock_done;
		qp->s_tail_ack_queue = prev;
		break;
	}

	case OP(COMPARE_SWAP):
	case OP(FETCH_ADD): {
		/*
		 * If we didn't find the atomic request in the ack queue
		 * or the send tasklet is already backed up to send an
		 * earlier entry, we can ignore this request.
		 */
		if (!e || e->opcode != (u8) opcode || old_req)
			goto unlock_done;
		qp->s_tail_ack_queue = prev;
		break;
	}

	default:
		/*
		 * Ignore this operation if it doesn't request an ACK
		 * or an earlier RDMA read or atomic is going to be resent.
		 */
		if (!(psn & IB_BTH_REQ_ACK) || old_req)
			goto unlock_done;
		/*
		 * Resend the most recent ACK if this request is
		 * after all the previous RDMA reads and atomics.
		 */
		if (i == qp->r_head_ack_queue) {
			spin_unlock_irqrestore(&qp->s_lock, flags);
			qp->r_nak_state = 0;
			qp->r_ack_psn = qp->r_psn - 1;
			goto send_ack;
		}

		/*
		 * Resend the RDMA read or atomic op which
		 * ACKs this duplicate request.
		 */
		qp->s_tail_ack_queue = i;
		break;
	}
	qp->s_ack_state = OP(ACKNOWLEDGE);
	qp->s_flags |= RVT_S_RESP_PENDING;
	qp->r_nak_state = 0;
	hfi2_schedule_send(qp);

unlock_done:
	spin_unlock_irqrestore(&qp->s_lock, flags);
done:
	return 1;

send_ack:
	return 0;
}

void hfi2_rc_error(struct rvt_qp *qp, enum ib_wc_status err)
{
	unsigned long flags;
	int lastwqe;

	spin_lock_irqsave(&qp->s_lock, flags);
	lastwqe = rvt_error_qp(qp, err);
	spin_unlock_irqrestore(&qp->s_lock, flags);

	if (lastwqe) {
		struct ib_event ev;

		ev.device = qp->ibqp.device;
		ev.element.qp = &qp->ibqp;
		ev.event = IB_EVENT_QP_LAST_WQE_REACHED;
		qp->ibqp.event_handler(&ev, qp->ibqp.qp_context);
	}
}

static inline void update_ack_queue(struct rvt_qp *qp, unsigned n)
{
	unsigned next;

	next = n + 1;
	if (next > OPA_IB_MAX_RDMA_ATOMIC)
		next = 0;
	qp->s_tail_ack_queue = next;
	qp->s_ack_state = OP(ACKNOWLEDGE);
}

#ifdef HFI_VERBS_TEST
/* In  Percentage 0 - 100 */
#define MIN_DROP_RATE	0
#define MAX_DROP_RATE	40
#define MAX_DROPS	4
bool hfi2_drop_packet(void)
{
	static bool prev_drop;
	static int prev_dr = MAX_DROP_RATE;
	static int num_dropped;
	uint8_t rand_num;
	uint8_t rand_size = sizeof(rand_num);
	int curr_dr;
	uint8_t check;

	curr_dr = prev_drop ? max_t(int, prev_dr - 10, MIN_DROP_RATE) :
				min_t(int, prev_dr + 10, MAX_DROP_RATE);
	check = (curr_dr << (rand_size * BITS_PER_BYTE)) / 100;
	prev_dr = curr_dr;

	get_random_bytes(&rand_num, rand_size);
	prev_drop = (rand_num < check) ? true : false;

	/* safety measure so we don't drop too many consecutive packets */
	if (prev_drop)
		num_dropped++;
	if (num_dropped > MAX_DROPS)
		prev_drop = false;
	if (!prev_drop)
		num_dropped = 0;

	return prev_drop;
}
#endif

/**
 * hfi2_rc_rcv - process an incoming RC packet
 * @qp: the QP for this packet
 * @packet: incoming packet information
 *
 * This is called from hfi2_rcv() to process an incoming RC packet
 * for the given QP.
 * TODO - verify that packet->ctx->wait_list usage is safe to not hold a lock.
 */
void hfi2_rc_rcv(struct rvt_qp *qp, struct hfi2_ib_packet *packet)
{
	struct hfi_ctx *ctx = packet->ctx;
	void *data = packet->ebuf;
	u32 tlen = packet->tlen;
	struct hfi2_ibport *ibp = packet->ibp;
	struct ib_l4_headers *ohdr = packet->ohdr;
	union hfi2_packet_header *ph = packet->hdr;
	u32 bth0, opcode;
	u32 hdrsize = packet->hlen;
	u32 psn;
	u32 pad;
	struct ib_wc wc;
	u32 pmtu = qp->pmtu;
	int diff;
	struct ib_reth *reth;
	unsigned long flags;
	u32 bth1;
	int ret;
	bool is_becn, is_fecn;
	u8 age, l4, rc, sc, extra_bytes;
	u16 entropy, len, pkey;
	u32 slid, dlid;
	bool is_16b = (packet->etype == RHF_RCV_TYPE_BYPASS);

	bth0 = be32_to_cpu(ohdr->bth[0]);
	bth1 = be32_to_cpu(ohdr->bth[1]);
	psn = be32_to_cpu(ohdr->bth[2]);
	opcode = bth0 >> 24;
	hdrsize += eth_len_by_opcode[opcode];
	if (is_16b) {
#if 0
		if (hfi2_ruc_check_hdr_16b(ibp, ph->opa16b,
					   !!packet->grh, qp, bth0))
			return;
#endif
		opa_parse_16b_header((u32 *)&ph->opa16b, &slid, &dlid, &len,
				     &pkey, &entropy, &sc, &rc, &is_fecn,
				     &is_becn, &age, &l4);

		/* Get the number of bytes the message was padded by. */
		pad = (be32_to_cpu(ohdr->bth[0]) >> 20) & 7;
		extra_bytes = 5;    /* ICRC + TAIL byte */
		data += hdrsize;
	} else {
#if 0
		if (hfi2_ruc_check_hdr(ibp, ph->ibh, !!packet->grh, qp, bth0))
			return;
#endif
		is_fecn = !!(bth1 & HFI1_FECN_SMASK);
		is_becn = !!(bth1 & HFI1_BECN_SMASK);

		/* Get the number of bytes the message was padded by. */
		pad = (be32_to_cpu(ohdr->bth[0]) >> 20) & 3;
		extra_bytes = 4;    /* ICRC */
	}

#if 0
	if (is_becn) {
		u32 rlid = qp->remote_ah_attr.dlid;
		u32 rqpn, lqpn;

		lqpn = qp->ibqp.qp_num;
		rqpn = qp->remote_qpn;

		process_becn(ibp, qp->remote_ah_attr.sl, rlid,
			     lqpn, rqpn, IB_CC_SVCTYPE_UC);
	}
#endif

	/*
	 * Process responses (ACKs) before anything else.  Note that the
	 * packet sequence number will be for something in the send work
	 * queue rather than the expected receive packet sequence number.
	 * In other words, this QP is the requester.
	 */
	if (opcode >= OP(RDMA_READ_RESPONSE_FIRST) &&
	    opcode <= OP(ATOMIC_ACKNOWLEDGE)) {
		rc_rcv_resp(ibp, ohdr, data, tlen, qp, opcode, psn,
			    hdrsize, pmtu, ctx, pad, extra_bytes);
		if (is_fecn)
			goto send_ack;
		return;
	}

	/* Compute 24 bits worth of difference. */
	diff = delta_psn(psn, qp->r_psn);
	if (unlikely(diff)) {
		if (rc_rcv_error(ohdr, data, qp, opcode, psn, diff, ctx))
			return;
		goto send_ack;
	}
#ifdef HFI_VERBS_TEST
	if (hfi2_drop_packet() && qp->s_retry_cnt) {
		dev_dbg(ibp->dev, "Dropping packet (opcode = %s) at rcv end with PSN = %u\n",
			opcode_to_str[opcode], mask_psn(psn));
		goto drop;
	}
#endif

	/* Check for opcode sequence errors. */
	switch (qp->r_state) {
	case OP(SEND_FIRST):
	case OP(SEND_MIDDLE):
		if (opcode == OP(SEND_MIDDLE) ||
		    opcode == OP(SEND_LAST) ||
		    opcode == OP(SEND_LAST_WITH_IMMEDIATE))
			break;
		goto nack_inv;

	case OP(RDMA_WRITE_FIRST):
	case OP(RDMA_WRITE_MIDDLE):
		if (opcode == OP(RDMA_WRITE_MIDDLE) ||
		    opcode == OP(RDMA_WRITE_LAST) ||
		    opcode == OP(RDMA_WRITE_LAST_WITH_IMMEDIATE))
			break;
		goto nack_inv;

	default:
		if (opcode == OP(SEND_MIDDLE) ||
		    opcode == OP(SEND_LAST) ||
		    opcode == OP(SEND_LAST_WITH_IMMEDIATE) ||
		    opcode == OP(RDMA_WRITE_MIDDLE) ||
		    opcode == OP(RDMA_WRITE_LAST) ||
		    opcode == OP(RDMA_WRITE_LAST_WITH_IMMEDIATE))
			goto nack_inv;
		/*
		 * Note that it is up to the requester to not send a new
		 * RDMA read or atomic operation before receiving an ACK
		 * for the previous operation.
		 */
		break;
	}

	if (qp->state == IB_QPS_RTR && !(qp->r_flags & RVT_R_COMM_EST))
		qp_comm_est(qp);

	/* OK, process the packet. */
	switch (opcode) {
	case OP(SEND_FIRST):
#ifdef HFI_VERBS_TEST
		if (hfi2_drop_packet() && qp->s_rnr_retry_cnt) {
			dev_dbg(ibp->dev, "Initiating RNR (opcode = %s) with PSN = %u\n"
				 , opcode_to_str[opcode], mask_psn(psn));
			goto rnr_nak;
		}
#endif
		ret = hfi2_get_rwqe(qp, 0);
		if (ret < 0)
			goto nack_op_err;
		if (!ret)
			goto rnr_nak;
		qp->r_rcv_len = 0;
		/* FALLTHROUGH */
	case OP(SEND_MIDDLE):
	case OP(RDMA_WRITE_MIDDLE):
send_middle:
		/* Check for invalid length PMTU or posted rwqe len. */
		if (unlikely(tlen != (hdrsize + pmtu + pad + extra_bytes)))
			goto nack_inv;
		qp->r_rcv_len += pmtu;
		if (unlikely(qp->r_rcv_len > qp->r_len))
			goto nack_inv;
		hfi2_copy_sge(&qp->r_sge, data, pmtu, 1);
		break;

	case OP(RDMA_WRITE_LAST_WITH_IMMEDIATE):
#ifdef HFI_VERBS_TEST
		if (hfi2_drop_packet() && qp->s_rnr_retry_cnt) {
			dev_dbg(ibp->dev, "Initiating RNR (opcode = %s) with PSN = %u\n"
				 , opcode_to_str[opcode], mask_psn(psn));
			goto rnr_nak;
		}
#endif
		/* consume RWQE */
		ret = hfi2_get_rwqe(qp, 1);
		if (ret < 0)
			goto nack_op_err;
		if (!ret)
			goto rnr_nak;
		goto send_last_imm;

	case OP(SEND_ONLY):
	case OP(SEND_ONLY_WITH_IMMEDIATE):
#ifdef HFI_VERBS_TEST
		if (hfi2_drop_packet() && qp->s_rnr_retry_cnt) {
			dev_dbg(ibp->dev, "Initiating RNR (opcode = %s) with PSN = %u\n"
				 , opcode_to_str[opcode], mask_psn(psn));
			goto rnr_nak;
		}
#endif
		ret = hfi2_get_rwqe(qp, 0);
		if (ret < 0)
			goto nack_op_err;
		if (!ret)
			goto rnr_nak;
		qp->r_rcv_len = 0;
		if (opcode == OP(SEND_ONLY))
			goto no_immediate_data;
		/* FALLTHROUGH for SEND_ONLY_WITH_IMMEDIATE */
	case OP(SEND_LAST_WITH_IMMEDIATE):
send_last_imm:
		wc.ex.imm_data = ohdr->u.imm_data;
		wc.wc_flags = IB_WC_WITH_IMM;
		goto send_last;
	case OP(SEND_LAST):
	case OP(RDMA_WRITE_LAST):
no_immediate_data:
		wc.wc_flags = 0;
		wc.ex.imm_data = 0;
send_last:
		/* Check for invalid length. */
		/* LAST len should be >= 1 */
		if (unlikely(tlen < (hdrsize + pad + extra_bytes)))
			goto nack_inv;
		/* Don't count the CRC. */
		tlen -= (hdrsize + pad + extra_bytes);
		wc.byte_len = tlen + qp->r_rcv_len;
		if (unlikely(wc.byte_len > qp->r_len))
			goto nack_inv;
		hfi2_copy_sge(&qp->r_sge, data, tlen, 1);
		rvt_put_ss(&qp->r_sge);
		qp->r_msn++;
		if (!test_and_clear_bit(RVT_R_WRID_VALID, &qp->r_aflags))
			break;
		wc.wr_id = qp->r_wr_id;
		wc.status = IB_WC_SUCCESS;
		if (opcode == OP(RDMA_WRITE_LAST_WITH_IMMEDIATE) ||
		    opcode == OP(RDMA_WRITE_ONLY_WITH_IMMEDIATE))
			wc.opcode = IB_WC_RECV_RDMA_WITH_IMM;
		else
			wc.opcode = IB_WC_RECV;
		wc.qp = &qp->ibqp;
		wc.src_qp = qp->remote_qpn;
		wc.slid = qp->remote_ah_attr.dlid;
		/*
		 * It seems that IB mandates the presence of an SL in a
		 * work completion only for the UD transport (see section
		 * 11.4.2 of IBTA Vol. 1).
		 *
		 * However, the way the SL is chosen below is consistent
		 * with the way that IB/qib works and is trying avoid
		 * introducing incompatibilities.
		 *
		 * See also OPA Vol. 1, section 9.7.6, and table 9-17.
		 */
		wc.sl = qp->remote_ah_attr.sl;
		/* zero fields that are N/A */
		wc.vendor_err = 0;
		wc.pkey_index = 0;
		wc.dlid_path_bits = 0;
		wc.port_num = 0;
		/* Signal completion event if the solicited bit is set. */
		rvt_cq_enter(ibcq_to_rvtcq(qp->ibqp.recv_cq), &wc,
			      (bth0 & IB_BTH_SOLICITED) != 0);
		break;

	case OP(RDMA_WRITE_FIRST):
	case OP(RDMA_WRITE_ONLY):
	case OP(RDMA_WRITE_ONLY_WITH_IMMEDIATE):
		if (unlikely(!(qp->qp_access_flags & IB_ACCESS_REMOTE_WRITE)))
			goto nack_inv;
		/* consume RWQE */
		reth = &ohdr->u.rc.reth;
		qp->r_len = be32_to_cpu(reth->length);
		qp->r_rcv_len = 0;
		qp->r_sge.sg_list = NULL;
		if (qp->r_len != 0) {
			u32 rkey = be32_to_cpu(reth->rkey);
			u64 vaddr = be64_to_cpu(reth->vaddr);
			int ok;

			/* Check rkey & NAK */
			ok = rvt_rkey_ok(qp, &qp->r_sge.sge, qp->r_len, vaddr,
					 rkey, IB_ACCESS_REMOTE_WRITE);
			if (unlikely(!ok))
				goto nack_acc;
			qp->r_sge.num_sge = 1;
		} else {
			qp->r_sge.num_sge = 0;
			qp->r_sge.sge.mr = NULL;
			qp->r_sge.sge.vaddr = NULL;
			qp->r_sge.sge.length = 0;
			qp->r_sge.sge.sge_length = 0;
		}
		if (opcode == OP(RDMA_WRITE_FIRST))
			goto send_middle;
		else if (opcode == OP(RDMA_WRITE_ONLY))
			goto no_immediate_data;
#ifdef HFI_VERBS_TEST
		if (hfi2_drop_packet() && qp->s_rnr_retry_cnt) {
			dev_dbg(ibp->dev, "Initiating RNR for packet (opcode = %s) at rcv end with PSN = %u\n"
				 , opcode_to_str[opcode], mask_psn(psn));
			goto rnr_nak;
		}
#endif
		ret = hfi2_get_rwqe(qp, 1);
		if (ret < 0)
			goto nack_op_err;
		if (!ret)
			goto rnr_nak;
		wc.ex.imm_data = ohdr->u.rc.imm_data;
		wc.wc_flags = IB_WC_WITH_IMM;
		goto send_last;

	case OP(RDMA_READ_REQUEST): {
		struct rvt_ack_entry *e;
		u32 len;
		u8 next;

		if (unlikely(!(qp->qp_access_flags & IB_ACCESS_REMOTE_READ)))
			goto nack_inv;
		next = qp->r_head_ack_queue + 1;
		/* s_ack_queue is size OPA_IB_MAX_RDMA_ATOMIC+1 so use > not >= */
		if (next > OPA_IB_MAX_RDMA_ATOMIC)
			next = 0;
		spin_lock_irqsave(&qp->s_lock, flags);
		if (unlikely(next == qp->s_tail_ack_queue)) {
			if (!qp->s_ack_queue[next].sent)
				goto nack_inv_unlck;
			update_ack_queue(qp, next);
		}
		e = &qp->s_ack_queue[qp->r_head_ack_queue];
		if (e->opcode == OP(RDMA_READ_REQUEST) && e->rdma_sge.mr) {
			rvt_put_mr(e->rdma_sge.mr);
			e->rdma_sge.mr = NULL;
		}
		reth = &ohdr->u.rc.reth;
		len = be32_to_cpu(reth->length);
		if (len) {
			u32 rkey = be32_to_cpu(reth->rkey);
			u64 vaddr = be64_to_cpu(reth->vaddr);
			int ok;

			/* Check rkey & NAK */
			ok = rvt_rkey_ok(qp, &e->rdma_sge, len, vaddr,
					 rkey, IB_ACCESS_REMOTE_READ);
			if (unlikely(!ok))
				goto nack_acc_unlck;
			/*
			 * Update the next expected PSN.  We add 1 later
			 * below, so only add the remainder here.
			 */
			if (len > pmtu)
				qp->r_psn += (len - 1) / pmtu;
		} else {
			e->rdma_sge.mr = NULL;
			e->rdma_sge.vaddr = NULL;
			e->rdma_sge.length = 0;
			e->rdma_sge.sge_length = 0;
		}
		e->opcode = opcode;
		e->sent = 0;
		e->psn = psn;
		e->lpsn = qp->r_psn;
		/*
		 * We need to increment the MSN here instead of when we
		 * finish sending the result since a duplicate request would
		 * increment it more than once.
		 */
		qp->r_msn++;
		qp->r_psn++;
		qp->r_state = opcode;
		qp->r_nak_state = 0;
		qp->r_head_ack_queue = next;

		/* Schedule the send tasklet. */
		qp->s_flags |= RVT_S_RESP_PENDING;
		hfi2_schedule_send(qp);

		spin_unlock_irqrestore(&qp->s_lock, flags);
		if (is_fecn)
			goto send_ack;
		return;
	}

	case OP(COMPARE_SWAP):
	case OP(FETCH_ADD): {
		struct ib_atomic_eth *ateth;
		struct rvt_ack_entry *e;
		u64 vaddr;
		atomic64_t *maddr;
		u64 sdata;
		u32 rkey;
		u8 next;

		if (unlikely(!(qp->qp_access_flags & IB_ACCESS_REMOTE_ATOMIC)))
			goto nack_inv;
		next = qp->r_head_ack_queue + 1;
		if (next > OPA_IB_MAX_RDMA_ATOMIC)
			next = 0;
		spin_lock_irqsave(&qp->s_lock, flags);
		if (unlikely(next == qp->s_tail_ack_queue)) {
			if (!qp->s_ack_queue[next].sent)
				goto nack_inv_unlck;
			update_ack_queue(qp, next);
		}
		e = &qp->s_ack_queue[qp->r_head_ack_queue];
		if (e->opcode == OP(RDMA_READ_REQUEST) && e->rdma_sge.mr) {
			rvt_put_mr(e->rdma_sge.mr);
			e->rdma_sge.mr = NULL;
		}
		ateth = &ohdr->u.atomic_eth;
		vaddr = ((u64) be32_to_cpu(ateth->vaddr[0]) << 32) |
			be32_to_cpu(ateth->vaddr[1]);
		if (unlikely(vaddr & (sizeof(u64) - 1)))
			goto nack_inv_unlck;
		rkey = be32_to_cpu(ateth->rkey);
		/* Check rkey & NAK */
		if (unlikely(!rvt_rkey_ok(qp, &qp->r_sge.sge, sizeof(u64),
					  vaddr, rkey,
					  IB_ACCESS_REMOTE_ATOMIC)))
			goto nack_acc_unlck;
		/* Perform atomic OP and save result. */
		maddr = (atomic64_t *) qp->r_sge.sge.vaddr;
		sdata = be64_to_cpu(ateth->swap_data);
		e->atomic_data = (opcode == OP(FETCH_ADD)) ?
			(u64) atomic64_add_return(sdata, maddr) - sdata :
			(u64) cmpxchg((u64 *) qp->r_sge.sge.vaddr,
				      be64_to_cpu(ateth->compare_data),
				      sdata);
		rvt_put_mr(qp->r_sge.sge.mr);
		qp->r_sge.num_sge = 0;
		e->opcode = opcode;
		e->sent = 0;
		e->psn = psn;
		e->lpsn = psn;
		qp->r_msn++;
		qp->r_psn++;
		qp->r_state = opcode;
		qp->r_nak_state = 0;
		qp->r_head_ack_queue = next;

		/* Schedule the send tasklet. */
		qp->s_flags |= RVT_S_RESP_PENDING;
		hfi2_schedule_send(qp);

		spin_unlock_irqrestore(&qp->s_lock, flags);
		if (is_fecn)
			goto send_ack;
		return;
	}

	default:
		/* NAK unknown opcodes. */
		goto nack_inv;
	}
	qp->r_psn++;
	qp->r_state = opcode;
	qp->r_ack_psn = psn;
	qp->r_nak_state = 0;
	/* Send an ACK if requested or required. */
	if (psn & IB_BTH_REQ_ACK) {
#if 0
		if (packet->numpkt == 0) {
			rc_cancel_ack(qp);
			goto send_ack;
		}
		/* FXRTODO: Enable as part of STL-7648 */
		if (qp->r_adefered >= HFI1_PSN_CREDIT) {
			rc_cancel_ack(qp);
			goto send_ack;
		}
#endif
		if (unlikely(is_fecn)) {
			rc_cancel_ack(qp);
			goto send_ack;
		}
		/* FXRTODO: Enable as part of STL-7648 */
#if 0
		qp->r_adefered++;
#endif
		rc_defered_ack(ctx, qp);
	}
	return;

rnr_nak:
	qp->r_nak_state = IB_RNR_NAK | qp->r_min_rnr_timer;
	qp->r_ack_psn = qp->r_psn;
	/* Queue RNR NAK for later */
	rc_defered_ack(ctx, qp);
	return;

nack_op_err:
	hfi2_rc_error(qp, IB_WC_LOC_QP_OP_ERR);
	qp->r_nak_state = IB_NAK_REMOTE_OPERATIONAL_ERROR;
	qp->r_ack_psn = qp->r_psn;
	/* Queue NAK for later */
	rc_defered_ack(ctx, qp);
	return;

nack_inv_unlck:
	spin_unlock_irqrestore(&qp->s_lock, flags);
nack_inv:
	hfi2_rc_error(qp, IB_WC_LOC_QP_OP_ERR);
	qp->r_nak_state = IB_NAK_INVALID_REQUEST;
	qp->r_ack_psn = qp->r_psn;
	/* Queue NAK for later */
	rc_defered_ack(ctx, qp);
	return;

nack_acc_unlck:
	spin_unlock_irqrestore(&qp->s_lock, flags);
nack_acc:
	hfi2_rc_error(qp, IB_WC_LOC_PROT_ERR);
	qp->r_nak_state = IB_NAK_REMOTE_ACCESS_ERROR;
	qp->r_ack_psn = qp->r_psn;
send_ack:
	hfi2_send_rc_ack(qp, is_fecn);
	return;
drop:
	dev_dbg(ibp->dev, "RC dropping packet\n");
	return;
}
