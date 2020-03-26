/*
 * svos_idle.c - native hardware idle loop for modern Intel processors
 *
 * Copyright (c) 2010, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */
#define MSR_IA32_POWER_CTL    0x000001fc
#define MWAIT_SUBSTATE_MASK      0xf
#define MWAIT_CSTATE_MASK     0xf
#define MWAIT_SUBSTATE_SIZE      4
#define MWAIT_HINT2CSTATE(hint)     (((hint) >> MWAIT_SUBSTATE_SIZE) & MWAIT_CSTATE_MASK)
#define MWAIT_HINT2SUBSTATE(hint)   ((hint) & MWAIT_CSTATE_MASK)

//
// defines to identify commands to the idle driver from
// the svkernel module when performing actions for the scheduler
// node.
//
#define	SET_SVFS_POLL_IDLE	0x8001	// set a pointer to handle poll idle.
#define	SET_SVFS_IDLE		0x8002  // change the current idle handler.
#define	GET_SVFS_IDLE		0x8003  // get the current idle handler.
#define	GET_SVFS_CSTATE		0x8004  // get the current cstate.
#define	SET_SVFS_CSTATE		0x8005  // set the current cstate.
#define CLEANUP_SVFS		0x8006	// tell driver sv unmounted
#define INIT_SVFS_IDLE		0x8007  // perform initialization for svfs.

enum svos_idle_routine_ids {
       SVOS_ORIG_IDLE, // idle routine in place when we booted
       SVOS_DEF_IDLE,          // default_idle (whatever it is)
       SVOS_MWAIT_IDLE,        // default linux mwait_idle
       SVOS_POLL_IDLE, // Traditional poll idle routine
       SVOS_ACPI_IDLE, // 2.6 compatibility
       SVOS_SVFS_POLL_IDLE,    // Custom svkernel svos_idle routine
       SVOS_HALT_IDLE,         // Kernel idle routine using hlt instruction
       SVOS_NUM_IDLE           // Must terminate list, count of number of entries
};

extern void default_idle(void);
/*
 * svos_idle is a cpuidle driver that loads on specific Intel processors
 * in lieu of the legacy ACPI processor_idle driver.  The intent is to
 * make Linux more efficient on these processors, as svos_idle knows
 * more than ACPI, as well as make Linux more immune to ACPI BIOS bugs.
 */

/*
 * Design Assumptions
 *
 * All CPUs have same idle states as boot CPU
 *
 * Chipset BM_STS (bus master status) bit is a NOP
 *	for preventing entry into deep C-stats
 */

/*
 * Known limitations
 *
 * The driver currently initializes for_each_online_cpu() upon modprobe.
 * It it unaware of subsequent processors hot-added to the system.
 * This means that if you boot with maxcpus=n and later online
 * processors above n, those processors will use C1 only.
 *
 * ACPI has a .suspend hack to turn off deep c-statees during suspend
 * to avoid complications with the lapic timer workaround.
 * Have not seen issues with suspend, but may need same workaround here.
 *
 * There is currently no kernel-based automatic probing/loading mechanism
 * if the driver is built as a module.
 */

/* un-comment DEBUG to enable pr_debug() statements */
#define DEBUG

#include <linux/kernel.h>
#include <linux/cpuidle.h>
#include <linux/clockchips.h>
#include <trace/events/power.h>
#include <linux/sched.h>
#include <linux/notifier.h>
#include <linux/cpu.h>
#include <linux/module.h>
#include <asm/cpu_device_id.h>
#include <asm/mwait.h>
#include <asm/msr.h>
#include <linux/svos.h>
#include <linux/version.h>
#include <linux/kallsyms.h>
#include <linux/tick.h>

#define SVOS_IDLE_VERSION "0.1"
#define PREFIX "svos_idle: "
// Interface for changing idle routine and values they use.
int custom_idle = SVOS_POLL_IDLE;		// If set, this is the index into table of pointers to various idle routines
//	If mwait_c_state is non-zero, use it as the cstate to use. We assume you know what you are doing!
int mwait_c_state = 0;

// per-cpu flag used only by svos_idle driver (default) to stay in cstate after interrupts
int no_break_on_int[NR_CPUS] = {0};

static struct cpuidle_driver svos_idle_driver = {
	.name = "svos_idle",
	.owner = THIS_MODULE,
};
/* svos_idle.max_cstate=0 disables driver */
static int max_cstate = CPUIDLE_STATE_MAX - 1;

static unsigned int mwait_substates;

