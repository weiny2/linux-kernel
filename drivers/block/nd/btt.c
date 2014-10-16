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

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/bio.h>
#include <linux/blkdev.h>
#include <linux/debugfs.h>
#include <linux/export.h>
#include <linux/fs.h>
#include <linux/hdreg.h>
#include <linux/idr.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include "btt.h"

/*
 * Lookup table and macro for looking up sequence numbers.  These are
 * the 2-bit numbers that cycle between 01, 10, and 11.
 *
 * To advance a sequence number to the next number, use something like:
 *	seq = NSEQ(seq);
 */
static const unsigned Nseq[] = { 0, 2, 3, 1 };
#define	NSEQ(seq) (Nseq[(seq) & 3])

enum log_ent_request {
	LOG_NEW_ENT = 0,
	LOG_OLD_ENT
};

static int btt_major;

static void __to_string(char *dst, u8 *src, u32 len)
{
	u32 i;
	char *dptr = dst;

	for (i = 0; i <= len; i++)
		dptr += sprintf(dptr, "%02x", src[i]);

	dst[2 * len] = 0;
}

/*
 * btt_sb_checksum: compute checksum for btt info block
 *
 * Returns a fletcher64 checksum of everything in the given info block
 * except the last field (since that's where the checksum lives).
 */

static u64 btt_sb_checksum(struct btt_sb *btt_sb)
{
	u32 *p32 = (u32 *)btt_sb;
	u32 *p32end = (u32 *)((void *)btt_sb + sizeof(struct btt_sb));
	u32 lo32 = 0;
	u32 hi32 = 0;

	while (p32 < p32end) {
		lo32 += *p32;
		hi32 += lo32;
		p32++;
	}

	return ((u64)hi32 << 32) | (u64)lo32;
}

static int btt_info_write(struct arena_info *arena, struct btt_sb *super)
{
	int ret;
	ret = arena->rw_bytes(arena->raw_bdev, super, sizeof(struct btt_sb),
			arena->info2off, WRITE);
	if (ret)
		return ret;

	return arena->rw_bytes(arena->raw_bdev, super, sizeof(struct btt_sb),
			arena->infooff, WRITE);
}

static int btt_info_read(struct arena_info *arena, struct btt_sb *super)
{
	WARN_ON(!super);
	return arena->rw_bytes(arena->raw_bdev, super, sizeof(struct btt_sb),
			arena->infooff, READ);

}

static int btt_map_write(struct arena_info *arena, u32 lba, u32 mapping)
{
	__le32 out = cpu_to_le32(mapping);
	WARN_ON(lba >= arena->external_nlba);
	return arena->rw_bytes(arena->raw_bdev, &out, MAP_ENT_SIZE,
			arena->mapoff + (lba * MAP_ENT_SIZE), WRITE);
}

static int btt_map_read(struct arena_info *arena, u32 lba, u32 *mapping,
			int *trim, int *error)
{
	int ret;
	u32 raw_mapping;
	__le32 in;

	WARN_ON(lba >= arena->external_nlba);

	ret = arena->rw_bytes(arena->raw_bdev, &in, MAP_ENT_SIZE,
			arena->mapoff + (lba * MAP_ENT_SIZE), READ);
	if (ret)
		return ret;

	raw_mapping = le32_to_cpu(in);

	*mapping = raw_mapping & MAP_LBA_MASK;
	if (trim)
		*trim = (raw_mapping >> MAP_TRIM_SHIFT) && MAP_TRIM_MASK;
	if (error)
		*error = (raw_mapping >> MAP_ERR_SHIFT) && MAP_ERR_MASK;

	return ret;
}

static int btt_log_read_pair(struct arena_info *arena, u32 lane,
			struct log_entry *ent)
{
	WARN_ON(!ent);
	return arena->rw_bytes(arena->raw_bdev, ent, 2 * LOG_ENT_SIZE,
			arena->logoff + (2 * lane * LOG_ENT_SIZE), READ);
}

static struct dentry *debugfs_root;

