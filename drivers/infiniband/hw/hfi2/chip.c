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

#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/utsname.h>
#include <linux/delay.h>
#include <linux/bitops.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/signal.h>
#include "chip/fxr_top_defs.h"
#include "chip/fxr_fast_path_defs.h"
#include "chip/fxr_tx_ci_cic_csrs_defs.h"
#include "chip/fxr_tx_ci_cid_csrs_defs.h"
#include "chip/fxr_rx_ci_cic_csrs_defs.h"
#include "chip/fxr_rx_ci_cid_csrs_defs.h"
#include "chip/fxr_rx_et_defs.h"
#include "chip/fxr_rx_hiarb_defs.h"
#include "chip/fxr_rx_hp_defs.h"
#include "chip/fxr_rx_e2e_defs.h"
#include "chip/fxr_linkmux_defs.h"
#include "chip/fxr_linkmux_tp_defs.h"
#include "chip/fxr_linkmux_cm_defs.h"
#include "chip/fxr_linkmux_fpc_defs.h"
#include "chip/fxr_oc_defs.h"
#include "chip/fxr_tx_otr_pkt_top_csrs_defs.h"
#include "chip/fxr_tx_otr_msg_top_csrs_defs.h"
#include "chip/fxr_tx_dma_defs.h"
#include "chip/fxr_rx_dma_defs.h"
#include "chip/fxr_pcim_defs.h"
#include "chip/fxr_8051_defs.h"
#include "chip/fxr_loca_defs.h"
#include "chip/fxr_perfmon_defs.h"
#include "hfi2.h"
#include "link.h"
#include "firmware.h"
#include "debugfs.h"
#ifdef CONFIG_HFI2_STLNP
#include "timesync.h"
#endif
#include "verbs/verbs.h"
#include "verbs/packet.h"
#include "platform.h"
#include "counters.h"
#include "trace.h"
#include "pend_cmdq.h"

#define HFI_DRIVER_RESET_RETRIES 10
#define HFI_UC_RANGE_START	FXR_AT_CSRS
#define HFI_VL_COUNT	10

#define PKEY_DISABLE_SMASK \
	FXR_RXE2E_CFG_PTL_PKEY_CHECK_DISABLE_PTL_PKEY_CHECK_DISABLE_SMASK
#define EQD_THRESH_MASK FXR_RXET_CFG_EQD_HEAD_REFETCH_THRESH_MASK
#define TX_ADDR_MASK FXR_TXOTR_PKT_CFG_PSN_BASE_ADDR_P0_TC_ADDRESS_MASK
#define TX_ADDR_SHIFT FXR_TXOTR_PKT_CFG_PSN_BASE_ADDR_P0_TC_ADDRESS_SHIFT
#define RX_ADDR_MASK FXR_RXE2E_CFG_PSN_BASE_ADDR_P0_TC_ADDRESS_MASK
#define RX_ADDR_SHIFT FXR_RXE2E_CFG_PSN_BASE_ADDR_P0_TC_ADDRESS_SHIFT

uint loopback;
module_param_named(loopback, loopback, uint, 0444);
MODULE_PARM_DESC(loopback,
		 "Put into loopback mode (1 = serdes, 3 = external cable, 4 = internal loopback");

/* TODO - should come from HW headers */
#define FXR_CACHE_CMD_INVALIDATE 0x8
#define HFI_LM_BECN_EVENT	255
#define HFI_8051_DOMAIN		281
/*
 * set the ITR delay for IRQ 255 -BECN event to 125 usec optimal rate at
 * which driver should process interrupt is 8000 per sec and hence delay
 * is set 125 usec
 */
#define HFI_ITR_DELAY_BECN	125
/* ITR_BECN_INDEX is calculated to be 255*8 */
#define HFI_ITR_BECN_INDEX	(HFI_LM_BECN_EVENT * 8)

#define HFI_AUTH_SRANK_MASK FXR_TXCID_CFG_AUTHENTICATION_CSR_SRANK_MASK
#define HFI_AUTH_SRANK_SHIFT FXR_TXCID_CFG_AUTHENTICATION_CSR_SRANK_SHIFT

static bool use_driver_reset = true;
/* Driver reset is the default and only choice until a kernel bug is fixed. */
module_param(use_driver_reset, bool, 0444);
MODULE_PARM_DESC(use_driver_reset, "Use driver reset");

/* FXRTODO: Remove this once we have opafm working with B2B setup */
bool opafm_disable;
module_param_named(opafm_disable, opafm_disable, bool, 0444);
MODULE_PARM_DESC(opafm_disable, "0 - driver needs opafm to work, 1 - driver works without opafm");

/* FXRTODO: Remove this once MNH is available on all Pre-Si setups */
bool no_mnh;
module_param_named(no_mnh, no_mnh, bool, 0444);
MODULE_PARM_DESC(no_mnh, "Set to true if MNH is not available");

/* FXRTODO: Remove this once PE FW is always available */
bool no_pe_fw;
module_param_named(no_pe_fw, no_pe_fw, bool, 0444);
MODULE_PARM_DESC(no_pe_fw, "Set to true if PE FW is not available");

bool simics = true;
module_param_named(simics, simics, bool, 0444);
MODULE_PARM_DESC(simics, "Set to true if running on Simics");

/* FXRTODO: Remove this once Silicon is stable */
bool detect_uncat_errs = true;
module_param_named(detect_uncat_errs, detect_uncat_errs, bool, 0444);
MODULE_PARM_DESC(detect_uncat_errs,
		 "Set to true to detect uncategorized error bits, false to detect undefined error bits generating spurious interrupts");

/* FXRTODO: Remove this once Silicon is stable */
bool no_interrupts;
module_param_named(no_interrupts, no_interrupts, bool, 0444);
MODULE_PARM_DESC(no_interrupts, "Set to true if not using interrupts");

/* FXRTODO: Remove this once Silicon is stable */
bool use_psn_cache;
module_param_named(use_psn_cache, use_psn_cache, bool, 0444);
MODULE_PARM_DESC(use_psn_cache, "Set to true if using psn cache instead of E2E");

bool no_verbs;
module_param_named(no_verbs, no_verbs, bool, 0444);
MODULE_PARM_DESC(no_verbs, "Set to false to load driver without verbs support");

bool pid_reuse = true;
module_param_named(pid_reuse, pid_reuse, bool, 0444);
MODULE_PARM_DESC(pid_reuse, "Set to false to disallow pid reuse");

bool lni_debug = false;
module_param_named(lni_debug, lni_debug, bool, 0444);
MODULE_PARM_DESC(lni_debug, "Disabled by default. Set to enable the Link trace manager");

DEFINE_SPINLOCK(hfi2_unit_lock);
static struct idr hfi2_unit_table;
static void hfi_cmdq_head_config(struct hfi_devdata *dd, u16 cmdq_idx,
				 void *head_base);
static int obtain_boardname(struct hfi_devdata *dd);

static const char *hfi2_class_name = "hfi2";

const char *hfi_class_name(void)
{
	return hfi2_class_name;
}

struct hfi_devdata *hfi_get_unit_dd(int unit)
{
	unsigned long flags;
	struct hfi_devdata *dd;

	spin_lock_irqsave(&hfi2_unit_lock, flags);
	dd = idr_find(&hfi2_unit_table, unit);
	spin_unlock_irqrestore(&hfi2_unit_lock, flags);
	return dd;
}

enum phy_port_states {
	POLLING_ACTIVE				= 0x20,
	POLLING_QUIET				= 0x21,
	DISABLED				= 0x30,
	CONFIG_PHY_DEBOUNCE			= 0x40,
	CONFIG_PHY_EST_COMM_TXRX_HUNT		= 0x41,
	CONFIG_PHY_EST_COMM_TX_HUNT		= 0x42,
	CONFIG_PHY_EST_COMM_LOCAL_COMPLETE	= 0x43,
	CONFIG_PHY_OPT_EQ_OPTIMIZING		= 0x44,
	CONFIG_PHY_OPT_EQ_LOCAL_COMPLETE	= 0x45,
	CONFIG_PHY_VERIFY_CAP_EXCHANGE		= 0x46,
	CONFIG_PHY_VERIFY_CAP_LOCAL_COMPLETE	= 0x47,
	CONFIG_LT_CONFIGURE			= 0x48,
	CONFIG_LT_LINK_TRANSFER_ACTIVE		= 0x49,
	CONFIG_PHY				= 0x4A,
	CONFIG_PCS				= 0x4B,
	CONFIG_SYNC				= 0x4C,
	LINKUP					= 0x50,
	GANGED					= 0x80,
	OFFLINE_QUIET				= 0x90,
	OFFLINE_PLANNED_DOWN_INFORM		= 0x91,
	OFFLINE_READY_TO_QUIET_LT		= 0x92,
	OFFLINE_REPORT_FAILURE			= 0x93,
	OFFLINE_READY_TO_QUIET_BCC		= 0x94,
	PHY_TEST				= 0xB0,
	INTERNAL_LOOPBACK			= 0xE1
};

static const char * const port_state_strs[] = {
	[POLLING_ACTIVE] = "Poll / Active",
	[POLLING_QUIET]  = "Polling Quiet",
	[DISABLED] = "Disabled",
	[CONFIG_PHY_DEBOUNCE] = "Cfg / Cfg Debounce",
	[CONFIG_PHY_EST_COMM_TXRX_HUNT] = "Cfg Est Comm / TxRx Hunt",
	[CONFIG_PHY_EST_COMM_TX_HUNT] = "Cfg Est Comm Tx Hunt",
	[CONFIG_PHY_EST_COMM_LOCAL_COMPLETE] = "Cfg Est Comm Local Complete",
	[CONFIG_PHY_OPT_EQ_OPTIMIZING] = "Cfg Opt EQ / Optimizing",
	[CONFIG_PHY_OPT_EQ_LOCAL_COMPLETE] = "Cfg Opt EQ Local Complete",
	[CONFIG_PHY_VERIFY_CAP_EXCHANGE] = "Verify Cap / Exchange",
	[CONFIG_PHY_VERIFY_CAP_LOCAL_COMPLETE] = "Verify Cap Local Complete",
	[CONFIG_LT_CONFIGURE] = "Cfg LT / Configure",
	[CONFIG_LT_LINK_TRANSFER_ACTIVE] = "Cfg LT Link xfer Active",
	[CONFIG_PHY] = "Cfg Phy",
	[CONFIG_PCS] = "Cfg Pcs",
	[CONFIG_SYNC] = "Cfg Sync",
	[LINKUP] = "LinkUp",
	[GANGED] = "Ganged",
	[OFFLINE_QUIET] = "Offline / Quiet",
	[OFFLINE_PLANNED_DOWN_INFORM] = "Offline Planned Dwn Inform",
	[OFFLINE_READY_TO_QUIET_LT] = "Offline Rdy to Quiet LT",
	[OFFLINE_REPORT_FAILURE] = "Offline Report Failure",
	[OFFLINE_READY_TO_QUIET_BCC] = "Offline Rdy to Quiet BCC",
	[PHY_TEST] = "Phy test",
	[INTERNAL_LOOPBACK] = "Internal Loopback"
};

static void hfi_get_time_info(unsigned long *cur, unsigned long *prev,
			      unsigned long *delta)
{
	*prev = *cur;
	*cur = jiffies_to_msecs(jiffies);
	*delta = *cur - *prev;
}

static int hfi_dbg_link_mgr(void *data)
{
	struct hfi_devdata *dd = (struct hfi_devdata *)data;
	unsigned long cur_time_ms = jiffies_to_msecs(jiffies);
	unsigned long prev_time_ms = cur_time_ms;
	unsigned long delta = 0;
	u64 crk_sts_cur_state = 0;
	u64 crk_dbg_sfr_map_1 = 0, reg = 0;
	u64 port_state_smask = CRK_CRK8051_STS_CUR_STATE_PORT_SMASK;
	u64 crk_code_addr_smask = CRK_CRK8051_DBG_CODE_TRACING_ADDR_VAL_SMASK;
	u64 crk_sfr_tmp1_smask = CRK_CRK8051_DBG_SFR_MAP_1_TMP01_SMASK;
	u64 crk_sfr_tmp2_smask = CRK_CRK8051_DBG_SFR_MAP_1_TMP02_SMASK;
	u64 crk_sfr_tmp8_smask = CRK_CRK8051_DBG_SFR_MAP_1_TMP08_SMASK;
	int idx = 0, array_idx;

	dd_dev_err(dd, "%s:  Idx Time(ms) Delta(ms)       Info\n",
		   __func__);

	/* Handle a SIGINT sent using kill(1) to the pid of this thread */
	allow_signal(SIGINT);
	while (!kthread_should_stop()) {
		reg = read_csr(dd, CRK_CRK8051_STS_CUR_STATE);
		/* Just compare bits 0:7 for OPA Mode PHY State */
		if ((reg ^ crk_sts_cur_state) & (port_state_smask)) {
			crk_sts_cur_state = reg;
			array_idx = crk_sts_cur_state & port_state_smask;
			hfi_get_time_info(&cur_time_ms, &prev_time_ms,
					  &delta);
			reg = read_csr(dd, CRK_CRK8051_DBG_CODE_TRACING_ADDR);
			dd_dev_dbg(dd,
			    "%s: %4d  %6ld    %6ld    phy: %s, 8051 code addr: 0x%llx\n",
			    __func__, idx++, cur_time_ms, delta,
			    (port_state_strs[array_idx]),
			    (reg & crk_code_addr_smask));
		}

		reg = read_csr(dd, CRK_CRK8051_DBG_SFR_MAP_1);
		if ((reg ^ crk_dbg_sfr_map_1) & crk_sfr_tmp8_smask) {
			crk_dbg_sfr_map_1 = reg;
			hfi_get_time_info(&cur_time_ms, &prev_time_ms,
					  &delta);
			reg = read_csr(dd, CRK_CRK8051_DBG_CODE_TRACING_ADDR);
			dd_dev_dbg(dd,
			    "%s: %4d  %6ld    %6ld    tmp1: 0x%2x tmp2: 0x%2x tmp8: 0x%2x 8051 code addr 0x%llx\n",
			    __func__, idx++, cur_time_ms, delta,
			    (u8)(crk_dbg_sfr_map_1 & crk_sfr_tmp1_smask),
			    (u8)(crk_dbg_sfr_map_1 & crk_sfr_tmp2_smask),
			    (u8)(crk_dbg_sfr_map_1 & crk_sfr_tmp8_smask),
			    (reg & crk_code_addr_smask));
		}
		usleep_range(10, 20);
		schedule();
	}
	dd_dev_dbg(dd, "%s: Exiting link manager\n", __func__);
	return 0;
}

int neigh_is_hfi(struct hfi_pportdata *ppd)
{
	return (ppd->neighbor_type & OPA_PI_MASK_NEIGH_NODE_TYPE) == 0;
}

void hfi_write_lm_fpc_prf_per_vl_csr(const struct hfi_pportdata *ppd,
				     u32 offset, u32 index, u64 value)
{
	offset = offset + (index * 8);
	write_csr(ppd->dd, offset, value);
}

void hfi_write_lm_tp_prf_csr(const struct hfi_pportdata *ppd,
			     u32 offset, u64 value)
{
	offset = FXR_TP_PRF_COUNTERS + (offset * 8);
	write_csr(ppd->dd, offset, value);
}

u64 hfi_read_pmon_csr(const struct hfi_devdata *dd,
		      u32 index)
{
	u64 offset = FXR_PMON_CFG_ARRAY + (index * 8);

	return read_csr(dd, offset);
}

u64 hfi_read_lm_fpc_prf_per_vl_csr(const struct hfi_pportdata *ppd,
				   u32 offset, u32 index)
{
	offset = offset + (index * 8);
	return read_csr(ppd->dd, offset);
}

u64 hfi_read_lm_tp_prf_csr(const struct hfi_pportdata *ppd,
			   u32 offset)
{
	offset = FXR_TP_PRF_COUNTERS + (offset * 8);
	return read_csr(ppd->dd, offset);
}

static u64 get_all_cpu_total(u64 __percpu *cntr)
{
	int cpu;
	u64 counter = 0;

	for_each_possible_cpu(cpu)
		counter += *per_cpu_ptr(cntr, cpu);
	return counter;
}

u64 hfi_read_per_cpu_counter(u64 *z_val, u64 __percpu *cntr)
{
	u64 ret = 0;

	ret = get_all_cpu_total(cntr) - *z_val;
	return ret;
}

static void hfi_init_max_lid_csrs(const struct hfi_pportdata *ppd)
{
	const struct hfi_devdata *dd = ppd->dd;
	u64 rx_tc_slid;
	u64 tx_tc_slid;

	tx_tc_slid = read_csr(dd, FXR_TXOTR_PKT_CFG_VALID_TC_DLID);
	rx_tc_slid = read_csr(dd, FXR_RXE2E_CFG_VALID_TC_SLID);

	tx_tc_slid &= ~FXR_TXOTR_PKT_CFG_VALID_TC_DLID_MAX_DLID_P0_SMASK;
	rx_tc_slid &= ~FXR_RXE2E_CFG_VALID_TC_SLID_MAX_SLID_P0_SMASK;

	tx_tc_slid |= (FXR_TXOTR_PKT_CFG_VALID_TC_DLID_MAX_DLID_P0_MASK &
		       dd->pport[0].max_lid) <<
		       FXR_TXOTR_PKT_CFG_VALID_TC_DLID_MAX_DLID_P0_SHIFT;
	rx_tc_slid |= (FXR_RXE2E_CFG_VALID_TC_SLID_MAX_SLID_P0_MASK &
		       dd->pport[0].max_lid) <<
		       FXR_RXE2E_CFG_VALID_TC_SLID_MAX_SLID_P0_SHIFT;

	write_csr(dd, FXR_TXOTR_PKT_CFG_VALID_TC_DLID, tx_tc_slid);
	write_csr(dd, FXR_RXE2E_CFG_VALID_TC_SLID, rx_tc_slid);
}

static void hfi_init_rx_e2e_csrs(const struct hfi_devdata *dd)
{
	u64 tc_slid = 0;

	tc_slid = (FXR_RXE2E_CFG_VALID_TC_SLID_TC_VALID_P0_MASK & 0xf) <<
		   FXR_RXE2E_CFG_VALID_TC_SLID_TC_VALID_P0_SHIFT;
	write_csr(dd, FXR_RXE2E_CFG_VALID_TC_SLID, tc_slid);

	/* FXRTODO: Disable Pkey checks since DV has not validated it */
	if (dd->emulation)
		write_csr(dd, FXR_RXE2E_CFG_PTL_PKEY_CHECK_DISABLE,
			  PKEY_DISABLE_SMASK);
}

static void hfi_set_rfs(const struct hfi_devdata *dd, u16 mtu)
{
	u64 reg = 0;
	int i;
	u8 mtu_id = opa_mtu_to_enum(mtu);

	if (mtu_id == INVALID_MTU_ENC) {
		dd_dev_warn(dd, "invalid mtu for rfs %d", mtu);
		return;
	}

	/* 10K RFS is not supported, default to 8K */
	if (mtu_id == OPA_MTU_10240) {
		mtu_id = OPA_MTU_8192;
		dd_dev_dbg(dd, "%s: 10K RFS converted to 8K\n", __func__);
	}

	for (i = 0; i < (HFI_MAX_TC * HFI_MAX_MC); i++)
		reg |= (mtu_id << (i * 4));

	dd_dev_dbg(dd, "Setting RFS to %u, reg value: %llx\n", mtu, reg);

	write_csr(dd, FXR_TXOTR_MSG_CFG_RFS, reg);
}

static u16 hfi_get_smallest_mtu(const struct hfi_devdata *dd)
{
	int i, port;
	u16 smallest_mtu = INVALID_MTU;

	for (port = 1; port <= dd->num_pports; port++) {
		struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);

		/* Only check data VLs */
		for (i = 0; i < HFI_NUM_DATA_VLS; i++) {
			u16 vl_mtu = ppd->vl_mtu[i];

			if (vl_mtu && vl_mtu < smallest_mtu)
				smallest_mtu = vl_mtu;
		}
	}

	dd_dev_dbg(dd, "Found smallest MTU %u\n", smallest_mtu);

	return smallest_mtu;
}

static void hfi_init_tx_otr_mtu(const struct hfi_devdata *dd, u16 mtu)
{
	u64 vlmtu = 0;
	int i, port;
	u8 mtu_id = opa_mtu_to_enum(mtu);

	if (mtu_id == INVALID_MTU_ENC) {
		dd_dev_warn(dd, "invalid mtu %d", mtu);
		return;
	}

	hfi_set_rfs(dd, mtu);

	for (i = 0; i < HFI_NUM_DATA_VLS; i++)
		vlmtu |= (u64)mtu_id << (i * 4);

	for (port = 1; port <= dd->num_pports; port++) {
		struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);

		for (i = 0; i < HFI_NUM_DATA_VLS; i++)
			ppd->vl_mtu[i] = mtu;

		write_csr(ppd->dd, FXR_TP_CFG_VL_MTU, vlmtu);
	}
}

static void hfi_set_small_headers(const struct hfi_pportdata *ppd)
{
	u64 small;

	small = read_csr(ppd->dd, FXR_TXOTR_MSG_CFG_SMALL_HEADER);

#if 0
	/*
	 * If the upper 4 bits of LID are used (non-zero), then we
	 * can't enable small headers
	 */
	if ((ppd->lid & HFI_EXTENDED_LID_MASK) == 0) {
		ppd_dev_info(ppd, "Enabling small headers\n");
		small.field.ENABLE = 1;

	} else {
		ppd_dev_info(ppd, "LID too large for small headers\n");
		small.field.ENABLE = 0;
	}
#else
	/*
	 * Temporarily disable 8B headers since the firmware handling
	 * for 8B headers is not working correctly with 5.0.99 Simics
	 */
	small &= ~FXR_TXOTR_MSG_CFG_SMALL_HEADER_ENABLE_SMASK;
#endif

	write_csr(ppd->dd, FXR_TXOTR_MSG_CFG_SMALL_HEADER, small);
}

static void hfi_init_tx_otr_csrs(const struct hfi_devdata *dd)
{
	u64 tc_slid = 0;

	tc_slid = (FXR_TXOTR_PKT_CFG_VALID_TC_DLID_TC_VALID_P0_MASK & 0xf) <<
		   FXR_TXOTR_PKT_CFG_VALID_TC_DLID_TC_VALID_P0_SHIFT;
	write_csr(dd, FXR_TXOTR_PKT_CFG_VALID_TC_DLID, tc_slid);

	hfi_init_tx_otr_mtu(dd, HFI_DEFAULT_MAX_MTU);

	if (dd->emulation) {
		/* FXRTODO: Why is this emulation specific? */
		write_csr(dd, FXR_TXOTR_MSG_CFG_RENDEZVOUS_RC, 0x0);
		/* FXRTODO: Disable Pkey checks since DV has not validated it */
		write_csr(dd, FXR_TXOTR_PKT_PKEY_CHECK_DISABLE,
			  PKEY_DISABLE_SMASK);
	}
}

static void hfi_init_tx_cid_csrs(const struct hfi_devdata *dd)
{
	u64 reg;

	/* Set up the traffic and message class for VL15 */
	reg = read_csr(dd, FXR_TXCID_CFG_MAD_TO_TC);
	reg &= ~FXR_TXCID_CFG_MAD_TO_TC_TC_SMASK;
	reg &= ~FXR_TXCID_CFG_MAD_TO_TC_MC_SMASK;

	reg |= (FXR_TXCID_CFG_MAD_TO_TC_TC_MASK & HFI_VL15_TC) <<
		FXR_TXCID_CFG_MAD_TO_TC_TC_SHIFT;
	reg |= (FXR_TXCID_CFG_MAD_TO_TC_MC_MASK & HFI_VL15_MC) <<
		FXR_TXCID_CFG_MAD_TO_TC_MC_SHIFT;
	write_csr(dd, FXR_TXCID_CFG_MAD_TO_TC, reg);
}

static void hfi_init_rate_control(const struct hfi_devdata *dd)
{
	u64 tx_reg = 0;
	u64 rx_reg = 0;

	/*
	 * Set command queue update frequency to 1/4 of queue size.
	 * For TX update freq will be (1 << 5) = 32 - hfi will update
	 * cq head every 32 commands.
	 * The largest tx command should be ~65 slots, so 1/32 is minimal
	 * freq that we can use without deadlock.
	 */
	tx_reg = 5ULL << FXR_TXCIC_CFG_HEAD_UPDATE_CNTRL_RATE_CTRL_SHIFT;
	write_csr(dd, FXR_TXCIC_CFG_HEAD_UPDATE_CNTRL, tx_reg);

	/*
	 * For RX update head every (1 << 2) = 4 commands since RX queues
	 * have only 16 slots.
	 */
	rx_reg = 2ULL << FXR_RXCID_CFG_HEAD_UPDATE_CNTRL_RATE_CTRL_SHIFT;
	write_csr(dd, FXR_RXCID_CFG_HEAD_UPDATE_CNTRL, rx_reg);
}

void hfi_write_qp_state_csrs(struct hfi_devdata *dd, void *qp_state_base,
			     u32 max_qpn)
{
	u64 addr, max;

	addr = (u64)!!qp_state_base << FXR_RXHIARB_CFG_QP_TABLE_VALID_SHIFT;
	addr |= ((u64)qp_state_base >> PAGE_SHIFT) &
		FXR_RXHIARB_CFG_QP_TABLE_QP_BASE_MASK;
	write_csr(dd, FXR_RXHIARB_CFG_QP_TABLE, addr);

	max = max_qpn & FXR_RXHIARB_CFG_QP_MAX_MAX_QP_MASK;
	write_csr(dd, FXR_RXHIARB_CFG_QP_MAX, max);
}

static void hfi_read_guid(struct hfi_devdata *dd)
{
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, 1);
	int port;
	u64 base_guid, guid;

#define NODE_GUID              0x11750101000000UL
	/*
	 * Read from one of the port's 8051 register, since node
	 * guid is common between the two ports.
	 */
	if (no_mnh) {
		base_guid = NODE_GUID;
		if (!strncmp(utsname()->nodename, "viper", 5)) {
			const char *hostname = utsname()->nodename;
			int node = 0, rc;

			/* Extract the node id from the host name */
			hostname += 5;
			rc = kstrtoint(hostname, 0, &node);
			if (rc)
				dd_dev_info(dd, "kstrtoint fail %d\n", rc);
			base_guid += node * 4;
			dd_dev_info(dd, "viper%d port0 base guid 0x%llx\n",
				    node, base_guid);
		}
	} else {
		base_guid = read_csr(ppd->dd, CRK_CRK8051_CFG_LOCAL_GUID);
	}

	dd->nguid = cpu_to_be64(base_guid);

	for (port = 1; port <= dd->num_pports; port++) {
		ppd = to_hfi_ppd(dd, port);
		guid = PORT_GUID(dd->nguid, port);
		ppd->guids[HFI2_PORT_GUID_INDEX] = cpu_to_be64(guid);
		ppd_dev_info(ppd, "PORT GUID %llx",
			     (unsigned long long)guid);
		/*
		 * If we are forcing loopback and want to use the FM, we need to
		 * hack in our neighbor guid and port. Use the same as our own.
		 */
		if (no_mnh || ((loopback == LOOPBACK_HFI) && !opafm_disable)) {
			ppd->neighbor_guid = base_guid;
			ppd->neighbor_port_number = port;
		}
	}

	dd_dev_info(dd, "NODE GUID %llx",
		    (unsigned long long)be64_to_cpu(dd->nguid));
}

