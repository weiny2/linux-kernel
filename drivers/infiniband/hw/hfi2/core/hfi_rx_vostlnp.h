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

#ifndef _HFI_RX_VOSTLNP_H
#define _HFI_RX_VOSTLNP_H

#include "hfi_cmdq.h"

#define RKEY_OFFSET			0x1000
#define RKEY_IGNORE_BITS		0xffffffff00000000ull
#define RKEY_PID(rkey)			_hfi_extract(rkey, 20, 0xfff00000)
#define RKEY_LIST_HANDLE(rkey)		(_hfi_extract(rkey, 8, 0x000fff00)+RKEY_OFFSET)
#define RKEY_SALT(rkey)			_hfi_extract(rkey, 0, 0x000000ff)
#define RKEY_MATCH_BITS(rkey, pd)	((RKEY_SALT(rkey) << 24) | \
					 ((pd) & 0xffffff))

#define RECVQ_IGNORE_BITS		0xffffffffff000000ull
#define RECVQ_MATCH_BITS(pd)		((pd) & 0xffffff)

/* Format an RX CQ command for a RKEY WRITE operation */
static inline
int hfi_format_rkey_write(hfi_ni_t ni,
			  const void *logical_addr,
			  const void *start, hfi_size_t length,
			  hfi_process_t match_id, hfi_id_t id, uint32_t pd,
			  uint32_t rkey, hfi_me_options_t me_options,
			  hfi_ct_handle_t ct_handle,
			  hfi_user_ptr_t user_ptr,
			  uint8_t ncc, uint8_t valid,
			  union hfi_rx_cq_command *cmd)
{
	cmd->list.flit0.a             = RKEY_MATCH_BITS(rkey,pd);
	cmd->list.flit0.b	      = RKEY_IGNORE_BITS;
	cmd->list.flit0.c.user_id     = id;
	cmd->list.flit0.c.me_options  = me_options;
	cmd->list.flit0.d.initiator_id  = ((uint64_t)logical_addr);
	cmd->list.flit0.d.ct_handle   = ct_handle;
	cmd->list.flit0.d.command     = RKEY_WRITE;
	cmd->list.flit0.d.cmd_len     = (sizeof(cmd->list) >> 5) - 1;
	cmd->state.flit0.d.ncc        = ncc;

	cmd->list.flit1.e.cmd_pid     = RKEY_PID(rkey);
	cmd->list.flit1.e.list_handle = RKEY_LIST_HANDLE(rkey);
	cmd->list.flit1.e.min_free    = ((((uint64_t)logical_addr)>>36)&0x1fffffull);
	cmd->list.flit1.f.length      = length;
	cmd->list.flit1.g             = user_ptr;
	cmd->list.flit1.j.val         = 0;
	cmd->list.flit1.j.start       = (uintptr_t) start;
	/* no longer set o and f bits which are private to firmware */
	cmd->list.flit1.j.ni          = ni;
	cmd->list.flit1.j.v           = valid;

	return sizeof(cmd->list) >> 6;
}

/* Format an RX CQ command for a receive queue APPEND operation */
static inline
int hfi_format_recvq_append(hfi_ni_t ni,
			 const void *start, hfi_size_t length,
			 uint32_t recvq_root,
			 hfi_process_t match_id, hfi_id_t id, hfi_pid_t pid,
			 uint32_t pd,
			 hfi_me_options_t me_options,
			 hfi_ct_handle_t ct_handle,
			 hfi_user_ptr_t user_ptr, hfi_me_handle_t me_handle,
			 uint8_t ncc, union hfi_rx_cq_command *cmd)
{
	cmd->list.flit0.a             = RECVQ_MATCH_BITS(pd);
	cmd->list.flit0.b	      = RECVQ_IGNORE_BITS;
	cmd->list.flit0.c.user_id     = id;
	cmd->list.flit0.c.me_options  = me_options;
	cmd->list.flit0.d.initiator_id   = match_id.val;
	cmd->list.flit0.d.ct_handle   = ct_handle;
	cmd->list.flit0.d.command     = RECVQ_APPEND;
	cmd->list.flit0.d.cmd_len     = (sizeof(cmd->list) >> 5) - 1;
	cmd->state.flit0.d.ncc        = ncc;

	cmd->list.flit1.e.cmd_pid     = pid;
	cmd->list.flit1.e.list_handle = me_handle;
	cmd->list.flit1.e.min_free    = recvq_root;	// FIXME
	cmd->list.flit1.f.length      = length;
	cmd->list.flit1.g             = user_ptr;
	cmd->list.flit1.j.val         = 0;
	cmd->list.flit1.j.start       = (uintptr_t) start;
	/* no longer set o and f bits which are private to firmware */
	cmd->list.flit1.j.ni          = ni;
	cmd->list.flit1.j.v           = 1;

	return sizeof(cmd->list) >> 6;
}

static inline
int hfi_rkey_free(struct hfi_cmdq *rx, uint32_t rkey,
		  hfi_user_ptr_t user_ptr)
{
	union hfi_rx_cq_command cmd __aligned(64);
	int rc;

	cmd.list.flit0.d.command	= RKEY_FREE;
	cmd.list.flit0.d.cmd_len	= (sizeof(cmd.list) >> 5)-1;
	cmd.state.flit0.d.ncc		= HFI_GEN_CC;

	cmd.list.flit1.e.cmd_pid	= RKEY_PID(rkey);
	cmd.list.flit1.e.list_handle	= RKEY_LIST_HANDLE(rkey);
	cmd.list.flit1.g		= user_ptr;

	do {
		/* Single slot command */
		rc = hfi_rx_command(rx, (uint64_t *)&cmd, sizeof(cmd.list)>>6);
	} while (rc == -EAGAIN);

	return rc;
}

