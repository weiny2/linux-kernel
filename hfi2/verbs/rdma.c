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

#include <linux/sched.h>
#include "verbs.h"
#include "packet.h"
#include "trace.h"

static void hfi2_cnp_rcv(struct rvt_qp *qp, struct hfi2_ib_packet *packet);
typedef void (*opcode_handler)(struct rvt_qp *qp,
			       struct hfi2_ib_packet *packet);

static const opcode_handler opcode_handler_tbl[256] = {
	/* RC */
	[IB_OPCODE_RC_SEND_FIRST]                     = &hfi2_rc_rcv,
	[IB_OPCODE_RC_SEND_MIDDLE]                    = &hfi2_rc_rcv,
	[IB_OPCODE_RC_SEND_LAST]                      = &hfi2_rc_rcv,
	[IB_OPCODE_RC_SEND_LAST_WITH_IMMEDIATE]       = &hfi2_rc_rcv,
	[IB_OPCODE_RC_SEND_ONLY]                      = &hfi2_rc_rcv,
	[IB_OPCODE_RC_SEND_ONLY_WITH_IMMEDIATE]       = &hfi2_rc_rcv,
	[IB_OPCODE_RC_RDMA_WRITE_FIRST]               = &hfi2_rc_rcv,
	[IB_OPCODE_RC_RDMA_WRITE_MIDDLE]              = &hfi2_rc_rcv,
	[IB_OPCODE_RC_RDMA_WRITE_LAST]                = &hfi2_rc_rcv,
	[IB_OPCODE_RC_RDMA_WRITE_LAST_WITH_IMMEDIATE] = &hfi2_rc_rcv,
	[IB_OPCODE_RC_RDMA_WRITE_ONLY]                = &hfi2_rc_rcv,
	[IB_OPCODE_RC_RDMA_WRITE_ONLY_WITH_IMMEDIATE] = &hfi2_rc_rcv,
	[IB_OPCODE_RC_RDMA_READ_REQUEST]              = &hfi2_rc_rcv,
	[IB_OPCODE_RC_RDMA_READ_RESPONSE_FIRST]       = &hfi2_rc_rcv,
	[IB_OPCODE_RC_RDMA_READ_RESPONSE_MIDDLE]      = &hfi2_rc_rcv,
	[IB_OPCODE_RC_RDMA_READ_RESPONSE_LAST]        = &hfi2_rc_rcv,
	[IB_OPCODE_RC_RDMA_READ_RESPONSE_ONLY]        = &hfi2_rc_rcv,
	[IB_OPCODE_RC_ACKNOWLEDGE]                    = &hfi2_rc_rcv,
	[IB_OPCODE_RC_ATOMIC_ACKNOWLEDGE]             = &hfi2_rc_rcv,
	[IB_OPCODE_RC_COMPARE_SWAP]                   = &hfi2_rc_rcv,
	[IB_OPCODE_RC_FETCH_ADD]                      = &hfi2_rc_rcv,
	/* UC */
	[IB_OPCODE_UC_SEND_FIRST]                     = &hfi2_uc_rcv,
	[IB_OPCODE_UC_SEND_MIDDLE]                    = &hfi2_uc_rcv,
	[IB_OPCODE_UC_SEND_LAST]                      = &hfi2_uc_rcv,
	[IB_OPCODE_UC_SEND_LAST_WITH_IMMEDIATE]       = &hfi2_uc_rcv,
	[IB_OPCODE_UC_SEND_ONLY]                      = &hfi2_uc_rcv,
	[IB_OPCODE_UC_SEND_ONLY_WITH_IMMEDIATE]       = &hfi2_uc_rcv,
	[IB_OPCODE_UC_RDMA_WRITE_FIRST]               = &hfi2_uc_rcv,
	[IB_OPCODE_UC_RDMA_WRITE_MIDDLE]              = &hfi2_uc_rcv,
	[IB_OPCODE_UC_RDMA_WRITE_LAST]                = &hfi2_uc_rcv,
	[IB_OPCODE_UC_RDMA_WRITE_LAST_WITH_IMMEDIATE] = &hfi2_uc_rcv,
	[IB_OPCODE_UC_RDMA_WRITE_ONLY]                = &hfi2_uc_rcv,
	[IB_OPCODE_UC_RDMA_WRITE_ONLY_WITH_IMMEDIATE] = &hfi2_uc_rcv,
	/* UD */
	[IB_OPCODE_UD_SEND_ONLY]                      = &hfi2_ud_rcv,
	[IB_OPCODE_UD_SEND_ONLY_WITH_IMMEDIATE]       = &hfi2_ud_rcv,
	/* CNP */
	[CNP_OPCODE]				      = &hfi2_cnp_rcv
};

