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

#include "verbs.h"
#include "native.h"
#include "packet.h"

/**
 * hfi2_qp_iter_cb - callback for iterator
 * @qp - the qp
 * @v - the sl in low bits of v
 *
 * This is called from the iterator callback to work
 * on an individual qp.
 */
static void hfi2_qp_iter_cb(struct rvt_qp *qp, u64 v)
{
	int lastwqe;
	struct ib_event ev;
	struct ib_device *ibdev;
	struct hfi2_ibport *ibp;
	struct hfi_pportdata *ppd;
	u8 sl = (u8)v;

	ibdev = qp->ibqp.device;
	ibp = to_hfi_ibp(ibdev, qp->port_num);
	ppd = ibp->ppd;

	if (qp->port_num != ppd->pnum ||
	    (qp->ibqp.qp_type != IB_QPT_UC &&
	     qp->ibqp.qp_type != IB_QPT_RC) ||
	    rdma_ah_get_sl(&qp->remote_ah_attr) != sl ||
	    !(ib_rvt_state_ops[qp->state] & RVT_POST_SEND_OK))
		return;

	spin_lock_irq(&qp->r_lock);
	spin_lock(&qp->s_hlock);
	spin_lock(&qp->s_lock);
	lastwqe = rvt_error_qp(qp, IB_WC_WR_FLUSH_ERR);
	spin_unlock(&qp->s_lock);
	spin_unlock(&qp->s_hlock);
	spin_unlock_irq(&qp->r_lock);
	if (lastwqe) {
		ev.device = qp->ibqp.device;
		ev.element.qp = &qp->ibqp;
		ev.event = IB_EVENT_QP_LAST_WQE_REACHED;
		qp->ibqp.event_handler(&ev, qp->ibqp.qp_context);
	}
}

/*
 * qp_set_16b - Set the hdr_type based on whether the slid or the
 * dlid in the connection is extended. Only applicable for RC and UC
 * QPs. UD QPs determine this on the fly from the ah in the wqe
 */
static inline void qp_set_16b(struct rvt_qp *qp)
{
	struct hfi2_qp_priv *qp_priv = qp->priv;
	struct ib_device *ibdev;
	struct hfi2_ibport *ibp;

	/* Update ah_attr to account for extended lids */
	hfi2_update_ah_attr(qp->ibqp.device, &qp->remote_ah_attr);

	/* Create 32 bit lids */
	hfi2_make_opa_lid(&qp->remote_ah_attr);

	if (!(rdma_ah_get_ah_flags(&qp->remote_ah_attr) & IB_AH_GRH))
		return;

	ibdev = qp->ibqp.device;
	ibp = to_hfi_ibp(ibdev, qp->port_num);
	qp_priv->hdr_type = hfi2_get_hdr_type(ibp->ppd->lid,
					      &qp->remote_ah_attr);
}

static void flush_tx_list(struct rvt_qp *qp)
{
	/* FXRTODO SDMA -> TX_CQ */
}

static void flush_iowait(struct rvt_qp *qp)
{
	/* FXRTODO looks to be unneeded if we avoid adding a verbs_txreq */
}

int hfi2_check_modify_qp(struct rvt_qp *qp, struct ib_qp_attr *attr,
			 int attr_mask, struct ib_udata *udata)
{
	return 0;
}

void hfi2_modify_qp(struct rvt_qp *qp, struct ib_qp_attr *attr,
		    int attr_mask, struct ib_udata *udata)
{
	struct hfi2_ibdev *ibd = to_hfi_ibd(qp->ibqp.device);
#ifdef CONFIG_HFI2_STLNP
	int ret;
#endif

	if (attr_mask & IB_QP_AV)
		qp_set_16b(qp);

	if (attr_mask & IB_QP_PATH_MIG_STATE &&
	    attr->path_mig_state == IB_MIG_MIGRATED &&
	    qp->s_mig_state == IB_MIG_ARMED)
		qp_set_16b(qp);

	if (qp->state == IB_QPS_INIT)
		hfi2_ctx_assign_qp(qp->priv, udata);

	if ((attr_mask & IB_QP_TIMEOUT) && (simics || ibd->dd->emulation) &&
	    !ibd->rc_drop_enabled) {
		/*
		 * increase the timeout value to maximum allowed, accommodate
		 * for slower processing of acks on simics during heavy load
		 */
		qp->timeout = HFI2_QP_MAX_TIMEOUT;
		qp->timeout_jiffies = rvt_timeout_to_jiffies(qp->timeout);
	}

#ifdef CONFIG_HFI2_STLNP
	/* attempt to setup for native transport */
	ret = hfi2_native_modify_qp(qp, attr, attr_mask, udata);
	/*
	 * TODO do we silently remain in legacy QP mode if some failure is
	 * encountered?  This is protoype, so display warning for now.
	 */
	WARN_ON(ret != 0);
#endif
}

