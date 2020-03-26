// SPDX-License-Identifier: GPL-2.0-only

#include <linux/export.h>
#include <asm/mktme.h>

/* Mask to extract KeyID from physical address. */
phys_addr_t __mktme_keyid_mask;
phys_addr_t mktme_keyid_mask(void)
{
	return __mktme_keyid_mask;
}
EXPORT_SYMBOL_GPL(mktme_keyid_mask);

/* Shift of KeyID within physical address. */
int __mktme_keyid_shift;
int mktme_keyid_shift(void)
{
	return __mktme_keyid_shift;
}
EXPORT_SYMBOL_GPL(mktme_keyid_shift);

/*
 * Number of KeyIDs available for MKTME.
 * Excludes KeyID-0 which used by TME. MKTME KeyIDs start from 1.
 */
int __mktme_nr_keyids;
int mktme_nr_keyids(void)
{
	return __mktme_nr_keyids;
}
