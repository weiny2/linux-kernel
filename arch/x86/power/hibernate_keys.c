/* Hibernation keys handler
 *
 * Copyright (C) 2015 Lee, Chun-Yi <jlee@suse.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public Licence
 * as published by the Free Software Foundation; either version
 * 2 of the Licence, or (at your option) any later version.
 */

#include <linux/bootmem.h>
#include <linux/memblock.h>
#include <linux/suspend.h>
#include <asm/suspend.h>

/* physical address of hibernation keys from boot params */
static u64 keys_phys_addr;

/* A page used to keep hibernation keys */
static struct hibernation_keys *hibernation_keys;

void __init parse_hibernation_keys(u64 phys_addr, u32 data_len)
{
	struct setup_data *hibernation_setup_data;

	/* Reserve keys memory, will copy and erase in init_hibernation_keys() */
	keys_phys_addr = phys_addr + sizeof(struct setup_data);
	memblock_reserve(keys_phys_addr, sizeof(struct hibernation_keys));

	/* clear hibernation_data */
	hibernation_setup_data = early_memremap(phys_addr, data_len);
	if (!hibernation_setup_data)
		return;

	memset(hibernation_setup_data, 0, sizeof(struct setup_data));
	early_iounmap(hibernation_setup_data, data_len);
}

int get_hibernation_key(u8 **hkey)
{
	if (!hibernation_keys)
		return -ENODEV;

	if (!hibernation_keys->hkey_status)
		*hkey = hibernation_keys->hibernation_key;

	return hibernation_keys->hkey_status;
}


bool swsusp_page_is_keys(struct page *page)
{
	bool ret = false;

	if (!hibernation_keys || hibernation_keys->hkey_status)
		return ret;

	ret = (page_to_pfn(page) == page_to_pfn(virt_to_page(hibernation_keys)));
	if (ret)
		pr_info("PM: Avoid snapshot the page of hibernation key.\n");

	return ret;
}

static int __init init_hibernation_keys(void)
{
	struct hibernation_keys *keys;
	int ret = 0;

	if (!keys_phys_addr)
		return -ENODEV;

	keys = early_memremap(keys_phys_addr, sizeof(struct hibernation_keys));

	/* Copy hibernation keys to a allocated page */
	hibernation_keys = (struct hibernation_keys *)get_zeroed_page(GFP_KERNEL);
	if (hibernation_keys) {
		*hibernation_keys = *keys;
	} else {
		pr_err("PM: Allocate hibernation keys page failed\n");
		ret = -ENOMEM;
	}

	/* Erase keys data no matter copy success or failed */
	memset(keys, 0, sizeof(struct hibernation_keys));
	early_iounmap(keys, sizeof(struct hibernation_keys));
	memblock_free(keys_phys_addr, sizeof(struct hibernation_keys));
	keys_phys_addr = 0;

	return ret;
}

late_initcall(init_hibernation_keys);
