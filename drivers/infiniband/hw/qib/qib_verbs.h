/*
 * Copyright (c) 2012, 2013 Intel Corporation.  All rights reserved.
 * Copyright (c) 2006 - 2012 QLogic Corporation. All rights reserved.
 * Copyright (c) 2005, 2006 PathScale, Inc. All rights reserved.
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

#ifndef QIB_VERBS_H
#define QIB_VERBS_H

#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/kref.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/completion.h>
#include <rdma/ib_pack.h>
#include <rdma/ib_user_verbs.h>
#include <rdma/rdma_vt.h>

struct qib_ctxtdata;
struct qib_pportdata;
struct qib_devdata;
struct qib_verbs_txreq;

#define QIB_MAX_RDMA_ATOMIC     16
#define QIB_GUIDS_PER_PORT	5

#define QPN_MAX                 (1 << 24)
#define QPNMAP_ENTRIES          (QPN_MAX / PAGE_SIZE / BITS_PER_BYTE)

/*
 * Increment this value if any changes that break userspace ABI
 * compatibility are made.
 */
#define QIB_UVERBS_ABI_VERSION       2

/*
 * Define an ib_cq_notify value that is not valid so we know when CQ
 * notifications are armed.
 */
#define IB_CQ_NONE      (IB_CQ_NEXT_COMP + 1)

#define IB_SEQ_NAK	(3 << 29)

/* AETH NAK opcode values */
#define IB_RNR_NAK                      0x20
#define IB_NAK_PSN_ERROR                0x60
#define IB_NAK_INVALID_REQUEST          0x61
#define IB_NAK_REMOTE_ACCESS_ERROR      0x62
#define IB_NAK_REMOTE_OPERATIONAL_ERROR 0x63
#define IB_NAK_INVALID_RD_REQUEST       0x64

/* Flags for checking QP state (see ib_qib_state_ops[]) */
#define QIB_POST_SEND_OK                0x01
#define QIB_POST_RECV_OK                0x02
#define QIB_PROCESS_RECV_OK             0x04
#define QIB_PROCESS_SEND_OK             0x08
#define QIB_PROCESS_NEXT_SEND_OK        0x10
#define QIB_FLUSH_SEND			0x20
#define QIB_FLUSH_RECV			0x40
#define QIB_PROCESS_OR_FLUSH_SEND \
	(QIB_PROCESS_SEND_OK | QIB_FLUSH_SEND)

/* IB Performance Manager status values */
#define IB_PMA_SAMPLE_STATUS_DONE       0x00
#define IB_PMA_SAMPLE_STATUS_STARTED    0x01
#define IB_PMA_SAMPLE_STATUS_RUNNING    0x02

/* Mandatory IB performance counter select values. */
#define IB_PMA_PORT_XMIT_DATA   cpu_to_be16(0x0001)
#define IB_PMA_PORT_RCV_DATA    cpu_to_be16(0x0002)
#define IB_PMA_PORT_XMIT_PKTS   cpu_to_be16(0x0003)
#define IB_PMA_PORT_RCV_PKTS    cpu_to_be16(0x0004)
#define IB_PMA_PORT_XMIT_WAIT   cpu_to_be16(0x0005)

#define QIB_VENDOR_IPG		cpu_to_be16(0xFFA0)

#define IB_BTH_REQ_ACK		(1 << 31)
#define IB_BTH_SOLICITED	(1 << 23)
#define IB_BTH_MIG_REQ		(1 << 22)

/* XXX Should be defined in ib_verbs.h enum ib_port_cap_flags */
#define IB_PORT_OTHER_LOCAL_CHANGES_SUP (1 << 26)

#define IB_GRH_VERSION		6
#define IB_GRH_VERSION_MASK	0xF
#define IB_GRH_VERSION_SHIFT	28
#define IB_GRH_TCLASS_MASK	0xFF
#define IB_GRH_TCLASS_SHIFT	20
#define IB_GRH_FLOW_MASK	0xFFFFF
#define IB_GRH_FLOW_SHIFT	0
#define IB_GRH_NEXT_HDR		0x1B

#define IB_DEFAULT_GID_PREFIX	cpu_to_be64(0xfe80000000000000ULL)

