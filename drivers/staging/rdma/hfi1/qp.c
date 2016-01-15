/*
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright(c) 2015 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * BSD LICENSE
 *
 * Copyright(c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Intel Corporation nor the names of its
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
 */

#include <linux/err.h>
#include <linux/vmalloc.h>
#include <linux/hash.h>
#include <linux/module.h>
#include <linux/random.h>
#include <linux/seq_file.h>
#include <rdma/rdma_vt.h>
#include <rdma/rdmavt_qp.h>

#include "hfi.h"
#include "qp.h"
#include "trace.h"
#include "sdma.h"

unsigned int hfi1_qp_table_size = 256;
module_param_named(qp_table_size, hfi1_qp_table_size, uint, S_IRUGO);
MODULE_PARM_DESC(qp_table_size, "QP table size");

static void flush_tx_list(struct rvt_qp *qp);
static int iowait_sleep(
	struct sdma_engine *sde,
	struct iowait *wait,
	struct sdma_txreq *stx,
	unsigned seq);
static void iowait_wakeup(struct iowait *wait, int reason);

static inline unsigned mk_qpn(struct rvt_qpn_table *qpt,
			      struct rvt_qpn_map *map, unsigned off)
{
	return (map - qpt->map) * RVT_BITS_PER_PAGE + off;
}

/*
 * Convert the AETH credit code into the number of credits.
 */
static const u16 credit_table[31] = {
	0,                      /* 0 */
	1,                      /* 1 */
	2,                      /* 2 */
	3,                      /* 3 */
	4,                      /* 4 */
	6,                      /* 5 */
	8,                      /* 6 */
	12,                     /* 7 */
	16,                     /* 8 */
	24,                     /* 9 */
	32,                     /* A */
	48,                     /* B */
	64,                     /* C */
	96,                     /* D */
	128,                    /* E */
	192,                    /* F */
	256,                    /* 10 */
	384,                    /* 11 */
	512,                    /* 12 */
	768,                    /* 13 */
	1024,                   /* 14 */
	1536,                   /* 15 */
	2048,                   /* 16 */
	3072,                   /* 17 */
	4096,                   /* 18 */
	6144,                   /* 19 */
	8192,                   /* 1A */
	12288,                  /* 1B */
	16384,                  /* 1C */
	24576,                  /* 1D */
	32768                   /* 1E */
};

static void flush_tx_list(struct rvt_qp *qp)
{
	struct hfi1_qp_priv *priv = qp->priv;

	while (!list_empty(&priv->s_iowait.tx_head)) {
		struct sdma_txreq *tx;

		tx = list_first_entry(
			&priv->s_iowait.tx_head,
			struct sdma_txreq,
			list);
		list_del_init(&tx->list);
		hfi1_put_txreq(
			container_of(tx, struct verbs_txreq, txreq));
	}
}

static void flush_iowait(struct rvt_qp *qp)
{
	struct hfi1_qp_priv *priv = qp->priv;
	struct hfi1_ibdev *dev = to_idev(qp->ibqp.device);
	unsigned long flags;

	write_seqlock_irqsave(&dev->iowait_lock, flags);
	if (!list_empty(&priv->s_iowait.list)) {
		list_del_init(&priv->s_iowait.list);
		if (atomic_dec_and_test(&qp->refcount))
			wake_up(&qp->wait);
	}
	write_sequnlock_irqrestore(&dev->iowait_lock, flags);
}

static inline int opa_mtu_enum_to_int(int mtu)
{
	switch (mtu) {
	case OPA_MTU_8192:  return 8192;
	case OPA_MTU_10240: return 10240;
	default:            return -1;
	}
}

/**
 * This function is what we would push to the core layer if we wanted to be a
 * "first class citizen".  Instead we hide this here and rely on Verbs ULPs
 * to blindly pass the MTU enum value from the PathRecord to us.
 *
 * The actual flag used to determine "8k MTU" will change and is currently
 * unknown.
 */
