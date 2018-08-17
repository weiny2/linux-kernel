/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2018 Intel Corporation.
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
 * Copyright (c) 2018 Intel Corporation.
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
 * Intel(R) OPA Gen2 Verbs over Native provider
 */

#include "hfi2.h"
#include "hfi_kclient.h"
#include "hfi_tx_base.h"
#include "hfi_rx_vostlnp.h"
#include "native.h"

#define IB_REMOTE_FLAGS	(IB_ACCESS_REMOTE_WRITE | IB_ACCESS_REMOTE_READ | \
			 IB_ACCESS_REMOTE_ATOMIC)

/* TODO - Consider implementing new data structure to optimize this */
struct hfi_ctx *
hfi2_rkey_to_hw_ctx(struct hfi_ibcontext *ctx, uint32_t rkey)
{
	struct hfi_ctx *hw_ctx = NULL;
	int i;

	for (i = 0; i < ctx->num_ctx; i++) {
		if (RKEY_PID(rkey) == ctx->hw_ctx[i]->pid) {
			hw_ctx = ctx->hw_ctx[i];
			break;
		}
	}
	return hw_ctx;
}

int hfi2_alloc_lkey(struct rvt_mregion *mr, int acc_flags, bool dma_region)
{
	struct hfi_ibcontext *ctx = NULL;
	struct rvt_mregion *lkey_mr;
	u32 lkey, rkey;
	bool need_reserved_rkey;
	int ret;

	ctx = obj_to_ibctx(mr->pd);
	if (!ctx) {
		WARN_ON(!ctx);
		return -EINVAL;
	}

	if (ctx->supports_native) {
		/* Early memory registration needs HW context */
		ret = hfi2_add_initial_hw_ctx(ctx);
		/*
		 * If error, we have run out of HW contexts.
		 * So fallback to legacy transport.
		 */
		if (ret != 0)
			ctx->supports_native = false;
	}

	need_reserved_rkey = !ctx->is_user && !dma_region;

	if (dma_region) {
		/* Use a reserved LKEY for kernel DMA */
		lkey = 0;
		lkey_mr = NULL;
	} else {
		/* Allocate LKEY */
		lkey = hfi2_pop_key(&ctx->lkey_ks);
		if (lkey == INVALID_KEY)
			return -ENOMEM;
		lkey_mr = mr;
	}

	if (need_reserved_rkey || (acc_flags & IB_REMOTE_FLAGS)) {
		if (ctx->lkey_only) {
			rkey = lkey;
		} else {
			/* Allocate RKEY */
			rkey = hfi2_pop_key(&ctx->rkey_ks);
			if (rkey == INVALID_KEY) {
				hfi2_push_key(&ctx->lkey_ks, lkey);
				return -ENOMEM;
			}
		}
	} else {
		rkey = INVALID_KEY;
	}

	/* Setup MR */
	mr->lkey = lkey;
	mr->rkey = rkey;
	mr->access_flags = acc_flags;
	mr->lkey_published = (bool)lkey_mr;
	/* TODO - review locking of lkey_mr[] */
	ctx->lkey_mr[LKEY_INDEX(lkey)] = lkey_mr;

	return 0;
}

int hfi2_free_lkey(struct rvt_mregion *mr)
{
	struct hfi_ibcontext *ctx = obj_to_ibctx(mr->pd);
	struct hfi_ctx *rkey_ctx = NULL;
	unsigned long flags;
	int ret;
	u64 done = 0;

	/* Check LKEY */
	if (!ctx || LKEY_INDEX(mr->lkey) >= ctx->lkey_ks.num_keys)
		return -EINVAL;

	/* Return RKEY */
	if (!ctx->lkey_only && !IS_INVALID_KEY(mr->rkey))
		rkey_ctx = hfi2_rkey_to_hw_ctx(ctx, mr->rkey);
	if (rkey_ctx) {
		ret = hfi2_push_key(&ctx->rkey_ks, mr->rkey);
		/* If this fails we've somehow leaked an RKEY, but is freed */
		WARN_ON(ret != 0);

		spin_lock_irqsave(&ctx->cmdq->rx.lock, flags);
		ret = hfi_rkey_invalidate(&ctx->cmdq->rx, mr->rkey, (u64)&done);
		spin_unlock_irqrestore(&ctx->cmdq->rx.lock, flags);
		WARN_ON(ret != 0);

		ret = hfi_eq_poll_cmd_complete(rkey_ctx, &done);
		WARN_ON(ret != 0);
	}

	/* We skip free of the reserved LKEY for kernel DMA */
	if (mr->lkey != 0) {
		/* Cannot find MR after this */
		WARN_ON(!mr->lkey_published);
		ctx->lkey_mr[LKEY_INDEX(mr->lkey)] = NULL;
		/* TODO - review locking of lkey_mr[] */

		/* Return LKEY */
		ret = hfi2_push_key(&ctx->lkey_ks, RESET_LKEY(mr->lkey));
		/* If this fails we've somehow leaked an LKEY, but is freed */
		WARN_ON(ret != 0);

		/*
		 * Mark MR free, note there is some delay before rdmavt frees
		 * the MR completely, as it waits for refcount to reach 0.
		 */
		mr->lkey_published = 0;
	}
	return 0;
}

