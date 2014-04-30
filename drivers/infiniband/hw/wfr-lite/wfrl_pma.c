/*
 * Copyright (c) 2012 - 2014 Intel Corporation.  All rights reserved.
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

#define STL_PM_ATTRIB_ID_PORT_STATUS			cpu_to_be16(0x0040)
#define STL_PM_ATTRIB_ID_CLEAR_PORT_STATUS			cpu_to_be16(0x0041)
#define STL_PM_ATTRIB_ID_DATA_PORT_COUNTERS		cpu_to_be16(0x0042)
#define STL_PM_ATTRIB_ID_ERROR_PORT_COUNTERS		cpu_to_be16(0x0043)
#define STL_PM_ATTRIB_ID_ERROR_INFO			cpu_to_be16(0x0044)

struct stl_pma_mad {
	struct ib_mad_hdr mad_hdr;
	u8 data[2024];
};

struct stl_port_status_req {
	__u8   port_num;
	__u8   reserved[3];
	__be32 vl_select_mask;
};

enum counter_selects {
	PM_CS_PortXmitData                 = (1 << 0),
	PM_CS_PortRcvData                  = (1 << 1),
	PM_CS_PortXmitPkts                 = (1 << 2),
	PM_CS_PortRcvPkts                  = (1 << 3),
	PM_CS_PortMulticastXmitPkts        = (1 << 4),
	PM_CS_PortMulticastRcvPkts         = (1 << 5),
	PM_CS_PortXmitWait                 = (1 << 6),
	PM_CS_SwPortCongestion             = (1 << 7),
	PM_CS_PortRcvFECN                  = (1 << 8),
	PM_CS_PortRcvBECN                  = (1 << 9),
	PM_CS_PortXmitTimeCong             = (1 << 10),
	PM_CS_PortXmitWastedBW             = (1 << 11),
	PM_CS_PortXmitWaitData             = (1 << 12),
	PM_CS_PortRcvBubble                = (1 << 13),
	PM_CS_PortMarkFECN                 = (1 << 14),
	PM_CS_PortRcvConstraintErrors      = (1 << 15),
	PM_CS_VL15Dropped                  = (1 << 16),
	PM_CS_PortRcvSwitchRelayErrors     = (1 << 17),
	PM_CS_PortXmitDiscards             = (1 << 18),
	PM_CS_PortXmitConstraintErrors     = (1 << 19),
	PM_CS_PortRcvRemotePhysicalErrors  = (1 << 20),
	PM_CS_LocalLinkIntegrityErrors     = (1 << 21),
	PM_CS_PortRcvErrors                = (1 << 22),
	PM_CS_ExcessiveBufferOverruns      = (1 << 23),
	PM_CS_FMConfigErrors               = (1 << 24),
	PM_CS_SymbolErrors                 = (1 << 25),
	PM_CS_LinkErrorRecovery            = (1 << 26),
	PM_CS_LinkDowned                   = (1 << 27),
	PM_CS_UncorrectableErrors          = (1 << 28),
};

enum vl_counter_selects {
	PM_CS_PortVLXmitData     = (1 << 0),
	PM_CS_PortVLRcvData      = (1 << 1),
	PM_CS_PortVLXmitPkts     = (1 << 2),
	PM_CS_PortVLRcvPkts      = (1 << 3),
	PM_CS_PortVLXmitWait     = (1 << 4),
	PM_CS_SwPortVLCongestion = (1 << 5),
	PM_CS_PortVLRcvFECN      = (1 << 6),
	PM_CS_PortVLRcvBECN      = (1 << 7),
	PM_CS_PortVLXmitTimeCong = (1 << 8),
	PM_CS_PortVLXmitWastedBW = (1 << 9),
	PM_CS_PortVLXmitWaitData = (1 << 10),
	PM_CS_PortVLRcvBubble    = (1 << 11),
	PM_CS_PortVLMarkFECN     = (1 << 12),
	PM_CS_PortVLXmitDiscards = (1 << 13),
};

struct stl_clear_port_status {
	__be64 port_select_mask[4];
	__be32 counter_select_mask;
};

struct stl_port_status_rsp {
	__u8   port_num;
	__u8   res1[3];
	__be32 vl_select_mask;

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
	u8 uncorrectabe_errors;

	u8 link_quality_indicator; /* 4res, 4bit */
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

	} vls[1];		/* actual array size defined by number of bits in VLSelectmask */
};

/* Request contains first two fields, response contains those plus the rest */
struct stl_port_data_counters_msg {
	__be64 port_select_mask[4];
	__be32 vl_select_mask;

	/* Response fields follow */
	__be32 reserved1;
	struct _port_dctrs {
		u8 port_number;
		u8 reserved2[3];
		__be32 link_quality_indicator; /* 4res, 4bit */

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

		__be64 port_error_counter_summary; /* Sum of all error counters for port */

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
		} vls[1];		/* actual array size defined by number of bits in VLSelectmask */
	} port[1]; 	/* actual array size defined by number of ports in attribute modifier */
};

/* Request contains first two fields, response contains those plus the rest */
struct stl_port_error_counters_msg {
	__be64 port_select_mask[4];
	__be32 vl_select_mask;

