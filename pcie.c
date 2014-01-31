/*
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
#include <linux/vmalloc.h>
#include <linux/aer.h>
#include <linux/module.h>

#include "hfi.h"

/*
 * This file contains PCIe utility routines.
 */

/*
 * Code to adjust PCIe capabilities.
 */
static int qib_tune_pcie_caps(struct hfi_devdata *);
static int qib_tune_pcie_coalesce(struct hfi_devdata *);

/*
 * Do all the common PCIe setup and initialization.
 * devdata is not yet allocated, and is not allocated until after this
 * routine returns success.  Therefore dd_dev_err() can't be used for error
 * printing.
 */
int qib_pcie_init(struct pci_dev *pdev, const struct pci_device_id *ent)
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
		qib_early_err(&pdev->dev, "pci enable failed: error %d\n",
			      -ret);
		goto done;
	}

	ret = pci_request_regions(pdev, DRIVER_NAME);
	if (ret) {
		qib_early_err(&pdev->dev,
			"pci_request_regions fails: err %d\n", -ret);
		goto bail;
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
			qib_early_err(&pdev->dev,
				"Unable to set DMA mask: %d\n", ret);
			goto bail;
		}
		ret = pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(32));
	} else
		ret = pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(64));
	if (ret) {
		qib_early_err(&pdev->dev,
			      "Unable to set DMA consistent mask: %d\n", ret);
		goto bail;
	}

	pci_set_master(pdev);
	ret = pci_enable_pcie_error_reporting(pdev);
	if (ret) {
		qib_early_err(&pdev->dev,
			      "Unable to enable pcie error reporting: %d\n",
			      ret);
		ret = 0;
	}
	goto done;

bail:
	pci_disable_device(pdev);
	pci_release_regions(pdev);
done:
	return ret;
}

/*
 * Do remaining PCIe setup, once dd is allocated, and save away
 * fields required to re-initialize after a chip reset, or for
 * various other purposes
 */
int qib_pcie_ddinit(struct hfi_devdata *dd, struct pci_dev *pdev,
		    const struct pci_device_id *ent)
{
	unsigned long len;
	resource_size_t addr;

	dd->pcidev = pdev;
	pci_set_drvdata(pdev, dd);

	addr = pci_resource_start(pdev, 0);
	len = pci_resource_len(pdev, 0);

	/*
	 * The TXE PIO buffers are at the tail end of the chip space.
	 * Cut them off and map them separately.
	 */

	/* sanity check vs expectations */
	if (len != WFR_TXE_PIO_SEND + WFR_TXE_PIO_SIZE) {
		dd_dev_err(dd, "chip PIO range does not match\n");
		return -EINVAL;
	}

#if defined(__powerpc__)
	/* There isn't a generic way to specify writethrough mappings */
	dd->kregbase = __ioremap(addr, WFR_TXE_PIO_SEND,
					_PAGE_NO_CACHE | _PAGE_WRITETHRU);
#else
	dd->kregbase = ioremap_nocache(addr, WFR_TXE_PIO_SEND);
#endif

	if (!dd->kregbase)
		return -ENOMEM;

	dd->piobase = ioremap_wc(addr + WFR_TXE_PIO_SEND, WFR_TXE_PIO_SIZE);
	if (!dd->piobase) {
		iounmap(dd->kregbase);
		return -ENOMEM;
	}

	dd->flags |= QIB_PRESENT;	/* now register routines work */

	dd->kregend = dd->kregbase + WFR_TXE_PIO_SEND;
	dd->physaddr = addr;        /* used for io_remap, etc. */

	/*
	 * Save BARs to rewrite after device reset.
	 */
	dd->pcibar0 = addr;
	dd->pcibar1 = addr >> 32;
	dd->deviceid = ent->device; /* save for later use */
	dd->vendorid = ent->vendor;

	return 0;
}

/*
 * Do PCIe cleanup, after chip-specific cleanup, etc.  Just prior
 * to releasing the dd memory.
 * Void because all of the core pcie cleanup functions are void.
 */
