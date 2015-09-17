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

/* IB header prefixed with 8-bytes of OPA2-specific data */
struct opa_ib_dma_header {
	uint64_t opa2_hdr_reserved;
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
	void *ebuf;
	void *hdr;
	u64 rhf;
	u8 port;
	u16 tlen;
	u16 hlen;
	u32 etype;
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

/*
 * Receive Header Flags
 */
#define RHF_PKT_LEN_SHIFT	0
#define RHF_PKT_LEN_MASK	0xfffull
#define RHF_PKT_LEN_SMASK (RHF_PKT_LEN_MASK << RHF_PKT_LEN_SHIFT)

#define RHF_EGR_INDEX_SHIFT	12
#define RHF_EGR_INDEX_MASK	0x7ffull
#define RHF_EGR_INDEX_SMASK (RHF_EGR_INDEX_MASK << RHF_EGR_INDEX_SHIFT)

#define RHF_EGR_OFFSET_SHIFT	24
#define RHF_EGR_OFFSET_MASK	0xffffull
#define RHF_EGR_OFFSET_SMASK (RHF_EGR_OFFSET_MASK << RHF_EGR_OFFSET_SHIFT)

#define RHF_PORT_SHIFT		41
#define RHF_PORT_MASK		0x1ull
#define RHF_PORT_SMASK (RHF_PORT_MASK << RHF_PORT_SHIFT)

#define RHF_RCV_TYPE_ERR_SHIFT	42
#define RHF_RCV_TYPE_ERR_MASK	0x7ul
#define RHF_RCV_TYPE_ERR_SMASK (RHF_RCV_TYPE_ERR_MASK << RHF_RCV_TYPE_ERR_SHIFT)
#define RHF_TID_ERR		(0x1ull << 45)
#define RHF_LEN_ERR		(0x1ull << 46)
#define RHF_ICRC_ERR		(0x1ull << 47)
#define RHF_K_HDR_LEN_ERR	(0x1ull << 53)

#define RHF_HDR_LEN_SHIFT	48
#define RHF_HDR_LEN_MASK	0x1full
#define RHF_HDR_LEN_SMASK (RHF_HDR_LEN_MASK << RHF_HDR_LEN_SHIFT)

#define RHF_DC_INFO_SHIFT	54
#define RHF_DC_INFO_MASK	0x1ull
#define RHF_DC_INFO_SMASK (RHF_DC_INFO_MASK << RHF_DC_INFO_SHIFT)

#define RHF_USE_EGR_BFR_SHIFT	55
#define RHF_USE_EGR_BFR_MASK	0x1ull
#define RHF_USE_EGR_BFR_SMASK (RHF_USE_EGR_BFR_MASK << RHF_USE_EGR_BFR_SHIFT)

#define RHF_RCV_TYPE_SHIFT	56
#define RHF_RCV_TYPE_MASK	0x3full
#define RHF_RCV_TYPE_SMASK (RHF_RCV_TYPE_MASK << RHF_RCV_TYPE_SHIFT)

/* RHF receive types */
#define RHF_RCV_TYPE_EXPECTED 0x30
#define RHF_RCV_TYPE_EAGER    0x31
#define RHF_RCV_TYPE_IB       0x32 /* normal IB, IB Raw, or IPv6 */
#define RHF_RCV_TYPE_ERROR    0x33
#define RHF_RCV_TYPE_BYPASS   0x34
#define RHF_RCV_TYPE_INVALID5 0x35
#define RHF_RCV_TYPE_INVALID6 0x36
#define RHF_RCV_TYPE_INVALID7 0x37

/* RHF receive type error - expected packet errors */
#define RHF_RTE_EXPECTED_FLOW_SEQ_ERR	0x2
#define RHF_RTE_EXPECTED_FLOW_GEN_ERR	0x4

/* RHF receive type error - eager packet errors */
#define RHF_RTE_EAGER_NO_ERR		0x0

/* RHF receive type error - IB packet errors */
#define RHF_RTE_IB_NO_ERR		0x0

/* RHF receive type error - error packet errors */
#define RHF_RTE_ERROR_NO_ERR		0x0
#define RHF_RTE_ERROR_OP_CODE_ERR	0x1
#define RHF_RTE_ERROR_KHDR_MIN_LEN_ERR	0x2
#define RHF_RTE_ERROR_KHDR_HCRC_ERR	0x3
#define RHF_RTE_ERROR_KHDR_KVER_ERR	0x4
#define RHF_RTE_ERROR_CONTEXT_ERR	0x5
#define RHF_RTE_ERROR_KHDR_TID_ERR	0x6

/* RHF receive type error - bypass packet errors */
#define RHF_RTE_BYPASS_NO_ERR		0x0

static inline __u64 rhf_to_cpu(const __le32 *rbuf)
{
	return __le64_to_cpu(*((__le64 *)rbuf));
}

static inline u32 rhf_rcv_type(u64 rhf)
{
	return (rhf >> RHF_RCV_TYPE_SHIFT) & RHF_RCV_TYPE_MASK;
}

static inline u32 rhf_rcv_type_err(u64 rhf)
{
	return (rhf >> RHF_RCV_TYPE_ERR_SHIFT) & RHF_RCV_TYPE_ERR_MASK;
}

/* return size is in bytes, not DWORDs */
static inline u32 rhf_pkt_len(u64 rhf)
{
	return ((rhf & RHF_PKT_LEN_SMASK) >> RHF_PKT_LEN_SHIFT) << 2;
}

/* return size is in bytes, not DWORDs */
static inline u32 rhf_hdr_len(u64 rhf)
{
	return ((rhf & RHF_HDR_LEN_SMASK) >> RHF_HDR_LEN_SHIFT) << 2;
}

static inline u64 *rhf_get_hdr(struct opa_ib_packet *pkt, u64 *rhf_entry)
{
	return (rhf_entry + 1);
}

static inline u32 rhf_egr_index(u64 rhf)
{
	return (rhf >> RHF_EGR_INDEX_SHIFT) & RHF_EGR_INDEX_MASK;
}

static inline u64 rhf_use_egr_bfr(u64 rhf)
{
	return rhf & RHF_USE_EGR_BFR_SMASK;
}

static inline u64 rhf_sc4(u64 rhf)
{
	return rhf & RHF_DC_INFO_SMASK;
}

static inline u32 rhf_egr_buf_offset(u64 rhf)
{
	return (rhf >> RHF_EGR_OFFSET_SHIFT) & RHF_EGR_OFFSET_MASK;
}

static inline u32 rhf_port(u64 rhf)
{
	return (rhf >> RHF_PORT_SHIFT) & RHF_PORT_MASK;
}

#endif
