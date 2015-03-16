/*
 * Copyright (c) 2013 Intel Corporation. All rights reserved.
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

#include "hfi.h"
#include "device.h"

static struct class *class;
static dev_t hfi_dev;

int hfi_cdev_init(int minor, const char *name,
	      const struct file_operations *fops,
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

	device = device_create(class, NULL, dev, NULL, "%s", name);
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

static const char *hfi_class_name = "hfi";

const char *class_name(void)
{
	return hfi_class_name;
}

int __init dev_init(void)
{
	int ret;

	ret = alloc_chrdev_region(&hfi_dev, 0, HFI_NMINORS, DRIVER_NAME);
	if (ret < 0) {
		pr_err("Could not allocate chrdev region (err %d)\n", -ret);
		goto done;
	}

	class = class_create(THIS_MODULE, class_name());
	if (IS_ERR(class)) {
		ret = PTR_ERR(class);
		pr_err("Could not create device class (err %d)\n", -ret);
		unregister_chrdev_region(hfi_dev, HFI_NMINORS);
	}

done:
	return ret;
}

void dev_cleanup(void)
{
	if (class) {
		class_destroy(class);
		class = NULL;
	}

	unregister_chrdev_region(hfi_dev, HFI_NMINORS);
}