static inline int verbs_mtu_enum_to_int(struct ib_device *dev, enum ib_mtu mtu)
{
	int val = opa_mtu_enum_to_int((int)mtu);

	if (val > 0)
		return val;
	return ib_mtu_enum_to_int(mtu);
}

int hfi1_check_modify_qp(struct rvt_qp *qp, struct ib_qp_attr *attr,
			 int attr_mask, struct ib_udata *udata)
{
	struct ib_qp *ibqp = &qp->ibqp;
	struct hfi1_ibdev *dev = to_idev(ibqp->device);
	struct hfi1_devdata *dd = dd_from_dev(dev);
	u8 sc;

	if (attr_mask & IB_QP_AV) {
		sc = ah_to_sc(ibqp->device, &attr->ah_attr);
		if (!qp_to_sdma_engine(qp, sc) &&
		    dd->flags & HFI1_HAS_SEND_DMA)
			return -EINVAL;
	}

	if (attr_mask & IB_QP_ALT_PATH) {
		sc = ah_to_sc(ibqp->device, &attr->alt_ah_attr);
		if (!qp_to_sdma_engine(qp, sc) &&
		    dd->flags & HFI1_HAS_SEND_DMA)
			return -EINVAL;
	}

	return 0;
}

void hfi1_modify_qp(struct rvt_qp *qp, struct ib_qp_attr *attr,
		    int attr_mask, struct ib_udata *udata)
{
	struct ib_qp *ibqp = &qp->ibqp;
	struct hfi1_qp_priv *priv = qp->priv;

	if (attr_mask & IB_QP_AV) {
		priv->s_sc = ah_to_sc(ibqp->device, &qp->remote_ah_attr);
		priv->s_sde = qp_to_sdma_engine(qp, priv->s_sc);
	}

	if (attr_mask & IB_QP_PATH_MIG_STATE &&
	    attr->path_mig_state == IB_MIG_MIGRATED &&
	    qp->s_mig_state == IB_MIG_ARMED) {
		qp->s_flags |= RVT_S_AHG_CLEAR;
		priv->s_sc = ah_to_sc(ibqp->device, &qp->remote_ah_attr);
		priv->s_sde = qp_to_sdma_engine(qp, priv->s_sc);
	}
}

int hfi1_query_qp(struct ib_qp *ibqp, struct ib_qp_attr *attr,
		  int attr_mask, struct ib_qp_init_attr *init_attr)
{
	struct rvt_qp *qp = ibqp_to_rvtqp(ibqp);

	attr->qp_state = qp->state;
	attr->cur_qp_state = attr->qp_state;
	attr->path_mtu = qp->path_mtu;
	attr->path_mig_state = qp->s_mig_state;
	attr->qkey = qp->qkey;
	attr->rq_psn = mask_psn(qp->r_psn);
	attr->sq_psn = mask_psn(qp->s_next_psn);
	attr->dest_qp_num = qp->remote_qpn;
	attr->qp_access_flags = qp->qp_access_flags;
	attr->cap.max_send_wr = qp->s_size - 1;
	attr->cap.max_recv_wr = qp->ibqp.srq ? 0 : qp->r_rq.size - 1;
	attr->cap.max_send_sge = qp->s_max_sge;
	attr->cap.max_recv_sge = qp->r_rq.max_sge;
	attr->cap.max_inline_data = 0;
	attr->ah_attr = qp->remote_ah_attr;
	attr->alt_ah_attr = qp->alt_ah_attr;
	attr->pkey_index = qp->s_pkey_index;
	attr->alt_pkey_index = qp->s_alt_pkey_index;
	attr->en_sqd_async_notify = 0;
	attr->sq_draining = qp->s_draining;
	attr->max_rd_atomic = qp->s_max_rd_atomic;
	attr->max_dest_rd_atomic = qp->r_max_rd_atomic;
	attr->min_rnr_timer = qp->r_min_rnr_timer;
	attr->port_num = qp->port_num;
	attr->timeout = qp->timeout;
	attr->retry_cnt = qp->s_retry_cnt;
	attr->rnr_retry = qp->s_rnr_retry_cnt;
	attr->alt_port_num = qp->alt_ah_attr.port_num;
	attr->alt_timeout = qp->alt_timeout;

	init_attr->event_handler = qp->ibqp.event_handler;
	init_attr->qp_context = qp->ibqp.qp_context;
	init_attr->send_cq = qp->ibqp.send_cq;
	init_attr->recv_cq = qp->ibqp.recv_cq;
	init_attr->srq = qp->ibqp.srq;
	init_attr->cap = attr->cap;
	if (qp->s_flags & RVT_S_SIGNAL_REQ_WR)
		init_attr->sq_sig_type = IB_SIGNAL_REQ_WR;
	else
		init_attr->sq_sig_type = IB_SIGNAL_ALL_WR;
	init_attr->qp_type = qp->ibqp.qp_type;
	init_attr->port_num = qp->port_num;
	return 0;
}

