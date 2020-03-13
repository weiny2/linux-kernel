// SPDX-License-Identifier: GPL-2.0-only
/*
 * Intel Memory Protection Keys management
 * Copyright (c) 2015, Intel Corporation.
 */
#include <linux/debugfs.h>		/* debugfs_create_u32()		*/
#include <linux/mm_types.h>             /* mm_struct, vma, etc...       */
#include <linux/pkeys.h>                /* PKEY_*                       */
#include <uapi/asm-generic/mman-common.h>

#include <asm/cpufeature.h>             /* boot_cpu_has, ...            */
#include <asm/mmu_context.h>            /* vma_pkey()                   */
#include <asm/fpu/internal.h>		/* init_fpstate			*/

/*
 * The IA32_PKRS MSR is cached in one variable. The MSRs on all CPUs have
 *  the same value. Initial value 0 means no protection in all keys in the MSR.
 */
static u64 ia32_pkrs_cached;

int __execute_only_pkey(struct mm_struct *mm)
{
	bool need_to_set_mm_pkey = false;
	int execute_only_pkey = mm->context.execute_only_pkey;
	int ret;

	/* Do we need to assign a pkey for mm's execute-only maps? */
	if (execute_only_pkey == -1) {
		/* Go allocate one to use, which might fail */
		execute_only_pkey = mm_pkey_alloc(mm);
		if (execute_only_pkey < 0)
			return -1;
		need_to_set_mm_pkey = true;
	}

	/*
	 * We do not want to go through the relatively costly
	 * dance to set PKRU if we do not need to.  Check it
	 * first and assume that if the execute-only pkey is
	 * write-disabled that we do not have to set it
	 * ourselves.
	 */
	if (!need_to_set_mm_pkey &&
	    !__pkru_allows_read(read_pkru(), execute_only_pkey)) {
		return execute_only_pkey;
	}

	/*
	 * Set up PKRU so that it denies access for everything
	 * other than execution.
	 */
	ret = arch_set_user_pkey_access(current, execute_only_pkey,
			PKEY_DISABLE_ACCESS);
	/*
	 * If the PKRU-set operation failed somehow, just return
	 * 0 and effectively disable execute-only support.
	 */
	if (ret) {
		mm_set_pkey_free(mm, execute_only_pkey);
		return -1;
	}

	/* We got one, store it and use it from here on out */
	if (need_to_set_mm_pkey)
		mm->context.execute_only_pkey = execute_only_pkey;
	return execute_only_pkey;
}

static inline bool vma_is_pkey_exec_only(struct vm_area_struct *vma)
{
	/* Do this check first since the vm_flags should be hot */
	if ((vma->vm_flags & (VM_READ | VM_WRITE | VM_EXEC)) != VM_EXEC)
		return false;
	if (vma_pkey(vma) != vma->vm_mm->context.execute_only_pkey)
		return false;

	return true;
}

/*
 * This is only called for *plain* mprotect calls.
 */
int __arch_override_mprotect_pkey(struct vm_area_struct *vma, int prot, int pkey)
{
	/*
	 * Is this an mprotect_pkey() call?  If so, never
	 * override the value that came from the user.
	 */
	if (pkey != -1)
		return pkey;

	/*
	 * The mapping is execute-only.  Go try to get the
	 * execute-only protection key.  If we fail to do that,
	 * fall through as if we do not have execute-only
	 * support in this mm.
	 */
	if (prot == PROT_EXEC) {
		pkey = execute_only_pkey(vma->vm_mm);
		if (pkey > 0)
			return pkey;
	} else if (vma_is_pkey_exec_only(vma)) {
		/*
		 * Protections are *not* PROT_EXEC, but the mapping
		 * is using the exec-only pkey.  This mapping was
		 * PROT_EXEC and will no longer be.  Move back to
		 * the default pkey.
		 */
		return ARCH_DEFAULT_PKEY;
	}

	/*
	 * This is a vanilla, non-pkey mprotect (or we failed to
	 * setup execute-only), inherit the pkey from the VMA we
	 * are working on.
	 */
	return vma_pkey(vma);
}

#define PKRU_AD_KEY(pkey)	(PKRU_AD_BIT << ((pkey) * PKRU_BITS_PER_PKEY))

/*
 * Make the default PKRU value (at execve() time) as restrictive
 * as possible.  This ensures that any threads clone()'d early
 * in the process's lifetime will not accidentally get access
 * to data which is pkey-protected later on.
 */
