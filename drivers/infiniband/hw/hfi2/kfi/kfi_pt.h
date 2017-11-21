/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2015-2016 Intel Corporation.
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

#ifndef _KFI_PT_H
#define _KFI_PT_H

#include "../core/hfi_pt_common.h"

/* PT entry properties for creating a normal PT entry. */
struct hfi_pt_args {
	struct hfi_eq	*eq_handle;
	bool		flow_control;
	u64		flags;
};

/* PT entry properties for creating a fast PT entry. */
struct hfi_pt_fast_args {
	struct hfi_eq	*eq_handle;
	u16		ct_handle;
	u64		start;
	u64		length;
	u64		flags;
};

/* Internally allocate a PT index. */
static inline int
_hfi_pt_alloc_ptidx(struct hfi_ctx *ctx, u8 ni,
		    u32 req_pt_idx, u32 *actual_pt_idx)
{
	int i;

	if (ctx->pt_free_index[ni] >= HFI_NUM_PT_ENTRIES)
		return -ENOSPC;

	if (req_pt_idx == HFI_PT_ANY) {
		/* Pick any free PT index */
		*actual_pt_idx =
			ctx->pt_free_list[ni][ctx->pt_free_index[ni]++];
		return 0;
	}

	/* Check that the requested PT is free and remove it from list */
	for (i = ctx->pt_free_index[ni]; i < HFI_NUM_PT_ENTRIES; i++) {
		if (ctx->pt_free_list[ni][i] == req_pt_idx)
			break;
	}

	if (i == HFI_NUM_PT_ENTRIES)
		/* Requested PT index is allocated */
		return -EBUSY;

	/* To remove, just fill the now-empty slot with the top value */
	ctx->pt_free_list[ni][i] =
			ctx->pt_free_list[ni][ctx->pt_free_index[ni]++];
	*actual_pt_idx = req_pt_idx;
	return 0;
}

/* Initializes a normal PT entry at the given PT index. */
static inline
int _hfi_pt_normal(struct hfi_ctx *ctx, struct hfi_cmdq *rx_cq, u8 ni,
		   u32 pt_idx, struct hfi_pt_args args)
{
	int rc;
	union ptentry_fp0 ptentry;

	ptentry.val[0] = 0;
	ptentry.val[1] = 0;
	ptentry.val[2] = ~0ULL;
	ptentry.val[3] = ~0ULL;

	ptentry.ni = ni;
	ptentry.eq_handle = args.eq_handle->idx;
	ptentry.enable = 1;
	ptentry.fc = args.flow_control;
	ptentry.v = 1;

	rc = _hfi_pt_issue_cmd(ctx, rx_cq, PT_UPDATE_LOWER, ni, pt_idx,
			       &ptentry.val[0]);
	if (rc < 0)
		return rc;

	return 0;
}

static inline void
_hfi_pt_write_fp_le_flags(u64 flags, union ptentry_fp1 *ptentry)
{
	union fp_le_options le_opts;

	le_opts.fxr_fp_op_put = !!(flags & PTL_OP_PUT);
	le_opts.fxr_fp_op_get = !!(flags & PTL_OP_GET);
	le_opts.fxr_fp_event_ct_comm = !!(flags & PTL_EVENT_CT_COMM);
	le_opts.fxr_fp_event_ct_bytes = !!(flags & PTL_EVENT_CT_BYTES);
	le_opts.fxr_fp_event_comm_disable = !!(flags & PTL_EVENT_COMM_DISABLE);
	le_opts.fxr_fp_event_success_disable =
			!!(flags & PTL_EVENT_SUCCESS_DISABLE);
	le_opts.fxr_fp_ack_disable = !!(flags & PTL_ACK_DISABLE);
	le_opts.fxr_fp_no_atomic = !!(flags & PTL_NO_ATOMIC);

	ptentry->le_2_0 = le_opts.val & 0x7;
	ptentry->le_7_3 = (le_opts.val >> 3) & 0x1F;
	ptentry->le_9_8 = (le_opts.val >> 8) & 0x3;
}

