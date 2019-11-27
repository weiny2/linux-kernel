// SPDX-License-Identifier: GPL-2.0
/*
 * MSI framework for platform devices
 *
 * Copyright (C) 2015 ARM Limited, All Rights Reserved.
 * Author: Marc Zyngier <marc.zyngier@arm.com>
 */

#include <linux/device.h>
#include <linux/idr.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/msi.h>
#include <linux/slab.h>

#define DEV_ID_SHIFT	21
#define MAX_DEV_MSIS	(1 << (32 - DEV_ID_SHIFT))

/*
 * Internal data structure containing a (made up, but unique) devid
 * and the callback to write the MSI message.
 */
struct platform_msi_priv_data {
	struct device			*dev;
	void				*host_data;
	msi_alloc_info_t		arg;
	const struct platform_msi_ops	*ops;
	int				devid;
};

/* The devid allocator */
static DEFINE_IDA(platform_msi_devid_ida);

/*
 * Convert an msi_desc to a globaly unique identifier (per-device
 * devid + msi_desc position in the msi_list).
 */
irq_hw_number_t platform_msi_calc_hwirq(struct msi_desc *desc)
{
	u32 devid;

	devid = desc->platform.msi_priv_data->devid;

	return (devid << (32 - DEV_ID_SHIFT)) | desc->platform.msi_index;
}

#ifdef GENERIC_MSI_DOMAIN_OPS
static void platform_msi_set_desc(msi_alloc_info_t *arg, struct msi_desc *desc)
{
	arg->desc = desc;
	arg->hwirq = platform_msi_calc_hwirq(desc);
}

static int platform_msi_init(struct irq_domain *domain,
			     struct msi_domain_info *info,
			     unsigned int virq, irq_hw_number_t hwirq,
			     msi_alloc_info_t *arg)
{
	return irq_domain_set_hwirq_and_chip(domain, virq, hwirq,
					     info->chip, info->chip_data);
}
#else
#define platform_msi_set_desc		NULL
#define platform_msi_init		NULL
#endif

static void platform_msi_update_dom_ops(struct msi_domain_info *info)
{
	struct msi_domain_ops *ops = info->ops;

	BUG_ON(!ops);

	if (ops->msi_init == NULL)
		ops->msi_init = platform_msi_init;
	if (ops->set_desc == NULL)
		ops->set_desc = platform_msi_set_desc;
}

void platform_msi_write_msg(struct irq_data *data, struct msi_msg *msg)
{
	struct msi_desc *desc = irq_data_get_msi_desc(data);
	struct platform_msi_priv_data *priv_data;

	priv_data = desc->platform.msi_priv_data;

	priv_data->ops->write_msg(desc, msg);
}

static void platform_msi_update_chip_ops(struct msi_domain_info *info)
{
	struct irq_chip *chip = info->chip;

	BUG_ON(!chip);
	if (!chip->irq_mask)
		chip->irq_mask = irq_chip_mask_parent;
	if (!chip->irq_unmask)
		chip->irq_unmask = irq_chip_unmask_parent;
	if (!chip->irq_eoi)
		chip->irq_eoi = irq_chip_eoi_parent;
	if (!chip->irq_set_affinity)
		chip->irq_set_affinity = msi_domain_set_affinity;
	if (!chip->irq_write_msi_msg)
		chip->irq_write_msi_msg = platform_msi_write_msg;
	if (WARN_ON((info->flags & MSI_FLAG_LEVEL_CAPABLE) &&
		    !(chip->flags & IRQCHIP_SUPPORTS_LEVEL_MSI)))
		info->flags &= ~MSI_FLAG_LEVEL_CAPABLE;
}