void qib_pcie_ddcleanup(struct hfi_devdata *dd)
{
	u64 __iomem *base = (void __iomem *) dd->kregbase;

	dd->flags &= ~QIB_PRESENT;
	dd->kregbase = NULL;
	iounmap(base);
	if (dd->piobase)
		iounmap(dd->piobase);

	pci_disable_device(dd->pcidev);
	pci_release_regions(dd->pcidev);

	pci_set_drvdata(dd->pcidev, NULL);
}

/*
 * Do a Function Level Reset (FLR) on the device.
 * Based on static function drivers/pci/pci.c:pcie_flr().
 */
void hfi_pcie_flr(struct hfi_devdata *dd)
{
	int i;
	u16 status;

	/* no need to check for the capability - we know the device has it */

	/* wait for Transaction Pending bit to clear, at most a few ms */
	for (i = 0; i < 4; i++) {
		if (i)
			msleep((1 << (i - 1)) * 100);

		pcie_capability_read_word(dd->pcidev, PCI_EXP_DEVSTA, &status);
		if (!(status & PCI_EXP_DEVSTA_TRPND))
			goto clear;
	}

	dd_dev_err(dd, "Transaction Pending bit is not clearing, proceedingreset anyway\n");

clear:
	pcie_capability_set_word(dd->pcidev, PCI_EXP_DEVCTL,
						PCI_EXP_DEVCTL_BCR_FLR);
	/* PCIe spec requires the function to be back within 100ms */
	msleep(100);
}

static void qib_msix_setup(struct hfi_devdata *dd, int pos, u32 *msixcnt,
			   struct qib_msix_entry *qib_msix_entry)
{
	int ret;
	u32 tabsize = 0;
	u16 msix_flags;
	struct msix_entry *msix_entry;
	int i;

	/* We can't pass qib_msix_entry array to qib_msix_setup
	 * so use a dummy msix_entry array and copy the allocated
	 * irq back to the qib_msix_entry array. */
	msix_entry = kmalloc(*msixcnt * sizeof(*msix_entry), GFP_KERNEL);
	if (!msix_entry) {
		ret = -ENOMEM;
		goto do_intx;
	}
	for (i = 0; i < *msixcnt; i++)
		msix_entry[i] = qib_msix_entry[i].msix;

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
		qib_msix_entry[i].msix = msix_entry[i];
	kfree(msix_entry);
	*msixcnt = tabsize;

	if (ret)
		qib_enable_intx(dd->pcidev);

}

int qib_pcie_params(struct hfi_devdata *dd, u32 minw, u32 *nent,
		    struct qib_msix_entry *entry)
{
	u16 linkstat, speed;
	int pos, ret = 0;

	if (!pci_is_pcie(dd->pcidev)) {
		dd_dev_err(dd, "Can't find PCI Express capability!\n");
		/* set up something... */
		dd->lbus_width = 1;
		dd->lbus_speed = 2500; /* Gen1, 2.5GHz */
		ret = -EINVAL;
		goto bail;
	}

	pos = pci_find_capability(dd->pcidev, PCI_CAP_ID_MSIX);
	if (nent && *nent && pos) {
		qib_msix_setup(dd, pos, nent, entry);
		/* did it, either MSI-X or INTx */
	} else {
		qib_enable_intx(dd->pcidev);
	}

	pcie_capability_read_word(dd->pcidev, PCI_EXP_LNKSTA, &linkstat);
	speed = linkstat & PCI_EXP_LNKSTA_CLS;
	dd->lbus_width = (linkstat & PCI_EXP_LNKSTA_NLW) >> PCI_EXP_LNKSTA_NLW_SHIFT;

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
	if (minw && linkstat < minw)
		dd_dev_err(dd,
			    "PCIe width %u (x%u HCA), performance reduced\n",
			    linkstat, minw);

	qib_tune_pcie_caps(dd);

	qib_tune_pcie_coalesce(dd);

bail:
	/* fill in string, even on errors */
	snprintf(dd->lbus_info, sizeof(dd->lbus_info),
		 "PCIe,%uMHz,x%u\n", dd->lbus_speed, dd->lbus_width);
	return ret;
}

/*
 * Disable MSI-X.
 */
void qib_nomsix(struct hfi_devdata *dd)
{
	pci_disable_msix(dd->pcidev);
}

