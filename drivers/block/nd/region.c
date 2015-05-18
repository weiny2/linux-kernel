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
#include <linux/cpumask.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/nd.h>
#include "nd.h"

struct nd_percpu_lane {
	int count[CONFIG_ND_MAX_REGIONS];
	spinlock_t lock[CONFIG_ND_MAX_REGIONS];
};

static DEFINE_PER_CPU(struct nd_percpu_lane, nd_percpu_lane);

static void __init nd_region_init_locks(void)
{
	unsigned int i, j;

	for (i = 0; i < nr_cpu_ids; i++)
		for (j = 0; j < CONFIG_ND_MAX_REGIONS; j++) {
			struct nd_percpu_lane *ndl;

			ndl = per_cpu_ptr(&nd_percpu_lane, i);
			spin_lock_init(&ndl->lock[j]);
			ndl->count[j] = 0;
		}
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
	if (nd_region->num_lanes < nr_cpu_ids) {
		struct nd_percpu_lane *ndl_lock, *ndl_count;
		unsigned int id = nd_region->id;

		lane = cpu % nd_region->num_lanes;
		ndl_count = per_cpu_ptr(&nd_percpu_lane, cpu);
		ndl_lock = per_cpu_ptr(&nd_percpu_lane, lane);
		if (ndl_count->count[id]++ == 0)
			spin_lock(&ndl_lock->lock[id]);
	} else
		lane = cpu;

	return lane;
}
EXPORT_SYMBOL(nd_region_acquire_lane);

void nd_region_release_lane(struct nd_region *nd_region, unsigned int lane)
{
	if (nd_region->num_lanes < nr_cpu_ids) {
		unsigned int cpu = get_cpu();
		unsigned int id = nd_region->id;
		struct nd_percpu_lane *ndl_lock, *ndl_count;

		ndl_count = per_cpu_ptr(&nd_percpu_lane, cpu);
		ndl_lock = per_cpu_ptr(&nd_percpu_lane, lane);
		if (--ndl_count->count[id] == 0)
			spin_unlock(&ndl_lock->lock[id]);
		put_cpu();
	}
	put_cpu();
}
EXPORT_SYMBOL(nd_region_release_lane);

static int nd_region_probe(struct device *dev)
{
	int err, rc;
	static unsigned long once;
	struct nd_region_namespaces *num_ns;
	struct nd_region *nd_region = to_nd_region(dev);

	if (nd_region->num_lanes > num_online_cpus()
			&& nd_region->num_lanes < num_possible_cpus()
			&& !test_and_set_bit(0, &once)) {
		dev_info(dev, "online cpus (%d) < concurrent i/o lanes (%d) < possible cpus (%d)\n",
				num_online_cpus(), nd_region->num_lanes,
				num_possible_cpus());
		dev_info(dev, "setting nr_cpus=%d may yield better libnd device performance\n",
				nd_region->num_lanes);
	}

	rc = nd_blk_region_init(nd_region);
	if (rc)
		return rc;

	rc = nd_region_register_namespaces(nd_region, &err);
	num_ns = devm_kzalloc(dev, sizeof(*num_ns), GFP_KERNEL);
	if (!num_ns)
		return -ENOMEM;

	if (rc < 0)
		return rc;

	num_ns->active = rc;
	num_ns->count = rc + err;
	dev_set_drvdata(dev, num_ns);

	if (err == 0)
		return 0;

	if (rc == err)
		return -ENODEV;

	/*
	 * Given multiple namespaces per region, we do not want to
	 * disable all the successfully registered peer namespaces upon
	 * a single registration failure.  If userspace is missing a
	 * namespace that it expects it can disable/re-enable the region
	 * to retry discovery after correcting the failure.
	 * <regionX>/namespaces returns the current
	 * "<async-registered>/<total>" namespace count.
	 */
	dev_err(dev, "failed to register %d namespace%s, continuing...\n",
			err, err == 1 ? "" : "s");
	return 0;
}

static int child_unregister(struct device *dev, void *data)
{
	nd_device_unregister(dev, ND_SYNC);
	return 0;
}

static int nd_region_remove(struct device *dev)
{
	struct nd_region *nd_region = to_nd_region(dev);

	/* flush attribute readers and disable */
	nd_bus_lock(dev);
	nd_region->ns_seed = NULL;
	dev_set_drvdata(dev, NULL);
	nd_bus_unlock(dev);

	device_for_each_child(dev, NULL, child_unregister);
	return 0;
}

static struct nd_device_driver nd_region_driver = {
	.probe = nd_region_probe,
	.remove = nd_region_remove,
	.drv = {
		.name = "nd_region",
	},
	.type = ND_DRIVER_REGION_BLK | ND_DRIVER_REGION_PMEM,
};

int __init nd_region_init(void)
{
	nd_region_init_locks();
	return nd_driver_register(&nd_region_driver);
}

void __exit nd_region_exit(void)
{
	driver_unregister(&nd_region_driver.drv);
}

MODULE_ALIAS_ND_DEVICE(ND_DEVICE_REGION_PMEM);
MODULE_ALIAS_ND_DEVICE(ND_DEVICE_REGION_BLK);
