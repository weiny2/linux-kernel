/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a GPLv2 license.  When using or
 * redistributing this file, you must do so under this license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2017 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

/*
 * Intel(R) Omni-Path Gen2 HFI PCIe Driver
 */

#include <linux/kthread.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/pci-ats.h>
#include <linux/signal.h>
#include <linux/sched/signal.h>
#include <linux/sched/mm.h>
#include <asm/cacheflush.h>
#include "chip/fxr_top_defs.h"
#include "chip/fxr_at_csrs.h"
#include "chip/fxr_at_defs.h"
#include "hfi2.h"
#include "debugfs.h"
#include "at.h"
#include "trace.h"

/* TODO: Do not upstream CPU Page Table (CPT) support */
static bool use_kernel_cpt = true;
module_param(use_kernel_cpt, bool, 0444);
MODULE_PARM_DESC(use_kernel_cpt, "Use/Share kernel CPU page table");

static bool use_user_cpt = true;
module_param(use_user_cpt, bool, 0444);
MODULE_PARM_DESC(use_user_cpt, "Use/Share user process CPU page tables");

/* TODO: Check whether we should only support page promotion method */
static bool user_page_promo = true;
module_param(user_page_promo, bool, 0444);
MODULE_PARM_DESC(user_page_promo, "user page promotion");

static bool kernel_page_promo = true;
module_param(kernel_page_promo, bool, 0444);
MODULE_PARM_DESC(kernel_page_promo, "kernel page promotion");

/* AT interrupt handling. Most stuff are MSI-like. */
enum faulttype {
	AT_REMAP,
	UNKNOWN,
};

/* Spec defines remap fault reason between 0x1-0x1f */
static const char * const at_remap_fault_reasons[] = {
	"Software",
	"Present bit in root entry is clear",
	"Present bit in context entry is clear",
	"Invalid context entry",
	"Access beyond MGAW",
	"PTE Write access is not set",
	"PTE Read access is not set",
	"Next page table ptr is invalid",
	"Root table address invalid",
	"Context table ptr is invalid",
	"non-zero reserved fields in RTP",
	"non-zero reserved fields in CTP",
	"non-zero reserved fields in PTE",
	"PCE for translation request specifies blocking"
};

/* protect pasid idr code */
static DEFINE_MUTEX(pasid_mutex);

static void hfi_at_stats_cleanup(struct hfi_at *at);
static void hfi_flush_svm_range(struct hfi_at_svm *svm, unsigned long address,
				unsigned long pages, int ih, int gl);

static const char *at_get_fault_reason(u8 fault_reason, int *fault_type)
{
	if (fault_reason < ARRAY_SIZE(at_remap_fault_reasons)) {
		*fault_type = AT_REMAP;
		return at_remap_fault_reasons[fault_reason];
	}

	*fault_type = UNKNOWN;
	return "Unknown";
}

static int at_fault_do_one(struct hfi_at *at, int type,
			   u8 fault_reason, u16 source_id,
			   unsigned long long addr)
{
	const char *reason;
	int fault_type;

	reason = at_get_fault_reason(fault_reason, &fault_type);

	dd_dev_err(at->dd,
		   "[%s] at [%s]/[%02x:%02x.%d] fault addr %llx [fault reason %02d] %s\n",
		   type ? "AT Read" : "AT Write", at->name,
		   source_id >> 8, PCI_SLOT(source_id & 0xFF),
		   PCI_FUNC(source_id & 0xFF), addr, fault_reason, reason);
	return 0;
}

#define PRIMARY_FAULT_REG_LEN (16)
static irqreturn_t at_fault(int irq, void *dev_id)
{
	struct hfi_irq_entry *me = dev_id;
	struct hfi_at *at = me->dd->at;
	int reg, fault_index;
	u32 fault_status;
	unsigned long flag;
	bool ratelimited;
	static DEFINE_RATELIMIT_STATE(rs,
				      DEFAULT_RATELIMIT_INTERVAL,
				      DEFAULT_RATELIMIT_BURST);

	/* Disable printing, simply clear the fault when ratelimited */
	ratelimited = !__ratelimit(&rs);

	raw_spin_lock_irqsave(&at->register_lock, flag);
	fault_status = readl(at->reg + AT_FSTS_REG);
	if (fault_status && !ratelimited)
		dd_dev_err(at->dd, "AT: handling fault status reg %x\n",
			   fault_status);

	/* TBD: ignore advanced fault log currently */
	if (!(fault_status & AT_FSTS_PPF))
		goto unlock_exit;

	fault_index = at_fsts_fault_record_index(fault_status);
	reg = cap_fault_reg_offset(at->cap);
	while (1) {
		u8 fault_reason;
		u16 source_id;
		u64 guest_addr;
		int type;
		u32 data;

		/* highest 32 bits */
		data = readl(at->reg + reg +
				fault_index * PRIMARY_FAULT_REG_LEN + 12);
		if (!(data & AT_FRCD_F))
			break;

		if (!ratelimited) {
			fault_reason = at_frcd_fault_reason(data);
			type = at_frcd_type(data);

			data = readl(at->reg + reg +
				     fault_index * PRIMARY_FAULT_REG_LEN + 8);
			source_id = at_frcd_source_id(data);

			guest_addr = at_readq(at->reg + reg +
					fault_index * PRIMARY_FAULT_REG_LEN);
			guest_addr = at_frcd_page_addr(guest_addr);
		}

		/* clear the fault */
		writel(AT_FRCD_F, at->reg + reg +
			fault_index * PRIMARY_FAULT_REG_LEN + 12);

		raw_spin_unlock_irqrestore(&at->register_lock, flag);

		if (!ratelimited)
			at_fault_do_one(at, type, fault_reason,
					source_id, guest_addr);

		fault_index++;
		if (fault_index >= cap_num_fault_regs(at->cap))
			fault_index = 0;
		raw_spin_lock_irqsave(&at->register_lock, flag);
	}

	writel(AT_FSTS_PFO | AT_FSTS_PPF, at->reg + AT_FSTS_REG);

unlock_exit:
	raw_spin_unlock_irqrestore(&at->register_lock, flag);
	return IRQ_HANDLED;
}

/*
 * Reclaim all the submitted descriptors which have completed its work.
 */
static inline void reclaim_free_desc(struct q_inval *qi)
{
	while (qi->desc_status[qi->free_tail] == QI_DONE ||
	       qi->desc_status[qi->free_tail] == QI_ABORT) {
		qi->desc_status[qi->free_tail] = QI_FREE;
		qi->free_tail = (qi->free_tail + 1) % QI_LENGTH;
		qi->free_cnt++;
	}
}

static int qi_check_fault(struct hfi_at *at, int index)
{
	u32 fault;
	int head, tail;
	struct q_inval *qi = at->qi;
	int wait_index = (index + 1) % QI_LENGTH;

	if (qi->desc_status[wait_index] == QI_ABORT)
		return -EAGAIN;

	fault = readl(at->reg + AT_FSTS_REG);

	/*
	 * If IQE happens, the head points to the descriptor associated
	 * with the error. No new descriptors are fetched until the IQE
	 * is cleared.
	 */
	if (fault & AT_FSTS_IQE) {
		head = readl(at->reg + AT_IQH_REG);
		if ((head >> AT_IQ_SHIFT) == index) {
			dd_dev_err(at->dd,
				   "AT detected invalid descriptor: low=%llx, high=%llx\n",
				   (unsigned long long)qi->desc[index].low,
				   (unsigned long long)qi->desc[index].high);
			memcpy(&qi->desc[index], &qi->desc[wait_index],
			       sizeof(struct qi_desc));
			writel(AT_FSTS_IQE, at->reg + AT_FSTS_REG);
			return -EINVAL;
		}
	}

	/*
	 * If ITE happens, all pending wait_desc commands are aborted.
	 * No new descriptors are fetched until the ITE is cleared.
	 */
	if (fault & AT_FSTS_ITE) {
		head = readl(at->reg + AT_IQH_REG);
		head = ((head >> AT_IQ_SHIFT) - 1 + QI_LENGTH) % QI_LENGTH;
		head |= 1;
		tail = readl(at->reg + AT_IQT_REG);
		tail = ((tail >> AT_IQ_SHIFT) - 1 + QI_LENGTH) % QI_LENGTH;

		writel(AT_FSTS_ITE, at->reg + AT_FSTS_REG);

		do {
			if (qi->desc_status[head] == QI_IN_USE)
				qi->desc_status[head] = QI_ABORT;
			head = (head - 2 + QI_LENGTH) % QI_LENGTH;
		} while (head != tail);

		if (qi->desc_status[wait_index] == QI_ABORT)
			return -EAGAIN;
	}

	if (fault & AT_FSTS_ICE)
		writel(AT_FSTS_ICE, at->reg + AT_FSTS_REG);

	return 0;
}

/*
 * Submit the queued invalidation descriptor to the remapping
 * hardware unit and wait for its completion.
 */
static int qi_submit_sync(struct qi_desc *desc, struct hfi_at *at)
{
	int rc;
	struct q_inval *qi = at->qi;
	struct qi_desc *hw, wait_desc;
	int wait_index, index;
	unsigned long flags;

	if (!qi)
		return 0;

	hw = qi->desc;

restart:
	rc = 0;

	raw_spin_lock_irqsave(&qi->q_lock, flags);
	while (qi->free_cnt < 3) {
		raw_spin_unlock_irqrestore(&qi->q_lock, flags);
		cpu_relax();
		raw_spin_lock_irqsave(&qi->q_lock, flags);
	}

	index = qi->free_head;
	wait_index = (index + 1) % QI_LENGTH;

	qi->desc_status[index] = qi->desc_status[wait_index] = QI_IN_USE;

	hw[index] = *desc;

	wait_desc.low = QI_IWD_STATUS_DATA(QI_DONE) |
			QI_IWD_STATUS_WRITE | QI_IWD_TYPE;
	wait_desc.high = (unsigned long)qi->desc_status_dma +
			 (wait_index * sizeof(int));

	hw[wait_index] = wait_desc;

	qi->free_head = (qi->free_head + 2) % QI_LENGTH;
	qi->free_cnt -= 2;

	/*
	 * update the HW tail register indicating the presence of
	 * new descriptors.
	 */
	writel(qi->free_head << AT_IQ_SHIFT, at->reg + AT_IQT_REG);

	while (qi->desc_status[wait_index] != QI_DONE) {
		/*
		 * We will leave the interrupts disabled, to prevent interrupt
		 * context to queue another cmd while a cmd is already submitted
		 * and waiting for completion on this cpu. This is to avoid
		 * a deadlock where the interrupt context can wait indefinitely
		 * for free slots in the queue.
		 */
		rc = qi_check_fault(at, index);
		if (rc)
			break;

		raw_spin_unlock(&qi->q_lock);
		cpu_relax();
		raw_spin_lock(&qi->q_lock);
	}

	qi->desc_status[index] = QI_DONE;

	reclaim_free_desc(qi);
	raw_spin_unlock_irqrestore(&qi->q_lock, flags);

	if (rc == -EAGAIN)
		goto restart;

	return rc;
}

#define PRQ_RING_MASK ((0x1000 << PRQ_ORDER) - 0x10)

