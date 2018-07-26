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
#include <rdma/ib_addr.h>
#include <rdma/ib_mad.h>
#include <rdma/ib_pma.h>
#include <rdma/opa_port_info.h>

#define OPA_DEFAULT_SM_TRAP_QP		0x0
#define OPA_DEFAULT_SA_QP		0x1

#define OPA_NUM_PKEY_BLOCKS_PER_SMP	(OPA_SMP_DR_DATA_SIZE \
			/ (OPA_PKEY_TABLE_BLK_COUNT * sizeof(u16)))

#define OPA_LIM_MGMT_PKEY		0x7FFF
#define OPA_FULL_MGMT_PKEY		0xFFFF
#define OPA_LIM_MGMT_PKEY_IDX		1
#define OPA_PKEY_TABLE_BLK_COUNT	OPA_PARTITION_TABLE_BLK_SIZE

/* OPA SMA aggregate */
#define OPA_MAX_AGGR_ATTR		117
#define OPA_MIN_AGGR_ATTR		1
#define OPA_AGGR_REQ_LEN_SHIFT		0
#define OPA_AGGR_REQ_LEN_MASK		0x7f
#define OPA_AGGR_REQ_LEN(req)		(((req) >> OPA_AGGR_REQ_LEN_SHIFT) & \
					OPA_AGGR_REQ_LEN_MASK)
#define OPA_AGGR_REQ_BYTES_PER_UNIT	8
#define OPA_AGGR_ERROR			cpu_to_be16(0x8000)

/* OPA SMA attribute IDs */
#define OPA_ATTRIB_ID_CONGESTION_INFO		cpu_to_be16(0x008b)
#define OPA_ATTRIB_ID_HFI_CONGESTION_LOG	cpu_to_be16(0x008f)
#define OPA_ATTRIB_ID_HFI_CONGESTION_SETTING	cpu_to_be16(0x0090)
#define OPA_ATTRIB_ID_CONGESTION_CONTROL_TABLE	cpu_to_be16(0x0091)

/* OPA PMA attribute IDs */
#define OPA_PM_ATTRIB_ID_PORT_STATUS		cpu_to_be16(0x0040)
#define OPA_PM_ATTRIB_ID_CLEAR_PORT_STATUS	cpu_to_be16(0x0041)
#define OPA_PM_ATTRIB_ID_DATA_PORT_COUNTERS	cpu_to_be16(0x0042)
#define OPA_PM_ATTRIB_ID_ERROR_PORT_COUNTERS	cpu_to_be16(0x0043)
#define OPA_PM_ATTRIB_ID_ERROR_INFO		cpu_to_be16(0x0044)
#define OPA_PM_ATTRIB_ID_FREQUENT_COUNTERS	cpu_to_be16(0x0045)
#define OPA_PM_ATTRIB_ID_INFREQUENT_COUNTERS	cpu_to_be16(0x0046)
#define OPA_PM_ATTRIB_ID_ALL_COUNTERS		cpu_to_be16(0x0047)

/* OPA TimeSync attribute ID */
#define OPA_ATTRIB_TS_E2E_PROPAGATION_DELAY	cpu_to_be16(0x009E)

#define OPA_PM_STATUS_REQUEST_TOO_LARGE		cpu_to_be16(0x100)
/* attribute modifier macros */
#define OPA_AM_NPORT_SHIFT	24
#define OPA_AM_NPORT_MASK	0xff
#define OPA_AM_NPORT_SMASK	(OPA_AM_NPORT_MASK << OPA_AM_NPORT_SHIFT)
#define OPA_AM_NPORT(am)	(((am) >> OPA_AM_NPORT_SHIFT) & \
					OPA_AM_NPORT_MASK)

#define OPA_AM_NBLK_SHIFT	24
#define OPA_AM_NBLK_MASK	0xff
#define OPA_AM_NBLK_SMASK	(OPA_AM_NBLK_MASK << OPA_AM_NBLK_SHIFT)
#define OPA_AM_NBLK(am)		(((am) >> OPA_AM_NBLK_SHIFT) & \
					OPA_AM_NBLK_MASK)

