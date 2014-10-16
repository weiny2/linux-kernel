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
#include <linux/export.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include "nd-private.h"
#include "nfit.h"
#include "nd.h"

LIST_HEAD(nd_bus_list);
DEFINE_MUTEX(nd_bus_list_mutex);
static DEFINE_IDA(nd_ida);

static bool wait_probe;
module_param(wait_probe, bool, 0);
MODULE_PARM_DESC(wait_probe,
		"Wait for nd devices to be probed (default: async probing)");

static void nd_bus_release(struct device *dev)
{
	struct nd_bus *nd_bus = container_of(dev, struct nd_bus, dev);
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

	ida_simple_remove(&nd_ida, nd_bus->id);
	kfree(nd_bus);
}

struct nd_bus *to_nd_bus(struct device *dev)
{
	struct nd_bus *nd_bus = container_of(dev, struct nd_bus, dev);

	WARN_ON(nd_bus->dev.release != nd_bus_release);
	return nd_bus;
}

struct nd_bus *walk_to_nd_bus(struct device *nd_dev)
{
	struct device *dev = nd_dev;

	while (dev && dev->release != nd_bus_release)
		dev = dev->parent;
	dev_WARN_ONCE(nd_dev, !dev, "invalid dev, not on nd bus\n");
	if (dev)
		return to_nd_bus(dev);
	return NULL;
}

static void *nd_bus_new(struct device *parent,
		struct nfit_bus_descriptor *nfit_desc)
{
	struct nd_bus *nd_bus = kzalloc(sizeof(*nd_bus), GFP_KERNEL);
	int rc;

	if (!nd_bus)
		return NULL;
	INIT_LIST_HEAD(&nd_bus->spas);
	INIT_LIST_HEAD(&nd_bus->dcrs);
	INIT_LIST_HEAD(&nd_bus->bdws);
	INIT_LIST_HEAD(&nd_bus->memdevs);
	INIT_LIST_HEAD(&nd_bus->deferred);
	INIT_LIST_HEAD(&nd_bus->list);
	init_completion(&nd_bus->registration);
	init_waitqueue_head(&nd_bus->deferq);
	nd_bus->id = ida_simple_get(&nd_ida, 0, 0, GFP_KERNEL);
	if (test_bit(NFIT_FLAG_FIC1_CAP, &nfit_desc->flags))
		nd_bus->format_interface_code = 1;
	if (nd_bus->id < 0) {
		kfree(nd_bus);
		return NULL;
	}
	nd_bus->nfit_desc = nfit_desc;
	nd_bus->dev.parent = parent;
	nd_bus->dev.release = nd_bus_release;
	dev_set_name(&nd_bus->dev, "ndbus%d", nd_bus->id);
	rc = device_register(&nd_bus->dev);
	if (rc) {
		dev_dbg(&nd_bus->dev, "device registration failed: %d\n", rc);
		put_device(&nd_bus->dev);
		return NULL;
	}
	return nd_bus;
}

struct nfit_table_header {
	__le16 type;
	__le16 length;
};

