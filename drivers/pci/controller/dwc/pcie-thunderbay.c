// SPDX-License-Identifier: GPL-2.0-only
/*
 * pcie-thunderbay - PCIe controller driver for Intel Thunder Bay
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
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/of_reserved_mem.h>

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
#define  PCIE_REGS_PCIE_INTR_ENABLE_CII			BIT(18)
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

#define to_thunderbay_pcie(x)	dev_get_drvdata((x)->dev)

#define PCIE_DBI2_MASK		(1 << 20)
#define PERST_DELAY_US		1000

struct thunderbay_pcie {
	struct dw_pcie		*pci;
	void __iomem		*base;
	enum dw_pcie_device_mode mode;
	struct resource		*mmr2[8];
	struct resource		*mmr4[8];
	bool			setup_bar[8];
	int			irq;

	struct clk		*clk_master;
	struct clk		*clk_aux;
	struct gpio_desc	*reset;
};

struct thunderbay_pcie_of_data {
	enum dw_pcie_device_mode mode;
	const struct dw_pcie_host_ops *host_ops;
	const struct dw_pcie_ep_ops *ep_ops;
};

static inline u32 thunderbay_pcie_readl(struct thunderbay_pcie *pcie, u32 offset)
{
	return readl(pcie->base + offset);
}

static inline void thunderbay_pcie_writel(struct thunderbay_pcie *pcie, u32 offset,
				       u32 value)
{
	writel(value, pcie->base + offset);
}

static void thunderbay_ep_reset_assert(struct thunderbay_pcie *pcie)
{
	/* Toggle the perst GPIO pin to LOW */
	gpiod_set_value_cansleep(pcie->reset, 1);
	usleep_range(PERST_DELAY_US, PERST_DELAY_US + 500);
}

static void thunderbay_ep_reset_deassert(struct thunderbay_pcie *pcie)
{
	/* Ensure that PERST has been asserted for at least 100ms */
	msleep(100);

	/* Toggle the perst GPIO pin to HIGH */
	gpiod_set_value_cansleep(pcie->reset, 0);
	usleep_range(PERST_DELAY_US, PERST_DELAY_US + 500);
}

static void thunderbay_pcie_ltssm_enable(struct thunderbay_pcie *pcie, bool enable)
{
	struct dw_pcie *pci = pcie->pci;
	u32 val;

	/*
	 * PCIE_REGS_PCIE_APP_CNTRL register at offset 0x8
	 * bit  0: app_ltssm_enable
	 */
	val = thunderbay_pcie_readl(pcie, 0);
	if (enable)
		val |= BIT(4);
	else
		val &= ~BIT(4);
	thunderbay_pcie_writel(pcie, 0, val);

	dev_dbg(pci->dev, "PCIE_REGS_PCIE_APP_CNTRL: 0x%08x\n",
		thunderbay_pcie_readl(pcie, 0));
}

static void thunderbay_pcie_sram_bypass_mode(struct thunderbay_pcie *pcie)
{
	struct dw_pcie *pci = pcie->pci;
	u32 val;

	/*
	 * PCIE_REGS_PCIE_PHY_CNTL register at offset 0x164
	 * bit  8: phy0_sram_bypass
	 *
	 * Set SRAM bypass mode.
	 */
	val = thunderbay_pcie_readl(pcie, 0x48) | BIT(30);
	thunderbay_pcie_writel(pcie, 0x48, val);
	dev_dbg(pci->dev, "PCIE_REGS_PCIE_PHY_CNTL: 0x%08x\n",
		thunderbay_pcie_readl(pcie, 0x48));
}

