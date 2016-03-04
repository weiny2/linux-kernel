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

#include <linux/delay.h>
#include "verbs.h"
#include "packet.h"
#include <rdma/opa_core_ib.h>
#include <rdma/hfi_args.h>
#include <rdma/hfi_eq.h>
#include <rdma/hfi_pt.h>
#include <rdma/hfi_rx.h>
#include <rdma/hfi_tx.h>

static bool disable_ofed_dma;

#define OPA_IB_CQ_FULL_RETRIES		10
#define OPA_IB_CQ_FULL_DELAY_MS		1
#define OPA_IB_EAGER_COUNT		2048 /* minimum Eager entries */
#define OPA_IB_EAGER_COUNT_ORDER	9 /* log2 of above - 2 */
#define OPA_IB_EAGER_SIZE		(PAGE_SIZE * 8)
#define OPA_IB_EAGER_PT_FLAGS		(PTL_MAY_ALIGN | PTL_MANAGE_LOCAL)

/* TODO - delete and make common for all kernel clients */
static int _hfi_eq_alloc(struct hfi_ctx *ctx, struct hfi_cq *cq,
			 spinlock_t *cq_lock,
			 struct opa_ev_assign *eq_alloc,
			 struct hfi_eq *eq)
{
	u32 *eq_head_array, *eq_head_addr;
	u64 *eq_entry = NULL;
	int rc;

	eq_alloc->base = (u64)vzalloc(eq_alloc->count * HFI_EQ_ENTRY_SIZE);
	if (!eq_alloc->base)
		return -ENOMEM;
	eq_alloc->mode = OPA_EV_MODE_BLOCKING;
	rc = hfi_cteq_assign(ctx, eq_alloc);
	if (rc < 0) {
		vfree((void *)eq_alloc->base);
		goto err;
	}
	eq_head_array = ctx->eq_head_addr;
	/* Reset the EQ SW head */
	eq_head_addr = &eq_head_array[eq_alloc->ev_idx];
	*eq_head_addr = 0;
	eq->idx = eq_alloc->ev_idx;
	eq->base = (void *)eq_alloc->base;
	eq->count = eq_alloc->count;

	/* Check on EQ 0 NI 0 for a PTL_CMD_COMPLETE event */
	hfi_eq_wait_timed(ctx, &ctx->eq_zero[0], HFI_EQ_WAIT_TIMEOUT_MS,
			  &eq_entry);
	if (eq_entry) {
		unsigned long flags;

		spin_lock_irqsave(cq_lock, flags);
		hfi_eq_advance(ctx, cq, &ctx->eq_zero[0], eq_entry);
		spin_unlock_irqrestore(cq_lock, flags);
	} else {
		rc = -EIO;
	}

err:
	return rc;
}

/* TODO - delete and make common for all kernel clients */
static void _hfi_eq_release(struct hfi_ctx *ctx, struct hfi_cq *cq,
			    spinlock_t *cq_lock, struct hfi_eq *eq)
{
	u64 *eq_entry = NULL, done;

	hfi_cteq_release(ctx, 0, eq->idx, (u64)&done);
	/* Check on EQ 0 NI 0 for a PTL_CMD_COMPLETE event */
	hfi_eq_wait_timed(ctx, &ctx->eq_zero[0], HFI_EQ_WAIT_TIMEOUT_MS,
			  &eq_entry);
	if (eq_entry) {
		unsigned long flags;

		spin_lock_irqsave(cq_lock, flags);
		hfi_eq_advance(ctx, cq, &ctx->eq_zero[0], eq_entry);
		spin_unlock_irqrestore(cq_lock, flags);
	}

	vfree(eq->base);
	eq->base = NULL;
}

/*
 * compute how many IOVECs we need at most for sending
 *
 * TODO may need to determine packet boundaries depending on how auto-header
 * optimization is implemented in upper layers (need add'l header IOVECs)
 */
static int qp_max_iovs(struct hfi2_ibport *ibp, struct hfi2_qp *qp,
		       int *out_niovs)
{
	int niovs, i;
	size_t off, segsz, payload_bytes;
	struct hfi2_sge *sge;
	struct hfi2_sge_state *ss = qp->s_cur_sge;

	niovs = 1; /* one IOV for header */

	/* step through SGEs to send in this packet */
	sge = &qp->s_sge.sge;
	for (payload_bytes = 0, i = 0;
	     (payload_bytes < qp->s_cur_size) && sge;
	     payload_bytes += sge->sge_length, sge = ss->sg_list + i, i++) {

		/* we only support kernel virtual addresses for now */
		if ((u64)sge->vaddr < PAGE_OFFSET) {
			dev_err(ibp->dev,
				"PT %d: DMA send with UVA %p not supported\n",
				ibp->port_num, sge->vaddr);
			return -EFAULT;
		}

		if (sge->mr->mapsz) {
			segsz = 1 << sge->mr->page_shift;
			/* TODO - store sge->offset to remove this */
			off = sge->mr->map[sge->m]->segs[sge->n].length -
				sge->length;
			niovs += ((off + sge->sge_length + segsz - 1) / segsz);
			/* TODO - above needs to limit to s_cur_size? */
		} else {
			niovs++;
		}
	}

	*out_niovs = niovs;
	return 0;
}

