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
#include <linux/export.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include "nd-private.h"
#include "nfit.h"

LIST_HEAD(nd_bus_list);
DEFINE_MUTEX(nd_bus_list_mutex);
static DEFINE_IDA(nd_ida);

static void nd_bus_free(struct nd_bus *nd_bus)
{
	struct nd_spa *nd_spa, *_spa;
	struct nd_dcr *nd_dcr, *_dcr;
	struct nd_bdw *nd_bdw, *_bdw;
	struct nd_mem *nd_mem, *_mem;

	if (!nd_bus)
		return;

	list_for_each_entry_safe(nd_spa, _spa, &nd_bus->spas, list) {
		list_del_init(&nd_spa->list);
		kfree(nd_spa);
	}
	list_for_each_entry_safe(nd_dcr, _dcr, &nd_bus->dcrs, list) {
		list_del_init(&nd_dcr->list);
		kfree(nd_dcr);
	}
	list_for_each_entry_safe(nd_bdw, _bdw, &nd_bus->bdws, list) {
		list_del_init(&nd_bdw->list);
		kfree(nd_bdw);
	}
	list_for_each_entry_safe(nd_mem, _mem, &nd_bus->memdevs, list) {
		list_del_init(&nd_mem->list);
		kfree(nd_mem);
	}

	if (nd_bus->nfit)
		iounmap(nd_bus->nfit);

	ida_simple_remove(&nd_ida, nd_bus->id);
	kfree(nd_bus);
}

static void nd_bus_release(struct device *dev)
{
	struct nd_bus *nd_bus = container_of(dev, struct nd_bus, dev);

	nd_bus_free(nd_bus);
}

static void *nd_bus_alloc(struct nfit_bus_descriptor *nfit_desc)
{
	struct nd_bus *nd_bus = kzalloc(sizeof(*nd_bus), GFP_KERNEL);

	if (!nd_bus)
		return NULL;
	INIT_LIST_HEAD(&nd_bus->spas);
	INIT_LIST_HEAD(&nd_bus->dcrs);
	INIT_LIST_HEAD(&nd_bus->bdws);
	INIT_LIST_HEAD(&nd_bus->memdevs);
	INIT_LIST_HEAD(&nd_bus->list);
	nd_bus->id = ida_simple_get(&nd_ida, 0, 0, GFP_KERNEL);
	if (test_bit(NFIT_FLAG_FIC1_CAP, &nfit_desc->flags))
		nd_bus->format_interface_code = 1;
	if (nd_bus->id < 0) {
		kfree(nd_bus);
		return NULL;
	}
	nd_bus->nfit_desc = nfit_desc;

	return nd_bus;
}

struct nfit_table_header {
	__le16 type;
	__le16 length;
};

static void __iomem *add_table(struct nd_bus *nd_bus, void __iomem *table,
		const void __iomem *end)
{
	struct nfit_table_header __iomem *hdr;

	if (table >= end)
		return NULL;

	hdr = (struct nfit_table_header __iomem *) table;
	switch (readw(&hdr->type)) {
	case NFIT_TABLE_SPA: {
		struct nd_spa *nd_spa = kzalloc(sizeof(*nd_spa), GFP_KERNEL);

		if (!nd_spa)
			return ERR_PTR(-ENOMEM);
		INIT_LIST_HEAD(&nd_spa->list);
		nd_spa->nfit_spa = table;
		list_add_tail(&nd_spa->list, &nd_bus->spas);
		break;
	}
	case NFIT_TABLE_MEM: {
		struct nd_mem *nd_mem = kzalloc(sizeof(*nd_mem), GFP_KERNEL);

		if (!nd_mem)
			return ERR_PTR(-ENOMEM);
		INIT_LIST_HEAD(&nd_mem->list);
		nd_mem->nfit_mem = table;
		list_add_tail(&nd_mem->list, &nd_bus->memdevs);
		break;
	}
	case NFIT_TABLE_DCR: {
		struct nd_dcr *nd_dcr = kzalloc(sizeof(*nd_dcr), GFP_KERNEL);

		if (!nd_dcr)
			return ERR_PTR(-ENOMEM);
		INIT_LIST_HEAD(&nd_dcr->list);
		nd_dcr->nfit_dcr = table;
		list_add_tail(&nd_dcr->list, &nd_bus->dcrs);
		break;
	}
	case NFIT_TABLE_BDW: {
		struct nd_bdw *nd_bdw = kzalloc(sizeof(*nd_bdw), GFP_KERNEL);

		if (!nd_bdw)
			return ERR_PTR(-ENOMEM);
		INIT_LIST_HEAD(&nd_bdw->list);
		nd_bdw->nfit_bdw = table;
		list_add_tail(&nd_bdw->list, &nd_bus->bdws);
		break;
	}
	case NFIT_TABLE_IDT:
	case NFIT_TABLE_FLUSH:
	case NFIT_TABLE_SMBIOS:
		/* TODO */
		break;
	default:
		pr_err("unknown table '%d' parsing nfit\n", readw(&hdr->type));
		return ERR_PTR(-EINVAL);
	}

	return table + readw(&hdr->length);
}

