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

#include <linux/delay.h>
#include "verbs.h"
#include "packet.h"
#include <rdma/opa_core_ib.h>
#include <rdma/hfi_args.h>
#include <rdma/hfi_eq.h>
#include <rdma/hfi_pt.h>
#include <rdma/hfi_rx.h>
#include <rdma/hfi_tx.h>

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
			 uint16_t *eq, void **eq_base)
{
	u32 *eq_head_array, *eq_head_addr;
	u64 *eq_entry, done;
	int rc;

	eq_alloc->base = (u64)vzalloc(eq_alloc->size * HFI_EQ_ENTRY_SIZE);
	if (!eq_alloc->base)
		return -ENOMEM;
	eq_alloc->mode = OPA_EV_MODE_BLOCKING;
	eq_alloc->user_data = (u64)&done;
	rc = ctx->ops->ev_assign(ctx, eq_alloc);
	if (rc < 0) {
		vfree((void *)eq_alloc->base);
		goto err;
	}
	eq_head_array = ctx->eq_head_addr;
	/* Reset the EQ SW head */
	eq_head_addr = &eq_head_array[eq_alloc->ev_idx];
	*eq_head_addr = 0;
	*eq = eq_alloc->ev_idx;
	*eq_base = (void *)eq_alloc->base;

	/* Check on EQ 0 NI 0 for a PTL_CMD_COMPLETE event */
	hfi_eq_wait(ctx, 0x0, &eq_entry);
	if (eq_entry) {
		unsigned long flags;

		spin_lock_irqsave(cq_lock, flags);
		hfi_eq_advance(ctx, cq, 0x0, eq_entry);
		spin_unlock_irqrestore(cq_lock, flags);
	}
err:
	return rc;
}

/* TODO - delete and make common for all kernel clients */
static void _hfi_eq_release(struct hfi_ctx *ctx, struct hfi_cq *cq,
			    spinlock_t *cq_lock,
			    hfi_eq_handle_t eq, void *eq_base)
{
	u64 *eq_entry, done;

	ctx->ops->ev_release(ctx, 0, eq, (u64)&done);
	/* Check on EQ 0 NI 0 for a PTL_CMD_COMPLETE event */
	hfi_eq_wait(ctx, 0x0, &eq_entry);
	if (eq_entry) {
		unsigned long flags;

		spin_lock_irqsave(cq_lock, flags);
		hfi_eq_advance(ctx, cq, 0x0, eq_entry);
		spin_unlock_irqrestore(cq_lock, flags);
	}

	vfree(eq_base);
}

/*
 * compute how many IOVECs we need at most for sending
 *
 * TODO may need to determine packet boundaries depending on how auto-header
 * optimization is implemented in upper layers (need add'l header IOVECs)
 */
static int qp_max_iovs(struct opa_ib_portdata *ibp, struct opa_ib_qp *qp,
		       int *out_niovs)
{
	int niovs, i;
	size_t off, segsz, payload_bytes;
	struct hfi2_sge *sge;
	struct opa_ib_sge_state *ss = qp->s_cur_sge;

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
			off = sge->mr->map[sge->m]->segs[sge->n].length - sge->length;
			niovs += ((off + sge->sge_length + segsz - 1) / segsz);
		} else {
			niovs++;
		}
	}

	/* TODO - delete below when TX firmware supports this */
	if (niovs > 4) {
		dev_err(ibp->dev,
			"PT %d: large IOVEC not supported in simics (%d)\n",
			ibp->port_num, niovs);
		return -EIO;
	}

	*out_niovs = niovs;
	return 0;
}

/*
 * update QP state that is tracking progress within the SG list to note
 * that length bytes have been sent
 */
static void qp_update_sge(struct opa_ib_qp *qp, u32 length)
{
	struct opa_ib_sge_state *ss = qp->s_cur_sge;
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

/* TODO - keep around for now, in case this hack has value again during bring-up */
static int __attribute__ ((unused))
send_wqe_pio(struct opa_ib_portdata *ibp, struct opa_ib_swqe *wqe)
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
	ret = hfi_tx_cmd_bypass_pio(&ibp->cmdq_tx, wqe->s_ctx,
				    wqe->s_hdr, 8 + (wqe->s_hdrwords << 2),
				    wqe->sg_list[0].vaddr,
				    wqe->length, ibp->port_num,
				    wqe->sl, 0, KDETH_9B_PIO);
	spin_unlock_irqrestore(&ibp->cmdq_tx_lock, flags);

	/* caller must handle retransmit due to no slots (-EAGAIN) */
	return ret;
}

