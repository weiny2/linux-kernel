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
#include <linux/init.h>

static void local_nfit_release(struct device *dev)
{
}

static struct platform_device local_nfit_dev = {
	.name = "local_nfit",
	.dev = {
		.release = local_nfit_release,
	},
};

static int local_init(void)
{
	int rc = platform_device_register(&local_nfit_dev);

	if (rc)
		put_device(&local_nfit_dev.dev);
	return rc;
}
late_initcall(local_init);
