/* TODO: copyright OK?  from qib_init.c */
/*
 * Copyright (c) 2013 Intel Corporation. All rights reserved.
 * Copyright (c) 2006, 2007, 2008, 2009, 2010 QLogic Corporation.
 * All rights reserved.
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
#include <linux/netdevice.h>
#include <linux/io.h>
#include <linux/module.h>

#include "hfi.h"

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Intel <ibsupport@intel.com>");
MODULE_DESCRIPTION("Intel HFI driver");
MODULE_VERSION(HFI_DRIVER_VERSION);


static DEFINE_PCI_DEVICE_TABLE(hfi_pci_tbl) = {
	{ PCI_DEVICE(0x8086, 0x7323) },
	{ 0, }
};

MODULE_DEVICE_TABLE(pci, hfi_pci_tbl);


/*
 * Simple, stupid device number allocator.  Cannot reuse numbers once
 * allocated.
 * TODO: Too much?  Too little?
 */
static unsigned int devnum_counter;
static void allocate_devnum(struct hfi_devdata *dd)
{
	dd->devnum = devnum_counter++;
}

static void release_devnum(struct hfi_devdata *dd)
{
}

/* returns 0 on success, -ERRNO on failure */
static int pci_enable(struct hfi_devdata *dd)
{
	int ret;

	ret = pci_enable_device(dd->pdev);
	if (ret) {
		pr_err("%s: cannot enable device\n", HFI_DRIVER_NAME);
		return ret;
	}

	/* map BAR0 */
	ret = pci_request_region(dd->pdev, 0, "hfi_driver");
	if (ret) {
		pr_err("%s: cannot request region\n", HFI_DRIVER_NAME);
		pci_disable_device(dd->pdev);
		return ret;
	}

	dd->bar0 = pci_resource_start(dd->pdev, 0);
	dd->bar0len = pci_resource_len(dd->pdev, 0);

	/* TODO: does this include the write combining space? */
	dd->kregbase = ioremap_nocache(dd->bar0, dd->bar0len);
	if (!dd->kregbase) {
		pr_err("%s: cannot ioremap registers\n", HFI_DRIVER_NAME);
		pci_disable_device(dd->pdev);
		/* after pci_disable_device */
		pci_release_region(dd->pdev, 0);
		return -ENOMEM;
	}
	dd->kregend = dd->kregbase + dd->bar0len;

	return 0;
}

static void pci_release(struct hfi_devdata *dd)
{
	/* before pci_disable_device */
	if (dd->kregbase) {
		iounmap(dd->kregbase);
		dd->kregbase = NULL;
		dd->kregend = NULL;
	}

	pci_disable_device(dd->pdev);

	/* after pci_disable_device */
	pci_release_region(dd->pdev, 0);
}

static struct hfi_devdata *allocate_devdata(void)
{
	struct hfi_devdata *dd;

	dd = kzalloc(sizeof(*dd), GFP_KERNEL);
	if (dd)
		dd->devnum = -1;	/* invalid device number */

	return dd;
}

static int hfi_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	struct hfi_devdata *dd;
	int ret = 0;

	dd = allocate_devdata();
	if (!dd)
		return -ENOMEM;

	dd->pdev = pdev;
	pci_set_drvdata(pdev, dd);

	allocate_devnum(dd);

	ret = pci_enable(dd);
	if (ret)
		goto pci_enable_failed;

	pr_info("%s: device %d:%d.%d mapped at 0x%lx\n", HFI_DRIVER_NAME,
		pdev->bus->number, PCI_SLOT(pdev->devfn), PCI_FUNC(pdev->devfn),
		(unsigned long)dd->kregbase);

	ret = load_device_firmware(dd);
	if (ret)
		goto load_firmware_failed;

	ret = hfi_device_create(dd);
	if (ret)
		goto device_failed;

	return 0; /* success */
/*
Uncomment this to undo a successful hfi_device_create().
	hfi_device_remove(dd);
*/
device_failed:
	unload_device_firmware(dd);
load_firmware_failed:
	pci_release(dd);
pci_enable_failed:
	release_devnum(dd);
	pci_set_drvdata(pdev, NULL);
	kfree(dd);
	return ret;
}

static void hfi_remove(struct pci_dev *pdev)
{
	struct hfi_devdata *dd = pci_get_drvdata(pdev);

	hfi_device_remove(dd);
	unload_device_firmware(dd);
	pci_release(dd);
	pci_set_drvdata(pdev, NULL);
	release_devnum(dd);
	kfree(dd);

	pr_info("%s: removed device %d:%d.%d\n", HFI_DRIVER_NAME,
		pdev->bus->number, PCI_SLOT(pdev->devfn),
		PCI_FUNC(pdev->devfn));
}

static pci_ers_result_t hfi_pci_error_detected(struct pci_dev *pdev,
				pci_channel_state_t state)
{
	/* TODO: implement */
	pr_err("%s: unimplemented\n", __func__);
	return PCI_ERS_RESULT_RECOVERED;
}

static pci_ers_result_t hfi_pci_mmio_enabled(struct pci_dev *pdev)
{
	/* TODO: implement */
	pr_err("%s: unimplemented\n", __func__);
	return PCI_ERS_RESULT_RECOVERED;
}

static pci_ers_result_t hfi_pci_slot_reset(struct pci_dev *pdev)
{
	/* TODO: implement */
	pr_err("%s: unimplemented\n", __func__);
	return PCI_ERS_RESULT_CAN_RECOVER;
}

static pci_ers_result_t hfi_pci_link_reset(struct pci_dev *pdev)
{
	/* TODO: implement */
	pr_err("%s: unimplemented\n", __func__);
	return PCI_ERS_RESULT_CAN_RECOVER;
}

static void hfi_pci_resume(struct pci_dev *pdev)
{
	/* TODO: implement */
	pr_err("%s: unimplemented\n", __func__);
}

static struct pci_error_handlers hfi_pci_err_handler = {
	.error_detected = hfi_pci_error_detected,
	.mmio_enabled = hfi_pci_mmio_enabled,
	.link_reset = hfi_pci_link_reset,
	.slot_reset = hfi_pci_slot_reset,
	.resume = hfi_pci_resume,
};

static struct pci_driver hfi_driver = {
	.name = HFI_DRIVER_NAME,
	.probe = hfi_probe,
	.remove = hfi_remove,
	.id_table = hfi_pci_tbl,
	.err_handler = &hfi_pci_err_handler,
};

static int __init hfi_init(void)
{
	int ret;

	pr_info("%s: loading Intel HFI driver, v%s\n",
		HFI_DRIVER_NAME, HFI_DRIVER_VERSION);

	/* devices must be initialized before pci probe */
	ret = device_file_init(); /* error message printed on failure */
	if (ret < 0)
		goto fail_dev;

	ret = pci_register_driver(&hfi_driver);
	if (ret < 0) {
		pr_err("%s: Unable to register driver: error %d\n",
			HFI_DRIVER_NAME, -ret);
		goto fail_pci;
	}

	return 0; /* all OK */

#if 0
fail_xxx:
	pci_unregister_driver(&hfi_driver);
#endif

fail_pci:
	device_file_cleanup();
fail_dev:
	return ret;
}
module_init(hfi_init);

static void __exit hfi_exit(void)
{
	pci_unregister_driver(&hfi_driver);
	device_file_cleanup();
	pr_info("%s: unloading Intel HFI driver, v%s\n",
		HFI_DRIVER_NAME, HFI_DRIVER_VERSION);
}
module_exit(hfi_exit);
