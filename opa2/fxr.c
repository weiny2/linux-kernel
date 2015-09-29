/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2014 - 2015 Intel Corporation.
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
 * Copyright (c) 2014 - 2015 Intel Corporation.
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
 *
 * Intel(R) Omni-Path Gen2 HFI PCIe Driver
 */

#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/utsname.h>
#if 1 /* TODO: should be __SIMICS__ instead of 1 */
#include <linux/delay.h>
#endif
#include <linux/bitops.h>
#include <rdma/fxr/fxr_top_defs.h>
#include <rdma/fxr/fxr_fast_path_defs.h>
#include <rdma/fxr/fxr_tx_ci_cic_csrs.h>
#include <rdma/fxr/fxr_tx_ci_cid_csrs.h>
#include <rdma/fxr/fxr_rx_ci_cic_csrs.h>
#include <rdma/fxr/fxr_rx_ci_cid_csrs.h>
#include <rdma/fxr/fxr_rx_et_defs.h>
#include <rdma/fxr/fxr_rx_et_csrs.h>
#include <rdma/fxr/fxr_rx_hiarb_defs.h>
#include <rdma/fxr/fxr_rx_hiarb_csrs.h>
#include <rdma/fxr/fxr_rx_hp_defs.h>
#include <rdma/fxr/fxr_rx_hp_csrs.h>
#include <rdma/fxr/fxr_rx_e2e_csrs.h>
#include <rdma/fxr/fxr_rx_e2e_defs.h>
#include <rdma/fxr/fxr_lm_csrs.h>
#include <rdma/fxr/fxr_lm_tp_csrs.h>
#include <rdma/fxr/fxr_lm_fpc_csrs.h>
#include <rdma/fxr/fxr_linkmux_tp_defs.h>
#include <rdma/fxr/fxr_linkmux_fpc_defs.h>
#include <rdma/fxr/fxr_fc_defs.h>
#include <rdma/fxr/fxr_tx_otr_pkt_top_csrs_defs.h>
#include <rdma/fxr/fxr_tx_otr_pkt_top_csrs.h>
#include <rdma/fxr/fxr_tx_otr_msg_top_csrs_defs.h>
#include <rdma/fxr/fxr_pcim_defs.h>
#include "opa_hfi.h"
#include <rdma/opa_core_ib.h>
#include "link.h"

/* TODO - should come from HW headers */
#define FXR_CACHE_CMD_INVALIDATE 0x8

#define DLID_TABLE_SIZE (64 * 1024) /* 64k */

/* TODO: delete module parameter when possible */
static uint force_loopback;
module_param(force_loopback, uint, S_IRUGO);
MODULE_PARM_DESC(force_loopback, "Force FXR into loopback");

/* FXRTODO: Remove this once we have opafm working with B2B setup */
uint opafm_disable = 0;
module_param_named(opafm_disable, opafm_disable, uint, S_IRUGO);
MODULE_PARM_DESC(opafm_disable, "0 - driver needs opafm to work, \
			        1 - driver works without opafm");

static void hfi_cq_head_config(struct hfi_devdata *dd, u16 cq_idx,
			       void *head_base);

static void write_lm_fpc_csr(const struct hfi_pportdata *ppd,
			     u32 offset, u64 value)
{
	if (ppd->pnum == 1)
		offset += FXR_LM_FPC0_CSRS;
	else if (ppd->pnum == 2)
		offset += FXR_LM_FPC1_CSRS;
	else
		dd_dev_warn(ppd->dd, "invalid port");

	write_csr(ppd->dd, offset, value);
}

static void write_lm_tp_csr(const struct hfi_pportdata *ppd,
			    u32 offset, u64 value)
{
	if (ppd->pnum == 1)
		offset += FXR_LM_TP0_CSRS;
	else if (ppd->pnum == 2)
		offset += FXR_LM_TP1_CSRS;
	else
		dd_dev_warn(ppd->dd, "invalid port");

	write_csr(ppd->dd, offset, value);
}

static void write_lm_cm_csr(const struct hfi_pportdata *ppd,
			    u32 offset, u64 value)
{
	if (ppd->pnum == 1)
		offset += FXR_LM_CM0_CSRS;
	else if (ppd->pnum == 2)
		offset += FXR_LM_CM1_CSRS;
	else
		dd_dev_warn(ppd->dd, "invalid port");

	write_csr(ppd->dd, offset, value);
}

static u64 read_lm_tp_csr(const struct hfi_pportdata *ppd,
			  u32 offset)
{
	if (ppd->pnum == 1)
		offset += FXR_LM_TP0_CSRS;
	else if (ppd->pnum == 2)
		offset += FXR_LM_TP1_CSRS;
	else
		dd_dev_warn(ppd->dd, "invalid ppd->pnum");

	return read_csr(ppd->dd, offset);
}

static void hfi_init_rx_e2e_csrs(const struct hfi_devdata *dd)
{
	RXE2E_CFG_MC0_HDR_FLIT_CNT_CAM_t flt = {.val = 0};
	RXE2E_CFG_VALID_TC_SLID_t tc_slid = {.val = 0};
	u32 offset = 0;

	/* lowest entries in the CAM have the highest priority */
	flt.field.valid = 1;
	flt.field.l2 = HDR_EXT;
	flt.field.l2_mask = 0;
	flt.field.l4_mask = ~0;
	flt.field.opcode_mask = ~0;
	flt.field.cnt_m1 = 3;
	write_csr(dd, FXR_RXE2E_CFG_MC0_HDR_FLIT_CNT_CAM + offset, flt.val);

	flt.field.valid = 1;
	flt.field.l4 = PTL_RTS;
	flt.field.l2_mask = ~0;
	flt.field.l4_mask = 0;
	flt.field.opcode_mask = ~0;
	flt.field.cnt_m1 = 2;
	offset += 8;
	write_csr(dd, FXR_RXE2E_CFG_MC0_HDR_FLIT_CNT_CAM + offset, flt.val);

	flt.field.l4 = PTL_REND_EVENT;
	offset += 8;
	write_csr(dd, FXR_RXE2E_CFG_MC0_HDR_FLIT_CNT_CAM + offset, flt.val);

	tc_slid.field.max_slid_p0 = HFI_MAX_LID_SUPP;
	tc_slid.field.tc_valid_p0 = 0xf;
	tc_slid.field.max_slid_p1 = HFI_MAX_LID_SUPP;
	tc_slid.field.tc_valid_p1 = 0xf;
	write_csr(dd, FXR_RXE2E_CFG_VALID_TC_SLID, tc_slid.val);
}

static int hfi_mtu_to_mc_tc_shift(const struct hfi_devdata *dd, int mtu)
{
	switch (mtu) {
	case 128: return 0;
	case IB_MTU_256: return 1;
	case IB_MTU_512: return 2;
	case IB_MTU_1024: return 3;
	case IB_MTU_2048: return 4;
	case IB_MTU_4096: return 5;
	case OPA_MTU_8192: return 6;
	case OPA_MTU_10240: return 7;
	default:
		dd_dev_warn(dd, "invalid mtu");
		return INVALID_MTU_ENC;
	}
}

static void hfi_init_tx_otr_mtu(const struct hfi_devdata *dd, int mtu)
{
	int i;
	u64 reg = 0;

	u64 val = hfi_mtu_to_mc_tc_shift(dd, mtu);

	for (i = 0; i < 8; i++)
		reg |= (val << (i * 4));
	write_csr(dd, FXR_TXOTR_PKT_CFG_RFS, reg);
#if 0
	/* Error seen when writing to these registers */
	/* TODO: These are not implemented in Simics yet */
	write_csr(dd, FXR_TXOTR_MSG_CFG_MTU_PT0, reg);
	write_csr(dd, FXR_TXOTR_MSG_CFG_MTU_PT1, reg);
#endif
}

static void hfi_init_tx_otr_csrs(const struct hfi_devdata *dd)
{
	txotr_pkt_cfg_valid_tc_dlid_t tc_slid = {.val = 0};

	tc_slid.field.max_dlid_p0 = HFI_MAX_LID_SUPP;
	tc_slid.field.tc_valid_p0 = 0xf;
	tc_slid.field.max_dlid_p1 = HFI_MAX_LID_SUPP;
	tc_slid.field.tc_valid_p1 = 0xf;
	write_csr(dd, FXR_TXOTR_PKT_CFG_VALID_TC_DLID, tc_slid.val);

	/* FXRTODO: Re-initialize if FM requests different MTU? */
	hfi_init_tx_otr_mtu(dd, IB_MTU_4096);
}

