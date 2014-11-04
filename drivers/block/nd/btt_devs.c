/*
 * Copyright(c) 2013-2014 Intel Corporation. All rights reserved.
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
#include <linux/device.h>
#include <linux/ctype.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include "nd-private.h"
#include "nd.h"

static DEFINE_IDA(btt_ida);

void nd_btt_release(struct device *dev)
{
	struct nd_btt *nd_btt = to_nd_btt(dev);

	WARN_ON(nd_btt->backing_dev);
	ndio_del_claim(nd_btt->ndio_claim);
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

static const unsigned long lbasize_supported[] = { 512, };

static ssize_t sector_size_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nd_btt *nd_btt = to_nd_btt(dev);
	ssize_t len = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(lbasize_supported); i++)
		if (nd_btt->lbasize == lbasize_supported[i])
			len += sprintf(buf + len, "[%ld] ",
					lbasize_supported[i]);
		else
			len += sprintf(buf + len, "%ld ",
					lbasize_supported[i]);
	len += sprintf(buf + len, "\n");
	return len;
}

static ssize_t __sector_size_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	struct nd_btt *nd_btt = to_nd_btt(dev);
	unsigned long lbasize;
	int rc, i;

	if (dev->driver) {
		dev_dbg(dev, "%s: -EBUSY\n", __func__);
		return -EBUSY;
	}

	rc = kstrtoul(buf, 0, &lbasize);
	if (rc)
		return rc;

	for (i = 0; i < ARRAY_SIZE(lbasize_supported); i++)
		if (lbasize == lbasize_supported[i])
			break;

	if (i < ARRAY_SIZE(lbasize_supported)) {
		nd_btt->lbasize = lbasize;
		return len;
	}

	return -EINVAL;
}

static ssize_t sector_size_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	ssize_t rc;

	device_lock(dev);
	rc = __sector_size_store(dev, attr, buf, len);
	device_unlock(dev);

	return rc;
}
static DEVICE_ATTR_RW(sector_size);

static ssize_t uuid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nd_btt *nd_btt = to_nd_btt(dev);
	unsigned short field3, field2, field1;
	unsigned long long field0 = 0;
	unsigned long field4;

	if (!nd_btt->uuid)
		return sprintf(buf, "\n");

	memcpy(&field0, nd_btt->uuid, 6);
	memcpy(&field1, &nd_btt->uuid[6], 2);
	memcpy(&field2, &nd_btt->uuid[8], 2);
	memcpy(&field3, &nd_btt->uuid[10], 2);
	memcpy(&field4, &nd_btt->uuid[12], 4);

	return sprintf(buf, "%.4lx-%.2x-%.2x-%.2x-%.6llx\n",
			field4, field3, field2, field1, field0);
}

static bool is_uuid_sep(char sep)
{
	if (sep == '\n' || sep == '-' || sep == ':' || sep == '\0')
		return true;
	return false;
}

static ssize_t __uuid_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	struct nd_btt *nd_btt = to_nd_btt(dev);
	unsigned long long uuid[2];
	char field_str[2][17];
	char uuid_str[32];
	int rc, pos;
	size_t i;

	if (dev->driver) {
		dev_dbg(dev, "%s: -EBUSY\n", __func__);
		return -EBUSY;
	}

	for (pos = 0, i = 0; i < len && pos < sizeof(uuid_str); i++) {
		if (isxdigit(buf[i]))
			uuid_str[pos++] = buf[i];
		else if (!is_uuid_sep(buf[i]))
			break;
	}

	if (pos < sizeof(uuid_str) || !is_uuid_sep(buf[i])) {
		dev_dbg(dev, "%s: pos: %d buf[%zd]: %c\n",
				__func__, pos, i, buf[i]);
		return -EINVAL;
	}

	memcpy(field_str[1], uuid_str, 16);
	field_str[1][16] = '\0';
	memcpy(field_str[0], &uuid_str[16], 16);
	field_str[0][16] = '\0';

	rc = kstrtoull(field_str[1], 16, &uuid[1]);
	if (rc)
		return -EINVAL;
	rc = kstrtoull(field_str[0], 16, &uuid[0]);
	if (rc)
		return -EINVAL;

	kfree(nd_btt->uuid);
	nd_btt->uuid = kmemdup(uuid, 16, GFP_KERNEL);
	if (!nd_btt->uuid)
		return -ENOMEM;

	return len;
}

static ssize_t uuid_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	ssize_t rc;

	device_lock(dev);
	rc = __uuid_store(dev, attr, buf, len);
	device_unlock(dev);

	return rc;
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

static const fmode_t nd_btt_devs_mode = FMODE_READ | FMODE_WRITE | FMODE_EXCL;

static void nd_btt_ndio_notify_remove(struct nd_io_claim *ndio_claim)
{
	char bdev_name[BDEVNAME_SIZE];
	struct nd_btt *nd_btt;

	if (!ndio_claim || !ndio_claim->holder)
		return;

	nd_btt = to_nd_btt(ndio_claim->holder);
	WARN_ON_ONCE(!is_nd_bus_locked(&nd_btt->dev));
	dev_dbg(&nd_btt->dev, "%pf: %s: release /dev/%s\n",
			__builtin_return_address(0), __func__,
			bdevname(nd_btt->backing_dev, bdev_name));
	blkdev_put(nd_btt->backing_dev, nd_btt_devs_mode);
	nd_btt->backing_dev = NULL;

	/*
	 * Once we've had our backing device removed we need to be fully
	 * reconfigured.  The bus will have already created a new seed
	 * for this purpose, so now is a good time to clean up this
	 * stale nd_btt instance.
	 */
	if (nd_btt->dev.driver)
		nd_device_unregister(&nd_btt->dev, ND_ASYNC);
	else
		ndio_del_claim(ndio_claim);
}

