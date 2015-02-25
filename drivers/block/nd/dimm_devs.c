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
#include <linux/highmem.h>
#include <linux/ndctl.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include "nd-private.h"
#include "label.h"
#include "nfit.h"
#include "nd.h"

static DEFINE_IDA(dimm_ida);

#define CL_SHIFT		6

enum {
	BCW_OFFSET_MASK		= (1ULL << 48)-1,
	BCW_LEN_SHIFT		= 48,
	BCW_LEN_MASK		= (1ULL << 8) - 1,
	BCW_CMD_SHIFT		= 56,
};

bool is_acpi_blk(struct device *dev)
{
	if (strcmp(nd_blk_bus_provider(dev), "OLD_ACPI.NFIT") == 0)
		return true;
	else if (strcmp(nd_blk_bus_provider(dev), "ACPI.NFIT") == 0)
		return true;

	return false;
}
EXPORT_SYMBOL(is_acpi_blk);

/* for now, hard code index 0 */
/* for NT stores, check out __copy_user_nocache() */
static void nd_blk_write_blk_ctl(struct block_window *bw,
		resource_size_t dev_offset, unsigned int len, bool write)
{
	u64 cmd		= 0;
	u64 cl_offset	= dev_offset >> CL_SHIFT;
	u64 cl_len	= len >> CL_SHIFT;

	cmd |= cl_offset & BCW_OFFSET_MASK;
	cmd |= (cl_len & BCW_LEN_MASK) << BCW_LEN_SHIFT;
	if (write)
		cmd |= 1ULL << BCW_CMD_SHIFT;

	writeq(cmd, bw->bw_ctl_virt);
	clflushopt(bw->bw_ctl_virt);
}

static int nd_blk_read_blk_win(struct block_window *bw, void *dst,
		unsigned int len)
{
	u32 status;

	/* FIXME: NT */
	memcpy(dst, bw->bw_apt_virt, len);
	clflushopt(bw->bw_apt_virt);

	status = readl(bw->bw_stat_virt);

	if (status) {
		/* FIXME: return more precise error values at some point */
		return -EIO;
	}

	return 0;
}

static int nd_blk_write_blk_win(struct block_window *bw, void *src,
		unsigned int len)
{
	/* non-temporal writes, need to flush via flush hints, yada yada. */
	u32 status;

	/* FIXME: NT */
	memcpy(bw->bw_apt_virt, src, len);

	status = readl(bw->bw_stat_virt);

	if (status) {
		/* FIXME: return more precise error values at some point */
		return -EIO;
	}

	return 0;
}

static int nd_blk_read(struct block_window *bw, void *dst,
		resource_size_t dev_offset, unsigned int len)
{
	nd_blk_write_blk_ctl(bw, dev_offset, len, false);
	return nd_blk_read_blk_win(bw, dst, len);
}

static int nd_blk_write(struct block_window *bw, void *src,
		resource_size_t dev_offset, unsigned int len)
{
	nd_blk_write_blk_ctl(bw, dev_offset, len, true);
	return nd_blk_write_blk_win(bw, src, len);
}

static void acquire_bw(struct nd_blk_dimm *dimm, unsigned *bw_index)
{
	*bw_index = atomic_inc_return(&(dimm->last_bw)) % dimm->num_bw;
	spin_lock(&(dimm->bw_lock[*bw_index]));
}

static void release_bw(struct nd_blk_dimm *dimm, unsigned bw_index)
{
	spin_unlock(&(dimm->bw_lock[bw_index]));
}

/* len is <= PAGE_SIZE by this point, so it can be done in a single BW I/O */
int nd_blk_do_io(struct nd_blk_dimm *dimm, struct page *page,
		unsigned int len, unsigned int off, int rw,
		resource_size_t dev_offset)
{
	void *mem = kmap_atomic(page);
	struct block_window *bw;
	unsigned bw_index;
	int rc;

	acquire_bw(dimm, &bw_index);
	bw = &dimm->bw[bw_index];

	if (rw == READ)
		rc = nd_blk_read(bw, mem + off, dev_offset, len);
	else
		rc = nd_blk_write(bw, mem + off, dev_offset, len);

	release_bw(dimm, bw_index);
	kunmap_atomic(mem);

	return rc;
}
EXPORT_SYMBOL(nd_blk_do_io);

/*
 * Retrieve bus and dimm handle and return if this bus supports
 * get_config_data commands
 */
