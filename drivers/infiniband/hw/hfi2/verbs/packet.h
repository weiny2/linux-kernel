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
 */

/*
 * Intel(R) OPA Gen2 IB Driver
 */

#ifndef _OPA_PACKET_H_
#define _OPA_PACKET_H_

#include <rdma/ib_pack.h>
#include <rdma/opa_addr.h>

#define IB_SEQ_NAK		(3 << 29)

/* AETH NAK opcode values */
#define IB_RNR_NAK                      0x20
#define IB_NAK_PSN_ERROR                0x60
#define IB_NAK_INVALID_REQUEST          0x61
#define IB_NAK_REMOTE_ACCESS_ERROR      0x62
#define IB_NAK_REMOTE_OPERATIONAL_ERROR 0x63
#define IB_NAK_INVALID_RD_REQUEST       0x64

#define IB_16B_BTH_MIG_REQ	BIT(31)

#define IB_GRH_VERSION		6
#define IB_GRH_VERSION_MASK	0xF
#define IB_GRH_VERSION_SHIFT	28
#define IB_GRH_TCLASS_MASK	0xFF
#define IB_GRH_TCLASS_SHIFT	20
#define IB_GRH_FLOW_MASK	0xFFFFF
#define IB_GRH_FLOW_SHIFT	0
#define IB_GRH_NEXT_HDR		0x1B

#define SIZE_OF_CRC 1
#define SIZE_OF_LT 1

#define HFI1_LRH_GRH 0x0003      /* 1. word of IB LRH - next header: GRH */
#define HFI1_LRH_BTH 0x0002      /* 1. word of IB LRH - next header: BTH */

/* L4 Encoding for 8B,10B,16B packets */
/* FXRTODO: Use correct L4 type for management packets */
#define HFI1_L4_IB_MGMT   0x0008
#define HFI1_L4_IB_LOCAL  0x0009
#define HFI1_L4_IB_GLOBAL 0x000A

#define HFI1_PERMISSIVE_LID 0xFFFF
#define HFI1_16B_PERMISSIVE_LID 0xFFFFFF
#define HFI1_MULTICAST_LID_BASE 0xC000
#define HFI1_16B_MULTICAST_LID_BASE 0xF00000

struct hfi2_16b_header {
	u32 lrh[4];
	union {
		struct {
			struct ib_grh grh;
			struct ib_other_headers oth;
		} l;
		struct ib_other_headers oth;
	} u;
} __packed;

/**
 * 0xF8 - 4 bits of multicast range and 1 bit for collective range
 * Example: For 24 bit LID space,
 * Multicast range: 0xF00000 to 0xF7FFFF
 * Collective range: 0xF80000 to 0xFFFFFE
 */
#define OPA_MCAST_MASK ((0xFFFFFFFF << (32 - OPA_MCAST_NR)) >> 8)
#define OPA_COLLECTIVE_MASK ((0xFFFFFFFF <<                               \
			      (32 - (OPA_MCAST_NR + OPA_COLLECTIVE_NR))) >> 8)

/* Packet header with either IB or OPA 16B format */
union hfi2_packet_header {
	struct ib_header ibh;
	struct hfi2_16b_header opah;
} __packed;

/*
 * 9B IB header prefixed with 8-bytes of OPA2-specific data.
 * 16B header is not prefixed with padding as matches TX command.
 * Tail padded to 128B to allow for 64B writes to command queue
 * for OFED_DMA commands.
 */
union hfi2_ib_dma_header {
	struct {
		u64 opa2_hdr_reserved;
		struct ib_header ibh;
	} ph;
	struct hfi2_16b_header opah;
	u64 padding[16]; /* 128 bytes */
} __packed;

/*
 * Represents a single packet at a high level. Put commonly computed things in
 * here so we do not have to keep doing them over and over. The rule of thumb is
 * if something is used one time to derive some value, store that something in
 * here. If it is used multiple times, then store the result of that derivation
 * in here.
 */
struct hfi2_ib_packet {
	struct hfi2_ibrcv *rcv_ctx;
	struct hfi2_ibport *ibp;
	void *ebuf;
	void *hdr;
	struct ib_other_headers *ohdr;
	struct ib_grh *grh;
	struct rvt_qp *qp;
	void *payload;
	u64 rhf;
	u8 port;
	u8 etype;
	u8 sc4_bit;
	int numpkt;
	u16 tlen;
	u16 hlen;
	u16 hlen_9b;
	u16 pkey;
	u32 slid;
	u32 dlid;
	u8 pad;
	u8 extra_byte;
	u8 sl;
	u8 sc;
	u8 opcode;
	bool fecn;
	bool is_mcast;
	bool migrated;
};

