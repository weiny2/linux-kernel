// SPDX-License-Identifier: GPL-3.0

/* Documentation/x86/mktme/ */

#include <linux/acpi.h>
#include <linux/cred.h>
#include <linux/cpu.h>
#include <linux/init.h>
#include <linux/key.h>
#include <linux/memory.h>
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
static bool mktme_allow_keys;		     /* HW topology supports keys */

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

static void mktme_clear_hardware_keyid(struct work_struct *work);
static DECLARE_WORK(mktme_clear_work, mktme_clear_hardware_keyid);

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
	mktme_map[keyid].state = KEYID_REF_RELEASED;
	spin_unlock_irqrestore(&mktme_lock, flags);
	schedule_work(&mktme_clear_work);
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

static void mktme_clear_hardware_keyid(struct work_struct *work)
{
	struct mktme_payload *clear_payload;
	unsigned long flags;
	int keyid, ret;

	clear_payload = kzalloc(sizeof(*clear_payload), GFP_KERNEL);
	if (!clear_payload)
		return;
	clear_payload->keyid_ctrl |= MKTME_KEYID_CLEAR_KEY;

	for (keyid = 1; keyid <= mktme_nr_keyids(); keyid++) {
		if (mktme_map[keyid].state != KEYID_REF_RELEASED)
			continue;

		ret = mktme_program_keyid(keyid, clear_payload);
		if (ret != MKTME_PROG_SUCCESS)
			pr_debug("mktme: clear key failed [%s]\n",
				 mktme_error[ret].msg);

		spin_lock_irqsave(&mktme_lock, flags);
		mktme_release_keyid(keyid);
		spin_unlock_irqrestore(&mktme_lock, flags);
	}
	kfree(clear_payload); /* nothing secret in here */
}

static void mktme_update_pconfig_targets(void);

int mktme_get_key(struct key *key, struct mktme_payload *payload)
{
	unsigned long flags;
	int ret = -ENOKEY;
	int keyid;

	spin_lock_irqsave(&mktme_lock, flags);

	/* Topology supports key creation */
	if (mktme_allow_keys)
		goto get_key;

	/* Topology unknown, check it. */
	if (!mktme_hmat_evaluate()) {
		ret = -EINVAL;
		goto out_unlock;
	}
	/* Keys are now allowed. Update the programming targets. */
	mktme_update_pconfig_targets();
	mktme_allow_keys = true;

get_key:
	keyid = mktme_reserve_keyid(key);
	spin_unlock_irqrestore(&mktme_lock, flags);
	if (!keyid)
		goto out;

	if (percpu_ref_init(&encrypt_count[keyid], mktme_percpu_ref_release,
			    0, GFP_KERNEL))
		goto out_free_key;

	ret = mktme_program_keyid(keyid, payload);
	if (ret == MKTME_PROG_SUCCESS)
		goto out;

	/* Key programming failed */
	percpu_ref_exit(&encrypt_count[keyid]);

out_free_key:
	spin_lock_irqsave(&mktme_lock, flags);
	mktme_release_keyid(keyid);
out_unlock:
	spin_unlock_irqrestore(&mktme_lock, flags);
out:
	return ret;
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
	if (!capable(CAP_SYS_RESOURCE) && !capable(CAP_SYS_ADMIN))
		return -EACCES;

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

static int mktme_cpu_teardown(unsigned int cpu)
{
	int new_leadcpu, ret = 0;
	unsigned long flags;

	/* Do not allow key programming during cpu hotplug event */
	spin_lock_irqsave(&mktme_lock, flags);

	/*
	 * When no keys are in use, allow the teardown, and set
	 * mktme_allow_keys to FALSE. That forces an evaluation
	 * of the topology before the next key creation.
	 */
	if (mktme_available_keyids == mktme_nr_keyids()) {
		mktme_allow_keys = false;
		goto out;
	}
	/* Teardown CPU is not a lead CPU. Allow teardown. */
	if (!cpumask_test_cpu(cpu, mktme_leadcpus))
		goto out;

	/* Teardown CPU is a lead CPU. Look for a new lead CPU. */
	new_leadcpu = cpumask_any_but(topology_core_cpumask(cpu), cpu);

	if (new_leadcpu < nr_cpumask_bits) {
		/* New lead CPU found. Update the programming mask */
		__cpumask_clear_cpu(cpu, mktme_leadcpus);
		__cpumask_set_cpu(new_leadcpu, mktme_leadcpus);
	} else {
		/* New lead CPU not found. Do not allow CPU teardown */
		ret = -1;
	}
out:
	spin_unlock_irqrestore(&mktme_lock, flags);
	return ret;
}

static int mktme_memory_callback(struct notifier_block *nb,
				 unsigned long action, void *arg)
{
	/*
	 * Do not allow the hot add of memory until run time
	 * support of the ACPI HMAT is available via an _HMA
	 * method. Without it, the new memory cannot be
	 * evaluated to determine an MTKME safe topology.
	 */
	if (action == MEM_GOING_ONLINE)
		return NOTIFY_BAD;

	return NOTIFY_OK;
}

static struct notifier_block mktme_memory_nb = {
	.notifier_call = mktme_memory_callback,
	.priority = 99,				/* priority ? */
};

static int __init init_mktme(void)
{
	int cpuhp;

	mktme_keytype_enabled = false;

	/* Verify keys are present */
	if (mktme_nr_keyids() < 1)
		return 0;

	mktme_available_keyids = mktme_nr_keyids();

	/* Require an ACPI HMAT to identify MKTME safe topologies */
	if (!acpi_hmat_present()) {
		pr_warn("mktme: Initialization failed. No ACPI HMAT.\n");
		return -EINVAL;
	}

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

	cpuhp = cpuhp_setup_state_nocalls(CPUHP_AP_ONLINE_DYN,
					  "keys/mktme:online",
					  NULL, mktme_cpu_teardown);
	if (cpuhp < 0)
		goto free_encrypt;

	if (register_memory_notifier(&mktme_memory_nb))
		goto remove_cpuhp;

	pr_notice("Key type encrypted:mktme initialized\n");
	mktme_keytype_enabled = true;
	return 0;

remove_cpuhp:
	cpuhp_remove_state_nocalls(cpuhp);
free_encrypt:
	kvfree(encrypt_count);
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
