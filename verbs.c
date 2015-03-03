/*
 * Copyright (c) 2012 - 2015 Intel Corporation.  All rights reserved.
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

#include <rdma/ib_mad.h>
#include <rdma/ib_user_verbs.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/utsname.h>
#include <linux/rculist.h>
#include <linux/mm.h>
#include <linux/random.h>

#include "hfi.h"
#include "common.h"
#include "device.h"
#include "trace.h"
#include "qp.h"
#include "sdma.h"

unsigned int ib_qib_lkey_table_size = 16;
module_param_named(lkey_table_size, ib_qib_lkey_table_size, uint,
		   S_IRUGO);
MODULE_PARM_DESC(lkey_table_size,
		 "LKEY table size in bits (2^n, 1 <= n <= 23)");

static unsigned int ib_qib_max_pds = 0xFFFF;
module_param_named(max_pds, ib_qib_max_pds, uint, S_IRUGO);
MODULE_PARM_DESC(max_pds,
		 "Maximum number of protection domains to support");

static unsigned int ib_qib_max_ahs = 0xFFFF;
module_param_named(max_ahs, ib_qib_max_ahs, uint, S_IRUGO);
MODULE_PARM_DESC(max_ahs, "Maximum number of address handles to support");

unsigned int ib_qib_max_cqes = 0x2FFFF;
module_param_named(max_cqes, ib_qib_max_cqes, uint, S_IRUGO);
MODULE_PARM_DESC(max_cqes,
		 "Maximum number of completion queue entries to support");

unsigned int ib_qib_max_cqs = 0x1FFFF;
module_param_named(max_cqs, ib_qib_max_cqs, uint, S_IRUGO);
MODULE_PARM_DESC(max_cqs, "Maximum number of completion queues to support");

unsigned int ib_qib_max_qp_wrs = 0x3FFF;
module_param_named(max_qp_wrs, ib_qib_max_qp_wrs, uint, S_IRUGO);
MODULE_PARM_DESC(max_qp_wrs, "Maximum number of QP WRs to support");

unsigned int ib_qib_max_qps = 16384;
module_param_named(max_qps, ib_qib_max_qps, uint, S_IRUGO);
MODULE_PARM_DESC(max_qps, "Maximum number of QPs to support");

unsigned int ib_qib_max_sges = 0x60;
module_param_named(max_sges, ib_qib_max_sges, uint, S_IRUGO);
MODULE_PARM_DESC(max_sges, "Maximum number of SGEs to support");

unsigned int ib_qib_max_mcast_grps = 16384;
module_param_named(max_mcast_grps, ib_qib_max_mcast_grps, uint, S_IRUGO);
MODULE_PARM_DESC(max_mcast_grps,
		 "Maximum number of multicast groups to support");

unsigned int ib_qib_max_mcast_qp_attached = 16;
module_param_named(max_mcast_qp_attached, ib_qib_max_mcast_qp_attached,
		   uint, S_IRUGO);
MODULE_PARM_DESC(max_mcast_qp_attached,
		 "Maximum number of attached QPs to support");

unsigned int ib_qib_max_srqs = 1024;
module_param_named(max_srqs, ib_qib_max_srqs, uint, S_IRUGO);
MODULE_PARM_DESC(max_srqs, "Maximum number of SRQs to support");

unsigned int ib_qib_max_srq_sges = 128;
module_param_named(max_srq_sges, ib_qib_max_srq_sges, uint, S_IRUGO);
MODULE_PARM_DESC(max_srq_sges, "Maximum number of SRQ SGEs to support");

unsigned int ib_qib_max_srq_wrs = 0x1FFFF;
module_param_named(max_srq_wrs, ib_qib_max_srq_wrs, uint, S_IRUGO);
MODULE_PARM_DESC(max_srq_wrs, "Maximum number of SRQ WRs support");

static void verbs_sdma_complete(
	struct sdma_txreq *cookie,
	int status,
	int drained);

/*
 * Note that it is OK to post send work requests in the SQE and ERR
 * states; qib_do_send() will process them and generate error
 * completions as per IB 1.2 C10-96.
 */
const int ib_qib_state_ops[IB_QPS_ERR + 1] = {
	[IB_QPS_RESET] = 0,
	[IB_QPS_INIT] = QIB_POST_RECV_OK,
	[IB_QPS_RTR] = QIB_POST_RECV_OK | QIB_PROCESS_RECV_OK,
	[IB_QPS_RTS] = QIB_POST_RECV_OK | QIB_PROCESS_RECV_OK |
	    QIB_POST_SEND_OK | QIB_PROCESS_SEND_OK |
	    QIB_PROCESS_NEXT_SEND_OK,
	[IB_QPS_SQD] = QIB_POST_RECV_OK | QIB_PROCESS_RECV_OK |
	    QIB_POST_SEND_OK | QIB_PROCESS_SEND_OK,
	[IB_QPS_SQE] = QIB_POST_RECV_OK | QIB_PROCESS_RECV_OK |
	    QIB_POST_SEND_OK | QIB_FLUSH_SEND,
	[IB_QPS_ERR] = QIB_POST_RECV_OK | QIB_FLUSH_RECV |
	    QIB_POST_SEND_OK | QIB_FLUSH_SEND,
};

struct qib_ucontext {
	struct ib_ucontext ibucontext;
};

static inline struct qib_ucontext *to_iucontext(struct ib_ucontext
						  *ibucontext)
{
	return container_of(ibucontext, struct qib_ucontext, ibucontext);
}

/*
 * Translate ib_wr_opcode into ib_wc_opcode.
 */
const enum ib_wc_opcode ib_qib_wc_opcode[] = {
	[IB_WR_RDMA_WRITE] = IB_WC_RDMA_WRITE,
	[IB_WR_RDMA_WRITE_WITH_IMM] = IB_WC_RDMA_WRITE,
	[IB_WR_SEND] = IB_WC_SEND,
	[IB_WR_SEND_WITH_IMM] = IB_WC_SEND,
	[IB_WR_RDMA_READ] = IB_WC_RDMA_READ,
	[IB_WR_ATOMIC_CMP_AND_SWP] = IB_WC_COMP_SWAP,
	[IB_WR_ATOMIC_FETCH_AND_ADD] = IB_WC_FETCH_ADD
};

/*
 * Length of header by opcode, 0 --> not supported
 */
const u8 hdr_len_by_opcode[256] = {
	/* RC */
	[IB_OPCODE_RC_SEND_FIRST]                     = 12 + 8,
	[IB_OPCODE_RC_SEND_MIDDLE]                    = 12 + 8,
	[IB_OPCODE_RC_SEND_LAST]                      = 12 + 8,
	[IB_OPCODE_RC_SEND_LAST_WITH_IMMEDIATE]       = 12 + 8 + 4,
	[IB_OPCODE_RC_SEND_ONLY]                      = 12 + 8,
	[IB_OPCODE_RC_SEND_ONLY_WITH_IMMEDIATE]       = 12 + 8 + 4,
	[IB_OPCODE_RC_RDMA_WRITE_FIRST]               = 12 + 8 + 16,
	[IB_OPCODE_RC_RDMA_WRITE_MIDDLE]              = 12 + 8,
	[IB_OPCODE_RC_RDMA_WRITE_LAST]                = 12 + 8,
	[IB_OPCODE_RC_RDMA_WRITE_LAST_WITH_IMMEDIATE] = 12 + 8 + 4,
	[IB_OPCODE_RC_RDMA_WRITE_ONLY]                = 12 + 8 + 16,
	[IB_OPCODE_RC_RDMA_WRITE_ONLY_WITH_IMMEDIATE] = 12 + 8 + 20,
	[IB_OPCODE_RC_RDMA_READ_REQUEST]              = 12 + 8 + 16,
	[IB_OPCODE_RC_RDMA_READ_RESPONSE_FIRST]       = 12 + 8 + 4,
	[IB_OPCODE_RC_RDMA_READ_RESPONSE_MIDDLE]      = 12 + 8,
	[IB_OPCODE_RC_RDMA_READ_RESPONSE_LAST]        = 12 + 8 + 4,
	[IB_OPCODE_RC_RDMA_READ_RESPONSE_ONLY]        = 12 + 8 + 4,
	[IB_OPCODE_RC_ACKNOWLEDGE]                    = 12 + 8 + 4,
	[IB_OPCODE_RC_ATOMIC_ACKNOWLEDGE]             = 12 + 8 + 4,
	[IB_OPCODE_RC_COMPARE_SWAP]                   = 12 + 8 + 28,
	[IB_OPCODE_RC_FETCH_ADD]                      = 12 + 8 + 28,
	/* UC */
	[IB_OPCODE_UC_SEND_FIRST]                     = 12 + 8,
	[IB_OPCODE_UC_SEND_MIDDLE]                    = 12 + 8,
	[IB_OPCODE_UC_SEND_LAST]                      = 12 + 8,
	[IB_OPCODE_UC_SEND_LAST_WITH_IMMEDIATE]       = 12 + 8 + 4,
	[IB_OPCODE_UC_SEND_ONLY]                      = 12 + 8,
	[IB_OPCODE_UC_SEND_ONLY_WITH_IMMEDIATE]       = 12 + 8 + 4,
	[IB_OPCODE_UC_RDMA_WRITE_FIRST]               = 12 + 8 + 16,
	[IB_OPCODE_UC_RDMA_WRITE_MIDDLE]              = 12 + 8,
	[IB_OPCODE_UC_RDMA_WRITE_LAST]                = 12 + 8,
	[IB_OPCODE_UC_RDMA_WRITE_LAST_WITH_IMMEDIATE] = 12 + 8 + 4,
	[IB_OPCODE_UC_RDMA_WRITE_ONLY]                = 12 + 8 + 16,
	[IB_OPCODE_UC_RDMA_WRITE_ONLY_WITH_IMMEDIATE] = 12 + 8 + 20,
	/* UD */
	[IB_OPCODE_UD_SEND_ONLY]                      = 12 + 8 + 8,
	[IB_OPCODE_UD_SEND_ONLY_WITH_IMMEDIATE]       = 12 + 8 + 12
};

/*
 * System image GUID.
 */
__be64 ib_qib_sys_image_guid;

/**
 * qib_copy_sge - copy data to SGE memory
 * @ss: the SGE state
 * @data: the data to copy
 * @length: the length of the data
 */
void qib_copy_sge(struct qib_sge_state *ss, void *data, u32 length, int release)
{
	struct qib_sge *sge = &ss->sge;

	while (length) {
		u32 len = sge->length;

		if (len > length)
			len = length;
		if (len > sge->sge_length)
			len = sge->sge_length;
		BUG_ON(len == 0);
		memcpy(sge->vaddr, data, len);
		sge->vaddr += len;
		sge->length -= len;
		sge->sge_length -= len;
		if (sge->sge_length == 0) {
			if (release)
				qib_put_mr(sge->mr);
			if (--ss->num_sge)
				*sge = *ss->sg_list++;
		} else if (sge->length == 0 && sge->mr->lkey) {
			if (++sge->n >= QIB_SEGSZ) {
				if (++sge->m >= sge->mr->mapsz)
					break;
				sge->n = 0;
			}
			sge->vaddr =
				sge->mr->map[sge->m]->segs[sge->n].vaddr;
			sge->length =
				sge->mr->map[sge->m]->segs[sge->n].length;
		}
		data += len;
		length -= len;
	}
}