static void arena_debugfs_init(struct arena_info *a, struct dentry *parent,
				int idx)
{
	char dirname[32];
	struct dentry *d;

	/* If for some reason, parent bttN was not created, exit */
	if (!parent)
		return;

	snprintf(dirname, 32, "arena%d", idx);
	d = debugfs_create_dir(dirname, parent);
	if (IS_ERR_OR_NULL(d))
		return;
	a->debugfs_dir = d;

	debugfs_create_x64("size", S_IRUGO, d, &a->size);
	debugfs_create_x64("external_lba_start", S_IRUGO, d,
				&a->external_lba_start);
	debugfs_create_x32("internal_nlba", S_IRUGO, d, &a->internal_nlba);
	debugfs_create_u32("internal_lbasize", S_IRUGO, d,
				&a->internal_lbasize);
	debugfs_create_x32("external_nlba", S_IRUGO, d, &a->external_nlba);
	debugfs_create_u32("external_lbasize", S_IRUGO, d,
				&a->external_lbasize);
	debugfs_create_u32("nfree", S_IRUGO, d, &a->nfree);
	debugfs_create_u16("version_major", S_IRUGO, d, &a->version_major);
	debugfs_create_u16("version_minor", S_IRUGO, d, &a->version_minor);
	debugfs_create_x64("nextoff", S_IRUGO, d, &a->nextoff);
	debugfs_create_x64("infooff", S_IRUGO, d, &a->infooff);
	debugfs_create_x64("dataoff", S_IRUGO, d, &a->dataoff);
	debugfs_create_x64("mapoff", S_IRUGO, d, &a->mapoff);
	debugfs_create_x64("logoff", S_IRUGO, d, &a->logoff);
	debugfs_create_x64("info2off", S_IRUGO, d, &a->info2off);
	debugfs_create_x32("flags", S_IRUGO, d, &a->flags);
}

static void btt_debugfs_init(struct btt *btt)
{
	int i = 0;
	struct arena_info *arena;

	btt->debugfs_dir = debugfs_create_dir(btt->btt_disk->disk_name,
						debugfs_root);
	if (IS_ERR_OR_NULL(btt->debugfs_dir))
		return;

	list_for_each_entry(arena, &btt->arena_list, list) {
		arena_debugfs_init(arena, btt->debugfs_dir, i);
		i++;
	}
}

/*
 * This function accepts two log entries, and uses the
 * sequence number to find the 'older' entry.
 * It also updates the sequence number in this old entry to
 * make it the 'new' one if the mark_flag is set.
 * Finally, it returns which of the entries was the older one.
 *
 * TODO The logic feels a bit kludge-y. make it better..
 */
static int btt_log_get_old(struct log_entry *ent)
{
	int old;

	/*
	 * the first ever time this is seen, the entry goes into [0]
	 * the next time, the following logic works out to put this
	 * (next) entry into [1]
	 */
	if (ent[0].seq == 0) {
		ent[0].seq = 1;
		return 0;
	}

	if (ent[0].seq == ent[1].seq)
		return -EINVAL;
	if (ent[0].seq + ent[1].seq > 5)
		return -EINVAL;

	if (ent[0].seq < ent[1].seq) {
		if (ent[1].seq - ent[0].seq == 1)
			old = 0;
		else
			old = 1;
	} else {
		if (ent[0].seq - ent[1].seq == 1)
			old = 1;
		else
			old = 0;
	}

	return old;
}

/*
 * This function copies the desired (old/new) log entry into ent if
 * it is not NULL. It returns the sub-slot number (0 or 1)
 * where the desired log entry was found. Negative return values
 * indicate errors.
 */
static int btt_log_read(struct arena_info *arena, u32 lane,
			struct log_entry *ent, int old_flag)
{
	int ret;
	int old_ent, ret_ent;
	struct log_entry log[2];

	ret = btt_log_read_pair(arena, lane, log);
	if (ret)
		return -EIO;

	old_ent = btt_log_get_old(log);
	if (old_ent < 0 || old_ent > 1) {
		pr_info("log corruption (%d): lane %d seq [%d, %d]\n",
			old_ent, lane, log[0].seq, log[1].seq);
		/* TODO set error state? */
		return -EIO;
	}

	ret_ent = (old_flag ? old_ent : (1 - old_ent));

	if (ent != NULL)
		memcpy(ent, &log[ret_ent], LOG_ENT_SIZE);

	return ret_ent;
}

/*
 * This function commits a log entry to media
 * It does _not_ prepare the freelist entry for the next write
 * btt_flog_write is the wrapper for updating the freelist elements
 */
static int __btt_log_write(struct arena_info *arena, u32 lane,
			u32 sub, struct log_entry *ent)
{
	int ret;
	unsigned int log_half = LOG_ENT_SIZE / 2;
	u64 ns_off = arena->logoff + (((2 * lane) + sub) * LOG_ENT_SIZE);
	void *src = ent;

	/* split the 16B write into atomic, durable halves */
	ret = arena->rw_bytes(arena->raw_bdev, src, log_half, ns_off, WRITE);
	if (ret)
		return ret;

	ns_off += log_half;
	src += log_half;
	return arena->rw_bytes(arena->raw_bdev, src, log_half, ns_off, WRITE);
}

static int btt_flog_write(struct arena_info *arena, u32 lane, u32 sub,
			struct log_entry *ent)
{
	int ret;

	ret = __btt_log_write(arena, lane, sub, ent);
	if (ret)
		return ret;

	/* prepare the next free entry */
	arena->freelist[lane].sub = 1 - arena->freelist[lane].sub;
	if (++(arena->freelist[lane].seq) == 4)
		arena->freelist[lane].seq = 1;
	arena->freelist[lane].block = ent->old_map;

	return ret;
}

