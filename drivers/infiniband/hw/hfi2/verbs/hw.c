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
#include <linux/sched/signal.h>
#include "verbs.h"
#include "packet.h"
#include "trace.h"
#include "hfi_kclient.h"
#include "hfi_tx_bypass.h"

static bool disable_ib_dma;
static bool disable_pio = true;

/* use HFI_IB prefix to note these are private defines for Verbs receive */
#define HFI_IB_CMDQ_FULL_RETRIES	10
#define HFI_IB_CMDQ_FULL_DELAY_MS	1
/* minimum Eager entries */
#define HFI_IB_EAGER_COUNT		2048
/* log2 of EAGER_COUNT - 2 */
#define HFI_IB_EAGER_COUNT_ORDER	9
#define HFI_IB_EAGER_SIZE		(PAGE_SIZE * 8)
#define HFI_IB_EAGER_PT_FLAGS		(PTL_MAY_ALIGN | PTL_MANAGE_LOCAL)
#define HFI_IB_EAGER_BUFSIZE		(HFI_IB_EAGER_COUNT * HFI_IB_EAGER_SIZE)

/*
 * PIO threshold in bytes - will use PIO if payload is less than this.
 * Use caution in sizing as we potentially have many QPs sharing few
 * command queues.  The hardware maximum is 4096.
 */
#define HFI_IB_PIO_THRESHOLD	1024

/*
 * PT entry update rate. RHQ is power of 2 value.
 * this macro defines the times to update PT for a
 * full RHQ processing, must be power of 2 value.
 */
#define HFI_IB_PT_UPDATE_RATE		4
#define HFI_IB_EAGER_UPDATE_COUNT	(HFI_IB_EAGER_COUNT / HFI_IB_PT_UPDATE_RATE)

/*
 * The header argument must point to memory that is padded to a full
 * cacheline.  In addition, the first 8-bytes is reserved for this function
 * if using 9B header, which begins at (hdr + 8).
 */
static void hfi_format_nonptl_flit0(struct hfi_ctx *ctx, void *hdr,
				    u8 port, u8 sl, u8 slid_low,
				    bool becn, u8 ack_rate,
				    u8 nptl_cmd, u16 cmd_length)
{
	struct tx_cq_ib_dma_flit0 *flit0; /* common for non-portals */
	u8 l2;

	flit0 = hdr;
	if (nptl_cmd < OFED_16B_DMA) {
		l2 = HDR_EXT;
	} else {
		struct hfi2_16b_header *opa16b = hdr;
		u32 dlid = hfi2_16B_get_dlid(opa16b);
		u16 len = hfi2_16B_get_pkt_len(opa16b);
		u8 l4 = hfi2_16B_get_l4(opa16b);

		/* Write A4 information, overwrites part of 16B header */
		_hfi_format_nonptl_flit0_a4(ctx, &flit0->a4,
					    dlid, len, l4, 0);
		l2 = HDR_16B;
	}

	/*
	 * Current expectation is that caller already filled in all
	 * IB header fields and passes in header pointer.
	 * Update just A3 information, which is before IB fields.
	 * For 16B, the A3 and A4 information replaces 16B header.
	 */
	_hfi_format_nonptl_flit0_a3(ctx, &flit0->a3, l2,
				    nptl_cmd, cmd_length,
				    port, sl, slid_low, 0, !!becn,
				    ack_rate);
}

static int hfi_format_ib_dma_cmd(struct hfi_ctx *ctx,
				 void *hdr, void *start, u32 len,
				 u8 eth_size, u8 mtu_id,
				 u64 user_ptr, u32 md_opts,
				 struct hfi_eq *eq_handle,
				 u8 port, u8 sl, u8 slid_low,
				 bool becn, u8 ack_rate,
				 u8 dma_cmd)
{
	union hfi_tx_ib_dma *cmd; /* all IB DMA commands use this struct */

	hfi_format_nonptl_flit0(ctx, hdr, port, sl, slid_low, becn, ack_rate,
				dma_cmd, 16 /* num 8B flits in 2 slots */);

	/* Here we update just P..E4 flit, which is after IB fields */
	cmd = hdr;
	_hfi_format_ib_dma_flit3(ctx, &cmd->flit3,
				 eth_size, mtu_id,
				 (u64)start, len,
				 md_opts, eq_handle, user_ptr);

	/* Two slot command */
	return 2;
}

static void hfi_tx_nonptl_pio(struct hfi_cmdq *cq, void *hdr, size_t hdr_len,
			      void *start, size_t length, u16 cmd_slots)
{
	size_t offset, remaining_head_slot_bytes;

	remaining_head_slot_bytes = 64 - hdr_len;
	if (remaining_head_slot_bytes && start) {
		/*
		 * Copy portion of payload into buffer for first CMDQ slot.
		 * The provided header memory is required to be padded to
		 * allow for this.
		 */
		offset = min_t(size_t, length, remaining_head_slot_bytes);
		_hfi_memcpy(hdr + hdr_len, start, offset);
		start += offset;
		length -= offset;
	}

	/* Issue the single or multi-slot PIO */
	if (cmd_slots == 1)
		_hfi_command(cq, (u64 *)hdr, HFI_CMDQ_TX_ENTRIES);
	else
		_hfi_command_pio(cq, (u64 *)hdr, (const void *)start,
				 length, cmd_slots, HFI_CMDQ_TX_ENTRIES);
}

/*
 * compute how many IOVECs we need at most for sending
 *
 * TODO may need to determine packet boundaries depending on how auto-header
 * optimization is implemented in upper layers (need add'l header IOVECs)
 */
static int qp_max_iovs(struct hfi2_ibport *ibp, struct rvt_qp *qp,
		       int *out_niovs)
{
	int niovs, i;
	size_t off, segsz, payload_bytes, length;
	struct rvt_sge_state *ss = qp->s_cur_sge;
	struct rvt_sge *sge = &ss->sge;

	niovs = 1; /* one IOV for header */

	/* step through SGEs to send in this packet */
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
			/*
			 * compute max iovs needed if sending all bytes in SGE,
			 * except limited to remaining payload size.
			 */
			length = min_t(size_t, qp->s_cur_size - payload_bytes,
				       sge->sge_length);
			niovs += ((off + length + segsz - 1) / segsz);
		} else {
			niovs++;
		}
	}

	*out_niovs = niovs;
	return 0;
}

/*
 * read QP's SG list state for next address and length to send
 */
static void qp_read_sge(struct rvt_qp *qp, u32 sent,
			u64 *start, u32 *length)
{
	u32 iov_length;
	struct rvt_sge_state *ss = qp->s_cur_sge;
	struct rvt_sge *sge = &ss->sge;

	/* get smaller of remaining bytes or SGE length */
	iov_length = rvt_get_sge_length(sge, qp->s_cur_size - sent);
	*length = iov_length;
	*start = (u64)sge->vaddr;

	/* update QP internal state */
	rvt_update_sge(qp->s_cur_sge, iov_length, false);
}

