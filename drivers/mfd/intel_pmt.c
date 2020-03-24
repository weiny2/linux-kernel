// SPDX-License-Identifier: GPL-2.0
/*
 * Intel Platform Monitoring Technology MFD driver
 *
 * Copyright (c) 2020, Intel Corporation.
 * All Rights Reserved.
 *
 * Authors: David E. Box <david.e.box@linux.intel.com>
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/mfd/core.h>
#include <linux/intel-dvsec.h>

#define TELEM_DEV_NAME		"pmt_telemetry"
#define WATCHER_DEV_NAME	"pmt_watcher"
#define CRASHLOG_DEV_NAME	"pmt_crashlog"

static const struct pmt_platform_info tgl_info = {
	.quirks = PMT_QUIRK_NO_WATCHER | PMT_QUIRK_NO_CRASHLOG,
};

static const struct pmt_platform_info pmt_info = {
};

static int
pmt_add_dev(struct pci_dev *pdev, struct intel_dvsec_header *header,
	    struct pmt_platform_info *info)
{
	struct mfd_cell *cell, *tmp;
	const char *name;
	int i;

	switch (header->id) {
	case DVSEC_INTEL_ID_TELEM:
		name = TELEM_DEV_NAME;
		break;
	case DVSEC_INTEL_ID_WATCHER:
		if (info->quirks && PMT_QUIRK_NO_WATCHER) {
			dev_info(&pdev->dev, "Watcher not supported\n");
			return 0;
		}
		name = WATCHER_DEV_NAME;
		break;
	case DVSEC_INTEL_ID_CRASHLOG:
		if (info->quirks && PMT_QUIRK_NO_WATCHER) {
			dev_info(&pdev->dev, "Crashlog not supported\n");
			return 0;
		}
		name = CRASHLOG_DEV_NAME;
		break;
	default:
		return -EINVAL;
	}

	cell = devm_kcalloc(&pdev->dev, header->num_entries,
			    sizeof(*cell), GFP_KERNEL);
	if (!cell)
		return -ENOMEM;

	/* Create a platform device for each entry. */
	for (i = 0, tmp = cell; i < header->num_entries; i++, tmp++) {
		struct resource *res;

		res = devm_kzalloc(&pdev->dev, sizeof(*res), GFP_KERNEL);
		if (!res)
			return -ENOMEM;

		tmp->name = name;

		res->start = pdev->resource[header->tbir].start +
			     header->offset +
			     (i * (INTEL_DVSEC_ENTRY_SIZE << 2));
		res->end = res->start + (header->entry_size << 2) - 1;
		res->flags = IORESOURCE_MEM;

		tmp->resources = res;
		tmp->num_resources = 1;
		tmp->platform_data = header;
		tmp->pdata_size = sizeof(*header);

	}

	return devm_mfd_add_devices(&pdev->dev, PLATFORM_DEVID_AUTO, cell,
				    header->num_entries, NULL, 0, NULL);
}

static int
pmt_pci_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	u16 vid;
	u32 table;
	int ret, pos = 0, last_pos = 0;
	struct pmt_platform_info *info;
	struct intel_dvsec_header header;

	ret = pcim_enable_device(pdev);
	if (ret)
		return ret;

	info = devm_kmemdup(&pdev->dev, (void *)id->driver_data, sizeof(*info),
			    GFP_KERNEL);

	if (!info)
		return -ENOMEM;

	while ((pos = pci_find_next_ext_capability(pdev, pos, PCI_EXT_CAP_ID_DVSEC))) {
		pci_read_config_word(pdev, pos + PCI_DVSEC_HEADER1, &vid);
		if (vid != PCI_VENDOR_ID_INTEL)
			continue;

		pci_read_config_word(pdev, pos + PCI_DVSEC_HEADER2,
				     &header.id);

		pci_read_config_byte(pdev, pos + INTEL_DVSEC_ENTRIES,
				     &header.num_entries);

		pci_read_config_byte(pdev, pos + INTEL_DVSEC_SIZE,
				     &header.entry_size);

		if (!header.num_entries || !header.entry_size)
			return -EINVAL;

		pci_read_config_dword(pdev, pos + INTEL_DVSEC_TABLE,
				      &table);

		header.tbir = INTEL_DVSEC_TABLE_BAR(table);
		header.offset = INTEL_DVSEC_TABLE_OFFSET(table);
		ret = pmt_add_dev(pdev, &header, info);
		if (ret)
			dev_warn(&pdev->dev,
				 "Failed to add devices for DVSEC id %d\n",
				 header.id);
		last_pos = pos;
	}

	if (!last_pos) {
		dev_err(&pdev->dev, "No supported PMT capabilities found.\n");
		return -ENODEV;
	}

	pm_runtime_put(&pdev->dev);
	pm_runtime_allow(&pdev->dev);

	return 0;
}

static void pmt_pci_remove(struct pci_dev *pdev)
{
	pm_runtime_forbid(&pdev->dev);
	pm_runtime_get_sync(&pdev->dev);
}

static const struct pci_device_id pmt_pci_ids[] = {
	/* TGL */
	{ PCI_VDEVICE(INTEL, 0x9a0d), (kernel_ulong_t)&tgl_info },
	{ }
};
MODULE_DEVICE_TABLE(pci, pmt_pci_ids);

static struct pci_driver pmt_pci_driver = {
	.name = "intel-pmt",
	.id_table = pmt_pci_ids,
	.probe = pmt_pci_probe,
	.remove = pmt_pci_remove,
};

module_pci_driver(pmt_pci_driver);

MODULE_AUTHOR("David E. Box <david.e.box@linux.intel.com>");
MODULE_DESCRIPTION("Intel Platform Monitoring Technology MFD driver");
MODULE_LICENSE("GPL v2");
