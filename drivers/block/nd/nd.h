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

struct gendisk;
struct nfit_mem;
struct nfit_dcr;
struct nd_dimm {
	struct nfit_mem __iomem *nfit_mem;
	struct nfit_dcr __iomem *nfit_dcr;
	struct device dev;
	int config_size;
	int id;
	struct nfit_cmd_get_config_data_hdr *data;
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
 * @num_lanes: number of simultaneous transactions in flight
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
	int num_lanes;
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

int nd_device_register(struct device *dev, enum nd_async_mode mode);
void nd_device_unregister(struct device *dev, enum nd_async_mode mode);
int nd_register_ndio(struct nd_io *ndio);
void nd_init_ndio(struct nd_io *ndio, nd_rw_bytes_fn rw_bytes,
		struct device *dev, struct gendisk *disk, int num_lanes,
		unsigned long align);
void ndio_del_claim(struct nd_io_claim *ndio_claim);
struct nd_io_claim *ndio_add_claim(struct nd_io *ndio, struct device *holder,
		ndio_notify_remove_fn notify_remove);
int nd_unregister_ndio(struct nd_io *ndio);
extern struct attribute_group nd_device_attribute_group;
struct nd_region *to_nd_region(struct device *dev);
struct nd_dimm *to_nd_dimm(struct device *dev);
struct nd_btt *to_nd_btt(struct device *dev);
struct nd_region *to_nd_region(struct device *dev);
int nd_dimm_get_config_size(struct nd_dimm *nd_dimm,
		struct nfit_cmd_get_config_size *cmd);
int nd_dimm_get_config_data(struct nd_dimm *nd_dimm,
		struct nfit_cmd_get_config_data_hdr *cmd, size_t len);
int nd_region_to_namespace_type(struct nd_region *nd_region);
void nd_bus_lock(struct device *dev);
void nd_bus_unlock(struct device *dev);
bool is_nd_bus_locked(struct device *dev);
#endif /* __ND_H__ */
