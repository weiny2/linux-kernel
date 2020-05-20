// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2019 Intel Corporation. All rights rsvd. */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/device.h>
#include <linux/sched/task.h>
#include <linux/io-64-nonatomic-lo-hi.h>
#include <linux/mm.h>
#include <linux/mmu_context.h>
#include <linux/vfio.h>
#include <linux/mdev.h>
#include <linux/msi.h>
#include <linux/intel-iommu.h>
#include <linux/intel-svm.h>
#include <linux/kvm_host.h>
#include <linux/eventfd.h>
#include <linux/circ_buf.h>
#include <uapi/linux/idxd.h>
#include "registers.h"
#include "idxd.h"
#include "../../vfio/pci/vfio_pci_private.h"
#include "mdev.h"
#include "vdev.h"

static u64 idxd_pci_config[] = {
	0x001000000b258086ULL,
	0x0080000008800000ULL,
	0x000000000000000cULL,
	0x000000000000000cULL,
	0x0000000000000000ULL,
	0x2010808600000000ULL,
	0x0000004000000000ULL,
	0x000000ff00000000ULL,
	0x0000060000005011ULL, /* MSI-X capability */
	0x0000070000000000ULL,
	0x0000000000920010ULL, /* PCIe capability */
	0x0000000000000000ULL,
	0x0000000000000000ULL,
	0x0000000000000000ULL,
	0x0070001000000000ULL,
	0x0000000000000000ULL,
};

static u64 idxd_pci_ext_cap[] = {
	0x000000611101000fULL, /* ATS capability */
	0x0000000000000000ULL,
	0x8100000012010013ULL, /* Page Request capability */
	0x0000000000000001ULL,
	0x000014040001001bULL, /* PASID capability */
	0x0000000000000000ULL,
	0x0181808600010023ULL, /* Scalable IOV capability */
	0x0000000100000005ULL,
	0x0000000000000001ULL,
	0x0000000000000000ULL,
};

static u64 idxd_cap_ctrl_reg[] = {
	0x0000000000000100ULL,
	0x0000000000000000ULL,
	0x00000001013f038fULL, /* gencap */
	0x0000000000000000ULL,
	0x0000000000000000ULL,
	0x0000000000000000ULL,
	0x0000000000004004ULL, /* grpcap */
	0x0000000000000004ULL, /* engcap */
	0x00000001003f03ffULL, /* opcap */
	0x0000000000000000ULL,
	0x0000000000000000ULL,
	0x0000000000000000ULL,
	0x0000000000000000ULL, /* offsets */
};

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
	struct ims_irq_entry *irq_entry = data;
	struct vdcm_idxd *vidxd = irq_entry->vidxd;
	int msix_idx = irq_entry->int_src;

	vidxd_send_interrupt(vidxd, msix_idx + 1);
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

static inline bool handle_valid(unsigned long handle)
{
	return !!(handle & ~0xff);
}

static void idxd_vdcm_reinit(struct vdcm_idxd *vidxd)
{
	struct idxd_wq *wq;
	struct idxd_device *idxd;

	memset(vidxd->cfg, 0, VIDXD_MAX_CFG_SPACE_SZ);
	memset(&vidxd->bar0, 0, sizeof(struct vdcm_idxd_pci_bar0));

	memcpy(vidxd->cfg, idxd_pci_config, sizeof(idxd_pci_config));
	memcpy(vidxd->cfg + 0x100, idxd_pci_ext_cap,
	       sizeof(idxd_pci_ext_cap));

	memcpy(vidxd->bar0.cap_ctrl_regs, idxd_cap_ctrl_reg,
	       sizeof(idxd_cap_ctrl_reg));

	/* Set the MSI-X table size */
	vidxd->cfg[VIDXD_MSIX_TBL_SZ_OFFSET] = 1;
	idxd = vidxd->idxd;
	wq = vidxd->wq;

	if (wq_dedicated(wq))
		idxd_wq_disable(wq, NULL);

	vidxd_mmio_init(vidxd);
}

struct vfio_region {
	u32 type;
	u32 subtype;
	size_t size;
	u32 flags;
};

struct kvmidxd_guest_info {
	struct kvm *kvm;
	struct vdcm_idxd *vidxd;
};

static int kvmidxd_guest_init(struct mdev_device *mdev)
{
	struct kvmidxd_guest_info *info;
	struct vdcm_idxd *vidxd;
	struct kvm *kvm;
	struct device *dev = mdev_dev(mdev);

	vidxd = mdev_get_drvdata(mdev);
	if (handle_valid(vidxd->handle))
		return -EEXIST;

	kvm = vidxd->vdev.kvm;
	if (!kvm || kvm->mm != current->mm) {
		dev_err(dev, "KVM is required to use Intel vIDXD\n");
		return -ESRCH;
	}

	info = vzalloc(sizeof(*info));
	if (!info)
		return -ENOMEM;

	vidxd->handle = (unsigned long)info;
	info->vidxd = vidxd;
	info->kvm = kvm;

	return 0;
}

static bool kvmidxd_guest_exit(unsigned long handle)
{
	if (handle == 0)
		return false;

	vfree((void *)handle);

	return true;
}

static void __idxd_vdcm_release(struct vdcm_idxd *vidxd)
{
	int rc;
	struct device *dev = &vidxd->idxd->pdev->dev;

	if (atomic_cmpxchg(&vidxd->vdev.released, 0, 1))
		return;

	if (!handle_valid(vidxd->handle))
		return;

	/* Re-initialize the VIDXD to a pristine state for re-use */
	rc = vfio_unregister_notifier(mdev_dev(vidxd->vdev.mdev),
				      VFIO_GROUP_NOTIFY,
				      &vidxd->vdev.group_notifier);
	if (rc < 0)
		dev_warn(dev, "vfio_unregister_notifier group failed: %d\n",
			 rc);

	kvmidxd_guest_exit(vidxd->handle);
	vidxd_free_ims_entries(vidxd);

	vidxd->vdev.kvm = NULL;
	vidxd->handle = 0;
	idxd_vdcm_reinit(vidxd);
}

static void idxd_vdcm_release(struct mdev_device *mdev)
{
	struct vdcm_idxd *vidxd = mdev_get_drvdata(mdev);
	struct device *dev = mdev_dev(mdev);

	dev_dbg(dev, "vdcm_idxd_release %d\n", vidxd->type->type);
	__idxd_vdcm_release(vidxd);
}

static void idxd_vdcm_release_work(struct work_struct *work)
{
	struct vdcm_idxd *vidxd = container_of(work, struct vdcm_idxd,
					       vdev.release_work);

	__idxd_vdcm_release(vidxd);
}

static bool idxd_wq_match_uuid(struct idxd_wq *wq, const guid_t *uuid)
{
	struct idxd_wq_uuid *entry;
	bool found = false;

	list_for_each_entry(entry, &wq->uuid_list, list) {
		if (guid_equal(&entry->uuid, uuid)) {
			found = true;
			break;
		}
	}

	return found;
}

