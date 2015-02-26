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
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/nd.h>
#include "nd-private.h"
#include "nd.h"

#include <asm-generic/io-64-nonatomic-lo-hi.h>

static void namespace_io_release(struct device *dev)
{
	struct nd_namespace_io *nsio = to_nd_namespace_io(dev);

	kfree(nsio);
}

static void namespace_pmem_release(struct device *dev)
{
	struct nd_namespace_pmem *nspm = to_nd_namespace_pmem(dev);

	kfree(nspm->alt_name);
	kfree(nspm->uuid);
	kfree(nspm);
}

static void namespace_blk_release(struct device *dev)
{
	struct nd_namespace_blk *nsblk = to_nd_namespace_blk(dev);
	struct nd_region *nd_region = to_nd_region(dev->parent);

	if (nsblk->id >= 0)
		ida_simple_remove(&nd_region->ns_ida, nsblk->id);
	kfree(nsblk->bus_private_data);
	kfree(nsblk->alt_name);
	kfree(nsblk->uuid);
	kfree(nsblk->res);
	kfree(nsblk);
}

static struct device_type namespace_io_device_type = {
	.name = "nd_namespace_io",
	.release = namespace_io_release,
};

static struct device_type namespace_pmem_device_type = {
	.name = "nd_namespace_pmem",
	.release = namespace_pmem_release,
};

static struct device_type namespace_blk_device_type = {
	.name = "nd_namespace_blk",
	.release = namespace_blk_release,
};

static bool is_namespace_pmem(struct device *dev)
{
	return dev ? dev->type == &namespace_pmem_device_type : false;
}

static bool is_namespace_blk(struct device *dev)
{
	return dev ? dev->type == &namespace_blk_device_type : false;
}

static bool is_namespace_io(struct device *dev)
{
	return dev ? dev->type == &namespace_io_device_type : false;
}

static ssize_t type_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nd_region *nd_region = to_nd_region(dev->parent);

	return sprintf(buf, "%d\n", nd_region_to_namespace_type(nd_region));
}
static DEVICE_ATTR_RO(type);

static ssize_t __alt_name_store(struct device *dev, const char *buf,
		const size_t len)
{
	char *input, *pos, *alt_name, **ns_altname;
	ssize_t rc;

	if (is_namespace_pmem(dev)) {
		struct nd_namespace_pmem *nspm = to_nd_namespace_pmem(dev);

		ns_altname = &nspm->alt_name;
	} else if (is_namespace_blk(dev)) {
		struct nd_namespace_blk *nsblk = to_nd_namespace_blk(dev);

		ns_altname = &nsblk->alt_name;
	} else
		return -ENXIO;

	if (dev->driver)
		return -EBUSY;

	input = kmemdup(buf, len + 1, GFP_KERNEL);
	if (!input)
		return -ENOMEM;

	input[len] = '\0';
	pos = strim(input);
	if (strlen(pos) + 1 > NSLABEL_NAME_LEN) {
		rc = -EINVAL;
		goto out;
	}

	alt_name = kzalloc(NSLABEL_NAME_LEN, GFP_KERNEL);
	if (!alt_name) {
		rc = -ENOMEM;
		goto out;
	}
	kfree(*ns_altname);
	*ns_altname = alt_name;
	sprintf(*ns_altname, "%s", pos);
	rc = len;

out:
	kfree(input);
	return rc;
}

static resource_size_t nd_namespace_blk_size(struct nd_namespace_blk *nsblk)
{
	struct nd_region *nd_region = to_nd_region(nsblk->dev.parent);
	struct nd_mapping *nd_mapping = &nd_region->mapping[0];
	struct nd_dimm_drvdata *ndd = to_ndd(nd_mapping);
	struct nd_label_id label_id;
	resource_size_t size = 0;
	struct resource *res;

	if (!nsblk->uuid)
		return 0;
	nd_label_gen_id(&label_id, nsblk->uuid, NSLABEL_FLAG_LOCAL);
	for_each_dpa_resource(ndd, res)
		if (strcmp(res->name, label_id.id) == 0)
			size += resource_size(res);
	return size;
}

resource_size_t nd_namespace_blk_validate(struct nd_namespace_blk *nsblk)
{
	struct nd_region *nd_region = to_nd_region(nsblk->dev.parent);
	struct nd_mapping *nd_mapping = &nd_region->mapping[0];
	struct nd_dimm_drvdata *ndd = to_ndd(nd_mapping);
	struct nd_label_id label_id;
	struct resource *res;
	int count, i;

	if (!nsblk->uuid || !nsblk->lbasize)
		return 0;

	count = 0;
	nd_label_gen_id(&label_id, nsblk->uuid, NSLABEL_FLAG_LOCAL);
	for_each_dpa_resource(ndd, res) {
		if (strcmp(res->name, label_id.id) != 0)
			continue;
		/*
		 * Resources with unacknoweldged adjustments indicate a
		 * failure to update labels
		 */
		if (res->flags & DPA_RESOURCE_ADJUSTED)
			return 0;
		count++;
	}

	/* These values match after a successful label update */
	if (count != nsblk->num_resources)
		return 0;
		
	for (i = 0; i < nsblk->num_resources; i++) {
		struct resource *found = NULL;

		for_each_dpa_resource(ndd, res)
			if (res == nsblk->res[i]) {
				found = res;
				break;
			}
		/* stale resource */
		if (!found)
			return 0;
	}

	return nd_namespace_blk_size(nsblk);
}
EXPORT_SYMBOL(nd_namespace_blk_validate);