static int __validate_dimm(struct nd_dimm_drvdata *ndd)
{
	struct nd_bus *nd_bus;
	struct nd_dimm *nd_dimm;
	struct nfit_mem __iomem *nfit_mem;
	struct nfit_dcr __iomem *nfit_dcr;
	struct nfit_bus_descriptor *nfit_desc;

	if (!ndd)
		return -EINVAL;

	nd_dimm = to_nd_dimm(ndd->dev);
	nd_bus = walk_to_nd_bus(&nd_dimm->dev);
	nfit_desc = nd_bus->nfit_desc;

	if (!test_bit(NFIT_CMD_GET_CONFIG_DATA, &nd_dimm->dsm_mask))
		return -ENXIO;

	nfit_dcr = nd_dimm->nd_mem->nfit_dcr;
	if (!nfit_dcr || nfit_dcr_fic(nfit_desc, nfit_dcr) != NFIT_FIC)
		return -ENODEV;
	nfit_mem = nd_dimm->nd_mem->nfit_mem;
	return 0;
}

static int validate_dimm(struct nd_dimm_drvdata *ndd)
{
	int rc = __validate_dimm(ndd);

	if (rc && ndd)
		dev_dbg(ndd->dev, "%pf: %s error: %d\n",
				__builtin_return_address(0), __func__, rc);
	return rc;
}

/**
 * nd_dimm_init_nsarea - determine the geometry of a dimm's namespace area
 * @nd_dimm: dimm to initialize
 */
int nd_dimm_init_nsarea(struct nd_dimm_drvdata *ndd)
{
	struct nfit_cmd_get_config_size *cmd = &ndd->nsarea;
	struct nd_bus *nd_bus = walk_to_nd_bus(ndd->dev);
	struct nfit_bus_descriptor *nfit_desc;
	int rc = validate_dimm(ndd);

	if (rc)
		return rc;

	if (cmd->config_size)
		return 0; /* already valid */

	memset(cmd, 0, sizeof(*cmd));
	nfit_desc = nd_bus->nfit_desc;
	return nfit_desc->nfit_ctl(nfit_desc, to_nd_dimm(ndd->dev),
			NFIT_CMD_GET_CONFIG_SIZE, cmd, sizeof(*cmd));
}

int nd_dimm_init_config_data(struct nd_dimm_drvdata *ndd)
{
	struct nd_bus *nd_bus = walk_to_nd_bus(ndd->dev);
	struct nfit_cmd_get_config_data_hdr *cmd;
	struct nfit_bus_descriptor *nfit_desc;
	int rc = validate_dimm(ndd);
	u32 max_cmd_size, config_size;
	size_t offset;

	if (rc)
		return rc;

	if (ndd->data)
		return 0;

	if (ndd->nsarea.status || ndd->nsarea.max_xfer == 0
			|| ndd->nsarea.config_size < ND_LABEL_MIN_SIZE)
		return -ENXIO;

	ndd->data = kmalloc(ndd->nsarea.config_size, GFP_KERNEL);
	if (!ndd->data)
		ndd->data = vmalloc(ndd->nsarea.config_size);

	if (!ndd->data)
		return -ENOMEM;

	max_cmd_size = min_t(u32, PAGE_SIZE, ndd->nsarea.max_xfer);
	cmd = kzalloc(max_cmd_size + sizeof(*cmd), GFP_KERNEL);
	if (!cmd)
		return -ENOMEM;

	nfit_desc = nd_bus->nfit_desc;
	for (config_size = ndd->nsarea.config_size, offset = 0;
			config_size; config_size -= cmd->in_length,
			offset += cmd->in_length) {
		cmd->in_length = min(config_size, max_cmd_size);
		cmd->in_offset = offset;
		rc = nfit_desc->nfit_ctl(nfit_desc, to_nd_dimm(ndd->dev),
				NFIT_CMD_GET_CONFIG_DATA, cmd,
				cmd->in_length + sizeof(*cmd));
		if (rc || cmd->status) {
			rc = -ENXIO;
			break;
		}
		memcpy(ndd->data + offset, cmd->out_buf, cmd->in_length);
	}
	dev_dbg(ndd->dev, "%s: len: %zd rc: %d\n", __func__, offset, rc);
	kfree(cmd);

	return rc;
}

