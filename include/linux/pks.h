/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_PKS_H
#define _LINUX_PKS_H

#ifdef CONFIG_ARCH_ENABLE_SUPERVISOR_PKEYS

#include <linux/types.h>

#include <uapi/asm-generic/mman-common.h>

#include <asm/pks.h>

/**
 * pks_available() - Is PKS available on this system
 *
 * Return if PKS is currently supported and enabled on this system.
 */
static inline bool pks_available(void)
{
	return arch_pks_available();
}

/**
 * pks_set_noaccess() - Disable all access to the domain
 * @pkey: the pkey for which the access should change.
 *
 * Disable all access to the domain specified by pkey.  This is not a global
 * update and only affects the current running thread.
 */
static inline void pks_set_noaccess(const u8 pkey)
{
	arch_pks_update_protection(pkey, PKEY_DISABLE_ACCESS);
}

/**
 * pks_set_nowrite() - Make the domain Read only
 * @pkey: the pkey for which the access should change.
 *
 * Allow read access to the domain specified by pkey.  This is not a global
 * update and only affects the current running thread.
 */
static inline void pks_set_nowrite(const u8 pkey)
{
	arch_pks_update_protection(pkey, PKEY_DISABLE_WRITE);
}

/**
 * pks_set_readwrite() - Make the domain Read/Write
 * @pkey: the pkey for which the access should change.
 *
 * Allow all access, read and write, to the domain specified by pkey.  This is
 * not a global update and only affects the current running thread.
 */
static inline void pks_set_readwrite(const u8 pkey)
{
	arch_pks_update_protection(pkey, PKEY_READ_WRITE);
}


/**
 * pks_update_exception() - Update the protections of a faulted thread
 *
 * @regs: Faulting thread registers
 * @pkey: pkey to update
 * @protection: protection bits to use.
 *
 * CONTEXT: Exception
 *
 * pks_update_exception() updates the faulted threads protections in addition
 * to the protections within the exception.
 *
 * This is useful because the pks_set_*() functions will not work to change the
 * protections of a thread which has been interrupted.  Only the current
 * context is updated by those functions.  Therefore, if a PKS fault callback
 * wants to update the faulted threads protections it must call
 * pks_update_exception().
 */
static inline void pks_update_exception(struct pt_regs *regs, const u8 pkey,
					const u8 protection)
{
	arch_pks_update_exception(regs, pkey, protection);
}

typedef bool (*pks_key_callback)(struct pt_regs *regs, unsigned long address,
				 bool write);

#else /* !CONFIG_ARCH_ENABLE_SUPERVISOR_PKEYS */

static inline bool pks_available(void)
{
	return false;
}

static inline void pks_set_noaccess(u8 pkey) {}
static inline void pks_set_nowrite(u8 pkey) {}
static inline void pks_set_readwrite(u8 pkey) {}
static inline void pks_update_exception(struct pt_regs *regs,
					u8 pkey,
					u8 protection)
{ }

#endif /* CONFIG_ARCH_ENABLE_SUPERVISOR_PKEYS */

#ifdef CONFIG_PKS_TEST

bool pks_test_fault_callback(struct pt_regs *regs, unsigned long address,
			     bool write);

#endif /* CONFIG_PKS_TEST */

#endif /* _LINUX_PKS_H */
