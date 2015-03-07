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
#include <linux/scatterlist.h>
#include <linux/highmem.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/sort.h>
#include <linux/io.h>
#include <linux/nd.h>
#include "nd-private.h"
#include "nfit.h"
#include "nd.h"

static struct {
	struct {
		int count[CONFIG_ND_MAX_REGIONS];
		spinlock_t lock[CONFIG_ND_MAX_REGIONS];
	} lane[NR_CPUS];
} nd_percpu_lane;

static void nd_region_init_locks(void)
{
	int i, j;

	for (i = 0; i < NR_CPUS; i++)
		for (j = 0; j < CONFIG_ND_MAX_REGIONS; j++)
			spin_lock_init(&nd_percpu_lane.lane[i].lock[j]);
}

/**
 * nd_region_acquire_lane - allocate and lock a lane
 * @nd_region: region id and number of lanes possible
 *
 * A lane correlates to a BLK-data-window and/or a log slot in the BTT.
 * We optimize for the common case where there are 256 lanes, one
 * per-cpu.  For larger systems we need to lock to share lanes.  For now
 * this implementation assumes the cost of maintaining an allocator for
 * free lanes is on the order of the lock hold time, so it implements a
 * static lane = cpu % num_lanes mapping.
 *
 * In the case of a BTT instance on top of a BLK namespace a lane may be
 * acquired recursively.  We lock on the first instance.
 *
 * In the case of a BTT instance on top of PMEM, we only acquire a lane
 * for the BTT metadata updates.
 */
unsigned int nd_region_acquire_lane(struct nd_region *nd_region)
{
	unsigned int cpu, lane;

	cpu = get_cpu();

	if (nd_region->num_lanes < NR_CPUS) {
		unsigned int id = nd_region->id;

		lane = cpu % nd_region->num_lanes;
		if (nd_percpu_lane.lane[cpu].count[id]++ == 0)
			spin_lock(&nd_percpu_lane.lane[lane].lock[id]);
	} else
		lane = cpu;

	return lane;
}
EXPORT_SYMBOL(nd_region_acquire_lane);

void nd_region_release_lane(struct nd_region *nd_region, unsigned int lane)
{
	if (nd_region->num_lanes < NR_CPUS) {
		unsigned int cpu = get_cpu();
		unsigned int id = nd_region->id;

		if (--nd_percpu_lane.lane[cpu].count[id] == 0)
			spin_unlock(&nd_percpu_lane.lane[lane].lock[id]);
		put_cpu();
	}
	put_cpu();
}
EXPORT_SYMBOL(nd_region_release_lane);

static DEFINE_IDA(region_ida);

static void nd_region_release(struct device *dev)
{
	struct nd_region *nd_region = to_nd_region(dev);
	u16 i;

	for (i = 0; i < nd_region->ndr_mappings; i++) {
		struct nd_mapping *nd_mapping = &nd_region->mapping[i];
		struct nd_dimm *nd_dimm = nd_mapping->nd_dimm;

		put_device(&nd_dimm->dev);
	}
	ida_simple_remove(&region_ida, nd_region->id);
	kfree(nd_region);
}

static struct device_type nd_block_device_type = {
	.name = "nd_blk",
	.release = nd_region_release,
};

static struct device_type nd_pmem_device_type = {
	.name = "nd_pmem",
	.release = nd_region_release,
};

static struct device_type nd_volatile_device_type = {
	.name = "nd_volatile",
	.release = nd_region_release,
};

bool is_nd_pmem(struct device *dev)
{
	return dev ? dev->type == &nd_pmem_device_type : false;
}

bool is_nd_blk(struct device *dev)
{
	return dev ? dev->type == &nd_block_device_type : false;
}

struct nd_region *to_nd_region(struct device *dev)
{
	struct nd_region *nd_region = container_of(dev, struct nd_region, dev);

	WARN_ON(dev->type->release != nd_region_release);
	return nd_region;
}
EXPORT_SYMBOL(to_nd_region);

/**
 * nd_region_to_namespace_type() - region to an integer namespace type
 * @nd_region: region-device to interrogate
 *
 * This is the 'nstype' attribute of a region as well, an input to the
 * MODALIAS for namespace devices, and bit number for a nd_bus to match
 * namespace devices with namespace drivers.
 */
int nd_region_to_namespace_type(struct nd_region *nd_region)
{
	if (is_nd_pmem(&nd_region->dev)) {
		if (nd_region->ndr_mappings)
			return ND_DEVICE_NAMESPACE_PMEM;
		else
			return ND_DEVICE_NAMESPACE_IO;
	} else if (is_nd_blk(&nd_region->dev)) {
		return ND_DEVICE_NAMESPACE_BLOCK;
	}

	return 0;
}
EXPORT_SYMBOL(nd_region_to_namespace_type);

