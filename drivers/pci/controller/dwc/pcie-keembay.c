// SPDX-License-Identifier: GPL-2.0-only
/*
 * pcie-keembay - PCIe controller driver for Intel Keem Bay
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/gpio/consumer.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/delay.h>

#include "pcie-designware.h"

/* PCIE_REGS_APB_SLV Registers */
#define PCIE_REGS_PCIE_SUBSYSTEM_VERSION		0x0
#define PCIE_REGS_PCIE_CFG				0x4
#define  PCIE_REGS_PCIE_CFG_PCIE_RSTN			BIT(0)
#define PCIE_REGS_PCIE_APP_CNTRL			0x8
#define  PCIE_REGS_PCIE_APP_CNTRL_APP_LTSSM_EN		BIT(0)
#define PCIE_REGS_PCIE_DEBUG_AUX			0xc
#define PCIE_REGS_PCIE_DEBUG_CORE			0x10
#define PCIE_REGS_PCIE_SYS_CNTRL			0x14
#define PCIE_REGS_PCIE_INTR_ENABLE			0x18
#define PCIE_REGS_PCIE_INTR_ENABLE_CII			BIT(18)
#define PCIE_REGS_PCIE_INTR_FLAGS			0x1c
#define PCIE_REGS_PCIE_ERR_INTR_ENABLE			0x20
#define PCIE_REGS_PCIE_ERR_INTR_FLAGS			0x24
#define PCIE_REGS_INTERRUPT_ENABLE			0x28
#define PCIE_REGS_INTERRUPT_STATUS			0x2c
#define PCIE_REGS_PCIE_MISC_STATUS			0x30
#define PCIE_REGS_PCIE_MSI_STATUS			0x34
#define PCIE_REGS_PCIE_MSI_STATUS_IO			0x38
#define PCIE_REGS_PCIE_MSI_SII_DATA			0x3c
#define PCIE_REGS_PCIE_MSI_SII_CTRL			0x40
#define PCIE_REGS_PCIE_CFG_MSI				0x44
#define PCIE_REGS_PCIE_CFG_MSI_DATA			0x48
#define PCIE_REGS_PCIE_CFG_MSI_ADDR0			0x4c
#define PCIE_REGS_PCIE_CFG_MSI_ADDR1			0x50
#define PCIE_REGS_PCIE_CFG_MSI_MASK			0x54
#define PCIE_REGS_PCIE_CFG_MSI_PENDING			0x58
#define PCIE_REGS_PCIE_TRGT_TIMEOUT			0x5c
#define PCIE_REGS_PCIE_RADM_TIMEOUT			0x60
#define PCIE_REGS_VENDOR_MSG_PAYLOAD_0			0x64
#define PCIE_REGS_VENDOR_MSG_PAYLOAD_1			0x68
#define PCIE_REGS_VENDOR_MSG_REQ_ID			0x6c
#define PCIE_REGS_LTR_MSG_PAYLOAD_0			0x70
#define PCIE_REGS_LTR_MSG_PAYLOAD_1			0x74
#define PCIE_REGS_LTR_MSG_REQ_ID			0x78
#define PCIE_REGS_PCIE_SYS_CFG_CORE			0x7c
#define PCIE_REGS_PCIE_SYS_CFG_AUX			0x80
#define PCIE_REGS_PCIE_ELECTR_MECH			0x84
#define PCIE_REGS_PCIE_CII_HDR_0			0x88
#define PCIE_REGS_PCIE_CII_HDR_1			0x8c
#define PCIE_REGS_PCIE_CII_DATA				0x90
#define PCIE_REGS_PCIE_CII_CNTRL			0x94
#define PCIE_REGS_PCIE_CII_OVERRIDE_DATA		0x98
#define PCIE_REGS_PCIE_LTR_STA				0x9c
#define PCIE_REGS_PCIE_LTR_CNTRL			0xa0
#define PCIE_REGS_PCIE_LTR_MSG_LATENCY			0xa4
#define PCIE_REGS_PCIE_CFG_LTR_MAX_LATENCY		0xa8
#define PCIE_REGS_PCIE_APP_LTR_LATENCY			0xac
#define PCIE_REGS_PCIE_SII_PM_STATE			0xb0
#define  PCIE_REGS_PCIE_SII_PM_STATE_SMLH_LINK_UP	BIT(19)
#define  PCIE_REGS_PCIE_SII_PM_STATE_RDLH_LINK_UP	BIT(8)
#define PCIE_REGS_PCIE_SII_PM_STATE_1			0xb4
#define PCIE_REGS_VMI_CTRL				0xbc
#define PCIE_REGS_VMI_PARAMS_0				0xc0
#define PCIE_REGS_VMI_PARAMS_1				0xc4
#define PCIE_REGS_VMI_DATA_0				0xc8
#define PCIE_REGS_VMI_DATA_1				0xcc
#define PCIE_REGS_PCIE_DIAG_CTRL			0xd0
#define PCIE_REGS_PCIE_DIAG_STATUS_0			0xd4
#define PCIE_REGS_PCIE_DIAG_STATUS_1			0xd8
#define PCIE_REGS_PCIE_DIAG_STATUS_2			0xdc
#define PCIE_REGS_PCIE_DIAG_STATUS_3			0xe0
#define PCIE_REGS_PCIE_DIAG_STATUS_4			0xe4
#define PCIE_REGS_PCIE_DIAG_STATUS_5			0xe8
#define PCIE_REGS_PCIE_DIAG_STATUS_6			0xec
#define PCIE_REGS_PCIE_DIAG_STATUS_7			0xf0
#define PCIE_REGS_PCIE_DIAG_STATUS_8			0xf4
#define PCIE_REGS_PCIE_DIAG_STATUS_9			0xf8
#define PCIE_REGS_PCIE_DIAG_STATUS_10			0xfc
#define PCIE_REGS_PCIE_DIAG_STATUS_11			0x100
#define PCIE_REGS_PCIE_DIAG_STATUS_12			0x104
#define PCIE_REGS_PCIE_DIAG_STATUS_13			0x108
#define PCIE_REGS_PCIE_DIAG_STATUS_14			0x10c
#define PCIE_REGS_PCIE_DIAG_STATUS_15			0x110
#define PCIE_REGS_PCIE_DIAG_STATUS_16			0x114
#define PCIE_REGS_PCIE_DIAG_STATUS_17			0x118
#define PCIE_REGS_PCIE_DIAG_STATUS_18			0x11c
#define PCIE_REGS_PCIE_DIAG_STATUS_19			0x120
#define PCIE_REGS_PCIE_DIAG_STATUS_20			0x124
#define PCIE_REGS_PCIE_DIAG_STATUS_21			0x128
#define PCIE_REGS_CXPL_DEBUG_INFO_0			0x12c
#define PCIE_REGS_CXPL_DEBUG_INFO_1			0x130
#define PCIE_REGS_CXPL_DEBUG_INFO_EI			0x134
#define PCIE_REGS_PCIE_MSTR_RMISC_INFO_0		0x138
#define PCIE_REGS_PCIE_SLV_AWMISC_INFO_0		0x13c
#define PCIE_REGS_PCIE_SLV_AWMISC_INFO_1		0x140
#define PCIE_REGS_PCIE_SLV_AWMISC_INFO_2		0x144
#define PCIE_REGS_PCIE_SLV_AWMISC_INFO_3		0x148
#define PCIE_REGS_PCIE_SLV_ARMISC_INFO_0		0x14c
#define PCIE_REGS_PCIE_SLV_ARMISC_INFO_1		0x150
#define PCIE_REGS_PCIE_WAKE_CNTRL			0x154
#define PCIE_REGS_PCIE_WAKE_STA				0x158
#define PCIE_REGS_EXT_CLK_CNTRL				0x15c
#define PCIE_REGS_PCIE_LANE_FLIP			0x160
#define PCIE_REGS_PCIE_PHY_CNTL				0x164
#define  PCIE_REGS_PCIE_PHY_CNTL_PHY0_SRAM_BYPASS	BIT(8)
#define PCIE_REGS_PCIE_PHY_STAT				0x168
#define  PCIE_REGS_PCIE_PHY_STAT_PHY0_MPLLA_STATE	BIT(1)
#define  PCIE_REGS_PCIE_PHY_STAT_PHY0_MPLLB_STATE	BIT(2)
#define PCIE_REGS_LJPLL_STA				0x16c
#define  PCIE_REGS_LJPLL_STA_LJPLL_LOCK			BIT(0)
#define PCIE_REGS_LJPLL_CNTRL_0				0x170
#define  PCIE_REGS_LJPLL_CNTRL_0_LJPLL_EN		BIT(29)
#define  PCIE_REGS_LJPLL_CNTRL_0_LJPLL_FOUT_EN		GENMASK(24, 21)
#define PCIE_REGS_LJPLL_CNTRL_1				0x174
#define PCIE_REGS_LJPLL_CNTRL_2				0x178
#define  PCIE_REGS_LJPLL_CNTRL_2_LJPLL_REF_DIV		GENMASK(17, 12)
#define  PCIE_REGS_LJPLL_CNTRL_2_LJPLL_FB_DIV		GENMASK(11, 10)
#define PCIE_REGS_LJPLL_CNTRL_3				0x17c
#define  PCIE_REGS_LJPLL_CNTRL_3_LJPLL_POST_DIV3A	GENMASK(24, 22)
#define  PCIE_REGS_LJPLL_CNTRL_3_LJPLL_POST_DIV2A	GENMASK(15, 13)
#define PCIE_REGS_MEM_ACCESS_IRQ_VECTOR			0x180
#define PCIE_REGS_MEM_ACCESS_IRQ_ENABLE			0x184
#define PCIE_REGS_MEM_CTRL0				0xb8
#define PCIE_DBI2_MASK					(1 << 20)

