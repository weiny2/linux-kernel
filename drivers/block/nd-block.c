/*
 * NVDIMM Block Window Driver
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

#include <linux/blkdev.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

#define SECTOR_SHIFT		9
#define CL_SHIFT		6

enum {
	/* FIXME: should be (1 << 48)-1, once Simics is updated to match NFIT 0.8s2 */
	BCW_OFFSET_MASK		= (1UL << 37)-1,
	/* FIXME: should be 48, once Simics is updated to match NFIT 0.8s2 */
	BCW_LEN_SHIFT		= 37,
	/* FIXME: already correct with NFIT 0.8s */
	BCW_LEN_MASK		= (1UL << 8) - 1,
	/* FIXME: should be 56 once Simics is updated to match NFIT 0.8s2 */
	BCW_CMD_SHIFT		= 45,
};


// FIXME: move this stuff into the ndb_device
phys_addr_t	bw_apt_phys 	= 0xf008000000; 	// FIXME: hard coded to match simics
phys_addr_t	bw_ctl_phys 	= 0xf000800000; 	// FIXME: hard coded to match simics
void		*bw_apt_virt;
u64 		*bw_ctl_virt;
u32 		*bw_stat_virt;
size_t		bw_size = 8096;				// FIXME: hard coded for now
size_t		disk_size = (size_t)64  * 1024 * 1024 * 1024; 	// 64 GiB

static int ndb_major;
static DEFINE_MUTEX(ndb_mutex); // temporary until we get lanes for mutual exclusion.

struct ndb_device {
	struct request_queue	*ndb_queue;
	struct gendisk		*ndb_disk;
	struct list_head	ndb_list;
};

static LIST_HEAD(ndb_devices);
static int ndb_count = 1;

/* for now, hard code index 0 */
// for NT stores, check out __copy_user_nocache()
static void ndb_write_blk_ctl(sector_t sector, unsigned int len, bool write)
{
	u64 cmd 	= 0;
	u64 cl_offset 	= sector << (SECTOR_SHIFT - CL_SHIFT);
	u64 cl_len 	= len >> CL_SHIFT;

	cmd |= cl_offset & BCW_OFFSET_MASK;
	cmd |= (cl_len & BCW_LEN_MASK) << BCW_LEN_SHIFT;
	if (write)
		cmd |= 1UL << BCW_CMD_SHIFT;

	*bw_ctl_virt = cmd;
	clflushopt(bw_ctl_virt);
}

static void ndb_read_blk_win(void *dst, unsigned int len)
{
	u32 status;

	// FIXME: NT
	memcpy(dst, bw_apt_virt, len);
	clflushopt(bw_apt_virt);
	
	// FIXME: check status.  Do I need to clflushopt before I/O or
	// something?
	status = *bw_stat_virt;

	if (status)
		printk("REZ %s:  status:%08x\n", __func__, status);
}

// FIXME: track an I/O all the way through the stack to make sure it's being
// written in the right place
// FIXME: use direct I/O to test non-4096 I/Os
static void ndb_write_blk_win(void *src, unsigned int len)
{
	// non-temporal writes, need to flush via flush hints, yada yada.
	u32 status;

	// FIXME: NT
	memcpy(bw_apt_virt, src, len);

	status = *bw_stat_virt;

	if (status)
		printk("REZ %s:  status:%08x\n", __func__, status);
}

static void ndb_read(struct ndb_device *ndb, void *dst, sector_t sector, unsigned int len)
{
	ndb_write_blk_ctl(sector, len, false);
	ndb_read_blk_win(dst, len);
}

static void ndb_write(struct ndb_device *ndb, void *src, sector_t sector, unsigned int len)
{
	ndb_write_blk_ctl(sector, len, true);
	ndb_write_blk_win(src, len);
}

/* len is <= PAGE_SIZE by this point, so it can be done in a single BW I/O */
static void ndb_do_bvec(struct ndb_device *ndb, struct page *page,
			unsigned int len, unsigned int off, int rw,
			sector_t sector)
{
	void *mem = kmap_atomic(page);

	if (rw == READ)
		ndb_read(ndb, mem + off, sector, len);
	else
		ndb_write(ndb, mem + off, sector, len);

	kunmap_atomic(mem);
}

static void ndb_make_request(struct request_queue *q, struct bio *bio)
{
	struct block_device *bdev = bio->bi_bdev;
	struct ndb_device *ndb = bdev->bd_disk->private_data;
	int rw;
	struct bio_vec bvec;
	sector_t sector;
	struct bvec_iter iter;
	int err = 0;

	mutex_lock(&ndb_mutex);
	sector = bio->bi_iter.bi_sector;
	if (bio_end_sector(bio) > get_capacity(bdev->bd_disk)) {
		err = -EIO;
		goto out;
	}

	BUG_ON(bio->bi_rw & REQ_DISCARD);

	rw = bio_rw(bio);
	if (rw == READA)
		rw = READ;

	bio_for_each_segment(bvec, bio, iter) {
		unsigned int len = bvec.bv_len;
		BUG_ON(len > PAGE_SIZE);
		ndb_do_bvec(ndb, bvec.bv_page, len,
			    bvec.bv_offset, rw, sector);
		sector += len >> SECTOR_SHIFT;
	}

out:
	bio_endio(bio, err);
	mutex_unlock(&ndb_mutex);
}

