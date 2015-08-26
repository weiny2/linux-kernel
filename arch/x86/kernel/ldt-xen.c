/*
 * Copyright (C) 1992 Krishna Balasubramanian and Linus Torvalds
 * Copyright (C) 1999 Ingo Molnar <mingo@redhat.com>
 * Copyright (C) 2002 Andi Kleen
 *
 * This handles calls from both 32bit and 64bit mode.
 */

#include <linux/errno.h>
#include <linux/gfp.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/smp.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>

#include <asm/ldt.h>
#include <asm/desc.h>
#include <asm/mmu_context.h>
#include <asm/syscalls.h>

static void flush_ldt(void *current_mm)
{
	if (current->active_mm == current_mm) {
		/* context.lock is held for us, so we don't need any locking. */
		mm_context_t *pc = &current->active_mm->context;
		set_ldt(pc->ldt, pc->size);
	}
}

/* The caller must call finalize_ldt_struct on the result. */
static struct ldt_struct *alloc_ldt_struct(int size)
{
	struct ldt_struct *new_ldt;
	int alloc_size;

	if (size > LDT_ENTRIES)
		return NULL;

	new_ldt = kmalloc(sizeof(struct ldt_struct), GFP_KERNEL);
	if (!new_ldt)
		return NULL;

	BUILD_BUG_ON(LDT_ENTRY_SIZE != sizeof(struct desc_struct));
	alloc_size = size * LDT_ENTRY_SIZE;

	if (alloc_size > PAGE_SIZE)
		new_ldt->entries = vmalloc(alloc_size);
	else
		new_ldt->entries = (void *)__get_free_page(GFP_KERNEL);
	if (!new_ldt->entries) {
		kfree(new_ldt);
		return NULL;
	}

	new_ldt->size = size;
	return new_ldt;
}

/* After calling this, the LDT is immutable. */
static void finalize_ldt_struct(struct ldt_struct *ldt)
{
	make_pages_readonly(ldt->entries, PFN_DOWN(ldt->size * LDT_ENTRY_SIZE),
			    XENFEAT_writable_descriptor_tables);
}

static void install_ldt(struct mm_struct *current_mm,
			struct ldt_struct *ldt)
{
	/* context.lock is held */
	preempt_disable();

	/* Synchronizes with lockless_dereference in load_mm_ldt. */
	current_mm->context.size = ldt->size;
	smp_store_release(&current_mm->context.ldt, ldt->entries);

	/* Activate for this CPU. */
	flush_ldt(current->mm);

#ifdef CONFIG_SMP
	/* Synchronize with other CPUs. */
	if (!cpumask_equal(mm_cpumask(current_mm),
			   cpumask_of(smp_processor_id())))
		smp_call_function(flush_ldt, current_mm, 1);
#endif
	preempt_enable();
}

static void free_ldt_struct(void *ldt, int ldt_size)
{
	if (unlikely(ldt)) {
		int alloc_size = ldt_size * LDT_ENTRY_SIZE;

		make_pages_writable(ldt, PFN_DOWN(alloc_size),
				    XENFEAT_writable_descriptor_tables);
		if (alloc_size > PAGE_SIZE)
			vfree(ldt);
		else
			put_page(virt_to_page(ldt));
	}
}

/*
 * we do not have to muck with descriptors here, that is
 * done in switch_mm() as needed.
 */
int init_new_context(struct task_struct *tsk, struct mm_struct *mm)
{
	struct ldt_struct *new_ldt;
	struct mm_struct *old_mm;
	int retval = 0;

	memset(&mm->context, 0, sizeof(mm->context));
	mutex_init(&mm->context.lock);
	old_mm = current->mm;
	if (!old_mm)
		return 0;
	mm->context.vdso = old_mm->context.vdso;

	mutex_lock(&old_mm->context.lock);
	if (!old_mm->context.ldt)
		goto out_unlock;

	new_ldt = alloc_ldt_struct(old_mm->context.size);
	if (!new_ldt) {
		retval = -ENOMEM;
		goto out_unlock;
	}

	memcpy(new_ldt->entries, old_mm->context.ldt, new_ldt->size * LDT_ENTRY_SIZE);
	finalize_ldt_struct(new_ldt);

	mm->context.ldt  = new_ldt->entries;
	mm->context.size = new_ldt->size;

	/*
	 * This is different from upstream - we use new_ldt only up to here and
	 * and copy its contents into mm->context members.
	 * Free it here.
	 */
	kfree(new_ldt);

out_unlock:
	mutex_unlock(&old_mm->context.lock);
	return retval;
}

