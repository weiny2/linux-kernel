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
 * Intel(R) Omni-Path Gen2 HFI PCIe Driver
 */
#ifndef _OPA_HFI_H
#define _OPA_HFI_H

#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <rdma/ib_smi.h>
#include <rdma/hfi_cmd.h>
#include <rdma/opa_core.h>
#include <rdma/fxr/fxr_linkmux_defs.h>
#include <rdma/fxr/fxr_linkmux_tp_defs.h>
#include <rdma/fxr/fxr_linkmux_fpc_defs.h>
#include <rdma/fxr/fxr_linkmux_cm_defs.h>
#include "verbs/mad.h"
#include "firmware.h"
#include "verbs/verbs.h"

extern unsigned int hfi_max_mtu;
#define DRIVER_NAME		KBUILD_MODNAME
#define DRIVER_CLASS_NAME	DRIVER_NAME

enum {
	SHARED_CREDITS,
	DEDICATED_CREDITS
};

enum {
	/* Port type is undefined */
	HFI_PORT_TYPE_UNKNOWN = 0,
	/* port is not currently usable, CableInfo not available */
	HFI_PORT_TYPE_DISCONNECTED = 1,
	/* A fixed backplane port in a director class switch. All HFI ASICS */
	HFI_PORT_TYPE_FIXED = 2,
	/* A backplane port in a blade system, possibly mixed configuration */
	HFI_PORT_TYPE_VARIABLE = 3,
	/* Implies a SFF-8636 defined format for CableInfo (QSFP) */
	HFI_PORT_TYPE_STANDARD = 4,
	/*
	 * Intel Silicon Photonics x16 mid-board module connector is
	 * integrated onto system board
	 */
	HFI_PORT_TYPE_SI_PHOTONICS = 5,
	/* 6 - 15 are reserved */
};

/* partition enforcement flags */
#define HFI_PART_ENFORCE_IN	BIT(0)
#define HFI_PART_ENFORCE_OUT	BIT(1)

#define HFI_IB_CC_TABLE_CAP_DEFAULT 31
#define HFI_FXR_BAR		0
#define HFI_NUM_PPORTS		2
#define HFI_MAX_VLS		32
#define HFI_PID_SYSTEM		0
#define HFI_PID_BYPASS_BASE	0xF00
#define HFI_NUM_BYPASS_PIDS	160
/* last PID not usable as an RX context (0xFFF reserved as match any PID) */
#define HFI_NUM_USABLE_PIDS	(HFI_NUM_PIDS - 1)
#define HFI_TPID_ENTRIES	16
#define HFI_DLID_TABLE_SIZE	(64 * 1024)

#define IS_PID_VIRTUALIZED(ctx) \
	((ctx)->mode & (HFI_CTX_MODE_VTPID_MATCHING | \
			HFI_CTX_MODE_VTPID_NONMATCHING))

/*
 * For TPID_CAM.UID, use first value from resource manager (if set).
 * This value is inherited during open() and returned to the user as
 * their default UID.
 */
#define TPID_UID(ctx) \
	(((ctx)->auth_mask & 0x1) ? (ctx)->auth_uid[0] : (ctx)->ptl_uid)

/* FXRTODO: based on 16bit (9B) LID */
#define HFI_MULTICAST_LID_BASE	0xC000
/* Maximum number of traffic classes supported */
#define HFI_MAX_TC		4
/* Maximum number of message classes supported */
#define HFI_MAX_MC		2

/* Maximum number of unicast LIDs supported */
#define HFI_MAX_LID_SUPP	(0xBFFF)

/*
 * Size of packet sequence number state assuming LMC = 0
 * The same PSN buffer is used for both TX and RX and hence
 * the multiplication by 2
 */
#define HFI_PSN_SIZE		(2 * HFI_MAX_LID_SUPP * 8)

/* TX timeout for E2E control messages */
#define HFI_TX_TIMEOUT_MS	100

/* timeout for EQ assignment */
#define HFI_EQ_WAIT_TIMEOUT_MS	500
#define HFI_VL_STATUS_CLEAR_TIMEOUT	5000	/* per-VL status clear, in ms */

/* use this MTU size if none other is given */
#define HFI_DEFAULT_ACTIVE_MTU	10240
/* use this MTU size as the default maximum */
#define HFI_DEFAULT_MAX_MTU	10240
/* The largest MAD packet size. */
#define HFI_MIN_VL_15_MTU	2048
/* Number of Data VLs supported */
#define HFI_NUM_DATA_VLS	9
/* Number of PKey entries in the HW */
#define HFI_MAX_PKEYS		64

/* In accordance with stl vol 1 section 4.1 */
#define PGUID_MASK		(~(0x3UL << 32))
#define PORT_GUID(ng, pn)	(((be64_to_cpu(ng)) & PGUID_MASK) |\
				 (((u64)pn) << 32))

#define pidx_to_pnum(id)	((id) + 1)
#define pnum_to_pidx(pn)	((pn) - 1)

#define HFI_PKEY_CAM_SHIFT		0
#define HFI_PKEY_CAM_MASK		0x7fff
#define HFI_PKEY_CAM(pkey)		(((pkey) >> HFI_PKEY_CAM_SHIFT) & \
					HFI_PKEY_CAM_MASK)

#define HFI_PKEY_MEMBER_SHIFT		15
#define HFI_PKEY_MEMBER_MASK		0x1
#define HFI_PKEY_MEMBER_TYPE(pkey)	(((pkey) >> HFI_PKEY_MEMBER_SHIFT) & \
					HFI_PKEY_MEMBER_MASK)

