/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2016 Intel Corporation.
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
 * Copyright (c) 2016 Intel Corporation.
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
#include "hfi2.h"
#include "pend_cmdq.h"
#include "hfi_kclient.h"
#include "hfi_tx_bypass.h"

/* FXRTODO: Delete or clean up before upstreaming */
static void hfi_dump_pend_cmd(struct hfi_devdata *dd, struct hfi_pend_cmd *cmd)
{
	dd_dev_info(dd, "Pending command: %p, CMDQ: %p, EQ: %p\n", cmd,
		    cmd->cmdq, cmd->eq);
	dd_dev_info(dd, "slot_ptr %p, cmd_slots %d, ret %d, wait %s\n",
		    cmd->slot_ptr, cmd->cmd_slots, cmd->ret,
		    cmd->wait ? "true" : "false");
	dd_dev_info(dd, "Slot data: 0x%.16llx 0x%.16llx\n", cmd->slot_ptr[0],
		    cmd->cmd_slots > 1 ? cmd->slot_ptr[1] : 0);
}

/* FXRTODO: Delete or clean up and move to the right place before upstreaming */
static void hfi_dump_cmdq(struct hfi_devdata *dd, struct hfi_cmdq *cmdq)
{
	dd_dev_info(dd, "CMDQ: %p, cmdq_idx %u\n", cmdq, cmdq->cmdq_idx);
	dd_dev_info(dd, "base %p, size %lu, head_addr %p, slot_idx %u\n",
		    cmdq->base, cmdq->size, cmdq->head_addr, cmdq->slot_idx);
	dd_dev_info(dd, "sw_head_idx %u, slots_avail %u, slots_total %u.\n",
		    cmdq->sw_head_idx, cmdq->slots_avail, cmdq->slots_total);
}

/* FXRTODO: Delete or clean up and move to the right place before upstreaming */
static void hfi_dump_eq(struct hfi_devdata *dd, struct hfi_eq *eq)
{
	dd_dev_info(dd, "EQ: %p\n", eq);
	dd_dev_info(dd, "base %p, events_pending %d, count %u, ",
		    eq->base, atomic_read(&eq->events_pending), eq->count);
	dd_dev_info(dd, "idx %u, head_addr %p\n", eq->idx, eq->head_addr);
}

static void hfi_write_pending_cmd(struct hfi_pend_cmd *cmd,
				  struct hfi_devdata *dd)
{
	struct hfi_cmdq *cmdq = cmd->cmdq;
	unsigned long timeout;

	timeout = jiffies + msecs_to_jiffies(HFI_PRIV_CMDQ_TIMEOUT_MS);

	/*
	 * FXRTODO: STL-14912, handle the case where no forward progress
	 * is made and attempt to remove the busy wait through waking on
	 * TX EQ events.
	 */
	/* Busy wait until enough slots are available */
	while (!hfi_queue_ready(cmdq, cmd->cmd_slots, cmd->eq)) {
		if (time_after(jiffies, timeout))
			goto timeout;

		udelay(2);
	}

	if (cmd->eq)
		hfi_eq_pending_inc(cmd->eq);

	if (cmd->cmd_slots == 1)
		_hfi_command(cmdq, cmd->slot_ptr, cmdq->slots_total);
	else if (cmd->cmd_slots == 2)
		_hfi_command2(cmdq, cmd->slot_ptr, cmdq->slots_total);

	cmd->ret = 0;

	return;
timeout:

	dd_dev_warn(dd,
		    "Timeout waiting for pend CMDQ %p slots: %d total: %d\n",
		    cmdq, cmd->cmd_slots, cmdq->slots_total);

	hfi_dump_cmdq(dd, cmd->cmdq);
	hfi_dump_pend_cmd(dd, cmd);

	if (cmd->eq) {
		dd_dev_warn(dd, "CMDQ %p EQ %p %d %d\n", cmdq, cmd->eq,
			    atomic_read(&cmd->eq->events_pending),
			    cmd->eq->count);
		hfi_dump_eq(dd, cmd->eq);
	}

