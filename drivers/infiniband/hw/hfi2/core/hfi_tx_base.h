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

#ifndef _HFI_TX_BASE_H
#define _HFI_TX_BASE_H

#include "hfi_cmdq.h"

/* TODO - remove usage of these */
typedef enum flag		hfi_flag_t;
typedef enum ptl_list		hfi_list_t;
typedef enum ptl_op_req		hfi_op_req_t;
typedef enum ptl_fetch_op_req	hfi_fetch_op_req_t;
typedef enum ptl_datatype	hfi_atomic_type_t;
typedef enum ptl_op		hfi_atomic_op_t;
typedef enum ptl_event_kind	hfi_event_kind_t;
typedef enum ptl_ni_fail	hfi_fail_type_t;
typedef enum ptl_L4_ack_req	hfi_ack_req_t;
typedef uint8_t			hfi_ni_t;
typedef enum tx_ctype		hfi_tx_ctype_t;
typedef enum transfer_type	hfi_tx_ttype_t;
typedef ptl_hdr_data_t		hfi_hdr_data_t;
typedef ptl_match_bits_t	hfi_match_bits_t;
typedef user_pointer_t		hfi_user_ptr_t;
typedef threshold_t		hfi_threshold_t;
typedef union ptl_cmd		hfi_cmd_t;
typedef uint16_t 		hfi_pid_t;
typedef uint32_t		hfi_id_t;
typedef hfi_id_t		hfi_uid_t;
typedef uint32_t		hfi_lid_t;
typedef uint32_t		hfi_rank_t;
typedef uint64_t		hfi_size_t;
typedef uint16_t		hfi_ct_handle_t;
typedef uint16_t		hfi_tx_handle_t;
typedef uint16_t		hfi_me_handle_t;
typedef uint32_t		hfi_md_options_t;
typedef union resp_md_options	hfi_resp_md_options_t;
typedef uint32_t		hfi_me_options_t;
typedef uint8_t			hfi_port_t;
typedef uint8_t			hfi_rc_t;
typedef uint8_t			hfi_service_level_t;
typedef uint8_t			hfi_becn_t;
typedef uint16_t		hfi_pkey_t;
typedef uint8_t			hfi_slid_low_t;
typedef uint8_t			hfi_auth_idx_t;
typedef union hfi_process	hfi_process_t;
typedef struct hfi_eq *hfi_eq_handle_t;

/*
 * TX Command formats
 */

/*
 * Buffered Put/Atomic
 */
typedef union hfi_tx_cq_buff_put {
	struct {
		union tx_cq_base_put_flit0 		flit0;
		union tx_cq_buff_put_flit1 		flit1;
	};
	uint64_t					command[16];
} hfi_tx_cq_buff_put_t __aligned(64);

typedef union hfi_tx_cq_buff_put_match {
	struct {
		union tx_cq_base_put_flit0 		flit0;
		union tx_cq_buff_put_superset_flit1	flit1;
	};
	uint64_t					command[16];
} hfi_tx_cq_buff_put_match_t __aligned(64);

/*
 * Buffered Two Operand or Fetching Atomic
 */
typedef union hfi_tx_cq_buff_toa_fetch {
	struct {
		union tx_cq_base_put_flit0 		flit0;
		union tx_cq_buff_two_op_put_flit1	flit1;
	};
	uint64_t					command[16];
} hfi_tx_cq_buff_toa_fetch_t __aligned(64);

typedef union hfi_tx_cq_buff_toa_fetch_match {
	struct {
		union tx_cq_base_put_flit0 		flit0;
		union tx_cq_buff_two_op_put_superset_flit1  flit1;
		uint64_t 				payload[8];
	};
	uint64_t					command[16];
} hfi_tx_cq_buff_toa_fetch_match_t __aligned(64);

/*
 * PIO Put/Atomic
 */
typedef union hfi_tx_cq_pio_put {
	struct {
		union tx_cq_base_put_flit0 		flit0;
		union tx_cq_pio_put_flit1 		flit1;
	};
	uint64_t					command[8];
} hfi_tx_cq_pio_put_t __aligned(64);