static int is_uuid_busy(struct device *dev, void *data)
{
	struct nd_region *nd_region = to_nd_region(dev->parent);
	u8 *uuid = data;

	switch (nd_region_to_namespace_type(nd_region)) {
	case ND_DEVICE_NAMESPACE_PMEM: {
		struct nd_namespace_pmem *nspm = to_nd_namespace_pmem(dev);

		if (!nspm->uuid)
			break;
		if (memcmp(uuid, nspm->uuid, NSLABEL_UUID_LEN) == 0)
			return -EBUSY;
		break;
	}
	case ND_DEVICE_NAMESPACE_BLOCK: {
		struct nd_namespace_blk *nsblk = to_nd_namespace_blk(dev);

		if (!nsblk->uuid)
			break;
		if (memcmp(uuid, nsblk->uuid, NSLABEL_UUID_LEN) == 0)
			return -EBUSY;
		break;
	}
	default:
		break;
	}

	return 0;
}

static int for_each_namespace(struct device *dev, void *data)
{
	if (is_nd_pmem(dev) || is_nd_blk(dev))
		return device_for_each_child(dev, data, is_uuid_busy);
	return 0;
}

/**
 * nd_is_uuid_unique - verify that no other namespace has @uuid
 * @dev: any device on a nd_bus
 * @uuid: uuid to check
 */
bool nd_is_uuid_unique(struct device *dev, u8 *uuid)
{
	struct nd_bus *nd_bus = walk_to_nd_bus(dev);

	if (!nd_bus)
		return false;
	WARN_ON_ONCE(!is_nd_bus_locked(&nd_bus->dev));
	if (device_for_each_child(&nd_bus->dev, uuid, for_each_namespace) != 0)
		return false;
	return true;
}

static ssize_t size_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nd_region *nd_region = to_nd_region(dev);
	unsigned long long size = 0;

	if (is_nd_pmem(dev)) {
		size = nd_region->ndr_size;
	} else if (nd_region->ndr_mappings == 1) {
		struct nd_mapping *nd_mapping = &nd_region->mapping[0];

		size = nd_mapping->size;
	}

	return sprintf(buf, "%llu\n", size);
}
static DEVICE_ATTR_RO(size);

static ssize_t mappings_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nd_region *nd_region = to_nd_region(dev);

	return sprintf(buf, "%d\n", nd_region->ndr_mappings);
}
static DEVICE_ATTR_RO(mappings);

static ssize_t spa_index_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nd_region *nd_region = to_nd_region(dev);
	struct nd_spa *nd_spa = nd_region->nd_spa;
	struct nd_bus *nd_bus = walk_to_nd_bus(dev);
	u16 spa_index = nfit_spa_index(nd_bus->nfit_desc, nd_spa->nfit_spa);

	return sprintf(buf, "%d\n", spa_index);
}
static DEVICE_ATTR_RO(spa_index);

static ssize_t nstype_show(struct device *dev,
                struct device_attribute *attr, char *buf)
{
	struct nd_region *nd_region = to_nd_region(dev);

	return sprintf(buf, "%d\n", nd_region_to_namespace_type(nd_region));
}
static DEVICE_ATTR_RO(nstype);

static ssize_t set_cookie_show(struct device *dev,
                struct device_attribute *attr, char *buf)
{
	struct nd_region *nd_region = to_nd_region(dev);
	struct nd_spa *nd_spa = nd_region->nd_spa;

	if (is_nd_pmem(dev) && nd_spa->nd_set)
		/* pass, should be precluded by nd_region_visible */;
	else
		return -ENXIO;

	return sprintf(buf, "%#llx\n", nd_spa->nd_set->cookie);
}
static DEVICE_ATTR_RO(set_cookie);

resource_size_t nd_region_available_dpa(struct nd_region *nd_region)
{
	resource_size_t blk_max_overlap = 0, available, next_overlap;
	int i;

	WARN_ON(!is_nd_bus_locked(&nd_region->dev));

 retry:
	available = 0;
	next_overlap = blk_max_overlap;
	for (i = 0; i < nd_region->ndr_mappings; i++) {
		struct nd_mapping *nd_mapping = &nd_region->mapping[i];

		if (is_nd_pmem(&nd_region->dev)) {
			available += nd_pmem_available_dpa(nd_region,
					nd_mapping, &next_overlap);
			if (next_overlap > blk_max_overlap) {
				blk_max_overlap = next_overlap;
				goto retry;
			}
		} else if (is_nd_blk(&nd_region->dev)) {
			available += nd_blk_available_dpa(nd_mapping);
		}
	}

	return available;
}