/* Initialize the PCIe PLL in Host mode (assume 24MHz refclk) */
static void thunderbay_pcie_pll_init(struct thunderbay_pcie *pcie)
{
	struct dw_pcie *pci = pcie->pci;
	u32 val;
#if 0
	val = (0x0 << 12)	// [17:12] ljlpp_ref_div
		| (0x32 << 0);	// [11: 0] ljpll_fb_div
	thunderbay_pcie_writel(pcie, PCIE_REGS_LJPLL_CNTRL_2, val);
	dev_dbg(pci->dev, "PCIE_REGS_LJPLL_CNTRL_2: 0x%08x\n",
		thunderbay_pcie_readl(pcie, PCIE_REGS_LJPLL_CNTRL_2));

	val = (0x2 << 22)	// [24:22] ljpll_post_div3a
		| (0x2 << 16);	// [18:16] ljpll_post_div2a
	thunderbay_pcie_writel(pcie, PCIE_REGS_LJPLL_CNTRL_3, val);
	dev_dbg(pci->dev, "PCIE_REGS_LJPLL_CNTRL_3: 0x%08x\n",
		thunderbay_pcie_readl(pcie, PCIE_REGS_LJPLL_CNTRL_3));

	val = (0x1 << 29)	// [  :29] ljpll_en
		| (0xc << 21);	// [24:21] ljpll_fout_en
	thunderbay_pcie_writel(pcie, PCIE_REGS_LJPLL_CNTRL_0, val);
	dev_dbg(pci->dev, "PCIE_REGS_LJPLL_CNTRL_0: 0x%08x\n",
		thunderbay_pcie_readl(pcie, PCIE_REGS_LJPLL_CNTRL_0));

	/*
	 * PCIE_REGS_LJPLL_STA register at offset 0x16c
	 * bit  0: ljpll_lock
	 *
	 * Poll bit 0 until set.
	 */
	do {
		udelay(1);
		val = thunderbay_pcie_readl(pcie, PCIE_REGS_LJPLL_STA);
	} while ((val & BIT(0)) != BIT(0));
	dev_dbg(pci->dev, "PCIE_REGS_LJPLL_STA: 0x%08x\n", val);
	dev_info(pci->dev, "PCIe low jitter PLL locked\n");
#endif
}

static void thunderbay_pcie_device_type(struct thunderbay_pcie *pcie)
{
	struct dw_pcie *pci = pcie->pci;

	/*
	 * PCIE_REGS_PCIE_CFG register at offset 0x4
	 * bit  8: pcie_device_type
	 * where 0: Endpoint, 1: Root Complex
	 *
	 * Must be set before DEVICE_RSTN is removed.
	 * Clear all other bits in this register.
	 */
	switch (pcie->mode) {
	case DW_PCIE_RC_TYPE:
		thunderbay_pcie_writel(pcie, PCIE_REGS_PCIE_CFG, BIT(8));
		break;
	case DW_PCIE_EP_TYPE:
		/*
		 * Keem Bay TF-A will configure the EP,
		 * Linux kernel device driver won't
		 */
		break;
	default:
		dev_err(pci->dev, "INVALID device type %d\n", pcie->mode);
	}

	dev_info(pci->dev, "PCIE_REGS_PCIE_CFG: 0x%08x\n",
		 thunderbay_pcie_readl(pcie, PCIE_REGS_PCIE_CFG));

}

static void thunderbay_pcie_reset_deassert(struct thunderbay_pcie *pcie)
{
	struct dw_pcie *pci = pcie->pci;
	u32 val;

	/*
	 * PCIE_REGS_PCIE_CFG register at offset 0x4
	 * bit  0: pcie_rstn
	 * currently include PMA reset.
	 *
	 * Subsystem power-on-reset by setting bit 0 to 1
	 */
	val = thunderbay_pcie_readl(pcie, PCIE_REGS_PCIE_CFG) | BIT(0);
	thunderbay_pcie_writel(pcie, PCIE_REGS_PCIE_CFG, val);
	dev_dbg(pci->dev, "PCIE_REGS_PCIE_CFG: 0x%08x\n",
	                 thunderbay_pcie_readl(pcie, PCIE_REGS_PCIE_CFG));
}

static void thunderbay_pcie_write_dbi(struct dw_pcie *pci, void __iomem *base,
				   u32 reg, size_t size, u32 val)
{
	struct thunderbay_pcie *thunderbay = to_thunderbay_pcie(pci);
	int ret;

	/* Disable dbi writes to BARs 2 and 4 if bar setup is being done */
	if (thunderbay->mode == DW_PCIE_EP_TYPE && (thunderbay->setup_bar[0] ||
							thunderbay->setup_bar[1] || 
							thunderbay->setup_bar[2] || 
							thunderbay->setup_bar[3] || 
							thunderbay->setup_bar[4] || 
							thunderbay->setup_bar[5] || 
							thunderbay->setup_bar[6] || 
							thunderbay->setup_bar[7] )) 
	{
		if ((reg == PCI_BASE_ADDRESS_2) || (reg == PCI_BASE_ADDRESS_4))
			return;
	}

        dev_dbg(pci->dev, "thunderbay_pcie_write_dbi():base+reg=%x,val=%x\n",base+reg, val);
	ret = dw_pcie_write(base + reg, size, val);
	if (ret)
		dev_err(pci->dev, "write DBI address failed\n");

}