static int
send_wqe_pio(struct hfi2_ibport *ibp, struct hfi2_qp_priv *qp_priv,
	     u8 offset, u8 pio_cmd)
{
	struct hfi2_ibtx *ibtx = qp_priv->s_ctx;
	struct rvt_qp *qp = qp_priv->owner;
	struct hfi_pend_queue *pq = &ibtx->pend_cmdq;
	unsigned long flags;
	u32 length = 0;
	u64 start = 0;
	u16 cmd_length, cmd_slots;

	if (qp->ibqp.qp_type == IB_QPT_SMI)
		return -EINVAL;

	cmd_length = _HFI_CMD_LENGTH(qp->s_cur_size, offset);
	cmd_slots  = _HFI_CMD_SLOTS(cmd_length);

	spin_lock_irqsave(&pq->lock, flags);
	/* only issue PIO if the pend_cmdq is empty and has required slots */
	if (!(list_empty(&pq->pending) &&
	      hfi_queue_ready(&ibtx->cmdq.tx, cmd_slots, NULL))) {
		spin_unlock_irqrestore(&pq->lock, flags);
		ibp->stats->sps_nopiobufs++;
		return -EAGAIN;
	}

	/* read the payload address if sending payload */
	if (qp->s_cur_size) {
		qp_read_sge(qp, 0, &start, &length);
		/* verify caller only asked us to PIO single segment */
		if (qp->s_cur_size != length) {
			spin_unlock_irqrestore(&pq->lock, flags);
			return -EIO;
		}
	}

	/* send the WQE via PIO path */
	hfi_format_nonptl_flit0(ibtx->ctx,
				qp_priv->s_hdr, ibp->port_num,
				qp_priv->s_sl, 0,
				qp_priv->send_becn, 0,
				pio_cmd, cmd_length);
	hfi_tx_nonptl_pio(&ibtx->cmdq.tx, qp_priv->s_hdr, offset,
			  (void *)start, length, cmd_slots);
	spin_unlock_irqrestore(&pq->lock, flags);
	ibp->stats->n_send_pio++;

	/* if RC or last UD/UC packet, send complete */
	if (qp->ibqp.qp_type == IB_QPT_RC) {
		spin_lock_irqsave(&qp->s_lock, flags);
		hfi2_rc_send_complete(qp, qp_priv->opcode,
				      qp_priv->bth2);
		spin_unlock_irqrestore(&qp->s_lock, flags);
	} else if (!qp->s_len) {
		spin_lock_irqsave(&qp->s_lock, flags);
		hfi2_send_complete(qp, qp->s_wqe, IB_WC_SUCCESS);
		spin_unlock_irqrestore(&qp->s_lock, flags);
	}

	return 0;
}

/*
 * The first argument is pointer to a driver clone of the EQ descriptor.
 * It cannot be used for tracking pending events, so we use ibtx->send_eq
 * instead for correct decrement of the pending event count.
 */
static void hfi2_send_event(struct hfi_eq *eq_tx, void *data)
{
	struct hfi2_ibtx *ibtx = data;
	struct hfi2_ibdev *ibd = ibtx->ibp->ibd;
	struct rvt_qp *qp;
	struct rvt_swqe *wqe;
	struct hfi2_wqe_dma *wqe_dma;
	u32 pkt_errors = 0;
	union initiator_EQEntry *eq_entry;
	bool dropped = false;
	int ret;

	/* driver bug if these don't match */
	if (WARN_ON_ONCE(eq_tx->idx != ibtx->send_eq.idx))
		return;

next_event:
	ret = hfi_eq_peek(&ibtx->send_eq, (u64 **)&eq_entry, &dropped);
	if (unlikely(dropped))
		dd_dev_warn_ratelimited(ibd->dd,
					"EQ %d send_event dropped!\n",
					ibtx->send_eq.idx);
	if (ret <= 0)
		return;

	if (eq_entry->event_kind != NON_PTL_EVENT_TX_COMPLETE) {
		dd_dev_warn_ratelimited(ibd->dd,
					"EQ %d unexpected EQ kind %x\n",
					ibtx->send_eq.idx,
					eq_entry->event_kind);
		goto eq_advance;
	}

	wqe_dma = (struct hfi2_wqe_dma *)eq_entry->user_ptr;
	/* if RC ACKs sent via IB_DMA optimization, can short circuit */
	if (!wqe_dma)
		goto eq_advance;

	qp = wqe_dma->qp;
	if (!qp) {
		/*
		 * RC ACKs sent via send_ack() don't set associated QP as
		 * don't require calling rc_send_complete.
		 * We must still free the structure used for General DMA.
		 */
		goto dma_free;
	} else if (qp->ibqp.qp_type == IB_QPT_RC) {
		wqe = NULL;
		/* for RC READ responses, release the MR */
		if (wqe_dma->mr)
			rvt_put_mr(wqe_dma->mr);
	} else {
		wqe = wqe_dma->wqe;
	}

	dd_dev_dbg(ibd->dd,
		   "TX event %p, fail %d, for QPN %d:0x%x, remain %d\n",
		   eq_entry, eq_entry->fail_type,
		   qp->ibqp.qp_num, wqe_dma->opcode, wqe_dma->remaining_bytes);

	spin_lock(&qp->s_lock);
	if (!wqe) {
		hfi2_rc_send_complete(qp, wqe_dma->opcode, wqe_dma->bth2);
	} else {
#ifdef HFI2_WQE_PKT_ERRORS
		/*
		 * FXRTODO - disabling this for now.
		 * Need to revisit if this is actually helpful to return
		 * completion error to user.
		 * Enabling this again would mean pushing changes upstream to
		 * modify struct rvt_swqe.
		 */
		if (eq_entry->fail_type) {
			wqe->pkt_errors++;
			ibp->stats->sps_txerrs++;
		}
		pkt_errors = wqe->pkt_errors;
#endif
		/* if final UD/UC packet, call send_complete */
		if (!wqe_dma->remaining_bytes)
			hfi2_send_complete(qp, wqe, pkt_errors ?
					   IB_WC_FATAL_ERR : IB_WC_SUCCESS);
	}
	spin_unlock(&qp->s_lock);
	/*
	 * Free the structure used for IB_DMA or General DMA; note this was
	 * allocated as either struct hfi2_wqe_dma or struct hfi2_wqe_iov.
	 */
dma_free:
	wqe_iov_dma_cache_put(ibd, wqe_dma);

eq_advance:
	hfi_eq_advance(&ibtx->send_eq, (u64 *)eq_entry);

	goto next_event;
}