static void hfi_init_tx_cid_csrs(const struct hfi_devdata *dd)
{
	TXCID_CFG_MAD_TO_TC_t reg;

	/* Set up the traffic and message class for VL15 */
	reg.val = read_csr(dd, FXR_TXCID_CFG_MAD_TO_TC);
	reg.field.tc = HFI_VL15_TC;
	reg.field.mc = HFI_VL15_MC;
	write_csr(dd, FXR_TXCID_CFG_MAD_TO_TC, reg.val);
}

static void init_csrs(struct hfi_devdata *dd)
{
	RXHP_CFG_NPTL_CTL_t nptl_ctl = {.val = 0};
	RXHP_CFG_NPTL_BTH_QP_t kdeth_qp = {.val = 0};

	/* enable non-portals */
	nptl_ctl.field.RcvQPMapEnable = 1;
	nptl_ctl.field.RcvBypassEnable = 1;
	write_csr(dd, FXR_RXHP_CFG_NPTL_CTL, nptl_ctl.val);
	kdeth_qp.field.KDETH_QP = (OPA_QPN_KDETH_BASE >> 16);
	write_csr(dd, FXR_RXHP_CFG_NPTL_BTH_QP, kdeth_qp.val);

	if (force_loopback) {
		LM_CONFIG_t lm_config = {.val = 0};

		lm_config.field.FORCE_LOOPBACK = force_loopback;
		write_csr(dd, FXR_LM_CONFIG, lm_config.val);
	 } else {
		LM_CONFIG_PORT0_t lmp0 = {.val = 0};
		LM_CONFIG_PORT1_t lmp1 = {.val = 0};
		txotr_pkt_cfg_timeout_t txotr_timeout = {.val = 0};

		txotr_timeout.val = read_csr(dd, FXR_TXOTR_PKT_CFG_TIMEOUT);
		/*
		 * Set the length of the timeout interval used to retransmit
		 * outstanding packets to 5 seconds for Simics back to back.
		 * TODO: Determine right value for FPGA & Silicon.
		 */
		txotr_timeout.field.SCALER = 0x165A0D;
		write_csr(dd, FXR_TXOTR_PKT_CFG_TIMEOUT, txotr_timeout.val);
		/*
		 * Set the SLID based on the hostname to enable back to back
		 * support in Simics.
		 * TODO: Delete this hack once the LID is received via MADs
		 */
		if (!strncmp(utsname()->nodename, "viper", 5)) {
			const char *hostname = utsname()->nodename;
			int node = 0, rc;

			/* Extract the node id from the host name */
			hostname += 5;
			rc = kstrtoint(hostname, 0, &node);
			if (rc)
				dd_dev_info(dd, "kstrtoint fail %d\n", rc);
			if (opafm_disable) {
				dd->pport[0].lid = (node * 2) + 1;
				dd->pport[1].lid = (node * 2) + 2;
			}
			dd_dev_info(dd, "viper%d port0 lid %d port1 lid %d\n",
				    node, dd->pport[0].lid, dd->pport[1].lid);
			/*
			 * FXRTODO: read nodeguid from MNH Register
			 * 8051_CFG_LOCAL_GUID. This register is
			 * yet to be implemented in Simics. Temporarily
			 * keeping node guid unique
			 */
			dd->nguid = cpu_to_be64(NODE_GUID + node);

			/*
			 * FXRTODO: Read this from DC_DC8051_STS_REMOTE_GUID
			 * equivalent in MNH. The neighbor guid is set to a
			 * random value for now.
			 *
			 * Move this code to 8051 INTR handler
			 */
			dd->pport[0].neighbor_guid = dd->nguid + 0x1000,
			dd->pport[1].neighbor_guid = dd->nguid + 0x1000;
		}
		if (opafm_disable) {
			lmp0.field.DLID = dd->pport[0].lid;
			write_csr(dd, FXR_LM_CONFIG_PORT0, lmp0.val);
			write_csr(dd, FXR_TXOTR_PKT_CFG_SLID_PT0,
							dd->pport[0].lid);
			lmp1.field.DLID = dd->pport[1].lid;
			write_csr(dd, FXR_LM_CONFIG_PORT1, lmp1.val);
			write_csr(dd, FXR_TXOTR_PKT_CFG_SLID_PT1,
							dd->pport[1].lid);
		}
	}
	hfi_init_rx_e2e_csrs(dd);
	hfi_init_tx_otr_csrs(dd);
	hfi_init_tx_cid_csrs(dd);
}

static int hfi_psn_init(const struct hfi_devdata *dd)
{
	int i, j, rc = 0;
	struct hfi_pportdata *port;
	u32 tx_offset = FXR_TXOTR_PKT_CFG_PSN_BASE_ADDR_P0_TC;
	txotr_pkt_cfg_psn_base_addr_p0_tc_t tx_psn_base = {.val = 0};
	u32 rx_offset = FXR_RXE2E_CFG_PSN_BASE_ADDR_P0_TC;
	RXE2E_CFG_PSN_BASE_ADDR_P0_TC_t rx_psn_base = {.val = 0};

	for (i = 0; i < HFI_NUM_PPORTS; i++) {
		port = &dd->pport[i];
		for (j = 0; j < HFI_MAX_TC; j++) {
			struct hfi_ptcdata *tc = &port->ptc[i];

			tc->psn_base = vmalloc(HFI_PSN_SIZE);
			if (!tc->psn_base) {
				rc = -ENOMEM;
				goto done;
			}
			tx_psn_base.field.address =
				(u64)tc->psn_base >> PAGE_SHIFT;
			rx_psn_base.field.address =
				(u64)tc->psn_base >> PAGE_SHIFT;
			write_csr(dd, tx_offset + i * 0x20 + j * 8,
				  tx_psn_base.val);
			write_csr(dd, rx_offset + i * 0x20 + j * 8,
				  rx_psn_base.val);
		}
	}
done:
	return rc;
}

