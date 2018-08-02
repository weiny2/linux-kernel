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
#include <linux/mutex.h>
#include <linux/module.h>
#include <rdma/ib_verbs.h>
#include <rdma/opa_smi.h>
#include "hfi_cmd.h"
#include "../chip/fxr_sw_structs.h"
#include "../chip/fxr_sw_structs_defs.h"
#include "../chip/fxr_sw_defs.h"

/* FXR identifies with this PCI ID */
#define PCI_VENDOR_ID_INTEL       0x8086
#define PCI_DEVICE_ID_INTEL_FXR0  0x26d0

#define HFI_CTX_INVALID(ctx)	((ctx) == NULL)
#define HFI_TS_MUTEX_LOCK(ctx)
#define HFI_TS_MUTEX_UNLOCK(ctx)

/* TODO - may remove this when supporting multiple PIDs */
#define HFI_MAX_IB_HW_CONTEXTS	1

#define HFI_LID_NONE	(__u32)(-1)
#define HFI_PID_NONE	(__u16)(-1)

/*
 * TODO - the subunions contain bitfields; kernel users of this
 * union should consider using alternate logic to set target id.
 */
union hfi_process {
	union phys_process phys;
	union logical_process logical;
	u64 val;
};

/* TODO - temporary typedefs */
typedef union host_rx_priority_me	hfi_me_t;
typedef union host_rx_uh_rts_get_le	hfi_uh_t;

struct hfi_devdata;
struct hfi_ctx;
struct hfi_cmdq;

struct hfi_ks {
	struct mutex lock;
	u32 num_keys;
	u32 key_head;
	u32 *free_keys;
};

/**
 * IB context allocated to IB core clients,
 * HW resources assigned and shared across Native PDs
 * @ibuc: IB ucontext state
 * @ops: hfi2 ops table (temporary for KFI)
 * @priv: hfi2 private data
 * @vma_head: linked list head for vma's to zap on release
 * @vm_lock: mutex to update the list of vma's
 * @support_native: boolean if this context is using native transport
 * @lkey_only: boolean if this context has shared LKEY/RKEYs
 * @is_user: boolean if this a user-space context
 * @num_ctx: number of associated hfi_ctx
 * @job_res_mode: mode for job_attach to lookup job reservation
 * @job_res_cookie: cookie for lookup of job reservation
 * @hw_ctx: assigned HW contexts
 * @cmdq: Command Queues (TX and RX)
 * @ctx_lock: lock for adding/removing resources
 * @rkey_ks: RKEYs stack
 * @lkey_ks: LKEYs stack
 * @mr: MR array for LKEYs
 */
struct hfi_ibcontext {
	struct ib_ucontext ibuc;
	struct opa_core_ops *ops;
	struct hfi_devdata *priv;
	struct list_head vma_head;
	struct mutex vm_lock;
	bool supports_native;
	bool lkey_only;
	bool is_user;
	u16 num_ctx;
	u16 job_res_mode;
	u64 job_res_cookie;
	struct hfi_ctx	*hw_ctx[HFI_MAX_IB_HW_CONTEXTS];
	struct hfi_cmdq_pair *cmdq;
	struct mutex	ctx_lock;
	struct hfi_ks	rkey_ks;
	struct hfi_ks	lkey_ks;
	struct rvt_mregion **lkey_mr;
	u64	*tx_qp_flow_ctl;
	spinlock_t flow_ctl_lock;
	struct rvt_cq *sync_cq;
};

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
struct hfi_ctx_error {
	struct list_head list;
	struct hfi_error_queue queue;
};

/*
 * struct hfi_eq - Event Queue
 * @ctx: hardware context containing this EQ
 * @base: base address of Event Buffer
 * @events_pending: counter of outstanding events to be delivered
 * @count: size of buffer, in number of events
 * @idx: EQ index into EQD table (per context)
 * @width: log2 of EQ event entry width
 * @head_addr: address where head pointer is stored
 */
struct hfi_eq {
	struct hfi_ctx *ctx;
	void *base;
	atomic_t events_pending;
	u32 count;
	u16 idx;
	u16 width;
	u32 *head_addr;
};

/*
 * struct hfi_ibeq - IB Event Queue
 * @eq: Event Queue
 * @hw_armed: used by kernel IB CQ to signal interrupt arming command completed
 * @hw_disarmed: used by kernel IB CQ to signal disarming command completed
 * @hw_cq: if EQ is an IB CQ points to next EQ associated with IB CQ
 */
