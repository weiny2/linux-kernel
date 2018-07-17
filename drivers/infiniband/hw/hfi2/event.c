/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2015 Intel Corporation.
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
 * Copyright (c) 2015 Intel Corporation.
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
 * Intel(R) Omni-Path Gen2 HFI PCIe Driver
 */

#include <linux/log2.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>
#include <linux/sched/signal.h>
#include <rdma/ib_verbs.h>
#include "hfi2.h"
#include "hfi_kclient.h"
#include "trace.h"

/*
 * Event Channel Operation Mode
 */
#define HFI_EC_MODE_EQ		0x01
#define HFI_EC_MODE_CT		0x02
#define HFI_EC_MODE_EQ_SELF	0x04
#define HFI_EC_MODE_CT_SELF	0x08
#define HFI_EC_MODE_IB		0x10

/*
 * Kernel Event Channel (EC) entry.
 * @wq - queue head for threads waiting on this ec
 * @ec_wait_head - eq list assoiciated with this ec
 * @mode - HFI_EC_MODE_EQ or HFI_EC_MODE_CT
 */
struct hfi_ec_entry {
	wait_queue_head_t wq;
	struct list_head ec_wait_head;
	u16 mode;
};

/*
 * Kernel Event Queue and Counting Event management object.
 *
 * This structure is used for both EQ and CT. The first
 * three fields must be the same as ec above, so that
 * we can typecast eq to ec as self event channel.
 *
 * @wq - queue head for threads waiting on this eq
 * @ec_wait_chain - link this eq to ec linklist
 * @mode - HFI_EC_MODE_EQ_SELF or HFI_EC_MODE_CT_SELF
 * @ec - event channel this eq associated with
 * @irq_wait_chain - link this eq to irq linklist,
 * @irq_vector - irq vector this eq uses
 * @num_unacked - number of unacked eq events
 * @refcount - reference counters
 * @isr_cb - kernel clients callback function
 * @cookie - kernel clients callback argument
 * @hfi_eq_desc - saved eq address and count/index
 * @user_handle - saved user space opaque handle data
 * @ibcq - associated IB CQ (handler)
 */
struct hfi_eq_mgmt {
	wait_queue_head_t wq;
	struct list_head ec_wait_chain;
	u16 mode;

	struct hfi_ec_entry *ec;
	struct list_head irq_wait_chain;
	u32 irq_vector;
	int num_unacked;
	struct kref refcount;

	void (*isr_cb)(struct hfi_eq *eq, void *cookie);
	void *cookie;

	struct hfi_eq desc;
	u64 user_handle;
	struct ib_cq *ibcq;
};

/**
 * struct hfi_eq_ctx - page pinning information for an EQ buffer
 * @ctx: hardware context associated with
 * @eqm: pointer to the management eq when the eq is in block mode
 * @npages: number of pages pinned for this eq
 * @pages: array of struct page pointers used to pin the eq buffer
 * @vaddr: user space page aligned virtual address of the EQ buffer
 * @ibeq: pointer to user space hfi_ibeq structure allocated for native verbs
 */
struct hfi_eq_ctx {
	struct hfi_ctx *ctx;
	struct hfi_eq_mgmt *eqm;
	int npages;
	struct page **pages;
	unsigned long vaddr;
	struct hfi_ibeq *ibeq;
};

static int hfi_eq_setup(struct hfi_ctx *ctx,
			u16 eq_idx, struct hfi_eq_mgmt **peqm);
static int hfi_eq_arm(struct hfi_ctx *ctx,
		      struct hfi_eq_mgmt *eqm, u64 user_data, u8 solicit);
static int hfi_eq_disarm(struct hfi_ctx *ctx,
			 struct hfi_eq_mgmt *eqm, u64 user_data, bool wait);
static int hfi_ec_remove(int ec_idx, void *idr_ptr, void *idr_ctx);
static void hfi_eq_free_ctx(struct hfi_eq_ctx *eq_ctx);
static void hfi_eq_unlink(struct hfi_ctx *ctx, struct hfi_eq_mgmt *eqm);

static
int _hfi_eq_update_intr(struct hfi_ctx *ctx, struct hfi_cmdq *rx_cq,
			u16 eq_idx, int irq, u8 solicit, u64 user_ptr,
			union hfi_rx_cq_command *cmd)
{
	u64 payload0, payload1, mask0, mask1;
	int cmd_slots;

	/*
	 * Update the EQD in the FXR EQD cache by issuing a command to the
	 * RX CMDQ for the FW to execute
	 *
	 * The HAS States:
	 *  [TargetStruct] <- ([TargetStruct] & ~[Mask]) | ([Payload] & [Mask])
	 */
	if (irq < 0) {
		payload0 = 0;
		mask0 = EQD_I_MASK;
		payload1 = 0;
		mask1 = EQD_S_MASK;
	} else {
		payload0 = EQD_I_MASK | (((u64)irq & 0xFF) << EQD_IRQ_LSB);
		mask0 = EQD_I_MASK | EQD_IRQ_MASK;
		if (solicit)
			payload1 = EQD_S_MASK;
		else
			payload1 = 0;
		mask1 = EQD_S_MASK;
	}
	cmd_slots = hfi_format_rx_update64(ctx,
					   eq_idx >> RX_D5_CT_HANDLE_WIDTH,
						/* extract NI from the handle */
					   HFI_PT_ANY,
						/* not needed */
					   EQ_DESC_UPDATE,
					   eq_idx,
						/* truncated to 11-bits */
					   payload0,
						/* payload0 */
					   payload1,
						/* payload1 */
					   mask0,
						/* mask0 */
					   mask1,
						/* mask1 */
					   user_ptr,
						/* user_ptr */
					   HFI_GEN_CC,
					   cmd);

	return cmd_slots;
}

static
int _hfi_eq_update_desc(struct hfi_ctx *ctx, u16 eq_idx,
			union eqd *eq_desc, u64 user_ptr,
			u8 ncc,
			union hfi_rx_cq_command *cmd)
{
	int cmd_slots;

	/* Update the EQD in the FXR EQD cache by issuing a command to the
	 * RX CMDQ for the FW to execute
	 *
	 * The HAS States:
	 *  [TargetStruct] <- ([TargetStruct] & ~[Mask]) | ([Payload] & [Mask])
	 */
	cmd_slots = hfi_format_rx_update64(ctx,
					   eq_idx >> RX_D5_CT_HANDLE_WIDTH,
						/* extract NI from the handle */
					   HFI_PT_ANY,
						/* not needed */
					   EQ_DESC_UPDATE,
					   eq_idx,
						/* truncated to 11-bits */
					   ((u64 *)eq_desc)[0],
						/* payload0 */
					   ((u64 *)eq_desc)[1],
						/* payload1 */
					   0xffffffffffffffffULL,
						/* mask0 */
					   0xffffffffffffffffULL,
						/* mask1 */
					   user_ptr,
						/* user_ptr */
					   ncc,
					   cmd);

	return cmd_slots;
}

static void hfi_eq_mgmt_free(struct kref *ref)
{
	struct hfi_eq_mgmt *eqm;

	eqm = container_of(ref, struct hfi_eq_mgmt, refcount);
	kfree(eqm);
}

static int hfi_ct_assign(struct hfi_ctx *ctx, struct opa_ev_assign *ev_assign)
{
	union ptl_ct_event *ct_desc_base;
	union ptl_ct_event ct_desc = {.val = {0} };
	u16 ct_base;
	int ct_idx;
	int num_cts = HFI_NUM_CT_ENTRIES;
	int ret = 0;

	ct_base = ev_assign->ni * num_cts;
	if (ev_assign->ni >= HFI_NUM_NIS)
		return -EINVAL;

	down_read(&ctx->ctx_rwsem);

	/* verify we have an assigned context */
	if (ctx->pid == HFI_PID_NONE) {
		ret = -EPERM;
		goto idr_end;
	}

	mutex_lock(&ctx->event_mutex);
	ct_idx = idr_alloc(&ctx->ct_used, ctx, ct_base, ct_base + num_cts,
			   GFP_KERNEL);
	mutex_unlock(&ctx->event_mutex);
	if (ct_idx < 0) {
		/* all EQs are assigned */
		ret = ct_idx;
		goto idr_end;
	}

	/*
	 * set CT descriptor in host memory;
	 * this memory is cache coherent with HFI, does not use RX CMD
	 */
	ct_desc_base = (void *)(ctx->ptl_state_base +
				HFI_PSB_CT_OFFSET);

	ct_desc.success = ev_assign->base;
	ct_desc.failure = ev_assign->user_data;
	ct_desc.ni = ev_assign->ni;
	ct_desc.irq = 0;
	ct_desc.i = 0;
	ct_desc.v = 1;
	ct_desc_base[ct_idx].val[0] = ct_desc.val[0];
	ct_desc_base[ct_idx].val[1] = ct_desc.val[1];
	ct_desc_base[ct_idx].val[2] = ct_desc.val[2];
	wmb();  /* barrier before writing Valid */
	ct_desc_base[ct_idx].val[3] = ct_desc.val[3];

	 /* return index to user */
	ev_assign->ev_idx = ct_idx;

idr_end:
	up_read(&ctx->ctx_rwsem);

	return ret;
}

