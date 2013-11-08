/*
 * Copyright (c) 2012 Intel Corporation.  All rights reserved.
 * Copyright (c) 2006 - 2012 QLogic Corporation. All rights reserved.
 * Copyright (c) 2005, 2006 PathScale, Inc. All rights reserved.
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

#include <linux/types.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <rdma/ib_smi.h>
#include <rdma/stl_smi.h>
#include <rdma/ib_pma.h>

#include "wfrl.h"
#include "wfrl_mad.h"


#define STL_PM_CLASS_VERSION				0x80

#define STL_PM_ATTRIB_ID_PORT_COUNTERS			cpu_to_be16(0x0012)
#define STL_PM_ATTRIB_ID_PORT_STATUS			cpu_to_be16(0x0040)
#define STL_PM_ATTRIB_ID_DATA_PORT_COUNTERS		cpu_to_be16(0x0041)
#define STL_PM_ATTRIB_ID_ERROR_PORT_COUNTERS		cpu_to_be16(0x0042)
#define STL_PM_ATTRIB_ID_ERROR_INFO			cpu_to_be16(0x0043)

struct stl_pma_mad {
	struct ib_mad_hdr mad_hdr;
	u8 data[2024];
};

struct stl_port_status_req {
	__be32 port_num;
	__be32 reserved;
	__be64 vl_select_mask;
};

enum counter_selects {
	PM_CS_PortXmitData                 = (1 << 0),
	PM_CS_PortRcvData                  = (1 << 1),
	PM_CS_PortXmitPkts                 = (1 << 2),
	PM_CS_PortRcvPkts                  = (1 << 3),
	PM_CS_PortMulticastXmitPkts        = (1 << 4),
	PM_CS_PortMulticastRcvPkts         = (1 << 5),
	PM_CS_LocalLinkIntegrityErrors     = (1 << 6),
	PM_CS_FMConfigErrors               = (1 << 7),
	PM_CS_PortRcvErrors                = (1 << 8),
	PM_CS_ExcessiveBufferOverruns      = (1 << 9),
	PM_CS_PortRcvConstraintErrors      = (1 << 10),
	PM_CS_VL15Dropped                  = (1 << 11),
	PM_CS_PortRcvSwitchRelayErrors     = (1 << 12),
	PM_CS_PortXmitDiscards             = (1 << 13),
	PM_CS_PortXmitConstraintErrors     = (1 << 14),
	PM_CS_PortRcvRemotePhysicalErrors  = (1 << 15),
	PM_CS_SymbolErrors                 = (1 << 16),
	PM_CS_SwPortCongestion             = (1 << 17),
	PM_CS_PortXmitWait                 = (1 << 18),
	PM_CS_PortRcvFECN                  = (1 << 19),
	PM_CS_PortRcvBECN                  = (1 << 20),
	PM_CS_PortXmitTimeCong             = (1 << 21),
	PM_CS_PortXmitWastedBW             = (1 << 22),
	PM_CS_PortXmitWaitData             = (1 << 23),
	PM_CS_PortRcvBubble                = (1 << 24),
	PM_CS_PortMarkFECN                 = (1 << 25),
	PM_CS_LinkErrorRecovery            = (1 << 26),
	PM_CS_LinkDowned                   = (1 << 27),
	PM_CS_UncorrectableErrors          = (1 << 28),
};

enum vl_counter_selects {
	PM_CS_PortVLXmitData     = (1 << 0),
	PM_CS_PortVLRcvData      = (1 << 1),
	PM_CS_PortVLXmitPkts     = (1 << 2),
	PM_CS_PortVLRcvPkts      = (1 << 3),
	PM_CS_PortVLXmitDiscards = (1 << 4),
	PM_CS_SwPortVLCongestion = (1 << 5),
	PM_CS_PortVLXmitWait     = (1 << 6),
	PM_CS_PortVLRcvFECN      = (1 << 7),
	PM_CS_PortVLRcvBECN      = (1 << 8),
	PM_CS_PortVLXmitTimeCong = (1 << 9),
	PM_CS_PortVLXmitWastedBW = (1 << 10),
	PM_CS_PortVLXmitWaitData = (1 << 11),
	PM_CS_PortVLRcvBubble    = (1 << 12),
	PM_CS_PortVLMarkFECN     = (1 << 13),
};

struct stl_port_status_set {
	__be64 port_select_mask;
	__be32 counter_select_mask;
	__be32 res1;
	__be64 vl_select_mask;
	__be64 port_xmit_data;
	__be64 port_rcv_data;
	__be64 port_xmit_pkts;
	__be64 port_rcv_pkts;
	__be64 port_multicast_xmit_pkts;
	__be64 port_multicast_rcv_pkts;
	__be64 local_link_integrity_errors;
	__be64 fm_config_errors;
	__be64 port_rcv_errors;
	__be64 excessive_buffer_overruns;
	__be64 port_rcv_constraint_errors;
	__be64 vl15_dropped;
	__be64 port_rcv_switch_relay_errors;
	__be64 port_xmit_discsrds;
	__be64 port_xmit_constraint_errors;
	__be64 port_rcv_emote_physical_errors;
	__be64 symbol_errors;
	__be64 sw_port_congestion;
	__be64 port_xmit_wait;
	__be64 port_rcv_fecn;
	__be64 port_rcv_becn;
	__be64 port_xmit_time_cong;
	__be64 port_xmit_wasted_bw;
	__be64 port_xmit_wait_data;
	__be64 port_rcv_bubble;
	__be64 port_mark_fecn;
	__be32 link_error_recovery;
	__be32 link_downed;
	u8 uncorrectabe_errors;
	u8 link_quality_indicator; /* 4res, 4bit */
	u8 res2[6];
	struct {
		__be32 vl_counter_select_mask; /* enum vl_counter_selects */
		__be32 res1;
		__be64 port_vl_xmit_data;
		__be64 port_vl_rcv_data;
		__be64 port_vl_xmit_pkts;
		__be64 port_vl_rcv_pkts;
		__be64 port_vl_xmit_discards;
		__be64 sw_port_vl_congestion;
		__be64 port_vl_xmit_wait;
		__be64 port_vl_rcv_fecn;
		__be64 port_vl_rcv_becn;
		__be64 port_xmit_time_cong;
		__be64 port_vl_xmit_wasted_bw;
		__be64 port_vl_xmit_wasted_data;
		__be64 port_vl_rcv_bubble;
		__be64 port_vl_mark_fecn;
	} vls[1]; /* n defined by number of bits in vl_select_mask */
};

