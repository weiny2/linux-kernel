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
#include "hfi2.h"
#include "link.h"
#include "chip/fxr_oc_defs.h"
#include "chip/fxr_hifis_defs.h"
#include "chip/fxr_loca_defs.h"
#include "chip/fxr_pcim_defs.h"
#include "chip/fxr_tx_otr_pkt_top_csrs_defs.h"
#include "chip/fxr_tx_otr_msg_top_csrs_defs.h"
#include "chip/fxr_tx_dma_defs.h"
#include "chip/fxr_tx_ci_cic_csrs_defs.h"
#include "chip/fxr_tx_ci_cid_csrs_defs.h"
#include "chip/fxr_rx_ci_cic_csrs_defs.h"
#include "chip/fxr_rx_ci_cid_csrs_defs.h"
#include "chip/fxr_rx_e2e_defs.h"
#include "chip/fxr_rx_hp_defs.h"
#include "chip/fxr_rx_dma_defs.h"
#include "chip/fxr_rx_et_defs.h"
#include "chip/fxr_rx_hiarb_defs.h"
#include "chip/fxr_at_defs.h"
#include "chip/fxr_perfmon_defs.h"
#include "chip/fxr_linkmux_defs.h"
#include "chip/fxr_linkmux_tp_defs.h"
#include "chip/fxr_linkmux_fpc_defs.h"
#include "chip/fxr_linkmux_cm_defs.h"
#include "trace.h"

extern bool detect_uncat_errs;

/*
 * error domain and error event processing structure.
 */
struct hfi_error_event {
	char *event_name;
	char *event_desc;
	u8    err_category;
	/* we can add action routine here */
	void (*action)(struct hfi_devdata *dd, u64 err_sts, char *name);
};

