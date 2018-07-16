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

#ifndef _HFI_TX_VOSTLNP_H
#define _HFI_TX_VOSTLNP_H

#include "hfi_tx_base.h"

#define FMT_SYNC_EQ_HD(opcode, ipid, src_qpn, dst_qp) \
	((((u64)opcode & 0xf) << 60) | \
	(((u64)ipid & 0xfff) << 48) | \
	(((u64)src_qpn & 0xffffff) << 24) | \
	((dst_qp & 0xffffff) << 0))
#define SYNC_EQ_OPCODE(hdr_data)	((hdr_data >> 60) & 0xf)
#define SYNC_EQ_IPID(hdr_data)		((hdr_data >> 48) & 0xfff)
#define SYNC_EQ_SRC_QPN(hdr_data)	((hdr_data >> 24) & 0xffffff)

/* Update the common TX CQ command flit0 */
static inline
void _hfi_update_verb_flit0_rc(union tx_cq_base_put_flit0 *flit0,
				hfi_cmd_t cmd,
				u16 cmd_length,
				u8 force_frag,
				hfi_rc_t rc,
				hfi_user_ptr_t user_ptr,
				hfi_md_options_t md_options,
				hfi_eq_handle_t eq_handle,
				hfi_ct_handle_t ct_handle,
				hfi_size_t remote_offset,
				hfi_tx_handle_t tx_handle)
{
	flit0->a.pt		= force_frag;
	flit0->a.cmd		= cmd.val;
	flit0->a.rc		= rc;
	flit0->a.cmd_length 	= cmd_length;
	flit0->b.md_handle 	= tx_handle;
	flit0->b.ct_handle	= ct_handle;
	flit0->b.md_options 	= md_options;
	flit0->b.eq_handle	= eq_handle->idx; /* resize_cq requires */
	flit0->c	 	= user_ptr;
	flit0->d.remote_offset 	= remote_offset;
}

static inline
int hfi_format_buff_vostlnp(struct hfi_ctx *ctx,
			    const void *start, hfi_size_t length,
			    hfi_tx_ctype_t ctype,
			    u32 dlid, u8 force_frag,
			    hfi_rc_t rc,
			    hfi_service_level_t sl,
			    hfi_pkey_t pkey, hfi_slid_low_t slid_low,
			    hfi_auth_idx_t auth_idx, hfi_user_ptr_t user_ptr,
			    hfi_match_bits_t match_bits,
			    hfi_hdr_data_t hdr_data,
			    hfi_md_options_t md_options,
			    hfi_eq_handle_t eq_handle,
			    hfi_ct_handle_t ct_handle,
			    hfi_size_t remote_offset,
			    hfi_tx_handle_t tx_handle,
			    hfi_op_req_t op,
			    u32 dst_qp,
			    union hfi_tx_cq_buff_put_match *command)
{
	union hfi_process target_id = {.phys.slid = dlid};
	u16 cmd_length = _HFI_CMD_LENGTH(length, offsetof(hfi_tx_cq_buff_put_match_t, flit1.p0));
	u16 cmd_slots  = _HFI_CMD_SLOTS(cmd_length);
	hfi_cmd_t cmd;
	cmd.ptl_opcode_low = op;
	cmd.ttype = BUFFERED;

	_hfi_format_verb_put_flit0(&command->flit0, cmd, ctype, cmd_length,
				   target_id, force_frag, rc, sl,
				   pkey, slid_low, auth_idx, user_ptr,
				   md_options, eq_handle, ct_handle,
				   remote_offset, tx_handle, dst_qp);

	_hfi_format_put_flit_e1(ctx, &command->flit1.e, (dst_qp>>8), PTL_UINT64_T, length);

	command->flit1.mb = match_bits;
	command->flit1.hd = hdr_data;

	/* Copy the user payload into the command */
	_hfi_memcpy(&command->flit1.p0, start, length);

	return cmd_slots;
}

static inline
int hfi_update_buff_rc(struct hfi_ctx *ctx,
		       const void *start, hfi_size_t length,
		       u8 force_frag,
		       hfi_rc_t rc,
		       hfi_user_ptr_t user_ptr,
		       hfi_match_bits_t match_bits,
		       hfi_hdr_data_t hdr_data,
		       hfi_md_options_t md_options,
		       hfi_eq_handle_t eq_handle,
		       hfi_ct_handle_t ct_handle,
		       hfi_size_t remote_offset,
		       hfi_tx_handle_t tx_handle,
		       hfi_op_req_t op,
		       u32 qp,
		       union hfi_tx_cq_buff_put_match *command)
{
	u16 cmd_length = _HFI_CMD_LENGTH(length, offsetof(hfi_tx_cq_buff_put_match_t, flit1.p0));
	u16 cmd_slots  = _HFI_CMD_SLOTS(cmd_length);
	hfi_cmd_t cmd;
	cmd.ptl_opcode_low = op;
	cmd.ttype = BUFFERED;