#define HFI_SC_TO_TC_MC_MASK	FXR_LM_CFG_PORT1_SC2MCTC0_SC0_TO_MCTC_MASK

#define HFI_SL_TO_TC_MC_MASK	(FXR_TXCID_CFG_SL0_TO_TC_SL0_P0_TC_MASK | \
			(FXR_TXCID_CFG_SL0_TO_TC_SL0_P0_MC_MASK << \
			FXR_TXCID_CFG_SL0_TO_TC_SL0_P0_MC_SHIFT))

#define HFI_GET_TC(mctc)	((mctc) & \
				(u8)(FXR_TXCID_CFG_SL0_TO_TC_SL0_P0_TC_MASK))

#define HFI_SL_TO_SC_MASK	FXR_LM_CFG_PORT0_SL2SC0_SL0_TO_SC_MASK

#define HFI_SC_TO_RESP_SL_MASK	FXR_LM_CFG_PORT0_SC2SL0_SC0_TO_SL_MASK

/* Any lane between 8 and 14 is illegal. Randomly chosen one from that list */
#define HFI_ILLEGAL_VL			12

#define HFI_SC_VLT_MASK			FXR_TP_CFG_SC_TO_VLT_MAP_ENTRY0_MASK

#define HFI_SC_VLNT_MASK		FXR_TP_CFG_CM_SC_TO_VLT_MAP_ENTRY0_MASK

#define HFI_SC_VLR_MASK			FXR_FPC_CFG_SC_VL_TABLE_15_0_ENTRY0_MASK

/* Message class and traffic class for VL15 packets */
#define HFI_VL15_MC			0
#define HFI_VL15_TC			3

#define hfi_vl_to_idx(vl)		(((vl) < HFI_NUM_DATA_VLS) ? \
				 (vl) : HFI_NUM_DATA_VLS)
#define hfi_idx_to_vl(idx)		(((idx) == HFI_NUM_DATA_VLS) ? \
				 15 : (idx))
#define hfi_valid_vl(idx)	((idx) < HFI_NUM_DATA_VLS || (idx) == 15)
#define HFI_NUM_USABLE_VLS 16	/* look at VL15 and less */

/* verify capability fabric fields */
#define HFI_VAU_SHIFT	0
#define HFI_VAU_MASK	0x0007
#define HFI_Z_SHIFT		3
#define HFI_Z_MASK		0x0001
#define HFI_VCU_SHIFT	4
#define HFI_VCU_MASK	0x0007
#define HFI_VL15BUF_SHIFT	8
#define HFI_VL15BUF_MASK	0x0fff
#define HFI_CRC_SIZES_SHIFT 20
#define HFI_CRC_SIZES_MASK	0x7
/*
 * Virtual Allocation Unit, defined as AU = 8*2^vAU, 64 bytes, AU is fixed
 * at 64 bytes for all generation one devices
 */
#define HFI_CM_VAU 3
#define HFI_CM_CU  1
/* in Bytes
 * FXRTODO: Until we have a register to read this value from
 * or a constant defined in the headers generated from CSR
 * use this harcoded value.
 */
#define HFI_RCV_BUFFER_SIZE		(128 * 1024)

/*
 * FXRTODO: Get rid of hard coded number of SL pairs and SL start once the FM
 * provides it
 */
/* Number of SL pairs used by Portals */
#define HFI_NUM_PTL_SLP 2

/*
 * Starting SL for Portals traffic. The following pairs are reserved for
 * portals. [0, 1], [2, 3]
 *
 */
#define HFI_PTL_SL_START 0

/*
 * HFI or Host Link States
 *
 * These describe the states the driver thinks the logical and physical
 * states are in.  Used as an argument to set_link_state().  Implemented
 * as bits for easy multi-state checking.  The actual state can only be
 * one.
 *
 * FXRTODO: STL2 related states need to be added
 * Ganged, ShallowSleep and DeepSleep.
 */
#define __HLS_UP_INIT_BP	0
#define __HLS_UP_ARMED_BP	1
#define __HLS_UP_ACTIVE_BP	2
#define __HLS_DN_DOWNDEF_BP	3	/* link down default */
#define __HLS_DN_POLL_BP	4
#define __HLS_DN_DISABLE_BP	5
#define __HLS_DN_OFFLINE_BP	6
#define __HLS_VERIFY_CAP_BP	7
#define __HLS_GOING_UP_BP	8
#define __HLS_GOING_OFFLINE_BP  9
#define __HLS_LINK_COOLDOWN_BP 10

#define HLS_UP_INIT	  (1 << __HLS_UP_INIT_BP)
#define HLS_UP_ARMED	  (1 << __HLS_UP_ARMED_BP)
#define HLS_UP_ACTIVE	  (1 << __HLS_UP_ACTIVE_BP)
#define HLS_DN_DOWNDEF	  (1 << __HLS_DN_DOWNDEF_BP) /* link down default */
#define HLS_DN_POLL	  (1 << __HLS_DN_POLL_BP)
#define HLS_DN_DISABLE	  (1 << __HLS_DN_DISABLE_BP)
#define HLS_DN_OFFLINE	  (1 << __HLS_DN_OFFLINE_BP)
#define HLS_VERIFY_CAP	  (1 << __HLS_VERIFY_CAP_BP)
#define HLS_GOING_UP	  (1 << __HLS_GOING_UP_BP)
#define HLS_GOING_OFFLINE (1 << __HLS_GOING_OFFLINE_BP)
#define HLS_LINK_COOLDOWN (1 << __HLS_LINK_COOLDOWN_BP)

