// SPDX-License-Identifier: GPL-2.0-only
/*
 * Intel ThunderBay eMMC PHY driver
 *
 * Copyright (C) 2020 Intel Corporation
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
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>

/* eMMC/SD/SDIO core/phy configuration registers */
#define CTRL_CFG_0	0x00
#define CTRL_CFG_1	0x04
#define CTRL_PRESET_0	0x08
#define CTRL_PRESET_1	0x0c
#define CTRL_PRESET_2	0x10
#define CTRL_PRESET_3	0x14
#define CTRL_PRESET_4	0x18
#define CTRL_CFG_2	0x1c
#define CTRL_CFG_3	0x20
#define PHY_CFG_0	0x24
#define PHY_CFG_1	0x28
#define PHY_CFG_2	0x2c
#define PHYBIST_CTRL	0x30
#define SDHC_STAT3	0x34
#define PHY_STAT	0x38
#define PHYBIST_STAT_0	0x3c
#define PHYBIST_STAT_1	0x40
#define EMMC_AXI        0x44

#if 0
#define SDHC_STAT_0	0x30
#define SDHC_STAT_1	0x34
#define SDHC_STAT_2	0x38
#define SDHC_STAT3	0x3c
#define PHY_STAT	0x40
#define RAM_CTRL	0x44
#define PHYBIST_CTRL	0x48
#define PHYBIST_STAT_0	0x4c
#define PHYBIST_STAT_1	0x50
#endif

/* CTRL_CFG_0 bit fields */
#define SUPPORT_HS_MASK		BIT(26)
#define SUPPORT_HS_SHIFT	26

/* CTRL_CFG_1 bit fields */
#define SUPPORT_SDR50_MASK	BIT(28)
#define SUPPORT_SDR50_SHIFT	28
#define SLOT_TYPE_MASK		GENMASK(27, 26)
#define SLOT_TYPE_OFFSET	26
#define SUPPORT_64B_MASK	BIT(24)
#define SUPPORT_64B_SHIFT	24
#define SUPPORT_HS400_MASK	BIT(2)
#define SUPPORT_HS400_SHIFT	2
#define SUPPORT_DDR50_MASK	BIT(1)
#define SUPPORT_DDR50_SHIFT	1
#define SUPPORT_SDR104_MASK	BIT(0)
#define SUPPORT_SDR104_SHIFT	0

/* PHY_CFG_0 bit fields */
#define OTAP_DLY_ENA_MASK	BIT(27)
#define OTAP_DLY_ENA_SHIFT	27
#define OTAP_DLY_SEL_MASK	GENMASK(26, 23)
#define OTAP_DLY_SEL_SHIFT	23
#define ITAP_CHG_WIN_MASK	BIT(22)
#define ITAP_CHG_WIN_SHIFT	22
#define ITAP_DLY_ENA_MASK	BIT(21)
#define ITAP_DLY_ENA_SHIFT	21
#define ITAP_DLY_SEL_MASK	GENMASK(20, 16)
#define ITAP_DLY_SEL_SHIFT	16
#define DLL_EN_MASK		BIT(10)
#define DLL_EN_SHIFT		10
#define PWR_DOWN_MASK		BIT(0)
#define PWR_DOWN_SHIFT		0

/* PHY_CFG_1 bit fields */
#define REN_DAT_MASK		GENMASK(19, 12)
#define REN_DAT_SHIFT		12
#define REN_CMD_MASK		BIT(11)
#define REN_CMD_SHIFT		11
#define REN_STRB_MASK		BIT(10)
#define REN_STRB_SHIFT		10

/* PHY_CFG_2 bit fields */
#define SEL_STRB_MASK		GENMASK(16, 13) 
#define SEL_STRB_SHIFT		13
#define SEL_FREQ_MASK		GENMASK(12, 10)
#define SEL_FREQ_SHIFT		10

/* PHY_STAT bit fields */
#define CAL_DONE		BIT(6)
#define DLL_RDY			BIT(5)

#define OTAP_DLY		0x26
#define ITAP_DLY		0x0
#define STRB			0x5

