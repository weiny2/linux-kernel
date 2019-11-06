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
#include <linux/mxlk_interface.h>
#include "../common/mxlk_core.h"

int keembay_pcie_boot_remote(const char *deviceName, const char *binaryPath)
{
	return -EPERM;
}
EXPORT_SYMBOL(keembay_pcie_boot_remote);

int keembay_pcie_reset_remote(void *fd)
{
	struct mxlk_interface *inf = (struct mxlk_interface *)fd;

	return mxlk_core_close(inf);
}
EXPORT_SYMBOL(keembay_pcie_reset_remote);

int keembay_pcie_get_devicename(int index, char *name, int nameSize, int pid)
{
	return 0;
}
EXPORT_SYMBOL(keembay_pcie_get_devicename);

int keembay_pcie_connect(const char *devPathRead, const char *devPathWrite,
			 void **fd)
{
	int error;
	struct mxlk_interface *inf = mxlk_core_default_interface();

	if (!inf)
		return -EACCES;

	*fd = inf;

	error = mxlk_core_open(inf);
	if (error < 0) {
		*fd = NULL;
		return error;
	}

	return 0;
}
EXPORT_SYMBOL(keembay_pcie_connect);

int keembay_pcie_read(void *fd, void *data, size_t *size, unsigned int timeout)
{
	struct mxlk_interface *inf = (struct mxlk_interface *)fd;

	return mxlk_core_read(inf, data, size, timeout);
}
EXPORT_SYMBOL(keembay_pcie_read);

int keembay_pcie_write(void *fd, void *data, size_t size, unsigned int timeout)
{
	struct mxlk_interface *inf = (struct mxlk_interface *)fd;

	return mxlk_core_write(inf, data, size, timeout);
}
EXPORT_SYMBOL(keembay_pcie_write);