static int nd_namespace_label_update(struct nd_region *nd_region, struct device *dev)
{
	dev_WARN_ONCE(dev, dev->driver,
			"namespace must be idle during label update\n");
	if (dev->driver)
		return 0;

	/*
	 * Only allow label writes that will result in a valid namespace
	 * or deletion of an existing namespace.
	 */
	if (is_namespace_pmem(dev)) {
		struct nd_namespace_pmem *nspm = to_nd_namespace_pmem(dev);
		resource_size_t size = resource_size(&nspm->nsio.res);

		if (size == 0 && nspm->uuid)
			/* delete allocation */;
		else if (!nspm->uuid)
			return 0;

		return nd_pmem_namespace_label_update(nd_region, nspm, size);
	} else if (is_namespace_blk(dev)) {
		struct nd_namespace_blk *nsblk = to_nd_namespace_blk(dev);
		resource_size_t size = nd_namespace_blk_size(nsblk);

		if (size == 0 && nsblk->uuid)
			/* delete allocation */;
		else if (!nsblk->uuid || !nsblk->lbasize)
			return 0;

		return nd_blk_namespace_label_update(nd_region, nsblk, size);
	} else
		return -ENXIO;
}

static ssize_t alt_name_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	struct nd_region *nd_region = to_nd_region(dev->parent);
	ssize_t rc;

	device_lock(dev);
	nd_bus_lock(dev);
	wait_nd_bus_probe_idle(dev);
	rc = __alt_name_store(dev, buf, len);
	if (rc >= 0)
		rc = nd_namespace_label_update(nd_region, dev);
	dev_dbg(dev, "%s: %s (%zd)\n", __func__, rc < 0 ? "fail" : "success", rc);
	nd_bus_unlock(dev);
	device_unlock(dev);

	return rc < 0 ? rc : len;
}

static ssize_t alt_name_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char *ns_altname;

	if (is_namespace_pmem(dev)) {
		struct nd_namespace_pmem *nspm = to_nd_namespace_pmem(dev);

		ns_altname = nspm->alt_name;
	} else if (is_namespace_blk(dev)) {
		struct nd_namespace_blk *nsblk = to_nd_namespace_blk(dev);

		ns_altname = nsblk->alt_name;
	} else
		return -ENXIO;

	return sprintf(buf, "%s\n", ns_altname ? ns_altname : "");
}
static DEVICE_ATTR_RW(alt_name);

static int scan_free(struct nd_region *nd_region,
		struct nd_mapping *nd_mapping, struct nd_label_id *label_id,
		resource_size_t n)
{
	bool is_pmem = strncmp(label_id->id, "pmem", 4) == 0;
	struct nd_dimm_drvdata *ndd = to_ndd(nd_mapping);
	int rc = 0;

	while (n) {
		struct resource *res, *last;
		resource_size_t new_start;

		last = NULL;
		for_each_dpa_resource(ndd, res)
			if (strcmp(res->name, label_id->id) == 0)
				last = res;
		res = last;
		if (!res)
			return 0;

		if (n >= resource_size(res)) {
			n -= resource_size(res);
			nd_dbg_dpa(nd_region, ndd, res, "delete %d\n", rc);
			devm_kfree(ndd->dev, (void *) res->name);
			__release_region(&ndd->dpa, res->start,
					resource_size(res));
			/* retry with last resource deleted */
			continue;
		}

		if (is_pmem)
			new_start = res->start + n;
		else
			new_start = res->start;

		rc = adjust_resource(res, new_start, resource_size(res) - n);
		if (rc == 0)
			res->flags |= DPA_RESOURCE_ADJUSTED;
		nd_dbg_dpa(nd_region, ndd, res, "shrink %d\n", rc);
		break;
	}

	return rc;
}

/**
 * shrink_dpa_allocation - for each dimm in region free n bytes for label_id
 * @nd_region: the set of dimms to reclaim @n bytes from
 * @label_id: unique identifier for the namespace consuming this dpa range
 * @n: number of bytes per-dimm to release
 *
 * Assumes resources are ordered.  Starting from the end try to
 * adjust_resource() the allocation to @n, but if @n is larger than the
 * allocation delete it and find the 'new' last allocation in the label
 * set.
 */
static int shrink_dpa_allocation(struct nd_region *nd_region,
		struct nd_label_id *label_id, resource_size_t n)
{
	int i;

	for (i = 0; i < nd_region->ndr_mappings; i++) {
		struct nd_mapping *nd_mapping = &nd_region->mapping[i];
		int rc;

		rc = scan_free(nd_region, nd_mapping, label_id, n);
		if (rc)
			return rc;
	}

	return 0;
}

static int init_dpa_allocation(struct nd_label_id *label_id,
		struct nd_region *nd_region, struct nd_mapping *nd_mapping,
		resource_size_t n)
{
	bool is_pmem = strncmp(label_id->id, "pmem", 4) == 0;
	struct nd_dimm_drvdata *ndd = to_ndd(nd_mapping);
	resource_size_t first_dpa;
	struct resource *res;
	int rc = 0;

	if (is_pmem)
		first_dpa = nd_mapping->start + nd_mapping->size - n;
	else
		first_dpa = nd_mapping->start;
	/* first resource allocation for this label-id or dimm */
	res = nd_dimm_allocate_dpa(ndd, label_id, first_dpa, n);
	if (!res)
		rc = -EBUSY;

	nd_dbg_dpa(nd_region, ndd, res, "init %d\n", rc);
	return rc;
}

static bool space_valid(bool is_pmem, struct nd_label_id *label_id,
		struct resource *res)
{
	/*
	 * For BLK-space any space is valid, for PMEM-space, it must be
	 * contiguous with an existing allocation.  Of course, initial
	 * PMEM allocations do not use this helper.
	 */
	if (!is_pmem)
		return true;
	if (strcmp(res ? res->name : "", label_id->id) == 0)
		return true;
	return false;
}

enum alloc_loc {
	ALLOC_ERR = 0, ALLOC_BEFORE, ALLOC_MID, ALLOC_AFTER,
};

