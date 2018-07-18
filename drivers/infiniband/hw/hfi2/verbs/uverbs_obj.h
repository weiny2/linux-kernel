/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2017-2018 Intel Corporation.
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
 * Copyright (c) 2017-2018 Intel Corporation.
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
 * Intel(R) OPA Gen2 IB Driver
 */
#ifndef _UVERBS_OBJ_H_
#define _UVERBS_OBJ_H_

#include <rdma/uverbs_ioctl.h>
#include <rdma/uverbs_std_types.h>
#include <uapi/rdma/hfi/hfi2_ioctl.h>
#include "../core/hfi_core.h"

#define HFI_MMAP_PSB_TOKEN(type, ptl_ctxt, size)  \
	HFI_MMAP_TOKEN((type), ptl_ctxt, 0, size)

/*
 * ABI of each HFI2 object must be incremented when user<->kernel ABI
 * is changed in a way that is not backwards compatible
 */
#define HFI2_OBJECT_DEVICE_ABI_VERSION	1
#define HFI2_OBJECT_CMDQ_ABI_VERSION	1
#define HFI2_OBJECT_CTX_ABI_VERSION	1
#define HFI2_OBJECT_JOB_ABI_VERSION	1

extern const struct uverbs_object_def hfi2_object_device;
extern const struct uverbs_object_def hfi2_object_ctx;
extern const struct uverbs_object_def hfi2_object_cmdq;
extern const struct uverbs_object_def hfi2_object_job;

extern const struct uverbs_object_tree_def hfi2_uverbs_objects;
extern const struct uverbs_type_group hfi2_object_types;
extern const struct uverbs_spec_root hfi2_root;

struct ib_uverbs_file {
	struct kref				ref;
	struct mutex				mutex;
	struct mutex                            cleanup_mutex;
	struct ib_uverbs_device		       *device;
	struct ib_ucontext		       *ucontext;
	struct ib_event_handler			event_handler;
	struct ib_uverbs_async_event_file       *async_file;
	struct list_head			list;
	int					is_closed;
	struct idr		idr;
	/* spinlock protects write access to idr */
	spinlock_t		idr_lock;
};

struct ib_uctx_object {
	struct ib_uobject uobject;
	struct ib_uverbs_file *verbs_file;
};

struct ib_ucmdq_object {
	struct ib_uobject uobject;
	struct ib_uverbs_file *verbs_file;
};

struct ib_ujob_object {
	struct ib_uobject uobject;
	struct ib_uverbs_file *verbs_file;
	struct list_head obj_list;
	u16 sid;
	u16 job_res_mode;
	u64 job_res_cookie;
};

bool hfi_job_init(struct hfi_ctx *ctx, u16 res_mode, u64 cookie);
int hfi2_mmap(struct ib_ucontext *context, struct vm_area_struct *vma);
void hfi_zap_vma_list(struct hfi_ibcontext *context, uint16_t cmdq_idx);
#endif
