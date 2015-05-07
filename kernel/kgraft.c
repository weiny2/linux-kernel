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
#include <linux/bug.h>

static int kgr_patch_code(struct kgr_patch_fun *patch_fun, bool final,
		bool revert, bool replace_revert);
static void kgr_work_fn(struct work_struct *work);
static void __kgr_handle_going_module(const struct module *mod);

static struct workqueue_struct *kgr_wq;
static DECLARE_DELAYED_WORK(kgr_work, kgr_work_fn);
static DEFINE_MUTEX(kgr_in_progress_lock);
static LIST_HEAD(kgr_patches);
static bool __percpu *kgr_irq_use_new;
bool kgr_in_progress;
bool kgr_force_load_module;
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
	struct task_struct *p, *t;
	bool failed = false;

	read_lock(&tasklist_lock);
	for_each_process_thread(p, t) {
		if (kgr_task_in_progress(t)) {
			failed = true;
			goto unlock;
		}
	}
unlock:
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
 *
 * In case finalization fails the system is in inconsistent state with no way
 * out. It is better to BUG() in this situation.
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
			if (ret < 0) {
				/*
				 * Note: This should not happen. We only disable
				 * slow stubs and if this failed we would BUG in
				 * kgr_switch_fops called by kgr_patch_code. But
				 * leave it here to be sure.
				 */
				pr_err("kgr: finalization for %s failed (%d). System in inconsistent state with no way out.\n",
					pf->name, ret);
				BUG();
			}
		}
}

