/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2017-2018 Intel Corporation.
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
 * Copyright (c) 2017-2018 Intel Corporation.
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
 */

/*
 * Intel(R) OPA Gen2 IB Driver
 */
#include "verbs.h"

/* TODO - turn off when RX unit is implemented in simulation */
static uint psb_rw = 1;
module_param(psb_rw, uint, 0444);
MODULE_PARM_DESC(psb_rw, "PSB mmaps allow RW");

void hfi_zap_vma_list(struct hfi_ibcontext *context, uint16_t cmdq_idx)
{
	struct list_head *pos, *temp;
	struct hfi_vma *v;
	struct vm_area_struct *vv;

	mutex_lock(&context->vm_lock);
	list_for_each_safe(pos, temp, &context->vma_head) {
		v = list_entry(pos, struct hfi_vma, vma_list);
		if (cmdq_idx == v->cmdq_idx) {
			vv = v->vma;
			zap_vma_ptes(vv, vv->vm_start,
				     vv->vm_end - vv->vm_start);
			list_del(pos);
			kfree(v);
		}
	}
	mutex_unlock(&context->vm_lock);
}

static int hfi_vma_list_add(struct hfi_ibcontext *context,
			     struct vm_area_struct *vma,
			     uint16_t cmdq_idx)
{
	struct hfi_vma *v;

	v = kmalloc(sizeof(*v), GFP_KERNEL);

	if (!v)
		return -ENOMEM;

	mutex_lock(&context->vm_lock);
	list_add(&v->vma_list, &context->vma_head);
	v->vma = vma;
	v->cmdq_idx = cmdq_idx;
	vma->vm_private_data = context;
	mutex_unlock(&context->vm_lock);
	return 0;
}

static void hfi_vma_list_remove(struct hfi_ibcontext *context,
				struct vm_area_struct *vma,
				uint16_t cmdq_idx)
{
	struct list_head *pos, *temp;
	struct hfi_vma *v;
	struct vm_area_struct *vv;

	mutex_lock(&context->vm_lock);
	list_for_each_safe(pos, temp, &context->vma_head) {
		v = list_entry(pos, struct hfi_vma, vma_list);
		vv = v->vma;
		if (vv == vma && cmdq_idx == v->cmdq_idx) {
			list_del(pos);
			kfree(v);
			vma->vm_private_data = NULL;
			break;
		}
	}
	mutex_unlock(&context->vm_lock);
}

/*
 * Convert kernel *virtual* addresses to physical addresses.
 * This handles vmalloc'ed or kmalloc'ed addresses.
 */
static u64 kvirt_to_phys(void *addr, int *high)
{
	struct page *page;
	u64 paddr = 0;

	if (!is_vmalloc_addr(addr)) {
		*high = 0;
		return virt_to_phys(addr);
	}

	*high = 1;
	page = vmalloc_to_page(addr);
	if (page)
		paddr = page_to_pfn(page) << PAGE_SHIFT;

	return paddr;
}

static inline int is_valid_mmap(u64 token)
{
	return (HFI_MMAP_TOKEN_GET(MAGIC, token) == HFI_MMAP_MAGIC);
}