	_hfi_update_verb_flit0_rc(&command->flit0, cmd, cmd_length,
				  force_frag, rc, user_ptr,
				  md_options, eq_handle, ct_handle,
				  remote_offset, tx_handle);

	_hfi_update_put_flit_e1(&command->flit1.e, length);

	command->flit1.mb = match_bits;
	command->flit1.hd = hdr_data;

	/* Copy the user payload into the command */
	_hfi_memcpy(&command->flit1.p0, start, length);

	return cmd_slots;
}

static inline
int hfi_format_buff_ud(struct hfi_ctx *ctx,
		       const void *start, hfi_size_t length,
		       u32 dlid, u8 force_frag,
		       hfi_rc_t rc,
		       hfi_service_level_t sl,
		       hfi_pkey_t pkey, hfi_slid_low_t slid_low,
		       hfi_auth_idx_t auth_idx, hfi_user_ptr_t user_ptr,
		       u32 src_qp,
		       u32 dst_qp,
		       u32 imm,
		       u32 qkey,
		       hfi_md_options_t md_options,
		       hfi_eq_handle_t eq_handle,
		       hfi_ct_handle_t ct_handle,
		       hfi_tx_handle_t tx_handle,
		       hfi_op_req_t op,
		       u8 solicit, u8 mb_opcode,
		       union hfi_tx_cq_buff_put_match *command)
{
	hfi_match_bits_t match_bits = FMT_VOSTLNP_MB(mb_opcode, src_qp, qkey);
	hfi_hdr_data_t hdr_data = FMT_VOSTLNP_HD(imm, solicit, dst_qp);
	u64 remote_offset = FMT_UD_REMOTE_OFFSET(dlid, sl, pkey, src_qp);

	return hfi_format_buff_vostlnp(ctx, start, length,
				       (hfi_tx_ctype_t)UD, dlid,
				       force_frag, rc, sl, pkey, slid_low,
				       auth_idx, user_ptr, match_bits, hdr_data,
				       md_options, eq_handle, ct_handle,
				       (hfi_size_t)remote_offset, tx_handle, op,
				       dst_qp, command);
}

static inline
int hfi_format_buff_sync_eq(struct hfi_ctx *ctx,
			    u32 dlid,
			    hfi_rc_t rc,
			    hfi_service_level_t sl,
			    hfi_pkey_t pkey, hfi_slid_low_t slid_low,
			    hfi_auth_idx_t auth_idx,
			    hfi_user_ptr_t user_ptr,
			    u32 src_qp,
			    u32 dst_qp,
			    u8 hd_opcode,
			    hfi_eq_handle_t eq_handle,
			    hfi_tx_handle_t tx_handle,
			    hfi_op_req_t op,
			    union hfi_tx_cq_buff_put_match *command)
{
	hfi_match_bits_t match_bits = FMT_VOSTLNP_MB(MB_OC_EXCEPTION, src_qp, 0);
	hfi_hdr_data_t hdr_data = FMT_SYNC_EQ_HD(hd_opcode, ctx->pid,
						 src_qp, dst_qp);

	return hfi_format_buff_vostlnp(ctx, (void*)0, 0,
				       (hfi_tx_ctype_t)VoNP_RC, dlid,
				       0, rc, sl, pkey, slid_low,
				       auth_idx, user_ptr,
				       match_bits, hdr_data,
				       PTL_MD_EVENT_SUCCESS_DISABLE,
				       eq_handle, PTL_CT_NONE,
				       (hfi_size_t)0, tx_handle, op,
				       dst_qp, command);
}

