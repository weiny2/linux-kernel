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
/*
 * This file contains all of the code that is specific to
 * Link Negotiation and Initialization(LNI)
 */

#include <linux/delay.h>
#include <linux/interrupt.h>
#include "chip/fxr_top_defs.h"
#include "chip/fxr_linkmux_fpc_defs.h"
#include "chip/fxr_linkmux_tp_defs.h"
#include "chip/fxr_linkmux_defs.h"
#include "chip/fxr_oc_defs.h"
#include "chip/fxr_8051_defs.h"
#include "hfi2.h"
#include "link.h"
#include "firmware.h"
#ifdef CONFIG_HFI2_STLNP
#include "timesync.h"
#endif
#include "trace.h"
#include "qsfp.h"

#define WAIT_TILL_8051_LINKUP 1000000

/* TODO: delete all module parameters when possible */
bool quick_linkup; /* skip VerifyCap and Config* state. */
module_param(quick_linkup, bool, 0444);
MODULE_PARM_DESC(quick_linkup, "quick linkup, i.e. skip Verifycap and goes to Init");

static void handle_linkup_change(struct hfi_pportdata *ppd, u32 linkup);
static u32 read_physical_state(const struct hfi_pportdata *ppd);
static void get_linkup_link_widths(struct hfi_pportdata *ppd);

static int write_local_device_id(struct hfi_pportdata *ppd, u16 device_id,
				 u8 device_rev)
{
	u32 frame;

	frame = ((u32)device_id << LOCAL_DEVICE_ID_SHIFT)
		| ((u32)device_rev << LOCAL_DEVICE_REV_SHIFT);
	return load_8051_config(ppd, LOCAL_DEVICE_ID, GENERAL_CONFIG, frame);
}

static void read_remote_device_id(struct hfi_pportdata *ppd, u16 *device_id,
				  u8 *device_rev)
{
	u32 frame;

	hfi2_read_8051_config(ppd, REMOTE_DEVICE_ID, GENERAL_CONFIG, &frame);
	*device_id = (frame >> REMOTE_DEVICE_ID_SHIFT) & REMOTE_DEVICE_ID_MASK;
	*device_rev = (frame >> REMOTE_DEVICE_REV_SHIFT)
			& REMOTE_DEVICE_REV_MASK;
}

static void read_vc_remote_fabric(struct hfi_pportdata *ppd, u8 *vau, u8 *z,
				  u8 *vcu, u16 *vl15buf, u8 *crc_sizes)
{
	u32 frame;

	hfi2_read_8051_config(ppd, VERIFY_CAP_REMOTE_FABRIC,
			      GENERAL_CONFIG, &frame);
	*vau = (frame >> HFI_VAU_SHIFT) & HFI_VAU_MASK;
	*z = (frame >> HFI_Z_SHIFT) & HFI_Z_MASK;
	*vcu = (frame >> HFI_VCU_SHIFT) & HFI_VCU_MASK;
	*vl15buf = (frame >> HFI_VL15BUF_SHIFT) & HFI_VL15BUF_MASK;
	*crc_sizes = (frame >> HFI_CRC_SIZES_SHIFT) & HFI_CRC_SIZES_MASK;
}

static int write_vc_local_fabric(struct hfi_pportdata *ppd, u8 vau, u8 z,
				 u8 vcu, u16 vl15buf, u8 crc_sizes)
{
	u32 frame;

	frame = (u32)vau << HFI_VAU_SHIFT
		| (u32)z << HFI_Z_SHIFT
		| (u32)vcu << HFI_VCU_SHIFT
		| (u32)vl15buf << HFI_VL15BUF_SHIFT
		| (u32)crc_sizes << HFI_CRC_SIZES_SHIFT;
	return load_8051_config(ppd, VERIFY_CAP_LOCAL_FABRIC,
				GENERAL_CONFIG, frame);
}

void hfi_read_link_quality(struct hfi_pportdata *ppd, u8 *link_quality)
{
	u32 frame;
	int ret;

	if (no_mnh) {
		/* 0x5 means "Excellent" */
		*link_quality = 0x5;
		return;
	}

	*link_quality = 0x1;
	if (ppd->host_link_state & HLS_UP) {
		ret = hfi2_read_8051_config(ppd, LINK_QUALITY_INFO,
					    GENERAL_CONFIG, &frame);
		ppd_dev_dbg(ppd, "link quality info is %#x\n", frame);
		if (ret == 0) {
			*link_quality = (frame >> LINK_QUALITY_SHIFT)
						& LINK_QUALITY_MASK;
		}
	}
}

/*
 * Mask conversion: Capability exchange to Port LTP.  The capability
 * exchange has an implicit 16b CRC that is mandatory.
 */
u16 hfi_cap_to_port_ltp(u16 cap)
{
	/* this mode is mandatory */
	u16 port_ltp = OPA_PORT_LTP_CRC_MODE_16;

	if (cap & HFI_CAP_CRC_14B)
		port_ltp |= OPA_PORT_LTP_CRC_MODE_14;
	if (cap & HFI_CAP_CRC_16B)
		port_ltp |= OPA_PORT_LTP_CRC_MODE_16;
	if (cap & HFI_CAP_CRC_48B)
		port_ltp |= OPA_PORT_LTP_CRC_MODE_48;
	if (cap & HFI_CAP_CRC_12B_16B_PER_LANE)
		port_ltp |= OPA_PORT_LTP_CRC_MODE_PER_LANE;

	return port_ltp;
}

/* Convert an OPA Port LTP mask to capability mask */
u16 hfi_port_ltp_to_cap(u16 port_ltp)
{
	u16 cap_mask = 0;

	if (port_ltp & OPA_PORT_LTP_CRC_MODE_14)
		cap_mask |= HFI_CAP_CRC_14B;
	if (port_ltp & OPA_PORT_LTP_CRC_MODE_16)
		cap_mask |= HFI_CAP_CRC_16B;
	if (port_ltp & OPA_PORT_LTP_CRC_MODE_48)
		cap_mask |= HFI_CAP_CRC_48B;
	if (port_ltp & OPA_PORT_LTP_CRC_MODE_PER_LANE)
		cap_mask |= HFI_CAP_CRC_12B_16B_PER_LANE;

	return cap_mask;
}

/*
 * Convert a single FC LCB CRC mode to an OPA Port LTP mask.
 */
static u16 hfi_lcb_to_port_ltp(struct hfi_pportdata *ppd, u16 lcb_crc)
{
	u16 port_ltp = 0;

	switch (lcb_crc) {
	case HFI_LCB_CRC_12B_16B_PER_LANE:
		port_ltp = OPA_PORT_LTP_CRC_MODE_PER_LANE;
		break;
	case HFI_LCB_CRC_48B:
		port_ltp = OPA_PORT_LTP_CRC_MODE_48;
		break;
	case HFI_LCB_CRC_14B:
		port_ltp = OPA_PORT_LTP_CRC_MODE_14;
		break;
	case HFI_LCB_CRC_16B:
		port_ltp = OPA_PORT_LTP_CRC_MODE_16;
		break;
	default:
		ppd_dev_warn(ppd, "lcb crc 0x%x. Driver might be in bad state",
			     lcb_crc);
	}

	return port_ltp;
}

/* return the link state name */
static const char *link_state_name(u32 state)
{
	const char *name;
	int n = ilog2(state);
	static const char * const names[] = {
		[__HLS_UP_INIT_BP]	 = "INIT",
		[__HLS_UP_ARMED_BP]	 = "ARMED",
		[__HLS_UP_ACTIVE_BP]	 = "ACTIVE",
		[__HLS_DN_DOWNDEF_BP]	 = "DOWNDEF",
		[__HLS_DN_POLL_BP]	 = "POLL",
		[__HLS_DN_DISABLE_BP]	 = "DISABLE",
		[__HLS_DN_OFFLINE_BP]	 = "OFFLINE",
		[__HLS_VERIFY_CAP_BP]	 = "VERIFY_CAP",
		[__HLS_GOING_UP_BP]	 = "GOING_UP",
		[__HLS_GOING_OFFLINE_BP] = "GOING_OFFLINE",
		[__HLS_LINK_COOLDOWN_BP] = "LINK_COOLDOWN"
	};

	name = n < ARRAY_SIZE(names) ? names[n] : NULL;
	return name ? name : "unknown";
}

/* return the OPA port physical state name */
static const char *opa_pstate_name(u32 pstate)
{
	static const char * const port_physical_names[] = {
		"PHYS_NOP",
		"reserved1",
		"PHYS_POLL",
		"PHYS_DISABLED",
		"PHYS_TRAINING",
		"PHYS_LINKUP",
		"PHYS_LINK_ERR_RECOVER",
		"PHYS_PHY_TEST",
		"reserved8",
		"PHYS_OFFLINE",
		"PHYS_GANGED",
		"PHYS_TEST",
	};
	if (pstate < ARRAY_SIZE(port_physical_names))
		return port_physical_names[pstate];
	return "unknown";
}

/* return the link state reason name */
static const char *link_state_reason_name(struct hfi_pportdata *ppd, u32 state)
{
	if (state == HLS_UP_INIT) {
		switch (ppd->linkinit_reason) {
		case OPA_LINKINIT_REASON_LINKUP:
			return "(LINKUP)";
		case OPA_LINKINIT_REASON_FLAPPING:
			return "(FLAPPING)";
		case OPA_LINKINIT_OUTSIDE_POLICY:
			return "(OUTSIDE_POLICY)";
		case OPA_LINKINIT_QUARANTINED:
			return "(QUARANTINED)";
		case OPA_LINKINIT_INSUFIC_CAPABILITY:
			return "(INSUFIC_CAPABILITY)";
		default:
			break;
		}
	}
	return "";
}

static const char * const link_down_reason_strs[] = {
	[OPA_LINKDOWN_REASON_NONE] = "None",
	[OPA_LINKDOWN_REASON_RCV_ERROR_0] = "Recive error 0",
	[OPA_LINKDOWN_REASON_BAD_PKT_LEN] = "Bad packet length",
	[OPA_LINKDOWN_REASON_PKT_TOO_LONG] = "Packet too long",
	[OPA_LINKDOWN_REASON_PKT_TOO_SHORT] = "Packet too short",
	[OPA_LINKDOWN_REASON_BAD_SLID] = "Bad SLID",
	[OPA_LINKDOWN_REASON_BAD_DLID] = "Bad DLID",
	[OPA_LINKDOWN_REASON_BAD_L2] = "Bad L2",
	[OPA_LINKDOWN_REASON_BAD_SC] = "Bad SC",
	[OPA_LINKDOWN_REASON_RCV_ERROR_8] = "Receive error 8",
	[OPA_LINKDOWN_REASON_BAD_MID_TAIL] = "Bad mid tail",
	[OPA_LINKDOWN_REASON_RCV_ERROR_10] = "Receive error 10",
	[OPA_LINKDOWN_REASON_PREEMPT_ERROR] = "Preempt error",
	[OPA_LINKDOWN_REASON_PREEMPT_VL15] = "Preempt vl15",
	[OPA_LINKDOWN_REASON_BAD_VL_MARKER] = "Bad VL marker",
	[OPA_LINKDOWN_REASON_RCV_ERROR_14] = "Receive error 14",
	[OPA_LINKDOWN_REASON_RCV_ERROR_15] = "Receive error 15",
	[OPA_LINKDOWN_REASON_BAD_HEAD_DIST] = "Bad head distance",
	[OPA_LINKDOWN_REASON_BAD_TAIL_DIST] = "Bad tail distance",
	[OPA_LINKDOWN_REASON_BAD_CTRL_DIST] = "Bad control distance",
	[OPA_LINKDOWN_REASON_BAD_CREDIT_ACK] = "Bad credit ack",
	[OPA_LINKDOWN_REASON_UNSUPPORTED_VL_MARKER] = "Unsupported VL marker",
	[OPA_LINKDOWN_REASON_BAD_PREEMPT] = "Bad preempt",
	[OPA_LINKDOWN_REASON_BAD_CONTROL_FLIT] = "Bad control flit",
	[OPA_LINKDOWN_REASON_EXCEED_MULTICAST_LIMIT] = "Exceed multicast limit",
	[OPA_LINKDOWN_REASON_RCV_ERROR_24] = "Receive error 24",
	[OPA_LINKDOWN_REASON_RCV_ERROR_25] = "Receive error 25",
	[OPA_LINKDOWN_REASON_RCV_ERROR_26] = "Receive error 26",
	[OPA_LINKDOWN_REASON_RCV_ERROR_27] = "Receive error 27",
	[OPA_LINKDOWN_REASON_RCV_ERROR_28] = "Receive error 28",
	[OPA_LINKDOWN_REASON_RCV_ERROR_29] = "Receive error 29",
	[OPA_LINKDOWN_REASON_RCV_ERROR_30] = "Receive error 30",
	[OPA_LINKDOWN_REASON_EXCESSIVE_BUFFER_OVERRUN] =
					"Excessive buffer overrun",
	[OPA_LINKDOWN_REASON_UNKNOWN] = "Unknown",
	[OPA_LINKDOWN_REASON_REBOOT] = "Reboot",
	[OPA_LINKDOWN_REASON_NEIGHBOR_UNKNOWN] = "Neighbor unknown",
	[OPA_LINKDOWN_REASON_FM_BOUNCE] = "FM bounce",
	[OPA_LINKDOWN_REASON_SPEED_POLICY] = "Speed policy",
	[OPA_LINKDOWN_REASON_WIDTH_POLICY] = "Width policy",
	[OPA_LINKDOWN_REASON_DISCONNECTED] = "Disconnected",
	[OPA_LINKDOWN_REASON_LOCAL_MEDIA_NOT_INSTALLED] =
					"Local media not installed",
	[OPA_LINKDOWN_REASON_NOT_INSTALLED] = "Not installed",
	[OPA_LINKDOWN_REASON_CHASSIS_CONFIG] = "Chassis config",
	[OPA_LINKDOWN_REASON_END_TO_END_NOT_INSTALLED] =
					"End to end not installed",
	[OPA_LINKDOWN_REASON_POWER_POLICY] = "Power policy",
	[OPA_LINKDOWN_REASON_LINKSPEED_POLICY] = "Link speed policy",
	[OPA_LINKDOWN_REASON_LINKWIDTH_POLICY] = "Link width policy",
	[OPA_LINKDOWN_REASON_SWITCH_MGMT] = "Switch management",
	[OPA_LINKDOWN_REASON_SMA_DISABLED] = "SMA disabled",
	[OPA_LINKDOWN_REASON_TRANSIENT] = "Transient"
};