struct stl_port_status_rsp {
	__be32 port_num;
	__be32 res1;
	__be64 vl_select_mask;
	__be64 port_xmit_data;
	__be64 port_rcv_data;
	__be64 port_xmit_pkts;
	__be64 port_rcv_pkts;
	__be64 port_multicast_xmit_pkts;
	__be64 port_multicast_rcv_pkts;
	__be64 local_link_integrity_errors;
	__be64 fm_config_errors;
	__be64 port_rcv_errors;
	__be64 excessive_buffer_overruns;
	__be64 port_rcv_constraint_errors;
	__be64 vl15_dropped;
	__be64 port_rcv_switch_relay_errors;
	__be64 port_xmit_discards;
	__be64 port_xmit_constraint_errors;
	__be64 port_rcv_emote_physical_errors;
	__be64 symbol_errors;
	__be64 sw_port_congestion;
	__be64 port_xmit_wait;
	__be64 port_rcv_fecn;
	__be64 port_rcv_becn;
	__be64 port_xmit_time_cong;
	__be64 port_xmit_wasted_bw;
	__be64 port_xmit_wait_data;
	__be64 port_rcv_bubble;
	__be64 port_mark_fecn;
	__be32 link_error_recovery;
	__be32 link_downed;
	u8 uncorrectabe_errors;
	u8 link_quality_indicator; /* 4res, 4bit */
	u8 res2[6];
	struct _vls_pctrs {
		__be64 port_vl_xmit_data;
		__be64 port_vl_rcv_data;
		__be64 port_vl_xmit_pkts;
		__be64 port_vl_rcv_pkts;
		__be64 port_vl_xmit_discards;
		__be64 sw_port_vl_congestion;
		__be64 port_vl_xmit_wait;
		__be64 port_vl_rcv_fecn;
		__be64 port_vl_rcv_becn;
		__be64 port_xmit_time_cong;
		__be64 port_vl_xmit_wasted_bw;
		__be64 port_vl_xmit_wasted_data;
		__be64 port_vl_rcv_bubble;
		__be64 port_vl_mark_fecn;
	} vls[1];		/* actual array size defined by number of bits in VLSelectmask */
};

#if 0
/* STL Data Port Counters - small request, bigger response */

struct stl_data_port_counters_req {
	__be64 port_select_mask;					/* signifies for which ports the PMA is to respond */
	__be64 vl_select_mask;					/* signifies for which VLs the PMA is to respond */
};