/*
 * update QP state that is tracking progress within the SG list to note
 * that length bytes have been sent
 */
static void qp_update_sge(struct hfi2_qp *qp, u32 length)
{
	struct hfi2_sge_state *ss = qp->s_cur_sge;
	struct hfi2_sge *sge = &ss->sge;

	sge->vaddr += length;
	sge->length -= length;
	sge->sge_length -= length;
	if (sge->sge_length == 0) {
		if (--ss->num_sge)
			*sge = *ss->sg_list++;
	} else if (sge->length == 0 && sge->mr->lkey) {
		if (++sge->n >= HFI2_SEGSZ) {
			if (++sge->m >= sge->mr->mapsz)
				return;
			sge->n = 0;
		}
		sge->vaddr = sge->mr->map[sge->m]->segs[sge->n].vaddr;
		sge->length = sge->mr->map[sge->m]->segs[sge->n].length;
	}
}

/*
 * read QP's SG list state for next address and length to send
 */
static void qp_read_sge(struct hfi2_qp *qp, uint64_t *start,
			uint32_t *length)
{
	uint32_t iov_length;
	struct hfi2_sge *sge = &qp->s_sge.sge;

	/* use length from next MR segment */
	iov_length = sge->length;
	/* the SGE can describe less than the MR segment */
	if (iov_length > sge->sge_length)
		iov_length = sge->sge_length;
	/* TODO SGE can be larger than total_length? */
	if (iov_length > qp->s_cur_size)
		iov_length = qp->s_cur_size;

	*length = iov_length;
	*start = (uint64_t)sge->vaddr;

	/* update internal state */
	qp_update_sge(qp, iov_length);
}

/*
 * TODO - keep around for now, in case this hack
 * has value again during bring-up
 */
static int __attribute__ ((unused))
send_wqe_pio(struct hfi2_ibport *ibp, struct hfi2_swqe *wqe)
{
	int ret;
	unsigned long flags;

	if (wqe->use_sc15)
		return -EINVAL;

	if (wqe->length > wqe->pmtu) {
		dev_err(ibp->dev,
			"PT %d: multi-packet PIO not supported (%d > %d)!\n",
			ibp->port_num, wqe->length, wqe->pmtu);
		return -EINVAL;
	}

	/* low-level PIO support routines don't do IOVEC */
	if (wqe->s_sge->num_sge > 1) {
		dev_err(ibp->dev,
			"PT %d: PIO-IOVEC not implemented!\n",
			ibp->port_num);
		return -EIO;
	}

	/* only support kernel virtual addresses */
	if ((u64)wqe->sg_list[0].vaddr < PAGE_OFFSET) {
		dev_err(ibp->dev,
			"PT %d: PIO send with UVA not supported\n",
			ibp->port_num);
		return -EFAULT;
	}

	dev_dbg(ibp->dev, "PT %d: PIO send hdr %p data %p %d\n",
		ibp->port_num, wqe->s_hdr,
		wqe->sg_list[0].vaddr, wqe->length);

	/* send the WQE via PIO path */
	spin_lock_irqsave(&ibp->cmdq_tx_lock, flags);
	ret = hfi_tx_9b_kdeth_cmd_pio(&ibp->cmdq_tx, wqe->s_ctx,
				    wqe->s_hdr, 8 + (wqe->s_hdrwords << 2),
				    wqe->sg_list[0].vaddr,
				    wqe->length, ibp->port_num,
				    wqe->sl, 0, KDETH_9B_PIO);
	spin_unlock_irqrestore(&ibp->cmdq_tx_lock, flags);

	/* caller must handle retransmit due to no slots (-EAGAIN) */
	return ret;
}

