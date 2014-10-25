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
#include <linux/list.h>
#include <linux/acpi.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include "nfit.h"

enum {
	NFIT_ACPI_NOTIFY_TABLE = 0x80,
};

struct acpi_nfit {
	struct nfit_bus_descriptor nfit_desc;
	struct acpi_device *dev;
	struct nd_bus *nd_bus;
};

static struct acpi_nfit *to_acpi_nfit(struct nfit_bus_descriptor *nfit_desc)
{
	return container_of(nfit_desc, struct acpi_nfit, nfit_desc);
}

static int nd_acpi_ctl(struct nfit_bus_descriptor *nfit_desc,
		unsigned int cmd, void *buf, unsigned int buf_len)
{
	struct acpi_nfit *nfit = to_acpi_nfit(nfit_desc);
	acpi_handle handle = nfit->dev->handle;
	struct device *dev = &nfit->dev->dev;
	static const u8 uuid[] = {
		0x66, 0x9A, 0x0C, 0x20,
		0x00, 0x08, 0x91, 0x91,
		0xE4, 0x11, 0x11, 0x0D,
		0x30, 0xAC, 0x09, 0x43,
	};
	union acpi_object argv4 = {
		.buffer.type = ACPI_TYPE_BUFFER,
		.buffer.length = buf_len,
		.buffer.pointer = buf,
	};
	union acpi_object *obj;
	int rc;

	/*
	 * FIXME is the _DSM handle duplicated per DIMM or is there a
	 * global handle we should be using?
	 */
	obj = acpi_evaluate_dsm(handle, uuid, 1, cmd, &argv4);
	if (!obj) {
		dev_dbg(dev, "%s: _DSM failed cmd: %d\n", __func__, cmd);
		return -EINVAL;
	}

	if (obj->type == ACPI_TYPE_BUFFER) {
		unsigned int len = min(obj->buffer.length, buf_len);

		memcpy(buf, obj->buffer.pointer, len);
		if (obj->buffer.length <= buf_len)
			rc = 0;
		else
			rc = len;
	} else {
		dev_dbg(dev, "%s: unknown _DSM return cmd: %d\n",
				__func__, cmd);
		rc = -EINVAL;
	}
	ACPI_FREE(obj);

	return rc;
}

static struct acpi_device *nd_acpi_root;

static int nd_acpi_add(struct acpi_device *dev)
{
	struct nfit_bus_descriptor *nfit_desc;
	struct acpi_table_header *tbl;
	acpi_status status = AE_OK;
	struct acpi_nfit *nfit;
	int rc = 0;
	acpi_size sz;

	if (nd_acpi_root) {
		dev_info(&dev->dev, "ignoring, only one root device supported\n");
		return -EINVAL;
	}

	status = acpi_get_table_with_size("NFIT", 0, &tbl, &sz);
	if (ACPI_FAILURE(status)) {
		dev_err(&dev->dev, "failed to find NFIT\n");
		return -ENXIO;
	}

	nfit = devm_kzalloc(&dev->dev, sizeof(*nfit), GFP_KERNEL);
	if (!nfit)
		return -ENOMEM;
	nfit->dev = dev;
	nfit_desc = &nfit->nfit_desc;
	nfit_desc->nfit_base = (void __iomem *) tbl;
	nfit_desc->nfit_size = sz;
	nfit_desc->provider_name = "ACPI.NFIT";
	nfit_desc->nfit_ctl = nd_acpi_ctl;

	/* ACPI references one global NFIT for all devices, i.e. no parent */
	nfit->nd_bus = nfit_bus_register(NULL, nfit_desc);
	if (!nfit->nd_bus)
		return -ENXIO;

	dev_set_drvdata(&dev->dev, nfit);
	nd_acpi_root = dev;
	return 0;
}

static int nd_acpi_remove(struct acpi_device *dev)
{
	struct acpi_nfit *nfit = dev_get_drvdata(&dev->dev);

	nfit_bus_unregister(nfit->nd_bus);
	nd_acpi_root = NULL;
	return 0;
}

static void nd_acpi_notify(struct acpi_device *dev, u32 event)
{
	/* TODO: handle ACPI_NOTIFY_BUS_CHECK notification */
	dev_dbg(&dev->dev, "%s: event: %d\n", __func__);
}

static const struct acpi_device_id nd_acpi_ids[] = {
	{ "ACPI0011", 0 },
	{ "", 0 },
};
MODULE_DEVICE_TABLE(acpi, nd_acpi_ids);

static struct acpi_driver nd_acpi_driver = {
	.name = KBUILD_MODNAME,
	.ids = nd_acpi_ids,
	.flags = ACPI_DRIVER_ALL_NOTIFY_EVENTS,
	.ops = {
		.add = nd_acpi_add,
		.remove = nd_acpi_remove,
		.notify = nd_acpi_notify
	},
};

static const struct acpi_device_id nd_acpi_dimm_ids[] = {
	{ "ACPI0010", 0 },
	{ "", 0 },
};
MODULE_DEVICE_TABLE(acpi, nd_acpi_dimm_ids);

static int nd_acpi_dimm_add(struct acpi_device *dev)
{
	if (!nd_acpi_root) {
		dev_err(&dev->dev, "missing ACPI.NFIT root device\n");
		return -ENXIO;
	}

	/* TODO: add _FIT parsing */

	return 0;
}

static int nd_acpi_dimm_remove(struct acpi_device *dev)
{
	return 0;
}

static void nd_acpi_dimm_notify(struct acpi_device *dev, u32 event)
{
	/* TODO: handle NFIT_ACPI_NOTIFY_TABLE notification */
	dev_dbg(&dev->dev, "%s: event: %d\n", __func__);
}

static struct acpi_driver nd_acpi_dimm_driver = {
	.name = KBUILD_MODNAME "_dimm",
	.ids = nd_acpi_dimm_ids,
	.flags = ACPI_DRIVER_ALL_NOTIFY_EVENTS,
	.ops = {
		.add = nd_acpi_dimm_add,
		.remove = nd_acpi_dimm_remove,
		.notify = nd_acpi_dimm_notify
	},
};

static __init int nd_acpi_init(void)
{
	int rc;

	rc = acpi_bus_register_driver(&nd_acpi_driver);
	if (rc)
		return rc;
	rc = acpi_bus_register_driver(&nd_acpi_dimm_driver);
	if (rc) {
		acpi_bus_unregister_driver(&nd_acpi_driver);
		return rc;
	}
	return 0;
}

static __exit void nd_acpi_exit(void)
{
	acpi_bus_unregister_driver(&nd_acpi_dimm_driver);
	acpi_bus_unregister_driver(&nd_acpi_driver);
}

module_init(nd_acpi_init);
module_exit(nd_acpi_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Intel Corporation");