/**
 * hfi1_compute_aeth - compute the AETH (syndrome + MSN)
 * @qp: the queue pair to compute the AETH for
 *
 * Returns the AETH.
 */
__be32 hfi1_compute_aeth(struct rvt_qp *qp)
{
	u32 aeth = qp->r_msn & HFI1_MSN_MASK;

	if (qp->ibqp.srq) {
		/*
		 * Shared receive queues don't generate credits.
		 * Set the credit field to the invalid value.
		 */
		aeth |= HFI1_AETH_CREDIT_INVAL << HFI1_AETH_CREDIT_SHIFT;
	} else {
		u32 min, max, x;
		u32 credits;
		struct rvt_rwq *wq = qp->r_rq.wq;
		u32 head;
		u32 tail;

		/* sanity check pointers before trusting them */
		head = wq->head;
		if (head >= qp->r_rq.size)
			head = 0;
		tail = wq->tail;
		if (tail >= qp->r_rq.size)
			tail = 0;
		/*
		 * Compute the number of credits available (RWQEs).
		 * There is a small chance that the pair of reads are
		 * not atomic, which is OK, since the fuzziness is
		 * resolved as further ACKs go out.
		 */
		credits = head - tail;
		if ((int)credits < 0)
			credits += qp->r_rq.size;
		/*
		 * Binary search the credit table to find the code to
		 * use.
		 */
		min = 0;
		max = 31;
		for (;;) {
			x = (min + max) / 2;
			if (credit_table[x] == credits)
				break;
			if (credit_table[x] > credits)
				max = x;
			else if (min == x)
				break;
			else
				min = x;
		}
		aeth |= x << HFI1_AETH_CREDIT_SHIFT;
	}
	return cpu_to_be32(aeth);
}

/**
 * hfi1_get_credit - flush the send work queue of a QP
 * @qp: the qp who's send work queue to flush
 * @aeth: the Acknowledge Extended Transport Header
 *
 * The QP s_lock should be held.
 */
void hfi1_get_credit(struct rvt_qp *qp, u32 aeth)
{
	u32 credit = (aeth >> HFI1_AETH_CREDIT_SHIFT) & HFI1_AETH_CREDIT_MASK;

	/*
	 * If the credit is invalid, we can send
	 * as many packets as we like.  Otherwise, we have to
	 * honor the credit field.
	 */
	if (credit == HFI1_AETH_CREDIT_INVAL) {
		if (!(qp->s_flags & RVT_S_UNLIMITED_CREDIT)) {
			qp->s_flags |= RVT_S_UNLIMITED_CREDIT;
			if (qp->s_flags & RVT_S_WAIT_SSN_CREDIT) {
				qp->s_flags &= ~RVT_S_WAIT_SSN_CREDIT;
				hfi1_schedule_send(qp);
			}
		}
	} else if (!(qp->s_flags & RVT_S_UNLIMITED_CREDIT)) {
		/* Compute new LSN (i.e., MSN + credit) */
		credit = (aeth + credit_table[credit]) & HFI1_MSN_MASK;
		if (cmp_msn(credit, qp->s_lsn) > 0) {
			qp->s_lsn = credit;
			if (qp->s_flags & RVT_S_WAIT_SSN_CREDIT) {
				qp->s_flags &= ~RVT_S_WAIT_SSN_CREDIT;
				hfi1_schedule_send(qp);
			}
		}
	}
}