static ssize_t available_size_show(struct device *dev,
                struct device_attribute *attr, char *buf)
{
	struct nd_region *nd_region = to_nd_region(dev);
	unsigned long long available = 0;

	/*
	 * Flush in-flight updates and grab a snapshot of the available
	 * size.  Of course, this value is potentially invalidated the
	 * memory nd_bus_lock() is dropped, but that's userspace's
	 * problem to not race itself.
	 */
	nd_bus_lock(dev);
	wait_nd_bus_probe_idle(dev);
	available = nd_region_available_dpa(nd_region);
	nd_bus_unlock(dev);

	return sprintf(buf, "%llu\n", available);
}
static DEVICE_ATTR_RO(available_size);

static ssize_t init_namespaces_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nd_region_namespaces *num_ns = dev_get_drvdata(dev);

	if (!num_ns)
		return -ENXIO;

	return sprintf(buf, "%d/%d\n", num_ns->active, num_ns->count);
}
static DEVICE_ATTR_RO(init_namespaces);

static ssize_t namespace_seed_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nd_region *nd_region = to_nd_region(dev);
	ssize_t rc;

	nd_bus_lock(dev);
	if (nd_region->ns_seed)
		rc = sprintf(buf, "%s\n", dev_name(nd_region->ns_seed));
	else
		rc = sprintf(buf, "\n");
	nd_bus_unlock(dev);
	return rc;
}
static DEVICE_ATTR_RO(namespace_seed);

static struct attribute *nd_region_attributes[] = {
	&dev_attr_size.attr,
	&dev_attr_nstype.attr,
	&dev_attr_mappings.attr,
	&dev_attr_spa_index.attr,
	&dev_attr_set_cookie.attr,
	&dev_attr_available_size.attr,
	&dev_attr_namespace_seed.attr,
	&dev_attr_init_namespaces.attr,
	NULL,
};

static umode_t nd_region_visible(struct kobject *kobj, struct attribute *a, int n)
{
	struct device *dev = container_of(kobj, typeof(*dev), kobj);
	struct nd_region *nd_region = to_nd_region(dev);
	int type = nd_region_to_namespace_type(nd_region);
	struct nd_spa *nd_spa = nd_region->nd_spa;

	if (a != &dev_attr_set_cookie.attr && a != &dev_attr_available_size.attr)
		return a->mode;

	if ((type == ND_DEVICE_NAMESPACE_PMEM
				|| type == ND_DEVICE_NAMESPACE_BLOCK)
			&& a == &dev_attr_available_size.attr)
		return a->mode;
	else if (is_nd_pmem(dev) && nd_spa->nd_set)
		return a->mode;

	return 0;
}

static struct attribute_group nd_region_attribute_group = {
	.attrs = nd_region_attributes,
	.is_visible = nd_region_visible,
};

/*
 * Retrieve the nth entry referencing this spa, for pm there may be not only
 * multiple per device in the interleave, but multiple per-dimm for each region
 * of the dimm that maps into the interleave.
 */
static struct nd_memdev *nd_memdev_from_spa(struct nd_bus *nd_bus,
		u16 spa_index, int n)
{
	struct nd_memdev *nd_memdev;

	list_for_each_entry(nd_memdev, &nd_bus->memdevs, list)
		if (readw(&nd_memdev->nfit_mem->spa_index) == spa_index)
			if (n-- == 0)
				return nd_memdev;
	return NULL;
}

static int num_nd_mem(struct nd_bus *nd_bus, u16 spa_index)
{
	struct nd_memdev *nd_memdev;
	int count = 0;

	list_for_each_entry(nd_memdev, &nd_bus->memdevs, list)
		if (readw(&nd_memdev->nfit_mem->spa_index) == spa_index)
			count++;
	return count;
}

/* enough info to uniquely specify an interleave set */
struct nd_set_info {
	struct nd_set_info_map {
		u64 region_spa_offset;
		u32 serial_number;
	} mapping[0];
};

static size_t sizeof_nd_set_info(int num_mappings)
{
	return sizeof(struct nd_set_info)
		+ num_mappings * sizeof(struct nd_set_info_map);
}

static int cmp_map(const void *m0, const void *m1)
{
	const struct nd_set_info_map *map0 = m0;
	const struct nd_set_info_map *map1 = m1;

	return memcmp(&map0->region_spa_offset, &map1->region_spa_offset,
			sizeof(u64));
}

/* convert and anoymous MEMDEV to its set of associated tables */
static struct nd_mem *nd_memdev_to_mem(struct nd_bus *nd_bus,
		struct nd_memdev *nd_memdev)
{
	u32 nfit_handle = readl(&nd_memdev->nfit_mem->nfit_handle);
	struct nd_mem *nd_mem;

