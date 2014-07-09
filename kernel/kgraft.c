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

#include <linux/ftrace.h>
#include <linux/hardirq.h> /* for in_interrupt() */
#include <linux/kallsyms.h>
#include <linux/kgraft.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/percpu.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/sort.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/workqueue.h>

static int kgr_patch_code(struct kgr_patch_fun *patch_fun, bool final,
		bool revert);
static void kgr_work_fn(struct work_struct *work);

static struct workqueue_struct *kgr_wq;
static DECLARE_DELAYED_WORK(kgr_work, kgr_work_fn);
static DEFINE_MUTEX(kgr_in_progress_lock);
static LIST_HEAD(patches);
bool kgr_in_progress;
static bool kgr_initialized;
static struct kgr_patch *kgr_patch;
static bool kgr_revert;

/*
 * The stub needs to modify the RIP value stored in struct pt_regs
 * so that ftrace redirects the execution properly.
 */
static void kgr_stub_fast(unsigned long ip, unsigned long parent_ip,
		struct ftrace_ops *ops, struct pt_regs *regs)
{
	struct kgr_patch_fun *p = ops->private;

	pr_debug("kgr: fast stub: calling new code at %lx\n", p->loc_new);
	kgr_set_regs_ip(regs, p->loc_new);
}

static void kgr_stub_slow(unsigned long ip, unsigned long parent_ip,
		struct ftrace_ops *ops, struct pt_regs *regs)
{
	struct kgr_patch_fun *p = ops->private;
	bool go_old;

	if (in_interrupt())
		go_old = !*this_cpu_ptr(p->patch->irq_use_new);
	else
		go_old = kgr_task_in_progress(current);

	WARN(p->state != KGR_PATCH_SLOW && p->state != KGR_PATCH_REVERT_SLOW,
			"state is %d", p->state);

	if (p->state == KGR_PATCH_REVERT_SLOW)
		go_old = !go_old;

	if (go_old) {
		pr_debug("kgr: slow stub: calling old code at %lx\n",
				p->loc_old);
		kgr_set_regs_ip(regs, p->loc_old + MCOUNT_INSN_SIZE);
	} else {
		pr_debug("kgr: slow stub: calling new code at %lx\n",
				p->loc_new);
		kgr_set_regs_ip(regs, p->loc_new);
	}
}

static void kgr_refs_inc(void)
{
	struct kgr_patch *p;

	list_for_each_entry(p, &patches, list)
		p->refs++;
}

static void kgr_refs_dec(void)
{
	struct kgr_patch *p;

	list_for_each_entry(p, &patches, list)
		p->refs--;
}

static bool kgr_still_patching(void)
{
	struct task_struct *p;
	bool failed = false;

	read_lock(&tasklist_lock);
	for_each_process(p) {
		if (kgr_task_in_progress(p)) {
			failed = true;
			break;
		}
	}
	read_unlock(&tasklist_lock);
	return failed;
}

static void kgr_finalize(void)
{
	struct kgr_patch_fun *patch_fun;

	kgr_for_each_patch(kgr_patch, patch_fun) {
		int ret = kgr_patch_code(patch_fun, true, kgr_revert);

		if (ret < 0)
			pr_err("kgr: finalize for %s failed, trying to continue\n",
					patch_fun->name);
	}

	free_percpu(kgr_patch->irq_use_new);

	if (kgr_revert)
		module_put(kgr_patch->owner);

	mutex_lock(&kgr_in_progress_lock);
	kgr_in_progress = false;
	if (kgr_revert) {
		list_del(&kgr_patch->list);
		kgr_refs_dec();
	} else
		list_add_tail(&kgr_patch->list, &patches);
	mutex_unlock(&kgr_in_progress_lock);
}

static void kgr_work_fn(struct work_struct *work)
{
	if (kgr_still_patching()) {
		pr_info("kgr still in progress after timeout (%d)\n",
			KGR_TIMEOUT);
		/* recheck again later */
		queue_delayed_work(kgr_wq, &kgr_work, KGR_TIMEOUT * HZ);
		return;
	}

	/*
	 * victory, patching finished, put everything back in shape
	 * with as less performance impact as possible again
	 */
	pr_info("kgr succeeded\n");
	kgr_finalize();
}

static void kgr_mark_processes(void)
{
	struct task_struct *p;

	read_lock(&tasklist_lock);
	for_each_process(p)
		kgr_mark_task_in_progress(p);
	read_unlock(&tasklist_lock);
}

static void kgr_handle_processes(void)
{
	struct task_struct *p;

	read_lock(&tasklist_lock);
	for_each_process(p) {
		/* wake up kthreads, they will clean the progress flag */
		if (!p->mm) {
			/*
			 * this is incorrect for kthreads waiting still for
			 * their first wake_up.
			 */
			wake_up_process(p);
		}
		/* mark tasks wandering in userspace as already migrated */
		if (!kgr_needs_lazy_migration(p))
			kgr_task_safe(p);
	}
	read_unlock(&tasklist_lock);
}

