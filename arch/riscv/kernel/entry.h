/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2019 SiFive, Inc.
 */
#ifndef __ASM_ENTRY_H
#define __ASM_ENTRY_H

#include <linux/linkage.h>
#include <linux/init.h>

asmlinkage void do_trap_unknown(struct pt_regs *regs);
asmlinkage void do_trap_insn_misaligned(struct pt_regs *regs);
asmlinkage void do_trap_insn_fault(struct pt_regs *regs);
asmlinkage void do_trap_insn_illegal(struct pt_regs *regs);
asmlinkage void do_trap_load_misaligned(struct pt_regs *regs);
asmlinkage void do_trap_load_fault(struct pt_regs *regs);
asmlinkage void do_trap_store_misaligned(struct pt_regs *regs);
asmlinkage void do_trap_store_fault(struct pt_regs *regs);
asmlinkage void do_trap_ecall_u(struct pt_regs *regs);
asmlinkage void do_trap_ecall_s(struct pt_regs *regs);
asmlinkage void do_trap_ecall_m(struct pt_regs *regs);
asmlinkage void do_trap_break(struct pt_regs *regs);

asmlinkage void do_notify_resume(struct pt_regs *regs,
				 unsigned long thread_info_flags);

void __init trap_init(void);

#endif /* __ASM__H */
