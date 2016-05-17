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
#define CREATE_TRACE_POINTS
#include "trace.h"

static u8 __get_16b_hdr_len (struct hfi1_16b_header *hdr)
{
	struct hfi1_other_headers *ohdr;
	u8 hlen, l4, opcode;

	l4 = OPA_16B_GET_L4(0, 0, hdr->lrh[2], 0);
	if (l4 == HFI1_L4_IB_LOCAL) {
		ohdr = &hdr->u.oth;
	} else {
		hlen = 40; /* GRH */
		ohdr = &hdr->u.l.oth;
	}

	opcode = be32_to_cpu(ohdr->bth[0]) >> 24;
	hlen += hdr_16b_len_by_opcode[opcode];

	return hlen;
}

static u8 __get_ib_hdr_len (struct hfi1_ib_header *hdr)
{
	struct hfi1_other_headers *ohdr;
	u8 hlen, lnh, opcode;

	lnh = (u8)(be16_to_cpu(hdr->lrh[0]) & 3);
	if (lnh == HFI1_LRH_BTH)
		ohdr = &hdr->u.oth;
	else
		ohdr = &hdr->u.l.oth;

	opcode = be32_to_cpu(ohdr->bth[0]) >> 24;
	hlen =  hdr_len_by_opcode[opcode] == 0 ?
		0 : hdr_len_by_opcode[opcode] - (12 + 8);

	return hlen;
}

u8 hfi1_trace_packet_hdr_len(struct hfi1_packet *packet)
{
	if (packet->bypass)
		return __get_16b_hdr_len(packet->hdr);
	else
		return __get_ib_hdr_len(packet->hdr);
}

u8 hfi1_trace_opa_hdr_len(struct hfi1_opa_header *opa_hdr)
{
	if (opa_hdr->hdr_type)
		return __get_16b_hdr_len(&opa_hdr->pkt.opah);
	else
		return __get_ib_hdr_len(&opa_hdr->pkt.ibh);
}

const char *hfi1_trace_get_packet_str(struct hfi1_packet *packet)
{
	if (packet->bypass) {
		struct hfi1_16b_header *hdr_16b = packet->hdr;
		u8 l2 = OPA_16B_GET_L2(0, hdr_16b->lrh[1], 0, 0);

		switch(l2) {
		case 0:
			return "8B";
		case 1:
			return "10B";
		case 2:
			return "16B";
		case 3:
			return "9B";
		}
		return "";
	} else {
		return "IB";
	}
}

#define IMM_PRN  "imm %d"
#define RETH_PRN "reth vaddr 0x%.16llx rkey 0x%.8x dlen 0x%.8x"
#define AETH_PRN "aeth syn 0x%.2x %s msn 0x%.8x"
#define DETH_PRN "deth qkey 0x%.8x sqpn 0x%.6x"
#define ATOMICACKETH_PRN "origdata %lld"
#define ATOMICETH_PRN "vaddr 0x%llx rkey 0x%.8x sdata %lld cdata %lld"

#define OP(transport, op) IB_OPCODE_## transport ## _ ## op

static u64 ib_u64_get(__be32 *p)
{
	return ((u64)be32_to_cpu(p[0]) << 32) | be32_to_cpu(p[1]);
}

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

const char *hfi1_trace_get_packet_type_str(u8 l4)
{
	if (l4)
		return "16B";
	else
		return "9B";
}

void hfi1_trace_parse_bth(struct hfi1_other_headers *ohdr,
			   u8 *a, u8 *b, u8 *f, u8 *m,
			   u8 *se, u8 *pad, u8 *opcode, u8 *tver,
			   u16 *pkey, u32 *psn, u32 *qpn)
{
	*opcode = (be32_to_cpu(ohdr->bth[0]) >> 24) & 0xff;
	*se = (be32_to_cpu(ohdr->bth[0]) >> 23) & 1;
	*m = (be32_to_cpu(ohdr->bth[0]) >> 22) & 1;
	*pad = (be32_to_cpu(ohdr->bth[0]) >> 20) & 3;
	*tver = (be32_to_cpu(ohdr->bth[0]) >> 16) & 0xf;
	*pkey = be32_to_cpu(ohdr->bth[0]) & 0xffff;
	*f = (be32_to_cpu(ohdr->bth[1]) >> HFI1_FECN_SHIFT) & HFI1_FECN_MASK;
	*b = (be32_to_cpu(ohdr->bth[1]) >> HFI1_BECN_SHIFT) & HFI1_BECN_MASK;
	*qpn = be32_to_cpu(ohdr->bth[1]) & RVT_QPN_MASK;
	*a = (be32_to_cpu(ohdr->bth[2]) >> 31) & 1;
	/* allow for larger PSN */
	*psn = be32_to_cpu(ohdr->bth[2]) & 0x7fffffff;
}

void hfi1_trace_parse_16b_hdr(struct hfi1_16b_header *hdr,
			      u8 *age, u8 *becn, u8 *fecn, u8 *l4, u8 *rc,
			      u8 *sc, u16 *entropy, u16 *len, u16 *pkey,
			      u32 *dlid, u32 *slid, u8 *lnh, u8 *lver, u8 *sl)
{
	u32 h0 = hdr->lrh[0];
	u32 h1 = hdr->lrh[1];
	u32 h2 = hdr->lrh[2];
	u32 h3 = hdr->lrh[3];

	*l4 = OPA_16B_GET_L4(h0, h1, h2, h3);
	*rc = OPA_16B_GET_RC(h0, h1, h2, h3);
	*sc = OPA_16B_GET_SC(h0, h1, h2, h3);
	*age = OPA_16B_GET_AGE(h0, h1, h2, h3);
	*len = OPA_16B_GET_LEN(h0, h1, h2, h3);
	*becn = OPA_16B_GET_BECN(h0, h1, h2, h3);
	*dlid = OPA_16B_GET_DLID(h0, h1, h2, h3);
	*fecn = OPA_16B_GET_FECN(h0, h1, h2, h3);
	*pkey = OPA_16B_GET_PKEY(h0, h1, h2, h3);
	*slid = OPA_16B_GET_SLID(h0, h1, h2, h3);
	*entropy = OPA_16B_GET_ENTROPY(h0, h1, h2, h3);
	/* zero out 9B only fields */
	*lnh = 0;
	*lver = 0;
	*sl = 0;
}

