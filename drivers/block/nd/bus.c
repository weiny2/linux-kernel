/*
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
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
 *
 * BSD LICENSE
 *
 * Copyright(c) 2013-2014 Intel Corporation. All rights reserved.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/uaccess.h>
#include <linux/fcntl.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include "nd-private.h"
#include "nfit.h"
#include "nd.h"

static int nd_major;
static struct class *nd_class;

struct nd_dimm {
	struct device dev;
	u32 handle;
};

static inline struct nd_dimm *to_nd_dimm(struct device *dev)
{
	return container_of(dev, struct nd_dimm, dev);
}

static struct nd_region *to_nd_region(struct device *dev)
{
	return container_of(dev, struct nd_region, dev);
}

static void nd_dimm_release(struct device *dev)
{
	struct nd_dimm *dimm = to_nd_dimm(dev);

	kfree(dimm);
}

static struct device_type nd_dimm_device_type = {
	.name = "nd_dimm",
	.release = nd_dimm_release,
};

static void nd_region_release(struct device *dev)
{
	struct nd_region *nd_region = to_nd_region(dev);
	u16 i;

	for (i = 0; i < nd_region->ndr_mappings; i++) {
		struct nd_mapping *nd_mapping = &nd_region->mapping[i];
		struct nd_dimm *nd_dimm = nd_mapping->nd_dimm;

		put_device(&nd_dimm->dev);
	}
	kfree(nd_region);
}

static struct device_type nd_block_device_type = {
	.name = "nd_blk",
	.release = nd_region_release,
};

static struct device_type nd_pmem_device_type = {
	.name = "nd_pm",
	.release = nd_region_release,
};

static bool is_nd_pm(struct device *dev)
{
	return dev->type == &nd_pmem_device_type;
}

static bool is_nd_blk(struct device *dev)
{
	return dev->type == &nd_block_device_type;
}

static struct bus_type nd_bus_type = {
	.name = "nd",
};

static bool is_nd_dimm(struct device *dev)
{
	return dev->type == &nd_dimm_device_type;
}

static ssize_t provider_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nd_bus *nd_bus = to_nd_bus(dev->parent);

	if (nd_bus->nfit_desc->provider_name) {
		return sprintf(buf, "%s\n",
				nd_bus->nfit_desc->provider_name);
	} else {
		struct device *parent = nd_bus->dev.parent;

		return sprintf(buf, "%s\n",
				parent ? dev_name(parent) : "unknown");
	}
}
DEVICE_ATTR_RO(provider);

static ssize_t format_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nd_bus *nd_bus = to_nd_bus(dev->parent);

	return sprintf(buf, "%d\n", nd_bus->format_interface_code);
}
DEVICE_ATTR_RO(format);

static ssize_t revision_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nd_bus *nd_bus = to_nd_bus(dev->parent);
	struct nfit __iomem *nfit = nd_bus->nfit;

	return sprintf(buf, "%d\n", readb(&nfit->revision));
}
DEVICE_ATTR_RO(revision);

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

const struct attribute_group *nd_bus_attribute_groups[] = {
	&nd_bus_attribute_group,
	NULL,
};

int nd_bus_create(struct nd_bus *nd_bus)
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

void nd_bus_destroy(struct nd_bus *nd_bus)
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

static struct nd_dimm *nd_dimm_create(struct nd_bus *nd_bus,
		struct nd_mem *nd_mem)
{
	struct nd_dimm *dimm = kzalloc(sizeof(*dimm), GFP_KERNEL);
	struct device *dev;

	if (!dimm)
		return NULL;

	dimm->handle = readl(&nd_mem->nfit_mem->nfit_handle);
	dev = &dimm->dev;
	dev_set_name(dev, "dimm-%.3x:%.4x",
			NFIT_DIMM_NODE(dimm->handle),
			NFIT_DIMM_SICD(dimm->handle));
	dev->parent = &nd_bus->dev;
	dev->type = &nd_dimm_device_type;
	dev->bus = &nd_bus_type;
	if (device_register(dev) != 0) {
		kfree(dimm);
		dimm = NULL;
	}

	return dimm;
}

static int match_dimm(struct device *dev, void *data)
{
	u32 handle = *(u32 *) data;
	struct nd_dimm *nd_dimm;

	if (!is_nd_dimm(dev))
		return 0;
	nd_dimm = to_nd_dimm(dev);
	if (handle == nd_dimm->handle)
		return 1;

	return 0;
}

int nd_bus_register_dimms(struct nd_bus *nd_bus)
{
	struct nd_mem *nd_mem;
	struct nd_dimm *dimm;
	u32 handle;
	int rc = 0;

	mutex_lock(&nd_bus_list_mutex);
	list_for_each_entry(nd_mem, &nd_bus->memdevs, list) {
		struct device *dev;

		handle = readl(&nd_mem->nfit_mem->nfit_handle);
		dev = device_find_child(&nd_bus->dev, &handle, match_dimm);
		if (dev) {
			put_device(dev);
			continue;
		}
		dimm = nd_dimm_create(nd_bus, nd_mem);
		if (!dimm) {
			rc = -ENOMEM;
			break;
		}
	}
	mutex_unlock(&nd_bus_list_mutex);

	return rc;
}

static ssize_t type_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", is_nd_pm(dev) ? "pm" : is_nd_blk(dev)
			? "block" : "unknown");
}
DEVICE_ATTR_RO(type);

static ssize_t size_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nd_region *nd_region = to_nd_region(dev);

	return sprintf(buf, "%llu\n", nd_region->ndr_size);
}
DEVICE_ATTR_RO(size);

static ssize_t mappings_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nd_region *nd_region = to_nd_region(dev);

	return sprintf(buf, "%d\n", nd_region->ndr_mappings);
}
DEVICE_ATTR_RO(mappings);

static ssize_t interleave_ways_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nd_region *nd_region = to_nd_region(dev);

	return sprintf(buf, "%d\n", nd_region->interleave_ways);
}
DEVICE_ATTR_RO(interleave_ways);

static struct attribute *nd_region_attributes[] = {
	&dev_attr_type.attr,
	&dev_attr_size.attr,
	&dev_attr_mappings.attr,
	&dev_attr_interleave_ways.attr,
	NULL,
};

static struct attribute_group nd_region_attribute_group = {
	.attrs = nd_region_attributes,
};

/*
 * Retrieve the nth entry referencing this spa, for pm there may be not only
 * multiple per device in the interleave, but multiple per-dimm for each region
 * of the dimm that maps into the interleave.
 */
