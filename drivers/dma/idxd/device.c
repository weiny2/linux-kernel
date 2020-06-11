// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2019 Intel Corporation. All rights rsvd. */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/io-64-nonatomic-lo-hi.h>
#include <linux/dmaengine.h>
#include <uapi/linux/idxd.h>
#include "../dmaengine.h"
#include "idxd.h"
#include "registers.h"

static void idxd_cmd_exec(struct idxd_device *idxd, int cmd_code, u32 operand,
			  u32 *status);

/* Interrupt control bits */
int idxd_mask_msix_vector(struct idxd_device *idxd, int vec_id)
{
	struct pci_dev *pdev = idxd->pdev;
	int msixcnt = pci_msix_vec_count(pdev);
	union msix_perm perm;
	u32 offset;

	if (vec_id < 0 || vec_id >= msixcnt)
		return -EINVAL;

	offset = idxd->msix_perm_offset + vec_id * 8;
	perm.bits = ioread32(idxd->reg_base + offset);
	perm.ignore = 1;
	iowrite32(perm.bits, idxd->reg_base + offset);

	return 0;
}

void idxd_mask_msix_vectors(struct idxd_device *idxd)
{
	struct pci_dev *pdev = idxd->pdev;
	int msixcnt = pci_msix_vec_count(pdev);
	int i, rc;

	for (i = 0; i < msixcnt; i++) {
		rc = idxd_mask_msix_vector(idxd, i);
		if (rc < 0)
			dev_warn(&pdev->dev,
				 "Failed disabling msix vec %d\n", i);
	}
}

int idxd_unmask_msix_vector(struct idxd_device *idxd, int vec_id)
{
	struct pci_dev *pdev = idxd->pdev;
	int msixcnt = pci_msix_vec_count(pdev);
	union msix_perm perm;
	u32 offset;

	if (vec_id < 0 || vec_id >= msixcnt)
		return -EINVAL;

	offset = idxd->msix_perm_offset + vec_id * 8;
	perm.bits = ioread32(idxd->reg_base + offset);
	perm.ignore = 0;
	iowrite32(perm.bits, idxd->reg_base + offset);

	/*
	 * A readback from the device ensures that any previously generated
	 * completion record writes are visible to software based on PCI
	 * ordering rules.
	 */
	perm.bits = ioread32(idxd->reg_base + offset);

	return 0;
}

void idxd_unmask_error_interrupts(struct idxd_device *idxd)
{
	union genctrl_reg genctrl;

	genctrl.bits = ioread32(idxd->reg_base + IDXD_GENCTRL_OFFSET);
	genctrl.softerr_int_en = 1;
	iowrite32(genctrl.bits, idxd->reg_base + IDXD_GENCTRL_OFFSET);
}

void idxd_mask_error_interrupts(struct idxd_device *idxd)
{
	union genctrl_reg genctrl;

	genctrl.bits = ioread32(idxd->reg_base + IDXD_GENCTRL_OFFSET);
	genctrl.softerr_int_en = 0;
	iowrite32(genctrl.bits, idxd->reg_base + IDXD_GENCTRL_OFFSET);
}

static void free_hw_descs(struct idxd_wq *wq)
{
	int i;

	for (i = 0; i < wq->num_descs; i++)
		kfree(wq->hw_descs[i]);

	kfree(wq->hw_descs);
}

static int alloc_hw_descs(struct idxd_wq *wq, int num)
{
	struct device *dev = &wq->idxd->pdev->dev;
	int i;
	int node = dev_to_node(dev);

	wq->hw_descs = kcalloc_node(num, sizeof(struct dsa_hw_desc *),
				    GFP_KERNEL, node);
	if (!wq->hw_descs)
		return -ENOMEM;

	for (i = 0; i < num; i++) {
		wq->hw_descs[i] = kzalloc_node(sizeof(*wq->hw_descs[i]),
					       GFP_KERNEL, node);
		if (!wq->hw_descs[i]) {
			free_hw_descs(wq);
			return -ENOMEM;
		}
	}

	return 0;
}

static void free_descs(struct idxd_wq *wq)
{
	int i;

	for (i = 0; i < wq->num_descs; i++)
		kfree(wq->descs[i]);

	kfree(wq->descs);
}

static int alloc_descs(struct idxd_wq *wq, int num)
{
	struct device *dev = &wq->idxd->pdev->dev;
	int i;
	int node = dev_to_node(dev);

	wq->descs = kcalloc_node(num, sizeof(struct idxd_desc *),
				 GFP_KERNEL, node);
	if (!wq->descs)
		return -ENOMEM;

	for (i = 0; i < num; i++) {
		wq->descs[i] = kzalloc_node(sizeof(*wq->descs[i]),
					    GFP_KERNEL, node);
		if (!wq->descs[i]) {
			free_descs(wq);
			return -ENOMEM;
		}
	}

	return 0;
}