static ssize_t __backing_dev_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	struct nd_bus *nd_bus = walk_to_nd_bus(dev);
	struct nd_btt *nd_btt = to_nd_btt(dev);
	char bdev_name[BDEVNAME_SIZE];
	struct block_device *bdev;
	struct nd_io *ndio;
	char *path;

	if (dev->driver) {
		dev_dbg(dev, "%s: -EBUSY\n", __func__);
		return -EBUSY;
	}

	path = kstrndup(buf, len, GFP_KERNEL);
	if (!path)
		return -ENOMEM;

	/* detach the backing device */
	if (strcmp(strim(path), "") == 0) {
		if (!nd_btt->backing_dev)
			goto out;
		nd_btt_ndio_notify_remove(nd_btt->ndio_claim);
		goto out;
	} else if (nd_btt->backing_dev) {
		dev_dbg(dev, "backing_dev already set\n");
		len = -EBUSY;
		goto out;
	}

	bdev = blkdev_get_by_path(strim(path), nd_btt_devs_mode, nd_btt);
	if (IS_ERR(bdev)) {
		dev_dbg(dev, "open '%s' failed: %ld\n", strim(path),
				PTR_ERR(bdev));
		len = PTR_ERR(bdev);
		goto out;
	}

	ndio = ndio_lookup(nd_bus, bdevname(bdev->bd_contains, bdev_name));
	if (!ndio) {
		dev_dbg(dev, "%s does not have an ndio interface\n",
				strim(path));
		blkdev_put(bdev, nd_btt_devs_mode);
		len = -ENXIO;
		goto out;
	}

	nd_btt->ndio_claim = ndio_add_claim(ndio, &nd_btt->dev,
			nd_btt_ndio_notify_remove);
	if (!nd_btt->ndio_claim) {
		blkdev_put(bdev, nd_btt_devs_mode);
		len = -ENOMEM;
		goto out;
	}

	WARN_ON_ONCE(!is_nd_bus_locked(&nd_btt->dev));
	nd_btt->backing_dev = bdev;

 out:
	kfree(path);
	return len;
}

static ssize_t backing_dev_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	ssize_t rc;

	nd_bus_lock(dev);
	device_lock(dev);
	rc = __backing_dev_store(dev, attr, buf, len);
	device_unlock(dev);
	nd_bus_unlock(dev);

	return rc;
}
static DEVICE_ATTR_RW(backing_dev);

static bool is_nd_btt_idle(struct device *dev)
{
	struct nd_bus *nd_bus = walk_to_nd_bus(dev);
	struct nd_btt *nd_btt = to_nd_btt(dev);

	if (nd_bus->nd_btt == nd_btt || dev->driver || nd_btt->backing_dev)
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
	NULL,
};

struct nd_btt *nd_btt_create(struct nd_bus *nd_bus, struct block_device *bdev,
		struct nd_io_claim *ndio_claim, unsigned long lbasize,
		u8 uuid[16])
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

	nd_btt->ndio_claim = ndio_claim;
	nd_btt->lbasize = lbasize;
	nd_btt->backing_dev = bdev;
	if (uuid)
		uuid = kmemdup(uuid, 16, GFP_KERNEL);
	nd_btt->uuid = uuid;
	dev = &nd_btt->dev;
	dev_set_name(dev, "btt%d", nd_btt->id);
	dev->parent = &nd_bus->dev;
	dev->type = &nd_btt_device_type;
	dev->groups = nd_btt_attribute_groups;
	nd_device_register(dev, ND_ASYNC);

	return nd_btt;
}
