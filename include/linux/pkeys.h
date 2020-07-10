/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_PKEYS_H
#define _LINUX_PKEYS_H

#include <linux/mm.h>

#define ARCH_DEFAULT_PKEY	0

#ifdef CONFIG_ARCH_HAS_PKEYS
#include <asm/pkeys.h>
#else /* ! CONFIG_ARCH_HAS_PKEYS */
#define arch_max_pkey() (1)
#define execute_only_pkey(mm) (0)
#define arch_override_mprotect_pkey(vma, prot, pkey) (0)
#define PKEY_DEDICATED_EXECUTE_ONLY 0
#define ARCH_VM_PKEY_FLAGS 0

static inline int vma_pkey(struct vm_area_struct *vma)
{
	return 0;
}

static inline bool mm_pkey_is_allocated(struct mm_struct *mm, int pkey)
{
	return (pkey == 0);
}

static inline int mm_pkey_alloc(struct mm_struct *mm)
{
	return -1;
}

static inline int mm_pkey_free(struct mm_struct *mm, int pkey)
{
	return -EINVAL;
}

static inline int arch_set_user_pkey_access(struct task_struct *tsk, int pkey,
			unsigned long init_val)
{
	return 0;
}

static inline bool arch_pkeys_enabled(void)
{
	return false;
}

#endif /* ! CONFIG_ARCH_HAS_PKEYS */

#ifdef CONFIG_ARCH_ENABLE_SUPERVISOR_PKEYS

/**
 * DOC: PKS_KEY_ALLOCATION
 *
 * Users reserve a key value by adding an entry to enum pks_pkey_consumers with
 * a unique value from 1 to 15.  Then replacing that value in the
 * PKS_INIT_VALUE macro using the desired default protection; PKR_RW_KEY(),
 * PKR_WD_KEY(), or PKR_AD_KEY().
 *
 * PKS_KEY_DEFAULT must remain 0 key with a default of read/write to support
 * non-pks protected pages.  Unused keys should be set (Access Disabled
 * PKR_AD_KEY()).
 *
 * For example to configure a key for 'MY_FEATURE' with a default of Write
 * Disabled.
 *
 * .. code-block:: c
 *
 *	enum pks_pkey_consumers
 *	{
 *		PKS_KEY_DEFAULT         = 0,
 *		PKS_KEY_MY_FEATURE      = 1,
 *	}
 *
 *	#define PKS_INIT_VALUE (PKR_RW_KEY(PKS_KEY_DEFAULT)		|
 *				PKR_WD_KEY(PKS_KEY_MY_FEATURE)		|
 *				PKR_AD_KEY(2)	| PKR_AD_KEY(3)		|
 *				PKR_AD_KEY(4)	| PKR_AD_KEY(5)		|
 *				PKR_AD_KEY(6)	| PKR_AD_KEY(7)		|
 *				PKR_AD_KEY(8)	| PKR_AD_KEY(9)		|
 *				PKR_AD_KEY(10)	| PKR_AD_KEY(11)	|
 *				PKR_AD_KEY(12)	| PKR_AD_KEY(13)	|
 *				PKR_AD_KEY(14)	| PKR_AD_KEY(15))
 *
 */
enum pks_pkey_consumers {
	PKS_KEY_DEFAULT		= 0, /* Must be 0 for default PTE values */
	PKS_KEY_TEST		= 1,
};

#define PKS_INIT_VALUE (PKR_RW_KEY(PKS_KEY_DEFAULT)		| \
			PKR_AD_KEY(PKS_KEY_TEST)	| \
			PKR_AD_KEY(2)	| PKR_AD_KEY(3)		| \
			PKR_AD_KEY(4)	| PKR_AD_KEY(5)		| \
			PKR_AD_KEY(6)	| PKR_AD_KEY(7)		| \
			PKR_AD_KEY(8)	| PKR_AD_KEY(9)		| \
			PKR_AD_KEY(10)	| PKR_AD_KEY(11)	| \
			PKR_AD_KEY(12)	| PKR_AD_KEY(13)	| \
			PKR_AD_KEY(14)	| PKR_AD_KEY(15))

void pks_init_task(struct task_struct *task);

#else /* !CONFIG_ARCH_ENABLE_SUPERVISOR_PKEYS */

static inline void pks_init_task(struct task_struct *task) { }

#endif /* CONFIG_ARCH_ENABLE_SUPERVISOR_PKEYS */

#endif /* _LINUX_PKEYS_H */