/* WQ control bits */
int idxd_wq_alloc_resources(struct idxd_wq *wq)
{
	struct idxd_device *idxd = wq->idxd;
	struct device *dev = &idxd->pdev->dev;
	int rc, num_descs, i;
	int align;
	u64 tmp;

	if (wq->type != IDXD_WQT_KERNEL)
		return 0;

	wq->num_descs = wq->size;
	num_descs = wq->size;

	rc = alloc_hw_descs(wq, num_descs);
	if (rc < 0)
		return rc;

	if (idxd->type == IDXD_TYPE_DSA) {
		align = 32;
	} else if (idxd->type == IDXD_TYPE_IAX) {
		align = 64;
	} else {
		return -ENODEV;
	}

	wq->compls_size = num_descs * idxd->compl_size + align;
	wq->compls_raw = dma_alloc_coherent(dev, wq->compls_size,
					    &wq->compls_addr_raw, GFP_KERNEL);
	if (!wq->compls_raw) {
		rc = -ENOMEM;
		goto fail_alloc_compls;
	}


	/* Adjust alignment */
	wq->compls_addr = (wq->compls_addr_raw + (align - 1)) & ~(align - 1);
	tmp = (u64)(u64 *)wq->compls_raw;
	tmp = (tmp + (align - 1)) & ~(align - 1);
	wq->compls = (struct dsa_completion_record *)tmp;

	rc = alloc_descs(wq, num_descs);
	if (rc < 0)
		goto fail_alloc_descs;

	rc = sbitmap_queue_init_node(&wq->sbq, num_descs, -1, false, GFP_KERNEL,
				     dev_to_node(dev));
	if (rc < 0)
		goto fail_sbitmap_init;

	for (i = 0; i < num_descs; i++) {
		struct idxd_desc *desc = wq->descs[i];

		desc->hw = wq->hw_descs[i];
		if (idxd->type == IDXD_TYPE_DSA)
			desc->completion = &wq->compls[i];
		else if (idxd->type == IDXD_TYPE_IAX)
			desc->iax_completion = &wq->iax_compls[i];
		desc->compl_dma = wq->compls_addr + idxd->compl_size * i;
		desc->id = i;
		desc->wq = wq;
		desc->cpu = -1;
		dma_async_tx_descriptor_init(&desc->txd, &wq->dma_chan);
		desc->txd.tx_submit = idxd_dma_tx_submit;
	}

	return 0;

 fail_sbitmap_init:
	free_descs(wq);
 fail_alloc_descs:
	dma_free_coherent(dev, wq->compls_size, wq->compls_raw,
			  wq->compls_addr_raw);
 fail_alloc_compls:
	free_hw_descs(wq);
	return rc;
}

void idxd_wq_free_resources(struct idxd_wq *wq)
{
	struct device *dev = &wq->idxd->pdev->dev;

	if (wq->type != IDXD_WQT_KERNEL)
		return;

	free_hw_descs(wq);
	free_descs(wq);
	dma_free_coherent(dev, wq->compls_size, wq->compls_raw,
			  wq->compls_addr_raw);
	sbitmap_queue_free(&wq->sbq);
}

int idxd_wq_enable(struct idxd_wq *wq, u32 *status)
{
	struct idxd_device *idxd = wq->idxd;
	struct device *dev = &idxd->pdev->dev;
	u32 stat;

	if (wq->state == IDXD_WQ_ENABLED) {
		dev_dbg(dev, "WQ %d already enabled\n", wq->id);
		return -ENXIO;
	}

	idxd_cmd_exec(idxd, IDXD_CMD_ENABLE_WQ, wq->id, &stat);

	if (status)
		*status = stat;

	if (stat != IDXD_CMDSTS_SUCCESS &&
	    stat != IDXD_CMDSTS_ERR_WQ_ENABLED) {
		dev_dbg(dev, "WQ enable failed: %#x\n", stat);
		return -ENXIO;
	}

	wq->state = IDXD_WQ_ENABLED;
	dev_dbg(dev, "WQ %d enabled\n", wq->id);
	return 0;
}