static void hfi_psn_uninit(const struct hfi_devdata *dd)
{
	int i, j;
	struct hfi_pportdata *port;
	u32 tx_offset = FXR_TXOTR_PKT_CFG_PSN_BASE_ADDR_P0_TC;
	u32 rx_offset = FXR_RXE2E_CFG_PSN_BASE_ADDR_P0_TC;

	for (i = 0; i < HFI_NUM_PPORTS; i++) {
		port = &dd->pport[i];
		for (j = 0; j < HFI_MAX_TC; j++) {
			struct hfi_ptcdata *tc = &port->ptc[i];

			if (tc->psn_base) {
				vfree(tc->psn_base);
				tc->psn_base = NULL;
				write_csr(dd, tx_offset + i * 0x20 + j * 8, 0);
				write_csr(dd, rx_offset + i * 0x20 + j * 8, 0);
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
	LM_CONFIG_PORT0_t lmp0 = {.val = 0};
	LM_CONFIG_PORT1_t lmp1 = {.val = 0};

	switch (ppd_to_pnum(ppd)) {
	case 1:
		lmp0.val = read_csr(dd, FXR_LM_CONFIG_PORT0);
		lmp0.field.DLID = ppd->lid;
		if (!ppd->lmc) {
			lmp0.field.LMC_ENABLE = 1;
			lmp0.field.LMC_WIDTH = ppd->lmc;
		}
		write_csr(dd, FXR_LM_CONFIG_PORT0, lmp0.val);
		write_csr(dd, FXR_TXOTR_PKT_CFG_SLID_PT0, ppd->lid);
		break;
	case 2:
		lmp1.val = read_csr(dd, FXR_LM_CONFIG_PORT1);
		lmp1.field.DLID = ppd->lid;
		if (!ppd->lmc) {
			lmp1.field.LMC_ENABLE = 1;
			lmp1.field.LMC_WIDTH = ppd->lmc;
		}
		write_csr(dd, FXR_LM_CONFIG_PORT1, lmp1.val);
		write_csr(dd, FXR_TXOTR_PKT_CFG_SLID_PT1, ppd->lid);
		break;
	default:
		return;
	}

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
	/*FXRTODO: HW related code to change MTU */
}

static void hfi_cfg_pkey_check(struct hfi_pportdata *ppd, u8 enable)
{
	struct hfi_devdata *dd = ppd->dd;
	LM_CONFIG_PORT0_t lmp0;
	LM_CONFIG_PORT1_t lmp1;
	TP_CFG_MISC_CTRL_t misc;

	misc.val = read_lm_tp_csr(ppd, FXR_TP_CFG_MISC_CTRL);
	misc.field.disable_pkey_chk = !enable;
	write_lm_tp_csr(ppd, FXR_TP_CFG_MISC_CTRL, misc.val);
	/*
	 * FXRTODO: Writes to TP_CFG_PKEY_CHECK_CTRL are required
	 * for 10B and 8B packets
	 */
	switch (ppd_to_pnum(ppd)) {
	case 1:
		lmp0.val = read_csr(dd, FXR_LM_CONFIG_PORT0);
		lmp0.field.ENABLE_PKEY = enable;
		write_csr(dd, FXR_LM_CONFIG_PORT0, lmp0.val);
		break;
	case 2:
		lmp1.val = read_csr(dd, FXR_LM_CONFIG_PORT1);
		lmp1.field.ENABLE_PKEY = enable;
		write_csr(dd, FXR_LM_CONFIG_PORT1, lmp1.val);
		break;
	default:
		return;
	}
}

/*
 * Set the device/port partition key table. The MAD code
 * will ensure that, at least, the partial management
 * partition key is present in the table.
 */
static void hfi_set_pkey_table(struct hfi_pportdata *ppd)
{
	TP_CFG_PKEY_TABLE_t tx_pkey;
	FPC_CFG_PKEY_TABLE_t rx_pkey;
	int i;

	for (i = 0; i < HFI_MAX_PKEYS; i += 4) {
		tx_pkey.field.entry0 = ppd->pkeys[i];
		tx_pkey.field.entry1 = ppd->pkeys[i + 1];
		tx_pkey.field.entry2 = ppd->pkeys[i + 2];
		tx_pkey.field.entry3 = ppd->pkeys[i + 3];

		rx_pkey.field.entry0 = ppd->pkeys[i];
		rx_pkey.field.entry1 = ppd->pkeys[i + 1];
		rx_pkey.field.entry2 = ppd->pkeys[i + 2];
		rx_pkey.field.entry3 = ppd->pkeys[i + 3];
		write_lm_tp_csr(ppd, FXR_TP_CFG_PKEY_TABLE, tx_pkey.val);
		write_lm_fpc_csr(ppd, FXR_FPC_CFG_PKEY_TABLE, rx_pkey.val);
	}

	/*
	 * FXRTODO: Enabling Pkey check results in packet drop. Need to
	 * debug this. Disable pkey check for now.
	 */
	hfi_cfg_pkey_check(ppd, 0);
}

u8 hfi_porttype(struct hfi_pportdata *ppd)
{
	/* FXRTODO: QSFP logic goes here to decide port type */
	return HFI_PORT_TYPE_STANDARD;
}

static void hfi_set_sc_to_vlt(struct hfi_pportdata *ppd, u8 *t)
{
	u64 reg_val = 0;
	int i, j, sc_num;

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
		write_lm_tp_csr(ppd, FXR_TP_CFG_SC_TO_VLT_MAP +
					i * 8, reg_val);
	}

	/*
	 * FXRTODO: Cached table will not only be
	 * read by get_sma but also by verbs layer post
	 * initialization. So this needs to be protected
	 * by lock. If we use get_port_desc, then all
	 * the tables reported by that call needs to hold
	 * a lock. Design decision pending.
	 */
	  memcpy(ppd->sc_to_vlt, t, OPA_MAX_SCS);
}

static void hfi_set_sc_to_vlnt(struct hfi_pportdata *ppd, u8 *t)
{
	int i;
	u64 *t64 = (u64 *)t;

	/*
	 * 4 registers, 8 entries per register, 5 bit per entry
	 * FXRTODO: This register's bit fields might change in the
	 * next simics release.
	 */
	for (i = 0; i < 4; i++)
		write_lm_cm_csr(ppd, FXR_TP_CFG_CM_SC_TO_VLT_MAP +
							i * 8, *t64++);

	memcpy(ppd->sc_to_vlnt, t, OPA_MAX_SCS);
}

int hfi_get_ib_cfg(struct hfi_pportdata *ppd, int which)
{
	struct hfi_devdata *dd = ppd->dd;
	int val = 0;

	switch (which) {
	case HFI_IB_CFG_OP_VLS:
		val = ppd->vls_operational;
		break;
	default:
		dd_dev_info(dd, "%s: which %d: not implemented\n",
			__func__, which);
		break;
	}

	return val;
}

static void hfi_init_sl_to_mc_tc(struct hfi_pportdata *ppd, u8 *mc, u8 *tc)
{
	struct hfi_devdata *dd = ppd->dd;
	u64 reg[2];
	u32 tx_reg_addr, rx_reg_addr;
	int i, j, k;
	int nregs = ARRAY_SIZE(reg);

	switch (ppd_to_pnum(ppd)) {
	case 1:
		tx_reg_addr = FXR_TXCID_CFG_SL0_TO_TC;
		rx_reg_addr = FXR_RXCID_CFG_SL0_TO_TC;
		break;
	case 2:
		tx_reg_addr = FXR_TXCID_CFG_SL2_TO_TC;
		rx_reg_addr = FXR_RXCID_CFG_SL2_TO_TC;
		break;
	default:
		return;
	}

	/* Read from the TX and update both TX/RX mappings */
	for (i = 0; i < nregs; i++)
		reg[i] = read_csr(dd, tx_reg_addr + i * 8);

	for (i = 0, j = 0; i < nregs; i++) {
		for (k = 0; k < 64; k += 4, j++) {
			if (mc)
				hfi_set_bits(&reg[i], mc[j], HFI_SL_TO_MC_MASK,
						k + HFI_SL_TO_MC_SHIFT);
			if (tc)
				hfi_set_bits(&reg[i], tc[j], HFI_SL_TO_TC_MASK,
						k + HFI_SL_TO_TC_SHIFT);
		}
	}

	/* TX & RX SL -> TC/MC mappings should be identical */
	for (i = 0; i < nregs; i++) {
		write_csr(dd, tx_reg_addr + i * 8, reg[i]);
		write_csr(dd, rx_reg_addr + i * 8, reg[i]);
	}
}

static void hfi_sl_to_sc(struct hfi_pportdata *ppd, u8 *sl_to_sc)
{
	struct hfi_devdata *dd = ppd->dd;
	u64 reg[4];
	u32 reg_addr;
	int i, j, k;
	int nregs = ARRAY_SIZE(reg);

	if (!sl_to_sc) {
		dd_dev_warn(dd, "No sl_to_sc mapping");
		return;
	}

	switch (ppd_to_pnum(ppd)) {
	case 1:
		reg_addr = FXR_LM_CFG_PORT0_SC2SL0;
		break;
	case 2:
		reg_addr = FXR_LM_CFG_PORT1_SC2SL0;
		break;
	default:
		return;
	}

	for (i = 0; i < nregs; i++)
		reg[i] = read_csr(dd, reg_addr + i * 8);

	for (i = 0, j = 0; i < nregs; i++)
		for (k = 0; k < 64; k += 8, j++)
			hfi_set_bits(&reg[i], sl_to_sc[j], HFI_SL_TO_SC_MASK,
				     k + HFI_SL_TO_SC_SHIFT);
	for (i = 0; i < nregs; i++)
		write_csr(dd, reg_addr + i * 8, reg[i]);
}

static void hfi_sc_to_sl(struct hfi_pportdata *ppd)
{
	/* FXRTODO: Register not yet defined in HAS. STL-1703 */
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
		val *= 4096/64;
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
	case HFI_IB_CFG_SL_TO_SC:
		hfi_sl_to_sc(ppd, ppd->sl_to_sc);
		break;
	case HFI_IB_CFG_SC_TO_SL:
		hfi_sc_to_sl(ppd);
		break;
	case HFI_IB_CFG_SC_TO_VLT:
		hfi_set_sc_to_vlt(ppd, data);
		break;
	case HFI_IB_CFG_SC_TO_VLNT:
		hfi_set_sc_to_vlnt(ppd, data);
		break;
	default:
		dd_dev_info(dd, "%s: which %d: not implemented\n",
			__func__, which);
		break;
	}

	return ret;
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
	int ret = 0;

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
	if (ppd->host_link_state == HLS_UP_INIT
			|| ppd->host_link_state == HLS_UP_ARMED
			|| ppd->host_link_state == HLS_UP_ACTIVE)
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
	opa_core_notify_clients(ppd->dd->bus_dev,
				OPA_MTU_CHANGE, ppd->pnum);
	mutex_unlock(&ppd->hls_lock);

	return ret;
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

u8 hfi_ibphys_portstate(struct hfi_pportdata *ppd)
{
	u32 pstate;

	/*
	 * FXRTODO: To be implemented as part of LNI
	 * read from CSR. For now fake it with
	 * equivalent linkstate set earlier
	 */
	switch(ppd->lstate) {
	case IB_PORT_DOWN:
		pstate =  IB_PORTPHYSSTATE_DISABLED;
		break;
	default:
		pstate = IB_PORTPHYSSTATE_LINKUP;
	}

	return pstate;
}

/*
 * Send an idle SMA message.
 *
 * Returns 0 on success, -EINVAL on error
 */
int hfi_send_idle_sma(struct hfi_devdata *dd, u64 message)
{
	/* FXRTODO: To be implemented as part of MNH/FC commands */
	return -EINVAL;
}

void hfi_ack_interrupt(struct hfi_msix_entry *me)
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
	write_csr(dd, FXR_PCIM_INT_CLR + offset * 8,
		  BIT_ULL(me->intr_src & 0x3f));
}

static void hfi_ack_all_interrupts(const struct hfi_devdata *dd)
{
	int i;
	u64 val;

	for (i = 0; i <= PCIM_MAX_INT_CSRS; i++) {
		val = read_csr(dd, FXR_PCIM_INT_STS + i * 8);

		/* Clear the interrupt status read */
		if (val)
			write_csr(dd, FXR_PCIM_INT_CLR + i * 8, val);
	}
}

static irqreturn_t irq_eq_handler(int irq, void *dev_id)
{
	struct hfi_msix_entry *me = dev_id;
	struct hfi_event_queue *eq;

	hfi_ack_interrupt(me);
	/* wake head waiter for each EQ using this IRQ */
	read_lock(&me->irq_wait_lock);
	list_for_each_entry(eq, &me->irq_wait_head, irq_wait_chain) {
		if (eq->isr_cb)
			eq->isr_cb(eq->cookie);
		else
			wake_up_interruptible(&(eq->wq));
	}
	read_unlock(&me->irq_wait_lock);

	return IRQ_HANDLED;
}

/*
 * hfi_pport_uninit - uninitialize per port
 * data structs
 */
void hfi_pport_uninit(struct hfi_devdata *dd)
{
	u8 port;

	for (port = 1; port <= dd->num_pports; port++) {
		struct cc_state *cc_state;
		struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);

		spin_lock(&ppd->cc_state_lock);
		cc_state = hfi_get_cc_state(ppd);
		rcu_assign_pointer(ppd->cc_state, NULL);
		spin_unlock(&ppd->cc_state_lock);

		if (cc_state)
			call_rcu(&cc_state->rcu, hfi_cc_state_reclaim);
	}
}