	cmd->ret = -EIO;
}

static inline
int hfi_pend_cmd_direct(struct hfi_devdata *dd, struct hfi_cmdq *cmdq,
			struct hfi_eq *eq, void *cmd, int nslots)
{
	struct hfi_pend_cmd tmp;

	tmp.cmdq = cmdq;
	tmp.cmd_slots = nslots;
	tmp.slot_ptr = cmd;
	tmp.wait = false;
	tmp.eq = eq;

	hfi_write_pending_cmd(&tmp, dd);

	return tmp.ret;
}

int _hfi_pend_cmd_queue(struct hfi_pend_queue *pq, struct hfi_cmdq *cmdq,
			struct hfi_eq *eq, void *cmd, int nslots,
			bool wait, gfp_t gfp)
{
	struct hfi_pend_cmd *p;
	bool always_direct = false; /* FXRTODO: Add ability to tweak this */
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&pq->lock, flags);
	/* Optimization for empty list case */
	if (always_direct ||
	    (list_empty(&pq->pending) && hfi_queue_ready(cmdq, nslots, eq))) {
		ret = hfi_pend_cmd_direct(pq->dd, cmdq, eq, cmd, nslots);
		spin_unlock_irqrestore(&pq->lock, flags);
		return ret;
	}
	spin_unlock_irqrestore(&pq->lock, flags);

	p = kmem_cache_alloc(pq->cache, gfp);
	if (ZERO_OR_NULL_PTR(p)) {
		dd_dev_err(pq->dd, "Unable to allocate from kmem cache\n");
		return -ENOMEM;
	}

	p->ret = 0;
	p->cmdq = cmdq;
	p->cmd_slots = nslots;
	p->slot_ptr = cmd;
	p->wait = wait;
	p->eq = eq;

	/* Asynch case needs to copy the data to the p struct */
	if (!wait) {
		size_t nbytes = nslots * 64;

		if (nbytes > HFI_MAX_PEND_CMD_LEN_BYTES) {
			dd_dev_err(pq->dd, "%s: %d size %ld > %ld\n", __func__,
				   __LINE__, nbytes,
				   (size_t)HFI_MAX_PEND_CMD_LEN_BYTES);
			kmem_cache_free(pq->cache, p);
			return -EINVAL;
		}

		memcpy(p->slots, cmd, nslots * 64);
		p->slot_ptr = &p->slots[0];
	} else {
		init_completion(&p->completion);
	}

	spin_lock_irqsave(&pq->lock, flags);
	list_add_tail(&p->list, &pq->pending);
	spin_unlock_irqrestore(&pq->lock, flags);

	/* To sync with rmb in hfi_pend_cmdq_thread */
	wmb();
	wake_up_interruptible(&pq->event);

	if (wait) {
		wait_for_completion(&p->completion);

		/* To sync with wmb in hfi_pend_cmdq_thread */
		rmb();

		ret = p->ret;

		kmem_cache_free(pq->cache, p);
	} else {
		ret = 0;
	}

	return ret;
}