#define LAPIC_TIMER_ALWAYS_RELIABLE 0xFFFFFFFF
/* Reliable LAPIC Timer States, bit 1 for C1 etc.  */
static unsigned int lapic_timer_reliable_states = (1 << 1);	 /* Default to only C1 */

struct idle_cpu {
	struct cpuidle_state *state_table;

	/*
	 * Hardware C-state auto-demotion may not always be optimal.
	 * Indicate which enable bits to clear here.
	 */
	unsigned long auto_demotion_disable_flags;
	bool disable_promotion_to_c1e;
};

static const struct idle_cpu *icpu;
static struct cpuidle_device __percpu *svos_idle_cpuidle_devices;
static int svos_idle(struct cpuidle_device *dev,
			struct cpuidle_driver *drv, int index);
static int svos_idle_cpu_init(int cpu);

static struct cpuidle_state *cpuidle_state_table;

/*
 * Set this flag for states where the HW flushes the TLB for us
 * and so we don't need cross-calls to keep it consistent.
 * If this flag is set, SW flushes the TLB, so even if the
 * HW doesn't do the flushing, this flag is safe to use.
 */
#define CPUIDLE_FLAG_TLB_FLUSHED	0x10000

/*
 * MWAIT takes an 8-bit "hint" in EAX "suggesting"
 * the C-state (top nibble) and sub-state (bottom nibble)
 * 0x00 means "MWAIT(C1)", 0x10 means "MWAIT(C2)" etc.
 *
 * We store the hint at the top of our "flags" for each state.
 */
#define flg2MWAIT(flags) (((flags) >> 24) & 0xFF)
#define MWAIT2flg(eax) ((eax & 0xFF) << 24)

/*
 * States are indexed by the cstate number,
 * which is also the index into the MWAIT hint array.
 * Thus C0 is a dummy.
 */
static struct cpuidle_state def_cstates[CPUIDLE_STATE_MAX] = {
   {
      .name = "C1-DEF",
      .desc = "MWAIT 0x00",
      .flags = MWAIT2flg(0x00),
      .exit_latency = 1,
      .target_residency = 1,
      .enter = &svos_idle },
};

static struct cpuidle_state nehalem_cstates[CPUIDLE_STATE_MAX] = {
	{
		.name = "C1-NHM",
		.desc = "MWAIT 0x00",
		.flags = MWAIT2flg(0x00),
		.exit_latency = 3,
		.target_residency = 6,
		.enter = &svos_idle },
	{
		.name = "C1E-NHM",
		.desc = "MWAIT 0x01",
		.flags = MWAIT2flg(0x01),
		.exit_latency = 10,
		.target_residency = 20,
		.enter = &svos_idle },
	{
		.name = "C3-NHM",
		.desc = "MWAIT 0x10",
		.flags = MWAIT2flg(0x10) | CPUIDLE_FLAG_TLB_FLUSHED,
		.exit_latency = 20,
		.target_residency = 80,
		.enter = &svos_idle },
	{
		.name = "C6-NHM",
		.desc = "MWAIT 0x20",
		.flags = MWAIT2flg(0x20) | CPUIDLE_FLAG_TLB_FLUSHED,
		.exit_latency = 200,
		.target_residency = 800,
		.enter = &svos_idle },
	{
		.enter = NULL }
};

static struct cpuidle_state snb_cstates[CPUIDLE_STATE_MAX] = {
	{
		.name = "C1-SNB",
		.desc = "MWAIT 0x00",
		.flags = MWAIT2flg(0x00),
		.exit_latency = 2,
		.target_residency = 2,
		.enter = &svos_idle },
	{
		.name = "C1E-SNB",
		.desc = "MWAIT 0x01",
		.flags = MWAIT2flg(0x01),
		.exit_latency = 10,
		.target_residency = 20,
		.enter = &svos_idle },
	{
		.name = "C3-SNB",
		.desc = "MWAIT 0x10",
		.flags = MWAIT2flg(0x10) | CPUIDLE_FLAG_TLB_FLUSHED,
		.exit_latency = 80,
		.target_residency = 211,
		.enter = &svos_idle },
	{
		.name = "C6-SNB",
		.desc = "MWAIT 0x20",
		.flags = MWAIT2flg(0x20) | CPUIDLE_FLAG_TLB_FLUSHED,
		.exit_latency = 104,
		.target_residency = 345,
		.enter = &svos_idle },
	{
		.name = "C7-SNB",
		.desc = "MWAIT 0x30",
		.flags = MWAIT2flg(0x30) | CPUIDLE_FLAG_TLB_FLUSHED,
		.exit_latency = 109,
		.target_residency = 345,
		.enter = &svos_idle },
	{
		.enter = NULL }
};

