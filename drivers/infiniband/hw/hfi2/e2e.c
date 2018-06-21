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
 * Intel(R) Omni-Path Gen2 HFI PCIe Driver
 */

#include <linux/kthread.h>
#include <linux/sched/signal.h>
#include <linux/radix-tree.h>
#include "hfi2.h"
#include "hfi_kclient.h"

union hfi_tx_e2e_ctrl {
	struct {
		union tx_cq_base_put_flit0 flit0;
		union tx_cq_reply_flit1 flit1;
	};
	u64 command[8];
} __aligned(64);

static void hfi_e2e_cache_invalidate_rx(struct ida *cache, u32 lid)
{
	ida_simple_get(cache, lid, lid + 1, GFP_KERNEL);

	ida_simple_remove(cache, lid);
}

/*
 * IDR key looks like:
 *
 * +--------------------------------+
 * |XXLLLLLLLLLLLLLLLLLLLLLLLLIIIIII|
 * +--------------------------------+
 * where:
 *   X: Unused
 *   D: Destination LID
 *   I: PTL_PKEY ID
 */

#define HFI_E2E_DLID_SHIFT		6u
#define HFI_E2E_DLID_MASK		0xffffffu
#define HFI_E2E_PKEY_ID_SHIFT		0u
#define HFI_E2E_PKEY_ID_MASK		0x3fu

/*
 * Cache value stored in the IDR looks like:
 *
 * +----------------------------------------------------------------+
 * |XXXXXXXXXXXXXXXXXPPPPPPPPPPPPPPPPLLLLLLLLLLLLLLLLLLLLLLLLSSSSS10|
 * +----------------------------------------------------------------+
 * where:
 *   X: Unused
 *   P: PKey
 *   L: Source LID
 *   S: SC
 *  10: Exceptional entry for IDR (not data pointer)
 */
#define HFI_E2E_SLID_SHIFT		7ull
#define HFI_E2E_SLID_MASK		0xffffffull
#define HFI_E2E_SL_SHIFT		2ull
#define HFI_E2E_SL_MASK			0x1full
#define HFI_E2E_PKEY_SHIFT		31ull
#define HFI_E2E_PKEY_MASK		0xffffull

inline u32 hfi_e2e_key_to_dlid(int key)
{
	return (key >> HFI_E2E_DLID_SHIFT) & HFI_E2E_DLID_MASK;
}

inline u32 hfi_e2e_key_to_pkey_id(int key)
{
	return (key >> HFI_E2E_PKEY_ID_SHIFT) & HFI_E2E_PKEY_ID_MASK;
}

inline u16 hfi_e2e_key_to_pkey(struct hfi_pportdata *ppd, u32 key)
{
	return ppd->pkeys[hfi_e2e_key_to_pkey_id(key)];
}

inline u8 hfi_e2e_cache_data_to_sl(u64 *data)
{
	return (u16)(((u64)data >> HFI_E2E_SL_SHIFT) & HFI_E2E_SL_MASK);
}

inline u16 hfi_e2e_cache_data_to_pkey(u64 *data)
{
	return (u16)(((u64)data >> HFI_E2E_PKEY_SHIFT) & HFI_E2E_PKEY_MASK);
}

inline u32 hfi_e2e_cache_data_to_slid(u64 *data)
{
	return (u32)(((u64)data >> HFI_E2E_SLID_SHIFT) & HFI_E2E_SLID_MASK);
}

static inline int hfi_e2e_make_cache_key_pkey_id(u16 pkey_id, u32 dlid)
{
	return ((dlid & HFI_E2E_DLID_MASK) << HFI_E2E_DLID_SHIFT) |
		((pkey_id & HFI_E2E_PKEY_ID_MASK) << HFI_E2E_PKEY_ID_SHIFT);
}

static int hfi_e2e_make_cache_key_pkey(struct hfi_pportdata *ppd, u16 pkey,
				       u32 dlid)
{
	u8 i;

	if (HFI_PKEY_IS_EQ(pkey, 0))
		return -EINVAL;

	for (i = 0; i < HFI_MAX_PKEYS; i++)
		if (HFI_PKEY_IS_EQ(ppd->pkeys[i], pkey))
			break;

	if (i == HFI_MAX_PKEYS) {
		ppd_dev_err(ppd, "%s: Can't find pkey 0x%x\n", __func__, pkey);
		return -EINVAL;
	}

	return hfi_e2e_make_cache_key_pkey_id(i, dlid);
}

