// SPDX-License-Identifier: GPL-3.0

/* Documentation/x86/mktme/ */

#include <linux/cpu.h>
#include <linux/init.h>
#include <linux/key.h>
#include <linux/mm.h>
#include <linux/percpu-refcount.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <asm/intel_pconfig.h>
#include <asm/mktme.h>

#include "mktme_format.h"

static DEFINE_SPINLOCK(mktme_lock);
static unsigned int mktme_available_keyids;  /* Free Hardware KeyIDs */
static struct kmem_cache *mktme_prog_cache;  /* Hardware programming cache */
static unsigned long *mktme_target_map;      /* PCONFIG programming target */
static cpumask_var_t mktme_leadcpus;         /* One CPU per PCONFIG target */

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

struct percpu_ref *encrypt_count;
void mktme_percpu_ref_release(struct percpu_ref *ref)
{
	unsigned long flags;
	int keyid;

	for (keyid = 1; keyid <= mktme_nr_keyids(); keyid++) {
		if (&encrypt_count[keyid] == ref)
			break;
	}
	if (&encrypt_count[keyid] != ref) {
		pr_debug("%s: invalid ref counter\n", __func__);
		return;
	}
	percpu_ref_exit(ref);
	spin_lock_irqsave(&mktme_lock, flags);
	mktme_release_keyid(keyid);
	spin_unlock_irqrestore(&mktme_lock, flags);
}

struct mktme_hw_program_info {
	struct mktme_key_program *key_program;
	int *status;
};

struct mktme_err_table {
	const char *msg;
	bool retry;
};

static const struct mktme_err_table mktme_error[] = {
/* MKTME_PROG_SUCCESS     */ {"KeyID was successfully programmed",   false},
/* MKTME_INVALID_PROG_CMD */ {"Invalid KeyID programming command",   false},
/* MKTME_ENTROPY_ERROR    */ {"Insufficient entropy",		      true},
/* MKTME_INVALID_KEYID    */ {"KeyID not valid",		     false},
/* MKTME_INVALID_ENC_ALG  */ {"Invalid encryption algorithm chosen", false},
/* MKTME_DEVICE_BUSY      */ {"Failure to access key table",	      true},
};

static int mktme_parse_program_status(int status[])
{
	int cpu, sum = 0;

	/* Success: all CPU(s) programmed all key table(s) */
	for_each_cpu(cpu, mktme_leadcpus)
		sum += status[cpu];
	if (!sum)
		return MKTME_PROG_SUCCESS;

	/* Invalid Parameters: log the error and return the error. */
	for_each_cpu(cpu, mktme_leadcpus) {
		switch (status[cpu]) {
		case MKTME_INVALID_KEYID:
		case MKTME_INVALID_PROG_CMD:
		case MKTME_INVALID_ENC_ALG:
			pr_err("mktme: %s\n", mktme_error[status[cpu]].msg);
			return status[cpu];

		default:
			break;
		}
	}
	/*
	 * Device Busy or Insufficient Entropy: do not log the
	 * error. These will be retried and if retries (time or
	 * count runs out) caller will log the error.
	 */
	for_each_cpu(cpu, mktme_leadcpus) {
		if (status[cpu] == MKTME_DEVICE_BUSY)
			return status[cpu];
	}
	return MKTME_ENTROPY_ERROR;
}

/* Program a single key using one CPU. */
static void mktme_do_program(void *hw_program_info)
{
	struct mktme_hw_program_info *info = hw_program_info;
	int cpu;

	cpu = smp_processor_id();
	info->status[cpu] = mktme_key_program(info->key_program);
}

static int mktme_program_all_keytables(struct mktme_key_program *key_program)
{
	struct mktme_hw_program_info info;
	int err, retries = 10; /* Maybe users should handle retries */

	info.key_program = key_program;
	info.status = kcalloc(num_possible_cpus(), sizeof(info.status[0]),
			      GFP_KERNEL);

	while (retries--) {
		get_online_cpus();
		on_each_cpu_mask(mktme_leadcpus, mktme_do_program,
				 &info, 1);
		put_online_cpus();

		err = mktme_parse_program_status(info.status);
		if (!err)			   /* Success */
			return err;
		else if (!mktme_error[err].retry)  /* Error no retry */
			return -ENOKEY;
	}
	/* Ran out of retries */
	pr_err("mktme: %s\n", mktme_error[err].msg);
	return err;
}

void mktme_remove_key(struct key *key)
{
	int keyid = mktme_keyid_from_key(key);
	unsigned long flags;

	if (!mktme_keytype_enabled)
		return;
	spin_lock_irqsave(&mktme_lock, flags);
	mktme_map[keyid].key = NULL;
	mktme_map[keyid].state = KEYID_REF_KILLED;
	spin_unlock_irqrestore(&mktme_lock, flags);
	percpu_ref_kill(&encrypt_count[keyid]);
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

	ret = mktme_program_all_keytables(kprog);
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

	if (percpu_ref_init(&encrypt_count[keyid], mktme_percpu_ref_release,
			    0, GFP_KERNEL))
		goto err_out;

	ret = mktme_program_keyid(keyid, payload);
	if (ret == MKTME_PROG_SUCCESS)
		return ret;

	percpu_ref_exit(&encrypt_count[keyid]);
err_out:
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

static void mktme_update_pconfig_targets(void)
{
	int cpu, target_id;

	cpumask_clear(mktme_leadcpus);
	bitmap_clear(mktme_target_map, 0, sizeof(mktme_target_map));

	for_each_online_cpu(cpu) {
		target_id = topology_physical_package_id(cpu);
		if (!__test_and_set_bit(target_id, mktme_target_map))
			__cpumask_set_cpu(cpu, mktme_leadcpus);
	}
}

static int mktme_alloc_pconfig_targets(void)
{
	if (!alloc_cpumask_var(&mktme_leadcpus, GFP_KERNEL))
		return -ENOMEM;

	mktme_target_map = bitmap_alloc(topology_max_packages(), GFP_KERNEL);
	if (!mktme_target_map) {
		free_cpumask_var(mktme_leadcpus);
		return -ENOMEM;
	}
	return 0;
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

	/* Allocate the programming targets */
	if (mktme_alloc_pconfig_targets())
		goto free_cache;

	/* Initialize first programming targets */
	mktme_update_pconfig_targets();

	/* Reference counters to protect in use KeyIDs */
	encrypt_count = kvcalloc(mktme_nr_keyids() + 1,
				 sizeof(encrypt_count[0]), GFP_KERNEL);
	if (!encrypt_count)
		goto free_targets;

	pr_notice("Key type encrypted:mktme initialized\n");
	mktme_keytype_enabled = true;
	return 0;

free_targets:
	free_cpumask_var(mktme_leadcpus);
	bitmap_free(mktme_target_map);
free_cache:
	kmem_cache_destroy(mktme_prog_cache);
free_map:
	kvfree(mktme_map);
	pr_warn("Key type encrypted:mktme initialization failed\n");
	return -ENOMEM;
}

late_initcall(init_mktme);