static struct cpuidle_state ivb_cstates[CPUIDLE_STATE_MAX] = {
	{
		.name = "C1-IVB",
		.desc = "MWAIT 0x00",
		.flags = MWAIT2flg(0x00),
		.exit_latency = 1,
		.target_residency = 1,
		.enter = &svos_idle },
	{
		.name = "C1E-IVB",
		.desc = "MWAIT 0x01",
		.flags = MWAIT2flg(0x01),
		.exit_latency = 10,
		.target_residency = 20,
		.enter = &svos_idle },
	{
		.name = "C3-IVB",
		.desc = "MWAIT 0x10",
		.flags = MWAIT2flg(0x10) | CPUIDLE_FLAG_TLB_FLUSHED,
		.exit_latency = 59,
		.target_residency = 156,
		.enter = &svos_idle },
	{
		.name = "C6-IVB",
		.desc = "MWAIT 0x20",
		.flags = MWAIT2flg(0x20) | CPUIDLE_FLAG_TLB_FLUSHED,
		.exit_latency = 80,
		.target_residency = 300,
		.enter = &svos_idle },
	{
		.name = "C7-IVB",
		.desc = "MWAIT 0x30",
		.flags = MWAIT2flg(0x30) | CPUIDLE_FLAG_TLB_FLUSHED,
		.exit_latency = 87,
		.target_residency = 300,
		.enter = &svos_idle },
	{
		.enter = NULL }
};

static struct cpuidle_state hsw_cstates[CPUIDLE_STATE_MAX] = {
	{
		.name = "C1-HSW",
		.desc = "MWAIT 0x00",
		.flags = MWAIT2flg(0x00),
		.exit_latency = 2,
		.target_residency = 2,
		.enter = &svos_idle },
	{
		.name = "C1E-HSW",
		.desc = "MWAIT 0x01",
		.flags = MWAIT2flg(0x01),
		.exit_latency = 10,
		.target_residency = 20,
		.enter = &svos_idle },
	{
		.name = "C3-HSW",
		.desc = "MWAIT 0x10",
		.flags = MWAIT2flg(0x10) | CPUIDLE_FLAG_TLB_FLUSHED,
		.exit_latency = 33,
		.target_residency = 100,
		.enter = &svos_idle },
	{
		.name = "C6-HSW",
		.desc = "MWAIT 0x20",
		.flags = MWAIT2flg(0x20) | CPUIDLE_FLAG_TLB_FLUSHED,
		.exit_latency = 133,
		.target_residency = 400,
		.enter = &svos_idle },
	{
		.name = "C7s-HSW",
		.desc = "MWAIT 0x32",
		.flags = MWAIT2flg(0x32) | CPUIDLE_FLAG_TLB_FLUSHED,
		.exit_latency = 166,
		.target_residency = 500,
		.enter = &svos_idle },
	{
		.name = "C8-HSW",
		.desc = "MWAIT 0x40",
		.flags = MWAIT2flg(0x40) | CPUIDLE_FLAG_TLB_FLUSHED,
		.exit_latency = 300,
		.target_residency = 900,
		.enter = &svos_idle },
	{
		.name = "C9-HSW",
		.desc = "MWAIT 0x50",
		.flags = MWAIT2flg(0x50) | CPUIDLE_FLAG_TLB_FLUSHED,
		.exit_latency = 600,
		.target_residency = 1800,
		.enter = &svos_idle },
	{
		.name = "C10-HSW",
		.desc = "MWAIT 0x60",
		.flags = MWAIT2flg(0x60) | CPUIDLE_FLAG_TLB_FLUSHED,
		.exit_latency = 2600,
		.target_residency = 7700,
		.enter = &svos_idle },
	{
		.enter = NULL }
};

