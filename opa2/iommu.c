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
 * Routines below manage setting IOMMU tables for DMA remapping hardware
 * for w/PASID address translations.
 * The hierachy here of tables (root, context, pasid) is described in the
 * Intel VT-d specfication.
 *
 * a) The iommu_root_table is a page with entries of pointers to context
 *    tables for each PCIe bus.
 * b) The context table is a page of entries with pointers to PASID table
 *    for each device-function attached to a PCIe bus.
 * c) The PASID table contains entries with pointers to the root of the
 *    first level paging structures (base of PML4) for a process context.
 *    The PASID is one-to-one mapped to a Linux mm_struct and the associated
 *    page table for associated Linux tasks, hence mm_struct is passed
 *    into the set_pasid rountine.
 *
 * The page table for page walks is then found with:
 * Root[pci_bus]->Context[pci_device_func]->PASID[process_address_space_id]
 */

#include <linux/kthread.h>
#include <linux/types.h>
#include <rdma/fxr/iommu_cr_top_regs.h>
#include <rdma/fxr/fxr_top_defs.h>
#include <rdma/fxr/fxr_at_csrs.h>
#include <rdma/fxr/fxr_at_defs.h>
#include <rdma/fxr/translation_structs.h>
#include "opa_hfi.h"

#define NUM_CTXT_ENTRIES 256

static union ExtendedRootEntry_t *iommu_root_tbl;
static struct mm_struct *iommu_mm;

static void write_iommu_csr(const struct hfi_devdata *dd, u32 offset, u64 value)
{
	BUG_ON(dd->kregbase[1] == NULL);
	writeq(cpu_to_le64(value), dd->kregbase[1] + offset);
}

static void write_iommu_csr32(const struct hfi_devdata *dd, u32 offset, u32 value)
{
	BUG_ON(dd->kregbase[1] == NULL);
	writel(cpu_to_le32(value), dd->kregbase[1] + offset);
}

int
hfi_iommu_root_alloc(void)
{
	int ret = 0;

	if (iommu_root_tbl)
		return -EEXIST;

	/* Need page-alignment, root entries fill exactly one page */
	iommu_root_tbl = (void *)get_zeroed_page(GFP_KERNEL);
	if (!iommu_root_tbl)
		return -ENOMEM;

	/*
	 * We need valid MM struct for system PASID and kernel clients to
	 * access kernel virtual memory.  The thread that probes/loads modules
	 * is not likely present when the IOMMU is doing address translation.
	 * When Linux pasid APIs are available, supervisor-privileged PASIDs
	 * will be associated with master kernel page tables.
	 * As Linux pasid support is not available and Linux doesn't export
	 * init_mm, we will just hold this task's MM and release when driver
	 * is unloaded.
	 */
	iommu_mm = get_task_mm(current);
	if (!iommu_mm) {
		ret = -EFAULT;
		goto err;
	}

	return 0;

err:
	hfi_iommu_root_free();
	return ret;
}

void
hfi_iommu_root_free(void)
{
	if (iommu_mm) {
		mmput(iommu_mm);
		iommu_mm = NULL;
	}
	if (iommu_root_tbl) {
		free_page((u64)iommu_root_tbl);
		iommu_root_tbl = NULL;
	}
}

int
hfi_iommu_root_set_context(struct hfi_devdata *dd)
{
	RTADDR_REG_0_0_0_VTDBAR_t rt_csr = {.val = 0};
	GCMD_REG_0_0_0_VTDBAR_t gcmd_csr = {.val = 0};
	union ExtendedRootEntry_t r_entry;
	union ExtendedContextEntry_t *c, *c_tbl;
	void *p_tbl;
	unsigned bus, dev_id;

	if (!iommu_root_tbl)
		return -EFAULT;

	/* TODO - simics hardcodes these */
	bus = 1;
	dev_id = 0;

	/* Need page-alignment, fills two pages */
	c_tbl = (void *)__get_free_pages(GFP_KERNEL | __GFP_ZERO,
					 get_order(sizeof(*c) * NUM_CTXT_ENTRIES));
	if (!c_tbl)
		return -ENOMEM;
	/*
	 * We only allocate PASID table for 12-bits of PASID which is sufficient
	 * for STL2 driver management of IOMMU (PASID = PID).
	 * Need page-alignment.
	 */
	p_tbl = (void *)__get_free_pages(GFP_KERNEL | __GFP_ZERO,
					 get_order(sizeof(union PasidEntry_t) * HFI_NUM_PIDS));
	if (!p_tbl) {
		free_pages((u64)c_tbl,
			   get_order(sizeof(*c) * NUM_CTXT_ENTRIES));
		return -ENOMEM;
	}
	dd->iommu_excontext_tbl = c_tbl;
	dd->iommu_pasid_tbl = p_tbl;

	/* Set context_entry for PCI device-funcY, sets PASID table */
	c = &c_tbl[dev_id];
	c->PASIDE = 1;
	c->AW = 2;   /* 4-lvl page table */
	c->WPE = 1;  /* don't allow writes to read-only pages */
	c->PASIDPTR = (u64)virt_to_phys(p_tbl) >> PAGE_SHIFT;
	c->P = 1;

	/* write root context_entry for busX */
	/* TODO - if multi-HFI, this will overwrite context_entry ptr */
	r_entry.LCTP = (u64)virt_to_phys(c_tbl) >> PAGE_SHIFT;
	r_entry.LP = 1;
	iommu_root_tbl[bus].val[0] = r_entry.val[0];
	iommu_root_tbl[bus].val[1] = 0; /* devices 16-31 not supported */

	/* set root_table in IOMMU */
	rt_csr.field.RTT = 1;
	rt_csr.field.RTA = virt_to_phys(iommu_root_tbl) >> PAGE_SHIFT;
	write_iommu_csr(dd, RTADDR_REG_0_0_0_VTDBAR_OFFSET, rt_csr.val);
	gcmd_csr.field.TE = 1;
	gcmd_csr.field.SRTP = 1;
	write_iommu_csr32(dd, GCMD_REG_0_0_0_VTDBAR_OFFSET, gcmd_csr.val);

	return 0;
}

