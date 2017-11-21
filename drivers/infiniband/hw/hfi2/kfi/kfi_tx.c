/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2016 Intel Corporation.
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
 * Copyright (c) 2016 Intel Corporation.
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
 * Intel(R) Omni-Path Gen2 TX routines for KFI
 */

#include "../core/hfi_core.h"
#include "../core/hfi_cmdq.h"
#include "kfi_tx.h"

/*
 * DMA Command format
 * Used for DMA Put/Atomic/Get & Rendezvous Put/Atomic/Get
 */
union hfi_tx_cq_dma {
	struct {
		union tx_cq_base_put_flit0	flit0;
		union tx_cq_put_superset_flit1	flit1;
	};
	u64					command[8];
} __aligned(64);

/*
 * Union of all TX commands, add other formats as needed
 */
union hfi_tx_cq_command {
	union hfi_tx_cq_dma	dma;
	u64			command[16];
} __aligned(64);

/* Format a the common TX CQ command flit0 */
static
void _hfi_format_base_put_flit0(struct hfi_ctx *ctx, u8 ni,
				union tx_cq_base_put_flit0 *flit0,
				union ptl_cmd cmd,
				enum tx_ctype ctype,
				u16 cmd_length,
				union hfi_process target_id,
				u8 port, u32 pt_index,
				u8 rc, u8 sl,
				u8 becn, u16 pkey,
				u8 slid_low,
				u8 auth_idx,
				u64 user_ptr, bool hd,
				enum ptl_L4_ack_req ack_req,
				u32 md_options,
				struct hfi_eq *eq_handle,
				u16 ct_handle,
				size_t remote_offset,
				u16 tx_handle)
{
	flit0->a.val		= 0;
	flit0->a.pt		= port;
	flit0->a.cmd		= cmd.val;
	flit0->a.ptl_idx	= pt_index;
	flit0->a.rc		= rc;
	flit0->a.sl		= sl;
	flit0->a.b		= becn;
	/* Always request a short header - let HW decide */
	flit0->a.sh		= 1;
	flit0->a.ctype		= ctype;
	flit0->a.cmd_length	= cmd_length;
	flit0->a.dlid		= target_id.phys.slid;

	flit0->b.val		= 0;
	flit0->b.md_handle	= tx_handle;
	flit0->b.slid_low	= slid_low;
	flit0->b.auth_idx	= auth_idx;
	flit0->b.pkey		= pkey;
	flit0->b.eq_handle	= eq_handle->idx;
	flit0->b.ct_handle	= ct_handle;
	flit0->b.hd		= hd;
	flit0->b.md_options	= md_options;
	/* enable DLID table lookup (PD=0) if table was set and logical NI */
	flit0->b.pd	= !((ctx->mode & HFI_CTX_MODE_LID_VIRTUALIZED) &&
			    !HFI_PHYSICAL_NI(ni));

	flit0->c		= user_ptr;
	flit0->d.val		= 0;
	flit0->d.remote_offset	= remote_offset;
	flit0->d.ack_req	= ack_req;
	flit0->d.ni		= ni;
}

static
void _hfi_format_put_flit_e1(struct hfi_ctx *ctx,
			     union tx_cq_e1 *e,
			     u16 tpid,
			     enum ptl_datatype atomic_dtype,
			     size_t length)
{
	e->val			= 0;
	e->tpid			= tpid;
	e->ipid			= ctx->pid;
	e->atomic_dtype		= atomic_dtype;
	e->message_length	= length;
}