static struct cpuidle_state atom_cstates[CPUIDLE_STATE_MAX] = {
	{
		.name = "C1E-ATM",
		.desc = "MWAIT 0x00",
		.flags = MWAIT2flg(0x00),
		.exit_latency = 10,
		.target_residency = 20,
		.enter = &svos_idle },
	{
		.name = "C2-ATM",
		.desc = "MWAIT 0x10",
		.flags = MWAIT2flg(0x10),
		.exit_latency = 20,
		.target_residency = 80,
		.enter = &svos_idle },
	{
		.name = "C4-ATM",
		.desc = "MWAIT 0x30",
		.flags = MWAIT2flg(0x30) | CPUIDLE_FLAG_TLB_FLUSHED,
		.exit_latency = 100,
		.target_residency = 400,
		.enter = &svos_idle },
	{
		.name = "C6-ATM",
		.desc = "MWAIT 0x52",
		.flags = MWAIT2flg(0x52) | CPUIDLE_FLAG_TLB_FLUSHED,
		.exit_latency = 140,
		.target_residency = 560,
		.enter = &svos_idle },
	{
		.enter = NULL }
};

static struct cpuidle_state knl_cstates[CPUIDLE_STATE_MAX] = {
	{
		.name = "C1-KNL",
		.desc = "MWAIT 0x00",
		.flags = MWAIT2flg(0x00),
		.exit_latency = 10,
		.target_residency = 20,
		.enter = &svos_idle },
	{
		.name = "C6-KNL",
		.desc = "MWAIT 0x10",
		.flags = MWAIT2flg(0x10) | CPUIDLE_FLAG_TLB_FLUSHED,
		.exit_latency = 38,
		.target_residency = 90,
		.enter = &svos_idle },
	{
		.enter = NULL }
};

/* exported so svkernel can customize the idle routine.
	If custom_idle is non-zero, use it to index into array of idle routines. See include/linux/svos.h 
	If mwait_c_state is non-zero, use it as the cstate to use. We assume you know what you are doing!
	If no_break_on_int is set, clear the break_on_int bit so interrupts do not break from cstates
	(in default svos_init idle routine.)
*/
void ** svos_idle_routines[SVOS_NUM_IDLE] = {0};
EXPORT_SYMBOL(svos_idle_routines);
EXPORT_SYMBOL(custom_idle);
EXPORT_SYMBOL(mwait_c_state);
EXPORT_SYMBOL(no_break_on_int);

/**
 * svos_idle
 * @dev: cpuidle_device
 * @drv: cpuidle driver
 * @index: index of cpuidle state
 *
 * Must be called under local_irq_disable().
 */
static int svos_idle(struct cpuidle_device *dev,
		struct cpuidle_driver *drv, int index)
{
	unsigned long ecx = 1; /* break on interrupt flag */
	struct cpuidle_state *state = &drv->states[index];
	unsigned long eax = flg2MWAIT(state->flags);
	unsigned int cstate;
	void (* idle_p)(struct cpuidle_device *, struct cpuidle_driver *, int);
	int cpu = smp_processor_id();

	/****************************************************************************************
	 * custom_idle == 0 means we have no custom routine, so we use svos_driver (mwait) routine
	 * custom_idle == SVOS_MWAIT_IDLE means we want mwait, so we use svos_driver (mwait) routine
	 ***************************************************************************************/
	if((custom_idle != 0) && (custom_idle != SVOS_MWAIT_IDLE) && svos_idle_routines[custom_idle]) {
		idle_p = (void *)svos_idle_routines[custom_idle];
		if(idle_p) {
			idle_p(dev, drv, index);
			return index;
		}
	}

	// If they specified a cstate to use, use that one
	if(mwait_c_state) {
		eax = mwait_c_state;
	}
	cstate = (((eax) >> MWAIT_SUBSTATE_SIZE) & MWAIT_CSTATE_MASK) + 1;

	if(no_break_on_int[cpu])
		ecx=0;

	/*
	 * leave_mm() to avoid costly and often unnecessary wakeups
	 * for flushing the user TLB's associated with the active mm.
	 */
	if ( (state->flags & CPUIDLE_FLAG_TLB_FLUSHED) || (eax >= 0x10))
		leave_mm(cpu);

	if (!(lapic_timer_reliable_states & (1 << (cstate))))
		tick_broadcast_enter();

	stop_critical_timings();
	if (!need_resched()) {

		__monitor((void *)&current_thread_info()->flags, 0, 0);
		smp_mb();
		if (!need_resched())
			__mwait(eax, ecx);
	}

	start_critical_timings();

	if (!(lapic_timer_reliable_states & (1 << (cstate))))
		tick_broadcast_exit();

	return index;
}

static void __setup_broadcast_timer(void *arg)
{
	unsigned long reason = (unsigned long)arg;

	if (reason)
		tick_broadcast_enable();
	else
		tick_broadcast_disable();

}