/* From ACS_eMMC51_16nFFC_RO1100_Userguide_v1p0.pdf p17 */
#define FREQSEL_200M_170M	0x0
#define FREQSEL_170M_140M	0x1
#define FREQSEL_140M_110M	0x2
#define FREQSEL_110M_80M	0x3
#define FREQSEL_80M_50M		0x4
#define FREQSEL_275M_250M	0x5
#define FREQSEL_250M_225M	0x6
#define FREQSEL_225M_200M	0x7

struct thunderbay_emmc_phy {
	void __iomem	*reg_base;
	struct clk	*emmcclk;
};

static inline void update_reg(struct thunderbay_emmc_phy *tbh_phy, u32 offset,
			      u32 mask, u32 shift, u32 val)
{
	u32 tmp;

	tmp = readl(tbh_phy->reg_base + offset);
	tmp &= ~mask;
	tmp |= val << shift;
	writel(tmp, tbh_phy->reg_base + offset);
}

static int is_caldone(struct thunderbay_emmc_phy *tbh_phy)
{
	int retries = 2000;
	u32 val;

	do {
		udelay(10);
		val = readl(tbh_phy->reg_base + PHY_STAT);

		if (val & CAL_DONE)
			return 1;
	} while (--retries);

	return 0;
}

static int is_dllrdy(struct thunderbay_emmc_phy *tbh_phy)
{
	int retries = 2000;
	u32 val;

	do {
		udelay(10);
		val = readl(tbh_phy->reg_base + PHY_STAT);

		if (val & DLL_RDY)
			return 1;
	} while (--retries);

	return 0;
}

static int thunderbay_emmc_phy_power(struct phy *phy, bool power_on)
{
	struct thunderbay_emmc_phy *tbh_phy = phy_get_drvdata(phy);
	unsigned int freqsel = FREQSEL_200M_170M;
	unsigned long rate;

	update_reg(tbh_phy, PHY_CFG_0, PWR_DOWN_MASK, PWR_DOWN_SHIFT, 0x0);

	/* Disable DLL */
	update_reg(tbh_phy, PHY_CFG_0, DLL_EN_MASK, DLL_EN_SHIFT, 0x0);

	if (!power_on)
		return 0;

	rate = clk_get_rate(tbh_phy->emmcclk);
	/* TO BE REMOVED LATER */
	dev_info(&phy->dev, "clock rate: %lu\n", rate);

	if (rate != 0) {
		switch (rate) {
		case 170000001 ... 200000000:
			freqsel = FREQSEL_200M_170M;
			break;
		case 140000001 ... 170000000:
			freqsel = FREQSEL_170M_140M;
			break;
		case 110000001 ... 140000000:
			freqsel = FREQSEL_140M_110M;
			break;
		case 80000001 ... 110000000:
			freqsel = FREQSEL_110M_80M;
			break;
		case 50000000 ... 80000000:
			freqsel = FREQSEL_80M_50M;
			break;
		case 250000001 ... 275000000:
			freqsel = FREQSEL_275M_250M;
			break;
		case 225000001 ... 250000000:
			freqsel = FREQSEL_250M_225M;
			break;
		case 200000001 ... 225000000:
			freqsel = FREQSEL_225M_200M;
			break;
		default:
			break;
		}

		if ((rate < 50000000) || (rate > 200000000))
			dev_warn(&phy->dev, "Unsupported rate: %lu\n", rate);
	}

	udelay(5);
	update_reg(tbh_phy, PHY_CFG_0, PWR_DOWN_MASK, PWR_DOWN_SHIFT, 0x1);

/*	if (!is_caldone(tbh_phy)) {
		pr_err("%s: caldone failed\n", __func__);
		return -EAGAIN;
	}*/

	/* Set frequency of the DLL operation */
	update_reg(tbh_phy, PHY_CFG_2, SEL_FREQ_MASK, SEL_FREQ_SHIFT, freqsel);

	/* Enable DLL */
	update_reg(tbh_phy, PHY_CFG_0, DLL_EN_MASK, DLL_EN_SHIFT, 0x1);

	if (rate == 0)
		return 0;

/*	if (!is_dllrdy(tbh_phy)) {
		pr_err("%s: dllrdy failed\n", __func__);
		return -EAGAIN;
	}
*/
	return 0;
}

