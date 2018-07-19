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
#include <rdma/rdma_vt.h>
#include <rdma/rdmavt_cq.h>
#include <rdma/ib_hdrs.h>
#include <rdma/opa_port_info.h>
#include <rdma/opa_addr.h>
#include <rdma/uverbs_ioctl.h>
#include "ib_compat.h"
#include "iowait.h"
#include "../hfi2.h"

#define HFI_VERBS_TEST
/* drop rate is percentage 0 - 100 */
#define HFI_MIN_DROP_RATE	0
#define HFI_MAX_DROP_RATE	20

#define HFI2_MAX_RDMA_ATOMIC  16
#define HFI2_INIT_RDMA_ATOMIC 255
#define HFI2_MAX_MAP_PER_FMR  32767
#define HFI2_PSN_CREDIT       16

#define IB_DEFAULT_GID_PREFIX	cpu_to_be64(0xfe80000000000000ULL)

/*
 * Maximum Verbs contexts is only for sizing arrays.
 * FXRTODO: default to just use 2, need to make tunable.
 */
#define HFI2_IB_MAX_CTXTS 8
#define HFI2_IB_DEF_CTXTS 2
/* Send contexts is really TX command queues (per port) */
#define HFI2_IB_MAX_SEND_CTXTS 2
#define HFI2_IB_DEF_SEND_CTXTS 2

/*
 * QP numbering defines
 * Unique QPN[23..16] value (KDETH_BASE) determines start of the KDETH
 * QPNs (lower 16 bits = 64K QPNs).
 * RDMAVT manages QPN allocation.
 */
#define HFI2_QPN_KDETH_PREFIX		BIT(7)	/* Gen1 default */
#define HFI2_QPN_KDETH_SIZE		0xFFFF
#define HFI2_QPN_MAP_MAX		BIT(8)	/* 8-bits to map to Recv Context */
#define HFI2_QPN_QOS_SHIFT		1
#define HFI2_MAX_QPS			16384	/* maximum SW will allocate */
#define HFI2_MAX_QP_WRS			0x3FFF	/* default maximum QP WRs */

/*
 * Module parameter defaults
 */
#define HFI2_MAX_PDS			0xFFFF  /* max protection domains */
#define HFI2_MAX_AHS			0xFFFF  /* max address handles */
#define HFI2_MAX_CQES			0x2FFFF /* max completion queue entries */
#define HFI2_MAX_CQS			0x1FFFF /* max completion queues */
#define HFI2_MAX_SGES			0x60	/* max SGEs */
#define HFI2_MAX_MCAST_GRPS		16384   /* max multicast groups */
#define HFI2_MAX_MCAST_QP_ATTACHED	16	/* max attached QPs */
#define HFI2_MAX_SRQS			1024	/* max SRQs */
#define HFI2_MAX_SRQ_SGES		128	/* max SRQ SGEs */
#define HFI2_MAX_SRQ_WRS		0x1FFFF /* max SRQ WRs */
#define HFI2_LKEY_TABLE_SIZE		16	/* LKEY table size */
#define HFI2_QP_TABLE_SIZE		256	/* Size of QP hash table */


#define HFI2_RHF_RCV_TYPES      8

#define GET_16B_PADDING(hwords, data) \
	(-((data) + ((hwords) << 2) + (SIZE_OF_CRC << 2) + 1) & 7)
#define GET_16B_NWORDS(hwords, data, extra_bytes) \
	(((data) + (SIZE_OF_CRC << 2) + 1 + (extra_bytes)) >> 2)

#define HFI2_QP_MAX_TIMEOUT		21
extern __be64 hfi2_sys_guid;
extern unsigned int hfi2_max_cqes;
extern unsigned int hfi2_max_cqs;
extern unsigned int hfi2_max_qp_wrs;
extern unsigned int hfi2_max_qps;
extern unsigned int hfi2_max_sges;
extern unsigned int hfi2_lkey_table_size;
extern unsigned int hfi2_max_mcast_grps;
extern unsigned int hfi2_max_mcast_qp_attached;
extern unsigned int hfi2_kdeth_qp;

extern bool no_verbs;

