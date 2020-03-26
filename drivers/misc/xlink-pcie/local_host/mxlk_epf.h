/* SPDX-License-Identifier: GPL-2.0 only */
/*****************************************************************************
 *
 * Intel Keem Bay XLink PCIe Driver
 *
 * Copyright (C) 2019 Intel Corporation
 *
 ****************************************************************************/

#ifndef MXLK_EPF_HEADER_
#define MXLK_EPF_HEADER_

#include "../common/mxlk.h"

#define MXLK_DMA_TODEVICE_DONE (1 << 0)
#define MXLK_DMA_FROMDEVICE_DONE (1 << 1)
#define MXLK_OS_READY (1 << 2)

extern int mxlk_register_host_irq(struct mxlk *mxlk, irq_handler_t func,
				  unsigned int flags);

extern int mxlk_raise_irq(struct mxlk *mxlk, unsigned int flags);

extern int mxlk_copy_from_host(struct mxlk *mxlk, void *dst_addr, u64 pci_addr,
			       size_t len);
extern int mxlk_copy_to_host(struct mxlk *mxlk, u64 pci_addr, void *src_addr,
			     size_t len);

#endif // MXLK_EPF_