	/* Response fields follow */
	__be32 reserved1;
	struct _port_ectrs {
		u8 portNumber;
		u8 reserved2[7];
		__be64 port_rcv_constraint_errors;
		__be64 vl15_dropped;
		__be64 port_rcv_switch_relay_errors;
		__be64 port_xmit_discards;
		__be64 port_xmit_constraint_errors;
		__be64 port_rcv_remote_physical_errors;
		__be64 local_link_integrity_errors;
		__be64 port_rcv_errors;
		__be64 excessive_buffer_overruns;
		__be64 fm_config_errors;
		__be64 symbol_errors;
		__be32 link_error_recovery;
		__be32 link_downed;
		u8 uncorrectabe_errors;
		u8 reserved3[7];
		struct _vls_ectrs {
			__be64 port_vl_xmit_discards;
		} vls[1];	/* actual array size defined by number of bits in VLSelectmask */
	} port[1]; 	/* actual array size defined by number of ports in attribute modifier */
};

struct stl_port_error_info_msg {
	__be64 port_select_mask[4];
	__be32 error_info_select_mask;
	__be32 reserved1;
	struct _port_ei {

		u8 port_Number;
		u8 reserved2[7];

		u8 status_and_code;
		u8 packet_flit1[8]; /* EI1to12 format */
		u8 packet_flit2[8];
		u8 remaining_flit_bits12;
		u8 reserved3[6];

		u8 excessive_buffer_overrun_info;
		u8 reserved4[7];

		u8 port_xmit_constraint_error_info_status;
		u8 reserved5;
		__be16 port_xmit_constraint_error_info_pkey;
		__be32 port_xmit_constraint_error_info_slid;

		u8 port_rcv_constraint_error_info_status;
		u8 reserved6;
		__be16 port_rcv_constraint_error_info_pkey;
		__be32 port_rcv_constraint_error_info_slid;

		u8 port_rcv_switch_relay_error_info_status;
		u8 reserved7[3];
		__u32 relay_error_code_info;

		u8 uncorrectable_error_info_status;
		u8 reserved8;

		u8 fm_config_error_info_status;
		u8 config_error_info;
		__u32 reserved9;
	} port[1]; 	/* actual array size defined by number of ports in attribute modifier */
};

/* stl_port_error_info_msg error_info_select_mask bit definitions */
enum error_info_selects {
	PM_EIS_PortRcvErrorInfo             = (1 << 31),
	PM_EIS_ExcessiveBufferOverrunInfo   = (1 << 30),
	PM_EIS_PortXmitConstraintErrorInfo  = (1 << 29),
	PM_EIS_PortRcvConstraintErrorInfo   = (1 << 28),
	PM_EIS_PortRcvSwitchRelayErrorInfo  = (1 << 27),
	PM_EIS_UncorrectableErrorInfo       = (1 << 26),
	PM_EIS_FMConfigErrorInfo            = (1 << 25)
};

/* static_test_error_info is purely for mgmt code get/set test*/
static struct stl_port_error_info_msg static_test_error_info;

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

/* With the real WFR, the counters can be cleared so this will not be needed */
static void pma_adjust_counters_for_reset_done(struct qib_verbs_counters *cntrs, 
					       struct qib_ibport *ibp)
{
	cntrs->symbol_error_counter -= ibp->z_symbol_error_counter;
	cntrs->link_error_recovery_counter -=
		ibp->z_link_error_recovery_counter;
	cntrs->link_downed_counter -= ibp->z_link_downed_counter;
	cntrs->port_rcv_errors -= ibp->z_port_rcv_errors;
	cntrs->port_rcv_remphys_errors -= ibp->z_port_rcv_remphys_errors;
	cntrs->port_xmit_discards -= ibp->z_port_xmit_discards;
	cntrs->port_xmit_data -= ibp->z_port_xmit_data;
	cntrs->port_rcv_data -= ibp->z_port_rcv_data;
	cntrs->port_xmit_packets -= ibp->z_port_xmit_packets;
	cntrs->port_rcv_packets -= ibp->z_port_rcv_packets;
	cntrs->local_link_integrity_errors -=
		ibp->z_local_link_integrity_errors;
	cntrs->excessive_buffer_overrun_errors -=
		ibp->z_excessive_buffer_overrun_errors;
	cntrs->vl15_dropped -= ibp->z_vl15_dropped;
	cntrs->vl15_dropped += ibp->n_vl15_dropped;
}

static int pma_get_stl_classportinfo(struct stl_pma_mad *pmp,
				     struct ib_device *ibdev)
{
	struct ib_class_port_info *p =
		(struct ib_class_port_info *)pmp->data;

	memset(pmp->data, 0, sizeof(pmp->data));

	if (pmp->mad_hdr.attr_mod != 0)
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;

	/* Note that AllPortSelect is not valid */
	p->base_version = 0x80;
	p->class_version = 0x80;
	p->capability_mask = 0;
	/*
	 * Expected response time is 4.096 usec. * 2^18 == 1.073741824 sec.
	 */
	p->resp_time_value = 18;