#define HLS_UP (HLS_UP_INIT | HLS_UP_ARMED | HLS_UP_ACTIVE)

/* SMA idle flit payload commands */
#define SMA_IDLE_ARM	1
#define SMA_IDLE_ACTIVE 2

enum {
	HFI_IB_CFG_LIDLMC = 0,		/* LID and Mask*/
	HFI_IB_CFG_LWID_DG_ENB,		/* allowed Link-width downgrade */
	HFI_IB_CFG_LWID_ENB,		/* allowed Link-width */
	HFI_IB_CFG_LWID,		/* currently active Link-width */
	HFI_IB_CFG_SPD_ENB,		/* allowed Link speeds */
	HFI_IB_CFG_SPD,			/* current Link spd */
	HFI_IB_CFG_OP_VLS,		/* operational VLs */
	HFI_IB_CFG_PKEYS,		/* update partition keys */
	HFI_IB_CFG_MTU,			/* update MTU in IBC */
	HFI_IB_CFG_VL_HIGH_LIMIT,	/* Change VL high limit */
	HFI_IB_CFG_SL_TO_SC,		/* Change SLtoSC mapping */
	HFI_IB_CFG_SC_TO_RESP_SL,	/* Change SCtoRespSL mapping */
	HFI_IB_CFG_SC_TO_MCTC,		/* Change SCtoMCTC mapping */
	HFI_IB_CFG_SC_TO_VLR,		/* Change SCtoVLr mapping */
	HFI_IB_CFG_SC_TO_VLT,		/* Change SCtoVLt mapping */
	HFI_IB_CFG_SC_TO_VLNT,		/* Change Neighbor's SCtoVL mapping */
};

/* verify capability fabric CRC size bits */
enum {
	HFI_CAP_CRC_14B = (1 << 0), /* 14b CRC */
	HFI_CAP_CRC_48B = (1 << 1), /* 48b CRC */
	HFI_CAP_CRC_12B_16B_PER_LANE = (1 << 2) /* 12b-16b per lane CRC */
};

#define HFI_SUPPORTED_CRCS (HFI_CAP_CRC_14B | HFI_CAP_CRC_48B)

/* LCB_CFG_CRC_MODE TX_VAL and RX_VAL CRC mode values */
enum {
	HFI_LCB_CRC_16B = 0,	/* 16b CRC */
	HFI_LCB_CRC_14B = 1,	/* 14b CRC */
	HFI_LCB_CRC_48B = 2,	/* 48b CRC */
	HFI_LCB_CRC_12B_16B_PER_LANE = 3	/* 12b-16b per lane CRC */
};

#define HFI_LTP_CRC_SUPPORTED_SHIFT	8
#define HFI_LTP_CRC_SUPPORTED_MASK	0xf
#define HFI_LTP_CRC_SUPPORTED(val)	(((val) >> \
					HFI_LTP_CRC_SUPPORTED_SHIFT) & \
					HFI_LTP_CRC_SUPPORTED_MASK)

#define HFI_LTP_CRC_ENABLED_SHIFT	4
#define HFI_LTP_CRC_ENABLED_MASK	0xf
#define HFI_LTP_CRC_ENABLED(val)	(((val) >> \
					HFI_LTP_CRC_ENABLED_SHIFT) & \
					HFI_LTP_CRC_ENABLED_MASK)

#define HFI_LTP_CRC_ACTIVE_SHIFT	0
#define HFI_LTP_CRC_ACTIVE_MASK		0xf
#define HFI_LTP_CRC_ACTIVE(val)		(((val) >> \
					HFI_LTP_CRC_ENABLED_SHIFT) & \
					HFI_LTP_CRC_ACTIVE_MASK)

/*
 * FXRTODO: VL ARB is absent for FXR. Remove these
 * once we have STL2 specific opafm
 */
#define HFI_VL_ARB_TABLE_SIZE		16
#define OPA_VLARB_LOW_ELEMENTS		0
#define OPA_VLARB_HIGH_ELEMENTS		1
#define OPA_VLARB_PREEMPT_ELEMENTS	2
#define OPA_VLARB_PREEMPT_MATRIX	3

struct vl_limit {
	__be16 dedicated;
	__be16 shared;
};

struct buffer_control {
	__be16 reserved;
	__be16 overall_shared_limit;
	struct vl_limit vl[OPA_MAX_VLS];
};

struct hfi_link_down_reason {
	/*
	 * SMA-facing value.  Should be set from .latest when
	 * HLS_UP_* -> HLS_DN_* transition actually occurs.
	 */
	u8 sma;
	u8 latest;
};

struct err_info_rcvport {
	u8 status_and_code;
	u64 packet_flit1;
	u64 packet_flit2;
};

struct err_info_constraint {
	u8 status;
	u16 pkey;
	u32 slid;
};

struct hfi_msix_entry {
	struct msix_entry msix;
	void *arg;
	cpumask_var_t mask;
	struct list_head irq_wait_head;
	rwlock_t irq_wait_lock;
	struct hfi_devdata *dd;
	int intr_src;
};

struct hfi_event_queue {
	struct list_head irq_wait_chain;
	wait_queue_head_t wq;
	u32 irq_vector;
	void (*isr_cb)(struct hfi_eq *eq, void *cookie);
	void *cookie;
	struct hfi_eq desc;
};

