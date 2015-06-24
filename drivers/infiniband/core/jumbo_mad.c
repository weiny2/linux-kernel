/*
 * Copyright (c) 2013 Intel Corporation.  All rights reserved.
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
 *
 */


#include <linux/slab.h>

#include "mad_priv.h"
#include "stl_smi.h"
#include "jumbo_mad.h"
#include "jumbo_mad_rmpp.h"
#include "agent.h"


/** =========================================================================
 * The send side of 2k support is easier than the recv side.
 * This is because we can allow the current callers of this function to use
 * their previous lengths.
 *
 * The recv side however needs to either always allocate jumbo mad buffers
 * which the QP may or may not accept or have some decision on the card type so
 * that the STL card will always allocate jumbo's.  This is hard because on the
 * recv side we are called to "free" mads from many places and I am not sure if
 * I will know if the free is on a jumbo mad or not.
 */



static enum smi_action
handle_stl_smi(struct ib_mad_port_private *port_priv,
	       struct ib_mad_qp_info *qp_info,
	       struct ib_wc *wc,
	       int port_num,
	       struct jumbo_mad_private *recv,
	       struct jumbo_mad_private *response)
{
	enum smi_forward_action retsmi;

	if (stl_smi_handle_dr_smp_recv(&recv->mad.smp,
				   port_priv->device->node_type,
				   port_num,
				   port_priv->device->phys_port_cnt) ==
				   IB_SMI_DISCARD)
		return (IB_SMI_DISCARD);

	retsmi = stl_smi_check_forward_dr_smp(&recv->mad.smp);
	if (retsmi == IB_SMI_LOCAL)
		return (IB_SMI_HANDLE);

	if (retsmi == IB_SMI_SEND) { /* don't forward */
		if (stl_smi_handle_dr_smp_send(&recv->mad.smp,
					   port_priv->device->node_type,
					   port_num) == IB_SMI_DISCARD)
			return (IB_SMI_DISCARD);

		if (stl_smi_check_local_smp(&recv->mad.smp, port_priv->device) == IB_SMI_DISCARD)
			return (IB_SMI_DISCARD);

	}
#if 0
STL does not yet run Linux in the Switch
 else if (port_priv->device->node_type == RDMA_NODE_IB_SWITCH) {
		/* forward case for switches */
		memcpy(response, recv, sizeof(*response));
		response->header.recv_wc.wc = &response->header.wc;
		response->header.recv_wc.recv_buf.mad = (struct ib_mad *)&response->mad.mad;
		response->header.recv_wc.recv_buf.grh = &response->grh;

		agent_send_response((struct ib_mad *)&response->mad.mad,
				    &response->grh, wc,
				    port_priv->device,
				    stl_smi_get_fwd_port(&recv->mad.smp),
				    qp_info->qp->qp_num);

		return (IB_SMI_DISCARD);
	}
#endif

	return (IB_SMI_HANDLE);
}

static enum smi_action
jumbo_handle_smi(struct ib_mad_port_private *port_priv,
		 struct ib_mad_qp_info *qp_info,
		 struct ib_wc *wc,
		 int port_num,
		 struct jumbo_mad_private *recv,
		 struct jumbo_mad_private *response)
{
	if (recv->mad.mad.mad_hdr.base_version == JUMBO_MGMT_BASE_VERSION) {
		switch (recv->mad.mad.mad_hdr.class_version) {
			case STL_SMI_CLASS_VERSION:
				return handle_stl_smi(port_priv, qp_info, wc, port_num, recv, response);
			/* stub for other Jumbo SMI versions */
		}
	}

	return handle_ib_smi(port_priv, qp_info, wc, port_num,
			     (struct ib_mad_private *)recv,
			     (struct ib_mad_private *)response);
}

static void jumbo_mad_complete_recv(struct ib_mad_agent_private *mad_agent_priv,
				 struct ib_mad_recv_wc *mad_recv_wc)
{
	struct ib_mad_send_wr_private *mad_send_wr;
	struct ib_mad_send_wc mad_send_wc;
	unsigned long flags;

