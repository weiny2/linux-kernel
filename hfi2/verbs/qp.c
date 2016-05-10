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

#include <linux/sched.h>
#include "verbs.h"
#include "packet.h"

static void remove_qp(struct hfi2_ibdev *ibd, struct rvt_qp *qp);

/*
 * Allocate the next available QPN or
 * zero/one for QP type IB_QPT_SMI/IB_QPT_GSI.
 */
static int alloc_qpn(struct hfi2_ibdev *ibd,
		     enum ib_qp_type type, u8 port)
{
	int ret;

	if (type == IB_QPT_SMI || type == IB_QPT_GSI) {
		unsigned long n;

		ret = type == IB_QPT_GSI;
		n = 1 << (ret + 2 * (port - 1));
		spin_lock(&ibd->qpt_lock);
		if (test_bit(n, &ibd->reserved_qps))
			ret = -EINVAL;
		else
			set_bit(n, &ibd->reserved_qps);
		spin_unlock(&ibd->qpt_lock);
	} else {
		/*
		 * QPN[8..1] is used to spread QPNs across HW receive contexts,
		 * so allocate even QPNs first.
		 */
		ret = ida_simple_get(&ibd->qpn_even_table, 1,
				     OPA_QPN_VERBS_MAX/2, GFP_KERNEL);
		if (ret > 0)
			ret = ret << 1;
		else {
			ret = ida_simple_get(&ibd->qpn_odd_table, 1,
					     OPA_QPN_VERBS_MAX/2, GFP_KERNEL);
			if (ret > 0)
				ret = (ret << 1) + 1;
		}
		/* returned value is QPN or error */
	}

	return ret;
}

static void free_qpn(struct hfi2_ibdev *ibd, struct rvt_qp *qp)
{
	u8 ib_port = qp->port_num;
	u32 qpn = qp->ibqp.qp_num;

	if (qpn > 1) {
		struct ida *qpn_table = (qpn % 2) ? &ibd->qpn_odd_table :
						    &ibd->qpn_even_table;
		ida_simple_remove(qpn_table, qpn >> 1);
	} else {
		unsigned n;

		n = 1 << (qpn + 2 * (ib_port - 1));
		spin_lock(&ibd->qpt_lock);
		if (test_bit(n, &ibd->reserved_qps))
			clear_bit(n, &ibd->reserved_qps);
		spin_unlock(&ibd->qpt_lock);
	}
}

/*
 * Put the QP into the hash table.
 * The hash table holds a reference to the QP.
 */
static int insert_qp(struct hfi2_ibdev *ibd, struct rvt_qp *qp, bool is_user)
{
	struct hfi2_ibport *ibp;
	int ret = 0;
	unsigned long flags;
	u32 qpn = qp->ibqp.qp_num;

	ibp = to_hfi_ibp(qp->ibqp.device, qp->port_num);

	atomic_inc(&qp->refcount);
	idr_preload(GFP_KERNEL);
	spin_lock_irqsave(&ibd->qpt_lock, flags);
	if (qpn <= 1) {
		rcu_assign_pointer(ibp->qp[qpn], qp);
	} else {
		ret = idr_alloc(&ibd->qp_ptr, qp, qpn, qpn+1, GFP_NOWAIT);
		if (ret >= 0) {
			BUG_ON(ret != qpn); /* temporary sanity check */
			ret = 0; /* return success */
		}
	}
	spin_unlock_irqrestore(&ibd->qpt_lock, flags);
	idr_preload_end();

	if (ret < 0)
		goto bail;

	/* Associate QP with Send Context */
	ret = hfi2_ctx_assign_qp(ibp, qp->priv, is_user);
	if (ret < 0)
		goto bail_ctx;

	return 0;

bail_ctx:
	remove_qp(ibd, qp);
bail:
	return ret;
}

/*
 * Remove the QP from the table so it can't be found asynchronously by
 * the receive interrupt routine.
 */
