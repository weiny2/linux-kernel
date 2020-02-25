/* SPDX-License-Identifier: GPL-2.0 only */
/*****************************************************************************
 *
 * Intel Keem Bay xlink IPC Driver
 *
 * Copyright (C) 2019 Intel Corporation
 *
 ****************************************************************************/

#ifndef _XLINK_IPC_H_
#define _XLINK_IPC_H_

#include <linux/types.h>

struct xlink_ipc_fd {
	u32 sw_device_id;
	u8 node;
	u16 chan;
	u8 is_volatile;
};

int xlink_ipc_connect(const char *device_name, void **fd);

int xlink_ipc_read(void *fd, void *data, size_t *const size, u32 timeout);

int xlink_ipc_write(void *fd, void *data, size_t *const size, u32 timeout);

int xlink_ipc_get_device_list(u32 *sw_device_id_list, u32 *num_devices,
		int pid);

int xlink_ipc_get_device_name(u32 sw_device_id, char *device_name,
		size_t name_size);

int xlink_ipc_get_device_status(const char *device_name, u32 *device_status);

int xlink_ipc_boot_device(const char *device_name, const char *binary_path);

int xlink_ipc_reset_device(void *fd, u32 operating_frequency);

int xlink_ipc_open_channel(void *fd, u32 channel);

int xlink_ipc_close_channel(void *fd, u32 channel);

#endif /* _XLINK_IPC_H_ */