/*
 * This function initializes the BTT map to a state with all externally
 * exposed blocks having an identity mapping, and the TRIM flag set
 */
static int btt_map_init(struct arena_info *arena)
{
	int ret;
	size_t i;

	/* XXX rewrite to not call _map_write for each entry */
	for (i = 0; i < arena->external_nlba; i++) {
		ret = btt_map_write(arena, i, (i | (1 << MAP_TRIM_SHIFT)));
		if (ret)
			return ret;
	}

	return 0;
}

/*
 * This function initializes the BTT log with 'fake' entries pointing
 * to the initial reserved set of blocks as being free
 */
static int btt_log_init(struct arena_info *arena)
{
	int ret;
	u32 i;
	struct log_entry log, zerolog;

	memset(&zerolog, 0, sizeof(zerolog));

	for (i = 0; i < arena->nfree; i++) {
		log.lba = i;
		log.old_map = arena->external_nlba + i;
		log.new_map = arena->external_nlba + i;
		log.seq = LOG_SEQ_INIT;
		ret = __btt_log_write(arena, i, 0, &log);
		if (ret)
			return ret;
		ret = __btt_log_write(arena, i, 1, &zerolog);
		if (ret)
			return ret;
	}

	return 0;
}

static int btt_freelist_init(struct arena_info *arena)
{
	int old, new;
	u32 i;
	struct log_entry log_new, log_old;

	arena->freelist = kzalloc(arena->nfree * sizeof(struct free_entry),
					GFP_KERNEL);
	if (!arena->freelist)
		return -ENOMEM;

	for (i = 0; i < arena->nfree; i++) {
		old = btt_log_read(arena, i, &log_old, LOG_OLD_ENT);
		if (old < 0)
			return old;

		new = btt_log_read(arena, i, &log_new, LOG_NEW_ENT);
		if (new < 0)
			return new;

		arena->freelist[i].block = log_new.old_map;
		/* sub points to the next one to be overwritten */
		arena->freelist[i].sub = 1 - new;
		arena->freelist[i].seq = NSEQ(log_new.seq);
	}

	return 0;
}

static int btt_rtt_init(struct arena_info *arena)
{
	arena->rtt = kzalloc(arena->nfree * sizeof(u32), GFP_KERNEL);
	if (arena->rtt == NULL)
		return -ENOMEM;

	return 0;
}

static int btt_maplocks_init(struct arena_info *arena)
{
	u32 i;

	arena->map_lock = kmalloc(arena->nfree * sizeof(spinlock_t),
				GFP_KERNEL);
	if (!arena->map_lock)
		return -ENOMEM;

	for (i = 0; i < arena->nfree; i++)
		spin_lock_init(&arena->map_lock[i]);

	return 0;
}

static struct arena_info *alloc_arena(struct btt *btt, size_t size,
				size_t start, size_t arena_off)
{
	struct arena_info *arena;
	u64 logsize, mapsize, datasize;
	u64 available = size;

	arena = kzalloc(sizeof(struct arena_info), GFP_KERNEL);
	if (!arena)
		return NULL;

	arena->raw_bdev = btt->raw_bdev;
	arena->rw_bytes = btt->raw_bdev->bd_disk->fops->rw_bytes;

	if (!size)
		return arena;

	arena->size = size;
	arena->external_lba_start = start;
	arena->external_lbasize = btt->lbasize;
	arena->internal_lbasize = roundup(arena->external_lbasize,
					INT_LBASIZE_ALIGNMENT);
	arena->nfree = BTT_DEFAULT_NFREE;
	arena->version_major = 1;
	arena->version_minor = 1;

	if (available % BTT_PG_SIZE)
		available -= (available % BTT_PG_SIZE);

	/* Two pages are reserved for the super block and its copy */
	available -= 2 * BTT_PG_SIZE;

	/* The log takes a fixed amount of space based on nfree */
	logsize = roundup(2 * arena->nfree * sizeof(struct log_entry),
				BTT_PG_SIZE);
	available -= logsize;

	/* Calculate optimal split between map and data area */
	arena->internal_nlba = (available - BTT_PG_SIZE) /
			(arena->internal_lbasize + MAP_ENT_SIZE);
	arena->external_nlba = arena->internal_nlba - arena->nfree;

	mapsize = roundup((arena->external_nlba * MAP_ENT_SIZE), BTT_PG_SIZE);
	datasize = available - mapsize;

	/* 'Absolute' values, relative to start of storage space */
	arena->infooff = arena_off;
	arena->dataoff = arena->infooff + BTT_PG_SIZE;
	arena->mapoff = arena->dataoff + datasize;
	arena->logoff = arena->mapoff + mapsize;
	arena->info2off = arena->logoff + logsize;
	return arena;
}

