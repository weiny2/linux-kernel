// SPDX-License-Identifier: GPL-2.0 only
/*****************************************************************************
 *
 * Intel Keem Bay XLink PCIe Driver
 *
 * Copyright (C) 2019 Intel Corporation
 *
 ****************************************************************************/

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/xlink_drv_inf.h>
#include "mxlk_pci.h"

int xlink_pcie_get_device_list(uint32_t *sw_device_id_list,
			       uint32_t *num_devices, int pid)
{
	if (pid == PCI_DEVICE_ID_INTEL_KEEMBAY)
		*num_devices = mxlk_get_device_num(sw_device_id_list);
	if (pid == PCI_DEVICE_ID_INTEL_THUNDERBAY)
		*num_devices = mxlk_get_device_num(sw_device_id_list);
	else
		*num_devices = 0;

	return 0;
}
EXPORT_SYMBOL(xlink_pcie_get_device_list);

int xlink_pcie_get_device_name(uint32_t sw_device_id, char *device_name,
			       size_t name_size)
{
	return mxlk_get_device_name_by_id(sw_device_id, device_name, name_size);
}
EXPORT_SYMBOL(xlink_pcie_get_device_name);

int xlink_pcie_get_device_status(const char *device_name,
				 uint32_t *device_status)
{
	int rc;
	u32 status;

	rc = mxlk_get_device_status_by_name(device_name, &status);
	if (rc)
		return rc;

	switch (status) {
	case MXLK_STATUS_READY:
	case MXLK_STATUS_RUN:
		*device_status = XLINK_DEV_READY;
		break;
	case MXLK_STATUS_ERROR:
		*device_status = XLINK_DEV_ERROR;
		break;
	case MXLK_STATUS_RECOVERY:
		*device_status = XLINK_DEV_RECOVERY;
		break;
	default:
		*device_status = XLINK_DEV_BUSY;
		break;
	}

	return 0;
}
EXPORT_SYMBOL(xlink_pcie_get_device_status);

int xlink_pcie_boot_remote(const char *device_name, const char *binary_path)
{
	return mxlk_pci_boot_device(device_name);
}
EXPORT_SYMBOL(xlink_pcie_boot_remote);

int xlink_pcie_connect(const char *device_name, void **fd)
{
	return mxlk_pci_connect_device(device_name, fd);
}
EXPORT_SYMBOL(xlink_pcie_connect);

int xlink_pcie_read(void *fd, void *data, size_t *const size, uint32_t timeout)
{
	return mxlk_pci_read(fd, data, size, timeout);
}
EXPORT_SYMBOL(xlink_pcie_read);

int xlink_pcie_write(void *fd, void *data, size_t *const size, uint32_t timeout)
{
	return mxlk_pci_write(fd, data, size, timeout);
}
EXPORT_SYMBOL(xlink_pcie_write);

int xlink_pcie_reset_device(void *fd, uint32_t operating_frequency)
{
	return mxlk_pci_reset_device(fd);
}
EXPORT_SYMBOL(xlink_pcie_reset_device);