static void init_csrs(struct hfi_devdata *dd)
{
	u64 nptl_ctl = 0;
	u64 kdeth_qp = 0;
	u64 lmp0 = 0;
	u64 txotr_timeout = 0;
	u64 lm_config = 0;
	u64 rxet_cfg_eqd;
	u64 bth_qp = 0;
	u64 pcim_itr = 0;
	u64 reg, cfg_ctl;
	void *zbr_ptr = NULL;

	if (dd->emulation) {
		/*
		 * TODO: Need to confirm this value with the hardware team
		 * since it is lower than the power on reset value of
		 * 0x40000. Sudeep suggested we set this to 0xff00, so
		 * keeping this line here and commented for now.
		 */
		/* TODO: In the ww16g RTL, HFI_PCIM_CFG_SBTO has a new offset */
		write_csr(dd, (HFI_PCIM_CSRS + 0x000000000040), 0xff0000);
		dd_dev_info(dd, "Maximizing side-band timeout: 0x%llx\n",
			    read_csr(dd, (HFI_PCIM_CSRS + 0x000000000040)));
		msleep(100);
	}

	/* enable logging PMON counter overflow */
	cfg_ctl = read_csr(dd, FXR_PMON_CFG_CONTROL);
	cfg_ctl |= FXR_PMON_CFG_CONTROL_LOG_OVERFLOW_SMASK;
	write_csr(dd, FXR_PMON_CFG_CONTROL, cfg_ctl);

	/* enable PMON counter overflow interrupt */
	reg = read_csr(dd, FXR_PMON_ERR_EN_HOST_1);
	reg |= FXR_PMON_ERR_EN_HOST_1_EVENTS_SMASK;
	write_csr(dd, FXR_PMON_ERR_EN_HOST_1, reg);

	/* enable non-portals */
	nptl_ctl = FXR_RXHP_CFG_NPTL_CTL_RCV_QP_MAP_ENABLE_SMASK |
		    FXR_RXHP_CFG_NPTL_CTL_RCV_RSM_ENABLE_SMASK |
		    FXR_RXHP_CFG_NPTL_CTL_RCV_BYPASS_ENABLE_SMASK;

	write_csr(dd, FXR_RXHP_CFG_NPTL_CTL, nptl_ctl);
	kdeth_qp = (FXR_RXHP_CFG_NPTL_BTH_QP_KDETH_QP_MASK &
		   hfi2_kdeth_qp) <<
		   FXR_RXHP_CFG_NPTL_BTH_QP_KDETH_QP_SHIFT;
	write_csr(dd, FXR_RXHP_CFG_NPTL_BTH_QP, kdeth_qp);

	bth_qp = (FXR_TXCID_CFG_SENDBTHQP_SEND_BTH_QP_MASK &
		 hfi2_kdeth_qp) <<
		 FXR_TXCID_CFG_SENDBTHQP_SEND_BTH_QP_SHIFT;
	write_csr(dd, FXR_TXCID_CFG_SENDBTHQP, bth_qp);

	if (loopback == LOOPBACK_HFI) {
		lm_config = FXR_LM_CFG_CONFIG_FORCE_LOOPBACK_SMASK;
		write_csr(dd, FXR_LM_CFG_CONFIG, lm_config);
		//quick_linkup = true;
	} else if (loopback == LOOPBACK_LCB) {
		lm_config = FXR_LM_CFG_CONFIG_NEAR_LOOPBACK_DISABLE_SMASK;
		write_csr(dd, FXR_LM_CFG_CONFIG, lm_config);
	}

	txotr_timeout = read_csr(dd, FXR_TXOTR_PKT_CFG_TIMEOUT);
	/*
	 * Set the length of the timeout interval used to retransmit
	 * outstanding packets to 5 seconds for Simics back to back.
	 * TODO: Determine right value for FPGA & Silicon.
	 */
	txotr_timeout &= ~FXR_TXOTR_PKT_CFG_TIMEOUT_SCALER_SMASK;
	txotr_timeout |= (FXR_TXOTR_PKT_CFG_TIMEOUT_SCALER_MASK & 0x165A0D) <<
			 FXR_TXOTR_PKT_CFG_TIMEOUT_SCALER_SHIFT;
	write_csr(dd, FXR_TXOTR_PKT_CFG_TIMEOUT, txotr_timeout);
	/*
	 * Set timeout for CMDQ.  This is intentionally high for Simics.
	 * 1 << 30 is 0.895 seconds
	 */
	write_csr(dd, FXR_TXCIC_CFG_TO_LIMIT, 1 << 30);

	/*
	 * FXRTODO Temporary hack.  Must set real value here.
	 * See https://hsdes.intel.com/appstore/article/#/1209668060
	 */
	rxet_cfg_eqd = read_csr(dd, FXR_RXET_CFG_EQD);
	rxet_cfg_eqd &= ~FXR_RXET_CFG_EQD_HEAD_REFETCH_THRESH_SMASK;
	/* FXRTODO: See HSD 1209668060 */
	if (dd->emulation)
		rxet_cfg_eqd |= (EQD_THRESH_MASK & 2) <<
				FXR_RXET_CFG_EQD_HEAD_REFETCH_THRESH_SHIFT;
	else
		rxet_cfg_eqd |= (EQD_THRESH_MASK & 1) <<
				FXR_RXET_CFG_EQD_HEAD_REFETCH_THRESH_SHIFT;

	write_csr(dd, FXR_RXET_CFG_EQD, rxet_cfg_eqd);

	/* FXRTODO: tune the ITR_DELAY after we receive the silicon */
	pcim_itr = read_csr(dd, HFI_PCIM_INT_ITR + HFI_ITR_BECN_INDEX);
	pcim_itr &= ~HFI_PCIM_INT_ITR_INT_DELAY_SMASK;
	pcim_itr |= (HFI_PCIM_INT_ITR_INT_DELAY_MASK & HFI_ITR_DELAY_BECN) <<
		     HFI_PCIM_INT_ITR_INT_DELAY_SHIFT;
	/* FXRTODO: See HSD 2204032747 */
	if (!dd->emulation)
		write_csr(dd, HFI_PCIM_INT_ITR + HFI_ITR_BECN_INDEX, pcim_itr);

	/*
	 * Set the SLID based on the hostname to enable back to back
	 * support in Simics if the FM won't set it later.
	 */
	if (opafm_disable) {
		if (!strncmp(utsname()->nodename, "viper", 5)) {
			const char *hostname = utsname()->nodename;
			int node = 0, rc;

			/* Extract the node id from the host name */
			hostname += 5;
			rc = kstrtoint(hostname, 0, &node);
			if (rc)
				dd_dev_info(dd, "kstrtoint fail %d\n", rc);
			dd->pport[0].lid = (node * 2) + 1;
			dd_dev_info(dd, "viper%d port0 lid %d\n", node,
				    dd->pport[0].lid);
		}
		lmp0 = (FXR_LM_CFG_PORT0_DLID_MASK & dd->pport[0].lid) <<
			FXR_LM_CFG_PORT0_DLID_SHIFT;
		write_csr(dd, FXR_LM_CFG_PORT0, lmp0);
		write_csr(dd, FXR_TXOTR_PKT_CFG_SLID_PT0,
			  dd->pport[0].lid);
	}
	hfi_init_rx_e2e_csrs(dd);
	hfi_init_tx_otr_csrs(dd);
	hfi_init_tx_cid_csrs(dd);
	/* For emulation debug, CQ pointers should not be rate controlled */
	if (!dd->emulation)
		hfi_init_rate_control(dd);

	/* FXRTODO: See HSD 2203689085 */
	if (dd->emulation) {
		/*
		 * FXRTODO: Perhaps point zbr_ptr to dd or other data structure
		 * instead of new allocation. Also need to free this memory
		 */
		zbr_ptr = kmalloc(sizeof(u64), GFP_DMA);
		if (zbr_ptr)
			write_csr(dd, HFI_LOCA_CFG_ZBR, virt_to_phys(zbr_ptr));
	}
}

static int hfi_psn_init(struct hfi_pportdata *port, u32 max_lid)
{
	int i, j, rc = 0;
	struct hfi_devdata *dd = port->dd;
	u32 tx_off = FXR_TXOTR_PKT_CFG_PSN_BASE_ADDR_P0_TC;
	u64 tx_psn = 0;
	u32 rx_off = FXR_RXE2E_CFG_PSN_BASE_ADDR_P0_TC;
	u64 rx_psn = 0;
	/*
	 * Size of packet sequence number state assuming LMC = 0
	 * The same PSN buffer is used for both TX and RX and hence
	 * the multiplication by 2
	 * TODO: LMC settings other than 0 need to be handled
	 */
	size_t psn_size = 2 * max_lid * 8;

	/* Only allocate for data TCs. TC3 is used for management */
	for (i = 0; i < HFI_MAX_TC - 1; i++) {
		struct hfi_ptcdata *tc = &port->ptc[i];

		tc->psn_size = psn_size;
		for (j = 0; j < HFI_MAX_PKEYS; j++) {
			u32 psn_off = (4 * j + i) * 8;

			tc->psn_base[j] = vzalloc_node(psn_size, dd->node);
			if (!tc->psn_base[j]) {
				rc = -ENOMEM;
				goto done;
			}
			hfi_at_reg_range(&dd->priv_ctx, tc->psn_base[j],
					 psn_size, NULL, true);
			tx_psn = (TX_ADDR_MASK &
				 (u64)tc->psn_base >> PAGE_SHIFT) <<
				 TX_ADDR_SHIFT;
			rx_psn = (RX_ADDR_MASK &
				 (u64)tc->psn_base >> PAGE_SHIFT) <<
				 RX_ADDR_SHIFT;
			write_csr(dd, tx_off + psn_off, tx_psn);
			write_csr(dd, rx_off + psn_off, rx_psn);
		}
	}
done:
	return rc;
}

void hfi_psn_uninit(struct hfi_pportdata *port)
{
	int i, j;
	struct hfi_devdata *dd = port->dd;
	u32 off_tx = FXR_TXOTR_PKT_CFG_PSN_BASE_ADDR_P0_TC;
	u32 off_rx = FXR_RXE2E_CFG_PSN_BASE_ADDR_P0_TC;

	/* Only allocate for data TCs. TC3 is used for management */
	for (i = 0; i < HFI_MAX_TC - 1; i++) {
		struct hfi_ptcdata *tc = &port->ptc[i];

		for (j = 0; j < HFI_MAX_PKEYS; j++) {
			if (tc->psn_base[j]) {
				u32 psn_off = (4 * j + i) * 8;

				hfi_at_dereg_range(&dd->priv_ctx,
						   tc->psn_base[j],
						   tc->psn_size);
				vfree(tc->psn_base[j]);
				tc->psn_base[j] = NULL;
				write_csr(dd, off_tx + psn_off, 0);
				write_csr(dd, off_rx + psn_off, 0);
			}
		}
	}
}

/*
 * FXRTODO: Currently lid is being assigned statically by
 * the driver (see init_csrs)depending on the hostname of the simics model.
 * Remove that code once this path is verified using opafm
 */
static void hfi_set_lid_lmc(struct hfi_pportdata *ppd)
{
	struct hfi_devdata *dd = ppd->dd;
	u64 lmp0;

	lmp0 = read_csr(dd, FXR_LM_CFG_PORT0);
	lmp0 &= ~FXR_LM_CFG_PORT0_DLID_SMASK;
	lmp0 |= (FXR_LM_CFG_PORT0_DLID_MASK & ppd->lid) <<
		 FXR_LM_CFG_PORT0_DLID_SHIFT;
	if (!ppd->lmc) {
		lmp0 &= ~FXR_LM_CFG_PORT0_LMC_ENABLE_SMASK;
		lmp0 &= ~FXR_LM_CFG_PORT0_LMC_WIDTH_SMASK;
		lmp0 |= FXR_LM_CFG_PORT0_LMC_ENABLE_SMASK;
		lmp0 |= (FXR_LM_CFG_PORT0_LMC_WIDTH_MASK & ppd->lmc) <<
			 FXR_LM_CFG_PORT0_LMC_WIDTH_SHIFT;
	}
	write_csr(dd, FXR_LM_CFG_PORT0, lmp0);
	write_csr(dd, FXR_TXOTR_PKT_CFG_SLID_PT0, ppd->lid);

	/*
	 * FXTODO: Initiate any HW events that needs to trigger due
	 * to LID and LMC change here.
	 */
}

/*
 * Set Send Length
 * @ppd - per port data
 */
static void hfi_set_send_length(struct hfi_pportdata *ppd)
{
	u64 tp;
	u64 fpc;
	u16 smallest_mtu;
	u8 vl0 = opa_mtu_to_enum(ppd->vl_mtu[0]);
	u8 vl1 = opa_mtu_to_enum(ppd->vl_mtu[1]);
	u8 vl2 = opa_mtu_to_enum(ppd->vl_mtu[2]);
	u8 vl3 = opa_mtu_to_enum(ppd->vl_mtu[3]);
	u8 vl4 = opa_mtu_to_enum(ppd->vl_mtu[4]);
	u8 vl5 = opa_mtu_to_enum(ppd->vl_mtu[5]);
	u8 vl6 = opa_mtu_to_enum(ppd->vl_mtu[6]);
	u8 vl7 = opa_mtu_to_enum(ppd->vl_mtu[7]);
	u8 vl8 = opa_mtu_to_enum(ppd->vl_mtu[8]);
	u8 vl15 = opa_mtu_to_enum(ppd->vl_mtu[15]);

	tp = read_csr(ppd->dd, FXR_TP_CFG_VL_MTU);
	fpc = read_csr(ppd->dd, FXR_FPC_CFG_PORT_CONFIG);

	if (vl0 != INVALID_MTU_ENC) {
		tp &= ~FXR_TP_CFG_VL_MTU_VL0_TX_MTU_SMASK;
		tp |= (FXR_TP_CFG_VL_MTU_VL0_TX_MTU_MASK & vl0) <<
		       FXR_TP_CFG_VL_MTU_VL0_TX_MTU_SHIFT;
		fpc &= ~FXR_FPC_CFG_PORT_CONFIG_VL0_MTU_SMASK;
		fpc |= (FXR_FPC_CFG_PORT_CONFIG_VL0_MTU_MASK & vl0) <<
			FXR_FPC_CFG_PORT_CONFIG_VL0_MTU_SHIFT;
	}

	if (vl1 != INVALID_MTU_ENC) {
		tp &= ~FXR_TP_CFG_VL_MTU_VL1_TX_MTU_SMASK;
		tp |= (FXR_TP_CFG_VL_MTU_VL1_TX_MTU_MASK & vl1) <<
		       FXR_TP_CFG_VL_MTU_VL1_TX_MTU_SHIFT;
		fpc &= ~FXR_FPC_CFG_PORT_CONFIG_VL1_MTU_SMASK;
		fpc |= (FXR_FPC_CFG_PORT_CONFIG_VL1_MTU_MASK & vl1) <<
			FXR_FPC_CFG_PORT_CONFIG_VL1_MTU_SHIFT;
	}

	if (vl2 != INVALID_MTU_ENC) {
		tp &= ~FXR_TP_CFG_VL_MTU_VL2_TX_MTU_SMASK;
		tp |= (FXR_TP_CFG_VL_MTU_VL2_TX_MTU_MASK & vl2) <<
		       FXR_TP_CFG_VL_MTU_VL2_TX_MTU_SHIFT;
		fpc &= ~FXR_FPC_CFG_PORT_CONFIG_VL2_MTU_SMASK;
		fpc |= (FXR_FPC_CFG_PORT_CONFIG_VL2_MTU_MASK & vl2) <<
			FXR_FPC_CFG_PORT_CONFIG_VL2_MTU_SHIFT;
	}

	if (vl3 != INVALID_MTU_ENC) {
		tp &= ~FXR_TP_CFG_VL_MTU_VL3_TX_MTU_SMASK;
		tp |= (FXR_TP_CFG_VL_MTU_VL3_TX_MTU_MASK & vl3) <<
		       FXR_TP_CFG_VL_MTU_VL3_TX_MTU_SHIFT;
		fpc &= ~FXR_FPC_CFG_PORT_CONFIG_VL3_MTU_SMASK;
		fpc |= (FXR_FPC_CFG_PORT_CONFIG_VL3_MTU_MASK & vl3) <<
			FXR_FPC_CFG_PORT_CONFIG_VL3_MTU_SHIFT;
	}

	if (vl4 != INVALID_MTU_ENC) {
		tp &= ~FXR_TP_CFG_VL_MTU_VL4_TX_MTU_SMASK;
		tp |= (FXR_TP_CFG_VL_MTU_VL4_TX_MTU_MASK & vl4) <<
		       FXR_TP_CFG_VL_MTU_VL4_TX_MTU_SHIFT;
		fpc &= ~FXR_FPC_CFG_PORT_CONFIG_VL4_MTU_SMASK;
		fpc |= (FXR_FPC_CFG_PORT_CONFIG_VL4_MTU_MASK & vl4) <<
			FXR_FPC_CFG_PORT_CONFIG_VL4_MTU_SHIFT;
	}

	if (vl5 != INVALID_MTU_ENC) {
		tp &= ~FXR_TP_CFG_VL_MTU_VL5_TX_MTU_SMASK;
		tp |= (FXR_TP_CFG_VL_MTU_VL5_TX_MTU_MASK & vl5) <<
		       FXR_TP_CFG_VL_MTU_VL5_TX_MTU_SHIFT;
		fpc &= ~FXR_FPC_CFG_PORT_CONFIG_VL5_MTU_SMASK;
		fpc |= (FXR_FPC_CFG_PORT_CONFIG_VL5_MTU_MASK & vl5) <<
			FXR_FPC_CFG_PORT_CONFIG_VL5_MTU_SHIFT;
	}

	if (vl6 != INVALID_MTU_ENC) {
		tp &= ~FXR_TP_CFG_VL_MTU_VL6_TX_MTU_SMASK;
		tp |= (FXR_TP_CFG_VL_MTU_VL6_TX_MTU_MASK & vl6) <<
		       FXR_TP_CFG_VL_MTU_VL6_TX_MTU_SHIFT;
		fpc &= ~FXR_FPC_CFG_PORT_CONFIG_VL6_MTU_SMASK;
		fpc |= (FXR_FPC_CFG_PORT_CONFIG_VL6_MTU_MASK & vl6) <<
			FXR_FPC_CFG_PORT_CONFIG_VL6_MTU_SHIFT;
	}

	if (vl7 != INVALID_MTU_ENC) {
		tp &= ~FXR_TP_CFG_VL_MTU_VL7_TX_MTU_SMASK;
		tp |= (FXR_TP_CFG_VL_MTU_VL7_TX_MTU_MASK & vl7) <<
		       FXR_TP_CFG_VL_MTU_VL7_TX_MTU_SHIFT;
		fpc &= ~FXR_FPC_CFG_PORT_CONFIG_VL7_MTU_SMASK;
		fpc |= (FXR_FPC_CFG_PORT_CONFIG_VL7_MTU_MASK & vl7) <<
			FXR_FPC_CFG_PORT_CONFIG_VL7_MTU_SHIFT;
	}

	if (vl8 != INVALID_MTU_ENC) {
		tp &= ~FXR_TP_CFG_VL_MTU_VL8_TX_MTU_SMASK;
		tp |= (FXR_TP_CFG_VL_MTU_VL8_TX_MTU_MASK & vl8) <<
		       FXR_TP_CFG_VL_MTU_VL8_TX_MTU_SHIFT;
		fpc &= ~FXR_FPC_CFG_PORT_CONFIG_VL8_MTU_SMASK;
		fpc |= (FXR_FPC_CFG_PORT_CONFIG_VL8_MTU_MASK & vl8) <<
			FXR_FPC_CFG_PORT_CONFIG_VL8_MTU_SHIFT;
	}

	if (vl15 != INVALID_MTU_ENC) {
		tp &= ~FXR_TP_CFG_VL_MTU_VL15_TX_MTU_SMASK;
		tp |= (FXR_TP_CFG_VL_MTU_VL15_TX_MTU_MASK & vl15) <<
		       FXR_TP_CFG_VL_MTU_VL15_TX_MTU_SHIFT;
		fpc &= ~FXR_FPC_CFG_PORT_CONFIG_VL15_MTU_SMASK;
		fpc |= (FXR_FPC_CFG_PORT_CONFIG_VL15_MTU_MASK & vl15) <<
			FXR_FPC_CFG_PORT_CONFIG_VL15_MTU_SHIFT;
	}
	write_csr(ppd->dd, FXR_TP_CFG_VL_MTU, tp);
	write_csr(ppd->dd, FXR_FPC_CFG_PORT_CONFIG, fpc);

	/*
	 * Find the smallest MTU from any valid data VL on
	 * any port for this HFI. This value will be used to set
	 * the Rendezvous Fragment Size for the whole HFI.
	 */
	smallest_mtu = hfi_get_smallest_mtu(ppd->dd);

	hfi_set_rfs(ppd->dd, smallest_mtu);
}

void hfi_cfg_reset_pmon_cntrs(struct hfi_devdata *dd)
{
	u64 cfg_ctl;

	cfg_ctl = read_csr(dd, FXR_PMON_CFG_CONTROL);
	cfg_ctl |= FXR_PMON_CFG_CONTROL_RESET_CTRS_SMASK;
	write_csr(dd, FXR_PMON_CFG_CONTROL, cfg_ctl);
}

void hfi_reset_pma_perf_cntrs(struct hfi_pportdata *ppd)
{
	int i;

	hfi_write_lm_tp_prf_csr(ppd, TP_PRF_XMIT_DATA, 0);
	write_csr(ppd->dd, FXR_FPC_PRF_PORTRCV_DATA_CNT, 0);
	hfi_write_lm_tp_prf_csr(ppd, TP_PRF_XMIT_PKTS, 0);
	write_csr(ppd->dd, FXR_FPC_PRF_PORTRCV_PKT_CNT, 0);
	hfi_write_lm_tp_prf_csr(ppd, TP_PRF_MULTICAST_XMIT_PKTS, 0);
	write_csr(ppd->dd, FXR_FPC_PRF_PORTRCV_MCAST_PKT_CNT, 0);
	hfi_write_lm_tp_prf_csr(ppd, TP_PRF_XMIT_WAIT, 0);
	write_csr(ppd->dd, FXR_FPC_PRF_PORTRCV_FECN, 0);
	write_csr(ppd->dd, FXR_FPC_PRF_PORTRCV_BECN, 0);
	write_csr(ppd->dd, FXR_FPC_PRF_PORTRCV_BUBBLE, 0);

	for (i = 0; i < HFI_VL_COUNT; i++) {
		hfi_write_lm_tp_prf_csr(ppd, TP_PRF_XMIT_DATA + i + 1, 0);
		hfi_write_lm_fpc_prf_per_vl_csr(ppd,
						FXR_FPC_PRF_PORT_VL_RCV_DATA_CNT
						, i, 0);
		hfi_write_lm_tp_prf_csr(ppd, TP_PRF_XMIT_PKTS + i + 1, 0);
		hfi_write_lm_fpc_prf_per_vl_csr(ppd,
						FXR_FPC_PRF_PORT_VL_RCV_PKT_CNT,
						i, 0);
		hfi_write_lm_tp_prf_csr(ppd, TP_PRF_XMIT_WAIT + i + 1, 0);
		hfi_write_lm_fpc_prf_per_vl_csr(ppd,
						FXR_FPC_PRF_PORT_VL_RCV_FECN, i,
						0);
		hfi_write_lm_fpc_prf_per_vl_csr(ppd,
						FXR_FPC_PRF_PORT_VL_RCV_BECN, i,
						0);
		hfi_write_lm_fpc_prf_per_vl_csr(ppd,
						FXR_FPC_PRF_PORT_VL_RCV_BUBBLE,
						i, 0);
		hfi_write_lm_tp_prf_csr(ppd, TP_ERR_XMIT_DISCARD + i + 1, 0);
	}
}

void hfi_reset_pma_error_cntrs(struct hfi_pportdata *ppd)
{
	write_csr(ppd->dd, FXR_FPC_ERR_PORTRCV_CONSTRAINT_ERROR, 0);
	write_csr(ppd->dd, FXR_FPC_ERR_PORTRCV_PHY_REMOTE_ERROR, 0);
	write_csr(ppd->dd, OC_LCB_ERR_INFO_TX_REPLAY_CNT, 0);
	write_csr(ppd->dd, OC_LCB_ERR_INFO_RX_REPLAY_CNT, 0);
	write_csr(ppd->dd, FXR_FPC_ERR_PORTRCV_ERROR, 0);
	write_csr(ppd->dd, FXR_LM_ERR_INFO3, 0);
	write_csr(ppd->dd, FXR_FPC_ERR_FMCONFIG_ERROR, 0);
	write_csr(ppd->dd, FXR_FPC_ERR_UNCORRECTABLE_ERROR, 0);
}

void hfi_cfg_out_pkey_check(struct hfi_pportdata *ppd, u8 enable)
{
	u64 misc;

	if (no_mnh)
		enable = false;

	if (enable)
		ppd->part_enforce |= HFI_PART_ENFORCE_OUT;
	else
		ppd->part_enforce &= ~HFI_PART_ENFORCE_OUT;
	misc = read_csr(ppd->dd, FXR_TP_CFG_MISC_CTRL);
	misc &= ~FXR_TP_CFG_MISC_CTRL_PKEY_CHK_ENABLE_SMASK;
	misc |= (FXR_TP_CFG_MISC_CTRL_PKEY_CHK_ENABLE_MASK & enable) <<
		 FXR_TP_CFG_MISC_CTRL_PKEY_CHK_ENABLE_SHIFT;
	write_csr(ppd->dd, FXR_TP_CFG_MISC_CTRL, misc);
}

void hfi_cfg_in_pkey_check(struct hfi_pportdata *ppd, u8 enable)
{
	u64 fpc;

	if (no_mnh)
		enable = false;

	if (enable)
		ppd->part_enforce |= HFI_PART_ENFORCE_IN;
	else
		ppd->part_enforce &= ~HFI_PART_ENFORCE_IN;
	fpc = read_csr(ppd->dd, FXR_FPC_CFG_PORT_CONFIG);
	fpc &= ~FXR_FPC_CFG_PORT_CONFIG_PKEY_EN_SMASK;
	fpc &= ~FXR_FPC_CFG_PORT_CONFIG_PKEY_MODE_SMASK;
	fpc |= (FXR_FPC_CFG_PORT_CONFIG_PKEY_EN_MASK & enable) <<
		FXR_FPC_CFG_PORT_CONFIG_PKEY_EN_SHIFT;
	/* 1 for HFI */
	fpc |= FXR_FPC_CFG_PORT_CONFIG_PKEY_MODE_SMASK;
	write_csr(ppd->dd, FXR_FPC_CFG_PORT_CONFIG, fpc);
}

void hfi_cfg_lm_pkey_check(struct hfi_pportdata *ppd, u8 enable)
{
	ppd->hw_lm_pkey = enable;
	hfi_cfg_in_pkey_check(ppd, enable);
	hfi_cfg_out_pkey_check(ppd, enable);
}

void hfi_cfg_ptl_pkey_check(struct hfi_pportdata *ppd, u8 enable)
{
	ppd->hw_ptl_pkey = enable;
	write_csr(ppd->dd, FXR_RXE2E_CFG_PTL_PKEY_CHECK_DISABLE, !enable);
	write_csr(ppd->dd, FXR_TXOTR_PKT_PKEY_CHECK_DISABLE, !enable);
}

