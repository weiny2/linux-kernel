#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/unistd.h>
#include <linux/export.h>
#include <linux/reboot.h>
#include <linux/sysrq.h>
#include <linux/stringify.h>
#include <linux/stop_machine.h>
#include <linux/syscore_ops.h>
#include <asm/irq.h>
#include <asm/mmu_context.h>
#include <xen/evtchn.h>
#include <asm/hypervisor.h>
#include <xen/xenbus.h>
#include <linux/cpu.h>
#include <xen/clock.h>
#include <xen/gnttab.h>
#include <xen/xencons.h>
#include <xen/cpu_hotplug.h>
#include <xen/interface/vcpu.h>
#include "../../base/base.h"

#if defined(__i386__) || defined(__x86_64__)
#include <asm/pci_x86.h>
/* TBD: Dom0 should propagate the determined value to Xen. */
bool port_cf9_safe = false;

/*
 * Power off function, if any
 */
void (*pm_power_off)(void);
EXPORT_SYMBOL(pm_power_off);

void machine_emergency_restart(void)
{
	/* We really want to get pending console data out before we die. */
	xencons_force_flush();
	HYPERVISOR_shutdown(SHUTDOWN_reboot);
}

void machine_restart(char * __unused)
{
	machine_emergency_restart();
}

void machine_halt(void)
{
	machine_power_off();
}

void machine_power_off(void)
{
	/* We really want to get pending console data out before we die. */
	xencons_force_flush();
	if (pm_power_off)
		pm_power_off();
	HYPERVISOR_shutdown(SHUTDOWN_poweroff);
}

#ifdef CONFIG_PM_SLEEP
static void pre_suspend(void)
{
	HYPERVISOR_shared_info = (shared_info_t *)empty_zero_page;
	WARN_ON(HYPERVISOR_update_va_mapping(fix_to_virt(FIX_SHARED_INFO),
					     __pte_ma(0), 0));

	xen_start_info->store_mfn = mfn_to_pfn(xen_start_info->store_mfn);
	xen_start_info->console.domU.mfn =
		mfn_to_pfn(xen_start_info->console.domU.mfn);
}

static void post_suspend(int suspend_cancelled, int fast_suspend)
{
	unsigned long shinfo_mfn;

	if (suspend_cancelled) {
		xen_start_info->store_mfn =
			pfn_to_mfn(xen_start_info->store_mfn);
		xen_start_info->console.domU.mfn =
			pfn_to_mfn(xen_start_info->console.domU.mfn);
	} else {
		unsigned int i;

#ifdef CONFIG_SMP
		cpumask_copy(vcpu_initialized_mask, cpu_online_mask);
#endif
		for_each_possible_cpu(i) {
			setup_runstate_area(i);

#ifdef CONFIG_XEN_VCPU_INFO_PLACEMENT
			if (fast_suspend && i != smp_processor_id()
			    && cpu_online(i)
			    && HYPERVISOR_vcpu_op(VCPUOP_down, i, NULL))
				BUG();

			setup_vcpu_info(i);

			if (fast_suspend && i != smp_processor_id()
			    && cpu_online(i)
			    && HYPERVISOR_vcpu_op(VCPUOP_up, i, NULL))
				BUG();
#endif

			if (cpu_online(i))
				setup_vsyscall_time_area(i);
		}
	}

	shinfo_mfn = xen_start_info->shared_info >> PAGE_SHIFT;
	if (HYPERVISOR_update_va_mapping(fix_to_virt(FIX_SHARED_INFO),
					 pfn_pte_ma(shinfo_mfn, PAGE_KERNEL),
					 0))
		BUG();
	HYPERVISOR_shared_info = (shared_info_t *)fix_to_virt(FIX_SHARED_INFO);

	clear_page(empty_zero_page);

	if (!suspend_cancelled)
		setup_pfn_to_mfn_frame_list(NULL);
}
#endif

#else /* !(defined(__i386__) || defined(__x86_64__)) */

#ifndef HAVE_XEN_PRE_SUSPEND
#define xen_pre_suspend()	((void)0)
#endif

#ifndef HAVE_XEN_POST_SUSPEND
#define xen_post_suspend(x)	((void)0)
#endif

#define switch_idle_mm()	((void)0)
#define mm_pin_all()		((void)0)
#define pre_suspend()		xen_pre_suspend()
#define post_suspend(x, f)	xen_post_suspend(x)