static void set_mgmt_allowed(struct hfi_pportdata *ppd)
{
	u32 frame;

	if (ppd->neighbor_type == NEIGHBOR_TYPE_HFI) {
		ppd->mgmt_allowed = 1;
	} else {
		hfi2_read_8051_config(ppd, REMOTE_LNI_INFO, GENERAL_CONFIG,
				      &frame);
		ppd->mgmt_allowed = (frame >> MGMT_ALLOWED_SHIFT)
				    & MGMT_ALLOWED_MASK;
	}
}

/* return the neighbor link down reason string */
static const char *link_down_reason_str(u8 reason)
{
	const char *str = "(invalid)";

	if (reason < ARRAY_SIZE(link_down_reason_strs))
		str = link_down_reason_strs[reason];

	return str;
}

static u32 hfi_to_opa_pstate(struct hfi_devdata *dd, u32 hfi_pstate)
{
	/* look at the HFI meta-states only */
	switch (hfi_pstate & 0xf0) {
	case PLS_DISABLED:
		return IB_PORTPHYSSTATE_DISABLED;
	case PLS_OFFLINE:
		return OPA_PORTPHYSSTATE_OFFLINE;
	case PLS_POLLING:
		return IB_PORTPHYSSTATE_POLLING;
	case PLS_CONFIGPHY:
		return IB_PORTPHYSSTATE_TRAINING;
	case PLS_LINKUP:
		return IB_PORTPHYSSTATE_LINKUP;
	case PLS_PHYTEST:
		return IB_PORTPHYSSTATE_PHY_TEST;
	default:
		dd_dev_err(dd, "Unexpected chip physical state of 0x%x\n",
			   hfi_pstate);
		return IB_PORTPHYSSTATE_DISABLED;
	}
}

static void log_state_transition(struct hfi_pportdata *ppd, u32 state)
{
	u32 ib_pstate = hfi_to_opa_pstate(ppd->dd, state);

	dd_dev_info(ppd->dd,
		    "physical state changed to %s (0x%x), phy 0x%x\n",
		    opa_pstate_name(ib_pstate), ib_pstate, state);
}

/*
 * Read the physical hardware link state and check if it matches host
 * drivers anticipated state.
 */
static void log_physical_state(struct hfi_pportdata *ppd, u32 state)
{
	u32 read_state = read_physical_state(ppd);

	if (read_state == state) {
		log_state_transition(ppd, state);
	} else {
		dd_dev_err(ppd->dd,
			   "anticipated phy link state 0x%x, read 0x%x\n",
			   state, read_state);
	}
}

/*
 * wait_physical_linkstate - wait for an physical link state change to occur
 * @ppd: port device
 * @state: the state to wait for
 * @msecs: the number of milliseconds to wait
 * Wait up to msecs milliseconds for physical link state change to occur.
 * Returns 0 if state reached, otherwise -ETIMEDOUT.
 */
static int wait_physical_linkstate(struct hfi_pportdata *ppd, u32 state,
				   int msecs)
{
	u32 read_state;
	unsigned long timeout;

	timeout = jiffies + msecs_to_jiffies(msecs);
	while (1) {
		read_state = read_physical_state(ppd);
		if (read_state == state)
			break;
		if (time_after(jiffies, timeout)) {
			dd_dev_err(ppd->dd,
				   "timeout waiting for phy link state 0x%x\n",
				   state);
			return -ETIMEDOUT;
		}
		usleep_range(1950, 2050); /* sleep 2ms-ish */
	}

	log_state_transition(ppd, state);
	return 0;
}

static void read_planned_down_reason_code(struct hfi_pportdata *ppd, u8 *pdrrc)
{
	u32 frame;

	hfi2_read_8051_config(ppd, LINK_QUALITY_INFO, GENERAL_CONFIG, &frame);
	*pdrrc = (frame >> DOWN_REMOTE_REASON_SHIFT) & DOWN_REMOTE_REASON_MASK;
}

static void read_link_down_reason(struct hfi_pportdata *ppd, u8 *ldr)
{
	u32 frame;

	hfi2_read_8051_config(ppd, LINK_DOWN_REASON, GENERAL_CONFIG, &frame);
	*ldr = (frame & 0xff);
}

/*
 * hfi_driver_pstate - convert the driver's notion of a port's
 * state (an HLS_*) into a physical state (a {IB,OPA}_PORTPHYSSTATE_*).
 * Return -1 (converted to a u32) to indicate error.
 */
u32 hfi_driver_pstate(struct hfi_pportdata *ppd)
{
	switch (ppd->host_link_state) {
	case HLS_UP_INIT:
	case HLS_UP_ARMED:
	case HLS_UP_ACTIVE:
		return IB_PORTPHYSSTATE_LINKUP;
	case HLS_DN_POLL:
		return IB_PORTPHYSSTATE_POLLING;
	case HLS_DN_DISABLE:
		return IB_PORTPHYSSTATE_DISABLED;
	case HLS_DN_OFFLINE:
		return OPA_PORTPHYSSTATE_OFFLINE;
	case HLS_VERIFY_CAP:
		return IB_PORTPHYSSTATE_POLLING;
	case HLS_GOING_UP:
		return IB_PORTPHYSSTATE_POLLING;
	case HLS_GOING_OFFLINE:
		return OPA_PORTPHYSSTATE_OFFLINE;
	case HLS_LINK_COOLDOWN:
		return OPA_PORTPHYSSTATE_OFFLINE;
	case HLS_DN_DOWNDEF:
	default:
		dd_dev_err(ppd->dd, "invalid host_link_state 0x%x\n",
			   ppd->host_link_state);
		return  -1;
	}
}

/*
 * hfi_driver_lstate - convert the driver's notion of a port's
 * state (an HLS_*) into a logical state (a IB_PORT_*). Return -1
 * (converted to a u32) to indicate error.
 */
u32 hfi_driver_lstate(struct hfi_pportdata *ppd)
{
	if (ppd->host_link_state && (ppd->host_link_state & HLS_DOWN))
		return IB_PORT_DOWN;

	switch (ppd->host_link_state & HLS_UP) {
	case HLS_UP_INIT:
		return IB_PORT_INIT;
	case HLS_UP_ARMED:
		return IB_PORT_ARMED;
	case HLS_UP_ACTIVE:
		return IB_PORT_ACTIVE;
	default:
		dd_dev_err(ppd->dd, "invalid host_link_state 0x%x\n",
			   ppd->host_link_state);
	return -1;
	}
}

void set_link_down_reason(struct hfi_pportdata *ppd, u8 lcl_reason,
			  u8 neigh_reason, u8 rem_reason)
{
	if (ppd->local_link_down_reason.latest == 0 &&
	    ppd->neigh_link_down_reason.latest == 0) {
		ppd->local_link_down_reason.latest = lcl_reason;
		ppd->neigh_link_down_reason.latest = neigh_reason;
		ppd->remote_link_down_reason = rem_reason;
	}
}

/*
 * Several pieces of LNI information were cached for SMA in ppd.
 * Reset these on link down
 */
static void reset_neighbor_info(struct hfi_pportdata *ppd)
{
	ppd->neighbor_guid = 0;
	ppd->neighbor_port_number = 0;
	ppd->neighbor_type = 0;
	ppd->neighbor_fm_security = 0;
}

/*
 * Graceful LCB shutdown.  This leaves the LCB FIFOs in reset.
 */
static void lcb_shutdown(struct hfi_pportdata *ppd, int abort)
{
	u64 lcb_reg, fpc_reg;

	/* clear lcb run: OC_LCB_CFG_RUN.EN = 0 */
	write_csr(ppd->dd, OC_LCB_CFG_RUN, 0);
	/* set tx fifo reset: OC_LCB_CFG_TX_FIFOS_RESET.VAL = 1 */
	write_csr(ppd->dd, OC_LCB_CFG_TX_FIFOS_RESET,
		      1ull << OC_LCB_CFG_TX_FIFOS_RESET_VAL_SHIFT);
	/* set fzc reset csr: OC_LCB_CFG_RESET_FROM_CSR.{reset_lcb} = 1 */
	ppd->lcb_err_en = read_csr(ppd->dd, OC_LCB_ERR_EN_HOST);
	lcb_reg = read_csr(ppd->dd, OC_LCB_CFG_RESET_FROM_CSR);
	write_csr(ppd->dd, OC_LCB_CFG_RESET_FROM_CSR, lcb_reg |
		  (1ull << OC_LCB_CFG_RESET_FROM_CSR_EN_SHIFT));
	/* make sure the write completed */
	(void)read_csr(ppd->dd, OC_LCB_CFG_RESET_FROM_CSR);

	/* Do the same again for the FPC (Fabric Protocol Block) */
	fpc_reg = read_csr(ppd->dd, FXR_FPC_CFG_RESET);
	write_csr(ppd->dd, FXR_FPC_CFG_RESET, fpc_reg |
		  (1ull << FXR_FPC_CFG_RESET_BLK_RESET_SHIFT));
	/* make sure the write completed */
	(void)read_csr(ppd->dd, FXR_FPC_CFG_RESET);
	if (!abort) {
		udelay(1);    /* must hold for the longer of 16cclks or 20ns */
		write_csr(ppd->dd, OC_LCB_CFG_RESET_FROM_CSR, lcb_reg);
		write_csr(ppd->dd, FXR_FPC_CFG_RESET, fpc_reg);
		write_csr(ppd->dd, OC_LCB_ERR_EN_HOST, ppd->lcb_err_en);
	}
}

/*
 * This routine should be called after the link has been transitioned to
 * OFFLINE (OFFLINE state has the side effect of putting the SerDes into
 * reset).
 *
 * The expectation is that the caller of this routine would have taken
 * care of properly transitioning the link into the correct state.
 *
 * NOTE: the caller needs to acquire ppd->crk8051_mutex lock before
 * calling this function
 */
static void _oc_shutdown(struct hfi_pportdata *ppd)
{
	lockdep_assert_held(&ppd->crk8051_mutex);
	if (ppd->oc_shutdown)
		return;

	ppd->oc_shutdown = 1;
	/* Shutdown the LCB */
	lcb_shutdown(ppd, 1);

	/*
	 * Going to OFFLINE would have causes the 8051 to put the
	 * SerDes into reset already. Just need to shut down the 8051,
	 * itself.
	 */
	write_csr(ppd->dd, CRK_CRK8051_CFG_RST, 0x1);
}

static void oc_shutdown(struct hfi_pportdata *ppd)
{
	mutex_lock(&ppd->crk8051_mutex);
	_oc_shutdown(ppd);
	mutex_unlock(&ppd->crk8051_mutex);
}

/*
 * Calling this after the MNH has been brought out of reset.
 * NOTE: the caller needs to acquire the ppd->crk8051_mutex before
 * calling this function
 */
void _oc_start(struct hfi_pportdata *ppd)
{
	if (no_mnh)
		return;

	lockdep_assert_held(&ppd->crk8051_mutex);
	if (!ppd->oc_shutdown)
		return;
	/* Take the 8051 out of reset, wait until 8051 is ready and set
	 * host version bit*/
	hfi2_release_and_wait_ready_8051_firmware(ppd);
	/* turn on the LCB (turn off in lcb_shutdown). */
	write_csr(ppd->dd, OC_LCB_CFG_RUN, OC_LCB_CFG_RUN_EN_MASK);
	/* lcb_shutdown() with abort=1 does not restore these */
	write_csr(ppd->dd, OC_LCB_ERR_EN_HOST, ppd->lcb_err_en);
	ppd->oc_shutdown = 0;
}

void oc_start(struct hfi_pportdata *ppd)
{
	mutex_lock(&ppd->crk8051_mutex);
	_oc_start(ppd);
	mutex_unlock(&ppd->crk8051_mutex);
}

