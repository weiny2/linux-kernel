/*
 * Copyright (c) 2005 Intel Inc. All rights reserved.
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

#ifndef __MAD_RMPP_H__
#define __MAD_RMPP_H__

enum {
	IB_RMPP_RESULT_PROCESSED,
	IB_RMPP_RESULT_CONSUMED,
	IB_RMPP_RESULT_INTERNAL,
	IB_RMPP_RESULT_UNHANDLED
};

enum rmpp_state {
	RMPP_STATE_ACTIVE,
	RMPP_STATE_TIMEOUT,
	RMPP_STATE_COMPLETE,
	RMPP_STATE_CANCELING
};

struct mad_rmpp_recv {
	struct ib_mad_agent_private *agent;
	struct list_head list;
	struct delayed_work timeout_work;
	struct delayed_work cleanup_work;
	struct completion comp;
	enum rmpp_state state;
	spinlock_t lock;
	atomic_t refcount;

	struct ib_ah *ah;
	struct ib_mad_recv_wc *rmpp_wc;
	struct ib_mad_recv_buf *cur_seg_buf;
	int last_ack;
	int seg_num;
	int newwin;
	int repwin;

	__be64 tid;
	u32 src_qp;
	u16 slid;
	u8 mgmt_class;
	u8 class_version;
	u8 method;
	u8 base_version; /* indicates Jumbo or IB MAD */
};


int ib_send_rmpp_mad(struct ib_mad_send_wr_private *mad_send_wr);

struct ib_mad_recv_wc *
ib_process_rmpp_recv_wc(struct ib_mad_agent_private *agent,
			struct ib_mad_recv_wc *mad_recv_wc);

int ib_process_rmpp_send_wc(struct ib_mad_send_wr_private *mad_send_wr,
			    struct ib_mad_send_wc *mad_send_wc);

void ib_rmpp_send_handler(struct ib_mad_send_wc *mad_send_wc);

void ib_cancel_rmpp_recvs(struct ib_mad_agent_private *agent);

int ib_retry_rmpp(struct ib_mad_send_wr_private *mad_send_wr);

static inline void deref_rmpp_recv(struct mad_rmpp_recv *rmpp_recv)
{
	if (atomic_dec_and_test(&rmpp_recv->refcount))
		complete(&rmpp_recv->comp);
}

void destroy_rmpp_recv(struct mad_rmpp_recv *rmpp_recv);

void format_ack(struct ib_mad_send_buf *msg,
		       struct ib_rmpp_base *data,
		       struct mad_rmpp_recv *rmpp_recv);

void nack_recv(struct ib_mad_agent_private *agent,
	       struct ib_mad_recv_wc *recv_wc, u8 rmpp_status);
struct mad_rmpp_recv *
find_rmpp_recv(struct ib_mad_agent_private *agent,
	       struct ib_mad_recv_wc *mad_recv_wc);
static inline int get_seg_num(struct ib_mad_recv_buf *seg)
{
	struct ib_rmpp_base *rmpp_base;

	rmpp_base = (struct ib_rmpp_base *) seg->mad;
	return be32_to_cpu(rmpp_base->rmpp_hdr.seg_num);
}

void ack_recv(struct mad_rmpp_recv *rmpp_recv,
	      struct ib_mad_recv_wc *recv_wc);
struct mad_rmpp_recv *
acquire_rmpp_recv(struct ib_mad_agent_private *agent,
		  struct ib_mad_recv_wc *mad_recv_wc);
void update_seg_num(struct mad_rmpp_recv *rmpp_recv,
		    struct ib_mad_recv_buf *new_buf);
struct mad_rmpp_recv *
insert_rmpp_recv(struct ib_mad_agent_private *agent,
		 struct mad_rmpp_recv *rmpp_recv);
struct mad_rmpp_recv *
create_rmpp_recv(struct ib_mad_agent_private *agent,
		 struct ib_mad_recv_wc *mad_recv_wc);
static inline int get_last_flag(struct ib_mad_recv_buf *seg)
{
	struct ib_rmpp_base *rmpp_base;

	rmpp_base = (struct ib_rmpp_base *) seg->mad;
	return ib_get_rmpp_flags(&rmpp_base->rmpp_hdr) & IB_MGMT_RMPP_FLAG_LAST;
}

void abort_send(struct ib_mad_agent_private *agent,
		       struct ib_mad_recv_wc *mad_recv_wc, u8 rmpp_status);
void recv_cleanup_handler(struct work_struct *work);
static inline void adjust_last_ack(struct ib_mad_send_wr_private *wr,
				   int seg_num)
{
	struct list_head *list;

	wr->last_ack = seg_num;
	list = &wr->last_ack_seg->list;
	list_for_each_entry(wr->last_ack_seg, list, list)
		if (wr->last_ack_seg->num == seg_num)
			break;
}
void process_rmpp_abort(struct ib_mad_agent_private *agent,
			       struct ib_mad_recv_wc *mad_recv_wc);
void process_rmpp_ack(struct ib_mad_agent_private *agent,
			     struct ib_mad_recv_wc *mad_recv_wc);
void process_rmpp_stop(struct ib_mad_agent_private *agent,
			      struct ib_mad_recv_wc *mad_recv_wc);

#endif	/* __MAD_RMPP_H__ */
