/*
 * Copyright (c) 2014 Intel Corporation.  All rights reserved.
 * Copyright (c) 2008, 2009 QLogic Corporation. All rights reserved.
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

#include <linux/pci.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/module.h>
#include "hfi.h"

/*
 * This file contains PCIe utility routines.
 */

static void hfi_enable_intx(struct pci_dev *pdev);

/*
 * Do all the common PCIe setup and initialization.
 * devdata is not yet allocated, and is not allocated until after this
 * routine returns success.  Therefore dd_dev_err() can't be used for error
 * printing.
 */
int hfi_pci_init(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	int ret;

	ret = pci_enable_device(pdev);
	if (ret) {
		/*
		 * This can happen (in theory) iff:
		 * We did a chip reset, and then failed to reprogram the
		 * BAR, or the chip reset due to an internal error.  We then
		 * unloaded the driver and reloaded it.
		 *
		 * Both reset cases set the BAR back to initial state.  For
		 * the latter case, the AER sticky error bit at offset 0x718
		 * should be set, but the Linux kernel doesn't yet know
		 * about that, it appears.  If the original BAR was retained
		 * in the kernel data structures, this may be OK.
		 */
		dev_err(&pdev->dev, "pci enable failed: error %d\n", -ret);
		goto done;
	}

	ret = pci_request_regions(pdev, DRIVER_NAME);
	if (ret) {
		dev_err(&pdev->dev,
			"pci_request_regions fails: err %d\n", -ret);
		goto err_pci;
	}

	ret = pci_set_dma_mask(pdev, DMA_BIT_MASK(64));
	if (ret) {
		/*
		 * If the 64 bit setup fails, try 32 bit.  Some systems
		 * do not setup 64 bit maps on systems with 2GB or less
		 * memory installed.
		 */
		ret = pci_set_dma_mask(pdev, DMA_BIT_MASK(32));
		if (ret) {
			dev_err(&pdev->dev,
				"Unable to set DMA mask: %d\n", ret);
			goto err_pci;
		}
		ret = pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(32));
	} else
		ret = pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(64));
	if (ret) {
		dev_err(&pdev->dev,
			"Unable to set DMA consistent mask: %d\n", ret);
		goto err_pci;
	}

	pci_set_master(pdev);

	return 0;

err_pci:
	hfi_pci_cleanup(pdev);
done:
	return ret;
}

/*
 * Clean what was done in hfi_pci_init()
 */
void hfi_pci_cleanup(struct pci_dev *pdev)
{
	pci_disable_device(pdev);
	/*
	 * Release regions should be called after the disable. OK to
	 * call if request regions has not been called or failed.
	 */
	pci_release_regions(pdev);
}

static void hfi_msix_setup(struct hfi_devdata *dd, int pos, u32 *msixcnt,
			   struct hfi_msix_entry *hfi_msix_entry)
{
	int ret;
	u32 tabsize = 0;
	u16 msix_flags;
	struct msix_entry *msix_entry;
	int i;

	/* We can't pass hfi_msix_entry array to hfi_msix_setup
	 * so use a dummy msix_entry array and copy the allocated
	 * irq back to the hfi_msix_entry array. */
	msix_entry = kmalloc_array(*msixcnt, sizeof(*msix_entry), GFP_KERNEL);
	if (!msix_entry) {
		ret = -ENOMEM;
		goto do_intx;
	}
	for (i = 0; i < *msixcnt; i++)
		msix_entry[i] = hfi_msix_entry[i].msix;

	pci_read_config_word(dd->pcidev, pos + PCI_MSIX_FLAGS, &msix_flags);
	tabsize = 1 + (msix_flags & PCI_MSIX_FLAGS_QSIZE);
	if (tabsize > *msixcnt)
		tabsize = *msixcnt;
	ret = pci_enable_msix(dd->pcidev, msix_entry, tabsize);
	if (ret > 0) {
		tabsize = ret;
		ret = pci_enable_msix(dd->pcidev, msix_entry, tabsize);
	}
do_intx:
	if (ret) {
		dd_dev_err(dd,
			"pci_enable_msix %d vectors failed: %d, falling back to INTx\n",
			tabsize, ret);
		tabsize = 0;
	}
	for (i = 0; i < tabsize; i++)
		hfi_msix_entry[i].msix = msix_entry[i];
	kfree(msix_entry);
	*msixcnt = tabsize;

	if (ret)
		hfi_enable_intx(dd->pcidev);
}

int hfi_pcie_params(struct hfi_devdata *dd, u32 minw, u32 *nent,
		    struct hfi_msix_entry *entry)
{
	u16 linkstat, speed, width;
	int pos, ret = 0;

	pos = pci_find_capability(dd->pcidev, PCI_CAP_ID_MSIX);
	if (nent && *nent && pos) {
		hfi_msix_setup(dd, pos, nent, entry);
		/* did it, either MSI-X or INTx */
	} else {
		if (!pos)
			dd_dev_err(dd, "Can't find PCI MSI-X capability!\n");
		hfi_enable_intx(dd->pcidev);
		if (nent) *nent = 0;
	}

	if (!pci_is_pcie(dd->pcidev)) {
		/* set up something... */
		dd->lbus_width = 1;
		dd->lbus_speed = 2500; /* Gen1, 2.5GHz */
		if (minw) {
			dd_dev_err(dd, "Can't find PCI Express capability!\n");
			ret = -EINVAL;
		}
		goto err_pcie;
	}

	pcie_capability_read_word(dd->pcidev, PCI_EXP_LNKSTA, &linkstat);
	speed = linkstat & PCI_EXP_LNKSTA_CLS;
	width = (linkstat & PCI_EXP_LNKSTA_NLW) >> PCI_EXP_LNKSTA_NLW_SHIFT;
	dd->lbus_width = width;

	switch (speed) {
	default: /* not defined, assume Gen1 */
	case PCI_EXP_LNKSTA_CLS_2_5GB:
		dd->lbus_speed = 2500; /* Gen 1, 2.5GHz */
		break;
	case PCI_EXP_LNKSTA_CLS_5_0GB:
		dd->lbus_speed = 5000; /* Gen 2, 5GHz */
		break;
	case 0x4: /* not defined in a kernel header */
		dd->lbus_speed = 8000; /* Gen 3, 8GHz */
		break;
	}

	/*
	 * Check against expected pcie width and complain if "wrong"
	 * on first initialization, not afterwards (i.e., reset).
	 */
	if (minw && width < minw)
		dd_dev_err(dd,
			    "PCIe width %u (x%u HCA), performance reduced\n",
			    width, minw);

err_pcie:
	return ret;
}

/*
 * Disable MSI-X.
 */
void hfi_disable_msix(struct hfi_devdata *dd)
{
	pci_disable_msix(dd->pcidev);
}

static void hfi_enable_intx(struct pci_dev *pdev)
{
	/* first, turn on INTx */
	pci_intx(pdev, 1);
	/* then turn off MSI-X */
	pci_disable_msix(pdev);
}