/*
 * hfi_pport_down - shutdown ports
 */
void hfi_pport_down(struct hfi_devdata *dd)
{
	hfi2_pport_link_uninit(dd);
	hfi_pport_uninit(dd);
}

/*
 * Do HFI chip-specific and PCIe cleanup. Free dd memory.
 * This is called in error cleanup from hfi_pci_dd_init().
 * Some state may be unset, so must use caution and test if
 * cleanup is needed.
 */
void hfi_pci_dd_free(struct hfi_devdata *dd)
{
	int i;

	/*
	 * shutdown ports to notify OPA core clients.
	 * FXRTODO: Check error handling if hfi_pci_dd_init fails early
	 */
	hfi_pport_down(dd);

	if (dd->bus_dev)
		opa_core_unregister_device(dd->bus_dev);

	hfi_e2e_destroy(dd);

	hfi_disable_interrupts(dd);

	/* release system context and any privileged CQs */
	if (dd->priv_ctx.devdata) {
		hfi_e2e_eq_release(&dd->priv_ctx);
		hfi_eq_zero_release(&dd->priv_ctx);
		hfi_cq_unmap(&dd->priv_tx_cq, &dd->priv_rx_cq);
		hfi_ctxt_cleanup(&dd->priv_ctx);
	}
	hfi_cleanup_interrupts(dd);

	hfi_iommu_root_clear_context(dd);

	hfi_psn_uninit(dd);

	/* free host memory for FXR and Portals resources */
	if (dd->cq_head_base)
		free_pages((unsigned long)dd->cq_head_base,
			   get_order(dd->cq_head_size));

	for (i = 0; i < HFI_NUM_BARS; i++)
		if (dd->kregbase[i])
			iounmap((void __iomem *)dd->kregbase[i]);

	idr_destroy(&dd->cq_pair);
	idr_destroy(&dd->ptl_user);
	rcu_barrier(); /* wait for rcu callbacks to complete */

	pci_set_drvdata(dd->pcidev, NULL);
	kfree(dd);
}

static void hfi_port_desc(struct opa_core_device *odev,
				struct opa_pport_desc *pdesc, u8 port_num)
{
	struct hfi_pportdata *ppd = to_hfi_ppd(odev->dd, port_num);
	struct hfi_devdata *dd = odev->dd;
	int i;

	pdesc->pguid = ppd->pguid;
	pdesc->lid = ppd->lid;
	pdesc->num_vls_supported = ppd->vls_supported;
	for (i = 0; i < ppd->vls_supported; i++)
		pdesc->vl_mtu[i] = dd->vl_mtu[i];
	pdesc->vl_mtu[15] = dd->vl_mtu[15];
	pdesc->pkey_tlen = HFI_MAX_PKEYS;
	pdesc->pkeys = ppd->pkeys;
	pdesc->ibmaxmtu = HFI_DEFAULT_MAX_MTU;
	for (i = 0; i < ARRAY_SIZE(ppd->sl_to_sc); i++)
		pdesc->sl_to_sc[i] = ppd->sl_to_sc[i];
	for (i = 0; i < ARRAY_SIZE(ppd->sc_to_sl); i++)
		pdesc->sc_to_sl[i] = ppd->sc_to_sl[i];
	for (i = 0; i < ARRAY_SIZE(ppd->sc_to_vlt); i++)
		pdesc->sc_to_vl[i] = ppd->sc_to_vlt[i];
	pdesc->lstate = ppd->lstate;
}

static void hfi_device_desc(struct opa_core_device *odev,
						struct opa_dev_desc *desc)
{
	struct hfi_devdata *dd = odev->dd;

	memcpy(desc->oui, dd->oui, ARRAY_SIZE(dd->oui));
	desc->num_pports = dd->num_pports;
	desc->nguid = dd->nguid;
	desc->numa_node = dd->node;
	desc->ibdev = dd->ibdev;
}

/* Save the registered IB device and notify OPA core clients */
void hfi_set_ibdev(struct opa_core_device *odev, struct ib_device *ibdev)
{
	struct hfi_devdata *dd = odev->dd;

	dd->ibdev = ibdev;
}

/* Clear the registered IB device and notify OPA core clients */
void hfi_clear_ibdev(struct opa_core_device *odev)
{
	struct hfi_devdata *dd = odev->dd;

	dd->ibdev = NULL;
}