static inline void *alloc_pgtable_page(int node)
{
	struct page *page;
	void *vaddr = NULL;

	page = alloc_pages_node(node, GFP_KERNEL | __GFP_ZERO, 0);
	if (page) {
		vaddr = page_address(page);
		hfi2_dbg("AT page alloc: %p\n", vaddr);
	}
	return vaddr;
}

static inline void free_pgtable_page(void *vaddr)
{
	hfi2_dbg("AT page free: %p\n", vaddr);
	free_page((unsigned long)vaddr);
}

static bool is_canonical_address(u64 addr)
{
	int shift = 64 - (__VIRTUAL_MASK_SHIFT + 1);
	long saddr = (long)addr;

	return (((saddr << shift) >> shift) == saddr);
}

static u64 make_canonical_address(u64 addr)
{
	int shift = 64 - (__VIRTUAL_MASK_SHIFT + 1);
	long saddr = (long)addr;

	return (u64)((saddr << shift) >> shift);
}

static bool access_error(struct vm_area_struct *vma, struct page_req_dsc *req)
{
	unsigned long requested = 0;

	if (req->exe_req)
		requested |= VM_EXEC;

	if (req->rd_req)
		requested |= VM_READ;

	if (req->wr_req)
		requested |= VM_WRITE;

	return (requested & ~vma->vm_flags) != 0;
}

#define __HFI_AT_MAX_PFN(gaw)  ((((u64)1) << (gaw - AT_PAGE_SHIFT)) - 1)
#define HFI_AT_MAX_PFN(gaw)    ((unsigned long)min_t(u64, \
				__HFI_AT_MAX_PFN(gaw), (unsigned long)-1))

#define HFI_AT_DEFAULT_MAX_PFN   HFI_AT_MAX_PFN(DEFAULT_ADDRESS_WIDTH)

#define AT_PFN(addr)    ((addr) >> AT_PAGE_SHIFT)

/* Returns a number of AT pages, but aligned to MM page size */
static inline unsigned long aligned_nrpages(unsigned long host_addr,
					    size_t size)
{
	host_addr &= ~PAGE_MASK;
	return PAGE_ALIGN(host_addr + size) >> AT_PAGE_SHIFT;
}

static inline int agaw_to_level(int agaw)
{
	return agaw + 2;
}

static inline int width_to_agaw(int width)
{
	return DIV_ROUND_UP(width - 30, LEVEL_STRIDE);
}

static inline unsigned int level_to_offset_bits(int level)
{
	return (level - 1) * LEVEL_STRIDE;
}

static inline int pfn_level_offset(unsigned long pfn, int level)
{
	return (pfn >> level_to_offset_bits(level)) & LEVEL_MASK;
}

static inline unsigned long level_mask(int level)
{
	return -1UL << level_to_offset_bits(level);
}

static inline unsigned long level_size(int level)
{
	return 1UL << level_to_offset_bits(level);
}

static inline void at_clear_pte(struct at_pte *pte)
{
	pte->val = 0;
}

static inline bool at_pte_superpage(struct at_pte *pte)
{
	return (pte->val & AT_PTE_LARGE_PAGE);
}

static inline int first_pte_in_page(struct at_pte *pte)
{
	return !((unsigned long)pte & ~AT_PAGE_MASK);
}

static inline bool at_pte_present(struct at_pte *pte)
{
	return (pte->val & AT_PTE_PRESENT) != 0;
}

static inline u64 at_pte_addr(struct at_pte *pte)
{
	return pte->val & (AT_PAGE_MASK ^ AT_PTE_EXEC_DIS);
}

static inline unsigned long mm_to_at_pfn(unsigned long mm_pfn)
{
	return mm_pfn << (PAGE_SHIFT - AT_PAGE_SHIFT);
}

static inline u64 virt_to_at_pfn(void *p)
{
	return (u64)mm_to_at_pfn(page_to_pfn(virt_to_page(p)));
}

static inline u64 iova_to_at_pfn(dma_addr_t iova)
{
	return (u64)mm_to_at_pfn(iova >> PAGE_SHIFT);
}

static inline void __at_flush_cache(struct hfi_at *at, void *addr, int size)
{
	if (!ecap_coherent(at->ecap))
		clflush_cache_range(addr, size);
}

static inline int at_pte_get_promo(int level)
{
	if (!level || level > MAX_PGTBL_LEVEL)
		return 0;

	return SZ_4K << ((level - 1) * LEVEL_STRIDE);
}

static void __print_page_tbl(struct seq_file *s, struct at_pte *pte,
			     u8 level, unsigned long pfn)
{
	char *lp, *lvl_prefix[MAX_PGTBL_LEVEL] = { "\t\t\t\t", "\t\t\t",
						   "\t\t", "\t", "" };
	int i = pfn ? pfn_level_offset(pfn, level) : 0;

	lp = lvl_prefix[level - 1];
	pte += i;
	do {
		if (at_pte_present(pte)) {
			u64 pte_val = at_pte_addr(pte);

			seq_printf(s, "%s0x%x: 0x%llx\n", lp, i, pte->val);
			if (level > 1 && !at_pte_superpage(pte))
				__print_page_tbl(s, phys_to_virt(pte_val),
						 level - 1, pfn);
		} else if (pfn) {
			seq_printf(s, "%s0x%x: 0x%llx\n", lp, i, pte->val);
		}

		if (pfn)
			break;

		pte++;
		i++;
	} while (!first_pte_in_page(pte));
}

static void print_page_tbl(struct seq_file *s, struct hfi_at_svm *svm)
{
	__print_page_tbl(s, svm->pgd, agaw_to_level(svm->at->agaw), 0);
}

static bool bad_address(void *p)
{
	unsigned long dummy;

	if (!p)
		return true;

	return probe_kernel_address((unsigned long *)p, dummy);
}

/* TODO: Check if tracing is enabled before walking page table */
/* TODO: Probably we should avoid using CPU page table macros here */
static void at_dump_pagetable(pgd_t *p, unsigned long address, char *prefix)
{
	pgd_t *base, *pgd;
	bool pgd_v = false;
	bool p4d_v = false;
	bool pud_v = false;
	bool pmd_v = false;
	bool pte_v = false;
	bool v = false;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;

	base = p ? p : __va(read_cr3_pa());
	pgd = base + pgd_index(address);
	if (bad_address(pgd))
		goto bad;

	pgd_v = true;
	if (!pgd_present(*pgd))
		goto out;

	p4d = p4d_offset(pgd, address);
	if (bad_address(p4d))
		goto bad;

	p4d_v = true;
	if (!p4d_present(*p4d) || p4d_large(*p4d))
		goto out;

	pud = pud_offset(p4d, address);
	if (bad_address(pud))
		goto bad;

	pud_v = true;
	if (!pud_present(*pud) || pud_large(*pud))
		goto out;

	pmd = pmd_offset(pud, address);
	if (bad_address(pmd))
		goto bad;

	pmd_v = true;
	if (!pmd_present(*pmd) || pmd_large(*pmd))
		goto out;

	pte = pte_offset_kernel(pmd, address);
	if (bad_address(pte))
		goto bad;

	pte_v = true;
out:
	v = true;
bad:
	hfi2_dbg("%s %s addr 0x%lx PGD %03x:%12lx P4D %03x:%12lx PUD %03x:%12lx PMD %03x:%12lx PTE %03x:%12lx\n",
		 prefix, v ? "" : "(ERROR)", address,
		 pgd_index(address), pgd_v ? pgd_val(*pgd) : 0,
		 p4d_index(address), p4d_v ? p4d_val(*p4d) : 0,
		 pud_index(address), pud_v ? pud_val(*pud) : 0,
		 pmd_index(address), pmd_v ? pmd_val(*pmd) : 0,
		 pte_index(address), pte_v ? pte_val(*pte) : 0);
}

static struct at_pte *pfn_to_at_pte(struct hfi_at_svm *svm, unsigned long pfn,
				    int *target_level, bool user)
{
	struct at_pte *parent = svm->pgd, *pte = NULL;
	int level = agaw_to_level(svm->at->agaw);
	int offset;

	if (!parent)
		return NULL;

	while (1) {
		void *tmp_page;

		offset = pfn_level_offset(pfn, level);
		pte = &parent[offset];
		if (!*target_level &&
		    (at_pte_superpage(pte) || !at_pte_present(pte)))
			break;
		if (level == *target_level)
			break;

		if (!at_pte_present(pte)) {
			u64 pteval;

			tmp_page = alloc_pgtable_page(svm->at->dd->node);

			if (!tmp_page)
				return NULL;

			__at_flush_cache(svm->at, tmp_page, AT_PAGE_SIZE);
			pteval = virt_to_at_pfn(tmp_page) << AT_PAGE_SHIFT;
			pteval |= AT_PTE_ACCESSED | AT_PTE_DIRTY |
				  AT_PTE_WRITE | AT_PTE_PRESENT;
			if (user)
				pteval |= AT_PTE_USER;

			if (cmpxchg64(&pte->val, 0ULL, pteval))
				/*
				 * Someone else set it while we were thinking;
				 * use theirs.
				 */
				free_pgtable_page(tmp_page);
			else
				__at_flush_cache(svm->at, pte, sizeof(*pte));
		}
		if (level == 1)
			break;

		parent = phys_to_virt(at_pte_addr(pte));
		level--;
	}

	if (!*target_level)
		*target_level = level;

	return pte;
}

static inline pgd_t *get_pti_user_pgdp(pgd_t *pgdp)
{
#ifdef CONFIG_PAGE_TABLE_ISOLATION
	if (static_cpu_has(X86_FEATURE_PTI))
		return kernel_to_user_pgdp(pgdp);
#endif

	return pgdp;
}

static unsigned long get_cpt_entry(pgd_t *pgdp, unsigned long address,
				   int *target_level)
{
	int level = MAX_PGTBL_LEVEL;
	unsigned long pteval = 0;
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;

	pgd = pgdp + pgd_index(address);
	if (bad_address(pgd) || !pgd_present(*pgd))
		return 0;

	level--;
	p4d = p4d_offset(pgd, address);
	if (bad_address(p4d) || !p4d_present(*p4d))
		return 0;

	if (p4d_large(*p4d)) {
		pteval = p4d_val(*p4d);
		goto out;
	}

	level--;
	pud = pud_offset(p4d, address);
	if (bad_address(pud) || !pud_present(*pud))
		return 0;

	if (pud_large(*pud)) {
		pteval = pud_val(*pud);
		goto out;
	}

	level--;
	pmd = pmd_offset(pud, address);
	if (bad_address(pmd) || !pmd_present(*pmd))
		return 0;

	if (pmd_large(*pmd)) {
		pteval = pmd_val(*pmd);
		goto out;
	}

	level--;
	pte = pte_offset_kernel(pmd, address);
	if (bad_address(pte) || !pte_present(*pte))
		return 0;

	pteval = pte_val(*pte);
out:
	if (target_level)
		*target_level = level;

	return pteval;
}

static int hfi_at_map(struct hfi_at_svm *svm, struct page_req_dsc *req,
		      struct page *page, bool user)
{
	u64 address = (u64)req->addr << AT_PAGE_SHIFT;
	struct hfi_devdata *dd = svm->at->dd;
	unsigned long phys_pfn, flags;
	struct at_pte *pte;
	u64 pteval, prot;
	dma_addr_t iova;
	int dir, level = 1, ret = 0;

