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
 * Everything a hfi_bus driver needs to work with any particular hfi_bus
 * implementation.
 */
#include "hfi.h"

struct hfi_bus_device_id {
	__u32 vendor;
	__u32 device;
};

/**
 * hfi_bus_device - representation of a device using hfi_bus
 * @bus_ops: the hardware ops supported by this device.
 * @id: the device type identification (used to match it with a driver).
 * @dev: underlying device.
 * @index: unique position on the hfi_bus bus
 */
struct hfi_bus_device {
	struct hfi_bus_ops *bus_ops;
	struct hfi_bus_device_id id;
	struct device dev;
	int index;
	int unit;
	struct hfi_devdata *dd;
};

/**
 * hfi_bus_driver - operations for a hfi_bus I/O driver
 * @driver: underlying device driver (populate name and owner).
 * @id_table: the ids serviced by this driver.
 * @probe: the function to call when a device is found.  Returns 0 or -errno.
 * @remove: the function to call when a device is removed.
 */
struct hfi_bus_driver {
	struct device_driver driver;
	const struct hfi_bus_device_id *id_table;
	/* TODO - can derive from driver(?) - yes */
	struct hfi_bus_device *bus_dev;
	int (*probe)(struct hfi_bus_device *dev);
	void (*scan)(struct hfi_bus_device *dev);
	void (*remove)(struct hfi_bus_device *dev);
	/* for character devices */
	struct class *class;
	struct cdev cdev;
	struct device *dev;
};

struct hfi_bus_device *
hfi_bus_register_device(struct device *dev, struct hfi_bus_device_id *id,
			struct hfi_devdata *dd, struct hfi_bus_ops *bus_ops);
void hfi_bus_unregister_device(struct hfi_bus_device *hfi_dev);

int hfi_bus_register_driver(struct hfi_bus_driver *drv);
void hfi_bus_unregister_driver(struct hfi_bus_driver *drv);

static inline struct hfi_bus_device *dev_to_hfi_bus(struct device *dev)
{
	return container_of(dev, struct hfi_bus_device, dev);
}

static inline struct hfi_bus_driver *drv_to_hfi_bus(struct device_driver *drv)
{
	return container_of(drv, struct hfi_bus_driver, driver);
}

#endif /* _HFI_BUS_H */