static void hfi_e2e_cache_invalidate_tx(struct idr *cache, u32 lid)
{
	u8 i;

	/* We can have most HFI_MAX_PKEYS connections per DLID */
	for (i = 0; i < HFI_MAX_PKEYS; i++) {
		void *val;
		int key;

		key = hfi_e2e_make_cache_key_pkey_id(i, lid);
		val = idr_find(cache, key);
		if (!val)
			continue;

		idr_remove(cache, key);
	}
}

/*
 * Block for an EQ zero event interrupt, return value:
 * <0: on error, including dropped,
 *  0: no new event,
 *  1. get new event,
 */
static int hfi_eq_zero_event_wait(struct hfi_ctx *ctx, u64 **eq_entry)
{
	int rc;
	bool dropped = false;

	/* event queue must be in blocking mode */
	rc = hfi_ev_wait_single(ctx, 1, ctx->eq_zero[0].idx,
				-1, NULL, NULL);
	if (!rc)
		rc = hfi_eq_peek(&ctx->eq_zero[0],
				 eq_entry, &dropped);

	if (rc == -ETIME || rc == -ERESTARTSYS)
		/* timeout or wait interrupted, not abnormal */
		rc = 0;
	else if (rc > 0 && dropped)
		/* driver bug with EQ sizing or IRQ logic */
		rc = -EIO;
	return rc;
}

/* kernel thread handling EQ0 events for the system PID */
static int hfi_eq_zero_thread(void *data)
{
	struct hfi_ctx *ctx = data;
	struct hfi_devdata *dd = ctx->devdata;
	u64 *eq_entry;
	union target_EQEntry *rxe;
	int rc;
	u8 port, tc;
	u32 slid;
	struct hfi_ptcdata *ptc;
	struct ida *cache_rx;
	struct idr *cache_tx;
	u32 tx_timeout = dd->emulation ?
		HFI_TX_TIMEOUT_MS_ZEBU : HFI_TX_TIMEOUT_MS;

	allow_signal(SIGINT);

	while (!kthread_should_stop()) {
		eq_entry = NULL;

		if (!dd->emulation) {
			rc = hfi_eq_zero_event_wait(ctx, &eq_entry);
			if (rc < 0) {
				/* only likely error is DROPPED event */
				dd_dev_err(dd,
					   "%s unexpected EQ wait error %d\n",
					   __func__, rc);
				break;
			}
			if (!rc)
				continue;
		} else {
			/* TODO: Disable interrupts to workaround 1407227170 */
			rc = hfi_eq_wait_timed(&ctx->eq_zero[0], tx_timeout,
					       &eq_entry);
			if (rc) {
				schedule();
				cpu_relax();
				msleep(1000);
				continue;
			}
		}
		rxe = (union target_EQEntry *)eq_entry;
		port = rxe->pt;
		tc = HFI_GET_TC(rxe->user_ptr);
		slid = rxe->initiator_id;

		switch (rxe->event_kind) {
		case PTL_EVENT_DISCONNECT:
			dd_dev_dbg(dd, "%s E2E disconnect request\n",
				   __func__);

			if (tc >= HFI_MAX_TC) {
				dd_dev_err(dd, "%s unexpected tc %d\n",
					   __func__, tc);
				break;
			}
			ptc = &dd->pport[port].ptc[tc];
			cache_rx = &ptc->e2e_rx_state_cache;
			cache_tx = &ptc->e2e_tx_state_cache;

			hfi_e2e_cache_invalidate_rx(cache_rx, slid);

			mutex_lock(&dd->e2e_lock);
			hfi_e2e_cache_invalidate_tx(cache_tx, slid);
			mutex_unlock(&dd->e2e_lock);

			dd_dev_info(dd, "%s ev %d pt %d lid %lld tc %d\n",
				    __func__, rxe->event_kind, rxe->pt,
				    (u64)rxe->initiator_id, tc);
			break;

		case PTL_EVENT_TARGET_CONNECT:
			dd_dev_dbg(dd, "%s E2E connect request\n",
				   __func__);

			if (tc >= HFI_MAX_TC) {
				dd_dev_err(dd, "%s unexpected tc %d\n",
					   __func__, tc);
				break;
			}
			ptc = &dd->pport[port].ptc[tc];
			cache_rx = &ptc->e2e_rx_state_cache;
			cache_tx = &ptc->e2e_tx_state_cache;

			rc = ida_simple_get(cache_rx, slid, slid + 1,
					    GFP_KERNEL);

			/*
			 * In the case where the RX cache entry exists, but
			 * we're getting another reconnect, that can mean
			 * the initiating node crashed. One exception to this
			 * is when we are removed from a partition (revoked
			 * PKey). For loopback E2E connections, we know that
			 * we're not a crashed node, so skip invalidating the
			 * TX E2E cache entry. Otherwise, if the source LID
			 * is not us, remove the TX entry.
			 */
			if (-ENOSPC == rc && slid != dd->pport[port].lid) {
				dd_dev_dbg(dd, "%s E2E Crashed node reconnect\n",
					   __func__);

				/*
				 * The node requesting this connect, went down
				 * without a disconnect request. Therefore the
				 * TX E2E connection has to be re established
				 * hence the TX cache invalidate.
				 */
				mutex_lock(&dd->e2e_lock);
				hfi_e2e_cache_invalidate_tx(cache_tx, slid);
				mutex_unlock(&dd->e2e_lock);
			}

			dd_dev_info(dd, "%s ev %d pt %d lid %lld tc %d\n",
				    __func__, rxe->event_kind, rxe->pt,
				    (u64)rxe->initiator_id, tc);
			break;
		case PTL_CMD_COMPLETE:
			/* These events are handled synchronously */
			break;
		default:
			dd_dev_err(dd, "%s unexpected event %d port %d\n",
				   __func__, rxe->event_kind, rxe->pt);
			break;
		}

		if (rxe->event_kind != PTL_CMD_COMPLETE)
			hfi_eq_advance(&ctx->eq_zero[0], eq_entry);
	}
	return 0;
}