static struct idxd_wq *find_wq_by_uuid(struct idxd_device *idxd,
				       const guid_t *uuid)
{
	int i;
	struct idxd_wq *wq;
	bool found = false;

	for (i = 0; i < idxd->max_wqs; i++) {
		wq = &idxd->wqs[i];
		found = idxd_wq_match_uuid(wq, uuid);
		if (found)
			return wq;
	}

	return NULL;
}

static struct vdcm_idxd *vdcm_vidxd_create(struct idxd_device *idxd,
					   struct mdev_device *mdev,
					   struct vdcm_idxd_type *type)
{
	struct vdcm_idxd *vidxd;
	struct idxd_wq *wq = NULL;
	struct device *dev = mdev_dev(mdev);

	wq = find_wq_by_uuid(idxd, mdev_uuid(mdev));
	if (!wq) {
		dev_dbg(dev, "No WQ found\n");
		return NULL;
	}

	if (wq->state != IDXD_WQ_ENABLED)
		return NULL;

	vidxd = kzalloc(sizeof(*vidxd), GFP_KERNEL);
	if (!vidxd)
		return NULL;

	vidxd->idxd = idxd;
	vidxd->vdev.mdev = mdev;
	vidxd->wq = wq;
	mdev_set_drvdata(mdev, vidxd);
	vidxd->type = type;
	vidxd->num_wqs = 1;

	mutex_lock(&wq->wq_lock);
	/* disable wq. will be enabled by the VM */
	if (wq_dedicated(wq))
		idxd_wq_disable(vidxd->wq, NULL);

	/* Initialize virtual PCI resources if it is an MDEV type for a VM */
	memcpy(vidxd->cfg, idxd_pci_config, sizeof(idxd_pci_config));
	memcpy(vidxd->cfg + 0x100, idxd_pci_ext_cap,
	       sizeof(idxd_pci_ext_cap));
	memcpy(vidxd->bar0.cap_ctrl_regs, idxd_cap_ctrl_reg,
	       sizeof(idxd_cap_ctrl_reg));

	/* Set the MSI-X table size */
	vidxd->cfg[VIDXD_MSIX_TBL_SZ_OFFSET] = 1;
	vidxd->bar_size[0] = VIDXD_BAR0_SIZE;
	vidxd->bar_size[1] = VIDXD_BAR2_SIZE;

	vidxd_mmio_init(vidxd);

	INIT_WORK(&vidxd->vdev.release_work, idxd_vdcm_release_work);

	idxd_wq_get(wq);
	mutex_unlock(&wq->wq_lock);

	return vidxd;
}

static struct vdcm_idxd_type idxd_mdev_types[IDXD_MDEV_TYPES] = {
	{
		.name = "wq",
		.description = "IDXD MDEV workqueue",
		.type = IDXD_MDEV_TYPE_WQ,
	},
};

static struct vdcm_idxd_type *idxd_vdcm_find_vidxd_type(struct device *dev,
							const char *name)
{
	int i;
	char dev_name[IDXD_MDEV_NAME_LEN];

	for (i = 0; i < IDXD_MDEV_TYPES; i++) {
		snprintf(dev_name, IDXD_MDEV_NAME_LEN, "idxd-%s",
			 idxd_mdev_types[i].name);

		if (!strncmp(name, dev_name, IDXD_MDEV_NAME_LEN))
			return &idxd_mdev_types[i];
	}

	return NULL;
}

static int idxd_vdcm_create(struct kobject *kobj, struct mdev_device *mdev)
{
	struct vdcm_idxd *vidxd;
	struct vdcm_idxd_type *type;
	struct device *dev, *parent;
	struct idxd_device *idxd;
	int rc = 0;

	parent = mdev_parent_dev(mdev);
	idxd = dev_get_drvdata(parent);
	dev = mdev_dev(mdev);

	mdev_set_iommu_device(dev, parent);
	mutex_lock(&idxd->mdev_lock);
	type = idxd_vdcm_find_vidxd_type(dev, kobject_name(kobj));
	if (!type) {
		dev_err(dev, "failed to find type %s to create\n",
			kobject_name(kobj));
		rc = -EINVAL;
		goto out;
	}

	vidxd = vdcm_vidxd_create(idxd, mdev, type);
	if (IS_ERR_OR_NULL(vidxd)) {
		rc = !vidxd ? -ENOMEM : PTR_ERR(vidxd);
		dev_err(dev, "failed to create vidxd: %d\n", rc);
		goto out;
	}

	list_add(&vidxd->list, &vidxd->wq->vdcm_list);
	dev_dbg(dev, "mdev creation success: %s\n", dev_name(mdev_dev(mdev)));

 out:
	mutex_unlock(&idxd->mdev_lock);
	return rc;
}

static void vdcm_vidxd_remove(struct vdcm_idxd *vidxd)
{
	struct idxd_device *idxd = vidxd->idxd;
	struct device *dev = &idxd->pdev->dev;
	struct idxd_wq *wq = vidxd->wq;

	dev_dbg(dev, "%s: removing for wq %d\n", __func__, vidxd->wq->id);

	mutex_lock(&wq->wq_lock);
	list_del(&vidxd->list);
	idxd_wq_put(wq);
	mutex_unlock(&wq->wq_lock);
	kfree(vidxd);
}

static int idxd_vdcm_remove(struct mdev_device *mdev)
{
	struct vdcm_idxd *vidxd = mdev_get_drvdata(mdev);

	if (handle_valid(vidxd->handle))
		return -EBUSY;

	vdcm_vidxd_remove(vidxd);
	return 0;
}

static int idxd_vdcm_group_notifier(struct notifier_block *nb,
				    unsigned long action, void *data)
{
	struct vdcm_idxd *vidxd = container_of(nb, struct vdcm_idxd,
			vdev.group_notifier);

	/* The only action we care about */
	if (action == VFIO_GROUP_NOTIFY_SET_KVM) {
		vidxd->vdev.kvm = data;

		if (!data)
			schedule_work(&vidxd->vdev.release_work);
	}

	return NOTIFY_OK;
}