struct stl_data_port_counters_rsp {
	__be64 port_select_mask;
	__be64 vl_select_mask;
	struct _port_dpctrs {
		uint32 PortNumber;
		union {
			uint32 AsReg32;
			struct { IB_BITFIELD2(uint32,
				Reserved : 28,
				LinkQualityIndicator : 4)
			} PACK_SUFFIX s;
		} lq;
		uint64 PortXmitData;
		uint64 PortRcvData;
		uint64 PortXmitPkts;
		uint64 PortRcvPkts;
		uint64 PortMulticastXmitPkts;
		uint64 PortMulticastRcvPkts;
		uint64 PortErrorCounterSummary;		/* sum of all error counters for port */
		struct _vls_dpctrs {
			uint64 PortVLXmitData;
			uint64 PortVLRcvData;
			uint64 PortVLXmitPkts;
			uint64 PortVLRcvPkts;
		} VLs[1];							/* actual array size defined by number of bits in VLSelectmask */
	} Port[1];								/* actual array size defined by number of ports in attribute modifier */
} PACK_SUFFIX STLDataPortCountersRsp, STL_DATA_PORT_COUNTERS_RSP;

/* STL Error Port Counters - small request, bigger response */

typedef struct _STL_Error_Port_Counters_Req {
	uint64 PortSelectMask;					/* signifies for which ports the PMA is to respond */
	uint64 VLSelectMask;					/* signifies for which VLs the PMA is to respond */
	uint32 LargeCounterCount;				/* number of 64-bit error counters, rest are 32-bit */
	uint32 LargeVLCounterCount;				/* number of 64-bit VL error counters, rest are 32-bit */
} PACK_SUFFIX STLErrorPortCountersReq, STL_ERROR_PORT_COUNTERS_REQ;

typedef struct _STL_Error_Port_Counters_Rsp {
	uint64 PortSelectMask;					/* echo from request */
	uint64 VLSelectMask;					/* echo from request */
	uint32 LargeCounterCount;				/* echo from request */
	uint32 LargeVLCounterCount;				/* echo from request */
	struct _port_epctrs {
		uint32 PortNumber;
		uint32 Reserved;
		uint64 LocalLinkIntegrityErrors;	/* all counters are shown as 64-bit */
		uint64 FMConfigErrors;
		uint64 PortRcvErrors;
		uint64 ExcessiveBufferOverruns;
		uint64 PortRcvConstraintErrors;
		uint64 VL15Dropped;
		uint64 PortRcvSwitchRelayErrors;
		uint64 PortXmitDiscards;
		uint64 PortXmitConstraintErrors;
		uint64 PortRcvRemotePhysicalErrors;
		uint64 SymbolErrors;
		uint64 SwPortCongestion;
		uint64 PortXmitWait;
		uint64 PortRcvFECN;
		uint64 PortRcvBECN;
		uint64 PortXmitTimeCong;
		uint64 PortXmitWastedBW;
		uint64 PortXmitWaitData;
		uint64 PortRcvBubble;
		uint64 PortMarkFECN;
		uint32 LinkErrorRecovery;			/* always 32-bit */
		uint32 LinkDowned;					/* always 32-bit */
		uint8 UncorrectableErrors;			/* always 8-bit */
		uint8 Reserved2[7];					/* may only need 3 reserve bytes if number of 32-bit ctrs is odd */
		struct _vls_epctrs {
			uint64 SwPortVLCongestion;		/* all per-VL counters are shown as 64-bit */
			uint64 PortVLXmitWait;
			uint64 PortVLRcvFECN;
			uint64 PortVLRcvBECN;
			uint64 PortVLXmitTimeCong;
			uint64 PortVLXmitWastedBW;
			uint64 PortVLXmitWaitData;
			uint64 PortVLRcvBubble;
			uint64 PortVLMarkFECN;
		} VLs[1];							/* actual array size defined by number of bits in VLSelectmask */
	} Port[1];								/* actual array size defined by number of ports in attribute modifier */
} PACK_SUFFIX STLErrorPortCountersRsp, STL_ERROR_PORT_COUNTERS_RSP;