struct keembay_pcie {
	struct dw_pcie		*pci;
	void __iomem		*base;
	enum dw_pcie_device_mode mode;
	struct resource		*mmr2;
	struct resource		*mmr4;
	bool			setup_bar;
	int 			irq;

	struct clk		*clk_master;
	struct clk		*clk_aux;
	struct gpio_desc	*reset;
};

struct keembay_pcie_of_data {
	enum dw_pcie_device_mode mode;
	const struct dw_pcie_host_ops *host_ops;
	const struct dw_pcie_ep_ops *ep_ops;
};

#define to_keembay_pcie(x)	dev_get_drvdata((x)->dev)

static inline u32 keembay_pcie_readl(struct keembay_pcie *pcie, u32 offset)
{
	return readl(pcie->base + offset);
}

static inline void keembay_pcie_writel(struct keembay_pcie *pcie, u32 offset,
				       u32 value)
{
	writel(value, pcie->base + offset);
}

/* Initialize the PCIe PLL in Host mode (assume 24MHz refclk) */
static void keembay_pcie_pll_init(struct keembay_pcie *pcie)
{
	struct dw_pcie *pci = pcie->pci;
	u32 val;

	val = (0x0 << 12) |	// [17:12] ljlpp_ref_div
		(0x32 << 0);	// [11: 0] ljpll_fb_div
	keembay_pcie_writel(pcie, PCIE_REGS_LJPLL_CNTRL_2, val);

	val = (0x2 << 22) |	// [24:22] ljpll_post_div3a
		(0x7 << 19) |	// [21:19] ljpll_post_div3b
		(0x2 << 16) |	// [18:16] ljpll_post_div2a
		(0x7 << 13);	// [15:13] ljpll_post_div2b
	keembay_pcie_writel(pcie, PCIE_REGS_LJPLL_CNTRL_3, val);

	val = (0x1 << 29) |	// [  :29] ljpll_en
		(0xc << 21);	// [24:21] ljpll_fout_en
	keembay_pcie_writel(pcie, PCIE_REGS_LJPLL_CNTRL_0, val);

	val = (0x1 << 29) |	// [  :29] ljpll_en
		(0xc << 21);	// [24:21] ljpll_fout_en
	keembay_pcie_writel(pcie, PCIE_REGS_LJPLL_CNTRL_0, val);

	/* Poll bit 0 of PCIE_REGS_LJPLL_STA register */
	do {
		udelay(1);
		val = keembay_pcie_readl(pcie, PCIE_REGS_LJPLL_STA);
	} while (!(val & PCIE_REGS_LJPLL_STA_LJPLL_LOCK));

	dev_info(pci->dev, "Low jitter PLL locked\n");
	dev_info(pci->dev,
		 "PCIE_REGS_LJPLL_CNTRL_2: 0x%08x\n"
		 "PCIE_REGS_LJPLL_CNTRL_3: 0x%08x\n"
		 "PCIE_REGS_LJPLL_CNTRL_3: 0x%08x\n",
		 keembay_pcie_readl(pcie, PCIE_REGS_LJPLL_CNTRL_2),
		 keembay_pcie_readl(pcie, PCIE_REGS_LJPLL_CNTRL_3),
		 keembay_pcie_readl(pcie, PCIE_REGS_LJPLL_CNTRL_0));
}

