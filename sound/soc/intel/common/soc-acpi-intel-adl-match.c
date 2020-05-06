// SPDX-License-Identifier: GPL-2.0
/*
 * soc-apci-intel-adl-match.c - tables and support for ADL ACPI enumeration.
 *
 * Copyright (c) 2020, Intel Corporation.
 *
 */

#include <sound/soc-acpi.h>
#include <sound/soc-acpi-intel-match.h>

struct snd_soc_acpi_mach snd_soc_acpi_intel_adl_machines[] = {
	{},
};
EXPORT_SYMBOL_GPL(snd_soc_acpi_intel_adl_machines);

/* this table is used when there is no I2S codec present */
struct snd_soc_acpi_mach snd_soc_acpi_intel_adl_sdw_machines[] = {
	{},
};
EXPORT_SYMBOL_GPL(snd_soc_acpi_intel_adl_sdw_machines);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Intel Common ACPI Match module");
