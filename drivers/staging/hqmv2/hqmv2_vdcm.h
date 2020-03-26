/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright(c) 2018-2019 Intel Corporation
 */

#ifndef __HQMV2_VDCM_H
#define __HQMV2_VDCM_H

#ifdef CONFIG_INTEL_HQMV2_SIOV

#include <linux/pci.h>

int hqmv2_vdcm_init(struct pci_dev *pdev);
void hqmv2_vdcm_exit(struct pci_dev *pdev);

#endif /* CONFIG_INTEL_HQMV2_SIOV */
#endif /* __HQMV2_VDCM_H */