/*
 * hfi2_check_send_wqe - validate wqe
 * @qp - The qp
 * @wqe - The built wqe
 *
 * validate wqe.  This is called
 * prior to inserting the wqe into
 * the ring but after the wqe has been
 * setup.
 * Returns 0 or 1 on success, -EINVAL on failure
 */
int hfi2_check_send_wqe(struct rvt_qp *qp,
			struct rvt_swqe *wqe)
{
	struct hfi2_ibport *ibp = to_hfi_ibp(qp->ibqp.device, qp->port_num);
	struct rvt_ah *ah;

	switch (qp->ibqp.qp_type) {
	case IB_QPT_RC:
	case IB_QPT_UC:
		if (wqe->length > 0x80000000U)
			return -EINVAL;
		break;
	case IB_QPT_SMI:
		/* test length against VL15 MTU */
		if (wqe->length > ibp->ppd->vl_mtu[15])
			return -EINVAL;
		break;
	case IB_QPT_GSI:
	case IB_QPT_UD:
		ah = ibah_to_rvtah(wqe->ud_wr.ah);
		if (wqe->length > (1 << ah->log_pmtu))
			return -EINVAL;
		if (ibp->ppd->sl_to_sc[rdma_ah_get_sl(&ah->attr)] == 0xf)
			return -EINVAL;
	default:
		break;
	}