static int hfi_e2e_eq_assign(struct hfi_ctx *ctx)
{
	struct hfi_devdata *dd = ctx->devdata;
	struct opa_ev_assign eq_assign = {0};
	int ret;
	u32 *eq_head_array, *eq_head_addr;
	u64 done = 0;

	eq_assign.user_data = (u64)&done;
	eq_assign.ni = PTL_NONMATCHING_LOGICAL;
	/* EQ is one page and meets 64B alignment */
	eq_assign.count = 64;
	eq_assign.base = (u64)kzalloc(eq_assign.count *
				      HFI_EQ_ENTRY_SIZE,
				      GFP_KERNEL);
	if (!eq_assign.base)
		return -ENOMEM;
	ret = hfi_eq_assign(ctx, &eq_assign);
	if (ret)
		return ret;
	eq_head_array = ctx->eq_head_addr;
	eq_head_addr = &eq_head_array[eq_assign.ev_idx];
	hfi_set_eq(ctx, &dd->e2e_eq, &eq_assign, eq_head_addr);
	/* Reset the EQ SW head */
	*eq_head_addr = 0;

	/* Check on EQ 0 NI 0 for a PTL_CMD_COMPLETE event */
	ret = hfi_eq_poll_cmd_complete_timeout(ctx, &done);

	return ret;
}

static void hfi_e2e_eq_release(struct hfi_ctx *ctx)
{
	struct hfi_devdata *dd = ctx->devdata;

	if (dd->e2e_eq.base) {
		hfi_eq_release(ctx, dd->e2e_eq.idx, 0x0);
		kfree(dd->e2e_eq.base);
		dd->e2e_eq.base = NULL;
	}
}

int hfi_e2e_start(struct hfi_ctx *ctx)
{
	int rc;
	struct hfi_devdata *dd = ctx->devdata;

	/* verify system PID */
	if (ctx->pid != HFI_PID_SYSTEM)
		return -EPERM;

	dd->eq_zero_thread = kthread_run(hfi_eq_zero_thread, ctx,
					 "hfi_eq_zero%d", dd->unit);
	if (IS_ERR(dd->eq_zero_thread)) {
		rc = PTR_ERR(dd->eq_zero_thread);
		return rc;
	}

	rc = hfi_e2e_eq_assign(ctx);
	if (rc) {
		kthread_stop(dd->eq_zero_thread);
		dd->eq_zero_thread = NULL;
		return rc;
	}

	return rc;
}

