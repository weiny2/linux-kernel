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

#include "native.h"

/* TODO - move this to header with HW defines */
#define	RKEY_PER_PID		4096

/*
 * RKEY:
 * upper 24-bits are hardware index
 * lower 8-bits are salt bits or consumer key
 */
#define BUILD_RKEY(list_index, pid, salt) \
	(((u64)(pid) & 0xfff) << 20 | \
	 ((u64)(list_index) & 0xfff) << 8 | \
	 ((u64)(salt) & 0xff) << 0)

/*
 * LKEY:
 * upper 24-bits are private index
 * lower 8-bits are reserved for consumer key
 */
#define BUILD_LKEY(index) \
	(((u64)(index) & 0xffffff) << 8)

/* Append new set of keys to LKEY and RKEY stacks */
static int hfi2_add_keys(struct hfi_ibcontext *ctx, struct hfi_ctx *hw_ctx)
{
	int i;
	u32 key_idx, extra_keys, *new_keys;
	struct rvt_mregion **new_mr;

	/* TODO - driver managed RKEYs not fully implemented yet */
	if (hw_ctx && !ctx->lkey_only) {
		/* Append new keys to RKEY Stack */
		mutex_lock(&ctx->rkey_ks.lock);
		new_keys = krealloc(ctx->rkey_ks.free_keys,
				    (ctx->rkey_ks.num_keys + RKEY_PER_PID) *
				    sizeof(u32), GFP_KERNEL);
		if (!new_keys) {
			mutex_unlock(&ctx->rkey_ks.lock);
			return -ENOMEM;
		}
		ctx->rkey_ks.free_keys = new_keys;

		for (i = 0; i < RKEY_PER_PID; ++i)
			ctx->rkey_ks.free_keys[ctx->rkey_ks.num_keys++] =
				BUILD_RKEY(i, hw_ctx->pid, 0);
		mutex_unlock(&ctx->rkey_ks.lock);
	}

	/* Append entries to LKEY Stack + MR Array */
	mutex_lock(&ctx->lkey_ks.lock);
	extra_keys = RKEY_PER_PID * 2;
	new_keys = krealloc(ctx->lkey_ks.free_keys,
			    (ctx->lkey_ks.num_keys + extra_keys) *
			    sizeof(u32), GFP_KERNEL);
	if (!new_keys) {
		mutex_unlock(&ctx->rkey_ks.lock);
		return -ENOMEM;
	}
	ctx->lkey_ks.free_keys = new_keys;

	new_mr = krealloc(ctx->lkey_mr, (ctx->lkey_ks.num_keys + extra_keys) *
			  sizeof(void *), GFP_KERNEL);
	if (!new_mr) {
		mutex_unlock(&ctx->lkey_ks.lock);
		return -ENOMEM;
	}
	ctx->lkey_mr = new_mr;

	/* hold back LKEY == 0, which is a reserved LKEY */
	if (ctx->lkey_ks.num_keys == 0) {
		ctx->lkey_ks.num_keys = 1;
		ctx->lkey_ks.key_head = 1;
		ctx->lkey_mr[0] = NULL;
		extra_keys--;
	}

	for (i = 0; i < extra_keys; ++i) {
		key_idx = ctx->lkey_ks.num_keys + i;
		ctx->lkey_ks.free_keys[key_idx] = BUILD_LKEY(key_idx);
		ctx->lkey_mr[key_idx] = NULL;
	}
	ctx->lkey_ks.num_keys += extra_keys;
	mutex_unlock(&ctx->lkey_ks.lock);

	return 0;
}

static void hfi2_free_all_keys(struct hfi_ibcontext *ctx)
{
	kfree(ctx->rkey_ks.free_keys);
	kfree(ctx->lkey_ks.free_keys);
	kfree(ctx->lkey_mr);
	ctx->rkey_ks.free_keys = NULL;
	ctx->lkey_ks.free_keys = NULL;
	ctx->lkey_mr = NULL;
}

void hfi2_native_alloc_ucontext(struct hfi_ibcontext *ctx, void *udata,
				bool is_enabled)
{
	/* Init key stacks */
	ctx->rkey_ks.key_head = 0;
	ctx->lkey_ks.key_head = 0;
	mutex_init(&ctx->rkey_ks.lock);
	mutex_init(&ctx->lkey_ks.lock);

	ctx->is_user = !!udata;
	/* Only enable native Verbs for kernel clients */
	ctx->supports_native = is_enabled && !ctx->is_user;
	if (ctx->supports_native) {
#if 0 /* TODO */
		/* Early memory registration needs HW context */
		ret = hfi2_add_initial_hw_ctx(ctx);
		/*
		 * If error, we have run out of HW contexts.
		 * So fallback to legacy transport.
		 */
		if (ret != 0)
#endif
			ctx->supports_native = false;
	}

	/* Use single key when not native Verbs */
	ctx->lkey_only = !ctx->supports_native;
	if (ctx->lkey_only)
		hfi2_add_keys(ctx, NULL);
}

void hfi2_native_dealloc_ucontext(struct hfi_ibcontext *ctx)
{
	hfi2_free_all_keys(ctx);
}
