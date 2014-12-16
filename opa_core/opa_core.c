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
#include "../common/opa_core.h"

/* Unique numbering for opa_core devices. */
static DEFINE_IDA(opa_core_index_ida);

static ssize_t device_show(struct device *d,
			   struct device_attribute *attr, char *buf)
{
	struct opa_core_device *dev = dev_to_opa_core(d);

	return sprintf(buf, "0x%04x\n", dev->id.device);
}
static DEVICE_ATTR_RO(device);

static ssize_t vendor_show(struct device *d,
			   struct device_attribute *attr, char *buf)
{
	struct opa_core_device *dev = dev_to_opa_core(d);

	return sprintf(buf, "0x%04x\n", dev->id.vendor);
}
static DEVICE_ATTR_RO(vendor);

static ssize_t modalias_show(struct device *d,
			     struct device_attribute *attr, char *buf)
{
	struct opa_core_device *dev = dev_to_opa_core(d);

	return sprintf(buf, "opa_core:d%08Xv%08X\n",
		       dev->id.device, dev->id.vendor);
}
static DEVICE_ATTR_RO(modalias);

static struct attribute *opa_core_dev_attrs[] = {
	&dev_attr_device.attr,
	&dev_attr_vendor.attr,
	&dev_attr_modalias.attr,
	NULL,
};
ATTRIBUTE_GROUPS(opa_core_dev);

static int stl_add_dev(struct device *dev, struct subsys_interface *si)
{
	struct opa_core_client *client =
		container_of(si, struct opa_core_client, si);
	struct opa_core_device *odev = dev_to_opa_core(dev);

	return client->add(odev);
}

static int stl_remove_dev(struct device *dev, struct subsys_interface *si)
{
	struct opa_core_client *client =
		container_of(si, struct opa_core_client, si);
	struct opa_core_device *odev = dev_to_opa_core(dev);

	client->remove(odev);
	return 0;
}

static int opa_core_uevent(struct device *d, struct kobj_uevent_env *env)
{
	struct opa_core_device *dev = dev_to_opa_core(d);

	return add_uevent_var(env, "MODALIAS=opa_core:d%08Xv%08X",
			      dev->id.device, dev->id.vendor);
}

static struct bus_type opa_core = {
	.name  = KBUILD_MODNAME,
	.dev_groups = opa_core_dev_groups,
	.uevent = opa_core_uevent,
};

int opa_core_client_register(struct opa_core_client *client)
{
	struct subsys_interface *si = &client->si;

	si->name = client->name;
	si->subsys = &opa_core;
	si->add_dev = stl_add_dev;
	si->remove_dev = stl_remove_dev;

	return subsys_interface_register(&client->si);
}
EXPORT_SYMBOL(opa_core_client_register);

void opa_core_client_unregister(struct opa_core_client *client)
{
	subsys_interface_unregister(&client->si);
}
EXPORT_SYMBOL(opa_core_client_unregister);

static void opa_core_release_device(struct device *d)
{
	struct opa_core_device *hdev = dev_to_opa_core(d);

	kfree(hdev);
}

struct opa_core_device *
opa_core_register_device(struct device *dev, struct opa_core_device_id *bus_id,
			 struct hfi_devdata *dd, struct opa_core_ops *bus_ops)
{
	int ret;
	struct opa_core_device *odev;

	odev = kzalloc(sizeof(*odev), GFP_KERNEL);
	if (!odev)
		return ERR_PTR(-ENOMEM);

	/* Assign a unique device index and hence name. */
	ret = ida_simple_get(&opa_core_index_ida, 0, 0, GFP_KERNEL);
	if (ret < 0)
		goto out;
	pr_info("bus_register_dev [%d] %x %x\n",
		ret, bus_id->vendor, bus_id->device);

	odev->index = ret;
	/* TODO - should set name, id, bus, bus_id */
	dev_set_name(&odev->dev, "opa_core%u", odev->index);
	odev->dev.bus = &opa_core;
	odev->dev.parent = dev;
	odev->dev.release = opa_core_release_device;
	odev->id = *bus_id;
	odev->dd = dd;
	odev->bus_ops = bus_ops;

	/*
	 * device_register() causes the bus infrastructure to look for a
	 * matching driver.
	 */
	ret = device_register(&odev->dev);
	if (ret)
		goto ida_remove;
	return odev;

ida_remove:
	ida_simple_remove(&opa_core_index_ida, odev->index);
out:
	return ERR_PTR(ret);
}
EXPORT_SYMBOL(opa_core_register_device);

void opa_core_unregister_device(struct opa_core_device *odev)
{
	int index = odev->index; /* save for after device release */

	device_unregister(&odev->dev);
	ida_simple_remove(&opa_core_index_ida, index);
}
EXPORT_SYMBOL(opa_core_unregister_device);

static int __init opa_core_init(void)
{
	return bus_register(&opa_core);
}

static void __exit opa_core_exit(void)
{
	bus_unregister(&opa_core);
	ida_destroy(&opa_core_index_ida);
}
core_initcall(opa_core_init);
module_exit(opa_core_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Intel Corporation");
MODULE_DESCRIPTION("Intel(R) STL Gen2 Bus Driver");
MODULE_VERSION(HFI_DRIVER_VERSION);
