/*
 * Copyright(c) 2013-2014 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/uaccess.h>
#include <linux/fcntl.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include "nd-private.h"
#include "nfit.h"

static int nd_major;
static struct class *nd_class;

static const char *nfit_desc_provider(struct device *parent,
		struct nfit_bus_descriptor *nfit_desc)
{
	if (nfit_desc->provider_name)
		return nfit_desc->provider_name;
	else if (parent)
		return dev_name(parent);
	else
		return "unknown";
}

static ssize_t provider_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nd_bus *nd_bus = to_nd_bus(dev->parent);

	return sprintf(buf, "%s\n", nfit_desc_provider(nd_bus->dev.parent,
				nd_bus->nfit_desc));
}
static DEVICE_ATTR_RO(provider);

static ssize_t format_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nd_bus *nd_bus = to_nd_bus(dev->parent);

	return sprintf(buf, "%d\n", nd_bus->format_interface_code);
}
static DEVICE_ATTR_RO(format);

static ssize_t revision_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nd_bus *nd_bus = to_nd_bus(dev->parent);
	struct nfit __iomem *nfit = nd_bus->nfit_desc->nfit_base;

	return sprintf(buf, "%d\n", readb(&nfit->revision));
}
static DEVICE_ATTR_RO(revision);

static struct attribute *nd_bus_attributes[] = {
	&dev_attr_provider.attr,
	&dev_attr_format.attr,
	&dev_attr_revision.attr,
	NULL,
};

static umode_t nd_bus_attr_visible(struct kobject *kobj, struct attribute *a, int n)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct nd_bus *nd_bus = to_nd_bus(dev->parent);

	/*
	 * if core and the bus descriptor don't agree on a format
	 * interface, hide the 'format' attribute, control messages will
	 * be disabled.
	 */
	if (a == &dev_attr_format.attr && !nd_bus->format_interface_code)
		return 0;
	return a->mode;
}

static struct attribute_group nd_bus_attribute_group = {
	.is_visible = nd_bus_attr_visible,
	.attrs = nd_bus_attributes,
};

static const struct attribute_group *nd_bus_attribute_groups[] = {
	&nd_bus_attribute_group,
	NULL,
};

int nd_bus_create_ndctl(struct nd_bus *nd_bus)
{
	dev_t devt = MKDEV(nd_major, nd_bus->id);
	struct device *dev;

	dev = device_create_with_groups(nd_class, &nd_bus->dev, devt, nd_bus,
			nd_bus_attribute_groups, "ndctl%d", nd_bus->id);

	if (IS_ERR(dev)) {
		dev_dbg(&nd_bus->dev, "failed to register ndctl%d: %ld\n",
				nd_bus->id, PTR_ERR(dev));
		return PTR_ERR(dev);
	}
	return 0;
}

void nd_bus_destroy_ndctl(struct nd_bus *nd_bus)
{
	device_destroy(nd_class, MKDEV(nd_major, nd_bus->id));
}

static long nd_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return -ENXIO;
}

static const struct file_operations nd_bus_fops = {
	.owner = THIS_MODULE,
	.open = nonseekable_open,
	.unlocked_ioctl = nd_ioctl,
	.compat_ioctl = nd_ioctl,
	.llseek = noop_llseek,
};

int __init nd_bus_init(void)
{
	int rc;

	rc = register_chrdev(0, "ndctl", &nd_bus_fops);
	if (rc < 0)
		return rc;
	nd_major = rc;

	nd_class = class_create(THIS_MODULE, "nd_bus");
	if (IS_ERR(nd_class))
		goto err_class;

	return 0;

 err_class:
	unregister_chrdev(nd_major, "ndctl");

	return rc;
}

void __exit nd_bus_exit(void)
{
	class_destroy(nd_class);
	unregister_chrdev(nd_major, "ndctl");
}