	list_for_each_entry(nd_mem, &nd_bus->dimms, dimms)
		if (readl(&nd_mem->nfit_mem->nfit_handle) == nfit_handle)
			return nd_mem;
	return NULL;
}

static int init_interleave_set(struct nd_bus *nd_bus,
		struct nd_interleave_set *nd_set, struct nd_spa *nd_spa)
{
	u16 spa_index = nfit_spa_index(nd_bus->nfit_desc, nd_spa->nfit_spa);
	int num_mappings = num_nd_mem(nd_bus, spa_index);
	struct nd_set_info *info;
	int i;

	info = kzalloc(sizeof_nd_set_info(num_mappings), GFP_KERNEL);
	if (!info)
		return -ENOMEM;
	for (i = 0; i < num_mappings; i++) {
		struct nd_set_info_map *map = &info->mapping[i];
		struct nd_memdev *nd_memdev = nd_memdev_from_spa(nd_bus,
				spa_index, i);
		struct nd_mem *nd_mem = nd_memdev_to_mem(nd_bus, nd_memdev);

		if (!nd_mem) {
			dev_err(&nd_bus->dev, "%s: failed to find DCR\n",
					__func__);
			kfree(info);
			return -ENODEV;
		}

		map->region_spa_offset = readl(
				&nd_memdev->nfit_mem->region_spa_offset);
		map->serial_number = nfit_dcr_serial(nd_bus->nfit_desc,
				nd_mem->nfit_dcr, nd_mem->nfit_mem);
	}

	sort(&info->mapping[0], num_mappings, sizeof(struct nd_set_info_map),
			cmp_map, NULL);
	nd_set->cookie = nd_fletcher64(info, sizeof_nd_set_info(num_mappings));

	kfree(info);

	return 0;
}

int nd_bus_init_interleave_sets(struct nd_bus *nd_bus)
{
	struct nd_spa *nd_spa;
	int rc = 0;

	/* PMEM interleave sets */
	list_for_each_entry(nd_spa, &nd_bus->spas, list) {
		u16 spa_index = nfit_spa_index(nd_bus->nfit_desc, nd_spa->nfit_spa);
		int spa_type = nfit_spa_type(nd_bus->nfit_desc, nd_spa->nfit_spa);
		struct nd_interleave_set *nd_set;

		if (spa_type != NFIT_SPA_PM)
			continue;
		if (nd_memdev_from_spa(nd_bus, spa_index, 0) == NULL)
			continue;
		nd_set = kzalloc(sizeof(*nd_set), GFP_KERNEL);
		if (!nd_set) {
			rc = -ENOMEM;
			break;
		}
		nd_spa->nd_set = nd_set;

		rc = init_interleave_set(nd_bus, nd_set, nd_spa);
		if (rc)
			break;
	}

	return rc;
}

u64 nd_region_interleave_set_cookie(struct nd_region *nd_region)
{
	struct nd_spa *nd_spa = nd_region->nd_spa;
	struct nd_interleave_set *nd_set = nd_spa ? nd_spa->nd_set : NULL;

	if (nd_set)
		return nd_set->cookie;
	return 0;
}

/*
 * Upon successful probe/remove, take/release a reference on the
 * associated interleave set (if present)
 */
static void nd_region_notify_driver_action(struct nd_bus *nd_bus,
		struct device *dev, int rc, bool probe)
{
	if (rc)
		return;

	if (is_nd_pmem(dev) || is_nd_blk(dev)) {
		struct nd_region *nd_region = to_nd_region(dev);
		int i;

		for (i = 0; i < nd_region->ndr_mappings; i++) {
			struct nd_mapping *nd_mapping = &nd_region->mapping[i];
			struct nd_dimm *nd_dimm = nd_mapping->nd_dimm;

			if (probe)
				atomic_inc(&nd_dimm->busy);
			else
				atomic_dec(&nd_dimm->busy);
		}
	} else if (dev->parent && is_nd_blk(dev->parent) && probe && rc == 0) {
		struct nd_region *nd_region = to_nd_region(dev->parent);

		if (nd_region->ns_seed == dev)
			nd_region_create_blk_seed(nd_region);
	}
}

void nd_region_probe_start(struct nd_bus *nd_bus, struct device *dev)
{
	nd_bus_lock(&nd_bus->dev);
	nd_bus->probe_active++;
	nd_bus_unlock(&nd_bus->dev);
}