int idxd_wq_disable(struct idxd_wq *wq, u32 *status)
{
	struct idxd_device *idxd = wq->idxd;
	struct device *dev = &idxd->pdev->dev;
	u32 stat, operand;

	dev_dbg(dev, "Disabling WQ %d\n", wq->id);

	if (wq->state != IDXD_WQ_ENABLED) {
		dev_dbg(dev, "WQ %d in wrong state: %d\n", wq->id, wq->state);
		return 0;
	}

	operand = BIT(wq->id % 16) | ((wq->id / 16) << 16);
	idxd_cmd_exec(idxd, IDXD_CMD_DISABLE_WQ, operand, &stat);

	if (status)
		*status = stat;

	if (stat != IDXD_CMDSTS_SUCCESS) {
		dev_dbg(dev, "WQ disable failed: %#x\n", stat);
		return -ENXIO;
	}

	wq->state = IDXD_WQ_DISABLED;
	dev_dbg(dev, "WQ %d disabled\n", wq->id);
	return 0;
}

void idxd_wq_drain(struct idxd_wq *wq)
{
	struct idxd_device *idxd = wq->idxd;
	struct device *dev = &idxd->pdev->dev;
	u32 operand;

	if (wq->state != IDXD_WQ_ENABLED) {
		dev_dbg(dev, "WQ %d in wrong state: %d\n", wq->id, wq->state);
		return;
	}

	dev_dbg(dev, "Draining WQ %d\n", wq->id);
	operand = BIT(wq->id % 16) | ((wq->id / 16) << 16);
	idxd_cmd_exec(idxd, IDXD_CMD_DRAIN_WQ, operand, NULL);
	dev_dbg(dev, "WQ %d drained\n", wq->id);
}

int idxd_wq_map_portal(struct idxd_wq *wq)
{
	struct idxd_device *idxd = wq->idxd;
	struct pci_dev *pdev = idxd->pdev;
	struct device *dev = &pdev->dev;
	resource_size_t start;

	start = pci_resource_start(pdev, IDXD_WQ_BAR);
	start = start + wq->id * IDXD_PORTAL_SIZE;

	wq->portal = devm_ioremap(dev, start, IDXD_PORTAL_SIZE);
	if (!wq->portal)
		return -ENOMEM;

	return 0;
}

void idxd_wq_unmap_portal(struct idxd_wq *wq)
{
	struct device *dev = &wq->idxd->pdev->dev;

	devm_iounmap(dev, wq->portal);
}

int idxd_wq_abort(struct idxd_wq *wq)
{
	struct idxd_device *idxd = wq->idxd;
	struct device *dev = &idxd->pdev->dev;
	u32 operand, status;

	dev_dbg(dev, "Abort WQ %d\n", wq->id);
	if (wq->state != IDXD_WQ_ENABLED) {
		dev_dbg(dev, "WQ %d not active\n", wq->id);
		return -ENXIO;
	}

	operand = BIT(wq->id % 16) | ((wq->id / 16) << 16);
	dev_dbg(dev, "cmd: %u operand: %#x\n", IDXD_CMD_ABORT_WQ, operand);
	idxd_cmd_exec(idxd, IDXD_CMD_ABORT_WQ, operand, &status);
	if (status != IDXD_CMDSTS_SUCCESS) {
		dev_dbg(dev, "WQ abort failed: %#x\n", status);
		return -ENXIO;
	}

	dev_dbg(dev, "WQ %d aborted\n", wq->id);
	return 0;
}

int idxd_wq_set_pasid(struct idxd_wq *wq, int pasid)
{
	struct idxd_device *idxd = wq->idxd;
	int rc;
	union wqcfg wqcfg;
	unsigned int offset;
	unsigned long flags;

	rc = idxd_wq_disable(wq, NULL);
	if (rc < 0)
		return rc;

	offset = WQCFG_OFFSET(idxd, wq->id, 2);
	spin_lock_irqsave(&idxd->dev_lock, flags);
	wqcfg.bits[2] = ioread32(idxd->reg_base + offset);
	wqcfg.pasid_en = 1;
	wqcfg.pasid = pasid;
	iowrite32(wqcfg.bits[2], idxd->reg_base + offset);
	spin_unlock_irqrestore(&idxd->dev_lock, flags);

	rc = idxd_wq_enable(wq, NULL);
	if (rc < 0)
		return rc;

	return 0;
}

