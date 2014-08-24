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

struct nd_dimm;
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

struct nd_region *to_nd_region(struct device *dev);
void nd_region_wait_for_peers(struct nd_region *nd_region);
int nd_region_to_namespace_type(struct nd_region *nd_region);
extern struct attribute_group nd_device_attribute_group;
#endif /* __ND_H__ */