static void auto_demotion_disable(void *dummy)
{
	unsigned long long msr_bits;

	rdmsrl(MSR_PKG_CST_CONFIG_CONTROL, msr_bits);
	msr_bits &= ~(icpu->auto_demotion_disable_flags);
	wrmsrl(MSR_PKG_CST_CONFIG_CONTROL, msr_bits);
}
static void c1e_promotion_disable(void *dummy)
{
	unsigned long long msr_bits;

	rdmsrl(MSR_IA32_POWER_CTL, msr_bits);
	msr_bits &= ~0x2;
	wrmsrl(MSR_IA32_POWER_CTL, msr_bits);
}

static const struct idle_cpu idle_cpu_default = {
	.state_table = def_cstates,
};

static const struct idle_cpu idle_cpu_nehalem = {
	.state_table = nehalem_cstates,
	.auto_demotion_disable_flags = NHM_C1_AUTO_DEMOTE | NHM_C3_AUTO_DEMOTE,
	.disable_promotion_to_c1e = true,
};

static const struct idle_cpu idle_cpu_atom = {
	.state_table = atom_cstates,
};

static const struct idle_cpu idle_cpu_lincroft = {
	.state_table = atom_cstates,
	.auto_demotion_disable_flags = ATM_LNC_C6_AUTO_DEMOTE,
};

static const struct idle_cpu idle_cpu_snb = {
	.state_table = snb_cstates,
	.disable_promotion_to_c1e = true,
};

static const struct idle_cpu idle_cpu_ivb = {
	.state_table = ivb_cstates,
	.disable_promotion_to_c1e = true,
};

static const struct idle_cpu idle_cpu_hsw = {
	.state_table = hsw_cstates,
	.disable_promotion_to_c1e = true,
};

static const struct idle_cpu idle_cpu_knl = {
	.state_table = knl_cstates,
};

#define ICPU(model, cpu) \
	{ X86_VENDOR_INTEL, 6, model, X86_FEATURE_MWAIT, (unsigned long)&cpu }

static const struct x86_cpu_id svos_idle_ids[] = {
	ICPU(0x1a, idle_cpu_nehalem),
	ICPU(0x1e, idle_cpu_nehalem),
	ICPU(0x1f, idle_cpu_nehalem),
	ICPU(0x25, idle_cpu_nehalem),
	ICPU(0x2c, idle_cpu_nehalem),
	ICPU(0x2e, idle_cpu_nehalem),
	ICPU(0x1c, idle_cpu_atom),
	ICPU(0x26, idle_cpu_lincroft),
	ICPU(0x2f, idle_cpu_nehalem),
	ICPU(0x2a, idle_cpu_snb),
	ICPU(0x2d, idle_cpu_snb),
	ICPU(0x3a, idle_cpu_ivb),
	ICPU(0x3e, idle_cpu_ivb),
	ICPU(0x3c, idle_cpu_hsw),
	ICPU(0x3f, idle_cpu_hsw),
	ICPU(0x45, idle_cpu_hsw),
	ICPU(0x46, idle_cpu_hsw),
	ICPU(0x57, idle_cpu_knl),
	ICPU(X86_MODEL_ANY, idle_cpu_default),
	ICPU(15, idle_cpu_default),
	{}
};
MODULE_DEVICE_TABLE(x86cpu, svos_idle_ids);

/*
 * svos_idle_probe()
 */
static int svos_idle_probe(void)
{
	unsigned int eax, ebx, ecx;
	const struct x86_cpu_id *id;

	if (max_cstate == 0) {
		pr_debug(PREFIX "disabled\n");
		return -EPERM;
	}

	id = x86_match_cpu(svos_idle_ids);
	if (!id) {
		if (boot_cpu_data.x86_vendor == X86_VENDOR_INTEL &&
		    boot_cpu_data.x86 == 6)
			pr_debug(PREFIX "does not run on family %d model %d\n",
				boot_cpu_data.x86, boot_cpu_data.x86_model);
		return -ENODEV;
	}

	if (boot_cpu_data.cpuid_level < CPUID_MWAIT_LEAF)
		return -ENODEV;

	cpuid(CPUID_MWAIT_LEAF, &eax, &ebx, &ecx, &mwait_substates);

	if (!(ecx & CPUID5_ECX_EXTENSIONS_SUPPORTED) ||
	    !(ecx & CPUID5_ECX_INTERRUPT_BREAK) ||
	    !mwait_substates)
			return -ENODEV;

	pr_debug(PREFIX "MWAIT substates: 0x%x\n", mwait_substates);

	icpu = (const struct idle_cpu *)id->driver_data;
	cpuidle_state_table = icpu->state_table;

	if (boot_cpu_has(X86_FEATURE_ARAT))	/* Always Reliable APIC Timer */
		lapic_timer_reliable_states = LAPIC_TIMER_ALWAYS_RELIABLE;
	else
		on_each_cpu(__setup_broadcast_timer, (void *)true, 1);

	pr_debug(PREFIX "v" SVOS_IDLE_VERSION
		" model 0x%X\n", boot_cpu_data.x86_model);

	pr_debug(PREFIX "lapic_timer_reliable_states 0x%x\n",
		lapic_timer_reliable_states);
	return 0;
}