static void handle_link_bounce(struct work_struct *work)
{
	struct hfi_pportdata *ppd = container_of(work, struct hfi_pportdata,
						  link_bounce_work);

	ppd_dev_info(ppd, "bouncing link\n");
	hfi_set_link_state(ppd, HLS_DN_OFFLINE);
	hfi_start_link(ppd);
}

/*
 * Handle a link up interrupt from the MNH/8051.
 *
 * This is a work-queue function outside of the interrupt.
 */
static void handle_link_up(struct work_struct *work)
{
	struct hfi_pportdata *ppd = container_of(work, struct hfi_pportdata,
								link_up_work);

	mutex_lock(&ppd->hls_lock);
	/* transit to Init only from Going_Up. */
	if (ppd->host_link_state != HLS_GOING_UP) {
		u32 reg_8051_port = read_physical_state(ppd);
		if (!ppd->dd->emulation)
			ppd_dev_info(ppd, "False interrupt on %s(): %s(%d) 0x%x",
				     __func__,
				     link_state_name(ppd->host_link_state),
				     ilog2(ppd->host_link_state),
				     reg_8051_port);
		mutex_unlock(&ppd->hls_lock);
		return;
	}
	mutex_unlock(&ppd->hls_lock);
	hfi_set_link_state(ppd, HLS_UP_INIT);

	/* cache the read of OC_LCB_STS_ROUND_TRIP_LTP_CNT */
	ppd->lcb_sts_rtt = read_csr(ppd->dd, OC_LCB_STS_ROUND_TRIP_LTP_CNT);

#if 0 /* WFR legacy */
	/*
	 * OPA specifies that certain counters are cleared on a transition
	 * to link up, so do that.
	 */
	clear_linkup_counters(ppd->dd);
	/*
	 * And (re)set link up default values.
	 */
	set_linkup_defaults(ppd);

	/* enforce link speed enabled */
	if ((ppd->link_speed_active & ppd->link_speed_enabled) == 0) {
		/* oops - current speed is not enabled, bounce */
		ppd_dev_err(ppd,
			    "Link speed active 0x%x is outside enabled 0x%x, downing link\n",
			    ppd->link_speed_active, ppd->link_speed_enabled);
		set_link_down_reason(ppd, OPA_LINKDOWN_REASON_SPEED_POLICY, 0,
				     OPA_LINKDOWN_REASON_SPEED_POLICY);
		set_link_state(ppd, HLS_DN_OFFLINE);
		start_link(ppd);
	}
#endif
}

/*
 * Handle a link down interrupt from the MNH/8051 which transit
 * READY_TO_QUIET_LT -> QUIET.
 *
 * This is a work-queue function outside of the interrupt.
 */
static void handle_link_down(struct work_struct *work)
{
	struct hfi_pportdata *ppd = container_of(work, struct hfi_pportdata,
			link_down_work);
	u8 lcl_reason, link_down_reason,  neigh_reason = 0;
	int was_up;
	static const char ldr_str[] = "Link down reason: ";

	if ((ppd->host_link_state &
	    (HLS_DN_POLL | HLS_VERIFY_CAP | HLS_GOING_UP)) &&
	     ppd->port_type == PORT_TYPE_FIXED)
		ppd->offline_disabled_reason =
			HFI_ODR_MASK(OPA_LINKDOWN_REASON_NOT_INSTALLED);

	/* Go offline first, then deal with reading/writing through 8051 */
	was_up = !!(ppd->host_link_state & HLS_UP);
	hfi_set_link_state(ppd, HLS_DN_OFFLINE);

	if (was_up) {
		lcl_reason = 0;
		/* link down reason is only valid if link was up */
		read_link_down_reason(ppd, &link_down_reason);
		switch (link_down_reason) {
		case LDR_LINK_TRANSFER_ACTIVE_LOW:
			dd_dev_info(ppd->dd, "%sUnexpected link down\n",
				    ldr_str);
			break;
		case LDR_RECEIVED_LINKDOWN_IDLE_MSG:
			/*
			 * The neighbor reason is only valid if an idle
			 * message was received for it.
			 */
			read_planned_down_reason_code(ppd, &neigh_reason);
			dd_dev_info(ppd->dd,
				    "%sNeighbor link down message %d, %s\n",
				    ldr_str, neigh_reason,
				    link_down_reason_str(neigh_reason));
			break;
		case LDR_RECEIVED_HOST_OFFLINE_REQ:
			dd_dev_info(ppd->dd,
				    "%sHost requested link to go offline\n",
				    ldr_str);
			break;
		default:
			dd_dev_info(ppd->dd, "%sUnknown reason 0x%x\n",
				    ldr_str, link_down_reason);
			break;
		}

		/*
		 * If no reason, assume peer-initiated but missed
		 * LinkGoingDown idle flits.
		 */
		if (neigh_reason == 0)
			lcl_reason = OPA_LINKDOWN_REASON_NEIGHBOR_UNKNOWN;
	} else {
		/* went down while polling or going up */
		lcl_reason = OPA_LINKDOWN_REASON_TRANSIENT;
	}

	set_link_down_reason(ppd, lcl_reason, neigh_reason, 0);

	/* inform the SMA when the link transitions from up to down */
	if (was_up && ppd->local_link_down_reason.sma == 0 &&
	    ppd->neigh_link_down_reason.sma == 0) {
		ppd->local_link_down_reason.sma =
			ppd->local_link_down_reason.latest;
		ppd->neigh_link_down_reason.sma =
			ppd->neigh_link_down_reason.latest;
	}

	reset_neighbor_info(ppd);

	/*
	 * If there is no cable attached, turn the DC off. Otherwise,
	 * start the link bring up.
	 */
	if (!hfi_qsfp_mod_present(ppd))
		oc_shutdown(ppd);
	else
		hfi_start_link(ppd);
}

static void set_logical_state(struct hfi_pportdata *ppd, u32 chip_lstate)
{
	u64 reg;

	reg = read_csr(ppd->dd, OC_LCB_CFG_PORT);
	/* clear current state, set new state */
	reg &= ~OC_LCB_CFG_PORT_LINK_STATE_SMASK;
	reg |= (u64)chip_lstate << OC_LCB_CFG_PORT_LINK_STATE_SHIFT;
	write_csr(ppd->dd, OC_LCB_CFG_PORT, reg);
}

/*
 * If the 8051 is in reset mode (ppd->oc_shutdown == 1), this function
 * will still continue executing
 *
 * Returns:
 *	< 0 = Linux error, not able to get access
 *	> 0 = 8051 command RETURN_CODE
 */
static int _do_8051_command(struct hfi_pportdata *ppd, u32 type,
			   u64 in_data, u64 *out_data)
{
	u64 reg;
	int return_code = 0;
	unsigned long timeout;

	lockdep_assert_held(&ppd->crk8051_mutex);
	/*
	 * If an 8051 host command timed out previously, then the 8051 is
	 * stuck.
	 *
	 * On first timeout, attempt to reset and restart the entire DC
	 * block (including 8051). (Is this too big of a hammer?)
	 *
	 * If the 8051 times out a second time, the reset did not bring it
	 * back to healthy life. In that case, fail any subsequent commands.
	 */
	if (ppd->crk8051_timed_out) {
		if (ppd->crk8051_timed_out > 1) {
			ppd_dev_err(ppd,
				    "Previous 8051 host command timed out, skipping command %u\n",
				    type);
			return_code = -ENXIO;
			goto fail;
		}
		_oc_shutdown(ppd);
		_oc_start(ppd);
	}

	/*
	 * If there is no timeout, then the 8051 command interface is
	 * waiting for a command.
	 */

	/*
	 * Do two writes: the first to stabilize the type and req_data, the
	 * second to activate.
	 */
	reg = ((u64)type & CRK_CRK8051_CFG_HOST_CMD_0_REQ_TYPE_MASK)
			<< CRK_CRK8051_CFG_HOST_CMD_0_REQ_TYPE_SHIFT
		| (in_data & CRK_CRK8051_CFG_HOST_CMD_0_REQ_DATA_MASK)
			<< CRK_CRK8051_CFG_HOST_CMD_0_REQ_DATA_SHIFT;
	write_csr(ppd->dd, CRK_CRK8051_CFG_HOST_CMD_0, reg);
	reg |= CRK_CRK8051_CFG_HOST_CMD_0_REQ_NEW_SMASK;
	write_csr(ppd->dd, CRK_CRK8051_CFG_HOST_CMD_0, reg);

	/* wait for completion, alternate: interrupt */
	timeout = jiffies + msecs_to_jiffies(CRK8051_COMMAND_TIMEOUT);
	while (1) {
		reg = read_csr(ppd->dd, CRK_CRK8051_CFG_HOST_CMD_1);
		if (reg & CRK_CRK8051_CFG_HOST_CMD_1_COMPLETED_SMASK)
			break;
		if (time_after(jiffies, timeout)) {
			ppd->crk8051_timed_out++;
			ppd_dev_err(ppd, "8051 host command %u timeout\n",
				    type);
			if (out_data)
				*out_data = 0;
			return_code = -ETIMEDOUT;
			goto fail;
		}
		udelay(2);
	}

	if (out_data) {
		*out_data = (reg >> CRK_CRK8051_CFG_HOST_CMD_1_RSP_DATA_SHIFT)
				& CRK_CRK8051_CFG_HOST_CMD_1_RSP_DATA_MASK;
		if (type == HCMD_READ_LCB_CSR) {
			/* top 16 bits are in a different register */
			*out_data |=
			  (read_csr(ppd->dd, CRK_CRK8051_CFG_EXT_DEV_1) &
			  CRK_CRK8051_CFG_EXT_DEV_1_REQ_DATA_SMASK) <<
			  (48 - CRK_CRK8051_CFG_EXT_DEV_1_REQ_DATA_SHIFT);
		}
	}
	return_code = (reg >> CRK_CRK8051_CFG_HOST_CMD_1_RETURN_CODE_SHIFT)
				& CRK_CRK8051_CFG_HOST_CMD_1_RETURN_CODE_MASK;
	ppd->crk8051_timed_out = 0;

	/*
	 * Clear command for next user.
	 */
	write_csr(ppd->dd, CRK_CRK8051_CFG_HOST_CMD_0, 0);

fail:
	hfi2_cdbg(8051, "type %d, data in 0x%012llx, out 0x%012llx, ret = %d",
		  type, in_data, out_data ? *out_data : 0, return_code);

	return return_code;
}

/*
 * Returns:
 * 	< 0 = Linux error, not able to get access
 * 	> 0 = 8051 command RETURN_CODE
 */
static int do_8051_command(struct hfi_pportdata *ppd, u32 type, u64 in_data,
			   u64 *out_data)
{
	int return_code;

	mutex_lock(&ppd->crk8051_mutex);
	/* We can't send any commands to the 8051 if it is in reset */
	if (ppd->oc_shutdown)
		return_code = -ENODEV;
	else
		return_code = _do_8051_command(ppd, type, in_data, out_data);
	mutex_unlock(&ppd->crk8051_mutex);
	return return_code;
}

static int set_physical_link_state(struct hfi_pportdata *ppd, u64 state)
{
	return do_8051_command(ppd, HCMD_CHANGE_PHY_STATE, state, NULL);
}

int _load_8051_config(struct hfi_pportdata *ppd, u8 field_id,
		     u8 lane_id, u32 config_data)
{
	u64 data;
	int ret;

	lockdep_assert_held(&ppd->crk8051_mutex);
	data = (u64)field_id << LOAD_DATA_FIELD_ID_SHIFT
		| (u64)lane_id << LOAD_DATA_LANE_ID_SHIFT
		| (u64)config_data << LOAD_DATA_DATA_SHIFT;
	ret = _do_8051_command(ppd, HCMD_LOAD_CONFIG_DATA, data, NULL);
	if (ret != HCMD_SUCCESS) {
		ppd_dev_err(ppd,
			    "load 8051 config: field id %d, lane %d, err %d\n",
			    (int)field_id, (int)lane_id, ret);
	}
	return ret;
}

int load_8051_config(struct hfi_pportdata *ppd, u8 field_id,
		     u8 lane_id, u32 config_data)
{
	int return_code;

	mutex_lock(&ppd->crk8051_mutex);
	return_code = _load_8051_config(ppd, field_id, lane_id, config_data);
	mutex_unlock(&ppd->crk8051_mutex);

	return return_code;
}

static void read_vc_local_link_width(struct hfi_pportdata *ppd, u8 *misc_bits,
				     u8 *flag_bits, u16 *link_widths)
{
	u32 frame;

	hfi2_read_8051_config(ppd, VERIFY_CAP_LOCAL_LINK_WIDTH, GENERAL_CONFIG,
			      &frame);
	*misc_bits = (frame >> MISC_CONFIG_BITS_SHIFT) & MISC_CONFIG_BITS_MASK;
	*flag_bits = (frame >> LOCAL_FLAG_BITS_SHIFT) & LOCAL_FLAG_BITS_MASK;
	*link_widths = (frame >> LINK_WIDTH_SHIFT) & LOCAL_LINK_WIDTH_MASK;
}

