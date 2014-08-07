/*
 * Copyright (c) 2012 - 2014 Intel Corporation.  All rights reserved.
 * Copyright (c) 2006 - 2012 QLogic Corporation. All rights reserved.
 * Copyright (c) 2003, 2004, 2005, 2006 PathScale, Inc. All rights reserved.
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
#include <linux/module.h>
#include <linux/printk.h>
#include "hfi.h"
#include "device.h"
#include "debugfs.h"

/* TODO - multiple HFI support... */
#define HFI_MAX_DEVICES 1
static struct hfi_devdata *hfi_dev_tbl[HFI_MAX_DEVICES] = {0};

/* TODO: module param? */
static int create_ui = 1;

/*
 * Define the driver version number.  This is something that refers only
 * to the driver itself, not the software interfaces it supports.
 */
#ifndef HFI_DRIVER_VERSION_BASE
#define HFI_DRIVER_VERSION_BASE "0.0"
#endif

/* create the final driver version string */
#ifdef HFI_IDSTR
#define HFI_DRIVER_VERSION HFI_DRIVER_VERSION_BASE " " HFI_IDSTR
#else
#define HFI_DRIVER_VERSION HFI_DRIVER_VERSION_BASE
#endif

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Intel <ibsupport@intel.com>");
MODULE_DESCRIPTION("Intel(R) STL Gen2 HFI Driver");
MODULE_VERSION(HFI_DRIVER_VERSION);

#define PCI_VENDOR_ID_INTEL 0x8086
#define PCI_DEVICE_ID_INTEL_WFR0 0x24f0
#define PCI_DEVICE_ID_INTEL_FXR0 0x24f1

static struct pci_device_id hfi_pci_tbl[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_FXR0) },
	{ 0, }
};
MODULE_DEVICE_TABLE(pci, hfi_pci_tbl);

static void hfi_pci_remove(struct pci_dev *);
static int hfi_pci_probe(struct pci_dev *, const struct pci_device_id *);

struct pci_driver hfi_driver = {
	.name = DRIVER_NAME,
	.probe = hfi_pci_probe,
	.remove = hfi_pci_remove,
	.id_table = hfi_pci_tbl,
};

/*
 * Allocate our primary per HFI-device data structure.
 */
struct hfi_devdata *hfi_alloc_devdata(struct pci_dev *pdev)
{
	struct hfi_devdata *dd;
	int unit;

	dd = kzalloc(sizeof(*dd), GFP_KERNEL);
	if (!dd)
		return ERR_PTR(-ENOMEM);

	/* TODO, multi-device, add locking */
	unit = 0;
	if (hfi_dev_tbl[unit] != NULL) {
		kfree(dd);
		return ERR_PTR(-ENOSPC);
	}
	hfi_dev_tbl[unit] = dd;

	dd->node = dev_to_node(&pdev->dev);
	dd->unit = unit;

	return dd;
}

/*
 * Create per HFI device files in /dev
 */
static int hfi_device_create(struct hfi_devdata *dd)
{
	int ret;

	ret = hfi_user_add(dd);
	if (!ret && create_ui)
		ret = hfi_ui_add(dd);
#ifdef HFI_DIAG_SUPPORT
	if (!ret)
		ret = hfi_diag_add(dd);
#endif
	return ret;
}

/*
 * Remove per-unit files in /dev
 */
static void hfi_device_remove(struct hfi_devdata *dd)
{
	hfi_user_remove(dd);
	hfi_ui_remove(dd);
#ifdef HFI_DIAG_SUPPORT
	hfi_diag_remove(dd);
#endif
}

/*
 * Device initialization, called from PCI probe.
 */
static int hfi_device_init(struct hfi_devdata *dd)
{
	int ret;

	ret = hfi_device_create(dd);
	if (ret)
		dd_dev_err(dd, "Failed to create /dev devices: %d\n", -ret);

	hfi_dbg_init(dd);
	return 0;
}

/*
 * Perform required device shutdown logic, also remove /dev entries.
 * Called when unloading the driver.
 */
static void hfi_device_shutdown(struct hfi_devdata *dd)
{
	hfi_dbg_exit();
	hfi_device_remove(dd);
}

/*
 * Cleanup and free data structures. Performed after device shutdown.
 */
static void hfi_device_cleanup(struct hfi_devdata *dd)
{
	struct pci_dev *pdev = dd->pcidev;

	hfi_pci_dd_free(dd);
	hfi_pci_cleanup(pdev);
}

/*
 * Device probe - where life begins.
 */
static int hfi_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	int ret;
	struct hfi_devdata *dd = NULL;

	ret = hfi_pci_init(pdev, ent);
	if (ret)
		goto err_pci;

	/*
	 * Do device hw-specific initialization, function table setup, dd
	 * allocation, etc.
	 */
	switch (ent->device) {
	case PCI_DEVICE_ID_INTEL_FXR0:
		dd = hfi_pci_dd_init(pdev, ent);
		if (IS_ERR(dd))
			ret = PTR_ERR(dd);
		break;
	default:
		dev_err(&pdev->dev,
			"Failing on unknown Intel deviceid 0x%x\n",
			ent->device);
		ret = -ENODEV;
	}

	if (ret)
		goto err_dd_init;

	/* Complete remaining device setup so HFI is ready for use. */
	ret = hfi_device_init(dd);
	if (ret)
		goto err_device_init;

	/* TODO - optionally register with IB core */
	//register_ib_device(dd);

	return 0;

err_device_init:
	hfi_pci_dd_free(dd);
err_dd_init:
	hfi_pci_cleanup(pdev);
err_pci:
	return ret;
}

static void hfi_pci_remove(struct pci_dev *pdev)
{
	struct hfi_devdata *dd = pci_get_drvdata(pdev);

	/* TODO - unregister from IB core */
	//unregister_ib_device(dd);

	hfi_device_shutdown(dd);
	hfi_device_cleanup(dd);
}