/*
 * svos_idle_cpuidle_devices_uninit()
 * unregister, free cpuidle_devices
 */
static void svos_idle_cpuidle_devices_uninit(void)
{
	int i;
	struct cpuidle_device *dev;

	for_each_online_cpu(i) {
		dev = per_cpu_ptr(svos_idle_cpuidle_devices, i);
		cpuidle_unregister_device(dev);
	}

	free_percpu(svos_idle_cpuidle_devices);
	return;
}
/*
 * svos_idle_cpuidle_driver_init()
 * allocate, initialize cpuidle_states
 */
static int svos_idle_cpuidle_driver_init(void)
{
	int cstate;
	struct cpuidle_driver *drv = &svos_idle_driver;

	drv->state_count = 1;

	for (cstate = 0; cstate < CPUIDLE_STATE_MAX; ++cstate) {
		int num_substates, mwait_hint, mwait_cstate, mwait_substate;

		if (cpuidle_state_table[cstate].enter == NULL)
			break;

		if (cstate + 1 > max_cstate) {
			printk(PREFIX "max_cstate %d reached\n",
				max_cstate);
			break;
		}

		mwait_hint = flg2MWAIT(cpuidle_state_table[cstate].flags);
		mwait_cstate = MWAIT_HINT2CSTATE(mwait_hint);
		mwait_substate = MWAIT_HINT2SUBSTATE(mwait_hint);

		/* does the state exist in CPUID.MWAIT? */
		num_substates = (mwait_substates >> ((mwait_cstate + 1) * 4))
					& MWAIT_SUBSTATE_MASK;

		/* if sub-state in table is not enumerated by CPUID */
		if ((mwait_substate + 1) > num_substates)
			continue;

		if (((mwait_cstate + 1) > 2) &&
			!boot_cpu_has(X86_FEATURE_NONSTOP_TSC))
			mark_tsc_unstable("TSC halts in idle"
					" states deeper than C2");

		drv->states[drv->state_count] =	/* structure copy */
			cpuidle_state_table[cstate];

		drv->state_count += 1;
	}

	if (icpu->auto_demotion_disable_flags)
		on_each_cpu(auto_demotion_disable, NULL, 1);

	if (icpu->disable_promotion_to_c1e)	/* each-cpu is redundant */
		on_each_cpu(c1e_promotion_disable, NULL, 1);

	return 0;
}


/*
 * svos_idle_cpu_init()
 * allocate, initialize, register cpuidle_devices
 * @cpu: cpu/core to initialize
 */
static int svos_idle_cpu_init(int cpu)
{
	struct cpuidle_device *dev;

	dev = per_cpu_ptr(svos_idle_cpuidle_devices, cpu);


	dev->cpu = cpu;

	if (cpuidle_register_device(dev)) {
		pr_debug(PREFIX "cpuidle_register_device %d failed!\n", cpu);
		svos_idle_cpuidle_devices_uninit();
		return -EIO;
	}

	if (icpu->auto_demotion_disable_flags)
		smp_call_function_single(cpu, auto_demotion_disable, NULL, 1);

	if (icpu->disable_promotion_to_c1e)
		smp_call_function_single(cpu, c1e_promotion_disable, NULL, 1);

	return 0;
}

