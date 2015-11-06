/*
 *	Xen SMP booting functions
 *
 *	See arch/i386/kernel/smpboot.c for copyright and credits for derived
 *	portions of this file.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/kernel_stat.h>
#include <linux/irq.h>
#include <linux/notifier.h>
#include <linux/cpu.h>
#include <linux/percpu.h>
#include <linux/tick.h>
#include <asm/desc.h>
#include <asm/pgalloc.h>
#include <xen/clock.h>
#include <xen/evtchn.h>
#include <xen/interface/vcpu.h>
#include <xen/cpu_hotplug.h>
#include <xen/xenbus.h>

extern int local_setup_timer(unsigned int cpu);
extern void local_teardown_timer(unsigned int cpu);

extern void hypervisor_callback(void);
extern void failsafe_callback(void);
extern void system_call(void);
extern void smp_trap_init(trap_info_t *);

cpumask_var_t vcpu_initialized_mask;

DEFINE_PER_CPU_READ_MOSTLY(struct cpuinfo_x86, cpu_info);
EXPORT_PER_CPU_SYMBOL(cpu_info);

static int __read_mostly ipi_irq = -1;

void __init prefill_possible_map(void)
{
	int i, rc;

	for_each_possible_cpu(i)
	    if (i != smp_processor_id())
		return;

	for (i = 0; i < NR_CPUS; i++) {
#ifndef CONFIG_HOTPLUG_CPU
		if (i >= setup_max_cpus)
			break;
#endif
		rc = HYPERVISOR_vcpu_op(VCPUOP_is_up, i, NULL);
		if (rc >= 0) {
			set_cpu_possible(i, true);
			nr_cpu_ids = i + 1;
		}
	}
	total_cpus = num_possible_cpus();
	for (; HYPERVISOR_vcpu_op(VCPUOP_is_up, i, NULL) >= 0; ++i)
		if (i != smp_processor_id())
			++total_cpus;
}

static irqreturn_t ipi_interrupt(int irq, void *dev_id)
{
	static void (*const handlers[])(struct pt_regs *) = {
		[RESCHEDULE_VECTOR] = smp_reschedule_interrupt,
		[CALL_FUNCTION_VECTOR] = smp_call_function_interrupt,
		[CALL_FUNC_SINGLE_VECTOR] = smp_call_function_single_interrupt,
		[REBOOT_VECTOR] = smp_reboot_interrupt,
#ifdef CONFIG_IRQ_WORK
		[IRQ_WORK_VECTOR] = smp_irq_work_interrupt,
#endif
	};
	unsigned long *pending = __get_cpu_var(ipi_pending);
	struct pt_regs *regs = get_irq_regs();
	irqreturn_t ret = IRQ_NONE;

	for (;;) {
		unsigned int ipi = find_first_bit(pending, NR_IPIS);

		if (ipi >= NR_IPIS) {
			clear_ipi_evtchn();
			ipi = find_first_bit(pending, NR_IPIS);
		}
		if (ipi >= NR_IPIS)
			return ret;
		ret = IRQ_HANDLED;
		do {
			clear_bit(ipi, pending);
			handlers[ipi](regs);
			ipi = find_next_bit(pending, NR_IPIS, ipi);
		} while (ipi < NR_IPIS);
	}
}

static int xen_smp_intr_init(unsigned int cpu)
{
	static struct irqaction ipi_action = {
		.handler = ipi_interrupt,
		.flags   = IRQF_DISABLED,
		.name    = "ipi"
	};
	int rc;

	rc = bind_ipi_to_irqaction(cpu, &ipi_action);
	if (rc < 0)
		return rc;
	if (ipi_irq < 0)
		ipi_irq = rc;
	else
		BUG_ON(ipi_irq != rc);

	rc = xen_spinlock_init(cpu);
	if (rc < 0)
		goto unbind_ipi;

	if ((cpu != 0) && ((rc = local_setup_timer(cpu)) != 0))
		goto fail;

	return 0;

 fail:
	xen_spinlock_cleanup(cpu);
 unbind_ipi:
	unbind_from_per_cpu_irq(ipi_irq, cpu, NULL);
	return rc;
}

static void xen_smp_intr_exit(unsigned int cpu)
{
	if (cpu != 0)
		local_teardown_timer(cpu);

	unbind_from_per_cpu_irq(ipi_irq, cpu, NULL);
	xen_spinlock_cleanup(cpu);
}

static void cpu_bringup(void)
{
	unsigned int cpu;

	cpu_init();
	identify_secondary_cpu(__this_cpu_ptr(&cpu_info));
	touch_softlockup_watchdog();
	preempt_disable();
	xen_setup_cpu_clockevents();
	cpu = smp_processor_id();
	notify_cpu_starting(cpu);
	set_cpu_online(cpu, true);
	local_irq_enable();
}

static void cpu_bringup_and_idle(void)
{
	cpu_bringup();
	cpu_startup_entry(CPUHP_ONLINE);
}

static void cpu_initialize_context(unsigned int cpu, unsigned long sp0)
{
	/* vcpu_guest_context_t is too large to allocate on the stack.
	 * Hence we allocate statically and protect it with a lock */
	static vcpu_guest_context_t ctxt;
	static DEFINE_SPINLOCK(ctxt_lock);

	if (cpumask_test_and_set_cpu(cpu, vcpu_initialized_mask))
		return;

	spin_lock(&ctxt_lock);

	memset(&ctxt, 0, sizeof(ctxt));

	ctxt.flags = VGCF_IN_KERNEL;
	ctxt.user_regs.ds = __USER_DS;
	ctxt.user_regs.es = __USER_DS;
	ctxt.user_regs.ss = __KERNEL_DS;
	ctxt.user_regs.eip = (unsigned long)cpu_bringup_and_idle;
	ctxt.user_regs.eflags = X86_EFLAGS_IF | 0x1000; /* IOPL_RING1 */

	smp_trap_init(ctxt.trap_ctxt);

	ctxt.gdt_frames[0] = arbitrary_virt_to_mfn(get_cpu_gdt_table(cpu));
	ctxt.gdt_ents = GDT_SIZE / 8;

	ctxt.user_regs.cs = __KERNEL_CS;
	ctxt.user_regs.esp = sp0 - sizeof(struct pt_regs);

	ctxt.kernel_ss = __KERNEL_DS;
	ctxt.kernel_sp = sp0;

	ctxt.event_callback_eip    = (unsigned long)hypervisor_callback;
	ctxt.failsafe_callback_eip = (unsigned long)failsafe_callback;
