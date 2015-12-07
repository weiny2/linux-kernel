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
#include <rdma/fxr/fxr_fc_defs.h>
#include <rdma/fxr/fxr_top_defs.h>
#include <rdma/fxr/mnh_8051_defs.h>
#include <linux/interrupt.h>
#include "opa_hfi.h"
#include <rdma/opa_core_ib.h>
#include "link.h"
#include "firmware.h"

#define WAIT_TILL_8051_LINKUP 1000

static bool quick_linkup = false; /* skip VerifyCap and Config* state. */

static void handle_linkup_change(struct hfi_pportdata *ppd, u32 linkup);
static u32 read_physical_state(const struct hfi_pportdata *ppd);
static int set_physical_link_state(struct hfi_pportdata *ppd, u64 state);
static void get_linkup_link_widths(struct hfi_pportdata *ppd);
static int load_8051_config(struct hfi_pportdata *ppd, u8 field_id,
			    u8 lane_id, u32 config_data);
/*
 * read FZC registers
 */
u64 read_fzc_csr(const struct hfi_pportdata *ppd, u32 offset)
{
	switch (ppd->pnum) {
	case 1:
		offset += FXR_FZC_LCB0_CSRS; break;
	case 2:
		offset += FXR_FZC_LCB1_CSRS; break;
	default:
		ppd_dev_warn(ppd, "invalid port"); break;
	}
	return read_csr(ppd->dd, offset);
}

/*
 * write FZC registers
 */
void write_fzc_csr(const struct hfi_pportdata *ppd, u32 offset, u64 value)
{
	switch (ppd->pnum) {
	case 1:
		offset += FXR_FZC_LCB0_CSRS; break;
	case 2:
		offset += FXR_FZC_LCB1_CSRS; break;
	default:
		ppd_dev_warn(ppd, "invalid port"); break;
	}
	write_csr(ppd->dd, offset, value);
}

/*
 * read 8051/MNH registers
 */
u64 read_8051_csr(const struct hfi_pportdata *ppd, u32 offset)
{
	switch (ppd->pnum) {
	case 1:
		offset += FXR_MNH_S0_8051_CSRS;
		break; /* nothing to do */
	case 2:
		offset += FXR_MNH_S1_8051_CSRS;
		break;
	default:
		ppd_dev_warn(ppd, "invalid port"); break;
	}
	return read_csr(ppd->dd, offset);
}

/*
 * write 8051/MNH registers
 */
void write_8051_csr(const struct hfi_pportdata *ppd, u32 offset, u64 value)
{
	switch (ppd->pnum) {
	case 1:
		offset += FXR_MNH_S0_8051_CSRS;
		break; /* nothing to do */
	case 2:
		offset += FXR_MNH_S1_8051_CSRS;
		break;
	default:
		ppd_dev_warn(ppd, "invalid port"); break;
	}
	write_csr(ppd->dd, offset, value);
}

static void read_mgmt_allowed(struct hfi_pportdata *ppd, u8 *mgmt_allowed)
{
	u32 frame;

	hfi2_read_8051_config(ppd, REMOTE_LNI_INFO, GENERAL_CONFIG, &frame);
	*mgmt_allowed = (frame >> MGMT_ALLOWED_SHIFT) & MGMT_ALLOWED_MASK;
}

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
		ppd_dev_warn(ppd, "Invalid lcb crc. Driver might be in bad state");
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

u32 hfi_to_opa_pstate(struct hfi_devdata *dd, u32 hfi_pstate)
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

u8 hfi_ibphys_portstate(struct hfi_pportdata *ppd)
{
	static u32 remembered_state = 0xff;
	u32 pstate;
	u32 ib_pstate;

	pstate = read_physical_state(ppd);
	ib_pstate = hfi_to_opa_pstate(ppd->dd, pstate);
	if (remembered_state != ib_pstate) {
		ppd_dev_info(ppd,
			"%s: physical state changed to %s (0x%x), phy 0x%x\n",
			__func__, opa_pstate_name(ib_pstate), ib_pstate,
			pstate);
		remembered_state = ib_pstate;
	}
	return ib_pstate;
}

/*
 * This routine should be called after the link has been transitioned to
 * OFFLINE (OFFLINE state has the side effect of putting the SerDes into
 * reset).
 *
 * The expectation is that the caller of this routine would have taken
 * care of properly transitioning the link into the correct state.
 */
static void mnh_shutdown(struct hfi_pportdata *ppd)
{
#if 0 /* WFR legacy */
	unsigned long flags;

	spin_lock_irqsave(&ppd->crk8051_lock, flags);
	if (dd->dc_shutdown) {
		spin_unlock_irqrestore(&ppd->crk8051_lock, flags);
		return;
	}
	dd->dc_shutdown = 1;
	spin_unlock_irqrestore(&ppd->crk8051_lock, flags);
	/* Shutdown the LCB */
	lcb_shutdown(dd, 1);
#endif
	/* Going to OFFLINE would have causes the 8051 to put the
	 * SerDes into reset already. Just need to shut down the 8051,
	 * itself. */
	write_8051_csr(ppd, CRK_CRK8051_CFG_RST, 0x1);
}

