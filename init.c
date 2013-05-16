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

#include <linux/netdevice.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/aer.h>

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

/*
 * In case we do not get enough MSI-X interrupts, we need to split
 * them between the two HFIs.  The first HFI will aquire the mutex
 * and acquire interrupts.  If it is not able to get all it wanted
 * it will halve the number it is able to acuire and write that
 * amount here.  The second HFI will see the count and acquire the
 * the same amount.
 *
 * Issues:
 * o This leads to asymmetric performance if the first is able to acquire
 *   all it wanted but the second is not.
 */
static DEFINE_MUTEX(msix_mutex);
#define MSIX_FIRST -1		/* no one has allocated MSI-X yet */
#define MSIX_SECOND -2		/* first HFI was able to allocate all */
static int msix_count = MSIX_FIRST;

static int allocate_msix_interrupts(struct hfi_devdata *dd)
{
	int i;
	int loop;
	int ret;

	/* 1-1 MSI-X entry assignment */
	for (i = 0; i < WFR_NUM_MSIX; i++)
		dd->msix[i].entry = i;

	/* always ask for everything */
	dd->nmsi = WFR_NUM_MSIX;

	loop = 0;
	mutex_lock(&msix_mutex);
	while (1) {
		/* returns 0 on success, < 0 on error, or N available */
		ret = pci_enable_msix(dd->pdev, &dd->msix[0], dd->nmsi);
		if (ret < 0) {
			dd->nmsi = 0;
			break;
		}
		if (ret == 0) {
			if (msix_count == MSIX_FIRST)
				msix_count = MSIX_SECOND;
			break;
		}
		/*
		 * Did not acquire the number of MSI-X interruptes we wanted.
		 *
		 * Explanation for each if below:
		 * o If this is the first device, then half the number
		 *   available and save it for the second device.
		 * o If the first device already allocated the full amount,
		 *   take as many as are available.
		 * o If this is the first time through the loop for for the
		 *   second device, request the same as the first device
		 *   requested.
		 * o Otherwise ask for as many as you can get.
		 *
		 * It is expected that we will loop at most twice.  If
		 * for some reason this loops more than that, then we
		 * will always be requesting the number returned, which
		 * should succeed.
		 *
		 * TODO: if there is only one device then we should request
		 * all available, rather than half.
		 */
		if (msix_count == MSIX_FIRST)
			msix_count = dd->nmsi = ret/2;
		else if (msix_count == MSIX_SECOND)
			dd->nmsi = ret;
		else if (loop == 0)
			dd->nmsi = msix_count;
		else
			dd->nmsi = ret;

		loop++;
	}
	mutex_unlock(&msix_mutex);

	if (ret < 0) {
		dev_err(&dd->pdev->dev,
			"cannot allocate any MSI-X vectors, err %d\n", ret);
	} else if (dd->nmsi != WFR_NUM_MSIX) {
		dev_err(&dd->pdev->dev,
			"allocated %d of %d MSI-X interrupts\n",
			dd->nmsi, WFR_NUM_MSIX);
	}

	return ret;
}