static void remove_qp(struct hfi2_ibdev *ibd, struct rvt_qp *qp)
{
	struct hfi2_ibport *ibp;
	unsigned long flags;
	int removed = 1;
	u32 qpn = qp->ibqp.qp_num;

	ibp = to_hfi_ibp(qp->ibqp.device, qp->port_num);

	/* Remove association with Send Context */
	hfi2_ctx_release_qp(ibp, qp->priv);

	spin_lock_irqsave(&ibd->qpt_lock, flags);
	if (qpn <= 1) {
		if (rcu_dereference_protected(ibp->qp[0],
		    lockdep_is_held(&ibd->qpt_lock)) == qp)
			RCU_INIT_POINTER(ibp->qp[0], NULL);
		else if (rcu_dereference_protected(ibp->qp[1],
			 lockdep_is_held(&ibd->qpt_lock)) == qp)
			RCU_INIT_POINTER(ibp->qp[1], NULL);
	} else {
		idr_remove(&ibd->qp_ptr, qpn);
	}
	spin_unlock_irqrestore(&ibd->qpt_lock, flags);

	if (removed) {
		synchronize_rcu();
		if (atomic_dec_and_test(&qp->refcount))
			wake_up(&qp->wait);
	}
}

static void flush_tx_list(struct rvt_qp *qp)
{
	/* FXRTODO SDMA -> TX_CQ */
}

static void flush_iowait(struct rvt_qp *qp)
{
	/* FXRTODO looks to be unneeded if we avoid adding a verbs_txreq */
}

/*
 * TODO - note the new functions below will be later registered with RDMAVT
 * in next phase of rvt_qp integration. They are based on functions in hfi1.
 */

static void *qp_priv_alloc(struct rvt_dev_info *rdi, struct rvt_qp *qp,
			   gfp_t gfp)
{
	struct hfi2_qp_priv *priv;

	priv = kzalloc_node(sizeof(*priv), gfp, rdi->dparms.node);
	if (!priv)
		return ERR_PTR(-ENOMEM);
	priv->owner = qp;

	priv->s_hdr = kzalloc_node(sizeof(*priv->s_hdr), gfp, rdi->dparms.node);
	if (!priv->s_hdr) {
		kfree(priv);
		return ERR_PTR(-ENOMEM);
	}

	return priv;
}

static void qp_priv_free(struct rvt_dev_info *rdi, struct rvt_qp *qp)
{
	struct hfi2_qp_priv *priv = qp->priv;

	kfree(priv->s_hdr);
	kfree(priv);
}

static void stop_send_queue(struct rvt_qp *qp)
{
	struct hfi2_qp_priv *priv = qp->priv;

	cancel_work_sync(&priv->s_iowait.iowork);
	del_timer_sync(&qp->s_timer);
}

static void quiesce_qp(struct rvt_qp *qp)
{
	struct hfi2_qp_priv *priv = qp->priv;

	iowait_sdma_drain(&priv->s_iowait);
	flush_tx_list(qp);
}

static void notify_qp_reset(struct rvt_qp *qp)
{
	struct hfi2_qp_priv *priv = qp->priv;

	iowait_init(&priv->s_iowait, 1, hfi2_do_send);

	/*
	 * pmtu unused for UD transport (unfragmented), so set to maximum here
	 * as logic to write DMA commands uses this
	 */
	qp->pmtu = opa_enum_to_mtu(OPA_MTU_10240);
}

void notify_error_qp(struct rvt_qp *qp)
{
	if (!(qp->s_flags & RVT_S_BUSY)) {
		flush_iowait(qp);
		qp->s_hdrwords = 0;
		if (qp->s_rdma_mr) {
			rvt_put_mr(qp->s_rdma_mr);
			qp->s_rdma_mr = NULL;
		}
		flush_tx_list(qp);
	}
}

/**
 * reset_qp - initialize the QP state to the reset state
 * @qp: the QP to reset
 * @type: the QP type
 */
static void reset_qp(struct rvt_qp *qp, enum ib_qp_type type)
{

	/* TODO - placeholder for future RDMAVT integration */
	notify_qp_reset(qp);

	qp->remote_qpn = 0;
	qp->qkey = 0;
	qp->qp_access_flags = 0;
	qp->s_flags &= RVT_S_SIGNAL_REQ_WR;
	qp->s_hdrwords = 0;
	qp->s_wqe = NULL;
	qp->s_draining = 0;
	qp->s_next_psn = 0;
	qp->s_last_psn = 0;
	qp->s_sending_psn = 0;
	qp->s_sending_hpsn = 0;
	qp->s_psn = 0;
	qp->r_psn = 0;
	qp->r_msn = 0;
	if (type == IB_QPT_RC) {
		qp->s_state = IB_OPCODE_RC_SEND_LAST;
		qp->r_state = IB_OPCODE_RC_SEND_LAST;
	} else {
		qp->s_state = IB_OPCODE_UC_SEND_LAST;
		qp->r_state = IB_OPCODE_UC_SEND_LAST;
	}
	qp->s_ack_state = IB_OPCODE_RC_ACKNOWLEDGE;
	qp->r_nak_state = 0;
	qp->r_aflags = 0;
	qp->r_flags = 0;
	qp->s_head = 0;
	qp->s_tail = 0;
	qp->s_cur = 0;
	qp->s_acked = 0;
	qp->s_last = 0;
	qp->s_ssn = 1;
	qp->s_lsn = 0;
	qp->s_mig_state = IB_MIG_MIGRATED;
	memset(qp->s_ack_queue, 0, sizeof(qp->s_ack_queue));
	qp->r_head_ack_queue = 0;
	qp->s_tail_ack_queue = 0;
	qp->s_num_rd_atomic = 0;
	if (qp->r_rq.wq) {
		qp->r_rq.wq->head = 0;
		qp->r_rq.wq->tail = 0;
	}
	qp->r_sge.num_sge = 0;
}

