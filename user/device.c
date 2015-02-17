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

static struct idr opa_device_tbl;
DEFINE_SPINLOCK(opa_device_lock);

/*
 * Device initialization, called by OPA core when a OPA device is discovered
 */
static int hfi_portals_add(struct opa_core_device *odev)
{
	unsigned long flags;
	int ret;
	struct hfi_info *hi;

	hi = kzalloc(sizeof(*hi), GFP_KERNEL);
	if (!hi) {
		ret = -ENOMEM;
		goto exit;
	}
	hi->odev = odev;

	idr_preload(GFP_KERNEL);
	spin_lock_irqsave(&opa_device_lock, flags);
	ret = idr_alloc(&opa_device_tbl, hi, odev->index, odev->index+1, GFP_NOWAIT);
	if (ret < 0) {
		kfree(hi);
		goto idr_end;
	}

	ret = hfi_user_add(hi);
	if (ret) {
		dev_err(&odev->dev, "Failed to create /dev devices: %d\n", ret);
		idr_remove(&opa_device_tbl, odev->index);
		kfree(hi);
	}

idr_end:
	spin_unlock_irqrestore(&opa_device_lock, flags);
	idr_preload_end();
exit:
	return ret;
}

/*
 * Perform required device shutdown logic, also remove /dev entries.
 * Called by OPA core when a OPA device is removed
 */
static void hfi_portals_remove(struct opa_core_device *odev)
{
	unsigned long flags;
	struct hfi_info *hi;

	spin_lock_irqsave(&opa_device_lock, flags);
	hi = idr_find(&opa_device_tbl, odev->index);
	if (hi) {
		idr_remove(&opa_device_tbl, odev->index);
		hfi_user_remove(hi);
		kfree(hi);
	}
	spin_unlock_irqrestore(&opa_device_lock, flags);
}

static struct opa_core_client hfi_portals = {
	.name = KBUILD_MODNAME,
	.add = hfi_portals_add,
	.remove = hfi_portals_remove,
};

static int __init hfi_init(void)
{
	idr_init(&opa_device_tbl);
	return opa_core_client_register(&hfi_portals);
}
module_init(hfi_init);

static void hfi_cleanup(void)
{
	opa_core_client_unregister(&hfi_portals);
	idr_destroy(&opa_device_tbl);
}
module_exit(hfi_cleanup);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Intel Corporation");
MODULE_DESCRIPTION("Intel(R) Omni-Path User RDMA Driver");
