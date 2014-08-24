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
#include <linux/mm.h>
#include <linux/nd.h>
#include "nd-private.h"
#include "nfit.h"
#include "nd.h"

static int nd_major;
static struct class *nd_class;
static DEFINE_IDA(region_ida);

static void nd_region_release(struct device *dev)
{
	struct nd_region *nd_region = to_nd_region(dev);
	u16 i;

	for (i = 0; i < nd_region->ndr_mappings; i++) {
		struct nd_mapping *nd_mapping = &nd_region->mapping[i];
		struct nd_dimm *nd_dimm = nd_mapping->nd_dimm;

		put_device(&nd_dimm->dev);
	}
	ida_simple_remove(&region_ida, nd_region->id);
	kfree(nd_region);
}

static struct device_type nd_block_device_type = {
	.name = "nd_blk",
	.release = nd_region_release,
};

static struct device_type nd_pmem_device_type = {
	.name = "nd_pmem",
	.release = nd_region_release,
};

/**
 * to_nd_region() - cast a device to a nd_region and verify it is a nd_region
 * @dev: device to cast
 */
struct nd_region *to_nd_region(struct device *dev)
{
	struct nd_region *nd_region = container_of(dev, struct nd_region, dev);

	WARN_ON(dev->type->release != nd_region_release);
	return nd_region;
}
EXPORT_SYMBOL(to_nd_region);

static bool is_nd_pmem(struct device *dev)
{
	return dev ? dev->type == &nd_pmem_device_type : false;
}

static bool is_nd_blk(struct device *dev)
{
	return dev ? dev->type == &nd_block_device_type : false;
}

static int to_nd_device_type(struct device *dev)
{
	if (is_nd_pmem(dev))
		return ND_DEVICE_REGION_PMEM;
	else if (is_nd_blk(dev))
		return ND_DEVICE_REGION_BLOCK;
	else if (is_nd_pmem(dev->parent) || is_nd_blk(dev->parent))
		return nd_region_to_namespace_type(to_nd_region(dev->parent));

	return 0;
}

static int nd_bus_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	add_uevent_var(env, "MODALIAS=" ND_DEVICE_MODALIAS_FMT,
			to_nd_device_type(dev));
	return 0;
}

static int nd_bus_match(struct device *dev, struct device_driver *drv)
{
	struct nd_device_driver *nd_drv = to_nd_drv(drv);

	return test_bit(to_nd_device_type(dev), &nd_drv->type);
}

struct bus_type nd_bus_type = {
	.name = "nd",
	.uevent = nd_bus_uevent,
	.match = nd_bus_match,
};

/**
 * nd_region_to_namespace_type() - region to an integer namespace type
 * @nd_region: region-device to interrogate
 *
 * This is the 'nstype' attribute of a region as well, an input to the
 * MODALIAS for namespace devices, and bit number for a nd_bus to match
 * namespace devices with namespace drivers.
 */
int nd_region_to_namespace_type(struct nd_region *nd_region)
{
	if (is_nd_pmem(&nd_region->dev)) {
		if (nd_region->ndr_mappings)
			return ND_DEVICE_NAMESPACE_PMEM;
		else
			return ND_DEVICE_NAMESPACE_IO;
	} else if (is_nd_blk(&nd_region->dev)) {
		return ND_DEVICE_NAMESPACE_BLOCK;
	}

	return 0;
}
EXPORT_SYMBOL(nd_region_to_namespace_type);

/**
 * __nd_driver_register() - register a region or a namespace driver
 * @nd_drv: driver to register
 * @owner: automatically set by nd_driver_register() macro
 * @mod_name: automatically set by nd_driver_register() macro
 */
int __nd_driver_register(struct nd_device_driver *nd_drv, struct module *owner,
			  const char *mod_name)
{
	struct device_driver *drv = &nd_drv->drv;

	if (!nd_drv->type) {
		pr_debug("nd: driver type flags not set\n");
		return -EINVAL;
	}

	drv->bus = &nd_bus_type;
	drv->owner = owner;
	drv->mod_name = mod_name;

	return driver_register(drv);
}
EXPORT_SYMBOL(__nd_driver_register);

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