static inline
int hfi_format_buff_rc_send(struct hfi_ctx *ctx,
			    const void *start, hfi_size_t length,
			    u8 force_frag,
			    hfi_rc_t rc,
			    hfi_user_ptr_t user_ptr,
			    u32 src_qp,
			    u32 dst_qp,
			    u32 imm,
			    hfi_md_options_t md_options,
			    hfi_eq_handle_t eq_handle,
			    hfi_ct_handle_t ct_handle,
			    hfi_tx_handle_t tx_handle,
			    hfi_op_req_t op,
			    u8 solicit,
			    union hfi_tx_cq_buff_put_match *command)
{
	hfi_match_bits_t match_bits = FMT_VOSTLNP_MB(MB_OC_OK, src_qp, imm);
	hfi_hdr_data_t hdr_data = FMT_VOSTLNP_HD(imm, solicit, dst_qp);

	return hfi_update_buff_rc(ctx, start, length,
				  force_frag, rc,
				  user_ptr, match_bits, hdr_data,
				  md_options, eq_handle, ct_handle,
				  (hfi_size_t)0, tx_handle, op, src_qp,
				  command);
}

static inline
int hfi_format_buff_rc_rdma_atomic(struct hfi_ctx *ctx,
			    const void *start,
			    u8 force_frag,
			    hfi_rc_t rc,
			    hfi_user_ptr_t user_ptr,
			    u32 src_qp,
			    u32 dst_qp,
			    u32 imm,
			    u32 rkey,
			    hfi_md_options_t md_options,
			    hfi_eq_handle_t eq_handle,
			    hfi_ct_handle_t ct_handle,
			    hfi_size_t remote_offset,
			    hfi_tx_handle_t tx_handle,
			    hfi_op_req_t op,
			    u8 solicit,
			    const void *operand0,
			    const void *operand1,
			    hfi_tx_cq_buff_toa_fetch_match_t *command)
{
	hfi_match_bits_t match_bits = FMT_VOSTLNP_MB(MB_OC_OK, src_qp, rkey);
	hfi_hdr_data_t hdr_data = FMT_VOSTLNP_HD(imm, solicit, dst_qp);
	u16 cmd_length = _HFI_CMD_LENGTH(operand1 ? 16 : 8,
					      offsetof(hfi_tx_cq_buff_toa_fetch_match_t, payload));
	u16 cmd_slots  = _HFI_CMD_SLOTS(cmd_length);
	hfi_cmd_t cmd;
	cmd.ptl_opcode_low = op;
	cmd.ttype = BUFFERED;

	_hfi_update_verb_flit0_rc(&command->flit0, cmd, cmd_length,
				  force_frag, rc, user_ptr,
				  md_options, eq_handle, ct_handle,
				  remote_offset, tx_handle);

	_hfi_update_put_flit_e2((union tx_cq_e2*)&command->flit1.e, 8);

	command->flit1.f.val = (uintptr_t)start; /* Just zero extend local_start to 64-bits  */

	command->flit1.mb = match_bits;
	command->flit1.hd = hdr_data;

	if (operand1) {
		/* The Operand comes before the regular Payload if this is a TOA */
		_hfi_memcpy(&command->payload, operand0, 8);
		_hfi_memcpy((u64 *)((uintptr_t)&command->payload + 8), operand1, 8);
	} else {
		/* Copy the Payload into the command */
		_hfi_memcpy(&command->payload, operand0, 8);
	}

	return cmd_slots;
}

static inline
int hfi_format_buff_rc_rdma(struct hfi_ctx *ctx,
			    const void *start, hfi_size_t length,
			    u8 force_frag,
			    hfi_rc_t rc,
			    hfi_user_ptr_t user_ptr,
			    u32 src_qp,
			    u32 dst_qp,
			    u32 imm,
			    u32 rkey,
			    hfi_md_options_t md_options,
			    hfi_eq_handle_t eq_handle,
			    hfi_ct_handle_t ct_handle,
			    hfi_size_t remote_offset,
			    hfi_tx_handle_t tx_handle,
			    hfi_op_req_t op,
			    u8 solicit,
			    union hfi_tx_cq_buff_put_match *command)
{
	hfi_match_bits_t match_bits = FMT_VOSTLNP_MB(MB_OC_OK, src_qp, rkey);
	hfi_hdr_data_t hdr_data = FMT_VOSTLNP_HD(imm, solicit, dst_qp);

	return hfi_update_buff_rc(ctx, start, length,
				  force_frag, rc,
				  user_ptr, match_bits, hdr_data,
				  md_options, eq_handle, ct_handle,
				  remote_offset, tx_handle, op, src_qp,
				  command);
}

