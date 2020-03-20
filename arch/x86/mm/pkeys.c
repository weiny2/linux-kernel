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
	if ((vma->vm_flags & VM_ACCESS_FLAGS) != VM_EXEC)
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

/* The IA32_PKRS MSR is cached per CPU. */
static DEFINE_PER_CPU(u32, pkrs);

void pks_init_task(struct task_struct *tsk)
{
	tsk->thread.pkrs = init_pkrs_value;
}

/* Update PKRS MSR and its cache on local CPU. */
static void pkrs_update(u32 pkrs_val)
{
	this_cpu_write(pkrs, pkrs_val);
	wrmsrl(MSR_IA32_PKRS, pkrs_val);
}

/* Init PKRS MSR to ensure pkey-protection unless pkey is changed. */
void pks_init(void)
{
	pkrs_update(init_pkrs_value);
}

/* Upload current's pkrs to MSR after switching to current. */
void pkrs_sched_in(void)
{
	u64 current_pkrs = current->thread.pkrs;

	/* Only update the MSR when current's pkrs is different from the MSR. */
	if (this_cpu_read(pkrs) == current_pkrs)
		return;

	pkrs_update(current_pkrs);
}

/* Get new pkru or pkrs value with new protection bits in the key. */
u32 get_new_pkr(u32 old_pkr, int pkey, unsigned long init_val)
{
	int pkey_shift = (pkey * PKRU_BITS_PER_PKEY);
	u32 new_pkr_bits = 0;

	/* Set the bits we need in PKRU:  */
	if (init_val & PKEY_DISABLE_ACCESS)
		new_pkr_bits |= PKRU_AD_BIT;
	if (init_val & PKEY_DISABLE_WRITE)
		new_pkr_bits |= PKRU_WD_BIT;

	/* Shift the bits in to the correct place in PKRU for pkey: */
	new_pkr_bits <<= pkey_shift;

	/* Mask off any old bits in place: */
	old_pkr &= ~((PKRU_AD_BIT | PKRU_WD_BIT) << pkey_shift);

	/* Return old part along with new part: */
	return old_pkr | new_pkr_bits;
}

/*
 * Check if a PKS key is valid for free/update operations.
 * Key 0 is reserved for the kernel and always invalid for the operations.
 */
static bool pks_key_validate(int pkey)
{
	if (!static_cpu_has(X86_FEATURE_PKS))
		return false;

	if (pkey >= PKS_NUM_KEYS || pkey <= PKS_KERN_DEFAULT_KEY)
		return false;

	return true;
}

/* Update current task's pkrs and local PKRS MSR. */
int update_local_sup_key(int pkey, unsigned long protection)
{
	u32 current_pkrs, cpu_pkrs, new_pkrs;

	if (!pks_key_validate(pkey))
		return -EINVAL;

	/* Get current's pkrs bits for pkey. */
	current_pkrs = current->thread.pkrs;
	new_pkrs = get_new_pkr(current_pkrs, pkey, protection);

	current->thread.pkrs = new_pkrs;
	set_thread_flag(TIF_PKS);

	preempt_disable();
	cpu_pkrs = this_cpu_read(pkrs);
	/* Update MSR only when protections are different. */
	if (cpu_pkrs != new_pkrs)
		pkrs_update(new_pkrs);
	preempt_enable();

	return 0;
}
EXPORT_SYMBOL_GPL(update_local_sup_key);

#undef pr_fmt
#define pr_fmt(fmt) "x86/PKS: " fmt

static const char pks_key_user0[] = "kernel";
/*
 * The array stores names of allocated keys. After a key is allocated, the
 * pointer indexed by the key points to a key name string. Key 0 is reserved
 * for the kernel. All other keys can be allocated dynamically.
 */
static const char *pks_key_users[PKS_NUM_KEYS] = {
	pks_key_user0
};

/*
 * One bit per protection key says whether the kernel can use it or not.
 * Protected by atomic bit operations. Bit 0 is set for key 0 initially.
 * Totally 16 bits for 16 keys. Defined as unsigned long to avoid split
 * lock in atomic bit ops.
 */
static unsigned long pks_key_allocation_map = 1 << PKS_KERN_DEFAULT_KEY;

/*
 * Allocate a PKS key for a given name. Caller must maintain the name string
 * in memory during the key's life cycle.
 */
int pks_key_alloc(const char * const pkey_user)
{
	int nr, old_bit, pkey = -ENOSPC;

	if (!static_cpu_has(X86_FEATURE_PKS))
		return -EINVAL;

	if (!pkey_user)
		return -EINVAL;

	/* Find a free bit (0) in the bit map. */
	while (1) {
		nr = ffz(pks_key_allocation_map);
		if (nr > PKS_NUM_KEYS - 1) {
			int i;

			pr_info("Cannot allocate key for %s. Key users are:\n",
				pkey_user);
			for (i = PKS_KERN_DEFAULT_KEY; i < PKS_NUM_KEYS; i++)
				pr_info("%s\n", pks_key_users[i]);

			return -ENOSPC;
		}

		old_bit = test_and_set_bit_lock(nr, &pks_key_allocation_map);
		if (!old_bit)
			break;
        }
	/* Allocate corresponding key. */
	pkey = nr;
	pks_key_users[pkey] = pkey_user;

	return pkey;
}
EXPORT_SYMBOL_GPL(pks_key_alloc);

/* Free a PKS key. */
void pks_key_free(int pkey)
{
	if (!pks_key_validate(pkey))
		return;

	clear_bit(pkey, &pks_key_allocation_map);

	if (pks_key_users[pkey]) {
		pks_key_users[pkey] = NULL;
		/* Restore to default AD=1 and WD=0. */
		update_local_sup_key(pkey,
				     PKEY_DISABLE_ACCESS | ~PKEY_DISABLE_WRITE);
	}
}
EXPORT_SYMBOL_GPL(pks_key_free);