static ssize_t size_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nd_region *nd_region = to_nd_region(dev);

	return sprintf(buf, "%llu\n", nd_region->ndr_size);
}
static DEVICE_ATTR_RO(size);

static ssize_t mappings_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nd_region *nd_region = to_nd_region(dev);

	return sprintf(buf, "%d\n", nd_region->ndr_mappings);
}
static DEVICE_ATTR_RO(mappings);

static ssize_t interleave_ways_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nd_region *nd_region = to_nd_region(dev);

	return sprintf(buf, "%d\n", nd_region->interleave_ways);
}
static DEVICE_ATTR_RO(interleave_ways);

static ssize_t spa_index_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nd_region *nd_region = to_nd_region(dev);

	return sprintf(buf, "%d\n", nd_region->spa_index);
}
DEVICE_ATTR_RO(spa_index);

static ssize_t nstype_show(struct device *dev,
                struct device_attribute *attr, char *buf)
{
	struct nd_region *nd_region = to_nd_region(dev);

	return sprintf(buf, "%d\n", nd_region_to_namespace_type(nd_region));
}
DEVICE_ATTR_RO(nstype);

static struct attribute *nd_region_attributes[] = {
	&dev_attr_size.attr,
	&dev_attr_nstype.attr,
	&dev_attr_mappings.attr,
	&dev_attr_spa_index.attr,
	&dev_attr_interleave_ways.attr,
	NULL,
};

static umode_t nd_region_visible(struct kobject *kobj, struct attribute *a, int n)
{
	struct device *dev = container_of(kobj, struct device, kobj);

	if (a != &dev_attr_spa_index.attr)
		return a->mode;

	if (is_nd_pmem(dev))
		return a->mode;

	return 0;
}

static struct attribute_group nd_region_attribute_group = {
	.attrs = nd_region_attributes,
	.is_visible = nd_region_visible,
};

static ssize_t modalias_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	return sprintf(buf, ND_DEVICE_MODALIAS_FMT "\n",
			to_nd_device_type(dev));
}
static DEVICE_ATTR_RO(modalias);

static ssize_t devtype_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	return sprintf(buf, "%s\n", dev->type->name);
}
DEVICE_ATTR_RO(devtype);

static struct attribute *nd_device_attributes[] = {
	&dev_attr_modalias.attr,
	&dev_attr_devtype.attr,
	NULL,
};

/**
 * nd_device_attribute_group - generic attributes for regions and namespaces
 */
struct attribute_group nd_device_attribute_group = {
	.attrs = nd_device_attributes,
};
EXPORT_SYMBOL(nd_device_attribute_group);

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
	struct nfit_mem __iomem *nfit_mem;
	struct nd_mapping *nd_mapping;
	struct nd_dimm *nd_dimm;

	if (n >= nd_region->ndr_mappings)
		return -ENXIO;
	nd_mapping = &nd_region->mapping[n];
	nd_dimm = nd_mapping->nd_dimm;
	nfit_mem = nd_dimm->nfit_mem;

	return sprintf(buf, "%#x,%llu,%llu\n", readl(&nfit_mem->nfit_handle),
			nd_mapping->start, nd_mapping->size);
}

