/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2015-2017 Intel Corporation.
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
 * Copyright (c) 2015-2017 Intel Corporation.
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

#ifndef _HFI_TX_BYPASS_H
#define _HFI_TX_BYPASS_H

#include "hfi_cmdq.h"

/* TODO Need CTYPEs. This avoids camel-case error from NonPortalsMsg */
#define HFI_CTYPE_BYPASS 6

/*
 * IB DMA
 */
struct tx_cq_ib_dma_flit0 {
	union tx_cq_a3	a3; /* 9B and 16B */
	union tx_cq_a4	a4; /* 16B only */
	u64		bthgrh[2];
};

union hfi_tx_ib_dma {
	struct {
		struct tx_cq_ib_dma_flit0	flit0;
		u64 eth[8]; /* opaque headers */
		union tx_cq_ofed_dma_flit3	flit3;
	};
	u64	command[16];
} __aligned(64);

union hfi_tx_general_dma {
	struct {
		union tx_cq_general_dma_flit0	flit0;
		union tx_cq_general_dma_flit1	flit1;
	};
	u64	command[8];
} __aligned(64);

/* Format the non-portals TX CMDQ command flit0 A3 */
static inline
void _hfi_format_nonptl_flit0_a3(struct hfi_ctx *ctx,
				 union tx_cq_a3 *flit0_a,
				 u8 l2, u8 cmd, u16 cmd_length,
				 u8 port, u8 sl, u8 slid_low,
				 u8 auth_idx, u8 becn,
				 u8 hdr_rate)
{
	flit0_a->val		= 0;
	flit0_a->pt		= port;
	flit0_a->cmd		= cmd;
	flit0_a->slid_low	= slid_low;
	flit0_a->auth_idx	= auth_idx; /* KDETH only */
	flit0_a->sl		= sl;
	flit0_a->l2		= l2;
	flit0_a->ctype		= HFI_CTYPE_BYPASS;
	flit0_a->cmd_length	= cmd_length;
	flit0_a->nonportals_pid = ctx->pid - HFI_PID_BYPASS_BASE;
	flit0_a->b		= becn;
	flit0_a->hdr_rate	= hdr_rate;
	/* caller can set RC and PER_PKT_DELAY if needed */
}

/* Format the non-portals TX CMDQ command flit0 A4 */
static inline
void _hfi_format_nonptl_flit0_a4(struct hfi_ctx *ctx,
				 union tx_cq_a4 *flit0_a,
				 u32 dlid, u16 length,
				 u8 l4, u16 entropy)
{
	flit0_a->val     = 0;
	flit0_a->dlid    = dlid;
	flit0_a->length  = length;
	flit0_a->l4      = l4;
	flit0_a->entropy = entropy;
}

static inline
void _hfi_format_ib_dma_flit3(struct hfi_ctx *ctx,
			      union tx_cq_ofed_dma_flit3 *flit3,
			      u8 eth_size, u8 mtu_id,
			      u64 start, u32 len,
			      u32 md_options,
			      struct hfi_eq *eq_handle,
			      u64 user_ptr)
{
	flit3->p.ethsize	= eth_size;
	flit3->p.user_mtu	= mtu_id;
	flit3->p.first_pkt_size	= 0;
	flit3->f.local_start	= start;
	flit3->user_ptr		= user_ptr;
	flit3->e.tid_entries	= 0;
	flit3->e.eq_handle	= eq_handle->idx;
	flit3->e.md_options	= md_options;
	flit3->e.message_length	= len;
}

static inline
int hfi_format_bypass_dma_cmd(struct hfi_ctx *ctx, u64 iov_base, u32 num_iovs,
			      u64 user_ptr, u32 md_opts,
			      struct hfi_eq *eq_handle,
			      u16 ct_handle, u8 port, u8 sl,
			      u8 slid_low, u8 l2, u8 dma_cmd,
			      union hfi_tx_general_dma *cmd)
{
	/* reuse function to write A3, then fill in other flit info */
	_hfi_format_nonptl_flit0_a3(ctx, &cmd->flit0.a, l2, dma_cmd,
				    5 /* 8B flits */, port, sl, slid_low,
				    0, 0, 0);
	cmd->flit0.b.val = 0;
	cmd->flit0.b.ct_handle = ct_handle;
	cmd->flit0.c = user_ptr;
	cmd->flit0.e.tid_entries = 1;
	cmd->flit0.e.eq_handle = eq_handle->idx;
	cmd->flit0.e.md_options = md_opts;
	cmd->flit0.e.message_length = num_iovs;
	cmd->flit1.f.local_start = iov_base;

	return 1;
}

#endif /* _HFI_TX_BYPASS_H */