void hfi1_qp_wakeup(struct rvt_qp *qp, u32 flag)
{
	unsigned long flags;

	spin_lock_irqsave(&qp->s_lock, flags);
	if (qp->s_flags & flag) {
		qp->s_flags &= ~flag;
		trace_hfi1_qpwakeup(qp, flag);
		hfi1_schedule_send(qp);
	}
	spin_unlock_irqrestore(&qp->s_lock, flags);
	/* Notify hfi1_destroy_qp() if it is waiting. */
	if (atomic_dec_and_test(&qp->refcount))
		wake_up(&qp->wait);
}

static int iowait_sleep(
	struct sdma_engine *sde,
	struct iowait *wait,
	struct sdma_txreq *stx,
	unsigned seq)
{
	struct verbs_txreq *tx = container_of(stx, struct verbs_txreq, txreq);
	struct rvt_qp *qp;
	struct hfi1_qp_priv *priv;
	unsigned long flags;
	int ret = 0;
	struct hfi1_ibdev *dev;

	qp = tx->qp;
	priv = qp->priv;

	spin_lock_irqsave(&qp->s_lock, flags);
	if (ib_rvt_state_ops[qp->state] & RVT_PROCESS_RECV_OK) {

		/*
		 * If we couldn't queue the DMA request, save the info
		 * and try again later rather than destroying the
		 * buffer and undoing the side effects of the copy.
		 */
		/* Make a common routine? */
		dev = &sde->dd->verbs_dev;
		list_add_tail(&stx->list, &wait->tx_head);
		write_seqlock(&dev->iowait_lock);
		if (sdma_progress(sde, seq, stx))
			goto eagain;
		if (list_empty(&priv->s_iowait.list)) {
			struct hfi1_ibport *ibp =
				to_iport(qp->ibqp.device, qp->port_num);

			ibp->rvp.n_dmawait++;
			qp->s_flags |= RVT_S_WAIT_DMA_DESC;
			list_add_tail(&priv->s_iowait.list, &sde->dmawait);
			trace_hfi1_qpsleep(qp, RVT_S_WAIT_DMA_DESC);
			atomic_inc(&qp->refcount);
		}
		write_sequnlock(&dev->iowait_lock);
		qp->s_flags &= ~RVT_S_BUSY;
		spin_unlock_irqrestore(&qp->s_lock, flags);
		ret = -EBUSY;
	} else {
		spin_unlock_irqrestore(&qp->s_lock, flags);
		hfi1_put_txreq(tx);
	}
	return ret;
eagain:
	write_sequnlock(&dev->iowait_lock);
	spin_unlock_irqrestore(&qp->s_lock, flags);
	list_del_init(&stx->list);
	return -EAGAIN;
}

static void iowait_wakeup(struct iowait *wait, int reason)
{
	struct rvt_qp *qp = iowait_to_qp(wait);

	WARN_ON(reason != SDMA_AVAIL_REASON);
	hfi1_qp_wakeup(qp, RVT_S_WAIT_DMA_DESC);
}

/**
 *
 * qp_to_sdma_engine - map a qp to a send engine
 * @qp: the QP
 * @sc5: the 5 bit sc
 *
 * Return:
 * A send engine for the qp or NULL for SMI type qp.
 */