/* Calling this after the MNH has been brought out of reset. */
static void mnh_start(const struct hfi_pportdata *ppd)
{
	int ret;
#if 0 /* WFR legacy */
	struct hfi_devdata *dd = ppd->dd;
	unsigned long flags;

	spin_lock_irqsave(&dd->crk8051_lock, flags);
	if (!dd->dc_shutdown)
		goto done;
	spin_unlock_irqrestore(&dd->crk8051_lock, flags);
	/* Take the 8051 out of reset */
#endif
	write_8051_csr(ppd, CRK_CRK8051_CFG_RST, 0ull);
	/* Wait until 8051 is ready */
	ret = hfi_wait_firmware_ready(ppd, TIMEOUT_8051_START);
	if (ret) {
		ppd_dev_err(ppd, "%s: timeout starting 8051 firmware\n",
			__func__);
	}
	/* turn on the LCB (turn off in lcb_shutdown). */
	write_fzc_csr(ppd, FZC_LCB_CFG_RUN, FZC_LCB_CFG_RUN_EN_MASK);
#if 0 /* WFR legacy */
	/* lcb_shutdown() with abort=1 does not restore these */
	write_csr(dd, DC_LCB_ERR_EN, dd->lcb_err_en);
	spin_lock_irqsave(&dd->dc8051_lock, flags);
	dd->dc_shutdown = 0;
done:
	spin_unlock_irqrestore(&dd->dc8051_lock, flags);
#endif
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
	u32 _8051_port = read_physical_state(ppd);

	/* transit to Init only from Going_Up. */
	if (ppd->host_link_state != HLS_GOING_UP) {
		ppd_dev_info(ppd, "False interrupt on %s(): %s(%d) 0x%x",
			__func__,
			link_state_name(ppd->host_link_state),
			ilog2(ppd->host_link_state), _8051_port);
		return;
	}
	hfi_set_link_state(ppd, HLS_UP_INIT);

#if 0 /* WFR legacy */
	/* cache the read of DC_LCB_STS_ROUND_TRIP_LTP_CNT */
	read_ltp_rtt(ppd->dd);
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
#if 0 /* WFR legacy */
	u8 lcl_reason, neigh_reason = 0;
#endif
	int ret;

	if (read_physical_state(ppd) == PLS_OFFLINE_READY_TO_QUIET_LT) {
		ret = set_physical_link_state(ppd, PLS_OFFLINE_QUIET);
		ret = ret != HCMD_SUCCESS ? -EINVAL: 0;
		if (ret)
			ppd_dev_err(ppd, "%s(): can't set physical port state to 0x%x",
				__func__, PLS_OFFLINE_QUIET);
	}

#if 0 /* WFR legacy */
	lcl_reason = 0;
	read_planned_down_reason_code(ppd->dd, &neigh_reason);

	/*
	 * If no reason, assume peer-initiated but missed
	 * LinkGoingDown idle flits.
	 */
	if (neigh_reason == 0)
		lcl_reason = OPA_LINKDOWN_REASON_NEIGHBOR_UNKNOWN;

	set_link_down_reason(ppd, lcl_reason, neigh_reason, 0);

	reset_neighbor_info(ppd);

	/* disable the port */
	clear_rcvctrl(ppd->dd, RCV_CTRL_RCV_PORT_ENABLE_SMASK);

	/* If there is no cable attached, turn the DC off. Otherwise,
	 * start the link bring up. */
	if (!qsfp_mod_present(ppd))
		dc_shutdown(ppd->dd);
	else
		start_link(ppd);
#endif
}

static void set_logical_state(struct hfi_pportdata *ppd, u32 chip_lstate)
{
	u64 reg;

	reg = read_fzc_csr(ppd, FZC_LCB_CFG_PORT);
	/* clear current state, set new state */
	reg &= ~FZC_LCB_CFG_PORT_LINK_STATE_SMASK;
	reg |= (u64)chip_lstate << FZC_LCB_CFG_PORT_LINK_STATE_SHIFT;
	write_fzc_csr(ppd, FZC_LCB_CFG_PORT, reg);
}

/*
 * Returns:
 *	< 0 = Linux error, not able to get access
 *	> 0 = 8051 command RETURN_CODE
 */
