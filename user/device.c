/*
 * Copyright (c) 2013 - 2014 Intel Corporation. All rights reserved.
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

#include <linux/device.h>
#include <linux/module.h>
#include <linux/slab.h>
#include "../common/opa_core.h"
#include "device.h"

/*
 * Device initialization, called by OPA core when a OPA device is discovered
 */
static int hfi_portals_add(struct opa_core_device *odev)
{
	int ret;
	struct hfi_info *hi;

	hi = kzalloc(sizeof(*hi), GFP_KERNEL);
	if (!hi) {
		ret = -ENOMEM;
		goto exit;
	}
	hi->odev = odev;
	dev_set_drvdata(&odev->dev, hi);
	ret = hfi_user_add(hi);
	if (ret) {
		dev_err(&odev->dev, "Failed to create /dev devices: %d\n", ret);
		goto kfree;
	}
	return ret;
kfree:
	kfree(hi);
exit:
	return ret;
}

/*
 * Perform required device shutdown logic, also remove /dev entries.
 * Called by OPA core when a OPA device is removed
 */
static void hfi_portals_remove(struct opa_core_device *odev)
{
	struct hfi_info *hi = dev_get_drvdata(&odev->dev);

	hfi_user_remove(hi);
	kfree(hi);
}

static struct opa_core_client hfi_portals = {
	.name = KBUILD_MODNAME,
	.add = hfi_portals_add,
	.remove = hfi_portals_remove,
};

static int __init hfi_init(void)
{
	return opa_core_client_register(&hfi_portals);
}
module_init(hfi_init);

static void hfi_cleanup(void)
{
	opa_core_client_unregister(&hfi_portals);
}
module_exit(hfi_cleanup);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Intel Corporation");
MODULE_DESCRIPTION("Intel(R) Omni-Path Portals Driver");