static void clear_mr_refs(struct rvt_qp *qp, int clr_sends)
{
	unsigned n;

	if (test_and_clear_bit(RVT_R_REWIND_SGE, &qp->r_aflags))
		rvt_put_ss(&qp->s_rdma_read_sge);

	rvt_put_ss(&qp->r_sge);

	if (clr_sends) {
		while (qp->s_last != qp->s_head) {
			struct rvt_swqe *wqe = rvt_get_swqe_ptr(qp, qp->s_last);
			unsigned i;

			for (i = 0; i < wqe->wr.num_sge; i++) {
				struct rvt_sge *sge = &wqe->sg_list[i];

				rvt_put_mr(sge->mr);
			}
			if (qp->ibqp.qp_type == IB_QPT_UD ||
			    qp->ibqp.qp_type == IB_QPT_SMI ||
			    qp->ibqp.qp_type == IB_QPT_GSI)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)
				atomic_dec(&ibah_to_rvtah(wqe->ud_wr.ah)->refcount);
#else
				atomic_dec(&ibah_to_rvtah(wqe->wr.wr.ud.ah)->refcount);
#endif
			if (++qp->s_last >= qp->s_size)
				qp->s_last = 0;
		}
		if (qp->s_rdma_mr) {
			rvt_put_mr(qp->s_rdma_mr);
			qp->s_rdma_mr = NULL;
		}
	}

	if (qp->ibqp.qp_type != IB_QPT_RC)
		return;

	for (n = 0; n < ARRAY_SIZE(qp->s_ack_queue); n++) {
		struct rvt_ack_entry *e = &qp->s_ack_queue[n];

		if (e->opcode == IB_OPCODE_RC_RDMA_READ_REQUEST &&
		    e->rdma_sge.mr) {
			rvt_put_mr(e->rdma_sge.mr);
			e->rdma_sge.mr = NULL;
		}
	}
}

/*
 * qp_set_16b - Determine and set the use_16b flag
 * @qp: queue pair pointer
 *
 * Set the use_16b flag if dgid is opa gid and dlid is extended,
 * or if the hop_limit is 1, or local port lid is extended.
 * Only applicable for RC and UC QPs.
 * UD QPs determine this on the fly from the ah in the wqe.
 */
static inline void qp_set_16b(struct rvt_qp *qp)
{
	struct hfi2_qp_priv *qp_priv = qp->priv;
	struct ib_device *ibdev;
	struct hfi2_ibport *ibp;
	union ib_gid *dgid;

	qp_priv->use_16b = false;
	if (!(qp->remote_ah_attr.ah_flags & IB_AH_GRH))
		return;

	ibdev = qp->ibqp.device;
	ibp = to_hfi_ibp(ibdev, qp->port_num);
	dgid = &qp->remote_ah_attr.grh.dgid;
	qp_priv->use_16b = (ibp->ppd->lid >= IB_MULTICAST_LID_BASE) ||
			   (qp->remote_ah_attr.grh.hop_limit == 1) ||
			   IS_EXT_LID(dgid);
}

/**
 * hfi2_modify_qp - modify the attributes of a queue pair
 * @ibqp: the queue pair who's attributes we're modifying
 * @attr: the new attributes
 * @attr_mask: the mask of attributes to modify
 * @udata: user data for libibverbs.so
 *
 * Return: 0 on success, otherwise returns an errno.
 */
