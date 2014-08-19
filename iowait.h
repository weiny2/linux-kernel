#ifndef _HFI_IOWAIT_H
#define _HFI_IOWAIT_H
/*
 * Copyright (c) 2014 Intel Corporation. All rights reserved.
 * Copyright (c) 2006, 2007, 2008, 2009 QLogic Corporation. All rights reserved.
 * Copyright (c) 2003, 2004, 2005, 2006 PathScale, Inc. All rights reserved.
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

#include <linux/list.h>
#include <linux/workqueue.h>

/*
 * typedef (*restart_t)() - restart callback
 * @work: pointer to work structure
 */
typedef void (*restart_t)(struct work_struct *work);

struct sdma_txreq;
/**
 * struct iowait - linkage for delayed progress/waiting
 * @list: used to add/insert into QP/PQ wait lists
 * @tx_head: overflow list of sdma_txreq's
 * @sleep: nospace callback
 * @wakeup: space callback
 * @iowork: workqueue overhead
 * @sdma_busy: # of packets inflight
 * @count: total number of descriptors in tx_head'ed list
 * @tx_limit: limit for overflow queuing
 * @tx_count: number of tx entry's in tx_head'ed list
 *
 * This is to be embedded in user's state structure
 * (QP or PQ).
 *
 * The sleep and wakeup members are a
 * bit misnamed.   They do not strictly
 * speaking sleep or wakeup, but they
 * are callbacks for the ULP to implement
 * what ever queuing/dequeuing of
 * the embedded iowait and its containing struct
 * when a resource shortage like SDMA ring space is seen.
 *
 * Both potentially have locks help
 * so sleeping is not allowed.
 */

struct iowait {
	struct list_head list;
	struct list_head tx_head;
	void (*sleep)(struct iowait *wait, struct sdma_txreq *tx);
	void (*wakeup)(struct iowait *wait, int reason);
	struct work_struct iowork;
	atomic_t sdma_busy;
	u32 count;
	u32 tx_limit;
	u32 tx_count;
};

#define SDMA_AVAIL_REASON 0

/**
 * iowait_init() - init wait structure
 * @wait: wait struct to initialize
 * @tx_limit: limit for overflow queueing
 * @func: restart function for workqueue
 * @sleep: sleep function for no space
 * @wakeup: wakeup function for no space
 *
 * This function initializes the iowait
 * structure embedded in the QP or PQ.
 *
 */

static inline void iowait_init(
	struct iowait *wait,
	u32 tx_limit,
	void (*func)(struct work_struct *work),
	void (*sleep)(struct iowait *wait, struct sdma_txreq *tx),
	void (*wakeup)(struct iowait *wait, int reason))
{
	wait->count = 0;
	INIT_LIST_HEAD(&wait->list);
	INIT_LIST_HEAD(&wait->tx_head);
	INIT_WORK(&wait->iowork, func);
	atomic_set(&wait->sdma_busy, 0);
	wait->tx_limit = tx_limit;
	wait->sleep = sleep;
	wait->wakeup = wakeup;
}

/**
 * iowait_schedule() - init wait structure
 * @wait: wait struct to schedule
 * @wq: workqueue for schedule
 */
static inline void iowait_schedule(
	struct iowait *wait,
	struct workqueue_struct *wq)
{
	queue_work(wq, &wait->iowork);
}

#endif