static int hfi_ct_release(struct hfi_ctx *ctx, u16 ct_idx)
{
	union ptl_ct_event *ct_desc_base;
	struct hfi_eq_mgmt *eqm;
	void *ct_present;
	int ret = 0;

	down_read(&ctx->ctx_rwsem);
	mutex_lock(&ctx->event_mutex);

	ct_present = idr_find(&ctx->ct_used, ct_idx);
	if (!ct_present) {
		ret = -EINVAL;
		goto idr_end;
	}

	if (ct_present != ctx) {
		eqm = (struct hfi_eq_mgmt *)ct_present;
		if (eqm->num_unacked > 0) {
			ret = -EBUSY;
			goto idr_end;
		}

		/* remove ct from internal list */
		hfi_eq_unlink(ctx, eqm);
		/* free ct management object */
		kref_put(&eqm->refcount, hfi_eq_mgmt_free);
	}

	/* clear/invalidate CT */
	ct_desc_base = (void *)(ctx->ptl_state_base +
				HFI_PSB_CT_OFFSET);
	ct_desc_base[ct_idx].val[3] = 0;

	idr_remove(&ctx->ct_used, ct_idx);

idr_end:
	mutex_unlock(&ctx->event_mutex);
	up_read(&ctx->ctx_rwsem);

	return ret;
}

static int hfi_ct_arm(struct hfi_ctx *ctx, struct hfi_eq_mgmt *eqm)
{
	struct hfi_devdata *dd = ctx->devdata;
	union ptl_ct_event *ct_desc_base = (void *)(ctx->ptl_state_base
						    + HFI_PSB_CT_OFFSET);
	struct hfi_irq_entry *me;
	unsigned long flags;
	int irq_idx;

	/* for now just do round-robin assignment */
	irq_idx = atomic_inc_return(&dd->irq_eq_next) %
			dd->num_eq_irqs;
	eqm->irq_vector = irq_idx;
	ct_desc_base[eqm->desc.idx].irq = irq_idx;
	ct_desc_base[eqm->desc.idx].i = 1;
	wmb(); /* barrier before updating irq list */

	/* add EQ to list of IRQ waiters */
	me = &dd->irq_entries[eqm->irq_vector];
	spin_lock_irqsave(&me->irq_wait_lock, flags);
	list_add(&eqm->irq_wait_chain, &me->irq_wait_head);
	spin_unlock_irqrestore(&me->irq_wait_lock, flags);

	return 0;
}

static int hfi_ct_disarm(struct hfi_ctx *ctx, struct hfi_eq_mgmt *eqm)
{
	struct hfi_devdata *dd = ctx->devdata;
	union ptl_ct_event *ct_desc_base = (void *)(ctx->ptl_state_base
						    + HFI_PSB_CT_OFFSET);
	struct hfi_irq_entry *me;
	unsigned long flags;

	/* turn off interrupt on this ct */
	ct_desc_base[eqm->desc.idx].irq = 0;
	ct_desc_base[eqm->desc.idx].i = 0;
	wmb(); /* barrier before updating irq list */

	/* remove CT from list of IRQ waiters */
	me = &dd->irq_entries[eqm->irq_vector];
	spin_lock_irqsave(&me->irq_wait_lock, flags);
	list_del_init(&eqm->irq_wait_chain);
	spin_unlock_irqrestore(&me->irq_wait_lock, flags);

	return 0;
}

/*
 * This function is called via event cleanup as a idr_for_each function, hence
 * the curious looking arguments.
 */
static int hfi_ct_remove(int eq_idx, void *idr_ptr, void *idr_ctx)
{
	/* Test if blocking mode CT */
	if (idr_ptr != idr_ctx) {
		hfi_eq_unlink(idr_ctx, idr_ptr);
		kfree(idr_ptr);
	}

	return 0;
}

int hfi_ct_ack_event(struct hfi_ctx *ctx, u16 ct_idx, u32 nevents)
{
	struct hfi_eq_mgmt *eqm;
	int ret = 0;

	down_read(&ctx->ctx_rwsem);
	mutex_lock(&ctx->event_mutex);

	eqm = idr_find(&ctx->ct_used, ct_idx);
	if (!eqm || ctx == (struct hfi_ctx *)eqm)
		ret = -EINVAL;
	else
		eqm->num_unacked -= nevents;

	mutex_unlock(&ctx->event_mutex);
	up_read(&ctx->ctx_rwsem);

	return ret;
}

static int hfi_ct_setup(struct hfi_ctx *ctx,
			u16 ct_idx, struct hfi_eq_mgmt **peqm)
{
	struct hfi_eq_mgmt *eqm;

	eqm = idr_find(&ctx->ct_used, ct_idx);
	if (!eqm)
		return -EINVAL;

	if (ctx == (struct hfi_ctx *)eqm) {
		eqm = kzalloc(sizeof(*eqm), GFP_KERNEL);
		if (!eqm)
			return -ENOMEM;

		/* initialize CT state */
		INIT_LIST_HEAD(&eqm->irq_wait_chain);
		INIT_LIST_HEAD(&eqm->ec_wait_chain);
		init_waitqueue_head(&eqm->wq);
		kref_init(&eqm->refcount);
		eqm->mode = HFI_EC_MODE_CT_SELF;

		eqm->desc.idx = ct_idx;

		/* replace idr pointer */
		idr_replace(&ctx->ct_used, eqm, ct_idx);
	}

	/* return ct to caller */
	*peqm = eqm;

	return 0;
}

static int hfi_eq_can_pin_pages(u32 npages)
{
	unsigned long ulimit = rlimit(RLIMIT_MEMLOCK), pinned;
	bool can_lock = capable(CAP_IPC_LOCK);

	if (!current || !current->mm)
		return -ESRCH; /* process exited */

	if (!npages)
		return -EINVAL;

	/*
	 * Always allow pages to be pinned if ulimit is set to "unlimited"
	 * or if this process is privileged.
	 */
	if (ulimit == (-1UL) || can_lock)
		return 0;

	down_read(&current->mm->mmap_sem);
	pinned = current->mm->pinned_vm;
	up_read(&current->mm->mmap_sem);

	/* Convert to number of pages */
	ulimit = DIV_ROUND_UP(ulimit, PAGE_SIZE);

	/* Check the absolute limit against all pinned pages */
	if ((pinned + npages) >= ulimit)
		return -EPERM;

	return 0;
}

/*
 * Pin EQ buffer pages.
 *
 * There is a chance that virtual addresses associated with EQ buffers can
 * be swapped during a transaction, causing a page fault over the fabric.
 * To avoid this, pin the EQ buffer pages by calling get_user_pages().
 */
static int hfi_eq_pin_pages(struct hfi_eq_ctx *eq_ctx)
{
	int ret, pinned;

	ret = hfi_eq_can_pin_pages(eq_ctx->npages);
	if (ret < 0)
		return ret;

	pinned = get_user_pages_fast(eq_ctx->vaddr, eq_ctx->npages,
				     true, eq_ctx->pages);
	if (pinned < 0)
		return -ENOMEM;

	if (pinned != eq_ctx->npages) {
		size_t  i;

		for (i = 0; i < pinned; i++)
			put_page(eq_ctx->pages[i]);
		return -ENOMEM;
	}

	hfi_at_reg_range(eq_ctx->ctx, (void *)eq_ctx->vaddr,
			 eq_ctx->npages * PAGE_SIZE, eq_ctx->pages, true);

	down_write(&current->mm->mmap_sem);
	current->mm->pinned_vm += pinned;
	up_write(&current->mm->mmap_sem);

	return 0;
}

static void hfi_eq_release_pages(struct hfi_eq_ctx *eq_ctx)
{
	size_t i;

	hfi_at_dereg_range(eq_ctx->ctx, (void *)eq_ctx->vaddr,
			   eq_ctx->npages * PAGE_SIZE);

	for (i = 0; i < eq_ctx->npages; i++)
		put_page(eq_ctx->pages[i]);

	if (current->mm) { /* during close after signal, mm can be NULL */
		down_write(&current->mm->mmap_sem);
		current->mm->pinned_vm -= eq_ctx->npages;
		up_write(&current->mm->mmap_sem);
	}
}

static inline int num_user_pages(const unsigned long addr,
				 const unsigned long len)
{
	const unsigned long spage = addr & PAGE_MASK;
	const unsigned long epage = (addr + len - 1) & PAGE_MASK;

	return 1 + ((epage - spage) >> PAGE_SHIFT);
}

