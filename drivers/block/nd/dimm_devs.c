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
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include "nd-private.h"
#include "nfit.h"
#include "nd.h"

static DEFINE_IDA(dimm_ida);

/*
 * Retrieve bus and dimm handle and return if this bus supports
 * get_config_data commands
 */
static int __validate_dimm(struct nd_dimm *nd_dimm, struct nd_bus **nd_bus,
		u32 *nfit_handle)
{
	struct nfit_bus_descriptor *nfit_desc;
	struct nfit_mem __iomem *nfit_mem;

	if (!nd_dimm)
		return -EINVAL;

	*nd_bus = walk_to_nd_bus(&nd_dimm->dev);
	if (*nd_bus == NULL)
		return -EINVAL;
	nfit_desc = (*nd_bus)->nfit_desc;
	if (!test_bit(NFIT_CMD_GET_CONFIG_DATA, &nfit_desc->dsm_mask)
			|| !nfit_desc->nfit_ctl)
		return -ENXIO;

	nfit_mem = nd_dimm->nd_mem->nfit_mem;
	*nfit_handle = readl(&nfit_mem->nfit_handle);
	return 0;
}

static int validate_dimm(struct nd_dimm *nd_dimm,
		struct nd_bus **nd_bus, u32 *nfit_handle)
{
	int rc = __validate_dimm(nd_dimm, nd_bus, nfit_handle);

	if (rc)
		dev_dbg(&nd_dimm->dev, "%pf: %s error: %d\n",
				__builtin_return_address(0), __func__, rc);
	return rc;
}

int nd_dimm_get_config_size(struct nd_dimm *nd_dimm,
		struct nfit_cmd_get_config_size *cmd)
{
	u32 nfit_handle;
	struct nd_bus *nd_bus;
	struct nfit_bus_descriptor *nfit_desc;
	int rc = validate_dimm(nd_dimm, &nd_bus, &nfit_handle);

	if (rc)
		return rc;

	nfit_desc = nd_bus->nfit_desc;
	memset(cmd, 0, sizeof(*cmd));
	cmd->nfit_handle = nfit_handle;
	return nfit_desc->nfit_ctl(nfit_desc, NFIT_CMD_GET_CONFIG_SIZE, cmd,
			sizeof(*cmd));
}

int nd_dimm_get_config_data(struct nd_dimm *nd_dimm,
		struct nfit_cmd_get_config_data_hdr *cmd, size_t len)
{
	u32 nfit_handle;
	struct nd_bus *nd_bus;
	struct nfit_bus_descriptor *nfit_desc;
	int rc = validate_dimm(nd_dimm, &nd_bus, &nfit_handle);

	if (rc)
		return rc;

	nfit_desc = nd_bus->nfit_desc;
	memset(cmd, 0, len);
	cmd->nfit_handle = nfit_handle;
	cmd->in_length = len - sizeof(*cmd);
	return nfit_desc->nfit_ctl(nfit_desc, NFIT_CMD_GET_CONFIG_DATA, cmd,
			len);
}

int nd_dimm_set_config_data(struct nd_dimm *nd_dimm, size_t offset,
		void *buf, size_t len)
{
	u32 nfit_handle;
	struct nd_bus *nd_bus;
	struct nfit_cmd_set_config_hdr *cmd;
	struct nfit_bus_descriptor *nfit_desc;
	size_t size = sizeof(*cmd) + len + sizeof(u32);
	int rc = validate_dimm(nd_dimm, &nd_bus, &nfit_handle);

	if (rc)
		return rc;

	cmd = kmalloc(size, GFP_KERNEL);
	if (!cmd)
		cmd = vmalloc(size);
	if (!cmd)
		return -ENOMEM;

	nfit_desc = nd_bus->nfit_desc;
	memset(cmd, 0, size);
	cmd->nfit_handle = nfit_handle;
	cmd->in_offset = offset;
	cmd->in_length = len;
	memcpy(cmd->in_buf, buf, len);
	rc = nfit_desc->nfit_ctl(nfit_desc, NFIT_CMD_SET_CONFIG_DATA, cmd,
			size);
	if (rc == 0) {
		/* rc == 0 == status valid */
		u32 *status = ((void *) cmd) + size - sizeof(u32);

		if (*status != 0)
			rc = -ENXIO;
	}
	dev_dbg(&nd_dimm->dev, "%s: offset: %zd len: %zd rc: %d\n",
			__func__, offset, len, rc);

	if (is_vmalloc_addr(cmd))
		vfree(cmd);
	else
		kfree(cmd);
	return rc;
}