static int scan_allocate(struct nd_region *nd_region,
		struct nd_mapping *nd_mapping, struct nd_label_id *label_id,
		resource_size_t n)
{
	resource_size_t mapping_end = nd_mapping->start + nd_mapping->size - 1;
	bool is_pmem = strncmp(label_id->id, "pmem", 4) == 0;
	struct nd_dimm_drvdata *ndd = to_ndd(nd_mapping);
	const resource_size_t to_allocate = n;
	struct resource *res;
	int first;

 retry:
	first = 0;
	for_each_dpa_resource(ndd, res) {
		resource_size_t allocate, available = 0, free_start, free_end;
		struct resource *next = res->sibling, *new_res = NULL;
		enum alloc_loc loc = ALLOC_ERR;
		const char *action;
		int rc = 0;

		/* ignore resources outside this nd_mapping */
		if (res->start >= mapping_end)
			continue;
		if (res->start + resource_size(res) - 1 < nd_mapping->start)
			continue;

		/* space at the beginning of the mapping */
		if (!first++ && res->start > nd_mapping->start) {
			free_start = nd_mapping->start;
			available = res->start - free_start;
			if (space_valid(is_pmem, label_id, res))
				loc = ALLOC_BEFORE;
		}

		/* space between allocations */
		if (!loc && next) {
			free_start = res->start + resource_size(res);
			free_end = min(mapping_end, next->start - 1);
			if (free_start < free_end)
				available = free_end + 1 - free_start;
			if (space_valid(is_pmem, label_id, next))
				loc = ALLOC_MID;
		}

		/* space at the end of the mapping */
		if (!loc && !next) {
			free_start = res->start + resource_size(res);
			free_end = mapping_end;
			if (free_start < free_end)
				available = free_end + 1 - free_start;
			if (space_valid(is_pmem, label_id, next))
				loc = ALLOC_AFTER;
		}

		if (!loc || !available)
			continue;
		allocate = min(available, n);
		switch (loc) {
		case ALLOC_BEFORE:
			if (strcmp(res->name, label_id->id) == 0) {
				/* adjust current resource up */
				rc = adjust_resource(res, res->start - allocate,
						resource_size(res) + allocate);
				action = "cur grow up";
			} else
				action = "allocate";
			break;
		case ALLOC_MID:
			if (strcmp(next->name, label_id->id) == 0) {
				/* adjust next resource up */
				rc = adjust_resource(next, next->start
						- allocate, resource_size(next)
						+ allocate);
				new_res = next;
				action = "next grow up";
			} else if (strcmp(res->name, label_id->id) == 0) {
				action = "grow down";
			} else
				action = "allocate";
			break;
		case ALLOC_AFTER:
			if (strcmp(res->name, label_id->id) == 0) {
				action = "grow down";
			} else
				action = "allocate";
			break;
		default:
			goto err;
		}

		if (strcmp(action, "allocate") == 0) {
			/* insert new resource */
			if (is_pmem)
				goto err;
			new_res = nd_dimm_allocate_dpa(ndd, label_id,
					free_start, allocate);
			if (!new_res)
				rc = -EBUSY;
		} else if (strcmp(action, "grow down") == 0) {
			/* adjust current resource down */
			if (is_pmem)
				goto err;
			rc = adjust_resource(res, res->start, resource_size(res)
					+ allocate);
			if (rc == 0)
				res->flags |= DPA_RESOURCE_ADJUSTED;
		}

		if (!new_res)
			new_res = res;

		nd_dbg_dpa(nd_region, ndd, new_res, "%s(%d) %d\n",
				action, loc, rc);

		if (rc)
			return rc;

		n -= allocate;
		if (n) {
			/*
			 * Retry scan with newly inserted resources.
			 * For example, if we did an ALLOC_BEFORE
			 * insertion there may also have been space
			 * available for an ALLOC_AFTER insertion, so we
			 * need to check this same resource again
			 */
			goto retry;
		} else
			return 0;
	}

	if (n == to_allocate)
		return init_dpa_allocation(label_id, nd_region, nd_mapping, n);

 err:
	dev_WARN_ONCE(&nd_region->dev, 1,
			"allocation underrun: %#llx of %#llx bytes\n",
			(unsigned long long) to_allocate - n,
			(unsigned long long) to_allocate);
	return -ENXIO;
}

static int merge_dpa(struct nd_region *nd_region,
		struct nd_mapping *nd_mapping, struct nd_label_id *label_id)
{
	struct nd_dimm_drvdata *ndd = to_ndd(nd_mapping);
	struct resource *res;

	if (strncmp("pmem", label_id->id, 4) == 0)
		return 0;
 retry:
	for_each_dpa_resource(ndd, res) {
		int rc;
		struct resource *next = res->sibling;
		resource_size_t end = res->start + resource_size(res);

		if (!next || strcmp(res->name, label_id->id) != 0
				|| strcmp(next->name, label_id->id) != 0
				|| end != next->start)
			continue;
		end += resource_size(next);
		__release_region(&ndd->dpa, next->start, resource_size(next));
		rc = adjust_resource(res, res->start, end - res->start);
		nd_dbg_dpa(nd_region, ndd, res, "merge %d\n", rc);
		if (rc)
			return rc;
		res->flags |= DPA_RESOURCE_ADJUSTED;
		goto retry;
	}

	return 0;
}

/**
 * grow_dpa_allocation - for each dimm allocate n bytes for @label_id
 * @nd_region: the set of dimms to allocate @n more bytes from
 * @label_id: unique identifier for the namespace consuming this dpa range
 * @n: number of bytes per-dimm to add to the existing allocation
 *
 * Assumes resources are ordered.  For BLK regions, starting from the
 * beginning try to adjust the resource to the next allocation boundary.
 * For PMEM regions start allocations from the bottom of the interleave
 * set.
 */
static int grow_dpa_allocation(struct nd_region *nd_region,
		struct nd_label_id *label_id, resource_size_t n)
{
	int i;

	for (i = 0; i < nd_region->ndr_mappings; i++) {
		struct nd_mapping *nd_mapping = &nd_region->mapping[i];
		int rc;

		rc = scan_allocate(nd_region, nd_mapping, label_id, n);
		if (rc)
			return rc;
		rc = merge_dpa(nd_region, nd_mapping, label_id);
		if (rc)
			return rc;
	}

	return 0;
}

static void nd_namespace_pmem_set_size(struct nd_region *nd_region,
		struct nd_namespace_pmem *nspm, resource_size_t size)
{
	struct resource *res = &nspm->nsio.res;

