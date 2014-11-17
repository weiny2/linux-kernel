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
#include <linux/sizes.h>
#include <linux/ndctl.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/nd.h>
#include "nd.h"

static void free_data(struct nd_dimm *nd_dimm)
{
	if (!nd_dimm || !nd_dimm->data)
		return;

	if (nd_dimm->data && is_vmalloc_addr(nd_dimm->data))
		vfree(nd_dimm->data);
	else
		kfree(nd_dimm->data);
	nd_dimm->data = NULL;
}

static int nd_dimm_probe(struct device *dev)
{
	int header_size = sizeof(struct nfit_cmd_get_config_data_hdr);
	struct nd_dimm *nd_dimm = to_nd_dimm(dev);
	struct nfit_cmd_get_config_size cmd_size;
	int rc, config_size;

	nd_dimm->config_size = -EINVAL;
	rc = nd_dimm_get_config_size(nd_dimm, &cmd_size);
	if (rc || cmd_size.config_size + header_size > INT_MAX)
		return -ENXIO;

	config_size = cmd_size.config_size + header_size;
	dev_dbg(&nd_dimm->dev, "config_size: %d\n", config_size);
	nd_dimm->data = kmalloc(config_size, GFP_KERNEL);
	if (!nd_dimm->data)
		nd_dimm->data = vmalloc(config_size);

	if (!nd_dimm->data)
		return -ENOMEM;

	rc = nd_dimm_get_config_data(nd_dimm, nd_dimm->data, config_size);
	if (rc) {
		free_data(nd_dimm);
		return rc < 0 ? rc : -ENXIO;
	}
	nd_dimm->config_size = config_size;

	return 0;
}

static int nd_dimm_remove(struct device *dev)
{
	struct nd_dimm *nd_dimm = to_nd_dimm(dev);

	free_data(nd_dimm);

	return 0;
}

static struct nd_device_driver nd_dimm_driver = {
	.probe = nd_dimm_probe,
	.remove = nd_dimm_remove,
	.drv = {
		.name = "nd_dimm",
	},
	.type = ND_DRIVER_DIMM,
};

int __init nd_dimm_init(void)
{
	return nd_driver_register(&nd_dimm_driver);
}

void nd_dimm_exit(void)
{
	driver_unregister(&nd_dimm_driver.drv);
}

MODULE_ALIAS_ND_DEVICE(ND_DEVICE_DIMM);
