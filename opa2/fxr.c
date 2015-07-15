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
#include <rdma/fxr/fxr_top_defs.h>
#include <rdma/fxr/fxr_fast_path_defs.h>
#include <rdma/fxr/fxr_tx_ci_csrs.h>
#include <rdma/fxr/fxr_rx_ci_csrs.h>
#include <rdma/fxr/fxr_rx_et_defs.h>
#include <rdma/fxr/fxr_rx_eq_csrs.h>
#include <rdma/fxr/fxr_rx_hiarb_defs.h>
#include <rdma/fxr/fxr_rx_hiarb_csrs.h>
#include <rdma/fxr/fxr_rx_hp_defs.h>
#include <rdma/fxr/fxr_rx_hp_csrs.h>
#include <rdma/fxr/fxr_rx_e2e_csrs.h>
#include <rdma/fxr/fxr_rx_e2e_defs.h>
#include <rdma/fxr/fxr_linkmux_defs.h>
#include <rdma/fxr/fxr_lm_csrs.h>
#include <rdma/fxr/fxr_tx_otr_pkt_top_csrs_defs.h>
#include <rdma/fxr/fxr_tx_otr_pkt_top_csrs.h>
#include "opa_hfi.h"
#include <rdma/opa_core_ib.h>

/* TODO - should come from HW headers */
#define FXR_CACHE_CMD_INVALIDATE 0x8

#define DLID_TABLE_SIZE (64 * 1024) /* 64k */

/* TODO: delete module parameter when possible */
static uint force_loopback;
module_param(force_loopback, uint, S_IRUGO);
MODULE_PARM_DESC(force_loopback, "Force FXR into loopback");

static void hfi_cq_head_config(struct hfi_devdata *dd, u16 cq_idx,
			       void *head_base);

static u64 read_csr(const struct hfi_devdata *dd, u32 offset)
{
	u64 val;

	BUG_ON(dd->kregbase[0] == NULL);
	val = readq(dd->kregbase[0] + offset);
	return le64_to_cpu(val);
}

static void write_csr(const struct hfi_devdata *dd, u32 offset, u64 value)
{
	BUG_ON(dd->kregbase[0] == NULL);
	writeq(cpu_to_le64(value), dd->kregbase[0] + offset);
}

static void hfi_init_rx_e2e_csrs(const struct hfi_devdata *dd)
{
	RXE2E_CFG_MC0_HDR_FLIT_CNT_CAM_t flt = {.val = 0};
	RXE2E_CFG_VALID_TC_SLID_t tc_slid = {.val = 0};

	flt.field.valid = 1;
	flt.field.l4 = PTL_RTS;
	flt.field.l2_mask = ~0;
	flt.field.l4_mask = 0;
	flt.field.opcode_mask = ~0;
	flt.field.cnt_m1 = 2;
	write_csr(dd, FXR_RXE2E_CFG_MC0_HDR_FLIT_CNT_CAM, flt.val);

	flt.field.l4 = PTL_REND_EVENT;
	write_csr(dd, FXR_RXE2E_CFG_MC0_HDR_FLIT_CNT_CAM + 8, flt.val);

	tc_slid.field.max_slid_p0 = HFI_MAX_LID_SUPP;
	tc_slid.field.tc_valid_p0 = 0xf;
	tc_slid.field.max_slid_p1 = HFI_MAX_LID_SUPP;
	tc_slid.field.tc_valid_p1 = 0xf;
	write_csr(dd, FXR_RXE2E_CFG_VALID_TC_SLID, tc_slid.val);
}

static void hfi_init_tx_otr_csrs(const struct hfi_devdata *dd)
{
	txotr_pkt_cfg_valid_tc_dlid_t tc_slid = {.val = 0};

	tc_slid.field.max_dlid_p0 = HFI_MAX_LID_SUPP;
	tc_slid.field.tc_valid_p0 = 0xf;
	tc_slid.field.max_dlid_p1 = HFI_MAX_LID_SUPP;
	tc_slid.field.tc_valid_p1 = 0xf;
	write_csr(dd, FXR_TXOTR_PKT_CFG_VALID_TC_DLID, tc_slid.val);
}

