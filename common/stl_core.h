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

#ifndef _STL_CORE_H_
#define _STL_CORE_H_
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
 * @dd: device specific information
 */
struct stl_core_device {
	struct stl_core_ops *bus_ops;
	struct stl_core_device_id id;
	struct device dev;
	int index;
	struct hfi_devdata *dd;
};

/**
 * stl_core_client - representation of a stl_core client
 *
 * @name: STL client name
 * @add: the function to call when a device is discovered
 * @remove: the function to call when a device is removed
 * @si: underlying subsystem interface (filled in by stl_core)
 */
struct stl_core_client {
	const char *name;
	int (*add)(struct stl_core_device *dev);
	void (*remove)(struct stl_core_device *dev);
	struct subsys_interface si;
};

struct hfi_devdata;
struct stl_core_device *
stl_core_register_device(struct device *dev, struct stl_core_device_id *id,
			struct hfi_devdata *dd, struct stl_core_ops *bus_ops);
void stl_core_unregister_device(struct stl_core_device *hfi_dev);

int stl_core_client_register(struct stl_core_client *drv);
void stl_core_client_unregister(struct stl_core_client *drv);

static inline struct stl_core_device *dev_to_stl_core(struct device *dev)
{
	return container_of(dev, struct stl_core_device, dev);
}
#endif /* _STL_CORE_H_ */