int idxd_wq_disable_pasid(struct idxd_wq *wq)
{
	struct idxd_device *idxd = wq->idxd;
	int rc;
	union wqcfg wqcfg;
	unsigned int offset;
	unsigned long flags;

	rc = idxd_wq_disable(wq, NULL);
	if (rc < 0)
		return rc;

	offset = idxd->wqcfg_offset + wq->id * sizeof(wqcfg);
	spin_lock_irqsave(&idxd->dev_lock, flags);
	wqcfg.bits[2] = ioread32(idxd->reg_base + offset);
	wqcfg.pasid_en = 0;
	wqcfg.pasid = 0;
	iowrite32(wqcfg.bits[2], idxd->reg_base + offset);
	spin_unlock_irqrestore(&idxd->dev_lock, flags);

	rc = idxd_wq_enable(wq, NULL);
	if (rc < 0)
		return rc;

	return 0;
}

void idxd_wq_disable_cleanup(struct idxd_wq *wq)
{
	struct idxd_device *idxd = wq->idxd;
	struct device *dev = &idxd->pdev->dev;
	int i, wq_offset;

	memset(&wq->wqcfg, 0, sizeof(wq->wqcfg));
	wq->type = IDXD_WQT_NONE;
	wq->size = 0;
	wq->group = NULL;
	wq->threshold = 0;
	wq->priority = 0;
	clear_bit(WQ_FLAG_DEDICATED, &wq->flags);
	memset(wq->name, 0, WQ_NAME_SIZE);

	for (i = 0; i < 8; i++) {
		wq_offset = idxd->wqcfg_offset + wq->id * 32 + i * sizeof(u32);
		iowrite32(0, idxd->reg_base + wq_offset);
		dev_dbg(dev, "WQ[%d][%d][%#x]: %#x\n",
			wq->id, i, wq_offset,
			ioread32(idxd->reg_base + wq_offset));
	}
}

void idxd_wq_update_pasid(struct idxd_wq *wq, int pasid)
{
	struct idxd_device *idxd = wq->idxd;
	int offset;

	lockdep_assert_held(&idxd->dev_lock);

	/* PASID fields are 8 bytes into the WQCFG register */
	offset = idxd->wqcfg_offset + wq->id * 32 + 8;
	wq->wqcfg.pasid = pasid;
	iowrite32(wq->wqcfg.bits[2], idxd->reg_base + offset);
}

void idxd_wq_update_priv(struct idxd_wq *wq, int priv)
{
	struct idxd_device *idxd = wq->idxd;
	int offset;

	lockdep_assert_held(&idxd->dev_lock);

	/* priv field is 8 bytes into the WQCFG register */
	offset = idxd->wqcfg_offset + wq->id * 32 + 8;
	wq->wqcfg.priv = !!priv;
	iowrite32(wq->wqcfg.bits[2], idxd->reg_base + offset);
}

/* Device control bits */
static inline bool idxd_is_enabled(struct idxd_device *idxd)
{
	union gensts_reg gensts;

	gensts.bits = ioread32(idxd->reg_base + IDXD_GENSTATS_OFFSET);

	if (gensts.state == IDXD_DEVICE_STATE_ENABLED)
		return true;
	return false;
}

/*
 * This is function is only used for reset during probe and will
 * poll for completion. Once the device is setup with interrupts,
 * all commands will be done via interrupt completion.
 */
void idxd_device_init_reset(struct idxd_device *idxd)
{
	struct device *dev = &idxd->pdev->dev;
	union idxd_command_reg cmd;
	unsigned long flags;

	memset(&cmd, 0, sizeof(cmd));
	cmd.cmd = IDXD_CMD_RESET_DEVICE;
	dev_dbg(dev, "%s: sending reset for init.\n", __func__);
	spin_lock_irqsave(&idxd->dev_lock, flags);
	iowrite32(cmd.bits, idxd->reg_base + IDXD_CMD_OFFSET);

	while (ioread32(idxd->reg_base + IDXD_CMDSTS_OFFSET) &
	       IDXD_CMDSTS_ACTIVE)
		cpu_relax();
	spin_unlock_irqrestore(&idxd->dev_lock, flags);
}