void hfi_e2e_stop(struct hfi_ctx *ctx)
{
	int rc;
	struct hfi_devdata *dd = ctx->devdata;

	if (ctx != &dd->priv_ctx)
		return;

	if (!IS_ERR_OR_NULL(dd->eq_zero_thread)) {
		rc = send_sig(SIGINT, dd->eq_zero_thread, 0);
		if (rc) {
			dd_dev_err(dd, "send_sig failed %d\n", rc);
			return;
		}
		kthread_stop(dd->eq_zero_thread);
		dd->eq_zero_thread = NULL;
	}
	hfi_e2e_eq_release(ctx);
}

static inline
void hfi_format_e2e_flit0(struct hfi_ctx *ctx, u8 ni,
			  union tx_cq_base_put_flit0 *flit0,
			  union ptl_cmd cmd, u16 cmd_length,
			  union hfi_process target_id,
			  u8 port, u8 rc, u8 sl,
			  u16 pkey, u8 slid_low,
			  u8 auth_idx, u64 user_ptr,
			  enum ptl_L4_ack_req ack_req,
			  u32 md_options,
			  struct hfi_eq *eq_handle,
			  u16 ct_handle)
{
	flit0->a.val		= 0;
	flit0->a.pt		= port;
	flit0->a.cmd		= cmd.val;
	flit0->a.rc		= rc;
	flit0->a.sl		= sl;
	flit0->a.sh		= 1; /* Always request a short header */
	flit0->a.ctype		= E2E_CTRL;
	flit0->a.cmd_length	= cmd_length;
	flit0->a.dlid		= target_id.phys.slid;
	flit0->b.val		= 0;
	flit0->b.slid_low	= slid_low;
	flit0->b.auth_idx	= auth_idx;
	flit0->b.pkey		= pkey;
	flit0->b.eq_handle	= eq_handle->idx;
	flit0->b.ct_handle	= ct_handle;
	flit0->b.md_options	= md_options;
	flit0->b.pd		= 1;
	flit0->c		= user_ptr;
	flit0->d.val		= 0;
	flit0->d.ack_req	= ack_req;
	flit0->d.ni		= ni;
}

/* Format an E2E control message, transmit it and wait for an acknowledgment */
static int hfi_put_e2e_ctrl(struct hfi_devdata *dd, int slid, int dlid,
			    int sl, int port, enum ptl_op_e2e_ctrl op,
			    u16 pkey)
{
	union hfi_tx_e2e_ctrl e2e_cmd;
	struct hfi_ctx *ctx = &dd->priv_ctx;
	union ptl_cmd cmd;
	u16 cmd_length = 8;
	u16 cmd_slots = _HFI_CMD_SLOTS(cmd_length);
	union hfi_process target_id;
	enum ptl_L4_ack_req ack_req = PTL_CT_ACK_REQ;
	u32 md_options = PTL_MD_EVENT_CT_ACK;
	u8 ni = PTL_NONMATCHING_LOGICAL;
	u64 *eq_entry;
	int rc;

	dd_dev_dbg(dd, "%s: .slid=%u .dlid=%u .pkey=%u .port_num=%u .sl=%u\n",
		   __func__, slid, dlid, pkey, port, sl);

	memset(&e2e_cmd, 0x0, sizeof(e2e_cmd));
	target_id.phys.slid = dlid;
	target_id.phys.ipid = dd->priv_ctx.pid;
	cmd.ptl_opcode_low = op;
	cmd.ttype = BUFFERED;

	hfi_format_e2e_flit0(ctx, ni, &e2e_cmd.flit0, cmd,
			     cmd_length, target_id, port,
			     RC_IN_ORDER_0, sl, pkey, 0, 0, 0,
			     ack_req, md_options, &dd->e2e_eq,
			     PTL_CT_NONE);

	e2e_cmd.flit1.e.max_dist = 1024;

	/* Queue it to be written, don't wait for completion */
	rc = hfi_pend_cmd_queue(&dd->pend_cmdq,
				&dd->priv_cmdq.tx,
				&dd->e2e_eq,
				&e2e_cmd.command,
				cmd_slots, GFP_KERNEL);

	if (rc)
		goto done;

	/* PTL_SINGLE_DESTROY does not initiate events at the initiator */
	if (op != PTL_SINGLE_CONNECT)
		goto done;
	/* Check on E2E EQ for a PTL_EVENT_INITIATOR_CONNECT event */
	rc = hfi_eq_wait_timed(&dd->e2e_eq, HFI_TX_TIMEOUT_MS,
			       &eq_entry);
	if (!rc) {
		union initiator_EQEntry *txe =
			(union initiator_EQEntry *)eq_entry;

		if (txe->event_kind == PTL_EVENT_INITIATOR_CONNECT) {
			if (txe->fail_type) {
				dd_dev_info(dd, "E2E EQ fail kind %d ft %d\n",
					    txe->event_kind, txe->fail_type);
				rc = -EIO;
			} else {
				dd_dev_info(dd, "E2E EQ success kind %d\n",
					    txe->event_kind);
			}
			hfi_eq_advance(&dd->e2e_eq, eq_entry);
		} else {
			rc = -EIO;
			dd_dev_err(dd, "Invalid E2E event %d\n",
				   txe->event_kind);
		}
	} else {
		dd_dev_err(dd, "E2E EQ failure rc %d\n", rc);
	}
done:
	return rc;
}