int nd_dimm_set_config_data(struct nd_dimm_drvdata *ndd, size_t offset,
		void *buf, size_t len)
{
	int rc = validate_dimm(ndd);
	size_t max_cmd_size, buf_offset;
	struct nfit_cmd_set_config_hdr *cmd;
	struct nd_bus *nd_bus = walk_to_nd_bus(ndd->dev);
	struct nfit_bus_descriptor *nfit_desc = nd_bus->nfit_desc;

	if (rc)
		return rc;

	if (!ndd->data)
		return -ENXIO;

	if (offset + len > ndd->nsarea.config_size)
		return -ENXIO;

	max_cmd_size = min(PAGE_SIZE, len);
	max_cmd_size = min_t(u32, max_cmd_size, ndd->nsarea.max_xfer);
	cmd = kzalloc(max_cmd_size + sizeof(*cmd) + sizeof(u32), GFP_KERNEL);
	if (!cmd)
		return -ENOMEM;

	for (buf_offset = 0; len; len -= cmd->in_length,
			buf_offset += cmd->in_length) {
		size_t cmd_size;
		u32 *status;

		cmd->in_offset = offset + buf_offset;
		cmd->in_length = min(max_cmd_size, len);
		memcpy(cmd->in_buf, buf + buf_offset, cmd->in_length);

		/* status is output in the last 4-bytes of the command buffer */
		cmd_size = sizeof(*cmd) + cmd->in_length + sizeof(u32);
		status = ((void *) cmd) + cmd_size - sizeof(u32);

		rc = nfit_desc->nfit_ctl(nfit_desc, to_nd_dimm(ndd->dev),
				NFIT_CMD_SET_CONFIG_DATA, cmd, cmd_size);
		if (rc || *status) {
			rc = rc ? rc : -ENXIO;
			break;
		}
	}
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

struct nd_dimm_drvdata *to_ndd(struct nd_mapping *nd_mapping)
{
	struct nd_dimm *nd_dimm = nd_mapping->nd_dimm;

	return dev_get_drvdata(&nd_dimm->dev);
}
EXPORT_SYMBOL(to_ndd);

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

u32 to_nfit_handle(struct nd_dimm *nd_dimm)
{
	struct nfit_mem __iomem *nfit_mem = nd_dimm->nd_mem->nfit_mem;

	return readl(&nfit_mem->nfit_handle);
}
EXPORT_SYMBOL(to_nfit_handle);

void *nd_dimm_get_pdata(struct nd_dimm *nd_dimm)
{
	if (nd_dimm)
		return nd_dimm->provider_data;
	return NULL;
}
EXPORT_SYMBOL(nd_dimm_get_pdata);

void nd_dimm_set_pdata(struct nd_dimm *nd_dimm, void *data)
{
	if (nd_dimm)
		nd_dimm->provider_data = data;
}
EXPORT_SYMBOL(nd_dimm_set_pdata);

unsigned long nd_dimm_get_dsm_mask(struct nd_dimm *nd_dimm)
{
	if (nd_dimm)
		return nd_dimm->dsm_mask;
	return 0;
}
EXPORT_SYMBOL(nd_dimm_get_dsm_mask);

void nd_dimm_set_dsm_mask(struct nd_dimm *nd_dimm, unsigned long dsm_mask)
{
	if (nd_dimm)
		nd_dimm->dsm_mask = dsm_mask;
}
EXPORT_SYMBOL(nd_dimm_set_dsm_mask);

static ssize_t handle_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%#x\n", to_nfit_handle(to_nd_dimm(dev)));
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

	return sprintf(buf, "%#x\n", nfit_dcr_fic(nd_bus->nfit_desc, nfit_dcr));
}
static DEVICE_ATTR_RO(format);

static ssize_t serial_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nfit_mem __iomem *nfit_mem = to_nfit_mem(dev);
	struct nfit_dcr __iomem *nfit_dcr = to_nfit_dcr(dev);
	struct nd_bus *nd_bus = walk_to_nd_bus(dev);

	return sprintf(buf, "%#x\n",
			nfit_dcr_serial(nd_bus->nfit_desc, nfit_dcr, nfit_mem));
}
static DEVICE_ATTR_RO(serial);

static ssize_t commands_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nd_dimm *nd_dimm = to_nd_dimm(dev);
	int cmd, len = 0;

	for_each_set_bit(cmd, &nd_dimm->dsm_mask, BITS_PER_LONG)
		len += sprintf(buf + len, "%s ", nfit_dimm_cmd_name(cmd));
	len += sprintf(buf + len, "\n");
	return len;
}
static DEVICE_ATTR_RO(commands);