static int idxd_vdcm_open(struct mdev_device *mdev)
{
	struct vdcm_idxd *vidxd = mdev_get_drvdata(mdev);
	unsigned long events;
	int rc;
	struct vdcm_idxd_type *type = vidxd->type;
	struct device *dev = mdev_dev(mdev);

	dev_dbg(dev, "%s: type: %d\n", __func__, type->type);

	vidxd->vdev.group_notifier.notifier_call = idxd_vdcm_group_notifier;
	events = VFIO_GROUP_NOTIFY_SET_KVM;
	rc = vfio_register_notifier(mdev_dev(mdev), VFIO_GROUP_NOTIFY,
				    &events, &vidxd->vdev.group_notifier);
	if (rc < 0) {
		dev_err(dev, "vfio_register_notifier for group failed: %d\n",
			rc);
		return rc;
	}

	/* allocate and setup IMS entries */
	rc = vidxd_setup_ims_entries(vidxd);
	if (rc < 0)
		goto undo_group;

	rc = kvmidxd_guest_init(mdev);
	if (rc)
		goto undo_ims;

	atomic_set(&vidxd->vdev.released, 0);

	return rc;

 undo_ims:
	vidxd_free_ims_entries(vidxd);
 undo_group:
	vfio_unregister_notifier(mdev_dev(mdev), VFIO_GROUP_NOTIFY,
				 &vidxd->vdev.group_notifier);
	return rc;
}

static int vdcm_vidxd_mmio_write(struct vdcm_idxd *vidxd, u64 pos, void *buf,
				 unsigned int size)
{
	u32 offset = pos & (vidxd->bar_size[0] - 1);
	struct vdcm_idxd_pci_bar0 *bar0 = &vidxd->bar0;
	struct device *dev = mdev_dev(vidxd->vdev.mdev);

	dev_WARN_ONCE(dev, (size & (size - 1)) != 0, "%s\n", __func__);
	dev_WARN_ONCE(dev, size > 8, "%s\n", __func__);
	dev_WARN_ONCE(dev, (offset & (size - 1)) != 0, "%s\n", __func__);

	dev_dbg(dev, "vidxd mmio W %d %x %x: %llx\n", vidxd->wq->id, size,
		offset, get_reg_val(buf, size));

	/* If we don't limit this, we potentially can write out of bound */
	if (size > 8)
		size = 8;

	switch (offset) {
	case IDXD_GENCFG_OFFSET ... IDXD_GENCFG_OFFSET + 7:
		/* Write only when device is disabled. */
		if (vidxd_state(vidxd) == IDXD_DEVICE_STATE_DISABLED)
			memcpy(&bar0->cap_ctrl_regs[offset], buf, size);
		break;

	case IDXD_GENCTRL_OFFSET:
		memcpy(&bar0->cap_ctrl_regs[offset], buf, size);
		break;

	case IDXD_INTCAUSE_OFFSET:
		bar0->cap_ctrl_regs[offset] &= ~(get_reg_val(buf, 1) & 0x0f);
		break;

	case IDXD_CMD_OFFSET:
		if (size == 4) {
			u8 *cap_ctrl = &bar0->cap_ctrl_regs[0];
			unsigned long *cmdsts =
				(unsigned long *)&cap_ctrl[IDXD_CMDSTS_OFFSET];
			u32 val = get_reg_val(buf, size);

			/* Check and set device active */
			if (test_and_set_bit(31, cmdsts) == 0) {
				*(u32 *)cmdsts = 1 << 31;
				vidxd_do_command(vidxd, val);
			}
		}
		break;

	case IDXD_SWERR_OFFSET:
		/* W1C */
		bar0->cap_ctrl_regs[offset] &= ~(get_reg_val(buf, 1) & 3);
		break;

	case VIDXD_WQCFG_OFFSET ... VIDXD_WQCFG_OFFSET + VIDXD_WQ_CTRL_SZ - 1: {
		union wqcfg *wqcfg;
		int wq_id = (offset - VIDXD_WQCFG_OFFSET) / 0x20;
		struct idxd_wq *wq;
		int subreg = offset & 0x1c;
		u32 new_val;

		if (wq_id >= 1)
			break;
		wq = vidxd->wq;
		wqcfg = (union wqcfg *)&bar0->wq_ctrl_regs[wq_id * 0x20];
		if (size >= 4) {
			new_val = get_reg_val(buf, 4);
		} else {
			u32 tmp1, tmp2, shift, mask;

			switch (subreg) {
			case 4:
				tmp1 = wqcfg->bits[1]; break;
			case 8:
				tmp1 = wqcfg->bits[2]; break;
			case 12:
				tmp1 = wqcfg->bits[3]; break;
			case 16:
				tmp1 = wqcfg->bits[4]; break;
			case 20:
				tmp1 = wqcfg->bits[5]; break;
			default:
				tmp1 = 0;
			}

			tmp2 = get_reg_val(buf, size);
			shift = (offset & 0x03U) * 8;
			mask = ((1U << size * 8) - 1u) << shift;
			new_val = (tmp1 & ~mask) | (tmp2 << shift);
		}

		if (subreg == 8) {
			if (wqcfg->wq_state == 0) {
				wqcfg->bits[2] &= 0xfe;
				wqcfg->bits[2] |= new_val & 0xffffff01;
			}
		}

		break;
	}

	case VIDXD_MSIX_TABLE_OFFSET ...
		VIDXD_MSIX_TABLE_OFFSET + VIDXD_MSIX_TBL_SZ - 1: {
		int index = (offset - VIDXD_MSIX_TABLE_OFFSET) / 0x10;
		u8 *msix_entry = &bar0->msix_table[index * 0x10];
		u8 *msix_perm = &bar0->msix_perm_table[index * 8];
		int end;

		/* Upper bound checking to stop overflow */
		end = VIDXD_MSIX_TABLE_OFFSET + VIDXD_MSIX_TBL_SZ;
		if (offset + size > end)
			size = end - offset;

		memcpy(msix_entry + (offset & 0xf), buf, size);
		/* check mask and pba */
		if ((msix_entry[12] & 1) == 0) {
			*(u32 *)msix_perm &= ~3U;
			if (test_and_clear_bit(index, &bar0->msix_pba))
				vidxd_send_interrupt(vidxd, index);
		} else {
			*(u32 *)msix_perm |= 1;
		}
		break;
	}

	case VIDXD_MSIX_PERM_OFFSET ...
		VIDXD_MSIX_PERM_OFFSET + VIDXD_MSIX_PERM_TBL_SZ - 1:
		if ((offset & 7) == 0 && size == 4) {
			int index = (offset - VIDXD_MSIX_PERM_OFFSET) / 8;
			u32 *msix_perm =
				(u32 *)&bar0->msix_perm_table[index * 8];
			u8 *msix_entry = &bar0->msix_table[index * 0x10];
			u32 val = get_reg_val(buf, size) & 0xfffff00d;

			if (index > 0)
				vidxd_setup_ims_entry(vidxd, index - 1, val);

			if (val & 1) {
				msix_entry[12] |= 1;
				if (bar0->msix_pba & (1ULL << index))
					val |= 2;
			} else {
				msix_entry[12] &= ~1u;
				if (test_and_clear_bit(index,
						       &bar0->msix_pba))
					vidxd_send_interrupt(vidxd, index);
			}
			*msix_perm = val;
		}
		break;
	}

	return 0;
}