void
hfi_iommu_root_clear_context(struct hfi_devdata *dd)
{
	/* may be called during error cleanup without BAR mapped */
	if (!dd->kregbase[1])
		return;

	/* disable translation */
	write_iommu_csr32(dd, GCMD_REG_0_0_0_VTDBAR_OFFSET, 0);
	/* clear RT pointer */
	write_iommu_csr(dd, RTADDR_REG_0_0_0_VTDBAR_OFFSET, 0);

	/* remove context table ptr */
	/* TODO - simics hardcodes bus=1 */
	iommu_root_tbl[1].val[0] = 0;

	if (dd->iommu_excontext_tbl)
		free_pages((u64)dd->iommu_excontext_tbl,
			   get_order(sizeof(union ExtendedContextEntry_t) * NUM_CTXT_ENTRIES));
	if (dd->iommu_pasid_tbl)
		free_pages((u64)dd->iommu_pasid_tbl,
			   get_order(sizeof(union PasidEntry_t) * HFI_NUM_PIDS));
}

void
hfi_iommu_set_pasid(struct hfi_devdata *dd, struct mm_struct *user_mm, u16 pasid)
{
	AT_CFG_PASID_LUT_t lut = {.val = 0};
	union PasidEntry_t p_entry, *p_tbl;
	struct mm_struct *mm;

	p_tbl = (union PasidEntry_t *)dd->iommu_pasid_tbl;
	/*
	 * if user_mm is NULL, then request is for PASID with kernel-privileged
	 * (supervisor) access
	 */
	mm = (user_mm) ? user_mm : iommu_mm;

	p_entry.val = 0;
	p_entry.SRE = 1;
	/* this is level-1 (w/PASID) translation pg_table */
	p_entry.FLPTPTR = virt_to_phys(mm->pgd) >> PAGE_SHIFT;
	p_entry.P = 1;
	p_tbl[pasid].val = p_entry.val;

	if (pasid == HFI_PID_SYSTEM) {
		AT_CFG_USE_SYSTEM_PASID_t sys = {.val = 0};

		sys.field.enable = 1;
		sys.field.system_pasid = pasid;
		write_csr(dd, FXR_AT_CFG_USE_SYSTEM_PASID, sys.val);
	}
	lut.field.enable = 1;
	if (user_mm) {
		lut.field.privilege_level = 1;
		lut.field.pasid = pasid;
	} else {
		/* Use system PASID for the system PID and kernel clients */
		lut.field.privilege_level = 0;
		/* FXRTODO: All kernel clients should use system pasid 0 */
		lut.field.pasid = pasid;
	}
	write_csr(dd, FXR_AT_CFG_PASID_LUT + (pasid * 8), lut.val);
}

void
hfi_iommu_clear_pasid(struct hfi_devdata *dd, u16 pasid)
{
	union PasidEntry_t *p_tbl;
	AT_CFG_PASID_LUT_t lut = {.val = 0};

	lut.field.enable = 0;
	write_csr(dd, FXR_AT_CFG_PASID_LUT + (pasid * 8), lut.val);

	if (pasid == HFI_PID_SYSTEM) {
		AT_CFG_USE_SYSTEM_PASID_t sys = {.val = 0};

		sys.field.enable = 0;
		write_csr(dd, FXR_AT_CFG_USE_SYSTEM_PASID, sys.val);
	}

	p_tbl = (union PasidEntry_t *)dd->iommu_pasid_tbl;
	p_tbl[pasid].val = 0;
}
