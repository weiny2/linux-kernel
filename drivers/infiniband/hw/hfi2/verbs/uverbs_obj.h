/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2017 Intel Corporation.
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
 * Copyright (c) 2017 Intel Corporation.
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
#if 0
#include <rdma/uverbs_ioctl.h>
#include <rdma/uverbs_std_types.h>
#include "../core/hfi_cmd.h"

#define HFI_MMAP_PSB_TOKEN(type, ptl_ctxt, size)  \
	HFI_MMAP_TOKEN((type), ptl_ctxt, 0, size)

/* Single attribute per action */
#define HFI2_E2E_CONN_ATTR		0
#define HFI2_SL_PAIR_ATTR		0
#define HFI2_CTX_DETACH_IDX		0
#define HFI2_RELEASE_CMDQ_IDX		0
#define HFI2_JOB_INFO_RESP		0
#define HFI2_DLID_RELEASE_CTX_IDX	0
#define HFI2_GET_HW_LIMITS_RESP		0

extern const struct uverbs_type hfi2_type_device;
extern const struct uverbs_type hfi2_type_ctx;
extern const struct uverbs_type hfi2_type_cmdq;
extern const struct uverbs_type hfi2_type_job;

extern const struct uverbs_type_group hfi2_object_types;

extern const struct uverbs_spec_root hfi2_root;

struct hfi2_cmdq_auth_table {
	struct hfi_auth_tuple auth_table[HFI_NUM_AUTH_TUPLES];
};

/*
 * This is a temporary addition till we figure out the best place to add
 * it to.
 * Currently, it is defined in core/uverbs.h but can be moved to
 * include/rdma/
 */
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

struct ib_ucmdq_object {
	struct ib_uobject uobject;
	struct ib_uverbs_file *verbs_file;
};

struct ib_uctx_object {
	struct ib_uobject uobject;
	struct ib_uverbs_file *verbs_file;
};

struct ib_ujob_object {
	struct ib_uobject uobject;
	struct ib_uverbs_file *verbs_file;
	struct list_head obj_list;
	u16 job_res_mode;
	u16 sid;
};

struct hfi_ctx_attach_cmd {
	u16 pid;
	u16 flags;
	u16 le_me_count;
	u16 unexpected_count;
	u16 trig_op_count;
};

struct hfi_ctx_attach_resp {
	u16 pid;
	u16 pid_base;
	u16 pid_count;
	u16 pid_mode;
	u32 uid;
	u64 ct_token;
	u64 eq_desc_token;
	u64 eq_head_token;
	u64 pt_token;
	u64 le_me_token;
	u64 le_me_unlink_token;
	u64 unexpected_token;
	u64 trig_op_token;
};

enum hfi2_types_groups {
	UVERBS_COMMON_TYPES,
	HFI2_OBJECT_TYPES,
};

enum hfi2_obj_types {
	HFI2_TYPE_DEVICE,
	HFI2_TYPE_CMDQ,
	HFI2_TYPE_CTX,
	HFI2_TYPE_JOB,
};

/* Attributes */

/* ctx attach */
enum {
	HFI2_CTX_ATTACH_IDX,
	HFI2_CTX_ATTACH_CMD,
	HFI2_CTX_ATTACH_RESP,
	HFI2_CTX_PTL_UID,
};

/* ctx event */
enum {
	HFI2_CTX_EVT_CTX_IDX,
	HFI2_CTX_EVT_CMD,
	HFI2_CTX_EVT_RESP,
	HFI2_CTX_EVT_TYPE,
};

/* cmdq assign */
enum {
	HFI2_ASSIGN_CMDQ_IDX,
	HFI2_ASSIGN_CMDQ_CTX_IDX,
	HFI2_ASSIGN_CMDQ_AUTH_TABLE,
	HFI2_ASSIGN_CMDQ_TX_TOKEN,
	HFI2_ASSIGN_CMDQ_RX_TOKEN,
	HFI2_ASSIGN_CMDQ_HEAD_TOKEN,
};

/* cmdq update */
enum {
	HFI2_UPDATE_CMDQ_IDX,
	HFI2_UPDATE_CMDQ_AUTH_TABLE,
};

/* Job setup */
enum {
	HFI2_JOB_SETUP_CTX_IDX,
	HFI2_JOB_SETUP_CMD,
	HFI2_JOB_SETUP_PID_BASE,
};

/* DLID Assign */
enum {
	HFI2_DLID_ASSIGN_CTX_IDX,
	HFI2_DLID_ASSIGN_CMD,
};

