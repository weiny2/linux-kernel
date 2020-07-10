/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_PKS_H
#define _ASM_X86_PKS_H

#include <linux/sched.h>

#include <asm/pkeys_common.h>

#ifdef CONFIG_ARCH_ENABLE_SUPERVISOR_PKEYS

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

#else /* !CONFIG_ARCH_ENABLE_SUPERVISOR_PKEYS */

static inline void setup_pks(void) { }
static inline void pks_init_task(struct task_struct *tsk) { }
static inline void pks_sched_in(void) { }

#endif /* CONFIG_ARCH_ENABLE_SUPERVISOR_PKEYS */

#endif /* _ASM_X86_PKS_H */
