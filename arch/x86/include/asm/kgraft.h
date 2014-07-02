/*
 * kGraft Online Kernel Patching
 *
 *  Copyright (c) 2013-2014 SUSE
 *   Authors: Jiri Kosina
 *	      Vojtech Pavlik
 *	      Jiri Slaby
 */

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#ifndef ASM_KGR_H
#define ASM_KGR_H

#ifndef CC_USING_FENTRY
#error Your compiler has to support -mfentry for kGraft to work on x86
#endif

#include <asm/ptrace.h>
#include <linux/stacktrace.h>

static inline void kgr_set_regs_ip(struct pt_regs *regs, unsigned long ip)
{
	regs->ip = ip;
}

#ifdef CONFIG_STACKTRACE
/*
 * Tasks which are running in userspace after the patching has been started
 * can immediately be marked as migrated to the new universe.
 *
 * If this function returns non-zero (i.e. also when error happens), the task
 * needs to be migrated using kgraft lazy mechanism.
 */
static inline bool kgr_needs_lazy_migration(struct task_struct *p)
{
	unsigned long s[3];
	struct stack_trace t = {
		.nr_entries = 0,
		.skip = 0,
		.max_entries = 3,
		.entries = s,
	};

	save_stack_trace_tsk(p, &t);

	return t.nr_entries > 2;
}
#else
static inline bool kgr_needs_lazy_migration(struct task_struct *p)
{
	return true;
}
#endif

#endif