	return reply_stl_pma(pmp);
}

/** =========================================================================
 * Get PortStatus
 */
static int pma_get_stl_portstatus(struct stl_pma_mad *pmp,
				  struct ib_device *ibdev, u8 port)
{
	struct stl_port_status_req *req = (struct stl_port_status_req *)pmp->data;
	struct stl_port_status_rsp *rsp;
	u8 vl_index;
	unsigned long vl;

	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	struct qib_verbs_counters cntrs;
	u32 num_ports = be32_to_cpu(pmp->mad_hdr.attr_mod) >> 24;
	u32 vl_select_mask = be32_to_cpu(req->vl_select_mask);
	u8 num_vls = hweight32(vl_select_mask);
	u8 port_num = req->port_num;


	/* Sanity check */
	size_t response_data_size = sizeof(struct stl_port_status_rsp) -
				sizeof(struct _vls_pctrs) +
				num_vls * sizeof(struct _vls_pctrs);
	if (response_data_size > sizeof(pmp->data)) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		printk(KERN_WARNING PFX "STL PMA Requested Status Response too big for reply!"
			   " %zu vs %zu, num_ports %u, num_vls %u\n",
			   response_data_size, sizeof(pmp->data), num_ports, num_vls);
		return reply_stl_pma(pmp);
	}

	memset(pmp->data, 0, sizeof(pmp->data));
	if (num_ports != 1 || (req->port_num && req->port_num != port)
	    || num_vls > STL_MAX_VLS) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		printk(KERN_WARNING PFX "STL get stl portstatus PMA 0x%x ; Invalid AM; %d; %d; %d\n",
			be16_to_cpu(pmp->mad_hdr.attr_id),
			num_ports, req->port_num, num_vls);
		return (reply_stl_pma(pmp));
	}

	rsp = (struct stl_port_status_rsp *)pmp->data;
	if (port_num)
		rsp->port_num = port_num;
	else
		rsp->port_num = port;
	rsp->vl_select_mask = cpu_to_be32(vl_select_mask);

	qib_get_counters(ppd, &cntrs);
	pma_adjust_counters_for_reset_done(&cntrs, ibp);

	/* The real WFR will have more than this. This is just Suzie-Q info we have */
	rsp->link_error_recovery = cpu_to_be64(cntrs.link_error_recovery_counter);
	rsp->link_downed = cpu_to_be64(cntrs.link_downed_counter);
	rsp->port_rcv_errors = cpu_to_be64(cntrs.port_rcv_errors);

	rsp->port_xmit_discards = cpu_to_be64(cntrs.port_xmit_discards);
	rsp->port_rcv_remote_physical_errors = cpu_to_be64(cntrs.port_rcv_remphys_errors);
	rsp->local_link_integrity_errors = cpu_to_be64(cntrs.local_link_integrity_errors);
	rsp->excessive_buffer_overruns = cpu_to_be64(cntrs.excessive_buffer_overrun_errors);
	rsp->port_xmit_data = cpu_to_be64(cntrs.port_xmit_data);
	rsp->port_rcv_data = cpu_to_be64(cntrs.port_rcv_data);
	rsp->port_xmit_pkts = cpu_to_be64(cntrs.port_xmit_packets);
	rsp->port_rcv_pkts = cpu_to_be64(cntrs.port_rcv_packets);

	rsp->link_quality_indicator = 3;

	/* SuzieQ does not have per-VL counters, but put in some data for debug */
	vl_index = 0;
	for_each_set_bit(vl, (unsigned long *)&(vl_select_mask),
			sizeof(vl_select_mask)) {
		rsp->vls[vl_index].port_vl_xmit_pkts = cpu_to_be64(cntrs.port_xmit_packets);
		rsp->vls[vl_index].port_vl_rcv_pkts = cpu_to_be64(cntrs.port_rcv_packets);
		vl_index++;
	}

	/* TODO - FIXME May need to change the size of the reply, as it will
	 * not match request
	 *
	 * FIXME for upstream
	 * IKW this is because the MAD stack does not currently support
	 * variable sized responses as it would have required changes to the
	 * ib_core which we are trying to avoid until STL is announced.
	 * Once STL is announced the MAD stack can have a separate callback
	 * which the core can provide from HFI's which support jumbo (and
	 * potentially variable sized MAD's)
	 */
	return reply_stl_pma(pmp);
}

/* Set is used to clear the status */
static int pma_set_stl_portstatus(struct stl_pma_mad *pmp,
				struct ib_device *ibdev, u8 port)
{
	struct stl_clear_port_status *clear = (struct stl_clear_port_status *)pmp->data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	struct qib_verbs_counters cntrs;
	u32 num_ports = be32_to_cpu(pmp->mad_hdr.attr_mod) >> 24;