/* constructor that is called to initialze the cache object*/
static void wqe_dma_cache_ctor(void *obj)
{
	struct hfi2_wqe_dma *wqe_dma = (struct hfi2_wqe_dma *)obj;

	memset(wqe_dma, 0, sizeof(*wqe_dma));
}

/* constructor that is called to initialze the cache object*/
static void wqe_iov_cache_ctor(void *obj)
{
	struct hfi2_wqe_iov *wqe_iov = (struct hfi2_wqe_iov *)obj;

	memset(wqe_iov, 0, sizeof(*wqe_iov));
}

/* Initialize wqe_dma and wqe_iov caches */
int wqe_cache_create(struct hfi2_ibdev *ibd)
{
	char iov_buf[32];
	char dma_buf[32];

	snprintf(iov_buf, sizeof(iov_buf), "hfi2_%u_wqe_iov_cache",
		 ibd->assigned_node_id);

	snprintf(dma_buf, sizeof(dma_buf), "hfi2_%u_wqe_dma_cache",
		 ibd->assigned_node_id);

	ibd->wqe_iov_cache = kmem_cache_create(iov_buf,
					      sizeof(struct hfi2_wqe_iov),
					      0,
					      SLAB_HWCACHE_ALIGN,
					      wqe_iov_cache_ctor);
	if (!ibd->wqe_iov_cache)
		return -ENOMEM;
	ibd->wqe_dma_cache = kmem_cache_create(dma_buf,
					      sizeof(struct hfi2_wqe_dma),
					      0,
					      SLAB_HWCACHE_ALIGN,
					      wqe_dma_cache_ctor);

	if (!ibd->wqe_dma_cache) {
		kmem_cache_destroy(ibd->wqe_iov_cache);
		return -ENOMEM;
	}
	return 0;
}

/* Destroy caches */
void wqe_cache_destroy(struct hfi2_ibdev *ibd)
{
	kmem_cache_destroy(ibd->wqe_iov_cache);
	ibd->wqe_iov_cache = NULL;

	kmem_cache_destroy(ibd->wqe_dma_cache);
	ibd->wqe_dma_cache = NULL;
}

/* Allocate wqe_dma from cache, init iov pointer */
static struct hfi2_wqe_iov *wqe_iov_cache_get(struct hfi2_ibdev *ibd,
					      int num_iov, gfp_t gfp)
{
	struct hfi2_wqe_iov *wqe_iov;

	wqe_iov = kmem_cache_alloc_node(ibd->wqe_iov_cache, gfp, ibd->dd->node);

	if (!wqe_iov)
		return NULL;
	wqe_iov->dma.is_ib_dma = false;
	if (num_iov <= WQE_GDMA_IOV_CACHE_SIZE) {
		wqe_iov->iov = wqe_iov->iov_cache;
	} else {
		/* FXRTODO: replace kcalloc with kcalloc_node */
		/* More IOVs needed that we have in cache, alloc dynamically */
		wqe_iov->iov = kcalloc(num_iov, sizeof(union base_iovec),
				       gfp);
		if (!wqe_iov->iov) {
			kmem_cache_free(ibd->wqe_iov_cache, wqe_iov);
			return NULL;
		}
	}
	return wqe_iov;
}

/* Release wqe_dma or wqe_iov to cache */
void wqe_iov_dma_cache_put(struct hfi2_ibdev *ibd,
			   struct hfi2_wqe_dma *wqe_dma)
{
	struct hfi2_wqe_iov *wqe_iov;

	if (wqe_dma->is_ib_dma) {
		kmem_cache_free(ibd->wqe_dma_cache, wqe_dma);
	} else {
		wqe_iov = (struct hfi2_wqe_iov *)wqe_dma;
		if (wqe_iov->iov != wqe_iov->iov_cache)
			kfree(wqe_iov->iov);
		kmem_cache_free(ibd->wqe_iov_cache, wqe_iov);
	}
}

