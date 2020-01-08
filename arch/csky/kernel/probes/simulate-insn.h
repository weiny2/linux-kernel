/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __CSKY_KERNEL_KPROBES_SIMULATE_INSN_H
#define __CSKY_KERNEL_KPROBES_SIMULATE_INSN_H

void simulate_subisp16(u32 opcode, long addr, struct pt_regs *regs);

#endif /* __CSKY_KERNEL_KPROBES_SIMULATE_INSN_H */