int hfi2_modify_qp(struct ib_qp *ibqp, struct ib_qp_attr *attr,
		     int attr_mask, struct ib_udata *udata)
{
	struct rvt_qp *qp = ibqp_to_rvtqp(ibqp);
	struct ib_device *ibdev = ibqp->device;
	struct hfi2_ibdev *ibd = to_hfi_ibd(ibdev);
	enum ib_qp_state cur_state, new_state;
	struct ib_event ev;
	int lastwqe = 0;
	int mig = 0;
	int ret;
	u32 pmtu = 0; /* for gcc warning only */

	spin_lock_irq(&qp->r_lock);
	spin_lock(&qp->s_lock);

	cur_state = attr_mask & IB_QP_CUR_STATE ?
		attr->cur_qp_state : qp->state;
	new_state = attr_mask & IB_QP_STATE ? attr->qp_state : cur_state;

	if (!ib_modify_qp_is_ok(cur_state, new_state, ibqp->qp_type,
				attr_mask
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 3, 0)
				, IB_LINK_LAYER_UNSPECIFIED
#endif
				))
		goto inval;

	if (attr_mask & IB_QP_AV) {
		if (hfi2_check_mcast(&attr->ah_attr) ||
		    hfi2_check_permissive(&attr->ah_attr))
			goto inval;
		if (hfi2_check_ah(ibdev, &attr->ah_attr))
			goto inval;
	}

	if (attr_mask & IB_QP_ALT_PATH) {
		if (hfi2_check_mcast(&attr->alt_ah_attr) ||
		    hfi2_check_permissive(&attr->alt_ah_attr))
			goto inval;
		if (hfi2_check_ah(ibdev, &attr->alt_ah_attr))
			goto inval;
		if (attr->alt_pkey_index >= HFI_MAX_PKEYS)
			goto inval;
	}

	if (attr_mask & IB_QP_PKEY_INDEX)
		if (attr->pkey_index >= HFI_MAX_PKEYS)
			goto inval;

	if (attr_mask & IB_QP_MIN_RNR_TIMER)
		if (attr->min_rnr_timer > 31)
			goto inval;

	if (attr_mask & IB_QP_PORT)
		if (qp->ibqp.qp_type == IB_QPT_SMI ||
		    qp->ibqp.qp_type == IB_QPT_GSI ||
		    attr->port_num == 0 ||
		    attr->port_num > ibdev->phys_port_cnt)
			goto inval;

	if (attr_mask & IB_QP_DEST_QPN)
		if (attr->dest_qp_num > HFI1_QPN_MASK)
			goto inval;

	if (attr_mask & IB_QP_RETRY_CNT)
		if (attr->retry_cnt > 7)
			goto inval;

	if (attr_mask & IB_QP_RNR_RETRY)
		if (attr->rnr_retry > 7)
			goto inval;

	/*
	 * Don't allow invalid path_mtu values.  OK to set greater
	 * than the active mtu (or even the max_cap, if we have tuned
	 * that to a small mtu.  We'll set qp->path_mtu
	 * to the lesser of requested attribute mtu and active,
	 * for packetizing messages.
	 * Note that the QP port has to be set in INIT and MTU in RTR.
	 */
	if (attr_mask & IB_QP_PATH_MTU) {
		struct hfi2_ibport *ibp;
		struct hfi_pportdata *ppd;
		u16 mtu;

		ibp = to_hfi_ibp(ibdev, qp->port_num);
		ppd = ibp->ppd;

		mtu = opa_enum_to_mtu(attr->path_mtu);
		if (mtu == INVALID_MTU)
			goto inval;

		if (mtu > ppd->ibmtu)
			pmtu = opa_mtu_to_enum_safe(ppd->ibmtu, IB_MTU_2048);
		else
			pmtu = attr->path_mtu;
	}

	if (attr_mask & IB_QP_PATH_MIG_STATE) {
		if (attr->path_mig_state == IB_MIG_REARM) {
			if (qp->s_mig_state == IB_MIG_ARMED)
				goto inval;
			if (new_state != IB_QPS_RTS)
				goto inval;
		} else if (attr->path_mig_state == IB_MIG_MIGRATED) {
			if (qp->s_mig_state == IB_MIG_REARM)
				goto inval;
			if (new_state != IB_QPS_RTS && new_state != IB_QPS_SQD)
				goto inval;
			if (qp->s_mig_state == IB_MIG_ARMED)
				mig = 1;
		} else
			goto inval;
	}

	if (attr_mask & IB_QP_MAX_DEST_RD_ATOMIC)
		if (attr->max_dest_rd_atomic > OPA_IB_MAX_RDMA_ATOMIC)
			goto inval;

	switch (new_state) {
	case IB_QPS_RESET:
		if (qp->state != IB_QPS_RESET) {
			qp->state = IB_QPS_RESET;
			flush_iowait(qp);
			qp->s_flags &= ~(RVT_S_TIMER | RVT_S_ANY_WAIT);
			spin_unlock(&qp->s_lock);
			spin_unlock_irq(&qp->r_lock);
			/* Stop the sending work queue and retry timer */
			stop_send_queue(qp);
			quiesce_qp(qp);
			remove_qp(ibd, qp);
			wait_event(qp->wait, !atomic_read(&qp->refcount));
			spin_lock_irq(&qp->r_lock);
			spin_lock(&qp->s_lock);
			clear_mr_refs(qp, 1);
			reset_qp(qp, ibqp->qp_type);
		}
		break;

	case IB_QPS_RTR:
		/* Allow event to re-trigger if QP set to RTR more than once */
		qp->r_flags &= ~RVT_R_COMM_EST;
		qp->state = new_state;
		break;

	case IB_QPS_SQD:
		qp->s_draining = qp->s_last != qp->s_cur;
		qp->state = new_state;
		break;

	case IB_QPS_SQE:
		if (qp->ibqp.qp_type == IB_QPT_RC)
			goto inval;
		qp->state = new_state;
		break;

	case IB_QPS_ERR:
		lastwqe = rvt_error_qp(qp, IB_WC_WR_FLUSH_ERR);
		break;

	default:
		qp->state = new_state;
		break;
	}

	if (attr_mask & IB_QP_PKEY_INDEX)
		qp->s_pkey_index = attr->pkey_index;

	if (attr_mask & IB_QP_PORT)
		qp->port_num = attr->port_num;

	if (attr_mask & IB_QP_DEST_QPN)
		qp->remote_qpn = attr->dest_qp_num;

	if (attr_mask & IB_QP_SQ_PSN) {
		qp->s_next_psn = attr->sq_psn & PSN_MODIFY_MASK;
		qp->s_psn = qp->s_next_psn;
		qp->s_sending_psn = qp->s_next_psn;
		qp->s_last_psn = qp->s_next_psn - 1;
		qp->s_sending_hpsn = qp->s_last_psn;
	}

	if (attr_mask & IB_QP_RQ_PSN)
		qp->r_psn = attr->rq_psn & PSN_MODIFY_MASK;

	if (attr_mask & IB_QP_ACCESS_FLAGS)
		qp->qp_access_flags = attr->qp_access_flags;

	if (attr_mask & IB_QP_AV) {
		qp->remote_ah_attr = attr->ah_attr;
		qp->s_srate = attr->ah_attr.static_rate;
		qp->srate_mbps = ib_rate_to_mbps(qp->s_srate);
		qp_set_16b(qp);
	}

	if (attr_mask & IB_QP_ALT_PATH) {
		qp->alt_ah_attr = attr->alt_ah_attr;
		qp->s_alt_pkey_index = attr->alt_pkey_index;
	}

	if (attr_mask & IB_QP_PATH_MIG_STATE) {
		qp->s_mig_state = attr->path_mig_state;
		if (mig) {
			qp->remote_ah_attr = qp->alt_ah_attr;
			qp->port_num = qp->alt_ah_attr.port_num;
			qp->s_pkey_index = qp->s_alt_pkey_index;
			qp_set_16b(qp);
		}
	}

	if (attr_mask & IB_QP_PATH_MTU) {
		struct hfi2_ibport *ibp;
		struct hfi_pportdata *ppd;
		u8 sc, vl;
		u16 mtu;

		ibp = to_hfi_ibp(ibdev, qp->port_num);
		ppd = ibp->ppd;

		/*
		 * FXRTODO - this code was different with HFI1 driver as
		 * SC2VL was in equivalent of hfi2 driver, revisit later
		 */
		sc = ppd->sl_to_sc[qp->remote_ah_attr.sl];
		vl = ppd->sc_to_vlt[sc];

		mtu = opa_enum_to_mtu(pmtu);
		if (vl < ppd->vls_supported)
			mtu = min_t(u16, mtu, ppd->vl_mtu[vl]);
		pmtu = opa_mtu_to_enum_safe(mtu, OPA_MTU_8192);

		qp->path_mtu = pmtu;
		qp->pmtu = mtu;
	}

	if (attr_mask & IB_QP_RETRY_CNT) {
		qp->s_retry_cnt = attr->retry_cnt;
		qp->s_retry = attr->retry_cnt;
	}

	if (attr_mask & IB_QP_RNR_RETRY) {
		qp->s_rnr_retry_cnt = attr->rnr_retry;
		qp->s_rnr_retry = attr->rnr_retry;
	}

	if (attr_mask & IB_QP_MIN_RNR_TIMER)
		qp->r_min_rnr_timer = attr->min_rnr_timer;

	if (attr_mask & IB_QP_TIMEOUT) {
		qp->timeout = attr->timeout;
		qp->timeout_jiffies =
			usecs_to_jiffies((4096UL * (1UL << qp->timeout)) /
				1000UL);
	}

	if (attr_mask & IB_QP_QKEY)
		qp->qkey = attr->qkey;

	if (attr_mask & IB_QP_MAX_DEST_RD_ATOMIC)
		qp->r_max_rd_atomic = attr->max_dest_rd_atomic;

	if (attr_mask & IB_QP_MAX_QP_RD_ATOMIC)
		qp->s_max_rd_atomic = attr->max_rd_atomic;

	spin_unlock(&qp->s_lock);
	spin_unlock_irq(&qp->r_lock);

	if (cur_state == IB_QPS_RESET && new_state == IB_QPS_INIT) {
		ret = insert_qp(ibd, qp, udata != NULL);
		if (ret < 0) {
			/* revert QP back to RESET on error */
			spin_lock_irq(&qp->r_lock);
			spin_lock(&qp->s_lock);
			qp->state = cur_state;
			spin_unlock(&qp->s_lock);
			spin_unlock_irq(&qp->r_lock);
			reset_qp(qp, ibqp->qp_type);
			goto bail;
		}
	}

	if (lastwqe) {
		ev.device = qp->ibqp.device;
		ev.element.qp = &qp->ibqp;
		ev.event = IB_EVENT_QP_LAST_WQE_REACHED;
		qp->ibqp.event_handler(&ev, qp->ibqp.qp_context);
	}
	if (mig) {
		ev.device = qp->ibqp.device;
		ev.element.qp = &qp->ibqp;
		ev.event = IB_EVENT_PATH_MIG;
		qp->ibqp.event_handler(&ev, qp->ibqp.qp_context);
	}
	ret = 0;
	goto bail;