int hfi_eq_assign(struct hfi_ctx *ctx, struct opa_ev_assign *eq_assign)
{
	struct hfi_devdata *dd = ctx->devdata;
	union hfi_rx_cq_command cmd;
	struct hfi_eq_mgmt *eqm = NULL;
	union eqd *eq_desc_base;
	union eqd eq_desc;
	u16 eq_base;
	int order, eq_idx, irq_idx = 0;
	int num_eqs = HFI_NUM_EVENT_HANDLES;
	struct hfi_eq_ctx *eq_ctx;
	int ret = 0, slots;
	u8 ncc = HFI_GEN_CC;

	if (eq_assign->ni >= HFI_NUM_NIS)
		return -EINVAL;
	if (eq_assign->base & (HFI_EQ_ALIGNMENT - 1))
		return -EFAULT;

	/* FXR requires EQ entry count as power of 2 */
	if (is_power_of_2(eq_assign->count))
		order = __ilog2_u64(eq_assign->count);
	else
		return -EINVAL;
	if ((order < HFI_MIN_EVENT_ORDER) ||
	    (order > HFI_MAX_EVENT_ORDER))
		return -EINVAL;

	if (eq_assign->mode & OPA_EV_MODE_BLOCKING) {
		eqm = kzalloc(sizeof(*eqm), GFP_KERNEL);
		if (!eqm)
			return -ENOMEM;
	}

	eq_base = eq_assign->ni * num_eqs;

	eq_ctx = kzalloc(sizeof(*eq_ctx), GFP_KERNEL);
	if (!eq_ctx) {
		kfree(eqm);
		return -ENOMEM;
	}

	eq_ctx->ctx = ctx;
	eq_ctx->eqm = eqm;

	down_read(&ctx->ctx_rwsem);

	/* verify we have an assigned context */
	if (ctx->pid == HFI_PID_NONE) {
		kfree(eqm);
		kfree(eq_ctx);
		up_read(&ctx->ctx_rwsem);
		return -EPERM;
	}

	/*
	 * Only pin user pages. Since EQ0 and E2E are allocated by the driver,
	 * they will not be attached to a process.
	 */
	if (ctx->type == HFI_CTX_TYPE_USER) {
		/* Convert number of EQ entries to pages */
		eq_ctx->npages = num_user_pages(
			eq_assign->base,
			(eq_assign->count <<
				(eq_assign->jumbo ? HFI_EQ_ENTRY_LOG2_JUMBO :
						    HFI_EQ_ENTRY_LOG2)));
		eq_ctx->vaddr = eq_assign->base & PAGE_MASK;

		/* Array of struct page pointers needed for pinning */
		eq_ctx->pages = kcalloc(eq_ctx->npages, sizeof(*eq_ctx->pages),
					GFP_KERNEL);
		if (!eq_ctx->pages) {
			up_read(&ctx->ctx_rwsem);
			kfree(eqm);
			kfree(eq_ctx);
			return -ENOMEM;
		}
		ret = hfi_eq_pin_pages(eq_ctx);
		if (ret < 0) {
			up_read(&ctx->ctx_rwsem);
			kfree(eqm);
			kfree(eq_ctx->pages);
			kfree(eq_ctx);
			return ret;
		}
	}

	mutex_lock(&ctx->event_mutex);
	/* IDR contents differ based on blocking or non-blocking */
	eq_idx = idr_alloc(&ctx->eq_used, (void *)eq_ctx, eq_base,
			   eq_base + num_eqs, GFP_KERNEL);
	if (eq_idx < 0) {
		/* all EQs are assigned */
		ret = eq_idx;
		kfree(eqm);
		hfi_eq_free_ctx(eq_ctx);
		goto idr_end;
	}
	/* set index to return to user */
	eq_assign->ev_idx = eq_idx;
	if (eqm) {
		struct hfi_irq_entry *me;
		u32 *eq_head_array;
		unsigned long flags;

		/* initialize EQ IRQ state */
		INIT_LIST_HEAD(&eqm->irq_wait_chain);
		INIT_LIST_HEAD(&eqm->ec_wait_chain);
		init_waitqueue_head(&eqm->wq);
		kref_init(&eqm->refcount);
		eqm->mode = HFI_EC_MODE_EQ_SELF;

		/* for now just do round-robin assignment */
		irq_idx = atomic_inc_return(&dd->irq_eq_next) %
			   dd->num_eq_irqs;
		me = &dd->irq_entries[irq_idx];
		eqm->irq_vector = irq_idx;
		eqm->isr_cb = eq_assign->isr_cb;
		eqm->cookie = eq_assign->cookie;

		/* record info needed for hfi_eq_peek() */
		eq_head_array = ctx->eq_head_addr;
		hfi_set_eq(ctx, &eqm->desc, eq_assign, &eq_head_array[eq_idx]);

		/* setup self event channel */
		eqm->ec = (struct hfi_ec_entry *)eqm;

		/* add EQ to list of IRQ waiters */
		spin_lock_irqsave(&me->irq_wait_lock, flags);
		list_add(&eqm->irq_wait_chain, &me->irq_wait_head);
		spin_unlock_irqrestore(&me->irq_wait_lock, flags);
	}

	/* set EQ descriptor in host memory */
	eq_desc_base = (void *)(ctx->ptl_state_base +
				HFI_PSB_EQ_DESC_OFFSET);

	eq_desc.val[0] = 0;
	eq_desc.val[1] = 0; /* clear head/tail */
	eq_desc.order = order;
	eq_desc.sz = eq_assign->jumbo;
	eq_desc.start = (eq_assign->base >> PAGE_SHIFT);
	eq_desc.ni = eq_assign->ni;
	/* Disable all interrupts for event queues to workaround 1407227170 */
	if (!dd->emulation) {
		eq_desc.irq = irq_idx;
		eq_desc.i = (eq_assign->mode & OPA_EV_MODE_BLOCKING);
	}
	eq_desc.v = 1;
	eq_desc_base[eq_idx].val[1] = eq_desc.val[1];
	wmb();  /* barrier before writing Valid */
	eq_desc_base[eq_idx].val[0] = eq_desc.val[0];

	/* Do not wait for CC for EQ 0 on all NI's */
	if (!(eq_idx & (HFI_NUM_EVENT_HANDLES - 1)))
		ncc = HFI_NCC;

	/* issue write to privileged CMDQ to complete EQ assignment */
	slots = _hfi_eq_update_desc(ctx, eq_idx, &eq_desc, eq_assign->user_data,
				    ncc, &cmd);

	/* Queue CMDQ write, wait for completion */
	ret = hfi_pend_cmd_queue_wait(&dd->pend_cmdq, &dd->priv_cmdq.rx, NULL,
				      &cmd, slots);
	if (ret) {
		dd_dev_err(dd, "%s: hfi_pend_cmd_queue_wait failed %d\n",
			   __func__, ret);
		eq_desc_base[eq_idx].val[0] = 0; /* clear valid */
		idr_remove(&ctx->eq_used, eq_idx);
		kfree(eqm);
		hfi_eq_free_ctx(eq_ctx);
		goto idr_end;
	}

idr_end:
	mutex_unlock(&ctx->event_mutex);
	up_read(&ctx->ctx_rwsem);

	return ret;
}

int hfi_eq_zero_assign(struct hfi_ctx *ctx)
{
	struct opa_ev_assign eq_assign = {0};
	int ni, ret;
	u32 *eq_head_array, *eq_head_addr;

	for (ni = 0; ni < HFI_NUM_NIS; ni++) {
		eq_assign.ni = ni;
		/* EQ is one page and meets 64B alignment */
		eq_assign.count = 64;
		eq_assign.base = (u64)kzalloc_node(eq_assign.count *
					      HFI_EQ_ENTRY_SIZE,
					      GFP_KERNEL,
					      ctx->devdata->node);
		if (!eq_assign.base)
			return -ENOMEM;

		if ((ctx->pid == HFI_PID_SYSTEM) && !ni)
			/*
			 * system PID EQ 0 needs to handle
			 * E2E connect/destroy events
			 */
			eq_assign.mode = OPA_EV_MODE_BLOCKING;
		else
			eq_assign.mode = 0x0;

		ret = hfi_eq_assign(ctx, &eq_assign);
		if (ret)
			return ret;
		eq_head_array = ctx->eq_head_addr;
		eq_head_addr = &eq_head_array[eq_assign.ev_idx];
		hfi_set_eq(ctx, &ctx->eq_zero[ni], &eq_assign, eq_head_addr);
		/* Reset the EQ SW head */
		*eq_head_addr = 0;
	}

	/* TODO - ensure that EQ0 is created before events are delivered */
	mdelay(2);

	return 0;
}

