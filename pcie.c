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
#include "include/wfr/wfr_pcie_defs.h"

/* link speed vector for Gen3 speed - not in Linux headers */
#define GEN3_SPEED_VECTOR 0x3

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
	hfi_pcie_cleanup(pdev);
done:
	return ret;
}

/*
 * Clean what was done in qib_pcie_init()
 */
void hfi_pcie_cleanup(struct pci_dev *pdev)
{
	pci_disable_device(pdev);
	/*
	 * Release regions should be called after the disable. OK to
	 * call if request regions has not been called or failed.
	 */
	pci_release_regions(pdev);
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
	 * Re-map the chip's RcvArray as write-compbining to allow us
	 * to write an entire cacheline worth of entries in one shot.
	 * If this re-map fails, just continue - the RcvArray programming
	 * function will handle both cases.
	 */
	dd->chip_rcv_array_count = read_csr(dd, WFR_RCV_ARRAY_CNT);
	dd->rcvarray_wc = ioremap_wc(addr + WFR_RCV_ARRAY,
				     dd->chip_rcv_array_count * 8);
	dd_dev_info(dd, "WC Remapped RcvArray: %p\n", dd->rcvarray_wc);
	/*
	 * Save BARs and command to rewrite after device reset.
	 */
	dd->pcibar0 = addr;
	dd->pcibar1 = addr >> 32;
	pci_read_config_dword(dd->pcidev, PCI_ROM_ADDRESS, &dd->pci_rom);
	pci_read_config_word(dd->pcidev, PCI_COMMAND, &dd->pci_command);
	dd->deviceid = ent->device; /* save for later use */
	dd->vendorid = ent->vendor;
	/*
	 * Read the subsystem values directly rather than use ent->sub*.
	 * The ent->sub* values are -1 (wildcard) rather than the actual
	 * values because the device table match uses a wildcard for those
	 * values.
	 */
	pci_read_config_word(dd->pcidev, PCI_SUBSYSTEM_VENDOR_ID,
		&dd->subvendorid);
	pci_read_config_word(dd->pcidev, PCI_SUBSYSTEM_ID, &dd->subdeviceid);

	return 0;
}

/*
 * Do PCIe cleanup related to dd, after chip-specific cleanup, etc.  Just prior
 * to releasing the dd memory.
 * Void because all of the core pcie cleanup functions are void.
 */
