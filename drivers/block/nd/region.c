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
#include "nd.h"

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

static struct device_type namespace_io_device_type = {
	.name = "nd_namespace_io",
	.release = namespace_io_release,
};

static struct device_type namespace_pmem_device_type = {
	.name = "nd_namespace_pmem",
	.release = namespace_pmem_release,
};

static bool is_namespace_pmem(struct device *dev)
{
	return dev ? dev->type == &namespace_pmem_device_type : false;
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
	struct nd_namespace_pmem *nspm;
	char *input, *pos, *alt_name;
	ssize_t rc;

	if (!is_namespace_pmem(dev))
		return -ENXIO;

	nspm = to_nd_namespace_pmem(dev);

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
	kfree(nspm->alt_name);
	nspm->alt_name = alt_name;
	sprintf(nspm->alt_name, "%s", pos);
	rc = len;

out:
	kfree(input);
	return rc;
}

static int check_label_space(struct nd_region *nd_region)
{
	int i;

	for (i = 0; i < nd_region->ndr_mappings; i++) {
		struct nd_mapping *nd_mapping = &nd_region->mapping[i];
		struct nd_dimm *nd_dimm = nd_mapping->nd_dimm;

		/* if there are no valid labels then all slots are free */
		if (nd_mapping->labels == NULL)
			return 0;

		if (nd_label_nfree(nd_dimm) < 1)
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

	if (is_namespace_pmem(dev)) {
		struct nd_namespace_pmem *nspm = to_nd_namespace_pmem(dev);
		struct resource *res = &nspm->nsio.res;

		/* only attempt update on idle+configured namespaces */
		if (resource_size(res) < ND_MIN_NAMESPACE_SIZE || !nspm->uuid)
			return 0;

		return nd_pmem_namespace_label_update(nd_region, nspm);
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
	rc = check_label_space(nd_region);
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
	struct nd_namespace_pmem *nspm;

	if (!is_namespace_pmem(dev))
		return -ENXIO;

	nspm = to_nd_namespace_pmem(dev);

	return sprintf(buf, "%s\n", nspm->alt_name ? nspm->alt_name : "");
}
DEVICE_ATTR_RW(alt_name);

static ssize_t __size_store(struct device *dev, const char *buf)
{
	struct nd_region *nd_region = to_nd_region(dev->parent);
	struct nd_namespace_pmem *nspm;
	unsigned long long val;
	struct resource *res;
	int rc;

	if (!is_namespace_pmem(dev))
		return -ENXIO;

	if (dev->driver)
		return -EBUSY;

	rc = kstrtoull(buf, 0, &val);
	if (rc)
		return rc;

	nspm = to_nd_namespace_pmem(dev);
	res = &nspm->nsio.res;
	if (val < ND_MIN_NAMESPACE_SIZE) {
		return -EINVAL;
	} else if (resource_size(res) >= val) {
		nd_region->available_size += resource_size(res) - val;
	} else {
		if (val > (nd_region->available_size + resource_size(res)))
			return -ENOSPC;
		nd_region->available_size -= val - resource_size(res);
	}

	res->end = res->start + val - 1;

	return 0;
}

static ssize_t size_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	struct nd_region *nd_region = to_nd_region(dev->parent);
	int rc;

	device_lock(dev);
	nd_bus_lock(dev);
	wait_nd_bus_probe_idle(dev);
	rc = check_label_space(nd_region);
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
	struct nd_namespace_io *nsio;

	if (is_namespace_pmem(dev)) {
		struct nd_namespace_pmem *nspm = to_nd_namespace_pmem(dev);

		nsio = &nspm->nsio;
	} else if (is_namespace_io(dev)) {
		nsio = to_nd_namespace_io(dev);
	} else
		return -ENXIO;

	return sprintf(buf, "%lld\n",
			(unsigned long long) resource_size(&nsio->res));
}
static DEVICE_ATTR(size, S_IRUGO, size_show, size_store);

static ssize_t uuid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nd_namespace_pmem *nspm;

	if (!is_namespace_pmem(dev))
		return -ENXIO;
	nspm = to_nd_namespace_pmem(dev);

	return nd_uuid_show(nspm->uuid, buf);
}