void hfi_eq_zero_release(struct hfi_ctx *ctx)
{
	int ni;

	for (ni = 0; ni < HFI_NUM_NIS; ni++) {
		if (ctx->eq_zero[ni].base) {
			hfi_eq_release(ctx, ni * HFI_NUM_EVENT_HANDLES, 0);
			kfree(ctx->eq_zero[ni].base);
			ctx->eq_zero[ni].base = NULL;
		}
	}
}

/* call list_del_init() to make reentrant. */
static void hfi_eq_unlink(struct hfi_ctx *ctx, struct hfi_eq_mgmt *eqm)
{
	/* unlink eq from irq list */
	if (!list_empty(&eqm->irq_wait_chain)) {
		unsigned long flags;
		struct hfi_irq_entry *me;

		me = &ctx->devdata->irq_entries[eqm->irq_vector];
		spin_lock_irqsave(&me->irq_wait_lock, flags);
		list_del_init(&eqm->irq_wait_chain);
		spin_unlock_irqrestore(&me->irq_wait_lock, flags);
	}

	/* check ec association */
	if (!eqm->ec)
		return;

	/* unlink eq from ec list, wake up waiting threads */
	if (!list_empty(&eqm->ec_wait_chain)) {
		list_del_init(&eqm->ec_wait_chain);
		if (list_empty(&eqm->ec->ec_wait_head))
			wake_up_interruptible_all(&eqm->ec->wq);
	} else {
		wake_up_interruptible_all(&eqm->ec->wq);
	}
}

static void hfi_eq_free_ctx(struct hfi_eq_ctx *eq_ctx)
{
	/*
	 * Do not free EQ0 or E2E EQ pages since they are allocated
	 * by the driver. EQ management object is freed separately.
	 */
	if (eq_ctx->pages) {
		hfi_eq_release_pages(eq_ctx);
		kfree(eq_ctx->pages);
	}
	kfree(eq_ctx);
}

/*
 * This function is called via event cleanup as a idr_for_each function, hence
 * the curious looking arguments.
 */
static int hfi_eq_remove(int eq_idx, void *idr_ptr, void *idr_ctx)
{
	struct hfi_eq_ctx *eq_ctx = idr_ptr;

	/* Test if blocking mode EQ */
	if (eq_ctx->eqm) {
		hfi_eq_unlink(idr_ctx, eq_ctx->eqm);
		kfree(eq_ctx->eqm);
	}
	hfi_eq_free_ctx(eq_ctx);

	return 0;
}

int hfi_eq_release(struct hfi_ctx *ctx, u16 eq_idx, u64 user_data)
{
	struct hfi_devdata *dd = ctx->devdata;
	union hfi_rx_cq_command cmd;
	union eqd *eq_desc_base;
	union eqd eq_desc;
	struct hfi_eq_ctx *eq_ctx;
	struct hfi_eq_mgmt *eqm;
	int ret, slots;
	u8 ncc = HFI_GEN_CC;

	down_read(&ctx->ctx_rwsem);
	mutex_lock(&ctx->event_mutex);

	eq_ctx = idr_find(&ctx->eq_used, eq_idx);
	if (!eq_ctx) {
		ret = -EINVAL;
		goto idr_end;
	}

	/*
	 * If the eq exists, this is a management object and
	 * eq is in block mode, refer to hfi_eq_assign() for
	 * more info.
	 */
	if (eq_ctx->eqm) {
		eqm = eq_ctx->eqm;
		if (eqm->num_unacked > 0) {
			ret = -EBUSY;
			goto idr_end;
		}

		/* remove eq from internal list */
		hfi_eq_unlink(ctx, eqm);
		/* free eq management object */
		kref_put(&eqm->refcount, hfi_eq_mgmt_free);
	}

	hfi_eq_free_ctx(eq_ctx);
	idr_remove(&ctx->eq_used, eq_idx);

	/* need cleared EQ desc to send via RX CMDQ */
	eq_desc_base = (void *)(ctx->ptl_state_base +
				HFI_PSB_EQ_DESC_OFFSET);
	eq_desc.val[0] = 0;
	eq_desc.val[1] = 0;
	eq_desc_base[eq_idx].val[0] = 0; /* clear valid */

	/* Do not wait for CC for EQ 0 on all NI's */
	if (!(eq_idx & (HFI_NUM_EVENT_HANDLES - 1)))
		ncc = HFI_NCC;

	/* issue write to privileged CMDQ to complete EQ release */
	slots = _hfi_eq_update_desc(ctx, eq_idx, &eq_desc, user_data, ncc,
				    &cmd);

	/* Queue write, wait for completion */
	ret = hfi_pend_cmd_queue_wait(&dd->pend_cmdq, &dd->priv_cmdq.rx, NULL,
				      &cmd, slots);
	if (ret)
		dd_dev_err(dd, "%s: hfi_pend_cmd_queue_wait failed %d\n",
			   __func__, ret);

idr_end:
	mutex_unlock(&ctx->event_mutex);
	up_read(&ctx->ctx_rwsem);

	return ret;
}

/*
 * hfi_eq_peek_nth_u is called in cases where a user space application
 * calls into the kernel. In this case, copy_from_user needs to be used
 * in order to prevent direct access into user space from kernel
 */
static int hfi_eq_peek_u(struct hfi_eq *eq_handle)
{
	u64 *eq_entry;
	u64 tmp;

	eq_entry = _hfi_eq_nth_entry(eq_handle, 0);

	if (!eq_entry)
		return -EINVAL;

	if (copy_from_user(&tmp, (const void __user *)eq_entry,
			   sizeof(u64)))
		return -EFAULT;
	/*
	 * Test the valid bit of the EQ FIFO entry to see if it
	 * has been written by the HW, we don't check dropped
	 * for user space event.
	 */
	if (likely(tmp & TARGET_EQENTRY_V_MASK))
		return 1;

	return HFI_EQ_EMPTY;
}

/* Returns true if there is a valid pending event and false otherwise */
static inline bool __hfi_eq_wait_condition(struct hfi_ctx *ctx,
					   struct hfi_eq *eq)
{
	int ret;

	/* call hfi_eq_peek_u in case of user space applicaiton */
	if (ctx->type != HFI_CTX_TYPE_USER)
		return hfi_eq_wait_condition(eq);

	ret = hfi_eq_peek_u(eq);

	if (ret > 0)
		return true;
	return false;
}

static inline bool __hfi_ct_wait_condition(union ptl_ct_event *ct_desc_base,
					   struct hfi_eq_mgmt *eqm)
{
	return (ct_desc_base[eqm->desc.idx].success >=
		ct_desc_base[eqm->desc.idx].threshold);
}

static bool hfi_has_event(struct hfi_ctx *ctx,
			  struct hfi_eq_mgmt *eqm, int *ret)
{
	bool cond;

	mutex_lock(&ctx->event_mutex);

	if (eqm->ec->mode == HFI_EC_MODE_EQ_SELF) {
		union eqd *eq_desc_base =
			(void *)(ctx->ptl_state_base +
				HFI_PSB_EQ_DESC_OFFSET);
		/* if not valid, eq has been released */
		if (!eq_desc_base[eqm->desc.idx].v) {
			cond = true;
			*ret = -EINTR;
		} else {
			cond = __hfi_eq_wait_condition(ctx, &eqm->desc);
			*ret = 0;
		}
	} else {
		union ptl_ct_event *ct_desc_base =
			(void *)(ctx->ptl_state_base +
				HFI_PSB_CT_OFFSET);
		/* if not valid, ct has been released */
		if (!ct_desc_base[eqm->desc.idx].v) {
			cond = true;
			*ret = -EINTR;
		} else {
			cond = __hfi_ct_wait_condition(ct_desc_base, eqm);
			*ret = 0;
		}
	}

	mutex_unlock(&ctx->event_mutex);

	return cond;
}

/*
 * This function will be called for both EQ and CT, and also
 * for both user space context and kernel client context.
 * When called for kernel client, user_data0 and user_data1
 * are not used and passed as NULL.
 *
 * For user space context for EQ, user_data0 is the pointer
 * to completion address for the first thread to turn on EQ
 * interrupt, user_data1 is the pointer to completion address
 * for the last thread to turn off EQ interrupt.
 *
 * For user space context for CT, user_data0 is not used,
 * user_data1 is the pointer to space to return ct.threshold
 * when ct.success >= ct.threshold.
 */