int hfi_e2e_ctrl(struct hfi_ibcontext *uc, struct hfi_e2e_conn *e2e)
{
	struct hfi_devdata *dd = uc->priv;
	struct hfi_pportdata *ppd;
	struct hfi_ptcdata *ptc;
	u64 cache_data;
	u8 tc;
	u8 sl = e2e->sl;
	struct idr *cache;
	int ret, key;

	dd_dev_dbg(dd, "%s: .slid=%u .dlid=%u .pkey=%u .port_num=%u .sl=%u\n",
		   __func__, e2e->slid, e2e->dlid, e2e->pkey, e2e->port_num,
		   e2e->sl);

	ppd = to_hfi_ppd(dd, e2e->port_num);
	if (!ppd)
		return -EINVAL;

	if (e2e->dlid >= ppd->max_lid)
		return -EINVAL;

	if (!hfi_is_req_sl(ppd, sl))
		return -EINVAL;

	if (!e2e->slid)
		return -EINVAL;

	tc = HFI_GET_TC(ppd->sl_to_mctc[sl]);

	ptc = &ppd->ptc[tc];
	cache = &ptc->e2e_tx_state_cache;

	mutex_lock(&dd->e2e_lock);
	key = hfi_e2e_make_cache_key_pkey(ppd, e2e->pkey, e2e->dlid);
	if (key < 0) {
		ppd_dev_err(ppd, "%s: Unable to make cache key %d\n", __func__,
			    key);
		ret = key;
		goto unlock;
	}

	cache_data = (sl & HFI_E2E_SL_MASK) << HFI_E2E_SL_SHIFT;
	cache_data |= (e2e->slid & HFI_E2E_SLID_MASK) << HFI_E2E_SLID_SHIFT;
	cache_data |= (e2e->pkey & HFI_E2E_PKEY_MASK) << HFI_E2E_PKEY_SHIFT;
	cache_data |= RADIX_TREE_EXCEPTIONAL_ENTRY;

	/* Check if a new entry can be inserted into the cache */
	ret = idr_alloc(cache, (void *)cache_data, key, key + 1, GFP_KERNEL);
	/*
	 * If the entry is allocated then the E2E connection is established
	 * already and there is no need to initiate another E2E connection
	 */
	if (-ENOSPC == ret) {
		ppd_dev_dbg(ppd, "%s: Key 0x%x exists\n", __func__, key);
		ret = 0;
		goto unlock;
	}

	/* Bail out upon other IDR failures */
	if (ret < 0) {
		ppd_dev_err(ppd, "%s: idr alloc fail %d\n", __func__, ret);
		goto unlock;
	}

	/* Initiate an E2E connection if one did not exist */
	ret = hfi_put_e2e_ctrl(dd, e2e->slid, e2e->dlid,
			       sl, e2e->port_num - 1, PTL_SINGLE_CONNECT,
			       e2e->pkey);
	/* remove the entry from the cache upon failure */
	if (ret < 0) {
		ppd_dev_err(ppd, "%s: e2e put ctrl fail %d\n", __func__, ret);
		idr_remove(cache, key);
	}

unlock:
	mutex_unlock(&dd->e2e_lock);
	return ret;
}

