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
#include <uapi/linux/idxd.h>
#include "registers.h"
#include "idxd.h"
#include "../../vfio/pci/vfio_pci_private.h"
#include "mdev.h"
#include "vdev.h"

static int idxd_get_mdev_pasid(struct mdev_device *mdev)
{
	struct iommu_domain *domain;
	struct device *dev = mdev_dev(mdev);

	domain = mdev_get_iommu_domain(dev);
	if (!domain)
		return -EINVAL;

	return iommu_aux_get_pasid(domain, dev->parent);
}

int vidxd_send_interrupt(struct vdcm_idxd *vidxd, int msix_idx)
{
	int rc = -1;
	struct device *dev = &vidxd->idxd->pdev->dev;

	dev_dbg(dev, "%s interrput %d\n", __func__, msix_idx);

	if (!vidxd->vdev.msix_trigger[msix_idx]) {
		dev_warn(dev, "%s: intr evtfd not found %d\n",
			 __func__, msix_idx);
		return -EINVAL;
	}

	rc = eventfd_signal(vidxd->vdev.msix_trigger[msix_idx], 1);
	if (rc != 1)
		dev_err(dev, "eventfd signal failed (%d)\n", rc);
	else
		dev_dbg(dev, "vidxd interrupt triggered wq(%d) %d\n",
			vidxd->wq->id, msix_idx);

	return rc;
}

static void vidxd_mmio_init_grpcfg(struct vdcm_idxd *vidxd,
				   struct grpcfg *grpcfg)
{
	struct idxd_wq *wq = vidxd->wq;
	struct idxd_group *group = wq->group;
	int i;

	/*
	 * At this point, we are only exporting a single workqueue for
	 * each mdev. So we need to just fake it as first workqueue
	 * and also mark the available engines in this group.
	 */

	/* Set single workqueue and the first one */
	grpcfg->wqs[0] = 0x1;
	grpcfg->engines = 0;
	for (i = 0; i < group->num_engines; i++)
		grpcfg->engines |= BIT(i);
	grpcfg->flags.bits = group->grpcfg.flags.bits;
}

void vidxd_mmio_init(struct vdcm_idxd *vidxd)
{
	struct vdcm_idxd_pci_bar0 *bar0 = &vidxd->bar0;
	struct idxd_device *idxd = vidxd->idxd;
	struct idxd_wq *wq = vidxd->wq;
	union wqcfg *wqcfg;
	struct grpcfg *grpcfg;
	union wq_cap_reg *wq_cap;
	union offsets_reg *offsets;

	/* setup wqcfg */
	wqcfg = (union wqcfg *)&bar0->wq_ctrl_regs[0];
	grpcfg = (struct grpcfg *)&bar0->grp_ctrl_regs[0];

	wqcfg->wq_size = wq->size;
	wqcfg->wq_thresh = wq->threshold;

	if (wq_dedicated(wq))
		wqcfg->mode = 1;

	if (idxd->hw.gen_cap.block_on_fault &&
	    test_bit(WQ_FLAG_BLOCK_ON_FAULT, &wq->flags))
		wqcfg->bof = 1;

	wqcfg->priority = wq->priority;
	wqcfg->max_xfer_shift = idxd->hw.gen_cap.max_xfer_shift;
	wqcfg->max_batch_shift = idxd->hw.gen_cap.max_batch_shift;
	/* make mode change read-only */
	wqcfg->mode_support = 0;

	/* setup grpcfg */
	vidxd_mmio_init_grpcfg(vidxd, grpcfg);

	/* setup wqcap */
	wq_cap = (union wq_cap_reg *)&bar0->cap_ctrl_regs[IDXD_WQCAP_OFFSET];
	memset(wq_cap, 0, sizeof(union wq_cap_reg));
	wq_cap->total_wq_size = wq->size;
	wq_cap->num_wqs = 1;
	if (wq_dedicated(wq))
		wq_cap->dedicated_mode = 1;
	else
		wq_cap->shared_mode = 1;

	offsets = (union offsets_reg *)&bar0->cap_ctrl_regs[IDXD_TABLE_OFFSET];
	offsets->grpcfg = VIDXD_GRPCFG_OFFSET / 0x100;
	offsets->wqcfg = VIDXD_WQCFG_OFFSET / 0x100;
	offsets->msix_perm = VIDXD_MSIX_PERM_OFFSET / 0x100;

	/* Clear MSI-X permissions table */
	memset(bar0->msix_perm_table, 0, 2 * 8);
}

