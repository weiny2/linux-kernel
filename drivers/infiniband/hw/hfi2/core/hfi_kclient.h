/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2015 - 2016 Intel Corporation.
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
 * Copyright (c) 2015 - 2016 Intel Corporation.
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
 * Intel(R) Omni-Path Kernel Client Logic
 */

#ifndef _HFI_KCLIENT_H
#define _HFI_KCLIENT_H

#include <linux/vmalloc.h>
#include "hfi_core.h"
#include "hfi_pt_bypass.h"
#include "../hfi2.h"

/*
 * Macros to extract and deposit from/to structures and bitfields
 * from the hardware defined structures
 */
static inline
u64 _hfi_extract(u64 val, const u64 offset, const u64 mask)
{
	return ((val & mask) >> offset);
}

static inline
u64 _hfi_deposit(u64 reg, u64 src, const u64 offset, const u64 mask)
{
	return ((reg & ~mask) | ((src << offset) & mask));
}

/*
 * An hfi_eq pointer can never be NULL (we don't want to NULL-check
 * everywhere), so we create a dummy value we can pass into API
 * functions in cases where we didn't ask for and don't want any
 * events to be generated.
 */
static struct hfi_eq __attribute__((unused)) eq_handle_none;
#define HFI_EQ_NONE (&eq_handle_none)

/* timeout for EQ0 completion event */
#define HFI_EQ0_WAIT_TIMEOUT_MS		500

static inline
void hfi_set_eq(struct hfi_ctx *ctx, struct hfi_eq *eq,
		struct opa_ev_assign *eq_assign,
		u32 *head_addr)
{
	eq->base = (void *)eq_assign->base;
	eq->count = eq_assign->count;
	eq->width = eq_assign->jumbo ? HFI_EQ_ENTRY_LOG2_JUMBO :
				       HFI_EQ_ENTRY_LOG2;
	eq->idx = eq_assign->ev_idx;
	eq->head_addr = head_addr;
	eq->ctx = ctx;
	eq->events_pending.counter = 0;
}

/*
 * 'dropped' is converted to -EIO error, return:
 * 0 on new event without error, otherwise negative error code.
 */
static inline
int hfi_eq_wait_timed(struct hfi_eq *eq, unsigned int timeout_ms,
		      u64 **eq_entry)
{
	int ret;
	bool dropped;
	unsigned long exit_jiffies = jiffies +
			msecs_to_jiffies(timeout_ms);

	while (1) {
		ret = hfi_eq_peek(eq, eq_entry, &dropped);
		if (ret != HFI_EQ_EMPTY)
			break;
		if (time_after(jiffies, exit_jiffies))
			return -ETIME;
		cpu_relax();
	}
	if (ret > 0)
		ret = dropped ? -EIO : 0;

	return ret;
}

static inline
int hfi_eq_wait_irq(struct hfi_eq *eq_handle, unsigned int timeout_ms,
		    u64 **eq_entry, bool *dropped)
{
	int ret;
	struct hfi_ctx *ctx = eq_handle->ctx;

	/* when allocating this 'eq', it must be in blocking mode,
	 * as OPA_EV_MODE_BLOCKING in _hfi_eq_alloc()
	 */
	ret = ctx->ops->ev_wait_single(ctx, 1, eq_handle->idx,
				       timeout_ms, NULL, NULL);
	if (!ret)
		ret = hfi_eq_peek(eq_handle, eq_entry, dropped);
	return ret;
}

static inline
int hfi_eq_poll_cmd_complete_timeout(struct hfi_ctx *ctx, u64 *done)
{
	int ret;
	bool dropped = false;
	unsigned long exit_jiffies = jiffies +
			msecs_to_jiffies(HFI_EQ0_WAIT_TIMEOUT_MS);

	while (*done == 0) {
		u64 *entry = NULL;
		union initiator_EQEntry *event;

		do {
			ret = hfi_eq_peek(HFI_EQ_ZERO(ctx, 0), &entry,
					  &dropped);
			if (ret < 0)
				return ret;

			if (time_after(jiffies, exit_jiffies))
				return -ETIME;
			cpu_relax();
		} while (ret == HFI_EQ_EMPTY);

		event = (union initiator_EQEntry *)entry;

		/* Check that the completion event makes sense */
		if (unlikely(event->event_kind != PTL_CMD_COMPLETE) || dropped)
			return -EIO;

		/* Update the caller's 'done' flag
		 * and return the Event fail_type (NB: must be a non-zero value)
		 */
		if (event->user_ptr)
			*(u64 *)event->user_ptr =
				(event->fail_type | (1ULL << 63));

		hfi_eq_advance(HFI_EQ_ZERO(ctx, 0), entry);
	}

	return 0;
}