/* PT Update lower */
enum {
	HFI2_PT_UPDATE_CTX_IDX,
	HFI2_PT_UPDATE_LOWER_ARGS,
};

/* TS Master */
enum {
	HFI2_GET_TS_MASTER_CMD,
	HFI2_GET_TS_MASTER_RESP,
};

/* TS FM */
enum {
	HFI2_GET_TS_FM_CMD,
	HFI2_GET_TS_FM_RESP,
};

/* Actions */
enum {
	HFI2_CMDQ_ASSIGN,
	HFI2_CMDQ_RELEASE,
	HFI2_CMDQ_UPDATE,
};

enum {
	HFI2_CTX_ATTACH,
	HFI2_CTX_DETACH,
	HFI2_CTX_EVENT_CMD,
	HFI2_PT_UPDATE_LOWER,
};

enum {
	HFI2_JOB_SETUP,
	HFI2_JOB_INFO,
	HFI2_DLID_ASSIGN,
	HFI2_DLID_RELEASE,
};

enum {
	HFI2_CMD_EQ_ASSIGN,
	HFI2_CMD_EQ_RELEASE,
	HFI2_CMD_EC_WAIT_EQ,
	HFI2_CMD_EC_SET_EQ,
	HFI2_CMD_EQ_WAIT,
	HFI2_CMD_EQ_ACK,
	HFI2_CMD_EC_ASSIGN,
	HFI2_CMD_EC_RELEASE,
	HFI2_CMD_CT_ASSIGN,
	HFI2_CMD_CT_RELEASE,
	HFI2_CMD_EC_WAIT_CT,
	HFI2_CMD_EC_SET_CT,
	HFI2_CMD_CT_WAIT,
	HFI2_CMD_CT_ACK,
};

enum {
	HFI2_DEV_RESERVED_COMPAT, /* TODO - placeholder for query/compat */
	HFI2_DEV_E2E_CONNECT,
	HFI2_DEV_CHK_SL_PAIR,
	HFI2_DEV_GET_HW_LIMITS,
	HFI2_DEV_TS_GET_MASTER,
	HFI2_DEV_TS_GET_FM,
};

int hfi_job_init(struct hfi_ctx *ctx);
void uverbs_initialize_type_group(const struct uverbs_type_group *type_group);

int hfi2_ctx_free(struct ib_uobject *uobject,
		  enum rdma_remove_reason why);

int hfi2_free_cmdq(struct ib_uobject *uobject,
		   enum rdma_remove_reason why);

int hfi2_job_free(struct ib_uobject *uobject,
		  enum rdma_remove_reason why);

int hfi2_cmdq_assign(struct ib_device *ib_dev,
		     struct ib_uverbs_file *file,
		     struct uverbs_attr_array *attrs,
		     size_t num);

int hfi2_cmdq_update(struct ib_device *ib_dev,
		     struct ib_uverbs_file *file,
		     struct uverbs_attr_array *attrs,
		     size_t num);

int hfi2_cmdq_release(struct ib_device *ib_dev,
		      struct ib_uverbs_file *file,
		      struct uverbs_attr_array *attrs,
		      size_t num);

int hfi2_ctx_attach(struct ib_device *ib_dev,
		    struct ib_uverbs_file *file,
		    struct uverbs_attr_array *attrs,
		    size_t num);

int hfi2_ctx_event_cmd_handler(struct ib_device *ib_dev,
			       struct ib_uverbs_file *file,
			       struct uverbs_attr_array *attrs,
			       size_t num);

int hfi2_pt_update_lower(struct ib_device *ib_dev,
			       struct ib_uverbs_file *file,
			       struct uverbs_attr_array *attrs,
			       size_t num);

int hfi2_ctx_detach(struct ib_device *ib_dev,
		    struct ib_uverbs_file *file,
		    struct uverbs_attr_array *attrs,
		    size_t num);

int hfi2_job_setup(struct ib_device *ib_dev,
		   struct ib_uverbs_file *file,
		   struct uverbs_attr_array *attrs,
		   size_t num);

int hfi2_job_info(struct ib_device *ib_dev,
		  struct ib_uverbs_file *file,
		  struct uverbs_attr_array *attrs,
		  size_t num);

int hfi2_dlid_assign(struct ib_device *ib_dev,
		     struct ib_uverbs_file *file,
		     struct uverbs_attr_array *attrs,
		     size_t num);

int hfi2_dlid_release(struct ib_device *ib_dev,
		      struct ib_uverbs_file *file,
		      struct uverbs_attr_array *attrs,
		      size_t num);

int hfi2_mmap(struct ib_ucontext *context, struct vm_area_struct *vma);
#endif
#endif