static inline
int hfi_format_pio_vostlnp(struct hfi_ctx *ctx,
			   void *start, hfi_size_t length,
			   hfi_tx_ctype_t ctype,
			   u32 dlid, u8 force_frag,
			   hfi_rc_t rc,
			   hfi_service_level_t sl,
			   hfi_pkey_t pkey, hfi_slid_low_t slid_low,
			   hfi_auth_idx_t auth_idx,
			   hfi_user_ptr_t user_ptr,
			   hfi_match_bits_t match_bits,
			   hfi_hdr_data_t hdr_data,
			   hfi_md_options_t md_options,
			   hfi_eq_handle_t eq_handle,
			   hfi_ct_handle_t ct_handle,
			   hfi_size_t remote_offset,
			   hfi_tx_handle_t tx_handle,
			   hfi_op_req_t op_req,
			   u32 dst_qp,
			   hfi_tx_cq_pio_put_match_t *command)
{
	union hfi_process target_id = {.phys.slid = dlid};
	u16 cmd_length = _HFI_CMD_LENGTH(length, sizeof(hfi_tx_cq_pio_put_match_t));
	u16 cmd_slots  = _HFI_CMD_SLOTS(cmd_length);
	hfi_cmd_t cmd;
	cmd.ptl_opcode_low = op_req;
	cmd.ttype = PIO;

	_hfi_format_verb_put_flit0(&command->flit0, cmd, ctype, cmd_length,
				   target_id, force_frag, rc, sl,
				   pkey, slid_low, auth_idx, user_ptr,
				   md_options, eq_handle, ct_handle,
				   remote_offset, tx_handle, dst_qp);

	_hfi_format_put_flit_e1(ctx, &command->flit1.e,
				(dst_qp>>8), PTL_UINT64_T, length);

	command->flit1.f.val = (uintptr_t)start; /* Just zero extend local_start to 64-bits  */

	command->flit1.mb = match_bits;
	command->flit1.hd = hdr_data;

	return cmd_slots;
}

static inline
int hfi_format_pio_ud(struct hfi_ctx *ctx,
		      void *start, hfi_size_t length,
		      u32 dlid, u8 force_frag,
		      hfi_rc_t rc,
		      hfi_service_level_t sl,
		      hfi_pkey_t pkey, hfi_slid_low_t slid_low,
		      hfi_auth_idx_t auth_idx,
		      hfi_user_ptr_t user_ptr,
		      u32 src_qp,
		      u32 dst_qp,
		      u32 imm,
		      u32 qkey,
		      hfi_md_options_t md_options,
		      hfi_eq_handle_t eq_handle,
		      hfi_ct_handle_t ct_handle,
		      hfi_tx_handle_t tx_handle,
		      hfi_op_req_t op_req,
		      u8 solicit,
		      hfi_tx_cq_pio_put_match_t *command)
{
	hfi_match_bits_t match_bits = FMT_VOSTLNP_MB(MB_OC_OK, src_qp, qkey);
	hfi_hdr_data_t hdr_data = FMT_VOSTLNP_HD(imm, solicit, dst_qp);
	u64 remote_offset = FMT_UD_REMOTE_OFFSET(dlid, sl, pkey, src_qp);

	return hfi_format_pio_vostlnp(ctx, start, length,
				      (hfi_tx_ctype_t)UD, dlid, force_frag,
				      rc, sl, pkey,
				      slid_low, auth_idx, user_ptr, match_bits,
				      hdr_data, md_options, eq_handle,
				      ct_handle, (hfi_size_t)remote_offset,
				      tx_handle, op_req, dst_qp, command);
}

static inline
int hfi_format_pio_rc_send(struct hfi_ctx *ctx,
			   void *start, hfi_size_t length,
			   u32 dlid, u8 force_frag,
			   hfi_rc_t rc,
			   hfi_service_level_t sl,
			   hfi_pkey_t pkey, hfi_slid_low_t slid_low,
			   hfi_auth_idx_t auth_idx,
			   hfi_user_ptr_t user_ptr,
			   u32 src_qp,
			   u32 dst_qp,
			   u32 imm,
			   hfi_md_options_t md_options,
			   hfi_eq_handle_t eq_handle,
			   hfi_ct_handle_t ct_handle,
			   hfi_tx_handle_t tx_handle,
			   hfi_op_req_t op_req,
			   u8 solicit,
			   hfi_tx_cq_pio_put_match_t *command)
{
	hfi_match_bits_t match_bits = FMT_VOSTLNP_MB(MB_OC_OK, src_qp, imm);
	hfi_hdr_data_t hdr_data = FMT_VOSTLNP_HD(imm, solicit, dst_qp);
	return hfi_format_pio_vostlnp(ctx, start, length,
				      (hfi_tx_ctype_t)VoNP_RC, dlid, force_frag,
				      rc, sl, pkey,
				      slid_low, auth_idx, user_ptr, match_bits,
				      hdr_data, md_options, eq_handle,
				      ct_handle, (hfi_size_t)0, tx_handle,
				      op_req, dst_qp, command);
}

