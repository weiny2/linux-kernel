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

static void flush_tx_list(struct rvt_qp *qp)
{
	/* FXRTODO SDMA -> TX_CQ */
}

static void flush_iowait(struct rvt_qp *qp)
{
	/* FXRTODO looks to be unneeded if we avoid adding a verbs_txreq */
}

void *qp_priv_alloc(struct rvt_dev_info *rdi, struct rvt_qp *qp, gfp_t gfp)
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

void qp_priv_free(struct rvt_dev_info *rdi, struct rvt_qp *qp)
{
	struct hfi2_qp_priv *priv = qp->priv;

	kfree(priv->s_hdr);
	kfree(priv);
}

/* FXRTODO - why can't this be merged into rvt_free_all_qps()? */
unsigned free_all_qps(struct rvt_dev_info *rdi)
{
	struct hfi2_ibdev *ibd = to_hfi_ibd(&rdi->ibdev);
	int n;
	unsigned qp_inuse = 0;

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
	flush_iowait(qp);
	/* FXRTODO: stop RC timers */
}

void stop_send_queue(struct rvt_qp *qp)
{
	struct hfi2_qp_priv *priv = qp->priv;

	cancel_work_sync(&priv->s_iowait.iowork);
	del_timer_sync(&qp->s_timer);
}

void quiesce_qp(struct rvt_qp *qp)
{
	struct hfi2_qp_priv *priv = qp->priv;
	struct hfi2_ibport *ibp;

	ibp = to_hfi_ibp(qp->ibqp.device, qp->port_num);
	iowait_sdma_drain(&priv->s_iowait);
	flush_tx_list(qp);
	hfi2_ctx_release_qp(ibp, priv);
}

void notify_qp_reset(struct rvt_qp *qp)
{
	struct hfi2_qp_priv *priv = qp->priv;

	iowait_init(&priv->s_iowait, 1, hfi2_do_send);

	/*
	 * pmtu unused for UD transport (unfragmented), so set to maximum here
	 * as logic to write DMA commands uses this
	 */
	qp->pmtu = opa_enum_to_mtu(OPA_MTU_10240);
}

int mtu_to_path_mtu(u32 mtu)
{
	return opa_mtu_to_enum_safe(mtu, OPA_MTU_8192);
}

u32 mtu_from_qp(struct rvt_dev_info *rdi, struct rvt_qp *qp, u32 pmtu)
{
	struct ib_device *ibdev = &rdi->ibdev;
	struct hfi2_ibport *ibp;
	struct hfi_pportdata *ppd;
	u8 sc, vl;
	u16 mtu;

	ibp = to_hfi_ibp(ibdev, qp->port_num);
	ppd = ibp->ppd;

	sc = ppd->sl_to_sc[qp->remote_ah_attr.sl];
	vl = ppd->sc_to_vlt[sc];
	mtu = opa_enum_to_mtu(pmtu);
	if (vl < ppd->vls_supported)
		mtu = min_t(u16, mtu, ppd->vl_mtu[vl]);
	return mtu;
}

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
	else if (mtu > ppd->ibmtu)
		return opa_mtu_to_enum_safe(ppd->ibmtu, IB_MTU_2048);
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

int hfi2_check_modify_qp(struct rvt_qp *qp, struct ib_qp_attr *attr,
			 int attr_mask, struct ib_udata *udata)
{
	if (attr_mask & IB_QP_AV) {
		if (hfi2_check_mcast(&attr->ah_attr) ||
		    hfi2_check_permissive(&attr->ah_attr))
			return -EINVAL;
	}

	if (attr_mask & IB_QP_ALT_PATH) {
		if (hfi2_check_mcast(&attr->alt_ah_attr) ||
		    hfi2_check_permissive(&attr->alt_ah_attr))
			return -EINVAL;
	}

	return 0;
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

void hfi2_modify_qp(struct rvt_qp *qp, struct ib_qp_attr *attr,
		    int attr_mask, struct ib_udata *udata)
{
	struct ib_device *ibdev = qp->ibqp.device;
	struct hfi2_ibport *ibp;

	if (attr_mask & IB_QP_AV) {
		qp_set_16b(qp);
		/* FXRTODO - can assign context based on QoS here */
	}

	if (attr_mask & IB_QP_PATH_MIG_STATE &&
	    attr->path_mig_state == IB_MIG_MIGRATED &&
	    qp->s_mig_state == IB_MIG_ARMED) {
		qp_set_16b(qp);
		/* FXRTODO - can assign context based on QoS here */
	}

	if (qp->state == IB_QPS_INIT) {
		ibp = to_hfi_ibp(ibdev, qp->port_num);
		hfi2_ctx_assign_qp(ibp, qp->priv, udata != NULL);
	}
}
