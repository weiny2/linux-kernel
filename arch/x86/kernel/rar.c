/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Remote Action Request (RAR) routines
 */
#include <linux/bits.h>
#include <linux/bug.h>
#include <linux/kgdb.h>
#include <linux/printk.h>
#include <linux/sched.h>
#include <asm/current.h>
#include <asm/irq_vectors.h>
#include <asm/rar.h>
#include <asm/smp.h>
#include <asm/sync_bitops.h>
#include <asm/tlbflush.h>
#include <asm/io.h>

static unsigned long rar_in_use = ~GENMASK(RAR_MAX_PAYLOADS - 1, 0);
static struct rar_payload rar_payload[RAR_MAX_PAYLOADS] __page_aligned_bss;
static DEFINE_PER_CPU_ALIGNED(u64[(RAR_MAX_PAYLOADS + 7) / 8], rar_action);

static unsigned long get_payload(void)
{
	while (1) {
		unsigned long bit;
		unsigned long old;

		old = READ_ONCE(rar_in_use);

		if (old == ~0UL)
			continue;

		/*
		 * Find a free bit and confirm with cmpxchg().
		 */
		bit = ffz(old);

		if (old == cmpxchg(&rar_in_use, old, old | (1UL << bit)))
			return bit;
	}
}

static void free_payload(unsigned long idx)
{
	clear_bit(idx, &rar_in_use);
}

static void rar_invpcid_payload(int payload_idx, u16 pcid,
				const struct flush_tlb_info *info)
{
	struct rar_payload *p = &rar_payload[payload_idx];

	uint64_t subtype;
	uint64_t num_pages;
	uint64_t stride;

	if (!info->mm) {
		subtype = RAR_INVPCID_ALL_GLOBAL;
		num_pages = 0;
		stride = 0;
	} else if ((info->end == TLB_FLUSH_ALL) ||
		   (info->stride_shift < PAGE_SHIFT) ||
		   (info->stride_shift > PUD_SHIFT)) {
		subtype = RAR_INVPCID_ONE_PCID_ALL;
		num_pages = 0;
		stride = 0;
	} else {
		uint64_t stride_size;

		if (info->stride_shift < PMD_SHIFT) {
			stride = RAR_STRIDE_4K;
			stride_size = PAGE_SIZE;
		} else if (info->stride_shift < PUD_SHIFT) {
			stride = RAR_STRIDE_2M;
			stride_size = PMD_SIZE;
		} else {
			stride = RAR_STRIDE_1G;
			stride_size = PUD_SIZE;
		}

		num_pages = (info->end - info->start + stride_size - 1)
				/ stride_size;

		if (num_pages > RAR_MAX_PAGES) {
			num_pages = 0;
			subtype = RAR_INVPCID_ONE_PCID_ALL;
		} else {
			subtype = RAR_INVPCID_ADDR_RANGE;
		}
	}

	p->tlb_flush = (RAR_TYPE_INVPCID << RAR_TYPE_SHIFT) |
		       (subtype << RAR_SUBTYPE_SHIFT) |
		       (num_pages << RAR_PAGES_SHIFT) |
		       (stride << RAR_STRIDE_SHIFT);

	p->pcid = pcid;
	p->addr = info->start;
	p->must_be_zero = 0;
}

static void rar_invpg_payload(int payload_idx,
			      const struct flush_tlb_info *info)
{
	struct rar_payload *p = &rar_payload[payload_idx];

	uint64_t subtype;
	uint64_t num_pages;
	uint64_t stride;

	if ((!info->mm) || (info->end == TLB_FLUSH_ALL) ||
	    (info->stride_shift < PAGE_SHIFT) ||
	    (info->stride_shift > PUD_SHIFT)) {
		subtype = RAR_INVPG_ALL;
		num_pages = 0;
		stride = 0;
	} else {
		uint64_t stride_size;

		if (info->stride_shift < PMD_SHIFT) {
			stride = RAR_STRIDE_4K;
			stride_size = PAGE_SIZE;
		} else if (info->stride_shift < PUD_SHIFT) {
			stride = RAR_STRIDE_2M;
			stride_size = PMD_SIZE;
		} else {
			stride = RAR_STRIDE_1G;
			stride_size = PUD_SIZE;
		}

		num_pages = (info->end - info->start + stride_size - 1)
				/ stride_size;

		if (num_pages > RAR_MAX_PAGES) {
			num_pages = 0;
			subtype = RAR_INVPG_ALL;
		} else {
			subtype = RAR_INVPG_ADDR_RANGE;
		}
	}

	p->tlb_flush = (RAR_TYPE_INVPG << RAR_TYPE_SHIFT) |
		       (subtype << RAR_SUBTYPE_SHIFT) |
		       (num_pages << RAR_PAGES_SHIFT) |
		       (stride << RAR_STRIDE_SHIFT);

	p->cr3 = __read_cr3();
	p->addr = info->start;
	p->must_be_zero = 0;
}