#define OPA_AM_START_BLK_SHIFT	0
#define OPA_AM_START_BLK_MASK	0x7ff
#define OPA_AM_START_BLK_SMASK	(OPA_AM_START_BLK_MASK << \
					OPA_AM_START_BLK_SHIFT)
#define OPA_AM_START_BLK(am)	(((am) >> OPA_AM_START_BLK_SHIFT) & \
					OPA_AM_START_BLK_MASK)

#define OPA_AM_START_SM_CFG_SHIFT	9
#define OPA_AM_START_SM_CFG_MASK	0x1
#define OPA_AM_START_SM_CFG_SMASK	(OPA_AM_START_SM_CFG_MASK << \
						OPA_AM_START_SM_CFG_SHIFT)
#define OPA_AM_START_SM_CFG(am)		(((am) >> OPA_AM_START_SM_CFG_SHIFT) \
						& OPA_AM_START_SM_CFG_MASK)

#define OPA_AM_ASYNC_SHIFT	12
#define OPA_AM_ASYNC_MASK	0x1
#define OPA_AM_ASYNC_SMASK	(OPA_AM_ASYNC_MASK << OPA_AM_ASYNC_SHIFT)
#define OPA_AM_ASYNC(am)	(((am) >> OPA_AM_ASYNC_SHIFT) & \
					OPA_AM_ASYNC_MASK)

#define OPA_BW_SECTION_SHIFT	16
#define OPA_BW_SECTION_MASK	0xff
#define OPA_BW_SECTION_SMASK	(OPA_BW_SECTION_MASK << OPA_BW_SECTION_SHIFT)
#define OPA_BW_SECTION(am)	(((am) >> OPA_BW_SECTION_SHIFT) & \
					OPA_BW_SECTION_MASK)
#define OPA_MTU_0     0
#define OPA_INVALID_LID_MASK	0xFFFF0000
#define IS_VALID_LID_SIZE(lid) (!((lid) & (OPA_INVALID_LID_MASK)))
#define INVALID_MTU	0xffff
#define INVALID_MTU_ENC	0xff

#define OPA_AM_NATTR_SHIFT	0
#define OPA_AM_NATTR_MASK	0xff
#define OPA_AM_NATTR(am)	(((am) >> OPA_AM_NATTR_SHIFT) & \
					OPA_AM_NATTR_MASK)

#define OPA_AM_CI_ADDR_SHIFT	19
#define OPA_AM_CI_ADDR_MASK	0xfff
#define OPA_AM_CI_ADDR_SMASK	(OPA_AM_CI_ADDR_MASK << OPA_CI_ADDR_SHIFT)
#define OPA_AM_CI_ADDR(am)	(((am) >> OPA_AM_CI_ADDR_SHIFT) & \
					OPA_AM_CI_ADDR_MASK)

#define OPA_AM_CI_LEN_SHIFT	13
#define OPA_AM_CI_LEN_MASK	0x3f
#define OPA_AM_CI_LEN_SMASK	(OPA_AM_CI_LEN_MASK << OPA_CI_LEN_SHIFT)
#define OPA_AM_CI_LEN(am)	(((am) >> OPA_AM_CI_LEN_SHIFT) & \
					OPA_AM_CI_LEN_MASK)
#define MKEY_SHIFT		6

#define CI_PAGE_SIZE	128
#define CI_PAGE_MASK	~(CI_PAGE_SIZE - 1)
#define CI_PAGE_NUM(a) ((a) & CI_PAGE_MASK)

/* FXRTODO:  Rightful place for these are in ./include/rdma/opa_smi.h */
#define OPA_ATTRIB_ID_BW_ARBITRATION		cpu_to_be16(0x0092)
#define OPA_ATTRIB_ID_SL_PAIRS			cpu_to_be16(0x0097)

#define OPA_BWARB_GROUP				0
#define OPA_BWARB_PREEMPT_MATRIX		1

