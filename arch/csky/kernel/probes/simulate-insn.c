// SPDX-License-Identifier: GPL-2.0+

#include <linux/bitops.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>

#include "simulate-insn.h"

void __kprobes
simulate_subisp16(u32 opcode, long addr, struct pt_regs *regs)
{
	u32 imm;

	/*
	 * Immediate value layout in subisp16:
	 * 0001 01xx 001x xxxx
	 *      IMM2    IMM5
	 * sp = sp - {IMM2, IMM5}
	 */
	imm  = opcode & 0x1f;
	imm |= (opcode & 0x300) >> 8 << 5;
	imm  = imm << 2;

	regs->usp -= imm;
}
