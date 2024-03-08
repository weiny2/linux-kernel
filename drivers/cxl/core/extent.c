// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2024 Intel Corporation. All rights reserved. */

#include <linux/device.h>
#include <linux/slab.h>
#include <cxl.h>

static DEFINE_IDA(cxl_extent_ida);

static ssize_t offset_show(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
	struct dr_extent *dr_extent = to_dr_extent(dev);

	return sysfs_emit(buf, "%pa\n", &dr_extent->hpa_range.start);
}
static DEVICE_ATTR_RO(offset);

static ssize_t length_show(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
	struct dr_extent *dr_extent = to_dr_extent(dev);
	u64 length = range_len(&dr_extent->hpa_range);

	return sysfs_emit(buf, "%pa\n", &length);
}
static DEVICE_ATTR_RO(length);

static ssize_t label_show(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	struct dr_extent *dr_extent = to_dr_extent(dev);

	return sysfs_emit(buf, "%s\n", dr_extent->label);
}
static DEVICE_ATTR_RO(label);

static struct attribute *dr_extent_attrs[] = {
	&dev_attr_offset.attr,
	&dev_attr_length.attr,
	&dev_attr_label.attr,
	NULL,
};

static const struct attribute_group dr_extent_attribute_group = {
	.attrs = dr_extent_attrs,
};

static const struct attribute_group *dr_extent_attribute_groups[] = {
	&dr_extent_attribute_group,
	NULL,
};

static void dr_extent_release(struct device *dev)
{
	struct dr_extent *dr_extent = to_dr_extent(dev);

	cxl_release_ed_extent(&dr_extent->ed_ext);
	ida_free(&cxl_extent_ida, dr_extent->dev.id);
	kfree(dr_extent);
}

const struct device_type dr_extent_type = {
	.name = "extent",
	.release = dr_extent_release,
	.groups = dr_extent_attribute_groups,
};

bool is_dr_extent(struct device *dev)
{
	return dev->type == &dr_extent_type;
}
EXPORT_SYMBOL_NS_GPL(is_dr_extent, CXL);

static void dr_extent_unregister(void *ext)
{
	struct dr_extent *dr_extent = ext;

	dev_dbg(&dr_extent->dev, "DAX region rm extent HPA %#llx - %#llx\n",
		dr_extent->hpa_range.start, dr_extent->hpa_range.end);
	device_unregister(&dr_extent->dev);
}

int cxl_get_dr_ext_not_zero(struct dr_extent *dr_ext)
{
	return kref_get_unless_zero(&dr_ext->kref);
}
EXPORT_SYMBOL_NS_GPL(cxl_get_dr_ext_not_zero, CXL);

static void dr_ext_release(struct kref *kref)
{
	struct dr_extent *dr_ext = container_of(kref, struct dr_extent, kref);
	struct device *region_dev = dr_ext->dev.parent;

	devm_release_action(region_dev, dr_extent_unregister, dr_ext);
}

int cxl_put_dr_ext(struct dr_extent *dr_ext)
{
	return kref_put(&dr_ext->kref, dr_ext_release);
}
EXPORT_SYMBOL_NS_GPL(cxl_put_dr_ext, CXL);

int dax_region_create_ext(struct cxl_dax_region *cxlr_dax,
			  struct range *hpa_range,
			  const char *label,
			  struct range *dpa_range,
			  struct cxl_endpoint_decoder *cxled)
{
	struct dr_extent *dr_extent;
	struct device *dev;
	int rc, id;

	id = ida_alloc(&cxl_extent_ida, GFP_KERNEL);
	if (id < 0)
		return -ENOMEM;

	dr_extent = kzalloc(sizeof(*dr_extent), GFP_KERNEL);
	if (!dr_extent)
		return -ENOMEM;

	dr_extent->hpa_range = *hpa_range;
	dr_extent->ed_ext.dpa_range = *dpa_range;
	dr_extent->ed_ext.cxled = cxled;
	kref_init(&dr_extent->kref);
	snprintf(dr_extent->label, DAX_EXTENT_LABEL_LEN, "%s", label);

	dev = &dr_extent->dev;
	device_initialize(dev);
	dev->id = id;
	device_set_pm_not_required(dev);
	dev->parent = &cxlr_dax->dev;
	dev->type = &dr_extent_type;
	rc = dev_set_name(dev, "extent%d", dev->id);
	if (rc)
		goto err;

	rc = device_add(dev);
	if (rc)
		goto err;

	rc = cxl_region_notify_extent(cxled->cxld.region, DCD_ADD_CAPACITY, dr_extent);
	if (rc)
		goto err;

	dev_dbg(dev, "DAX region extent HPA %#llx - %#llx\n",
		dr_extent->hpa_range.start, dr_extent->hpa_range.end);

	return devm_add_action_or_reset(&cxlr_dax->dev, dr_extent_unregister, dr_extent);

err:
	dev_err(&cxlr_dax->dev, "Failed to initialize DAX extent dev HPA %#llx - %#llx\n",
		dr_extent->hpa_range.start, dr_extent->hpa_range.end);

	put_device(dev);
	return rc;
}
