// SPDX-License-Identifier: GPL-3.0

/* Documentation/x86/mktme/ */

#include <linux/init.h>
#include <linux/key.h>
#include <linux/mm.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <asm/intel_pconfig.h>
#include <asm/mktme.h>

#include "mktme_format.h"

static DEFINE_SPINLOCK(mktme_lock);
static unsigned int mktme_available_keyids;  /* Free Hardware KeyIDs */
static struct kmem_cache *mktme_prog_cache;  /* Hardware programming cache */

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

int mktme_reserve_keyid(struct key *key)
{
	int i;

	if (!mktme_available_keyids)
		return 0;

	for (i = 1; i <= mktme_nr_keyids(); i++) {
		if (mktme_map[i].state == KEYID_AVAILABLE) {
			mktme_map[i].state = KEYID_ASSIGNED;
			mktme_map[i].key = key;
			mktme_available_keyids--;
			return i;
		}
	}
	return 0;
}

static void mktme_release_keyid(int keyid)
{
	 mktme_map[keyid].state = KEYID_AVAILABLE;
	 mktme_available_keyids++;
}

int mktme_keyid_from_key(struct key *key)
{
	int i;

	for (i = 1; i <= mktme_nr_keyids(); i++) {
		if (mktme_map[i].key == key)
			return i;
	}
	return 0;
}

void mktme_remove_key(struct key *key)
{
	int keyid = mktme_keyid_from_key(key);
	unsigned long flags;

	if (!mktme_keytype_enabled)
		return;
	spin_lock_irqsave(&mktme_lock, flags);
	mktme_release_keyid(keyid);
	spin_unlock_irqrestore(&mktme_lock, flags);
}

/* Copy the payload to the HW programming structure and program this KeyID */
static int mktme_program_keyid(int keyid, struct mktme_payload *payload)
{
	struct mktme_key_program *kprog = NULL;
	int ret;

	kprog = kmem_cache_zalloc(mktme_prog_cache, GFP_KERNEL);
	if (!kprog)
		return -ENOMEM;

	/* Hardware programming requires cached aligned struct */
	kprog->keyid = keyid;
	kprog->keyid_ctrl = payload->keyid_ctrl;
	memcpy(kprog->key_field_1, payload->data_key, MKTME_AES_XTS_SIZE);
	memcpy(kprog->key_field_2, payload->tweak_key, MKTME_AES_XTS_SIZE);

	ret = MKTME_PROG_SUCCESS;	/* Future programming call */
	kmem_cache_free(mktme_prog_cache, kprog);
	return ret;
}

int mktme_get_key(struct key *key, struct mktme_payload *payload)
{
	unsigned long flags;
	int keyid, ret;

	spin_lock_irqsave(&mktme_lock, flags);
	keyid = mktme_reserve_keyid(key);
	spin_unlock_irqrestore(&mktme_lock, flags);
	if (!keyid)
		return -ENOKEY;

	ret = mktme_program_keyid(keyid, payload);
	if (ret == MKTME_PROG_SUCCESS)
		return ret;

	spin_lock_irqsave(&mktme_lock, flags);
	mktme_release_keyid(keyid);
	spin_unlock_irqrestore(&mktme_lock, flags);
	return -ENOKEY;
}

static void mktme_build_new_payload(struct mktme_payload *payload)
{
	payload->keyid_ctrl = MKTME_KEYID_SET_KEY_DIRECT | MKTME_AES_XTS_128;
	get_random_bytes(&payload->data_key, MKTME_AES_XTS_SIZE);
	get_random_bytes(&payload->tweak_key, MKTME_AES_XTS_SIZE);
}

int mktme_verify_payload(struct mktme_payload *p)
{
	/* Verify the control field */
	if (p->keyid_ctrl != (MKTME_KEYID_SET_KEY_DIRECT | MKTME_AES_XTS_128))
		return -EINVAL;

	/*
	 * Check that the unused bits of data and tweak field are zero.
	 * Cannot verify the data and tweak key data itself.
	 */
	return 0;
}

int mktme_request_key(struct key *key, struct mktme_payload *payload,
		      int encrypted_key_cmd)
{
	enum { OPT_NEW, OPT_LOAD };     /* encrypted_key_cmd options */

	if (!mktme_keytype_enabled)
		return -EINVAL;

	if (encrypted_key_cmd == OPT_NEW) {
		mktme_build_new_payload(payload);
		return mktme_get_key(key, payload);
	}

	if (encrypted_key_cmd == OPT_LOAD) {
		if (!mktme_verify_payload(payload))
			return mktme_get_key(key, payload);
	}

	return -EINVAL;
}

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

	/* Used to program the hardware key tables */
	mktme_prog_cache = KMEM_CACHE(mktme_key_program, SLAB_PANIC);
	if (!mktme_prog_cache)
		goto free_map;

	pr_notice("Key type encrypted:mktme initialized\n");
	mktme_keytype_enabled = true;
	return 0;

free_map:
	kvfree(mktme_map);
	pr_warn("Key type encrypted:mktme initialization failed\n");
	return -ENOMEM;
}

late_initcall(init_mktme);
