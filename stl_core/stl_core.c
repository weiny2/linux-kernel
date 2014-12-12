/*
 * Copyright(c) 2014 Intel Corporation.
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

#include <linux/slab.h>
#include <linux/module.h>
#include <linux/idr.h>
#include "../common/stl_core.h"

/* Unique numbering for stl_core devices. */
static DEFINE_IDA(stl_core_index_ida);

static ssize_t device_show(struct device *d,
			   struct device_attribute *attr, char *buf)
{
	struct stl_core_device *dev = dev_to_stl_core(d);

	return sprintf(buf, "0x%04x\n", dev->id.device);
}
static DEVICE_ATTR_RO(device);

static ssize_t vendor_show(struct device *d,
			   struct device_attribute *attr, char *buf)
{
	struct stl_core_device *dev = dev_to_stl_core(d);

	return sprintf(buf, "0x%04x\n", dev->id.vendor);
}
static DEVICE_ATTR_RO(vendor);

static ssize_t modalias_show(struct device *d,
			     struct device_attribute *attr, char *buf)
{
	struct stl_core_device *dev = dev_to_stl_core(d);

	return sprintf(buf, "stl_core:d%08Xv%08X\n",
		       dev->id.device, dev->id.vendor);
}
static DEVICE_ATTR_RO(modalias);

static struct attribute *stl_core_dev_attrs[] = {
	&dev_attr_device.attr,
	&dev_attr_vendor.attr,
	&dev_attr_modalias.attr,
	NULL,
};
ATTRIBUTE_GROUPS(stl_core_dev);

static int stl_add_dev(struct device *dev, struct subsys_interface *si)
{
	struct stl_core_client *client =
		container_of(si, struct stl_core_client, si);
	struct stl_core_device *sdev = dev_to_stl_core(dev);

	return client->add(sdev);
}

static int stl_remove_dev(struct device *dev, struct subsys_interface *si)
{
	struct stl_core_client *client =
		container_of(si, struct stl_core_client, si);
	struct stl_core_device *sdev = dev_to_stl_core(dev);

	client->remove(sdev);
	return 0;
}

static struct bus_type stl_core = {
	.name  = "stl_core",
	.dev_groups = stl_core_dev_groups,
};

int stl_core_client_register(struct stl_core_client *client)
{
	struct subsys_interface *si = &client->si;

	si->name = client->name;
	si->subsys = &stl_core;
	si->add_dev = stl_add_dev;
	si->remove_dev = stl_remove_dev;

	return subsys_interface_register(&client->si);
}
EXPORT_SYMBOL(stl_core_client_register);

void stl_core_client_unregister(struct stl_core_client *client)
{
	subsys_interface_unregister(&client->si);
}
EXPORT_SYMBOL(stl_core_client_unregister);

static void stl_core_release_device(struct device *d)
{
	struct stl_core_device *hdev = dev_to_stl_core(d);

	kfree(hdev);
}

struct stl_core_device *
stl_core_register_device(struct device *dev, struct stl_core_device_id *bus_id,
			 struct hfi_devdata *dd, struct stl_core_ops *bus_ops)
{
	int ret;
	struct stl_core_device *sdev;

	sdev = kzalloc(sizeof(*sdev), GFP_KERNEL);
	if (!sdev)
		return ERR_PTR(-ENOMEM);

	/* Assign a unique device index and hence name. */
	ret = ida_simple_get(&stl_core_index_ida, 0, 0, GFP_KERNEL);
	if (ret < 0)
		goto out;
	pr_info("bus_register_dev [%d] %x %x\n",
		ret, bus_id->vendor, bus_id->device);

	sdev->index = ret;
	/* TODO - should set name, id, bus, bus_id */
	dev_set_name(&sdev->dev, "stl_core%u", sdev->index);
	sdev->dev.bus = &stl_core;
	sdev->dev.parent = dev;
	sdev->dev.release = stl_core_release_device;
	sdev->id = *bus_id;
	sdev->dd = dd;
	sdev->bus_ops = bus_ops;

	/*
	 * device_register() causes the bus infrastructure to look for a
	 * matching driver.
	 */
	ret = device_register(&sdev->dev);
	if (ret)
		goto ida_remove;
	return sdev;

ida_remove:
	ida_simple_remove(&stl_core_index_ida, sdev->index);
out:
	return ERR_PTR(ret);
}
EXPORT_SYMBOL(stl_core_register_device);

void stl_core_unregister_device(struct stl_core_device *sdev)
{
	int index = sdev->index; /* save for after device release */

	device_unregister(&sdev->dev);
	ida_simple_remove(&stl_core_index_ida, index);
}
EXPORT_SYMBOL(stl_core_unregister_device);

static int __init stl_core_init(void)
{
	return bus_register(&stl_core);
}

static void __exit stl_core_exit(void)
{
	bus_unregister(&stl_core);
	ida_destroy(&stl_core_index_ida);
}
core_initcall(stl_core_init);
module_exit(stl_core_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Intel Corporation");
MODULE_DESCRIPTION("Intel(R) STL Gen2 Bus Driver");
MODULE_VERSION(HFI_DRIVER_VERSION);
