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
 * Intel(R) Omni-Path Gen2 HFI PCIe Driver
 */

#include <linux/interrupt.h>
/* TODO - prefer hfi_cmd.h to only be included by files in hfi2_user.ko */
#include <rdma/hfi_cmd.h>
#include "opa_hfi.h"
#include "link.h"
#include "fxr/fxr_fc_defs.h"
#include "fxr/fxr_hifis_defs.h"
#include "fxr/fxr_loca_defs.h"
#include "fxr/fxr_pcim_defs.h"
#include "fxr/fxr_tx_otr_pkt_top_csrs_defs.h"
#include "fxr/fxr_tx_otr_msg_top_csrs_defs.h"
#include "fxr/fxr_tx_dma_defs.h"
#include "fxr/fxr_tx_ci_cic_csrs_defs.h"
#include "fxr/fxr_tx_ci_cid_csrs_defs.h"
#include "fxr/fxr_rx_ci_cic_csrs_defs.h"
#include "fxr/fxr_rx_ci_cid_csrs_defs.h"
#include "fxr/fxr_rx_e2e_defs.h"
#include "fxr/fxr_rx_hp_defs.h"
#include "fxr/fxr_rx_dma_defs.h"
#include "fxr/fxr_rx_et_defs.h"
#include "fxr/fxr_rx_hiarb_defs.h"
#include "fxr/fxr_at_defs.h"
#include "fxr/mnh_opio_defs.h"
#include "fxr/fxr_perfmon_defs.h"
#include "fxr/fxr_linkmux_defs.h"
#include "fxr/fxr_linkmux_tp_defs.h"
#include "fxr/fxr_linkmux_fpc_defs.h"
#include "fxr/fxr_linkmux_cm_defs.h"

/*
 * error domain and error event processing structure.
 */
struct hfi_error_event {
	char *event_name;
	char *event_desc;
	/* we can add action routine here */
	void (*action)(struct hfi_devdata *dd, u64 err_sts, char *name);
};

struct hfi_error_csr {
	char *csr_name;
	uint64_t err_en_host;
	uint64_t err_first_host;
	uint64_t err_sts;		/* status CSR */
	uint64_t err_clr;		/* clear CSR */
	uint64_t err_frc;		/* set CSR */
	struct hfi_error_event events[64];
};

struct hfi_error_domain {
	struct hfi_error_csr	*csr;
	int			count;
	int			irq;
};

#define HFI_ERROR_DISABLE	-1
#define HFI_MAX_ERR_PER_CTXT	256

/*
 * error entry for error queue.
 */
struct hfi_error_entry {
	struct hfi_async_error ae;
	struct list_head list;	/* context queue */
};

/*
 * Each error csr definition.
 */
#include "errdef.h"

/*
 * Interrupts assignment for each error domains.
 */
#define HFI_FZC0_IRQ		257
#define HFI_FZC1_IRQ		258
#define HFI_HIFIS_IRQ		259
#define HFI_LOCA_IRQ		260
#define HFI_PCIM_IRQ		261
#define HFI_TXCID_IRQ		262
#define HFI_OTR_IRQ		263
#define HFI_TXDMA_IRQ		264
#define HFI_LM_IRQ		265
#define HFI_RXE2E_IRQ		266
#define HFI_RXHP_IRQ		267
#define HFI_RXDMA_IRQ		268
#define HFI_RXET_IRQ		269
#define HFI_RXHIARB_IRQ		270
#define HFI_AT_IRQ		271
#define HFI_OPIO_IRQ		272
#define HFI_RXCID_IRQ		273
#define HFI_FPC0_IRQ		274
#define HFI_FPC1_IRQ		275
#define HFI_TP0_IRQ		276
#define HFI_TP1_IRQ		277
#define HFI_PMON_IRQ		278

/* macros for error bits in FPC_ERR_STS reg */
/* UnCorrectable Error */
#define FPC_UC_ERR	13
/* Link Error */
#define FPC_LINK_ERR	19
/* FmConfig Error */
#define FPC_FMC_ERR	54
/* RcvPort Error */
#define FPC_RP_ERR	55

/*
 * When HFILM_ERR is added, this will be 22
 */
#define HFI_NUM_ERR_DOMAIN	21

/*
 * Put all error domains into an array structure
 */
#define HFI_MAKE_DOMAIN(e, irq)	\
	{e, sizeof(e) / sizeof(struct hfi_error_csr), irq }

