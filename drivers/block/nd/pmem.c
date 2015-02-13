/*
 * Persistent Memory Driver
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
 *
 * This driver is heavily based on drivers/block/brd.c.
 * Copyright (C) 2007 Nick Piggin
 * Copyright (C) 2007 Novell Inc.
 */

#include <asm/cacheflush.h>
#include <linux/bio.h>
#include <linux/blkdev.h>
#include <linux/fs.h>
#include <linux/hdreg.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/cpumask.h>
#include <linux/slab.h>
#include <linux/nd.h>
#include "nd.h"

static DEFINE_IDA(pmem_ida);

#define SECTOR_SHIFT		9
#define PAGE_SECTORS_SHIFT	(PAGE_SHIFT - SECTOR_SHIFT)
#define PAGE_SECTORS		(1 << PAGE_SECTORS_SHIFT)

struct pmem_device {
	struct request_queue	*pmem_queue;
	struct gendisk		*pmem_disk;
	struct nd_io		ndio;

	/* One contiguous memory region per device */
	phys_addr_t		phys_addr;
	void			*virt_addr;
	size_t			size;
	int id;
};

static int pmem_getgeo(struct block_device *bd, struct hd_geometry *geo)
{
	/* some standard values */
	geo->heads = 1 << 6;
	geo->sectors = 1 << 5;
	geo->cylinders = get_capacity(bd->bd_disk) >> 11;
	return 0;
}

/*
 * direct translation from (pmem,sector) => void*
 * We do not require that sector be page aligned.
 * The return value will point to the beginning of the page containing the
 * given sector, not to the sector itself.
 */
static void *pmem_lookup_pg_addr(struct pmem_device *pmem, sector_t sector)
{
	size_t page_offset = sector >> PAGE_SECTORS_SHIFT;
	size_t offset = page_offset << PAGE_SHIFT;

	BUG_ON(offset >= pmem->size);
	return pmem->virt_addr + offset;
}

/* sector must be page aligned */
static unsigned long pmem_lookup_pfn(struct pmem_device *pmem, sector_t sector)
{
	size_t page_offset = sector >> PAGE_SECTORS_SHIFT;

	BUG_ON(sector & (PAGE_SECTORS - 1));
	return (pmem->phys_addr >> PAGE_SHIFT) + page_offset;
}

/*
 * sector is not required to be page aligned.
 * n is at most a single page, but could be less.
 */
static void copy_to_pmem(struct pmem_device *pmem, const void *src,
			sector_t sector, size_t n)
{
	void *dst;
	unsigned int offset = (sector & (PAGE_SECTORS - 1)) << SECTOR_SHIFT;
	size_t copy;

	BUG_ON(n > PAGE_SIZE);

	copy = min_t(size_t, n, PAGE_SIZE - offset);
	dst = pmem_lookup_pg_addr(pmem, sector);
	memcpy(dst + offset, src, copy);

	if (copy < n) {
		src += copy;
		sector += copy >> SECTOR_SHIFT;
		copy = n - copy;
		dst = pmem_lookup_pg_addr(pmem, sector);
		memcpy(dst, src, copy);
	}
}

/*
 * sector is not required to be page aligned.
 * n is at most a single page, but could be less.
 */
static void copy_from_pmem(void *dst, struct pmem_device *pmem,
			  sector_t sector, size_t n)
{
	void *src;
	unsigned int offset = (sector & (PAGE_SECTORS - 1)) << SECTOR_SHIFT;
	size_t copy;

	BUG_ON(n > PAGE_SIZE);

	copy = min_t(size_t, n, PAGE_SIZE - offset);
	src = pmem_lookup_pg_addr(pmem, sector);

	memcpy(dst, src + offset, copy);

	if (copy < n) {
		dst += copy;
		sector += copy >> SECTOR_SHIFT;
		copy = n - copy;
		src = pmem_lookup_pg_addr(pmem, sector);
		memcpy(dst, src, copy);
	}
}

