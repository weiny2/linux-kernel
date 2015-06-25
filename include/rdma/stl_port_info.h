/*
 * Copyright (c) 2013 Intel Corporation.  All rights reserved.
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

#if !defined(STL_PORT_INFO_H)
#define STL_PORT_INFO_H

/* STL Port link mode, indicated as follows:
 * values are additive for Supported and Enabled fields
 */
#define STL_PORT_LINK_MODE_NOP	0		/* No change */
#define STL_PORT_LINK_MODE_IB	1		/* Port mode is IB (Gateway) */
#define STL_PORT_LINK_MODE_ETH	2		/* Port mode is ETH (Gateway) */
#define STL_PORT_LINK_MODE_STL	4		/* Port mode is STL */

/* STL Port link formats, indicated as follows:
 * values are additive for Supported and Enabled fields
 */
#define STL_PORT_PACKET_FORMAT_NOP	0		/* No change */
#define STL_PORT_PACKET_FORMAT_8B	1		/* Format 8B */
#define STL_PORT_PACKET_FORMAT_9B	2		/* Format 9B */
#define STL_PORT_PACKET_FORMAT_10B	4		/* Format 10B */
#define STL_PORT_PACKET_FORMAT_16B	8		/* Format 16B */

/* STL Port LTP CRC mode, indicated as follows:
 * values are additive for Supported and Enabled fields
 */
#define STL_PORT_LTP_CRC_MODE_NONE	0	/* No change */
#define STL_PORT_LTP_CRC_MODE_14	1	/* 14-bit LTP CRC mode (optional) */
#define STL_PORT_LTP_CRC_MODE_16	2	/* 16-bit LTP CRC mode */
#define STL_PORT_LTP_CRC_MODE_48	4	/* 48-bit LTP CRC mode (optional) */
#define STL_PORT_LTP_CRC_MODE_12_16_PER_LANE 8	/* 12/16-bit per lane LTP CRC mode */

/* Link Down Reason; indicated as follows: */
#define STL_LINKDOWN_REASON_NONE				0	/* No specified reason */
#define STL_LINKDOWN_REASON_BAD_LT				1
#define STL_LINKDOWN_REASON_BAD_PKT_LEN				2
#define STL_LINKDOWN_REASON_PKT_TOO_LONG			3
#define STL_LINKDOWN_REASON_PKT_TOO_SHORT			4
#define STL_LINKDOWN_REASON_BAD_SLID				5
#define STL_LINKDOWN_REASON_BAD_DLID				6
#define STL_LINKDOWN_REASON_BAD_L2				7
#define STL_LINKDOWN_REASON_BAD_SC				8
#define STL_LINKDOWN_REASON_BAD_MID_TAIL			9
#define STL_LINKDOWN_REASON_BAD_FECN				10
#define STL_LINKDOWN_REASON_PREEMPT_ERROR			11
#define STL_LINKDOWN_REASON_PREEMPT_VL15			12
#define STL_LINKDOWN_REASON_BAD_VL_MARKER			13
#define STL_LINKDOWN_REASON_BAD_HEAD_DIST			14
#define STL_LINKDOWN_REASON_BAD_TAIL_DIST			15
#define STL_LINKDOWN_REASON_BAD_CTRL_DIST			16
#define STL_LINKDOWN_REASON_BAD_CREDIT_ACK			17
#define STL_LINKDOWN_REASON_UNSUPPORTED_VL_MARKER		18
#define STL_LINKDOWN_REASON_BAD_PREEMPT				19
#define STL_LINKDOWN_REASON_BAD_CONTROL_FLIT			20
#define STL_LINKDOWN_REASON_EXCESSIVE_BUFFER_OVERRUN		21

/*  STL Link speed, continued from IB_LINK_SPEED and indicated as follows:
 * values are additive for Supported and Enabled fields
 */
#define STL_LINK_SPEED_RESERVED         0x0080  /*  Reserved (1-5 Gbps) */
#define STL_LINK_SPEED_12_5G            0x0100  /*  12.5 Gbps */
#define STL_LINK_SPEED_25G              0x0200  /*  25.78125?  Gbps (EDR) */
#define STL_LINK_SPEED_RESERVED3        0x0400  /*  Reserved (>25G) */
#define STL_LINK_SPEED_ALL_SUPPORTED    0x03FF  /*  valid only for STL LinkSpeedEnabled */

/*  STL Link width, continued from IB_LINK_WIDTH and indicated as follows:
 * values are additive for Supported and Enabled fields
 */