static void free_arenas(struct btt *btt)
{
	struct arena_info *arena, *next;

	list_for_each_entry_safe(arena, next, &btt->arena_list, list) {
		list_del(&arena->list);
		kfree(arena->rtt);
		kfree(arena->map_lock);
		kfree(arena->freelist);
		debugfs_remove_recursive(arena->debugfs_dir);
		kfree(arena);
	}
}

/*
 * This function checks if the metadata layout is valid and error free
 */
static int arena_is_valid(struct arena_info *arena, struct btt_sb *super,
				u8 *uuid)
{
	u64 checksum;

	if (memcmp(super->uuid, uuid, 16))
		return 0;

	checksum = le64_to_cpu(super->checksum);
	super->checksum = 0;
	if (checksum != btt_sb_checksum(super))
		return 0;
	super->checksum = cpu_to_le64(checksum);

	/* TODO: figure out action for this */
	if ((le32_to_cpu(super->flags) & IB_FLAG_ERROR_MASK) != 0)
		pr_info("Found arena with an error flag\n");

	return 1;
}

/*
 * This function reads an existing valid btt superblock and
 * populates the corresponding arena_info struct
 */
static void parse_arena_meta(struct arena_info *arena, struct btt_sb *super,
				u64 arena_off)
{
	arena->internal_nlba = le32_to_cpu(super->internal_nlba);
	arena->internal_lbasize = le32_to_cpu(super->internal_lbasize);
	arena->external_nlba = le32_to_cpu(super->external_nlba);
	arena->external_lbasize = le32_to_cpu(super->external_lbasize);
	arena->nfree = le32_to_cpu(super->nfree);
	arena->version_major = le16_to_cpu(super->version_major);
	arena->version_minor = le16_to_cpu(super->version_minor);

	arena->nextoff = (super->nextoff == 0) ? 0 : (arena_off +
			le64_to_cpu(super->nextoff));
	arena->infooff = arena_off;
	arena->dataoff = arena_off + le64_to_cpu(super->dataoff);
	arena->mapoff = arena_off + le64_to_cpu(super->mapoff);
	arena->logoff = arena_off + le64_to_cpu(super->logoff);
	arena->info2off = arena_off + le64_to_cpu(super->info2off);

	arena->size = (super->nextoff > 0) ? (le64_to_cpu(super->nextoff)) :
			(arena->info2off - arena->infooff + BTT_PG_SIZE);

	arena->flags = le32_to_cpu(super->flags);
}

static int discover_arenas(struct btt *btt)
{
	int ret = 0;
	struct btt_sb *super;
	size_t remaining = btt->rawsize;
	u64 cur_nlba = 0;
	size_t cur_off = 0;
	int num_arenas = 0;

	super = kzalloc(sizeof(*super), GFP_KERNEL);
	if (!super)
		return -ENOMEM;

	while (remaining) {
		struct arena_info *arena;

		/* Alloc memory for arena */
		arena = alloc_arena(btt, 0, 0, 0);
		if (!arena)
			return -ENOMEM;

		arena->infooff = cur_off;
		ret = btt_info_read(arena, super);
		if (ret)
			goto out;

		if (!arena_is_valid(arena, super, btt->uuid)) {
			if (remaining == btt->rawsize) {
				btt->init_state = INIT_NOTFOUND;
				pr_info("No existing arenas\n");
				kfree(arena);
				goto out;
			} else {
				pr_err("Found corrupted metadata!\n");
				goto out;
			}
		}

		arena->external_lba_start = cur_nlba;
		parse_arena_meta(arena, super, cur_off);

		ret = btt_freelist_init(arena);
		if (ret)
			goto out;

		ret = btt_rtt_init(arena);
		if (ret)
			goto out;

		ret = btt_maplocks_init(arena);
		if (ret)
			goto out;

		list_add_tail(&arena->list, &btt->arena_list);

		remaining -= arena->size;
		cur_off += arena->size;
		cur_nlba += arena->external_nlba;
		num_arenas++;

		if (arena->nextoff == 0)
			break;
	}
	btt->num_arenas = num_arenas;
	btt->nlba = cur_nlba;
	btt->init_state = INIT_READY;

	kfree(super);
	return ret;

out:
	free_arenas(btt);
	kfree(super);
	return ret;
}

static int create_arenas(struct btt *btt)
{
	size_t remaining = btt->rawsize;
	size_t cur_off = 0;

	while (remaining) {
		struct arena_info *arena;
		size_t arena_size = min(ARENA_MAX_SIZE, remaining);

		remaining -= arena_size;
		if (arena_size < ARENA_MIN_SIZE)
			break;

		arena = alloc_arena(btt, arena_size, btt->nlba, cur_off);
		if (!arena) {
			free_arenas(btt);
			return -ENOMEM;
		}
		btt->nlba += arena->external_nlba;
		if (remaining >= ARENA_MIN_SIZE)
			arena->nextoff = arena->size;
		else
			arena->nextoff = 0;
		cur_off += arena_size;
		list_add_tail(&arena->list, &btt->arena_list);
	}

	return 0;
}

