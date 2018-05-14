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

#ifndef _HFI_CMD_H
#define _HFI_CMD_H

#include <linux/types.h>
#include <uapi/rdma/hfi/hfi2_ioctl.h>
/*
 * This version number is given to the driver by the user code during
 * early initialization sequence.
 * The driver can check for compatibility with user code.
 *
 * The major version changes when data structures change in an
 * incompatible way.  The driver must be the same or higher for
 * initialization to succeed.  In some cases, a higher version driver
 * will not interoperate with older software, and initialization will
 * return an error.
 */
#define HFI_USER_SWMAJOR 1

/*
 * Minor version differences are always compatible a within a major
 * version, however if user software is larger than driver software,
 * some new features and/or structure fields may not be implemented;
 * the user code must deal with this if it cares, or it must abort
 * after initialization reports the difference.
 */
#define HFI_USER_SWMINOR 0
#define HFI_USER_SWVERSION ((HFI_USER_SWMAJOR << 16) | HFI_USER_SWMINOR)

#ifndef HFI_KERN_TYPE
#define HFI_KERN_TYPE 0
#endif

/*
 * Similarly, this is the kernel version going back to the user.  It's
 * slightly different, in that we want to tell if the driver was built
 * as part of an Intel release, or from the driver from openfabrics.org,
 * kernel.org, or a standard distribution, for support reasons.
 * The high bit is 0 for non-Intel and 1 for Intel-built/supplied.
 *
 * It's returned by the driver to the user code during the same early
 * initialization sequence, so the user code can in turn check for
 * compatibility with the kernel.
 */
#define HFI_KERN_SWVERSION ((HFI_KERN_TYPE << 31) | HFI_USER_SWVERSION)

/* General defines for hardware resource sizes */
#define HFI_NUM_NIS		4
#define HFI_NUM_PT_ENTRIES	256
#define HFI_TRIG_OP_MAX_COUNT	65535
#define HFI_LE_ME_MAX_COUNT	65535
#define HFI_UNEXP_MAX_COUNT	65535
#define HFI_PT_INDEX(NI, PT)	(((NI) * HFI_NUM_PT_ENTRIES) + (PT))
/* Completions / Event Queues */
#define HFI_NUM_CT_ENTRIES	2048
#define HFI_NUM_CT_RESERVED	16
#define HFI_NUM_EVENT_HANDLES	2048
#define HFI_MIN_EVENT_ORDER	6
#define HFI_MAX_EVENT_ORDER	22
#define HFI_EQ_ALIGNMENT	0x40
#define HFI_EQ_ENTRY_SIZE	64
#define HFI_EQ_ENTRY_LOG2	6
#define HFI_EQ_ENTRY_LOG2_JUMBO	7
#define HFI_CT_INDEX(NI, CT)	(((NI) * HFI_NUM_CT_ENTRIES) + (CT))
#define HFI_EQ_INDEX(NI, EQ)	(((NI) * HFI_NUM_EVENT_HANDLES) + (EQ))
#define HFI_SR_INDEX(NI, SR)	(((NI) * HFI_NUM_CT_RESERVED) + (SR))
/* Command Queues */
#define HFI_CMDQ_TX_ENTRIES	128
#define HFI_CMDQ_RX_ENTRIES	16

/* Defines for backwards compat transports */
#define HFI_NI_BYPASS		0x3
#define HFI_PID_BYPASS_BASE	0xF00
#define HFI_NUM_BYPASS_PIDS	160
#define HFI_PT_BYPASS_EAGER	32
#define HFI_RX_EAGER_MAX_COUNT	2048
#define HFI_RX_EAGER_MAX_ORDER	9
#define HFI_PT_BYPASS_EXPECTED_COUNT	32
#define HFI_RX_EXPECTED_MAX_ORDER	9

/* Return boolean value of NI types */
#define HFI_MATCHING_NI(NI) (((NI) & 2) >> 1)
#define HFI_PHYSICAL_NI(NI) (((NI) & 1))

/* Generate CMD_COMPLETE event with commands to RX CQ */
#define HFI_GEN_CC		0
/* Do not generate CMD_COMPLETE event with commands to RX CQ */
#define HFI_NCC			1