static void idxd_cmd_exec(struct idxd_device *idxd, int cmd_code, u32 operand,
			  u32 *status)
{
	union idxd_command_reg cmd;
	DECLARE_COMPLETION_ONSTACK(done);
	unsigned long flags;

	memset(&cmd, 0, sizeof(cmd));
	cmd.cmd = cmd_code;
	cmd.operand = operand;
	cmd.int_req = 1;

	spin_lock_irqsave(&idxd->dev_lock, flags);
	wait_event_lock_irq(idxd->cmd_waitq,
			    !test_bit(IDXD_FLAG_CMD_RUNNING, &idxd->flags),
			    idxd->dev_lock);

	dev_dbg(&idxd->pdev->dev, "%s: sending cmd: %#x op: %#x\n",
		__func__, cmd_code, operand);

	__set_bit(IDXD_FLAG_CMD_RUNNING, &idxd->flags);
	idxd->cmd_done = &done;
	iowrite32(cmd.bits, idxd->reg_base + IDXD_CMD_OFFSET);

	/*
	 * After command submitted, release lock and go to sleep until
	 * the command completes via interrupt.
	 */
	spin_unlock_irqrestore(&idxd->dev_lock, flags);
	wait_for_completion(&done);
	spin_lock_irqsave(&idxd->dev_lock, flags);
	if (status)
		*status = ioread32(idxd->reg_base + IDXD_CMDSTS_OFFSET);
	__clear_bit(IDXD_FLAG_CMD_RUNNING, &idxd->flags);
	/* Wake up other pending commands */
	wake_up(&idxd->cmd_waitq);
	spin_unlock_irqrestore(&idxd->dev_lock, flags);
}

int idxd_device_enable(struct idxd_device *idxd)
{
	struct device *dev = &idxd->pdev->dev;
	u32 status;

	if (idxd_is_enabled(idxd)) {
		dev_dbg(dev, "Device already enabled\n");
		return -ENXIO;
	}

	idxd_cmd_exec(idxd, IDXD_CMD_ENABLE_DEVICE, 0, &status);

	/* If the command is successful or if the device was enabled */
	if (status != IDXD_CMDSTS_SUCCESS &&
	    status != IDXD_CMDSTS_ERR_DEV_ENABLED) {
		dev_dbg(dev, "%s: err_code: %#x\n", __func__, status);
		return -ENXIO;
	}

	idxd->state = IDXD_DEV_ENABLED;
	return 0;
}

int idxd_device_disable(struct idxd_device *idxd)
{
	struct device *dev = &idxd->pdev->dev;
	u32 status;

	if (!idxd_is_enabled(idxd)) {
		dev_dbg(dev, "Device is not enabled\n");
		return 0;
	}

	idxd_cmd_exec(idxd, IDXD_CMD_DISABLE_DEVICE, 0, &status);

	/* If the command is successful or if the device was disabled */
	if (status != IDXD_CMDSTS_SUCCESS &&
	    !(status & IDXD_CMDSTS_ERR_DIS_DEV_EN)) {
		dev_dbg(dev, "%s: err_code: %#x\n", __func__, status);
		return -ENXIO;
	}

	idxd->state = IDXD_DEV_CONF_READY;
	return 0;
}

void idxd_device_reset(struct idxd_device *idxd)
{
	idxd_cmd_exec(idxd, IDXD_CMD_RESET_DEVICE, 0, NULL);
}

void idxd_device_drain_pasid(struct idxd_device *idxd, int pasid)
{
	struct device *dev = &idxd->pdev->dev;
	u32 operand;

	operand = pasid;
	dev_dbg(dev, "cmd: %u operand: %#x\n", IDXD_CMD_DRAIN_PASID, operand);
	idxd_cmd_exec(idxd, IDXD_CMD_DRAIN_PASID, operand, NULL);
	dev_dbg(dev, "pasid %d drained\n", pasid);
}

void idxd_device_abort_pasid(struct idxd_device *idxd, int pasid)
{
	struct device *dev = &idxd->pdev->dev;
	u32 operand;

	dev_dbg(dev, "Abort pasid %d\n", pasid);

	operand = pasid;
	dev_dbg(dev, "cmd: %u operand: %#x\n", IDXD_CMD_ABORT_PASID, operand);
	idxd_cmd_exec(idxd, IDXD_CMD_ABORT_PASID, operand, NULL);
	dev_dbg(dev, "pasid %d aborted\n", pasid);
}

int idxd_device_request_int_handle(struct idxd_device *idxd, int idx,
				   int *handle)
{
	struct device *dev = &idxd->pdev->dev;
	u32 operand, status;

	if (!idxd->hw.gen_cap.int_handle_req)
		return -EOPNOTSUPP;

	dev_dbg(dev, "get int handle, idx %d\n", idx);

	operand = idx & 0xffff;
	dev_dbg(dev, "cmd: %u operand: %#x\n",
		IDXD_CMD_REQUEST_INT_HANDLE, operand);
	idxd_cmd_exec(idxd, IDXD_CMD_REQUEST_INT_HANDLE, operand, &status);

	if ((status & 0xff) != IDXD_CMDSTS_SUCCESS) {
		dev_dbg(dev, "request int handle failed: %#x\n", status);
		return -ENXIO;
	}

	*handle = (status >> 8) & 0xffff;

	dev_dbg(dev, "int handle acquired: %u\n", *handle);
	return 0;
}