static void init_csrs(const struct hfi_devdata *dd)
{
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
		 * outstanding packets to 1 second for Simics back to back.
		 * TODO: Determine right value for FPGA & Silicon.
		 */
		txotr_timeout.field.SCALER = 0x00047869;
		write_csr(dd, FXR_TXOTR_PKT_CFG_TIMEOUT, txotr_timeout.val);
		/*
		 * Set the SLID based on the hostname to enable back to back
		 * support in Simics.
		 * TODO: Delete this hack once the LID is received via MADs
		 */
		if (!strcmp(utsname()->nodename, "viper0")) {
			dd->pport[0].lid = 0;
			dd->pport[1].lid = 1;
		}
		if (!strcmp(utsname()->nodename, "viper1")) {
			dd->pport[0].lid = 2;
			dd->pport[1].lid = 3;
		}
		lmp0.field.DLID = dd->pport[0].lid;
		write_csr(dd, FXR_LM_CONFIG_PORT0, lmp0.val);
		write_csr(dd, FXR_TXOTR_PKT_CFG_SLID_PT0, dd->pport[0].lid);
		lmp1.field.DLID = dd->pport[1].lid;
		write_csr(dd, FXR_LM_CONFIG_PORT1, lmp1.val);
		write_csr(dd, FXR_TXOTR_PKT_CFG_SLID_PT1, dd->pport[1].lid);
	}
	hfi_init_rx_e2e_csrs(dd);
	hfi_init_tx_otr_csrs(dd);
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
	u64 lmp_val;
	u32 lmp_offset;
	u32 slid_offset;

	switch (ppd->pnum) {
	case 1:
		lmp0.field.DLID = ppd->lid;
		if (!ppd->lmc) {
			lmp0.field.LMC_ENABLE = 1;
			lmp0.field.LMC_WIDTH = ppd->lmc;
		}
		lmp_offset = FXR_LM_CONFIG_PORT0;
		slid_offset = FXR_TXOTR_PKT_CFG_SLID_PT0;
		lmp_val = lmp0.val;
		break;
	case 2:
		lmp1.field.DLID = ppd->lid;
		if (!ppd->lmc) {
			lmp1.field.LMC_ENABLE = 1;
			lmp1.field.LMC_WIDTH = ppd->lmc;
		}
		lmp_offset = FXR_LM_CONFIG_PORT1;
		slid_offset = FXR_TXOTR_PKT_CFG_SLID_PT1;
		lmp_val = lmp1.val;
		break;
	default:
		dd_dev_err(dd, "Illegal port number %d\n", ppd->pnum);
		BUG_ON(1);
		return;
	}


	write_csr(dd, lmp_offset, lmp_val);
	write_csr(dd, slid_offset, ppd->lid);

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

int hfi_set_ib_cfg(struct hfi_pportdata *ppd, int which, u32 val)
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
	case HFI_IB_CFG_OP_VLS:
		if (ppd->vls_operational != val) {
			ppd->vls_operational = val;
		 /* FXRTODO: Implement FXR equivalent */
#if	0
			ret = sdma_map_init(
				ppd->dd,
				ppd->port - 1,
				hfi1_num_vls(val),
				NULL);
#endif
		}
		break;
	case HFI_IB_CFG_MTU:
		hfi_set_send_length(ppd);
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

	hfi_set_ib_cfg(ppd, HFI_IB_CFG_MTU, 0);

	/* FXRTODO: Fix the MTU change dependency for FXR */
#if 0
	if (drain)
		open_fill_data_vls(dd); /* reopen all VLs */

err:
#endif
	mutex_unlock(&ppd->hls_lock);

	return ret;
}

u8 hfi_ibphys_portstate(struct hfi_pportdata *ppd)
{
	/*
	 * FXRTODO: To be implemented as part of LNI
	 * for now return as disabled
	 */
	return IB_PORTPHYSSTATE_DISABLED;
}

void hfi_set_link_down_reason(struct hfi_pportdata *ppd, u8 lcl_reason,
			  u8 neigh_reason, u8 rem_reason)
{
	/* FXRTODO: To be implemented as part of LNI */
}

/*
 * Change the physical and/or logical link state.
 *
 * Returns 0 on success, -errno on failure.
 */
