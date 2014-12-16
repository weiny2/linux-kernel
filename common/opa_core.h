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

#ifndef _OPA_CORE_H_
#define _OPA_CORE_H_
/*
 * Everything a opa_core driver needs to work with any particular opa_core
 * implementation.
 */
#include "opa.h"

struct opa_core_device_id {
	__u32 vendor;
	__u32 device;
};

/**
 * opa_core_device - representation of a device using opa_core
 * @bus_ops: the hardware ops supported by this device.
 * @id: the device type identification (used to match it with a driver).
 * @dev: underlying device.
 * @index: unique position on the opa_core bus
 * @dd: device specific information
 */
struct opa_core_device {
	struct opa_core_ops *bus_ops;
	struct opa_core_device_id id;
	struct device dev;
	int index;
	struct hfi_devdata *dd;
};

/**
 * opa_core_client - representation of a opa_core client
 *
 * @name: STL client name
 * @add: the function to call when a device is discovered
 * @remove: the function to call when a device is removed
 * @si: underlying subsystem interface (filled in by opa_core)
 */
struct opa_core_client {
	const char *name;
	int (*add)(struct opa_core_device *odev);
	void (*remove)(struct opa_core_device *odev);
	struct subsys_interface si;
};

struct hfi_devdata;
struct opa_core_device *
opa_core_register_device(struct device *dev, struct opa_core_device_id *id,
			struct hfi_devdata *dd, struct opa_core_ops *bus_ops);
void opa_core_unregister_device(struct opa_core_device *odev);

int opa_core_client_register(struct opa_core_client *client);
void opa_core_client_unregister(struct opa_core_client *client);

static inline struct opa_core_device *dev_to_opa_core(struct device *dev)
{
	return container_of(dev, struct opa_core_device, dev);
}
#endif /* _OPA_CORE_H_ */
