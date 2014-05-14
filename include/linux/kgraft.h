#ifndef LINUX_KGR_H
#define LINUX_KGR_H

#include <linux/hardirq.h> /* for in_interrupt() */
#include <linux/init.h>
#include <linux/ftrace.h>

#if IS_ENABLED(CONFIG_KGRAFT)

static void kgr_mark_task_in_progress(struct task_struct *p);
static bool kgr_task_in_progress(struct task_struct *p);

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
	KGR_STUB_ARCH_SLOW(_name, _new_function);				\
	KGR_STUB_ARCH_FAST(_name, _new_function);				\
	extern void _new_function ## _stub_slow (unsigned long, unsigned long,	\
				       struct ftrace_ops *, struct pt_regs *);	\
	extern void _new_function ## _stub_fast (unsigned long, unsigned long,	\
				       struct ftrace_ops *, struct pt_regs *);	\
	static struct ftrace_ops __kgr_patch_ftrace_ops_slow_ ## _name = {	\
		.func = _new_function ## _stub_slow,				\
		.flags = FTRACE_OPS_FL_SAVE_REGS,				\
	};									\
	static struct ftrace_ops __kgr_patch_ftrace_ops_fast_ ## _name = {	\
		.func = _new_function ## _stub_fast,				\
		.flags = FTRACE_OPS_FL_SAVE_REGS,				\
	};									\
	static struct kgr_patch_fun __kgr_patch_ ## _name = {			\
		.name = #_name,							\
		.new_name = #_new_function,					\
		.abort_if_missing = abort,					\
		.new_function = _new_function,					\
		.ftrace_ops_slow = &__kgr_patch_ftrace_ops_slow_ ## _name,	\
		.ftrace_ops_fast = &__kgr_patch_ftrace_ops_fast_ ## _name,	\
	};									\

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
