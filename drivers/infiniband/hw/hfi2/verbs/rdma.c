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

#include <linux/signal.h>
#include "verbs.h"
#include "packet.h"
#include "hfi_kclient.h"
#include "trace.h"

/*
 * MAX_PKT_RCV is the max # of packets processed at a time
 */
#define MAX_PKT_RECV 64

static void hfi2_cnp_rcv(struct hfi2_ib_packet *packet);
typedef void (*opcode_handler)(struct hfi2_ib_packet *packet);

/*
 * Length of header by opcode, 0 --> not supported
 */
const u8 ib_hdr_len_by_opcode[256] = {
	/* RC */
	[IB_OPCODE_RC_SEND_FIRST]                     = 12 + 8,
	[IB_OPCODE_RC_SEND_MIDDLE]                    = 12 + 8,
	[IB_OPCODE_RC_SEND_LAST]                      = 12 + 8,
	[IB_OPCODE_RC_SEND_LAST_WITH_IMMEDIATE]       = 12 + 8 + 4,
	[IB_OPCODE_RC_SEND_ONLY]                      = 12 + 8,
	[IB_OPCODE_RC_SEND_ONLY_WITH_IMMEDIATE]       = 12 + 8 + 4,
	[IB_OPCODE_RC_RDMA_WRITE_FIRST]               = 12 + 8 + 16,
	[IB_OPCODE_RC_RDMA_WRITE_MIDDLE]              = 12 + 8,
	[IB_OPCODE_RC_RDMA_WRITE_LAST]                = 12 + 8,
	[IB_OPCODE_RC_RDMA_WRITE_LAST_WITH_IMMEDIATE] = 12 + 8 + 4,
	[IB_OPCODE_RC_RDMA_WRITE_ONLY]                = 12 + 8 + 16,
	[IB_OPCODE_RC_RDMA_WRITE_ONLY_WITH_IMMEDIATE] = 12 + 8 + 20,
	[IB_OPCODE_RC_RDMA_READ_REQUEST]              = 12 + 8 + 16,
	[IB_OPCODE_RC_RDMA_READ_RESPONSE_FIRST]       = 12 + 8 + 4,
	[IB_OPCODE_RC_RDMA_READ_RESPONSE_MIDDLE]      = 12 + 8,
	[IB_OPCODE_RC_RDMA_READ_RESPONSE_LAST]        = 12 + 8 + 4,
	[IB_OPCODE_RC_RDMA_READ_RESPONSE_ONLY]        = 12 + 8 + 4,
	[IB_OPCODE_RC_ACKNOWLEDGE]                    = 12 + 8 + 4,
	[IB_OPCODE_RC_ATOMIC_ACKNOWLEDGE]             = 12 + 8 + 4 + 8,
	[IB_OPCODE_RC_COMPARE_SWAP]                   = 12 + 8 + 28,
	[IB_OPCODE_RC_FETCH_ADD]                      = 12 + 8 + 28,
	[IB_OPCODE_RC_SEND_LAST_WITH_INVALIDATE]      = 12 + 8 + 4,
	[IB_OPCODE_RC_SEND_ONLY_WITH_INVALIDATE]      = 12 + 8 + 4,
	/* UC */
	[IB_OPCODE_UC_SEND_FIRST]                     = 12 + 8,
	[IB_OPCODE_UC_SEND_MIDDLE]                    = 12 + 8,
	[IB_OPCODE_UC_SEND_LAST]                      = 12 + 8,
	[IB_OPCODE_UC_SEND_LAST_WITH_IMMEDIATE]       = 12 + 8 + 4,
	[IB_OPCODE_UC_SEND_ONLY]                      = 12 + 8,
	[IB_OPCODE_UC_SEND_ONLY_WITH_IMMEDIATE]       = 12 + 8 + 4,
	[IB_OPCODE_UC_RDMA_WRITE_FIRST]               = 12 + 8 + 16,
	[IB_OPCODE_UC_RDMA_WRITE_MIDDLE]              = 12 + 8,
	[IB_OPCODE_UC_RDMA_WRITE_LAST]                = 12 + 8,
	[IB_OPCODE_UC_RDMA_WRITE_LAST_WITH_IMMEDIATE] = 12 + 8 + 4,
	[IB_OPCODE_UC_RDMA_WRITE_ONLY]                = 12 + 8 + 16,
	[IB_OPCODE_UC_RDMA_WRITE_ONLY_WITH_IMMEDIATE] = 12 + 8 + 20,
	/* UD */
	[IB_OPCODE_UD_SEND_ONLY]                      = 12 + 8 + 8,
	[IB_OPCODE_UD_SEND_ONLY_WITH_IMMEDIATE]       = 12 + 8 + 12
};

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
	[IB_OPCODE_RC_SEND_LAST_WITH_INVALIDATE]      = &hfi2_rc_rcv,
	[IB_OPCODE_RC_SEND_ONLY_WITH_INVALIDATE]      = &hfi2_rc_rcv,
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
 * @copy_last: do a separate copy of the last 8 bytes
 */
