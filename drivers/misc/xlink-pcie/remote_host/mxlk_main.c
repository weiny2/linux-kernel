// SPDX-License-Identifier: GPL-2.0 only
/*****************************************************************************
 *
 * Intel Keem Bay XLink PCIe Driver
 *
 * Copyright (C) 2019 Intel Corporation
 *
 ****************************************************************************/

#include "mxlk_pci.h"

static int func_no = 0;
module_param(func_no, int, 0664);
MODULE_PARM_DESC(func_no, "THB function number enabler");

static const struct pci_device_id mxlk_pci_table[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, 0x4fc0), 0 },
	{ 0 }
};

static int mxlk_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	int ret = 0;
        uint8_t func = 0xFF;
	struct mxlk_pcie *xdev = NULL;
	        func = PCI_FUNC(pdev->devfn);
        printk("mxlk_probe() for function idx=%x start\n",func);
	if (func > 7)
	{
		printk("return incomplete mxlk_probe() for function idx=%x\n",func);
		return 0; // for now only probe for function 0 ;
	}

	xdev = kzalloc(sizeof(struct mxlk_pcie), GFP_KERNEL);
 	if (!xdev)
		return -ENOMEM;

	ret = mxlk_pci_init(xdev, pdev);
	if (ret)
		kfree(xdev);

        printk("mxlk_probe() for function idx=%x end\n",func);
	return ret;
}

static void mxlk_remove(struct pci_dev *pdev)
{
	struct mxlk_pcie *xdev = pci_get_drvdata(pdev);
	if (xdev) {
		mxlk_pci_cleanup(xdev);
		kfree(xdev);
	}
}

static struct pci_driver mxlk_driver = {
	.name = MXLK_DRIVER_NAME,
	.id_table = mxlk_pci_table,
	.probe = mxlk_probe,
	.remove = mxlk_remove
};

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
MODULE_VERSION(MXLK_DRIVER_VERSION);