static void platform_msi_free_descs(struct device *dev, int base, int nvec,
				    unsigned int group)
{
	struct msi_desc *desc, *tmp;
	struct platform_msi_group_entry *platform_msi_group,
					*tmp_platform_msi_group;

	list_for_each_entry_safe(platform_msi_group, tmp_platform_msi_group,
				 dev_to_platform_msi_group_list(dev),
				 group_list) {
		if (platform_msi_group->group_id == group) {
			list_for_each_entry_safe(desc, tmp,
						&platform_msi_group->entry_list,
						list) {
				if (desc->platform.msi_index >= base &&
				    desc->platform.msi_index < (base + nvec)) {
					list_del(&desc->list);
					free_msi_entry(desc);
				}
			}
			list_del(&platform_msi_group->group_list);
			kfree(platform_msi_group);
		}
	}
}

static int platform_msi_alloc_descs_with_irq(struct device *dev, int virq,
					     int nvec,
					     struct platform_msi_priv_data *data)

{
	struct msi_desc *desc;
	int i, base = 0;

	if (!list_empty(platform_msi_current_group_entry_list(dev))) {
		desc = list_last_entry(platform_msi_current_group_entry_list(dev),
				       struct msi_desc, list);
		base = desc->platform.msi_index + 1;
	}

	for (i = 0; i < nvec; i++) {
		desc = alloc_msi_entry(dev, 1, NULL);
		if (!desc)
			break;

		desc->platform.msi_priv_data = data;
		desc->platform.msi_index = base + i;
		desc->irq = virq ? virq + i : 0;

		list_add_tail(&desc->list,
			      platform_msi_current_group_entry_list(dev));
	}

	if (i != nvec) {
		/* Clean up the mess */
		platform_msi_free_descs(dev, base, nvec, dev->group_id);

		return -ENOMEM;
	}

	return 0;
}

static int platform_msi_alloc_descs(struct device *dev, int nvec,
				    struct platform_msi_priv_data *data)

{
	return platform_msi_alloc_descs_with_irq(dev, 0, nvec, data);
}

/**
 * platform_msi_create_irq_domain - Create a platform MSI interrupt domain
 * @fwnode:		Optional fwnode of the interrupt controller
 * @info:	MSI domain info
 * @parent:	Parent irq domain
 *
 * Updates the domain and chip ops and creates a platform MSI
 * interrupt domain.
 *
 * Returns:
 * A domain pointer or NULL in case of failure.
 */
struct irq_domain *platform_msi_create_irq_domain(struct fwnode_handle *fwnode,
						  struct msi_domain_info *info,
						  struct irq_domain *parent)
{
	struct irq_domain *domain;

	if (info->flags & MSI_FLAG_USE_DEF_DOM_OPS)
		platform_msi_update_dom_ops(info);
	if (info->flags & MSI_FLAG_USE_DEF_CHIP_OPS)
		platform_msi_update_chip_ops(info);

	domain = msi_create_irq_domain(fwnode, info, parent);
	if (domain)
		irq_domain_update_bus_token(domain, DOMAIN_BUS_PLATFORM_MSI);

	return domain;
}

static struct platform_msi_priv_data *
platform_msi_alloc_priv_data(struct device *dev, unsigned int nvec,
			     const struct platform_msi_ops *platform_ops)
{
	struct platform_msi_priv_data *datap;
	/*
	 * Limit the number of interrupts to 2048 per device. Should we
	 * need to bump this up, DEV_ID_SHIFT should be adjusted
	 * accordingly (which would impact the max number of MSI
	 * capable devices).
	 */
	if (!dev->msi_domain || !platform_ops->write_msg || !nvec ||
	    nvec > MAX_DEV_MSIS)
		return ERR_PTR(-EINVAL);

	if (dev->msi_domain->bus_token != DOMAIN_BUS_PLATFORM_MSI) {
		dev_err(dev, "Incompatible msi_domain, giving up\n");
		return ERR_PTR(-EINVAL);
	}

	/* Already had a helping of MSI? Greed... */
	if (!list_empty(platform_msi_current_group_entry_list(dev)))
		return ERR_PTR(-EBUSY);

	datap = kzalloc(sizeof(*datap), GFP_KERNEL);
	if (!datap)
		return ERR_PTR(-ENOMEM);

	datap->devid = ida_simple_get(&platform_msi_devid_ida,
				      0, 1 << DEV_ID_SHIFT, GFP_KERNEL);
	if (datap->devid < 0) {
		int err = datap->devid;
		kfree(datap);
		return ERR_PTR(err);
	}

	datap->ops = platform_ops;
	datap->dev = dev;

	return datap;
}

