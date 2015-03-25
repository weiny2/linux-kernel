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
#include <linux/vermagic.h>
#include <linux/workqueue.h>

static int kgr_patch_code(struct kgr_patch_fun *patch_fun, bool final,
		bool revert, bool replace_revert);
static void kgr_work_fn(struct work_struct *work);

static struct workqueue_struct *kgr_wq;
static DECLARE_DELAYED_WORK(kgr_work, kgr_work_fn);
static DEFINE_MUTEX(kgr_in_progress_lock);
static LIST_HEAD(kgr_patches);
static bool __percpu *kgr_irq_use_new;
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
 *
 * Stubs have to be labeled with notrace to prevent recursion loop in ftrace.
 */
static notrace void kgr_stub_fast(unsigned long ip, unsigned long parent_ip,
		struct ftrace_ops *ops, struct pt_regs *regs)
{
	struct kgr_patch_fun *p = ops->private;

	kgr_set_regs_ip(regs, p->loc_new);
}

static notrace void kgr_stub_slow(unsigned long ip, unsigned long parent_ip,
		struct ftrace_ops *ops, struct pt_regs *regs)
{
	struct kgr_patch_fun *p = ops->private;
	bool go_old;

	if (in_interrupt())
		go_old = !*this_cpu_ptr(kgr_irq_use_new);
	else if (test_bit(0, kgr_immutable)) {
		kgr_mark_task_in_progress(current);
		go_old = true;
	} else {
		rmb(); /* test_bit before kgr_mark_task_in_progress */
		go_old = kgr_task_in_progress(current);
	}

	if (p->state == KGR_PATCH_REVERT_SLOW)
		go_old = !go_old;

