/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2015 Intel Corporation.
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
 *
 * Intel(R) OPA Gen2 IB Driver
 */

#ifndef _OPA_PACKET_H_
#define _OPA_PACKET_H_

#include <rdma/ib_pack.h>

#define IB_SEQ_NAK		(3 << 29)

/* AETH NAK opcode values */
#define IB_RNR_NAK                      0x20
#define IB_NAK_PSN_ERROR                0x60
#define IB_NAK_INVALID_REQUEST          0x61
#define IB_NAK_REMOTE_ACCESS_ERROR      0x62
#define IB_NAK_REMOTE_OPERATIONAL_ERROR 0x63
#define IB_NAK_INVALID_RD_REQUEST       0x64

#define IB_BTH_REQ_ACK		(1 << 31)
#define IB_BTH_SOLICITED	(1 << 23)
#define IB_BTH_MIG_REQ		(1 << 22)

#define IB_GRH_VERSION		6
#define IB_GRH_VERSION_MASK	0xF
#define IB_GRH_VERSION_SHIFT	28
#define IB_GRH_TCLASS_MASK	0xFF
#define IB_GRH_TCLASS_SHIFT	20
#define IB_GRH_FLOW_MASK	0xFFFFF
#define IB_GRH_FLOW_SHIFT	0
#define IB_GRH_NEXT_HDR		0x1B

#define SIZE_OF_CRC 1

#define HFI1_LRH_GRH 0x0003      /* 1. word of IB LRH - next header: GRH */
#define HFI1_LRH_BTH 0x0002      /* 1. word of IB LRH - next header: BTH */

#define HFI1_PERMISSIVE_LID 0xFFFF
#define HFI1_AETH_CREDIT_SHIFT 24
#define HFI1_AETH_CREDIT_MASK 0x1F
#define HFI1_AETH_CREDIT_INVAL 0x1F
#define HFI1_MSN_MASK 0xFFFFFF
#define HFI1_QPN_MASK 0xFFFFFF
#define HFI1_FECN_SHIFT 31
#define HFI1_FECN_MASK 0x1
#define HFI1_BECN_SHIFT 30
#define HFI1_BECN_MASK 0x1
#define HFI1_MULTICAST_LID_BASE 0xC000

struct ib_reth {
	__be64 vaddr;
	__be32 rkey;
	__be32 length;
} __packed;

struct ib_atomic_eth {
	__be32 vaddr[2];        /* unaligned so access as 2 32-bit words */
	__be32 rkey;
	__be64 swap_data;
	__be64 compare_data;
} __packed;

union ib_ehdrs {
	struct {
		__be32 deth[2];
		__be32 imm_data;
	} ud;
	struct {
		struct ib_reth reth;
		__be32 imm_data;
	} rc;
	struct {
		__be32 aeth;
		__be32 atomic_ack_eth[2];
	} at;
	__be32 imm_data;
	__be32 aeth;
	struct ib_atomic_eth atomic_eth;
}  __packed;

struct ib_l4_headers {
	__be32 bth[3];
	union ib_ehdrs u;
} __packed;

/*
 * Note that UD packets with a GRH header are 8+40+12+8 = 68 bytes
 * long (72 w/ imm_data).  Only the first 56 bytes of the IB header
 * will be in the eager header buffer.  The remaining 12 or 16 bytes
 * are in the data buffer.
 */
struct opa_ib_header {
	__be16 lrh[4];
	union {
		struct {
			struct ib_grh grh;
			struct ib_l4_headers oth;
		} l;
		struct ib_l4_headers oth;
	} u;
} __packed;

/* FXRTODO - extend similar to WFR's ahg_ib_header */
struct opa_ib_dma_header {
	struct opa_ib_header ibh;
};

/*
 * Represents a single packet at a high level. Put commonly computed things in
 * here so we do not have to keep doing them over and over. The rule of thumb is
 * if something is used one time to derive some value, store that something in
 * here. If it is used multiple times, then store the result of that derivation
 * in here.
 */
struct opa_ib_packet {
	struct hfi_ctx *ctx;
	struct opa_ib_portdata *ibp;
	/* FXRTODO - verify these makes sense and are set */
	void *ebuf;
	void *hdr;
	u64 rhf;
	u16 tlen;
	u16 hlen;
	u32 updegr;
	u32 rcv_flags;
};

/* where best to put this opcode? */
#define CNP_OPCODE 0x80

/* TODO - 31-bit PSN stuff?? */
/*
 * The PSN_MASK and PSN_SHIFT allow for
 * 1) comparing two PSNs
 * 2) returning the PSN with any upper bits masked
 * 3) returning the difference between to PSNs
 *
 * The number of signficant bits in the PSN must
 * necessarily be at least one bit less than
 * the container holding the PSN.
 */
#define PSN_MASK 0xFFFFFF
#define PSN_SHIFT 8
#define PSN_MODIFY_MASK 0xFFFFFF

/*
 * Compare two PSNs
 * Returns an integer <, ==, or > than zero.
 */
static inline int cmp_psn(u32 a, u32 b)
{
	return (((int) a) - ((int) b)) << PSN_SHIFT;
}

/*
 * Return masked PSN
 */
static inline u32 mask_psn(u32 a)
{
	return a & PSN_MASK;
}

/**
 * opa_ib_make_grh - construct a GRH header
 * @ibp: a pointer to the IB port
 * @hdr: a pointer to the GRH header being constructed
 * @grh: the global route address to send to
 * @hwords: the number of 32 bit words of header being sent
 * @nwords: the number of 32 bit words of data being sent
 *
 * Return the size of the header in 32 bit words.
 */
static inline
u32 opa_ib_make_grh(struct opa_ib_portdata *ibp, struct ib_grh *hdr,
		    struct ib_global_route *grh, u32 hwords, u32 nwords)
{
	hdr->version_tclass_flow =
		cpu_to_be32((IB_GRH_VERSION << IB_GRH_VERSION_SHIFT) |
			    (grh->traffic_class << IB_GRH_TCLASS_SHIFT) |
			    (grh->flow_label << IB_GRH_FLOW_SHIFT));
	hdr->paylen = cpu_to_be16((hwords - 2 + nwords + SIZE_OF_CRC) << 2);
	/* next_hdr is defined by C8-7 in ch. 8.4.1 */
	hdr->next_hdr = IB_GRH_NEXT_HDR;
	hdr->hop_limit = grh->hop_limit;
	/* The SGID is 32-bit aligned. */
	hdr->sgid.global.subnet_prefix = ibp->gid_prefix;
	hdr->sgid.global.interface_id = ibp->guid;
	hdr->dgid = grh->dgid;

	/* GRH header size in 32-bit words. */
	return sizeof(struct ib_grh) / sizeof(u32);
}
#endif
