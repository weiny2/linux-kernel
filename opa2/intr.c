/*
 * Copyright (c) 2013 - 2014 Intel Corporation.  All rights reserved.
 * Copyright (c) 2008 - 2012 QLogic Corporation. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <linux/interrupt.h>
#include "opa_hfi.h"

void cleanup_interrupts(struct hfi_devdata *dd)
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

		kfree(dd->msix_entries);
	}
	dd->msix_entries = NULL;
	dd->num_msix_entries = 0;
}

int setup_interrupts(struct hfi_devdata *dd, int total, int minw)
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

		/* TODO: Consider handling reduced interrupt case? */
		dd_dev_err(dd,
			   "cannot handle reduced interrupt case, %u < %u\n",
			   request, total);
		ret = -ENOSPC;
		goto fail;
	} else {
		/* using MSI-X */
		dd->num_msix_entries = request;
		dd->msix_entries = entries;
		dd_dev_info(dd, "%u MSI-X interrupts allocated\n", total);
	}

	return 0;
fail:
	cleanup_interrupts(dd);
	return ret;
}