void qib_enable_intx(struct pci_dev *pdev)
{
	/* first, turn on INTx */
 	pci_intx(pdev, 1);
	/* then turn off MSI-X */
	pci_disable_msix(pdev);
}

/*
 * These two routines are helper routines for the device reset code
 * to move all the pcie code out of the chip-specific driver code.
 */
void qib_pcie_getcmd(struct hfi_devdata *dd, u16 *cmd, u8 *iline, u8 *cline)
{
	pci_read_config_word(dd->pcidev, PCI_COMMAND, cmd);
	pci_read_config_byte(dd->pcidev, PCI_INTERRUPT_LINE, iline);
	pci_read_config_byte(dd->pcidev, PCI_CACHE_LINE_SIZE, cline);
}

void qib_pcie_reenable(struct hfi_devdata *dd, u16 cmd, u8 iline, u8 cline)
{
	int r;
	r = pci_write_config_dword(dd->pcidev, PCI_BASE_ADDRESS_0,
				   dd->pcibar0);
	if (r)
		dd_dev_err(dd, "rewrite of BAR0 failed: %d\n", r);
	r = pci_write_config_dword(dd->pcidev, PCI_BASE_ADDRESS_1,
				   dd->pcibar1);
	if (r)
		dd_dev_err(dd, "rewrite of BAR1 failed: %d\n", r);
	/* now re-enable memory access, and restore cosmetic settings */
	pci_write_config_word(dd->pcidev, PCI_COMMAND, cmd);
	pci_write_config_byte(dd->pcidev, PCI_INTERRUPT_LINE, iline);
	pci_write_config_byte(dd->pcidev, PCI_CACHE_LINE_SIZE, cline);
	r = pci_enable_device(dd->pcidev);
	if (r)
		dd_dev_err(dd,
			"pci_enable_device failed after reset: %d\n", r);
}

/* code to adjust PCIe capabilities. */

static int fld2val(int wd, int mask)
{
	int lsbmask;

	if (!mask)
		return 0;
	wd &= mask;
	lsbmask = mask ^ (mask & (mask - 1));
	wd /= lsbmask;
	return wd;
}

static int val2fld(int wd, int mask)
{
	int lsbmask;

	if (!mask)
		return 0;
	lsbmask = mask ^ (mask & (mask - 1));
	wd *= lsbmask;
	return wd;
}

static int qib_pcie_coalesce;
module_param_named(pcie_coalesce, qib_pcie_coalesce, int, S_IRUGO);
MODULE_PARM_DESC(pcie_coalesce, "tune PCIe colescing on some Intel chipsets");

/*
 * Enable PCIe completion and data coalescing, on Intel 5x00 and 7300
 * chipsets.   This is known to be unsafe for some revisions of some
 * of these chipsets, with some BIOS settings, and enabling it on those
 * systems may result in the system crashing, and/or data corruption.
 */
static int qib_tune_pcie_coalesce(struct hfi_devdata *dd)
{
	int r;
	struct pci_dev *parent;
	u16 devid;
	u32 mask, bits, val;

	if (!qib_pcie_coalesce)
		return 0;

	/* Find out supported and configured values for parent (root) */
	parent = dd->pcidev->bus->self;
	if (parent->bus->parent) {
		dd_dev_info(dd, "Parent not root\n");
		return 1;
	}
	if (!pci_is_pcie(parent))
		return 1;
	if (parent->vendor != 0x8086)
		return 1;

	/*
	 *  - bit 12: Max_rdcmp_Imt_EN: need to set to 1
	 *  - bit 11: COALESCE_FORCE: need to set to 0
	 *  - bit 10: COALESCE_EN: need to set to 1
	 *  (but limitations on some on some chipsets)
	 *
	 *  On the Intel 5000, 5100, and 7300 chipsets, there is
	 *  also: - bit 25:24: COALESCE_MODE, need to set to 0
	 */
	devid = parent->device;
	if (devid >= 0x25e2 && devid <= 0x25fa) {
		/* 5000 P/V/X/Z */
		if (parent->revision <= 0xb2)
			bits = 1U << 10;
		else
			bits = 7U << 10;
		mask = (3U << 24) | (7U << 10);
	} else if (devid >= 0x65e2 && devid <= 0x65fa) {
		/* 5100 */
		bits = 1U << 10;
		mask = (3U << 24) | (7U << 10);
	} else if (devid >= 0x4021 && devid <= 0x402e) {
		/* 5400 */
		bits = 7U << 10;
		mask = 7U << 10;
	} else if (devid >= 0x3604 && devid <= 0x360a) {
		/* 7300 */
		bits = 7U << 10;
		mask = (3U << 24) | (7U << 10);
	} else {
		/* not one of the chipsets that we know about */
		return 1;
	}
	pci_read_config_dword(parent, 0x48, &val);
	val &= ~mask;
	val |= bits;
	r = pci_write_config_dword(parent, 0x48, val);
	return 0;
}

