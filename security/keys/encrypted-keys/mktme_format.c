// SPDX-License-Identifier: GPL-3.0

/* Documentation/x86/mktme/ */

#include <linux/init.h>
#include <linux/key.h>
#include <linux/mm.h>
#include <asm/mktme.h>

/*
 * Maximum value of mktme_available_keyids is the total number
 * of MKTME keys supported by the platform hardare. This number
 * is reported by BIOS and returned by mktme_nr_keyids()
 */
static unsigned int mktme_available_keyids;
static bool mktme_keytype_enabled;

enum mktme_keyid_state {
	KEYID_AVAILABLE,	/* Available to be assigned */
	KEYID_ASSIGNED,		/* Assigned to a userspace key */
	KEYID_REF_KILLED,	/* Userspace key has been destroyed */
	KEYID_REF_RELEASED,	/* Last reference is released */
};

/* 1:1 Mapping between Userspace Keys (struct key) and Hardware KeyIDs */
struct mktme_mapping {
	struct key		*key;
	enum mktme_keyid_state	state;
};

static struct mktme_mapping *mktme_map;

static int __init init_mktme(void)
{
	mktme_keytype_enabled = false;

	/* Verify keys are present */
	if (mktme_nr_keyids() < 1)
		return 0;

	mktme_available_keyids = mktme_nr_keyids();

	/* Mapping of Userspace Keys to Hardware KeyIDs */
	mktme_map = kvzalloc((sizeof(*mktme_map) * (mktme_nr_keyids() + 1)),
			     GFP_KERNEL);
	if (!mktme_map)
		return -ENOMEM;

	pr_notice("Key type encrypted:mktme initialized\n");
	mktme_keytype_enabled = true;
	return 0;
}

late_initcall(init_mktme);