static int write_vc_local_link_width(struct hfi_pportdata *ppd,
				     u8 misc_bits,
				     u8 flag_bits,
				     u16 link_widths)
{
	u32 frame;

	frame = (u32)misc_bits << MISC_CONFIG_BITS_SHIFT
		| (u32)flag_bits << LOCAL_FLAG_BITS_SHIFT
		| (u32)link_widths << LINK_WIDTH_SHIFT;
	return load_8051_config(ppd, VERIFY_CAP_LOCAL_LINK_WIDTH,
				GENERAL_CONFIG, frame);
}

static void read_vc_remote_link_width(struct hfi_pportdata *ppd,
				      u8 *remote_tx_rate,
				      u16 *link_widths)
{
	u32 frame;

	hfi2_read_8051_config(ppd, VERIFY_CAP_REMOTE_LINK_WIDTH,
			      GENERAL_CONFIG, &frame);
	*remote_tx_rate = (frame >> REMOTE_TX_RATE_SHIFT)
				& REMOTE_TX_RATE_MASK;
	*link_widths = (frame >> LINK_WIDTH_SHIFT) & REMOTE_LINK_WIDTH_MASK;
}

/*
 * Read an idle LCB message.
 *
 * Returns 0 on success, -EINVAL on error
 */
static int read_idle_message(struct hfi_pportdata *ppd, u64 type, u64 *data_out)
{
	int ret;

	ret = do_8051_command(ppd, HCMD_READ_LCB_IDLE_MSG,
			      type, data_out);
	if (ret != HCMD_SUCCESS) {
		ppd_dev_err(ppd, "read idle message: type %d, err %d\n",
			    (u32)type, ret);
		return -EINVAL;
	}
	ppd_dev_info(ppd, "%s: read idle message 0x%llx\n", __func__,
		     *data_out);
	/* return only the payload as we already know the type */
	*data_out >>= IDLE_PAYLOAD_SHIFT;

	return 0;
}

/*
 * Read an idle SMA message.  To be done in response to a notification from
 * the 8051.
 *
 * Returns 0 on success, -EINVAL on error
 */
static int read_idle_sma(struct hfi_pportdata *ppd, u64 *data)
{
	return read_idle_message(ppd,
			(u64)IDLE_SMA << IDLE_MSG_TYPE_SHIFT, data);
}

/*
 * Send an idle LCB message.
 *
 * Returns 0 on success, -EINVAL on error
 */
static int send_idle_message(struct hfi_pportdata *ppd, u64 data)
{
	int ret;
	unsigned long timeout = jiffies + msecs_to_jiffies(MNH_QUEUE_TIMEOUT);

	ppd_dev_info(ppd, "%s: sending idle message 0x%llx\n", __func__, data);

	do {
		ret = do_8051_command(ppd, HCMD_SEND_LCB_IDLE_MSG, data, NULL);

		if (ret == HCMD_SUCCESS)
			break;

		if (ret == HCMD_FLOWCONTROL) {
			ppd_dev_dbg(ppd, "resend idle due to queue full\n");
			usleep_range(1950, 2050); /* sleep 2ms-ish */
		} else {
			ppd_dev_err(ppd,
				    "send idle message error %d: data 0x%llx\n",
				    ret, data);
			return -EINVAL;
		}

	} while (ret == HCMD_FLOWCONTROL && !time_after(jiffies, timeout));

	if (time_after(jiffies, timeout)) {
		ppd_dev_err(ppd, "timeout waiting for link queue\n");
		return -EBUSY;
	}

	return 0;
}

/*
 * Send an idle SMA message.
 *
 * Returns 0 on success, -EINVAL on error
 */
int hfi_send_idle_sma(struct hfi_pportdata *ppd, u64 message)
{
	u64 data;

	if (no_mnh)
		return 0;

	ppd_dev_info(ppd, "%s: sending idle sma 0x%llx\n", __func__, message);
	data = ((message & IDLE_PAYLOAD_MASK) << IDLE_PAYLOAD_SHIFT)
		| ((u64)IDLE_SMA << IDLE_MSG_TYPE_SHIFT);
	return send_idle_message(ppd, data);
}

/*
 * Initialize the LCB then do a quick link up.  This may or may not be
 * in loopback.
 *
 * return 0 on success, -errno on error
 */
static int do_quick_linkup(struct hfi_pportdata *ppd)
{
	int ret;
#if 0 /* WFR legacy */
	struct hfi_devdata *dd = ppd->dd;
	u64 reg;
	unsigned long timeout;

	lcb_shutdown(ppd, 0);

	if (loopback) {
		/* LCB_CFG_LOOPBACK.VAL = 2 */
		/* LCB_CFG_LANE_WIDTH.VAL = 0 */
		write_csr(dd, DC_LCB_CFG_LOOPBACK,
			  IB_PACKET_TYPE << DC_LCB_CFG_LOOPBACK_VAL_SHIFT);
		write_csr(dd, DC_LCB_CFG_LANE_WIDTH, 0);
	}

	/* start the LCBs */
	/* LCB_CFG_TX_FIFOS_RESET.VAL = 0 */
	write_csr(dd, DC_LCB_CFG_TX_FIFOS_RESET, 0);

	/* simulator only loopback steps */
	if (loopback && dd->icode == ICODE_FUNCTIONAL_SIMULATOR) {
		/* LCB_CFG_RUN.EN = 1 */
		write_csr(dd, DC_LCB_CFG_RUN,
			  1ull << DC_LCB_CFG_RUN_EN_SHIFT);

		/* watch LCB_STS_LINK_TRANSFER_ACTIVE */
		timeout = jiffies + msecs_to_jiffies(10);
		while (1) {
			reg = read_csr(dd,
				       DC_LCB_STS_LINK_TRANSFER_ACTIVE);
			if (reg)
				break;
			if (time_after(jiffies, timeout)) {
				ppd_dev_err(ppd,
					    "timeout waiting for LINK_TRANSFER_ACTIVE\n");
				return -ETIMEDOUT;
			}
			udelay(2);
		}

		write_csr(dd, DC_LCB_CFG_ALLOW_LINK_UP,
			  1ull << DC_LCB_CFG_ALLOW_LINK_UP_VAL_SHIFT);
	}

	if (!loopback) {
		/*
		 * When doing quick linkup and not in loopback, both
		 * sides must be done with LCB set-up before either
		 * starts the quick linkup.  Put a delay here so that
		 * both sides can be started and have a chance to be
		 * done with LCB set up before resuming.
		 */
		ppd_dev_err(ppd,
			    "Pausing for peer to be finished with LCB set up\n");
		msleep(5000);
		ppd_dev_err(ppd,
			    "Continuing with quick linkup\n");
	}

	write_csr(dd, DC_LCB_ERR_EN, 0); /* mask LCB errors */
	set_8051_lcb_access(dd);
#endif

	ppd->host_link_state = HLS_UP_INIT;
	/*
	 * State "quick" LinkUp request sets the physical link state to
	 * LinkUp without a verify capability sequence.
	 */
	ret = set_physical_link_state(ppd, PLS_QUICK_LINKUP);
	if (ret != HCMD_SUCCESS) {
		ppd_dev_err(ppd,
			    "%s: set physical link state to quick LinkUp failed with return %d\n",
			    __func__, ret);

#if 0 /* WFR legacy */
		write_csr(dd, DC_LCB_ERR_EN, ~0ull); /* watch LCB errors */

#endif
		if (ret >= 0)
			ret = -EINVAL;
		return ret;
	}
	ret = hfi2_wait_logical_linkstate(ppd, IB_PORT_INIT, 1000);
	if (ret) {
		/* logical state didn't change. */
		ppd_dev_err(ppd,
			    "%s: logical state did not change to INIT\n",
			    __func__);
		return ret;
	}
	/* clear old transient LINKINIT_REASON code */
	if (ppd->linkinit_reason >= OPA_LINKINIT_REASON_CLEAR)
		ppd->linkinit_reason = OPA_LINKINIT_REASON_LINKUP;
	handle_linkup_change(ppd, 1);

	return 0; /* success */
}

static u32 read_physical_state(const struct hfi_pportdata *ppd)
{
	u64 reg;

	reg = read_csr(ppd->dd, CRK_CRK8051_STS_CUR_STATE);
	return (reg >> CRK_CRK8051_STS_CUR_STATE_PORT_SHIFT)
				& CRK_CRK8051_STS_CUR_STATE_PORT_MASK;
}

static u32 read_logical_state(const struct hfi_pportdata *ppd)
{
	u64 reg;

	reg = read_csr(ppd->dd, OC_LCB_CFG_PORT);
	return (reg >> OC_LCB_CFG_PORT_LINK_STATE_SHIFT)
				& OC_LCB_CFG_PORT_LINK_STATE_MASK;
}

/*
 * Helper for set_link_state().  Do not call except from that routine.
 * Expects ppd->hls_mutex to be held.
 *
 * @rem_reason value to be sent to the neighbor
 *
 * LinkDownReasons only set if transition succeeds.
 */
static int goto_offline(struct hfi_pportdata *ppd, u8 rem_reason)
{
#if 0 /* WFR legacy */
	struct hfi_devdata *dd = ppd->dd;
#endif
	u32 previous_state;
	int ret;

	previous_state = ppd->host_link_state;
	ppd->host_link_state = HLS_GOING_OFFLINE;

	/* start offline transition */
	ret = set_physical_link_state(ppd, (rem_reason << 8) | PLS_OFFLINE);
	if (ret != HCMD_SUCCESS) {
		ppd_dev_err(ppd,
			"Failed to transition to Offline link state, return %d\n",
			ret);
		return -EINVAL;
	}
#if 0 /* WFR legacy */
	if (ppd->offline_disabled_reason ==
			HFI_ODR_MASK(OPA_LINKDOWN_REASON_NONE))
		ppd->offline_disabled_reason =
		HFI_ODR_MASK(OPA_LINKDOWN_REASON_TRANSIENT);
#endif

	/*
	 * Wait for offline transition. It can take a while for
	 * the link to go down.
	 */
	ret = wait_physical_linkstate(ppd, PLS_OFFLINE, 10000);
	if (ret < 0)
		return ret;

	/* make sure the logical state is also down */
	hfi2_wait_logical_linkstate(ppd, IB_PORT_DOWN, 1000);
	/*
	 * Now in charge of LCB - must be after the physical state is
	 * offline.quiet and before host_link_state is changed.
	 */
#if 0 /* WFR legacy */
	write_csr(ppd->dd, DC_LCB_ERR_EN, ~0ull); /* watch LCB errors */
#endif
	ppd->host_link_state = HLS_LINK_COOLDOWN; /* LCB access allowed */

	/*
	 * The LNI has a mandatory wait time after the physical state
	 * moves to Offline.Quiet.  The wait time may be different
	 * depending on how the link went down.  The 8051 firmware
	 * will observe the needed wait time and only move to ready
	 * when that is completed.  The largest of the quiet timeouts
	 * is 2.5s, so wait that long and then a bit more.
	 */
	ret = hfi_wait_firmware_ready(ppd, 3000);
	if (ret) {
		ppd_dev_err(ppd,
			    "After going offline, timed out waiting for the 8051 to become ready to accept host requests\n");
		/* state is really offline, so make it so */
		ppd->host_link_state = HLS_DN_OFFLINE;
		return ret;
	}

	/*
	 * The state is now offline and the 8051 is ready to accept host
	 * requests.
	 *	- change our state
	 *	- notify others if we were previously in a linkup state
	 */
	ppd->host_link_state = HLS_DN_OFFLINE;
	if (previous_state & HLS_UP)
		/* went down while link was up */
		handle_linkup_change(ppd, 0);
#if 0 /* WFR legacy */
	} else if (previous_state
			& (HLS_DN_POLL | HLS_VERIFY_CAP | HLS_GOING_UP)) {
		/* went down while attempting link up */

		/* The QSFP does not need to be reset on LNI failure */
		ppd->qsfp_info.reset_needed = 0;
		/* byte 1 of last_*_state is the failure reason */
		read_last_local_state(dd, &last_local_state);
		read_last_remote_state(dd, &last_remote_state);
		ppd_dev_err(ppd,
			    "LNI failure last states: local 0x%08x, remote 0x%08x\n",
			    last_local_state, last_remote_state);
	}
#endif

	/* the active link width (downgrade) is 0 on link down */
	ppd->link_width_active = 0;
	ppd->link_width_downgrade_tx_active = 0;
	ppd->link_width_downgrade_rx_active = 0;
#if 0 /* WFR legacy */
	ppd->current_egress_rate = 0;
#endif
	return 0;
}

/*
 * Translate from the OPA_LINK_WIDTH handed to us by the FM to bits
 * used in the Verify Capability link width attribute.
 */
static u16 opa_to_vc_link_widths(u16 opa_widths)
{
	int i;
	u16 result = 0;

	static const struct link_bits {
		u16 from;
		u16 to;
	} opa_link_xlate[] = {
		{ OPA_LINK_WIDTH_1X, 1 << (1 - 1)  },
		{ OPA_LINK_WIDTH_2X, 1 << (2 - 1)  },
		{ OPA_LINK_WIDTH_3X, 1 << (3 - 1)  },
		{ OPA_LINK_WIDTH_4X, 1 << (4 - 1)  },
	};

	for (i = 0; i < ARRAY_SIZE(opa_link_xlate); i++) {
		if (opa_widths & opa_link_xlate[i].from)
			result |= opa_link_xlate[i].to;
	}
	return result;
}

