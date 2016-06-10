/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2015 Intel Corporation.
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
 * Copyright (c) 2015 Intel Corporation.
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
 * Intel(R) Omni-Path Core Driver Interface
 */
#ifndef _HFI_CORE_H_
#define _HFI_CORE_H_
#include <linux/device.h>
#include <linux/idr.h>
#include <linux/gfp.h>
#include <linux/module.h>
#include <rdma/ib_verbs.h>
#include <rdma/hfi_types.h>
#include <rdma/opa_smi.h>

/*
 * TODO - Based on hfi_ctx in common provider. hfi_ctx needs to get moved/merged
 * with data types defined by the common provider logic.
 */
struct hfi_devdata;
struct opa_core_device;

/**
 * Error event queue, to be picked by user thread
 * @waitq: user thread waiting for new error event
 * @lock: spin lock for count/head update
 * @count: -1: queue disabled, otherwise, # of events on queue
 * @head: link list head for error event queue
 */
struct hfi_error_queue {
	wait_queue_head_t waitq;
	spinlock_t lock;
	int count;
	struct list_head head;
};

/**
 * Context error list for dispatching by interrupt handler
 * @list: link to global dispatch queue for the device
 * @queue: error queue for a context
 */
struct hfi_ctx_error{
	struct list_head list;
	struct hfi_error_queue queue;
};

/**
 * struct hfi_ctx - state for HFI resources assigned to this context
 * @devdata: HFI device specific data, private to the hardware driver
 * @ops: OPA_CORE device operations
 * @type: kernel or user context
 * @rsm_mask: associated RSM rules, if non-portals context
 * @qpn_map_idx: if a Verbs context, index into QPN->PID table
 * @qpn_map_count: if a Verbs context, count of mapped QPNs
 * @mode: Describes if PIDs or LIDs are virtualized or not
 * @pid: Assigned Portals Process ID
 * @pasid: Assigned Process Address Space ID
 * @ptl_uid: Assigned Protection Domain ID
 * @ptl_state_base: Pointer to Portals state in host memory
 * @le_me_addr: Pointer to head of ME/LE descriptor buffer
 * @ptl_state_size: Size of the total Portals state memory
 * @le_me_count: Number of ME/LE entries
 * @le_me_size: Size of ME/LE buffer
 * @unexpected_size: Size of unexpected header buffer
 * @trig_op_size: Size of triggered ops buffer
 * @tpid_idx: Index into virtual target PID CAM
 * @allow_phys_dlid: Physical LIDs allowed in commands (vs virtual LIDs)
 * @auth_mask: Mask of enabled Protection Domains in @auth_uid
 * @auth_uid: Table of allowed Protection Domains for Command Queues
 * @dlid_base: Base virtual DLID (if DLID table is used)
 * @lid_count: Number of LIDs (fabric endpoints)
 * @lid_offset: LID offset from base for this context (if contiguous LIDs)
 * @sl_mask: Mask of allowed service levels for this context
 * @pid_base: Base of contiguous assigned PIDs on this LID
 * @pid_count: Number of PIDs assigned on this LID
 * @pid_total: Number of PIDs across all LIDs of the application
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
 * @eq_zero: EQ 0 handles
 * @le_me_free_list: List of free ME/LE handles
 * @le_me_free_index: Index of first free handle
 * @pt_free_list: List of free PT entries (per NI)
 * @pt_free_index: Index of first free PT entry (per NI)
 * @error: struct to report errors to user space
 * @wait_list: context waitlist, needed by some kernel-clients
 * TODO @wait_list needed by Verbs, look at moving to hfi2_ibrcv
 */
struct hfi_ctx {
	struct hfi_devdata *devdata;
	struct opa_core_ops *ops;
	u8	type;
	u8	rsm_mask;
	u8	qpn_map_idx;
	u16	qpn_map_count;
	u16	mode;
	u16	pid;
	u32	pasid;
	u32	ptl_uid;
	void	*ptl_state_base;
	void    *le_me_addr;
	u32	ptl_state_size;
	u32	le_me_count;
	u32	le_me_size;
	u32	unexpected_size;
	u32	trig_op_size;
	u8	tpid_idx;
	u8	allow_phys_dlid;
	u8	auth_mask;
	u32	auth_uid[HFI_NUM_AUTH_TUPLES];
	u32	dlid_base;
	u32	lid_count;
	u32	lid_offset;
	u32	sl_mask;
	u16	pid_base;
	u16	pid_count;
	u64	pid_total;
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
	struct hfi_eq	eq_zero[HFI_NUM_NIS];
	hfi_me_handle_t *le_me_free_list;
	u32		le_me_free_index;
	u8		pt_free_list[HFI_NUM_NIS][HFI_NUM_PT_ENTRIES];
	u32		pt_free_index[HFI_NUM_NIS];
	struct hfi_ctx_error error;
	struct list_head wait_list;
};

