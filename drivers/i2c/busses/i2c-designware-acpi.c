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
#include <linux/acpi.h>
#include <linux/device.h>
#include <linux/dmi.h>
#include <linux/export.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/types.h>

#include "i2c-designware-core.h"

/*
 * The HCNT/LCNT information coming from ACPI should be the most accurate
 * for given platform. However, some systems get it wrong. On such systems
 * we get better results by calculating those based on the input clock.
 */
static const struct dmi_system_id i2c_dw_no_acpi_params[] = {
	{
		.ident = "Dell Inspiron 7348",
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "Dell Inc."),
			DMI_MATCH(DMI_PRODUCT_NAME, "Inspiron 7348"),
		},
	},
	{}
};

static void i2c_dw_acpi_params(struct device *device, char method[],
			       u16 *hcnt, u16 *lcnt, u32 *sda_hold)
{
	struct acpi_buffer buf = { ACPI_ALLOCATE_BUFFER };
	acpi_handle handle = ACPI_HANDLE(device);
	union acpi_object *obj;

	if (dmi_check_system(i2c_dw_no_acpi_params))
		return;

	if (ACPI_FAILURE(acpi_evaluate_object(handle, method, NULL, &buf)))
		return;

	obj = (union acpi_object *)buf.pointer;
	if (obj->type == ACPI_TYPE_PACKAGE && obj->package.count == 3) {
		const union acpi_object *objs = obj->package.elements;

		*hcnt = (u16)objs[0].integer.value;
		*lcnt = (u16)objs[1].integer.value;
		*sda_hold = (u32)objs[2].integer.value;
	}

	kfree(buf.pointer);
}

int i2c_dw_acpi_configure(struct device *device)
{
	struct dw_i2c_dev *dev = dev_get_drvdata(device);
	struct i2c_timings *t = &dev->timings;
	u32 ss_ht = 0, fp_ht = 0, hs_ht = 0, fs_ht = 0;

	dev->tx_fifo_depth = 32;
	dev->rx_fifo_depth = 32;

	/*
	 * Try to get SDA hold time and *CNT values from an ACPI method for
	 * selected speed modes.
	 */
	i2c_dw_acpi_params(device, "SSCN", &dev->ss_hcnt, &dev->ss_lcnt, &ss_ht);
	i2c_dw_acpi_params(device, "FPCN", &dev->fp_hcnt, &dev->fp_lcnt, &fp_ht);
	i2c_dw_acpi_params(device, "HSCN", &dev->hs_hcnt, &dev->hs_lcnt, &hs_ht);
	i2c_dw_acpi_params(device, "FMCN", &dev->fs_hcnt, &dev->fs_lcnt, &fs_ht);

	switch (t->bus_freq_hz) {
	case I2C_MAX_STANDARD_MODE_FREQ:
		dev->sda_hold_time = ss_ht;
		break;
	case I2C_MAX_FAST_MODE_PLUS_FREQ:
		dev->sda_hold_time = fp_ht;
		break;
	case I2C_MAX_HIGH_SPEED_MODE_FREQ:
		dev->sda_hold_time = hs_ht;
		break;
	case I2C_MAX_FAST_MODE_FREQ:
	default:
		dev->sda_hold_time = fs_ht;
		break;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(i2c_dw_acpi_configure);

void i2c_dw_acpi_adjust_bus_speed(struct device *device)
{
	struct dw_i2c_dev *dev = dev_get_drvdata(device);
	struct i2c_timings *t = &dev->timings;
	u32 acpi_speed;
	int i;

	acpi_speed = i2c_acpi_find_bus_speed(device);
	/*
	 * Some DSTDs use a non standard speed, round down to the lowest
	 * standard speed.
	 */
	for (i = 0; i < ARRAY_SIZE(i2c_dw_supported_speeds); i++) {
		if (acpi_speed >= i2c_dw_supported_speeds[i])
			break;
	}
	acpi_speed = i < ARRAY_SIZE(i2c_dw_supported_speeds) ? i2c_dw_supported_speeds[i] : 0;

	/*
	 * Find bus speed from the "clock-frequency" device property, ACPI
	 * or by using fast mode if neither is set.
	 */
	if (acpi_speed && t->bus_freq_hz)
		t->bus_freq_hz = min(t->bus_freq_hz, acpi_speed);
	else if (acpi_speed || t->bus_freq_hz)
		t->bus_freq_hz = max(t->bus_freq_hz, acpi_speed);
	else
		t->bus_freq_hz = I2C_MAX_FAST_MODE_FREQ;
}
EXPORT_SYMBOL_GPL(i2c_dw_acpi_adjust_bus_speed);