#define IB_SMP_UNSUP_VERSION    cpu_to_be16(0x0004)
#define IB_SMP_UNSUP_METHOD     cpu_to_be16(0x0008)
#define IB_SMP_UNSUP_METH_ATTR  cpu_to_be16(0x000C)
#define IB_SMP_INVALID_FIELD    cpu_to_be16(0x001C)

#define OPA_MAX_BW_GROUP		32
#define OPA_NUM_BW_GROUP_SUPPORTED	8

struct opa_bw_element {
	u8 priority;
	u8 bw_percentage;
	u8 reserved[2];
	__be32 vl_mask;
} __packed;

union opa_bw_arb_table {
	struct opa_bw_element bw_group[OPA_MAX_BW_GROUP];
	__be32 matrix[OPA_MAX_BW_GROUP];
} __packed;

struct opa_bw_arb {
	__be64 port_sel_mask[4];
	union opa_bw_arb_table arb_block[0];
} __packed;

enum opa_vl_prioriy {
	OPA_VL_PRIORITY_LOW,
	OPA_VL_PRIORITY_MEDIUM,
	OPA_VL_PRIORITY_HIGH
};

struct opa_pma_mad {
	struct ib_mad_hdr mad_hdr;
	u8 data[2024];
} __packed;

#define VL_MASK_ALL		0x000081ff
#define OPA_EI_STATUS_SMASK	0x80
#define OPA_EI_CODE_SMASK	0x0f

struct opa_ts_e2e_mad {
	u8 bits;
	u8 reserved1;
	__be16 accuracy;
	__be16 jitter;
	__be16 clock_delay;
	__be64 clock_offset;
	__be16 periodicity;
	u16 reserved2;
	u32 reserved3;
};

#define OPA_E2E_USE_OFFSET_MASK         (0x00000001 << 7)
#define OPA_E2E_CLOCK_ID_MASK           (0x00000003 << 5)

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
	} vls[0]; /* real array size defined by # bits set in vl_select_mask */
};

struct opa_clear_port_status {
	__be64 port_select_mask[4];
	__be32 counter_select_mask;
};

#define MSK_LLI 0x000000f0
#define MSK_LLI_SFT 4
#define MSK_LER 0x0000000f
#define MSK_LER_SFT 0
#define ADD_LLI 8
#define ADD_LER 2

enum {
	C_VL_0 = 0,
	C_VL_1,
	C_VL_2,
	C_VL_3,
	C_VL_4,
	C_VL_5,
	C_VL_6,
	C_VL_7,
	C_VL_8,
	C_VL_15,
	C_VL_COUNT
};

static inline int vl_from_idx(int idx)
{
	return (idx == C_VL_15 ? 15 : idx);
}

static inline int idx_from_vl(int vl)
{
	return (vl == 15 ? C_VL_15 : vl);
}

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

struct _port_ectrs_cpu {
		u8 port_number;
		u8 reserved2[7];
		u64 port_rcv_constraint_errors;
		u64 port_rcv_switch_relay_errors;
		u64 port_xmit_discards;
		u64 port_xmit_constraint_errors;
		u64 port_rcv_remote_physical_errors;
		u64 local_link_integrity_errors;
		u64 port_rcv_errors;
		u64 excessive_buffer_overruns;
		u64 fm_config_errors;
		u32 link_error_recovery;
		u32 link_downed;
		u8 uncorrectable_errors;
		u8 reserved3[7];
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

/* Attributes Structs Introduced in Gen2 */
struct opa_frequent_counters_rsp {
	__be64 port_select_mask[4];
	__be32 vl_select_mask;
	__be32 resolution;
	__be64 node_clocks;

