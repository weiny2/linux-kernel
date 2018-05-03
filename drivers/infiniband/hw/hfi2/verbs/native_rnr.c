/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2017 Intel Corporation.
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
 * Copyright (c) 2017 Intel Corporation.
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
 * Intel(R) OPA Gen2 Verbs over Native provider
 */
#include <linux/signal.h>
#include "hfi_kclient.h"
#include "hfi_ct.h"
#include "hfi_tx_base.h"
#include "hfi_tx_vostlnp.h"
#include "hfi_rx_vostlnp.h"
#include "native.h"

#define NATIVE_AUTH_IDX         0
#define NATIVE_NI               PTL_MATCHING_PHYSICAL
#define NATIVE_RC               0

/* SYNC_EQ HDR_DATA defines */
#define PID_EXCHANGE		0
#define PID_SYNC		1
#define RNR_TX_ENTER		2
#define RNR_RX_EXIT		3
#define RNR_TX_EXIT		4
#define FMT_SYNC_EQ_HD(opcode, ipid, src_qpn, dst_qpn)\
	((((u64)opcode & 0xf) << 60) |\
	(((u64)ipid & 0xfff) << 48) |\
	(((u64)src_qpn & 0xffffff) << 24) |\
	((dst_qpn & 0xffffff) << 0))
#define SYNC_EQ_OPCODE(hdr_data)	((hdr_data >> 60) & 0xf)
#define SYNC_EQ_IPID(hdr_data)		((hdr_data >> 48) & 0xfff)
#define SYNC_EQ_SRC_QPN(hdr_data)	((hdr_data >> 24) & 0xffffff)

static struct hfi_eq __attribute__((unused)) eq_handle_dummy = {
	.idx = PTL_EQ_NONE
};

#define BUILD_SYNC_EQ_CMD(qp, t, cmd)	\
	hfi_format_buff_sync_eq(((struct hfi_rq *)(qp)->r_rq.hw_rq)->hw_ctx, \
				NATIVE_NI, pt,\
				(qp)->remote_ah_attr.port_num - 1, \
				NATIVE_RC, (qp)->remote_ah_attr.sl, 0, \
				((struct hfi2_qp_priv *)(qp)->priv)->pkey, \
				rdma_ah_get_path_bits(&(qp)->remote_ah_attr), \
				NATIVE_AUTH_IDX, (hfi_user_ptr_t)NULL,\
				FMT_SYNC_EQ_HD(t,\
				((struct hfi_rq *)qp->r_rq.hw_rq)->hw_ctx->pid,\
				qp->ibqp.qp_num, qp->remote_qpn), 0,\
				&eq_handle_dummy, PTL_CT_NONE, \
				0, PTL_RC_EXCEPTION, \
				&(cmd)->buff_put_match)
#define PTL_EVENT_KIND_SHIFT       56
#define PTL_EVENT_KIND_MASK        0x7full
#define PTL_EVENT_KIND_SMASK (PTL_EVENT_KIND_MASK << PTL_EVENT_KIND_SHIFT)

static
int hfi_event_kind(u64 *eqe)
{
	return (*eqe & PTL_EVENT_KIND_SMASK) >> PTL_EVENT_KIND_SHIFT;
}

static inline
bool hfi2_recvq_root_in_flow_ctl(struct hfi_ibcontext *ctx, struct rvt_qp *qp)
{
	struct hfi_rq *rq = qp->r_rq.hw_rq;
	union meread_EQEntry meread = {.val[0] = 0};
	union hfi_rx_cq_command cmd __aligned(64);

	hfi_format_entry_read(rq->hw_ctx, NATIVE_NI, rq->recvq_root,
			      &cmd, (u64)&meread);

	hfi_rx_command(ctx->rx_cmdq, (u64 *)&cmd, sizeof(cmd.list) >> 6);

	hfi_eq_poll_cmd_complete(rq->hw_ctx, (u64 *)&meread);
	return (meread.next == 0);
}

