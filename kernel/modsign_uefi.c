#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/cred.h>
#include <linux/err.h>
#include <linux/efi.h>
#include <linux/slab.h>
#include <crypto/public_key.h>
#include <keys/asymmetric-type.h>
#include <keys/system_keyring.h>
#include "module-internal.h"

struct efi_hash_type {
	u8 hash;                      /* Hash algorithm [enum pkey_hash_algo] */
	efi_guid_t efi_cert_guid;       /* EFI_CERT_*_GUID  */
	char *hash_name;                /* nams string of hash */
	size_t size;                    /* size of hash */
};

static const struct efi_hash_type efi_hash_types[] = {
	{PKEY_HASH_SHA256, EFI_CERT_SHA256_GUID, "sha256", 32},
	{PKEY_HASH_SHA1, EFI_CERT_SHA1_GUID, "sha1", 20},
	{PKEY_HASH_SHA512, EFI_CERT_SHA512_GUID, "sha512", 64},
	{PKEY_HASH_SHA224, EFI_CERT_SHA224_GUID, "sha224", 28},
	{PKEY_HASH_SHA384, EFI_CERT_SHA384_GUID, "sha384", 48},
	{PKEY_HASH__LAST}
};

static __init int check_ignore_db(void)
{
	efi_status_t status;
	unsigned int db = 0;
	unsigned long size = sizeof(db);
	efi_guid_t guid = EFI_SHIM_LOCK_GUID;

	/* Check and see if the MokIgnoreDB variable exists.  If that fails
	 * then we don't ignore DB.  If it succeeds, we do.
	 */
	status = efi.get_variable(L"MokIgnoreDB", &guid, NULL, &size, &db);
	if (status != EFI_SUCCESS)
		return 0;

	return 1;
}

static __init void *get_cert_list(efi_char16_t *name, efi_guid_t *guid, unsigned long *size)
{
	efi_status_t status;
	unsigned long lsize = 4;
	unsigned long tmpdb[4];
	void *db = NULL;

	status = efi.get_variable(name, guid, NULL, &lsize, &tmpdb);
	if (status != EFI_BUFFER_TOO_SMALL) {
		pr_err("Couldn't get size: 0x%lx\n", status);
		return NULL;
	}

	db = kmalloc(lsize, GFP_KERNEL);
	if (!db) {
		pr_err("Couldn't allocate memory for uefi cert list\n");
		goto out;
	}

	status = efi.get_variable(name, guid, NULL, &lsize, db);
	if (status != EFI_SUCCESS) {
		kfree(db);
		db = NULL;
		pr_err("Error reading db var: 0x%lx\n", status);
	}
out:
	*size = lsize;
	return db;
}

static int __init signature_blacklist_func(efi_guid_t efi_cert_guid,
		const efi_signature_data_t *elem, size_t esize,
		struct key *keyring)
{
	struct module_hash *module_hash = NULL;
	int i;

	for (i = 0; efi_hash_types[i].hash < PKEY_HASH__LAST; i++) {
		struct efi_hash_type type = efi_hash_types[i];

		if (efi_guidcmp(efi_cert_guid, type.efi_cert_guid) != 0)
			continue;

		if (strcmp(type.hash_name, CONFIG_MODULE_SIG_HASH)) {
			pr_err("Hash type is %s not %s: %*phN\n",
				type.hash_name, CONFIG_MODULE_SIG_HASH,
				(int)type.size, elem->signature_data);
			return -ENOTSUPP;
		}

		module_hash = kzalloc(sizeof(*module_hash) + type.size, GFP_KERNEL);
		if (!module_hash) {
			pr_err("module hash allocate fail\n");
			return -ENOMEM;
		}
		module_hash->hash = type.hash;
		module_hash->hash_name = type.hash_name;
		module_hash->size = type.size;
		memcpy(module_hash->hash_data, elem->signature_data, type.size);
	}

	if (!module_hash) {
		pr_err("Problem loading hash of blacklisted module: %pUb\n",
			&efi_cert_guid);
		return -ENOTSUPP;
	} else {
		pr_notice("Loaded %s hash %*phN to blacklisted modules\n",
				module_hash->hash_name, (int) module_hash->size,
				module_hash->hash_data);
	}

	list_add(&module_hash->list, &module_hash_blacklist);

	return 0;
}

static int __init parse_efi_signature_list_hashs(const void *data, size_t size)
{
	int i, rc = 0;

	for (i = 0; !rc && efi_hash_types[i].hash < PKEY_HASH__LAST; i++) {
		struct efi_hash_type type = efi_hash_types[i];
		rc = parse_efi_signature_db(data, size, NULL,
			type.efi_cert_guid, signature_blacklist_func);
		if (rc)
			pr_err("Couldn't parse signatures of %s hash: %d\n",
				type.hash_name, rc);
	}

	return rc;
}

/*
 *  * Load the certs contained in the UEFI databases
 *   */
static int __init load_uefi_certs(void)
{
	efi_guid_t secure_var = EFI_IMAGE_SECURITY_DATABASE_GUID;
	efi_guid_t mok_var = EFI_SHIM_LOCK_GUID;
	void *db = NULL, *dbx = NULL, *mok = NULL, *mokx = NULL;
	unsigned long dbsize = 0, dbxsize = 0, moksize = 0, mokxsize = 0;
	int ignore_db, rc = 0;

	/* Check if SB is enabled and just return if not */
	if (!efi_enabled(EFI_SECURE_BOOT))
		return 0;

	/* See if the user has setup Ignore DB mode */
	ignore_db = check_ignore_db();

	/* Get db, MokListRT, and dbx.  They might not exist, so it isn't
	 * an error if we can't get them.
	 */
	if (!ignore_db) {
		db = get_cert_list(L"db", &secure_var, &dbsize);
		if (!db) {
			pr_err("MODSIGN: Couldn't get UEFI db list\n");
		} else {
			rc = parse_efi_signature_list(db, dbsize, system_trusted_keyring);
			if (rc)
				pr_err("Couldn't parse db signatures: %d\n", rc);
			kfree(db);
		}
	}

	mok = get_cert_list(L"MokListRT", &mok_var, &moksize);
	if (!mok) {
		pr_info("MODSIGN: Couldn't get UEFI MokListRT\n");
	} else {
		rc = parse_efi_signature_list(mok, moksize, system_trusted_keyring);
		if (rc)
			pr_err("Couldn't parse MokListRT signatures: %d\n", rc);
		kfree(mok);
	}

	dbx = get_cert_list(L"dbx", &secure_var, &dbxsize);
	if (!dbx) {
		pr_info("MODSIGN: Couldn't get UEFI dbx list\n");
	} else {
		rc = parse_efi_signature_list(dbx, dbxsize,
			system_blacklist_keyring);
		if (rc)
			pr_err("Couldn't parse dbx signatures: %d\n", rc);
		kfree(dbx);
	}

	mokx = get_cert_list(L"MokListXRT", &mok_var, &mokxsize);
	if (!mokx) {
		pr_info("MODSIGN: Couldn't get UEFI MokListXRT\n");
	} else {
		rc = parse_efi_signature_list(mokx, mokxsize,
			system_blacklist_keyring);
		if (rc)
			pr_err("Couldn't parse MokListXRT signatures: %d\n", rc);
		rc = parse_efi_signature_list_hashs(mokx, mokxsize);
		if (rc)
			pr_err("Couldn't parse MokListXRT hashes: %d\n", rc);
		kfree(mokx);
	}

	return rc;
}
late_initcall(load_uefi_certs);