/* Values for set/get portinfo VLCap OperationalVLs */
#define IB_VL_VL0       1
#define IB_VL_VL0_1     2
#define IB_VL_VL0_3     3
#define IB_VL_VL0_7     4
#define IB_VL_VL0_14    5

static inline int qib_num_vls(int vls)
{
	switch (vls) {
	default:
	case IB_VL_VL0:
		return 1;
	case IB_VL_VL0_1:
		return 2;
	case IB_VL_VL0_3:
		return 4;
	case IB_VL_VL0_7:
		return 8;
	case IB_VL_VL0_14:
		return 15;
	}
}

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

struct qib_other_headers {
	__be32 bth[3];
	union {
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
	} u;
} __packed;

/*
 * Note that UD packets with a GRH header are 8+40+12+8 = 68 bytes
 * long (72 w/ imm_data).  Only the first 56 bytes of the IB header
 * will be in the eager header buffer.  The remaining 12 or 16 bytes
 * are in the data buffer.
 */
struct qib_ib_header {
	__be16 lrh[4];
	union {
		struct {
			struct ib_grh grh;
			struct qib_other_headers oth;
		} l;
		struct qib_other_headers oth;
	} u;
} __packed;

struct qib_pio_header {
	__le32 pbc[2];
	struct qib_ib_header hdr;
} __packed;

/*
 * There is one struct qib_mcast for each multicast GID.
 * All attached QPs are then stored as a list of
 * struct qib_mcast_qp.
 */
struct qib_mcast_qp {
	struct list_head list;
	struct rvt_qp *qp;
};

struct qib_mcast {
	struct rb_node rb_node;
	union ib_gid mgid;
	struct list_head qp_list;
	wait_queue_head_t wait;
	atomic_t refcount;
	int n_attached;
};

/*
 * This structure is used to contain the head pointer, tail pointer,
 * and completion queue entries as a single memory allocation so
 * it can be mmap'ed into user space.
 */
struct qib_cq_wc {
	u32 head;               /* index of next entry to fill */
	u32 tail;               /* index of next ib_poll_cq() entry */
	union {
		/* these are actually size ibcq.cqe + 1 */
		struct ib_uverbs_wc uqueue[0];
		struct ib_wc kqueue[0];
	};
};

/*
 * The completion queue structure.
 */
struct qib_cq {
	struct ib_cq ibcq;
	struct kthread_work comptask;
	struct qib_devdata *dd;
	spinlock_t lock; /* protect changes in this struct */
	u8 notify;
	u8 triggered;
	struct qib_cq_wc *queue;
	struct rvt_mmap_info *ip;
};

/*
 * qib specific data structure that will be hidden from rvt after the queue pair
 * is made common.
 */
struct qib_qp_priv {
	struct qib_ib_header *s_hdr;    /* next packet header to send */
	struct list_head iowait;        /* link for wait PIO buf */
	atomic_t s_dma_busy;
	struct qib_verbs_txreq *s_tx;
	struct work_struct s_work;
	wait_queue_head_t wait_dma;
	struct rvt_qp *owner;
};

/*
 * Atomic bit definitions for r_aflags.
 */
#define QIB_R_WRID_VALID        0
#define QIB_R_REWIND_SGE        1

/*
 * Bit definitions for r_flags.
 */
#define QIB_R_REUSE_SGE 0x01
#define QIB_R_RDMAR_SEQ 0x02
#define QIB_R_RSP_NAK   0x04
#define QIB_R_RSP_SEND  0x08
#define QIB_R_COMM_EST  0x10