static void hfi2_send_event(struct hfi_eq *eq_tx, void *data)
{
	struct hfi2_ibport *ibp = data;
	struct hfi2_qp *qp;
	struct hfi2_swqe *wqe;
	struct hfi2_wqe_iov *wqe_iov;
	unsigned long flags;
	union initiator_EQEntry *eq_entry;

next_event:
	eq_entry = NULL;
	hfi_eq_peek(ibp->ctx, eq_tx, (uint64_t **)&eq_entry);
	if (!eq_entry)
		return;

	if (eq_entry->event_kind != NON_PTL_EVENT_TX_COMPLETE) {
		dev_warn(ibp->dev, "PT %d: unexpected EQ kind %x\n",
			 ibp->port_num, eq_entry->event_kind);
		goto eq_advance;
	}

	wqe_iov = (struct hfi2_wqe_iov *)eq_entry->user_ptr;
	wqe = wqe_iov->wqe;

	/* ACKs don't have backing wqe */
	if (!wqe) {
		kfree(wqe_iov);
		goto eq_advance;
	}

	qp = wqe->s_qp;
	dev_dbg(ibp->dev,
		"PT %d: TX event %p, fail %d, for QPN %d, remain %d\n",
		ibp->port_num, eq_entry, eq_entry->fail_type,
		qp->ibqp.qp_num, wqe_iov->remaining_bytes);

	if (eq_entry->fail_type)
		wqe->pkt_errors++;

	/* if final packet, tell verbs if success or failure */
	if (!wqe_iov->remaining_bytes) {
		spin_lock(&qp->s_lock);
		if (qp->ibqp.qp_type == IB_QPT_RC)
			hfi2_rc_send_complete(qp, &wqe_iov->ph.ibh);
		else if (wqe->pkt_errors)
			hfi2_send_complete(qp, wqe, IB_WC_FATAL_ERR);
		else
			hfi2_send_complete(qp, wqe, IB_WC_SUCCESS);
		spin_unlock(&qp->s_lock);
	}
	kfree(wqe_iov);

eq_advance:
	spin_lock_irqsave(&ibp->cmdq_rx_lock, flags);
	hfi_eq_advance(ibp->ctx, &ibp->cmdq_rx, eq_tx,
		       (uint64_t *)eq_entry);
	spin_unlock_irqrestore(&ibp->cmdq_rx_lock, flags);

	goto next_event;
}

static int build_iovec_array(struct hfi2_ibport *ibp, struct hfi2_qp *qp,
			     struct hfi2_wqe_iov *wqe_iov, int *out_niovs,
			     uint32_t *iov_bytes)
{
	int i, niovs = *out_niovs;
	uint32_t payload_bytes;
	union base_iovec *iov;
	struct hfi2_swqe *wqe = wqe_iov->wqe;

	iov = wqe_iov->iov;
	/* first entry is IB header */
	iov[0].length = (wqe->s_hdrwords << 2);
	iov[0].use_9b = !wqe->use_16b;
	iov[0].sp = 1;
	iov[0].v = 1;
	iov[0].start = (uint64_t)(&wqe_iov->ph);

	/* write IOVEC entries */
	payload_bytes = 0;
	for (i = 1; payload_bytes < qp->s_cur_size && i < niovs; i++) {
		uint64_t iov_start;
		uint32_t iov_length;

		/* read current SGE address/length, updates internal state */
		qp_read_sge(qp, &iov_start, &iov_length);

		/*
		 * If payload is now larger than MTU, start new packet;
		 * caller doesn't do this currently, but test for it
		 * TODO - may need if auto-header optimization
		 */
		if (payload_bytes + iov_length > wqe->pmtu && i > 1) {
			dev_err(ibp->dev,
				"PT %d: large DMA not implemented, %d > %d MTU!\n",
				ibp->port_num,
				payload_bytes + iov_length, wqe->pmtu);
			return -EIO;
		}

		iov[i].length = iov_length;
		iov[i].use_9b = !wqe->use_16b;
		iov[i].ep = 0;
		iov[i].sp = 0;
		iov[i].v = 1;
		iov[i].start = iov_start;
		payload_bytes += iov_length;
	}

	/* adjust iovs actually used, earlier calculation can be larger */
	niovs = i;
	iov[niovs - 1].ep = 1;

	*out_niovs = niovs;
	*iov_bytes = payload_bytes;
	return 0;
}

#ifdef HFI_VERBS_TEST
bool hfi2_drop_check(uint64_t *count, uint64_t pkt_num)
{
	(*count)++;

	if (*count % pkt_num == 0) {
		*count = 0;
		return true;
	}
	return false;
}

bool hfi2_drop_ack(uint64_t pkt_num)
{
	static uint64_t count;

	return hfi2_drop_check(&count, pkt_num);
}
#endif

int hfi2_send_ack(struct hfi2_ibport *ibp, struct hfi2_qp *qp,
		  struct hfi2_ib_header *hdr, size_t hwords)
{
	int ret, ndelays = 0;
	unsigned long flags;
	uint8_t dma_cmd = GENERAL_DMA;
	struct hfi2_wqe_iov *wqe_iov = NULL;
	uint32_t num_iovs = 1, length = 0;
	union base_iovec *iov;
	uint8_t sl = qp->remote_ah_attr.sl;

#ifdef HFI_VERBS_TEST
	if (hfi2_drop_ack(5)) {
		dev_dbg(ibp->dev, "Droping %s with PSN = %d\n",
			(qp->r_nak_state) ? "NAK" : "ACK",
			 be32_to_cpu(hdr->u.oth.bth[2]));
		return -1;
	}
#endif
	wqe_iov = kzalloc(sizeof(*wqe_iov) + sizeof(*wqe_iov->iov) * num_iovs,
			  GFP_ATOMIC);
	if (!wqe_iov)
		return -ENOMEM;

