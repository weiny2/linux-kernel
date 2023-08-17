// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2023 Intel Corporation. All rights reserved. */
#include <linux/module.h>
#include <linux/dax.h>

#include "../cxl/cxl.h"
#include "bus.h"
#include "dax-private.h"

static void dax_reg_ext_get(struct dax_region_extent *dr_extent)
{
	kref_get(&dr_extent->ref);
}

static void dr_release(struct kref *kref)
{
	struct dax_region_extent *dr_extent;
	struct cxl_dr_extent *cxl_dr_ext;

	dr_extent = container_of(kref, struct dax_region_extent, ref);
	cxl_dr_ext = dr_extent->private_data;
	cxl_dr_extent_put(cxl_dr_ext);
	kfree(dr_extent);
}

static void dax_reg_ext_put(struct dax_region_extent *dr_extent)
{
	kref_put(&dr_extent->ref, dr_release);
}

static int cxl_dax_region_create_extent(struct dax_region *dax_region,
					struct cxl_dr_extent *cxl_dr_ext)
{
	struct dax_region_extent *dr_extent;
	int rc;

	dr_extent = kzalloc(sizeof(*dr_extent), GFP_KERNEL);
	if (!dr_extent)
		return -ENOMEM;

	dr_extent->private_data = cxl_dr_ext;
	dr_extent->get = dax_reg_ext_get;
	dr_extent->put = dax_reg_ext_put;

	/* device manages the dr_extent on success */
	kref_init(&dr_extent->ref);

	rc = dax_region_ext_create_dev(dax_region, dr_extent,
				       cxl_dr_ext->hpa_offset,
				       cxl_dr_ext->hpa_length,
				       cxl_dr_ext->label);
	if (rc) {
		kfree(dr_extent);
		return rc;
	}

	/* extent accepted */
	cxl_dr_extent_get(cxl_dr_ext);
	return 0;
}

static int cxl_dax_region_create_extents(struct cxl_dax_region *cxlr_dax)
{
	struct cxl_dr_extent *cxl_dr_ext;
	unsigned long index;

	dev_dbg(&cxlr_dax->dev, "Adding extents\n");
	xa_for_each(&cxlr_dax->extents, index, cxl_dr_ext) {
		/*
		 * get not zero is important because this is racing with the
		 * region driver which is racing with the memory device which
		 * could be removing the extent at the same time.
		 */
		if (cxl_dr_extent_get_not_zero(cxl_dr_ext)) {
			struct dax_region *dax_region;
			int rc;

			dax_region = dev_get_drvdata(&cxlr_dax->dev);
			dev_dbg(&cxlr_dax->dev, "Found OFF:%llx LEN:%llx\n",
				cxl_dr_ext->hpa_offset, cxl_dr_ext->hpa_length);
			rc = cxl_dax_region_create_extent(dax_region, cxl_dr_ext);
			cxl_dr_extent_put(cxl_dr_ext);
			if (rc)
				return rc;
		}
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
		flags |= IORESOURCE_DAX_DYNAMIC_CAP;

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
