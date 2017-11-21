/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2015 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * BSD LICENSE
 *
 * Copyright (c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Intel(R) Omni-Path User RDMA Driver
 */

#include <linux/device.h>
#include <linux/idr.h>
#include <linux/module.h>
#include <linux/slab.h>
#include "device.h"

static void hfi_misc_add(struct ib_device *ibdev);
static void hfi_misc_remove(struct ib_device *device, void *client_data);

static struct ib_client hfi_misc = {
	.name = KBUILD_MODNAME,
	.add = hfi_misc_add,
	.remove = hfi_misc_remove,
};

static void hfi_event_notify(struct ib_event_handler *eh, struct ib_event *e)
{
	struct ib_device *ibdev = e->device;

	/* FXRTODO: Add event handling */
	dev_dbg(&ibdev->dev, "%s port %d event %d\n", __func__,
		e->element.port_num, e->event);
}

/*
 * Device initialization, called by IB core when a OPA device is discovered
 */
static void hfi_misc_add(struct ib_device *ibdev)
{
	int ret;
	struct hfi_info *hi;

	/*
	 * ignore IB devices except hfi2 devices
	 * TODO - revisit if still needed after extended Verbs integration
	 */
	if (!(ibdev->attrs.vendor_id == PCI_VENDOR_ID_INTEL &&
	      ibdev->attrs.vendor_part_id == PCI_DEVICE_ID_INTEL_FXR0))
		return;

	hi = kzalloc(sizeof(*hi), GFP_KERNEL);
	if (!hi) {
		ret = -ENOMEM;
		goto exit;
	}
	hi->ibdev = ibdev;

	INIT_IB_EVENT_HANDLER(&hi->eh, ibdev, hfi_event_notify);
	ret = ib_register_event_handler(&hi->eh);
	if (ret)
		goto eh_error;

	ret = hfi_user_add(hi);
	if (ret)
		goto add_error;

	ib_set_client_data(ibdev, &hfi_misc, hi);
	return;

add_error:
	ib_unregister_event_handler(&hi->eh);
eh_error:
	kfree(hi);
exit:
	dev_err(&ibdev->dev, "Failed to create /dev devices: %d\n", ret);
}

/*
 * Perform required device shutdown logic, also remove /dev entries.
 * Called by IB core when a OPA device is removed
 */
static void hfi_misc_remove(struct ib_device *device, void *client_data)
{
	struct hfi_info *hi = client_data;

	if (!hi)
		return;
	hfi_user_remove(hi);
	ib_unregister_event_handler(&hi->eh);
	kfree(hi);
}

static int __init hfi_init(void)
{
	return ib_register_client(&hfi_misc);
}
module_init(hfi_init);

static void hfi_cleanup(void)
{
	ib_unregister_client(&hfi_misc);
}
module_exit(hfi_cleanup);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Intel Corporation");
MODULE_DESCRIPTION("Intel(R) Omni-Path User RDMA Driver");
