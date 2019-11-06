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

#include <linux/interrupt.h>
#include "../common/mxlk.h"

int mxlk_pci_init(struct mxlk *mxlk, struct pci_dev *pdev);

void mxlk_pci_cleanup(struct mxlk *mxlk);

int mxlk_pci_register_irq(struct mxlk *mxlk, irq_handler_t irq_handler);

#endif
