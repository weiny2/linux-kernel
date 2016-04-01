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
#define IB_16B_BTH_MIG_REQ	(1 << 31)

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

/* L4 Encoding for 8B,10B,16B packets */
/* FXRTODO: Use correct L4 type for management packets */
#define HFI1_L4_IB_MGMT   0x0008
#define HFI1_L4_IB_LOCAL  0x0009
#define HFI1_L4_IB_GLOBAL 0x000A

#define HFI1_PERMISSIVE_LID 0xFFFF
#define HFI1_16B_PERMISSIVE_LID 0xFFFFFF
#define HFI1_AETH_CREDIT_SHIFT 24
#define HFI1_AETH_CREDIT_MASK 0x1F
#define HFI1_AETH_CREDIT_INVAL 0x1F
#define HFI1_MSN_MASK 0xFFFFFF
#define HFI1_QPN_MASK 0xFFFFFF
#define HFI1_FECN_SHIFT 31
#define HFI1_FECN_MASK 0x1
#define HFI1_FECN_SMASK BIT(HFI1_FECN_SHIFT)
#define HFI1_BECN_SHIFT 30
#define HFI1_BECN_MASK 0x1
#define HFI1_BECN_SMASK BIT(HFI1_BECN_SHIFT)
#define HFI1_MULTICAST_LID_BASE 0xC000
#define HFI1_16B_MULTICAST_LID_BASE 0xF00000

/**
 * 0xF8 - 4 bits of multicast range and 1 bit for collective range
 * Example: For 24 bit LID space,
 * Multicast range: 0xF00000 to 0xF7FFFF
 * Collective range: 0xF80000 to 0xFFFFFE
 */
#define HFI1_MCAST_NR 0x4 /* Number of top bits set */
#define HFI1_COLLECTIVE_NR 0x1 /* Number of bits after MCAST_NR */
#define HFI1_MCAST_MASK ((0xFFFFFFFF << (32 - HFI1_MCAST_NR)) >> 8)
#define HFI1_COLLECTIVE_MASK ((0xFFFFFFFF <<                               \
			(32 - (HFI1_MCAST_NR + HFI1_COLLECTIVE_NR))) >> 8)

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
struct hfi2_ib_header {
	__be16 lrh[4];
	union {
		struct {
			struct ib_grh grh;
			struct ib_l4_headers oth;
		} l;
		struct ib_l4_headers oth;
	} u;
} __packed;

/* OPA 16B header structure with BTH, GRH and DETH */
struct hfi2_opa16b_header {
	u32 opah[4];
	union {
		struct {
			struct ib_grh grh;
			struct ib_l4_headers oth;
		} l;
		struct ib_l4_headers oth;
	} u;
} __packed;

/* Packet header with either IB or OPA 16B format */
union hfi2_packet_header {
	struct hfi2_ib_header ibh;
	struct hfi2_opa16b_header opa16b;
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
		struct hfi2_ib_header ibh;
	} ph;
	struct hfi2_opa16b_header opa16b;
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
	struct hfi_ctx *ctx;
	struct hfi2_ibport *ibp;
	void *ebuf;
	void *hdr;
	struct ib_l4_headers *ohdr;
	struct ib_grh *grh;
	u64 rhf;
	u8 port;
	u8 etype;
	u8 sc4_bit;
	u16 tlen;
	u16 hlen;
	u16 hlen_9b;
};

/*
 * Store IOVEC array information for General DMA command.
 * FXR will read the packet header from this structure when processing
 * the General DMA command's first IOVEC entry.
 *
 * The first 3 fields are needed in order to call send_complete()
 * or rc_send_complete(), and put_mr() in the send EQ callback:
 *    QP is null for RC ACKs, set for all other transmits
 *    WQE is set for UD/UC queue pairs only
 *    MR is set for RC READ responses only
 */
struct hfi2_wqe_iov {
	struct hfi2_qp *qp;
	union {
		struct hfi2_swqe *wqe;
		struct hfi2_mregion *mr;
	};
	union hfi2_packet_header ph;
	bool use_16b;
	u32 remaining_bytes;
	union base_iovec iov[0];
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
 * Compare the lower 24 bits of the msn values.
 * Returns an integer <, ==, or > than zero.
 */
static inline int cmp_msn(u32 a, u32 b)
{
	return (((int)a) - ((int)b)) << 8;
}

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
		  struct ib_global_route *grh, u32 hwords, u32 nwords)
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
	hdr->sgid.global.subnet_prefix = ibp->gid_prefix;
	hdr->sgid.global.interface_id =
		(grh->sgid_index && grh->sgid_index <= ARRAY_SIZE(ibp->guids)) ?
		ibp->guids[grh->sgid_index - 1] : ibp->ppd->pguid;

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