#define HFI_CTX_TYPE_KERNEL	1
#define HFI_CTX_TYPE_USER	2

#define HFI_CTX_MODE_BYPASS_9B    0x100
#define HFI_CTX_MODE_BYPASS_10B   0x200
#define HFI_CTX_MODE_BYPASS_16B   0x400
#define HFI_CTX_MODE_BYPASS_RSM   0x800
#define HFI_CTX_MODE_BYPASS_MASK  0xF00

#define HFI_CTX_INIT(ctx, dd, bus_ops)		\
	do {					\
		(ctx)->devdata = (dd);		\
		(ctx)->ops = (bus_ops);		\
		(ctx)->type = HFI_CTX_TYPE_KERNEL; \
		(ctx)->mode = 0;		\
		(ctx)->allow_phys_dlid = 1;	\
		/* allow all SLs by default */	\
		(ctx)->sl_mask = -1;		\
		(ctx)->pid = HFI_PID_NONE;	\
		(ctx)->pid_count = 0;		\
		/* 0 reserved for kernel clients */ \
		(ctx)->ptl_uid = 0;		\
		INIT_LIST_HEAD(&(ctx)->wait_list); \
	} while (0)

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
 * enum opa_core_event - events opa_core clients are notified about
 * @OPA_LINK_STATE_CHANGE: there has been a change in the link state
 * @OPA_MTU_CHANGE: there has been a change in the MTU
 * @OPA_PKEY_CHANGE: there has been a change in the partition keys
 */
