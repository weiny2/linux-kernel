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
#ifndef __LINUX_ND_H__
#define __LINUX_ND_H__
#include <linux/device.h>

struct nd_device_driver {
	struct device_driver drv;
	unsigned long type;
};

static inline struct nd_device_driver *to_nd_drv(struct device_driver *drv)
{
	return container_of(drv, struct nd_device_driver, drv);
}

struct nd_namespace_io {
	struct device dev;
	struct resource res;
};

static inline struct nd_namespace_io *to_nd_namespace_io(struct device *dev)
{
	return container_of(dev, struct nd_namespace_io, dev);
}

#define ND_DEVICE_DIMM 1            /* nd_dimm (no driver, informational) */
#define ND_DEVICE_REGION_PMEM 2     /* nd_region (parent of pmem namespaces) */
#define ND_DEVICE_REGION_BLOCK 3    /* nd_region (parent of block namespaces) */
#define ND_DEVICE_NAMESPACE_IO 4    /* legacy persistent memory */
#define ND_DEVICE_NAMESPACE_PMEM 5  /* persistent memory namespace (may alias) */
#define ND_DEVICE_NAMESPACE_BLOCK 6 /* block-data-window namespace (may alias) */

enum nd_driver_flags {
	ND_DRIVER_DIMM            = 1 << ND_DEVICE_DIMM,
	ND_DRIVER_REGION_PMEM     = 1 << ND_DEVICE_REGION_PMEM,
	ND_DRIVER_REGION_BLOCK    = 1 << ND_DEVICE_REGION_BLOCK,
	ND_DRIVER_NAMESPACE_IO    = 1 << ND_DEVICE_NAMESPACE_IO,
	ND_DRIVER_NAMESPACE_PMEM  = 1 << ND_DEVICE_NAMESPACE_PMEM,
	ND_DRIVER_NAMESPACE_BLOCK = 1 << ND_DEVICE_NAMESPACE_BLOCK,
};

#define MODULE_ALIAS_ND_DEVICE(type) \
	MODULE_ALIAS("nd:t" __stringify(type) "*")
#define ND_DEVICE_MODALIAS_FMT "nd:t%d"

int __must_check __nd_driver_register(struct nd_device_driver *nd_drv,
		struct module *module, const char *mod_name);
#define nd_driver_register(driver) \
	__nd_driver_register(driver, THIS_MODULE, KBUILD_MODNAME)
#endif /* __LINUX_ND_H__ */
