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

#include <asm-generic/io-64-nonatomic-lo-hi.h>

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

struct block_window {
	void			*bw_apt_virt;
	u64 			*bw_ctl_virt;
	u32 			*bw_stat_virt;
};

struct ndbw_device {
	struct request_queue	*ndbw_queue;
	struct gendisk		*ndbw_disk;
//	struct nd_io		ndio;

	struct block_window	*bw;
	int 			num_bw;

	size_t			disk_size;
	int 			id;
};

static int ndbw_major;
static DEFINE_MUTEX(ndbw_mutex); // temporary until we get lanes for mutual exclusion.
struct ndbw_device *ndbw_singleton;
static DEFINE_IDA(ndbw_ida);

/* for now, hard code index 0 */
// for NT stores, check out __copy_user_nocache()
static void ndbw_write_blk_ctl(struct ndbw_device *bw_dev, sector_t sector,
		unsigned int len, bool write)
{
	u64 cmd 	= 0;
	u64 cl_offset 	= sector << (SECTOR_SHIFT - CL_SHIFT);
	u64 cl_len 	= len >> CL_SHIFT;

	cmd |= cl_offset & BCW_OFFSET_MASK;
	cmd |= (cl_len & BCW_LEN_MASK) << BCW_LEN_SHIFT;
	if (write)
		cmd |= 1UL << BCW_CMD_SHIFT;

	writeq(cmd, bw_dev->bw[255].bw_ctl_virt);
	clflushopt(bw_dev->bw[255].bw_ctl_virt);
}

static int ndbw_read_blk_win(struct ndbw_device *bw_dev, void *dst,
		unsigned int len)
{
	u32 status;

	// FIXME: NT
	memcpy(dst, bw_dev->bw[255].bw_apt_virt, len);
	clflushopt(bw_dev->bw[255].bw_apt_virt);
	
	status = readl(bw_dev->bw[255].bw_stat_virt);

	if (status) {
		/* FIXME: return more precise error values at some point */
		return -EIO;
	}

	return 0;
}

static int ndbw_write_blk_win(struct ndbw_device *bw_dev, void *src,
		unsigned int len)
{
	// non-temporal writes, need to flush via flush hints, yada yada.
	u32 status;

	// FIXME: NT
	memcpy(bw_dev->bw[255].bw_apt_virt, src, len);

	status = readl(bw_dev->bw[255].bw_stat_virt);

	if (status) {
		/* FIXME: return more precise error values at some point */
		return -EIO;
	}

	return 0;
}

static int ndbw_read(struct ndbw_device *bw_dev, void *dst, sector_t sector, unsigned int len)
{
	ndbw_write_blk_ctl(bw_dev, sector, len, false);
	return ndbw_read_blk_win(bw_dev, dst, len);
}

static int ndbw_write(struct ndbw_device *bw_dev, void *src, sector_t sector, unsigned int len)
{
	ndbw_write_blk_ctl(bw_dev, sector, len, true);
	return ndbw_write_blk_win(bw_dev, src, len);
}

/* len is <= PAGE_SIZE by this point, so it can be done in a single BW I/O */
static int ndbw_do_bvec(struct ndbw_device *bw_dev, struct page *page,
			unsigned int len, unsigned int off, int rw,
			sector_t sector)
{
	void *mem = kmap_atomic(page);
	int rc;

	if (rw == READ)
		rc = ndbw_read(bw_dev, mem + off, sector, len);
	else
		rc = ndbw_write(bw_dev, mem + off, sector, len);

	kunmap_atomic(mem);

	return rc;
}

static void ndbw_make_request(struct request_queue *q, struct bio *bio)
{
	struct block_device *bdev = bio->bi_bdev;
	struct ndbw_device *bw_dev = bdev->bd_disk->private_data;
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

		err = ndbw_do_bvec(bw_dev, bvec.bv_page, len,
			    bvec.bv_offset, rw, sector);
		if (err)
			goto out;

		sector += len >> SECTOR_SHIFT;
	}

out:
	bio_endio(bio, err);
	mutex_unlock(&ndbw_mutex);
}

static const struct block_device_operations ndbw_fops = {
	.owner =		THIS_MODULE,
};