#endif

#ifdef CONFIG_PM_SLEEP
struct suspend {
	int fast_suspend;
	void (*resume_notifier)(int);
};

static int take_machine_down(void *_suspend)
{
	struct suspend *suspend = _suspend;
	int suspend_cancelled;

	BUG_ON(!irqs_disabled());

	mm_pin_all();
	suspend_cancelled = syscore_suspend();
	if (!suspend_cancelled) {
		pre_suspend();

		/*
		 * This hypercall returns 1 if suspend was cancelled or the domain was
		 * merely checkpointed, and 0 if it is resuming in a new domain.
		 */
		suspend_cancelled = HYPERVISOR_suspend(virt_to_mfn(xen_start_info));
	} else
		BUG_ON(suspend_cancelled > 0);
	suspend->resume_notifier(suspend_cancelled);
	if (suspend_cancelled >= 0)
		post_suspend(suspend_cancelled, suspend->fast_suspend);
	if (!suspend_cancelled)
		xen_clockevents_resume(false);
	if (suspend_cancelled >= 0)
		syscore_resume();
	if (!suspend_cancelled) {
#ifdef __x86_64__
		/*
		 * Older versions of Xen do not save/restore the user %cr3.
		 * We do it here just in case, but there's no need if we are
		 * in fast-suspend mode as that implies a new enough Xen.
		 */
		if (!suspend->fast_suspend)
			xen_new_user_pt(current->active_mm->pgd);
#endif
	}

	return suspend_cancelled;
}

int __xen_suspend(int fast_suspend, void (*resume_notifier)(int))
{
	int err, suspend_cancelled;
	const char *what;
	struct suspend suspend;

#define _check(fn, args...) ({ \
	what = #fn; \
	err = (fn)(args); \
})

	BUG_ON(smp_processor_id() != 0);
	BUG_ON(in_interrupt());

#if defined(__i386__) || defined(__x86_64__)
	if (xen_feature(XENFEAT_auto_translated_physmap)) {
		pr_warning("Can't suspend in auto_translated_physmap mode\n");
		return -EOPNOTSUPP;
	}
#endif

	/* If we are definitely UP then 'slow mode' is actually faster. */
	if (num_possible_cpus() == 1)
		fast_suspend = 0;

	suspend.fast_suspend = fast_suspend;
	suspend.resume_notifier = resume_notifier;

	if (_check(dpm_suspend_start, PMSG_SUSPEND)) {
		dpm_resume_end(PMSG_RESUME);
		pr_err("%s() failed: %d\n", what, err);
		return err;
	}

	if (fast_suspend) {
		xenbus_suspend();

		if (_check(dpm_suspend_end, PMSG_SUSPEND)) {
			xenbus_suspend_cancel();
			dpm_resume_end(PMSG_RESUME);
			pr_err("%s() failed: %d\n", what, err);
			return err;
		}

		err = stop_machine(take_machine_down, &suspend,
				   cpumask_of(0));
		if (err < 0)
			xenbus_suspend_cancel();
	} else {
		BUG_ON(irqs_disabled());

		for (;;) {
			xenbus_suspend();

			if (!_check(dpm_suspend_end, PMSG_SUSPEND)
			    && _check(smp_suspend))
				dpm_resume_start(PMSG_RESUME);
			if (err) {
				xenbus_suspend_cancel();
				dpm_resume_end(PMSG_RESUME);
				pr_err("%s() failed: %d\n", what, err);
				return err;
			}

			preempt_disable();

			if (num_online_cpus() == 1)
				break;

			preempt_enable();

			dpm_resume_start(PMSG_RESUME);

			xenbus_suspend_cancel();
		}

		local_irq_disable();
		err = take_machine_down(&suspend);
		local_irq_enable();
	}

	dpm_resume_start(PMSG_RESUME);

	if (err >= 0) {
		suspend_cancelled = err;
		if (!suspend_cancelled) {
			xencons_resume();
			xenbus_resume();
		} else {
			xenbus_suspend_cancel();
			err = 0;
		}

		if (!fast_suspend)
			smp_resume();
		else
			xen_clockevents_resume(true);
	}

	dpm_resume_end(PMSG_RESUME);

	/* Make sure timer events get retriggered on all CPUs */
	clock_was_set();

	return err;
}
#endif