static
int hfi2_enter_tx_flow_ctl(struct hfi_ibcontext *ctx, u64 *eq)
{
	union target_EQEntry *teq = (union target_EQEntry *)eq;
	union initiator_EQEntry *ieq;
	int qp_num = EXTRACT_HD_DST_QP(teq->hdr_data);
	struct rvt_qp *qp;
	union hfi_tx_cq_command cmd __aligned(64);
	int nslots, ret, i, cidx, outstanding_cmd = 0;
	bool dropped;
	u64 *eq_p = NULL;
	struct hfi_swqe *swqe;
	struct hfi2_ibdev *ibd = to_hfi_ibd(ctx->ibuc.device);
	struct hfi2_ibport *ibp = ibd->pport;
	struct hfi2_qp_priv *priv;
	struct rvt_cq *cq;
	int limit = 0;
	unsigned long flags;
	union hfi_process pt;

	rcu_read_lock();
	qp = rvt_lookup_qpn(&ibd->rdi, &ibp->rvp, qp_num);
	rcu_read_unlock();
	/* Bad QP */
	if (!qp)
		return 0;

	pt.phys.slid = rdma_ah_get_dlid(&qp->remote_ah_attr);
	pt.phys.ipid = PTL_PID_ANY;
	priv = (struct hfi2_qp_priv *)qp->priv;
	/* Check State */
	if (qp->state != IB_QPS_RTS)
		return 0;

	cq = ibcq_to_rvtcq(qp->ibqp.send_cq);
	spin_lock_irqsave(&cq->lock, flags);

	/* Lock QP  */
	spin_lock(&qp->s_lock);
	qp->s_flags |= RVT_S_WAIT_RNR;
	/* Calculate number of outstanding commands */
	limit = priv->current_cidx % qp->s_size;
	for (i = priv->current_eidx; i != limit;
	     i = (i + 1) % qp->s_size) {
		if (!QP_EQ_VALID(priv, i) && !(QP_EQ_FC_SEEN(priv, i)))
			++outstanding_cmd;
	}

	/* Walk EQs to find all commands that need to be retransmitted */
	for (i = 0; outstanding_cmd; ++i) {
		do {
			ret = hfi_eq_peek_nth((struct hfi_eq *)cq->hw_cq,
					      &eq_p, i,
					      &dropped);
			if (ret < 0) {
				spin_unlock(&qp->s_lock);
				spin_unlock_irqrestore(&cq->lock, flags);
				return ret;
			} else if (ret == HFI_EQ_EMPTY) {
				mdelay(10);
			}
		} while (ret == HFI_EQ_EMPTY);

		/* Skip RX EQs + TX EQs not associated with flow control QP */
		ieq = (union initiator_EQEntry *)eq_p;
		if (hfi_event_kind(eq_p) != NON_PTL_EVENT_VERBS_TX ||
		    EXTRACT_MB_SRC_QP(ieq->match_bits) != qp_num ||
		    EXTRACT_MB_OPCODE(ieq->match_bits) != 0)
			continue;

		--outstanding_cmd;

		/* Done with non-flow control EQs */
		if (ieq->fail_type != PTL_NI_PT_DISABLED)
			continue;

		swqe = (struct hfi_swqe *)ieq->user_ptr;
		if (IS_NON_IOVEC_SWQE(swqe))
			cidx = NON_IOVEC_SWQE_CIDX(swqe, qp);
		else
			cidx = swqe->cidx;

		/*
		 * Consume EQ (make this function + hfi2_process_tx_eq
		 * skip this EQ)
		 */
		ieq->match_bits = FMT_VOSTLNP_MB(MB_OC_QP_RESET, qp_num, 0);

		/* Enter flow control */
		if (!IS_FLOW_CTL(ctx->tx_qp_flow_ctl, qp_num)) {
			priv->fc_cidx = cidx;
			priv->fc_eidx = (cidx + 1) % (qp->s_size * 2);
			mutex_lock(&ctx->ctx_lock);
			SET_FLOW_CTL(ctx->tx_qp_flow_ctl, qp_num);
			mutex_unlock(&ctx->ctx_lock);
		} else if (IS_FLOW_CTL(ctx->tx_qp_flow_ctl, qp_num) &&
			   IS_CIDX_LESS_THAN(cidx, priv->fc_cidx, qp)) {
			priv->fc_cidx = cidx;
		} else if (IS_FLOW_CTL(ctx->tx_qp_flow_ctl, qp_num) &&
			   IS_CIDX_LESS_THAN(priv->fc_eidx,
					     (cidx + 1) % (qp->s_size * 2),
					     qp)) {
			priv->fc_eidx = (cidx + 1) % (qp->s_size * 2);
		}
	}
	spin_unlock(&qp->s_lock);
	spin_unlock_irqrestore(&cq->lock, flags);

	/* Signal target to exit flow control */
	nslots = BUILD_SYNC_EQ_CMD(qp, RNR_RX_EXIT, &cmd);
	hfi_tx_command(ctx->tx_cmdq, (u64 *)&cmd, nslots);

	return 0;
}