/**
 * qib_skip_sge - skip over SGE memory - XXX almost dup of prev func
 * @ss: the SGE state
 * @length: the number of bytes to skip
 */
void qib_skip_sge(struct qib_sge_state *ss, u32 length, int release)
{
	struct qib_sge *sge = &ss->sge;

	while (length) {
		u32 len = sge->length;

		if (len > length)
			len = length;
		if (len > sge->sge_length)
			len = sge->sge_length;
		BUG_ON(len == 0);
		sge->vaddr += len;
		sge->length -= len;
		sge->sge_length -= len;
		if (sge->sge_length == 0) {
			if (release)
				qib_put_mr(sge->mr);
			if (--ss->num_sge)
				*sge = *ss->sg_list++;
		} else if (sge->length == 0 && sge->mr->lkey) {
			if (++sge->n >= QIB_SEGSZ) {
				if (++sge->m >= sge->mr->mapsz)
					break;
				sge->n = 0;
			}
			sge->vaddr =
				sge->mr->map[sge->m]->segs[sge->n].vaddr;
			sge->length =
				sge->mr->map[sge->m]->segs[sge->n].length;
		}
		length -= len;
	}
}

/*
 * Copy from the SGEs to the data buffer.
 */
void qib_copy_from_sge(void *data, struct qib_sge_state *ss, u32 length)
{
	struct qib_sge *sge = &ss->sge;

	while (length) {
		u32 len = sge->length;

		if (len > length)
			len = length;
		if (len > sge->sge_length)
			len = sge->sge_length;
		BUG_ON(len == 0);
		memcpy(data, sge->vaddr, len);
		sge->vaddr += len;
		sge->length -= len;
		sge->sge_length -= len;
		if (sge->sge_length == 0) {
			if (--ss->num_sge)
				*sge = *ss->sg_list++;
		} else if (sge->length == 0 && sge->mr->lkey) {
			if (++sge->n >= QIB_SEGSZ) {
				if (++sge->m >= sge->mr->mapsz)
					break;
				sge->n = 0;
			}
			sge->vaddr =
				sge->mr->map[sge->m]->segs[sge->n].vaddr;
			sge->length =
				sge->mr->map[sge->m]->segs[sge->n].length;
		}
		data += len;
		length -= len;
	}
}

/**
 * qib_post_one_send - post one RC, UC, or UD send work request
 * @qp: the QP to post on
 * @wr: the work request to send
 */
static int qib_post_one_send(struct qib_qp *qp, struct ib_send_wr *wr,
	int *scheduled)
{
	struct qib_swqe *wqe;
	u32 next;
	int i;
	int j;
	int acc;
	int ret;
	unsigned long flags;
	struct qib_lkey_table *rkt;
	struct qib_pd *pd;
	u8 sc5;
	struct hfi_devdata *dd = dd_from_ibdev(qp->ibqp.device);
	struct qib_pportdata *ppd;
	struct qib_ibport *ibp;

	spin_lock_irqsave(&qp->s_lock, flags);
	ppd = &dd->pport[qp->port_num - 1];
	ibp = &ppd->ibport_data;

	/* Check that state is OK to post send. */
	if (unlikely(!(ib_qib_state_ops[qp->state] & QIB_POST_SEND_OK)))
		goto bail_inval;

	/* IB spec says that num_sge == 0 is OK. */
	if (wr->num_sge > qp->s_max_sge)
		goto bail_inval;

	/*
	 * Don't allow RDMA reads or atomic operations on UC or
	 * undefined operations.
	 * Make sure buffer is large enough to hold the result for atomics.
	 */
	if (wr->opcode == IB_WR_FAST_REG_MR) {
		if (qib_fast_reg_mr(qp, wr))
			goto bail_inval;
	} else if (qp->ibqp.qp_type == IB_QPT_UC) {
		if ((unsigned) wr->opcode >= IB_WR_RDMA_READ)
			goto bail_inval;
	} else if (qp->ibqp.qp_type != IB_QPT_RC) {
		/* Check IB_QPT_SMI, IB_QPT_GSI, IB_QPT_UD opcode */
		if (wr->opcode != IB_WR_SEND &&
		    wr->opcode != IB_WR_SEND_WITH_IMM)
			goto bail_inval;
		/* Check UD destination address PD */
		if (qp->ibqp.pd != wr->wr.ud.ah->pd)
			goto bail_inval;
	} else if ((unsigned) wr->opcode > IB_WR_ATOMIC_FETCH_AND_ADD)
		goto bail_inval;
	else if (wr->opcode >= IB_WR_ATOMIC_CMP_AND_SWP &&
		   (wr->num_sge == 0 ||
		    wr->sg_list[0].length < sizeof(u64) ||
		    wr->sg_list[0].addr & (sizeof(u64) - 1)))
		goto bail_inval;
	else if (wr->opcode >= IB_WR_RDMA_READ && !qp->s_max_rd_atomic)
		goto bail_inval;

	next = qp->s_head + 1;
	if (next >= qp->s_size)
		next = 0;
	if (next == qp->s_last) {
		ret = -ENOMEM;
		goto bail;
	}

	rkt = &to_idev(qp->ibqp.device)->lk_table;
	pd = to_ipd(qp->ibqp.pd);
	wqe = get_swqe_ptr(qp, qp->s_head);
	wqe->wr = *wr;
	wqe->length = 0;
	j = 0;
	if (wr->num_sge) {
		acc = wr->opcode >= IB_WR_RDMA_READ ?
			IB_ACCESS_LOCAL_WRITE : 0;
		for (i = 0; i < wr->num_sge; i++) {
			u32 length = wr->sg_list[i].length;
			int ok;

			if (length == 0)
				continue;
			ok = qib_lkey_ok(rkt, pd, &wqe->sg_list[j],
					 &wr->sg_list[i], acc);
			if (!ok)
				goto bail_inval_free;
			wqe->length += length;
			j++;
		}
		wqe->wr.num_sge = j;
	}
	if (qp->ibqp.qp_type == IB_QPT_UC ||
	    qp->ibqp.qp_type == IB_QPT_RC) {
		if (wqe->length > 0x80000000U)
			goto bail_inval_free;
		sc5 = ibp->sl_to_sc[qp->remote_ah_attr.sl];
	} else if (wqe->length > (dd_from_ibdev(qp->ibqp.device)->pport +
				  qp->port_num - 1)->ibmtu)
		goto bail_inval_free;
	else {
		struct qib_ah *ah = to_iah(wr->wr.ud.ah);

		atomic_inc(&ah->refcount);
		sc5 = ibp->sl_to_sc[ah->attr.sl];
	}
	wqe->ssn = qp->s_ssn++;
	qp->s_head = next;

	ret = 0;
	goto bail;

bail_inval_free:
	while (j) {
		struct qib_sge *sge = &wqe->sg_list[--j];

		qib_put_mr(sge->mr);
	}
bail_inval:
	ret = -EINVAL;
bail:
	if (!ret && !wr->next) {
		struct sdma_engine *sde;

		sde = qp_to_sdma_engine(qp, sc5);
		if (sde && !sdma_empty(sde)) {
			qib_schedule_send(qp);
			*scheduled = 1;
		}
	}
	spin_unlock_irqrestore(&qp->s_lock, flags);
	return ret;
}

/**
 * qib_post_send - post a send on a QP
 * @ibqp: the QP to post the send on
 * @wr: the list of work requests to post
 * @bad_wr: the first bad WR is put here
 *
 * This may be called from interrupt context.
 */
static int qib_post_send(struct ib_qp *ibqp, struct ib_send_wr *wr,
			 struct ib_send_wr **bad_wr)
{
	struct qib_qp *qp = to_iqp(ibqp);
	int err = 0;
	int scheduled = 0;

	for (; wr; wr = wr->next) {
		err = qib_post_one_send(qp, wr, &scheduled);
		if (err) {
			*bad_wr = wr;
			goto bail;
		}
	}

	/* Try to do the send work in the caller's context. */
	if (!scheduled)
		qib_do_send(&qp->s_iowait.iowork);

bail:
	return err;
}

/**
 * qib_post_receive - post a receive on a QP
 * @ibqp: the QP to post the receive on
 * @wr: the WR to post
 * @bad_wr: the first bad WR is put here
 *
 * This may be called from interrupt context.
 */
static int qib_post_receive(struct ib_qp *ibqp, struct ib_recv_wr *wr,
			    struct ib_recv_wr **bad_wr)
{
	struct qib_qp *qp = to_iqp(ibqp);
	struct qib_rwq *wq = qp->r_rq.wq;
	unsigned long flags;
	int ret;

	/* Check that state is OK to post receive. */
	if (!(ib_qib_state_ops[qp->state] & QIB_POST_RECV_OK) || !wq) {
		*bad_wr = wr;
		ret = -EINVAL;
		goto bail;
	}

	for (; wr; wr = wr->next) {
		struct qib_rwqe *wqe;
		u32 next;
		int i;

		if ((unsigned) wr->num_sge > qp->r_rq.max_sge) {
			*bad_wr = wr;
			ret = -EINVAL;
			goto bail;
		}

		spin_lock_irqsave(&qp->r_rq.lock, flags);
		next = wq->head + 1;
		if (next >= qp->r_rq.size)
			next = 0;
		if (next == wq->tail) {
			spin_unlock_irqrestore(&qp->r_rq.lock, flags);
			*bad_wr = wr;
			ret = -ENOMEM;
			goto bail;
		}

		wqe = get_rwqe_ptr(&qp->r_rq, wq->head);
		wqe->wr_id = wr->wr_id;
		wqe->num_sge = wr->num_sge;
		for (i = 0; i < wr->num_sge; i++)
			wqe->sg_list[i] = wr->sg_list[i];
		/* Make sure queue entry is written before the head index. */
		smp_wmb();
		wq->head = next;
		spin_unlock_irqrestore(&qp->r_rq.lock, flags);
	}
	ret = 0;

bail:
	return ret;
}

/**
 * qib_qp_rcv - processing an incoming packet on a QP
 * @rcd: the context pointer
 * @hdr: the packet header
 * @rcv_flags: flags relevant to rcv processing
 * @data: the packet data
 * @tlen: the packet length
 * @qp: the QP the packet came on
 *
 * This is called from qib_ib_rcv() to process an incoming packet
 * for the given QP.
 * Called at interrupt level.
 */
