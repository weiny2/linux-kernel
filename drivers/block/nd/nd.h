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
#include <linux/types.h>
#include "label.h"

enum {
	/*
	 * Limits the maximum number of block apertures a dimm can
	 * support and is an input to the geometry/on-disk-format of a
	 * BTT instance
	 */
	ND_MAX_LANES = 256,
};

struct gendisk;

struct nd_dimm_drvdata {
	struct device *dev;
	int nsindex_size;
	struct nfit_cmd_get_config_size nsarea;
	void *data;
	int ns_current, ns_next;
	struct resource dpa;
};

struct nd_region_namespaces {
	int count;
	int active;
};

static inline struct nd_namespace_index __iomem *to_namespace_index(
		struct nd_dimm_drvdata *ndd, int i)
{
	if (i < 0)
		return NULL;

	return ((void __iomem *) ndd->data + sizeof_namespace_index(ndd) * i);
}

static inline struct nd_namespace_index __iomem *to_current_namespace_index(
		struct nd_dimm_drvdata *ndd)
{
	return to_namespace_index(ndd, ndd->ns_current);
}

static inline struct nd_namespace_index __iomem *to_next_namespace_index(
		struct nd_dimm_drvdata *ndd)
{
	return to_namespace_index(ndd, ndd->ns_next);
}

struct nd_dimm;
struct nd_mapping {
	struct nd_dimm *nd_dimm;
	struct nd_namespace_label **labels;
	u64 start;
	u64 size;
};