#define HFI_LID_ANY	(__u32)(16777215)  /* 24-bit LID */
#define HFI_PID_ANY	(__u16)(4095)      /* 12-bit PID */
#define HFI_PT_ANY	(__u32)(-1)
#define HFI_UID_ANY	(__u32)(-1)
#define HFI_CT_NONE	0

/* hfi2 Commands */
#define HFI_CMD_CTXT_ATTACH       20
#define HFI_CMD_CTXT_DETACH       21
#define HFI_CMD_CMDQ_ASSIGN       22
#define HFI_CMD_CMDQ_RELEASE      23
#define HFI_CMD_EQ_ASSIGN         24
#define HFI_CMD_EQ_RELEASE        25
#define HFI_CMD_DLID_ASSIGN       26
#define HFI_CMD_DLID_RELEASE      27
#define HFI_CMD_JOB_INFO          28
#define HFI_CMD_JOB_SETUP         29
#define HFI_CMD_CMDQ_UPDATE       30
#define HFI_CMD_AT_PREFETCH       31
#define HFI_CMD_UNUSED            32
#define HFI_CMD_CT_ASSIGN         33
#define HFI_CMD_CT_RELEASE        34
#define HFI_CMD_E2E_CONN          35
#define HFI_CMD_EC_WAIT_EQ        36
#define HFI_CMD_EQ_WAIT           37
#define HFI_CMD_CHECK_SL_PAIR     38
#define HFI_CMD_GET_HW_LIMITS     39
#define HFI_CMD_TS_GET_MASTER     40
#define HFI_CMD_TS_GET_FM_DATA    41
#define HFI_CMD_GET_ASYNC_ERROR   42
#define HFI_CMD_EC_ASSIGN         43
#define HFI_CMD_EC_RELEASE        44
#define HFI_CMD_EC_SET_EQ         45
#define HFI_CMD_EQ_ACK            46
#define HFI_CMD_PT_UPDATE_LOWER	  47
#define HFI_CMD_EC_WAIT_CT        48
#define HFI_CMD_CT_WAIT           49
#define HFI_CMD_EC_SET_CT         50
#define HFI_CMD_CT_ACK            51

/**
 * struct hfi_cmd - Command descriptor to perform HFI operations
 * @api_version: input HFI_USER_SWVERISON to inform driver of API version
 * @type: command opcode from HFI_CMD defines above
 * @length: length of @context data
 * @context: data for this command
 */
struct hfi_cmd {
	__u32 api_version;
	__u32 type;
	__u32 length;
	__u64 context;
};

#define IN    /* input argument */
#define INOUT /* input and output argument */
#define OUT   /* output argument */

/*
 * struct hfi_cmdq - Command Queue attributes
 * @cmdq_idx: index of assigned Command Queue
 * @base: base address of Command Queue
 * @size: size of Command Queue in bytes
 * @head_addr: address where head pointer is written
 * @slot_idx: next slot to write (tail)
 * @sw_head_idx: first slot waiting for device to consume
 * @slots_avail: number of slots available (before tail meets head)
 * @slots_total: total Command Queue slots
 * @auth_table: table of configured hfi_auth_tuples
 */
struct hfi_cmdq {
	__u16	cmdq_idx;
	void	*base;
	size_t	size;

	volatile __u8 *head_addr;
#if (!defined(__KERNEL__) && !defined(EFIAPI))
	pthread_spinlock_t thread_spin;
#endif
	__u8	slot_idx;
	__u8	sw_head_idx;
	__u8	slots_avail;
	__u8	slots_total;
	struct hfi_auth_tuple auth_table[HFI_NUM_AUTH_TUPLES];
	struct ib_device *device;
	struct ib_uobject *uobject;
	struct hfi_ctx *ctx;
};

/**
 * struct hfi_cmdq_assign_args - Command Queue assignment
 * @auth_table: array of Authentication Tuples [USER_ID,SRANK]
 * @cmdq_idx: index of Command Queue that was assigned
 * @cmdq_tx_token: mmap offset for TX Command Queue
 * @cmdq_rx_token: mmap offset for RX Command Queue
 * @cmdq_head_token: mmap offset for CQ head addresses
 */
