// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2019 Microsoft Corporation
 *
 * Author: Lakshmi Ramasubramanian (nramas@linux.microsoft.com)
 *
 * File: ima_asymmetric_keys.c
 *       Defines an IMA hook to measure asymmetric keys on key
 *       create or update.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <keys/asymmetric-type.h>
#include "ima.h"

/*
 * Flag to indicate whether a key can be processed
 * right away or should be queued for processing later.
 */
static bool ima_process_keys;

/*
 * To synchronize access to the list of keys that need to be measured
 */
static DEFINE_SPINLOCK(ima_keys_lock);
static LIST_HEAD(ima_keys);

static void ima_free_key_entry(struct ima_key_entry *entry)
{
	if (entry) {
		kfree(entry->payload);
		kfree(entry->keyring_name);
		kfree(entry);
	}
}

static struct ima_key_entry *ima_alloc_key_entry(struct key *keyring,
						 const void *payload,
						 size_t payload_len)
{
	int rc = 0;
	struct ima_key_entry *entry;

	entry = kzalloc(sizeof(*entry), GFP_KERNEL);
	if (entry) {
		entry->payload = kmemdup(payload, payload_len, GFP_KERNEL);
		entry->keyring_name = kstrdup(keyring->description,
					      GFP_KERNEL);
		entry->payload_len = payload_len;
	}

	if ((entry == NULL) || (entry->payload == NULL) ||
	    (entry->keyring_name == NULL)) {
		rc = -ENOMEM;
		goto out;
	}

	INIT_LIST_HEAD(&entry->list);

out:
	if (rc) {
		ima_free_key_entry(entry);
		entry = NULL;
	}

	return entry;
}

static bool ima_queue_key(struct key *keyring, const void *payload,
			  size_t payload_len)
{
	bool queued = false;
	struct ima_key_entry *entry;

	entry = ima_alloc_key_entry(keyring, payload, payload_len);
	if (!entry)
		return false;

	spin_lock(&ima_keys_lock);
	if (!ima_process_keys) {
		list_add_tail(&entry->list, &ima_keys);
		queued = true;
	}
	spin_unlock(&ima_keys_lock);

	if (!queued)
		ima_free_key_entry(entry);

	return queued;
}

/*
 * ima_process_queued_keys() - process keys queued for measurement
 *
 * This function sets ima_process_keys to true and processes queued keys.
 * From here on keys will be processed right away (not queued).
 */
void ima_process_queued_keys(void)
{
	struct ima_key_entry *entry, *tmp;
	bool process = false;

	if (ima_process_keys)
		return;

	/*
	 * Since ima_process_keys is set to true, any new key will be
	 * processed immediately and not be queued to ima_keys list.
	 * First one setting the ima_process_keys flag to true will
	 * process the queued keys.
	 */
	spin_lock(&ima_keys_lock);
	if (!ima_process_keys) {
		ima_process_keys = true;
		process = true;
	}
	spin_unlock(&ima_keys_lock);

	if (!process)
		return;

	list_for_each_entry_safe(entry, tmp, &ima_keys, list) {
		process_buffer_measurement(entry->payload, entry->payload_len,
					   entry->keyring_name, KEY_CHECK, 0,
					   entry->keyring_name);
		list_del(&entry->list);
		ima_free_key_entry(entry);
	}
}

/**
 * ima_post_key_create_or_update - measure asymmetric keys
 * @keyring: keyring to which the key is linked to
 * @key: created or updated key
 * @payload: The data used to instantiate or update the key.
 * @payload_len: The length of @payload.
 * @flags: key flags
 * @create: flag indicating whether the key was created or updated
 *
 * Keys can only be measured, not appraised.
 * The payload data used to instantiate or update the key is measured.
 */
void ima_post_key_create_or_update(struct key *keyring, struct key *key,
				   const void *payload, size_t payload_len,
				   unsigned long flags, bool create)
{
	bool queued = false;

	/* Only asymmetric keys are handled by this hook. */
	if (key->type != &key_type_asymmetric)
		return;

	if (!payload || (payload_len == 0))
		return;

	if (!ima_process_keys)
		queued = ima_queue_key(keyring, payload, payload_len);

	if (queued)
		return;

	/*
	 * keyring->description points to the name of the keyring
	 * (such as ".builtin_trusted_keys", ".ima", etc.) to
	 * which the given key is linked to.
	 *
	 * The name of the keyring is passed in the "eventname"
	 * parameter to process_buffer_measurement() and is set
	 * in the "eventname" field in ima_event_data for
	 * the key measurement IMA event.
	 *
	 * The name of the keyring is also passed in the "keyring"
	 * parameter to process_buffer_measurement() to check
	 * if the IMA policy is configured to measure a key linked
	 * to the given keyring.
	 */
	process_buffer_measurement(payload, payload_len,
				   keyring->description, KEY_CHECK, 0,
				   keyring->description);
}
