/*
 * Copyright (c) 2005 Intel Inc. All rights reserved.
 * Copyright (c) 2005-2006 Voltaire, Inc. All rights reserved.
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

#include <linux/slab.h>

#include "mad_priv.h"
#include "jumbo_mad_rmpp.h"
#include "jumbo_mad.h"

static inline int window_size(struct ib_mad_agent_private *agent)
{
	return max(agent->qp_info->recv_queue.max_active >> 3, 1);
}

static struct ib_mad_recv_buf * find_seg_location(struct list_head *rmpp_list,
						  int seg_num)
{
	struct ib_mad_recv_buf *seg_buf;
	int cur_seg_num;

	list_for_each_entry_reverse(seg_buf, rmpp_list, list) {
		cur_seg_num = get_seg_num(seg_buf);
		if (seg_num > cur_seg_num)
			return seg_buf;
		if (seg_num == cur_seg_num)
			break;
	}
	return NULL;
}

static inline int jumbo_get_mad_len(struct mad_rmpp_recv *rmpp_recv)
{
	struct ib_rmpp_base *rmpp_base;
	int hdr_size, data_size, pad;

	rmpp_base = &((struct jumbo_rmpp_mad *)rmpp_recv->cur_seg_buf->mad)->base;

	hdr_size = ib_get_mad_data_offset(rmpp_base->mad_hdr.mgmt_class);
	if (rmpp_recv->base_version == JUMBO_MGMT_BASE_VERSION) {
		data_size = sizeof(struct jumbo_rmpp_mad) - hdr_size;
		pad = JUMBO_MGMT_RMPP_DATA - be32_to_cpu(rmpp_base->rmpp_hdr.paylen_newwin);
		if (pad > JUMBO_MGMT_RMPP_DATA || pad < 0)
			pad = 0;
	} else {
		data_size = sizeof(struct ib_rmpp_mad) - hdr_size;
		pad = IB_MGMT_RMPP_DATA - be32_to_cpu(rmpp_base->rmpp_hdr.paylen_newwin);
		if (pad > IB_MGMT_RMPP_DATA || pad < 0)
			pad = 0;
	}

	return hdr_size + rmpp_recv->seg_num * data_size - pad;
}

static struct ib_mad_recv_wc * jumbo_complete_rmpp(struct mad_rmpp_recv *rmpp_recv)
{
	struct ib_mad_recv_wc *rmpp_wc;

	ack_recv(rmpp_recv, rmpp_recv->rmpp_wc);
	if (rmpp_recv->seg_num > 1)
		cancel_delayed_work(&rmpp_recv->timeout_work);

	rmpp_wc = rmpp_recv->rmpp_wc;
	rmpp_wc->mad_len = jumbo_get_mad_len(rmpp_recv);
	/* 10 seconds until we can find the packet lifetime */
	queue_delayed_work(rmpp_recv->agent->qp_info->port_priv->wq,
			   &rmpp_recv->cleanup_work, msecs_to_jiffies(10000));
	return rmpp_wc;
}

static struct ib_mad_recv_wc *
continue_rmpp(struct ib_mad_agent_private *agent,
	      struct ib_mad_recv_wc *mad_recv_wc)
{
	struct mad_rmpp_recv *rmpp_recv;
	struct ib_mad_recv_buf *prev_buf;
	struct ib_mad_recv_wc *done_wc;
	int seg_num;
	unsigned long flags;

	rmpp_recv = acquire_rmpp_recv(agent, mad_recv_wc);
	if (!rmpp_recv)
		goto drop1;

	seg_num = get_seg_num(&mad_recv_wc->recv_buf);

	spin_lock_irqsave(&rmpp_recv->lock, flags);
	if ((rmpp_recv->state == RMPP_STATE_TIMEOUT) ||
	    (seg_num > rmpp_recv->newwin))
		goto drop3;

	if ((seg_num <= rmpp_recv->last_ack) ||
	    (rmpp_recv->state == RMPP_STATE_COMPLETE)) {
		spin_unlock_irqrestore(&rmpp_recv->lock, flags);
		ack_recv(rmpp_recv, mad_recv_wc);
		goto drop2;
	}

	prev_buf = find_seg_location(&rmpp_recv->rmpp_wc->rmpp_list, seg_num);
	if (!prev_buf)
		goto drop3;

