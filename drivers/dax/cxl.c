// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2023 Intel Corporation. All rights reserved. */
#include <linux/module.h>
#include <linux/dax.h>

#include "../cxl/cxl.h"
#include "bus.h"
#include "dax-private.h"

static void cxl_dr_ext_release(void *pd)
{
	struct cxl_dr_extent *cxl_dr_ext = pd;

	cxl_dr_extent_put(cxl_dr_ext);
}

static int cxl_dax_region_create_extent(struct dax_region *dax_region,
					struct cxl_dr_extent *cxl_dr_ext)
{
	int rc;

	rc = dax_region_ext_create_dev(dax_region,
				       cxl_dr_ext,
				       cxl_dr_ext_release,
				       cxl_dr_ext->hpa_offset,
				       cxl_dr_ext->hpa_length,
				       cxl_dr_ext->label);
	if (rc)
		return rc;

	/* extent accepted */
	cxl_dr_extent_get(cxl_dr_ext);
	return 0;
}

static int cxl_dax_region_add_extent(struct cxl_dax_region *cxlr_dax,
				     struct cxl_dr_extent *cxl_dr_ext)
{
	struct dax_region *dax_region;
	int rc;

	/*
	 * get not zero is important because this is racing with the
	 * region driver which is racing with the memory device which
	 * could be removing the extent at the same time.
	 */
	if (!cxl_dr_extent_get_not_zero(cxl_dr_ext))
		return 0;

	dax_region = dev_get_drvdata(&cxlr_dax->dev);
	dev_dbg(&cxlr_dax->dev, "Adding extent HPA offset:%#llx LEN:%#llx\n",
		cxl_dr_ext->hpa_offset, cxl_dr_ext->hpa_length);
	rc = cxl_dax_region_create_extent(dax_region, cxl_dr_ext);
	cxl_dr_extent_put(cxl_dr_ext);
	return rc;
}

static int cxl_dax_region_create_extents(struct cxl_dax_region *cxlr_dax)
{
	struct cxl_dr_extent *cxl_dr_ext;
	unsigned long index;

	dev_dbg(&cxlr_dax->dev, "Adding extents\n");
	xa_for_each(&cxlr_dax->extents, index, cxl_dr_ext) {
		int rc;

		rc = cxl_dax_region_add_extent(cxlr_dax, cxl_dr_ext);
		if (rc)
			return rc;
	}
	return 0;
}

static int match_cxl_dr_extent(struct device *dev, void *data)
{
	struct dr_extent *dr_extent;

	if (!is_dr_extent(dev))
		return 0;

	dr_extent = to_dr_extent(dev);
	return data == dr_extent->private_data;
}

static int cxl_dax_region_rm_extent(struct cxl_dax_region *cxlr_dax,
				    struct cxl_dr_extent *cxl_dr_ext)
{
	struct dr_extent *dr_extent;
	struct dax_region *dax_region;
	struct device *dev;

	dev = device_find_child(&cxlr_dax->dev, cxl_dr_ext,
				match_cxl_dr_extent);
	if (!dev)
		return -EINVAL;

	dr_extent = to_dr_extent(dev);
	put_device(dev);
	dax_region = dev_get_drvdata(&cxlr_dax->dev);
	dax_region_ext_del(dax_region, dr_extent);

	return 0;
}

static int cxl_dax_region_notify(struct device *dev,
				 struct cxl_drv_nd *nd)
{
	struct cxl_dax_region *cxlr_dax = to_cxl_dax_region(dev);
	struct cxl_dr_extent *cxl_dr_ext = nd->cxl_dr_ext;

	switch (nd->event) {
	case DCD_ADD_CAPACITY:
		return cxl_dax_region_add_extent(cxlr_dax, cxl_dr_ext);
	case DCD_RELEASE_CAPACITY:
	case DCD_FORCED_CAPACITY_RELEASE:
		return cxl_dax_region_rm_extent(cxlr_dax, cxl_dr_ext);
	default:
		dev_err(&cxlr_dax->dev, "Unknown DC event %d\n", nd->event);
		break;
	}

	return 0;
}

static int cxl_dax_region_probe(struct device *dev)
{
	struct cxl_dax_region *cxlr_dax = to_cxl_dax_region(dev);
	int nid = phys_to_target_node(cxlr_dax->hpa_range.start);
	struct cxl_region *cxlr = cxlr_dax->cxlr;
	struct dax_region *dax_region;
	struct dev_dax_data data;
	resource_size_t dev_size;
	unsigned long flags;

	if (nid == NUMA_NO_NODE)
		nid = memory_add_physaddr_to_nid(cxlr_dax->hpa_range.start);

	flags = IORESOURCE_DAX_KMEM;
	if (cxlr->mode == CXL_REGION_DC)
		flags |= IORESOURCE_DAX_SPARSE_CAP;

	dax_region = alloc_dax_region(dev, cxlr->id, &cxlr_dax->hpa_range, nid,
				      PMD_SIZE, flags);
	if (!dax_region)
		return -ENOMEM;

	dev_size = range_len(&cxlr_dax->hpa_range);
	if (cxlr->mode == CXL_REGION_DC) {
		int rc;

		/* NOTE: Depends on dax_region being set in driver data */
		rc = cxl_dax_region_create_extents(cxlr_dax);
		if (rc)
			return rc;

		/* Add empty seed dax device */
		dev_size = 0;
	}

	data = (struct dev_dax_data) {
		.dax_region = dax_region,
		.id = -1,
		.size = dev_size,
	};

	return PTR_ERR_OR_ZERO(devm_create_dev_dax(&data));
}

static struct cxl_driver cxl_dax_region_driver = {
	.name = "cxl_dax_region",
	.probe = cxl_dax_region_probe,
	.notify = cxl_dax_region_notify,
	.id = CXL_DEVICE_DAX_REGION,
	.drv = {
		.suppress_bind_attrs = true,
	},
};

module_cxl_driver(cxl_dax_region_driver);
MODULE_ALIAS_CXL(CXL_DEVICE_DAX_REGION);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Intel Corporation");
MODULE_IMPORT_NS(CXL);
