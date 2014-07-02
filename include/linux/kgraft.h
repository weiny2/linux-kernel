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
#include <linux/kobject.h>
#include <linux/ftrace.h>
#include <linux/sched.h>

#if IS_ENABLED(CONFIG_KGRAFT)

#include <asm/kgraft.h>

#define KGR_TIMEOUT 30

struct kgr_patch;

/**
 * struct kgr_patch_fun -- state of a single function in a kGraft patch
 *
 * @name: function to patch
 * @new_name: function with new body
 * @loc_old: cache of @name's fentry
 * @loc_new: cache of @new_name's fentry
 * @ftrace_ops_slow: ftrace ops for slow (temporary) stub
 * @ftrace_ops_fast: ftrace ops for fast () stub
 */
struct kgr_patch_fun {
	struct kgr_patch *patch;

	const char *name;
	const char *new_name;

	bool abort_if_missing;
	enum kgr_patch_state {
		KGR_PATCH_INIT,
		KGR_PATCH_SLOW,
		KGR_PATCH_APPLIED,

		KGR_PATCH_REVERT_SLOW,
		KGR_PATCH_REVERTED,

		KGR_PATCH_SKIPPED,
	} state;

	unsigned long loc_old;
	unsigned long loc_new;

	struct ftrace_ops *ftrace_ops_slow;
	struct ftrace_ops *ftrace_ops_fast;

	unsigned long suse_kabi_padding[4];
};

/**
 * struct kgr_patch -- a kGraft patch
 *
 * @kobj: object representing the sysfs entry
 * @finish: waiting till it is safe to remove the module with the patch
 * @name: name of the patch (to appear in sysfs)
 * @owner: module to refcount on patching
 * @irq_use_new: per-cpu array to remember kGraft state for interrupts
 * @patches: array of @kgr_patch_fun structures
 */
struct kgr_patch {
	struct kobject kobj;
	struct completion finish;
	const char *name;
	struct module *owner;
	bool __percpu *irq_use_new;
	unsigned long suse_kabi_padding[8];
	struct kgr_patch_fun *patches[];
};

#define KGR_PATCHED_FUNCTION(_name, _new_function, abort)			\
	typeof(_new_function) __used _new_function;				\
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
		.ftrace_ops_slow = &__kgr_patch_ftrace_ops_slow_ ## _name,	\
		.ftrace_ops_fast = &__kgr_patch_ftrace_ops_fast_ ## _name,	\
	};

#define KGR_PATCH(name)		&__kgr_patch_ ## name
#define KGR_PATCH_END		NULL

extern bool kgr_in_progress;

extern int kgr_patch_kernel(struct kgr_patch *);
extern void kgr_patch_remove(struct kgr_patch *);

extern int kgr_modify_kernel(struct kgr_patch *patch, bool revert);
extern int kgr_patch_dir_add(struct kgr_patch *patch);
extern void kgr_patch_dir_del(struct kgr_patch *patch);
extern int kgr_add_files(void);
extern void kgr_remove_files(void);

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
