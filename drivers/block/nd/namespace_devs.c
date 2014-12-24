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
	/* TODO: blk namespace support */
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
		/* TODO: blk namespace support */
		return -ENXIO;
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

static int check_label_space(struct nd_region *nd_region, struct device *dev)
{
	int i, nlabel;

	if (is_namespace_pmem(dev)) {
		nlabel = 1;
	} else if (is_namespace_blk(dev)) {
		/* TODO: blk namespace support */
		return -ENXIO;
	} else
		return -ENXIO;

	for (i = 0; i < nd_region->ndr_mappings; i++) {
		struct nd_mapping *nd_mapping = &nd_region->mapping[i];
		struct nd_dimm *nd_dimm = nd_mapping->nd_dimm;

		/* if there are no valid labels then all slots are free */
		if (nd_mapping->labels == NULL)
			return 0;

		if (nd_label_nfree(nd_dimm) <= nlabel)
			return -ENOSPC;
	}

	return 0;
}

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
		struct resource *res = &nspm->nsio.res;
		resource_size_t size = resource_size(res);

		if (size == 0)
			/* pass */;
		else if (size < ND_MIN_NAMESPACE_SIZE || !nspm->uuid)
			return 0;

		return nd_pmem_namespace_label_update(nd_region, nspm, size);
	} else if (is_namespace_blk(dev)) {
		/* TODO: blk namespace support */
		return -ENXIO;
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
	rc = check_label_space(nd_region, dev);
	if (rc >= 0)
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
		/* TODO: blk namespace support */
		return -ENXIO;
	} else
		return -ENXIO;

	return sprintf(buf, "%s\n", ns_altname ? ns_altname : "");
}
static DEVICE_ATTR_RW(alt_name);