struct sdma_engine *qp_to_sdma_engine(struct rvt_qp *qp, u8 sc5)
{
	struct hfi1_devdata *dd = dd_from_ibdev(qp->ibqp.device);
	struct sdma_engine *sde;

	if (!(dd->flags & HFI1_HAS_SEND_DMA))
		return NULL;
	switch (qp->ibqp.qp_type) {
	case IB_QPT_SMI:
		return NULL;
	default:
		break;
	}
	sde = sdma_select_engine_sc(dd, qp->ibqp.qp_num >> dd->qos_shift, sc5);
	return sde;
}

struct qp_iter {
	struct hfi1_ibdev *dev;
	struct rvt_qp *qp;
	int specials;
	int n;
};

struct qp_iter *qp_iter_init(struct hfi1_ibdev *dev)
{
	struct qp_iter *iter;

	iter = kzalloc(sizeof(*iter), GFP_KERNEL);
	if (!iter)
		return NULL;

	iter->dev = dev;
	iter->specials = dev->rdi.ibdev.phys_port_cnt * 2;
	if (qp_iter_next(iter)) {
		kfree(iter);
		return NULL;
	}

	return iter;
}

int qp_iter_next(struct qp_iter *iter)
{
	struct hfi1_ibdev *dev = iter->dev;
	int n = iter->n;
	int ret = 1;
	struct rvt_qp *pqp = iter->qp;
	struct rvt_qp *qp;

	/*
	 * The approach is to consider the special qps
	 * as an additional table entries before the
	 * real hash table.  Since the qp code sets
	 * the qp->next hash link to NULL, this works just fine.
	 *
	 * iter->specials is 2 * # ports
	 *
	 * n = 0..iter->specials is the special qp indices
	 *
	 * n = iter->specials..dev->rdi.qp_dev->qp_table_size+iter->specials are
	 * the potential hash bucket entries
	 *
	 */
	for (; n <  dev->rdi.qp_dev->qp_table_size + iter->specials; n++) {
		if (pqp) {
			qp = rcu_dereference(pqp->next);
		} else {
			if (n < iter->specials) {
				struct hfi1_pportdata *ppd;
				struct hfi1_ibport *ibp;
				int pidx;

				pidx = n % dev->rdi.ibdev.phys_port_cnt;
				ppd = &dd_from_dev(dev)->pport[pidx];
				ibp = &ppd->ibport_data;

				if (!(n & 1))
					qp = rcu_dereference(ibp->rvp.qp[0]);
				else
					qp = rcu_dereference(ibp->rvp.qp[1]);
			} else {
				qp = rcu_dereference(
					dev->rdi.qp_dev->qp_table[
						(n - iter->specials)]);
			}
		}
		pqp = qp;
		if (qp) {
			iter->qp = qp;
			iter->n = n;
			return 0;
		}
	}
	return ret;
}

static const char * const qp_type_str[] = {
	"SMI", "GSI", "RC", "UC", "UD",
};

static int qp_idle(struct rvt_qp *qp)
{
	return
		qp->s_last == qp->s_acked &&
		qp->s_acked == qp->s_cur &&
		qp->s_cur == qp->s_tail &&
		qp->s_tail == qp->s_head;
}

