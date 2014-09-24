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
#include <linux/slab.h>

#define SECTOR_SHIFT		9
#define PAGE_SECTORS_SHIFT	(PAGE_SHIFT - SECTOR_SHIFT)
#define PAGE_SECTORS		(1 << PAGE_SECTORS_SHIFT)

struct pmem_device {
	struct request_queue	*pmem_queue;
	struct gendisk		*pmem_disk;
	struct list_head	pmem_list;

	/* One contiguous memory region per device */
	phys_addr_t		phys_addr;
	void			*virt_addr;
	size_t			size;
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

/* Kernel module stuff */
static int pmem_start_gb = CONFIG_BLK_DEV_PMEM_START;
module_param(pmem_start_gb, int, S_IRUGO);
MODULE_PARM_DESC(pmem_start_gb, "Offset in GB of where to start claiming space");

static int pmem_size_gb = CONFIG_BLK_DEV_PMEM_SIZE;
module_param(pmem_size_gb,  int, S_IRUGO);
MODULE_PARM_DESC(pmem_size_gb,  "Total size in GB of space to claim for all disks");

static int pmem_count = CONFIG_BLK_DEV_PMEM_COUNT;
module_param(pmem_count, int, S_IRUGO);
MODULE_PARM_DESC(pmem_count, "Number of pmem devices to evenly split allocated space");

static LIST_HEAD(pmem_devices);
static int pmem_major;

/* pmem->phys_addr and pmem->size need to be set.
 * Will then set virt_addr if successful.
 */
int pmem_mapmem(struct pmem_device *pmem)
{
	struct resource *res_mem;
	int err;

	res_mem = request_mem_region_exclusive(pmem->phys_addr, pmem->size,
					       "pmem");
	if (!res_mem) {
		pr_warn("pmem: request_mem_region_exclusive phys=0x%llx size=0x%zx failed\n",
			   pmem->phys_addr, pmem->size);
		return -EINVAL;
	}

	pmem->virt_addr = ioremap_cache(pmem->phys_addr, pmem->size);
	if (unlikely(!pmem->virt_addr)) {
		err = -ENXIO;
		goto out_release;
	}
	return 0;

out_release:
	release_mem_region(pmem->phys_addr, pmem->size);
	return err;
}

void pmem_unmapmem(struct pmem_device *pmem)
{
	if (unlikely(!pmem->virt_addr))
		return;

	iounmap(pmem->virt_addr);
	release_mem_region(pmem->phys_addr, pmem->size);
	pmem->virt_addr = NULL;
}

static struct pmem_device *pmem_alloc(phys_addr_t phys_addr, size_t disk_size,
				      int i)
{
	struct pmem_device *pmem;
	struct gendisk *disk;
	int err;

	pmem = kzalloc(sizeof(*pmem), GFP_KERNEL);
	if (unlikely(!pmem)) {
		err = -ENOMEM;
		goto out;
	}

	pmem->phys_addr = phys_addr;
	pmem->size = disk_size;

	err = pmem_mapmem(pmem);
	if (unlikely(err))
		goto out_free_dev;

	pmem->pmem_queue = blk_alloc_queue(GFP_KERNEL);
	if (unlikely(!pmem->pmem_queue)) {
		err = -ENOMEM;
		goto out_unmap;
	}

	blk_queue_make_request(pmem->pmem_queue, pmem_make_request);
	blk_queue_max_hw_sectors(pmem->pmem_queue, 1024);
	blk_queue_bounce_limit(pmem->pmem_queue, BLK_BOUNCE_ANY);

	disk = alloc_disk(0);
	if (unlikely(!disk)) {
		err = -ENOMEM;
		goto out_free_queue;
	}

	disk->major		= pmem_major;
	disk->first_minor	= 0;
	disk->fops		= &pmem_fops;
	disk->private_data	= pmem;
	disk->queue		= pmem->pmem_queue;
	disk->flags		= GENHD_FL_EXT_DEVT;
	sprintf(disk->disk_name, "pmem%d", i);
	set_capacity(disk, disk_size >> SECTOR_SHIFT);
	pmem->pmem_disk = disk;

	return pmem;

out_free_queue:
	blk_cleanup_queue(pmem->pmem_queue);
out_unmap:
	pmem_unmapmem(pmem);
out_free_dev:
	kfree(pmem);
out:
	return ERR_PTR(err);
}

static void pmem_free(struct pmem_device *pmem)
{
	put_disk(pmem->pmem_disk);
	blk_cleanup_queue(pmem->pmem_queue);
	pmem_unmapmem(pmem);
	kfree(pmem);
}

static void pmem_del_one(struct pmem_device *pmem)
{
	list_del(&pmem->pmem_list);
	del_gendisk(pmem->pmem_disk);
	pmem_free(pmem);
}

static int __init pmem_init(void)
{
	int result, i;
	struct pmem_device *pmem, *next;
	phys_addr_t phys_addr;
	size_t total_size, disk_size;

	phys_addr  = (phys_addr_t) pmem_start_gb * 1024 * 1024 * 1024;
	total_size = (size_t)	   pmem_size_gb  * 1024 * 1024 * 1024;
	disk_size = total_size / pmem_count;

	result = register_blkdev(0, "pmem");
	if (result < 0)
		return -EIO;
	else
		pmem_major = result;

	for (i = 0; i < pmem_count; i++) {
		pmem = pmem_alloc(phys_addr, disk_size, i);
		if (IS_ERR(pmem)) {
			result = PTR_ERR(pmem);
			goto out_free;
		}
		list_add_tail(&pmem->pmem_list, &pmem_devices);
		phys_addr += disk_size;
	}

	list_for_each_entry(pmem, &pmem_devices, pmem_list)
		add_disk(pmem->pmem_disk);

	pr_info("pmem: module loaded\n");
	return 0;

out_free:
	list_for_each_entry_safe(pmem, next, &pmem_devices, pmem_list) {
		list_del(&pmem->pmem_list);
		pmem_free(pmem);
	}
	unregister_blkdev(pmem_major, "pmem");

	return result;
}

static void __exit pmem_exit(void)
{
	struct pmem_device *pmem, *next;

	list_for_each_entry_safe(pmem, next, &pmem_devices, pmem_list)
		pmem_del_one(pmem);

	unregister_blkdev(pmem_major, "pmem");
	pr_info("pmem: module unloaded\n");
}

MODULE_AUTHOR("Ross Zwisler <ross.zwisler@linux.intel.com>");
MODULE_LICENSE("GPL");
module_init(pmem_init);
module_exit(pmem_exit);
