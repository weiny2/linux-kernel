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
#include "native.h"

#define IB_REMOTE_FLAGS	(IB_ACCESS_REMOTE_WRITE | IB_ACCESS_REMOTE_READ | \
			 IB_ACCESS_REMOTE_ATOMIC)

#define LKEY_TO_INDEX(lkey)	(((lkey) >> 8) & 0xFFFFFF)

int hfi2_alloc_lkey(struct rvt_mregion *mr, int acc_flags, bool dma_region)
{
	struct hfi_ibcontext *ctx = NULL;
	u32 lkey, rkey;

	if (dma_region) {
		/* Use a reserved LKEY for kernel DMA */
		lkey = 0;
	} else {
		ctx = ibpd_to_ibctx(mr->pd);
		if (!ctx) {
			WARN_ON(!ctx);
			return -EINVAL;
		}

		/* Allocate LKEY */
		lkey = hfi2_pop_key(&ctx->lkey_ks);
		if (lkey == INVALID_KEY)
			return -ENOMEM;
	}

	if (acc_flags & IB_REMOTE_FLAGS) {
		/* TODO - doesn't yet support RKEY for DMA_MR */
		if (!ctx || ctx->lkey_only) {
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
	mr->lkey_published = 1;
	/* TODO - review locking of lkey_mr[] */
	if (ctx)
		ctx->lkey_mr[LKEY_TO_INDEX(lkey)] = mr;
	return 0;
}

int hfi2_free_lkey(struct rvt_mregion *mr)
{
	struct hfi_ibcontext *ctx = ibpd_to_ibctx(mr->pd);
	int ret;

	if (!ctx) {
		if (mr->lkey)
			return -EINVAL;
		/* This is the reserved LKEY for kernel DMA */
		/* TODO - free RKEY when implemented */
		goto done;
	}

	/* Check LKEY */
	if (LKEY_TO_INDEX(mr->lkey) >= ctx->lkey_ks.num_keys)
		return -EINVAL;

	/* Return RKEY */
	if (!ctx->lkey_only && mr->rkey != INVALID_KEY) {
		ret = hfi2_push_key(&ctx->rkey_ks, mr->rkey);
		if (ret != 0)
			return ret;
		mr->rkey = INVALID_KEY;
	}

	/* Cannot find MR after this */
	WARN_ON(!mr->lkey_published);
	ctx->lkey_mr[LKEY_TO_INDEX(mr->lkey)] = NULL;
	/* TODO - review locking of lkey_mr[] */

	/* Return LKEY */
	ret = hfi2_push_key(&ctx->lkey_ks, mr->lkey);
	/* If this fails we've somehow leaked an LKEY, but is freed */
	WARN_ON(ret != 0);

done:
	/*
	 * Mark MR free, note there is some delay before rdmavt frees
	 * the MR completely, as it waits for refcount to reach 0.
	 */
	mr->lkey_published = 0;
	return 0;
}

/*
 * Below routines Lookup the MR for a given LKEY or RKEY.
 * Used for legacy QP post_send or RDMA receive. We are required to increment
 * the MR refcount here.
 */
struct rvt_mregion *hfi2_find_mr_from_lkey(struct rvt_pd *pd, u32 lkey)
{
	struct hfi_ibcontext *ctx = ibpd_to_ibctx(&pd->ibpd);
	struct rvt_mregion *mr = NULL;
	u32 key_idx;

	if (lkey == 0) {
		/* return MR for the reserved LKEY */
		rcu_read_lock();
		mr = rcu_dereference(ib_to_rvt(pd->ibpd.device)->dma_mr);
		if (mr)
			rvt_get_mr(mr);
		rcu_read_unlock();
	} else {
		if (!ctx) {
			WARN_ON(!ctx);
			return NULL;
		}
		key_idx = LKEY_TO_INDEX(lkey);
		if (key_idx >= ctx->lkey_ks.num_keys)
			return NULL;
		/* TODO - review locking of lkey_mr[] */
		mr = ctx->lkey_mr[key_idx];
		if (mr)
			rvt_get_mr(mr);
	}

	return mr;
}

struct rvt_mregion *hfi2_find_mr_from_rkey(struct rvt_pd *pd, u32 lkey)
{
	struct hfi_ibcontext *ctx = ibpd_to_ibctx(&pd->ibpd);

	/* TODO - don't yet support unique RKEY (from LKEY) */
	if (!ctx || ctx->lkey_only)
		return hfi2_find_mr_from_lkey(pd, lkey);
	else
		return NULL;
}
