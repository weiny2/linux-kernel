/* SPDX-License-Identifier: GPL-2.0 only */
/*****************************************************************************
 *
 * Intel Keem Bay XLink PCIe Driver
 *
 * Copyright (C) 2019 Intel Corporation
 *
 ****************************************************************************/

#ifndef MXLK_HEADER_
#define MXLK_HEADER_

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/stddef.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/version.h>
#include <linux/mempool.h>
#include <linux/dma-mapping.h>
#include <linux/cache.h>
#include <linux/wait.h>

#include "mxlk_common.h"

#ifdef XLINK_PCIE_REMOTE
#define MXLK_DRIVER_NAME "mxlk"
#define MXLK_DRIVER_DESC "Intel(R) Keem Bay XLink PCIe driver"
#else
#define MXLK_DRIVER_NAME "mxlk_pcie_epf"
#define MXLK_DRIVER_DESC "Intel(R) xLink PCIe endpoint function driver"
#endif

struct mxlk_pipe {
	u32 old;
	u32 ndesc;
	u32 *head;
	u32 *tail;
	struct mxlk_transfer_desc *tdr;
};

struct mxlk_buf_desc {
	struct mxlk_buf_desc *next;
	void *head;
	size_t true_len;
	void *data;
	size_t length;
	int interface;
};

struct mxlk_dma_desc {
	struct mxlk_buf_desc *bd;
	dma_addr_t phys;
	size_t length;
};

struct mxlk_stream {
	size_t frag;
	struct mxlk_pipe pipe;
#ifdef XLINK_PCIE_REMOTE
	struct mxlk_dma_desc *ddr;
#endif
};

struct mxlk_list {
	spinlock_t lock;
	size_t bytes;
	size_t buffers;
	struct mxlk_buf_desc *head;
	struct mxlk_buf_desc *tail;
};

struct mxlk_interface {
	int id;
	struct mxlk *mxlk;
	struct mutex rlock;
	struct mxlk_list read;
	struct mxlk_buf_desc *partial_read;
	bool data_available;
	wait_queue_head_t rx_waitqueue;
};

struct mxlk {
	u32 status;

	void __iomem *io_comm; /* IO communication space */
	void __iomem *mmio; /* XLink memory space */
	void __iomem *bar4; /* not used for now */

	struct workqueue_struct *wq;

	struct mxlk_interface interfaces[MXLK_NUM_INTERFACES];

	size_t fragment_size;
	struct mxlk_cap_txrx *txrx;
	struct mxlk_stream tx;
	struct mxlk_stream rx;

	struct mutex wlock;
	struct mxlk_list write;
	bool no_tx_buffer;
	wait_queue_head_t tx_waitqueue;
	bool tx_pending;

	struct mxlk_list rx_pool;
	struct mxlk_list tx_pool;

	struct delayed_work rx_event;
	struct delayed_work tx_event;
};

#endif