/* Kernel thread handling privileged CMDQ writes */
static int hfi_pend_cmdq_thread(void *data)
{
	struct hfi_pend_queue *pq = data;
	struct hfi_pend_cmd *p, *next;
	unsigned long flags;

	allow_signal(SIGINT);

	while (!kthread_should_stop()) {
		wait_event_interruptible(pq->event,
					 !list_empty(&pq->pending));
		/* To sync with wmb in _hfi_pend_cmd_queue */
		rmb();
		spin_lock_irqsave(&pq->lock, flags);
		list_for_each_entry_safe(p, next, &pq->pending, list) {
			spin_unlock_irqrestore(&pq->lock, flags);

			hfi_write_pending_cmd(p, pq->dd);

			/*
			 * Delete the list entry only after writing the pending
			 * command - otherwise, the "empty list" optimization
			 * might see an empty list, and write an out of order
			 * command while we are still processing
			 */
			spin_lock_irqsave(&pq->lock, flags);
			list_del(&p->list);
			spin_unlock_irqrestore(&pq->lock, flags);

			/* We might be working here a really long time */
			cond_resched();

			if (p->wait) {
				/* To sync with rmb in _hfi_pend_cmd_queue */
				wmb();
				complete(&p->completion);
			} else {
				kmem_cache_free(pq->cache, p);
			}

			spin_lock_irqsave(&pq->lock, flags);
		}
		spin_unlock_irqrestore(&pq->lock, flags);
	}

	/* Let all callers know that we're done */
	spin_lock_irqsave(&pq->lock, flags);
	list_for_each_entry_safe(p, next, &pq->pending, list) {
		list_del(&p->list);

		/* If nobody is waiting, wake nobody up */
		if (!p->wait) {
			kmem_cache_free(pq->cache, p);
			continue;
		}

		p->ret = -EINTR;

		/* To sync with rmb in _hfi_pend_cmd_queue */
		wmb();

		complete(&p->completion);
	}
	spin_unlock_irqrestore(&pq->lock, flags);

	return 0;
}

static void hfi_stop_pend_cmdq_thread(struct hfi_pend_queue *pq)
{
	int ret;

	if (IS_ERR_OR_NULL(pq->thread)) {
		pq->thread = NULL;
		return;
	}

	ret = send_sig(SIGINT, pq->thread, 0);
	if (ret)
		dd_dev_dbg(pq->dd, "%s: Thread already exited\n", __func__);

	kthread_stop(pq->thread);

	pq->thread = NULL;
}

int hfi_pend_cmdq_info_alloc(struct hfi_devdata *dd, struct hfi_pend_queue *pq,
			     char *name)
{
	init_waitqueue_head(&pq->event);
	INIT_LIST_HEAD(&pq->pending);
	spin_lock_init(&pq->lock);

	pq->thread = kthread_run(hfi_pend_cmdq_thread, pq,
				 "hfi2_cmdq%d-%s", dd->unit, name);
	if (IS_ERR(pq->thread)) {
		int ret = PTR_ERR(pq->thread);

		dd_dev_err(dd, "Unable to create priv cmdq kthread: %d\n", ret);
		return ret;
	}

	pq->cache = KMEM_CACHE(hfi_pend_cmd, 0);
	if (!pq->cache) {
		dd_dev_err(dd, "Unable to create priv cmdq cache\n");
		hfi_stop_pend_cmdq_thread(pq);
		return -ENOMEM;
	}

	pq->dd = dd;

	return 0;
}

void hfi_pend_cmdq_info_free(struct hfi_pend_queue *pq)
{
	/* The thread will clean up the list and notify callers */
	hfi_stop_pend_cmdq_thread(pq);

	kmem_cache_destroy(pq->cache);
}

static inline
int hfi_pt_write_cmd_pending(struct hfi_pend_queue *pq, struct hfi_ctx *ctx,
			     struct hfi_cmdq *rx_cmdq, enum rx_cq_cmd cmdq_cmd,
			     u8 ni, u32 pt_idx, u64 *val, u64 user_data)
{
	union hfi_rx_cq_command cmd;
	struct hfi_eq *eq;
	int cmd_slots;

	/*
	 * Update the PT Entry in the FXR cache by issuing a command to the
	 * RX CMDQ for the FW to execute
	 *
	 * The HAS States:
	 *  [TargetStruct] <- ([TargetStruct] & ~[Mask]) | ([Payload] & [Mask])
	 */
	cmd_slots = hfi_format_rx_write64(ctx,
					  ni,
					  pt_idx,
					  cmdq_cmd,
					  HFI_CT_NONE,	/* Not used */
					  val,		/* payload[0-3] */
					  user_data,
					  HFI_GEN_CC,
					  &cmd);
	eq = HFI_EQ_ZERO(ctx, 0);
	return hfi_pend_cmd_queue_wait(pq, rx_cmdq, eq, &cmd, cmd_slots);
}

