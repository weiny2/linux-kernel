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

int xlink_platform_write(int interface, void *fd, void *data, size_t size,
		unsigned int timeout);

int xlink_platform_read(int interface, void *fd, void *data, size_t *size,
		unsigned int timeout);

int xlink_platform_connect(int interface, const char *dev_path_read,
		const char *dev_path_write, void **fd);

int xlink_platform_get_device_name(int interface, int index, char *name,
		int name_size);

int xlink_platform_get_device_name_extended(int interface, int index,
		char *name, int name_size, int pid);

int xlink_platform_boot_remote(int interface, const char *dev_name,
		const char *bin_path);

int xlink_platform_reset_remote(int interface, void *device, void *channel);

void* xlink_platform_allocate(struct device *dev, dma_addr_t *handle,
		uint32_t size, uint32_t alignment);

void xlink_platform_deallocate(struct device *dev, void *buf,
		dma_addr_t handle, uint32_t size, uint32_t alignment);

#endif /* __XLINK_PLATFORM_H */