static ssize_t available_slots_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nd_dimm_drvdata *ndd = dev_get_drvdata(dev);
	ssize_t rc;
	u32 nfree;

	if (!ndd)
		return -ENXIO;

	nd_bus_lock(dev);
	nfree = nd_label_nfree(ndd);
	if (nfree - 1 > nfree) {
		dev_WARN_ONCE(dev, 1, "we ate our last label?\n");
		nfree = 0;
	} else
		nfree--;
	rc = sprintf(buf, "%d\n", nfree);
	nd_bus_unlock(dev);
	return rc;
}
static DEVICE_ATTR_RO(available_slots);

static ssize_t state_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct nd_bus *nd_bus = walk_to_nd_bus(dev);
	struct nd_dimm *nd_dimm = to_nd_dimm(dev);
	struct nd_mem *nd_mem = nd_dimm->nd_mem;
	u32 key = to_interleave_set_key(nd_mem);
	struct nd_interleave_set *nd_set;
	int busy;

	/*
	 * The state may be in the process of changing, userspace should
	 * quiesce probing if it wants a static answer
	 */
	nd_bus_lock(dev);
	nd_set = radix_tree_lookup(&nd_bus->interleave_sets, key);
	if (!nd_set)
		busy = -ENXIO;
	else
		busy = nd_set->busy;
	nd_bus_unlock(dev);

	if (busy < 0)
		return busy;

	return sprintf(buf, "%s\n", busy ? "active" : "idle");
}
static DEVICE_ATTR_RO(state);

static struct attribute *nd_dimm_attributes[] = {
	&dev_attr_handle.attr,
	&dev_attr_phys_id.attr,
	&dev_attr_vendor.attr,
	&dev_attr_device.attr,
	&dev_attr_format.attr,
	&dev_attr_serial.attr,
	&dev_attr_state.attr,
	&dev_attr_revision.attr,
	&dev_attr_commands.attr,
	&dev_attr_available_slots.attr,
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

/**
 * nd_dimm_firmware_status - retrieve NFIT-specific state of the dimm
 * @dev: dimm device to interrogate
 *
 * At init time as the NFIT parsing code discovers DIMMs (memdevs) it
 * validates the state of those devices against the NFIT provider.  It
 * is possible that an NFIT entry exists for the DIMM but the device is
 * disabled.  In that case we will still create an nd_dimm, but prevent
 * it from binding to its driver.
 */
int nd_dimm_firmware_status(struct device *dev)
{
	struct nd_dimm *nd_dimm = to_nd_dimm(dev);

	return nd_dimm->nfit_status;
}

static struct nd_dimm *nd_dimm_create(struct nd_bus *nd_bus,
		struct nd_mem *nd_mem)
{
	struct nd_dimm *nd_dimm = kzalloc(sizeof(*nd_dimm), GFP_KERNEL);
	struct nfit_bus_descriptor *nfit_desc = nd_bus->nfit_desc;
	struct device *dev;
	u32 nfit_handle;

	if (!nd_dimm)
		return NULL;

	nd_dimm->del_info = kzalloc(sizeof(struct nd_dimm_delete), GFP_KERNEL);
	if (!nd_dimm->del_info)
		goto err_del_info;
	nd_dimm->del_info->nd_bus = nd_bus;
	nd_dimm->del_info->nd_mem = nd_mem;

	nfit_handle = readl(&nd_mem->nfit_mem->nfit_handle);
	if (radix_tree_insert(&nd_bus->dimm_radix, nfit_handle, nd_dimm) != 0)
		goto err_radix;

	nd_dimm->id = ida_simple_get(&dimm_ida, 0, 0, GFP_KERNEL);
	if (nd_dimm->id < 0)
		goto err_ida;

