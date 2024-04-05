// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2023 Intel Corporation. All rights reserved. */
#include <linux/module.h>
#include <linux/dax.h>

#include "../cxl/cxl.h"
#include "bus.h"
#include "dax-private.h"

static int __cxl_dax_region_add_extent(struct dax_region *dax_region,
				       struct region_extent *reg_ext)
{
	struct device *ext_dev = &reg_ext->dev;
	resource_size_t start, length;

	dev_dbg(dax_region->dev, "Adding extent HPA %#llx - %#llx\n",
		reg_ext->hpa_range.start, reg_ext->hpa_range.end);

	start = dax_region->res.start + reg_ext->hpa_range.start;
	length = reg_ext->hpa_range.end - reg_ext->hpa_range.start + 1;

	return dax_region_add_extent(dax_region, ext_dev, start, length);
}

static int cxl_dax_region_add_extent(struct device *dev, void *data)
{
	struct dax_region *dax_region = data;
	struct region_extent *reg_ext;

	if (!is_region_extent(dev))
		return 0;

	reg_ext = to_region_extent(dev);

	return __cxl_dax_region_add_extent(dax_region, reg_ext);
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
