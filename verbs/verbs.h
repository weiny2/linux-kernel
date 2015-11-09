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

#ifndef _OPA_VERBS_H_
#define _OPA_VERBS_H_

#include <linux/idr.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/version.h>
#include <rdma/ib_verbs.h>
#include <rdma/ib_user_verbs.h>
#include <rdma/opa_smi.h>
#include <rdma/opa_port_info.h>
#include <rdma/opa_core.h>
#include "ib_compat.h"
#include "iowait.h"

/* TODO - these carried from WFR driver */
#define OPA_IB_MAX_RDMA_ATOMIC  16
#define OPA_IB_INIT_RDMA_ATOMIC 255
#define OPA_IB_MAX_MAP_PER_FMR  32767
#define OPA_IB_MAX_MSG_SZ       0x80000000
#define IB_DEFAULT_GID_PREFIX	cpu_to_be64(0xfe80000000000000ULL)
#define OPA_LIM_MGMT_PKEY       0x7FFF
#define OPA_FULL_MGMT_PKEY      0xFFFF

/* Flags for checking QP state (see ib_hfi1_state_ops[]) */
#define HFI1_POST_SEND_OK                0x01
#define HFI1_POST_RECV_OK                0x02
#define HFI1_PROCESS_RECV_OK             0x04
#define HFI1_PROCESS_SEND_OK             0x08
#define HFI1_PROCESS_NEXT_SEND_OK        0x10
#define HFI1_FLUSH_SEND                  0x20
#define HFI1_FLUSH_RECV                  0x40
#define HFI1_PROCESS_OR_FLUSH_SEND \
	(HFI1_PROCESS_SEND_OK | HFI1_FLUSH_SEND)

/* flags passed by opa_ib_rcv() */
#define HFI1_HAS_GRH                     0x1
#define HFI1_SC4_BIT                     0x2

#define OPA_DEFAULT_SM_TRAP_QP			0x0
#define OPA_DEFAULT_SA_QP			0x1

/* TODO - placeholders */
extern __be64 opa_ib_sys_guid;

extern const int ib_qp_state_ops[];
extern unsigned int opa_ib_max_cqes;
extern unsigned int opa_ib_max_cqs;
extern unsigned int opa_ib_max_qp_wrs;
extern unsigned int opa_ib_max_qps;
extern unsigned int opa_ib_max_sges;
extern unsigned int opa_ib_lkey_table_size;
extern unsigned int opa_ib_max_mcast_grps;
extern unsigned int opa_ib_max_mcast_qp_attached;
extern struct ib_dma_mapping_ops opa_ib_dma_mapping_ops;

struct ib_l4_headers;
struct opa_ib_header;
struct opa_ib_packet;

struct opa_ucontext {
	struct ib_ucontext ibucontext;
};

struct opa_ib_pd {
	struct ib_pd ibpd;
	int is_user;
};

struct opa_ib_ah {
	struct ib_ah ibah;
	struct ib_ah_attr attr;
	atomic_t refcount;
};

/*
 * This structure is used by opa_ib_mmap() to validate an offset
 * when an mmap() request is made.  The vm_area_struct then uses
 * this as its vm_private_data.
 */
struct opa_ib_mmap_info {
	struct list_head pending_mmaps;
	struct ib_ucontext *context;
	void *obj;
	__u64 offset;
	struct kref ref;
	unsigned size;
};

/*
 * This structure is used to contain the head pointer, tail pointer,
 * and completion queue entries as a single memory allocation so
 * it can be mmap'ed into user space.
 */
struct opa_ib_cq_wc {
	u32 head;               /* index of next entry to fill */
	u32 tail;               /* index of next ib_poll_cq() entry */
	union {
		/* these are actually size ibcq.cqe + 1 */
		struct ib_uverbs_wc uqueue[0];
		struct ib_wc kqueue[0];
	};
};

struct opa_ib_cq {
	struct ib_cq ibcq;
	struct kthread_work comptask;
	struct opa_ib_data *ibd;
	spinlock_t lock;	/* protect changes in this struct */
	u8 notify;
	u8 triggered;
	struct opa_ib_cq_wc *queue;
	struct opa_ib_mmap_info *ip;
};

struct opa_ib_lkey_table {
	spinlock_t lock;        /* protect changes in this struct */
	u32 gen;                /* generation count */
	u32 max;                /* size of the table */
	struct idr table;
};

