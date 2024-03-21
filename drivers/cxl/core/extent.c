// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2024 Intel Corporation. All rights reserved. */

#include <linux/device.h>
#include <linux/slab.h>
#include <cxl.h>

static DEFINE_IDA(cxl_extent_ida);

static ssize_t offset_show(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
	struct region_extent *reg_ext = to_region_extent(dev);

	return sysfs_emit(buf, "%pa\n", &reg_ext->hpa_range.start);
}
static DEVICE_ATTR_RO(offset);

static ssize_t length_show(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
	struct region_extent *reg_ext = to_region_extent(dev);
	u64 length = range_len(&reg_ext->hpa_range);

	return sysfs_emit(buf, "%pa\n", &length);
}
static DEVICE_ATTR_RO(length);

static ssize_t label_show(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	struct region_extent *reg_ext = to_region_extent(dev);

	return sysfs_emit(buf, "%s\n", reg_ext->label);
}
static DEVICE_ATTR_RO(label);

static struct attribute *region_extent_attrs[] = {
	&dev_attr_offset.attr,
	&dev_attr_length.attr,
	&dev_attr_label.attr,
	NULL,
};

static const struct attribute_group region_extent_attribute_group = {
	.attrs = region_extent_attrs,
};

static const struct attribute_group *region_extent_attribute_groups[] = {
	&region_extent_attribute_group,
	NULL,
};

static void region_extent_release(struct device *dev)
{
	struct region_extent *reg_ext = to_region_extent(dev);

	cxl_release_ed_extent(&reg_ext->ed_ext);
	ida_free(&cxl_extent_ida, reg_ext->dev.id);
	kfree(reg_ext);
}

static const struct device_type region_extent_type = {
	.name = "extent",
	.release = region_extent_release,
	.groups = region_extent_attribute_groups,
};

bool is_region_extent(struct device *dev)
{
	return dev->type == &region_extent_type;
}
EXPORT_SYMBOL_NS_GPL(is_region_extent, CXL);

static void region_extent_unregister(void *ext)
{
	struct region_extent *reg_ext = ext;

	dev_dbg(&reg_ext->dev, "DAX region rm extent HPA %#llx - %#llx\n",
		reg_ext->hpa_range.start, reg_ext->hpa_range.end);
	device_unregister(&reg_ext->dev);
}

void dax_reg_ext_release(struct region_extent *reg_ext)
{
	struct device *region_dev = reg_ext->dev.parent;

	devm_release_action(region_dev, region_extent_unregister, reg_ext);
}
EXPORT_SYMBOL_NS_GPL(dax_reg_ext_release, CXL);

int dax_region_create_ext(struct cxl_dax_region *cxlr_dax,
			  struct range *hpa_range,
			  const char *label,
			  struct range *dpa_range,
			  struct cxl_endpoint_decoder *cxled)
{
	struct region_extent *reg_ext;
	struct device *dev;
	int rc, id;

	id = ida_alloc(&cxl_extent_ida, GFP_KERNEL);
	if (id < 0)
		return -ENOMEM;

	reg_ext = kzalloc(sizeof(*reg_ext), GFP_KERNEL);
	if (!reg_ext)
		return -ENOMEM;

	reg_ext->hpa_range = *hpa_range;
	reg_ext->ed_ext.dpa_range = *dpa_range;
	reg_ext->ed_ext.cxled = cxled;
	snprintf(reg_ext->label, DAX_EXTENT_LABEL_LEN, "%s", label);

	dev = &reg_ext->dev;
	device_initialize(dev);
	dev->id = id;
	device_set_pm_not_required(dev);
	dev->parent = &cxlr_dax->dev;
	dev->type = &region_extent_type;
	rc = dev_set_name(dev, "extent%d", dev->id);
	if (rc)
		goto err;

	rc = device_add(dev);
	if (rc)
		goto err;

	rc = cxl_region_notify_extent(cxled->cxld.region, DCD_ADD_CAPACITY, reg_ext);
	if (rc)
		goto err;

	dev_dbg(dev, "DAX region extent HPA %#llx - %#llx\n",
		reg_ext->hpa_range.start, reg_ext->hpa_range.end);

	return devm_add_action_or_reset(&cxlr_dax->dev, region_extent_unregister,
	reg_ext);

err:
	dev_err(&cxlr_dax->dev, "Failed to initialize DAX extent dev HPA %#llx - %#llx\n",
		reg_ext->hpa_range.start, reg_ext->hpa_range.end);

	put_device(dev);
	return rc;
}
