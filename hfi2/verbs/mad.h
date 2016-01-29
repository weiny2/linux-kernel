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
 * Intel(R) OPA Gen2 IB Driver
 */
#ifndef _OPA_MAD_H
#define _OPA_MAD_H

#include <rdma/opa_smi.h>
#include <rdma/ib_mad.h>
#include <rdma/ib_pma.h>
#include <rdma/opa_port_info.h>
#include <rdma/opa_core_ib.h>

#define MKEY_SHIFT		6

#define IB_SMP_UNSUP_VERSION    cpu_to_be16(0x0004)
#define IB_SMP_UNSUP_METHOD     cpu_to_be16(0x0008)
#define IB_SMP_UNSUP_METH_ATTR  cpu_to_be16(0x000C)
#define IB_SMP_INVALID_FIELD    cpu_to_be16(0x001C)

struct opa_pma_mad {
	struct ib_mad_hdr mad_hdr;
	u8 data[2024];
} __packed;

#define VL_MASK_ALL		0x000080ff
#define OPA_EI_STATUS_SMASK	0x80

struct opa_class_port_info {
	u8 base_version;
	u8 class_version;
	__be16 cap_mask;
	__be32 cap_mask2_resp_time;

	u8 redirect_gid[16];
	__be32 redirect_tc_fl;
	__be32 redirect_lid;
	__be32 redirect_sl_qp;
	__be32 redirect_qkey;

	u8 trap_gid[16];
	__be32 trap_tc_fl;
	__be32 trap_lid;
	__be32 trap_hl_qp;
	__be32 trap_qkey;

	__be16 trap_pkey;
	__be16 redirect_pkey;

	u8 trap_sl_rsvd;
	u8 reserved[3];
} __packed;

struct opa_port_status_req {
	__u8 port_num;
	__u8 reserved[3];
	__be32 vl_select_mask;
};

struct opa_port_status_rsp {
	__u8 port_num;
	__u8 reserved[3];
	__be32  vl_select_mask;

	/* Data counters */
	__be64 port_xmit_data;
	__be64 port_rcv_data;
	__be64 port_xmit_pkts;
	__be64 port_rcv_pkts;
	__be64 port_multicast_xmit_pkts;
	__be64 port_multicast_rcv_pkts;
	__be64 port_xmit_wait;
	__be64 sw_port_congestion;
	__be64 port_rcv_fecn;
	__be64 port_rcv_becn;
	__be64 port_xmit_time_cong;
	__be64 port_xmit_wasted_bw;
	__be64 port_xmit_wait_data;
	__be64 port_rcv_bubble;
	__be64 port_mark_fecn;
	/* Error counters */
	__be64 port_rcv_constraint_errors;
	__be64 port_rcv_switch_relay_errors;
	__be64 port_xmit_discards;
	__be64 port_xmit_constraint_errors;
	__be64 port_rcv_remote_physical_errors;
	__be64 local_link_integrity_errors;
	__be64 port_rcv_errors;
	__be64 excessive_buffer_overruns;
	__be64 fm_config_errors;
	__be32 link_error_recovery;
	__be32 link_downed;
	u8 uncorrectable_errors;

	u8 link_quality_indicator; /* 5res, 3bit */
	u8 res2[6];
	struct _vls_pctrs {
		/* per-VL Data counters */
		__be64 port_vl_xmit_data;
		__be64 port_vl_rcv_data;
		__be64 port_vl_xmit_pkts;
		__be64 port_vl_rcv_pkts;
		__be64 port_vl_xmit_wait;
		__be64 sw_port_vl_congestion;
		__be64 port_vl_rcv_fecn;
		__be64 port_vl_rcv_becn;
		__be64 port_xmit_time_cong;
		__be64 port_vl_xmit_wasted_bw;
		__be64 port_vl_xmit_wait_data;
		__be64 port_vl_rcv_bubble;
		__be64 port_vl_mark_fecn;
		__be64 port_vl_xmit_discards;
	/*
	 * FXRTODO: add STL2 specific fields after confirming with
	 * hardware team. STL2 specific fields are not present in the mad
	 * structre in the latest STL2 spec
	 */
	} vls[0]; /* real array size defined by # bits set in vl_select_mask */
};