/*
 * A segment is a linear region of low physical memory.
 * Used by the verbs layer.
 */
struct hfi2_seg {
	void *vaddr;
	size_t length;
};

/* The number of hfi2_segs that fit in a page. */
#define HFI2_SEGSZ     (PAGE_SIZE / sizeof(struct hfi2_seg))

struct hfi2_segarray {
	struct hfi2_seg segs[HFI2_SEGSZ];
};

struct hfi2_mregion {
	struct ib_pd *pd;       /* shares refcnt of ibmr.pd */
	u64 user_base;          /* User's address for this region */
	u64 iova;               /* IB start address of this region */
	size_t length;
	u32 lkey;
	u32 offset;             /* offset (bytes) to start of region */
	int access_flags;
	u32 max_segs;           /* number of hfi1_segs in all the arrays */
	u32 mapsz;              /* size of the map array */
	u8  page_shift;         /* 0 - non unform/non powerof2 sizes */
	u8  lkey_published;     /* in global table */
	struct completion comp; /* complete when refcount goes to zero */
	atomic_t refcount;
	struct hfi2_segarray *map[0];    /* the segments */
};

struct opa_ib_mr {
	struct ib_mr ibmr;
	struct ib_umem *umem;
	struct hfi2_mregion mr;  /* must be last */
};

static inline void hfi2_get_mr(struct hfi2_mregion *mr)
{
	atomic_inc(&mr->refcount);
}

static inline void hfi2_put_mr(struct hfi2_mregion *mr)
{
	if (unlikely(atomic_dec_and_test(&mr->refcount)))
		complete(&mr->comp);
}

/*
 * These keep track of the copy progress within a memory region.
 * Used by the verbs layer.
 */
struct hfi2_sge {
	struct hfi2_mregion *mr;
	void *vaddr;            /* kernel virtual address of segment */
	u32 sge_length;         /* length of the SGE */
	u32 length;             /* remaining length of the segment */
	u16 m;                  /* current index: mr->map[m] */
	u16 n;                  /* current index: mr->map[m]->segs[n] */
};

/*
 * Send work request queue entry.
 * The size of the sg_list is determined when the QP is created and stored
 * in qp->s_max_sge.
 */
struct opa_ib_swqe {
	struct ib_send_wr wr;   /* don't use wr.sg_list */
	u32 psn;                /* first packet sequence number */
	u32 lpsn;               /* last packet sequence number */
	u32 ssn;                /* send sequence number */
	u32 length;             /* total length of data in sg_list */

	/*
	 * TODO - try maintaining all state here instead of creating a
	 * verbs_txreq and sdma_txreq as WFR does
	 */
	struct list_head pending_list;
	struct opa_ib_sge_state	*s_sge;
	struct opa_ib_qp *s_qp;
	struct opa_ib_dma_header *s_hdr; /* next packet header to send */
	struct hfi_ctx *s_ctx;           /* associated send context */
	u16 s_hdrwords; 	         /* size of s_hdr in 32 bit words */
	u16 pmtu;
	u8 lnh;
	u8 sl;
	bool use_sc15;
	void *s_iov;

	struct hfi2_sge sg_list[0];
};

/*
 * Receive work request queue entry.
 * The size of the sg_list is determined when the QP (or SRQ) is created
 * and stored in qp->r_rq.max_sge (or srq->rq.max_sge).
 */
struct opa_ib_rwqe {
	u64 wr_id;
	u8 num_sge;
	struct ib_sge sg_list[0];
};

/*
 * This structure is used to contain the head pointer, tail pointer,
 * and receive work queue entries as a single memory allocation so
 * it can be mmap'ed into user space.
 * Note that the wq array elements are variable size so you can't
 * just index into the array to get the N'th element;
 * use get_rwqe_ptr() instead.
 */
struct opa_ib_rwq {
	u32 head;               /* new work requests posted to the head */
	u32 tail;               /* receives pull requests from here. */
	struct opa_ib_rwqe wq[0];
};

struct opa_ib_rq {
	struct opa_ib_rwq *wq;
	u32 size;               /* size of RWQE array */
	u8 max_sge;
	/* protect changes in this struct */
	spinlock_t lock ____cacheline_aligned_in_smp;
};

struct opa_ib_srq {
	struct ib_srq ibsrq;
	struct opa_ib_rq rq;
	struct opa_ib_mmap_info *ip;
	/* send signal when number of RWQEs < limit */
	u32 limit;
};

