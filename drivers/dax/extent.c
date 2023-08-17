// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2023 Intel Corporation. All rights reserved. */

#include <linux/device.h>
#include <linux/slab.h>
#include "dax-private.h"

static ssize_t length_show(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
	struct dax_reg_ext_dev *dr_reg_ext_dev = to_dr_ext_dev(dev);

	return sysfs_emit(buf, "%#llx\n", dr_reg_ext_dev->length);
}
static DEVICE_ATTR_RO(length);

static ssize_t label_show(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	struct dax_reg_ext_dev *dr_reg_ext_dev = to_dr_ext_dev(dev);

	return sysfs_emit(buf, "%s\n", dr_reg_ext_dev->label);
}

static ssize_t label_store(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t len)
{
	struct dax_reg_ext_dev *dr_reg_ext_dev = to_dr_ext_dev(dev);

	snprintf(dr_reg_ext_dev->label, DAX_EXTENT_LABEL_LEN, "%s", buf);
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
	struct dax_reg_ext_dev *dr_reg_ext_dev = to_dr_ext_dev(dev);

	kfree(dr_reg_ext_dev);
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

bool is_dr_ext_dev(struct device *dev)
{
	return dev->type == &dr_extent_type;
}
EXPORT_SYMBOL_GPL(is_dr_ext_dev);

static void unregister_dr_extent(void *ext)
{
	struct dax_reg_ext_dev *dr_reg_ext_dev = ext;
	struct dax_region_extent *dr_extent;

	dr_extent = dr_reg_ext_dev->dr_extent;
	dev_dbg(&dr_reg_ext_dev->dev, "Unregister DAX region ext OFF:%llx L:%s\n",
		dr_reg_ext_dev->offset, dr_reg_ext_dev->label);
	dr_extent_put(dr_extent);
	device_unregister(&dr_reg_ext_dev->dev);
}

int dax_region_ext_create_dev(struct dax_region *dax_region,
			      struct dax_region_extent *dr_extent,
			      resource_size_t offset,
			      resource_size_t length,
			      const char *label)
{
	struct dax_reg_ext_dev *dr_reg_ext_dev;
	struct device *dev;
	int rc;

	dr_reg_ext_dev = kzalloc(sizeof(*dr_reg_ext_dev), GFP_KERNEL);
	if (!dr_reg_ext_dev)
		return -ENOMEM;

	dr_reg_ext_dev->dr_extent = dr_extent;
	dr_reg_ext_dev->offset = offset;
	dr_reg_ext_dev->length = length;
	snprintf(dr_reg_ext_dev->label, DAX_EXTENT_LABEL_LEN, "%s", label);

	dev = &dr_reg_ext_dev->dev;
	device_initialize(dev);
	dev->id = offset / PMD_SIZE ;
	device_set_pm_not_required(dev);
	dev->parent = dax_region->dev;
	dev->type = &dr_extent_type;
	rc = dev_set_name(dev, "extent%d", dev->id);
	if (rc)
		goto err;

	rc = device_add(dev);
	if (rc)
		goto err;

	dev_dbg(dev, "DAX region extent OFF:%llx LEN:%llx\n",
		dr_reg_ext_dev->offset, dr_reg_ext_dev->length);
	return devm_add_action_or_reset(dax_region->dev, unregister_dr_extent,
					dr_reg_ext_dev);

err:
	dev_err(dev, "Failed to initialize DAX extent dev OFF:%llx LEN:%llx\n",
		dr_reg_ext_dev->offset, dr_reg_ext_dev->length);
	put_device(dev);
	return rc;
}
EXPORT_SYMBOL_GPL(dax_region_ext_create_dev);

void dax_region_ext_del_dev(struct dax_region *dax_region,
			    struct dax_reg_ext_dev *dr_reg_ext_dev)
{
	devm_remove_action(dax_region->dev, unregister_dr_extent, dr_reg_ext_dev);
	unregister_dr_extent(dr_reg_ext_dev);
}
EXPORT_SYMBOL_GPL(dax_region_ext_del_dev);