static unsigned long kgr_get_fentry_loc(const char *f_name)
{
	unsigned long orig_addr, fentry_loc;
	const char *check_name;
	char check_buf[KSYM_SYMBOL_LEN];

	orig_addr = kallsyms_lookup_name(f_name);
	if (!orig_addr) {
		pr_err("kgr: function %s not resolved\n", f_name);
		return -ENOENT;
	}

	fentry_loc = ftrace_function_to_fentry(orig_addr);
	if (!fentry_loc) {
		pr_err("kgr: fentry_loc not properly resolved\n");
		return -ENXIO;
	}

	check_name = kallsyms_lookup(fentry_loc, NULL, NULL, NULL, check_buf);
	if (strcmp(check_name, f_name)) {
		pr_err("kgr: we got out of bounds the intended function (%s -> %s)\n",
				f_name, check_name);
		return -EINVAL;
	}

	return fentry_loc;
}

static void kgr_handle_irq_cpu(struct work_struct *work)
{
	unsigned long flags;

	local_irq_save(flags);
	*this_cpu_ptr(kgr_patch->irq_use_new) = true;
	local_irq_restore(flags);
}

static void kgr_handle_irqs(void)
{
	schedule_on_each_cpu(kgr_handle_irq_cpu);
}

static unsigned long kgr_get_old_fun(const struct kgr_patch_fun *patch_fun)
{
	const char *name = patch_fun->name;
	unsigned long last_new_fun = 0;
	struct kgr_patch_fun *pf;
	struct kgr_patch *p;

	list_for_each_entry(p, &patches, list) {
		kgr_for_each_patch(p, pf) {
			if (pf->state != KGR_PATCH_APPLIED)
				continue;

			if (!strcmp(pf->name, name))
				last_new_fun = (unsigned long)pf->new_fun;
		}
	}

	if (last_new_fun)
		return ftrace_function_to_fentry(last_new_fun);

	return kgr_get_fentry_loc(name);
}

static int kgr_init_ftrace_ops(struct kgr_patch_fun *patch_fun)
{
	struct ftrace_ops *fops;
	unsigned long fentry_loc;

	/*
	 * Initialize the ftrace_ops->private with pointers to the fentry
	 * sites of both old and new functions. This is used as a
	 * redirection target in the stubs.
	 */

	fentry_loc = ftrace_function_to_fentry(
			((unsigned long)patch_fun->new_fun));
	if (!fentry_loc) {
		pr_err("kgr: fentry_loc not properly resolved\n");
		return -ENXIO;
	}

	pr_debug("kgr: storing %lx to loc_new for %pf\n",
			fentry_loc, patch_fun->new_fun);
	patch_fun->loc_new = fentry_loc;

	fentry_loc = kgr_get_old_fun(patch_fun);
	if (IS_ERR_VALUE(fentry_loc))
		return fentry_loc;

	pr_debug("kgr: storing %lx to loc_old for %s\n",
			fentry_loc, patch_fun->name);
	patch_fun->loc_old = fentry_loc;

	fops = &patch_fun->ftrace_ops_fast;
	fops->private = patch_fun;
	fops->func = kgr_stub_fast;
	fops->flags = FTRACE_OPS_FL_SAVE_REGS;

	fops = &patch_fun->ftrace_ops_slow;
	fops->private = patch_fun;
	fops->func = kgr_stub_slow;
	fops->flags = FTRACE_OPS_FL_SAVE_REGS;

	return 0;
}

static int kgr_patch_code(struct kgr_patch_fun *patch_fun, bool final,
		bool revert)
{
	struct ftrace_ops *new_ops = NULL, *unreg_ops = NULL;
	enum kgr_patch_state next_state;
	int err;

	switch (patch_fun->state) {
	case KGR_PATCH_INIT:
		if (revert || final)
			return -EINVAL;
		err = kgr_init_ftrace_ops(patch_fun);
		if (err) {
			if (err == -ENOENT && !patch_fun->abort_if_missing) {
				patch_fun->state = KGR_PATCH_SKIPPED;
				return 0;
			}
			return err;
		}

		next_state = KGR_PATCH_SLOW;
		new_ops = &patch_fun->ftrace_ops_slow;
		break;
	case KGR_PATCH_SLOW:
		if (revert || !final)
			return -EINVAL;
		next_state = KGR_PATCH_APPLIED;
		new_ops = &patch_fun->ftrace_ops_fast;
		unreg_ops = &patch_fun->ftrace_ops_slow;
		break;
	case KGR_PATCH_APPLIED:
		if (!revert || final)
			return -EINVAL;
		next_state = KGR_PATCH_REVERT_SLOW;
		new_ops = &patch_fun->ftrace_ops_slow;
		unreg_ops = &patch_fun->ftrace_ops_fast;
		break;
	case KGR_PATCH_REVERT_SLOW:
		if (!revert || !final)
			return -EINVAL;
		next_state = KGR_PATCH_REVERTED;
		unreg_ops = &patch_fun->ftrace_ops_slow;
		break;
	case KGR_PATCH_SKIPPED:
		return 0;
	default:
		return -EINVAL;
	}

	if (new_ops) {
		/* Flip the switch */
		err = ftrace_set_filter_ip(new_ops, patch_fun->loc_old, 0, 0);
		if (err) {
			pr_err("kgr: setting filter for %lx (%s) failed\n",
					patch_fun->loc_old, patch_fun->name);
			return err;
		}

		err = register_ftrace_function(new_ops);
		if (err) {
			pr_err("kgr: registering ftrace function for %lx (%s) failed\n",
					patch_fun->loc_old, patch_fun->name);
			return err;
		}
	}

	/*
	 * Get rid of the slow stub. Having two stubs in the interim is fine,
	 * the last one always "wins", as it'll be dragged earlier from the
	 * ftrace hashtable
	 */
	if (unreg_ops) {
		err = unregister_ftrace_function(unreg_ops);
		if (err) {
			pr_warning("kgr: unregistering ftrace function for %s failed with %d\n",
					patch_fun->name, err);
			/* don't fail: we are only slower */
		}
	}

	patch_fun->state = next_state;

	pr_debug("kgr: redirection for %s done\n", patch_fun->name);

	return 0;
}