static
int hfi2_exit_rx_flow_ctl(struct hfi_ibcontext *ctx, u64 *eq)
{
	union target_EQEntry *teq = (union target_EQEntry *)eq;
	struct rvt_qp *qp;
	int ret, nslots;
	struct hfi2_ibdev *ibd = to_hfi_ibd(ctx->ibuc.device);
	struct hfi2_ibport *ibp = ibd->pport;
	struct hfi_rq *rq;
	union hfi_tx_cq_command cmd __aligned(64);
	int qp_num = EXTRACT_HD_DST_QP(teq->hdr_data);
	union hfi_process pt;
	u32 pd_handle;
	struct ib_qp *ibqp;

	rcu_read_lock();
	qp = rvt_lookup_qpn(&ibd->rdi, &ibp->rvp, qp_num);
	rcu_read_unlock();
	/* Bad QP */
	if (!qp)
		return 0;
	pt.phys.slid = rdma_ah_get_dlid(&qp->remote_ah_attr);
	pt.phys.ipid = PTL_PID_ANY;

	ibqp = &qp->ibqp;

	/* Check State */
	if (qp->state != IB_QPS_RTS)
		return 0;

	/*
	 *  Wait for buffers to be posted - TODO - Revisit this.
	 * If RECVQ_ROOT is still in flow control could be append this QP to a
	 * linked list in the RQ and when the next append happens the main
	 * thread updates the QP and signals back to initiator?
	 */
	while (hfi2_recvq_root_in_flow_ctl(ctx, qp) &&
	       qp->r_flags & RVT_S_WAIT_RNR)
		mdelay(5);

	/* TODO - how to set this? */
	pd_handle = ibqp->pd->uobject ?
		    ibqp->pd->uobject->user_handle : 0;
	rq = (struct hfi_rq *)qp->r_rq.hw_rq;
	ret = hfi_set_qp_state(ctx->rx_cmdq, rq, qp,
			       rdma_ah_get_dlid(&qp->remote_ah_attr),
			       PTL_PID_ANY, VERBS_OK, false);

	/* Signal initiator to exit flow control */
	nslots = BUILD_SYNC_EQ_CMD(qp, RNR_TX_EXIT, &cmd);
	hfi_tx_command(ctx->tx_cmdq, (u64 *)&cmd, nslots);

	return ret;
}

static
int hfi2_exit_tx_flow_ctl(struct hfi_ibcontext *ctx, u64 *eq)
{
	union target_EQEntry *teq = (union target_EQEntry *)eq;
	int qp_num = EXTRACT_HD_DST_QP(teq->hdr_data);
	union hfi_tx_cq_command *cmd;
	int nslots, cidx;
	struct rvt_qp *qp;
	struct hfi2_ibdev *ibd = to_hfi_ibd(ctx->ibuc.device);
	struct hfi2_ibport *ibp = ibd->pport;
	struct hfi2_qp_priv *priv;

	rcu_read_lock();
	qp = rvt_lookup_qpn(&ibd->rdi, &ibp->rvp, qp_num);
	rcu_read_unlock();
	/* Bad QP */
	if (!qp)
		return 0;
	priv = (struct hfi2_qp_priv *)qp->priv;
	/* Check State */
	if (qp->state != IB_QPS_RTS)
		return 0;

	/* Lock QP  */
	spin_lock(&qp->s_lock);

	cidx = priv->fc_cidx;
	/* Retransmit commands */
	do {
		int ret = 0;
		unsigned long flags;

		CLR_QP_FC_SEEN(priv, cidx, qp);
		cmd = &priv->cmd[cidx % qp->s_size];
		nslots = ((cmd->buff_put.flit0.a.cmd_length - 1) / 8) + 1;

		spin_lock_irqsave(&ctx->tx_cmdq->lock, flags);
		ret = hfi_tx_command(ctx->tx_cmdq, (u64 *)cmd, nslots);
		spin_unlock_irqrestore(&ctx->tx_cmdq->lock, flags);
		cidx = (cidx + 1) % (qp->s_size * 2);
	} while (cidx != priv->fc_eidx);

	/* Exit flow control */
	mutex_lock(&ctx->ctx_lock);
	CLEAR_FLOW_CTL(ctx->tx_qp_flow_ctl, qp_num);
	mutex_unlock(&ctx->ctx_lock);

	/* Unlock QP  */
	spin_unlock(&qp->s_lock);
	qp->s_flags &= ~RVT_S_WAIT_RNR;
	return 0;
}