static int do_8051_command(struct hfi_pportdata *ppd, u32 type,
	u64 in_data, u64 *out_data)
{
	u64 reg;
	int return_code = 0;
#if 0 /* WFR legacy */
	unsigned long flags;
#endif
	unsigned long timeout;
#if 0 /* WFR legacy */
	struct hfi_devdata *dd = ppd->dd;

	hfi1_cdbg(DC8051, "type %d, data 0x%012llx", type, in_data);

	/*
	 * Alternative to holding the lock for a long time:
	 * - keep busy wait - have other users bounce off
	 */
	spin_lock_irqsave(&ppd->crk8051_lock, flags);

	/* We can't send any commands to the 8051 if it's in reset */
	if (dd->dc_shutdown) {
		return_code = -ENODEV;
		goto fail;
	}
#endif

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
#if 0 /* WFR legacy */
		spin_unlock_irqrestore(&ppd->crk8051_lock, flags);
#endif
		mnh_shutdown(ppd);
		mnh_start(ppd);
#if 0 /* WFR legacy */
		spin_lock_irqsave(&ppd->crk8051_lock, flags);
#endif
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
	write_8051_csr(ppd, CRK_CRK8051_CFG_HOST_CMD_0, reg);
	reg |= CRK_CRK8051_CFG_HOST_CMD_0_REQ_NEW_SMASK;
	write_8051_csr(ppd, CRK_CRK8051_CFG_HOST_CMD_0, reg);

	/* wait for completion, alternate: interrupt */
	timeout = jiffies + msecs_to_jiffies(CRK8051_COMMAND_TIMEOUT);
	while (1) {
		reg = read_8051_csr(ppd, CRK_CRK8051_CFG_HOST_CMD_1);
		if (reg & CRK_CRK8051_CFG_HOST_CMD_1_COMPLETED_SMASK)
			break;
		if (time_after(jiffies, timeout)) {
			ppd->crk8051_timed_out++;
			ppd_dev_err(ppd, "8051 host command %u timeout\n", type);
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
			*out_data |= (read_8051_csr(ppd, CRK_CRK8051_CFG_EXT_DEV_1)
				& CRK_CRK8051_CFG_EXT_DEV_1_REQ_DATA_SMASK)
				<< (48
				    - CRK_CRK8051_CFG_EXT_DEV_1_REQ_DATA_SHIFT);
		}
	}
	return_code = (reg >> CRK_CRK8051_CFG_HOST_CMD_1_RETURN_CODE_SHIFT)
				& CRK_CRK8051_CFG_HOST_CMD_1_RETURN_CODE_MASK;
	ppd->crk8051_timed_out = 0;
	/*
	 * Clear command for next user.
	 */
	write_8051_csr(ppd, CRK_CRK8051_CFG_HOST_CMD_0, 0);

fail:
#if 0 /* WFR legacy */
	spin_unlock_irqrestore(&ppd->crk8051_lock, flags);
#endif

	return return_code;
}

static int set_physical_link_state(struct hfi_pportdata *ppd, u64 state)
{
	return do_8051_command(ppd, HCMD_CHANGE_PHY_STATE, state, NULL);
}

static int load_8051_config(struct hfi_pportdata *ppd, u8 field_id,
			    u8 lane_id, u32 config_data)
{
	u64 data;
	int ret;

	data = (u64)field_id << LOAD_DATA_FIELD_ID_SHIFT
		| (u64)lane_id << LOAD_DATA_LANE_ID_SHIFT
		| (u64)config_data << LOAD_DATA_DATA_SHIFT;
	ret = do_8051_command(ppd, HCMD_LOAD_CONFIG_DATA, data, NULL);
	if (ret != HCMD_SUCCESS) {
		ppd_dev_err(ppd,
			"load 8051 config: field id %d, lane %d, err %d\n",
			(int)field_id, (int)lane_id, ret);
	}
	return ret;
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
	return load_8051_config(ppd, VERIFY_CAP_LOCAL_LINK_WIDTH, GENERAL_CONFIG,
		     frame);
}

static void read_vc_remote_link_width(struct hfi_pportdata *ppd,
				      u8 *remote_tx_rate,
				      u16 *link_widths)
{
	u32 frame;

	hfi2_read_8051_config(ppd, VERIFY_CAP_REMOTE_LINK_WIDTH, GENERAL_CONFIG,
			 &frame);
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

	ppd_dev_info(ppd, "%s: sending idle message 0x%llx\n", __func__, data);
	ret = do_8051_command(ppd, HCMD_SEND_LCB_IDLE_MSG, data, NULL);
	if (ret != HCMD_SUCCESS) {
		ppd_dev_err(ppd, "send idle message: data 0x%llx, err %d\n",
			data, ret);
		return -EINVAL;
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

	lcb_shutdown(dd, 0);

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

		if (ret >= 0)
			ret = -EINVAL;
#endif
		return ret;
	}

	return 0; /* success */
}

static u32 read_physical_state(const struct hfi_pportdata *ppd)
{
	u64 reg;

	reg = read_8051_csr(ppd, CRK_CRK8051_STS_CUR_STATE);
	return (reg >> CRK_CRK8051_STS_CUR_STATE_PORT_SHIFT)
				& CRK_CRK8051_STS_CUR_STATE_PORT_MASK;
}

static u32 read_logical_state(const struct hfi_pportdata *ppd)
{
	u64 reg;

	reg = read_fzc_csr(ppd, FZC_LCB_CFG_PORT);
	return (reg >> FZC_LCB_CFG_PORT_LINK_STATE_SHIFT)
				& FZC_LCB_CFG_PORT_LINK_STATE_MASK;
}

