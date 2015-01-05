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

#ifndef _OPA_USER_H
#define _OPA_USER_H

#include <linux/list.h>
#include <linux/slab.h>
#include <rdma/hfi_cmd.h>
#include <rdma/opa_core.h>

#define HFI_MMAP_PSB_TOKEN(type, ptl_ctxt, size)  \
	HFI_MMAP_TOKEN((type), ptl_ctxt, 0, size)

/* Private data for file operations, created at open(). */
struct hfi_userdata {
	struct opa_core_ops *bus_ops;
	pid_t pid;
	pid_t sid;
	/* Per PID Portals State */
	struct hfi_ctx ctx;
	u16 job_res_mode;
	struct list_head job_list;
	struct list_head mpin_head;
	spinlock_t mpin_lock;
};

void hfi_job_init(struct hfi_userdata *ud);
int hfi_job_info(struct hfi_userdata *ud, struct hfi_job_info_args *job_info);
int hfi_job_setup(struct hfi_userdata *ud, struct hfi_job_setup_args *job_setup);
void hfi_job_free(struct hfi_userdata *ud);
int hfi_mpin(struct hfi_userdata *ud, struct hfi_mpin_args *mpin);
int hfi_munpin(struct hfi_userdata *ud, struct hfi_munpin_args *munpin);
int hfi_munpin_all(struct hfi_userdata *ud);

/* printk wrappers (pr_warn, etc) can also be used for general debugging. */
#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#endif /* _OPA_USER_H */