	struct _port_fcntrs {
		u8 link_quality_indicator;
		u8 reserved[7];
		__be64 port_xmit_data;
		__be64 port_rcv_data;
		__be64 port_xmit_pkts;
		__be64 port_rcv_pkts;
		__be64 port_multicast_xmit_pkts;
		__be64 port_multicast_rcv_pkts;
		__be64 port_xmit_reliable_ltps;
		__be64 port_rcv_reliable_ltps;
		__be64 port_xmit_wait;
		__be64 port_xmit_time_cong;
		__be64 port_xmit_wasted_bw;
		__be64 port_xmit_wait_data;
		__be64 port_rcv_bubble;
		__be64 infrequent_counter_summary;
		struct _vl_fcntrs {
			__be64 port_vl_xmit_data;
			__be64 port_vl_rcv_data;
			__be64 port_vl_xmit_pkts;
			__be64 port_vl_rcv_pkts;
			__be64 port_vl_xmit_wait;
			__be64 port_vl_xmit_time_cong;
			__be64 port_vl_xmit_wasted_bw;
			__be64 port_vl_xmit_wait_data;
			__be64 port_vl_rcv_bubble;
		} vls[0];

	} port[1];
};

struct opa_frequent_counters_req {
	__be64 port_select_mask[4];
	__be32 vl_select_mask;
	__be32 resolution;
};

struct opa_infrequent_counters_rsp {
	__be64 port_select_mask[4];
	__be32 vl_select_mask;
	__be32 reserved;
	struct _port_ifcntrs {
		__be64 port_rcv_constraint_errors;
		__be64 port_rcv_switch_relay_errors;
		__be64 port_xmit_discards;
		__be64 port_xmit_constraint_errors;
		__be64 port_rcv_remote_physical_errors;
		__be64 local_link_integrity_errors;
		__be64 port_rcv_errors;
		__be64 excessive_buffer_overruns;
		__be64 fm_config_errors;
		__be64 sw_port_congestion;
		__be64 port_rcv_fecn;
		__be64 port_rcv_becn;
		__be64 port_mark_fecn;
		__be32 link_error_recovery;
		__be32 link_downed;
		u8 uncorrectable_errors;
		u8 reserved2[7];
		struct _vl_ifcntrs {
			__be64 port_vl_xmit_discards;
			__be64 Swport_vl_congestion;
			__be64 port_vl_rcv_fecn;
			__be64 port_vl_rcv_becn;
			__be64 port_vl_mark_fecn;
		} vls[0];
	} port[1];
};

struct opa_infrequent_counters_req {
	__be64 port_select_mask[4];
	__be32 vl_select_mask;
	__be32 reserved;
};

struct opa_all_counters_req {
	u8 port_num;
	u8 reserved[3];
	__be32 vl_select_mask;
};

struct opa_all_counters_rsp {
	u8 port_num;
	u8 reserved[3];
	__be32 vl_select_mask;
	u8 link_quality_indicator;
	u8 reserved2[7];
	__be64 node_clocks;
	__be64 port_xmit_data;
	__be64 port_rcv_data;
	__be64 port_xmit_pkts;
	__be64 port_rcv_pkts;
	__be64 port_multicast_xmit_pkts;
	__be64 port_multicast_rcv_pkts;
	__be64 port_xmit_reliable_ltps;
	__be64 port_rcv_reliable_ltps;
	__be64 port_xmit_wait;
	__be64 port_xmit_time_cong;
	__be64 port_xmit_wasted_bw;
	__be64 port_xmit_wait_data;
	__be64 port_rcv_bubble;

	__be64 port_rcv_constraint_errors;
	__be64 port_rcv_switch_relay_errors;
	__be64 port_xmit_discards;
	__be64 port_xmit_constraint_errors;
	__be64 port_rcv_remote_physical_errors;
	__be64 local_link_integrity_errors;
	__be64 port_rcv_errors;
	__be64 excessive_buffer_overruns;
	__be64 fm_config_errors;
	__be64 sw_port_congestion;
	__be64 port_rcv_fecn;
	__be64 port_rcv_becn;
	__be64 port_mark_fecn;
	__be32 link_error_recovery;
	__be32 link_downed;
	u8 uncorrectable_errors;
	u8 reserved3[7];
	struct _vl_all_cntrs {
		__be64 port_vl_xmit_data;
		__be64 port_vl_rcv_data;
		__be64 port_vl_xmit_pkts;
		__be64 port_vl_rcv_pkts;
		__be64 port_vl_xmit_wait;
		__be64 port_vl_xmit_time_cong;
		__be64 port_vl_xmit_wasted_bw;
		__be64 port_vl_xmit_wait_data;
		__be64 port_vl_rcv_bubble;

