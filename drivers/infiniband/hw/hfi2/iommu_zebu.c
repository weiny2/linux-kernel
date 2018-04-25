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
#include <linux/sched/mm.h>
#include "chip/iommu_cr_top_regs.h"
#include "chip/fxr_top_defs.h"
#include "chip/fxr_at_csrs.h"
#include "chip/fxr_at_defs.h"
#include "chip/fxr_pcim_defs.h"
#include "chip/fxr_pcim_csrs.h"
#include "chip/fxr_linkmux_defs.h"
#include "chip/translation_structs.h"
#include "hfi2.h"

#define NUM_CTXT_ENTRIES 256

static union ExtendedRootEntry_t *iommu_root_tbl;
static struct mm_struct *iommu_mm;

static void write_iommu_csr(const struct hfi_devdata *dd, u32 offset, u64 value)
{
	BUG_ON(!dd->iommu_base);
	writeq(cpu_to_le64(value), dd->iommu_base + offset);
}

#if 1
static u64 read_iommu_csr(const struct hfi_devdata *dd, u32 offset)
{
	BUG_ON(!dd->iommu_base);
	return readq(dd->iommu_base + offset);
}
#endif

static void write_iommu_csr32(const struct hfi_devdata *dd, u32 offset, u32 value)
{
	BUG_ON(!dd->iommu_base);
	writel(cpu_to_le32(value), dd->iommu_base + offset);
}

static u32 read_iommu_csr32(const struct hfi_devdata *dd, u32 offset)
{
	BUG_ON(!dd->iommu_base);
	return readl(dd->iommu_base + offset);
}

int
hfi_iommu_root_alloc(void)
{
	int ret = 0;

	if (!zebu || !iommu_hack)
		return 0;

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
	if (!zebu || !iommu_hack)
		return;

	if (iommu_mm) {
		mmput(iommu_mm);
		iommu_mm = NULL;
	}
	if (iommu_root_tbl) {
		free_page((u64)iommu_root_tbl);
		iommu_root_tbl = NULL;
	}
}

struct qi_desc {
	u64 low, high;
};

struct q_inval {
	struct page_req_dsc *prq;
	struct mutex	q_lock;
	struct qi_desc  *desc;          /* invalidation queue */
	int             *desc_status;   /* desc status */
	int             free_head;      /* first free entry */
	int             free_tail;      /* last free entry */
};

static struct q_inval qi;

#define QI_LENGTH 256 /* queue length */

#define DMAR_PQH_REG    0xc0    /* Page request queue head register */
#define DMAR_PQT_REG    0xc8    /* Page request queue tail register */
#define DMAR_PQA_REG    0xd0    /* Page request queue address register */