static inline
int hfi_pt_issue_cmd_pending(struct hfi_pend_queue *pq, struct hfi_ctx *ctx,
			     struct hfi_cmdq *rx_cmdq, enum rx_cq_cmd cmdq_cmd,
			     u8 ni, u32 pt_idx, u64 *val)
{
	int rc;
	u64 done = 0;

	/* Issue the PT_WRITE command via the RX CMDQ */
	rc = hfi_pt_write_cmd_pending(pq, ctx, rx_cmdq, cmdq_cmd, ni, pt_idx,
				      val, (u64)&done);

	if (rc < 0)
		return -EIO;

	/* Busy poll for the PT_WRITE command completion on EQD=0 (NI=0) */
	rc = hfi_eq_poll_cmd_complete_timeout(ctx, &done);

	return rc;
}

int hfi_pt_update_pending(struct hfi_pend_queue *pq, struct hfi_ctx *ctx,
			  struct hfi_cmdq *rx_cmdq, u16 eager_head)
{
	union ptentry_fp0_bc1_et0 ptentry;
	int rc;

	if (HFI_CTX_INVALID(ctx))
		return -ENODEV;
	if (!rx_cmdq)
		return -EINVAL;

	/* Initialize the PT entry */
	ptentry.val[0] = 0;
	ptentry.val[1] = 0;
	ptentry.enable = 1;
	ptentry.eager_head = eager_head;
	/* Mask for enable the PT entry */
	ptentry.val[2] = PTENTRY_FP0_BC1_ET0_ENABLE_MASK;
	/* Mask for just the PT entry EagerHead */
	ptentry.val[3] = PTENTRY_FP0_BC1_ET0_EAGER_HEAD_MASK;

	rc = hfi_pt_issue_cmd_pending(pq, ctx, rx_cmdq, PT_UPDATE_LOWER,
				      HFI_NI_BYPASS, HFI_PT_BYPASS_EAGER,
				      &ptentry.val[0]);
	if (rc < 0)
		return -EIO;

	return 0;
}

int hfi_pt_disable_pending(struct hfi_pend_queue *pq, struct hfi_ctx *ctx,
			   struct hfi_cmdq *rx_cmdq, u8 ni, u32 pt_idx)
{
	union ptentry_fp0 ptentry;
	int rc;

	if (HFI_CTX_INVALID(ctx))
		return -ENODEV;
	if (ni >= HFI_NUM_NIS || !rx_cmdq)
		return -EINVAL;
	if (pt_idx == HFI_PT_ANY && pt_idx >= HFI_NUM_PT_ENTRIES)
		return -EINVAL;

	/*
	 * Initialise the PT entry
	 */
	ptentry.val[0] = 0;
	ptentry.val[1] = 0;
	ptentry.val[3] = 0;
	ptentry.enable = 0;

	/* Mask for just the PT entry enable bit. */
	ptentry.val[2] = PTENTRY_FP0_ENABLE_MASK;

	/* Issue the PT_UPDATE_LOWER command via the RX CMDQ */
	rc = hfi_pt_issue_cmd_pending(pq, ctx, rx_cmdq, PT_UPDATE_LOWER, ni,
				      pt_idx, &ptentry.val[0]);

	if (rc < 0)
		return -EIO;

	return 0;
}

int hfi_pt_alloc_eager_pending(struct hfi_pend_queue *pq, struct hfi_ctx *ctx,
			       struct hfi_cmdq *rx_cmdq,
			       struct hfi_pt_alloc_eager_args *args)
{
	union ptentry_fp0_bc1_et0 ptentry;
	int rc;

	rc = hfi_pt_eager_entry_init(&ptentry, args);
	if (rc)
		return rc;

	ptentry.val[2] = ~0ULL;
	ptentry.val[3] = ~0ULL;

	rc = hfi_pt_issue_cmd_pending(pq, ctx, rx_cmdq, PT_UPDATE_LOWER_PRIV,
				      HFI_NI_BYPASS, HFI_PT_BYPASS_EAGER,
				      &ptentry.val[0]);
	if (rc < 0)
		return -EIO;

	return 0;
}