	done_wc = NULL;
	list_add(&mad_recv_wc->recv_buf.list, &prev_buf->list);
	if (rmpp_recv->cur_seg_buf == prev_buf) {
		update_seg_num(rmpp_recv, &mad_recv_wc->recv_buf);
		if (get_last_flag(rmpp_recv->cur_seg_buf)) {
			rmpp_recv->state = RMPP_STATE_COMPLETE;
			spin_unlock_irqrestore(&rmpp_recv->lock, flags);
			done_wc = jumbo_complete_rmpp(rmpp_recv);
			goto out;
		} else if (rmpp_recv->seg_num == rmpp_recv->newwin) {
			rmpp_recv->newwin += window_size(agent);
			spin_unlock_irqrestore(&rmpp_recv->lock, flags);
			ack_recv(rmpp_recv, mad_recv_wc);
			goto out;
		}
	}
	spin_unlock_irqrestore(&rmpp_recv->lock, flags);
out:
	deref_rmpp_recv(rmpp_recv);
	return done_wc;

drop3:	spin_unlock_irqrestore(&rmpp_recv->lock, flags);
drop2:	deref_rmpp_recv(rmpp_recv);
drop1:	ib_free_recv_mad(mad_recv_wc);
	return NULL;
}

static struct ib_mad_recv_wc *
start_rmpp(struct ib_mad_agent_private *agent,
	   struct ib_mad_recv_wc *mad_recv_wc)
{
	struct mad_rmpp_recv *rmpp_recv;
	unsigned long flags;

	rmpp_recv = create_rmpp_recv(agent, mad_recv_wc);
	if (!rmpp_recv) {
		ib_free_recv_mad(mad_recv_wc);
		return NULL;
	}

	spin_lock_irqsave(&agent->lock, flags);
	if (insert_rmpp_recv(agent, rmpp_recv)) {
		spin_unlock_irqrestore(&agent->lock, flags);
		/* duplicate first MAD */
		destroy_rmpp_recv(rmpp_recv);
		return continue_rmpp(agent, mad_recv_wc);
	}
	atomic_inc(&rmpp_recv->refcount);

	if (get_last_flag(&mad_recv_wc->recv_buf)) {
		rmpp_recv->state = RMPP_STATE_COMPLETE;
		spin_unlock_irqrestore(&agent->lock, flags);
		jumbo_complete_rmpp(rmpp_recv);
	} else {
		spin_unlock_irqrestore(&agent->lock, flags);
		/* 40 seconds until we can find the packet lifetimes */
		queue_delayed_work(agent->qp_info->port_priv->wq,
				   &rmpp_recv->timeout_work,
				   msecs_to_jiffies(40000));
		rmpp_recv->newwin += window_size(agent);
		ack_recv(rmpp_recv, mad_recv_wc);
		mad_recv_wc = NULL;
	}
	deref_rmpp_recv(rmpp_recv);
	return mad_recv_wc;
}

static int jumbo_send_next_seg(struct ib_mad_send_wr_private *mad_send_wr)
{
	struct jumbo_rmpp_mad *rmpp_mad;
	int timeout;
	u32 paylen = 0;

	rmpp_mad = mad_send_wr->send_buf.mad;
	ib_set_rmpp_flags(&rmpp_mad->base.rmpp_hdr, IB_MGMT_RMPP_FLAG_ACTIVE);
	rmpp_mad->base.rmpp_hdr.seg_num = cpu_to_be32(++mad_send_wr->seg_num);

	if (mad_send_wr->seg_num == 1) {
		rmpp_mad->base.rmpp_hdr.rmpp_rtime_flags |= IB_MGMT_RMPP_FLAG_FIRST;
		paylen = mad_send_wr->send_buf.seg_count * IB_MGMT_RMPP_DATA -
			 mad_send_wr->pad;
	}

	if (mad_send_wr->seg_num == mad_send_wr->send_buf.seg_count) {
		rmpp_mad->base.rmpp_hdr.rmpp_rtime_flags |= IB_MGMT_RMPP_FLAG_LAST;
		paylen = IB_MGMT_RMPP_DATA - mad_send_wr->pad;
	}
	rmpp_mad->base.rmpp_hdr.paylen_newwin = cpu_to_be32(paylen);

	/* 2 seconds for an ACK until we can find the packet lifetime */
	timeout = mad_send_wr->send_buf.timeout_ms;
	if (!timeout || timeout > 2000)
		mad_send_wr->timeout = msecs_to_jiffies(2000);

	return ib_send_mad(mad_send_wr);
}