	INIT_LIST_HEAD(&mad_recv_wc->rmpp_list);
	list_add(&mad_recv_wc->recv_buf.list, &mad_recv_wc->rmpp_list);
	if (kernel_rmpp_agent(&mad_agent_priv->agent)) {
		mad_recv_wc = jumbo_process_rmpp_recv_wc(mad_agent_priv,
						      mad_recv_wc);
		if (!mad_recv_wc) {
			deref_mad_agent(mad_agent_priv);
			return;
		}
	}

	/* Complete corresponding request */
	if (ib_response_mad(mad_recv_wc->recv_buf.mad)) {
		spin_lock_irqsave(&mad_agent_priv->lock, flags);
		mad_send_wr = ib_find_send_mad(mad_agent_priv, mad_recv_wc);
		if (!mad_send_wr) {
			spin_unlock_irqrestore(&mad_agent_priv->lock, flags);
			if (mad_agent_priv->agent.flags & IB_MAD_USER_RMPP
			   && ib_is_mad_class_rmpp(mad_recv_wc->recv_buf.mad->mad_hdr.mgmt_class)
			   && (ib_get_rmpp_flags(&((struct jumbo_rmpp_mad *)mad_recv_wc->recv_buf.mad)->rmpp_hdr)
					& IB_MGMT_RMPP_FLAG_ACTIVE)) {
				// user rmpp is in effect
				mad_recv_wc->wc->wr_id = 0;
				mad_agent_priv->agent.recv_handler(&mad_agent_priv->agent,
								   mad_recv_wc);
				atomic_dec(&mad_agent_priv->refcount);
			} else {
				// not user rmpp, revert to normal behavior and drop the mad
				ib_free_recv_mad(mad_recv_wc);
				deref_mad_agent(mad_agent_priv);
				return;
			}
		} else {
			ib_mark_mad_done(mad_send_wr);
			spin_unlock_irqrestore(&mad_agent_priv->lock, flags);

			/* Defined behavior is to complete response before request */
			mad_recv_wc->wc->wr_id = (unsigned long) &mad_send_wr->send_buf;
			mad_agent_priv->agent.recv_handler(&mad_agent_priv->agent,
							   mad_recv_wc);
			atomic_dec(&mad_agent_priv->refcount);

			mad_send_wc.status = IB_WC_SUCCESS;
			mad_send_wc.vendor_err = 0;
			mad_send_wc.send_buf = &mad_send_wr->send_buf;
			ib_mad_complete_send_wr(mad_send_wr, &mad_send_wc);
		}
	} else {
		mad_agent_priv->agent.recv_handler(&mad_agent_priv->agent,
						   mad_recv_wc);
		deref_mad_agent(mad_agent_priv);
	}
}

static int validate_jumbo_mad(struct jumbo_mad *mad, u32 qp_num)
{
	int valid = 0;

	/* MAD version can be IB or JUMBO */
	if (mad->mad_hdr.base_version != JUMBO_MGMT_BASE_VERSION
	    && mad->mad_hdr.base_version != IB_MGMT_BASE_VERSION) {
		printk(KERN_ERR PFX "Jumbo MAD received with unsupported base "
		       "version %d\n", mad->mad_hdr.base_version);
		goto out;
	}

	/* Filter SMI packets sent to other than QP0 */
	if ((mad->mad_hdr.mgmt_class == IB_MGMT_CLASS_SUBN_LID_ROUTED) ||
	    (mad->mad_hdr.mgmt_class == IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE)) {
		if (qp_num == 0)
			valid = 1;
	} else {
		/* Filter GSI packets sent to QP0 */
		if (qp_num != 0)
			valid = 1;
	}

out:
	return valid;
}

static bool generate_unmatched_resp(struct jumbo_mad_private *recv,
				    struct jumbo_mad_private *response)
{
	if (recv->mad.mad.mad_hdr.method == IB_MGMT_METHOD_GET ||
	    recv->mad.mad.mad_hdr.method == IB_MGMT_METHOD_SET) {
		memcpy(response, recv, sizeof *response);
		response->header.recv_wc.wc = &response->header.wc;
		response->header.recv_wc.recv_buf.mad = (struct ib_mad *)&response->mad.mad;
		response->header.recv_wc.recv_buf.grh = &response->grh;
		response->mad.mad.mad_hdr.method = IB_MGMT_METHOD_GET_RESP;
		response->mad.mad.mad_hdr.status =
			cpu_to_be16(IB_MGMT_MAD_STATUS_UNSUPPORTED_METHOD_ATTRIB);
		if (recv->mad.mad.mad_hdr.mgmt_class == IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE)
			response->mad.mad.mad_hdr.status |= IB_SMP_DIRECTION;

		return true;
	} else {
		return false;
	}
}

