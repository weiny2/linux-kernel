/*
 * Copyright (c) 2013 - 2014 Intel Corporation. All rights reserved.
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

#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include "../common/hfi.h"
#include "device.h"

static struct stl_core_device_id id_table[] = {
	{ PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_FXR0 },
	{ 0 },
};
MODULE_DEVICE_TABLE(stl_core, id_table);

static int hfi_portals_probe(struct stl_core_device *hdev);
static void hfi_portals_remove(struct stl_core_device *hdev);

static struct stl_core_driver hfi_portals_driver = {
	.driver.name = KBUILD_MODNAME,
	.driver.owner = THIS_MODULE,
	.id_table = id_table,
	.probe = hfi_portals_probe,
	.remove = hfi_portals_remove,
};

static dev_t hfi_dev;

int hfi_cdev_init(int minor, const char *name,
		  const struct file_operations *fops,
		  struct class *class,
		  struct cdev *cdev, struct device **devp)
{
	const dev_t dev = MKDEV(MAJOR(hfi_dev), minor);
	struct device *device = NULL;
	int ret;

	cdev_init(cdev, fops);
	cdev->owner = THIS_MODULE;
	kobject_set_name(&cdev->kobj, name);

	ret = cdev_add(cdev, dev, 1);
	if (ret < 0) {
		pr_err("Could not add cdev for minor %d, %s (err %d)\n",
		       minor, name, -ret);
		goto done;
	}

	device = device_create(class, NULL, dev, NULL, name);
	if (!IS_ERR(device))
		goto done;
	ret = PTR_ERR(device);
	device = NULL;
	pr_err("Could not create device for minor %d, %s (err %d)\n",
	       minor, name, -ret);
	cdev_del(cdev);
done:
	*devp = device;
	return ret;
}

void hfi_cdev_cleanup(struct cdev *cdev, struct device **devp)
{
	struct device *device = *devp;

	if (device) {
		device_unregister(device);
		*devp = NULL;

		cdev_del(cdev);
	}
}

/*
 * Device initialization, called from PCI probe.
 */
static int hfi_portals_probe(struct stl_core_device *hdev)
{
	int ret;

	hfi_portals_driver.bus_dev = hdev;
	ret = hfi_user_add(&hfi_portals_driver, hdev->unit);
	if (ret)
		pr_err("Failed to create /dev devices: %d\n", -ret);

	return 0;
}

/*
 * Perform required device shutdown logic, also remove /dev entries.
 * Called when unloading the driver.
 */
static void hfi_portals_remove(struct stl_core_device *hdev)
{
	hfi_user_remove(&hfi_portals_driver);
}

int __init hfi_init(void)
{
	int ret;

	ret = alloc_chrdev_region(&hfi_dev, 0, HFI_NMINORS, DRIVER_NAME);
	if (ret < 0) {
		pr_err("Could not allocate chrdev region (err %d)\n", -ret);
		goto done;
	}

	ret = stl_core_register_driver(&hfi_portals_driver);
	if (ret < 0)
		unregister_chrdev_region(hfi_dev, HFI_NMINORS);
done:
	return ret;
}
module_init(hfi_init);

void hfi_cleanup(void)
{
	stl_core_unregister_driver(&hfi_portals_driver);
	unregister_chrdev_region(hfi_dev, HFI_NMINORS);
}
module_exit(hfi_cleanup);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Intel Corporation");
MODULE_DESCRIPTION("Intel(R) STL Gen2 Portals Driver");
MODULE_VERSION(HFI_DRIVER_VERSION);