	res->end = nd_region->ndr_start + nd_region->ndr_size - 1;
	res->start = res->end + 1 - size;
}

static ssize_t __size_store(struct device *dev, const char *buf)
{
	resource_size_t allocated = 0, available = 0;
	struct nd_region *nd_region = to_nd_region(dev->parent);
	struct nd_mapping *nd_mapping;
	struct nd_dimm_drvdata *ndd;
	struct nd_label_id label_id;
	unsigned long long val;
	u8 *uuid = NULL;
	u32 flags =  0;
	int rc, i;

	if (dev->driver)
		return -EBUSY;

	if (is_namespace_pmem(dev)) {
		struct nd_namespace_pmem *nspm = to_nd_namespace_pmem(dev);

		uuid = nspm->uuid;
	} else if (is_namespace_blk(dev)) {
		struct nd_namespace_blk *nsblk = to_nd_namespace_blk(dev);

		uuid = nsblk->uuid;
		flags = NSLABEL_FLAG_LOCAL;
	}

	/*
	 * We need a uuid for the allocation-label and dimm(s) on which
	 * to store the label.
	 */
	if (!uuid || nd_region->ndr_mappings == 0)
		return -ENXIO;

	rc = kstrtoull(buf, 0, &val);
	if (rc)
		return rc;

	if (val % (SZ_4K * nd_region->ndr_mappings)) {
		dev_dbg(dev, "%llu is not %dK aligned\n", val,
				(SZ_4K * nd_region->ndr_mappings) / SZ_1K);
		return -EINVAL;
	}

	nd_label_gen_id(&label_id, uuid, flags);
	for (i = 0; i < nd_region->ndr_mappings; i++) {
		nd_mapping = &nd_region->mapping[i];
		ndd = to_ndd(nd_mapping);

		/*
		 * All dimms in an interleave set, or the base dimm for a blk
		 * region, need to be enabled for the size to be changed.
		 */
		if (!ndd)
			return -ENXIO;

		allocated += nd_dimm_allocated_dpa(ndd, &label_id);
	}
	available = nd_region_available_dpa(nd_region);

	if (val > available + allocated)
		return -ENOSPC;

	if (val == allocated)
		return 0;

	val /= nd_region->ndr_mappings;
	allocated /= nd_region->ndr_mappings;

	if (val < allocated)
		rc = shrink_dpa_allocation(nd_region, &label_id, allocated - val);
	else
		rc = grow_dpa_allocation(nd_region, &label_id, val - allocated);

	if (rc)
		return rc;

	if (is_namespace_pmem(dev)) {
		struct nd_namespace_pmem *nspm = to_nd_namespace_pmem(dev);

		nd_namespace_pmem_set_size(nd_region, nspm,
				val * nd_region->ndr_mappings);
	} else if (is_namespace_blk(dev)) {
		/*
		 * Try to delete the namespace if we deleted all of its
		 * allocation and this is not the seed device for the
		 * region.
		 */
		if (val == 0 && nd_region->ns_seed != dev)
			nd_device_unregister(dev, ND_ASYNC);
	}

	return rc;
}

static ssize_t size_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	struct nd_region *nd_region = to_nd_region(dev->parent);
	int rc;

	device_lock(dev);
	nd_bus_lock(dev);
	wait_nd_bus_probe_idle(dev);
	rc = __size_store(dev, buf);
	if (rc >= 0)
		rc = nd_namespace_label_update(nd_region, dev);
	dev_dbg(dev, "%s: %s %s (%d)\n", __func__, buf,
			rc < 0 ? "fail" : "success", rc);
	nd_bus_unlock(dev);
	device_unlock(dev);

	return rc < 0 ? rc : len;
}

static ssize_t size_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned long long size = 0;

	nd_bus_lock(dev);
	if (is_namespace_pmem(dev)) {
		struct nd_namespace_pmem *nspm = to_nd_namespace_pmem(dev);

		size = resource_size(&nspm->nsio.res);
	} else if (is_namespace_blk(dev)) {
		size = nd_namespace_blk_size(to_nd_namespace_blk(dev));
	} else if (is_namespace_io(dev)) {
		struct nd_namespace_io *nsio = to_nd_namespace_io(dev);

		size = resource_size(&nsio->res);
	}
	nd_bus_unlock(dev);

	return sprintf(buf, "%llu\n", size);
}
static DEVICE_ATTR(size, S_IRUGO, size_show, size_store);

static ssize_t uuid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u8 *uuid;

	if (is_namespace_pmem(dev)) {
		struct nd_namespace_pmem *nspm = to_nd_namespace_pmem(dev);

		uuid = nspm->uuid;
	} else if (is_namespace_blk(dev)) {
		struct nd_namespace_blk *nsblk = to_nd_namespace_blk(dev);

		uuid = nsblk->uuid;
	} else
		return -ENXIO;

	if (uuid)
		return sprintf(buf, "%pUb\n", uuid);
	return sprintf(buf, "\n");
}

/**
 * namespace_update_uuid - check for a unique uuid and whether we're "renaming"
 * @nd_region: parent region so we can updates all dimms in the set
 * @dev: namespace type for generating label_id
 * @new_uuid: incoming uuid
 * @old_uuid: reference to the uuid storage location in the namespace object
 */
static int namespace_update_uuid(struct nd_region *nd_region,
		struct device *dev, u8 *new_uuid, u8 **old_uuid)
{
	u32 flags = is_namespace_blk(dev) ? NSLABEL_FLAG_LOCAL : 0;
	struct nd_label_id old_label_id;
	struct nd_label_id new_label_id;
	int i;

	if (!nd_is_uuid_unique(dev, new_uuid))
		return -EINVAL;

	if (*old_uuid == NULL)
		goto out;