/* Format a TX CQ command for DMA Put/Atomic with Match Bits and Header Data */
static
int hfi_format_dma_match(struct hfi_ctx *ctx, u8 ni,
			 void *start, size_t length,
			 union hfi_process target_id, u8 port,
			 u32 pt_index, u8 rc, u8 sl,
			 u8 becn, u16 pkey,
			 u8 slid_low, u8 auth_idx,
			 u64 user_ptr, u64 match_bits,
			 u64 hdr_data, enum ptl_L4_ack_req ack_req,
			 u32 md_options,
			 struct hfi_eq *eq_handle,
			 u16 ct_handle, size_t remote_offset,
			 u16 tx_handle,
			 enum ptl_op_req op_req, enum tx_ctype ctype,
			 enum ptl_datatype atomic_dtype,
			 union hfi_tx_cq_dma *command)
{
	u16 cmd_length = _HFI_CMD_LENGTH(0, sizeof(union hfi_tx_cq_dma));
	u16 cmd_slots  = _HFI_CMD_SLOTS(cmd_length);
	union ptl_cmd cmd;

	cmd.ptl_opcode_low = op_req;
	cmd.ttype = DMA;

	_hfi_format_base_put_flit0(ctx, ni,
				   &command->flit0, cmd, ctype, cmd_length,
				   target_id, port, pt_index, rc, sl, becn,
				   pkey, slid_low, auth_idx, user_ptr, true,
				   ack_req, md_options, eq_handle, ct_handle,
				   remote_offset, tx_handle);

	_hfi_format_put_flit_e1(ctx, &command->flit1.e,
				target_id.phys.ipid, atomic_dtype, length);

	/* Just zero extend local_start to 64-bits  */
	command->flit1.f.val = (uintptr_t)start;

	command->flit1.mb = match_bits;
	command->flit1.hd = hdr_data;

	return cmd_slots;
}

static
int hfi_format_put_match(struct hfi_ctx *ctx, u8 ni,
			 void *start, size_t length, u16 mtu,
			 union hfi_process target_id, u8 port,
			 u32 pt_index, u8 rc, u8 sl,
			 u8 becn, u16 pkey,
			 u8 slid_low, u8 auth_idx,
			 u64 user_ptr, u64 match_bits,
			 u64 hdr_data, enum ptl_L4_ack_req ack_req,
			 u32 md_options,
			 struct hfi_eq *eq_handle,
			 u16 ct_handle, size_t remote_offset,
			 u16 tx_handle,
			 union hfi_tx_cq_command *command)
{
	enum tx_ctype ctype;

	/* Switch to RTS for very large Puts */
	ctype = (length > mtu) ? RTS : REQUEST;

	return hfi_format_dma_match(ctx, ni,
				    start, length,
				    target_id, port, pt_index,
				    rc, sl, becn, pkey, slid_low,
				    auth_idx, user_ptr,
				    match_bits, hdr_data,
				    ack_req, md_options,
				    eq_handle, ct_handle,
				    remote_offset, tx_handle,
				    PTL_REQ_PUT, ctype, 0,
				    &command->dma);
}

int hfi2_tx_write(struct hfi_cmdq *tx, struct hfi_ctx *ctx,
		  u8 ni, void *start, size_t length, u16 mtu,
		  union hfi_process target_id, u8 port,
		  u32 pt_index, u8 rc, u8 sl, u8 becn, u16 pkey,
		  u8 slid_low, u8 auth_idx, u64 user_ptr,
		  u64 match_bits, u64 hdr_data,
		  enum ptl_L4_ack_req ack_req, u32 md_options,
		  struct hfi_eq *eq_handle, u16 ct_handle,
		  size_t remote_offset, u16 tx_handle)
{
	union hfi_tx_cq_command cmd;
	int cmd_slots, ret;

	cmd_slots = hfi_format_put_match(ctx, ni,
					 start, length, mtu,
					 target_id, port,
					 pt_index, rc, sl,
					 becn, pkey,
					 slid_low, auth_idx,
					 user_ptr,
					 match_bits, hdr_data,
					 ack_req, md_options,
					 eq_handle, ct_handle,
					 remote_offset,
					 tx_handle, &cmd);

	do {
		ret = hfi_tx_command(tx, cmd.command, cmd_slots);
	} while (ret == -EAGAIN);

	return ret;
}