/*
 * This function completes arena initialization by writing
 * all the metadata.
 * It is only called for an uninitialized arena when a write
 * to that arena occurs for the first time.
 */
static int btt_arena_write_layout(struct arena_info *arena, u8 *uuid)
{
	int ret;
	struct btt_sb *super;

	ret = btt_map_init(arena);
	if (ret)
		return ret;

	ret = btt_log_init(arena);
	if (ret)
		return ret;

	super = kzalloc(sizeof(struct btt_sb), GFP_NOIO);
	if (!super)
		return -ENOMEM;

	strncpy(super->signature, BTT_SIG, BTT_SIG_LEN);
	memcpy(super->uuid, uuid, 16);
	super->flags = cpu_to_le32(arena->flags);
	super->version_major = cpu_to_le16(arena->version_major);
	super->version_minor = cpu_to_le16(arena->version_minor);
	super->external_lbasize = cpu_to_le32(arena->external_lbasize);
	super->external_nlba = cpu_to_le32(arena->external_nlba);
	super->internal_lbasize = cpu_to_le32(arena->internal_lbasize);
	super->internal_nlba = cpu_to_le32(arena->internal_nlba);
	super->nfree = cpu_to_le32(arena->nfree);
	super->infosize = cpu_to_le32(sizeof(struct btt_sb));

	/* TODO: make these relative to arena start. For now we get this
	 * since each file = 1 arena = 1 dimm, but will change */
	super->nextoff = cpu_to_le64(arena->nextoff);
	/*
	 * Subtract arena->infooff (arena start) so numbers are relative
	 * to 'this' arena
	 */
	super->dataoff = cpu_to_le64(arena->dataoff - arena->infooff);
	super->mapoff = cpu_to_le64(arena->mapoff - arena->infooff);
	super->logoff = cpu_to_le64(arena->logoff - arena->infooff);
	super->info2off = cpu_to_le64(arena->info2off - arena->infooff);

	super->flags = 0;
	super->checksum = cpu_to_le64(btt_sb_checksum(super));

	ret = btt_info_write(arena, super);

	kfree(super);
	return ret;
}

/*
 * This function completes the initialization for the BTT namespace
 * such that it is ready to accept IOs
 */
static int btt_meta_init(struct btt *btt)
{
	int ret = 0;
	struct arena_info *arena;

	mutex_lock(&btt->init_lock);
	if (btt->init_state == INIT_READY)
		goto unlock;


	list_for_each_entry(arena, &btt->arena_list, list) {
		ret = btt_arena_write_layout(arena, btt->uuid);
		if (ret)
			goto unlock;

		ret = btt_freelist_init(arena);
		if (ret)
			goto unlock;

		ret = btt_rtt_init(arena);
		if (ret)
			goto unlock;

		ret = btt_maplocks_init(arena);
		if (ret)
			goto unlock;
	}

	btt->init_state = INIT_READY;

 unlock:
	mutex_unlock(&btt->init_lock);
	return ret;
}

static int check_init_meta_state(struct btt *btt)
{
	WARN_ON(btt->init_state == INIT_UNCHECKED);
	if (likely(btt->init_state == INIT_READY))
		return 0;

	return btt_meta_init(btt);
}

/*
 * This function calculates the arena in which the given LBA lies
 * by doing a linear walk. This is acceptable since we expect only
 * a few arenas. If we have backing devices that get much larger,
 * we can construct a balanced binary tree of arenas at init time
 * so that this range search becomes faster.
 */
static int lba_to_arena(struct btt *btt, __u64 lba, __u32 *premap,
				struct arena_info **arena)
{
	struct arena_info *arena_list;

	list_for_each_entry(arena_list, &btt->arena_list, list) {
		if (lba < arena_list->external_nlba) {
			*arena = arena_list;
			*premap = lba;
			return 0;
		}
		lba -= arena_list->external_nlba;
	}

	return -EIO;
}

/*
 * This function acquires a lane, and will block till one is available.
 */
static void acquire_lane(struct btt *btt, u32 *lane)
{
	*lane = atomic_inc_return(&(btt->last_lane)) % btt->num_lanes;
	spin_lock(&(btt->lane_lock[*lane]));
}

static void release_lane(struct btt *btt, u32 lane)
{
	spin_unlock(&(btt->lane_lock[lane]));
}

/*
 * The following (lock_map, unlock_map) are mostly just to improve
 * readability, since they index into an array of locks
 */
static void lock_map(struct arena_info *arena, u32 idx)
{
	spin_lock(&arena->map_lock[idx]);
}

static void unlock_map(struct arena_info *arena, u32 idx)
{
	spin_unlock(&arena->map_lock[idx]);
}