	/* hfi_tx_cmd_bypass_dma is asynchronous. Don't use stack memory */
	/* FXRTODO: Remove memcpy when implementing STL--6302 */
	memcpy(&wqe_iov->ph.ibh, hdr, sizeof(*hdr));

	iov = wqe_iov->iov;
	wqe_iov->wqe = NULL;
	wqe_iov->remaining_bytes = 0;

	/* first entry is IB header */
	iov[0].length = (hwords << 2);
	iov[0].use_9b = 1;
	iov[0].sp = 1;
	iov[0].ep = 1;
	iov[0].v = 1;
	iov[0].start = (uint64_t)&wqe_iov->ph.ibh;

	dev_info(ibp->dev, "PT %d: cmd %d len 0 n_iov 1 sent %d, length (iov[0]) = %u\n",
		 ibp->port_num, dma_cmd, length, iov[0].length);

_tx_cmd:
	/* send with GENERAL or MGMT DMA */
	spin_lock_irqsave(&ibp->cmdq_tx_lock, flags);
	ret = hfi_tx_cmd_bypass_dma(&ibp->cmdq_tx, qp->s_ctx,
				    (uint64_t)wqe_iov->iov, num_iovs,
				    (uint64_t)wqe_iov, PTL_MD_RESERVED_IOV,
				    &ibp->send_eq, HFI_CT_NONE,
				    ibp->port_num, sl, 0,
				    HDR_EXT, dma_cmd);
	spin_unlock_irqrestore(&ibp->cmdq_tx_lock, flags);

	if (ret == -EAGAIN) {
		/* FXRTODO - remove delay via deferred send WQE logic */
		if (ndelays >= OPA_IB_CQ_FULL_RETRIES) {
			dev_warn(ibp->dev,
				 "PT %d: TX CQ still full after %d retries!\n",
				 ibp->port_num, ndelays);
			goto err;
		} else {
			ndelays++;
			mdelay(OPA_IB_CQ_FULL_DELAY_MS);
			goto _tx_cmd;
		}
	} else if (ret < 0) {
		goto err;
	} else if (ndelays) {
		dev_info(ibp->dev, "PT %d: full TX CQ required %d retries\n",
			 ibp->port_num, ndelays);
	}

	return 0;
err:
	kfree(wqe_iov);
	return ret;
}

int hfi2_send_wqe(struct hfi2_ibport *ibp, struct hfi2_qp *qp,
		  struct hfi2_swqe *wqe)
{
	int ret, ndelays = 0;
	unsigned long flags;
	uint8_t dma_cmd, eth_size = 0;
	struct hfi2_wqe_iov *wqe_iov = NULL;
	uint32_t num_iovs, length = 0;
	uint64_t start = 0;
	bool use_ofed_dma;

	BUG_ON(!wqe); /* TODO - delete me after RC acks stable */

	dma_cmd = (wqe->use_sc15) ? MGMT_DMA : GENERAL_DMA;

	ret = qp_max_iovs(ibp, qp, &num_iovs);
	if (ret)
		return ret;

	/*
	 * use OFED_DMA command if possible (header + single IOVEC payload)
	 * TODO - WFR-like AHG optimization for Middles
	 * TODO - add 16B support
	 */
	use_ofed_dma = (num_iovs == 2 && dma_cmd != MGMT_DMA &&
			!disable_ofed_dma && !wqe->use_16b);
	if (use_ofed_dma) {
		/* switch to appropriate OFED_DMA command */
		if (wqe->lnh == HFI1_LRH_GRH) {
			dma_cmd = OFED_9B_DMA_GRH;
			eth_size = (wqe->s_hdrwords - 15) << 2;
		} else {
			dma_cmd = OFED_9B_DMA;
			eth_size = (wqe->s_hdrwords - 5) << 2;
		}
		num_iovs = 0;
	}

	wqe_iov = kzalloc(sizeof(*wqe_iov) + sizeof(*wqe_iov->iov) * num_iovs,
			  GFP_ATOMIC);
	if (!wqe_iov)
		return -ENOMEM;
	wqe_iov->wqe = wqe;
	wqe_iov->remaining_bytes = qp->s_len;

