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
#include <linux/nd.h>
#include <linux/sizes.h>

#include <asm-generic/io-64-nonatomic-lo-hi.h>

#define SECTOR_SHIFT		9
#define CL_SHIFT		6

enum {
	BCW_OFFSET_MASK		= (1UL << 48)-1,
	BCW_LEN_SHIFT		= 48,
	BCW_LEN_MASK		= (1UL << 8) - 1,
	BCW_CMD_SHIFT		= 56,
};

struct block_window {
	void			*bw_apt_virt;
	u64 			*bw_ctl_virt;
	u32 			*bw_stat_virt;
};

struct nd_blk_device {
	struct request_queue	*queue;
	struct gendisk		*disk;
//	struct nd_io		ndio;

	struct block_window	*bw;
	int 			num_bw;
	atomic_t 		last_bw;
	spinlock_t 		*bw_lock; /* Array of 'num_bw' locks */

	size_t			disk_size;
	int 			id;
};

static int nd_blk_major;
struct nd_blk_device *nd_blk_singleton;
static DEFINE_IDA(nd_blk_ida);

/* for now, hard code index 0 */
// for NT stores, check out __copy_user_nocache()
static void nd_blk_write_blk_ctl(struct block_window *bw, sector_t sector,
		unsigned int len, bool write)
{
	u64 cmd 	= 0;
	u64 cl_offset 	= sector << (SECTOR_SHIFT - CL_SHIFT);
	u64 cl_len 	= len >> CL_SHIFT;

	cmd |= cl_offset & BCW_OFFSET_MASK;
	cmd |= (cl_len & BCW_LEN_MASK) << BCW_LEN_SHIFT;
	if (write)
		cmd |= 1UL << BCW_CMD_SHIFT;

	writeq(cmd, bw->bw_ctl_virt);
	clflushopt(bw->bw_ctl_virt);
}

static int nd_blk_read_blk_win(struct block_window *bw, void *dst,
		unsigned int len)
{
	u32 status;

	// FIXME: NT
	memcpy(dst, bw->bw_apt_virt, len);
	clflushopt(bw->bw_apt_virt);
	
	status = readl(bw->bw_stat_virt);

	if (status) {
		/* FIXME: return more precise error values at some point */
		return -EIO;
	}

	return 0;
}

static int nd_blk_write_blk_win(struct block_window *bw, void *src,
		unsigned int len)
{
	// non-temporal writes, need to flush via flush hints, yada yada.
	u32 status;

	// FIXME: NT
	memcpy(bw->bw_apt_virt, src, len);

	status = readl(bw->bw_stat_virt);

	if (status) {
		/* FIXME: return more precise error values at some point */
		return -EIO;
	}

	return 0;
}

static int nd_blk_read(struct block_window *bw, void *dst, sector_t sector,
		unsigned int len)
{
	nd_blk_write_blk_ctl(bw, sector, len, false);
	return nd_blk_read_blk_win(bw, dst, len);
}

static int nd_blk_write(struct block_window *bw, void *src, sector_t sector,
		unsigned int len)
{
	nd_blk_write_blk_ctl(bw, sector, len, true);
	return nd_blk_write_blk_win(bw, src, len);
}

/* len is <= PAGE_SIZE by this point, so it can be done in a single BW I/O */
static int nd_blk_do_bvec(struct block_window *bw, struct page *page,
		unsigned int len, unsigned int off, int rw, sector_t sector)
{
	void *mem = kmap_atomic(page);
	int rc;

	if (rw == READ)
		rc = nd_blk_read(bw, mem + off, sector, len);
	else
		rc = nd_blk_write(bw, mem + off, sector, len);

	kunmap_atomic(mem);

	return rc;
}

static void acquire_bw(struct nd_blk_device *blk_dev, int *bw_index)
{
	*bw_index = atomic_inc_return(&(blk_dev->last_bw)) % blk_dev->num_bw;
	spin_lock(&(blk_dev->bw_lock[*bw_index]));
}

static void release_bw(struct nd_blk_device *blk_dev, int bw_index)
{
	spin_unlock(&(blk_dev->bw_lock[bw_index]));
}