static int scan_free(struct nd_region *nd_region,
		struct nd_mapping *nd_mapping, char *label_id,
		resource_size_t n)
{
	struct nd_dimm *nd_dimm = nd_mapping->nd_dimm;
	int rc = 0;

	while (n) {
		struct resource *res, *last;

		last = NULL;
		for_each_dpa_resource(nd_dimm, res)
			if (strcmp(res->name, label_id) == 0)
				last = res;
		res = last;
		if (!res)
			return 0;

		if (n >= resource_size(res)) {
			n -= resource_size(res);
			dev_dbg(&nd_region->dev, "%s: %s: delete %#llx@%#llx: %d\n",
					dev_name(&nd_dimm->dev), res->name,
					(unsigned long long) resource_size(res),
					(unsigned long long) res->start, rc);
			__devm_release_region(&nd_dimm->dev, &nd_dimm->dpa,
					res->start, resource_size(res));
			/* retry with last resource deleted */
			continue;
		}

		rc = adjust_resource(res, res->start, resource_size(res) - n);
		dev_dbg(&nd_region->dev, "%s: %s: shrink %#llx@%#llx: %d\n",
				dev_name(&nd_dimm->dev), res->name,
				(unsigned long long) resource_size(res),
				(unsigned long long) res->start, rc);
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
static int shrink_dpa_allocation(struct nd_region *nd_region, char *label_id,
		resource_size_t n)
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

static int scan_allocate(struct nd_region *nd_region,
		struct nd_mapping *nd_mapping, char *label_id,
		resource_size_t n)
{
	struct nd_dimm *nd_dimm = nd_mapping->nd_dimm;
	struct resource *res;

	for_each_dpa_resource(nd_dimm, res) {
		resource_size_t allocate, available = 0, free_start;
		struct resource *next = res->sibling;
		const char *action;
		int rc;

		/* space at the beginning of the mapping */
		if (res == nd_dimm->dpa.child) {
			free_start = nd_mapping->start;
			available = res->start - free_start;
		}

		/* space between allocations */
		if (!available && next) {
			free_start = res->start + resource_size(res);
			available = next->start - free_start;
		}

		/* space at the end of the mapping */
		if (!available && !next) {
			free_start = res->start + resource_size(res);
			available = nd_mapping->start + nd_mapping->size
				- free_start;
		}

		if (available == 0)
			continue;
		allocate = min(available, n);
		n -= allocate;
		if (strcmp(res->name, label_id) == 0) {
			rc = adjust_resource(res, res->start, resource_size(res)
					+ allocate);
			action = "grow";
		} else if (strncmp("pmem", label_id, 4) == 0
				&& free_start != nd_mapping->start) {
			rc = -ENXIO;
			action = "allocate pmem";
		} else {
			res = nd_dimm_allocate_dpa(nd_dimm, label_id,
					free_start, allocate);
			rc = res ? 0 : -EBUSY;
			action = "allocate";
		}
		dev_dbg(&nd_region->dev, "%s: %s: %s: %#llx@%#llx: %d\n",
				dev_name(&nd_dimm->dev), label_id, action,
				(unsigned long long) allocate,
				(unsigned long long) free_start, rc);
		if (rc)
			return rc;

		if (n == 0)
			break;
	}

	return 0;
}

/**
 * grow_dpa_allocation - for each dimm allocate n bytes for @label_id
 * @nd_region: the set of dimms to allocate @n more bytes from
 * @label_id: unique identifier for the namespace consuming this dpa range
 * @n: number of bytes per-dimm to add to the existing allocation
 *
 * Assumes resources are ordered.  Starting from the beginning try to
 * adjust the resource to the next allocation boundary.  If the region
 * is pmem we're done, otherwise skip to the next free space and start a
 * new resource claim.
 */
static int grow_dpa_allocation(struct nd_region *nd_region, char *label_id,
		resource_size_t n)
{
	bool is_pmem = strncmp(label_id, "pmem", 4) == 0;
	int i;

	for (i = 0; i < nd_region->ndr_mappings; i++) {
		struct nd_mapping *nd_mapping = &nd_region->mapping[i];
		struct nd_dimm *nd_dimm = nd_mapping->nd_dimm;
		struct resource *res;
		bool do_init = false;
		int rc;

		if (is_pmem) {
			struct resource *found = NULL;

			for_each_dpa_resource(nd_dimm, res)
				if (strcmp(res->name, label_id) == 0) {
					found = res;
					break;
				}
			if (!found)
				do_init = true;
		} else if (nd_dimm->dpa.child == NULL) {
			do_init = true;
		}

		if (do_init) {
			/* first resource allocation for this dimm */
			res = nd_dimm_allocate_dpa(nd_dimm, label_id,
						nd_mapping->start, n);
			rc = res ? 0 : -EBUSY;
			dev_dbg(&nd_region->dev, "%s: %s: init: %#llx@%#llx: %d\n",
					dev_name(&nd_dimm->dev), label_id,
					(unsigned long long) n,
					(unsigned long long) nd_mapping->start,
					rc);
			if (rc)
				return rc;
			continue;
		}

		rc = scan_allocate(nd_region, nd_mapping, label_id, n);
		if (rc)
			return rc;
	}

	return 0;
}

static ssize_t __size_store(struct device *dev, const char *buf)
{
	resource_size_t allocated = 0, available = 0, min_available = 0;
	struct nd_region *nd_region = to_nd_region(dev->parent);
	struct nd_dimm *nd_dimm, *min_dimm = NULL;
	char label_id[ND_LABEL_ID_SIZE];
	struct nd_mapping *nd_mapping;
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
		/* TODO: blk namespace support */
		return -ENXIO;
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

	nd_label_gen_id(label_id, uuid, flags);
	for (i = 0; i < nd_region->ndr_mappings; i++) {
		resource_size_t dimm_available, min;

		nd_mapping = &nd_region->mapping[i];
		nd_dimm = nd_mapping->nd_dimm;

		/*
		 * All dimms in an interleave set, or the base dimm for a blk
		 * region, need to be enabled for the size to be changed.
		 */
		if (!nd_dimm->dev.driver)
			return -ENXIO;

		allocated += nd_dimm_allocated_dpa(nd_dimm, label_id);
		dimm_available = nd_dimm_available_dpa(nd_dimm, nd_region);
		available += dimm_available;
		min = min_not_zero(dimm_available, min_available);
		if (!min_dimm || min != min_available)
			min_dimm = nd_dimm;
		min_available = min;
	}

	if (val > available + allocated)
		return -ENOSPC;

	if (val == allocated)
		return 0;

	if (val % nd_region->ndr_mappings)
		dev_info(dev, "%lld truncated to %lld setting namespace size\n",
				val, (val / nd_region->ndr_mappings)
				* nd_region->ndr_mappings);
	val /= nd_region->ndr_mappings;
	allocated /= nd_region->ndr_mappings;

	if (val > allocated && val > min_available) {
		if (min_dimm)
			dev_dbg(dev, "%s only has %lld bytes available\n",
					dev_name(&min_dimm->dev),
					min_available);
		return -ENOSPC;
	}

	if (val < allocated)
		rc = shrink_dpa_allocation(nd_region, label_id, allocated - val);
	else
		rc = grow_dpa_allocation(nd_region, label_id, val - allocated);

	if (rc)
		return rc;

	if (is_namespace_pmem(dev)) {
		struct nd_namespace_pmem *nspm = to_nd_namespace_pmem(dev);
		struct resource *res = &nspm->nsio.res;

		res->start = nd_region->ndr_start;
		res->end = res->start + val * nd_region->ndr_mappings - 1;
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
	rc = check_label_space(nd_region, dev);
	if (rc >= 0)
		rc = __size_store(dev, buf);
	if (rc >= 0)
		rc = nd_namespace_label_update(nd_region, dev);
	dev_dbg(dev, "%s: %s (%d)\n", __func__, rc < 0 ? "fail" : "success", rc);
	nd_bus_unlock(dev);
	device_unlock(dev);

	return rc < 0 ? rc : len;
}

static ssize_t size_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if (is_namespace_pmem(dev)) {
		struct nd_namespace_pmem *nspm = to_nd_namespace_pmem(dev);

		return sprintf(buf, "%llu\n", (unsigned long long)
				resource_size(&nspm->nsio.res));
	} else if (is_namespace_blk(dev)) {
		/* TODO: blk namespace support */
		return -ENXIO;
	} else if (is_namespace_io(dev)) {
		struct nd_namespace_io *nsio = to_nd_namespace_io(dev);

		return sprintf(buf, "%llu\n", (unsigned long long)
				resource_size(&nsio->res));
	} else
		return -ENXIO;
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
		/* TODO: blk namespace support */
		return -ENXIO;
	} else
		return -ENXIO;

	return nd_uuid_show(uuid, buf);
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
	char old_label_id[ND_LABEL_ID_SIZE];
	char new_label_id[ND_LABEL_ID_SIZE];
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

		if (nd_mapping->labels)
			return -EBUSY;
	}

	nd_label_gen_id(old_label_id, *old_uuid, flags);
	nd_label_gen_id(new_label_id, new_uuid, flags);
	for (i = 0; i < nd_region->ndr_mappings; i++) {
		struct nd_mapping *nd_mapping = &nd_region->mapping[i];
		struct nd_dimm *nd_dimm = nd_mapping->nd_dimm;
		struct resource *res;

		for_each_dpa_resource(nd_dimm, res)
			if (strcmp(res->name, old_label_id) == 0)
				sprintf((void *) res->name, "%s", new_label_id);
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
		/* TODO: blk namespace support */
		return -ENXIO;
	} else
		return -ENXIO;

	device_lock(dev);
	nd_bus_lock(dev);
	wait_nd_bus_probe_idle(dev);
	rc = check_label_space(nd_region, dev);
	if (rc >= 0)
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

static struct attribute *nd_namespace_attributes[] = {
	&dev_attr_type.attr,
	&dev_attr_size.attr,
	&dev_attr_uuid.attr,
	&dev_attr_alt_name.attr,
	NULL,
};

static umode_t nd_namespace_attr_visible(struct kobject *kobj, struct attribute *a, int n)
{
	struct device *dev = container_of(kobj, struct device, kobj);

	if (is_namespace_pmem(dev) || is_namespace_blk(dev)) {
		if (a == &dev_attr_size.attr)
			return S_IWUSR;

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
				dev_dbg(&nd_mapping->nd_dimm->dev,
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
		 * range published in NFIT and does not overlap a BLK
		 * namespace
		 */
		hw_start = nd_mapping->start;
		hw_end = hw_start + nd_mapping->size;
		pmem_start = readq(&select->dpa);
		pmem_end = pmem_start + readq(&select->rawsize);
		if (pmem_end <= hw_end && pmem_start == hw_start)
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
	struct resource *res = &nspm->nsio.res;
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

	res->start = nd_region->ndr_start;
	res->end = res->start + size - 1;

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

			if (!nd_mapping->labels)
				continue;
			devm_kfree(&nd_region->dev, nd_mapping->labels);
			nd_mapping->labels = NULL;
		}

		/* Publish a zero-sized namespace for userspace to configure. */
		res->start = nd_region->ndr_start;
		res->end = nd_region->ndr_start - 1;

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

static int init_active_labels(struct nd_region *nd_region)
{
	int i;

	for (i = 0; i < nd_region->ndr_mappings; i++) {
		struct nd_mapping *nd_mapping = &nd_region->mapping[i];
		struct nd_dimm *nd_dimm = nd_mapping->nd_dimm;
		int count, j;

		/*
		 * If the dimm is disabled then prevent the region from
		 * being activated
		 */
		if (nd_dimm->dev.driver == NULL) {
			dev_dbg(&nd_region->dev, "%s: is disabled, failing probe\n",
					dev_name(&nd_dimm->dev));
			return -ENXIO;
		}

		count = nd_label_active_count(nd_dimm);
		dev_dbg(&nd_dimm->dev, "%s: %d\n", __func__, count);
		if (!count)
			continue;
		nd_mapping->labels = devm_kcalloc(&nd_region->dev, count + 1,
				sizeof(struct nd_namespace_label *), GFP_KERNEL);
		if (!nd_mapping->labels)
			return -ENOMEM;
		for (j = 0; j < count; j++)
			nd_mapping->labels[j] = nd_label_active(nd_dimm, j);
	}

	return 0;
}

int nd_region_register_namespaces(struct nd_region *nd_region)
{
	struct device **devs = NULL;
	int i, rc, type;

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
		/* TODO: blk namespace support */
	default:
		break;
	}
	nd_bus_unlock(&nd_region->dev);

	if (!devs)
		return -ENODEV;

	for (i = 0; devs[i]; i++) {
		struct device *dev = devs[i];

		dev_set_name(dev, "namespace%d.%d", nd_region->id, i);
		dev->parent = &nd_region->dev;
		dev->groups = nd_namespace_attribute_groups;
		nd_device_register(dev);
	}
	kfree(devs);

	return rc;
}