struct hfi_cmdq_assign_args {
	IN  struct hfi_auth_tuple auth_table[HFI_NUM_AUTH_TUPLES];
	OUT __u16 cmdq_idx;
	OUT __u64 cmdq_tx_token;
	OUT __u64 cmdq_rx_token;
	OUT __u64 cmdq_head_token;
};

/**
 * struct hfi_cmdq_update_args - update Command Queue configuration
 * @cmdq_idx: index of Command Queue to update
 * @auth_table: array of new Authentication Tuples [USER_ID,SRANK]
 */
struct hfi_cmdq_update_args {
	IN  __u16 cmdq_idx;
	IN  struct hfi_auth_tuple auth_table[HFI_NUM_AUTH_TUPLES];
};

/**
 * struct hfi_ct_assign_args - Command Queue release
 * @cmdq_idx: index of Command Queue to free
 */
struct hfi_cmdq_release_args {
	IN  __u16 cmdq_idx;
};

/**
 * struct hfi_dlid_assign_args - DLID relocation
 * @dlid_base: requested base in DLID relocation table
 * @count: count of entries in DLID relocation array
 * @granularity: granularity used to construct DLID relocation entry array
 * @dlid_entries_ptr: input array of DLID relocation entries
 */
struct hfi_dlid_assign_args {
	IN __u32 dlid_base;
	IN __u32 count;
	IN __u16 granularity;
	IN __u64 *dlid_entries_ptr;
};

/**
 * struct hfi_event_args - event operation arguments
 * this struct is used by following function calls:
 *
 * hfi_ct_alloc: idx0(ni), idx1(ct_idx),
 *               data0: user initial success
 *               data1: user initial failure
 * hfi_ct_free:  idx1(ct_idx)
 * hfi_ct_wait:  idx1(ct_idx), count: timeout
 *               data1: returned ct threshold
 * hfi_ec_wait_ct: idx0(ec_idx), count: timeout
 *               data0: returned ct_idx
 *               data1: returned ct threshold
 * hfi_ec_set_ct:idx0(ec_idx), idx1(ct_idx)
 * hfi_ct_ack:   idx1(ct_idx),
 *               count: number of events
 *
 * hfi_eq_alloc: idx0(ni), idx1(eq_idx),
 *               count: eq entries
 *               data0: eq base buffer,
 *               data1: user completion address to allocate eq
 * hfi_eq_free:  idx1(eq_idx),
 *               data1: user completion address to release eq
 * hfi_eq_wait:  idx1(eq_idx), count: timeout
 *               data0: user completion address to turn on interrupt
 *               data1: user completion address to turn off interrupt
 * hfi_ec_wait_eq: idx0(ec_idx), count: timeout
 *               data0: user handle from kernel
 *               data1: user completion address to turn off interrupt
 * hfi_ec_set_eq:idx0(ec_idx), idx1(eq_idx),
 *               data0: user handle to kernel
 *               data1: user completion address to turn on interrupt
 * hfi_eq_ack:   idx1(eq_idx),
 *               count: number of events
 *
 * hfi_ec_alloc: idx0(ec_idx)
 * hfi_ec_free:  idx0(ec_idx)

 * @idx0: index for NI, EC
 * @idx1: index for EQ, CT
 * @count: for timeout, entries, events
 * @data0: various 64bits user data to/from kernel
 * @data1: various 64bits user data to/from kernel
 */
struct hfi_event_args {
	IN    __u16 idx0;
	IN    __u16 idx1;
	IN    __u32 count;

	INOUT __u64 data0;
	INOUT __u64 data1;
};

/**
 * struct hfi_pt_update_lower_args - lower 16B portal table update
 * @ni: network interface for operation
 * @pt_idx: portal table index
 * @val: value of portal table entry
 * @user data: user data for completion event
 */
struct hfi_pt_update_lower_args {
	IN __u8	ni;
	IN __u32 pt_idx;
	IN __u64 val[2];
	IN __u64 user_data;
};

/*
 * Defines below are shared between user and kernel,
 * Flag bits for hfi_ctxt_attach_args and hfi_job_setup_args
 */
#define HFI_CTX_FLAG_REUSE_ADDR        0x1
#define HFI_CTX_FLAG_SET_VIRTUAL_PIDS  0x2
#define HFI_CTX_FLAG_ASYNC_ERROR       0x4
#define HFI_CTX_FLAG_USE_BYPASS        0x8