		__be64 port_vl_xmit_discards;
		__be64 Swport_vl_congestion;
		__be64 port_vl_rcv_fecn;
		__be64 port_vl_rcv_becn;
		__be64 port_vl_mark_fecn;
	} vls[0];
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

enum counter_selects {
	CS_PORT_XMIT_DATA			= (1 << 31),
	CS_PORT_RCV_DATA			= (1 << 30),
	CS_PORT_XMIT_PKTS			= (1 << 29),
	CS_PORT_RCV_PKTS			= (1 << 28),
	CS_PORT_MCAST_XMIT_PKTS			= (1 << 27),
	CS_PORT_MCAST_RCV_PKTS			= (1 << 26),
	CS_PORT_XMIT_WAIT			= (1 << 25),
	CS_SW_PORT_CONGESTION			= (1 << 24),
	CS_PORT_RCV_FECN			= (1 << 23),
	CS_PORT_RCV_BECN			= (1 << 22),
	CS_PORT_XMIT_TIME_CONG			= (1 << 21),
	CS_PORT_XMIT_WASTED_BW			= (1 << 20),
	CS_PORT_XMIT_WAIT_DATA			= (1 << 19),
	CS_PORT_RCV_BUBBLE			= (1 << 18),
	CS_PORT_MARK_FECN			= (1 << 17),
	CS_PORT_RCV_CONSTRAINT_ERRORS		= (1 << 16),
	CS_PORT_RCV_SWITCH_RELAY_ERRORS		= (1 << 15),
	CS_PORT_XMIT_DISCARDS			= (1 << 14),
	CS_PORT_XMIT_CONSTRAINT_ERRORS		= (1 << 13),
	CS_PORT_RCV_REMOTE_PHYSICAL_ERRORS	= (1 << 12),
	CS_LOCAL_LINK_INTEGRITY_ERRORS		= (1 << 11),
	CS_PORT_RCV_ERRORS			= (1 << 10),
	CS_EXCESSIVE_BUFFER_OVERRUNS		= (1 << 9),
	CS_FM_CONFIG_ERRORS			= (1 << 8),
	CS_LINK_ERROR_RECOVERY			= (1 << 7),
	CS_LINK_DOWNED				= (1 << 6),
	CS_UNCORRECTABLE_ERRORS			= (1 << 5),
	CS_PORT_XMIT_RELIABLE_LTPS		= (1 << 4),
	CS_PORT_RCV_RELIABLE_LTPS		= (1 << 3),
	CS_NODE_CLOCKS				= (1 << 2),
};

struct opa_mad_notice_attr {
	u8 generic_type;
	u8 prod_type_msb;
	__be16 prod_type_lsb;
	__be16 trap_num;
	__be16 toggle_count;
	__be32 issuer_lid;
	__be32 reserved1;
	union ib_gid issuer_gid;

	union {
		struct {
			u8	details[64];
		} raw_data;

		struct {
			union ib_gid	gid;
		} __packed ntc_64_65_66_67;

		struct {
			__be32	lid;
		} __packed ntc_128;

		struct {
			__be32	lid;		/* where violation happened */
			u8	port_num;	/* where violation happened */
		} __packed ntc_129_130_131;

		struct {
			__be32	lid;		/* LID where change occurred */
			__be32	new_cap_mask;	/* new capability mask */
			__be16	reserved2;
			__be16	cap_mask3;
			__be16	change_flags;	/* low 4 bits only */
		} __packed ntc_144;

		struct {
			__be64	new_sys_guid;
			__be32	lid;		/* lid where sys guid changed */
		} __packed ntc_145;

		struct {
			__be32	lid;
			__be32	dr_slid;
			u8	method;
			u8	dr_trunc_hop;
			__be16	attr_id;
			__be32	attr_mod;
			__be64	mkey;
			u8	dr_rtn_path[30];
		} __packed ntc_256;