	/*
	 * Returning 1 instructs rdmavt to optimistically call hfi2 to
	 * send the packet via our do_send() callback.
	 * Returning 0 would instruct rdmavt to pessimisticaly schedule
	 * our worker thread (except for the case when queueing single WR
	 * to an empty queue). But we already make our own determination
	 * on when to schedule a worker thread in our do_send() callback.
	 *
	 * hfi1 would make a determination here based on PIO threshold,
	 * but hfi2 hardware has lightweight DMA programming allowing us
	 * to always return 1 and prefer rdmavt to call do_send().
	 */
	return 1;
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

void qp_iter_print(struct seq_file *s, struct rvt_qp_iter *iter)
{
	struct rvt_swqe *wqe;
	struct rvt_qp *qp = iter->qp;
	struct hfi2_qp_priv *priv = qp->priv;

	wqe = rvt_get_swqe_ptr(qp, qp->s_last);
	seq_printf(s,
		   "N %d PN %d %s QP %x R %u %s %u %u %u f=%x %u %u %u %u %u %u PSN %x %x %x %x %x (%u %u %u %u %u %u %u) RQP %x LID %x SL %u MTU %u %u %u %u RC %u SC %u SCQ %u %u PID %d\n",
		   iter->n,
		   qp->port_num,
		   qp_idle(qp) ? "I" : "B",
		   qp->ibqp.qp_num,
		   atomic_read(&qp->refcount),
		   qp_type_str[qp->ibqp.qp_type],
		   qp->state,
		   wqe ? wqe->wr.opcode : 0,
		   qp->s_hdrwords,
		   qp->s_flags,
		   iowait_sdma_pending(&priv->s_iowait),
		   0,	// TODO: iowait_pio_pending(&priv->s_iowait),
		   0,	// TODO: !list_empty(&priv->s_iowait.list),
		   qp->timeout,
		   wqe ? wqe->ssn : 0,
		   qp->s_lsn,
		   qp->s_last_psn,
		   qp->s_psn, qp->s_next_psn,
		   qp->s_sending_psn, qp->s_sending_hpsn,
		   qp->s_last, qp->s_acked, qp->s_cur,
		   qp->s_tail, qp->s_head, qp->s_size,
		   qp->s_avail,
		   qp->remote_qpn,
		   rdma_ah_get_dlid(&qp->remote_ah_attr),
		   rdma_ah_get_sl(&qp->remote_ah_attr),
		   qp->pmtu,
		   qp->s_retry,
		   qp->s_retry_cnt,
		   qp->s_rnr_retry_cnt,
		   priv->ibrcv ? priv->ibrcv->ctx->pid : 0,
		   priv->s_ctx ? priv->s_ctx->ctx->pid : 0,
		   ibcq_to_rvtcq(qp->ibqp.send_cq)->queue->head,
		   ibcq_to_rvtcq(qp->ibqp.send_cq)->queue->tail,
		   qp->pid);
}

void *qp_priv_alloc(struct rvt_dev_info *rdi, struct rvt_qp *qp)
{
	struct hfi2_qp_priv *priv;

	priv = kzalloc_node(sizeof(*priv), GFP_KERNEL, rdi->dparms.node);
	if (!priv)
		return ERR_PTR(-ENOMEM);
	priv->owner = qp;

	priv->s_hdr = kzalloc_node(sizeof(*priv->s_hdr), GFP_KERNEL,
				   rdi->dparms.node);
	if (!priv->s_hdr) {
		kfree(priv);
		return ERR_PTR(-ENOMEM);
	}
	qp->priv = priv;
	iowait_init(&priv->s_iowait, 1, hfi2_do_work);
	INIT_LIST_HEAD(&priv->poll_qp);
	INIT_LIST_HEAD(&priv->send_qp_ll);
	INIT_LIST_HEAD(&priv->recv_qp_ll);
	spin_lock_init(&priv->s_lock);
	init_completion(&priv->pid_xchg_comp);
	notify_qp_reset(qp);
	return priv;
}

void qp_priv_free(struct rvt_dev_info *rdi, struct rvt_qp *qp)
{
	struct hfi2_qp_priv *priv = qp->priv;

	kfree(priv->s_hdr);
	vfree(priv->cmd);
	vfree(priv->wc);
	kfree(priv);
}

/**
 * set_middle_length - stores the total payload size of all middle packets
 * @qp: a pointer to the QP
 * @wqe: current send wqe
 */
void hfi_set_middle_length(struct rvt_qp *qp, u32 bth2, bool is_middle)
{
	struct hfi2_qp_priv *qp_priv = qp->priv;
	struct rvt_swqe *wqe = qp->s_wqe;
	struct rvt_sge_state *ss = qp->s_cur_sge;
	struct rvt_sge *sge = &ss->sge;

	/**
	 * OFED dma requires data in single contiguous virtual memory
	 * region to construct middle packets of size pmtu, when size
	 * of data to be transferred is > pmtu. To effectively utilize
	 * the HW we coalesce the sge's into single sge when they are
	 * virtually contiguous and have same lkey, thereby use ofed dma
	 * to send all middle packets. The condition len > pmtu and num_sge
	 * 1 indicate that we have single contiguous memory to transmit.
	 */
	if (is_middle && wqe->wr.num_sge == 1 &&
	    sge->mr->lkey == 0) {
		bool req_ack;
		u32 n_middles;

		n_middles = delta_psn(wqe->lpsn, wqe->psn) - 1;
		qp_priv->middle_len = n_middles << qp->log_pmtu;
		qp_priv->lpsn = wqe->lpsn;
		/*
		 * This BTH2 value is used in send_event() and passed
		 * into rc_send_complete().  We need REQ_ACK bit set
		 * if this series of Middles will generate an AETH packet.
		 * rc_send_complete() will then start the RC timer.
		 */
		req_ack = (n_middles + 1) >= HFI2_PSN_CREDIT;
		qp_priv->bth2 = req_ack ? (IB_BTH_REQ_ACK | bth2) : bth2;
	} else {
		qp_priv->middle_len = 0;
		qp_priv->bth2 = bth2;
	}
}

/* FXRTODO - why can't this be merged into rvt_free_all_qps()? */
unsigned int free_all_qps(struct rvt_dev_info *rdi)
{
	struct hfi2_ibdev *ibd = to_hfi_ibd(&rdi->ibdev);
	int n;
	unsigned int qp_inuse = 0;

	for (n = 0; n < ibd->num_pports; n++) {
		struct hfi2_ibport *ibp = &ibd->pport[n];

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
	lockdep_assert_held(&qp->s_lock);
	flush_iowait(qp);
}

void stop_send_queue(struct rvt_qp *qp)
{
	struct hfi2_qp_priv *priv = qp->priv;

	cancel_work_sync(&priv->s_iowait.iowork);
	rvt_del_timers_sync(qp);
}

void quiesce_qp(struct rvt_qp *qp)
{
	struct hfi2_qp_priv *priv = qp->priv;

	iowait_sdma_drain(&priv->s_iowait);
	flush_tx_list(qp);
}

void notify_qp_reset(struct rvt_qp *qp)
{
	struct hfi2_qp_priv *priv;

	priv = qp->priv;
	qp->r_adefered = 0;
	hfi2_ctx_release_qp(priv);
#ifdef CONFIG_HFI2_STLNP
	hfi2_native_reset_qp(qp);
#endif
	/*
	 * pmtu unused for UD transport (unfragmented), so set to maximum here
	 * as logic to write DMA commands uses this.
	 * Further restrict as rdmavt requires MTU to be power of 2.
	 */
	qp->pmtu = HFI_VERBS_MAX_MTU;
	WARN_ON(!is_power_of_2(qp->pmtu));
	qp->log_pmtu = ilog2(qp->pmtu);
}

/*
 * Converts the MTU (in bytes) to MTU enum which is returned in QUERY_QP.
 */
int mtu_to_path_mtu(u32 mtu)
{
	/* return MTU of 0 if we were given garbage input */
	return opa_mtu_to_enum_safe(mtu, OPA_MTU_0);
}

u16 mtu_from_sl(struct hfi2_ibport *ibp, u8 sl)
{
	struct hfi_pportdata *ppd = ibp->ppd;
	u8 sc, vl;
	u16 mtu = 0;

	sc = ppd->sl_to_sc[sl];
	vl = ppd->sc_to_vlt[sc];
	if (vl < ppd->vls_supported)
		mtu = ppd->vl_mtu[vl];
	return mtu;
}

/*
 * Takes the MTU returned from get_pmtu_from_attr() and returns (in bytes)
 * the smaller of that MTU or the VL-specific MTU.
 */
u32 mtu_from_qp(struct rvt_dev_info *rdi, struct rvt_qp *qp, u32 pmtu)
{
	struct ib_device *ibdev = &rdi->ibdev;
	struct hfi2_ibport *ibp;
	u8 sl;
	u16 mtu;

	ibp = to_hfi_ibp(ibdev, qp->port_num);
	sl = rdma_ah_get_sl(&qp->remote_ah_attr);

	mtu = mtu_from_sl(ibp, sl);
	if (mtu)
		mtu = min_t(u16, mtu, opa_enum_to_mtu(pmtu));
	else
		mtu = opa_enum_to_mtu(pmtu);
	/* here we may further restrict due to software limitations */
	mtu = min_t(u16, mtu, HFI_VERBS_MAX_MTU);
	return mtu;
}

/*
 * Takes the MTU requested by user and returns the smaller of that MTU
 * or the max active MTU (across all VLs).  A later callback will determine
 * if the VL for the packet has even smaller MTU.
 */
int get_pmtu_from_attr(struct rvt_dev_info *rdi, struct rvt_qp *qp,
		       struct ib_qp_attr *attr)
{
	struct ib_device *ibdev = &rdi->ibdev;
	struct hfi2_ibport *ibp;
	struct hfi_pportdata *ppd;
	u16 mtu;

	ibp = to_hfi_ibp(ibdev, qp->port_num);
	ppd = ibp->ppd;

	mtu = opa_enum_to_mtu(attr->path_mtu);
	if (mtu == INVALID_MTU)
		return -1; /* values less than 0 are error */
	else if (mtu > ppd->max_active_mtu)
		return opa_mtu_to_enum(ppd->max_active_mtu);
	else
		return attr->path_mtu;
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
 * hfi2_error_port_qps - put a port's RC/UC qps into error state
 * @ppd: perport data for a port.
 * @sl: the service level.
 *
 * This function places all RC/UC qps with a given service level into error
 * state. It is generally called to force upper lay apps to abandon stale qps
 * after an sl->sc mapping change.
 */
void hfi2_error_port_qps(struct hfi_pportdata *ppd, u8 sl)
{
	struct hfi_devdata *dd = ppd->dd;
	struct hfi2_ibdev *dev = dd->ibd;

	rvt_qp_iter(&dev->rdi, sl, hfi2_qp_iter_cb);
}
