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
#include "opa_hfi.h"
#include <rdma/fxr/fxr_fc_defs.h>
#include <rdma/fxr/fxr_hifis_defs.h>
#include <rdma/fxr/fxr_loca_defs.h>
#include <rdma/fxr/fxr_pcim_defs.h>
#include <rdma/fxr/fxr_tx_otr_pkt_top_csrs_defs.h>
#include <rdma/fxr/fxr_tx_otr_msg_top_csrs_defs.h>
#include <rdma/fxr/fxr_tx_dma_defs.h>
#include <rdma/fxr/fxr_rx_e2e_defs.h>
#include <rdma/fxr/fxr_rx_hp_defs.h>
#include <rdma/fxr/fxr_rx_dma_defs.h>
#include <rdma/fxr/fxr_rx_et_defs.h>
#include <rdma/fxr/fxr_rx_hiarb_defs.h>
#include <rdma/fxr/fxr_at_defs.h>
#include <rdma/fxr/mnh_opio_defs.h>
#include <rdma/fxr/fxr_perfmon_defs.h>

/*
 * error domain and error event processing structure.
 */
struct hfi_error_event {
	char *event_name;
	char *event_desc;
	/* we can add action routine here */
	void (*action)(struct hfi_devdata *dd);
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

/*
 * Each error csr definition.
 */
#include "errdef.h"

/*
 * Interrupts assignment for each error domains.
 */
#define FZC0_IRQ		257
#define FZC1_IRQ		258
#define HIFIS_IRQ		259
#define LOCA_IRQ		260
#define PCIM_IRQ		261
#define TXCID_IRQ		262
#define OTR_IRQ			263
#define TXDMA_IRQ		264
#define LM_IRQ			265
#define RXE2E_IRQ		266
#define RXHP_IRQ		267
#define RXDMA_IRQ		268
#define RXET_IRQ		269
#define RXHIARB_IRQ		270
#define AT_IRQ			271
#define OPIO_IRQ		272
#define RXCID_IRQ		273
#define FPC0_IRQ		274
#define FPC1_IRQ		275
#define TP0_IRQ			276
#define TP1_IRQ			277
#define PMON_IRQ		278

/*
 * When LM_ERR is added, this will be 22
 */
#define NUM_ERR_DOMAIN		21

/*
 * Put all error domains into an array structure
 */
#define MAKE_DOMAIN(e, irq)	\
	{e, sizeof(e) / sizeof(struct hfi_error_csr), irq }

static struct hfi_error_domain hfi_error_domain[] = {
	MAKE_DOMAIN(hfi_fzc0_error, FZC0_IRQ),
	MAKE_DOMAIN(hfi_fzc1_error, FZC1_IRQ),
	MAKE_DOMAIN(hfi_hifis_error, HIFIS_IRQ),
	MAKE_DOMAIN(hfi_loca_error, LOCA_IRQ),
	MAKE_DOMAIN(hfi_pcim_error, PCIM_IRQ),
	MAKE_DOMAIN(hfi_txcid_error, TXCID_IRQ),
	MAKE_DOMAIN(hfi_otr_error, OTR_IRQ),
	MAKE_DOMAIN(hfi_txdma_error, TXDMA_IRQ),
	/* add LM error here */
	MAKE_DOMAIN(hfi_rxe2e_error, RXE2E_IRQ),
	MAKE_DOMAIN(hfi_rxhp_error, RXHP_IRQ),
	MAKE_DOMAIN(hfi_rxdma_error, RXDMA_IRQ),
	MAKE_DOMAIN(hfi_rxet_error, RXET_IRQ),
	MAKE_DOMAIN(hfi_rxhiarb_error, RXHIARB_IRQ),
	MAKE_DOMAIN(hfi_at_error, AT_IRQ),
	MAKE_DOMAIN(hfi_opio_error, OPIO_IRQ),
	MAKE_DOMAIN(hfi_rxcid_error, RXCID_IRQ),
	MAKE_DOMAIN(hfi_fpc0_error, FPC0_IRQ),
	MAKE_DOMAIN(hfi_fpc1_error, FPC1_IRQ),
	MAKE_DOMAIN(hfi_tp0_error, TP0_IRQ),
	MAKE_DOMAIN(hfi_tp1_error, TP1_IRQ),
	MAKE_DOMAIN(hfi_pmon_error, PMON_IRQ)
};

/*
 * real error interrupt handler.
 */
static irqreturn_t irq_err_handler(int irq, void *dev_id)
{
	struct hfi_msix_entry *me = dev_id;
	struct hfi_devdata *dd = me->dd;
	struct hfi_error_domain *domain = me->arg;
	struct hfi_error_csr *csr = domain->csr;
	struct hfi_error_event *event;
	int count = domain->count;
	int i, j;
	u64 val;

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
					event->action(dd);
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
 * configure and setup driver to receive error interrupt.
 */
int hfi_setup_irqerr(struct hfi_devdata *dd)
{
	struct hfi_msix_entry *me;
	struct hfi_error_csr *csr;
	int count, irq;
	int i, j, ret;
	u64 val;

	/* set all bits */
	val = ~0;
	BUILD_BUG_ON(sizeof(hfi_error_domain) !=
		(NUM_ERR_DOMAIN * sizeof(struct hfi_error_domain)));

	for (i = 0; i < NUM_ERR_DOMAIN; i++) {
		irq = hfi_error_domain[i].irq;
		me = &dd->msix_entries[irq];

		BUG_ON(me->arg != NULL);
		me->dd = dd;
		me->intr_src = irq;

		dd_dev_info(dd, "request for IRQ %d:%d\n", irq, me->msix.vector);
		ret = request_irq(me->msix.vector, irq_err_handler, 0,
				"hfi_irq_error", me);
		if (ret) {
			dd_dev_err(dd, "IRQ[%d] request failed %d\n", irq, ret);
			return ret;
		}

		/* mark as in use */
		me->arg = &hfi_error_domain[i];

		/* enable interrupt from domains */
		count = hfi_error_domain[i].count;
		csr = hfi_error_domain[i].csr;
		for (j = 0; j < count; j++)
			write_csr(dd, csr[j].err_en_host, val);
	}

	return 0;
}

