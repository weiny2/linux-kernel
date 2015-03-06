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
#include <linux/device.h>
#include <linux/idr.h>
#include <linux/gfp.h>
#include <rdma/hfi_types.h>

/*
 * TODO: These macros are required for compiling against the OPA headers
 * shared with user space. Delete from this file eventually
 */
#define _HFI_STATIC_INLINE static inline
#define _HFI_PRINTF(FMT, args...)
#define _HFI_ASSERT(X)
#define HFI_TEST_ERR(x, y)
#define assert(x)
#define _align64 __aligned(64)

/*
 * TODO - Based on hfi_ctx in common provider. hfi_ctx needs to get moved/merged
 * with data types defined by the common provider logic.
 */
struct hfi_devdata;

/**
 * struct hfi_ctx - state for HFI resources assigned to this context
 * @devdata: HFI device specific data, private to the hardware driver
 * @pid: dummy Portals Process ID. The user space hfi_ctx has this field
 * @fd: dummy file descriptor. The user space hfi_ctx has this field
 * @ptl_pid: Assigned Portals Process ID
 * @ptl_uid: Assigned Protection Domain ID
 * @ptl_state_base: Pointer to Portals state in host memory
 * @le_me_addr: Pointer to head of ME/LE descriptor buffer
 * @ptl_state_size: Size of the total Portals state memory
 * @le_me_size: Size of ME/LE buffer
 * @unexpected_size: Size of unexpected header buffer
 * @trig_op_size: Size of triggered ops buffer
 * @allow_phys_dlid: Physical LIDs allowed in commands (vs virtual LIDs)
 * @auth_mask: Mask of enabled Protection Domains in @auth_uid
 * @auth_uid: Table of allowed Protection Domains for Command Queues
 * @dlid_base: Base virtual DLID (if DLID table is used)
 * @lid_count: Number of LIDs (fabric endpoints)
 * @lid_offset: LID offset from base for this context (if contiguous LIDs)
 * @sl_mask: Mask of allowed service levels for this context
 * @pid_base: Base of contiguous assigned PIDs on this LID
 * @pid_count: Number of PIDs assigned on this LID
 * @pid_mode: Describes if PIDs or LIDs are virtualized or not
 * @cq_pair_num_assigned: Counter of assigned Command Queues
 * @ct_used: IDR table of allocated Event Counters (CTs)
 * @eq_used: IDR table of allocated Events Queues
 * @cteq_lock: Lock for CT and EQ table
 * @ct_addr: Pointer to the counting events array
 * @ct_size: Size of counting events array
 * @pt_addr: Pointer to portal table entries
 * @pt_size: Size of portal table entries
 * @eq_addr: Pointer to the event queue desc table
 * @eq_size: Size of event queue desc table
 * @eq_head_addr: Pointer to the event queue head pointers table
 * @eq_head_size: Size of the event queue head pointers table
 * @status_reg: Status Registers (SR) in each NI
 */
struct hfi_ctx {
	struct hfi_devdata *devdata;
	int	fd;
	u16	pid;
	u16	ptl_pid;
	u32	ptl_uid;
	void	*ptl_state_base;
	void    *le_me_addr;
	u32	ptl_state_size;
	u32	le_me_size;
	u32	unexpected_size;
	u32	trig_op_size;
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
	struct idr ct_used;
	struct idr eq_used;
	spinlock_t cteq_lock;
	void	*ct_addr;
	ssize_t	ct_size;
	void	*pt_addr;
	ssize_t	pt_size;
	void	*eq_addr;
	ssize_t	eq_size;
	void	*eq_head_addr;
	ssize_t	eq_head_size;
	u64	status_reg[HFI_NUM_NIS * HFI_NUM_CT_RESERVED];
};

