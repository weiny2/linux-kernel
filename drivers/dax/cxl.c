// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2023 Intel Corporation. All rights reserved. */
#include <linux/module.h>
#include <linux/dax.h>

#include "../cxl/cxl.h"
#include "bus.h"
#include "dax-private.h"

static void release_extent_reg(struct dax_extent *dax_ext)
{
	struct dax_region *dax_region = dax_ext->region;

	dev_dbg(dax_region->dev, "Extent release resource %pr\n", dax_ext->res);
	if (dax_ext->res)
		__release_region(&dax_region->res, dax_ext->res->start,
				 resource_size(dax_ext->res));
	dax_ext->res = NULL;
}

static void dax_region_rm_resource(void *ext)
{
	struct dax_extent *dax_ext = ext;

	release_extent_reg(dax_ext);
	kfree(dax_ext);
}

static int dax_region_add_resource(struct dax_region *dax_region,
				   struct dax_extent *dax_ext,
				   struct dr_extent *dr_extent)
{
	resource_size_t start = dax_region->res.start + dr_extent->hpa_range.start;
	resource_size_t length = dr_extent->hpa_range.end -
				 dr_extent->hpa_range.start + 1;
	struct resource *ext_res;

	dev_dbg(dax_region->dev, "DAX region resource %pr\n", &dax_region->res);
	ext_res = __request_region(&dax_region->res, start, length, "extent", 0);
	if (!ext_res) {
		dev_err(dax_region->dev, "Failed to add region s:%pa l:%pa\n",
			&start, &length);
		return -ENOSPC;
	}

	dax_ext->region = dax_region;
	dax_ext->res = ext_res;
	dev_dbg(dax_region->dev, "Extent add resource %pr\n", ext_res);

	return 0;
}

static int __cxl_dax_region_add_extent(struct dax_region *dax_region,
				       struct dr_extent *dr_extent)
{
	struct dax_extent *dax_ext __free(kfree) = NULL;
	struct device *dev __free(put_device) = get_device(&dr_extent->dev);
	int rc;

	dev_dbg(dax_region->dev, "Adding extent HPA %#llx - %#llx\n",
		dr_extent->hpa_range.start, dr_extent->hpa_range.end);

	dax_ext = kzalloc(sizeof(*dax_ext), GFP_KERNEL);
	if (!dax_ext)
		return 0;

	rc = dax_region_add_resource(dax_region, dax_ext, dr_extent);
	if (rc)
		return rc;

	return devm_add_action_or_reset(dev, dax_region_rm_resource,
					no_free_ptr(dax_ext));
}

static int cxl_dax_region_add_extent(struct device *dev, void *data)
{
	struct dax_region *dax_region = data;
	struct dr_extent *dr_extent;

	if (!is_dr_extent(dev))
		return 0;

	dr_extent = to_dr_extent(dev);

	return __cxl_dax_region_add_extent(dax_region, dr_extent);
}

static void cxl_dax_region_add_extents(struct cxl_dax_region *cxlr_dax,
				      struct dax_region *dax_region)
{
	dev_dbg(&cxlr_dax->dev, "Adding extents\n");
	device_for_each_child(&cxlr_dax->dev, dax_region, cxl_dax_region_add_extent);
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
		/* NOTE: Depends on dax_region being set in driver data */
		cxl_dax_region_add_extents(cxlr_dax, dax_region);
		/* Add empty seed dax device */
		dev_size = 0;
	}

	data = (struct dev_dax_data) {
		.dax_region = dax_region,
		.id = -1,
		.size = dev_size,
		.memmap_on_memory = true,
	};

	return PTR_ERR_OR_ZERO(devm_create_dev_dax(&data));
}

static struct cxl_driver cxl_dax_region_driver = {
	.name = "cxl_dax_region",
	.probe = cxl_dax_region_probe,
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
