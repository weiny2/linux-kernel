/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2017 Intel Corporation.
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
 * Copyright (c) 2017 Intel Corporation.
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
 *
 * Intel(R) Omni-Path Gen2 CT routines
 */
#ifndef _HFI_CT_H_
#define _HFI_CT_H_

static inline
u64 hfi_ct_get_success(struct hfi_ctx *ctx, u16 ct_handle)
{
	union ptl_ct_event *ct_array;
	volatile union ptl_ct_event *ct_entry;

	ct_array = (union ptl_ct_event *)ctx->ct_addr;
	ct_entry = (volatile union ptl_ct_event *)&ct_array[ct_handle];

	return ct_entry->success;
}

static inline
u64 hfi_ct_get_failure(struct hfi_ctx *ctx, u16 ct_handle)
{
	union ptl_ct_event *ct_array;
	volatile union ptl_ct_event *ct_entry;

	ct_array = (union ptl_ct_event *)ctx->ct_addr;
	ct_entry = (volatile union ptl_ct_event *)&ct_array[ct_handle];

	return ct_entry->failure;
}

static inline
int hfi_ct_wait_timed(struct hfi_ctx *ctx, u16 ct_h,
		      unsigned long threshold, unsigned int timeout_ms,
		      unsigned long *ct_val)
{
	unsigned long val;
	unsigned long exit_jiffies = jiffies + msecs_to_jiffies(timeout_ms);

	while (1) {
		if (hfi_ct_get_failure(ctx, ct_h))
			return -EFAULT;
		val = hfi_ct_get_success(ctx, ct_h);
		if (val >= threshold)
			break;
		if (time_after(jiffies, exit_jiffies))
			return -ETIME;
		cpu_relax();
	}

	if (ct_val)
		*ct_val = val;
	return 0;
}

static inline
int hfi_ct_alloc(struct hfi_ctx *ctx, u8 ni, u16 *ct_handlep)
{
	unsigned int ct_idx;
	struct opa_ev_assign ev_assign = {0};
	int ret;

	if (!ct_handlep)
		return -EINVAL;
	if (ni >= HFI_NUM_NIS)
		return -EINVAL;

	ev_assign.ni = ni;
	ev_assign.mode = OPA_EV_MODE_COUNTER;
	ret = ctx->ops->ev_assign(ctx, &ev_assign);
	ct_idx = ev_assign.ev_idx;
	if (ret < 0)
		return ret;

	/* Returns the absolute CT index */
	*ct_handlep = ct_idx;

	return 0;
}

static inline
int hfi_ct_free(struct hfi_ctx *ctx, u16 ct_handle)
{
	union ptl_ct_event *ct_array;
	int ret;

	if (ct_handle == 0 || ct_handle >= HFI_NUM_NIS * HFI_NUM_CT_ENTRIES)
		return -EINVAL;

	ct_array = (union ptl_ct_event *)ctx->ct_addr;
	/* Check that the requested CT entry is valid */
	if (ct_array[ct_handle].v != 1)
		return -EINVAL;

	/* Clear the CT entry */
	ret = ctx->ops->ev_release(ctx, OPA_EV_MODE_COUNTER, ct_handle, 0);
	if (ret < 0)
		return ret;

	return 0;
}
#endif
