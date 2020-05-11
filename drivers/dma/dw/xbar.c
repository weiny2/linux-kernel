// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2020 Intel Corporation

#include <linux/acpi.h>
#include <linux/acpi_dma.h>
#include <linux/bits.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/pci.h>

#include "internal.h"

#define DMA_CTL_CH(x)			(0x1000 + (x) * 4)
#define DMA_SRC_ADDR_FILLIN(x)		(0x1100 + (x) * 4)
#define DMA_DST_ADDR_FILLIN(x)		(0x1200 + (x) * 4)
#define DMA_XBAR_SEL(x)			(0x1300 + (x) * 4)
#define DMA_REGACCESS_CHID_CFG		0x1400

#define CTL_CH_TRANSFER_MODE_MASK	GENMASK(1, 0)
#define CTL_CH_TRANSFER_MODE_S2S	0
#define CTL_CH_TRANSFER_MODE_S2D	1
#define CTL_CH_TRANSFER_MODE_D2S	2
#define CTL_CH_TRANSFER_MODE_D2D	3
#define CTL_CH_RD_RS_MASK		GENMASK(4, 3)
#define CTL_CH_WR_RS_MASK		GENMASK(6, 5)
#define CTL_CH_RD_NON_SNOOP_BIT		BIT(8)
#define CTL_CH_WR_NON_SNOOP_BIT		BIT(9)

#define XBAR_SEL_DEVID_MASK		GENMASK(15, 0)
#define XBAR_SEL_RX_TX_BIT		BIT(16)
#define XBAR_SEL_RX_TX_SHIFT		16

#define REGACCESS_CHID_MASK		GENMASK(2, 0)

static bool xbar_filter(struct dma_chan *chan, void *param)
{
	struct acpi_dma_spec *dma_spec = param;
	struct dw_dma_slave slave = {
		.dma_dev = dma_spec->dev,
		.src_id = dma_spec->slave_id,
		.dst_id = dma_spec->slave_id,
	};

	return dw_dma_filter(chan, &slave);
}

static void xbar_configure(struct acpi_dma_spec *dma_spec, struct dma_chan *chan)
{
	struct dw_dma_chip_pdata *data = dev_get_drvdata(dma_spec->dev);
	struct pci_dev *pdev = to_pci_dev(dma_spec->consumer);
#if 0
	phys_addr_t base = pci_resource_start(pdev, 0);
#endif
	void __iomem *x = data->chip->regs;
	size_t d = dma_spec->index;
	int c = chan->chan_id;
	u32 value;

	/* DMA Channel ID Configuration register must be programmed first */
	value = readl(x + DMA_REGACCESS_CHID_CFG);
	value &= ~REGACCESS_CHID_MASK;
	value |= c;
	writel(value, x + DMA_REGACCESS_CHID_CFG);

#if 0
	/* Configure upper part of the address */
	if (d) {
		writel(upper_32_bits(base), x + DMA_SRC_ADDR_FILLIN(c));
		writel(0, x + DMA_DST_ADDR_FILLIN(c));
	} else {
		writel(0, x + DMA_SRC_ADDR_FILLIN(c));
		writel(upper_32_bits(base), x + DMA_DST_ADDR_FILLIN(c));
	}
#endif

	/* Configure crossbar selection */
	value = readl(x + DMA_XBAR_SEL(c));

	/* DEVFN selection */
	value &= XBAR_SEL_DEVID_MASK;
	value |= pdev->devfn;

	/* Direction */
	if (d)
		value &= ~XBAR_SEL_RX_TX_BIT;
	else
		value |= XBAR_SEL_RX_TX_BIT;

	writel(value, x + DMA_XBAR_SEL(c));

	/* Configure channel attributes */
	value = readl(x + DMA_CTL_CH(c));
#if 0
	value &= CTL_CH_RD_NON_SNOOP_BIT | CTL_CH_WR_NON_SNOOP_BIT;
	value &= CTL_CH_RD_RS_MASK | CTL_CH_WR_RS_MASK;
#endif
	value &= CTL_CH_TRANSFER_MODE_MASK;
#if 0
	value |= d ? CTL_CH_RD_NON_SNOOP_BIT : CTL_CH_WR_NON_SNOOP_BIT;
#endif
	value |= d ? CTL_CH_TRANSFER_MODE_S2D : CTL_CH_TRANSFER_MODE_D2S;
	writel(value, x + DMA_CTL_CH(c));
}

static struct dma_chan *xbar_xlate(struct acpi_dma_spec *dma_spec, struct acpi_dma *adma)
{
	struct acpi_dma_filter_info *info = adma->data;
	struct dma_chan *chan;

	if (!info || !info->filter_fn)
		return NULL;

	chan = dma_request_channel(info->dma_cap, info->filter_fn, dma_spec);
	if (!chan)
		return NULL;

	xbar_configure(dma_spec, chan);
	return chan;
}

void xbar_acpi_controller_register(struct dw_dma *dw)
{
	struct device *dev = dw->dma.dev;
	struct acpi_dma_filter_info *info;
	int ret;

	if (!has_acpi_companion(dev))
		return;

	info = devm_kzalloc(dev, sizeof(*info), GFP_KERNEL);
	if (!info)
		return;

	dma_cap_zero(info->dma_cap);
	dma_cap_set(DMA_SLAVE, info->dma_cap);
	info->filter_fn = xbar_filter;

	ret = acpi_dma_controller_register(dev, xbar_xlate, info);
	if (ret)
		dev_err(dev, "could not register acpi_dma_controller\n");
}
EXPORT_SYMBOL_GPL(xbar_acpi_controller_register);

void xbar_acpi_controller_free(struct dw_dma *dw)
{
	struct device *dev = dw->dma.dev;

	if (!has_acpi_companion(dev))
		return;

	acpi_dma_controller_free(dev);
}
EXPORT_SYMBOL_GPL(xbar_acpi_controller_free);
