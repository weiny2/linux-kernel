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

#ifndef LINUX_KGR_H
#define LINUX_KGR_H

#include <linux/bitops.h>
#include <linux/compiler.h>
#include <linux/ftrace.h>
#include <linux/sched.h>

#if IS_ENABLED(CONFIG_KGRAFT)

#include <asm/kgraft.h>

#define KGR_TIMEOUT 30

struct kgr_patch {
	bool __percpu *irq_use_new;
	struct kgr_patch_fun {
		const char *name;
		const char *new_name;

		bool abort_if_missing;
		bool applied;

		void *new_function;

		struct ftrace_ops *ftrace_ops_slow;
		struct ftrace_ops *ftrace_ops_fast;
	} *patches[];
};

/*
 * data structure holding locations of the source and target function
 * fentry sites to avoid repeated lookups
 */
struct kgr_loc_caches {
	unsigned long old;
	unsigned long new;
	bool __percpu *irq_use_new;
};

#define KGR_PATCHED_FUNCTION(_name, _new_function, abort)			\
	static struct ftrace_ops __kgr_patch_ftrace_ops_slow_ ## _name = {	\
		.flags = FTRACE_OPS_FL_SAVE_REGS,				\
	};									\
	static struct ftrace_ops __kgr_patch_ftrace_ops_fast_ ## _name = {	\
		.flags = FTRACE_OPS_FL_SAVE_REGS,				\
	};									\
	static struct kgr_patch_fun __kgr_patch_ ## _name = {			\
		.name = #_name,							\
		.new_name = #_new_function,					\
		.abort_if_missing = abort,					\
		.new_function = _new_function,					\
		.ftrace_ops_slow = &__kgr_patch_ftrace_ops_slow_ ## _name,	\
		.ftrace_ops_fast = &__kgr_patch_ftrace_ops_fast_ ## _name,	\
	};

#define KGR_PATCH(name)		&__kgr_patch_ ## name
#define KGR_PATCH_END		NULL

extern int kgr_start_patching(struct kgr_patch *);

static inline void kgr_mark_task_in_progress(struct task_struct *p)
{
	set_tsk_thread_flag(p, TIF_KGR_IN_PROGRESS);
}

static inline bool kgr_task_in_progress(struct task_struct *p)
{
	return test_tsk_thread_flag(p, TIF_KGR_IN_PROGRESS);
}

#endif /* IS_ENABLED(CONFIG_KGRAFT) */

#endif /* LINUX_KGR_H */
