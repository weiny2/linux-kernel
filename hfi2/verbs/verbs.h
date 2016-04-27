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
#include <rdma/opa_core.h>
#include "rdmavt/vt.h"
#include "opa_port_info.h"
#include "ib_compat.h"
#include "iowait.h"
#include "../opa_hfi.h"

/*
 * FXRTODO: ipoib connected test fails with this
 * to be fixed
 */
#if 0
#define HFI_VERBS_TEST
#endif

/* TODO - these carried from WFR driver */
#define OPA_IB_MAX_RDMA_ATOMIC  16
#define OPA_IB_INIT_RDMA_ATOMIC 255
#define OPA_IB_MAX_MAP_PER_FMR  32767
#define OPA_IB_MAX_MSG_SZ       0x80000000
#define IB_DEFAULT_GID_PREFIX	cpu_to_be64(0xfe80000000000000ULL)
#define OPA_LIM_MGMT_PKEY       0x7FFF
#define OPA_FULL_MGMT_PKEY      0xFFFF

#define HFI2_GUIDS_PER_PORT     2
#define HFI2_RHF_RCV_TYPES      8

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

#define OPA_DEFAULT_SM_TRAP_QP			0x0
#define OPA_DEFAULT_SA_QP			0x1

/* TODO - placeholders */
extern __be64 hfi2_sys_guid;

extern const int ib_qp_state_ops[];
extern unsigned int hfi2_max_cqes;
extern unsigned int hfi2_max_cqs;
extern unsigned int hfi2_max_qp_wrs;
extern unsigned int hfi2_max_qps;
extern unsigned int hfi2_max_sges;
extern unsigned int hfi2_lkey_table_size;
extern unsigned int hfi2_max_mcast_grps;
extern unsigned int hfi2_max_mcast_qp_attached;
extern const enum ib_wc_opcode ib_hfi1_wc_opcode[];
extern const u32 ib_hfi1_rnr_table[];

struct ib_l4_headers;
struct hfi2_ib_header;
struct hfi2_ib_packet;
union hfi2_packet_header;

/*
 * hfi2 specific QP state which is hidden from rvt after queue pair created,
 * used for hfi2 driver specific interactions with hardware device.
 * @owner: the master rvt_qp structure for this QP
 * @s_hdr: pointer to packet header in memory for next send WQE
 * @s_ctx: hardware send context used for sending next packet
 * @s_sc: the SC value for next packet to send
 * @s_sl: the SL value for next packet to send
 * @use_16b: describes packet type for next packet
 * TODO: below is not implemented yet
 * @s_iowait: iowait structure if pending sends are stalled waiting for
 * hardwware resources (command queue slots)
 */
struct hfi2_qp_priv {
	struct rvt_qp *owner;
	union hfi2_ib_dma_header *s_hdr;
	struct hfi_ctx *s_ctx;
	u8 s_sc;
	u8 s_sl;
	bool use_16b;
	struct iowait s_iowait;
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
 * There is one struct hfi2_mcast for each multicast GID.
 * All attached QPs are then stored as a list of
 * struct hfi2_mcast_qp.
 */
struct hfi2_mcast_qp {
	struct list_head list;
	struct rvt_qp *qp;
};

struct hfi2_mcast {
	struct rb_node rb_node;
	union ib_gid mgid;
	struct list_head qp_list;
	wait_queue_head_t wait;
	atomic_t refcount;
	int n_attached;
};

/* TODO - hfi1 returns an int, review when we attempt common logic. */
typedef void (*rhf_rcv_function_ptr)(struct hfi2_ib_packet *packet);

struct hfi2_ibrcv {
	struct hfi2_ibport *ibp;
	struct hfi_ctx *ctx;
	struct hfi_eq eq;
	uint16_t egr_last_idx;
	void *egr_base;
	struct task_struct *task;
};

struct hfi2_ibport {
	struct rvt_ibport rvp;
	struct hfi2_ibdev *ibd;
	struct device *dev; /* from IB's ib_device */
	struct hfi_pportdata *ppd;
	struct rvt_qp __rcu *qp[2];
	/* non-zero when timer is set */
	unsigned long trap_timeout;
	unsigned long mkey_lease_timeout;
	__be64 gid_prefix;
	__be64 mkey;
	__be64 guids[HFI2_GUIDS_PER_PORT - 1];
	u64 tid;
	u64 n_rc_resends;
	u64 n_seq_naks;
	u64 n_rdma_seq;
	u64 n_rnr_naks;
	u64 n_other_naks;
	u64 n_loop_pkts;
	u64 n_pkt_drops;
	u64 n_vl15_dropped;
	u64 n_rc_timeouts;
	u64 n_rc_dupreq;
	u64 n_rc_seqnak;
	/* Hot-path per CPU counters to avoid cacheline trading to update */
	u64 __percpu *rc_acks;
	u64 __percpu *rc_qacks;
	u64 __percpu *rc_delayed_comp;