	if (go_old)
		kgr_set_regs_ip(regs, p->loc_old + MCOUNT_INSN_SIZE);
	else
		kgr_set_regs_ip(regs, p->loc_new);
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

/*
 * The patches are no longer used and can be removed immediately, because
 * replace_all patch is finalized (and not yet in the kgr_patches list). We do
 * not have to deal with refs for older patches, since all of them are to be
 * removed.
 */
static void kgr_remove_patches_fast(void)
{
	struct kgr_patch *p, *tmp;

	list_for_each_entry_safe(p, tmp, &kgr_patches, list) {
		list_del(&p->list);
		module_put(p->owner);
	}
}

/*
 * In case of replace_all patch we need to finalize also reverted functions in
 * all previous patches. All previous patches contain only functions either in
 * APPLIED state (not reverted and no longer used) or in REVERT_SLOW state. We
 * mark the former as REVERTED and finalize the latter. Afterwards the patches
 * can be safely removed from the patches list (by calling
 * kgr_remove_patches_fast as in kgr_finalize).
 */
static void kgr_finalize_replaced_funs(void)
{
	struct kgr_patch_fun *pf;
	struct kgr_patch *p;
	int ret;

	list_for_each_entry(p, &kgr_patches, list)
		kgr_for_each_patch_fun(p, pf) {
			/*
			 * Function was not reverted, but is no longer used.
			 * Mark it as reverted so the user would not be confused
			 * by sysfs reporting of states.
			 */
			if (pf->state == KGR_PATCH_APPLIED) {
				pf->state = KGR_PATCH_REVERTED;
				continue;
			}

			ret = kgr_patch_code(pf, true, true, true);
			if (ret < 0)
				pr_err("kgr: finalize for %s failed, trying to continue\n",
				      pf->name);
		}
}

static void kgr_finalize(void)
{
	struct kgr_patch_fun *patch_fun;

	pr_info("kgr succeeded\n");

	mutex_lock(&kgr_in_progress_lock);

	kgr_for_each_patch_fun(kgr_patch, patch_fun) {
		int ret = kgr_patch_code(patch_fun, true, kgr_revert, false);

		if (ret < 0)
			pr_err("kgr: finalize for %s failed, trying to continue\n",
					patch_fun->name);

		/*
		 * When applying the replace_all patch all older patches are
		 * removed. We need to update loc_old and point it to the
		 * original function for the patch_funs from replace_all patch.
		 * The change is safe because the fast stub is used now. The
		 * correct value might be needed later when the patch is
		 * reverted.
		 */
		if (kgr_patch->replace_all && !kgr_revert)
			patch_fun->loc_old = patch_fun->loc_name;
	}

	if (kgr_patch->replace_all && !kgr_revert) {
		kgr_finalize_replaced_funs();
		kgr_remove_patches_fast();
	}

	free_percpu(kgr_irq_use_new);

	if (kgr_revert) {
		kgr_refs_dec();
		module_put(kgr_patch->owner);
	} else {
		list_add_tail(&kgr_patch->list, &kgr_patches);
	}

	kgr_patch = NULL;
	kgr_in_progress = false;

	mutex_unlock(&kgr_in_progress_lock);
}

static void kgr_work_fn(struct work_struct *work)
{
	static bool printed = false;

	if (kgr_still_patching()) {
		if (!printed) {
			pr_info("kgr still in progress after timeout, will keep"
					" trying every %d seconds\n",
				KGR_TIMEOUT);
			printed = true;
		}
		/* recheck again later */
		queue_delayed_work(kgr_wq, &kgr_work, KGR_TIMEOUT * HZ);
		return;
	}

	/*
	 * victory, patching finished, put everything back in shape
	 * with as less performance impact as possible again
	 */
	kgr_finalize();
	printed = false;
}

void kgr_unmark_processes(void)
{
	struct task_struct *p;

	read_lock(&tasklist_lock);
	for_each_process(p)
		kgr_task_safe(p);
	read_unlock(&tasklist_lock);
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
	*this_cpu_ptr(kgr_irq_use_new) = true;
	local_irq_restore(flags);
}

static void kgr_handle_irqs(void)
{
	schedule_on_each_cpu(kgr_handle_irq_cpu);
}

/*
 * There might be different variants of a function in different patches.
 * The patches are stacked in the order in which they are added. The variant
 * of a function from a newer patch takes precedence over the older variants
 * and makes the older variants unused.
 *
 * There might be an interim time when two variants of the same function
 * might be used by the system. Therefore we split the patches into two
 * categories.
 *
 * One patch might be in progress. It is either being added or being reverted.
 * In each case, there might be threads that are using the code from this patch
 * and also threads that are using the old code. Where the old code is the
 * original code or the code from the previous patch if any. This patch
 * might be found in the variable kgr_patch.
 *
 * The other patches are finalized. It means that the whole system started
 * using them at some point. Note that some parts of the patches might be
 * unused when there appeared new variants in newer patches. Also some threads
 * might already started using the patch in progress. Anyway, the finalized
 * patches might be found in the list kgr_patches.
 *
 * When manipulating the patches, we need to search and check the right variant
 * of a function on the stack. The following types are used to define
 * the requested variant.
 */
enum kgr_find_type {
	/*
	 * Find previous function variant in respect to stacking. Take
	 * into account even the patch in progress that is considered to be
	 * on top of the stack.
	 */
	KGR_PREVIOUS,
	/* Find the last finalized variant of the function on the stack. */
	KGR_LAST_FINALIZED,
	/*
	 * Find the last variant of the function on the stack. Take into
	 * account even the patch in progress.
	 */
	KGR_LAST_EXISTING,
	/* Find the variant of the function _only_ in the patch in progress. */
	KGR_IN_PROGRESS,
	/*
	 * This is the first unused find type. It can be used to check for
	 * invalid value.
	 */
	KGR_LAST_TYPE
};

/*
 * This function takes information about the patched function from the given
 * struct kgr_patch_fun and tries to find the requested variant of the
 * function. It returns NULL when the requested variant cannot be found.
 */
static struct kgr_patch_fun *
kgr_get_patch_fun(const struct kgr_patch_fun *patch_fun,
		  enum kgr_find_type type)
{
	const char *name = patch_fun->name;
	struct kgr_patch_fun *pf, *found_pf = NULL;
	struct kgr_patch *p;