static int __init svos_idle_init(void)
{
	int retval, i;

	/* Do not load svos_idle at all for now if idle= is passed */
	if (boot_option_idle_override != IDLE_NO_OVERRIDE) {
		pr_debug(PREFIX "boot_option_idle_override is not zero, remove idle= boot option to use svos_idle driver!\n");
		return -ENODEV;
	}
	svos_idle_routines[SVOS_POLL_IDLE] = (void *)kallsyms_lookup_name("poll_idle");
	svos_idle_routines[SVOS_DEF_IDLE] = (void *)&default_idle;
	svos_idle_routines[SVOS_HALT_IDLE] = (void *)&default_idle;
	
	retval = svos_idle_probe();
	if (retval)
		return retval;

	svos_idle_cpuidle_driver_init();
	retval = cpuidle_register_driver(&svos_idle_driver);
	if (retval) {
		struct cpuidle_driver *drv = cpuidle_get_driver();
		printk(KERN_DEBUG PREFIX "svos_idle yielding to %s",
			drv ? drv->name : "none");
		return retval;
	}

	svos_idle_cpuidle_devices = alloc_percpu(struct cpuidle_device);
	if (svos_idle_cpuidle_devices == NULL)
		return -ENOMEM;

	for_each_online_cpu(i) {
		retval = svos_idle_cpu_init(i);
		if (retval) {
			cpuidle_unregister_driver(&svos_idle_driver);
			return retval;
		}
	}
	/*
	 *	See if they have changed the default idle policy.  The boot parameter svos_idle.policy
	 *	can have values poll, halt or mwait.  We use this to set custom_idle.
	 * A value in custom_idle of 0 indicates the svos driver uses its embedded mwait policy.
	 * A value of "halt" will invoke the default_idle routine, which uses "halt".
	 * A value of "poll" will invoke the poll_idle routine if we can locate the address of that routine.
	 * A value of "mwait" will set coustom_idle to 0 so we use the  embedded mwait policy of this driver.
	 */
	if((custom_idle == 0) || (custom_idle == SVOS_MWAIT_IDLE)) {
		custom_idle = 0;
		printk(KERN_DEBUG "%s using mwait idle policy\n",__FUNCTION__);
	} else if(svos_idle_routines[custom_idle] == NULL) {
		if(svos_idle_routines[SVOS_POLL_IDLE] != NULL) {
			custom_idle = SVOS_POLL_IDLE;
			printk(KERN_DEBUG "%s Using poll idle for initial idle loop routine\n",__FUNCTION__);
		} else {
			svos_idle_routines[SVOS_DEF_IDLE] = (void *)&default_idle;
			custom_idle = SVOS_DEF_IDLE;
			boot_option_idle_override = IDLE_HALT;
			printk(KERN_DEBUG "%s using halt idle policy\n",__FUNCTION__);
		}
	}
	
	return 0;
}

static void __exit svos_idle_exit(void)
{
	svos_idle_cpuidle_devices_uninit();
	cpuidle_unregister_driver(&svos_idle_driver);


	if (lapic_timer_reliable_states != LAPIC_TIMER_ALWAYS_RELIABLE)
		on_each_cpu(__setup_broadcast_timer, (void *)false, 1);
    
	return;
}

module_init(svos_idle_init);
module_exit(svos_idle_exit);

module_param(max_cstate, int, 0444);
module_param(custom_idle, int, 0644);
module_param(mwait_c_state, int, 0644);

static int __init idle_setup(char *str)
{
	if (!str)
		return -EINVAL;

	if (!strcmp(str, "poll")) {
		pr_info("using polling idle threads.\n");
		cpu_idle_poll_ctrl(true);
		custom_idle = SVOS_POLL_IDLE;
		return 0;
	} else if (!strcmp(str, "halt")) {
		pr_info("using halt idle threads.\n");
		custom_idle = SVOS_DEF_IDLE;
		return 0;
	} else if (!strcmp(str, "mwait")) {
		pr_info("using mwait idle threads.\n");
		custom_idle = 0;
		return 0;
	}
	return -1;
}

/*
 *	To configure which policy to use from the boot command line use svos_idle.policy=poll (or mwait or halt)
 * For conformity with legacy kernel parameters, we also support svos_idle.idle=poll (or mwait or halt)
 *
 * To dynamically view or change the policy, read or write the node /sys/modules/svos_idle/parameters/policy
 * cat /sys/module/svos_idle/parameters/policy
 *		mwait halt [poll]	 indicates the choices atr mwait, halt, and poll and poll is currently in effect.
 * echo "mwait">/sys/module/svos_idle/parameters/policy  will switch to using mwait 
 */


early_param("idle", idle_setup);

static int __init policy_setup(char *str)
{
	if (!str)
		return -EINVAL;
	return idle_setup(str);
}

early_param("policy", policy_setup);

static const char *policy_str[] = {
   [SVOS_ORIG_IDLE] = "mwait",
   [SVOS_DEF_IDLE] = "halt",
   [SVOS_MWAIT_IDLE] = "mwait",
   [SVOS_POLL_IDLE] = "poll",
};