/* Mode bits for hfi_ctx.mode (set during CTXT_ATTACH) */
#define HFI_CTX_MODE_LID_VIRTUALIZED   0x1
#define HFI_CTX_MODE_VTPID_NONMATCHING 0x2
#define HFI_CTX_MODE_VTPID_MATCHING    0x4
#define HFI_CTX_MODE_USE_BYPASS        0x8
#define HFI_CTX_MODE_DRIVER_RESERVED   0xF00

/**
 * struct hfi_ctxt_attach_args - assign a Portals Context
 * @pid: requested Portals PID (or HFI_PID_ANY)
 * @flags: PID assignment flags (see HFI_CTX_FLAG_XXX)
 * @le_me_count_count: count of LE/MEs to allocate
 * @unexpected_count: count of unexpected headers to allocate
 * @trig_op_count: count of Triggered Op descriptors to allocate
 * @pid_base: physical PID base of reserved PIDs
 * @pid_count: Number of locally reserved PIDs
 * @pid_mode: flag bits if LIDs/PIDs were virtualized, and for bypass
 * @uid: driver assigned default Portals UID for Command Queues
 * @ct_token: mmap offset for Counting Events
 * @eq_desc_token: mmap offset for EQ descriptors
 * @eq_head_token: mmap offset for EQ read pointer
 * @pt_token: mmap offset for Portals Table
 * @le_me_token: mmap offset for LE/ME list
 * @le_me_unlink_token: mmap offset for freed LE/MEs
 * @unexpected_token: mmap offset for unexpected list
 * @trig_op_token: mmap offset for trig op list
 */
struct hfi_ctxt_attach_args {
	IN  __u16 pid;
	IN  __u16 flags;
	IN  __u16 le_me_count;
	IN  __u16 unexpected_count;
	IN  __u16 trig_op_count;
	OUT __u16 pid_base;
	OUT __u16 pid_count;
	OUT __u16 pid_mode;
	OUT __u32 uid;
	OUT __u64 ct_token;
	OUT __u64 eq_desc_token;
	OUT __u64 eq_head_token;
	OUT __u64 pt_token;
	OUT __u64 le_me_token;
	OUT __u64 le_me_unlink_token;
	OUT __u64 unexpected_token;
	OUT __u64 trig_op_token;
};

/*
 * struct hfi_job_info - describes resources assigned for the current job
 * @pid_total: Number of total job ranks
 * @pid_base: Local physical PID base of reserved PIDs
 * @pid_count: Number of locally reserved PIDs
 * @pid_mode: Flags if PIDs or LIDs were virtualized, or for bypass.
 * @dlid_base:  Index into DLID relocation table if LIDs are virtualized
 * @lid_offset: Logical LID of the caller
 * @lid_count: Number of LIDs for this job
 * @auth_uid: Portals UIDs allowed for this job
 */
struct hfi_job_info {
	__u64	pid_total;
	__u16	pid_base;
	__u16	pid_count;
	__u16	pid_mode;
	__u32	dlid_base;
	__u32	lid_offset;
	__u32	lid_count;
	__u32   auth_uid[HFI_NUM_AUTH_TUPLES];
};

#define HFI_JOB_RES_SESSION 0x1
#define HFI_JOB_RES_CGROUP  0x2

/*
 * struct hfi_job_setup - reserve resources for the current job,
 *   some of this is just passed thru to the job processes to assist
 *   with logical [LID,PID] addressing computations
 * @pid_total: Number of total job ranks (across all LIDs)
 * @pid_base: Requested physical PID base of reserved PIDs (or PID_ANY)
 * @pid_count: Number of locally reserved PIDs
 * @flags: Flags if PIDs or LIDs were virtualized, or for bypass
 * @pid_align: Requested alignment (power of 2) of reserved PID range,
 *             (pid_base will be multiple of this power of 2)
 * @res_mode: Select OS mechanism to pass job info to job processes
 * @lid_offset: Logical LID of the caller
 * @lid_count: Number of LIDs for this job
 * @auth_uid: Portals UIDs allowed for this job
 */
