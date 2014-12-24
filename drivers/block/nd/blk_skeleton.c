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
#include <linux/nd.h>
#include "nd.h"

static int nd_blk_probe(struct device *dev)
{
	struct nd_namespace_blk *nsblk = to_nd_namespace_blk(dev);
	resource_size_t size = 0;
	int i;

	for (i = 0; i < nsblk->num_resources; i++)
		size += resource_size(nsblk->res[i]);

	if (size < ND_MIN_NAMESPACE_SIZE || !nsblk->uuid
			|| !nsblk->lbasize)
		return -ENXIO;

	return 0;
}

static int nd_blk_remove(struct device *dev)
{
	return 0;
}

static struct nd_device_driver nd_blk_driver = {
	.probe = nd_blk_probe,
	.remove = nd_blk_remove,
	.drv = {
		.name = "nd_blk_skeleton",
	},
	.type = ND_DRIVER_NAMESPACE_BLOCK,
};

int __init nd_blk_init(void)
{
	return nd_driver_register(&nd_blk_driver);
}

void __exit nd_blk_exit(void)
{
	driver_unregister(&nd_blk_driver.drv);
}

module_init(nd_blk_init);
module_exit(nd_blk_exit);
MODULE_ALIAS_ND_DEVICE(ND_DEVICE_NAMESPACE_BLOCK);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Intel Corporation");