static void pmem_do_bvec(struct pmem_device *pmem, struct page *page,
			unsigned int len, unsigned int off, int rw,
			sector_t sector)
{
	void *mem = kmap_atomic(page);

	if (rw == READ) {
		copy_from_pmem(mem + off, pmem, sector, len);
		flush_dcache_page(page);
	} else {
		/*
		 * FIXME: Need more involved flushing to ensure that writes to
		 * NVDIMMs are actually durable before returning.
		 */
		flush_dcache_page(page);
		copy_to_pmem(pmem, mem + off, sector, len);
	}

	kunmap_atomic(mem);
}

static void pmem_make_request(struct request_queue *q, struct bio *bio)
{
	struct block_device *bdev = bio->bi_bdev;
	struct pmem_device *pmem = bdev->bd_disk->private_data;
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

	bio_for_each_segment(bvec, bio, iter) {
		unsigned int len = bvec.bv_len;

		BUG_ON(len > PAGE_SIZE);
		pmem_do_bvec(pmem, bvec.bv_page, len,
			    bvec.bv_offset, rw, sector);
		sector += len >> SECTOR_SHIFT;
	}

out:
	bio_endio(bio, err);
}

static int pmem_rw_page(struct block_device *bdev, sector_t sector,
		       struct page *page, int rw)
{
	struct pmem_device *pmem = bdev->bd_disk->private_data;

	pmem_do_bvec(pmem, page, PAGE_CACHE_SIZE, 0, rw, sector);
	page_endio(page, rw & WRITE, 0);
	return 0;
}

static long pmem_direct_access(struct block_device *bdev, sector_t sector,
			      void **kaddr, unsigned long *pfn, long size)
{
	struct pmem_device *pmem = bdev->bd_disk->private_data;

	if (!pmem)
		return -ENODEV;

	*kaddr = pmem_lookup_pg_addr(pmem, sector);
	*pfn = pmem_lookup_pfn(pmem, sector);

	return pmem->size - (sector * 512);
}

static const struct block_device_operations pmem_fops = {
	.owner =		THIS_MODULE,
	.rw_page =		pmem_rw_page,
	.direct_access =	pmem_direct_access,
	.getgeo =		pmem_getgeo,
};

static int pmem_major;

static int pmem_rw_bytes(struct nd_io *ndio, void *buf, size_t offset,
		size_t n, unsigned long flags)
{
	struct pmem_device *pmem = container_of(ndio, typeof(*pmem), ndio);
	int rw = nd_data_dir(flags);

	if (unlikely(offset + n > pmem->size)) {
		dev_WARN_ONCE(ndio->dev, 1, "%s: request out of range\n",
				__func__);
		return -EFAULT;
	}

	if (rw == READ)
		memcpy(buf, pmem->virt_addr + offset, n);
	else
		memcpy(pmem->virt_addr + offset, buf, n);

	return 0;
}