static void platform_msi_free_priv_data(struct platform_msi_priv_data *data)
{
	ida_simple_remove(&platform_msi_devid_ida, data->devid);
	kfree(data);
}

/**
 * platform_msi_domain_alloc_irqs - Allocate MSI interrupts for @dev
 * @dev:		The device for which to allocate interrupts
 * @nvec:		The number of interrupts to allocate
 * @write_msi_msg:	Callback to write an interrupt message for @dev
 *
 * Returns:
 * Zero for success, or an error code in case of failure
 */
int platform_msi_domain_alloc_irqs(struct device *dev, unsigned int nvec,
				   const struct platform_msi_ops *platform_ops)
{
	return platform_msi_domain_alloc_irqs_group(dev, nvec, platform_ops,
									NULL);
}
EXPORT_SYMBOL_GPL(platform_msi_domain_alloc_irqs);

int platform_msi_domain_alloc_irqs_group(struct device *dev, unsigned int nvec,
					 const struct platform_msi_ops *platform_ops,
					 unsigned int *group_id)
{
	struct platform_msi_group_entry *platform_msi_group;
	struct platform_msi_priv_data *priv_data;
	int err;

	dev->platform_msi_type = GEN_PLAT_MSI;

	if (group_id)
		*group_id = ++dev->group_id;

	platform_msi_group = kzalloc(sizeof(*platform_msi_group), GFP_KERNEL);
	if (!platform_msi_group) {
		err = -ENOMEM;
		goto out_platform_msi_group;
	}

	INIT_LIST_HEAD(&platform_msi_group->group_list);
	INIT_LIST_HEAD(&platform_msi_group->entry_list);
	platform_msi_group->group_id = dev->group_id;
	list_add_tail(&platform_msi_group->group_list,
		      dev_to_platform_msi_group_list(dev));

	priv_data = platform_msi_alloc_priv_data(dev, nvec, platform_ops);
	if (IS_ERR(priv_data))
		return PTR_ERR(priv_data);

	err = platform_msi_alloc_descs(dev, nvec, priv_data);
	if (err)
		goto out_free_priv_data;

	err = msi_domain_alloc_irqs(dev->msi_domain, dev, nvec);
	if (err)
		goto out_free_desc;

	return 0;

out_free_desc:
	platform_msi_free_descs(dev, 0, nvec, dev->group_id);
out_free_priv_data:
	platform_msi_free_priv_data(priv_data);
out_platform_msi_group:
	kfree(platform_msi_group);
	return err;
}
EXPORT_SYMBOL_GPL(platform_msi_domain_alloc_irqs_group);

/**
 * platform_msi_domain_free_irqs - Free MSI interrupts for @dev
 * @dev:	The device for which to free interrupts
 */
void platform_msi_domain_free_irqs(struct device *dev)
{
	platform_msi_domain_free_irqs_group(dev, 0);
}
EXPORT_SYMBOL_GPL(platform_msi_domain_free_irqs);

void platform_msi_domain_free_irqs_group(struct device *dev, unsigned int group)
{
	struct platform_msi_group_entry *platform_msi_group;

	list_for_each_entry(platform_msi_group,
			    dev_to_platform_msi_group_list((dev)), group_list) {
		if (platform_msi_group->group_id == group) {
			if (!list_empty(&platform_msi_group->entry_list)) {
				struct msi_desc *desc;

				desc = list_first_entry(&(platform_msi_group)->entry_list,
							struct msi_desc, list);
				platform_msi_free_priv_data(desc->platform.msi_priv_data);
			}
		}
	}
	msi_domain_free_irqs_group(dev->msi_domain, dev, group);
	platform_msi_free_descs(dev, 0, MAX_DEV_MSIS, group);
}
EXPORT_SYMBOL_GPL(platform_msi_domain_free_irqs_group);

