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
#define MAP_ENT_SIZE 4
#define MAP_TRIM_SHIFT 31
#define MAP_TRIM_MASK 0x1
#define MAP_ERR_SHIFT 30
#define MAP_ERR_MASK 0x1
#define MAP_LBA_MASK (~((1 << MAP_TRIM_SHIFT) | (1 << MAP_ERR_SHIFT)))
#define LOG_ENT_SIZE sizeof(struct log_entry)
#define ARENA_MIN_SIZE ((size_t)(1 << 24))	/* 16 MB */
#define ARENA_MAX_SIZE ((size_t)(1ULL << 39))	/* 512 GB */
#define RTT_VALID (1U << 31)
#define RTT_INVALID 0
#define INT_LBASIZE_ALIGNMENT 256
#define BTT_PG_SIZE 4096
#define BTT_DEFAULT_NFREE 256
#define LOG_SEQ_INIT 1

#define IB_FLAG_ERROR 0x00000001
#define IB_FLAG_ERROR_MASK 0x00000001

#define SECTOR_SHIFT		9

enum btt_init_state {
	INIT_UNCHECKED = 0,
	INIT_NOTFOUND,
	INIT_READY
};

struct log_entry {
	__le32 lba;
	__le32 old_map;
	__le32 new_map;
	__le32 seq;
};

struct btt_sb {
	u8 signature[BTT_SIG_LEN];
	u8 uuid[16];
	__le32 flags;
	__le16 version_major;
	__le16 version_minor;
	__le32 external_lbasize;
	__le32 external_nlba;
	__le32 internal_lbasize;
	__le32 internal_nlba;
	__le32 nfree;
	__le32 infosize;
	__le64 nextoff;
	__le64 dataoff;
	__le64 mapoff;
	__le64 logoff;
	__le64 info2off;
	u8 padding[3984];
	__le64 checksum;
};

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
	struct nd_btt *nd_btt;
	struct list_head list;
	struct dentry *debugfs_dir;
	/* Arena flags */
	u32 flags;
};

struct btt {
	struct gendisk *btt_disk;
	struct request_queue *btt_queue;
	struct list_head arena_list;
	struct dentry *debugfs_dir;
	struct block_device *backing_dev;
	struct nd_btt *nd_btt;
	size_t nlba;
	size_t rawsize;
	u32 lbasize;
	spinlock_t *lane_lock;		/* Array of lane locks */
	struct mutex init_lock;
	atomic_t last_lane;
	int init_state;
	int num_arenas;
	int num_lanes;
};
#endif