	if (num_ports != 1 ||
	    /* note: per spec, the [3] is where user port 1 (software port 0) is defined */
	    (clear->port_select_mask[3] &&
			be64_to_cpu(clear->port_select_mask[3]) != (1 << port))) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		printk(KERN_WARNING PFX "STL Set PMA 0x%x ; Invalid AM; %d; 0x%llx\n",
			be16_to_cpu(pmp->mad_hdr.attr_id),
			num_ports, be64_to_cpu(clear->port_select_mask[3]));
		return (reply_stl_pma(pmp));
	}

	/*
	 * Since the IB HW that wfr-lite uses doesn't support clearing counters, we save the
	 * current count and subtract it from future responses.
	 */

	qib_get_counters(ppd, &cntrs);

	/** 
	 *  TODO 1/15/2014 Additionally, per Russ, the per VL counters
	 *  are to get cleared. Since Suzie-Q oes not have real per-VL
	 *  counters, the wfr-lite can't do this. Just noting that the
	 *  real WFR driver will have to do clear them too.
	 */
	if (clear->counter_select_mask & PM_CS_SymbolErrors)
		ibp->z_symbol_error_counter = cntrs.symbol_error_counter;

	if (clear->counter_select_mask & PM_CS_LinkErrorRecovery)
		ibp->z_link_error_recovery_counter =
			cntrs.link_error_recovery_counter;

	if (clear->counter_select_mask & PM_CS_LinkDowned)
		ibp->z_link_downed_counter = cntrs.link_downed_counter;

	if (clear->counter_select_mask & PM_CS_PortRcvErrors)
		ibp->z_port_rcv_errors = cntrs.port_rcv_errors;

	if (clear->counter_select_mask & PM_CS_PortRcvRemotePhysicalErrors)
		ibp->z_port_rcv_remphys_errors = cntrs.port_rcv_remphys_errors;

	if (clear->counter_select_mask & PM_CS_PortXmitDiscards)
		ibp->z_port_xmit_discards = cntrs.port_xmit_discards;

	if (clear->counter_select_mask & PM_CS_LocalLinkIntegrityErrors)
		ibp->z_local_link_integrity_errors = cntrs.local_link_integrity_errors;

	if (clear->counter_select_mask & PM_CS_ExcessiveBufferOverruns)
		ibp->z_excessive_buffer_overrun_errors = 
			cntrs.excessive_buffer_overrun_errors;

	if (clear->counter_select_mask & PM_CS_VL15Dropped) {
		ibp->n_vl15_dropped = 0;
		ibp->z_vl15_dropped = cntrs.vl15_dropped;
	}

	if (clear->counter_select_mask & PM_CS_PortXmitData)
		ibp->z_port_xmit_data = cntrs.port_xmit_data;

	if (clear->counter_select_mask & PM_CS_PortRcvData)
		ibp->z_port_rcv_data = cntrs.port_rcv_data;

	if (clear->counter_select_mask & PM_CS_PortXmitPkts)
		ibp->z_port_xmit_packets = cntrs.port_xmit_packets;

	if (clear->counter_select_mask & PM_CS_PortRcvPkts)
		ibp->z_port_rcv_packets = cntrs.port_rcv_packets;

	return reply_stl_pma(pmp);
}

static int pma_get_stl_datacounters(struct stl_pma_mad *pmp,
				struct ib_device *ibdev, u8 port)
{
	struct stl_port_data_counters_msg *req;
	u32 num_ports;
	u8 num_pslm;
	u8 num_vls;
	size_t response_data_size;
	struct _port_dctrs *rsp;
	unsigned long port_num;
	struct _vls_dctrs * vlinfo;
	u8 vl_index;
	unsigned long vl;
	struct qib_ibport *ibp;
	struct qib_pportdata *ppd ;
	struct qib_verbs_counters cntrs;
	u64 port_mask;
	u32 vl_select_mask;

	req = (struct stl_port_data_counters_msg *)pmp->data;

	num_ports = be32_to_cpu(pmp->mad_hdr.attr_mod) >> 24;
	num_pslm = hweight64(be64_to_cpu(req->port_select_mask[3]));
	num_vls = hweight32(be32_to_cpu(req->vl_select_mask));