static void keembay_pcie_device_type(struct keembay_pcie *pcie)
{
	struct dw_pcie *pci = pcie->pci;
	u32 val;

	/* Bypass SRAM */
	val = keembay_pcie_readl(pcie, PCIE_REGS_PCIE_PHY_CNTL);
	val |= PCIE_REGS_PCIE_PHY_CNTL_PHY0_SRAM_BYPASS;
	keembay_pcie_writel(pcie, PCIE_REGS_PCIE_PHY_CNTL, val);

	/*
	 * PCIE_REGS_PCIE_CFG register bit 8
	 * pcie_device_type	0: Endpoint, 1: Root Complex
	 *
	 * Must be set before DEVICE_RSTN is removed.
	 * Clear all other bits in this register.
	 */
	switch (pcie->mode) {
	case DW_PCIE_RC_TYPE:
		keembay_pcie_writel(pcie, PCIE_REGS_PCIE_CFG, BIT(8));
		break;
	case DW_PCIE_EP_TYPE:
		/* ATF configure the EP, Linux kernel device driver won't */
		break;
	default:
		dev_err(pci->dev, "INVALID device type %d\n", pcie->mode);
	}
}

static void keembay_pcie_reset_deassert(struct keembay_pcie *pcie)
{
	u32 val;

	/* Subsystem power-on-reset -> set PCIE_REGS_PCIE_CFG[0] to 1 */
	val = keembay_pcie_readl(pcie, PCIE_REGS_PCIE_CFG) | BIT(0);
	keembay_pcie_writel(pcie, PCIE_REGS_PCIE_CFG, val);

	/* Release the warm reset -> set pcie_perst_n to HIGH */
	if (pcie->reset) {
		usleep_range(1000, 1500);
		gpiod_set_value_cansleep(pcie->reset, 1);
		usleep_range(1000, 1500);
	}
}