void nd_region_probe_end(struct nd_bus *nd_bus, struct device *dev, int rc)
{
	nd_bus_lock(&nd_bus->dev);
	nd_region_notify_driver_action(nd_bus, dev, rc, true);
	if (--nd_bus->probe_active == 0)
		wake_up(&nd_bus->probe_wait);
	nd_bus_unlock(&nd_bus->dev);
}

void nd_region_notify_remove(struct nd_bus *nd_bus, struct device *dev, int rc)
{
	nd_bus_lock(dev);
	nd_region_notify_driver_action(nd_bus, dev, rc, false);
	nd_bus_unlock(dev);
}

static ssize_t mappingN(struct device *dev, char *buf, int n)
{
	struct nd_region *nd_region = to_nd_region(dev);
	struct nfit_mem __iomem *nfit_mem;
	struct nd_mapping *nd_mapping;
	struct nd_dimm *nd_dimm;

	if (n >= nd_region->ndr_mappings)
		return -ENXIO;
	nd_mapping = &nd_region->mapping[n];
	nd_dimm = nd_mapping->nd_dimm;
	nfit_mem = nd_dimm->nd_mem->nfit_mem;

	return sprintf(buf, "%#x,%llu,%llu\n", readl(&nfit_mem->nfit_handle),
			nd_mapping->start, nd_mapping->size);
}

#define REGION_MAPPING(idx) \
static ssize_t mapping##idx##_show(struct device *dev,		\
		struct device_attribute *attr, char *buf)	\
{								\
	return mappingN(dev, buf, idx);				\
}								\
static DEVICE_ATTR_RO(mapping##idx)

/*
 * 32 should be enough for a while, even in the presence of socket
 * interleave a 32-way interleave set is a degenerate case.
 */
REGION_MAPPING(0);
REGION_MAPPING(1);
REGION_MAPPING(2);
REGION_MAPPING(3);
REGION_MAPPING(4);
REGION_MAPPING(5);
REGION_MAPPING(6);
REGION_MAPPING(7);
REGION_MAPPING(8);
REGION_MAPPING(9);
REGION_MAPPING(10);
REGION_MAPPING(11);
REGION_MAPPING(12);
REGION_MAPPING(13);
REGION_MAPPING(14);
REGION_MAPPING(15);
REGION_MAPPING(16);
REGION_MAPPING(17);
REGION_MAPPING(18);
REGION_MAPPING(19);
REGION_MAPPING(20);
REGION_MAPPING(21);
REGION_MAPPING(22);
REGION_MAPPING(23);
REGION_MAPPING(24);
REGION_MAPPING(25);
REGION_MAPPING(26);
REGION_MAPPING(27);
REGION_MAPPING(28);
REGION_MAPPING(29);
REGION_MAPPING(30);
REGION_MAPPING(31);

static umode_t nd_mapping_visible(struct kobject *kobj, struct attribute *a, int n)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct nd_region *nd_region = to_nd_region(dev);

	if (n < nd_region->ndr_mappings)
		return a->mode;
	return 0;
}

static struct attribute *nd_mapping_attributes[] = {
	&dev_attr_mapping0.attr,
	&dev_attr_mapping1.attr,
	&dev_attr_mapping2.attr,
	&dev_attr_mapping3.attr,
	&dev_attr_mapping4.attr,
	&dev_attr_mapping5.attr,
	&dev_attr_mapping6.attr,
	&dev_attr_mapping7.attr,
	&dev_attr_mapping8.attr,
	&dev_attr_mapping9.attr,
	&dev_attr_mapping10.attr,
	&dev_attr_mapping11.attr,
	&dev_attr_mapping12.attr,
	&dev_attr_mapping13.attr,
	&dev_attr_mapping14.attr,
	&dev_attr_mapping15.attr,
	&dev_attr_mapping16.attr,
	&dev_attr_mapping17.attr,
	&dev_attr_mapping18.attr,
	&dev_attr_mapping19.attr,
	&dev_attr_mapping20.attr,
	&dev_attr_mapping21.attr,
	&dev_attr_mapping22.attr,
	&dev_attr_mapping23.attr,
	&dev_attr_mapping24.attr,
	&dev_attr_mapping25.attr,
	&dev_attr_mapping26.attr,
	&dev_attr_mapping27.attr,
	&dev_attr_mapping28.attr,
	&dev_attr_mapping29.attr,
	&dev_attr_mapping30.attr,
	&dev_attr_mapping31.attr,
	NULL,
};

static struct attribute_group nd_mapping_attribute_group = {
	.is_visible = nd_mapping_visible,
	.attrs = nd_mapping_attributes,
};

static const struct attribute_group *nd_region_attribute_groups[] = {
	&nd_region_attribute_group,
	&nd_device_attribute_group,
	&nd_mapping_attribute_group,
	NULL,
};

