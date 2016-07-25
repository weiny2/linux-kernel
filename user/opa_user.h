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

#ifndef _OPA_USER_H
#define _OPA_USER_H

#include <linux/list.h>
#include <linux/slab.h>
#include <rdma/hfi_cmd.h>
#include <rdma/opa_core.h>

#define HFI_MMAP_PSB_TOKEN(type, ptl_ctxt, size)  \
	HFI_MMAP_TOKEN((type), ptl_ctxt, 0, size)

/* List of vma pointers to zap on release */
struct hfi_vma {
	struct vm_area_struct *vma;
	u16 cq_idx;
	struct list_head vma_list;
};

/* Private data for file operations, created at open(). */
struct hfi_userdata {
	struct opa_core_device *odev;
	struct opa_core_ops *bus_ops;
	pid_t pid;
	pid_t sid;
	/* Per PID Portals State */
	struct hfi_ctx ctx;
	u16 job_res_mode;
	struct list_head job_list;
	struct list_head mpin_head;
	spinlock_t mpin_lock;
	/* List of vma's to zap on release */
	struct list_head vma_head;
	/* Lock for updating vma_head listt */
	spinlock_t vma_lock;
};

void hfi_job_init(struct hfi_userdata *ud);
int hfi_job_info(struct hfi_userdata *ud, struct hfi_job_info *job_info);
int hfi_job_setup(struct hfi_userdata *ud, struct hfi_job_setup_args *job_setup);
void hfi_job_free(struct hfi_userdata *ud);
int hfi_mpin(struct hfi_userdata *ud, struct hfi_mpin_args *mpin);
int hfi_munpin(struct hfi_userdata *ud, struct hfi_munpin_args *munpin);
int hfi_munpin_all(struct hfi_userdata *ud);

/* printk wrappers (pr_warn, etc) can also be used for general debugging. */
#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#endif /* _OPA_USER_H */
