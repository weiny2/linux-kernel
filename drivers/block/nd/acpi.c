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
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/list.h>
#include <linux/acpi.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include "nfit.h"

static LIST_HEAD(nfits);
static DEFINE_MUTEX(nd_acpi_lock);

struct acpi_nfit {
	struct nfit_bus_descriptor nfit_desc;
	struct list_head list;
	struct nd_bus *nd_bus;
	acpi_handle handle;
	struct kref kref;
};

static void nd_acpi_nfit_release(struct kref *kref)
{
	struct acpi_nfit *nfit = container_of(kref, typeof(*nfit), kref);

	nfit_bus_unregister(nfit->nd_bus);
	list_del(&nfit->list);
	kfree(nfit);
}

static struct acpi_nfit *to_acpi_nfit(struct nfit_bus_descriptor *nfit_desc)
{
	return container_of(nfit_desc, struct acpi_nfit, nfit_desc);
}

static int nd_acpi_ctl(struct nfit_bus_descriptor *nfit_desc,
		unsigned int cmd, void *buf, unsigned int buf_len)
{
	struct acpi_nfit *nfit = to_acpi_nfit(nfit_desc);
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
	obj = acpi_evaluate_dsm(nfit->handle, uuid, 1, cmd, &argv4);
	if (!obj) {
		pr_debug("%s: _DSM failed cmd: %d\n", __func__, cmd);
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
		pr_debug("%s: unknown _DSM return cmd: %d\n", __func__, cmd);
		rc = -EINVAL;
	}
	ACPI_FREE(obj);

	return rc;
}

static int nfit_devices;

static int __nd_acpi_add(struct acpi_device *dev)
{
	struct acpi_table_header *tbl;
	acpi_status status = AE_OK;
	struct acpi_nfit *nfit;
	int rc = 0, i;
	acpi_size sz;

	if (nfit_devices > 0) {
		list_for_each_entry(nfit, &nfits, list)
			kref_get(&nfit->kref);
		return 0;
	} else if (nfit_devices < 0) {
		return nfit_devices;
	} else
		/* pass */;

	for (i = 0; status == AE_OK; i++) {
		struct nfit_bus_descriptor *nfit_desc;

		status = acpi_get_table_with_size("NFIT", i, &tbl, &sz);
		if (ACPI_FAILURE(status)) {
			if (i == 0) {
				dev_err(&dev->dev, "failed to find NFIT\n");
				rc = -ENXIO;
			}
			break;
		}

		nfit = kzalloc(sizeof(*nfit), GFP_KERNEL);
		if (!nfit) {
			rc = -ENOMEM;
			break;
		}
		kref_init(&nfit->kref);
		nfit->handle = dev->handle;
		INIT_LIST_HEAD(&nfit->list);
		nfit_desc = &nfit->nfit_desc;
		nfit_desc->nfit_base = (void __iomem *) tbl;
		nfit_desc->nfit_size = sz;
		nfit_desc->provider_name = "ACPI.NFIT";
		nfit_desc->nfit_ctl = nd_acpi_ctl;
		/* declare support for "format interface code 1" messages */
		set_bit(NFIT_FLAG_FIC1_CAP, &nfit_desc->flags);
		list_add(&nfit->list, &nfits);

		/* ACPI references one global NFIT for all devices, i.e. no parent */
		nfit->nd_bus = nfit_bus_register(NULL, nfit_desc);
		if (!nfit->nd_bus) {
			rc = -ENXIO;
			break;
		}
	}

	if (rc < 0) {
		struct acpi_nfit *_n;

		list_for_each_entry_safe(nfit, _n, &nfits, list)
			kref_put(&nfit->kref, nd_acpi_nfit_release);
		nfit_devices = rc;
		return rc;
	} else {
		nfit_devices++;
	}

	return rc;
}

static int nd_acpi_add(struct acpi_device *dev)
{
	int rc;

	mutex_lock(&nd_acpi_lock);
	rc = __nd_acpi_add(dev);
	mutex_unlock(&nd_acpi_lock);

	return rc < 0 ? rc : 0;
}

static int nd_acpi_remove(struct acpi_device *dev)
{
	mutex_lock(&nd_acpi_lock);
	if (nfit_devices > 0) {
		struct acpi_nfit *nfit, *_n;

		nfit_devices--;
		list_for_each_entry_safe(nfit, _n, &nfits, list)
			kref_put(&nfit->kref, nd_acpi_nfit_release);
	}
	mutex_unlock(&nd_acpi_lock);
	return 0;
}

static const struct acpi_device_id nd_acpi_ids[] = {
	{ "ACPI0010", 0 },
	{ "", 0 },
};
MODULE_DEVICE_TABLE(acpi, nd_acpi_ids);

static struct acpi_driver nd_acpi_driver = {
	.name = KBUILD_MODNAME,
	.ids = nd_acpi_ids,
	.ops = {
		.add = nd_acpi_add,
		.remove = nd_acpi_remove,
	},
};

static __init int nd_acpi_init(void)
{
	return acpi_bus_register_driver(&nd_acpi_driver);
}

static __exit void nd_acpi_exit(void)
{
	acpi_bus_unregister_driver(&nd_acpi_driver);
	WARN_ON(!list_empty(&nfits));
}

module_init(nd_acpi_init);
module_exit(nd_acpi_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Intel Corporation");