static void hfi_set_implicit_pkeys(struct hfi_pportdata *ppd,
				   u16 *pkey_8b, u16 *pkey_10b)
{
	u64 small;
	u64 eg_pkey;
	u64 in_pkey;

	if (no_mnh) {
		pkey_8b = NULL;
		pkey_10b = NULL;
	}

	/* Set ingress and egress implicit 8B/10B pkeys */
	eg_pkey = read_csr(ppd->dd, FXR_TP_CFG_PKEY_CHECK_CTRL);
	in_pkey = read_csr(ppd->dd, FXR_FPC_CFG_PKEY_CTRL);

	if (pkey_8b) {
		u64 rxhp_cfg;

		ppd_dev_dbg(ppd, "Setting 8b implicit to 0x%x\n", *pkey_8b);

		eg_pkey &= ~FXR_TP_CFG_PKEY_CHECK_CTRL_PKEY_8B_SMASK;
		eg_pkey |= (FXR_TP_CFG_PKEY_CHECK_CTRL_PKEY_8B_MASK &
			    *pkey_8b) <<
			    FXR_TP_CFG_PKEY_CHECK_CTRL_PKEY_8B_SHIFT;
		in_pkey &= ~FXR_FPC_CFG_PKEY_CTRL_PKEY_8B_SMASK;
		in_pkey |= (FXR_FPC_CFG_PKEY_CTRL_PKEY_8B_MASK &
			    *pkey_8b) <<
			    FXR_FPC_CFG_PKEY_CTRL_PKEY_8B_SHIFT;
		ppd->pkey_8b = *pkey_8b;

		/* Set small header pkey value */
		small = read_csr(ppd->dd, FXR_TXOTR_MSG_CFG_SMALL_HEADER);
		small &= ~FXR_TXOTR_MSG_CFG_SMALL_HEADER_PKEY_8B_SMASK;
		small |= (FXR_TXOTR_MSG_CFG_SMALL_HEADER_PKEY_8B_MASK &
			  *pkey_8b) <<
			  FXR_TXOTR_MSG_CFG_SMALL_HEADER_PKEY_8B_SHIFT;
		write_csr(ppd->dd, FXR_TXOTR_MSG_CFG_SMALL_HEADER, small);

		rxhp_cfg = read_csr(ppd->dd, FXR_RXHP_CFG_CTL);
		rxhp_cfg &= ~FXR_RXHP_CFG_CTL_DEFAULT_8B_PKEY_SMASK;
		rxhp_cfg |= (FXR_RXHP_CFG_CTL_DEFAULT_8B_PKEY_MASK &
			     *pkey_8b) <<
			     FXR_RXHP_CFG_CTL_DEFAULT_8B_PKEY_SHIFT;
		write_csr(ppd->dd, FXR_RXHP_CFG_CTL, rxhp_cfg);
	}

	if (pkey_10b) {
		ppd_dev_dbg(ppd, "Setting 10b implicit to 0x%x\n", *pkey_10b);

		eg_pkey &= ~FXR_TP_CFG_PKEY_CHECK_CTRL_PKEY_10B_SMASK;
		eg_pkey |= (FXR_TP_CFG_PKEY_CHECK_CTRL_PKEY_10B_MASK &
			   (*pkey_10b >> 4)) <<
			   FXR_TP_CFG_PKEY_CHECK_CTRL_PKEY_10B_SHIFT;
		in_pkey &= ~FXR_FPC_CFG_PKEY_CTRL_PKEY_10B_SMASK;
		in_pkey |= (FXR_FPC_CFG_PKEY_CTRL_PKEY_10B_MASK &
			   (*pkey_10b >> 4)) <<
			   FXR_FPC_CFG_PKEY_CTRL_PKEY_10B_SHIFT;
		ppd->pkey_10b = *pkey_10b;
	}

	write_csr(ppd->dd, FXR_TP_CFG_PKEY_CHECK_CTRL, eg_pkey);
	write_csr(ppd->dd, FXR_FPC_CFG_PKEY_CTRL, in_pkey);
}

void hfi_cfg_pkey_check(struct hfi_pportdata *ppd, u8 enable)
{
	/* Enable input/output LM checking */
	hfi_cfg_lm_pkey_check(ppd, enable);

	/* Always disable PTL_PKEY check in ZEBU and FPGA until validated */
	if (ppd->dd->emulation)
		hfi_cfg_ptl_pkey_check(ppd, 0);
	else
		hfi_cfg_ptl_pkey_check(ppd, enable);
}

#ifdef CONFIG_HFI2_STLNP

static void hfi_set_ptl_pkey_entry(struct hfi_devdata *dd, u16 pkey, u8 id)
{
	/* Disable on ZEBU for now until known-working */
	if (dd->emulation)
		return;

	write_csr(dd, FXR_TXOTR_PKT_CFG_PTL_PKEY_TABLE + 8 * id, pkey);
	write_csr(dd, FXR_RXE2E_CFG_PTL_PKEY_TABLE_CAM + 8 * id, pkey);
}

static void hfi_set_ptl_pkey_table(struct hfi_pportdata *ppd)
{
	int i, j;

	/*
	 * PTL_PKEY tables must be unique, since it is used as a CAM
	 * lookup. Also filter out management PKEYs since those should
	 * never be used for Portals traffic.
	 */
	for (i = 0; i < HFI_MAX_PKEYS; i++) {
		int pkey_id = i;
		u16 new = ppd->pkeys[i];

		if (HFI_PKEY_CAM(new) == OPA_LIM_MGMT_PKEY)
			continue;

		/*
		 * Scan for duplicates earlier in the list, set to the max
		 * of any duplicate. This effectively sets the higher of the
		 * permissions of the membership bit.
		 */
		for (j = 0; j < i; j++) {
			u16 old = ppd->pkeys[j];

			if (HFI_PKEY_IS_EQ(new, old)) {
				if (new > old)
					pkey_id = j;
				break;
			}
		}

		hfi_set_ptl_pkey_entry(ppd->dd, new, pkey_id);
	}
}
#endif

/*
 * Set the device/port partition key table. The MAD code
 * will ensure that, at least, the partial management
 * partition key is present in the table.
 */
static void hfi_set_pkey_table(struct hfi_pportdata *ppd)
{
	u64 tx_pkey;
	u64 rx_pkey;
	int i, j;

	for (i = 0, j = 0; i < HFI_MAX_PKEYS; i += 4, j++) {
		tx_pkey = 0;
		rx_pkey = 0;
		tx_pkey |= (FXR_TP_CFG_PKEY_TABLE_ENTRY0_MASK &
			    ppd->pkeys[i]) <<
			    FXR_TP_CFG_PKEY_TABLE_ENTRY0_SHIFT;
		tx_pkey |= (FXR_TP_CFG_PKEY_TABLE_ENTRY1_MASK &
			    ppd->pkeys[i + 1]) <<
			    FXR_TP_CFG_PKEY_TABLE_ENTRY1_SHIFT;
		tx_pkey |= (FXR_TP_CFG_PKEY_TABLE_ENTRY2_MASK &
			    ppd->pkeys[i + 2]) <<
			    FXR_TP_CFG_PKEY_TABLE_ENTRY2_SHIFT;
		tx_pkey |= (FXR_TP_CFG_PKEY_TABLE_ENTRY3_MASK &
			    ppd->pkeys[i + 3]) <<
			    FXR_TP_CFG_PKEY_TABLE_ENTRY3_SHIFT;

		rx_pkey |= (FXR_FPC_CFG_PKEY_TABLE_ENTRY0_MASK &
			    ppd->pkeys[i]) <<
			    FXR_FPC_CFG_PKEY_TABLE_ENTRY0_SHIFT;
		rx_pkey |= (FXR_FPC_CFG_PKEY_TABLE_ENTRY1_MASK &
			    ppd->pkeys[i + 1]) <<
			    FXR_FPC_CFG_PKEY_TABLE_ENTRY1_SHIFT;
		rx_pkey |= (FXR_FPC_CFG_PKEY_TABLE_ENTRY2_MASK &
			    ppd->pkeys[i + 2]) <<
			    FXR_FPC_CFG_PKEY_TABLE_ENTRY2_SHIFT;
		rx_pkey |= (FXR_FPC_CFG_PKEY_TABLE_ENTRY3_MASK &
			    ppd->pkeys[i + 3]) <<
			    FXR_FPC_CFG_PKEY_TABLE_ENTRY3_SHIFT;
		write_csr(ppd->dd, FXR_TP_CFG_PKEY_TABLE + 0x08 * j,
				    tx_pkey);
		write_csr(ppd->dd, FXR_FPC_CFG_PKEY_TABLE + 0x08 * j,
				     rx_pkey);
	}

#ifdef CONFIG_HFI2_STLNP
	hfi_set_ptl_pkey_table(ppd);
#endif
	hfi_cfg_pkey_check(ppd, 1);
}

/*
 * Our neighbor has indicated that we are allowed to act as a fabric
 * manager, so place the full management partition key in the second
 * (0-based) pkey array position. Note that we should already have the
 * limited management partition key in array element 1, and also that the
 * port is not yet up when add_full_mgmt_pkey() is invoked.
 */
void hfi_add_full_mgmt_pkey(struct hfi_pportdata *ppd)
{
	/*
	 * Sanity check - ppd->pkeys[2] should be 0 or already
	 * initialized
	 */
	if (!((ppd->pkeys[2] == 0) || (ppd->pkeys[2] == OPA_FULL_MGMT_PKEY)))
		ppd_dev_err(ppd, "%s pkey[2] already set to 0x%x, resetting it to 0x%x\n",
			    __func__, ppd->pkeys[2], OPA_FULL_MGMT_PKEY);
	ppd->pkeys[2] = OPA_FULL_MGMT_PKEY;

	hfi_set_ib_cfg(ppd, HFI_IB_CFG_PKEYS, 0, NULL);
	hfi_event_pkey_change(&ppd->dd->ibd->rdi.ibdev, ppd->pnum);
}

void hfi_clear_full_mgmt_pkey(struct hfi_pportdata *ppd)
{
	if (ppd->pkeys[2] != 0) {
		ppd->pkeys[2] = 0;
		hfi_set_ib_cfg(ppd, HFI_IB_CFG_PKEYS, 0, NULL);
		hfi_event_pkey_change(&ppd->dd->ibd->rdi.ibdev, ppd->pnum);
	}
}

static void hfi_set_sc_to_vlr(struct hfi_pportdata *ppd, u8 *t)
{
	u64 reg_val = 0;
	int i, j, sc_num;

	/*
	 * FXRTODO: Cached table will not only be
	 * read by get_sma but also by verbs layer post
	 * initialization. So this needs to be protected
	 * by lock. If we use query_port, then all
	 * the tables reported by that call needs to hold
	 * a lock. Design decision pending.
	 */
	if (ppd->dd->emulation)
		t[1] = 1;
	/*
	 * We modify the table below, so keep a copy before modification.
	 * The modification is to avoid programming 0xF into non-SC15
	 * hardware registers
	 */
	memcpy(ppd->sc_to_vlr, t, OPA_MAX_SCS);

	/*
	 * change mappings to VL15 to HFI_ILLEGAL_VL (except
	 * for SC15, which must map to VL15). If we don't remap things this
	 * way it is possible for VL15 counters to increment when we try to
	 * send on a SC which is mapped to an invalid VL.
	 */
	for (i = 0; i < OPA_MAX_SCS; i++) {
		if (i == 15)
			continue;
		if ((t[i] & 0x1f) == 0xf)
			t[i] = HFI_ILLEGAL_VL;
	}

	/* 2 registers, 16 entries per register, 4 bit per entry */
	for (i = 0, sc_num = 0; i < 2; i++) {
		for (j = 0, reg_val = 0; j < 16; j++, sc_num++)
			reg_val |= (t[sc_num] & HFI_SC_VLR_MASK)
					 << (j * 4);
		write_csr(ppd->dd, FXR_FPC_CFG_SC_VL_TABLE_15_0 +
				 i * 8, reg_val);
	}
}

static void hfi_set_sc_to_vlt(struct hfi_pportdata *ppd, u8 *t)
{
	u64 reg_val = 0;
	int i, j, sc_num;

	/*
	 * FXRTODO: Cached table will not only be
	 * read by get_sma but also by verbs layer post
	 * initialization. So this needs to be protected
	 * by lock. If we use query_port, then all
	 * the tables reported by that call needs to hold
	 * a lock. Design decision pending.
	 */
	if (ppd->dd->emulation)
		t[1] = 1;

	/*
	 * We modify the table below, so keep a copy before modification.
	 * The modification is to avoid programming 0xF into non-SC15
	 * hardware registers
	 */
	memcpy(ppd->sc_to_vlt, t, OPA_MAX_SCS);

	/*
	 * change mappings to VL15 to HFI_ILLEGAL_VL (except
	 * for SC15, which must map to VL15). If we don't remap things this
	 * way it is possible for VL15 counters to increment when we try to
	 * send on a SC which is mapped to an invalid VL.
	 */
	for (i = 0; i < OPA_MAX_SCS; i++) {
		if (i == 15)
			continue;
		if ((t[i] & 0x1f) == 0xf)
			t[i] = HFI_ILLEGAL_VL;
	}

	/* 2 registers, 16 entries per register, 4 bit per entry */
	for (i = 0, sc_num = 0; i < 2; i++) {
		for (j = 0, reg_val = 0; j < 16; j++, sc_num++)
			reg_val |= (t[sc_num] & HFI_SC_VLT_MASK)
					 << (j * 4);
		write_csr(ppd->dd, FXR_TP_CFG_SC_TO_VLT_MAP +
					i * 8, reg_val);
	}
}

/* convert a vCU to a CU */
static u32 hfi_vcu_to_cu(u8 vcu)
{
	return 1 << vcu;
}

/* convert a vAU to an AU */
static inline u32 hfi_vau_to_au(u8 vau)
{
	return 8 * (1 << vau);
}

/* convert a CU to a vCU */
static inline u8 hfi_cu_to_vcu(u32 cu)
{
	return ilog2(cu);
}

/*
 * Fill out the given AU table using the given CU.  A CU is defined in terms
 * AUs.  The table is a an encoding: given the index, how many AUs does that
 * represent?
 */
static void hfi_assign_cm_au_table(struct hfi_pportdata *ppd, u32 cu,
				   u32 offset)
{
	u64 r0 = 0;
	u64 r1 = 0;

	r0 |= (FXR_TP_CFG_CM_REMOTE_CRDT_RETURN_TBL0_ENTRY0_MASK & 0ull) <<
	       FXR_TP_CFG_CM_REMOTE_CRDT_RETURN_TBL0_ENTRY0_SHIFT;
	r0 |= (FXR_TP_CFG_CM_REMOTE_CRDT_RETURN_TBL0_ENTRY1_MASK & 1ull) <<
	       FXR_TP_CFG_CM_REMOTE_CRDT_RETURN_TBL0_ENTRY1_SHIFT;
	r0 |= (FXR_TP_CFG_CM_REMOTE_CRDT_RETURN_TBL0_ENTRY2_MASK &
	      (2ull * cu)) <<
	       FXR_TP_CFG_CM_REMOTE_CRDT_RETURN_TBL0_ENTRY2_SHIFT;
	r0 |= (FXR_TP_CFG_CM_REMOTE_CRDT_RETURN_TBL0_ENTRY3_MASK &
	      (4ull * cu)) <<
	       FXR_TP_CFG_CM_REMOTE_CRDT_RETURN_TBL0_ENTRY3_SHIFT;
	r1 |= (FXR_TP_CFG_CM_REMOTE_CRDT_RETURN_TBL1_ENTRY4_MASK &
	      (8ull * cu)) <<
	       FXR_TP_CFG_CM_REMOTE_CRDT_RETURN_TBL1_ENTRY4_SHIFT;
	r1 |= (FXR_TP_CFG_CM_REMOTE_CRDT_RETURN_TBL1_ENTRY5_MASK &
	      (16ull * cu)) <<
	       FXR_TP_CFG_CM_REMOTE_CRDT_RETURN_TBL1_ENTRY5_SHIFT;
	r1 |= (FXR_TP_CFG_CM_REMOTE_CRDT_RETURN_TBL1_ENTRY6_MASK &
	      (32ull * cu)) <<
	       FXR_TP_CFG_CM_REMOTE_CRDT_RETURN_TBL1_ENTRY6_SHIFT;
	r1 |= (FXR_TP_CFG_CM_REMOTE_CRDT_RETURN_TBL1_ENTRY7_MASK &
	      (64ull * cu)) <<
	       FXR_TP_CFG_CM_REMOTE_CRDT_RETURN_TBL1_ENTRY7_SHIFT;

	write_csr(ppd->dd, offset, r0);
	write_csr(ppd->dd, offset + 0x08, r1);
}

static void hfi_assign_local_cm_au_table(struct hfi_pportdata *ppd, u8 vcu)
{
	hfi_assign_cm_au_table(ppd, hfi_vcu_to_cu(vcu),
			       FXR_TP_CFG_CM_REMOTE_CRDT_RETURN_TBL0);
}

void hfi_assign_remote_cm_au_table(struct hfi_pportdata *ppd, u8 vcu)
{
	hfi_assign_cm_au_table(ppd, hfi_vcu_to_cu(vcu),
			       FXR_TP_CFG_CM_LOCAL_CRDT_RETURN_TBL0);
}

static u16 hfi_get_dedicated_crdt(struct hfi_pportdata *ppd, int vl_idx)
{
	u64 reg;
	u32 addr = FXR_TP_CFG_CM_DEDICATED_VL_CRDT_LIMIT + 8 * vl_idx;

	reg = read_csr(ppd->dd, addr);
	reg = (reg >> FXR_TP_CFG_CM_DEDICATED_VL_CRDT_LIMIT_DEDICATED_SHIFT) &
	       FXR_TP_CFG_CM_DEDICATED_VL_CRDT_LIMIT_DEDICATED_MASK;
	return reg;
}

static u16 hfi_get_shared_crdt(struct hfi_pportdata *ppd, int vl_idx)
{
	u64 reg;
	u32 addr = FXR_TP_CFG_CM_SHARED_VL_CRDT_LIMIT + 8 * vl_idx;

	reg = read_csr(ppd->dd, addr);
	reg = (reg >> FXR_TP_CFG_CM_SHARED_VL_CRDT_LIMIT_SHARED_SHIFT) &
	       FXR_TP_CFG_CM_SHARED_VL_CRDT_LIMIT_SHARED_MASK;
	return reg;
}

/*
 * Read the current credit merge limits.
 */
void hfi_get_buffer_control(struct hfi_pportdata *ppd,
			    struct buffer_control *bc, u16 *overall_limit)
{
	u64 reg, lim;
	int i;

	/* not all entries are filled in */
	memset(bc, 0, sizeof(*bc));

	/* Reg offset 0-9 corresponds to VL0-8 & VL15 */
	for (i = 0; i <= HFI_NUM_DATA_VLS; i++) {
		int vl_num = hfi_idx_to_vl(i);
		struct vl_limit *vll = &bc->vl[vl_num];

		vll->dedicated = cpu_to_be16(hfi_get_dedicated_crdt(ppd, i));
		vll->shared = cpu_to_be16(hfi_get_shared_crdt(ppd, i));
	}

	reg = read_csr(ppd->dd, FXR_TP_CFG_CM_GLOBAL_SHRD_CRDT_LIMIT);
	lim = reg & FXR_TP_CFG_CM_GLOBAL_SHRD_CRDT_LIMIT_SHARED_MASK;
	bc->overall_shared_limit = cpu_to_be16(lim);

	lim = (reg >> FXR_TP_CFG_CM_GLOBAL_SHRD_CRDT_LIMIT_TOTAL_CREDIT_LIMIT_SHIFT) &
	      FXR_TP_CFG_CM_GLOBAL_SHRD_CRDT_LIMIT_TOTAL_CREDIT_LIMIT_MASK;
	if (overall_limit)
		*overall_limit = lim;
}

static void hfi_nonzero_msg(struct hfi_devdata *dd, int idx, const char *what,
			    u16 limit)
{
	if (limit != 0)
		dd_dev_info(dd, "Invalid %s limit %d on VL %d, ignoring\n",
			    what, (int)limit, idx);
}

static void hfi_set_vau(struct hfi_pportdata *ppd, u8 vau)
{
	u64 reg;

	reg = read_csr(ppd->dd, FXR_TP_CFG_CM_CRDT_SEND);
	reg &= ~FXR_TP_CFG_CM_CRDT_SEND_AU_SMASK;
	reg |= (FXR_TP_CFG_CM_CRDT_SEND_AU_MASK & vau) <<
		FXR_TP_CFG_CM_CRDT_SEND_AU_SHIFT;
	write_csr(ppd->dd, FXR_TP_CFG_CM_CRDT_SEND, reg);
}

static void hfi_set_global_shared(struct hfi_pportdata *ppd, u16 limit)
{
	u64 reg;

	reg = read_csr(ppd->dd, FXR_TP_CFG_CM_GLOBAL_SHRD_CRDT_LIMIT);
	reg &= ~FXR_TP_CFG_CM_GLOBAL_SHRD_CRDT_LIMIT_SHARED_SMASK;
	reg |= (FXR_TP_CFG_CM_GLOBAL_SHRD_CRDT_LIMIT_SHARED_MASK & limit) <<
		FXR_TP_CFG_CM_GLOBAL_SHRD_CRDT_LIMIT_SHARED_SHIFT;
	write_csr(ppd->dd, FXR_TP_CFG_CM_GLOBAL_SHRD_CRDT_LIMIT, reg);
}

static void hfi_set_global_limit(struct hfi_pportdata *ppd, u16 limit)
{
	u64 reg;

	reg = read_csr(ppd->dd, FXR_TP_CFG_CM_GLOBAL_SHRD_CRDT_LIMIT);
	reg &= ~FXR_TP_CFG_CM_GLOBAL_SHRD_CRDT_LIMIT_TOTAL_CREDIT_LIMIT_SMASK;
	reg |= (FXR_TP_CFG_CM_GLOBAL_SHRD_CRDT_LIMIT_TOTAL_CREDIT_LIMIT_MASK &
		limit) <<
		FXR_TP_CFG_CM_GLOBAL_SHRD_CRDT_LIMIT_TOTAL_CREDIT_LIMIT_SHIFT;
	write_csr(ppd->dd, FXR_TP_CFG_CM_GLOBAL_SHRD_CRDT_LIMIT, reg);
}

static void hfi_set_vl_dedicated(struct hfi_pportdata *ppd, int vl, u16 limit)
{
	u64 reg;
	u32 addr = FXR_TP_CFG_CM_DEDICATED_VL_CRDT_LIMIT +
		   (8 * hfi_vl_to_idx(vl));

	reg = read_csr(ppd->dd, addr);
	reg &= ~FXR_TP_CFG_CM_DEDICATED_VL_CRDT_LIMIT_DEDICATED_SMASK;
	reg |= (FXR_TP_CFG_CM_DEDICATED_VL_CRDT_LIMIT_DEDICATED_MASK &
		limit) <<
		FXR_TP_CFG_CM_DEDICATED_VL_CRDT_LIMIT_DEDICATED_SHIFT;
	write_csr(ppd->dd, addr, reg);
}

static void hfi_set_vl_shared(struct hfi_pportdata *ppd, int vl, u16 limit)
{
	u64 reg;
	u32 addr = FXR_TP_CFG_CM_SHARED_VL_CRDT_LIMIT + (8 * hfi_vl_to_idx(vl));

	reg = read_csr(ppd->dd, addr);
	reg &= ~FXR_TP_CFG_CM_SHARED_VL_CRDT_LIMIT_SHARED_SMASK;
	reg |= (FXR_TP_CFG_CM_SHARED_VL_CRDT_LIMIT_SHARED_MASK & limit) <<
		FXR_TP_CFG_CM_SHARED_VL_CRDT_LIMIT_SHARED_SHIFT;
	write_csr(ppd->dd, addr, reg);
}

/*
 * Set up initial VL15 credits of the remote.  Assumes the rest of
 * the CM credit registers are zero from a previous global or credit reset .
 */
void hfi_set_up_vl15(struct hfi_pportdata *ppd, u8 vau, u16 vl15buf)
{
	/* leave shared count at zero for both global and VL15 */
	hfi_set_global_shared(ppd, 0);
	hfi_set_global_limit(ppd, vl15buf);
	hfi_set_vau(ppd, vau);
	hfi_set_vl_dedicated(ppd, 15, vl15buf);

	/* FXRTODO: Implement snoop mode ? */
#if 0

	/* We may need some credits for another VL when sending packets
	 * with the snoop interface. Dividing it down the middle for VL15
	 * and VL0 should suffice.
	 */
	if (unlikely(dd->hfi1_snoop.mode_flag == HFI1_PORT_SNOOP_MODE)) {
		write_csr(dd, SEND_CM_CREDIT_VL15, (u64)(vl15buf >> 1)
		    << SEND_CM_CREDIT_VL15_DEDICATED_LIMIT_VL_SHIFT);
		write_csr(dd, SEND_CM_CREDIT_VL, (u64)(vl15buf >> 1)
		    << SEND_CM_CREDIT_VL_DEDICATED_LIMIT_VL_SHIFT);
	} else {
		write_csr(dd, SEND_CM_CREDIT_VL15, (u64)vl15buf
			<< SEND_CM_CREDIT_VL15_DEDICATED_LIMIT_VL_SHIFT);
	}
#endif
}

/* spin until the given per-VL status mask bits clear */
static void hfi_wait_for_vl_status_clear(struct hfi_pportdata *ppd, u64 mask,
					 const char *which)
{
	unsigned long timeout;
	u64 reg;

	timeout = jiffies + msecs_to_jiffies(HFI_VL_STATUS_CLEAR_TIMEOUT);
	while (1) {
		reg = read_csr(ppd->dd, FXR_TP_CFG_CM_CRDT_SEND) & mask;

		if (reg == 0)
			return;	/* success */
		if (time_after(jiffies, timeout))
			break;		/* timed out */
		udelay(1);
	}

	ppd_dev_err(ppd, "%s credit change status not clearing after %dms, mask 0x%llx, not clear 0x%llx\n"
		   , which, HFI_VL_STATUS_CLEAR_TIMEOUT, mask, reg);
	/*
	 * If this occurs, it is likely there was a credit loss on the link.
	 * The only recovery from that is a link bounce.
	 */
	ppd_dev_err(ppd, "Continuing anyway.  A credit loss may occur. Suggest a link bounce\n");
}

/*
 * The number of credits on the VLs may be changed while everything
 * is "live", but the following algorithm must be followed due to
 * how the hardware is actually implemented.  In particular,
 * Return_Credit_Status[] is the only correct status check.
 *
 * if (reducing Global_Shared_Credit_Limit or any shared limit changing)
 *     set Global_Shared_Credit_Limit = 0
 *     use_all_vl = 1
 * mask0 = all VLs that are changing either dedicated or shared limits
 * set Shared_Limit[mask0] = 0
 * spin until Return_Credit_Status[use_all_vl ? all VL : mask0] == 0
 * if (changing any dedicated limit)
 *     mask1 = all VLs that are lowering dedicated limits
 *     lower Dedicated_Limit[mask1]
 *     spin until Return_Credit_Status[mask1] == 0
 *     raise Dedicated_Limits
 * raise Shared_Limits
 * raise Global_Shared_Credit_Limit
 *
 * lower = if the new limit is lower, set the limit to the new value
 * raise = if the new limit is higher than the current value (may be changed
 *	earlier in the algorithm), set the new limit to the new value
 */
int hfi_set_buffer_control(struct hfi_pportdata *ppd,
			   struct buffer_control *new_bc)
{
	struct hfi_devdata *dd = ppd->dd;
	u64 changing_mask, ld_mask, stat_mask;
	int change_count;
	int i, use_all_mask;
	struct buffer_control cur_bc;
	u8 changing[OPA_MAX_VLS];
	u8 lowering_dedicated[OPA_MAX_VLS];
	u16 cur_total;
	u32 new_total = 0;
	const u64 all_mask =
		FXR_TP_CFG_CM_CRDT_SEND_RETURN_CREDIT_STATUS_MASK <<
		FXR_TP_CFG_CM_CRDT_SEND_RETURN_CREDIT_STATUS_SHIFT;

	/* find the new total credits, do sanity check on unused VLs */
	for (i = 0; i < OPA_MAX_VLS; i++) {
		if (hfi_valid_vl(i)) {
			new_total += be16_to_cpu(new_bc->vl[i].dedicated);
			continue;
		}
		hfi_nonzero_msg(dd, i, "dedicated",
				be16_to_cpu(new_bc->vl[i].dedicated));
		hfi_nonzero_msg(dd, i, "shared",
				be16_to_cpu(new_bc->vl[i].shared));
		new_bc->vl[i].dedicated = 0;
		new_bc->vl[i].shared = 0;
	}
	new_total += be16_to_cpu(new_bc->overall_shared_limit);

	/* fetch the current values */
	hfi_get_buffer_control(ppd, &cur_bc, &cur_total);

