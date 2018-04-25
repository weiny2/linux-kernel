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

void hfi_disable_interrupts(struct hfi_devdata *dd)
{
	int i;

	/* remove irqs - must happen before disabling/turning off */
	if (dd->num_irq_entries) {
		/* MSI-X */
		struct hfi_irq_entry *me = dd->irq_entries;

		/* loop over all IRQs, don't break early */
		for (i = 0; i < dd->num_irq_entries; i++, me++) {
			if (!me->arg) /* => no irq, no affinity */
				continue;
			irq_set_affinity_hint(pci_irq_vector(dd->pdev, i),
					      NULL);
			free_irq(pci_irq_vector(dd->pdev, i), me);
		}

		/* turn off interrupts */
		hfi_disable_msix(dd);
	} else {
		/* INTx */
		free_irq(dd->pdev->irq, dd);
	}
}

void hfi_cleanup_interrupts(struct hfi_devdata *dd)
{
	kfree(dd->irq_entries);
	dd->irq_entries = NULL;
	dd->num_irq_entries = 0;
}

int hfi_setup_interrupts(struct hfi_devdata *dd, int total)
{
	struct hfi_irq_entry *entries;
	u32 request;
	int ret;

	entries = kcalloc(total, sizeof(*entries), GFP_KERNEL);
	if (!entries) {
		ret = -ENOMEM;
		goto fail;
	}

	/* ask for MSI-X interrupts */
	request = total;
	ret = hfi_enable_msix(dd, &request);
	if (ret)
		goto fail;

	/* if request is zero, INTx is used */
	dd->num_irq_entries = request;
	dd->irq_entries = entries;

	/* MSIX-alias mode is not supported yet */
	if (request > 0 && request < total) {
		/* FXRTODO: handle using MSI-X, with reduced interrupts */
		dd_dev_err(dd, "reduced MSI-X interrupts, %u < %u err\n",
			   request, total);
		ret = -ENXIO;
		goto fail;
	}

	return 0;
fail:
	hfi_disable_msix(dd);
	hfi_cleanup_interrupts(dd);
	return ret;
}