static void nd_blk_make_request(struct request_queue *q, struct bio *bio)
{
	struct block_device *bdev = bio->bi_bdev;
	struct nd_blk_device *blk_dev = bdev->bd_disk->private_data;
	int rw;
	struct bio_vec bvec;
	sector_t sector;
	struct bvec_iter iter;
	int err = 0;
	int bw_index;

	acquire_bw(blk_dev, &bw_index);

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

		err = nd_blk_do_bvec(&blk_dev->bw[bw_index], bvec.bv_page, len,
				bvec.bv_offset, rw, sector);
		if (err)
			goto out;

		sector += len >> SECTOR_SHIFT;
	}

out:
	bio_endio(bio, err);
	release_bw(blk_dev, bw_index);
}

static const struct block_device_operations nd_blk_fops = {
	.owner =		THIS_MODULE,
};

static int nd_blk_init_locks(struct nd_blk_device *blk_dev)
{
	int i;

	blk_dev->bw_lock = kmalloc(blk_dev->num_bw * sizeof(spinlock_t),
				GFP_KERNEL);
	if (!blk_dev->bw_lock)
		return -ENOMEM;

	for (i = 0; i < blk_dev->num_bw; i++)
		spin_lock_init(&blk_dev->bw_lock[i]);

	return 0;
}

static int nd_blk_probe_dimm(struct block_window *bw, int num_bw,
		size_t disk_size, struct device *dev)
{
	//FIXME: need to get all those params via struct device eventually
	size_t disk_sectors =  disk_size >> SECTOR_SHIFT;
	struct nd_blk_device *blk_dev;
	struct gendisk *disk;
	int err;

	blk_dev = kzalloc(sizeof(*blk_dev), GFP_KERNEL);
	if (!blk_dev)
		return -ENOMEM;

	blk_dev->id = ida_simple_get(&nd_blk_ida, 0, 0, GFP_KERNEL);
	if (blk_dev->id < 0) {
		err = blk_dev->id;
		goto err_ida;
	}

	blk_dev->bw		= bw;
	blk_dev->num_bw		= num_bw;
	blk_dev->disk_size	= disk_size;

	err = nd_blk_init_locks(blk_dev);
	if (err)
		goto err_init_locks;

	blk_dev->queue = blk_alloc_queue(GFP_KERNEL);
	if (!blk_dev->queue) {
		err = -ENOMEM;
		goto err_alloc_queue;
	}

	blk_queue_make_request(blk_dev->queue, nd_blk_make_request);
	blk_queue_max_hw_sectors(blk_dev->queue, 1024);
	blk_queue_bounce_limit(blk_dev->queue, BLK_BOUNCE_ANY);

	disk = blk_dev->disk = alloc_disk(0);
	if (!disk) {
		err = -ENOMEM;
		goto err_alloc_disk;
	}

//	disk->driverfs_dev	= dev;
	disk->major		= nd_blk_major;
	disk->first_minor	= 0;
	disk->fops		= &nd_blk_fops;
	disk->private_data	= blk_dev;
	disk->queue		= blk_dev->queue;
	disk->flags		= GENHD_FL_EXT_DEVT;
	sprintf(disk->disk_name, "nd%d", blk_dev->id);
	set_capacity(disk, disk_sectors);

	add_disk(disk);
//	dev_set_drvdata(dev, blk_dev);

	nd_blk_singleton = blk_dev;

	return 0;

err_alloc_disk:
	blk_cleanup_queue(blk_dev->queue);
err_alloc_queue:
	kfree(blk_dev->bw_lock);
err_init_locks:
	ida_simple_remove(&nd_blk_ida, blk_dev->id);
err_ida:
	kfree(blk_dev);
	return err;
}

static int nd_blk_remove_dimm(struct nd_blk_device *blk_dev)
{
	// FIXME: eventually need to get to nd_blk_device from struct device.
//	struct nd_namespace_io *nsio = to_nd_namespace_io(dev);
//	struct pmem_device *pmem = dev_get_drvdata(dev);

	del_gendisk(blk_dev->disk);
	put_disk(blk_dev->disk);
	blk_cleanup_queue(blk_dev->queue);
	kfree(blk_dev->bw_lock);
	ida_simple_remove(&nd_blk_ida, blk_dev->id);
	kfree(blk_dev);

	return 0;
}