/*
 * hfi_ptcdata - HFI traffic class specific information per port
 * @psn_base: packet sequence number buffer used for TX/RX
 * @e2e_tx_state_cache: E2E connection TX state cache
 * @e2e_rx_state_cache: E2E connection RX state cache
 * @max_e2e_dlid: Maximum DLID to which an E2E connection has been
 *	established which is used to detect the DLID till which
 *	destroy messages have to be sent during driver unload
 * @sl: request SL of the SL pairs used for E2E connection
 */
struct hfi_ptcdata {
	void *psn_base;
	struct ida e2e_tx_state_cache;
	struct ida e2e_rx_state_cache;
	u32 max_e2e_dlid;
	u8 req_sl;
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 3, 0)
/*
 * FXRTODO: VL ARB is absent for FXR. Remove these
 * once we have STL2 specific opafm
 */
struct ib_vl_weight_elem {
	u8      vl;     /* Only low 4 bits, upper 4 bits reserved */
	u8      weight;
};
#endif

/*
 * hfi_pportdata - HFI port specific information
 *
 * @dd: pointer to the per node hfi_devdata
 * @pguid: port_guid identifying port
 * @neighbor_guid: Node guid of the neighboring port
 * @lid: LID for this port
 * @ptc: per traffic class specific fields
 * @lstate: Logical link state
 * @ibmtu: The MTU programmed for this port
 * @port_error_action: contains bit mask for various errors. The HFI
 *	should initiate link-bounce when the corresponding error occurs.
 * @port_type: Type of physical port
 * @sm_trap_qp: qp number to send trap to SM from SMA
 * @sa_qp: SA qp number
 * @link_width_supported: Supported link width
 * @link_width_downgrade_supported: Supported link width downgrading
 * @link_speed_supported: Supported link speed
 * @link_width_enabled: Enabled link width
 * @link_width_downgrade_enabled: Enabled link width downgrading
 * @link_speed_enabled: Enabled link speed
 * @link_width_active: Currently active link width
 * @link_width_downgrade_tx_active: Current TX side link width downgrading
 * @link_width_downgrade_rx_active: Current RX side link width downgrading
 * @link_speed_active: Current active link speed
 * @port_ltp_crc_mode: Supported, enabled and active LTP CRC modes
 * @port_crc_mode_enabled: CRC mode to be enabled
 * @lmc: LID mask control
 * @pnum: port number of this port
 * @vls_supported: Virtual lane supported
 * @vls_operational: Virtual lane operational
 *	VL Arbitration table
 * @neighbor_type: Node type of neighboring port
 *			0 - HFI
 *			1 - Switch
 * @neighbor_normal: State of neighbor's port's Logical link state
 *	0 - Neighbor Down/Init
 *	1 - Neighbhor LinkArmed/LinkActive
 * @neighbor_fm_security: Configuration of a switch pin which can be used to
 *	disable firmware authentication during switch ASIC boot.
 *	0 - Authenticated
 *	1 - Authentication bypassed
 * @neighbor_port_number: Port number of the neighboring port
 * @is_sm_config_started: indicates that FM has started configuration for this
 *	port.
 * @offline_disabled_reason: offline reason
 * @is_active_optimize_enabled: if enabled, then LinkArmed -> LinkActive state
 *	change propogates this event to neighboring port via SMA Idle Flit
 *	messages
 * @local_tx_rate: rate given to 8051 firmware
 * @mgmt_allowed: Indicates if neighbor is allowing this node to be a mgmt node
 *	(information received via LNI)
 * @part_enforce: Partition enforcement flags
 * @sl_to_sc: service level to service class mapping table
 * @sc_to_sl: service class to service level mapping table
 * @sc_to_resp_sl: service class to response service level mapping table
 *	This is only used for portals traffic
 * @sc_to_vlr: service class to (RX) virtual lane table
 * @sc_to_vlt: service class to (TX) virtual lane table
 * @sc_to_vlnt: service class to (RX) neighbor virtual lane table
 * @sl_to_mctc: service level to traffic class & message class mapping
 * @sc_to_mctc: service class to traffic class & message class mapping
 * @ptl_slp: SL pairs reserved for portals
 * @num_ptl_slp: number of SL pairs reserved for portals
 * @bct: buffer control table
 *@local_link_down_reason: Reason why this port transitioned to link down
 *@local_link_down_reason: Reason why the neighboring port transitioned to
 *	link down
 *@remote_link_down_reason: Value to be sent to link peer on LinkDown
 *@ccti_entries: List of congestion control table entries
 *@total_cct_entry: Total number of congestion control table entries
 *@cc_sl_control_map: Congestion control Map (Mask)
 *@cc_max_table_entries: CA's max number of 64 entry units in the
 * congestion control table
 *@cc_state_lock: lock for cc_state
 *@cc_state: Congestion control state
 *@congestion_entries: Congestion control entries
 *@crk8051_lock: for exclusive access to 8051
 *@crk8051_timed_out: remember if the 8051 timed out
 *@hfi_wq: workqueue which connects the upper half interrupt handler,
 *		irq_mnh_handler() and bottom half interrupt handlers which are
 *		queued by each work_struct.
 *@link_vc_work: for VerifyCap -> GoingUp/ConfigLT
 *@link_up_work: for GoingUp/ConfigLT -> LinkUp/Init
 *@link_down_work: for LinkUp -> LinkDown
 *@sma_message_work: for the interrupt caused by "Receive a back channel msg
 *		using LCB idle protocol HOST Type SMA"
 *@vau: Virtual allocation unit for this port
 *@vcu: Virtual return credit unit for this port
 *@link_credits: link credits of this device
 *@vl15_init: Initial vl15 credits to use
 */