static inline
int hfi_format_pio_rc_rdma(struct hfi_ctx *ctx,
			   void *start, hfi_size_t length,
			   u32 dlid, u8 force_frag,
			   hfi_rc_t rc,
			   hfi_service_level_t sl,
			   hfi_pkey_t pkey, hfi_slid_low_t slid_low,
			   hfi_auth_idx_t auth_idx,
			   hfi_user_ptr_t user_ptr,
			   u32 src_qp,
			   u32 dst_qp,
			   u32 imm,
			   u32 rkey,
			   hfi_md_options_t md_options,
			   hfi_eq_handle_t eq_handle,
			   hfi_ct_handle_t ct_handle,
			   hfi_size_t remote_offset,
			   hfi_tx_handle_t tx_handle,
			   hfi_op_req_t op_req,
			   u8 solicit,
			   hfi_tx_cq_pio_put_match_t *command)
{
	hfi_match_bits_t match_bits = FMT_VOSTLNP_MB(MB_OC_OK, src_qp, rkey);
	hfi_hdr_data_t hdr_data = FMT_VOSTLNP_HD(imm, solicit, dst_qp);
	return hfi_format_pio_vostlnp(ctx, start, length,
				      (hfi_tx_ctype_t)VoNP_RC, dlid, force_frag,
				      rc, sl, pkey,
				      slid_low, auth_idx, user_ptr, match_bits,
				      hdr_data, md_options, eq_handle,
				      ct_handle, remote_offset, tx_handle,
				      op_req, dst_qp, command);
}

static inline
int hfi_format_dma_vostlnp(struct hfi_ctx *ctx,
			   void *start, hfi_size_t length,
			   hfi_tx_ctype_t ctype,
			   u32 dlid, u8 force_frag,
			   hfi_rc_t rc,
			   hfi_service_level_t sl,
			   hfi_pkey_t pkey, hfi_slid_low_t slid_low,
			   hfi_auth_idx_t auth_idx, hfi_user_ptr_t user_ptr,
			   hfi_match_bits_t match_bits,
			   hfi_hdr_data_t hdr_data,
			   hfi_md_options_t md_options,
			   hfi_eq_handle_t eq_handle,
			   hfi_ct_handle_t ct_handle,
			   hfi_size_t remote_offset,
			   hfi_tx_handle_t tx_handle,
			   hfi_op_req_t op_req,
			   u32 dst_qp,
			   union hfi_tx_cq_dma *command)
{
	union hfi_process target_id = {.phys.slid = dlid};
	u16 cmd_length = _HFI_CMD_LENGTH(0, sizeof(union hfi_tx_cq_dma));
	u16 cmd_slots  = _HFI_CMD_SLOTS(cmd_length);
	hfi_cmd_t cmd;

	cmd.ptl_opcode_low = op_req;
	cmd.ttype = DMA;

	_hfi_format_verb_put_flit0(&command->flit0, cmd, ctype, cmd_length,
				   target_id, force_frag, rc, sl,
				   pkey, slid_low, auth_idx, user_ptr,
				   md_options, eq_handle, ct_handle,
				   remote_offset, tx_handle, dst_qp);

	_hfi_format_put_flit_e1(ctx, &command->flit1.e,
				(dst_qp>>8), PTL_UINT64_T, length);

	command->flit1.f.val = (uintptr_t)start; /* Just zero extend local_start to 64-bits  */

	command->flit1.mb = match_bits;
	command->flit1.hd = hdr_data;

	return cmd_slots;
}

static inline
int hfi_update_dma_rc(struct hfi_ctx *ctx,
		      void *start, hfi_size_t length,
		      u8 force_frag,
		      hfi_rc_t rc,
		      hfi_user_ptr_t user_ptr,
		      hfi_match_bits_t match_bits,
		      hfi_hdr_data_t hdr_data,
		      hfi_md_options_t md_options,
		      hfi_eq_handle_t eq_handle,
		      hfi_ct_handle_t ct_handle,
		      hfi_size_t remote_offset,
		      hfi_tx_handle_t tx_handle,
		      hfi_op_req_t op_req,
		      u32 qp,
		      union hfi_tx_cq_dma *command)
{
	u16 cmd_length = _HFI_CMD_LENGTH(0, sizeof(union hfi_tx_cq_dma));
	u16 cmd_slots  = _HFI_CMD_SLOTS(cmd_length);
	hfi_cmd_t cmd;