/*
 * Store information for sent WQE.
 * Keeping this structure <= 32 bytes keeps it in the 32 byte slab.
 * The first 3 fields are needed in order to call send_complete()
 * or rc_send_complete(), and put_mr() in the send EQ callback.
 * @qp: QP for this send work request, unset for ACKs sent via send_ack()
 * @wqe: WQE set for UD/UC queue pairs only
 * @mr: MR set for RC READ responses only
 * @opcode: BTH.opcode field in CPU endianness
 * @bth2: last 4 bytes of BTH field in CPU endianness
 * @remaining_bytes: bytes remaining in work request (informs if LAST/ONLY)
 */
struct hfi2_wqe_dma {
	struct rvt_qp *qp;
	union {
		struct rvt_swqe *wqe;
		struct rvt_mregion *mr;
	};
	u32 opcode;
	u32 bth2;
	u32 remaining_bytes;
	bool is_ib_dma;
};

/*
 * iov_cache size is estimated to be 4, based on 1 for packet header.
 * 3 for payload, as 8k mtu can span atmost 3 pages.
 */
#define WQE_GDMA_IOV_CACHE_SIZE 4

/*
 * hfi2_wqe_iov is a heavily used structure in TX path which
 * we manage using a kmem cache. It is of size 256 bytes and stores
 * IOVEC array information for General DMA command. HFI2 hw will
 * read the packet header from this structure when processing these
 * commands. An inline IOV cache is here for small SGLs, but large
 * SGL will allocate IOV array on demand."
 */
struct hfi2_wqe_iov {
	struct hfi2_wqe_dma dma;
	union base_iovec *iov;
	/*
	 * For best performance we align both packet header and
	 * IOV array to 64 bytes.
	 */
	union hfi2_packet_header ph __aligned(64);
	union base_iovec iov_cache[WQE_GDMA_IOV_CACHE_SIZE] __aligned(64);
};

void wqe_iov_dma_cache_put(struct hfi2_ibdev *ibd,
			   struct hfi2_wqe_dma *wqe_dma);

/* where best to put this opcode? */
#define CNP_OPCODE 0x80

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
#define PSN_MASK 0x7FFFFFFF
#define PSN_SHIFT 1
#define PSN_MODIFY_MASK 0xFFFFFF

/*
 * Compare two PSNs
 * Returns an integer <, ==, or > than zero.
 */
static inline int cmp_psn(u32 a, u32 b)
{
	return (((int)a) - ((int)b)) << PSN_SHIFT;
}

/*
 * Return masked PSN
 */
static inline u32 mask_psn(u32 a)
{
	return a & PSN_MASK;
}

/*
 * Return delta between two PSNs
 */
static inline u32 delta_psn(u32 a, u32 b)
{
	return (((int)a - (int)b) << PSN_SHIFT) >> PSN_SHIFT;
}

static inline void hfi2_make_opa_lid(struct rdma_ah_attr *attr)
{
	u32 dlid = rdma_ah_get_dlid(attr);
	const struct ib_global_route *grh = rdma_ah_read_grh(attr);

	/* Modify ah_attr.dlid to be in the 32 bit LID space.
	 * This is how the address will be laid out:
	 * Assuming MCAST_NR to be 4,
	 * 32 bit permissive LID = 0xFFFFFFFF
	 * Multicast LID range = 0xFFFFFFFE to 0xF0000000
	 * Unicast LID range = 0xEFFFFFFF to 1
	 * Invalid LID = 0
	 */
	if (ib_is_opa_gid(&grh->dgid))
		dlid = opa_get_lid_from_gid(&grh->dgid);
	else if ((dlid >= be16_to_cpu(IB_MULTICAST_LID_BASE)) &&
		 (dlid != be16_to_cpu(IB_LID_PERMISSIVE)) &&
		 (dlid != be32_to_cpu(OPA_LID_PERMISSIVE)))
		dlid = dlid - be16_to_cpu(IB_MULTICAST_LID_BASE) +
			opa_get_mcast_base(OPA_MCAST_NR);
	else if (dlid == be16_to_cpu(IB_LID_PERMISSIVE))
		dlid = be32_to_cpu(OPA_LID_PERMISSIVE);

	rdma_ah_set_dlid(attr, dlid);
}