	if (num_ports != 1 || num_ports != num_pslm) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		printk(KERN_WARNING PFX "STL Get STL Data Counters PMA 0x%x ;"
			   " Invalid Req. np=%u, mask=0x%llx\n",
			be16_to_cpu(pmp->mad_hdr.attr_id), num_ports,
			be64_to_cpu(req->port_select_mask[3]));
		return reply_stl_pma(pmp);
	}

	/* Sanity check */
	response_data_size = sizeof(struct stl_port_data_counters_msg) -
		sizeof(struct _port_dctrs) + num_ports * ( sizeof(struct _port_dctrs) -
		sizeof(struct _vls_dctrs) + num_vls * sizeof(struct _vls_dctrs));
	if (response_data_size > sizeof(pmp->data)) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		printk(KERN_WARNING PFX "STL PMA Requested Response too big for reply!"
			   " %zu vs %zu, num_ports %u, num_vls %u\n",
			   response_data_size, sizeof(pmp->data), num_ports, num_vls);
		return reply_stl_pma(pmp);
	}

	/**
	 * The real WFR will have only one port (port 1), so the bit set
	 * in the mask needs to be consistent with the port the request 
	 * came in on. 
	 */
	port_mask = be64_to_cpu(req->port_select_mask[3]);
	port_num = find_first_bit((unsigned long *)&port_mask,
					 sizeof(port_mask));

	if ((u8)port_num != port) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		printk(KERN_WARNING PFX "PMA Data Requested for Port num %u not valid for HFI\n",
			   (u8)port_num);
		return reply_stl_pma(pmp);
	}

	rsp = (struct _port_dctrs *)&(req->port[0]);

	ibp = to_iport(ibdev, port_num);
	ppd = ppd_from_ibp(ibp);

	qib_get_counters(ppd, &cntrs);
	pma_adjust_counters_for_reset_done(&cntrs, ibp);

	rsp->port_number = (u8)port;
	rsp->link_quality_indicator = cpu_to_be32(3); // just a test value (no equiv in SuzieQ)

	// The real WFR will have more than this. This is just Suzie-Q info we have
	rsp->port_xmit_data = cpu_to_be64(cntrs.port_xmit_data);
	rsp->port_rcv_data = cpu_to_be64(cntrs.port_rcv_data);
	rsp->port_xmit_pkts = cpu_to_be64(cntrs.port_xmit_packets);
	rsp->port_rcv_pkts = cpu_to_be64(cntrs.port_rcv_packets);

	rsp->port_error_counter_summary = 
		cpu_to_be64(cntrs.symbol_error_counter) +
		cpu_to_be64(cntrs.link_error_recovery_counter) +
		cpu_to_be64(cntrs.link_downed_counter) +
		cpu_to_be64(cntrs.port_rcv_errors) +
		cpu_to_be64(cntrs.port_xmit_discards) + 
		cpu_to_be64(cntrs.port_rcv_remphys_errors) +
		cpu_to_be64(cntrs.local_link_integrity_errors) +
		cpu_to_be64(cntrs.excessive_buffer_overrun_errors);

	vlinfo = &(rsp->vls[0]);
	vl_index = 0;
	vl_select_mask = cpu_to_be32(req->vl_select_mask);
	for_each_set_bit(vl, (unsigned long *)&(vl_select_mask),
					 sizeof(req->vl_select_mask))
	{
		/* There is no "per VL" data from SuzieQ, but for
		 * debug, copy some data in. */
		memset(vlinfo, 0, sizeof(*vlinfo));
		vlinfo->port_vl_rcv_pkts  = cpu_to_be64(cntrs.port_rcv_packets);
		vlinfo->port_vl_xmit_pkts = cpu_to_be64(cntrs.port_xmit_packets);
		vlinfo->port_vl_rcv_data = cpu_to_be64(cntrs.port_rcv_data);
		vlinfo->port_vl_xmit_data = cpu_to_be64(cntrs.port_xmit_data);
		vlinfo += 1;
	}
	rsp = (struct _port_dctrs *)vlinfo;

	/* TODO - FIXME May need to change the size of the reply, as it will
	 * not match request
	 *
	 * IKW same comment as above
	 */
	return reply_stl_pma(pmp);
}

static int pma_get_stl_errorcounters(struct stl_pma_mad *pmp,
				struct ib_device *ibdev, u8 port)
{
	size_t response_data_size;
	struct _port_ectrs *rsp;
	unsigned long port_num;
	struct stl_port_error_counters_msg *req;
	u32 num_ports;
	u32 counter_size_mode;
	u8 num_pslm;
	u8 num_vls;
	struct qib_ibport *ibp;
	struct qib_pportdata *ppd;
	struct qib_verbs_counters cntrs;
	struct _vls_ectrs * vlinfo;
	unsigned long vl;
	u64 port_mask;
	u32 vl_select_mask;

	req = (struct stl_port_error_counters_msg *)pmp->data;

	num_ports = be32_to_cpu(pmp->mad_hdr.attr_mod) >> 24;
	counter_size_mode = (be32_to_cpu(pmp->mad_hdr.attr_mod) >> 22) & 0x3;

	num_pslm = hweight64(be64_to_cpu(req->port_select_mask[3]));
	num_vls = hweight32(be32_to_cpu(req->vl_select_mask));