static const char *spa_type_name(u16 type)
{
	switch (type) {
	case NFIT_SPA_PM: return "pmem";
	case NFIT_SPA_DCR: return "dimm-control-region";
	case NFIT_SPA_BDW: return "block-data-window";
	default: return "unknown";
	}
}

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
		struct nfit_spa __iomem *nfit_spa = table;

		if (!nd_spa)
			return ERR_PTR(-ENOMEM);
		INIT_LIST_HEAD(&nd_spa->list);
		nd_spa->nfit_spa = nfit_spa;
		list_add_tail(&nd_spa->list, &nd_bus->spas);
		dev_dbg(&nd_bus->dev, "%s: spa index: %d type: %s\n", __func__,
				readw(&nfit_spa->spa_index),
				spa_type_name(readw(&nfit_spa->spa_type)));
		break;
	}
	case NFIT_TABLE_MEM: {
		struct nd_mem *nd_mem = kzalloc(sizeof(*nd_mem), GFP_KERNEL);
		struct nfit_mem __iomem *nfit_mem = table;

		if (!nd_mem)
			return ERR_PTR(-ENOMEM);
		INIT_LIST_HEAD(&nd_mem->list);
		nd_mem->nfit_mem = nfit_mem;
		list_add_tail(&nd_mem->list, &nd_bus->memdevs);
		dev_dbg(&nd_bus->dev, "%s: mem handle: %#x spa: %d dcr: %d\n",
				__func__, readl(&nfit_mem->nfit_handle),
				readw(&nfit_mem->spa_index),
				readw(&nfit_mem->dcr_index));
		break;
	}
	case NFIT_TABLE_DCR: {
		struct nd_dcr *nd_dcr = kzalloc(sizeof(*nd_dcr), GFP_KERNEL);
		struct nfit_dcr __iomem *nfit_dcr = table;

		if (!nd_dcr)
			return ERR_PTR(-ENOMEM);
		INIT_LIST_HEAD(&nd_dcr->list);
		nd_dcr->nfit_dcr = nfit_dcr;
		list_add_tail(&nd_dcr->list, &nd_bus->dcrs);
		dev_dbg(&nd_bus->dev, "%s: dcr index: %d num_bdw: %d\n",
				__func__, readw(&nfit_dcr->dcr_index),
				readw(&nfit_dcr->num_bdw));
		break;
	}
	case NFIT_TABLE_BDW: {
		struct nd_bdw *nd_bdw = kzalloc(sizeof(*nd_bdw), GFP_KERNEL);
		struct nfit_bdw __iomem *nfit_bdw = table;

		if (!nd_bdw)
			return ERR_PTR(-ENOMEM);
		INIT_LIST_HEAD(&nd_bdw->list);
		nd_bdw->nfit_bdw = nfit_bdw;
		list_add_tail(&nd_bdw->list, &nd_bus->bdws);
		dev_dbg(&nd_bus->dev, "%s: bdw dcr: %d num_bdw: %d\n", __func__,
				readw(&nfit_bdw->dcr_index),
				readw(&nfit_bdw->num_bdw));
		break;
	}
	/* TODO */
	case NFIT_TABLE_IDT:
		dev_dbg(&nd_bus->dev, "%s: idt\n", __func__);
		break;
	case NFIT_TABLE_FLUSH:
		dev_dbg(&nd_bus->dev, "%s: flush\n", __func__);
		break;
	case NFIT_TABLE_SMBIOS:
		dev_dbg(&nd_bus->dev, "%s: smbios\n", __func__);
		break;
	default:
		pr_err("unknown table '%d' parsing nfit\n", readw(&hdr->type));
		return ERR_PTR(-EINVAL);
	}

	return table + readw(&hdr->length);
}

static int child_unregister(struct device *dev, void *data)
{
	/*
	 * the singular ndctl class device per bus needs to be
	 * "device_destroy"ed, so skip it here
	 *
	 * i.e. remove classless children
	 */
	if (dev->class)
		/* pass */;
	else
		nd_device_unregister(dev, ND_SYNC);
	return 0;
}

static struct nd_bus *nd_bus_probe(struct nd_bus *nd_bus)
{
	struct nfit_bus_descriptor *nfit_desc = nd_bus->nfit_desc;
	struct nfit __iomem *nfit = nfit_desc->nfit_base;
	u8 *data, sum, signature[4];
	void __iomem *base = nfit;
	const void __iomem *end;
	size_t size, i;
	int rc;

	if (!nd_bus)
		return NULL;

	size = nd_bus->nfit_desc->nfit_size;
	if (size < sizeof(struct nfit))
		goto err;

	size = min_t(u32, size, readl(&nfit->length));
	data = (u8 *) base;
	for (i = 0, sum = 0; i < size; i++)
		sum += readb(&data[i]);
	if (sum != 0) {
		dev_dbg(&nd_bus->dev, "%s: nfit checksum failure\n", __func__);
		goto err;
	}

