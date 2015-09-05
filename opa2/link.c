/*
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright(c) 2015 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * BSD LICENSE
 *
 * Copyright(c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Intel Corporation nor the names of its
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
 *
 */

/*
 * This file contains all of the code that is specific to
 * Link Negotiation and Initialization(LNI)
 */

#include <linux/delay.h>
#include <rdma/fxr/dcc_csrs_defs.h>
#include <rdma/fxr/fxr_fc_defs.h>
#include <rdma/fxr/fxr_top_defs.h>
#include <rdma/fxr/dc_8051_csrs_defs.h>
#include "opa_hfi.h"
#include "link.h"
#include "firmware.h"

bool quick_linkup = true; /* skip LNI. */

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
		dd_dev_warn(ppd->dd, "invalid port"); break;
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
		dd_dev_warn(ppd->dd, "invalid port"); break;
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
		break; /* nothing to do */
	case 2:
		offset += FXR_MNH_S1_8051_CSRS - FXR_MNH_S0_8051_CSRS;
		break;
	default:
		dd_dev_warn(ppd->dd, "invalid port"); break;
	}
	return read_csr(ppd->dd, offset);
}

/*
 * write 8051/MNH registers
 */
void write_8051_csr(const struct hfi_pportdata *ppd, u32 offset,	u64 value)
{
	switch (ppd->pnum) {
	case 1:
		break; /* nothing to do */
	case 2:
		offset += FXR_MNH_S1_8051_CSRS - FXR_MNH_S0_8051_CSRS;
		break;
	default:
		dd_dev_warn(ppd->dd, "invalid port"); break;
	}
	write_csr(ppd->dd, offset, value);
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
	write_8051_csr(ppd, CRK8051_CFG_RST, 0x1);
}