#ifdef __i386__
	ctxt.event_callback_cs     = __KERNEL_CS;
	ctxt.failsafe_callback_cs  = __KERNEL_CS;

	ctxt.ctrlreg[3] = xen_pfn_to_cr3(virt_to_mfn(swapper_pg_dir));

	ctxt.user_regs.fs = __KERNEL_PERCPU;
	ctxt.user_regs.gs = __KERNEL_STACK_CANARY;
#else /* __x86_64__ */
	ctxt.syscall_callback_eip  = (unsigned long)system_call;

	ctxt.ctrlreg[3] = xen_pfn_to_cr3(virt_to_mfn(init_level4_pgt));

	ctxt.gs_base_kernel = per_cpu_offset(cpu);
#endif

	if (HYPERVISOR_vcpu_op(VCPUOP_initialise, cpu, &ctxt))
		BUG();

	spin_unlock(&ctxt_lock);
}

void __init smp_prepare_cpus(unsigned int max_cpus)
{
	unsigned int cpu;
	int apicid;
	struct vcpu_get_physid cpu_id;
	void *gdt_addr;

	apicid = 0;
	if (HYPERVISOR_vcpu_op(VCPUOP_get_physid, 0, &cpu_id) == 0)
		apicid = xen_vcpu_physid_to_x86_apicid(cpu_id.phys_id);
	cpu_data(0) = boot_cpu_data;
	current_thread_info()->cpu = 0;

	if (xen_smp_intr_init(0))
		BUG();

	if (!alloc_cpumask_var(&vcpu_initialized_mask, GFP_KERNEL))
		BUG();
	cpumask_copy(vcpu_initialized_mask, cpumask_of(0));

	/* Restrict the possible_map according to max_cpus. */
	while ((num_possible_cpus() > 1) && (num_possible_cpus() > max_cpus)) {
		for (cpu = nr_cpu_ids-1; !cpu_possible(cpu); cpu--)
			continue;
		set_cpu_possible(cpu, false);
	}

	for_each_possible_cpu (cpu) {
		if (cpu == 0)
			continue;

		gdt_addr = get_cpu_gdt_table(cpu);
		make_page_readonly(gdt_addr, XENFEAT_writable_descriptor_tables);

		apicid = cpu;
		if (HYPERVISOR_vcpu_op(VCPUOP_get_physid, cpu, &cpu_id) == 0)
			apicid = xen_vcpu_physid_to_x86_apicid(cpu_id.phys_id);
		cpu_data(cpu) = boot_cpu_data;
		cpu_data(cpu).cpu_index = cpu;

		irq_ctx_init(cpu);

#ifdef CONFIG_HOTPLUG_CPU
		if (is_initial_xendomain())
#endif
			set_cpu_present(cpu, true);
	}

	init_xenbus_allowed_cpumask();

#ifdef CONFIG_X86_IO_APIC
	/*
	 * Here we can be sure that there is an IO-APIC in the system. Let's
	 * go and set it up:
	 */
	if (cpu_has_apic && !skip_ioapic_setup && nr_ioapics)
		setup_IO_APIC();
#endif
}

