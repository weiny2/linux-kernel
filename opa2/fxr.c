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

#include <linux/module.h>
#include <rdma/fxr/fxr_fast_path_defs.h>
#include <rdma/fxr/fxr_tx_ci_csrs.h>
#include <rdma/fxr/fxr_rx_ci_csrs.h>
#include <rdma/fxr/fxr_rx_et_defs.h>
#include <rdma/fxr/fxr_rx_eq_csrs.h>
#include <rdma/fxr/fxr_rx_hiarb_defs.h>
#include <rdma/fxr/fxr_rx_hiarb_csrs.h>
#include <rdma/fxr/fxr_rx_hp_defs.h>
#include <rdma/fxr/fxr_rx_hp_csrs.h>
#include <rdma/fxr/fxr_linkmux_defs.h>
#include <rdma/fxr/fxr_lm_csrs.h>
#include "opa_hfi.h"

/* TODO - should come from HW headers */
#define FXR_CACHE_CMD_INVALIDATE 0x8

/* TODO - for now, start FXR in loopback */
static uint force_loopback = 1;
module_param(force_loopback, uint, S_IRUGO);
MODULE_PARM_DESC(force_loopback, "Force FXR into loopback");

static void hfi_cq_head_config(struct hfi_devdata *dd, u16 cq_idx,
			       void *head_base);

#if 0
/* Unused for now */
static u64 read_csr(const struct hfi_devdata *dd, u32 offset)
{
	u64 val;
	BUG_ON(dd->kregbase[0] == NULL);
	val = readq(dd->kregbase[0] + offset);
	return le64_to_cpu(val);
}
#endif

static void write_csr(const struct hfi_devdata *dd, u32 offset, u64 value)
{
	BUG_ON(dd->kregbase[0] == NULL);
	writeq(cpu_to_le64(value), dd->kregbase[0] + offset);
}

static void init_csrs(const struct hfi_devdata *dd)
{
	LM_CONFIG_t lm_config = {.val = 0};

	lm_config.field.FORCE_LOOPBACK = force_loopback;
	write_csr(dd, FXR_LM_CONFIG, lm_config.val);
}

/*
 * Do HFI chip-specific and PCIe cleanup. Free dd memory.
 */
void hfi_pci_dd_free(struct hfi_devdata *dd)
{
	int i;

	opa_core_unregister_device(dd->bus_dev);

	cleanup_interrupts(dd);

	/* release any privileged CQs */
	hfi_cq_cleanup(&dd->priv_ctx);

	idr_destroy(&dd->cq_pair);
	idr_destroy(&dd->ptl_user);

	hfi_iommu_root_clear_context(dd);

	/* free host memory for FXR and Portals resources */
	if (dd->cq_head_base)
		free_pages((unsigned long)dd->cq_head_base,
			   get_order(dd->cq_head_size));

	for (i = 0; i < HFI_NUM_BARS; i++)
		if (dd->kregbase[i])
			iounmap((void __iomem *)dd->kregbase[i]);

	pci_set_drvdata(dd->pcidev, NULL);
	kfree(dd);
}

static void hfi_port_desc(struct opa_pport_desc *pdesc, u8 port_num)
{
	struct hfi_pportdata *ppd = get_ppd_pn(pdesc->devdata, port_num);

	pdesc->pguid = ppd->pguid;
}

static void hfi_device_desc(struct opa_dev_desc *desc)
{
	struct hfi_devdata *dd = desc->devdata;

	memcpy(desc->oui, dd->oui, ARRAY_SIZE(dd->oui));
	desc->num_pports = dd->num_pports;
	desc->nguid = dd->nguid;
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
	.dlid_assign = hfi_dlid_assign,
	.dlid_release = hfi_dlid_release,
	.eq_assign = hfi_eq_assign,
	.eq_release = hfi_eq_release,
	.get_device_desc = hfi_device_desc,
	.get_port_desc = hfi_port_desc,
};

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
	int i, ret;
	struct opa_core_device_id bus_id;
	struct hfi_ctx *ctx;
	u16 priv_cq_idx;

	dd = hfi_alloc_devdata(pdev);
	if (IS_ERR(dd))
		return dd;

	/*
	 * Do remaining PCIe setup and save PCIe values in dd.
	 * On return, we have the chip mapped.
	 */

	dd->pcidev = pdev;
	pci_set_drvdata(pdev, dd);

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

	/* Host Memory allocations -- */

	/* Portals PID assignments */
	idr_init(&dd->ptl_user);
	spin_lock_init(&dd->ptl_lock);

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
	dd->cq_tx_base = (void *)dd->physaddr + FXR_TXCQ_ENTRY;
	dd->cq_rx_base = (void *)dd->physaddr + FXR_RXCQ_ENTRY;
	idr_init(&dd->cq_pair);
	spin_lock_init(&dd->cq_lock);

	ctx = &dd->priv_ctx;
	ctx->devdata = dd;
	ctx->allow_phys_dlid = 1;
	ctx->sl_mask = -1;
	ctx->ptl_pid = HFI_PID_NONE;
	ctx->ptl_uid = 0;
	/* assign one CQ for privileged commands (DLID, EQ_DESC_WRITE) */
	ret = hfi_cq_assign_privileged(ctx, &priv_cq_idx);
	if (ret)
		goto err_post_alloc;
	/* TODO - next is to write the hfi_cq structs (see VNIC code) */

	/* enable MSI-X */
	/* TODO - just ask for 64 IRQs for now */
	ret = setup_interrupts(dd, /* HFI_NUM_INTERRUPTS */ 64, 0);
	if (ret)
		goto err_post_alloc;

	bus_id.vendor = ent->vendor;
	bus_id.device = ent->device;
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
	rx_cfg_hiarb_pcb_low_t pcb_low = {.val = 0};
	rx_cfg_hiarb_pcb_high_t pcb_high = {.val = 0};
	RXHP_CFG_PTE_CACHE_ACCESS_CTL_t pte_cache_access = {.val = 0};
	RXET_CFG_EQ_DESC_CACHE_ACCESS_CTL_t eq_cache_access = {.val = 0};
	RXET_CFG_TRIG_OP_CACHE_ACCESS_CTL_t trig_op_cache_access = {.val = 0};
	union pte_cache_addr pte_cache_tag;
	union eq_cache_addr eq_cache_tag;
	union trig_op_cache_addr trig_op_cache_tag;

	/* write PCB_LOW first to clear valid bit */
	write_csr(dd, FXR_RX_CFG_HIARB_PCB_LOW + (ptl_pid * 8), pcb_low.val);
	write_csr(dd, FXR_RX_CFG_HIARB_PCB_HIGH + (ptl_pid * 8), pcb_high.val);

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
	write_csr(dd, 0x2820000, 1);
}