#define nd_dbg_dpa(r, d, res, fmt, arg...) \
	dev_dbg((r) ? &(r)->dev : (d)->dev, "%s: %.13s: %#llx @ %#llx " fmt, \
		(r) ? dev_name((d)->dev) : "", res ? res->name : "null", \
		(unsigned long long) res ? resource_size(res) : 0, \
		(unsigned long long) res ? res->start : 0, ##arg)

/* sparse helpers */
static inline void nd_set_label(struct nd_namespace_label **labels,
		struct nd_namespace_label __iomem *label, int idx)
{
	labels[idx] = (void __force *) label;
}

static inline struct nd_namespace_label __iomem *nd_get_label(
		struct nd_namespace_label **labels, int idx)
{
	struct nd_namespace_label __iomem *label = NULL;

	if (labels)
		label = (struct nd_namespace_label __iomem *) labels[idx];

	return label;
}

#define for_each_label(l, label, labels) \
	for (l = 0; (label = nd_get_label(labels, l)); l++)

#define for_each_dpa_resource(ndd, res) \
	for (res = (ndd)->dpa.child; res; res = res->sibling)

#define for_each_dpa_resource_safe(ndd, res, next) \
	for (res = (ndd)->dpa.child, next = res ? res->sibling : NULL; \
			res; res = next, next = next ? next->sibling : NULL)

struct nd_region {
	struct device dev;
	struct nd_spa *nd_spa;
	struct ida ns_ida;
	struct device *ns_seed;
	u16 ndr_mappings;
	u64 ndr_size;
	u64 ndr_start;
	int id;
	int num_lanes;
	/* only valid for blk regions */
	struct nd_blk_window {
		u64 bdw_size;
		u64 ctl_size;
		void *aperture;
		u64 __iomem *ctl_base;
		u32 __iomem *stat_base;
	} bw;
	struct nd_mapping mapping[0];
};

static inline struct nd_region *ndbw_to_region(struct nd_blk_window *ndbw)
{
	return container_of(ndbw, struct nd_region, bw);
}

struct nd_io;
/**
 * nd_rw_bytes_fn() - access bytes relative to the "whole disk" namespace device
 * @ndio: per-namespace context
 * @buf: source / target for the write / read
 * @offset: offset relative to the start of the namespace device
 * @n: num bytes to access
 * @flags: READ, WRITE, and other REQ_* flags
 *
 * Note: Implementations may assume that offset + n never crosses ndio->align
 */
typedef int (*nd_rw_bytes_fn)(struct nd_io *ndio, void *buf, size_t offset,
		size_t n, unsigned long flags);
#define nd_data_dir(flags) (flags & 1)

/*
 * Lookup next in the repeating sequence of 01, 10, and 11.
 */
static inline unsigned nd_inc_seq(unsigned seq)
{
	static const unsigned next[] = { 0, 2, 3, 1 };

	return next[seq & 3];
}

/**
 * struct nd_io - info for byte-aligned access to nd devices
 * @rw_bytes: operation to perform byte-aligned access
 * @align: a single ->rw_bytes() request may not cross this alignment
 * @gendisk: whole disk block device for the namespace
 * @list: for the core to cache a list of "ndio"s for later association
 * @dev: namespace device
 * @claims: list of clients using this interface
 * @lock: protect @claims mutation
 */
struct nd_io {
	nd_rw_bytes_fn rw_bytes;
	unsigned long align;
	struct gendisk *disk;
	struct list_head list;
	struct device *dev;
	struct list_head claims;
	spinlock_t lock;
};

struct nd_io_claim;
typedef void (*ndio_notify_remove_fn)(struct nd_io_claim *ndio_claim);

/**
 * struct nd_io_claim - instance of a claim on a parent ndio
 * @notify_remove: ndio is going away, release resources
 * @holder: object that has claimed this ndio
 * @parent: ndio in use
 * @holder: holder device
 * @list: claim peers
 *
 * An ndio may be claimed multiple times, consider the case of a btt
 * instance per partition on a namespace.
 */
struct nd_io_claim {
	struct nd_io *parent;
	ndio_notify_remove_fn notify_remove;
	struct list_head list;
	struct device *holder;
};

struct nd_btt {
	struct device dev;
	struct nd_io *ndio;
	struct block_device *backing_dev;
	unsigned long lbasize;
	u8 *uuid;
	u64 offset;
	int id;
	struct nd_io_claim *ndio_claim;
};

enum nd_async_mode {
	ND_SYNC,
	ND_ASYNC,
};

void wait_nd_bus_probe_idle(struct device *dev);
void nd_device_register(struct device *dev);
void nd_device_unregister(struct device *dev, enum nd_async_mode mode);
u64 nd_fletcher64(void __iomem *addr, size_t len);
int nd_uuid_store(struct device *dev, u8 **uuid_out, const char *buf,
		size_t len);
ssize_t nd_sector_size_show(unsigned long current_lbasize,
		const unsigned long *supported, char *buf);
ssize_t nd_sector_size_store(struct device *dev, const char *buf,
		unsigned long *current_lbasize, const unsigned long *supported);
int nd_register_ndio(struct nd_io *ndio);
int nd_unregister_ndio(struct nd_io *ndio);
void nd_init_ndio(struct nd_io *ndio, nd_rw_bytes_fn rw_bytes,
		struct device *dev, struct gendisk *disk, unsigned long align);
void ndio_del_claim(struct nd_io_claim *ndio_claim);
struct nd_io_claim *ndio_add_claim(struct nd_io *ndio, struct device *holder,
		ndio_notify_remove_fn notify_remove);
extern struct attribute_group nd_device_attribute_group;
struct nd_dimm *to_nd_dimm(struct device *dev);
struct nd_dimm_drvdata *to_ndd(struct nd_mapping *nd_mapping);
u32 to_nfit_handle(struct nd_dimm *nd_dimm);
void *nd_dimm_get_pdata(struct nd_dimm *nd_dimm);
void nd_dimm_set_pdata(struct nd_dimm *nd_dimm, void *data);
unsigned long nd_dimm_get_dsm_mask(struct nd_dimm *nd_dimm);
void nd_dimm_set_dsm_mask(struct nd_dimm *nd_dimm, unsigned long dsm_mask);
struct nd_btt *to_nd_btt(struct device *dev);
struct nd_region *to_nd_region(struct device *dev);
int nd_dimm_get_config_size(struct nd_dimm_drvdata *ndd,
		struct nfit_cmd_get_config_size *cmd);
int nd_dimm_get_config_data(struct nd_dimm_drvdata *ndd,
		struct nfit_cmd_get_config_data_hdr *cmd, size_t len);
int nd_dimm_set_config_data(struct nd_dimm_drvdata *ndd, size_t offset,
		void *buf, size_t len);
int nd_blk_init_region(struct nd_region *nd_region);
unsigned int nd_region_acquire_lane(struct nd_region *nd_region);
void nd_region_release_lane(struct nd_region *nd_region, unsigned int lane);
int nd_region_to_namespace_type(struct nd_region *nd_region);
int nd_region_register_namespaces(struct nd_region *nd_region, int *err);
u64 nd_region_interleave_set_cookie(struct nd_region *nd_region);
void nd_bus_lock(struct device *dev);
void nd_bus_unlock(struct device *dev);
bool is_nd_bus_locked(struct device *dev);
int nd_label_reserve_dpa(struct nd_dimm_drvdata *ndd);
const char *nd_blk_bus_provider(struct device *dev);
int nd_blk_do_io(struct nd_blk_window *ndbw, struct page *page,
		unsigned int len, unsigned int off, int rw,
		resource_size_t dev_offset);
resource_size_t nd_namespace_blk_validate(struct nd_namespace_blk *nsblk);
int nd_dimm_init_nsarea(struct nd_dimm_drvdata *ndd);
int nd_dimm_init_config_data(struct nd_dimm_drvdata *ndd);
int nd_dimm_firmware_status(struct device *dev);
#endif /* __ND_H__ */