		struct {
			__be32		lid1;
			__be32		lid2;
			__be32		key;
			u8		sl;	/* SL: high 5 bits */
			u8		reserved3[3];
			union ib_gid	gid1;
			union ib_gid	gid2;
			__be32		qp1;	/* high 8 bits reserved */
			__be32		qp2;	/* high 8 bits reserved */
		} __packed ntc_257_258;

		struct {
			__be16		flags;	/* low 8 bits reserved */
			__be16		pkey;
			__be32		lid1;
			__be32		lid2;
			u8		sl;	/* SL: high 5 bits */
			u8		reserved4[3];
			union ib_gid	gid1;
			union ib_gid	gid2;
			__be32		qp1;	/* high 8 bits reserved */
			__be32		qp2;	/* high 8 bits reserved */
		} __packed ntc_259;

		struct {
			__be32	lid;
		} __packed ntc_2048;
	};
	u8	class_data[0];
};

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
#define OPA_TRAP_CHANGE_CAPABILITY	cpu_to_be16(144)
#define OPA_TRAP_CHANGE_SYSGUID		cpu_to_be16(145)
#define OPA_TRAP_BAD_M_KEY		cpu_to_be16(256)
#define OPA_TRAP_BAD_P_KEY		cpu_to_be16(257)
#define OPA_TRAP_BAD_Q_KEY		cpu_to_be16(258)

/*
 * Generic trap/notice other local changes flags (trap 144).
 */
#define	OPA_NOTICE_TRAP_LWDE_CHG	0x08	/* Link Width Downgrade Enable
						 * changed
						 */
#define OPA_NOTICE_TRAP_LSE_CHG		0x04	/* Link Speed Enable changed */
#define OPA_NOTICE_TRAP_LWE_CHG		0x02	/* Link Width Enable changed */
#define OPA_NOTICE_TRAP_NODE_DESC_CHG	0x01

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
#define OPA_CC_LOG_TYPE_HFI	2

enum {
	OPA_TC_MTU_0,
	OPA_TC_MTU_384,
	OPA_TC_MTU_640,
	OPA_TC_MTU_1152,
	OPA_TC_MTU_2176,
	OPA_TC_MTU_4224,
	OPA_TC_MTU_8320,
	OPA_TC_MTU_10368
};

/*
 * lqpn: local qp cn entry
 * rqpn: remote qp that is connected to the local qp
 * sl: service level associated with the local QP
 * svc_type: service tupe of the local QP
 * rlid: LID of the remote poort that is connected to the local QP
 * timestamp: wider than 32 bits to detect 32 bit rollover
 */
struct opa_hfi_cong_log_event_internal {
	u32 lqpn;
	u32 rqpn;
	u8 sl;
	u8 svc_type;
	u32 rlid;
	u64 timestamp;
};

struct opa_hfi_cong_log_event {
	u8 local_qp_cn_entry[3];
	u8 remote_qp_number_cn_entry[3];
	u8 sl_svc_type_cn_entry; /* 5 bits SL, 3 bits svc type */
	u8 reserved;
	__be32 remote_lid_cn_entry;
	__be32 timestamp_cn_entry;
} __packed;

#define OPA_CONG_LOG_ELEMS	96

struct opa_hfi_cong_log {
	u8 log_type;
	u8 congestion_flags;
	__be16 threshold_event_counter;
	__be32 current_time_stamp;
	u8 threshold_cong_event_map[OPA_MAX_SLS / 8];
	struct opa_hfi_cong_log_event events[OPA_CONG_LOG_ELEMS];
} __packed;

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

/*
 * Convert 4bit number format
 * described in IB 1.3 (14.2.5.6) and OPA spec
 * to MTU sizes in bytes
 */
static inline u16 opa_enum_to_mtu(u8 emtu)
{
	switch (emtu) {
	case OPA_MTU_0:     return 0;
	case IB_MTU_256:   return 256;
	case IB_MTU_512:   return 512;
	case IB_MTU_1024:  return 1024;
	case IB_MTU_2048:  return 2048;
	case IB_MTU_4096:  return 4096;
	case OPA_MTU_8192:  return 8192;
	case OPA_MTU_10240: return 10240;
	default: return INVALID_MTU;
	}
}

/*
 * Convert MTU sizes to 4bit number format
 * described in IB 1.3 (14.2.5.6) and OPA spec
 */
static inline u8 opa_mtu_to_enum(u16 mtu)
{
	switch (mtu) {
	case	 0: return OPA_MTU_0;
	case   256: return IB_MTU_256;
	case   512: return IB_MTU_512;
	case  1024: return IB_MTU_1024;
	case  2048: return IB_MTU_2048;
	case  4096: return IB_MTU_4096;
	case  8192: return OPA_MTU_8192;
	case 10240: return OPA_MTU_10240;
	default: return INVALID_MTU_ENC;
	}
}

static inline u16 txotr_enum_to_mtu(u8 enum_val)
{
	switch (enum_val) {
	case OPA_TC_MTU_0:	return 0;
	case OPA_TC_MTU_384:	return 384;
	case OPA_TC_MTU_640:	return 640;
	case OPA_TC_MTU_1152:	return 1152;
	case OPA_TC_MTU_2176:	return 2176;
	case OPA_TC_MTU_4224:	return 4224;
	case OPA_TC_MTU_8320:	return 8320;
	case OPA_TC_MTU_10368:	return 10368;
	default:		return INVALID_MTU;
	}
}

static inline u8 opa_mtu_to_enum_safe(u16 mtu, u8 if_bad_mtu)
{
	u8 ibmtu = opa_mtu_to_enum(mtu);

	return (ibmtu == INVALID_MTU_ENC) ? if_bad_mtu : ibmtu;
}

/*
 * Helper function to read mtu field for a given VL from
 * opa_port_info
 */
static inline u16 opa_pi_to_mtu(struct opa_port_info *pi, int vl_num)
{
	u8 mtu_enc;
	u16 mtu;

	mtu = pi->neigh_mtu.pvlx_to_mtu[vl_num / 2];
	if (!(vl_num % 2))
		mtu_enc = (mtu >> 4) & 0xF;
	else
		mtu_enc = mtu & 0xF;

	mtu = opa_enum_to_mtu(mtu_enc);

	WARN(mtu == INVALID_MTU,
	     "SubnSet(OPA_PortInfo) mtu(enc) %u invalid for VL %d\n",
	     mtu_enc, vl_num);

	return mtu;
}

/*
 * OPA port physical states
 * Returned by the ibphys_portstate() routine.
 */
enum opa_port_phys_state {
	IB_PORTPHYSSTATE_NOP = 0,
	/* 1 is reserved */
	IB_PORTPHYSSTATE_POLLING = 2,
	IB_PORTPHYSSTATE_DISABLED = 3,
	IB_PORTPHYSSTATE_TRAINING = 4,
	IB_PORTPHYSSTATE_LINKUP = 5,
	IB_PORTPHYSSTATE_LINK_ERROR_RECOVERY = 6,
	IB_PORTPHYSSTATE_PHY_TEST = 7,
	/* 8 is reserved */
	OPA_PORTPHYSSTATE_OFFLINE = 9,
	OPA_PORTPHYSSTATE_GANGED = 10,
	OPA_PORTPHYSSTATE_TEST = 11,
	OPA_PORTPHYSSTATE_MAX = 11,
	/* values 12-15 are reserved/ignored */
};

/*
 * States reported by SMA-HFI
 * to SMA-IB
 */
enum opa_sma_status {
	OPA_SMA_SUCCESS = 0,
	OPA_SMA_FAIL_WITH_DATA,
	OPA_SMA_FAIL_WITH_NO_DATA,
};

void hfi_event_pkey_change(struct ib_device *ibdev, u8 port);
void hfi_handle_trap_timer(struct timer_list *t);

#endif				/* _OPA_MAD_H */