int hfi_ev_wait_single(struct hfi_ctx *ctx, u16 eqflag,
		       u16 ev_idx, int timeout,
		       u64 *user_data0, u64 *user_data1)
{
	struct hfi_eq_mgmt *eqm;
	long time_in_jiffies, stat;	/* has to be same type */
	int ret;

	down_read(&ctx->ctx_rwsem);
	mutex_lock(&ctx->event_mutex);

	ret = eqflag ?	/* EQ or CT ? */
		hfi_eq_setup(ctx, ev_idx, &eqm) :
		hfi_ct_setup(ctx, ev_idx, &eqm);
	if (ret)
		goto idr_end;

	if (eqm->ec) {
		if (eqm->ec != (struct hfi_ec_entry *)eqm) {
			ret = -EBUSY;
			goto idr_end;
		}
	} else {
		/* associate eq with self event channel */
		eqm->ec = (struct hfi_ec_entry *)eqm;

		if (ctx->type != HFI_CTX_TYPE_KERNEL) { /* user */
			if (eqflag) { /* EQ */
				ret = hfi_eq_arm(ctx, eqm, *user_data0, 0);
				if (ret)
					goto idr_end;

				/*
				 * clear data0, user space will know this
				 * thread has setup the interrupt and
				 * wait for completion.
				 */
				*user_data0 = 0;
			} else { /* CT */
				ret = hfi_ct_arm(ctx, eqm);
				if (ret)
					goto idr_end;
			}
		}
	}

	kref_get(&eqm->refcount);
	mutex_unlock(&ctx->event_mutex);

	if (timeout < 0)
		time_in_jiffies = MAX_SCHEDULE_TIMEOUT;
	else
		time_in_jiffies = msecs_to_jiffies(timeout);

	/*
	 * 'stat' must be 'long' type because
	 * the function return 'long' value, otherwise,
	 * 'stat' could become negative value on success.
	 */
	stat = wait_woken_event_interruptible_timeout(
		eqm->wq,
		hfi_has_event(ctx, eqm, &ret),
		time_in_jiffies);

	mutex_lock(&ctx->event_mutex);
	dd_dev_dbg(ctx->devdata, "eq_wait_event returned %ld\n", stat);

	/*
	 * At this point, there several situations:
	 * 1. if ret !=0, EQ/CT is released by another thread;
	 * 2. otherwise, it could be interrupted, timedout,
	 *    or a successful event return. In either case,
	 *    we want to turn off the EQ/CT interrupt, and
	 *    dis-associate the self event channel.
	 * Note that the kref count is 1 on opening and drops to 0 on closing.
	 * Each waiting thread inc/dec count during the operation, so "<=2"
	 * means I am the only waiting thread.
	 */
	if (!ret && kref_read(&eqm->refcount) <= 2 &&
	    ctx->type != HFI_CTX_TYPE_KERNEL) {
		/* turn off interrupt on this eq */
		if (eqflag) { /* EQ */
			ret = hfi_eq_disarm(ctx, eqm, *user_data1, true);
			if (!ret) {
				/*
				 * clear data1, user space will know this
				 * thread has stopped the interrupt and
				 * wait for completion.
				 */
				*user_data1 = 0;
			}
		} else { /* CT */
			ret = hfi_ct_disarm(ctx, eqm);
			if (!ret) {
				/*
				 * Everything is fine, return ct.threshold
				 * back to user space.
				 */
				union ptl_ct_event *ct_desc_base =
					(void *)(ctx->ptl_state_base
					+ HFI_PSB_CT_OFFSET);
				*user_data1 = ct_desc_base
					[eqm->desc.idx].threshold;
			}
		}

		/* disassociate with event channel */
		eqm->ec = NULL;
	}

	if (!ret) {
		if (stat < 0)		/* interrupted */
			ret = -ERESTARTSYS;
		else if (stat == 0)	/* timeout */
			ret = -ETIME;
	}
	kref_put(&eqm->refcount, hfi_eq_mgmt_free);

idr_end:
	mutex_unlock(&ctx->event_mutex);
	up_read(&ctx->ctx_rwsem);

	return ret;
}

/*
 * This function is called via event cleanup as a idr_for_each function, hence
 * the curious looking arguments.
 */
static int hfi_ec_remove(int ec_idx, void *idr_ptr, void *idr_ctx)
{
	/* there is no other cleanup work for ec */
	kfree(idr_ptr);
	return 0;
}

static void hfi_ib_cq_isr(struct hfi_eq_mgmt *eqm)
{
	struct ib_cq *ibcq = eqm->ibcq;
	struct rvt_cq *cq = ibcq_to_rvtcq(ibcq);
	struct hfi_ctx *ctx;
	struct hfi_devdata *dd;
	union eqd *eq_desc_base;
	union eqd eq_desc;
	union hfi_rx_cq_command cmd;
	int ret, slots;
	struct hfi_ibeq *ibeq;

	if (!cq || !ibcq)
		return;
	/*
	 * TODO - this use of cq is unsafe until we hook into
	 * destroy_cq.  It needs to call hfi_eq_unlink().
	 */

	/* schedule work item to notify user space */
	if (ibcq->comp_handler && (cq->notify & IB_CQ_SOLICITED_MASK)) {
		spin_lock(&cq->rdi->n_cqs_lock);
		if (likely(cq->rdi->worker)) {
			cq->notify = RVT_CQ_NONE;
			cq->triggered++;
			kthread_queue_work(cq->rdi->worker, &cq->comptask);
		}
		spin_unlock(&cq->rdi->n_cqs_lock);
	}

	spin_lock(&cq->lock);
	eqm->ibcq = NULL;
	if (list_empty(&cq->hw_cq))
		goto done;

	list_for_each_entry(ibeq, &cq->hw_cq, hw_cq) {
		eq_desc_base = (void *)(ibeq->eq.ctx->ptl_state_base +
					HFI_PSB_EQ_DESC_OFFSET);
		ctx = ibeq->eq.ctx;
		dd = ctx->devdata;

		/* setup interrupt for this EQ */
		/* issue write to privileged CMDQ to complete */
		slots = _hfi_eq_update_intr(ctx, &dd->priv_cmdq.rx,
					    ibeq->eq.idx, -1, 0,
					    (ctx->type == HFI_CTX_TYPE_USER) ?
					    ibeq->hw_disarmed :
					    (u64)&ibeq->hw_disarmed,
					    &cmd);

		/* Queue write, no wait */
		ret = hfi_pend_cmd_queue(&dd->pend_cmdq, &dd->priv_cmdq.rx,
					 NULL, &cmd, slots, GFP_ATOMIC);
		if (ret) {
			dd_dev_err(dd, "%s: hfi_pend_cmd_queue failed %d\n",
				   __func__, ret);
		}

		/* update host memory EQD copy */
		eq_desc.val[0] = eq_desc_base[ibeq->eq.idx].val[0];
		eq_desc.irq = 0;
		eq_desc.i = 0;
		eq_desc.s = 0;
		eq_desc_base[ibeq->eq.idx].val[0] = eq_desc.val[0];
	}
done:
	spin_unlock(&cq->lock);

	/* remove EQ to list of IRQ waiters */
	eqm->irq_vector = 0;
	list_del_init(&eqm->irq_wait_chain);
}

irqreturn_t hfi_irq_eq_handler(int irq, void *dev_id)
{
	struct hfi_irq_entry *me = dev_id;
	struct hfi_devdata *dd = me->dd;
	struct hfi_eq_mgmt *eqm, *next;

	this_cpu_inc(*dd->int_counter);

	/*
	 * FXRTODO: Revisit if this delay is required with FXR PCIe
	 * It was required to prevent a race between interrupt and data
	 * over IDI and IOSF legacy FXR data paths.
	 */
	if (dd->emulation)
		mdelay(10);

	trace_hfi2_irq_eq(me);

	/*
	 * Wake up threads using this IRQ:
	 * 1. for event channel waiting, wake up the first thread;
	 * 2. for direct EQ waiting, wake up the first thread;
	 * 3. for direct CT waiting, wake up all the threads;
	 *
	 * If eqm is on IRQ list, it must have eqm->ec pointer.
	 */
	spin_lock(&me->irq_wait_lock);
	list_for_each_entry_safe(eqm, next, &me->irq_wait_head,
				 irq_wait_chain) {
		if (eqm->mode == HFI_EC_MODE_IB)
			hfi_ib_cq_isr(eqm);
		else if (eqm->isr_cb)
			eqm->isr_cb(&eqm->desc, eqm->cookie);
		else if (eqm->ec->mode == HFI_EC_MODE_CT_SELF)
			wake_up_interruptible_all(&eqm->ec->wq);
		else
			wake_up_interruptible(&eqm->ec->wq);
	}
	spin_unlock(&me->irq_wait_lock);

	return IRQ_HANDLED;
}