/*
 * Bit definitions for s_flags.
 *
 * QIB_S_SIGNAL_REQ_WR - set if QP send WRs contain completion signaled
 * QIB_S_BUSY - send tasklet is processing the QP
 * QIB_S_TIMER - the RC retry timer is active
 * QIB_S_ACK_PENDING - an ACK is waiting to be sent after RDMA read/atomics
 * QIB_S_WAIT_FENCE - waiting for all prior RDMA read or atomic SWQEs
 *                         before processing the next SWQE
 * QIB_S_WAIT_RDMAR - waiting for a RDMA read or atomic SWQE to complete
 *                         before processing the next SWQE
 * QIB_S_WAIT_RNR - waiting for RNR timeout
 * QIB_S_WAIT_SSN_CREDIT - waiting for RC credits to process next SWQE
 * QIB_S_WAIT_DMA - waiting for send DMA queue to drain before generating
 *                  next send completion entry not via send DMA
 * QIB_S_WAIT_PIO - waiting for a send buffer to be available
 * QIB_S_WAIT_TX - waiting for a struct qib_verbs_txreq to be available
 * QIB_S_WAIT_DMA_DESC - waiting for DMA descriptors to be available
 * QIB_S_WAIT_KMEM - waiting for kernel memory to be available
 * QIB_S_WAIT_PSN - waiting for a packet to exit the send DMA queue
 * QIB_S_WAIT_ACK - waiting for an ACK packet before sending more requests
 * QIB_S_SEND_ONE - send one packet, request ACK, then wait for ACK
 */
#define QIB_S_SIGNAL_REQ_WR	0x0001
#define QIB_S_BUSY		0x0002
#define QIB_S_TIMER		0x0004
#define QIB_S_RESP_PENDING	0x0008
#define QIB_S_ACK_PENDING	0x0010
#define QIB_S_WAIT_FENCE	0x0020
#define QIB_S_WAIT_RDMAR	0x0040
#define QIB_S_WAIT_RNR		0x0080
#define QIB_S_WAIT_SSN_CREDIT	0x0100
#define QIB_S_WAIT_DMA		0x0200
#define QIB_S_WAIT_PIO		0x0400
#define QIB_S_WAIT_TX		0x0800
#define QIB_S_WAIT_DMA_DESC	0x1000
#define QIB_S_WAIT_KMEM		0x2000
#define QIB_S_WAIT_PSN		0x4000
#define QIB_S_WAIT_ACK		0x8000
#define QIB_S_SEND_ONE		0x10000
#define QIB_S_UNLIMITED_CREDIT	0x20000

/*
 * Wait flags that would prevent any packet type from being sent.
 */
#define QIB_S_ANY_WAIT_IO (QIB_S_WAIT_PIO | QIB_S_WAIT_TX | \
	QIB_S_WAIT_DMA_DESC | QIB_S_WAIT_KMEM)

/*
 * Wait flags that would prevent send work requests from making progress.
 */
#define QIB_S_ANY_WAIT_SEND (QIB_S_WAIT_FENCE | QIB_S_WAIT_RDMAR | \
	QIB_S_WAIT_RNR | QIB_S_WAIT_SSN_CREDIT | QIB_S_WAIT_DMA | \
	QIB_S_WAIT_PSN | QIB_S_WAIT_ACK)

#define QIB_S_ANY_WAIT (QIB_S_ANY_WAIT_IO | QIB_S_ANY_WAIT_SEND)

#define QIB_PSN_CREDIT  16

/*
 * Since struct rvt_swqe is not a fixed size, we can't simply index into
 * struct rvt_qp.s_wq.  This function does the array index computation.
 */
static inline struct rvt_swqe *get_swqe_ptr(struct rvt_qp *qp,
					    unsigned n)
{
	return (struct rvt_swqe *)((char *)qp->s_wq +
				     (sizeof(struct rvt_swqe) +
				      qp->s_max_sge *
				      sizeof(struct rvt_sge)) * n);
}

/*
 * Since struct rvt_rwqe is not a fixed size, we can't simply index into
 * struct rvt_rwq.wq.  This function does the array index computation.
 */
static inline struct rvt_rwqe *get_rwqe_ptr(struct rvt_rq *rq, unsigned n)
{
	return (struct rvt_rwqe *)
		((char *) rq->wq->wq +
		 (sizeof(struct rvt_rwqe) +
		  rq->max_sge * sizeof(struct ib_sge)) * n);
}

/*
 * QPN-map pages start out as NULL, they get allocated upon
 * first use and are never deallocated. This way,
 * large bitmaps are not allocated unless large numbers of QPs are used.
 */
struct qpn_map {
	void *page;
};

