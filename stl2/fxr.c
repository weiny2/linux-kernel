/*
 * Copyright (c) 2013 - 2014 Intel Corporation.  All rights reserved.
 * Copyright (c) 2008 - 2012 QLogic Corporation. All rights reserved.
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

#include <linux/pci.h>
#include <linux/module.h>
#include "../common/hfi.h"
#include "../include/fxr/fxr_fast_path_defs.h"
#include "../include/fxr/fxr_tx_ci_csrs.h"
#include "../include/fxr/fxr_rx_ci_csrs.h"
#include "../include/fxr/fxr_rx_hiarb_defs.h"
#include "../include/fxr/fxr_rx_hiarb_csrs.h"

static void hfi_cq_head_config(struct hfi_devdata *dd, u16 cq_idx,
			       void *head_base);

u64 read_csr(const struct hfi_devdata *dd, u32 offset)
{
	u64 val;
	BUG_ON(dd->kregbase == NULL);
	val = readq((void *)dd->kregbase + offset);
	return le64_to_cpu(val);
}

void write_csr(const struct hfi_devdata *dd, u32 offset, u64 value)
{
	BUG_ON(dd->kregbase == NULL);
	writeq(cpu_to_le64(value), (void *)dd->kregbase + offset);
}

/*
 * Do HFI chip-specific and PCIe cleanup. Free dd memory.
 */
void hfi_pci_dd_free(struct hfi_devdata *dd)
{
	stl_core_unregister_device(dd->bus_dev);

	cleanup_interrupts(dd);

	/* free host memory for FXR and Portals resources */
	if (dd->cq_head_base)
		free_pages((unsigned long)dd->cq_head_base,
			   get_order(dd->cq_head_size));
	if (dd->ptl_user)
		free_pages((unsigned long)dd->ptl_user,
			   get_order(dd->ptl_user_size));

	if (dd->kregbase)
		iounmap((void __iomem *)dd->kregbase);

	pci_set_drvdata(dd->pcidev, NULL);
	kfree(dd);
}

static struct stl_core_ops stl_core_ops = {
	.ctxt_assign = hfi_ptl_attach,
	.ctxt_release = hfi_ptl_cleanup,
	.cq_assign = hfi_cq_assign,
	.cq_update = hfi_cq_update,
	.cq_release = hfi_cq_release,
	.dlid_assign = hfi_dlid_assign,
	.dlid_release = hfi_dlid_release,
	.job_info = hfi_job_info,
	.job_init = hfi_job_init,
	.job_free = hfi_job_free,
	.job_setup = hfi_job_setup,
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
	struct stl_core_device_id bus_id;

	dd = hfi_alloc_devdata(pdev);
	if (IS_ERR(dd))
		return dd;

	/*
	 * Do remaining PCIe setup and save PCIe values in dd.
	 * On return, we have the chip mapped.
	 */

	dd->pcidev = pdev;
	pci_set_drvdata(pdev, dd);

	/* FXR has a single 128 MB BAR. */
	addr = pci_resource_start(pdev, 0);
	len = pci_resource_len(pdev, 0);
	pr_debug("BAR @ start 0x%lx len %lu\n", (long)addr, len);

	dd->kregbase = ioremap_nocache(addr, len);
	if (!dd->kregbase) {
		ret = -ENOMEM;
		goto err_post_alloc;
	}
	dd->kregend = dd->kregbase + len;

	/* Save BAR to rewrite after device reset. */
	dd->pcibar0 = addr;
	dd->pcibar1 = 0;
	dd->physaddr = addr;		/* used for io_remap, etc. */

	/* Host Memory allocations -- */

	/* Portals PID assignments - 32 KB */
	dd->ptl_user_size = (HFI_NUM_PIDS * 8);
	dd->ptl_user = (void *)__get_free_pages(GFP_KERNEL | __GFP_ZERO,
					get_order(dd->ptl_user_size));
	if (!dd->ptl_user) {
		ret = -ENOMEM;
		goto err_post_alloc;
	}
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

	/* TX and RX command queues */
	dd->cq_tx_base = (void *)dd->physaddr + FXR_TX_CQ_CSR;
	dd->cq_rx_base = (void *)dd->physaddr + FXR_RX_CQ_CSR;
	memset(&dd->cq_pair, HFI_PID_NONE, sizeof(dd->cq_pair));
	spin_lock_init(&dd->cq_lock);

	ret = setup_interrupts(dd, HFI_NUM_INTERRUPTS, 0);
	/* TODO - ignore error, FXR model missing MSI-X table */
	ret = 0;
	if (ret)
		goto err_post_alloc;

	/* TODO - bus support */
	bus_id.vendor = ent->vendor; //PCI_VENDOR_ID_INTEL;
	bus_id.device = ent->device; //PCI_DEVICE_ID_INTEL_FXR0;
	/* bus agent can be probed immediately, no writing dd->bus_dev after this */
	dd->bus_dev = stl_core_register_device(&pdev->dev, &bus_id, dd, &stl_core_ops);

	return dd;

err_post_alloc:
	hfi_pci_dd_free(dd);

	return ERR_PTR(ret);
}

