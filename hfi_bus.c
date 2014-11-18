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
#include "hfi_bus.h"

static struct class *dev_class;

/* Unique numbering for hfi_bus devices. */
static DEFINE_IDA(hfi_bus_index_ida);

static ssize_t device_show(struct device *d,
			   struct device_attribute *attr, char *buf)
{
	struct hfi_bus_device *dev = dev_to_hfi_bus(d);
	return sprintf(buf, "0x%04x\n", dev->id.device);
}
static DEVICE_ATTR_RO(device);

static ssize_t vendor_show(struct device *d,
			   struct device_attribute *attr, char *buf)
{
	struct hfi_bus_device *dev = dev_to_hfi_bus(d);
	return sprintf(buf, "0x%04x\n", dev->id.vendor);
}
static DEVICE_ATTR_RO(vendor);

static ssize_t modalias_show(struct device *d,
			     struct device_attribute *attr, char *buf)
{
	struct hfi_bus_device *dev = dev_to_hfi_bus(d);
	return sprintf(buf, "hfi_bus:d%08Xv%08X\n",
		       dev->id.device, dev->id.vendor);
}
static DEVICE_ATTR_RO(modalias);

static struct attribute *hfi_bus_dev_attrs[] = {
	&dev_attr_device.attr,
	&dev_attr_vendor.attr,
	&dev_attr_modalias.attr,
	NULL,
};
ATTRIBUTE_GROUPS(hfi_bus_dev);

static inline int hfi_bus_id_match(const struct hfi_bus_device *dev,
				  const struct hfi_bus_device_id *id)
{
	return (id->vendor == dev->id.vendor) &&
	       (id->device == dev->id.device);
}

/* This looks through all the IDs a driver claims to support.  If any of them
 * match, we return 1 and the kernel will call hfi_bus_dev_probe(). */
static int hfi_bus_dev_match(struct device *d, struct device_driver *dr)
{
	unsigned int i;
	struct hfi_bus_device *dev = dev_to_hfi_bus(d);
	const struct hfi_bus_device_id *ids;

	ids = drv_to_hfi_bus(dr)->id_table;
	for (i = 0; ids[i].device; i++)
		if (hfi_bus_id_match(dev, &ids[i]))
			return 1;
	return 0;
}

static int hfi_bus_uevent(struct device *d, struct kobj_uevent_env *env)
{
	struct hfi_bus_device *dev = dev_to_hfi_bus(d);

	return add_uevent_var(env, "MODALIAS=hfi_bus:d%08Xv%08X",
			      dev->id.device, dev->id.vendor);
}

static int hfi_bus_dev_probe(struct device *d)
{
	int err;
	struct hfi_bus_device *dev = dev_to_hfi_bus(d);
	struct hfi_bus_driver *drv = drv_to_hfi_bus(dev->dev.driver);

	pr_info("bus_probe dev[%d] and drv %s\n",
		dev->index, drv->driver.name);

	err = drv->probe(dev);
	if (!err)
		if (drv->scan)
			drv->scan(dev);

	return err;
}

static int hfi_bus_dev_remove(struct device *d)
{
	struct hfi_bus_device *dev = dev_to_hfi_bus(d);
	struct hfi_bus_driver *drv = drv_to_hfi_bus(dev->dev.driver);

	drv->remove(dev);

	/* Driver should have reset device. */
	//WARN_ON_ONCE(dev->config->get_status(dev));
	return 0;
}

static struct bus_type hfi_bus = {
	.name  = "hfi_bus",
	.match = hfi_bus_dev_match,
	.dev_groups = hfi_bus_dev_groups,
	.uevent = hfi_bus_uevent,
	.probe = hfi_bus_dev_probe,
	.remove = hfi_bus_dev_remove,
};

int hfi_bus_register_driver(struct hfi_bus_driver *driver)
{
	driver->class = dev_class;
	driver->driver.bus = &hfi_bus;
	return driver_register(&driver->driver);
}
EXPORT_SYMBOL(hfi_bus_register_driver);

void hfi_bus_unregister_driver(struct hfi_bus_driver *driver)
{
	driver_unregister(&driver->driver);
}
EXPORT_SYMBOL(hfi_bus_unregister_driver);

static void hfi_bus_release_device(struct device *d)
{
	struct hfi_bus_device *hdev = dev_to_hfi_bus(d);
	kfree(hdev);
}

struct hfi_bus_device *
hfi_bus_register_device(struct device *dev, struct hfi_bus_device_id *bus_id,
			struct hfi_devdata *dd, struct hfi_bus_ops *bus_ops)
{
	int ret;
	struct hfi_bus_device *hfi_dev;

	hfi_dev = kzalloc(sizeof(*hfi_dev), GFP_KERNEL);
	if (!hfi_dev)
		return ERR_PTR(-ENOMEM);

	/* Assign a unique device index and hence name. */
	ret = ida_simple_get(&hfi_bus_index_ida, 0, 0, GFP_KERNEL);
	if (ret < 0)
		goto out;
	pr_info("bus_register_dev [%d] %x %x\n", ret, bus_id->vendor, bus_id->device);

	hfi_dev->index = ret;
	/* TODO - should set name, id, bus, bus_id */
	dev_set_name(&hfi_dev->dev, "hfi_bus%u", hfi_dev->index);
	hfi_dev->dev.bus = &hfi_bus;
	hfi_dev->dev.parent = dev;
	hfi_dev->dev.release = hfi_bus_release_device;
	hfi_dev->id = *bus_id;
	hfi_dev->dd = dd;
	hfi_dev->unit = dd->unit;
	hfi_dev->bus_ops = bus_ops;

	/* We always start by resetting the device, in case a previous
	 * driver messed it up.  This also tests that code path a little. */
	//hfi_dev->config->reset(hfi_dev);

	/*
	 * device_register() causes the bus infrastructure to look for a
	 * matching driver.
	 */
	ret = device_register(&hfi_dev->dev);
	if (ret)
		goto ida_remove;
	return hfi_dev;

ida_remove:
	ida_simple_remove(&hfi_bus_index_ida, hfi_dev->index);
out:
	return ERR_PTR(ret);
}
EXPORT_SYMBOL(hfi_bus_register_device);

void hfi_bus_unregister_device(struct hfi_bus_device *hfi_dev)
{
	int index = hfi_dev->index; /* save for after device release */

	device_unregister(&hfi_dev->dev);
	ida_simple_remove(&hfi_bus_index_ida, index);
}
EXPORT_SYMBOL(hfi_bus_unregister_device);

static int __init hfi_bus_init(void)
{
	int ret;

	dev_class = class_create(THIS_MODULE, DRIVER_CLASS_NAME);
	if (IS_ERR(dev_class)) {
		ret = PTR_ERR(dev_class);
		pr_err("Could not create device dev_class (err %d)\n", -ret);
		goto err_dev_class;
	}

	ret = bus_register(&hfi_bus);
	if (ret)
		class_destroy(dev_class);

err_dev_class:
	return ret;
}

static void __exit hfi_bus_exit(void)
{
	bus_unregister(&hfi_bus);
	class_destroy(dev_class);
}
core_initcall(hfi_bus_init);
module_exit(hfi_bus_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Intel Corporation");
MODULE_DESCRIPTION("Intel(R) STL Gen2 Bus Driver");
MODULE_VERSION(HFI_DRIVER_VERSION);