static const struct block_device_operations ndb_fops = {
	.owner =		THIS_MODULE,
};

static struct ndb_device *ndb_alloc(int i)
{
	struct ndb_device *ndb;
	struct gendisk *disk;
	size_t disk_sectors =  disk_size >> SECTOR_SHIFT;

	ndb = kzalloc(sizeof(*ndb), GFP_KERNEL);
	if (!ndb)
		goto out;

	ndb->ndb_queue = blk_alloc_queue(GFP_KERNEL);
	if (!ndb->ndb_queue)
		goto out_free_dev;

	blk_queue_make_request(ndb->ndb_queue, ndb_make_request);
	blk_queue_max_hw_sectors(ndb->ndb_queue, 1024);
	blk_queue_bounce_limit(ndb->ndb_queue, BLK_BOUNCE_ANY);

	disk = ndb->ndb_disk = alloc_disk(0);
	if (!disk)
		goto out_free_queue;
	disk->major		= ndb_major;
	disk->first_minor	= 0;
	disk->fops		= &ndb_fops;
	disk->private_data	= ndb;
	disk->queue		= ndb->ndb_queue;
	disk->flags		= GENHD_FL_EXT_DEVT;
	sprintf(disk->disk_name, "nd%d", i);
	set_capacity(disk, disk_sectors);

	return ndb;

out_free_queue:
	blk_cleanup_queue(ndb->ndb_queue);
out_free_dev:
	kfree(ndb);
out:
	return NULL;
}

static void ndb_free(struct ndb_device *ndb)
{
	put_disk(ndb->ndb_disk);
	blk_cleanup_queue(ndb->ndb_queue);
	kfree(ndb);
}

static void ndb_del_one(struct ndb_device *ndb)
{
	list_del(&ndb->ndb_list);
	del_gendisk(ndb->ndb_disk);
	ndb_free(ndb);
}

static int __init ndb_init(void)
{
	int result, i;
	struct resource *res_mem;
	struct ndb_device *ndb, *next;

	/* map block aperture memory */
	res_mem = request_mem_region_exclusive(bw_apt_phys, bw_size, "nd-block");
	if (!res_mem)
		return -ENOMEM;

	bw_apt_virt = ioremap_cache(bw_apt_phys, bw_size);
	if (!bw_apt_virt) {
		result = -ENOMEM;
		goto out_release;
	}

	/* map block control memory */
	res_mem = request_mem_region_exclusive(bw_ctl_phys, bw_size, "nd-block");
	if (!res_mem)
		return -ENOMEM; // FIXME

	bw_ctl_virt = ioremap_cache(bw_ctl_phys, bw_size);
	if (!bw_ctl_virt) {
		result = -ENOMEM;
		goto out_release; // FIXME
	}
	bw_stat_virt = (void*)bw_ctl_virt + 0x1000;

	/* get a major */
	result = register_blkdev(ndb_major, "ndb");
	if (result < 0) {
		result = -EIO;
		goto out_unmap;
	} else if (result > 0)
		ndb_major = result;

	/* initialize our device structures */
	for (i = 0; i < ndb_count; i++) {
		ndb = ndb_alloc(i);
		if (!ndb) {
			result = -ENOMEM;
			goto out_free;
		}
		list_add_tail(&ndb->ndb_list, &ndb_devices);
	}

	list_for_each_entry(ndb, &ndb_devices, ndb_list)
		add_disk(ndb->ndb_disk);

	pr_info("nd-block: module loaded\n");
	return 0;

	// FIXME: enhance error handling for all the exit cases
out_free:
	list_for_each_entry_safe(ndb, next, &ndb_devices, ndb_list) {
		list_del(&ndb->ndb_list);
		ndb_free(ndb);
	}
	unregister_blkdev(ndb_major, "ndb");

out_unmap:
//	iounmap(virt_addr);

out_release:
	release_mem_region(bw_apt_phys, bw_size);
	return result;
}

static void __exit ndb_exit(void)
{
	struct ndb_device *ndb, *next;

	list_for_each_entry_safe(ndb, next, &ndb_devices, ndb_list)
		ndb_del_one(ndb);

	unregister_blkdev(ndb_major, "ndb");

	/* free block aperture memory */
	iounmap(bw_apt_virt);
	release_mem_region(bw_apt_phys, bw_size);

	/* free block control memory */
	iounmap(bw_ctl_virt);
	release_mem_region(bw_ctl_phys, bw_size);

	pr_info("nd-block: module unloaded\n");
}

MODULE_AUTHOR("Ross Zwisler <ross.zwisler@linux.intel.com>");
MODULE_LICENSE("GPL");
module_init(ndb_init);
module_exit(ndb_exit);
