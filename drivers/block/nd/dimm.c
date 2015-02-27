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
#include <linux/vmalloc.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/sizes.h>
#include <linux/ndctl.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/nd.h>
#include "label.h"
#include "nd.h"

static bool force_enable_dimms;
module_param(force_enable_dimms, bool, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(force_enable_dimms, "Ignore DIMM NFIT/firmware status");

struct block_window	*bw;
#define NUM_BW		256
static phys_addr_t	bw_apt_phys	= 0xf000a00000;
static phys_addr_t	bw_ctl_phys	= 0xf000800000;
static void		*bw_apt_virt;
static u64		*bw_ctl_virt;
static size_t		bw_size		= SZ_8K * NUM_BW;

static int nd_blk_init_locks(struct nd_blk_dimm *dimm)
{
	int i;

	dimm->bw_lock = kmalloc_array(dimm->num_bw, sizeof(spinlock_t),
				GFP_KERNEL);
	if (!dimm->bw_lock)
		return -ENOMEM;

	for (i = 0; i < dimm->num_bw; i++)
		spin_lock_init(&dimm->bw_lock[i]);

	return 0;
}

/* This code should do things that will eventually be moved into the rest of
 * ND.  This includes things like
 *	- ioremapping and iounmapping the BDWs and DCRs
 *	- initializing instances of the driver with proper parameters
 *	- when we do interleaving, implementing a generic interleaving method
 */
static int nd_blk_wrapper_init(struct device *dev)
{
	struct nd_dimm_drvdata *ndd = dev_get_drvdata(dev);
	struct nd_blk_dimm *dimm = &ndd->blk_dimm;
	struct resource *res;
	int err, i;

	if (!is_acpi_blk(dev))
		return 0;

	/* map block aperture memory */
	res = request_mem_region(bw_apt_phys, bw_size, "nd_blk");
	if (!res)
		return -EBUSY;

	bw_apt_virt = ioremap_cache(bw_apt_phys, bw_size);
	if (!bw_apt_virt) {
		err = -ENXIO;
		goto err_apt_ioremap;
	}

	/* map block control memory */
	res = request_mem_region(bw_ctl_phys, bw_size, "nd_blk");
	if (!res) {
		err = -EBUSY;
		goto err_ctl_reserve;
	}

	bw_ctl_virt = ioremap_cache(bw_ctl_phys, bw_size);
	if (!bw_ctl_virt) {
		err = -ENXIO;
		goto err_ctl_ioremap;
	}

	/* set up block windows */
	bw = kmalloc(sizeof(*bw) * NUM_BW, GFP_KERNEL);
	if (!bw) {
		err = -ENOMEM;
		goto err_bw;
	}

	for (i = 0; i < NUM_BW; i++) {
		bw[i].bw_apt_virt = (void *)bw_apt_virt + i*0x2000;
		bw[i].bw_ctl_virt = (void *)bw_ctl_virt + i*0x2000;
		bw[i].bw_stat_virt = (void *)bw[i].bw_ctl_virt + 0x1000;
	}


	dimm->num_bw		= NUM_BW;
	atomic_set(&dimm->last_bw, 0);
	dimm->bw		= bw;

	err = nd_blk_init_locks(dimm);
	if (err)
		goto err_init_locks;

	return 0;

err_init_locks:
	kfree(bw);
err_bw:
	iounmap(bw_ctl_virt);
err_ctl_ioremap:
	release_mem_region(bw_ctl_phys, bw_size);
err_ctl_reserve:
	iounmap(bw_apt_virt);
err_apt_ioremap:
	release_mem_region(bw_apt_phys, bw_size);
	return err;
}

static void nd_blk_wrapper_exit(struct device *dev)
{
	struct nd_dimm_drvdata *ndd = dev_get_drvdata(dev);
	struct nd_blk_dimm *dimm = &ndd->blk_dimm;

	if (!is_acpi_blk(dev))
		return;

	kfree(dimm->bw_lock);
	kfree(bw);

	/* free block control memory */
	iounmap(bw_ctl_virt);
	release_mem_region(bw_ctl_phys, bw_size);

	/* free block aperture memory */
	iounmap(bw_apt_virt);
	release_mem_region(bw_apt_phys, bw_size);
}

static void free_data(struct nd_dimm_drvdata *ndd)
{
	if (!ndd)
		return;

	if (ndd->data && is_vmalloc_addr(ndd->data))
		vfree(ndd->data);
	else
		kfree(ndd->data);
	kfree(ndd);
}

static int nd_dimm_probe(struct device *dev)
{
	struct nd_dimm_drvdata *ndd;
	int rc;

	rc = nd_dimm_firmware_status(dev);
	if (rc < 0) {
		dev_info(dev, "disabled by firmware: %d\n", rc);
		if (!force_enable_dimms)
			return rc;
	}

	ndd = kzalloc(sizeof(*ndd), GFP_KERNEL);
	if (!ndd)
		return -ENOMEM;

	dev_set_drvdata(dev, ndd);
	ndd->dpa.name = dev_name(dev);
	ndd->ns_current = -1;
	ndd->ns_next = -1;
	ndd->dpa.start = 0,
	ndd->dpa.end = -1,
	ndd->dev = dev;

	rc = nd_dimm_init_nsarea(ndd);
	if (rc)
		goto err;

	rc = nd_dimm_init_config_data(ndd);
	if (rc)
		goto err;

	dev_dbg(dev, "config data size: %d\n", ndd->nsarea.config_size);

	nd_bus_lock(dev);
	ndd->ns_current = nd_label_validate(ndd);
	ndd->ns_next = nd_label_next_nsindex(ndd->ns_current);
	nd_label_copy(ndd, to_next_namespace_index(ndd),
			to_current_namespace_index(ndd));
	rc = nd_label_reserve_dpa(ndd);
	nd_bus_unlock(dev);

	if (rc)
		goto err;

	rc = nd_blk_wrapper_init(dev);
	if (rc)
		goto err;

	return 0;

 err:
	free_data(ndd);
	return rc;
}

static int nd_dimm_remove(struct device *dev)
{
	struct nd_dimm_drvdata *ndd = dev_get_drvdata(dev);
	struct resource *res, *_r;

	nd_blk_wrapper_exit(dev);

	nd_bus_lock(dev);
	dev_set_drvdata(dev, NULL);
	for_each_dpa_resource_safe(ndd, res, _r)
		__release_region(&ndd->dpa, res->start, resource_size(res));
	nd_bus_unlock(dev);
	free_data(ndd);

	return 0;
}

static struct nd_device_driver nd_dimm_driver = {
	.probe = nd_dimm_probe,
	.remove = nd_dimm_remove,
	.drv = {
		.name = "nd_dimm",
	},
	.type = ND_DRIVER_DIMM,
};

int __init nd_dimm_init(void)
{
	return nd_driver_register(&nd_dimm_driver);
}

void nd_dimm_exit(void)
{
	driver_unregister(&nd_dimm_driver.drv);
}

MODULE_ALIAS_ND_DEVICE(ND_DEVICE_DIMM);