static int hfi_eq_arm(struct hfi_ctx *ctx, struct hfi_eq_mgmt *eqm,
		      u64 user_data, u8 solicit)
{
	struct hfi_devdata *dd = ctx->devdata;
	struct hfi_irq_entry *me;
	union eqd *eq_desc_base = (void *)(ctx->ptl_state_base +
				  HFI_PSB_EQ_DESC_OFFSET);
	unsigned long flags;
	union eqd eq_desc;
	union hfi_rx_cq_command cmd;
	int irq_idx, eq_idx;
	int ret, slots;

	/* for now just do round-robin assignment */
	irq_idx = atomic_inc_return(&dd->irq_eq_next) %
		   dd->num_eq_irqs;

	/* setup interrupt for this EQ */
	/* issue write to privileged CMDQ to complete */
	slots = _hfi_eq_update_intr(ctx, &dd->priv_cmdq.rx,
				    eqm->desc.idx, irq_idx, 0,
				    user_data, &cmd);

	/* Queue write, wait for completion */
	ret = hfi_pend_cmd_queue_wait(&dd->pend_cmdq, &dd->priv_cmdq.rx, NULL,
				      &cmd, slots);
	if (ret) {
		dd_dev_err(dd, "%s: hfi_pend_cmd_queue_wait failed %d\n",
			   __func__, ret);
		return ret;
	}

	/* update host memory EQD copy */
	eq_idx = eqm->desc.idx;
	eq_desc.val[0] = eq_desc_base[eq_idx].val[0];
	eq_desc.irq = irq_idx;
	eq_desc.i = 1;
	eq_desc.s = solicit;
	eq_desc_base[eq_idx].val[0] = eq_desc.val[0];

	/* add EQ to list of IRQ waiters */
	me = &dd->irq_entries[irq_idx];
	eqm->irq_vector = irq_idx;
	spin_lock_irqsave(&me->irq_wait_lock, flags);
	list_add(&eqm->irq_wait_chain, &me->irq_wait_head);
	spin_unlock_irqrestore(&me->irq_wait_lock, flags);

	return ret;
}

static int hfi_eq_disarm(struct hfi_ctx *ctx, struct hfi_eq_mgmt *eqm,
			 u64 user_data, bool wait)
{
	struct hfi_devdata *dd = ctx->devdata;
	struct hfi_irq_entry *me;
	union eqd *eq_desc_base = (void *)(ctx->ptl_state_base +
				  HFI_PSB_EQ_DESC_OFFSET);
	unsigned long flags;
	union eqd eq_desc;
	union hfi_rx_cq_command cmd;
	int eq_idx, ret, slots;

	/* remove interrupt for this EQ */
	slots = _hfi_eq_update_intr(ctx, &dd->priv_cmdq.rx,
				    eqm->desc.idx, -1, 0, user_data, &cmd);

	/* Queue write, wait for completion */
	if (wait)
		ret = hfi_pend_cmd_queue_wait(&dd->pend_cmdq, &dd->priv_cmdq.rx,
					      NULL, &cmd, slots);
	else
		ret = hfi_pend_cmd_queue(&dd->pend_cmdq, &dd->priv_cmdq.rx,
					 NULL, &cmd, slots, GFP_ATOMIC);
	if (ret) {
		dd_dev_err(dd, "%s: hfi_pend_cmd_queue_wait failed %d\n",
			   __func__, ret);
		return ret;
	}

	/* update host memory EQD copy */
	eq_idx = eqm->desc.idx;
	eq_desc.val[0] = eq_desc_base[eq_idx].val[0];
	eq_desc.irq = 0;
	eq_desc.i = 0;
	eq_desc_base[eq_idx].val[0] = eq_desc.val[0];

	/* remove EQ to list of IRQ waiters */
	me = &dd->irq_entries[eqm->irq_vector];
	eqm->irq_vector = 0;
	if (eqm->mode != HFI_EC_MODE_IB) {
		spin_lock_irqsave(&me->irq_wait_lock, flags);
		list_del_init(&eqm->irq_wait_chain);
		spin_unlock_irqrestore(&me->irq_wait_lock, flags);
	} else {
		list_del_init(&eqm->irq_wait_chain);
	}

	return ret;
}

static bool hfi_ec_has_event(struct hfi_ctx *ctx,
			     struct hfi_ec_entry *ec,
			     u64 *user_data0, u64 *user_data1, int *ret)
{
	union ptl_ct_event *ct_desc_base = (void *)(ctx->ptl_state_base
						    + HFI_PSB_CT_OFFSET);
	struct hfi_eq_mgmt *eqm;
	bool eqflag = (ec->mode == HFI_EC_MODE_EQ);
	bool cond = false;

	mutex_lock(&ctx->event_mutex);

	/* if ec is empty, the last eq/ct on ec has been released */
	if (list_empty(&ec->ec_wait_head)) {
		cond = true;
		*ret = -EINTR;
	} else {
		list_for_each_entry(eqm, &ec->ec_wait_head, ec_wait_chain) {
			cond = eqflag ?
				__hfi_eq_wait_condition(ctx, &eqm->desc) :
				__hfi_ct_wait_condition(ct_desc_base, eqm);
			if (!cond)
				continue;

			/*
			 * remove eq/ct from ec list so other
			 * thread won't get the same eq/ct
			 */
			list_del_init(&eqm->ec_wait_chain);

			/* turn off interrupt for this eq */
			*ret = eqflag ?
				hfi_eq_disarm(ctx, eqm, *user_data1, true) :
				hfi_ct_disarm(ctx, eqm);

			/* disassociate eq/ct with event channel */
			eqm->ec = NULL;

			/* return eq event to user space */
			eqm->num_unacked++;
			if (eqflag) {
				*user_data0 = eqm->user_handle;
			} else {
				*user_data0 = (u64)eqm->desc.idx;
				*user_data1 = ct_desc_base
					[eqm->desc.idx].threshold;
			}

			break;
		}
	}

	mutex_unlock(&ctx->event_mutex);

	return cond;
}

int hfi_ib_eq_arm(struct hfi_ctx *ctx, struct ib_cq *ibcq,
		  struct hfi_ibeq *ibeq, u8 solicit)
{
	struct rvt_cq *rcq = rcq = ibcq_to_rvtcq(ibcq);
	union hfi_rx_cq_command cmd;
	union eqd *eq_desc_base;
	union eqd eq_desc;
	int slots, ret = 0, irq_idx;

	eq_desc_base = (void *)(ibeq->eq.ctx->ptl_state_base +
				HFI_PSB_EQ_DESC_OFFSET);
	irq_idx = ((struct hfi_eq_mgmt *)rcq->eqm)->irq_vector;

	/* setup interrupt for this EQ */
	/* issue write to privileged CMDQ to complete */
	slots = _hfi_eq_update_intr(ibeq->eq.ctx,
				    &ctx->devdata->priv_cmdq.rx,
				    ibeq->eq.idx, irq_idx,
				    solicit,
				    (ibeq->eq.ctx->type == HFI_CTX_TYPE_USER) ?
				    ibeq->hw_armed : (u64)&ibeq->hw_armed,
				    &cmd);

	/* Queue write, no wait */
	ret = hfi_pend_cmd_queue(&ctx->devdata->pend_cmdq,
				 &ctx->devdata->priv_cmdq.rx,
				 NULL, &cmd, slots, GFP_ATOMIC);
	if (ret) {
		dd_dev_err(ctx->devdata, "%s: hfi_pend_cmd_queue failed %d\n",
			   __func__, ret);
	}

	/* update host memory EQD copy */
	eq_desc.val[0] = eq_desc_base[ibeq->eq.idx].val[0];
	eq_desc.irq = irq_idx;
	eq_desc.i = 1;
	eq_desc.s = solicit;
	eq_desc_base[ibeq->eq.idx].val[0] = eq_desc.val[0];

	return 0;
}

int hfi_ib_cq_arm(struct hfi_ctx *ctx, struct ib_cq *ibcq, u8 solicit)
{
	struct rvt_cq *rcq = rcq = ibcq_to_rvtcq(ibcq);
	struct hfi_eq_mgmt *eqm;
	struct hfi_irq_entry *me;
	unsigned long flags;
	int irq_idx;
	struct hfi_devdata *dd;
	struct hfi_ibeq *ibeq;
	int ret;

	if (!rcq)
		return 0;

	if (!rcq->eqm) {
		eqm = kzalloc(sizeof(*eqm), GFP_KERNEL);
		if (!eqm)
			return -ENOMEM;
		rcq->eqm = eqm;

		/* initialize EQM state */
		INIT_LIST_HEAD(&eqm->irq_wait_chain);
		INIT_LIST_HEAD(&eqm->ec_wait_chain);
		init_waitqueue_head(&eqm->wq);
		kref_init(&eqm->refcount);
	} else {
		eqm = rcq->eqm;
	}

	eqm->mode = HFI_EC_MODE_IB;
	eqm->ibcq = ibcq;
	eqm->ec = (struct hfi_ec_entry *)eqm;
	dd = ctx->devdata;
	eqm->cookie = (void *)dd;
	/* for now just do round-robin assignment */
	irq_idx = atomic_inc_return(&dd->irq_eq_next) %
		   dd->num_eq_irqs;
	eqm->irq_vector = irq_idx;

	spin_lock_irqsave(&rcq->lock, flags);
	if (ctx->type == HFI_CTX_TYPE_USER) {
		if (solicit)
			rcq->notify = IB_CQ_SOLICITED;
		else
			rcq->notify = IB_CQ_NEXT_COMP;
	}
	if (!list_empty(&rcq->hw_cq)) {
		list_for_each_entry(ibeq, &rcq->hw_cq, hw_cq) {
			ret = hfi_ib_eq_arm(ctx, ibcq, ibeq, solicit);
			if (ret) {
				spin_unlock_irqrestore(&rcq->lock, flags);
				return ret;
			}
		}
	}
	spin_unlock_irqrestore(&rcq->lock, flags);

	/* add EQ to list of IRQ waiters */
	me = &dd->irq_entries[irq_idx];
	spin_lock_irqsave(&me->irq_wait_lock, flags);
	list_add(&eqm->irq_wait_chain, &me->irq_wait_head);
	spin_unlock_irqrestore(&me->irq_wait_lock, flags);

	return 0;
}