	/*
	 * If we've already written a label with this uuid, then it's
	 * too late to rename because we can't reliably update the uuid
	 * without losing the old namespace.  Userspace must delete this
	 * namespace to abandon the old uuid.
	 */
	for (i = 0; i < nd_region->ndr_mappings; i++) {
		struct nd_mapping *nd_mapping = &nd_region->mapping[i];

		/*
		 * This check by itself is sufficient because old_uuid
		 * would be NULL above if this uuid did not exist in the
		 * currently written set.
		 *
		 * FIXME: can we delete uuid with zero dpa allocated?
		 */
		if (nd_mapping->labels)
			return -EBUSY;
	}

	nd_label_gen_id(&old_label_id, *old_uuid, flags);
	nd_label_gen_id(&new_label_id, new_uuid, flags);
	for (i = 0; i < nd_region->ndr_mappings; i++) {
		struct nd_mapping *nd_mapping = &nd_region->mapping[i];
		struct nd_dimm_drvdata *ndd = to_ndd(nd_mapping);
		struct resource *res;

		for_each_dpa_resource(ndd, res)
			if (strcmp(res->name, old_label_id.id) == 0)
				sprintf((void *) res->name, "%s",
						new_label_id.id);
	}
	kfree(*old_uuid);
 out:
	*old_uuid = new_uuid;
	return 0;
}

static ssize_t uuid_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	struct nd_region *nd_region = to_nd_region(dev->parent);
	u8 *uuid = NULL;
	u8 **ns_uuid;
	ssize_t rc;

	if (is_namespace_pmem(dev)) {
		struct nd_namespace_pmem *nspm = to_nd_namespace_pmem(dev);

		ns_uuid = &nspm->uuid;
	} else if (is_namespace_blk(dev)) {
		struct nd_namespace_blk *nsblk = to_nd_namespace_blk(dev);

		ns_uuid = &nsblk->uuid;
	} else
		return -ENXIO;

	device_lock(dev);
	nd_bus_lock(dev);
	wait_nd_bus_probe_idle(dev);
	rc = nd_uuid_store(dev, &uuid, buf, len);
	if (rc >= 0)
		rc = namespace_update_uuid(nd_region, dev, uuid, ns_uuid);
	if (rc >= 0)
		rc = nd_namespace_label_update(nd_region, dev);
	else
		kfree(uuid);
	dev_dbg(dev, "%s: result: %zd wrote: %s%s", __func__,
			rc, buf, buf[len - 1] == '\n' ? "" : "\n");
	nd_bus_unlock(dev);
	device_unlock(dev);

	return rc < 0 ? rc : len;
}
static DEVICE_ATTR_RW(uuid);

static const unsigned long ns_lbasize_supported[] = { 512, 0 };

static ssize_t sector_size_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nd_namespace_blk *nsblk = to_nd_namespace_blk(dev);

	if (!is_namespace_blk(dev))
		return -ENXIO;

	return nd_sector_size_show(nsblk->lbasize, ns_lbasize_supported, buf);
}

static ssize_t sector_size_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	struct nd_namespace_blk *nsblk = to_nd_namespace_blk(dev);
	struct nd_region *nd_region = to_nd_region(dev->parent);
	ssize_t rc;

	if (!is_namespace_blk(dev))
		return -ENXIO;

	device_lock(dev);
	nd_bus_lock(dev);
	rc = nd_sector_size_store(dev, buf, &nsblk->lbasize,
			ns_lbasize_supported);
	if (rc >= 0)
		rc = nd_namespace_label_update(nd_region, dev);
	dev_dbg(dev, "%s: result: %zd %s: %s%s", __func__,
			rc, rc < 0 ? "tried" : "wrote", buf,
			buf[len - 1] == '\n' ? "" : "\n");
	nd_bus_unlock(dev);
	device_unlock(dev);

	return rc ? rc : len;
}
static DEVICE_ATTR_RW(sector_size);

static ssize_t resource_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct resource *res;

	if (is_namespace_pmem(dev)) {
		struct nd_namespace_pmem *nspm = to_nd_namespace_pmem(dev);

		res = &nspm->nsio.res;
	} else if (is_namespace_io(dev)) {
		struct nd_namespace_io *nsio = to_nd_namespace_io(dev);

		res = &nsio->res;
	} else
		return -ENXIO;

	/* no address to convey if the namespace has no allocation */
	if (resource_size(res) == 0)
		return -ENXIO;
	return sprintf(buf, "%#llx\n", (unsigned long long) res->start);
}
static DEVICE_ATTR_RO(resource);

static struct attribute *nd_namespace_attributes[] = {
	&dev_attr_type.attr,
	&dev_attr_size.attr,
	&dev_attr_uuid.attr,
	&dev_attr_resource.attr,
	&dev_attr_alt_name.attr,
	&dev_attr_sector_size.attr,
	NULL,
};

static umode_t nd_namespace_attr_visible(struct kobject *kobj, struct attribute *a, int n)
{
	struct device *dev = container_of(kobj, struct device, kobj);

	if (a == &dev_attr_resource.attr) {
		if (is_namespace_blk(dev))
			return 0;
		return a->mode;
	}

	if (is_namespace_pmem(dev) || is_namespace_blk(dev)) {
		if (a == &dev_attr_size.attr)
			return S_IWUSR;

		if (is_namespace_pmem(dev) && a == &dev_attr_sector_size.attr)
			return 0;

		return a->mode;
	}

	if (a == &dev_attr_type.attr || a == &dev_attr_size.attr)
		return a->mode;

	return 0;
}

static struct attribute_group nd_namespace_attribute_group = {
	.attrs = nd_namespace_attributes,
	.is_visible = nd_namespace_attr_visible,
};

static const struct attribute_group *nd_namespace_attribute_groups[] = {
	&nd_device_attribute_group,
	&nd_namespace_attribute_group,
	NULL,
};

static struct device **create_namespace_io(struct nd_region *nd_region)
{
	struct nd_namespace_io *nsio;
	struct device *dev, **devs;
	struct resource *res;

	nsio = kzalloc(sizeof(*nsio), GFP_KERNEL);
	if (!nsio)
		return NULL;

	devs = kcalloc(2, sizeof(struct device *), GFP_KERNEL);
	if (!devs) {
		kfree(nsio);
		return NULL;
	}