static void idxd_complete_command(struct vdcm_idxd *vidxd,
				  enum idxd_cmdsts_err val)
{
	struct vdcm_idxd_pci_bar0 *bar0 = &vidxd->bar0;
	u32 *cmd = (u32 *)&bar0->cap_ctrl_regs[IDXD_CMD_OFFSET];
	u32 *cmdsts = (u32 *)&bar0->cap_ctrl_regs[IDXD_CMDSTS_OFFSET];
	struct mdev_device *mdev = vidxd->vdev.mdev;
	struct device *dev = mdev_dev(mdev);

	*cmdsts = val;
	dev_dbg(dev, "%s: cmd: %#x  status: %#x\n", __func__, *cmd, val);

	if (*cmd & IDXD_CMD_INT_MASK) {
		bar0->cap_ctrl_regs[IDXD_INTCAUSE_OFFSET] |= IDXD_INTC_CMD;
		vidxd_send_interrupt(vidxd, 0);
	}
}

static void vidxd_enable(struct vdcm_idxd *vidxd)
{
	struct vdcm_idxd_pci_bar0 *bar0 = &vidxd->bar0;
	bool ats = (*(u16 *)&vidxd->cfg[VIDXD_ATS_OFFSET + 6]) & (1U << 15);
	bool prs = (*(u16 *)&vidxd->cfg[VIDXD_PRS_OFFSET + 4]) & 1U;
	bool pasid = (*(u16 *)&vidxd->cfg[VIDXD_PASID_OFFSET + 6]) & 1U;
	u32 vdev_state = *(u32 *)&bar0->cap_ctrl_regs[IDXD_GENSTATS_OFFSET] &
			 IDXD_GENSTATS_MASK;
	struct mdev_device *mdev = vidxd->vdev.mdev;
	struct device *dev = mdev_dev(mdev);

	dev_dbg(dev, "%s\n", __func__);

	if (vdev_state == IDXD_DEVICE_STATE_ENABLED)
		return idxd_complete_command(vidxd,
					     IDXD_CMDSTS_ERR_DEV_ENABLED);

	/* Check PCI configuration */
	if (!(vidxd->cfg[PCI_COMMAND] & PCI_COMMAND_MASTER))
		return idxd_complete_command(vidxd,
					     IDXD_CMDSTS_ERR_BUSMASTER_EN);

	if (pasid != prs || (pasid && !ats))
		return idxd_complete_command(vidxd,
					     IDXD_CMDSTS_ERR_BUSMASTER_EN);

	bar0->cap_ctrl_regs[IDXD_GENSTATS_OFFSET] = IDXD_DEVICE_STATE_ENABLED;

	return idxd_complete_command(vidxd, IDXD_CMDSTS_SUCCESS);
}

static void vidxd_disable(struct vdcm_idxd *vidxd)
{
	struct idxd_wq *wq;
	struct idxd_device *idxd;
	union wqcfg *wqcfg;

	struct vdcm_idxd_pci_bar0 *bar0 = &vidxd->bar0;
	struct mdev_device *mdev = vidxd->vdev.mdev;
	struct device *dev = mdev_dev(mdev);
	u32 vdev_state = *(u32 *)&bar0->cap_ctrl_regs[IDXD_GENSTATS_OFFSET] &
			 IDXD_GENSTATS_MASK;

	dev_dbg(dev, "%s\n", __func__);

	if (vdev_state == IDXD_DEVICE_STATE_DISABLED) {
		idxd_complete_command(vidxd, IDXD_CMDSTS_ERR_DIS_DEV_EN);
		return;
	}

	wqcfg = (union wqcfg *)&bar0->wq_ctrl_regs[0];
	wq = vidxd->wq;
	idxd = wq->idxd;

	/* If it is a DWQ, need to disable the DWQ as well */
	idxd_wq_drain(wq);
	if (wq_dedicated(wq))
		idxd_wq_disable(wq, NULL);

	wqcfg->wq_state = 0;
	bar0->cap_ctrl_regs[IDXD_GENSTATS_OFFSET] = IDXD_DEVICE_STATE_DISABLED;
	idxd_complete_command(vidxd, IDXD_CMDSTS_SUCCESS);
}

