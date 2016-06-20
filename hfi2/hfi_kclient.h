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

#include <rdma/hfi_eq.h>
#include <rdma/hfi_tx.h>

static inline int hfi_tx_cmd_bypass_dma(struct hfi_cq *cq, struct hfi_ctx *ctx,
					uint64_t iov_base, uint32_t num_iovs,
					hfi_user_ptr_t user_ptr,
					hfi_md_options_t md_opts,
					hfi_eq_handle_t eq_handle,
					hfi_ct_handle_t ct_handle,
					uint8_t port, uint8_t sl,
					uint8_t slid_low,
					uint8_t l2, uint8_t dma_cmd)
{
	union hfi_tx_general_dma cmd;

	hfi_format_bypass_dma(ctx, &cmd, l2, dma_cmd,
			      iov_base, num_iovs, user_ptr,
			      md_opts, eq_handle, ct_handle,
			      port, sl, slid_low);

	if (unlikely(!hfi_cmd_slots_avail(cq, 1)))
		return -EAGAIN;

	/* Issue the 1-slot DMA */
	_hfi_command(cq, (uint64_t *)&cmd, HFI_CQ_TX_ENTRIES);
	return 0;
}

static int _hfi_eq_alloc(struct hfi_ctx *ctx, struct hfi_cq *cq,
			 spinlock_t *cq_lock,
			 struct opa_ev_assign *eq_alloc,
			 struct hfi_eq *eq, unsigned long timeout_ms)
{
	u32 *eq_head_array, *eq_head_addr;
	u64 *eq_entry = NULL;
	int rc;

	eq_alloc->base = (u64)vzalloc(eq_alloc->count * HFI_EQ_ENTRY_SIZE);
	if (!eq_alloc->base)
		return -ENOMEM;
	eq_alloc->mode = OPA_EV_MODE_BLOCKING;
	rc = ctx->ops->ev_assign(ctx, eq_alloc);
	if (rc < 0) {
		vfree((void *)eq_alloc->base);
		goto err;
	}
	eq_head_array = ctx->eq_head_addr;
	/* Reset the EQ SW head */
	eq_head_addr = &eq_head_array[eq_alloc->ev_idx];
	*eq_head_addr = 0;
	eq->idx = eq_alloc->ev_idx;
	eq->base = (void *)eq_alloc->base;
	eq->count = eq_alloc->count;

	/* Check on EQ 0 NI 0 for a PTL_CMD_COMPLETE event */
	hfi_eq_wait_timed(ctx, &ctx->eq_zero[0], timeout_ms, &eq_entry);
	if (eq_entry) {
		unsigned long flags;

		spin_lock_irqsave(cq_lock, flags);
		hfi_eq_advance(ctx, cq, &ctx->eq_zero[0], eq_entry);
		spin_unlock_irqrestore(cq_lock, flags);
	} else {
		rc = -EIO;
	}

err:
	return rc;
}

static void _hfi_eq_release(struct hfi_ctx *ctx, struct hfi_cq *cq,
			    spinlock_t *cq_lock, struct hfi_eq *eq,
			    unsigned long timeout_ms)
{
	u64 *eq_entry = NULL, done;

	ctx->ops->ev_release(ctx, 0, eq->idx, (u64)&done);
	/* Check on EQ 0 NI 0 for a PTL_CMD_COMPLETE event */
	hfi_eq_wait_timed(ctx, &ctx->eq_zero[0], timeout_ms, &eq_entry);
	if (eq_entry) {
		unsigned long flags;

		spin_lock_irqsave(cq_lock, flags);
		hfi_eq_advance(ctx, cq, &ctx->eq_zero[0], eq_entry);
		spin_unlock_irqrestore(cq_lock, flags);
	}

	vfree(eq->base);
	eq->base = NULL;
}
#endif
