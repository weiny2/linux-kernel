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
#include <linux/kallsyms.h>
#include <linux/kgraft.h>
#include <linux/module.h>
#include <linux/percpu.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/sort.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/workqueue.h>

static int kgr_patch_code(const struct kgr_patch *patch,
		struct kgr_patch_fun *patch_fun, bool final);
static void kgr_work_fn(struct work_struct *work);

static struct workqueue_struct *kgr_wq;
static DECLARE_DELAYED_WORK(kgr_work, kgr_work_fn);
static DEFINE_MUTEX(kgr_in_progress_lock);
static bool kgr_in_progress;
static bool kgr_initialized;
static const struct kgr_patch *kgr_patch;

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
	struct kgr_patch_fun *const *patch_fun;

	for (patch_fun = kgr_patch->patches; *patch_fun; patch_fun++) {
		int ret = kgr_patch_code(kgr_patch, *patch_fun, true);
		/*
		 * In case any of the symbol resolutions in the set
		 * has failed, patch all the previously replaced fentry
		 * callsites back to nops and fail with grace
		 */
		if (ret < 0)
			pr_err("kgr: finalize for %s failed, trying to continue\n",
					(*patch_fun)->name);
	}
	free_percpu(kgr_patch->irq_use_new);
}

static void kgr_work_fn(struct work_struct *work)
{
	if (kgr_still_patching()) {
		pr_info("kgr failed after timeout (%d), still in degraded mode\n",
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
	mutex_lock(&kgr_in_progress_lock);
	kgr_in_progress = false;
	mutex_unlock(&kgr_in_progress_lock);
}

static void kgr_handle_processes(void)
{
	struct task_struct *p;

	read_lock(&tasklist_lock);
	for_each_process(p) {
		kgr_mark_task_in_progress(p);

		/* wake up kthreads, they will clean the progress flag */
		if (!p->mm) {
			/*
			 * this is incorrect for kthreads waiting still for
			 * their first wake_up.
			 */
			wake_up_process(p);
		}
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

static int kgr_init_ftrace_ops(const struct kgr_patch_fun *patch_fun)
{
	struct kgr_loc_caches *caches;
	unsigned long fentry_loc;
	int ret;

	/*
	 * Initialize the ftrace_ops->private with pointers to the fentry
	 * sites of both old and new functions. This is used as a
	 * redirection target in the per-arch stubs.
	 *
	 * Beware! -- freeing (once unloading will be implemented)
	 * will require synchronize_sched() etc.
	 */

	caches = kmalloc(sizeof(*caches), GFP_KERNEL);
	if (!caches) {
		pr_debug("kgr: unable to allocate fentry caches\n");
		return -ENOMEM;
	}

	fentry_loc = kgr_get_fentry_loc(patch_fun->new_name);
	if (IS_ERR_VALUE(fentry_loc)) {
		pr_debug("kgr: fentry location lookup failed\n");
		ret = fentry_loc;
		goto free_caches;
	}
	pr_debug("kgr: storing %lx to caches->new for %s\n",
			fentry_loc, patch_fun->new_name);
	caches->new = fentry_loc;

	fentry_loc = kgr_get_fentry_loc(patch_fun->name);
	if (IS_ERR_VALUE(fentry_loc)) {
		pr_debug("kgr: fentry location lookup failed\n");
		ret = fentry_loc;
		goto free_caches;
	}

	pr_debug("kgr: storing %lx to caches->old for %s\n",
			fentry_loc, patch_fun->name);
	caches->old = fentry_loc;

	patch_fun->ftrace_ops_fast->private = caches;
	patch_fun->ftrace_ops_slow->private = caches;

	return 0;
free_caches:
	kfree(caches);
	return ret;
}

static int kgr_patch_code(const struct kgr_patch *patch,
		struct kgr_patch_fun *patch_fun, bool final)
{
	struct ftrace_ops *new_ops;
	struct kgr_loc_caches *caches;
	unsigned long fentry_loc;
	int err;

	/* Choose between slow and fast stub */
	if (!final) {
		err = kgr_init_ftrace_ops(patch_fun);
		if (err) {
			if (err == -ENOENT && !patch_fun->abort_if_missing)
				return 0;
			return err;
		}
		pr_debug("kgr: patching %s to slow stub\n", patch_fun->name);
		new_ops = patch_fun->ftrace_ops_slow;
	} else {
		if (!patch_fun->applied)
			return 0;
		pr_debug("kgr: patching %s to fast stub\n", patch_fun->name);
		new_ops = patch_fun->ftrace_ops_fast;
	}

	/* Flip the switch */
	caches = new_ops->private;
	caches->irq_use_new = patch->irq_use_new;
	fentry_loc = caches->old;
	err = ftrace_set_filter_ip(new_ops, fentry_loc, 0, 0);
	if (err) {
		pr_debug("kgr: setting filter for %lx (%s) failed\n",
				caches->old, patch_fun->name);
		return err;
	}

	err = register_ftrace_function(new_ops);
	if (err) {
		pr_debug("kgr: registering ftrace function for %lx (%s) failed\n",
				caches->old, patch_fun->name);
		return err;
	}

	/*
	 * Get rid of the slow stub. Having two stubs in the interim is fine,
	 * the last one always "wins", as it'll be dragged earlier from the
	 * ftrace hashtable
	 */
	if (final) {
		err = unregister_ftrace_function(patch_fun->ftrace_ops_slow);
		if (err) {
			pr_debug("kgr: unregistering ftrace function for %lx (%s) failed with %d\n",
					fentry_loc, patch_fun->name, err);
			/* don't fail: we are only slower */
			return 0;
		}
	} else
		patch_fun->applied = true;

	pr_debug("kgr: redirection for %lx (%s) done\n", fentry_loc,
			patch_fun->name);

	return 0;
}

/**
 * kgr_start_patching -- the entry for a kgraft patch
 * @patch: patch to be applied
 *
 * Start patching of code.
 */
int kgr_start_patching(struct kgr_patch *patch)
{
	struct kgr_patch_fun *const *patch_fun;
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
	if (kgr_in_progress) {
		pr_err("kgr: can't patch, another patching not yet finalized\n");
		ret = -EAGAIN;
		goto unlock_free;
	}

	for (patch_fun = patch->patches; *patch_fun; patch_fun++) {
		ret = kgr_patch_code(patch, *patch_fun, false);
		/*
		 * In case any of the symbol resolutions in the set
		 * has failed, patch all the previously replaced fentry
		 * callsites back to nops and fail with grace
		 */
		if (ret < 0) {
			for (patch_fun--; patch_fun >= patch->patches;
					patch_fun--) {
				if ((*patch_fun)->applied)
					unregister_ftrace_function(
						(*patch_fun)->ftrace_ops_slow);
			}
			goto unlock_free;
		}
	}
	kgr_in_progress = true;
	kgr_patch = patch;
	mutex_unlock(&kgr_in_progress_lock);

	kgr_handle_irqs();
	kgr_handle_processes();

	/*
	 * give everyone time to exit kernel, and check after a while
	 */
	queue_delayed_work(kgr_wq, &kgr_work, 5 * HZ);

	return 0;
unlock_free:
	mutex_unlock(&kgr_in_progress_lock);

	free_percpu(patch->irq_use_new);

	return ret;
}
EXPORT_SYMBOL_GPL(kgr_start_patching);

static int __init kgr_init(void)
{
	if (ftrace_is_dead()) {
		pr_warn("kgr: enabled, but no fentry locations found ... aborting\n");
		return -ENODEV;
	}

	kgr_wq = create_singlethread_workqueue("kgraft");
	if (!kgr_wq) {
		pr_err("kgr: cannot allocate a work queue, aborting!\n");
		return -ENOMEM;
	}

	kgr_initialized = true;
	pr_info("kgr: successfully initialized\n");

	return 0;
}
module_init(kgr_init);