struct hfi2_ib_packet;
union hfi2_packet_header;

extern const enum ib_wc_opcode ib_hfi2_wc_opcode[];
extern const struct rvt_operation_params hfi2_post_parms[];
extern const u8 ib_hdr_len_by_opcode[];

struct hfi_tx_wc {
	struct ib_wc		ib_wc;
	u8			flags;
};

/*
 * hfi2 specific QP state which is hidden from rvt after queue pair created,
 * used for hfi2 driver specific interactions with hardware device.
 * @owner: the master rvt_qp structure for this QP
 * @s_hdr: pointer to packet header in memory for next send WQE
 * @s_ctx: hardware send context used for sending next packet
 * @ibrcv: receive context and receive buffer state
 * @bth2: last 4 bytes of BTH field in CPU endianness
 * @pkey: pkey corresponding to pkey_index in next packet
 * @hdr_type: describes packet type for next packet
 * @opcode: BTH.opcode field in CPU endianness
 * @s_sc: the SC value for next packet to send
 * @s_sl: the SL value for next packet to send
 * @send_becn: informs if next packet requires BECN bit
 * @s_iowait: iowait structure for SDMA/PIO draining on hfi1 (TODO for hfi2)
 * @middle_len: size of all middle packets if optimizing Middles
 * @lpsn: last psn in current wr
 * Below are for Verbs over native provider:
 * @outstanding_cnt: total number of outsanding commands
 * @outstanding_rd_cnt: total number of read or atomic commands
 * @cmd: array of TX commands sized based on SQ length
 * @wc: array of work completions pending to be delivered
 * @current_cidx: index into command buffer (@cmd)
 * @current_eidx: index into event buffer (@wc)
 * @fc_cidx: flow control command index
 * @fc_eidx: flow control event index
 * @recvq_root: ME root for RQ descriptors
 * @tpid: connected QP's remote PID
 * @rq_ctx: hardware context for RQ
 * @poll_qp: list for TX work completion processing in poll_cq
 */
struct hfi2_qp_priv {
	struct rvt_qp *owner;
	union hfi2_ib_dma_header *s_hdr;
	struct hfi2_ibtx *s_ctx;
	struct hfi2_ibrcv *ibrcv;
	u32 bth2;
	u16 pkey;
	u8 hdr_type;
	u8 opcode;
	u8 s_sc;
	u8 s_sl;
	bool send_becn;
	struct iowait s_iowait;
	u32 middle_len;
	u32 lpsn;
	u32 outstanding_cnt;
	u32 outstanding_rd_cnt;
	union hfi_tx_cq_command *cmd;
	struct hfi_tx_wc        *wc;
	int current_cidx;
	int current_eidx;
	int fc_cidx;
	int fc_eidx;
	u16 tpid;
	u32 recvq_root;
	struct hfi_ctx *rq_ctx;
	struct list_head poll_qp;
	struct completion pid_xchg_comp;
	u8 state;
	u8 pidex_hdr_type;
	u32 pidex_slid;
	u32 pidex_qpn;
	struct hfi_ibeq *recv_eq;
	struct list_head send_qp_ll;
	struct list_head recv_qp_ll;
	spinlock_t s_lock;
};

struct hfi2_ibtx {
	struct hfi2_ibport *ibp;
	struct hfi_ctx *ctx;
	struct hfi_cmdq_pair cmdq;
	struct hfi_pend_queue pend_cmdq;
	struct hfi_eq send_eq;
};

struct hfi2_ibrcv {
	struct hfi2_ibport *ibp;
	struct hfi_ctx *ctx;
	struct hfi_cmdq *cmdq_rx;
	struct hfi_pend_queue *pend_cmdq;
	struct hfi_eq eq;
	u32 rhq_update_mask;
	u16 egr_last_idx;
	u16 egr_pending_update;
	void *egr_base;
	struct task_struct *task;
	struct list_head wait_list;
};