	/*
	 * store a copy of the IB header as MIDDLE and LAST packets modify it,
	 * and we'd like to maintain asyncronous send and send_complete
	 * TODO - can be removed by refactoring where upper layer builds header
	 */
	if (!wqe->use_16b)
		memcpy(&wqe_iov->ph, &wqe->s_hdr->ph.ibh, wqe->s_hdrwords << 2);
	else
		memcpy(&wqe_iov->ph, &wqe->s_hdr->opa16b, wqe->s_hdrwords << 2);

	if (use_ofed_dma) {
		/* read current SGE address/length, updates internal state */
		qp_read_sge(qp, &start, &length);
	} else {
		ret = build_iovec_array(ibp, qp, wqe_iov, &num_iovs, &length);
		if (ret)
			goto err;
	}

	if (qp->s_cur_size != length) {
		dev_err(ibp->dev,
			"PT %d: TX error %d of %d bytes unsent, %d iovs\n",
			ibp->port_num, qp->s_cur_size - length,
			qp->s_cur_size, num_iovs);
		ret = -EIO;
		goto err;
	}

	dev_dbg(ibp->dev, "PT %d: cmd %d len %d/%d pmtu %d n_iov %d sent %d\n",
		ibp->port_num, dma_cmd, qp->s_cur_size, wqe->length, wqe->pmtu,
		num_iovs, length);

_tx_cmd:
	spin_lock_irqsave(&ibp->cmdq_tx_lock, flags);
	if (use_ofed_dma) {
		/* send with OFED_DMA */
		ret = hfi_tx_cmd_ofed_dma(&ibp->cmdq_tx, wqe->s_ctx,
					  wqe->s_hdr, (void *)start,
					  length, eth_size,
					  opa_mtu_to_id(wqe->pmtu),
					  (uint64_t)wqe_iov, 0,
					  &ibp->send_eq,
					  ibp->port_num, wqe->sl,
					  ibp->ppd->lid, dma_cmd);
	} else {
		/* send with GENERAL or MGMT DMA */
		ret = hfi_tx_cmd_bypass_dma(&ibp->cmdq_tx, wqe->s_ctx,
					    (uint64_t)wqe_iov->iov, num_iovs,
					    (uint64_t)wqe_iov,
					    PTL_MD_RESERVED_IOV,
					    &ibp->send_eq, HFI_CT_NONE,
					    ibp->port_num, wqe->sl, 0,
					    wqe->use_16b ? HDR_16B : HDR_EXT,
					    dma_cmd);
	}
	spin_unlock_irqrestore(&ibp->cmdq_tx_lock, flags);

	if (ret == -EAGAIN) {
		/* FXRTODO - remove delay via deferred send WQE logic */
		if (ndelays >= OPA_IB_CQ_FULL_RETRIES) {
			dev_warn(ibp->dev,
				 "PT %d: TX CQ still full after %d retries!\n",
				 ibp->port_num, ndelays);
			goto err;
		} else {
			ndelays++;
			mdelay(OPA_IB_CQ_FULL_DELAY_MS);
			goto _tx_cmd;
		}
	} else if (ret < 0) {
		goto err;
	} else if (ndelays) {
		dev_info(ibp->dev, "PT %d: full TX CQ required %d retries\n",
			 ibp->port_num, ndelays);
	}

	return 0;
err:
	kfree(wqe_iov);
	return ret;
}

void *hfi2_rcv_get_ebuf(struct hfi2_ibrcv *rcv, u16 idx, u32 offset)
{
	struct hfi2_ibport *ibp;
	int ret;
	unsigned long flags;

	/* TODO - this needs to be optimized to limit PT_UPDATEs */
	if (idx != rcv->egr_last_idx) {
		ibp = rcv->ibp;
		dev_dbg(ibp->dev, "PT %d: EAGER_HEAD UPDATE %d -> %d\n",
			ibp->port_num, rcv->egr_last_idx, idx);

		/* Tell HW we are finished reading previous eager buffer */
		spin_lock_irqsave(&ibp->cmdq_rx_lock, flags);
		ret = hfi_pt_update_eager(rcv->ctx, &ibp->cmdq_rx, idx);
		spin_unlock_irqrestore(&ibp->cmdq_rx_lock, flags);
		if (ret < 0) {
			/*
			 * only potential error is DROPPED event on EQ0 to
			 * confirm the PT_UPDATE, print error and ignore.
			 */
			dev_err(ibp->dev, "unexpected PT update error %d\n",
				ret);
		}
		rcv->egr_last_idx = idx;
	}

	return rcv->egr_base + (idx * OPA_IB_EAGER_SIZE) + offset;
}