static void qib_qp_rcv(struct qib_ctxtdata *rcd, struct qib_ib_header *hdr,
		       u32 rcv_flags, void *data, u32 tlen, struct qib_qp *qp)
{
	struct qib_ibport *ibp = &rcd->ppd->ibport_data;

	spin_lock(&qp->r_lock);

	/* Check for valid receive state. */
	if (!(ib_qib_state_ops[qp->state] & QIB_PROCESS_RECV_OK)) {
		ibp->n_pkt_drops++;
		goto unlock;
	}

	switch (qp->ibqp.qp_type) {
	case IB_QPT_SMI:
	case IB_QPT_GSI:
		if (!HFI_CAP_IS_KSET(ENABLE_SMA))
			break;
		/* FALLTHROUGH */
	case IB_QPT_UD:
		qib_ud_rcv(ibp, hdr, rcv_flags, data, tlen, qp);
		break;

	case IB_QPT_RC:
		qib_rc_rcv(rcd, hdr, rcv_flags, data, tlen, qp);
		break;

	case IB_QPT_UC:
		qib_uc_rcv(ibp, hdr, rcv_flags, data, tlen, qp);
		break;

	default:
		break;
	}

unlock:
	spin_unlock(&qp->r_lock);
}

/**
 * qib_ib_rcv - process an incoming packet
 * @packet: data packet information
 *
 * This is called to process an incoming packet at interrupt level.
 *
 * Tlen is the length of the header + data + CRC in bytes.
 */
void qib_ib_rcv(struct hfi_packet *packet)
{
	struct qib_ctxtdata *rcd = packet->rcd;
	struct qib_ib_header *hdr = packet->hdr;
	void *data = packet->ebuf;
	u32 tlen = packet->tlen;
	struct qib_pportdata *ppd = rcd->ppd;
	struct qib_ibport *ibp = &ppd->ibport_data;
	struct qib_other_headers *ohdr;
	struct qib_qp *qp;
	u32 qp_num;
	u32 rcv_flags = 0;
	int lnh;
	u8 opcode;
	u16 lid;

	/* 24 == LRH+BTH+CRC */
	if (unlikely(tlen < 24))
		goto drop;

	/* Check for a valid destination LID (see ch. 7.11.1). */
	lid = be16_to_cpu(hdr->lrh[1]);

	/* Check for GRH */
	lnh = be16_to_cpu(hdr->lrh[0]) & 3;
	if (lnh == QIB_LRH_BTH)
		ohdr = &hdr->u.oth;
	else if (lnh == QIB_LRH_GRH) {
		u32 vtf;

		ohdr = &hdr->u.l.oth;
		if (hdr->u.l.grh.next_hdr != IB_GRH_NEXT_HDR)
			goto drop;
		vtf = be32_to_cpu(hdr->u.l.grh.version_tclass_flow);
		if ((vtf >> IB_GRH_VERSION_SHIFT) != IB_GRH_VERSION)
			goto drop;
	} else
		goto drop;

	trace_input_ibhdr(rcd->dd, hdr);

	opcode = (be32_to_cpu(ohdr->bth[0]) >> 24) & 0x7f;
	inc_opstats(tlen, &rcd->opstats->stats[opcode]);

	/* Get the destination QP number. */
	qp_num = be32_to_cpu(ohdr->bth[1]) & QIB_QPN_MASK;
	if ((lid >= QIB_MULTICAST_LID_BASE) &&
	    (lid != QIB_PERMISSIVE_LID)) {
		struct qib_mcast *mcast;
		struct qib_mcast_qp *p;

		if (lnh != QIB_LRH_GRH)
			goto drop;
		mcast = qib_mcast_find(ibp, &hdr->u.l.grh.dgid);
		if (mcast == NULL)
			goto drop;
		ibp->n_multicast_rcv++;
		rcv_flags |= QIB_HAS_GRH;
		if (rhf_dc_info(packet->rhf))
			rcv_flags |= QIB_SC4_BIT;
		list_for_each_entry_rcu(p, &mcast->qp_list, list)
			qib_qp_rcv(rcd, hdr, rcv_flags, data, tlen, p->qp);
		/*
		 * Notify qib_multicast_detach() if it is waiting for us
		 * to finish.
		 */
		if (atomic_dec_return(&mcast->refcount) <= 1)
			wake_up(&mcast->wait);
	} else {
		if (rcd->lookaside_qp) {
			if (rcd->lookaside_qpn != qp_num) {
				if (atomic_dec_and_test(
					&rcd->lookaside_qp->refcount))
					wake_up(
					 &rcd->lookaside_qp->wait);
					rcd->lookaside_qp = NULL;
				}
		}
		if (!rcd->lookaside_qp) {
			qp = qib_lookup_qpn(ibp, qp_num);
			if (!qp)
				goto drop;
			rcd->lookaside_qp = qp;
			rcd->lookaside_qpn = qp_num;
		} else
			qp = rcd->lookaside_qp;
		ibp->n_unicast_rcv++;
		if (lnh == QIB_LRH_GRH)
			rcv_flags |= QIB_HAS_GRH;
		if (rhf_dc_info(packet->rhf))
			rcv_flags |= QIB_SC4_BIT;
		qib_qp_rcv(rcd, hdr, rcv_flags, data, tlen, qp);
	}
	return;

drop:
	ibp->n_pkt_drops++;
}

/*
 * This is called from a timer to check for QPs
 * which need kernel memory in order to send a packet.
 */
static void mem_timer(unsigned long data)
{
	struct qib_ibdev *dev = (struct qib_ibdev *) data;
	struct list_head *list = &dev->memwait;
	struct qib_qp *qp = NULL;
	struct iowait *wait;
	unsigned long flags;

	spin_lock_irqsave(&dev->pending_lock, flags);
	if (!list_empty(list)) {
		wait = list_first_entry(list, struct iowait, list);
		qp = container_of(wait, struct qib_qp, s_iowait);
		list_del_init(&qp->s_iowait.list);
		/* refcount held until actual wakeup */
		if (!list_empty(list))
			mod_timer(&dev->mem_timer, jiffies + 1);
	}
	spin_unlock_irqrestore(&dev->pending_lock, flags);

	if (qp)
		qib_qp_wakeup(qp, QIB_S_WAIT_KMEM);
}

void update_sge(struct qib_sge_state *ss, u32 length)
{
	struct qib_sge *sge = &ss->sge;

	sge->vaddr += length;
	sge->length -= length;
	sge->sge_length -= length;
	if (sge->sge_length == 0) {
		if (--ss->num_sge)
			*sge = *ss->sg_list++;
	} else if (sge->length == 0 && sge->mr->lkey) {
		if (++sge->n >= QIB_SEGSZ) {
			if (++sge->m >= sge->mr->mapsz)
				return;
			sge->n = 0;
		}
		sge->vaddr = sge->mr->map[sge->m]->segs[sge->n].vaddr;
		sge->length = sge->mr->map[sge->m]->segs[sge->n].length;
	}
}

static noinline struct verbs_txreq *__get_txreq(struct qib_ibdev *dev,
					   struct qib_qp *qp)
{
	struct verbs_txreq *tx;
	unsigned long flags;

	spin_lock_irqsave(&qp->s_lock, flags);
	spin_lock(&dev->pending_lock);

	if (!list_empty(&dev->txreq_free)) {
		struct list_head *l = dev->txreq_free.next;

		list_del(l);
		spin_unlock(&dev->pending_lock);
		spin_unlock_irqrestore(&qp->s_lock, flags);
		tx = list_entry(l, struct verbs_txreq, txreq.list);
		tx->qp = qp;
		atomic_inc(&qp->refcount);
	} else {
		if (ib_qib_state_ops[qp->state] & QIB_PROCESS_RECV_OK &&
		    list_empty(&qp->s_iowait.list)) {
			dev->n_txwait++;
			qp->s_flags |= QIB_S_WAIT_TX;
			list_add_tail(&qp->s_iowait.list, &dev->txwait);
			trace_hfi_qpsleep(qp, QIB_S_WAIT_TX);
			atomic_inc(&qp->refcount);
		}
		qp->s_flags &= ~QIB_S_BUSY;
		spin_unlock(&dev->pending_lock);
		spin_unlock_irqrestore(&qp->s_lock, flags);
		tx = ERR_PTR(-EBUSY);
	}
	return tx;
}

static inline struct verbs_txreq *get_txreq(struct qib_ibdev *dev,
					 struct qib_qp *qp)
{
	struct verbs_txreq *tx;
	unsigned long flags;

	spin_lock_irqsave(&dev->pending_lock, flags);
	/* assume the list non empty */
	if (likely(!list_empty(&dev->txreq_free))) {
		struct list_head *l = dev->txreq_free.next;

		list_del(l);
		spin_unlock_irqrestore(&dev->pending_lock, flags);
		tx = list_entry(l, struct verbs_txreq, txreq.list);
		tx->qp = qp;
		atomic_inc(&qp->refcount);
	} else {
		/* call slow path to get the extra lock */
		spin_unlock_irqrestore(&dev->pending_lock, flags);
		tx =  __get_txreq(dev, qp);
	}
	return tx;
}

void qib_put_txreq(struct verbs_txreq *tx)
{
	struct qib_ibdev *dev;
	struct qib_qp *qp;
	unsigned long flags;

	qp = tx->qp;
	dev = to_idev(qp->ibqp.device);

	if (atomic_dec_and_test(&qp->refcount))
		wake_up(&qp->wait);
	if (tx->mr) {
		qib_put_mr(tx->mr);
		tx->mr = NULL;
	}
	sdma_txclean(dd_from_dev(dev), &tx->txreq);

	spin_lock_irqsave(&dev->pending_lock, flags);

	/* Put struct back on free list */
	list_add(&tx->txreq.list, &dev->txreq_free);

	if (!list_empty(&dev->txwait)) {
		struct iowait *wait;

		/* Wake up first QP wanting a free struct */
		wait = list_first_entry(&dev->txwait, struct iowait, list);
		qp = container_of(wait, struct qib_qp, s_iowait);
		list_del_init(&qp->s_iowait.list);
		/* refcount held until actual wakeup */
		spin_unlock_irqrestore(&dev->pending_lock, flags);
		qib_qp_wakeup(qp, QIB_S_WAIT_TX);
	} else
		spin_unlock_irqrestore(&dev->pending_lock, flags);
}

/*
 * This is called with progress side lock held.
 */
/* New API */
static void verbs_sdma_complete(
	struct sdma_txreq *cookie,
	int status,
	int drained)
{
	struct verbs_txreq *tx =
		container_of(cookie, struct verbs_txreq, txreq);
	struct qib_qp *qp = tx->qp;

	spin_lock(&qp->s_lock);
	if (tx->wqe)
		qib_send_complete(qp, tx->wqe, IB_WC_SUCCESS);
	else if (qp->ibqp.qp_type == IB_QPT_RC) {
		struct qib_ib_header *hdr;
		struct qib_ibdev *dev = to_idev(qp->ibqp.device);

		hdr = &dev->pio_hdrs[tx->hdr_inx].phdr.hdr;
		qib_rc_send_complete(qp, hdr);
	}
	if (drained) {
		/*
		 * This happens when the send engine notes
		 * a QP in the error state and cannot
		 * do the flush work until that QP's
		 * sdma work has finished.
		 */
		if (qp->s_flags & QIB_S_WAIT_DMA) {
			qp->s_flags &= ~QIB_S_WAIT_DMA;
			qib_schedule_send(qp);
		}
	}
	spin_unlock(&qp->s_lock);

	qib_put_txreq(tx);
}