struct opa_ib_sge_state {
	struct hfi2_sge *sg_list;  /* next SGE to be used if any */
	struct hfi2_sge sge;	   /* progress state for the current SGE */
	u32 total_len;
	u8 num_sge;
};

static inline void opa_ib_put_ss(struct opa_ib_sge_state *ss)
{
	while (ss->num_sge) {
		hfi2_put_mr(ss->sge.mr);
		if (--ss->num_sge)
			ss->sge = *ss->sg_list++;
	}
}

/*
 * This structure holds the information that the send tasklet needs
 * to send a RDMA read response or atomic operation.
 */
struct opa_ib_ack_entry {
	u8 opcode;
	u8 sent;
	u32 psn;
	u32 lpsn;
	union {
		struct hfi2_sge rdma_sge;
		u64 atomic_data;
	};
};

/*
 * Variables prefixed with s_ are for the requester (sender).
 * Variables prefixed with r_ are for the responder (receiver).
 * Variables prefixed with ack_ are for responder replies.
 *
 * Common variables are protected by both r_rq.lock and s_lock in that order
 * which only happens in modify_qp() or changing the QP 'state'.
 */
struct opa_ib_qp {
	struct ib_qp ibqp;
	/* read mostly fields above and below */
	struct ib_ah_attr remote_ah_attr;
	struct ib_ah_attr alt_ah_attr;
	struct opa_ib_swqe *s_wq;  /* send work queue */
	struct opa_ib_mmap_info *ip;
	struct opa_ib_dma_header *s_hdr; /* next packet header to send */
	unsigned long timeout_jiffies;  /* computed from timeout */

	enum ib_mtu path_mtu;
	int srate_mbps;		/* s_srate (below) converted to Mbit/s */
	u32 remote_qpn;
	u16 pmtu;		/* decoded from path_mtu */
	u32 qkey;               /* QKEY for this QP (for UD or RD) */
	u32 s_size;             /* send work queue size */
	u32 s_rnr_timeout;      /* number of milliseconds for RNR timeout */

	u8 state;               /* QP state */
	u8 qp_access_flags;
	u8 alt_timeout;         /* Alternate path timeout for this QP */
	u8 timeout;             /* Timeout for this QP */
	u8 s_srate;
	u8 s_mig_state;
	u8 port_num;
	u8 s_pkey_index;        /* PKEY index to use */
	u8 s_alt_pkey_index;    /* Alternate path PKEY index to use */
	u8 r_max_rd_atomic;     /* max number of RDMA read/atomic to receive */
	u8 s_max_rd_atomic;     /* max number of RDMA read/atomic to send */
	u8 s_retry_cnt;         /* number of times to retry */
	u8 s_rnr_retry_cnt;
	u8 r_min_rnr_timer;     /* retry timeout value for RNR NAKs */
	u8 s_max_sge;           /* size of s_wq->sg_list */
	u8 s_draining;
	u8 s_sc;		/* SC[0..4] for next packet */

	/* start of read/write fields */

	atomic_t refcount ____cacheline_aligned_in_smp;
	wait_queue_head_t wait;

	struct opa_ib_ack_entry s_ack_queue[OPA_IB_MAX_RDMA_ATOMIC + 1]
		____cacheline_aligned_in_smp;
	struct opa_ib_sge_state s_rdma_read_sge;

	spinlock_t r_lock ____cacheline_aligned_in_smp;      /* used for APM */
	unsigned long r_aflags;
	u64 r_wr_id;            /* ID for current receive WQE */
	u32 r_ack_psn;          /* PSN for next ACK or atomic ACK */
	u32 r_len;              /* total length of r_sge */
	u32 r_rcv_len;          /* receive data len processed */
	u32 r_psn;              /* expected rcv packet sequence number */
	u32 r_msn;              /* message sequence number */
	u8 r_state;             /* opcode of last packet received */
	u8 r_flags;
	u8 r_head_ack_queue;    /* index into s_ack_queue[] */
	struct list_head rspwait;	/* link for waiting to respond */
	struct opa_ib_sge_state r_sge;	/* current receive data */
	struct opa_ib_rq r_rq;		/* receive work queue */

