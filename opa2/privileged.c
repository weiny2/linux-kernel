/*
 * Copyright (c) 2014 Intel Corporation.  All rights reserved.
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

#include <linux/sched.h>
#include "opa_hfi.h"

int hfi_dlid_assign(struct hfi_ctx *ctx, struct hfi_dlid_assign_args *dlid_assign)
{
	struct hfi_devdata *dd = ctx->devdata;
	void *cq_base;

	BUG_ON(!capable(CAP_SYS_ADMIN));

	/* can only write DLID mapping once */
	if (ctx->dlid_base != HFI_LID_ANY)
		return -EPERM;

	/* TODO - write DLID table via TX CQ */
	cq_base = dd->cq_tx_base;

	ctx->dlid_base = dlid_assign->dlid_base;
	return 0;
}

int hfi_dlid_release(struct hfi_ctx *ctx)
{
	BUG_ON(!capable(CAP_SYS_ADMIN));

	ctx->dlid_base = HFI_LID_ANY;
	/* TODO - write DLID table via TX CQ */
	return 0;
}