void qp_iter_print(struct seq_file *s, struct qp_iter *iter)
{
	struct rvt_swqe *wqe;
	struct rvt_qp *qp = iter->qp;
	struct hfi1_qp_priv *priv = qp->priv;
	struct sdma_engine *sde;

	sde = qp_to_sdma_engine(qp, priv->s_sc);
	wqe = rvt_get_swqe_ptr(qp, qp->s_last);
	seq_printf(s,
		   "N %d %s QP%u R %u %s %u %u %u f=%x %u %u %u %u %u PSN %x %x %x %x %x (%u %u %u %u %u %u) QP%u LID %x SL %u MTU %d %u %u %u SDE %p,%u\n",
		   iter->n,
		   qp_idle(qp) ? "I" : "B",
		   qp->ibqp.qp_num,
		   atomic_read(&qp->refcount),
		   qp_type_str[qp->ibqp.qp_type],
		   qp->state,
		   wqe ? wqe->wr.opcode : 0,
		   qp->s_hdrwords,
		   qp->s_flags,
		   atomic_read(&priv->s_iowait.sdma_busy),
		   !list_empty(&priv->s_iowait.list),
		   qp->timeout,
		   wqe ? wqe->ssn : 0,
		   qp->s_lsn,
		   qp->s_last_psn,
		   qp->s_psn, qp->s_next_psn,
		   qp->s_sending_psn, qp->s_sending_hpsn,
		   qp->s_last, qp->s_acked, qp->s_cur,
		   qp->s_tail, qp->s_head, qp->s_size,
		   qp->remote_qpn,
		   qp->remote_ah_attr.dlid,
		   qp->remote_ah_attr.sl,
		   qp->pmtu,
		   qp->s_retry_cnt,
		   qp->timeout,
		   qp->s_rnr_retry_cnt,
		   sde,
		   sde ? sde->this_idx : 0);
}

void qp_comm_est(struct rvt_qp *qp)
{
	qp->r_flags |= RVT_R_COMM_EST;
	if (qp->ibqp.event_handler) {
		struct ib_event ev;

		ev.device = qp->ibqp.device;
		ev.element.qp = &qp->ibqp;
		ev.event = IB_EVENT_COMM_EST;
		qp->ibqp.event_handler(&ev, qp->ibqp.qp_context);
	}
}

void *qp_priv_alloc(struct rvt_dev_info *rdi, struct rvt_qp *qp)
{
	struct hfi1_qp_priv *priv;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return ERR_PTR(-ENOMEM);

	priv->owner = qp;

	priv->s_hdr = kzalloc(sizeof(*priv->s_hdr), GFP_KERNEL);
	if (!priv->s_hdr) {
		kfree(priv);
		return ERR_PTR(-ENOMEM);
	}

	return priv;
}

void qp_priv_free(struct rvt_dev_info *rdi, struct rvt_qp *qp)
{
	struct hfi1_qp_priv *priv = qp->priv;

	kfree(priv->s_hdr);
	kfree(priv);
}

unsigned free_all_qps(struct rvt_dev_info *rdi)
{
	struct hfi1_ibdev *verbs_dev = container_of(rdi,
						    struct hfi1_ibdev,
						    rdi);
	struct hfi1_devdata *dd = container_of(verbs_dev,
					       struct hfi1_devdata,
					       verbs_dev);
	int n;
	unsigned qp_inuse = 0;

	for (n = 0; n < dd->num_pports; n++) {
		struct hfi1_ibport *ibp = &dd->pport[n].ibport_data;

		rcu_read_lock();
		if (rcu_dereference(ibp->rvp.qp[0]))
			qp_inuse++;
		if (rcu_dereference(ibp->rvp.qp[1]))
			qp_inuse++;
		rcu_read_unlock();
	}

	return qp_inuse;
}

void flush_qp_waiters(struct rvt_qp *qp)
{
	flush_iowait(qp);
}

void stop_send_queue(struct rvt_qp *qp)
{
	struct hfi1_qp_priv *priv = qp->priv;

	cancel_work_sync(&priv->s_iowait.iowork);
}

void quiesce_qp(struct rvt_qp *qp)
{
	struct hfi1_qp_priv *priv = qp->priv;

	iowait_sdma_drain(&priv->s_iowait);
	flush_tx_list(qp);
}

void notify_qp_reset(struct rvt_qp *qp)
{
	struct hfi1_qp_priv *priv = qp->priv;

	iowait_init(
		&priv->s_iowait,
		1,
		_hfi1_do_send,
		iowait_sleep,
		iowait_wakeup);
	priv->r_adefered = 0;
	clear_ahg(qp);
}

