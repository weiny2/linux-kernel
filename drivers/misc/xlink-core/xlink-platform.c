/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * xlink Linux Kernel Platform API
 *
 * Copyright (C) 2018-2019 Intel Corporation
 *
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/xlink_drv_inf.h>

#ifdef CONFIG_XLINK_LOCAL_HOST
#include <linux/keembay-vpu-ipc.h>
#endif

#include "xlink-platform.h"

/*
 * IPC driver interface functions
 */

static int xlink_ipc_write(void *fd, void *data, size_t * const size, unsigned int timeout)
{
	int rc = 0;
#ifdef CONFIG_XLINK_LOCAL_HOST
	struct xlink_ipc_fd *ipc = (struct xlink_ipc_fd *)fd;
	rc = intel_keembay_vpu_ipc_send(NULL, ipc->node, ipc->chan,
			*(uint32_t *)data, *size);
#endif
	return rc;
}

static int xlink_ipc_read(void *fd, void *data, size_t * const size, unsigned int timeout)
{
	int rc = 0;
#ifdef CONFIG_XLINK_LOCAL_HOST
	int addr = 0;
	struct xlink_ipc_fd *ipc = (struct xlink_ipc_fd *)fd;
	rc = intel_keembay_vpu_ipc_recv(NULL, ipc->node, ipc->chan, &addr,
			size, timeout);
	*(uint32_t*)data = addr;
#endif
	return rc;
}

static int xlink_ipc_connect(const char *device_name, void **fd)
{
	/* To be implemented */
	return 0;
}

/*
 * xlink low-level driver interface arrays
 *
 * note: array indices based on xlink_dev_type enum definition
 */

int (*connect_fcts[NMB_OF_DEVICE_TYPES])(const char *, void **) = \
		{NULL, xlink_pcie_connect, NULL, NULL, xlink_ipc_connect};
int (*write_fcts[NMB_OF_DEVICE_TYPES])(void *, void *, size_t * const, uint32_t) = \
		{NULL, xlink_pcie_write, NULL, NULL, xlink_ipc_write};
int (*read_fcts[NMB_OF_DEVICE_TYPES])(void *, void *, size_t * const, uint32_t) = \
		{NULL, xlink_pcie_read, NULL, NULL, xlink_ipc_read};
int (*reset_fcts[NMB_OF_DEVICE_TYPES])(void *, uint32_t) = \
		{NULL, xlink_pcie_reset_device, NULL, NULL, NULL};
int (*boot_fcts[NMB_OF_DEVICE_TYPES])(const char *, const char *) = \
		{NULL, xlink_pcie_boot_remote, NULL, NULL, NULL};
int (*dev_name_fcts[NMB_OF_DEVICE_TYPES])(uint32_t, char *, size_t) = \
		{NULL, xlink_pcie_get_device_name, NULL, NULL, NULL};
int (*dev_list_fcts[NMB_OF_DEVICE_TYPES])(uint32_t *, uint32_t *, int) = \
		{NULL, xlink_pcie_get_device_list, NULL, NULL, NULL};
int (*dev_status_fcts[NMB_OF_DEVICE_TYPES])(const char *, uint32_t *) = \
		{NULL, xlink_pcie_get_device_status, NULL, NULL, NULL};

/*
 * xlink low-level driver interface
 */

int xlink_platform_connect(uint32_t interface, const char *device_name,
		void **fd)
{
	if (interface >= NMB_OF_DEVICE_TYPES || !connect_fcts[interface])
		return -1;
	return connect_fcts[interface](device_name, fd);
}

int xlink_platform_write(uint32_t interface, void *fd, void *data,
		size_t * const size, uint32_t timeout)
{
	if (interface >= NMB_OF_DEVICE_TYPES || !write_fcts[interface])
		return -1;

	return write_fcts[interface](fd, data, size, timeout);
}

int xlink_platform_read(uint32_t interface, void *fd, void *data,
		size_t * const size, uint32_t timeout)
{
	if (interface >= NMB_OF_DEVICE_TYPES || !read_fcts[interface])
		return -1;

	return read_fcts[interface](fd, data, size, timeout);
}

int xlink_platform_reset_device(void *fd, uint32_t operating_frequency)
{
	int interface = PCIE_DEVICE; // TODO: hard code interface for now

	if (interface >= NMB_OF_DEVICE_TYPES || !reset_fcts[interface])
		return -1;

	return reset_fcts[interface](fd, operating_frequency);
}

int xlink_platform_boot_device(const char *device_name,
		const char *binary_path)
{
	int interface = PCIE_DEVICE; // TODO: hard code interface for now

	if (interface >= NMB_OF_DEVICE_TYPES || !boot_fcts[interface])
		return -1;

	return boot_fcts[interface](device_name, binary_path);
}

int xlink_platform_get_device_name(uint32_t sw_device_id, char *device_name,
		size_t name_size)
{
	int interface = PCIE_DEVICE; // TODO: hard code interface for now

	if (interface >= NMB_OF_DEVICE_TYPES || !dev_name_fcts[interface])
		return -1;

	return dev_name_fcts[interface](sw_device_id, device_name, name_size);
}

int xlink_platform_get_device_list(uint32_t *sw_device_id_list,
		uint32_t *num_devices, int pid)
{
	int interface = PCIE_DEVICE; // TODO: hard code interface for now

	if (interface >= NMB_OF_DEVICE_TYPES || !dev_list_fcts[interface])
		return -1;

	return dev_list_fcts[interface](sw_device_id_list, num_devices, pid);
}

int xlink_platform_get_device_status(const char *device_name,
		uint32_t *device_status)
{
	int interface = PCIE_DEVICE; // TODO: hard code interface for now

	if (interface >= NMB_OF_DEVICE_TYPES || !dev_status_fcts[interface])
		return -1;

	return dev_status_fcts[interface](device_name, device_status);
}

void * xlink_platform_allocate(struct device *dev, dma_addr_t *handle,
		uint32_t size, uint32_t alignment)
{
// #ifdef CONFIG_XLINK_LOCAL_HOST
	// return dma_alloc_coherent(dev, size, handle, GFP_KERNEL);
// #else
	return kzalloc(size, GFP_KERNEL);
// #endif
}

void xlink_platform_deallocate(struct device *dev, void *buf,
		dma_addr_t handle, uint32_t size, uint32_t alignment)
{
// #ifdef CONFIG_XLINK_LOCAL_HOST
	// dma_free_coherent(dev, size, buf, handle);
// #else
	if (buf) {
		kfree(buf);
	}
// #endif
}