static void vidxd_drain(struct vdcm_idxd *vidxd)
{
	struct idxd_wq *wq;
	struct idxd_device *idxd;
	union wqcfg *wqcfg;
	struct vdcm_idxd_pci_bar0 *bar0 = &vidxd->bar0;
	u32 vdev_state = *(u32 *)&bar0->cap_ctrl_regs[IDXD_GENSTATS_OFFSET] &
			 IDXD_GENSTATS_MASK;
	struct mdev_device *mdev = vidxd->vdev.mdev;
	struct device *dev = mdev_dev(mdev);

	dev_dbg(dev, "%s\n", __func__);

	if (vdev_state == IDXD_DEVICE_STATE_DISABLED) {
		idxd_complete_command(vidxd, IDXD_CMDSTS_ERR_DEV_NOT_EN);
		return;
	}

	wqcfg = (union wqcfg *)&bar0->wq_ctrl_regs[0];
	wq = vidxd->wq;
	idxd = wq->idxd;

	idxd_wq_drain(wq);
	idxd_complete_command(vidxd, IDXD_CMDSTS_SUCCESS);
}

static void vidxd_abort(struct vdcm_idxd *vidxd)
{
	struct idxd_wq *wq;
	struct idxd_device *idxd;
	union wqcfg *wqcfg;
	struct vdcm_idxd_pci_bar0 *bar0 = &vidxd->bar0;
	u32 vdev_state = *(u32 *)&bar0->cap_ctrl_regs[IDXD_GENSTATS_OFFSET] &
			 IDXD_GENSTATS_MASK;
	struct mdev_device *mdev = vidxd->vdev.mdev;
	struct device *dev = mdev_dev(mdev);

	dev_dbg(dev, "%s\n", __func__);

	if (vdev_state == IDXD_DEVICE_STATE_DISABLED) {
		idxd_complete_command(vidxd, IDXD_CMDSTS_ERR_DEV_NOT_EN);
		return;
	}

	wqcfg = (union wqcfg *)&bar0->wq_ctrl_regs[0];
	wq = vidxd->wq;
	idxd = wq->idxd;

	idxd_wq_abort(wq);

	idxd_complete_command(vidxd, IDXD_CMDSTS_SUCCESS);
}

static void vidxd_wq_drain(struct vdcm_idxd *vidxd, int val)
{
	vidxd_drain(vidxd);
}

static void vidxd_wq_abort(struct vdcm_idxd *vidxd, int val)
{
	vidxd_abort(vidxd);
}

void vidxd_reset(struct vdcm_idxd *vidxd)
{
	struct vdcm_idxd_pci_bar0 *bar0 = &vidxd->bar0;
	struct idxd_wq *wq;
	struct idxd_device *idxd;

	*(u32 *)&bar0->cap_ctrl_regs[IDXD_GENSTATS_OFFSET] =
		IDXD_DEVICE_STATE_DRAIN;

	wq = vidxd->wq;
	idxd = wq->idxd;

	idxd_wq_drain(wq);

	/* If it is a DWQ, need to disable the DWQ as well */
	if (wq_dedicated(wq))
		idxd_wq_disable(wq, NULL);

	vidxd_mmio_init(vidxd);
	idxd_complete_command(vidxd, IDXD_CMDSTS_SUCCESS);
}

static void vidxd_alloc_int_handle(struct vdcm_idxd *vidxd, int vidx)
{
	bool ims = (vidx >> 16) & 1;
	u32 cmdsts;
	struct mdev_device *mdev = vidxd->vdev.mdev;
	struct device *dev = mdev_dev(mdev);

	vidx = vidx & 0xffff;

	dev_dbg(dev, "allocating int handle for %x\n", vidx);

	if (vidx != 1) {
		idxd_complete_command(vidxd, IDXD_CMDSTS_ERR_INVAL_INT_IDX);
		return;
	}

	if (ims) {
		dev_warn(dev, "IMS allocation is not implemented yet\n");
		idxd_complete_command(vidxd, IDXD_CMDSTS_ERR_NO_HANDLE);
	} else {
		vidx--; /* MSIX idx 0 is a slow path interrupt */
		cmdsts = vidxd->ims_index[vidx] << 8;
		dev_dbg(dev, "int handle %d:%lld\n", vidx,
			vidxd->ims_index[vidx]);
		idxd_complete_command(vidxd, cmdsts);
	}
}