/* Device configuration bits */
static void idxd_group_config_write(struct idxd_group *group)
{
	struct idxd_device *idxd = group->idxd;
	struct device *dev = &idxd->pdev->dev;
	int i;
	u32 grpcfg_offset;

	dev_dbg(dev, "Writing group %d cfg registers\n", group->id);

	/* setup GRPWQCFG */
	for (i = 0; i < 4; i++) {
		grpcfg_offset = idxd->grpcfg_offset +
			group->id * 64 + i * sizeof(u64);
		iowrite64(group->grpcfg.wqs[i],
			  idxd->reg_base + grpcfg_offset);
		dev_dbg(dev, "GRPCFG wq[%d:%d: %#x]: %#llx\n",
			group->id, i, grpcfg_offset,
			ioread64(idxd->reg_base + grpcfg_offset));
	}

	/* setup GRPENGCFG */
	grpcfg_offset = idxd->grpcfg_offset + group->id * 64 + 32;
	iowrite64(group->grpcfg.engines, idxd->reg_base + grpcfg_offset);
	dev_dbg(dev, "GRPCFG engs[%d: %#x]: %#llx\n", group->id,
		grpcfg_offset, ioread64(idxd->reg_base + grpcfg_offset));

	/* setup GRPFLAGS */
	grpcfg_offset = idxd->grpcfg_offset + group->id * 64 + 40;
	iowrite32(group->grpcfg.flags.bits, idxd->reg_base + grpcfg_offset);
	dev_dbg(dev, "GRPFLAGS flags[%d: %#x]: %#x\n",
		group->id, grpcfg_offset,
		ioread32(idxd->reg_base + grpcfg_offset));
}

static int idxd_groups_config_write(struct idxd_device *idxd)

{
	union gencfg_reg reg;
	int i;
	struct device *dev = &idxd->pdev->dev;

	/* Setup bandwidth token limit */
	if (idxd->token_limit) {
		reg.bits = ioread32(idxd->reg_base + IDXD_GENCFG_OFFSET);
		reg.token_limit = idxd->token_limit;
		iowrite32(reg.bits, idxd->reg_base + IDXD_GENCFG_OFFSET);
	}

	dev_dbg(dev, "GENCFG(%#x): %#x\n", IDXD_GENCFG_OFFSET,
		ioread32(idxd->reg_base + IDXD_GENCFG_OFFSET));

	for (i = 0; i < idxd->max_groups; i++) {
		struct idxd_group *group = &idxd->groups[i];

		idxd_group_config_write(group);
	}

	return 0;
}

static int idxd_wq_config_write_ro(struct idxd_wq *wq)
{
	struct idxd_device *idxd = wq->idxd;
	int wq_offset;

	if (!wq->group)
		return 0;

	if (idxd->pasid_enabled) {
		wq->wqcfg.pasid_en = 1;
		if (wq->type == IDXD_WQT_KERNEL && wq_dedicated(wq))
			wq->wqcfg.pasid = idxd->pasid;
	} else {
		wq->wqcfg.pasid_en = 0;
	}

	if (wq->type == IDXD_WQT_KERNEL)
		wq->wqcfg.priv = 1;

	if (idxd->type == IDXD_TYPE_DSA &&
	    idxd->hw.gen_cap.block_on_fault &&
	    test_bit(WQ_FLAG_BLOCK_ON_FAULT, &wq->flags))
		wq->wqcfg.bof = 1;

	wq_offset = idxd->wqcfg_offset + wq->id * 32 + 2 * sizeof(u32);
	iowrite32(wq->wqcfg.bits[2], idxd->reg_base + wq_offset);

	return 0;
}

