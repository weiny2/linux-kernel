// SPDX-License-Identifier: GPL-2.0 only
/*****************************************************************************
 *
 * Intel Keem Bay XLink PCIe Driver
 *
 * Copyright (C) 2019 Intel Corporation
 *
 ****************************************************************************/

#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/types.h>
#include "mxlk_pci.h"
#include "../common/mxlk_boot.h"
#include "../common/mxlk_core.h"
#include "../common/mxlk_mmio.h"
#include "../common/mxlk_print.h"

static int aspm_enable;
module_param(aspm_enable, int, 0664);
MODULE_PARM_DESC(aspm_enable, "enable ASPM");

static bool os_ready;

static void mxlk_pci_set_aspm(struct mxlk *mxlk, int aspm)
{
	u8 cap_exp;
	u16 link_control;

	cap_exp = pci_find_capability(mxlk->pci, PCI_CAP_ID_EXP);
	if (!cap_exp) {
		mxlk_error("failed to find pcie capability\n");
		return;
	}

	pci_read_config_word(mxlk->pci, cap_exp + PCI_EXP_LNKCTL,
			     &link_control);
	link_control &= ~(PCI_EXP_LNKCTL_ASPMC);
	link_control |= (aspm & PCI_EXP_LNKCTL_ASPMC);
	pci_write_config_word(mxlk->pci, cap_exp + PCI_EXP_LNKCTL,
			      link_control);
}

static void mxlk_pci_unmap_bar(struct mxlk *mxlk)
{
	if (mxlk->io_comm) {
		iounmap(mxlk->io_comm);
		mxlk->io_comm = NULL;
		mxlk->mmio = NULL;
	}

	if (mxlk->bar4) {
		iounmap(mxlk->bar4);
		mxlk->bar4 = NULL;
	}
}

static int mxlk_pci_map_bar(struct mxlk *mxlk)
{
	if (pci_resource_len(mxlk->pci, 2) < MXLK_IO_COMM_SIZE) {
		mxlk_error("device BAR region is too small\n");
		return -1;
	}

	mxlk->io_comm = pci_ioremap_bar(mxlk->pci, 2);
	if (!mxlk->io_comm) {
		mxlk_error("failed to ioremap BAR2\n");
		goto bar_error;
	}
	mxlk->mmio = mxlk->io_comm + MXLK_MMIO_OFFSET;

	mxlk->bar4 = pci_ioremap_bar(mxlk->pci, 4);
	if (!mxlk->bar4) {
		mxlk_error("failed to ioremap BAR4\n");
		goto bar_error;
	}

	return 0;

bar_error:
	mxlk_pci_unmap_bar(mxlk);
	return -1;
}

#define STR_EQUAL(a, b) !strncmp(a, b, strlen(b))

static enum mxlk_stage mxlk_check_magic(struct mxlk *mxlk)
{
	char magic[MXLK_BOOT_MAGIC_STRLEN];

	mxlk_rd_buffer(mxlk->io_comm, MXLK_BOOT_OFFSET_MAGIC, magic,
		       MXLK_BOOT_MAGIC_STRLEN);

	if (strlen(magic) == 0)
		return STAGE_UNINIT;

	if (STR_EQUAL(magic, MXLK_BOOT_MAGIC_ROM))
		return STAGE_ROM;

	if (STR_EQUAL(magic, MXLK_BOOT_MAGIC_EMMC))
		return STAGE_ROM;

	if (STR_EQUAL(magic, MXLK_BOOT_MAGIC_BL2))
		return STAGE_BL2;

	if (STR_EQUAL(magic, MXLK_BOOT_MAGIC_UBOOT))
		return STAGE_UBOOT;

	if (STR_EQUAL(magic, MXLK_BOOT_MAGIC_YOCTO))
		return STAGE_OS;

	return STAGE_UNINIT;
}

static irqreturn_t mxlk_boot_interrupt(int irq, void *args)
{
	struct mxlk *mxlk = args;

	schedule_work(&mxlk->fw_event);

	return IRQ_HANDLED;
}

static void mxlk_pci_irq_cleanup(struct mxlk *mxlk)
{
#if KERNEL_VERSION(4, 8, 0) <= LINUX_VERSION_CODE
	int irq = pci_irq_vector(mxlk->pci, 0);

	if (irq >= 0) {
		synchronize_irq(irq);
		free_irq(irq, mxlk);
		pci_free_irq_vectors(mxlk->pci);
	}
#else
	if (pci_msi_enabled()) {
		synchronize_irq(mxlk->pci->irq);
		free_irq(mxlk->pci->irq, mxlk);
		pci_disable_msi(mxlk->pci);
	}
#endif
}