struct nd_blk_ctl *blk_ctl;
#define NUM_BW 256
#define CL_SHIFT 6
static phys_addr_t bw_apt_phys = 0xf000a00000;
static phys_addr_t bw_ctl_phys = 0xf000800000;
static void *bw_apt;
static __iomem u64 *bw_ctl;
static size_t bw_size = SZ_8K * NUM_BW;

enum {
	BCW_OFFSET_MASK = (1ULL << 48)-1,
	BCW_LEN_SHIFT = 48,
	BCW_LEN_MASK = (1ULL << 8) - 1,
	BCW_CMD_SHIFT = 56,
};

bool is_acpi_blk(struct device *dev)
{
	if (strcmp(nd_blk_bus_provider(dev), "OLD_ACPI.NFIT") == 0)
		return true;
	else if (strcmp(nd_blk_bus_provider(dev), "ACPI.NFIT") == 0)
		return true;

	return false;
}
EXPORT_SYMBOL(is_acpi_blk);

/* for now, hard code index 0 */
/* for NT stores, check out __copy_user_nocache() */
static void nd_blk_write_blk_ctl(struct nd_blk_ctl *blk_ctl,
		resource_size_t dev_offset, unsigned int len, bool write)
{
	u64 cmd	= 0;
	u64 cl_offset = dev_offset >> CL_SHIFT;
	u64 cl_len = len >> CL_SHIFT;

	cmd |= cl_offset & BCW_OFFSET_MASK;
	cmd |= (cl_len & BCW_LEN_MASK) << BCW_LEN_SHIFT;
	if (write)
		cmd |= 1ULL << BCW_CMD_SHIFT;

	writeq(cmd, blk_ctl->bw_ctl);
	clflushopt(blk_ctl->bw_ctl);
}

static int nd_blk_read_blk_win(struct nd_blk_ctl *blk_ctl, void *dst,
		unsigned int len)
{
	u32 status;

	/* FIXME: NT */
	memcpy(dst, blk_ctl->bw_apt, len);
	clflushopt(blk_ctl->bw_apt);

	status = readl(blk_ctl->bw_stat);

	if (status) {
		/* FIXME: return more precise error values at some point */
		return -EIO;
	}

	return 0;
}

static int nd_blk_write_blk_win(struct nd_blk_ctl *blk_ctl, void *src,
		unsigned int len)
{
	/* non-temporal writes, need to flush via flush hints, yada yada. */
	u32 status;

	/* FIXME: NT */
	memcpy(blk_ctl->bw_apt, src, len);

	status = readl(blk_ctl->bw_stat);

	if (status) {
		/* FIXME: return more precise error values at some point */
		return -EIO;
	}

	return 0;
}

static int nd_blk_read(struct nd_blk_ctl *blk_ctl, void *dst,
		resource_size_t dev_offset, unsigned int len)
{
	nd_blk_write_blk_ctl(blk_ctl, dev_offset, len, false);
	return nd_blk_read_blk_win(blk_ctl, dst, len);
}

static int nd_blk_write(struct nd_blk_ctl *blk_ctl, void *src,
		resource_size_t dev_offset, unsigned int len)
{
	nd_blk_write_blk_ctl(blk_ctl, dev_offset, len, true);
	return nd_blk_write_blk_win(blk_ctl, src, len);
}

/* len is <= PAGE_SIZE by this point, so it can be done in a single BW I/O */
int nd_blk_do_io(struct nd_blk_window *ndbw, struct page *page,
		unsigned int len, unsigned int off, int rw,
		resource_size_t dev_offset)
{
	struct nd_region *nd_region = ndbw_to_region(ndbw);
	void *mem = kmap_atomic(page);
	struct nd_blk_ctl *blk_ctl;
	unsigned bw_index;
	int rc;

	bw_index = nd_region_acquire_lane(nd_region);
	blk_ctl = &ndbw->blk_ctl[bw_index];

	if (rw == READ)
		rc = nd_blk_read(blk_ctl, mem + off, dev_offset, len);
	else
		rc = nd_blk_write(blk_ctl, mem + off, dev_offset, len);

	nd_region_release_lane(nd_region, bw_index);
	kunmap_atomic(mem);

	return rc;
}
EXPORT_SYMBOL(nd_blk_do_io);