struct hfi_pportdata {
	struct hfi_devdata *dd;
	__be64 pguid;
	__be64 neighbor_guid;
	u32 lid;
	struct hfi_ptcdata ptc[HFI_MAX_TC];
	/* host link state which keeps both Physical Port and Logical Link
	   state by having HLS_* */
	struct mutex hls_lock;
	u32 host_link_state;
	u32 lstate;
	u32 ibmtu;
	u32 port_error_action;
	u32 port_type;
	u32 sm_trap_qp;
	u32 sa_qp;
	u16 pkeys[HFI_MAX_PKEYS];
	u16 link_width_supported;
	u16 link_width_downgrade_supported;
	u16 link_speed_supported;
	u16 link_width_enabled;
	u16 link_width_downgrade_enabled;
	u16 link_speed_enabled;
	u16 link_width_active;
	u16 link_width_downgrade_tx_active;
	u16 link_width_downgrade_rx_active;
	u16 link_speed_active;
	u16 port_ltp_crc_mode;
	u8 linkinit_reason;
	u8 port_crc_mode_enabled;
	u8 lmc;
	u8 pnum;
	u8 vls_supported;
	u8 vls_operational;
	u8 neighbor_type;
	u8 neighbor_normal;
	u8 neighbor_fm_security;
	u8 neighbor_port_number;
	u8 is_sm_config_started;
	u8 offline_disabled_reason;
	u8 is_active_optimize_enabled;
	u8 local_tx_rate;
	u8 mgmt_allowed;
	u8 part_enforce;
	u8 sl_to_sc[OPA_MAX_SLS];
	u8 sc_to_sl[OPA_MAX_SCS];
	u8 sc_to_resp_sl[OPA_MAX_SCS];
	u8 sc_to_vlr[OPA_MAX_SCS];
	u8 sc_to_vlt[OPA_MAX_SCS];
	u8 sc_to_vlnt[OPA_MAX_SCS];
	u8 sl_to_mctc[OPA_MAX_SLS];
	u8 sc_to_mctc[OPA_MAX_SCS];
	u8 ptl_slp[OPA_MAX_SLS / 2][HFI_MAX_MC];
	u8 num_ptl_slp;
	u16 vl_mtu[OPA_MAX_VLS];
	struct hfi_link_down_reason local_link_down_reason;
	struct hfi_link_down_reason neigh_link_down_reason;
	struct buffer_control bct;
	u8 remote_link_down_reason;

	/* Congestion control */
	struct ib_cc_table_entry_shadow ccti_entries[HFI_CC_TABLE_SHADOW_MAX];
	u16 total_cct_entry;
	u32 cc_sl_control_map;
	u8 cc_max_table_entries;
	spinlock_t cc_state_lock ____cacheline_aligned_in_smp;
	struct cc_state __rcu *cc_state;
	struct opa_congestion_setting_entry_shadow
		congestion_entries[OPA_MAX_SLS];

	/* Credit Mgmt/Buffer control */
	u8 vau;
	u8 vcu;
	u16 link_credits;
	u16 vl15_init;

	/*
	 * FXRTODO: VL ARB is absent for FXR. Remove these
	 * once we have STL2 specific opafm
	 */
	struct ib_vl_weight_elem vl_arb_low[HFI_VL_ARB_TABLE_SIZE];
	struct ib_vl_weight_elem vl_arb_high[HFI_VL_ARB_TABLE_SIZE];
	struct ib_vl_weight_elem vl_arb_prempt_ele[HFI_VL_ARB_TABLE_SIZE];
	struct ib_vl_weight_elem vl_arb_prempt_mat[HFI_VL_ARB_TABLE_SIZE];

	/* for exclusive access to 8051 */
	spinlock_t crk8051_lock;
	int crk8051_timed_out;	/* remember if the 8051 timed out */

	/*
		workqueue which connects the upper half interrupt handler,
		irq_mnh_handler() and bottom half interrupt handlers which are
		queued by each work_struct.
	 */
	struct workqueue_struct *hfi_wq;
	struct work_struct link_vc_work; /* for VerifyCap -> GoingUp/ConfigLT */
	struct work_struct link_up_work; /* for GoingUp/ConfigLT -> LinkUp/Init */
	struct work_struct link_down_work; /* for LinkUp -> LinkDown */
	/* for the interrupt caused by "Receive a back channel msg using LCB idle
	   protocol HOST Type SMA" */
	struct work_struct sma_message_work;

#ifdef CONFIG_DEBUG_FS
	struct dentry *hfi_port_dbg;
#endif
};

/*
 * device data struct contains only per-HFI info.
 *
 *@fw_mutex: The mutex protects fw_state, fw_err, and all of the
 * 		firmware_details variables.
 *@fw_state: keep firmware status, FW_EMPTY, FW_TRY, FW_FINAL, FW_ERR
 *@fw_err: keep firmware loading error code.
 *@fw_8051_name: Firmware file names get set in firmware_init()
 *@fw_8051_load: indicate whether 8051 firmware should be loaded
 *@fw_8051: 8051 firmware loaded in memory
 *@crk8051_ver: 8051 firmware version
 */
struct hfi_devdata {
	/* pci access data structure */
	struct pci_dev *pcidev;
	struct opa_core_device *bus_dev;

