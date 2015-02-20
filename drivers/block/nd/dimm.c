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
#include <linux/sizes.h>
#include <linux/ndctl.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/nd.h>
#include "label.h"
#include "nd.h"

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
	struct resource *res;
	struct nd_dimm *nd_dimm = to_nd_dimm(dev);
	struct nd_blk_dimm *dimm = &nd_dimm->blk_dimm;
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
	struct nd_dimm *nd_dimm = to_nd_dimm(dev);
	struct nd_blk_dimm *dimm = &nd_dimm->blk_dimm;

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

static void free_data(struct nd_dimm *nd_dimm)
{
	if (!nd_dimm || !nd_dimm->data)
		return;

	if (nd_dimm->data && is_vmalloc_addr(nd_dimm->data))
		vfree(nd_dimm->data);
	else
		kfree(nd_dimm->data);
	nd_dimm->data = NULL;
}

static int nd_dimm_probe(struct device *dev)
{
	int header_size = sizeof(struct nfit_cmd_get_config_data_hdr);
	struct nd_dimm *nd_dimm = to_nd_dimm(dev);
	struct nfit_cmd_get_config_size cmd_size;
	int rc, cmd_get_config_size;

	nd_dimm->config_size = -EINVAL;
	rc = nd_dimm_get_config_size(nd_dimm, &cmd_size);
	if (rc || cmd_size.config_size + header_size > INT_MAX
			|| cmd_size.config_size < ND_LABEL_MIN_SIZE)
		return -ENXIO;

	nd_dimm->config_size = cmd_size.config_size;
	dev_dbg(&nd_dimm->dev, "config data size: %d\n", nd_dimm->config_size);
	cmd_get_config_size = cmd_size.config_size + header_size;
	nd_dimm->data = kmalloc(cmd_get_config_size, GFP_KERNEL);
	if (!nd_dimm->data)
		nd_dimm->data = vmalloc(cmd_get_config_size);

	if (!nd_dimm->data)
		return -ENOMEM;

	rc = nd_dimm_get_config_data(nd_dimm, nd_dimm->data,
			cmd_get_config_size);
	if (rc) {
		free_data(nd_dimm);
		return rc < 0 ? rc : -ENXIO;
	}

	nd_bus_lock(dev);
	nd_dimm->ns_current = nd_label_validate(nd_dimm);
	nd_dimm->ns_next = nd_label_next_nsindex(nd_dimm->ns_current);
	nd_label_copy(nd_dimm, to_next_namespace_index(nd_dimm),
			to_current_namespace_index(nd_dimm));
	rc = nd_label_reserve_dpa(nd_dimm);
	nd_bus_unlock(dev);

	nd_blk_wrapper_init(dev);
	return rc;
}

static int nd_dimm_remove(struct device *dev)
{
	struct nd_dimm *nd_dimm = to_nd_dimm(dev);

	nd_blk_wrapper_exit(dev);
	free_data(nd_dimm);

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
