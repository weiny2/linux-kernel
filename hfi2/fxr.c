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
#include <linux/sched.h>
#include <linux/utsname.h>
#if 1 /* TODO: should be __SIMICS__ instead of 1 */
#include <linux/delay.h>
#endif
#include <linux/bitops.h>
#include <linux/vmalloc.h>
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
#include <rdma/fxr/fxr_lm_cm_csrs.h>
#include <rdma/fxr/fxr_lm_fpc_csrs.h>
#include <rdma/fxr/fxr_linkmux_tp_defs.h>
#include <rdma/fxr/mnh_8051_defs.h>
#include <rdma/fxr/fxr_linkmux_fpc_defs.h>
#include <rdma/fxr/fxr_fc_defs.h>
#include <rdma/fxr/fxr_tx_otr_pkt_top_csrs_defs.h>
#include <rdma/fxr/fxr_tx_otr_pkt_top_csrs.h>
#include <rdma/fxr/fxr_pcim_defs.h>
#include "opa_hfi.h"
#include <rdma/opa_core_ib.h>
#include "link.h"
#include "firmware.h"
#include "verbs/verbs.h"

/* TODO - should come from HW headers */
#define FXR_CACHE_CMD_INVALIDATE 0x8

/* TODO: delete all module parameters when possible */
bool force_loopback;
module_param(force_loopback, bool, S_IRUGO);
MODULE_PARM_DESC(force_loopback, "Force FXR into loopback");

/* FXRTODO: Remove this once we have opafm working with B2B setup */
bool opafm_disable;
module_param_named(opafm_disable, opafm_disable, bool, S_IRUGO);
MODULE_PARM_DESC(opafm_disable, "0 - driver needs opafm to work, \
		 1 - driver works without opafm");

/* FXRTODO: Remove this once MNH is available on all Pre-Si setups */
bool no_mnh;
module_param_named(no_mnh, no_mnh, bool, S_IRUGO);
MODULE_PARM_DESC(mnh_avail, "Set to true if MNH is not available");

/* FXRTODO: Remove this once PE FW is always available */
bool no_pe_fw;
module_param_named(no_pe_fw, no_pe_fw, bool, S_IRUGO);
MODULE_PARM_DESC(mnh_avail, "Set to true if PE FW is not available");

/* FXRTODO: Remove this once Silicon is stable */
bool zebu;
module_param_named(zebu, zebu, bool, S_IRUGO);
MODULE_PARM_DESC(mnh_avail, "Set to true if running on ZEBU");

static void hfi_cq_head_config(struct hfi_devdata *dd, u16 cq_idx,
			       void *head_base);

static const char *hfi2_class_name = "hfi2";

const char *hfi_class_name(void)
{
	return hfi2_class_name;
}

int neigh_is_hfi(struct hfi_pportdata *ppd)
{
	return (ppd->neighbor_type & OPA_PI_MASK_NEIGH_NODE_TYPE) == 0;
}

static void write_lm_fpc_csr(const struct hfi_pportdata *ppd,
			     u32 offset, u64 value)
{
	if (ppd->pnum == 1)
		offset += FXR_LM_FPC0_CSRS;
	else if (ppd->pnum == 2)
		offset += FXR_LM_FPC1_CSRS;
	else
		ppd_dev_warn(ppd, "invalid port");

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
		ppd_dev_warn(ppd, "invalid port");

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
		ppd_dev_warn(ppd, "invalid port");

	write_csr(ppd->dd, offset, value);
}

static u64 read_lm_fpc_csr(const struct hfi_pportdata *ppd,
			     u32 offset)
{
	if (ppd->pnum == 1)
		offset += FXR_LM_FPC0_CSRS;
	else if (ppd->pnum == 2)
		offset += FXR_LM_FPC1_CSRS;
	else
		ppd_dev_warn(ppd, "invalid port");

	return read_csr(ppd->dd, offset);
}

static u64 read_lm_tp_csr(const struct hfi_pportdata *ppd,
			  u32 offset)
{
	if (ppd->pnum == 1)
		offset += FXR_LM_TP0_CSRS;
	else if (ppd->pnum == 2)
		offset += FXR_LM_TP1_CSRS;
	else
		ppd_dev_warn(ppd, "invalid ppd->pnum");

	return read_csr(ppd->dd, offset);
}