int _hfi2_rcv_wait(struct hfi2_ibrcv *rcv, u64 **rhf_entry)
{
	int rc;

	rc = hfi_eq_wait_irq(rcv->ctx, &rcv->eq, -1,
			     rhf_entry);
	if (rc == -EAGAIN || rc == -ERESTARTSYS) {
		/* timeout or wait interrupted, not abnormal */
		rc = 0;
	} else if (rc == HFI_EQ_DROPPED || rc == HFI_EQ_EMPTY) {
		/* driver bug with EQ sizing or IRQ logic */
		rc = -EIO;
	}
	return rc;
}

void hfi2_rcv_advance(struct hfi2_ibrcv *rcv, u64 *rhf_entry)
{
	struct hfi2_ibport *ibp = rcv->ibp;
	unsigned long flags;

	spin_lock_irqsave(&ibp->cmdq_rx_lock, flags);
	hfi_eq_advance(rcv->ctx, &ibp->cmdq_rx, &rcv->eq, rhf_entry);
	spin_unlock_irqrestore(&ibp->cmdq_rx_lock, flags);
}

static void hfi2_rcv_start(struct hfi2_ibrcv *rcv)
{
	/* RX events can be received after this thread becomes runable */
	if (rcv->task)
		wake_up_process(rcv->task);
}

static void hfi2_rcv_stop(struct hfi2_ibrcv *rcv)
{
	if (rcv->task) {
		/* stop the RX thread, signal it to break out of wait_event */
		send_sig(SIGINT, rcv->task, 1);
		kthread_stop(rcv->task);
		rcv->task = NULL;
	}
}

int hfi2_rcv_init(struct hfi2_ibport *ibp, struct hfi_ctx *ctx,
		  struct hfi2_ibrcv *rcv)
{
	struct task_struct *rcv_task;
	struct hfi_pt_alloc_eager_args pt_alloc;
	struct opa_ev_assign eq_alloc;
	union hfi_rx_cq_command rx_cmd;
	u32 total_eager_size, rhq_count;
	int ret, i, n_slots;

	/*
	 * Receiving on a single context and Event Queue for both ports.
	 * As such, we only perform RX setup for the first port.
	 * TODO - for disjoint_mode, the table is supposed to be partitioned;
	 *  and we can elect to have Event Queue (WFR RHQ) per port
	 */
	if (ibp->port_num != 0)
		return 0;
	rcv->ibp = ibp;
	rcv->ctx = ctx;

	total_eager_size = OPA_IB_EAGER_COUNT * OPA_IB_EAGER_SIZE;
	/*
	 * size the RHQ as one event per 64 B of eager buffer; this prevents
	 * RHQ overflow since payload consumes minimum of 64 B with MAY_ALIGN
	 */
	rhq_count = total_eager_size / 64;

	/* allocate Eager PT and EQ */
	memset(&eq_alloc, 0, sizeof(eq_alloc));
	eq_alloc.ni = HFI_NI_BYPASS;
	eq_alloc.user_data = (unsigned long)ibp;
	eq_alloc.count = rhq_count;
	ret = _hfi_eq_alloc(rcv->ctx, &ibp->cmdq_rx, &ibp->cmdq_rx_lock,
			    &eq_alloc, &rcv->eq);
	if (ret < 0)
		goto kthread_err;

	rcv->egr_last_idx = 0;
	rcv->egr_base = vzalloc(total_eager_size);
	if (!rcv->egr_base)
		goto kthread_err;

	pt_alloc.eq_handle = &rcv->eq;
	pt_alloc.eager_order = OPA_IB_EAGER_COUNT_ORDER;
	ret = hfi_pt_alloc_eager(rcv->ctx, &ibp->cmdq_rx, &pt_alloc);
	if (ret < 0)
		goto kthread_err;

	/* write Eager entries */
	for (i = 0; i < OPA_IB_EAGER_COUNT; i++) {
		u64 *eq_entry = NULL, done;

		n_slots = hfi_format_rx_bypass(rcv->ctx, HFI_NI_BYPASS,
					       rcv->egr_base +
					       (i*OPA_IB_EAGER_SIZE),
					       OPA_IB_EAGER_SIZE,
					       HFI_PT_BYPASS_EAGER,
					       rcv->ctx->ptl_uid,
					       OPA_IB_EAGER_PT_FLAGS |
					       PTL_OP_PUT,
					       HFI_CT_NONE,
						/* minfree is max MTU */
					       HFI_DEFAULT_MAX_MTU,
					       (unsigned long)&done,
					       i, &rx_cmd);
		ret = hfi_rx_command(&ibp->cmdq_rx, (uint64_t *)&rx_cmd,
				     n_slots);
		if (ret < 0)
			goto kthread_err;

		/* Check on EQ 0 NI 0 for a PTL_CMD_COMPLETE event */
		hfi_eq_wait_timed(rcv->ctx, &rcv->ctx->eq_zero[0],
				  HFI_EQ_WAIT_TIMEOUT_MS, &eq_entry);
		if (eq_entry) {
			hfi_eq_advance(rcv->ctx, &ibp->cmdq_rx,
				       &rcv->ctx->eq_zero[0], eq_entry);
		} else {
			ret = -EIO;
			goto kthread_err;
		}
	}