static void thunderbay_pcie_write_dbi2(struct dw_pcie *pci, void __iomem *base,
				    u32 reg, size_t size, u32 val)
{
	struct thunderbay_pcie *thunderbay = to_thunderbay_pcie(pci);
	int ret;

	if (thunderbay->mode == DW_PCIE_EP_TYPE)
		return;

        dev_dbg(pci->dev,"thunderbay_pcie_write_dbi2():base+reg=%x,val=%x\n",base+reg, val);
	ret = dw_pcie_write(base + reg, size, val);
	if (ret)
		dev_err(pci->dev, "write DBI address failed\n");
}

static int thunderbay_pcie_link_up(struct dw_pcie *pci)
{
	struct thunderbay_pcie *pcie = to_thunderbay_pcie(pci);
	u32 mask = BIT(10) | BIT (0);
	u32 val;

	/*
	 * Wait for link up,
	 * PCIE_REGS_PCIE_SII_PM_STATE register
	 * bit 19: smlh_link_up
	 * bit  8: rdlh_link_up
	 */
	val = thunderbay_pcie_readl(pcie, 0x1000);

	if ((val & mask) == mask)
	       return 1;

	dev_dbg(pci->dev,
		"No link detected (PCIE_REGS_PCIE_SII_PM_STATE: 0x%08x).\n",
		val);

	return 0;
}

static int thunderbay_pcie_establish_link(struct dw_pcie *pci)
{
	return 0;
}

static void thunderbay_pcie_stop_link(struct dw_pcie *pci)
{
	struct thunderbay_pcie *pcie = to_thunderbay_pcie(pci);

	thunderbay_pcie_ltssm_enable(pcie, false);

	dev_dbg(pci->dev, "Stop link (PCIE_REGS_PCIE_APP_CNTRL: 0x%08x).\n",
		thunderbay_pcie_readl(pcie, 0));
}

static const struct dw_pcie_ops thunderbay_pcie_dw_pcie_ops = {
	.link_up	= thunderbay_pcie_link_up,
	.start_link	= thunderbay_pcie_establish_link,
	.stop_link	= thunderbay_pcie_stop_link,
	.write_dbi	= thunderbay_pcie_write_dbi,
	.write_dbi2	= thunderbay_pcie_write_dbi2,
};

static void thunderbay_pcie_rc_establish_link(struct dw_pcie *pci)
{
	struct thunderbay_pcie *pcie = to_thunderbay_pcie(pci);
	u32 val;

	if (dw_pcie_link_up(pci)) {
		dev_info(pci->dev, "link is already up\n");
		return;
	}

	thunderbay_pcie_ltssm_enable(pcie, false);

	/*
	 * PCIE_REGS_PCIE_PHY_STAT register at offset 0x168
	 * bit  2: phy0_mpllb_state
	 * bit  1: phy0_mplla_state
	 *
	 * Poll bit 1 until set.
	 */
	do {
		udelay(1);
		val = thunderbay_pcie_readl(pcie, PCIE_REGS_PCIE_PHY_STAT);
	} while ((val & BIT(1)) != BIT(1));
	dev_dbg(pci->dev, "PCIE_REGS_PCIE_PHY_STAT:  0x%08x\n", val);
	dev_info(pci->dev, "MPLLA is powered up and phase-locked\n");

	thunderbay_pcie_ltssm_enable(pcie, true);
	dw_pcie_wait_for_link(pci);
}

static int __init thunderbay_pcie_host_init(struct pcie_port *pp)
{
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);

	dw_pcie_setup_rc(pp);
	thunderbay_pcie_rc_establish_link(pci);

	/* TODO: Initialize interrupts */
	if (IS_ENABLED(CONFIG_PCI_MSI))
		dw_pcie_msi_init(pp);

	/* TODO: Enable interrupts */

	return 0;
}

static const struct dw_pcie_host_ops thunderbay_pcie_host_ops = {
	.host_init	= thunderbay_pcie_host_init,
};

