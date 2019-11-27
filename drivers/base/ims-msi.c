// SPDX-License-Identifier: GPL-2.0-only
/*
 * Support for Device Specific IMS interrupts.
 *
 * Copyright © 2019 Intel Corporation.
 *
 * Author: Megha Dey <megha.dey@intel.com>
 */

#include <linux/dmar.h>
#include <linux/irq.h>
#include <linux/mdev.h>
#include <linux/pci.h>

/*
 * Determine if a dev is mdev or not. Return NULL if not mdev device.
 * Return mdev's parent dev if success.
 */
static inline struct device *mdev_to_parent(struct device *dev)
{
	struct device *ret = NULL;
	struct device *(*fn)(struct device *dev);
	struct bus_type *bus = symbol_get(mdev_bus_type);

	if (bus && dev->bus == bus) {
		fn = symbol_get(mdev_dev_to_parent_dev);
		ret = fn(dev);
		symbol_put(mdev_dev_to_parent_dev);
		symbol_put(mdev_bus_type);
	}

	return ret;
}

static irq_hw_number_t dev_ims_get_hwirq(struct msi_domain_info *info,
					 msi_alloc_info_t *arg)
{
	return arg->ims_hwirq;
}

static int dev_ims_prepare(struct irq_domain *domain, struct device *dev,
			   int nvec, msi_alloc_info_t *arg)
{
	if (dev_is_mdev(dev))
		dev = mdev_to_parent(dev);

	init_irq_alloc_info(arg, NULL);
	arg->dev = dev;
	arg->type = X86_IRQ_ALLOC_TYPE_IMS;

	return 0;
}

static void dev_ims_set_desc(msi_alloc_info_t *arg, struct msi_desc *desc)
{
	arg->ims_hwirq = platform_msi_calc_hwirq(desc);
}

static struct msi_domain_ops dev_ims_domain_ops = {
	.get_hwirq	= dev_ims_get_hwirq,
	.msi_prepare	= dev_ims_prepare,
	.set_desc	= dev_ims_set_desc,
};

static struct irq_chip dev_ims_ir_controller = {
	.name			= "IR-DEV-IMS",
	.irq_ack		= irq_chip_ack_parent,
	.irq_retrigger		= irq_chip_retrigger_hierarchy,
	.irq_set_vcpu_affinity	= irq_chip_set_vcpu_affinity_parent,
	.flags			= IRQCHIP_SKIP_SET_WAKE,
	.irq_write_msi_msg	= platform_msi_write_msg,
};

static struct msi_domain_info ims_ir_domain_info = {
	.flags		= MSI_FLAG_USE_DEF_DOM_OPS | MSI_FLAG_USE_DEF_CHIP_OPS,
	.ops		= &dev_ims_domain_ops,
	.chip		= &dev_ims_ir_controller,
	.handler	= handle_edge_irq,
	.handler_name	= "edge",
};

struct irq_domain *arch_create_ims_irq_domain(struct irq_domain *parent,
					      const char *name)
{
	struct fwnode_handle *fn;
	struct irq_domain *domain;

	fn = irq_domain_alloc_named_fwnode(name);
	if (!fn)
		return NULL;

	domain = msi_create_irq_domain(fn, &ims_ir_domain_info, parent);
	if (!domain)
		return NULL;

	irq_domain_update_bus_token(domain, DOMAIN_BUS_PLATFORM_MSI);
	irq_domain_free_fwnode(fn);

	return domain;
}