static struct hfi_error_domain hfi_error_domain[] = {
	HFI_MAKE_DOMAIN(hfi_fzc0_error, HFI_FZC0_IRQ),
	HFI_MAKE_DOMAIN(hfi_fzc1_error, HFI_FZC1_IRQ),
	HFI_MAKE_DOMAIN(hfi_hifis_error, HFI_HIFIS_IRQ),
	HFI_MAKE_DOMAIN(hfi_loca_error, HFI_LOCA_IRQ),
	HFI_MAKE_DOMAIN(hfi_pcim_error, HFI_PCIM_IRQ),
	HFI_MAKE_DOMAIN(hfi_txcid_error, HFI_TXCID_IRQ),
	HFI_MAKE_DOMAIN(hfi_otr_error, HFI_OTR_IRQ),
	HFI_MAKE_DOMAIN(hfi_txdma_error, HFI_TXDMA_IRQ),
	/* add LM error here */
	HFI_MAKE_DOMAIN(hfi_rxe2e_error, HFI_RXE2E_IRQ),
	HFI_MAKE_DOMAIN(hfi_rxhp_error, HFI_RXHP_IRQ),
	HFI_MAKE_DOMAIN(hfi_rxdma_error, HFI_RXDMA_IRQ),
	HFI_MAKE_DOMAIN(hfi_rxet_error, HFI_RXET_IRQ),
	HFI_MAKE_DOMAIN(hfi_rxhiarb_error, HFI_RXHIARB_IRQ),
	HFI_MAKE_DOMAIN(hfi_at_error, HFI_AT_IRQ),
	HFI_MAKE_DOMAIN(hfi_opio_error, HFI_OPIO_IRQ),
	HFI_MAKE_DOMAIN(hfi_rxcid_error, HFI_RXCID_IRQ),
	HFI_MAKE_DOMAIN(hfi_fpc0_error, HFI_FPC0_IRQ),
	HFI_MAKE_DOMAIN(hfi_fpc1_error, HFI_FPC1_IRQ),
	HFI_MAKE_DOMAIN(hfi_tp0_error, HFI_TP0_IRQ),
	HFI_MAKE_DOMAIN(hfi_tp1_error, HFI_TP1_IRQ),
	HFI_MAKE_DOMAIN(hfi_pmon_error, HFI_PMON_IRQ)
};

/*
 * IOMMU page group request error, dispatch to the target process
 */
void hfi_handle_pgrrsp_error(struct hfi_devdata *dd, u64 err_sts, char *name)
{
	struct hfi_ctx *ctx;
	struct hfi_ctx_error *error;
	struct hfi_error_queue *errq;
	struct hfi_error_entry *entry;
	u64 info1f, info1g;
	int pasid, cid;
	char *client;

	/* read page group request error info */
	info1f = read_csr(dd, FXR_AT_ERR_INFO_1F);
	info1g = read_csr(dd, FXR_AT_ERR_INFO_1G);
	pasid = info1f & FXR_AT_ERR_INFO_1F_PGR_ERROR_PASID_MASK;
	cid = (info1f >> FXR_AT_ERR_INFO_1F_PGR_ERROR_CLIENT_SHIFT) &
		FXR_AT_ERR_INFO_1F_PGR_ERROR_CLIENT_MASK;
	if (cid == 0x1)
		client = "TXDMA";
	else if (cid == 0x2)
		client = "TXOTR";
	else if (cid == 0x4)
		client = "RXDMA";
	else
		client = "Unknown";

	dd_dev_err(dd, "pgr errinfo: client=%s, pasid=%d\n",
		   client, pasid);
	dd_dev_err(dd, "vaddr=0x%llx, r=%lld w=%lld pa=%lld\n",
		   (info1g & FXR_AT_ERR_INFO_1G_PGR_ERROR_VA_SMASK),
		   (info1g >> FXR_AT_ERR_INFO_1G_PGR_ERROR_R_SHIFT) &
			FXR_AT_ERR_INFO_1G_PGR_ERROR_R_MASK,
		   (info1g >> FXR_AT_ERR_INFO_1G_PGR_ERROR_W_SHIFT) &
			FXR_AT_ERR_INFO_1G_PGR_ERROR_W_MASK,
		   (info1g >> FXR_AT_ERR_INFO_1G_PGR_ERROR_PA_SHIFT) &
			FXR_AT_ERR_INFO_1G_PGR_ERROR_PA_MASK);

	spin_lock(&dd->error_dispatch_lock);
	list_for_each_entry(error, &dd->error_dispatch_head, list) {
		ctx = container_of(error, struct hfi_ctx, error);
		if (ctx->pasid != pasid)
			continue;

		errq = &error->queue;
		spin_lock(&errq->lock);

		/* queue is full, drop new error */
		if (errq->count == HFI_MAX_ERR_PER_CTXT) {
			spin_unlock(&errq->lock);
			break;
		}

		/* No sleep, if no memory, terminate dispatching */
		entry = kmalloc(sizeof(*entry), GFP_ATOMIC);
		if (!entry) {
			spin_unlock(&errq->lock);
			break;
		}

		/* put new error into queue */
		entry->ae.etype = HFI_PGR_ERR;
		entry->ae.pgrinfo = info1g;
		list_add_tail(&entry->list, &errq->head);
		errq->count++;

		spin_unlock(&errq->lock);

		/* wake up waiting thread */
		wake_up_interruptible(&errq->waitq);

		/* only send to single ctx */
		break;
	}
	spin_unlock(&dd->error_dispatch_lock);
}