static int vdcm_vidxd_mmio_read(struct vdcm_idxd *vidxd, u64 pos, void *buf,
				unsigned int size)
{
	u32 offset = pos & (vidxd->bar_size[0] - 1);
	struct vdcm_idxd_pci_bar0 *bar0 = &vidxd->bar0;
	u8 *reg_addr, *msix_table, *msix_perm_table;
	struct device *dev = mdev_dev(vidxd->vdev.mdev);
	u32 end;

	dev_WARN_ONCE(dev, (size & (size - 1)) != 0, "%s\n", __func__);
	dev_WARN_ONCE(dev, size > 8, "%s\n", __func__);
	dev_WARN_ONCE(dev, (offset & (size - 1)) != 0, "%s\n", __func__);

	/* If we don't limit this, we potentially can write out of bound */
	if (size > 8)
		size = 8;

	switch (offset) {
	case 0 ... VIDXD_CAP_CTRL_SZ - 1:
		end = VIDXD_CAP_CTRL_SZ;
		if (offset + 8 > end)
			size = end - offset;
		reg_addr = &bar0->cap_ctrl_regs[offset];
		break;

	case VIDXD_GRPCFG_OFFSET ...
		VIDXD_GRPCFG_OFFSET + VIDXD_GRP_CTRL_SZ - 1:
		end = VIDXD_GRPCFG_OFFSET + VIDXD_GRP_CTRL_SZ;
		if (offset + 8 > end)
			size = end - offset;
		reg_addr = &bar0->grp_ctrl_regs[offset - VIDXD_GRPCFG_OFFSET];
		break;

	case VIDXD_WQCFG_OFFSET ... VIDXD_WQCFG_OFFSET + VIDXD_WQ_CTRL_SZ - 1:
		end = VIDXD_WQCFG_OFFSET + VIDXD_WQ_CTRL_SZ;
		if (offset + 8 > end)
			size = end - offset;
		reg_addr = &bar0->wq_ctrl_regs[offset - VIDXD_WQCFG_OFFSET];
		break;

	case VIDXD_MSIX_TABLE_OFFSET ...
		VIDXD_MSIX_TABLE_OFFSET + VIDXD_MSIX_TBL_SZ - 1:
		end = VIDXD_MSIX_TABLE_OFFSET + VIDXD_MSIX_TBL_SZ;
		if (offset + 8 > end)
			size = end - offset;
		msix_table = &bar0->msix_table[0];
		reg_addr = &msix_table[offset - VIDXD_MSIX_TABLE_OFFSET];
		break;

	case VIDXD_MSIX_PBA_OFFSET ... VIDXD_MSIX_PBA_OFFSET + 7:
		end = VIDXD_MSIX_PBA_OFFSET + 8;
		if (offset + 8 > end)
			size = end - offset;
		reg_addr = (u8 *)&bar0->msix_pba;
		break;

	case VIDXD_MSIX_PERM_OFFSET ...
		VIDXD_MSIX_PERM_OFFSET + VIDXD_MSIX_PERM_TBL_SZ - 1:
		end = VIDXD_MSIX_PERM_OFFSET + VIDXD_MSIX_PERM_TBL_SZ;
		if (offset + 8 > end)
			size = end - offset;
		msix_perm_table = &bar0->msix_perm_table[0];
		reg_addr = &msix_perm_table[offset - VIDXD_MSIX_PERM_OFFSET];
		break;

	default:
		reg_addr = NULL;
		break;
	}

	if (reg_addr)
		memcpy(buf, reg_addr, size);
	else
		memset(buf, 0, size);

	dev_dbg(dev, "vidxd mmio R %d %x %x: %llx\n",
		vidxd->wq->id, size, offset, get_reg_val(buf, size));
	return 0;
}

static int vdcm_vidxd_cfg_read(struct vdcm_idxd *vidxd, unsigned int pos,
			       void *buf, unsigned int count)
{
	u32 offset = pos & 0xfff;
	struct device *dev = mdev_dev(vidxd->vdev.mdev);

	memcpy(buf, &vidxd->cfg[offset], count);

	dev_dbg(dev, "vidxd pci R %d %x %x: %llx\n",
		vidxd->wq->id, count, offset, get_reg_val(buf, count));

	return 0;
}

static int vdcm_vidxd_cfg_write(struct vdcm_idxd *vidxd, unsigned int pos,
				void *buf, unsigned int size)
{
	u32 offset = pos & 0xfff;
	u64 val;
	u8 *cfg = vidxd->cfg;
	u8 *bar0 = vidxd->bar0.cap_ctrl_regs;
	struct device *dev = mdev_dev(vidxd->vdev.mdev);

	dev_dbg(dev, "vidxd pci W %d %x %x: %llx\n", vidxd->wq->id, size,
		offset, get_reg_val(buf, size));

	switch (offset) {
	case PCI_COMMAND: { /* device control */
		bool bme;

		memcpy(&cfg[offset], buf, size);
		bme = cfg[offset] & PCI_COMMAND_MASTER;
		if (!bme &&
		    ((*(u32 *)&bar0[IDXD_GENSTATS_OFFSET]) & 0x3) != 0) {
			*(u32 *)(&bar0[IDXD_SWERR_OFFSET]) = 0x51u << 8;
			*(u32 *)(&bar0[IDXD_GENSTATS_OFFSET]) = 0;
		}

		if (size < 4)
			break;
		offset += 2;
		buf = buf + 2;
		size -= 2;
	}
	/* fall through */

	case PCI_STATUS: { /* device status */
		u16 nval = get_reg_val(buf, size) << (offset & 1) * 8;

		nval &= 0xf900;
		*(u16 *)&cfg[offset] = *((u16 *)&cfg[offset]) & ~nval;
		break;
	}

	case PCI_CACHE_LINE_SIZE:
	case PCI_INTERRUPT_LINE:
		memcpy(&cfg[offset], buf, size);
		break;

	case PCI_BASE_ADDRESS_0: /* BAR0 */
	case PCI_BASE_ADDRESS_1: /* BAR1 */
	case PCI_BASE_ADDRESS_2: /* BAR2 */
	case PCI_BASE_ADDRESS_3: /* BAR3 */ {
		unsigned int bar_id, bar_offset;
		u64 bar, bar_size;

		bar_id = (offset - PCI_BASE_ADDRESS_0) / 8;
		bar_size = vidxd->bar_size[bar_id];
		bar_offset = PCI_BASE_ADDRESS_0 + bar_id * 8;

		val = get_reg_val(buf, size);
		bar = *(u64 *)&cfg[bar_offset];
		memcpy((u8 *)&bar + (offset & 0x7), buf, size);
		bar &= ~(bar_size - 1);

		*(u64 *)&cfg[bar_offset] = bar |
			PCI_BASE_ADDRESS_MEM_TYPE_64 |
			PCI_BASE_ADDRESS_MEM_PREFETCH;

		if (val == -1U || val == -1ULL)
			break;
		if (bar == 0 || bar == -1ULL - -1U)
			break;
		if (bar == (-1U & ~(bar_size - 1)))
			break;
		if (bar == (-1ULL & ~(bar_size - 1)))
			break;
		if (bar == vidxd->bar_val[bar_id])
			break;

		vidxd->bar_val[bar_id] = bar;
		break;
	}

	case VIDXD_ATS_OFFSET + 4:
		if (size < 4)
			break;
		offset += 2;
		buf = buf + 2;
		size -= 2;
		/* fall through */

	case VIDXD_ATS_OFFSET + 6:
		memcpy(&cfg[offset], buf, size);
		break;

	case VIDXD_PRS_OFFSET + 4: {
		u8 old_val, new_val;

		val = get_reg_val(buf, 1);
		old_val = cfg[VIDXD_PRS_OFFSET + 4];
		new_val = val & 1;

		cfg[offset] = new_val;
		if (old_val == 0 && new_val == 1) {
			/*
			 * Clear Stopped, Response Failure,
			 * and Unexpected Response.
			 */
			*(u16 *)&cfg[VIDXD_PRS_OFFSET + 6] &= ~(u16)(0x0103);
		}

		if (size < 4)
			break;

		offset += 2;
		buf = (u8 *)buf + 2;
		size -= 2;
	}
	/* fall through */

	case VIDXD_PRS_OFFSET + 6:
		cfg[offset] &= ~(get_reg_val(buf, 1) & 3);
		break;
	case VIDXD_PRS_OFFSET + 12 ... VIDXD_PRS_OFFSET + 15:
		memcpy(&cfg[offset], buf, size);
		break;

	case VIDXD_PASID_OFFSET + 4:
		if (size < 4)
			break;
		offset += 2;
		buf = buf + 2;
		size -= 2;
		/* fall through */
	case VIDXD_PASID_OFFSET + 6:
		cfg[offset] = get_reg_val(buf, 1) & 5;
		break;
	}

	return 0;
}