	/* kthread create and wait for packets! */
	rcv_task = kthread_create_on_node(hfi2_rcv_wait, rcv,
					  ibp->ibd->assigned_node_id,
					  "hfi2_ibrcv");
	if (IS_ERR(rcv_task)) {
		ret = PTR_ERR(rcv_task);
		goto kthread_err;
	}
	rcv->task = rcv_task;

	return 0;

kthread_err:
	hfi2_rcv_uninit(rcv);
	return ret;
}

void hfi2_rcv_uninit(struct hfi2_ibrcv *rcv)
{
	struct hfi2_ibport *ibp = rcv->ibp;
	int ret;
	unsigned long flags;

	if (!ibp)
		return;

	/* stop further RX processing */
	hfi2_rcv_stop(rcv);

	/* disable PT so EQ buffer and RX buffers can be released */
	spin_lock_irqsave(&ibp->cmdq_rx_lock, flags);
	ret = hfi_pt_disable(rcv->ctx, &ibp->cmdq_rx, HFI_NI_BYPASS,
			     HFI_PT_BYPASS_EAGER);
	spin_unlock_irqrestore(&ibp->cmdq_rx_lock, flags);
	if (ret < 0)
		dev_err(ibp->dev, "unexpected PT disable error %d\n", ret);

	if (rcv->eq.base)
		_hfi_eq_release(rcv->ctx, &ibp->cmdq_rx, &ibp->cmdq_rx_lock,
				&rcv->eq);
	if (rcv->egr_base) {
		vfree(rcv->egr_base);
		rcv->egr_base = NULL;
	}
	rcv->ctx = NULL;
	rcv->ibp = NULL;
}

void hfi2_ctx_start_port(struct hfi2_ibport *ibp)
{
	hfi2_rcv_start(&ibp->sm_rcv);
	hfi2_rcv_start(&ibp->qp_rcv);
}

int hfi2_ctx_init(struct hfi2_ibdev *ibd, struct opa_core_ops *bus_ops)
{
	int ret;
	struct opa_ctx_assign ctx_assign = {0};
	struct hfi_rsm_rule rule = {0};
	struct hfi_ctx *rsm_ctx[1];

	/* Allocate the management context */
	HFI_CTX_INIT(&ibd->sm_ctx, ibd->dd, bus_ops);
	ibd->sm_ctx.mode |= HFI_CTX_MODE_BYPASS_9B;
	ibd->sm_ctx.qpn_map_idx = 0; /* this context is for QPN 0 */
	ibd->sm_ctx.qpn_map_count = 1;
	ctx_assign.pid = HFI_PID_ANY;
	ctx_assign.le_me_count = OPA_IB_EAGER_COUNT;
	ret = hfi_ctxt_attach(&ibd->sm_ctx, &ctx_assign);
	if (ret < 0)
		goto err;

	/*
	 * Allocate the general QP receive context.
	 * TODO - later we will likely allocate a pool of general contexts
	 * to choose based on QPN or SC.
	 */
	HFI_CTX_INIT(&ibd->qp_ctx, ibd->dd, bus_ops);
	ibd->qp_ctx.mode |= HFI_CTX_MODE_BYPASS_9B | HFI_CTX_MODE_BYPASS_RSM;
	/* TODO - for now map all general QPNs to this receive context */
	ibd->qp_ctx.qpn_map_idx = 1;
	ibd->qp_ctx.qpn_map_count = OPA_QPN_MAP_MAX - 1;
	ctx_assign.pid = HFI_PID_ANY;
	ctx_assign.le_me_count = OPA_IB_EAGER_COUNT;
	ret = hfi_ctxt_attach(&ibd->qp_ctx, &ctx_assign);
	if (ret < 0)
		goto err_qp_ctx;


	/* set RSM rule for 16B Verbs receive using L2 and L4 */
	rule.idx = 0;
	rule.pkt_type = 0x4; /* bypass */
	/* match L2 == 16B (0x2)*/
	rule.match_offset[0] = 61;
	rule.match_mask[0] = OPA_BYPASS_L2_MASK;
	rule.match_value[0] = OPA_BYPASS_HDR_16B;
	/* match IB L4s: 0x8 0x9 0xA */
	rule.match_offset[1] = 64;
	rule.match_mask[1] = 0xFC;
	rule.match_value[1] = 0x8;
	/* disable selection, result is always RSM_MAP index of 0 */
	rule.select_width[0] = 0;
	rule.select_width[1] = 0;
	rsm_ctx[0] = &ibd->qp_ctx;
	ret = hfi_rsm_set_rule(ibd->dd, &rule, rsm_ctx, 1);
	if (ret < 0)
		goto err_rsm;
	ibd->rsm_mask |= (1 << rule.idx);
	return 0;

err_rsm:
	hfi_ctxt_cleanup(&ibd->qp_ctx);
err_qp_ctx:
	hfi_ctxt_cleanup(&ibd->sm_ctx);
err:
	return ret;
}