#define STL_LINK_WIDTH_2X            0x0010
#define STL_LINK_WIDTH_3X            0x0020
#define STL_LINK_WIDTH_6X            0x0040
#define STL_LINK_WIDTH_9X            0x0080
#define STL_LINK_WIDTH_16X           0x0100
#define STL_LINK_WIDTH_ALL_SUPPORTED 0x0FFF /*  valid only for LinkWidthEnabled */

/**
 * STL Cap Mask 3 values
 */
#define STL_CAP_MASK3_IsSnoopSupported            (1 << 7)
#define STL_CAP_MASK3_IsAsyncSC2VLSupported       (1 << 6)
#define STL_CAP_MASK3_IsAddrRangeConfigSupported  (1 << 5)
#define STL_CAP_MASK3_IsPassThroughSupported      (1 << 4)
#define STL_CAP_MASK3_IsSharedSpaceSupported      (1 << 3)
#define STL_CAP_MASK3_IsVL15MulticastSupported    (1 << 2)
#define STL_CAP_MASK3_IsVLMarkerSupported         (1 << 1)
#define STL_CAP_MASK3_IsVLrSupported              (1 << 0)

/**
 * new MTU values
 */
enum {
	STL_MTU_8192  = 8,
	STL_MTU_10240 = 9,
};

enum {
	STL_PORT_PHYS_CONF_DISCONNECTED = 0,
	STL_PORT_PHYS_CONF_STANDARD     = 1,
	STL_PORT_PHYS_CONF_FIXED        = 2,
	STL_PORT_PHYS_CONF_VARIABLE     = 3,
	STL_PORT_PHYS_CONF_SI_PHOTO     = 4
};

enum port_info_field_masks {
	/* vl.inittype */
	STL_PI_MASK_INITTYPE                      = 0x0F,
	/* vl.cap */
	STL_PI_MASK_VL_CAP                        = 0x1F,
	/* port_states.offline_reason */
	STL_PI_MASK_OFFLINE_REASON                = 0x0F,
	/* port_states.unsleepstate_downdefstate */
	STL_PI_MASK_UNSLEEP_STATE                 = 0xF0,
	STL_PI_MASK_DOWNDEF_STATE                 = 0x0F,
	/* port_states.portphysstate_portstate */
	STL_PI_MASK_PORT_PHYSICAL_STATE           = 0xF0,
	STL_PI_MASK_PORT_STATE                    = 0x0F,
	/* port_phys_conf */
	STL_PI_MASK_PORT_PHYSICAL_CONF            = 0x0F,
	/* collectivemask_multicastmask */
	STL_PI_MASK_COLLECT_MASK                  = 0x38,
	STL_PI_MASK_MULTICAST_MASK                = 0x07,
	/* mkeyprotect_lmc */
	STL_PI_MASK_MKEY_PROT_BIT                 = 0xC0,
	STL_PI_MASK_LMC                           = 0x0F,
	/* smsl */
	STL_PI_MASK_SMSL                          = 0x1F,
	/* partenforce_filterraw */
	STL_PI_MASK_PARTITION_ENFORCE_IN          = 0x08,
	STL_PI_MASK_PARTITION_ENFORCE_OUT         = 0x04,
	STL_PI_MASK_FILTER_RAW_IN                 = 0x02,
	STL_PI_MASK_FILTER_RAW_OUT                = 0x01,
	/* operational_vls */
	STL_PI_MASK_OPERATIONAL_VL                = 0x1F,
	/* sa_qp */
	STL_PI_MASK_SA_QP                         = 0x00FFFFFF,
	/* sm_trap_qp */
	STL_PI_MASK_SM_TRAP_QP                    = 0x00FFFFFF,
	/* localphy_overrun_errors */
	STL_PI_MASK_LOCAL_PHY_ERRORS              = 0xF0,
	STL_PI_MASK_OVERRUN_ERRORS                = 0x0F,
	/* clientrereg_subnettimeout */
	STL_PI_MASK_CLIENT_REREGISTER             = 0x80,
	STL_PI_MASK_SUBNET_TIMEOUT                = 0x1F,
	/* port_link_mode */
	STL_PI_MASK_PORT_LINK_SUPPORTED           = (0x001F << 10),
	STL_PI_MASK_PORT_LINK_ENABLED             = (0x001F <<  5),
	STL_PI_MASK_PORT_LINK_ACTIVE              = (0x001F <<  0),
	/* port_link_crc_mode */
	STL_PI_MASK_PORT_LINK_CRC_SUPPORTED       = 0x0F00,
	STL_PI_MASK_PORT_LINK_CRC_ENABLED         = 0x00F0,
	STL_PI_MASK_PORT_LINK_CRC_ACTIVE          = 0x000F,
	/* port_mode */
	STL_PI_MASK_PORT_MODE_SECURITY_CHECK      = 0x0001,
	STL_PI_MASK_PORT_MODE_16B_TRAP_QUERY      = 0x0002,
	STL_PI_MASK_PORT_MODE_PKEY_CONVERT        = 0x0004,
	STL_PI_MASK_PORT_MODE_SC2SC_MAPPING       = 0x0008,
	STL_PI_MASK_PORT_MODE_VL_MARKER           = 0x0010,
	STL_PI_MASK_PORT_PASS_THROUGH             = 0x0020,
	/* flit_control.interleave */
	STL_PI_MASK_INTERLEAVE_DIST_SUP           = (0x0003 << 12),
	STL_PI_MASK_INTERLEAVE_DIST_ENABLE        = (0x0003 << 10),
	STL_PI_MASK_INTERLEAVE_MAX_NEST_TX        = (0x001F <<  5),
	STL_PI_MASK_INTERLEAVE_MAX_NEST_RX        = (0x001F <<  0),