	spin_lock_irqsave(&svm->lock, flags);

	/* TODO: is iova alignment guaranteed? */
	dir = req->wr_req ? DMA_BIDIRECTIONAL : DMA_TO_DEVICE;
	iova = pci_map_page(dd->pdev, page, 0, PAGE_SIZE, dir);
	if (unlikely(pci_dma_mapping_error(dd->pdev, iova))) {
		ret = -ENOSPC;
		goto out;
	}

	phys_pfn = iova_to_at_pfn(iova);
	pte = pfn_to_at_pte(svm, req->addr, &level, user);
	if (!pte) {
		pci_unmap_page(dd->pdev, iova, PAGE_SIZE, dir);
		ret = -ENOMEM;
		goto out;
	}

	prot = AT_PTE_ACCESSED | AT_PTE_PRESENT;
	if (req->wr_req)
		prot |= AT_PTE_WRITE | AT_PTE_DIRTY;
	if (user)
		prot |= AT_PTE_USER;

	pteval = ((phys_addr_t)phys_pfn << AT_PAGE_SHIFT) | prot;
	if (cmpxchg64_local(&pte->val, 0ULL, pteval)) {
		u64 tmp = pteval & ~(AT_PTE_WRITE | AT_PTE_DIRTY);

		/* allow upgrade of access privilege */
		if (cmpxchg64_local(&pte->val, tmp, pteval) != tmp) {
			pr_debug("Mapping already set\n");
			svm->stats->prq_dup++;
		}
	}
	__at_flush_cache(svm->at, pte, sizeof(*pte));

	at_dump_pagetable((pgd_t *)svm->pgd, address, "SPT");
	/* TODO: investigate necessity of compound_head() call */
	if (req->wr_req) {
		struct page *head_page = compound_head(page);

		set_page_dirty_lock(head_page);
	}

out:
	spin_unlock_irqrestore(&svm->lock, flags);
	return ret;
}

static int hfi_at_map_promo(struct hfi_at_svm *svm, struct page_req_dsc *req,
			    bool user)
{
	pgd_t *pgd_ref = user ? get_pti_user_pgdp(svm->mm->pgd)
			 : svm->at->system_mm->pgd;
	u64 address = (u64)req->addr << AT_PAGE_SHIFT;
	struct hfi_devdata *dd = svm->at->dd;
	u64 pteval, phys_address;
	unsigned long phys_pfn, flags;
	int dir, promo, level, ret = 0;
	struct at_pte *pte;
	struct page *page;
	dma_addr_t iova;

	spin_lock_irqsave(&svm->lock, flags);

	pteval = get_cpt_entry(pgd_ref, address, &level);
	if (!pteval) {
		ret = -EAGAIN;
		goto out;
	}

	promo = at_pte_get_promo(level);
	if (!promo) {
		ret = -EINVAL;
		goto out;
	}

	/* TODO: is iova alignment guaranteed? */
	phys_address = pteval & ~((promo - 1) | AT_PTE_EXEC_DIS);
	page = pfn_to_page(phys_address >> PAGE_SHIFT);
	dir = req->wr_req ? DMA_BIDIRECTIONAL : DMA_TO_DEVICE;
	iova = pci_map_page(dd->pdev, page, 0, promo, dir);
	if (unlikely(pci_dma_mapping_error(dd->pdev, iova))) {
		ret = -ENOSPC;
		goto out;
	}

	phys_pfn = iova_to_at_pfn(iova);
	pte = pfn_to_at_pte(svm, req->addr, &level, user);
	if (!pte) {
		pci_unmap_page(dd->pdev, iova, promo, dir);
		ret = -ENOMEM;
		goto out;
	}

	pteval = ((phys_addr_t)phys_pfn << AT_PAGE_SHIFT) |
		 (pteval & ((promo - 1) | AT_PTE_EXEC_DIS));
	if (cmpxchg64_local(&pte->val, 0ULL, pteval)) {
		u64 tmp = pteval & ~(AT_PTE_WRITE | AT_PTE_DIRTY);

		/* TODO: seems like page promotion will have issues here */
		/* allow upgrade of access privilege */
		if (cmpxchg64_local(&pte->val, tmp, pteval) != tmp) {
			pr_debug("Mapping already set\n");
			svm->stats->prq_dup++;
		}
	}
	__at_flush_cache(svm->at, pte, sizeof(*pte));
	at_dump_pagetable((pgd_t *)svm->pgd, address, "SPT");
	at_dump_pagetable(pgd_ref,  address, "CPT");

out:
	spin_unlock_irqrestore(&svm->lock, flags);
	return ret;
}

static int cpt_page_faultin(struct hfi_at_svm *svm,
			    struct page_req_dsc *req)
{
	u64 address = (u64)req->addr << AT_PAGE_SHIFT;
	struct vm_area_struct *vma;
	int ret = -ENOMEM;

	vma = find_extend_vma(svm->mm, address);
	if (!vma || address < vma->vm_start)
		return ret;

	if (access_error(vma, req))
		return ret;

	ret = handle_mm_fault(vma, address,
			      req->wr_req ? FAULT_FLAG_WRITE : 0);
	if (ret & VM_FAULT_ERROR)
		return ret;

	return 0;
}

static int handle_at_cpt_page_fault(struct hfi_at_svm *svm,
				    struct page_req_dsc *req)
{
	u64 address = (u64)req->addr << AT_PAGE_SHIFT;
	int ret;

	/* If address is not canonical, return invalid response */
	if (!is_canonical_address(address))
		return -EINVAL;

	/* If the mm is already defunct, don't handle faults. */
	if (!mmget_not_zero(svm->mm))
		return -EINVAL;
	down_read(&svm->mm->mmap_sem);

	ret = cpt_page_faultin(svm, req);

	up_read(&svm->mm->mmap_sem);
	mmput(svm->mm);
	return ret;
}

static int handle_at_spt_page_fault(struct hfi_at_svm *svm,
				    struct page_req_dsc *req, struct page *page)
{
	u64 address = (u64)req->addr << AT_PAGE_SHIFT;
	int ret;

	/* TODO: investigate mapping requests with null pgd */
	if (!svm->pgd) {
		dd_dev_err(svm->at->dd, "null pgd pasid %d\n", req->pasid);
		return -ENOMEM;
	}

	hfi2_dbg("AT SPT map: pasid %d virt pfn 0x%lx prot %s\n",
		 req->pasid, req->addr, req->wr_req ? "rw" : "r");

	if (svm->mm) {
		int wr;

		/* If address is not canonical, return invalid response */
		if (!is_canonical_address(address))
			return -EINVAL;

		/* No promotion and page is pinned */
		if (!user_page_promo && page)
			return hfi_at_map(svm, req, page, true);

		/* TODO: Check whether we really need mmget here */
		/* If the mm is already defunct, don't handle faults. */
		if (!mmget_not_zero(svm->mm))
			return -ENOMEM;
		down_read(&svm->mm->mmap_sem);

		if (user_page_promo) {
			ret = cpt_page_faultin(svm, req);
			if (!ret)
				ret = hfi_at_map_promo(svm, req, true);

			goto unlock;
		}

		wr = req->wr_req ? FOLL_WRITE | FOLL_FORCE : 0,
		ret = get_user_pages_remote(svm->tsk, svm->mm,
					    address & PAGE_MASK, 1,
					    wr, &page, NULL, NULL);
		if (ret != 1) {
			dd_dev_err(svm->at->dd, "gup error %d addr 0x%llx\n",
				   ret, address);
			ret = -ENOMEM;
			goto unlock;
		}

		ret = hfi_at_map(svm, req, page, true);
		put_page(page);

unlock:
		up_read(&svm->mm->mmap_sem);
		mmput(svm->mm);
	} else {
		address = make_canonical_address(address);
		if (kernel_page_promo) {
			req->addr = address >> AT_PAGE_SHIFT;
			ret = hfi_at_map_promo(svm, req, false);

			/* TODO: revisit later, needed for some vmalloc pages */
			/* Upon -EAGAIN, fallback on manual mapping */
			if (ret != -EAGAIN)
				return ret;
		}

		if (is_vmalloc_addr((void *)address))
			page = vmalloc_to_page((void *)address);
		else
			page = virt_to_page(address);

		ret = hfi_at_map(svm, req, page, false);
	}

	return ret;
}

static irqreturn_t prq_event_thread(int irq, void *d)
{
	struct hfi_irq_entry *me = d;
	struct hfi_at *at = me->dd->at;
	struct hfi_at_svm *svm = NULL;
	int head, tail, handled = 0;

	/*
	 * Clear PPR bit before reading head/tail registers, to
	 * ensure that we get a new interrupt if needed.
	 */
	writel(AT_PRS_PPR, at->reg + AT_PRS_REG);

	tail = at_readq(at->reg + AT_PQT_REG) & PRQ_RING_MASK;
	head = at_readq(at->reg + AT_PQH_REG) & PRQ_RING_MASK;
	while (head != tail) {
		bool use_cpt = use_user_cpt;
		struct page_req_dsc *req;
		struct qi_desc resp;
		int ret, result;
		u64 address;

		handled = 1;

		req = &at->prq[head / sizeof(*req)];

		result = QI_RESP_FAILURE;
		address = (u64)req->addr << AT_PAGE_SHIFT;
		if (!req->pasid_present) {
			dd_dev_err(at->dd,
				   "%s: Page request without PASID: %08llx %08llx\n",
				   at->name, ((unsigned long long *)req)[0],
				   ((unsigned long long *)req)[1]);
			goto bad_req;
		}

		if (!svm || svm->pasid != req->pasid) {
			rcu_read_lock();
			svm = idr_find(&at->pasid_idr, req->pasid);
			/* It *can't* go away, because the driver is not
			 * permitted to unbind the mm while any page faults
			 * are outstanding. So we only need RCU to protect
			 * the internal idr code.
			 */
			rcu_read_unlock();

			if (!svm) {
				dd_dev_err(at->dd, "%s: Page request for invalid PASID %d: %08llx %08llx\n",
					   at->name, req->pasid,
					   ((unsigned long long *)req)[0],
					   ((unsigned long long *)req)[1]);
				goto no_pasid;
			}
		}

		/* TODO: investigate PRQ requests with null address */
		if (!address) {
			dd_dev_err(at->dd, "null address for pasid %d\n",
				   req->pasid);
			goto bad_req;
		}

		/*
		 * we should never take any faults on kernel addresses
		 * when using kernel CPU page table.
		 * TODO: fix the borrowed page table problem
		 */
		if (!svm->mm) {
			use_cpt = use_kernel_cpt;

			if (likely(use_cpt))
				goto bad_req;
		}

		svm->stats->prq++;
		if (use_cpt)
			ret = handle_at_cpt_page_fault(svm, req);
		else
			ret = handle_at_spt_page_fault(svm, req, NULL);

		if (unlikely(ret)) {
			result = QI_RESP_INVALID;
			svm->stats->prq_fail++;
		} else {
			result = QI_RESP_SUCCESS;
		}
bad_req:
		svm = NULL;
no_pasid:
		if (req->lpig) {
			/* Page Group Response */
			resp.low = QI_PGRP_PASID(req->pasid) |
				QI_PGRP_DID((req->bus << 8) | req->devfn) |
				QI_PGRP_PASID_P(req->pasid_present) |
				QI_PGRP_RESP_TYPE;
			resp.high = QI_PGRP_IDX(req->prg_index) |
				QI_PGRP_PRIV(req->private) |
				QI_PGRP_RESP_CODE(result);

			qi_submit_sync(&resp, at);
		} else if (req->srr) {
			/* Page Stream Response */
			resp.low = QI_PSTRM_IDX(req->prg_index) |
				QI_PSTRM_PRIV(req->private) |
				QI_PSTRM_BUS(req->bus) |
				QI_PSTRM_PASID(req->pasid) |
				QI_PSTRM_RESP_TYPE;
			resp.high = QI_PSTRM_ADDR(address) |
				QI_PSTRM_DEVFN(req->devfn) |
				QI_PSTRM_RESP_CODE(result);

			qi_submit_sync(&resp, at);
		}

		head = (head + sizeof(*req)) & PRQ_RING_MASK;
	}

	at_writeq(at->reg + AT_PQH_REG, tail);

	return IRQ_RETVAL(handled);
}