void hfi2_ctx_uninit(struct hfi2_ibdev *ibd)
{
	int i;

	for (i = 0; i < HFI_NUM_RSM_RULES; i++)
		if (ibd->rsm_mask & (1 << i))
			hfi_rsm_clear_rule(ibd->dd, i);
	ibd->rsm_mask = 0;
	hfi_ctxt_cleanup(&ibd->qp_ctx);
	hfi_ctxt_cleanup(&ibd->sm_ctx);
}

int hfi2_ctx_init_port(struct hfi2_ibport *ibp)
{
	struct hfi2_ibdev *ibd = ibp->ibd;
	struct opa_ev_assign eq_alloc;
	int ret;
	u16 cq_idx;

	spin_lock_init(&ibp->cmdq_tx_lock);
	spin_lock_init(&ibp->cmdq_rx_lock);

	/*
	 * Obtain a pair of command queues for this port.
	 * TODO - later we will likely allocate a pool of general Contexts
	 * and associated CMDQs.
	 * For now, associate this with the management context.
	 */
	ret = hfi_cq_assign(&ibd->sm_ctx, NULL, &cq_idx);
	if (ret)
		goto err;
	ret = hfi_cq_map(&ibd->sm_ctx, cq_idx, &ibp->cmdq_tx, &ibp->cmdq_rx);
	if (ret) {
		hfi_cq_release(&ibd->sm_ctx, cq_idx);
		goto err;
	}
	/*
	 * set Send Context pointer only after CMDQs allocated
	 * Currently we use single CMDQ and send EQ, attached to this context.
	 */
	ibp->ctx = &ibd->sm_ctx;

	/* Each port has send EQ for TX completions */
	memset(&eq_alloc, 0, sizeof(eq_alloc));
	eq_alloc.ni = HFI_NI_BYPASS;
	eq_alloc.user_data = (unsigned long)ibp;
	/* TODO - set EQ size equal to CmdQ slots (DMA commands) for now */
	eq_alloc.count = HFI_CQ_TX_ENTRIES;
	eq_alloc.isr_cb = hfi2_send_event;
	eq_alloc.cookie = (void *)ibp;
	ret = _hfi_eq_alloc(ibp->ctx, &ibp->cmdq_rx, &ibp->cmdq_rx_lock,
			    &eq_alloc, &ibp->send_eq);
	if (ret < 0)
		goto ctx_init_err;

	/* assign device specific RX resources */
	ret = hfi2_rcv_init(ibp, &ibd->sm_ctx, &ibp->sm_rcv);
	if (ret)
		goto ctx_init_err;
	ret = hfi2_rcv_init(ibp, &ibd->qp_ctx, &ibp->qp_rcv);
	if (ret)
		goto ctx_init_err;

	return 0;

ctx_init_err:
	hfi2_ctx_uninit_port(ibp);
err:
	return ret;
}

void hfi2_ctx_uninit_port(struct hfi2_ibport *ibp)
{
	u16 cq_idx = ibp->cmdq_tx.cq_idx;

	if (!ibp->ctx)
		return;

	if (ibp->send_eq.base)
		_hfi_eq_release(ibp->ctx, &ibp->cmdq_rx, &ibp->cmdq_rx_lock,
				&ibp->send_eq);

	hfi2_rcv_uninit(&ibp->qp_rcv);
	hfi2_rcv_uninit(&ibp->sm_rcv);
	hfi_cq_unmap(&ibp->cmdq_tx, &ibp->cmdq_rx);
	hfi_cq_release(ibp->ctx, cq_idx);
	ibp->ctx = NULL;
}

int hfi2_ctx_assign_qp(struct hfi2_ibport *ibp, struct hfi2_qp *qp,
			 bool is_user)
{
	/* FXRTODO: fix me */
#if 0
	int ret;

	/* hold the device so it cannot be removed while QP is active */
	ret = opa_core_device_get(ibd->odev);
	if (ret)
		return ret;
#endif
	qp->s_ctx = ibp->ctx;
	return 0;
}

void hfi2_ctx_release_qp(struct hfi2_ibport *ibp, struct hfi2_qp *qp)
{
	qp->s_ctx = NULL;
	/* FXRTODO: fix me */
#if 0
	opa_core_device_put(ibd->odev);
#endif
}