static struct opa_core_ops opa_core_ops = {
	.ctx_assign = hfi_ctxt_attach,
	.ctx_release = hfi_ctxt_cleanup,
	.ctx_reserve = hfi_ctxt_reserve,
	.ctx_unreserve = hfi_ctxt_unreserve,
	.ctx_addr = hfi_ctxt_hw_addr,
	.cq_assign = hfi_cq_assign,
	.cq_update = hfi_cq_update,
	.cq_release = hfi_cq_release,
	.cq_map = hfi_cq_map,
	.cq_unmap = hfi_cq_unmap,
	.ev_assign = hfi_cteq_assign,
	.ev_release = hfi_cteq_release,
	.ev_wait_single = hfi_cteq_wait_single,
	.dlid_assign = hfi_dlid_assign,
	.dlid_release = hfi_dlid_release,
	.e2e_ctrl = hfi_e2e_ctrl,
	.get_device_desc = hfi_device_desc,
	.get_port_desc = hfi_port_desc,
	.get_sma = hfi_get_sma,
	.set_sma = hfi_set_sma,
	.set_ibdev = hfi_set_ibdev,
	.clear_ibdev = hfi_clear_ibdev
};

/*
 * hfi_ptc_init - initialize per traffic class data structs per port
 */
void hfi_ptc_init(struct hfi_pportdata *ppd)
{
	int i;

	for (i = 0; i < HFI_MAX_TC; i++) {
		struct hfi_ptcdata *tc = &ppd->ptc[i];

		ida_init(&tc->e2e_state_cache);
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
static u16 hfi_lcb_to_port_ltp(struct hfi_devdata *dd, u16 lcb_crc)
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
		dd_dev_warn(dd, "Invalid lcb crc. Driver might be in bad state");
	}

	return port_ltp;
}

/*
 * Convert a OPA Port cap mask to a single FC LCB CRC mode
 */
u16 hfi_port_cap_to_lcb(struct hfi_devdata *dd, u16 crc_mask)
{
	u16 crc_val;

	if (crc_mask & HFI_CAP_CRC_14B)
		crc_val = HFI_LCB_CRC_14B;
	else if (crc_mask & HFI_CAP_CRC_48B)
		crc_val = HFI_LCB_CRC_48B;
	else if (crc_mask & HFI_CAP_CRC_12B_16B_PER_LANE)
		crc_val = HFI_LCB_CRC_12B_16B_PER_LANE;
	else
		crc_val = HFI_LCB_CRC_16B;

	return crc_val;
}

void hfi_set_crc_mode(struct hfi_pportdata *ppd, u16 crc_lcb_mode)
{
	write_fzc_csr(ppd, FZC_LCB_CFG_CRC_MODE,
		(u64)crc_lcb_mode << FZC_LCB_CFG_CRC_MODE_TX_VAL_SHIFT);
}

void hfi_init_sc_to_vlt(struct hfi_pportdata *ppd)
{
	u8 vlt[OPA_MAX_SCS];
	int i;

	memset(vlt, 0, sizeof(vlt));
	/*
	 * SC 0-7 -> VL 0-7 (respectively)
	 * SC 15  -> VL 15
	 * otherwise
	 *        -> VL 0
	 */
	for (i = 0; i < HFI_NUM_DATA_VLS; i++)
		vlt[i] = i;
	vlt[15] = 15;
	hfi_set_sc_to_vlt(ppd, vlt);
}

/*
 * hfi_pport_init - initialize per port
 * data structs
 */
int hfi_pport_init(struct hfi_devdata *dd)
{
	struct hfi_pportdata *ppd;
	int i, size;
	u8 port;
	u16 crc_val;

	for (port = 1; port <= dd->num_pports; port++) {
		ppd = to_hfi_ppd(dd, port);
		ppd->dd = dd;
		ppd->pnum = port;
		ppd->pguid = cpu_to_be64(PORT_GUID(dd->nguid, port));
		ppd->lstate = IB_PORT_DOWN;
		mutex_init(&ppd->hls_lock);
		/* initializa Manner Hill - mnh */
#if 0 /* WFR legacy */
		spin_lock_init(&ppd->crk8051_lock);
#endif
		ppd->crk8051_timed_out = 0;
		ppd->host_link_state = HLS_DN_OFFLINE;
		hfi_8051_reset(ppd);

		ppd->cc_max_table_entries = HFI_IB_CC_TABLE_CAP_DEFAULT;

		/*
		 * FXRTODO: The below 4 variables
		 * are set during 8051 interrupt handler
		 * and are read from appropriate 8051
		 * registers. Move this code to 8051 INTR handler.
		 */
		ppd->mgmt_allowed = 0x1;
		/* WFR ref reg: DC_DC8051_STS_REMOTE_NODE_TYPE */
		ppd->neighbor_type = 0;
		/* WFR ref reg: DC_DC8051_STS_REMOTE_FM_SECURITY */
		ppd->neighbor_fm_security = 0;
		/*
		 * WFR ref reg: DC_DC8051_STS_REMOTE_PORT_NO
		 * Current value reflects simics B2B setup :
		 * viper0-port1 <-> viper1-port1
		 * viper0-port2 <-> viper1-port2
		 */
		ppd->neighbor_port_number = port;

		/* FXRTODO: handled as part of SMA idle message. Ref STL-2306 */
		ppd->neighbor_normal = 1;

		/*
		 * Since OPA uses management pkey there is no
		 * need to initialize entry 0 with default application
		 * pkey 0x8001 as mentioned in ib spec v1.3
		 */
		ppd->pkeys[OPA_LIM_MGMT_PKEY_IDX] = OPA_LIM_MGMT_PKEY;

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

		/*
		 * FXRTODO: FXR supports 32Gb/s. This needs to be added to
		 * opa_port_info.h
		 */
		ppd->link_speed_supported = OPA_LINK_SPEED_25G;
		ppd->link_speed_enabled = dd->pport->link_speed_supported;
		ppd->link_speed_active = OPA_LINK_SPEED_25G;

		/*
		 * FXRTODO: These 3 variables are to be initialized during LNI.
		 * Move this to LNI code and initialize appropriately once
		 * the code is implemented.
		 */
		ppd->link_width_active = OPA_LINK_WIDTH_4X;
		ppd->link_width_downgrade_tx_active = ppd->link_width_active;
		ppd->link_width_downgrade_rx_active = ppd->link_width_active;

		ppd->port_crc_mode_enabled = HFI_SUPPORTED_CRCS;
		/* initialize supported LTP CRC mode */
		ppd->port_ltp_crc_mode = hfi_cap_to_port_ltp(HFI_SUPPORTED_CRCS) <<
					HFI_LTP_CRC_SUPPORTED_SHIFT;
		/* initialize enabled LTP CRC mode */
		ppd->port_ltp_crc_mode |= hfi_cap_to_port_ltp(HFI_SUPPORTED_CRCS) <<
					HFI_LTP_CRC_ENABLED_SHIFT;

		/*
		 * FXRTODO: These need to be moved to LNI code.
		 * Each port needs to negotiate on the CRC
		 * that is to be used with the neighboring port.
		 *
		 * crc_val is obtained during LNI process
		 * from the neighbor port.
		 */
		crc_val = HFI_LCB_CRC_14B;
		ppd->port_ltp_crc_mode |= hfi_lcb_to_port_ltp(dd, crc_val) <<
					HFI_LTP_CRC_ACTIVE_SHIFT;
		hfi_set_crc_mode(ppd, crc_val);

		/*
		 * Set the default MTU only for VL 15
		 * For rest of the data VLs, MTU of 0
		 * is valid as per the spec
		 */
		dd->vl_mtu[15] = HFI_MIN_VL_15_MTU;
		ppd->vls_operational = ppd->vls_supported;
		for (i = 0; i < ARRAY_SIZE(ppd->sl_to_sc); i++)
			ppd->sl_to_sc[i] = i;
		for (i = 0; i < ARRAY_SIZE(ppd->sc_to_sl); i++)
			ppd->sc_to_sl[i] = i;
		/*
		 * FXRTODO: FXR Simics model has the tx_sl_to_mc and
		 * tx_sl_to_tc mapping hard coded based on the formulas
		 * below which are being used in the driver as well
		 * till such time that we have a better solution.
		 */
		for (i = 0; i < ARRAY_SIZE(ppd->tx_sl_to_mc); i++)
			ppd->tx_sl_to_mc[i] = (i & 0x4) >> 2;
		for (i = 0; i < ARRAY_SIZE(ppd->tx_sl_to_tc); i++)
			ppd->tx_sl_to_tc[i] = i % 4;
		hfi_init_sl_to_mc_tc(ppd, ppd->tx_sl_to_mc, ppd->tx_sl_to_tc);
		hfi_set_ib_cfg(ppd, HFI_IB_CFG_SL_TO_SC, 0, NULL);

		hfi_init_sc_to_vlt(ppd);
		hfi_ptc_init(ppd);

		size = sizeof(struct cc_state);
		RCU_INIT_POINTER(ppd->cc_state, kzalloc(size, GFP_KERNEL));
		if (!rcu_dereference(ppd->cc_state)) {
			dd_dev_err(dd, "Congestion Control Agent"\
					" disabled for port %d\n", port);
		}
	}

	return hfi2_pport_link_init(dd);
}