	dev = &nsio->dev;
	dev->type = &namespace_io_device_type;
	res = &nsio->res;
	res->name = dev_name(&nd_region->dev);
	res->flags = IORESOURCE_MEM;
	res->start = nd_region->ndr_start;
	res->end = res->start + nd_region->ndr_size - 1;

	devs[0] = dev;
	return devs;
}

static bool has_uuid_at_pos(struct nd_region *nd_region, u8 *uuid, u64 cookie, u16 pos)
{
	struct nd_namespace_label __iomem *found = NULL;
	int i;

	for (i = 0; i < nd_region->ndr_mappings; i++) {
		struct nd_mapping *nd_mapping = &nd_region->mapping[i];
		struct nd_namespace_label __iomem *nd_label;
		u8 *found_uuid = NULL;
		int l;

		for_each_label(l, nd_label, nd_mapping->labels) {
			u64 isetcookie = readq(&nd_label->isetcookie);
			u16 position = readw(&nd_label->position);
			u16 nlabel = readw(&nd_label->nlabel);
			u8 label_uuid[NSLABEL_UUID_LEN];

			if (isetcookie != cookie)
				continue;

			memcpy_fromio(label_uuid, nd_label->uuid,
					NSLABEL_UUID_LEN);
			if (memcmp(label_uuid, uuid, NSLABEL_UUID_LEN) != 0)
				continue;

			if (found_uuid) {
				dev_dbg(to_ndd(nd_mapping)->dev,
						"%s duplicate entry for uuid\n",
						__func__);
				return false;
			}
			found_uuid = label_uuid;
			if (nlabel != nd_region->ndr_mappings)
				continue;
			if (position != pos)
				continue;
			found = nd_label;
			break;
		}
		if (found)
			break;
	}
	return found != NULL;
}

static int select_pmem_uuid(struct nd_region *nd_region, u8 *pmem_uuid)
{
	struct nd_namespace_label __iomem *select = NULL;
	int i;

	if (!pmem_uuid)
		return -ENODEV;

	for (i = 0; i < nd_region->ndr_mappings; i++) {
		struct nd_mapping *nd_mapping = &nd_region->mapping[i];
		struct nd_namespace_label __iomem *nd_label;
		u64 hw_start, hw_end, pmem_start, pmem_end;
		int l;

		for_each_label(l, nd_label, nd_mapping->labels) {
			u8 label_uuid[NSLABEL_UUID_LEN];

			memcpy_fromio(label_uuid, nd_label->uuid,
					NSLABEL_UUID_LEN);
			if (memcmp(label_uuid, pmem_uuid, NSLABEL_UUID_LEN) == 0)
				break;
		}

		if (!nd_label) {
			WARN_ON(1);
			return -EINVAL;
		}

		select = nd_label;
		/*
		 * Check that this label is compliant with the dpa
		 * range published in NFIT
		 */
		hw_start = nd_mapping->start;
		hw_end = hw_start + nd_mapping->size;
		pmem_start = readq(&select->dpa);
		pmem_end = pmem_start + readq(&select->rawsize);
		if (pmem_end == hw_end && pmem_start >= hw_start
				&& pmem_start < hw_end)
			/* pass */;
		else
			return -EINVAL;

		nd_set_label(nd_mapping->labels, select, 0);
		nd_set_label(nd_mapping->labels, (void __iomem *) NULL, 1);
	}
	return 0;
}

/**
 * find_pmem_label_set - validate interleave set labelling, retrieve label0
 * @nd_region: region with mappings to validate
 */
static int find_pmem_label_set(struct nd_region *nd_region,
		struct nd_namespace_pmem *nspm)
{
	u64 cookie = nd_region_interleave_set_cookie(nd_region);
	struct nd_namespace_label __iomem *nd_label;
	u8 select_uuid[NSLABEL_UUID_LEN];
	resource_size_t size = 0;
	u8 *pmem_uuid = NULL;
	int rc = -ENODEV, l;
	u16 i;

	if (cookie == 0)
		return -ENXIO;

	/*
	 * Find a complete set of labels by uuid.  By definition we can start
	 * with any mapping as the reference label
	 */
	for_each_label(l, nd_label, nd_region->mapping[0].labels) {
		u64 isetcookie = readq(&nd_label->isetcookie);
		u8 label_uuid[NSLABEL_UUID_LEN];

		if (isetcookie != cookie)
			continue;

		memcpy_fromio(label_uuid, nd_label->uuid,
				NSLABEL_UUID_LEN);
		for (i = 0; nd_region->ndr_mappings; i++)
			if (!has_uuid_at_pos(nd_region, label_uuid, cookie, i))
				break;
		if (i < nd_region->ndr_mappings) {
			/*
			 * Give up if we don't find an instance of a
			 * uuid at each position (from 0 to
			 * nd_region->ndr_mappings - 1), or if we find a
			 * dimm with two instances of the same uuid.
			 */
			rc = -EINVAL;
			goto err;
		} else if (pmem_uuid) {
			/*
			 * If there is more than one valid uuid set, we
			 * need userspace to clean this up.
			 */
			rc = -EBUSY;
			goto err;
		}
		memcpy(select_uuid, label_uuid, NSLABEL_UUID_LEN);
		pmem_uuid = select_uuid;
	}

	/*
	 * Fix up each mapping's 'labels' to have the validated pmem label for
	 * that position at labels[0], and NULL at labels[1].  In the process,
	 * check that the namespace aligns with interleave-set.  We know
	 * that it does not overlap with any blk namespaces by virtue of
	 * the dimm being enabled (i.e. nd_label_reserve_dpa()
	 * succeeded).
	 */
	rc = select_pmem_uuid(nd_region, pmem_uuid);
	if (rc)
		goto err;