static int build_iovec_array(struct hfi2_ibport *ibp, struct rvt_qp *qp,
			     bool use_16b, struct hfi2_wqe_iov *wqe_iov,
			     int *out_niovs, u32 *iov_bytes)
{
	int i, niovs = *out_niovs;
	u32 payload_bytes;
	union base_iovec *iov;

	iov = wqe_iov->iov;
	/* first entry is IB header */
	iov[0].length = (qp->s_hdrwords << 2);
	iov[0].use_9b = !use_16b;
	iov[0].sp = 1;
	iov[0].ep = 0;
	iov[0].v = 1;
	iov[0].start = (uint64_t)(&wqe_iov->ph);

	/* write IOVEC entries */
	payload_bytes = 0;
	for (i = 1; payload_bytes < qp->s_cur_size && i < niovs; i++) {
		u64 iov_start;
		u32 iov_length;

		/* read current SGE address/length, updates internal state */
		qp_read_sge(qp, payload_bytes, &iov_start, &iov_length);

		/*
		 * If payload is now larger than MTU, start new packet;
		 * caller doesn't do this currently, but test for it
		 * TODO - may need if auto-header optimization
		 */
		if (payload_bytes + iov_length > qp->pmtu && i > 1) {
			dev_err(ibp->dev,
				"PT %d: large DMA not implemented, %d > %d MTU!\n",
				ibp->port_num,
				payload_bytes + iov_length, qp->pmtu);
			return -EIO;
		}

		iov[i].length = iov_length;
		iov[i].use_9b = !use_16b;
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

/*
 * Checking if the verbs user is using resp SL
 * This function requires the sl passed is valid SL
 * Returns true if the SL is resp SL  of the req/resp SL pairs.
 */

bool hfi2_is_verbs_resp_sl(struct hfi_pportdata *ppd, u8 sl)
{
	int i;
	int num_sls = ARRAY_SIZE(ppd->sl_pairs);

	for (i = 0; i < num_sls; i++)
		if (ppd->sl_pairs[i] == sl)
			return true;
	return false;
}

int hfi2_send_ack(struct hfi2_ibport *ibp, struct hfi2_qp_priv *qp_priv,
		  union hfi2_packet_header *ph, size_t hwords)
{
	struct hfi2_ibtx *ibtx = qp_priv->s_ctx;
	struct rvt_qp *qp = qp_priv->owner;
	int ret, slots;
	u8 dma_cmd = GENERAL_DMA;
	struct hfi2_wqe_iov *wqe_iov = NULL;
	u32 num_iovs = 1, hlen = (hwords << 2);
	union base_iovec *iov;
	union hfi_tx_general_dma cmd;
	u8 md_opts, sl = rdma_ah_get_sl(&qp->remote_ah_attr);
	bool use_16b = qp_priv->hdr_type == HFI2_PKT_TYPE_16B;

	md_opts = (sl & 0x10) ? NONPTL_SC4 : 0;

#ifdef HFI_VERBS_TEST
	if (hfi2_drop_packet(qp) && qp->s_retry_cnt) {
		dev_dbg(ibp->dev, "Dropping %s with PSN = %d\n",
			(qp->r_nak_state) ? "NAK" : "ACK",
			mask_psn(qp->r_ack_psn));
		return -1;
	}
#endif
	wqe_iov = wqe_iov_cache_get(ibp->ibd, num_iovs, GFP_ATOMIC);
	if (!wqe_iov)
		return -ENOMEM;

	replace_sc(ph, sl, use_16b);

	/* General DMA is asynchronous. Don't use stack memory */
	memcpy(&wqe_iov->ph, ph, hlen);

	iov = wqe_iov->iov;
	/*
	 * ACKs sent via this routine don't require calling rc_send_complete(),
	 * so we don't need QP or BTH fields set
	 */
	wqe_iov->dma.qp = NULL;
	wqe_iov->dma.remaining_bytes = 0;

	/* first entry is IB header */
	iov[0].length = hlen;
	iov[0].use_9b = !use_16b;
	iov[0].sp = 1;
	iov[0].ep = 1;
	iov[0].v = 1;
	iov[0].start = (u64)&wqe_iov->ph;

	dev_dbg(ibp->dev, "PT %d: cmd %d len 0 n_iov 1 sent (iov[0]) = %u\n",
		ibp->port_num, dma_cmd, iov[0].length);

	trace_hfi2_hdr_send_ack(ibp->ibd->dd, &wqe_iov->ph, use_16b);

	/* send with GENERAL or MGMT DMA */
	slots = hfi_format_bypass_dma_cmd(ibtx->ctx,
					  (u64)wqe_iov->iov, num_iovs,
					  (u64)wqe_iov,
					  md_opts | PTL_MD_RESERVED_IOV,
					  &ibtx->send_eq, HFI_CT_NONE,
					  ibp->port_num, sl, 0,
					  use_16b ? HDR_16B : HDR_EXT,
					  dma_cmd, &cmd);
	/* Queue write, don't wait. */
	ret = hfi_pend_cmd_queue(&ibtx->pend_cmdq, &ibtx->cmdq.tx,
				 &ibtx->send_eq, &cmd, slots, GFP_KERNEL);
	if (ret) {
		dd_dev_err(ibp->ibd->dd,
			   "%s: hfi_pend_cmd_queue failed %d\n",
			   __func__, ret);
		goto err;
	}

	return 0;
err:
	wqe_iov_dma_cache_put(ibp->ibd, &wqe_iov->dma);
	return ret;
}

int hfi2_send_wqe(struct hfi2_ibport *ibp, struct hfi2_qp_priv *qp_priv,
		  bool allow_sleep)
{
	struct hfi2_ibtx *ibtx = qp_priv->s_ctx;
	struct rvt_qp *qp = qp_priv->owner;
	union hfi_tx_general_dma general_cmd;
	void *cmd_ptr;
	enum ib_qp_type qp_type = qp->ibqp.qp_type;
	struct hfi2_wqe_dma *wqe_dma;
	struct hfi2_wqe_iov *wqe_iov = NULL;
	void *phdr;
	int ret, slots;
	u8 md_opts, sl, dma_cmd, ib_dma_cmd, pio_cmd;
	u8 base_hdrwords, pio_hlen, eth_size = 0, ack_rate = 0;
	u32 num_iovs, length = 0;
	u64 start = 0;
	bool use_16b, use_ib_dma, try_pio, have_grh;
	gfp_t gfp = allow_sleep ? GFP_KERNEL : GFP_ATOMIC;

	dma_cmd = (qp_type == IB_QPT_SMI) ? MGMT_DMA : GENERAL_DMA;
	/*
	 * TODO - below QP fields cannot be used when we implement sending
	 * WQE from pending list; suboptimal to call hfi2_use_16b() again for
	 * UD queue pair, should make_req() stash the pkt_type in QP?
	 */
	sl = qp_priv->s_sl;
	md_opts = (sl & 0x10) ? NONPTL_SC4 : 0;
	use_16b = qp_priv->hdr_type;
	pio_hlen = (qp->s_hdrwords << 2);

	/* Get packet header pointer, used for memcpy and trace */
	if (!use_16b) {
		phdr = &qp_priv->s_hdr->ph.ibh;
		base_hdrwords = 5; /* LRH + BTH */
		ib_dma_cmd = OFED_9B_DMA;
		pio_cmd = OFED_9B_PIO;
		pio_hlen += 8;  /* 9B is offset by 8 bytes */
	} else {
		phdr = &qp_priv->s_hdr->opah;
		base_hdrwords = 7; /* L2 + BTH */
		ib_dma_cmd = OFED_16B_DMA;
		pio_cmd = OFED_16B_PIO;
	}
	eth_size = (qp->s_hdrwords - base_hdrwords) << 2;
	/* Largest ETH is 36 bytes, larger means we need to account for GRH */
	have_grh = (eth_size >= 40);

	/*
	 * use IB_DMA command if possible (header + single IOVEC payload)
	 * TODO - PIO might work for 9B + GRH (possibly not 16B + GRH)
	 */
	use_ib_dma = (dma_cmd != MGMT_DMA && !disable_ib_dma);
	try_pio = (dma_cmd != MGMT_DMA && !have_grh && !disable_pio);

	if (use_ib_dma && qp_priv->middle_len) {
		qp->s_len -= (qp_priv->middle_len - qp->s_cur_size);
		qp->s_cur_size = qp_priv->middle_len;
		qp->s_psn = qp_priv->lpsn;
		/* replace PSN with the last PSN of Middles */
		qp_priv->bth2 = (qp_priv->bth2 & IB_BTH_REQ_ACK) |
				mask_psn(qp_priv->lpsn - 1);
		/* ack_rate controls when BTH.A bit is set in packet */
		ack_rate = HFI2_PSN_CREDIT;
	} else {
		/* Test if we require IOVEC array */
		ret = qp_max_iovs(ibp, qp, &num_iovs);
		if (ret)
			return ret;

		/*
		 * Adjust use_ib_dma depending on number of payload segments
		 * required. (num_iovs > 2) means we have multiple payload
		 * memory segments.  Multiple segments uses General DMA, else
		 * we test if PIO can be used.  We can later optimize to also
		 * consider PIO for multiple payload segments.
		 */
		if (num_iovs > 2) {
			use_ib_dma = false;
		} else if (try_pio && qp->s_cur_size <= HFI_IB_PIO_THRESHOLD) {
			ret = send_wqe_pio(ibp, qp_priv, pio_hlen, pio_cmd);
			if (!ret)
				/* WQE sent using PIO, we are done */
				return 0;
			else if (ret != -EAGAIN)
				return ret;
			/* no slots for PIO, continue using DMA */
		}
	}

	if (use_ib_dma) {
		dma_cmd = ib_dma_cmd;
		/*
		 * switch IB_DMA command if using GRH and correct ETH size
		 * to remove the 40 bytes of GRH.
		 */
		if (have_grh) {
			dma_cmd++;
			eth_size -= 40;
		}
		num_iovs = 0;

		/* Allocate smaller structure which omits header and IOVEC */
		wqe_dma = kmem_cache_alloc(ibp->ibd->wqe_dma_cache,
					   allow_sleep ?
					   GFP_KERNEL : GFP_ATOMIC);

		if (!wqe_dma)
			return -ENOMEM;
		wqe_dma->is_ib_dma = true;
		/*
		 * read current SGE address/length, updates internal state;
		 * if no payload (cur_size), start and length are zero
		 */
		if (qp->s_cur_size)
			qp_read_sge(qp, 0, &start, &length);
	} else {
		/*
		 * Both IB_DMA and GENERAL_DMA use a SL rather than a SC.
		 * However, in the case of a IB_DMA the SL is included in
		 * the command rather than in fully formed packet in memory.
		 * So replace_sc only has to done for GENERAL_DMA. There is
		 * no need to replace the SC for MGMT_DMA.
		 */
		if (dma_cmd == GENERAL_DMA)
			replace_sc_dma(qp_priv->s_hdr, sl, use_16b);
		wqe_iov = wqe_iov_cache_get(ibp->ibd, num_iovs, gfp);

		if (!wqe_iov)
			return -ENOMEM;
		wqe_dma = &wqe_iov->dma;
		/*
		 * Store a copy of the IB header as MIDDLE and LAST packets
		 * modify it, and we'd like to maintain asyncronous send
		 * and send_complete. (Needed for General DMA only.)
		 * TODO - can be removed by refactoring where upper layer
		 * builds header.
		 */
		memcpy(&wqe_iov->ph, phdr, qp->s_hdrwords << 2);

		ret = build_iovec_array(ibp, qp, use_16b, wqe_iov,
					&num_iovs, &length);
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

	/* fill in rest of wqe_dma needed for send completion event */
	wqe_dma->qp = qp;
	if (qp_type == IB_QPT_RC) {
		/* take over release of MR for RC responses */
		wqe_dma->mr = qp->s_rdma_mr;
		qp->s_rdma_mr = NULL;
		wqe_dma->opcode = qp_priv->opcode;
		wqe_dma->bth2 = qp_priv->bth2;
		/* ack_rate set above if using Middle optimization */
	} else {
		ack_rate = 0; /* make sure BTH.A unset for UD/UC */
		wqe_dma->wqe = qp->s_wqe;
	}
	wqe_dma->remaining_bytes = qp->s_len;

	dev_dbg(ibp->dev, "PT %d: cmd %d len %d pmtu %d n_iov %d sent %d\n",
		ibp->port_num, dma_cmd, qp->s_cur_size, qp->pmtu,
		num_iovs, length);

	trace_hfi2_hdr_send_wqe(ibp->ibd->dd, phdr, use_16b);

	if (use_ib_dma) {
		cmd_ptr = qp_priv->s_hdr;
		/*
		 * send with IB DMA command
		 * Note, the header in QP (qp_priv->s_hdr) will be modified
		 * if this is a 16B packet. 16B header is replaced with 16 bytes
		 * of data for the TX command.  This is not an issue as the
		 * caller does not use qp_priv->s_hdr further for this packet.
		 */
		slots = hfi_format_ib_dma_cmd(ibtx->ctx,
					      cmd_ptr, (void *)start,
					      length, eth_size,
					      opa_mtu_to_enum(qp->pmtu),
					      (u64)wqe_dma, md_opts,
					      &ibtx->send_eq,
					      ibp->port_num, sl,
					      ibp->ppd->lid,
					      qp_priv->send_becn,
					      ack_rate, dma_cmd);
		ibp->stats->n_send_ib_dma++;
	} else {
		cmd_ptr = &general_cmd;

		/* send with GENERAL or MGMT DMA */
		slots = hfi_format_bypass_dma_cmd(ibtx->ctx,
						  (u64)wqe_iov->iov, num_iovs,
						  (u64)wqe_iov,
						  md_opts | PTL_MD_RESERVED_IOV,
						  &ibtx->send_eq, HFI_CT_NONE,
						  ibp->port_num, sl, 0,
						  use_16b ? HDR_16B : HDR_EXT,
						  dma_cmd, cmd_ptr);
		ibp->stats->n_send_dma++;
	}

	/* Write command, don't wait. */
	ret = hfi_pend_cmd_queue(&ibtx->pend_cmdq, &ibtx->cmdq.tx,
				 &ibtx->send_eq, cmd_ptr, slots, gfp);
	if (ret) {
		dd_dev_err(ibp->ibd->dd,
			   "%s: hfi_pend_cmd_queue failed %d\n",
			   __func__, ret);
		goto err;
	}

	return 0;
err:
	wqe_iov_dma_cache_put(ibp->ibd, wqe_dma);
	return ret;
}

void *hfi2_rcv_get_ebuf_ptr(struct hfi2_ibrcv *rcv, u16 idx, u32 offset)
{
	return rcv->egr_base + (idx * HFI_IB_EAGER_SIZE) + offset;
}

void *hfi2_rcv_get_ebuf(struct hfi2_ibrcv *rcv, u16 idx, u32 offset)
{
	/*
	 * Increment running counter of eager segments that have
	 * been processed, but we haven't yet informed hardware.
	 * Packet size is less than eager segment, so we can only
	 * advance by one; thus we simply test if index has changed.
	 */
	rcv->egr_pending_update += !!(rcv->egr_last_idx != idx);

	rcv->egr_last_idx = idx;
	return hfi2_rcv_get_ebuf_ptr(rcv, idx, offset);
}

/*
 * Block for an EQ event interrupt, return value:
 * <0: on error, including dropped,
 *  0: no new event,
 *  1. get new event,
 */
int _hfi2_rcv_wait(struct hfi2_ibrcv *rcv, u64 **rhf_entry)
{
	int rc;
	bool dropped = false;

	rc = hfi_eq_wait_irq(&rcv->eq, rcv->eq.ctx->devdata->emulation ? 1 : -1,
			     rhf_entry, &dropped);
	if (rc == -ETIME || rc == -ERESTARTSYS) {
		/* timeout or wait interrupted, not abnormal */
		rc = 0;
	} else if (rc > 0 && dropped) {
		/* driver bug with EQ sizing */
		rc = -EIO;
	}
	return rc;
}

void hfi2_rcv_advance(struct hfi2_ibrcv *rcv, u64 *rhf_entry)
{
	hfi_eq_advance(&rcv->eq, rhf_entry);

	/*
	 * if either header queue or eager buffer has passed update threshold
	 * then send PT_UPDATE
	 */
	if (!((*rcv->eq.head_addr) & rcv->rhq_update_mask) ||
	    (rcv->egr_pending_update >= HFI_IB_EAGER_UPDATE_COUNT)) {
		int ret;

		/* Tell HW re-enable PT and update eager buffer */
		ret = hfi_pt_update_pending(rcv->pend_cmdq, rcv->ctx,
					    rcv->cmdq_rx,
					    rcv->egr_last_idx);
		if (ret < 0) {
			/*
			 * only potential error is DROPPED event on EQ0 to
			 * confirm the PT_UPDATE, print error and ignore.
			 */
			dev_err(rcv->ibp->dev,
				"unexpected PT update error %d\n", ret);
		}
		rcv->egr_pending_update = 0;
	}
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
		  struct hfi2_ibrcv *rcv, int cmdq_idx)
{
	struct task_struct *rcv_task;
	struct hfi_pt_alloc_eager_args pt_alloc = {0};
	struct opa_ev_assign eq_alloc = {0};
	union hfi_rx_cq_command rx_cmd;
	u32 rhq_count;
	int ret, i, n_slots;

	/* Revisit if we ever support multi-port device */
	if (WARN_ON(ibp->port_num != 0))
		return 0;
	rcv->ibp = ibp;
	rcv->ctx = ctx;
	/* Receive context needs RX CMDQ for managing RX resources */
	rcv->pend_cmdq = &ibp->port_tx[cmdq_idx].pend_cmdq;
	rcv->cmdq_rx = &ibp->port_tx[cmdq_idx].cmdq.rx;

	/*
	 * size the RHQ as one event per 64 B of eager buffer; this prevents
	 * RHQ overflow since payload consumes minimum of 64 B with MAY_ALIGN
	 */
	BUILD_BUG_ON(!is_power_of_2(HFI_IB_EAGER_BUFSIZE / 64));
	rhq_count = HFI_IB_EAGER_BUFSIZE / 64;

	/* set PT update rate mask (re-enable & advance eager-head) */
	rcv->rhq_update_mask = (rhq_count / HFI_IB_PT_UPDATE_RATE) - 1;
	rcv->egr_pending_update = 0;

	/* allocate Eager PT and EQ */
	eq_alloc.ni = HFI_NI_BYPASS;
	eq_alloc.count = rhq_count;
	/*
	 * To support GRH header, RX EQs need to be double-wide.
	 * If memory usage becomes concern, an RSM rule can specify
	 * smaller header/payload split to avoid.
	 */
	eq_alloc.jumbo = true;
	ret = _hfi_eq_alloc(rcv->ctx, &eq_alloc, &rcv->eq);
	if (ret < 0)
		goto init_err;

	/*
	 * Because this EQ is associated with PT, reserve one EQ
	 * entry for flow-control, this is for initial setup only,
	 * firmware will take care the rest.
	 */
	ret = hfi_rx_eq_command(rcv->ctx, &rcv->eq,
				rcv->cmdq_rx, EQ_DESC_RESERVE);
	if (ret < 0)
		goto init_err;

	rcv->egr_last_idx = 0;
	rcv->egr_base = vzalloc_node(HFI_IB_EAGER_BUFSIZE, ctx->devdata->node);
	if (!rcv->egr_base)
		goto init_err;

	hfi_at_reg_range(ctx, rcv->egr_base,
			 HFI_IB_EAGER_BUFSIZE, NULL, true);

	pt_alloc.eq_handle = &rcv->eq;
	pt_alloc.eager_order = HFI_IB_EAGER_COUNT_ORDER;
	ret = hfi_pt_alloc_eager_pending(rcv->pend_cmdq, rcv->ctx, rcv->cmdq_rx,
					 &pt_alloc);
	if (ret < 0)
		goto init_err;

	/* write Eager entries */
	for (i = 0; i < HFI_IB_EAGER_COUNT; i++) {
		u64 done;
		void *egr_start = rcv->egr_base + (i * HFI_IB_EAGER_SIZE);

		done = 0;
		WARN_ON(!PTR_ALIGN(egr_start, 64));
		n_slots = hfi_format_rx_bypass(rcv->ctx, HFI_NI_BYPASS,
					       egr_start,
					       HFI_IB_EAGER_SIZE,
					       HFI_PT_BYPASS_EAGER,
					       rcv->ctx->ptl_uid,
					       HFI_IB_EAGER_PT_FLAGS |
					       PTL_OP_PUT,
					       HFI_CT_NONE,
						/* minfree is max MTU */
					       HFI_DEFAULT_MAX_MTU,
					       (u64)&done,
					       i, HFI_GEN_CC, &rx_cmd);

		ret = hfi_pend_cmd_queue(rcv->pend_cmdq, rcv->cmdq_rx,
					 HFI_EQ_ZERO(rcv->ctx, 0), &rx_cmd,
					 n_slots, GFP_KERNEL);
		if (ret < 0)
			goto init_err;

		/* Check on EQ 0 NI 0 for a PTL_CMD_COMPLETE event */
		ret = hfi_eq_poll_cmd_complete_timeout(rcv->ctx, &done);
		if (ret)
			goto init_err;
	}

	/* kthread create and wait for packets! */
	rcv_task = kthread_create_on_node(hfi2_rcv_wait, rcv,
					  ibp->ibd->assigned_node_id,
					  "hfi2_ibrcv");
	if (IS_ERR(rcv_task)) {
		ret = PTR_ERR(rcv_task);
		goto init_err;
	}
	rcv->task = rcv_task;

	return 0;

init_err:
	dev_err(ibp->dev, "unexpected rcv_init error %d\n", ret);
	hfi2_rcv_uninit(rcv);
	return ret;
}

static int hfi2_rcv_qp_init(struct hfi2_ibport *ibp, int idx)
{
	struct hfi2_ibdev *ibd = ibp->ibd;
	struct hfi2_ibrcv *rcv;
	int ret;

	rcv = kzalloc(sizeof(*rcv), GFP_KERNEL);
	if (!rcv)
		return -ENOMEM;

	ret = hfi2_rcv_init(ibp, ibd->qp_ctx[idx], rcv,
			    idx % ibd->num_send_cmdqs);
	if (ret < 0)
		kfree(rcv);
	else
		ibp->qp_rcv[idx] = rcv;
	return ret;
}

void hfi2_rcv_uninit(struct hfi2_ibrcv *rcv)
{
	int ret;

	if (!rcv->ibp)
		return;

	/* stop further RX processing */
	hfi2_rcv_stop(rcv);

	/* disable PT so EQ buffer and RX buffers can be released */
	ret = hfi_pt_disable_pending(rcv->pend_cmdq, rcv->ctx, rcv->cmdq_rx,
				     HFI_NI_BYPASS, HFI_PT_BYPASS_EAGER);
	if (ret < 0)
		dev_err(rcv->ibp->dev, "unexpected PT disable error %d\n", ret);

	if (rcv->eq.base)
		_hfi_eq_free(&rcv->eq);
	if (rcv->egr_base) {
		hfi_at_dereg_range(rcv->ctx, rcv->egr_base,
				   HFI_IB_EAGER_BUFSIZE);
		vfree(rcv->egr_base);
		rcv->egr_base = NULL;
	}
	rcv->ctx = NULL;
	rcv->ibp = NULL;
}

void hfi2_ctx_start_port(struct hfi2_ibport *ibp)
{
	int i;

	hfi2_rcv_start(&ibp->sm_rcv);
	for (i = 0; i < ibp->ibd->num_qp_ctxs; i++)
		hfi2_rcv_start(ibp->qp_rcv[i]);
}

int hfi2_ctx_init(struct hfi2_ibdev *ibd, int num_ctxs, int num_cmdqs)
{
	int i, ret;
	struct opa_ctx_assign ctx_assign = {0};
	struct hfi_rsm_rule rule = {0};
	struct hfi_ctx *qp_ctx;
	struct hfi_ctx *rcv_ctx[HFI2_IB_MAX_CTXTS];

	if (no_verbs)
		return 0;

	if (num_ctxs > HFI2_IB_MAX_CTXTS ||
	    !is_power_of_2(num_ctxs))
		return -EINVAL;

	if (num_cmdqs > HFI2_IB_MAX_SEND_CTXTS ||
	    !is_power_of_2(num_cmdqs))
		return -EINVAL;
	/* Allocate the management context */
	hfi_ctx_init(&ibd->sm_ctx, ibd->dd);
	ibd->sm_ctx.mode |= HFI_CTX_MODE_USE_BYPASS;
	ctx_assign.pid = HFI_PID_ANY;
	ctx_assign.le_me_count = HFI_IB_EAGER_COUNT;
	ret = hfi_ctx_attach(&ibd->sm_ctx, &ctx_assign);
	if (ret < 0)
		goto err;
	hfi_ctx_set_vl15(&ibd->sm_ctx);
	hfi_ctx_set_mcast(&ibd->sm_ctx);
	ibd->num_send_cmdqs = num_cmdqs;

	/*
	 * Initialize QP contexts first so hfi_ctx_uninit()
	 * can be called for error case below.
	 */
	for (i = 0; i < num_ctxs; i++) {
		qp_ctx = kzalloc(sizeof(*qp_ctx), GFP_KERNEL);
		if (!qp_ctx) {
			ret = -ENOMEM;
			goto err_ctx;
		}
		hfi_ctx_init(qp_ctx, ibd->dd);
		ibd->qp_ctx[i] = qp_ctx;
	}
	ibd->num_qp_ctxs = num_ctxs;

	/*
	 * Allocate the general QP receive contexts.
	 * Hardware will index using QPN_MAP or RSM_MAP.
	 */
	for (i = 0; i < ibd->num_qp_ctxs; i++) {
		qp_ctx = ibd->qp_ctx[i];
		qp_ctx->mode |= HFI_CTX_MODE_USE_BYPASS;
		ctx_assign.pid = HFI_PID_ANY;
		ret = hfi_ctx_attach(qp_ctx, &ctx_assign);
		if (ret < 0)
			goto err_ctx;
		rcv_ctx[i] = qp_ctx;
	}

	/* map QPNs to these receive contexts (9B only) */
	hfi_ctx_set_qp_table(ibd->dd, rcv_ctx, ibd->num_qp_ctxs);

	/* set RSM rule for 16B Verbs receive using L2 and L4 */
	rule.idx = 0;
	rule.pkt_type = 0x4; /* bypass */
	rule.hdr_size = 4; /* 16 bytes header */
	/* match L2 == 16B (0x2)*/
	rule.match_offset[0] = 61;
	rule.match_mask[0] = OPA_BYPASS_L2_MASK;
	rule.match_value[0] = OPA_BYPASS_HDR_16B;
	/* match IB L4s: 0x8 0x9 0xA */
	rule.match_offset[1] = 64;
	rule.match_mask[1] = 0xFC;
	rule.match_value[1] = 0x8;
	if (ibd->num_qp_ctxs > 1) {
		/*
		 * Select context based on SC field only;
		 * Note, we cannot use BTH.DEST_QP because we've matched
		 * on multiple L4s. We can only select using L2 fields.
		 * SC field is 5 bits, hence the min_t here.
		 */
		rule.select_offset[0] = 52;
		rule.select_width[0] = min_t(u8, ilog2(ibd->num_qp_ctxs), 5);
	} else {
		/* disable selection, result is always RSM_MAP index of 0 */
		rule.select_width[0] = 0;
	}
	rule.select_width[1] = 0;
	ret = hfi_rsm_set_rule(ibd->dd, &rule, rcv_ctx, ibd->num_qp_ctxs);
	if (ret < 0)
		goto err_ctx;
	ibd->rsm_mask |= (1 << rule.idx);
	return 0;

err_ctx:
	hfi2_ctx_uninit(ibd);
err:
	return ret;
}

void hfi2_ctx_uninit(struct hfi2_ibdev *ibd)
{
	int i;

	if (no_verbs)
		return;

	for (i = 0; i < HFI_NUM_RSM_RULES; i++)
		if (ibd->rsm_mask & (1 << i))
			hfi_rsm_clear_rule(ibd->dd, i);
	ibd->rsm_mask = 0;
	for (i = 0; i < HFI2_IB_MAX_CTXTS; i++) {
		if (ibd->qp_ctx[i]) {
			hfi_ctx_cleanup(ibd->qp_ctx[i]);
			kfree(ibd->qp_ctx[i]);
			ibd->qp_ctx[i] = NULL;
		}
	}
	hfi_ctx_cleanup(&ibd->sm_ctx);
}

static int hfi2_send_wait(void *data)
{
	struct hfi2_ibtx *ibtx = data;

	allow_signal(SIGINT);
	while (!kthread_should_stop()) {
		hfi2_send_event(&ibtx->send_eq, data);
		msleep(20);
		schedule();
	}
	return 0;
}

static int hfi2_ibtx_init(struct hfi2_ibport *ibp, struct hfi_ctx *ctx,
			  struct hfi2_ibtx *ibtx, int idx)
{
	struct opa_ev_assign eq_alloc;
	int ret;
	char name[16];

	/* Each send context has send EQ for TX completions */
	memset(&eq_alloc, 0, sizeof(eq_alloc));
	eq_alloc.ni = HFI_NI_BYPASS;
	/*
	 * set EQ size equal to CmdQ slots (DMA commands), plus some extra;
	 * must be power of 2, so allocate twice CmdQ size
	 */
	eq_alloc.count = HFI_CMDQ_TX_ENTRIES * 2;
	eq_alloc.isr_cb = hfi2_send_event;
	eq_alloc.cookie = (void *)ibtx;
	ret = _hfi_eq_alloc(ctx, &eq_alloc, &ibtx->send_eq);
	if (ret < 0)
		goto tx_eq_err;

	/* Obtain a pair of command queues for this send context. */
	ret = hfi_cmdq_assign(&ibtx->cmdq, ctx, NULL);
	if (ret)
		goto tx_cmdq_err;
	ret = hfi_cmdq_map(&ibtx->cmdq);
	if (ret)
		goto tx_map_err;

	snprintf(name, sizeof(name), "ib%d", idx);
	ret = hfi_pend_cmdq_info_alloc(ibp->ibd->dd, &ibtx->pend_cmdq, name);
	if (ret < 0)
		goto pend_cmdq_err;

	/* set Send Context pointer last to mark send context as enabled */
	ibtx->ctx = ctx;
	ibtx->ibp = ibp;
	return 0;

pend_cmdq_err:
	hfi_cmdq_unmap(&ibtx->cmdq);
tx_map_err:
	hfi_cmdq_release(&ibtx->cmdq);
tx_cmdq_err:
	_hfi_eq_free(&ibtx->send_eq);
tx_eq_err:
	return 0;
}

static void hfi2_ibtx_uninit(struct hfi2_ibtx *ibtx)
{
	if (!ibtx->ctx)
		return;

	hfi_pend_cmdq_info_free(&ibtx->pend_cmdq);
	hfi_cmdq_unmap(&ibtx->cmdq);
	hfi_cmdq_release(&ibtx->cmdq);
	/* eq_free last as CMDQ release (above) may generate events */
	_hfi_eq_free(&ibtx->send_eq);
	ibtx->ctx = NULL;
}

int hfi2_ctx_init_port(struct hfi2_ibport *ibp)
{
	struct hfi2_ibdev *ibd = ibp->ibd;
	int i, ret;

	if (no_verbs)
		return 0;

	/* allocate pool of send contexts (command queues) */
	for (i = 0; i < ibd->num_send_cmdqs; i++) {
		ret = hfi2_ibtx_init(ibp, &ibd->sm_ctx, &ibp->port_tx[i], i);
		if (ret)
			goto ctx_init_err;
	}

	/* assign device specific RX resources */
	ret = hfi2_rcv_init(ibp, &ibd->sm_ctx, &ibp->sm_rcv, 0);
	if (ret)
		goto ctx_init_err;
	for (i = 0; i < ibd->num_qp_ctxs; i++) {
		ret = hfi2_rcv_qp_init(ibp, i);
		if (ret)
			goto ctx_init_err;
	}

	for (i = 0; i < ibd->num_send_cmdqs; i++) {
		if (ibp->ibd->dd->emulation) {
			struct task_struct *send_task;
			/* FXRTODO: Need to destroy this kthread during uninit */
			/* kthread create and wait for TX events! */
			send_task = kthread_run(hfi2_send_wait, &ibp->port_tx[i], "hfi2_ibsend");
			BUG_ON(IS_ERR(send_task));
		}
	}

	return 0;

ctx_init_err:
	hfi2_ctx_uninit_port(ibp);
	return ret;
}

void hfi2_ctx_uninit_port(struct hfi2_ibport *ibp)
{
	int i;

	if (no_verbs)
		return;

	for (i = 0; i < ibp->ibd->num_qp_ctxs; i++) {
		if (ibp->qp_rcv[i]) {
			hfi2_rcv_uninit(ibp->qp_rcv[i]);
			kfree(ibp->qp_rcv[i]);
			ibp->qp_rcv[i] = NULL;
		}
	}
	hfi2_rcv_uninit(&ibp->sm_rcv);

	for (i = 0; i < ibp->ibd->num_send_cmdqs; i++)
		hfi2_ibtx_uninit(&ibp->port_tx[i]);
}

void hfi2_ctx_assign_qp(struct hfi2_qp_priv *qp_priv, bool is_user)
{
	struct rvt_qp *qp = qp_priv->owner;
	struct hfi2_ibport *ibp = to_hfi_ibp(qp->ibqp.device, qp->port_num);
	int tx_idx;

	if (!ibp->ibd->num_send_cmdqs) {
		qp_priv->s_ctx = NULL;
		return;
	}

	/*
	 * Below uses a simple selection to round robin across
	 * send contexts based on QPN.
	 *
	 * Possible improvements:
	 * First partition user and kernel QPs before using round-robin.
	 * For better quality of service, should consider porting
	 * select_send_context_sc() logic from hfi1.
	 */
	tx_idx = (qp->ibqp.qp_num >> ibp->ibd->rdi.dparms.qos_shift) &
		 (ibp->ibd->num_send_cmdqs - 1);

	qp_priv->s_ctx = &ibp->port_tx[tx_idx];
	dd_dev_dbg(ibp->ibd->dd,
		   "QPT[%d] QPN[%d] assigned send_context[%d]\n",
		   qp->ibqp.qp_type, qp->ibqp.qp_num, tx_idx);
}

void hfi2_ctx_release_qp(struct hfi2_qp_priv *qp_priv)
{
	/*
	 * Note, we can be called during early QP init without
	 * having a send context or even having qp->port_num set.
	 */
	qp_priv->s_ctx = NULL;
}

void hfi_send_cnp(struct hfi2_ibport *ibp, struct rvt_qp *qp, u8 sl, bool grh,
		  bool use_16b)
{
	int ret, slots;
	u8 md_opts, dma_cmd;
	struct hfi2_qp_priv *qp_priv = qp->priv;
	struct hfi2_ibtx *ibtx = qp_priv->s_ctx;

	md_opts = (sl & 0x10) ? NONPTL_SC4 : 0;
	dma_cmd = use_16b ? OFED_16B_DMA : OFED_9B_DMA;
	if (grh)
		dma_cmd++;

	slots = hfi_format_ib_dma_cmd(ibtx->ctx, qp_priv->s_hdr, ibp,
				      0, 0, opa_mtu_to_enum(qp->pmtu), 0,
				      md_opts, &ibtx->send_eq,
				      ibp->port_num, sl,
				      ibp->ppd->lid, true, 0, dma_cmd);
	/* Queue write, don't wait. */
	ret = hfi_pend_cmd_queue(&ibtx->pend_cmdq, &ibtx->cmdq.tx,
				 &ibtx->send_eq, qp_priv->s_hdr, slots,
				 GFP_KERNEL);
	if (ret) {
		dd_dev_warn(ibp->ibd->dd,
			    "%s: hfi_pend_cmd_queue failed %d\n",
			    __func__, ret);
		return;
	}
}