/**
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
	unsigned long len;
	resource_size_t addr;
	int i, n_irqs, ret;
	struct opa_core_device_id bus_id;
	struct hfi_ctx *ctx;
	u16 priv_cq_idx;
	struct opa_ctx_assign ctx_assign = {0};

	dd = hfi_alloc_devdata(pdev);
	if (IS_ERR(dd))
		return dd;

	/*
	 * Do remaining PCIe setup and save PCIe values in dd.
	 * On return, we have the chip mapped.
	 */

	dd->pcidev = pdev;
	pci_set_drvdata(pdev, dd);

	/* For Portals PID and CQ assignments */
	idr_init(&dd->ptl_user);
	spin_lock_init(&dd->ptl_lock);
	idr_init(&dd->cq_pair);
	spin_lock_init(&dd->cq_lock);
	spin_lock_init(&dd->priv_tx_cq_lock);
	spin_lock_init(&dd->priv_rx_cq_lock);
	mutex_init(&dd->e2e_lock);

	for (i = 0; i < HFI_NUM_BARS; i++) {
		addr = pci_resource_start(pdev, i);
		len = pci_resource_len(pdev, i);
		pr_debug("BAR[%d] @ start 0x%lx len %lu\n", i, (long)addr, len);

		dd->pcibar[i] = addr;
		dd->kregbase[i] = ioremap_nocache(addr, len);
		if (!dd->kregbase[i]) {
			ret = -ENOMEM;
			goto err_post_alloc;
		}
		dd->kregend[i] = dd->kregbase[i] + len;
	}
	/* FXR resources are on BAR0 (used for io_remap, etc.) */
	dd->physaddr = dd->pcibar[0];

	ret = hfi_psn_init(dd);
	if (ret)
		goto err_post_alloc;

	/* Ensure CSRs are sane, we can't trust they haven't been manipulated */
	init_csrs(dd);

	/* set IOMMU tables for w/PASID translations */
	ret = hfi_iommu_root_set_context(dd);
	if (ret)
		goto err_post_alloc;

	/* enable MSI-X */
	ret = hfi_setup_interrupts(dd, HFI_NUM_INTERRUPTS, 0);
	if (ret)
		goto err_post_alloc;
	hfi_ack_all_interrupts(dd);

	/* per port init */
	dd->num_pports = HFI_NUM_PPORTS;
	ret = hfi_pport_init(dd);
	if (ret)
		goto err_post_alloc;

	dd->oui[0] = be64_to_cpu(dd->nguid) >> 56 & 0xFF;
	dd->oui[1] = be64_to_cpu(dd->nguid) >> 48 & 0xFF;
	dd->oui[2] = be64_to_cpu(dd->nguid) >> 40 & 0xFF;

	/* CQ head (read) indices - 16 KB */
	dd->cq_head_size = (HFI_CQ_COUNT * HFI_CQ_HEAD_OFFSET);
	dd->cq_head_base = (void *)__get_free_pages(GFP_KERNEL | __GFP_ZERO,
					get_order(dd->cq_head_size));
	if (!dd->cq_head_base) {
		ret = -ENOMEM;
		goto err_post_alloc;
	}
	/* now configure CQ head addresses */
	for (i = 0; i < HFI_CQ_COUNT; i++)
		hfi_cq_head_config(dd, i, dd->cq_head_base);

	/* TX and RX command queues - fast path access */
	dd->cq_tx_base = (void *)dd->physaddr + FXR_TX_CQ_ENTRY;
	dd->cq_rx_base = (void *)dd->physaddr + FXR_RX_CQ_ENTRY;

	ctx = &dd->priv_ctx;
	HFI_CTX_INIT(ctx, dd, &opa_core_ops);
	/* configure system PID/PASID needed by privileged CQs */
	ctx_assign.pid = HFI_PID_SYSTEM;
	ctx_assign.le_me_count = 0;
	ret = hfi_ctxt_attach(ctx, &ctx_assign);
	if (ret)
		goto err_post_alloc;

	/* assign one CQ for privileged commands (DLID, EQ_DESC_WRITE) */
	ret = hfi_cq_assign_privileged(ctx, &priv_cq_idx);
	if (ret)
		goto err_post_alloc;
	ret = hfi_cq_map(ctx, priv_cq_idx, &dd->priv_tx_cq, &dd->priv_rx_cq);
	if (ret)
		goto err_post_alloc;

	/* configure IRQs for EQ groups */
	n_irqs = min_t(int, dd->num_msix_entries, HFI_NUM_EQ_INTERRUPTS);
	for (i = 0; i < n_irqs; i++) {
		struct hfi_msix_entry *me = &dd->msix_entries[i];

		BUG_ON(me->arg != NULL);
		INIT_LIST_HEAD(&me->irq_wait_head);
		rwlock_init(&me->irq_wait_lock);
		me->dd = dd;
		me->intr_src = i;

		dd_dev_dbg(dd, "request for IRQ %d:%d\n", i, me->msix.vector);
		ret = request_irq(me->msix.vector, irq_eq_handler, 0,
				  "hfi_irq_eq", me);
		if (ret) {
			dd_dev_err(dd, "IRQ[%d] request failed %d\n", i, ret);
			/* IRQ cleanup done in hfi_pci_dd_free() */
			goto err_post_alloc;
		}
		/* mark as in use */
		me->arg = me;
	}
	dd->num_eq_irqs = i;
	atomic_set(&dd->msix_eq_next, 0);
	/* TODO - remove or change to debug later */
	dd_dev_info(dd, "%d IRQs assigned to EQs\n", i);

	/* assign one EQ for privileged events */
	ret = hfi_eq_zero_assign_privileged(ctx);
	if (ret)
		goto err_post_alloc;

	/* assign EQs for initiator E2E initiator events */
	ret = hfi_e2e_eq_assign(ctx);
	if (ret)
		goto err_post_alloc;

	bus_id.vendor = ent->vendor;
	bus_id.device = ent->device;
	bus_id.revision = (u32)pdev->revision;
	/* bus agent can be probed immediately, no writing dd->bus_dev after this */
	dd->bus_dev = opa_core_register_device(&pdev->dev, &bus_id, dd, &opa_core_ops);
	/* All the unit management is handled by opa_core */
	dd->unit = dd->bus_dev->index;
	dd->bus_dev->kregbase = dd->kregbase[0];
	dd->bus_dev->kregend = dd->kregend[0];

	return dd;

err_post_alloc:
	dd_dev_err(dd, "%s %d FAILED ret %d\n", __func__, __LINE__, ret);
	hfi_pci_dd_free(dd);

	return ERR_PTR(ret);
}

void hfi_eq_cache_invalidate(struct hfi_devdata *dd, u16 ptl_pid)
{
	RXET_CFG_EQ_DESC_CACHE_ACCESS_CTL_t eq_cache_access = {.val = 0};
	union eq_cache_addr eq_cache_tag;

	eq_cache_access.field.cmd = FXR_CACHE_CMD_INVALIDATE;
	eq_cache_tag.val = 0;
	eq_cache_tag.pid = ptl_pid;
	eq_cache_access.field.address = eq_cache_tag.val;
	eq_cache_tag.val = -1;
	eq_cache_tag.pid = 0;
	eq_cache_access.field.mask_address = eq_cache_tag.val;
	write_csr(dd, FXR_RXET_CFG_EQ_DESC_CACHE_ACCESS_CTL,
		  eq_cache_access.val);

	/* TODO - above incomplete, deferred processing to wait for .ack bit */
	mdelay(10);
}

