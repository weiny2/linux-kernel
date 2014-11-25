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
#include <linux/sched.h>
#include <linux/slab.h>
#include "nd-private.h"
#include "nfit.h"
#include "nd.h"

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

static struct device_type nd_volatile_device_type = {
	.name = "nd_volatile",
	.release = nd_region_release,
};

bool is_nd_pmem(struct device *dev)
{
	return dev ? dev->type == &nd_pmem_device_type : false;
}

bool is_nd_blk(struct device *dev)
{
	return dev ? dev->type == &nd_block_device_type : false;
}

struct nd_region *to_nd_region(struct device *dev)
{
	struct nd_region *nd_region = container_of(dev, struct nd_region, dev);

	WARN_ON(dev->type->release != nd_region_release);
	return nd_region;
}

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

static ssize_t spa_index_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nd_region *nd_region = to_nd_region(dev);
	struct nd_spa *nd_spa = nd_region->nd_spa;
	u16 spa_index = readw(&nd_spa->nfit_spa->spa_index);

	return sprintf(buf, "%d\n", spa_index);
}
DEVICE_ATTR_RO(spa_index);

static ssize_t nstype_show(struct device *dev,
                struct device_attribute *attr, char *buf)
{
	struct nd_region *nd_region = to_nd_region(dev);

	return sprintf(buf, "%d\n", nd_region_to_namespace_type(nd_region));
}
DEVICE_ATTR_RO(nstype);

static ssize_t set_state_show(struct device *dev,
                struct device_attribute *attr, char *buf)
{
	struct nd_region *nd_region = to_nd_region(dev);
	struct nd_spa *nd_spa = nd_region->nd_spa;

	if (is_nd_pmem(dev) && nd_spa->nd_set)
		/* pass, should be precluded by nd_region_visible */;
	else
		return -ENXIO;

	/*
	 * The state may be in the process of changing, userspace should quiesce
	 * probing if it wants a static answer
	 */
	return sprintf(buf, "%s\n", nd_spa->nd_set->busy ? "active" : "idle");
}
DEVICE_ATTR_RO(set_state);

static struct attribute *nd_region_attributes[] = {
	&dev_attr_size.attr,
	&dev_attr_nstype.attr,
	&dev_attr_mappings.attr,
	&dev_attr_spa_index.attr,
	&dev_attr_set_state.attr,
	NULL,
};

static umode_t nd_region_visible(struct kobject *kobj, struct attribute *a, int n)
{
	struct device *dev = container_of(kobj, typeof(*dev), kobj);
	struct nd_region *nd_region = to_nd_region(dev);
	struct nd_spa *nd_spa = nd_region->nd_spa;

	if (a != &dev_attr_set_state.attr)
		return a->mode;

	if (is_nd_pmem(dev) && nd_spa->nd_set)
		return a->mode;

	return 0;
}

static struct attribute_group nd_region_attribute_group = {
	.attrs = nd_region_attributes,
	.is_visible = nd_region_visible,
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

static int init_interleave_set(struct nd_bus *nd_bus,
		struct nd_interleave_set *nd_set, struct nd_spa *nd_spa)
{
	u16 spa_index = readw(&nd_spa->nfit_spa->spa_index);
	int num_mappings = num_nd_mem(nd_bus, spa_index);
	int i, rc = -ENXIO;

	for (i = 0; i < num_mappings; i++) {
		struct nd_mem *nd_mem = nd_mem_from_spa(nd_bus, spa_index, i);
		u32 key = to_interleave_set_key(nd_mem);

		if (radix_tree_lookup(&nd_bus->interleave_sets, key)) {
			dev_err(&nd_bus->dev, "%s: duplicate mapping (set: %d key: %#x)\n",
					__func__, spa_index, key);
			rc = -EBUSY;
			break;
		}

		rc = radix_tree_insert(&nd_bus->interleave_sets, key, nd_set);
		if (rc) {
			dev_err(&nd_bus->dev, "%s: failed to add set: %d key: %#x\n",
					__func__, spa_index, key);
			break;
		}
		dev_dbg(&nd_bus->dev, "%s: set: %d key: %#x\n",
				__func__, spa_index, key);
	}

	return rc;
}

int nd_bus_init_interleave_sets(struct nd_bus *nd_bus)
{
	struct nd_spa *nd_spa;
	int rc = 0;

	list_for_each_entry(nd_spa, &nd_bus->spas, list) {
		u16 spa_index = readw(&nd_spa->nfit_spa->spa_index);
		u16 spa_type = readw(&nd_spa->nfit_spa->spa_type);
		struct nd_interleave_set *nd_set;

		if (spa_type != NFIT_SPA_PM)
			continue;
		if (nd_mem_from_spa(nd_bus, spa_index, 0) == NULL)
			continue;
		nd_set = kzalloc(sizeof(*nd_set), GFP_KERNEL);
		if (!nd_set) {
			rc = -ENOMEM;
			break;
		}
		nd_set->spa_index = spa_index;
		nd_spa->nd_set = nd_set;

		rc = init_interleave_set(nd_bus, nd_set, nd_spa);
		if (rc)
			break;
	}
	return rc;
}


/*
 * Upon successful probe/remove, take/release a reference on the
 * associated interleave set (if present)
 */
static void nd_region_notify_driver_action(struct nd_bus *nd_bus,
		struct device *dev, int rc, bool probe)
{
	struct nd_interleave_set *nd_set = NULL;