	/*
	 * Create the masks we will use.
	 */
	memset(changing, 0, sizeof(changing));
	memset(lowering_dedicated, 0, sizeof(lowering_dedicated));
	/*
	 * Assumes that the individual VL bits are adjacent and in
	 * increasing order
	 */
	stat_mask = 1 << FXR_TP_CFG_CM_CRDT_SEND_RETURN_CREDIT_STATUS_SHIFT;
	changing_mask = 0;
	ld_mask = 0;
	change_count = 0;
	for (i = 0; i < HFI_NUM_USABLE_VLS; i++, stat_mask <<= 1) {
		if (!hfi_valid_vl(i))
			continue;
		if (new_bc->vl[i].dedicated != cur_bc.vl[i].dedicated ||
		    new_bc->vl[i].shared != cur_bc.vl[i].shared) {
			changing[i] = 1;
			changing_mask |= stat_mask;
			change_count++;
		}
		if (be16_to_cpu(new_bc->vl[i].dedicated) <
		    be16_to_cpu(cur_bc.vl[i].dedicated)) {
			lowering_dedicated[i] = 1;
			ld_mask |= stat_mask;
		}
	}

	/* bracket the credit change with a total adjustment */
	if (new_total > cur_total)
		hfi_set_global_limit(ppd, new_total);

	/*
	 * Start the credit change algorithm.
	 */
	use_all_mask = 0;
	if ((be16_to_cpu(new_bc->overall_shared_limit) <
	    be16_to_cpu(cur_bc.overall_shared_limit))) {
		hfi_set_global_shared(ppd, 0);
		cur_bc.overall_shared_limit = 0;
		use_all_mask = 1;
	}

	for (i = 0; i < HFI_NUM_USABLE_VLS; i++) {
		if (!hfi_valid_vl(i))
			continue;

		if (changing[i]) {
			hfi_set_vl_shared(ppd, i, 0);
			cur_bc.vl[i].shared = 0;
		}
	}

	hfi_wait_for_vl_status_clear(ppd, use_all_mask ? all_mask :
				     changing_mask, "shared");

	if (change_count > 0) {
		for (i = 0; i < HFI_NUM_USABLE_VLS; i++) {
			if (!hfi_valid_vl(i))
				continue;

			if (lowering_dedicated[i]) {
				u16 dvl = be16_to_cpu(new_bc->vl[i].dedicated);

				hfi_set_vl_dedicated(ppd, i, dvl);
				cur_bc.vl[i].dedicated =
						new_bc->vl[i].dedicated;
			}
		}

		hfi_wait_for_vl_status_clear(ppd, ld_mask, "dedicated");

		/* now raise all dedicated that are going up */
		for (i = 0; i < HFI_NUM_USABLE_VLS; i++) {
			if (!hfi_valid_vl(i))
				continue;

			if (be16_to_cpu(new_bc->vl[i].dedicated) >
			    be16_to_cpu(cur_bc.vl[i].dedicated)) {
				u16 dvl = be16_to_cpu(new_bc->vl[i].dedicated);

				hfi_set_vl_dedicated(ppd, i, dvl);
			}
		}
	}

	/* next raise all shared that are going up */
	for (i = 0; i < HFI_NUM_USABLE_VLS; i++) {
		if (!hfi_valid_vl(i))
			continue;

		if (be16_to_cpu(new_bc->vl[i].shared) >
		    be16_to_cpu(cur_bc.vl[i].shared)) {
			u16 svl = be16_to_cpu(new_bc->vl[i].shared);

			hfi_set_vl_shared(ppd, i, svl);
		}
	}

	/* finally raise the global shared */
	if (be16_to_cpu(new_bc->overall_shared_limit) >
	    be16_to_cpu(cur_bc.overall_shared_limit)) {
		u16 ovl = be16_to_cpu(new_bc->overall_shared_limit);

		hfi_set_global_shared(ppd, ovl);
	}

	/* bracket the credit change with a total adjustment */
	if (new_total < cur_total)
		hfi_set_global_limit(ppd, new_total);

	/*
	 * Determine the actual number of operational VLs using the
	 * number of dedicated and shared credits for each VL
	 */
	if (change_count > 0) {
		ppd->actual_vls_operational = 0;
		for (i = 0; i < HFI_NUM_DATA_VLS - 1; i++)
			if (be16_to_cpu(new_bc->vl[i].dedicated) > 0 ||
			    be16_to_cpu(new_bc->vl[i].shared) > 0)
				ppd->actual_vls_operational++;
	}
	return 0;
}

static void hfi_set_sc_to_vlnt(struct hfi_pportdata *ppd, u8 *t)
{
	int i, j, sc_num;
	u64 reg_val;

	if (ppd->dd->emulation)
		t[1] = 1;
	/* 2 registers, 16 entries per register, 4 bit per entry */
	for (i = 0, sc_num = 0; i < 2; i++) {
		for (j = 0, reg_val = 0; j < 16; j++, sc_num++)
			reg_val |= (t[sc_num] & HFI_SC_VLNT_MASK)
					 << (j * 4);
		write_csr(ppd->dd, FXR_TP_CFG_CM_SC_TO_VLT_MAP +
				i * 8, reg_val);
	}

	memcpy(ppd->sc_to_vlnt, t, OPA_MAX_SCS);
}

static void hfi_get_bw_arb(struct hfi_pportdata *ppd, int section,
			   struct opa_bw_arb *bw_arb)
{
	struct bw_arb_cache *cache = &ppd->bw_arb_cache;
	union opa_bw_arb_table *arb_block = bw_arb->arb_block;

	spin_lock(&cache->lock);

	switch (section) {
	case OPA_BWARB_GROUP:
		memcpy(arb_block->bw_group, cache->bw_group,
		       sizeof(cache->bw_group));
		break;
	case OPA_BWARB_PREEMPT_MATRIX:
		memcpy(arb_block->matrix, cache->preempt_matrix,
		       sizeof(cache->preempt_matrix));
		break;
	default:
		ppd_dev_err(ppd, "Invalid section when setting BW ARB table\n");
	}

	spin_unlock(&cache->lock);
}

static void hfi_get_sl_pairs(struct hfi_pportdata *ppd, u8 *sl_pairs)
{
	memcpy(sl_pairs, ppd->sl_pairs, sizeof(ppd->sl_pairs));
}

void hfi_shutdown_led_override(struct hfi_pportdata *ppd)
{
	if (no_mnh)
		return;
	/*
	 * This pairs with the memory barrier in hfi_start_led_override to
	 * ensure that we read the correct state of LED beaconing represented
	 * by led_override_timer_active
	 */
	smp_rmb();
	if (atomic_read(&ppd->led_override_timer_active)) {
		del_timer_sync(&ppd->led_override_timer);
		atomic_set(&ppd->led_override_timer_active, 0);
		/* Ensure the atomic_set is visible to all CPUs */
		smp_wmb();
	}

	/* Hand control of the LED to the HW for normal operation */
	write_csr(ppd->dd, OC_LCB_CFG_LED, 0);
}

void hfi_set_ext_led(struct hfi_pportdata *ppd, u32 on)
{
	if (no_mnh)
		return;
	if (on)
		write_csr(ppd->dd, OC_LCB_CFG_LED, 0x1F);
	else
		write_csr(ppd->dd, OC_LCB_CFG_LED, 0x10);
}

static void hfi_run_led_override(struct timer_list *t)
{
	struct hfi_pportdata *ppd = from_timer(ppd, t, led_override_timer);
	unsigned long timeout;
	int phase_idx;

	if (no_mnh)
		return;
	phase_idx = ppd->led_override_phase & 1;

	hfi_set_ext_led(ppd, phase_idx);

	timeout = ppd->led_override_vals[phase_idx];

	/* Set up for next phase */
	ppd->led_override_phase = !ppd->led_override_phase;

	mod_timer(&ppd->led_override_timer, jiffies + timeout);
}

/*
 * To have the LED blink in a particular pattern, provide timeon and timeoff
 * in milliseconds.
 * To turn off custom blinking and return to normal operation, use
 * shutdown_led_override()
 */
void hfi_start_led_override(struct hfi_pportdata *ppd, unsigned int time_on,
			    unsigned int time_off)
{
	if (no_mnh)
		return;
	/* Convert to jiffies for direct use in timer */
	ppd->led_override_vals[0] = msecs_to_jiffies(time_off);
	ppd->led_override_vals[1] = msecs_to_jiffies(time_on);

	/* Arbitrarily start from LED on phase */
	ppd->led_override_phase = 1;

	/*
	 * If the timer has not already been started, do so. Use a "quick"
	 * timeout so the handler will be called soon to look at our request.
	 */
	if (!timer_pending(&ppd->led_override_timer)) {
		timer_setup(&ppd->led_override_timer, hfi_run_led_override,
			    0);
		ppd->led_override_timer.expires = jiffies + 1;
		add_timer(&ppd->led_override_timer);
		atomic_set(&ppd->led_override_timer_active, 1);
		/* Ensure the atomic_set is visible to all CPUs */
		smp_wmb();
	}
}

int hfi_get_ib_cfg(struct hfi_pportdata *ppd, int which, u32 val, void *data)
{
	struct hfi_devdata *dd = ppd->dd;
	int ret = 0;

	switch (which) {
	case HFI_IB_CFG_OP_VLS:
		ret = ppd->actual_vls_operational;
		break;
	case HFI_IB_CFG_BW_ARB:
		hfi_get_bw_arb(ppd, val, data);
		break;
	case HFI_IB_CFG_SL_PAIRS:
		hfi_get_sl_pairs(ppd, data);
		break;
	default:
		dd_dev_info(dd, "%s: which %d: not implemented\n",
			    __func__, which);
		break;
	}

	return ret;
}

/*
 * Returns true if the SL is part of an SL pair and the given SL
 * is the request SL.
 */
bool hfi_is_req_sl(struct hfi_pportdata *ppd, u8 sl)
{
	int num_sls = ARRAY_SIZE(ppd->sl_pairs);

	return (sl < num_sls &&  ppd->sl_to_sc[sl] != HFI_INVALID_RESP_SL &&
		ppd->sl_pairs[sl] != HFI_INVALID_RESP_SL &&
		ppd->sl_pairs[sl] < num_sls);
}

/*
 * Returns true if the SL is part of an SL pair and the given SL
 * is the response SL.
 */
static bool hfi_is_resp_sl(struct hfi_pportdata *ppd, u8 sl)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ppd->sl_pairs); i++)
		if (hfi_is_req_sl(ppd, i) &&
		    ppd->sl_pairs[i] == sl)
			return true;
	return false;
}

/*
 * Returns true if the SL is part of an SL pair.
 * If true, then the response SL is also returned in resp_sl.
 */
static bool hfi_is_sl_pair(struct hfi_pportdata *ppd, u8 sl, u8 *resp_sl)
{
	bool is_req = hfi_is_req_sl(ppd, sl);

	if (is_req || hfi_is_resp_sl(ppd, sl)) {
		if (resp_sl)
			*resp_sl = is_req ? ppd->sl_pairs[sl] : sl;
		return true;
	}

	return false;
}

/*
 * Check an SL pair against the driver list of SL pairs from FM
 * Returns 0 for success and -EINVAL for failure
 */
int hfi_check_sl_pair(struct hfi_ibcontext *uc, struct hfi_sl_pair *slp)
{
	struct hfi_devdata *dd = uc->priv;
	struct hfi_pportdata *ppd;

	if (slp->port <= 0 || slp->port > dd->num_pports)
		return -EINVAL;

	ppd = to_hfi_ppd(dd, slp->port);
	return (hfi_is_req_sl(ppd, slp->sl1) &&
		ppd->sl_pairs[slp->sl1] == slp->sl2) ? 0 : -EINVAL;
}

/*
 * Use sl_to_mctc table to program MC/TC HW
 */
static void hfi_set_mctc_regs(struct hfi_pportdata *ppd)
{
	struct hfi_devdata *dd = ppd->dd;
	u64 reg[2];
	u32 tx_reg_addr, rx_reg_addr;
	int nregs = ARRAY_SIZE(reg);
	int i, j, k;

	tx_reg_addr = FXR_TXCID_CFG_SL0_TO_TC;
	rx_reg_addr = FXR_RXCID_CFG_SL0_TO_TC;

	/* Read from the TX and update both TX/RX mappings */
	for (i = 0; i < nregs; i++)
		reg[i] = read_csr(dd, tx_reg_addr + i * 8);

	for (i = 0, j = 0; i < nregs; i++)
		for (k = 0; k < 64; k += 4, j++)
			hfi_set_bits(&reg[i], ppd->sl_to_mctc[j],
				     HFI_SL_TO_TC_MC_MASK, k);

	/* FXRTODO: Can these be changed on the fly while apps are running */
	/* TX & RX SL -> TC/MC mappings should be identical */
	for (i = 0; i < nregs; i++) {
		write_csr(dd, tx_reg_addr + i * 8, reg[i]);
		write_csr(dd, rx_reg_addr + i * 8, reg[i]);
	}
}

int hfi_get_sl_for_mctc(struct hfi_pportdata *ppd, u8 mc, u8 tc, u8 *sl)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ppd->sl_to_mctc); i++) {
		if (ppd->sl_to_mctc[i] == HFI_GET_MCTC(mc, tc)) {
			*sl = i;
			return 0;
		}
	}

	return -EPERM;
}

/* Service level to message class and traffic class mapping */
static void hfi_sl_to_mctc(struct hfi_pportdata *ppd)
{
	int i, tc = 0;
	int num_sls = ARRAY_SIZE(ppd->sl_pairs);

	/* set up SL to MCTC for SL pairs */
	for (i = 0; i < num_sls; i++) {
		int resp_sl;

		if (!hfi_is_req_sl(ppd, i))
			continue;

		resp_sl = ppd->sl_pairs[i];

		ppd->sl_to_mctc[i] = HFI_GET_MCTC(0, tc);
		ppd->sl_to_mctc[resp_sl] = HFI_GET_MCTC(1, tc);

		/* Round robin through available TCs. Locking? */
		tc = (tc + 1) % HFI_MAX_TC;
		if (tc == HFI_VL15_TC)
			tc = (tc + 1) % HFI_MAX_TC;
	}

	/* set up SL to MCTC for normal SLs */
	for (i = 0; i < ARRAY_SIZE(ppd->sl_to_mctc); i++) {
		if (hfi_is_sl_pair(ppd, i, NULL))
			continue;
		/*
		 * FXRTODO: FXR Simics model has the sl_to_mctc mapping hard
		 * coded based on the formulas below which are being used in
		 * the driver as well till such time that we have a better
		 * solution.
		 */
		tc = (i % HFI_MAX_TC);
		/* Use TC 0 for any TC clashing with the HFI_VL15_TC */
		tc = tc == HFI_VL15_TC ? 0 : tc;
		/*
		 * Always use MC0 for non-portals. Transmitting non-portals on
		 * MC1 is allowed but we cannot receive anything but portals
		 * on MC1 because of the way the control pipeline is plumbed.
		 */
		ppd->sl_to_mctc[i] = HFI_GET_MCTC(0, tc);
	}

	hfi_set_mctc_regs(ppd);
}

/* Service channel to message class and traffic class mapping */
static void hfi_sc_to_mctc(struct hfi_pportdata *ppd, void *data)
{
	struct hfi_devdata *dd = ppd->dd;
	u8 *sc_to_sl = data;
	u64 reg[2];
	u32 reg_addr;
	int i, j, k;
	int nregs = ARRAY_SIZE(reg);

	reg_addr = FXR_LM_CFG_PORT0_SC2MCTC0;

	for (i = 0; i < nregs; i++)
		reg[i] = read_csr(dd, reg_addr + i * 8);

	/*
	 * In order to obtain the SC -> MCTC map, obtain the SC to SL map
	 * and then find the SL -> MCTC mapping
	 */
	for (i = 0, j = 0; i < nregs; i++) {
		for (k = 0; k < 64; k += 4, j++) {
			ppd->sc_to_mctc[j] = (j == 15) ? 0x3 :
					     ppd->sl_to_mctc[sc_to_sl[j]];
			hfi_set_bits(&reg[i], ppd->sc_to_mctc[j],
				     HFI_SC_TO_TC_MC_MASK, k);
		}
	}

	for (i = 0; i < nregs; i++)
		write_csr(dd, reg_addr + i * 8, reg[i]);
}

/* Service level to service channel mapping */
static void hfi_sl_to_sc(struct hfi_pportdata *ppd)
{
	struct hfi_devdata *dd = ppd->dd;
	u8 *sl_to_sc = ppd->sl_to_sc;
	u64 reg[4];
	u32 reg_addr;
	int i, j, k;
	int nregs = ARRAY_SIZE(reg);

	if (!sl_to_sc) {
		ppd_dev_warn(ppd, "No sl_to_sc mapping");
		return;
	}

	reg_addr = FXR_LM_CFG_PORT0_SL2SC0;

	for (i = 0; i < nregs; i++)
		reg[i] = read_csr(dd, reg_addr + i * 8);

	for (i = 0, j = 0; i < nregs; i++) {
		for (k = 0; k < 64; k += 8, j++) {
			hfi_set_bits(&reg[i], sl_to_sc[j],
				     HFI_SL_TO_SC_MASK, k);
		}
	}

	for (i = 0; i < nregs; i++)
		write_csr(dd, reg_addr + i * 8, reg[i]);
}

/* Service channel to service level mapping */
static void hfi_sc_to_resp_sl(struct hfi_pportdata *ppd, void *data)
{
	struct hfi_devdata *dd = ppd->dd;
	u8 *sc_to_sl = data;
	u8 *sc_to_resp_sl = ppd->sc_to_resp_sl, resp_sl = 0, sl;
	u64 reg[4];
	u32 reg_addr;
	int i, j, k;
	int nregs = ARRAY_SIZE(reg);

	if (!sc_to_sl) {
		ppd_dev_warn(ppd, "No sc_to_sl mapping");
		return;
	}

	reg_addr = FXR_LM_CFG_PORT0_SC2SL0;

	for (i = 0; i < nregs; i++)
		reg[i] = read_csr(dd, reg_addr + i * 8);

	/*
	 * Received packets are mapped through the Link Mux using
	 * SC*_TO_SL registers to obtain the outgoing SL based on the incoming
	 * SC. These replace the SC in the packet for Hfi2 transport packets
	 * only. The way to determine how this table should be programmed is by
	 * looking up the SC -> request SL mapping provided by the FM for
	 * requests and then finding out the response SL based on the SL->TC/MC
	 * mapping
	 */
	for (i = 0, j = 0; i < nregs; i++) {
		for (k = 0; k < 64; k += 8, j++) {
			if (hfi_is_sl_pair(ppd, sc_to_sl[j], &resp_sl))
				sl = resp_sl;
			else
				sl = sc_to_sl[j];
			sc_to_resp_sl[j] = sl;
			hfi_set_bits(&reg[i], sl, HFI_SC_TO_RESP_SL_MASK, k);
		}
	}

	for (i = 0; i < nregs; i++)
		write_csr(dd, reg_addr + i * 8, reg[i]);
}

static void hfi_set_txdma_mctc_bw(struct hfi_devdata *dd, u8 whole, u8 frac,
				  u8 mctc, bool enable_capping)
{
	u64 cfg = 0;

	cfg = (FXR_TXDMA_CFG_BWMETER_LEAK_AMOUNT_FRACTIONAL_MASK &
	       frac) <<
	       FXR_TXDMA_CFG_BWMETER_LEAK_AMOUNT_FRACTIONAL_SHIFT;
	cfg |= (FXR_TXDMA_CFG_BWMETER_LEAK_AMOUNT_INTEGRAL_MASK &
		whole) <<
		FXR_TXDMA_CFG_BWMETER_LEAK_AMOUNT_INTEGRAL_SHIFT;
	cfg |= (FXR_TXDMA_CFG_BWMETER_ENABLE_CAPPING_MASK &
		enable_capping) <<
		FXR_TXDMA_CFG_BWMETER_ENABLE_CAPPING_SHIFT;
	cfg |= (FXR_TXDMA_CFG_BWMETER_BANDWIDTH_LIMIT_MASK &
		HFI_DEFAULT_BW_LIMIT_FLITS) <<
		FXR_TXDMA_CFG_BWMETER_BANDWIDTH_LIMIT_SHIFT;
	write_csr(dd, FXR_TXDMA_CFG_BWMETER + (8 * mctc), cfg);
}

static void hfi_set_txdma_bw(struct hfi_devdata *dd, u8 whole, u8 frac, u8 tc,
			     bool enable_capping)
{
	hfi_set_txdma_mctc_bw(dd, whole, frac, HFI_GET_MCTC(0, tc),
			      enable_capping);
	hfi_set_txdma_mctc_bw(dd, whole, frac, HFI_GET_MCTC(1, tc),
			      enable_capping);
}

static void hfi_set_rxdma_mctc_bw(struct hfi_devdata *dd, u8 whole, u8 frac,
				  u8 mctc)
{
	u64 cfg;
	int offset = 8 * mctc;

	cfg = read_csr(dd, FXR_RXDMA_CFG_BW_SHAPE + offset);

	cfg &= ~(FXR_RXDMA_CFG_BW_SHAPE_BW_LIMIT_SMASK |
		 FXR_RXDMA_CFG_BW_SHAPE_LEAK_FRACTION_SMASK |
		 FXR_RXDMA_CFG_BW_SHAPE_LEAK_INTEGER_SMASK);

	cfg |= (FXR_RXDMA_CFG_BW_SHAPE_BW_LIMIT_MASK &
		HFI_DEFAULT_BW_LIMIT_FLITS) <<
		FXR_RXDMA_CFG_BW_SHAPE_BW_LIMIT_SHIFT;
	cfg |= (FXR_RXDMA_CFG_BW_SHAPE_LEAK_FRACTION_MASK & frac) <<
		FXR_RXDMA_CFG_BW_SHAPE_LEAK_FRACTION_SHIFT;
	cfg |= (FXR_RXDMA_CFG_BW_SHAPE_LEAK_INTEGER_MASK & whole) <<
		FXR_RXDMA_CFG_BW_SHAPE_LEAK_INTEGER_SHIFT;

	write_csr(dd, FXR_RXDMA_CFG_BW_SHAPE + offset, cfg);
}

static void hfi_set_rxdma_bw(struct hfi_devdata *dd, u8 whole, u8 frac, u8 tc)
{
	hfi_set_rxdma_mctc_bw(dd, whole, frac, HFI_GET_MCTC(0, tc));
	hfi_set_rxdma_mctc_bw(dd, whole, frac, HFI_GET_MCTC(1, tc));
}

static void hfi_set_txotr_mctc_bw(struct hfi_devdata *dd, u8 whole, u8 frac,
				  u8 mctc)
{
	u64 fpe_cfg = 0;
	u64 fp_cfg = 0;
	u64 arb_cfg = 0;

	fpe_cfg = (FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_FPE_BWMETER_BW_LIMIT_MASK &
		   HFI_DEFAULT_BW_LIMIT_FLITS) <<
		   FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_FPE_BWMETER_BW_LIMIT_SHIFT;
	fpe_cfg |= (HFI_FPE_CFG_LEAK_FRACTION_MASK & frac) <<
		   HFI_FPE_CFG_LEAK_FRACTION_SHIFT;
	fpe_cfg |= (HFI_FPE_CFG_LEAK_INTEGER_MASK & whole) <<
		   HFI_FPE_CFG_LEAK_INTEGER_SHIFT;
	write_csr(dd, FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_FPE_BWMETER + (8 * mctc),
		  fpe_cfg);

	fp_cfg = (FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_BWMETER_BW_LIMIT_MASK &
		  HFI_DEFAULT_BW_LIMIT_FLITS) <<
		  FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_BWMETER_BW_LIMIT_SHIFT;
	fp_cfg |= (HFI_FP_CFG_LEAK_INTEGER_MASK & frac) <<
		   HFI_FP_CFG_LEAK_FRACTION_SHIFT;
	fp_cfg |= (HFI_FP_CFG_LEAK_INTEGER_MASK & whole) <<
		   HFI_FP_CFG_LEAK_INTEGER_SHIFT;
	write_csr(dd, FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_BWMETER + (8 * mctc),
		  fp_cfg);

	arb_cfg = (FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_MCTC_BWMETER_BW_LIMIT_MASK &
		    HFI_DEFAULT_BW_LIMIT_FLITS) <<
		    FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_MCTC_BWMETER_BW_LIMIT_SHIFT;
	arb_cfg |= (HFI_MCTC_CFG_LEAK_FRACTION_MASK & frac) <<
		    HFI_MCTC_CFG_LEAK_FRACTION_SHIFT;
	arb_cfg |= (HFI_MCTC_CFG_LEAK_INTEGER_MASK & whole) <<
		    HFI_MCTC_CFG_LEAK_INTEGER_SHIFT;
	write_csr(dd, FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_MCTC_BWMETER + (8 * mctc),
		  arb_cfg);
}

static void hfi_set_txotr_bw(struct hfi_devdata *dd, u8 whole, u8 frac, u8 tc)
{
	hfi_set_txotr_mctc_bw(dd, whole, frac, HFI_GET_MCTC(0, tc));
	hfi_set_txotr_mctc_bw(dd, whole, frac, HFI_GET_MCTC(1, tc));
}

static void hfi_set_tc(struct hfi_pportdata *ppd, int sl, int tc)
{
	int old_mctc = ppd->sl_to_mctc[sl];
	int old_mc = HFI_GET_MC(old_mctc);
	int old_tc = HFI_GET_TC(old_mctc);

	ppd_dev_dbg(ppd, "sl2mctc[%d], mctc (%d,%d)->(%d,%d)\n",
		    sl, old_mc, old_tc, old_mc, tc);
	ppd->sl_to_mctc[sl] = HFI_GET_MCTC(old_mc, tc);
}

static int hfi_realloc_tc(struct hfi_pportdata *ppd, u32 vl_mask, int tc)
{
	int sl;

	if (tc >= HFI_VL15_TC)
		return -EINVAL;

	ppd_dev_dbg(ppd, "realloc mctc to tc %d vl 0x%x\n",
		    tc, vl_mask);

	for (sl = 0; sl < ARRAY_SIZE(ppd->sl_to_sc); sl++) {
		int sc, vlr, vlt;

		sc = ppd->sl_to_sc[sl];
		if (sc >= ARRAY_SIZE(ppd->sc_to_vlt)) {
			ppd_dev_dbg(ppd, "illegal sc %d\n", sc);
			continue;
		}

		vlr = ppd->sc_to_vlr[sc];
		vlt = ppd->sc_to_vlt[sc];

		/*
		 * If sl->sc->vlr or sl->sc->vlt is in the vl mask, this is
		 * a candidate to move to this BW group's TC. Hopefully both
		 * are in the mask.
		 */
		if (vl_mask & (BIT(vlr) | BIT(vlt)))
			hfi_set_tc(ppd, sl, tc);
	}

	return 0;
}

static void hfi_program_mctc_bw_arb(struct hfi_pportdata *ppd, u32 vl_mask,
				    u8 tx_int, u8 tx_frac, u8 rx_int,
				    u8 rx_frac, int tc)
{
	int ret;

	/* Ignore mgmt VLs */
	if (tc == HFI_VL15_TC)
		return;

	hfi_set_rxdma_bw(ppd->dd, rx_int, rx_frac, tc);
	hfi_set_txdma_bw(ppd->dd, tx_int, tx_frac, tc, !HFI_ENABLE_CAPPING);
	hfi_set_txotr_bw(ppd->dd, tx_int, tx_frac, tc);

	ret = hfi_realloc_tc(ppd, vl_mask, tc);
	if (ret) {
		ppd_dev_err(ppd,
			    "realloc mctc err %d: vl_mask 0x%x, group %d\n",
			    ret, vl_mask, tc);
		return;
	}

	hfi_set_mctc_regs(ppd);
	hfi_sc_to_mctc(ppd, ppd->sc_to_sl);
}