void hfi2_copy_sge(struct rvt_sge_state *ss, void *data, u32 length,
		   bool release, bool copy_last)
{
	struct rvt_sge *sge = &ss->sge;
	int in_last = 0;
	int i;

	if (copy_last) {
		if (length > 8) {
			length -= 8;
		} else {
			copy_last = false;
			in_last = true;
		}
	}

again:
	while (length) {
		u32 len = rvt_get_sge_length(sge, length);

		WARN_ON_ONCE(len == 0);
		if (in_last) {
			/* enforce byte transer ordering */
			for (i = 0; i < len; i++)
				((u8 *)sge->vaddr)[i] = ((u8 *)data)[i];
		} else {
			memcpy(sge->vaddr, data, len);
		}
		rvt_update_sge(ss, len, release);
		data += len;
		length -= len;
	}

	if (copy_last) {
		copy_last = false;
		in_last = true;
		length = 8;
		goto again;
	}
}

/*
 * Make sure the QP is ready and able to accept the given opcode.
 */

static inline opcode_handler qp_ok(struct hfi2_ib_packet *packet)
{
	if (!(ib_rvt_state_ops[packet->qp->state] & RVT_PROCESS_RECV_OK))
		return NULL;
	if (((packet->opcode & RVT_OPCODE_QP_MASK) ==
	    packet->qp->allowed_ops) ||
	    (packet->opcode == IB_OPCODE_CNP))
		return opcode_handler_tbl[packet->opcode];

	return NULL;
}

/**
 * hfi2_ib_rcv - process an incoming packet
 * @packet: data packet information
 *
 * Called from receive kthread upon processing event in RHQ.
 */
void hfi2_ib_rcv(struct hfi2_ib_packet *packet)
{
	struct ib_grh *grh = NULL;
	u32 tlen = packet->tlen;
	struct hfi2_ibport *ibp = packet->ibp;
	struct ib_other_headers *ohdr;
	u32 qp_num;
	opcode_handler packet_handler;

	trace_hfi2_hdr_rcv(ibp->ibd->dd, packet->hdr,
			   packet->etype == RHF_RCV_TYPE_BYPASS);
	ohdr = packet->ohdr;
	grh = packet->grh;

#if 0	/* TODO */
	inc_opstats(tlen, &rcd->opstats->stats[opcode]);
#endif
	/* Get the destination QP number. */
	qp_num = ib_bth_get_qpn(packet->ohdr);
	dev_dbg(ibp->dev, "PT %d: IB packet %d len %d for PID %d QPN %d\n",
		ibp->port_num, !!packet->grh, tlen, packet->rcv_ctx->ctx->pid,
		qp_num);

	if (packet->is_mcast) {
		struct rvt_mcast *mcast;
		struct rvt_mcast_qp *p;

		if (!packet->grh)
			goto drop;
		mcast = rvt_mcast_find(&ibp->rvp, &grh->dgid,
				       opa_get_lid(packet->dlid, 9B));
		if (!mcast)
			goto drop;
		list_for_each_entry_rcu(p, &mcast->qp_list, list) {
			packet->qp = p->qp;
			spin_lock_bh(&packet->qp->r_lock);
			packet_handler = qp_ok(packet);
			if (likely(packet_handler)) {
				packet_handler(packet);
			} else {
				spin_unlock_bh(&packet->qp->r_lock);
				goto drop;
			}
			spin_unlock_bh(&packet->qp->r_lock);
		}
		/*
		 * Notify multicast_detach() if it is waiting for us
		 * to finish.
		 */
		if (atomic_dec_return(&mcast->refcount) <= 1)
			wake_up(&mcast->wait);
	} else {
		rcu_read_lock();
		packet->qp = rvt_lookup_qpn(&ibp->ibd->rdi, &ibp->rvp, qp_num);
		if (!packet->qp) {
			/* FXRTODO: Hazard - qp might be not initialized yet */
			rcu_read_unlock();
			goto drop;
		}
		spin_lock_bh(&packet->qp->r_lock);
		packet_handler = qp_ok(packet);
		if (likely(packet_handler)) {
			packet_handler(packet);
		} else {
			spin_unlock_bh(&packet->qp->r_lock);
			rcu_read_unlock();
			goto drop;
		}
		spin_unlock_bh(&packet->qp->r_lock);
		rcu_read_unlock();
	}
	return;

drop:
	ibp->rvp.n_pkt_drops++;
}