	cmd.ptl_opcode_low = op_req;
	cmd.ttype = DMA;

	_hfi_update_verb_flit0_rc(&command->flit0, cmd, cmd_length,
				  force_frag, rc, user_ptr,
				  md_options, eq_handle, ct_handle,
				  remote_offset, tx_handle);

	_hfi_update_put_flit_e1(&command->flit1.e, length);

	command->flit1.f.val = (uintptr_t)start; /* Just zero extend local_start to 64-bits  */

	command->flit1.mb = match_bits;
	command->flit1.hd = hdr_data;

	return cmd_slots;
}

static inline
int hfi_format_dma_ud(struct hfi_ctx *ctx,
		      void *start, hfi_size_t length,
		      u32 dlid, u8 force_frag,
		      hfi_rc_t rc,
		      hfi_service_level_t sl,
		      hfi_pkey_t pkey, hfi_slid_low_t slid_low,
		      hfi_auth_idx_t auth_idx, hfi_user_ptr_t user_ptr,
		      u32 src_qp,
		      u32 dst_qp,
		      u32 imm,
		      u32 qkey,
		      hfi_md_options_t md_options,
		      hfi_eq_handle_t eq_handle,
		      hfi_ct_handle_t ct_handle,
		      hfi_tx_handle_t tx_handle,
		      hfi_op_req_t op_req,
		      u8 solicit,
		      union hfi_tx_cq_dma *command)
{
	hfi_match_bits_t match_bits = FMT_VOSTLNP_MB(MB_OC_OK, src_qp, qkey);
	hfi_hdr_data_t hdr_data = FMT_VOSTLNP_HD(imm, solicit, dst_qp);
	u64 remote_offset = FMT_UD_REMOTE_OFFSET(dlid, sl, pkey, src_qp);

	return hfi_format_dma_vostlnp(ctx, start, length,
				      (hfi_tx_ctype_t)UD, dlid, force_frag,
				      rc, sl, pkey,
				      slid_low, auth_idx, user_ptr, match_bits,
				      hdr_data, md_options, eq_handle,
				      ct_handle, (hfi_size_t)remote_offset,
				      tx_handle, op_req, dst_qp, command);
}

static inline
int hfi_format_dma_rc_send(struct hfi_ctx *ctx,
			   void *start, hfi_size_t length,
			   u8 force_frag,
			   hfi_rc_t rc,
			   hfi_user_ptr_t user_ptr,
			   u32 src_qp,
			   u32 dst_qp,
			   u32 imm,
			   hfi_md_options_t md_options,
			   hfi_eq_handle_t eq_handle,
			   hfi_ct_handle_t ct_handle,
			   hfi_tx_handle_t tx_handle,
			   hfi_op_req_t op_req,
			   u8 solicit,
			   union hfi_tx_cq_dma *command)
{
	hfi_match_bits_t match_bits = FMT_VOSTLNP_MB(MB_OC_OK, src_qp, imm);
	hfi_hdr_data_t hdr_data = FMT_VOSTLNP_HD(imm, solicit, dst_qp);

	return hfi_update_dma_rc(ctx, start, length, force_frag,
				 rc, user_ptr, match_bits,
				 hdr_data, md_options, eq_handle,
				 ct_handle, (hfi_size_t)0, tx_handle,
				 op_req, src_qp, command);
}

static inline
int hfi_format_dma_rc_rdma(struct hfi_ctx *ctx,
			   void *start, hfi_size_t length,
			   u8 force_frag,
			   hfi_rc_t rc,
			   hfi_user_ptr_t user_ptr,
			   u32 src_qp,
			   u32 dst_qp,
			   u32 imm,
			   u32 rkey,
			   hfi_md_options_t md_options,
			   hfi_eq_handle_t eq_handle,
			   hfi_ct_handle_t ct_handle,
			   hfi_size_t remote_offset,
			   hfi_tx_handle_t tx_handle,
			   hfi_op_req_t op_req,
			   u8 solicit,
			   union hfi_tx_cq_dma *command)
{
	hfi_match_bits_t match_bits = FMT_VOSTLNP_MB(MB_OC_OK, src_qp, rkey);
	hfi_hdr_data_t hdr_data = FMT_VOSTLNP_HD(imm, solicit, dst_qp);