/*
 * Set link attributes before moving to polling.
 */
static int set_local_link_attributes(struct hfi_pportdata *ppd)
{
	struct hfi_devdata *dd = ppd->dd;
#if 0 /* WFR legacy */
	u8 enable_lane_tx;
	u8 tx_polarity_inversion;
	u8 rx_polarity_inversion;
	u8 local_tx_rate;
#endif
	int ret;

#if 0 /* WFR legacy */
	/* reset our fabric serdes to clear any lingering problems */
	fabric_serdes_reset(dd);

	/* set the local tx rate - need to read-modify-write */
	ret = read_tx_settings(
		dd, &enable_lane_tx, &tx_polarity_inversion,
		&rx_polarity_inversion, &local_tx_rate);
	if (ret)
		goto set_local_link_attributes_fail;

	/* set the tx rate to all enabled */
	local_tx_rate = ppd->link_speed_enabled;

	enable_lane_tx = 0xF; /* enable all four lanes */
	ret = write_tx_settings(dd, enable_lane_tx, tx_polarity_inversion,
				rx_polarity_inversion, local_tx_rate);
	if (ret != HCMD_SUCCESS)
		goto set_local_link_attributes_fail;

	/*
	 * DC supports continuous updates.
	 */
	ret = write_vc_local_phy(dd,
				 0 /* no power management */,
				 1 /* continuous updates */);
	if (ret != HCMD_SUCCESS)
		goto set_local_link_attributes_fail;
#endif
	write_csr(dd, CRK_CRK8051_CFG_LOCAL_PORT_NO,
		  ppd->pnum & CRK_CRK8051_CFG_LOCAL_PORT_NO_VAL_MASK);

	ret = write_vc_local_fabric(ppd, ppd->vau, 0, ppd->vcu,
				    ppd->vl15_init,
				    ppd->port_crc_mode_enabled);
	if (ret != HCMD_SUCCESS)
		goto set_local_link_attributes_fail;

	/*
	 * write desired link width policy.
	 * See "Initial Link Width Policy Configuration" of MNH HAS.
	 */
	ret = write_vc_local_link_width(
		ppd, 0, 0,
		opa_to_vc_link_widths(ppd->link_width_enabled));
	if (ret != HCMD_SUCCESS)
		goto set_local_link_attributes_fail;

	/* let peer know who we are */
	ret = write_local_device_id(ppd, dd->pdev->device, dd->minrev);
	if (ret == HCMD_SUCCESS)
		return 0;

set_local_link_attributes_fail:
	ppd_dev_err(ppd,
		    "Failed to set local link attributes, return 0x%x\n",
		    ret);
	return ret;
}

static u32 chip_to_opa_lstate(struct hfi_devdata *dd, u32 chip_lstate)
{
	switch (chip_lstate) {
	default:
		dd_dev_err(dd,
			   "Unknown logical state 0x%x, reporting IB_PORT_DOWN\n",
			   chip_lstate);
		/* fall through */
	case LSTATE_DOWN:
		return IB_PORT_DOWN;
	case LSTATE_INIT:
		return IB_PORT_INIT;
	case LSTATE_ARMED:
		return IB_PORT_ARMED;
	case LSTATE_ACTIVE:
		return IB_PORT_ACTIVE;
	}
}

/* return the OPA port logical state name */
static const char *opa_lstate_name(u32 lstate)
{
	static const char * const port_logical_names[] = {
		"PORT_NOP",
		"PORT_DOWN",
		"PORT_INIT",
		"PORT_ARMED",
		"PORT_ACTIVE",
		"PORT_ACTIVE_DEFER",
	};
	if (lstate < ARRAY_SIZE(port_logical_names))
		return port_logical_names[lstate];
	return "unknown";
}

/*
 * hfi2_wait_logical_linkstate - wait for an IB link state change to occur
 * @ppd: port device
 * @state: the state to wait for
 * @msecs: the number of milliseconds to wait
 *
 * Wait up to msecs milliseconds for IB link state change to occur.
 * For now, take the easy polling route.
 * Returns 0 if state reached, otherwise -ETIMEDOUT.
 */
int hfi2_wait_logical_linkstate(struct hfi_pportdata *ppd, u32 state,
				int msecs)
{
	unsigned long timeout;
	u32 new_state;

	timeout = jiffies + msecs_to_jiffies(msecs);
	while (1) {
		new_state = chip_to_opa_lstate(ppd->dd,
					       read_logical_state(ppd));
		if (new_state == state)
			break;
		if (time_after(jiffies, timeout)) {
			dd_dev_err(ppd->dd,
				   "timeout waiting for link state 0x%x\n",
				   state);
			return -ETIMEDOUT;
		}
		msleep(20);
		schedule();
	}

	dd_dev_info(ppd->dd,
		    "logical state changed to %s (0x%x)\n",
		    opa_lstate_name(state),
		    state);
	return 0;
}

/*
 * Handle a SMA idle message
 *
 * This is a work-queue function outside of the interrupt.
 */
static void handle_sma_message(struct work_struct *work)
{
	struct hfi_pportdata *ppd = container_of(work, struct hfi_pportdata,
							sma_message_work);
	u64 msg;
	int ret;

	/*
	 * msg is bytes 1-4 of the 40-bit idle message - the command code
	 * is stripped off
	 */
	ret = read_idle_sma(ppd, &msg);
	if (ret)
		return;
	/*
	 * React to the SMA message.  Byte[1] (0 for us) is the command.
	 */
	switch (msg & 0xff) {
	case SMA_IDLE_ARM:
		/*
		 * See OPAv1 table 9-14 - HFI and External Switch Ports Key
		 * State Transitions
		 *
		 * Only expected in INIT or ARMED, discard otherwise.
		 */

		if (ppd->host_link_state & (HLS_UP_INIT | HLS_UP_ARMED))
			ppd->neighbor_normal = 1;

		break;
	case SMA_IDLE_ACTIVE:
		/*
		 * See OPAv1 table 9-14 - HFI and External Switch Ports Key
		 * State Transitions
		 *
		 * Can activate the node.  Discard otherwise.
		 */

		/*
		 * FXRTODO: We should only do this in the ARMED state or when
		 * active optimize is enabled according to STL spec (chapter 9).
		 * In Simics we see on FXR that MNH is only interrupting once
		 * when we send two idle SMA messages (sending ARMED, then
		 * ACTIVE). Since there is only room for one message, the
		 * neighbor sees only the ACTIVE message. Set neighbor_normal
		 * regardless of the state until we can get this sorted.
		 */
		ppd->neighbor_normal = 1;

		if (ppd->host_link_state == HLS_UP_ARMED &&
		    ppd->is_active_optimize_enabled) {
			ret = hfi_set_link_state(ppd, HLS_UP_ACTIVE);
			if (ret)
				ppd_dev_err(ppd,
					    "%s: received Active SMA idle message, couldn't set link to Active %d\n",
					    __func__,
					    ret);
		}

		break;
	default:
		ppd_dev_err(ppd,
			    "%s: received unexpected SMA idle message 0x%llx\n",
			    __func__, msg);
		break;
	}
}

static inline void add_rcvctrl(struct hfi_devdata *dd, u64 add)
{
#if 0 /* WFR legacy */
	adjust_rcvctrl(dd, add, 0);
#endif
}

static void handle_quick_linkup(struct hfi_pportdata *ppd)
{
	ppd->neighbor_guid = read_csr(ppd->dd, CRK_CRK8051_STS_REMOTE_GUID);
	ppd->neighbor_type =
		read_csr(ppd->dd,
			      CRK_CRK8051_STS_REMOTE_NODE_TYPE) &
			      CRK_CRK8051_STS_REMOTE_NODE_TYPE_VAL_SMASK;
	ppd->neighbor_port_number =
		read_csr(ppd->dd,
			      CRK_CRK8051_STS_REMOTE_PORT_NO) &
			      CRK_CRK8051_STS_REMOTE_PORT_NO_VAL_SMASK;
	dd_dev_info(ppd->dd, "Neighbor GUID: %llx Neighbor type %d\n",
		    ppd->neighbor_guid, ppd->neighbor_type);
}

/*
 * Handle a linkup or link down notification.
 * This is called outside an interrupt.
 */
static void handle_linkup_change(struct hfi_pportdata *ppd, u32 linkup)
{
	enum ib_event_type ev;

#if 0 /* WFR legacy */
	if (!(ppd->linkup ^ !!linkup))
		return;	/* no change, nothing to do */
#endif

	if (linkup) {
		/*
		 * Quick linkup and all link up on the simulator does not
		 * trigger or implement:
		 *	- VerifyCap interrupt
		 *	- VerifyCap frames
		 * But rather moves directly to LinkUp.
		 *
		 * Do the work of the VerifyCap interrupt handler,
		 * handle_verify_cap(), but do not try moving the state to
		 * LinkUp as we are already there.
		 *
		 * NOTE: This uses this device's vAU, vCU, and vl15_init for
		 * the remote values.  Both sides must be using the values.
		 */
		if (quick_linkup) {
			hfi_set_up_vl15(ppd, ppd->vau, ppd->vl15_init);
			write_csr(ppd->dd, OC_LCB_CFG_CRC_MODE,
				      HFI_LCB_CRC_14B <<
				      OC_LCB_CFG_CRC_MODE_TX_VAL_SHIFT);
			hfi_assign_remote_cm_au_table(ppd, ppd->vcu);
			handle_quick_linkup(ppd);
			hfi_add_full_mgmt_pkey(ppd);
		}

		ppd->neighbor_guid =
			read_csr(ppd->dd, CRK_CRK8051_STS_REMOTE_GUID);
		ppd->neighbor_port_number =
			read_csr(ppd->dd, CRK_CRK8051_STS_REMOTE_PORT_NO) &
				CRK_CRK8051_STS_REMOTE_PORT_NO_VAL_SMASK;
		ppd->neighbor_type =
			read_csr(ppd->dd, CRK_CRK8051_STS_REMOTE_NODE_TYPE) &
				CRK_CRK8051_STS_REMOTE_NODE_TYPE_VAL_SMASK;
		ppd->neighbor_fm_security =
			read_csr(ppd->dd, CRK_CRK8051_STS_REMOTE_FM_SECURITY) &
				CRK_CRK8051_STS_REMOTE_FM_SECURITY_DISABLED_SMASK;

		ppd_dev_info(ppd,
			"Neighbor Guid: %llx Neighbor type %d MgmtAllowed %d FM security bypass %d\n",
			ppd->neighbor_guid, ppd->neighbor_type,
			ppd->mgmt_allowed, ppd->neighbor_fm_security);

		/*
		 * MgmtAllowed information, which is exchanged during
		 * LNI, is available at this point
		 */
		set_mgmt_allowed(ppd);

		if (ppd->mgmt_allowed)
			hfi_add_full_mgmt_pkey(ppd);

		/* physical link went up */
#if 0 /* WFR legacy */
		ppd->linkup = 1;
#endif
		ppd->offline_disabled_reason = OPA_LINKDOWN_REASON_NONE;

		/*
		 * update link_width_* after firmware updated.
		 * See "Link Width Refinement During LNI" of MNH HAS.
		 */
		get_linkup_link_widths(ppd);

	} else {
		struct ib_event event;
#if 0 /* WFR legacy */
		/* physical link went down */
		ppd->linkup = 0;

		/* clear HW details of the previous connection */
		reset_link_credits(dd);

		/* freeze after a link down to guarantee a clean egress */
		start_freeze_handling(ppd, FREEZE_SELF | FREEZE_LINK_DOWN);
#endif

		ev = IB_EVENT_PORT_ERR;

		/* if we are down, the neighbor is down */
		ppd->neighbor_normal = 0;
		ppd->actual_vls_operational = 0;

		/* notify IB of the link change */
		event.event = IB_EVENT_PORT_ERR;
		event.device = &ppd->dd->ibd->rdi.ibdev;
		event.element.port_num = ppd->pnum;
		ib_dispatch_event(&event);
	}
}

void hfi_set_link_down_reason(struct hfi_pportdata *ppd, u8 lcl_reason,
			      u8 neigh_reason, u8 rem_reason)
{
	if (ppd->local_link_down_reason.latest == 0 &&
	    ppd->neigh_link_down_reason.latest == 0) {
		ppd->local_link_down_reason.latest = lcl_reason;
		ppd->neigh_link_down_reason.latest = neigh_reason;
		ppd->remote_link_down_reason = rem_reason;
	}
}

/*
 * Convert the given link width to the OPA link width bitmask.
 */
static u16 link_width_to_bits(struct hfi_pportdata *ppd, u16 width)
{
	switch (width) {
	case 0:
		/*
		 * quick linkup do not set the width.
		 * Just set it to 4x without complaint.
		 */
		if (quick_linkup)
			return OPA_LINK_WIDTH_4X;
		return 0; /* no lanes up */
	case 1: return OPA_LINK_WIDTH_1X;
	case 2: return OPA_LINK_WIDTH_2X;
	case 3: return OPA_LINK_WIDTH_3X;
	default:
		ppd_dev_info(ppd, "%s: invalid width %d, using 4\n",
			     __func__, width);
		/* fall through */
	case 4: return OPA_LINK_WIDTH_4X;
	}
}