static void hfi2_cnp_rcv(struct hfi2_ib_packet *packet)
{
	struct rvt_qp *qp = packet->qp;
	struct hfi2_ibport *ibp = packet->ibp;

	if (qp->ibqp.qp_type == IB_QPT_UC)
		hfi2_uc_rcv(packet);
	else if (qp->ibqp.qp_type == IB_QPT_UD)
		hfi2_ud_rcv(packet);
	else
		ibp->rvp.n_pkt_drops++;
}

static int hfi2_setup_9B_packet(struct hfi2_ib_packet *packet)
{
	struct ib_header *hdr;
	u8 lnh;

	packet->hlen = rhf_hdr_len(packet->rhf);
	hdr = packet->hdr;

	/* 24 == LRH+BTH+CRC */
	if (unlikely(packet->tlen < 24))
		goto drop;

	lnh = ib_get_lnh(hdr);
	if (lnh == HFI1_LRH_BTH) {
		packet->ohdr = &hdr->u.oth;
		packet->grh = NULL;
	} else if (lnh == HFI1_LRH_GRH) {
		u32 vtf;

		packet->ohdr = &hdr->u.l.oth;
		packet->grh = &hdr->u.l.grh;
		if (packet->grh->next_hdr != IB_GRH_NEXT_HDR)
			goto drop;
		vtf = be32_to_cpu(packet->grh->version_tclass_flow);
		if ((vtf >> IB_GRH_VERSION_SHIFT) != IB_GRH_VERSION)
			goto drop;
	} else {
		goto drop;
	}

	packet->opcode = ib_bth_get_opcode(packet->ohdr);
	packet->payload = packet->ebuf;
	packet->slid = ib_get_slid(hdr);
	packet->dlid = ib_get_dlid(hdr);

	packet->is_mcast = false;
	if (unlikely((packet->dlid >= be16_to_cpu(IB_MULTICAST_LID_BASE)) &&
		     (packet->dlid != be16_to_cpu(IB_LID_PERMISSIVE)))) {
		packet->dlid += opa_get_mcast_base(OPA_MCAST_NR) -
				be16_to_cpu(IB_MULTICAST_LID_BASE);
		packet->is_mcast = true;
	}
	packet->sl = ib_get_sl(hdr);
	packet->sc = hfi2_9B_get_sc5(hdr, packet->rhf);
	packet->pad = ib_bth_get_pad(packet->ohdr);
	packet->extra_byte = 0;
	packet->fecn = ib_bth_get_fecn(packet->ohdr);
	if (packet->fecn)
		packet->ibp->prescan_9B = true;
	packet->pkey = ib_bth_get_pkey(packet->ohdr);
	packet->migrated = ib_bth_is_migration(packet->ohdr);
	return 0;
drop:
	dev_err(packet->ibp->dev, "%s: packet dropped\n", __func__);
	packet->ibp->rvp.n_pkt_drops++;
	return -EINVAL;
}