static struct ib_mad_recv_wc *
jumbo_process_rmpp_data(struct ib_mad_agent_private *agent,
		  struct ib_mad_recv_wc *mad_recv_wc)
{
	struct ib_rmpp_hdr *rmpp_hdr;
	u8 rmpp_status;

	rmpp_hdr = &((struct jumbo_rmpp_mad *)mad_recv_wc->recv_buf.mad)->base.rmpp_hdr;

	if (rmpp_hdr->rmpp_status) {
		rmpp_status = IB_MGMT_RMPP_STATUS_BAD_STATUS;
		goto bad;
	}

	if (rmpp_hdr->seg_num == cpu_to_be32(1)) {
		if (!(ib_get_rmpp_flags(rmpp_hdr) & IB_MGMT_RMPP_FLAG_FIRST)) {
			rmpp_status = IB_MGMT_RMPP_STATUS_BAD_SEG;
			goto bad;
		}
		return start_rmpp(agent, mad_recv_wc);
	} else {
		if (ib_get_rmpp_flags(rmpp_hdr) & IB_MGMT_RMPP_FLAG_FIRST) {
			rmpp_status = IB_MGMT_RMPP_STATUS_BAD_SEG;
			goto bad;
		}
		return continue_rmpp(agent, mad_recv_wc);
	}
bad:
	nack_recv(agent, mad_recv_wc, rmpp_status);
	ib_free_recv_mad(mad_recv_wc);
	return NULL;
}

struct ib_mad_recv_wc *
jumbo_process_rmpp_recv_wc(struct ib_mad_agent_private *agent,
			struct ib_mad_recv_wc *mad_recv_wc)
{
	struct jumbo_rmpp_mad *rmpp_mad;

	rmpp_mad = (struct jumbo_rmpp_mad *)mad_recv_wc->recv_buf.mad;
	if (!(rmpp_mad->base.rmpp_hdr.rmpp_rtime_flags & IB_MGMT_RMPP_FLAG_ACTIVE))
		return mad_recv_wc;

	if (rmpp_mad->base.rmpp_hdr.rmpp_version != IB_MGMT_RMPP_VERSION) {
		abort_send(agent, mad_recv_wc, IB_MGMT_RMPP_STATUS_UNV);
		nack_recv(agent, mad_recv_wc, IB_MGMT_RMPP_STATUS_UNV);
		goto out;
	}

	switch (rmpp_mad->base.rmpp_hdr.rmpp_type) {
	case IB_MGMT_RMPP_TYPE_DATA:
		return jumbo_process_rmpp_data(agent, mad_recv_wc);
	case IB_MGMT_RMPP_TYPE_ACK:
		process_rmpp_ack(agent, mad_recv_wc);
		break;
	case IB_MGMT_RMPP_TYPE_STOP:
		process_rmpp_stop(agent, mad_recv_wc);
		break;
	case IB_MGMT_RMPP_TYPE_ABORT:
		process_rmpp_abort(agent, mad_recv_wc);
		break;
	default:
		abort_send(agent, mad_recv_wc, IB_MGMT_RMPP_STATUS_BADT);
		nack_recv(agent, mad_recv_wc, IB_MGMT_RMPP_STATUS_BADT);
		break;
	}
out:
	ib_free_recv_mad(mad_recv_wc);
	return NULL;
}

static int init_newwin(struct ib_mad_send_wr_private *mad_send_wr)
{
	struct ib_mad_agent_private *agent = mad_send_wr->mad_agent_priv;
	struct ib_mad_hdr *mad_hdr = mad_send_wr->send_buf.mad;
	struct mad_rmpp_recv *rmpp_recv;
	struct ib_ah_attr ah_attr;
	unsigned long flags;
	int newwin = 1;

	if (!(mad_hdr->method & IB_MGMT_METHOD_RESP))
		goto out;

	spin_lock_irqsave(&agent->lock, flags);
	list_for_each_entry(rmpp_recv, &agent->rmpp_list, list) {
		if (rmpp_recv->tid != mad_hdr->tid ||
		    rmpp_recv->mgmt_class != mad_hdr->mgmt_class ||
		    rmpp_recv->class_version != mad_hdr->class_version ||
		    (rmpp_recv->method & IB_MGMT_METHOD_RESP))
			continue;

		if (ib_query_ah(mad_send_wr->send_buf.ah, &ah_attr))
			continue;

		if (rmpp_recv->slid == ah_attr.dlid) {
			newwin = rmpp_recv->repwin;
			break;
		}
	}
	spin_unlock_irqrestore(&agent->lock, flags);
out:
	return newwin;
}