	/* port_error_action */
	STL_PI_MASK_EX_BUFFER_OVERRUN             = 0x80000000,
		/* 3 bits reserved */
	STL_PI_MASK_FM_CFG_BAD_CONTROL_FLIT       = 0x00400000,
	STL_PI_MASK_FM_CFG_BAD_PREEMPT            = 0x00200000,
	STL_PI_MASK_FM_CFG_UNSUPPORTED_VL_MARKER  = 0x00100000,
	STL_PI_MASK_FM_CFG_BAD_CRDT_ACK           = 0x00080000,
	STL_PI_MASK_FM_CFG_BAD_CTRL_DIST          = 0x00040000,
	STL_PI_MASK_FM_CFG_BAD_TAIL_DIST          = 0x00020000,
	STL_PI_MASK_FM_CFG_BAD_HEAD_DIST          = 0x00010000,
		/* 2 bits reserved */
	STL_PI_MASK_PORT_RCV_BAD_VL_MARKER        = 0x00002000,
	STL_PI_MASK_PORT_RCV_PREEMPT_VL15         = 0x00001000,
	STL_PI_MASK_PORT_RCV_PREEMPT_ERROR        = 0x00000800,
	STL_PI_MASK_PORT_RCV_BAD_FECN             = 0x00000400,
	STL_PI_MASK_PORT_RCV_BAD_MidTail          = 0x00000200,
		/* 1 bit reserved */
	STL_PI_MASK_PORT_RCV_BAD_SC               = 0x00000080,
	STL_PI_MASK_PORT_RCV_BAD_L2               = 0x00000040,
	STL_PI_MASK_PORT_RCV_BAD_DLID             = 0x00000020,
	STL_PI_MASK_PORT_RCV_BAD_SLID             = 0x00000010,
	STL_PI_MASK_PORT_RCV_PKTLEN_TOOSHORT      = 0x00000008,
	STL_PI_MASK_PORT_RCV_PKTLEN_TOOLONG       = 0x00000004,
	STL_PI_MASK_PORT_RCV_BAD_PKTLEN           = 0x00000002,
	STL_PI_MASK_PORT_RCV_BAD_LT               = 0x00000001,

	/* pass_through.res_drctl */
	STL_PI_MASK_PASS_THROUGH_DR_CONTROL       = 0x01,

	/* buffer_units */
	STL_PI_MASK_BUF_UNIT_VL15_INIT            = (0x00000FFF  << 11),
	STL_PI_MASK_BUF_UNIT_VL15_CREDIT_RATE     = (0x0000001F  <<  6),
	STL_PI_MASK_BUF_UNIT_CREDIT_ACK           = (0x00000003  <<  3),
	STL_PI_MASK_BUF_UNIT_BUF_ALLOC            = (0x00000003  <<  0),

	/* neigh_mtu.pvlx_to_mtu */
	STL_PI_MASK_NEIGH_MTU_PVL0                = 0xF0,
	STL_PI_MASK_NEIGH_MTU_PVL1                = 0x0F,

	/* neigh_mtu.vlstall_hoq_life */
	STL_PI_MASK_VL_STALL                      = (0x03 << 5),
	STL_PI_MASK_HOQ_LIFE                      = (0x1F << 0),

	/* link_roundtrip_latency */
	STL_PI_MASK_LINK_ROUND_TRIP_LAT           = 0x00FFFFFF,

	/* port_neigh_mode */
	STL_PI_MASK_NEIGH_MGMT_ALLOWED            = (0x01 << 3),
	STL_PI_MASK_NEIGH_FW_AUTH_BYPASS          = (0x01 << 2),
	STL_PI_MASK_NEIGH_NODE_TYPE               = (0x03 << 0),

