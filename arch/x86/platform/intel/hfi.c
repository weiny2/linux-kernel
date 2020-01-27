// SPDX-License-Identifier: GPL-2.0-only
/*
 * Hardware Feedback Interface Driver
 *
 * Copyright (c) 2020, Intel Corporation.
 *
 * Authors: Aubrey Li <aubrey.li@linux.intel.com>
 *          Ricardo Neri <ricardo.neri-calderon@linux.intel.com>
 *
 *
 * The Hardware Feedback Interface provides a performance and energy efficiency
 * capability rating fo each logical processor in the system. Hardware
 * continuously updates these ratings depending on the operating conditions
 * of the system (e.g., power limits, thermal constraints, etc). This file
 * provides functionality to use the HFI to update the CPU scheduling priority
 * using the existing Intel Turbo Boost Max 3.0 facilities.
 */

#define pr_fmt(fmt)  "intel-hfi: " fmt

#include <linux/cpu.h>
#include <linux/cpuhotplug.h>
#include <linux/cpumask.h>
#include <linux/gfp.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/topology.h>
#include <linux/workqueue.h>

#include <asm/hfi.h>
#include <asm/io.h>

#define MAX_HFI_PAGES (((num_possible_cpus() * sizeof(struct hfi_cpu_cap)) >> \
		       PAGE_SHIFT) + 1)

#define THERM_STATUS_CLEAR_PKG_MASK (BIT(1) | BIT(3) | BIT(5) | BIT(7) | \
				     BIT(9) | BIT(11) | BIT(26))

/* Delay reading HFI updates a few milliseconds */
#define HFI_UPDATE_DELAY (1000 / HZ)

/* Per-processor capabilities structure */
struct hfi_cpu_cap {
	u8	perf_cap;
	u8	ee_cap;
	u8	reserved[6];
};

/* Hardware Feedback Interface structure */
struct hfi_table {
	u64			ts_counter;
	u8			perf_change;
	u8			ee_change;
	u8			reserved[6];
	struct hfi_cpu_cap	cpu_cap[0];
};

struct hfi_info {
	u64 ts_counter;
	u32 perf_cap;
	u32 index;
	bool initialized;
};

static DEFINE_PER_CPU(struct hfi_info, hfi_info);
static struct hfi_table *hfi_table_base;
static bool package_initialized;

static u32 get_cpu_index(void)
{
	u32 edx;

	/*
	 * The index of a n'th CPU does not necessarily matches the n'th entry
	 * in the n'th entry in the HFI table.
	 */
	edx = cpuid_edx(CPUID_HFI_LEAF);
	return (edx & CPUID_HFI_CPU_INDEX_MASK) >> CPUID_HFI_CPU_INDEX_SHIFT;
}

static void update_prio(int cpu)
{
	struct hfi_table *table = hfi_table_base;
	u32 perf_cap_old, index;

	if (!table)
		return;

	/*
	 * Sanity check that the timestamp has gone forward before consuming
	 * the table update.
	 */
	if (per_cpu(hfi_info, cpu).ts_counter > table->ts_counter)
		return;

	perf_cap_old = per_cpu(hfi_info, cpu).perf_cap;
	index = per_cpu(hfi_info, cpu).index;
	per_cpu(hfi_info, cpu).perf_cap = table->cpu_cap[index].perf_cap;
	per_cpu(hfi_info, cpu).ts_counter = table->ts_counter;
	sched_set_itmt_core_prio(per_cpu(hfi_info, cpu).perf_cap, cpu);
}

static void hfi_update_work_fn(struct work_struct *work)
{
	u64 msr_val;
	int cpu;

	get_online_cpus();
	for_each_online_cpu(cpu)
		update_prio(cpu);
	put_online_cpus();

	rdmsrl(MSR_IA32_PACKAGE_THERM_STATUS, msr_val);
	msr_val &= THERM_STATUS_CLEAR_PKG_MASK &
		   ~PACKAGE_THERM_STATUS_HFI_UPDATED;
	wrmsrl(MSR_IA32_PACKAGE_THERM_STATUS, msr_val);
}
DECLARE_DELAYED_WORK(hfi_update_work, hfi_update_work_fn);