typedef union hfi_tx_cq_pio_put_match {
	struct {
		union tx_cq_base_put_flit0 		flit0;
		union tx_cq_put_superset_flit1 		flit1;
	};
	uint64_t					command[8];
} hfi_tx_cq_pio_put_match_t __aligned(64);

/*
 * DMA Command format
 * Used for DMA Put/Atomic/Get & Rendezvous Put/Atomic/Get
 */
typedef union hfi_tx_cq_dma {
	struct {
		union tx_cq_base_put_flit0 		flit0;
		union tx_cq_put_superset_flit1 		flit1;
	};
	uint64_t					command[8];
} hfi_tx_cq_dma_t __aligned(64);

/*
 * DMA IOVEC Command format
 * Used for DMA Put IOVEC Operations
 */
typedef union hfi_tx_cq_dma_iovec {
	struct {
		union tx_cq_base_put_flit0	flit0;
		union tx_cq_iovec_flit1		flit1;
		union tx_cq_iovec_flit2		flit2;
	};
	uint64_t					command[12];
} hfi_tx_cq_dma_iovec_t __aligned(64);

/*
 * Union of all TX commands
 */
typedef union hfi_tx_cq_command {
	hfi_tx_cq_buff_put_t				buff_put;
	hfi_tx_cq_buff_put_match_t			buff_put_match;
	hfi_tx_cq_buff_toa_fetch_t			buff_toa_fetch;
	hfi_tx_cq_buff_toa_fetch_match_t		buff_toa_fetch_match;
	hfi_tx_cq_pio_put_t				pio_put;
	hfi_tx_cq_pio_put_match_t			pio_put_match;
	hfi_tx_cq_dma_t					dma;
	hfi_tx_cq_dma_iovec_t				dma_iovec;
	uint64_t					command[16];
} hfi_tx_cq_command_t __aligned(64);


/* Format a the common TX CQ command flit0 */
static inline
void _hfi_format_base_put_flit0(struct hfi_ctx *ctx, hfi_ni_t ni,
				union tx_cq_base_put_flit0 *flit0,
				hfi_cmd_t cmd, hfi_tx_ctype_t ctype,
				uint16_t cmd_length,
				hfi_process_t target_id, hfi_port_t port,
				uint32_t pt_index,
				hfi_rc_t rc, hfi_service_level_t sl,
				hfi_becn_t becn, hfi_pkey_t pkey,
				hfi_slid_low_t slid_low,
				hfi_auth_idx_t auth_idx,
				hfi_user_ptr_t user_ptr, hfi_flag_t hd,
				hfi_ack_req_t ack_req,
				hfi_md_options_t md_options,
				hfi_eq_handle_t eq_handle,
				hfi_ct_handle_t ct_handle,
				hfi_size_t remote_offset,
				hfi_tx_handle_t tx_handle)
{
	flit0->a.val 		= 0;
	flit0->a.pt		= port;
	flit0->a.cmd		= cmd.val;
	flit0->a.ptl_idx 	= pt_index;
	flit0->a.rc		= rc;
	flit0->a.sl		= sl;
	flit0->a.b		= becn;
	flit0->a.sh		= 1; /* Always request a short header - let HW decide */
	flit0->a.ctype		= ctype;
	flit0->a.cmd_length 	= cmd_length;
	flit0->a.dlid 		= target_id.phys.slid;

	flit0->b.val 		= 0;
	flit0->b.md_handle 	= tx_handle;
	flit0->b.slid_low	= slid_low;
	flit0->b.auth_idx	= auth_idx;
	flit0->b.pkey		= pkey;
	flit0->b.eq_handle	= eq_handle->idx;
	flit0->b.ct_handle	= ct_handle;
	flit0->b.md_options 	= md_options;

	/* enable DLID table lookup (PD=0) if table was set and logical NI */
	flit0->b.pd		= !((ctx->mode & HFI_CTX_MODE_LID_VIRTUALIZED) &&
				    !HFI_PHYSICAL_NI(ni));

	flit0->b.hd		= hd;

	flit0->c	 	= user_ptr;

	flit0->d.val 		= 0; /* TODO do we need to do this ? */
	flit0->d.remote_offset 	= remote_offset;
	flit0->d.ack_req 	= ack_req;
	flit0->d.ni 		= ni;
}