int nd_blk_init_region(struct nd_region *nd_region)
{
	struct nd_blk_window *ndbw = &nd_region->bw;
	struct nd_mapping *nd_mapping;
	struct nd_dimm *nd_dimm;
	struct nd_mem *nd_mem;
	struct resource res;
	int i;

	if (!is_nd_blk(&nd_region->dev))
		return 0;

	if (!is_acpi_blk(&nd_region->dev))
		return 0;

	/* FIXME: use nfit values rather than hard coded */
	if (nd_region->ndr_mappings != 1)
		return -ENXIO;

	nd_mapping = &nd_region->mapping[0];
	nd_dimm = nd_mapping->nd_dimm;
	nd_mem = nd_dimm->nd_mem;
	if (!nd_mem->nfit_dcr || !nd_mem->nfit_bdw)
		return -ENXIO;

	/* map block aperture memory */
	res.start = bw_apt_phys;
	res.end = res.start + bw_size - 1;
	res.name = dev_name(&nd_region->dev);
	res.flags = IORESOURCE_MEM | IORESOURCE_CACHEABLE;
	/* FIXME: add devm_ioremap_cache() support */
	bw_apt = devm_ioremap_resource(&nd_region->dev, &res);
	if (IS_ERR(bw_apt))
		return PTR_ERR(bw_apt);

	/* map block control memory */
	res.start = bw_ctl_phys;
	res.end = res.start + bw_size - 1;
	res.flags &= ~IORESOURCE_CACHEABLE;
	bw_ctl = devm_ioremap_resource(&nd_region->dev, &res);
	if (IS_ERR(bw_ctl))
		return PTR_ERR(bw_ctl);

	/* set up block windows */
	blk_ctl = devm_kmalloc(&nd_region->dev, sizeof(*blk_ctl) * NUM_BW,
			GFP_KERNEL);
	if (!blk_ctl)
		return -ENOMEM;

	for (i = 0; i < NUM_BW; i++) {
		blk_ctl[i].bw_apt = (void *)bw_apt + i*0x2000;
		blk_ctl[i].bw_ctl = (void *)bw_ctl + i*0x2000;
		blk_ctl[i].bw_stat = (void *)blk_ctl[i].bw_ctl + 0x1000;
	}

	ndbw->blk_ctl = blk_ctl;

	return 0;
}

static void nd_blk_init(struct nd_bus *nd_bus, struct nd_region *nd_region)
{
	struct nd_spa *nd_spa = nd_region->nd_spa;
	u16 spa_index = nfit_spa_index(nd_bus->nfit_desc, nd_spa->nfit_spa);
	struct nd_mem *nd_mem, *found = NULL;
	struct nd_mapping *nd_mapping;
	struct nd_dimm *nd_dimm;
	u32 nfit_handle;

	nd_region->dev.type = &nd_block_device_type;
	nd_region->ndr_mappings = 0;
	list_for_each_entry(nd_mem, &nd_bus->dimms, dimms)
		if (nfit_spa_index(nd_bus->nfit_desc, nd_mem->nfit_spa_dcr)
				== spa_index) {
			found = nd_mem;
			break;
		}

	if (!found) {
		dev_WARN_ONCE(&nd_region->dev, !found,
				"BUG: missing dimm-control-region\n");
		return;
	}

	nfit_handle = readl(&nd_mem->nfit_mem->nfit_handle);
	nd_dimm = nd_dimm_by_handle(nd_bus, nfit_handle);
	if (!nd_mem->nfit_bdw) {
		dev_dbg(&nd_region->dev,
				"%s: %s no block-data-window descriptor\n",
				__func__, dev_name(&nd_dimm->dev));
		put_device(&nd_dimm->dev);
		return;
	}
	if (readq(&nd_mem->nfit_bdw->blk_offset) % SZ_4K) {
		dev_err(&nd_region->dev, "%s: %s block-capacity is not 4K aligned\n",
				__func__, dev_name(&nd_dimm->dev));
		put_device(&nd_dimm->dev);
		return;
	}

	nd_region->ndr_mappings = 1;
	nd_region->num_lanes = min_t(unsigned short,
			readw(&nd_mem->nfit_bdw->num_bdw), ND_MAX_LANES);
	nd_mapping = &nd_region->mapping[0];
	nd_mapping->nd_dimm = nd_dimm;
	nd_mapping->size = readq(&nd_mem->nfit_bdw->blk_capacity);
	nd_mapping->start = readq(&nd_mem->nfit_bdw->blk_offset);
}

static void nd_spa_range_init(struct nd_bus *nd_bus, struct nd_region *nd_region,
		struct device_type *type)
{
	u16 i;
	struct nd_spa *nd_spa = nd_region->nd_spa;
	u16 spa_index = nfit_spa_index(nd_bus->nfit_desc, nd_spa->nfit_spa);