	nd_dimm->nd_mem = nd_mem;
	dev = &nd_dimm->dev;
	dev_set_name(dev, "nvdimm%d", nd_dimm->id);
	dev->parent = &nd_bus->dev;
	dev->type = &nd_dimm_device_type;
	dev->groups = nd_dimm_attribute_groups;
	dev->devt = MKDEV(nd_dimm_major, nd_dimm->id);
	if (nfit_desc->add_dimm)
		nd_dimm->nfit_status = nfit_desc->add_dimm(nfit_desc, nd_dimm);

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

/**
 * nd_blk_available_dpa - account the unused dpa of BLK region
 * @nd_mapping: container of dpa-resource-root + labels
 *
 * Unlike PMEM, BLK namespaces can occupy discontiguous DPA ranges.
 */
resource_size_t nd_blk_available_dpa(struct nd_mapping *nd_mapping)
{
	resource_size_t map_end, res_end, busy = 0, available;
	struct nd_dimm_drvdata *ndd = to_ndd(nd_mapping);
	struct resource *res;

	map_end = nd_mapping->start + nd_mapping->size;
	for_each_dpa_resource(ndd, res) {
		res_end = res->start + resource_size(res);
		if (res->start >= nd_mapping->start && res->start < map_end) {
			resource_size_t end = min(map_end, res_end);

			busy += end - res->start;
		} else if (res_end >= nd_mapping->start && res_end < map_end) {
			resource_size_t start;

			start = max_t(u64, nd_mapping->start, res->start);
			busy += res_end - start;
		}
	}

	available = map_end - nd_mapping->start;
	if (busy < available)
		return available - busy;
	return 0;
}

/**
 * nd_pmem_available_dpa - for the given dimm+region account unallocated dpa
 * @nd_mapping: container of dpa-resource-root + labels
 * @nd_region: constrain available space check to this reference region
 * @overlap: calculate available space assuming this level of overlap
 *
 * Validate that a PMEM label, if present, aligns with the
 * interleave set end boundary and truncate the available size at the
 * highest BLK overlap point.
 *
 * The expectation is that this routine is called multiple times as it
 * probes for the maximum BLK overlap point for any single member DIMM
 * of the interleave set.  Once that value is determined the
 * PMEM-boundary for the set can be established.
 */
resource_size_t nd_pmem_available_dpa(struct nd_region *nd_region,
		struct nd_mapping *nd_mapping, resource_size_t *overlap)
{
	resource_size_t map_end, res_end, busy = 0, available, blk_max;
	struct nd_dimm_drvdata *ndd = to_ndd(nd_mapping);
	struct resource *res;

	blk_max = nd_mapping->start + *overlap;
	map_end = nd_mapping->start + nd_mapping->size - 1;
	for_each_dpa_resource(ndd, res) {
		res_end = res->start + resource_size(res) - 1;
		if (res->start >= nd_mapping->start && res->start < map_end) {
			if (strncmp(res->name, "blk", 3) == 0)
				blk_max = min(res_end, map_end) + 1;
			else if (res_end != map_end)
				goto err;
			else {
				/* duplicate overlapping PMEM reservations? */
				if (busy)
					goto err;
				busy += resource_size(res);
				continue;
			}
		} else if (res_end >= nd_mapping->start && res_end < map_end) {
			if (strncmp(res->name, "blk", 3) == 0)
				blk_max = min(res_end, map_end) + 1;
			else
				goto err;
		}
	}

	*overlap = blk_max - nd_mapping->start;
	available = map_end + 1 - blk_max;
	if (busy < available)
		return available - busy;
	return 0;

 err:
	/*
	 * Something is wrong, PMEM must align with the end of the
	 * interleave set
	 */
	nd_dbg_dpa(nd_region, ndd, res, "misaligned to iset\n");
	return 0;
}

struct resource *nd_dimm_allocate_dpa(struct nd_dimm_drvdata *ndd,
		struct nd_label_id *label_id, resource_size_t start,
		resource_size_t n)
{
	char *name = devm_kmemdup(ndd->dev, label_id, sizeof(*label_id),
			GFP_KERNEL);
	struct resource *res;

	if (!name)
		return NULL;

	WARN_ON_ONCE(!is_nd_bus_locked(ndd->dev));
	res = __request_region(&ndd->dpa, start, n, name, 0);
	if (!res)
		devm_kfree(ndd->dev, name);
	return res;
}

/**
 * nd_dimm_allocated_dpa - sum up the dpa currently allocated to this label_id
 * @nd_dimm: container of dpa-resource-root + labels
 * @label_id: dpa resource name of the form {pmem|blk}-<human readable uuid>
 */
resource_size_t nd_dimm_allocated_dpa(struct nd_dimm_drvdata *ndd,
		struct nd_label_id *label_id)
{
	resource_size_t allocated = 0;
	struct resource *res;

	for_each_dpa_resource(ndd, res)
		if (strcmp(res->name, label_id->id) == 0)
			allocated += resource_size(res);

	return allocated;
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
		u32 nfit_handle;

		nfit_handle = readl(&nd_mem->nfit_mem->nfit_handle);
		nd_dimm = nd_dimm_by_handle(nd_bus, nfit_handle);
		if (nd_dimm) {
			put_device(&nd_dimm->dev);
			continue;
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