struct hfi2_ibport {
	struct rvt_ibport rvp;
	struct hfi2_ibdev *ibd;
	struct device *dev; /* from IB's ib_device */
	struct hfi_pportdata *ppd;
	u8 port_num;
	struct hfi2_ibtx port_tx[HFI2_IB_MAX_SEND_CTXTS];
	/*
	 * hfi2_ibrcv structs contain eager receive resources and
	 * are placed here in hfi2_ibport as setup requires using the
	 * cmdq_rx above. The underlying receive contexts however are
	 * a per-device resource.
	 */
	struct hfi2_ibrcv sm_rcv;
	struct hfi2_ibrcv *qp_rcv[HFI2_IB_MAX_CTXTS];

	/* Informational counters beyond what rdmavt provides */
	struct hfi2_ib_stats *stats;

	/*
	 * prescan_9B/16B: indicates if verbs CCA prescan is turned on
	 * for 9B/16B
	 */
	bool prescan_9B;
	bool prescan_16B;
};

typedef void (*rhf_rcv_function_ptr)(struct hfi2_ib_packet *packet);

struct hfi2_ibdev {
	struct rvt_dev_info rdi;
	struct device *parent_dev;
	struct hfi_devdata *dd;
	__be64 node_guid;
	u8 num_pports;
	u8 oui[3];
	u8 rsm_mask;
	u8 rc_drop_max_rate;
	u8 num_send_cmdqs;
	u8 num_qp_ctxs;
	bool rc_drop_enabled;
	int assigned_node_id;
	struct hfi2_ibport *pport;
	struct ib_ucontext *ibkc;

	/* per device cq worker */
	struct kthread_worker *worker;

	/* send functions, snoop overrides */
	int (*send_wqe)(struct hfi2_ibport *ibp, struct hfi2_qp_priv *priv,
			bool allow_sleep);
	int (*send_ack)(struct hfi2_ibport *ibp, struct hfi2_qp_priv *priv,
			union hfi2_packet_header *hdr, size_t hwords);
	/* receive interrupt functions, snoop intercepts */
	rhf_rcv_function_ptr *rhf_rcv_function_map;
	rhf_rcv_function_ptr rhf_rcv_functions[HFI2_RHF_RCV_TYPES];

	struct kmem_cache *wqe_iov_cache;
	struct kmem_cache *wqe_dma_cache;
	struct hfi_ctx sm_ctx;
	struct hfi_ctx *qp_ctx[HFI2_IB_MAX_CTXTS];

	/* HFI2 uverbs spec root */
	const struct uverbs_spec_root *hfi2_root;
};

/* List of vma pointers to zap on release */
struct hfi_vma {
	struct vm_area_struct *vma;
	u16 cmdq_idx;
	struct list_head vma_list;
};

#define to_hfi_ibd(ibdev)	container_of(ib_to_rvt(ibdev),\
				struct hfi2_ibdev, rdi)

static inline struct hfi2_ibport *to_hfi_ibp(struct ib_device *ibdev,
					     u8 port)
{
	struct hfi2_ibdev *ibd = to_hfi_ibd(ibdev);
	unsigned int pidx = port - 1; /* IB number port from 1, hdw from 0 */

	if (pidx >= ibd->num_pports) {
		WARN_ON(pidx >= ibd->num_pports);
		return NULL;
	}
	return &ibd->pport[pidx];
}

static inline struct hfi_devdata *hfi_dd_from_ibdev(struct ib_device *ibdev)
{
	return to_hfi_ibd(ibdev)->dd;
}

#define hfi2_get_pkey(ibp, index) \
	((index) >= (HFI_MAX_PKEYS) ? 0 : (ibp)->ppd->pkeys[(index)])

static inline u8 valid_ib_mtu(u16 mtu)
{
	return mtu == 256 || mtu == 512 ||
		mtu == 1024 || mtu == 2048 ||
		mtu == 4096;
}

static inline void hfi2_update_ah_attr(struct ib_device *ibdev,
				       struct rdma_ah_attr *attr)
{
	struct hfi_pportdata *ppd;
	struct hfi2_ibport *ibp;
	u32 dlid = rdma_ah_get_dlid(attr);