static void nd_dimm_release(struct device *dev)
{
	struct nd_dimm *nd_dimm = to_nd_dimm(dev);

	ida_simple_remove(&dimm_ida, nd_dimm->id);
	nd_dimm_delete(nd_dimm);
	kfree(nd_dimm);
}

static struct device_type nd_dimm_device_type = {
	.name = "nd_dimm",
	.release = nd_dimm_release,
};

bool is_nd_dimm(struct device *dev)
{
	return dev->type == &nd_dimm_device_type;
}

struct nd_dimm *to_nd_dimm(struct device *dev)
{
	struct nd_dimm *nd_dimm = container_of(dev, struct nd_dimm, dev);

	WARN_ON(!is_nd_dimm(dev));
	return nd_dimm;
}

static struct nfit_mem __iomem *to_nfit_mem(struct device *dev)
{
	struct nd_dimm *nd_dimm = to_nd_dimm(dev);
	struct nd_mem *nd_mem = nd_dimm->nd_mem;
	struct nfit_mem __iomem *nfit_mem = nd_mem->nfit_mem;

	return nfit_mem;
}

static struct nfit_dcr __iomem *to_nfit_dcr(struct device *dev)
{
	struct nd_dimm *nd_dimm = to_nd_dimm(dev);
	struct nd_mem *nd_mem = nd_dimm->nd_mem;
	struct nfit_dcr __iomem *nfit_dcr = nd_mem->nfit_dcr;

	return nfit_dcr;
}

static ssize_t handle_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nfit_mem __iomem *nfit_mem = to_nfit_mem(dev);

	return sprintf(buf, "%#x\n", readl(&nfit_mem->nfit_handle));
}
static DEVICE_ATTR_RO(handle);

static ssize_t phys_id_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nfit_mem __iomem *nfit_mem = to_nfit_mem(dev);

	return sprintf(buf, "%#x\n", readw(&nfit_mem->phys_id));
}
static DEVICE_ATTR_RO(phys_id);

static ssize_t vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nfit_dcr __iomem *nfit_dcr = to_nfit_dcr(dev);

	return sprintf(buf, "%#x\n", readw(&nfit_dcr->vendor_id));
}
static DEVICE_ATTR_RO(vendor);

static ssize_t revision_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nfit_dcr __iomem *nfit_dcr = to_nfit_dcr(dev);

	return sprintf(buf, "%#x\n", readw(&nfit_dcr->revision_id));
}
static DEVICE_ATTR_RO(revision);

static ssize_t device_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nfit_dcr __iomem *nfit_dcr = to_nfit_dcr(dev);

	return sprintf(buf, "%#x\n", readw(&nfit_dcr->device_id));
}
static DEVICE_ATTR_RO(device);

static ssize_t format_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nfit_dcr __iomem *nfit_dcr = to_nfit_dcr(dev);
	struct nd_bus *nd_bus = walk_to_nd_bus(dev);

	return sprintf(buf, "%#x\n", nfit_dcr_fic(nfit_dcr,
				nd_bus->nfit_desc->old_nfit));
}
static DEVICE_ATTR_RO(format);

static struct attribute *nd_dimm_attributes[] = {
	&dev_attr_handle.attr,
	&dev_attr_phys_id.attr,
	&dev_attr_vendor.attr,
	&dev_attr_device.attr,
	&dev_attr_format.attr,
	&dev_attr_revision.attr,
	NULL,
};

static umode_t nd_dimm_attr_visible(struct kobject *kobj, struct attribute *a, int n)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct nd_dimm *nd_dimm = to_nd_dimm(dev);

	if (a == &dev_attr_handle.attr || a == &dev_attr_phys_id.attr
			|| to_nfit_dcr(&nd_dimm->dev))
		return a->mode;
	else
		return 0;
}