static void hfi_set_bw_group(struct hfi_pportdata *ppd,
			     const struct opa_bw_element *bw_group)
{
	int i;
	u32 tx_amt, rx_amt;
	u16 lws;
	u16 lwa;
	u8 tx_int, tx_frac, rx_int, rx_frac;

	lws = (u16)find_last_bit((void *)&ppd->link_width_supported, 16) + 1;
	lwa = (u16)find_first_bit((void *)&ppd->link_width_active, 16) + 1;

	/*
	 * We support 3 bandwidth groups (BGs) to the FM, one for each
	 * TC that we use for data (TC3 is management only). This provides
	 * a nice 1:1 mapping between BG and TC which we will use below.
	 * Take each BG from the FM, calculate the leak amount, and
	 * program both MCs in that TC with the leak amount for the BG.
	 */
	for (i = 0; i < HFI2_NUM_BW_GROUPS; i++) {
		u32 vl_mask;

		u8 prio = bw_group[i].priority;
		u8 perc = bw_group[i].bw_percentage;

		vl_mask = (u32)(be32_to_cpu(bw_group[i].vl_mask));
		if (!vl_mask)
			continue;

		ppd_dev_dbg(ppd, "BW group %d priority %u\n", i, prio);
		ppd_dev_dbg(ppd, "bw_percent = %u\n", perc);
		ppd_dev_dbg(ppd, "vl_mask = %#x\n", vl_mask);
		ppd_dev_dbg(ppd, "link width = %u/%u\n", lwa, lws);

		/* The integral and fractional amounts are calculated
		 * by multiplying the FM supplied percent by the flits
		 * per clock and the ratio of currently enabled lanes.
		 * Since these are 8 bit values, the fractional part is
		 * scaled by 256 before the division. For example:
		 * If a group is given 25% of the BW and the channel is
		 * at full rate, capable of sending 2 flits per clock,
		 * then Leak Amount would be set to 0.5 flits per clock.
		 * With the fraction portion being an 8-bit field,
		 * holding a maximum value of 256, 0.5 is represented
		 * by setting 0 for the integer portion and 128 for its
		 * fractional portion.
		 */
		tx_amt = 256 * perc * lwa * HFI_TX_FLITS_PER_CLK;
		tx_amt /= lws * 100;
		tx_int = (u8)(tx_amt >> 8);
		tx_frac = (u8)tx_amt;

		rx_amt = 256 * perc * lwa * HFI_RX_FLITS_PER_CLK;
		rx_amt /= lws * 100;
		rx_int = (u8)(rx_amt >> 8);
		rx_frac = (u8)rx_amt;

		ppd_dev_dbg(ppd, "tx int amount = %u\n", tx_int);
		ppd_dev_dbg(ppd, "tx frac amount = %u\n", tx_frac);
		ppd_dev_dbg(ppd, "rx int amount = %u\n", rx_int);
		ppd_dev_dbg(ppd, "rx frac amount = %u\n", rx_frac);

		hfi_program_mctc_bw_arb(ppd, vl_mask, tx_int, tx_frac, rx_int,
					rx_frac, i);
	}
}

static void hfi_set_bw_prempt_matrix(struct hfi_pportdata *ppd,
				     const __be32 *matrix)
{
	int i;
	u64 reg = 0;

	ppd_dev_dbg(ppd, " Preemption Matrix :\n");

	for (i = 0; i < TPORT_PREEMPT_R0_DATA_CNT; i++) {
		int vl = i;
		u64 val = (u64)be32_to_cpu(matrix[i]);
		u16 shift = i * TPORT_PREEMPT_BITS_PER_ENTRY;

		ppd_dev_dbg(ppd, "vl %d: 0x%llx shift: %u\n", vl, val, shift);
		val &= TPORT_PREEMPT_DATA_VLS;
		reg |= val << shift;
	}
	ppd_dev_dbg(ppd, "reg 0: 0x%llx\n", reg);

	write_csr(ppd->dd, FXR_TP_CFG_PREEMPT_MATRIX, reg);

	reg = 0;
	for (i = 0; i < TPORT_PREEMPT_R1_DATA_CNT; i++) {
		int vl = i + TPORT_PREEMPT_R0_DATA_CNT;
		u64 val = (u64)be32_to_cpu(matrix[vl]);
		u16 shift = i * TPORT_PREEMPT_BITS_PER_ENTRY;

		ppd_dev_dbg(ppd, "vl %d: 0x%llx shift: %u\n", vl, val, shift);
		val &= TPORT_PREEMPT_DATA_VLS;
		reg |= val << shift;
	}

	/* VL15 can preempt all others. FXRTODO: Is this actually necessary? */
	reg |= TPORT_PREEMPT_DATA_VLS << TPORT_PREEMPT_VL15_SHIFT;

	ppd_dev_dbg(ppd, "reg 1: 0x%llx\n", reg);

	write_csr(ppd->dd, FXR_TP_CFG_PREEMPT_MATRIX + 0x8, reg);

	ppd_dev_dbg(ppd, "mgmt vl: 0x%x\n", be32_to_cpu(matrix[15]));
}

static void hfi_set_bw_arb(struct hfi_pportdata *ppd, int section,
			   const struct opa_bw_arb *bw_arb)
{
	struct bw_arb_cache *cache = &ppd->bw_arb_cache;
	const union opa_bw_arb_table *arb_block = bw_arb->arb_block;

	switch (section) {
	case OPA_BWARB_GROUP:
		spin_lock(&cache->lock);
		memcpy(cache->bw_group, arb_block->bw_group,
		       sizeof(cache->bw_group));
		spin_unlock(&cache->lock);
		hfi_set_bw_group(ppd, arb_block->bw_group);
		break;
	case OPA_BWARB_PREEMPT_MATRIX:
		spin_lock(&cache->lock);
		memcpy(cache->preempt_matrix, arb_block->matrix,
		       sizeof(cache->preempt_matrix));
		spin_unlock(&cache->lock);
		hfi_set_bw_prempt_matrix(ppd, arb_block->matrix);
		break;
	default:
		ppd_dev_err(ppd, "Invalid section when getting BW ARB table\n");
	}
}

static void hfi_set_pkt_format(struct hfi_pportdata *ppd,
			       const u8 *pkt_formats_enabled)
{
	u64 fpc;

	if (*pkt_formats_enabled == OPA_PORT_PACKET_FORMAT_NOP)
		return;

	ppd->packet_format_enabled = *pkt_formats_enabled;
	fpc = read_csr(ppd->dd, FXR_FPC_CFG_PORT_CONFIG);
	fpc &= ~FXR_FPC_CFG_PORT_CONFIG_PKT_FORMATS_ENABLED_SMASK;
	fpc &= ~FXR_FPC_CFG_PORT_CONFIG_PKT_FORMATS_SUPPORTED_SMASK;

	fpc |= (FXR_FPC_CFG_PORT_CONFIG_PKT_FORMATS_ENABLED_MASK &
		*pkt_formats_enabled) <<
		FXR_FPC_CFG_PORT_CONFIG_PKT_FORMATS_ENABLED_SHIFT;
	fpc |= (FXR_FPC_CFG_PORT_CONFIG_PKT_FORMATS_SUPPORTED_MASK &
		ppd->packet_format_supported) <<
		FXR_FPC_CFG_PORT_CONFIG_PKT_FORMATS_SUPPORTED_SHIFT;
	write_csr(ppd->dd, FXR_FPC_CFG_PORT_CONFIG, fpc);
}

static int hfi_set_sl_pairs(struct hfi_pportdata *ppd, const u8 *sl_pairs)
{
	int num_sls = ARRAY_SIZE(ppd->sl_pairs);
	u32 req_sl_mask;
	int i;

	/* only allow valid SLs marked as pairs */
	req_sl_mask = 0;

	for (i = 0 ; i < num_sls; i++) {
		u8 resp = sl_pairs[i];

		if (resp == HFI_INVALID_RESP_SL)
			continue;
		/*
		 * sl_pairs[resp] != HFI_INVALID_RESP_SL
		 * means "Any SL which is itself a response SL shall
		 * map to 0xff". Additionally, do not allow SLs mapped as
		 * SC15 in the SL2SC table.
		 */
		if (resp >= num_sls || sl_pairs[resp] != HFI_INVALID_RESP_SL ||
		    ppd->sl_to_sc[i] == 15) {
			ppd_dev_err(ppd, "Invalid SL pair table\n");
			return -EINVAL;
		}

		set_bit(i, (unsigned long *)&req_sl_mask);
	}

	ppd->req_sl_mask = req_sl_mask;

	memcpy(ppd->sl_pairs, sl_pairs, sizeof(ppd->sl_pairs));
	hfi_set_ib_cfg(ppd, HFI_IB_CFG_SL_TO_MCTC, 0, NULL);
	hfi_set_ib_cfg(ppd, HFI_IB_CFG_SC_TO_RESP_SL, 0, ppd->sc_to_sl);
	hfi_set_ib_cfg(ppd, HFI_IB_CFG_SC_TO_MCTC, 0, ppd->sc_to_sl);
	return 0;
}

int hfi_set_ib_cfg(struct hfi_pportdata *ppd, int which, u32 val, void *data)
{
	struct hfi_devdata *dd = ppd->dd;
	int ret = 0;

	switch (which) {
	case HFI_IB_CFG_LIDLMC:
		hfi_set_lid_lmc(ppd);
		break;
	case HFI_IB_CFG_VL_HIGH_LIMIT:
		/* FXRTODO: Implement FXR equivalent */
#if 0
		/*
		 * The VL Arbitrator high limit is sent in units of 4k
		 * bytes, while HFI stores it in units of 64 bytes.
		 */
		val *= 4096 / 64;
		reg = ((u64)val & SEND_HIGH_PRIORITY_LIMIT_LIMIT_MASK)
			<< SEND_HIGH_PRIORITY_LIMIT_LIMIT_SHIFT;
		write_csr(ppd->dd, SEND_HIGH_PRIORITY_LIMIT, reg);
#endif
		break;
	case HFI_IB_CFG_LWID_DG_ENB:
		ppd->link_width_downgrade_enabled =
			val & ppd->link_width_downgrade_supported;
		break;
	case HFI_IB_CFG_OP_VLS:
		if (ppd->vls_operational != val) {
			ppd->vls_operational = val;
			/* FXRTODO: if we map contexts to VLs, update here. */
		}
		break;
	case HFI_IB_CFG_MTU:
		hfi_set_send_length(ppd);
		break;
	case HFI_IB_CFG_PKEYS:
		/*FXRTODO: Implement HFI capability set/get support in verbs */
#if 0
		if (HFI1_CAP_IS_KSET(PKEY_CHECK))
#endif
			hfi_set_pkey_table(ppd);
		break;
	case HFI_IB_CFG_8B_PKEY:
		hfi_set_implicit_pkeys(ppd, data, NULL);
		break;
	case HFI_IB_CFG_10B_PKEY:
		hfi_set_implicit_pkeys(ppd, NULL, data);
		break;
	case HFI_IB_CFG_SL_TO_SC:
		hfi_sl_to_sc(ppd);
		break;
	case HFI_IB_CFG_SL_TO_MCTC:
		hfi_sl_to_mctc(ppd);
		break;
	case HFI_IB_CFG_SC_TO_RESP_SL:
		hfi_sc_to_resp_sl(ppd, data);
		break;
	case HFI_IB_CFG_SC_TO_MCTC:
		hfi_sc_to_mctc(ppd, data);
		break;
	case HFI_IB_CFG_SC_TO_VLR:
		hfi_set_sc_to_vlr(ppd, data);
		break;
	case HFI_IB_CFG_SC_TO_VLT:
		hfi_set_sc_to_vlt(ppd, data);
		break;
	case HFI_IB_CFG_SC_TO_VLNT:
		hfi_set_sc_to_vlnt(ppd, data);
		break;
	case HFI_IB_CFG_BW_ARB:
		hfi_set_bw_arb(ppd, val, data);
		break;
	case HFI_IB_CFG_PKT_FORMAT:
		hfi_set_pkt_format(ppd, data);
		break;
	case HFI_IB_CFG_SL_PAIRS:
		ret = hfi_set_sl_pairs(ppd, data);
		break;
	default:
		dd_dev_info(dd, "%s: which %d: not implemented\n",
			    __func__, which);
		break;
	}

	return ret;
}

int hfi_set_lid(struct hfi_pportdata *ppd, u32 lid, u8 lmc)
{
	ppd->lid = lid;
	ppd->lmc = lmc;

	hfi_set_small_headers(ppd);

	hfi_set_ib_cfg(ppd, HFI_IB_CFG_LIDLMC, 0, NULL);

	ppd_dev_info(ppd, "IB%u:%u got a lid: 0x%x\n",
		     ppd->dd->unit, ppd->pnum, lid);

	return 0;
}

int hfi_set_max_lid(struct hfi_pportdata *ppd, u32 lid)
{
	struct hfi_devdata *dd = ppd->dd;
	int rc = 0;

	mutex_lock(&ppd->hls_lock);
	if (ppd->max_lid != lid) {
		if (!lid || lid >= HFI1_16B_MULTICAST_LID_BASE ||
		    lid < ppd->lid) {
			rc = -EINVAL;
			ppd_dev_err(ppd, "invalid %s lid 0x%x\n",
				    __func__, lid);
			goto unlock;
		}
		/*
		 * It is an error to modify the PSN state while the link state
		 * is armed or active to avoid disruption to active jobs
		 * FXRTODO: On ZEBU, we hack the link state to Active early
		 * on so it is okay to change the PSN state for now.
		 */
		if (!dd->emulation &&
		    (ppd->host_link_state == HLS_UP_ARMED ||
		     ppd->host_link_state == HLS_UP_ACTIVE)) {
			rc = -EINVAL;
			ppd_dev_err(ppd, "invalid %s state 0x%x\n",
					__func__, ppd->host_link_state);
			goto unlock;
		}
		/* Free the old PSN state first before allocating a new one */
		hfi_psn_uninit(ppd);
		rc = hfi_psn_init(ppd, lid);
		if (!rc) {
			ppd->max_lid = lid;
			hfi_init_max_lid_csrs(ppd);
			/* Workaround used when E2E connections do not work */
			if (use_psn_cache)
				hfi_psn_cache_fill(dd);
			ppd_dev_dbg(ppd, "IB%u:%u max_lid 0x%x\n",
				    dd->unit, ppd->pnum, lid);
		} else {
			ppd_dev_err(ppd, "IB%u:%u %s lid 0x%x rc %d\n",
				    dd->unit, ppd->pnum, __func__, lid, rc);
		}
	}
unlock:
	mutex_unlock(&ppd->hls_lock);
	return rc;
}

/*
 * set_mtu - set the MTU
 * @ppd: the per port data
 *
 * We can handle "any" incoming size, the issue here is whether we
 * need to restrict our outgoing size.  We do not deal with what happens
 * to programs that are already running when the size changes.
 */
int hfi_set_mtu(struct hfi_pportdata *ppd)
{
	/*
	 * FXRTODO: is ibmaxlen needed for FXR ? In WFR
	 * it was only used in egress_cycles.
	 */
#if 0
	ppd->ibmaxlen = ppd->ibmtu + lrh_max_header_bytes(ppd->dd);
#endif

	mutex_lock(&ppd->hls_lock);
	/* FXRTODO: Fix the MTU change dependency for FXR */
#if 0
	if (ppd->host_link_state == HLS_UP_INIT ||
	    ppd->host_link_state == HLS_UP_ARMED ||
	    ppd->host_link_state == HLS_UP_ACTIVE)
		is_up = 1;

	drain = !is_ax(dd) && is_up;

	if (drain)
		/*
		 * MTU is specified per-VL. To ensure that no packet gets
		 * stuck (due, e.g., to the MTU for the packet's VL being
		 * reduced), empty the per-VL FIFOs before adjusting MTU.
		 */
		ret = stop_drain_data_vls(dd);

	if (ret) {
		dd_dev_err(dd, "%s: cannot stop/drain VLs - refusing to change per-VL MTUs\n",
			   __func__);
		goto err;
	}
#endif

	hfi_set_ib_cfg(ppd, HFI_IB_CFG_MTU, 0, NULL);

	/* FXRTODO: Fix the MTU change dependency for FXR */
#if 0
	if (drain)
		open_fill_data_vls(dd); /* reopen all VLs */

err:
#endif
	mutex_unlock(&ppd->hls_lock);

	return 0;
}

/*
 * Apply the link width downgrade enabled policy against the current active
 * link widths.
 *
 * Called when the enabled policy changes or the active link widths change.
 */
void hfi_apply_link_downgrade_policy(struct hfi_pportdata *ppd,
				     int refresh_widths)
{
	/*
	 * FXRTODO: FC dependent link width code.
	 * JIRA STL-66
	 */
}

void hfi_ack_interrupt(struct hfi_irq_entry *me)
{
	struct hfi_devdata *dd = me->dd;
	/*
	 * There are 320 interrupt sources with status bits implemented in 5
	 * 64 bit CSRs. The offset of the CSR to write is computed as below
	 */
	u32 offset = me->intr_src >> 6;
	/*
	 * Writing a 1 to the interrupt source which fired clears the interrupt
	 * whereas a zero has no effect. Clear the interrupt unconditionally
	 * without reading since it must be set if we received an interrupt.
	 */
	write_csr(dd, HFI_PCIM_INT_CLR + offset * 8,
		  BIT_ULL(me->intr_src & 0x3f));
}

static void hfi_ack_all_interrupts(const struct hfi_devdata *dd)
{
	int i;
	u64 val;

	for (i = 0; i <= PCIM_MAX_INT_CSRS; i++) {
		val = read_csr(dd, HFI_PCIM_INT_STS + i * 8);

		/* Clear the interrupt status read */
		if (val)
			write_csr(dd, HFI_PCIM_INT_CLR + i * 8, val);
	}
}

/*
 * FXR defines 320 interrupt vectors, each vector is represented
 * by a bit in INT_STS formed with 5 64bit CSRs (5*64=320).
 *
 * 'i' loop below goes through the 5 CSRs, 'j' loop goes through
 * the 64 bits within a CSR.
 *
 * vectors 0-254 are assigned to EQ operation, which are all in
 * the first 4 CSRs; the rest 64 vectors are in the last CSR.
 * vector 255 is used to notify reciept of FECN/BECN
 * vector 256 is assigned to MNH, 257 and above are assigned to
 * error domain interrupts.
 *
 * The dispatching of interrupt handlers are based on above info.
 */
static irqreturn_t hfi_irq_intx_handler(int irq, void *dev_id)
{
	struct hfi_devdata *dd = dev_id;
	struct hfi_irq_entry *me;
	int i, j, irq_bit;
	u64 val;

	for (i = 0; i <= PCIM_MAX_INT_CSRS; i++) {
		val = read_csr(dd, HFI_PCIM_INT_STS + i * 8);
		if (!val)
			continue;

		for (j = 0; j < 64; j++) {
			if (!(val & (1uLL << j)))
				continue;

			irq_bit = i * 64 + j;
			me = &dd->irq_entries[irq_bit];
			hfi_ack_interrupt(me);

			if (irq_bit < 255)
				hfi_irq_eq_handler(irq, me);
			else if (irq_bit == 255)
				hfi_irq_becn_handler(irq, me);
			else if (irq_bit == 281)
				hfi_irq_oc_handler(irq, me);
			else
				hfi_irq_errd_handler(irq, me);
		}
	}

	return IRQ_HANDLED;
}

/*
 * configure IRQs for BECN/MNH
 */
static int hfi_setup_becn_oc_irq(struct hfi_devdata *dd, int irq)
{
	struct hfi_irq_entry *me = &dd->irq_entries[irq];
	int ret = 0;

	if (me->arg) {
		dd_dev_err(dd, "MSIX entry is already configured: %d\n",
			   irq);
		return -EINVAL;
	}

	INIT_LIST_HEAD(&me->irq_wait_head);
	spin_lock_init(&me->irq_wait_lock);
	me->dd = dd;
	me->intr_src = irq;

	/* if intx interrupt, we return here */
	if (!dd->num_irq_entries) {
		me->arg = me;	/* mark as in use */
		return 0;
	}

	dd_dev_dbg(dd, "request for msix IRQ %d:%d\n",
		   irq, pci_irq_vector(dd->pdev, irq));

	switch (irq) {
	case HFI_LM_BECN_EVENT:
		ret = request_irq(pci_irq_vector(dd->pdev, irq),
				  hfi_irq_becn_handler, 0,
				  "hfi_irq_becn", me);
		break;
	case HFI_8051_DOMAIN:
		ret = request_irq(pci_irq_vector(dd->pdev, irq),
				  hfi_irq_oc_handler, 0,
				  "hfi_irq_mnh", me);
		break;
	default:
		dd_dev_err(dd, "invalid irq number %d\n", irq);
		ret = -EINVAL;
		break;
	}

	if (ret)
		dd_dev_err(dd, "msix IRQ[%d] request failed %d\n", irq, ret);
	if (!ret)
		me->arg = me;	/* mark as in use */
	return ret;
}

static int hfi_setup_irqs(struct hfi_devdata *dd)
{
	struct pci_dev *pdev = dd->pdev;
	int i, ret;

	/* configure IRQs for Address Translation */
	ret = hfi_at_setup_irq(dd);
	if (ret)
		return ret;

	/* configure IRQ for MNH if enabled */
	if (!no_mnh) {
		ret = hfi_setup_becn_oc_irq(dd, HFI_8051_DOMAIN);
		if (ret)
			return ret;
	}

	/* configure IRQs for error domains */
	ret = hfi_setup_errd_irq(dd);
	if (ret)
		return ret;

	ret = hfi_setup_becn_oc_irq(dd, HFI_LM_BECN_EVENT);
	if (ret)
		return ret;

	/* configure IRQs for EQ groups */
	for (i = 0; i < HFI_NUM_EQ_INTERRUPTS; i++) {
		struct hfi_irq_entry *me = &dd->irq_entries[i];

		WARN_ON(me->arg);
		INIT_LIST_HEAD(&me->irq_wait_head);
		spin_lock_init(&me->irq_wait_lock);
		me->dd = dd;
		me->intr_src = i;

		/* if intx interrupt, we continue here */
		if (!dd->num_irq_entries) {
			/* mark as in use */
			me->arg = me;
			continue;
		}

		dev_dbg(&pdev->dev, "request for msix IRQ %d:%d\n",
			i, pci_irq_vector(dd->pdev, i));
		ret = request_irq(pci_irq_vector(dd->pdev, i),
				  hfi_irq_eq_handler, 0,
				  "hfi_irq_eq", me);
		if (ret) {
			dev_err(&pdev->dev, "msix IRQ[%d] request failed %d\n",
				i, ret);
			/* IRQ cleanup done in hfi_pci_dd_free() */
			return ret;
		}
		/* mark as in use */
		me->arg = me;
	}

	/* Even with intx, we use all the entries */
	dd->num_eq_irqs = HFI_NUM_EQ_INTERRUPTS;
	atomic_set(&dd->irq_eq_next, 0);
	/* TODO - remove or change to debug later */
	dev_info(&pdev->dev, "%d IRQs assigned to EQs\n", i);

	/* request INTx irq */
	if (!dd->num_irq_entries) {
		dev_dbg(&pdev->dev, "request for intx IRQ %d\n", pdev->irq);
		ret = request_irq(dd->pdev->irq, hfi_irq_intx_handler, 0,
				  "hfi_irq_intx", dd);
		if (ret) {
			/* IRQ cleanup done in hfi_pci_dd_free() */
			dev_err(&pdev->dev, "intx IRQ[%d] request failed %d\n",
				pdev->irq, ret);
			return ret;
		}
	}

	return 0;
}

/*
 * hfi_pport_uninit - uninitialize per port
 * data structs
 */
static void hfi_pport_uninit(struct hfi_devdata *dd)
{
	int i;
	u8 port;

	for (port = 1; port <= dd->num_pports; port++) {
		struct cc_state *cc_state;
		struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);

		hfi_clean_i2c_bus(ppd->i2c_bus);
		ppd->i2c_bus = NULL;

#ifdef CONFIG_HFI2_STLNP
		hfi_timesync_stop(ppd);
#endif

		if (ppd->hfi_wq) {
			destroy_workqueue(ppd->hfi_wq);
			ppd->hfi_wq = NULL;
		}

		for (i = 0; i < OPA_MAX_SLS; i++)
			hrtimer_cancel(&ppd->cca_timer[i].hrtimer);

		spin_lock(&ppd->cc_state_lock);
		cc_state = hfi_get_cc_state_protected(ppd);
		RCU_INIT_POINTER(ppd->cc_state, NULL);
		spin_unlock(&ppd->cc_state_lock);

		if (cc_state)
			kfree_rcu(cc_state, rcu);
	}
}

/*
 * hfi_pport_down - shutdown ports
 */
static void hfi_pport_down(struct hfi_devdata *dd)
{
	hfi2_pport_link_uninit(dd);
	hfi2_dispose_firmware(dd);
}

static int hfi_cmdq_wc_map(struct hfi_devdata *dd)
{
	/* TX and RX command queues - fast path access */
	dd->cmdq_tx_base = dd->physaddr + FXR_TX_CQ_ENTRY;
	dd->cmdq_rx_base = dd->physaddr + FXR_RX_CQ_ENTRY;

	dd->cmdq_tx_wc = ioremap_wc(dd->cmdq_tx_base, FXR_TX_CI_FAST_CSRS_SIZE);
	dd->cmdq_rx_wc = ioremap_wc(dd->cmdq_rx_base, FXR_RX_CI_FAST_CSRS_SIZE);
	if (!dd->cmdq_tx_wc || !dd->cmdq_rx_wc)
		return -ENOMEM;
	return 0;
}

static void hfi_cmdq_wc_unmap(struct hfi_devdata *dd)
{
	if (dd->cmdq_tx_wc)
		iounmap(dd->cmdq_tx_wc);
	if (dd->cmdq_rx_wc)
		iounmap(dd->cmdq_rx_wc);
	dd->cmdq_tx_wc = NULL;
	dd->cmdq_rx_wc = NULL;
}

/*
 * Do HFI chip-specific and PCIe cleanup. Free dd memory.
 * This is called in error cleanup from hfi_pci_dd_init().
 * Some state may be unset, so must use caution and test if
 * cleanup is needed.
 */
void hfi_pci_dd_free(struct hfi_devdata *dd)
{
	unsigned long flags;
	int port;
	int ret = 0;
	/*
	 * shutdown ports to notify OPA core clients.
	 * FXRTODO: Check error handling if hfi_pci_dd_init fails early
	 */
	hfi_free_cntrs(dd);
	hfi_pport_down(dd);

	hfi2_ib_remove(dd);
	/*
	 * hfi_vnic_uninit() must be after hfi2_ib_remove() because it uses
	 * dd->vnic in hfi2_vnic_deinit() which is called from
	 * hfi2_ib_remove().
	 */
	hfi_vnic_uninit(dd);
	if (dd->hfi2_link_mgr) {
		kthread_stop(dd->hfi2_link_mgr);
		dd->hfi2_link_mgr = NULL;
	}
	/* unregister from opa_core and IB first, before clear any portdata */
	hfi_pport_uninit(dd);

	/* remove logical unit nop */
	spin_lock_irqsave(&hfi2_unit_lock, flags);
	idr_remove(&hfi2_unit_table, dd->unit);
	spin_unlock_irqrestore(&hfi2_unit_lock, flags);

#ifdef CONFIG_HFI2_STLNP
	hfi_e2e_destroy_all(dd);
#endif
	if (!no_interrupts)
		hfi_disable_interrupts(dd);
	hfi_free_spill_area(dd);

	/* release system context and any privileged CMDQs */
	if (dd->priv_ctx.devdata) {
#ifdef CONFIG_HFI2_STLNP
		hfi_e2e_stop(&dd->priv_ctx);
#endif
		hfi_eq_zero_release(&dd->priv_ctx);
		hfi_cmdq_unmap(&dd->priv_cmdq);
		hfi_cmdq_cleanup(&dd->priv_ctx);
		ret = hfi_ctx_cleanup(&dd->priv_ctx);
		if (ret)
			dd_dev_warn(dd, "hfi_ctx_cleanup returned %d\n",
				    ret);
	}
	hfi_pend_cmdq_info_free(&dd->pend_cmdq);
	hfi_cleanup_interrupts(dd);
	hfi_at_exit(dd);

	for (port = 1; port <= dd->num_pports; port++) {
		struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);

		hfi_shutdown_led_override(ppd);

		if (ppd->led_override_timer.function) {
			del_timer_sync(&ppd->led_override_timer);
			atomic_set(&ppd->led_override_timer_active, 0);
		}
	}

	/* free host memory for FXR and Portals resources */
	if (dd->cmdq_head_base)
		free_pages((unsigned long)dd->cmdq_head_base,
			   get_order(dd->cmdq_head_size));

	/* late exit init dd->hfi_dev_dbg */
	hfi_dbg_dev_late_exit(dd);

	hfi_cmdq_wc_unmap(dd);
	if (dd->kregbase)
		iounmap((void __iomem *)dd->kregbase);

	hfi2_free_platform_config(dd);

	idr_destroy(&dd->cmdq_pair);
	idr_destroy(&dd->ptl_user);
	idr_destroy(&dd->ptl_tpid);
	idr_destroy(&dd->pid_wait);

	rcu_barrier(); /* wait for rcu callbacks to complete */

	free_percpu(dd->int_counter);
	pci_set_drvdata(dd->pdev, NULL);

	kfree(dd);
}