	if (num_ports != 1 || num_ports != num_pslm || 
			(counter_size_mode != 0) /* TODO only ALL64 presently supported */) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		printk(KERN_WARNING PFX "STL Get STL Error Counters PMA 0x%x ;"
			   " Invalid Req. np=%u, mask=0x%llx, mode=%u\n",
			   be16_to_cpu(pmp->mad_hdr.attr_id), num_ports,
			   be64_to_cpu(req->port_select_mask[3]), counter_size_mode);
		return reply_stl_pma(pmp);
	}

	/* Sanity check */
	response_data_size = sizeof(struct stl_port_error_counters_msg) 
		- sizeof(struct _port_ectrs) + num_ports * (sizeof(struct _port_ectrs) 
				- sizeof(struct _vls_ectrs) + num_vls * sizeof(struct _vls_ectrs));
	if (response_data_size > sizeof(pmp->data)) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		printk(KERN_WARNING PFX "STL PMA Requested Error Response too big for reply!" 
			   " %zu vs 0x%zu, num_ports %u, num_vls %u\n",
			   response_data_size, sizeof(pmp->data), num_ports, num_vls);
		return reply_stl_pma(pmp);
	}

	/**
	 * The real WFR will have only one port (port 1), so the bit set
	 * in the mask needs to be consistent with the port the request 
	 * came in on. 
	 */
	port_mask = be64_to_cpu(req->port_select_mask[3]);
	port_num = find_first_bit((unsigned long *)&port_mask,
					 sizeof(port_mask));

	if ((u8)port_num != port) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		printk(KERN_WARNING PFX "PMA Data Requested for Port %u not valid for HFI\n",
			   (u8)port_num);
		return reply_stl_pma(pmp);
	}

	rsp = (struct _port_ectrs *)&(req->port[0]);

	ibp = to_iport(ibdev, port_num);
	ppd = ppd_from_ibp(ibp);

	qib_get_counters(ppd, &cntrs);
	pma_adjust_counters_for_reset_done(&cntrs, ibp);

	memset(rsp, 0, sizeof(*rsp));
	rsp->portNumber = (u8)port_num;

	/* The real WFR will have more than this. This is just Suzie-Q
	 * info we have */
	rsp->symbol_errors = cpu_to_be64(cntrs.symbol_error_counter);
	rsp->link_error_recovery = cpu_to_be64(cntrs.link_error_recovery_counter);
	rsp->link_downed = cpu_to_be64(cntrs.link_downed_counter);
	rsp->port_rcv_errors = cpu_to_be64(cntrs.port_rcv_errors);
	rsp->port_xmit_discards = cpu_to_be64(cntrs.port_xmit_discards);
	rsp->port_rcv_remote_physical_errors = cpu_to_be64(cntrs.port_rcv_remphys_errors);
	rsp->local_link_integrity_errors = cpu_to_be64(cntrs.local_link_integrity_errors);
	rsp->excessive_buffer_overruns = cpu_to_be64(cntrs.excessive_buffer_overrun_errors);
	rsp->vl15_dropped = cpu_to_be64(cntrs.vl15_dropped);

	vlinfo = (struct _vls_ectrs *)&(rsp->vls[0]);
	vl_select_mask = cpu_to_be32(req->vl_select_mask);
	for_each_set_bit(vl, (unsigned long *)&(vl_select_mask),
					 sizeof(req->vl_select_mask))
	{
		/* There is no "per VL" data from SuzieQ, but for
		 * debug, copy some data in. */
		memset(vlinfo, 0, sizeof(*vlinfo));
		vlinfo->port_vl_xmit_discards = cpu_to_be64(cntrs.port_xmit_discards);
		vlinfo += 1;
	}
	rsp = (struct _port_ectrs *)vlinfo;

	/* TODO - FIXME May need to change the size of the reply, as it will
	 * not match request
	 *
	 * IKW same comment as above
	 */
	return reply_stl_pma((struct stl_pma_mad *)pmp);
}

static int pma_get_stl_errorinfo(struct stl_pma_mad *pmp,
				struct ib_device *ibdev, u8 port)
{
	size_t response_data_size;
	struct _port_ei *rsp;
	struct stl_port_error_info_msg *req;
	u64 port_mask;
	u32 num_ports;
	unsigned long port_num;
	u8 num_pslm;

	req = (struct stl_port_error_info_msg *)pmp->data;
	rsp = (struct _port_ei *)&(req->port[0]);

	num_ports = be32_to_cpu(pmp->mad_hdr.attr_mod) >> 24;
	num_pslm = hweight64(be64_to_cpu(req->port_select_mask[3]));

	if (num_ports != 1 || num_ports != num_pslm ) { 
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		printk(KERN_WARNING PFX "STL Get STL Error Info PMA 0x%x ;"
			   " Invalid Req. num ports=%u, mask=0x%llx\n",
			   be16_to_cpu(pmp->mad_hdr.attr_id), num_ports,
			   be64_to_cpu(req->port_select_mask[3]));
		return reply_stl_pma(pmp);
	}

