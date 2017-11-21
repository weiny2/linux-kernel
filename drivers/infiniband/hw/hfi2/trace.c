/*
 * Copyright(c) 2015, 2016 Intel Corporation.
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * BSD LICENSE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Intel Corporation nor the names of its
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
 */

/*
 * Intel(R) Omni-Path Gen2 HFI PCIe Driver
 */

#define CREATE_TRACE_POINTS
#include "trace.h"

u8 ibhdr_exhdr_len(union hfi2_packet_header *hdr,
		   bool use_16b)
{
	struct ib_other_headers *ohdr;
	u8 opcode;

	if (use_16b) {
		if (hfi2_16B_get_l4(&hdr->opah) == HFI1_L4_IB_LOCAL)
			ohdr = &hdr->opah.u.oth;
		else
			ohdr = &hdr->opah.u.l.oth;
	} else {
		u8 lnh = (u8)(be16_to_cpu(hdr->ibh.lrh[0]) & 3);

		if (lnh == HFI1_LRH_BTH)
			ohdr = &hdr->ibh.u.oth;
		else
			ohdr = &hdr->ibh.u.l.oth;
	}
	opcode = be32_to_cpu(ohdr->bth[0]) >> 24;
	return ib_hdr_len_by_opcode[opcode];
}

#define IMM_PRN  "imm %d"
#define RETH_PRN "reth vaddr 0x%.16llx rkey 0x%.8x dlen 0x%.8x"
#define AETH_PRN "aeth syn 0x%.2x %s msn 0x%.8x"
#define DETH_PRN "deth qkey 0x%.8x sqpn 0x%.6x"
#define ATOMICACKETH_PRN "origdata %lld"
#define ATOMICETH_PRN "vaddr 0x%llx rkey 0x%.8x sdata %lld cdata %lld"

#define OP(transport, op) IB_OPCODE_## transport ## _ ## op

static const char *parse_syndrome(u8 syndrome)
{
	switch (syndrome >> 5) {
	case 0:
		return "ACK";
	case 1:
		return "RNRNAK";
	case 3:
		return "NAK";
	}
	return "";
}

const char *parse_everbs_hdrs(
	struct trace_seq *p,
	u8 opcode,
	void *ehdrs)
{
	union ib_ehdrs *eh = ehdrs;
	const char *ret = trace_seq_buffer_ptr(p);

	switch (opcode) {
	/* imm */
	case OP(RC, SEND_LAST_WITH_IMMEDIATE):
	case OP(UC, SEND_LAST_WITH_IMMEDIATE):
	case OP(RC, SEND_ONLY_WITH_IMMEDIATE):
	case OP(UC, SEND_ONLY_WITH_IMMEDIATE):
	case OP(RC, RDMA_WRITE_LAST_WITH_IMMEDIATE):
	case OP(UC, RDMA_WRITE_LAST_WITH_IMMEDIATE):
		trace_seq_printf(p, IMM_PRN,
				 be32_to_cpu(eh->imm_data));
		break;
	/* reth + imm */
	case OP(RC, RDMA_WRITE_ONLY_WITH_IMMEDIATE):
	case OP(UC, RDMA_WRITE_ONLY_WITH_IMMEDIATE):
		trace_seq_printf(p, RETH_PRN " " IMM_PRN,
				 get_ib_reth_vaddr(&eh->rc.reth),
				 be32_to_cpu(eh->rc.reth.rkey),
				 be32_to_cpu(eh->rc.reth.length),
				 be32_to_cpu(eh->rc.imm_data));
		break;
	/* reth */
	case OP(RC, RDMA_READ_REQUEST):
	case OP(RC, RDMA_WRITE_FIRST):
	case OP(UC, RDMA_WRITE_FIRST):
	case OP(RC, RDMA_WRITE_ONLY):
	case OP(UC, RDMA_WRITE_ONLY):
		trace_seq_printf(p, RETH_PRN,
				 get_ib_reth_vaddr(&eh->rc.reth),
				 be32_to_cpu(eh->rc.reth.rkey),
				 be32_to_cpu(eh->rc.reth.length));
		break;
	case OP(RC, RDMA_READ_RESPONSE_FIRST):
	case OP(RC, RDMA_READ_RESPONSE_LAST):
	case OP(RC, RDMA_READ_RESPONSE_ONLY):
	case OP(RC, ACKNOWLEDGE):
		trace_seq_printf(p, AETH_PRN,
				 be32_to_cpu(eh->aeth) >> 24,
				 parse_syndrome(be32_to_cpu(eh->aeth) >> 24),
				 be32_to_cpu(eh->aeth) & IB_MSN_MASK);
		break;
	/* aeth + atomicacketh */
	case OP(RC, ATOMIC_ACKNOWLEDGE):
		trace_seq_printf(p, AETH_PRN " " ATOMICACKETH_PRN,
				 be32_to_cpu(eh->at.aeth) >> 24,
				 parse_syndrome(be32_to_cpu(eh->at.aeth) >> 24),
				 be32_to_cpu(eh->at.aeth) & IB_MSN_MASK,
				 ib_u64_get(&eh->at.atomic_ack_eth));
		break;
	/* atomiceth */
	case OP(RC, COMPARE_SWAP):
	case OP(RC, FETCH_ADD):
		trace_seq_printf(p, ATOMICETH_PRN,
				 get_ib_reth_vaddr(&eh->rc.reth),
				 eh->atomic_eth.rkey,
				 get_ib_ateth_swap(&eh->atomic_eth),
				 get_ib_ateth_compare(&eh->atomic_eth));
		break;
	/* deth */
	case OP(UD, SEND_ONLY):
	case OP(UD, SEND_ONLY_WITH_IMMEDIATE):
		trace_seq_printf(p, DETH_PRN,
				 be32_to_cpu(eh->ud.deth[0]),
				 be32_to_cpu(eh->ud.deth[1]) & RVT_QPN_MASK);
		break;
	}
	trace_seq_putc(p, 0);
	return ret;
}

__hfi2_trace_fn(DEBUG);
__hfi2_trace_fn(SNOOP);
__hfi2_trace_fn(8051);
