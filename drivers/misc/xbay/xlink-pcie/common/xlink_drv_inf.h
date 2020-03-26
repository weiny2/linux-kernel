/* SPDX-License-Identifier: GPL-2.0 only */
/*****************************************************************************
 *
 * Intel Keem Bay XLink PCIe Driver
 *
 * Copyright (C) 2019 Intel Corporation
 *
 ****************************************************************************/

#ifndef _XLINK_DRV_INF_H_
#define _XLINK_DRV_INF_H_

#include <linux/types.h>

enum _xlink_device_status {
	_XLINK_DEV_OFF,
	_XLINK_DEV_ERROR,
	_XLINK_DEV_BUSY,
	_XLINK_DEV_RECOVERY,
	_XLINK_DEV_READY
};

/* SW device id encoding based on VPU 2.1 platform Spec*/
enum slice_id
{
	SLICE_ID_0,
	SLICE_ID_1,
	SLICE_ID_2,
	SLICE_ID_3
};

enum tbh_device_type
{
	TBH_PRIME=0x10,
	TBH_STANDARD=0x20
};

enum function_type
{
	VPU_FUNCTION = 0,
	MEDIA_FUNCTION = 0x40
};

int xlink_pcie_get_device_list(uint32_t *sw_device_id_list,
			       uint32_t *num_devices, int pid);
int xlink_pcie_get_device_name(uint32_t sw_device_id, char *device_name,
			       size_t name_size);
int xlink_pcie_get_device_status(const char *device_name,
				 uint32_t *device_status);
int xlink_pcie_boot_remote(const char *device_name, const char *binary_path);
int xlink_pcie_connect(const char *device_name, void **fd);
int xlink_pcie_read(void *fd, void *data, size_t *const size, uint32_t timeout);
int xlink_pcie_write(void *fd, void *data, size_t *const size,
		     uint32_t timeout);
int xlink_pcie_reset_device(void *fd, uint32_t operating_frequency);

#endif
