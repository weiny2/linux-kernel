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
#include <linux/sizes.h>

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


// FIXME: move this stuff into the ndbw_device
phys_addr_t	bw_apt_phys 	= 0xf008000000; 	// FIXME: hard coded to match simics
phys_addr_t	bw_ctl_phys 	= 0xf000800000; 	// FIXME: hard coded to match simics
void		*bw_apt_virt;
u64 		*bw_ctl_virt;
u32 		*bw_stat_virt;
size_t		bw_size 	= SZ_8K;
size_t		disk_size 	= SZ_64G;

static int ndbw_major;
static DEFINE_MUTEX(ndbw_mutex); // temporary until we get lanes for mutual exclusion.

struct ndbw_device {
	struct request_queue	*ndbw_queue;
	struct gendisk		*ndbw_disk;
	struct list_head	ndbw_list;
};

static LIST_HEAD(ndbw_devices);
static int ndbw_count = 1;

/* for now, hard code index 0 */
// for NT stores, check out __copy_user_nocache()
static void ndbw_write_blk_ctl(sector_t sector, unsigned int len, bool write)
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

static void ndbw_read_blk_win(void *dst, unsigned int len)
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
static void ndbw_write_blk_win(void *src, unsigned int len)
{
	// non-temporal writes, need to flush via flush hints, yada yada.
	u32 status;

	// FIXME: NT
	memcpy(bw_apt_virt, src, len);

	status = *bw_stat_virt;

	if (status)
		printk("REZ %s:  status:%08x\n", __func__, status);
}

static void ndbw_read(struct ndbw_device *ndbw, void *dst, sector_t sector, unsigned int len)
{
	ndbw_write_blk_ctl(sector, len, false);
	ndbw_read_blk_win(dst, len);
}

static void ndbw_write(struct ndbw_device *ndbw, void *src, sector_t sector, unsigned int len)
{
	ndbw_write_blk_ctl(sector, len, true);
	ndbw_write_blk_win(src, len);
}

/* len is <= PAGE_SIZE by this point, so it can be done in a single BW I/O */
static void ndbw_do_bvec(struct ndbw_device *ndbw, struct page *page,
			unsigned int len, unsigned int off, int rw,
			sector_t sector)
{
	void *mem = kmap_atomic(page);

	if (rw == READ)
		ndbw_read(ndbw, mem + off, sector, len);
	else
		ndbw_write(ndbw, mem + off, sector, len);

	kunmap_atomic(mem);
}

static void ndbw_make_request(struct request_queue *q, struct bio *bio)
{
	struct block_device *bdev = bio->bi_bdev;
	struct ndbw_device *ndbw = bdev->bd_disk->private_data;
	int rw;
	struct bio_vec bvec;
	sector_t sector;
	struct bvec_iter iter;
	int err = 0;

	mutex_lock(&ndbw_mutex);
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
		ndbw_do_bvec(ndbw, bvec.bv_page, len,
			    bvec.bv_offset, rw, sector);
		sector += len >> SECTOR_SHIFT;
	}

out:
	bio_endio(bio, err);
	mutex_unlock(&ndbw_mutex);
}

static const struct block_device_operations ndbw_fops = {
	.owner =		THIS_MODULE,
};

static struct ndbw_device *ndbw_alloc(int i)
{
	struct ndbw_device *ndbw;
	struct gendisk *disk;
	size_t disk_sectors =  disk_size >> SECTOR_SHIFT;

	ndbw = kzalloc(sizeof(*ndbw), GFP_KERNEL);
	if (!ndbw)
		goto out;

	ndbw->ndbw_queue = blk_alloc_queue(GFP_KERNEL);
	if (!ndbw->ndbw_queue)
		goto out_free_dev;

	blk_queue_make_request(ndbw->ndbw_queue, ndbw_make_request);
	blk_queue_max_hw_sectors(ndbw->ndbw_queue, 1024);
	blk_queue_bounce_limit(ndbw->ndbw_queue, BLK_BOUNCE_ANY);

	disk = ndbw->ndbw_disk = alloc_disk(0);
	if (!disk)
		goto out_free_queue;
	disk->major		= ndbw_major;
	disk->first_minor	= 0;
	disk->fops		= &ndbw_fops;
	disk->private_data	= ndbw;
	disk->queue		= ndbw->ndbw_queue;
	disk->flags		= GENHD_FL_EXT_DEVT;
	sprintf(disk->disk_name, "nd%d", i);
	set_capacity(disk, disk_sectors);