	u32 port_cap_flags;
	u16 mkey_lease_period;
	u16 pkey_violations;
	u16 qkey_violations;
	u16 mkey_violations;
	u32 sm_lid;
	u8 port_num;
	u8 sm_sl;
	u8 mkeyprot;
	u8 subnet_timeout;
	u8 vl_high_limit;

	/* protect changes in this struct */
	spinlock_t lock;
	struct rb_root mcast_tree;

	struct hfi_ctx *ctx;
	struct hfi_cq cmdq_tx;
	struct hfi_cq cmdq_rx;
	spinlock_t cmdq_tx_lock;
	spinlock_t cmdq_rx_lock;
	struct hfi_eq send_eq;
	struct hfi2_ibrcv sm_rcv;
	struct hfi2_ibrcv qp_rcv;
};

struct hfi2_ibdev {
	struct rvt_dev_info rdi;
	struct device *parent_dev;
	struct hfi_devdata *dd;
	__be64 node_guid;
	u8 num_pports;
	u8 oui[3];
	u8 rsm_mask;
	int assigned_node_id;
	struct hfi2_ibport *pport;
	struct ida qpn_even_table;
	struct ida qpn_odd_table;
	struct idr qp_ptr;
	unsigned long reserved_qps;
	spinlock_t qpt_lock;
	u32 n_ahs_allocated;
	spinlock_t n_ahs_lock;
	u32 n_qps_allocated;
	spinlock_t n_qps_lock;
	u32 n_mcast_grps_allocated; /* number of mcast groups allocated */
	spinlock_t n_mcast_grps_lock;

	/* per device cq worker */
	struct kthread_worker *worker;

	/* send functions, snoop overrides */
	int (*send_wqe)(struct hfi2_ibport *ibp, struct hfi2_qp_priv *priv);
	int (*send_ack)(struct hfi2_ibport *ibp, struct hfi2_qp_priv *priv,
			union hfi2_packet_header *hdr, size_t hwords,
			bool use_16b);
	/* receive interrupt functions, snoop intercepts */
	rhf_rcv_function_ptr *rhf_rcv_function_map;
	rhf_rcv_function_ptr rhf_rcv_functions[HFI2_RHF_RCV_TYPES];

	struct hfi_ctx sm_ctx;
	struct hfi_ctx qp_ctx;
};

/* TODO - for now use typecast below, revisit when fully RDMAVT integrated */
#define to_hfi_ibd(ibdev)	container_of((struct rvt_dev_info *)(ibdev),\
				struct hfi2_ibdev, rdi)

static inline struct hfi2_ibport *to_hfi_ibp(struct ib_device *ibdev,
							u8 port)
{
	struct hfi2_ibdev *ibd = to_hfi_ibd(ibdev);
	unsigned pidx = port - 1; /* IB number port from 1, hdw from 0 */

	if (pidx >= ibd->num_pports) {
		WARN_ON(pidx >= ibd->num_pports);
		return NULL;
	}
	return &(ibd->pport[pidx]);
}