static ssize_t idxd_vdcm_rw(struct mdev_device *mdev, char *buf,
			    size_t count, loff_t *ppos, enum idxd_vdcm_rw mode)
{
	struct vdcm_idxd *vidxd = mdev_get_drvdata(mdev);
	unsigned int index = VFIO_PCI_OFFSET_TO_INDEX(*ppos);
	u64 pos = *ppos & VFIO_PCI_OFFSET_MASK;
	struct device *dev = mdev_dev(mdev);
	int rc = -EINVAL;

	if (index >= VFIO_PCI_NUM_REGIONS) {
		dev_err(dev, "invalid index: %u\n", index);
		return -EINVAL;
	}

	switch (index) {
	case VFIO_PCI_CONFIG_REGION_INDEX:
		if (mode == IDXD_VDCM_WRITE)
			rc = vdcm_vidxd_cfg_write(vidxd, pos, buf, count);
		else
			rc = vdcm_vidxd_cfg_read(vidxd, pos, buf, count);
		break;
	case VFIO_PCI_BAR0_REGION_INDEX:
	case VFIO_PCI_BAR1_REGION_INDEX:
		if (mode == IDXD_VDCM_WRITE)
			rc = vdcm_vidxd_mmio_write(vidxd,
						   vidxd->bar_val[0] + pos, buf,
						   count);
		else
			rc = vdcm_vidxd_mmio_read(vidxd,
						  vidxd->bar_val[0] + pos, buf,
						  count);
		break;
	case VFIO_PCI_BAR2_REGION_INDEX:
	case VFIO_PCI_BAR3_REGION_INDEX:
	case VFIO_PCI_BAR4_REGION_INDEX:
	case VFIO_PCI_BAR5_REGION_INDEX:
	case VFIO_PCI_VGA_REGION_INDEX:
	case VFIO_PCI_ROM_REGION_INDEX:
	default:
		dev_err(dev, "unsupported region: %u\n", index);
	}

	return rc == 0 ? count : rc;
}

static ssize_t idxd_vdcm_read(struct mdev_device *mdev, char __user *buf,
			      size_t count, loff_t *ppos)
{
	unsigned int done = 0;
	int rc;

	while (count) {
		size_t filled;

		if (count >= 8 && !(*ppos % 8)) {
			u64 val;

			rc = idxd_vdcm_rw(mdev, (char *)&val, sizeof(val),
					  ppos, IDXD_VDCM_READ);
			if (rc <= 0)
				goto read_err;

			if (copy_to_user(buf, &val, sizeof(val)))
				goto read_err;

			filled = 8;
		} else if (count >= 4 && !(*ppos % 4)) {
			u32 val;

			rc = idxd_vdcm_rw(mdev, (char *)&val, sizeof(val),
					  ppos, IDXD_VDCM_READ);
			if (rc <= 0)
				goto read_err;

			if (copy_to_user(buf, &val, sizeof(val)))
				goto read_err;

			filled = 4;
		} else if (count >= 2 && !(*ppos % 2)) {
			u16 val;

			rc = idxd_vdcm_rw(mdev, (char *)&val, sizeof(val),
					  ppos, IDXD_VDCM_READ);
			if (rc <= 0)
				goto read_err;

			if (copy_to_user(buf, &val, sizeof(val)))
				goto read_err;

			filled = 2;
		} else {
			u8 val;

			rc = idxd_vdcm_rw(mdev, &val, sizeof(val), ppos,
					  IDXD_VDCM_READ);
			if (rc <= 0)
				goto read_err;

			if (copy_to_user(buf, &val, sizeof(val)))
				goto read_err;

			filled = 1;
		}

		count -= filled;
		done += filled;
		*ppos += filled;
		buf += filled;
	}

	return done;

 read_err:
	return -EFAULT;
}

static ssize_t idxd_vdcm_write(struct mdev_device *mdev,
			       const char __user *buf, size_t count,
			       loff_t *ppos)
{
	unsigned int done = 0;
	int rc;

	while (count) {
		size_t filled;

		if (count >= 8 && !(*ppos % 8)) {
			u64 val;

			if (copy_from_user(&val, buf, sizeof(val)))
				goto write_err;

			rc = idxd_vdcm_rw(mdev, (char *)&val, sizeof(val),
					  ppos, IDXD_VDCM_WRITE);
			if (rc <= 0)
				goto write_err;

			filled = 8;
		} else if (count >= 4 && !(*ppos % 4)) {
			u32 val;

			if (copy_from_user(&val, buf, sizeof(val)))
				goto write_err;

			rc = idxd_vdcm_rw(mdev, (char *)&val, sizeof(val),
					  ppos, IDXD_VDCM_WRITE);
			if (rc <= 0)
				goto write_err;

			filled = 4;
		} else if (count >= 2 && !(*ppos % 2)) {
			u16 val;

			if (copy_from_user(&val, buf, sizeof(val)))
				goto write_err;

			rc = idxd_vdcm_rw(mdev, (char *)&val,
					  sizeof(val), ppos, IDXD_VDCM_WRITE);
			if (rc <= 0)
				goto write_err;

			filled = 2;
		} else {
			u8 val;

			if (copy_from_user(&val, buf, sizeof(val)))
				goto write_err;

			rc = idxd_vdcm_rw(mdev, &val, sizeof(val),
					  ppos, IDXD_VDCM_WRITE);
			if (rc <= 0)
				goto write_err;

			filled = 1;
		}

		count -= filled;
		done += filled;
		*ppos += filled;
		buf += filled;
	}

	return done;
write_err:
	return -EFAULT;
}