static inline __u64 rhf_to_cpu(const __le32 *rbuf)
{
	return __le64_to_cpu(*((__le64 *)rbuf));
}

static inline u32 rhf_rcv_type(u64 rhf)
{
	return ((rhf >> RHF_RCV_TYPE_SHIFT) & RHF_RCV_TYPE_MASK) -
	       RHF_RCV_TYPE_BASE;
}

static inline u32 rhf_rcv_type_err(u64 rhf)
{
	return (rhf >> RHF_RCV_TYPE_ERR_SHIFT) & RHF_RCV_TYPE_ERR_MASK;
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

static inline u32 hfi2_get_lid_from_gid(union ib_gid *gid)
{
	/* Caller should ensure gid is of type opa gid */
	return be64_to_cpu(gid->global.interface_id) & 0xFFFFFFFF;
}

/**
 * hfi1_mcast_xlate - Translate 9B MLID to the 16B MLID range
 */
static inline u32 hfi1_mcast_xlate(u32 lid)
{
	if ((lid >= HFI1_MULTICAST_LID_BASE) &&
	    (lid != HFI1_PERMISSIVE_LID))
		return lid - HFI1_MULTICAST_LID_BASE +
			HFI1_16B_MULTICAST_LID_BASE;
	return lid;
}

/**
 * hfi2_retrieve_lid - Get lid in the GID.
 *
 * Extended LIDs are stored in the GID if the STL
 * device supports extended addresses. This function
 * retirieves the 32 bit lid.
 *
 * If GRH is not specified or if an IB GID is specified
 * in the GRH, the function simply returns the 16 bit lid.
 */
static inline u32 hfi2_retrieve_lid(struct ib_ah_attr *ah_attr)
{
	union ib_gid *dgid;

	if ((ah_attr->ah_flags & IB_AH_GRH)) {
		dgid = &ah_attr->grh.dgid;
		if (ib_is_opa_gid(dgid))
			return hfi2_get_lid_from_gid(dgid);
	}
	return hfi1_mcast_xlate(ah_attr->dlid);
}

/**
 * hfi2_check_mcast- Check if the lid contained
 * in the address handle is a multicast LID.
 *
 * The LID might either reside in ah.dlid or might be
 * in the GRH of the address handle as DGID if extended
 * addresses are in use.
 */
static inline bool hfi2_check_mcast(struct ib_ah_attr *ah_attr)
{
	union ib_gid dgid;
	u32 lid;

	if (ah_attr->ah_flags & IB_AH_GRH) {
		dgid = ah_attr->grh.dgid;
		if (ib_is_opa_gid(&dgid)) {
			lid = hfi2_get_lid_from_gid(&dgid);
			return ((lid >= HFI1_16B_MULTICAST_LID_BASE) &&
				(lid != HFI1_16B_PERMISSIVE_LID));
		}
	}
	lid = ah_attr->dlid;
	return ((lid >= HFI1_MULTICAST_LID_BASE) &&
		(lid != HFI1_PERMISSIVE_LID));
}

/**
 * hfi2_check_permissive- Check if the lid contained
 * in the address handle is a permissive LID.
 *
 * The LID might either reside in ah.dlid or might be
 * in the GRH of the address handle as DGID if extended
 * addresses are in use.
 */
static inline bool hfi2_check_permissive(struct ib_ah_attr *ah_attr)
{
	union ib_gid dgid;

	if (ah_attr->ah_flags & IB_AH_GRH) {
		dgid = ah_attr->grh.dgid;
		if (ib_is_opa_gid(&dgid)) {
			return HFI1_16B_PERMISSIVE_LID ==
				hfi2_get_lid_from_gid(&dgid);
		}
	}
	return ah_attr->dlid == HFI1_PERMISSIVE_LID;
}

#define IS_EXT_LID(x) (ib_is_opa_gid(x) &&              \
		       (hfi2_get_lid_from_gid(x) >=     \
			HFI1_MULTICAST_LID_BASE))