static inline
int hfi_rkey_invalidate(struct hfi_cmdq *rx, uint32_t rkey,
			hfi_user_ptr_t user_ptr)
{
	union hfi_rx_cq_command cmd __aligned(64);
	int cmd_slots, rc;
	hfi_process_t pt;

	cmd_slots = hfi_format_rkey_write(0, 0,
			0, 0,
			pt,
			0, 0, rkey,
			0, 0,
			user_ptr,
			HFI_GEN_CC, 0, &cmd);

	do {
		/* Single slot command */
		rc = hfi_rx_command(rx, (uint64_t *)&cmd, cmd_slots);
	} while (rc == -EAGAIN);

	return rc;
}

static inline
int hfi_rkey_write(struct hfi_cmdq *rx, hfi_ni_t ni,
		const void *logical_addr,
		const void *start, hfi_size_t length,
		hfi_process_t match_id, hfi_id_t id,
		uint32_t pd, uint32_t rkey,
		hfi_me_options_t me_options,
		hfi_ct_handle_t ct_handle,
		hfi_user_ptr_t user_ptr)
{
	union hfi_rx_cq_command cmd __aligned(64);
	int cmd_slots, rc;

	cmd_slots = hfi_format_rkey_write(ni,
			logical_addr, start, length,
			match_id,
			id, pd, rkey,
			me_options, ct_handle,
			user_ptr,
			HFI_GEN_CC, 1, &cmd);

	do {
		/* Single slot command */
		rc = hfi_rx_command(rx, (uint64_t *)&cmd, cmd_slots);
	} while (rc == -EAGAIN);

	return rc;
}

static inline
int hfi_recvq_append(struct hfi_cmdq *rx, hfi_ni_t ni,
		const void *start, hfi_size_t length,
		uint32_t recvq_root,
		hfi_process_t match_id, hfi_id_t id, hfi_pid_t pid,
		uint32_t pd,
		hfi_me_options_t me_options,
		hfi_ct_handle_t ct_handle,
		hfi_user_ptr_t user_ptr, hfi_me_handle_t me_handle,
		uint8_t ncc)
{
	union hfi_rx_cq_command cmd __aligned(64);
	int cmd_slots, rc;

	cmd_slots = hfi_format_recvq_append(ni,
			start, length,
			recvq_root,
			match_id,
			id, pid, pd,
			me_options, ct_handle,
			user_ptr,
			me_handle, ncc, &cmd);

	do {
		/* Single slot command */
		rc = hfi_rx_command(rx, (uint64_t *)&cmd, cmd_slots);
	} while (rc == -EAGAIN);

	return rc;
}

static inline
void hfi_format_entry_read(struct hfi_ctx *ctx,
			   uint8_t ni, uint16_t me_handle,
			   union hfi_rx_cq_command *cmd,
			   uint64_t user_ptr)
{
	memset(&cmd->list, 0, sizeof(cmd->list));

	/* Only need ME/LE handle, process ID, NI, and portal table index */
	cmd->list.flit1.e.list_handle = me_handle;
	cmd->list.flit1.e.cmd_pid     = ctx->pid;
	cmd->list.flit1.j.ni          = ni;
	cmd->list.flit0.d.ct_handle   = HFI_CT_NONE;
	cmd->list.flit0.d.command     = ENTRY_READ;
	cmd->list.flit0.d.cmd_len     = (sizeof(cmd->list) >> 5) - 1;
	cmd->list.flit1.g	      = user_ptr;
}

static inline
int hfi_recvq_init(struct hfi_cmdq *rx, uint8_t ni, struct hfi_ctx *ctx,
		   uint32_t recvq_root, uint64_t user_ptr)
{
	union hfi_rx_cq_command cmd __aligned(64);
	int rc;

	cmd.state.flit0.d.ni		= ni;
	cmd.list.flit0.d.ct_handle	= PTL_CT_NONE;
	cmd.list.flit0.d.ncc		= HFI_GEN_CC;
	cmd.list.flit0.d.command	= RECVQ_INIT;
	cmd.list.flit0.d.cmd_len	= (sizeof(cmd.list) >> 5) - 1;

	cmd.list.flit1.e.cmd_pid	= ctx->pid;
	cmd.list.flit1.e.list_handle	= recvq_root;
	cmd.list.flit1.g		= user_ptr;

	do {
		/* Single slot command */
		rc = hfi_rx_command(rx, (uint64_t *)&cmd,
				    sizeof(cmd.list) >> 6);
	} while (rc == -EAGAIN);

	return rc;
}

static inline
int hfi_recvq_unlink(struct hfi_cmdq *rx, uint8_t ni, struct hfi_ctx *ctx,
		     uint32_t recvq_root, uint64_t user_ptr)
{
	union hfi_rx_cq_command cmd __aligned(64);
	int rc;

	cmd.list.flit0.d.ct_handle	= PTL_CT_NONE;
	cmd.list.flit0.d.ncc		= HFI_GEN_CC;
	cmd.list.flit0.d.command	= RECVQ_UNLINK;
	cmd.list.flit0.d.cmd_len	= (sizeof(cmd.list) >> 5) - 1;

	cmd.list.flit1.e.cmd_pid	= ctx->pid;
	cmd.list.flit1.e.min_free       = recvq_root;	// FIXME
	cmd.list.flit1.g		= user_ptr;
	cmd.list.flit1.j.ni             = ni;

	do {
		/* Single slot command */
		rc = hfi_rx_command(rx, (uint64_t *)&cmd,
				    sizeof(cmd.list) >> 6);
	} while (rc == -EAGAIN);

	return rc;
}

#endif /* _HFI_RX_VOSTLNP_H */
