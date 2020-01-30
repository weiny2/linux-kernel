/* SPDX-License-Identifier: GPL-2.0 only */
/*****************************************************************************
 *
 * Intel Keem Bay XLink PCIe Driver
 *
 * Copyright (C) 2019 Intel Corporation
 *
 ****************************************************************************/

#ifndef MXLK_PCI_HEADER_
#define MXLK_PCI_HEADER_

#include <linux/list.h>
#include <linux/interrupt.h>
#include "../common/mxlk.h"
#include "../common/mxlk_boot.h"

#define MXLK_MAX_NAME_LEN (32)

struct mxlk_pcie {
	struct list_head list;
	struct mutex lock;

	struct pci_dev *pci;
	char name[MXLK_MAX_NAME_LEN];
	u32 devid;

	struct delayed_work wait_event;
	struct work_struct irq_event;
	wait_queue_head_t waitqueue;
	bool irq_enabled;

	char partition_name[MXLK_BOOT_DEST_STRLEN];
	unsigned long partition_offset;
	void *dma_buf;
	size_t dma_buf_offset;

	struct mxlk mxlk;
};

int mxlk_pci_init(struct mxlk_pcie *xdev, struct pci_dev *pdev);
int mxlk_pci_cleanup(struct mxlk_pcie *xdev);
int mxlk_pci_register_irq(struct mxlk_pcie *xdev, irq_handler_t irq_handler);

u32 mxlk_get_device_num(u32 *id_list);
int mxlk_get_device_name_by_id(u32 id, char *device_name, size_t name_size);
int mxlk_get_device_status_by_name(const char *name, u32 *status);

int mxlk_pci_boot_device(const char *device_name);
int mxlk_pci_connect_device(const char *device_name, void **fd);
int mxlk_pci_read(void *fd, void *data, size_t *size, uint32_t timeout);
int mxlk_pci_write(void *fd, void *data, size_t *size, uint32_t timeout);
int mxlk_pci_reset_device(void *fd);

u64 mxlk_pci_hw_dev_id(struct mxlk_pcie *xdev);

#endif