/* Check if current wqe needs 16B */
static inline bool hfi2_use_16b(struct hfi2_qp *qp)
{
	struct hfi2_swqe *wqe = qp->s_wqe;
	struct ib_ah_attr *ah_attr;
	union ib_gid sgid;
	union ib_gid *dgid;

	if ((qp->ibqp.qp_type == IB_QPT_RC) ||
	    (qp->ibqp.qp_type == IB_QPT_UC)) {
		if (ib_query_gid(qp->ibqp.device, qp->port_num,
				 qp->remote_ah_attr.grh.sgid_index,
				 &sgid
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)
				 , NULL
#endif
				 ))
			return false;
		dgid = &qp->remote_ah_attr.grh.dgid;
		return IS_EXT_LID(dgid) || IS_EXT_LID(&sgid);
	}

	if (!wqe)
		return false;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)
	ah_attr = &to_hfi_ah(wqe->ud_wr.ah)->attr;
#else
	ah_attr = &to_hfi_ah(wqe->wr.wr.ud.ah)->attr;
#endif
	if (ib_query_gid(qp->ibqp.device, qp->port_num,
			 ah_attr->grh.sgid_index, &sgid
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)
			 , NULL
#endif
			 ))
		return false;
	dgid = &ah_attr->grh.dgid;
	return IS_EXT_LID(dgid) || IS_EXT_LID(&sgid);
}

static inline void hfi2_make_ext_grh(struct hfi2_ibport *ibp,
				     struct ib_grh *grh, u32 slid, u32 dlid)
{
	grh->hop_limit = 1;
	grh->sgid.global.subnet_prefix = ibp->gid_prefix;
	grh->sgid.global.interface_id = OPA_MAKE_GID(slid);

	/* This is called in the recv codepath where the dlid will
	 * will be the local lid of the node. If this is a DR packet
	 * in which case dlid is permissive, set the default
	 * gid in the GRH.
	 * FXRTODO: The permissive LID check below seems wrong and also
	 * unnecessary. Revisit later.
	 */
	grh->dgid.global.subnet_prefix = ibp->gid_prefix;
	if ((dlid == HFI1_16B_PERMISSIVE_LID) ||
	    (dlid == HFI1_PERMISSIVE_LID))
		grh->dgid.global.interface_id = ibp->ppd->pguid;
	else
		grh->dgid.global.interface_id = OPA_MAKE_GID(dlid);
}

/* Bypass packet types */
#define OPA_BYPASS_HDR_8B              0x0
#define OPA_BYPASS_HDR_10B             0x1
#define OPA_BYPASS_HDR_16B             0x2

/* OPA 16B Header fields */
#define OPA_16B_LID_MASK        0xFFFFFull
#define OPA_16B_SLID_SHFT       8
#define OPA_16B_SLID_MASK       0xF00ull
#define OPA_16B_DLID_SHFT       12
#define OPA_16B_DLID_MASK       0xF000ull
#define OPA_16B_LEN_SHFT        20
#define OPA_16B_LEN_MASK        0x7FF00000ull
#define OPA_16B_BECN_SHFT       31
#define OPA_16B_BECN_MASK       0x80000000ull
#define OPA_16B_FECN_SHFT       28
#define OPA_16B_FECN_MASK       0x10000000ull
#define OPA_16B_SC_SHFT         20
#define OPA_16B_SC_MASK         0x1F00000ull
#define OPA_16B_RC_SHFT         25
#define OPA_16B_RC_MASK         0xE000000ull
#define OPA_16B_PKEY_SHFT       16
#define OPA_16B_PKEY_MASK       0xFFFF0000ull
#define OPA_BYPASS_L2_OFFSET    1
#define OPA_BYPASS_L2_SHFT      29
#define OPA_BYPASS_L2_MASK      0x3ull
#define OPA_16B_L4_OFFSET       2
#define OPA_16B_L4_MASK         0xFFull
#define OPA_16B_ENTROPY_MASK    0xFFFFull
#define OPA_16B_AGE_SHFT        16
#define OPA_16B_AGE_MASK        0xFF0000ull