/* returns 0 on success, -ERRNO on failure */
static int pci_enable(struct hfi_devdata *dd)
{
	int consistent = 0;
	int requested_region = 0;
	int ret;

	ret = pci_enable_device(dd->pdev);
	if (ret) {
		dev_err(&dd->pdev->dev, "cannot enable device, err %d\n", ret);
		return ret;
	}

	/* map BAR0 */
	ret = pci_request_region(dd->pdev, 0, HFI_DRIVER_NAME);
	if (ret) {
		dev_err(&dd->pdev->dev, "cannot request region, err %d\n", ret);
		goto request_region_fail;
	}
	requested_region = 1;

	dd->bar0 = pci_resource_start(dd->pdev, 0);
	dd->bar0len = pci_resource_len(dd->pdev, 0);

	/* TODO: does this include the write combining space? */
	dd->kregbase = ioremap_nocache(dd->bar0, dd->bar0len);
	if (!dd->kregbase) {
		dev_err(&dd->pdev->dev, "cannot ioremap registers\n");
		ret = -ENOMEM;
		goto ioremap_fail;
	}
	dd->kregend = dd->kregbase + dd->bar0len;

	ret = pci_set_dma_mask(dd->pdev, DMA_BIT_MASK(64));
	if (!ret) {
		consistent = 1;
		ret = pci_set_consistent_dma_mask(dd->pdev, DMA_BIT_MASK(64));
	}
	if (ret) {
		dev_err(&dd->pdev->dev, "cannot set up %sDMA mask, err %d\n",
			consistent ? "consistent " : "", ret);
		goto dma_mask_fail;
	}

	pci_set_master(dd->pdev);
	ret = pci_enable_pcie_error_reporting(dd->pdev);
	if (ret) {
		dev_err(&dd->pdev->dev,
			"cannot set up error reporting, err %d\n", ret);
		goto error_reporting_fail;
	}

	ret = allocate_msix_interrupts(dd);
	if (ret < 0)
		goto msix_fail;

	return 0;

msix_fail:
	if (dd->nmsi > 0)
		pci_disable_msix(dd->pdev);
error_reporting_fail:
	;	/* nothing to clean up for dma mask */
dma_mask_fail:
	iounmap(dd->kregbase);
	dd->kregbase = NULL;
ioremap_fail:
	; /* release region should happen after the device is disabled */
request_region_fail:
	pci_disable_device(dd->pdev);
	if (requested_region)
		pci_release_region(dd->pdev, 0);

	return ret;
}

static void pci_release(struct hfi_devdata *dd)
{
	if (dd->nmsi > 0)
		pci_disable_msix(dd->pdev);
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

	dev_info(&pdev->dev, "mapped at 0x%p\n", dd->kregbase);

	ret = load_device_firmware(dd);
	if (ret)
		goto load_firmware_failed;

	ret = hfi_device_create(dd);
	if (ret)
		goto device_failed;

	ret = register_ib_device(dd);
	if (ret) goto register_ib_failed;


	return 0; /* success */

	/*unregister_ib_device(dd);*/
register_ib_failed:
	hfi_device_remove(dd);
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

	dev_info(&pdev->dev, "removed\n");

	unregister_ib_device(dd);
	hfi_device_remove(dd);
	unload_device_firmware(dd);
	pci_release(dd);
	pci_set_drvdata(pdev, NULL);
	release_devnum(dd);
	kfree(dd);
}

static pci_ers_result_t hfi_pci_error_detected(struct pci_dev *pdev,
				pci_channel_state_t state)
{
	/* TODO: implement */
	dev_err(&pdev->dev, "%s unimplemented\n", __func__);
	return PCI_ERS_RESULT_RECOVERED;
}

static pci_ers_result_t hfi_pci_mmio_enabled(struct pci_dev *pdev)
{
	/* TODO: implement */
	dev_err(&pdev->dev, "%s unimplemented\n", __func__);
	return PCI_ERS_RESULT_RECOVERED;
}

static pci_ers_result_t hfi_pci_slot_reset(struct pci_dev *pdev)
{
	/* TODO: implement */
	dev_err(&pdev->dev, "%s unimplemented\n", __func__);
	return PCI_ERS_RESULT_CAN_RECOVER;
}

static pci_ers_result_t hfi_pci_link_reset(struct pci_dev *pdev)
{
	/* TODO: implement */
	dev_err(&pdev->dev, "%s unimplemented\n", __func__);
	return PCI_ERS_RESULT_CAN_RECOVER;
}

static void hfi_pci_resume(struct pci_dev *pdev)
{
	/* TODO: implement */
	dev_err(&pdev->dev, "%s unimplemented\n", __func__);
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