	if (rc)
		return;

	if (is_nd_pmem(dev)) {
		struct nd_region *nd_region = to_nd_region(dev);

		nd_set = nd_region->nd_spa->nd_set;
	} else if (is_nd_blk(dev)) {
		struct nd_region *nd_region = to_nd_region(dev);
		struct nd_mapping *nd_mapping;
		struct nd_mem *nd_mem;

		nd_region = to_nd_region(dev);
		nd_mapping = &nd_region->mapping[0];
		nd_mem = nd_mapping->nd_dimm->nd_mem;
		nd_set = radix_tree_lookup(&nd_bus->interleave_sets,
				to_interleave_set_key(nd_mem));
	}

	if (!nd_set)
		return;

	if (probe)
		nd_set->busy++;
	else
		nd_set->busy--;

	dev_dbg(dev, "%s: action: %s set: %d busy: %d\n", __func__,
		probe ? "probe" : "remove", nd_set->spa_index,
		nd_set->busy);
}

void nd_region_probe_start(struct nd_bus *nd_bus, struct device *dev)
{
	nd_bus_lock(&nd_bus->dev);
	nd_bus->probe_active++;
	nd_bus_unlock(&nd_bus->dev);
}

void nd_region_probe_end(struct nd_bus *nd_bus, struct device *dev, int rc)
{
	nd_bus_lock(&nd_bus->dev);
	nd_region_notify_driver_action(nd_bus, dev, rc, true);
	if (--nd_bus->probe_active == 0)
		wake_up(&nd_bus->probe_wait);
	nd_bus_unlock(&nd_bus->dev);
}

void nd_region_notify_remove(struct nd_bus *nd_bus, struct device *dev, int rc)
{
	nd_bus_lock(dev);
	nd_region_notify_driver_action(nd_bus, dev, rc, false);
	nd_bus_unlock(dev);
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
	nfit_mem = nd_dimm->nd_mem->nfit_mem;

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
	struct nd_spa *nd_spa = nd_region->nd_spa;
	u16 spa_index = readw(&nd_spa->nfit_spa->spa_index);
	struct nd_bdw *iter, *nd_bdw = NULL;
	struct nd_mapping *nd_mapping;
	struct nd_mem *nd_mem;
	u32 nfit_handle;
	u16 dcr_index;

	nd_region->dev.type = &nd_block_device_type;

	nd_mem = nd_mem_from_spa(nd_bus, spa_index, 0);
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

	nfit_handle = readl(&nd_mem->nfit_mem->nfit_handle);
	nd_mapping = &nd_region->mapping[0];
	nd_mapping->nd_dimm = nd_dimm_by_handle(nd_bus, nfit_handle);
	nd_mapping->size = readq(&nd_bdw->nfit_bdw->dimm_capacity);
	nd_mapping->start = readq(&nd_bdw->nfit_bdw->dimm_block_offset);
}

static void nd_spa_range_init(struct nd_bus *nd_bus, struct nd_region *nd_region,
		struct device_type *type)
{
	u16 i;
	struct nd_mem *nd_mem;
	struct nd_spa *nd_spa = nd_region->nd_spa;
	u16 spa_index = readw(&nd_spa->nfit_spa->spa_index);

	nd_region->dev.type = type;
	for (i = 0; i < nd_region->ndr_mappings; i++) {
		struct nd_mapping *nd_mapping = &nd_region->mapping[i];
		u32 nfit_handle;

		nd_mem = nd_mem_from_spa(nd_bus, spa_index, i);
		nfit_handle = readl(&nd_mem->nfit_mem->nfit_handle);
		nd_mapping->nd_dimm = nd_dimm_by_handle(nd_bus, nfit_handle);
		nd_mapping->start = readq(&nd_mem->nfit_mem->region_dpa);
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
	nd_region->nd_spa = nd_spa;
	nd_region->ndr_mappings = num_mappings;
	dev = &nd_region->dev;
	dev_set_name(dev, "region%d", nd_region->id);
	dev->parent = &nd_bus->dev;
	dev->groups = nd_region_attribute_groups;
	nd_region->ndr_size = readq(&nd_spa->nfit_spa->spa_length);
	nd_region->ndr_start = readq(&nd_spa->nfit_spa->spa_base);
	switch (spa_type) {
	case NFIT_SPA_PM:
		nd_spa_range_init(nd_bus, nd_region, &nd_pmem_device_type);
		break;
	case NFIT_SPA_VOLATILE:
		nd_spa_range_init(nd_bus, nd_region, &nd_volatile_device_type);
		break;
	case NFIT_SPA_DCR:
		nd_blk_init(nd_bus, nd_region);
		break;
	default:
		break;
	}
	nd_device_register(dev);

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
		case NFIT_SPA_VOLATILE:
			nd_region = nd_region_create(nd_bus, nd_spa);
			if (!nd_region)
				rc = -ENOMEM;
			break;
		case NFIT_SPA_BDW:
			/* we'll consume this in nd_blk_register for the DCR */
			break;
		default:
			dev_dbg(&nd_bus->dev, "spa[%d] unknown type: %d\n",
					spa_index, spa_type);
			break;
		}
	}
	mutex_unlock(&nd_bus_list_mutex);

	nd_synchronize();

	return rc;
}