int kgr_modify_kernel(struct kgr_patch *patch, bool revert)
{
	struct kgr_patch_fun *patch_fun;
	int ret;

	if (!kgr_initialized) {
		pr_err("kgr: can't patch, not initialized\n");
		return -EINVAL;
	}

	patch->irq_use_new = alloc_percpu(bool);
	if (!patch->irq_use_new) {
		pr_err("kgr: can't patch, cannot allocate percpu data\n");
		return -ENOMEM;
	}

	mutex_lock(&kgr_in_progress_lock);
	if (patch->refs) {
		pr_err("kgr: can't patch, this patch is still referenced\n");
		ret = -EBUSY;
		goto err_unlock;
	}

	if (kgr_in_progress) {
		pr_err("kgr: can't patch, another patching not yet finalized\n");
		ret = -EAGAIN;
		goto err_unlock;
	}

	kgr_mark_processes();

	kgr_for_each_patch(patch, patch_fun) {
		patch_fun->patch = patch;

		ret = kgr_patch_code(patch_fun, false, revert);
		/*
		 * In case any of the symbol resolutions in the set
		 * has failed, patch all the previously replaced fentry
		 * callsites back to nops and fail with grace
		 */
		if (ret < 0) {
			for (patch_fun--; patch_fun >= patch->patches;
					patch_fun--) {
				if (patch_fun->state == KGR_PATCH_SLOW)
					unregister_ftrace_function(&patch_fun->ftrace_ops_slow);
			}
			goto err_unlock;
		}
	}
	kgr_in_progress = true;
	kgr_patch = patch;
	kgr_revert = revert;
	if (!revert)
		kgr_refs_inc();
	mutex_unlock(&kgr_in_progress_lock);

	kgr_handle_irqs();
	kgr_handle_processes();

	/*
	 * give everyone time to exit kernel, and check after a while
	 */
	queue_delayed_work(kgr_wq, &kgr_work, 10 * HZ);

	return 0;
err_unlock:
	mutex_unlock(&kgr_in_progress_lock);
	free_percpu(patch->irq_use_new);

	return ret;
}

/**
 * kgr_patch_kernel -- the entry for a kgraft patch
 * @patch: patch to be applied
 *
 * Start patching of code.
 */
int kgr_patch_kernel(struct kgr_patch *patch)
{
	int ret;

	if (!try_module_get(patch->owner)) {
		pr_err("kgr: can't increase patch module refcount\n");
		return -EBUSY;
	}

	init_completion(&patch->finish);

	ret = kgr_patch_dir_add(patch);
	if (ret)
		goto err_put;

	ret = kgr_modify_kernel(patch, false);
	if (ret)
		goto err_dir_del;

	return ret;
err_dir_del:
	kgr_patch_dir_del(patch);
err_put:
	module_put(patch->owner);

	return ret;
}
EXPORT_SYMBOL_GPL(kgr_patch_kernel);

/**
 * kgr_patch_remove -- module with this patch is leaving
 *
 * @patch: this patch is going away
 */
void kgr_patch_remove(struct kgr_patch *patch)
{
	kgr_patch_dir_del(patch);
}
EXPORT_SYMBOL_GPL(kgr_patch_remove);

static int __init kgr_init(void)
{
	int ret;

	if (ftrace_is_dead()) {
		pr_warn("kgr: enabled, but no fentry locations found ... aborting\n");
		return -ENODEV;
	}

	ret = kgr_add_files();
	if (ret)
		return ret;

	kgr_wq = create_singlethread_workqueue("kgraft");
	if (!kgr_wq) {
		pr_err("kgr: cannot allocate a work queue, aborting!\n");
		ret = -ENOMEM;
		goto err_remove_files;
	}

	kgr_initialized = true;
	pr_info("kgr: successfully initialized\n");

	return 0;
err_remove_files:
	kgr_remove_files();

	return ret;
}
module_init(kgr_init);