/**
 * hfi2_make_grh - construct a GRH header
 * @ibp: a pointer to the IB port
 * @hdr: a pointer to the GRH header being constructed
 * @grh: the global route address to send to
 * @hwords: size of header after grh being sent in dwords
 * @nwords: size of data being sent in dwords
 *
 * Return the size of the header in 32 bit words.
 */
static inline
u32 hfi2_make_grh(struct hfi2_ibport *ibp, struct ib_grh *hdr,
		  const struct ib_global_route *grh, u32 hwords, u32 nwords)
{
	hdr->version_tclass_flow =
		cpu_to_be32((IB_GRH_VERSION << IB_GRH_VERSION_SHIFT) |
			    (grh->traffic_class << IB_GRH_TCLASS_SHIFT) |
			    (grh->flow_label << IB_GRH_FLOW_SHIFT));
	hdr->paylen = cpu_to_be16((hwords + nwords) << 2);
	/* next_hdr is defined by C8-7 in ch. 8.4.1 */
	hdr->next_hdr = IB_GRH_NEXT_HDR;
	hdr->hop_limit = grh->hop_limit;
	/* The SGID is 32-bit aligned. */
	hdr->sgid.global.subnet_prefix = ibp->rvp.gid_prefix;
	hdr->sgid.global.interface_id =
		grh->sgid_index < HFI2_GUIDS_PER_PORT ?
			get_sguid(ibp->ppd, grh->sgid_index) :
			get_sguid(ibp->ppd, HFI2_PORT_GUID_INDEX);

	hdr->dgid = grh->dgid;

	/* GRH header size in 32-bit words. */
	return sizeof(struct ib_grh) / sizeof(u32);
}

/* Bypass packet types */
#define OPA_BYPASS_HDR_8B              0x0
#define OPA_BYPASS_HDR_10B             0x1
#define OPA_BYPASS_HDR_16B             0x2
#define HFI2_PKT_TYPE_9B		0
#define HFI2_PKT_TYPE_16B		1

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
#define RHF_ERROR_SHIFT		42
#define RHF_ERROR_MASK		0x83full  /* 7 error bits 53,47:42 */
#define RHF_ERROR_SMASK (RHF_ERROR_MASK << RHF_ERROR_SHIFT)

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
#define RHF_RCV_TYPE_BASE     0x30
#define RHF_RCV_TYPE_EXPECTED 0x0
#define RHF_RCV_TYPE_EAGER    0x1
#define RHF_RCV_TYPE_IB       0x2 /* normal IB, IB Raw, or IPv6 */
#define RHF_RCV_TYPE_ERROR    0x3
#define RHF_RCV_TYPE_BYPASS   0x4
#define RHF_RCV_TYPE_INVALID5 0x5
#define RHF_RCV_TYPE_INVALID6 0x6
#define RHF_RCV_TYPE_INVALID7 0x7

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

static inline u32 rhf_rcv_type(u64 rhf)
{
	return ((rhf >> RHF_RCV_TYPE_SHIFT) & RHF_RCV_TYPE_MASK) -
	       RHF_RCV_TYPE_BASE;
}

static inline u64 rhf_err_flags(u64 rhf)
{
	return rhf & RHF_ERROR_SMASK;
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

static inline u64 *rhf_get_hdr(struct hfi2_ib_packet *pkt, u64 *rhf_entry)
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
	return ((rhf >> RHF_EGR_OFFSET_SHIFT) & RHF_EGR_OFFSET_MASK) << 6;
}

static inline u32 rhf_port(u64 rhf)
{
	return (rhf >> RHF_PORT_SHIFT) & RHF_PORT_MASK;
}

/**
 * hfi2_check_mcast- Check if the given lid is
 * in the OPA multicast range.
 *
 * The LID might either reside in ah.dlid or might be
 * in the GRH of the address handle as DGID if extended
 * addresses are in use.
 */
static inline bool hfi2_check_mcast(u32 lid)
{
	return ((lid >= opa_get_mcast_base(OPA_MCAST_NR)) &&
		(lid != be32_to_cpu(OPA_LID_PERMISSIVE)));
}

#define opa_get_lid(lid, format)        \
	__opa_get_lid(lid, OPA_PORT_PACKET_FORMAT_##format)

#define IS_EXT_LID(x) (ib_is_opa_gid(x) &&              \
		       (opa_get_lid_from_gid(x) >=      \
		       HFI1_MULTICAST_LID_BASE))