/**
 * NOTE: Processing of recv jumbo MADs is kept separate for buffer handling
 * however, incomming MAD's may not be jumbo.
 */
void ib_mad_recv_done_jumbo_handler(struct ib_mad_port_private *port_priv,
				    struct ib_wc *wc,
				    struct ib_mad_private_header *mad_priv_hdr,
				    struct ib_mad_qp_info *qp_info)
{
	struct jumbo_mad_private *recv, *response = NULL;
	struct ib_mad_agent_private *mad_agent;
	int port_num;
	int ret = IB_MAD_RESULT_SUCCESS;
	u8 base_version;

	recv = container_of(mad_priv_hdr, struct jumbo_mad_private, header);
	ib_dma_unmap_single(port_priv->device,
			    recv->header.mapping,
			    sizeof(struct jumbo_mad_private) -
			      sizeof(struct ib_mad_private_header),
			    DMA_FROM_DEVICE);

	/* Setup MAD receive work completion from "normal" work completion */
	recv->header.wc = *wc;
	recv->header.recv_wc.wc = &recv->header.wc;
	base_version = recv->mad.mad.mad_hdr.base_version;
	if (base_version == JUMBO_MGMT_BASE_VERSION)
		recv->header.recv_wc.mad_len = sizeof(struct jumbo_mad);
	else
		recv->header.recv_wc.mad_len = sizeof(struct ib_mad);
	recv->header.recv_wc.recv_buf.mad = (struct ib_mad *)&recv->mad.mad;
	recv->header.recv_wc.recv_buf.grh = &recv->grh;

	if (atomic_read(&qp_info->snoop_count))
		snoop_recv(qp_info, &recv->header.recv_wc, IB_MAD_SNOOP_RECVS);

	if (!validate_jumbo_mad((struct jumbo_mad *)&recv->mad.mad, qp_info->qp->qp_num))
		goto out;

	response = kmem_cache_alloc(jumbo_mad_cache, GFP_KERNEL);
	if (!response) {
		printk(KERN_ERR PFX "ib_mad_recv_done_jumbo_handler no memory "
		       "for response buffer (jumbo)\n");
		goto out;
	}
	response->header.flags = IB_MAD_PRIV_FLAG_JUMBO;

	if (port_priv->device->node_type == RDMA_NODE_IB_SWITCH)
		port_num = wc->port_num;
	else
		port_num = port_priv->port_num;

	if (recv->mad.mad.mad_hdr.mgmt_class ==
	    IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE) {
		if (jumbo_handle_smi(port_priv, qp_info, wc, port_num, recv, response)
		    == IB_SMI_DISCARD)
			goto out;
	}

	/* Give driver "right of first refusal" on incoming MAD */
	if (port_priv->device->process_mad) {
		/* FIXME for upstream:
		 * Drivers which support jumbo mads know we are actually
		 * passing them a jumbo_mad type through the ib_mad parameter
		 */
		ret = port_priv->device->process_mad(port_priv->device, 0,
						     port_priv->port_num,
						     wc, &recv->grh,
						     (struct ib_mad *)&recv->mad.mad,
						     (struct ib_mad *)&response->mad.mad);
		if (ret & IB_MAD_RESULT_SUCCESS) {
			if (ret & IB_MAD_RESULT_CONSUMED)
				goto out;
			if (ret & IB_MAD_RESULT_REPLY) {
				agent_send_jumbo_response(&response->mad.mad,
						    &recv->grh, wc,
						    port_priv->device,
						    port_num,
						    qp_info->qp->qp_num);
				goto out;
			}
		}
	}

