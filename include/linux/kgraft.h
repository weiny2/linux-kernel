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
#include <linux/list.h>
#include <linux/ftrace.h>
#include <linux/sched.h>

#if IS_ENABLED(CONFIG_KGRAFT)

#include <asm/kgraft.h>

#define KGR_TIMEOUT 2

struct kgr_patch;

/**
 * struct kgr_patch_fun -- state of a single function in a kGraft patch
 *
 * @name: function to patch
 * @new_fun: function with the new body
 * @loc_name: cache of @name's fentry
 * @loc_old: cache of the last entry for @name in the patches list
 * @loc_new: cache of @new_name's fentry
 * @ftrace_ops_slow: ftrace ops for slow (temporary) stub
 * @ftrace_ops_fast: ftrace ops for fast () stub
 */
struct kgr_patch_fun {
	struct kgr_patch *patch;

	const char *name;
	void *new_fun;

	bool abort_if_missing;
	enum kgr_patch_state {
		KGR_PATCH_INIT,
		KGR_PATCH_SLOW,
		KGR_PATCH_APPLIED,

		KGR_PATCH_REVERT_SLOW,
		KGR_PATCH_REVERTED,

		KGR_PATCH_SKIPPED,
	} state;

	unsigned long loc_name;
	unsigned long loc_old;
	unsigned long loc_new;

	struct ftrace_ops ftrace_ops_slow;
	struct ftrace_ops ftrace_ops_fast;
};

/**
 * struct kgr_patch -- a kGraft patch
 *
 * @kobj: object representing the sysfs entry
 * @list: member in patches list
 * @finish: waiting till it is safe to remove the module with the patch
 * @refs: how many patches need to be reverted before this one
 * @name: name of the patch (to appear in sysfs)
 * @owner: module to refcount on patching
 * @replace_all: revert everything applied before and apply this one instead
 * @patches: array of @kgr_patch_fun structures
 */
struct kgr_patch {
	/* internal state information */
	struct kobject kobj;
	struct list_head list;
	struct completion finish;
	unsigned int refs;

	/* a patch shall set these */
	const char *name;
	struct module *owner;
	bool replace_all;
	struct kgr_patch_fun patches[];
};

#define kgr_for_each_patch_fun(p, pf)	\
	for (pf = p->patches; pf->name; pf++)

#define KGR_PATCH(_name, _new_function, abort)	{			\
		.name = #_name,						\
		.new_fun = _new_function,				\
		.abort_if_missing = abort,				\
	}
#define KGR_PATCH_END				{ }

extern bool kgr_in_progress;
extern bool kgr_force_load_module;

extern int kgr_patch_kernel(struct kgr_patch *);
extern void kgr_patch_remove(struct kgr_patch *);

extern void kgr_unmark_processes(void);
extern int kgr_modify_kernel(struct kgr_patch *patch, bool revert);
extern int kgr_module_init(const struct module *mod);
extern int kgr_patch_dir_add(struct kgr_patch *patch);
extern void kgr_patch_dir_del(struct kgr_patch *patch);
extern int kgr_add_files(void);
extern void kgr_remove_files(void);

static inline void kgr_mark_task_in_progress(struct task_struct *p)
{
	set_tsk_thread_flag(p, TIF_KGR_IN_PROGRESS);
}

#endif /* IS_ENABLED(CONFIG_KGRAFT) */

#endif /* LINUX_KGR_H */