struct opa_clear_port_status {
	__be64 port_select_mask[4];
	__be32 counter_select_mask;
};

struct opa_port_data_counters_msg {
	__be64 port_select_mask[4];
	__be32 vl_select_mask;
	__be32 resolution;

	/* Response fields follow */
	struct _port_dctrs {
		u8 port_number;
		u8 reserved2[3];
		__be32 link_quality_indicator; /* 29res, 3bit */

		/* Data counters */
		__be64 port_xmit_data;
		__be64 port_rcv_data;
		__be64 port_xmit_pkts;
		__be64 port_rcv_pkts;
		__be64 port_multicast_xmit_pkts;
		__be64 port_multicast_rcv_pkts;
		__be64 port_xmit_wait;
		__be64 sw_port_congestion;
		__be64 port_rcv_fecn;
		__be64 port_rcv_becn;
		__be64 port_xmit_time_cong;
		__be64 port_xmit_wasted_bw;
		__be64 port_xmit_wait_data;
		__be64 port_rcv_bubble;
		__be64 port_mark_fecn;

		__be64 port_error_counter_summary;
		/* Sum of error counts/port */

		struct _vls_dctrs {
			/* per-VL Data counters */
			__be64 port_vl_xmit_data;
			__be64 port_vl_rcv_data;
			__be64 port_vl_xmit_pkts;
			__be64 port_vl_rcv_pkts;
			__be64 port_vl_xmit_wait;
			__be64 sw_port_vl_congestion;
			__be64 port_vl_rcv_fecn;
			__be64 port_vl_rcv_becn;
			__be64 port_xmit_time_cong;
			__be64 port_vl_xmit_wasted_bw;
			__be64 port_vl_xmit_wait_data;
			__be64 port_vl_rcv_bubble;
			__be64 port_vl_mark_fecn;
		} vls[0];
		/* array size defined by #bits set in vl_select_mask*/
	} port[1]; /* array size defined by  #ports in attribute modifier */
};

struct opa_port_error_counters64_msg {
	/*
	 * Request contains first two fields, response contains the
	 * whole magilla
	 */
	__be64 port_select_mask[4];
	__be32 vl_select_mask;

	/* Response-only fields follow */
	__be32 reserved1;
	struct _port_ectrs {
		u8 port_number;
		u8 reserved2[7];
		__be64 port_rcv_constraint_errors;
		__be64 port_rcv_switch_relay_errors;
		__be64 port_xmit_discards;
		__be64 port_xmit_constraint_errors;
		__be64 port_rcv_remote_physical_errors;
		__be64 local_link_integrity_errors;
		__be64 port_rcv_errors;
		__be64 excessive_buffer_overruns;
		__be64 fm_config_errors;
		__be32 link_error_recovery;
		__be32 link_downed;
		u8 uncorrectable_errors;
		u8 reserved3[7];
		struct _vls_ectrs {
			__be64 port_vl_xmit_discards;
		} vls[0];
		/* array size defined by #bits set in vl_select_mask */
	} port[1]; /* array size defined by #ports in attribute modifier */
};

struct opa_port_error_info_msg {
	__be64 port_select_mask[4];
	__be32 error_info_select_mask;
	__be32 reserved1;
	struct _port_ei {
		u8 port_number;
		u8 reserved2[7];

		/* PortRcvErrorInfo */
		struct {
			u8 status_and_code;
			union {
				u8 raw[17];
				struct {
					/* EI1to12 format */
					u8 packet_flit1[8];
					u8 packet_flit2[8];
					u8 remaining_flit_bits12;
				} ei1to12;
				struct {
					u8 packet_bytes[8];
					u8 remaining_flit_bits;
				} ei13;
			} ei;
			u8 reserved3[6];
		} __packed port_rcv_ei;

		/* ExcessiveBufferOverrunInfo */
		struct {
			u8 status_and_sc;
			u8 reserved4[7];
		} __packed excessive_buffer_overrun_ei;

		/* PortXmitConstraintErrorInfo */
		struct {
			u8 status;
			u8 reserved5;
			__be16 pkey;
			__be32 slid;
		} __packed port_xmit_constraint_ei;

		/* PortRcvConstraintErrorInfo */
		struct {
			u8 status;
			u8 reserved6;
			__be16 pkey;
			__be32 slid;
		} __packed port_rcv_constraint_ei;

