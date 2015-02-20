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
#include <linux/string.h>
#include "nd.h"

#include <asm-generic/io-64-nonatomic-lo-hi.h>

#define SECTOR_SHIFT		9

struct nd_blk_device {
	struct request_queue	*queue;
	struct gendisk		*disk;
	struct nd_namespace_blk *nsblk;
/*	struct nd_io		ndio; */

	size_t			disk_size;
	int			id;
};

static int nd_blk_major;
static DEFINE_IDA(nd_blk_ida);

static void nd_blk_make_request(struct request_queue *q, struct bio *bio)
{
	struct block_device *bdev = bio->bi_bdev;
	/* FIXME: reduce per I/O pointer chasing */
	struct nd_blk_device *blk_dev = bdev->bd_disk->private_data;
	struct nd_namespace_blk *nsblk = blk_dev->nsblk;
	struct nd_region *nd_region = to_nd_region(nsblk->dev.parent);
	struct nd_dimm_drvdata *ndd = to_ndd(&nd_region->mapping[0]);
	int rw, i;
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
		resource_size_t res_offset = sector << SECTOR_SHIFT;
		resource_size_t	dev_offset = 0;
		unsigned int len = bvec.bv_len;

		BUG_ON(len > PAGE_SIZE);

		for (i = 0; i < nsblk->num_resources; i++) {
			if (res_offset < resource_size(nsblk->res[i])) {
				BUG_ON(res_offset + len >
						resource_size(nsblk->res[i]));
				dev_offset = nsblk->res[i]->start +
					res_offset;
				break;
			}
			res_offset -= resource_size(nsblk->res[i]);
		}

		err = nd_blk_do_io(&ndd->blk_dimm, bvec.bv_page,
				len, bvec.bv_offset, rw, dev_offset);
		if (err)
			goto out;

		sector += len >> SECTOR_SHIFT;
	}

out:
	bio_endio(bio, err);
}

static const struct block_device_operations nd_blk_fops = {
	.owner =		THIS_MODULE,
};

static int nd_blk_probe(struct device *dev)
{
	struct nd_namespace_blk *nsblk = to_nd_namespace_blk(dev);
	resource_size_t disk_size = 0;
	struct nd_blk_device *blk_dev;
	struct gendisk *disk;
	int err, i;

	for (i = 0; i < nsblk->num_resources; i++)
		disk_size += resource_size(nsblk->res[i]);

	if (disk_size < ND_MIN_NAMESPACE_SIZE || !nsblk->uuid ||
			!nsblk->lbasize)
		return -ENXIO;

	if (!is_acpi_blk(dev))
		return 0;

	blk_dev = kzalloc(sizeof(*blk_dev), GFP_KERNEL);
	if (!blk_dev)
		return -ENOMEM;

	blk_dev->id = ida_simple_get(&nd_blk_ida, 0, 0, GFP_KERNEL);
	if (blk_dev->id < 0) {
		err = blk_dev->id;
		goto err_ida;
	}

	blk_dev->disk_size	= disk_size;

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

	blk_dev->nsblk = nsblk;

	disk->major		= nd_blk_major;
	disk->first_minor	= 0;
	disk->fops		= &nd_blk_fops;
	disk->private_data	= blk_dev;
	disk->queue		= blk_dev->queue;
	disk->flags		= GENHD_FL_EXT_DEVT;
	sprintf(disk->disk_name, "nd%d", blk_dev->id);
	set_capacity(disk, disk_size >> SECTOR_SHIFT);

	dev_set_drvdata(dev, blk_dev);

	add_disk(disk);

	return 0;

err_alloc_disk:
	blk_cleanup_queue(blk_dev->queue);
err_alloc_queue:
	ida_simple_remove(&nd_blk_ida, blk_dev->id);
err_ida:
	kfree(blk_dev);
	return err;
}

static int nd_blk_remove(struct device *dev)
{
	/* FIXME: eventually need to get to nd_blk_device from struct device.
	struct nd_namespace_io *nsio = to_nd_namespace_io(dev); */

	struct nd_blk_device *blk_dev = dev_get_drvdata(dev);

	if (!is_acpi_blk(dev))
		return 0;

	del_gendisk(blk_dev->disk);
	put_disk(blk_dev->disk);
	blk_cleanup_queue(blk_dev->queue);
	ida_simple_remove(&nd_blk_ida, blk_dev->id);
	kfree(blk_dev);

	blk_dev->nsblk = NULL;

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

MODULE_AUTHOR("Ross Zwisler <ross.zwisler@linux.intel.com>");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS_ND_DEVICE(ND_DEVICE_NAMESPACE_BLOCK);
module_init(nd_blk_init);
module_exit(nd_blk_exit);