/*
 * error domain interrupt handler.
 */
irqreturn_t hfi_irq_errd_handler(int irq, void *dev_id)
{
	struct hfi_irq_entry *me = dev_id;
	struct hfi_devdata *dd = me->dd;
	struct hfi_error_domain *domain = me->arg;
	struct hfi_error_csr *csr = domain->csr;
	struct hfi_error_event *event;
	int count = domain->count;
	int i, j;
	u64 val;

	/* FXRTODO: remove this acking after simics bug fixed */
	hfi_ack_interrupt(me);

	val = 0;

	/* disable interrupt for domains using this irq */
	for (i = 0; i < count; i++)
		write_csr(dd, csr[i].err_en_host, val);

	/* read all err_sts csr */
	for (i = 0; i < count; i++) {
		val = read_csr(dd, csr[i].err_sts);
		if (!val)
			continue;

		for (j = 0; j < 64; j++) {
			if (val & (1uLL << j)) {
				event = &csr[i].events[j];
				dd_dev_err(dd, "%s:%s:%s\n",
					csr[i].csr_name,
					event->event_name,
					event->event_desc);
				if (event->action)
					event->action(dd, (1uLL << j),
						      csr->csr_name);
			}
		}
	}

	/* clear or set all bits */
	val = ~0;

	/* clear err_sts bits */
	for (i = 0; i < count; i++)
		write_csr(dd, csr[i].err_clr, val);

	/* enable interrupt again for the domain */
	for (i = 0; i < count; i++)
		write_csr(dd, csr[i].err_en_host, val);

	return IRQ_HANDLED;
}

/*
 * configure and setup driver to receive error domain interrupt.
 */
int hfi_setup_errd_irq(struct hfi_devdata *dd)
{
	struct hfi_irq_entry *me;
	struct hfi_error_csr *csr;
	int count, irq;
	int i, j, ret;
	u64 val;

	/*
	 * Set address translation error event handler:
	 * 11: PageGroup Response Error
	 */
	hfi_at_error[0].events[11].action = hfi_handle_pgrrsp_error;

	hfi_fpc0_error[0].events[FPC_UC_ERR].action =
				hfi_handle_fpc_uncorrectable_error;
	hfi_fpc0_error[0].events[FPC_LINK_ERR].action =
				hfi_handle_fpc_link_error;
	hfi_fpc0_error[0].events[FPC_FMC_ERR].action =
				hfi_handle_fpc_fmconfig_error;
	hfi_fpc0_error[0].events[FPC_RP_ERR].action =
				hfi_handle_fpc_rcvport_error;
	hfi_fpc1_error[0].events[FPC_UC_ERR].action =
				hfi_handle_fpc_uncorrectable_error;
	hfi_fpc1_error[0].events[FPC_LINK_ERR].action =
				hfi_handle_fpc_link_error;
	hfi_fpc1_error[0].events[FPC_FMC_ERR].action =
				hfi_handle_fpc_fmconfig_error;
	hfi_fpc1_error[0].events[FPC_RP_ERR].action =
				hfi_handle_fpc_rcvport_error;

	/* Setup interrupt handler for each IRQ */

	/* set all bits */
	val = ~0;
	BUILD_BUG_ON(sizeof(hfi_error_domain) !=
		(HFI_NUM_ERR_DOMAIN * sizeof(struct hfi_error_domain)));

	for (i = 0; i < HFI_NUM_ERR_DOMAIN; i++) {
		irq = hfi_error_domain[i].irq;
		me = &dd->irq_entries[irq];

		BUG_ON(me->arg != NULL);
		me->dd = dd;
		me->intr_src = irq;

		/* enable interrupt from domains */
		count = hfi_error_domain[i].count;
		csr = hfi_error_domain[i].csr;
		for (j = 0; j < count; j++)
			write_csr(dd, csr[j].err_en_host, val);

		/* if intx interrupt, we continue here */
		if (!dd->num_irq_entries) {
			/* mark as in use */
			me->arg = &hfi_error_domain[i];
			continue;
		}

		dd_dev_info(dd, "request for msix IRQ %d:%d\n",
			    irq, dd->msix[irq].vector);
		ret = request_irq(dd->msix[irq].vector, hfi_irq_errd_handler, 0,
				  "hfi_irq_errd", me);
		if (ret) {
			dd_dev_err(dd, "msix IRQ[%d] request failed %d\n",
				   irq, ret);
			/* IRQ cleanup done in hfi_pci_dd_free() */
			return ret;
		}
		/* mark as in use */
		me->arg = &hfi_error_domain[i];
	}

	return 0;
}

