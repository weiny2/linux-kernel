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
#include <linux/radix-tree.h>
#include <linux/device.h>
#include <linux/sizes.h>
#include <linux/mutex.h>
#include <linux/io.h>
#include <linux/nd.h>
#include "nfit.h"

extern struct list_head nd_bus_list;
extern struct mutex nd_bus_list_mutex;
extern int nd_dimm_major;

enum {
	/* need to set a limit somewhere, but yes, this is likely overkill */
	ND_IOCTL_MAX_BUFLEN = SZ_4M,
	/* mark newly adjusted resources as requiring a label update */
	DPA_RESOURCE_ADJUSTED = 1 << 0,
};

struct block_device;
struct nd_io_claim;
struct nd_btt;
struct nd_io;

/*
 * List manipulation is protected by nd_bus_list_mutex, except for the
 * deferred probe tracking list which nests under instances where
 * nd_bus_list_mutex is locked
 */
struct nd_bus {
	struct radix_tree_root interleave_sets;
	struct nfit_bus_descriptor *nfit_desc;
	struct radix_tree_root dimm_radix;
	wait_queue_head_t probe_wait;
	struct module *module;
	struct list_head memdevs;
	struct list_head spas;
	struct list_head dcrs;
	struct list_head bdws;
	struct list_head ndios;
	struct list_head list;
	struct device dev;
	int id, probe_active;
	struct mutex reconfig_mutex;
	struct nd_btt *nd_btt;
};

struct nd_dimm {
	unsigned long dsm_mask;
	struct nd_mem *nd_mem;
	struct device dev;
	void *provider_data;
	int id;
	struct nd_dimm_delete {
		struct nd_bus *nd_bus;
		struct nd_mem *nd_mem;
	} *del_info;
};

struct nd_interleave_set {
	u16 spa_index;
	u64 cookie;
	int busy;
};

struct nd_spa {
	struct nfit_spa __iomem *nfit_spa;
	struct nd_interleave_set *nd_set;
	struct list_head list;
};

struct nd_dcr {
	struct nfit_dcr __iomem *nfit_dcr;
	u32 parent_handle;
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

/* uniquely identify a mapping */
static inline u32 to_interleave_set_key(struct nd_mem *nd_mem)
{
	u16 dcr_index = readw(&nd_mem->nfit_mem->dcr_index);
	u16 spa_index = readw(&nd_mem->nfit_mem->spa_index);
	u32 key;

	key = spa_index;
	key <<= 16;
	key |= dcr_index;
	return key;
}

struct nd_io *ndio_lookup(struct nd_bus *nd_bus, const char *diskname);
const char *spa_type_name(u16 type);
int nfit_spa_type(struct nfit_bus_descriptor *nfit_desc,
		struct nfit_spa __iomem *nfit_spa);
struct nd_dimm *nd_dimm_by_handle(struct nd_bus *nd_bus, u32 nfit_handle);
bool is_nd_dimm(struct device *dev);
bool is_nd_blk(struct device *dev);
bool is_nd_pmem(struct device *dev);
#if IS_ENABLED(CONFIG_ND_BTT_DEVS)
bool is_nd_btt(struct device *dev);
struct nd_btt *nd_btt_create(struct nd_bus *nd_bus, struct block_device *bdev,
		struct nd_io_claim *ndio_claim, unsigned long lbasize,
		u8 uuid[16]);
#else
static inline bool is_nd_btt(struct device *dev)
{
	return false;
}

static inline struct nd_btt *nd_btt_create(struct nd_bus *nd_bus,
		struct block_device *bdev, struct nd_io_claim *ndio_claim,
		unsigned long lbasize, u8 uuid[16])
{
	return NULL;
}
#endif
struct nd_bus *to_nd_bus(struct device *dev);
bool is_nd_dimm(struct device *dev);
struct nd_bus *walk_to_nd_bus(struct device *nd_dev);
void nd_synchronize(void);
int __init nd_bus_init(void);
void nd_bus_exit(void);
void nd_dimm_delete(struct nd_dimm *nd_dimm);
int __init nd_dimm_init(void);
int __init nd_region_init(void);
void nd_dimm_exit(void);
int nd_region_exit(void);
void nd_region_probe_start(struct nd_bus *nd_bus, struct device *dev);
void nd_region_probe_end(struct nd_bus *nd_bus, struct device *dev, int rc);
struct nd_region;
void nd_region_create_blk_seed(struct nd_region *nd_region);
void nd_region_notify_remove(struct nd_bus *nd_bus, struct device *dev, int rc);
int nd_bus_create_ndctl(struct nd_bus *nd_bus);
void nd_bus_destroy_ndctl(struct nd_bus *nd_bus);
const char *nd_bus_provider(struct nd_bus *nd_bus);
int nd_bus_register_dimms(struct nd_bus *nd_bus);
int nd_bus_register_regions(struct nd_bus *nd_bus);
int nd_bus_init_interleave_sets(struct nd_bus *nd_bus);
int nd_match_dimm(struct device *dev, void *data);
bool is_nd_dimm(struct device *dev);
struct nd_label_id;
char *nd_label_gen_id(struct nd_label_id *label_id, u8 *uuid, u32 flags);
bool nd_is_uuid_unique(struct device *dev, u8 *uuid);
struct nd_dimm_drvdata;
resource_size_t nd_dimm_available_dpa(struct nd_dimm_drvdata *ndd,
		struct nd_region *nd_region);
struct resource *nd_dimm_allocate_dpa(struct nd_dimm_drvdata *ndd,
		struct nd_label_id *label_id, resource_size_t start,
		resource_size_t n);
resource_size_t nd_dimm_allocated_dpa(struct nd_dimm_drvdata *ndd,
		struct nd_label_id *label_id);
struct nd_mapping;
struct resource *nsblk_add_resource(struct nd_region *nd_region,
		struct nd_dimm_drvdata *ndd, struct nd_namespace_blk *nsblk,
		resource_size_t start);
#endif /* __ND_PRIVATE_H__ */