static void core_ctx_init(struct ib_device *ibdev, struct hfi_ctx *ctx)
{
	hfi_ctx_init(ctx, hfi_dd_from_ibdev(ibdev));
}

static struct opa_core_ops opa_core_ops = {
	.ctx_init = core_ctx_init,
	.ctx_assign = hfi_ctx_attach,
	.ctx_release = hfi_ctx_release,
	.ctx_reserve = hfi_ctx_reserve,
	.ctx_unreserve = hfi_ctx_unreserve,
	.ctx_addr = hfi_ctx_hw_addr,
	.cmdq_assign = hfi_cmdq_assign,
	.cmdq_update = hfi_cmdq_update,
	.cmdq_release = hfi_cmdq_release,
	.cmdq_map = hfi_cmdq_map,
	.cmdq_unmap = hfi_cmdq_unmap,
	.ev_assign = hfi_cteq_assign,
	.ev_release = hfi_cteq_release,
	.ev_wait_single = hfi_ev_wait_single,
	.ev_set_channel = hfi_ev_set_channel,
	.ec_wait_event = hfi_ec_wait_event,
	.eq_ack_event = hfi_eq_ack_event,
	.ct_ack_event = hfi_ct_ack_event,
	.ec_assign = hfi_ec_assign,
	.ec_release = hfi_ec_release,
	.pt_update_lower = hfi_pt_update_lower,
	.get_hw_limits = hfi_get_hw_limits,
	.get_async_error = hfi_get_async_error,
	.at_prefetch = hfi_at_prefetch,
#ifdef CONFIG_HFI2_STLNP
	.check_sl_pair = hfi_check_sl_pair,
	.ctx_set_allowed_uids = hfi_ctx_set_allowed_uids,
	.dlid_assign = hfi_dlid_assign,
	.dlid_release = hfi_dlid_release,
	.e2e_ctrl = hfi_e2e_ctrl,
	.get_ts_master_regs = hfi_get_ts_master_regs,
	.get_ts_fm_data = hfi_get_ts_fm_data,
#endif
};

static void hfi_init_sc_to_vl_tables(struct hfi_pportdata *ppd)
{
	u8 vlt[OPA_MAX_SCS];

	memset(vlt, 0, sizeof(vlt));
	/*
	 * Only SC 15 -> VL 15 (SMP) will be used until a valid table
	 * is setup by FM
	 */
	vlt[15] = 15;
	hfi_set_sc_to_vlt(ppd, vlt);
	hfi_set_sc_to_vlnt(ppd, vlt);
	hfi_set_sc_to_vlr(ppd, vlt);
}

static void hfi_init_linkmux_csrs(struct hfi_pportdata *ppd)
{
	u64 val;
	/*TP_PRF_CTRL_t tp_perf;*/
	u64 fpc1;

	val = read_csr(ppd->dd, FXR_TP_CFG_MISC_CTRL);
	val &= ~(FXR_TP_CFG_MISC_CTRL_DISABLE_RELIABLE_VL15_PKTS_SMASK |
		 FXR_TP_CFG_MISC_CTRL_DISABLE_RELIABLE_VL8_0_PKTS_SMASK |
		 FXR_TP_CFG_MISC_CTRL_OPERATIONAL_VL_SMASK);
	val |= (0x3FF & FXR_TP_CFG_MISC_CTRL_OPERATIONAL_VL_MASK) <<
		FXR_TP_CFG_MISC_CTRL_OPERATIONAL_VL_SHIFT;
	write_csr(ppd->dd, FXR_TP_CFG_MISC_CTRL, val);

	#if 0
	tp_perf.val = read_csr(ppd->dd, FXR_TP_PRF_CTRL);
	tp_perf.field.enable_cntrs = 0x1;
	write_csr(ppd->dd, FXR_TP_PRF_CTRL, tp_perf.val);
	#endif

	fpc1 = read_csr(ppd->dd, FXR_FPC_CFG_PORT_CONFIG1);

	/* Enable BECN interrupts on all SCs */
	fpc1 |= FXR_FPC_CFG_PORT_CONFIG1_B_SC_ENABLE_SMASK;
	write_csr(ppd->dd, FXR_FPC_CFG_PORT_CONFIG1, fpc1);
}

static void hfi_init_bw_arb_caches(struct hfi_pportdata *ppd)
{
	/*
	 * Note that we always return values directly from the
	 * 'bw_arb_cache' (and do no CSR reads) in response to a
	 * 'Get(BWArbTable)'. This is obviously correct after a
	 * 'Set(BWArbTable)', since the cache will then be up to
	 * date. But it's also correct prior to any 'Set(BWArbTable)'
	 * since then both the cache, and the relevant h/w registers
	 * will be zeroed.
	 */

	spin_lock_init(&ppd->bw_arb_cache.lock);
}

/*
 * calculate and set the new leak amount integer and fractional parts
 * for the mctc taking into the account the shift and mult factor
 */
static void hfi_set_leak_amt(struct hfi_pportdata *ppd, u8 sl, u16 shift,
			     u16 mult,
		      bool cap)
{
	u32 leak_amt;
	u16 lws, lwa;
	u8 perc, leak_int, leak_frac;
	struct opa_bw_element *bw;
	u8 tc, mctc;

	lws = (u16)find_last_bit((void *)&ppd->link_width_supported, 16) + 1;
	lwa = (u16)find_first_bit((void *)&ppd->link_width_active, 16) + 1;
	mctc = ppd->sl_to_mctc[sl];
	tc = HFI_GET_TC(mctc);
	bw = (struct opa_bw_element *)&ppd->bw_arb_cache.bw_group;
	perc = bw[tc].bw_percentage;
	/*
	 * the math to calculate the leak_amt, leak_int and leak_frac
	 * is same as explained in hfi_set_bw_group. The only change
	 * is the additional BW percentage factored in to change the
	 * leak rate to introduce the IPG. the formula for additional
	 * BW perc is derived as Tp/(Tp+IPG) where Tp is the packet time
	 * the IBTA spec specifies the IPG to be ( Tp >> shift) * mult
	 * where shift and mult are obtained from CCT.
	 * Substituing for IPG in the above formula, we calculate the
	 * additional BW perc to be (2^shift)/(mult + 2^shift)
	 */
	leak_amt = (256 * lwa * HFI_TX_FLITS_PER_CLK * perc) << shift;
	leak_amt /= (lws * (mult + (1 << shift)) * 100);
	leak_int = (u8)(leak_amt >> 8);
	leak_frac = (u8)leak_amt;
	if (hfi_is_sl_pair(ppd, sl, NULL))
		hfi_set_txdma_bw(ppd->dd, leak_int, leak_frac, tc, cap);
	else
		hfi_set_txdma_mctc_bw(ppd->dd, leak_int, leak_frac, mctc, cap);
	ppd_dev_dbg(ppd, "tc : %u\n", tc);
	ppd_dev_dbg(ppd, "leak_int:%04x\n", leak_int);
	ppd_dev_dbg(ppd, "frac : %04x\n", leak_frac);
}

/*
 * this function is called when the timer expires. The CCTI is decremented and
 * new leak rate is calculated
 */
static enum hrtimer_restart cca_timer_fn(struct hrtimer *t)
{
	struct cca_timer *cca_timer;
	struct hfi_pportdata *ppd;
	u8 sl;
	u16 ccti_timer, ccti_min, cce, shift, mult;
	struct cc_state *cc_state;
	unsigned long flags, nsec;
	enum hrtimer_restart ret = HRTIMER_NORESTART;

	cca_timer = container_of(t, struct cca_timer, hrtimer);
	ppd = cca_timer->ppd;
	sl = cca_timer->sl;

	rcu_read_lock();
	cc_state = hfi_get_cc_state(ppd);

	if (!cc_state) {
		rcu_read_unlock();
		return HRTIMER_NORESTART;
	}

	ccti_min = cc_state->cong_setting.entries[sl].ccti_min;
	ccti_timer = cc_state->cong_setting.entries[sl].ccti_timer;

	spin_lock_irqsave(&ppd->cca_timer_lock, flags);

	/*
	 * decrement CCTI if CCTI is greater than ccti_min
	 */
	if (cca_timer->ccti > ccti_min) {
		bool cap_bw;

		cca_timer->ccti--;
		cce = cc_state->cct.entries[cca_timer->ccti].entry;
		shift = (cce & 0xc000) >> 14;
		mult = (cce & 0x3fff);
		ppd_dev_dbg(ppd, "Time Out. Decrement CCTI\n");
		ppd_dev_dbg(ppd, "sl:%d ccti:%d mult:%d shift:%d\n",
			    sl, cca_timer->ccti, mult, shift);

		/* Only cap bandwidth when we're above the
		 * ccti minimum.
		 */
		cap_bw = (cca_timer->ccti > ccti_min);
		hfi_set_leak_amt(ppd, sl, shift, mult, cap_bw);
	}

	if (cca_timer->ccti > ccti_min) {
		nsec = 1024 * ccti_timer;
		hrtimer_forward_now(t, ns_to_ktime(nsec));
		ret = HRTIMER_RESTART;
	}

	spin_unlock_irqrestore(&ppd->cca_timer_lock, flags);
	rcu_read_unlock();
	return ret;
}

static void hfi_log_cca_event(struct hfi_pportdata *ppd, u8 sl)
{
	struct opa_hfi_cong_log_event_internal *cc_event;
	unsigned long flags;

	if (sl >= OPA_MAX_SLS)
		return;

	spin_lock_irqsave(&ppd->cc_log_lock, flags);

	ppd->threshold_cong_event_map[sl / 8] |= 1 << (sl % 8);
	ppd->threshold_event_counter++;

	cc_event = &ppd->cc_events[ppd->cc_log_idx++];
	if (ppd->cc_log_idx == OPA_CONG_LOG_ELEMS)
		ppd->cc_log_idx = 0;
	/*
	 * FXRTODO: decide whther to log CCA events only for Verbs
	 * we dont have qp and svc_type info for non-verbs transport
	 * lqpn, rqpn, rlid set to 0 as we dont have the info in the
	 * interrupt handler. svc_type is set to an invald type
	 */
	cc_event->lqpn = 0;
	cc_event->rqpn = 0;
	cc_event->sl = sl;
	cc_event->svc_type = 5;
	cc_event->rlid = 0;
	/* keep timestamp in units of 1.024 usec */
	cc_event->timestamp = ktime_get_ns() / 1024;

	spin_unlock_irqrestore(&ppd->cc_log_lock, flags);
}

static u64 hfi_becn_rcvd(struct hfi_pportdata *ppd, int sc)
{
	u64 new_becn_count, old_becn_count;
	u64 becn_incr;

	new_becn_count = hfi_read_lm_fpc_prf_per_vl_csr(
				ppd,
				FXR_FPC_PRF_PORTRCV_SC_BECN,
				sc);
	old_becn_count = ppd->per_sc_becn_count[sc];
	becn_incr = new_becn_count - old_becn_count;

	ppd_dev_dbg(ppd, "No. of BECNs received per sc: %llu\n",
		    becn_incr);
	ppd->per_sc_becn_count[sc] = new_becn_count;
	return becn_incr;
}

static void hfi_decrease_bw(struct hfi_pportdata *ppd, u8 sl, u64 becn_incr)
{
	u16 cce, ccti_incr, ccti_timer, ccti_limit, shift, mult;
	u64 new_ccti;
	struct cc_state *cc_state;
	struct cca_timer *cca_timer;
	unsigned long nsec;

	rcu_read_lock();
	cc_state = hfi_get_cc_state(ppd);
	if (!cc_state) {
		rcu_read_unlock();
		return;
	}
	/*
	 * obtain the ccti_incr, ccti_limit and
	 * ccti_timer from the congestion sesstings
	 * sent by the FM
	 */
	ccti_limit = cc_state->cct.ccti_limit;
	ccti_incr = cc_state->cong_setting.entries[sl].ccti_increase;
	ccti_timer = cc_state->cong_setting.entries[sl].ccti_timer;
	spin_lock(&ppd->cca_timer_lock);
	cca_timer = &ppd->cca_timer[sl];

	/*
	 * increment the ccti
	 */
	new_ccti = cca_timer->ccti + ccti_incr * becn_incr;
	cca_timer->ccti = min_t(u16, ccti_limit, new_ccti);

	/*
	 * obtain the shift and mult factor from the cce
	 */
	cce = cc_state->cct.entries[cca_timer->ccti].entry;
	shift = (cce & 0xc000) >> 14;
	mult = (cce & 0x3fff);
	ppd_dev_dbg(ppd, "shift:%d mult:%d\n", shift, mult);
	hfi_set_leak_amt(ppd, sl, shift, mult, HFI_ENABLE_CAPPING);

	/*
	 * start the hrtimer
	 */
	if (!hrtimer_active(&cca_timer->hrtimer) && mult && ccti_timer) {
		nsec = 1024 * ccti_timer;
		hrtimer_start(&cca_timer->hrtimer, ns_to_ktime(nsec),
			      HRTIMER_MODE_REL);
	}
	spin_unlock(&ppd->cca_timer_lock);
	rcu_read_unlock();
}

/*
 * hfi_irq_becn_handler is the interrupt handler for becns
 * on Hfi2 transport. This function changes the leak rate
 * and bw limit of the mctc corresponding to the
 * SC on which the BECN arrived. It also increments
 * the per SL CCTI and starts a timer
 */
irqreturn_t hfi_irq_becn_handler(int irq, void *dev_id)
{
	struct hfi_pportdata *ppd;
	u64 becn_incr = 0;
	u8 port, sl, sc, vl;
	u64 becn_mask;

	struct hfi_irq_entry *me = dev_id;
	struct hfi_devdata *dd = me->dd;

	this_cpu_inc(*dd->int_counter);

	for (port = 1; port <= dd->num_pports; port++) {
		ppd = to_hfi_ppd(dd, port);
		becn_mask = read_csr(ppd->dd, FXR_FPC_STS_BECN_SC_RCVD);
		/*
		 * reset the sc mask
		 */
		write_csr(ppd->dd, FXR_FPC_STS_BECN_SC_RCVD, becn_mask);
		ppd_dev_dbg(ppd, "becn_mask is 0x%llx\n", becn_mask);
		for_each_set_bit(sc, (unsigned long *)&becn_mask,
				 8 * sizeof(becn_mask)) {
			vl = ppd->sc_to_vlr[sc];
			vl = idx_from_vl(vl);
			sl = ppd->sc_to_sl[sc];
			ppd_dev_dbg(ppd, "sc:%u sl:%u vl:%u\n", sc, sl, vl);
			becn_incr = hfi_becn_rcvd(ppd, sc);
			if (becn_incr && vl != C_VL_15) {
				hfi_decrease_bw(ppd, sl, becn_incr);
				hfi_log_cca_event(ppd, sl);
			}
		}
	}
	return IRQ_HANDLED;
}

static void hfi_zebu_hack_vl_credits(struct hfi_pportdata *ppd)
{
	if (!ppd->dd->emulation)
		return;

	/*
	 * These are taken from running with Simics+FM and
	 * logging what is actually set. FXRTODO: Remove this.
	 */
	hfi_set_up_vl15(ppd, 3, 272);
	hfi_set_vau(ppd, 2);
	hfi_set_global_limit(ppd, 2048);
	hfi_set_vl_dedicated(ppd, 15, 104);
	hfi_set_vl_dedicated(ppd, 0, 198);
	hfi_set_vl_dedicated(ppd, 1, 198);
	hfi_set_vl_dedicated(ppd, 2, 198);
	hfi_set_vl_dedicated(ppd, 3, 198);
	hfi_set_vl_dedicated(ppd, 4, 198);
	hfi_set_vl_dedicated(ppd, 5, 198);
	hfi_set_vl_dedicated(ppd, 6, 198);
	hfi_set_vl_shared(ppd, 0, 756);
	hfi_set_vl_shared(ppd, 1, 756);
	hfi_set_vl_shared(ppd, 2, 756);
	hfi_set_vl_shared(ppd, 3, 756);
	hfi_set_vl_shared(ppd, 4, 756);
	hfi_set_vl_shared(ppd, 5, 756);
	hfi_set_vl_shared(ppd, 6, 756);
	hfi_set_vl_shared(ppd, 15, 756);
	hfi_set_global_shared(ppd, 756);
}

static void hfi_zebu_hack_sl_tables(struct hfi_pportdata *ppd)
{
	ppd->sl_pairs[0] = 1;
	ppd->sl_pairs[2] = 3;
	hfi_set_sl_pairs(ppd, ppd->sl_pairs);
	ppd->sl_to_sc[1] = 1;
	ppd->sl_to_mctc[1] = 4;
	ppd->sc_to_mctc[1] = 4;
	ppd->sc_to_resp_sl[1] = 1;
	ppd->sc_to_sl[1] = 1;
	hfi_set_mctc_regs(ppd);
}

/*
 * hfi_pport_init - initialize per port
 * data structs
 */
static int hfi_pport_init(struct hfi_devdata *dd)
{
	struct hfi_pportdata *ppd;
	struct cc_state *cc_state;
	int i;
	u8 port;
	int ret = 0;

	for (port = 1; port <= dd->num_pports; port++) {
		ppd = to_hfi_ppd(dd, port);
		ppd->dd = dd;
		ppd->pnum = port;

		if (no_mnh) {
			if (opafm_disable) {
				ppd->host_link_state = HLS_UP_ACTIVE;
			} else {
				ppd->host_link_state = HLS_UP_INIT;
				ppd->link_width_active = 0x8;
				ppd->neighbor_normal = 1;
				ppd->pkeys[2] = 0xffff;
			}

			/* Required since Zebu skips full LNI */
			hfi2_oc_init(ppd);
			hfi_zebu_hack_vl_credits(ppd);
		} else {
			ppd->host_link_state = HLS_DN_OFFLINE;
		}
		mutex_init(&ppd->hls_lock);
		/* initialize Manor Hill - mnh/8051 */
		spin_lock_init(&ppd->crk8051_lock);
		spin_lock_init(&ppd->qsfp_info.qsfp_lock);
		mutex_init(&ppd->crk8051_mutex);
		ppd->crk8051_timed_out = 0;
		ppd->linkinit_reason = OPA_LINKINIT_REASON_LINKUP;
		ppd->sm_trap_qp = OPA_DEFAULT_SM_TRAP_QP;
		ppd->sa_qp = OPA_DEFAULT_SA_QP;
		ppd->last_phystate = PLS_PREV_STATE_UNKNOWN;

		/* FXRTODO: Remove after QSFP code implemented */
		ppd->port_type = PORT_TYPE_QSFP;

		ppd->cc_max_table_entries = HFI_IB_CC_TABLE_CAP_DEFAULT;

		/* Initialize credit management variables */
		/* assign link credit variables */
		if (dd->emulation) {
			ppd->vau = HFI_CM_VAU;
		} else {
			/*
			 * FXRTODO: Remove when Simics model updates to have
			 * vAU = 2 JIRA: STL-33783
			 */
			ppd->vau = 3;
		}
		ppd->link_credits = HFI_RCV_BUFFER_SIZE /
				   hfi_vau_to_au(ppd->vau);
		ppd->vcu = hfi_cu_to_vcu(HFI_CM_CU);
		/* enough room for 8 MAD packets plus header - 17K */
		ppd->vl15_init = (8 * (HFI_MIN_VL_15_MTU + 128)) /
					hfi_vau_to_au(ppd->vau);
		if (ppd->vl15_init > ppd->link_credits)
			ppd->vl15_init = ppd->link_credits;

		hfi_assign_local_cm_au_table(ppd, ppd->vcu);

		/* Initialize the I2C bus */
		ppd->i2c_bus = hfi_init_i2c_bus(ppd, 0);
		if (IS_ERR(ppd->i2c_bus)) {
			ret = PTR_ERR(ppd->i2c_bus);
			ppd_dev_err(ppd, "I2C bus initialization failed\n");
			goto err;
		}

		/*
		 * Since OPA uses management pkey there is no
		 * need to initialize entry 0 with default application
		 * pkey 0x8001 as mentioned in ib spec v1.3
		 */
		ppd->pkeys[OPA_LIM_MGMT_PKEY_IDX] = OPA_LIM_MGMT_PKEY;

		if (opafm_disable) {
			ppd->pkeys[0] = 0x8015;
			ppd->pkeys[2] = 0xffff;
			ppd->pkeys[3] = 0x8016;
			ppd->pkeys[4] = 0x8002;
			ppd->pkeys[5] = 0x8006;
		}

		ppd->packet_format_supported =
			OPA_PORT_PACKET_FORMAT_9B | OPA_PORT_PACKET_FORMAT_8B |
			OPA_PORT_PACKET_FORMAT_16B;
		hfi_set_ib_cfg(ppd, HFI_IB_CFG_PKT_FORMAT, 0,
			       &ppd->packet_format_supported);
		/*
		 * FXRTODO: Disabling for now since we are not yet
		 * PKEY ready
		 */
		hfi_set_ib_cfg(ppd, HFI_IB_CFG_PKEYS, 0, NULL);

		ppd->vls_supported = HFI_NUM_DATA_VLS;

		ppd->link_width_supported =
			OPA_LINK_WIDTH_1X | OPA_LINK_WIDTH_2X |
			OPA_LINK_WIDTH_3X | OPA_LINK_WIDTH_4X;
		ppd->link_width_downgrade_supported =
			ppd->link_width_supported;
		/* start out enabling only 4X */
		ppd->link_width_enabled = OPA_LINK_WIDTH_4X;

		ppd->link_width_downgrade_enabled =
					ppd->link_width_downgrade_supported;

		ppd->link_speed_supported = OPA_LINK_SPEED_50G |
					    OPA_LINK_SPEED_25G;
		ppd->link_speed_enabled = ppd->link_speed_supported;
		/* give a reasonable active value, will be set on link up */
		ppd->link_speed_active = OPA_LINK_SPEED_25G;

		ppd->port_crc_mode_enabled = HFI_SUPPORTED_CRCS;
		/* initialize supported LTP CRC mode */
		ppd->port_ltp_crc_mode = hfi_cap_to_port_ltp(
					HFI_SUPPORTED_CRCS) <<
					HFI_LTP_CRC_SUPPORTED_SHIFT;
		/* initialize enabled LTP CRC mode */
		ppd->port_ltp_crc_mode |= hfi_cap_to_port_ltp(
					HFI_SUPPORTED_CRCS) <<
					HFI_LTP_CRC_ENABLED_SHIFT;
		if (dd->emulation) {
			hfi_zebu_hack_default_mtu(ppd);
			ppd->mgmt_allowed = 1;
		}

		/*
		 * Set the default MTU only for VL 15
		 * For rest of the data VLs, MTU of 0
		 * is valid as per the spec
		 */
		ppd->vl_mtu[15] = HFI_MIN_VL_15_MTU;
		ppd->max_active_mtu = HFI_DEFAULT_ACTIVE_MTU;
		hfi_set_send_length(ppd);
		ppd->vls_operational = ppd->vls_supported;
		hfi_init_linkmux_csrs(ppd);
		for (i = 0; i < ARRAY_SIZE(ppd->sl_to_sc); i++)
			ppd->sl_to_sc[i] = i;
		for (i = 0; i < ARRAY_SIZE(ppd->sc_to_sl); i++)
			ppd->sc_to_sl[i] = i;

		for (i = 0; i < ARRAY_SIZE(ppd->sl_pairs); i++)
			ppd->sl_pairs[i] = HFI_INVALID_RESP_SL;
		/* allow all SLs by default */
		ppd->sl_mask = -1;
		for (i = 0; i < ARRAY_SIZE(ppd->per_sc_becn_count); i++)
			ppd->per_sc_becn_count[i] =
				hfi_read_lm_fpc_prf_per_vl_csr(
					ppd,
					FXR_FPC_PRF_PORTRCV_SC_BECN, i);
		hfi_set_ib_cfg(ppd, HFI_IB_CFG_SL_TO_MCTC, 0, NULL);
		hfi_set_ib_cfg(ppd, HFI_IB_CFG_SL_TO_SC, 0, NULL);
		hfi_set_ib_cfg(ppd, HFI_IB_CFG_SC_TO_RESP_SL, 0, ppd->sc_to_sl);
		hfi_set_ib_cfg(ppd, HFI_IB_CFG_SC_TO_MCTC, 0, ppd->sc_to_sl);
		hfi_init_bw_arb_caches(ppd);
		/* Required since Zebu does not enable an FM */
		if (dd->emulation)
			hfi_zebu_hack_sl_tables(ppd);

		hfi_init_sc_to_vl_tables(ppd);
#ifdef CONFIG_HFI2_STLNP
		hfi_e2e_init(ppd);
#endif
		spin_lock_init(&ppd->cca_timer_lock);
		for (i = 0; i < OPA_MAX_SLS; i++) {
			hrtimer_init(&ppd->cca_timer[i].hrtimer,
				     CLOCK_MONOTONIC, HRTIMER_MODE_REL);
			ppd->cca_timer[i].ppd = ppd;
			ppd->cca_timer[i].sl = i;
			ppd->cca_timer[i].ccti = 0;
			ppd->cca_timer[i].hrtimer.function = cca_timer_fn;
		}
		spin_lock_init(&ppd->cc_state_lock);
		cc_state = kzalloc(sizeof(*cc_state), GFP_KERNEL);
		if (!cc_state) {
			dd_dev_err(dd, "Congestion Control Agent disabled for port %d\n",
				   port);
		}
		RCU_INIT_POINTER(ppd->cc_state, cc_state);

		/* TODO: adjust 0 for max_active */
		ppd->hfi_wq = alloc_workqueue("hfi2_%d_%d",
			WQ_SYSFS | WQ_HIGHPRI | WQ_CPU_INTENSIVE, 0,
			dd->unit, port);
		if (!ppd->hfi_wq) {
			dd_dev_err(dd, "Can't allocate workqueue\n");
			ret = -ENOMEM;
			goto err;
		}

		/* Turn the LED to off */
		hfi_set_ext_led(ppd, 0);
		/* Required since Zebu does not enable an FM */
		if (dd->emulation)
			hfi_set_small_headers(ppd);
	}

	ret = hfi2_load_firmware(dd);
	if (ret) {
		dd_dev_err(dd, "can't load firmware on 8051s");
		goto err;
	}

	if (lni_debug) {
		dd->hfi2_link_mgr = kthread_run(hfi_dbg_link_mgr, dd,
						"hfi2_link_mgr_%d", dd->unit);
		if (IS_ERR(dd->hfi2_link_mgr))
			dd_dev_err(dd, "Unable to create link_mgr\n");
	}

err:
	/* On error, cleanup is done by caller in hfi_pci_dd_free() */
	return ret;
}

/*
 * hfi_flr - pcie function level reset
 * @dd: device specific data
 * @dev: the pci_dev for this HFI device
 *
 * Perform a PCIe function level reset.
 *
 * Returns 0 for success and -EINVAL for failure
 */