/*
 * Read verify_cap_local_fm_link_width[1] to obtain the link widths.
 * Valid after the end of VerifyCap and during LinkUp.  Does not change
 * after link up.  I.e. look elsewhere for downgrade information.
 *
 * Bits are:
 *	+ bits [7:4] contain the number of active transmitters
 *	+ bits [3:0] contain the number of active receivers
 * These are numbers 1 through 4 and can be different values if the
 * link is asymmetric.
 *
 * verify_cap_local_fm_link_width[0] retains its original value.
 */
static void get_linkup_widths(struct hfi_pportdata *ppd, u16 *tx_width,
			      u16 *rx_width)
{
	u16 widths, tx, rx;
	u8 misc_bits, local_flags;
#if 0 /* WFR legacy */
	u16 active_tx, active_rx;
#endif

	read_vc_local_link_width(ppd, &misc_bits, &local_flags, &widths);
	tx = widths >> 12;
	rx = (widths >> 8) & 0xf;

	*tx_width = link_width_to_bits(ppd, tx);
	*rx_width = link_width_to_bits(ppd, rx);
}

/*
 * Set ppd->link_width_active and ppd->link_width_downgrade_active using
 * hardware information when the link first comes up.
 *
 * The link width is not available until after VerifyCap.AllFramesReceived
 * (the trigger for handle_verify_cap), so this is outside that routine
 * and should be called when the 8051 signals linkup.
 */
static void get_linkup_link_widths(struct hfi_pportdata *ppd)
{
	u16 tx_width, rx_width;

	/* get end-of-LNI link widths */
	get_linkup_widths(ppd, &tx_width, &rx_width);

	/* use tx_width as the link is supposed to be symmetric on link up */
	ppd->link_width_active = tx_width;
	/* link width downgrade active (LWD.A) starts out matching LW.A */
	ppd->link_width_downgrade_tx_active = ppd->link_width_active;
	ppd->link_width_downgrade_rx_active = ppd->link_width_active;
	/* per OPA spec, on link up LWD.E resets to LWD.S */
	ppd->link_width_downgrade_enabled = ppd->link_width_downgrade_supported;
#if 0 /* WFR legacy */
	/* cache the active egress rate (units {10^6 bits/sec]) */
	ppd->current_egress_rate = active_egress_rate(ppd);
#endif
}

static void adjust_lcb_for_ckts_firmware(struct hfi_pportdata *ppd)
{
	struct hfi_devdata *dd = ppd->dd;

	write_csr(dd, OC_LCB_CFG_TX_FIFOS_RESET, 0x1);
	write_csr(dd, OC_LCB_CFG_LN_RE_ENABLE, 0x3);
	write_csr(dd, OC_LCB_CFG_LN_TEST_TIME_TO_PASS, 0xfff);
	write_csr(dd, OC_LCB_CFG_LN_DEGRADE, 0x2730);
	write_csr(dd, OC_LCB_CFG_CNT_FOR_SKIP_STALL, 0x1113);
	write_csr(dd, OC_LCB_CFG_LINK_PARTNER_GEN, 0x1);
	write_csr(dd, OC_LCB_CFG_REPLAY_PROTOCOL, 0x4401);
	write_csr(dd, OC_LCB_CFG_TX_FIFOS_RESET, 0x0);
}

/*
 * Handle a verify capabilities interrupt from MNH/8051.
 *
 * This is a work-queue function outside of the interrupt.
 */
static void handle_verify_cap(struct work_struct *work)
{
	struct hfi_pportdata *ppd = container_of(work, struct hfi_pportdata,
								link_vc_work);
	u8 vcu;
	u8 vau;
	u8 z;
	u16 vl15buf;
	u16 device_id;
	u16 crc_mask;
	u16 crc_val;
	u8 device_rev;
	u8 partner_supported_crc;
#if 0 /* WFR legacy */
	struct hfi_devdata *dd = ppd->dd;
#endif
	u16 link_widths;
	u8 remote_max_rate;
	u8 rate;
	u32 _8051_port = read_physical_state(ppd);

	if (_8051_port != PLS_CONFIGPHY_VERIFYCAP) {
		ppd_dev_info(ppd, "False interrupt on %s(): 0x%x. Ignore.",
			     __func__, _8051_port);
		return;
	}
	hfi_set_link_state(ppd, HLS_VERIFY_CAP);
#if 0 /* WFR legacy */
	lcb_shutdown(ppd, 0);

	read_vc_remote_phy(dd, &power_management, &continious);
#endif

	read_vc_remote_fabric(
		ppd,
		&vau,
		&z,
		&vcu,
		&vl15buf,
		&partner_supported_crc);

	read_vc_remote_link_width(ppd, &remote_max_rate, &link_widths);
	read_remote_device_id(ppd, &device_id, &device_rev);

#if 0
	/* print the active widths */
	get_link_widths(dd, &active_tx, &active_rx);
	ppd_dev_info(
		ppd,
		"Peer PHY: power management 0x%x, continuous updates 0x%x\n",
		(int)power_management, (int)continious);
#endif
	ppd_dev_info(
		ppd,
		"Peer Fabric: vAU %d, Z %d, vCU %d, vl15 credits 0x%x, CRC sizes 0x%x\n",
		(int)vau,
		(int)z,
		(int)vcu,
		(int)vl15buf,
		(int)partner_supported_crc);

	ppd_dev_info(ppd, "Peer Link Width: tx rate 0x%x, widths 0x%x\n",
		     (u32)remote_max_rate, (u32)link_widths);
	ppd_dev_info(ppd, "Peer Device ID: 0x%04x, Revision 0x%02x\n",
		     (u32)device_id, (u32)device_rev);

	hfi_set_up_vl15(ppd, vau, vl15buf);

	/* set up the LCB CRC mode */
	crc_mask = ppd->port_crc_mode_enabled & partner_supported_crc;

	/* order is important: use the lowest bit in common */
	if (crc_mask & HFI_CAP_CRC_14B)
		crc_val = HFI_LCB_CRC_14B;
	else if (crc_mask & HFI_CAP_CRC_48B)
		crc_val = HFI_LCB_CRC_48B;
	else if (crc_mask & HFI_CAP_CRC_12B_16B_PER_LANE)
		crc_val = HFI_LCB_CRC_12B_16B_PER_LANE;
	else
		crc_val = HFI_LCB_CRC_16B;

	ppd_dev_info(ppd, "Final LCB CRC mode: %d\n", (int)crc_val);
	write_csr(ppd->dd, OC_LCB_CFG_CRC_MODE,
		      (u64)crc_val << OC_LCB_CFG_CRC_MODE_TX_VAL_SHIFT);
#if 0
	/* set (14b only) or clear sideband credit */
	reg = read_csr(dd, SEND_CM_CTRL);
	if (crc_val == LCB_CRC_14B && crc_14b_sideband) {
		write_csr(dd, SEND_CM_CTRL,
			  reg | SEND_CM_CTRL_FORCE_CREDIT_MODE_SMASK);
	} else {
		write_csr(dd, SEND_CM_CTRL,
			  reg & ~SEND_CM_CTRL_FORCE_CREDIT_MODE_SMASK);
	}
#endif

	ppd->link_speed_active = 0;	/* invalid value */
	/* actual rate is highest bit of the ANDed rates */
	rate = 0;
	switch (remote_max_rate) {
	default:
		ppd_dev_err(ppd, "invalid remote max rate %d, set up to 50G",
			    remote_max_rate);
		/* fall through */
	case 0: /* 50G */
		rate |= OPA_LINK_SPEED_50G; /* fall through */
	case 1: /* 25G */
		rate |= OPA_LINK_SPEED_25G | OPA_LINK_SPEED_12_5G;
		break;
	}
	rate &= ppd->link_speed_enabled;
	if (rate & OPA_LINK_SPEED_50G) {
		ppd->link_speed_active = OPA_LINK_SPEED_50G;
	} else if (rate & OPA_LINK_SPEED_25G) {
		ppd->link_speed_active = OPA_LINK_SPEED_25G;
	} else if (rate & OPA_LINK_SPEED_12_5G) {
		ppd->link_speed_active = OPA_LINK_SPEED_12_5G;
	} else {
		ppd_dev_err(ppd, "no common speed with remote: 0x%x, set up 50Gb\n",
			    rate);
		ppd->link_speed_active = OPA_LINK_SPEED_50G;
	}

	/*
	 * Cache the values of the supported, enabled, and active
	 * LTP CRC modes to return in 'portinfo' queries. But the bit
	 * flags that are returned in the portinfo query differ from
	 * what's in the link_crc_mask, crc_sizes, and crc_val
	 * variables. Convert these here.
	 */
	/* supported crc modes */
	ppd->port_ltp_crc_mode = hfi_cap_to_port_ltp(HFI_SUPPORTED_CRCS) <<
			HFI_LTP_CRC_SUPPORTED_SHIFT;

	/* enabled crc modes */
	ppd->port_ltp_crc_mode |=
		hfi_cap_to_port_ltp(ppd->port_crc_mode_enabled) <<
		HFI_LTP_CRC_ENABLED_SHIFT;

	/* active crc mode */
	ppd->port_ltp_crc_mode |= hfi_lcb_to_port_ltp(ppd, crc_val);

	/* set up the remote credit return table */
	hfi_assign_remote_cm_au_table(ppd, vcu);

#if 0
	/*
	 * The LCB is reset on entry to handle_verify_cap(), so this must
	 * be applied on every link up.
	 *
	 * Adjust LCB error kill enable to kill the link if
	 * these RBUF errors are seen:
	 *	REPLAY_BUF_MBE_SMASK
	 *	FLIT_INPUT_BUF_MBE_SMASK
	 */
	if (is_a0(dd)) {			/* fixed in B0 */
		reg = read_csr(dd, DC_LCB_CFG_LINK_KILL_EN);
		reg |= DC_LCB_CFG_LINK_KILL_EN_REPLAY_BUF_MBE_SMASK
			| DC_LCB_CFG_LINK_KILL_EN_FLIT_INPUT_BUF_MBE_SMASK;
		write_csr(dd, DC_LCB_CFG_LINK_KILL_EN, reg);
	}

	/* pull LCB fifos out of reset - all fifo clocks must be stable */
	write_csr(dd, DC_LCB_CFG_TX_FIFOS_RESET, 0);

	/* give 8051 access to the LCB CSRs */
	write_csr(dd, DC_LCB_ERR_EN, 0); /* mask LCB errors */
	set_8051_lcb_access(dd);
#endif

	/* tell the 8051 to go to LinkUp */
	hfi_set_link_state(ppd, HLS_GOING_UP);
}

static void hfi_handle_8051_host_msg(struct hfi_pportdata *ppd, u64 msg)
{
	if (msg & VERIFY_CAP_FRAME) {
		queue_work(ppd->hfi_wq, &ppd->link_vc_work);
		msg &= ~VERIFY_CAP_FRAME;
	}

	if (msg & LINKUP_ACHIEVED) {
		queue_work(ppd->hfi_wq, &ppd->link_up_work);
		msg &= ~LINKUP_ACHIEVED;
	}

	if (msg & LINK_GOING_DOWN) {
		/*
		 * if the link is already going down or disabled, do not
		 * queue another
		 */
		if (ppd->host_link_state & (HLS_GOING_OFFLINE |
			HLS_LINK_COOLDOWN | HLS_DN_OFFLINE)) {
			ppd_dev_info(ppd,
				     "%s() %d: not queuing link down: 0x%x\n",
				     __func__, __LINE__, ppd->host_link_state);
		} else {
			queue_work(ppd->hfi_wq, &ppd->link_down_work);
		}
		msg &= ~LINK_GOING_DOWN;
	}

	if (msg & BC_SMA_MSG) {
		queue_work(ppd->hfi_wq, &ppd->sma_message_work);
		msg &= ~BC_SMA_MSG;
	}

	/* FXRTODO: Implement this */
	if (msg & EXT_DEVICE_CFG_REQ) {
		ppd_dev_dbg(ppd, "EXT_DEVICE_CFG_REQ\n");
		msg &= ~EXT_DEVICE_CFG_REQ;
	}

	/* FXRTODO: Implement this */
	if (msg & LINK_WIDTH_DOWNGRADED) {
		ppd_dev_dbg(ppd, "LINK_WIDTH_DOWNGRADED\n");
		msg &= ~LINK_WIDTH_DOWNGRADED;
	}
	if (msg && !ppd->dd->emulation)
		ppd_dev_warn(ppd, "Unhandled host message 0x%llx\n", msg);
}

static void hfi_handle_8051_err(struct hfi_pportdata *ppd, u64 err)
{
	ppd->dd->stats.sps_errints++;
	/* FXRTODO: Implement this */
	if (err & FAILED_LNI) {
		ppd_dev_dbg(ppd, "FAILED_LNI\n");
		err &= ~FAILED_LNI;
	}

	/* FXRTODO: Implement this */
	if (err & UNKNOWN_FRAME) {
		ppd_dev_dbg(ppd, "UNKNOWN_FRAME\n");
		err &= ~UNKNOWN_FRAME;
	}

	if (err && !ppd->dd->emulation)
		ppd_dev_warn(ppd, "Unhandled 8051 error 0x%llx\n", err);
}