	return hfi_update_dma_rc(ctx, start, length, force_frag,
				 rc, user_ptr, match_bits,
				 hdr_data, md_options, eq_handle,
				 ct_handle, remote_offset, tx_handle,
				 op_req, dst_qp, command);
}

static inline
int hfi_format_dma_iovec_vostlnp(struct hfi_ctx *ctx,
				 void *start, hfi_size_t length,
				 hfi_tx_ctype_t ctype,
				 u32 dlid, u8 force_frag,
				 hfi_rc_t rc,
				 hfi_service_level_t sl,
				 hfi_pkey_t pkey, hfi_slid_low_t slid_low,
				 hfi_auth_idx_t auth_idx,
				 hfi_user_ptr_t user_ptr,
				 hfi_match_bits_t match_bits,
				 hfi_hdr_data_t hdr_data,
				 hfi_md_options_t md_options,
				 hfi_eq_handle_t eq_handle,
				 hfi_ct_handle_t ct_handle,
				 hfi_size_t remote_offset,
				 hfi_tx_handle_t tx_handle,
				 hfi_op_req_t op_req,
				 u64 iov_count, u64 iov_offset,
				 u64 dst_qp,
				 union hfi_tx_cq_dma_iovec *command)
{
	union hfi_process target_id = {.phys.slid = dlid};
	/* CMD length for Triggered OPs is 9 QWORDS */
	/* however, true CMD length due to reserved bits is 12 QWORDS*/
	/* TODO: change back to using _HFI_CMD_LENGTH once >9 size is allowed*/
	u16 cmd_length = 9;
	u16 cmd_slots  = _HFI_CMD_SLOTS(cmd_length);
	hfi_cmd_t cmd;

	cmd.ptl_opcode_low = op_req;
	cmd.ttype = DMA;

	_hfi_format_verb_put_flit0(&command->flit0, cmd, ctype, cmd_length,
				   target_id, force_frag, rc, sl,
				   pkey, slid_low, auth_idx, user_ptr,
				   md_options, eq_handle, ct_handle,
				   remote_offset, tx_handle, dst_qp);

	_hfi_format_put_flit_e1(ctx, &command->flit1.e,
				(dst_qp>>8), PTL_UINT64_T, length);

	command->flit1.f.val = (uintptr_t)start; /* Just zero extend local_start to 64-bits  */

	command->flit1.mb = match_bits;
	command->flit1.hd = hdr_data;
	command->flit2.l.iov_entries = iov_count;
	command->flit2.l.iov_offset = iov_offset;
	return cmd_slots;
}

static inline
int hfi_update_dma_iovec_rc(struct hfi_ctx *ctx,
			    void *start, hfi_size_t length,
			    u8 force_frag,
			    hfi_rc_t rc,
			    hfi_user_ptr_t user_ptr,
			    hfi_match_bits_t match_bits,
			    hfi_hdr_data_t hdr_data,
			    hfi_md_options_t md_options,
			    hfi_eq_handle_t eq_handle,
			    hfi_ct_handle_t ct_handle,
			    hfi_size_t remote_offset,
			    hfi_tx_handle_t tx_handle,
			    hfi_op_req_t op_req,
			    u64 iov_count, u64 iov_offset,
			    u32 qp,
			    union hfi_tx_cq_dma_iovec *command)
{
	/* CMD length for Triggered OPs is 9 QWORDS */
	/* however, true CMD length due to reserved bits is 12 QWORDS*/
	/* TODO: change back to using _HFI_CMD_LENGTH once >9 size is allowed*/
	u16 cmd_length = 9;
	u16 cmd_slots  = _HFI_CMD_SLOTS(cmd_length);
	hfi_cmd_t cmd;

	cmd.ptl_opcode_low = op_req;
	cmd.ttype = DMA;

	_hfi_update_verb_flit0_rc(&command->flit0, cmd, cmd_length,
				  force_frag, rc, user_ptr,
				  md_options, eq_handle, ct_handle,
				  remote_offset, tx_handle);

	_hfi_update_put_flit_e1(&command->flit1.e, length);

	command->flit1.f.val = (uintptr_t)start; /* Just zero extend local_start to 64-bits  */

	command->flit1.mb = match_bits;
	command->flit1.hd = hdr_data;