/*
 * BIOS may not set PCIe bus-utilization parameters for best performance.
 * Check and optionally adjust them to maximize our throughput.
 */
static int qib_pcie_caps;
module_param_named(pcie_caps, qib_pcie_caps, int, S_IRUGO);
MODULE_PARM_DESC(pcie_caps, "Max PCIe tuning: Payload (0..3), ReadReq (4..7)");

static int qib_tune_pcie_caps(struct hfi_devdata *dd)
{
	int ret = 1; /* Assume the worst */
	struct pci_dev *parent;
	u16 pcaps, pctl, ecaps, ectl;
	int rc_sup, ep_sup;
	int rc_cur, ep_cur;

	/* Find out supported and configured values for parent (root) */
	parent = dd->pcidev->bus->self;
	if (parent->bus->parent) {
		dd_dev_info(dd, "Parent not root\n");
		goto bail;
	}

	if (!pci_is_pcie(parent) || !pci_is_pcie(dd->pcidev))
		goto bail;
	pcie_capability_read_word(parent, PCI_EXP_DEVCAP, &pcaps);
	pcie_capability_read_word(parent, PCI_EXP_DEVCTL, &pctl);
	/* Find out supported and configured values for endpoint (us) */
	pcie_capability_read_word(dd->pcidev, PCI_EXP_DEVCAP, &ecaps);
	pcie_capability_read_word(dd->pcidev, PCI_EXP_DEVCTL, &ectl);

	ret = 0;
	/* Find max payload supported by root, endpoint */
	rc_sup = fld2val(pcaps, PCI_EXP_DEVCAP_PAYLOAD);
	ep_sup = fld2val(ecaps, PCI_EXP_DEVCAP_PAYLOAD);
	if (rc_sup > ep_sup)
		rc_sup = ep_sup;

	rc_cur = fld2val(pctl, PCI_EXP_DEVCTL_PAYLOAD);
	ep_cur = fld2val(ectl, PCI_EXP_DEVCTL_PAYLOAD);

	/* If Supported greater than limit in module param, limit it */
	if (rc_sup > (qib_pcie_caps & 7))
		rc_sup = qib_pcie_caps & 7;
	/* If less than (allowed, supported), bump root payload */
	if (rc_sup > rc_cur) {
		rc_cur = rc_sup;
		pctl = (pctl & ~PCI_EXP_DEVCTL_PAYLOAD) |
			val2fld(rc_cur, PCI_EXP_DEVCTL_PAYLOAD);
		pcie_capability_write_word(parent, PCI_EXP_DEVCTL, pctl);
	}
	/* If less than (allowed, supported), bump endpoint payload */
	if (rc_sup > ep_cur) {
		ep_cur = rc_sup;
		ectl = (ectl & ~PCI_EXP_DEVCTL_PAYLOAD) |
			val2fld(ep_cur, PCI_EXP_DEVCTL_PAYLOAD);
		pcie_capability_write_word(dd->pcidev, PCI_EXP_DEVCTL, ectl);
	}

	/*
	 * Now the Read Request size.
	 * No field for max supported, but PCIe spec limits it to 4096,
	 * which is code '5' (log2(4096) - 7)
	 */
	rc_sup = 5;
	if (rc_sup > ((qib_pcie_caps >> 4) & 7))
		rc_sup = (qib_pcie_caps >> 4) & 7;
	rc_cur = fld2val(pctl, PCI_EXP_DEVCTL_READRQ);
	ep_cur = fld2val(ectl, PCI_EXP_DEVCTL_READRQ);

	if (rc_sup > rc_cur) {
		rc_cur = rc_sup;
		pctl = (pctl & ~PCI_EXP_DEVCTL_READRQ) |
			val2fld(rc_cur, PCI_EXP_DEVCTL_READRQ);
		pcie_capability_write_word(parent, PCI_EXP_DEVCTL, pctl);
	}
	if (rc_sup > ep_cur) {
		ep_cur = rc_sup;
		ectl = (ectl & ~PCI_EXP_DEVCTL_READRQ) |
			val2fld(ep_cur, PCI_EXP_DEVCTL_READRQ);
		pcie_capability_write_word(dd->pcidev, PCI_EXP_DEVCTL, ectl);
	}
bail:
	return ret;
}
/* End of PCIe capability tuning */

