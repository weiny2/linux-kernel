/*
 * Copyright (c) 2014, 2015 Intel Corporation. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#define CREATE_TRACE_POINTS
#define HFI_TRACE_DO_NOT_CREATE_INLINES
#include "trace.h"

u8 ibhdr_exhdr_len(struct qib_ib_header *hdr)
{
	struct qib_other_headers *ohdr;
	u8 opcode;
	u8 lnh = (u8)(be16_to_cpu(hdr->lrh[0]) & 3);

	if (lnh == QIB_LRH_BTH)
		ohdr = &hdr->u.oth;
	else
		ohdr = &hdr->u.l.oth;
	opcode = be32_to_cpu(ohdr->bth[0]) >> 24;
	return hdr_len_by_opcode[opcode] == 0 ?
	       0 : hdr_len_by_opcode[opcode] - (12 + 8);
}

#define IMM_PRN  "imm %d"
#define RETH_PRN "reth vaddr 0x%.16llx rkey 0x%.8x dlen 0x%.8x"
#define AETH_PRN "aeth syn 0x%.2x msn 0x%.8x"
#define DETH_PRN "deth qkey 0x%.8x sqpn 0x%.6x"
#define ATOMICACKETH_PRN "origdata %lld"
#define ATOMICETH_PRN "vaddr 0x%llx rkey 0x%.8x sdata %lld cdata %lld"

#define OP(transport, op) IB_OPCODE_## transport ## _ ## op

static u64 ib_u64_get(__be32 *p)
{
	return ((u64)be32_to_cpu(p[0]) << 32) | be32_to_cpu(p[1]);
}

const char *parse_everbs_hdrs(
	struct trace_seq *p,
	u8 opcode,
	void *ehdrs)
{
	union ib_ehdrs *eh = ehdrs;
	const char *ret = p->buffer + p->len;

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
			(unsigned long long)ib_u64_get(
				(__be32 *)&eh->rc.reth.vaddr),
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
			(unsigned long long)ib_u64_get(
				(__be32 *)&eh->rc.reth.vaddr),
			be32_to_cpu(eh->rc.reth.rkey),
			be32_to_cpu(eh->rc.reth.length));
		break;
	case OP(RC, RDMA_READ_RESPONSE_FIRST):
	case OP(RC, RDMA_READ_RESPONSE_LAST):
	case OP(RC, RDMA_READ_RESPONSE_ONLY):
	case OP(RC, ACKNOWLEDGE):
		trace_seq_printf(p, AETH_PRN,
			be32_to_cpu(eh->aeth) >> 24,
			be32_to_cpu(eh->aeth) & QIB_QPN_MASK);
		break;
	/* aeth + atomicacketh */
	case OP(RC, ATOMIC_ACKNOWLEDGE):
		trace_seq_printf(p, AETH_PRN " " ATOMICACKETH_PRN,
			(be32_to_cpu(eh->at.aeth) >> 24) & 0xff,
			be32_to_cpu(eh->at.aeth) & QIB_QPN_MASK,
			(unsigned long long)ib_u64_get(eh->at.atomic_ack_eth));
		break;
	/* atomiceth */
	case OP(RC, COMPARE_SWAP):
	case OP(RC, FETCH_ADD):
		trace_seq_printf(p, ATOMICETH_PRN,
			(unsigned long long)ib_u64_get(eh->atomic_eth.vaddr),
			eh->atomic_eth.rkey,
			(unsigned long long)ib_u64_get(
				(__be32 *)&eh->atomic_eth.swap_data),
			(unsigned long long) ib_u64_get(
				 (__be32 *)&eh->atomic_eth.compare_data));
		break;
	/* deth */
	case OP(UD, SEND_ONLY):
	case OP(UD, SEND_ONLY_WITH_IMMEDIATE):
		trace_seq_printf(p, DETH_PRN,
			be32_to_cpu(eh->ud.deth[0]),
			be32_to_cpu(eh->ud.deth[1]) & QIB_QPN_MASK);
		break;
	}
	trace_seq_putc(p, 0);
	return ret;
}

const char *parse_sdma_flags(
	struct trace_seq *p,
	u64 desc0, u64 desc1)
{
	const char *ret = p->buffer + p->len;
	char flags[5] = { 'x', 'x', 'x', 'x', 0 };

	flags[0] = (desc1 & SDMA_DESC1_INT_REQ_FLAG) ? 'I' : '-';
	flags[1] = (desc1 & SDMA_DESC1_HEAD_TO_HOST_FLAG) ?  'H' : '-';
	flags[2] = (desc0 & SDMA_DESC0_FIRST_DESC_FLAG) ? 'F' : '-';
	flags[3] = (desc0 & SDMA_DESC0_LAST_DESC_FLAG) ? 'L' : '-';
	trace_seq_printf(p, "%s", flags);
	if (desc0 & SDMA_DESC0_FIRST_DESC_FLAG)
		trace_seq_printf(p, " amode:%u aidx:%u alen:%u",
			(u8)((desc1 >> SDMA_DESC1_HEADER_MODE_SHIFT)
				& SDMA_DESC1_HEADER_MODE_MASK),
			(u8)((desc1 >> SDMA_DESC1_HEADER_INDEX_SHIFT)
				& SDMA_DESC1_HEADER_INDEX_MASK),
			(u8)((desc1 >> SDMA_DESC1_HEADER_DWS_SHIFT)
				& SDMA_DESC1_HEADER_DWS_MASK));
	return ret;
}

const char *print_u32_array(
	struct trace_seq *p,
	u32 *arr, int len)
{
	int i;
	const char *ret = p->buffer + p->len;

	for (i = 0; i < len ; i++)
		trace_seq_printf(p, "%s%#x", i == 0 ? "" : " ", arr[i]);
	trace_seq_putc(p, 0);
	return ret;
}

const char *print_u64_array(
	struct trace_seq *p,
	u64 *arr, int len)
{
	int i;
	const char *ret = p->buffer + p->len;

	for (i = 0; i < len; i++)
		trace_seq_printf(p, "%s0x%016llx", i == 0 ? "" : " ", arr[i]);
	trace_seq_putc(p, 0);
	return ret;
}
#undef HFI_TRACE_DO_NOT_CREATE_INLINES
