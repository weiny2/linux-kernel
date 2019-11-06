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
#include <linux/mxlk_interface.h>

#ifdef CONFIG_XLINK_LOCAL_HOST
#include <linux/keembay-vpu-ipc.h>
#endif

#include "xlink-platform.h"

/*
 * IPC Functions
 */

static int ipc_write(void *fd, void *data, size_t size, unsigned int timeout)
{
	int rc = 0;
#ifdef CONFIG_XLINK_LOCAL_HOST
	struct xlink_ipc_fd *ipc = (struct xlink_ipc_fd *)fd;
	rc = intel_keembay_vpu_ipc_send(ipc->node, ipc->chan, *(uint32_t *)data,
			size);
#endif
	return rc;
}

static int ipc_read(void *fd, void *data, size_t *size, unsigned int timeout)
{
	int rc = 0;
#ifdef CONFIG_XLINK_LOCAL_HOST
	int addr = 0;
	struct xlink_ipc_fd *ipc = (struct xlink_ipc_fd *)fd;
	rc = intel_keembay_vpu_ipc_recv(ipc->node, ipc->chan, &addr,
			size, timeout);
	*(uint32_t*)data = addr;
#endif
	return rc;
}

static int ipc_open(const char *dev_path_read, const char *dev_path_write,
		void **fd)
{
	/* To be implemented */
	return 0;
}

static int ipc_close(void *f)
{
	/* To be implemented */
	return 0;
}

static int ipc_devicename(int index, char *name, int nameSize , int pid)
{
	/* To be implemented */
	return 0;
}

/*
 * USB Functions
 */

static int usb_write(void *fd, void*data, size_t size, unsigned int timeout)
{
	/* To be implemented */
	return 0;
}

static int usb_read(void *fd, void *data, size_t *size, unsigned int timeout)
{
	/* To be implemented */
	return 0;
}

static int usb_open(const char *dev_path_read, const char *dev_path_write,
		void **fd)
{
	/* To be implemented */
	return 0;
}

static int usb_close(void *f)
{
	/* To be implemented */
	return 0;
}

static int usb_devicename(int index, char *name, int nameSize , int pid)
{
	/* To be implemented */
	return 0;
}

/*
 * xlink low-level driver interface arrays
 *
 * note: array indices based on xlink_dev_type enum definition
 */
int (*write_fcts[NMB_OF_DEVICE_TYPES])(void*, void*, size_t, unsigned int) = \
		{NULL, keembay_pcie_write, usb_write, NULL, ipc_write};
int (*read_fcts[NMB_OF_DEVICE_TYPES])(void*, void*, size_t*, unsigned int) = \
		{NULL, keembay_pcie_read, usb_read, NULL, ipc_read};
int (*open_fcts[NMB_OF_DEVICE_TYPES])(const char*, const char*, void**) = \
		{NULL, keembay_pcie_connect, usb_open, NULL, ipc_open};
int (*close_fcts[NMB_OF_DEVICE_TYPES])(void*) = \
		{NULL, keembay_pcie_reset_remote, usb_close, NULL, ipc_close};
int (*devicename_fcts[NMB_OF_DEVICE_TYPES])(int, char*, int, int) = \
		{NULL, keembay_pcie_get_devicename, usb_devicename, NULL,
		ipc_devicename};

/*
 * xlink low-level driver interface
 */

int xlink_platform_connect(int interface, const char *dev_path_read,
		const char *dev_path_write, void **fd)
{
	return open_fcts[interface](dev_path_read, dev_path_write, fd);
}

int xlink_platform_write(int interface, void *fd, void *data, size_t size,
		unsigned int timeout)
{
	return write_fcts[interface](fd, data, size, timeout);
}

int xlink_platform_read(int interface, void *fd, void *data, size_t *size,
		unsigned int timeout)
{
	return read_fcts[interface](fd, data, size, timeout);
}

int xlink_platform_reset_remote(int interface, void *device, void *channel)
{
	return close_fcts[interface](device);
}

int xlink_platform_get_device_name(int interface, int index, char *name,
		int name_size)
{
	return devicename_fcts[interface](index, name, name_size, 0);
}

int xlink_platform_get_device_name_extended(int interface, int index,
		char *name, int name_size, int pid)
{
	return devicename_fcts[interface](index, name, name_size, pid);
}

int xlink_platform_boot_remote(int interface, const char *dev_name,
		const char *bin_path)
{
	/* To be implemented */
	return 0;
}

void *xlink_platform_allocate(struct device *dev, dma_addr_t *handle,
		uint32_t size, uint32_t alignment)
{
#ifdef CONFIG_XLINK_LOCAL_HOST
	return dma_alloc_coherent(dev, size, handle, GFP_KERNEL);
#else
	return kzalloc(size, GFP_KERNEL);
#endif
}

void xlink_platform_deallocate(struct device *dev, void *buf, dma_addr_t handle,
		uint32_t size, uint32_t alignment)
{
#ifdef CONFIG_XLINK_LOCAL_HOST
	dma_free_coherent(dev, size, buf, handle);
#else
	if (buf) {
		kfree(buf);
	}
#endif
}
