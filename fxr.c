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
#include "hfi.h"
#include "fxr.h"
#include "include/fxr/fxr_rx_hiarb_defs.h"
#include "include/fxr/fxr_rx_hiarb_csrs.h"

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
	cleanup_interrupts(dd);

	/* free host memory for FXR and Portals resources */
	if (dd->ptl_pid_user)
		free_pages((unsigned long)dd->ptl_pid_user,
			   get_order(dd->ptl_pid_user_size));
	if (dd->ptl_control)
		free_pages((unsigned long)dd->ptl_control,
			   get_order(dd->ptl_control_size));

	if (dd->kregbase)
		iounmap((void __iomem *)dd->kregbase);

	pci_set_drvdata(dd->pcidev, NULL);
	kfree(dd);
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
	int ret;
	rx_cfg_hiarb_pcb_base_t pcb_base = {.val = 0};

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

	/* Portals Control Block (PCB) - 128 KB */
	dd->ptl_control_size = HFI_PCB_TOTAL_MEM;
	dd->ptl_control = (void *)__get_free_pages(GFP_KERNEL | __GFP_ZERO,
					   get_order(dd->ptl_control_size));
	if (!dd->ptl_control) {
		ret = -ENOMEM;
		goto err_post_alloc;
	}
	spin_lock_init(&dd->ptl_control_lock);

	/* Portals PID assignments - 32 KB */
	dd->ptl_pid_user_size = (HFI_NUM_PTL_PIDS * 8);
	dd->ptl_pid_user = (void *)__get_free_pages(GFP_KERNEL | __GFP_ZERO,
					get_order(dd->ptl_pid_user_size));
	if (!dd->ptl_pid_user) {
		ret = -ENOMEM;
		goto err_post_alloc;
	}

	/* write PCB address in FXR */
	pcb_base.base_address = virt_to_phys(dd->ptl_control);
	pcb_base.physical = 1;
	/* TODO - RX_CFG_HIARB defines are broken in FXR header */
	//write_csr(dd, FXR_RX_CFG_HIARB_PCB_BASE, pcb_base.val);

	/* PSB is per PID and so allocated later,
	 * this is the minimum allocated.
	 */
	dd->trig_op_min_entries = HFI_PSB_TRIG_MIN_COUNT;
	dd->ptl_state_min_size = HFI_PSB_MIN_TOTAL_MEM;

	ret = setup_interrupts(dd, HFI_NUM_INTERRUPTS, 0);
	if (ret)
		goto err_post_alloc;

	return dd;

err_post_alloc:
	hfi_pci_dd_free(dd);

	return ERR_PTR(ret);
}
