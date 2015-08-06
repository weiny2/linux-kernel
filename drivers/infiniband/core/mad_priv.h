/*
 * Copyright (c) 2004, 2005, Voltaire, Inc. All rights reserved.
 * Copyright (c) 2005 Intel Corporation. All rights reserved.
 * Copyright (c) 2005 Sun Microsystems, Inc. All rights reserved.
 * Copyright (c) 2009 HNR Consulting. All rights reserved.
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

#ifndef __IB_MAD_PRIV_H__
#define __IB_MAD_PRIV_H__

#include <linux/completion.h>
#include <linux/err.h>
#include <linux/workqueue.h>
#include <rdma/ib_mad.h>
#include <rdma/ib_smi.h>
#include <rdma/stl_smi.h>


#define PFX "ib_mad: "

#define IB_MAD_QPS_CORE		2 /* Always QP0 and QP1 as a minimum */

/* QP and CQ parameters */
#define IB_MAD_QP_SEND_SIZE	128
#define IB_MAD_QP_RECV_SIZE	512
#define IB_MAD_QP_MIN_SIZE	64
#define IB_MAD_QP_MAX_SIZE	8192
#define IB_MAD_SEND_REQ_MAX_SG	2
#define IB_MAD_RECV_REQ_MAX_SG	1

#define IB_MAD_SEND_Q_PSN	0

/* Registration table sizes */
#define MAX_MGMT_CLASS		80
#define MAX_MGMT_VERSION	0x83 /* FIXME -- STL specific
					how big should this be for STL?
					This makes the version table huge
					for agent registrations */
#define MAX_MGMT_OUI		8
#define MAX_MGMT_VENDOR_RANGE2	(IB_MGMT_CLASS_VENDOR_RANGE2_END - \
				IB_MGMT_CLASS_VENDOR_RANGE2_START + 1)

/* shared betwee mad.c and jumbo_mad.c */
extern struct kmem_cache *jumbo_mad_cache;

struct ib_mad_list_head {
	struct list_head list;
	struct ib_mad_queue *mad_queue;
};

enum ib_mad_private_flags {
	IB_MAD_PRIV_FLAG_JUMBO = (1 << 0)
};
struct ib_mad_private_header {
	struct ib_mad_list_head mad_list;
	struct ib_mad_recv_wc recv_wc;
	struct ib_wc wc;
	u64 mapping;
	u64 flags;
} __attribute__ ((packed));

struct ib_mad_private {
	struct ib_mad_private_header header;
	struct ib_grh grh;
	union {
		struct ib_mad mad;
		struct ib_rmpp_mad rmpp_mad;
		struct ib_smp smp;
	} mad;
} __attribute__ ((packed));

/**
 * It might be possible to define this as part of the ib_mad_private by simply
 * extending the union there.
 * The problem is I don't know what other RDMA hardware will do if you attempt
 * to post >256B Rcv WR on their managment QP's.
 * Therefore we are going to try and keep this structure separate and only use
 * it on jumbo_mad capable devices.
 * Furthermore, this allows us to use smaller kmem_cache's on non-jumbo capable
 * devices for less memory ussage.
 */
struct jumbo_mad_private {
	struct ib_mad_private_header header;
	struct ib_grh grh;
	union {
		struct jumbo_mad mad;
		struct jumbo_rmpp_mad rmpp_mad;
		struct stl_smp smp;
	} mad;
} __attribute__ ((packed));

struct ib_rmpp_segment {
	struct list_head list;
	u32 num;
	u8 data[0];
};

struct ib_mad_agent_private {
	struct list_head agent_list;
	struct ib_mad_agent agent;
	struct ib_mad_reg_req *reg_req;
	struct ib_mad_qp_info *qp_info;

	spinlock_t lock;
	struct list_head send_list;
	struct list_head wait_list;
	struct list_head done_list;
	struct delayed_work timed_work;
	unsigned long timeout;
	struct list_head local_list;
	struct work_struct local_work;
	struct list_head rmpp_list;

	atomic_t refcount;
	struct completion comp;
};

struct ib_mad_snoop_private {
	struct ib_mad_agent agent;
	struct ib_mad_qp_info *qp_info;
	int snoop_index;
	int mad_snoop_flags;
	atomic_t refcount;
	struct completion comp;
};

struct ib_mad_send_wr_private {
	struct ib_mad_list_head mad_list;
	struct list_head agent_list;
	struct ib_mad_agent_private *mad_agent_priv;
	struct ib_mad_send_buf send_buf;
	u64 header_mapping;
	u64 payload_mapping;
	struct ib_send_wr send_wr;
	struct ib_sge sg_list[IB_MAD_SEND_REQ_MAX_SG];
	__be64 tid;
	unsigned long timeout;
	unsigned long sq_timeout;
	int max_retries;
	int retries_left;
	int retry;
	int refcount;
	enum ib_wc_status status;