struct hfi_job_setup_args {
	IN __u64 pid_total;
	IN __u16 pid_base;
	IN __u16 pid_count;
	IN __u16 flags;
	IN __u16 pid_align;
	IN __u16 res_mode;
	IN __u32 lid_offset;
	IN __u32 lid_count;
	IN __u32 auth_uid[HFI_NUM_AUTH_TUPLES];
	IN __u64 reserved;
};

/*
 * struct hfi_at_prefetch - address translation prefetch for user buffers.
 * @iovec: an array of 'struct iovec' structure
 * @count: iovec count in the array
 * @flags: accessing info, readonly or readwrite
 */
struct hfi_at_prefetch_args {
	IN __u64 iovec;
	IN __u32 count;
	IN __u32 flags;
};

#define HFI_AT_READONLY		0x0
#define HFI_AT_READWRITE	0x1

/**
 * struct hfi_e2e_conn - An E2E connection request
 * @sl: service level
 * @port_num: initiator port number (starts from 1)
 * @slid: source LID
 * @dlid: destination LID
 * @pkey: e2e connection setup/teardown pkey
 * @conn_status: status returned as 0 for success or < 0 -ve errno for error
 */
struct hfi_e2e_conn {
	IN __u8 sl;
	IN __u8 port_num;
	IN __u32 slid;
	IN __u32 dlid;
	IN __u16 pkey;
	OUT __s32 conn_status;
};

/**
 * struct hfi_e2e_conn_args - Used to initiate E2E connection requests
 * @e2e_req: Array of E2E connection requests
 * @num_req: Number of entries in e2e_req array
 * @num_conn: Number of successful connections returned by the driver
 */
struct hfi_e2e_conn_args {
#ifdef __KERNEL__
	IN __user struct hfi_e2e_conn *e2e_req;
#else
	IN struct hfi_e2e_conn *e2e_req;
#endif
	IN __u32 num_req;
	OUT __u32 num_conn;
};

/**
 * enum hfi_erro_type - async error event type
 */
enum hfi_error_type {
	HFI_PGR_ERR = 1,	/* page group request error */
};

/**
 * page group request error info:
 * following macro to decode the page aligned virtual address
 * and accessing information (r)ead/(w)rite/(p)rivileged-access
 * when error happens (r and w can't both set):
 * 1: when p is set, requestor does not have privilege to
 *    access the privileged page (r/w is also set, but this
 *    is not the error cause).
 * 2: if p is not set,
 *    2.1: r is set, requestor tries to read from the page but
 *         the page is not readable.
 *    2.2: w is set, requestor tries to write to the page, but
 *         the page is not writeable.
 */
#define HFI_PGR_READ_MASK	0x1ULL
#define HFI_PGR_READ_SHIFT	1
#define HFI_PGR_WRITE_MASK	0x1ULL
#define HFI_PGR_WRITE_SHIFT	2
#define HFI_PGR_PRIVACS_MASK	0x1ULL
#define HFI_PGR_PRIVACS_SHIFT	3
#define HFI_PGR_VADDR_MASK	0xfffffffffffff000ULL
#define HFI_PGR_VADDR_SHIFT	0

/**
 * struct hfi_async_error - return async error info
 * @etype: async error event type, see "enum hfi_error_type"
 * @pgrinfo: page group request info: vaddr,read,write,pa.
 *           refer above mask/shift for coding.
 * @cookie: dummy item for future expension.
 */
struct hfi_async_error {
	OUT __u32 etype;
	OUT __u32 padding;
	union {
		OUT __u64 pgrinfo;
		OUT __u64 cookie;
	};
};

/**
 * struct hfi_async_error_args - User space to retrieve interrupt error
 * @ae: pointer to async error user space storage.
 * @timeout: time (ms unit) to wait until return;
 *   0x0:             return immediately, even if no error;
 *   0xffffffff (-1): wait until an error arrives.
 */
struct hfi_async_error_args {
	IN __u64 ae;
	IN __u32 timeout;
	IN __u32 padding;
};

/*
 * struct hfi_sl_pair - Used to check a given SL pair for portals
 * @sl1: first service level in the pair
 * @sl2: second service level in the pair
 * @port: port number
 */