static int svos_idle_set_policy(const char *val, const struct kernel_param *kp)
{
	int i;
	for (i = 1; i < ARRAY_SIZE(policy_str); i++) {
		if (!strncmp(val, policy_str[i], strlen(policy_str[i]))) {
			break;
		}
	}
	if (i >= ARRAY_SIZE(policy_str))
		return -EINVAL;

	if (i == custom_idle) {
		return 0;
	}
	switch (i) {
	case 0:
	case SVOS_MWAIT_IDLE:
		custom_idle = 0;
		boot_option_idle_override = IDLE_NO_OVERRIDE;
		printk(KERN_DEBUG "%s Switch idle policy to mwait\n",__FUNCTION__);
		return 0;
		break;
	case SVOS_POLL_IDLE:
		if(svos_idle_routines[SVOS_POLL_IDLE] == NULL) {
			svos_idle_routines[SVOS_POLL_IDLE] = (void *)kallsyms_lookup_name("poll_idle");
			if(svos_idle_routines[SVOS_POLL_IDLE] == NULL) {
				printk(KERN_DEBUG "%s Failed to find poll_idle policy\n",__FUNCTION__);
				return -EINVAL;
			}
		}
		printk(KERN_DEBUG "%s Switch idle policy to poll\n",__FUNCTION__);
		boot_option_idle_override = IDLE_POLL;
		break;
	case SVOS_HALT_IDLE:
	case SVOS_DEF_IDLE:
		if(svos_idle_routines[SVOS_DEF_IDLE] == NULL) {
			svos_idle_routines[SVOS_DEF_IDLE] = (void *)&default_idle;
		}
		printk(KERN_DEBUG "%s Switch idle policy to halt\n",__FUNCTION__);
		boot_option_idle_override = IDLE_HALT;
		break;
	default:
		return -EINVAL;
		break;
	}
	custom_idle = i;
	return 0;
}

static int svos_idle_get_policy(char *buffer, const struct kernel_param *kp)
{
	int i, cnt = 0;
	for (i = 0; i < ARRAY_SIZE(policy_str); i++) {
		if (i == SVOS_MWAIT_IDLE)
			continue;
		if((strlen(policy_str[i]) <= 0))
			continue;
		if (i == custom_idle)
			cnt += sprintf(buffer + cnt, "[%s] ", policy_str[i]);
		else
			cnt += sprintf(buffer + cnt, "%s ", policy_str[i]);
	}
	return cnt;
}

module_param_call(policy, svos_idle_set_policy, svos_idle_get_policy,
   NULL, 0644);


//
// interface to svfs /sv/kernel/scheduler driver to manipulate the idle method.
//
int saved_custom_idle;
int
_svos_idle_driver( int cmd, void *arg)
{
	int	whichIdle;
	int	error = 0;

	switch (cmd) {

		case INIT_SVFS_IDLE:
			saved_custom_idle = custom_idle;
			svos_idle_routines[SVOS_POLL_IDLE] = arg;
			break;
			
		case SET_SVFS_POLL_IDLE:
			svos_idle_routines[SVOS_POLL_IDLE] = arg;
			break;

		case CLEANUP_SVFS:
			custom_idle = saved_custom_idle;
			break;

		case SET_SVFS_IDLE:
			whichIdle = *(unsigned int *)arg;
			if( (whichIdle < 0 || whichIdle > SVOS_NUM_IDLE) ||
				(svos_idle_routines[whichIdle] == NULL)  ){

				printk(KERN_DEBUG "%s %x invalid idle routine specifier\n",__FUNCTION__,whichIdle);
				error = -EINVAL;
				break;
			}
			if( whichIdle == SVOS_MWAIT_IDLE ){
				whichIdle = 0;
			}
			custom_idle = whichIdle;
			boot_option_idle_override = SVOS_HALT_IDLE;
			break;
			

		case GET_SVFS_IDLE:
			*(unsigned int *)arg = custom_idle;
			break;

		case GET_SVFS_CSTATE:
			*(unsigned int *)arg = mwait_c_state;
			break;

		case SET_SVFS_CSTATE:
			mwait_c_state = *(unsigned int *)arg;
			break;
		default:
			error = -EINVAL;
			break;
	}
	return error;
}
EXPORT_SYMBOL(_svos_idle_driver);

MODULE_DESCRIPTION("Cpuidle driver for SVOS" SVOS_IDLE_VERSION);
MODULE_LICENSE("GPL");
