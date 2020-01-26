/* SPDX-License-Identifier: GPL-2.0 */
#ifndef DRIVERS_PCIE_DPC_H
#define DRIVERS_PCIE_DPC_H

#include <linux/pci.h>

struct dpc_dev {
	struct pci_dev		*pdev;
	u16			cap_pos;
	bool			rp_extensions;
	u8			rp_log_size;
	u16			ctl;
	u16			cap;
};

pci_ers_result_t dpc_reset_link_common(struct dpc_dev *dpc);
void dpc_process_error(struct dpc_dev *dpc);
void dpc_dev_init(struct pci_dev *pdev, struct dpc_dev *dpc);

#endif /* DRIVERS_PCIE_DPC_H */