		/* PortRcvSwitchRelayErrorInfo */
		struct {
			u8 status_and_code;
			u8 reserved7[3];
			__u32 error_info;
		} __packed port_rcv_switch_relay_ei;

		/* UncorrectableErrorInfo */
		struct {
			u8 status_and_code;
			u8 reserved8;
		} __packed uncorrectable_ei;

		/* FMConfigErrorInfo */
		struct {
			u8 status_and_code;
			u8 error_info;
		} __packed fm_config_ei;
		__u32 reserved9;
	} port[1]; /* actual array size defined by #ports in attr modifier */
};

/* opa_port_error_info_msg error_info_select_mask bit definitions */
enum error_info_selects {
	ES_PORT_RCV_ERROR_INFO			= (1 << 31),
	ES_EXCESSIVE_BUFFER_OVERRUN_INFO	= (1 << 30),
	ES_PORT_XMIT_CONSTRAINT_ERROR_INFO	= (1 << 29),
	ES_PORT_RCV_CONSTRAINT_ERROR_INFO	= (1 << 28),
	ES_PORT_RCV_SWITCH_RELAY_ERROR_INFO	= (1 << 27),
	ES_UNCORRECTABLE_ERROR_INFO		= (1 << 26),
	ES_FM_CONFIG_ERROR_INFO			= (1 << 25)
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 3, 0)
struct ib_node_info {
	u8 base_version;
	u8 class_version;
	u8 node_type;
	u8 num_ports;
	__be64 sys_guid;
	__be64 node_guid;
	__be64 port_guid;
	__be16 partition_cap;
	__be16 device_id;
	__be32 revision;
	u8 local_port_num;
	u8 vendor_id[3];
} __packed;

struct ib_mad_notice_attr {
	u8 generic_type;
	u8 prod_type_msb;
	__be16 prod_type_lsb;
	__be16 trap_num;
	__be16 issuer_lid;
	__be16 toggle_count;

	union {
		struct {
			u8	details[54];
		} raw_data;

		struct {
			__be16	reserved;
			__be16	lid;		/* where violation happened */
			u8	port_num;	/* where violation happened */
		} __packed ntc_129_131;

		struct {
			__be16	reserved;
			__be16	lid;		/* LID where change occurred */
			u8	reserved2;
			u8	local_changes;	/* low bit - local changes */
			__be32	new_cap_mask;	/* new capability mask */
			u8	reserved3;
			u8	change_flags;	/* low 3 bits only */
		} __packed ntc_144;

		struct {
			__be16	reserved;
			__be16	lid;		/* lid where sys guid changed */
			__be16	reserved2;
			__be64	new_sys_guid;
		} __packed ntc_145;

		struct {
			__be16	reserved;
			__be16	lid;
			__be16	dr_slid;
			u8	method;
			u8	reserved2;
			__be16	attr_id;
			__be32	attr_mod;
			__be64	mkey;
			u8	reserved3;
			u8	dr_trunc_hop;
			u8	dr_rtn_path[30];
		} __packed ntc_256;

		struct {
			__be16		reserved;
			__be16		lid1;
			__be16		lid2;
			__be32		key;
			__be32		sl_qp1;	/* SL: high 4 bits */
			__be32		qp2;	/* high 8 bits reserved */
			union ib_gid	gid1;
			union ib_gid	gid2;
		} __packed ntc_257_258;

	} details;
};
#endif

struct opa_aggregate {
	__be16 attr_id;
	__be16 err_reqlength;	/* 1 bit, 8 res, 7 bit */
	__be32 attr_mod;
	u8 data[0];
};

/*
 * Generic trap/notice types
 */
#define IB_NOTICE_TYPE_FATAL	0x80
#define IB_NOTICE_TYPE_URGENT	0x81
#define IB_NOTICE_TYPE_SECURITY	0x82
#define IB_NOTICE_TYPE_SM	0x83
#define IB_NOTICE_TYPE_INFO	0x84

/*
 * Generic trap/notice producers
 */
#define IB_NOTICE_PROD_CA		cpu_to_be16(1)
#define IB_NOTICE_PROD_SWITCH		cpu_to_be16(2)
#define IB_NOTICE_PROD_ROUTER		cpu_to_be16(3)
#define IB_NOTICE_PROD_CLASS_MGR	cpu_to_be16(4)