	/* Calculate total size and populate namespace properties from label0 */
	for (i = 0; i < nd_region->ndr_mappings; i++) {
		struct nd_mapping *nd_mapping = &nd_region->mapping[i];
		struct nd_namespace_label __iomem *label0;

		label0 = nd_get_label(nd_mapping->labels, 0);
		size += readq(&label0->rawsize);
		if (readl(&label0->position) != 0)
			continue;
		WARN_ON(nspm->alt_name || nspm->uuid);
		nspm->alt_name = kmemdup((void __force *) label0->name,
				NSLABEL_NAME_LEN, GFP_KERNEL);
		nspm->uuid = kmemdup((void __force *) label0->uuid,
				NSLABEL_UUID_LEN, GFP_KERNEL);
	}

	if (!nspm->alt_name || !nspm->uuid) {
		rc = -ENOMEM;
		goto err;
	}

	nd_namespace_pmem_set_size(nd_region, nspm, size);

	return 0;
 err:
	switch (rc) {
	case -EINVAL:
		dev_dbg(&nd_region->dev, "%s: invalid label(s)\n", __func__);
		break;
	case -ENODEV:
		dev_dbg(&nd_region->dev, "%s: label not found\n", __func__);
		break;
	default:
		dev_dbg(&nd_region->dev, "%s: unexpected err: %d\n", __func__, rc);
		break;
	}
	return rc;
}

static struct device **create_namespace_pmem(struct nd_region *nd_region)
{
	struct nd_namespace_pmem *nspm;
	struct device *dev, **devs;
	struct resource *res;
	int rc;

	nspm = kzalloc(sizeof(*nspm), GFP_KERNEL);
	if (!nspm)
		return NULL;

	dev = &nspm->nsio.dev;
	dev->type = &namespace_pmem_device_type;
	res = &nspm->nsio.res;
	res->name = dev_name(&nd_region->dev);
	res->flags = IORESOURCE_MEM;
	rc = find_pmem_label_set(nd_region, nspm);
	if (rc == -ENODEV) {
		int i;

		/* Pass, try to permit namespace creation... */
		for (i = 0; i < nd_region->ndr_mappings; i++) {
			struct nd_mapping *nd_mapping = &nd_region->mapping[i];

			kfree(nd_mapping->labels);
			nd_mapping->labels = NULL;
		}

		/* Publish a zero-sized namespace for userspace to configure. */
		nd_namespace_pmem_set_size(nd_region, nspm, 0);

		rc = 0;
	} else if (rc)
		goto err;

	devs = kcalloc(2, sizeof(struct device *), GFP_KERNEL);
	if (!devs)
		goto err;

	devs[0] = dev;
	return devs;

 err:
	namespace_pmem_release(&nspm->nsio.dev);
	return NULL;
}

struct resource *nsblk_add_resource(struct nd_region *nd_region,
		struct nd_dimm_drvdata *ndd, struct nd_namespace_blk *nsblk,
		resource_size_t start)
{
	struct nd_label_id label_id;
	struct resource *res;

	nd_label_gen_id(&label_id, nsblk->uuid, NSLABEL_FLAG_LOCAL);
	nsblk->res = krealloc(nsblk->res,
			sizeof(void *) * (nsblk->num_resources + 1),
			GFP_KERNEL);
	if (!nsblk->res)
		return NULL;
	for_each_dpa_resource(ndd, res)
		if (strcmp(res->name, label_id.id) == 0 && res->start == start) {
			nsblk->res[nsblk->num_resources++] = res;
			return res;
		}
	return NULL;
}

static struct device *nd_namespace_blk_create(struct nd_region *nd_region)
{
	struct nd_namespace_blk *nsblk;
	struct device *dev;

	if (!is_nd_blk(&nd_region->dev))
		return NULL;

	nsblk = kzalloc(sizeof(*nsblk), GFP_KERNEL);
	if (!nsblk)
		return NULL;

	dev = &nsblk->dev;
	dev->type = &namespace_blk_device_type;
	nsblk->id = ida_simple_get(&nd_region->ns_ida, 0, 0, GFP_KERNEL);
	if (nsblk->id < 0) {
		kfree(nsblk);
		return NULL;
	}
	dev_set_name(dev, "namespace%d.%d", nd_region->id, nsblk->id);
	dev->parent = &nd_region->dev;
	dev->groups = nd_namespace_attribute_groups;

	return &nsblk->dev;
}

void nd_region_create_blk_seed(struct nd_region *nd_region)
{
	WARN_ON(!is_nd_bus_locked(&nd_region->dev));
	nd_region->ns_seed = nd_namespace_blk_create(nd_region);
	/*
	 * Seed creation failures are not fatal, provisioning is simply
	 * disabled until memory becomes available
	 */
	if (!nd_region->ns_seed)
		dev_err(&nd_region->dev, "failed to create blk namespace\n");
	else
		nd_device_register(nd_region->ns_seed);
}

static struct device **create_namespace_blk(struct nd_region *nd_region)
{
	struct nd_mapping *nd_mapping = &nd_region->mapping[0];
	struct nd_namespace_label __iomem *nd_label;
	struct device *dev, **devs = NULL;
	u8 label_uuid[NSLABEL_UUID_LEN];
	struct nd_namespace_blk *nsblk;
	struct nd_dimm_drvdata *ndd;
	int i, l, count = 0;
	struct resource *res;

	if (nd_region->ndr_mappings == 0)
		return NULL;

