/*
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
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
 *
 * BSD LICENSE
 *
 * Copyright(c) 2013-2014 Intel Corporation. All rights reserved.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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
	struct nd_bus *nd_bus;
	struct list_head list;
	acpi_handle handle;
	struct kref kref;
};

static void nd_acpi_nfit_release(struct kref *kref)
{
	struct acpi_nfit *nfit = container_of(kref, typeof(*nfit), kref);

	nfit_bus_unregister(nfit->nd_bus);
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

struct acpi_nfit *nd_acpi_get_nfit(resource_size_t start)
{
	struct acpi_nfit *nfit;
	struct nfit_bus_descriptor *nfit_desc;

	list_for_each_entry(nfit, &nfits, list) {
		nfit_desc = &nfit->nfit_desc;
		if (nfit_desc->nfit_start == start) {
			kref_get(&nfit->kref);
			return nfit;
		}
	}
	return NULL;
}

static acpi_status nd_acpi_add_nfit(struct acpi_resource *resource, void *_h)
{
	struct acpi_resource_address64 address64;
	struct nfit_bus_descriptor *nfit_desc;
	struct acpi_nfit *nfit;
	resource_size_t start;
	acpi_status status;
	acpi_handle handle = _h;

	status = acpi_resource_to_address64(resource, &address64);
	if (ACPI_FAILURE(status))
		return AE_ERROR;

	start = address64.minimum;
	mutex_lock(&nd_acpi_lock);
	nfit = nd_acpi_get_nfit(start);
	if (nfit)
		goto out;

	nfit = kzalloc(sizeof(*nfit), GFP_KERNEL);
	if (!nfit) {
		status = AE_ERROR;
		goto out;
	}
	kref_init(&nfit->kref);
	INIT_LIST_HEAD(&nfit->list);
	nfit->handle = handle;
	nfit_desc = &nfit->nfit_desc;
	nfit_desc->nfit_start = start;
	nfit_desc->nfit_size = address64.address_length;
	nfit_desc->provider_name = "ACPI.NFIT";
	nfit_desc->nfit_ctl = nd_acpi_ctl;
	/* declare support for "format interface code 1" messages */
	set_bit(NFIT_FLAG_FIC1_CAP, &nfit_desc->flags);

	/* ACPI references one global NFIT for all devices, i.e. no parent */
	nfit->nd_bus = nfit_bus_register(NULL, nfit_desc);
	if (!nfit->nd_bus) {
		kref_put(&nfit->kref, nd_acpi_nfit_release);
		status = AE_ERROR;
	} else {
		list_add(&nfit->list, &nfits);
		status = AE_OK;
	}
 out:
	mutex_unlock(&nd_acpi_lock);

	return status;
}

static int nd_acpi_add(struct acpi_device *dev)
{
	acpi_status status;

	status = acpi_walk_resources(dev->handle, METHOD_NAME__CRS,
				     nd_acpi_add_nfit, dev->handle);
	if (ACPI_FAILURE(status))
		return -EINVAL;

	return 0;
}

static acpi_status nd_acpi_remove_nfit(struct acpi_resource *resource, void *n)
{
	struct acpi_resource_address64 address64;
	struct acpi_nfit *nfit;
	resource_size_t start;
	acpi_status status;

	status = acpi_resource_to_address64(resource, &address64);
	if (ACPI_FAILURE(status))
		return AE_OK; /* sarcastic "ok", nothing we can do */

	start = address64.minimum;
	mutex_lock(&nd_acpi_lock);
	nfit = nd_acpi_get_nfit(start);
	if (nfit) {
		list_del_init(&nfit->list);
		kref_put(&nfit->kref, nd_acpi_nfit_release);
	}
	mutex_unlock(&nd_acpi_lock);

	return AE_OK;
}

static int nd_acpi_remove(struct acpi_device *dev)
{
	acpi_walk_resources(dev->handle, METHOD_NAME__CRS, nd_acpi_remove_nfit,
			NULL);
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
	int rc = acpi_bus_register_driver(&nd_acpi_driver);

	if (rc)
		return rc;
	return 0;
}

static __exit void nd_acpi_exit(void)
{
	acpi_bus_unregister_driver(&nd_acpi_driver);
	WARN_ON(!list_empty(&nfits));
}

module_init(nd_acpi_init);
module_exit(nd_acpi_exit);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Intel Corporation");