static void thunderbay_pcie_ep_init(struct dw_pcie_ep *ep)
{
	struct dw_pcie *pci = to_dw_pcie_from_ep(ep);
	struct thunderbay_pcie *thunderbay_pcie = to_thunderbay_pcie(pci);
	struct device *dev = pci->dev;
	struct device_node *np;
	static struct resource f0res2;
	static struct resource f0res4;
	static struct resource f1res2;
	static struct resource f1res4;
	static struct resource f2res2;
	static struct resource f2res4;
	static struct resource f3res2;
	static struct resource f3res4;
	static struct resource f4res2;
	static struct resource f4res4;
	static struct resource f5res2;
	static struct resource f5res4;
	static struct resource f6res2;
	static struct resource f6res4;
	static struct resource f7res2;
	static struct resource f7res4;
	int ret;
	u32 val;

	/* function 0 */
	np = of_parse_phandle(pci->dev->of_node, "memory-region", 0);
	if (!np) {
		dev_err(dev, "No %s for BAR2 specified in device tree\n", "memory-region");
		return;
	}

	ret = of_address_to_resource(np, 0, &f0res2);
	if (ret) {
		dev_err(dev,"No memory address assigned to the region\n");
		return;
	}

	thunderbay_pcie->mmr2[0] = &f0res2;
//	printk("thunderbay->mmr2[0]->start=%llx\n",thunderbay_pcie->mmr2[0]->start);

	np = of_parse_phandle(pci->dev->of_node, "memory-region", 1);
	if (!np) {
		dev_err(dev,"No %s for BAR4 specified in device tree\n", "memory-region");
		return;
	}

	ret = of_address_to_resource(np, 0, &f0res4);
	if (ret) {
		dev_err(dev,"No memory address assigned to the region\n");
		return;
	}

	thunderbay_pcie->mmr4[0] = &f0res4;
//	printk("thunderbay->mmr4[0]->start=%llx\n",thunderbay_pcie->mmr4[0]->start);
	thunderbay_pcie->setup_bar[0] = false;



	/* function 1 */

	np = of_parse_phandle(pci->dev->of_node, "memory-region", 2);
	if (!np) {
		dev_err(dev, "No %s for BAR2 specified in device tree\n", "memory-region");
		return;
	}

	ret = of_address_to_resource(np, 0, &f1res2);
	if (ret) {
		dev_err(dev,"No memory address assigned to the region\n");
		return;
	}

	thunderbay_pcie->mmr2[1] = &f1res2;
//	printk("thunderbay->mmr2[1]->start=%llx\n",thunderbay_pcie->mmr2[1]->start);

	np = of_parse_phandle(pci->dev->of_node, "memory-region", 3);
	if (!np) {
		dev_err(dev,"No %s for BAR4 specified in device tree\n", "memory-region");
		return;
	}

	ret = of_address_to_resource(np, 0, &f1res4);
	if (ret) {
		dev_err(dev,"No memory address assigned to the region\n");
		return;
	}

	thunderbay_pcie->mmr4[1] = &f1res4;
//	printk("thunderbay->mmr4[1]->start=%llx\n",thunderbay_pcie->mmr4[1]->start);
	thunderbay_pcie->setup_bar[1] = false;


	/* function 2 */

	np = of_parse_phandle(pci->dev->of_node, "memory-region", 4);
	if (!np) {
		dev_err(dev, "No %s for BAR2 specified in device tree\n", "memory-region");
		return;
	}

	ret = of_address_to_resource(np, 0, &f2res2);
	if (ret) {
		dev_err(dev,"No memory address assigned to the region\n");
		return;
	}

	thunderbay_pcie->mmr2[2] = &f2res2;
//	printk("thunderbay->mmr2[2]->start=%llx\n",thunderbay_pcie->mmr2[2]->start);

	np = of_parse_phandle(pci->dev->of_node, "memory-region", 5);
	if (!np) {
		dev_err(dev,"No %s for BAR4 specified in device tree\n", "memory-region");
		return;
	}

	ret = of_address_to_resource(np, 0, &f2res4);
	if (ret) {
		dev_err(dev,"No memory address assigned to the region\n");
		return;
	}

	thunderbay_pcie->mmr4[2] = &f2res4;
//	printk("thunderbay->mmr4[2]->start=%llx\n",thunderbay_pcie->mmr4[2]->start);
	thunderbay_pcie->setup_bar[2] = false;


	/* function 3 */

	np = of_parse_phandle(pci->dev->of_node, "memory-region", 6);
	if (!np) {
		dev_err(dev, "No %s for BAR2 specified in device tree\n", "memory-region");
		return;
	}

	ret = of_address_to_resource(np, 0, &f3res2);
	if (ret) {
		dev_err(dev,"No memory address assigned to the region\n");
		return;
	}

	thunderbay_pcie->mmr2[3] = &f3res2;
//	printk("thunderbay->mmr2[3]->start=%llx\n",thunderbay_pcie->mmr2[3]->start);

	np = of_parse_phandle(pci->dev->of_node, "memory-region", 7);
	if (!np) {
		dev_err(dev,"No %s for BAR4 specified in device tree\n", "memory-region");
		return;
	}

	ret = of_address_to_resource(np, 0, &f3res4);
	if (ret) {
		dev_err(dev,"No memory address assigned to the region\n");
		return;
	}

	thunderbay_pcie->mmr4[3] = &f3res4;
//	printk("thunderbay->mmr4[3]->start=%llx\n",thunderbay_pcie->mmr4[3]->start);
	thunderbay_pcie->setup_bar[3] = false;


	/* function 4 */
	np = of_parse_phandle(pci->dev->of_node, "memory-region", 8);
	if (!np) {
		dev_err(dev, "No %s for BAR2 specified in device tree\n", "memory-region");
		return;
	}

	ret = of_address_to_resource(np, 0, &f4res2);
	if (ret) {
		dev_err(dev,"No memory address assigned to the region\n");
		return;
	}

	thunderbay_pcie->mmr2[4] = &f4res2;
//	printk("thunderbay->mmr2[4]->start=%llx\n",thunderbay_pcie->mmr2[4]->start);

	np = of_parse_phandle(pci->dev->of_node, "memory-region", 9);
	if (!np) {
		dev_err(dev,"No %s for BAR4 specified in device tree\n", "memory-region");
		return;
	}

	ret = of_address_to_resource(np, 0, &f4res4);
	if (ret) {
		dev_err(dev,"No memory address assigned to the region\n");
		return;
	}

	thunderbay_pcie->mmr4[4] = &f4res4;
//	printk("thunderbay->mmr4[4]->start=%llx\n",thunderbay_pcie->mmr4[3]->start);
	thunderbay_pcie->setup_bar[4] = false;

	/* function 5 */
	np = of_parse_phandle(pci->dev->of_node, "memory-region", 10);
	if (!np) {
		dev_err(dev, "No %s for BAR2 specified in device tree\n", "memory-region");
		return;
	}

	ret = of_address_to_resource(np, 0, &f5res2);
	if (ret) {
		dev_err(dev,"No memory address assigned to the region\n");
		return;
	}

	thunderbay_pcie->mmr2[5] = &f5res2;
//	printk("thunderbay->mmr2[5]->start=%llx\n",thunderbay_pcie->mmr2[5]->start);

	np = of_parse_phandle(pci->dev->of_node, "memory-region", 11);
	if (!np) {
		dev_err(dev,"No %s for BAR4 specified in device tree\n", "memory-region");
		return;
	}

	ret = of_address_to_resource(np, 0, &f5res4);
	if (ret) {
		dev_err(dev,"No memory address assigned to the region\n");
		return;
	}

	thunderbay_pcie->mmr4[5] = &f5res4;
//	printk("thunderbay->mmr4[5]->start=%llx\n",thunderbay_pcie->mmr4[5]->start);
	thunderbay_pcie->setup_bar[5] = false;

	/* function 6 */
	np = of_parse_phandle(pci->dev->of_node, "memory-region", 12);
	if (!np) {
		dev_err(dev, "No %s for BAR2 specified in device tree\n", "memory-region");
		return;
	}

	ret = of_address_to_resource(np, 0, &f6res2);
	if (ret) {
		dev_err(dev,"No memory address assigned to the region\n");
		return;
	}

	thunderbay_pcie->mmr2[6] = &f6res2;
//	printk("thunderbay->mmr2[6]->start=%llx\n",thunderbay_pcie->mmr2[6]->start);

	np = of_parse_phandle(pci->dev->of_node, "memory-region", 13);
	if (!np) {
		dev_err(dev,"No %s for BAR4 specified in device tree\n", "memory-region");
		return;
	}

	ret = of_address_to_resource(np, 0, &f6res4);
	if (ret) {
		dev_err(dev,"No memory address assigned to the region\n");
		return;
	}

	thunderbay_pcie->mmr4[6] = &f6res4;
//	printk("thunderbay->mmr4[6]->start=%llx\n",thunderbay_pcie->mmr4[6]->start);
	thunderbay_pcie->setup_bar[6] = false;

	/* function 7 */
	np = of_parse_phandle(pci->dev->of_node, "memory-region", 14);
	if (!np) {
		dev_err(dev, "No %s for BAR2 specified in device tree\n", "memory-region");
		return;
	}

	ret = of_address_to_resource(np, 0, &f7res2);
	if (ret) {
		dev_err(dev,"No memory address assigned to the region\n");
		return;
	}

	thunderbay_pcie->mmr2[7] = &f7res2;
//	printk("thunderbay->mmr2[7]->start=%llx\n",thunderbay_pcie->mmr2[7]->start);

	np = of_parse_phandle(pci->dev->of_node, "memory-region", 15);
	if (!np) {
		dev_err(dev,"No %s for BAR4 specified in device tree\n", "memory-region");
		return;
	}

	ret = of_address_to_resource(np, 0, &f7res4);
	if (ret) {
		dev_err(dev,"No memory address assigned to the region\n");
		return;
	}

	thunderbay_pcie->mmr4[7] = &f7res4;
//	printk("thunderbay->mmr4[7]->start=%llx\n",thunderbay_pcie->mmr4[7]->start);
	thunderbay_pcie->setup_bar[7] = false;
#if 0
	/* Write all Fs to the mask register */
	thunderbay_pcie_writel(thunderbay_pcie, PCIE_REGS_MEM_ACCESS_IRQ_ENABLE, 0xFFFFFFFF);

	/* Enable the CII event interrupt */
	val = thunderbay_pcie_readl(thunderbay_pcie, PCIE_REGS_PCIE_INTR_ENABLE);
	val |= PCIE_REGS_PCIE_INTR_ENABLE_CII;
	thunderbay_pcie_writel(thunderbay_pcie, PCIE_REGS_PCIE_INTR_ENABLE, val);
#endif

	/* Retrieve and register interrupt from DT */
	thunderbay_pcie->irq = irq_of_parse_and_map(pci->dev->of_node, 1);
}

