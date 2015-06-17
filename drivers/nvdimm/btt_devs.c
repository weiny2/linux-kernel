/*
 * Copyright(c) 2013-2015 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */
#include <linux/blkdev.h>
#include <linux/device.h>
#include <linux/genhd.h>
#include <linux/sizes.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include "nd-core.h"
#include "btt.h"
#include "nd.h"

static DEFINE_IDA(btt_ida);

static void nd_btt_release(struct device *dev)
{
	struct nd_btt *nd_btt = to_nd_btt(dev);

	dev_dbg(dev, "%s\n", __func__);
	WARN_ON(nd_btt->backing_dev);
	ida_simple_remove(&btt_ida, nd_btt->id);
	kfree(nd_btt->uuid);
	kfree(nd_btt);
}

static struct device_type nd_btt_device_type = {
	.name = "nd_btt",
	.release = nd_btt_release,
};

bool is_nd_btt(struct device *dev)
{
	return dev->type == &nd_btt_device_type;
}

struct nd_btt *to_nd_btt(struct device *dev)
{
	struct nd_btt *nd_btt = container_of(dev, struct nd_btt, dev);

	WARN_ON(!is_nd_btt(dev));
	return nd_btt;
}
EXPORT_SYMBOL(to_nd_btt);

static const unsigned long btt_lbasize_supported[] = { 512, 520, 528,
	4096, 4104, 4160, 4224, 0 };

static ssize_t sector_size_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nd_btt *nd_btt = to_nd_btt(dev);

	return nd_sector_size_show(nd_btt->lbasize, btt_lbasize_supported, buf);
}

static ssize_t sector_size_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	struct nd_btt *nd_btt = to_nd_btt(dev);
	ssize_t rc;

	device_lock(dev);
	nvdimm_bus_lock(dev);
	rc = nd_sector_size_store(dev, buf, &nd_btt->lbasize,
			btt_lbasize_supported);
	dev_dbg(dev, "%s: result: %zd wrote: %s%s", __func__,
			rc, buf, buf[len - 1] == '\n' ? "" : "\n");
	nvdimm_bus_unlock(dev);
	device_unlock(dev);

	return rc ? rc : len;
}
static DEVICE_ATTR_RW(sector_size);

static ssize_t uuid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nd_btt *nd_btt = to_nd_btt(dev);

	if (nd_btt->uuid)
		return sprintf(buf, "%pUb\n", nd_btt->uuid);
	return sprintf(buf, "\n");
}

static ssize_t uuid_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	struct nd_btt *nd_btt = to_nd_btt(dev);
	ssize_t rc;

	device_lock(dev);
	rc = nd_uuid_store(dev, &nd_btt->uuid, buf, len);
	dev_dbg(dev, "%s: result: %zd wrote: %s%s", __func__,
			rc, buf, buf[len - 1] == '\n' ? "" : "\n");
	device_unlock(dev);

	return rc ? rc : len;
}
static DEVICE_ATTR_RW(uuid);

static ssize_t backing_dev_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nd_btt *nd_btt = to_nd_btt(dev);
	char name[BDEVNAME_SIZE];

	if (nd_btt->backing_dev)
		return sprintf(buf, "/dev/%s\n",
				bdevname(nd_btt->backing_dev, name));
	else
		return sprintf(buf, "\n");
}

static const fmode_t nd_btt_devs_mode = FMODE_READ | FMODE_EXCL;

static void nd_btt_remove_bdev(struct nd_btt *nd_btt, const char *caller)
{
	struct block_device *bdev = nd_btt->backing_dev;
	char bdev_name[BDEVNAME_SIZE];

	if (!nd_btt->backing_dev)
		return;

	WARN_ON_ONCE(!is_nvdimm_bus_locked(&nd_btt->dev));
	dev_dbg(&nd_btt->dev, "%s: %s: release %s\n", caller, __func__,
			bdevname(bdev, bdev_name));
	blkdev_put(bdev, nd_btt_devs_mode);
	nd_btt->backing_dev = NULL;

	/*
	 * Once we've had our backing device removed we need to be fully
	 * reconfigured.  The bus will have already created a new seed
	 * for this purpose, so now is a good time to clean up this
	 * stale nd_btt instance.
	 */
	if (nd_btt->dev.driver)
		nd_device_unregister(&nd_btt->dev, ND_ASYNC);
}

static int __nd_btt_remove_disk(struct device *dev, void *data)
{
	struct gendisk *disk = data;
	struct block_device *bdev;
	struct nd_btt *nd_btt;

	if (!is_nd_btt(dev))
		return 0;

	nd_btt = to_nd_btt(dev);
	bdev = nd_btt->backing_dev;
	if (bdev && bdev->bd_disk == disk)
		nd_btt_remove_bdev(nd_btt, __func__);
	return 0;
}

void nd_btt_remove_disk(struct nvdimm_bus *nvdimm_bus, struct gendisk *disk)
{
	device_for_each_child(&nvdimm_bus->dev, disk, __nd_btt_remove_disk);
}