	/* RMPP control */
	struct list_head rmpp_list;
	struct ib_rmpp_segment *last_ack_seg;
	struct ib_rmpp_segment *cur_seg;
	int last_ack;
	int seg_num;
	int newwin;
	int pad;
};

struct ib_mad_local_private {
	struct list_head completion_list;
	struct ib_mad_private *mad_priv; /* can be jumbo_mad_private */
	struct ib_mad_agent_private *recv_mad_agent;
	struct ib_mad_send_wr_private *mad_send_wr;
	size_t return_wc_byte_len;
};

struct ib_mad_mgmt_method_table {
	struct ib_mad_agent_private *agent[IB_MGMT_MAX_METHODS];
};

struct ib_mad_mgmt_class_table {
	struct ib_mad_mgmt_method_table *method_table[MAX_MGMT_CLASS];
};

struct ib_mad_mgmt_vendor_class {
	u8	oui[MAX_MGMT_OUI][3];
	struct ib_mad_mgmt_method_table *method_table[MAX_MGMT_OUI];
};

struct ib_mad_mgmt_vendor_class_table {
	struct ib_mad_mgmt_vendor_class *vendor_class[MAX_MGMT_VENDOR_RANGE2];
};

struct ib_mad_mgmt_version_table {
	struct ib_mad_mgmt_class_table *class;
	struct ib_mad_mgmt_vendor_class_table *vendor;
};

struct ib_mad_queue {
	spinlock_t lock;
	struct list_head list;
	int count;
	int max_active;
	struct ib_mad_qp_info *qp_info;
};

struct ib_mad_qp_info {
	struct ib_mad_port_private *port_priv;
	struct ib_qp *qp;
	struct ib_mad_queue send_queue;
	struct ib_mad_queue recv_queue;
	struct list_head overflow_list;
	spinlock_t snoop_lock;
	struct ib_mad_snoop_private **snoop_table;
	int snoop_table_size;
	atomic_t snoop_count;
	int supports_jumbo_mads;
	int send_queue_stalled;
};

struct ib_mad_port_private {
	struct list_head port_list;
	struct ib_device *device;
	int port_num;
	struct ib_cq *cq;
	struct ib_pd *pd;
	struct ib_mr *mr;

	spinlock_t reg_lock;
	/* FIXME with increased version range this table is huge */
	struct ib_mad_mgmt_version_table version[MAX_MGMT_VERSION];
	struct list_head agent_list;
	struct workqueue_struct *wq;
	struct work_struct work;
	struct ib_mad_qp_info qp_info[IB_MAD_QPS_CORE];
	int    supports_jumbo_mads;
};

int ib_send_mad(struct ib_mad_send_wr_private *mad_send_wr);

struct ib_mad_send_wr_private *
ib_find_send_mad(struct ib_mad_agent_private *mad_agent_priv,
		 struct ib_mad_recv_wc *mad_recv_wc);

void ib_mad_complete_send_wr(struct ib_mad_send_wr_private *mad_send_wr,
			     struct ib_mad_send_wc *mad_send_wc);

void ib_mark_mad_done(struct ib_mad_send_wr_private *mad_send_wr);

void ib_reset_mad_timeout(struct ib_mad_send_wr_private *mad_send_wr,
			  int timeout_ms);

static inline void deref_mad_agent(struct ib_mad_agent_private *mad_agent_priv)
{
	if (atomic_dec_and_test(&mad_agent_priv->refcount))
		complete(&mad_agent_priv->comp);
}


void dequeue_mad(struct ib_mad_list_head *mad_list);
struct ib_mad_agent_private * find_mad_agent(
				struct ib_mad_port_private *port_priv,
				struct ib_mad *mad);

void snoop_recv(struct ib_mad_qp_info *qp_info,
		struct ib_mad_recv_wc *mad_recv_wc,
		int mad_snoop_flags);
void wait_for_response(struct ib_mad_send_wr_private *mad_send_wr);
void adjust_timeout(struct ib_mad_agent_private *mad_agent_priv);
enum smi_action handle_ib_smi(struct ib_mad_port_private *port_priv,
			      struct ib_mad_qp_info *qp_info,
			      struct ib_wc *wc,
			      int port_num,
			      struct ib_mad_private *recv,
			      struct ib_mad_private *response);

#endif	/* __IB_MAD_PRIV_H__ */