static void opa_ib_send_event(void *data)
{
	struct opa_ib_portdata *ibp = data;
	struct opa_ib_qp *qp;
	struct opa_ib_swqe *wqe;
	struct hfi2_wqe_iov *wqe_iov;
	unsigned long flags;
	union initiator_EQEntry *eq_entry;

next_event:
	eq_entry = NULL;
	hfi_eq_peek(ibp->ctx, ibp->send_eq, (uint64_t **)&eq_entry);
	if (!eq_entry)
		return;

	if (eq_entry->event_kind != NON_PTL_EVENT_TX_COMPLETE) {
		dev_warn(ibp->dev, "PT %d: unexpected EQ kind %x\n",
			 ibp->port_num, eq_entry->event_kind);
		goto eq_advance;
	}

	wqe_iov = (struct hfi2_wqe_iov *)eq_entry->user_ptr;
	wqe = wqe_iov->wqe;
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
		if (wqe->pkt_errors)
			opa_ib_send_complete(qp, wqe, IB_WC_FATAL_ERR);
		else
			opa_ib_send_complete(qp, wqe, IB_WC_SUCCESS);
		spin_unlock(&qp->s_lock);
	}
	kfree(wqe_iov);

eq_advance:
	spin_lock_irqsave(&ibp->cmdq_rx_lock, flags);
	hfi_eq_advance(ibp->ctx, &ibp->cmdq_rx, ibp->send_eq,
		       (uint64_t *)eq_entry);
	spin_unlock_irqrestore(&ibp->cmdq_rx_lock, flags);

	goto next_event;
}

int opa_ib_send_wqe(struct opa_ib_portdata *ibp, struct opa_ib_qp *qp,
		    struct opa_ib_swqe *wqe)
{
	int i, ret, ndelays = 0;
	unsigned long flags;
	uint8_t dma_cmd;
	union base_iovec *iov = NULL;
	struct hfi2_wqe_iov *wqe_iov = NULL;
	uint32_t num_iovs, total_length;
	struct hfi2_sge *sge;
	size_t payload_bytes;

	dma_cmd = (wqe->use_sc15) ? MGMT_DMA : GENERAL_DMA;
	/* TODO - optimize for OFED_DMA here */

	ret = qp_max_iovs(ibp, qp, &num_iovs);
	if (ret)
		return ret;

	wqe_iov = kzalloc(sizeof(*wqe_iov) + sizeof(*iov) * num_iovs,
			  GFP_ATOMIC);
	if (!wqe_iov)
		return -ENOMEM;

	wqe_iov->wqe = wqe;
	wqe_iov->remaining_bytes = qp->s_len;
	iov = wqe_iov->iov;

	/* first entry is IB header */
	iov[0].length = (wqe->s_hdrwords << 2);
	iov[0].use_9b = 1;
	iov[0].sp = 1;
	iov[0].v = 1;
	/*
	 * store a copy of the IB header as MIDDLE and LAST packets modify it,
	 * and we'd like to maintain asyncronous send and send_complete
	 * TODO - can be removed by refactoring where upper layer builds header
	 */
	memcpy(&wqe_iov->ib_hdr, &wqe->s_hdr->ibh, iov[0].length);
	iov[0].start = (uint64_t)(&wqe_iov->ib_hdr);

