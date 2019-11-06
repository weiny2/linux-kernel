/* SPDX-License-Identifier: GPL-2.0 only */
/*****************************************************************************
 *
 * Intel Keem Bay XLink PCIe Driver
 *
 * Copyright (C) 2019 Intel Corporation
 *
 ****************************************************************************/

#ifndef MXLK_INTERFACE_HEADER_
#define MXLK_INTERFACE_HEADER_

int keembay_pcie_boot_remote(const char *deviceName, const char *binaryPath);
int keembay_pcie_reset_remote(void *fd);
int keembay_pcie_get_devicename(int index, char *name, int nameSize, int pid);
int keembay_pcie_connect(const char *devPathRead, const char *devPathWrite,
			 void **fd);
int keembay_pcie_read(void *fd, void *data, size_t *size, unsigned int timeout);
int keembay_pcie_write(void *fd, void *data, size_t size, unsigned int timeout);

#endif