struct hfi_error_csr {
	char *csr_name;
	u64 err_en_host;
	u64 err_first_host;
	u64 err_sts;		/* status CSR */
	u64 err_clr;		/* clear CSR */
	u64 err_frc;		/* set CSR */
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

static void hfi_error_handler_hw_bug(struct hfi_devdata *dd, u64 reg,
				     char *name)
{
	dd_dev_err(dd, "%s: %s undefined bit %lld fired, HW BUG\n",
		   __func__, name, reg);
}

static void hfi_error_handler_unimpl(struct hfi_devdata *dd, u64 reg,
				     char *name)
{
	dd_dev_info(dd, "%s: %s bit %lld fired, unimpl\n", __func__,
		    name, reg);
}

static void hfi_error_handler_ok(struct hfi_devdata *dd, u64 reg, char *name)
{
	dd_dev_info(dd, "%s: %s bit %lld fired, nothing to do\n",
		    __func__, name, reg);
}

static void hfi_error_handler_info(struct hfi_devdata *dd, u64 reg, char *name)
{
	dd_dev_info(dd, "%s: %s bit %lld fired, information only\n",
		    __func__, name, reg);
}

static void hfi_error_handler_corr(struct hfi_devdata *dd, u64 reg, char *name)
{
	dd_dev_info(dd, "%s: %s bit %lld fired, corrected, we should not get this?\n",
		    __func__, name, reg);
}

static void hfi_error_handler_trans(struct hfi_devdata *dd, u64 reg, char *name)
{
	dd_dev_info(dd, "%s: %s bit %lld fired, transaction error\n",
		    __func__, name, reg);
}

static void hfi_error_handler_proc(struct hfi_devdata *dd, u64 reg, char *name)
{
	dd_dev_info(dd, "%s: %s bit %lld fired, process error\n",
		    __func__, name, reg);
}

static void hfi_error_handler_hfi(struct hfi_devdata *dd, u64 reg, char *name)
{
	dd_dev_info(dd, "%s: %s bit %lld fired, resetting the HFI\n",
		    __func__, name, reg);
}

static void hfi_error_handler_node(struct hfi_devdata *dd, u64 reg, char *name)
{
	dd_dev_info(dd, "%s: %s bit %lld fired, node error\n",
		    __func__, name, reg);
}

static void hfi_error_handler_def(struct hfi_devdata *dd, u64 reg, char *name)
{
	dd_dev_info(dd, "%s: %s bit %lld fired, bit not categorized\n",
		    __func__, name, reg);
}

/*
 * Each error csr definition.
 */
#include "errdef.h"

/*
 * Interrupts assignment for each error domains.
 */
#define HFI_OTR_PKT_IRQ		258
#define HFI_HIFIS_IRQ		259
#define HFI_LOCA_IRQ		260
#define HFI_PCIM_IRQ		261
#define HFI_TXCID_IRQ		262
#define HFI_TXCIC_IRQ		263
#define HFI_OTR_MSG_IRQ		264
#define HFI_TXDMA_IRQ		265
#define HFI_LM_IRQ		266
#define HFI_RXE2E_IRQ		267
#define HFI_RXHP_IRQ		268
#define HFI_RXDMA_IRQ		269
#define HFI_RXET_IRQ		270
#define HFI_RXHIARB_IRQ		271
#define HFI_AT_IRQ		272
//#define HFI_OPIO_IRQ		273
#define HFI_RXCID_IRQ		274
#define HFI_RXCIC_IRQ		275
#define HFI_FPC_IRQ		276
#define HFI_TP_IRQ		277
#define HFI_PMON_IRQ		278
#define HFI_OC_IRQ		279

/* Number of error domains */
#define HFI_NUM_ERR_DOMAIN      21

/* macros for error bits in FPC_ERR_STS reg */
/* UnCorrectable Error */
#define FPC_UC_ERR	13
/* Link Error */
#define FPC_LINK_ERR	19
/* FmConfig Error */
#define FPC_FMC_ERR	54
/* RcvPort Error */
#define FPC_RP_ERR	55

/* Number of groups in FXR_PMON_CFG_ARRAY */
#define HFI_PMON_NUM_GROUPS	16

/* Number of counters within a group */
#define HFI_PMON_NUM_CNTRS	32

/*
 * Put all error domains into an array structure
 */
#define HFI_MAKE_DOMAIN(e, irq)	\
	{e, sizeof(e) / sizeof(struct hfi_error_csr), irq }

static struct hfi_error_domain hfi_error_domain[] = {
	HFI_MAKE_DOMAIN(hfi_otr_pkt_error, HFI_OTR_PKT_IRQ),
	HFI_MAKE_DOMAIN(hfi_hifis_error, HFI_HIFIS_IRQ),
	HFI_MAKE_DOMAIN(hfi_loca_error, HFI_LOCA_IRQ),
	HFI_MAKE_DOMAIN(hfi_pcim_error, HFI_PCIM_IRQ),
	HFI_MAKE_DOMAIN(hfi_txcid_error, HFI_TXCID_IRQ),
	HFI_MAKE_DOMAIN(hfi_txcic_error, HFI_TXCIC_IRQ),
	HFI_MAKE_DOMAIN(hfi_otr_msg_error, HFI_OTR_MSG_IRQ),
	HFI_MAKE_DOMAIN(hfi_txdma_error, HFI_TXDMA_IRQ),
	HFI_MAKE_DOMAIN(hfi_lm_error, HFI_LM_IRQ),
	HFI_MAKE_DOMAIN(hfi_rxe2e_error, HFI_RXE2E_IRQ),
	HFI_MAKE_DOMAIN(hfi_rxhp_error, HFI_RXHP_IRQ),
	HFI_MAKE_DOMAIN(hfi_rxdma_error, HFI_RXDMA_IRQ),
	HFI_MAKE_DOMAIN(hfi_rxet_error, HFI_RXET_IRQ),
	HFI_MAKE_DOMAIN(hfi_rxhiarb_error, HFI_RXHIARB_IRQ),
	HFI_MAKE_DOMAIN(hfi_at_error, HFI_AT_IRQ),
	HFI_MAKE_DOMAIN(hfi_rxcid_error, HFI_RXCID_IRQ),
	HFI_MAKE_DOMAIN(hfi_rxcic_error, HFI_RXCIC_IRQ),
	HFI_MAKE_DOMAIN(hfi_fpc_error, HFI_FPC_IRQ),
	HFI_MAKE_DOMAIN(hfi_tp_error, HFI_TP_IRQ),
	HFI_MAKE_DOMAIN(hfi_pmon_error, HFI_PMON_IRQ),
	HFI_MAKE_DOMAIN(hfi_oc_error, HFI_OC_IRQ)
};

/*
 * Increment 16-bit software counter(s) by 1
 */
static void hfi_update_pmon_counters(struct hfi_devdata *dd,
				     int offset)
{
	*(dd->scntrs + offset) += 1;
}

/*
 * PMON counter overflow interrupt handler checks for the group
 * within which an overflow occurred and increments software
 * device counters corresponding to the counters which encountered
 * an overflow within the same group
 */
static void hfi_handle_pmon_overflow(struct hfi_devdata *dd,
				     u64 err_sts, char *name)
{
	int i, cntr_idx, base_idx;
	u64 overflow, cntr_addr, counter_value;

	overflow = read_csr(dd, FXR_PMON_ERR_INFO_OVERFLOW);
	cntr_addr = overflow & FXR_PMON_ERR_INFO_OVERFLOW_CNTR_ADDR_SMASK;

	/* obtain the index of the first and
	 * least significant counter to set overflow
	 */
	cntr_idx = (cntr_addr - FXR_PMON_ARRAY_OFFSET) / 8;
	/* obtain the index of the counter within the group */
	base_idx = cntr_idx & (HFI_PMON_NUM_CNTRS - 1);

	for (i = base_idx; i <= HFI_PMON_NUM_CNTRS; i++) {
		counter_value = hfi_read_pmon_csr(dd, cntr_idx);

		if (counter_value & FXR_PMON_CFG_ARRAY_OVERFLOW_SMASK)
			hfi_update_pmon_counters(dd, cntr_idx);

		cntr_idx++;
	}
}

/*
 * IOMMU page group request error, dispatch to the target process
 */
static void hfi_handle_pgrrsp_error(struct hfi_devdata *dd, u64 err_sts,
				    char *name)
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