static inline
void _hfi_format_put_flit_e1(struct hfi_ctx *ctx,
			     union tx_cq_e1 *e,
			     hfi_pid_t tpid,
			     hfi_atomic_type_t atomic_dtype,
			     hfi_size_t length)
{
	e->val			= 0; /* TODO do we need to do this ? */
	e->tpid			= tpid;
	e->ipid			= ctx->pid;
	e->atomic_dtype		= atomic_dtype;
	e->message_length	= length;
}

static inline
void _hfi_format_put_flit_e2(struct hfi_ctx *ctx,
			     union tx_cq_e2 *e,
			     hfi_pid_t tpid,
			     hfi_atomic_type_t atomic_dtype,
			     hfi_size_t length,
			     hfi_eq_handle_t resp_eq_handle,
			     hfi_ct_handle_t resp_ct_handle,
			     hfi_resp_md_options_t resp_md_options)
{
	e->val			= 0; /* TODO do we need to do this ? */
	e->tpid			= tpid;
	e->ipid			= ctx->pid;
	e->atomic_dtype		= atomic_dtype;
	e->resp_eq_handle	= resp_eq_handle->idx;
	e->resp_ct_handle	= resp_ct_handle;
	e->message_length	= length - 1;
	e->resp_md_opts		= resp_md_options.val;
}

/* Encoding of Native Verbs MB, ROFFSET, and HDR_DATA */
#define MB_OC_OK				0
#define MB_OC_TX_FLUSH				1
#define MB_OC_RX_FLUSH				2
#define MB_OC_LOCAL_INV				3
#define MB_OC_REG_MR				4
#define MB_OC_QP_RESET				5
#define FMT_VOSTLNP_MB(opcode, src_qp, l32)	(((uint64_t)(opcode)) << 56 | \
						((uint64_t)(src_qp)) << 32 | \
						((uint64_t)(l32)) << 0)
#define EXTRACT_MB_SRC_QP(mb)		_hfi_extract(mb, 32, 0x00ffffff00000000)
#define EXTRACT_MB_OPCODE(mb)		_hfi_extract(mb, 56, 0xff00000000000000)
#define FMT_UD_REMOTE_OFFSET(dlid, sl, pkey, src_qp) \
						(((uint64_t)(dlid)) << 45 | \
						((uint64_t)(sl)) << 40 | \
						((uint64_t)(pkey)) << 24 | \
						((uint64_t)(src_qp)) << 0)
#define EXTRACT_UD_DLID(ro)		_hfi_extract(ro, 45, 0x0000e00000000000)
#define EXTRACT_UD_SL(ro)		_hfi_extract(ro, 40, 0x00001f0000000000)
#define EXTRACT_UD_RKEY(ro)		_hfi_extract(ro, 24, 0x000000ffff000000)
#define EXTRACT_UD_SRC_QP(ro)		_hfi_extract(ro, 0, 0x0000000000ffffff)
#define FMT_VOSTLNP_HD(imm, mtype, s, dst_qp)	(((uint64_t)(imm)) << 32 | \
						((uint64_t)(mtype)) << 31 | \
						((uint64_t)(s)) << 24 | \
						((uint64_t)(dst_qp)) << 0)
#define EXTRACT_HD_IMM_DATA(hd)		_hfi_extract(hd, 32, 0xffffffff00000000)
#define EXTRACT_HD_DST_QP(hd)		_hfi_extract(hd,  0, 0x0000000000ffffff)

#endif /* _HFI_TX_BASE_H */