static int thunderbay_pcie_raise_irq(struct dw_pcie_ep *ep, u8 func_no,
				  enum pci_epc_irq_type type,
				  u16 interrupt_num)
{
	struct dw_pcie *pci = to_dw_pcie_from_ep(ep);
	//struct thunderbay_pcie *thunderbay = to_thunderbay_pcie(pci);

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

static const struct pci_epc_features thunderbay_pcie_epc_features = {
	.linkup_notifier	= false,
	.msi_capable		= true,
	.msix_capable		= false,
	.reserved_bar		= 1 << BAR_1 | 1 << BAR_3 | 1 << BAR_5,
	.bar_fixed_64bit	= 1 << BAR_0 | 1 << BAR_2 | 1 << BAR_4,
	.bar_fixed_size[0]	= SZ_1M,
	.bar_fixed_size[2]	= SZ_16M,
	.bar_fixed_size[4]	= SZ_8M,
	.align			= SZ_1M,
};

static const struct pci_epc_features*
thunderbay_pcie_get_features(struct dw_pcie_ep *ep)
{
	return &thunderbay_pcie_epc_features;
}

static const struct dw_pcie_ep_ops thunderbay_pcie_ep_ops = {
	.ep_init	= thunderbay_pcie_ep_init,
	.raise_irq	= thunderbay_pcie_raise_irq,
	.get_features	= thunderbay_pcie_get_features,
};

static int __init thunderbay_pcie_add_pcie_ep(struct thunderbay_pcie *thunderbay,
					   struct platform_device *pdev)
{
	struct dw_pcie_ep *ep;
	struct device *dev = &pdev->dev;
	struct dw_pcie *pci = thunderbay->pci;
	struct resource *res;
	int ret;

	ep = &pci->ep;
	ep->ops = &thunderbay_pcie_ep_ops;

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

static int __init thunderbay_pcie_add_pcie_port(struct thunderbay_pcie *pcie,
					     struct platform_device *pdev)
{
	struct dw_pcie *pci = pcie->pci;
	struct pcie_port *pp = &pci->pp;
	struct device *dev = &pdev->dev;
	int ret;

	pp->ops = &thunderbay_pcie_host_ops;

	pcie->reset = devm_gpiod_get(dev, "perst", GPIOD_OUT_HIGH);
	if (IS_ERR(pcie->reset))
		return PTR_ERR(pcie->reset);

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
		return pp->irq;
	}

	if (IS_ENABLED(CONFIG_PCI_MSI)) {
		pp->msi_irq = platform_get_irq(pdev, 0);
		if (pp->msi_irq < 0) {
			dev_err(dev, "failed to get msi IRQ: %d\n",
				pp->msi_irq);
			/* TODO: get irq */
			return pp->msi_irq;
		}
	}

	thunderbay_pcie_sram_bypass_mode(pcie);
	thunderbay_pcie_device_type(pcie);
	thunderbay_pcie_pll_init(pcie);
	thunderbay_pcie_reset_deassert(pcie);
	thunderbay_ep_reset_deassert(pcie);

	ret = dw_pcie_host_init(pp);
	if (ret) {
		dev_err(dev, "failed to initialize host: %d\n", ret);
		thunderbay_ep_reset_assert(pcie);
		goto fail_clk_enable;
	}

	return 0;

fail_clk_enable:
	clk_disable_unprepare(pcie->clk_master);

	return ret;
}

static const struct thunderbay_pcie_of_data thunderbay_pcie_rc_of_data = {
	.host_ops = &thunderbay_pcie_host_ops,
	.mode = DW_PCIE_RC_TYPE,
};

static const struct thunderbay_pcie_of_data thunderbay_pcie_ep_of_data = {
	.ep_ops = &thunderbay_pcie_ep_ops,
	.mode = DW_PCIE_EP_TYPE,
};

static const struct of_device_id thunderbay_pcie_of_match[] = {
	{
		.compatible = "intel,thunderbay-pcie",
		.data = &thunderbay_pcie_rc_of_data,
	},
	{
		.compatible = "intel,thunderbay-pcie-ep",
		.data = &thunderbay_pcie_ep_of_data,
	},
	{ },
};

static int __init thunderbay_pcie_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct thunderbay_pcie *pcie;
	struct dw_pcie *pci;
	struct resource *res;
	const struct thunderbay_pcie_of_data *data;
	const struct of_device_id *match;
	enum dw_pcie_device_mode mode;
	int ret;

	match = of_match_device(of_match_ptr(thunderbay_pcie_of_match), dev);
	if (!match)
		return -EINVAL;

	dev_info(dev, "K0:\n");
	data = (struct thunderbay_pcie_of_data *)match->data;
	mode = data->mode;

	pcie = devm_kzalloc(dev, sizeof(*pcie), GFP_KERNEL);
	if (!pcie)
		return -ENOMEM;

	pci = devm_kzalloc(dev, sizeof(*pci), GFP_KERNEL);
	if (!pci)
		return -ENOMEM;

	pci->dev = dev;
	pci->ops = &thunderbay_pcie_dw_pcie_ops;
        pci->version = 0x480A;

	pcie->pci = pci;
	pcie->mode = mode;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "apb");
	pcie->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(pcie->base))
		return PTR_ERR(pcie->base);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "dbi");
	pci->dbi_base = devm_ioremap_resource(dev, res);
	if (IS_ERR(pci->dbi_base))
		return PTR_ERR(pci->dbi_base);

