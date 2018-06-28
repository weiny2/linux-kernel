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
#include <linux/module.h>
#include <linux/printk.h>
#include "verbs/verbs.h"
#include "hfi2.h"
#include "debugfs.h"

static struct pci_device_id hfi_pci_tbl[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_FXR0) },
	{ 0, }
};
MODULE_DEVICE_TABLE(pci, hfi_pci_tbl);

static void hfi_pci_remove(struct pci_dev *);
static int hfi_pci_probe(struct pci_dev *, const struct pci_device_id *);

static struct pci_driver hfi_driver = {
	.name = DRIVER_NAME,
	.probe = hfi_pci_probe,
	.remove = hfi_pci_remove,
	.id_table = hfi_pci_tbl,
	.err_handler = &hfi_pci_err_handler,
};

/*
 * Allocate our primary per HFI-device data structure.
 */
struct hfi_devdata *hfi_alloc_devdata(struct pci_dev *pdev)
{
	struct hfi_devdata *dd;
	size_t psize = sizeof(struct hfi_pportdata) * HFI_NUM_PPORTS;

	dd = kzalloc(sizeof(*dd) + psize, GFP_KERNEL);
	if (!dd)
		return ERR_PTR(-ENOMEM);

	dd->node = dev_to_node(&pdev->dev);
	if (dd->node < 0) {
		dev_info(&pdev->dev, "dev_to_node ret %d, trying 0.", dd->node);
		dd->node = 0;
	}

	dd->pport = (struct hfi_pportdata *)(dd + 1);
	return dd;
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

	ret = hfi_snoop_add(dd);
	if (ret)
		goto err_snoop;

	ret = hfi2_diag_init(dd);
	if (ret)
		goto err_diag;

	hfi_dbg_dev_init(dd);

	return 0;

err_diag:
	hfi_snoop_remove(dd);
err_snoop:
	hfi_pci_dd_free(dd);
err_dd_init:
	hfi_pci_cleanup(pdev);
err_pci:
	return ret;
}

static void hfi_pci_remove(struct pci_dev *pdev)
{
	struct hfi_devdata *dd = pci_get_drvdata(pdev);

	hfi_dbg_dev_exit(dd);
	hfi2_diag_uninit(dd);
	hfi_snoop_remove(dd);
	hfi_pci_dd_free(dd);
	hfi_pci_cleanup(pdev);
}

static int __init hfi_init(void)
{
	int ret;

	hfi_dbg_init();
	hfi_mod_init();

	hfi2_diag_add();

	ret = pci_register_driver(&hfi_driver);
	if (ret < 0) {
		pr_err("Unable to register driver: error %d\n", ret);
		goto pci_err;
	}
	return 0;

pci_err:
	hfi2_diag_remove();
	hfi_mod_deinit();
	hfi_dbg_exit();
	return ret;
}
module_init(hfi_init);

static void hfi_cleanup(void)
{
	pci_unregister_driver(&hfi_driver);
	hfi2_diag_remove();
	hfi_mod_deinit();
	hfi_dbg_exit();
}
module_exit(hfi_cleanup);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Intel Corporation");
MODULE_DESCRIPTION("Intel(R) Omni-Path Gen2 HFI Driver");
