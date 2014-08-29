/*
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
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
 *
 * BSD LICENSE
 *
 * Copyright(c) 2013-2014 Intel Corporation. All rights reserved.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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
	int format_interface_code;
	struct nfit __iomem *nfit;
	struct list_head regions;
	struct list_head memdevs;
	struct list_head spas;
	struct list_head dcrs;
	struct list_head bdws;
	struct list_head list;
	struct device dev;
	int id;
};

static inline struct nd_bus *to_nd_bus(struct device *dev)
{
	return container_of(dev, struct nd_bus, dev);
}

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
	struct list_head list;
};

int __init nd_bus_init(void);
void __init nd_bus_exit(void);
int nd_bus_create(struct nd_bus *nd_bus);
void nd_bus_destroy(struct nd_bus *nd_bus);
int nd_bus_register_dimms(struct nd_bus *nd_bus);
int nd_bus_register_regions(struct nd_bus *nd_bus);
#endif /* __ND_PRIVATE_H__ */
