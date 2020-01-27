/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * xlink Linux Kernel Platform API
 *
 * Copyright (C) 2018-2019 Intel Corporation
 *
 */
#ifndef __XLINK_PLATFORM_H
#define __XLINK_PLATFORM_H

#include "xlink-defs.h"

int xlink_platform_connect(uint32_t interface, const char *device_name,
		void **fd);

int xlink_platform_write(uint32_t interface, void *fd, void *data,
		size_t * const size, uint32_t timeout);

int xlink_platform_read(uint32_t interface, void *fd, void *data,
		size_t * const size, uint32_t timeout);

int xlink_platform_reset_device(uint32_t interface, void *fd,
		uint32_t operating_frequency);

int xlink_platform_boot_device(uint32_t interface, const char *device_name,
		const char *binary_path);

int xlink_platform_get_device_name(uint32_t interface, uint32_t sw_device_id,
		char *device_name, size_t name_size);

int xlink_platform_get_device_list(uint32_t interface,
		uint32_t *sw_device_id_list, uint32_t *num_devices, int pid);

int xlink_platform_get_device_status(uint32_t interface,
		const char *device_name, uint32_t *device_status);

int xlink_platform_open_channel(uint32_t interface, void *fd,
		uint32_t channel);

int xlink_platform_close_channel(uint32_t interface, void *fd,
		uint32_t channel);

void * xlink_platform_allocate(struct device *dev, dma_addr_t *handle,
		uint32_t size, uint32_t alignment);

void xlink_platform_deallocate(struct device *dev, void *buf,
		dma_addr_t handle, uint32_t size, uint32_t alignment);

#endif /* __XLINK_PLATFORM_H */