static void keembay_pcie_write_dbi(struct dw_pcie *pci, void __iomem *base,
				   u32 reg, size_t size, u32 val)
{
	struct keembay_pcie *keembay = to_keembay_pcie(pci);
	int ret;

	/* Disable dbi writes to BARs 2 and 4 if bar setup is being done */
	if (keembay->mode == DW_PCIE_EP_TYPE && keembay->setup_bar) {
		if ((reg == PCI_BASE_ADDRESS_2) || (reg == PCI_BASE_ADDRESS_4))
			return;
	}

	ret = dw_pcie_write(base + reg, size, val);
	if (ret)
		dev_err(pci->dev, "write DBI address failed\n");

}

static void keembay_pcie_write_dbi2(struct dw_pcie *pci, void __iomem *base,
				    u32 reg, size_t size, u32 val)
{
	struct keembay_pcie *keembay = to_keembay_pcie(pci);
	int ret;

	if (keembay->mode == DW_PCIE_EP_TYPE)
		return;

	ret = dw_pcie_write(base + reg, size, val);
	if (ret)
		dev_err(pci->dev, "write DBI address failed\n");
}

static int keembay_pcie_link_up(struct dw_pcie *pci)
{
	struct keembay_pcie *pcie = to_keembay_pcie(pci);
	u32 mask = BIT(19) | BIT (8);
	u32 val;

	/*
	 * Wait for link up,
	 * PCIE_REGS_PCIE_SII_PM_STATE register
	 * bit 19: smlh_link_up
	 * bit  8: rdlh_link_up
	 */
	val = keembay_pcie_readl(pcie, PCIE_REGS_PCIE_SII_PM_STATE);

	if ((val & mask) == mask)
	       return 1;

	dev_info(pci->dev,
		 "No link detected (PCIE_REGS_PCIE_SII_PM_STATE: 0x%08x).\n",
		 val);

	return 0;
}