static void kgr_finalize(void)
{
	struct kgr_patch_fun *patch_fun;
	int ret;

	mutex_lock(&kgr_in_progress_lock);

	kgr_for_each_patch_fun(kgr_patch, patch_fun) {
		ret = kgr_patch_code(patch_fun, true, kgr_revert, false);

		if (ret < 0) {
			pr_err("kgr: finalization for %s failed (%d). System in inconsistent state with no way out.\n",
				patch_fun->name, ret);
			BUG();
		}

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

	pr_info("kgr succeeded\n");

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
	struct task_struct *p, *t;

	read_lock(&tasklist_lock);
	for_each_process_thread(p, t)
		kgr_task_safe(t);
	read_unlock(&tasklist_lock);
}

static void kgr_handle_processes(void)
{
	struct task_struct *p, *t;

	read_lock(&tasklist_lock);
	for_each_process_thread(p, t) {
		/* skip tasks wandering in userspace as already migrated */
		if (kgr_needs_lazy_migration(t))
			kgr_mark_task_in_progress(t);
		/* wake up kthreads, they will clean the progress flag */
		if (t->flags & PF_KTHREAD) {
			/*
			 * this is incorrect for kthreads waiting still for
			 * their first wake_up.
			 */
			wake_up_process(t);
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

static int kgr_switch_fops(struct kgr_patch_fun *patch_fun,
		struct ftrace_ops *new_fops, struct ftrace_ops *unreg_fops)
{
	int err;

	if (new_fops) {
		err = kgr_ftrace_enable(patch_fun, new_fops);
		if (err) {
			pr_err("kgr: cannot enable ftrace function for %s (%lx, %d)\n",
				patch_fun->name, patch_fun->loc_old, err);
			return err;
		}
	}

	/*
	 * Get rid of the other stub. Having two stubs in the interim is fine.
	 * The first one registered always "wins", as it'll be dragged last from
	 * the ftrace hashtable. The redirected RIP however currently points to
	 * the same function in both stubs.
	 */
	if (unreg_fops) {
		err = kgr_ftrace_disable(patch_fun, unreg_fops);
		if (err) {
			pr_err("kgr: disabling ftrace function for %s failed (%d)\n",
				patch_fun->name, err);
			/*
			 * In case of failure we do not know which state we are
			 * in. There is something wrong going on in kGraft of
			 * ftrace, so better BUG.
			 */
			BUG();
		}
	}

	return 0;
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

	/*
	 * In case of error the caller can still have a chance to restore the
	 * previous consistent state.
	 */
	err = kgr_switch_fops(patch_fun, new_ops, unreg_ops);
	if (err)
		return err;

	patch_fun->state = next_state;

	pr_debug("kgr: redirection for %s done\n", patch_fun->name);

	return 0;
}

/*
 * The patching failed so we need to go one step back for the affected
 * patch_fun. It means to switch the present slow stubs to previous fast stubs
 * for KGR_PATCH_SLOW and KGR_PATCH_REVERT_SLOW states. Nothing has to be done
 * for KGR_PATCH_APPLIED and KGR_PATCH_SKIPPED.
 *
 * This function is intended for the first stage of patching. It must not be
 * used during the finalization (kgr_finalize).
 */
static void kgr_patch_code_failed(struct kgr_patch_fun *patch_fun)
{
	struct ftrace_ops *new_fops = NULL, *unreg_fops = NULL;
	enum kgr_patch_state next_state;
	unsigned long old_fun;
	int err;

	switch (patch_fun->state) {
	case KGR_PATCH_SLOW:
		new_fops = kgr_get_old_fops(patch_fun);
		unreg_fops = &patch_fun->ftrace_ops_slow;
		next_state = KGR_PATCH_INIT;
		break;
	case KGR_PATCH_REVERT_SLOW:
		/*
		 * There is a check for KGR_LAST_FINALIZED at similar place in
		 * kgr_patch_code. Here we need to test for KGR_LAST_EXISTING,
		 * because kgr_patch_code_failed can be called also on patch_fun
		 * from kgr_patch (which is in reverting process).
		 */
		if (kgr_is_patch_fun(patch_fun, KGR_LAST_EXISTING)) {
			new_fops = &patch_fun->ftrace_ops_fast;
			unreg_fops = &patch_fun->ftrace_ops_slow;
		}
		/*
		 * Set loc_old back to the previous stacked function. The check
		 * is not optimal, because loc_name is valid for some
		 * patch_funs, but it is not wrong and certainly the simplest.
		 */
		if (patch_fun->loc_old == patch_fun->loc_name) {
			old_fun = kgr_get_old_fun(patch_fun);
			/*
			 * This should not happen. loc_old had been correctly
			 * set before (by kgr_init_ftrace_ops).
			 */
			WARN(!old_fun, "kgr: loc_old is set incorrectly for %s. Do not revert anything!",
				patch_fun->name);
			patch_fun->loc_old = old_fun;
		}
		next_state = KGR_PATCH_APPLIED;
		break;
	case KGR_PATCH_APPLIED:
		/*
		 * Nothing to do. Although kgr_patching_failed is called from
		 * kgr_revert_replaced_funs, it does not touch non-replaced
		 * functions at all.
		 */
		return;
	case KGR_PATCH_SKIPPED:
		/*
		 * patch_fun can be in SKIPPED state, because affected module
		 * can be missing. Nothing to do here.
		 */
		return;
	default:
		/*
		 * patch_fun cannot be in any other state given the
		 * circumstances (all is (being) applied/reverted)
		 */
		pr_warn("kgr: kgr_patch_code_failed: unexpected patch function state (%d)\n",
			patch_fun->state);
		return;
	}

	err = kgr_switch_fops(patch_fun, new_fops, unreg_fops);
	/*
	 * Patching failed, attempt to handle the failure failed as well. We do
	 * not know which patch_funs were (partially) (un)patched and which were
	 * not. The system can be in unstable state so we BUG().
	 */
	BUG_ON(err);

	patch_fun->state = next_state;
}

/*
 * In case kgr_patch_code call has failed (due to any of the symbol resolutions
 * or something else), return all the functions patched up to now back to
 * previous state so we can fail with grace.
 *
 * This function is intended for the first stage of patching. It must not be
 * used during the finalization (kgr_finalize).
 */
static void kgr_patching_failed(struct kgr_patch *patch,
		struct kgr_patch_fun *patch_fun, bool process_all)
{
	struct kgr_patch_fun *pf;
	struct kgr_patch *p = patch;

	for (patch_fun--; patch_fun >= p->patches; patch_fun--)
		kgr_patch_code_failed(patch_fun);

	if (process_all) {
		/*
		 * kgr_patching_failed may be called during application of
		 * replace_all patch. In this case all reverted patch_funs not
		 * present in the patch have to be returned back to the previous
		 * states. The variable patch contains patch in progress which
		 * is not in the kgr_patches list.
		 * list_for_each_entry_continue_reverse continues with the
		 * previous entry in the list. This would crash when called on
		 * replace_all patch. So we add it to the list temporarily to
		 * ensure the list traversal works.
		 */
		if (patch == kgr_patch)
			list_add_tail(&patch->list, &kgr_patches);

		list_for_each_entry_continue_reverse(p, &kgr_patches, list)
			kgr_for_each_patch_fun(p, pf)
				kgr_patch_code_failed(pf);

		if (patch == kgr_patch)
			list_del(&patch->list);
	}

	WARN(1, "kgr: patching failed. Previous state was recovered.\n");
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
	unsigned long loc_old_temp;
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
				loc_old_temp = pf->loc_old;
				pf->loc_old = pf->loc_name;

				ret = kgr_patch_code(pf, false, true, true);
				if (ret < 0) {
					pr_err("kgr: cannot revert function %s in patch %s (%d)\n",
					      pf->name, p->name, ret);
					pf->loc_old = loc_old_temp;
					kgr_patching_failed(p, pf, true);
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
	 * Set kgr_patch before it can be used in kgr_patching_failed if
	 * something bad happens.
	 */
	kgr_patch = patch;

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
		if (ret < 0) {
			kgr_patching_failed(patch, patch_fun,
				patch->replace_all && !revert);
			goto err_free;
		}
	}
	kgr_in_progress = true;
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
	kgr_patch = NULL;
	/* No need for barrier as there are no slow stubs involved */
	clear_bit(0, kgr_immutable);
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
 *
 * The patching of the module (registration of the stubs) could fail. This would
 * prevent the loading. However the user can force the loading. In such
 * situation we continue. This can lead the inconsistent system state but the
 * user should know what he is doing.
 */
static int kgr_handle_patch_for_loaded_module(struct kgr_patch *patch,
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
		if (err) {
			if (kgr_force_load_module) {
				WARN(1, "kgr: delayed patching of the module (%s) failed (%d). Insertion of the module forced.\n",
					mod->name, err);
				continue;
			} else {
				return err;
			}
		}

		err = kgr_patch_code_delayed(patch_fun);
		if (err) {
			if (kgr_force_load_module)
				WARN(1, "kgr: delayed patching of the module (%s) failed (%d). Insertion of the module forced.\n",
					mod->name, err);
			else
				return err;
		}
	}

	return 0;
}

/**
 * kgr_module_init -- apply skipped patches for newly loaded modules
 *
 * It must be called when symbols are visible to kallsyms but before the module
 * init is called. Otherwise, it would not be able to use the fast stub.
 */
int kgr_module_init(const struct module *mod)
{
	struct kgr_patch *p;
	int ret;

	/* early modules will be patched once KGraft is initialized */
	if (!kgr_initialized)
		return 0;

	mutex_lock(&kgr_in_progress_lock);

	/*
	 * Check already applied patches for skipped functions. If there are
	 * more patches we want to set them all. They need to be in place when
	 * we remove some patch.
	 * If the patching fails remove all stubs. It is doable by call to
	 * module going notifier.
	 */
	list_for_each_entry(p, &kgr_patches, list) {
		ret = kgr_handle_patch_for_loaded_module(p, mod);
		if (ret) {
			pr_err("kgr: delayed patching of the module (%s) failed (%d). Module was not inserted.\n",
				mod->name, ret);
			__kgr_handle_going_module(mod);
			goto out;
		}
	}

	/* also check the patch in progress that is being applied */
	if (kgr_patch) {
		ret = kgr_handle_patch_for_loaded_module(kgr_patch, mod);
		if (ret) {
			pr_err("kgr: delayed patching of the module (%s) failed (%d). Module was not inserted.\n",
				mod->name, ret);
			__kgr_handle_going_module(mod);
			goto out;
		}
	}

out:
	mutex_unlock(&kgr_in_progress_lock);
	return ret;
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
			pr_err("kgr: forced disabling of ftrace function for %s failed (%d)\n",
				patch_fun->name, err);
			/*
			 * Cannot remove stubs for leaving module. This is very
			 * suspicious situation, so we better BUG here.
			 */
			BUG();
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
 * The module removal cannot be stopped at this stage. All affected patches have
 * to be removed. Ftrace does not unregister stubs itself in order to optimize
 * when the affected module gets loaded again. We have to do it ourselves. If we
 * fail here and the module is loaded once more, we are going to patch it. This
 * could lead to the conflicts with ftrace and more errors. We would not be able
 * to load the module cleanly.
 *
 * In case of any error we BUG in the process.
 */
static void __kgr_handle_going_module(const struct module *mod)
{
	struct kgr_patch *p;

	list_for_each_entry(p, &kgr_patches, list)
		kgr_handle_patch_for_going_module(p, mod);

	/* also check the patch in progress for removed functions */
	if (kgr_patch)
		kgr_handle_patch_for_going_module(kgr_patch, mod);
}

static void kgr_handle_going_module(const struct module *mod)
{
	/* Nope when kGraft has not been initialized yet */
	if (!kgr_initialized)
		return;

	mutex_lock(&kgr_in_progress_lock);
	__kgr_handle_going_module(mod);
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
	if (ret) {
		pr_err("kgr: failed to register kGraft module exit notifier (%d)\n",
			ret);
		goto err_destroy_wq;
	}

	kgr_initialized = true;
	pr_info("kgr: successfully initialized\n");

	return 0;
err_destroy_wq:
	destroy_workqueue(kgr_wq);
err_remove_files:
	kgr_remove_files();

	return ret;
}
module_init(kgr_init);