static int wait_kmem(struct qib_ibdev *dev, struct qib_qp *qp)
{
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&qp->s_lock, flags);
	if (ib_qib_state_ops[qp->state] & QIB_PROCESS_RECV_OK) {
		spin_lock(&dev->pending_lock);
		if (list_empty(&qp->s_iowait.list)) {
			if (list_empty(&dev->memwait))
				mod_timer(&dev->mem_timer, jiffies + 1);
			qp->s_flags |= QIB_S_WAIT_KMEM;
			list_add_tail(&qp->s_iowait.list, &dev->memwait);
			trace_hfi_qpsleep(qp, QIB_S_WAIT_KMEM);
			atomic_inc(&qp->refcount);
		}
		spin_unlock(&dev->pending_lock);
		qp->s_flags &= ~QIB_S_BUSY;
		ret = -EBUSY;
	}
	spin_unlock_irqrestore(&qp->s_lock, flags);

	return ret;
}

/*
 * This routine calls txadds for each sg entry.
 *
 * Add failures will revert the sge cursor
 */
static int build_verbs_ulp_payload(
	struct sdma_engine *sde,
	struct qib_sge_state *ss,
	u32 length,
	struct verbs_txreq *tx)
{
	struct qib_sge *sg_list = ss->sg_list;
	struct qib_sge sge = ss->sge;
	u8 num_sge = ss->num_sge;
	u32 len;
	int ret = 0;

	while (length) {
		len = ss->sge.length;
		if (len > length)
			len = length;
		if (len > ss->sge.sge_length)
			len = ss->sge.sge_length;
		BUG_ON(len == 0);
		ret = sdma_txadd_kvaddr(
			sde->dd,
			&tx->txreq,
			ss->sge.vaddr,
			len);
		if (ret)
			goto bail_txadd;
		update_sge(ss, len);
		length -= len;
	}
	return ret;
bail_txadd:
	/* unwind cursor */
	ss->sge = sge;
	ss->num_sge = num_sge;
	ss->sg_list = sg_list;
	return ret;
}

/*
 * Build the number of DMA descriptors needed to send length bytes of data.
 *
 * NOTE: DMA mapping is held in the tx until completed in the ring or
 *       the tx desc is freed without having been submitted to the ring
 *
 * This routine insures the following all the helper routine
 * calls succeed.
 */
/* New API */
static int build_verbs_tx_desc(
	struct sdma_engine *sde,
	struct qib_sge_state *ss,
	u32 length,
	struct verbs_txreq *tx,
	struct ahg_ib_header *ahdr,
	u64 pbc)
{
	struct qib_ibdev *dev = to_idev(tx->qp->ibqp.device);
	int ret = 0;
	struct qib_pio_header *phdr;
	u16 hdrbytes = tx->hdr_dwords << 2;


	phdr = &dev->pio_hdrs[tx->hdr_inx].phdr;
	if (!ahdr->ahgcount) {
		ret = sdma_txinit_ahg(
			&tx->txreq,
			ahdr->tx_flags,
			hdrbytes + length,
			ahdr->ahgidx,
			0,
			NULL,
			0,
			verbs_sdma_complete);
		if (ret)
			goto bail_txadd;
		phdr->pbc = cpu_to_le64(pbc);
		memcpy(&phdr->hdr, &ahdr->ibh, hdrbytes - sizeof(phdr->pbc));
		/* add the header */
		ret = sdma_txadd_daddr(
			sde->dd,
			&tx->txreq,
			dev->pio_hdrs_phys + tx->hdr_inx *
				sizeof(struct tx_pio_header),
			tx->hdr_dwords << 2);
		if (ret)
			goto bail_txadd;
	} else {
		struct qib_other_headers *sohdr = &ahdr->ibh.u.oth;
		struct qib_other_headers *dohdr = &phdr->hdr.u.oth;

		/* needed in rc_send_complete() */
		phdr->hdr.lrh[0] = ahdr->ibh.lrh[0];
		if ((be16_to_cpu(phdr->hdr.lrh[0]) & 3) == QIB_LRH_GRH) {
			sohdr = &ahdr->ibh.u.l.oth;
			dohdr = &phdr->hdr.u.l.oth;
		}
		/* opcode */
		dohdr->bth[0] = sohdr->bth[0];
		/* PSN/ACK  */
		dohdr->bth[2] = sohdr->bth[2];
		ret = sdma_txinit_ahg(
			&tx->txreq,
			ahdr->tx_flags,
			length,
			ahdr->ahgidx,
			ahdr->ahgcount,
			ahdr->ahgdesc,
			hdrbytes,
			verbs_sdma_complete);
		if (ret)
			goto bail_txadd;
	}

	/* add the ulp payload - if any.  ss can be NULL for acks */
	if (ss)
		ret = build_verbs_ulp_payload(sde, ss, length, tx);
bail_txadd:
	return ret;
}

int qib_verbs_send_dma(struct qib_qp *qp, struct ahg_ib_header *ahdr,
			      u32 hdrwords, struct qib_sge_state *ss, u32 len,
			      u32 plen, u32 dwords, u64 pbc)
{
	struct qib_ibdev *dev = to_idev(qp->ibqp.device);
	struct verbs_txreq *tx;
	struct sdma_txreq *stx;
	u64 pbc_flags = 0;
	struct sdma_engine *sde;
	u8 sc5 = qp->s_sc;
	int ret;

	if (!list_empty(&qp->s_iowait.tx_head)) {
		stx = list_first_entry(
			&qp->s_iowait.tx_head,
			struct sdma_txreq,
			list);
		list_del_init(&stx->list);
		tx = container_of(stx, struct verbs_txreq, txreq);
		ret = sdma_send_txreq(tx->sde, &qp->s_iowait, stx);
		if (unlikely(ret == -ECOMM))
			goto bail_ecomm;
		return ret;
	}

	tx = get_txreq(dev, qp);
	if (IS_ERR(tx))
		goto bail_tx;

	if (!qp->s_hdr->sde)
		tx->sde = sde = qp_to_sdma_engine(qp, sc5);
	else
		tx->sde = sde = qp->s_hdr->sde;

	if (likely(pbc == 0)) {
		u32 vl = sc_to_vlt(dd_from_ibdev(qp->ibqp.device), sc5);
		/* No vl15 here */
		/* set WFR_PBC_DC_INFO bit (aka SC[4]) in pbc_flags */
		pbc_flags |= (!!(sc5 & 0x10)) << WFR_PBC_DC_INFO_SHIFT;

		pbc = create_pbc(pbc_flags, qp->s_srate, vl, plen);
	}
	tx->wqe = qp->s_wqe;
	tx->mr = qp->s_rdma_mr;
	if (qp->s_rdma_mr)
		qp->s_rdma_mr = NULL;
	tx->hdr_dwords = hdrwords + 2;
	ret = build_verbs_tx_desc(sde, ss, len, tx, ahdr, pbc);
	if (unlikely(ret))
		goto bail_build;
	trace_output_ibhdr(dd_from_ibdev(qp->ibqp.device), &ahdr->ibh);
	ret =  sdma_send_txreq(sde, &qp->s_iowait, &tx->txreq);
	if (unlikely(ret == -ECOMM))
		goto bail_ecomm;
	return ret;
bail_ecomm:
	/* The current one got "sent" */
	return 0;
bail_build:
	/* kmalloc or mapping fail */
	qib_put_txreq(tx);
	return wait_kmem(dev, qp);
bail_tx:
	return PTR_ERR(tx);
}

/*
 * If we are now in the error state, return zero to flush the
 * send work request.
 */
static int no_bufs_available(struct qib_qp *qp, struct send_context *sc)
{
	struct hfi_devdata *dd = sc->dd;
	struct qib_ibdev *dev = &dd->verbs_dev;
	unsigned long flags;
	int ret = 0;

	/*
	 * Note that as soon as want_buffer() is called and
	 * possibly before it returns, sc_piobufavail()
	 * could be called. Therefore, put QP on the I/O wait list before
	 * enabling the PIO avail interrupt.
	 */
	spin_lock_irqsave(&qp->s_lock, flags);
	if (ib_qib_state_ops[qp->state] & QIB_PROCESS_RECV_OK) {
		spin_lock(&dev->pending_lock);
		if (list_empty(&qp->s_iowait.list)) {
			struct qib_ibdev *dev = &dd->verbs_dev;
			int was_empty;

			dev->n_piowait++;
			qp->s_flags |= QIB_S_WAIT_PIO;
			was_empty = list_empty(&sc->piowait);
			list_add_tail(&qp->s_iowait.list, &sc->piowait);
			trace_hfi_qpsleep(qp, QIB_S_WAIT_PIO);
			atomic_inc(&qp->refcount);
			/* counting: only call wantpiobuf_intr if first user */
			if (was_empty)
				dd->f_wantpiobuf_intr(sc, 1);
		}
		spin_unlock(&dev->pending_lock);
		qp->s_flags &= ~QIB_S_BUSY;
		ret = -EBUSY;
	}
	spin_unlock_irqrestore(&qp->s_lock, flags);
	return ret;
}

struct send_context *qp_to_send_context(struct qib_qp *qp, u8 sc5)
{
	struct hfi_devdata *dd = dd_from_ibdev(qp->ibqp.device);
	struct qib_pportdata *ppd = dd->pport + (qp->port_num - 1);
	u8 vl;

	vl = sc_to_vlt(dd, sc5);
	if (vl >= hfi_num_vls(ppd->vls_supported) && vl != 15)
		return NULL;
	return dd->vld[vl].sc;
}