/*
 * No need to lock the MM as we are the last user
 *
 * 64bit: Don't touch the LDT register - we're already in the next thread.
 */
void destroy_context(struct mm_struct *mm)
{
	free_ldt_struct(mm->context.ldt, mm->context.size);
	mm->context.ldt = NULL;
}

static int read_ldt(void __user *ptr, unsigned long bytecount)
{
	int retval;
	unsigned long size;
	struct mm_struct *mm = current->mm;

	mutex_lock(&mm->context.lock);

	if (!mm->context.ldt) {
		retval = 0;
		goto out_unlock;
	}

	if (bytecount > LDT_ENTRY_SIZE * LDT_ENTRIES)
		bytecount = LDT_ENTRY_SIZE * LDT_ENTRIES;

	size = mm->context.size * LDT_ENTRY_SIZE;
	if (size > bytecount)
		size = bytecount;

	if (copy_to_user(ptr, mm->context.ldt, size)) {
		retval = -EFAULT;
		goto out_unlock;
	}

	if (size != bytecount) {
		/* Zero-fill the rest and pretend we read bytecount bytes. */
		if (clear_user(ptr + size, bytecount - size) != 0) {
			retval = -EFAULT;
			goto out_unlock;
		}
	}
	retval = bytecount;

out_unlock:
	mutex_unlock(&mm->context.lock);
	return retval;
}

static int read_default_ldt(void __user *ptr, unsigned long bytecount)
{
	/* CHECKME: Can we use _one_ random number ? */
#ifdef CONFIG_X86_32
	unsigned long size = 5 * sizeof(struct desc_struct);
#else
	unsigned long size = 128;
#endif
	if (bytecount > size)
		bytecount = size;
	if (clear_user(ptr, bytecount))
		return -EFAULT;
	return bytecount;
}

static int write_ldt(void __user *ptr, unsigned long bytecount, int oldmode)
{
	struct mm_struct *mm = current->mm;
	struct desc_struct ldt;
	int error;
	struct user_desc ldt_info;
	int oldsize, newsize;
	struct ldt_struct *new_ldt;
	void *old_ldt;

	error = -EINVAL;
	if (bytecount != sizeof(ldt_info))
		goto out;
	error = -EFAULT;
	if (copy_from_user(&ldt_info, ptr, sizeof(ldt_info)))
		goto out;

	error = -EINVAL;
	if (ldt_info.entry_number >= LDT_ENTRIES)
		goto out;
	if (ldt_info.contents == 3) {
		if (oldmode)
			goto out;
		if (ldt_info.seg_not_present == 0)
			goto out;
	}

	if ((oldmode && ldt_info.base_addr == 0 && ldt_info.limit == 0) ||
	    LDT_empty(&ldt_info)) {
		/* The user wants to clear the entry. */
		memset(&ldt, 0, sizeof(ldt));
	} else {
		if (!IS_ENABLED(CONFIG_X86_16BIT) && !ldt_info.seg_32bit) {
			error = -EINVAL;
			goto out;
		}

		fill_ldt(&ldt, &ldt_info);
		if (oldmode)
			ldt.avl = 0;
	}

	mutex_lock(&mm->context.lock);

	old_ldt = mm->context.ldt;
	oldsize = old_ldt ? mm->context.size : 0;
	newsize = max((int)(ldt_info.entry_number + 1), oldsize);

	error = -ENOMEM;
	new_ldt = alloc_ldt_struct(newsize);
	if (!new_ldt)
		goto out_unlock;

	if (old_ldt)
		memcpy(new_ldt->entries, old_ldt, oldsize * LDT_ENTRY_SIZE);

	memset(new_ldt->entries + oldsize, 0,
	       (newsize - oldsize) * LDT_ENTRY_SIZE);

	new_ldt->entries[ldt_info.entry_number] = ldt;
	finalize_ldt_struct(new_ldt);

	install_ldt(mm, new_ldt);
	free_ldt_struct(old_ldt, oldsize);

	/*
	 * Container new_ldt is not needed anymore, free it.
	 */
	kfree(new_ldt);

	error = 0;

out_unlock:
	mutex_unlock(&mm->context.lock);
out:
	return error;
}

asmlinkage int sys_modify_ldt(int func, void __user *ptr,
			      unsigned long bytecount)
{
	int ret = -ENOSYS;

	switch (func) {
	case 0:
		ret = read_ldt(ptr, bytecount);
		break;
	case 1:
		ret = write_ldt(ptr, bytecount, 1);
		break;
	case 2:
		ret = read_default_ldt(ptr, bytecount);
		break;
	case 0x11:
		ret = write_ldt(ptr, bytecount, 0);
		break;
	}
	return ret;
}