#define REGION_MAPPING(idx) \
static ssize_t mapping##idx##_show(struct device *dev,		\
		struct device_attribute *attr, char *buf)	\
{								\
	return mappingN(dev, buf, idx);				\
}								\
static DEVICE_ATTR_RO(mapping##idx)

/*
 * 32 should be enough for a while, even in the presence of socket
 * interleave a 32-way interleave set is a degenerate case.
 */
REGION_MAPPING(0);
REGION_MAPPING(1);
REGION_MAPPING(2);
REGION_MAPPING(3);
REGION_MAPPING(4);
REGION_MAPPING(5);
REGION_MAPPING(6);
REGION_MAPPING(7);
REGION_MAPPING(8);
REGION_MAPPING(9);
REGION_MAPPING(10);
REGION_MAPPING(11);
REGION_MAPPING(12);
REGION_MAPPING(13);
REGION_MAPPING(14);
REGION_MAPPING(15);
REGION_MAPPING(16);
REGION_MAPPING(17);
REGION_MAPPING(18);
REGION_MAPPING(19);
REGION_MAPPING(20);
REGION_MAPPING(21);
REGION_MAPPING(22);
REGION_MAPPING(23);
REGION_MAPPING(24);
REGION_MAPPING(25);
REGION_MAPPING(26);
REGION_MAPPING(27);
REGION_MAPPING(28);
REGION_MAPPING(29);
REGION_MAPPING(30);
REGION_MAPPING(31);

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
	&dev_attr_mapping8.attr,
	&dev_attr_mapping9.attr,
	&dev_attr_mapping10.attr,
	&dev_attr_mapping11.attr,
	&dev_attr_mapping12.attr,
	&dev_attr_mapping13.attr,
	&dev_attr_mapping14.attr,
	&dev_attr_mapping15.attr,
	&dev_attr_mapping16.attr,
	&dev_attr_mapping17.attr,
	&dev_attr_mapping18.attr,
	&dev_attr_mapping19.attr,
	&dev_attr_mapping20.attr,
	&dev_attr_mapping21.attr,
	&dev_attr_mapping22.attr,
	&dev_attr_mapping23.attr,
	&dev_attr_mapping24.attr,
	&dev_attr_mapping25.attr,
	&dev_attr_mapping26.attr,
	&dev_attr_mapping27.attr,
	&dev_attr_mapping28.attr,
	&dev_attr_mapping29.attr,
	&dev_attr_mapping30.attr,
	&dev_attr_mapping31.attr,
	NULL,
};

static struct attribute_group nd_mapping_attribute_group = {
	.is_visible = nd_mapping_visible,
	.attrs = nd_mapping_attributes,
};

static const struct attribute_group *nd_region_attribute_groups[] = {
	&nd_region_attribute_group,
	&nd_device_attribute_group,
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
		nd_region->ndr_mappings = 0;
		return;
	}

	handle = readl(&nd_mem->nfit_mem->nfit_handle);
	nd_region->interleave_ways = 1;
	nd_mapping = &nd_region->mapping[0];
	dev = device_find_child(&nd_bus->dev, &handle, nd_match_dimm);
	nd_mapping->nd_dimm = to_nd_dimm(dev);
	nd_mapping->size = readq(&nd_bdw->nfit_bdw->dimm_capacity);
	nd_mapping->start = readq(&nd_bdw->nfit_bdw->dimm_block_offset);
}

static void nd_pm_init(struct nd_bus *nd_bus, struct nd_region *nd_region)
{
	struct nd_mem *nd_mem;
	u16 i, ways;

	nd_region->dev.type = &nd_pmem_device_type;
	nd_mem = nd_mem_from_spa(nd_bus, nd_region->spa_index, 0);
	ways = nd_mem ? readw(&nd_mem->nfit_mem->interleave_ways) : 0;
	nd_region->interleave_ways = ways;

	for (i = 0; i < nd_region->ndr_mappings; i++) {
		struct nd_mapping *nd_mapping = &nd_region->mapping[i];
		struct device *dev;
		u32 handle;

		nd_mem = nd_mem_from_spa(nd_bus, nd_region->spa_index, i);
		handle = readl(&nd_mem->nfit_mem->nfit_handle);
		dev = device_find_child(&nd_bus->dev, &handle, nd_match_dimm);
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
	nd_region->id = ida_simple_get(&region_ida, 0, 0, GFP_KERNEL);
	if (nd_region->id < 0) {
		kfree(nd_region);
		return NULL;
	}
	nd_region->spa_index = spa_index;
	nd_region->ndr_mappings = num_mappings;
	dev = &nd_region->dev;
	dev_set_name(dev, "region%d", nd_region->id);
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
		put_device(dev);
		return NULL;
	}

	return nd_region;
}

/**
 * nd_region_wait_for_peers() - wait for all region devices to be discovered
 * @nd_region: wait for all peers of this region to be registered
 *
 * Region devices are registered asynchronously.  Before proceeding to
 * parsing the label, the ND subsystem needs to know what regions (pmem
 * vs blk) might alias with each other.
 */
void nd_region_wait_for_peers(struct nd_region *nd_region)
{
	struct nd_bus *nd_bus = to_nd_bus(nd_region->dev.parent);

	wait_for_completion(&nd_bus->registration);
}
EXPORT_SYMBOL(nd_region_wait_for_peers);

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
