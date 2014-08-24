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
#ifndef __ND_H__
#define __ND_H__
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/ndctl.h>

struct nfit_mem;
struct nfit_dcr;
struct nd_dimm {
	struct nfit_mem __iomem *nfit_mem;
	struct nfit_dcr __iomem *nfit_dcr;
	struct device dev;
	int config_size;
	int id;
	struct nfit_cmd_get_config_data *data;
};

struct nd_mapping {
	struct nd_dimm *nd_dimm;
	u64 start;
	u64 size;
};

struct nd_region {
	struct device dev;
	u16 spa_index;
	u16 interleave_ways;
	u16 ndr_mappings;
	u64 ndr_size;
	u64 ndr_start;
	int id;
	struct nd_mapping mapping[0];
};

enum nd_async_mode {
	ND_SYNC,
	ND_ASYNC,
};

int nd_device_register(struct device *dev, enum nd_async_mode mode);
void nd_device_unregister(struct device *dev, enum nd_async_mode mode);
extern struct attribute_group nd_device_attribute_group;
struct nd_region *to_nd_region(struct device *dev);
struct nd_dimm *to_nd_dimm(struct device *dev);
struct nd_region *to_nd_region(struct device *dev);
int nd_dimm_get_config_size(struct nd_dimm *nd_dimm,
		struct nfit_cmd_get_config_size *cmd);
int nd_dimm_get_config_data(struct nd_dimm *nd_dimm,
		struct nfit_cmd_get_config_data *cmd, size_t len);
int nd_region_to_namespace_type(struct nd_region *nd_region);
#endif /* __ND_H__ */