static void at_irq_unmask(struct hfi_at *at, int reg)
{
	unsigned long flag;

	/* unmask it */
	raw_spin_lock_irqsave(&at->register_lock, flag);
	writel(0, at->reg + reg);
	/* Read a reg to force flush the post write */
	readl(at->reg + reg);
	raw_spin_unlock_irqrestore(&at->register_lock, flag);
}

int hfi_at_setup_irq(struct hfi_devdata *dd)
{
	struct hfi_irq_entry *me;
	int i, irq, ret;
	int irq_ctl[2] = { AT_FECTL_REG, AT_PECTL_REG };
	int irq_num[2] = { HFI_AT_FAULT_IRQ, HFI_AT_PRQ_IRQ };
	char *irq_name[2] = { "at_faut", "prq_event_thread" };
	irq_handler_t irq_hdl[2] = { at_fault, prq_event_thread };

	for (i = 0; i < 2; i++) {
		irq = irq_num[i];
		me = &dd->irq_entries[irq];
		me->dd = dd;
		me->intr_src = irq;

		dd_dev_info(dd, "request for msix IRQ %d:%d\n",
			    irq, pci_irq_vector(dd->pdev, irq));
		ret = request_threaded_irq(pci_irq_vector(dd->pdev, irq),
					   NULL, irq_hdl[i], IRQF_ONESHOT,
					   irq_name[i], me);
		if (ret) {
			dev_err(&dd->pdev->dev, "msix IRQ[%d] request failed %d\n",
				irq, ret);
			/* IRQ cleanup done in hfi_pci_dd_free() */
			return ret;
		}
		/* mark as in use */
		me->arg = me;

		/* unmask interrupt */
		at_irq_unmask(dd->at, irq_ctl[i]);
	}

	return 0;
}

static int hfi_svm_enable_prq(struct hfi_at *at)
{
	/* TODO: is alignment guaranteed? */
	at->prq = pci_zalloc_consistent(at->dd->pdev, PAGE_SIZE << PRQ_ORDER,
					&at->prq_dma);
	if (!at->prq) {
		dd_dev_err(at->dd, "AT: %s: Failed to allocate page request queue\n",
			   at->name);
		return -ENOMEM;
	}

	at_writeq(at->reg + AT_PQH_REG, 0ULL);
	at_writeq(at->reg + AT_PQT_REG, 0ULL);
	at_writeq(at->reg + AT_PQA_REG, at->prq_dma | PRQ_ORDER);

	return 0;
}

static int hfi_svm_finish_prq(struct hfi_at *at)
{
	if (!at->prq)
		return 0;

	at_writeq(at->reg + AT_PQH_REG, 0ULL);
	at_writeq(at->reg + AT_PQT_REG, 0ULL);
	at_writeq(at->reg + AT_PQA_REG, 0ULL);

	pci_free_consistent(at->dd->pdev, PAGE_SIZE << PRQ_ORDER,
			    at->prq, at->prq_dma);
	at->prq = NULL;

	return 0;
}

static void free_at(struct hfi_at *at)
{
	struct q_inval *qi = at->qi;

	if (at->system_mm) {
		mmdrop(at->system_mm);
		at->system_mm = NULL;
	}

	if (qi) {
		int status_size = QI_LENGTH * sizeof(int);

		pci_free_consistent(at->dd->pdev, PAGE_SIZE,
				    qi->desc, qi->desc_dma);
		pci_free_consistent(at->dd->pdev, status_size,
				    qi->desc_status, qi->desc_status_dma);
		kfree(qi);
	}

	kfree(at);
}

static int __at_calculate_agaw(struct hfi_at *at, int max_gaw)
{
	unsigned long sagaw;
	int agaw = -1;

	sagaw = cap_sagaw(at->cap);
	for (agaw = width_to_agaw(max_gaw); agaw >= 0; agaw--) {
		if (test_bit(agaw, &sagaw))
			break;
	}

	return agaw;
}

static int at_calculate_agaw(struct hfi_at *at)
{
	return __at_calculate_agaw(at, DEFAULT_ADDRESS_WIDTH);
}

static int alloc_at(struct hfi_devdata *dd)
{
	struct hfi_at *at;
	u32 ver, sts;
	int agaw = 0;
	int err;

	at = kzalloc(sizeof(*at), GFP_KERNEL);
	if (!at)
		return -ENOMEM;

	at->seq_id = dd->unit;
	sprintf(at->name, "hfi-at-%d", dd->unit);
	at->dd = dd;
	at->reg = dd->kregbase + 0x1681000 - dd->wc_off;

	err = -EINVAL;
	at->cap = at_readq(at->reg + AT_CAP_REG);
	at->ecap = at_readq(at->reg + AT_ECAP_REG);
	if (at->cap == (u64)-1 && at->ecap == (u64)-1) {
		dd_dev_err(at->dd, "AT cap/ecap return all ones\n");
		goto error;
	}

	ver = readl(at->reg + AT_VER_REG);
	dd_dev_info(dd, "%s: ver %d:%d cap %llx ecap %llx\n",
		    at->name,
		    AT_VER_MAJOR(ver), AT_VER_MINOR(ver),
		    (unsigned long long)at->cap,
		    (unsigned long long)at->ecap);

	/* TODO: Do we need agaw? seems like it is only for SLPTPTR */
	agaw = at_calculate_agaw(at);
	if (agaw < 0) {
		dd_dev_err(dd, "Cannot get a valid agaw for %s\n",
			   at->name);
		goto error;
	}
	at->agaw = agaw;

	sts = readl(at->reg + AT_GSTS_REG);
	if (sts & AT_GSTS_IRES)
		at->gcmd |= AT_GCMD_IRE;
	if (sts & AT_GSTS_TES)
		at->gcmd |= AT_GCMD_TE;
	if (sts & AT_GSTS_QIES)
		at->gcmd |= AT_GCMD_QIE;

	raw_spin_lock_init(&at->register_lock);
	spin_lock_init(&at->lock);
	dd->at = at;

	/* get system_mm for system pasid */
	if (use_kernel_cpt || kernel_page_promo) {
		struct task_struct *task;

		task = get_pid_task(find_pid_ns(1, &init_pid_ns), PIDTYPE_PID);
		if (!task || !task->mm) {
			dd_dev_err(dd, "Failed to get system_mm\n");
			goto error;
		}
		mmgrab(task->mm);
		at->system_mm = task->mm;
		put_task_struct(task);
	}

	return 0;

error:
	kfree(at);
	return err;
}

/*
 * Enable queued invalidation.
 */
static void __at_enable_qi(struct hfi_at *at)
{
	u32 sts;
	unsigned long flags;
	struct q_inval *qi = at->qi;

	qi->free_head = qi->free_tail = 0;
	qi->free_cnt = QI_LENGTH;

	raw_spin_lock_irqsave(&at->register_lock, flags);

	/* write zero to the tail reg */
	writel(0, at->reg + AT_IQT_REG);

	at_writeq(at->reg + AT_IQA_REG, qi->desc_dma);

	at->gcmd |= AT_GCMD_QIE;
	writel(at->gcmd, at->reg + AT_GCMD_REG);

	/* Make sure hardware complete it */
	AT_WAIT_OP(at, AT_GSTS_REG, readl, (sts & AT_GSTS_QIES), sts);

	raw_spin_unlock_irqrestore(&at->register_lock, flags);
}

/*
 * Enable Queued Invalidation interface.
 */
static int at_enable_qi(struct hfi_at *at)
{
	struct q_inval *qi;
	int status_size = QI_LENGTH * sizeof(int);

	at->qi = kmalloc(sizeof(*qi), GFP_KERNEL);
	if (!at->qi)
		return -ENOMEM;

	qi = at->qi;

	qi->desc = pci_zalloc_consistent(at->dd->pdev, PAGE_SIZE,
					 &qi->desc_dma);
	if (!qi->desc) {
		kfree(qi);
		at->qi = NULL;
		return -ENOMEM;
	}

	qi->desc_status = pci_zalloc_consistent(at->dd->pdev, status_size,
						&qi->desc_status_dma);
	if (!qi->desc_status) {
		pci_free_consistent(at->dd->pdev, PAGE_SIZE,
				    qi->desc, qi->desc_dma);
		kfree(qi);
		at->qi = NULL;
		return -ENOMEM;
	}

	raw_spin_lock_init(&qi->q_lock);

	__at_enable_qi(at);

	return 0;
}

static void qi_flush_context(struct hfi_at *at, u16 did, u16 sid, u8 fm,
			     u64 type)
{
	struct qi_desc desc;

	desc.low = QI_CC_FM(fm) | QI_CC_SID(sid) | QI_CC_DID(did)
			| QI_CC_GRAN(type) | QI_CC_TYPE;
	desc.high = 0;

	qi_submit_sync(&desc, at);
}

static void qi_flush_iotlb(struct hfi_at *at, u16 did, u64 addr,
			   unsigned int size_order, u64 type)
{
	u8 dw = 0, dr = 0;

	struct qi_desc desc;
	int ih = 0;

	if (cap_write_drain(at->cap))
		dw = 1;

	if (cap_read_drain(at->cap))
		dr = 1;

	desc.low = QI_IOTLB_DID(did) | QI_IOTLB_DR(dr) | QI_IOTLB_DW(dw)
		| QI_IOTLB_GRAN(type) | QI_IOTLB_TYPE;
	desc.high = QI_IOTLB_ADDR(addr) | QI_IOTLB_IH(ih)
		| QI_IOTLB_AM(size_order);

	qi_submit_sync(&desc, at);
}

static int hfi_at_init_qi(struct hfi_at *at)
{
	int err;
	struct hfi_irq_entry me;

	if (!ecap_qis(at->ecap))
		return -ENOENT;

	/*
	 * Clear any previous faults.
	 */
	me.dd = at->dd;
	at_fault(-1, &me);

	err = at_enable_qi(at);
	if (err)
		return err;

	dd_dev_info(at->dd, "%s: Using Queued invalidation\n", at->name);

	return 0;
}

static void at_flush_write_buffer(struct hfi_at *at)
{
	u32 val;
	unsigned long flag;

	if (!cap_rwbf(at->cap))
		return;

	raw_spin_lock_irqsave(&at->register_lock, flag);
	writel(at->gcmd | AT_GCMD_WBF, at->reg + AT_GCMD_REG);

	/* Make sure hardware complete it */
	AT_WAIT_OP(at, AT_GSTS_REG,
		   readl, (!(val & AT_GSTS_WBFS)), val);

	raw_spin_unlock_irqrestore(&at->register_lock, flag);
}