static int nd_blk_probe(struct device *dev)
{
	struct nd_namespace_blk *nsblk = to_nd_namespace_blk(dev);
	resource_size_t size = 0;
	int i;

	for (i = 0; i < nsblk->num_resources; i++)
		size += resource_size(nsblk->res[i]);

	if (size < ND_MIN_NAMESPACE_SIZE || !nsblk->uuid
			|| !nsblk->lbasize)
		return -ENXIO;

	return 0;
}

static int nd_blk_remove(struct device *dev)
{
	return 0;
}

static struct nd_device_driver nd_blk_driver = {
	.probe = nd_blk_probe,
	.remove = nd_blk_remove,
	.drv = {
		.name = "nd_blk",
	},
	.type = ND_DRIVER_NAMESPACE_BLOCK,
};

static int __init nd_blk_init(void)
{
	int rc;

	rc = register_blkdev(0, "nd_blk");
	if (rc < 0)
		return rc;

	nd_blk_major = rc;
	rc = nd_driver_register(&nd_blk_driver);

	if (rc < 0)
		unregister_blkdev(nd_blk_major, "nd_blk");

	return rc;
}

static void __exit nd_blk_exit(void)
{
	driver_unregister(&nd_blk_driver.drv);
	unregister_blkdev(nd_blk_major, "nd_blk");
}

/* BEGIN HELPER FUNCTIONS - EVENTUALLY IN ND */

struct block_window 	*bw;
#define NUM_BW	 	256
static phys_addr_t	bw_apt_phys	= 0xf000a00000;
static phys_addr_t	bw_ctl_phys 	= 0xf000800000;
static void		*bw_apt_virt;
static u64 		*bw_ctl_virt;
static size_t		bw_size 	= SZ_8K * NUM_BW;
static size_t		disk_size 	= SZ_64G;

/* This code should do things that will eventually be moved into the rest of
 * ND.  This includes things like
 * 	- ioremapping and iounmapping the BDWs and DCRs
 * 	- initializing instances of the driver with proper parameters
 * 	- when we do interleaving, implementing a generic interleaving method
 */
static int __init nd_blk_wrapper_init(void)
{
	struct resource *res;
	int err, i;

	pr_info("nd_blk: module loaded via wrapper\n");

	/* map block aperture memory */
	res = request_mem_region(bw_apt_phys, bw_size, "nd_blk");
	if (!res)
		return -EBUSY;

	bw_apt_virt = ioremap_cache(bw_apt_phys, bw_size);
	if (!bw_apt_virt) {
		err = -ENXIO;
		goto err_apt_ioremap;
	}

	/* map block control memory */
	res = request_mem_region(bw_ctl_phys, bw_size, "nd_blk");
	if (!res) {
		err = -EBUSY;
		goto err_ctl_reserve;
	}

	bw_ctl_virt = ioremap_cache(bw_ctl_phys, bw_size);
	if (!bw_ctl_virt) {
		err = -ENXIO;
		goto err_ctl_ioremap;
	}

	/* set up block windows */
	bw = kmalloc(sizeof(*bw) * NUM_BW, GFP_KERNEL);
	if (!bw) {
		err = -ENOMEM;
		goto err_bw;
	}

	for (i = 0; i < NUM_BW; i++) {
		bw[i].bw_apt_virt = (void*)bw_apt_virt + i*0x2000;
		bw[i].bw_ctl_virt = (void*)bw_ctl_virt + i*0x2000;
		bw[i].bw_stat_virt = (void*)bw[i].bw_ctl_virt + 0x1000;
	}

	err = nd_blk_init();
	if (err < 0)
		goto err_init;

	nd_blk_probe_dimm(bw, NUM_BW, disk_size, NULL);

	return 0;

err_init:
	kfree(bw);
err_bw:
	iounmap(bw_ctl_virt);
err_ctl_ioremap:
	release_mem_region(bw_ctl_phys, bw_size);
err_ctl_reserve:
	iounmap(bw_apt_virt);
err_apt_ioremap:
	release_mem_region(bw_apt_phys, bw_size);
	return err;
}

static void __exit nd_blk_wrapper_exit(void)
{
	nd_blk_remove_dimm(nd_blk_singleton);

	nd_blk_exit();

	kfree(bw);

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
MODULE_ALIAS_ND_DEVICE(ND_DEVICE_NAMESPACE_BLOCK);
module_init(nd_blk_wrapper_init);
module_exit(nd_blk_wrapper_exit);
