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

#include <linux/bitmap.h>
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
static LIST_HEAD(kgr_patches);
static LIST_HEAD(kgr_to_revert);
bool kgr_in_progress;
static bool kgr_initialized;
static struct kgr_patch *kgr_patch;
static bool kgr_revert;
/*
 * Setting the per-process flag and stub instantiation has to be performed
 * "atomically", otherwise the flag might get cleared and old function called
 * during the race window.
 *
 * kgr_immutable is an atomic flag which signals whether we are in the
 * actual race window and lets the stub take a proper action (reset the
 * 'in progress' state)
 */
static DECLARE_BITMAP(kgr_immutable, 1);

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
	else if (test_bit(0, kgr_immutable)) {
		kgr_mark_task_in_progress(current);
		go_old = true;
	} else {
		rmb(); /* test_bit before kgr_mark_task_in_progress */
		go_old = kgr_task_in_progress(current);
	}

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

	list_for_each_entry(p, &kgr_patches, list)
		p->refs++;
}

static void kgr_refs_dec(void)
{
	struct kgr_patch *p;

	list_for_each_entry(p, &kgr_patches, list)
		p->refs--;
}

/* decrease reference for all older patches */
static void kgr_refs_dec_limited(struct kgr_patch *limit)
{
	struct kgr_patch *p;

	list_for_each_entry(p, &kgr_patches, list) {
		if (p == limit)
			return;
		p->refs--;
	}
}

static int kgr_ftrace_enable(struct kgr_patch_fun *pf, struct ftrace_ops *fops)
{
	int ret;

	ret = ftrace_set_filter_ip(fops, pf->loc_name, 0, 0);
	if (ret)
		return ret;

	ret = register_ftrace_function(fops);
	if (ret)
		ftrace_set_filter_ip(fops, pf->loc_name, 1, 0);

	return ret;
}