	/* localbus width (1, 2,4,8,16,32) from config space  */
	u32 lbus_width;
	/* localbus speed in MHz */
	u32 lbus_speed;
	int unit; /* unit # of this chip */
	int node; /* home node of this chip */
	/* mem-mapped pointer to base of chip regs */
	u8 __iomem *kregbase;
	u8 __iomem *kregend;
	/* physical address of chip for io_remap, etc. */
	resource_size_t physaddr;

	struct opa_core_device_id bus_id;

	/* MSI-X information */
	struct hfi_msix_entry *msix_entries;
	u32 num_msix_entries;
	u32 num_eq_irqs;
	atomic_t msix_eq_next;

	/* RSM */
	u8 rsm_offset[HFI_NUM_RSM_RULES];
	u16 rsm_size[HFI_NUM_RSM_RULES];
	DECLARE_BITMAP(rsm_map, HFI_RSM_MAP_SIZE);

	/* Device Portals State */
	struct idr ptl_user;
	struct idr ptl_tpid;
	DECLARE_BITMAP(ptl_map, HFI_NUM_PIDS);
	u16 vl15_pid;
	u16 bypass_pid;
	spinlock_t ptl_lock;
	struct hfi_ctx priv_ctx;
	struct hfi_cq priv_tx_cq;
	struct hfi_cq priv_rx_cq;
	struct err_info_rcvport err_info_rcvport;
	struct err_info_constraint err_info_rcv_constraint;
	struct err_info_constraint err_info_xmit_constraint;
	u8 err_info_uncorrectable;
	u8 err_info_fmconfig;

	/* Command Queue State */
	struct idr cq_pair;
	spinlock_t cq_lock;
	void *cq_tx_base;
	void *cq_rx_base;
	void *cq_head_base;
	size_t cq_head_size;

	/* iommu system pasid */
	u32 system_pasid;

	/* node_guid identifying node */
	__be64 nguid;

	/* Number of physical ports available */
	u8 num_pports;

	/*
	 * hfi_pportdata, points to array of port
	 * port-specifix data structs
	 */
	struct hfi_pportdata *pport;

	/* OUI comes from the HW. Used everywhere as 3 separate bytes. */
	u8 oui[3];

	/* Lock to synchronize access to tx_priv_cq */
	spinlock_t priv_tx_cq_lock;

	/* Lock to synchronize access to rx_priv_cq */
	spinlock_t priv_rx_cq_lock;

	/* Thread to handle PID 0 EQ 0 tasks */
	struct task_struct *eq_zero_thread;

	/* Mutex lock synchronizing E2E operations */
	struct mutex e2e_lock;

	/* E2E EQ handle for initiator events */
	struct hfi_eq e2e_eq;

	/* registered IB device data pointer */
	struct hfi2_ibdev *ibd;

	/* number of portal IDs in use */
	u16 pid_num_assigned;

	/* number of command queues in use */
	u16 cq_pair_num_assigned;

	/* for firmware download */
	struct mutex fw_mutex;
	enum fw_state fw_state;
	int fw_err;
	/* Firmware file names get set in firmware_init() */
	char *fw_8051_name;
	bool fw_8051_load;
	struct firmware_details fw_8051;
	u16 crk8051_ver; /* 8051 firmware version */

	/* chip minor rev, from CceRevision */
	u8 minrev;
#ifdef CONFIG_DEBUG_FS
	/* per HFI debugfs */
	struct dentry *hfi_dev_dbg;
	/* per HFI symlinks to above */
	struct dentry *hfi_dev_link;
#endif
};

/*
 * Readers of cc_state must call get_cc_state() under rcu_read_lock().
 * Writers of cc_state must call get_cc_state() under cc_state_lock.
 */
static inline struct cc_state *hfi_get_cc_state(struct hfi_pportdata *ppd)
{
	return rcu_dereference(ppd->cc_state);
}

/* return the driver's idea of the logical OPA port state */
static inline u32 hfi_driver_lstate(struct hfi_pportdata *ppd)
{
	return ppd->lstate; /* use the cached value */
}

int hfi_pci_init(struct pci_dev *pdev, const struct pci_device_id *ent);
void hfi_pci_cleanup(struct pci_dev *pdev);
struct hfi_devdata *hfi_pci_dd_init(struct pci_dev *pdev,
				    const struct pci_device_id *ent);
void hfi_pci_dd_free(struct hfi_devdata *dd);
int hfi_pcie_params(struct hfi_devdata *dd, u32 minw, u32 *nent,
		    struct hfi_msix_entry *entry);
void hfi_disable_msix(struct hfi_devdata *dd);
int hfi_setup_interrupts(struct hfi_devdata *dd, int total, int minw);
void hfi_cleanup_interrupts(struct hfi_devdata *dd);
void hfi_disable_interrupts(struct hfi_devdata *dd);

struct hfi_devdata *hfi_alloc_devdata(struct pci_dev *pdev);
void hfi_cc_state_reclaim(struct rcu_head *rcu);

/* HFI specific functions */
int hfi_cq_assign_privileged(struct hfi_ctx *ctx, u16 *cq_idx);
int hfi_eq_zero_assign_privileged(struct hfi_ctx *ctx);
void hfi_eq_zero_release(struct hfi_ctx *ctx);
int hfi_e2e_eq_assign(struct hfi_ctx *ctx);
void hfi_e2e_eq_release(struct hfi_ctx *ctx);
void hfi_cq_cleanup(struct hfi_ctx *ctx);
void hfi_cq_config(struct hfi_ctx *ctx, u16 cq_idx,
		   struct hfi_auth_tuple *auth_table, bool user_priv);