static int check_vma(struct idxd_wq *wq, struct vm_area_struct *vma,
		     const char *func)
{
	if (vma->vm_end < vma->vm_start)
		return -EINVAL;
	if (!(vma->vm_flags & VM_SHARED))
		return -EINVAL;

	return 0;
}

static int idxd_vdcm_mmap(struct mdev_device *mdev, struct vm_area_struct *vma)
{
	unsigned int wq_idx, rc;
	unsigned long req_size, pgoff = 0, offset;
	pgprot_t pg_prot;
	struct vdcm_idxd *vidxd = mdev_get_drvdata(mdev);
	struct idxd_wq *wq = vidxd->wq;
	struct idxd_device *idxd = vidxd->idxd;
	enum idxd_portal_prot virt_limited, phys_limited;
	phys_addr_t base = pci_resource_start(idxd->pdev, IDXD_WQ_BAR);
	struct device *dev = mdev_dev(mdev);

	rc = check_vma(wq, vma, __func__);
	if (rc)
		return rc;

	pg_prot = vma->vm_page_prot;
	req_size = vma->vm_end - vma->vm_start;
	vma->vm_flags |= VM_DONTCOPY;

	offset = (vma->vm_pgoff << PAGE_SHIFT) &
		 ((1ULL << VFIO_PCI_OFFSET_SHIFT) - 1);

	wq_idx = offset >> (PAGE_SHIFT + 2);
	if (wq_idx >= 1) {
		dev_err(dev, "mapping invalid wq %d off %lx\n",
			wq_idx, offset);
		return -EINVAL;
	}

	virt_limited = ((offset >> PAGE_SHIFT) & 0x3) == 1;
	phys_limited = IDXD_PORTAL_LIMITED;

	if (virt_limited == IDXD_PORTAL_UNLIMITED && wq_dedicated(wq))
		phys_limited = IDXD_PORTAL_UNLIMITED;

	/* We always map IMS portals to the guest */
	pgoff = (base +
		idxd_get_wq_portal_full_offset(wq->id, phys_limited,
					       IDXD_IRQ_IMS)) >> PAGE_SHIFT;

	dev_dbg(dev, "mmap %lx %lx %lx %lx\n", vma->vm_start, pgoff, req_size,
		pgprot_val(pg_prot));
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_private_data = mdev;
	vma->vm_pgoff = pgoff;
	vma->vm_private_data = mdev;

	return remap_pfn_range(vma, vma->vm_start, pgoff, req_size, pg_prot);
}

static int idxd_vdcm_get_irq_count(struct vdcm_idxd *vidxd, int type)
{
	if (type == VFIO_PCI_MSI_IRQ_INDEX ||
	    type == VFIO_PCI_MSIX_IRQ_INDEX)
		return vidxd->num_wqs + 1;

	return 0;
}

static int vdcm_idxd_set_msix_trigger(struct vdcm_idxd *vidxd,
				      unsigned int index, unsigned int start,
				      unsigned int count, uint32_t flags,
				      void *data)
{
	struct eventfd_ctx *trigger;
	int i, rc = 0;
	struct mdev_device *mdev = vidxd->vdev.mdev;
	struct device *dev = mdev_dev(mdev);

	if (count > VIDXD_MAX_MSIX_ENTRIES - 1)
		count = VIDXD_MAX_MSIX_ENTRIES - 1;

	if (count == 0 && (flags & VFIO_IRQ_SET_DATA_NONE)) {
		/* Disable all MSIX entries */
		for (i = 0; i < VIDXD_MAX_MSIX_ENTRIES; i++) {
			if (vidxd->vdev.msix_trigger[i]) {
				dev_dbg(dev, "disable MSIX entry %d\n", i);
				eventfd_ctx_put(vidxd->vdev.msix_trigger[i]);
				vidxd->vdev.msix_trigger[i] = 0;

				if (i) {
					rc = vidxd_free_ims_entry(vidxd, i - 1);
					if (rc)
						return rc;
				}
			}
		}
		return 0;
	}

	for (i = 0; i < count; i++) {
		if (flags & VFIO_IRQ_SET_DATA_EVENTFD) {
			u32 fd = *(u32 *)(data + i * sizeof(u32));

			dev_dbg(dev, "enable MSIX entry %d\n", i);
			trigger = eventfd_ctx_fdget(fd);
			if (IS_ERR(trigger)) {
				pr_err("eventfd_ctx_fdget failed %d\n", i);
				return PTR_ERR(trigger);
			}
			vidxd->vdev.msix_trigger[i] = trigger;
			/*
			 * Allocate a vector from the OS and set in the IMS
			 * entry
			 */
			if (i) {
				rc = vidxd_setup_ims_entry(vidxd, i - 1, 0);
				if (rc)
					return rc;
			}
			fd++;
		} else if (flags & VFIO_IRQ_SET_DATA_NONE) {
			dev_dbg(dev, "disable MSIX entry %d\n", i);
			eventfd_ctx_put(vidxd->vdev.msix_trigger[i]);
			vidxd->vdev.msix_trigger[i] = 0;

			if (i) {
				rc = vidxd_free_ims_entry(vidxd, i - 1);
				if (rc)
					return rc;
			}
		}
	}
	return rc;
}