#if KERNEL_VERSION(4, 8, 0) <= LINUX_VERSION_CODE
static int mxlk_pci_irq_init(struct mxlk *mxlk)
{
	int irq;
	int error;

	error = pci_alloc_irq_vectors(mxlk->pci, 1, 1, PCI_IRQ_MSI);
	if (error < 0) {
		mxlk_error("failed to allocate %d MSI vectors\n", 1);
		return error;
	}

	irq = pci_irq_vector(mxlk->pci, 0);
	if (irq < 0) {
		mxlk_error("failed to get irq\n");
		error = irq;
		goto error_irq;
	}
	error = request_irq(irq, &mxlk_boot_interrupt, 0, MXLK_DRIVER_NAME,
			    mxlk);
	if (error) {
		mxlk_error("failed to request irqs\n");
		goto error_irq;
	}

	return 0;

error_irq:
	pci_free_irq_vectors(mxlk->pci);
	return error;
}
#else
static int mxlk_pci_irq_init(struct mxlk *mxlk)
{
	int error;

	error = pci_enable_msi(mxlk->pci);
	if (error < 1) {
		mxlk_error("failed to allocate %d MSI vectors\n", 1);
		return error;
	}

	error = request_irq(mxlk->pci->irq, &mxlk_boot_interrupt, 0,
			    MXLK_DRIVER_NAME, mxlk);
	if (error) {
		mxlk_error("failed to request irqs\n");
		goto error_irq;
	}

	return 0;

error_irq:
	pci_disable_msi(mxlk->pci);
	return error;
}
#endif

static int mxlk_device_wait_transfer(struct mxlk *mxlk, u32 image_id,
				     dma_addr_t addr, size_t size)
{
	u32 status = MXLK_BOOT_STATUS_START;

	mxlk_wr64(mxlk->io_comm, MXLK_BOOT_OFFSET_MF_START, addr);
	mxlk_wr32(mxlk->io_comm, MXLK_BOOT_OFFSET_MF_LEN, size);
	mxlk_wr32(mxlk->io_comm, MXLK_BOOT_OFFSET_MF_READY, image_id);

	while (status != MXLK_BOOT_STATUS_DOWNLOADED) {
		mdelay(1);

		status = mxlk_rd32(mxlk->io_comm, MXLK_BOOT_OFFSET_MF_READY);

		switch (status) {
		case MXLK_BOOT_STATUS_INVALID:
			mxlk_error("the firmware image data is invalid.\n");
			return -EINVAL;
		case MXLK_BOOT_STATUS_ERROR:
			mxlk_error("failed to download firmware image.\n");
			return -EINVAL;
		default:
			break;
		}
	}

	return 0;
}

static int mxlk_device_download(struct mxlk *mxlk, u32 image_id)
{
	const struct firmware *firmware;
	const char *fw_image;
	void *fw_buffer = NULL;
	const size_t buffer_size = 1024 * 1024;
	dma_addr_t phys_addr;
	size_t size_left;
	const u8 *data;
	size_t size;
	struct device *dev = &mxlk->pci->dev;
	int error = 0;

	switch (image_id) {
	case MXLK_BOOT_FIP_ID:
		fw_image = MXLK_FIP_FW_NAME;
		break;
	case MXLK_BOOT_OS_ID:
		fw_image = MXLK_OS_FW_NAME;
		break;
	case MXLK_BOOT_ROOTFS_ID:
		fw_image = MXLK_ROOTFS_FW_NAME;
		break;
	default:
		mxlk_error("unknown firmware id\n");
		return -EINVAL;
	}

	if (request_firmware(&firmware, fw_image, dev) < 0)
		return -EIO;

	if (firmware->size == 0) {
		error = -EIO;
		goto cleanup;
	}

	fw_buffer = kmalloc(buffer_size, GFP_KERNEL);
	if (!fw_buffer) {
		error = -ENOMEM;
		goto cleanup;
	}

	data = firmware->data;
	size_left = firmware->size;

	while (size_left) {
		size = (size_left > buffer_size) ? buffer_size : size_left;

		memcpy(fw_buffer, data, size);

		phys_addr = dma_map_single(dev, fw_buffer, size, DMA_TO_DEVICE);
		if (dma_mapping_error(dev, phys_addr)) {
			error = -ENOMEM;
			goto cleanup;
		}

		error = mxlk_device_wait_transfer(mxlk, image_id, phys_addr,
						  size);
		if (error)
			goto cleanup;

		dma_unmap_single(dev, phys_addr, size, DMA_TO_DEVICE);

		size_left -= size;
		data += size;
	}

cleanup:
	kfree(fw_buffer);
	release_firmware(firmware);

	return error;
}

static void mxlk_device_fw(struct work_struct *work)
{
	int error;
	struct mxlk *mxlk = container_of(work, struct mxlk, fw_event);
	enum mxlk_stage stage = mxlk_check_magic(mxlk);

	if (stage == STAGE_ROM) {
		error = mxlk_device_download(mxlk, MXLK_BOOT_FIP_ID);
		if (error) {
			mxlk_error("failed to download FIP image\n");
			return;
		}
	} else if (stage == STAGE_UBOOT) {
		error = mxlk_device_download(mxlk, MXLK_BOOT_OS_ID);
		if (error) {
			mxlk_error("failed to download OS image\n");
			return;
		}

		error = mxlk_device_download(mxlk, MXLK_BOOT_ROOTFS_ID);
		if (error) {
			mxlk_error("failed to download ROOTFS image\n");
			return;
		}
	} else {
		mxlk_error("it's not expected to receive MSI at stage %d\n",
			   stage);
		return;
	}

	mxlk_wr32(mxlk->io_comm, MXLK_BOOT_OFFSET_MF_READY,
		  MXLK_BOOT_STATUS_DONE);
}