static ssize_t __backing_dev_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	struct nvdimm_bus *nvdimm_bus = walk_to_nvdimm_bus(dev);
	const struct block_device_operations *ops;
	struct nd_btt *nd_btt = to_nd_btt(dev);
	struct block_device *bdev;
	char *path;

	if (dev->driver) {
		dev_dbg(dev, "%s: -EBUSY\n", __func__);
		return -EBUSY;
	}

	path = kstrndup(buf, len, GFP_KERNEL);
	if (!path)
		return -ENOMEM;
	strim(path);

	/* detach the backing device */
	if (strcmp(path, "") == 0) {
		nd_btt_remove_bdev(nd_btt, __func__);
		goto out;
	} else if (nd_btt->backing_dev) {
		dev_dbg(dev, "backing_dev already set\n");
		len = -EBUSY;
		goto out;
	}

	bdev = blkdev_get_by_path(path, nd_btt_devs_mode, nd_btt);
	if (IS_ERR(bdev)) {
		dev_dbg(dev, "open '%s' failed: %ld\n", path, PTR_ERR(bdev));
		len = PTR_ERR(bdev);
		goto out;
	}

	if (nvdimm_bus != walk_to_nvdimm_bus(disk_to_dev(bdev->bd_disk))) {
		dev_dbg(dev, "%s not a descendant of %s\n", path,
				dev_name(&nvdimm_bus->dev));
		blkdev_put(bdev, nd_btt_devs_mode);
		len = -EINVAL;
		goto out;
	}

	if (get_capacity(bdev->bd_disk) < SZ_16M / 512) {
		dev_dbg(dev, "%s too small to host btt\n", path);
		blkdev_put(bdev, nd_btt_devs_mode);
		len = -ENXIO;
		goto out;
	}

	ops = bdev->bd_disk->fops;
	if (!ops->rw_bytes) {
		dev_dbg(dev, "%s does not implement ->rw_bytes()\n", path);
		blkdev_put(bdev, nd_btt_devs_mode);
		len = -EINVAL;
		goto out;
	}

	WARN_ON_ONCE(!is_nvdimm_bus_locked(&nd_btt->dev));
	nd_btt->backing_dev = bdev;

 out:
	kfree(path);
	return len;
}

static ssize_t backing_dev_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	ssize_t rc;

	nvdimm_bus_lock(dev);
	device_lock(dev);
	rc = __backing_dev_store(dev, attr, buf, len);
	dev_dbg(dev, "%s: result: %zd wrote: %s%s", __func__,
			rc, buf, buf[len - 1] == '\n' ? "" : "\n");
	device_unlock(dev);
	nvdimm_bus_unlock(dev);

	return rc;
}
static DEVICE_ATTR_RW(backing_dev);

static bool is_nd_btt_idle(struct device *dev)
{
	struct nvdimm_bus *nvdimm_bus = walk_to_nvdimm_bus(dev);
	struct nd_btt *nd_btt = to_nd_btt(dev);

	if (nvdimm_bus->nd_btt == nd_btt || dev->driver || nd_btt->backing_dev)
		return false;
	return true;
}

static ssize_t delete_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	/* return 1 if can be deleted */
	return sprintf(buf, "%d\n", is_nd_btt_idle(dev));
}

static ssize_t delete_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	unsigned long val;

	/* write 1 to delete */
	if (kstrtoul(buf, 0, &val) != 0 || val != 1)
		return -EINVAL;

	/* prevent deletion while this btt is active, or is the current seed */
	if (!is_nd_btt_idle(dev))
		return -EBUSY;

	/*
	 * userspace raced itself if device goes active here and it gets
	 * to keep the pieces
	 */
	nd_device_unregister(dev, ND_ASYNC);

	return len;
}
static DEVICE_ATTR_RW(delete);

static struct attribute *nd_btt_attributes[] = {
	&dev_attr_sector_size.attr,
	&dev_attr_backing_dev.attr,
	&dev_attr_delete.attr,
	&dev_attr_uuid.attr,
	NULL,
};

static struct attribute_group nd_btt_attribute_group = {
	.attrs = nd_btt_attributes,
};

static const struct attribute_group *nd_btt_attribute_groups[] = {
	&nd_btt_attribute_group,
	&nd_device_attribute_group,
	&nd_numa_attribute_group,
	NULL,
};

static struct nd_btt *__nd_btt_create(struct nvdimm_bus *nvdimm_bus,
		unsigned long lbasize, u8 *uuid, struct block_device *bdev)
{
	struct nd_btt *nd_btt = kzalloc(sizeof(*nd_btt), GFP_KERNEL);
	struct device *dev;

	if (!nd_btt)
		return NULL;
	nd_btt->id = ida_simple_get(&btt_ida, 0, 0, GFP_KERNEL);
	if (nd_btt->id < 0) {
		kfree(nd_btt);
		return NULL;
	}

	nd_btt->lbasize = lbasize;
	if (uuid)
		uuid = kmemdup(uuid, 16, GFP_KERNEL);
	nd_btt->uuid = uuid;
	nd_btt->backing_dev = bdev;
	dev = &nd_btt->dev;
	dev_set_name(dev, "btt%d", nd_btt->id);
	dev->parent = &nvdimm_bus->dev;
	dev->type = &nd_btt_device_type;
	dev->groups = nd_btt_attribute_groups;
	nd_device_register(&nd_btt->dev);
	return nd_btt;
}