/*
 * Generic trap/notice numbers
 */
#define IB_NOTICE_TRAP_LLI_THRESH	cpu_to_be16(129)
#define IB_NOTICE_TRAP_EBO_THRESH	cpu_to_be16(130)
#define IB_NOTICE_TRAP_FLOW_UPDATE	cpu_to_be16(131)
#define IB_NOTICE_TRAP_CAP_MASK_CHG	cpu_to_be16(144)
#define IB_NOTICE_TRAP_SYS_GUID_CHG	cpu_to_be16(145)
#define IB_NOTICE_TRAP_BAD_MKEY		cpu_to_be16(256)
#define IB_NOTICE_TRAP_BAD_PKEY		cpu_to_be16(257)
#define IB_NOTICE_TRAP_BAD_QKEY		cpu_to_be16(258)

/*
 * Generic trap/notice M_Key violation flags in dr_trunc_hop (trap 256).
 */
#define IB_NOTICE_TRAP_DR_NOTICE	0x80
#define IB_NOTICE_TRAP_DR_TRUNC		0x40

#define OPA_CONG_LOG_ELEMS	96
#define PHYSPORTSTATE_SHIFT	4
#define NNORMAL_SHIFT		4
#define SM_CONFIG_SHIFT		5
#define HFI_PKEY_BLOCKS_AVAIL	(HFI_MAX_PKEYS / OPA_PARTITION_TABLE_BLK_SIZE)

/* the reset value from the FM is supposed to be 0xffff, handle both */
#define HFI_LINK_WIDTH_RESET_OLD 0x0fff
#define HFI_LINK_WIDTH_RESET 0xffff

#define HFI_IB_CC_TABLE_CAP_DEFAULT	31
#define HFI_IB_CCT_ENTRIES		64
#define HFI_CC_TABLE_SHADOW_MAX \
	(HFI_IB_CC_TABLE_CAP_DEFAULT * HFI_IB_CCT_ENTRIES)

/* port control flags */
#define HFI_IB_CC_CCS_PC_SL_BASED 0x01

struct opa_congestion_info_attr {
	__be16 congestion_info;
	u8 control_table_cap;	/* Multiple of 64 entry unit CCTs */
	u8 congestion_log_length;
} __packed;

struct opa_congestion_setting_entry {
	u8 ccti_increase;
	u8 reserved;
	__be16 ccti_timer;
	u8 trigger_threshold;
	u8 ccti_min; /* min CCTI for cc table */
} __packed;

struct opa_congestion_setting_entry_shadow {
	u8 ccti_increase;
	u8 reserved;
	u16 ccti_timer;
	u8 trigger_threshold;
	u8 ccti_min; /* min CCTI for cc table */
} __packed;

struct ib_cc_table_entry_shadow {
	u16 entry; /* shift:2, multiplier:14 */
};

struct ib_cc_table_entry {
	__be16 entry; /* shift:2, multiplier:14 */
};

struct ib_cc_table_attr {
	__be16 ccti_limit; /* max CCTI for cc table */
	struct ib_cc_table_entry ccti_entries[HFI_IB_CCT_ENTRIES];
} __packed;

struct cc_table_shadow {
	u16 ccti_limit; /* max CCTI for cc table */
	struct ib_cc_table_entry_shadow entries[HFI_CC_TABLE_SHADOW_MAX];
} __packed;

struct opa_congestion_setting_attr {
	__be32 control_map;
	__be16 port_control;
	struct opa_congestion_setting_entry entries[OPA_MAX_SLS];
} __packed;

struct opa_congestion_setting_attr_shadow {
	u32 control_map;
	u16 port_control;
	struct opa_congestion_setting_entry_shadow entries[OPA_MAX_SLS];
} __packed;

/*
 * struct cc_state combines the (active) per-port congestion control
 * table, and the (active) per-SL congestion settings. cc_state data
 * may need to be read in code paths that we want to be fast, so it
 * is an RCU protected structure.
 */
struct cc_state {
	struct rcu_head rcu;
	struct cc_table_shadow cct;
	struct opa_congestion_setting_attr_shadow cong_setting;
};
#endif				/* _OPA_MAD_H */