void hfi_pcb_reset(struct hfi_devdata *dd, u16 ptl_pid)
{
	RX_HIARB_CFG_PCB_LOW_t pcb_low = {.val = 0};
	RX_HIARB_CFG_PCB_HIGH_t pcb_high = {.val = 0};
	RXHP_CFG_PTE_CACHE_ACCESS_CTL_t pte_cache_access = {.val = 0};
	RXET_CFG_TRIG_OP_CACHE_ACCESS_CTL_t trig_op_cache_access = {.val = 0};
	/*
	 * FXRTODO: Use trig op for me_le/uh_cache_access for now since
	 * HAS does not have these definitions yet
	 */
	RXET_CFG_TRIG_OP_CACHE_ACCESS_CTL_t me_le_uh_cache_access = {.val = 0};
	union pte_cache_addr pte_cache_tag;
	union trig_op_cache_addr trig_op_cache_tag;
	/* Fake Simics structure for LE/ME/UH cache invalidation */
	union me_le_uh_cache_addr {
		struct {
			uint64_t pid	: 12; /* [11:0] Target Process Id */
			uint64_t me_le	: 1; /* [12:12] ME/LE */
			uint64_t handle	: 16; /* [28:13] Handle */
		};
		uint64_t val;
	} me_le_uh_cache_tag;

	/* write PCB_LOW first to clear valid bit */
	write_csr(dd, FXR_RX_HIARB_CFG_PCB_LOW + (ptl_pid * 8), pcb_low.val);
	write_csr(dd, FXR_RX_HIARB_CFG_PCB_HIGH + (ptl_pid * 8), pcb_high.val);

	/* invalidate cached host memory in HFI for Portals Tables by PID */
	pte_cache_access.field.cmd = FXR_CACHE_CMD_INVALIDATE;
	pte_cache_tag.val = 0;
	pte_cache_tag.tpid = ptl_pid;
	pte_cache_access.field.address = pte_cache_tag.val;
	pte_cache_tag.val = -1;
	pte_cache_tag.tpid = 0;
	pte_cache_access.field.mask_address = pte_cache_tag.val;
	write_csr(dd, FXR_RXHP_CFG_PTE_CACHE_ACCESS_CTL, pte_cache_access.val);

	/* invalidate cached host memory in HFI for EQ Descs by PID */
	hfi_eq_cache_invalidate(dd, ptl_pid);

	/* invalidate cached host memory in HFI for Triggered Ops by PID */
	trig_op_cache_access.field.cmd = FXR_CACHE_CMD_INVALIDATE;
	trig_op_cache_tag.val = 0;
	trig_op_cache_tag.pid = ptl_pid;
	trig_op_cache_access.field.address = trig_op_cache_tag.val;
	trig_op_cache_tag.val = -1;
	trig_op_cache_tag.pid = 0;
	trig_op_cache_access.field.mask_address = trig_op_cache_tag.val;
	write_csr(dd, FXR_RXET_CFG_TRIG_OP_CACHE_ACCESS_CTL,
		  trig_op_cache_access.val);

	/* invalidate cached host memory in HFI for ME/LE/UH */
	me_le_uh_cache_access.field.cmd = FXR_CACHE_CMD_INVALIDATE;
	me_le_uh_cache_tag.val = 0;
	me_le_uh_cache_tag.pid = ptl_pid;
	me_le_uh_cache_access.field.address = me_le_uh_cache_tag.val;
	me_le_uh_cache_tag.val = -1;
	me_le_uh_cache_tag.pid = 0;
	me_le_uh_cache_access.field.mask_address = me_le_uh_cache_tag.val;
	write_csr(dd, 0x1408000, me_le_uh_cache_access.val);

	/* TODO - above incomplete, deferred processing to wait for .ack bit */
	mdelay(10);

	/* TODO - write fake simics CSR to flush mini-TLB (AT interface TBD) */
	write_csr(dd, FXR_RX_HIARB_CSRS + 0x20000, 1);
}

void hfi_pcb_write(struct hfi_ctx *ctx, u16 ptl_pid)
{
	struct hfi_devdata *dd = ctx->devdata;
	RX_HIARB_CFG_PCB_LOW_t pcb_low = {.val = 0};
	RX_HIARB_CFG_PCB_HIGH_t pcb_high = {.val = 0};
	u64 psb_addr;

	psb_addr = (u64)ctx->ptl_state_base;

	/* write PCB in FXR */
	pcb_low.field.valid = 1;
	pcb_low.field.portals_state_base = psb_addr >> PAGE_SHIFT;
	pcb_high.field.triggered_op_size = (ctx->trig_op_size >> PAGE_SHIFT);
	pcb_high.field.unexpected_size = (ctx->unexpected_size >> PAGE_SHIFT);
	pcb_high.field.le_me_size = (ctx->le_me_size >> PAGE_SHIFT);

	write_csr(dd, FXR_RX_HIARB_CFG_PCB_HIGH + (ptl_pid * 8), pcb_high.val);
	write_csr(dd, FXR_RX_HIARB_CFG_PCB_LOW + (ptl_pid * 8), pcb_low.val);
}

/* Write CSR address for CQ head index, maintained by FXR */
static void hfi_cq_head_config(struct hfi_devdata *dd, u16 cq_idx,
			       void *head_base)
{
	u32 head_offset;
	RXCID_CFG_HEAD_UPDATE_ADDR_t cq_head = {.val = 0};
	TXCIC_CFG_DRAIN_RESET_t tx_cq_reset = {.val = 0};
	RXCIC_CFG_CQ_DRAIN_RESET_t rx_cq_reset = {.val = 0};

	head_offset = FXR_RXCID_CFG_HEAD_UPDATE_ADDR + (cq_idx * 8);

	/* disable CQ head before reset, as no assigned PASID */
	write_csr(dd, head_offset, 0);

	/* reset CQ state, as CQ head starts at 0 */
	tx_cq_reset.field.drain_cq = cq_idx;
	tx_cq_reset.field.reset = 1;
	rx_cq_reset.field.drain_cq = cq_idx;
	rx_cq_reset.field.reset = 1;

	write_csr(dd, FXR_TXCIC_CFG_DRAIN_RESET, tx_cq_reset.val);
	write_csr(dd, FXR_RXCIC_CFG_CQ_DRAIN_RESET, rx_cq_reset.val);
#if 1 /* TODO: should be __SIMICS__ instead of 1 */
	/* 'reset to zero' is not visible for a few cycles in Simics */
	mdelay(1);
#endif

	/* set CQ head, should be set after CQ reset */
	cq_head.field.valid = 1;
	cq_head.field.hd_ptr_host_addr_56_4 =
			(u64)HFI_CQ_HEAD_ADDR(head_base, cq_idx) >> 4;
	write_csr(dd, head_offset, cq_head.val);
}

/*
 * Disable pair of CQs and reset flow control.
 * Reset of flow control is needed since CQ might have only had
 * a partial command or slot written by errant user.
 */
void hfi_cq_disable(struct hfi_devdata *dd, u16 cq_idx)
{
	TXCIC_CFG_DRAIN_RESET_t tx_cq_reset = {.val = 0};
	RXCIC_CFG_CQ_DRAIN_RESET_t rx_cq_reset = {.val = 0};

	/* reset CQ state, as CQ head starts at 0 */
	tx_cq_reset.field.drain_cq = cq_idx;
	tx_cq_reset.field.reset = 1;
	rx_cq_reset.field.drain_cq = cq_idx;
	rx_cq_reset.field.reset = 1;

	write_csr(dd, FXR_TXCIC_CFG_DRAIN_RESET, tx_cq_reset.val);
	write_csr(dd, FXR_RXCIC_CFG_CQ_DRAIN_RESET, rx_cq_reset.val);
#if 1 /* TODO: should be __SIMICS__ instead of 1 */
	/* 'reset to zero' is not visible for a few cycles in Simics */
	mdelay(1);
#endif
}

void hfi_cq_config_tuples(struct hfi_ctx *ctx, u16 cq_idx,
			  struct hfi_auth_tuple *auth_table)
{
	struct hfi_devdata *dd = ctx->devdata;
	int i;
	u32 offset;
	TXCID_CFG_AUTHENTICATION_CSR_t cq_auth = {.val = 0};