//static int ndbw_probe(struct device *dev)
static int ndbw_probe(struct block_window *bw, int num_bw,
		size_t disk_size, struct device *dev)
{
	//FIXME: need to get all those params via struct device eventually
	size_t disk_sectors =  disk_size >> SECTOR_SHIFT;
	struct ndbw_device *bw_dev;
	struct gendisk *disk;
	int err;

	bw_dev = kzalloc(sizeof(*bw_dev), GFP_KERNEL);
	if (!bw_dev)
		return -ENOMEM;

	bw_dev->id = ida_simple_get(&ndbw_ida, 0, 0, GFP_KERNEL);
	if (bw_dev->id < 0) {
		err = bw_dev->id;
		goto err_ida;
	}

	bw_dev->bw            = bw;
	bw_dev->num_bw 	    = num_bw;
	bw_dev->disk_size     = disk_size;

	bw_dev->ndbw_queue = blk_alloc_queue(GFP_KERNEL);
	if (!bw_dev->ndbw_queue) {
		err = -ENOMEM;
		goto err_alloc_queue;
	}

	blk_queue_make_request(bw_dev->ndbw_queue, ndbw_make_request);
	blk_queue_max_hw_sectors(bw_dev->ndbw_queue, 1024);
	blk_queue_bounce_limit(bw_dev->ndbw_queue, BLK_BOUNCE_ANY);

	disk = bw_dev->ndbw_disk = alloc_disk(0);
	if (!disk) {
		err = -ENOMEM;
		goto err_alloc_disk;
	}

//	disk->driverfs_dev	= dev;
	disk->major		= ndbw_major;
	disk->first_minor	= 0;
	disk->fops		= &ndbw_fops;
	disk->private_data	= bw_dev;
	disk->queue		= bw_dev->ndbw_queue;
	disk->flags		= GENHD_FL_EXT_DEVT;
	sprintf(disk->disk_name, "nd%d", bw_dev->id);
	set_capacity(disk, disk_sectors);

	add_disk(disk);
//	dev_set_drvdata(dev, bw_dev);

	ndbw_singleton = bw_dev;

	return 0;

err_alloc_disk:
	blk_cleanup_queue(bw_dev->ndbw_queue);
err_alloc_queue:
	ida_simple_remove(&ndbw_ida, bw_dev->id);
err_ida:
	kfree(bw_dev);
	return err;
}

//static int ndbw_remove(struct device *dev)
//called once per device before ndbw_exit is called
static int ndbw_remove(struct ndbw_device *bw_dev)
{
	// FIXME: eventually need to get to ndbw_device from struct device.
//	struct nd_namespace_io *nsio = to_nd_namespace_io(dev);
//	struct pmem_device *pmem = dev_get_drvdata(dev);

	del_gendisk(bw_dev->ndbw_disk);
	put_disk(bw_dev->ndbw_disk);
	blk_cleanup_queue(bw_dev->ndbw_queue);
	ida_simple_remove(&ndbw_ida, bw_dev->id);
	kfree(bw_dev);

	return 0;
}

static int __init ndbw_init(void)
{
	int rc;

	rc = register_blkdev(0, "nd_blk");
	if (rc < 0)
		return rc;

	ndbw_major = rc;
	// FIXME: nd_driver registration & error handling
	/*
	rc = nd_driver_register(&nd_ndbw_driver);

	if (rc < 0)
		unregister_blkdev(ndbw_major, "ndbw");
	*/

	return 0;
}

static void __exit ndbw_exit(void)
{
	// FIXME: nd driver unregister
	unregister_blkdev(ndbw_major, "nd_blk");
}

/* BEGIN HELPER FUNCTIONS - EVENTUALLY IN ND */

struct block_window 	*bw;
#define NUM_BW	 	256
static phys_addr_t	bw_apt_phys 	= 0xf008000000;
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
static int __init ndbw_wrapper_init(void)
{
	struct resource *res;
	int err, i;

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

	err = ndbw_init();
	if (err < 0)
		goto err_init;

	ndbw_probe(bw, NUM_BW, disk_size, NULL);

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

static void __exit ndbw_wrapper_exit(void)
{
	ndbw_remove(ndbw_singleton);

	ndbw_exit();

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
module_init(ndbw_wrapper_init);
module_exit(ndbw_wrapper_exit);