int hfi_set_link_state(struct hfi_pportdata *ppd, u32 state)
{
	/* FXRTODO: To be implemented as part of LNI */
	return -EINVAL;
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

static irqreturn_t irq_eq_handler(int irq, void *dev_id)
{
	struct hfi_msix_entry *me = dev_id;
	struct hfi_event_queue *eq;

	/* wake head waiter for each EQ using this IRQ */
	read_lock(&me->irq_wait_lock);
	list_for_each_entry(eq, &me->irq_wait_head, irq_wait_chain)
		wake_up_interruptible(&(eq->wq));
	read_unlock(&me->irq_wait_lock);

	return IRQ_HANDLED;
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

	if (dd->bus_dev)
		opa_core_unregister_device(dd->bus_dev);

	hfi_e2e_destroy(dd);

	cleanup_interrupts(dd);

	/* release system context and any privileged CQs */
	if (dd->priv_ctx.devdata) {
		hfi_cq_unmap(&dd->priv_tx_cq, &dd->priv_rx_cq);
		hfi_ctxt_cleanup(&dd->priv_ctx);
	}

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
	pdesc->ibmaxmtu = HFI_DEFAULT_MAX_MTU;
}

static void hfi_device_desc(struct opa_core_device *odev,
						struct opa_dev_desc *desc)
{
	struct hfi_devdata *dd = odev->dd;

	memcpy(desc->oui, dd->oui, ARRAY_SIZE(dd->oui));
	desc->num_pports = dd->num_pports;
	desc->nguid = dd->nguid;
	desc->numa_node = dd->node;
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
 * hfi_pport_init - initialize per port
 * data structs
 */
void hfi_pport_init(struct hfi_devdata *dd)
{
	struct hfi_pportdata *ppd;
	u8 port;

	for (port = 1; port <= dd->num_pports; port++) {
		ppd = to_hfi_ppd(dd, port);
		ppd->dd = dd;
		ppd->pnum = port;
		ppd->pguid = cpu_to_be64(PORT_GUID(dd->nguid, port));
		ppd->lstate = IB_PORT_DOWN;
		mutex_init(&ppd->hls_lock);

		ppd->vls_supported = HFI_NUM_DATA_VLS;

		/*
		 * Set the default MTU only for VL 15
		 * For rest of the data VLs, MTU of 0
		 * is valid as per the spec
		 */
		dd->vl_mtu[15] = HFI_MIN_VL_15_MTU;
		ppd->vls_operational = ppd->vls_supported;
		hfi_ptc_init(ppd);
	}
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

	/*
	 * FXRTODO: read nodeguid from MNH Register
	 * 8051_CFG_LOCAL_GUID. This register is
	 * yet to be implemented in Simics.
	 */
	dd->nguid = NODE_GUID;

	/* per port init */
	dd->num_pports = HFI_NUM_PPORTS;
	hfi_pport_init(dd);
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
	HFI_CTX_INIT(ctx, dd);
	/* configure system PID/PASID needed by privileged CQs */
	ctx_assign.pid = HFI_PID_SYSTEM;
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

	/* assign one EQ for privileged events */
	ret = hfi_eq_assign_privileged(ctx);
	if (ret)
		goto err_post_alloc;

	/* enable MSI-X */
	/* TODO - just ask for 64 IRQs for now */
	ret = setup_interrupts(dd, /* HFI_NUM_INTERRUPTS */ 64, 0);
	if (ret)
		goto err_post_alloc;

	/* configure IRQs for EQ groups */
	n_irqs = min_t(int, dd->num_msix_entries, HFI_NUM_EQ_INTERRUPTS);
	for (i = 0; i < n_irqs; i++) {
		struct hfi_msix_entry *me = &dd->msix_entries[i];

		BUG_ON(me->arg != NULL);
		INIT_LIST_HEAD(&me->irq_wait_head);
		rwlock_init(&me->irq_wait_lock);

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
	hfi_pci_dd_free(dd);

	return ERR_PTR(ret);
}

void hfi_pcb_reset(struct hfi_devdata *dd, u16 ptl_pid)
{
	RX_HIARB_CFG_PCB_LOW_t pcb_low = {.val = 0};
	RX_HIARB_CFG_PCB_HIGH_t pcb_high = {.val = 0};
	RXHP_CFG_PTE_CACHE_ACCESS_CTL_t pte_cache_access = {.val = 0};
	RXET_CFG_EQ_DESC_CACHE_ACCESS_CTL_t eq_cache_access = {.val = 0};
	RXET_CFG_TRIG_OP_CACHE_ACCESS_CTL_t trig_op_cache_access = {.val = 0};
	union pte_cache_addr pte_cache_tag;
	union eq_cache_addr eq_cache_tag;
	union trig_op_cache_addr trig_op_cache_tag;

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
	eq_cache_access.field.cmd = FXR_CACHE_CMD_INVALIDATE;
	eq_cache_tag.val = 0;
	eq_cache_tag.pid = ptl_pid;
	eq_cache_access.field.address = eq_cache_tag.val;
	eq_cache_tag.val = -1;
	eq_cache_tag.pid = 0;
	eq_cache_access.field.mask_address = eq_cache_tag.val;
	write_csr(dd, FXR_RXET_CFG_EQ_DESC_CACHE_ACCESS_CTL,
		  eq_cache_access.val);

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

	/* TODO - above incomplete, deferred processing to wait for .ack bit */

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
	RXCI_CFG_HEAD_UPDATE_ADDR_t cq_head = {.val = 0};
	TXCI_CFG_RESET_t tx_cq_reset = {.val = 0};
	RXCI_CFG_CQ_RESET_t rx_cq_reset = {.val = 0};

	head_offset = FXR_RXCI_CFG_HEAD_UPDATE_ADDR + (cq_idx * 8);

	/* disable CQ head before reset, as no assigned PASID */
	write_csr(dd, head_offset, 0);

	/* reset CQ state, as CQ head starts at 0 */
	tx_cq_reset.field.reset_cq = cq_idx;
	rx_cq_reset.field.reset_cq = cq_idx;
	write_csr(dd, FXR_TXCI_CFG_RESET, tx_cq_reset.val);
	write_csr(dd, FXR_RXCI_CFG_CQ_RESET, rx_cq_reset.val);
	/* TODO - reset needs async processing to wait for complete */

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
	TXCI_CFG_RESET_t tx_cq_reset = {.val = 0};
	RXCI_CFG_CQ_RESET_t rx_cq_reset = {.val = 0};

	/* reset CQ state, as CQ head starts at 0 */
	tx_cq_reset.field.reset_cq = cq_idx;
	rx_cq_reset.field.reset_cq = cq_idx;
	write_csr(dd, FXR_TXCI_CFG_RESET, tx_cq_reset.val);
	write_csr(dd, FXR_RXCI_CFG_CQ_RESET, rx_cq_reset.val);
#if 1 /* TODO: should be __SIMICS__ instead of 1 */
	mdelay(1); /* 'reset to zero' is not visible for a few cycles in Simics */
#endif
}

void hfi_cq_config_tuples(struct hfi_ctx *ctx, u16 cq_idx,
			  struct hfi_auth_tuple *auth_table)
{
	struct hfi_devdata *dd = ctx->devdata;
	int i;
	u32 offset;
	TXCI_CFG_AUTHENTICATION_CSR_t cq_auth = {.val = 0};

	/* write AUTH tuples */
	offset = FXR_TXCI_CFG_AUTHENTICATION_CSR + (cq_idx * HFI_NUM_AUTH_TUPLES * 8);
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
	TXCI_CFG_DLID_RT_t *p =
		(TXCI_CFG_DLID_RT_t *)dlid_assign->dlid_entries_ptr;
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

		write_csr(dd, FXR_TXCI_CFG_DLID_RT + offset, p->val);
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

	for (offset = dlid_base * sizeof(TXCI_CFG_DLID_RT_t);
		offset < (dlid_base + count) * sizeof(TXCI_CFG_DLID_RT_t);
		offset += sizeof(TXCI_CFG_DLID_RT_t))
		write_csr(dd, FXR_TXCI_CFG_DLID_RT + offset, 0);
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
	TXCI_CFG_CSR_t tx_cq_config = {.val = 0};
	RXCI_CFG_CNTRL_t rx_cq_config = {.val = 0};

	hfi_cq_config_tuples(ctx, cq_idx, auth_table);

	/* set TX CQ config, enable */
	tx_cq_config.field.enable = 1;
	tx_cq_config.field.pid = ctx->pid;
	tx_cq_config.field.priv_level = user_priv;
	tx_cq_config.field.dlid_base = ctx->dlid_base;
	tx_cq_config.field.phys_dlid = ctx->allow_phys_dlid;
	tx_cq_config.field.sl_enable = ctx->sl_mask;
	offset = FXR_TXCI_CFG_CSR + (cq_idx * 8);
	write_csr(dd, offset, tx_cq_config.val);

	/* set RX CQ config, enable */
	rx_cq_config.field.enable = 1;
	rx_cq_config.field.pid = ctx->pid;
	tx_cq_config.field.priv_level = user_priv;
	offset = FXR_RXCI_CFG_CNTRL + (cq_idx * 8);
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