	/*
	 * Kernel clients may not have setup GRH information
	 * Set that here.
	 */
	ibp = to_hfi_ibp(ibdev, rdma_ah_get_port_num(attr));
	ppd = ibp->ppd;
	if ((((dlid >= be16_to_cpu(IB_MULTICAST_LID_BASE)) ||
	      (ppd->lid >= be16_to_cpu(IB_MULTICAST_LID_BASE))) &&
	     (dlid != be32_to_cpu(OPA_LID_PERMISSIVE)) &&
	     (dlid != be16_to_cpu(IB_LID_PERMISSIVE)) &&
	     (!(rdma_ah_get_ah_flags(attr) & IB_AH_GRH))) ||
	    (rdma_ah_get_make_grd(attr))) {
		rdma_ah_set_ah_flags(attr, IB_AH_GRH);
		rdma_ah_set_interface_id(attr, OPA_MAKE_ID(dlid));
		rdma_ah_set_subnet_prefix(attr, ibp->rvp.gid_prefix);
	}
}

void hfi_set_middle_length(struct rvt_qp *qp, u32 bth2, bool is_middle);
void hfi_return_cnp(struct hfi2_ib_packet *packet, u32 src_qpn, u16 pkey);
void hfi_return_cnp_bypass(struct hfi2_ib_packet *packet, u32 src_qpn,
			   u16 pkey);

/* callbacks registered with rdmavt */
void qp_iter_print(struct seq_file *s, struct rvt_qp_iter *iter);
void *qp_priv_alloc(struct rvt_dev_info *rdi, struct rvt_qp *qp);
void qp_priv_free(struct rvt_dev_info *rdi, struct rvt_qp *qp);
unsigned int free_all_qps(struct rvt_dev_info *rdi);
void flush_qp_waiters(struct rvt_qp *qp);
void stop_send_queue(struct rvt_qp *qp);
void quiesce_qp(struct rvt_qp *qp);
void notify_qp_reset(struct rvt_qp *qp);
int mtu_to_path_mtu(u32 mtu);
u16 mtu_from_sl(struct hfi2_ibport *ibp, u8 sl);
u32 mtu_from_qp(struct rvt_dev_info *rdi, struct rvt_qp *qp, u32 pmtu);
int get_pmtu_from_attr(struct rvt_dev_info *rdi, struct rvt_qp *qp,
		       struct ib_qp_attr *attr);
void notify_error_qp(struct rvt_qp *qp);
void hfi2_error_port_qps(struct hfi_pportdata *ppd, u8 sl);
int hfi2_check_modify_qp(struct rvt_qp *qp, struct ib_qp_attr *attr,
			 int attr_mask, struct ib_udata *udata);
void hfi2_modify_qp(struct rvt_qp *qp, struct ib_qp_attr *attr,
		    int attr_mask, struct ib_udata *udata);
int hfi2_check_send_wqe(struct rvt_qp *qp,
			struct rvt_swqe *wqe);
int wqe_cache_create(struct hfi2_ibdev *ibd);
void wqe_cache_destroy(struct hfi2_ibdev *ibd);
void hfi2_notify_new_ah(struct ib_device *ibdev, struct rdma_ah_attr *ah_attr,
			struct rvt_ah *ah);
int hfi2_check_ah(struct ib_device *ibdev, struct rdma_ah_attr *ah_attr);
void hfi2_rc_error(struct rvt_qp *qp, enum ib_wc_status err);
void hfi2_rc_rcv(struct hfi2_ib_packet *packet);
void hfi2_uc_rcv(struct hfi2_ib_packet *packet);
void hfi2_ud_rcv(struct hfi2_ib_packet *packet);
void hfi2_ib_rcv(struct hfi2_ib_packet *packet);
int hfi2_rcv_wait(void *data);
int hfi2_lookup_pkey_idx(struct hfi2_ibport *ibp, u16 pkey);
int hfi2_get_rwqe(struct rvt_qp *qp, int wr_id_only);
void hfi2_migrate_qp(struct rvt_qp *qp);
void hfi2_make_ruc_header(struct rvt_qp *qp, struct ib_other_headers *ohdr,
			  u32 bth0, u32 bth2, bool is_middle);