struct hfi_ibeq {
	struct hfi_eq eq;
	u64 hw_armed;
	u64 hw_disarmed;
	struct list_head hw_cq;
	struct list_head qp_ll;
};

/**
 * struct hfi_job_res - set of HFI resources reserved by Resource Manager
 * @allow_phys_dlid: Physical LIDs allowed in commands (vs virtual LIDs)
 * @auth_mask: Mask of enabled Protection Domains in @auth_uid
 * @tpid_idx: Index into virtual target PID CAM
 * @mode: Describes if PIDs or LIDs are virtualized or not
 * @auth_uid: Table of allowed Protection Domains for Command Queues
 * @dlid_base: Base virtual DLID (if DLID table is used)
 * @lid_count: Number of LIDs (fabric endpoints)
 * @lid_offset: LID offset from base for this context (if contiguous LIDs)
 * @pid_base: Base of contiguous assigned PIDs on this LID
 * @pid_count: Number of PIDs assigned on this LID
 * @pid_total: Number of PIDs across all LIDs of the application
 * @res_mutex: mutex to protect updates of reserved resources
 */
struct hfi_job_res {
	u8	allow_phys_dlid;
	u8	auth_mask;
	u8	tpid_idx;
	u16	mode;
	u32	auth_uid[HFI_NUM_AUTH_TUPLES];
	u32	dlid_base;
	u32	lid_count;
	u32	lid_offset;
	u16	pid_base;
	u16	pid_count;
	u64	pid_total;
	struct mutex res_mutex;
};

/**
 * struct hfi_ctx - state for HFI resources assigned to this context
 * @devdata: HFI device specific data, private to the hardware driver
 * @ops: OPA_CORE device operations
 * @type: kernel or user context
 * @rsm_mask: associated RSM rules, if non-portals context
 * @mode: Describes if PIDs or LIDs are virtualized or not
 * @cmdq_pair_num_assigned: Counter of assigned Command Queues
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
 * @ct_used: IDR table of allocated Event Counters (CTs)
 * @eq_used: IDR table of allocated Events Queues
 * @ec_used: IDR table of allocated Events Channels
 * @event_mutex: Lock for event related operations
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
 * @ctx_rwsem: protect multiple threads of same context from attaching/freeing
 * resources concurrently
 * @shr_me_ks: ME key stack for use with native Verbs provider
 * @eq_lock: spin lock used for provider RX command processing
 * @sync_eq: eq for RNR events
 */
struct hfi_ctx {
	struct hfi_devdata *devdata;
	struct opa_core_ops *ops;
	u8	type;
	u8	rsm_mask;
	u16	mode;
	u16	cmdq_pair_num_assigned;
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
	struct hfi_job_res res;

	struct idr ct_used;
	struct idr eq_used;
	struct idr ec_used;
	struct mutex event_mutex;
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

	u16	*le_me_free_list;
	u32	le_me_free_index;

	u8	pt_free_list[HFI_NUM_NIS][HFI_NUM_PT_ENTRIES];
	u32	pt_free_index[HFI_NUM_NIS];

	struct hfi_ctx_error error;

	struct rw_semaphore ctx_rwsem;
	struct hfi_ks	*shr_me_ks;
	spinlock_t eq_lock;
	struct hfi_eq   sync_eq;

	struct ib_uobject *uobject;
};

#define HFI_CTX_TYPE_KERNEL	1
#define HFI_CTX_TYPE_USER	2

/* Last CTX_MODE bit reserved for kernel drivers */
#define HFI_CTX_MODE_REQUEST_DEFAULT_BYPASS	0x800

