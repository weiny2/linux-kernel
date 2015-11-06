/******************************************************************************
 * gnttab.h
 * 
 * Two sets of functionality:
 * 1. Granting foreign access to our memory reservation.
 * 2. Accessing others' memory reservations via grant references.
 * (i.e., mechanisms for both sender and recipient of grant references)
 * 
 * Copyright (c) 2004-2005, K A Fraser
 * Copyright (c) 2005, Christopher Clark
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation; or, when distributed
 * separately from the Linux kernel or incorporated into other
 * software packages, subject to the following license:
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this source file (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef __ASM_GNTTAB_H__
#define __ASM_GNTTAB_H__

#include <asm/hypervisor.h>
#include <asm/maddr.h> /* maddr_t */
#include <linux/mm.h>
#include <linux/delay.h>
#include <xen/interface/grant_table.h>
#include <xen/features.h>

#define GRANT_INVALID_REF	0

struct gnttab_free_callback {
	struct gnttab_free_callback *next;
	void (*fn)(void *);
	void *arg;
	u16 count;
	u8 queued;
};

int gnttab_grant_foreign_access(domid_t domid, unsigned long frame,
				int flags);

/*
 * End access through the given grant reference, iff the grant entry is no
 * longer in use.  Return 1 if the grant entry was freed, 0 if it is still in
 * use.
 */
int gnttab_end_foreign_access_ref(grant_ref_t ref);

/*
 * Eventually end access through the given grant reference, and once that
 * access has been ended, free the given page too.  Access will be ended
 * immediately iff the grant entry is not in use, otherwise it will happen
 * some time later.  page may be 0, in which case no freeing will occur.
 */
void gnttab_end_foreign_access(grant_ref_t ref, unsigned long page);
void gnttab_multi_end_foreign_access(unsigned int nr, grant_ref_t [],
				     struct page *[]);

int gnttab_grant_foreign_transfer(domid_t domid, unsigned long pfn);

unsigned long gnttab_end_foreign_transfer_ref(grant_ref_t ref);
unsigned long gnttab_end_foreign_transfer(grant_ref_t ref);

int gnttab_query_foreign_access(grant_ref_t ref);

/*
 * operations on reserved batches of grant references
 */
int gnttab_alloc_grant_references(u16 count, grant_ref_t *pprivate_head);

void gnttab_free_grant_reference(grant_ref_t ref);

void gnttab_free_grant_references(grant_ref_t head);

int gnttab_empty_grant_references(const grant_ref_t *pprivate_head);

int gnttab_claim_grant_reference(grant_ref_t *pprivate_head);

void gnttab_release_grant_reference(grant_ref_t *private_head,
				    grant_ref_t release);

void gnttab_request_free_callback(struct gnttab_free_callback *callback,
				  void (*fn)(void *), void *arg, u16 count);
void gnttab_cancel_free_callback(struct gnttab_free_callback *callback);

void gnttab_grant_foreign_access_ref(grant_ref_t ref, domid_t domid,
				     unsigned long frame, int flags);

void gnttab_grant_foreign_transfer_ref(grant_ref_t, domid_t domid,
				       unsigned long pfn);

int gnttab_copy_grant_page(grant_ref_t ref, struct page **pagep);
#if IS_ENABLED(CONFIG_XEN_BACKEND)
void __gnttab_dma_map_page(struct page *page);
#else
#define __gnttab_dma_map_page __gnttab_dma_unmap_page
#endif
static inline void __gnttab_dma_unmap_page(struct page *page)
{
}

void gnttab_reset_grant_page(struct page *page);

#ifndef CONFIG_XEN
int gnttab_resume(void);
#endif

void *arch_gnttab_alloc_shared(xen_pfn_t *frames);

static inline void
gnttab_set_map_op(struct gnttab_map_grant_ref *map, maddr_t addr,
		  uint32_t flags, grant_ref_t ref, domid_t domid)
{
	if (flags & GNTMAP_contains_pte)
		map->host_addr = addr;
	else if (xen_feature(XENFEAT_auto_translated_physmap))
		map->host_addr = __pa(addr);
	else
		map->host_addr = addr;

	map->flags = flags;
	map->ref = ref;
	map->dom = domid;
}

static inline void
gnttab_set_unmap_op(struct gnttab_unmap_grant_ref *unmap, maddr_t addr,
		    uint32_t flags, grant_handle_t handle)
{
	if (flags & GNTMAP_contains_pte)
		unmap->host_addr = addr;
	else if (xen_feature(XENFEAT_auto_translated_physmap))
		unmap->host_addr = __pa(addr);
	else
		unmap->host_addr = addr;

	unmap->handle = handle;
	unmap->dev_bus_addr = 0;
}

static inline void
gnttab_set_replace_op(struct gnttab_unmap_and_replace *unmap, maddr_t addr,
		      maddr_t new_addr, grant_handle_t handle)
{
	if (xen_feature(XENFEAT_auto_translated_physmap)) {
		unmap->host_addr = __pa(addr);
		unmap->new_addr = __pa(new_addr);
	} else {
		unmap->host_addr = addr;
		unmap->new_addr = new_addr;
	}

	unmap->handle = handle;
}

#define gnttab_check_GNTST_eagain_while(__HCop, __HCarg_p)			\
{										\
	u8 __hc_delay = 1;							\
	int __ret;								\
	while (unlikely((__HCarg_p)->status == GNTST_eagain && __hc_delay)) {	\
		msleep(__hc_delay++);						\
		__ret = HYPERVISOR_grant_table_op(__HCop, (__HCarg_p), 1);	\
		BUG_ON(__ret);							\
	}									\
	if (__hc_delay == 0) {							\
		pr_err("%s: %s gnt busy\n", __func__, current->comm);		\
		(__HCarg_p)->status = GNTST_bad_page;				\
	}									\
	if ((__HCarg_p)->status != GNTST_okay)					\
		pr_err("%s: %s gnt status %x\n",				\
			__func__, current->comm, (__HCarg_p)->status);		\
}

#define gnttab_check_GNTST_eagain_do_while(__HCop, __HCarg_p)			\
{										\
	u8 __hc_delay = 1;							\
	int __ret;								\
	do {									\
		__ret = HYPERVISOR_grant_table_op(__HCop, (__HCarg_p), 1);	\
		BUG_ON(__ret);							\
		if ((__HCarg_p)->status == GNTST_eagain)			\
			msleep(__hc_delay++);					\
	} while ((__HCarg_p)->status == GNTST_eagain && __hc_delay);		\
	if (__hc_delay == 0) {							\
		pr_err("%s: %s gnt busy\n", __func__, current->comm);		\
		(__HCarg_p)->status = GNTST_bad_page;				\
	}									\
	if ((__HCarg_p)->status != GNTST_okay)					\
		pr_err("%s: %s gnt status %x\n",				\
			__func__, current->comm, (__HCarg_p)->status);		\
}

#endif /* __ASM_GNTTAB_H__ */