void hfi_pcb_reset(struct hfi_devdata *dd, u16 ptl_pid)
{
	rx_cfg_hiarb_pcb_low_t pcb_low = {.val = 0};
	rx_cfg_hiarb_pcb_high_t pcb_high = {.val = 0};

	write_csr(dd, FXR_RX_CFG_HIARB_PCB_HIGH + (ptl_pid * 8), pcb_high.val);
	write_csr(dd, FXR_RX_CFG_HIARB_PCB_LOW + (ptl_pid * 8), pcb_low.val);
	/* TODO - write fake FXR simics CSR to invalidate cached PT */
	#define HFI_HP_PT_CACHE_CTRL 0x2210000
	write_csr(dd, HFI_HP_PT_CACHE_CTRL, 1);
}

void hfi_pcb_write(struct hfi_userdata *ud, u16 ptl_pid, int phys)
{
	struct hfi_devdata *dd = ud->devdata;
	rx_cfg_hiarb_pcb_low_t pcb_low = {.val = 0};
	rx_cfg_hiarb_pcb_high_t pcb_high = {.val = 0};
	u64 psb_addr;

	psb_addr = (phys) ? virt_to_phys(ud->ptl_state_base) :
			    (u64)ud->ptl_state_base;

	/* write PCB in FXR */
	pcb_low.valid = 1;
	pcb_low.physical = phys;
	pcb_low.portals_state_base = psb_addr >> PAGE_SHIFT;
	pcb_high.triggered_op_size = (ud->ptl_trig_op_size >> PAGE_SHIFT);
	pcb_high.unexpected_size = (ud->ptl_unexpected_size >> PAGE_SHIFT);
	pcb_high.le_me_size = (ud->ptl_le_me_size >> PAGE_SHIFT);

	write_csr(dd, FXR_RX_CFG_HIARB_PCB_HIGH + (ptl_pid * 8), pcb_high.val);
	write_csr(dd, FXR_RX_CFG_HIARB_PCB_LOW + (ptl_pid * 8), pcb_low.val);
}

/* Write CSR address for CQ head index, maintained by FXR */
static void hfi_cq_head_config(struct hfi_devdata *dd, u16 cq_idx,
			       void *head_base)
{
	u32 offset;
	u64 paddr;
	RX_CQ_HEAD_UPDATE_ADDR_t cq_head = {.val = 0};

	paddr = virt_to_phys(HFI_CQ_HEAD_ADDR(head_base, cq_idx));
	cq_head.Valid = 1;
	cq_head.PA = 1;
	cq_head.HD_PTR_HOST_ADDR = paddr;
	offset = FXR_RX_CQ_HEAD_UPDATE_ADDR + (cq_idx * 8);
	write_csr(dd, offset, cq_head.val);
}

/* 
 * Disable pair of CQs and reset flow control.
 * Reset of flow control is needed since CQ might have only had
 * a partial command or slot written by errant user.
 */
void hfi_cq_disable(struct hfi_devdata *dd, u16 cq_idx)
{
	/* write 0 to disable CSR (enable=0) */
	write_csr(dd, FXR_TX_CQ_CONFIG_CSR + (cq_idx * 8), 0);
	write_csr(dd, FXR_RX_CQ_CONFIG_CSR + (cq_idx * 8), 0);

	/* TODO - Drain or Reset CQ */
}

void hfi_cq_config_tuples(struct hfi_userdata *ud, u16 cq_idx,
			  struct hfi_auth_tuple *auth_table)
{
	struct hfi_devdata *dd = ud->devdata;
	int i;
	u32 offset;
	TX_CQ_AUTHENTICATION_CSR_t cq_auth = {.val = 0};

	/* write AUTH tuples */
	offset = FXR_TX_CQ_AUTHENTICATION_CSR + (cq_idx * HFI_NUM_AUTH_TUPLES * 8);
	for (i = 0; i < HFI_NUM_AUTH_TUPLES; i++) {
		if (ud->auth_mask == 0)
			cq_auth.USER_ID = ud->ptl_uid;
		else
			cq_auth.USER_ID = auth_table[i].uid;
		cq_auth.SRANK = auth_table[i].srank;
		write_csr(dd, offset, cq_auth.val);
		offset += 8;
	}
}

/* 
 * Write CSRs to configure a TX and RX Command Queue.
 * Authentication Tuple UIDs have been pre-validated by caller.
 */
void hfi_cq_config(struct hfi_userdata *ud, u16 cq_idx, void *head_base,
		   struct hfi_auth_tuple *auth_table)
{
	struct hfi_devdata *dd = ud->devdata;
	u32 offset;
	TX_CQ_CONFIG_CSR_t tx_cq_config = {.val = 0};
	RX_CQ_CONFIG_CSR_t rx_cq_config = {.val = 0};

	hfi_cq_config_tuples(ud, cq_idx, auth_table);

	/* set TX CQ config, enable */
	tx_cq_config.ENABLE = 1;
	tx_cq_config.PID = ud->ptl_pid;
	tx_cq_config.PRIV_LEVEL = 1;
	tx_cq_config.DLID_BASE = ud->dlid_base;
	tx_cq_config.PHYS_DLID = ud->allow_phys_dlid;
	tx_cq_config.SL_ENABLE = ud->sl_mask;
	offset = FXR_TX_CQ_CONFIG_CSR + (cq_idx * 8);
	write_csr(dd, offset, tx_cq_config.val);

	/* set RX CQ config, enable */
	rx_cq_config.ENABLE = 1;
	rx_cq_config.PID = ud->ptl_pid;
	rx_cq_config.PRIV_LEVEL = 1;
	offset = FXR_RX_CQ_CONFIG_CSR + (cq_idx * 8);
	write_csr(dd, offset, rx_cq_config.val);
}