static u64 read_lm_cm_csr(const struct hfi_pportdata *ppd,
			  u32 offset)
{
	if (ppd->pnum == 1)
		offset += FXR_LM_CM0_CSRS;
	else if (ppd->pnum == 2)
		offset += FXR_LM_CM1_CSRS;
	else
		ppd_dev_warn(ppd, "invalid ppd->pnum");

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

static void hfi_init_tx_otr_mtu(const struct hfi_devdata *dd, u16 mtu)
{
	int i;
	u64 reg = 0;
	u8 mtu_id = opa_mtu_to_id(mtu);

	if (mtu_id == INVALID_MTU_ENC) {
		dd_dev_warn(dd, "invalid mtu %d", mtu);
		return;
	}

	for (i = 0; i < 8; i++)
		reg |= (mtu_id << (i * 4));
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
	hfi_init_tx_otr_mtu(dd, 4096);
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

static void hfi_read_guid(struct hfi_devdata *dd)
{
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, 1);
	int port;
	u64 base_guid;

#define NODE_GUID              0x11750101000000UL
	/*
	 * Read from one of the port's 8051 register, since node
	 * guid is common between the two ports.
	 */
	if (no_mnh)
		base_guid = NODE_GUID;
	else
		base_guid = read_8051_csr(ppd, CRK_CRK8051_CFG_LOCAL_GUID);
	dd->nguid =  cpu_to_be64(base_guid);

	for (port = 1; port <= dd->num_pports; port++) {
		ppd = to_hfi_ppd(dd, port);
		ppd->pguid = cpu_to_be64(PORT_GUID(dd->nguid, port));
		ppd_dev_info(ppd, "PORT GUID %llx",
			     (unsigned long long)ppd->pguid);
	}

	dd_dev_info(dd, "NODE GUID %llx",
		    (unsigned long long)be64_to_cpu(dd->nguid));
}

static void init_csrs(struct hfi_devdata *dd)
{
	RXHP_CFG_NPTL_CTL_t nptl_ctl = {.val = 0};
	RXHP_CFG_NPTL_BTH_QP_t kdeth_qp = {.val = 0};
	LM_CONFIG_PORT0_t lmp0 = {.val = 0};
	LM_CONFIG_PORT1_t lmp1 = {.val = 0};
	txotr_pkt_cfg_timeout_t txotr_timeout = {.val = 0};
	LM_CONFIG_t lm_config = {.val = 0};

	/* enable non-portals */
	nptl_ctl.field.RcvQPMapEnable = 1;
	nptl_ctl.field.RcvRsmEnable = 1;
	nptl_ctl.field.RcvBypassEnable = 1;
	write_csr(dd, FXR_RXHP_CFG_NPTL_CTL, nptl_ctl.val);
	kdeth_qp.field.KDETH_QP = (OPA_QPN_KDETH_BASE >> 16);
	write_csr(dd, FXR_RXHP_CFG_NPTL_BTH_QP, kdeth_qp.val);

	if (force_loopback) {
		lm_config.field.FORCE_LOOPBACK = force_loopback;
		write_csr(dd, FXR_LM_CONFIG, lm_config.val);
		/* Turn on quick linkup by default for loopback */
		quick_linkup = true;
	}

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
	TP_CFG_VL_MTU_t vlmtu;

	/*FXRTODO: Set vl*_tx_mtu from values suppled by fabric manager */
	vlmtu.val = read_lm_tp_csr(ppd, FXR_TP_CFG_VL_MTU);
	vlmtu.field.vl0_tx_mtu = 0x7;
	vlmtu.field.vl1_tx_mtu = 0x7;
	vlmtu.field.vl2_tx_mtu = 0x7;
	vlmtu.field.vl3_tx_mtu = 0x7;
	vlmtu.field.vl4_tx_mtu = 0x7;
	vlmtu.field.vl5_tx_mtu = 0x7;
	vlmtu.field.vl6_tx_mtu = 0x7;
	vlmtu.field.vl7_tx_mtu = 0x7;
	vlmtu.field.vl8_tx_mtu = 0x7;
	write_lm_tp_csr(ppd, FXR_TP_CFG_VL_MTU, vlmtu.val);
}

void hfi_cfg_out_pkey_check(struct hfi_pportdata *ppd, u8 enable)
{
	TP_CFG_MISC_CTRL_t misc;

	misc.val = read_lm_tp_csr(ppd, FXR_TP_CFG_MISC_CTRL);
	misc.field.disable_pkey_chk = !enable;
	write_lm_tp_csr(ppd, FXR_TP_CFG_MISC_CTRL, misc.val);
}

void hfi_cfg_in_pkey_check(struct hfi_pportdata *ppd, u8 enable)
{
	struct hfi_devdata *dd = ppd->dd;
	LM_CONFIG_PORT0_t lmp0;
	LM_CONFIG_PORT1_t lmp1;

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

void hfi_set_implicit_pkeys(struct hfi_pportdata *ppd,
			    u16 *pkey_8b, u16 *pkey_10b)
{
	TP_CFG_PKEY_CHECK_CTRL_t reg;
	struct hfi_devdata *dd = ppd->dd;
	LM_CONFIG_PORT0_t lmp0;
	LM_CONFIG_PORT1_t lmp1;

	/* Egress */
	reg.val = read_lm_tp_csr(ppd, FXR_TP_CFG_PKEY_CHECK_CTRL);
	if (pkey_8b)
		reg.field.pkey_8b = *pkey_8b;
	if (pkey_10b)
		reg.field.pkey_10b = *pkey_10b;
	write_lm_tp_csr(ppd, FXR_TP_CFG_PKEY_CHECK_CTRL, reg.val);

	/* Ingress */
	switch (ppd_to_pnum(ppd)) {
	case 1:
		lmp0.val = read_csr(dd, FXR_LM_CONFIG_PORT0);
		if (pkey_8b)
			lmp0.field.IMPLICIT_PKEY_8B = *pkey_8b;
		if (pkey_10b)
			lmp0.field.IMPLICIT_PKEY_10B = *pkey_10b;
		write_csr(dd, FXR_LM_CONFIG_PORT0, lmp0.val);
		break;
	case 2:
		lmp1.val = read_csr(dd, FXR_LM_CONFIG_PORT1);
		if (pkey_8b)
			lmp1.field.IMPLICIT_PKEY_8B = *pkey_8b;
		if (pkey_10b)
			lmp1.field.IMPLICIT_PKEY_10B = *pkey_10b;
		write_csr(dd, FXR_LM_CONFIG_PORT1, lmp1.val);
		break;
	default:
		return;
	}
}

static void hfi_cfg_pkey_check(struct hfi_pportdata *ppd, u8 enable)
{
	hfi_cfg_out_pkey_check(ppd, enable);
	hfi_cfg_in_pkey_check(ppd, enable);
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
	int i, j;
	u16 pkey_8b, pkey_10b;

	for (i = 0, j = 0; i < HFI_MAX_PKEYS; i += 4, j++) {
		tx_pkey.field.entry0 = ppd->pkeys[i];
		tx_pkey.field.entry1 = ppd->pkeys[i + 1];
		tx_pkey.field.entry2 = ppd->pkeys[i + 2];
		tx_pkey.field.entry3 = ppd->pkeys[i + 3];

		rx_pkey.field.entry0 = ppd->pkeys[i];
		rx_pkey.field.entry1 = ppd->pkeys[i + 1];
		rx_pkey.field.entry2 = ppd->pkeys[i + 2];
		rx_pkey.field.entry3 = ppd->pkeys[i + 3];
		write_lm_tp_csr(ppd, FXR_TP_CFG_PKEY_TABLE + 0x08 * j,
				tx_pkey.val);
		write_lm_fpc_csr(ppd, FXR_FPC_CFG_PKEY_TABLE + 0x08 * j,
				 rx_pkey.val);
	}

	/*
	 * FXRTODO: 8B and 10B implicit pkeys are to obtained from FM.
	 * Until that is implemented use a pkey of 0x8002 for 8B
	 * which is currently only used by portals traffic.
	 * Correspondingly opafm.xml will have 0x8002
	 * programmed.
	 *
	 * For 10B use the upper 12bits as 0x800. Make sure that opafm.xml
	 * uses a PKEY in the range of 0x0 - 0xF for 10B
	 */
	pkey_8b = 0x8002;
	pkey_10b = 0x800;
	hfi_set_implicit_pkeys(ppd, &pkey_8b, &pkey_10b);

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
	/* Sanity check - ppd->pkeys[2] should be 0 */
	if (ppd->pkeys[2] != 0)
		ppd_dev_err(ppd, "%s pkey[2] already set to 0x%x, resetting it to 0x%x\n",
			    __func__, ppd->pkeys[2], OPA_FULL_MGMT_PKEY);
	ppd->pkeys[2] = OPA_FULL_MGMT_PKEY;
	hfi_set_ib_cfg(ppd, HFI_IB_CFG_PKEYS, 0, NULL);
}

static void hfi_set_sc_to_vlr(struct hfi_pportdata *ppd, u8 *t)
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
			reg_val |= (t[sc_num] & HFI_SC_VLR_MASK)
					 << (j * 4);
		write_lm_fpc_csr(ppd, FXR_FPC_CFG_SC_VL_TABLE_15_0 +
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
	  memcpy(ppd->sc_to_vlr, t, OPA_MAX_SCS);
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
	TP_CFG_CM_REMOTE_CRDT_RETURN_TBL0_t reg0;
	TP_CFG_CM_REMOTE_CRDT_RETURN_TBL1_t reg1;

	reg0.field.Entry0 = 0ull;
	reg0.field.Entry1 = 1ull;
	reg0.field.Entry2 = 2ull * cu;
	reg0.field.Entry3 = 4ull * cu;
	reg1.field.Entry4 = 8ull * cu;
	reg1.field.Entry5 = 16ull * cu;
	reg1.field.Entry6 = 32ull * cu;
	reg1.field.Entry7 = 64ull * cu;

	write_lm_cm_csr(ppd, offset, reg0.val);
	write_lm_cm_csr(ppd, offset + 0x08, reg1.val);
}

void hfi_assign_local_cm_au_table(struct hfi_pportdata *ppd, u8 vcu)
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
	TP_CFG_CM_DEDICATED_VL_CRDT_LIMIT_t reg;
	u32 addr = FXR_TP_CFG_CM_DEDICATED_VL_CRDT_LIMIT + 8 * vl_idx;

	reg.val = read_lm_cm_csr(ppd, addr);
	return reg.field.Dedicated;
}

static u16 hfi_get_shared_crdt(struct hfi_pportdata *ppd, int vl_idx)
{
	TP_CFG_CM_SHARED_VL_CRDT_LIMIT_t reg;
	u32 addr = FXR_TP_CFG_CM_SHARED_VL_CRDT_LIMIT + 8 * vl_idx;

	reg.val = read_lm_cm_csr(ppd, addr);
	return reg.field.Shared;
}

/*
 * Read the current credit merge limits.
 */
void hfi_get_buffer_control(struct hfi_pportdata *ppd,
			    struct buffer_control *bc, u16 *overall_limit)
{
	TP_CFG_CM_GLOBAL_SHRD_CRDT_LIMIT_t reg;
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

	reg.val = read_lm_cm_csr(ppd, FXR_TP_CFG_CM_GLOBAL_SHRD_CRDT_LIMIT);
	bc->overall_shared_limit = reg.field.Shared;
	if (overall_limit)
		*overall_limit = reg.field.Total_Credit_Limit;
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
	TP_CFG_CM_CRDT_SEND_t reg;

	reg.val = read_lm_cm_csr(ppd, FXR_TP_CFG_CM_CRDT_SEND);
	reg.field.AU = vau;
	write_lm_cm_csr(ppd, FXR_TP_CFG_CM_CRDT_SEND, reg.val);
}

static void hfi_set_global_shared(struct hfi_pportdata *ppd, u16 limit)
{
	TP_CFG_CM_GLOBAL_SHRD_CRDT_LIMIT_t reg;

	reg.val = read_lm_cm_csr(ppd, FXR_TP_CFG_CM_GLOBAL_SHRD_CRDT_LIMIT);
	reg.field.Shared = limit;
	write_lm_cm_csr(ppd, FXR_TP_CFG_CM_GLOBAL_SHRD_CRDT_LIMIT, reg.val);
}

static void hfi_set_global_limit(struct hfi_pportdata *ppd, u16 limit)
{
	TP_CFG_CM_GLOBAL_SHRD_CRDT_LIMIT_t reg;

	reg.val = read_lm_cm_csr(ppd, FXR_TP_CFG_CM_GLOBAL_SHRD_CRDT_LIMIT);
	reg.field.Total_Credit_Limit = limit;
	write_lm_cm_csr(ppd, FXR_TP_CFG_CM_GLOBAL_SHRD_CRDT_LIMIT, reg.val);
}

static void hfi_set_vl_dedicated(struct hfi_pportdata *ppd, int vl, u16 limit)
{
	TP_CFG_CM_DEDICATED_VL_CRDT_LIMIT_t reg;
	u32 addr = FXR_TP_CFG_CM_DEDICATED_VL_CRDT_LIMIT + (8 * hfi_vl_to_idx(vl));

	reg.val = read_lm_cm_csr(ppd, addr);
	reg.field.Dedicated = limit;
	write_lm_cm_csr(ppd, addr, reg.val);
}

static void hfi_set_vl_shared(struct hfi_pportdata *ppd, int vl, u16 limit)
{
	TP_CFG_CM_SHARED_VL_CRDT_LIMIT_t reg;
	u32 addr = FXR_TP_CFG_CM_SHARED_VL_CRDT_LIMIT + (8 * hfi_vl_to_idx(vl));

	reg.val = read_lm_cm_csr(ppd, addr);
	reg.field.Shared = limit;
	write_lm_cm_csr(ppd, addr, reg.val);
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
		reg = read_lm_cm_csr(ppd, FXR_TP_CFG_CM_CRDT_SEND) & mask;

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
	return 0;
}

static void hfi_set_sc_to_vlnt(struct hfi_pportdata *ppd, u8 *t)
{
	int i, j, sc_num;
	u64 reg_val;

	/* 2 registers, 16 entries per register, 4 bit per entry */
	for (i = 0, sc_num = 0; i < 2; i++) {
		for (j = 0, reg_val = 0; j < 16; j++, sc_num++)
			reg_val |= (t[sc_num] & HFI_SC_VLNT_MASK)
					 << (j * 4);
		write_lm_cm_csr(ppd, FXR_TP_CFG_CM_SC_TO_VLT_MAP +
				i * 8, reg_val);
	}

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

/*
 * Returns true if the SL is used for portals traffic and the SL is the
 * req SL of the req/resp SL pairs.
 */
bool hfi_is_portals_req_sl(struct hfi_pportdata *ppd, u8 sl)
{
	int i;

	for (i = 0; i < ppd->num_ptl_slp; i++)
		if (ppd->ptl_slp[i][0] == sl)
			return true;

	return false;
}

/*
 * Returns true if the SL is used for portals traffic. If it is a portals
 * SL then the response SL is also returned in resp_sl.
 */
static bool hfi_is_portals_sl(struct hfi_pportdata *ppd, u8 sl, u8 *resp_sl)
{
	int i, j;

	for (i = 0; i < ppd->num_ptl_slp; i++) {
		for (j = 0; j < HFI_MAX_MC; j++) {
			if (ppd->ptl_slp[i][j] == sl) {
				if (resp_sl)
					*resp_sl = ppd->ptl_slp[i][1];
				return true;
			}
		}
	}
	return false;
}

/*
 * Check portals SL pairs against the driver list of SL pairs from FM
 * Returns 0 for success and -EINVAL for failure
 */
static int hfi_check_ptl_slp(struct hfi_ctx *ctx, struct hfi_sl_pair *slp)
{
	struct hfi_devdata *dd = ctx->devdata;
	struct hfi_pportdata *ppd;
	int i;

	if (slp->port <= 0 || slp->port > dd->num_pports)
		return -EINVAL;

	ppd = to_hfi_ppd(dd, slp->port);
	for (i = 0; i < ppd->num_ptl_slp; i++) {
		u8 sl1 = ppd->ptl_slp[i][0];
		u8 sl2 = ppd->ptl_slp[i][1];

		if (slp->sl1 == sl1 && slp->sl2 == sl2)
			return 0;
	}
	return -EINVAL;
}

/* Service level to message class and traffic class mapping */
static void hfi_sl_to_mctc(struct hfi_pportdata *ppd)
{
	struct hfi_devdata *dd = ppd->dd;
	u64 reg[2];
	u32 tx_reg_addr, rx_reg_addr;
	int i, j, k, tc = 0;
	int nregs = ARRAY_SIZE(reg);

	/* set up SL to MCTC for portals traffic */
	for (i = 0; i < ppd->num_ptl_slp; i++) {
		u8 sl1 = ppd->ptl_slp[i][0];
		u8 sl2 = ppd->ptl_slp[i][1];

		/* Request SL on TC MC 0 */
		ppd->sl_to_mctc[sl1] = tc;
		/* Response SL on TC MC 1 */
		ppd->sl_to_mctc[sl2] = tc | (1 << 2);

		/* Round robin through available TCs. Locking? */
		tc = (tc + 1) % HFI_MAX_TC;
		if (tc == HFI_VL15_TC)
			tc = (tc + 1) % HFI_MAX_TC;
	}

	/* set up SL to MCTC for non portals traffic */
	for (i = 0; i < ARRAY_SIZE(ppd->sl_to_mctc); i++) {
		if (hfi_is_portals_sl(ppd, i, NULL))
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
		ppd->sl_to_mctc[i] = tc;
	}

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

/* Service channel to message class and traffic class mapping */
static void hfi_sc_to_mctc(struct hfi_pportdata *ppd, void *data)
{
	struct hfi_devdata *dd = ppd->dd;
	u8 *sc_to_sl = data;
	u64 reg[2];
	u32 reg_addr;
	int i, j, k;
	int nregs = ARRAY_SIZE(reg);

	switch (ppd_to_pnum(ppd)) {
	case 1:
		reg_addr = FXR_LM_CFG_PORT0_SC2MCTC0;
		break;
	case 2:
		reg_addr = FXR_LM_CFG_PORT1_SC2MCTC0;
		break;
	default:
		return;
	}

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

	switch (ppd_to_pnum(ppd)) {
	case 1:
		reg_addr = FXR_LM_CFG_PORT0_SL2SC0;
		break;
	case 2:
		reg_addr = FXR_LM_CFG_PORT1_SL2SC0;
		break;
	default:
		return;
	}

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

	/*
	 * Received packets are mapped through the Link Mux using
	 * SC*_TO_SL registers to obtain the outgoing SL based on the incoming
	 * SC. These replace the SC in the packet for Gen2 transport packets
	 * only. The way to determine how this table should be programmed is by
	 * looking up the SC -> request SL mapping provided by the FM for
	 * requests and then finding out the response SL based on the SL->TC/MC
	 * mapping
	 */
	for (i = 0, j = 0; i < nregs; i++) {
		for (k = 0; k < 64; k += 8, j++) {
			if (hfi_is_portals_sl(ppd, sc_to_sl[j], &resp_sl))
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
		hfi_sl_to_sc(ppd);
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
	default:
		dd_dev_info(dd, "%s: which %d: not implemented\n",
			__func__, which);
		break;
	}

	return ret;
}

int hfi_set_lid(struct hfi_pportdata *ppd, u32 lid, u8 lmc)
{
	struct hfi_devdata *dd = ppd->dd;

	ppd->lid = lid;
	ppd->lmc = lmc;
	hfi_set_ib_cfg(ppd, HFI_IB_CFG_LIDLMC, 0, NULL);

	ppd_dev_info(ppd, "IB%u:%u got a lid: 0x%x\n",
		     dd->unit, ppd->pnum, lid);

	return 0;
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
			eq->isr_cb(&eq->desc, eq->cookie);
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
	hfi2_dispose_firmware(dd);
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
	/*
	 * shutdown ports to notify OPA core clients.
	 * FXRTODO: Check error handling if hfi_pci_dd_init fails early
	 */
	hfi_pport_down(dd);

	if (dd->bus_dev)
		opa_core_unregister_device(dd->bus_dev);

	hfi2_ib_remove(dd);

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

	hfi_psn_uninit(dd);

	/* free host memory for FXR and Portals resources */
	if (dd->cq_head_base)
		free_pages((unsigned long)dd->cq_head_base,
			   get_order(dd->cq_head_size));

	if (dd->kregbase)
		iounmap((void __iomem *)dd->kregbase);

	idr_destroy(&dd->cq_pair);
	idr_destroy(&dd->ptl_user);
	idr_destroy(&dd->ptl_tpid);
	rcu_barrier(); /* wait for rcu callbacks to complete */

	pci_set_drvdata(dd->pcidev, NULL);
	kfree(dd);
}

static void hfi_port_desc(struct opa_core_device *odev,
			  struct opa_pport_desc *pdesc, u8 port_num)
{
	struct hfi_pportdata *ppd = to_hfi_ppd(odev->dd, port_num);
	int i;

	pdesc->pguid = ppd->pguid;
	pdesc->lid = ppd->lid;
	pdesc->num_vls_supported = ppd->vls_supported;
	for (i = 0; i < ppd->vls_supported; i++)
		pdesc->vl_mtu[i] = ppd->vl_mtu[i];
	pdesc->vl_mtu[15] = ppd->vl_mtu[15];
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
	pdesc->pstate = hfi_ibphys_portstate(ppd);
}

static void hfi_device_desc(struct opa_core_device *odev,
			    struct opa_dev_desc *desc)
{
	struct hfi_devdata *dd = odev->dd;

	memcpy(desc->oui, dd->oui, ARRAY_SIZE(dd->oui));
	desc->num_pports = dd->num_pports;
	desc->nguid = dd->nguid;
	desc->numa_node = dd->node;
	desc->ibdev = &dd->ibd->ibdev;
}

static int set_rsm_rule(struct opa_core_device *odev, struct hfi_rsm_rule *rule,
			struct hfi_ctx *rx_ctx[], u16 num_contexts)
{
	return hfi_rsm_set_rule(odev->dd, rule, rx_ctx, num_contexts);
}


static void clear_rsm_rule(struct opa_core_device *odev, u8 rule_idx)
{
	hfi_rsm_clear_rule(odev->dd, rule_idx);
}

static struct opa_core_ops opa_core_ops = {
	.ctx_assign = hfi_ctxt_attach,
	.ctx_release = hfi_ctxt_cleanup,
	.ctx_reserve = hfi_ctxt_reserve,
	.ctx_unreserve = hfi_ctxt_unreserve,
	.ctx_set_allowed_uids = hfi_ctxt_set_allowed_uids,
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
	.check_ptl_slp = hfi_check_ptl_slp,
	.get_hw_limits = hfi_get_hw_limits,
	.set_rsm_rule = set_rsm_rule,
	.clear_rsm_rule = clear_rsm_rule,
};

/*
 * hfi_ptc_init - initialize per traffic class data structs per port
 */
void hfi_ptc_init(struct hfi_pportdata *ppd)
{
	int i;

	for (i = 0; i < HFI_MAX_TC; i++) {
		struct hfi_ptcdata *tc = &ppd->ptc[i];

		ida_init(&tc->e2e_tx_state_cache);
		ida_init(&tc->e2e_rx_state_cache);
	}
}

void hfi_init_sc_to_vl_tables(struct hfi_pportdata *ppd)
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
	TP_CFG_MISC_CTRL_t misc;
	FPC_CFG_PORT_CONFIG_t fpc;

	misc.val = read_lm_tp_csr(ppd, FXR_TP_CFG_MISC_CTRL);
	/*FXRTODO: Check spec if operational_vl value is supplied by FM */
	misc.field.operational_vl = 0x1FF;
	misc.field.disable_reliable_vl15_pkts = 0;
	misc.field.disable_reliable_vl8_0_pkts = 0;
	write_lm_tp_csr(ppd, FXR_TP_CFG_MISC_CTRL, misc.val);

	fpc.val = read_lm_fpc_csr(ppd, FXR_FPC_CFG_PORT_CONFIG);
	fpc.field.mtu_cap = opa_mtu_to_id(HFI_DEFAULT_MAX_MTU);
	fpc.field.pkt_formats_enabled = 0xF;
	fpc.field.pkt_formats_supported = 0xF;
	write_lm_fpc_csr(ppd, FXR_FPC_CFG_PORT_CONFIG, fpc.val);
}

/*
 * hfi_pport_init - initialize per port
 * data structs
 */
int hfi_pport_init(struct hfi_devdata *dd)
{
	struct hfi_pportdata *ppd;
	int i, j, size;
	u8 port;
	int ret;

	for (port = 1; port <= dd->num_pports; port++) {
		ppd = to_hfi_ppd(dd, port);
		ppd->dd = dd;
		ppd->pnum = port;
		if (no_mnh) {
			ppd->lstate = IB_PORT_INIT;
			ppd->host_link_state = HLS_UP_INIT;
		} else {
			ppd->lstate = IB_PORT_DOWN;
			ppd->host_link_state = HLS_DN_OFFLINE;
		}
		mutex_init(&ppd->hls_lock);
		/* initialize Manor Hill - mnh/8051 */
		spin_lock_init(&ppd->crk8051_lock);
		ppd->crk8051_timed_out = 0;
		ppd->linkinit_reason = OPA_LINKINIT_REASON_LINKUP;
		ppd->sm_trap_qp = OPA_DEFAULT_SM_TRAP_QP;
		ppd->sa_qp = OPA_DEFAULT_SA_QP;

		/* FXRTODO: Remove after QSFP code implemented */
		ppd->port_type = HFI_PORT_TYPE_STANDARD;

		ppd->cc_max_table_entries = HFI_IB_CC_TABLE_CAP_DEFAULT;

		/* Initialize credit management variables */
		/* assign link credit variables */
		ppd->vau = HFI_CM_VAU;
		ppd->link_credits = HFI_RCV_BUFFER_SIZE /
				   hfi_vau_to_au(ppd->vau);
		ppd->vcu = hfi_cu_to_vcu(HFI_CM_CU);
		/* enough room for 8 MAD packets plus header - 17K */
		ppd->vl15_init = (8 * (HFI_MIN_VL_15_MTU + 128)) /
					hfi_vau_to_au(ppd->vau);
		if (ppd->vl15_init > ppd->link_credits)
			ppd->vl15_init = ppd->link_credits;

		hfi_assign_local_cm_au_table(ppd, ppd->vcu);

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
		ppd->link_speed_enabled = ppd->link_speed_supported;

		ppd->port_crc_mode_enabled = HFI_SUPPORTED_CRCS;
		/* initialize supported LTP CRC mode */
		ppd->port_ltp_crc_mode = hfi_cap_to_port_ltp(HFI_SUPPORTED_CRCS) <<
					HFI_LTP_CRC_SUPPORTED_SHIFT;
		/* initialize enabled LTP CRC mode */
		ppd->port_ltp_crc_mode |= hfi_cap_to_port_ltp(HFI_SUPPORTED_CRCS) <<
					HFI_LTP_CRC_ENABLED_SHIFT;

		/*
		 * Set the default MTU only for VL 15
		 * For rest of the data VLs, MTU of 0
		 * is valid as per the spec
		 */
		ppd->vl_mtu[15] = HFI_MIN_VL_15_MTU;
		ppd->vls_operational = ppd->vls_supported;
		hfi_init_linkmux_csrs(ppd);
		for (i = 0; i < ARRAY_SIZE(ppd->sl_to_sc); i++)
			ppd->sl_to_sc[i] = i;
		for (i = 0; i < ARRAY_SIZE(ppd->sc_to_sl); i++)
			ppd->sc_to_sl[i] = i;
		/*
		 * FXRTODO: Hard code the number of SL pairs for portals till
		 * we receive these values from the FM
		 */
		ppd->num_ptl_slp = HFI_NUM_PTL_SLP;
		for (i = 0, j = HFI_PTL_SL_START;
			i < ppd->num_ptl_slp; i++, j += 2) {
			ppd->ptl_slp[i][0] = j;
			ppd->ptl_slp[i][1] = j + 1;
		}
		hfi_sl_to_mctc(ppd);
		hfi_set_ib_cfg(ppd, HFI_IB_CFG_SL_TO_SC, 0, NULL);
		hfi_set_ib_cfg(ppd, HFI_IB_CFG_SC_TO_RESP_SL, 0, ppd->sc_to_sl);
		hfi_set_ib_cfg(ppd, HFI_IB_CFG_SC_TO_MCTC, 0, ppd->sc_to_sl);

		hfi_init_sc_to_vl_tables(ppd);
		hfi_ptc_init(ppd);

		size = sizeof(struct cc_state);
		RCU_INIT_POINTER(ppd->cc_state, kzalloc(size, GFP_KERNEL));
		if (!rcu_dereference(ppd->cc_state)) {
			dd_dev_err(dd, "Congestion Control Agent"\
					" disabled for port %d\n", port);
		}
	}

	ret = hfi2_load_firmware(dd);
	if (ret) {
		dd_dev_err(dd, "can't load firmware on 8051s");
		return ret;
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

	/* FXRTODO: Remove once silicon is stable */
	if (zebu) {
		force_loopback = true;
		no_mnh = true;
		no_pe_fw = true;
	}
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
	idr_init(&dd->ptl_tpid);
	idr_init(&dd->cq_pair);
	spin_lock_init(&dd->cq_lock);
	spin_lock_init(&dd->priv_tx_cq_lock);
	spin_lock_init(&dd->priv_rx_cq_lock);
	mutex_init(&dd->e2e_lock);

	/* FXR resources are on BAR0 (used for io_remap, etc.) */
	addr = pci_resource_start(pdev, HFI_FXR_BAR);
	len = pci_resource_len(pdev, HFI_FXR_BAR);
	dd->kregbase = ioremap_nocache(addr, len);
	if (!dd->kregbase) {
		ret = -ENOMEM;
		goto err_post_alloc;
	}
	dd->kregend = dd->kregbase + len;
	dd->physaddr = addr;
	dev_dbg(&pdev->dev, "BAR[%d] @ start 0x%lx len %lu\n",
			HFI_FXR_BAR, (long)addr, len);

	ret = hfi_psn_init(dd);
	if (ret)
		goto err_post_alloc;

	/* Ensure CSRs are sane, we can't trust they haven't been manipulated */
	init_csrs(dd);

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

	hfi_read_guid(dd);

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

		dev_dbg(&pdev->dev, "request for IRQ %d:%d\n", i, me->msix.vector);
		ret = request_irq(me->msix.vector, irq_eq_handler, 0,
				  "hfi_irq_eq", me);
		if (ret) {
			dev_err(&pdev->dev, "IRQ[%d] request failed %d\n", i, ret);
			/* IRQ cleanup done in hfi_pci_dd_free() */
			goto err_post_alloc;
		}
		/* mark as in use */
		me->arg = me;
	}
	dd->num_eq_irqs = i;
	atomic_set(&dd->msix_eq_next, 0);
	/* TODO - remove or change to debug later */
	dev_info(&pdev->dev, "%d IRQs assigned to EQs\n", i);

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
	memcpy(&dd->bus_id, &bus_id, sizeof(struct opa_core_device_id));
	ret = hfi2_ib_add(dd, &opa_core_ops);
	if (ret)
		goto err_post_alloc;

	/* bus agent can be probed immediately, no writing dd->bus_dev after this */
	dd->bus_dev = opa_core_register_device(&pdev->dev, &bus_id, dd, &opa_core_ops);
	/* All the unit management is handled by opa_core */
	dd->unit = dd->bus_dev->index;
	dd->bus_dev->kregbase = dd->kregbase;
	dd->bus_dev->kregend = dd->kregend;

	return dd;

err_post_alloc:
	dev_err(&pdev->dev, "%s %d FAILED ret %d\n", __func__, __LINE__, ret);
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
	RXHP_CFG_PSC_CACHE_ACCESS_CTL_t me_le_uh_cache_access = {.val = 0};
	union pte_cache_addr pte_cache_tag;
	union trig_op_cache_addr trig_op_cache_tag;
	/* TODO temporary structure for LE/ME/UH cache invalidation */
	union me_le_uh_cache_addr {
		struct {
			uint64_t me_le	: 1;  /* [0:0] ME/LE */
			uint64_t pid	: 12; /* [12:1] Target Process Id */
			uint64_t handle	: 16; /* [28:13] Handle */
		};
		uint64_t val;
	} me_le_uh_cache_tag;

	/* write PCB_LOW first to clear valid bit */
	write_csr(dd, FXR_RX_HIARB_CFG_PCB_LOW + (ptl_pid * 8), pcb_low.val);
	write_csr(dd, FXR_RX_HIARB_CFG_PCB_HIGH + (ptl_pid * 8), pcb_high.val);

	/* We need this lock to guard cache invalidation
	 * CSR writes, all pids use the same CSR */
	spin_lock(&dd->ptl_lock);

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
	write_csr(dd, FXR_RXHP_CFG_PSC_CACHE_ACCESS_CTL, me_le_uh_cache_access.val);

	/* TODO - above incomplete, deferred processing to wait for .ack bit */
	mdelay(10);

	/* TODO - write fake simics CSR to flush mini-TLB (AT interface TBD) */
	write_csr(dd, FXR_RX_HIARB_CSRS + 0x20000, 1);

	spin_unlock(&dd->ptl_lock);
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
	/* written as number of pages - 1, caller allocates at least one page */
	pcb_high.field.triggered_op_size = (ctx->trig_op_size >> PAGE_SHIFT) - 1;
	pcb_high.field.unexpected_size = (ctx->unexpected_size >> PAGE_SHIFT) - 1;
	pcb_high.field.le_me_size = (ctx->le_me_size >> PAGE_SHIFT) - 1;

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

void hfi_tpid_enable(struct hfi_devdata *dd, u8 idx, u16 base, u32 ptl_uid)
{
	RXHP_CFG_VTPID_CAM_t tpid;

	tpid.val = 0;
	tpid.field.valid = 1;
	tpid.field.uid = ptl_uid;
	tpid.field.tpid_base = base;
	write_csr(dd, FXR_RXHP_CFG_VTPID_CAM + (8 * idx), tpid.val);
}

void hfi_tpid_disable(struct hfi_devdata *dd, u8 idx)
{
	write_csr(dd, FXR_RXHP_CFG_VTPID_CAM + (8 * idx), 0);
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

static void write_csr_pidmap(struct hfi_devdata *dd, u32 base, int i, u8 rx_ctx)
{
	u32 csr_idx, off;
	u64 val;

	off = (i / 8) * 8;
	val = read_csr(dd, base + off);
	/* set just the 8-bit sub-field in this CSR */
	csr_idx = i % 8;
	val &= ~(0xFFuLL << (csr_idx * 8));
	val |= (u64)rx_ctx << (csr_idx * 8);
	write_csr(dd, base + off, val);
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
	RXHP_CFG_NPTL_RSM_CFG_t rsm_cfg = {.val = 0};
	RXHP_CFG_NPTL_RSM_SELECT_t select = {.val = 0};
	RXHP_CFG_NPTL_RSM_MATCH_t match = {.val = 0};
	int i, ret = 0;
	u8 idx = rule->idx;
	u16 map_idx, map_size;

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
		if (rx_ctx[i]->pid < HFI_PID_BYPASS_BASE ||
		    !(rx_ctx[i]->mode & HFI_CTX_MODE_BYPASS_MASK))
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

	rsm_cfg.val |= (1 << idx); /* set Enable bit */
	rsm_cfg.field.BypassHdrSize = rule->hdr_size;
	/* packet matching criteria */
	rsm_cfg.field.PacketType = rule->pkt_type;
	match.field.Mask1 = rule->match_mask[0];
	match.field.Value1 = rule->match_value[0];
	match.field.Mask2 = rule->match_mask[1];
	match.field.Value2 = rule->match_value[1];
	select.field.Field1Offset = rule->match_offset[0];
	select.field.Field2Offset = rule->match_offset[1];
	/* context selection criteria */
	rsm_cfg.field.Offset = map_idx;
	select.field.Index1Offset = rule->select_offset[0];
	select.field.Index1Width = rule->select_width[0];
	select.field.Index2Offset = rule->select_offset[1];
	select.field.Index2Width = rule->select_width[1];

	write_csr(dd, FXR_RXHP_CFG_NPTL_RSM_CFG + (8 * idx), rsm_cfg.val);
	write_csr(dd, FXR_RXHP_CFG_NPTL_RSM_SELECT + (8 * idx), select.val);
	write_csr(dd, FXR_RXHP_CFG_NPTL_RSM_MATCH + (8 * idx), match.val);

	for (i = 0; i < map_size; i++) {
		write_csr_pidmap(dd, FXR_RXHP_CFG_NPTL_RSM_MAP_TABLE,
				 map_idx + i,
				 rx_ctx[i]->pid - HFI_PID_BYPASS_BASE);
		rx_ctx[i]->rsm_mask |= (1 << idx);
	}

	dd_dev_dbg(dd, "active RSM rule %d off %d size %d\n",
		   idx, map_idx, map_size);
done:
	spin_unlock(&dd->ptl_lock);
	return ret;
}

void hfi_ctxt_set_bypass(struct hfi_ctx *ctx)
{
	struct hfi_devdata *dd = ctx->devdata;
	u8 rx_ctx;

	/* Get the 'Gen1 context' number from FXR PID */
	rx_ctx = ctx->pid - HFI_PID_BYPASS_BASE;

	spin_lock(&dd->ptl_lock);
	if (ctx->mode & (HFI_CTX_MODE_BYPASS_10B | HFI_CTX_MODE_BYPASS_16B)) {
		/*
		 * FXRTODO - Unconditionally claim bypass_pid for 10B/16B
		 * for now. Use RSM to distribute packets for other bypass
		 * contexts eventually.
		 */
		RXHP_CFG_NPTL_BYPASS_t bypass = {.val = 0};

		bypass.field.BypassContext = rx_ctx;
		bypass.field.HdrSize = 0;

		write_csr(dd, FXR_RXHP_CFG_NPTL_BYPASS, bypass.val);
		dd->bypass_pid = ctx->pid;
	}

	if (ctx->mode & HFI_CTX_MODE_BYPASS_9B) {
		int i, count;

		/* TODO - revisit when adding multiple receive context support */

		if (ctx->qpn_map_idx == 0) {
			write_csr(dd, FXR_RXHP_CFG_NPTL_VL15, rx_ctx);
			dd->vl15_pid = ctx->pid;
		}

		/* for each QPN_MAP entry, read appropriate CSR and update */
		count = min_t(int, ctx->qpn_map_count, OPA_QPN_MAP_MAX);
		for (i = ctx->qpn_map_idx; i < count; i++)
			write_csr_pidmap(dd, FXR_RXHP_CFG_NPTL_QP_MAP_TABLE,
					 i, rx_ctx);
	}
	spin_unlock(&dd->ptl_lock);
}