static int keembay_pcie_establish_link(struct dw_pcie *pci)
{
	struct keembay_pcie *pcie = to_keembay_pcie(pci);
	u32 val;

	if (dw_pcie_link_up(pci)) {
		dev_info(pci->dev, "link is already up\n");
		return 0;
	}

	/* Clear PCIE_REGS_PCIE_APP_CNTRL[0] */
	val = keembay_pcie_readl(pcie, PCIE_REGS_PCIE_APP_CNTRL) &
		~PCIE_REGS_PCIE_APP_CNTRL_APP_LTSSM_EN;
	keembay_pcie_writel(pcie, PCIE_REGS_PCIE_APP_CNTRL, val);

	/* Poll bit 2 of PCIE_REGS_PCIE_PHY_STAT register */
	do {
		udelay(1);
		val = keembay_pcie_readl(pcie, PCIE_REGS_PCIE_PHY_STAT);
	} while (!(val & PCIE_REGS_PCIE_PHY_STAT_PHY0_MPLLB_STATE));

	dev_info(pci->dev, "MPLLB is powered up and phase-locked\n");

	/* TODO: Wait 100ms before starting link training? */
	msleep(100);

	/* Initiate link training */
	val = keembay_pcie_readl(pcie, PCIE_REGS_PCIE_APP_CNTRL) |
		PCIE_REGS_PCIE_APP_CNTRL_APP_LTSSM_EN;
	keembay_pcie_writel(pcie, PCIE_REGS_PCIE_APP_CNTRL, val);

	dev_info(pci->dev,
		 "Start link training (PCIE_REGS_PCIE_APP_CNTRL: 0x%08x).\n",
		 keembay_pcie_readl(pcie, PCIE_REGS_PCIE_APP_CNTRL));

	return dw_pcie_wait_for_link(pci);
}

static void keembay_pcie_stop_link(struct dw_pcie *pci)
{
	struct keembay_pcie *pcie = to_keembay_pcie(pci);
	u32 val;

	/* Disable link training */
	val = keembay_pcie_readl(pcie, PCIE_REGS_PCIE_APP_CNTRL) &
		~PCIE_REGS_PCIE_APP_CNTRL_APP_LTSSM_EN;
	keembay_pcie_writel(pcie, PCIE_REGS_PCIE_APP_CNTRL, val);

	dev_info(pci->dev,
		 "Stop link (PCIE_REGS_PCIE_APP_CNTRL: 0x%08x).\n",
		 keembay_pcie_readl(pcie, PCIE_REGS_PCIE_APP_CNTRL));
}

