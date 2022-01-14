/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_PKS_H
#define _ASM_X86_PKS_H

#ifdef CONFIG_ARCH_ENABLE_SUPERVISOR_PKEYS

void pks_setup(void);
void pks_write_current(void);
void pks_save_pt_regs(struct pt_regs *regs);
void pks_restore_pt_regs(struct pt_regs *regs);
void pks_dump_fault_info(struct pt_regs_auxiliary *aux_pt_regs);

#else /* !CONFIG_ARCH_ENABLE_SUPERVISOR_PKEYS */

static inline void pks_setup(void) { }
static inline void pks_write_current(void) { }
static inline void pks_save_pt_regs(struct pt_regs *regs) { }
static inline void pks_restore_pt_regs(struct pt_regs *regs) { }
static inline void pks_dump_fault_info(struct pt_regs_auxiliary *aux_pt_regs) { }

#endif /* CONFIG_ARCH_ENABLE_SUPERVISOR_PKEYS */


#ifdef CONFIG_PKS_TEST

bool pks_test_callback(struct pt_regs *regs);

#else /* !CONFIG_PKS_TEST */

static inline bool pks_test_callback(struct pt_regs *regs)
{
	return false;
}

#endif /* CONFIG_PKS_TEST */

#endif /* _ASM_X86_PKS_H */
