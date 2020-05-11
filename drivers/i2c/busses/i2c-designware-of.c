// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Synopsys DesignWare I2C adapter driver.
 *
 * Based on the TI DAVINCI I2C adapter driver.
 *
 * Copyright (C) 2006 Texas Instruments.
 * Copyright (C) 2007 MontaVista Software Inc.
 * Copyright (C) 2009 Provigent Ltd.
 */
#include <linux/bits.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/export.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/types.h>

#include "i2c-designware-core.h"

#define MSCC_ICPU_CFG_TWI_DELAY		0x0
#define MSCC_ICPU_CFG_TWI_DELAY_ENABLE	BIT(0)
#define MSCC_ICPU_CFG_TWI_SPIKE_FILTER	0x4

static int mscc_twi_set_sda_hold_time(struct dw_i2c_dev *dev)
{
	writel((dev->sda_hold_time << 1) | MSCC_ICPU_CFG_TWI_DELAY_ENABLE,
	       dev->ext + MSCC_ICPU_CFG_TWI_DELAY);

	return 0;
}

int i2c_dw_of_configure(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);
	struct dw_i2c_dev *dev = platform_get_drvdata(pdev);

	switch (dev->flags & MODEL_MASK) {
	case MODEL_MSCC_OCELOT:
		dev->ext = devm_platform_ioremap_resource(pdev, 1);
		if (!IS_ERR(dev->ext))
			dev->set_sda_hold_time = mscc_twi_set_sda_hold_time;
		break;
	default:
		break;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(i2c_dw_of_configure);