static void set_action_entry(unsigned long idx, int target_cpu)
{
	u8 *bitmap = (u8 *)per_cpu(rar_action, target_cpu);

	WRITE_ONCE(bitmap[idx], RAR_ACTION_START);
}

static void wait_for_done(unsigned long idx, int target_cpu)
{
	u8 status;
	u8 *bitmap = (u8 *)per_cpu(rar_action, target_cpu);

	status = smp_cond_load_acquire(&bitmap[idx],
				       (VAL == RAR_ACTION_OK) ||
				       (VAL == RAR_ACTION_FAIL));
	WARN_ONCE(status == RAR_ACTION_FAIL, "RAR TLB flush failed\n");
}

void rar_cpu_init(void)
{
	u64 r;
	u8 *bitmap;
	int this_cpu = smp_processor_id();

	rdmsrl(MSR_IA32_RAR_INFO, r);
	pr_info_once("RAR: support %lld payloads\n",
		     (r >> RAR_INFO_MAX_PAYLOAD_SHIFT) + 1);

	bitmap = (u8 *)per_cpu(rar_action, this_cpu);
	wrmsrl(MSR_IA32_RAR_ACT_VEC, (u64)virt_to_phys(bitmap));
	wrmsrl(MSR_IA32_RAR_PAYLOAD_BASE, (u64)virt_to_phys(rar_payload));

	r = RAR_CTRL_ENABLE | RAR_CTRL_IGNORE_IF;
	wrmsrl(MSR_IA32_RAR_CTRL, r);
}

/*
 * This is a modified version of smp_call_function_many(), and must be called
 * with preemption disabled but not interrupts disabled.
 * The calling CPU creates a TLB flush payload and sends an RAR command to the
 * APIC.  The receiving CPUs flush their TLBs according to the payload.
 */
void rar_flush_tlb_others(const struct cpumask *cpumask, unsigned long pcid,
			  const struct flush_tlb_info *info)
{
	unsigned long idx;
	int cpu, this_cpu = smp_processor_id();
	cpumask_t dest_mask;

	WARN_ON(preemptible());

	/*
	 * Can deadlock when called with interrupts disabled.
	 * We allow cpu's that are not yet online though, as no one else can
	 * send smp call function interrupt to this cpu and as such deadlocks
	 * can't happen.
	 */
	WARN_ON_ONCE(cpu_online(this_cpu) && irqs_disabled()
		     && !oops_in_progress && !early_boot_irqs_disabled);

	cpumask_and(&dest_mask, cpumask, cpu_online_mask);
	cpumask_clear_cpu(this_cpu, &dest_mask);

	/* Some callers race with other cpus changing the passed mask */
	if (unlikely(!cpumask_weight(&dest_mask)))
		return;

	idx = get_payload();

	if (static_cpu_has(X86_FEATURE_PCID))
		rar_invpcid_payload(idx, pcid, info);
	else
		rar_invpg_payload(idx, info);

	/*
	 * A payload can get processed by receiving processors
	 * even before another RAR command is issued.
	 * Make sure things are globally visible.
	 */
	smp_mb();

	for_each_cpu(cpu, &dest_mask)
		set_action_entry(idx, cpu);

	/* Send a message to all CPUs in the map */
	arch_send_rar_ipi_mask(&dest_mask);

	for_each_cpu(cpu, &dest_mask)
		wait_for_done(idx, cpu);

	free_payload(idx);
}
EXPORT_SYMBOL(rar_flush_tlb_others);