	spinlock_t s_lock ____cacheline_aligned_in_smp;
	struct opa_ib_sge_state *s_cur_sge;
	u32 s_flags;
	struct opa_ib_swqe *s_wqe;
	struct opa_ib_sge_state s_sge;     /* current send request data */
	struct hfi2_mregion *s_rdma_mr;
	u32 s_cur_size;         /* size of send packet in bytes */
	u32 s_len;              /* total length of s_sge */
	u32 s_rdma_read_len;    /* total length of s_rdma_read_sge */
	u32 s_next_psn;         /* PSN for next request */
	u32 s_last_psn;         /* last response PSN processed */
	u32 s_sending_psn;      /* lowest PSN that is being sent */
	u32 s_sending_hpsn;     /* highest PSN that is being sent */
	u32 s_psn;              /* current packet sequence number */
	u32 s_ack_rdma_psn;     /* PSN for sending RDMA read responses */
	u32 s_ack_psn;          /* PSN for acking sends and RDMA writes */
	u32 s_head;             /* new entries added here */
	u32 s_tail;             /* next entry to process */
	u32 s_cur;              /* current work queue entry */
	u32 s_acked;            /* last un-ACK'ed entry */
	u32 s_last;             /* last completed entry */
	u32 s_ssn;              /* SSN of tail entry */
	u32 s_lsn;              /* limit sequence number (credit) */
	u16 s_hdrwords;         /* size of s_hdr in 32 bit words */
	u16 s_rdma_ack_cnt;
	s8 s_ahgidx;
	u8 s_state;             /* opcode of last packet sent */
	u8 s_ack_state;         /* opcode of packet to ACK */
	u8 s_nak_state;         /* non-zero if NAK is pending */
	u8 r_nak_state;         /* non-zero if NAK is pending */
	u8 s_retry;             /* requester retry counter */
	u8 s_rnr_retry;         /* requester RNR retry counter */
	u8 s_num_rd_atomic;     /* number of RDMA read/atomic pending */
	u8 s_tail_ack_queue;    /* index into s_ack_queue[] */
	u8 allowed_ops;		/* high order bits of allowed opcodes */

	struct opa_ib_sge_state s_ack_rdma_sge;
	struct timer_list s_timer;
	struct iowait s_iowait;

	struct hfi2_sge r_sg_list[0] /* verified SGEs */
		____cacheline_aligned_in_smp;

	struct hfi_ctx *s_ctx;	/* QP's send context */
};

/*
 * Atomic bit definitions for r_aflags.
 */
#define HFI1_R_WRID_VALID        0
#define HFI1_R_REWIND_SGE        1

/*
 * Bit definitions for r_flags.
 */
#define HFI1_R_REUSE_SGE 0x01
#define HFI1_R_RDMAR_SEQ 0x02
#define HFI1_R_RSP_NAK   0x04
#define HFI1_R_RSP_SEND  0x08
#define HFI1_R_COMM_EST  0x10

/*
 * Bit definitions for s_flags.
 *
 * HFI1_S_SIGNAL_REQ_WR - set if QP send WRs contain completion signaled
 * HFI1_S_BUSY - send tasklet is processing the QP
 * HFI1_S_TIMER - the RC retry timer is active
 * HFI1_S_ACK_PENDING - an ACK is waiting to be sent after RDMA read/atomics
 * HFI1_S_WAIT_FENCE - waiting for all prior RDMA read or atomic SWQEs
 *                         before processing the next SWQE
 * HFI1_S_WAIT_RDMAR - waiting for a RDMA read or atomic SWQE to complete
 *                         before processing the next SWQE
 * HFI1_S_WAIT_RNR - waiting for RNR timeout
 * HFI1_S_WAIT_SSN_CREDIT - waiting for RC credits to process next SWQE
 * HFI1_S_WAIT_DMA - waiting for send DMA queue to drain before generating
 *                  next send completion entry not via send DMA
 * HFI1_S_WAIT_PIO - waiting for a send buffer to be available
 * HFI1_S_WAIT_TX - waiting for a struct verbs_txreq to be available
 * HFI1_S_WAIT_DMA_DESC - waiting for DMA descriptors to be available
 * HFI1_S_WAIT_KMEM - waiting for kernel memory to be available
 * HFI1_S_WAIT_PSN - waiting for a packet to exit the send DMA queue
 * HFI1_S_WAIT_ACK - waiting for an ACK packet before sending more requests
 * HFI1_S_SEND_ONE - send one packet, request ACK, then wait for ACK
 * HFI1_S_ECN - a  BECN was queued to the send engine
 */