static int thunderbay_emmc_phy_init(struct phy *phy)
{
	struct thunderbay_emmc_phy *tbh_phy = phy_get_drvdata(phy);
	int ret = 0;

	tbh_phy->emmcclk = clk_get(&phy->dev, "emmcclk");
	if (IS_ERR(tbh_phy->emmcclk)) {
		dev_dbg(&phy->dev, "Error getting emmcclk: %d\n", ret);
		tbh_phy->emmcclk = NULL;
	}

	return ret;
}

static int thunderbay_emmc_phy_exit(struct phy *phy)
{
	struct thunderbay_emmc_phy *tbh_phy = phy_get_drvdata(phy);

	clk_put(tbh_phy->emmcclk);

	return 0;
}

static int thunderbay_emmc_phy_power_on(struct phy *phy)
{
	struct thunderbay_emmc_phy *tbh_phy = phy_get_drvdata(phy);

	/* Overwrite capability bits configureable in bootloader */
	update_reg(tbh_phy, CTRL_CFG_0,
		   SUPPORT_HS_MASK, SUPPORT_HS_SHIFT, 0x1);
	update_reg(tbh_phy, CTRL_CFG_1,
		   SUPPORT_SDR50_MASK, SUPPORT_SDR50_SHIFT, 0x1);
	update_reg(tbh_phy, CTRL_CFG_1,
		   SUPPORT_DDR50_MASK, SUPPORT_DDR50_SHIFT, 0x1);
	update_reg(tbh_phy, CTRL_CFG_1,
		   SUPPORT_SDR104_MASK, SUPPORT_SDR104_SHIFT, 0x1);
	update_reg(tbh_phy, CTRL_CFG_1,
		   SUPPORT_HS400_MASK, SUPPORT_HS400_SHIFT, 0x1);

	/* Enable OTAP delay configuration */
	update_reg(tbh_phy, PHY_CFG_0,
		   OTAP_DLY_ENA_MASK, OTAP_DLY_ENA_SHIFT, 0x1);

	/* OTAP delay */
	update_reg(tbh_phy, PHY_CFG_0,
		   OTAP_DLY_SEL_MASK, OTAP_DLY_SEL_SHIFT, OTAP_DLY);

	/* Assert ITAP_CHG_WIN */
	update_reg(tbh_phy, PHY_CFG_0,
		   ITAP_CHG_WIN_MASK, ITAP_CHG_WIN_SHIFT, 0x0);

	/* Enable ITAP delay configuration */
	update_reg(tbh_phy, PHY_CFG_0,
		   ITAP_DLY_ENA_MASK, ITAP_DLY_ENA_SHIFT, 0x0);

	/* ITAP delay */
	update_reg(tbh_phy, PHY_CFG_0,
		   ITAP_DLY_SEL_MASK, ITAP_DLY_SEL_SHIFT, ITAP_DLY);

	/* Deassert ITAP_CHG_WIN */
	update_reg(tbh_phy, PHY_CFG_0,
		   ITAP_CHG_WIN_MASK, ITAP_CHG_WIN_SHIFT, 0x0);

	update_reg(tbh_phy, PHY_CFG_1, REN_DAT_MASK, REN_DAT_SHIFT, 0x0);
	update_reg(tbh_phy, PHY_CFG_1, REN_CMD_MASK, REN_CMD_SHIFT, 0x0);
	update_reg(tbh_phy, PHY_CFG_1, REN_STRB_MASK, REN_STRB_SHIFT, 0x0);

	update_reg(tbh_phy, PHY_CFG_2, SEL_STRB_MASK, SEL_STRB_SHIFT, STRB);

	update_reg(tbh_phy, PHY_CFG_0, PWR_DOWN_MASK, PWR_DOWN_SHIFT, 0x1);

	return thunderbay_emmc_phy_power(phy, 1);
}

static int thunderbay_emmc_phy_power_off(struct phy *phy)
{
	return thunderbay_emmc_phy_power(phy, 0);
}