	command->flit2.l.iov_entries = iov_count;
	command->flit2.l.iov_offset = iov_offset;
	return cmd_slots;
}

static inline
int hfi_format_dma_iovec_ud(struct hfi_ctx *ctx,
			    void *start, hfi_size_t length,
			    u32 dlid, u8 force_frag,
			    hfi_rc_t rc,
			    hfi_service_level_t sl,
			    hfi_pkey_t pkey, hfi_slid_low_t slid_low,
			    hfi_auth_idx_t auth_idx, hfi_user_ptr_t user_ptr,
			    u32 src_qp,
			    u32 dst_qp,
			    u32 imm,
			    u32 qkey,
			    hfi_md_options_t md_options,
			    hfi_eq_handle_t eq_handle,
			    hfi_ct_handle_t ct_handle,
			    hfi_tx_handle_t tx_handle,
			    hfi_op_req_t op_req,
			    u8 solicit,
			    u64 iov_count, u64 iov_offset,
			    union hfi_tx_cq_dma_iovec *command)
{
	hfi_match_bits_t match_bits = FMT_VOSTLNP_MB(MB_OC_OK, src_qp, qkey);
	hfi_hdr_data_t hdr_data = FMT_VOSTLNP_HD(imm, solicit, dst_qp);
	u64 remote_offset = FMT_UD_REMOTE_OFFSET(dlid, sl, pkey, src_qp);

	return hfi_format_dma_iovec_vostlnp(ctx, start, length,
				      (hfi_tx_ctype_t)UD, dlid, force_frag,
				      rc, sl, pkey,
				      slid_low, auth_idx, user_ptr, match_bits,
				      hdr_data, md_options, eq_handle,
				      ct_handle, (hfi_size_t)remote_offset,
				      tx_handle, op_req, iov_count, iov_offset,
				      dst_qp, command);
}

static inline
int hfi_format_dma_iovec_rc_send(struct hfi_ctx *ctx,
				 void *start, hfi_size_t length,
				 u8 force_frag,
				 hfi_rc_t rc,
				 hfi_user_ptr_t user_ptr,
				 u32 src_qp,
				 u32 dst_qp,
				 u32 imm,
				 hfi_md_options_t md_options,
				 hfi_eq_handle_t eq_handle,
				 hfi_ct_handle_t ct_handle,
				 hfi_tx_handle_t tx_handle,
				 hfi_op_req_t op_req,
				 u8 solicit,
				 u64 iov_count, u64 iov_offset,
				 union hfi_tx_cq_dma_iovec *command)
{
	hfi_match_bits_t match_bits = FMT_VOSTLNP_MB(MB_OC_OK, src_qp, imm);
	hfi_hdr_data_t hdr_data = FMT_VOSTLNP_HD(imm, solicit, dst_qp);

	return hfi_update_dma_iovec_rc(ctx, start, length, force_frag,
				       rc, user_ptr, match_bits,
				       hdr_data, md_options, eq_handle,
				       ct_handle, (hfi_size_t)0, tx_handle,
				       op_req, iov_count, iov_offset, src_qp,
				       command);
}

static inline
int hfi_format_dma_iovec_rc_rdma(struct hfi_ctx *ctx,
			      void *start, hfi_size_t length,
			      u8 force_frag,
			      hfi_rc_t rc,
			      hfi_user_ptr_t user_ptr,
			      u32 src_qp,
			      u32 dst_qp,
			      u32 imm,
			      u32 rkey,
			      hfi_md_options_t md_options,
			      hfi_eq_handle_t eq_handle,
			      hfi_ct_handle_t ct_handle,
			      hfi_size_t remote_offset,
			      hfi_tx_handle_t tx_handle,
			      hfi_op_req_t op_req,
			      u8 solicit,
			      u64 iov_count, u64 iov_offset,
			      union hfi_tx_cq_dma_iovec *command)
{
	hfi_match_bits_t match_bits = FMT_VOSTLNP_MB(MB_OC_OK, src_qp, rkey);
	hfi_hdr_data_t hdr_data = FMT_VOSTLNP_HD(imm, solicit, dst_qp);

	return hfi_update_dma_iovec_rc(ctx, start, length, force_frag,
				       rc, user_ptr, match_bits,
				       hdr_data, md_options, eq_handle,
				       ct_handle, remote_offset, tx_handle,
				       op_req, iov_count, iov_offset, src_qp,
				       command);
}

#endif /* _HFI_TX_VOSTLNP_H */
