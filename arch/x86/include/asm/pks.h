/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_PKS_H
#define _ASM_X86_PKS_H

#include <linux/sched.h>

#include <asm/pkeys_common.h>

#ifdef CONFIG_ARCH_ENABLE_SUPERVISOR_PKEYS

/*  PKS supports 16 keys. Key 0 is reserved for the kernel. */
#define        PKS_KERN_DEFAULT_KEY    0
#define        PKS_NUM_KEYS            16

struct extended_pt_regs {
	u32 thread_pkrs;
	/* Keep stack 8 byte aligned */
	u32 pad;
	struct pt_regs pt_regs;
};

void setup_pks(void);

static inline void pks_init_task(struct task_struct *tsk)
{
	/*
	 * New tasks get the most restrictive PKRS value.
	 */
	tsk->thread.saved_pkrs = INIT_PKRS_VALUE;
}

void write_pkrs(u32 new_pkrs);

static inline void pks_sched_in(void)
{
	/*
	 * PKRS is only temporarily changed during specific code paths.  Only a
	 * preemption during these windows away from the default value would
	 * require updating the MSR.  write_pkrs() handles this optimization.
	 */
	write_pkrs(current->thread.saved_pkrs);
}

static inline struct extended_pt_regs *extended_pt_regs(struct pt_regs *regs)
{
	return container_of(regs, struct extended_pt_regs, pt_regs);
}

void show_extended_regs_oops(struct pt_regs *regs, unsigned error_code);

#else /* !CONFIG_ARCH_ENABLE_SUPERVISOR_PKEYS */

static inline void setup_pks(void) { }
static inline void pks_init_task(struct task_struct *tsk) { }
static inline void pks_sched_in(void) { }
static inline void show_extended_regs_oops(struct pt_regs *regs,
					   unsigned error_code) { }

#endif /* CONFIG_ARCH_ENABLE_SUPERVISOR_PKEYS */

#endif /* _ASM_X86_PKS_H */