static int idxd_vdcm_set_irqs(struct vdcm_idxd *vidxd, uint32_t flags,
			      unsigned int index, unsigned int start,
			      unsigned int count, void *data)
{
	int (*func)(struct vdcm_idxd *vidxd, unsigned int index,
		    unsigned int start, unsigned int count, uint32_t flags,
		    void *data) = NULL;
	struct mdev_device *mdev = vidxd->vdev.mdev;
	struct device *dev = mdev_dev(mdev);
	int msixcnt = pci_msix_vec_count(vidxd->idxd->pdev);

	if (msixcnt < 0)
		return -ENXIO;

	switch (index) {
	case VFIO_PCI_INTX_IRQ_INDEX:
		dev_warn(dev, "intx interrupts not supported.\n");
		break;
	case VFIO_PCI_MSI_IRQ_INDEX:
		dev_dbg(dev, "msi interrupt.\n");
		switch (flags & VFIO_IRQ_SET_ACTION_TYPE_MASK) {
		case VFIO_IRQ_SET_ACTION_MASK:
		case VFIO_IRQ_SET_ACTION_UNMASK:
			break;
		case VFIO_IRQ_SET_ACTION_TRIGGER:
			func = vdcm_idxd_set_msix_trigger;
			break;
		}
		break;
	case VFIO_PCI_MSIX_IRQ_INDEX:
		switch (flags & VFIO_IRQ_SET_ACTION_TYPE_MASK) {
		case VFIO_IRQ_SET_ACTION_MASK:
		case VFIO_IRQ_SET_ACTION_UNMASK:
			break;
		case VFIO_IRQ_SET_ACTION_TRIGGER:
			func = vdcm_idxd_set_msix_trigger;
			break;
		}
		break;
	default:
		return -ENOTTY;
	}

	if (!func)
		return -ENOTTY;

	return func(vidxd, index, start, count, flags, data);
}

static void vidxd_vdcm_reset(struct vdcm_idxd *vidxd)
{
	vidxd_reset(vidxd);
}

static long idxd_vdcm_ioctl(struct mdev_device *mdev, unsigned int cmd,
			    unsigned long arg)
{
	struct vdcm_idxd *vidxd = mdev_get_drvdata(mdev);
	unsigned long minsz;
	int rc = -EINVAL;
	struct device *dev = mdev_dev(mdev);

	dev_dbg(dev, "vidxd %lx ioctl, cmd: %d\n", vidxd->handle, cmd);

	if (cmd == VFIO_DEVICE_GET_INFO) {
		struct vfio_device_info info;

		minsz = offsetofend(struct vfio_device_info, num_irqs);

		if (copy_from_user(&info, (void __user *)arg, minsz))
			return -EFAULT;

		if (info.argsz < minsz)
			return -EINVAL;

		info.flags = VFIO_DEVICE_FLAGS_PCI;
		info.flags |= VFIO_DEVICE_FLAGS_RESET;
		info.num_regions = VFIO_PCI_NUM_REGIONS;
		info.num_irqs = VFIO_PCI_NUM_IRQS;

		return copy_to_user((void __user *)arg, &info, minsz) ?
			-EFAULT : 0;

	} else if (cmd == VFIO_DEVICE_GET_REGION_INFO) {
		struct vfio_region_info info;
		struct vfio_info_cap caps = { .buf = NULL, .size = 0 };
		int i;
		struct vfio_region_info_cap_sparse_mmap *sparse = NULL;
		size_t size;
		int nr_areas = 1;
		int cap_type_id = 0;

		minsz = offsetofend(struct vfio_region_info, offset);

		if (copy_from_user(&info, (void __user *)arg, minsz))
			return -EFAULT;

		if (info.argsz < minsz)
			return -EINVAL;

		switch (info.index) {
		case VFIO_PCI_CONFIG_REGION_INDEX:
			info.offset = VFIO_PCI_INDEX_TO_OFFSET(info.index);
			info.size = VIDXD_MAX_CFG_SPACE_SZ;
			info.flags = VFIO_REGION_INFO_FLAG_READ |
				     VFIO_REGION_INFO_FLAG_WRITE;
			break;
		case VFIO_PCI_BAR0_REGION_INDEX:
			info.offset = VFIO_PCI_INDEX_TO_OFFSET(info.index);
			info.size = vidxd->bar_size[info.index];
			if (!info.size) {
				info.flags = 0;
				break;
			}

			info.flags = VFIO_REGION_INFO_FLAG_READ |
				     VFIO_REGION_INFO_FLAG_WRITE;
			break;
		case VFIO_PCI_BAR1_REGION_INDEX:
			info.offset = VFIO_PCI_INDEX_TO_OFFSET(info.index);
			info.size = 0;
			info.flags = 0;
			break;
		case VFIO_PCI_BAR2_REGION_INDEX:
			info.offset = VFIO_PCI_INDEX_TO_OFFSET(info.index);
			info.flags = VFIO_REGION_INFO_FLAG_CAPS |
				VFIO_REGION_INFO_FLAG_MMAP |
				VFIO_REGION_INFO_FLAG_READ |
				VFIO_REGION_INFO_FLAG_WRITE;
			info.size = vidxd->bar_size[1];

			/*
			 * Every WQ has two areas for unlimited and limited
			 * MSI-X portals. IMS portals are not reported
			 */
			nr_areas = 2;

			size = sizeof(*sparse) +
				(nr_areas * sizeof(*sparse->areas));
			sparse = kzalloc(size, GFP_KERNEL);
			if (!sparse)
				return -ENOMEM;

			sparse->header.id = VFIO_REGION_INFO_CAP_SPARSE_MMAP;
			sparse->header.version = 1;
			sparse->nr_areas = nr_areas;
			cap_type_id = VFIO_REGION_INFO_CAP_SPARSE_MMAP;

			sparse->areas[0].offset = 0;
			sparse->areas[0].size = PAGE_SIZE;

			sparse->areas[1].offset = PAGE_SIZE;
			sparse->areas[1].size = PAGE_SIZE;
			break;

		case VFIO_PCI_BAR3_REGION_INDEX ... VFIO_PCI_BAR5_REGION_INDEX:
			info.offset = VFIO_PCI_INDEX_TO_OFFSET(info.index);
			info.size = 0;
			info.flags = 0;
			dev_dbg(dev, "get region info bar:%d\n", info.index);
			break;

		case VFIO_PCI_ROM_REGION_INDEX:
		case VFIO_PCI_VGA_REGION_INDEX:
			dev_dbg(dev, "get region info index:%d\n",
				info.index);
			break;
		default: {
			struct vfio_region_info_cap_type cap_type = {
				.header.id = VFIO_REGION_INFO_CAP_TYPE,
				.header.version = 1
			};

			if (info.index >= VFIO_PCI_NUM_REGIONS +
					vidxd->vdev.num_regions)
				return -EINVAL;

			i = info.index - VFIO_PCI_NUM_REGIONS;

			info.offset = VFIO_PCI_INDEX_TO_OFFSET(info.index);
			info.size = vidxd->vdev.region[i].size;
			info.flags = vidxd->vdev.region[i].flags;

			cap_type.type = vidxd->vdev.region[i].type;
			cap_type.subtype = vidxd->vdev.region[i].subtype;

			rc = vfio_info_add_capability(&caps, &cap_type.header,
						      sizeof(cap_type));
			if (rc)
				return rc;
		} /* default */
		} /* info.index switch */

		if ((info.flags & VFIO_REGION_INFO_FLAG_CAPS) && sparse) {
			if (cap_type_id == VFIO_REGION_INFO_CAP_SPARSE_MMAP) {
				rc = vfio_info_add_capability(&caps,
							      &sparse->header,
							      sizeof(*sparse) +
							      (sparse->nr_areas *
							      sizeof(*sparse->areas)));
				kfree(sparse);
				if (rc)
					return rc;
			}
		}

		if (caps.size) {
			if (info.argsz < sizeof(info) + caps.size) {
				info.argsz = sizeof(info) + caps.size;
				info.cap_offset = 0;
			} else {
				vfio_info_cap_shift(&caps, sizeof(info));
				if (copy_to_user((void __user *)arg +
						 sizeof(info), caps.buf,
						 caps.size)) {
					kfree(caps.buf);
					return -EFAULT;
				}
				info.cap_offset = sizeof(info);
			}

			kfree(caps.buf);
		}

		return copy_to_user((void __user *)arg, &info, minsz) ?
				    -EFAULT : 0;
	} else if (cmd == VFIO_DEVICE_GET_IRQ_INFO) {
		struct vfio_irq_info info;

		minsz = offsetofend(struct vfio_irq_info, count);

		if (copy_from_user(&info, (void __user *)arg, minsz))
			return -EFAULT;

		if (info.argsz < minsz || info.index >= VFIO_PCI_NUM_IRQS)
			return -EINVAL;

		switch (info.index) {
		case VFIO_PCI_MSI_IRQ_INDEX:
		case VFIO_PCI_MSIX_IRQ_INDEX:
		default:
			return -EINVAL;
		} /* switch(info.index) */

		info.flags = VFIO_IRQ_INFO_EVENTFD | VFIO_IRQ_INFO_NORESIZE;
		info.count = idxd_vdcm_get_irq_count(vidxd, info.index);

		return copy_to_user((void __user *)arg, &info, minsz) ?
			-EFAULT : 0;
	} else if (cmd == VFIO_DEVICE_SET_IRQS) {
		struct vfio_irq_set hdr;
		u8 *data = NULL;
		size_t data_size = 0;

		minsz = offsetofend(struct vfio_irq_set, count);

		if (copy_from_user(&hdr, (void __user *)arg, minsz))
			return -EFAULT;

		if (!(hdr.flags & VFIO_IRQ_SET_DATA_NONE)) {
			int max = idxd_vdcm_get_irq_count(vidxd, hdr.index);

			rc = vfio_set_irqs_validate_and_prepare(&hdr, max,
								VFIO_PCI_NUM_IRQS,
								&data_size);
			if (rc) {
				dev_err(dev, "intel:vfio_set_irqs_validate_and_prepare failed\n");
				return -EINVAL;
			}
			if (data_size) {
				data = memdup_user((void __user *)(arg + minsz),
						   data_size);
				if (IS_ERR(data))
					return PTR_ERR(data);
			}
		}

		if (!data)
			return -EINVAL;

		rc = idxd_vdcm_set_irqs(vidxd, hdr.flags, hdr.index,
					hdr.start, hdr.count, data);
		kfree(data);
		return rc;
	} else if (cmd == VFIO_DEVICE_RESET) {
		vidxd_vdcm_reset(vidxd);
		return 0;
	}

	return rc;
}