/* Calling this after the MNH has been brought out of reset. */
static void mnh_start(const struct hfi_pportdata *ppd)
{
	int ret;
	struct hfi_devdata *dd = ppd->dd;
#if 0 /* WFR legacy */
	unsigned long flags;

	spin_lock_irqsave(&dd->crk8051_lock, flags);
	if (!dd->dc_shutdown)
		goto done;
	spin_unlock_irqrestore(&dd->crk8051_lock, flags);
	/* Take the 8051 out of reset */
#endif
	write_8051_csr(ppd, CRK8051_CFG_RST, 0ull);
	/* Wait until 8051 is ready */
	ret = hfi_wait_firmware_ready(ppd, TIMEOUT_8051_START);
	if (ret) {
		dd_dev_err(dd, "%s: timeout starting 8051 firmware\n",
			__func__);
	}
	/* Take away reset for LCB and RX FPE (set in lcb_shutdown). */
	write_csr(dd, CRK_CFG_RESET, 0x10);
#if 0 /* WFR legacy */
	/* lcb_shutdown() with abort=1 does not restore these */
	write_csr(dd, DC_LCB_ERR_EN, dd->lcb_err_en);
	spin_lock_irqsave(&dd->dc8051_lock, flags);
	dd->dc_shutdown = 0;
done:
	spin_unlock_irqrestore(&dd->dc8051_lock, flags);
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
	struct hfi_devdata *dd = ppd->dd;

#if 0 /* WFR legacy */
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
			dd_dev_err(dd,
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
	reg = ((u64)type & CRK8051_CFG_HOST_CMD_0_REQ_TYPE_MASK)
			<< CRK8051_CFG_HOST_CMD_0_REQ_TYPE_SHIFT
		| (in_data & CRK8051_CFG_HOST_CMD_0_REQ_DATA_MASK)
			<< CRK8051_CFG_HOST_CMD_0_REQ_DATA_SHIFT;
	write_8051_csr(ppd, CRK8051_CFG_HOST_CMD_0, reg);
	reg |= CRK8051_CFG_HOST_CMD_0_REQ_NEW_SMASK;
	write_8051_csr(ppd, CRK8051_CFG_HOST_CMD_0, reg);

	/* wait for completion, alternate: interrupt */
	timeout = jiffies + msecs_to_jiffies(CRK8051_COMMAND_TIMEOUT);
	while (1) {
		reg = read_8051_csr(ppd, CRK8051_CFG_HOST_CMD_1);
		if (reg & CRK8051_CFG_HOST_CMD_1_COMPLETED_SMASK)
			break;
		if (time_after(jiffies, timeout)) {
			ppd->crk8051_timed_out++;
			dd_dev_err(dd, "8051 host command %u timeout\n", type);
			if (out_data)
				*out_data = 0;
			return_code = -ETIMEDOUT;
			goto fail;
		}
		udelay(2);
	}

	if (out_data) {
		*out_data = (reg >> CRK8051_CFG_HOST_CMD_1_RSP_DATA_SHIFT)
				& CRK8051_CFG_HOST_CMD_1_RSP_DATA_MASK;
		if (type == HCMD_READ_LCB_CSR) {
			/* top 16 bits are in a different register */
			*out_data |= (read_8051_csr(ppd, CRK8051_CFG_EXT_DEV_1)
				& CRK8051_CFG_EXT_DEV_1_REQ_DATA_SMASK)
				<< (48
				    - CRK8051_CFG_EXT_DEV_1_REQ_DATA_SHIFT);
		}
	}
	return_code = (reg >> CRK8051_CFG_HOST_CMD_1_RETURN_CODE_SHIFT)
				& CRK8051_CFG_HOST_CMD_1_RETURN_CODE_MASK;
	ppd->crk8051_timed_out = 0;
	/*
	 * Clear command for next user.
	 */
	write_8051_csr(ppd, CRK8051_CFG_HOST_CMD_0, 0);

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

/*
 * Initialize the LCB then do a quick link up.  This may or may not be
 * in loopback.
 *
 * return 0 on success, -errno on error
 */
static int do_quick_linkup(struct hfi_pportdata *ppd)
{
	int ret;
	struct hfi_devdata *dd = ppd->dd;
#if 0 /* WFR legacy */
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
				dd_dev_err(dd,
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
		dd_dev_err(dd,
			"Pausing for peer to be finished with LCB set up\n");
		msleep(5000);
		dd_dev_err(dd,
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
		dd_dev_err(dd,
			"%s: set physical link state to quick LinkUp failed with return %d\n",
			__func__, ret);

#if 0 /* WFR legacy */
		set_host_lcb_access(dd);
		write_csr(dd, DC_LCB_ERR_EN, ~0ull); /* watch LCB errors */

		if (ret >= 0)
			ret = -EINVAL;
#endif
		return ret;
	}

	return 0; /* success */
}

static u32 read_logical_state(const struct hfi_pportdata *ppd)
{
	u64 reg;

	reg = read_fzc_csr(ppd, FZC_LCB_CFG_PORT);
	return (reg >> FZC_LCB_CFG_PORT_LINK_STATE_SHIFT)
				& FZC_LCB_CFG_PORT_LINK_STATE_MASK;
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
	struct hfi1_devdata *dd = ppd->dd;
	u32 pstate, previous_state;
	u32 last_local_state;
	u32 last_remote_state;
	int ret;
	int do_transition;
	int do_wait;

	previous_state = ppd->host_link_state;
	ppd->host_link_state = HLS_GOING_OFFLINE;
	pstate = read_physical_state(dd);
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
			dd_dev_err(dd,
				"Failed to transition to Offline link state, return %d\n",
				ret);
			return -EINVAL;
		}
		if (ppd->offline_disabled_reason == OPA_LINKDOWN_REASON_NONE)
			ppd->offline_disabled_reason =
			OPA_LINKDOWN_REASON_TRANSIENT;
	}

	if (do_wait) {
		/* it can take a while for the link to go down */
		ret = wait_phy_linkstate(dd, PLS_OFFLINE, 10000);
		if (ret < 0)
			return ret;
	}

	/* make sure the logical state is also down */
	wait_logical_linkstate(ppd, IB_PORT_DOWN, 1000);

	/*
	 * Now in charge of LCB - must be after the physical state is
	 * offline.quiet and before host_link_state is changed.
	 */
	set_host_lcb_access(dd);
	write_csr(dd, DC_LCB_ERR_EN, ~0ull); /* watch LCB errors */
	ppd->host_link_state = HLS_LINK_COOLDOWN; /* LCB access allowed */

	/*
	 * The LNI has a mandatory wait time after the physical state
	 * moves to Offline.Quiet.  The wait time may be different
	 * depending on how the link went down.  The 8051 firmware
	 * will observe the needed wait time and only move to ready
	 * when that is completed.  The largest of the quiet timeouts
	 * is 2.5s, so wait that long and then a bit more.
	 */
	ret = hfi_wait_firmware_ready(dd, 3000);
	if (ret) {
		dd_dev_err(dd,
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
	if (previous_state & HLS_UP) {
		/* went down while link was up */
		handle_linkup_change(ppd, 0);
	} else if (previous_state
			& (HLS_DN_POLL | HLS_VERIFY_CAP | HLS_GOING_UP)) {
		/* went down while attempting link up */
		/* byte 1 of last_*_state is the failure reason */
		read_last_local_state(dd, &last_local_state);
		read_last_remote_state(dd, &last_remote_state);
		dd_dev_err(dd,
			"LNI failure last states: local 0x%08x, remote 0x%08x\n",
			last_local_state, last_remote_state);
	}

	/* the active link width (downgrade) is 0 on link down */
	ppd->link_width_active = 0;
	ppd->link_width_downgrade_tx_active = 0;
	ppd->link_width_downgrade_rx_active = 0;
	ppd->current_egress_rate = 0;
#endif
	return 0;
}

/*
 * Set link attributes before moving to polling.
 */
static int set_local_link_attributes(struct hfi_pportdata *ppd)
{
#if 1
	return 0;
#else /* WFR legacy */
	struct hfi1_devdata *dd = ppd->dd;
	u8 enable_lane_tx;
	u8 tx_polarity_inversion;
	u8 rx_polarity_inversion;
	int ret;

	/* reset our fabric serdes to clear any lingering problems */
	fabric_serdes_reset(dd);

	/* set the local tx rate - need to read-modify-write */
	ret = read_tx_settings(dd, &enable_lane_tx, &tx_polarity_inversion,
		&rx_polarity_inversion, &ppd->local_tx_rate);
	if (ret)
		goto set_local_link_attributes_fail;

	if (dd->dc8051_ver < dc8051_ver(0, 20)) {
		/* set the tx rate to the fastest enabled */
		if (ppd->link_speed_enabled & OPA_LINK_SPEED_25G)
			ppd->local_tx_rate = 1;
		else
			ppd->local_tx_rate = 0;
	} else {
		/* set the tx rate to all enabled */
		ppd->local_tx_rate = 0;
		if (ppd->link_speed_enabled & OPA_LINK_SPEED_25G)
			ppd->local_tx_rate |= 2;
		if (ppd->link_speed_enabled & OPA_LINK_SPEED_12_5G)
			ppd->local_tx_rate |= 1;
	}

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

	/* z=1 in the next call: AU of 0 is not supported by the hardware */
	ret = write_vc_local_fabric(dd, dd->vau, 1, dd->vcu, dd->vl15_init,
				    ppd->port_crc_mode_enabled);
	if (ret != HCMD_SUCCESS)
		goto set_local_link_attributes_fail;

	ret = write_vc_local_link_width(dd, 0, 0,
		     opa_to_vc_link_widths(ppd->link_width_enabled));
	if (ret != HCMD_SUCCESS)
		goto set_local_link_attributes_fail;

	/* let peer know who we are */
	ret = write_local_device_id(dd, dd->pcidev->device, dd->minrev);
	if (ret == HCMD_SUCCESS)
		return 0;

set_local_link_attributes_fail:
	dd_dev_err(dd,
		"Failed to set local link attributes, return 0x%x\n",
		ret);
	return ret;
#endif
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
const char *opa_lstate_name(u32 lstate)
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
 * Read the hardware link state and set the driver's cached value of it.
 * Return the (new) current value.
 */
u32 get_logical_state(struct hfi_pportdata *ppd)
{
	u32 new_state;

	new_state = chip_to_opa_lstate(ppd->dd, read_logical_state(ppd));
	if (new_state != ppd->lstate) {
		dd_dev_info(ppd->dd, "logical state changed to %s (0x%x)\n",
			opa_lstate_name(new_state), new_state);
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
 * wait_logical_linkstate - wait for an IB link state change to occur
 * @ppd: port device
 * @state: the state to wait for
 * @msecs: the number of milliseconds to wait
 *
 * Wait up to msecs milliseconds for IB link state change to occur.
 * For now, take the easy polling route.
 * Returns 0 if state reached, otherwise -ETIMEDOUT.
 */
static int wait_logical_linkstate(struct hfi_pportdata *ppd, u32 state,
				  int msecs)
{
	unsigned long timeout;

	timeout = jiffies + msecs_to_jiffies(msecs);
	while (1) {
		if (get_logical_state(ppd) == state)
			return 0;
		if (time_after(jiffies, timeout))
			break;
		msleep(20);
	}
	dd_dev_err(ppd->dd, "timeout waiting for link state 0x%x\n", state);

	return -ETIMEDOUT;
}

static inline void add_rcvctrl(struct hfi_devdata *dd, u64 add)
{
#if 0 /* WFR legacy */
	adjust_rcvctrl(dd, add, 0);
#endif
}

/*
 * Handle a linkup or link down notification.
 * This is called outside an interrupt.
 */
void handle_linkup_change(struct hfi_pportdata *ppd, u32 linkup)
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
			ppd->neighbor_guid =
				read_8051_csr(ppd,
					CRK8051_STS_REMOTE_GUID);
			ppd->neighbor_type =
				read_8051_csr(ppd, CRK8051_STS_REMOTE_NODE_TYPE) &
					CRK8051_STS_REMOTE_NODE_TYPE_VAL_MASK;
#if 0 /* WFR legacy */
			ppd->neighbor_port_number =
				read_8051_csr(ppd, CRK8051_STS_REMOTE_PORT_NO) &
					CRK8051_STS_REMOTE_PORT_NO_VAL_SMASK;
#endif
			dd_dev_info(ppd->dd,
				"Neighbor GUID: %llx Neighbor type %d\n",
				ppd->neighbor_guid,
				ppd->neighbor_type);
		}

#if 0 /* WFR legacy */
		/* physical link went up */
		ppd->linkup = 1;
		ppd->offline_disabled_reason = OPA_LINKDOWN_REASON_NONE;

		/* link widths are not available until the link is fully up */
		get_linkup_link_widths(ppd);
#endif

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

/*
	reset 8051
 */
void hfi_8051_reset(const struct hfi_pportdata *ppd)
{
	write_8051_csr(ppd, CRK8051_CFG_RST, 0x1ull);
	write_8051_csr(ppd, CRK8051_CFG_RST, 0x0ull);
}

/*
 * Change the physical port and/or logical link state.
 *
 * Returns 0 on success, -errno on failure.
 */
int hfi_set_link_state(struct hfi_pportdata *ppd, u32 state)
{
	struct hfi_devdata *dd = ppd->dd;
	int ret1, ret = 0;

	mutex_lock(&ppd->hls_lock);

	dd_dev_info(dd, "%p: %s(%d) -> %s(%d)\n", ppd,
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
#if 1 /* new on FXR */
		ppd->host_link_state = HLS_DN_DOWNDEF;
		ppd->lstate = IB_PORT_DOWN;
		opa_core_notify_clients(ppd->dd->bus_dev,
					OPA_LINK_STATE_CHANGE, ppd->pnum);
#endif
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
			dd_dev_err(dd,
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
		   fxr_simics_4_8_33 implements quick_linkup which does not have
		 * VerifyCap state
		 */
		ret = do_quick_linkup(ppd);
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
#if 1 /* new on FXR */
		ppd->host_link_state = HLS_UP_INIT;
		ppd->lstate = IB_PORT_INIT;
#endif
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
		ret = wait_logical_linkstate(ppd, IB_PORT_INIT, 1000);
		if (ret) {
			/* logical state didn't change, stay at going_up */
			ppd->host_link_state = HLS_GOING_UP;
			dd_dev_err(dd,
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

		ret = wait_logical_linkstate(ppd, IB_PORT_ARMED, 1000);
		if (ret) {
			/* logical state didn't change, stay at init */
			ppd->host_link_state = HLS_UP_INIT;
			dd_dev_err(dd,
				"%s: logical state did not change to ARMED\n",
				__func__);
		}
		/*
		 * The simulator does not currently implement SMA messages,
		 * so neighbor_normal is not set.  Set it here when we first
		 * move to Armed.
		 */
#if 0 /* WFR legacy */
		if (dd->icode == ICODE_FUNCTIONAL_SIMULATOR)
#endif
			ppd->neighbor_normal = 1;
		ppd->lstate = IB_PORT_ARMED;
		break;
	case HLS_UP_ACTIVE:
		if (ppd->host_link_state != HLS_UP_ARMED)
			goto unexpected;

		ppd->host_link_state = HLS_UP_ACTIVE;
		set_logical_state(ppd, LSTATE_ACTIVE);
		ret = wait_logical_linkstate(ppd, IB_PORT_ACTIVE, 1000);
		if (ret) {
			/* logical state didn't change, stay at armed */
			ppd->host_link_state = HLS_UP_ARMED;
			dd_dev_err(dd,
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

	case HLS_GOING_OFFLINE:		/* transient within goto_offline() */
	case HLS_LINK_COOLDOWN:		/* transient within goto_offline() */
	default:
		dd_dev_info(dd, "%s(): state: %s\n",
			__func__, link_state_name(state));
		ret = -EINVAL;
	}

	goto _return;

unexpected:
	dd_dev_err(dd, "%s: unexpected state transition from %s to %s\n",
		__func__, link_state_name(ppd->host_link_state),
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
