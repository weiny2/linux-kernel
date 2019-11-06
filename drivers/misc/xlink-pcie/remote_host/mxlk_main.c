// SPDX-License-Identifier: GPL-2.0 only
/*****************************************************************************
 *
 * Intel Keem Bay XLink PCIe Driver
 *
 * Copyright (C) 2019 Intel Corporation
 *
 ****************************************************************************/

#include "../common/mxlk.h"
#include "../common/mxlk_print.h"
#include "mxlk_pci.h"

static const struct pci_device_id mxlk_pci_table[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, 0x6240), 0 },
	{ 0 }
};

static struct mxlk *mxlk;

static int mxlk_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	int error = 0;

	mxlk = kzalloc(sizeof(*mxlk), GFP_KERNEL);
	if (!mxlk)
		return -ENOMEM;

	error = mxlk_pci_init(mxlk, pdev);
	if (error)
		goto error_pci;

	return 0;

error_pci:
	kfree(mxlk);
	mxlk = NULL;

	return error;
}

static void mxlk_remove(struct pci_dev *pdev)
{
	if (mxlk) {
		mxlk_pci_cleanup(mxlk);
		kfree(mxlk);
	}
}

static struct pci_driver mxlk_driver = { .name = MXLK_DRIVER_NAME,
					 .id_table = mxlk_pci_table,
					 .probe = mxlk_probe,
					 .remove = mxlk_remove };

static int __init mxlk_init_module(void)
{
	return pci_register_driver(&mxlk_driver);
}

static void __exit mxlk_exit_module(void)
{
	pci_unregister_driver(&mxlk_driver);
}

module_init(mxlk_init_module);
module_exit(mxlk_exit_module);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Intel");
MODULE_DESCRIPTION(MXLK_DRIVER_DESC);