/*
 * hfi_e2e_init - initialize per traffic class E2E state
 */
void hfi_e2e_init(struct hfi_pportdata *ppd)
{
	int i;

	for (i = 0; i < HFI_MAX_TC; i++) {
		struct hfi_ptcdata *tc = &ppd->ptc[i];

		idr_init(&tc->e2e_tx_state_cache);
		ida_init(&tc->e2e_rx_state_cache);
	}
}

/* Tear down all existing E2E connections */
void hfi_e2e_destroy_all(struct hfi_devdata *dd)
{
	u8 port;

	mutex_lock(&dd->e2e_lock);
	for (port = 0; port < HFI_NUM_PPORTS; port++) {
		struct hfi_pportdata *ppd = &dd->pport[port];
		u32 slid = ppd->lid;
		u8 tc;

		for (tc = 0; tc < HFI_MAX_TC; tc++) {
			struct hfi_ptcdata *ptc = &ppd->ptc[tc];
			struct idr *cache = &ptc->e2e_tx_state_cache;
			u64 *cache_data;
			u32 key;

			idr_for_each_entry(cache, cache_data, key) {
				u32 dlid = hfi_e2e_key_to_dlid(key);
				u16 pkey = hfi_e2e_key_to_pkey(ppd, key);
				u8 req_sl;

				req_sl = hfi_e2e_cache_data_to_sl(cache_data);

				hfi_put_e2e_ctrl(dd, slid, dlid,
						 req_sl, port,
						 PTL_SINGLE_DESTROY,
						 pkey);
				idr_remove(cache, key);
			}
			idr_destroy(cache);
		}
	}
	mutex_unlock(&dd->e2e_lock);
}

static void hfi_e2e_destroy_pkeys(struct hfi_pportdata *ppd, u16 *pkeys,
				  u64 pkey_del)
{
	u8 tc;

	mutex_lock(&ppd->dd->e2e_lock);

	for (tc = 0; tc < HFI_MAX_TC; tc++) {
		struct hfi_ptcdata *ptc = &ppd->ptc[tc];
		struct idr *cache = &ptc->e2e_tx_state_cache;
		u64 *cache_data;
		u32 key;

		idr_for_each_entry(cache, cache_data, key) {
			unsigned long oid;
			u16 pkey = hfi_e2e_cache_data_to_pkey(cache_data);

			/*
			 * If it's in the tx cache, and it's in the list of
			 * removed pkeys, then delete it.
			 */
			for_each_set_bit(oid, (unsigned long *)&pkey_del,
					 8 * sizeof(pkey_del)) {
				u16 old_pkey;

				old_pkey = pkeys[oid];

				if (HFI_PKEY_IS_EQ(pkey, old_pkey)) {
					idr_remove(cache, key);
					break;
				}
			}
		}
	}
	mutex_unlock(&ppd->dd->e2e_lock);
}

static bool hfi_ppd_pkeys_contains(struct hfi_pportdata *ppd, u16 pkey)
{
	int i;

	for (i = 0; i < HFI_MAX_PKEYS; i++)
		if (ppd->pkeys[i] == pkey)
			return true;

	ppd_dev_dbg(ppd, "PKey 0x%x not matched\n", pkey);

	return false;
}

void hfi_e2e_destroy_old_pkeys(struct hfi_pportdata *ppd, u16 *old_pkeys)
{
	u64 pkey_del = 0;
	int i;

	/*
	 * Check every old value. If it doesn't exist in the new table,
	 * it's been deleted. Add to the mask, then invalidate.
	 */
	for (i = 0; i < HFI_MAX_PKEYS; i++) {
		if (HFI_PKEY_CAM(old_pkeys[i]) == 0)
			continue;
		if (!hfi_ppd_pkeys_contains(ppd, old_pkeys[i])) {
			ppd_dev_dbg(ppd, "pkey 0x%x index %d deleted\n",
				    old_pkeys[i], i);
			pkey_del |= BIT_ULL(i);
		}
	}

	if (pkey_del)
		hfi_e2e_destroy_pkeys(ppd, old_pkeys, pkey_del);
}

