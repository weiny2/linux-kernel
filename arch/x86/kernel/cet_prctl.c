/* SPDX-License-Identifier: GPL-2.0 */

#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/prctl.h>
#include <linux/compat.h>
#include <linux/mman.h>
#include <linux/elfcore.h>
#include <asm/processor.h>
#include <asm/prctl.h>
#include <asm/cet.h>

/* See Documentation/x86/intel_cet.rst. */

static int handle_get_status(unsigned long arg2)
{
	struct cet_status *cet = &current->thread.cet;
	unsigned int features = 0;
	unsigned long buf[3];

	if (cet->shstk_enabled)
		features |= GNU_PROPERTY_X86_FEATURE_1_SHSTK;

	buf[0] = (unsigned long)features;
	buf[1] = cet->shstk_base;
	buf[2] = cet->shstk_size;
	return copy_to_user((unsigned long __user *)arg2, buf,
			    sizeof(buf));
}

static int handle_alloc_shstk(unsigned long arg2)
{
	int err = 0;
	unsigned long arg;
	unsigned long addr = 0;
	unsigned long size = 0;

	if (get_user(arg, (unsigned long __user *)arg2))
		return -EFAULT;

	size = arg;
	err = cet_alloc_shstk(&arg);
	if (err)
		return err;

	addr = arg;
	if (put_user(addr, (unsigned long __user *)arg2)) {
		vm_munmap(addr, size);
		return -EFAULT;
	}

	return 0;
}

int prctl_cet(int option, unsigned long arg2)
{
	struct cet_status *cet = &current->thread.cet;

	if (!cpu_x86_cet_enabled())
		return -EINVAL;

	switch (option) {
	case ARCH_X86_CET_STATUS:
		return handle_get_status(arg2);

	case ARCH_X86_CET_DISABLE:
		if (cet->locked)
			return -EPERM;
		if (arg2 & GNU_PROPERTY_X86_FEATURE_1_SHSTK)
			cet_disable_free_shstk(current);

		return 0;

	case ARCH_X86_CET_LOCK:
		cet->locked = 1;
		return 0;

	case ARCH_X86_CET_ALLOC_SHSTK:
		return handle_alloc_shstk(arg2);

	default:
		return -EINVAL;
	}
}