void hfi_cq_config_tuples(struct hfi_ctx *ctx, u16 cq_idx,
			  struct hfi_auth_tuple *auth_table);
void hfi_cq_disable(struct hfi_devdata *dd, u16 cq_idx);
void hfi_pcb_write(struct hfi_ctx *ctx, u16 ptl_pid);
void hfi_pcb_reset(struct hfi_devdata *dd, u16 ptl_pid);
void hfi_eq_cache_invalidate(struct hfi_devdata *dd, u16 ptl_pid);
void hfi_tpid_enable(struct hfi_devdata *dd, u8 idx, u16 base, u32 ptl_uid);
void hfi_tpid_disable(struct hfi_devdata *dd, u8 idx);
int hfi_iommu_set_pasid(struct hfi_ctx *ctx);
int hfi_iommu_clear_pasid(struct hfi_ctx *ctx);

/* OPA core functions */
int hfi_cq_assign(struct hfi_ctx *ctx, struct hfi_auth_tuple *auth_table, u16 *cq_idx);
int hfi_cq_update(struct hfi_ctx *ctx, u16 cq_idx, struct hfi_auth_tuple *auth_table);
int hfi_cq_release(struct hfi_ctx *ctx, u16 cq_idx);
int hfi_cq_map(struct hfi_ctx *ctx, u16 cq_idx,
	       struct hfi_cq *tx, struct hfi_cq *rx);
void hfi_cq_unmap(struct hfi_cq *tx, struct hfi_cq *rx);
int hfi_dlid_assign(struct hfi_ctx *ctx,
		    struct hfi_dlid_assign_args *dlid_assign);
int hfi_dlid_release(struct hfi_ctx *ctx, u32 dlid_base, u32 count);
int hfi_cteq_assign(struct hfi_ctx *ctx, struct opa_ev_assign *ev_assign);
int hfi_cteq_release(struct hfi_ctx *ctx, u16 eq_mode, u16 eq_idx,
		     u64 user_data);
int hfi_cteq_wait_single(struct hfi_ctx *ctx, u16 eq_mode, u16 ev_idx,
			 long timeout);
int hfi_ctxt_attach(struct hfi_ctx *ctx, struct opa_ctx_assign *ctx_assign);
void hfi_ctxt_cleanup(struct hfi_ctx *ctx);
int hfi_ctxt_set_allowed_uids(struct hfi_ctx *ctx, u32 *auth_uid,
			      u8 num_uids);
int hfi_ctxt_reserve(struct hfi_ctx *ctx, u16 *base, u16 count, u16 align,
		     u16 flags);
int hfi_get_hw_limits(struct hfi_ctx *ctx, struct hfi_hw_limit *hwl);
void hfi_ctxt_unreserve(struct hfi_ctx *ctx);
int hfi_ctxt_hw_addr(struct hfi_ctx *ctx, int token, u16 ctxt, void **addr,
		     ssize_t *len);
void hfi_ctxt_set_bypass(struct hfi_ctx *ctx);
int hfi_e2e_ctrl(struct hfi_ctx *ctx, struct opa_e2e_ctrl *e2e_ctrl);
void hfi_e2e_destroy(struct hfi_devdata *dd);
void hfi_rsm_clear_rule(struct hfi_devdata *dd, u8 rule_idx);
int hfi_rsm_set_rule(struct hfi_devdata *dd, struct hfi_rsm_rule *rule,
		     struct hfi_ctx *rx_ctx[], u16 num_contexts);

static inline struct hfi_pportdata *to_hfi_ppd(struct hfi_devdata *dd,
							u8 port)
{
	u8 pidx = port - 1; /* IB number port from 1, hdw from 0 */

	if (pidx >= dd->num_pports) {
		WARN_ON(pidx >= dd->num_pports);
		return NULL;
	}
	return &(dd->pport[pidx]);
}

static inline int ppd_to_pnum(struct hfi_pportdata *ppd)
{
	u8 pnum = ppd->pnum;
	struct hfi_devdata *dd = ppd->dd;

	if (unlikely(pnum == 0 || pnum > dd->num_pports)) {
		WARN(1, "Invalid port number");
		return -1;
	}

	return (int)pnum;
}

static inline void hfi_set_bits(u64 *target, u64 val, u64 mask, u8 shift)
{
	/* clear */
	*target &=  ~(mask << shift);
	/* set */
	*target |= (val & mask) << shift;
}

static inline u8 hfi_parity(u16 val)
{
	int i, m = 0;

	/* m is 1 for odd parity */
	for (i = 0; i < sizeof(val) * 8; i++, val >>= 1)
		m ^= (val & 0x1);

	return m;
}

int hfi_get_sma(struct hfi_devdata *dd, u16 attr_id, struct opa_smp *smp,
		u32 am, u8 *data, u8 port, u32 *resp_len, u8 *sma_status);
int hfi_set_sma(struct hfi_devdata *dd, u16 attr_id, struct opa_smp *smp,
		u32 am, u8 *data, u8 port, u32 *resp_len, u8 *sma_status);

void hfi_set_link_down_reason(struct hfi_pportdata *ppd, u8 lcl_reason,
			  u8 neigh_reason, u8 rem_reason);
void hfi_apply_link_downgrade_policy(struct hfi_pportdata *ppd,
						int refresh_widths);
