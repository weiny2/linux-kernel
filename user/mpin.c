/*
 * Copyright (c) 2014 Intel Corporation.  All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <linux/pagemap.h>
#include "../common/hfi.h"

/*
 * This page pinning interface is likely unneeded when we have an IOMMU.
 * But perhaps even then we will still want ability for user to pin 
 * certain memory regions to ensure they stay resident.
 */
int hfi_mpin(struct hfi_userdata *ud, struct hfi_mpin_args *mpin)
{
	struct page *pages;
	struct hfi_mpin_entry *mpin_entry;
	int nr_pages, ret;
	unsigned long flags;

	/* limit to single page */
	if (mpin->base & ~PAGE_MASK || mpin->len != PAGE_SIZE)
		return -EINVAL;
	nr_pages = 1;

	mpin_entry = kmalloc(sizeof(*mpin_entry), GFP_KERNEL);
	if (mpin_entry == NULL)
		return -ENOMEM;

	/* pin a single page */
	ret = get_user_pages_fast(mpin->base, nr_pages, 1, &pages);
	if (ret < 0) {
		kfree(mpin_entry);
	} else {
		BUG_ON(ret != nr_pages);
		ret = 0;

		/* TODO - delete exposing phys_addrs when FXR has IOMMU */
		mpin->phys_addr = page_to_phys(&pages[0]);

		/* queue page for removal at process exit */
		mpin_entry->pfn = page_to_pfn(&pages[0]);
		spin_lock_irqsave(&ud->mpin_lock, flags);
		list_add(&mpin_entry->list, &ud->mpin_head);
		spin_unlock_irqrestore(&ud->mpin_lock, flags);
	}

	return ret;
}

static void __hfi_mpin_free(struct hfi_mpin_entry *mpin_entry)
{
	list_del(&mpin_entry->list);
	put_page(pfn_to_page(mpin_entry->pfn));
	kfree(mpin_entry);
}

int hfi_munpin(struct hfi_userdata *ud, struct hfi_munpin_args *munpin)
{
	struct hfi_mpin_entry *mpin_entry;
	unsigned long flags;

	spin_lock_irqsave(&ud->mpin_lock, flags);
	list_for_each_entry(mpin_entry, &ud->mpin_head, list) {
		if (mpin_entry->pfn == (munpin->phys_addr >> PAGE_SHIFT)) {
			__hfi_mpin_free(mpin_entry);
			break;
		}
	}
	spin_unlock_irqrestore(&ud->mpin_lock, flags);

	return 0;
}

int hfi_munpin_all(struct hfi_userdata *ud)
{
	struct hfi_mpin_entry *mpin_entry, *next;
	unsigned long flags;

	spin_lock_irqsave(&ud->mpin_lock, flags);
	list_for_each_entry_safe(mpin_entry, next, &ud->mpin_head, list) {
		__hfi_mpin_free(mpin_entry);
	}
	spin_unlock_irqrestore(&ud->mpin_lock, flags);

	return 0;
}