static ssize_t uuid_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	struct nd_region *nd_region = to_nd_region(dev->parent);
	struct nd_namespace_pmem *nspm;
	ssize_t rc;

	if (!is_namespace_pmem(dev))
		return -ENXIO;
	nspm = to_nd_namespace_pmem(dev);

	device_lock(dev);
	nd_bus_lock(dev);
	wait_nd_bus_probe_idle(dev);
	rc = check_label_space(nd_region);
	if (rc >= 0)
		rc = nd_uuid_store(dev, &nspm->uuid, buf, len);
	if (rc >= 0)
		rc = nd_namespace_label_update(nd_region, dev);
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

	if (is_namespace_pmem(dev)) {
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

#define for_each_label(label, mapping) \
	for ((label) = (mapping)->labels; (label) && *(label); (label)++)

static bool has_uuid_at_pos(struct nd_region *nd_region, u8 *uuid, u64 cookie, u16 pos)
{
	struct nd_namespace_label __iomem *found = NULL;
	int i;

	for (i = 0; i < nd_region->ndr_mappings; i++) {
		struct nd_mapping *nd_mapping = &nd_region->mapping[i];
		struct nd_namespace_label * __iomem *nd_label;
		u8 *found_uuid = NULL;

		for_each_label(nd_label, nd_mapping) {
			u64 isetcookie = readq(&(*nd_label)->isetcookie);
			u16 position = readw(&(*nd_label)->position);
			u16 nlabel = readw(&(*nd_label)->nlabel);
			u8 *label_uuid = (*nd_label)->uuid;

			if (isetcookie != cookie)
				continue;

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
			found = *nd_label;
			break;
		}
		if (found)
			break;
	}
	return found != NULL;
}

/**
 * check_blk_overlap - validate the pmem and blk are mutually exclusive
 * @nd_mapping: dimm + labels to check
 * @pmem_start: check blk labels against a pmem range, optional
 * @pmem_end: check blk labels against a pmem range, optional
 * @blk_start_min: output, lowest blk start in interleave set
 */
static int check_blk_overlap(struct nd_mapping *nd_mapping,
		u64 pmem_start, u64 pmem_end, u64 *blk_start_min_out)
{
	u64 blk_start_min, blk_start, blk_end, hw_start, hw_end;
	struct nd_namespace_label * __iomem *nd_label;

	hw_start = nd_mapping->start;
	hw_end = hw_start + nd_mapping->size;
	blk_start_min = hw_end;
	for_each_label(nd_label, nd_mapping) {
		unsigned long flags = readl(&(*nd_label)->flags);

		if ((flags & NSLABEL_FLAG_LOCAL) == 0)
			continue;
		blk_start = readq(&(*nd_label)->dpa);
		blk_end = blk_start + readq(&(*nd_label)->rawsize);
		if (blk_start >= hw_start && blk_start < hw_end) {
			if (blk_start_min == hw_end)
				blk_start_min = blk_start;
			else
				blk_start_min = min(blk_start_min, blk_start);
		}
		if (blk_end >= hw_start && blk_end < hw_end)
			blk_start_min = hw_start;

		/* skip optional pmem overlap check */
		if (pmem_start == pmem_end)
			continue;

		if ((blk_start < pmem_end && blk_start >= pmem_start)
				|| (blk_end < pmem_end
					&& blk_end > pmem_start))
			return -EINVAL;
	}

	*blk_start_min_out = blk_start_min;
	return 0;
}

static int select_pmem_uuid(struct nd_region *nd_region, u8 *pmem_uuid)
{
	unsigned long long available_size = nd_region->ndr_size, s;
	struct nd_namespace_label __iomem *select = NULL;
	int i;

	if (!pmem_uuid)
		return -ENODEV;

	for (i = 0; i < nd_region->ndr_mappings; i++) {
		u64 hw_start, hw_end, pmem_start, pmem_end, blk_start_min;
		struct nd_mapping *nd_mapping = &nd_region->mapping[i];
		struct nd_namespace_label * __iomem *nd_label;

		for_each_label(nd_label, nd_mapping) {
			u8 *uuid = (*nd_label)->uuid;

			if (memcmp(uuid, pmem_uuid, NSLABEL_UUID_LEN) == 0)
				break;
		}

		if (!nd_label || !(*nd_label)) {
			WARN_ON(1);
			return -EINVAL;
		}

		select = *nd_label;
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

		if (check_blk_overlap(nd_mapping, pmem_start, pmem_end,
					&blk_start_min))
			return -EINVAL;

		s = available_size;
		available_size -= (pmem_end - pmem_start)
			+ (hw_end - blk_start_min);
		if (available_size > s) {
			dev_err(&nd_region->dev,
					"%s: available_size underflow\n",
					__func__);
			available_size = 0;
		}
		nd_mapping->labels[0] = select;
		nd_mapping->labels[1] = NULL;
	}

	nd_region->available_size = available_size;
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
	struct nd_namespace_label * __iomem *nd_label;
	struct resource *res = &nspm->nsio.res;
	resource_size_t size = 0;
	u8 *pmem_uuid = NULL;
	int rc = -ENODEV;
	u16 i;

	if (cookie == 0)
		return -ENXIO;

	/*
	 * Find a complete set of labels by uuid.  By definition we can start
	 * with any mapping as the reference label
	 */
	for_each_label(nd_label, &nd_region->mapping[0]) {
		u64 isetcookie = readq(&(*nd_label)->isetcookie);
		u8 *uuid = (*nd_label)->uuid;

		if (isetcookie != cookie)
			continue;

		for (i = 0; nd_region->ndr_mappings; i++)
			if (!has_uuid_at_pos(nd_region, uuid, cookie, i))
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
		pmem_uuid = uuid;
	}

	/*
	 * Fix up each mapping's 'labels' to have the validated pmem label for
	 * that position at labels[0], and NULL at labels[1].  In the process,
	 * check that the namespace aligns with interleave-set and
	 * peer-BLK-namespace boundaries
	 */
	rc = select_pmem_uuid(nd_region, pmem_uuid);
	if (rc)
		goto err;

	/* Calculate total size and populate namespace properties from label0 */
	for (i = 0; i < nd_region->ndr_mappings; i++) {
		struct nd_mapping *nd_mapping = &nd_region->mapping[i];
		struct nd_namespace_label __iomem *label0;

		label0 = nd_mapping->labels[0];
		size += readq(&label0->rawsize);
		if (readl(&label0->position) != 0)
			continue;
		WARN_ON(nspm->alt_name || nspm->uuid);
		nspm->alt_name = kmemdup(label0->name, NSLABEL_NAME_LEN,
				GFP_KERNEL);
		nspm->uuid = kmemdup(label0->uuid, NSLABEL_UUID_LEN,
				GFP_KERNEL);
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

/**
 * nd_namespace_pmem_request_dpa - mark dimm physical address ranges as in use
 * @nspm: initialized namespace the pmem driver is probing
 *
 * LOCKING: Expects nd_bus_lock held at entry
 */
int nd_namespace_pmem_request_dpa(struct nd_namespace_pmem *nspm)
{
	int i;
	struct device *dev = &nspm->nsio.dev;
	struct nd_region *nd_region = to_nd_region(dev->parent);

	WARN_ON_ONCE(!is_nd_bus_locked(dev));

	for (i = 0; i < nd_region->ndr_mappings; i++) {
		struct nd_mapping *nd_mapping = &nd_region->mapping[i];
		struct nd_dimm *nd_dimm = nd_mapping->nd_dimm;
		struct nd_namespace_label __iomem *nd_label;
		struct resource *res;

		nd_label = nd_mapping->labels[0];
		if (!nd_label)
			return -ENXIO;

		res = __devm_request_region(dev, &nd_dimm->dpa,
				readq(&nd_label->dpa),
				readq(&nd_label->rawsize), dev_name(dev));
		if (!res) {
			dev_err(dev, "%s: dpa resource conflict\n", __func__);
			return -EBUSY;
		}
	}

	return 0;
}
EXPORT_SYMBOL(nd_namespace_pmem_request_dpa);

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

	nd_bus_lock(&nd_region->dev);
	rc = find_pmem_label_set(nd_region, nspm);
	nd_bus_unlock(&nd_region->dev);

	res = &nspm->nsio.res;
	res->name = dev_name(&nd_region->dev);
	res->flags = IORESOURCE_MEM;
	if (rc == -ENODEV) {
		u64 overlap = 0;
		int i;

		/* Pass, try to permit namespace creation... */

		/* First, release labels and reduce the available size. */
		for (i = 0; i < nd_region->ndr_mappings; i++) {
			struct nd_mapping *nd_mapping = &nd_region->mapping[i];
			u64 blk_start_min, hw_end;

			if (check_blk_overlap(nd_mapping, 0, 0, &blk_start_min))
				break;

			hw_end = nd_mapping->start + nd_mapping->size;
			overlap += hw_end - blk_start_min;
		}
		if (i < nd_region->ndr_mappings || overlap > nd_region->ndr_size)
			goto err;
		nd_region->available_size = nd_region->ndr_size - overlap;

		/*
		 * If we don't have available_size, then there is no point in
		 * publishing a namespace.
		 */
		if (nd_region->available_size == 0)
			goto err;

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

static int nd_region_probe(struct device *dev)
{
	struct nd_region *nd_region = to_nd_region(dev);
	struct device **devs = NULL;
	int i, rc;

	rc = init_active_labels(nd_region);
	if (rc)
		return rc;

	switch (nd_region_to_namespace_type(nd_region)) {
	case ND_DEVICE_NAMESPACE_IO:
		devs = create_namespace_io(nd_region);
		break;
	case ND_DEVICE_NAMESPACE_PMEM:
		devs = create_namespace_pmem(nd_region);
		break;
	default:
		break;
	}

	for (i = 0; devs && devs[i]; i++) {
		struct device *dev = devs[i];

		dev_set_name(dev, "namespace%d.%d", nd_region->id, i);
		dev->parent = &nd_region->dev;
		dev->groups = nd_namespace_attribute_groups;
		nd_device_register(dev);
	}
	kfree(devs);

	return devs ? 0 : -ENODEV;
}

static int child_unregister(struct device *dev, void *data)
{
	nd_device_unregister(dev, ND_SYNC);
	return 0;
}

static int nd_region_remove(struct device *dev)
{
	device_for_each_child(dev, NULL, child_unregister);
	return 0;
}

static struct nd_device_driver nd_region_driver = {
	.probe = nd_region_probe,
	.remove = nd_region_remove,
	.drv = {
		.name = "nd_region",
	},
	.type = ND_DRIVER_REGION_BLOCK | ND_DRIVER_REGION_PMEM,
};

int __init nd_region_init(void)
{
	return nd_driver_register(&nd_region_driver);
}

void __exit nd_region_exit(void)
{
	driver_unregister(&nd_region_driver.drv);
}

MODULE_ALIAS_ND_DEVICE(ND_DEVICE_REGION_PMEM);
MODULE_ALIAS_ND_DEVICE(ND_DEVICE_REGION_BLOCK);