	if (type < 0 || type >= KGR_LAST_TYPE) {
		pr_warn("kgr_get_patch_fun: invalid find type: %d\n", type);
		return NULL;
	}

	if (kgr_patch && (type == KGR_IN_PROGRESS || type == KGR_LAST_EXISTING))
		kgr_for_each_patch_fun(kgr_patch, pf)
			if (!strcmp(pf->name, name))
				return pf;

	if (type == KGR_IN_PROGRESS)
		goto out;

	list_for_each_entry(p, &kgr_patches, list) {
		kgr_for_each_patch_fun(p, pf) {
			if (type == KGR_PREVIOUS && pf == patch_fun)
				goto out;

			if (!strcmp(pf->name, name))
				found_pf = pf;
		}
	}
out:
	return found_pf;
}

/*
 * Check if the given struct patch_fun is the given type.
 * Note that it does not make sense for KGR_PREVIOUS.
 */
static bool kgr_is_patch_fun(const struct kgr_patch_fun *patch_fun,
		 enum kgr_find_type type)
{
	struct kgr_patch_fun *found_pf;

	if (type == KGR_IN_PROGRESS)
		return patch_fun->patch == kgr_patch;

	found_pf = kgr_get_patch_fun(patch_fun, type);
	return patch_fun == found_pf;
}

static unsigned long kgr_get_old_fun(const struct kgr_patch_fun *patch_fun)
{
	struct kgr_patch_fun *pf = kgr_get_patch_fun(patch_fun, KGR_PREVIOUS);

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
	struct kgr_patch_fun *pf = kgr_get_patch_fun(patch_fun, KGR_PREVIOUS);

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
		bool revert, bool replace_revert)
{
	struct ftrace_ops *new_ops = NULL, *unreg_ops = NULL;
	enum kgr_patch_state next_state;
	int err;

	switch (patch_fun->state) {
	case KGR_PATCH_INIT:
		if (revert || final || replace_revert)
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
		if (revert || !final || replace_revert)
			return -EINVAL;
		next_state = KGR_PATCH_APPLIED;
		new_ops = &patch_fun->ftrace_ops_fast;
		unreg_ops = &patch_fun->ftrace_ops_slow;
		break;
	case KGR_PATCH_APPLIED:
		if (!revert || final)
			return -EINVAL;
		next_state = KGR_PATCH_REVERT_SLOW;
		/*
		 * Update ftrace ops only when used. It is always needed for
		 * normal revert and in case of replace_all patch for the last
		 * patch_fun stacked (which has been as such called till now).
		 */
		if (!replace_revert ||
		    kgr_is_patch_fun(patch_fun, KGR_LAST_FINALIZED)) {
			new_ops = &patch_fun->ftrace_ops_slow;
			unreg_ops = &patch_fun->ftrace_ops_fast;
		}
		break;
	case KGR_PATCH_REVERT_SLOW:
		if (!revert || !final)
			return -EINVAL;
		next_state = KGR_PATCH_REVERTED;
		/*
		 * Update ftrace only when used. Normal revert removes the slow
		 * ops and enables fast ops from the fallback patch if any. In
		 * case of replace_all patch and reverting old patch_funs we
		 * just need to remove the slow stub and only for the last old
		 * patch_fun. The original code will be used.
		 */
		if (!replace_revert) {
			unreg_ops = &patch_fun->ftrace_ops_slow;
			new_ops = kgr_get_old_fops(patch_fun);
		} else if (kgr_is_patch_fun(patch_fun, KGR_LAST_FINALIZED)) {
			unreg_ops = &patch_fun->ftrace_ops_slow;
		}
		break;
	case KGR_PATCH_REVERTED:
		if (!revert || final || replace_revert)
			return -EINVAL;
		return 0;
	case KGR_PATCH_SKIPPED:
		return 0;
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

static bool kgr_patch_contains(const struct kgr_patch *p, const char *name)
{
	const struct kgr_patch_fun *pf;

	kgr_for_each_patch_fun(p, pf)
		if (!strcmp(pf->name, name))
			return true;

	return false;
}

/*
 * When replace_all patch is processed, all patches from kgr_patches are
 * obsolete and will get replaced. All functions from the patches which are not
 * patched in replace_all patch have to be reverted.
 */
static int kgr_revert_replaced_funs(struct kgr_patch *patch)
{
	struct kgr_patch *p;
	struct kgr_patch_fun *pf;
	int ret;

	list_for_each_entry(p, &kgr_patches, list)
		kgr_for_each_patch_fun(p, pf)
			if (!kgr_patch_contains(patch, pf->name)) {
				/*
				 * Calls from new universe to all functions
				 * being reverted are redirected to loc_old in
				 * the slow stub. We need to call the original
				 * functions and not the previous ones in terms
				 * of stacking, so loc_old is changed to
				 * loc_name.  Fast stub is still used, so change
				 * of loc_old is safe.
				 */
				pf->loc_old = pf->loc_name;

				ret = kgr_patch_code(pf, false, true, true);
				if (ret < 0) {
					/*
					 * No need to fail with grace as in
					 * kgr_modify_kernel
					 */
					pr_err("kgr: cannot revert function %s in patch %s\n",
					      pf->name, p->name);
					return ret;
				}
			}

	return 0;
}

/**
 * kgr_modify_kernel -- apply or revert a patch
 * @patch: patch to deal with
 * @revert: if @patch should be reverted, set to true
 */
int kgr_modify_kernel(struct kgr_patch *patch, bool revert)
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

	if (kgr_in_progress) {
		pr_err("kgr: can't patch, another patching not yet finalized\n");
		ret = -EAGAIN;
		goto err_unlock;
	}

	if (revert && list_empty(&patch->list)) {
		pr_err("kgr: can't patch, this one was already reverted\n");
		ret = -EINVAL;
		goto err_unlock;
	}

	kgr_irq_use_new = alloc_percpu(bool);
	if (!kgr_irq_use_new) {
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

	/*
	 * We need to revert patches of functions not patched in replace_all
	 * patch. Do that only while applying the replace_all patch.
	 */
	if (patch->replace_all && !revert) {
		ret = kgr_revert_replaced_funs(patch);
		if (ret)
			goto err_free;
	}

	kgr_for_each_patch_fun(patch, patch_fun) {
		patch_fun->patch = patch;

		ret = kgr_patch_code(patch_fun, false, revert, false);
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
		queue_delayed_work(kgr_wq, &kgr_work, KGR_TIMEOUT * HZ);
	}

	return 0;
err_free:
	free_percpu(kgr_irq_use_new);
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

	if (patch->vermagic && strcmp(patch->vermagic, VERMAGIC_STRING) != 0) {
		pr_err("kgr: kernel version mismatch: \"%s\" != \"%s\"\n",
				patch->vermagic, VERMAGIC_STRING);
		return -EINVAL;
	}
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

#ifdef CONFIG_MODULES

/*
 * Put the patch into the same state that the related patch is in.
 * It means that we need to use fast stub if the patch is finalized
 * and slow stub when the patch is in progress. Also we need to
 * register the ftrace stub only for the last patch on the stack
 * for the given function.
 *
 * Note the patched function from this module might call another
 * patched function from the kernel core. Some thread might still
 * use an old variant for the core functions. This is why we need to
 * use the slow stub when the patch is in progress. Both the core
 * kernel and module functions must be from the same universe.
 *
 * The situation differs a bit when the replace_all patch is being
 * applied. The patch_funs present in the patch are pushed forward in
 * the same way as for normal patches. So the slow stub is registered.
 * The patch_funs not present in the replace_all patch have to be
 * reverted. The slow stub is thus registered as well and next state
 * set to KGR_PATCH_REVERT_SLOW. We can do it only for the last
 * patch_funs in terms of stacking (KGR_LAST_FINALIZED), because the
 * other would not be used at all and all older patches are going to
 * be removed during finalization. Ftrace stub registration has to be
 * done only for this last patch_fun.
 */
static int kgr_patch_code_delayed(struct kgr_patch_fun *patch_fun)
{
	struct ftrace_ops *new_ops = NULL;
	enum kgr_patch_state next_state;
	int err;

	if (kgr_is_patch_fun(patch_fun, KGR_IN_PROGRESS)) {
		if (kgr_revert)
			next_state = KGR_PATCH_REVERT_SLOW;
		else
			next_state = KGR_PATCH_SLOW;
		/* this must be the last existing patch on the stack */
		new_ops = &patch_fun->ftrace_ops_slow;
	} else {
		if (kgr_patch && kgr_patch->replace_all && !kgr_revert &&
		    !kgr_patch_contains(kgr_patch, patch_fun->name)) {
			next_state = KGR_PATCH_REVERT_SLOW;
			patch_fun->loc_old = patch_fun->loc_name;
			/*
			 * Check for the last finalized patch is enough. We are
			 * here only when the function is not included in the
			 * patch in progress (kgr_patch).
			 */
			if (kgr_is_patch_fun(patch_fun, KGR_LAST_FINALIZED))
				new_ops = &patch_fun->ftrace_ops_slow;
		} else {
			next_state = KGR_PATCH_APPLIED;
			/*
			 * Check for the last existing and not the last
			 * finalized patch_fun here! This function is called
			 * for all patches. There might be another patch_fun
			 * in the patch in progress that will need to register
			 * the ftrace ops.
			 */
			if (kgr_is_patch_fun(patch_fun, KGR_LAST_EXISTING))
				new_ops = &patch_fun->ftrace_ops_fast;
		}
	}

	if (new_ops) {
		err = kgr_ftrace_enable(patch_fun, new_ops);
		if (err) {
			pr_err("kgr: enabling of ftrace function for the originally skipped %lx (%s) failed with %d\n",
			       patch_fun->loc_old, patch_fun->name, err);
			return err;
		}
	}

	patch_fun->state = next_state;
	pr_debug("kgr: delayed redirection for %s done\n", patch_fun->name);
	return 0;
}

/*
 * This function is called when new module is loaded but before it is used.
 * Therefore it could set the fast path for already finalized patches.
 */
static void kgr_handle_patch_for_loaded_module(struct kgr_patch *patch,
					       const struct module *mod)
{
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

		kgr_patch_code_delayed(patch_fun);
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
	if (kgr_patch)
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
	struct ftrace_ops *ops = NULL;
	int err;

	switch (patch_fun->state) {
	case KGR_PATCH_INIT:
	case KGR_PATCH_SKIPPED:
		return 0;
	case KGR_PATCH_SLOW:
	case KGR_PATCH_REVERT_SLOW:
		if (kgr_is_patch_fun(patch_fun, KGR_LAST_EXISTING))
			ops = &patch_fun->ftrace_ops_slow;
		break;
	case KGR_PATCH_APPLIED:
		if (kgr_is_patch_fun(patch_fun, KGR_LAST_EXISTING))
			ops = &patch_fun->ftrace_ops_fast;
		break;
	default:
		return -EINVAL;
	}

	if (ops) {
		err = kgr_ftrace_disable(patch_fun, ops);
		if (err) {
			pr_warn("kgr: forced disabling of ftrace function for %s failed with %d\n",
				patch_fun->name, err);
			return err;
		}
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