	/* write AUTH tuples */
	offset = FXR_TXCID_CFG_AUTHENTICATION_CSR + (cq_idx * HFI_NUM_AUTH_TUPLES * 8);
	for (i = 0; i < HFI_NUM_AUTH_TUPLES; i++) {
		if (auth_table) {
			cq_auth.field.USER_ID = auth_table[i].uid;
			cq_auth.field.SRANK = auth_table[i].srank;
		} else {
			cq_auth.field.USER_ID = ctx->ptl_uid;
			cq_auth.field.SRANK = 0;
		}
		write_csr(dd, offset, cq_auth.val);
		offset += 8;
	}
}

/*
  update DLID relocation table
  assume granularity = 1 for now
*/
int
hfi_update_dlid_relocation_table(struct hfi_ctx *ctx,
				 struct hfi_dlid_assign_args *dlid_assign)
{
	struct hfi_devdata *dd = ctx->devdata;
	TXCID_CFG_DLID_RT_t *p =
		(TXCID_CFG_DLID_RT_t *)dlid_assign->dlid_entries_ptr;
	u32 offset;

	if (dlid_assign->dlid_base >= DLID_TABLE_SIZE)
		return -EINVAL;
	if (dlid_assign->dlid_base + dlid_assign->count > DLID_TABLE_SIZE)
		return -EINVAL;

	for (offset = dlid_assign->dlid_base * sizeof(*p);
		offset < (dlid_assign->dlid_base + dlid_assign->count) * sizeof(*p);
		p++, offset += sizeof(*p)) {
		/* They must be 0 when granularity = 1 */
		if (p->field.RPLC_BLK_SIZE != 0)
			return -EINVAL;
		if (p->field.RPLC_MATCH != 0)
			return -EINVAL;

		write_csr(dd, FXR_TXCID_CFG_DLID_RT + offset, p->val);
	}
	return 0;
}

/*
  reset DLID relocation table
  assume granularity = 1 for now
*/
int
hfi_reset_dlid_relocation_table(struct hfi_ctx *ctx, u32 dlid_base, u32 count)
{
	struct hfi_devdata *dd = ctx->devdata;
	u32 offset;

	if (dlid_base >= DLID_TABLE_SIZE)
		return -EINVAL;
	if (dlid_base + count > DLID_TABLE_SIZE)
		return -EINVAL;

	for (offset = dlid_base * sizeof(TXCID_CFG_DLID_RT_t);
		offset < (dlid_base + count) * sizeof(TXCID_CFG_DLID_RT_t);
		offset += sizeof(TXCID_CFG_DLID_RT_t))
		write_csr(dd, FXR_TXCID_CFG_DLID_RT + offset, 0);
	return 0;
}

/*
 * Write CSRs to configure a TX and RX Command Queue.
 * Authentication Tuple UIDs have been pre-validated by caller.
 */
void hfi_cq_config(struct hfi_ctx *ctx, u16 cq_idx,
		   struct hfi_auth_tuple *auth_table, bool user_priv)
{
	struct hfi_devdata *dd = ctx->devdata;
	u32 offset;
	TXCID_CFG_CSR_t tx_cq_config = {.val = 0};
	RXCID_CFG_CNTRL_t rx_cq_config = {.val = 0};

	hfi_cq_config_tuples(ctx, cq_idx, auth_table);

	/* set TX CQ config, enable */
	tx_cq_config.field.enable = 1;
	tx_cq_config.field.pid = ctx->pid;
	tx_cq_config.field.priv_level = user_priv;
	tx_cq_config.field.dlid_base = ctx->dlid_base;
	tx_cq_config.field.phys_dlid = ctx->allow_phys_dlid;
	tx_cq_config.field.sl_enable = ctx->sl_mask;
	offset = FXR_TXCID_CFG_CSR + (cq_idx * 8);
	write_csr(dd, offset, tx_cq_config.val);

	/* set RX CQ config, enable */
	rx_cq_config.field.enable = 1;
	rx_cq_config.field.pid = ctx->pid;
	tx_cq_config.field.priv_level = user_priv;
	offset = FXR_RXCID_CFG_CNTRL + (cq_idx * 8);
	write_csr(dd, offset, rx_cq_config.val);
}

int hfi_ctxt_hw_addr(struct hfi_ctx *ctx, int type, u16 ctxt, void **addr, ssize_t *len)
{
	struct hfi_devdata *dd;
	void *psb_base = NULL;
	struct hfi_ctx *cq_ctx;
	unsigned long flags;
	int ret = 0;

	BUG_ON(!ctx || !ctx->devdata);
	dd = ctx->devdata;

	/*
	 * Validate passed in ctxt value.
	 * For CQs, verify ownership of this CQ before allowing remapping.
	 * Unused for per-PID state, we are already restricted to
	 * host memory associated with the PID (ctx->ptl_state_base).
	 */
	switch (type) {
	case TOK_CQ_TX:
	case TOK_CQ_RX:
	case TOK_CQ_HEAD:
		spin_lock_irqsave(&dd->cq_lock, flags);
		cq_ctx = idr_find(&dd->cq_pair, ctxt);
		spin_unlock_irqrestore(&dd->cq_lock, flags);
		if (cq_ctx != ctx)
			return -EINVAL;
		break;
	default:
		psb_base = ctx->ptl_state_base;
		BUG_ON(psb_base == NULL);
		break;
	}

	switch (type) {
	case TOK_CQ_TX:
		/* mmio - RW */
		*addr = HFI_CQ_TX_IDX_ADDR(dd->cq_tx_base, ctxt);
		*len = HFI_CQ_TX_SIZE;
		break;
	case TOK_CQ_RX:
		/* mmio - RW */
		*addr = HFI_CQ_RX_IDX_ADDR(dd->cq_rx_base, ctxt);
		*len = PAGE_ALIGN(HFI_CQ_RX_SIZE);
		break;
	case TOK_CQ_HEAD:
		/* kmalloc - RO */
		*addr = HFI_CQ_HEAD_ADDR(dd->cq_head_base, ctxt);
		*len = PAGE_SIZE;
		break;
	case TOK_CONTROL_BLOCK:
		/* kmalloc - RO (debug) */
		/* TODO - this was requested but there are security concerns */
		ret = -ENOSYS;
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

void hfi_ctxt_set_bypass(struct hfi_ctx *ctx)
{
	struct hfi_devdata *dd = ctx->devdata;
	u8 rx_ctx;

	/* Get the 'Gen1 context' number from FXR PID */
	rx_ctx = ctx->pid - HFI_PID_BYPASS_BASE;

	spin_lock(&dd->ptl_lock);
	if (ctx->mode & HFI_CTX_MODE_BYPASS_10B) {
		/* TODO - don't unconditionally claim bypass_pid */
		write_csr(dd, FXR_RXHP_CFG_NPTL_BYPASS, rx_ctx);
		dd->bypass_pid = ctx->pid;
	}

	if (ctx->mode & HFI_CTX_MODE_BYPASS_9B) {
		u64 val;
		u32 csr_idx, off;
		int i, count;

		/* TODO - revisit when adding multiple receive context support */

		if (ctx->qpn_map_idx == 0) {
			write_csr(dd, FXR_RXHP_CFG_NPTL_VL15, rx_ctx);
			dd->vl15_pid = ctx->pid;
		}

		/* for each QPN_MAP entry, read appropriate CSR and update */
		count = min_t(int, ctx->qpn_map_count, OPA_QPN_MAP_MAX);
		for (i = ctx->qpn_map_idx; i < count; i++) {
			off = (i / 8) * 8;
			val = read_csr(dd, FXR_RXHP_CFG_NPTL_QP_MAP_TABLE + off);
			/* set just the 8-bit sub-field in this CSR for this QPN */
			csr_idx = i % 8;
			val &= ~(0xFFuLL << (csr_idx * 8));
			val |= rx_ctx << (csr_idx * 8);
			write_csr(dd, FXR_RXHP_CFG_NPTL_QP_MAP_TABLE + off, val);
		}
	}
	spin_unlock(&dd->ptl_lock);
}