static const struct dw_pcie_ops keembay_pcie_dw_pcie_ops = {
	.link_up	= keembay_pcie_link_up,
	.start_link	= keembay_pcie_establish_link,
	.stop_link	= keembay_pcie_stop_link,
	.write_dbi	= keembay_pcie_write_dbi,
	.write_dbi2	= keembay_pcie_write_dbi2,
};

static int __init keembay_pcie_host_init(struct pcie_port *pp)
{
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);
	int ret;

	dw_pcie_setup_rc(pp);

	ret = keembay_pcie_establish_link(pci);
	if (ret)
		return ret;

	/* TODO: Initialize interrupts */
	if (IS_ENABLED(CONFIG_PCI_MSI))
		dw_pcie_msi_init(pp);

	/* TODO: Enable interrupts */

	return 0;
}

static const struct dw_pcie_host_ops keembay_pcie_host_ops = {
	.host_init	= keembay_pcie_host_init,
};

static void keembay_pcie_ep_init(struct dw_pcie_ep *ep)
{
	struct dw_pcie *pci = to_dw_pcie_from_ep(ep);
	struct keembay_pcie *keembay_pcie = to_keembay_pcie(pci);
	struct device *dev = pci->dev;
	struct device_node *np;
	static struct resource res2;
	static struct resource res4;
	int ret;
	u32 val;

	np = of_parse_phandle(pci->dev->of_node, "memory-region", 0);
	if (!np) {
		dev_err(dev, "No %s for BAR2 specified in device tree\n", "memory-region");
		return;
	}

	ret = of_address_to_resource(np, 0, &res2);
	if (ret) {
		dev_err(dev,"No memory address assigned to the region\n");
		return;
	}

	keembay_pcie->mmr2 = &res2;

	np = of_parse_phandle(pci->dev->of_node, "memory-region", 1);
	if (!np) {
		dev_err(dev,"No %s for BAR4 specified in device tree\n", "memory-region");
		return;
	}

	ret = of_address_to_resource(np, 0, &res4);
	if (ret) {
		dev_err(dev,"No memory address assigned to the region\n");
		return;
	}

	keembay_pcie->mmr4 = &res4;
	keembay_pcie->setup_bar = false;

	/* Write all Fs to the mask register */
	keembay_pcie_writel(keembay_pcie, PCIE_REGS_MEM_ACCESS_IRQ_ENABLE, 0xFFFFFFFF);

	/* Enable the CII event interrupt */
	val = keembay_pcie_readl(keembay_pcie, PCIE_REGS_PCIE_INTR_ENABLE);
	val |= PCIE_REGS_PCIE_INTR_ENABLE_CII;
	keembay_pcie_writel(keembay_pcie, PCIE_REGS_PCIE_INTR_ENABLE, val);

	/* Retrieve and register interrupt from DT */
	keembay_pcie->irq = irq_of_parse_and_map(pci->dev->of_node, 1);
}

static int keembay_pcie_raise_irq(struct dw_pcie_ep *ep, u8 func_no,
				  enum pci_epc_irq_type type,
				  u16 interrupt_num)
{
	struct dw_pcie *pci = to_dw_pcie_from_ep(ep);
	//struct keembay_pcie *keembay = to_keembay_pcie(pci);

	switch (type) {
	/* Keem Bay does not support legacy IRQ */
	case PCI_EPC_IRQ_LEGACY:
		dev_err(pci->dev, "UNSUPPORTED IRQ TYPE\n");
		return -EINVAL;
		//return dw_pcie_ep_raise_legacy_irq(ep, func_no);
	case PCI_EPC_IRQ_MSI:
		return dw_pcie_ep_raise_msi_irq(ep, func_no, interrupt_num);
	case PCI_EPC_IRQ_MSIX:
		/* Keem Bay does not has use case for MSIX */
		return dw_pcie_ep_raise_msix_irq(ep, func_no, interrupt_num);
	default:
		dev_err(pci->dev, "UNKNOWN IRQ TYPE\n");
		return -EINVAL;
	}
}