static int btt_data_read(struct arena_info *arena, struct page *page,
			unsigned int off, sector_t sector, u32 len)
{
	int ret;
	u64 nsoff = arena->dataoff + (sector * arena->internal_lbasize);
	void *mem = kmap_atomic(page);

	ret = arena->rw_bytes(arena->raw_bdev, mem + off, len, nsoff, READ);
	kunmap_atomic(mem);

	return ret;
}

static int btt_data_write(struct arena_info *arena, sector_t sector,
			struct page *page, unsigned int off, u32 len)
{
	int ret;
	u64 nsoff = arena->dataoff + (sector * arena->internal_lbasize);
	void *mem = kmap_atomic(page);

	ret = arena->rw_bytes(arena->raw_bdev, mem + off, len, nsoff, WRITE);
	kunmap_atomic(mem);

	return ret;
}

static void zero_fill_data(struct page *page, unsigned int off, u32 len)
{
	void *mem = kmap_atomic(page);

	memset(mem + off, 0, len);
	kunmap_atomic(mem);
}

static int btt_read_pg(struct btt *btt, struct page *page, unsigned int off,
			sector_t sector, unsigned int len)
{
	int ret = 0;
	int t_flag, e_flag;
	struct arena_info *arena = NULL;
	u32 lane = 0, premap, postmap;

	while (len) {
		u32 cur_len;

		acquire_lane(btt, &lane);

		ret = lba_to_arena(btt, sector, &premap, &arena);
		if (ret)
			goto out_lane;
		cur_len = min(arena->external_lbasize, len);

		ret = btt_map_read(arena, premap, &postmap, &t_flag, &e_flag);
		if (ret)
			goto out_lane;

		/*
		 * We loop to make sure that the post map LBA didn't change
		 * from under us between writing the RTT and doing the actual
		 * read.
		 */
		while (1) {
			u32 new_map;

			if (t_flag) {
				zero_fill_data(page, off, cur_len);
				goto out_lane;
			}

			if (e_flag) {
				ret = -EIO;
				goto out_lane;
			}

			arena->rtt[lane] = RTT_VALID | postmap;

			ret = btt_map_read(arena, premap, &new_map, &t_flag,
						&e_flag);
			if (ret)
				goto out_rtt;

			if (postmap == new_map)
				break;
			else
				postmap = new_map;
		}

		ret = btt_data_read(arena, page, off, postmap, cur_len);
		if (ret)
			goto out_rtt;

		arena->rtt[lane] = RTT_INVALID;
		release_lane(btt, lane);

		len -= cur_len;
		off += cur_len;
		sector++;
	}

	return 0;

 out_rtt:
	arena->rtt[lane] = RTT_INVALID;
 out_lane:
	release_lane(btt, lane);
	return ret;
}

static int btt_write_pg(struct btt *btt, sector_t sector, struct page *page,
		unsigned int off, unsigned int len)
{
	int ret = 0;
	struct arena_info *arena = NULL;
	u32 premap = 0, old_postmap, new_postmap, lane = 0, i;
	struct log_entry log;
	int sub;

	while (len) {
		u32 cur_len;

		acquire_lane(btt, &lane);

		ret = lba_to_arena(btt, sector, &premap, &arena);
		if (ret)
			goto out_lane;
		cur_len = min(arena->external_lbasize, len);

		if ((arena->flags & IB_FLAG_ERROR_MASK) != 0) {
			ret = -EIO;
			goto out_lane;
		}

		new_postmap = arena->freelist[lane].block;

		/* Wait if the new block is being read from */
		for (i = 0; i < arena->nfree; i++)
			while (arena->rtt[i] == (RTT_VALID | new_postmap))
				;

		if (new_postmap >= arena->internal_nlba) {
			ret = -EIO;
			goto out_lane;
		} else
			ret = btt_data_write(arena, new_postmap, page,
						off, cur_len);
		if (ret)
			goto out_lane;

		lock_map(arena, premap % arena->nfree);
		ret = btt_map_read(arena, premap, &old_postmap, NULL, NULL);
		if (ret)
			goto out_map;
		if (old_postmap >= arena->internal_nlba) {
			ret = -EIO;
			goto out_map;
		}

		log.lba = premap;
		log.old_map = old_postmap;
		log.new_map = new_postmap;
		log.seq = arena->freelist[lane].seq;
		sub = arena->freelist[lane].sub;
		ret = btt_flog_write(arena, lane, sub, &log);
		if (ret)
			goto out_map;

		ret = btt_map_write(arena, premap, new_postmap);
		if (ret)
			goto out_map;

		unlock_map(arena, premap % arena->nfree);
		release_lane(btt, lane);

		len -= cur_len;
		off += cur_len;
		sector++;
	}

	return 0;

 out_map:
	unlock_map(arena, premap % arena->nfree);
 out_lane:
	release_lane(btt, lane);
	return ret;
}

