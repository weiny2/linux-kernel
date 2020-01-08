// SPDX-License-Identifier: GPL-2.0+

#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/module.h>
#include <linux/kallsyms.h>
#include <asm/sections.h>

#include "decode-insn.h"
#include "simulate-insn.h"

#define SUBISP16_MASK 0xfce0
#define SUBISP16_VAL  0x1420

/* Return:
 *   INSN_REJECTED     If instruction is one not allowed to kprobe,
 *   INSN_GOOD_NO_SLOT If instruction is supported but doesn't use its slot.
 */
enum probe_insn __kprobes
csky_probe_decode_insn(kprobe_opcode_t *addr, struct arch_probe_insn *api)
{
	probe_opcode_t insn = le32_to_cpu(*addr);

	if ((insn & SUBISP16_MASK) == SUBISP16_VAL) {
		api->handler = simulate_subisp16;
	} else {
		pr_warn("Rejected unknown instruction %x\n", insn);
		return INSN_REJECTED;
	}

	return INSN_GOOD_NO_SLOT;
}
