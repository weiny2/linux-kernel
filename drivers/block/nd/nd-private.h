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
#ifndef __ND_PRIVATE_H__
#define __ND_PRIVATE_H__
#include <linux/device.h>
#include <linux/sizes.h>

extern struct list_head nd_bus_list;
extern struct mutex nd_bus_list_mutex;

enum {
	/* need to set a limit somewhere, but yes, this is ludicrously big */
	ND_IOCTL_MAX_BUFLEN = SZ_4M,
};

struct nd_bus {
	struct nfit_bus_descriptor *nfit_desc;
	struct completion registration;
	int format_interface_code;
	wait_queue_head_t deferq;
	struct list_head deferred;
	struct list_head memdevs;
	struct list_head spas;
	struct list_head dcrs;
	struct list_head bdws;
	struct list_head list;
	struct device dev;
	int id;
};

struct nd_spa {
	struct nfit_spa __iomem *nfit_spa;
	struct list_head list;
};

struct nd_dcr {
	struct nfit_dcr __iomem *nfit_dcr;
	struct list_head list;
};

struct nd_bdw {
	struct nfit_bdw __iomem *nfit_bdw;
	struct list_head list;
};

struct nd_mem {
	struct nfit_mem __iomem *nfit_mem;
	struct nfit_dcr __iomem *nfit_dcr;
	struct list_head list;
};

void nd_bus_wait_probe(struct nd_bus *nd_bus);
bool is_nd_dimm(struct device *dev);
bool is_nd_blk(struct device *dev);
bool is_nd_pmem(struct device *dev);
struct nd_bus *to_nd_bus(struct device *dev);
struct nd_bus *walk_to_nd_bus(struct device *nd_dev);
int __init nd_bus_init(void);
void nd_bus_exit(void);
int __init nd_dimm_init(void);
int __init nd_region_init(void);
void nd_dimm_exit(void);
int nd_region_exit(void);
int nd_bus_create_ndctl(struct nd_bus *nd_bus);
void nd_bus_destroy_ndctl(struct nd_bus *nd_bus);
int nd_bus_register_dimms(struct nd_bus *nd_bus);
int nd_bus_register_regions(struct nd_bus *nd_bus);
int nd_match_dimm(struct device *dev, void *data);
bool is_nd_dimm(struct device *dev);
#endif /* __ND_PRIVATE_H__ */