int hfi2_native_reg_mr(struct rvt_mregion *mr)
{
	struct hfi_ibcontext *ctx = obj_to_ibctx(mr->pd);
	struct hfi_ctx *rkey_ctx;
	union hfi_process pt;
	int ret;
	u32 me_options, pd_handle;
	u64 done = 0;
	unsigned long flags;

	if (!ctx || !ctx->supports_native || IS_INVALID_KEY(mr->rkey))
		return 0;

	/* for fast MR, wait until the work request */
	if (atomic_read(&mr->lkey_invalid))
		return 0;

	if (!mr->mapsz)
		return -EFAULT;

	pd_handle = to_pd_handle(mr->pd);

	me_options = PTL_ME_NO_TRUNCATE;
	if (mr->access_flags & IB_ACCESS_REMOTE_WRITE)
		me_options |= PTL_OP_PUT;
	if (mr->access_flags & IB_ACCESS_REMOTE_READ)
		me_options |= PTL_OP_GET;
	if (!(mr->access_flags & IB_ACCESS_REMOTE_ATOMIC))
		me_options |= PTL_NO_ATOMIC;
	pt.phys.slid = PTL_LID_ANY;
	pt.phys.ipid = PTL_PID_ANY;

	rkey_ctx = hfi2_rkey_to_hw_ctx(ctx, mr->rkey);
	if (!rkey_ctx)
		return -EINVAL;

	if ((mr->segs_per_map == mr->max_segs) &&
	    mr->map[0]->segs[0].length < mr->length) {
		int length = 0;
		int m = 0, n = 0;

		me_options |= PTL_IOVEC;
		while (m < mr->mapsz) {
			if (mr->map[m] && mr->map[m]->segs[n].vaddr) {
				mr->map[m]->segs[n].flags = IOVEC_VALID;
				length += mr->map[m]->segs[n].length;
				n++;
			}
			if (length >= mr->length)
				break;
			if (n >= RVT_SEGSZ) {
				m++;
				n = 0;
			}
		}
		spin_lock_irqsave(&ctx->cmdq->rx.lock, flags);
		ret = hfi_rkey_write(&ctx->cmdq->rx, NATIVE_NI,
				     (const void *)mr->iova,
				     (struct hfi_iovec *)mr->map[0], mr->length,
				     pt, rkey_ctx->ptl_uid,
				     pd_handle, mr->rkey, me_options,
				     HFI_CT_NONE, (u64)&done);
	} else {
		WARN_ON(mr->map[0]->segs[0].length < mr->length);
		spin_lock_irqsave(&ctx->cmdq->rx.lock, flags);
		ret = hfi_rkey_write(&ctx->cmdq->rx, NATIVE_NI,
				     (const void *)mr->iova,
				     mr->map[0]->segs[0].vaddr,
				     mr->length, pt, rkey_ctx->ptl_uid,
				     pd_handle, mr->rkey, me_options,
				     HFI_CT_NONE, (u64)&done);
	}
	spin_unlock_irqrestore(&ctx->cmdq->rx.lock, flags);
	if (ret)
		return ret;

	ret = hfi_eq_poll_cmd_complete(rkey_ctx, &done);
	return ret;
}

/*
 * Below routines Lookup the MR for a given LKEY or RKEY.
 * Used for legacy QP post_send or RDMA receive. We are required to increment
 * the MR refcount here.
 */
struct rvt_mregion *hfi2_find_mr_from_lkey(struct rvt_pd *pd, u32 lkey)
{
	struct hfi_ibcontext *ctx = obj_to_ibctx(&pd->ibpd);

	if (!ctx) {
		WARN_ON(!ctx);
		return NULL;
	}
	return _hfi2_find_mr_from_lkey(ctx, lkey, true);
}

struct rvt_mregion *hfi2_find_mr_from_rkey(struct rvt_pd *pd, u32 rkey)
{
	struct hfi_ibcontext *ctx = obj_to_ibctx(&pd->ibpd);

	if (!ctx) {
		WARN_ON(!ctx);
		return NULL;
	}

	if (ctx->lkey_only)
		return _hfi2_find_mr_from_lkey(ctx, rkey, true);
	else
		return _hfi2_find_mr_from_rkey(ctx, rkey);
}

struct rvt_mregion *_hfi2_find_mr_from_rkey(struct hfi_ibcontext *ctx, u32 rkey)
{
	struct rvt_mregion *mr = NULL;
	int i;

	for (i = 0; i < ctx->rkey_ks.num_keys; i++)
		if (ctx->lkey_mr[i] && ctx->lkey_mr[i]->rkey == rkey) {
			mr = ctx->lkey_mr[i];
			rvt_get_mr(mr);
			break;
		}
	return mr;
}