/**
 * enum mmap_token_types -  Types of memory regions mapped by the context and
 * @TOK_CONTROL_BLOCK: map process control block
 * @TOK_PORTALS_TABLE: map portal table entries
 * @TOK_CMDQ_TX: map TX command queue
 * @TOK_CMDQ_RX: map RX command queue
 * @TOK_CMDQ_HEAD: map command queue head
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
	TOK_CMDQ_TX,
	TOK_CMDQ_RX,
	TOK_CMDQ_HEAD,
	TOK_EVENTS_CT,
	TOK_EVENTS_EQ_DESC,
	TOK_EVENTS_EQ_HEAD,
	TOK_TRIG_OP,
	TOK_LE_ME,
	TOK_UNEXPECTED,
};

#define HFI_CMDQ_COUNT		192
#define HFI_CMDQ_ENTRY_SIZE	64
#define HFI_CMDQ_TX_SIZE	(HFI_CMDQ_TX_ENTRIES * HFI_CMDQ_ENTRY_SIZE)
#define HFI_CMDQ_RX_SIZE	(HFI_CMDQ_RX_ENTRIES * HFI_CMDQ_ENTRY_SIZE)
#define HFI_PT_ENTRY_SIZE	32
#define HFI_CT_ENTRY_SIZE	32
#define HFI_EQ_DESC_ENTRY_SIZE	16
#define HFI_EQ_HEAD_ENTRY_SIZE	4

/* sizes of Portals State segments for above mmaps */
#define HFI_PSB_PT_NI_SIZE	(HFI_NUM_PT_ENTRIES * HFI_PT_ENTRY_SIZE)
#define HFI_PSB_PT_SIZE		(HFI_PSB_PT_NI_SIZE * HFI_NUM_NIS)
#define HFI_PSB_CT_NI_SIZE	(HFI_NUM_CT_ENTRIES * HFI_CT_ENTRY_SIZE)
#define HFI_PSB_CT_SIZE		(HFI_PSB_CT_NI_SIZE * HFI_NUM_NIS)
#define HFI_PSB_EQ_DESC_NI_SIZE	(HFI_NUM_EVENT_HANDLES * HFI_EQ_DESC_ENTRY_SIZE)
#define HFI_PSB_EQ_DESC_SIZE	(HFI_PSB_EQ_DESC_NI_SIZE * HFI_NUM_NIS)
#define HFI_PSB_EQ_HEAD_NI_SIZE	(HFI_NUM_EVENT_HANDLES * HFI_EQ_HEAD_ENTRY_SIZE)
#define HFI_PSB_EQ_HEAD_SIZE	(HFI_PSB_EQ_HEAD_NI_SIZE * HFI_NUM_NIS)

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
 * @ev_idx: Returns index of EV resource
 * @jumbo: boolean if this EQ uses double-wide events
 * @user_data: EQ in/out user space data, or CT failure value
 * @base: Base of event buffer in host memory, or CT success value
 * @count: Size specified as number of events
 * @isr_cb: callback function invoked upon receipt of an interrupt. Clients
 *		cannot perform tasks which are not suitable for an atomic
 *		context in the isr_cb. Clients should not use the
 *		ev_wait_single OPA core operation if they provide an isr_cb.
 * @cookie: cookie to be provided as an argument to isr_cb
 */
struct opa_ev_assign {
	u16 ni;
	u16 mode;
	u16 ev_idx;
	bool jumbo;
	u64 user_data;
	u64 base;
	u32 count;
	void (*isr_cb)(struct hfi_eq *eq, void *cookie);
	void *cookie;
};

/*
 * opa_ev_assign.mode bits, intent is upper bits for specifying EV mechanism,
 * lower bits for controlling behavior
 */
#define OPA_EV_MODE_BLOCKING  0x1
#define OPA_EV_MODE_COUNTER   0x100

struct hfi_dlid_assign_args;

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
 * @ctx_init: Initialize the hfi_ctx structure for performing OPA operations
 * @ctx_assign: Assign a Send/Receive context of the HFI
 * @ctx_release: Release Send/Receive context in the HFI
 * @ctx_reserve: Reserve range of contiguous Send/Receive contexts in the HFI
 * @ctx_unreserve: Release reservation from ctx_reserve
 * @ctx_set_allowed_uids: Set Portals UIDs allowed for application
 * @ctx_addr: Return address for HFI resources providing memory access/mapping
 * @cmdq_assign: Assign a Command Queue for HFI send/receive operations
 * @cmdq_update: Update configuration of Command Queue
 * @cmdq_release: Release Command Queue
 * @cmdq_map: remap Command Queues into kernel virtual memory
 * @cmdq_unmap: unmap Command Queues
 * @ev_assign: Assign an Event Completion Queue (EQ) or Event Counter
 * @ev_release: Release an Event Completion Queue (EQ) or Event Counter
 * @ev_wait_single: Block on WaitQueue for EQ/CT event
 * @ev_set_channel: set EQ/CT to event channel
 * @ec_wait_event: Block on event channel for EQ/CT event
 * @eq_ack_event: Ack events on EQ
 * @ct_ack_event: Ack events on CT
 * @ec_assign: allocate an event channel
 * @ec_release: release an event channel
 * @pt_update_lower: update the lower 16 bytes of the PTE under mask
 * @dlid_assign: Assign entries from the DLID relocation table
 * @dlid_release: Release entries from the DLID relocation table
 * @e2e_ctrl: Initiate E2E control messages
 * @check_sl_pair: check if have valid SL pair
 * @get_hw_limits: obtain HW specific resource limits
 * @get_master_ts_regs: Timesync master data
 * @get_ts_fm_data: Return timesync data collected from FM MADs
 */