	return ndbw;

out_free_queue:
	blk_cleanup_queue(ndbw->ndbw_queue);
out_free_dev:
	kfree(ndbw);
out:
	return NULL;
}

static void ndbw_free(struct ndbw_device *ndbw)
{
	put_disk(ndbw->ndbw_disk);
	blk_cleanup_queue(ndbw->ndbw_queue);
	kfree(ndbw);
}

static void ndbw_del_one(struct ndbw_device *ndbw)
{
	list_del(&ndbw->ndbw_list);
	del_gendisk(ndbw->ndbw_disk);
	ndbw_free(ndbw);
}

static int __init ndbw_init(void)
{
	int rc, i;
	struct ndbw_device *ndbw, *next;

	rc = register_blkdev(ndbw_major, "nd_blk");
	if (rc < 0)
		return rc;

	ndbw_major = rc;

	for (i = 0; i < ndbw_count; i++) {
		ndbw = ndbw_alloc(i);
		if (!ndbw) {
			rc = -ENOMEM;
			goto out_free;
		}
		list_add_tail(&ndbw->ndbw_list, &ndbw_devices);
	}

	list_for_each_entry(ndbw, &ndbw_devices, ndbw_list)
		add_disk(ndbw->ndbw_disk);

	return 0;

out_free:
	list_for_each_entry_safe(ndbw, next, &ndbw_devices, ndbw_list) {
		list_del(&ndbw->ndbw_list);
		ndbw_free(ndbw);
	}
	unregister_blkdev(ndbw_major, "ndbw");
	return rc;
}

static void __exit ndbw_exit(void)
{
	struct ndbw_device *ndbw, *next;

	list_for_each_entry_safe(ndbw, next, &ndbw_devices, ndbw_list)
		ndbw_del_one(ndbw);

	unregister_blkdev(ndbw_major, "nd_blk");
}

/* BEGIN HELPER FUNCTIONS - EVENTUALLY IN ND */

/* This code should do things that will eventually be moved into the rest of
 * ND.  This includes things like
 * 	- ioremapping and iounmapping the BDWs and DCRs
 * 	- initializing instances of the driver with proper parameters
 * 	- when we do interleaving, implementing a generic interleaving method
 */
static int __init ndbw_wrapper_init(void)
{
	struct resource *res;
	int err;

	pr_info("nd_blk: module loaded via wrapper\n");

	/* map block aperture memory */
	res = request_mem_region_exclusive(bw_apt_phys, bw_size, "nd_blk");
	if (!res)
		return -EBUSY;

	bw_apt_virt = ioremap_cache(bw_apt_phys, bw_size);
	if (!bw_apt_virt) {
		err = -ENXIO;
		goto err_apt_ioremap;
	}

	/* map block control memory */
	res = request_mem_region_exclusive(bw_ctl_phys, bw_size, "nd_blk");
	if (!res) {
		err = -EBUSY;
		goto err_ctl_reserve;
	}

	bw_ctl_virt = ioremap_cache(bw_ctl_phys, bw_size);
	if (!bw_ctl_virt) {
		err = -ENXIO;
		goto err_ctl_ioremap;
	}

	bw_stat_virt = (void*)bw_ctl_virt + 0x1000;

	err = ndbw_init();
	if (err < 0)
		goto err_init;

	return 0;

err_init:
	iounmap(bw_ctl_virt);
err_ctl_ioremap:
	release_mem_region(bw_ctl_phys, bw_size);
err_ctl_reserve:
	iounmap(bw_apt_virt);
err_apt_ioremap:
	release_mem_region(bw_apt_phys, bw_size);
	return err;
}

static void __exit ndbw_wrapper_exit(void)
{
	ndbw_exit();

	/* free block control memory */
	iounmap(bw_ctl_virt);
	release_mem_region(bw_ctl_phys, bw_size);

	/* free block aperture memory */
	iounmap(bw_apt_virt);
	release_mem_region(bw_apt_phys, bw_size);

	pr_info("nd_blk: module unloaded via wrapper\n");
}

/* END HELPER FUNCTIONS - EVENTUALLY IN ND */

MODULE_AUTHOR("Ross Zwisler <ross.zwisler@linux.intel.com>");
MODULE_LICENSE("GPL v2");
module_init(ndbw_wrapper_init);
module_exit(ndbw_wrapper_exit);