static void vidxd_wq_enable(struct vdcm_idxd *vidxd, int wq_id)
{
	struct idxd_wq *wq;
	struct vdcm_idxd_pci_bar0 *bar0 = &vidxd->bar0;
	union wq_cap_reg *wqcap;
	struct mdev_device *mdev = vidxd->vdev.mdev;
	struct device *dev = mdev_dev(mdev);
	struct idxd_device *idxd;
	union wqcfg *vwqcfg, *wqcfg;
	unsigned long flags;

	dev_dbg(dev, "%s\n", __func__);

	if (wq_id >= 1) {
		idxd_complete_command(vidxd, IDXD_CMDSTS_INVAL_WQIDX);
		return;
	}

	idxd = vidxd->idxd;
	wq = vidxd->wq;

	dev_dbg(dev, "%s: wq %u:%u\n", __func__, wq_id, wq->id);

	vwqcfg = (union wqcfg *)&bar0->wq_ctrl_regs[wq_id];
	wqcap = (union wq_cap_reg *)&bar0->cap_ctrl_regs[IDXD_WQCAP_OFFSET];
	wqcfg = &wq->wqcfg;

	if (vidxd_state(vidxd) != IDXD_DEVICE_STATE_ENABLED) {
		idxd_complete_command(vidxd, IDXD_CMDSTS_ERR_DEV_NOTEN);
		return;
	}

	if (vwqcfg->wq_state != IDXD_WQ_DEV_DISABLED) {
		idxd_complete_command(vidxd, IDXD_CMDSTS_ERR_WQ_ENABLED);
		return;
	}

	if (vwqcfg->wq_size == 0) {
		idxd_complete_command(vidxd, IDXD_CMDSTS_ERR_WQ_SIZE);
		return;
	}

	if ((!wq_dedicated(wq) && wqcap->shared_mode == 0) ||
	    (wq_dedicated(wq) && wqcap->dedicated_mode == 0)) {
		idxd_complete_command(vidxd, IDXD_CMDSTS_ERR_WQ_MODE);
		return;
	}

	if (wq_dedicated(wq)) {
		int wq_pasid;
		u32 status;
		int priv;

		wq_pasid = idxd_get_mdev_pasid(mdev);
		priv = 1;

		if (wq_pasid >= 0) {
			wqcfg->bits[2] &= ~0x3fffff00;
			wqcfg->priv = priv;
			wqcfg->pasid_en = 1;
			wqcfg->pasid = wq_pasid;
			dev_dbg(dev, "program pasid %d in wq %d\n",
				wq_pasid, wq->id);
			spin_lock_irqsave(&idxd->dev_lock, flags);
			idxd_wq_update_pasid(wq, wq_pasid);
			idxd_wq_update_priv(wq, priv);
			spin_unlock_irqrestore(&idxd->dev_lock, flags);
			idxd_wq_enable(wq, &status);
			if (status) {
				dev_err(dev, "vidxd enable wq %d failed\n",
					wq->id);
				idxd_complete_command(vidxd, status);
				return;
			}
		} else {
			dev_err(dev,
				"idxd pasid setup failed wq %d wq_pasid %d\n",
				wq->id, wq_pasid);
			idxd_complete_command(vidxd, IDXD_CMDSTS_ERR_PASID_EN);
			return;
		}
	}

	vwqcfg->wq_state = IDXD_WQ_DEV_ENABLED;
	idxd_complete_command(vidxd, IDXD_CMDSTS_SUCCESS);
}

static void vidxd_wq_disable(struct vdcm_idxd *vidxd, int wq_id_mask)
{
	struct idxd_wq *wq;
	struct idxd_device *idxd;
	union wqcfg *wqcfg;
	struct vdcm_idxd_pci_bar0 *bar0 = &vidxd->bar0;
	struct mdev_device *mdev = vidxd->vdev.mdev;
	struct device *dev = mdev_dev(mdev);

	wq = vidxd->wq;
	idxd = wq->idxd;

	if (!(wq_id_mask & BIT(0))) {
		idxd_complete_command(vidxd, IDXD_CMDSTS_INVAL_WQIDX);
		return;
	}

	dev_dbg(dev, "vidxd disable wq %u:%u\n", 0, wq->id);

	wqcfg = (union wqcfg *)&bar0->wq_ctrl_regs[0];
	if (wqcfg->wq_state != IDXD_WQ_DEV_ENABLED) {
		idxd_complete_command(vidxd, IDXD_CMDSTS_ERR_DEV_NOT_EN);
		return;
	}

	if (wq_dedicated(wq)) {
		u32 status;

		idxd_wq_disable(wq, &status);
		if (status) {
			dev_err(dev, "vidxd disable wq %d failed\n", wq->id);
			idxd_complete_command(vidxd, status);
			return;
		}
	}

	wqcfg->wq_state = IDXD_WQ_DEV_DISABLED;
	idxd_complete_command(vidxd, IDXD_CMDSTS_SUCCESS);
}