typedef struct _STL_Error_Info_Req {
	uint64 PortSelectMask; /* signifies for which ports the PMA is to respond */
	struct _port {
		uint32 PortNumber;
		union {
			uint32 AsReg32;
			struct { IB_BITFIELD8(uint32,
				PortRcvErrorInfo : 1,
				ExcessiveBufferOverrunInfo : 1,
				PortXmitConstraintErrorInfo : 1,
				PortRcvConstraintErrorInfo : 1,
				PortRcvSwitchRelayErrorInfo : 1,
				UncorrectableErrorInfo : 1,
				FMConfigErrorInfo : 1,
				Reserved : 25)
			} PACK_SUFFIX s;
		} ErrorInfoSelectMask;
		struct {
			uint16 P_Key;
			uint16 Reserved1;
			IB_BITFIELD3(uint32,
				Status : 1,
				Reserved2 : 7,
				SLID : 24)
		} PACK_SUFFIX PortXmitConstraintErrorInfo;
		struct {
			uint16 P_Key;
			uint16 Reserved1;
			IB_BITFIELD3(uint32,
				Status : 1,
				Reserved2 : 7,
				SLID : 24)
		} PACK_SUFFIX PortRcvConstraintErrorInfo;
		struct { IB_BITFIELD3(uint32,
				Status : 1,
				Reserved : 3,
				Info : 28)
		} PACK_SUFFIX PortRcvSwitchRelayErrorInfo;
		struct {
			IB_BITFIELD3(uint8,
				Status : 1,
				SC : 5,
				Reserved : 1)
		} PACK_SUFFIX ExcessiveBufferOverrunErrorInfo;
		struct {
			IB_BITFIELD3(uint8,
				Status : 1,
				Reserved : 3,
				ErrorCode : 4)
		} PACK_SUFFIX UncorrectableErrorInfo;
		struct {
			IB_BITFIELD2(uint8,
				Status : 1,
				Reserved : 7);
				uint8 ErrorCode;
		} PACK_SUFFIX FMConfigErrorInfo;
		uint8 Reserved;
		struct {
			IB_BITFIELD2(uint8,
				Status : 1,
				Reserved1 : 7)
			uint8 ErrorInfo[17]; /* Two flits plus status code */
			uint8 Reserved2[6];
		} PortRcvErrorInfo;
	} Port[1]; /* x defined by number of ports in attribute modifier */
} PACK_SUFFIX STLErrorInfoReq, STL_ERROR_INFO_REQ;

#endif


/**
 * BEGIN Functions
 */
static int reply_stl_pma(struct stl_pma_mad *pmp)
{
	/*
	 * The verbs framework will handle the directed/LID route
	 * packet changes.
	 */
	pmp->mad_hdr.method = IB_MGMT_METHOD_GET_RESP;
	if (pmp->mad_hdr.mgmt_class == IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE)
		pmp->mad_hdr.status |= IB_SMP_DIRECTION;
	return IB_MAD_RESULT_SUCCESS | IB_MAD_RESULT_REPLY;
}

static int pma_get_stl_classportinfo(struct stl_pma_mad *pmp,
				 struct ib_device *ibdev)
{
	struct ib_class_port_info *p =
		(struct ib_class_port_info *)pmp->data;
	struct qib_devdata *dd = dd_from_ibdev(ibdev);

	memset(pmp->data, 0, sizeof(pmp->data));

	if (pmp->mad_hdr.attr_mod != 0)
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;

	/* Note that AllPortSelect is not valid */
	p->base_version = 0x80;
	p->class_version = 0x80;
	p->capability_mask = 0;
	/*
	 * Set the most significant bit of CM2 to indicate support for
	 * congestion statistics
	 */
	p->reserved[0] = dd->psxmitwait_supported << 7;
	/*
	 * Expected response time is 4.096 usec. * 2^18 == 1.073741824 sec.
	 */
	p->resp_time_value = 18;

	return reply_stl_pma(pmp);
}


static int pma_get_stl_portcounters(struct ib_pma_mad *pmp,
				struct ib_device *ibdev, u8 port)
{
	struct ib_pma_portcounters *p = (struct ib_pma_portcounters *)
		pmp->data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	struct qib_verbs_counters cntrs;
	u8 port_select = p->port_select;