/* Convert a lid to a specific lid space */
static inline u32 __opa_get_lid(u32 lid, u8 format)
{
	bool is_mcast = hfi2_check_mcast(lid);

	switch (format) {
	case OPA_PORT_PACKET_FORMAT_8B:
	case OPA_PORT_PACKET_FORMAT_10B:
		if (is_mcast)
			return (lid - opa_get_mcast_base(OPA_MCAST_NR) +
				0xF0000);
		return lid & 0xFFFFF;
	case OPA_PORT_PACKET_FORMAT_16B:
		if (is_mcast)
			return (lid - opa_get_mcast_base(OPA_MCAST_NR) +
				0xF00000);
		return lid & 0xFFFFFF;
	case OPA_PORT_PACKET_FORMAT_9B:
		if (is_mcast)
			return (lid -
				opa_get_mcast_base(OPA_MCAST_NR) +
				be16_to_cpu(IB_MULTICAST_LID_BASE));
		else
			return lid & 0xFFFF;
	default:
		return lid;
	}
}

/* Return true if the given lid is the OPA 16B multicast range */
static inline bool hfi2_is_16B_mcast(u32 lid)
{
	return ((lid >=
		opa_get_lid(opa_get_mcast_base(OPA_MCAST_NR), 16B)) &&
		(lid != opa_get_lid(be32_to_cpu(OPA_LID_PERMISSIVE), 16B)));
}

static inline int hfi2_get_packet_type(u32 lid)
{
	/* 9B if lid > 0xF0000000 */
	if (lid >= opa_get_mcast_base(OPA_MCAST_NR))
		return HFI2_PKT_TYPE_9B;

	/* 16B if lid > 0xC000 */
	if (lid >= opa_get_lid(opa_get_mcast_base(OPA_MCAST_NR), 9B))
		return HFI2_PKT_TYPE_16B;

	return HFI2_PKT_TYPE_9B;
}

static inline bool hfi2_get_hdr_type(u32 lid, struct rdma_ah_attr *attr)
{
	/*
	 * If there was an incoming 16B packet with permissive
	 * LIDs, OPA GIDs would have been programmed when those
	 * packets were received. A 16B packet will have to
	 * be sent in response to that packet. Return a 16B
	 * header type if that's the case.
	 */
	if (rdma_ah_get_dlid(attr) == be32_to_cpu(OPA_LID_PERMISSIVE))
		return (ib_is_opa_gid(&rdma_ah_read_grh(attr)->dgid)) ?
			HFI2_PKT_TYPE_16B : HFI2_PKT_TYPE_9B;

	/*
	 * Return a 16B header type if either the the destination
	 * or source lid is extended.
	 */
	if (hfi2_get_packet_type(rdma_ah_get_dlid(attr)) == HFI2_PKT_TYPE_16B)
		return HFI2_PKT_TYPE_16B;

	return hfi2_get_packet_type(lid);
}

/**
 * hfi2_make_ext_grh - Make grh for extended LID case
 *
 * This function is called upon UD receive for 16B packets.
 * Since GRH was stripped out when creating 16B on the send side,
 * it is added back on the receive side.
 */
static inline void hfi2_make_ext_grh(struct hfi2_ibport *ibp,
				     struct ib_grh *grh, u32 slid, u32 dlid)
{
	grh->hop_limit = 1;
	grh->sgid.global.subnet_prefix = ibp->rvp.gid_prefix;
	if (slid == opa_get_lid(be32_to_cpu(OPA_LID_PERMISSIVE), 16B))
		grh->sgid.global.interface_id =
			OPA_MAKE_ID(be32_to_cpu(OPA_LID_PERMISSIVE));
	else
		grh->sgid.global.interface_id = OPA_MAKE_ID(slid);

	/*
	 * Upper layers (like mad) may compare the dgid in the
	 * wc that is obtained here with the sgid_index in
	 * the wr. Since sgid_index in wr is always 0 for
	 * extended lids, set the dgid here to the default
	 * IB gid.
	 */
	grh->dgid.global.subnet_prefix = ibp->rvp.gid_prefix;
	grh->dgid.global.interface_id = get_sguid(ibp->ppd,
						  HFI2_PORT_GUID_INDEX);
}