static int idxd_wq_config_write(struct idxd_wq *wq)
{
	struct idxd_device *idxd = wq->idxd;
	struct device *dev = &idxd->pdev->dev;
	u32 wq_offset;
	int i;

	if (!wq->group)
		return 0;

	memset(&wq->wqcfg, 0, sizeof(union wqcfg));

	/* byte 0-3 */
	wq->wqcfg.wq_size = wq->size;

	if (wq->size == 0) {
		dev_warn(dev, "Incorrect work queue size: 0\n");
		return -EINVAL;
	}

	/* bytes 4-7 */
	wq->wqcfg.wq_thresh = wq->threshold;

	/* byte 8-11 */
	wq->wqcfg.priv = wq->type == IDXD_WQT_KERNEL ? 1 : 0;
	if (wq_dedicated(wq))
		wq->wqcfg.mode = 1;

	if (idxd->pasid_enabled) {
		wq->wqcfg.pasid_en = 1;
		if (wq->type == IDXD_WQT_KERNEL && wq_dedicated(wq))
			wq->wqcfg.pasid = idxd->pasid;
	}

	wq->wqcfg.priority = wq->priority;

	if (idxd->type == IDXD_TYPE_DSA &&
	    idxd->hw.gen_cap.block_on_fault &&
	    test_bit(WQ_FLAG_BLOCK_ON_FAULT, &wq->flags))
		wq->wqcfg.bof = 1;

	/* bytes 12-15 */
	wq->wqcfg.max_xfer_shift = idxd->hw.gen_cap.max_xfer_shift;
	wq->wqcfg.max_batch_shift = idxd->hw.gen_cap.max_batch_shift;

	dev_dbg(dev, "WQ %d CFGs\n", wq->id);
	for (i = 0; i < 8; i++) {
		wq_offset = idxd->wqcfg_offset + wq->id * 32 + i * sizeof(u32);
		iowrite32(wq->wqcfg.bits[i], idxd->reg_base + wq_offset);
		dev_dbg(dev, "WQ[%d][%d][%#x]: %#x\n",
			wq->id, i, wq_offset,
			ioread32(idxd->reg_base + wq_offset));
	}

	return 0;
}

static int idxd_wqs_config_write(struct idxd_device *idxd, bool rw)
{
	int i, rc;

	for (i = 0; i < idxd->max_wqs; i++) {
		struct idxd_wq *wq = &idxd->wqs[i];

		if (rw)
			rc = idxd_wq_config_write(wq);
		else
			rc = idxd_wq_config_write_ro(wq);
		if (rc < 0)
			return rc;
	}

	return 0;
}

static void idxd_group_flags_setup(struct idxd_device *idxd)
{
	int i;

	/* TC-A 0 and TC-B 1 should be defaults */
	for (i = 0; i < idxd->max_groups; i++) {
		struct idxd_group *group = &idxd->groups[i];

		if (group->tc_a == -1)
			group->tc_a = group->grpcfg.flags.tc_a = 0;
		else
			group->grpcfg.flags.tc_a = group->tc_a;
		if (group->tc_b == -1)
			group->tc_b = group->grpcfg.flags.tc_b = 1;
		else
			group->grpcfg.flags.tc_b = group->tc_b;
		group->grpcfg.flags.use_token_limit = group->use_token_limit;
		group->grpcfg.flags.tokens_reserved = group->tokens_reserved;
		if (group->tokens_allowed)
			group->grpcfg.flags.tokens_allowed =
				group->tokens_allowed;
		else
			group->grpcfg.flags.tokens_allowed = idxd->max_tokens;
	}
}

static int idxd_engines_setup(struct idxd_device *idxd)
{
	int i, engines = 0;
	struct idxd_engine *eng;
	struct idxd_group *group;

	for (i = 0; i < idxd->max_groups; i++) {
		group = &idxd->groups[i];
		group->grpcfg.engines = 0;
	}

	for (i = 0; i < idxd->max_engines; i++) {
		eng = &idxd->engines[i];
		group = eng->group;

		if (!group)
			continue;

		group->grpcfg.engines |= BIT(eng->id);
		engines++;
	}

	if (!engines)
		return -EINVAL;

	return 0;
}

static int idxd_wqs_setup(struct idxd_device *idxd)
{
	struct idxd_wq *wq;
	struct idxd_group *group;
	int i, j, configured = 0;
	struct device *dev = &idxd->pdev->dev;

	for (i = 0; i < idxd->max_groups; i++) {
		group = &idxd->groups[i];
		for (j = 0; j < 4; j++)
			group->grpcfg.wqs[j] = 0;
	}

	for (i = 0; i < idxd->max_wqs; i++) {
		wq = &idxd->wqs[i];
		group = wq->group;

		if (!wq->group)
			continue;
		if (!wq->size)
			continue;

		if (!wq_dedicated(wq) && !idxd->pasid_enabled) {
			dev_warn(dev, "No shared wq support but configured.\n");
			return -EINVAL;
		}

		group->grpcfg.wqs[wq->id / 64] |= BIT(wq->id % 64);
		configured++;
	}

	if (configured == 0)
		return -EINVAL;

	return 0;
}

int idxd_device_ro_config(struct idxd_device *idxd)
{
	lockdep_assert_held(&idxd->dev_lock);
	return idxd_wqs_config_write(idxd, false);
}

