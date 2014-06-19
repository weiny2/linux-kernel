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
#include <asm/uaccess.h>
#include <linux/stacktrace.h>
#include <linux/slab.h>

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
static inline int kgr_needs_lazy_migration(struct task_struct *p)
{
	struct stack_trace t;
	unsigned long *s;
	int ret;

	s = kzalloc(3 * sizeof(*s), GFP_KERNEL);
	if (!s)
		return -ENOMEM;

	t.nr_entries = 0;
	t.skip = 0;
	t.max_entries = 3;
	t.entries = s;

	save_stack_trace_tsk(p, &t);
	if (t.nr_entries > 2)
		ret = 1;
	else
		ret = 0;
	kfree(s);
	return ret;
}
#else
static inline int kgr_needs_lazy_migration(struct task_struct *p)
{
	return 1;
}
#endif

#endif