static struct attribute_group nd_dimm_attribute_group = {
	.attrs = nd_dimm_attributes,
	.is_visible = nd_dimm_attr_visible,
};

static const struct attribute_group *nd_dimm_attribute_groups[] = {
	&nd_dimm_attribute_group,
	&nd_device_attribute_group,
	NULL,
};

static struct nd_dimm *nd_dimm_create(struct nd_bus *nd_bus,
		struct nd_mem *nd_mem)
{
	struct nd_dimm *nd_dimm = kzalloc(sizeof(*nd_dimm), GFP_KERNEL);
	struct device *dev;
	u32 nfit_handle;

	if (!nd_dimm)
		return NULL;

	nd_dimm->del_info = kzalloc(sizeof(struct nd_dimm_delete), GFP_KERNEL);
	if (!nd_dimm->del_info)
		goto err_del_info;
	nd_dimm->del_info->nd_bus = nd_bus;
	nd_dimm->del_info->nd_mem = nd_mem;
	nd_dimm->ns_current = -1;
	nd_dimm->ns_next = -1;

	nfit_handle = readl(&nd_mem->nfit_mem->nfit_handle);
	if (radix_tree_insert(&nd_bus->dimm_radix, nfit_handle, nd_dimm) != 0)
		goto err_radix;

	nd_dimm->id = ida_simple_get(&dimm_ida, 0, 0, GFP_KERNEL);
	if (nd_dimm->id < 0)
		goto err_ida;

	nd_dimm->nd_mem = nd_mem;
	dev = &nd_dimm->dev;
	dev_set_name(dev, "dimm%d", nd_dimm->id);
	dev->parent = &nd_bus->dev;
	dev->type = &nd_dimm_device_type;
	dev->groups = nd_dimm_attribute_groups;
	nd_dimm->dpa.name = dev_name(dev);
	nd_dimm->dpa.start = 0,
	nd_dimm->dpa.end = -1,
	nd_dimm->dpa.flags = IORESOURCE_REG;
	nd_device_register(dev);

	return nd_dimm;
 err_ida:
	radix_tree_delete(&nd_bus->dimm_radix, nfit_handle);
 err_radix:
	kfree(nd_dimm->del_info);
 err_del_info:
	kfree(nd_dimm);
	return NULL;
}

static int count_dimms(struct device *dev, void *c)
{
	int *count = c;

	if (is_nd_dimm(dev))
		(*count)++;
	return 0;
}

int nd_bus_register_dimms(struct nd_bus *nd_bus)
{
	int rc = 0, dimm_count = 0;
	struct nd_mem *nd_mem;

	mutex_lock(&nd_bus_list_mutex);
	list_for_each_entry(nd_mem, &nd_bus->memdevs, list) {
		struct nd_dimm *nd_dimm;
		struct nd_dcr *nd_dcr;
		u32 nfit_handle;
		u16 dcr_index;

		nfit_handle = readl(&nd_mem->nfit_mem->nfit_handle);
		nd_dimm = nd_dimm_by_handle(nd_bus, nfit_handle);
		if (nd_dimm) {
			put_device(&nd_dimm->dev);
			continue;
		}

		dcr_index = readw(&nd_mem->nfit_mem->dcr_index);
		list_for_each_entry(nd_dcr, &nd_bus->dcrs, list) {
			if (readw(&nd_dcr->nfit_dcr->dcr_index) == dcr_index) {
				nd_mem->nfit_dcr = nd_dcr->nfit_dcr;
				break;
			}
		}

		if (!nd_dimm_create(nd_bus, nd_mem)) {
			rc = -ENOMEM;
			break;
		}
		dimm_count++;
	}
	mutex_unlock(&nd_bus_list_mutex);

	/*
	 * Flush dimm registration as 'nd_region' registration depends on
	 * finding 'nd_dimm's on the bus.
	 */
	nd_synchronize();
	if (rc)
		return rc;

	rc = 0;
	device_for_each_child(&nd_bus->dev, &rc, count_dimms);
	dev_dbg(&nd_bus->dev, "%s: count: %d\n", __func__, rc);
	if (rc != dimm_count)
		return -ENXIO;
	return 0;
}