inval:
	spin_unlock(&qp->s_lock);
	spin_unlock_irq(&qp->r_lock);
	ret = -EINVAL;
bail:
	return ret;
}

/**
 * hfi2_query_qp - query the attributes of a queue pair
 * @ibqp: the queue pair who's attributes we want
 * @attr: requested QP attribute values returned here
 * @attr_mask: the mask of attributes to query
 * @init_attr: initialization defaults for QP attributes returned here
 *
 * Return: 0 on success and attributes returned in @attr, otherwise
 * returns an errno.
 */
int hfi2_query_qp(struct ib_qp *ibqp, struct ib_qp_attr *attr,
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
 * hfi2_create_qp - create a queue pair for a device
 * @ibpd: the protection domain who's device we create the queue pair for
 * @init_attr: the attributes of the queue pair
 * @udata: user data for libibverbs.so
 *
 * Called by the ib_create_qp() core verbs function.
 *
 * Return: the queue pair on success, otherwise returns an errno.
 */
struct ib_qp *hfi2_create_qp(struct ib_pd *ibpd,
			       struct ib_qp_init_attr *init_attr,
			       struct ib_udata *udata)
{
	struct rvt_qp *qp;
	int err;
	struct rvt_swqe *swq = NULL;
	struct hfi2_ibdev *ibd = to_hfi_ibd(ibpd->device);
	struct rvt_dev_info *rdi = &ibd->rdi;
	size_t sz;
	size_t sg_list_sz;
	struct ib_qp *ret;

	if (init_attr->cap.max_send_sge > hfi2_max_sges ||
	    init_attr->cap.max_send_wr > hfi2_max_qp_wrs ||
	    init_attr->create_flags) {
		ret = ERR_PTR(-EINVAL);
		goto bail;
	}

	/* Check receive queue parameters if no SRQ is specified. */
	if (!init_attr->srq) {
		if (init_attr->cap.max_recv_sge > hfi2_max_sges ||
		    init_attr->cap.max_recv_wr > hfi2_max_qp_wrs) {
			ret = ERR_PTR(-EINVAL);
			goto bail;
		}
		if (init_attr->cap.max_send_sge +
		    init_attr->cap.max_send_wr +
		    init_attr->cap.max_recv_sge +
		    init_attr->cap.max_recv_wr == 0) {
			ret = ERR_PTR(-EINVAL);
			goto bail;
		}
	}

	switch (init_attr->qp_type) {
	case IB_QPT_SMI:
	case IB_QPT_GSI:
		if (init_attr->port_num == 0 ||
		    init_attr->port_num > ibpd->device->phys_port_cnt) {
			ret = ERR_PTR(-EINVAL);
			goto bail;
		}
		break;
	case IB_QPT_UD:
	case IB_QPT_UC:
	case IB_QPT_RC:
		break;
	default:
		/* Don't support raw QPs */
		ret = ERR_PTR(-ENOSYS);
		goto bail;
	}

	sz = sizeof(struct rvt_sge) *
		init_attr->cap.max_send_sge +
		sizeof(struct rvt_swqe);
	swq = vzalloc((init_attr->cap.max_send_wr + 1) * sz);
	if (swq == NULL) {
		ret = ERR_PTR(-ENOMEM);
		goto bail;
	}

	sg_list_sz = 0;
	if (init_attr->srq) {
		struct rvt_srq *srq = ibsrq_to_rvtsrq(init_attr->srq);

		if (srq->rq.max_sge > 1)
			sg_list_sz = sizeof(*qp->r_sg_list) *
				     (srq->rq.max_sge - 1);
	} else if (init_attr->cap.max_recv_sge > 1)
		sg_list_sz = sizeof(*qp->r_sg_list) *
			     (init_attr->cap.max_recv_sge - 1);
	qp = kzalloc(sizeof(*qp) + sg_list_sz, GFP_KERNEL);
	if (!qp) {
		ret = ERR_PTR(-ENOMEM);
		goto bail_swq;
	}

	qp->priv = qp_priv_alloc(rdi, qp, GFP_KERNEL);
	if (!qp->priv) {
		ret = ERR_PTR(-ENOMEM);
		goto bail_qp;
	}
	qp->timeout_jiffies = usecs_to_jiffies((4096UL * (1UL << qp->timeout)) /
			      1000UL);

	if (init_attr->srq)
		sz = 0;
	else {
		qp->r_rq.size = init_attr->cap.max_recv_wr + 1;
		qp->r_rq.max_sge = init_attr->cap.max_recv_sge;
		sz = (sizeof(struct ib_sge) * qp->r_rq.max_sge) +
		     sizeof(struct rvt_rwqe);
		qp->r_rq.wq = vmalloc_user(sizeof(struct rvt_rwq) +
					   qp->r_rq.size * sz);
		if (!qp->r_rq.wq) {
			ret = ERR_PTR(-ENOMEM);
			goto bail_qp;
		}
	}

	/*
	 * ib_create_qp() will initialize qp->ibqp
	 * except for qp->ibqp.qp_num.
	 */
	spin_lock_init(&qp->r_lock);
	spin_lock_init(&qp->s_lock);
	spin_lock_init(&qp->r_rq.lock);
	atomic_set(&qp->refcount, 0);
	init_waitqueue_head(&qp->wait);
	init_timer(&qp->s_timer);
	qp->s_timer.data = (unsigned long)qp;
	INIT_LIST_HEAD(&qp->rspwait);
	qp->state = IB_QPS_RESET;
	qp->s_wq = swq;
	qp->s_size = init_attr->cap.max_send_wr + 1;
	qp->s_max_sge = init_attr->cap.max_send_sge;
	if (init_attr->sq_sig_type == IB_SIGNAL_REQ_WR)
		qp->s_flags = RVT_S_SIGNAL_REQ_WR;

	err = alloc_qpn(ibd, init_attr->qp_type, init_attr->port_num);
	if (err < 0) {
		ret = ERR_PTR(err);
		vfree(qp->r_rq.wq);
		goto bail_qp;
	}
	qp->ibqp.qp_num = err;
	qp->port_num = init_attr->port_num;
	reset_qp(qp, init_attr->qp_type);

	init_attr->cap.max_inline_data = 0;

	/*
	 * Return the address of the RWQ as the offset to mmap.
	 * See rvt_mmap() for details.
	 */
	if (udata && udata->outlen >= sizeof(__u64)) {
		if (!qp->r_rq.wq) {
			__u64 offset = 0;

			err = ib_copy_to_udata(udata, &offset,
					       sizeof(offset));
			if (err) {
				ret = ERR_PTR(err);
				goto bail_ip;
			}
		} else {
			u32 s = sizeof(struct rvt_rwq) + qp->r_rq.size * sz;

			qp->ip = rvt_create_mmap_info(&ibd->rdi, s,
						      ibpd->uobject->context,
						      qp->r_rq.wq);
			if (!qp->ip) {
				ret = ERR_PTR(-ENOMEM);
				goto bail_ip;
			}

			err = ib_copy_to_udata(udata, &(qp->ip->offset),
					       sizeof(qp->ip->offset));
			if (err) {
				ret = ERR_PTR(err);
				goto bail_ip;
			}
		}
	}

	spin_lock(&ibd->n_qps_lock);
	if (ibd->n_qps_allocated == hfi2_max_qps) {
		spin_unlock(&ibd->n_qps_lock);
		ret = ERR_PTR(-ENOMEM);
		goto bail_ip;
	}

	ibd->n_qps_allocated++;
	spin_unlock(&ibd->n_qps_lock);

	if (qp->ip) {
		spin_lock_irq(&ibd->rdi.pending_lock);
		list_add(&qp->ip->pending_mmaps, &ibd->rdi.pending_mmaps);
		spin_unlock_irq(&ibd->rdi.pending_lock);
	}

	ret = &qp->ibqp;

	/*
	 * We have our QP and its good, now keep track of what types of opcodes
	 * can be processed on this QP. We do this by keeping track of what the
	 * 3 high order bits of the opcode are.
	 */
	switch (init_attr->qp_type) {
	case IB_QPT_SMI:
	case IB_QPT_GSI:
	case IB_QPT_UD:
		qp->allowed_ops = IB_OPCODE_UD_SEND_ONLY & RVT_OPCODE_QP_MASK;
		break;
	case IB_QPT_RC:
		qp->allowed_ops = IB_OPCODE_RC_SEND_ONLY & RVT_OPCODE_QP_MASK;
		break;
	case IB_QPT_UC:
		qp->allowed_ops = IB_OPCODE_UC_SEND_ONLY & RVT_OPCODE_QP_MASK;
		break;
	default:
		ret = ERR_PTR(-EINVAL);
		goto bail_ip;
	}

	goto bail;

bail_ip:
	if (qp->ip)
		kref_put(&qp->ip->ref, rvt_release_mmap_info);
	else
		vfree(qp->r_rq.wq);
	free_qpn(ibd, qp);
bail_qp:
	qp_priv_free(rdi, qp);
	kfree(qp);
bail_swq:
	vfree(swq);
bail:
	return ret;
}

/**
 * hfi2_destroy_qp - destroy a queue pair
 * @ibqp: the queue pair to destroy
 *
 * Note, this can be called while the QP is actively sending or receiving!
 *
 * Return: 0 on success, otherwise returns an errno.
 */
int hfi2_destroy_qp(struct ib_qp *ibqp)
{
	struct rvt_qp *qp = ibqp_to_rvtqp(ibqp);
	struct hfi2_ibdev *ibd = to_hfi_ibd(ibqp->device);
	struct rvt_dev_info *rdi = &ibd->rdi;

	/* Make sure HW and driver activity is stopped. */
	spin_lock_irq(&qp->r_lock);
	spin_lock(&qp->s_lock);
	if (qp->state != IB_QPS_RESET) {
		qp->state = IB_QPS_RESET;
		flush_iowait(qp);
		qp->s_flags &= ~(RVT_S_TIMER | RVT_S_ANY_WAIT);
		spin_unlock(&qp->s_lock);
		spin_unlock_irq(&qp->r_lock);
		stop_send_queue(qp);
		quiesce_qp(qp);
		remove_qp(ibd, qp);
		wait_event(qp->wait, !atomic_read(&qp->refcount));
		spin_lock_irq(&qp->r_lock);
		spin_lock(&qp->s_lock);
		clear_mr_refs(qp, 1);
	}
	spin_unlock(&qp->s_lock);
	spin_unlock_irq(&qp->r_lock);

	/* all user's cleaned up, mark it available */
	free_qpn(ibd, qp);
	spin_lock(&ibd->n_qps_lock);
	ibd->n_qps_allocated--;
	spin_unlock(&ibd->n_qps_lock);

	if (qp->ip)
		kref_put(&qp->ip->ref, rvt_release_mmap_info);
	else
		vfree(qp->r_rq.wq);
	vfree(qp->s_wq);
	qp_priv_free(rdi, qp);
	kfree(qp);
	return 0;
}