static int nd_pmem_probe(struct device *dev)
{
	struct nd_region *nd_region = to_nd_region(dev->parent);
	struct nd_namespace_io *nsio = to_nd_namespace_io(dev);
	size_t disk_sectors = resource_size(&nsio->res) / 512;
	struct pmem_device *pmem;
	struct gendisk *disk;
	struct resource *res;
	int err;

	if (resource_size(&nsio->res) < ND_MIN_NAMESPACE_SIZE) {
		resource_size_t size = resource_size(&nsio->res);

		dev_dbg(dev, "%s: size: %pa, too small must be at least %#x\n",
				__func__, &size, ND_MIN_NAMESPACE_SIZE);
		return -ENODEV;
	}

	if (nd_region_to_namespace_type(nd_region) == ND_DEVICE_NAMESPACE_PMEM) {
		struct nd_namespace_pmem *nspm = to_nd_namespace_pmem(dev);

		if (!nspm->uuid) {
			dev_dbg(dev, "%s: uuid not set\n", __func__);
			return -ENODEV;
		}
	}

	pmem = kzalloc(sizeof(*pmem), GFP_KERNEL);
	if (!pmem)
		return -ENOMEM;

	pmem->id = ida_simple_get(&pmem_ida, 0, 0, GFP_KERNEL);
	if (pmem->id < 0) {
		err = pmem->id;
		goto err_ida;
	}

	res = request_mem_region(nsio->res.start, resource_size(&nsio->res),
			dev_name(dev));
	if (!res) {
		err = -EBUSY;
		goto err_request_mem_region;
	}

	/*
	 * TODO: update ioremap_cache() to drop the __iomem annotation on its
	 * return value.  All other usages in the kernel either mishandle the
	 * annotation or force cast it.  pmem assumes it is backed by normal
	 * memory that is suitable for DAX.
	 */
	pmem->virt_addr = (__force void *)ioremap_cache(res->start,
						resource_size(res));
	if (!pmem->virt_addr) {
		err = -ENXIO;
		goto err_ioremap;
	}
	pmem->phys_addr = res->start;
	pmem->size = resource_size(res);
	pmem->pmem_queue = blk_alloc_queue(GFP_KERNEL);
	if (!pmem->pmem_queue) {
		err = -ENOMEM;
		goto err_alloc_queue;
	}

	blk_queue_make_request(pmem->pmem_queue, pmem_make_request);
	blk_queue_max_hw_sectors(pmem->pmem_queue, 1024);
	blk_queue_bounce_limit(pmem->pmem_queue, BLK_BOUNCE_ANY);

	disk = pmem->pmem_disk = alloc_disk(0);
	if (!disk) {
		err = -ENOMEM;
		goto err_alloc_disk;
	}

	disk->driverfs_dev	= dev;
	disk->major		= pmem_major;
	disk->first_minor	= 0;
	disk->fops		= &pmem_fops;
	disk->private_data	= pmem;
	disk->queue		= pmem->pmem_queue;
	disk->flags		= GENHD_FL_EXT_DEVT;
	sprintf(disk->disk_name, "pmem%d", pmem->id);
	set_capacity(disk, disk_sectors);

	nd_bus_lock(dev);
	add_disk(disk);
	dev_set_drvdata(dev, pmem);
	nd_init_ndio(&pmem->ndio, pmem_rw_bytes, dev, disk,
			num_possible_cpus(), 0);
	nd_register_ndio(&pmem->ndio);
	nd_bus_unlock(dev);

	return 0;

 err_alloc_disk:
	blk_cleanup_queue(pmem->pmem_queue);
 err_alloc_queue:
	iounmap((__force void __iomem *)pmem->virt_addr);
 err_ioremap:
	release_mem_region(res->start, resource_size(res));
 err_request_mem_region:
	ida_simple_remove(&pmem_ida, pmem->id);
 err_ida:
	kfree(pmem);
	return err;
}

static int nd_pmem_remove(struct device *dev)
{
	struct nd_namespace_io *nsio = to_nd_namespace_io(dev);
	struct pmem_device *pmem = dev_get_drvdata(dev);

	nd_unregister_ndio(&pmem->ndio);
	del_gendisk(pmem->pmem_disk);
	blk_cleanup_queue(pmem->pmem_queue);
	iounmap((__force void __iomem *)pmem->virt_addr);
	release_mem_region(nsio->res.start, resource_size(&nsio->res));
	put_disk(pmem->pmem_disk);
	ida_simple_remove(&pmem_ida, pmem->id);
	kfree(pmem);

	return 0;
}

MODULE_ALIAS("pmem");
MODULE_ALIAS_ND_DEVICE(ND_DEVICE_NAMESPACE_IO);
MODULE_ALIAS_ND_DEVICE(ND_DEVICE_NAMESPACE_PMEM);
static struct nd_device_driver nd_pmem_driver = {
	.probe = nd_pmem_probe,
	.remove = nd_pmem_remove,
	.drv = {
		.name = "pmem",
	},
	.type = ND_DRIVER_NAMESPACE_IO | ND_DRIVER_NAMESPACE_PMEM,
};

static int __init pmem_init(void)
{
	int rc;

	rc = register_blkdev(0, "pmem");
	if (rc < 0)
		return rc;

	pmem_major = rc;
	rc = nd_driver_register(&nd_pmem_driver);

	if (rc < 0)
		unregister_blkdev(pmem_major, "pmem");

	return rc;
}

static void __exit pmem_exit(void)
{
	driver_unregister(&nd_pmem_driver.drv);
	unregister_blkdev(pmem_major, "pmem");
}

MODULE_AUTHOR("Ross Zwisler <ross.zwisler@linux.intel.com>");
MODULE_LICENSE("GPL");
module_init(pmem_init);
module_exit(pmem_exit);
