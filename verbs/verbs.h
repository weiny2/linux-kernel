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

#include <linux/slab.h>
#include <rdma/ib_verbs.h>
#include <rdma/opa_smi.h>
#include <rdma/opa_port_info.h>
#include <rdma/opa_core.h>
#include "ib_compat.h"

/* TODO - these carried from WFR driver */
#define OPA_IB_MAX_RDMA_ATOMIC  16
#define OPA_IB_INIT_RDMA_ATOMIC 255
#define OPA_IB_MAX_MAP_PER_FMR  32767
#define OPA_IB_MAX_MSG_SZ       0x80000000
#define OPA_IB_PORT_NUM_PKEYS   16
#define IB_DEFAULT_GID_PREFIX	cpu_to_be64(0xfe80000000000000ULL)

/* TODO - placeholders */
extern __be64 opa_ib_sys_guid;

extern unsigned int opa_ib_max_cqes;

struct opa_ib_portdata {
	__be64 gid_prefix;
	__be64 guid;
	u32 lstate;
	u32 ibmtu;
	u32 port_cap_flags;
	u16 lid;
	u16 sm_lid;
	u16 link_speed_supported;
	u16 link_speed_enabled;
	u16 link_speed_active;
	u16 link_width_supported;
	u16 link_width_enabled;
	u16 link_width_active;
	/* list of pkeys programmed; 0 if not set */
	u16 pkeys[OPA_IB_PORT_NUM_PKEYS];
	u16 pkey_violations;
	u16 qkey_violations;
	u8 lmc;
	u8 max_vls;
	u8 sm_sl;
	u8 subnet_timeout;
};

struct opa_ib_data {
	struct ib_device ibdev;
	struct opa_core_device_id id;
	__be64 node_guid;
	u8 num_pports;
	u8 oui[3];
	struct opa_ib_portdata *pport;
	struct device *parent_dev;
};

struct opa_ib_pd {
	struct ib_pd ibpd;
	int is_user;
};

struct opa_ib_ah {
	struct ib_ah ibah;
	struct ib_ah_attr attr;
};

struct opa_ib_qp {
	struct ib_qp ibqp;
};

struct opa_ib_cq {
	struct ib_cq ibcq;
};

struct opa_ib_mr {
	struct ib_mr ibmr;
};

struct opa_ucontext {
	struct ib_ucontext ibucontext;
};

#define to_opa_ibpd(pd)	container_of((pd), struct opa_ib_pd, ibpd)
#define to_opa_ibah(ah)	container_of((ah), struct opa_ib_ah, ibah)
#define to_opa_ibqp(qp)	container_of((qp), struct opa_ib_qp, ibqp)
#define to_opa_ibcq(cq)	container_of((cq), struct opa_ib_cq, ibcq)
#define to_opa_ibmr(mr)	container_of((mr), struct opa_ib_mr, ibmr)
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

struct ib_pd *opa_ib_alloc_pd(struct ib_device *ibdev,
			      struct ib_ucontext *context,
			      struct ib_udata *udata);
int opa_ib_dealloc_pd(struct ib_pd *ibpd);
struct ib_ah *opa_ib_create_ah(struct ib_pd *pd,
			       struct ib_ah_attr *ah_attr);
int opa_ib_destroy_ah(struct ib_ah *ibah);
int opa_ib_modify_ah(struct ib_ah *ibah, struct ib_ah_attr *ah_attr);
int opa_ib_query_ah(struct ib_ah *ibah, struct ib_ah_attr *ah_attr);
struct ib_qp *opa_ib_create_qp(struct ib_pd *ibpd,
			       struct ib_qp_init_attr *init_attr,
			       struct ib_udata *udata);
int opa_ib_modify_qp(struct ib_qp *ibqp, struct ib_qp_attr *attr,
		  int attr_mask, struct ib_udata *udata);
int opa_ib_query_qp(struct ib_qp *ibqp, struct ib_qp_attr *attr,
		    int attr_mask, struct ib_qp_init_attr *init_attr);
int opa_ib_destroy_qp(struct ib_qp *ibqp);
struct ib_cq *opa_ib_create_cq(struct ib_device *ibdev, int entries,
			       int comp_vector, struct ib_ucontext *context,
			       struct ib_udata *udata);
int opa_ib_destroy_cq(struct ib_cq *ibcq);
int opa_ib_poll_cq(struct ib_cq *ibcq, int num_entries, struct ib_wc *entry);
int opa_ib_req_notify_cq(struct ib_cq *ibcq, enum ib_cq_notify_flags notify_flags);
int opa_ib_resize_cq(struct ib_cq *ibcq, int cqe, struct ib_udata *udata);
struct ib_mr *opa_ib_get_dma_mr(struct ib_pd *pd, int acc);
int opa_ib_dereg_mr(struct ib_mr *ibmr);
int opa_ib_post_send(struct ib_qp *ibqp, struct ib_send_wr *wr,
		     struct ib_send_wr **bad_wr);
int opa_ib_post_receive(struct ib_qp *ibqp, struct ib_recv_wr *wr,
			struct ib_recv_wr **bad_wr);
int opa_ib_process_mad(struct ib_device *ibdev, int mad_flags, u8 port,
			struct ib_wc *in_wc, struct ib_grh *in_grh,
			struct ib_mad *in_mad, struct ib_mad *out_mad);
#endif
