/*
 * Copyright(c) 2013-2015 Intel Corporation. All rights reserved.
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
#include <linux/libnvdimm.h>
#include <linux/blkdev.h>
#include <linux/device.h>
#include <linux/genhd.h>
#include <linux/mutex.h>
#include <linux/ndctl.h>
#include <linux/types.h>
#include <linux/fs.h>
#include "label.h"

enum {
	/*
	 * Limits the maximum number of block apertures a dimm can
	 * support and is an input to the geometry/on-disk-format of a
	 * BTT instance
	 */
	ND_MAX_LANES = 256,
	SECTOR_SHIFT = 9,
	INT_LBASIZE_ALIGNMENT = 64,
};

struct nvdimm_drvdata {
	struct device *dev;
	int nsindex_size;
	struct nd_cmd_get_config_size nsarea;
	void *data;
	int ns_current, ns_next;
	struct resource dpa;
};

struct nd_region_namespaces {
	int count;
	int active;
};

static inline struct nd_namespace_index __iomem *to_namespace_index(
		struct nvdimm_drvdata *ndd, int i)
{
	if (i < 0)
		return NULL;

	return ((void __iomem *) ndd->data + sizeof_namespace_index(ndd) * i);
}

static inline struct nd_namespace_index __iomem *to_current_namespace_index(
		struct nvdimm_drvdata *ndd)
{
	return to_namespace_index(ndd, ndd->ns_current);
}

static inline struct nd_namespace_index __iomem *to_next_namespace_index(
		struct nvdimm_drvdata *ndd)
{
	return to_namespace_index(ndd, ndd->ns_next);
}

#define nd_dbg_dpa(r, d, res, fmt, arg...) \
	dev_dbg((r) ? &(r)->dev : (d)->dev, "%s: %.13s: %#llx @ %#llx " fmt, \
		(r) ? dev_name((d)->dev) : "", res ? res->name : "null", \
		(unsigned long long) (res ? resource_size(res) : 0), \
		(unsigned long long) (res ? res->start : 0), ##arg)

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
	struct ida ns_ida;
	struct device *ns_seed;
	u16 ndr_mappings;
	u64 ndr_size;
	u64 ndr_start;
	int id, num_lanes;
	void *provider_data;
	struct nd_interleave_set *nd_set;
	struct nd_mapping mapping[0];
};

struct nd_blk_region {
	int (*enable)(struct nvdimm_bus *nvdimm_bus, struct device *dev);
	void (*disable)(struct nvdimm_bus *nvdimm_bus, struct device *dev);
	int (*do_io)(struct nd_blk_region *ndbr, void *iobuf, u64 len,
			int write, resource_size_t dpa);
	void *blk_provider_data;
	struct nd_region nd_region;
};

/*
 * Lookup next in the repeating sequence of 01, 10, and 11.
 */
static inline unsigned nd_inc_seq(unsigned seq)
{
	static const unsigned next[] = { 0, 2, 3, 1 };

	return next[seq & 3];
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

static inline u64 nd_partition_offset(struct block_device *bdev)
{
	struct hd_struct *p;

	if (bdev == bdev->bd_contains)
		return 0;

	p = bdev->bd_part;
	return ((u64) p->start_sect) << SECTOR_SHIFT;
}

enum nd_async_mode {
	ND_SYNC,
	ND_ASYNC,
};

int nd_integrity_init(struct gendisk *disk, unsigned long meta_size);
void wait_nvdimm_bus_probe_idle(struct device *dev);
void nd_device_register(struct device *dev);
void nd_device_unregister(struct device *dev, enum nd_async_mode mode);
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
int __init nvdimm_init(void);
int __init nd_region_init(void);
void nvdimm_exit(void);
void nd_region_exit(void);
struct nvdimm;
struct nvdimm_drvdata *to_ndd(struct nd_mapping *nd_mapping);
int nvdimm_init_nsarea(struct nvdimm_drvdata *ndd);
int nvdimm_init_config_data(struct nvdimm_drvdata *ndd);
int nvdimm_set_config_data(struct nvdimm_drvdata *ndd, size_t offset,
		void *buf, size_t len);
struct nd_btt *to_nd_btt(struct device *dev);
struct btt_sb;
u64 nd_btt_sb_checksum(struct btt_sb *btt_sb);
struct nd_region *to_nd_region(struct device *dev);
int nd_region_to_namespace_type(struct nd_region *nd_region);
int nd_region_register_namespaces(struct nd_region *nd_region, int *err);
u64 nd_region_interleave_set_cookie(struct nd_region *nd_region);
void nvdimm_bus_lock(struct device *dev);
void nvdimm_bus_unlock(struct device *dev);
bool is_nvdimm_bus_locked(struct device *dev);
int nd_label_reserve_dpa(struct nvdimm_drvdata *ndd);
void nvdimm_free_dpa(struct nvdimm_drvdata *ndd, struct resource *res);
struct resource *nvdimm_allocate_dpa(struct nvdimm_drvdata *ndd,
		struct nd_label_id *label_id, resource_size_t start,
		resource_size_t n);
int nd_blk_region_init(struct nd_region *nd_region);
void nd_blk_queue_init(struct request_queue *q);
void __nd_iostat_start(struct bio *bio, unsigned long *start);
static inline bool nd_iostat_start(struct bio *bio, unsigned long *start)
{
	struct gendisk *disk = bio->bi_bdev->bd_disk;

	if (!blk_queue_io_stat(disk->queue))
		return false;

	__nd_iostat_start(bio, start);
	return true;
}
void nd_iostat_end(struct bio *bio, unsigned long start);
resource_size_t nd_namespace_blk_validate(struct nd_namespace_blk *nsblk);
#endif /* __ND_H__ */
