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
#include <rdma/opa_core_ib.h>
#include <rdma/hfi_args.h>
#include <rdma/hfi_eq.h>
#include <rdma/hfi_pt.h>
#include <rdma/hfi_rx.h>
#include <rdma/hfi_tx.h>

#define OPA_IB_EAGER_COUNT		8 /* minimum Eager entries */
#define OPA_IB_EAGER_COUNT_ORDER	1 /* log2 of above - 2 */
#define OPA_IB_EAGER_SIZE		(PAGE_SIZE * 4)
#define OPA_IB_EAGER_MIN_FREE		2048
/* TODO - still experimenting with 64B alignment (PTL_MAY_ALIGN) */
#define OPA_IB_EAGER_PT_FLAGS		PTL_MANAGE_LOCAL

/* TODO - delete and make common for all kernel clients */
static int _hfi_eq_alloc(struct hfi_ctx *ctx, struct hfi_cq *cq,
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
	eq_alloc->isr_cb = NULL;
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
	if (eq_entry)
		hfi_eq_advance(ctx, cq, 0x0, eq_entry);
err:
	return rc;
}

/* TODO - delete and make common for all kernel clients */
static void _hfi_eq_release(struct hfi_ctx *ctx, struct hfi_cq *cq,
			    hfi_eq_handle_t eq, void *eq_base)
{
	u64 *eq_entry, done;

	ctx->ops->ev_release(ctx, 0, eq, (u64)&done);
	/* Check on EQ 0 NI 0 for a PTL_CMD_COMPLETE event */
	hfi_eq_wait(ctx, 0x0, &eq_entry);
	if (eq_entry)
		hfi_eq_advance(ctx, cq, 0x0, eq_entry);

	vfree(eq_base);
}

int opa_ib_send_wqe(struct opa_ib_portdata *ibp, struct opa_ib_swqe *wqe)
{
	int i, ret;
	unsigned long flags;

	/* TODO - eventually this needs to be rewritten to use MGMT_DMA */

	/*
	 * Test if these WGEs can be coalesced into single PIO send.
	 * If not, return an error as we cannot yet handle IOVEC sends.
	 * TODO - add PIO-IOVEC support when available in opa-headers
	 */
	for (i = 0; i < wqe->s_sge->num_sge; i++) {
		/* TODO only support kernel virtual addresses for now */
		if ((u64)wqe->sg_list[i].vaddr < PAGE_OFFSET)
			return -EFAULT;

		if (i > 0 &&
		    ((wqe->sg_list[i-1].vaddr + wqe->sg_list[i-1].length) !=
		     wqe->sg_list[i].vaddr)) {
			dev_err(ibp->dev,
				"PT %d: PIO-IOVEC not implemented!\n",
				ibp->port_num);
			return -EIO;
		}
	}

	dev_dbg(ibp->dev, "PT %d: Send hdr %p data %p %d\n",
		ibp->port_num, wqe->s_hdr,
		wqe->sg_list[0].vaddr, wqe->length);

	/*
	 * FXRTODO: Mapping sl to sc15 is invalid.
	 * According to latest discussion, there will
	 * be bit to flip or different command if the packet
	 * is MAD. Until that is fixed hardcode sl as 15 so that
	 * the packet(s) take SC15->VL15 path. Only MAD packet
	 * will be supported with this hack
	 *
	 * Ref STL-2662
	 */

	/* send the WQE via PIO path */
	spin_lock_irqsave(&ibp->cmdq_tx_lock, flags);
	ret = hfi_tx_cmd_bypass_pio(&ibp->cmdq_tx, wqe->s_ctx,
				    wqe->s_hdr, 8 + (wqe->s_hdrwords << 2),
				    wqe->sg_list[0].vaddr,
				    wqe->length, ibp->port_num,
				    15, 0, KDETH_9B_PIO);
	spin_unlock_irqrestore(&ibp->cmdq_tx_lock, flags);
	if (ret < 0)
		dev_err(ibp->dev, "PT %d: TX CQ is full!\n", ibp->port_num);
	return ret;
}

void *opa_ib_rcv_get_ebuf(struct opa_ib_portdata *ibp, u16 idx, u32 offset)
{
	int ret;
	size_t buf_offset;

	/* TODO - this needs to be optimized to limit PT_UPDATEs */
	if (idx != ibp->rcv_egr_last_idx) {
		dev_dbg(ibp->dev, "PT %d: EAGER_HEAD UPDATE %d -> %d\n",
			 ibp->port_num, ibp->rcv_egr_last_idx, idx);

		/* Tell HW we are finished reading previous eager buffer */
		ret = hfi_pt_update_eager(ibp->ctx, &ibp->cmdq_rx, idx);
		/* TODO - handle error  */
		BUG_ON(ret < 0);
		ibp->rcv_egr_last_idx = idx;
	}

	/* offset calculation differs based on use of 64B alignment */
	if (OPA_IB_EAGER_PT_FLAGS & PTL_MAY_ALIGN)
		buf_offset = (idx * OPA_IB_EAGER_SIZE) + (offset << 6);
	else
		buf_offset = (idx * OPA_IB_EAGER_SIZE) + offset;
	return ibp->rcv_egr_base + buf_offset;
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
	hfi_eq_advance(ibp->ctx, &ibp->cmdq_rx, ibp->rcv_eq, rhf_entry);
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
	/* TODO - for now, size the RHQ as one event per 128 B of eager buffer */
	rhq_count = total_eager_size / 128;

	/* allocate Eager PT and EQ */
	eq_alloc.ni = HFI_NI_BYPASS;
	eq_alloc.user_data = (unsigned long)ibp;
	eq_alloc.size = rhq_count;
	ret = _hfi_eq_alloc(ibp->ctx, &ibp->cmdq_rx, &eq_alloc, &ibp->rcv_eq,
			    &ibp->rcv_eq_base);
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
					       OPA_IB_EAGER_MIN_FREE, (unsigned long)&done,
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

	if (ibp->port_num != 0)
		return;

	/* stop further RX processing */
	opa_ib_rcv_stop(ibp);

	/* disable PT so EQ buffer and RX buffers can be released */
	ret = hfi_pt_disable(ibp->ctx, &ibp->cmdq_rx, HFI_NI_BYPASS,
			     HFI_PT_BYPASS_EAGER);
	/* TODO - handle error  */
	BUG_ON(ret < 0);

	if (ibp->rcv_eq_base) {
		_hfi_eq_release(ibp->ctx, &ibp->cmdq_rx, ibp->rcv_eq, ibp->rcv_eq_base);
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
	int ret;
	u16 cq_idx;

	spin_lock_init(&ibp->cmdq_tx_lock);
	/*
	 * NOTE, this RX lock currently unused as this Command Queue
	 * is used exclusively by the spawned kthread.
	 */
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

	/* assign device specific RX resources */
	ret = opa_ib_rcv_init(ibp);
	if (ret)
		goto rcv_init_err;

	return 0;

rcv_init_err:
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