struct qib_qpn_table {
	spinlock_t lock; /* protect changes in this struct */
	unsigned flags;         /* flags for QP0/1 allocated for each port */
	u32 last;               /* last QP number allocated */
	u32 nmaps;              /* size of the map table */
	u16 limit;
	u16 mask;
	/* bit map of free QP numbers other than 0/1 */
	struct qpn_map map[QPNMAP_ENTRIES];
};

struct qib_opcode_stats {
	u64 n_packets;          /* number of packets */
	u64 n_bytes;            /* total number of bytes */
};

struct qib_opcode_stats_perctx {
	struct qib_opcode_stats stats[128];
};

struct qib_pma_counters {
	u64 n_unicast_xmit;     /* total unicast packets sent */
	u64 n_unicast_rcv;      /* total unicast packets received */
	u64 n_multicast_xmit;   /* total multicast packets sent */
	u64 n_multicast_rcv;    /* total multicast packets received */
};

struct qib_ibport {
	struct rvt_ibport rvp;
	struct rvt_ah *sm_ah;
	struct rvt_ah *smi_ah;
	__be64 guids[QIB_GUIDS_PER_PORT	- 1];	/* writable GUIDs */
	struct qib_pma_counters __percpu *pmastats;
	u64 z_unicast_xmit;     /* starting count for PMA */
	u64 z_unicast_rcv;      /* starting count for PMA */
	u64 z_multicast_xmit;   /* starting count for PMA */
	u64 z_multicast_rcv;    /* starting count for PMA */
	u64 z_symbol_error_counter;             /* starting count for PMA */
	u64 z_link_error_recovery_counter;      /* starting count for PMA */
	u64 z_link_downed_counter;              /* starting count for PMA */
	u64 z_port_rcv_errors;                  /* starting count for PMA */
	u64 z_port_rcv_remphys_errors;          /* starting count for PMA */
	u64 z_port_xmit_discards;               /* starting count for PMA */
	u64 z_port_xmit_data;                   /* starting count for PMA */
	u64 z_port_rcv_data;                    /* starting count for PMA */
	u64 z_port_xmit_packets;                /* starting count for PMA */
	u64 z_port_rcv_packets;                 /* starting count for PMA */
	u32 z_local_link_integrity_errors;      /* starting count for PMA */
	u32 z_excessive_buffer_overrun_errors;  /* starting count for PMA */
	u32 z_vl15_dropped;                     /* starting count for PMA */
	u8 sl_to_vl[16];
};

struct qib_ibdev {
	struct rvt_dev_info rdi;
	struct list_head pending_mmaps;
	spinlock_t mmap_offset_lock; /* protect mmap_offset */
	u32 mmap_offset;

	/* QP numbers are shared by all IB ports */
	struct qib_qpn_table qpn_table;
	struct list_head piowait;       /* list for wait PIO buf */
	struct list_head dmawait;	/* list for wait DMA */
	struct list_head txwait;        /* list for wait qib_verbs_txreq */
	struct list_head memwait;       /* list for wait kernel memory */
	struct list_head txreq_free;
	struct timer_list mem_timer;
	struct rvt_qp __rcu **qp_table;
	struct qib_pio_header *pio_hdrs;
	dma_addr_t pio_hdrs_phys;
	/* list of QPs waiting for RNR timer */
	spinlock_t pending_lock; /* protect wait lists, PMA counters, etc. */
	u32 qp_table_size; /* size of the hash table */
	u32 qp_rnd; /* random bytes for hash */
	spinlock_t qpt_lock;

	u32 n_piowait;
	u32 n_txwait;

	u32 n_cqs_allocated;    /* number of CQs allocated for device */
	spinlock_t n_cqs_lock;
	u32 n_qps_allocated;    /* number of QPs allocated for device */
	spinlock_t n_qps_lock;
	u32 n_srqs_allocated;   /* number of SRQs allocated for device */
	spinlock_t n_srqs_lock;
	u32 n_mcast_grps_allocated; /* number of mcast groups allocated */
	spinlock_t n_mcast_grps_lock;
#ifdef CONFIG_DEBUG_FS
	/* per HCA debugfs */
	struct dentry *qib_ibdev_dbg;
#endif
};