#define OPA_BYPASS_GET_L2_TYPE(data)                         \
	((*((u32 *)(data) + OPA_BYPASS_L2_OFFSET) >>         \
	  OPA_BYPASS_L2_SHFT) & OPA_BYPASS_L2_MASK)

#define OPA_16B_GET_L4_TYPE(data)                            \
	(*((u32 *)(data) + OPA_16B_L4_OFFSET) & OPA_16B_L4_MASK)

static inline u32 opa_16b_get_dlid(u32 *hdr)
{
	u32 h1 = hdr[1];
	u32 h2 = hdr[2];

	/* Append upper 4 bits with lower 20 bits of dlid */
	return ((h1 & OPA_16B_LID_MASK) |
		(((h2 & OPA_16B_DLID_MASK) >> OPA_16B_DLID_SHFT) << 20));
}

static inline u32 opa_16b_pkt_len(u32 *hdr)
{
	u32 h0 = *hdr;

	/* packet field in qwords, return length in bytes */
	return ((h0 & OPA_16B_LEN_MASK) >> OPA_16B_LEN_SHFT) << 3;
}

static inline void opa_make_16b_header(u32 *hdr, u32 slid, u32 dlid, u16 len,
				       u16 pkey, u16 entropy, u8 sc, u8 rc,
				       bool fecn, bool becn, u8 age, u8 l4)
{
	u32 h0 = 0;
	u32 h1 = 0x40000000; /* 16B L2=10 */
	u32 h2 = 0;
	u32 h3 = 0;

	/* Extract and set 4 upper bits and 20 lower bits of the lids */
	h0 |= (slid & OPA_16B_LID_MASK);
	h2 |= ((slid >> (20 - OPA_16B_SLID_SHFT)) & OPA_16B_SLID_MASK);

	h0 |= (len << OPA_16B_LEN_SHFT);
	if (becn)
		h0 |= OPA_16B_BECN_MASK;

	h1 |= (dlid & OPA_16B_LID_MASK);
	h2 |= ((dlid >> (20 - OPA_16B_DLID_SHFT)) & OPA_16B_DLID_MASK);

	h1 |= (rc << OPA_16B_RC_SHFT);
	h1 |= (sc << OPA_16B_SC_SHFT);
	if (fecn)
		h1 |= OPA_16B_FECN_MASK;

	h2 |= l4;
	h2 |= ((u32)pkey << OPA_16B_PKEY_SHFT);

	h3 |= entropy;
	h3 |= ((u32)age << OPA_16B_AGE_SHFT);

	hdr[0] = h0;
	hdr[1] = h1;
	hdr[2] = h2;
	hdr[3] = h3;
}

static inline void opa_parse_16b_header(u32 *hdr, u32 *slid, u32 *dlid,
					u16 *len, u16 *pkey, u16 *entropy,
					u8 *sc, u8 *rc, bool *fecn, bool *becn,
					u8 *age, u8 *l4)
{
	u32 h0 = *hdr++;
	u32 h1 = *hdr++;
	u32 h2 = *hdr++;
	u32 h3 = *hdr;

	/* Append upper 4 bits with lower 20 bits of lid */
	*slid = (h0 & OPA_16B_LID_MASK) |
		(((h2 & OPA_16B_SLID_MASK) >> OPA_16B_SLID_SHFT) << 20);
	*dlid = (h1 & OPA_16B_LID_MASK) |
		(((h2 & OPA_16B_DLID_MASK) >> OPA_16B_DLID_SHFT) << 20);

	*len  = (h0 & OPA_16B_LEN_MASK)  >> OPA_16B_LEN_SHFT;
	*becn = !!((h0 & OPA_16B_BECN_MASK) >> OPA_16B_BECN_SHFT);

	*sc = (h1 & OPA_16B_SC_MASK) >> OPA_16B_SC_SHFT;
	*rc = (h1 & OPA_16B_RC_MASK) >> OPA_16B_RC_SHFT;
	*fecn = !!((h1 & OPA_16B_FECN_MASK) >> OPA_16B_FECN_SHFT);

	*l4 = h2 & OPA_16B_L4_MASK;
	*pkey = (h2 & OPA_16B_PKEY_MASK) >> OPA_16B_PKEY_SHFT;

	*entropy = h3 & OPA_16B_ENTROPY_MASK;
	*age = (h3 & OPA_16B_AGE_MASK) >> OPA_16B_AGE_SHFT;
}
#endif
