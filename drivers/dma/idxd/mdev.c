// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2019,2020 Intel Corporation. All rights rsvd. */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/device.h>
#include <linux/io-64-nonatomic-lo-hi.h>
#include <linux/msi.h>
#include <linux/mdev.h>
#include <linux/vfio.h>
#include "../../vfio/pci/vfio_pci_private.h"
#include <uapi/linux/idxd.h>
#include "registers.h"
#include "idxd.h"
#include "mdev.h"

static void idxd_free_ims_index(struct idxd_device *idxd,
				unsigned long ims_idx)
{
	sbitmap_clear_bit(&idxd->ims_sbmap, ims_idx);
	atomic_dec(&idxd->num_allocated_ims);
}

static int vidxd_free_ims_entries(struct vdcm_idxd *vidxd)
{
	struct idxd_device *idxd = vidxd->idxd;
	struct ims_irq_entry *irq_entry;
	struct mdev_device *mdev = vidxd->vdev.mdev;
	struct device *dev = mdev_dev(mdev);
	struct msi_desc *desc;
	int i = 0;
	struct platform_msi_group_entry *platform_msi_group;

	for_each_platform_msi_entry_in_group(desc, platform_msi_group, 0, dev) {
		irq_entry = &vidxd->irq_entries[i];
		devm_free_irq(dev, desc->irq, irq_entry);
		i++;
	}

	platform_msi_domain_free_irqs(dev);

	for (i = 0; i < vidxd->num_wqs; i++)
		idxd_free_ims_index(idxd, vidxd->ims_index[i]);
	return 0;
}

static int idxd_alloc_ims_index(struct idxd_device *idxd)
{
	int index;

	index = sbitmap_get(&idxd->ims_sbmap, 0, false);
	if (index < 0)
		return -ENOSPC;
	return index;
}

static unsigned int idxd_ims_irq_mask(struct msi_desc *desc)
{
	int ims_offset;
	u32 mask_bits = desc->platform.masked;
	struct device *dev = desc->dev;
	struct mdev_device *mdev = mdev_from_dev(dev);
	struct vdcm_idxd *vidxd = mdev_get_drvdata(mdev);
	struct idxd_device *idxd = vidxd->idxd;
	void __iomem *base;
	int ims_id = desc->platform.msi_index;

	dev_dbg(dev, "idxd irq mask: %d\n", ims_id);

	mask_bits |= PCI_MSIX_ENTRY_CTRL_MASKBIT;
	ims_offset = idxd->ims_offset + vidxd->ims_index[ims_id] * 0x10;
	base = idxd->reg_base + ims_offset;
	iowrite32(mask_bits, base + PCI_MSIX_ENTRY_VECTOR_CTRL);

	return mask_bits;
}

static unsigned int idxd_ims_irq_unmask(struct msi_desc *desc)
{
	int ims_offset;
	u32 mask_bits = desc->platform.masked;
	struct device *dev = desc->dev;
	struct mdev_device *mdev = mdev_from_dev(dev);
	struct vdcm_idxd *vidxd = mdev_get_drvdata(mdev);
	struct idxd_device *idxd = vidxd->idxd;
	void __iomem *base;
	int ims_id = desc->platform.msi_index;

	dev_dbg(dev, "idxd irq unmask: %d\n", ims_id);

	mask_bits &= ~PCI_MSIX_ENTRY_CTRL_MASKBIT;
	ims_offset = idxd->ims_offset + vidxd->ims_index[ims_id] * 0x10;
	base = idxd->reg_base + ims_offset;
	iowrite32(mask_bits, base + PCI_MSIX_ENTRY_VECTOR_CTRL);

	return mask_bits;
}

static void idxd_ims_write_msg(struct msi_desc *desc, struct msi_msg *msg)
{
	int ims_offset;
	struct device *dev = desc->dev;
	struct mdev_device *mdev = mdev_from_dev(dev);
	struct vdcm_idxd *vidxd = mdev_get_drvdata(mdev);
	struct idxd_device *idxd = vidxd->idxd;
	void __iomem *base;
	int ims_id = desc->platform.msi_index;

	dev_dbg(dev, "ims_write: %d %x\n", ims_id, msg->address_lo);

	ims_offset = idxd->ims_offset + vidxd->ims_index[ims_id] * 0x10;
	base = idxd->reg_base + ims_offset;
	iowrite32(msg->address_lo, base + PCI_MSIX_ENTRY_LOWER_ADDR);
	iowrite32(msg->address_hi, base + PCI_MSIX_ENTRY_UPPER_ADDR);
	iowrite32(msg->data, base + PCI_MSIX_ENTRY_DATA);
}

static struct platform_msi_ops idxd_ims_ops  = {
	.irq_mask		= idxd_ims_irq_mask,
	.irq_unmask		= idxd_ims_irq_unmask,
	.write_msg		= idxd_ims_write_msg,
};

static irqreturn_t idxd_guest_wq_completion_interrupt(int irq, void *data)
{
	/* send virtual interrupt */
	return IRQ_HANDLED;
}

static int vidxd_setup_ims_entries(struct vdcm_idxd *vidxd)
{
	struct idxd_device *idxd = vidxd->idxd;
	struct ims_irq_entry *irq_entry;
	struct mdev_device *mdev = vidxd->vdev.mdev;
	struct device *dev = mdev_dev(mdev);
	struct msi_desc *desc;
	int err, i = 0;
	int group;
	struct platform_msi_group_entry *platform_msi_group;

	if (!atomic_add_unless(&idxd->num_allocated_ims, vidxd->num_wqs,
			       idxd->ims_size))
		return -ENOSPC;

	vidxd->ims_index[0] = idxd_alloc_ims_index(idxd);

	err = platform_msi_domain_alloc_irqs_group(dev, vidxd->num_wqs,
						   &idxd_ims_ops, &group);
	if (err < 0) {
		dev_dbg(dev, "Enabling IMS entry! %d\n", err);
		return err;
	}

	i = 0;
	for_each_platform_msi_entry_in_group(desc, platform_msi_group, group, dev) {
		irq_entry = &vidxd->irq_entries[i];
		irq_entry->vidxd = vidxd;
		irq_entry->int_src = i;
		err = devm_request_irq(dev, desc->irq,
				       idxd_guest_wq_completion_interrupt, 0,
				       "idxd-ims", irq_entry);
		if (err)
			break;
		i++;
	}

	if (err) {
		i = 0;
		for_each_platform_msi_entry_in_group(desc, platform_msi_group, group, dev) {
			irq_entry = &vidxd->irq_entries[i];
			devm_free_irq(dev, desc->irq, irq_entry);
			i++;
		}
		platform_msi_domain_free_irqs_group(dev, group);
	}

	return 0;
}