struct qib_verbs_counters {
	u64 symbol_error_counter;
	u64 link_error_recovery_counter;
	u64 link_downed_counter;
	u64 port_rcv_errors;
	u64 port_rcv_remphys_errors;
	u64 port_xmit_discards;
	u64 port_xmit_data;
	u64 port_rcv_data;
	u64 port_xmit_packets;
	u64 port_rcv_packets;
	u32 local_link_integrity_errors;
	u32 excessive_buffer_overrun_errors;
	u32 vl15_dropped;
};

static inline struct qib_cq *to_icq(struct ib_cq *ibcq)
{
	return container_of(ibcq, struct qib_cq, ibcq);
}

static inline struct rvt_qp *to_iqp(struct ib_qp *ibqp)
{
	return container_of(ibqp, struct rvt_qp, ibqp);
}

static inline struct qib_ibdev *to_idev(struct ib_device *ibdev)
{
	struct rvt_dev_info *rdi;

	rdi = container_of(ibdev, struct rvt_dev_info, ibdev);
	return container_of(rdi, struct qib_ibdev, rdi);
}

/*
 * Send if not busy or waiting for I/O and either
 * a RC response is pending or we can process send work requests.
 */
static inline int qib_send_ok(struct rvt_qp *qp)
{
	return !(qp->s_flags & (QIB_S_BUSY | QIB_S_ANY_WAIT_IO)) &&
		(qp->s_hdrwords || (qp->s_flags & QIB_S_RESP_PENDING) ||
		 !(qp->s_flags & QIB_S_ANY_WAIT_SEND));
}

/*
 * This must be called with s_lock held.
 */
void qib_schedule_send(struct rvt_qp *qp);

static inline int qib_pkey_ok(u16 pkey1, u16 pkey2)
{
	u16 p1 = pkey1 & 0x7FFF;
	u16 p2 = pkey2 & 0x7FFF;

	/*
	 * Low 15 bits must be non-zero and match, and
	 * one of the two must be a full member.
	 */
	return p1 && p1 == p2 && ((__s16)pkey1 < 0 || (__s16)pkey2 < 0);
}

void qib_bad_pqkey(struct qib_ibport *ibp, __be16 trap_num, u32 key, u32 sl,
		   u32 qp1, u32 qp2, __be16 lid1, __be16 lid2);
void qib_cap_mask_chg(struct qib_ibport *ibp);
void qib_sys_guid_chg(struct qib_ibport *ibp);
void qib_node_desc_chg(struct qib_ibport *ibp);
int qib_process_mad(struct ib_device *ibdev, int mad_flags, u8 port_num,
		    const struct ib_wc *in_wc, const struct ib_grh *in_grh,
		    const struct ib_mad_hdr *in, size_t in_mad_size,
		    struct ib_mad_hdr *out, size_t *out_mad_size,
		    u16 *out_mad_pkey_index);
int qib_create_agents(struct qib_ibdev *dev);
void qib_free_agents(struct qib_ibdev *dev);

/*
 * Compare the lower 24 bits of the two values.
 * Returns an integer <, ==, or > than zero.
 */
static inline int qib_cmp24(u32 a, u32 b)
{
	return (((int) a) - ((int) b)) << 8;
}

struct qib_mcast *qib_mcast_find(struct qib_ibport *ibp, union ib_gid *mgid);

int qib_snapshot_counters(struct qib_pportdata *ppd, u64 *swords,
			  u64 *rwords, u64 *spkts, u64 *rpkts,
			  u64 *xmit_wait);

int qib_get_counters(struct qib_pportdata *ppd,
		     struct qib_verbs_counters *cntrs);

int qib_multicast_attach(struct ib_qp *ibqp, union ib_gid *gid, u16 lid);

int qib_multicast_detach(struct ib_qp *ibqp, union ib_gid *gid, u16 lid);

int qib_mcast_tree_empty(struct qib_ibport *ibp);

__be32 qib_compute_aeth(struct rvt_qp *qp);

struct rvt_qp *qib_lookup_qpn(struct qib_ibport *ibp, u32 qpn);

struct ib_qp *qib_create_qp(struct ib_pd *ibpd,
			    struct ib_qp_init_attr *init_attr,
			    struct ib_udata *udata);

int qib_destroy_qp(struct ib_qp *ibqp);