	ndd = to_ndd(nd_mapping);
	for_each_label(l, nd_label, nd_mapping->labels) {
		u32 flags = readl(&nd_label->flags);
		char *name[NSLABEL_NAME_LEN];
		struct device **__devs;

		if (flags & NSLABEL_FLAG_LOCAL)
			/* pass */;
		else
			continue;

		memcpy_fromio(label_uuid, nd_label->uuid, NSLABEL_UUID_LEN);
		for (i = 0; i < count; i++) {
			nsblk = to_nd_namespace_blk(devs[i]);
			if (memcmp(nsblk->uuid, label_uuid,
						NSLABEL_UUID_LEN) == 0) {
				res = nsblk_add_resource(nd_region, ndd, nsblk,
						readq(&nd_label->dpa));
				if (!res)
					goto err;
				nd_dbg_dpa(nd_region, ndd, res, "%s assign\n",
					dev_name(&nsblk->dev));
				break;
			}
		}
		if (i < count)
			continue;
		__devs = kcalloc(count + 2, sizeof(dev), GFP_KERNEL);
		if (!__devs)
			goto err;
		memcpy(__devs, devs, sizeof(dev) * count);
		kfree(devs);
		devs = __devs;

		nsblk = kzalloc(sizeof(*nsblk), GFP_KERNEL);
		if (!nsblk)
			goto err;
		dev = &nsblk->dev;
		dev->type = &namespace_blk_device_type;
		dev_set_name(dev, "namespace%d.%d", nd_region->id, count);
		devs[count++] = dev;
		nsblk->id = -1;
		nsblk->lbasize = readq(&nd_label->lbasize);
		nsblk->uuid = kmemdup(label_uuid, NSLABEL_UUID_LEN, GFP_KERNEL);
		if (!nsblk->uuid)
			goto err;
		memcpy_fromio(name, nd_label->name, NSLABEL_NAME_LEN);
		if (name[0])
			nsblk->alt_name = kmemdup(name, NSLABEL_NAME_LEN,
					GFP_KERNEL);
		res = nsblk_add_resource(nd_region, ndd, nsblk,
				readq(&nd_label->dpa));
		if (!res)
			goto err;
		nd_dbg_dpa(nd_region, ndd, res, "%s assign\n",
				dev_name(&nsblk->dev));
	}

	dev_dbg(&nd_region->dev, "%s: discovered %d blk namespace%s\n",
			__func__, count, count == 1 ? "" : "s");

	if (count == 0) {
		/* Publish a zero-sized namespace for userspace to configure. */
		for (i = 0; i < nd_region->ndr_mappings; i++) {
			struct nd_mapping *nd_mapping = &nd_region->mapping[i];

			kfree(nd_mapping->labels);
			nd_mapping->labels = NULL;
		}

		devs = kcalloc(2, sizeof(dev), GFP_KERNEL);
		if (!devs)
			goto err;
		nsblk = kzalloc(sizeof(*nsblk), GFP_KERNEL);
		if (!nsblk)
			goto err;
		dev = &nsblk->dev;
		dev->type = &namespace_blk_device_type;
		devs[count++] = dev;
	}

	return devs;

err:
	for (i = 0; i < count; i++) {
		nsblk = to_nd_namespace_blk(devs[i]);
		namespace_blk_release(&nsblk->dev);
	}
	kfree(devs);
	return NULL;
}

static int init_active_labels(struct nd_region *nd_region)
{
	int i;

	for (i = 0; i < nd_region->ndr_mappings; i++) {
		struct nd_mapping *nd_mapping = &nd_region->mapping[i];
		struct nd_dimm_drvdata *ndd = to_ndd(nd_mapping);
		int count, j;

		/*
		 * If the dimm is disabled then prevent the region from
		 * being activated
		 */
		if (!ndd) {
			dev_dbg(&nd_region->dev, "%s: is disabled, failing probe\n",
					dev_name(&nd_mapping->nd_dimm->dev));
			return -ENXIO;
		}

		count = nd_label_active_count(ndd);
		dev_dbg(ndd->dev, "%s: %d\n", __func__, count);
		if (!count)
			continue;
		nd_mapping->labels = kcalloc(count + 1,
				sizeof(struct nd_namespace_label *), GFP_KERNEL);
		if (!nd_mapping->labels)
			return -ENOMEM;
		for (j = 0; j < count; j++) {
			struct nd_namespace_label __iomem *label;

			label = nd_label_active(ndd, j);
			nd_set_label(nd_mapping->labels, label, j);
		}
	}

	return 0;
}

int nd_region_register_namespaces(struct nd_region *nd_region, int *err)
{
	struct device **devs = NULL;
	int i, rc = 0, type;

	*err = 0;
	nd_bus_lock(&nd_region->dev);
	rc = init_active_labels(nd_region);
	if (rc) {
		nd_bus_unlock(&nd_region->dev);
		return rc;
	}

	type = nd_region_to_namespace_type(nd_region);
	switch (type) {
	case ND_DEVICE_NAMESPACE_IO:
		devs = create_namespace_io(nd_region);
		break;
	case ND_DEVICE_NAMESPACE_PMEM:
		devs = create_namespace_pmem(nd_region);
		break;
	case ND_DEVICE_NAMESPACE_BLOCK:
		devs = create_namespace_blk(nd_region);
		break;
	default:
		break;
	}
	nd_bus_unlock(&nd_region->dev);

	if (!devs) {
		rc = -ENODEV;
		goto err;
	}

	nd_region->ns_seed = devs[0];
	for (i = 0; devs[i]; i++) {
		struct device *dev = devs[i];
		int id;

		if (type == ND_DEVICE_NAMESPACE_BLOCK) {
			struct nd_namespace_blk *nsblk;

			nsblk = to_nd_namespace_blk(dev);
			id = ida_simple_get(&nd_region->ns_ida, 0, 0,
					GFP_KERNEL);
			nsblk->id = id;
		} else
			id = i;

		if (id < 0)
			break;
		dev_set_name(dev, "namespace%d.%d", nd_region->id, id);
		dev->parent = &nd_region->dev;
		dev->groups = nd_namespace_attribute_groups;
		nd_device_register(dev);
	}

	if (devs[i]) {
		int j;

		for (j = i; devs[j]; j++) {
			struct device *dev = devs[j];

			device_initialize(dev);
			put_device(dev);
		}
		*err = j - i;
		/*
		 * All of the namespaces we tried to register failed, so
		 * fail region activation.
		 */
		if (*err == 0)
			rc = -ENODEV;
	}
	kfree(devs);

 err:
	if (rc == -ENODEV) {
		nd_region->ns_seed = NULL;
		for (i = 0; i < nd_region->ndr_mappings; i++) {
			struct nd_mapping *nd_mapping = &nd_region->mapping[i];

			kfree(nd_mapping->labels);
			nd_mapping->labels = NULL;
		}
		return rc;
	}

	return i;
}
