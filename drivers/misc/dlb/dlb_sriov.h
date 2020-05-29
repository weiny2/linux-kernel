/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright(c) 2017-2020 Intel Corporation
 */

#ifndef __DLB_SRIOV_H
#define __DLB_SRIOV_H

int dlb_pci_sriov_configure(struct pci_dev *pdev, int num_vfs);

#endif /* __DLB_SRIOV_H */