static struct nd_mem *nd_mem_from_spa(struct nd_bus *nd_bus, u16 spa_index, int n)
{
	struct nd_mem *nd_mem;

	list_for_each_entry(nd_mem, &nd_bus->memdevs, list)
		if (readw(&nd_mem->nfit_mem->spa_index) == spa_index)
			if (n-- == 0)
				return nd_mem;
	return NULL;
}

static int num_nd_mem(struct nd_bus *nd_bus, u16 spa_index)
{
	struct nd_mem *nd_mem;
	int count = 0;

	list_for_each_entry(nd_mem, &nd_bus->memdevs, list)
		if (readw(&nd_mem->nfit_mem->spa_index) == spa_index)
			count++;
	return count;
}

static ssize_t mappingN(struct device *dev, char *buf, int n)
{
	struct nd_region *nd_region = to_nd_region(dev);
	struct nd_mapping *nd_mapping;
	struct nd_dimm *nd_dimm;

	if (n >= nd_region->ndr_mappings)
		return -ENXIO;
	nd_mapping = &nd_region->mapping[n];
	nd_dimm = nd_mapping->nd_dimm;

	return sprintf(buf, "%.3x:%.4x,%llu,%llu\n",
			NFIT_DIMM_NODE(nd_dimm->handle),
			NFIT_DIMM_SICD(nd_dimm->handle),
			nd_mapping->start, nd_mapping->size);
}

#define REGION_MAPPING(idx) \
static ssize_t mapping##idx##_show(struct device *dev,		\
		struct device_attribute *attr, char *buf)	\
{								\
	return mappingN(dev, buf, idx);				\
}								\
DEVICE_ATTR_RO(mapping##idx)

/* FIXME: might we ever have more than 8 mappings per region? */
REGION_MAPPING(0);
REGION_MAPPING(1);
REGION_MAPPING(2);
REGION_MAPPING(3);
REGION_MAPPING(4);
REGION_MAPPING(5);
REGION_MAPPING(6);
REGION_MAPPING(7);

static umode_t nd_mapping_visible(struct kobject *kobj, struct attribute *a, int n)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct nd_region *nd_region = to_nd_region(dev);

	if (n < nd_region->ndr_mappings)
		return a->mode;
	return 0;
}

static struct attribute *nd_mapping_attributes[] = {
	&dev_attr_mapping0.attr,
	&dev_attr_mapping1.attr,
	&dev_attr_mapping2.attr,
	&dev_attr_mapping3.attr,
	&dev_attr_mapping4.attr,
	&dev_attr_mapping5.attr,
	&dev_attr_mapping6.attr,
	&dev_attr_mapping7.attr,
	NULL,
};

static struct attribute_group nd_mapping_attribute_group = {
	.is_visible = nd_mapping_visible,
	.attrs = nd_mapping_attributes,
};

const struct attribute_group *nd_region_attribute_groups[] = {
	&nd_region_attribute_group,
	&nd_mapping_attribute_group,
	NULL,
};