/* OPA 16B Header fields */
#define OPA_16B_LID_MASK	0xFFFFFull
#define OPA_16B_SLID_SHIFT	20
#define OPA_16B_SLID_HIGH_SHIFT	8
#define OPA_16B_SLID_MASK	0xF00ull
#define OPA_16B_DLID_SHIFT	20
#define OPA_16B_DLID_MASK	0xF000ull
#define OPA_16B_DLID_HIGH_SHIFT	12
#define OPA_16B_LEN_SHIFT	20
#define OPA_16B_LEN_MASK	0x7FF00000ull
#define OPA_16B_BECN_SHIFT	31
#define OPA_16B_BECN_MASK	0x80000000ull
#define OPA_16B_FECN_SHIFT	28
#define OPA_16B_FECN_MASK	0x10000000ull
#define OPA_16B_SC_SHIFT	20
#define OPA_16B_SC_MASK		0x1F00000ull
#define OPA_16B_PKEY_SHIFT	16
#define OPA_16B_PKEY_MASK	0xFFFF0000ull
#define OPA_BYPASS_L2_OFFSET	1
#define OPA_BYPASS_L2_SHIFT	29
#define OPA_BYPASS_L2_MASK	0x3ull
#define OPA_16B_L4_MASK		0xFFull
#define OPA_16B_L2_MASK		0x60000000ull
#define OPA_16B_L2_SHIFT	29

#define OPA_BYPASS_GET_L2_TYPE(data)                         \
	((*((u32 *)(data) + OPA_BYPASS_L2_OFFSET) >>         \
	  OPA_BYPASS_L2_SHIFT) & OPA_BYPASS_L2_MASK)

#define OPA_16B_BTH_GET_PAD(ohdr) ((u8)((be32_to_cpu((ohdr)->bth[0]) >>	\
					 OPA_16B_BTH_PAD_SHIFT) \
					 & OPA_16B_BTH_PAD_BITS))

#define OPA_16B_MAKE_QW(low_dw, high_dw) (((u64)(high_dw) << 32) | (low_dw))

static inline void replace_sc(union hfi2_packet_header *ph, u8 sl, bool use_16b)
{
	if (use_16b) {
		u32 h1;

		h1 = ph->opah.lrh[1] & ~OPA_16B_SC_MASK;
		h1 |=  (sl & 0x1F) << OPA_16B_SC_SHIFT;
		ph->opah.lrh[1] = h1;
	} else {
		u16 lrh0;

		lrh0 = (u16)(ph->ibh.lrh[0]) & 0xFF0F;
		lrh0 |= (sl & 0xF) << 4;
		ph->ibh.lrh[0] = (__be16)lrh0;
	}
}

/*
 * Wrapper for above function due to differences between the different
 * packet structures that are used.
 */
static inline void replace_sc_dma(union hfi2_ib_dma_header *s_hdr, u8 sl,
				  bool use_16b)
{
	void *ph;

	if (use_16b)
		ph = &s_hdr->opah;
	else
		ph = &s_hdr->ph.ibh;
	replace_sc(ph, sl, use_16b);
}

/* extract service channel from header and rhf */
static inline int hfi2_9B_get_sc5(struct ib_header *hdr, u64 rhf)
{
	return ib_get_sc(hdr) | ((!!(rhf_sc4(rhf))) << 4);
}

/* OPA 16B packet */
static inline u8 hfi2_16B_get_l4(struct hfi2_16b_header *hdr)
{
	return (u8)(hdr->lrh[2] & OPA_16B_L4_MASK);
}

static inline u8 hfi2_16B_get_sc(struct hfi2_16b_header *hdr)
{
	return (u8)((hdr->lrh[1] & OPA_16B_SC_MASK) >> OPA_16B_SC_SHIFT);
}

static inline u32 hfi2_16B_get_dlid(struct hfi2_16b_header *hdr)
{
	return (u32)((hdr->lrh[1] & OPA_16B_LID_MASK) |
		     (((hdr->lrh[2] & OPA_16B_DLID_MASK) >>
		     OPA_16B_DLID_HIGH_SHIFT) << OPA_16B_DLID_SHIFT));
}

static inline u32 hfi2_16B_get_slid(struct hfi2_16b_header *hdr)
{
	return (u32)((hdr->lrh[0] & OPA_16B_LID_MASK) |
		     (((hdr->lrh[2] & OPA_16B_SLID_MASK) >>
		     OPA_16B_SLID_HIGH_SHIFT) << OPA_16B_SLID_SHIFT));
}