static inline struct hfi_devdata *hfi_dd_from_ibdev(struct ib_device *ibdev)
{
	return to_hfi_ibd(ibdev)->dd;
}

/*
 * Return the QP with the given QPN
 * The caller must hold the rcu_read_lock(), and keep the lock until
 * the returned qp is no longer in use.
 * TODO - revisit if IDR is really best for a fast lookup_qpn.
 */
static inline struct rvt_qp *hfi2_lookup_qpn(struct hfi2_ibport *ibp,
						  u32 qpn) __must_hold(RCU)
{
	if (unlikely(qpn <= 1))
		return rcu_dereference(ibp->qp[qpn]);
	else
		return idr_find(&ibp->ibd->qp_ptr, qpn);
}

#define hfi2_get_pkey(ibp, index) \
	((index) >= (HFI_MAX_PKEYS) ? 0 : (ibp)->ppd->pkeys[(index)])

static inline u8 valid_ib_mtu(u16 mtu)
{
	return mtu == 256 || mtu == 512 ||
		mtu == 1024 || mtu == 2048 ||
		mtu == 4096;
}
int hfi2_check_ah(struct ib_device *ibdev, struct ib_ah_attr *ah_attr);
struct ib_ah *hfi2_create_ah(struct ib_pd *pd,
			     struct ib_ah_attr *ah_attr);
int hfi2_destroy_ah(struct ib_ah *ibah);
struct ib_ah *hfi2_create_qp0_ah(struct hfi2_ibport *ibp, u32 dlid);
int hfi2_modify_ah(struct ib_ah *ibah, struct ib_ah_attr *ah_attr);
int hfi2_query_ah(struct ib_ah *ibah, struct ib_ah_attr *ah_attr);
struct ib_qp *hfi2_create_qp(struct ib_pd *ibpd,
			     struct ib_qp_init_attr *init_attr,
			     struct ib_udata *udata);
struct rvt_qp *hfi2_lookup_qpn(struct hfi2_ibport *ibp, u32 qpn);
int hfi2_modify_qp(struct ib_qp *ibqp, struct ib_qp_attr *attr,
		   int attr_mask, struct ib_udata *udata);
int hfi2_query_qp(struct ib_qp *ibqp, struct ib_qp_attr *attr,
		  int attr_mask, struct ib_qp_init_attr *init_attr);
int hfi2_destroy_qp(struct ib_qp *ibqp);
int hfi2_error_qp(struct rvt_qp *qp, enum ib_wc_status err);
void hfi2_rc_error(struct rvt_qp *qp, enum ib_wc_status err);
void hfi2_rc_rcv(struct rvt_qp *qp, struct hfi2_ib_packet *packet);
void hfi2_uc_rcv(struct rvt_qp *qp, struct hfi2_ib_packet *packet);
void hfi2_ud_rcv(struct rvt_qp *qp, struct hfi2_ib_packet *packet);
void hfi2_ib_rcv(struct hfi2_ib_packet *packet);
int hfi2_rcv_wait(void *data);
int hfi2_lookup_pkey_idx(struct hfi2_ibport *ibp, u16 pkey);
int hfi2_get_rwqe(struct rvt_qp *qp, int wr_id_only);
void hfi2_migrate_qp(struct rvt_qp *qp);
void hfi2_make_ruc_header(struct rvt_qp *qp, struct ib_l4_headers *ohdr,
			  u32 bth0, u32 bth2);
void hfi2_make_16b_ruc_header(struct rvt_qp *qp, struct ib_l4_headers *ohdr,
			      u32 bth0, u32 bth2);
void hfi2_do_send(struct work_struct *work);
void hfi2_schedule_send(struct rvt_qp *qp);
void hfi2_send_complete(struct rvt_qp *qp, struct rvt_swqe *wqe,
			enum ib_wc_status status);