int hfi2_mmap(struct ib_ucontext *context, struct vm_area_struct *vma)
{
	struct hfi_devdata *dd;
	struct hfi_ctx *ctx;
	unsigned long flags, pfn;
	void *remap_addr, *kvaddr = NULL;
	u64 token = vma->vm_pgoff << PAGE_SHIFT;
	u64 phys_addr = 0;
	int high = 0, mapio = 0, vm_ro = 0, type;
	ssize_t memlen = 0;
	int ret = 0;
	u16 ctxt;
	bool add_to_vma_list = false;
	struct hfi_ibcontext *hfi_context = container_of(context,
							 struct hfi_ibcontext,
							 ibuc);
	if (!context || !hfi_context)
		return -EINVAL;
	dd = hfi_dd_from_ibdev(context->device);

	if (!is_valid_mmap(token))
		return -EACCES;

	if (!(vma->vm_flags & VM_SHARED)) {
		ret = -EINVAL;
		goto done;
	}

	/*
	 * ctxt variable should equal the pid of the hfi_ctxt
	 *
	 * IDR lookup to get the index of command queue object, or the pid for
	 * the hfi context
	 *
	 * TODO: Rework the tokens: User can put any CTXT value they
	 * want in the token and below can lookup someone else's
	 * hfi_ctx. We will need the CTXT field in the token to be an
	 * index into the big uobject IDR, as this is all the caller
	 * should have access to.
	 */
	ctxt = HFI_MMAP_TOKEN_GET(CTXT, token);
	type = HFI_MMAP_TOKEN_GET(TYPE, token);
	flags = vma->vm_flags;

	switch (type) {
	case TOK_CMDQ_TX:
	case TOK_CMDQ_RX:
	case TOK_CMDQ_HEAD:
		spin_lock(&dd->cmdq_lock);
		ctx = idr_find(&dd->cmdq_pair, ctxt);
		spin_unlock(&dd->cmdq_lock);
		break;
	case TOK_EVENTS_CT:
	case TOK_EVENTS_EQ_DESC:
	case TOK_PORTALS_TABLE:
	case TOK_TRIG_OP:
	case TOK_LE_ME:
	case TOK_UNEXPECTED:
	case TOK_EVENTS_EQ_HEAD:
		spin_lock(&dd->ptl_lock);
		ctx = idr_find(&dd->ptl_user, ctxt);
		spin_unlock(&dd->ptl_lock);
		break;
	default:
		ctx = NULL;
		break;
	}

	/* validate we have an assigned Portals PID */
	if (!ctx || ctx->pid == HFI_PID_NONE) {
		ret = -EINVAL;
		goto done;
	}

	/* these mmaps don't need to litter the MM of forked children */
	flags |= VM_DONTCOPY;

	/* handle errors before we attempt the mapping */
	switch (type) {
	case TOK_CMDQ_TX:
	case TOK_CMDQ_RX:
		mapio = 1;
	case TOK_CMDQ_HEAD:
		/* Save vma for later zapping */
		add_to_vma_list = true;
		ret = hfi_vma_list_add(hfi_context, vma, ctxt);
		if (ret)
			return ret;
		if (type == TOK_CMDQ_HEAD)
			vm_ro = 1;
		break;
	case TOK_EVENTS_CT:
	case TOK_EVENTS_EQ_DESC:
	case TOK_PORTALS_TABLE:
	case TOK_TRIG_OP:
	case TOK_LE_ME:
	case TOK_UNEXPECTED:
		if (!psb_rw)
			vm_ro = 1;
		break;
	case TOK_EVENTS_EQ_HEAD:
	default:
		break;
	}

	if (vm_ro) {
		/* enforce no write access to RO mappings */
		if (flags & VM_WRITE) {
			ret = -EPERM;
			goto done;
		}
		flags &= ~VM_MAYWRITE;
	}

	/*
	 * Get address and length from HW driver.
	 * For CMDQ tokens, ownership tested here.
	 */
	ret = hfi_ctx_hw_addr(ctx, type, ctxt, &remap_addr, &memlen);
	if (ret)
		goto done;

	if ((vma->vm_end - vma->vm_start) != memlen) {
		ret = -EINVAL;
		goto done;
	}

	if (mapio) {
		phys_addr = (u64)remap_addr;
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	} else {
		/* align remap_addr as may be offset into PAGE */
		kvaddr = (void *)((u64)remap_addr & PAGE_MASK);
		phys_addr = kvirt_to_phys(kvaddr, &high);
	}

	vma->vm_flags = flags;

	pr_debug("%s: %u type:%u io/h:%d/%d, addr:0x%llx, len:%lu(%lu), flags:0x%lx\n",
		 __func__, ctxt, type, mapio, high,
		 phys_addr, memlen, vma->vm_end - vma->vm_start,
		 vma->vm_flags);

	pfn = (unsigned long)(phys_addr >> PAGE_SHIFT);
	if (mapio) {
		ret = io_remap_pfn_range(vma, vma->vm_start, pfn, memlen,
					 vma->vm_page_prot);
	} else if (high) {
		ret = remap_vmalloc_range_partial(vma, vma->vm_start,
						  kvaddr, memlen);
	} else {
		ret = remap_pfn_range(vma, vma->vm_start, pfn, memlen,
				      vma->vm_page_prot);
	}

	/* Something failed above -- remove vma from zap list */
	if (ret && add_to_vma_list)
		hfi_vma_list_remove(hfi_context, vma, ctxt);
done:
	return ret;
}