/* Initializes a fast PT entry at the given PT index. */
static inline int
_hfi_pt_fast(struct hfi_ctx *ctx, struct hfi_cmdq *rx_cq, u8 ni,
	     u32 pt_idx, struct hfi_pt_fast_args args)
{
	int rc;
	union ptentry_fp1 ptentry;

	/* GCC bug: can't use universal zero initializer with unions */
	ptentry.val[0] = 0;
	ptentry.val[1] = 0;
	ptentry.val[2] = 0;
	ptentry.val[3] = 0;

	_hfi_pt_write_fp_le_flags(args.flags, &ptentry);
	ptentry.user_id = ctx->ptl_uid;
	ptentry.length = args.length;
	ptentry.ct_handle = args.ct_handle;
	ptentry.eq_handle = args.eq_handle->idx;
	ptentry.start = args.start;
	ptentry.ni = ni;
	ptentry.fp = 1;
	ptentry.v = 1;

	rc = _hfi_pt_issue_cmd(ctx, rx_cq, PT_WRITE, ni, pt_idx,
			       &ptentry.val[0]);
	if (rc < 0)
		return rc;

	return 0;
}

/*
 * Attempt to allocate a fast PT entry at the requested PT index.  If the
 * requested PT index is already in use, the allocation will fail.
 */
static inline int
hfi_pt_alloc(struct hfi_ctx *ctx, struct hfi_cmdq *rx_cq, u8 ni,
	     u32 req_pt_idx, u32 *actual_pt_idx,
	     struct hfi_pt_args args)
{
	int rc;

	if (HFI_CTX_INVALID(ctx))
		return -ENODEV;
	if (!actual_pt_idx || !rx_cq || !args.eq_handle)
		return -EINVAL;
	if (req_pt_idx != HFI_PT_ANY && req_pt_idx >= HFI_NUM_PT_ENTRIES)
		return -EINVAL;
	if (ni >= HFI_NUM_NIS)
		return -EINVAL;

	if (args.eq_handle->idx >= HFI_NUM_NIS * HFI_NUM_EVENT_HANDLES)
		return -EINVAL;

	HFI_TS_MUTEX_LOCK(ctx);
	rc = _hfi_pt_alloc_ptidx(ctx, ni, req_pt_idx, actual_pt_idx);
	HFI_TS_MUTEX_UNLOCK(ctx);
	if (rc < 0)
		return rc;

	rc = _hfi_pt_normal(ctx, rx_cq, ni, *actual_pt_idx, args);
	if (rc)
		return rc;

	return 0;
}

static inline int
hfi_pt_fast_alloc(struct hfi_ctx *ctx, struct hfi_cmdq *rx_cq, u8 ni,
		  u32 req_pt_idx, u32 *actual_pt_idx,
		  struct hfi_pt_fast_args args)
{
	int rc;

	if (HFI_CTX_INVALID(ctx))
		return -ENODEV;
	if (!actual_pt_idx || !rx_cq || !args.eq_handle)
		return -EINVAL;
	if (req_pt_idx != HFI_PT_ANY && req_pt_idx >= HFI_NUM_PT_ENTRIES)
		return -EINVAL;
	if (ni != PTL_NONMATCHING_PHYSICAL && ni != PTL_NONMATCHING_LOGICAL)
		return -EINVAL;

	HFI_TS_MUTEX_LOCK(ctx);
	rc = _hfi_pt_alloc_ptidx(ctx, ni, req_pt_idx, actual_pt_idx);
	HFI_TS_MUTEX_UNLOCK(ctx);
	if (rc < 0)
		return rc;

	rc = _hfi_pt_fast(ctx, rx_cq, ni, *actual_pt_idx, args);
	if (rc)
		return rc;

	return 0;
}

/*
 * Converts a fast PT entry to a normal PT entry.  User must specify whether or
 * not to enable flow control on the new normal PT entry, as it is not optional
 * for fast PT entries.
 */