	mad_agent = find_mad_agent(port_priv, (struct ib_mad *)&recv->mad.mad);
	if (mad_agent) {
		jumbo_mad_complete_recv(mad_agent, &recv->header.recv_wc);
		/*
		 * recv is freed up in error cases in ib_mad_complete_recv
		 * or via recv_handler in ib_mad_complete_recv()
		 */
		recv = NULL;
	} else if ((ret & IB_MAD_RESULT_SUCCESS) &&
		   generate_unmatched_resp(recv, response)) {
		agent_send_jumbo_response(&response->mad.mad, &recv->grh, wc,
				    port_priv->device, port_num, qp_info->qp->qp_num);
	}

out:
	/* Post another receive request for this QP */
	if (response) {
		ib_mad_post_jumbo_rcv_mads(qp_info, response);
		if (recv) {
			BUG_ON(!(recv->header.flags & IB_MAD_PRIV_FLAG_JUMBO));
			kmem_cache_free(jumbo_mad_cache, recv);
		}
	} else
		ib_mad_post_jumbo_rcv_mads(qp_info, recv);
}


/*
 * Allocate jumbo receive MADs and post receive WRs for them
 * FIXME: combine common code with ib_mad_post_receive_mads
 */
int ib_mad_post_jumbo_rcv_mads(struct ib_mad_qp_info *qp_info,
				    struct jumbo_mad_private *mad)
{
	unsigned long flags;
	int post, ret;
	struct jumbo_mad_private *mad_priv;
	struct ib_sge sg_list;
	struct ib_recv_wr recv_wr, *bad_recv_wr;
	struct ib_mad_queue *recv_queue = &qp_info->recv_queue;

	if (unlikely(!qp_info->supports_jumbo_mads)) {
		printk(KERN_ERR PFX "Attempt to post jumbo MAD on non-jumbo QP\n");
		return (-EINVAL);
	}

	/* Initialize common scatter list fields */
	sg_list.length = sizeof *mad_priv - sizeof mad_priv->header;
	sg_list.lkey = (*qp_info->port_priv->mr).lkey;

	/* Initialize common receive WR fields */
	recv_wr.next = NULL;
	recv_wr.sg_list = &sg_list;
	recv_wr.num_sge = 1;

	do {
		/* Allocate and map receive buffer */
		if (mad) {
			mad_priv = mad;
			mad = NULL;
		} else {
			mad_priv = kmem_cache_alloc(jumbo_mad_cache, GFP_KERNEL);
			if (!mad_priv) {
				printk(KERN_ERR PFX "No memory for jumbo receive buffer\n");
				ret = -ENOMEM;
				break;
			}
			mad_priv->header.flags = IB_MAD_PRIV_FLAG_JUMBO;
		}
		sg_list.addr = ib_dma_map_single(qp_info->port_priv->device,
						 &mad_priv->grh,
						 sizeof *mad_priv -
						   sizeof mad_priv->header,
						 DMA_FROM_DEVICE);
		mad_priv->header.mapping = sg_list.addr;
		recv_wr.wr_id = (unsigned long)&mad_priv->header.mad_list;
		mad_priv->header.mad_list.mad_queue = recv_queue;

		/* Post receive WR */
		spin_lock_irqsave(&recv_queue->lock, flags);
		post = (++recv_queue->count < recv_queue->max_active);
		list_add_tail(&mad_priv->header.mad_list.list, &recv_queue->list);
		spin_unlock_irqrestore(&recv_queue->lock, flags);
		ret = ib_post_recv(qp_info->qp, &recv_wr, &bad_recv_wr);
		if (ret) {
			spin_lock_irqsave(&recv_queue->lock, flags);
			list_del(&mad_priv->header.mad_list.list);
			recv_queue->count--;
			spin_unlock_irqrestore(&recv_queue->lock, flags);
			ib_dma_unmap_single(qp_info->port_priv->device,
					    mad_priv->header.mapping,
					    sizeof *mad_priv -
					      sizeof mad_priv->header,
					    DMA_FROM_DEVICE);
			BUG_ON(!(mad_priv->header.flags & IB_MAD_PRIV_FLAG_JUMBO));
			kmem_cache_free(jumbo_mad_cache, mad_priv);
			printk(KERN_ERR PFX "ib_post_recv failed: %d\n", ret);
			break;
		}
	} while (post);

	return ret;
}