static int hfi_flr(struct hfi_devdata *dd, struct pci_dev *pdev)
{
	struct pci_saved_state *pci_saved_state = NULL;
	int ret;

	/* FXRTODO: Reset has not been validated by DV */
	if (dd->emulation) {
		dd_dev_info(dd, "Skipping reset HFI with PCIe FLR\n");
		return 0;
	}

	dd_dev_info(dd, "Resetting HFI with PCIe FLR\n");

	/*
	 * pci_reset_function can't be used here because it
	 * holds device_lock which is already held since this
	 * routine is called in the driver probe context.
	 *
	 * Must save/restore PCIe registers around reset.
	 */
	ret = pci_save_state(pdev);
	if (ret) {
		dd_dev_err(dd, "Count not save PCI state\n");
		return ret;
	}
	pci_saved_state = pci_store_saved_state(pdev);

	pcie_flr(pdev);

	ret = pci_load_and_free_saved_state(pdev, &pci_saved_state);
	if (ret) {
		dd_dev_err(dd, "Could not reload PCI state\n");
		return ret;
	}

	pci_restore_state(pdev);
	return 0;
}

/*
 * hfi_driver_reset - Issue a driver reset
 * @dd: device specific data
 *
 * Perform a driver reset.
 *
 * Returns 0 for success and -EINVAL for failure
 */
static int hfi_driver_reset(struct hfi_devdata *dd)
{
	int i;
	u64 reg = 0;

	/* FXRTODO: Reset has not been validated by DV */
	if (dd->emulation) {
		dd_dev_info(dd, "Skipping DRIVER_RESET\n");
		return 0;
	}

	dd_dev_info(dd, "Resetting HFI with DRIVER_RESET\n");

	reg = HFI_PCIM_CFG_DRIVER_RESET_SMASK;
	write_csr(dd, HFI_PCIM_CFG, reg);

	for (i = 0; i < HFI_DRIVER_RESET_RETRIES; i++) {
		reg = read_csr(dd, HFI_PCIM_CFG);

		if (!(reg & HFI_PCIM_CFG_DRIVER_RESET_SMASK))
			break;
		msleep(20);
	}

	if (i == HFI_DRIVER_RESET_RETRIES) {
		dd_dev_err(dd, "Driver reset failed to complete\n");
		return -EINVAL;
	}
	return 0;
}

static int reset_device(struct hfi_devdata *dd)
{
	/* cannot reset if there are any user/vnic contexts */
	if (dd->stats.sps_ctxts)
		return -EBUSY;

	if (use_driver_reset)
		return hfi_driver_reset(dd);
	else
		return hfi_flr(dd, dd->pdev);
}

void hfi_read_lm_link_state(const struct hfi_pportdata *ppd)
{
	u64 val;

	if (loopback != LOOPBACK_LCB || !ppd->dd->emulation)
		return;

	val = read_csr(ppd->dd, FXR_TP_STS_STATE) &
		       FXR_TP_STS_STATE_LINK_STATE_SMASK;
	ppd_dev_info(ppd, "%s %d OC_LCB_CFG_PORT 0x%llx sts 0x%llx\n",
		     __func__, __LINE__, read_csr(ppd->dd, OC_LCB_CFG_PORT),
		     val);
}

static void hfi_cmdq_config_all(struct hfi_devdata *dd)
{
	int i;

	__hfi_cmdq_config_all(dd);
	/* now configure CMDQ head addresses */
	for (i = 0; i < HFI_CMDQ_COUNT; i++)
		hfi_cmdq_head_config(dd, i, dd->cmdq_head_base);
}

/*
 * hfi_pci_dd_init - chip-specific initialization
 * @dev: the pci_dev for this HFI device
 * @ent: pci_device_id struct for this dev
 *
 * Allocates, inits, and returns the devdata struct for this
 * device instance.
 *
 * Do remaining PCIe setup, once dd is allocated, and save away
 * fields required to re-initialize after a chip reset, or for
 * various other purposes.
 */
struct hfi_devdata *hfi_pci_dd_init(struct pci_dev *pdev,
				    const struct pci_device_id *ent)
{
	struct hfi_devdata *dd;
	unsigned long len, flags;
	resource_size_t addr;
	int ret;
	struct hfi_ctx *ctx;
	struct page *page;
	struct opa_ctx_assign ctx_assign = {0};
	static const char * const inames[] = { /* implementation names */
		"RTL silicon",
		"RTL FPGA emulation",
		"RTL Zebu emulation",
		"Functional simulator"
	};

	dd = hfi_alloc_devdata(pdev);
	if (IS_ERR(dd))
		return dd;

	/*
	 * Do remaining PCIe setup and save PCIe values in dd.
	 * On return, we have the chip mapped.
	 */

	dd->pdev = pdev;
	pci_set_drvdata(pdev, dd);

	/* For Portals PID and CMDQ assignments */
	idr_init(&dd->ptl_user);
	spin_lock_init(&dd->ptl_lock);
	idr_init(&dd->ptl_tpid);
	idr_init(&dd->cmdq_pair);
	spin_lock_init(&dd->cmdq_lock);
	idr_init(&dd->pid_wait);
	mutex_init(&dd->e2e_lock);
	dd->core_ops = &opa_core_ops;

	dd->int_counter = alloc_percpu(u64);
	if (!dd->int_counter) {
		ret = -ENOMEM;
		goto err_post_alloc;
	}

	/* List of context error queue to dispatch error to */
	INIT_LIST_HEAD(&dd->error_dispatch_head);
	spin_lock_init(&dd->error_dispatch_lock);

	/* give unit a logical number 0-n */
	idr_preload(GFP_KERNEL);
	spin_lock_irqsave(&hfi2_unit_lock, flags);
	ret = idr_alloc(&hfi2_unit_table, dd, 0, 0, GFP_NOWAIT);
	spin_unlock_irqrestore(&hfi2_unit_lock, flags);
	idr_preload_end();
	if (ret < 0)
		goto err_post_alloc;
	dd->unit = ret;

	/* FXR resources are on BAR0 (used for io_remap, etc.) */
	addr = pci_resource_start(pdev, HFI_FXR_BAR);
	len = pci_resource_len(pdev, HFI_FXR_BAR);
	/* wc_off excludes TX/RX cmdq'd and reserved regions */
	dd->wc_off = HFI_UC_RANGE_START;
	dd->kregbase = ioremap_nocache(addr + dd->wc_off, len - dd->wc_off);
	if (!dd->kregbase) {
		ret = -ENOMEM;
		goto err_post_alloc;
	}
	dd->kregend = dd->kregbase + len - dd->wc_off;
	dd->physaddr = addr;
	dev_dbg(&pdev->dev, "BAR[%d] @ start 0x%lx len %lu\n",
		HFI_FXR_BAR, (long)addr, len);
	ret = hfi_cmdq_wc_map(dd);
	if (ret)
		goto err_post_alloc;

	dd->icode = (dd->pdev->revision & HFI2_PCI_REVISION_ICODE_SMASK) >>
		    HFI2_PCI_REVISION_ICODE_SHIFT;
	dd->minrev = (dd->pdev->revision & HFI2_PCI_REVISION_REV_SMASK) >>
		     HFI2_PCI_REVISION_REV_SHIFT;
	dd->emulation = dd->icode == ICODE_FPGA_EMULATION ||
			dd->icode == ICODE_ZEBU_EMULATION;

	dd_dev_info(dd, "Implementation: %s, revision 0x%x\n",
		dd->icode < ARRAY_SIZE(inames) ? inames[dd->icode] : "unknown",
		(int)dd->pdev->revision);

	/*
	 * Workaround for https://hsdes.intel.com/appstore/article/#/2202720924.
	 * MSI-X interrupts are not currently supported on FPGA emulation
	 *
	 * Workaround for https://sid-jira.pdx.intel.com/browse/STL-36858.
	 * Do not load PE firmware until backdoor mechanism is in place or
	 * FPGAs can handle firmware download from /lib/firmware.
	 */
	if (dd->icode == ICODE_FPGA_EMULATION)
		no_interrupts = true;

	if (dd->emulation) {
		opafm_disable = true;
	}

	if (no_interrupts)
		no_verbs = true;

	ret = reset_device(dd);
	if (ret)
		goto err_post_alloc;

	/* Ensure CSRs are sane, we can't trust they haven't been manipulated */
	init_csrs(dd);
	dd->bypass_pid = HFI_PID_NONE;
	/* vl15 and mcast PIDs set by hfi2_ib_add */
	dd->vl15_pid = HFI_PID_NONE;
	dd->mcast_pid = HFI_PID_NONE;

	/* early init dd->hfi_dev_dbg */
	hfi_dbg_dev_early_init(dd);

	hfi_pend_cmdq_info_alloc(dd, &dd->pend_cmdq, "sys");

	/* per port init */
	dd->num_pports = HFI_NUM_PPORTS;
	ret = hfi_pport_init(dd);
	if (ret)
		goto err_post_alloc;

	hfi_read_guid(dd);

	dd->oui[0] = be64_to_cpu(dd->nguid) >> 56 & 0xFF;
	dd->oui[1] = be64_to_cpu(dd->nguid) >> 48 & 0xFF;
	dd->oui[2] = be64_to_cpu(dd->nguid) >> 40 & 0xFF;

	/* CMDQ head (read) indices - 16 KB */
	dd->cmdq_head_size = (HFI_CMDQ_COUNT * HFI_CMDQ_HEAD_OFFSET);
	page = alloc_pages_node(dd->node, GFP_KERNEL | __GFP_ZERO,
				get_order(dd->cmdq_head_size));
	if (!page) {
		ret = -ENOMEM;
		goto err_post_alloc;
	}
	dd->cmdq_head_base = page_address(page);
	/* For ZEBU currently we configure cmdq after ctx_init */
	if (!dd->emulation)
		hfi_cmdq_config_all(dd);

	ret = hfi_at_init(dd);
	if (ret)
		goto err_post_alloc;

	if (!no_interrupts) {
		/* enable MSI-X or INTx */
		ret = hfi_setup_interrupts(dd, HFI_NUM_INTERRUPTS);
		if (ret)
			goto err_post_alloc;
		hfi_ack_all_interrupts(dd);

		/* configure IRQs */
		ret = hfi_setup_irqs(dd);
		if (ret)
			goto err_post_alloc;
	} else {
		dd_dev_info(dd, "Skipping MSI-X setup\n");
	}

	/* configure system PID/PASID needed by privileged CMDQs */
	ctx = &dd->priv_ctx;
	hfi_ctx_init(ctx, dd);
	ctx_assign.pid = HFI_PID_SYSTEM;
	ctx_assign.le_me_count = 0;
	ret = hfi_ctx_attach(ctx, &ctx_assign);
	if (ret)
		goto err_post_alloc;

	/* setup spill area after context attachment */
	if (hfi_alloc_spill_area(dd)) {
		ret = -ENOMEM;
		goto err_post_alloc;
	}

	/* For ZEBU currently we configure cmdq after ctx_init */
	if (dd->emulation)
		hfi_cmdq_config_all(dd);

	/*
	 * prefetch into shadow page table.
	 * unbinding system-pasid will remove it
	 */
	hfi_at_reg_range(ctx, dd->cmdq_head_base,
			 dd->cmdq_head_size, NULL, true);

	/* assign one CMDQ for privileged commands (DLID, EQ_DESC_WRITE) */
	ret = hfi_cmdq_assign_privileged(&dd->priv_cmdq, ctx);
	if (ret)
		goto err_post_alloc;
	ret = hfi_cmdq_map(&dd->priv_cmdq);
	if (ret)
		goto err_post_alloc;

	/* assign one EQ for privileged events */
	ret = hfi_eq_zero_assign(ctx);
	if (ret)
		goto err_post_alloc;

#ifdef CONFIG_HFI2_STLNP
	/* Initialize E2E only if psn cache is not being warmed */
	if (!use_psn_cache) {
		/*
		 * assign EQ for initiator E2E events and spawn thread to
		 * monitor E2E events on EQ zero
		 */
		ret = hfi_e2e_start(ctx);
		if (ret)
			goto err_post_alloc;
	}
#endif

	hfi_init_cntrs(dd);
	ret = hfi2_ib_add(dd);
	if (ret)
		goto err_post_alloc;

	/* read platform_config to dd->platform_config */
	hfi2_get_platform_config(dd);
	/* dd->platform_config -> dd->pcfg_cache */
	hfi2_parse_platform_config(dd);

	ret = hfi2_pport_link_init(dd);
	if (ret)
		goto err_post_alloc;

	obtain_boardname(dd); /* Set dd->boardname */
	ret = hfi_vnic_init(dd);
	if (ret)
		goto err_post_alloc;

	return dd;

err_post_alloc:
	dev_err(&pdev->dev, "%s %d FAILED ret %d\n", __func__, __LINE__, ret);
	if (!dd->emulation)
		hfi_pci_dd_free(dd);

	return ERR_PTR(ret);
}

static void hfi_cancel_pending_otr(struct hfi_devdata *dd, u16 ptl_pid)
{
	u64 cancel_msg_req1 = 0;

	/*
	 * Cancel pending OTR operations for current PID,
	 * except ipid_mask, all other masks are zero.
	 */
	write_csr(dd, FXR_TXOTR_MSG_CFG_CANCEL_MSG_REQ_4, 0);
	write_csr(dd, FXR_TXOTR_MSG_CFG_CANCEL_MSG_REQ_3, 0);
	write_csr(dd, FXR_TXOTR_MSG_CFG_CANCEL_MSG_REQ_2, 0);
	cancel_msg_req1 = FXR_TXOTR_MSG_CFG_CANCEL_MSG_REQ_1_IPID_MASK_SMASK;
	cancel_msg_req1 |= (FXR_TXOTR_MSG_CFG_CANCEL_MSG_REQ_1_IPID_MASK &
			    ptl_pid) <<
			    FXR_TXOTR_MSG_CFG_CANCEL_MSG_REQ_1_IPID_SHIFT;
	cancel_msg_req1 |= FXR_TXOTR_MSG_CFG_CANCEL_MSG_REQ_1_BUSY_SMASK;
	write_csr(dd, FXR_TXOTR_MSG_CFG_CANCEL_MSG_REQ_1, cancel_msg_req1);

	/* HW takes max 64K clock cycles on 1.2GHz, so delay a bit longer */
	mdelay(HFI_OTR_CANCELLATION_TIMEOUT_MS);
	/* for debugging purpose, read the CSR back */
	cancel_msg_req1 = read_csr(dd, FXR_TXOTR_MSG_CFG_CANCEL_MSG_REQ_1);
	if (cancel_msg_req1 & FXR_TXOTR_MSG_CFG_CANCEL_MSG_REQ_1_BUSY_SMASK)
		/* FXRTODO: Change to dd_dev_err once Simics is fixed */
		dd_dev_dbg(dd, "OTR cancellation not done after %dms, pid %d\n",
			   HFI_OTR_CANCELLATION_TIMEOUT_MS, ptl_pid);
}

void hfi_pcb_reset(struct hfi_devdata *dd, u16 ptl_pid)
{
	u64 pte = 0;
	u64 eq = 0;
	u64 trig = 0;
	u64 me_le_uh = 0;
	union pte_cache_addr pte_cache_tag;
	union eq_cache_addr eq_cache_tag;
	union trig_op_cache_addr trig_op_cache_tag;
	union psc_cache_addr me_le_uh_cache_tag;
	int time;
	int cache_inval_timeout = dd->emulation ?
		HFI_CACHE_INVALIDATION_TIMEOUT_MS_ZEBU :
		HFI_CACHE_INVALIDATION_TIMEOUT_MS;

	/* write PCB_LOW first to clear valid bit */
	write_csr(dd, FXR_RXHIARB_CFG_PCB_LOW + (ptl_pid * 8), 0);
	write_csr(dd, FXR_RXHIARB_CFG_PCB_HIGH + (ptl_pid * 8), 0);

	/* We need this lock to guard cache invalidation
	 * CSR writes, all pids use the same CSR
	 */
	spin_lock(&dd->ptl_lock);

	if (!dd->emulation)
		hfi_cancel_pending_otr(dd, ptl_pid);

	/* invalidate cached host memory in HFI for Portals Tables by PID */
	pte = (FXR_RXHP_CFG_PTE_CACHE_ACCESS_CTL_CMD_MASK &
			    FXR_CACHE_CMD_INVALIDATE) <<
			    FXR_RXHP_CFG_PTE_CACHE_ACCESS_CTL_CMD_SHIFT;
	pte_cache_tag.val = 0;
	pte_cache_tag.tpid = ptl_pid;
	pte |= (FXR_RXHP_CFG_PTE_CACHE_ACCESS_CTL_ADDRESS_MASK &
			     pte_cache_tag.val) <<
			     FXR_RXHP_CFG_PTE_CACHE_ACCESS_CTL_ADDRESS_SHIFT;
	pte_cache_tag.val = -1;
	pte_cache_tag.tpid = 0;
	pte |= (FXR_RXHP_CFG_PTE_CACHE_ACCESS_CTL_MASK_ADDRESS_MASK &
		pte_cache_tag.val) <<
		FXR_RXHP_CFG_PTE_CACHE_ACCESS_CTL_MASK_ADDRESS_SHIFT;
	pte |= FXR_RXHP_CFG_PTE_CACHE_ACCESS_CTL_BUSY_SMASK;
	write_csr(dd, FXR_RXHP_CFG_PTE_CACHE_ACCESS_CTL, pte);

	/* invalidate cached host memory in HFI for EQ Descs by PID */
	eq = (FXR_RXET_CFG_EQ_DESC_CACHE_ACCESS_CTL_CMD_MASK &
	      FXR_CACHE_CMD_INVALIDATE) <<
	      FXR_RXET_CFG_EQ_DESC_CACHE_ACCESS_CTL_CMD_SHIFT;
	eq_cache_tag.val = 0;
	eq_cache_tag.pid = ptl_pid;
	eq |= (FXR_RXET_CFG_EQ_DESC_CACHE_ACCESS_CTL_ADDRESS_MASK &
	       eq_cache_tag.val) <<
	       FXR_RXET_CFG_EQ_DESC_CACHE_ACCESS_CTL_ADDRESS_SHIFT;
	eq_cache_tag.val = -1;
	eq_cache_tag.pid = 0;
	eq |= (FXR_RXET_CFG_EQ_DESC_CACHE_ACCESS_CTL_MASK_ADDRESS_MASK &
	       eq_cache_tag.val) <<
	       FXR_RXET_CFG_EQ_DESC_CACHE_ACCESS_CTL_MASK_ADDRESS_SHIFT;
	eq |= FXR_RXET_CFG_EQ_DESC_CACHE_ACCESS_CTL_BUSY_SMASK;
	write_csr(dd, FXR_RXET_CFG_EQ_DESC_CACHE_ACCESS_CTL, eq);

	/* invalidate cached host memory in HFI for Triggered Ops by PID */
	trig = (FXR_RXET_CFG_TRIG_OP_CACHE_ACCESS_CTL_CMD_MASK &
		FXR_CACHE_CMD_INVALIDATE) <<
		FXR_RXET_CFG_TRIG_OP_CACHE_ACCESS_CTL_CMD_SHIFT;
	trig_op_cache_tag.val = 0;
	trig_op_cache_tag.pid = ptl_pid;
	trig |= (FXR_RXET_CFG_TRIG_OP_CACHE_ACCESS_CTL_ADDRESS_MASK &
		 trig_op_cache_tag.val) <<
		 FXR_RXET_CFG_TRIG_OP_CACHE_ACCESS_CTL_ADDRESS_SHIFT;
	trig_op_cache_tag.val = -1;
	trig_op_cache_tag.pid = 0;
	trig |= (FXR_RXET_CFG_TRIG_OP_CACHE_ACCESS_CTL_MASK_ADDRESS_MASK &
		 trig_op_cache_tag.val) <<
		 FXR_RXET_CFG_TRIG_OP_CACHE_ACCESS_CTL_MASK_ADDRESS_SHIFT;
	trig |= FXR_RXET_CFG_TRIG_OP_CACHE_ACCESS_CTL_BUSY_SMASK;
	write_csr(dd, FXR_RXET_CFG_TRIG_OP_CACHE_ACCESS_CTL, trig);

	/* invalidate cached host memory in HFI for ME/LE/UH */
	me_le_uh = (FXR_RXHP_CFG_PSC_CACHE_ACCESS_CTL_CMD_MASK &
		     FXR_CACHE_CMD_INVALIDATE) <<
		     FXR_RXHP_CFG_PSC_CACHE_ACCESS_CTL_CMD_SHIFT;
	me_le_uh_cache_tag.val = 0;
	me_le_uh_cache_tag.pid = ptl_pid;
	me_le_uh |= (FXR_RXHP_CFG_PSC_CACHE_ACCESS_CTL_ADDRESS_MASK &
		     me_le_uh_cache_tag.val) <<
		     FXR_RXHP_CFG_PSC_CACHE_ACCESS_CTL_ADDRESS_SHIFT;
	me_le_uh_cache_tag.val = -1;
	me_le_uh_cache_tag.pid = 0;
	me_le_uh |= (FXR_RXHP_CFG_PSC_CACHE_ACCESS_CTL_MASK_ADDRESS_MASK &
		     me_le_uh_cache_tag.val) <<
		     FXR_RXHP_CFG_PSC_CACHE_ACCESS_CTL_MASK_ADDRESS_SHIFT;
	me_le_uh |= FXR_RXHP_CFG_PSC_CACHE_ACCESS_CTL_BUSY_SMASK;

	write_csr(dd, FXR_RXHP_CFG_PSC_CACHE_ACCESS_CTL, me_le_uh);

	/* wait for completion, if timeout log a message */
	for (time = 0; time < HFI_CACHE_INVALIDATION_TIMEOUT_MS; time++) {
		mdelay(1);
		if (pte & FXR_RXHP_CFG_PTE_CACHE_ACCESS_CTL_BUSY_SMASK)
			pte = read_csr(dd, FXR_RXHP_CFG_PTE_CACHE_ACCESS_CTL);
		if (eq & FXR_RXET_CFG_EQ_DESC_CACHE_ACCESS_CTL_BUSY_SMASK)
			eq = read_csr(dd,
				      FXR_RXET_CFG_EQ_DESC_CACHE_ACCESS_CTL);
		if (trig & FXR_RXET_CFG_TRIG_OP_CACHE_ACCESS_CTL_BUSY_SMASK)
			trig = read_csr(dd,
					FXR_RXET_CFG_TRIG_OP_CACHE_ACCESS_CTL);
		if (me_le_uh & FXR_RXHP_CFG_PSC_CACHE_ACCESS_CTL_BUSY_SMASK)
			me_le_uh = read_csr(dd,
					    FXR_RXHP_CFG_PSC_CACHE_ACCESS_CTL);
		if (!(pte & FXR_RXHP_CFG_PTE_CACHE_ACCESS_CTL_BUSY_SMASK) &&
		    !(eq & FXR_RXET_CFG_EQ_DESC_CACHE_ACCESS_CTL_BUSY_SMASK) &&
		    !(trig &
		      FXR_RXET_CFG_TRIG_OP_CACHE_ACCESS_CTL_BUSY_SMASK) &&
		    !(me_le_uh & FXR_RXHP_CFG_PSC_CACHE_ACCESS_CTL_BUSY_SMASK))
			break;
		mdelay(1);
	}
	if (time >= cache_inval_timeout) {
		if (pte & FXR_RXHP_CFG_PTE_CACHE_ACCESS_CTL_BUSY_SMASK)
			dd_dev_err(dd, "PTE cache invalidation not done after %dms\n",
				   cache_inval_timeout);
		if (eq & FXR_RXET_CFG_EQ_DESC_CACHE_ACCESS_CTL_BUSY_SMASK)
			dd_dev_err(dd, "EQ cache invalidation not done after %dms\n",
				   cache_inval_timeout);
		if (trig & FXR_RXET_CFG_TRIG_OP_CACHE_ACCESS_CTL_BUSY_SMASK)
			dd_dev_err(dd, "trig_op cache invalidation not done after %dms\n",
				   cache_inval_timeout);
		if (me_le_uh & FXR_RXHP_CFG_PSC_CACHE_ACCESS_CTL_BUSY_SMASK)
			dd_dev_err(dd, "ME/LE/UH cache invalidation not done after %dms\n",
				   cache_inval_timeout);
	}

	spin_unlock(&dd->ptl_lock);
}

void hfi_pcb_write(struct hfi_ctx *ctx, u16 ptl_pid)
{
	struct hfi_devdata *dd = ctx->devdata;
	u64 pcb_low = 0;
	u64 pcb_high = 0;
	u64 psb_addr;

	psb_addr = (u64)ctx->ptl_state_base;

	/* write PCB in FXR */
	pcb_low = FXR_RXHIARB_CFG_PCB_LOW_VALID_SMASK;
	pcb_low |= (FXR_RXHIARB_CFG_PCB_LOW_PORTALS_STATE_BASE_MASK &
		   (psb_addr >> PAGE_SHIFT)) <<
		    FXR_RXHIARB_CFG_PCB_LOW_PORTALS_STATE_BASE_SHIFT;
	/*
	 * written as number of pages - 1, caller allocates at least one
	 * page
	 */
	pcb_high = (FXR_RXHIARB_CFG_PCB_HIGH_TRIGGERED_OP_SIZE_MASK &
		   ((ctx->trig_op_size >> PAGE_SHIFT) - 1)) <<
		    FXR_RXHIARB_CFG_PCB_HIGH_TRIGGERED_OP_SIZE_SHIFT;
	pcb_high |= (FXR_RXHIARB_CFG_PCB_HIGH_UNEXPECTED_SIZE_MASK &
		    ((ctx->unexpected_size >> PAGE_SHIFT) - 1)) <<
		     FXR_RXHIARB_CFG_PCB_HIGH_UNEXPECTED_SIZE_SHIFT;
	pcb_high |= (FXR_RXHIARB_CFG_PCB_HIGH_LE_ME_SIZE_MASK &
		    ((ctx->le_me_size >> PAGE_SHIFT) - 1)) <<
		     FXR_RXHIARB_CFG_PCB_HIGH_LE_ME_SIZE_SHIFT;

	write_csr(dd, FXR_RXHIARB_CFG_PCB_HIGH + (ptl_pid * 8), pcb_high);
	write_csr(dd, FXR_RXHIARB_CFG_PCB_LOW + (ptl_pid * 8), pcb_low);
}

/* Write CSR address for CMDQ head index, maintained by FXR */
static void hfi_cmdq_head_config(struct hfi_devdata *dd, u16 cmdq_idx,
				 void *head_base)
{
	u32 head_offset;
	u64 cq_head = 0;

	head_offset = FXR_RXCID_CFG_HEAD_UPDATE_ADDR + (cmdq_idx * 8);

	/* disable CMDQ head before reset, as no assigned PASID */
	write_csr(dd, head_offset, 0);

	/*
	 * reset CMDQ state, as CMDQ head starts at 0
	 *
	 * TODO: for FPGA, TX CMDQ size is reduced. Some issues seen
	 * reducing the TX CMDQ size. Until those issues are resolved,
	 * skip the initial CMDQ reset sequence.
	 */
	if (!dd->emulation)
		hfi_cmdq_disable(dd, cmdq_idx);

	/* set CQ head, should be set after CQ reset */
	cq_head = FXR_RXCID_CFG_HEAD_UPDATE_ADDR_VALID_SMASK;
	cq_head |= (FXR_RXCID_CFG_HEAD_UPDATE_ADDR_HD_PTR_HOST_ADDR_56_4_MASK &
		   ((u64)HFI_CMDQ_HEAD_ADDR(head_base, cmdq_idx) >> 4)) <<
		    FXR_RXCID_CFG_HEAD_UPDATE_ADDR_HD_PTR_HOST_ADDR_56_4_SHIFT;
	write_csr(dd, head_offset, cq_head);
}

/*
 * Disable pair of CMDQs and reset flow control.
 * Reset of flow control is needed since CMDQ might have only had
 * a partial command or slot written by errant user.
 */