	if (pmp->mad_hdr.attr_mod != 0 || port_select != port) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		return reply_stl_pma((struct stl_pma_mad *)pmp);
	}

	qib_get_counters(ppd, &cntrs);

	/* Adjust counters for any resets done. */
	cntrs.symbol_error_counter -= ibp->z_symbol_error_counter;
	cntrs.link_error_recovery_counter -=
		ibp->z_link_error_recovery_counter;
	cntrs.link_downed_counter -= ibp->z_link_downed_counter;
	cntrs.port_rcv_errors -= ibp->z_port_rcv_errors;
	cntrs.port_rcv_remphys_errors -= ibp->z_port_rcv_remphys_errors;
	cntrs.port_xmit_discards -= ibp->z_port_xmit_discards;
	cntrs.port_xmit_data -= ibp->z_port_xmit_data;
	cntrs.port_rcv_data -= ibp->z_port_rcv_data;
	cntrs.port_xmit_packets -= ibp->z_port_xmit_packets;
	cntrs.port_rcv_packets -= ibp->z_port_rcv_packets;
	cntrs.local_link_integrity_errors -=
		ibp->z_local_link_integrity_errors;
	cntrs.excessive_buffer_overrun_errors -=
		ibp->z_excessive_buffer_overrun_errors;
	cntrs.vl15_dropped -= ibp->z_vl15_dropped;
	cntrs.vl15_dropped += ibp->n_vl15_dropped;

	memset(pmp->data, 0, sizeof(pmp->data));

	p->port_select = port_select;
	if (cntrs.symbol_error_counter > 0xFFFFUL)
		p->symbol_error_counter = cpu_to_be16(0xFFFF);
	else
		p->symbol_error_counter =
			cpu_to_be16((u16)cntrs.symbol_error_counter);
	if (cntrs.link_error_recovery_counter > 0xFFUL)
		p->link_error_recovery_counter = 0xFF;
	else
		p->link_error_recovery_counter =
			(u8)cntrs.link_error_recovery_counter;
	if (cntrs.link_downed_counter > 0xFFUL)
		p->link_downed_counter = 0xFF;
	else
		p->link_downed_counter = (u8)cntrs.link_downed_counter;
	if (cntrs.port_rcv_errors > 0xFFFFUL)
		p->port_rcv_errors = cpu_to_be16(0xFFFF);
	else
		p->port_rcv_errors =
			cpu_to_be16((u16) cntrs.port_rcv_errors);
	if (cntrs.port_rcv_remphys_errors > 0xFFFFUL)
		p->port_rcv_remphys_errors = cpu_to_be16(0xFFFF);
	else
		p->port_rcv_remphys_errors =
			cpu_to_be16((u16)cntrs.port_rcv_remphys_errors);
	if (cntrs.port_xmit_discards > 0xFFFFUL)
		p->port_xmit_discards = cpu_to_be16(0xFFFF);
	else
		p->port_xmit_discards =
			cpu_to_be16((u16)cntrs.port_xmit_discards);
	if (cntrs.local_link_integrity_errors > 0xFUL)
		cntrs.local_link_integrity_errors = 0xFUL;
	if (cntrs.excessive_buffer_overrun_errors > 0xFUL)
		cntrs.excessive_buffer_overrun_errors = 0xFUL;
	p->link_overrun_errors = (cntrs.local_link_integrity_errors << 4) |
		cntrs.excessive_buffer_overrun_errors;
	if (cntrs.vl15_dropped > 0xFFFFUL)
		p->vl15_dropped = cpu_to_be16(0xFFFF);
	else
		p->vl15_dropped = cpu_to_be16((u16)cntrs.vl15_dropped);
	if (cntrs.port_xmit_data > 0xFFFFFFFFUL)
		p->port_xmit_data = cpu_to_be32(0xFFFFFFFF);
	else
		p->port_xmit_data = cpu_to_be32((u32)cntrs.port_xmit_data);
	if (cntrs.port_rcv_data > 0xFFFFFFFFUL)
		p->port_rcv_data = cpu_to_be32(0xFFFFFFFF);
	else
		p->port_rcv_data = cpu_to_be32((u32)cntrs.port_rcv_data);
	if (cntrs.port_xmit_packets > 0xFFFFFFFFUL)
		p->port_xmit_packets = cpu_to_be32(0xFFFFFFFF);
	else
		p->port_xmit_packets =
			cpu_to_be32((u32)cntrs.port_xmit_packets);
	if (cntrs.port_rcv_packets > 0xFFFFFFFFUL)
		p->port_rcv_packets = cpu_to_be32(0xFFFFFFFF);
	else
		p->port_rcv_packets =
			cpu_to_be32((u32) cntrs.port_rcv_packets);

	return reply_stl_pma((struct stl_pma_mad *)pmp);
}
static int pma_set_stl_portcounters(struct ib_pma_mad *pmp,
				struct ib_device *ibdev, u8 port)
{
	struct ib_pma_portcounters *p = (struct ib_pma_portcounters *)
		pmp->data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	struct qib_verbs_counters cntrs;