static int btt_do_bvec(struct btt *btt, struct page *page,
			unsigned int len, unsigned int off, int rw,
			sector_t sector)
{
	int ret;

	if (rw == READ) {
		ret = btt_read_pg(btt, page, off, sector, len);
		flush_dcache_page(page);
	} else {
		flush_dcache_page(page);
		ret = btt_write_pg(btt, sector, page, off, len);
	}

	return ret;
}

static void btt_make_request(struct request_queue *q, struct bio *bio)
{
	struct block_device *bdev = bio->bi_bdev;
	struct btt *btt = q->queuedata;
	int rw;
	struct bio_vec bvec;
	sector_t sector;
	struct bvec_iter iter;
	int err = 0;

	sector = bio->bi_iter.bi_sector;
	if (bio_end_sector(bio) > get_capacity(bdev->bd_disk)) {
		err = -EIO;
		goto out;
	}

	BUG_ON(bio->bi_rw & REQ_DISCARD);

	rw = bio_rw(bio);
	if (rw == READA)
		rw = READ;

	if (rw == READ && btt->init_state == INIT_NOTFOUND) {
		zero_fill_bio(bio);
		goto out;
	}

	if (rw == WRITE && btt->init_state != INIT_READY) {
		err = check_init_meta_state(btt);
		if (err)
			goto out;
	}

	bio_for_each_segment(bvec, bio, iter) {
		unsigned int len = bvec.bv_len;
		BUG_ON(len > PAGE_SIZE);
		/* Make sure len is in multiples of lbasize. */
		/* XXX is this right? */
		BUG_ON(len < btt->lbasize);
		BUG_ON(len % btt->lbasize);

		err = btt_do_bvec(btt, bvec.bv_page, len, bvec.bv_offset,
				rw, sector);
		if (err) {
			pr_info("io error in %s sector %ld, len %d,\n",
				(rw == READ) ? "READ" : "WRITE", sector, len);
			goto out;
		}
		sector += len >> SECTOR_SHIFT;
	}

out:
	bio_endio(bio, err);
}

static int lane_locks_init(struct btt *btt)
{
	u32 i;

	btt->lane_lock = kmalloc(btt->num_lanes * sizeof(spinlock_t),
				GFP_KERNEL);
	if (!btt->lane_lock)
		return -ENOMEM;

	for (i = 0; i < btt->num_lanes; i++)
		spin_lock_init(&btt->lane_lock[i]);

	return 0;
}

static int btt_getgeo(struct block_device *bd, struct hd_geometry *geo)
{
	/* some standard values */
	geo->heads = 1 << 6;
	geo->sectors = 1 << 5;
	geo->cylinders = get_capacity(bd->bd_disk) >> 11;
	return 0;
}

static int btt_rw_page(struct block_device *bdev, sector_t sector,
		       struct page *page, int rw)
{
	struct btt *btt = bdev->bd_disk->private_data;

	btt_do_bvec(btt, page, PAGE_CACHE_SIZE, 0, rw, sector);
	page_endio(page, rw & WRITE, 0);
	return 0;
}

static const struct block_device_operations btt_fops = {
	.owner =		THIS_MODULE,
	.rw_page =		btt_rw_page,
	.getgeo =		btt_getgeo,
};

static DEFINE_IDA(btt_ida);

static int btt_blk_init(struct btt *btt)
{
	int ret;

	/* create a new disk and request queue for btt */
	btt->btt_queue = blk_alloc_queue(GFP_KERNEL);
	if (!btt->btt_queue)
		return -ENOMEM;

	btt->btt_disk = alloc_disk(0);
	if (!btt->btt_disk) {
		ret = -ENOMEM;
		goto out_free_queue;
	}

	btt->disk_id = ida_simple_get(&btt_ida, 0, 0, GFP_KERNEL);
	sprintf(btt->btt_disk->disk_name, "btt%d", btt->disk_id);
	btt->btt_disk->major = btt_major;
	btt->btt_disk->first_minor = 0;
	btt->btt_disk->fops = &btt_fops;
	btt->btt_disk->private_data = btt;
	btt->btt_disk->queue = btt->btt_queue;
	btt->btt_disk->flags = GENHD_FL_EXT_DEVT;

	blk_queue_make_request(btt->btt_queue, btt_make_request);
	blk_queue_max_hw_sectors(btt->btt_queue, 1024);
	blk_queue_bounce_limit(btt->btt_queue, BLK_BOUNCE_ANY);
	blk_queue_physical_block_size(btt->btt_queue, btt->lbasize);
	blk_queue_logical_block_size(btt->btt_queue, btt->lbasize);
	btt->btt_queue->queuedata = btt;

	set_capacity(btt->btt_disk, btt->nlba);
	add_disk(btt->btt_disk);

	return 0;

out_free_queue:
	blk_cleanup_queue(btt->btt_queue);
	return ret;
}

static void btt_blk_cleanup(struct btt *btt)
{
	del_gendisk(btt->btt_disk);
	put_disk(btt->btt_disk);
	blk_cleanup_queue(btt->btt_queue);
	blkdev_put(btt->raw_bdev, FMODE_EXCL);
	ida_simple_remove(&btt_ida, btt->disk_id);
}