void hfi_cmdq_disable(struct hfi_devdata *dd, u16 cmdq_idx)
{
	u64 tx = 0;
	u64 rx = 0;
	int time;
	int timeout = HFI_CMDQ_DRAIN_RESET_TIMEOUT_MS;
	int cmdq_reset_timeout = dd->emulation ?
		HFI_CMDQ_RESET_TIMEOUT_MS_ZEBU : HFI_CMDQ_RESET_TIMEOUT_MS;

	/* reset CQ state, as CQ head starts at 0 */
	tx = (FXR_TXCIC_CFG_DRAIN_RESET_DRAIN_CQ_MASK & cmdq_idx) <<
	      FXR_TXCIC_CFG_DRAIN_RESET_DRAIN_CQ_SHIFT;
	tx |= FXR_TXCIC_CFG_DRAIN_RESET_RESET_SMASK;
	tx |= FXR_TXCIC_CFG_DRAIN_RESET_DRAIN_SMASK;
	tx |= FXR_TXCIC_CFG_DRAIN_RESET_BUSY_SMASK;
	rx = (FXR_RXCIC_CFG_CQ_DRAIN_RESET_DRAIN_CQ_MASK & cmdq_idx) <<
		     FXR_RXCIC_CFG_CQ_DRAIN_RESET_DRAIN_CQ_SHIFT;
	rx |= FXR_RXCIC_CFG_CQ_DRAIN_RESET_RESET_SMASK;
	rx |= FXR_RXCIC_CFG_CQ_DRAIN_RESET_DRAIN_SMASK;
	rx |= FXR_RXCIC_CFG_CQ_DRAIN_RESET_BUSY_SMASK;

retry_reset:
	if (tx & FXR_TXCIC_CFG_DRAIN_RESET_RESET_SMASK)
		write_csr(dd, FXR_TXCIC_CFG_DRAIN_RESET, tx);

	if (rx & FXR_RXCIC_CFG_CQ_DRAIN_RESET_RESET_SMASK)
		write_csr(dd, FXR_RXCIC_CFG_CQ_DRAIN_RESET, rx);

	/* wait for completion, if timeout log a message */
	for (time = 0; time < timeout; time++) {
		mdelay(1);
		if (tx & FXR_TXCIC_CFG_DRAIN_RESET_BUSY_SMASK)
			tx = read_csr(dd, FXR_TXCIC_CFG_DRAIN_RESET);
		if (rx & FXR_RXCIC_CFG_CQ_DRAIN_RESET_BUSY_SMASK)
			rx = read_csr(dd, FXR_RXCIC_CFG_CQ_DRAIN_RESET);
		if (!(tx & FXR_TXCIC_CFG_DRAIN_RESET_BUSY_SMASK) &&
		    !(rx & FXR_RXCIC_CFG_CQ_DRAIN_RESET_BUSY_SMASK))
			break;
		if (dd->emulation)
			mdelay(1);
	}
	if (!dd->emulation && time >= timeout) {
		if ((tx & FXR_TXCIC_CFG_DRAIN_RESET_DRAIN_SMASK) ||
		    (rx & FXR_RXCIC_CFG_CQ_DRAIN_RESET_DRAIN_SMASK)) {
			if (tx & FXR_TXCIC_CFG_DRAIN_RESET_BUSY_SMASK) {
				tx &= ~FXR_TXCIC_CFG_DRAIN_RESET_DRAIN_SMASK;
				dd_dev_err(dd, "TX CQ reset and drain are not done after %dms..trying only reset\n",
					   cmdq_reset_timeout);
			}
			if (rx & FXR_RXCIC_CFG_CQ_DRAIN_RESET_BUSY_SMASK) {
				rx &= ~FXR_RXCIC_CFG_CQ_DRAIN_RESET_DRAIN_SMASK;
				dd_dev_err(dd, "RX CQ reset and drain are not done after %dms..trying only reset\n",
					   cmdq_reset_timeout);
			}
			/*
			 * FXRTODO revisit retry mechanism after design team
			 * confirmation
			 */
			tx |= (FXR_TXCIC_CFG_DRAIN_RESET_DRAIN_CQ_MASK &
			       cmdq_idx) <<
			       FXR_TXCIC_CFG_DRAIN_RESET_DRAIN_CQ_SHIFT;
			tx |= (FXR_TXCIC_CFG_DRAIN_RESET_RESET_MASK &
			      ((tx >>
			       FXR_TXCIC_CFG_DRAIN_RESET_BUSY_SHIFT) &
			       FXR_TXCIC_CFG_DRAIN_RESET_BUSY_MASK)) <<
			       FXR_TXCIC_CFG_DRAIN_RESET_RESET_SHIFT;
			rx |= (FXR_RXCIC_CFG_CQ_DRAIN_RESET_DRAIN_CQ_MASK &
			       cmdq_idx) <<
			       FXR_RXCIC_CFG_CQ_DRAIN_RESET_DRAIN_CQ_SHIFT;
			rx |= (FXR_RXCIC_CFG_CQ_DRAIN_RESET_RESET_MASK &
			      ((rx >>
			       FXR_RXCIC_CFG_CQ_DRAIN_RESET_BUSY_SHIFT) &
			       FXR_RXCIC_CFG_CQ_DRAIN_RESET_BUSY_MASK)) <<
			       FXR_RXCIC_CFG_CQ_DRAIN_RESET_RESET_SHIFT;
			timeout = cmdq_reset_timeout;
			goto retry_reset;
		}
		if (tx & FXR_TXCIC_CFG_DRAIN_RESET_BUSY_SMASK)
			dd_dev_err(dd, "TX CQ reset not done after %dms\n",
				   cmdq_reset_timeout);
		if (rx & FXR_RXCIC_CFG_CQ_DRAIN_RESET_BUSY_SMASK)
			dd_dev_err(dd, "RX CQ reset not done after %dms\n",
				   cmdq_reset_timeout);
	}
}

void hfi_cmdq_config_tuples(struct hfi_ctx *ctx, u16 cmdq_idx,
			    struct hfi_auth_tuple *auth_table)
{
	struct hfi_devdata *dd = ctx->devdata;
	int i;
	u32 offset;
	u64 reg = 0;

	/* write AUTH tuples */
	offset = FXR_TXCID_CFG_AUTHENTICATION_CSR +
		 (cmdq_idx * HFI_NUM_AUTH_TUPLES * 8);
	for (i = 0; i < HFI_NUM_AUTH_TUPLES; i++) {
		if (auth_table) {
			/* if BYPASS, values adjusted in validate_tuples */
			reg = (FXR_TXCID_CFG_AUTHENTICATION_CSR_USER_ID_MASK &
			       auth_table[i].uid) <<
			       FXR_TXCID_CFG_AUTHENTICATION_CSR_USER_ID_SHIFT;
			reg |= (HFI_AUTH_SRANK_MASK & auth_table[i].srank) <<
					HFI_AUTH_SRANK_SHIFT;
		} else {
			/* only kernel-clients can come through here */
			reg = (FXR_TXCID_CFG_AUTHENTICATION_CSR_USER_ID_MASK &
			       ctx->ptl_uid) <<
			       FXR_TXCID_CFG_AUTHENTICATION_CSR_USER_ID_SHIFT;
			if (IS_PID_BYPASS(ctx))
				reg |= (HFI_AUTH_SRANK_MASK & PTL_RANK_ANY) <<
					HFI_AUTH_SRANK_SHIFT;
		}
		write_csr(dd, offset, reg);
		offset += 8;
	}
}

void hfi_tpid_enable(struct hfi_devdata *dd, u8 idx, u16 base, u32 ptl_uid)
{
	u64 tpid = 0;

	tpid = FXR_RXHP_CFG_VTPID_CAM_VALID_SMASK;
	tpid |= (FXR_RXHP_CFG_VTPID_CAM_UID_MASK & ptl_uid) <<
		 FXR_RXHP_CFG_VTPID_CAM_UID_SHIFT;
	tpid |= (FXR_RXHP_CFG_VTPID_CAM_TPID_BASE_MASK & base) <<
		 FXR_RXHP_CFG_VTPID_CAM_TPID_BASE_SHIFT;
	write_csr(dd, FXR_RXHP_CFG_VTPID_CAM + (8 * idx), tpid);
}

void hfi_tpid_disable(struct hfi_devdata *dd, u8 idx)
{
	write_csr(dd, FXR_RXHP_CFG_VTPID_CAM + (8 * idx), 0);
}

/*
 * Write CSRs to configure a TX and RX Command Queue.
 * Authentication Tuple UIDs have been pre-validated by caller.
 */
void hfi_cmdq_config(struct hfi_ctx *ctx, u16 cmdq_idx,
		     struct hfi_auth_tuple *auth_table, bool user_priv)
{
	struct hfi_devdata *dd = ctx->devdata;
	u32 offset;
	u64 tx_config = 0;
	u64 rx_config = 0;

	hfi_cmdq_config_tuples(ctx, cmdq_idx, auth_table);

	/* set TX CMDQ config, enable */
	tx_config = FXR_TXCID_CFG_CSR_ENABLE_SMASK;
	tx_config |= (FXR_TXCID_CFG_CSR_PID_MASK & ctx->pid) <<
			 FXR_TXCID_CFG_CSR_PID_SHIFT;
	tx_config |= (FXR_TXCID_CFG_CSR_PRIV_LEVEL_MASK & user_priv) <<
			 FXR_TXCID_CFG_CSR_PRIV_LEVEL_SHIFT;
	if (ctx->res.lid_count)
		tx_config |= (FXR_TXCID_CFG_CSR_DLID_BASE_MASK &
				 ctx->res.dlid_base) <<
				 FXR_TXCID_CFG_CSR_DLID_BASE_SHIFT;
	tx_config |= (FXR_TXCID_CFG_CSR_PHYS_DLID_MASK &
			 ctx->res.allow_phys_dlid) <<
			 FXR_TXCID_CFG_CSR_PHYS_DLID_SHIFT;
	/*
	 * Use enabled SLs from port 0. MC1 assumes all SLs are enabled.
	 * This is to properly restrict user-space selection of SL.
	 * Kernel ULPs are trusted and might allocate resources before SL pairs
	 * are configured, so allow any SL for kernel contexts.
	 */
	if (ctx->type == HFI_CTX_TYPE_KERNEL)
		tx_config |= FXR_TXCID_CFG_CSR_SL_ENABLE_SMASK;
	else if (IS_PID_BYPASS(ctx))
		tx_config |= (FXR_TXCID_CFG_CSR_SL_ENABLE_MASK &
				dd->pport->sl_mask) <<
				FXR_TXCID_CFG_CSR_SL_ENABLE_SHIFT;
	else
		tx_config |= (FXR_TXCID_CFG_CSR_SL_ENABLE_MASK &
				dd->pport->req_sl_mask) <<
				FXR_TXCID_CFG_CSR_SL_ENABLE_SHIFT;
	offset = FXR_TXCID_CFG_CSR + (cmdq_idx * 8);
	write_csr(dd, offset, tx_config);

#if 0
	/* FXRTODO: Need to program the pkey to be used for a CQ */
	offset = FXR_TXCID_CFG_EXT_CSR + (cmdq_idx * 8);
	tx_config = (0x2 << FXR_TXCID_CFG_EXT_CSR_PKEY_SHIFT);
	write_csr(dd, offset, tx_config);
#endif

	/* set RX CQ config, enable */
	rx_config = FXR_RXCID_CFG_CNTRL_ENABLE_SMASK;
	rx_config |= (FXR_RXCID_CFG_CNTRL_PID_MASK & ctx->pid) <<
			 FXR_RXCID_CFG_CNTRL_PID_SHIFT;
	rx_config |= (FXR_RXCID_CFG_CNTRL_PRIV_LEVEL_MASK & user_priv) <<
			 FXR_RXCID_CFG_CNTRL_PRIV_LEVEL_SHIFT;
	offset = FXR_RXCID_CFG_CNTRL + (cmdq_idx * 8);
	write_csr(dd, offset, rx_config);
}

int hfi_ctx_hw_addr(struct hfi_ctx *ctx, int type, u16 ctxt, void **addr,
		    ssize_t *len)
{
	struct hfi_devdata *dd = ctx->devdata;
	void *psb_base = NULL, *cmdq_addr;
	struct hfi_ctx *cmdq_ctx;
	unsigned long flags;
	int ret = 0;

	/*
	 * Validate passed in ctxt value.
	 * For CMDQs, verify ownership of this CMDQ before allowing remapping.
	 * Unused for per-PID state, we are already restricted to
	 * host memory associated with the PID (ctx->ptl_state_base).
	 */
	switch (type) {
	case TOK_CMDQ_TX:
	case TOK_CMDQ_RX:
	case TOK_CMDQ_HEAD:
		spin_lock_irqsave(&dd->cmdq_lock, flags);
		cmdq_ctx = idr_find(&dd->cmdq_pair, ctxt);
		spin_unlock_irqrestore(&dd->cmdq_lock, flags);
		if (cmdq_ctx != ctx)
			return -EINVAL;
		break;
	default:
		psb_base = ctx->ptl_state_base;
		if (!psb_base)
			return -EINVAL;
		break;
	}

	switch (type) {
	case TOK_CMDQ_TX:
		/*
		 * TX/RX cmdq's are mapped WC but cmdq pa must be returned
		 * for user contexts for use in mmap(). Kernel clients
		 * directly use the ioremap'd va.
		 */
		/* ioremap_wc va/pa - RW */
		cmdq_addr = (ctx->type == HFI_CTX_TYPE_KERNEL) ?
			dd->cmdq_tx_wc : (void *)dd->cmdq_tx_base;
		*addr = HFI_CMDQ_TX_IDX_ADDR(cmdq_addr, ctxt);
		*len = HFI_CMDQ_TX_SIZE;
		break;
	case TOK_CMDQ_RX:
		/* ioremap_wc va/pa - RW */
		cmdq_addr = (ctx->type == HFI_CTX_TYPE_KERNEL) ?
			dd->cmdq_rx_wc : (void *)dd->cmdq_rx_base;
		*addr = HFI_CMDQ_RX_IDX_ADDR(cmdq_addr, ctxt);
		*len = PAGE_ALIGN(HFI_CMDQ_RX_SIZE);
		break;
	case TOK_CMDQ_HEAD:
		/* kmalloc - RO */
		*addr = HFI_CMDQ_HEAD_ADDR(dd->cmdq_head_base, ctxt);
		*len = PAGE_SIZE;
		break;
	case TOK_EVENTS_CT:
		/* vmalloc - RO */
		*addr = (psb_base + HFI_PSB_CT_OFFSET);
		*len = HFI_PSB_CT_SIZE;
		break;
	case TOK_EVENTS_EQ_DESC:
		/* vmalloc - RO */
		*addr = (psb_base + HFI_PSB_EQ_DESC_OFFSET);
		*len = HFI_PSB_EQ_DESC_SIZE;
		break;
	case TOK_EVENTS_EQ_HEAD:
		/* vmalloc - RW */
		*addr = (psb_base + HFI_PSB_EQ_HEAD_OFFSET);
		*len = HFI_PSB_EQ_HEAD_SIZE;
		break;
	case TOK_PORTALS_TABLE:
		/* vmalloc - RO */
		*addr = (psb_base + HFI_PSB_PT_OFFSET);
		*len = HFI_PSB_PT_SIZE;
		break;
	case TOK_TRIG_OP:
		/* vmalloc - RO */
		*addr = (psb_base + HFI_PSB_TRIG_OFFSET);
		*len = ctx->trig_op_size;
		break;
	case TOK_LE_ME:
		/* vmalloc - RO */
		*addr = ctx->le_me_addr;
		*len = ctx->le_me_size;
		break;
	case TOK_UNEXPECTED:
		/* vmalloc - RO */
		*addr = ctx->le_me_addr + ctx->le_me_size;
		*len = ctx->unexpected_size;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

/* set the 8-bit sub-field in u64 */
static u64 insert_u8(u64 val, u8 idx, u8 ins_val)
{
	val &= ~(0xFFuLL << (idx * 8));
	val |= (u64)ins_val << (idx * 8);
	return val;
}

void hfi_rsm_clear_rule(struct hfi_devdata *dd, u8 rule_idx)
{
	/* Update bitmap and disable rule in hardware */
	spin_lock(&dd->ptl_lock);
	bitmap_clear(dd->rsm_map, dd->rsm_offset[rule_idx],
		     dd->rsm_size[rule_idx]);
	dd->rsm_size[rule_idx] = 0;
	write_csr(dd, FXR_RXHP_CFG_NPTL_RSM_CFG + (8 * rule_idx), 0);
	spin_unlock(&dd->ptl_lock);
}

int hfi_rsm_set_rule(struct hfi_devdata *dd, struct hfi_rsm_rule *rule,
		     struct hfi_ctx *rx_ctx[], u16 num_contexts)
{
	u64 rsm_cfg = 0;
	u64 select = 0;
	u64 match = 0;
	int i, ret = 0;
	u8 idx = rule->idx;
	u16 map_idx, map_size;
	u32 csr_off;
	u64 csr_val;

	if (rule->idx >= HFI_NUM_RSM_RULES)
		return -EINVAL;
	if (rule->select_width[0] > 8)
		return -EINVAL;
	if (rule->select_width[1] > 8)
		return -EINVAL;
	map_size = 1 << max_t(u8, rule->select_width[0], rule->select_width[1]);
	/* specified number of contexts must match RSM_MAP entries */
	if (map_size != num_contexts)
		return -EINVAL;
	/* verify contexts were correctly setup for bypass mode */
	for (i = 0; i < map_size; i++)
		if (!IS_PID_BYPASS(rx_ctx[i]))
			return -EINVAL;

	spin_lock(&dd->ptl_lock);
	/* RSM rule is in use if already has RSM_MAP entries */
	if (dd->rsm_size[idx]) {
		ret = -EBUSY;
		goto done;
	}

	/* find space in RSM map */
	map_idx = bitmap_find_next_zero_area(dd->rsm_map, HFI_RSM_MAP_SIZE,
					     0, map_size, 0);
	if (map_idx >= HFI_RSM_MAP_SIZE) {
		ret = -EBUSY;
		goto done;
	}
	bitmap_set(dd->rsm_map, map_idx, map_size);
	/* record information for validation in setting RSM_MAP entry later */
	dd->rsm_offset[idx] = map_idx;
	dd->rsm_size[idx] = map_size;

	rsm_cfg = (1 << idx); /* set Enable bit */
	rsm_cfg |= (FXR_RXHP_CFG_NPTL_RSM_CFG_BYPASS_HDR_SIZE_MASK &
		    rule->hdr_size) <<
		    FXR_RXHP_CFG_NPTL_RSM_CFG_BYPASS_HDR_SIZE_SHIFT;
	/* packet matching criteria */
	rsm_cfg |= (FXR_RXHP_CFG_NPTL_RSM_CFG_PACKET_TYPE_MASK &
		    rule->pkt_type) <<
		    FXR_RXHP_CFG_NPTL_RSM_CFG_PACKET_TYPE_SHIFT;
	match |= (FXR_RXHP_CFG_NPTL_RSM_MATCH_MASK1_MASK &
		  rule->match_mask[0]) <<
		  FXR_RXHP_CFG_NPTL_RSM_MATCH_MASK1_SHIFT;
	match |= (FXR_RXHP_CFG_NPTL_RSM_MATCH_VALUE1_MASK &
		  rule->match_value[0]) <<
		  FXR_RXHP_CFG_NPTL_RSM_MATCH_VALUE1_SHIFT;
	match |= (FXR_RXHP_CFG_NPTL_RSM_MATCH_MASK2_MASK &
		  rule->match_mask[1]) <<
		  FXR_RXHP_CFG_NPTL_RSM_MATCH_MASK2_SHIFT;
	match |= (FXR_RXHP_CFG_NPTL_RSM_MATCH_VALUE2_MASK &
		  rule->match_value[1]) <<
		  FXR_RXHP_CFG_NPTL_RSM_MATCH_VALUE2_SHIFT;
	select |= (FXR_RXHP_CFG_NPTL_RSM_SELECT_FIELD1_OFFSET_MASK &
		   rule->match_offset[0]) <<
		   FXR_RXHP_CFG_NPTL_RSM_SELECT_FIELD1_OFFSET_SHIFT;
	select |= (FXR_RXHP_CFG_NPTL_RSM_SELECT_FIELD2_OFFSET_MASK &
		   rule->match_offset[1]) <<
		   FXR_RXHP_CFG_NPTL_RSM_SELECT_FIELD2_OFFSET_SHIFT;
	/* context selection criteria */
	rsm_cfg |= (FXR_RXHP_CFG_NPTL_RSM_CFG_OFFSET_MASK & map_idx) <<
		    FXR_RXHP_CFG_NPTL_RSM_CFG_OFFSET_SHIFT;
	select |= (FXR_RXHP_CFG_NPTL_RSM_SELECT_INDEX1_OFFSET_MASK &
		   rule->select_offset[0]) <<
		   FXR_RXHP_CFG_NPTL_RSM_SELECT_INDEX1_OFFSET_SHIFT;
	select |= (FXR_RXHP_CFG_NPTL_RSM_SELECT_INDEX1_WIDTH_MASK &
		   rule->select_width[0]) <<
		   FXR_RXHP_CFG_NPTL_RSM_SELECT_INDEX1_WIDTH_SHIFT;
	select |= (FXR_RXHP_CFG_NPTL_RSM_SELECT_INDEX2_OFFSET_MASK &
		   rule->select_offset[1]) <<
		   FXR_RXHP_CFG_NPTL_RSM_SELECT_INDEX2_OFFSET_SHIFT;
	select |= (FXR_RXHP_CFG_NPTL_RSM_SELECT_INDEX2_WIDTH_MASK &
		   rule->select_width[1]) <<
		   FXR_RXHP_CFG_NPTL_RSM_SELECT_INDEX2_WIDTH_SHIFT;

	write_csr(dd, FXR_RXHP_CFG_NPTL_RSM_CFG + (8 * idx), rsm_cfg);
	write_csr(dd, FXR_RXHP_CFG_NPTL_RSM_SELECT + (8 * idx), select);
	write_csr(dd, FXR_RXHP_CFG_NPTL_RSM_MATCH + (8 * idx), match);

	/* Write hardware RSM context mapping table */
	csr_off = FXR_RXHP_CFG_NPTL_RSM_MAP_TABLE + (map_idx / 8) * 8;
	csr_val = read_csr(dd, csr_off);
	for (i = 0; i < map_size; i++) {
		csr_val = insert_u8(csr_val, (map_idx + i) % 8,
				    rx_ctx[i]->pid - HFI_PID_BYPASS_BASE);
		rx_ctx[i]->rsm_mask |= (1 << idx);
		/* If last entry or we filled all entries in CSR, write it */
		if (i + 1 >= map_size) {
			write_csr(dd, csr_off, csr_val);
			break;
		} else if ((map_idx + i) % 8 == 7) {
			write_csr(dd, csr_off, csr_val);
			csr_off += 8;
			csr_val = read_csr(dd, csr_off);
		}
	}

	dd_dev_dbg(dd, "active RSM rule %d off %d size %d\n",
		   idx, map_idx, map_size);
done:
	spin_unlock(&dd->ptl_lock);
	return ret;
}

void hfi_ctx_clear_bypass(struct hfi_ctx *ctx)
{
	struct hfi_devdata *dd = ctx->devdata;
	u64 bypass = 0;

	spin_lock(&dd->ptl_lock);
	if (ctx->pid == dd->bypass_pid) {
		bypass = FXR_RXHP_CFG_NPTL_BYPASS_BYPASS_CONTEXT_SMASK;
		write_csr(dd, FXR_RXHP_CFG_NPTL_BYPASS, bypass);
		dd->bypass_pid = HFI_PID_NONE;
	}
	spin_unlock(&dd->ptl_lock);
}

/*
 * If not already in use, claim use of default bypass PID.
 * Must use RSM to distribute packets for other bypass contexts.
 */
int hfi_ctx_set_bypass(struct hfi_ctx *ctx, u8 hdr_size)
{
	struct hfi_devdata *dd = ctx->devdata;
	int ret = 0;
	u64 bypass = 0;

	if (!IS_PID_BYPASS(ctx))
		return -EINVAL;

	spin_lock(&dd->ptl_lock);
	if (dd->bypass_pid != HFI_PID_NONE) {
		ret = -EBUSY;
		goto done;
	}
	bypass = (FXR_RXHP_CFG_NPTL_BYPASS_BYPASS_CONTEXT_MASK &
		 (ctx->pid - HFI_PID_BYPASS_BASE)) <<
		  FXR_RXHP_CFG_NPTL_BYPASS_BYPASS_CONTEXT_SHIFT;
	bypass |= (FXR_RXHP_CFG_NPTL_BYPASS_HDR_SIZE_MASK & hdr_size) <<
		   FXR_RXHP_CFG_NPTL_BYPASS_HDR_SIZE_SHIFT;
	write_csr(dd, FXR_RXHP_CFG_NPTL_BYPASS, bypass);
	dd->bypass_pid = ctx->pid;
done:
	spin_unlock(&dd->ptl_lock);
	return ret;
}

void hfi_ctx_set_mcast(struct hfi_ctx *ctx)
{
	struct hfi_devdata *dd = ctx->devdata;

	write_csr(dd, FXR_RXHP_CFG_NPTL_MULTICAST,
		  ctx->pid - HFI_PID_BYPASS_BASE);
	dd->mcast_pid = ctx->pid;
}

void hfi_ctx_set_vl15(struct hfi_ctx *ctx)
{
	struct hfi_devdata *dd = ctx->devdata;

	write_csr(dd, FXR_RXHP_CFG_NPTL_VL15, ctx->pid - HFI_PID_BYPASS_BASE);
	dd->vl15_pid = ctx->pid;
}

/*
 * The QPN mapping table is indexed using BTH.DestQP[8..1].
 * This is the backup mechanism for receive side scaling if RSM is not used.
 * Write the mapping table to spread across the provided set of contexts.
 * This should be a power of 2, so the mapping is uniform, but we don't
 * enforce this.
 */
void hfi_ctx_set_qp_table(struct hfi_devdata *dd, struct hfi_ctx *rx_ctx[],
			  u16 num_contexts)
{
	int i;
	u32 pid, csr_off;
	u64 csr_val = 0;

	/*
	 * iterate over all QPN_MAP entries, write appropriate CSR after
	 * we've filled in the 8 entries per CSR
	 */
	csr_off = FXR_RXHP_CFG_NPTL_QP_MAP_TABLE;
	for (i = 0; i < HFI2_QPN_MAP_MAX; i++) {
		pid = rx_ctx[i % num_contexts]->pid - HFI_PID_BYPASS_BASE;
		csr_val = insert_u8(csr_val, i % 8, pid);
		if (i % 8 == 7) {
			write_csr(dd, csr_off, csr_val);
			csr_off += 8;
			csr_val = 0;
		}
	}
}

/*
 * Set dd->boardname.  Use a generic name if a name is not returned from
 * EFI variable space.
 *
 * Return 0 on success, -ENOMEM if space could not be allocated.
 */
static int obtain_boardname(struct hfi_devdata *dd)
{
	/* generic board description */
	const char generic[] =
		"Intel Omni-Path Host Fabric Interface Adapter 100 Series";
#if 0 /* FXRTODO: implement read_hfi2_efi_var. */
	unsigned long size;
	int ret;

	ret = read_hfi2_efi_var(dd, "description", &size,
				(void **)&dd->boardname);
	if (ret) {
		dd_dev_info(dd, "Board description not found\n");
#endif
		/* use generic description */
		dd->boardname = kstrdup(generic, GFP_KERNEL);
		if (!dd->boardname)
			return -ENOMEM;
#if 0 /* FXRTODO: implement read_hfi2_efi_var. */
	}
#endif
	return 0;
}

void __init hfi_mod_init(void)
{
	idr_init(&hfi2_unit_table);
}

void hfi_mod_deinit(void)
{
	idr_destroy(&hfi2_unit_table);
}