	/* write IOVEC entries */
	total_length = qp->s_cur_size;
	payload_bytes = 0;
	for (i = 1; total_length && i < num_iovs; i++) {
		uint32_t iov_length;

		/* get current SGE, qp_update_sge() advances */
		sge = &qp->s_sge.sge;

		/* use length from next MR segment */
		iov_length = sge->length;
		/* the SGE can describe less than the MR segment */
		if (iov_length > sge->sge_length)
			iov_length = sge->sge_length;
		/* TODO SGE can be larger than total_length? */
		if (iov_length > total_length)
			iov_length = total_length;

		/*
		 * If payload is now larger than MTU, start new packet;
		 * caller doesn't do this currently, but test for it
		 * TODO - may need if auto-header optimization
		 */
		if (payload_bytes + iov_length > wqe->pmtu && i > 1) {
			dev_err(ibp->dev,
				"PT %d: large DMA not implemented, %ld > %d MTU!\n",
				ibp->port_num,
				payload_bytes + iov_length, wqe->pmtu);
			ret = -EIO;
			goto err;
		}

		iov[i].length = iov_length;
		iov[i].use_9b = 1;
		iov[i].ep = 0;
		iov[i].sp = 0;
		iov[i].v = 1;
		iov[i].start = (uint64_t)sge->vaddr;
		payload_bytes += iov_length;
		total_length -= iov_length;

		/* update internal state in QP tracking SG_list progress */
		qp_update_sge(qp, iov_length);
	}

	if (total_length) {
		dev_err(ibp->dev,
			"PT %d: TX error, %d bytes unsent after %d iovs\n",
			ibp->port_num, total_length, num_iovs);
		ret = -EIO;
		goto err;
	}

	/* adjust iovs actually used, earlier calculation can be larger */
	num_iovs = i;
	iov[num_iovs - 1].ep = 1;

