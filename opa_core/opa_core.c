/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2014 - 2015 Intel Corporation.
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
 * Copyright (c) 2014 - 2015 Intel Corporation.
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
 *
 * Intel(R) Omni-Path Core Driver
 */

#include <linux/slab.h>
#include <linux/module.h>
#include <linux/idr.h>
#include <rdma/opa_core.h>

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

static int opa_add_dev(struct device *dev, struct subsys_interface *si)
{
	struct opa_core_client *client =
		container_of(si, struct opa_core_client, si);
	struct opa_core_device *odev = dev_to_opa_core(dev);

	return client->add(odev);
}

static int opa_remove_dev(struct device *dev, struct subsys_interface *si)
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
	si->add_dev = opa_add_dev;
	si->remove_dev = opa_remove_dev;

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
	kfree(odev);
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
MODULE_DESCRIPTION("Intel(R) Omni-Path Core Driver");