static inline
int _hfi_eq_alloc_mode(struct hfi_ctx *ctx,
		       struct opa_ev_assign *eq_alloc,
		       struct hfi_eq *eq,
		       u16 mode)
{
	u16 width;
	u32 *eq_head_array, *eq_head_addr;
	u64 done = 0;
	int rc;

	eq_alloc->user_data = (u64)&done;
	width = eq_alloc->jumbo ? HFI_EQ_ENTRY_LOG2_JUMBO : HFI_EQ_ENTRY_LOG2;
	eq_alloc->base = (u64)vzalloc_node(eq_alloc->count << width,
				ctx->devdata->node);
	if (!eq_alloc->base)
		return -ENOMEM;
	eq_alloc->mode = mode;
	rc = ctx->ops->ev_assign(ctx, eq_alloc);
	if (rc < 0) {
		vfree((void *)eq_alloc->base);
		return rc;
	}
	eq_head_array = ctx->eq_head_addr;
	/* Reset the EQ SW head */
	eq_head_addr = &eq_head_array[eq_alloc->ev_idx];
	*eq_head_addr = 0;
	hfi_set_eq(ctx, eq, eq_alloc, eq_head_addr);

	/* Check on EQ 0 NI 0 for a PTL_CMD_COMPLETE event */
	rc = hfi_eq_poll_cmd_complete_timeout(ctx, &done);
	if (rc < 0)
		return rc;

	hfi_at_reg_range(ctx, (void *)eq_alloc->base,
			 eq_alloc->count << width, NULL, true);

	return 0;
}

#define _hfi_eq_alloc(ctx, eq_alloc, eq) \
	_hfi_eq_alloc_mode(ctx, eq_alloc, eq, OPA_EV_MODE_BLOCKING)

static inline
struct hfi_ibeq *hfi_ibeq_alloc(struct hfi_ctx *ctx,
				struct opa_ev_assign *eq_alloc)
{
	int ret;
	struct hfi_ibeq *ibeq;

	ibeq = kmalloc(sizeof(*ibeq), GFP_KERNEL);
	if (!ibeq)
		return ERR_PTR(-ENOMEM);
	INIT_LIST_HEAD(&ibeq->hw_cq);
	INIT_LIST_HEAD(&ibeq->qp_ll);

	ret = _hfi_eq_alloc_mode(ctx, eq_alloc, &ibeq->eq, 0);
	if (ret) {
		kfree(ibeq);
		ibeq = ERR_PTR(ret);
	}
	return ibeq;
}

static inline
struct hfi_eq *hfi_eq_alloc(struct hfi_ctx *ctx,
			    struct opa_ev_assign *eq_alloc)
{
	int ret;
	struct hfi_eq *eq;

	eq = kmalloc(sizeof(*eq), GFP_KERNEL);
	if (!eq)
		return ERR_PTR(-ENOMEM);

	ret = _hfi_eq_alloc(ctx, eq_alloc, eq);
	if (ret) {
		kfree(eq);
		eq = ERR_PTR(ret);
	}
	return eq;
}

static inline
void _hfi_eq_free(struct hfi_eq *eq)
{
	u64 done = 0;
	struct hfi_ctx *ctx = eq->ctx;

	ctx->ops->ev_release(ctx, 0, eq->idx, (u64)&done);

	/* Check on EQ 0 NI 0 for a PTL_CMD_COMPLETE event */
	hfi_eq_poll_cmd_complete_timeout(ctx, &done);

	hfi_at_dereg_range(ctx, eq->base, eq->count << eq->width);
	vfree(eq->base);
	eq->base = NULL;
}

static inline
void hfi_eq_free(struct hfi_eq *eq)
{
	_hfi_eq_free(eq);
	kfree(eq);
}

/* Returns true if there is a valid pending event and false otherwise */
static inline
bool hfi_eq_wait_condition(struct hfi_eq *eq)
{
	u64 *entry;
	bool dropped;
	int ret;

	ret = hfi_eq_peek(eq, &entry, &dropped);

	if (ret > 0)
		return true;
	return false;
}

static inline
bool hfi_queue_ready(struct hfi_cmdq *cq, u32 slots,
		     struct hfi_eq *eq)
{
	bool slots_avail = hfi_cmd_slots_avail(cq, slots);

	if (eq)
		return slots_avail && !hfi_eq_full(eq);

	return slots_avail;
}

static inline
int hfi_pt_alloc_eager(struct hfi_ctx *ctx, struct hfi_cmdq *rx_cq,
		       struct hfi_pt_alloc_eager_args *args)
{
	union ptentry_fp0_bc1_et0 ptentry;
	int rc;

	if (HFI_CTX_INVALID(ctx))
		return -ENODEV;
	if (!args || !rx_cq)
		return -EINVAL;
	if (args->eager_order > HFI_RX_EAGER_MAX_ORDER)
		return -EINVAL;

	rc = hfi_pt_eager_entry_init(&ptentry, args);
	if (rc)
		return rc;

	ptentry.val[2] = ~0ULL;
	ptentry.val[3] = ~0ULL;
	/* Issue the PT_UPDATE_LOWER_PRIV command via the RX CMDQ */
	do {
		rc = _hfi_pt_issue_cmd(ctx, rx_cq, PT_UPDATE_LOWER_PRIV,
				       HFI_NI_BYPASS,
				       HFI_PT_BYPASS_EAGER,
				       &ptentry.val[0]);
	} while (rc == -EAGAIN);

	if (rc < 0)
		return -EIO;

	return 0;
}
#endif
