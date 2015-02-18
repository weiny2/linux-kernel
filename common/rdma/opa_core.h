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
 * Intel(R) Omni-Path Core Driver Interface
 */

#ifndef _OPA_CORE_H_
#define _OPA_CORE_H_
/*
 * Everything a opa_core driver needs to work with any particular opa_core
 * implementation.
 */
#include <linux/device.h>
#include <rdma/hfi_types.h>

/*
 * HFI API data types
 * TODO - common provider integration
 * Based on hfi_ctx in common provider.
 * hfi_ctx needs to get moved/merged with data types defined by the common
 * provider logic.
 */
struct hfi_devdata;

struct hfi_ctx {
	struct hfi_devdata *devdata;
	u16	ptl_pid;		/* PID associated with this context */
	u32	ptl_uid;		/* UID associated with this context */
	void	*ptl_state_base;
	void    *le_me_addr;
	u32	ptl_state_size;
	u32	le_me_size;		/* ME/LE mmap size */
	u32	unexpected_size;	/* unexpected mmap size */
	u32	trig_op_size;		/* triggered_ops mmap size */
	u8	allow_phys_dlid;
	u8	auth_mask;
	u32	auth_uid[HFI_NUM_AUTH_TUPLES];
	u32	dlid_base;
	u32	lid_count;
	u32	lid_offset;
	u32	sl_mask;
	u16	pid_base;
	u16	pid_count;
	u16	pid_mode;
	u16	cq_pair_num_assigned;
};

/*
 * Types of memory regions mapped into user processes' space
 * Used with ctx_addr operation.
 */
enum mmap_token_types {
	TOK_CONTROL_BLOCK = 1,
	TOK_PORTALS_TABLE,
	TOK_CQ_TX,
	TOK_CQ_RX,
	TOK_CQ_HEAD,
	TOK_EVENTS_CT,
	TOK_EVENTS_EQ_DESC,
	TOK_EVENTS_EQ_HEAD,
	TOK_TRIG_OP,
	TOK_LE_ME,
	TOK_UNEXPECTED,
};

struct opa_ctx_assign {
	u16 pid;
	u16 flags;
	u16 le_me_count;
	u16 unexpected_count;
	u16 trig_op_count;
};

struct hfi_eq_assign_args;
struct hfi_dlid_assign_args;

/**
 * opa_core_ops - Hardware operations for accessing an OPA device on the OPA bus.
 * @ctx_assign: Assign a Send/Receive context of the HFI.
 * @ctx_release: Release Send/Receive context in the HFI.
 * @ctx_reserve: Reserve range of contiguous Send/Receive contexts in the HFI.
 * @ctx_unreserve: Release reservation from ctx_reserve.
 * @ctx_addr: Return address for HFI resources providing memory access/mapping.
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
	int (*ctx_assign)(struct hfi_ctx *ctx, struct opa_ctx_assign *ctx_assign);
	void (*ctx_release)(struct hfi_ctx *ctx);
	int (*ctx_reserve)(struct hfi_ctx *ctx, u16 *base, u16 count);
	void (*ctx_unreserve)(struct hfi_ctx *ctx);
	int (*ctx_addr)(struct hfi_ctx *ctx, int type, u16 ctxt, void **addr,
			ssize_t *len);
	int (*cq_assign)(struct hfi_ctx *ctx, struct hfi_auth_tuple *auth_table, u16 *cq_idx);
	int (*cq_update)(struct hfi_ctx *ctx, u16 cq_idx, struct hfi_auth_tuple *auth_table);
	int (*cq_release)(struct hfi_ctx *ctx, u16 cq_idx);
	int (*eq_assign)(struct hfi_ctx *ctx, struct hfi_eq_assign_args *eq_assign);
	int (*eq_release)(struct hfi_ctx *ctx, u16 eq_type, u16 eq_idx);
	int (*dlid_assign)(struct hfi_ctx *ctx,
			   struct hfi_dlid_assign_args *dlid_assign);
	int (*dlid_release)(struct hfi_ctx *ctx);

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

struct opa_core_device_id {
	__u32 vendor;
	__u32 device;
};

/**
 * opa_core_device - representation of a device using opa_core
 * @bus_ops: the hardware ops supported by this device.
 * @id: the device type identification (used to match it with a driver).
 * @dev: underlying device.
 * @index: unique position on the opa_core bus
 * @dd: device specific information
 */
struct opa_core_device {
	struct opa_core_ops *bus_ops;
	struct opa_core_device_id id;
	struct device dev;
	int index;
	struct hfi_devdata *dd;
};

/**
 * opa_core_client - representation of a opa_core client
 *
 * @name: OPA client name
 * @add: the function to call when a device is discovered
 * @remove: the function to call when a device is removed
 * @si: underlying subsystem interface (filled in by opa_core)
 */
struct opa_core_client {
	const char *name;
	int (*add)(struct opa_core_device *odev);
	void (*remove)(struct opa_core_device *odev);
	struct subsys_interface si;
};

struct hfi_devdata;
struct opa_core_device *
opa_core_register_device(struct device *dev, struct opa_core_device_id *id,
			 struct hfi_devdata *dd, struct opa_core_ops *bus_ops);
void opa_core_unregister_device(struct opa_core_device *odev);

int opa_core_client_register(struct opa_core_client *client);
void opa_core_client_unregister(struct opa_core_client *client);

static inline struct opa_core_device *dev_to_opa_core(struct device *dev)
{
	return container_of(dev, struct opa_core_device, dev);
}
#endif /* _OPA_CORE_H_ */
