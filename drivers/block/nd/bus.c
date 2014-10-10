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
#include <linux/ndctl.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include "nd-private.h"
#include "nfit.h"

static int nd_major;
static struct class *nd_class;

struct bus_type nd_bus_type = {
	.name = "nd",
};

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

static int __nd_ioctl(struct nd_bus *nd_bus, int read_only, unsigned int cmd,
		unsigned long arg)
{
	struct nfit_bus_descriptor *nfit_desc = nd_bus->nfit_desc;
	void __user *p = (void __user *) arg;
	size_t buf_len = 0;
	void *buf = NULL;
	int rc;

	/*
	 * validate the command buffer according to
	 * format-interface-code1 expectations
	 */
	if (nd_bus->format_interface_code != 1)
		return -ENXIO;

	/* fail write commands (when read-only), or unknown commands */
	switch (cmd) {
	case NFIT_CMD_SCRUB: {
		struct nfit_cmd_scrub nfit_scrub;

		if (!access_ok(VERIFY_WRITE, p, sizeof(nfit_scrub)))
			return -EFAULT;
		if (copy_from_user(&nfit_scrub, p, sizeof(nfit_scrub)))
			return -EFAULT;
		/* might as well calculate the buf_len here */
		buf_len = sizeof(nfit_scrub) + nfit_scrub.out_length;

		if (read_only && nfit_scrub.cmd == NFIT_ARS_START)
			return -EPERM;
		else if (nfit_scrub.cmd == NFIT_ARS_QUERY)
			break;
		else
			return -EPERM;
		break;
	}
	case NFIT_CMD_SET_CONFIG_DATA:
	case NFIT_CMD_VENDOR:
		if (read_only)
			return -EPERM;
		/* fallthrough */
	case NFIT_CMD_SMART:
	case NFIT_CMD_GET_CONFIG_SIZE:
	case NFIT_CMD_GET_CONFIG_DATA:
		break;
	default:
		return -EPERM;
	}

	/* validate input buffer / determine size */
	switch (cmd) {
	case NFIT_CMD_SCRUB:
		if (WARN_ON(buf_len == 0))
			return -EPERM;
		break;
	case NFIT_CMD_SET_CONFIG_DATA: {
		struct nfit_cmd_set_config_hdr nfit_cmd_set;

		if (!access_ok(VERIFY_WRITE, p, sizeof(nfit_cmd_set)))
			return -EFAULT;
		if (copy_from_user(&nfit_cmd_set, p, sizeof(nfit_cmd_set)))
			return -EFAULT;
		/* include input buffer size and trailing status */
		buf_len = sizeof(nfit_cmd_set) + nfit_cmd_set.in_length + 4;
		break;
	}
	case NFIT_CMD_VENDOR: {
		struct nfit_cmd_vendor_hdr nfit_cmd_v;
		struct nfit_cmd_vendor_tail nfit_cmd_vt;

		if (!access_ok(VERIFY_WRITE, p, sizeof(nfit_cmd_v)))
			return -EFAULT;
		if (copy_from_user(&nfit_cmd_v, p, sizeof(nfit_cmd_v)))
			return -EFAULT;
		buf_len = sizeof(nfit_cmd_v) + nfit_cmd_v.in_length;
		if (!access_ok(VERIFY_WRITE, p + buf_len, sizeof(nfit_cmd_vt)))
			return -EFAULT;
		if (copy_from_user(&nfit_cmd_vt, p + buf_len,
					sizeof(nfit_cmd_vt)))
			return -EFAULT;
		buf_len += sizeof(nfit_cmd_vt) + nfit_cmd_vt.out_length;
		break;
	}
	case NFIT_CMD_SMART:
		buf_len = sizeof(struct nfit_cmd_smart);
		break;
	case NFIT_CMD_GET_CONFIG_SIZE:
		buf_len = sizeof(struct nfit_cmd_get_config_size);
		break;
	case NFIT_CMD_GET_CONFIG_DATA:
		buf_len = sizeof(struct nfit_cmd_get_config_data);
		break;
	default:
		return -EPERM;
	}

	if (!access_ok(VERIFY_WRITE, p, sizeof(buf_len)))
		return -EFAULT;

	if (buf_len > ND_IOCTL_MAX_BUFLEN)
		return -EINVAL;

	if (buf_len < KMALLOC_MAX_SIZE)
		buf = kmalloc(buf_len, GFP_KERNEL);

	if (!buf)
		buf = vmalloc(buf_len);

	if (!buf)
		return -ENOMEM;

	if (copy_from_user(buf, p, buf_len)) {
		rc = -EFAULT;
		goto out;
	}

	rc = nfit_desc->nfit_ctl(nfit_desc, cmd, buf, buf_len);
	if (rc)
		goto out;
	if (copy_to_user(p, buf, buf_len))
		rc = -EFAULT;
 out:
	if (is_vmalloc_addr(buf))
		vfree(buf);
	else
		kfree(buf);
	return rc;
}

static long nd_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long id = (long) file->private_data;
	int rc = -ENXIO, read_only;
	struct nd_bus *nd_bus;

	read_only = (O_RDWR != (file->f_flags & O_ACCMODE));
	mutex_lock(&nd_bus_list_mutex);
	list_for_each_entry(nd_bus, &nd_bus_list, list) {
		if (nd_bus->id == id) {
			rc = __nd_ioctl(nd_bus, read_only, cmd, arg);
			break;
		}
	}
	mutex_unlock(&nd_bus_list_mutex);

	return rc;
}

static int nd_open(struct inode *inode, struct file *file)
{
	long minor = iminor(inode);

	file->private_data = (void *) minor;
	return 0;
}

static const struct file_operations nd_bus_fops = {
	.owner = THIS_MODULE,
	.open = nd_open,
	.unlocked_ioctl = nd_ioctl,
	.compat_ioctl = nd_ioctl,
	.llseek = noop_llseek,
};

int __init nd_bus_init(void)
{
	int rc;

	rc = bus_register(&nd_bus_type);
	if (rc)
		return rc;

	rc = register_chrdev(0, "ndctl", &nd_bus_fops);
	if (rc < 0)
		goto err_chrdev;
	nd_major = rc;

	nd_class = class_create(THIS_MODULE, "nd_bus");
	if (IS_ERR(nd_class))
		goto err_class;

	return 0;

 err_class:
	unregister_chrdev(nd_major, "ndctl");
 err_chrdev:
	bus_unregister(&nd_bus_type);

	return rc;
}

void __exit nd_bus_exit(void)
{
	class_destroy(nd_class);
	unregister_chrdev(nd_major, "ndctl");
	bus_unregister(&nd_bus_type);
}
