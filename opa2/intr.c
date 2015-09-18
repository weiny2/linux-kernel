/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2014 - 2015 Intel Corporation.
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
 * Copyright (c) 2014 - 2015 Intel Corporation.
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
 * Intel(R) Omni-Path Gen2 HFI PCIe Driver
 */

#include <linux/interrupt.h>
#include "opa_hfi.h"

void hfi_disable_interrupts(struct hfi_devdata *dd)
{
	int i;

	/* remove irqs - must happen before disabling/turning off */
	if (dd->num_msix_entries) {
		/* MSI-X */
		struct hfi_msix_entry *me = dd->msix_entries;

		for (i = 0; i < dd->num_msix_entries; i++, me++) {
			if (me->arg == NULL) /* => no irq, no affinity */
				break;
			irq_set_affinity_hint(dd->msix_entries[i].msix.vector,
					NULL);
			free_irq(me->msix.vector, me->arg);
		}

		/* turn off interrupts */
		hfi_disable_msix(dd);
	}
}

void hfi_cleanup_interrupts(struct hfi_devdata *dd)
{
	kfree(dd->msix_entries);
	dd->msix_entries = NULL;
	dd->num_msix_entries = 0;
}

int hfi_setup_interrupts(struct hfi_devdata *dd, int total, int minw)
{
	struct hfi_msix_entry *entries;
	u32 request;
	int i, ret;

	entries = kcalloc(total, sizeof(*entries), GFP_KERNEL);
	if (!entries) {
		dd_dev_err(dd, "cannot allocate msix table\n");
		ret = -ENOMEM;
		goto fail;
	}
	/* 1-1 MSI-X entry assignment */
	for (i = 0; i < total; i++)
		entries[i].msix.entry = i;

	/* ask for MSI-X interrupts */
	request = total;
	ret = hfi_pcie_params(dd, minw, &request, entries);
	if (ret)
		goto fail;

	if (request == 0) {
		/* INTx not supported */
		/* dd->num_msix_entries already zero */
		kfree(entries);
		ret = -ENXIO;
		goto fail;
	} else if (request != total) {
		/* using MSI-X, with reduced interrupts */
		dd->num_msix_entries = request;
		dd->msix_entries = entries;
		dd_dev_err(dd, "reduced MSI-X interrupts, %u < %u\n",
			   request, total);
	} else {
		/* using MSI-X */
		dd->num_msix_entries = request;
		dd->msix_entries = entries;
		dd_dev_info(dd, "%u MSI-X interrupts allocated\n", total);
	}

	return 0;
fail:
	hfi_disable_interrupts(dd);
	hfi_cleanup_interrupts(dd);
	return ret;
}