static inline u8 hfi2_16B_get_becn(struct hfi2_16b_header *hdr)
{
	return (u8)((hdr->lrh[0] & OPA_16B_BECN_MASK) >> OPA_16B_BECN_SHIFT);
}

static inline u8 hfi2_16B_get_fecn(struct hfi2_16b_header *hdr)
{
	return (u8)((hdr->lrh[1] & OPA_16B_FECN_MASK) >> OPA_16B_FECN_SHIFT);
}

static inline u8 hfi2_16B_get_l2(struct hfi2_16b_header *hdr)
{
	return (u8)((hdr->lrh[1] & OPA_16B_L2_MASK) >> OPA_16B_L2_SHIFT);
}

static inline u16 hfi2_16B_get_pkt_len(struct hfi2_16b_header *hdr)
{
	return (u16)((hdr->lrh[0] & OPA_16B_LEN_MASK) >> OPA_16B_LEN_SHIFT);
}

static inline u16 hfi2_16B_get_pkey(struct hfi2_16b_header *hdr)
{
	return (u16)((hdr->lrh[2] & OPA_16B_PKEY_MASK) >> OPA_16B_PKEY_SHIFT);
}

/*
 * BTH
 */
#define OPA_16B_BTH_PAD_MASK	7
static inline u8 hfi2_16B_bth_get_pad(struct ib_other_headers *ohdr)
{
	return (u8)((be32_to_cpu(ohdr->bth[0]) >> IB_BTH_PAD_SHIFT) &
		   OPA_16B_BTH_PAD_MASK);
}

static inline int hfi2_get_16b_padding(u32 hdr_size, u32 payload)
{
	u8 extra = ((hdr_size + payload + (SIZE_OF_CRC << 2) +
		     SIZE_OF_LT) % 8);
	return extra ? (8 - extra) : 0;
}

static inline void hfi2_make_ib_hdr(struct ib_header *hdr,
				    u16 lrh0, u16 len,
				    u16 dlid, u16 slid)
{
	hdr->lrh[0] = cpu_to_be16(lrh0);
	hdr->lrh[1] = cpu_to_be16(dlid);
	hdr->lrh[2] = cpu_to_be16(len);
	hdr->lrh[3] = cpu_to_be16(slid);
}

static inline void hfi2_make_16b_hdr(struct hfi2_16b_header *hdr,
				     u32 slid, u32 dlid,
				     u16 len, u16 pkey,
				     bool becn, bool fecn, u8 l4,
				     u8 sc)
{
	u32 lrh0 = 0;
	u32 lrh1 = 0xC0000000;
	u32 lrh2 = 0;
	u32 lrh3 = 0;

	lrh0 = (lrh0 & ~OPA_16B_BECN_MASK) | (becn << OPA_16B_BECN_SHIFT);
	lrh0 = (lrh0 & ~OPA_16B_LEN_MASK) | (len << OPA_16B_LEN_SHIFT);
	lrh0 = (lrh0 & ~OPA_16B_LID_MASK)  | (slid & OPA_16B_LID_MASK);
	lrh1 = (lrh1 & ~OPA_16B_FECN_MASK) | (fecn << OPA_16B_FECN_SHIFT);
	lrh1 = (lrh1 & ~OPA_16B_SC_MASK) | (sc << OPA_16B_SC_SHIFT);
	lrh1 = (lrh1 & ~OPA_16B_LID_MASK) | (dlid & OPA_16B_LID_MASK);
	lrh2 = (lrh2 & ~OPA_16B_SLID_MASK) |
		((slid >> OPA_16B_SLID_SHIFT) << OPA_16B_SLID_HIGH_SHIFT);
	lrh2 = (lrh2 & ~OPA_16B_DLID_MASK) |
		((dlid >> OPA_16B_DLID_SHIFT) << OPA_16B_DLID_HIGH_SHIFT);
	lrh2 = (lrh2 & ~OPA_16B_PKEY_MASK) | (pkey << OPA_16B_PKEY_SHIFT);
	lrh2 = (lrh2 & ~OPA_16B_L4_MASK) | l4;

	hdr->lrh[0] = lrh0;
	hdr->lrh[1] = lrh1;
	hdr->lrh[2] = lrh2;
	hdr->lrh[3] = lrh3;
}

static inline bool opa_bth_is_migration(struct ib_other_headers *ohdr)
{
	return (ohdr->bth[1] & cpu_to_be32(IB_16B_BTH_MIG_REQ));
}
#endif
