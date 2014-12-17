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

#ifndef _OPA_COMMON_H
#define _OPA_COMMON_H

/*
 * This header file contains shared structures between OPA clients and devices.
 */

#include <linux/slab.h>
#include "../include/hfi_cmd.h"
#include "opa_core.h"

struct hfi_devdata;

/* Private data for file operations, created at open(). */
struct hfi_userdata {
	struct hfi_info *hi;
	struct opa_core_ops *bus_ops;
	struct hfi_devdata *devdata;
	pid_t pid;
	pid_t sid;

	/* Per PID Portals State */
	void *ptl_state_base;
	void *ptl_le_me_base;
	u32 ptl_state_size;
	u32 ptl_le_me_size;
	u32 ptl_unexpected_size;
	u32 ptl_trig_op_size;
	u16 ptl_pid;
	u16 pasid;
	u16 cq_pair_num_assigned;
	u8 allow_phys_dlid;
	u8 auth_mask;
	u32 auth_uid[HFI_NUM_AUTH_TUPLES];
	u32 ptl_uid; /* default UID if auth_tuples not used */
	struct list_head mpin_head;
	spinlock_t mpin_lock;

	u32 dlid_base;
	u32 lid_offset;
	u32 lid_count;
	u16 pid_base;
	u16 pid_count;
	u16 pid_mode;
	u16 res_mode;
	u32 sl_mask;
	struct list_head job_list;
};

/**
 * opa_core_ops - Hardware operations for accessing an OPA device on the OPA bus.
 * @ctxt_assign: Assign a Send/Receive context of the HFI.
 * @ctxt_release: Release Send/Receive context in the HFI.
 * @ctxt_reserve: Reserve range of contiguous Send/Receive contexts in the HFI.
 * @ctxt_unreserve: Release reservation from ctxt_reserve.
 * @ctxt_addr: Return address for HFI resources providing memory access/mapping.
 * @cq_assign: Assign a Command Queue for HFI send/receive operations.
 * @cq_update: Update configuration of Command Queue.
 * @cq_release: Release Command Queue.
 * @eq_assign: Assign an Event Completion Queue.
 * @eq_release: Release an Event Completion Queue.
 * @dlid_assign: Assign entries from the DLID relocation table.
 * @dlid_release: Release entries from the DLID relocation table.
 */
struct opa_core_ops {
	/* Resource Allocation ops */
	int (*ctxt_assign)(struct hfi_userdata *ud, struct hfi_ctxt_attach_args *ctxt_attach);
	void (*ctxt_release)(struct hfi_userdata *ud);
	int (*ctxt_reserve)(struct hfi_userdata *ud, u16 *base, u16 count);
	void (*ctxt_unreserve)(struct hfi_userdata *ud, u16 base, u16 count);
	int (*ctxt_addr)(struct hfi_userdata *ud, int type, u16 ctxt, void **addr, ssize_t *len);
	int (*cq_assign)(struct hfi_userdata *ud, struct hfi_cq_assign_args *cq_assign);
	int (*cq_update)(struct hfi_userdata *ud, struct hfi_cq_update_args *cq_update);
	int (*cq_release)(struct hfi_userdata *ud, u16 cq_idx);
	int (*eq_assign)(struct hfi_userdata *ud, struct hfi_eq_assign_args *eq_assign);
	int (*eq_release)(struct hfi_userdata *ud, u16 eq_type, u16 eq_idx);
	int (*dlid_assign)(struct hfi_userdata *ud, struct hfi_dlid_assign_args *dlid_assign);
	int (*dlid_release)(struct hfi_userdata *ud);

#if 0
	/* TODO - below is starting attempt at 'kernel provider' operations */
	/* critical functions would be inlined instead of bus operation */
	/* RX and completion ops */
	int (*rx_alloc)(struct hfi_ctx *ctx, struct hfi_rx_alloc_args *rx_alloc);
	int (*rx_free)(struct hfi_ctx *ctx, u16 rx_type, u32 rx_idx);
	int (*rx_enable)(struct hfi_ctx *ctx, u16 rx_type, u32 rx_idx);
	int (*rx_disable)(struct hfi_ctx *ctx, u16 rx_type, u32 rx_idx);
	int (*eq_alloc)(struct hfi_ctx *ctx, struct hfi_eq_alloc_args *eq_alloc);
	int (*eq_free)(struct hfi_ctx *ctx, u16 eq_type, u16 eq_idx);
	int (*eq_get)(struct hfi_ctx *ctx, u16 eq_type, u16 eq_idx);
	/* TX ops */
	int (*tx_command)(struct hfi_ctx *ctx, u64 cmd, void *header, void *payload);
#endif
};

/* printk wrappers (pr_warn, etc) can also be used for general debugging. */
#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#endif /* _OPA_COMMON_H */