static const struct phy_ops thunderbay_emmc_phy_ops = {
	.init		= thunderbay_emmc_phy_init,
	.exit		= thunderbay_emmc_phy_exit,
	.power_on	= thunderbay_emmc_phy_power_on,
	//.power_on	= NULL,
	.power_off	= thunderbay_emmc_phy_power_off,
	.owner		= THIS_MODULE,
};

static int thunderbay_sd_phy_dummy(struct phy *phy)
{
	return 0;
}

static int thunderbay_sd_phy_power_on(struct phy *phy)
{
	struct thunderbay_emmc_phy *tbh_phy = phy_get_drvdata(phy);

	/* Overwrite capability bits configureable in bootloader */
	update_reg(tbh_phy, CTRL_CFG_0,
		   SUPPORT_HS_MASK, SUPPORT_HS_SHIFT, 0x1);
	update_reg(tbh_phy, CTRL_CFG_1,
		   SUPPORT_SDR50_MASK, SUPPORT_SDR50_SHIFT, 0x1);
	update_reg(tbh_phy, CTRL_CFG_1,
		   SUPPORT_DDR50_MASK, SUPPORT_DDR50_SHIFT, 0x1);
	update_reg(tbh_phy, CTRL_CFG_1,
		   SUPPORT_SDR104_MASK, SUPPORT_SDR104_SHIFT, 0x1);
	update_reg(tbh_phy, CTRL_CFG_1,
		   SUPPORT_HS400_MASK, SUPPORT_HS400_SHIFT, 0x1);

	return 0;
}

static const struct phy_ops thunderbay_sd_phy_ops = {
	.init		= thunderbay_sd_phy_dummy,
	.exit		= thunderbay_sd_phy_dummy,
	.power_on	= thunderbay_sd_phy_power_on,
	.power_off	= thunderbay_sd_phy_dummy,
	.owner		= THIS_MODULE,
};

static const struct of_device_id thunderbay_emmc_phy_of_match[] = {
	{ .compatible = "intel,thunderbay-emmc-phy",
		(void *)&thunderbay_emmc_phy_ops },
	{ .compatible = "intel,thunderbay-sd-phy",
		(void *)&thunderbay_sd_phy_ops },
	{}
};
MODULE_DEVICE_TABLE(of, thunderbay_emmc_phy_of_match);

static int thunderbay_emmc_phy_probe(struct platform_device *pdev)
{
	const struct of_device_id *id;
	struct device *dev = &pdev->dev;
	struct thunderbay_emmc_phy *tbh_phy;
	struct phy *generic_phy;
	struct phy_provider *phy_provider;
	struct resource *res;

	if (!dev->of_node)
		return -ENODEV;

	tbh_phy = devm_kzalloc(dev, sizeof(*tbh_phy), GFP_KERNEL);
	if (!tbh_phy)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	tbh_phy->reg_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(tbh_phy->reg_base)) {
		dev_err(&pdev->dev, "region map failed\n");
		return PTR_ERR(tbh_phy->reg_base);
	}

	id = of_match_node(thunderbay_emmc_phy_of_match, pdev->dev.of_node);
	generic_phy = devm_phy_create(dev, dev->of_node, id->data);
	if (IS_ERR(generic_phy)) {
		dev_err(dev, "failed to create PHY\n");
		return PTR_ERR(generic_phy);
	}

	phy_set_drvdata(generic_phy, tbh_phy);
	phy_provider = devm_of_phy_provider_register(dev, of_phy_simple_xlate);

	return PTR_ERR_OR_ZERO(phy_provider);
}

static struct platform_driver thunderbay_emmc_phy_driver = {
	.probe		= thunderbay_emmc_phy_probe,
	.driver		= {
		.name	= "thunderbay-emmc-phy",
		.of_match_table = thunderbay_emmc_phy_of_match,
	},
};
module_platform_driver(thunderbay_emmc_phy_driver);

MODULE_AUTHOR("Nandhini S <nandhini.srikandan@intel.com>");
MODULE_DESCRIPTION("Intel ThunderBay eMMC PHY driver");
MODULE_LICENSE("GPL v2");