void hfi2_rc_send_complete(struct rvt_qp *qp, struct ib_l4_headers *ohdr);
int hfi2_make_uc_req(struct rvt_qp *qp);
int hfi2_make_ud_req(struct rvt_qp *qp);
int hfi2_make_rc_req(struct rvt_qp *qp);
void hfi2_copy_sge(struct rvt_sge_state *ss, void *data, u32 length,
		   int release);
void hfi2_skip_sge(struct rvt_sge_state *ss, u32 length, int release);
void hfi2_update_sge(struct rvt_sge_state *ss, u32 length);
int hfi2_post_send(struct ib_qp *ibqp, struct ib_send_wr *wr,
		   struct ib_send_wr **bad_wr);
int hfi2_post_receive(struct ib_qp *ibqp, struct ib_recv_wr *wr,
		      struct ib_recv_wr **bad_wr);
int hfi2_process_mad(struct ib_device *ibdev, int mad_flags, u8 port,
		     const struct ib_wc *in_wc, const struct ib_grh *in_grh,
		     const struct ib_mad_hdr *in_mad, size_t in_mad_size,
		     struct ib_mad_hdr *out_mad, size_t *out_mad_size,
		     u16 *out_mad_pkey_index);
int hfi2_create_agents(struct ib_device *ibdev);
void hfi2_free_agents(struct ib_device *ibdev);
int hfi2_multicast_attach(struct ib_qp *ibqp, union ib_gid *gid, u16 lid);
int hfi2_multicast_detach(struct ib_qp *ibqp, union ib_gid *gid, u16 lid);
struct hfi2_mcast *
hfi2_mcast_find(struct hfi2_ibport *ibp, union ib_gid *mgid);

/* Device specific */
int hfi2_send_wqe(struct hfi2_ibport *ibp, struct hfi2_qp_priv *qp_priv);
#ifdef HFI_VERBS_TEST
bool hfi2_drop_packet(void);
#endif
int hfi2_send_ack(struct hfi2_ibport *ibp, struct hfi2_qp_priv *qp_priv,
		  union hfi2_packet_header *ph, size_t hwords, bool use_16b);
void *hfi2_rcv_get_ebuf(struct hfi2_ibrcv *rcv, u16 idx, u32 offset);
void hfi2_rcv_advance(struct hfi2_ibrcv *rcv, u64 *rhf_entry);
int _hfi2_rcv_wait(struct hfi2_ibrcv *rcv, u64 **rhf_entry);
int hfi2_rcv_init(struct hfi2_ibport *ibp, struct hfi_ctx *ctx,
		  struct hfi2_ibrcv *rcv);
void hfi2_rcv_uninit(struct hfi2_ibrcv *rcv);
int hfi2_ctx_init(struct hfi2_ibdev *ibd, struct opa_core_ops *bus_ops);
void hfi2_ctx_uninit(struct hfi2_ibdev *ibd);
int hfi2_ctx_init_port(struct hfi2_ibport *ibp);
void hfi2_ctx_uninit_port(struct hfi2_ibport *ibp);
void hfi2_ctx_start_port(struct hfi2_ibport *ibp);
int hfi2_ctx_assign_qp(struct hfi2_ibport *ibp,
		       struct hfi2_qp_priv *qp_priv, bool is_user);
void hfi2_ctx_release_qp(struct hfi2_ibport *ibp, struct hfi2_qp_priv *qp_priv);
int hfi2_ib_add(struct hfi_devdata *dd, struct opa_core_ops *bus_ops);
void hfi2_ib_remove(struct hfi_devdata *dd);
void hfi2_cap_mask_chg(struct hfi2_ibport *ibp);
void hfi2_sys_guid_chg(struct hfi2_ibport *ibp);
void hfi2_node_desc_chg(struct hfi2_ibport *ibp);
void hfi2_verbs_register_sysfs(struct device *dev);

#endif