/*
 * Switch to alternate path.
 * The QP s_lock should be held and interrupts disabled.
 */
void hfi1_migrate_qp(struct rvt_qp *qp)
{
	struct hfi1_qp_priv *priv = qp->priv;
	struct ib_event ev;

	qp->s_mig_state = IB_MIG_MIGRATED;
	qp->remote_ah_attr = qp->alt_ah_attr;
	qp->port_num = qp->alt_ah_attr.port_num;
	qp->s_pkey_index = qp->s_alt_pkey_index;
	qp->s_flags |= RVT_S_AHG_CLEAR;
	priv->s_sc = ah_to_sc(qp->ibqp.device, &qp->remote_ah_attr);
	priv->s_sde = qp_to_sdma_engine(qp, priv->s_sc);

	ev.device = qp->ibqp.device;
	ev.element.qp = &qp->ibqp;
	ev.event = IB_EVENT_PATH_MIG;
	qp->ibqp.event_handler(&ev, qp->ibqp.qp_context);
}

int mtu_to_path_mtu(u32 mtu)
{
	return mtu_to_enum(mtu, OPA_MTU_8192);
}

u32 mtu_from_qp(struct rvt_dev_info *rdi, struct rvt_qp *qp, u32 pmtu)
{
	u32 mtu;
	struct hfi1_ibdev *verbs_dev = container_of(rdi,
						    struct hfi1_ibdev,
						    rdi);
	struct hfi1_devdata *dd = container_of(verbs_dev,
					       struct hfi1_devdata,
					       verbs_dev);
	struct hfi1_ibport *ibp;
	u8 sc, vl;

	ibp = &dd->pport[qp->port_num - 1].ibport_data;
	sc = ibp->sl_to_sc[qp->remote_ah_attr.sl];
	vl = sc_to_vlt(dd, sc);

	mtu = verbs_mtu_enum_to_int(qp->ibqp.device, pmtu);
	if (vl < PER_VL_SEND_CONTEXTS)
		mtu = min_t(u32, mtu, dd->vld[vl].mtu);
	return mtu;
}

int get_pmtu_from_attr(struct rvt_dev_info *rdi, struct rvt_qp *qp,
		       struct ib_qp_attr *attr)
{
	int mtu, pidx = qp->port_num - 1;
	struct hfi1_ibdev *verbs_dev = container_of(rdi,
						    struct hfi1_ibdev,
						    rdi);
	struct hfi1_devdata *dd = container_of(verbs_dev,
					       struct hfi1_devdata,
					       verbs_dev);
	mtu = verbs_mtu_enum_to_int(qp->ibqp.device, attr->path_mtu);
	if (mtu == -1)
		return -1; /* values less than 0 are error */

	if (mtu > dd->pport[pidx].ibmtu)
		return mtu_to_enum(dd->pport[pidx].ibmtu, IB_MTU_2048);
	else
		return attr->path_mtu;
}

void notify_error_qp(struct rvt_qp *qp)
{
	struct hfi1_ibdev *dev = to_idev(qp->ibqp.device);
	struct hfi1_qp_priv *priv = qp->priv;

	write_seqlock(&dev->iowait_lock);
	if (!list_empty(&priv->s_iowait.list) && !(qp->s_flags & RVT_S_BUSY)) {
		qp->s_flags &= ~RVT_S_ANY_WAIT_IO;
		list_del_init(&priv->s_iowait.list);
		if (atomic_dec_and_test(&qp->refcount))
			wake_up(&qp->wait);
	}
	write_sequnlock(&dev->iowait_lock);

	if (!(qp->s_flags & RVT_S_BUSY)) {
		qp->s_hdrwords = 0;
		if (qp->s_rdma_mr) {
			rvt_put_mr(qp->s_rdma_mr);
			qp->s_rdma_mr = NULL;
		}
		flush_tx_list(qp);
	}
}