int
hfi_iommu_root_set_context(struct hfi_devdata *dd)
{
	RTADDR_REG_0_0_0_VTDBAR_t rt_csr = {.val = 0};
	GCMD_REG_0_0_0_VTDBAR_t gcmd_csr = {.val = 0};
	ECAP_REG_0_0_0_VTDBAR_t ecap = {.val = 0};
	union ExtendedRootEntry_t r_entry;
	union ExtendedContextEntry_t *c, *c_tbl;
	void *p_tbl;
	unsigned bus, dev_id;
	struct page *desc_page;

	if (!zebu || !iommu_hack)
		return 0;

	if (!iommu_root_tbl)
		return -EFAULT;

	dd->iommu_base = ioremap_nocache(0xfed15000, 0x1000);
	if (!dd->iommu_base)
		return -ENOMEM;

	/* TODO - simics hardcodes these */
#if 0
	bus = 0x86;
	dev_id = 0x71;
#else
	bus = 0x0;
	dev_id = 0x40;
#endif

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
	//ctxt_e[0].wf_SLPTPTR((__SecondLevelAddr>>FOUR_K_PAGE_SBIT))
	c->PASIDE = 1;
	c->AW = 2;   /* 4-lvl page table */
	c->WPE = 1;  /* don't allow writes to read-only pages */
	c->PASIDPTR = (u64)virt_to_phys(p_tbl) >> PAGE_SHIFT;
	c->FPD = 1;
	c->PRE = 1;
	c->T = 1;
	c->NXE = 1;
	c->CD = 1;
	c->SMEP = 1;
	c->PTS = 1;
	c->DID = 1;
	c->PTS = 7;
	c->P = 1;

	/* write root context_entry for busX */
	/* TODO - if multi-HFI, this will overwrite context_entry ptr */
#if 1
	r_entry.UCTP = (u64)virt_to_phys(c_tbl) >> PAGE_SHIFT;
	r_entry.UP = 1;
#else
	r_entry.LCTP = (u64)virt_to_phys(c_tbl) >> PAGE_SHIFT;
	r_entry.LP = 1;
#endif
	iommu_root_tbl[bus].val[0] = r_entry.val[0];
	iommu_root_tbl[bus].val[1] = r_entry.val[1]; /* devices 16-31 not supported */

	/* set root_table in IOMMU */
	rt_csr.field.RTT = 1;
	rt_csr.field.RTA = virt_to_phys(iommu_root_tbl) >> PAGE_SHIFT;
	write_iommu_csr(dd, RTADDR_REG_0_0_0_VTDBAR_OFFSET, rt_csr.val);

	ecap.val = read_iommu_csr(dd, ECAP_REG_0_0_0_VTDBAR_OFFSET);

	if (!ecap.field.QI) {
		pr_err("design does not support QI\n");
		BUG_ON(1);
	}

	desc_page = alloc_pages(GFP_KERNEL|__GFP_ZERO, 0);
	if (!desc_page) {
		pr_err("Allocation failed\n");
		BUG_ON(1);
	}

	qi.desc_status = kzalloc(QI_LENGTH * sizeof(int), GFP_ATOMIC);
	if (!qi.desc_status) {
		pr_err("Allocation failed\n");
		BUG_ON(1);
	}

	mutex_init(&qi.q_lock);

	qi.desc = page_address(desc_page);

	qi.free_head = qi.free_tail = 0;

	/* write zero to the tail reg */
	write_iommu_csr(dd, IQT_REG_0_0_0_VTDBAR_OFFSET, 0);

	write_iommu_csr(dd, IQA_REG_0_0_0_VTDBAR_OFFSET, virt_to_phys(qi.desc));
#if 0
	desc_page = alloc_pages(GFP_KERNEL | __GFP_ZERO, 0);
	if (!desc_page) {
		pr_err("Allocation failed\n");
		BUG_ON(1);
	}
	qi.prq = page_address(desc_page);

	write_iommu_csr(dd, DMAR_PQH_REG, 0x0);
	write_iommu_csr(dd, DMAR_PQT_REG, 0x0);
	write_iommu_csr(dd, DMAR_PQA_REG, virt_to_phys(qi.prq));
#endif

	gcmd_csr.field.TE = 1;
	gcmd_csr.field.SRTP = 1;
	gcmd_csr.field.QIE = 1;
	write_iommu_csr32(dd, GCMD_REG_0_0_0_VTDBAR_OFFSET, gcmd_csr.val);

	while (1) {
		GSTS_REG_0_0_0_VTDBAR_t gsts;

		gsts.val = read_iommu_csr32(dd, GSTS_REG_0_0_0_VTDBAR_OFFSET);
		printk("qies 0x%x\n", gsts.field.QIES);
		if (gsts.field.QIES)
			break;
		printk("waiting for qies 0x%x\n", gsts.field.QIES);
		mdelay(1);
	}

	pr_err("iommu_root_tbl 0x%llx c_tbl 0x%llx p_tbl 0x%llx ver 0x%x BDF 0x%llx cap 0x%llx ecap 0x%llx\n",
	       virt_to_phys(iommu_root_tbl), virt_to_phys(c_tbl),
	       virt_to_phys(p_tbl), read_iommu_csr32(dd, VER_REG_0_0_0_VTDBAR_OFFSET),
	       read_csr(dd, FXR_PCIM_STS_BDF),
	       read_iommu_csr(dd, CAP_REG_0_0_0_VTDBAR_OFFSET),
	       read_iommu_csr(dd, ECAP_REG_0_0_0_VTDBAR_OFFSET));
#if 0
	{
		u64 head = read_iommu_csr(dd, DMAR_PQH_REG);
		u64 tail = read_iommu_csr(dd, DMAR_PQT_REG);
		u32 sts = read_iommu_csr32(dd, FSTS_REG_0_0_0_VTDBAR_OFFSET);

		pr_info("%s %d head 0x%llx tail 0x%llx flag 0x%x\n",
			__func__, __LINE__,
			head, tail, sts);
	}
#endif
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
hfi_iommu_zebu_set_pasid(struct hfi_devdata *dd, struct mm_struct *user_mm, u16 pid)
{
	FXR_AT_CFG_PASID_LUT_t lut = {.val = 0};
	union PasidEntry_t p_entry, *p_tbl;
	struct mm_struct *mm;
	u16 pasid;

	p_tbl = (union PasidEntry_t *)dd->iommu_pasid_tbl;
	/*
	 * if user_mm is NULL, then request is for PASID with kernel-privileged
	 * (supervisor) access
	 */
	mm = (user_mm) ? user_mm : iommu_mm;

	if (user_mm)
		pasid = pid;
	else
		pasid = 0;

	p_entry.val = 0;
	p_entry.SRE = 1;
	/* this is level-1 (w/PASID) translation pg_table */
	p_entry.FLPTPTR = virt_to_phys(mm->pgd) >> PAGE_SHIFT;
	p_entry.P = 1;
	p_tbl[pasid].val = p_entry.val;
	//pr_info("%s mm->pgd 0x%llx\n", __func__, virt_to_phys(mm->pgd));
	//pr_info("%s pdp 0x%llx\n", __func__, *(u64*)((char *)mm->pgd + 0x880));

	if (pasid == HFI_PID_SYSTEM) {
		FXR_AT_CFG_USE_SYSTEM_PASID_t sys = {.val = 0};

		sys.field.enable = 1;
		sys.field.system_pasid = pasid;
		write_csr(dd, FXR_AT_CFG_USE_SYSTEM_PASID, sys.val);
		//pr_info("%s %d 0x%llx\n", __func__, __LINE__,
		//	read_csr(dd, FXR_AT_CFG_USE_SYSTEM_PASID));
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
	write_csr(dd, FXR_AT_CFG_PASID_LUT + (pid * 8), lut.val);
	//pr_info("%s %d 0x%llx\n", __func__, __LINE__,
	//	read_csr(dd, FXR_AT_CFG_PASID_LUT + (pid * 8)));
}

#define HFI_IOMMU_TIMEOUT_MS	10000

#if 0
static void
hfi_iommu_flush_context(struct hfi_devdata *dd)
{
	CCMD_REG_0_0_0_VTDBAR_t ccmd = {.val = 0};
	unsigned long exit_jiffies = jiffies +
		msecs_to_jiffies(HFI_IOMMU_TIMEOUT_MS);

	ccmd.field.CIRG = 1;
	ccmd.field.ICC = 1;

	pr_debug("%s ccmd.val 0x%llx\n", __func__, ccmd.val);
	write_iommu_csr(dd, CCMD_REG_0_0_0_VTDBAR_OFFSET, ccmd.val);

	while(1) {
		ccmd.val = read_iommu_csr(dd, CCMD_REG_0_0_0_VTDBAR_OFFSET);
		pr_debug("%s ccmd.val 0x%llx\n", __func__,
			read_iommu_csr(dd, CCMD_REG_0_0_0_VTDBAR_OFFSET));
		if (!ccmd.field.ICC)
			break;
		if (time_after(jiffies, exit_jiffies)) {
			pr_err("%s timed out\n", __func__);
			break;
		}
		cpu_relax_lowlatency();
		mdelay(1);
	}
	pr_debug("%s ccmd.val 0x%llx\n", __func__,
		read_iommu_csr(dd, CCMD_REG_0_0_0_VTDBAR_OFFSET));
}

static void
hfi_iommu_flush_iotlb(struct hfi_devdata *dd)
{
	IOTLB_REG_0_0_0_VTDBAR_t ccmd = {.val = 0};
	ECAP_REG_0_0_0_VTDBAR_t ecap = {.val = 0};
	u32 offset;
	unsigned long exit_jiffies = jiffies +
		msecs_to_jiffies(HFI_IOMMU_TIMEOUT_MS);

	ecap.val = read_iommu_csr(dd, ECAP_REG_0_0_0_VTDBAR_OFFSET);

	pr_debug("%s ecap.val 0x%llx iro 0x%llx\n",
		__func__, ecap.val, (u64)ecap.field.IRO);
	offset = IOTLB_REG_0_0_0_VTDBAR_OFFSET + (ecap.field.IRO * 16);

	ccmd.field.IVT = 1;
	ccmd.field.IIRG = 1;

	pr_debug("%s ccmd.val 0x%llx\n", __func__, ccmd.val);
	write_iommu_csr(dd, offset, ccmd.val);

	while(1) {
		ccmd.val = read_iommu_csr(dd, offset);
		pr_debug("%s ccmd.val 0x%llx\n", __func__,
			read_iommu_csr(dd, offset));
		if (!ccmd.field.IVT)
			break;
		if (time_after(jiffies, exit_jiffies)) {
			pr_err("%s timed out\n", __func__);
			break;
		}
		cpu_relax_lowlatency();
		mdelay(1);
	}
	pr_debug("%s ccmd.val 0x%llx\n", __func__, read_iommu_csr(dd, offset));
}
#else

#define DMA_TLB_FLUSH_GRANU_OFFSET 60
#define DMA_CCMD_INVL_GRANU_OFFSET 61

#define DMA_CCMD_GLOBAL_INVL (((u64)1) << 61)

#define QI_CC_TYPE              0x1
#define QI_IOTLB_TYPE           0x2
#define QI_DIOTLB_TYPE          0x3
#define QI_IWD_TYPE             0x5
#define QI_EIOTLB_TYPE          0x6
#define QI_DEIOTLB_TYPE         0x8

#define VTD_PAGE_SHIFT          (12)
#define VTD_PAGE_SIZE           (1UL << VTD_PAGE_SHIFT)
#define VTD_PAGE_MASK           (((u64)-1) << VTD_PAGE_SHIFT)
#define VTD_PAGE_ALIGN(addr)    (((addr) + VTD_PAGE_SIZE - 1) & VTD_PAGE_MASK)

#define QI_IOTLB_DID(did)       (((u64)did) << 16)
#define QI_IOTLB_DR(dr)         (((u64)dr) << 7)
#define QI_IOTLB_DW(dw)         (((u64)dw) << 6)
#define QI_IOTLB_GRAN(gran)     (((u64)gran) >> (DMA_TLB_FLUSH_GRANU_OFFSET-4))
#define QI_IOTLB_ADDR(addr)     (((u64)addr) & VTD_PAGE_MASK)
#define QI_IOTLB_IH(ih)         (((u64)ih) << 6)
#define QI_IOTLB_AM(am)         (((u8)am))

#define QI_CC_FM(fm)            (((u64)fm) << 48)
#define QI_CC_SID(sid)          (((u64)sid) << 32)
#define QI_CC_DID(did)          (((u64)did) << 16)
#define QI_CC_GRAN(gran)        (((u64)gran) >> (DMA_CCMD_INVL_GRANU_OFFSET-4))

#define QI_PC_TYPE              0x7
#define QI_PC_PASID(pasid)      (((u64)pasid) << 32)
#define QI_PC_DID(did)          (((u64)did) << 16)
#define QI_PC_GRAN(gran)        (((u64)gran) << 4)
#define QI_PC_PASID_SEL         (QI_PC_TYPE | QI_PC_GRAN(1))

/* IOTLB_REG */
#define DMA_TLB_FLUSH_GRANU_OFFSET  60
#define DMA_TLB_GLOBAL_FLUSH (((u64)1) << 60)
#define DMA_TLB_DSI_FLUSH (((u64)2) << 60)
#define DMA_TLB_PSI_FLUSH (((u64)3) << 60)
#define DMA_TLB_IIRG(type) ((type >> 60) & 7)
#define DMA_TLB_IAIG(val) (((val) >> 57) & 7)
#define DMA_TLB_READ_DRAIN (((u64)1) << 49)
#define DMA_TLB_WRITE_DRAIN (((u64)1) << 48)
#define DMA_TLB_DID(id) (((u64)((id) & 0xffff)) << 32)
#define DMA_TLB_IVT (((u64)1) << 63)
#define DMA_TLB_IH_NONLEAF (((u64)1) << 6)
#define DMA_TLB_MAX_SIZE (0x3f)

enum {
        QI_FREE,
        QI_IN_USE,
        QI_DONE,
        QI_ABORT
};

#define QI_IWD_STATUS_DATA(d)   (((u64)d) << 32)
#define QI_IWD_STATUS_WRITE     (((u64)1) << 5)
#define DMAR_IQ_SHIFT   4       /* Invalidation queue head/tail shift */

#define QI_EIOTLB_ADDR(addr)    ((u64)(addr) & VTD_PAGE_MASK)
#define QI_EIOTLB_GL(gl)        (((u64)gl) << 7)
#define QI_EIOTLB_IH(ih)        (((u64)ih) << 6)
#define QI_EIOTLB_AM(am)        (((u64)am))
#define QI_EIOTLB_PASID(pasid)  (((u64)pasid) << 32)
#define QI_EIOTLB_DID(did)      (((u64)did) << 16)
#define QI_EIOTLB_GRAN(gran)    (((u64)gran) << 4)

#define QI_DEV_EIOTLB_ADDR(a)   ((u64)(a) & VTD_PAGE_MASK)
#define QI_DEV_EIOTLB_SIZE      (((u64)1) << 11)
#define QI_DEV_EIOTLB_GLOB(g)   ((u64)g)
#define QI_DEV_EIOTLB_PASID(p)  (((u64)p) << 32)
#define QI_DEV_EIOTLB_SID(sid)  ((u64)((sid) & 0xffff) << 16)
#define QI_DEV_EIOTLB_QDEP(qd)  (((qd) & 0x1f) << 12)
#define QI_DEV_EIOTLB_MAX_INVS  32

#define QI_DEV_IOTLB_SID(sid)   ((u64)((sid) & 0xffff) << 32)
#define QI_DEV_IOTLB_QDEP(qdep) (((qdep) & 0x1f) << 16)
#define QI_DEV_IOTLB_ADDR(addr) ((u64)(addr) & VTD_PAGE_MASK)
#define QI_DEV_IOTLB_SIZE       1
#define QI_DEV_IOTLB_MAX_INVS   32

#define QI_GRAN_ALL_ALL                 0

static void qi_submit_sync(struct hfi_devdata *dd, struct qi_desc *desc, bool wait)
{
	int rc;
	struct qi_desc *hw, wait_desc;
	int wait_index, index;
	unsigned long exit_jiffies = jiffies +
		msecs_to_jiffies(HFI_IOMMU_TIMEOUT_MS);

	hw = qi.desc;
	rc = 0;

	mutex_lock(&qi.q_lock);
	index = qi.free_head;
	wait_index = (index + 1) % QI_LENGTH;

	qi.desc_status[index] = qi.desc_status[wait_index] = QI_IN_USE;

	hw[index] = *desc;

	if (wait) {
#define STATUS_WRITE 1
#if STATUS_WRITE
		wait_desc.low = QI_IWD_STATUS_DATA(QI_DONE) |
			QI_IWD_STATUS_WRITE | QI_IWD_TYPE;
		wait_desc.high = virt_to_phys(&qi.desc_status[wait_index]);
		hw[wait_index] = wait_desc;
#else
		wait_desc.low = QI_IWD_TYPE;
		wait_desc.high = 0;
		hw[wait_index] = wait_desc;
#endif
		qi.free_head = (qi.free_head + 2) % QI_LENGTH;
	} else {
		qi.free_head = (qi.free_head + 1) % QI_LENGTH;
	}
	/*
	 * update the HW tail register indicating the presence of
	 * new descriptors.
	 */
	write_iommu_csr(dd, IQT_REG_0_0_0_VTDBAR_OFFSET, qi.free_head << DMAR_IQ_SHIFT);

	while(1) {
		IQH_REG_0_0_0_VTDBAR_t head;
		IQT_REG_0_0_0_VTDBAR_t tail;

		head.val = read_iommu_csr(dd, IQH_REG_0_0_0_VTDBAR_OFFSET);
		tail.val = read_iommu_csr(dd, IQT_REG_0_0_0_VTDBAR_OFFSET);

		//pr_err("%s %d head 0x%x tail 0x%x\n",
		//	 __func__, __LINE__, head.field.QH, tail.field.QT);
		if (head.field.QH == tail.field.QT)
			break;
#if 1
		if (time_after(jiffies, exit_jiffies)) {
			pr_err("%s timed out\n", __func__);
			break;
		}
#endif
		cpu_relax();
		mdelay(10);
	}
#if STATUS_WRITE
	if (wait && qi.desc_status[wait_index] != QI_DONE)
		pr_err("%s %d QI Wait Status value %d incorrect\n",
		       __func__, __LINE__, qi.desc_status[wait_index]);
	qi.desc_status[index] = QI_DONE;
#endif
	mutex_unlock(&qi.q_lock);
}

#define QI_GRAN_ALL_ALL                 0
#define QI_GRAN_NONG_ALL                1
#define QI_GRAN_NONG_PASID              2
#define QI_GRAN_PSI_PASID               3

void hfi_iommu_flush_iotlb(struct hfi_devdata *dd, u16 pasid)
{
	struct qi_desc desc;

	if (!zebu || !iommu_hack)
		return;

	/* Invalidate Pasid - All Pasid */
	desc.high = 0;
	desc.low = QI_PC_TYPE | QI_PC_DID(1) | QI_PC_PASID_SEL | QI_PC_PASID(pasid);
	//desc.low = QI_PC_TYPE | QI_PC_DID(1);
	qi_submit_sync(dd, &desc, false);

#if 1
	/* Invalidate IOTLB */
	desc.high = 0;
	//desc.low = QI_IOTLB_GRAN(DMA_TLB_GLOBAL_FLUSH) | QI_IOTLB_TYPE;
	desc.low = 0xd0 | QI_IOTLB_TYPE;
	qi_submit_sync(dd, &desc, true);
#endif

	/* Invalidate Extended IOTLB */
	desc.low = QI_EIOTLB_PASID(pasid) | QI_EIOTLB_GRAN(QI_GRAN_PSI_PASID) | QI_EIOTLB_DID(1) | QI_EIOTLB_TYPE;
	desc.high = 0;
	qi_submit_sync(dd, &desc, false);
#if 0
	/* Invalidate Device TLB */
	desc.low = QI_DEV_IOTLB_SID(0xC0) | QI_DIOTLB_TYPE;
	desc.high = QI_DEV_IOTLB_ADDR(-1ULL >> 1) | QI_DEV_IOTLB_SIZE;
	qi_submit_sync(dd, &desc, false);
	pr_err("Submitting Device TLB descriptor\n");
#endif
	read_csr(dd, FXR_LM_CFG_FP_TIMER_PORT0);
	/* Invalidate Extended Device TLB */
	desc.low = QI_DEV_EIOTLB_PASID(pasid) | QI_DEV_EIOTLB_SID(0xC0)
		| QI_DEIOTLB_TYPE;
	desc.high = QI_DEV_EIOTLB_ADDR(-1ULL >> 1) | QI_DEV_EIOTLB_SIZE;
	//printk("%s %d desc.high 0x%llx desc.low 0x%llx\n",
	//	__func__, __LINE__, desc.high, desc.low);
	qi_submit_sync(dd, &desc, true);
}
#endif

void
hfi_iommu_zebu_clear_pasid(struct hfi_devdata *dd, u16 pasid)
{
	union PasidEntry_t *p_tbl;
	FXR_AT_CFG_PASID_LUT_t lut = {.val = 0};

	if (!zebu || !iommu_hack)
		return;

	lut.field.enable = 0;
	write_csr(dd, FXR_AT_CFG_PASID_LUT + (pasid * 8), lut.val);
	pr_debug("%s %d 0x%llx\n", __func__, __LINE__,
		read_csr(dd, FXR_AT_CFG_PASID_LUT + (pasid * 8)));

	if (pasid == HFI_PID_SYSTEM) {
		FXR_AT_CFG_USE_SYSTEM_PASID_t sys = {.val = 0};

		sys.field.enable = 0;
		write_csr(dd, FXR_AT_CFG_USE_SYSTEM_PASID, sys.val);
		pr_info("%s %d 0x%llx\n", __func__, __LINE__,
			read_csr(dd, FXR_AT_CFG_USE_SYSTEM_PASID));
	}

	p_tbl = (union PasidEntry_t *)dd->iommu_pasid_tbl;
	p_tbl[pasid].val = 0;
#if 0
	mdelay(1000);
	{
		u64 head = read_iommu_csr(dd, DMAR_PQH_REG);
		u64 tail = read_iommu_csr(dd, DMAR_PQT_REG);
		u32 sts = read_iommu_csr32(dd, FSTS_REG_0_0_0_VTDBAR_OFFSET);

		pr_info("%s %d head 0x%llx tail 0x%llx flag 0x%x\n",
			__func__, __LINE__,
			head, tail, sts);
	}
#endif
#if 0
	if (!simics)
		hfi_iommu_flush_context(dd);
	hfi_iommu_flush_iotlb(dd);
#else
	hfi_iommu_flush_iotlb(dd, pasid);
#endif
}