#define HFI1_S_SIGNAL_REQ_WR	0x0001
#define HFI1_S_BUSY		0x0002
#define HFI1_S_TIMER		0x0004
#define HFI1_S_RESP_PENDING	0x0008
#define HFI1_S_ACK_PENDING	0x0010
#define HFI1_S_WAIT_FENCE	0x0020
#define HFI1_S_WAIT_RDMAR	0x0040
#define HFI1_S_WAIT_RNR		0x0080
#define HFI1_S_WAIT_SSN_CREDIT	0x0100
#define HFI1_S_WAIT_DMA		0x0200
#define HFI1_S_WAIT_PIO		0x0400
#define HFI1_S_WAIT_TX		0x0800
#define HFI1_S_WAIT_DMA_DESC	0x1000
#define HFI1_S_WAIT_KMEM	0x2000
#define HFI1_S_WAIT_PSN		0x4000
#define HFI1_S_WAIT_ACK		0x8000
#define HFI1_S_SEND_ONE		0x10000
#define HFI1_S_UNLIMITED_CREDIT	0x20000
#define HFI1_S_ECN		0x40000

/*
 * Wait flags that would prevent any packet type from being sent.
 */
#define HFI1_S_ANY_WAIT_IO (HFI1_S_WAIT_PIO | HFI1_S_WAIT_TX | \
	HFI1_S_WAIT_DMA_DESC | HFI1_S_WAIT_KMEM)

/*
 * Wait flags that would prevent send work requests from making progress.
 */
#define HFI1_S_ANY_WAIT_SEND (HFI1_S_WAIT_FENCE | HFI1_S_WAIT_RDMAR | \
	HFI1_S_WAIT_RNR | HFI1_S_WAIT_SSN_CREDIT | HFI1_S_WAIT_DMA | \
	HFI1_S_WAIT_PSN | HFI1_S_WAIT_ACK)

#define HFI1_S_ANY_WAIT (HFI1_S_ANY_WAIT_IO | HFI1_S_ANY_WAIT_SEND)

/* Number of bits to pay attention to in the opcode for checking qp type */
#define OPCODE_QP_MASK 0xE0

/*
 * Since struct opa_ib_swqe is not a fixed size, we can't simply index into
 * struct opa_ib_qp.s_wq.  This function does the array index computation.
 */
static inline struct opa_ib_swqe *get_swqe_ptr(struct opa_ib_qp *qp,
					      unsigned n)
{
	return (struct opa_ib_swqe *)((char *)qp->s_wq +
				     (sizeof(struct opa_ib_swqe) +
				      qp->s_max_sge *
				      sizeof(struct hfi2_sge)) * n);
}

/*
 * Since struct opa_ib_rwqe is not a fixed size, we can't simply index into
 * struct opa_ib_rwq.wq.  This function does the array index computation.
 */
static inline struct opa_ib_rwqe *get_rwqe_ptr(struct opa_ib_rq *rq, unsigned n)
{
	return (struct opa_ib_rwqe *)
		((char *) rq->wq->wq +
		 (sizeof(struct opa_ib_rwqe) +
		  rq->max_sge * sizeof(struct ib_sge)) * n);
}

/*
 * There is one struct opa_mcast for each multicast GID.
 * All attached QPs are then stored as a list of
 * struct opa_mcast_qp.
 */
struct opa_mcast_qp {
	struct list_head list;
	struct opa_ib_qp *qp;
};

struct opa_mcast {
	struct rb_node rb_node;
	union ib_gid mgid;
	struct list_head qp_list;
	wait_queue_head_t wait;
	atomic_t refcount;
	int n_attached;
};

struct opa_ib_portdata {
	struct opa_core_device *odev;
	struct opa_ib_data *ibd;
	struct device *dev; /* from IB's ib_device */
	struct opa_ib_qp __rcu *qp[2];
	/* non-zero when timer is set */
	unsigned long mkey_lease_timeout;
	__be64 gid_prefix;
	__be64 mkey;
	__be64 guid;
	u64 n_loop_pkts;
	u64 n_pkt_drops;
	u64 n_vl15_dropped;