struct opa_core_ops {
	void (*ctx_init)(struct ib_device *ibdev, struct hfi_ctx *ctx);
	int (*ctx_assign)(struct hfi_ctx *ctx,
			  struct opa_ctx_assign *ctx_assign);
	int (*ctx_release)(struct hfi_ctx *ctx);
	int (*ctx_reserve)(struct hfi_ctx *ctx, u16 *base, u16 count,
			   u16 align, u16 flags);
	void (*ctx_unreserve)(struct hfi_ctx *ctx);
	int (*ctx_set_allowed_uids)(struct hfi_ctx *ctx, u32 *auth_uid,
				    u8 num_uids);
	int (*ctx_addr)(struct hfi_ctx *ctx, int type, u16 ctxt, void **addr,
			ssize_t *len);
	int (*cmdq_assign)(struct hfi_cmdq_pair *cmdq, struct hfi_ctx *ctx,
			   struct hfi_auth_tuple *auth_table);
	int (*cmdq_update)(struct hfi_cmdq_pair *cmdq,
			   struct hfi_auth_tuple *auth_table);
	int (*cmdq_release)(struct hfi_cmdq_pair *cmdq);
	int (*cmdq_map)(struct hfi_cmdq_pair *cmdq);
	void (*cmdq_unmap)(struct hfi_cmdq_pair *cmdq);

	int (*ev_assign)(struct hfi_ctx *ctx, struct opa_ev_assign *ev_assign);
	int (*ev_release)(struct hfi_ctx *ctx, u16 ev_mode, u16 ev_idx,
			  u64 user_data);
	int (*ev_wait_single)(struct hfi_ctx *ctx, u16 eqflag, u16 ev_idx,
			      int timeout, u64 *user_data0, u64 *user_data1);
	int (*ev_set_channel)(struct hfi_ctx *ctx, u16 ec_idx, u16 ev_idx,
			      u64 eq_handle, u64 user_data);
	int (*ec_wait_event)(struct hfi_ctx *ctx, u16 ec_idx, int timeout,
			     u64 *user_data0, u64 *user_data1);
	int (*eq_ack_event)(struct hfi_ctx *ctx, u16 eq_idx, u32 nevents);
	int (*ct_ack_event)(struct hfi_ctx *ctx, u16 ct_idx, u32 nevents);
	int (*ec_assign)(struct hfi_ctx *ctx, u16 *ec_idx);
	int (*ec_release)(struct hfi_ctx *ctx,  u16 ec_idx);

	int (*pt_update_lower)(struct hfi_ctx *ctx, u8 ni, u32 pt_idx,
			       u64 *val, u64 user_data);
	int (*dlid_assign)(struct hfi_ctx *ctx,
			   struct hfi_dlid_assign_args *dlid_assign);
	int (*dlid_release)(struct hfi_ctx *ctx);
	int (*e2e_ctrl)(struct hfi_ibcontext *uc,
			struct hfi_e2e_conn *e2e_conn);
	int (*check_sl_pair)(struct hfi_ibcontext *uc, struct hfi_sl_pair *slp);
	int (*get_hw_limits)(struct hfi_ibcontext *uc,
			     struct hfi_hw_limit *hwl);
	int (*get_ts_master_regs)(struct hfi_ibcontext *uc,
				  struct hfi_ts_master_regs *ts_master_regs);
	int (*get_ts_fm_data)(struct hfi_ibcontext *uc,
			      struct hfi_ts_fm_data *fm_data);
	int (*get_async_error)(struct hfi_ctx *ctx, void *ae, int timeout);
	int (*at_prefetch)(struct hfi_ctx *ctx,
			   struct hfi_at_prefetch_args *atpf);
};
#endif /* _HFI_CORE_H_ */