static int wait_phy_linkstate(struct hfi_pportdata *ppd, u32 state, u32 msecs)
{
	unsigned long timeout;
	u32 curr_state;

	timeout = jiffies + msecs_to_jiffies(msecs);
	while (1) {
		curr_state = read_physical_state(ppd);
		if (curr_state == state)
			break;
		if (time_after(jiffies, timeout)) {
			ppd_dev_err(ppd,
				"timeout waiting for phy link state 0x%x, current state is 0x%x\n",
				state, curr_state);
			return -ETIMEDOUT;
		}
		usleep_range(1950, 2050); /* sleep 2ms-ish */
	}

	return 0;
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
	u32 pstate, previous_state;
	int do_transition;
	int do_wait;
	int ret;
#if 0 /* WFR legacy */
	u32 last_local_state;
	u32 last_remote_state;
#endif

	previous_state = ppd->host_link_state;
	ppd->host_link_state = HLS_GOING_OFFLINE;
	pstate = read_physical_state(ppd);
	if (pstate == PLS_OFFLINE) {
		do_transition = 0;	/* in right state */
		do_wait = 0;		/* ...no need to wait */
	} else if ((pstate & 0xff) == PLS_OFFLINE) {
		do_transition = 0;	/* in an offline transient state */
		do_wait = 1;		/* ...wait for it to settle */
	} else {
		do_transition = 1;	/* need to move to offline */
		do_wait = 1;		/* ...will need to wait */
	}

	if (do_transition) {
		ret = set_physical_link_state(ppd,
			PLS_OFFLINE | (rem_reason << 8));

		if (ret != HCMD_SUCCESS) {
			ppd_dev_err(ppd,
				"Failed to transition to Offline link state, return %d\n",
				ret);
			return -EINVAL;
		}
#if 0 /* WFR legacy */
		if (ppd->offline_disabled_reason == OPA_LINKDOWN_REASON_NONE)
			ppd->offline_disabled_reason =
			OPA_LINKDOWN_REASON_TRANSIENT;
#endif
	}

	if (do_wait) {
		/* it can take a while for the link to go down */
		ret = wait_phy_linkstate(ppd, PLS_OFFLINE, 10000);
		if (ret < 0)
			return ret;
	}

	/* make sure the logical state is also down */
	hfi2_wait_logical_linkstate(ppd, IB_PORT_DOWN, 1000);

	/*
	 * Now in charge of LCB - must be after the physical state is
	 * offline.quiet and before host_link_state is changed.
	 */
	write_fzc_csr(ppd, FZC_LCB_ERR_FRC, ~0ull); /* watch LCB errors */
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
#if 0 /* WFR legacy */
	if (previous_state & HLS_UP) {
		/* went down while link was up */
		handle_linkup_change(ppd, 0);
	} else if (previous_state
			& (HLS_DN_POLL | HLS_VERIFY_CAP | HLS_GOING_UP)) {
		/* went down while attempting link up */
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
#endif
	int ret;

#if 0 /* WFR legacy */
	/* reset our fabric serdes to clear any lingering problems */
	fabric_serdes_reset(dd);

	/* set the local tx rate - need to read-modify-write */
	ret = read_tx_settings(dd, &enable_lane_tx, &tx_polarity_inversion,
		&rx_polarity_inversion, &ppd->local_tx_rate);
	if (ret)
		goto set_local_link_attributes_fail;
#endif

	/* set the tx rate to all enabled */
	ppd->local_tx_rate = 0;
	switch (ppd->link_speed_enabled) {
	case OPA_LINK_SPEED_32G: /* fall through */
		ppd->local_tx_rate |= HFI2_LINK_SPEED_32G;
	case OPA_LINK_SPEED_25G: /* fall through */
		ppd->local_tx_rate |= HFI2_LINK_SPEED_25G;
	case OPA_LINK_SPEED_12_5G:
		ppd->local_tx_rate |= HFI2_LINK_SPEED_12_5G;
		break;
	default:
		ppd_dev_err(ppd, "invalid link_speed_enabled: %d",
			ppd->link_speed_enabled);
		return -EINVAL;
	}

#if 0 /* WFR legacy */
	enable_lane_tx = 0xF; /* enable all four lanes */
	ret = write_tx_settings(dd, enable_lane_tx, tx_polarity_inversion,
		     rx_polarity_inversion, ppd->local_tx_rate);
	if (ret != HCMD_SUCCESS)
		goto set_local_link_attributes_fail;

	/*
	 * DC supports continuous updates.
	 */
	ret = write_vc_local_phy(dd, 0 /* no power management */,
				     1 /* continuous updates */);
	if (ret != HCMD_SUCCESS)
		goto set_local_link_attributes_fail;
#endif
	write_8051_csr(ppd, CRK_CRK8051_CFG_LOCAL_PORT_NO,
		       ppd->pnum & CRK_CRK8051_CFG_LOCAL_PORT_NO_VAL_MASK);

	/* z=1 in the next call: AU of 0 is not supported by the hardware */
	ret = write_vc_local_fabric(ppd, ppd->vau, 1, ppd->vcu, ppd->vl15_init,
				    ppd->port_crc_mode_enabled);
	if (ret != HCMD_SUCCESS)
		goto set_local_link_attributes_fail;

	/*
	 * write desired link width policy.
	 * See "Initial Link Width Policy Configuration" of MNH HAS.
	 */
	ret = write_vc_local_link_width(ppd, 0, 0,
		     opa_to_vc_link_widths(ppd->link_width_enabled));
	if (ret != HCMD_SUCCESS)
		goto set_local_link_attributes_fail;

	/* let peer know who we are */
	ret = write_local_device_id(ppd, dd->pcidev->device, dd->minrev);
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

#if 0
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
#endif

/*
 * Read the hardware link state and set the driver's cached value of it.
 * Return the (new) current value.
 */
static u32 get_logical_state(struct hfi_pportdata *ppd)
{
	u32 new_state;

	new_state = chip_to_opa_lstate(ppd->dd, read_logical_state(ppd));
	if (new_state != ppd->lstate) {
#if 0
		dd_dev_info(ppd->dd, "logical link state: %s(%d) -> %s(%d)\n",
			opa_lstate_name(ppd->lstate), ppd->lstate,
			opa_lstate_name(new_state), new_state);
#endif
		ppd->lstate = new_state;
	}
#if 0 /* WFR legacy */
	/*
	 * Set port status flags in the page mapped into userspace
	 * memory. Do it here to ensure a reliable state - this is
	 * the only function called by all state handling code.
	 * Always set the flags due to the fact that the cache value
	 * might have been changed explicitly outside of this
	 * function.
	 */
	if (ppd->statusp) {
		switch (ppd->lstate) {
		case IB_PORT_DOWN:
		case IB_PORT_INIT:
			*ppd->statusp &= ~(HFI1_STATUS_IB_CONF |
					   HFI_STATUS_IB_READY);
			break;
		case IB_PORT_ARMED:
			*ppd->statusp |= HFI_STATUS_IB_CONF;
			break;
		case IB_PORT_ACTIVE:
			*ppd->statusp |= HFI_STATUS_IB_READY;
			break;
		}
	}
#endif
	return ppd->lstate;
}

/**
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
	u32 cur_state;

	timeout = jiffies + msecs_to_jiffies(msecs);
	while (1) {
		cur_state = get_logical_state(ppd);
		if (cur_state == state)
			return 0;
		if (time_after(jiffies, timeout))
			break;
		msleep(20);
	}
	ppd_dev_err(ppd, "timeout waiting for link state 0x%x (0x%x)\n",
		   state, cur_state);

	return -ETIMEDOUT;
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

	/* msg is bytes 1-4 of the 40-bit idle message - the command code
	   is stripped off */
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
		if (ppd->host_link_state == HLS_UP_ARMED
					&& ppd->is_active_optimize_enabled) {
			ppd->neighbor_normal = 1;
			ret = hfi_set_link_state(ppd, HLS_UP_ACTIVE);
			if (ret)
				ppd_dev_err(ppd,
					"%s: received Active SMA idle message, couldn't set link to Active\n",
					__func__);
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
	ppd->neighbor_guid = read_8051_csr(ppd, CRK_CRK8051_STS_REMOTE_GUID);
	ppd->neighbor_type =
		read_8051_csr(ppd,
			      CRK_CRK8051_STS_REMOTE_NODE_TYPE) &
			      CRK_CRK8051_STS_REMOTE_NODE_TYPE_VAL_SMASK;
	ppd->neighbor_port_number =
		read_8051_csr(ppd,
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
#if 1
		if (quick_linkup) {
#else /* WFR legacy */
		if (quick_linkup
			    || dd->icode == ICODE_FUNCTIONAL_SIMULATOR) {
			set_up_vl15(dd, dd->vau, dd->vl15_init);
			assign_remote_cm_au_table(dd, dd->vcu);
#endif
			handle_quick_linkup(ppd);
		}

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
#if 0 /* WFR legacy */
		/* physical link went down */
		ppd->linkup = 0;

		/* clear HW details of the previous connection */
		reset_link_credits(dd);

		/* freeze after a link down to guarantee a clean egress */
		start_freeze_handling(ppd, FREEZE_SELF|FREEZE_LINK_DOWN);
#endif

		ev = IB_EVENT_PORT_ERR;

#if 0 /* WFR legacy */
		hfi1_set_uevent_bits(ppd, _HFI1_EVENT_LINKDOWN_BIT);

		/* if we are down, the neighbor is down */
		ppd->neighbor_normal = 0;

		/* notify IB of the link change */
		signal_ib_event(ppd, ev);
#endif
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
void get_linkup_link_widths(struct hfi_pportdata *ppd)
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
	lcb_shutdown(dd, 0);
	adjust_lcb_for_fpga_serdes(dd);

	/*
	 * These are now valid:
	 *	remote VerifyCap fields in the general LNI config
	 *	CSR DC8051_STS_REMOTE_GUID
	 *	CSR DC8051_STS_REMOTE_NODE_TYPE
	 *	CSR DC8051_STS_REMOTE_FM_SECURITY
	 *	CSR DC8051_STS_REMOTE_PORT_NO
	 */

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

	/*
	 * And the 'MgmtAllowed' information, which is exchanged during
	 * LNI, is also be available at this point.
	 */
	read_mgmt_allowed(ppd, &ppd->mgmt_allowed);
#if 0
	/* print the active widths */
	get_link_widths(dd, &active_tx, &active_rx);
	ppd_dev_info(ppd,
		"Peer PHY: power management 0x%x, continuous updates 0x%x\n",
		(int)power_management, (int)continious);
#endif
	ppd_dev_info(ppd,
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
	/*
	 * The peer vAU value just read is the peer receiver value.  HFI does
	 * not support a transmit vAU of 0 (AU == 8).  We advertised that
	 * with Z=1 in the fabric capabilities sent to the peer.  The peer
	 * will see our Z=1, and, if it advertised a vAU of 0, will move its
	 * receive to vAU of 1 (AU == 16).  Do the same here.  We do not care
	 * about the peer Z value - our sent vAU is 3 (hardwired) and is not
	 * subject to the Z value exception.
	 */
	if (vau == 0)
		vau = 1;
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
	write_fzc_csr(ppd, FZC_LCB_CFG_CRC_MODE,
		      (u64)crc_val << FZC_LCB_CFG_CRC_MODE_TX_VAL_SHIFT);
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
	case 0: /* 32G */
		rate |= HFI2_LINK_SPEED_32G; /* fall through */
	case 1: /* 25G */
		rate |= HFI2_LINK_SPEED_25G | HFI2_LINK_SPEED_12_5G;
		break;
	default:
		ppd_dev_err(ppd, "invalid remote max rate %d, set up to 32G",
			remote_max_rate);
		rate = HFI2_LINK_SPEED_32G | HFI2_LINK_SPEED_25G |
			HFI2_LINK_SPEED_12_5G; break;
		break;
	}
	rate &= ppd->local_tx_rate;
	if (rate & HFI2_LINK_SPEED_32G) {
		ppd->link_speed_active = OPA_LINK_SPEED_32G;
	} else if (rate & HFI2_LINK_SPEED_25G) {
		ppd->link_speed_active = OPA_LINK_SPEED_25G;
	} else if (rate & HFI2_LINK_SPEED_12_5G) {
		ppd->link_speed_active = OPA_LINK_SPEED_12_5G;
	} else {
		ppd_dev_err(ppd, "no common speed with remote: 0x%x, set up 32Gb\n",
			rate);
		ppd->link_speed_active = OPA_LINK_SPEED_32G;
	}

	/*
	 * Cache the values of the supported, enabled, and active
	 * LTP CRC modes to return in 'portinfo' queries. But the bit
	 * flags that are returned in the portinfo query differ from
	 * what's in the link_crc_mask, crc_sizes, and crc_val
	 * variables. Convert these here.
	 */
	ppd->port_ltp_crc_mode = hfi_lcb_to_port_ltp(ppd, HFI_SUPPORTED_CRCS) <<
			HFI_LTP_CRC_SUPPORTED_SHIFT;
		/* supported crc modes */
	ppd->port_ltp_crc_mode |=
		hfi_lcb_to_port_ltp(ppd, ppd->port_crc_mode_enabled) <<
		HFI_LTP_CRC_ENABLED_SHIFT;
		/* enabled crc modes */
	ppd->port_ltp_crc_mode |= hfi_lcb_to_port_ltp(ppd, crc_val);
		/* active crc mode */

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

	ppd->neighbor_guid =
		read_csr(dd, DC_DC8051_STS_REMOTE_GUID);
#endif
	ppd->neighbor_port_number =
		read_8051_csr(ppd, CRK_CRK8051_STS_REMOTE_PORT_NO) &
				CRK_CRK8051_STS_REMOTE_PORT_NO_VAL_SMASK;
	ppd->neighbor_type =
		read_8051_csr(ppd, CRK_CRK8051_STS_REMOTE_NODE_TYPE) &
			      CRK_CRK8051_STS_REMOTE_NODE_TYPE_VAL_SMASK;
	ppd->neighbor_fm_security =
		read_8051_csr(ppd, CRK_CRK8051_STS_REMOTE_FM_SECURITY) &
		CRK_CRK8051_STS_REMOTE_FM_SECURITY_DISABLED_SMASK;
#if 0
	ppd_dev_info(ppd,
		"Neighbor Guid: %llx Neighbor type %d MgmtAllowed %d FM security bypass %d\n",
		ppd->neighbor_guid, ppd->neighbor_type,
		ppd->mgmt_allowed, ppd->neighbor_fm_security);
#endif
	if (ppd->mgmt_allowed)
		hfi_add_full_mgmt_pkey(ppd);

	/* tell the 8051 to go to LinkUp */
	hfi_set_link_state(ppd, HLS_GOING_UP);
}

static irqreturn_t irq_mnh_handler(int irq, void *dev_id)
{
	struct hfi_msix_entry *me = dev_id;
	struct hfi_devdata *dd = me->dd;
	struct hfi_pportdata *ppd;
	u64 reg;
	u8 port;

	hfi_ack_interrupt(me);

	for (port = 1; port <= dd->num_pports; port++) {
		ppd = to_hfi_ppd(dd, port);
		reg = read_8051_csr(ppd, CRK_CRK8051_DBG_ERR_INFO_SET_BY_8051);
		reg >>= CRK_CRK8051_DBG_ERR_INFO_SET_BY_8051_HOST_MSG_SHIFT;
		if (reg & VERIFY_CAP_FRAME)
			queue_work(ppd->hfi_wq, &ppd->link_vc_work);
		if (reg & LINKUP_ACHIEVED)
			queue_work(ppd->hfi_wq, &ppd->link_up_work);
		if (reg & LINK_GOING_DOWN)
			queue_work(ppd->hfi_wq, &ppd->link_down_work);
		if (reg & BC_SMA_MSG)
			queue_work(ppd->hfi_wq, &ppd->sma_message_work);
		/* TODO: take care other interrupts */

		if (reg & CRK_CRK8051_DBG_ERR_INFO_SET_BY_8051_HOST_MSG_SMASK) {
			reg = read_8051_csr(ppd, CRK_CRK8051_ERR_CLR);
			write_8051_csr(ppd, CRK_CRK8051_ERR_CLR,
				reg | CRK_CRK8051_ERR_CLR_SET_BY_8051_SMASK);
		}
	}

	return IRQ_HANDLED;
}

/*
 * configure IRQs for MNH
*/
int hfi2_cfg_link_intr_vector(struct hfi_devdata *dd)
{
	struct hfi_msix_entry *me = &dd->msix_entries[HFI2_MNH_ERROR];
	int ret;

	if (me->arg != NULL) {
		dd_dev_err(dd, "MSIX entry is already configured: %d\n",
			HFI2_MNH_ERROR);
		ret = -EINVAL;
		goto _return;
	}
	INIT_LIST_HEAD(&me->irq_wait_head);
	rwlock_init(&me->irq_wait_lock);
	me->dd = dd;
	me->intr_src = HFI2_MNH_ERROR;

	dd_dev_dbg(dd, "request for IRQ %d:%d\n", HFI2_MNH_ERROR, me->msix.vector);
	ret = request_irq(me->msix.vector, irq_mnh_handler, 0,
		"hfi_irq_mnh", me);
	if (ret) {
		dd_dev_err(dd, "IRQ[%d] request failed %d\n", HFI2_MNH_ERROR, ret);
		goto _return;
	}
	me->arg = me;	/* mark as in use */
 _return:
	return ret;
}

/*
 * Enable interrupots from MNH/8051 by configuring its registers
*/
int hfi2_enable_8051_intr(struct hfi_pportdata *ppd)
{
	int ret;
	u64 reg;

	/* disable 8051 command complete interrupt and enable all others */
	ret = load_8051_config(ppd, HOST_INT_MSG_MASK, GENERAL_CONFIG,
		(BC_PWR_MGM_MSG | BC_SMA_MSG | BC_BCC_UNKOWN_MSG |
		 BC_IDLE_UNKNOWN_MSG | EXT_DEVICE_CFG_REQ | VERIFY_CAP_FRAME |
		 LINKUP_ACHIEVED | LINK_GOING_DOWN | LINK_WIDTH_DOWNGRADED) << 8);
	ret = ret != HCMD_SUCCESS ? -EINVAL: 0;
	if (ret)
		goto _return;

	/* enable 8051 interrupt */
	reg = read_8051_csr(ppd, CRK_CRK8051_ERR_EN_HOST);
	write_8051_csr(ppd, CRK_CRK8051_ERR_EN_HOST, reg |
		CRK_CRK8051_ERR_EN_HOST_SET_BY_8051_SMASK);
	reg = read_8051_csr(ppd, CRK_CRK8051_ERR_CLR);
	write_8051_csr(ppd,
		CRK_CRK8051_ERR_CLR, reg | CRK_CRK8051_ERR_CLR_SET_BY_8051_SMASK);
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
	ret = ret != HCMD_SUCCESS ? -EINVAL: 0;
	if (ret)
		goto _return;

	/* disable 8051 interrupt */
	reg = read_8051_csr(ppd, CRK_CRK8051_ERR_EN_HOST);
	write_8051_csr(ppd, CRK_CRK8051_ERR_EN_HOST, reg &
		~CRK_CRK8051_ERR_EN_HOST_SET_BY_8051_SMASK);
 _return:
	return ret;
}

/*
	un-initialize ports
 */
void hfi2_pport_link_uninit(struct hfi_devdata *dd)
{
	u8 port;
	struct hfi_pportdata *ppd;
	int ret;

	/*
	 * Make both port1 and 2 to Offline state then disable both ports'
	 * interrupt. Otherwise irq_mnh_handle() produces spurious interrupt.
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
		if (ppd->hfi_wq) {
			destroy_workqueue(ppd->hfi_wq);
			ppd->hfi_wq = NULL;
		}
	}
}

/*
	initialize ports
 */
int hfi2_pport_link_init(struct hfi_devdata *dd)
{
	int ret;
	u8 port;
	struct hfi_pportdata *ppd;

	ret = hfi2_cfg_link_intr_vector(dd);
	if (ret) {
		dd_dev_err(dd, "Can't configure interrupt vector of MNH/8051: %d\n",
			ret);
		goto _return;
	}

	for (port = 1; port <= dd->num_pports; port++) {
		ppd = to_hfi_ppd(dd, port);
		/* configure workqueues */
		INIT_WORK(&ppd->link_vc_work, handle_verify_cap);
		INIT_WORK(&ppd->link_up_work, handle_link_up);
		INIT_WORK(&ppd->link_down_work, handle_link_down);
		INIT_WORK(&ppd->sma_message_work, handle_sma_message);
		/* TODO: adjust 0 for max_active */
		ppd->hfi_wq = alloc_workqueue("hfi2_%d_%d",
			WQ_SYSFS | WQ_HIGHPRI | WQ_CPU_INTENSIVE, 0, dd->unit, port);
		if (!ppd->hfi_wq) {
			dd_dev_err(dd, "Can't allocate workqueue\n");
			ret = -ENOMEM;
			goto _return;
		}

		 /* configure interrupt registers of MNH/8051 */
		ret = hfi2_enable_8051_intr(ppd);
		if (ret) {
			dd_dev_err(dd, "Can't configure interrupt registers of MNH/8051: %d\n", ret);
			goto _return;
		}
	}
	/*
	 * The following code has to be separated from above "for" loop because
	 * hfi_start_link(ppd) induces VerifyCap interrupt and its interrupt
	 * service routine,irq_mnh_handler() scans both ports.
	 */
	for (port = 1; port <= dd->num_pports; port++) {
		ppd = to_hfi_ppd(dd, port);
		hfi_start_link(ppd);
		if (opafm_disable) {
			ret = hfi2_wait_logical_linkstate(ppd, IB_PORT_INIT,
				WAIT_TILL_8051_LINKUP);
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

	mutex_lock(&ppd->hls_lock);

	ppd_dev_info(ppd, "%s(%d) -> %s(%d)\n",
		link_state_name(ppd->host_link_state), ilog2(ppd->host_link_state),
		link_state_name(state), ilog2(state));

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
			mnh_start(ppd);

		/* allow any state to transition to offline */
		ret = goto_offline(ppd, ppd->remote_link_down_reason);
		if (!ret)
			ppd->remote_link_down_reason = 0;
		opa_core_notify_clients(ppd->dd->bus_dev,
					OPA_LINK_STATE_CHANGE, ppd->pnum);
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
		ppd->host_link_state = HLS_DN_DISABLE;
		mnh_shutdown(ppd);
		ppd->lstate = IB_PORT_DOWN;
		opa_core_notify_clients(ppd->dd->bus_dev,
					OPA_LINK_STATE_CHANGE, ppd->pnum);
		break;
	case HLS_DN_DOWNDEF:
		ppd->host_link_state = HLS_DN_DOWNDEF;
		ppd->lstate = IB_PORT_DOWN;
		opa_core_notify_clients(ppd->dd->bus_dev,
					OPA_LINK_STATE_CHANGE, ppd->pnum);
		break;
	case HLS_DN_POLL:
		if ((ppd->host_link_state == HLS_DN_DISABLE ||
		     ppd->host_link_state == HLS_DN_OFFLINE))
			mnh_start(ppd);
#if 0 /* WFR legacy */
		/* Hand LED control to the DC */
		write_csr(dd, CRK_CFG_LED_CNTRL, 0);
#endif

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
		   fxr_simics_4_8_35 implements both quick_linkup and full linkup
		 */
		if (quick_linkup) {
			ret = do_quick_linkup(ppd);
#if 1 /* new on FXR */
			ppd->host_link_state = HLS_UP_INIT;
			ppd->lstate = IB_PORT_INIT;
#endif
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

		ppd->host_link_state = HLS_UP_INIT;
		ret = hfi2_wait_logical_linkstate(ppd, IB_PORT_INIT, 1000);
		if (ret) {
			/* logical state didn't change, stay at going_up */
			ppd->host_link_state = HLS_GOING_UP;
			ppd_dev_err(ppd,
				"%s: logical state did not change to INIT\n",
				__func__);
		} else {
			/* enable the port */
#if 0 /* where RCV_CTRL_RCV_PORT_ENABLE_SMASK is defined */
			add_rcvctrl(dd, RCV_CTRL_RCV_PORT_ENABLE_SMASK);
#endif
			handle_linkup_change(ppd, 1);
		}
		ppd->lstate = IB_PORT_INIT;
		break;
	case HLS_UP_ARMED:
		if (ppd->host_link_state != HLS_UP_INIT)
			goto unexpected;

		ppd->host_link_state = HLS_UP_ARMED;
		set_logical_state(ppd, LSTATE_ARMED);

		ret = hfi2_wait_logical_linkstate(ppd, IB_PORT_ARMED, 1000);
		if (ret) {
			/* logical state didn't change, stay at init */
			ppd->host_link_state = HLS_UP_INIT;
			ppd_dev_err(ppd,
				"%s: logical state did not change to ARMED\n",
				__func__);
		}
		ppd->lstate = IB_PORT_ARMED;
		break;
	case HLS_UP_ACTIVE:
		if (ppd->host_link_state != HLS_UP_ARMED)
			goto unexpected;

		ppd->host_link_state = HLS_UP_ACTIVE;
		set_logical_state(ppd, LSTATE_ACTIVE);
		ret = hfi2_wait_logical_linkstate(ppd, IB_PORT_ACTIVE, 1000);
		if (ret) {
			/* logical state didn't change, stay at armed */
			ppd->host_link_state = HLS_UP_ARMED;
			ppd_dev_err(ppd,
				"%s: logical state did not change to ACTIVE\n",
				__func__);
#if 0 /* WFR legacy */
		} else {

			/* tell all engines to go running */
			sdma_all_running(dd);

			/* Signal the IB layer that the port has went active */
			event.device = &dd->verbs_dev.ibdev;
			event.element.port_num = ppd->port;
			event.event = IB_EVENT_PORT_ACTIVE;
#endif
		}
		ppd->lstate = IB_PORT_ACTIVE;
		opa_core_notify_clients(ppd->dd->bus_dev,
					OPA_LINK_STATE_CHANGE, ppd->pnum);
		break;
	case HLS_VERIFY_CAP:
		if (ppd->host_link_state != HLS_DN_POLL)
			goto unexpected;
		ppd->host_link_state = HLS_VERIFY_CAP;
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

	return ret;
}

/*
 * Start the link. Schedule a retry if the cable is not
 * present or if unable to start polling. Do not do anything if the
 * link is disabled.  Returns 0 if link is disabled or moved to polling
 */
int hfi_start_link(struct hfi_pportdata *ppd)
{
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