int hfi_ec_assign(struct hfi_ctx *ctx, u16 *ec_idx)
{
	struct hfi_ec_entry *ec;
	int idr_max, idr, ret = 0;

	ec = kzalloc(sizeof(*ec), GFP_KERNEL);
	if (!ec)
		return -ENOMEM;

	idr_max = HFI_NUM_NIS * HFI_NUM_EVENT_HANDLES;

	down_read(&ctx->ctx_rwsem);

	/* verify we have an assigned context */
	if (ctx->pid == HFI_PID_NONE) {
		kfree(ec);
		up_read(&ctx->ctx_rwsem);
		return -EPERM;
	}

	/*
	 * allocate event channel idr index and
	 * associate with ec managemen object.
	 */
	mutex_lock(&ctx->event_mutex);
	idr = idr_alloc(&ctx->ec_used, (void *)ec,
			0, idr_max, GFP_KERNEL);
	if (idr < 0) {
		/* all ECs are assigned */
		ret = idr;
		kfree(ec);
		goto idr_end;
	}

	/* initialize EC state */
	INIT_LIST_HEAD(&ec->ec_wait_head);
	init_waitqueue_head(&ec->wq);

	*ec_idx = idr;

idr_end:
	mutex_unlock(&ctx->event_mutex);
	up_read(&ctx->ctx_rwsem);

	return ret;
}

int hfi_ec_release(struct hfi_ctx *ctx, u16 ec_idx)
{
	struct hfi_ec_entry *ec;
	int idr_max = HFI_NUM_NIS * HFI_NUM_EVENT_HANDLES;
	int ret = 0;

	/* check input range */
	if (ec_idx >= idr_max)
		return -EINVAL;

	down_read(&ctx->ctx_rwsem);
	mutex_lock(&ctx->event_mutex);
	ec = idr_find(&ctx->ec_used, ec_idx);
	if (!ec) {
		ret = -EINVAL;
		goto idr_end;
	}

	if (!list_empty(&ec->ec_wait_head)) {
		ret = -EBUSY;
		goto idr_end;
	}

	hfi_ec_remove(ec_idx, ec, ctx);

	idr_remove(&ctx->ec_used, ec_idx);

idr_end:
	mutex_unlock(&ctx->event_mutex);
	up_read(&ctx->ctx_rwsem);

	return ret;
}

/*
 * For EC with EQs, user_data0 is the address to return user
 * space eq address, user data1 is the user space completion
 * address to turn off eq interrupt.
 * For EC with CTs, user_data0 is the address to return ct
 * handle (index), user_data1 is the address to return
 * ct.threshold.
 */
int hfi_ec_wait_event(struct hfi_ctx *ctx, u16 ec_idx,
		      int timeout, u64 *user_data0, u64 *user_data1)
{
	struct hfi_ec_entry *ec;
	int idr_max = HFI_NUM_NIS * HFI_NUM_EVENT_HANDLES;
	long time_in_jiffies, stat;
	int ret = 0;

	/* check input range */
	if (ec_idx >= idr_max)
		return -EINVAL;

	/* check where to return event */
	if (!user_data0)
		return -EINVAL;

	down_read(&ctx->ctx_rwsem);

	mutex_lock(&ctx->event_mutex);
	ec = idr_find(&ctx->ec_used, ec_idx);
	mutex_unlock(&ctx->event_mutex);

	if (!ec) {
		ret = -EINVAL;
		goto idr_end;
	}

	/* if event channel is empty */
	if (list_empty(&ec->ec_wait_head)) {
		ret = -EINVAL;
		goto idr_end;
	}

	if (ec->mode != HFI_EC_MODE_CT && ec->mode != HFI_EC_MODE_EQ) {
		ret = -EINVAL;
		goto idr_end;
	}

	if (timeout < 0)
		time_in_jiffies = MAX_SCHEDULE_TIMEOUT;
	else
		time_in_jiffies = msecs_to_jiffies(timeout);

	/*
	 * 'stat' must be 'long' type because
	 * the function return 'long' value, otherwise,
	 * 'stat' could become negative value on success.
	 */
	stat = wait_woken_event_interruptible_timeout(
		ec->wq,
		hfi_ec_has_event(ctx, ec, user_data0, user_data1, &ret),
		time_in_jiffies);

	dd_dev_dbg(ctx->devdata, "ec_wait_eq_event returned %ld\n", stat);

	/*
	 * When stat > 0, either EC becomes empty, or the returned
	 * EQ/CT has an event, ret has already been set correctly.
	 * if an EQ/CT is removed from EC in this call to make EC
	 * empty, we need to wake up other waiting threads.
	 */
	if (stat < 0)		/* interrupted */
		ret = -ERESTARTSYS;
	else if (stat == 0)	/* timeout */
		ret = -ETIME;
	else if (!ret && list_empty(&ec->ec_wait_head))
		wake_up_interruptible_all(&ec->wq);

idr_end:
	up_read(&ctx->ctx_rwsem);

	return ret;
}

int hfi_eq_ack_event(struct hfi_ctx *ctx, u16 eq_idx, u32 nevents)
{
	struct hfi_eq_ctx *eq_ctx;
	int ret = 0;

	down_read(&ctx->ctx_rwsem);
	mutex_lock(&ctx->event_mutex);

	eq_ctx = idr_find(&ctx->eq_used, eq_idx);
	if (!eq_ctx || !eq_ctx->eqm)
		ret = -EINVAL;
	else
		eq_ctx->eqm->num_unacked -= nevents;

	mutex_unlock(&ctx->event_mutex);
	up_read(&ctx->ctx_rwsem);

	return ret;
}

static int hfi_eq_setup(struct hfi_ctx *ctx,
			u16 eq_idx, struct hfi_eq_mgmt **peqm)
{
	struct hfi_eq_ctx *eq_ctx;
	struct hfi_eq_mgmt *eqm;
	union eqd *eq_desc_base = (void *)(ctx->ptl_state_base +
				  HFI_PSB_EQ_DESC_OFFSET);

	eq_ctx = idr_find(&ctx->eq_used, eq_idx);
	if (!eq_ctx)
		return -EINVAL;

	if (!eq_ctx->eqm) {
		u32 *eq_head_array = ctx->eq_head_addr;

		eqm = kzalloc(sizeof(*eqm), GFP_KERNEL);
		if (!eqm)
			return -ENOMEM;

		eq_ctx->eqm = eqm;

		/* initialize EQ state */
		INIT_LIST_HEAD(&eqm->irq_wait_chain);
		INIT_LIST_HEAD(&eqm->ec_wait_chain);
		init_waitqueue_head(&eqm->wq);
		kref_init(&eqm->refcount);
		eqm->mode = HFI_EC_MODE_EQ_SELF;

		/*
		 * Record info needed for hfi_eq_peek(),
		 * we must make typecasting first before shifting,
		 * otherwise we might shift out of 'start' size
		 * and lose address info.
		 */
		eqm->desc.base = (void *)(((u64)
				eq_desc_base[eq_idx].start) <<
				PAGE_SHIFT);
		eqm->desc.count = 1 << eq_desc_base[eq_idx].order;
		eqm->desc.width = eq_desc_base[eq_idx].sz ?
					HFI_EQ_ENTRY_LOG2_JUMBO :
					HFI_EQ_ENTRY_LOG2;
		eqm->desc.idx = eq_idx;
		eqm->desc.head_addr = &eq_head_array[eq_idx];
	} else {
		eqm = eq_ctx->eqm;
	}

	/* return eq to caller */
	*peqm = eqm;

	return 0;
}

/*
 * When called for EQ, 'eq_handle' is the user space EQ address
 * pointer, 'user_data' is the user space completion address for
 * turning on interrupt. When called for CT, 'eq_handle' is zero
 * to distinguish from EQ, 'user_data' is ignored.
 */
