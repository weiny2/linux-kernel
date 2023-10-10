// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2023 Intel Corporation. All rights reserved. */

#include <linux/device.h>
#include <linux/slab.h>
#include "dax-private.h"

static ssize_t length_show(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
	struct dr_extent *dr_extent = to_dr_extent(dev);

	return sysfs_emit(buf, "%#llx\n", dr_extent->length);
}
static DEVICE_ATTR_RO(length);

static ssize_t label_show(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	struct dr_extent *dr_extent = to_dr_extent(dev);

	return sysfs_emit(buf, "%s\n", dr_extent->label);
}

static ssize_t label_store(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t len)
{
	struct dr_extent *dr_extent = to_dr_extent(dev);

	snprintf(dr_extent->label, DAX_EXTENT_LABEL_LEN, "%s", buf);
	return len;
}
static DEVICE_ATTR_RW(label);

static struct attribute *dr_extent_attrs[] = {
	&dev_attr_length.attr,
	&dev_attr_label.attr,
	NULL,
};

static const struct attribute_group dr_extent_attribute_group = {
	.attrs = dr_extent_attrs,
};

static void dr_extent_release(struct device *dev)
{
	struct dr_extent *dr_extent = to_dr_extent(dev);

	kfree(dr_extent);
}

static const struct attribute_group *dr_extent_attribute_groups[] = {
	&dr_extent_attribute_group,
	NULL,
};

const struct device_type dr_extent_type = {
	.name = "extent",
	.release = dr_extent_release,
	.groups = dr_extent_attribute_groups,
};

void dr_extent_get(struct dr_extent *dr_extent)
{
	kref_get(&dr_extent->ref);
}

static void dr_release(struct kref *kref)
{
	struct dr_extent *dr_extent;

	dr_extent = container_of(kref, struct dr_extent, ref);
	if (dr_extent->release)
		dr_extent->release(dr_extent->private_data);
	dev_dbg(&dr_extent->dev, "Unregister DAX region ext OFF:%#llx L:%s\n",
		dr_extent->offset, dr_extent->label);
	device_unregister(&dr_extent->dev);
}

void dr_extent_put(struct dr_extent *dr_extent)
{
	kref_put(&dr_extent->ref, dr_release);
}

void dr_ext_reg_put(void *data)
{
	struct dr_extent *dr_extent = data;

	dr_extent_put(dr_extent);
}

int dax_region_ext_create_dev(struct dax_region *dax_region,
			      void *private_data,
			      void (*release)(void *private_data),
			      resource_size_t offset,
			      resource_size_t length,
			      const char *label)
{
	struct dr_extent *dr_extent;
	struct device *dev;
	int rc;

	dr_extent = kzalloc(sizeof(*dr_extent), GFP_KERNEL);
	if (!dr_extent)
		return -ENOMEM;

	dr_extent->offset = offset;
	dr_extent->length = length;
	snprintf(dr_extent->label, DAX_EXTENT_LABEL_LEN, "%s", label);

	dr_extent->private_data = private_data;
	dr_extent->release = release;
	kref_init(&dr_extent->ref);

	dev = &dr_extent->dev;
	device_initialize(dev);
	dev->id = offset / PMD_SIZE;
	device_set_pm_not_required(dev);
	dev->parent = dax_region->dev;
	dev->type = &dr_extent_type;
	rc = dev_set_name(dev, "extent%d", dev->id);
	if (rc)
		goto err;

	rc = device_add(dev);
	if (rc)
		goto err;

	dev_dbg(dev, "DAX region extent OFF:%#llx LEN:%#llx\n", offset, length);
	return devm_add_action_or_reset(dax_region->dev, dr_ext_reg_put,
					dr_extent);

err:
	dev_err(dev, "Failed to initialize DAX extent dev OFF:%#llx LEN:%#llx\n",
		offset, length);
	put_device(dev);
	return rc;
}
EXPORT_SYMBOL_GPL(dax_region_ext_create_dev);