	memcpy_fromio(signature, &nfit->signature, sizeof(signature));
	if (memcmp(signature, "NFIT", 4) != 0) {
		dev_dbg(&nd_bus->dev, "%s: nfit signature mismatch\n",
				__func__);
		goto err;
	}

	end = base + size;
	base += sizeof(struct nfit);
	base = add_table(nd_bus, base, end);
	while (!IS_ERR_OR_NULL(base))
		base = add_table(nd_bus, base, end);

	if (IS_ERR(base)) {
		dev_dbg(&nd_bus->dev, "%s: nfit table parsing error: %ld\n",
				__func__, PTR_ERR(base));
		goto err;
	}

	rc = nd_bus_create_ndctl(nd_bus);
	if (rc)
		goto err;

	rc = nd_bus_register_dimms(nd_bus);
	if (rc)
		goto err_child;

	rc = nd_bus_register_regions(nd_bus);
	if (rc)
		goto err_child;

	mutex_lock(&nd_bus_list_mutex);
	list_add_tail(&nd_bus->list, &nd_bus_list);
	mutex_unlock(&nd_bus_list_mutex);
	complete_all(&nd_bus->registration);
	if (wait_probe)
		nd_bus_wait_probe(nd_bus);

	return nd_bus;
 err_child:
	complete_all(&nd_bus->registration);
	nd_bus_wait_probe(nd_bus);
	device_for_each_child(&nd_bus->dev, NULL, child_unregister);
	nd_bus_destroy_ndctl(nd_bus);
 err:
	put_device(&nd_bus->dev);
	return NULL;

}

struct nd_bus *nfit_bus_register(struct device *parent,
		struct nfit_bus_descriptor *nfit_desc)
{
	static DEFINE_MUTEX(mutex);
	struct nd_bus *nd_bus;

	/* enforce single bus at a time registration */
	mutex_lock(&mutex);
	nd_bus = nd_bus_new(parent, nfit_desc);
	nd_bus = nd_bus_probe(nd_bus);
	mutex_unlock(&mutex);

	if (!nd_bus)
		return NULL;

	return nd_bus;
}
EXPORT_SYMBOL(nfit_bus_register);

void nfit_bus_unregister(struct nd_bus *nd_bus)
{
	if (!nd_bus)
		return;

	mutex_lock(&nd_bus_list_mutex);
	list_del_init(&nd_bus->list);
	mutex_unlock(&nd_bus_list_mutex);

	nd_bus_wait_probe(nd_bus);
	device_for_each_child(&nd_bus->dev, NULL, child_unregister);
	nd_bus_destroy_ndctl(nd_bus);

	device_unregister(&nd_bus->dev);
}
EXPORT_SYMBOL(nfit_bus_unregister);

static __init int nd_core_init(void)
{
	int rc;

	BUILD_BUG_ON(sizeof(struct nfit) != 40);
	BUILD_BUG_ON(sizeof(struct nfit_spa) != 32);
	BUILD_BUG_ON(sizeof(struct nfit_mem) != 48);
	BUILD_BUG_ON(sizeof(struct nfit_idt) != 20);
	BUILD_BUG_ON(sizeof(struct nfit_smbios) != 8);
	BUILD_BUG_ON(sizeof(struct nfit_dcr) != 56);
	BUILD_BUG_ON(sizeof(struct nfit_bdw) != 40);
	BUILD_BUG_ON(sizeof(struct nfit_flush) != 24);

	rc = nd_bus_init();
	if (rc)
		return rc;
	rc = nd_dimm_init();
	if (rc)
		goto err_dimm;
	return 0;
 err_dimm:
	nd_bus_exit();
	return rc;

}

static __exit void nd_core_exit(void)
{
	WARN_ON(!list_empty(&nd_bus_list));
	nd_dimm_exit();
	nd_bus_exit();
}
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Intel Corporation");
module_init(nd_core_init);
module_exit(nd_core_exit);