/**
 * enum mmap_token_types -  Types of memory regions mapped by the context and
 * @TOK_CONTROL_BLOCK: map process control block
 * @TOK_PORTALS_TABLE: map portal table entries
 * @TOK_CQ_TX: map TX command queue
 * @TOK_CQ_RX: map RX command queue
 * @TOK_CQ_HEAD: map command queue head
 * @TOK_EVENTS_CT: map counting events array
 * @TOK_EVENTS_EQ_DESC: map event queue desc array
 * @TOK_EVENTS_EQ_HEAD: map event queue head
 * @TOK_TRIG_OP: map triggered operations desc array
 * @TOK_LE_ME: map array of LE/ME entries
 * @TOK_UNEXPECTED: map array of unexpected headers
 *
 * Used with ctx_addr operation
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

/**
 * struct opa_ctx_assign - Used with opa_core_ops.ctx_assign operation
 * @pid: Requested Portals PID or -1 for driver assignment
 * @flags: Reserved for choosing alternate PID allocation behavior
 * @le_me_count: Requested size of LE/ME buffer
 * @unexpected_count: Requested size of unexpected headers buffer
 * @trig_op_count: Requested size of triggered ops buffer
 * Used with @ctx_assign, assigned PID returned in ctx->ptl_pid
 */
struct opa_ctx_assign {
	u16 pid;
	u16 flags;
	u16 le_me_count;
	u16 unexpected_count;
	u16 trig_op_count;
};

/**
 * struct opa_ev_assign - Used with opa_core_ops.ev_assign operation
 * @ni: Network Interface for operation
 * @mode: Mode bits for EV assignment behavior
 * @base: Base of event buffer in host memory
 * @size: Size specified as number of events
 * @threshold: Num events before blocking EV wakes user
 * @ev_idx: Returns index of EV resource
 */
struct opa_ev_assign {
	u16 ni;
	u16 mode;
	u64 base;
	u64 size;
	u64 threshold;
	u16 ev_idx;
};

/*
 * opa_ev_assign.mode bits, intent is upper bits for specifying EV mechanism,
 * lower bits for controlling behavior
 */
#define OPA_EV_MODE_BLOCKING  0x1
#define OPA_EV_MODE_COUNTER   0x100

struct hfi_dlid_assign_args;

/**
 * struct opa_pport_desc - Used for querying immutable per port
 * opa*_hfi HW  details
 * @devdata: underlying hfi* device structure
 * @pguid: port GUID for this port
 */
struct opa_pport_desc {
	struct hfi_devdata *devdata;
	__be64 pguid;
};

/**
 * struct opa_dev_desc - Used for querying immutable per node
 * opa*_hfi HW  details
 * @devdata: underlying hfi* device structure
 * @oui: Organizational Unique Identifier
 * @num_pports: Number of physical ports
 * @nguid: node GUID, unique per node
 */
struct opa_dev_desc {
	struct hfi_devdata *devdata;
	u8 oui[3];
	u8 num_pports;
	__be64 nguid;
};

/**
 * struct opa_core_ops - HW operations for accessing an OPA device
 * @ctx_assign: Assign a Send/Receive context of the HFI
 * @ctx_release: Release Send/Receive context in the HFI
 * @ctx_reserve: Reserve range of contiguous Send/Receive contexts in the HFI
 * @ctx_unreserve: Release reservation from ctx_reserve
 * @ctx_addr: Return address for HFI resources providing memory access/mapping
 * @cq_assign: Assign a Command Queue for HFI send/receive operations
 * @cq_update: Update configuration of Command Queue
 * @cq_release: Release Command Queue
 * @ev_assign: Assign an Event Completion Queue or Event Counter
 * @ev_release: Release an Event Completion Queue or Event Counter
 * @dlid_assign: Assign entries from the DLID relocation table
 * @dlid_release: Release entries from the DLID relocation table
 */
struct opa_core_ops {
	int (*ctx_assign)(struct hfi_ctx *ctx,
			  struct opa_ctx_assign *ctx_assign);
	void (*ctx_release)(struct hfi_ctx *ctx);
	int (*ctx_reserve)(struct hfi_ctx *ctx, u16 *base, u16 count);
	void (*ctx_unreserve)(struct hfi_ctx *ctx);
	int (*ctx_addr)(struct hfi_ctx *ctx, int type, u16 ctxt, void **addr,
			ssize_t *len);
	int (*cq_assign)(struct hfi_ctx *ctx,
			 struct hfi_auth_tuple *auth_table, u16 *cq_idx);
	int (*cq_update)(struct hfi_ctx *ctx, u16 cq_idx,
			 struct hfi_auth_tuple *auth_table);
	int (*cq_release)(struct hfi_ctx *ctx, u16 cq_idx);
	int (*ev_assign)(struct hfi_ctx *ctx, struct opa_ev_assign *ev_assign);
	int (*ev_release)(struct hfi_ctx *ctx, u16 ev_mode, u16 ev_idx);
	int (*dlid_assign)(struct hfi_ctx *ctx,
			   struct hfi_dlid_assign_args *dlid_assign);
	int (*dlid_release)(struct hfi_ctx *ctx);
	void (*get_device_desc)(struct opa_dev_desc *desc);
	void (*get_port_desc)(struct opa_pport_desc *pdesc, u8 port_num);
};

