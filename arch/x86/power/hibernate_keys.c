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

/* Forward information and keys from boot kernel to image kernel */
struct forward_info {
	bool            sig_enforce;
	int             sig_verify_ret;
	struct hibernation_keys hibernation_keys;
};

static struct forward_info *forward_buff;

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

unsigned long get_forward_buff_pfn(void)
{
	if (!forward_buff)
		return 0;

	return page_to_pfn(virt_to_page(forward_buff));
}

void fill_forward_info(void *forward_buff_page, int verify_ret)
{
	struct forward_info *info;

	if (!forward_buff_page)
		return;

	memset(forward_buff_page, 0, PAGE_SIZE);
	info = (struct forward_info *)forward_buff_page;
	info->sig_verify_ret = verify_ret;
	info->sig_enforce = sigenforce;

	if (hibernation_keys && !hibernation_keys->hkey_status) {
		info->hibernation_keys = *hibernation_keys;
		memset(hibernation_keys, 0, PAGE_SIZE);
	} else
		pr_info("PM: Fill hibernation keys failed\n");

	pr_info("PM: Filled sign information to forward buffer\n");
}

void restore_sig_forward_info(void)
{
	if (!forward_buff) {
		pr_warn("PM: Did not allocate forward buffer\n");
		return;
	}

	sigenforce = forward_buff->sig_enforce;
	if (sigenforce)
		pr_info("PM: Enforce hibernate signature verifying\n");

	if (forward_buff->sig_verify_ret) {
		pr_warn("PM: Hibernate signature verifying failed: %d\n",
			forward_buff->sig_verify_ret);

		/* taint kernel */
		if (!sigenforce) {
			pr_warn("PM: System restored from unsafe snapshot - "
				"tainting kernel\n");
			add_taint(TAINT_UNSAFE_HIBERNATE, LOCKDEP_STILL_OK);
			pr_info("%s\n", print_tainted());
		}
	} else
		pr_info("PM: Signature verifying pass\n");

	if (hibernation_keys) {
		memset(hibernation_keys, 0, PAGE_SIZE);
		*hibernation_keys = forward_buff->hibernation_keys;
		pr_info("PM: Restored hibernation keys\n");
	}

	/* clean forward information buffer for next round */
	memset(forward_buff, 0, PAGE_SIZE);
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
		forward_buff = (struct forward_info *)get_zeroed_page(GFP_KERNEL);
		if (!forward_buff) {
			pr_err("PM: Allocate forward buffer failed\n");
			ret = -ENOMEM;
		}
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