/**
 * platform_msi_get_host_data - Query the private data associated with
 *                              a platform-msi domain
 * @domain:	The platform-msi domain
 *
 * Returns the private data provided when calling
 * platform_msi_create_device_domain.
 */
void *platform_msi_get_host_data(struct irq_domain *domain)
{
	struct platform_msi_priv_data *data = domain->host_data;
	return data->host_data;
}

/**
 * platform_msi_create_device_domain - Create a platform-msi domain
 *
 * @dev:		The device generating the MSIs
 * @nvec:		The number of MSIs that need to be allocated
 * @write_msi_msg:	Callback to write an interrupt message for @dev
 * @ops:		The hierarchy domain operations to use
 * @host_data:		Private data associated to this domain
 *
 * Returns an irqdomain for @nvec interrupts
 */
struct irq_domain *
__platform_msi_create_device_domain(struct device *dev,
				    unsigned int nvec,
				    bool is_tree,
				    const struct platform_msi_ops *platform_ops,
				    const struct irq_domain_ops *ops,
				    void *host_data)
{
	struct platform_msi_priv_data *data;
	struct irq_domain *domain;
	int err;

	data = platform_msi_alloc_priv_data(dev, nvec, platform_ops);
	if (IS_ERR(data))
		return NULL;

	data->host_data = host_data;
	domain = irq_domain_create_hierarchy(dev->msi_domain, 0,
					     is_tree ? 0 : nvec,
					     dev->fwnode, ops, data);
	if (!domain)
		goto free_priv;

	err = msi_domain_prepare_irqs(domain->parent, dev, nvec, &data->arg);
	if (err)
		goto free_domain;

	return domain;

free_domain:
	irq_domain_remove(domain);
free_priv:
	platform_msi_free_priv_data(data);
	return NULL;
}

/**
 * platform_msi_domain_free - Free interrupts associated with a platform-msi
 *                            domain
 *
 * @domain:	The platform-msi domain
 * @virq:	The base irq from which to perform the free operation
 * @nvec:	How many interrupts to free from @virq
 */
void platform_msi_domain_free(struct irq_domain *domain, unsigned int virq,
			      unsigned int nvec)
{
	struct platform_msi_priv_data *data = domain->host_data;
	struct msi_desc *desc, *tmp;
	struct platform_msi_group_entry *platform_msi_group,
					*tmp_platform_msi_group;

	list_for_each_entry_safe(platform_msi_group, tmp_platform_msi_group,
				 dev_to_platform_msi_group_list(data->dev),
				 group_list) {
		if (platform_msi_group->group_id == data->dev->group_id) {
			list_for_each_entry_safe(desc, tmp,
						&platform_msi_group->entry_list,
						list) {
				if (WARN_ON(!desc->irq || desc->nvec_used != 1))
					return;
				if (!(desc->irq >= virq && desc->irq < (virq + nvec)))
					continue;

				irq_domain_free_irqs_common(domain, desc->irq, 1);
				list_del(&desc->list);
				free_msi_entry(desc);
			}
			list_del(&platform_msi_group->group_list);
			kfree(platform_msi_group);
		}
	}
}

/**
 * platform_msi_domain_alloc - Allocate interrupts associated with
 *			       a platform-msi domain
 *
 * @domain:	The platform-msi domain
 * @virq:	The base irq from which to perform the allocate operation
 * @nvec:	How many interrupts to free from @virq
 *
 * Return 0 on success, or an error code on failure. Must be called
 * with irq_domain_mutex held (which can only be done as part of a
 * top-level interrupt allocation).
 */
int platform_msi_domain_alloc(struct irq_domain *domain, unsigned int virq,
			      unsigned int nr_irqs)
{
	struct platform_msi_priv_data *data = domain->host_data;
	int err;

	err = platform_msi_alloc_descs_with_irq(data->dev, virq, nr_irqs, data);
	if (err)
		return err;

	err = msi_domain_populate_irqs(domain->parent, data->dev,
				       virq, nr_irqs, &data->arg);
	if (err)
		platform_msi_domain_free(domain, virq, nr_irqs);

	return err;
}
