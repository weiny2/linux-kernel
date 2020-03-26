/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef	_ASM_X86_MKTME_H
#define	_ASM_X86_MKTME_H

#include <linux/types.h>

#ifdef CONFIG_X86_INTEL_MKTME
extern phys_addr_t __mktme_keyid_mask;
extern phys_addr_t mktme_keyid_mask(void);
extern int __mktme_keyid_shift;
extern int mktme_keyid_shift(void);
extern int __mktme_nr_keyids;
extern int mktme_nr_keyids(void);
#else
#define mktme_keyid_mask()	((phys_addr_t)0)
#define mktme_nr_keyids()	0
#define mktme_keyid_shift()	0
#endif

#endif