void vidxd_do_command(struct vdcm_idxd *vidxd, u32 val)
{
	union idxd_command_reg *reg =
		(union idxd_command_reg *)&vidxd->bar0.cap_ctrl_regs[IDXD_CMD_OFFSET];
	struct mdev_device *mdev = vidxd->vdev.mdev;
	struct device *dev = mdev_dev(mdev);

	reg->bits = val;

	dev_dbg(dev, "%s: cmd code: %u reg: %x\n", __func__, reg->cmd,
		reg->bits);

	switch (reg->cmd) {
	case IDXD_CMD_ENABLE_DEVICE:
		vidxd_enable(vidxd);
		break;
	case IDXD_CMD_DISABLE_DEVICE:
		vidxd_disable(vidxd);
		break;
	case IDXD_CMD_DRAIN_ALL:
		vidxd_drain(vidxd);
		break;
	case IDXD_CMD_ABORT_ALL:
		vidxd_abort(vidxd);
		break;
	case IDXD_CMD_RESET_DEVICE:
		vidxd_reset(vidxd);
		break;
	case IDXD_CMD_ENABLE_WQ:
		vidxd_wq_enable(vidxd, reg->operand);
		break;
	case IDXD_CMD_DISABLE_WQ:
		vidxd_wq_disable(vidxd, reg->operand);
		break;
	case IDXD_CMD_DRAIN_WQ:
		vidxd_wq_drain(vidxd, reg->operand);
		break;
	case IDXD_CMD_ABORT_WQ:
		vidxd_wq_abort(vidxd, reg->operand);
		break;
	case IDXD_CMD_REQUEST_INT_HANDLE:
		vidxd_alloc_int_handle(vidxd, reg->operand);
		break;
	default:
		idxd_complete_command(vidxd, IDXD_CMDSTS_INVAL_CMD);
		break;
	}
}

int vidxd_setup_ims_entry(struct vdcm_idxd *vidxd, int ims_idx, u32 val)
{
	struct mdev_device *mdev = vidxd->vdev.mdev;
	struct device *dev = mdev_dev(mdev);
	int pasid;
	unsigned int ims_offset;

	/*
	 * Current implementation limits to 1 WQ for the vdev and therefore
	 * also only 1 IMS interrupt for that vdev.
	 */
	if (ims_idx >= VIDXD_MAX_WQS) {
		dev_warn(dev, "ims_idx greater than vidxd allowed: %d\n",
			 ims_idx);
		return -EINVAL;
	}

	/* Setup the PASID filtering */
	pasid = idxd_get_mdev_pasid(mdev);

	if (pasid >= 0) {
		val = (1 << 3) | (pasid << 12) | (val & 7);
		ims_offset = vidxd->idxd->ims_offset +
			     vidxd->ims_index[ims_idx] * 0x10;
		iowrite32(val, vidxd->idxd->reg_base + ims_offset + 12);
	} else {
		dev_warn(dev, "pasid setup failed for ims entry %lld\n",
			 vidxd->ims_index[ims_idx]);
	}

	return 0;
}

int vidxd_free_ims_entry(struct vdcm_idxd *vidxd, int msix_idx)
{
	return 0;
}

static void vidxd_send_errors(struct vdcm_idxd *vidxd)
{
	struct idxd_device *idxd = vidxd->idxd;
	struct vdcm_idxd_pci_bar0 *bar0 = &vidxd->bar0;
	u64 *swerr = (u64 *)&bar0->cap_ctrl_regs[IDXD_SWERR_OFFSET];
	int i;

	for (i = 0; i < 4; i++) {
		*swerr = idxd->sw_err.bits[i];
		swerr++;
	}
	vidxd_send_interrupt(vidxd, 0);
}

void idxd_wq_vidxd_send_errors(struct idxd_wq *wq)
{
	struct vdcm_idxd *vidxd;

	list_for_each_entry(vidxd, &wq->vdcm_list, list)
		vidxd_send_errors(vidxd);
}

void idxd_vidxd_send_errors(struct idxd_device *idxd)
{
	int i;

	for (i = 0; i < idxd->max_wqs; i++) {
		struct idxd_wq *wq = &idxd->wqs[i];

		idxd_wq_vidxd_send_errors(wq);
	}
}