static int hfi2_setup_bypass_packet(struct hfi2_ib_packet *packet)
{
	/*
	 * Bypass packets have a different header/payload split
	 * compared to an IB packet.
	 * Current split is set such that 16 bytes of the actual
	 * header is in the header buffer and the remining is in
	 * the eager buffer. We chose 16 since hfi1 driver only
	 * supports 16B bypass packets and we will be able to
	 * receive the entire LRH with such a split.
	 */

	u8 l4;
	u8 grh_len;

	if (unlikely(packet->tlen < 32))
		goto drop;

	l4 = hfi2_16B_get_l4(packet->hdr);
	if (l4 == HFI1_L4_IB_LOCAL) {
		grh_len = 0;
		packet->ohdr = packet->ebuf;
		packet->grh = NULL;
	} else if (l4 == HFI1_L4_IB_GLOBAL) {
		u32 vtf;

		grh_len = sizeof(struct ib_grh);
		packet->ohdr = packet->ebuf + grh_len;
		packet->grh = packet->ebuf;
		if (packet->grh->next_hdr != IB_GRH_NEXT_HDR)
			goto drop;
		vtf = be32_to_cpu(packet->grh->version_tclass_flow);
		if ((vtf >> IB_GRH_VERSION_SHIFT) != IB_GRH_VERSION)
			goto drop;
	} else {
		goto drop;
	}

	/* Query commonly used fields from packet header */
	packet->opcode = ib_bth_get_opcode(packet->ohdr);
	packet->hlen = ib_hdr_len_by_opcode[packet->opcode] + 8 + grh_len;
	packet->payload = packet->ebuf + packet->hlen - (4 * sizeof(u32));
	packet->slid = hfi2_16B_get_slid(packet->hdr);
	packet->dlid = hfi2_16B_get_dlid(packet->hdr);
	packet->is_mcast = false;
	if (unlikely(hfi2_is_16B_mcast(packet->dlid))) {
		packet->dlid += opa_get_mcast_base(OPA_MCAST_NR) -
				opa_get_lid(opa_get_mcast_base(OPA_MCAST_NR),
					    16B);
		packet->is_mcast = true;
	}
	packet->sc = hfi2_16B_get_sc(packet->hdr);
	packet->sl = packet->ibp->ppd->sc_to_sl[packet->sc];
	packet->pad = hfi2_16B_bth_get_pad(packet->ohdr);
	packet->extra_byte = SIZE_OF_LT;
	packet->fecn = hfi2_16B_get_fecn(packet->hdr);
	if (packet->fecn)
		packet->ibp->prescan_16B = true;
	packet->pkey = hfi2_16B_get_pkey(packet->hdr);
	packet->migrated = opa_bth_is_migration(packet->ohdr);
	return 0;
drop:
	dev_err(packet->ibp->dev, "%s: packet dropped\n", __func__);
	packet->ibp->rvp.n_pkt_drops++;
	return -EINVAL;
}

static bool process_rcv_packet(struct hfi2_ibport *ibp,
			       struct hfi2_ibrcv *rcv,
			       u64 *rhf_entry,
			       struct hfi2_ib_packet *packet)
{
	u16 idx = 0, off = 0;
	u64 rhf = *rhf_entry;
	bool have_egr_buf;

	packet->ibp = ibp;
	packet->rcv_ctx = rcv;
	packet->rhf = rhf;
	packet->sc4_bit = rhf_sc4(rhf) ? BIT(4) : 0;
	packet->hdr = rhf_get_hdr(packet, rhf_entry);
	packet->tlen = rhf_pkt_len(rhf);  /* in bytes */
	packet->port = rhf_port(rhf);
	packet->etype = rhf_rcv_type(rhf);
	have_egr_buf = !!rhf_use_egr_bfr(rhf);
	if (have_egr_buf) {
		idx = rhf_egr_index(rhf);
		off = rhf_egr_buf_offset(rhf);
		packet->ebuf = hfi2_rcv_get_ebuf(rcv, idx, off);
	} else {
		/* zero payload or RHF error */
		packet->ebuf = NULL;
	}
	if (packet->etype == RHF_RCV_TYPE_IB) {
		if (hfi2_setup_9B_packet(packet))
			return false;
	} else if (packet->etype == RHF_RCV_TYPE_BYPASS) {
		if (OPA_BYPASS_GET_L2_TYPE(packet->hdr) != OPA_BYPASS_HDR_16B)
			return false;
		/* 16B writes part of header into eager buffer */
		if (!have_egr_buf)
			return false;
		if (hfi2_setup_bypass_packet(packet))
			return false;
	} else {
		/* not supported packet type */
		return false;
	}

	dev_dbg(ibp->dev,
		"PT %d: RX type 0x%x hlen %d tlen %d egr %d %d %d ebuf %p flags 0x%llx rhf 0x%llx\n",
		packet->port, packet->etype, packet->hlen, packet->tlen,
		have_egr_buf, idx, off, packet->ebuf, rhf_err_flags(rhf), rhf);

