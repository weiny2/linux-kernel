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
#include <linux/module.h>
#include <linux/device.h>
#include <linux/async.h>
#include <linux/slab.h>
#include <linux/nd.h>
#include "nd.h"

static ASYNC_DOMAIN_EXCLUSIVE(nd_region_async);
static bool wait_probe;

static int nd_region_wait_probe_get(char *val, const struct kernel_param *kp)
{
	async_synchronize_full_domain(&nd_region_async);
	wait_probe = true;
	return param_get_bool(val, kp);
}

static struct kernel_param_ops wait_probe_ops = {
	.get = nd_region_wait_probe_get,
	.set = param_set_bool,
};
module_param_cb(wait_probe, &wait_probe_ops, &wait_probe, S_IRUGO);
MODULE_PARM_DESC(wait_probe, "Flush async probe events");

static void namespace_io_release(struct device *dev)
{
	struct nd_namespace_io *nsio = to_nd_namespace_io(dev);

	kfree(nsio);
}

static struct device_type namespace_io_device_type = {
	.name = "nd_namespace_io",
	.release = namespace_io_release,
};

static ssize_t type_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nd_region *nd_region = to_nd_region(dev->parent);

	return sprintf(buf, "%d\n", nd_region_to_namespace_type(nd_region));
}
static DEVICE_ATTR_RO(type);

static struct attribute *nd_namespace_attributes[] = {
	&dev_attr_type.attr,
	NULL,
};

static struct attribute_group nd_namespace_attribute_group = {
	.attrs = nd_namespace_attributes,
};

static const struct attribute_group *nd_namespace_attribute_groups[] = {
	&nd_device_attribute_group,
	&nd_namespace_attribute_group,
	NULL,
};

static struct device **create_namespace_io(struct nd_region *nd_region)
{
	struct nd_namespace_io *nsio;
	struct device *dev, **devs;
	struct resource *res;

	nsio = kzalloc(sizeof(*nsio), GFP_KERNEL);
	if (!nsio)
		return NULL;

	devs = kcalloc(2, sizeof(struct device *), GFP_KERNEL);
	if (!devs) {
		kfree(nsio);
		return NULL;
	}

	dev = &nsio->dev;
	dev->type = &namespace_io_device_type;
	res = &nsio->res;
	res->name = dev_name(&nd_region->dev);
	res->flags = IORESOURCE_MEM;
	res->start = nd_region->ndr_start;
	res->end = res->start + nd_region->ndr_size - 1;

	devs[0] = dev;
	return devs;
}

static void nd_region_release_driver(void *data, async_cookie_t cookie)
{
	struct nd_region *nd_region = data;

	device_release_driver(&nd_region->dev);
}

static void __nd_region_probe(void *data, async_cookie_t cookie)
{
	struct nd_region *nd_region = data;
	struct device **devs = NULL;
	int i;

	nd_region_wait_for_peers(nd_region);

	switch (nd_region_to_namespace_type(nd_region)) {
	case ND_DEVICE_NAMESPACE_IO:
		devs = create_namespace_io(nd_region);
		break;
	default:
		break;
	}

	for (i = 0; devs && devs[i]; i++) {
		struct device *dev = devs[i];

		dev_set_name(dev, "namespace%d.%d", nd_region->id, i);
		dev->parent = &nd_region->dev;
		dev->bus = nd_region->dev.bus;
		dev->groups = nd_namespace_attribute_groups;
		if (device_register(dev) != 0) {
			dev_err(&nd_region->dev, "failed to register %s\n",
					dev_name(dev));
			put_device(dev);
			break;
		}
	}

	/*
	 * If we found no children to register, or failed to register a child,
	 * disable (unbind) the region.  Note, driver release will unregister
	 * all child namespaces
	 */
	if (!devs || devs[i]) {
		/*
		 * Given the remove path syncs async-probing, we need to do the
		 * driver release out of line
		 */
		async_schedule_domain(nd_region_release_driver, nd_region,
				&nd_region_async);
	}
	kfree(devs);
}

static int nd_region_probe(struct device *dev)
{
	struct nd_region *nd_region = to_nd_region(dev);
	async_cookie_t cookie;

	cookie = async_schedule_domain(__nd_region_probe, nd_region,
			&nd_region_async);
	dev_set_drvdata(dev, (void *) ((unsigned long) cookie));

	return 0;
}

static int child_unregister(struct device *dev, void *data)
{
	device_unregister(dev);
	return 0;
}

static int nd_region_remove(struct device *dev)
{
	async_cookie_t cookie;

	cookie = (unsigned long) dev_get_drvdata(dev);
	async_synchronize_cookie_domain(cookie, &nd_region_async);

	device_for_each_child(dev, NULL, child_unregister);
	return 0;
}

static struct nd_device_driver nd_region_driver = {
	.drv = {
		.name = "nd_region",
		.probe = nd_region_probe,
		.remove = nd_region_remove,
	},
	.type = ND_DRIVER_REGION_BLOCK | ND_DRIVER_REGION_PMEM,
};

static int __init nd_region_init(void)
{
	int rc = nd_driver_register(&nd_region_driver);

	if (wait_probe)
		async_synchronize_full_domain(&nd_region_async);
	return rc;
}

static void __exit nd_region_exit(void)
{
	driver_unregister(&nd_region_driver.drv);
	async_synchronize_full_domain(&nd_region_async);
}

MODULE_ALIAS_ND_DEVICE(ND_DEVICE_REGION_PMEM);
MODULE_ALIAS_ND_DEVICE(ND_DEVICE_REGION_BLOCK);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Intel Corporation");
module_init(nd_region_init);
module_exit(nd_region_exit);