int qib_error_qp(struct rvt_qp *qp, enum ib_wc_status err);

int qib_modify_qp(struct ib_qp *ibqp, struct ib_qp_attr *attr,
		  int attr_mask, struct ib_udata *udata);

int qib_query_qp(struct ib_qp *ibqp, struct ib_qp_attr *attr,
		 int attr_mask, struct ib_qp_init_attr *init_attr);

unsigned qib_free_all_qps(struct qib_devdata *dd);

void qib_init_qpn_table(struct qib_devdata *dd, struct qib_qpn_table *qpt);

void qib_free_qpn_table(struct qib_qpn_table *qpt);

#ifdef CONFIG_DEBUG_FS

struct qib_qp_iter;

struct qib_qp_iter *qib_qp_iter_init(struct qib_ibdev *dev);

int qib_qp_iter_next(struct qib_qp_iter *iter);

void qib_qp_iter_print(struct seq_file *s, struct qib_qp_iter *iter);

#endif

void qib_get_credit(struct rvt_qp *qp, u32 aeth);

unsigned qib_pkt_delay(u32 plen, u8 snd_mult, u8 rcv_mult);

void qib_verbs_sdma_desc_avail(struct qib_pportdata *ppd, unsigned avail);

void qib_put_txreq(struct qib_verbs_txreq *tx);

int qib_verbs_send(struct rvt_qp *qp, struct qib_ib_header *hdr,
		   u32 hdrwords, struct rvt_sge_state *ss, u32 len);

void qib_copy_sge(struct rvt_sge_state *ss, void *data, u32 length,
		  int release);

void qib_skip_sge(struct rvt_sge_state *ss, u32 length, int release);

void qib_uc_rcv(struct qib_ibport *ibp, struct qib_ib_header *hdr,
		int has_grh, void *data, u32 tlen, struct rvt_qp *qp);

void qib_rc_rcv(struct qib_ctxtdata *rcd, struct qib_ib_header *hdr,
		int has_grh, void *data, u32 tlen, struct rvt_qp *qp);

int qib_check_ah(struct ib_device *ibdev, struct ib_ah_attr *ah_attr);

struct ib_ah *qib_create_qp0_ah(struct qib_ibport *ibp, u16 dlid);

void qib_rc_rnr_retry(unsigned long arg);

void qib_rc_send_complete(struct rvt_qp *qp, struct qib_ib_header *hdr);

void qib_rc_error(struct rvt_qp *qp, enum ib_wc_status err);

int qib_post_ud_send(struct rvt_qp *qp, struct ib_send_wr *wr);

void qib_ud_rcv(struct qib_ibport *ibp, struct qib_ib_header *hdr,
		int has_grh, void *data, u32 tlen, struct rvt_qp *qp);

int qib_post_srq_receive(struct ib_srq *ibsrq, struct ib_recv_wr *wr,
			 struct ib_recv_wr **bad_wr);

struct ib_srq *qib_create_srq(struct ib_pd *ibpd,
			      struct ib_srq_init_attr *srq_init_attr,
			      struct ib_udata *udata);

int qib_modify_srq(struct ib_srq *ibsrq, struct ib_srq_attr *attr,
		   enum ib_srq_attr_mask attr_mask,
		   struct ib_udata *udata);

int qib_query_srq(struct ib_srq *ibsrq, struct ib_srq_attr *attr);

int qib_destroy_srq(struct ib_srq *ibsrq);

int qib_cq_init(struct qib_devdata *dd);

void qib_cq_exit(struct qib_devdata *dd);

void qib_cq_enter(struct qib_cq *cq, struct ib_wc *entry, int sig);

int qib_poll_cq(struct ib_cq *ibcq, int num_entries, struct ib_wc *entry);

struct ib_cq *qib_create_cq(struct ib_device *ibdev,
			    const struct ib_cq_init_attr *attr,
			    struct ib_ucontext *context,
			    struct ib_udata *udata);

int qib_destroy_cq(struct ib_cq *ibcq);

int qib_req_notify_cq(struct ib_cq *ibcq, enum ib_cq_notify_flags notify_flags);

int qib_resize_cq(struct ib_cq *ibcq, int cqe, struct ib_udata *udata);