static inline bool translation_status(struct hfi_at *at)
{
	return (readl(at->reg + AT_GSTS_REG) & AT_GSTS_TES);
}

static void at_enable_translation(struct hfi_at *at)
{
	u32 sts;
	unsigned long flags;

	raw_spin_lock_irqsave(&at->register_lock, flags);
	at->gcmd |= AT_GCMD_TE;
	writel(at->gcmd, at->reg + AT_GCMD_REG);

	/* Make sure hardware complete it */
	AT_WAIT_OP(at, AT_GSTS_REG,
		   readl, (sts & AT_GSTS_TES), sts);

	raw_spin_unlock_irqrestore(&at->register_lock, flags);
}

static void at_disable_translation(struct hfi_at *at)
{
	u32 sts;
	unsigned long flag;

	raw_spin_lock_irqsave(&at->register_lock, flag);
	at->gcmd &= ~AT_GCMD_TE;
	writel(at->gcmd, at->reg + AT_GCMD_REG);

	/* Make sure hardware complete it */
	AT_WAIT_OP(at, AT_GSTS_REG,
		   readl, (!(sts & AT_GSTS_TES)), sts);

	raw_spin_unlock_irqrestore(&at->register_lock, flag);
}

static void at_disable_protect_mem_regions(struct hfi_at *at)
{
	u32 pmen;
	unsigned long flags;

	raw_spin_lock_irqsave(&at->register_lock, flags);
	pmen = readl(at->reg + AT_PMEN_REG);
	pmen &= ~AT_PMEN_EPM;
	writel(pmen, at->reg + AT_PMEN_REG);

	/* wait for the protected region status bit to clear */
	AT_WAIT_OP(at, AT_PMEN_REG,
		   readl, !(pmen & AT_PMEN_PRS), pmen);

	raw_spin_unlock_irqrestore(&at->register_lock, flags);
}

static int at_alloc_root_entry(struct hfi_at *at)
{
	struct hfi_devdata *dd = at->dd;
	unsigned long flags;

	spin_lock_irqsave(&at->lock, flags);
	at->root_entry = pci_zalloc_consistent(dd->pdev, ROOT_SIZE,
					       &at->root_entry_dma);
	if (!at->root_entry) {
		spin_unlock_irqrestore(&at->lock, flags);
		dd_dev_err(dd, "Allocating root entry for %s failed\n",
			   at->name);
		return -ENOMEM;
	}

	__at_flush_cache(at, at->root_entry, ROOT_SIZE);
	spin_unlock_irqrestore(&at->lock, flags);

	return 0;
}

static void at_set_root_entry(struct hfi_at *at)
{
	u64 addr;
	u32 sts;
	unsigned long flag;

	addr = at->root_entry_dma | AT_RTADDR_RTT;

	raw_spin_lock_irqsave(&at->register_lock, flag);
	at_writeq(at->reg + AT_RTADDR_REG, addr);

	writel(at->gcmd | AT_GCMD_SRTP, at->reg + AT_GCMD_REG);

	/* Make sure hardware complete it */
	AT_WAIT_OP(at, AT_GSTS_REG,
		   readl, (sts & AT_GSTS_RTPS), sts);

	raw_spin_unlock_irqrestore(&at->register_lock, flag);
}

static inline struct context_entry *at_context_addr(struct hfi_at *at,
						    int alloc)
{
	struct root_entry *root;
	struct context_entry *context;
	unsigned long phy_addr;
	dma_addr_t context_dma;
	u8 devfn = at->devfn;
	u64 *entry;

	if (!at->root_entry)
		return NULL;

	root = &at->root_entry[at->bus];
	entry = &root->lo;
	if (devfn >= 0x80) {
		devfn -= 0x80;
		entry = &root->hi;
	}
	devfn *= 2;

	if (at->context)
		return &at->context[devfn];

	if (!alloc)
		return NULL;

	context = pci_zalloc_consistent(at->dd->pdev, CONTEXT_SIZE,
					&context_dma);
	if (!context)
		return NULL;

	at->context = context;
	at->context_dma = context_dma;
	__at_flush_cache(at, (void *)context, CONTEXT_SIZE);
	phy_addr = context_dma;
	*entry = phy_addr | 1;
	__at_flush_cache(at, entry, sizeof(*entry));

	return &context[devfn];
}

static void free_context_table(struct hfi_at *at)
{
	unsigned long flags;

	spin_lock_irqsave(&at->lock, flags);
	if (!at->root_entry)
		goto out;

	if (at->context)
		pci_free_consistent(at->dd->pdev, CONTEXT_SIZE,
				    at->context, at->context_dma);

	pci_free_consistent(at->dd->pdev, ROOT_SIZE,
			    at->root_entry, at->root_entry_dma);
	at->root_entry = NULL;
out:
	spin_unlock_irqrestore(&at->lock, flags);
}

static int hfi_svm_alloc_pasid_tables(struct hfi_at *at)
{
	struct hfi_devdata *dd = at->dd;
	int order;

	/* Start at 2 because it's defined as 2^(1+PSS) */
	at->pasid_max = 2 << ecap_pss(at->ecap);

	/* we limit to 4K PASID because of hfi 4K PID */
	if (at->pasid_max > 0x1000)
		at->pasid_max = 0x1000;

	order = get_order(sizeof(struct pasid_entry) * at->pasid_max);
	at->pasid_table = pci_zalloc_consistent(dd->pdev, PAGE_SIZE << order,
						&at->pasid_table_dma);
	if (!at->pasid_table) {
		dd_dev_err(dd, "AT: %s: Failed to allocate PASID table\n",
			   at->name);
		return -ENOMEM;
	}
	dd_dev_info(dd, "%s: Allocated order %d PASID table.\n",
		    at->name, order);

	idr_init(&at->pasid_idr);
	idr_init(&at->pasid_stats_idr);

	return 0;
}

static int hfi_svm_free_pasid_tables(struct hfi_at *at)
{
	int order = get_order(sizeof(struct pasid_entry) * at->pasid_max);

	if (!at->pasid_table)
		return 0;

	pci_free_consistent(at->dd->pdev, PAGE_SIZE << order,
			    at->pasid_table, at->pasid_table_dma);
	at->pasid_table = NULL;

	idr_destroy(&at->pasid_idr);
	hfi_at_stats_cleanup(at);
	idr_destroy(&at->pasid_stats_idr);
	return 0;
}

#define MAX_NR_PASID_BITS (20)
static inline unsigned long hfi_at_get_pts(struct hfi_at *at)
{
	/*
	 * Convert ecap_pss to extend context entry pts encoding, also
	 * respect the soft pasid_max value set by the at.
	 * - number of PASID bits = ecap_pss + 1
	 * - number of PASID table entries = 2^(pts + 5)
	 * Therefore, pts = ecap_pss - 4
	 * e.g. KBL ecap_pss = 0x13, PASID has 20 bits, pts = 15
	 */
	if (ecap_pss(at->ecap) < 5)
		return 0;

	/* pasid_max is encoded as actual number of entries not the bits */
	return find_first_bit((unsigned long *)&at->pasid_max,
			MAX_NR_PASID_BITS) - 5;
}

static inline bool context_pasid_enabled(struct context_entry *context)
{
	return !!(context->lo & (1ULL << 11));
}

static inline bool context_copied(struct context_entry *context)
{
	return !!(context->hi & (1ULL << 3));
}

static inline bool __context_present(struct context_entry *context)
{
	return (context->lo & 1);
}

static inline bool context_present(struct context_entry *context)
{
	return context_pasid_enabled(context) ?
	     __context_present(context) :
	     __context_present(context) && !context_copied(context);
}

static inline void context_set_present(struct context_entry *context)
{
	context->lo |= 1;
}

static inline void context_set_fault_enable(struct context_entry *context)
{
	context->lo &= (((u64)-1) << 2) | 1;
}

static inline void context_set_translation_type(struct context_entry *context,
						unsigned long value)
{
	context->lo &= (((u64)-1) << 4) | 3;
	context->lo |= (value & 3) << 2;
}

static inline void context_set_address_width(struct context_entry *context,
					     unsigned long value)
{
	context->hi |= value & 7;
}

static inline void context_set_domain_id(struct context_entry *context,
					 unsigned long value)
{
	context->hi |= (value & ((1 << 16) - 1)) << 8;
}

static inline int context_domain_id(struct context_entry *c)
{
	return((c->hi >> 8) & 0xffff);
}

static inline void context_clear_entry(struct context_entry *context)
{
	context->lo = 0;
	context->hi = 0;
}

static int at_setup_device_context(struct hfi_devdata *dd)
{
	struct hfi_at *at = dd->at;
	struct pci_dev *pdev = dd->pdev;
	struct context_entry *context;
	u64 ctx_lo;

	at->bus = pdev->bus->number;
	at->devfn = pdev->devfn;
	at->ats_qdep = HFI_ATS_MAX_QDEP;

	context = at_context_addr(at, 1);
	if (!context)
		return -ENOMEM;

	if (context_present(context))
		return -EINVAL;

	context_clear_entry(context);
	context_set_domain_id(context, at->seq_id);

	/* TODO: we don't need to set second level page table */
	context_set_address_width(context, at->agaw);

	context_set_translation_type(context, CONTEXT_TT_DEV_IOTLB);
	context_set_fault_enable(context);
	context_set_present(context);
	__at_flush_cache(at, context, sizeof(*context));

	/*
	 * It's a non-present to present mapping. If hardware doesn't cache
	 * non-present entry we only need to flush the write-buffer. If the
	 * _does_ cache non-present entries, then it does so in the special
	 * domain #0, which we have to flush:
	 */
	if (cap_caching_mode(at->cap)) {
		qi_flush_context(at, 0,
				 (((u16)at->bus) << 8) | at->devfn,
				 AT_CCMD_MASK_NOBIT,
				 AT_CCMD_DEVICE_INVL);
		qi_flush_iotlb(at, at->seq_id,
			       0, 0, AT_TLB_DSI_FLUSH);
	} else {
		at_flush_write_buffer(at);
	}

	ctx_lo = context[0].lo;
	if (ctx_lo & CONTEXT_PASIDE)
		return -EINVAL;

	context[1].lo = (u64)at->pasid_table_dma | hfi_at_get_pts(at);

	/* barrier context[1] entry */
	wmb();
	if ((ctx_lo & CONTEXT_TT_MASK) == (CONTEXT_TT_PASS_THROUGH << 2)) {
		ctx_lo &= ~CONTEXT_TT_MASK;
		ctx_lo |= CONTEXT_TT_PT_PASID_DEV_IOTLB << 2;
	}
	ctx_lo |= CONTEXT_PASIDE;
	ctx_lo |= CONTEXT_PRS;
	context[0].lo = ctx_lo;

	/* barrier context[0] entry */
	wmb();
	qi_flush_context(at, at->seq_id,
			 PCI_DEVID(at->bus, at->devfn),
			 AT_CCMD_MASK_NOBIT,
			 AT_CCMD_DEVICE_INVL);

	return 0;
}

