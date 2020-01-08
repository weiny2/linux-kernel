/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __ASM_CSKY_KPROBES_H
#define __ASM_CSKY_KPROBES_H

#include <asm-generic/kprobes.h>

#ifdef CONFIG_KPROBES
#include <linux/types.h>
#include <linux/ptrace.h>
#include <linux/percpu.h>

#define flush_insn_slot(p)		do { } while (0)
#define kretprobe_blacklist_size	0

#include <asm/probes.h>

struct prev_kprobe {
	struct kprobe *kp;
	unsigned int status;
};

/* per-cpu kprobe control block */
struct kprobe_ctlblk {
	unsigned int kprobe_status;
	struct prev_kprobe prev_kprobe;
};

void arch_remove_kprobe(struct kprobe *p);
int kprobe_fault_handler(struct pt_regs *regs, unsigned int trapnr);
int kprobe_breakpoint_handler(struct pt_regs *regs);
void kretprobe_trampoline(void);
void __kprobes *trampoline_probe_handler(struct pt_regs *regs);

#endif /* CONFIG_KPROBES */
#endif /* __ASM_CSKY_KPROBES_H */