	/* Sanity check */
	response_data_size = sizeof(struct stl_port_error_info_msg) 
		- sizeof(struct _port_ei) + num_ports * (sizeof(struct _port_ei)); 
	if (response_data_size > sizeof(pmp->data)) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		printk(KERN_WARNING PFX "STL PMA Requested Info Response too big for reply!" 
			   " %zu vs 0x%zu, num_ports %u\n",
			   response_data_size, sizeof(pmp->data), num_ports);
		return reply_stl_pma(pmp);
	}

	/**
	 * The real WFR will have only one port (port 1), so the bit set
	 * in the mask needs to be consistent with the port the request 
	 * came in on. 
	 */
	port_mask = be64_to_cpu(req->port_select_mask[3]);
	port_num = find_first_bit((unsigned long *)&port_mask,
					 sizeof(port_mask));

	if ((u8)port_num != port) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		printk(KERN_WARNING PFX "PMA Errors Requested for Port %u not valid for HFI\n",
			   (u8)port_num);
		return reply_stl_pma(pmp);
	}

	/* for testing, populate some non-zeros on first get */
	if (static_test_error_info.reserved1 != 0xbabeface) {
		u8 i;
		memset(&static_test_error_info, 0, sizeof(static_test_error_info));
		static_test_error_info.reserved1 = 0xbabeface;

		static_test_error_info.port[0].port_Number = port;

		static_test_error_info.port[0].status_and_code = 0x83;
		for (i=0; i<8; i++) {
			static_test_error_info.port[0].packet_flit1[i] = i;
			static_test_error_info.port[0].packet_flit2[i] = i;
		}

		static_test_error_info.port[0].excessive_buffer_overrun_info = 0xFF - 3;

		static_test_error_info.port[0].port_xmit_constraint_error_info_status = 
			0x80;
		static_test_error_info.port[0].port_xmit_constraint_error_info_pkey = 
			be16_to_cpu(0x1234);
		static_test_error_info.port[0].port_xmit_constraint_error_info_slid = 
			be32_to_cpu(0x55667788);

		static_test_error_info.port[0].port_rcv_constraint_error_info_status = 
			0x80;
		static_test_error_info.port[0].port_rcv_constraint_error_info_pkey = 
			be16_to_cpu(0x5678);
		static_test_error_info.port[0].port_rcv_constraint_error_info_slid = 
			be32_to_cpu(0x99001122);

		static_test_error_info.port[0].port_rcv_switch_relay_error_info_status =
			0x83;
		static_test_error_info.port[0].relay_error_code_info = 
			be32_to_cpu(0x19181716);

		static_test_error_info.port[0].uncorrectable_error_info_status = 0x81;

		static_test_error_info.port[0].fm_config_error_info_status = 0x82;
		static_test_error_info.port[0].config_error_info = 0x34;
	}

	/* for testing purposes, provide some recognizable test data */
	memcpy(rsp, &static_test_error_info.port[0], sizeof(*rsp));
	rsp->port_Number = (u8)port_num;

	/* TODO - FIXME May need to change the size of the reply, as it will
	 * not match request
	 *
	 * IKW same comment as above
	 */
	return reply_stl_pma((struct stl_pma_mad *)pmp);
}

static int pma_set_stl_errorinfo(struct stl_pma_mad *pmp,
				struct ib_device *ibdev, u8 port)
{
	size_t response_data_size;
	struct _port_ei *rsp;
	struct stl_port_error_info_msg *req;
	u64 port_mask;
	u32 num_ports;
	unsigned long port_num;
	u8 num_pslm;
	u32 error_select_mask;

	req = (struct stl_port_error_info_msg *)pmp->data;
	rsp = (struct _port_ei *)&(req->port[0]);

	num_ports = be32_to_cpu(pmp->mad_hdr.attr_mod) >> 24;
	num_pslm = hweight64(be64_to_cpu(req->port_select_mask[3]));

	if (num_ports != 1 || num_ports != num_pslm ) { 
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		printk(KERN_WARNING PFX "STL Set STL Error Info PMA 0x%x ;"
			   " Invalid Req. num ports=%u, mask=0x%llx\n",
			   be16_to_cpu(pmp->mad_hdr.attr_id), num_ports,
			   be64_to_cpu(req->port_select_mask[3]));
		return reply_stl_pma(pmp);
	}

	/* Sanity check */
	response_data_size = sizeof(struct stl_port_error_info_msg) 
		- sizeof(struct _port_ei) + num_ports * (sizeof(struct _port_ei)); 
	if (response_data_size > sizeof(pmp->data)) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		printk(KERN_WARNING PFX "STL PMA Requested Info Response too big for reply!" 
			   " %zu vs 0x%zu, num_ports %u\n",
			   response_data_size, sizeof(pmp->data), num_ports);
		return reply_stl_pma(pmp);
	}

	/**
	 * The real WFR will have only one port (port 1), so the bit set
	 * in the mask needs to be consistent with the port the request 
	 * came in on. 
	 */
	port_mask = be64_to_cpu(req->port_select_mask[3]);
	port_num = find_first_bit((unsigned long *)&port_mask,
					 sizeof(port_mask));

	if ((u8)port_num != port) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		printk(KERN_WARNING PFX "PMA Errors Requested for Port %u not valid for HFI\n",
			   (u8)port_num);
		return reply_stl_pma(pmp);
	}

	/* for testing, clear the requested static_test_error_info */
	error_select_mask = be32_to_cpu(req->error_info_select_mask);

	if (error_select_mask & PM_EIS_PortRcvErrorInfo) {
		u8 i;
		static_test_error_info.port[0].status_and_code = 0;
		for (i=0; i<8; i++) {
			static_test_error_info.port[0].packet_flit1[i] = 0;
			static_test_error_info.port[0].packet_flit2[i] = 0;
		}
	}

	if (error_select_mask & PM_EIS_ExcessiveBufferOverrunInfo) 
		static_test_error_info.port[0].excessive_buffer_overrun_info = 0;

	if (error_select_mask & PM_EIS_PortXmitConstraintErrorInfo) {
		static_test_error_info.port[0].port_xmit_constraint_error_info_status = 0;
		static_test_error_info.port[0].port_xmit_constraint_error_info_pkey = 0;
		static_test_error_info.port[0].port_xmit_constraint_error_info_slid = 0;
	}

	if (error_select_mask & PM_EIS_PortRcvConstraintErrorInfo) {
		static_test_error_info.port[0].port_rcv_constraint_error_info_status = 0;
		static_test_error_info.port[0].port_rcv_constraint_error_info_pkey = 0;
		static_test_error_info.port[0].port_rcv_constraint_error_info_slid = 0;
	}

	if (error_select_mask & PM_EIS_PortRcvSwitchRelayErrorInfo) {
		static_test_error_info.port[0].port_rcv_switch_relay_error_info_status = 0;
		static_test_error_info.port[0].relay_error_code_info = 0;
	}

	if (error_select_mask & PM_EIS_UncorrectableErrorInfo) 
		static_test_error_info.port[0].uncorrectable_error_info_status = 0;

	if (error_select_mask & PM_EIS_FMConfigErrorInfo) 
		static_test_error_info.port[0].fm_config_error_info_status = 0;

	rsp->port_Number = (u8)port_num;

	/* TODO - FIXME May need to change the size of the reply, as it will
	 * not match request
	 *
	 * IKW same comment as above
	 */
	return reply_stl_pma((struct stl_pma_mad *)pmp);
}

