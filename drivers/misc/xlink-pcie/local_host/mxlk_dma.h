/* SPDX-License-Identifier: GPL-2.0 only */
/*****************************************************************************
 *
 * Intel Keem Bay XLink PCIe Driver
 *
 * Copyright (C) 2019 Intel Corporation
 *
 ****************************************************************************/

#ifndef MXLK_DMA_HEADER_
#define MXLK_DMA_HEADER_

#include <linux/types.h>
#include <linux/pci-epc.h>
#include <linux/pci-epf.h>

void mxlk_ep_dma_init(struct pci_epc *epc);
void mxlk_ep_dma_uninit(struct pci_epc *epc);
int mxlk_ep_dma_read(struct pci_epc *epc, dma_addr_t dst, dma_addr_t src,
		     size_t len);
int mxlk_ep_dma_write(struct pci_epc *epc, dma_addr_t dst, dma_addr_t src,
		      size_t len);

#endif
