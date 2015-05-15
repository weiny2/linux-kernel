/*
 * libnd e820 support
 *
 * Copyright (c) 2014-2015, Intel Corporation.
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
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/libnd.h>

static const struct attribute_group *e820_pmem_attribute_groups[] = {
	&nd_bus_attribute_group,
	NULL,
};

static const struct attribute_group *e820_pmem_region_attribute_groups[] = {
	&nd_region_attribute_group,
	&nd_device_attribute_group,
	NULL,
};

static int e820_pmem_probe(struct platform_device *pdev)
{
	struct nd_bus_descriptor *nd_desc;
	struct nd_region_desc ndr_desc;
	struct nd_bus *nd_bus;
	struct resource *res;

	if (WARN_ON(pdev->num_resources > 1))
		return -ENXIO;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENXIO;

	nd_desc = devm_kzalloc(&pdev->dev, sizeof(*nd_desc), GFP_KERNEL);
	if (!nd_desc)
		return -ENOMEM;

	nd_desc->attr_groups = e820_pmem_attribute_groups;
	nd_desc->provider_name = "e820";
	nd_bus = nd_bus_register(&pdev->dev, nd_desc);
	if (!nd_bus)
		return -ENXIO;

	memset(&ndr_desc, 0, sizeof(ndr_desc));
	ndr_desc.res = res;
	ndr_desc.attr_groups = e820_pmem_region_attribute_groups;
	if (!nd_pmem_region_create(nd_bus, &ndr_desc)) {
		nd_bus_unregister(nd_bus);
		return -ENXIO;
	}

	platform_set_drvdata(pdev, nd_bus);

	return 0;
}

static int e820_pmem_remove(struct platform_device *pdev)
{
	struct nd_bus *nd_bus = platform_get_drvdata(pdev);

	nd_bus_unregister(nd_bus);

	return 0;
}

static struct platform_driver e820_pmem_driver = {
	.probe		= e820_pmem_probe,
	.remove		= e820_pmem_remove,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "e820_pmem",
	},
};

MODULE_ALIAS("platform:e820_pmem*");

static int __init e820_pmem_init(void)
{
	return platform_driver_register(&e820_pmem_driver);
}
module_init(e820_pmem_init);

static void e820_pmem_exit(void)
{
	platform_driver_unregister(&e820_pmem_driver);
}
module_exit(e820_pmem_exit);

MODULE_AUTHOR("Intel Corporation");
MODULE_LICENSE("GPL v2");
