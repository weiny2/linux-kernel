/*
 * Block Translation Table library
 * Copyright (c) 2014, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#ifndef _LINUX_BTT_H
#define _LINUX_BTT_H

#include <linux/types.h>

#define BTT_SIG_LEN 16
#define BTT_SIG "BTT_ARENA_INFO\0"

struct free_entry {
	u32 block;
	u8 sub;
	u8 seq;
};

struct arena_info {
	u64 size;			/* Total bytes for this arena */
	u64 external_lba_start;
	u32 internal_nlba;
	u32 internal_lbasize;
	u32 external_nlba;
	u32 external_lbasize;
	u32 nfree;
	u16 version_major;
	u16 version_minor;
	/* Byte offsets to the different on-media structures */
	u64 nextoff;
	u64 infooff;
	u64 dataoff;
	u64 mapoff;
	u64 logoff;
	u64 info2off;
	/* Pointers to other in-memory structures for this arena */
	struct free_entry *freelist;
	u32 *rtt;			/* Read Tracking Table */
	spinlock_t *map_lock;		/* Array of map locks */
	struct list_head list;
	struct dentry *debugfs_dir;
	/* Arena flags */
	u32 flags;
	struct block_device *raw_bdev;
	int (*rw_bytes)(struct block_device *, void *, size_t, off_t, int rw);
};

struct btt {
	struct gendisk *btt_disk;
	struct block_device *raw_bdev;
	struct request_queue *btt_queue;
	struct list_head arena_list;
	struct dentry *debugfs_dir;
	u8 uuid[16];
	size_t nlba;
	size_t rawsize;
	u32 lbasize;
	spinlock_t *lane_lock;		/* Array of lane locks */
	spinlock_t init_lock;
	atomic_t last_lane;
	int init_state;
	int num_arenas;
	int num_lanes;
	int disk_id;
};

struct btt *btt_init(struct block_device *bdev, size_t rawsize, u32 lbasize,
			u8 *uuid, int maxlane);
void btt_fini(struct btt *btt);

#endif