	if (pmp->mad_hdr.attr_mod != 0 || p->port_select != port) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		return reply_stl_pma((struct stl_pma_mad *)pmp);
	}

	/*
	 * Since the HW doesn't support clearing counters, we save the
	 * current count and subtract it from future responses.
	 */
	qib_get_counters(ppd, &cntrs);

	if (p->counter_select & IB_PMA_SEL_SYMBOL_ERROR)
		ibp->z_symbol_error_counter = cntrs.symbol_error_counter;

	if (p->counter_select & IB_PMA_SEL_LINK_ERROR_RECOVERY)
		ibp->z_link_error_recovery_counter =
			cntrs.link_error_recovery_counter;

	if (p->counter_select & IB_PMA_SEL_LINK_DOWNED)
		ibp->z_link_downed_counter = cntrs.link_downed_counter;

	if (p->counter_select & IB_PMA_SEL_PORT_RCV_ERRORS)
		ibp->z_port_rcv_errors = cntrs.port_rcv_errors;

	if (p->counter_select & IB_PMA_SEL_PORT_RCV_REMPHYS_ERRORS)
		ibp->z_port_rcv_remphys_errors =
			cntrs.port_rcv_remphys_errors;

	if (p->counter_select & IB_PMA_SEL_PORT_XMIT_DISCARDS)
		ibp->z_port_xmit_discards = cntrs.port_xmit_discards;

	if (p->counter_select & IB_PMA_SEL_LOCAL_LINK_INTEGRITY_ERRORS)
		ibp->z_local_link_integrity_errors =
			cntrs.local_link_integrity_errors;

	if (p->counter_select & IB_PMA_SEL_EXCESSIVE_BUFFER_OVERRUNS)
		ibp->z_excessive_buffer_overrun_errors =
			cntrs.excessive_buffer_overrun_errors;

	if (p->counter_select & IB_PMA_SEL_PORT_VL15_DROPPED) {
		ibp->n_vl15_dropped = 0;
		ibp->z_vl15_dropped = cntrs.vl15_dropped;
	}

	if (p->counter_select & IB_PMA_SEL_PORT_XMIT_DATA)
		ibp->z_port_xmit_data = cntrs.port_xmit_data;

	if (p->counter_select & IB_PMA_SEL_PORT_RCV_DATA)
		ibp->z_port_rcv_data = cntrs.port_rcv_data;

	if (p->counter_select & IB_PMA_SEL_PORT_XMIT_PACKETS)
		ibp->z_port_xmit_packets = cntrs.port_xmit_packets;

	if (p->counter_select & IB_PMA_SEL_PORT_RCV_PACKETS)
		ibp->z_port_rcv_packets = cntrs.port_rcv_packets;

	return pma_get_stl_portcounters(pmp, ibdev, port);
}


/** =========================================================================
 * Get/Set PortStatus
 */
static int pma_get_stl_portstatus(struct stl_pma_mad *pmp,
				struct ib_device *ibdev, u8 port)
{
	struct stl_port_status_req *req = (struct stl_port_status_req *)pmp->data;
	struct stl_port_status_rsp *rsp;

	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	struct qib_verbs_counters cntrs;
	u8 num_ports = be32_to_cpu(pmp->mad_hdr.attr_mod) >> 24;
	u8 num_vls = hweight64(be64_to_cpu(req->vl_select_mask));

	memset(pmp->data, 0, sizeof(pmp->data));
	if (num_ports != 1 || (req->port_num && req->port_num != port)
	    || num_vls > STL_MAX_VLS) {
		printk(KERN_WARNING PFX "STL Set PMA 0x%x ; Invalid AM; %d; %d; %d\n",
			be16_to_cpu(pmp->mad_hdr.attr_id),
			num_ports, req->port_num, num_vls);
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		return (reply_stl_pma(pmp));
	}

	rsp = (struct stl_port_status_rsp *)pmp->data;

	qib_get_counters(ppd, &cntrs);

	/* Adjust counters for any resets done. */
	cntrs.symbol_error_counter -= ibp->z_symbol_error_counter;
	cntrs.link_error_recovery_counter -=
		ibp->z_link_error_recovery_counter;
	cntrs.link_downed_counter -= ibp->z_link_downed_counter;
	cntrs.port_rcv_errors -= ibp->z_port_rcv_errors;
	cntrs.port_rcv_remphys_errors -= ibp->z_port_rcv_remphys_errors;
	cntrs.port_xmit_discards -= ibp->z_port_xmit_discards;
	cntrs.port_xmit_data -= ibp->z_port_xmit_data;
	cntrs.port_rcv_data -= ibp->z_port_rcv_data;
	cntrs.port_xmit_packets -= ibp->z_port_xmit_packets;
	cntrs.port_rcv_packets -= ibp->z_port_rcv_packets;
	cntrs.local_link_integrity_errors -=
		ibp->z_local_link_integrity_errors;
	cntrs.excessive_buffer_overrun_errors -=
		ibp->z_excessive_buffer_overrun_errors;
	cntrs.vl15_dropped -= ibp->z_vl15_dropped;
	cntrs.vl15_dropped += ibp->n_vl15_dropped;