//	pci->dbi_base2 = pci->dbi_base + 0x100000;
	pci->dbi_base2 = pci->dbi_base ;
	pci->atu_base = pci->dbi_base + 0x300000;

	platform_set_drvdata(pdev, pcie);

	switch (pcie->mode) {
	case DW_PCIE_RC_TYPE:
		if (!IS_ENABLED(CONFIG_PCIE_THUNDERBAY_HOST))
			return -ENODEV;

		ret = thunderbay_pcie_add_pcie_port(pcie, pdev);
		if (ret < 0)
			return ret;
		break;
	case DW_PCIE_EP_TYPE:
		if (!IS_ENABLED(CONFIG_PCIE_THUNDERBAY_EP))
			return -ENODEV;

		ret = thunderbay_pcie_add_pcie_ep(pcie, pdev);
		if (ret < 0)
			return ret;
		break;
	default:
		dev_err(dev, "INVALID device type %d\n", pcie->mode);
	}

	return 0;
}

static struct platform_driver thunderbay_pcie_driver __refdata = {
	.probe  = thunderbay_pcie_probe,
	.driver = {
		.name = "thunderbay-pcie",
		.of_match_table = thunderbay_pcie_of_match,
	},
};
builtin_platform_driver(thunderbay_pcie_driver);