void intel_hfi_check_event(void)
{
	int cpu = smp_processor_id();
	u64 msr_val;

	/*
	 * If this is the first time we update the priority, we must set the CPU
	 * index. We cannot set it in the CPU hotplug online call back, as it
	 * may be called after an interrupt has been received in a particular
	 * CPU.
	 */
	if (!per_cpu(hfi_info, cpu).initialized) {
		per_cpu(hfi_info, cpu).index = get_cpu_index();
		per_cpu(hfi_info, cpu).initialized = true;
	}

	rdmsrl(MSR_IA32_PACKAGE_THERM_STATUS, msr_val);
	if (!(msr_val & PACKAGE_THERM_STATUS_HFI_UPDATED))
		return;

	schedule_delayed_work(&hfi_update_work, HFI_UPDATE_DELAY);
}

static void intel_hfi_cpu_init(int cpu)
{
	u64 msr_val;

	if (package_initialized)
		return;

	/* Clear status bit to let hadware update the feedback interface. */
	rdmsrl(MSR_IA32_PACKAGE_THERM_STATUS, msr_val);
	msr_val &= THERM_STATUS_CLEAR_PKG_MASK &
		   ~PACKAGE_THERM_STATUS_HFI_UPDATED;
	wrmsrl(MSR_IA32_PACKAGE_THERM_STATUS, msr_val);

	/* Enable the hardware feedback interface. */
	rdmsrl(MSR_IA32_HW_FEEDBACK_CONFIG, msr_val);
	msr_val |= HFI_CONFIG_ENABLE_BIT;
	wrmsrl(MSR_IA32_HW_FEEDBACK_CONFIG, msr_val);

	package_initialized = true;
}

static int intel_hfi_cpu_online(unsigned int cpu)
{
	intel_hfi_cpu_init(cpu);
	return 0;
}

static void __init intel_hfi_init(void)
{
	unsigned int edx, table_pages;
	phys_addr_t addr;
	u64 msr_val;
	int ret;

	if (!boot_cpu_has(X86_FEATURE_INTEL_HFI))
		return;

	/* Today, HFI is single-package only. */
	if (topology_max_packages() > 1)
		return;

	/*
	 * We know that CPUID_HFI_LEAF exists since X86_FEATURE_INTEL_HFI was
	 * detected.
	 */
	edx = cpuid_edx(CPUID_HFI_LEAF);

	if (!(edx & HFI_PERFORMANCE_REPORTING)) {
		pr_err("Performance reporting not supported! Not using HFI\n");
		return;
	}

	table_pages = ((edx & CPUID_HFI_TABLE_SIZE_MASK) >>
		       CPUID_HFI_TABLE_SIZE_SHIFT) + 1;

	if (table_pages > MAX_HFI_PAGES) {
		pr_err("Requested pages are more than expected\n");
		return;
	}

	hfi_table_base = (struct hfi_table *)__get_free_pages(GFP_KERNEL |
							      __GFP_ZERO,
							      table_pages);
	if (!hfi_table_base) {
		pr_err("Unable to allocate pages for table\n");
		return;
	}

	addr = virt_to_phys(hfi_table_base);

	/* Program address of the feedback table. */
	msr_val = addr | HFI_PTR_VALID_BIT;
	wrmsrl(MSR_IA32_HW_FEEDBACK_PTR, msr_val);

	ret = cpuhp_setup_state(CPUHP_AP_ONLINE_DYN,
				"platform/x86/intel_hfi:online",
				intel_hfi_cpu_online, NULL);

	if (ret < 0) {
		pr_err("Failed to register hotplug callback %d\n", ret);
		goto err;
	}

	return;

err:
	msr_val &= ~HFI_PTR_VALID_BIT;
	wrmsrl(MSR_IA32_HW_FEEDBACK_PTR, msr_val);
	free_pages((unsigned long)hfi_table_base, table_pages);
}

device_initcall(intel_hfi_init);