void hfi_pcb_write(struct hfi_ctx *ctx, u16 ptl_pid)
{
	struct hfi_devdata *dd = ctx->devdata;
	rx_cfg_hiarb_pcb_low_t pcb_low = {.val = 0};
	rx_cfg_hiarb_pcb_high_t pcb_high = {.val = 0};
	u64 psb_addr;

	psb_addr = (u64)ctx->ptl_state_base;

	/* write PCB in FXR */
	pcb_low.field.valid = 1;
	pcb_low.field.portals_state_base = psb_addr >> PAGE_SHIFT;
	pcb_high.field.triggered_op_size = (ctx->trig_op_size >> PAGE_SHIFT);
	pcb_high.field.unexpected_size = (ctx->unexpected_size >> PAGE_SHIFT);
	pcb_high.field.le_me_size = (ctx->le_me_size >> PAGE_SHIFT);

	write_csr(dd, FXR_RX_CFG_HIARB_PCB_HIGH + (ptl_pid * 8), pcb_high.val);
	write_csr(dd, FXR_RX_CFG_HIARB_PCB_LOW + (ptl_pid * 8), pcb_low.val);
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
	cq_head.field.hd_ptr_host_addr =
			(u64)HFI_CQ_HEAD_ADDR(head_base, cq_idx);
	write_csr(dd, head_offset, cq_head.val);
}

/*
 * Disable pair of CQs and reset flow control.
 * Reset of flow control is needed since CQ might have only had
 * a partial command or slot written by errant user.
 */
void hfi_cq_disable(struct hfi_devdata *dd, u16 cq_idx)
{
	/* write 0 to disable CSR (enable=0) */
	write_csr(dd, FXR_TXCI_CFG_CSR + (cq_idx * 8), 0);
	write_csr(dd, FXR_RXCI_CFG_CNTRL + (cq_idx * 8), 0);

	/* TODO - Drain or Reset CQ */
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
 * Write CSRs to configure a TX and RX Command Queue.
 * Authentication Tuple UIDs have been pre-validated by caller.
 */
void hfi_cq_config(struct hfi_ctx *ctx, u16 cq_idx, void *head_base,
		   struct hfi_auth_tuple *auth_table, bool unprivileged)
{
	struct hfi_devdata *dd = ctx->devdata;
	u32 offset;
	TXCI_CFG_CSR_t tx_cq_config = {.val = 0};
	RXCI_CFG_CNTRL_t rx_cq_config = {.val = 0};

	hfi_cq_config_tuples(ctx, cq_idx, auth_table);

	/* set TX CQ config, enable */
	tx_cq_config.field.enable = 1;
	tx_cq_config.field.pid = ctx->ptl_pid;
	tx_cq_config.field.priv_level = unprivileged;
	tx_cq_config.field.dlid_base = ctx->dlid_base;
	tx_cq_config.field.phys_dlid = ctx->allow_phys_dlid;
	tx_cq_config.field.sl_enable = ctx->sl_mask;
	offset = FXR_TXCI_CFG_CSR + (cq_idx * 8);
	write_csr(dd, offset, tx_cq_config.val);

	/* set RX CQ config, enable */
	rx_cq_config.field.enable = 1;
	rx_cq_config.field.pid = ctx->ptl_pid;
	tx_cq_config.field.priv_level = unprivileged;
	offset = FXR_RXCI_CFG_CNTRL + (cq_idx * 8);
	write_csr(dd, offset, rx_cq_config.val);
}

int hfi_ctxt_hw_addr(struct hfi_ctx *ctx, int type, u16 ctxt, void **addr, ssize_t *len)
{
	struct hfi_devdata *dd;
	void *psb_base;
	struct hfi_ctx *cq_ctx;
	unsigned long flags;
	int ret = 0;

	BUG_ON(!ctx || !ctx->devdata);
	dd = ctx->devdata;

	psb_base = ctx->ptl_state_base;
	BUG_ON(psb_base == NULL);

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
