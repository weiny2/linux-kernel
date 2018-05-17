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

#include <linux/pci.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/module.h>
#include "hfi2.h"

/*
 * This file contains PCIe utility routines.
 */

/*
 * Disable MSI-X.
 */
void hfi_disable_msix(struct hfi_devdata *dd)
{
	pci_disable_msix(dd->pdev);
}

static void hfi_enable_intx(struct pci_dev *pdev)
{
	/* first, turn on INTx */
	pci_intx(pdev, 1);
	/* then turn off MSI-X */
	pci_disable_msix(pdev);
}

/*
 * From here through hfi_pci_err_handler definition is invoked via
 * PCI error infrastructure, registered via pci
 */
static pci_ers_result_t hfi_pci_error_detected(struct pci_dev *pdev,
					       pci_channel_state_t state)
{
	return PCI_ERS_RESULT_RECOVERED;
}

static pci_ers_result_t hfi_pci_mmio_enabled(struct pci_dev *pdev)
{
	return PCI_ERS_RESULT_RECOVERED;
}

static pci_ers_result_t hfi_pci_slot_reset(struct pci_dev *pdev)
{
	struct hfi_devdata *dd = pci_get_drvdata(pdev);

	dd_dev_info(dd, "HFI slot_reset function called, ignored\n");
	return PCI_ERS_RESULT_CAN_RECOVER;
}

static void hfi_pci_resume(struct pci_dev *pdev)
{
	struct hfi_devdata *dd = pci_get_drvdata(pdev);

	dd_dev_info(dd, "HFI resume function called\n");
}

const struct pci_error_handlers hfi_pci_err_handler = {
	.error_detected = hfi_pci_error_detected,
	.mmio_enabled = hfi_pci_mmio_enabled,
	.slot_reset = hfi_pci_slot_reset,
	.resume = hfi_pci_resume,
};

/*
 * Do all the common PCIe setup and initialization.
 * devdata is not yet allocated, and is not allocated until after this
 * routine returns success.  Therefore dd_dev_err() can't be used for error
 * printing.
 */
int hfi_pci_init(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	unsigned int mgaw;
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

	/*
	 * Only request HFI_FXR_BAR region.
	 */
	ret = pci_request_region(pdev, HFI_FXR_BAR, DRIVER_NAME);
	if (ret) {
		dev_err(&pdev->dev,
			"pci_request_region fails: err %d\n", -ret);
		goto err_pci;
	}

	mgaw = cpu_feature_enabled(X86_FEATURE_LA57) ? 52 : 46;
	ret = pci_set_dma_mask(pdev, DMA_BIT_MASK(mgaw));
	if (ret) {
		/*
		 * If the mgaw bit setup fails, try 32 bit.  Some systems
		 * do not setup >32 bit maps on systems with 2GB or less
		 * memory installed.
		 */
		ret = pci_set_dma_mask(pdev, DMA_BIT_MASK(32));
		if (ret) {
			dev_err(&pdev->dev,
				"Unable to set DMA mask: %d\n", ret);
			goto err_pci;
		}
		ret = pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(32));
	} else {
		ret = pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(mgaw));
	}
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
	pci_release_region(pdev, HFI_FXR_BAR);
}

int hfi_enable_msix(struct hfi_devdata *dd, u32 *nent)
{
	u16 linkstat, speed, width;
	int ret, tabsize;

	ret = pci_msix_vec_count(dd->pdev);

	if (!ret) {
		tabsize = 0;
	} else if (ret > 0) {
		tabsize = ret;
		if (tabsize > *nent)
			tabsize = *nent;

		ret = pci_alloc_irq_vectors(dd->pdev, 1, tabsize,
					    PCI_IRQ_MSIX | PCI_IRQ_LEGACY);
	}

	if (ret <= 0) {
		dd_dev_err(dd, "Can't enable PCI MSI-X capability!\n");
		hfi_enable_intx(dd->pdev);
		tabsize = 0;
	} else if (ret < tabsize) {
		tabsize = ret;
	}

	/* return vectors configured */
	*nent = tabsize;

	ret = pcie_capability_read_word(dd->pdev, PCI_EXP_LNKSTA, &linkstat);
	if (ret)
		goto read_error;

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
	case PCI_EXP_LNKSTA_CLS_8_0GB:
		dd->lbus_speed = 8000; /* Gen 3, 8GHz */
		break;
	}

	return 0;
read_error:
	dd_dev_err(dd, "Unable to read from PCI config\n");
	return ret;
}