static void at_context_clear(struct hfi_at *at)
{
	unsigned long flags;
	struct context_entry *context;
	u16 did_old;

	spin_lock_irqsave(&at->lock, flags);
	context = at_context_addr(at, 0);
	if (!context) {
		spin_unlock_irqrestore(&at->lock, flags);
		return;
	}
	did_old = context_domain_id(context);
	context_clear_entry(context);
	__at_flush_cache(at, context, sizeof(*context));
	spin_unlock_irqrestore(&at->lock, flags);
	qi_flush_context(at,
			 did_old,
			 (((u16)at->bus) << 8) | at->devfn,
			 AT_CCMD_MASK_NOBIT,
			 AT_CCMD_DEVICE_INVL);
	qi_flush_iotlb(at, did_old, 0, 0,
		       AT_TLB_DSI_FLUSH);
}

static void disable_at_context(struct hfi_at *at)
{
	at_context_clear(at);

	if (at->gcmd & AT_GCMD_TE)
		at_disable_translation(at);
}

static void free_at_context(struct hfi_at *at)
{
	/* free context mapping */
	free_context_table(at);

	hfi_svm_finish_prq(at);
	hfi_svm_free_pasid_tables(at);
}

static void hfi_at_enable_svm(struct hfi_at *at, struct hfi_at_svm *svm)
{
	svm->did = at->seq_id;
	svm->sid = PCI_DEVID(at->bus, at->devfn);
	svm->qdep = at->ats_qdep;
	if (svm->qdep >= QI_DEV_EIOTLB_MAX_INVS)
		svm->qdep = 0;
}

static struct hfi_at *hfi_svm_device_to_at(struct device *dev)
{
	struct hfi_devdata *dd = pci_get_drvdata(to_pci_dev(dev));

	if (dd)
		return dd->at;
	else
		return NULL;
}

static inline void at_pte_dma_unmap(struct hfi_at_svm *svm, struct at_pte *pte,
				    int level)
{
	struct hfi_devdata *dd = svm->at->dd;
	int promo, dir;

	promo = at_pte_get_promo(level);
	if (!promo) {
		dd_dev_err(dd, "Inavlid promotion level %d\n", level);
		return;
	}

	/*
	 * TODO: Ensure mapping/unmapping same address
	 * do not overlap.
	 */
	dir = (pte->val & AT_PTE_WRITE) ? DMA_BIDIRECTIONAL : DMA_TO_DEVICE;
	pci_unmap_page(dd->pdev, at_pte_addr(pte), promo, dir);
}

static struct page *at_pte_list_pgtbls(struct hfi_at_svm *svm, int level,
				       struct at_pte *pte,
				       struct page *freelist)
{
	struct page *pg;

	pg = pfn_to_page(at_pte_addr(pte) >> PAGE_SHIFT);
	pg->freelist = freelist;
	freelist = pg;

	pte = page_address(pg);
	do {
		if (!at_pte_present(pte))
			goto clear_next;

		if (level > 1 && !at_pte_superpage(pte))
			freelist = at_pte_list_pgtbls(svm, level - 1,
						      pte, freelist);
		else
			at_pte_dma_unmap(svm, pte, level);

clear_next:
		pte++;
	} while (!first_pte_in_page(pte));

	return freelist;
}

static struct page *at_pte_clear_level(struct hfi_at_svm *svm, int level,
				       struct at_pte *pte,
				       unsigned long pfn,
				       unsigned long start_pfn,
				       unsigned long last_pfn,
				       struct page *freelist)
{
	struct at_pte *first_pte = NULL, *last_pte = NULL;

	pfn = max(start_pfn, pfn);
	pte = &pte[pfn_level_offset(pfn, level)];

	do {
		unsigned long level_pfn;

		if (!at_pte_present(pte))
			goto next;

		level_pfn = pfn & level_mask(level);
		/* If range covers entire pagetable, free it */
		if (start_pfn <= level_pfn &&
		    last_pfn >= level_pfn + level_size(level) - 1) {
			/*
			 * These suborbinate page tables are going away.
			 * Don't bother to clear them; we're just going
			 * to *free* them.
			 */
			if (level > 1 && !at_pte_superpage(pte))
				freelist = at_pte_list_pgtbls(svm, level - 1,
							      pte, freelist);
			else
				at_pte_dma_unmap(svm, pte, level);

			at_clear_pte(pte);
			if (!first_pte)
				first_pte = pte;
			last_pte = pte;
		} else if (level > 1) {
			struct at_pte *lower_pte;

			/*
			 * At any level, if the page is superpage,
			 * we can't move down to next level.
			 */
			if (at_pte_superpage(pte))
				goto next;

			/*
			 * Recurse down into a level that isn't *entirely*
			 * obsolete.
			 */
			lower_pte = phys_to_virt(at_pte_addr(pte));
			freelist = at_pte_clear_level(svm, level - 1, lower_pte,
						      level_pfn, start_pfn,
						      last_pfn, freelist);
		}
next:
		pfn += level_size(level);
	} while (!first_pte_in_page(++pte) && pfn <= last_pfn);

	++last_pte;
	if (first_pte)
		__at_flush_cache(svm->at, first_pte,
				 (void *)last_pte - (void *)first_pte);

	return freelist;
}

static struct page *hfi_at_unmap(struct hfi_at_svm *svm,
				 unsigned long start_pfn,
				 unsigned long last_pfn)
{
	struct page *freelist = NULL;
	unsigned long flags;

	if (start_pfn > last_pfn)
		return NULL;

	hfi2_dbg("AT SPT unmap: pasid %d start pfn 0x%lx last pfn 0x%lx\n",
		 svm->pasid, start_pfn, last_pfn);

	spin_lock_irqsave(&svm->lock, flags);
	if (!svm->pgd)
		goto unmap_done;

	/* we don't need lock here; nobody else touches the iova range */
	freelist = at_pte_clear_level(svm, agaw_to_level(svm->at->agaw),
				      svm->pgd, 0, start_pfn, last_pfn,
				      NULL);

	/* free pgd */
	if (start_pfn == 0 && last_pfn == HFI_AT_DEFAULT_MAX_PFN) {
		struct page *pgd_page = virt_to_page(svm->pgd);

		pgd_page->freelist = freelist;
		freelist = pgd_page;
		svm->pgd = NULL;
	}

unmap_done:
	spin_unlock_irqrestore(&svm->lock, flags);
	return freelist;
}

static void hfi_at_free_pagelist(struct page *freelist)
{
	struct page *pg;

	while ((pg = freelist)) {
		freelist = pg->freelist;
		free_pgtable_page(page_address(pg));
	}
}

void hfi_at_dereg_range(struct hfi_ctx *ctx, void *vaddr, u32 size)
{
	struct hfi_at *at = ctx->devdata->at;
	struct hfi_at_svm *svm;

	mutex_lock(&pasid_mutex);
	svm = idr_find(&at->pasid_idr, ctx->pasid);
	mutex_unlock(&pasid_mutex);
	if (!svm || !svm->pgd)
		return;

	hfi_flush_svm_range(svm, (unsigned long)vaddr,
			    (size + PAGE_SIZE - 1) >> AT_PAGE_SHIFT,
			    0, 0);
}

int hfi_at_reg_range(struct hfi_ctx *ctx, void *vaddr, u32 size,
		     struct page **pages, bool write)
{
	struct hfi_at *at = ctx->devdata->at;
	struct hfi_at_svm *svm;
	struct page *page = NULL;
	struct page_req_dsc req = { 0 };
	unsigned long pageaddr;
	unsigned long nrpages;

	mutex_lock(&pasid_mutex);
	svm = idr_find(&at->pasid_idr, ctx->pasid);
	mutex_unlock(&pasid_mutex);
	if (!svm || !svm->pgd)
		return -EINVAL;

	req.pasid = svm->pasid;
	req.wr_req = write;
	pageaddr = ((unsigned long)vaddr & PAGE_MASK) >> AT_PAGE_SHIFT;
	nrpages = aligned_nrpages((unsigned long)vaddr, size);

	while (nrpages) {
		int ret;

		req.addr = pageaddr;
		if (pages)
			page = *(pages++);

		/*
		 * TODO: handle the counter change atomically,
		 * or move the increasing down into locked
		 * function hfi_at_map(), to make thread-safe.
		 */
		svm->stats->preg++;
		ret = handle_at_spt_page_fault(svm, &req, page);
		if (ret) {
			svm->stats->preg_fail++;
			return ret;
		}

		pageaddr++;
		nrpages--;
	}

	return 0;
}

int hfi_at_prefetch(struct hfi_ctx *ctx,
		    struct hfi_at_prefetch_args *atpf)
{
	struct iovec iovec, *iovecp = (struct iovec *)atpf->iovec;
	bool writable = (atpf->flags & HFI_AT_READWRITE) ? true : false;

	while (atpf->count) {
		if (copy_from_user(&iovec, iovecp, sizeof(iovec)))
			return -EFAULT;

		hfi_at_reg_range(ctx, iovec.iov_base,
				 iovec.iov_len, NULL, writable);

		atpf->count--;
		iovecp++;
	}

	return 0;
}

static void hfi_flush_svm_range_dev(struct hfi_at_svm *svm,
				    unsigned long address,
				    unsigned long pages, int ih, int gl)
{
	struct qi_desc desc;

	if (pages == -1) {
		/*
		 * For global kernel pages we have to flush them in *all*
		 * PASIDs because that's the only option the hardware
		 * gives us. Despite the fact that they are actually only
		 * accessible through one.
		 */
		if (gl)
			desc.low = QI_EIOTLB_PASID(svm->pasid) |
				QI_EIOTLB_DID(svm->did) |
				QI_EIOTLB_GRAN(QI_GRAN_ALL_ALL) |
				QI_EIOTLB_TYPE;
		else
			desc.low = QI_EIOTLB_PASID(svm->pasid) |
				QI_EIOTLB_DID(svm->did) |
				QI_EIOTLB_GRAN(QI_GRAN_NONG_PASID) |
				QI_EIOTLB_TYPE;
		desc.high = 0;
	} else {
		int mask = ilog2(__roundup_pow_of_two(pages));

		desc.low = QI_EIOTLB_PASID(svm->pasid) |
			QI_EIOTLB_DID(svm->did) |
			QI_EIOTLB_GRAN(QI_GRAN_PSI_PASID) | QI_EIOTLB_TYPE;
		desc.high = QI_EIOTLB_ADDR(address) | QI_EIOTLB_GL(gl) |
			QI_EIOTLB_IH(ih) | QI_EIOTLB_AM(mask);
	}
	qi_submit_sync(&desc, svm->at);

	desc.low = QI_DEV_EIOTLB_PASID(svm->pasid) |
		QI_DEV_EIOTLB_SID(svm->sid) |
		QI_DEV_EIOTLB_QDEP(svm->qdep) | QI_DEIOTLB_TYPE;
	if (pages == -1) {
		desc.high = QI_DEV_EIOTLB_ADDR(-1ULL >> 1) |
			QI_DEV_EIOTLB_SIZE;
	} else if (pages > 1) {
		/*
		 * The least significant zero bit indicates the size. So,
		 * for example, an "address" value of 0x12345f000 will
		 * flush from 0x123440000 to 0x12347ffff (256KiB).
		 */
		unsigned long last = address +
				((unsigned long)(pages - 1) << AT_PAGE_SHIFT);
		unsigned long mask = __rounddown_pow_of_two(address ^ last);

		desc.high = QI_DEV_EIOTLB_ADDR((address & ~mask) |
				(mask - 1)) | QI_DEV_EIOTLB_SIZE;
	} else {
		desc.high = QI_DEV_EIOTLB_ADDR(address);
	}
	qi_submit_sync(&desc, svm->at);
}