static ssize_t name_show(struct kobject *kobj, struct device *dev, char *buf)
{
	struct vdcm_idxd_type *type;

	type = idxd_vdcm_find_vidxd_type(dev, kobject_name(kobj));

	if (type)
		return sprintf(buf, "%s\n", type->description);

	return -EINVAL;
}
static MDEV_TYPE_ATTR_RO(name);

static int find_available_mdev_instances(struct idxd_device *idxd)
{
	int count = 0, i;

	for (i = 0; i < idxd->max_wqs; i++) {
		struct idxd_wq *wq;

		wq = &idxd->wqs[i];
		if (!is_idxd_wq_mdev(wq))
			continue;

		if ((idxd_wq_refcount(wq) <= 1 && wq_dedicated(wq)) ||
		    !wq_dedicated(wq))
			count++;
	}

	return count;
}

static ssize_t available_instances_show(struct kobject *kobj,
					struct device *dev, char *buf)
{
	int count;
	struct idxd_device *idxd = dev_get_drvdata(dev);
	struct vdcm_idxd_type *type;

	type = idxd_vdcm_find_vidxd_type(dev, kobject_name(kobj));
	if (!type)
		return -EINVAL;

	count = find_available_mdev_instances(idxd);

	return sprintf(buf, "%d\n", count);
}
static MDEV_TYPE_ATTR_RO(available_instances);

static ssize_t device_api_show(struct kobject *kobj, struct device *dev,
			       char *buf)
{
	return sprintf(buf, "%s\n", VFIO_DEVICE_API_PCI_STRING);
}
static MDEV_TYPE_ATTR_RO(device_api);

static struct attribute *idxd_mdev_types_attrs[] = {
	&mdev_type_attr_name.attr,
	&mdev_type_attr_device_api.attr,
	&mdev_type_attr_available_instances.attr,
	NULL,
};

static struct attribute_group idxd_mdev_type_group0 = {
	.name  = "wq",
	.attrs = idxd_mdev_types_attrs,
};

static struct attribute_group *idxd_mdev_type_groups[] = {
	&idxd_mdev_type_group0,
	NULL,
};

static const struct mdev_parent_ops idxd_vdcm_ops = {
	.supported_type_groups	= idxd_mdev_type_groups,
	.create			= idxd_vdcm_create,
	.remove			= idxd_vdcm_remove,
	.open			= idxd_vdcm_open,
	.release		= idxd_vdcm_release,
	.read			= idxd_vdcm_read,
	.write			= idxd_vdcm_write,
	.mmap			= idxd_vdcm_mmap,
	.ioctl			= idxd_vdcm_ioctl,
};

int idxd_mdev_host_init(struct idxd_device *idxd)
{
	struct device *dev = &idxd->pdev->dev;
	int rc;

	if (iommu_dev_has_feature(dev, IOMMU_DEV_FEAT_AUX)) {
		rc = iommu_dev_enable_feature(dev, IOMMU_DEV_FEAT_AUX);
		if (rc < 0)
			dev_warn(dev, "Failed to enable aux-domain: %d\n",
				 rc);
	} else {
		dev_dbg(dev, "No aux-domain feature.\n");
	}

	return mdev_register_device(dev, &idxd_vdcm_ops);
}

void idxd_mdev_host_release(struct idxd_device *idxd)
{
	struct device *dev = &idxd->pdev->dev;
	int rc;

	if (iommu_dev_has_feature(dev, IOMMU_DEV_FEAT_AUX)) {
		rc = iommu_dev_disable_feature(dev, IOMMU_DEV_FEAT_AUX);
		if (rc < 0)
			dev_warn(dev, "Failed to disable aux-domain: %d\n",
				 rc);
	}

	mdev_unregister_device(dev);
}
