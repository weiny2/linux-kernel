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
#include <linux/platform_device.h>
#include <linux/firmware.h>
#include <linux/module.h>
#include "nfit.h"

struct local_nfit_data {
	struct nfit_bus_descriptor nfit_desc;
	struct nd_bus *nd_bus;
};

static int local_nfit_probe(struct platform_device *pdev)
{
	struct nfit_bus_descriptor *nfit_desc;
	struct local_nfit_data *data;
	const struct firmware *fw;
	int rc;

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;
	platform_set_drvdata(pdev, data);

	rc = request_firmware(&fw, KBUILD_MODNAME ".bin", &pdev->dev);
	if (rc)
		return rc;

	nfit_desc = &data->nfit_desc;
	nfit_desc->nfit_base = (void __iomem *) devm_kmemdup(&pdev->dev,
			fw->data, fw->size, GFP_KERNEL);
	nfit_desc->nfit_size = fw->size;
	release_firmware(fw);

	if (!nfit_desc->nfit_base)
		return -ENOMEM;

	data->nd_bus = nfit_bus_register(&pdev->dev, nfit_desc);
	if (!data->nd_bus)
		return -EINVAL;

	return 0;
}

static int local_nfit_remove(struct platform_device *pdev)
{
	struct local_nfit_data *data = platform_get_drvdata(pdev);

	nfit_bus_unregister(data->nd_bus);

	return 0;
}

static const struct platform_device_id local_nfit_id[] = {
	{ KBUILD_MODNAME },
	{ },
};

static struct platform_driver local_nfit_driver = {
	.probe = local_nfit_probe,
	.remove = local_nfit_remove,
	.driver = {
		.name = KBUILD_MODNAME,
		.owner = THIS_MODULE,
	},
	.id_table = local_nfit_id,
};

static __init int local_nfit_init(void)
{
	return platform_driver_register(&local_nfit_driver);
}

static __exit void local_nfit_exit(void)
{
	platform_driver_unregister(&local_nfit_driver);
}


module_init(local_nfit_init);
module_exit(local_nfit_exit);
MODULE_ALIAS("platform:" KBUILD_MODNAME "*");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Intel Corporation");
MODULE_FIRMWARE(KBUILD_MODNAME ".bin");