	rsp->symbol_errors = cpu_to_be64(cntrs.symbol_error_counter);
	rsp->link_error_recovery = cpu_to_be64(cntrs.link_error_recovery_counter);
	rsp->link_downed = cpu_to_be64(cntrs.link_downed_counter);
	rsp->port_rcv_errors = cpu_to_be64(cntrs.port_rcv_errors);

	rsp->port_xmit_discards = cpu_to_be64(cntrs.port_xmit_discards);

	rsp->local_link_integrity_errors = cpu_to_be64(cntrs.local_link_integrity_errors);
	rsp->excessive_buffer_overruns = cpu_to_be64(cntrs.excessive_buffer_overrun_errors);
	rsp->vl15_dropped = cpu_to_be64(cntrs.vl15_dropped);
	rsp->port_xmit_data = cpu_to_be64(cntrs.port_xmit_data);
	rsp->port_rcv_data = cpu_to_be64(cntrs.port_rcv_data);
	rsp->port_xmit_pkts = cpu_to_be64(cntrs.port_xmit_packets);
	rsp->port_rcv_pkts = cpu_to_be64(cntrs.port_rcv_packets);

	rsp->link_quality_indicator = 3;

#if 0
/* nothing to be done here since the HW does not have per VL counters... */
	for_each_set_bit(bit, (u64 *)&req->vl_select_mask,
			sizeof(req->vl_select_mask)) {
	}
#endif

	return reply_stl_pma((struct stl_pma_mad *)pmp);
}
static int pma_set_stl_portstatus(struct stl_pma_mad *pmp,
				struct ib_device *ibdev, u8 port)
{
	struct stl_port_status_set *set = (struct stl_port_status_set *)pmp->data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	struct qib_verbs_counters cntrs;
	u8 num_ports = be32_to_cpu(pmp->mad_hdr.attr_mod) >> 24;

	if (num_ports != 1 ||
	    (set->port_select_mask &&
	    be64_to_cpu(set->port_select_mask) != (1 << (port-1)))) {
		printk(KERN_WARNING PFX "STL Set PMA 0x%x ; Invalid AM; %d; 0x%llx\n",
			be16_to_cpu(pmp->mad_hdr.attr_id),
			num_ports, be64_to_cpu(set->port_select_mask));
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		return (reply_stl_pma(pmp));
	}

	/*
	 * Since the HW doesn't support clearing counters, we save the
	 * current count and subtract it from future responses.
	 */
	qib_get_counters(ppd, &cntrs);

	if (set->counter_select_mask & PM_CS_SymbolErrors)
		ibp->z_symbol_error_counter = cntrs.symbol_error_counter;

	if (set->counter_select_mask & PM_CS_LinkErrorRecovery)
		ibp->z_link_error_recovery_counter =
			cntrs.link_error_recovery_counter;

	if (set->counter_select_mask & PM_CS_LinkDowned)
		ibp->z_link_downed_counter = cntrs.link_downed_counter;

	if (set->counter_select_mask & PM_CS_PortRcvErrors)
		ibp->z_port_rcv_errors = cntrs.port_rcv_errors;

	if (set->counter_select_mask & PM_CS_PortRcvRemotePhysicalErrors)
		ibp->z_port_rcv_remphys_errors =
			cntrs.port_rcv_remphys_errors;

	if (set->counter_select_mask & PM_CS_PortXmitDiscards)
		ibp->z_port_xmit_discards = cntrs.port_xmit_discards;

	if (set->counter_select_mask & PM_CS_LocalLinkIntegrityErrors)
		ibp->z_local_link_integrity_errors =
			cntrs.local_link_integrity_errors;

	if (set->counter_select_mask & PM_CS_ExcessiveBufferOverruns)
		ibp->z_excessive_buffer_overrun_errors =
			cntrs.excessive_buffer_overrun_errors;

	if (set->counter_select_mask & PM_CS_VL15Dropped) {
		ibp->n_vl15_dropped = 0;
		ibp->z_vl15_dropped = cntrs.vl15_dropped;
	}

	if (set->counter_select_mask & PM_CS_PortXmitData)
		ibp->z_port_xmit_data = cntrs.port_xmit_data;

	if (set->counter_select_mask & PM_CS_PortRcvData)
		ibp->z_port_rcv_data = cntrs.port_rcv_data;

	if (set->counter_select_mask & PM_CS_PortXmitPkts)
		ibp->z_port_xmit_packets = cntrs.port_xmit_packets;