	if (!dd->emulation) {
		this_cpu_inc(*dd->int_counter);
		trace_hfi2_irq_err(me);
	}

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
					event->action(dd, j, csr->csr_name);
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
 * error domain status reset
 */
void hfi_reset_errd_sts(struct hfi_devdata *dd)
{
	struct hfi_error_csr *csr;
	int i, j, count;
	u64 val;

	for (i = 0; i < HFI_NUM_ERR_DOMAIN; i++) {
		csr = hfi_error_domain[i].csr;
		count = hfi_error_domain[i].count;

		for (j = 0; j < count; j++) {
			val = 0;
			/* disable interrupt */
			write_csr(dd, csr[j].err_en_host, val);
			/* clear first host status */
			write_csr(dd, csr[j].err_first_host, val);

			/* clear error domain status */
			val = ~0;
			write_csr(dd, csr[j].err_clr, val);
		}
	}
}

/*
 * configure and setup driver to receive error domain interrupt.
 */
int hfi_setup_errd_irq(struct hfi_devdata *dd)
{
	struct hfi_irq_entry *me;
	struct hfi_error_csr *csr;
	int count, irq;
	int i, j, k, ret;
	u64 val;
	struct hfi_error_event *ee_ptr = NULL;

	/* reset all error domain statuses first */
	hfi_reset_errd_sts(dd);

	/* Setup interrupt handler for each IRQ */
	for (i = 0; i < HFI_NUM_ERR_DOMAIN; i++) {
		count = hfi_error_domain[i].count;
		for (j = 0; j < count; j++) {
			csr = &hfi_error_domain[i].csr[j];
			for (k = 0; k < 64; k++) {
				ee_ptr = &csr->events[k];

				if (strstr(ee_ptr->event_desc, "Unused"))
					ee_ptr->action =
						hfi_error_handler_hw_bug;
				else
					ee_ptr->action =
						hfi_error_handler_unimpl;

				switch (ee_ptr->err_category) {
				case ERR_CATEGORY_OKAY:
					ee_ptr->action =
						hfi_error_handler_ok;
					break;
				case ERR_CATEGORY_INFO:
					ee_ptr->action =
						hfi_error_handler_info;
					break;
				case ERR_CATEGORY_CORRECTABLE:
					ee_ptr->action =
						hfi_error_handler_corr;
					break;
				case ERR_CATEGORY_TRANSACTION:
					ee_ptr->action =
						hfi_error_handler_trans;
					break;
				case ERR_CATEGORY_PROCESS:
					ee_ptr->action =
						hfi_error_handler_proc;
					break;
				case ERR_CATEGORY_HFI:
					ee_ptr->action =
						hfi_error_handler_hfi;
					break;
				case ERR_CATEGORY_NODE:
					ee_ptr->action =
						hfi_error_handler_node;
					break;
				case ERR_CATEGORY_DEFAULT:
					/*
					 * FXRTODO: Software-only, this is to
					 * make sure all event bits get
					 * assigned into one of the others
					 */
					if (detect_uncat_errs)
						ee_ptr->action =
							hfi_error_handler_def;
					break;
				}
			}
		}
	}

	/*
	 * Set address translation error event handler:
	 * 11: PageGroup Response Error
	 */
	hfi_at_error[0].events[11].action = hfi_handle_pgrrsp_error;

	hfi_fpc_error[0].events[FPC_UC_ERR].action =
				hfi_handle_fpc_uncorrectable_error;
	hfi_fpc_error[0].events[FPC_LINK_ERR].action =
				hfi_handle_fpc_link_error;
	hfi_fpc_error[0].events[FPC_FMC_ERR].action =
				hfi_handle_fpc_fmconfig_error;
	hfi_fpc_error[0].events[FPC_RP_ERR].action =
				hfi_handle_fpc_rcvport_error;

	/* Setup PMON interrupt handler for each group */
	for (i = 1; i <= HFI_PMON_NUM_GROUPS; i++)
		hfi_pmon_error[0].events[i].action =
				hfi_handle_pmon_overflow;

	/* set all bits */
	val = ~0;
	BUILD_BUG_ON(sizeof(hfi_error_domain) !=
		(HFI_NUM_ERR_DOMAIN * sizeof(struct hfi_error_domain)));

	/* Currently don't enable error domains on ZEBU */
	if (dd->emulation)
		return 0;

	for (i = 0; i < HFI_NUM_ERR_DOMAIN; i++) {
		irq = hfi_error_domain[i].irq;
		me = &dd->irq_entries[irq];

		WARN_ON(me->arg);
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
			    irq, pci_irq_vector(dd->pdev, irq));
		ret = request_irq(pci_irq_vector(dd->pdev, irq),
				  hfi_irq_errd_handler, 0,
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
	long jiffies, stat;	/* has to be same type */
	int ret = 0;

	down_read(&ctx->ctx_rwsem);

	/* verify we have an assigned context */
	if (ctx->pid == HFI_PID_NONE) {
		ret = -EPERM;
		goto unlock;
	}

	if (timeout < 0)
		jiffies = MAX_SCHEDULE_TIMEOUT;
	else
		jiffies = msecs_to_jiffies(timeout);

	spin_lock_irqsave(&errq->lock, flags);

	/* check if no error event, wait as necessary */
	if (!errq->count) {
		spin_unlock_irqrestore(&errq->lock, flags);

		stat = wait_event_interruptible_timeout(errq->waitq,
							errq->count,
							jiffies);
		if (stat < 0) { /* interrupted */
			ret = -ERESTARTSYS;
			goto unlock;
		} else if (stat == 0) { /* timeout */
			ret = -ETIME;
			goto unlock;
		}

		spin_lock_irqsave(&errq->lock, flags);
	}

	/* check if terminating condition */
	if (errq->count == HFI_ERROR_DISABLE) {
		spin_unlock_irqrestore(&errq->lock, flags);
		ret = -EPERM;
		goto unlock;
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

unlock:
	up_read(&ctx->ctx_rwsem);
	return ret;
}