void qib_pcie_ddcleanup(struct hfi_devdata *dd)
{
	u64 __iomem *base = (void __iomem *) dd->kregbase;

	dd->flags &= ~QIB_PRESENT;
	dd->kregbase = NULL;
	iounmap(base);
	if (dd->rcvarray_wc)
		iounmap(dd->rcvarray_wc);
	if (dd->piobase)
		iounmap(dd->piobase);

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

/* return the PCIe link speed from the given link status */
static u32 extract_speed(u16 linkstat)
{
	u32 speed;

	switch (linkstat & PCI_EXP_LNKSTA_CLS) {
	default: /* not defined, assume Gen1 */
	case PCI_EXP_LNKSTA_CLS_2_5GB:
		speed = 2500; /* Gen 1, 2.5GHz */
		break;
	case PCI_EXP_LNKSTA_CLS_5_0GB:
		speed = 5000; /* Gen 2, 5GHz */
		break;
	case GEN3_SPEED_VECTOR:
		speed = 8000; /* Gen 3, 8GHz */
		break;
	}
	return speed;
}

/* return the PCIe link speed from the given link status */
static u32 extract_width(u16 linkstat)
{
	return (linkstat & PCI_EXP_LNKSTA_NLW) >> PCI_EXP_LNKSTA_NLW_SHIFT;
}

/* read the link status and set dd->{lbus_width,lbus_speed,lbus_info} */
static void update_lbus_info(struct hfi_devdata *dd)
{
	u16 linkstat;

	pcie_capability_read_word(dd->pcidev, PCI_EXP_LNKSTA, &linkstat);
	dd->lbus_width = extract_width(linkstat);
	dd->lbus_speed = extract_speed(linkstat);
	snprintf(dd->lbus_info, sizeof(dd->lbus_info),
		 "PCIe,%uMHz,x%u", dd->lbus_speed, dd->lbus_width);
}

/*
 * Read in the current PCIe link width and speed.  Find if the link is
 * Gen3 capable.
 */
int pcie_speeds(struct hfi_devdata *dd)
{
	u32 linkcap;

	if (!pci_is_pcie(dd->pcidev)) {
		dd_dev_err(dd, "Can't find PCI Express capability!\n");
		return -EINVAL;
	}

	/* find if our max speed is Gen3 and parent supports Gen3 speeds */
	dd->link_gen3_capable = 1;

	pcie_capability_read_dword(dd->pcidev, PCI_EXP_LNKCAP, &linkcap);
	if ((linkcap & PCI_EXP_LNKCAP_SLS) != GEN3_SPEED_VECTOR) {
		dd_dev_info(dd,
			"This HFI is not Gen3 capable, max speed 0x%x, need 0x3\n",
			linkcap & PCI_EXP_LNKCAP_SLS);
		dd->link_gen3_capable = 0;
	}

	/*
	 * bus->max_bus_speed is set from the bridge's linkcap Max Link Speed
	 */
	if (dd->pcidev->bus->max_bus_speed != PCIE_SPEED_8_0GT) {
		dd_dev_info(dd, "Parent PCIe bridge does not support Gen3\n");
		dd->link_gen3_capable = 0;
	}

	/* obtain the link width and current speed */
	update_lbus_info(dd);

	/* check against expected pcie width and complain if "wrong" */
	if (dd->lbus_width < 16)
		dd_dev_err(dd, "PCIe width %u (x16 HCA), performance reduced\n",
			dd->lbus_width);

	return 0;
}

/*
 * Returns in *nent:
 *	- actual number of interrupts allocated
 *	- 0 if fell back to INTx.
 */
void request_msix(struct hfi_devdata *dd, u32 *nent,
			struct qib_msix_entry *entry)
{
	int pos;

	pos = pci_find_capability(dd->pcidev, PCI_CAP_ID_MSIX);
	if (*nent && pos) {
		qib_msix_setup(dd, pos, nent, entry);
		/* did it, either MSI-X or INTx */
	} else {
		*nent = 0;
		qib_enable_intx(dd->pcidev);
	}

	qib_tune_pcie_caps(dd);
	qib_tune_pcie_coalesce(dd);
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

static inline u32 read_dbi_status(struct hfi_devdata *dd)
{
	return (u32)((read_csr(dd, WFR_CCE_DBI_CTRL)
					& WFR_CCE_DBI_CTRL_STATUS_SMASK)
				>> WFR_CCE_DBI_CTRL_STATUS_SHIFT);
}

/*
 * Wait for DBI to go idle.  Return
 *   0 on success
 *   -ETIMEDOUT on timeout
 */
static int wait_dbi_idle(struct hfi_devdata *dd)
{
	unsigned long timeout;
	u32 status;

	timeout = jiffies + msecs_to_jiffies(DBI_TIMEOUT);
	while (1) {
		status = read_dbi_status(dd);
		if (status == DBI_CTL_IDLE)
			return 0;
		if (time_after(jiffies, timeout)) {
			dd_dev_err(dd, "Timeout waiting for DBI to go idle\n");
			return -ETIMEDOUT;
		}
		/* keep trying to go to idle */
		write_csr(dd, WFR_CCE_DBI_CTRL, 0);
	}
}

/*
 * Wait for DBI to finish.  Return
 *   0 on success
 *   -ETIMEDOUT on timeout
 *   -EIO on error
 */
static int wait_dbi_done(struct hfi_devdata *dd)
{
	unsigned long timeout;
	u32 status;

	timeout = jiffies + msecs_to_jiffies(DBI_TIMEOUT);
	while (1) {
		status = read_dbi_status(dd);
		if (status == DBI_CTL_SUCCESS)
			return 0;
		if (status == DBI_CTL_ERROR) {
			dd_dev_err(dd, "DBI error\n");
			return -EIO;
		}
		if (time_after(jiffies, timeout)) {
			dd_dev_err(dd,
				"Timeout waiting for DBI to finish, status %d\n",
				status);
			return -ETIMEDOUT;
		}
		udelay(1);
	}
}

static int adjust_pci(struct hfi_devdata *dd, u32 address, u32 data)
{
	int ret;

	/* simulator does not implement DBI */
	if (dd->icode == WFR_ICODE_FUNCTIONAL_SIMULATOR)
		return 0;

	/* make sure DBI is idle */
	ret = wait_dbi_idle(dd);
	if (ret)
		return ret;

	write_csr(dd, WFR_CCE_DBI_ADDR, address);
	write_csr(dd, WFR_CCE_DBI_DATA, data);
	/* must set WriteEnables, then CS, in two writes */
	write_csr(dd, WFR_CCE_DBI_CTRL,
		(u32)0xf << WFR_CCE_DBI_CTRL_WRITE_ENABLES_SHIFT);
	write_csr(dd, WFR_CCE_DBI_CTRL,
		((u32)0xf << WFR_CCE_DBI_CTRL_WRITE_ENABLES_SHIFT)
		| ((u32)1 << WFR_CCE_DBI_CTRL_CS_SHIFT));

	/* wait for DBI to finish */
	ret = wait_dbi_done(dd);
	/* (try to) go idle no matter what happened */
	write_csr(dd, WFR_CCE_DBI_CTRL, 0);

	return ret;
}

/* restore command and BARs after a reset has wiped them out */
void restore_pci_variables(struct hfi_devdata *dd)
{
	pci_write_config_word(dd->pcidev, PCI_COMMAND, dd->pci_command);
	pci_write_config_dword(dd->pcidev,
				PCI_BASE_ADDRESS_0, dd->pcibar0);
	pci_write_config_dword(dd->pcidev,
				PCI_BASE_ADDRESS_1, dd->pcibar1);
	pci_write_config_dword(dd->pcidev,
				PCI_ROM_ADDRESS, dd->pci_rom);
	/*
	 * TODO: Use of DBI back-door is undesirable but the only way to
	 *       restore the subsystem IDs.
	 * TODO: This is an _expected_ B0 fix.  A new PCIe is being tested
	 *	 now that has this fixed.  Adding the A0 check for now.
	 * HSD 290390 - Need to restore subsystem ids after a reset.
	 */
	if (is_a0(dd)) {
		int ret;

		ret = adjust_pci(dd, WFR_PCI_CFG_REG11,
			(u32)dd->subdeviceid << 16 | (u32)dd->subvendorid);
		if (ret)
			dd_dev_err(dd,
				"Unable to restore PCI subsystem settings\n");
	}
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

/*============================================================================*/
/* PCIe Gen3 support */

/*
 * This code is separated out because it is expected to be removed in the
 * final shipping product.  If not, then it will be revisited and items
 * will be moved to more standard locations.
 */

/* this CSR definition does not appear in the pcie_defs headers */
#define WFR_PCIE_CFG_REG_PL106 (WFR_PCIE + 0x0000000008A8)
#define WFR_PCIE_CFG_REG_PL106_GEN3_EQ_PSET_REQ_VEC_SHIFT 8

/* ASIC_PCI_SD_HOST_STATUS.FW_DNLD_STS field values */
#define WFR_DL_STATUS_HFI0 0x1	/* hfi0 firmware download complete */
#define WFR_DL_STATUS_HFI1 0x2	/* hfi1 firmware download complete */
#define WFR_DL_STATUS_BOTH 0x3	/* hfi0 and hfi1 firmware download complete */

/* ASIC_PCI_SD_HOST_STATUS.FW_DNLD_ERR field values */
#define WFR_DL_ERR_NONE		0x0	/* no error */
#define WFR_DL_ERR_SWAP_PARITY	0x1	/* parity error in SerDes interrupt */
					/*   or response data */
#define WFR_DL_ERR_DISABLED	0x2	/* hfi disabled */
#define WFR_DL_ERR_SECURITY	0x3	/* security check failed */
#define WFR_DL_ERR_SBUS		0x4	/* SBUS status error */
#define WFR_DL_ERR_XFR_PARITY	0x5	/* parity error during ROM transfer*/

/* gasket block secondary bus reset delay */
#define SBR_DELAY_US 20000	/* 20ms */

/* mask for PCIe capability regiser lnkctl2 target link speed */
#define LNKCTL2_TARGET_LINK_SPEED_MASK 0xf

static uint pcie_gen3_transition;
module_param(pcie_gen3_transition, uint, S_IRUGO);
MODULE_PARM_DESC(pcie_gen3_transition, "Driver will attempt the transition to PCIe Gen3 speed");

static uint pcie_gen3_required;
module_param(pcie_gen3_required, uint, S_IRUGO);
MODULE_PARM_DESC(pcie_gen3_required, "Driver will fail to load if unable to raech PCie Gen3 speed");

/* for testing */
static uint pcie_gen3_ignore_speed_check;
module_param(pcie_gen3_ignore_speed_check, uint, S_IRUGO);
MODULE_PARM_DESC(pcie_gen3_ignore_speed_check, "Ignore check whether links are gen3 capable (test only)");

/* discrete silicon preliminary equalization values */
static const u8 discrete_preliminary_eq[11][3] = {
	/* prec   attn   post */
	{  0x00,  0x00,  0x00 },	/* p0 */
	{  0x00,  0x00,  0x0c },	/* p1 */
	{  0x00,  0x00,  0x0f },	/* p2 */
	{  0x00,  0x00,  0x09 },	/* p3 */
	{  0x00,  0x00,  0x00 },	/* p4 */
	{  0x06,  0x00,  0x00 },	/* p5 */
	{  0x09,  0x00,  0x00 },	/* p6 */
	{  0x06,  0x00,  0x0f },	/* p7 */
	{  0x09,  0x00,  0x09 },	/* p8 */
	{  0x0c,  0x00,  0x09 },	/* p9 */
	{  0x0c,  0x00,  0x18 },	/* p10 */
};

/* integrated silicon preliminary equalization values */
static const u8 integrated_preliminary_eq[11][3] = {
	/* prec   attn   post */
	{  0x00,  0x1e,  0x07 },	/* p0 */
	{  0x00,  0x1e,  0x05 },	/* p1 */
	{  0x00,  0x1e,  0x06 },	/* p2 */
	{  0x00,  0x1e,  0x04 },	/* p3 */
	{  0x00,  0x1e,  0x00 },	/* p4 */
	{  0x03,  0x1e,  0x00 },	/* p5 */
	{  0x04,  0x1e,  0x00 },	/* p6 */
	{  0x03,  0x1e,  0x06 },	/* p7 */
	{  0x03,  0x1e,  0x04 },	/* p8 */
	{  0x05,  0x1e,  0x00 },	/* p9 */
	{  0x00,  0x1e,  0x0a },	/* p10 */
};

/* helper to format the value to write to hardware */
#define eq_value(pre, curr, post) \
	(((u32)(pre) << WFR_PCIE_CFG_REG_PL102_GEN3_EQ_PRE_CURSOR_PSET_SHIFT) \
	| ((u32)(curr) << WFR_PCIE_CFG_REG_PL102_GEN3_EQ_CURSOR_PSET_SHIFT) \
	| ((u32)(post) << \
		WFR_PCIE_CFG_REG_PL102_GEN3_EQ_POST_CURSOR_PSET_SHIFT))

/*
 * Load the given EQ preset table into the PCIe hardware.
 */
static int load_eq_table(struct hfi_devdata *dd, const u8 eq[11][3])
{
	struct pci_dev *pdev = dd->pcidev;
	u32 hit_error = 0;
	u32 violation;
	u32 i;

	for (i = 0; i < 11; i++) {
		/* set index */
		pci_write_config_dword(pdev, WFR_PCIE_CFG_REG_PL103, i);
		/* write the value */
		pci_write_config_dword(pdev, WFR_PCIE_CFG_REG_PL102,
			eq_value(eq[i][0], eq[i][1], eq[i][2]));
		/* check if these coefficients violate EQ rules */
		pci_read_config_dword(dd->pcidev, WFR_PCIE_CFG_REG_PL105,
								&violation);
		if (violation) {
			if (hit_error == 0) {
				dd_dev_err(dd,
					"Gen3 EQ Table Coefficient rule violations\n");
				dd_dev_err(dd, "         prec   attn   post\n");
			}
			dd_dev_err(dd, "   p%02d:   %02x     %02x     %02x\n",
				i, (u32)eq[i][0], (u32)eq[i][1], (u32)eq[i][2]);
			hit_error = 1;
		}
	}
	if (hit_error)
		return -EINVAL;
	return 0;
}

/*
 * Trigger a secondary bus reset (SBR) on ourself using our parent.
 *
 * Based on pci_parent_bus_reset() which is not exported by the
 * kernel core.
 */
static int trigger_sbr(struct hfi_devdata *dd)
{
	struct pci_dev *dev = dd->pcidev;
	struct pci_dev *pdev;

	/* need a parent */
	if (!dev->bus->self) {
		dd_dev_err(dd, "%s: no parent device\n", __func__);
		return -ENOTTY;
	}

	/* should not be anyone else on the bus */
	list_for_each_entry(pdev, &dev->bus->devices, bus_list)
		if (pdev != dev) {
			dd_dev_err(dd,
				"%s: another device is on the same bus\n",
				__func__);
			return -ENOTTY;
		}

	/*
	 * A secondary bus reset (SBR) issues a hot reset to our device.
	 * The following routine does a 1s wait after the reset is dropped
	 * per PCI Trhfa (recovery time).  PCIe 3.0 section 6.6.1 -
	 * Conventional Reset, paragraph 3, line 35 also says that a 1s
	 * delay after a reset is required.  Per spec requirements,
	 * the link is either working or not after that point.
	 */
	pci_reset_bridge_secondary_bus(dev->bus->self);

	return 0;
}

/*
 * Tell the gasket logic how to react to the reset.
 */
static void arm_gasket_logic(struct hfi_devdata *dd)
{
	u64 reg;

	reg = (((u64)1 << dd->hfi_id)
			<< WFR_ASIC_PCIE_SD_HOST_CMD_INTRPT_CMD_SHIFT)
		| ((u64)pcie_serdes_broadcast[dd->hfi_id]
			<< WFR_ASIC_PCIE_SD_HOST_CMD_SBUS_RCVR_ADDR_SHIFT
		| WFR_ASIC_PCIE_SD_HOST_CMD_SBR_MODE_SMASK
		| ((u64)SBR_DELAY_US & WFR_ASIC_PCIE_SD_HOST_CMD_TIMER_MASK)
			<< WFR_ASIC_PCIE_SD_HOST_CMD_TIMER_SHIFT
		);
	write_csr(dd, WFR_ASIC_PCIE_SD_HOST_CMD, reg);
}

/*
 * Tell the PCIe SerDes to ignore the first EQ presset value.
 */
static void sbus_ignore_first_eq_preset(struct hfi_devdata *dd)
{
	u8 ra;				/* receiver address */

	ra = pcie_serdes_broadcast[dd->hfi_id];
	sbus_request(dd, ra, 0x03, WRITE_SBUS_RECEIVER, 0x00265202);
}

/*
 * Do all the steps needed to transition the PCIe link to Gen3 speed.
 */
int do_pcie_gen3_transition(struct hfi_devdata *dd)
{
	u64 reg;
	u32 reg32, fs, lf;
	int ret, return_error = 0;
	u16 lnkctl, lnkctl2, vendor;

	/* if already at Gen3, done */
	if (dd->lbus_speed == 8000) /* Gen3, 8GHz */
		return 0;

	/* told not to do the transition */
	if (!pcie_gen3_transition)
		return 0;

	/*
	 * Emulation has no SBUS and the PCIe SerDes is different.  The only
	 * thing we can to is try a SBR to see if we can recover.
	 */
	if (dd->icode == WFR_ICODE_FPGA_EMULATION) {
		/* hold DC in reset */
		write_csr(dd, WFR_CCE_DC_CTRL, WFR_CCE_DC_CTRL_DC_RESET_SMASK);
		(void) read_csr(dd, WFR_CCE_DC_CTRL); /* DC reset hold */

		dd_dev_info(dd, "%s: Setting CceScratch[0] to 0xdeadbeef\n",
			__func__);
		write_csr(dd, WFR_CCE_SCRATCH, 0xdeadbeefull);

		dd_dev_info(dd, "%s: calling trigger_sbr\n", __func__);
		ret = trigger_sbr(dd);
		if (ret)
			goto done_no_mutex;

		/* restore PCI space registers we know were reset */
		dd_dev_info(dd,
			"%s: calling restore_pci_variables\n", __func__);
		restore_pci_variables(dd);

		/*
		 * Read the CCE scratch register we set above to see if:
		 * a) the link works
		 * b) if the reset was applied and our CCE scratch is
		 *    zeroed
		 */
		reg = read_csr(dd, WFR_CCE_SCRATCH);
		if (reg == ~0ull) {
			dd_dev_info(dd, "%s: cannot read CSRs\n", __func__);
			return_error = 1; /* override !pcie_gen3_required */
			goto done_no_mutex;
		}
		dd_dev_info(dd, "%s: CceScratch[0] 0x%llx\n", __func__, reg);

		/* clear the DC reset */
		write_csr(dd, WFR_CCE_DC_CTRL, 0);

		/* update our link information cache */
		update_lbus_info(dd);
		dd_dev_info(dd, "%s: new speed and width: %s\n", __func__,
			dd->lbus_info);

		/* finished */
		goto done_no_mutex;
	}

	/*
	 * Do the Gen3 transition.  Steps are those of the PCIe Gen3
	 * recipe, Table 5-6 in the WFR HAS.
	 */

	/* step 1: pcie link working in gen1/gen2 */

	/* step 2: if either side is not capable of Gen3, done */
	if (!dd->link_gen3_capable && !pcie_gen3_ignore_speed_check) {
		dd_dev_err(dd, "The PCIe link is not Gen3 capable\n");
		ret = -ENOSYS;
		goto done_no_mutex;
	}

	/* hold the HW mutex across the firmware download and SBR */
	ret = acquire_hw_mutex(dd);
	if (ret)
		return ret;

	/* step 3: download SBUS Master firmware */
	/* step 4: download PCIe Gen3 SerDes firmware */
	dd_dev_info(dd, "%s: downloading firmware\n", __func__);
	ret = load_pcie_firmware(dd);
	if (ret)
		goto done;

	/* step 5: set up device parameter settings */
	dd_dev_info(dd, "%s: setting PCIe registers\n", __func__);

	/*
	 * PcieCfgSpcie1 - Link Control 3
	 * Leave at reset value.  No need to set PerfEq - link equalization
	 * will be performced automatically after the SBR when the target
	 * speed is 8GT/s.
	 */

	/* clear all 16 per-lane error bits (PCIe: Lane Error Status) */
	pci_write_config_dword(dd->pcidev, WFR_PCIE_CFG_SPCIE2, 0xffff);

	/* step 5a: Set Synopsys Port Logic registers */

	/*
	 * PcieCfgRegPl2 - Port Force Link
	 *
	 * Set the low power field to 0x10 to avoid unnecessary power
	 * management messages.  All other fields are zero.
	 */
	reg32 = 0x10ul << WFR_PCIE_CFG_REG_PL2_LOW_PWR_ENT_CNT_SHIFT;
	pci_write_config_dword(dd->pcidev, WFR_PCIE_CFG_REG_PL2, reg32);

	/*
	 * PcieCfgRegPl100 - Gen3 Control
	 *
	 * turn off PcieCfgRegPl100.Gen3ZRxDcNonCompl (erratum)
	 * turn on PcieCfgRegPl100.EqEieosCnt (erratum)
	 * Everything else zero.
	 */
	reg32 = WFR_PCIE_CFG_REG_PL100_EQ_EIEOS_CNT_SMASK;
	pci_write_config_dword(dd->pcidev, WFR_PCIE_CFG_REG_PL100, reg32);

	/*
	 * PcieCfgRegPl101 - Gen3 EQ FS and LF
	 * PcieCfgRegPl102 - Gen3 EQ Presets to Coefficients Mapping
	 * PcieCfgRegPl103 - Gen3 EQ Preset Index
	 * PcieCfgRegPl105 - Gen3 EQ Status
	 *
	 * Give initial EQ settings.
	 */
	if (dd->deviceid == PCI_DEVICE_ID_INTEL_WFR0) { /* discrete */
		/* 1000mV, FS=24, LF = 8 */
		ret = load_eq_table(dd, discrete_preliminary_eq);
		fs = 24;
		lf = 8;
	} else {
		/* 400mV, FS=29, LF = 9 */
		ret = load_eq_table(dd, integrated_preliminary_eq);
		fs = 29;
		lf = 9;
	}
	if (ret)
		goto done;
	pci_write_config_dword(dd->pcidev, WFR_PCIE_CFG_REG_PL101,
		(fs << WFR_PCIE_CFG_REG_PL101_GEN3_EQ_LOCAL_FS_SHIFT)
		| (lf << WFR_PCIE_CFG_REG_PL101_GEN3_EQ_LOCAL_LF_SHIFT));

	/*
	 * PcieCfgRegPl106 - Gen3 EQ Control
	 *
	 * Gen3EqPsetReqVec=0x4
	 */
	pci_write_config_dword(dd->pcidev, WFR_PCIE_CFG_REG_PL106,
		0x4 << WFR_PCIE_CFG_REG_PL106_GEN3_EQ_PSET_REQ_VEC_SHIFT);

	/*
	 * step 5b: Tell the serdes to ignore first EQ preset.
	 */
	sbus_ignore_first_eq_preset(dd);

	/*
	 * step 5c: program XMT margin
	 * Right now, leave the default alone.  To change, do a
	 * read-modify-write of:
	 *	CcePcieCtrl.XmtMargin
	 *	CcePcieCtrl.XmitMarginOverwriteEnable
	 */

	/* step 5d: disable active state power management (ASPM) */
	pcie_capability_read_word(dd->pcidev, PCI_EXP_LNKCTL, &lnkctl);
	lnkctl &= ~PCI_EXP_LNKCTL_ASPMC;
	pcie_capability_write_word(dd->pcidev, PCI_EXP_LNKCTL, lnkctl);

	/*
	 * step 5e: clear DirectSpeedChange
	 * PcieCfgRegPl67.DirectSpeedChange must be zero to prevent the
	 * change in the speed target from starting before we are ready.
	 * This field defaults to 0 and we are not changing it, so nothing
	 * needs to be done.
	 */

	/* step 5f: arm gasket logic */
	dd_dev_info(dd, "%s: arming gasket logic\n", __func__);
	arm_gasket_logic(dd);

	/* step 5g: Set target link speed */
	/* set target link speed to be Gen3 */
	dd_dev_info(dd, "%s: setting target link speed\n", __func__);
	pcie_capability_read_word(dd->pcidev, PCI_EXP_LNKCTL2, &lnkctl2);
	dd_dev_info(dd, "%s: ..old link control2: 0x%x\n", __func__,
		(u32)lnkctl2);
	lnkctl2 &= ~LNKCTL2_TARGET_LINK_SPEED_MASK;
	lnkctl2 |= GEN3_SPEED_VECTOR;
	dd_dev_info(dd, "%s: ..new link control2: 0x%x\n", __func__,
		(u32)lnkctl2);
	pcie_capability_write_word(dd->pcidev, PCI_EXP_LNKCTL2, lnkctl2);

	/* hold DC in reset across the SBR */
	write_csr(dd, WFR_CCE_DC_CTRL, WFR_CCE_DC_CTRL_DC_RESET_SMASK);
	(void) read_csr(dd, WFR_CCE_DC_CTRL); /* DC reset hold */

	/*
	 * step 6: quiesce PCIe link
	 * The chip has already been reset, so there will be no traffic
	 * from the chip.  Linux has no easy way to enforce that it will
	 * not try to access the device, so we just need to hope it doesn't
	 * do it while we are doing the reset.
	 */

	/*
	 * step 7: initiate the secondary bus reset (SBR)
	 * step 8: hardware brings the links back up
	 * step 9: wait for link speed transition to be complete
	 */
	dd_dev_info(dd, "%s: calling trigger_sbr\n", __func__);
	ret = trigger_sbr(dd);
	if (ret)
		goto done;

	/* step 10: decide what to do next */

	/* check if we can read PCI space */
	ret = pci_read_config_word(dd->pcidev, PCI_VENDOR_ID, &vendor);
	if (ret) {
		dd_dev_info(dd,
			"%s: read of VendorID failed after SBR, err %d\n",
			__func__, ret);
		return_error = 1;	/* override !pcie_gen3_required */
		goto done;
	}
	if (vendor == 0xffff) {
		dd_dev_info(dd, "%s: VendorID is all 1s after SBR\n", __func__);
		return_error = 1;	/* override !pcie_gen3_required */
		ret = -EIO;
		goto done;
	}

	/* restore PCI space registers we know were reset */
	dd_dev_info(dd, "%s: calling restore_pci_variables\n", __func__);
	restore_pci_variables(dd);

	/*
	 * Check the gasket block status.
	 *
	 * If the read returns all 1s (fails), the link did not make it back.
	 *
	 * If ASIC_PCIE_SD_HOST_STATUS is non-zero then the gasket block
	 * failed.  Read the status for more details.
	 *
	 * If ASIC_PCIE_SD_HOST_STATUS is zero then the reset occured,
	 * the link is back, and (presumably) everything is OK.
	 */
	reg = read_csr(dd, WFR_ASIC_PCIE_SD_HOST_STATUS);
	dd_dev_info(dd, "%s: gasket block status: 0x%llx\n", __func__, reg);
	if (reg == ~0ull) {	/* PCIe read failed/timeout */
		dd_dev_err(dd, "SBR failed - unable to read from device\n");
		return_error = 1;	/* override !pcie_gen3_required */
		ret = -ENOSYS;
		goto done;
	}

	/* clear the DC reset */
	write_csr(dd, WFR_CCE_DC_CTRL, 0);

	pci_read_config_dword(dd->pcidev, WFR_PCIE_CFG_SPCIE2, &reg32);
	dd_dev_info(dd, "%s: per-lane errors: 0x%x\n", __func__, reg32);

	/*
	 * If ASIC_PCIE_SD_HOST_STATUS is zero then the reset occured.
	 * Otherwise the gasket block failed.  Read the status for
	 * more information.
	 */
	if (reg != 0) {
		u32 status, err;

		status = (reg >> WFR_ASIC_PCIE_SD_HOST_STATUS_FW_DNLD_STS_SHIFT)
				& WFR_ASIC_PCIE_SD_HOST_STATUS_FW_DNLD_STS_MASK;
		if (status != (1 << dd->hfi_id)) {
			dd_dev_err(dd,
				"%s: gasket status 0x%x, expecting 0x%x\n",
				__func__, status, (int)dd->hfi_id);
		}
		err = (reg >> WFR_ASIC_PCIE_SD_HOST_STATUS_FW_DNLD_ERR_SHIFT)
			& WFR_ASIC_PCIE_SD_HOST_STATUS_FW_DNLD_ERR_MASK;
		dd_dev_err(dd, "%s: gasket error %d\n", __func__, err);
		ret = -EIO;
		goto done;
	}

	/* update our link information cache */
	update_lbus_info(dd);
	dd_dev_info(dd, "%s: new speed and width: %s\n", __func__,
		dd->lbus_info);

	if (dd->lbus_speed != 8000) {	/* not Gen3 */
		/*
		 * At this point we could try again with different
		 * parameters in the hopes of success.  For now, just
		 * report an error.
		 */
		dd_dev_err(dd, "PCIe link speed did not switch to Gen3\n");
		ret = -EIO;
	}

done:
	release_hw_mutex(dd);
done_no_mutex:
	/* return no error if it is OK to be in Gen1/2 */
	if (ret && !return_error && !pcie_gen3_required) {
		dd_dev_err(dd, "Proceeding at current speed PCIe speed\n");
		ret = 0;
	}

	dd_dev_info(dd, "%s: done\n", __func__);
	return ret;
}