int qib_verbs_send_pio(struct qib_qp *qp, struct ahg_ib_header *ahdr,
			      u32 hdrwords, struct qib_sge_state *ss, u32 len,
			      u32 plen, u32 dwords, u64 pbc)
{
	struct qib_ibport *ibp = to_iport(qp->ibqp.device, qp->port_num);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	u32 *hdr = (u32 *)&ahdr->ibh;
	u64 pbc_flags = 0;
	u32 sc5;
	unsigned long flags = 0;
	struct send_context *sc;
	struct pio_buf *pbuf;
	int wc_status = IB_WC_SUCCESS;

	/* vl15 special case taken care of in ud.c */
	sc5 = qp->s_sc;
	sc = qp_to_send_context(qp, sc5);

	if (!sc)
		return -EINVAL;
	if (likely(pbc == 0)) {
		u32 vl = sc_to_vlt(dd_from_ibdev(qp->ibqp.device), sc5);
		/* set WFR_PBC_DC_INFO bit (aka SC[4]) in pbc_flags */
		pbc_flags |= (!!(sc5 & 0x10)) << WFR_PBC_DC_INFO_SHIFT;
		pbc = create_pbc(pbc_flags, qp->s_srate, vl, plen);
	}
	pbuf = sc_buffer_alloc(sc, plen, NULL, 0);
	if (unlikely(pbuf == NULL)) {
		if (ppd->host_link_state != HLS_UP_ACTIVE) {
			/*
			 * If we have filled the PIO buffers to capacity and are
			 * not in an active state this request is not going to
			 * go out to so just complete it with an error or else a
			 * ULP or the core may be stuck waiting.
			 */
			hfi_cdbg(PIO,
				"alloc failed. state not active, completing");
			wc_status = IB_WC_GENERAL_ERR;
			goto pio_bail;
		} else {
			/*
			 * This is a normal occurance. The PIO buffs are full up
			 * but we are still happily sending, well we could be so
			 * lets continue to queue the request.
			 */
			hfi_cdbg(PIO, "alloc failed. state active, queuing");
			return no_bufs_available(qp, sc);
		}
	}

	if (len == 0) {
		pio_copy(ppd->dd, pbuf, pbc, hdr, hdrwords);
	} else {
		seg_pio_copy_start(pbuf, pbc, hdr, hdrwords*4);
		while (len) {
			void *addr = ss->sge.vaddr;
			u32 slen = ss->sge.length;

			if (slen > len)
				slen = len;
			update_sge(ss, slen);
			seg_pio_copy_mid(pbuf, addr, slen);
			len -= slen;
		}
		seg_pio_copy_end(pbuf);
	}

	trace_output_ibhdr(dd_from_ibdev(qp->ibqp.device), &ahdr->ibh);

	if (qp->s_rdma_mr) {
		qib_put_mr(qp->s_rdma_mr);
		qp->s_rdma_mr = NULL;
	}

pio_bail:
	if (qp->s_wqe) {
		spin_lock_irqsave(&qp->s_lock, flags);
		qib_send_complete(qp, qp->s_wqe, wc_status);
		spin_unlock_irqrestore(&qp->s_lock, flags);
	} else if (qp->ibqp.qp_type == IB_QPT_RC) {
		spin_lock_irqsave(&qp->s_lock, flags);
		qib_rc_send_complete(qp, &ahdr->ibh);
		spin_unlock_irqrestore(&qp->s_lock, flags);
	}
	return 0;
}
/*
 * egress_pkey_matches_entry - return 1 if the pkey matches ent (ent
 * being an entry from the ingress partition key table), return 0
 * otherwise. Use the matching criteria for egress partition keys
 * specified in the STLv1 spec., section 9.1l.7.
 */
static inline int egress_pkey_matches_entry(u16 pkey, u16 ent)
{
	u16 mkey = pkey & PKEY_LOW_15_MASK;
	u16 ment = ent & PKEY_LOW_15_MASK;

	if (mkey == ment) {
		/*
		 * If pkey[15] is set (full partition member),
		 * is bit 15 in the corresponding table element
		 * clear (limited member)?
		 */
		if (pkey & PKEY_MEMBER_MASK)
			return !!(ent & PKEY_MEMBER_MASK);
		return 1;
	}
	return 0;
}

/*
 * egress_pkey_check - return 0 if hdr's pkey matches according to the
 * criterea in the STLv1 spec., section 9.11.7.
 */
static inline int egress_pkey_check(struct qib_pportdata *ppd,
				    struct qib_ib_header *hdr,
				    struct qib_qp *qp)
{
	struct qib_other_headers *ohdr;
	struct hfi_devdata *dd;
	int i = 0;
	u16 pkey;
	u8 lnh, sc5 = qp->s_sc;

	if (!(ppd->part_enforce & HFI_PART_ENFORCE_OUT))
		return 0;

	/* locate the pkey within the headers */
	lnh = be16_to_cpu(hdr->lrh[0]) & 3;
	if (lnh == QIB_LRH_GRH)
		ohdr = &hdr->u.l.oth;
	else
		ohdr = &hdr->u.oth;

	pkey = (u16)be32_to_cpu(ohdr->bth[0]);

	/* If SC15, pkey[0:14] must be 0x7fff */
	if ((sc5 == 0xf) && ((pkey & PKEY_LOW_15_MASK) != PKEY_LOW_15_MASK))
		goto bad;


	/* Is the pkey = 0x0, or 0x8000? */
	if ((pkey & PKEY_LOW_15_MASK) == 0)
		goto bad;

	/* The most likely matching pkey has index qp->s_pkey_index */
	if (!egress_pkey_matches_entry(pkey, ppd->pkeys[qp->s_pkey_index])) {
		/* no match - try the entire table */
		for (; i < WFR_MAX_PKEY_VALUES; i++) {
			if (egress_pkey_matches_entry(pkey, ppd->pkeys[i]))
				break;
		}
	}

	if (i < WFR_MAX_PKEY_VALUES)
		return 0;
bad:
	incr_cntr64(&ppd->port_xmit_constraint_errors);
	dd = ppd->dd;
	if (!(dd->err_info_xmit_constraint.status & STL_EI_STATUS_SMASK)) {
		u16 slid = be16_to_cpu(hdr->lrh[3]);
		dd->err_info_xmit_constraint.status |= STL_EI_STATUS_SMASK;
		dd->err_info_xmit_constraint.slid = slid;
		dd->err_info_xmit_constraint.pkey = pkey;
	}
	return 1;
}

/**
 * qib_verbs_send - send a packet
 * @qp: the QP to send on
 * @ahdr: the packet header
 * @hdrwords: the number of 32-bit words in the header
 * @ss: the SGE to send
 * @len: the length of the packet in bytes
 *
 * Return zero if packet is sent or queued OK.
 * Return non-zero and clear qp->s_flags QIB_S_BUSY otherwise.
 */
int qib_verbs_send(struct qib_qp *qp, struct ahg_ib_header *ahdr,
		   u32 hdrwords, struct qib_sge_state *ss, u32 len)
{
	struct hfi_devdata *dd = dd_from_ibdev(qp->ibqp.device);
	u32 plen;
	int ret;
	int pio = 0;
	unsigned long flags = 0;
	u32 dwords = (len + 3) >> 2;

	/*
	 * VL15 packets (IB_QPT_SMI) will always use PIO, so we
	 * can defer SDMA restart until link goes ACTIVE without
	 * worrying about just how we got there.
	 */
	if (qp->ibqp.qp_type == IB_QPT_SMI
		|| !(dd->flags & HFI_HAS_SEND_DMA))
		pio = 1;

	ret = egress_pkey_check(dd->pport, &ahdr->ibh, qp);
	if (unlikely(ret)) {
		/*
		 * The value we are returning here does not get propagated to
		 * the verbs caller. Thus we need to complete the request with
		 * error otherwise the caller could be sitting waiting on the
		 * completeion event. Only do this for PIO. SDMA has its own
		 * mechansim for handling the errors. So for SDMA we can just
		 * return.
		 */
		if (pio) {
			hfi_cdbg(PIO, "%s() Failed. Completing with err",
				 __func__);
			spin_lock_irqsave(&qp->s_lock, flags);
			qib_send_complete(qp, qp->s_wqe, IB_WC_GENERAL_ERR);
			spin_unlock_irqrestore(&qp->s_lock, flags);
		}
		return -EINVAL;
	}

	/*
	 * Calculate the send buffer trigger address.
	 * The +2 counts for the pbc control qword
	 */
	plen = hdrwords + dwords + 2;

	if (pio) {
		ret = dd->process_pio_send(
			qp, ahdr, hdrwords, ss, len, plen, dwords, 0);
	} else {
#ifdef JAG_SDMA_VERBOSITY
		dd_dev_err(dd, "JAG SDMA %s:%d %s()\n",
			slashstrip(__FILE__), __LINE__, __func__);
		dd_dev_err(dd, "SDMA hdrwords = %u, len = %u\n", hdrwords, len);
#endif
		ret = dd->process_dma_send(
			qp, ahdr, hdrwords, ss, len, plen, dwords, 0);
	}

	return ret;
}

int qib_snapshot_counters(struct qib_pportdata *ppd, u64 *swords,
			  u64 *rwords, u64 *spkts, u64 *rpkts,
			  u64 *xmit_wait)
{
	int ret;
	struct hfi_devdata *dd = ppd->dd;

	if (!(dd->flags & HFI_PRESENT)) {
		/* no hardware, freeze, etc. */
		ret = -EINVAL;
		goto bail;
	}
	*swords = dd->f_portcntr(ppd, QIBPORTCNTR_WORDSEND);
	*rwords = dd->f_portcntr(ppd, QIBPORTCNTR_WORDRCV);
	*spkts = dd->f_portcntr(ppd, QIBPORTCNTR_PKTSEND);
	*rpkts = dd->f_portcntr(ppd, QIBPORTCNTR_PKTRCV);
	*xmit_wait = dd->f_portcntr(ppd, QIBPORTCNTR_SENDSTALL);

	ret = 0;

bail:
	return ret;
}

/**
 * qib_get_counters - get various chip counters
 * @dd: the qlogic_ib device
 * @cntrs: counters are placed here
 *
 * Return the counters needed by recv_pma_get_portcounters().
 */
int qib_get_counters(struct qib_pportdata *ppd,
		     struct qib_verbs_counters *cntrs)
{
	int ret;

	if (!(ppd->dd->flags & HFI_PRESENT)) {
		/* no hardware, freeze, etc. */
		ret = -EINVAL;
		goto bail;
	}
	cntrs->symbol_error_counter =
		ppd->dd->f_portcntr(ppd, QIBPORTCNTR_IBSYMBOLERR);
	cntrs->link_error_recovery_counter =
		ppd->dd->f_portcntr(ppd, QIBPORTCNTR_IBLINKERRRECOV);
	/*
	 * The link downed counter counts when the other side downs the
	 * connection.  We add in the number of times we downed the link
	 * due to local link integrity errors to compensate.
	 */
	cntrs->link_downed_counter =
		ppd->dd->f_portcntr(ppd, QIBPORTCNTR_IBLINKDOWN);
	cntrs->port_rcv_errors =
		ppd->dd->f_portcntr(ppd, QIBPORTCNTR_RXDROPPKT) +
		ppd->dd->f_portcntr(ppd, QIBPORTCNTR_RCVOVFL) +
		ppd->dd->f_portcntr(ppd, QIBPORTCNTR_ERR_RLEN) +
		ppd->dd->f_portcntr(ppd, QIBPORTCNTR_INVALIDRLEN) +
		ppd->dd->f_portcntr(ppd, QIBPORTCNTR_ERRLINK) +
		ppd->dd->f_portcntr(ppd, QIBPORTCNTR_ERRICRC) +
		ppd->dd->f_portcntr(ppd, QIBPORTCNTR_ERRVCRC) +
		ppd->dd->f_portcntr(ppd, QIBPORTCNTR_ERRLPCRC) +
		ppd->dd->f_portcntr(ppd, QIBPORTCNTR_BADFORMAT);
	cntrs->port_rcv_errors +=
		ppd->dd->f_portcntr(ppd, QIBPORTCNTR_RXLOCALPHYERR);
	cntrs->port_rcv_errors +=
		ppd->dd->f_portcntr(ppd, QIBPORTCNTR_RXVLERR);
	cntrs->port_rcv_remphys_errors =
		ppd->dd->f_portcntr(ppd, QIBPORTCNTR_RCVEBP);
	cntrs->port_xmit_discards =
		ppd->dd->f_portcntr(ppd, QIBPORTCNTR_UNSUPVL);
	cntrs->port_xmit_data = ppd->dd->f_portcntr(ppd,
			QIBPORTCNTR_WORDSEND);
	cntrs->port_rcv_data = ppd->dd->f_portcntr(ppd,
			QIBPORTCNTR_WORDRCV);
	cntrs->port_xmit_packets = ppd->dd->f_portcntr(ppd,
			QIBPORTCNTR_PKTSEND);
	cntrs->port_rcv_packets = ppd->dd->f_portcntr(ppd,
			QIBPORTCNTR_PKTRCV);
	cntrs->local_link_integrity_errors =
		ppd->dd->f_portcntr(ppd, QIBPORTCNTR_LLI);
	cntrs->excessive_buffer_overrun_errors =
		ppd->dd->f_portcntr(ppd, QIBPORTCNTR_EXCESSBUFOVFL);
	cntrs->vl15_dropped =
		ppd->dd->f_portcntr(ppd, QIBPORTCNTR_VL15PKTDROP);