static int nd_child_unregister(struct device *dev, void *data)
{
	/*
	 * the singular ndctl class device per bus needs to be
	 * "device_destroy"ed, so skip it here
	 */
	if (dev->class)
		/* pass */;
	else
		device_unregister(dev);
	return 0;
}

static struct nd_bus *__nd_bus_register(struct device *parent,
		struct nd_bus *nd_bus)
{
	struct nfit __iomem *nfit = nd_bus->nfit;
	u8 *data, sum, signature[4];
	void __iomem *base = nfit;
	const void __iomem *end;
	size_t size, i;
	int rc;

	size = nd_bus->nfit_desc->nfit_size;
	if (size < sizeof(struct nfit))
		goto err;

	size = min_t(u32, size, readl(&nfit->length));
	data = (u8 *) base;
	for (i = 0, sum = 0; i < size; i++)
		sum += readb(&data[i]);
	sum -= readb(&nfit->checksum);
	if (sum != readb(&nfit->checksum)) {
		pr_debug("%s: nfit checksum failure\n", __func__);
		goto err;
	}

	memcpy_fromio(signature, &nfit->signature, sizeof(signature));
	if (memcmp(signature, "NFIT", 4) != 0) {
		pr_debug("%s: nfit signature mismatch\n", __func__);
		goto err;
	}

	end = base + size;
	base += sizeof(struct nfit);
	base = add_table(nd_bus, base, end);
	while (!IS_ERR_OR_NULL(base))
		base = add_table(nd_bus, base, end);

	if (IS_ERR(base)) {
		pr_debug("%s: nfit table parsing error: %ld\n",
				__func__, PTR_ERR(base));
		goto err;
	}

	nd_bus->dev.parent = parent;
	nd_bus->dev.release = nd_bus_release;
	dev_set_name(&nd_bus->dev, "ndbus%d", nd_bus->id);
	rc = device_register(&nd_bus->dev);
	if (rc) {
		pr_debug("%s: nd_bus device registration failed: %d\n",
				__func__, rc);
		goto err;
	}

	rc = nd_bus_create(nd_bus);
	if (rc)
		goto err;

	rc = nd_bus_register_dimms(nd_bus);
	if (rc)
		goto err_child;

	mutex_lock(&nd_bus_list_mutex);
	list_add_tail(&nd_bus->list, &nd_bus_list);
	mutex_unlock(&nd_bus_list_mutex);

	return nd_bus;
 err_child:
	device_for_each_child(&nd_bus->dev, NULL, nd_child_unregister);
	nd_bus_destroy(nd_bus);
 err:
	nd_bus_free(nd_bus);
	return NULL;
}

struct nd_bus *nfit_bus_register(struct device *parent,
		struct nfit_bus_descriptor *nfit_desc)
{
	void __iomem *nfit_base;
	struct nd_bus *nd_bus;

	nfit_base = ioremap_cache(nfit_desc->nfit_start, nfit_desc->nfit_size);
	if (!nfit_base)
		return NULL;

	nd_bus = nd_bus_alloc(nfit_desc);
	if (!nd_bus)
		goto err_bus_alloc;

	nd_bus->nfit = nfit_base;

	return __nd_bus_register(parent, nd_bus);

 err_bus_alloc:
	iounmap(nfit_base);

	return NULL;
}
EXPORT_SYMBOL(nfit_bus_register);

void nfit_bus_unregister(struct nd_bus *nd_bus)
{
	mutex_lock(&nd_bus_list_mutex);
	list_del_init(&nd_bus->list);
	mutex_unlock(&nd_bus_list_mutex);

	device_for_each_child(&nd_bus->dev, NULL, nd_child_unregister);
	nd_bus_destroy(nd_bus);

	device_unregister(&nd_bus->dev);
}
EXPORT_SYMBOL(nfit_bus_unregister);

static __init int nd_core_init(void)
{
	BUILD_BUG_ON(sizeof(struct nfit) != 40);
	BUILD_BUG_ON(sizeof(struct nfit_spa) != 32);
	BUILD_BUG_ON(sizeof(struct nfit_mem) != 48);
	BUILD_BUG_ON(sizeof(struct nfit_idt) != 20);
	BUILD_BUG_ON(sizeof(struct nfit_smbios) != 8);
	BUILD_BUG_ON(sizeof(struct nfit_dcr) != 56);
	BUILD_BUG_ON(sizeof(struct nfit_bdw) != 40);
	BUILD_BUG_ON(sizeof(struct nfit_flush) != 24);

	return nd_bus_init();
}

static __exit void nd_core_exit(void)
{
	WARN_ON(!list_empty(&nd_bus_list));
	nd_bus_exit();
}
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Intel Corporation");
module_init(nd_core_init);
module_exit(nd_core_exit);