static
int hfi2_enter_rx_flow_ctl(struct hfi_ibcontext *ctx, u64 *eq)
{
	union target_EQEntry *teq = (union target_EQEntry *)eq;
	int qp_num = EXTRACT_HD_DST_QP(teq->hdr_data);
	struct rvt_qp *qp;
	int nslots;
	union hfi_tx_cq_command cmd __aligned(64);
	struct hfi2_ibdev *ibd = to_hfi_ibd(ctx->ibuc.device);
	struct hfi2_ibport *ibp = ibd->pport;
	union hfi_process pt;

	rcu_read_lock();
	qp = rvt_lookup_qpn(&ibd->rdi, &ibp->rvp, qp_num);
	rcu_read_unlock();
	/* Bad QP */
	if (!qp)
		return 0;

	pt.phys.slid = rdma_ah_get_dlid(&qp->remote_ah_attr);
	pt.phys.ipid = PTL_PID_ANY;

	qp->r_flags |= RVT_S_WAIT_RNR;
	/* Signal initiator to enter flow control */
	nslots = BUILD_SYNC_EQ_CMD(qp, RNR_TX_ENTER, &cmd);
	hfi_tx_command(ctx->tx_cmdq, (u64 *)&cmd, nslots);

	return 0;
}

/*
 * Block for an EQ event interrupt, return value:
 * <0: on error, including dropped,
 *  0: no new event,
 *  1. get new event,
 */
static
int _hfi2_native_wait(struct hfi_ibcontext *ctx, u64 **rhf_entry)
{
	int rc;
	bool dropped = false;

	rc = hfi_eq_wait_irq(&ctx->hw_ctx->sync_eq, -1,
			     rhf_entry, &dropped);
	if (rc == -ETIME || rc == -ERESTARTSYS) {
		/* timeout or wait interrupted, not abnormal */
		rc = 0;
	} else if (rc > 0 && dropped) {
		/* driver bug with EQ sizing */
		rc = -EIO;
	}
	return rc;
}

int hfi2_qp_sync_thread(void *data)
{
	struct hfi_ibcontext *ctx = data;
	union target_EQEntry *teq;
	u64 *eq = NULL;
	int ret, ret2;

	allow_signal(SIGINT);
	while (!kthread_should_stop()) {
		ret = _hfi2_native_wait(ctx, &eq);
		if (ret < 0)
			return ret;
		else if (!ret)
			continue;

		ret2 = 0;
		teq = (union target_EQEntry *)eq;
		switch (hfi_event_kind(eq)) {
		case NON_PTL_EVENT_VERBS_TX:
			/*TODO Handle asynchronous events */
#if 0
			ret2 = hfi2_async_event(ctx, eq);
#endif
			break;
		case NON_PTL_EVENT_VERBS_RX:
			switch (SYNC_EQ_OPCODE(teq->hdr_data)) {
			case PID_EXCHANGE:
			case PID_SYNC:
			/*TODO Handle PID_EXCHANGE */
#if 0
				ret2 = hfi2_handle_pid_exchange(ctx, eq);
#endif
				break;
			case RNR_TX_ENTER:
				ret2 = hfi2_enter_tx_flow_ctl(ctx, eq);
				break;
			case RNR_RX_EXIT:
				ret2 = hfi2_exit_rx_flow_ctl(ctx, eq);
				break;
			case RNR_TX_EXIT:
				ret2 = hfi2_exit_tx_flow_ctl(ctx, eq);
				break;
			default:
				pr_err("unregisted event %d\n",
				       (int)SYNC_EQ_OPCODE(teq->hdr_data));
				break;
			}
			break;
		case PTL_EVENT_PT_DISABLED:
			ret2 = hfi2_enter_rx_flow_ctl(ctx, eq);
			break;
		default:
			break;
		}
		if (ret2 != 0)
			goto thread_exit;
		hfi_eq_advance(&ctx->hw_ctx->sync_eq, eq);
	}
	return 0;

thread_exit:
	pr_warn("%s exited with %d", __func__, ret2);
	return ret2;
}