	if (set->counter_select_mask & PM_CS_PortRcvPkts)
		ibp->z_port_rcv_packets = cntrs.port_rcv_packets;

	return reply_stl_pma((struct stl_pma_mad *)pmp);
}

/* Return 0's */
static int pma_get_stl_stub(struct stl_pma_mad *pmp, struct ib_device *ibdev, u8 port)
{
	printk(KERN_WARNING PFX "STL Get PMA Attribute 0x%x stubbed out; returning 0's\n",
			be16_to_cpu(pmp->mad_hdr.attr_id));
	memset(pmp->data, 0, sizeof(pmp->data));
	return reply_stl_pma(pmp);
}
static int pma_set_stl_stub(struct stl_pma_mad *pmp, struct ib_device *ibdev, u8 port)
{
	printk(KERN_WARNING PFX "STL Set PMA Attribute 0x%x stubbed out; returning 0's\n",
			be16_to_cpu(pmp->mad_hdr.attr_id));
	memset(pmp->data, 0, sizeof(pmp->data));
	return reply_stl_pma(pmp);
}

int process_stl_perf(struct ib_device *ibdev, u8 port,
			struct jumbo_mad *in_mad,
			struct jumbo_mad *out_mad)
{
	struct stl_pma_mad *pmp = (struct stl_pma_mad *)out_mad;
	int ret;

	*out_mad = *in_mad;
	if (pmp->mad_hdr.class_version != STL_PM_CLASS_VERSION) {
		pmp->mad_hdr.status |= IB_SMP_UNSUP_VERSION;
		ret = reply_stl_pma(pmp);
		goto bail;
	}

	switch (pmp->mad_hdr.method) {
	case IB_MGMT_METHOD_GET:
		switch (pmp->mad_hdr.attr_id) {
		case IB_PMA_CLASS_PORT_INFO:
			ret = pma_get_stl_classportinfo(pmp, ibdev);
			goto bail;
		case STL_PM_ATTRIB_ID_PORT_COUNTERS:
			ret = pma_get_stl_portcounters((struct ib_pma_mad *)pmp, ibdev, port);
			goto bail;
		case STL_PM_ATTRIB_ID_PORT_STATUS:
			ret = pma_get_stl_portstatus(pmp, ibdev, port);
			goto bail;
		case STL_PM_ATTRIB_ID_DATA_PORT_COUNTERS:
			ret = pma_get_stl_stub(pmp, ibdev, port);
			goto bail;
		case STL_PM_ATTRIB_ID_ERROR_PORT_COUNTERS:
			ret = pma_get_stl_stub(pmp, ibdev, port);
			goto bail;
		case STL_PM_ATTRIB_ID_ERROR_INFO:
			ret = pma_get_stl_stub(pmp, ibdev, port);
			goto bail;
		default:
			pmp->mad_hdr.status |= IB_SMP_UNSUP_METH_ATTR;
			ret = reply_stl_pma(pmp);
			goto bail;
		}

	case IB_MGMT_METHOD_SET:
		switch (pmp->mad_hdr.attr_id) {
		case STL_PM_ATTRIB_ID_PORT_COUNTERS:
			ret = pma_set_stl_portcounters((struct ib_pma_mad *)pmp, ibdev, port);
			goto bail;
		case STL_PM_ATTRIB_ID_PORT_STATUS:
			ret = pma_set_stl_portstatus(pmp, ibdev, port);
			goto bail;
		case STL_PM_ATTRIB_ID_DATA_PORT_COUNTERS:
			ret = pma_set_stl_stub(pmp, ibdev, port);
			goto bail;
		case STL_PM_ATTRIB_ID_ERROR_PORT_COUNTERS:
			ret = pma_set_stl_stub(pmp, ibdev, port);
			goto bail;
		case STL_PM_ATTRIB_ID_ERROR_INFO:
			ret = pma_set_stl_stub(pmp, ibdev, port);
			goto bail;
		default:
			pmp->mad_hdr.status |= IB_SMP_UNSUP_METH_ATTR;
			ret = reply_stl_pma(pmp);
			goto bail;
		}

	case IB_MGMT_METHOD_TRAP:
	case IB_MGMT_METHOD_GET_RESP:
		/*
		 * The ib_mad module will call us to process responses
		 * before checking for other consumers.
		 * Just tell the caller to process it normally.
		 */
		ret = IB_MAD_RESULT_SUCCESS;
		goto bail;

	default:
		pmp->mad_hdr.status |= IB_SMP_UNSUP_METHOD;
		ret = reply_stl_pma(pmp);
	}

bail:
	return ret;
}