static int kgr_ftrace_disable(struct kgr_patch_fun *pf, struct ftrace_ops *fops)
{
	int ret;

	ret = unregister_ftrace_function(fops);
	if (ret)
		return ret;

	ret = ftrace_set_filter_ip(fops, pf->loc_name, 1, 0);
	if (ret)
		register_ftrace_function(fops);

	return ret;
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

static bool kgr_patch_contains(const struct kgr_patch *p, const char *name)
{
	const struct kgr_patch_fun *pf;

	kgr_for_each_patch_fun(p, pf)
		if (!strcmp(pf->name, name))
			return true;

	return false;
}

/*
 * The patch is not longer used and can be removed immediately.
 * This function does the same as kgr_finalize() for reverted patches.
 * It just does not need to update the state of functions.
 */
static void kgr_remove_patch_fast(struct kgr_patch *patch)
{
	kgr_refs_dec_limited(patch);
	list_del(&patch->list);
	module_put(patch->owner);
}

/*
 * All patches from kgr_patches are obsoleted and will get replaced
 * by kgr_patch.
 */
static void kgr_replace_all(void)
{
	struct kgr_patch_fun *pf;
	struct kgr_patch *p, *tmp;

	list_for_each_entry_safe(p, tmp, &kgr_patches, list) {
		bool needs_revert = false;

		kgr_for_each_patch_fun(p, pf) {
			if (pf->state != KGR_PATCH_APPLIED)
				continue;

			if (!kgr_patch_contains(kgr_patch, pf->name)) {
				needs_revert = true;
				continue;
			}

			/* the fast ftrace fops were disabled during patching */
			pf->state = KGR_PATCH_REVERTED;
		}

		if (needs_revert)
			list_move(&p->list, &kgr_to_revert);
		else
			kgr_remove_patch_fast(p);
	}
}

static struct kgr_patch *__kgr_finalize(void)
{
	struct kgr_patch_fun *patch_fun;
	struct kgr_patch *p_to_revert = NULL;

	kgr_for_each_patch_fun(kgr_patch, patch_fun) {
		int ret = kgr_patch_code(patch_fun, true, kgr_revert);

		if (ret < 0)
			pr_err("kgr: finalize for %s failed, trying to continue\n",
					patch_fun->name);
	}

	free_percpu(kgr_patch->irq_use_new);

	if (kgr_revert) {
		kgr_refs_dec();
		module_put(kgr_patch->owner);
	} else {
		if (kgr_patch->replace_all)
			kgr_replace_all();
		list_add_tail(&kgr_patch->list, &kgr_patches);
	}

	kgr_patch = NULL;
	if (list_empty(&kgr_to_revert)) {
		kgr_in_progress = false;
	} else {
		/*
		 * kgr_in_progress is not cleared to avoid races after the
		 * unlock in kgr_finalize() below. The force flag is set
		 * instead.
		 */
		p_to_revert = list_first_entry(&kgr_to_revert, struct kgr_patch,
				list);
	}

	return p_to_revert;
}

static void kgr_finalize(void)
{
	struct kgr_patch *p_to_revert;

	pr_info("kgr succeeded\n");

	mutex_lock(&kgr_in_progress_lock);

	p_to_revert = __kgr_finalize();

	mutex_unlock(&kgr_in_progress_lock);

	if (p_to_revert) {
		int ret = kgr_modify_kernel(p_to_revert, true, true);
		if (ret)
			pr_err("kgr: continual revert of %s failedwith %d, but continuing\n",
					p_to_revert->name, ret);
	}
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
	kgr_finalize();
}

int kgr_unmark_processes(void)
{
	struct task_struct *p;
	struct kgr_patch *p_to_revert;

	mutex_lock(&kgr_in_progress_lock);

	if (!kgr_in_progress) {
		pr_info("kgr: no patching is in progress\n");
		mutex_unlock(&kgr_in_progress_lock);
		return -1;
	}

	read_lock(&tasklist_lock);

	for_each_process(p)
		kgr_task_safe(p);

	read_unlock(&tasklist_lock);

	p_to_revert = __kgr_finalize();
	mutex_unlock(&kgr_in_progress_lock);

	if (p_to_revert) {
		int ret = kgr_modify_kernel(p_to_revert, true, true);
		if (ret)
			pr_err("kgr: continual revert of %s failedwith %d, but continuing\n",
					p_to_revert->name, ret);
	}

	/*
	 * Patching was forced to finish, everything is back in shape via
	 * kgr_finalize. Last thing is to cancel the workqueue, otherwise
	 * kgr_finalize would be called again.
	 */
	cancel_delayed_work_sync(&kgr_work);

	return 0;
}

static void kgr_handle_processes(void)
{
	struct task_struct *p;

	read_lock(&tasklist_lock);
	for_each_process(p) {
		/* skip tasks wandering in userspace as already migrated */
		if (kgr_needs_lazy_migration(p))
			kgr_mark_task_in_progress(p);
		/* wake up kthreads, they will clean the progress flag */
		if (p->flags & PF_KTHREAD) {
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

static struct kgr_patch_fun *
kgr_get_last_pf(const struct kgr_patch_fun *patch_fun)
{
	const char *name = patch_fun->name;
	struct kgr_patch_fun *pf, *last_pf = NULL;
	struct kgr_patch *p;

	list_for_each_entry(p, &kgr_patches, list) {
		kgr_for_each_patch_fun(p, pf) {
			if (pf->state != KGR_PATCH_APPLIED)
				continue;

			if (!strcmp(pf->name, name))
				last_pf = pf;
		}
	}

	return last_pf;
}

static unsigned long kgr_get_old_fun(const struct kgr_patch_fun *patch_fun)
{
	struct kgr_patch_fun *pf = kgr_get_last_pf(patch_fun);

	if (pf)
		return ftrace_function_to_fentry((unsigned long)pf->new_fun);

	return patch_fun->loc_name;
}

/*
 * Obtain the "previous" (in the sense of patch stacking) value of ftrace_ops
 * so that it can be put back properly in case of reverting the patch
 */
static struct ftrace_ops *
kgr_get_old_fops(const struct kgr_patch_fun *patch_fun)
{
	struct kgr_patch_fun *pf = kgr_get_last_pf(patch_fun);

	return pf ? &pf->ftrace_ops_fast : NULL;
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

	fentry_loc = kgr_get_fentry_loc(patch_fun->name);
	if (IS_ERR_VALUE(fentry_loc))
		return fentry_loc;

	pr_debug("kgr: storing %lx to loc_name for %s\n",
			fentry_loc, patch_fun->name);
	patch_fun->loc_name = fentry_loc;

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
		/*
		 * If some previous patch already patched a function, the old
		 * fops need to be disabled, otherwise the new redirection will
		 * never be used.
		 */
		unreg_ops = kgr_get_old_fops(patch_fun);
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
		/*
		 * Put back in place the old fops that were deregistered in
		 * case of stacked patching (see the comment above).
		 */
		new_ops = kgr_get_old_fops(patch_fun);
		break;
	case KGR_PATCH_REVERTED:
		if (!revert || final)
			return -EINVAL;
		return 0;
	case KGR_PATCH_SKIPPED:
		return 0;
	case KGR_PATCH_APPLIED_NON_FINALIZED:
		/* like KGR_PATCH_SLOW but ops changes are already in place */
		if (revert || !final)
			return -EINVAL;
		next_state = KGR_PATCH_APPLIED;
		break;
	default:
		return -EINVAL;
	}

	if (new_ops) {
		/* Flip the switch */
		err = kgr_ftrace_enable(patch_fun, new_ops);
		if (err) {
			pr_err("kgr: cannot enable ftrace function for %lx (%s)\n",
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
		err = kgr_ftrace_disable(patch_fun, unreg_ops);
		if (err) {
			pr_warning("kgr: disabling ftrace function for %s failed with %d\n",
					patch_fun->name, err);
			/* don't fail: we are only slower */
		}
	}

	patch_fun->state = next_state;

	pr_debug("kgr: redirection for %s done\n", patch_fun->name);

	return 0;
}

/**
 * kgr_modify_kernel -- apply or revert a patch
 * @patch: patch to deal with
 * @revert: if @patch should be reverted, set to true
 * @force: if kgr_in_progress should be ignored, set to true (internal use)
 */
int kgr_modify_kernel(struct kgr_patch *patch, bool revert, bool force)
{
	struct kgr_patch_fun *patch_fun;
	int ret;

	if (!kgr_initialized) {
		pr_err("kgr: can't patch, not initialized\n");
		return -EINVAL;
	}

	mutex_lock(&kgr_in_progress_lock);
	if (patch->refs) {
		pr_err("kgr: can't patch, this patch is still referenced\n");
		ret = -EBUSY;
		goto err_unlock;
	}

	if (!force && (kgr_in_progress || !list_empty(&kgr_to_revert))) {
		pr_err("kgr: can't patch, another patching not yet finalized\n");
		ret = -EAGAIN;
		goto err_unlock;
	}

	if (revert && list_empty(&patch->list)) {
		pr_err("kgr: can't patch, this one was already reverted\n");
		ret = -EINVAL;
		goto err_unlock;
	}

	patch->irq_use_new = alloc_percpu(bool);
	if (!patch->irq_use_new) {
		pr_err("kgr: can't patch, cannot allocate percpu data\n");
		ret = -ENOMEM;
		goto err_unlock;
	}

	/*
	 * If the patch has immediate flag set, avoid the lazy-switching
	 * between universes completely.
	 */
	if (!patch->immediate) {
		set_bit(0, kgr_immutable);
		wmb(); /* set_bit before kgr_handle_processes */
	}

	kgr_for_each_patch_fun(patch, patch_fun) {
		patch_fun->patch = patch;

		ret = kgr_patch_code(patch_fun, false, revert);
		/*
		 * In case any of the symbol resolutions in the set
		 * has failed, patch all the previously replaced fentry
		 * callsites back to nops and fail with grace
		 */
		if (ret < 0) {
			for (patch_fun--; patch_fun >= patch->patches;
					patch_fun--)
				if (patch_fun->state == KGR_PATCH_SLOW)
					kgr_ftrace_disable(patch_fun,
						&patch_fun->ftrace_ops_slow);
			goto err_free;
		}
	}
	kgr_in_progress = true;
	kgr_patch = patch;
	kgr_revert = revert;
	if (revert)
		list_del_init(&patch->list); /* init for list_empty() above */
	else if (!patch->replace_all)
		/* block all older patches if they are not replaced */
		kgr_refs_inc();
	mutex_unlock(&kgr_in_progress_lock);

	kgr_handle_irqs();

	if (patch->immediate) {
		kgr_finalize();
	} else {
		kgr_handle_processes();
		wmb(); /* clear_bit after kgr_handle_processes */
		clear_bit(0, kgr_immutable);
		/*
		 * give everyone time to exit kernel, and check after a while
		 */
		queue_delayed_work(kgr_wq, &kgr_work, 10 * HZ);
	}

	return 0;
err_free:
	free_percpu(patch->irq_use_new);
err_unlock:
	mutex_unlock(&kgr_in_progress_lock);

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

	ret = kgr_modify_kernel(patch, false, false);
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

#ifdef CONFIG_MODULES

/*
 * This function is called when new module is loaded but before it is used.
 * Therefore it could set the fast path directly.
 */
static void kgr_handle_patch_for_loaded_module(struct kgr_patch *patch,
					       const struct module *mod)
{
	struct ftrace_ops *unreg_ops;
	struct kgr_patch_fun *patch_fun;
	unsigned long addr;
	int err;

	kgr_for_each_patch_fun(patch, patch_fun) {
		if (patch_fun->state != KGR_PATCH_SKIPPED)
			continue;

		addr =  kallsyms_lookup_name(patch_fun->name);
		if (!within_module(addr, mod))
			continue;

		err = kgr_init_ftrace_ops(patch_fun);
		if (err)
			continue;

		unreg_ops = kgr_get_old_fops(patch_fun);

		err = kgr_ftrace_enable(patch_fun, &patch_fun->ftrace_ops_fast);
		if (err) {
			pr_err("kgr: cannot enable ftrace function for the originally skipped %lx (%s)\n",
			       patch_fun->loc_old, patch_fun->name);
			continue;
		}

		if (unreg_ops) {
			err = kgr_ftrace_disable(patch_fun, unreg_ops);
			if (err) {
				pr_warning("kgr: disabling ftrace function for %s failed with %d\n",
					   patch_fun->name, err);
			}
		}

		if (patch == kgr_patch)
			patch_fun->state = KGR_PATCH_APPLIED_NON_FINALIZED;
		else
			patch_fun->state = KGR_PATCH_APPLIED;

		pr_debug("kgr: fast redirection for %s done\n", patch_fun->name);
	}
}

/**
 * kgr_module_init -- apply skipped patches for newly loaded modules
 *
 * It must be called when symbols are visible to kallsyms but before the module
 * init is called. Otherwise, it would not be able to use the fast stub.
 */
void kgr_module_init(const struct module *mod)
{
	struct kgr_patch *p;

	/* early modules will be patched once KGraft is initialized */
	if (!kgr_initialized)
		return;

	mutex_lock(&kgr_in_progress_lock);

	/*
	 * Check already applied patches for skipped functions. If there are
	 * more patches we want to set them all. They need to be in place when
	 * we remove some patch.
	 */
	list_for_each_entry(p, &kgr_patches, list)
		kgr_handle_patch_for_loaded_module(p, mod);

	/* also check the patch in progress that is being applied */
	if (kgr_patch && !kgr_revert)
		kgr_handle_patch_for_loaded_module(kgr_patch, mod);

	mutex_unlock(&kgr_in_progress_lock);
}

/*
 * Disable the patch immediately. It does not matter in which state it is.
 *
 * This function is used when a module is being removed and the code is
 * no longer called.
 */
static int kgr_forced_code_patch_removal(struct kgr_patch_fun *patch_fun)
{
	struct ftrace_ops *ops;
	int err;

	switch (patch_fun->state) {
	case KGR_PATCH_INIT:
	case KGR_PATCH_SKIPPED:
		return 0;
	case KGR_PATCH_SLOW:
	case KGR_PATCH_REVERT_SLOW:
		ops = &patch_fun->ftrace_ops_slow;
		break;
	case KGR_PATCH_APPLIED:
	case KGR_PATCH_APPLIED_NON_FINALIZED:
		ops = &patch_fun->ftrace_ops_fast;
		break;
	default:
		return -EINVAL;
	}

	err = kgr_ftrace_disable(patch_fun, ops);
	if (err) {
		pr_warn("kgr: forced disabling of ftrace function for %s failed with %d\n",
			patch_fun->name, err);
		return err;
	}

	patch_fun->state = KGR_PATCH_SKIPPED;
	pr_debug("kgr: forced disabling for %s done\n", patch_fun->name);
	return 0;
}

/*
 * Check the given patch and disable pieces related to the module
 * that is being removed.
 */
static void kgr_handle_patch_for_going_module(struct kgr_patch *patch,
					     const struct module *mod)
{
	struct kgr_patch_fun *patch_fun;
	unsigned long addr;

	kgr_for_each_patch_fun(patch, patch_fun) {
		addr = kallsyms_lookup_name(patch_fun->name);
		if (!within_module(addr, mod))
			continue;
		/*
		 * FIXME: It should schedule the patch removal or block
		 *	  the module removal or taint kernel or so.
		 */
		if (patch_fun->abort_if_missing) {
			pr_err("kgr: removing function %s that is required for the patch %s\n",
			       patch_fun->name, patch->name);
		}

		kgr_forced_code_patch_removal(patch_fun);
	}
}

/*
 * Disable patches for the module that is being removed.
 *
 * FIXME: The module removal cannot be stopped at this stage. All affected
 * patches have to be removed. Therefore, the operation continues even in
 * case of errors.
 */
static void kgr_handle_going_module(const struct module *mod)
{
	struct kgr_patch *p;

	/* Nope when kGraft has not been initialized yet */
	if (!kgr_initialized)
		return;

	mutex_lock(&kgr_in_progress_lock);

	list_for_each_entry(p, &kgr_patches, list)
		kgr_handle_patch_for_going_module(p, mod);

	/* also check the patch in progress for removed functions */
	if (kgr_patch)
		kgr_handle_patch_for_going_module(kgr_patch, mod);

	mutex_unlock(&kgr_in_progress_lock);
}

static int kgr_module_notify_exit(struct notifier_block *self,
				  unsigned long val, void *data)
{
	const struct module *mod = data;

	if (val == MODULE_STATE_GOING)
		kgr_handle_going_module(mod);

	return 0;
}

#else

static int kgr_module_notify_exit(struct notifier_block *self,
		unsigned long val, void *data)
{
	return 0;
}

#endif /* CONFIG_MODULES */

static struct notifier_block kgr_module_exit_nb = {
	.notifier_call = kgr_module_notify_exit,
	.priority = 0,
};

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

	ret = register_module_notifier(&kgr_module_exit_nb);
	if (ret)
		pr_warn("Failed to register kGraft module exit notifier\n");

	kgr_initialized = true;
	pr_info("kgr: successfully initialized\n");

	return 0;
err_remove_files:
	kgr_remove_files();

	return ret;
}
module_init(kgr_init);