/**
 * struct opa_core_device - device and vendor ID for an OPA core device
 * @vendor: OPA HFI PCIe vendor ID
 * @device: OPA HFI PCIe device ID
 */
struct opa_core_device_id {
	__u32 vendor;
	__u32 device;
};

/**
 * struct opa_core_device - representation of a device using opa_core
 * @bus_ops: the hardware ops supported by this device.
 * @id: the device type identification (used to match it with a driver).
 * @dev: underlying device.
 * @index: unique position on the opa_core bus
 * @kregbase: start VA for OPA hw csrs
 * @kregend: end VA for OPA hw csrs
 * @dd: device specific information
 */
struct opa_core_device {
	struct opa_core_ops *bus_ops;
	struct opa_core_device_id id;
	struct device dev;
	int index;
	u8 __iomem *kregbase;
	u8 __iomem *kregend;
	struct hfi_devdata *dd;
};

/**
 * opa_core_register_device - Register a hardware driver with the OPA core
 * @dev: The hardware device
 * @id: device and vendor ID
 * @dd: device specific data
 * @bus_ops: operations used by clients to access the hardware
 *
 * Return: an opa_core_device upon success or appropriate error
 */
struct opa_core_device *
opa_core_register_device(struct device *dev, struct opa_core_device_id *id,
			 struct hfi_devdata *dd, struct opa_core_ops *bus_ops);
/**
 * opa_core_unregister_device - unregister a hardware driver from the OPA core
 * @odev: opa core device
 *
 * Return: none
 */
void opa_core_unregister_device(struct opa_core_device *odev);

/**
 * struct opa_core_client - representation of an opa_core client
 *
 * @name: OPA client name
 * @add: the function to call when a device is discovered
 * @remove: the function to call when a device is removed
 * @si: subsystem interface (private to opa_core)
 * @id: ID allocator (private to opa_core)
 */
struct opa_core_client {
	const char *name;
	int (*add)(struct opa_core_device *odev);
	void (*remove)(struct opa_core_device *odev);
	struct subsys_interface si;
	struct idr id;
};

/**
 * opa_core_client_register - Register a client with the OPA core
 * @client: OPA core client
 *
 * Once a client has been successfully registered, its "add" callback
 * will be invoked when an OPA core hardware device is discovered by
 * the OPA core and its "remove" callback will be invoked when an OPA
 * core hardware device is removed from the OPA core.
 *
 * Return: 0 on success or appropriate error
 */
int opa_core_client_register(struct opa_core_client *client);

/**
 * opa_core_client_unregister - Unregister a client from the OPA core
 * @client: OPA core client
 *
 * Return: none
 */
void opa_core_client_unregister(struct opa_core_client *client);

/**
 * opa_core_set_priv_data - Set private data for a client/odev combination
 * @client: OPA core client
 * @odev: OPA core device
 * @priv: private data
 *
 * Return: 0 on success or appropriate error
 */
static inline int
opa_core_set_priv_data(struct opa_core_client *client,
		       struct opa_core_device *odev, void *priv)
{
	return idr_alloc(&client->id, priv, odev->index,
			odev->index + 1, GFP_NOWAIT);
}

/**
 * opa_core_get_priv_data - Get private data for a client/odev combination
 * @client: OPA core client
 * @odev: OPA core device
 *
 * Return: private data pointer or NULL if private data was not set
 */
static inline void *opa_core_get_priv_data(struct opa_core_client *client,
					   struct opa_core_device *odev)
{
	return idr_find(&client->id, odev->index);
}

/**
 * opa_core_clear_priv_data - Clear private data for a client/odev combination
 * @client: OPA core client
 * @odev: OPA core device
 *
 * Return: none
 */
static inline void opa_core_clear_priv_data(struct opa_core_client *client,
					    struct opa_core_device *odev)
{
	return idr_remove(&client->id, odev->index);
}
#endif /* _OPA_CORE_H_ */