static void mxlk_device_enable_irq(struct mxlk *mxlk)
{
	mxlk_wr32(mxlk->io_comm, MXLK_BOOT_OFFSET_INT_ENABLE,
		  MXLK_INT_ENABLE);
	mxlk_wr32(mxlk->io_comm, MXLK_BOOT_OFFSET_INT_MASK, ~MXLK_INT_MASK);
}

static void mxlk_device_wait(struct work_struct *work)
{
	struct mxlk *mxlk = container_of(work, struct mxlk, wait_event.work);
	static bool rom_irq_enabled = false;
	enum mxlk_stage stage = mxlk_check_magic(mxlk);

	if (stage == STAGE_ROM && !rom_irq_enabled) {
		mxlk_device_enable_irq(mxlk);
		rom_irq_enabled = true;
	} else if (stage == STAGE_OS) {
		mxlk->status = MXLK_STATUS_BOOT;
		if (mxlk_core_init(mxlk))
			mxlk_error("failed to sync with device\n");
		else
			os_ready = true;
		return;
	}

	schedule_delayed_work(&mxlk->wait_event, msecs_to_jiffies(100));
}

static int mxlk_device_init(struct mxlk *mxlk)
{
	int error;

	INIT_DELAYED_WORK(&mxlk->wait_event, mxlk_device_wait);
	INIT_WORK(&mxlk->fw_event, mxlk_device_fw);

	error = mxlk_pci_irq_init(mxlk);
	if (error)
		return error;

	pci_set_master(mxlk->pci);

	schedule_delayed_work(&mxlk->wait_event, 0);

	return error;
}

int mxlk_pci_init(struct mxlk *mxlk, struct pci_dev *pdev)
{
	int error;

	mxlk->pci = pdev;
	pci_set_drvdata(pdev, mxlk);

	error = pci_enable_device_mem(mxlk->pci);
	if (error) {
		mxlk_error("failed to enable pci device\n");
		return error;
	}

	error = pci_request_regions(mxlk->pci, MXLK_DRIVER_NAME);
	if (error) {
		mxlk_error("failed to request mmio regions\n");
		goto error_req_mem;
	}

	if (mxlk_pci_map_bar(mxlk)) {
		error = -EINVAL;
		goto error_map;
	}

	error = dma_set_mask_and_coherent(&mxlk->pci->dev, DMA_BIT_MASK(64));
	if (error) {
		mxlk_info("Unable to set dma mask for 64bit\n");
		error = dma_set_mask_and_coherent(&mxlk->pci->dev,
						  DMA_BIT_MASK(32));
		if (error) {
			mxlk_error("failed to set dma mask for 32bit\n");
			goto error_dma_mask;
		}
	}

	mxlk_pci_set_aspm(mxlk, aspm_enable);

	error = mxlk_device_init(mxlk);
	if (error)
		goto error_boot;

	return 0;

error_boot:
error_dma_mask:
	mxlk_pci_unmap_bar(mxlk);

error_map:
	pci_release_regions(mxlk->pci);

error_req_mem:
	pci_disable_device(mxlk->pci);

	mxlk->status = MXLK_STATUS_ERROR;

	return error;
}

void mxlk_pci_cleanup(struct mxlk *mxlk)
{
	cancel_delayed_work_sync(&mxlk->wait_event);
	cancel_work_sync(&mxlk->fw_event);
	mxlk_pci_irq_cleanup(mxlk);

	if (os_ready)
		mxlk_core_cleanup(mxlk);

	mxlk_pci_unmap_bar(mxlk);

	pci_release_regions(mxlk->pci);
	pci_disable_device(mxlk->pci);
	pci_set_drvdata(mxlk->pci, NULL);
}

int mxlk_pci_register_irq(struct mxlk *mxlk, irq_handler_t irq_handler)
{
	int irq;
	int error;

	if (mxlk->status != MXLK_STATUS_BOOT) {
		mxlk_error("failed to request irqs, device not booted\n");
		return -EINVAL;
	}

#if KERNEL_VERSION(4, 8, 0) <= LINUX_VERSION_CODE
	irq = pci_irq_vector(mxlk->pci, 0);
	if (irq < 0) {
		mxlk_error("failed to get irq number\n");
		return irq;
	}
#else
	irq = mxlk->pci->irq;
#endif

	synchronize_irq(irq);
	free_irq(irq, mxlk);

	error = request_irq(irq, irq_handler, 0, MXLK_DRIVER_NAME, mxlk);
	if (error) {
		mxlk_error("failed to request irqs - %d\n", error);
		return error;
	}

	return 0;
}
