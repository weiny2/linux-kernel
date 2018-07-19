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

#ifndef _HFI2_IOCTL_H_
#define _HFI2_IOCTL_H_

/* Single attribute per action */
#define HFI2_E2E_CONN_ATTR		0
#define HFI2_SL_PAIR_ATTR		0
#define HFI2_CTX_DETACH_IDX		0
#define HFI2_RELEASE_CMDQ_IDX		0
#define HFI2_JOB_INFO_RESP		0
#define HFI2_DLID_RELEASE_CTX_IDX	0
#define HFI2_GET_HW_LIMITS_RESP		0

/* For command queues */
#define HFI_NUM_AUTH_TUPLES	8

/*
 * struct hfi_auth_tuple - authentication tuples for TX command queue
 * @uid: Portals UID (protection domain) for members of this job
 * @srank: Portals SRANK for use in destination matching criteria
 */
struct hfi_auth_tuple {
	__u32	uid;
	__u32	srank;
};

struct hfi2_cmdq_auth_table {
	struct hfi_auth_tuple auth_table[HFI_NUM_AUTH_TUPLES];
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

enum {
	UVERBS_COMMON_OBJECTS,
	HFI2_OBJECTS,
};

/* Objects */
enum {
	HFI2_OBJECT_DEVICE,
	HFI2_OBJECT_CMDQ,
	HFI2_OBJECT_CTX,
	HFI2_OBJECT_JOB,
};

/* Actions for hfi ctx */
enum {
	HFI2_CTX_ATTACH,
	HFI2_CTX_DETACH,
	HFI2_CTX_EVENT_CMD,
	HFI2_PT_UPDATE_LOWER,
	HFI2_AT_PREFETCH,
};

/* Actions for device object*/
enum {
	HFI2_DEV_RESERVED_COMPAT, /* TODO - placeholder for query/compat */
	HFI2_DEV_E2E_CONNECT,
	HFI2_DEV_CHK_SL_PAIR,
	HFI2_DEV_GET_HW_LIMITS,
	HFI2_DEV_TS_GET_MASTER,
	HFI2_DEV_TS_GET_FM,
	HFI2_DEV_GET_SL_INFO,
};

/* Actions for cmdq */
enum {
	HFI2_CMDQ_ASSIGN,
	HFI2_CMDQ_RELEASE,
	HFI2_CMDQ_UPDATE,
};

/* Actions for hfi job object */
enum {
	HFI2_JOB_SETUP,
	HFI2_JOB_INFO,
	HFI2_DLID_ASSIGN,
	HFI2_DLID_RELEASE,
	HFI2_JOB_ATTACH,
};

/* Attributes for GET SL info */
enum {
	HFI2_SL_INFO_REQ_SL,
	HFI2_SL_INFO_RESP_SL,
	HFI2_SL_INFO_IS_PAIR,
	HFI2_SL_INFO_MTU,
};

/* Attributes for ctx attach */
enum {
	HFI2_CTX_ATTACH_IDX,
	HFI2_CTX_ATTACH_CMD,
	HFI2_CTX_ATTACH_RESP,
	HFI2_CTX_PTL_UID,
};

/* Attributes for ctx event */
enum {
	HFI2_CTX_EVT_CTX_IDX,
	HFI2_CTX_EVT_CMD,
	HFI2_CTX_EVT_RESP,
	HFI2_CTX_EVT_TYPE,
	HFI2_CTX_EVT_CQ_IDX,
	HFI2_CTX_EVT_ARM_DONE,
};

/* Attributes for PT update lower */
enum {
	HFI2_PT_UPDATE_CTX_IDX,
	HFI2_PT_UPDATE_LOWER_ARGS,
};

/* Attributes for cmdq assign */
enum {
	HFI2_ASSIGN_CMDQ_IDX,
	HFI2_ASSIGN_CMDQ_CTX_IDX,
	HFI2_ASSIGN_CMDQ_AUTH_TABLE,
	HFI2_ASSIGN_CMDQ_TX_TOKEN,
	HFI2_ASSIGN_CMDQ_RX_TOKEN,
	HFI2_ASSIGN_CMDQ_HEAD_TOKEN,
};

/* Attributes for cmdq update */
enum {
	HFI2_UPDATE_CMDQ_IDX,
	HFI2_UPDATE_CMDQ_AUTH_TABLE,
};

/* Attributes for TS master */
enum {
	HFI2_GET_TS_MASTER_TIME,
	HFI2_GET_TS_MASTER_TIMESTAMP,
	HFI2_GET_TS_MASTER_CLK_ID,
	HFI2_GET_TS_MASTER_PORT,
};

/* Attributes for TS FM */
enum {
	HFI2_GET_TS_FM_PORT,
	HFI2_GET_TS_FM_HACK,
	HFI2_GET_TS_FM_CLK_OFFSET,
	HFI2_GET_TS_FM_CLK_DELAY,
	HFI2_GET_TS_FM_PERIODICITY,
	HFI2_GET_TS_FM_CURRENT_CLK_ID,
	HFI2_GET_TS_FM_PTP_IDX,
	HFI2_GET_TS_FM_IS_ACTIVE_MASTER,
};

/* Attributes for job setup */
enum {
	HFI2_JOB_SETUP_CTX_IDX,
	HFI2_JOB_SETUP_CMD,
	HFI2_JOB_SETUP_PID_BASE,
};

/* Attributes for job attach */
enum {
	HFI2_JOB_ATTACH_MODE,
	HFI2_JOB_ATTACH_COOKIE,
	HFI2_JOB_ATTACH_RESP,
};

/* Attributes for DLID assign */
enum {
	HFI2_DLID_ASSIGN_CTX_IDX,
	HFI2_DLID_ASSIGN_CMD,
};

/* Attributes for AT Prefetch */
enum {
	HFI2_AT_PREFETCH_CTX_IDX,
	HFI2_AT_PREFETCH_IOVEC,
	HFI2_AT_PREFETCH_COUNT,
	HFI2_AT_PREFETCH_FLAGS,
};

/* Commands */
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
	HFI2_CMD_IB_EQ_ARM,
};

#endif /* _HFI2_IOCTL_H_ */
