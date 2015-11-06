/* EFI variable handler of hibernation key regen flag
 *
 * Copyright (C) 2015 Lee, Chun-Yi <jlee@suse.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public Licence
 * as published by the Free Software Foundation; either version
 * 2 of the Licence, or (at your option) any later version.
 */

#include <linux/efi.h>
#include <linux/slab.h>
#include <linux/suspend.h>

void create_hibernation_key_regen_flag(void)
{
	struct efivar_entry *entry = NULL;
	int err = 0;

	if (!efi_enabled(EFI_RUNTIME_SERVICES))
		return;

	if (!set_hibernation_key_regen_flag)
		return;

	entry = kzalloc(sizeof(*entry), GFP_KERNEL);
	if (!entry)
		return;

	memcpy(entry->var.VariableName,
		HIBERNATION_KEY_REGEN_FLAG, sizeof(HIBERNATION_KEY_REGEN_FLAG));
	memcpy(&(entry->var.VendorGuid),
		&EFI_HIBERNATION_GUID, sizeof(efi_guid_t));

	err = efivar_entry_set(entry, HIBERNATION_KEY_SEED_ATTRIBUTE,
				sizeof(bool), &set_hibernation_key_regen_flag, NULL);
	if (err)
		pr_warn("PM: Set flag of regenerating hibernation key failed: %d\n", err);

	kfree(entry);
}
EXPORT_SYMBOL_GPL(create_hibernation_key_regen_flag);