int hfi_set_link_state(struct hfi_pportdata *ppd, u32 state);
void hfi_ack_interrupt(struct hfi_msix_entry *me);
u8 hfi_porttype(struct hfi_pportdata *ppd);
int hfi_get_ib_cfg(struct hfi_pportdata *ppd, int which);
int hfi_set_ib_cfg(struct hfi_pportdata *ppd, int which, u32 val, void *data);
int hfi_set_mtu(struct hfi_pportdata *ppd);
u16 hfi_port_ltp_to_cap(u16 port_ltp);
u16 hfi_port_cap_to_lcb(struct hfi_devdata *dd, u16 crc_mask);
u16 hfi_cap_to_port_ltp(u16 cap);
void hfi_set_crc_mode(struct hfi_pportdata *ppd, u16 crc_lcb_mode);
bool hfi_is_portals_req_sl(struct hfi_pportdata *ppd, u8 sl);
void hfi_get_buffer_control(struct hfi_pportdata *ppd,
			    struct buffer_control *bc, u16 *overall_limit);
int hfi_set_buffer_control(struct hfi_pportdata *ppd,
			   struct buffer_control *new_bc);
void hfi_cfg_out_pkey_check(struct hfi_pportdata *ppd, u8 enable);
void hfi_cfg_in_pkey_check(struct hfi_pportdata *ppd, u8 enable);
void hfi_set_up_vl15(struct hfi_pportdata *ppd, u8 vau, u16 vl15buf);
void hfi_assign_remote_cm_au_table(struct hfi_pportdata *ppd, u8 vcu);
int neigh_is_hfi(struct hfi_pportdata *ppd);
void hfi_add_full_mgmt_pkey(struct hfi_pportdata *ppd);
const char *hfi_class_name(void);
int hfi_set_lid(struct hfi_pportdata *ppd, u32 lid, u8 lmc);

/*
 * dev_err can be used (only!) to print early errors before devdata is
 * allocated, or when dd->pcidev may not be valid, and at the tail end of
 * cleanup when devdata may have been freed, etc.
 * Otherwise these macros below are the preferred ones.
 */
#define dd_dev_err(dd, fmt, ...) \
	dev_err(&(dd)->pcidev->dev, "%d: " fmt, \
		(dd)->unit, ##__VA_ARGS__)

#define dd_dev_info(dd, fmt, ...) \
	dev_info(&(dd)->pcidev->dev, "%d: " fmt, \
		 (dd)->unit, ##__VA_ARGS__)

#define dd_dev_dbg(dd, fmt, ...) \
	dev_dbg(&(dd)->pcidev->dev, "%d: " fmt, \
		 (dd)->unit, ##__VA_ARGS__)

#define dd_dev_warn(dd, fmt, ...) \
	dev_warn(&(dd)->pcidev->dev, "%d: " fmt, \
		 (dd)->unit, ##__VA_ARGS__)

#define ppd_dev_err(ppd, fmt, ...) \
	dev_err(&(ppd)->dd->pcidev->dev, "%d/%d: " fmt,	\
		(ppd)->dd->unit, ppd->pnum, ##__VA_ARGS__)

#define ppd_dev_info(ppd, fmt, ...) \
	dev_info(&(ppd)->dd->pcidev->dev, "%d/%d: " fmt,	\
		(ppd)->dd->unit, ppd->pnum, ##__VA_ARGS__)

#define ppd_dev_dbg(ppd, fmt, ...) \
	dev_dbg(&(ppd)->dd->pcidev->dev, "%d/%d: " fmt,	\
		(ppd)->dd->unit, ppd->pnum, ##__VA_ARGS__)

#define ppd_dev_warn(ppd, fmt, ...) \
	dev_warn(&(ppd)->dd->pcidev->dev, "%d/%d: " fmt,	\
		(ppd)->dd->unit, ppd->pnum, ##__VA_ARGS__)

#ifndef BIT_ULL
#define BIT_ULL(nr)             (1ULL << (nr))
#endif

/* printk wrappers (pr_warn, etc) can also be used for general debugging. */
#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

/* FXRTODO: Hacks for early bring up. Delete before upstreaming */
extern bool quick_linkup;
extern bool force_loopback;
extern bool no_mnh;
extern bool no_pe_fw;
extern bool opafm_disable;

static inline u64 read_csr(const struct hfi_devdata *dd, u32 offset)
{
	u64 val;

	BUG_ON(dd->kregbase == NULL);

	/*
	 * FXRTODO: Do not touch MNH CSRs if MNH is not available
	 * for early bring up
	 */
	if (no_mnh && offset >= FXR_MNH_MISC_CSRS &&
		offset < FXR_MNH_S1_SNDN_CSRS) {
		dd_dev_err(dd, "%s MNH offset 0x%x\n", __func__, offset);
		BUG_ON(1);
		return 0;
	}

	val = readq(dd->kregbase + offset);
	return le64_to_cpu(val);
}

static inline void write_csr(const struct hfi_devdata *dd, u32 offset, u64 value)
{
	BUG_ON(dd->kregbase == NULL);
	/*
	 * FXRTODO: Do not touch MNH CSRs if MNH is not available
	 * for early bring up
	 */
	if (no_mnh && offset >= FXR_MNH_MISC_CSRS &&
		offset < FXR_MNH_S1_SNDN_CSRS) {
		dd_dev_err(dd, "%s MNH offset 0x%x\n", __func__, offset);
		BUG_ON(1);
		return;
	}
	writeq(cpu_to_le64(value), dd->kregbase + offset);
}
#endif