	ret = 0;

bail:
	return ret;
}

static int qib_query_device(struct ib_device *ibdev,
			    struct ib_device_attr *props)
{
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	struct qib_ibdev *dev = to_idev(ibdev);

	memset(props, 0, sizeof(*props));

	props->device_cap_flags = IB_DEVICE_BAD_PKEY_CNTR |
		IB_DEVICE_BAD_QKEY_CNTR | IB_DEVICE_SHUTDOWN_PORT |
		IB_DEVICE_SYS_IMAGE_GUID | IB_DEVICE_RC_RNR_NAK_GEN |
		IB_DEVICE_PORT_ACTIVE_EVENT | IB_DEVICE_SRQ_RESIZE |
		IB_DEVICE_JUMBO_MAD_SUPPORT;

	props->page_size_cap = PAGE_SIZE;
	props->vendor_id =
		dd->oui1 << 16 | dd->oui2 << 8 | dd->oui3;
	props->vendor_part_id = dd->pcidev->device;
	props->hw_ver = dd->minrev;
	props->sys_image_guid = ib_qib_sys_image_guid;
	props->max_mr_size = ~0ULL;
	props->max_qp = ib_qib_max_qps;
	props->max_qp_wr = ib_qib_max_qp_wrs;
	props->max_sge = ib_qib_max_sges;
	props->max_cq = ib_qib_max_cqs;
	props->max_ah = ib_qib_max_ahs;
	props->max_cqe = ib_qib_max_cqes;
	props->max_mr = dev->lk_table.max;
	props->max_fmr = dev->lk_table.max;
	props->max_map_per_fmr = 32767;
	props->max_pd = ib_qib_max_pds;
	props->max_qp_rd_atom = QIB_MAX_RDMA_ATOMIC;
	props->max_qp_init_rd_atom = 255;
	/* props->max_res_rd_atom */
	props->max_srq = ib_qib_max_srqs;
	props->max_srq_wr = ib_qib_max_srq_wrs;
	props->max_srq_sge = ib_qib_max_srq_sges;
	/* props->local_ca_ack_delay */
	props->atomic_cap = IB_ATOMIC_GLOB;
	props->max_pkeys = qib_get_npkeys(dd);
	props->max_mcast_grp = ib_qib_max_mcast_grps;
	props->max_mcast_qp_attach = ib_qib_max_mcast_qp_attached;
	props->max_total_mcast_qp_attach = props->max_mcast_qp_attach *
		props->max_mcast_grp;

	return 0;
}

static inline u16 stl_speed_to_ib(u16 in)
{
	u16 out = 0;

	if (in & OPA_LINK_SPEED_25G)
		out |= IB_SPEED_EDR;
	if (in & OPA_LINK_SPEED_12_5G)
		out |= IB_SPEED_QDR;

	BUG_ON(!out);
	return out;
}

/*
 * Convert a single STL link width (no multiple flags) to an IB value.
 * A zero STL link width means link down, which means the IB width value
 * is a don't care.
 */
static inline u16 stl_width_to_ib(u16 in)
{
	switch (in) {
	case OPA_LINK_WIDTH_1X:
	/* map 2x and 3x to 1x as they don't exist in IB */
	case OPA_LINK_WIDTH_2X:
	case OPA_LINK_WIDTH_3X:
		return IB_WIDTH_1X;
	default: /* link down or unknown, return our largest width */
	case OPA_LINK_WIDTH_4X:
		return IB_WIDTH_4X;
	}
}

static int qib_query_port(struct ib_device *ibdev, u8 port,
			  struct ib_port_attr *props)
{
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	u16 lid = ppd->lid;

	memset(props, 0, sizeof(*props));
	props->lid = lid ? lid : be16_to_cpu(IB_LID_PERMISSIVE);
	props->lmc = ppd->lmc;
	props->sm_lid = ibp->sm_lid;
	props->sm_sl = ibp->sm_sl;
	props->state = dd->f_iblink_state(ppd);
	props->phys_state = dd->f_ibphys_portstate(ppd);
	props->port_cap_flags = ibp->port_cap_flags;
	props->gid_tbl_len = QIB_GUIDS_PER_PORT;
	props->max_msg_sz = 0x80000000;
	props->pkey_tbl_len = qib_get_npkeys(dd);
	props->bad_pkey_cntr = ibp->pkey_violations;
	props->qkey_viol_cntr = ibp->qkey_violations;
	props->active_width = (u8)stl_width_to_ib(ppd->link_width_active);
	/* see rate_show() in ib core/sysfs.c */
	props->active_speed = (u8)stl_speed_to_ib(ppd->link_speed_active);
	props->max_vl_num = hfi_num_vls(ppd->vls_supported);
	props->init_type_reply = 0;

	/* FIXME (Mitko): I am not sure whether this is the correct thing
	 * for STL1 but for now IB can't really handle IB-unsupported
	 * MTUs. The alternative would be to add the STL MTUs to the
	 * core IB headers. */
	props->max_mtu = mtu_to_enum((!valid_ib_mtu(max_mtu) ?
				      4096 : max_mtu), IB_MTU_4096);
	props->active_mtu = !valid_ib_mtu(ppd->ibmtu) ? props->max_mtu :
		mtu_to_enum(ppd->ibmtu, IB_MTU_2048);
	props->subnet_timeout = ibp->subnet_timeout;

	return 0;
}

static int qib_modify_device(struct ib_device *device,
			     int device_modify_mask,
			     struct ib_device_modify *device_modify)
{
	struct hfi_devdata *dd = dd_from_ibdev(device);
	unsigned i;
	int ret;

	if (device_modify_mask & ~(IB_DEVICE_MODIFY_SYS_IMAGE_GUID |
				   IB_DEVICE_MODIFY_NODE_DESC)) {
		ret = -EOPNOTSUPP;
		goto bail;
	}

	if (device_modify_mask & IB_DEVICE_MODIFY_NODE_DESC) {
		memcpy(device->node_desc, device_modify->node_desc, 64);
		for (i = 0; i < dd->num_pports; i++) {
			struct qib_ibport *ibp = &dd->pport[i].ibport_data;

			qib_node_desc_chg(ibp);
		}
	}

	if (device_modify_mask & IB_DEVICE_MODIFY_SYS_IMAGE_GUID) {
		ib_qib_sys_image_guid =
			cpu_to_be64(device_modify->sys_image_guid);
		for (i = 0; i < dd->num_pports; i++) {
			struct qib_ibport *ibp = &dd->pport[i].ibport_data;

			qib_sys_guid_chg(ibp);
		}
	}

	ret = 0;

bail:
	return ret;
}

static int qib_modify_port(struct ib_device *ibdev, u8 port,
			   int port_modify_mask, struct ib_port_modify *props)
{
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	int ret = 0;

	ibp->port_cap_flags |= props->set_port_cap_mask;
	ibp->port_cap_flags &= ~props->clr_port_cap_mask;
	if (props->set_port_cap_mask || props->clr_port_cap_mask)
		qib_cap_mask_chg(ibp);
	if (port_modify_mask & IB_PORT_SHUTDOWN) {
		set_link_down_reason(ppd, OPA_LINKDOWN_REASON_UNKNOWN, 0,
		  OPA_LINKDOWN_REASON_UNKNOWN);
		ret = set_link_state(ppd, HLS_DN_DOWNDEF);
	}
	if (port_modify_mask & IB_PORT_RESET_QKEY_CNTR)
		ibp->qkey_violations = 0;
	return ret;
}

static int qib_query_gid(struct ib_device *ibdev, u8 port,
			 int index, union ib_gid *gid)
{
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	int ret = 0;

	if (!port || port > dd->num_pports)
		ret = -EINVAL;
	else {
		struct qib_ibport *ibp = to_iport(ibdev, port);
		struct qib_pportdata *ppd = ppd_from_ibp(ibp);

		gid->global.subnet_prefix = ibp->gid_prefix;
		if (index == 0)
			gid->global.interface_id = ppd->guid;
		else if (index < QIB_GUIDS_PER_PORT)
			gid->global.interface_id = ibp->guids[index - 1];
		else
			ret = -EINVAL;
	}

	return ret;
}

static struct ib_pd *qib_alloc_pd(struct ib_device *ibdev,
				  struct ib_ucontext *context,
				  struct ib_udata *udata)
{
	struct qib_ibdev *dev = to_idev(ibdev);
	struct qib_pd *pd;
	struct ib_pd *ret;

	/*
	 * This is actually totally arbitrary.  Some correctness tests
	 * assume there's a maximum number of PDs that can be allocated.
	 * We don't actually have this limit, but we fail the test if
	 * we allow allocations of more than we report for this value.
	 */

	pd = kmalloc(sizeof(*pd), GFP_KERNEL);
	if (!pd) {
		ret = ERR_PTR(-ENOMEM);
		goto bail;
	}

	spin_lock(&dev->n_pds_lock);
	if (dev->n_pds_allocated == ib_qib_max_pds) {
		spin_unlock(&dev->n_pds_lock);
		kfree(pd);
		ret = ERR_PTR(-ENOMEM);
		goto bail;
	}

	dev->n_pds_allocated++;
	spin_unlock(&dev->n_pds_lock);

	/* ib_alloc_pd() will initialize pd->ibpd. */
	pd->user = udata != NULL;

	ret = &pd->ibpd;

bail:
	return ret;
}

static int qib_dealloc_pd(struct ib_pd *ibpd)
{
	struct qib_pd *pd = to_ipd(ibpd);
	struct qib_ibdev *dev = to_idev(ibpd->device);

	spin_lock(&dev->n_pds_lock);
	dev->n_pds_allocated--;
	spin_unlock(&dev->n_pds_lock);

	kfree(pd);

	return 0;
}

/*
 * convert ah port,sl to sc
 */