	u32 port_cap_flags;
	u32 sm_trap_qp;
	u32 sa_qp;
	u16 ibmtu;
	u16 ibmaxmtu;
	u16 link_speed_supported;
	u16 link_speed_enabled;
	u16 link_speed_active;
	u16 link_width_supported;
	u16 link_width_enabled;
	u16 link_width_active;
	u16 mkey_lease_period;
	u16 pkey_tlen;
	/* list of pkeys programmed; 0 if not set */
	u16 *pkeys;
	u16 pkey_violations;
	u16 qkey_violations;
	u16 mkey_violations;
	u16 lid;
	u16 sm_lid;
	u8 lmc;
	u8 max_vls;
	u8 port_num;
	u8 smsl;
	u8 mkeyprot;
	u8 subnet_timeout;
	/* the first 16 entries are sl_to_vl for !STL */
	u8 sl_to_sc[OPA_MAX_SLS];
	u8 sc_to_sl[OPA_MAX_SCS];
	u8 sc_to_vl[OPA_MAX_SCS];
	u16 vl_mtu[OPA_MAX_VLS];

	/* link state */
	u8 lstate;
	/* Physical port state */
	u8 pstate;

	struct hfi_ctx *ctx;
	struct hfi_cq cmdq_tx;
	struct hfi_cq cmdq_rx;
	spinlock_t cmdq_tx_lock;
	spinlock_t cmdq_rx_lock;
	/* protect changes in this struct */
	spinlock_t lock;
	struct rb_root mcast_tree;
	struct task_struct *rcv_task;
	uint16_t rcv_eq;
	uint16_t rcv_egr_last_idx;
	uint16_t send_eq;
	void *rcv_eq_base;
	void *rcv_egr_base;
	void *send_eq_base;
};

struct opa_ib_data {
	struct ib_device ibdev;
	struct opa_core_device *odev;
	struct device *parent_dev;
	__be64 node_guid;
	u8 num_pports;
	u8 oui[3];
	int assigned_node_id;
	struct opa_ib_portdata *pport;

	struct ida qpn_even_table;
	struct ida qpn_odd_table;
	struct idr qp_ptr;
	unsigned long reserved_qps;
	spinlock_t qpt_lock;
	struct opa_ib_lkey_table lk_table;
	struct hfi2_mregion __rcu *dma_mr;
	struct list_head pending_mmaps;
	spinlock_t pending_lock;
	u32 mmap_offset;
	spinlock_t mmap_offset_lock;
	u32 n_pds_allocated;
	spinlock_t n_pds_lock;
	u32 n_ahs_allocated;
	spinlock_t n_ahs_lock;
	u32 n_cqs_allocated;
	spinlock_t n_cqs_lock;
	u32 n_qps_allocated;
	spinlock_t n_qps_lock;
	u32 n_srqs_allocated;
	spinlock_t n_srqs_lock;
	u32 n_mcast_grps_allocated; /* number of mcast groups allocated */
	spinlock_t n_mcast_grps_lock;

	/* per device cq worker */
	struct kthread_worker *worker;

	struct hfi_ctx ctx;
};

#define to_opa_ibpd(pd)	container_of((pd), struct opa_ib_pd, ibpd)
#define to_opa_ibah(ah)	container_of((ah), struct opa_ib_ah, ibah)
#define to_opa_ibqp(qp)	container_of((qp), struct opa_ib_qp, ibqp)
#define to_opa_ibcq(cq)	container_of((cq), struct opa_ib_cq, ibcq)
#define to_opa_ibmr(mr)	container_of((mr), struct opa_ib_mr, ibmr)
#define to_opa_ibsrq(srq)	container_of((srq), struct opa_ib_srq, ibsrq)
#define to_opa_ibdata(ibd)	container_of((ibd), struct opa_ib_data, ibdev)
#define to_opa_ucontext(ibu)	container_of((ibu),\
				struct opa_ucontext, ibucontext)

static inline struct opa_ib_portdata *to_opa_ibportdata(struct ib_device *ibdev,
							u8 port)
{
	struct opa_ib_data *ibd = to_opa_ibdata(ibdev);
	unsigned pidx = port - 1; /* IB number port from 1, hdw from 0 */

	if (pidx >= ibd->num_pports) {
		WARN_ON(pidx >= ibd->num_pports);
		return NULL;
	}
	return &(ibd->pport[pidx]);
}

/*
 * Return the QP with the given QPN
 * The caller must hold the rcu_read_lock(), and keep the lock until
 * the returned qp is no longer in use.
 * TODO - revisit if IDR is really best for a fast lookup_qpn.
 */
static inline struct opa_ib_qp *opa_ib_lookup_qpn(struct opa_ib_portdata *ibp,
						  u32 qpn) __must_hold(RCU)
{
	if (unlikely(qpn <= 1))
		return rcu_dereference(ibp->qp[qpn]);
	else
		return idr_find(&ibp->ibd->qp_ptr, qpn);
}