static const struct pci_epc_features keembay_pcie_epc_features = {
	.linkup_notifier	= false,
	.msi_capable		= true,
	.msix_capable		= true,
	.reserved_bar		= 1 << BAR_1 | 1 << BAR_3 | 1 << BAR_5,
	.bar_fixed_64bit	= 1 << BAR_0 | 1 << BAR_2 | 1 << BAR_4,
	.bar_fixed_size[0]	= SZ_1M,
	.bar_fixed_size[2]	= SZ_16M,
	.bar_fixed_size[4]	= SZ_512M,
	.align			= SZ_1M,
};

static const struct pci_epc_features*
keembay_pcie_get_features(struct dw_pcie_ep *ep)
{
	return &keembay_pcie_epc_features;
}

static const struct dw_pcie_ep_ops keembay_pcie_ep_ops = {
	.ep_init	= keembay_pcie_ep_init,
	.raise_irq	= keembay_pcie_raise_irq,
	.get_features	= keembay_pcie_get_features,
};

static int __init keembay_pcie_add_pcie_ep(struct keembay_pcie *keembay,
					   struct platform_device *pdev)
{
	struct dw_pcie_ep *ep;
	struct device *dev = &pdev->dev;
	struct dw_pcie *pci = keembay->pci;
	struct resource *res;
	int ret;

	ep = &pci->ep;
	ep->ops = &keembay_pcie_ep_ops;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "addr_space");
	if (!res) {
		return -EINVAL;
	}

	ep->phys_base = res->start;
	ep->addr_size = resource_size(res);

	ret = dw_pcie_ep_init(ep);
	if (ret) {
		dev_err(dev, "failed to initialize endpoint\n");
		return ret;
	}

	return 0;
}

static int __init keembay_pcie_add_pcie_port(struct keembay_pcie *pcie,
					     struct platform_device *pdev)
{
	struct dw_pcie *pci = pcie->pci;
	struct pcie_port *pp = &pci->pp;
	struct device *dev = &pdev->dev;
	int ret;

	pp->ops = &keembay_pcie_host_ops;

	pcie->clk_master = devm_clk_get(dev, "master");
	if (IS_ERR(pcie->clk_master)) {
		dev_err(dev, "failed to get master clock\n");
		return PTR_ERR(pcie->clk_master);
	}

	pcie->clk_aux = devm_clk_get(dev, "aux");
	if (IS_ERR(pcie->clk_aux)) {
		dev_err(dev, "failed to get auxiliary clock\n");
		return PTR_ERR(pcie->clk_aux);
	}

	ret = clk_prepare_enable(pcie->clk_master);
	if (ret) {
		dev_err(dev, "failed to enable master clock: %d\n", ret);
		return ret;
	}

	ret = clk_set_rate(pcie->clk_aux, 24000000);

	ret = clk_prepare_enable(pcie->clk_aux);
	if (ret) {
		dev_err(dev, "failed to enable auxiliary clock: %d\n", ret);
		goto fail_clk_enable;
	}

	/* TO BE REMOVED */
	pr_info("%s: master clock rate= %lu, aux clock rate= %lu\n",
		__func__,
		clk_get_rate(pcie->clk_master),
		clk_get_rate(pcie->clk_aux));

	pp->irq = platform_get_irq(pdev, 1);
	if (pp->irq < 0) {
		dev_err(dev, "failed to get IRQ: %d\n", pp->irq);
		/* TODO: get irq */
		//return pp->irq;
	}

	if (IS_ENABLED(CONFIG_PCI_MSI)) {
		pp->msi_irq = platform_get_irq(pdev, 0);
		if (pp->msi_irq < 0) {
			dev_err(dev, "failed to get msi IRQ: %d\n",
				pp->msi_irq);
			/* TODO: get irq */
			//return pp->msi_irq;
		}
	}

	keembay_pcie_device_type(pcie);
	keembay_pcie_pll_init(pcie);
	keembay_pcie_reset_deassert(pcie);