void __init smp_prepare_boot_cpu(void)
{
	unsigned int cpu;

	switch_to_new_gdt(smp_processor_id());
	prefill_possible_map();
	for_each_possible_cpu(cpu)
		if (cpu != smp_processor_id())
			setup_vcpu_info(cpu);
}

#ifdef CONFIG_HOTPLUG_CPU

/*
 * Initialize cpu_present_map late to skip SMP boot code in init/main.c.
 * But do it early enough to catch critical for_each_present_cpu() loops
 * in i386-specific code.
 */
static int __init initialize_cpu_present_map(void)
{
	unsigned int cpu;

	for_each_possible_cpu(cpu)
		set_cpu_present(cpu, true);

	return 0;
}
core_initcall(initialize_cpu_present_map);

int __cpu_disable(void)
{
	unsigned int cpu = smp_processor_id();

	if (cpu == 0)
		return -EBUSY;

	set_cpu_online(cpu, false);
	fixup_irqs();

	return 0;
}

void __cpu_die(unsigned int cpu)
{
	while (HYPERVISOR_vcpu_op(VCPUOP_is_up, cpu, NULL)) {
		current->state = TASK_UNINTERRUPTIBLE;
		schedule_timeout(HZ/10);
	}

	xen_smp_intr_exit(cpu);
}

#endif /* CONFIG_HOTPLUG_CPU */

int __cpu_up(unsigned int cpu, struct task_struct *idle)
{
	int rc;

	rc = cpu_up_check(cpu);
	if (rc)
		return rc;

	setup_vsyscall_time_area(cpu);

	rc = xen_smp_intr_init(cpu);
	if (rc)
		return rc;

#ifdef CONFIG_X86_64
	clear_tsk_thread_flag(idle, TIF_FORK);
	per_cpu(kernel_stack, cpu) = (unsigned long)task_stack_page(idle) -
				     KERNEL_STACK_OFFSET + THREAD_SIZE;
#endif
 	per_cpu(current_task, cpu) = idle;

	cpu_initialize_context(cpu, idle->thread.sp0);

	if (num_online_cpus() == 1)
		alternatives_enable_smp();

	/* This must be done before setting cpu_online_map */
	wmb();

	rc = HYPERVISOR_vcpu_op(VCPUOP_up, cpu, NULL);
	if (!rc) {
		/* Wait 5s total for a response. */
		unsigned long timeout = jiffies + 5 * HZ;

		while ((!cpu_online(cpu) || !cpu_active(cpu)) &&
			time_before_eq(jiffies, timeout))
			HYPERVISOR_yield();
		if (!cpu_online(cpu) || !cpu_active(cpu)) {
			VOID(HYPERVISOR_vcpu_op(VCPUOP_down, cpu, NULL));
			rc = -ETIMEDOUT;
		}
	}

	if (rc)
		xen_smp_intr_exit(cpu);

	return rc;
}

void __ref play_dead(void)
{
#ifdef CONFIG_HOTPLUG_CPU
	idle_task_exit();
	local_irq_disable();
	cpumask_clear_cpu(smp_processor_id(), cpu_initialized_mask);
	preempt_enable_no_resched();
	VOID(HYPERVISOR_vcpu_op(VCPUOP_down, smp_processor_id(), NULL));
	cpu_bringup();
	tick_nohz_idle_enter();
#else
	BUG();
#endif
}

void __init smp_cpus_done(unsigned int max_cpus)
{
	nmi_selftest();
}

#ifndef CONFIG_X86_LOCAL_APIC
int setup_profiling_timer(unsigned int multiplier)
{
	return -EINVAL;
}
#endif