void hfi2_do_send_from_rvt(struct rvt_qp *qp);
void hfi2_do_work(struct work_struct *work);
void hfi2_schedule_send_no_lock(struct rvt_qp *qp);
void hfi2_schedule_send(struct rvt_qp *qp);
void hfi2_send_complete(struct rvt_qp *qp, struct rvt_swqe *wqe,
			enum ib_wc_status status);
void hfi2_rc_send_complete(struct rvt_qp *qp, u32 opcode, u32 bth2);
int hfi2_make_uc_req(struct rvt_qp *qp);
int hfi2_make_ud_req(struct rvt_qp *qp);
int hfi2_make_rc_req(struct rvt_qp *qp);
void hfi2_copy_sge(struct rvt_sge_state *ss, void *data, u32 length,
		   bool release, bool copy_last);
int hfi2_process_mad(struct ib_device *ibdev, int mad_flags, u8 port,
		     const struct ib_wc *in_wc, const struct ib_grh *in_grh,
		     const struct ib_mad_hdr *in_mad, size_t in_mad_size,
		     struct ib_mad_hdr *out_mad, size_t *out_mad_size,
		     u16 *out_mad_pkey_index);
void hfi2_bad_pkey(struct hfi2_ibport *ibp, u32 key, u32 sl,
		    u32 qp1, u32 qp2, u32 lid1, u32 lid2);
int hfi2_ruc_check_hdr(struct hfi2_ibport *ibp, struct hfi2_ib_packet *packet);
void hfi2_rc_hdrerr(struct hfi2_ib_packet *packet);

/* Device specific */
int hfi2_send_wqe(struct hfi2_ibport *ibp, struct hfi2_qp_priv *qp_priv,
		  bool allow_sleep);
#ifdef HFI_VERBS_TEST
bool hfi2_drop_packet(struct rvt_qp *qp);
#endif
bool hfi2_is_verbs_resp_sl(struct hfi_pportdata *ppd, u8 sl);
int hfi2_send_ack(struct hfi2_ibport *ibp, struct hfi2_qp_priv *qp_priv,
		  union hfi2_packet_header *ph, size_t hwords);
void *hfi2_rcv_get_ebuf(struct hfi2_ibrcv *rcv, u16 idx, u32 offset);
void *hfi2_rcv_get_ebuf_ptr(struct hfi2_ibrcv *rcv, u16 idx, u32 offset);
void hfi2_rcv_advance(struct hfi2_ibrcv *rcv, u64 *rhf_entry);
int _hfi2_rcv_wait(struct hfi2_ibrcv *rcv, u64 **rhf_entry);
int hfi2_rcv_init(struct hfi2_ibport *ibp, struct hfi_ctx *ctx,
		  struct hfi2_ibrcv *rcv, int cmdq_idx);
void hfi2_rcv_uninit(struct hfi2_ibrcv *rcv);
int hfi2_ctx_init(struct hfi2_ibdev *ibd, int num_ctxs, int num_cmdqs);
void hfi2_ctx_uninit(struct hfi2_ibdev *ibd);
int hfi2_ctx_init_port(struct hfi2_ibport *ibp);
void hfi2_ctx_uninit_port(struct hfi2_ibport *ibp);
void hfi2_ctx_start_port(struct hfi2_ibport *ibp);
void hfi2_ctx_assign_qp(struct hfi2_qp_priv *qp_priv, bool is_user);
void hfi2_ctx_release_qp(struct hfi2_qp_priv *qp_priv);
int hfi2_ib_add(struct hfi_devdata *dd);
void hfi2_ib_remove(struct hfi_devdata *dd);
void hfi2_cap_mask_chg(struct hfi2_ibport *ibp);
void hfi2_sys_guid_chg(struct hfi2_ibport *ibp);
void hfi2_node_desc_chg(struct hfi2_ibport *ibp);
void hfi2_verbs_register_sysfs(struct device *dev);
void hfi_send_cnp(struct hfi2_ibport *ibp, struct rvt_qp *qp, u8 sl,
		  bool grh, bool use_16b);
void process_rcv_qp_work(struct hfi2_ib_packet *packet);
void hfi2_restart_rc(struct rvt_qp *qp, u32 psn, int wait);

void hfi_verbs_dbg_init(struct hfi_devdata *dd);
#endif