enum opa_core_event {
	OPA_LINK_STATE_CHANGE,
	OPA_MTU_CHANGE,
	OPA_PKEY_CHANGE
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
 * @user_data: Data returned via completion upon successful EV assignment
 * @base: Base of event buffer in host memory
 * @count: Size specified as number of events
 * @threshold: Num events before blocking EV wakes user
 * @ev_idx: Returns index of EV resource
 * @isr_cb: callback function invoked upon receipt of an interrupt. Clients
 *		cannot perform tasks which are not suitable for an atomic
 *		context in the isr_cb. Clients should not use the
 *		ev_wait_single OPA core operation if they provide an isr_cb.
 * @cookie: cookie to be provided as an argument to isr_cb
 */
struct opa_ev_assign {
	u16 ni;
	u16 mode;
	u64 user_data;
	u64 base;
	u32 count;
	u32 threshold;
	u16 ev_idx;
	void (*isr_cb)(struct hfi_eq *eq, void *cookie);
	void *cookie;
};

/**
 * struct opa_e2e_ctrl - Used with opa_core_ops.e2e_ctrl operation
 * @slid: source LID
 * @dlid: destination LID
 * @port_num: initiator port_num (starts from 1)
 * @sl: service level
 */
struct opa_e2e_ctrl {
	u32 slid;
	u32 dlid;
	u8 port_num;
	u8 sl;
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
 * @pguid: port GUID for this port
 * @lid: LID for this port
 * @ibmaxmtu: A constant max MTU chosen during driver load time
 * @pkey_tlen: Length of PKEY table
 * @pkeys: pointer to the pkey table
 * @num_vls_supported: Number of VL supported
 * @sl_to_sc: service level to service class mapping table
 * @sl_to_sc: service class to service level mapping table
 * @sc_to_vl: service class to virtual lane mapping table
 * @vl_mtu: Array of per VL MTU
 * @lstate: logical link state
 * @pstate: Physical port state
 */
struct opa_pport_desc {
	__be64 pguid;
	u32 lid;
	u16 ibmaxmtu;
	u16 pkey_tlen;
	u16 *pkeys;
	u8 num_vls_supported;
	u8 sl_to_sc[OPA_MAX_SLS];
	u8 sc_to_sl[OPA_MAX_SCS];
	u8 sc_to_vl[OPA_MAX_SCS];
	u16 vl_mtu[OPA_MAX_VLS];
	u8 lstate;
	u8 pstate;
};

/**
 * struct opa_dev_desc - Used for querying immutable per node
 * opa*_hfi HW  details
 * @oui: Organizational Unique Identifier
 * @num_pports: Number of physical ports
 * @nguid: node GUID, unique per node
 * @numa_node: numa node of device
 * @ibdev: registered IB device
 */
struct opa_dev_desc {
	u8 oui[3];
	u8 num_pports;
	__be64 nguid;
	int numa_node;
	struct ib_device *ibdev;
};

#define HFI_NUM_RSM_CONDITIONS 2
#define HFI_NUM_RSM_RULES 4
#define HFI_RSM_MAP_SIZE 256
#define HFI_VNIC_RSM_RULE_IDX 1

/*
 * struct hfi_rsm_rule - Receive Side Mapping (RSM) rule specification
 * @idx: Index of RSM rule to program (0-3)
 * @chain_mask: Mask of chained rules (not implemented)
 * @pkt_type: Packet type to match
 * @hdr_size: Header size used for payload split (bypass pkt_type only)
 * @match_offset: Bit offset of match byte (0-511)
 * @match_mask: 8-bit mask to use for match condition
 * @match_value: Match value used with above masked bits
 * @select_offset: Bit offset of context selection bits (0-511)
 * @select_width: Bits to use for context selection
 */
struct hfi_rsm_rule {
	u8 idx;
	u8 chain_mask;
	u8 pkt_type;
	u8 hdr_size;
	u16 match_offset[HFI_NUM_RSM_CONDITIONS];
	u8 match_mask[HFI_NUM_RSM_CONDITIONS];
	u8 match_value[HFI_NUM_RSM_CONDITIONS];
	u16 select_offset[HFI_NUM_RSM_CONDITIONS];
	u8 select_width[HFI_NUM_RSM_CONDITIONS];
};

/**
 * struct opa_core_ops - HW operations for accessing an OPA device
 * @ctx_assign: Assign a Send/Receive context of the HFI
 * @ctx_release: Release Send/Receive context in the HFI
 * @ctx_reserve: Reserve range of contiguous Send/Receive contexts in the HFI
 * @ctx_unreserve: Release reservation from ctx_reserve
 * @ctx_set_allowed_uids: Set Portals UIDs allowed for application
 * @ctx_addr: Return address for HFI resources providing memory access/mapping
 * @cq_assign: Assign a Command Queue for HFI send/receive operations
 * @cq_update: Update configuration of Command Queue
 * @cq_release: Release Command Queue
 * @cq_map: remap Command Queues into kernel virtual memory
 * @cq_unmap: unmap Command Queues
 * @ev_assign: Assign an Event Completion Queue or Event Counter
 * @ev_release: Release an Event Completion Queue or Event Counter
 * @ev_wait_single: Block on WaitQueue for associated Event Completion resource
 * @dlid_assign: Assign entries from the DLID relocation table
 * @dlid_release: Release entries from the DLID relocation table
 * @e2e_ctrl: Initiate E2E control messages
 * @get_device_desc: get device (node) specific HW details
 * @get_port_desc: get port specific HW details
 * @check_ptl_slp: check SL pair being used for portals traffic
 * @get_hw_limits: obtain HW specific resource limits
 * @set_rsm_rule: set an RSM rule for receive side context mapping
 * @clear_rsm_rule: disable previously set RSM rule
 * @get_master_ts_regs: Timesync master data
 * @get_ts_fm_data: Return timesync data collected from FM MADs
 */
struct opa_core_ops {
	int (*ctx_assign)(struct hfi_ctx *ctx,
			  struct opa_ctx_assign *ctx_assign);
	void (*ctx_release)(struct hfi_ctx *ctx);
	int (*ctx_reserve)(struct hfi_ctx *ctx, u16 *base, u16 count,
			   u16 align, u16 flags);
	void (*ctx_unreserve)(struct hfi_ctx *ctx);
	int (*ctx_set_allowed_uids)(struct hfi_ctx *ctx, u32 *auth_uid,
				    u8 num_uids);
	int (*ctx_addr)(struct hfi_ctx *ctx, int type, u16 ctxt, void **addr,
			ssize_t *len);
	int (*cq_assign)(struct hfi_ctx *ctx,
			 struct hfi_auth_tuple *auth_table, u16 *cq_idx);
	int (*cq_update)(struct hfi_ctx *ctx, u16 cq_idx,
			 struct hfi_auth_tuple *auth_table);
	int (*cq_release)(struct hfi_ctx *ctx, u16 cq_idx);
	int (*cq_map)(struct hfi_ctx *ctx,  u16 cq_idx, struct hfi_cq *tx_cq,
		      struct hfi_cq *rx_cq);
	void (*cq_unmap)(struct hfi_cq *tx_cq, struct hfi_cq *rx_cq);
	int (*ev_assign)(struct hfi_ctx *ctx, struct opa_ev_assign *ev_assign);
	int (*ev_release)(struct hfi_ctx *ctx, u16 ev_mode, u16 ev_idx,
			  u64 user_data);
	int (*ev_wait_single)(struct hfi_ctx *ctx, u16 ev_mode, u16 ev_idx,
			      long timeout);
	int (*dlid_assign)(struct hfi_ctx *ctx,
			   struct hfi_dlid_assign_args *dlid_assign);
	int (*dlid_release)(struct hfi_ctx *ctx, u32 dlid_base, u32 count);
	int (*e2e_ctrl)(struct hfi_ctx *ctx, struct opa_e2e_ctrl *e2e_ctrl);
	void (*get_device_desc)(struct opa_core_device *odev,
				struct opa_dev_desc *desc);
	void (*get_port_desc)(struct opa_core_device *odev,
			      struct opa_pport_desc *pdesc, u8 port_num);
	int (*check_ptl_slp)(struct hfi_ctx *ctx, struct hfi_sl_pair *slp);
	int (*get_hw_limits)(struct hfi_ctx *ctx, struct hfi_hw_limit *hwl);
	int (*set_rsm_rule)(struct opa_core_device *odev,
			    struct hfi_rsm_rule *rule,
			    struct hfi_ctx *rsm_ctx[], u16 num_contexts);
	void (*clear_rsm_rule)(struct opa_core_device *odev, u8 rule_idx);
	int (*get_ts_master_regs)(struct hfi_ctx *ctx,
				  struct hfi_ts_master_regs *ts_master_regs);
	int (*get_ts_fm_data)(struct hfi_ctx *ctx,
			      struct hfi_ts_fm_data *fm_data);
	int (*get_async_error)(struct hfi_ctx *ctx, void *ae, int timeout);
};

/**
 * struct opa_core_device - device, vendor and revision ID for an OPA core
 * device
 *
 * @vendor: OPA HFI PCIe vendor ID
 * @device: OPA HFI PCIe device ID
 * @revision: OPA HFI PCIe revision ID
 */
struct opa_core_device_id {
	__u32 vendor;
	__u32 device;
	__u32 revision;
};

/**
 * struct opa_core_device - representation of a device using opa_core
 * @bus_ops: the hardware ops supported by this device.
 * @id: the device type identification (used to match it with a driver).
 * @dev: underlying device.
 * @module: underlying device driver module.
 * @index: unique position on the opa_core bus
 * @kregbase: start VA for OPA hw csrs
 * @kregend: end VA for OPA hw csrs
 * @dd: device specific information
 */
struct opa_core_device {
	struct opa_core_ops *bus_ops;
	struct opa_core_device_id id;
	struct device dev;
	struct module *owner;
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
 * opa_core_notify_clients - Notify clients about a hardware event
 * @odev: opa core device
 * @event: hardware event
 * @port: port number of the port which generated this event
 *
 * Upon receipt of an event, a client should query the changes via
 * the (*get_port_desc) hardware operation. Clients are allowed to
 * sleep or block in these notifications so opa_core_notify_clients
 * should not be called from an atomic context.
 * Return: none
 */
void opa_core_notify_clients(struct opa_core_device *odev,
			     enum opa_core_event event, u8 port);

/**
 * struct opa_core_client - representation of an opa_core client
 *
 * @name: OPA client name
 * @add: the function to call when a device is discovered
 * @remove: the function to call when a device is removed
 * @event_notify: the function to call to notify the client
 *		  about hardware events. Clients are allowed
 *		  to sleep or block in these notifications.
 * @si: subsystem interface (private to opa_core)
 * @id: ID allocator (private to opa_core)
 * @id_num: Identifier for this client (private to opa_core)
 */
struct opa_core_client {
	const char *name;
	int (*add)(struct opa_core_device *odev);
	void (*remove)(struct opa_core_device *odev);
	void (*event_notify)(struct opa_core_device *odev,
			     enum opa_core_event event,
			     u8 port);
	struct subsys_interface si;
	struct idr id;
	int id_num;
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

/**
 * opa_core_device_get - Hold the opa_core_device to prevent removal
 * @odev: OPA core device
 *
 * The intent is to allow a client to prevent client->remove() callback
 * while the device is in active use which the remove() callback cannot
 * teardown.
 * Caller is responsible to ensure not called after remove() callback as
 * the opa_core_device is no longer accessible at that point.
 *
 * Return: 0 on success or appropriate error
 */
static inline int opa_core_device_get(struct opa_core_device *odev)
{
	return try_module_get(odev->owner) ? 0 : -ENODEV;
}

/**
 * opa_core_device_put - Release the opa_core_device
 * @odev: OPA core device
 *
 * Caller is responsible to ensure not called after remove() callback as
 * the opa_core_device is no longer accessible at that point.
 *
 * Return: none
 */
static inline void opa_core_device_put(struct opa_core_device *odev)
{
	module_put(odev->owner);
}
#endif /* _HFI_CORE_H_ */