struct nd_btt *nd_btt_create(struct nvdimm_bus *nvdimm_bus)
{
	return __nd_btt_create(nvdimm_bus, 0, NULL, NULL);
}

/*
 * nd_btt_sb_checksum: compute checksum for btt info block
 *
 * Returns a fletcher64 checksum of everything in the given info block
 * except the last field (since that's where the checksum lives).
 */
u64 nd_btt_sb_checksum(struct btt_sb *btt_sb)
{
	u64 sum;
	__le64 sum_save;

	sum_save = btt_sb->checksum;
	btt_sb->checksum = 0;
	sum = nd_fletcher64(btt_sb, sizeof(*btt_sb), 1);
	btt_sb->checksum = sum_save;
	return sum;
}
EXPORT_SYMBOL(nd_btt_sb_checksum);

int set_btt_ro(struct block_device *bdev, struct device *dev, int ro)
{
	struct nd_btt *nd_btt = to_nd_btt(dev);

	if (!dev->driver)
		return 0;

	/* we can only mark a btt device rw if its backing device is rw */
	if (bdev_read_only(nd_btt->backing_dev) && !ro)
		return -EBUSY;

	set_device_ro(bdev, ro);
	return 0;
}

int set_btt_disk_ro(struct device *dev, void *data)
{
	struct block_device *bdev = data;
	struct nd_btt *nd_btt;
	struct btt *btt;

	if (!is_nd_btt(dev))
		return 0;

	nd_btt = to_nd_btt(dev);
	if (nd_btt->backing_dev != bdev)
		return 0;

	/*
	 * We have the lock at this point and have flushed probing.  We
	 * are guaranteed that the btt driver is unbound, or has
	 * completed setup operations and is blocked from initiating
	 * disk teardown until we are done walking these pointers.
	 */
	btt = dev_get_drvdata(dev);
	if (btt && btt->btt_disk)
		set_disk_ro(btt->btt_disk, 1);
	return 0;
}

static struct nd_btt *__nd_btt_autodetect(struct nvdimm_bus *nvdimm_bus,
		struct block_device *bdev, struct btt_sb *btt_sb)
{
	u64 checksum;
	u32 lbasize;

	if (!btt_sb || !bdev || !nvdimm_bus)
		return NULL;

	if (bdev_read_bytes(bdev, SZ_4K, btt_sb, sizeof(*btt_sb)))
		return NULL;

	if (get_capacity(bdev->bd_disk) < SZ_16M / 512)
		return NULL;

	if (memcmp(btt_sb->signature, BTT_SIG, BTT_SIG_LEN) != 0)
		return NULL;

	checksum = le64_to_cpu(btt_sb->checksum);
	btt_sb->checksum = 0;
	if (checksum != nd_btt_sb_checksum(btt_sb))
		return NULL;
	btt_sb->checksum = cpu_to_le64(checksum);

	lbasize = le32_to_cpu(btt_sb->external_lbasize);
	return __nd_btt_create(nvdimm_bus, lbasize, btt_sb->uuid, bdev);
}

static int nd_btt_autodetect(struct nvdimm_bus *nvdimm_bus,
		struct block_device *bdev)
{
	char name[BDEVNAME_SIZE];
	struct nd_btt *nd_btt;
	struct btt_sb *btt_sb;

	btt_sb = kzalloc(sizeof(*btt_sb), GFP_KERNEL);
	nd_btt = __nd_btt_autodetect(nvdimm_bus, bdev, btt_sb);
	kfree(btt_sb);
	dev_dbg(&nvdimm_bus->dev, "%s: %s btt: %s\n", __func__,
			bdevname(bdev, name), nd_btt
			? dev_name(&nd_btt->dev) : "<none>");
	return nd_btt ? 0 : -ENODEV;
}

void nd_btt_add_disk(struct nvdimm_bus *nvdimm_bus, struct gendisk *disk)
{
	struct disk_part_iter piter;
	struct hd_struct *part;

	disk_part_iter_init(&piter, disk, DISK_PITER_INCL_PART0);
	while ((part = disk_part_iter_next(&piter))) {
		struct block_device *bdev;
		int rc;

		bdev = bdget_disk(disk, part->partno);
		if (!bdev)
			continue;
		if (blkdev_get(bdev, nd_btt_devs_mode, nvdimm_bus) != 0)
			continue;
		rc = nd_btt_autodetect(nvdimm_bus, bdev);
		if (rc)
			blkdev_put(bdev, nd_btt_devs_mode);
		/* no need to scan further in the case of whole disk btt */
		if (rc == 0 && part->partno == 0)
			break;
	}
	disk_part_iter_exit(&piter);
}
