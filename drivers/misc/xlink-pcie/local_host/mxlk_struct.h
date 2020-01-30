/* SPDX-License-Identifier: GPL-2.0 only */
/*****************************************************************************
 *
 * Intel Keem Bay XLink PCIe Driver
 *
 * Copyright (C) 2019 Intel Corporation
 *
 ****************************************************************************/

#ifndef MXLK_STRUCT_HEADER_
#define MXLK_STRUCT_HEADER_

#include <linux/pci-epc.h>
#include <linux/pci-epf.h>
#include "../common/mxlk.h"

#define MXLK_MAX_NAME_LEN (32)

struct mxlk_epf {
	struct pci_epf			*epf;
	void __iomem			*vaddr[BAR_5 + 1];
	enum pci_barno			comm_bar;
	enum pci_barno			bar4;
	const struct pci_epc_features	*epc_features;
	struct mxlk			mxlk;
	int				irq;
	int				irq_dma;
	void __iomem			*apb_base;

	wait_queue_head_t		dma_rd_wq;
	wait_queue_head_t		dma_wr_wq;
	u32                             sw_devid;
        char                            name[MXLK_MAX_NAME_LEN];
	struct list_head 		list;
};

/*
 * TODO FIXME?
 * Below are copied from drivers/pci/controller/dwc/pcie-designware.h, need to
 * get dbi_base to access DMA registers from this epf driver.
 */

struct dw_pcie_ep {
	struct pci_epc		*epc;
	const struct dw_pcie_ep_ops *ops;
	phys_addr_t		phys_base;
	size_t			addr_size;
	size_t			page_size;
	u8			bar_to_atu[6];
	phys_addr_t		*outbound_addr;
	unsigned long		*ib_window_map;
	unsigned long		*ob_window_map;
	u32			num_ib_windows;
	u32			num_ob_windows;
	void __iomem		*msi_mem;
	phys_addr_t		msi_mem_phys;
	u8			msi_cap;	/* MSI capability offset */
	u8			msix_cap;	/* MSI-X capability offset */
};

#define MAX_MSI_IRQS			256
#define MAX_MSI_IRQS_PER_CTRL		32
#define MAX_MSI_CTRLS			(MAX_MSI_IRQS / MAX_MSI_IRQS_PER_CTRL)

struct pcie_port {
	u8			root_bus_nr;
	u64			cfg0_base;
	void __iomem		*va_cfg0_base;
	u32			cfg0_size;
	u64			cfg1_base;
	void __iomem		*va_cfg1_base;
	u32			cfg1_size;
	resource_size_t		io_base;
	phys_addr_t		io_bus_addr;
	u32			io_size;
	u64			mem_base;
	phys_addr_t		mem_bus_addr;
	u32			mem_size;
	struct resource		*cfg;
	struct resource		*io;
	struct resource		*mem;
	struct resource		*busn;
	int			irq;
	const struct dw_pcie_host_ops *ops;
	int			msi_irq;
	struct irq_domain	*irq_domain;
	struct irq_domain	*msi_domain;
	dma_addr_t		msi_data;
	struct page		*msi_page;
	struct irq_chip		*msi_irq_chip;
	u32			num_vectors;
	u32			irq_mask[MAX_MSI_CTRLS];
	struct pci_bus		*root_bus;
	raw_spinlock_t		lock;
	DECLARE_BITMAP(msi_irq_in_use, MAX_MSI_IRQS);
};

struct dw_pcie {
	struct device		*dev;
	void __iomem		*dbi_base;
	void __iomem		*dbi_base2;
	/* Used when iatu_unroll_enabled is true */
	void __iomem		*atu_base;
	u32			num_viewport;
	u8			iatu_unroll_enabled;
	struct pcie_port	pp;
	struct dw_pcie_ep	ep;
	const struct dw_pcie_ops *ops;
	unsigned int		version;
};

#define to_dw_pcie_from_ep(endpoint)   \
		container_of((endpoint), struct dw_pcie, ep)

/*
 * TODO FIXME
 * Below are copied from KMB EPC drivers/pci/controller/dwc/pcie-keembay.c
 */

enum dw_pcie_device_mode {
	DW_PCIE_UNKNOWN_TYPE,
	DW_PCIE_EP_TYPE,
	DW_PCIE_LEG_EP_TYPE,
	DW_PCIE_RC_TYPE,
};

struct keembay_pcie {
	struct dw_pcie		*pci;
	void __iomem		*slv_apb_base;
	enum dw_pcie_device_mode mode;
	struct resource 	*mmr2[8];
	struct resource 	*mmr4[8];
	bool			setup_bar[8];
	int			irq;
	int			irq_dma;
};

#define to_keembay_pcie(x)				dev_get_drvdata((x)->dev)

#endif // MXLK_STRUCT_
