/*
 * Copyright (c) 2014 Intel Corporation. All rights reserved.
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

#ifndef _HFI_CMD_H
#define _HFI_CMD_H

#include <linux/types.h>

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

/* Gen 1: reserved for not yet defined PSM API */
#define HFI_CMD_ASSIGN_CTXT       1
#define HFI_CMD_CTXT_INFO         2
#define HFI_CMD_CTXT_SETUP        3
#define HFI_CMD_USER_INFO         4
#define HFI_CMD_TID_UPDATE        5
#define HFI_CMD_TID_FREE          6
#define HFI_CMD_CREDIT_UPD        7
#define HFI_CMD_SDMA_STATUS_UPD   8
#define HFI_CMD_RECV_CTRL         9
#define HFI_CMD_POLL_TYPE         10
#define HFI_CMD_ACK_EVENT         11
#define HFI_CMD_SET_PKEY          12
#define HFI_CMD_CTXT_RESET        13
/* Gen 2: */
#define HFI_CMD_PTL_ATTACH        20
#define HFI_CMD_PTL_DETACH        21
#define HFI_CMD_CQ_ASSIGN         22
#define HFI_CMD_CQ_RELEASE        23
#define HFI_CMD_EQ_ASSIGN         24
#define HFI_CMD_EQ_RELEASE        25
#define HFI_CMD_DLID_ASSIGN       26
#define HFI_CMD_DLID_RELEASE      27
#define HFI_CMD_JOB_INFO          28
#define HFI_CMD_JOB_SETUP         29

struct hfi_cmd {
	__u32 type;		/* command type */
	__u32 length;		/* length of user buffer */
	__u64 context;		/* context data for this command */
};

#ifdef __KERNEL__
#define HFI_LID_ANY (__u32)(-1)
#define HFI_NID_ANY (__u32)(-1)
#define HFI_PID_ANY (__u16)(-1)
#define HFI_PID_NONE (__u16)(-1)
#define HFI_UID_ANY (__u32)(-1)
typedef __u32 hfi_lid_t;
typedef __u16 hfi_pid_t;
typedef __u32 hfi_uid_t;
#endif

#define IN  /* input argument */
#define OUT /* output argument */

#define HFI_CQ_TX 0
#define HFI_CQ_RX 1

struct hfi_cq_assign_args {
	OUT __u8 auth_idx;
	OUT __u16 cq_idx;
	OUT __u64 cq_tx_token;
	OUT __u64 cq_rx_token;
	OUT __u64 cq_head_token;
};

struct hfi_cq_release_args {
	IN  __u16 cq_idx;
};

/* TODO - dlid granularity?, implicit in count? */
struct hfi_dlid_assign_args {
	IN __u32 dlid_base;
	IN __u32 count;
	IN __u32 *dlid_entries_ptr;
};

struct hfi_ptl_attach_args {
	IN  __u32 api_version;  /* HFI_USER_SWVERSION */
	IN  hfi_pid_t pid;
	IN  __u16 srank;        /* for logical matching */
	IN  __u16 flags;        /* PID assignment flags */
	IN  __u16 le_me_count;
	IN  __u16 unexpected_count;
	IN  __u16 trig_op_count;

	OUT hfi_pid_t pid_base;
	OUT __u16 pid_count;
	OUT __u16 pid_mode;	  /* tells if LIDs/PIDs virtualized */
	OUT __u64 ct_token;       /* mmap: Counting Events */
	OUT __u64 eq_desc_token;  /* mmap: EQ descriptors */
	OUT __u64 eq_head_token;  /* mmap: EQ read pointer */
	OUT __u64 pt_token;       /* mmap: Portals Table */
	OUT __u64 le_me_token;    /* mmap: LE/ME list */
	OUT __u64 le_me_unlink_token;  /* mmap: freed LE/MEs */
	OUT __u64 unexpected_token;    /* mmap: unexpected list */
	OUT __u64 trig_op_token;       /* mmap: trig op list */
};

struct hfi_job_info_args {
	IN  __u32 api_version;  /* HFI_USER_SWVERSION */
	OUT hfi_pid_t pid_base;
	OUT __u16 pid_count;
	OUT __u16 pid_mode;
	OUT hfi_lid_t dlid_base;
	OUT __u32 lid_offset;
	OUT __u32 lid_count;
	OUT __u32 sl_mask;
	OUT hfi_uid_t auth_table[HFI_NUM_AUTH_TUPLES];
};

#define HFI_JOB_RES_SESSION 0x1
#define HFI_JOB_RES_CGROUP  0x2

struct hfi_job_setup_args {
	IN __u32 api_version;  /* HFI_USER_SWVERSION */
	IN hfi_pid_t pid_base;
	IN __u16 pid_count;
	IN __u16 pid_mode;
	IN __u16 res_mode;
	IN __u32 lid_offset;
	IN __u32 lid_count;
	IN __u32 sl_mask;
	IN hfi_uid_t auth_table[HFI_NUM_AUTH_TUPLES];
};

#endif /* _HFI_CMD_H */