void hfi1_trace_parse_9b_hdr(struct hfi1_ib_header *hdr,
			     u8 *lnh, u8* lver, u8 *sl, u8 *sc,
			     u16 *len, u32 *dlid, u32 *slid,
			     u8 *age, u8 *becn, u8 *fecn, u8 *l4,
			     u8 *rc, u16 *entropy)
{
	*lnh = (u8)(be16_to_cpu(hdr->lrh[0]) & 3);
	*lver = (u8)(be16_to_cpu(hdr->lrh[0]) >> 8) & 0xf;
	*sl = (u8)(be16_to_cpu(hdr->lrh[0]) >> 4) & 0xf;
	*sc = OPA_9B_GET_SC5(be16_to_cpu(hdr->lrh[0]));
	*len = be16_to_cpu(hdr->lrh[2]);
	*dlid = be16_to_cpu(hdr->lrh[1]);
	*slid = be16_to_cpu(hdr->lrh[3]);
	/* zero out 16B only fields */
	*age = 0;
	*becn = 0;
	*fecn = 0;
	*l4 = 0;
	*rc = 0;
	*entropy = 0;
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
		trace_seq_printf(p, AETH_PRN, be32_to_cpu(eh->aeth) >> 24,
				 parse_syndrome(be32_to_cpu(eh->aeth) >> 24),
				 be32_to_cpu(eh->aeth) & HFI1_MSN_MASK);
		break;
	/* aeth + atomicacketh */
	case OP(RC, ATOMIC_ACKNOWLEDGE):
		trace_seq_printf(p, AETH_PRN " " ATOMICACKETH_PRN,
				 be32_to_cpu(eh->at.aeth) >> 24,
				 parse_syndrome(be32_to_cpu(eh->at.aeth) >> 24),
				 be32_to_cpu(eh->at.aeth) & HFI1_MSN_MASK,
				 (unsigned long long)
				 ib_u64_get(eh->at.atomic_ack_eth));
		break;
	/* atomiceth */
	case OP(RC, COMPARE_SWAP):
	case OP(RC, FETCH_ADD):
		trace_seq_printf(p, ATOMICETH_PRN,
				 (unsigned long long)ib_u64_get(
				 eh->atomic_eth.vaddr),
				 eh->atomic_eth.rkey,
				 (unsigned long long)ib_u64_get(
				 (__be32 *)&eh->atomic_eth.swap_data),
				 (unsigned long long)ib_u64_get(
				 (__be32 *)&eh->atomic_eth.compare_data));
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

const char *parse_sdma_flags(
	struct trace_seq *p,
	u64 desc0, u64 desc1)
{
	const char *ret = trace_seq_buffer_ptr(p);
	char flags[5] = { 'x', 'x', 'x', 'x', 0 };

	flags[0] = (desc1 & SDMA_DESC1_INT_REQ_FLAG) ? 'I' : '-';
	flags[1] = (desc1 & SDMA_DESC1_HEAD_TO_HOST_FLAG) ?  'H' : '-';
	flags[2] = (desc0 & SDMA_DESC0_FIRST_DESC_FLAG) ? 'F' : '-';
	flags[3] = (desc0 & SDMA_DESC0_LAST_DESC_FLAG) ? 'L' : '-';
	trace_seq_printf(p, "%s", flags);
	if (desc0 & SDMA_DESC0_FIRST_DESC_FLAG)
		trace_seq_printf(p, " amode:%u aidx:%u alen:%u",
				 (u8)((desc1 >> SDMA_DESC1_HEADER_MODE_SHIFT) &
				      SDMA_DESC1_HEADER_MODE_MASK),
				 (u8)((desc1 >> SDMA_DESC1_HEADER_INDEX_SHIFT) &
				      SDMA_DESC1_HEADER_INDEX_MASK),
				 (u8)((desc1 >> SDMA_DESC1_HEADER_DWS_SHIFT) &
				      SDMA_DESC1_HEADER_DWS_MASK));
	return ret;
}

const char *print_u32_array(
	struct trace_seq *p,
	u32 *arr, int len)
{
	int i;
	const char *ret = trace_seq_buffer_ptr(p);

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
	const char *ret = trace_seq_buffer_ptr(p);

	for (i = 0; i < len; i++)
		trace_seq_printf(p, "%s0x%016llx", i == 0 ? "" : " ", arr[i]);
	trace_seq_putc(p, 0);
	return ret;
}

__hfi1_trace_fn(PKT);
__hfi1_trace_fn(PROC);
__hfi1_trace_fn(SDMA);
__hfi1_trace_fn(LINKVERB);
__hfi1_trace_fn(DEBUG);
__hfi1_trace_fn(SNOOP);
__hfi1_trace_fn(CNTR);
__hfi1_trace_fn(PIO);
__hfi1_trace_fn(DC8051);
__hfi1_trace_fn(FIRMWARE);
__hfi1_trace_fn(RCVCTRL);
__hfi1_trace_fn(TID);
__hfi1_trace_fn(MMU);