static void hfi_flush_svm_range(struct hfi_at_svm *svm, unsigned long address,
				unsigned long pages, int ih, int gl)
{
	unsigned long start_pfn, last_pfn, nrpages;
	struct page *freelist;

	hfi_flush_svm_range_dev(svm, address, pages, ih, gl);

	if (svm->pgd) {
		nrpages = aligned_nrpages(address, pages * PAGE_SIZE);
		start_pfn = AT_PFN(address);
		last_pfn = start_pfn + nrpages - 1;
		freelist = hfi_at_unmap(svm, start_pfn, last_pfn);
		hfi_at_free_pagelist(freelist);
	}
}

static void hfi_at_change_pte(struct mmu_notifier *mn, struct mm_struct *mm,
			      unsigned long address, pte_t pte)
{
	struct hfi_at_svm *svm = container_of(mn, struct hfi_at_svm, notifier);

	hfi_flush_svm_range(svm, address, 1, 1, 0);
}

/* Pages have been freed at this point */
static void hfi_at_invalidate_range(struct mmu_notifier *mn,
				    struct mm_struct *mm,
				    unsigned long start, unsigned long end)
{
	struct hfi_at_svm *svm = container_of(mn, struct hfi_at_svm, notifier);

	hfi_flush_svm_range(svm, start,
			    (end - start + PAGE_SIZE - 1) >> AT_PAGE_SHIFT,
			    0, 0);
}

static void hfi_flush_pasid_dev(struct hfi_at_svm *svm, int pasid)
{
	struct qi_desc desc;

	desc.high = 0;
	desc.low = QI_PC_TYPE | QI_PC_DID(svm->did) |
		   QI_PC_PASID_SEL | QI_PC_PASID(pasid);

	qi_submit_sync(&desc, svm->at);
}

static void hfi_at_mm_release(struct mmu_notifier *mn, struct mm_struct *mm)
{
	struct hfi_at_svm *svm = container_of(mn, struct hfi_at_svm, notifier);
	struct page *freelist;

	/*
	 * This might end up being called from exit_mmap(), *before* the page
	 * tables are cleared. And __mmu_notifier_release() will delete us from
	 * the list of notifiers so that our invalidate_range() callback doesn't
	 * get called when the page tables are cleared. So we need to protect
	 * against hardware accessing those page tables.
	 *
	 * We do it by clearing the entry in the PASID table and then flushing
	 * the IOTLB and the PASID table caches. This might upset hardware;
	 * perhaps we'll want to point the PASID to a dummy PGD (like the zero
	 * page) so that we end up taking a fault that the hardware really
	 * *has* to handle gracefully without affecting other processes.
	 */
	svm->at->pasid_table[svm->pasid].val = 0;
	/* barrier pasid table teardown */
	wmb();

	hfi_flush_pasid_dev(svm, svm->pasid);
	hfi_flush_svm_range_dev(svm, 0, -1, 0, !svm->mm);

	if (svm->pgd) {
		freelist = hfi_at_unmap(svm, 0,	HFI_AT_DEFAULT_MAX_PFN);
		hfi_at_free_pagelist(freelist);
	}
}

static const struct mmu_notifier_ops hfi_at_mmuops = {
	.release = hfi_at_mm_release,
	.change_pte = hfi_at_change_pte,
	.invalidate_range = hfi_at_invalidate_range,
};

static void hfi_at_stats_cleanup(struct hfi_at *at)
{
	struct hfi_at_stats *stats;
	int i;

	mutex_lock(&pasid_mutex);
	idr_for_each_entry(&at->pasid_stats_idr, stats, i) {
		idr_remove(&at->pasid_stats_idr, i);
		kfree(stats);
	}
	mutex_unlock(&pasid_mutex);
}

static int hfi_at_stats_show(struct seq_file *s, void *unused)
{
	struct hfi_devdata *dd = s->private;
	struct hfi_at *at = dd->at;
	struct hfi_at_stats *stats;
	int i;

	seq_puts(s, "pasid: prq duplicate fail pre_reg pre_reg_fail\n");
	mutex_lock(&pasid_mutex);
	idr_for_each_entry(&at->pasid_stats_idr, stats, i)
		seq_printf(s, "%d: %llu %llu %llu %llu %llu\n",
			   stats->pasid, stats->prq, stats->prq_dup,
			   stats->prq_fail, stats->preg, stats->preg_fail);
	mutex_unlock(&pasid_mutex);

	return 0;
}

static ssize_t hfi_at_stats_write(struct file *file, const char __user *buf,
				  size_t count, loff_t *ppos)
{
	struct hfi_devdata *dd = private2dd(file);
	struct hfi_at *at = dd->at;
	struct hfi_at_stats *stats;
	int i;

	mutex_lock(&pasid_mutex);
	idr_for_each_entry(&at->pasid_stats_idr, stats, i) {
		stats->preg = 0;
		stats->preg_fail = 0;
		stats->prq = 0;
		stats->prq_dup = 0;
		stats->prq_fail = 0;
	}
	mutex_unlock(&pasid_mutex);

	return count;
}

static int hfi_at_page_tbl_show(struct seq_file *s, void *unused)
{
	struct hfi_ctx *ctx = s->private;
	struct hfi_devdata *dd = ctx->devdata;
	struct hfi_at *at = dd->at;
	struct hfi_at_svm *svm;

	mutex_lock(&pasid_mutex);
	svm = idr_find(&at->pasid_idr, ctx->pasid);
	if (svm && svm->pgd)
		print_page_tbl(s, svm);

	mutex_unlock(&pasid_mutex);
	return 0;
}

DEBUGFS_FILE_OPS_SINGLE_WITH_WRITE(at_stats);
DEBUGFS_FILE_OPS_SINGLE(at_page_tbl);

void hfi_at_dbg_init(struct hfi_devdata *dd)
{
	dd->hfi_at_dbg = debugfs_create_dir("at", dd->hfi_dev_dbg);

	debugfs_create_file("stats", 0644, dd->hfi_at_dbg, dd,
			    &hfi_at_stats_ops);
}

void hfi_at_ctx_dbg_init(struct hfi_ctx *ctx)
{
	struct hfi_devdata *dd = ctx->devdata;
	bool use_cpt = (ctx->type == HFI_CTX_TYPE_USER) ? use_user_cpt :
							  use_kernel_cpt;

	if (use_cpt)
		return;

	ctx->dbg = debugfs_create_dir(ctx->pasid_str, dd->hfi_at_dbg);
	debugfs_create_file("page_tbl", 0444, ctx->dbg,
			    ctx, &hfi_at_page_tbl_ops);
}

static struct hfi_at_stats *hfi_at_alloc_stats(struct hfi_at_svm *svm)
{
	struct hfi_at_stats *stats;
	int ret;

	stats = idr_find(&svm->at->pasid_stats_idr, svm->pasid);
	if (!stats) {
		stats = kmalloc(sizeof(*stats), GFP_KERNEL);
		if (!stats)
			return NULL;

		ret = idr_alloc(&svm->at->pasid_stats_idr, stats,
				svm->pasid, svm->pasid + 1, GFP_KERNEL);
		if (ret < 0) {
			kfree(stats);
			return NULL;
		}
	}
	stats->preg = 0;
	stats->preg_fail = 0;
	stats->prq = 0;
	stats->prq_dup = 0;
	stats->prq_fail = 0;
	stats->pasid = svm->pasid;

	return stats;
}

static void hfi_at_drain_pasid(struct hfi_at_svm *svm, int pasid)
{
	struct hfi_devdata *dd = svm->at->dd;
	u64 drain_pasid;
	int time, ms;

	if (dd->system_pasid == pasid)
		write_csr(dd, FXR_AT_CFG_USE_SYSTEM_PASID, 0);

	spin_lock(&dd->ptl_lock);

	/* drain pasid before freeing it */
	drain_pasid = ((FXR_AT_CFG_DRAIN_PASID_PASID_MASK & pasid) <<
		       FXR_AT_CFG_DRAIN_PASID_PASID_SHIFT) |
		FXR_AT_CFG_DRAIN_PASID_BUSY_SMASK;
	write_csr(dd, FXR_AT_CFG_DRAIN_PASID, drain_pasid);

	/* wait for completion, if timeout log a message */
	ms = 1;
	for (time = 0; time < HFI_PASID_DRAIN_TIMEOUT_MS; time += ms) {
		drain_pasid = read_csr(dd, FXR_AT_CFG_DRAIN_PASID);

		if (!(drain_pasid & FXR_AT_CFG_DRAIN_PASID_BUSY_SMASK))
			break;

		mdelay(ms);
		ms *= 2;	/* double the waiting time */
	}

	if (time >= HFI_PASID_DRAIN_TIMEOUT_MS)
		dd_dev_err(dd, "PASID draining not done after %dms, pasid %d\n",
			   time, pasid);

	/*
	 * TODO - Check whether we really need this mini-TLB flush any more.
	 * Write fake simics CSR to flush mini-TLB (AT interface TBD)
	 * If mini-TLB is not invalidated it may lead to packet drops
	 * or kernel crashes due to use of stale hardware addresses.
	 * See simics src:
	 *   constant HIARB_SIMICS_S_ADDR    =0x10010;
	 *   register FLUSH_TLB size 8 @ (HIARB_SIMICS_S_ADDR+0x000)
	 */
	write_csr(dd, FXR_RX_HIARB_CSRS + 0x10010, 1);

	spin_unlock(&dd->ptl_lock);
}