int hfi_ev_set_channel(struct hfi_ctx *ctx, u16 ec_idx,
		       u16 ev_idx, u64 eq_handle, u64 user_data)
{
	struct hfi_ec_entry *ec;
	struct hfi_eq_mgmt *eqm;
	int mode = eq_handle ? HFI_EC_MODE_EQ : HFI_EC_MODE_CT;
	int ret;

	down_read(&ctx->ctx_rwsem);
	mutex_lock(&ctx->event_mutex);

	ec = idr_find(&ctx->ec_used, ec_idx);
	if (!ec) {
		ret = -EINVAL;
		goto idr_end;
	}

	if (list_empty(&ec->ec_wait_head)) {
		ec->mode = mode;
	} else if (ec->mode != mode) {
		ret = -EINVAL;
		goto idr_end;
	}

	ret = eq_handle ?
		hfi_eq_setup(ctx, ev_idx, &eqm) :
		hfi_ct_setup(ctx, ev_idx, &eqm);
	if (ret)
		goto idr_end;

	/* error if eqm is already associated with ec */
	if (eqm->ec) {
		/* check if associated with currently requested ec */
		if (eqm->ec == ec)
			ret = -EALREADY;
		else
			ret = -EBUSY;

		goto idr_end;
	}

	/* put eq/ct to event channel list */
	list_add(&eqm->ec_wait_chain,
		 &ec->ec_wait_head);
	eqm->ec = ec;

	/* set user space EQ handle, skip EQ test */
	eqm->user_handle = eq_handle;

	ret = eq_handle ?
		hfi_eq_arm(ctx, eqm, user_data, 0) :
		hfi_ct_arm(ctx, eqm);

idr_end:
	mutex_unlock(&ctx->event_mutex);
	up_read(&ctx->ctx_rwsem);

	return ret;
}

/* called when holding ctx_rwsem exclusively. */
void hfi_event_cleanup(struct hfi_ctx *ctx)
{
	int ni;

	/* remove CTs from other lists and free memory */
	idr_for_each(&ctx->ct_used, hfi_ct_remove, ctx);
	idr_destroy(&ctx->ct_used);

	/* remove EQs from other lists and free memory */
	idr_for_each(&ctx->eq_used, hfi_eq_remove, ctx);
	idr_destroy(&ctx->eq_used);

	/* free even channel memory */
	idr_for_each(&ctx->ec_used, hfi_ec_remove, ctx);
	idr_destroy(&ctx->ec_used);

	/* free EQ0 base */
	for (ni = 0; ni < HFI_NUM_NIS; ni++) {
		kfree(ctx->eq_zero[ni].base);
		ctx->eq_zero[ni].base = NULL;
	}
}

int hfi_cteq_assign(struct hfi_ctx *ctx, struct opa_ev_assign *ev_assign)
{
	if (ev_assign->mode & OPA_EV_MODE_COUNTER)
		return hfi_ct_assign(ctx, ev_assign);
	else
		return hfi_eq_assign(ctx, ev_assign);
}

int hfi_cteq_release(struct hfi_ctx *ctx, u16 ev_mode,
		     u16 ev_idx, u64 user_data)
{
	if (ev_mode & OPA_EV_MODE_COUNTER)
		return hfi_ct_release(ctx, ev_idx);
	else
		return hfi_eq_release(ctx, ev_idx, user_data);
}

int hfi_ib_cq_armed(struct ib_cq *ibcq)
{
	struct rvt_cq *rcq = rcq = ibcq_to_rvtcq(ibcq);
	struct hfi_eq_mgmt *eqm = rcq->eqm;

	if (eqm && eqm->ibcq)
		return 1;
	else
		return 0;
}

int hfi_ib_cq_disarm_irq(struct ib_cq *ibcq)
{
	struct rvt_cq *rcq = rcq = ibcq_to_rvtcq(ibcq);
	struct hfi_eq_mgmt *eqm = rcq->eqm;
	unsigned long flags;

	if (hfi_ib_cq_armed(ibcq)) {
		struct hfi_devdata *dd = eqm->cookie;
		struct hfi_irq_entry *me = &dd->irq_entries[eqm->irq_vector];

		spin_lock_irqsave(&me->irq_wait_lock, flags);
		if (hfi_ib_cq_armed(ibcq))
			list_del_init(&eqm->irq_wait_chain);
		spin_unlock_irqrestore(&me->irq_wait_lock, flags);
	}

	return 0;
}

int hfi_ib_eq_release(struct hfi_ctx *ctx, struct ib_cq *cq, u16 eq_idx)
{
	struct hfi_eq_ctx *eq_ctx;
	unsigned long flags;
	struct hfi_ibeq *ibeq, *next;
	struct rvt_cq *rcq;
	int ret = 0;

	down_read(&ctx->ctx_rwsem);
	mutex_lock(&ctx->event_mutex);

	eq_ctx = idr_find(&ctx->eq_used, eq_idx);
	if (!eq_ctx) {
		ret = -EINVAL;
		goto idr_end;
	}

	if (eq_ctx->ibeq) {
		if (!cq) {
			ret = -EINVAL;
			goto idr_end;
		}

		rcq = ibcq_to_rvtcq(cq);
		spin_lock_irqsave(&rcq->lock, flags);
		if (!list_empty(&rcq->hw_cq)) {
			list_for_each_entry_safe(ibeq, next,
						 &rcq->hw_cq, hw_cq) {
				if (ibeq->eq.ctx == ctx &&
				    ibeq->eq.idx == eq_idx) {
					list_del(&ibeq->hw_cq);
					kfree(ibeq);
					goto found;
				}
			}
			ret = -EINVAL;
		}
found:
		spin_unlock_irqrestore(&rcq->lock, flags);
	} else if (cq) {
		ret = -EINVAL;
	}

idr_end:
	mutex_unlock(&ctx->event_mutex);
	up_read(&ctx->ctx_rwsem);

	return ret;
}

int hfi_ib_eq_add(struct hfi_ctx *ctx, struct ib_cq *cq, u64 disarm_done,
		  u64 arm_done, u16 eq_idx)
{
	struct rvt_cq *rcq = ibcq_to_rvtcq(cq);
	struct hfi_ibeq *ibeq;
	union hfi_rx_cq_command cmd;
	unsigned long flags;
	union eqd *eq_desc_base;
	union eqd eq_desc;
	int slots, ret = 0, irq_idx;
	struct hfi_eq_ctx *eq_ctx;

	ibeq = kmalloc(sizeof(*ibeq), GFP_KERNEL);
	if (!ibeq)
		return -ENOMEM;

	ibeq->eq.ctx = ctx;
	ibeq->hw_disarmed = disarm_done;
	ibeq->hw_armed = arm_done;
	ibeq->eq.idx = eq_idx;

	down_read(&ctx->ctx_rwsem);
	mutex_lock(&ctx->event_mutex);

	eq_ctx = idr_find(&ctx->eq_used, eq_idx);
	if (!eq_ctx) {
		kfree(ibeq);
		ret = -EINVAL;
		goto idr_end;
	}
	eq_ctx->ibeq = ibeq;

	spin_lock_irqsave(&rcq->lock, flags);
	list_add_tail(&ibeq->hw_cq, &rcq->hw_cq);
	if (hfi_ib_cq_armed(cq)) {
		eq_desc_base = (void *)(ctx->ptl_state_base +
					HFI_PSB_EQ_DESC_OFFSET);
		irq_idx = ((struct hfi_eq_mgmt *)rcq->eqm)->irq_vector;

		/* setup interrupt for this EQ */
		/* issue write to privileged CMDQ to complete */
		slots = _hfi_eq_update_intr(ctx, &ctx->devdata->priv_cmdq.rx,
					    ibeq->eq.idx, irq_idx,
					    (rcq->notify == IB_CQ_SOLICITED),
					    ibeq->hw_armed,
					    &cmd);

		/* Queue write, no wait */
		ret = hfi_pend_cmd_queue(&ctx->devdata->pend_cmdq,
					 &ctx->devdata->priv_cmdq.rx,
					 NULL, &cmd, slots, GFP_KERNEL);
		if (ret) {
			dd_dev_err(ctx->devdata,
				   "%s: hfi_pend_cmd_queue failed %d\n",
				   __func__, ret);
		}

		/* update host memory EQD copy */
		eq_desc.val[0] = eq_desc_base[ibeq->eq.idx].val[0];
		eq_desc.irq = irq_idx;
		eq_desc.i = 1;
		eq_desc.s = (rcq->notify == IB_CQ_SOLICITED);
		eq_desc_base[ibeq->eq.idx].val[0] = eq_desc.val[0];
	}
	spin_unlock_irqrestore(&rcq->lock, flags);

idr_end:
	mutex_unlock(&ctx->event_mutex);
	up_read(&ctx->ctx_rwsem);

	return ret;
}