u8 ah_to_sc(struct ib_device *ibdev, struct ib_ah_attr *ah)
{
	struct qib_ibport *ibp = to_iport(ibdev, ah->port_num);

	return ibp->sl_to_sc[ah->sl];
}

int qib_check_ah(struct ib_device *ibdev, struct ib_ah_attr *ah_attr)
{
	struct qib_ibport *ibp;
	struct qib_pportdata *ppd;
	struct hfi_devdata *dd;
	u8 sc5;

	/* A multicast address requires a GRH (see ch. 8.4.1). */
	if (ah_attr->dlid >= QIB_MULTICAST_LID_BASE &&
	    ah_attr->dlid != QIB_PERMISSIVE_LID &&
	    !(ah_attr->ah_flags & IB_AH_GRH))
		goto bail;
	if ((ah_attr->ah_flags & IB_AH_GRH) &&
	    ah_attr->grh.sgid_index >= QIB_GUIDS_PER_PORT)
		goto bail;
	if (ah_attr->dlid == 0)
		goto bail;
	if (ah_attr->port_num < 1 ||
	    ah_attr->port_num > ibdev->phys_port_cnt)
		goto bail;
	if (ah_attr->static_rate != IB_RATE_PORT_CURRENT &&
	    ib_rate_to_mbps(ah_attr->static_rate) < 0)
		goto bail;
	if (ah_attr->sl >= OPA_MAX_SLS)
		goto bail;
	/* test the mapping for validity */
	ibp = to_iport(ibdev, ah_attr->port_num);
	ppd = ppd_from_ibp(ibp);
	sc5 = ibp->sl_to_sc[ah_attr->sl];
	dd = dd_from_ppd(ppd);
	if (sc_to_vlt(dd, sc5) > num_vls)
		goto bail;
	return 0;
bail:
	return -EINVAL;
}

/**
 * qib_create_ah - create an address handle
 * @pd: the protection domain
 * @ah_attr: the attributes of the AH
 *
 * This may be called from interrupt context.
 */
static struct ib_ah *qib_create_ah(struct ib_pd *pd,
				   struct ib_ah_attr *ah_attr)
{
	struct qib_ah *ah;
	struct ib_ah *ret;
	struct qib_ibdev *dev = to_idev(pd->device);
	unsigned long flags;

	if (qib_check_ah(pd->device, ah_attr)) {
		ret = ERR_PTR(-EINVAL);
		goto bail;
	}

	ah = kmalloc(sizeof(*ah), GFP_ATOMIC);
	if (!ah) {
		ret = ERR_PTR(-ENOMEM);
		goto bail;
	}

	spin_lock_irqsave(&dev->n_ahs_lock, flags);
	if (dev->n_ahs_allocated == ib_qib_max_ahs) {
		spin_unlock_irqrestore(&dev->n_ahs_lock, flags);
		kfree(ah);
		ret = ERR_PTR(-ENOMEM);
		goto bail;
	}

	dev->n_ahs_allocated++;
	spin_unlock_irqrestore(&dev->n_ahs_lock, flags);

	/* ib_create_ah() will initialize ah->ibah. */
	ah->attr = *ah_attr;
	atomic_set(&ah->refcount, 0);

	ret = &ah->ibah;

bail:
	return ret;
}

struct ib_ah *qib_create_qp0_ah(struct qib_ibport *ibp, u16 dlid)
{
	struct ib_ah_attr attr;
	struct ib_ah *ah = ERR_PTR(-EINVAL);
	struct qib_qp *qp0;

	memset(&attr, 0, sizeof(attr));
	attr.dlid = dlid;
	attr.port_num = ppd_from_ibp(ibp)->port;
	rcu_read_lock();
	qp0 = rcu_dereference(ibp->qp0);
	if (qp0)
		ah = ib_create_ah(qp0->ibqp.pd, &attr);
	rcu_read_unlock();
	return ah;
}

/**
 * qib_destroy_ah - destroy an address handle
 * @ibah: the AH to destroy
 *
 * This may be called from interrupt context.
 */
static int qib_destroy_ah(struct ib_ah *ibah)
{
	struct qib_ibdev *dev = to_idev(ibah->device);
	struct qib_ah *ah = to_iah(ibah);
	unsigned long flags;

	if (atomic_read(&ah->refcount) != 0)
		return -EBUSY;

	spin_lock_irqsave(&dev->n_ahs_lock, flags);
	dev->n_ahs_allocated--;
	spin_unlock_irqrestore(&dev->n_ahs_lock, flags);

	kfree(ah);

	return 0;
}

static int qib_modify_ah(struct ib_ah *ibah, struct ib_ah_attr *ah_attr)
{
	struct qib_ah *ah = to_iah(ibah);

	if (qib_check_ah(ibah->device, ah_attr))
		return -EINVAL;

	ah->attr = *ah_attr;

	return 0;
}

static int qib_query_ah(struct ib_ah *ibah, struct ib_ah_attr *ah_attr)
{
	struct qib_ah *ah = to_iah(ibah);

	*ah_attr = ah->attr;

	return 0;
}

/**
 * qib_get_npkeys - return the size of the PKEY table for context 0
 * @dd: the qlogic_ib device
 */
unsigned qib_get_npkeys(struct hfi_devdata *dd)
{
	return ARRAY_SIZE(dd->pport[0].pkeys);
}

/*
 * Return the indexed PKEY from the port PKEY table.
 */
unsigned qib_get_pkey(struct qib_ibport *ibp, unsigned index)
{
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	unsigned ret;

	if (index >= ARRAY_SIZE(ppd->pkeys))
		ret = 0;
	else
		ret = ppd->pkeys[index];

	return ret;
}

static int qib_query_pkey(struct ib_device *ibdev, u8 port, u16 index,
			  u16 *pkey)
{
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	int ret;

	if (index >= qib_get_npkeys(dd)) {
		ret = -EINVAL;
		goto bail;
	}

	*pkey = qib_get_pkey(to_iport(ibdev, port), index);
	ret = 0;

bail:
	return ret;
}

/**
 * qib_alloc_ucontext - allocate a ucontest
 * @ibdev: the infiniband device
 * @udata: not used by the QLogic_IB driver
 */

static struct ib_ucontext *qib_alloc_ucontext(struct ib_device *ibdev,
					      struct ib_udata *udata)
{
	struct qib_ucontext *context;
	struct ib_ucontext *ret;

	context = kmalloc(sizeof(*context), GFP_KERNEL);
	if (!context) {
		ret = ERR_PTR(-ENOMEM);
		goto bail;
	}

	ret = &context->ibucontext;

bail:
	return ret;
}

static int qib_dealloc_ucontext(struct ib_ucontext *context)
{
	kfree(to_iucontext(context));
	return 0;
}

static void init_ibport(struct qib_pportdata *ppd)
{
	struct qib_verbs_counters cntrs;
	struct qib_ibport *ibp = &ppd->ibport_data;
	size_t sz = ARRAY_SIZE(ibp->sl_to_sc);
	int i;

	for (i = 0; i < sz; i++) {
		ibp->sl_to_sc[i] = i;
		ibp->sc_to_sl[i] = i;
	}

	spin_lock_init(&ibp->lock);
	/* Set the prefix to the default value (see ch. 4.1.1) */
	ibp->gid_prefix = IB_DEFAULT_GID_PREFIX;
	ibp->sm_lid = be16_to_cpu(0);
	/* Below should only set bits defined in STL PortInfo.CapabilityMask */
	ibp->port_cap_flags = IB_PORT_AUTO_MIGR_SUP |
		IB_PORT_CAP_MASK_NOTICE_SUP;
	ibp->pma_counter_select[0] = IB_PMA_PORT_XMIT_DATA;
	ibp->pma_counter_select[1] = IB_PMA_PORT_RCV_DATA;
	ibp->pma_counter_select[2] = IB_PMA_PORT_XMIT_PKTS;
	ibp->pma_counter_select[3] = IB_PMA_PORT_RCV_PKTS;
	ibp->pma_counter_select[4] = IB_PMA_PORT_XMIT_WAIT;

	/* Snapshot current HW counters to "clear" them. */
	qib_get_counters(ppd, &cntrs);
	ibp->z_symbol_error_counter = cntrs.symbol_error_counter;
	ibp->z_link_error_recovery_counter =
		cntrs.link_error_recovery_counter;
	ibp->z_link_downed_counter = cntrs.link_downed_counter;
	ibp->z_port_rcv_errors = cntrs.port_rcv_errors;
	ibp->z_port_rcv_remphys_errors = cntrs.port_rcv_remphys_errors;
	ibp->z_port_xmit_discards = cntrs.port_xmit_discards;
	ibp->z_port_xmit_data = cntrs.port_xmit_data;
	ibp->z_port_rcv_data = cntrs.port_rcv_data;
	ibp->z_port_xmit_packets = cntrs.port_xmit_packets;
	ibp->z_port_rcv_packets = cntrs.port_rcv_packets;
	ibp->z_local_link_integrity_errors =
		cntrs.local_link_integrity_errors;
	ibp->z_excessive_buffer_overrun_errors =
		cntrs.excessive_buffer_overrun_errors;
	ibp->z_vl15_dropped = cntrs.vl15_dropped;
	RCU_INIT_POINTER(ibp->qp0, NULL);
	RCU_INIT_POINTER(ibp->qp1, NULL);
}

/**
 * qib_register_ib_device - register our device with the infiniband core
 * @dd: the device data structure
 * Return the allocated qib_ibdev pointer or NULL on error.
 */