int jumbo_send_rmpp_mad(struct ib_mad_send_wr_private *mad_send_wr)
{
	struct ib_rmpp_mad *rmpp_mad;
	int ret;

	rmpp_mad = mad_send_wr->send_buf.mad;
	if (!(ib_get_rmpp_flags(&rmpp_mad->base.rmpp_hdr) &
	      IB_MGMT_RMPP_FLAG_ACTIVE))
		return IB_RMPP_RESULT_UNHANDLED;

	if (rmpp_mad->base.rmpp_hdr.rmpp_type != IB_MGMT_RMPP_TYPE_DATA) {
		mad_send_wr->seg_num = 1;
		return IB_RMPP_RESULT_INTERNAL;
	}

	mad_send_wr->newwin = init_newwin(mad_send_wr);

	/* We need to wait for the final ACK even if there isn't a response */
	mad_send_wr->refcount += (mad_send_wr->timeout == 0);
	ret = jumbo_send_next_seg(mad_send_wr);
	if (!ret)
		return IB_RMPP_RESULT_CONSUMED;
	return ret;
}

int jumbo_process_rmpp_send_wc(struct ib_mad_send_wr_private *mad_send_wr,
			    struct ib_mad_send_wc *mad_send_wc)
{
	struct ib_rmpp_base *rmpp_base;
	int ret;

	rmpp_base = mad_send_wr->send_buf.mad;
	if (!(ib_get_rmpp_flags(&rmpp_base->rmpp_hdr) &
	      IB_MGMT_RMPP_FLAG_ACTIVE))
		return IB_RMPP_RESULT_UNHANDLED; /* RMPP not active */

	if (rmpp_base->rmpp_hdr.rmpp_type != IB_MGMT_RMPP_TYPE_DATA)
		return IB_RMPP_RESULT_INTERNAL;	 /* ACK, STOP, or ABORT */

	if (mad_send_wc->status != IB_WC_SUCCESS ||
	    mad_send_wr->status != IB_WC_SUCCESS)
		return IB_RMPP_RESULT_PROCESSED; /* Canceled or send error */

	if (!mad_send_wr->timeout)
		return IB_RMPP_RESULT_PROCESSED; /* Response received */

	if (mad_send_wr->last_ack == mad_send_wr->send_buf.seg_count) {
		mad_send_wr->timeout =
			msecs_to_jiffies(mad_send_wr->send_buf.timeout_ms);
		return IB_RMPP_RESULT_PROCESSED; /* Send done */
	}

	if (mad_send_wr->seg_num == mad_send_wr->newwin ||
	    mad_send_wr->seg_num == mad_send_wr->send_buf.seg_count)
		return IB_RMPP_RESULT_PROCESSED; /* Wait for ACK */

	ret = jumbo_send_next_seg(mad_send_wr);
	if (ret) {
		mad_send_wc->status = IB_WC_GENERAL_ERR;
		return IB_RMPP_RESULT_PROCESSED;
	}
	return IB_RMPP_RESULT_CONSUMED;
}

int jumbo_retry_rmpp(struct ib_mad_send_wr_private *mad_send_wr)
{
	struct jumbo_rmpp_mad *rmpp_mad;
	int ret;

	rmpp_mad = mad_send_wr->send_buf.mad;
	if (!(ib_get_rmpp_flags(&rmpp_mad->base.rmpp_hdr) &
	      IB_MGMT_RMPP_FLAG_ACTIVE))
		return IB_RMPP_RESULT_UNHANDLED; /* RMPP not active */

	if (mad_send_wr->last_ack == mad_send_wr->send_buf.seg_count)
		return IB_RMPP_RESULT_PROCESSED;

	mad_send_wr->seg_num = mad_send_wr->last_ack;
	mad_send_wr->cur_seg = mad_send_wr->last_ack_seg;

	ret = jumbo_send_next_seg(mad_send_wr);
	if (ret)
		return IB_RMPP_RESULT_PROCESSED;

	return IB_RMPP_RESULT_CONSUMED;
}