/*
 * From here through qib_pci_err_handler definition is invoked via
 * PCI error infrastructure, registered via pci
 */
static pci_ers_result_t
qib_pci_error_detected(struct pci_dev *pdev, pci_channel_state_t state)
{
	struct hfi_devdata *dd = pci_get_drvdata(pdev);
	pci_ers_result_t ret = PCI_ERS_RESULT_RECOVERED;

	switch (state) {
	case pci_channel_io_normal:
		dd_dev_info(dd, "State Normal, ignoring\n");
		break;

	case pci_channel_io_frozen:
		dd_dev_info(dd, "State Frozen, requesting reset\n");
		pci_disable_device(pdev);
		ret = PCI_ERS_RESULT_NEED_RESET;
		break;

	case pci_channel_io_perm_failure:
		dd_dev_info(dd, "State Permanent Failure, disabling\n");
		if (dd) {
			/* no more register accesses! */
			dd->flags &= ~QIB_PRESENT;
			qib_disable_after_error(dd);
		}
		 /* else early, or other problem */
		ret =  PCI_ERS_RESULT_DISCONNECT;
		break;

	default: /* shouldn't happen */
		dd_dev_info(dd, "QIB PCI errors detected (state %d)\n",
			state);
		break;
	}
	return ret;
}

static pci_ers_result_t
qib_pci_mmio_enabled(struct pci_dev *pdev)
{
	u64 words = 0U;
	struct hfi_devdata *dd = pci_get_drvdata(pdev);
	pci_ers_result_t ret = PCI_ERS_RESULT_RECOVERED;

	if (dd && dd->pport) {
		words = dd->f_portcntr(dd->pport, QIBPORTCNTR_WORDRCV);
		if (words == ~0ULL)
			ret = PCI_ERS_RESULT_NEED_RESET;
	}
	dd_dev_info(dd,
		"QIB mmio_enabled function called, read wordscntr %Lx, returning %d\n",
		words, ret);
	return  ret;
}

static pci_ers_result_t
qib_pci_slot_reset(struct pci_dev *pdev)
{
	struct hfi_devdata *dd = pci_get_drvdata(pdev);
	dd_dev_info(dd, "QIB slot_reset function called, ignored\n");
	return PCI_ERS_RESULT_CAN_RECOVER;
}

static pci_ers_result_t
qib_pci_link_reset(struct pci_dev *pdev)
{
	struct hfi_devdata *dd = pci_get_drvdata(pdev);
	dd_dev_info(dd, "QIB link_reset function called, ignored\n");
	return PCI_ERS_RESULT_CAN_RECOVER;
}

static void
qib_pci_resume(struct pci_dev *pdev)
{
	struct hfi_devdata *dd = pci_get_drvdata(pdev);
	dd_dev_info(dd, "QIB resume function called\n");
	pci_cleanup_aer_uncorrect_error_status(pdev);
	/*
	 * Running jobs will fail, since it's asynchronous
	 * unlike sysfs-requested reset.   Better than
	 * doing nothing.
	 */
	qib_init(dd, 1); /* same as re-init after reset */
}

const struct pci_error_handlers qib_pci_err_handler = {
	.error_detected = qib_pci_error_detected,
	.mmio_enabled = qib_pci_mmio_enabled,
	.link_reset = qib_pci_link_reset,
	.slot_reset = qib_pci_slot_reset,
	.resume = qib_pci_resume,
};