void mr_rcu_callback(struct rcu_head *list);

static inline void qib_put_ss(struct rvt_sge_state *ss)
{
	while (ss->num_sge) {
		rvt_put_mr(ss->sge.mr);
		if (--ss->num_sge)
			ss->sge = *ss->sg_list++;
	}
}

void qib_release_mmap_info(struct kref *ref);

struct rvt_mmap_info *qib_create_mmap_info(struct qib_ibdev *dev, u32 size,
					   struct ib_ucontext *context,
					   void *obj);

void qib_update_mmap_info(struct qib_ibdev *dev, struct rvt_mmap_info *ip,
			  u32 size, void *obj);

int qib_mmap(struct ib_ucontext *context, struct vm_area_struct *vma);

int qib_get_rwqe(struct rvt_qp *qp, int wr_id_only);

void qib_migrate_qp(struct rvt_qp *qp);

int qib_ruc_check_hdr(struct qib_ibport *ibp, struct qib_ib_header *hdr,
		      int has_grh, struct rvt_qp *qp, u32 bth0);

u32 qib_make_grh(struct qib_ibport *ibp, struct ib_grh *hdr,
		 struct ib_global_route *grh, u32 hwords, u32 nwords);

void qib_make_ruc_header(struct rvt_qp *qp, struct qib_other_headers *ohdr,
			 u32 bth0, u32 bth2);

void qib_do_send(struct work_struct *work);

void qib_send_complete(struct rvt_qp *qp, struct rvt_swqe *wqe,
		       enum ib_wc_status status);

void qib_send_rc_ack(struct rvt_qp *qp);

int qib_make_rc_req(struct rvt_qp *qp);

int qib_make_uc_req(struct rvt_qp *qp);

int qib_make_ud_req(struct rvt_qp *qp);

int qib_register_ib_device(struct qib_devdata *);

void qib_unregister_ib_device(struct qib_devdata *);

void qib_ib_rcv(struct qib_ctxtdata *, void *, void *, u32);

void qib_ib_piobufavail(struct qib_devdata *);

unsigned qib_get_npkeys(struct qib_devdata *);

unsigned qib_get_pkey(struct qib_ibport *, unsigned);

extern const enum ib_wc_opcode ib_qib_wc_opcode[];

/*
 * Below  HCA-independent IB PhysPortState values, returned
 * by the f_ibphys_portstate() routine.
 */
#define IB_PHYSPORTSTATE_SLEEP 1
#define IB_PHYSPORTSTATE_POLL 2
#define IB_PHYSPORTSTATE_DISABLED 3
#define IB_PHYSPORTSTATE_CFG_TRAIN 4
#define IB_PHYSPORTSTATE_LINKUP 5
#define IB_PHYSPORTSTATE_LINK_ERR_RECOVER 6
#define IB_PHYSPORTSTATE_CFG_DEBOUNCE 8
#define IB_PHYSPORTSTATE_CFG_IDLE 0xB
#define IB_PHYSPORTSTATE_RECOVERY_RETRAIN 0xC
#define IB_PHYSPORTSTATE_RECOVERY_WAITRMT 0xE
#define IB_PHYSPORTSTATE_RECOVERY_IDLE 0xF
#define IB_PHYSPORTSTATE_CFG_ENH 0x10
#define IB_PHYSPORTSTATE_CFG_WAIT_ENH 0x13

extern const int ib_qib_state_ops[];

extern __be64 ib_qib_sys_image_guid;    /* in network order */

extern unsigned int ib_rvt_lkey_table_size;

extern unsigned int ib_qib_max_cqes;

extern unsigned int ib_qib_max_cqs;

extern unsigned int ib_qib_max_qp_wrs;

extern unsigned int ib_qib_max_qps;

extern unsigned int ib_qib_max_sges;

extern unsigned int ib_qib_max_mcast_grps;

extern unsigned int ib_qib_max_mcast_qp_attached;

extern unsigned int ib_qib_max_srqs;

extern unsigned int ib_qib_max_srq_sges;

extern unsigned int ib_qib_max_srq_wrs;

extern const u32 ib_qib_rnr_table[];

#endif                          /* QIB_VERBS_H */