static void hfi_handle_host_irq(struct hfi_pportdata *ppd)
{
	u64 reg, msg, err;

	reg = read_csr(ppd->dd, CRK_CRK8051_DBG_ERR_INFO_SET_BY_8051);
	msg = HFI_SET_BY_8051_SPLIT(reg, HOST_MSG);
	err = HFI_SET_BY_8051_SPLIT(reg, ERROR);

	if (err)
		hfi_handle_8051_err(ppd, err);

	if (msg)
		hfi_handle_8051_host_msg(ppd, msg);
	/*
	 * FXRTODO: Needs follow up. This generally should not happen,
	 * but it does. We still need to clear the SET_BY_8051 bit. It
	 * may be an issue with Simics. For now, just print an error
	 * and clear the bit regardless.
	 */
	if (!err && !msg && !ppd->dd->emulation)
		ppd_dev_err(ppd, "Unhandled 8051 interrupt!\n");

	write_csr(ppd->dd, CRK_CRK8051_ERR_CLR,
		       CRK_CRK8051_ERR_CLR_SET_BY_8051_SMASK);

	/* TODO: take care other interrupts */
}

irqreturn_t hfi_irq_oc_handler(int irq, void *dev_id)
{
	struct hfi_irq_entry *me = dev_id;
	struct hfi_devdata *dd = me->dd;
	u8 port;

	this_cpu_inc(*dd->int_counter);
	trace_hfi2_irq_phy(me);

	for (port = 1; port <= dd->num_pports; port++) {
		struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);

		hfi_handle_host_irq(ppd);
	}

	return IRQ_HANDLED;
}

/*
 * FPC error handling.
 */
static const char * const fm_config_txt[] = {
[0] =
	"BadHeadDist: Distance violation between two head flits",
[1] =
	"BadTailDist: Distance violation between two tail flits",
[2] =
	"BadCtrlDist: Distance violation between two credit control flits",
[3] =
	"BadCrdAck: Credits return for unsupported VL",
[4] =
	"UnsupportedVLMarker: Received VL Marker",
[5] =
	"BadPreempt: Exceeded the preemption nesting level",
[6] =
	"BadControlFlit: Received unsupported control flit",
/* no 7 */
[8] =
	"UnsupportedVLMarker: Received VL Marker for unconfigured or disabled VL",
};

static const char * const port_rcv_txt[] = {
[1] =
	"BadPktLen: Illegal PktLen",
[2] =
	"PktLenTooLong: Packet longer than PktLen",
[3] =
	"PktLenTooShort: Packet shorter than PktLen",
[4] =
	"BadSLID: Illegal SLID (0, using multicast as SLID)",
[5] =
	"BadDLID: Illegal DLID (0, doesn't match HFI)",
[6] =
	"BadL2: Unsupported L2 Type",
[7] =
	"BadSC: Invalid SC",
[9] =
	"Headless: Tail or Body before Head",
[11] =
	"PreemptError: Preempting with same VL",
[12] =
	"PreemptVL15: Interleaving a VL15 packet between two Gen1 devices",
[13] =
	"BadVLMarker: SC Marker for an inactive VL",
[14] =
	"PreemptL2Head: Interleaving of the L2 Header"
};

#define OPA_LDR_FMCONFIG_OFFSET 16
#define OPA_LDR_PORTRCV_OFFSET 0

void hfi_handle_fpc_uncorrectable_error(struct hfi_devdata *dd, u64 reg,
					char *fpc_name)
{
	struct hfi_pportdata *ppd;
	u64 info;

	ppd = to_hfi_ppd(dd, 1);
	if (!(ppd->err_info_uncorrectable &
		OPA_EI_STATUS_SMASK)) {
		info = read_csr(ppd->dd,
					   FXR_FPC_ERR_INFO_UNCORRECTABLE);
		/* save the error code and set the status bit */
		ppd->err_info_uncorrectable = info
					& OPA_EI_CODE_SMASK;
		ppd->err_info_uncorrectable |=
					OPA_EI_STATUS_SMASK;
	}
}

void hfi_handle_fpc_link_error(struct hfi_devdata *dd, u64 reg, char *fpc_name)
{
	struct hfi_pportdata *ppd;

	ppd = to_hfi_ppd(dd, 1);
	/* link_downed is reported as an error counter in pma */
	if (ppd->link_downed < (u32)UINT_MAX)
		ppd->link_downed++;
}

void hfi_handle_fpc_fmconfig_error(struct hfi_devdata *dd, u64 reg,
				   char *fpc_name)
{
	struct hfi_pportdata *ppd;
	const char *extra;
	u64 info;
	char buf[96];
	u8 reason_valid = 1;
	u8 lcl_reason = 0;
	int do_bounce = 0;

	ppd = to_hfi_ppd(dd, 1);
	info = read_csr(ppd->dd, FXR_FPC_ERR_INFO_FMCONFIG);
	if (!(ppd->err_info_fmconfig & OPA_EI_STATUS_SMASK)) {
		ppd->err_info_fmconfig = info & OPA_EI_CODE_SMASK;
		ppd->err_info_fmconfig |= OPA_EI_STATUS_SMASK;
	}
	switch (info) {
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
		extra = fm_config_txt[info];
		break;
	case 8:
		extra = fm_config_txt[info];
		if (ppd->port_error_action &
		    OPA_PI_MASK_FM_CFG_UNSUPPORTED_VL_MARKER) {
			do_bounce = 1;
			/*
			 * lcl_reason cannot be derived from info
			 * for this error
			 */
			lcl_reason =
			  OPA_LINKDOWN_REASON_UNSUPPORTED_VL_MARKER;
		}
		break;
	default:
		reason_valid = 0;
		snprintf(buf, sizeof(buf), "reserved%llu", info);
		extra = buf;
		break;
	}

	if (reason_valid && !do_bounce) {
		do_bounce = ppd->port_error_action &
				(1 << (OPA_LDR_FMCONFIG_OFFSET + info));
		lcl_reason = info + OPA_LINKDOWN_REASON_BAD_HEAD_DIST;
	}

	/* just report this */
	ppd_dev_info(ppd, "FPC Error: fmconfig error: %s\n", extra);
	if (do_bounce) {
		ppd_dev_info(ppd,
			     "%s: PortErrorAction bounce\n", __func__);
		hfi_set_link_down_reason(ppd, lcl_reason, 0,
					 lcl_reason);
		queue_work(ppd->hfi_wq, &ppd->link_bounce_work);
	}
}

void hfi_handle_fpc_rcvport_error(struct hfi_devdata *dd, u64 reg,
				  char *fpc_name)
{
	struct hfi_pportdata *ppd;
	u64 info, hdr0, hdr1;
	const char *extra;
	char buf[96];
	u8 reason_valid = 1;
	u8 lcl_reason = 0;
	int do_bounce = 0;

	ppd = to_hfi_ppd(dd, 1);
	info = read_csr(ppd->dd,
				   FXR_FPC_ERR_INFO_PORTRCV);
	hdr0 = read_csr(ppd->dd,
				   FXR_FPC_ERR_INFO_PORTRCV_HDR0_A);
	hdr1 = read_csr(ppd->dd,
				   FXR_FPC_ERR_INFO_PORTRCV_HDR1_A);
	if (!(ppd->err_info_rcvport.status_and_code &
		OPA_EI_STATUS_SMASK)) {
		ppd->err_info_rcvport.status_and_code =
			info & OPA_EI_CODE_SMASK;
		ppd->err_info_rcvport.status_and_code |=
			OPA_EI_STATUS_SMASK;
		ppd->err_info_rcvport.packet_flit1 = hdr0;
		ppd->err_info_rcvport.packet_flit2 = hdr1;
	}
	switch (info) {
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 9:
	case 11:
	case 12:
	case 13:
	case 14:
		extra = port_rcv_txt[info];
		break;
	default:
		reason_valid = 0;
		snprintf(buf, sizeof(buf), "reserved%lld", info);
		extra = buf;
		break;
	}

	if (reason_valid && !do_bounce) {
		do_bounce = ppd->port_error_action &
				(1 << (OPA_LDR_PORTRCV_OFFSET + info));
		lcl_reason = info + OPA_LINKDOWN_REASON_RCV_ERROR_0;
	}

	/* just report this */
	ppd_dev_info(ppd, "FPC Error: PortRcv error: %s\n", extra);
	ppd_dev_info(ppd, "           hdr0 0x%llx, hdr1 0x%llx\n",
		     hdr0, hdr1);
	if (do_bounce) {
		ppd_dev_info(ppd,
			     "%s: PortErrorAction bounce\n", __func__);
		hfi_set_link_down_reason(ppd, lcl_reason, 0,
					 lcl_reason);
		queue_work(ppd->hfi_wq, &ppd->link_bounce_work);
	}
}

/*
 *
 * Enable interrupots from MNH/8051 by configuring its registers
 */
int hfi2_enable_8051_intr(struct hfi_pportdata *ppd)
{
	int ret;
	u64 reg;

	/* disable 8051 command complete interrupt and enable all others */
	ret = load_8051_config(
		ppd, HOST_INT_MSG_MASK, GENERAL_CONFIG,
		(BC_PWR_MGM_MSG | BC_SMA_MSG | BC_BCC_UNKNOWN_MSG |
		 BC_IDLE_UNKNOWN_MSG | EXT_DEVICE_CFG_REQ | VERIFY_CAP_FRAME |
		 LINKUP_ACHIEVED | LINK_GOING_DOWN |
		 LINK_WIDTH_DOWNGRADED) << 8);
	ret = ret != HCMD_SUCCESS ? -EINVAL : 0;
	if (ret)
		goto _return;

	/* enable 8051 interrupt */
	reg = read_csr(ppd->dd, CRK_CRK8051_ERR_EN_HOST);
	write_csr(ppd->dd, CRK_CRK8051_ERR_EN_HOST, reg |
		CRK_CRK8051_ERR_EN_HOST_SET_BY_8051_SMASK);
	reg = read_csr(ppd->dd, CRK_CRK8051_ERR_CLR);
	write_csr(ppd->dd,
		       CRK_CRK8051_ERR_CLR, reg |
		       CRK_CRK8051_ERR_CLR_SET_BY_8051_SMASK);
 _return:
	return ret;
}

/*
 * Disable interrupts from MNH/8051 by configuring its registers
 */
int hfi2_disable_8051_intr(struct hfi_pportdata *ppd)
{
	int ret;
	u64 reg;

	/* disable all interrupt from 8051 */
	ret = load_8051_config(ppd, HOST_INT_MSG_MASK, GENERAL_CONFIG,
			       0x0000 << 8);
	ret = ret != HCMD_SUCCESS ? -EINVAL : 0;
	if (ret)
		goto _return;

	/* disable 8051 interrupt */
	reg = read_csr(ppd->dd, CRK_CRK8051_ERR_EN_HOST);
	write_csr(ppd->dd, CRK_CRK8051_ERR_EN_HOST, reg &
		~CRK_CRK8051_ERR_EN_HOST_SET_BY_8051_SMASK);
 _return:
	return ret;
}

/*
 * un-initialize ports
 */
void hfi2_pport_link_uninit(struct hfi_devdata *dd)
{
	u8 port;
	struct hfi_pportdata *ppd;
	int ret;

	if (no_mnh)
		return;

	/*
	 * Make both port1 and 2 to Offline state then disable both ports'
	 * interrupt. Otherwise irq_oc_handle() produces spurious interrupt.
	 */
	for (port = 1; port <= dd->num_pports; port++) {
		ppd = to_hfi_ppd(dd, port);
		hfi_set_link_state(ppd, HLS_DN_OFFLINE);
	}
	for (port = 1; port <= dd->num_pports; port++) {
		ppd = to_hfi_ppd(dd, port);
		ret = hfi2_disable_8051_intr(ppd);
		if (ret)
			dd_dev_err(dd, "can't disable MNH/8051 interrupt: %d\n",
				   ret);
		hfi_psn_uninit(ppd);
	}
}

static int hfi2_8051_task(void *data)
{
	struct hfi_devdata *dd = data;
	int port;

	dd_dev_info(dd, "8051 kthread starting\n");
	while (!kthread_should_stop()) {
		for (port = 1; port <= dd->num_pports; port++) {
			struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);

			hfi_handle_host_irq(ppd);
		}
		msleep(20);
		schedule();
	}
	return 0;
}

/*
 * initialize ports
 */
