/*
 * Copyright(c) 2014 Intel Corporation.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _HFI_BUS_H_
#define _HFI_BUS_H_
/*
 * Everything a stl_core driver needs to work with any particular stl_core
 * implementation.
 */
#include "hfi.h"

struct stl_core_device_id {
	__u32 vendor;
	__u32 device;
};

/**
 * stl_core_device - representation of a device using stl_core
 * @bus_ops: the hardware ops supported by this device.
 * @id: the device type identification (used to match it with a driver).
 * @dev: underlying device.
 * @index: unique position on the stl_core bus
 */
struct stl_core_device {
	struct stl_core_ops *bus_ops;
	struct stl_core_device_id id;
	struct device dev;
	int index;
	int unit;
	struct hfi_devdata *dd;
};

/**
 * stl_core_driver - operations for a stl_core I/O driver
 * @driver: underlying device driver (populate name and owner).
 * @id_table: the ids serviced by this driver.
 * @probe: the function to call when a device is found.  Returns 0 or -errno.
 * @remove: the function to call when a device is removed.
 */
struct stl_core_driver {
	struct device_driver driver;
	const struct stl_core_device_id *id_table;
	/* TODO - can derive from driver(?) - yes */
	struct stl_core_device *bus_dev;
	int (*probe)(struct stl_core_device *dev);
	void (*scan)(struct stl_core_device *dev);
	void (*remove)(struct stl_core_device *dev);
	/* for character devices */
	struct class *class;
	struct cdev cdev;
	struct device *dev;
};

struct stl_core_device *
stl_core_register_device(struct device *dev, struct stl_core_device_id *id,
			struct hfi_devdata *dd, struct stl_core_ops *bus_ops);
void stl_core_unregister_device(struct stl_core_device *hfi_dev);

int stl_core_register_driver(struct stl_core_driver *drv);
void stl_core_unregister_driver(struct stl_core_driver *drv);

static inline struct stl_core_device *dev_to_stl_core(struct device *dev)
{
	return container_of(dev, struct stl_core_device, dev);
}

static inline struct stl_core_driver *drv_to_stl_core(struct device_driver *drv)
{
	return container_of(drv, struct stl_core_driver, driver);
}

#endif /* _HFI_BUS_H */
