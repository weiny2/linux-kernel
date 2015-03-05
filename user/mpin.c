/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2014 - 2015 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * BSD LICENSE
 *
 * Copyright (c) 2014 - 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Intel(R) Omni-Path User RDMA Driver
 */

#include <linux/pagemap.h>
#include "opa_user.h"

struct hfi_mpin_entry {
	u64 pfn;
	struct list_head list;
};

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