	/* resptime_value */
	STL_PI_MASK_RESPONSE_TIME_VALUE           = 0x1F,

	/* inittypereply_mtucap */
	STL_PI_MASK_INIT_TYPE_REPLY               = 0xF0,
	STL_PI_MASK_MTU_CAP                       = 0x0F,
};

struct stl_port_info {
	__be32 lid;
	__be32 flow_control_mask;

	struct {
		u8     inittype;                  /* 4 res, 4 bits */
		u8     cap;                       /* 3 res, 5 bits */
		__be16 high_limit;
		__be16 preempt_limit;
		u8     arb_high_cap;
		u8     arb_low_cap;
	} vl;

	struct {
		u8     reserved;
		u8     offline_reason;            /* 4 res,  4 bits */
		u8     unsleepstate_downdefstate; /* 4 bits, 4 bits */
		u8     portphysstate_portstate;   /* 4 bits, 4 bits */
	} port_states;
	u8     port_phys_conf;                    /* 4 res, 4 bits */
	u8     collectivemask_multicastmask;      /* 2 res, 3, 3 */
	u8     mkeyprotect_lmc;                   /* 2 bits, 2 res, 4 bits */
	u8     smsl;                              /* 3 res, 5 bits */

	u8     partenforce_filterraw;             /* 4 res, bit fields */
	u8     operational_vls;                    /* 3 res, 5 bits */
	__be16 pkey_8b;
	__be16 pkey_10b;
	__be16 mkey_violations;

	__be16 pkey_violations;
	__be16 qkey_violations;
	__be32 sm_trap_qp;                        /* 8 bits, 24 bits */

	__be32 sa_qp;                             /* 8 bits, 24 bits */
	u8     reserved2;
	u8     link_down_reason;
	u8     localphy_overrun_errors;	          /* 4 bits, 4 bits */
	u8     clientrereg_subnettimeout;	  /* 1 bit, 2 bits, 5 */

	struct {
		__be16 supported;
		__be16 enabled;
		__be16 active;
	} link_speed;
	struct {
		__be16 supported;
		__be16 enabled;
		__be16 active;
	} link_width;
	struct {
		__be16 supported;
		__be16 enabled;
		__be16 active;
	} link_width_downgrade;
	__be16 reserved3;
	__be16 port_link_mode;                  /* 1 res, 5 bits, 5 bits, 5 bits */
	__be16 port_ltp_crc_mode;               /* 4 res, 4 bits, 4 bits, 4 bits */

	__be16 port_mode;                       /* 10 res, bit fields */
	struct {
		__be16 supported;
		__be16 enabled;
	} port_packet_format;
	struct {
		__be16 interleave;  /* 2 res, 2,2,5,5 */
		struct {
			__be16 min_initial;
			__be16 min_tail;
			u8     large_pkt_limit;
			u8     small_pkt_limit;
			u8     max_small_pkt_limit;
			u8     preemption_limit;
		} preemption;
	} flit_control;

	__be32 reserved4;
	__be32 port_error_action; /* bit field */

	struct {
		u8 egress_port;
		u8 res_drctl;                    /* 7 res, 1 */
	} pass_through;
	__be16 mkey_lease_period;
	__be32 buffer_units;                     /* 9 res, 12, 5, 3, 3 */

	__be32 reserved5;
	__be32 sm_lid;

	__be64 mkey;

	__be64 subnet_prefix;

	struct {
		u8 pvlx_to_mtu[STL_MAX_VLS/2]; /* 4 bits, 4 bits */
	} neigh_mtu;

	struct {
		u8 vlstall_hoqlife;             /* 3 bits, 5 bits */
	} xmit_q[STL_MAX_VLS];


	union ib_gid ip_addr_primary;

	union ib_gid ip_addr_secondary;

	__be64 neigh_node_guid;

	__be32 ib_cap_mask;
	__be16 ib_cap_mask2;
	__be16 stl_cap_mask;

	__be32 link_roundtrip_latency;          /* 8 res, 24 bits */
	__be16 overall_buffer_space;
	__be16 max_credit_hint;

	__be16 diag_code;
	struct {
		u8 buffer;
		u8 wire;
	} replay_depth;
	u8     port_neigh_mode;
	u8     inittypereply_mtucap;            /* 4 bits, 4 bits */

	u8     resptimevalue;		        /* 3 res, 5 bits */
	u8     local_port_num;
	u8     ganged_port_details;
	u8     guid_cap;
} __attribute__ ((packed));

#endif /* STL_PORT_INFO_H */