/* Return 0's */
static int pma_send_unsupported_method_response(struct stl_pma_mad *pmp)
{
	pmp->mad_hdr.status |= IB_SMP_UNSUP_METH_ATTR;
	printk(KERN_WARNING PFX "STL PMA Unsupported Attribute 0x%x\n",
			be16_to_cpu(pmp->mad_hdr.attr_id));
	return reply_stl_pma(pmp);
}

int process_stl_perf(struct ib_device *ibdev, u8 port,
			struct jumbo_mad *in_mad,
			struct jumbo_mad *out_mad,
			u32 *resp_len)
{
	struct stl_pma_mad *pmp = (struct stl_pma_mad *)out_mad;
	int ret;

	*out_mad = *in_mad;
	if (pmp->mad_hdr.class_version != STL_PM_CLASS_VERSION) {
		if (pmp->mad_hdr.attr_id != IB_PMA_CLASS_PORT_INFO) {
			pmp->mad_hdr.status |= IB_SMP_UNSUP_VERSION;
			printk(KERN_WARNING PFX "STL Perf bad request received:"
				   " class_ver 0x%x, attr 0x%x, status 0x%x\n",
				   (unsigned int)pmp->mad_hdr.class_version,
				   (unsigned int)be16_to_cpu(pmp->mad_hdr.attr_id),
				   (unsigned int)be16_to_cpu(pmp->mad_hdr.status));
			ret = reply_stl_pma(pmp);
			goto bail;
		}
	}

	/* FIXME decide proper length for all these */
	*resp_len = sizeof(struct jumbo_mad);

	switch (pmp->mad_hdr.method) {
	case IB_MGMT_METHOD_GET:
		switch (pmp->mad_hdr.attr_id) {
		case IB_PMA_CLASS_PORT_INFO:
			ret = pma_get_stl_classportinfo(pmp, ibdev);
			goto bail;
		case STL_PM_ATTRIB_ID_PORT_STATUS:
			ret = pma_get_stl_portstatus(pmp, ibdev, port);
			goto bail;
		case STL_PM_ATTRIB_ID_DATA_PORT_COUNTERS:
			ret = pma_get_stl_datacounters(pmp, ibdev, port);
			goto bail;
		case STL_PM_ATTRIB_ID_ERROR_PORT_COUNTERS:
			ret = pma_get_stl_errorcounters(pmp, ibdev, port);
			goto bail;
		case STL_PM_ATTRIB_ID_ERROR_INFO:
			ret = pma_get_stl_errorinfo(pmp, ibdev, port);
			goto bail;
		default:
			ret = pma_send_unsupported_method_response(pmp);
			goto bail;
		}

	case IB_MGMT_METHOD_SET:
		switch (pmp->mad_hdr.attr_id) {
		case STL_PM_ATTRIB_ID_CLEAR_PORT_STATUS:
			ret = pma_set_stl_portstatus(pmp, ibdev, port);
			goto bail;
		case STL_PM_ATTRIB_ID_ERROR_INFO:
			ret = pma_set_stl_errorinfo(pmp, ibdev, port);
			goto bail;
		case IB_PMA_CLASS_PORT_INFO:
		case STL_PM_ATTRIB_ID_PORT_STATUS:
		case STL_PM_ATTRIB_ID_DATA_PORT_COUNTERS:
		case STL_PM_ATTRIB_ID_ERROR_PORT_COUNTERS:
		default:
			ret = pma_send_unsupported_method_response(pmp);
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
		printk(KERN_WARNING PFX "STL Perf unsupported Method 0x%x received: class_ver 0x%x, attr 0x%x, status 0x%x\n",
			   pmp->mad_hdr.method, pmp->mad_hdr.class_version,
			   be16_to_cpu(pmp->mad_hdr.attr_id),
			   be16_to_cpu(pmp->mad_hdr.status));
		ret = reply_stl_pma(pmp);
	}

bail:
	return ret;
}