	ret = dw_pcie_host_init(pp);
	if (ret) {
		dev_err(dev, "failed to initialize host: %d\n", ret);

		if (pcie->reset) {
			gpiod_set_value_cansleep(pcie->reset, 0);
			usleep_range(1000, 1500);
		}

		return ret;
	}

	return 0;

fail_clk_enable:
	clk_disable_unprepare(pcie->clk_master);

	return ret;
}

static const struct keembay_pcie_of_data keembay_pcie_rc_of_data = {
	.host_ops = &keembay_pcie_host_ops,
	.mode = DW_PCIE_RC_TYPE,
};

static const struct keembay_pcie_of_data keembay_pcie_ep_of_data = {
	.ep_ops = &keembay_pcie_ep_ops,
	.mode = DW_PCIE_EP_TYPE,
};

static const struct of_device_id keembay_pcie_of_match[] = {
	{
		.compatible = "intel,keembay-pcie",
		.data = &keembay_pcie_rc_of_data,
	},
	{
		.compatible = "intel,keembay-pcie-ep",
		.data = &keembay_pcie_ep_of_data,
	},
	{ },
};

static int __init keembay_pcie_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct keembay_pcie *pcie;
	struct dw_pcie *pci;
	struct resource *res;
	const struct keembay_pcie_of_data *data;
	const struct of_device_id *match;
	enum dw_pcie_device_mode mode;
	int ret;

	match = of_match_device(of_match_ptr(keembay_pcie_of_match), dev);
	if (!match)
		return -EINVAL;

	data = (struct keembay_pcie_of_data *)match->data;
	mode = data->mode;

	pcie = devm_kzalloc(dev, sizeof(*pcie), GFP_KERNEL);
	if (!pcie)
		return -ENOMEM;

	pci = devm_kzalloc(dev, sizeof(*pci), GFP_KERNEL);
	if (!pci)
		return -ENOMEM;

	pci->dev = dev;
	pci->ops = &keembay_pcie_dw_pcie_ops;

	pcie->pci = pci;
	pcie->mode = mode;

	pcie->reset = devm_gpiod_get_optional(dev, "perst", GPIOD_OUT_LOW);
	if (IS_ERR(pcie->reset))
		return PTR_ERR(pcie->reset);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "apb");
	pcie->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(pcie->base))
		return PTR_ERR(pcie->base);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "dbi");
	pci->dbi_base = devm_ioremap_resource(dev, res);
	if (IS_ERR(pci->dbi_base))
		return PTR_ERR(pci->dbi_base);

	pci->dbi_base2 = pci->dbi_base;
	pci->atu_base = pci->dbi_base + 0x300000;

	if (!pci->dbi_base || !pci->dbi_base2) {
		dev_err(dev, "dbi_base/dbi_base2 is not populated\n");
		return -EINVAL;
	}

	platform_set_drvdata(pdev, pcie);

	switch (pcie->mode) {
	case DW_PCIE_RC_TYPE:
		if (!IS_ENABLED(CONFIG_PCIE_KEEMBAY_HOST))
			return -ENODEV;

		ret = keembay_pcie_add_pcie_port(pcie, pdev);
		if (ret < 0)
			return ret;
		break;
	case DW_PCIE_EP_TYPE:
		if (!IS_ENABLED(CONFIG_PCIE_KEEMBAY_EP))
			return -ENODEV;

		ret = keembay_pcie_add_pcie_ep(pcie, pdev);
		if (ret < 0)
			return ret;
		break;
	default:
		dev_err(dev, "INVALID device type %d\n", pcie->mode);
	}

	return 0;
}

static struct platform_driver keembay_pcie_driver __refdata = {
	.probe  = keembay_pcie_probe,
	.driver = {
		.name = "keembay-pcie",
		.of_match_table = keembay_pcie_of_match,
	},
};
builtin_platform_driver(keembay_pcie_driver);