static inline int
hfi_pt_fast_to_normal(struct hfi_ctx *ctx, struct hfi_cmdq *rx_cq, u8 ni,
		      u32 pt_idx, bool flow_control)
{
	int rc;

	/*
	 * The PT_UPDATE_LOWER command has two parts: 16b payload and 16b mask;
	 * _hfi_pt_issue_cmd expects these to be contiguous, so create a 32b
	 * buffer.
	 */
	u64 buffer[4] = {0};
	union ptentry_fp0 *ptentryp = (union ptentry_fp0 *)&buffer[0];
	union ptentry_fp0 *maskp = (union ptentry_fp0 *)&buffer[2];

	if (HFI_CTX_INVALID(ctx))
		return -ENODEV;
	if (!rx_cq || pt_idx >= HFI_NUM_PT_ENTRIES || ni >= HFI_NUM_NIS)
		return -EINVAL;

	ptentryp->overflow_head = 0;
	ptentryp->overflow_tail = 0;
	ptentryp->unexpected_head = 0;
	ptentryp->fp = 0;
	ptentryp->enable = 1;
	ptentryp->fc = flow_control;
	ptentryp->v = 1;

	maskp->overflow_head = 0xFFFF;
	maskp->overflow_tail = 0xFFFF;
	maskp->unexpected_head = 0xFFFF;
	maskp->fp = 0x1;
	maskp->enable = 0x1;
	maskp->fc = 0x1;
	maskp->v = 0x1;

	rc = _hfi_pt_issue_cmd(ctx, rx_cq, PT_UPDATE_LOWER, ni, pt_idx, buffer);
	if (rc < 0)
		return rc;

	return 0;
}

/*
 * Converts a normal PT entry to a fast PT entry.  There's no way to convert a
 * normal PT entry to a fast PT entry without overwriting all of the settings,
 * so the user must specify them again as if they were allocating it for the
 * first time.
 */
static inline int
hfi_pt_normal_to_fast(struct hfi_ctx *ctx, struct hfi_cmdq *rx_cq, u8 ni,
		      u32 pt_idx, struct hfi_pt_fast_args args)
{
	int rc;

	if (HFI_CTX_INVALID(ctx))
		return -ENODEV;
	if (!rx_cq)
		return -EINVAL;
	if (pt_idx != HFI_PT_ANY && pt_idx >= HFI_NUM_PT_ENTRIES)
		return -EINVAL;
	if (ni != PTL_NONMATCHING_PHYSICAL && ni != PTL_NONMATCHING_LOGICAL)
		return -EINVAL;

	rc = _hfi_pt_fast(ctx, rx_cq, ni, pt_idx, args);
	if (rc)
		return rc;

	return 0;
}

static inline int
hfi_pt_free(struct hfi_ctx *ctx, struct hfi_cmdq *rx_cq, u8 ni,
	    u32 pt_idx, u64 flags)
{
	union ptentry_fp0 ptentry;
	int rc;

	if (HFI_CTX_INVALID(ctx))
		return -ENODEV;
	if (pt_idx == HFI_PT_ANY && pt_idx >= HFI_NUM_PT_ENTRIES)
		return -EINVAL;
	if (ni >= HFI_NUM_NIS)
		return -EINVAL;
	if (!ctx->pt_free_index[ni])
		return -ENOENT;

	ptentry.val[0] = 0;
	ptentry.val[1] = 0;
	ptentry.val[2] = ~0ULL;
	ptentry.val[3] = ~0ULL;
	if (rx_cq) {
		/* Issue the PT_UPDATE_LOWER command via the RX CQ */
		rc = _hfi_pt_issue_cmd(ctx, rx_cq, PT_UPDATE_LOWER, ni, pt_idx,
				       &ptentry.val[0]);
		if (rc < 0)
			return -EIO;
	}
	HFI_TS_MUTEX_LOCK(ctx);
	ctx->pt_free_list[ni][--ctx->pt_free_index[ni]] = pt_idx;
	HFI_TS_MUTEX_UNLOCK(ctx);
	return 0;
}

static inline
int hfi_me_alloc(struct hfi_ctx *ctx, u16 *me_handlep, u64 flags)
{
	u16 me_idx;
	int rc = 0;

	HFI_TS_MUTEX_LOCK(ctx);
	/*
	 * The LE/ME free list is essentially a stack.  To allocate, pop a
	 * handle off the list.  Index 0 is initially the top of the stack.
	 */
	if (ctx->le_me_free_index >= ctx->le_me_count - 1) {
		rc = -ENOSPC;
		goto out;
	}
	me_idx = ctx->le_me_free_list[ctx->le_me_free_index++];

	*me_handlep = me_idx;
out:
	HFI_TS_MUTEX_UNLOCK(ctx);
	return rc;
}

static inline
int hfi_me_free(struct hfi_ctx *ctx, u16 me_handle, u64 flags)
{
	HFI_TS_MUTEX_LOCK(ctx);
	ctx->le_me_free_list[--ctx->le_me_free_index] = me_handle;
	HFI_TS_MUTEX_UNLOCK(ctx);
	return 0;
}
#endif /* _KFI_PT_H */