int idxd_device_config(struct idxd_device *idxd)
{
	int rc;

	lockdep_assert_held(&idxd->dev_lock);
	rc = idxd_wqs_setup(idxd);
	if (rc < 0)
		return rc;

	rc = idxd_engines_setup(idxd);
	if (rc < 0)
		return rc;

	idxd_group_flags_setup(idxd);

	rc = idxd_wqs_config_write(idxd, true);
	if (rc < 0)
		return rc;

	rc = idxd_groups_config_write(idxd);
	if (rc < 0)
		return rc;

	return 0;
}

static void idxd_wq_load_config(struct idxd_wq *wq)
{
	struct idxd_device *idxd = wq->idxd;
	struct device *dev = &idxd->pdev->dev;
	int wqcfg_offset;
	int i;

	wqcfg_offset = idxd->wqcfg_offset + wq->id * 32;
	memcpy_fromio(&wq->wqcfg, idxd->reg_base + wqcfg_offset,
		      sizeof(union wqcfg));

	wq->size = wq->wqcfg.wq_size;
	wq->threshold = wq->wqcfg.wq_thresh;
	if (wq->wqcfg.priv)
		wq->type = IDXD_WQT_KERNEL;

	if (wq->wqcfg.mode)
		set_bit(WQ_FLAG_DEDICATED, &wq->flags);

	wq->priority = wq->wqcfg.priority;

	if (wq->wqcfg.bof)
		set_bit(WQ_FLAG_BLOCK_ON_FAULT, &wq->flags);

	if (idxd->pasid_enabled) {
		wq->wqcfg.pasid_en = 1;
		wqcfg_offset = idxd->wqcfg_offset +
			       wq->id * 32 + 2 * sizeof(u32);
		iowrite32(wq->wqcfg.bits[2], idxd->reg_base + wqcfg_offset);
	}

	for (i = 0; i < 8; i++) {
		wqcfg_offset = idxd->wqcfg_offset +
			       wq->id * 32 + i * sizeof(u32);
		dev_dbg(dev, "WQ[%d][%d][%#x]: %#x\n",
			wq->id, i, wqcfg_offset, wq->wqcfg.bits[i]);
	}
}

static void idxd_group_load_config(struct idxd_group *group)
{
	struct idxd_device *idxd = group->idxd;
	struct device *dev = &idxd->pdev->dev;
	int i, j, grpcfg_offset;

	/* load wqs */
	for (i = 0; i < 4; i++) {
		struct idxd_wq *wq;

		grpcfg_offset = idxd->grpcfg_offset +
				group->id * 64 + i * sizeof(u64);
		group->grpcfg.wqs[i] =
			ioread64(idxd->reg_base + grpcfg_offset);
		dev_dbg(dev, "GRPCFG wq[%d:%d: %#x]: %#llx\n",
			group->id, i, grpcfg_offset,
			group->grpcfg.wqs[i]);

		for (j = 0; j < sizeof(u64); j++) {
			int wq_id = i * 64 + j;

			if (wq_id >= idxd->max_wqs)
				break;
			if (group->grpcfg.wqs[i] & BIT(j)) {
				wq = &idxd->wqs[wq_id];
				wq->group = group;
			}
		}
	}

	grpcfg_offset = idxd->grpcfg_offset + group->id * 64 + 32;
	group->grpcfg.engines = ioread64(idxd->reg_base + grpcfg_offset);
	dev_dbg(dev, "GRPCFG engs[%d: %#x]: %#llx\n", group->id,
		grpcfg_offset, group->grpcfg.engines);

	for (i = 0; i < sizeof(u64); i++) {
		if (i > idxd->max_engines)
			break;
		if (group->grpcfg.engines & BIT(i)) {
			struct idxd_engine *engine = &idxd->engines[i];

			engine->group = group;
		}
	}

	grpcfg_offset = idxd->grpcfg_offset + group->id * 64 + 40;
	group->grpcfg.flags.bits = ioread32(idxd->reg_base + grpcfg_offset);
	dev_dbg(dev, "GRPFLAGS flags[%d: %#x]: %#x\n",
		group->id, grpcfg_offset, group->grpcfg.flags.bits);
}

void idxd_device_load_config(struct idxd_device *idxd)
{
	union gencfg_reg reg;
	int i;

	reg.bits = ioread32(idxd->reg_base + IDXD_GENCFG_OFFSET);
	idxd->token_limit = reg.token_limit;

	for (i = 0; i < idxd->max_groups; i++) {
		struct idxd_group *group = &idxd->groups[i];

		idxd_group_load_config(group);
	}

	for (i = 0; i < idxd->max_wqs; i++) {
		struct idxd_wq *wq = &idxd->wqs[i];

		idxd_wq_load_config(wq);
	}
}