	trace_hfi2_rx_packet(packet->ibp->ppd->dd,
			     (u64)(packet->rcv_ctx->ctx),
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
 * Perform any handling of hardware reported receive errors.
 * We only have handling for TID_ERR, which for RC QPs we preemptively
 * schedule a NAK to the sender.
 */
static void rcv_hdrerr(struct hfi2_ib_packet *packet)
{
	struct hfi2_ibport *ibp = packet->ibp;
	struct rvt_dev_info *rdi = &ibp->ibd->rdi;
	u32 qp_num;

	if (!(packet->rhf & RHF_TID_ERR)) {
		if (packet->rhf & RHF_LEN_ERR)
			ibp->stats->sps_lenerrs++;
		return;
	}

	ibp->stats->sps_buffull++;
	/* Get the destination QP number. */
	qp_num = ib_bth_get_qpn(packet->ohdr);
	if (!packet->is_mcast) {
		struct rvt_qp *qp;
		unsigned long flags;

		rcu_read_lock();
		qp = rvt_lookup_qpn(rdi, &ibp->rvp, qp_num);
		if (!qp) {
			rcu_read_unlock();
			return;
		}
		packet->qp = qp;

		/*
		 * Handle only RC QPs - for other QP types drop error
		 * packet.
		 */
		spin_lock_irqsave(&qp->r_lock, flags);

		/* Check for valid receive state. */
		if (!(ib_rvt_state_ops[qp->state] & RVT_PROCESS_RECV_OK))
			ibp->rvp.n_pkt_drops++;

		if (qp->ibqp.qp_type == IB_QPT_RC)
			hfi2_rc_hdrerr(packet);

		spin_unlock_irqrestore(&qp->r_lock, flags);
		rcu_read_unlock();
	}
}

/*
 * prescan_rxq is called when FECNs are being received due to congestion
 * This function looks ahead at all the valid events in the event queue and
 * sends a CNP if a FECN is set. It essentially does a prescan to send a
 * CNP as soon as possible instead of going through the normal receive
 * processing pipeline. This helps react to congestion quickly.
 */
void prescan_rxq(struct hfi2_ibrcv *rcv)
{
	struct hfi2_ib_packet packet_data;
	struct rvt_qp *qp;
	struct ib_header *hdr;
	struct hfi2_ibport *ibp = rcv->ibp;
	struct hfi2_ib_packet *packet = &packet_data;
	bool dropped = false;
	int n = 0;
	u8 lnh;
	u32 src_qp, qp_num;
	u16 pkey, idx = 0, off = 0;
	u64 *rhf_entry;

	packet->ibp = rcv->ibp;

	/* loop until an invalid event is found */
	while (hfi_eq_peek_nth(&rcv->eq, &rhf_entry, n++, &dropped) > 0) {
		packet->hdr = rhf_get_hdr(packet, rhf_entry);
		hdr = packet->hdr;
		packet->rhf = *rhf_entry;

		if (rhf_rcv_type(*rhf_entry) == RHF_RCV_TYPE_IB) {
			lnh = ib_get_lnh(packet->hdr);
			if (lnh == HFI1_LRH_BTH) {
				packet->ohdr = &hdr->u.oth;
				packet->grh = NULL;
			} else if (lnh == HFI1_LRH_GRH) {
				packet->ohdr = &hdr->u.l.oth;
				packet->grh = &hdr->u.l.grh;
			} else {
				continue;
			}
			if (ib_bth_get_fecn(packet->ohdr)) {
				packet->slid = ib_get_slid(packet->hdr);
				packet->dlid = ib_get_dlid(packet->hdr);
				packet->sc = hfi2_9B_get_sc5(hdr, packet->rhf);
				src_qp = ib_get_sqpn(packet->ohdr);
				pkey = ib_bth_get_pkey(packet->ohdr);
				qp_num = ib_bth_get_qpn(packet->ohdr);
				rcu_read_lock();
				qp = rvt_lookup_qpn(&ibp->ibd->rdi,
						    &ibp->rvp, qp_num);
				if (!qp) {
					rcu_read_unlock();
					continue;
				}
				packet->qp = qp;
				hfi_return_cnp(packet, src_qp, pkey);
				rcu_read_unlock();
				/*
				 * clear the FECN bit to avoid CNP being sent
				 * twice while the packet in processed by the
				 * packet receive handler
				 */
				ib_bth_clear_fecn(packet->ohdr);
			}
		} else if (rhf_rcv_type(*rhf_entry) == RHF_RCV_TYPE_BYPASS) {
			u8 l4;

			if (OPA_BYPASS_GET_L2_TYPE(packet->hdr) !=
					OPA_BYPASS_HDR_16B)
				continue;
			if (!(bool)rhf_use_egr_bfr(*rhf_entry))
				continue;
			idx = rhf_egr_index(*rhf_entry);
			off = rhf_egr_buf_offset(*rhf_entry);
			packet->ebuf = hfi2_rcv_get_ebuf_ptr(rcv, idx, off);
			l4 = hfi2_16B_get_l4(packet->hdr);
			if (l4 == HFI1_L4_IB_LOCAL) {
				packet->ohdr = packet->ebuf;
				packet->grh = NULL;
			} else if (l4 == HFI1_L4_IB_GLOBAL) {
				packet->ohdr = packet->ebuf +
					       sizeof(struct ib_grh);
				packet->grh = packet->ebuf;
			} else {
				continue;
			}
			if (hfi2_16B_get_fecn(packet->hdr)) {
				packet->slid = hfi2_16B_get_slid(packet->hdr);
				packet->dlid = hfi2_16B_get_dlid(packet->hdr);
				packet->sc = hfi2_16B_get_sc(packet->hdr);
				pkey = hfi2_16B_get_pkey(packet->hdr);
				src_qp = ib_get_sqpn(packet->ohdr);
				qp_num = ib_bth_get_qpn(packet->ohdr);
				rcu_read_lock();
				qp = rvt_lookup_qpn(&ibp->ibd->rdi, &ibp->rvp,
						    qp_num);
				if (!qp) {
					rcu_read_unlock();
					continue;
				}
				packet->qp = qp;
				hfi_return_cnp_bypass(packet, src_qp, pkey);
				rcu_read_unlock();
				/*
				 * clear the FECN bit to avoid CNP being sent
				 * twice while the packet in processed by the
				 * packet receive handler
				 */
				hfi2_16B_clear_fecn(packet->hdr);
			}
		}
	}
	ibp->prescan_9B = false;
	ibp->prescan_16B = false;
}

/*
 * This is the RX kthread function, created per Receive Header Queue (RHQ).
 */
int hfi2_rcv_wait(void *data)
{
	struct hfi2_ibrcv *rcv = data;
	struct hfi2_ibport *ibp = rcv->ibp;
	struct hfi2_ibdev *ibd = ibp->ibd;
	struct hfi2_ib_packet pkt;
	int rc;
	bool hdr_valid;
	u64 *rhf_entry;
	u64 rhf_err;

	dev_info(ibp->dev, "RX kthread %d starting for CTX PID = %#x\n",
		 ibp->port_num, rcv->ctx->pid);
	allow_signal(SIGINT);
	pkt.numpkt = 0;
	INIT_LIST_HEAD(&rcv->wait_list);
	while (!kthread_should_stop()) {
		rc = _hfi2_rcv_wait(rcv, &rhf_entry);
		if (rc < 0) {
			/* only likely error is DROPPED event */
			dev_err(ibp->dev, "unexpected EQ wait error %d\n", rc);
			ibp->stats->sps_hdrfull++;
			break;
		}
		if (rc > 0) {
			/* First parse packet header */
			hdr_valid = process_rcv_packet(ibp, rcv,
						       rhf_entry, &pkt);
			/*
			 * As long as there are FECNs being received, look
			 * ahead for FECNs by calling prescan
			 */
			if (ibp->prescan_9B) {
				prescan_rxq(rcv);
				pkt.fecn = false;
			}

			/*
			 * Test for error bits in RHF. If none, then call the
			 * IB receive logic.
			 */
			rhf_err = rhf_err_flags(pkt.rhf);
			if (rhf_err) {
				ibp->stats->n_rhf_errors++;
				dev_dbg(ibp->dev,
					"PID %d: RHF error 0x%llx, %lld\n",
					rcv->ctx->pid, rhf_err_flags(pkt.rhf),
					ibp->stats->n_rhf_errors);
				/* Handle error if header is supported */
				if (hdr_valid)
					rcv_hdrerr(&pkt);
			} else if (hdr_valid) {
				ibd->rhf_rcv_function_map[pkt.etype](&pkt);
				pkt.numpkt++;
				if (unlikely((pkt.numpkt &
					      (MAX_PKT_RECV - 1)) == 0))
					cond_resched();
			} else {
				dev_warn(ibp->dev,
					 "PT %d: Unexpected packet! 0x%x\n",
					 pkt.port, pkt.etype);
			}

			/* mark RHF entry as processed */
			hfi2_rcv_advance(rcv, rhf_entry);
		} else {
			if (ibp->ppd->dd->emulation) {
				cpu_relax();
				schedule();
				msleep(20);
			}
		}
		/* Process any pending acks when there are no events pending */
		if (pkt.numpkt && !hfi_eq_wait_condition(&rcv->eq)) {
			process_rcv_qp_work(&pkt);
			pkt.numpkt = 0;
		}
	}
	dev_info(ibp->dev, "RX kthread %d stopping\n",
		 ibp->port_num);
	return 0;
}