/*
 * Return the indexed PKEY from the port PKEY table.
 */
static inline u16 opa_ib_get_npkeys(struct opa_ib_data *ibd)
{
	struct opa_ib_portdata *ibp1 = to_opa_ibportdata(&ibd->ibdev, 1);

	return ibp1->pkey_tlen;
}

#define opa_ib_get_pkey(ibp, index) \
	((index) >= ((ibp)->pkey_tlen) ? 0 : (ibp)->pkeys[(index)])

static inline u8 valid_ib_mtu(u16 mtu)
{
	return mtu == 256 || mtu == 512 ||
		mtu == 1024 || mtu == 2048 ||
		mtu == 4096;
}
struct ib_pd *opa_ib_alloc_pd(struct ib_device *ibdev,
			      struct ib_ucontext *context,
			      struct ib_udata *udata);
int opa_ib_dealloc_pd(struct ib_pd *ibpd);
int opa_ib_check_ah(struct ib_device *ibdev, struct ib_ah_attr *ah_attr);
struct ib_ah *opa_ib_create_ah(struct ib_pd *pd,
			       struct ib_ah_attr *ah_attr);
int opa_ib_destroy_ah(struct ib_ah *ibah);
int opa_ib_modify_ah(struct ib_ah *ibah, struct ib_ah_attr *ah_attr);
int opa_ib_query_ah(struct ib_ah *ibah, struct ib_ah_attr *ah_attr);
struct ib_qp *opa_ib_create_qp(struct ib_pd *ibpd,
			       struct ib_qp_init_attr *init_attr,
			       struct ib_udata *udata);
struct opa_ib_qp *opa_ib_lookup_qpn(struct opa_ib_portdata *ibp, u32 qpn);
int opa_ib_modify_qp(struct ib_qp *ibqp, struct ib_qp_attr *attr,
		  int attr_mask, struct ib_udata *udata);
int opa_ib_query_qp(struct ib_qp *ibqp, struct ib_qp_attr *attr,
		    int attr_mask, struct ib_qp_init_attr *init_attr);
int opa_ib_destroy_qp(struct ib_qp *ibqp);
int opa_ib_cq_init(struct opa_ib_data *ibd);
void opa_ib_cq_exit(struct opa_ib_data *ibd);
void opa_ib_cq_enter(struct opa_ib_cq *cq, struct ib_wc *entry, int solicited);
int opa_ib_poll_cq(struct ib_cq *ibcq, int num_entries, struct ib_wc *entry);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 3, 0)
struct ib_cq *opa_ib_create_cq(struct ib_device *ibdev,
			       const struct ib_cq_init_attr *attr,
			       struct ib_ucontext *context,
			       struct ib_udata *udata);
#else
struct ib_cq *opa_ib_create_cq(struct ib_device *ibdev, int entries,
			       int comp_vector, struct ib_ucontext *context,
			       struct ib_udata *udata);
#endif
int opa_ib_destroy_cq(struct ib_cq *ibcq);
int opa_ib_req_notify_cq(struct ib_cq *ibcq, enum ib_cq_notify_flags notify_flags);
int opa_ib_resize_cq(struct ib_cq *ibcq, int cqe, struct ib_udata *udata);
void opa_ib_rc_error(struct opa_ib_qp *qp, enum ib_wc_status err);
void opa_ib_rc_rcv(struct opa_ib_qp *qp, struct opa_ib_packet *packet);
void opa_ib_uc_rcv(struct opa_ib_qp *qp, struct opa_ib_packet *packet);
void opa_ib_ud_rcv(struct opa_ib_qp *qp, struct opa_ib_packet *packet);
int opa_ib_rcv_wait(void *data);
int opa_ib_lookup_pkey_idx(struct opa_ib_portdata *ibp, u16 pkey);
int opa_ib_lkey_ok(struct opa_ib_lkey_table *rkt, struct opa_ib_pd *pd,
		   struct hfi2_sge *isge, struct ib_sge *sge, int acc);
int opa_ib_rkey_ok(struct opa_ib_qp *qp, struct hfi2_sge *sge,
		   u32 len, u64 vaddr, u32 rkey, int acc);
struct ib_mr *opa_ib_get_dma_mr(struct ib_pd *pd, int acc);
struct ib_mr *opa_ib_reg_phys_mr(struct ib_pd *pd,
				 struct ib_phys_buf *buffer_list,
				 int num_phys_buf, int acc, u64 *iova_start);