/*
 * A context chooses to listen the interrupt error,
 * here we register the context's error queue into
 * the listening list.
 */
void hfi_setup_errq(struct hfi_ctx *ctx, struct opa_ctx_assign *ctx_assign)
{
	struct hfi_devdata *dd = ctx->devdata;
	struct hfi_ctx_error *error = &ctx->error;
	struct hfi_error_queue *errq = &error->queue;
	unsigned long flags;

	errq->count = HFI_ERROR_DISABLE;
	if (ctx->type == HFI_CTX_TYPE_KERNEL ||
	    !(ctx_assign->flags & HFI_CTX_FLAG_ASYNC_ERROR))
		return;

	/* initialize queue struct */
	init_waitqueue_head(&errq->waitq);
	INIT_LIST_HEAD(&errq->head);
	spin_lock_init(&errq->lock);
	errq->count = 0; /* enable and no error on queue */

	/* put into device error dispatch queue */
	INIT_LIST_HEAD(&error->list);
	spin_lock_irqsave(&dd->error_dispatch_lock, flags);
	list_add_tail(&error->list, &dd->error_dispatch_head);
	spin_unlock_irqrestore(&dd->error_dispatch_lock, flags);
}

/*
 * The context is done, remove the context's error queue
 * from device's error listening list. Also free all
 * the remianing error entry in the error queue.
 */
void hfi_errq_cleanup(struct hfi_ctx *ctx)
{
	struct hfi_devdata *dd = ctx->devdata;
	struct hfi_ctx_error *error = &ctx->error;
	struct hfi_error_queue *errq = &error->queue;
	unsigned long flags;
	struct hfi_error_entry *entry, *tmp;

	/* queue is disabled */
	if (errq->count == HFI_ERROR_DISABLE)
		return;

	/* remove from device error queue list */
	spin_lock_irqsave(&dd->error_dispatch_lock, flags);
	list_del(&error->list);
	spin_unlock_irqrestore(&dd->error_dispatch_lock, flags);

	spin_lock_irqsave(&errq->lock, flags);
	/* disable the error queue */
	errq->count = HFI_ERROR_DISABLE;
	/* free the remaining error events */
	list_for_each_entry_safe(entry, tmp, &errq->head, list)
		kfree(entry);
	spin_unlock_irqrestore(&errq->lock, flags);

	/* wake up the waiting threads */
	wake_up_interruptible_all(&errq->waitq);
}

int hfi_get_async_error(struct hfi_ctx *ctx, void *ae, int timeout)
{
	struct hfi_error_queue *errq = &ctx->error.queue;
	struct hfi_error_entry *entry;
	unsigned long flags;
	long jiffies, ret;	/* has to be same type */

	if (timeout < 0)
		jiffies = MAX_SCHEDULE_TIMEOUT;
	else
		jiffies = msecs_to_jiffies(timeout);

	spin_lock_irqsave(&errq->lock, flags);

	/* check if no error event, wait as necessary */
	if (!errq->count) {
		spin_unlock_irqrestore(&errq->lock, flags);

		ret = wait_event_interruptible_timeout(errq->waitq,
						       errq->count,
						       jiffies);
		if (ret < 0) /* interrupted */
			return -ERESTARTSYS;
		else if (ret == 0) /* timeout */
			return -EAGAIN;

		spin_lock_irqsave(&errq->lock, flags);
	}

	/* check if terminating condition */
	if (errq->count == HFI_ERROR_DISABLE) {
		spin_unlock_irqrestore(&errq->lock, flags);
		return -EPERM;
	}

	/* pick the first entry from the queue */
	entry = list_entry(errq->head.next,
			   struct hfi_error_entry, list);
	list_del(&entry->list);
	errq->count--;
	spin_unlock_irqrestore(&errq->lock, flags);

	memcpy(ae, &entry->ae, sizeof(entry->ae));
	/* free memory */
	kfree(entry);
	return 0;
}