struct hfi_sl_pair {
	__u8 sl1;
	__u8 sl2;
	__u8 port;
};

/*
 * struct hfi_hw_limit - Used to obtain resource limits
 * @num_pids_avail: total number of portal IDs available
 * @num_pids_limit: number of PIDs that can be allocated per process
 * @num_cmdq_pairs_avail: total number of command queue pairs available
 * @num_cmdq_pairs_limit: total number of command queue pairs that can be
 *			  allocated per process
 * @le_me_count_limit: number of LE/ME entries allowed per PID
 * @unexpected_count_limit: number of unexpected headers allowed per PID
 * @trig_op_count_limit: number of triggered operations allowed per PID
 */
struct hfi_hw_limit {
	OUT __u16 num_pids_avail;
	OUT __u16 num_pids_limit;
	OUT __u16 num_cmdq_pairs_avail;
	OUT __u16 num_cmdq_pairs_limit;
	OUT __u16 le_me_count_limit;
	OUT __u16 unexpected_count_limit;
	OUT __u16 trig_op_count_limit;
};

/*
 * struct hfi_ts_master_regs - Given a clock id and a port number, return
 * timesync master data
 *
 * @master: Time from master flits
 * @timestamp: Local clock timestamp of master flit arrival
 * @clkid: Clock id of current master from FM
 * @port: Desired port number
 */
struct hfi_ts_master_regs {
	OUT __u64 master;
	OUT __u64 timestamp;
	IN __u8 clkid;
	IN __u8 port;
};

/*
 * struct hfi_ts_fm_data - Given a port number, return FM provided data
 *
 * @clock_offset: FM provided fabric time offset
 * @clock_delay: FM provided propagation delay
 * @periodicity: FM provided filt update interval
 * @current_clock_id: FM provided current clock id
 * @ptp_index: Used to find /dev/ptpX
 * @is_active_master: If true this hfi is a timesync master clock
 * @port: Desired port number
 */
struct hfi_ts_fm_data {
	OUT __u64 clock_offset;
	OUT __u16 clock_delay;
	OUT __u16 periodicity;
	OUT __u8 current_clock_id;
	OUT __u8 ptp_index;
	OUT __u8 is_active_master;
	IN __u64 hack_timesync; /* Temporary Simics control */
	IN __u8 port;
};

/*
 * Masks and offsets defining the mmap tokens
 */
#define HFI_MMAP_OFFSET_MASK   0xfffULL
#define HFI_MMAP_OFFSET_SHIFT  0
#define HFI_MMAP_CTXT_MASK     0xfffULL
#define HFI_MMAP_CTXT_SHIFT    12
#define HFI_MMAP_TYPE_MASK     0xfULL
#define HFI_MMAP_TYPE_SHIFT    24
#define HFI_MMAP_NPAGES_MASK   0xfffULL
#define HFI_MMAP_NPAGES_SHIFT  32
#define HFI_MMAP_MAGIC_MASK    0xfffffULL
#define HFI_MMAP_MAGIC_SHIFT   44
#define HFI_MMAP_MAGIC         0xdabba

#define HFI_MMAP_TOKEN_SET(field, val)	\
	(((val) & HFI_MMAP_##field##_MASK) << HFI_MMAP_##field##_SHIFT)
#define HFI_MMAP_TOKEN_GET(field, token) \
	(((token) >> HFI_MMAP_##field##_SHIFT) & HFI_MMAP_##field##_MASK)
#define HFI_MMAP_TOKEN_GET_SIZE(token) \
	(HFI_MMAP_TOKEN_GET(NPAGES, token) << PAGE_SHIFT)
#define HFI_MMAP_TOKEN(type, ctxt, addr, size)   \
	(HFI_MMAP_TOKEN_SET(MAGIC, HFI_MMAP_MAGIC) | \
	 HFI_MMAP_TOKEN_SET(TYPE, type) | \
	 HFI_MMAP_TOKEN_SET(CTXT, ctxt) | \
	 HFI_MMAP_TOKEN_SET(NPAGES, ((size) >> PAGE_SHIFT)) | \
	 HFI_MMAP_TOKEN_SET(OFFSET, ((unsigned long)(addr) & ~PAGE_MASK)))

#endif /* _HFI_CMD_H */