struct ib_mr *opa_ib_reg_user_mr(struct ib_pd *pd, u64 start, u64 length,
				 u64 virt_addr, int mr_access_flags,
				 struct ib_udata *udata);
int opa_ib_dereg_mr(struct ib_mr *ibmr);
int opa_ib_fast_reg_mr(struct opa_ib_qp *qp, struct ib_send_wr *wr);
void opa_ib_release_mmap_info(struct kref *ref);
int opa_ib_mmap(struct ib_ucontext *context, struct vm_area_struct *vma);
struct opa_ib_mmap_info *opa_ib_create_mmap_info(struct opa_ib_data *ibd,
						 u32 size,
						 struct ib_ucontext *context,
						 void *obj);
void opa_ib_update_mmap_info(struct opa_ib_data *ibd,
			     struct opa_ib_mmap_info *ip,
			     u32 size, void *obj);
int opa_ib_get_rwqe(struct opa_ib_qp *qp, int wr_id_only);
void opa_ib_make_ruc_header(struct opa_ib_qp *qp, struct ib_l4_headers *ohdr,
			    u32 bth0, u32 bth2, u16 *lrh0);
void opa_ib_do_send(struct work_struct *work);
void opa_ib_schedule_send(struct opa_ib_qp *qp);
void opa_ib_send_complete(struct opa_ib_qp *qp, struct opa_ib_swqe *wqe,
			  enum ib_wc_status status);
int opa_ib_make_uc_req(struct opa_ib_qp *qp);
int opa_ib_make_ud_req(struct opa_ib_qp *qp);
void opa_ib_copy_sge(struct opa_ib_sge_state *ss, void *data, u32 length,
		     int release);
void opa_ib_skip_sge(struct opa_ib_sge_state *ss, u32 length, int release);
int opa_ib_post_send(struct ib_qp *ibqp, struct ib_send_wr *wr,
		     struct ib_send_wr **bad_wr);
int opa_ib_post_receive(struct ib_qp *ibqp, struct ib_recv_wr *wr,
			struct ib_recv_wr **bad_wr);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 3, 0)
int opa_ib_process_mad(struct ib_device *ibdev, int mad_flags, u8 port,
		       const struct ib_wc *in_wc, const struct ib_grh *in_grh,
		       const struct ib_mad_hdr *in_mad, size_t in_mad_size,
		       struct ib_mad_hdr *out_mad, size_t *out_mad_size,
		       u16 *out_mad_pkey_index);
#else
int opa_ib_process_mad(struct ib_device *ibdev, int mad_flags, u8 port,
		       struct ib_wc *in_wc, struct ib_grh *in_grh,
		       struct ib_mad *in_mad, struct ib_mad *out_mad);
#endif
int opa_ib_multicast_attach(struct ib_qp *ibqp, union ib_gid *gid, u16 lid);
int opa_ib_multicast_detach(struct ib_qp *ibqp, union ib_gid *gid, u16 lid);

/* Device specific */
int opa_ib_send_wqe_pio(struct opa_ib_portdata *ibp, struct opa_ib_swqe *wqe);
int opa_ib_send_wqe(struct opa_ib_portdata *ibp, struct opa_ib_qp *qp,
		    struct opa_ib_swqe *wqe);
void opa_ib_rcv_start(struct opa_ib_portdata *ibp);
void *opa_ib_rcv_get_ebuf(struct opa_ib_portdata *ibp, u16 idx, u32 offset);
void opa_ib_rcv_advance(struct opa_ib_portdata *ibp, u64 *rhf_entry);
int _opa_ib_rcv_wait(struct opa_ib_portdata *ibp, u64 **rhf_entry);
int opa_ib_rcv_init(struct opa_ib_portdata *ibp);
void opa_ib_rcv_uninit(struct opa_ib_portdata *ibp);
int opa_ib_ctx_init(struct opa_ib_data *ibd);
void opa_ib_ctx_uninit(struct opa_ib_data *ibd);
int opa_ib_ctx_init_port(struct opa_ib_portdata *ibp);
void opa_ib_ctx_uninit_port(struct opa_ib_portdata *ibp);
int opa_ib_ctx_assign_qp(struct opa_ib_data *ibd, struct opa_ib_qp *qp, bool is_user);
void opa_ib_ctx_release_qp(struct opa_ib_data *ibd, struct opa_ib_qp *qp);
#endif