/**
 * btt_init - initialize a block translation table for the given device
 * @bdev:	block_device to initialize the BTT on
 * @rawsize:	raw size in bytes of the backing device
 * @lbasize:	lba size of the backing device
 * @uuid:	A uuid for the backing device - this is stored on media
 * @maxlane:	maximum number of parallel requests the device can handle
 *
 * Initialize a Block Translation Table on a backing device to provide
 * single sector power fail atomicity.
 *
 * Context:
 * Might sleep.
 *
 * Returns:
 * Pointer to a new struct btt on success, NULL on failure.
 */
struct btt *btt_init(struct block_device *bdev, size_t rawsize, u32 lbasize,
			u8 *uuid, int maxlane)
{
	int ret;
	struct btt *btt;
	char uuid_str[33];

	__to_string(uuid_str, uuid, 16);

	btt = kzalloc(sizeof(struct btt), GFP_KERNEL);
	if (!btt)
		return NULL;

	BUG_ON(!bdev->bd_disk->fops->rw_bytes);

	btt->raw_bdev = bdev;
	btt->rawsize = rawsize;
	btt->lbasize = lbasize;
	btt->num_lanes = maxlane;
	memcpy(btt->uuid, uuid, 16);
	INIT_LIST_HEAD(&btt->arena_list);
	mutex_init(&btt->init_lock);

	ret = discover_arenas(btt);
	if (ret) {
		pr_info("init: error in arena_discover: %d\n", ret);
		return NULL;
	}

	if (btt->init_state != INIT_READY) {
		btt->num_arenas = (rawsize / ARENA_MAX_SIZE) +
			((rawsize % ARENA_MAX_SIZE) ? 1 : 0);
		pr_info("init uuid %s: %d arenas for %lu rawsize\n",
				uuid_str, btt->num_arenas, rawsize);

		ret = create_arenas(btt);
		if (ret) {
			pr_info("init: create_arenas: %d\n", ret);
			return NULL;
		}
	}

	ret = lane_locks_init(btt);
	if (ret) {
		pr_info("init: lane_locks_init: %d\n", ret);
		return NULL;
	}

	ret = btt_blk_init(btt);
	if (ret) {
		pr_info("init: error in blk_init: %d\n", ret);
		return NULL;
	}

	btt_debugfs_init(btt);

	return btt;
}
EXPORT_SYMBOL(btt_init);

/**
 * btt_fini - de-initialize a BTT
 * @btt:	the BTT handle that was generated by btt_init
 *
 * De-initialize a Block Translation Table on device removal
 *
 * Context:
 * Might sleep.
 */
void btt_fini(struct btt *btt)
{
	if (btt) {
		btt_blk_cleanup(btt);
		kfree(btt->lane_lock);
		free_arenas(btt);
		debugfs_remove_recursive(btt->debugfs_dir);
		kfree(btt);
	}
}
EXPORT_SYMBOL(btt_fini);

/* Kernel module stuff */
static int major = 259;
module_param(major, int, S_IRUGO);
MODULE_PARM_DESC(major, "Major number of the backing device");

static int minor;
module_param(minor, int, S_IRUGO);
MODULE_PARM_DESC(minor, "Minor number of the backing device");

static struct btt *btt_handle;

static int __init btt_module_init(void)
{
	int ret;
	struct block_device *bdev;
	u8 uuid[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
	size_t rawsize;

	ret = register_blkdev(0, "btt");
	if (ret < 0)
		return -EIO;
	else
		btt_major = ret;

	/* XXX this probably moves into btt_blk_init later */
	bdev = blkdev_get_by_dev(MKDEV(major, minor),
			FMODE_EXCL, KBUILD_MODNAME);
	if (IS_ERR(bdev)) {
		ret = PTR_ERR(bdev);
		pr_err("failed to open backing device : %d\n", ret);
		return ret;
	}

	debugfs_root = debugfs_create_dir("btt", NULL);
	if (IS_ERR_OR_NULL(debugfs_root))
		goto release;

	rawsize = get_capacity(bdev->bd_disk) << SECTOR_SHIFT;
	btt_handle = btt_init(bdev, rawsize, 512, uuid, 256);
	if (!btt_handle) {
		ret = -ENOMEM;
		goto release;
	}

	return 0;

release:
	blkdev_put(bdev, FMODE_EXCL);
	return ret;
}

static void __exit btt_module_exit(void)
{
	unregister_blkdev(btt_major, "btt");
	btt_fini(btt_handle);
	debugfs_remove_recursive(debugfs_root);
}

MODULE_AUTHOR("Vishal Verma <vishal.l.verma@linux.intel.com>");
MODULE_LICENSE("GPL v2");
module_init(btt_module_init);
module_exit(btt_module_exit);