u32 init_pkru_value = PKRU_AD_KEY( 1) | PKRU_AD_KEY( 2) | PKRU_AD_KEY( 3) |
		      PKRU_AD_KEY( 4) | PKRU_AD_KEY( 5) | PKRU_AD_KEY( 6) |
		      PKRU_AD_KEY( 7) | PKRU_AD_KEY( 8) | PKRU_AD_KEY( 9) |
		      PKRU_AD_KEY(10) | PKRU_AD_KEY(11) | PKRU_AD_KEY(12) |
		      PKRU_AD_KEY(13) | PKRU_AD_KEY(14) | PKRU_AD_KEY(15);

/*
 * Called from the FPU code when creating a fresh set of FPU
 * registers.  This is called from a very specific context where
 * we know the FPU regstiers are safe for use and we can use PKRU
 * directly.
 */
void copy_init_pkru_to_fpregs(void)
{
	u32 init_pkru_value_snapshot = READ_ONCE(init_pkru_value);
	/*
	 * Override the PKRU state that came from 'init_fpstate'
	 * with the baseline from the process.
	 */
	write_pkru(init_pkru_value_snapshot);
}

static ssize_t init_pkru_read_file(struct file *file, char __user *user_buf,
			     size_t count, loff_t *ppos)
{
	char buf[32];
	unsigned int len;

	len = sprintf(buf, "0x%x\n", init_pkru_value);
	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t init_pkru_write_file(struct file *file,
		 const char __user *user_buf, size_t count, loff_t *ppos)
{
	struct pkru_state *pk;
	char buf[32];
	ssize_t len;
	u32 new_init_pkru;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	/* Make the buffer a valid string that we can not overrun */
	buf[len] = '\0';
	if (kstrtouint(buf, 0, &new_init_pkru))
		return -EINVAL;

	/*
	 * Don't allow insane settings that will blow the system
	 * up immediately if someone attempts to disable access
	 * or writes to pkey 0.
	 */
	if (new_init_pkru & (PKRU_AD_BIT|PKRU_WD_BIT))
		return -EINVAL;

	WRITE_ONCE(init_pkru_value, new_init_pkru);
	pk = get_xsave_addr(&init_fpstate.xsave, XFEATURE_PKRU);
	if (!pk)
		return -EINVAL;
	pk->pkru = new_init_pkru;
	return count;
}

static const struct file_operations fops_init_pkru = {
	.read = init_pkru_read_file,
	.write = init_pkru_write_file,
	.llseek = default_llseek,
};

static int __init create_init_pkru_value(void)
{
	debugfs_create_file("init_pkru", S_IRUSR | S_IWUSR,
			arch_debugfs_dir, NULL, &fops_init_pkru);
	return 0;
}
late_initcall(create_init_pkru_value);

static __init int setup_init_pkru(char *opt)
{
	u32 new_init_pkru;

	if (kstrtouint(opt, 0, &new_init_pkru))
		return 1;

	WRITE_ONCE(init_pkru_value, new_init_pkru);

	return 1;
}
__setup("init_pkru=", setup_init_pkru);

void update_ia32_pkrs(void *unused)
{
	wrmsrl(MSR_IA32_PKRS, READ_ONCE(ia32_pkrs_cached));
}

/*
 * Update ia32_pkrs_cached and IA32_PKRS MSRs on all CPUs with new
 * pmem key.
 */
void update_pmem_key(int ad, int wd)
{
	u64 key = 0;

	if (!cpu_feature_enabled(X86_FEATURE_PKS))
		return;

	ad &= 1;
	wd &= 1;

	key |= ((wd << PKS_WD_OFFSET) | (ad << PKS_AD_OFFSET)) <<
	       (PKS_KEY_IDX_PMEM * PKS_BITS_PER_KEY);

	WRITE_ONCE(ia32_pkrs_cached, key);

	on_each_cpu(update_ia32_pkrs, NULL, 1);
}
EXPORT_SYMBOL_GPL(update_pmem_key);

/*
 * Update the local cpu supervisor pkey
 */
void update_local_sup_key(u8 key, int ad, int wd)
{
	u64 msr = 0;

	if (!cpu_feature_enabled(X86_FEATURE_PKS))
		return;

	ad &= 1;
	wd &= 1;

	rdmsrl(MSR_IA32_PKRS, msr);

	msr &= ~(((1 << PKS_WD_OFFSET) | (1 << PKS_AD_OFFSET)) <<
	        (PKS_KEY_IDX_PMEM * PKS_BITS_PER_KEY));
	msr |= ((wd << PKS_WD_OFFSET) | (ad << PKS_AD_OFFSET)) <<
	       (PKS_KEY_IDX_PMEM * PKS_BITS_PER_KEY);

	wrmsrl(MSR_IA32_PKRS, msr);
}
EXPORT_SYMBOL_GPL(update_local_sup_key);