int hfi2_pport_link_init(struct hfi_devdata *dd)
{
	int ret = 0;
	u8 port;
	struct hfi_pportdata *ppd;
	struct task_struct *task;

	for (port = 1; port <= dd->num_pports; port++) {
		ppd = to_hfi_ppd(dd, port);

		if (opafm_disable)
			hfi_set_max_lid(ppd, HFI_DEFAULT_MAX_LID_SUPP);

		if (no_mnh)
			continue;

		/* configure workqueues */
		INIT_WORK(&ppd->link_vc_work, handle_verify_cap);
		INIT_WORK(&ppd->link_up_work, handle_link_up);
		INIT_WORK(&ppd->link_down_work, handle_link_down);
		INIT_WORK(&ppd->link_bounce_work, handle_link_bounce);
		INIT_WORK(&ppd->sma_message_work, handle_sma_message);
		INIT_DELAYED_WORK(&ppd->start_link_work, handle_start_link);

		if (ppd->dd->emulation) {
			/* FXRTODO: Need to destroy kthread during uninit */
			task = kthread_run(hfi2_8051_task, dd, "hfi2_8051");
			if (IS_ERR(task)) {
				ret = PTR_ERR(task);
				goto _return;
			}
		}

		 /* configure interrupt registers of MNH/8051 */
		ret = hfi2_enable_8051_intr(ppd);
		if (ret) {
			dd_dev_err(dd, "Can't configure 8051 interrupts: %d\n",
				   ret);
			goto _return;
		}
		spin_lock_init(&ppd->cc_log_lock);
	}

	if (no_mnh)
		return 0;

	/*
	 * The following code has to be separated from above "for" loop because
	 * hfi_start_link(ppd) induces VerifyCap interrupt and its interrupt
	 * service routine,irq_oc_handler() scans both ports.
	 */
	for (port = 1; port <= dd->num_pports; port++) {
		ppd = to_hfi_ppd(dd, port);
		if (ppd->dd->icode == ICODE_ZEBU_EMULATION)
			adjust_lcb_for_ckts_firmware(ppd);
		hfi_start_link(ppd);
		if (opafm_disable) {
			ret = hfi2_wait_logical_linkstate(
				ppd, IB_PORT_INIT, WAIT_TILL_8051_LINKUP);
			if (ret) {
				dd_dev_err(dd, "Logical link state doesn't become INIT\n");
				goto _return;
			}
			hfi_set_link_state(ppd, HLS_UP_ARMED);
			hfi_set_link_state(ppd, HLS_UP_ACTIVE);
		}
	}
 _return:
	return ret;
}

/*
 * Change the physical port and/or logical link state.
 *
 * Returns 0 on success, -errno on failure.
 */
int hfi_set_link_state(struct hfi_pportdata *ppd, u32 state)
{
	int ret1, ret = 0;
	struct ib_event event = {.device = NULL};

	mutex_lock(&ppd->hls_lock);

	ppd_dev_info(ppd, "%s(%d) -> %s(%d) %s\n",
		     link_state_name(ppd->host_link_state),
		     ilog2(ppd->host_link_state),
		     link_state_name(state), ilog2(state),
		     link_state_reason_name(ppd, state));

	/*
	 * If we're going to a (HLS_*) link state that implies the logical
	 * link state is neither of (IB_PORT_ARMED, IB_PORT_ACTIVE), then
	 * reset is_sm_config_started to 0.
	 */
	if (!(state & (HLS_UP_ARMED | HLS_UP_ACTIVE)))
		ppd->is_sm_config_started = 0;

	/*
	 * Do nothing if the states match.  Let a poll to poll link bounce
	 * go through.
	 */
	if (ppd->host_link_state == state)
		goto _return;

	switch (state) {
	case HLS_DN_OFFLINE:
		if (ppd->host_link_state == HLS_DN_DISABLE)
			oc_start(ppd);

		/* allow any state to transition to offline */
		ret = goto_offline(ppd, ppd->remote_link_down_reason);
		if (!ret)
			ppd->remote_link_down_reason = 0;
		break;
	case HLS_DN_DISABLE:
		/* allow any state to transition to disabled */

		/* must transition to offline first */
		if (ppd->host_link_state != HLS_DN_OFFLINE) {
			ret = goto_offline(ppd, ppd->remote_link_down_reason);
			if (ret)
				break;
			ppd->remote_link_down_reason = 0;
		}

		ret1 = set_physical_link_state(ppd, PLS_DISABLED);
		if (ret1 != HCMD_SUCCESS) {
			ppd_dev_err(ppd,
				    "Failed to transition to Disabled link state, return 0x%x\n",
				    ret1);
			ret = -EINVAL;
			break;
		}
		ret = wait_physical_linkstate(ppd, PLS_DISABLED, 10000);
		if (ret) {
			dd_dev_err(ppd->dd,
				   "%s: physical state did not change to DISABLED\n",
				   __func__);
				break;
		}
		ppd->host_link_state = HLS_DN_DISABLE;
		oc_shutdown(ppd);
		break;
	case HLS_DN_DOWNDEF:
		ppd->host_link_state = HLS_DN_DOWNDEF;
		break;
	case HLS_DN_POLL:
		if ((ppd->host_link_state == HLS_DN_DISABLE ||
		     ppd->host_link_state == HLS_DN_OFFLINE))
			oc_start(ppd);

		/* Hand LED control to the HW */
		write_csr(ppd->dd, OC_LCB_CFG_LED, 0);

		if (ppd->host_link_state != HLS_DN_OFFLINE) {
			ret = goto_offline(ppd, ppd->remote_link_down_reason);
			ppd->remote_link_down_reason = 0;
		}

		ret = set_local_link_attributes(ppd);
		if (ret)
			break;

		ppd->port_error_action = 0;
		ppd->host_link_state = HLS_DN_POLL;

		/*
		 * fxr_simics_4_8_35 implements both quick_linkup and full
		 * linkup
		 */
		if (quick_linkup) {
			ret = do_quick_linkup(ppd);
		} else {
			ret1 = set_physical_link_state(ppd, PLS_POLLING);
			if (ret1 != HCMD_SUCCESS) {
				ppd_dev_err(ppd,
					    "Failed to transition to Polling link state, return 0x%x\n",
					    ret1);
				ret = -EINVAL;
			}
		}

#if 0 /* where OPA_LINKDOWN_REASON_NONE is defined? */
		ppd->offline_disabled_reason = OPA_LINKDOWN_REASON_NONE;
#endif
		/*
		 * If an error occurred on do_quick_linkup(),
		 * go back to offline. The caller may reschedule another
		 * attempt.
		 */
		if (ret)
			goto_offline(ppd, 0);
		else
			log_physical_state(ppd, PLS_POLLING);
		break;
	case HLS_UP_INIT:
		if (ppd->host_link_state == HLS_DN_POLL && quick_linkup) {
			/*
			 * Quick link up jumps from polling to here.
			 *
			 * Whether in normal or loopback mode, the
			 * simulator jumps from polling to link up.
			 * Accept that here.
			 */
			/* OK */;
		} else if (ppd->host_link_state != HLS_GOING_UP) {
			goto unexpected;
		}

		/* Wait for Link_Up physical state.
		 * Physical and Logical states should already
		 * be transitioned to LinkUp and LinkInit respectively.
		 */
		ret = wait_physical_linkstate(ppd, PLS_LINKUP, 1000);
		if (ret) {
			dd_dev_err(ppd->dd,
				   "%s: physical state did not change to LINK-UP\n",
				   __func__);
			break;
		}
		ppd->host_link_state = HLS_UP_INIT;
		ret = hfi2_wait_logical_linkstate(ppd, IB_PORT_INIT, 1000);
		if (ret) {
			ppd_dev_err(ppd,
				    "%s: logical state did not change to INIT\n",
				    __func__);
		} else {
			/* clear old transient LINKINIT_REASON code */
			if (ppd->linkinit_reason >= OPA_LINKINIT_REASON_CLEAR)
				ppd->linkinit_reason =
					OPA_LINKINIT_REASON_LINKUP;
			/* enable the port */
#if 0 /* where RCV_CTRL_RCV_PORT_ENABLE_SMASK is defined */
			add_rcvctrl(dd, RCV_CTRL_RCV_PORT_ENABLE_SMASK);
#endif
			handle_linkup_change(ppd, 1);
			ppd->host_link_state = HLS_UP_INIT;
		}
		break;
	case HLS_UP_ARMED:
		if (ppd->host_link_state != HLS_UP_INIT)
			goto unexpected;

		if (ppd->actual_vls_operational == 0) {
			ppd_dev_err(ppd,
				    "%s: data VLs not operational\n", __func__);
			goto unexpected;
		}

		set_logical_state(ppd, LSTATE_ARMED);
		ret = hfi2_wait_logical_linkstate(ppd, IB_PORT_ARMED, 1000);
		if (ret) {
			ppd_dev_err(ppd,
				    "%s: logical state did not change to ARMED\n",
				    __func__);
			break;
		}
		ppd->host_link_state = HLS_UP_ARMED;
		break;
	case HLS_UP_ACTIVE:
		if (ppd->host_link_state != HLS_UP_ARMED)
			goto unexpected;
		set_logical_state(ppd, LSTATE_ACTIVE);
		ret = hfi2_wait_logical_linkstate(ppd, IB_PORT_ACTIVE, 1000);
		if (ret) {
			ppd_dev_err(ppd,
				    "%s: logical state did not change to ACTIVE\n",
				    __func__);
		} else {
			ppd->host_link_state = HLS_UP_ACTIVE;
			/* Signal the IB layer that the port has went active */
			event.device = &ppd->dd->ibd->rdi.ibdev;
			event.element.port_num = ppd->pnum;
			event.event = IB_EVENT_PORT_ACTIVE;
		}
		break;
	case HLS_VERIFY_CAP:
		if (ppd->host_link_state != HLS_DN_POLL)
			goto unexpected;
		ppd->host_link_state = HLS_VERIFY_CAP;
		log_physical_state(ppd, PLS_CONFIGPHY_VERIFYCAP);
		break;
	case HLS_GOING_UP:
		if (ppd->host_link_state != HLS_VERIFY_CAP)
			goto unexpected;
		ret1 = set_physical_link_state(ppd, PLS_LINKUP);
		if (ret1 != HCMD_SUCCESS) {
			ppd_dev_err(ppd,
				    "Failed to transition to link up state, return 0x%x\n",
				    ret1);
			ret = -EINVAL;
			break;
		}
		ppd->host_link_state = HLS_GOING_UP;
#ifdef CONFIG_HFI2_STLNP
		hfi_timesync_init(ppd);
#endif
		break;
	case HLS_GOING_OFFLINE:		/* transient within goto_offline() */
	case HLS_LINK_COOLDOWN:		/* transient within goto_offline() */
	default:
		ppd_dev_info(ppd, "%s(): state: %s\n",
			     __func__, link_state_name(state));
		ret = -EINVAL;
	}

	goto _return;

unexpected:
	ppd_dev_err(ppd, "port%d %s: unexpected state transition from %s to %s\n",
		    ppd->pnum, __func__, link_state_name(ppd->host_link_state),
		link_state_name(state));
	ret = -EINVAL;

_return:
	mutex_unlock(&ppd->hls_lock);

	if (event.device)
		ib_dispatch_event(&event);

	return ret;
}

/*
 * Start the link. Schedule a retry if the cable is not
 * present or if unable to start polling. Do not do anything if the
 * link is disabled.  Returns 0 if link is disabled or moved to polling
 */
int hfi_start_link(struct hfi_pportdata *ppd)
{
	/*
	 * Tune the SerDes to a ballpark setting for optimal signal and
	 * bit error rate. Needs to be done before starting the link.
	 */
	tune_serdes(ppd);
#if 0 /* WFR legacy */
	if (!ppd->link_enabled) {
		dd_dev_info(ppd->dd,
			    "%s: stopping link start because link is disabled\n",
			    __func__);
		return 0;
	}

	if (!ppd->driver_link_ready) {
		dd_dev_info(ppd->dd,
			    "%s: stopping link start because driver is not ready\n",
			    __func__);
		return 0;
	}

	/*
	 * FULL_MGMT_P_KEY is cleared from the pkey table, so that the
	 * pkey table can be configured properly if the HFI unit is
	 * connected to switch port with MgmtAllowed=NO
	 */
	hfi_clear_full_mgmt_pkey(ppd);

	if (qsfp_mod_present(ppd) || loopback == LOOPBACK_SERDES ||
	    loopback == LOOPBACK_LCB ||
	    ppd->dd->icode == ICODE_FUNCTIONAL_SIMULATOR)
#endif
		return hfi_set_link_state(ppd, HLS_DN_POLL);

#if 0 /* WFR legacy */
	dd_dev_info(ppd->dd,
		    "%s: stopping link start because no cable is present\n",
		    __func__);
	return -EAGAIN;
#endif
}

/*
 * Try a QSFP read.  If it fails, schedule a retry for later.
 * Called on first link activation after driver load.
 */
void try_start_link(struct hfi_pportdata *ppd)
{
	if (hfi_test_qsfp_read(ppd)) {
		/* read failed */
		if (ppd->qsfp_retry_count >= MAX_QSFP_RETRIES) {
			dd_dev_err(ppd->dd,
				   "QSFP not responding, giving up\n");
			return;
		}
		dd_dev_info(ppd->dd,
			    "QSFP not responding, waiting and retrying %d\n",
			    (int)ppd->qsfp_retry_count);
		ppd->qsfp_retry_count++;
		queue_delayed_work(ppd->hfi_wq, &ppd->start_link_work,
				   msecs_to_jiffies(QSFP_RETRY_WAIT));
		return;
	}
	ppd->qsfp_retry_count = 0;

	hfi_start_link(ppd);
}

/*
 * Workqueue function to start the link after a delay.
 */
void handle_start_link(struct work_struct *work)
{
	struct hfi_pportdata *ppd = container_of(work, struct hfi_pportdata,
						 start_link_work.work);
	try_start_link(ppd);
}