static int hfi_svm_bind_mm(struct device *dev, int *pasid, int flags)
{
	struct hfi_at *at = hfi_svm_device_to_at(dev);
	struct hfi_at_svm *svm = NULL;
	struct mm_struct *mm = NULL;
	bool use_cpt = use_kernel_cpt;
	u64 pgdval;
	int ret;

	if (WARN_ON(!at))
		return -EINVAL;

	if ((flags & SVM_FLAG_SUPERVISOR_MODE)) {
		if (!ecap_srs(at->ecap))
			return -EINVAL;
	} else if (pasid) {
		mm = get_task_mm(current);
		use_cpt = use_user_cpt;
	}

	mutex_lock(&pasid_mutex);
	if (pasid && !(flags & SVM_FLAG_PRIVATE_PASID)) {
		int i;

		idr_for_each_entry(&at->pasid_idr, svm, i) {
			if (svm->mm != mm ||
			    (svm->flags & SVM_FLAG_PRIVATE_PASID))
				continue;

			svm->users++;
			goto success;
		}
	}

	if (!pasid) {
		/*
		 * If caller don't actually want to assign a PASID,
		 * this is just an enabling check/preparation.
		 */
		goto out;
	}

	svm = kzalloc(sizeof(*svm), GFP_KERNEL);
	if (!svm) {
		ret = -ENOMEM;
		goto out;
	}
	svm->at = at;
	svm->users = 1;
	hfi_at_enable_svm(at, svm);

	/* Do not use PASID 0 in caching mode (virtualised AT) */
	ret = idr_alloc(&at->pasid_idr, svm,
			!!cap_caching_mode(at->cap),
			at->pasid_max - 1, GFP_KERNEL);
	if (ret < 0) {
		kfree(svm);
		goto out;
	}
	spin_lock_init(&svm->lock);
	svm->pasid = ret;
	svm->notifier.ops = &hfi_at_mmuops;
	svm->mm = mm;
	svm->flags = flags;
	ret = -ENOMEM;

	svm->stats = hfi_at_alloc_stats(svm);
	if (!svm->stats) {
		idr_remove(&at->pasid_idr, svm->pasid);
		kfree(svm);
		goto out;
	}

	if (!use_cpt) {
		svm->tsk = current;
		svm->pgd = (struct at_pte *)alloc_pgtable_page(at->dd->node);
		if (!svm->pgd) {
			idr_remove(&svm->at->pasid_idr, svm->pasid);
			kfree(svm);
			goto out;
		}
		__at_flush_cache(at, svm->pgd, AT_PAGE_SIZE);

		pgdval = (u64)__pa(svm->pgd);
		hfi2_dbg("AT SPT alloc: pasid %d\n", svm->pasid);
	} else {
		/* we only use PTI for user process */
		pgdval = (u64)(mm ?
			__pa(get_pti_user_pgdp(svm->mm->pgd)) :
			__pa(at->system_mm->pgd));
	}

	if (mm) {
		ret = mmu_notifier_register(&svm->notifier, mm);
		if (ret) {
			if (svm->pgd)
				free_pgtable_page(svm->pgd);
			idr_remove(&svm->at->pasid_idr, svm->pasid);
			kfree(svm);
			goto out;
		}
		at->pasid_table[svm->pasid].val = pgdval | 1;
	} else {
		at->pasid_table[svm->pasid].val = pgdval | 1 | (1ULL << 11);
	}
	/* barrier for page table setup */
	wmb();
	/*
	 * In caching mode, we still have to flush with PASID 0 when
	 * a PASID table entry becomes present. Not entirely clear
	 * *why* that would be the case - surely we could just issue
	 * a flush with the PASID value that we've changed? The PASID
	 * is the index into the table, after all. It's not like domain
	 * IDs in the case of the equivalent context-entry change in
	 * caching mode. And for that matter it's not entirely clear why
	 * a VMM would be in the business of caching the PASID table
	 * anyway. Surely that can be left entirely to the guest?
	 */
	if (cap_caching_mode(at->cap))
		hfi_flush_pasid_dev(svm, 0);

 success:
	*pasid = svm->pasid;
	ret = 0;
 out:
	mutex_unlock(&pasid_mutex);
	if (mm)
		mmput(mm);
	return ret;
}

static int hfi_svm_unbind_mm(struct device *dev, int pasid)
{
	struct hfi_at *at;
	struct hfi_at_svm *svm;
	int ret = -EINVAL;

	mutex_lock(&pasid_mutex);
	at = hfi_svm_device_to_at(dev);
	if (!at || !at->pasid_table)
		goto out;

	svm = idr_find(&at->pasid_idr, pasid);
	if (!svm)
		goto out;

	ret = 0;
	svm->users--;
	if (!svm->users) {
		hfi_at_drain_pasid(svm, pasid);

		/*
		 * Flush the PASID cache and IOTLB for this device.
		 * Note that we do depend on the hardware *not* using
		 * the PASID any more. Just as we depend on other
		 * devices never using PASIDs that they have no right
		 * to use. We have a *shared* PASID table, because it's
		 * large and has to be physically contiguous. So it's
		 * hard to be as defensive as we might like.
		 */
		hfi_flush_pasid_dev(svm, svm->pasid);
		hfi_flush_svm_range_dev(svm, 0, -1, 0, !svm->mm);

		idr_remove(&svm->at->pasid_idr, svm->pasid);
		if (!at->dd->hfi_dev_dbg) {
			idr_remove(&at->pasid_stats_idr, svm->pasid);
			kfree(svm->stats);
		}

		if (svm->mm)
			mmu_notifier_unregister(&svm->notifier, svm->mm);

		if (svm->pgd) {
			struct page *freelist;

			hfi2_dbg("AT SPT free: pasid %d\n", svm->pasid);
			freelist = hfi_at_unmap(svm, 0, HFI_AT_DEFAULT_MAX_PFN);
			hfi_at_free_pagelist(freelist);
		}

		/*
		 * We mandate that no page faults may be outstanding
		 * for the PASID when hfi_svm_unbind_mm() is called.
		 * If that is not obeyed, subtle errors will happen.
		 * Let's make them less subtle...
		 */
		memset(svm, 0x6b, sizeof(*svm));
		kfree(svm);
	}
 out:
	mutex_unlock(&pasid_mutex);

	return ret;
}

int hfi_at_is_pasid_valid(struct device *dev, int pasid)
{
	struct hfi_at *at;
	struct hfi_at_svm *svm;
	int ret = -EINVAL;

	mutex_lock(&pasid_mutex);
	at = hfi_svm_device_to_at(dev);
	if (!at || !at->pasid_table)
		goto out;

	svm = idr_find(&at->pasid_idr, pasid);
	if (!svm)
		goto out;

	/* system-mm is used in this case */
	if (!svm->mm)
		ret = 1;
	else if (atomic_read(&svm->mm->mm_users) > 0)
		ret = 1;
	else
		ret = 0;

 out:
	mutex_unlock(&pasid_mutex);

	return ret;
}

static void at_cleanup(struct hfi_at *at)
{
	disable_at_context(at);
	free_at_context(at);
	free_at(at);
}

int
hfi_at_init(struct hfi_devdata *dd)
{
	int ret;
	struct hfi_at *at;

	ret = alloc_at(dd);
	if (ret) {
		dd_dev_err(dd, "alloc_at() failed\n");
		return ret;
	}

	at = dd->at;
	if (!ecap_ecs(at->ecap)) {
		dd_dev_err(dd, "AT: extended capability not supported\n");
		goto free_at;
	}

	if (translation_status(at)) {
		dd_dev_err(dd, "AT: translation already enabled.\n");
		goto free_at;
	}

	ret = hfi_at_init_qi(at);
	if (ret)
		goto free_at;

	if (!ecap_dev_iotlb_support(at->ecap)) {
		dd_dev_err(dd, "AT: DEVTLB not supported\n");
		goto free_at;
	}

	ret = at_alloc_root_entry(at);
	if (ret)
		goto free_at;

	if (!ecap_pasid(at->ecap)) {
		dd_dev_err(dd, "AT: pasid not supported.\n");
		goto free_at;
	}
	ret = hfi_svm_alloc_pasid_tables(at);
	if (ret)
		goto free_at;

	/*
	 * Now that qi is enabled on at, set the root entry and flush
	 * caches. This is required on some Intel X58 chipsets, otherwise
	 * flush_context function will loop forever and the boot hangs.
	 */
	at_flush_write_buffer(at);
	at_set_root_entry(at);
	qi_flush_context(at, 0, 0, 0, AT_CCMD_GLOBAL_INVL);
	qi_flush_iotlb(at, 0, 0, 0, AT_TLB_GLOBAL_FLUSH);

	if (!ecap_prs(at->ecap)) {
		dd_dev_err(dd, "AT: page-request not supported.\n");
		goto free_at;
	}
	ret = hfi_svm_enable_prq(at);
	if (ret)
		goto free_at;

	at_enable_translation(at);

	/*
	 * we always have to disable PMRs or DMA may fail on
	 * this unit
	 */
	at_disable_protect_mem_regions(at);

	ret = at_setup_device_context(dd);
	if (ret)
		goto free_at;

	return 0;

free_at:
	at_cleanup(at);
	dd->at = NULL;

	return ret;
}

void
hfi_at_exit(struct hfi_devdata *dd)
{
	struct hfi_at *at;

	at = dd->at;
	if (!at)
		return;

	at_cleanup(at);
	dd->at = NULL;
}

static inline
uint64_t hfi_at_system_pasid_enable(int id)
{
	u64 val;

	val = FXR_AT_CFG_USE_SYSTEM_PASID_ENABLE_SMASK |
		((FXR_AT_CFG_USE_SYSTEM_PASID_SYSTEM_PASID_MASK & id) <<
		FXR_AT_CFG_USE_SYSTEM_PASID_SYSTEM_PASID_SHIFT);

	return val;
}

int
hfi_at_set_pasid(struct hfi_ctx *ctx)
{
	struct hfi_devdata *dd = ctx->devdata;
	struct device *dev = &dd->pdev->dev;
	int ret, pasid, flags = 0;
	u64 lut = 0;

	if (ctx->type != HFI_CTX_TYPE_USER)
		flags |= SVM_FLAG_SUPERVISOR_MODE;

	/*
	 * Since we don't set SVM_FLAG_PRIVATE_PASID, all kernel entities
	 * will share the same PASID.
	 */
	ret = hfi_svm_bind_mm(dev, &pasid, flags);
	if (ret)
		return ret;

	/*
	 * Save pasid for cleanup.
	 */
	ctx->pasid = pasid;
	dd_dev_info(dd, "%s %d pasid %d system_pasid %d pid %d flags 0x%x type %d\n",
		    __func__, __LINE__, pasid, dd->system_pasid,
		    ctx->pid, flags, ctx->type);

	/*
	 * Set privileged mode for non-user contxt,
	 * also set system pasid for pid=0.
	 */
	if (ctx->type != HFI_CTX_TYPE_USER) {
		if (ctx->pid == HFI_PID_SYSTEM) {
			u64 sys = hfi_at_system_pasid_enable(pasid);

			write_csr(dd, FXR_AT_CFG_USE_SYSTEM_PASID, sys);

			dd->system_pasid = pasid;
		} else {
			WARN_ON(dd->system_pasid != pasid);
		}

		lut &= ~FXR_AT_CFG_PASID_LUT_PRIVILEGE_LEVEL_SMASK;
	} else {
		lut |= FXR_AT_CFG_PASID_LUT_PRIVILEGE_LEVEL_SMASK;
	}

	lut |= FXR_AT_CFG_PASID_LUT_ENABLE_SMASK |
		((FXR_AT_CFG_PASID_LUT_PASID_MASK & pasid) <<
		FXR_AT_CFG_PASID_LUT_PASID_SHIFT);

	write_csr(dd, FXR_AT_CFG_PASID_LUT + (ctx->pid * 8), lut);

	sprintf(ctx->pasid_str, "%d", ctx->pasid);
	hfi_at_ctx_dbg_init(ctx);
	return 0;
}

int
hfi_at_clear_pasid(struct hfi_ctx *ctx)
{
	struct hfi_devdata *dd = ctx->devdata;
	struct device *dev = &dd->pdev->dev;
	u64 lut = 0;

	debugfs_remove_recursive(ctx->dbg);
	memset(ctx->pasid_str, 0, ARRAY_SIZE(ctx->pasid_str));

	/* disable pid->pasid translation */
	lut &= ~FXR_AT_CFG_PASID_LUT_ENABLE_SMASK;
	write_csr(dd, FXR_AT_CFG_PASID_LUT + (ctx->pid * 8), lut);

	/* call into svm to free pasid */
	return hfi_svm_unbind_mm(dev, ctx->pasid);
}