int qib_register_ib_device(struct hfi_devdata *dd)
{
	struct qib_ibdev *dev = &dd->verbs_dev;
	struct ib_device *ibdev = &dev->ibdev;
	struct qib_pportdata *ppd = dd->pport;
	unsigned i, lk_tab_size;
	int ret;
	size_t lcpysz = IB_DEVICE_NAME_MAX;
	u16 descq_cnt;

	ret = qib_qp_init(dev);
	if (ret)
		goto err_qp_init;


	for (i = 0; i < dd->num_pports; i++)
		init_ibport(ppd + i);

	/* Only need to initialize non-zero fields. */
	spin_lock_init(&dev->n_pds_lock);
	spin_lock_init(&dev->n_ahs_lock);
	spin_lock_init(&dev->n_cqs_lock);
	spin_lock_init(&dev->n_qps_lock);
	spin_lock_init(&dev->n_srqs_lock);
	spin_lock_init(&dev->n_mcast_grps_lock);
	init_timer(&dev->mem_timer);
	dev->mem_timer.function = mem_timer;
	dev->mem_timer.data = (unsigned long) dev;

	/*
	 * The top ib_qib_lkey_table_size bits are used to index the
	 * table.  The lower 8 bits can be owned by the user (copied from
	 * the LKEY).  The remaining bits act as a generation number or tag.
	 */
	spin_lock_init(&dev->lk_table.lock);
	dev->lk_table.max = 1 << ib_qib_lkey_table_size;
	lk_tab_size = dev->lk_table.max * sizeof(*dev->lk_table.table);
	dev->lk_table.table = (struct qib_mregion __rcu **)
		__get_free_pages(GFP_KERNEL, get_order(lk_tab_size));
	if (dev->lk_table.table == NULL) {
		ret = -ENOMEM;
		goto err_lk;
	}
	RCU_INIT_POINTER(dev->dma_mr, NULL);
	for (i = 0; i < dev->lk_table.max; i++)
		RCU_INIT_POINTER(dev->lk_table.table[i], NULL);
	INIT_LIST_HEAD(&dev->pending_mmaps);
	spin_lock_init(&dev->pending_lock);
	dev->mmap_offset = PAGE_SIZE;
	spin_lock_init(&dev->mmap_offset_lock);
	INIT_LIST_HEAD(&dev->txwait);
	INIT_LIST_HEAD(&dev->memwait);
	INIT_LIST_HEAD(&dev->txreq_free);

	/* FIXME - come up with a better scheme
	 * - this one doesn't scale for multiple sdma engines
	 */
	descq_cnt = sdma_get_descq_cnt();
	/*
	 * AHG mode copy requires header be on cache line
	 */
	dev->pio_hdr_bytes = descq_cnt * sizeof(struct tx_pio_header);
	if (descq_cnt) {
		dev->pio_hdrs = dma_zalloc_coherent(&dd->pcidev->dev,
						dev->pio_hdr_bytes,
						&dev->pio_hdrs_phys,
						GFP_KERNEL);
		if (!dev->pio_hdrs) {
			ret = -ENOMEM;
			goto err_hdrs;
		}
	}

	for (i = 0; i < descq_cnt; i++) {
		struct verbs_txreq *tx;

		tx = kzalloc(sizeof(*tx), GFP_KERNEL);
		if (!tx) {
			ret = -ENOMEM;
			goto err_tx;
		}
		tx->hdr_inx = i;
		list_add(&tx->txreq.list, &dev->txreq_free);
	}

	/*
	 * The system image GUID is supposed to be the same for all
	 * IB HCAs in a single system but since there can be other
	 * device types in the system, we can't be sure this is unique.
	 */
	if (!ib_qib_sys_image_guid)
		ib_qib_sys_image_guid = ppd->guid;

	lcpysz = strlcpy(ibdev->name, class_name(), lcpysz);
	strlcpy(ibdev->name + lcpysz, "%d", lcpysz);
	ibdev->owner = THIS_MODULE;
	ibdev->node_guid = ppd->guid;
	ibdev->uverbs_abi_ver = QIB_UVERBS_ABI_VERSION;
	ibdev->uverbs_cmd_mask =
		(1ull << IB_USER_VERBS_CMD_GET_CONTEXT)         |
		(1ull << IB_USER_VERBS_CMD_QUERY_DEVICE)        |
		(1ull << IB_USER_VERBS_CMD_QUERY_PORT)          |
		(1ull << IB_USER_VERBS_CMD_ALLOC_PD)            |
		(1ull << IB_USER_VERBS_CMD_DEALLOC_PD)          |
		(1ull << IB_USER_VERBS_CMD_CREATE_AH)           |
		(1ull << IB_USER_VERBS_CMD_MODIFY_AH)           |
		(1ull << IB_USER_VERBS_CMD_QUERY_AH)            |
		(1ull << IB_USER_VERBS_CMD_DESTROY_AH)          |
		(1ull << IB_USER_VERBS_CMD_REG_MR)              |
		(1ull << IB_USER_VERBS_CMD_DEREG_MR)            |
		(1ull << IB_USER_VERBS_CMD_CREATE_COMP_CHANNEL) |
		(1ull << IB_USER_VERBS_CMD_CREATE_CQ)           |
		(1ull << IB_USER_VERBS_CMD_RESIZE_CQ)           |
		(1ull << IB_USER_VERBS_CMD_DESTROY_CQ)          |
		(1ull << IB_USER_VERBS_CMD_POLL_CQ)             |
		(1ull << IB_USER_VERBS_CMD_REQ_NOTIFY_CQ)       |
		(1ull << IB_USER_VERBS_CMD_CREATE_QP)           |
		(1ull << IB_USER_VERBS_CMD_QUERY_QP)            |
		(1ull << IB_USER_VERBS_CMD_MODIFY_QP)           |
		(1ull << IB_USER_VERBS_CMD_DESTROY_QP)          |
		(1ull << IB_USER_VERBS_CMD_POST_SEND)           |
		(1ull << IB_USER_VERBS_CMD_POST_RECV)           |
		(1ull << IB_USER_VERBS_CMD_ATTACH_MCAST)        |
		(1ull << IB_USER_VERBS_CMD_DETACH_MCAST)        |
		(1ull << IB_USER_VERBS_CMD_CREATE_SRQ)          |
		(1ull << IB_USER_VERBS_CMD_MODIFY_SRQ)          |
		(1ull << IB_USER_VERBS_CMD_QUERY_SRQ)           |
		(1ull << IB_USER_VERBS_CMD_DESTROY_SRQ)         |
		(1ull << IB_USER_VERBS_CMD_POST_SRQ_RECV);
	ibdev->node_type = RDMA_NODE_IB_CA;
	ibdev->phys_port_cnt = dd->num_pports;
	ibdev->num_comp_vectors = 1;
	ibdev->dma_device = &dd->pcidev->dev;
	ibdev->query_device = qib_query_device;
	ibdev->modify_device = qib_modify_device;
	ibdev->query_port = qib_query_port;
	ibdev->modify_port = qib_modify_port;
	ibdev->query_pkey = qib_query_pkey;
	ibdev->query_gid = qib_query_gid;
	ibdev->alloc_ucontext = qib_alloc_ucontext;
	ibdev->dealloc_ucontext = qib_dealloc_ucontext;
	ibdev->alloc_pd = qib_alloc_pd;
	ibdev->dealloc_pd = qib_dealloc_pd;
	ibdev->create_ah = qib_create_ah;
	ibdev->destroy_ah = qib_destroy_ah;
	ibdev->modify_ah = qib_modify_ah;
	ibdev->query_ah = qib_query_ah;
	ibdev->create_srq = qib_create_srq;
	ibdev->modify_srq = qib_modify_srq;
	ibdev->query_srq = qib_query_srq;
	ibdev->destroy_srq = qib_destroy_srq;
	ibdev->create_qp = qib_create_qp;
	ibdev->modify_qp = qib_modify_qp;
	ibdev->query_qp = qib_query_qp;
	ibdev->destroy_qp = qib_destroy_qp;
	ibdev->post_send = qib_post_send;
	ibdev->post_recv = qib_post_receive;
	ibdev->post_srq_recv = qib_post_srq_receive;
	ibdev->create_cq = qib_create_cq;
	ibdev->destroy_cq = qib_destroy_cq;
	ibdev->resize_cq = qib_resize_cq;
	ibdev->poll_cq = qib_poll_cq;
	ibdev->req_notify_cq = qib_req_notify_cq;
	ibdev->get_dma_mr = qib_get_dma_mr;
	ibdev->reg_phys_mr = qib_reg_phys_mr;
	ibdev->reg_user_mr = qib_reg_user_mr;
	ibdev->dereg_mr = qib_dereg_mr;
	ibdev->alloc_fast_reg_mr = qib_alloc_fast_reg_mr;
	ibdev->alloc_fast_reg_page_list = qib_alloc_fast_reg_page_list;
	ibdev->free_fast_reg_page_list = qib_free_fast_reg_page_list;
	ibdev->alloc_fmr = qib_alloc_fmr;
	ibdev->map_phys_fmr = qib_map_phys_fmr;
	ibdev->unmap_fmr = qib_unmap_fmr;
	ibdev->dealloc_fmr = qib_dealloc_fmr;
	ibdev->attach_mcast = qib_multicast_attach;
	ibdev->detach_mcast = qib_multicast_detach;
	ibdev->process_mad = qib_process_mad;
	ibdev->mmap = qib_mmap;
	ibdev->dma_ops = &qib_dma_mapping_ops;

	strncpy(ibdev->node_desc, init_utsname()->nodename,
		sizeof(ibdev->node_desc));

	ret = ib_register_device(ibdev, qib_create_port_files);
	if (ret)
		goto err_reg;

	ret = qib_create_agents(dev);
	if (ret)
		goto err_agents;

	if (qib_verbs_register_sysfs(dd))
		goto err_class;

	goto bail;

err_class:
	qib_free_agents(dev);
err_agents:
	ib_unregister_device(ibdev);
err_reg:
err_tx:
	while (!list_empty(&dev->txreq_free)) {
		struct list_head *l = dev->txreq_free.next;
		struct verbs_txreq *tx;

		list_del(l);
		tx = list_entry(l, struct verbs_txreq, txreq.list);
		kfree(tx);
	}
	if (dev->pio_hdrs)
		dma_free_coherent(&dd->pcidev->dev,
				  dev->pio_hdr_bytes,
				  dev->pio_hdrs, dev->pio_hdrs_phys);
err_hdrs:
	free_pages((unsigned long) dev->lk_table.table, get_order(lk_tab_size));
err_lk:
	qib_qp_exit(dev);
err_qp_init:
	dd_dev_err(dd, "cannot register verbs: %d!\n", -ret);
bail:
	return ret;
}

void qib_unregister_ib_device(struct hfi_devdata *dd)
{
	struct qib_ibdev *dev = &dd->verbs_dev;
	struct ib_device *ibdev = &dev->ibdev;
	unsigned lk_tab_size;

	qib_verbs_unregister_sysfs(dd);

	qib_free_agents(dev);

	ib_unregister_device(ibdev);

	if (!list_empty(&dev->txwait))
		dd_dev_err(dd, "txwait list not empty!\n");
	if (!list_empty(&dev->memwait))
		dd_dev_err(dd, "memwait list not empty!\n");
	if (dev->dma_mr)
		dd_dev_err(dd, "DMA MR not NULL!\n");

	qib_qp_exit(dev);
	del_timer_sync(&dev->mem_timer);
	while (!list_empty(&dev->txreq_free)) {
		struct list_head *l = dev->txreq_free.next;
		struct verbs_txreq *tx;

		list_del(l);
		tx = list_entry(l, struct verbs_txreq, txreq.list);
		kfree(tx);
	}
	if (dev->pio_hdrs)
		dma_free_coherent(&dd->pcidev->dev,
				  dev->pio_hdr_bytes,
				  dev->pio_hdrs, dev->pio_hdrs_phys);
	lk_tab_size = dev->lk_table.max * sizeof(*dev->lk_table.table);
	free_pages((unsigned long) dev->lk_table.table,
		   get_order(lk_tab_size));
}

/*
 * This must be called with s_lock held.
 */
void qib_schedule_send(struct qib_qp *qp)
{
	if (qib_send_ok(qp)) {
		struct qib_ibport *ibp =
			to_iport(qp->ibqp.device, qp->port_num);
		struct qib_pportdata *ppd = ppd_from_ibp(ibp);

		iowait_schedule(&qp->s_iowait, ppd->qib_wq);
	}
}
