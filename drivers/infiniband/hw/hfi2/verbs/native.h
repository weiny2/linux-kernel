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
 * Intel(R) OPA Gen2 IB Driver
 */

#ifndef NATIVE_VERBS_H
#define NATIVE_VERBS_H

#include <rdma/ib_verbs.h>
#include <rdma/rdma_vt.h>
#include "verbs.h"

#define RESET_LKEY(key)	((key) & 0xFFFFFF00)
#define LKEY_INDEX(key)	(((key) >> 8) & 0xFFFFFF)
#define INVALID_KEY	0xffffffff
#define INVALID_KEY_IDX	0xffffff00
#define IS_INVALID_KEY(key) (RESET_LKEY(key) == INVALID_KEY_IDX)

static inline
struct hfi_ibcontext *ibpd_to_ibctx(struct ib_pd *obj)
{
	return (struct hfi_ibcontext *)
		(obj->uobject ? obj->uobject->context :
				to_hfi_ibd(obj->device)->ibkc);
}

static inline
bool hfi2_ks_full(struct hfi_ks *ks)
{
	return (ks->key_head == 0);
}

static inline
bool hfi2_ks_empty(struct hfi_ks *ks)
{
	return (ks->key_head >= ks->num_keys);
}

static inline
u32 hfi2_pop_key(struct hfi_ks *ks)
{
	u32 key = INVALID_KEY;

	mutex_lock(&ks->lock);
	if (hfi2_ks_empty(ks))
		goto pop_exit;
	key = ks->free_keys[ks->key_head++];
pop_exit:
	mutex_unlock(&ks->lock);
	return key;
}

static inline
int hfi2_push_key(struct hfi_ks *ks, u32 key)
{
	int ret = 0;

	mutex_lock(&ks->lock);
	if (hfi2_ks_full(ks)) {
		ret = -EFAULT;
		goto push_exit;
	}
	ks->free_keys[--ks->key_head] = key;
push_exit:
	mutex_unlock(&ks->lock);
	return ret;
}

static inline
struct rvt_mregion *_hfi2_find_mr_from_lkey(struct hfi_ibcontext *ctx, u32 lkey,
					    bool inc)
{
	struct rvt_mregion *mr = NULL;
	u32 key_idx;

	if (lkey == 0) {
		/* return MR for the reserved LKEY */
		rcu_read_lock();
		mr = rcu_dereference(ib_to_rvt(ctx->ibuc.device)->dma_mr);
		if (mr && inc)
			rvt_get_mr(mr);
		rcu_read_unlock();
	} else {
		if (!ctx) {
			WARN_ON(!ctx);
			return NULL;
		}
		key_idx = LKEY_INDEX(lkey);
		if (key_idx >= ctx->lkey_ks.num_keys)
			return NULL;
		/* TODO - review locking of lkey_mr[] */
		mr = ctx->lkey_mr[key_idx];
		if (mr && inc)
			rvt_get_mr(mr);
	}

	return mr;
}

static inline
struct rvt_mregion *hfi2_chk_mr_sge(struct hfi_ibcontext *ctx,
				    struct ib_sge *sge)
{
	struct rvt_mregion *mr;

	/* TODO - pass false for last argument.
	 * Legacy QPs will reference count the MR, but this doesn't
	 * seem necessary for native transport.... revisit.
	 */
	mr = _hfi2_find_mr_from_lkey(ctx, sge->lkey, false);
#if 0
	if (!mr || (mr->lkey != sge->lkey) ||
	    (sge->addr < (u64)mr->user_base) ||
	    (sge->addr + sge->length > ((u64)mr->user_base) + mr->length))
		return NULL;
#endif
	return mr;
}

#ifndef CONFIG_HFI2_STLNP
#define hfi2_native_alloc_ucontext(ctx, udata, is_enabled)
#define hfi2_native_dealloc_ucontext(ctx)
#else
void hfi2_native_alloc_ucontext(struct hfi_ibcontext *ctx, void *udata,
				bool is_enabled);
void hfi2_native_dealloc_ucontext(struct hfi_ibcontext *ctx);
int hfi2_alloc_lkey(struct rvt_mregion *mr, int acc_flags, bool dma_region);
int hfi2_free_lkey(struct rvt_mregion *mr);
struct rvt_mregion *hfi2_find_mr_from_lkey(struct rvt_pd *pd, u32 lkey);
struct rvt_mregion *hfi2_find_mr_from_rkey(struct rvt_pd *pd, u32 rkey);
#endif
#endif /* NATIVE_VERBS_H */