/**
 * hfi2_copy_sge - copy data to SGE memory
 * @ss: the SGE state
 * @data: the data to copy
 * @length: the length of the data
 */
void hfi2_copy_sge(struct rvt_sge_state *ss, void *data, u32 length,
		     int release)
{
	struct rvt_sge *sge = &ss->sge;

	while (length) {
		u32 len = sge->length;

		if (len > length)
			len = length;
		if (len > sge->sge_length)
			len = sge->sge_length;
		BUG_ON(len == 0);
		if (data) {
			memcpy(sge->vaddr, data, len);
			data += len;
		}
		sge->vaddr += len;
		sge->length -= len;
		sge->sge_length -= len;
		if (sge->sge_length == 0) {
			if (release)
				rvt_put_mr(sge->mr);
			if (--ss->num_sge)
				*sge = *ss->sg_list++;
		} else if (sge->length == 0 && sge->mr->lkey) {
			if (++sge->n >= RVT_SEGSZ) {
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
}

/**
 * hfi2_skip_sge - skip over SGE memory
 * @ss: the SGE state
 * @length: the number of bytes to skip
 */
void hfi2_skip_sge(struct rvt_sge_state *ss, u32 length, int release)
{
	hfi2_copy_sge(ss, NULL, length, release);
}

/**
 * hfi2_update_sge - update state that is tracking progress within the
 * SG list to note that length bytes have been sent
 * @ss: the SGE state
 * @length: the number of bytes to skip
 */
void hfi2_update_sge(struct rvt_sge_state *ss, u32 length)
{
	struct rvt_sge *sge = &ss->sge;

	sge->vaddr += length;
	sge->length -= length;
	sge->sge_length -= length;
	if (sge->sge_length == 0) {
		if (--ss->num_sge)
			*sge = *ss->sg_list++;
	} else if (sge->length == 0 && sge->mr->lkey) {
		if (++sge->n >= RVT_SEGSZ) {
			if (++sge->m >= sge->mr->mapsz)
				return;
			sge->n = 0;
		}
		sge->vaddr = sge->mr->map[sge->m]->segs[sge->n].vaddr;
		sge->length = sge->mr->map[sge->m]->segs[sge->n].length;
	}
}

/**
 * post_one_send - post one RC, UC, or UD send work request
 * @qp: the QP to post on
 * @wr: the work request to send
 *
 * Return: 0 on success, otherwise returns an errno.
 */
static int post_one_send(struct rvt_qp *qp, struct ib_send_wr *wr,
			 int *scheduled)
{
	struct ib_device *ibdev = qp->ibqp.device;
	struct rvt_swqe *wqe;
	u32 next;
	int i;
	int j;
	int acc;
	int ret;
	unsigned long flags;
	struct rvt_lkey_table *rkt;
	struct rvt_pd *pd;
	struct rvt_sge *last_sge;

	spin_lock_irqsave(&qp->s_lock, flags);

	/* Check that state is OK to post send. */
	if (unlikely(!(ib_rvt_state_ops[qp->state] & RVT_POST_SEND_OK)))
		goto bail_inval;

	/* IB spec says that num_sge == 0 is OK. */
	if (wr->num_sge > qp->s_max_sge)
		goto bail_inval;

	/*
	 * Don't allow RDMA reads or atomic operations on UC or
	 * undefined operations.
	 * Make sure buffer is large enough to hold the result for atomics.
	 */
	if (qp->ibqp.qp_type == IB_QPT_UC) {
		if ((unsigned) wr->opcode >= IB_WR_RDMA_READ)
			goto bail_inval;
	} else if (qp->ibqp.qp_type != IB_QPT_RC) {
		/* Check IB_QPT_SMI, IB_QPT_GSI, IB_QPT_UD opcode */
		if (wr->opcode != IB_WR_SEND &&
		    wr->opcode != IB_WR_SEND_WITH_IMM)
			goto bail_inval;
		/* Check UD destination address PD */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)
		if (qp->ibqp.pd != ud_wr(wr)->ah->pd)
#else
		if (qp->ibqp.pd != wr->wr.ud.ah->pd)
#endif
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

	rkt = &ib_to_rvt(ibdev)->lkey_table;
	pd = ibpd_to_rvtpd(qp->ibqp.pd);
	wqe = rvt_get_swqe_ptr(qp, qp->s_head);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)
	if (qp->ibqp.qp_type != IB_QPT_UC &&
	    qp->ibqp.qp_type != IB_QPT_RC)
		memcpy(&wqe->ud_wr, ud_wr(wr), sizeof(wqe->ud_wr));
	else if (wr->opcode == IB_WR_RDMA_WRITE_WITH_IMM ||
		 wr->opcode == IB_WR_RDMA_WRITE ||
		 wr->opcode == IB_WR_RDMA_READ)
		memcpy(&wqe->rdma_wr, rdma_wr(wr), sizeof(wqe->rdma_wr));
	else if (wr->opcode == IB_WR_ATOMIC_CMP_AND_SWP ||
		 wr->opcode == IB_WR_ATOMIC_FETCH_AND_ADD)
		memcpy(&wqe->atomic_wr, atomic_wr(wr), sizeof(wqe->atomic_wr));
	else
		memcpy(&wqe->wr, wr, sizeof(wqe->wr));
#else
	wqe->wr = *wr;
#endif
	wqe->length = 0;
#ifdef HFI2_WQE_PKT_ERRORS
	wqe->pkt_errors = 0;
#endif
	j = 0;
	if (wr->num_sge) {
		last_sge = NULL;
		acc = wr->opcode >= IB_WR_RDMA_READ ?
			IB_ACCESS_LOCAL_WRITE : 0;
		for (i = 0; i < wr->num_sge; i++) {
			u32 length = wr->sg_list[i].length;
			int ok;

			if (length == 0)
				continue;

			/* attempt to coalesce SGEs if lkey == 0 */
			if (last_sge && wr->sg_list[i].lkey == 0 &&
			    ((uint64_t)(last_sge->vaddr + last_sge->length) ==
			     wr->sg_list[i].addr)) {
				/* contiguous, so append length to last SGE */
				last_sge->length += length;
				last_sge->sge_length += length;
			} else {
				ok = rvt_lkey_ok(rkt, pd, &wqe->sg_list[j],
						 &wr->sg_list[i], acc);
				if (!ok)
					goto bail_inval_free;

				if (wr->sg_list[i].lkey == 0)
					last_sge = &wqe->sg_list[j];
				else
					last_sge = NULL;
				j++;
			}
			wqe->length += length;
		}
		wqe->wr.num_sge = j;
	}
	if (qp->ibqp.qp_type == IB_QPT_UC ||
	    qp->ibqp.qp_type == IB_QPT_RC) {
		if (wqe->length > 0x80000000U)
			goto bail_inval_free;
	} else {
		struct hfi_pportdata *ppd;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)
		struct rvt_ah *ah = ibah_to_rvtah(ud_wr(wr)->ah);
#else
		struct rvt_ah *ah = ibah_to_rvtah(wr->wr.ud.ah);
#endif
		u8 sc5, vl;

		ppd = to_hfi_ibp(ibdev, qp->port_num)->ppd;
		if (qp->ibqp.qp_type == IB_QPT_SMI) {
			vl = ppd->sc_to_vlt[15];
		} else {
			sc5 = ppd->sl_to_sc[ah->attr.sl];
			vl = ppd->sc_to_vlt[sc5];
		}
		if (vl < OPA_MAX_VLS)
			if (wqe->length > ppd->vl_mtu[vl])
				goto bail_inval_free;

		atomic_inc(&ah->refcount);
	}
	wqe->ssn = qp->s_ssn++;
	qp->s_head = next;

	trace_hfi2_tx_post_one_send(qp, wqe);

	ret = 0;
	goto bail;

bail_inval_free:
	while (j) {
		struct rvt_sge *sge = &wqe->sg_list[--j];

		rvt_put_mr(sge->mr);
	}
bail_inval:
	ret = -EINVAL;
bail:
	if (!ret && !wr->next) {
		struct hfi2_qp_priv *qp_priv = qp->priv;

		/*
		 * FXRTODO - revisit versus WFR code.
		 * This appears to kick the workqueue if other prior Verbs
		 * requests are still pending, versus returning which will
		 * execute the work queue function directly in this process.
		 * WFR will test if the single SDMA engine selected by
		 * the SC is busy, we currently use one TX Command Queue
		 * so cannot do the more narrow check..
		 */
		if (is_iowait_sdma_busy(&qp_priv->s_iowait)) {
			hfi2_schedule_send(qp);
			*scheduled = 1;
		}
	}

	spin_unlock_irqrestore(&qp->s_lock, flags);
	return ret;
}

/**
 * hfi2_post_send - post a send on a QP
 * @ibqp: the QP to post the send on
 * @wr: the list of work requests to post
 * @bad_wr: the first bad WR is put here
 *
 * This may be called from interrupt context.
 *
 * Return: 0 on success, otherwise returns an errno.
 */
int hfi2_post_send(struct ib_qp *ibqp, struct ib_send_wr *wr,
		     struct ib_send_wr **bad_wr)
{
	struct rvt_qp *qp = ibqp_to_rvtqp(ibqp);
	int err = 0;
	int scheduled = 0;

	for (; wr; wr = wr->next) {
		err = post_one_send(qp, wr, &scheduled);
		if (err) {
			struct ib_device *ibdev = qp->ibqp.device;

			dev_err(&ibdev->dev, "QP[%d]: post_send err %d\n",
				qp->ibqp.qp_num, err);
			*bad_wr = wr;
			goto bail;
		}
	}

	/* Try to do the send work in the caller's context. */
	if (!scheduled) {
		struct hfi2_qp_priv *qp_priv = qp->priv;

		hfi2_do_send(&qp_priv->s_iowait.iowork);
	}

bail:
	return err;
}

/*
 * Make sure the QP is ready and able to accept the given opcode.
 */
static inline bool is_qp_ok(struct hfi2_ibport *ibp,
			    struct rvt_qp *qp, int opcode)
{
	if ((ib_rvt_state_ops[qp->state] & RVT_PROCESS_RECV_OK) &&
	    (((opcode & RVT_OPCODE_QP_MASK) == qp->allowed_ops) ||
	     (opcode == CNP_OPCODE))) {
		return true;
	} else {
		ibp->n_pkt_drops++;
		return false;
	}
}

/**
 * hfi2_ib_rcv - process an incoming packet
 * @packet: data packet information
 *
 * Called from receive kthread upon processing event in RHQ.
 */
void hfi2_ib_rcv(struct hfi2_ib_packet *packet)
{
	union hfi2_packet_header *ph = packet->hdr;
	struct ib_grh *grh = NULL;
	u32 tlen = packet->tlen;
	struct hfi2_ibport *ibp = packet->ibp;
	struct ib_l4_headers *ohdr;
	struct rvt_qp *qp;
	u32 qp_num, dlid;
	u8 opcode, l4;
	bool is_mcast;

	if (packet->etype == RHF_RCV_TYPE_IB) {
		int lnh;

		/* 24 == LRH+BTH+CRC */
		if (unlikely(tlen < 24))
			goto drop;

		/* Check for a valid destination LID (see ch. 7.11.1). */
		dlid = be16_to_cpu(ph->ibh.lrh[1]);

		/*
		 * Check for GRH.
		 * Note, packet->hlen already includes ETH headers but
		 * we reset packet->hlen here so the 9B/16B header parsing
		 * is kept common.
		 */
		lnh = be16_to_cpu(ph->ibh.lrh[0]) & 3;
		if (lnh == HFI1_LRH_BTH) {
			l4 = HFI1_L4_IB_LOCAL;
			/* 8B LRH + 12B BTH */
			packet->hlen = 20;
			ohdr = &ph->ibh.u.oth;
		} else if (lnh == HFI1_LRH_GRH) {
			l4 = HFI1_L4_IB_GLOBAL;
			/* 8B LRH + 40B GRH + 12B BTH */
			packet->hlen = 60;
			ohdr = &ph->ibh.u.l.oth;
			grh = &ph->ibh.u.l.grh;
		} else {
			goto drop;
		}

		is_mcast = (dlid >= HFI1_MULTICAST_LID_BASE) &&
				  (dlid != HFI1_PERMISSIVE_LID);

		trace_hfi2_hdr_rcv(ibp->ibd->dd, ph, false);
	} else if (packet->etype == RHF_RCV_TYPE_BYPASS) {
		/* 32 == 16B + (MGMT L4/BTH) FLIT + CRC FLIT */
		if (unlikely(tlen < 32))
			goto drop;

		l4 = OPA_16B_GET_L4_TYPE(ph);
		dlid = opa_16b_get_dlid((u32 *)ph);
		if (l4 == HFI1_L4_IB_LOCAL) {
			/* 16B + 12B BTH */
			packet->hlen = 28;
			ohdr = &ph->opa16b.u.oth;
		} else if (l4 == HFI1_L4_IB_GLOBAL) {
			/* 16B + 40B GRH + 12B BTH */
			packet->hlen = 68;
			ohdr = &ph->opa16b.u.l.oth;
			grh = &ph->opa16b.u.l.grh;
		} else {
			goto drop;
		}

		is_mcast = (dlid >= HFI1_16B_MULTICAST_LID_BASE) &&
				  (dlid != HFI1_16B_PERMISSIVE_LID);

		trace_hfi2_hdr_rcv(ibp->ibd->dd, ph, true);
	} else {
		dev_err(ibp->dev, "Invalid packet type\n");
		goto drop;
	}
	packet->ohdr = ohdr;
	packet->grh = grh;

	if (l4 == HFI1_L4_IB_GLOBAL) {
		u32 vtf;

		if (grh->next_hdr != IB_GRH_NEXT_HDR)
			goto drop;
		vtf = be32_to_cpu(grh->version_tclass_flow);
		if ((vtf >> IB_GRH_VERSION_SHIFT) != IB_GRH_VERSION)
			goto drop;
	}

	opcode = (be32_to_cpu(ohdr->bth[0]) >> 24) & 0x7f;
#if 0	/* TODO */
	inc_opstats(tlen, &rcd->opstats->stats[opcode]);
#endif

	/* Get the destination QP number. */
	qp_num = be32_to_cpu(ohdr->bth[1]) & HFI1_QPN_MASK;
	dev_dbg(ibp->dev, "PT %d: IB packet %d len %d for PID %d QPN %d\n",
		ibp->port_num, l4, tlen, packet->ctx->pid, qp_num);

	if (is_mcast) {
		struct rvt_mcast *mcast;
		struct rvt_mcast_qp *p;

		if (l4 != HFI1_L4_IB_GLOBAL)
			goto drop;
		mcast = rvt_mcast_find(&ibp->rvp, &grh->dgid);
		if (mcast == NULL)
			goto drop;
		list_for_each_entry_rcu(p, &mcast->qp_list, list) {
			qp = p->qp;
			spin_lock(&qp->r_lock);
			if (likely((is_qp_ok(ibp, qp, opcode))))
				opcode_handler_tbl[opcode](qp, packet);
			spin_unlock(&qp->r_lock);
		}
		/*
		 * Notify hfi1_multicast_detach() if it is waiting for us
		 * to finish.
		 */
		if (atomic_dec_return(&mcast->refcount) <= 1)
			wake_up(&mcast->wait);
	} else {
		rcu_read_lock();
		qp = rvt_lookup_qpn(&ibp->ibd->rdi, &ibp->rvp, qp_num);
		if (!qp) {
			/* FXRTODO: Hazard - qp might be not initialized yet */
			rcu_read_unlock();
			goto drop;
		}

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

static void hfi2_cnp_rcv(struct rvt_qp *qp, struct hfi2_ib_packet *packet)
{
	struct hfi2_ibport *ibp = packet->ibp;

	if (qp->ibqp.qp_type == IB_QPT_UC)
		hfi2_uc_rcv(qp, packet);
	else if (qp->ibqp.qp_type == IB_QPT_UD)
		hfi2_ud_rcv(qp, packet);
	else
		ibp->n_pkt_drops++;
}

static bool process_rcv_packet(struct hfi2_ibport *ibp,
			       struct hfi2_ibrcv *rcv,
			       u64 *rhf_entry,
			       struct hfi2_ib_packet *packet)
{
	u16 idx, off;
	u64 rhf = *rhf_entry;

	packet->ctx = rcv->ctx;
	packet->rhf = rhf;
	packet->sc4_bit = rhf_sc4(rhf) ? BIT(4) : 0;
	packet->hdr = rhf_get_hdr(packet, rhf_entry);
	packet->hlen_9b = rhf_hdr_len(rhf);  /* in bytes */
	packet->hlen = packet->hlen_9b;
	packet->tlen = rhf_pkt_len(rhf);  /* in bytes */
	packet->etype = rhf_rcv_type(rhf);
	idx = rhf_egr_index(rhf);
	off = rhf_egr_buf_offset(rhf);
	if (packet->etype == RHF_RCV_TYPE_IB) {
		packet->ebuf = hfi2_rcv_get_ebuf(rcv, idx, off);
	} else if (packet->etype == RHF_RCV_TYPE_BYPASS) {
		packet->ebuf = hfi2_rcv_get_ebuf(rcv, idx, off);
		packet->hdr = packet->ebuf;
		if (OPA_BYPASS_GET_L2_TYPE(packet->ebuf) != OPA_BYPASS_HDR_16B)
			return false;
	} else {
		/* not supported packet type */
		return false;
	}

	packet->port = rhf_port(rhf);
	dev_dbg(ibp->dev,
		"PT %d: RX type 0x%x hlen %d tlen %d egr %d %d ebuf %p\n",
		packet->port, packet->etype, packet->hlen, packet->tlen,
		idx, off, packet->ebuf);
	/*
	 * As the RHQ may be shared across multiple ports, we perform a lookup here
	 * of the associated IB portdata structure, which can differ from the passed
	 * in argument that contains the Eager Buffer for this RHQ.
	 */
	packet->ibp = &(ibp->ibd->pport[packet->port]);

	trace_hfi2_rx_packet(packet->ibp->ppd->dd,
			     (u64)(packet->ctx),
			     rhf_err_flags(rhf),
			     packet->etype,
			     packet->port,
			     packet->hlen,
			     packet->tlen,
			     idx,
			     off);

	return true;
}

/*
 * This is the RX kthread function, created per Receive Header Queue (RHQ).
 * There may be a RHQ per port (disjoint mode) or shared between the 2 ports.
 */
int hfi2_rcv_wait(void *data)
{
	struct hfi2_ibrcv *rcv = data;
	struct hfi2_ibport *ibp = rcv->ibp;
	struct hfi2_ibdev *ibd = ibp->ibd;
	struct hfi2_ib_packet pkt;
	int rc;
	u64 *rhf_entry;

	dev_info(ibp->dev, "RX kthread %d starting for CTX PID = %#x\n",
		 ibp->port_num, rcv->ctx->pid);
	allow_signal(SIGINT);
	while (!kthread_should_stop()) {
		rhf_entry = NULL;
		rc = _hfi2_rcv_wait(rcv, &rhf_entry);
		if (rc < 0) {
			/* only likely error is DROPPED event */
			dev_err(ibp->dev, "unexpected EQ wait error %d\n", rc);
			break;
		}

		if (rhf_entry) {
			/* parse packet header and call hfi2_rcv() to handle */
			if (process_rcv_packet(ibp, rcv, rhf_entry, &pkt))
				ibd->rhf_rcv_function_map[pkt.etype](&pkt);
			else
				dev_warn(ibp->dev,
					 "PT %d: Unexpected packet! 0x%x\n",
					 pkt.port, pkt.etype);

			/* mark RHF entry as processed */
			hfi2_rcv_advance(rcv, rhf_entry);
		}
	}

	dev_info(ibp->dev, "RX kthread %d stopping\n",
		 ibp->port_num);
	return 0;
}
