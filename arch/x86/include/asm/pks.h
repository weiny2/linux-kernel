/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_PKS_H
#define _ASM_X86_PKS_H

#ifdef CONFIG_ARCH_ENABLE_SUPERVISOR_PKEYS

#include <linux/pkeys.h>
#include <linux/pks-keys.h>

#include <asm/cpufeature.h>

DECLARE_PER_CPU(u32, pkrs_cache);

void pks_setup(void);
void x86_pkrs_load(struct thread_struct *thread);

/*
 * pks_write_pkrs() - Write the pkrs of the current CPU
 * @new_pkrs: New value to write to the current CPU register
 *
 * Optimizes the MSR writes by maintaining a per cpu cache.
 *
 * Context: must be called with preemption disabled
 * Context: must only be called if PKS is enabled
 *
 * It should also be noted that the underlying WRMSR(MSR_IA32_PKRS) is not
 * serializing but still maintains ordering properties similar to WRPKRU.
 * The current SDM section on PKRS needs updating but should be the same as
 * that of WRPKRU.  Quote from the WRPKRU text:
 *
 *     WRPKRU will never execute transiently. Memory accesses
 *     affected by PKRU register will not execute (even transiently)
 *     until all prior executions of WRPKRU have completed execution
 *     and updated the PKRU register.
 */
static inline void pks_write_pkrs(const u32 new_pkrs)
{
	u32 pkrs = __this_cpu_read(pkrs_cache);

	lockdep_assert_preemption_disabled();

	if (pkrs != new_pkrs) {
		__this_cpu_write(pkrs_cache, new_pkrs);
		wrmsrl(MSR_IA32_PKRS, new_pkrs);
	}
}

/*
 * See pks_set*()
 * @pkey: Key for the domain to change
 * @protection: protection bits to be used
 *
 * Protection utilizes the same protection bits specified for User pkeys
 *     PKEY_DISABLE_ACCESS
 *     PKEY_DISABLE_WRITE
 *
 */
static inline void arch_pks_update_protection(const u8 pkey,
					      const u8 protection)
{
	u32 pkrs;

	if (!cpu_feature_enabled(X86_FEATURE_PKS))
		return;

	preempt_disable();
	pkrs = pkey_update_pkval(current->thread.pkrs, pkey, protection);
	pks_write_pkrs(pkrs);
	current->thread.pkrs = pkrs;
	preempt_enable();
}

bool pks_handle_key_fault(struct pt_regs *regs, unsigned long hw_error_code,
			  unsigned long address);

#else /* !CONFIG_ARCH_ENABLE_SUPERVISOR_PKEYS */

static inline void pks_setup(void) { }
static inline void x86_pkrs_load(struct thread_struct *thread) { }

static inline bool pks_handle_key_fault(struct pt_regs *regs,
					unsigned long hw_error_code,
					unsigned long address)
{
	return false;
}

#endif /* CONFIG_ARCH_ENABLE_SUPERVISOR_PKEYS */

#endif /* _ASM_X86_PKS_H */