	nd_region->num_lanes = ND_MAX_LANES;
	nd_region->dev.type = type;
	for (i = 0; i < nd_region->ndr_mappings; i++) {
		struct nd_memdev *nd_memdev = nd_memdev_from_spa(nd_bus,
				spa_index, i);
		struct nd_mem *nd_mem = nd_memdev_to_mem(nd_bus, nd_memdev);
		u32 nfit_handle = readl(&nd_mem->nfit_mem->nfit_handle);
		struct nd_mapping *nd_mapping = &nd_region->mapping[i];
		struct nd_dimm *nd_dimm;

		nd_dimm = nd_dimm_by_handle(nd_bus, nfit_handle);
		nd_mapping->nd_dimm = nd_dimm;
		nd_mapping->start = readq(&nd_memdev->nfit_mem->region_dpa);
		nd_mapping->size = readq(&nd_memdev->nfit_mem->region_len);

		if ((nd_mapping->start | nd_mapping->size) % SZ_4K) {
			dev_err(&nd_region->dev, "%s: %s mapping is not 4K aligned\n",
					__func__, dev_name(&nd_dimm->dev));
			nd_region->ndr_mappings = 0;
			return;
		}
	}
}

static struct nd_region *nd_region_create(struct nd_bus *nd_bus,
		struct nd_spa *nd_spa)
{
	u16 spa_index = nfit_spa_index(nd_bus->nfit_desc, nd_spa->nfit_spa);
	int spa_type = nfit_spa_type(nd_bus->nfit_desc, nd_spa->nfit_spa);
	struct nd_region *nd_region;
	struct device *dev;
	u16 num_mappings;

	num_mappings = num_nd_mem(nd_bus, spa_index);
	nd_region = kzalloc(sizeof(struct nd_region)
			+ sizeof(struct nd_mapping) * num_mappings, GFP_KERNEL);
	if (!nd_region)
		return NULL;
	nd_region->id = ida_simple_get(&region_ida, 0, 0, GFP_KERNEL);
	if (nd_region->id < 0) {
		kfree(nd_region);
		return NULL;
	} else if (nd_region->id >= CONFIG_ND_MAX_REGIONS) {
		dev_err(&nd_bus->dev, "max region limit %d reached\n",
				CONFIG_ND_MAX_REGIONS);
		ida_simple_remove(&region_ida, nd_region->id);
		return NULL;
	}
	nd_region->nd_spa = nd_spa;
	nd_region->ndr_mappings = num_mappings;
	dev = &nd_region->dev;
	dev_set_name(dev, "region%d", nd_region->id);
	dev->parent = &nd_bus->dev;
	dev->groups = nd_region_attribute_groups;
	nd_region->ndr_size = nfit_spa_length(nd_bus->nfit_desc, nd_spa->nfit_spa);
	nd_region->ndr_start = nfit_spa_base(nd_bus->nfit_desc, nd_spa->nfit_spa);
	ida_init(&nd_region->ns_ida);
	switch (spa_type) {
	case NFIT_SPA_PM:
		nd_spa_range_init(nd_bus, nd_region, &nd_pmem_device_type);
		break;
	case NFIT_SPA_VOLATILE:
		nd_spa_range_init(nd_bus, nd_region, &nd_volatile_device_type);
		break;
	case NFIT_SPA_DCR:
		nd_blk_init(nd_bus, nd_region);
		break;
	default:
		break;
	}
	nd_device_register(dev);

	return nd_region;
}

int nd_bus_register_regions(struct nd_bus *nd_bus)
{
	struct nd_spa *nd_spa;
	int rc = 0;

	nd_region_init_locks();

	mutex_lock(&nd_bus_list_mutex);
	list_for_each_entry(nd_spa, &nd_bus->spas, list) {
		int spa_type;
		u16 spa_index;
		struct nd_region *nd_region;

		spa_type = nfit_spa_type(nd_bus->nfit_desc, nd_spa->nfit_spa);
		spa_index = nfit_spa_index(nd_bus->nfit_desc, nd_spa->nfit_spa);
		if (spa_index == 0) {
			dev_dbg(&nd_bus->dev, "detected invalid spa index\n");
			continue;
		}
		switch (spa_type) {
		case NFIT_SPA_PM:
		case NFIT_SPA_DCR:
		case NFIT_SPA_VOLATILE:
			nd_region = nd_region_create(nd_bus, nd_spa);
			if (!nd_region)
				rc = -ENOMEM;
			break;
		case NFIT_SPA_BDW:
			/* we'll consume this in nd_blk_register for the DCR */
			break;
		default:
			dev_info(&nd_bus->dev, "spa[%d] unhandled type: %s\n",
					spa_index, spa_type_name(spa_type));
			break;
		}
	}
	mutex_unlock(&nd_bus_list_mutex);

	nd_synchronize();

	return rc;
}