	dev_dbg(ibp->dev, "PT %d: cmd %d len %d/%d pmtu %d n_iov %d sent %ld\n",
		ibp->port_num, dma_cmd, qp->s_cur_size, wqe->length, wqe->pmtu,
		num_iovs, payload_bytes);

_tx_cmd:
	/* send with GENERAL or MGMT DMA */
	spin_lock_irqsave(&ibp->cmdq_tx_lock, flags);
	ret = hfi_tx_cmd_bypass_dma(&ibp->cmdq_tx, wqe->s_ctx,
				    (uint64_t)iov, num_iovs,
				    (uint64_t)wqe_iov, PTL_MD_RESERVED_IOV,
				    ibp->send_eq, HFI_CT_NONE,
				    ibp->port_num, wqe->sl, 0,
				    HDR_EXT /* 9B */, dma_cmd);
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

void *opa_ib_rcv_get_ebuf(struct opa_ib_portdata *ibp, u16 idx, u32 offset)
{
	int ret;
	unsigned long flags;

	/* TODO - this needs to be optimized to limit PT_UPDATEs */
	if (idx != ibp->rcv_egr_last_idx) {
		dev_dbg(ibp->dev, "PT %d: EAGER_HEAD UPDATE %d -> %d\n",
			ibp->port_num, ibp->rcv_egr_last_idx, idx);

		/* Tell HW we are finished reading previous eager buffer */
		spin_lock_irqsave(&ibp->cmdq_rx_lock, flags);
		ret = hfi_pt_update_eager(ibp->ctx, &ibp->cmdq_rx, idx);
		spin_unlock_irqrestore(&ibp->cmdq_rx_lock, flags);
		/* TODO - handle error  */
		BUG_ON(ret < 0);
		ibp->rcv_egr_last_idx = idx;
	}

	return ibp->rcv_egr_base + (idx * OPA_IB_EAGER_SIZE) + offset;
}

int _opa_ib_rcv_wait(struct opa_ib_portdata *ibp, u64 **rhf_entry)
{
	int rc;

	rc = hfi_eq_wait_irq(ibp->ctx, ibp->rcv_eq, -1,
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

void opa_ib_rcv_advance(struct opa_ib_portdata *ibp, u64 *rhf_entry)
{
	unsigned long flags;

	spin_lock_irqsave(&ibp->cmdq_rx_lock, flags);
	hfi_eq_advance(ibp->ctx, &ibp->cmdq_rx, ibp->rcv_eq, rhf_entry);
	spin_unlock_irqrestore(&ibp->cmdq_rx_lock, flags);
}

void opa_ib_rcv_start(struct opa_ib_portdata *ibp)
{
	/* RX events can be received after this thread becomes runable */
	if (ibp->rcv_task)
		wake_up_process(ibp->rcv_task);
}

static void opa_ib_rcv_stop(struct opa_ib_portdata *ibp)
{
	if (ibp->rcv_task) {
		/* stop the RX thread, signal it to break out of wait_event */
		send_sig(SIGINT, ibp->rcv_task, 1);
		kthread_stop(ibp->rcv_task);
		ibp->rcv_task = NULL;
	}
}

int opa_ib_rcv_init(struct opa_ib_portdata *ibp)
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
	eq_alloc.size = rhq_count;
	ret = _hfi_eq_alloc(ibp->ctx, &ibp->cmdq_rx, &ibp->cmdq_rx_lock,
			    &eq_alloc, &ibp->rcv_eq, &ibp->rcv_eq_base);
	if (ret < 0)
		goto kthread_err;

	ibp->rcv_egr_last_idx = 0;
	ibp->rcv_egr_base = vzalloc(total_eager_size);
	if (!ibp->rcv_egr_base)
		goto kthread_err;

	pt_alloc.eq_handle = ibp->rcv_eq;
	pt_alloc.eager_order = OPA_IB_EAGER_COUNT_ORDER;
	ret = hfi_pt_alloc_eager(ibp->ctx, &ibp->cmdq_rx, &pt_alloc);
	if (ret < 0)
		goto kthread_err;

	/* write Eager entries */
	for (i = 0; i < OPA_IB_EAGER_COUNT; i++) {
		u64 *eq_entry, done;

		n_slots = hfi_format_rx_bypass(ibp->ctx, HFI_NI_BYPASS,
					       ibp->rcv_egr_base + (i*OPA_IB_EAGER_SIZE),
					       OPA_IB_EAGER_SIZE,
					       HFI_PT_BYPASS_EAGER,
					       ibp->ctx->ptl_uid,
					       OPA_IB_EAGER_PT_FLAGS | PTL_OP_PUT,
					       HFI_CT_NONE,
					       ibp->ibmaxmtu, /* minfree is max MTU */
					       (unsigned long)&done,
					       i, &rx_cmd);
		ret = hfi_rx_command(&ibp->cmdq_rx, (uint64_t *)&rx_cmd, n_slots);
		if (ret < 0)
			goto kthread_err;

		/* Check on EQ 0 NI 0 for a PTL_CMD_COMPLETE event */
		hfi_eq_wait(ibp->ctx, 0x0, &eq_entry);
		if (eq_entry)
			hfi_eq_advance(ibp->ctx, &ibp->cmdq_rx, 0x0, eq_entry);
	}

	/* kthread create and wait for packets! */
	rcv_task = kthread_create_on_node(opa_ib_rcv_wait, ibp,
					  ibp->ibd->assigned_node_id,
					  "opa2_ib_rcv");
	if (IS_ERR(rcv_task)) {
		ret = PTR_ERR(rcv_task);
		goto kthread_err;
	}
	ibp->rcv_task = rcv_task;

	return 0;

kthread_err:
	opa_ib_rcv_uninit(ibp);
	return ret;
}

void opa_ib_rcv_uninit(struct opa_ib_portdata *ibp)
{
	int ret;
	unsigned long flags;

	if (ibp->port_num != 0)
		return;

	/* stop further RX processing */
	opa_ib_rcv_stop(ibp);

	/* disable PT so EQ buffer and RX buffers can be released */
	spin_lock_irqsave(&ibp->cmdq_rx_lock, flags);
	ret = hfi_pt_disable(ibp->ctx, &ibp->cmdq_rx, HFI_NI_BYPASS,
			     HFI_PT_BYPASS_EAGER);
	spin_unlock_irqrestore(&ibp->cmdq_rx_lock, flags);
	/* TODO - handle error  */
	BUG_ON(ret < 0);

	if (ibp->rcv_eq_base) {
		_hfi_eq_release(ibp->ctx, &ibp->cmdq_rx, &ibp->cmdq_rx_lock,
				ibp->rcv_eq, ibp->rcv_eq_base);
		ibp->rcv_eq_base = NULL;
	}
	if (ibp->rcv_egr_base) {
		vfree(ibp->rcv_egr_base);
		ibp->rcv_egr_base = NULL;
	}
}

int opa_ib_ctx_init(struct opa_ib_data *ibd)
{
	struct opa_core_device *odev = ibd->odev;
	struct opa_ctx_assign ctx_assign = {0};

	/*
	 * Allocate the management context.
	 * TODO - later we will likely allocate a pool of general contexts
	 * to choose based on QPN or SC.
	 */
	HFI_CTX_INIT(&ibd->ctx, odev->dd, odev->bus_ops);
	ibd->ctx.mode |= HFI_CTX_MODE_BYPASS_9B;
	ibd->ctx.qpn_map_idx = 0; /* this context is for QPN 0 and 1 */
	/* TODO - for now map all QPNs to this receive context */
	ibd->ctx.qpn_map_count = OPA_QPN_MAP_MAX;
	ctx_assign.pid = HFI_PID_ANY;
	ctx_assign.le_me_count = OPA_IB_EAGER_COUNT;
	return odev->bus_ops->ctx_assign(&ibd->ctx, &ctx_assign);
}

void opa_ib_ctx_uninit(struct opa_ib_data *ibd)
{
	ibd->odev->bus_ops->ctx_release(&ibd->ctx);
}

int opa_ib_ctx_init_port(struct opa_ib_portdata *ibp)
{
	struct opa_core_ops *ops = ibp->odev->bus_ops;
	struct opa_ib_data *ibd = ibp->ibd;
	struct opa_ev_assign eq_alloc;
	int ret;
	u16 cq_idx;

	spin_lock_init(&ibp->cmdq_tx_lock);
	spin_lock_init(&ibp->cmdq_rx_lock);

	/*
	 * Obtain a pair of command queues for this port for
	 * the management context.
	 * TODO - later we will likely allocate a pool of general Contexts
	 * and associated CMDQs.
	 */
	ret = ops->cq_assign(&ibd->ctx, NULL, &cq_idx);
	if (ret)
		goto err;
	ret = ops->cq_map(&ibd->ctx, cq_idx, &ibp->cmdq_tx, &ibp->cmdq_rx);
	if (ret) {
		ops->cq_release(&ibd->ctx, cq_idx);
		goto err;
	}
	/* set Context pointer only after CMDQs allocated */
	ibp->ctx = &(ibd->ctx);

	/* Each port has send EQ for TX completions */
	memset(&eq_alloc, 0, sizeof(eq_alloc));
	eq_alloc.ni = HFI_NI_BYPASS;
	eq_alloc.user_data = (unsigned long)ibp;
	/* TODO - set EQ size equal to CmdQ slots (DMA commands) for now */
	eq_alloc.size = HFI_CQ_TX_ENTRIES;
	eq_alloc.isr_cb = opa_ib_send_event;
	eq_alloc.cookie = (void *)ibp;
	ret = _hfi_eq_alloc(ibp->ctx, &ibp->cmdq_rx, &ibp->cmdq_rx_lock,
			    &eq_alloc, &ibp->send_eq, &ibp->send_eq_base);
	if (ret < 0)
		goto ctx_init_err;

	/* assign device specific RX resources */
	ret = opa_ib_rcv_init(ibp);
	if (ret)
		goto ctx_init_err;

	return 0;

ctx_init_err:
	opa_ib_ctx_uninit_port(ibp);
err:
	return ret;
}

void opa_ib_ctx_uninit_port(struct opa_ib_portdata *ibp)
{
	struct opa_core_ops *ops = ibp->odev->bus_ops;
	u16 cq_idx = ibp->cmdq_tx.cq_idx;

	if (!ibp->ctx)
		return;

	if (ibp->send_eq_base) {
		_hfi_eq_release(ibp->ctx, &ibp->cmdq_rx, &ibp->cmdq_rx_lock,
				ibp->send_eq, ibp->send_eq_base);
		ibp->send_eq_base = NULL;
	}

	opa_ib_rcv_uninit(ibp);
	ops->cq_unmap(&ibp->cmdq_tx, &ibp->cmdq_rx);
	ops->cq_release(ibp->ctx, cq_idx);
	ibp->ctx = NULL;
}

int opa_ib_ctx_assign_qp(struct opa_ib_data *ibd, struct opa_ib_qp *qp,
			 bool is_user)
{
	int ret;

	/* hold the device so it cannot be removed while QP is active */
	ret = opa_core_device_get(ibd->odev);
	if (ret)
		return ret;

	qp->s_ctx = &ibd->ctx;
	return 0;
}

void opa_ib_ctx_release_qp(struct opa_ib_data *ibd, struct opa_ib_qp *qp)
{
	qp->s_ctx = NULL;
	opa_core_device_put(ibd->odev);
}
