/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2014 - 2015 Intel Corporation.
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
 * Copyright (c) 2014 - 2015 Intel Corporation.
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
 *
 * Intel(R) Omni-Path User RDMA Driver
 */

#include <linux/device.h>
#include <linux/idr.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <rdma/opa_core.h>
#include "device.h"

static int hfi_misc_add(struct opa_core_device *odev);
static void hfi_misc_remove(struct opa_core_device *odev);
static void hfi_event_notify(struct opa_core_device *odev,
			     enum opa_core_event event, u8 port);

static struct opa_core_client hfi_misc = {
	.name = KBUILD_MODNAME,
	.add = hfi_misc_add,
	.remove = hfi_misc_remove,
	.event_notify = hfi_event_notify
};

static void hfi_event_notify(struct opa_core_device *odev,
			     enum opa_core_event event, u8 port)
{
	/* FXRTODO: Add event handling */
	dev_info(&odev->dev, "%s port %d event %d\n", __func__, port, event);
}

/*
 * Device initialization, called by OPA core when a OPA device is discovered
 */
static int hfi_misc_add(struct opa_core_device *odev)
{
	int ret;
	struct hfi_info *hi;

	hi = kzalloc(sizeof(*hi), GFP_KERNEL);
	if (!hi) {
		ret = -ENOMEM;
		goto exit;
	}
	hi->odev = odev;

	ret = opa_core_set_priv_data(&hfi_misc, odev, hi);
	if (ret)
		goto priv_error;

	ret = hfi_ui_add(hi);
	if (ret)
		goto add_error;

	ret = hfi_user_add(hi);
	if (ret) {
		hfi_ui_remove(hi);
		goto add_error;
	}
	return ret;
add_error:
	opa_core_clear_priv_data(&hfi_misc, odev);
priv_error:
	kfree(hi);
exit:
	dev_err(&odev->dev, "Failed to create /dev devices: %d\n", ret);
	return ret;
}

/*
 * Perform required device shutdown logic, also remove /dev entries.
 * Called by OPA core when a OPA device is removed
 */
static void hfi_misc_remove(struct opa_core_device *odev)
{
	struct hfi_info *hi;

	hi = opa_core_get_priv_data(&hfi_misc, odev);
	if (!hi)
		return;
	hfi_user_remove(hi);
	hfi_ui_remove(hi);
	kfree(hi);
	opa_core_clear_priv_data(&hfi_misc, odev);
}

static int __init hfi_init(void)
{
	return opa_core_client_register(&hfi_misc);
}
module_init(hfi_init);

static void hfi_cleanup(void)
{
	opa_core_client_unregister(&hfi_misc);
}
module_exit(hfi_cleanup);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Intel Corporation");
MODULE_DESCRIPTION("Intel(R) Omni-Path User RDMA Driver");