static void nd_blk_init(struct nd_bus *nd_bus, struct nd_region *nd_region)
{
	struct nd_bdw *iter, *nd_bdw = NULL;
	struct nd_mapping *nd_mapping;
	struct nd_mem *nd_mem;
	struct device *dev;
	u16 dcr_index;
	u32 handle;

	nd_region->dev.type = &nd_block_device_type;

	nd_mem = nd_mem_from_spa(nd_bus, nd_region->spa_index, 0);
	dcr_index = readw(&nd_mem->nfit_mem->dcr_index);
	list_for_each_entry(iter, &nd_bus->bdws, list) {
		if (readw(&iter->nfit_bdw->dcr_index) == dcr_index) {
			nd_bdw = iter;
			break;
		}
	}

	if (!nd_bdw) {
		dev_err(&nd_bus->dev, "%s: failed to find bdw table for dcr %d\n",
				__func__, dcr_index);
		return;
	}

	handle = readl(&nd_mem->nfit_mem->nfit_handle);
	nd_region->interleave_ways = 1;
	nd_mapping = &nd_region->mapping[0];
	dev = device_find_child(&nd_bus->dev, &handle, match_dimm);
	nd_mapping->nd_dimm = to_nd_dimm(dev);
	nd_mapping->size = readq(&nd_bdw->nfit_bdw->dimm_capacity);
	nd_mapping->start = readq(&nd_bdw->nfit_bdw->dimm_block_offset);
}

static void nd_pm_init(struct nd_bus *nd_bus, struct nd_region *nd_region)
{
	struct nd_mem *nd_mem;
	u16 i;

	nd_region->dev.type = &nd_pmem_device_type;
	nd_mem = nd_mem_from_spa(nd_bus, nd_region->spa_index, 0);
	nd_region->interleave_ways = readw(&nd_mem->nfit_mem->interleave_ways);

	for (i = 0; i < nd_region->ndr_mappings; i++) {
		struct nd_mapping *nd_mapping = &nd_region->mapping[i];
		struct device *dev;
		u32 handle;

		nd_mem = nd_mem_from_spa(nd_bus, nd_region->spa_index, i);
		handle = readl(&nd_mem->nfit_mem->nfit_handle);
		dev = device_find_child(&nd_bus->dev, &handle, match_dimm);
		nd_mapping->nd_dimm = to_nd_dimm(dev);
		nd_mapping->start = readq(&nd_mem->nfit_mem->region_offset);
		nd_mapping->size = readq(&nd_mem->nfit_mem->region_len);
	}
}

static struct nd_region *nd_region_create(struct nd_bus *nd_bus,
		struct nd_spa *nd_spa)
{
	u16 spa_index = readw(&nd_spa->nfit_spa->spa_index);
	u16 spa_type = readw(&nd_spa->nfit_spa->spa_type);
	struct nd_region *nd_region;
	struct device *dev;
	u16 num_mappings;

	num_mappings = num_nd_mem(nd_bus, spa_index);
	nd_region = kzalloc(sizeof(struct nd_region)
			+ sizeof(struct nd_mapping) * num_mappings, GFP_KERNEL);
	if (!nd_region)
		return NULL;
	nd_region->spa_index = spa_index;
	nd_region->ndr_mappings = num_mappings;
	dev = &nd_region->dev;
	dev_set_name(dev, "region%d-%d", nd_bus->id, spa_index - 1);
	dev->bus = &nd_bus_type;
	dev->parent = &nd_bus->dev;
	dev->groups = nd_region_attribute_groups;
	nd_region->ndr_size = readq(&nd_spa->nfit_spa->spa_length);
	nd_region->ndr_start = readq(&nd_spa->nfit_spa->spa_base);
	if (spa_type == NFIT_SPA_PM)
		nd_pm_init(nd_bus, nd_region);
	else
		nd_blk_init(nd_bus, nd_region);

	if (device_register(dev) != 0) {
		kfree(nd_region);
		nd_region = NULL;
	}

	return nd_region;
}

int nd_bus_register_regions(struct nd_bus *nd_bus)
{
	struct nd_spa *nd_spa;
	int rc = 0;

	mutex_lock(&nd_bus_list_mutex);
	list_for_each_entry(nd_spa, &nd_bus->spas, list) {
		u16 spa_type, spa_index;
		struct nd_region *nd_region;

		spa_type = readw(&nd_spa->nfit_spa->spa_type);
		spa_index = readw(&nd_spa->nfit_spa->spa_index);
		if (spa_index == 0) {
			dev_dbg(&nd_bus->dev, "detected invalid spa index\n");
			continue;
		}
		switch (spa_type) {
		case NFIT_SPA_PM:
		case NFIT_SPA_DCR:
			nd_region = nd_region_create(nd_bus, nd_spa);
			if (!nd_region) {
				rc = -ENOMEM;
				break;
			}
		default:
			dev_dbg(&nd_bus->dev, "spa[%d] unknown type: %d\n",
					spa_index, spa_type);
			/* fallthrough */
		case NFIT_SPA_BDW:
			/* we'll consume this in nd_blk_register */
			break;
		}
	}
	mutex_unlock(&nd_bus_list_mutex);

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
