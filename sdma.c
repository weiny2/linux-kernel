/*
 * Copyright (c) 2012,2013,2014 Intel Corporation. All rights reserved.
 * Copyright (c) 2007 - 2012 QLogic Corporation. All rights reserved.
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

#include <linux/spinlock.h>
#include <linux/netdevice.h>
#include <linux/moduleparam.h>

#include "hfi.h"
#include "common.h"
#include "qp.h"
#include "sdma.h"
#include "iowait.h"
#include "trace.h"

/* must be a power of 2 >= 64 <= 32768 */
#define SDMA_DESCQ_CNT 1024
/* default pio off, sdma on */
uint sdma_descq_cnt = SDMA_DESCQ_CNT;
module_param(sdma_descq_cnt, uint, S_IRUGO);
MODULE_PARM_DESC(sdma_descq_cnt, "Number of SDMA descq entries");

static uint sdma_idle_cnt = 64;
module_param(sdma_idle_cnt, uint, S_IRUGO);
MODULE_PARM_DESC(sdma_idle_cnt, "sdma interrupt idle delay (default 64)");

uint mod_num_sdma;
module_param_named(num_sdma, mod_num_sdma, uint, S_IRUGO);
MODULE_PARM_DESC(num_sdma, "Set max number SDMA engines to use");

#define SDMA_WAIT_BATCH_SIZE 20

static const char * const sdma_state_names[] = {
	[sdma_state_s00_hw_down]                = "s00_HwDown",
	[sdma_state_s10_hw_start_up_halt_wait]  = "s10_HwStartUpHaltWait",
	[sdma_state_s15_hw_start_up_clean_wait] = "s15_HwStartUpCleanWait",
	[sdma_state_s20_idle]                   = "s20_Idle",
	[sdma_state_s30_sw_clean_up_wait]       = "s30_SwCleanUpWait",
	[sdma_state_s40_hw_clean_up_wait]       = "s40_HwCleanUpWait",
	[sdma_state_s50_hw_halt_wait]           = "s50_HwHaltWait",
	[sdma_state_s99_running]                = "s99_Running",
};

static const char * const sdma_event_names[] = {
	[sdma_event_e00_go_hw_down]   = "e00_GoHwDown",
	[sdma_event_e10_go_hw_start]  = "e10_GoHwStart",
	[sdma_event_e15_hw_started1]  = "e15_HwStarted1",
	[sdma_event_e25_hw_started2]  = "e25_HwStarted2",
	[sdma_event_e30_go_running]   = "e30_GoRunning",
	[sdma_event_e40_sw_cleaned]   = "e40_SwCleaned",
	[sdma_event_e50_hw_cleaned]   = "e50_HwCleaned",
	[sdma_event_e60_hw_halted]    = "e60_HwHalted",
	[sdma_event_e70_go_idle]      = "e70_GoIdle",
	[sdma_event_e7220_err_halted] = "e7220_ErrHalted",
	[sdma_event_e7322_err_halted] = "e7322_ErrHalted",
	[sdma_event_e90_timer_tick]   = "e90_TimerTick",
};

static const struct sdma_set_state_action sdma_action_table[] = {
	[sdma_state_s00_hw_down] = {
		.go_s99_running_tofalse = 1,
		.op_enable = 0,
		.op_intenable = 0,
		.op_halt = 0,
		.op_cleanup = 0,
		.op_drain = 0,
	},
	[sdma_state_s10_hw_start_up_halt_wait] = {
		.op_enable = 0,
		.op_intenable = 1,
		.op_halt = 1,
		.op_cleanup = 0,
		.op_drain = 0,
	},
	[sdma_state_s15_hw_start_up_clean_wait] = {
		.op_enable = 0,
		.op_intenable = 1,
		.op_halt = 0,
		.op_cleanup = 1,
		.op_drain = 0,
	},
	[sdma_state_s20_idle] = {
		.op_enable = 0,
		.op_intenable = 1,
		.op_halt = 0,
		.op_cleanup = 0,
		.op_drain = 0,
	},
	[sdma_state_s30_sw_clean_up_wait] = {
		.op_enable = 0,
		.op_intenable = 1,
		.op_halt = 0,
		.op_cleanup = 0,
		.op_drain = 0,
	},
	[sdma_state_s40_hw_clean_up_wait] = {
		.op_enable = 1,
		.op_intenable = 1,
		.op_halt = 0,
		.op_cleanup = 0,
		.op_drain = 0,
	},
	[sdma_state_s50_hw_halt_wait] = {
		.op_enable = 1,
		.op_intenable = 1,
		.op_halt = 1,
		.op_cleanup = 0,
		.op_drain = 1,
	},
	[sdma_state_s99_running] = {
		.op_enable = 1,
		.op_intenable = 1,
		.op_halt = 0,
		.op_cleanup = 0,
		.op_drain = 0,
		.go_s99_running_totrue = 1,
	},
};

/* declare all statics here rather than keep sorting */
static void sdma_complete(struct kref *);
static void sdma_finalput(struct sdma_state *);
static void sdma_get(struct sdma_state *);
static void sdma_hw_clean_up_task(unsigned long);
static void sdma_put(struct sdma_state *);
static void sdma_set_state(struct sdma_engine *, enum sdma_states);
static void sdma_start_hw_clean_up(struct sdma_engine *);
static void sdma_start_sw_clean_up(struct sdma_engine *);
static void sdma_sw_clean_up_task(unsigned long);
static void unmap_desc(struct sdma_engine *, unsigned);
static void sdma_sendctrl(struct sdma_engine *, unsigned);
static void init_sdma_regs(struct sdma_engine *, u32);
static void sdma_process_event(
	struct sdma_engine *sde,
	enum sdma_events event);
static void __sdma_process_event(
	struct sdma_engine *sde,
	enum sdma_events event);
static void dump_sdma_state(struct sdma_engine *sde);
static int sdma_make_progress(struct sdma_engine *sde);

/**
 * sdma_event_name() - return event string from enum
 * @event: event
 */
const char *sdma_event_name(enum sdma_events event)
{
	return sdma_event_names[event];
}

/**
 * sdma_state_name() - return state string from enum
 * @state: state
 */
const char *sdma_state_name(enum sdma_states state)
{
	return sdma_state_names[state];
}

static void sdma_get(struct sdma_state *ss)
{
	kref_get(&ss->kref);
}

static void sdma_complete(struct kref *kref)
{
	struct sdma_state *ss =
		container_of(kref, struct sdma_state, kref);

	complete(&ss->comp);
}

static void sdma_put(struct sdma_state *ss)
{
	kref_put(&ss->kref, sdma_complete);
}

static void sdma_finalput(struct sdma_state *ss)
{
	sdma_put(ss);
	wait_for_completion(&ss->comp);
}

static inline void write_sde_csr(
	struct sdma_engine *sde,
	u32 offset0,
	u64 value)
{
	write_kctxt_csr(sde->dd, sde->this_idx, offset0, value);
}

static inline u64 read_sde_csr(
	struct sdma_engine *sde,
	u32 offset0)
{
	return read_kctxt_csr(sde->dd, sde->this_idx, offset0);
}


/*
 * Complete all the sdma requests on the active list, in the correct
 * order, and with appropriate processing.   Called when cleaning up
 * after sdma shutdown, and when new sdma requests are submitted for
 * a link that is down.   This matches what is done for requests
 * that complete normally, it's just the full list.
 *
 * Must be called with sdma_lock held
 */
/* new API version */
static void clear_sdma_activelist(struct sdma_engine *sde)
{
	struct sdma_txreq *txp, *txp_next;

	list_for_each_entry_safe(txp, txp_next, &sde->activelist, list) {
		list_del_init(&txp->list);
		/* FIXME - old api code fragment */
		if (txp->flags & SDMA_TXREQ_F_FREEDESC) {
			unsigned idx;

			idx = txp->start_idx;
			while (idx != txp->next_descq_idx) {
				unmap_desc(sde, idx);
				if (++idx == sde->descq_cnt)
					idx = 0;
			}
		} else {
			sdma_txclean(sde->dd, txp);
		}
		if (txp->complete)
			(*txp->complete)(txp, SDMA_TXREQ_S_ABORTED);
	}
}

static void sdma_hw_clean_up_task(unsigned long opaque)
{
	struct sdma_engine *sde = (struct sdma_engine *) opaque;
	u64 statuscsr;

	while (1) {
#ifdef JAG_SDMA_VERBOSITY
		dd_dev_err(sde->dd, "JAG SDMA(%u) %s:%d %s()\n",
			sde->this_idx, slashstrip(__FILE__), __LINE__,
			__func__);
#endif
		statuscsr = read_sde_csr(sde, WFR_SEND_DMA_STATUS);
		statuscsr &= WFR_SEND_DMA_STATUS_ENG_CLEANED_UP_SMASK;
		if (statuscsr) break;
		msleep(100);  // JAG SDMA too long, but ok for now with chatty debug
	}

	sdma_process_event(sde, sdma_event_e25_hw_started2);
}

static void sdma_sw_clean_up_task(unsigned long opaque)
{
	struct sdma_engine *sde = (struct sdma_engine *) opaque;
	unsigned long flags;

	spin_lock_irqsave(&sde->lock, flags);

	/*
	 * At this point, the following should always be true:
	 * - We are halted, so no more descriptors are getting retired.
	 * - We are not running, so no one is submitting new work.
	 * - Only we can send the e40_sw_cleaned, so we can't start
	 *   running again until we say so.  So, the active list and
	 *   descq are ours to play with.
	 */

	spin_lock(&sde->dd->pport->sdma_alllock);

	/* Process all retired requests. */
	sdma_make_progress(sde);

	clear_sdma_activelist(sde);

	spin_unlock(&sde->dd->pport->sdma_alllock);

	/*
	 * Resync count of added and removed.  It is VERY important that
	 * sdma_descq_removed NEVER decrement - user_sdma depends on it.
	 */
	sde->descq_removed = sde->descq_added;

	/*
	 * Reset our notion of head and tail.
	 * Note that the HW registers will be reset when switching states
	 * due to calling __qib_sdma_process_event() below.
	 */
	sde->descq_tail = 0;
	sde->descq_head = 0;
	*sde->head_dma = 0;
	sde->generation = 0;

	__sdma_process_event(sde, sdma_event_e40_sw_cleaned);

	spin_unlock_irqrestore(&sde->lock, flags);
}

static void sdma_sw_tear_down(struct sdma_engine *sde)
{
	struct sdma_state *ss = &sde->state;

	/* Releasing this reference means the state machine has stopped. */
	sdma_put(ss);
}

static void sdma_start_hw_clean_up(struct sdma_engine *sde)
{
	tasklet_hi_schedule(&sde->sdma_hw_clean_up_task);
}

static void sdma_start_sw_clean_up(struct sdma_engine *sde)
{
	tasklet_hi_schedule(&sde->sdma_sw_clean_up_task);
}

static void sdma_set_state(struct sdma_engine *sde,
	enum sdma_states next_state)
{
	struct sdma_state *ss = &sde->state;
	const struct sdma_set_state_action *action = sdma_action_table;
	unsigned op = 0;

	/* debugging bookkeeping */
	ss->previous_state = ss->current_state;
	ss->previous_op = ss->current_op;

	ss->current_state = next_state;

	if (action[next_state].op_enable)
		op |= QIB_SDMA_SENDCTRL_OP_ENABLE;

	if (action[next_state].op_intenable)
		op |= QIB_SDMA_SENDCTRL_OP_INTENABLE;

	if (action[next_state].op_halt)
		op |= QIB_SDMA_SENDCTRL_OP_HALT;

	if (action[next_state].op_cleanup)
		op |= QIB_SDMA_SENDCTRL_OP_CLEANUP;

	if (action[next_state].op_drain)
		op |= QIB_SDMA_SENDCTRL_OP_DRAIN;

	if (action[next_state].go_s99_running_tofalse)
		ss->go_s99_running = 0;

	if (action[next_state].go_s99_running_totrue)
		ss->go_s99_running = 1;

	ss->current_op = op;

	sdma_sendctrl(sde, ss->current_op);
}

/* FIXME - old routine */
static void unmap_desc(struct sdma_engine *sde, unsigned head)
{
	__le64 *descqp;
	u64 desc[2];
	dma_addr_t addr;
	size_t len;

	descqp = &sde->descq[head].qw[0];

	desc[0] = le64_to_cpu(descqp[0]);
	desc[1] = le64_to_cpu(descqp[1]);

	/* JAG SDMA TODO - these should only be in wfr.c */
	addr = (desc[0] >> SDMA_DESC0_PHY_ADDR_SHIFT) &
		SDMA_DESC0_PHY_ADDR_MASK;
	len = (desc[0] >> SDMA_DESC0_BYTE_COUNT_SHIFT) &
		SDMA_DESC0_BYTE_COUNT_MASK;
	dma_unmap_single(&sde->dd->pcidev->dev, addr, len, DMA_TO_DEVICE);
}

/**
 * sdma_get_descq_cnt() - called when device probed
 *
 * Return a validated descq count.
 *
 * This is currently only used in the verbs initialization to build the tx
 * list.
 *
 * This will probably be deleted in favor of a more scalable approach to
 * alloc tx's.
 *
 */
u16 sdma_get_descq_cnt(void)
{
	u16 count = sdma_descq_cnt;
	if (!count)
		return SDMA_DESCQ_CNT;
	/* count must be a power of 2 greater than 64 and less than
	 * 32768.   Otherwise return default.
	 */
	if (!is_power_of_2(count))
		return SDMA_DESCQ_CNT;
	if (count < 64 && count > 32768)
		return SDMA_DESCQ_CNT;
	return count;
}
/**
 * sdma_select_engine_vl() - select sdma engine
 * @dd: devdata
 * @selector: a spreading factor
 * @vl: this vl
 *
 *
 * This function returns an engine based on the selector and a vl.  The
 * mapping fields are protected by a seqlock with sdma_map_init().
 */
struct sdma_engine *sdma_select_engine_vl(
	struct hfi_devdata *dd,
	u32 selector,
	u8 vl)
{
	unsigned seq;
	u32 idx;

	BUG_ON(vl > 8);
	do {
		seq = read_seqbegin(&dd->sde_map_lock);
		idx = (
			(selector & dd->selector_sdma_mask)
				<< dd->selector_sdma_shift) +
			vl;
	} while (read_seqretry(&dd->sde_map_lock, seq));
	trace_hfi_sdma_engine_select(dd, selector, vl, idx);
	return dd->sdma_map[idx];
}

/**
 * sdma_select_engine_sc() - select sdma engine
 * @dd: devdata
 * @selector: a spreading factor
 * @sc5: the 5 bit sc
 *
 *
 * This function returns an engine based on the selector and an sc.
 */
struct sdma_engine *sdma_select_engine_sc(
	struct hfi_devdata *dd,
	u32 selector,
	u8 sc5)
{
	u8 vl = sc_to_vlt(dd, sc5);
	return sdma_select_engine_vl(dd, selector, vl);
}

/**
 * sdma_map_init - called when # vls change
 * @dd: hfi_devdata
 * @port: port number
 * @num_vls: number of vls
 *
 * This routine changes the mapping based on the number of vls.
 *
 * A seqlock is used here to control access to the mapping fields.
 */
void sdma_map_init(struct hfi_devdata *dd, u8 port, u8 num_vls)
{
	int i;
	struct qib_pportdata *ppd = dd->pport + port;
	int sde_per_vl;

	if (!(dd->flags & QIB_HAS_SEND_DMA))
		return;
	BUG_ON(num_vls > hfi_num_vls(ppd->vls_supported));
	sde_per_vl = dd->num_sdma / num_vls;
	write_seqlock_irq(&dd->sde_map_lock);
	dd->selector_sdma_mask = sde_per_vl - 1;
	dd->selector_sdma_shift = ilog2(num_vls);
	/*
	 * There is an opportunity here for
	 * a more sophisticated mapping where
	 * the loading of vl per engine is not
	 * uniform.
	 *
	 * for now just 1 - 1 the per_sdma.
	 */
	for (i = 0; i < dd->num_sdma; i++)
		dd->sdma_map[i] = &dd->per_sdma[i];
	write_sequnlock_irq(&dd->sde_map_lock);
	dd_dev_info(dd, "SDMA map mask 0x%x sde per vl %u\n",
		    dd->selector_sdma_mask, sde_per_vl);
}

/**
 * sdma_init() - called when device probed
 * @dd: hfi_devdata
 * @port: port number (currently only zero)
 * @num_engines: number of engines to initialize
 *
 * sdma_init initializes the specified number of engines.
 *
 * The code initializes each sde, its csrs.  Interrupts
 * are not required to be enabled.
 *
 * Returns:
 * 0 - success, -errno on failure
 */
int sdma_init(struct hfi_devdata *dd, u8 port, size_t num_engines)
{
	unsigned this_idx;
	struct sdma_engine *sde;
	u16 descq_cnt;
	void *curr_head;
	unsigned long flags;
	struct qib_pportdata *ppd = dd->pport + port;
	u32 per_sdma_credits =
		dd->chip_sdma_mem_size/(num_engines * WFR_SDMA_BLOCK_SIZE);

	descq_cnt = sdma_get_descq_cnt();
	dd_dev_info(dd, "SDMA engines %zu descq_cnt %u\n",
		num_engines, descq_cnt);

	/* alloc memory for array of send engines */
	dd->per_sdma = kzalloc(num_engines * sizeof(*dd->per_sdma),
			       GFP_KERNEL);
	if (!dd->per_sdma)
		return -ENOMEM;

	dd->sdma_map = kzalloc(num_engines *
			       sizeof(struct sdma_engine **),
			       GFP_KERNEL);
	if (!dd->sdma_map)
		goto nomem;

	/* Allocate memory for SendDMA descriptor FIFOs */
	for (this_idx = 0; this_idx < num_engines; ++this_idx) {
		sde = &dd->per_sdma[this_idx];
		sde->dd = dd;
		sde->ppd = ppd;
		sde->this_idx = this_idx;
		sde->descq_cnt = descq_cnt;

		spin_lock_init(&sde->lock);
		spin_lock_init(&sde->senddmactrl_lock);

		spin_lock_irqsave(&sde->lock, flags);
		sdma_set_state(sde, sdma_state_s00_hw_down);
		spin_unlock_irqrestore(&sde->lock, flags);

		/* set up reference counting */
		kref_init(&sde->state.kref);
		init_completion(&sde->state.comp);

		INIT_LIST_HEAD(&sde->activelist);
		INIT_LIST_HEAD(&sde->dmawait);

		if (dd->flags & QIB_HAS_SDMA_TIMEOUT)
			sde->default_desc1 =
				cpu_to_le64(SDMA_DESC1_HEAD_TO_HOST_FLAG);
		else
			sde->default_desc1 =
				cpu_to_le64(SDMA_DESC1_INT_REQ_FLAG);

		tasklet_init(&sde->sdma_hw_clean_up_task, sdma_hw_clean_up_task,
			(unsigned long)sde);

		tasklet_init(&sde->sdma_sw_clean_up_task, sdma_sw_clean_up_task,
			(unsigned long)sde);

		sde->descq = dma_alloc_coherent(
			&dd->pcidev->dev,
			descq_cnt * sizeof(u64[2]),
			&sde->descq_phys,
			GFP_KERNEL
		);
		if (!sde->descq)
			goto cleanup_descq;


	}

	dd->sdma_heads_size = L1_CACHE_BYTES * num_engines;
	/* Allocate memory for DMA of head registers to memory */
	dd->sdma_heads_dma = dma_alloc_coherent(
		&dd->pcidev->dev,
		dd->sdma_heads_size,
		&dd->sdma_heads_phys,
		GFP_KERNEL
	);
	if (!dd->sdma_heads_dma)
		goto heads_fail;

	memset((void *)dd->sdma_heads_dma, 0, dd->sdma_heads_size);
	/* assign each engine to different cacheline and init registers */
	curr_head = (void *)dd->sdma_heads_dma;
	for (this_idx = 0; this_idx < num_engines; ++this_idx) {
		unsigned long phys_offset;

		sde = &dd->per_sdma[this_idx];

		sde->head_dma = curr_head;
		curr_head += L1_CACHE_BYTES;
		phys_offset = (unsigned long)sde->head_dma -
			      (unsigned long)dd->sdma_heads_dma;
		sde->head_phys = dd->sdma_heads_phys + phys_offset;
		init_sdma_regs(sde, per_sdma_credits);
	}
	dd->flags |= QIB_HAS_SEND_DMA;
	dd->flags |= sdma_idle_cnt ? QIB_HAS_SDMA_TIMEOUT : 0;
	dd->num_sdma = num_engines;
	seqlock_init(&dd->sde_map_lock);
	sdma_map_init(dd, port, hfi_num_vls(ppd->vls_operational));
	return 0;

heads_fail:
	dd_dev_err(dd, "failed to allocate SendDMA head memory\n");
cleanup_descq:
	for (this_idx = 0; this_idx < num_engines; ++this_idx) {
		sde = &dd->per_sdma[this_idx];
		if (sde->descq) {
			dma_free_coherent(
				&dd->pcidev->dev,
				descq_cnt * sizeof(u64[2]),
				(void *)sde->descq,
				sde->descq_phys
			);
			sde->descq = NULL;
			sde->descq_phys = 0;
		} else {
			dd_dev_err(dd, "failed to allocate SendDMA descriptor FIFO memory\n");
			break;
		}
	}
	kfree(dd->sdma_map);
	dd->sdma_map = NULL;
nomem:
	kfree(dd->per_sdma);
	dd->per_sdma = NULL;
	return -ENOMEM;
}

/**
 * sdma_start() - called to kick off state processing for all engines
 * @dd: hfi_devdata
 *
 * This routine is for kicking off the state processing for all required
 * sdma engines.  Interrupts need to be working at this point.
 *
 */
void sdma_start(struct hfi_devdata *dd)
{
	unsigned i;
	struct sdma_engine *sde;

	/* kick off the engines state processing */
	for (i = 0; i < dd->num_sdma; ++i) {

		sde = &dd->per_sdma[i];
		sdma_process_event(sde, sdma_event_e10_go_hw_start);
		/* FIXME - need a better way to do this */
		sdma_process_event(sde, sdma_event_e30_go_running);
	}
}

/**
 * sdma_exit() - used when module is removed
 * @dd: hfi_devdata
 */
void sdma_exit(struct hfi_devdata *dd)
{
	unsigned this_idx;
	struct sdma_engine *sde;

	for (this_idx = 0; dd->per_sdma && this_idx < dd->num_sdma;
			++this_idx) {

		sde = &dd->per_sdma[this_idx];
		if (!list_empty(&sde->dmawait))
			dd_dev_err(dd, "sde %u: dmawait list not empty!\n",
				sde->this_idx);
		sdma_process_event(sde, sdma_event_e00_go_hw_down);

		/*
		 * This waits for the state machine to exit so it is not
		 * necessary to kill the sdma_sw_clean_up_task to make sure
		 * it is not running.
		 */
		sdma_finalput(&sde->state);
	}
	if (dd->sdma_heads_dma) {
		dma_free_coherent(&dd->pcidev->dev, dd->sdma_heads_size,
				  (void *)dd->sdma_heads_dma,
				  dd->sdma_heads_phys);
		dd->sdma_heads_dma = NULL;
		dd->sdma_heads_phys = 0;
	}
	for (this_idx = 0; this_idx < dd->num_sdma; ++this_idx) {
		sde = &dd->per_sdma[this_idx];

		sde->head_dma = NULL;
		sde->head_phys = 0;

		if (sde->descq) {
			dma_free_coherent(
				&dd->pcidev->dev,
				sde->descq_cnt * sizeof(u64[2]),
				sde->descq,
				sde->descq_phys
			);
			sde->descq = NULL;
			sde->descq_phys = 0;
		}
	}
	kfree(dd->sdma_map);
	dd->sdma_map = NULL;
	kfree(dd->per_sdma);
	dd->per_sdma = NULL;
}

/**
 * sdma_txclean() - clean tx of mappings, descp *kmalloc's
 * @dd: hfi_devdata for unmapping
 * @tx: tx request to clean
 *
 * This is used in the progress routine to clean the tx.
 */
void sdma_txclean(
	struct hfi_devdata *dd,
	struct sdma_txreq *tx)
{
	u16 i;

	kfree(tx->coalesce_buf);
	/* free mappings */
	for (i = 0; i < tx->num_desc; i++) {
		/* TODO - skip over AHG descriptors */
		switch (sdma_mapping_type(&tx->descp[i])) {
		case SDMA_MAP_SINGLE:
			dma_unmap_single(
				&dd->pcidev->dev,
				sdma_mapping_addr(&tx->descp[i]),
				sdma_mapping_len(&tx->descp[i]),
				DMA_TO_DEVICE);
			break;
		case SDMA_MAP_PAGE:
			dma_unmap_page(
				&dd->pcidev->dev,
				sdma_mapping_addr(&tx->descp[i]),
				sdma_mapping_len(&tx->descp[i]),
				DMA_TO_DEVICE);
			break;
		}
	}
	/* kmalloc'ed descp */
	if (unlikely(tx->desc_limit > ARRAY_SIZE(tx->descs)))
		kfree(tx->descp);
}

static inline void make_sdma_desc(struct sdma_engine *sde,
				  u64 *sdmadesc, u64 addr, u64 dwlen)
{
	u64 bytelen = dwlen * 4ULL;
	u64 generation = sde->generation;

	WARN_ON(addr & 3);

	/* SDmaByteCount[61:48] */
	sdmadesc[0] = (bytelen & SDMA_DESC0_BYTE_COUNT_MASK) << SDMA_DESC0_BYTE_COUNT_SHIFT;
	/* SDmaPhyAddr[47:0] */
	sdmadesc[0] |= (addr & SDMA_DESC0_PHY_ADDR_MASK) << SDMA_DESC0_PHY_ADDR_SHIFT;

	sdmadesc[1] = (generation & SDMA_DESC1_GENERATION_MASK) << SDMA_DESC1_GENERATION_SHIFT;
}

static u16 sdma_gethead(struct sdma_engine *sde)
{
	struct hfi_devdata *dd = sde->dd;
	int sane;
	int use_dmahead;
	u16 swhead;
	u16 swtail;
	u16 cnt;
	u16 hwhead;

#ifdef JAG_SDMA_VERBOSITY
	dd_dev_err(sde->dd, "JAG SDMA(%u) %s:%d %s()\n",
		sde->this_idx, slashstrip(__FILE__), __LINE__, __func__);
#endif

	use_dmahead = __sdma_running(sde) &&
		(dd->flags & QIB_HAS_SDMA_TIMEOUT);
retry:
	hwhead = use_dmahead ?
		(u16) le64_to_cpu(*sde->head_dma) :
		(u16) read_sde_csr(sde, WFR_SEND_DMA_HEAD);

	swhead = sde->descq_head;
	/* this code is really bad for cache line trading */
	swtail = sde->descq_tail;
	cnt = sde->descq_cnt;

	if (swhead < swtail)
		/* not wrapped */
		sane = (hwhead >= swhead) & (hwhead <= swtail);
	else if (swhead > swtail)
		/* wrapped around */
		sane = ((hwhead >= swhead) && (hwhead < cnt)) ||
			(hwhead <= swtail);
	else
		/* empty */
		sane = (hwhead == swhead);

	if (unlikely(!sane)) {
		dd_dev_err(dd, "IB%u:%u bad head %s hwhd=%hu swhd=%hu swtl=%hu cnt=%hu\n",
			dd->unit, sde->ppd->port,
			use_dmahead ? "(dma)" : "(kreg)",
			hwhead, swhead, swtail, cnt);
		if (use_dmahead) {
			/* try one more time, directly from the register */
			use_dmahead = 0;
			goto retry;
		}
		/* proceed as if no progress */
		hwhead = swhead;
	}

	return hwhead;
}

/*
 * This is called when there are send DMA descriptors that might be
 * available.
 *
 * This is called with ppd->sdma_lock held.
 */
void sdma_desc_avail(struct sdma_engine *sde, unsigned avail)
{
	struct iowait *wait, *nw;
	struct iowait *waits[SDMA_WAIT_BATCH_SIZE];
	unsigned i, n;
	struct sdma_txreq *stx;
	struct qib_ibdev *dev;

#ifdef JAG_SDMA_VERBOSITY
	dd_dev_err(sde->dd, "JAG SDMA %s:%d %s()\n", slashstrip(__FILE__),
		__LINE__, __func__);
	dd_dev_err(sde->dd, "avail: %u\n", avail);
#endif

	n = 0;

	dev = &sde->dd->verbs_dev;
	spin_lock(&dev->pending_lock);
	/* Search wait list for first QP wanting DMA descriptors. */
	list_for_each_entry_safe(wait, nw, &sde->dmawait, list) {
		if (!wait->wakeup)
			continue;
		if (n == ARRAY_SIZE(waits))
			break;
		stx = list_first_entry(&wait->tx_head, struct sdma_txreq, list);
		if (stx->num_desc > avail)
			break;
		avail -= stx->num_desc;
		list_del_init(&wait->list);
		waits[n++] = wait;
	}
	spin_unlock(&dev->pending_lock);

	for (i = 0; i < n; i++)
		waits[i]->wakeup(waits[i], SDMA_AVAIL_REASON);
}

/* sdma_lock must be held */
/* FIXME - convert to new routine */
static int sdma_make_progress(struct sdma_engine *sde)
{
	struct list_head *lp = NULL;
	struct sdma_txreq *txp = NULL;
	int progress = 0;
	u16 hwhead;
	u16 idx = 0;

#ifdef JAG_SDMA_VERBOSITY
	dd_dev_err(sde->dd, "JAG SDMA(%u) %s:%d %s()\n",
		sde->this_idx, slashstrip(__FILE__), __LINE__, __func__);
#endif

	hwhead = sdma_gethead(sde);

	/* The reason for some of the complexity of this code is that
	 * not all descriptors have corresponding txps.  So, we have to
	 * be able to skip over descs until we wander into the range of
	 * the next txp on the list.
	 */

	if (!list_empty(&sde->activelist)) {
		lp = sde->activelist.next;
		txp = list_entry(lp, struct sdma_txreq, list);
		idx = txp->start_idx;
	}

	while (sde->descq_head != hwhead) {
		/* if desc is part of this txp, unmap if needed */
		/* FIXME - old api code fragment */
		if (txp && (txp->flags & SDMA_TXREQ_F_FREEDESC) &&
		    (idx == sde->descq_head)) {
			unmap_desc(sde, sde->descq_head);
			if (++idx == sde->descq_cnt)
				idx = 0;
		}

		/* increment dequed desc count */
		sde->descq_removed++;

		/* advance head, wrap if needed */
		if (++sde->descq_head == sde->descq_cnt)
			sde->descq_head = 0;

		/* if now past this txp's descs, do the callback */
		/* FIXME - old api code fragment */
		if (txp && txp->next_descq_idx == sde->descq_head) {
			/* remove from active list */
			list_del_init(&txp->list);
			/* FIXME - old api code fragment */
			if (!(txp->flags &
					(SDMA_TXREQ_F_FREEDESC|
					 SDMA_TXREQ_F_FREEBUF)))
				sdma_txclean(sde->dd, txp);
			if (txp->complete)
				(*txp->complete)(txp, SDMA_TXREQ_S_OK);
			/* see if there is another txp */
			if (list_empty(&sde->activelist))
				txp = NULL;
			else {
				lp = sde->activelist.next;
				txp = list_entry(lp, struct sdma_txreq, list);
				idx = txp->start_idx;
			}
		}
		progress++;
	}
	if (progress)
		sdma_desc_avail(sde, sdma_descq_freecnt(sde));
	return progress;
}

/* sdma_lock must be held */
/* Old API - delete */
int qib_sdma_make_progress(struct sdma_engine *sde)
{
	return sdma_make_progress(sde);
}

/**
 * sdma_engine_interrupt() - interrupt handler for engine
 * @sde: sdma engine
 * @status: sdma interrupt reason
 *
 * Status is a mask of the 3 possible interrupts for this engine.  It will
 * contain bits _only_ for this SDMA engine.  It will contain at least one
 * bit, it may contain more.
 */
void sdma_engine_interrupt(struct sdma_engine *sde, u64 status)
{
	unsigned long flags;
#ifdef JAG_SDMA_VERBOSITY
	dd_dev_err(sde->dd, "JAG SDMA(%u) %s:%d %s()\n", sde->this_idx,
		slashstrip(__FILE__), __LINE__, __func__);
	dd_dev_err(sde->dd, "status: 0x%llx\n", status);
#endif
	spin_lock_irqsave(&sde->lock, flags);
	qib_sdma_make_progress(sde);
	spin_unlock_irqrestore(&sde->lock, flags);
}

/**
 * sdma_engine_error() - error handler for engine
 * @sde: sdma engine
 * @status: sdma interrupt reason
 */
void sdma_engine_error(struct sdma_engine *sde, u64 status)
{
	unsigned long flags;

#ifdef JAG_SDMA_VERBOSITY
	dd_dev_err(sde->dd, "JAG SDMA(%u) %s:%d %s()\n",
		sde->this_idx,
		slashstrip(__FILE__), __LINE__, __func__);
	dd_dev_err(sde->dd, "JAG SDMA (%u) status: 0x%llx state %s\n",
		sde->this_idx,
		(unsigned long long)status,
		sdma_state_names[sde->state.current_state]);
	sdma_dumpstate(sde);
#endif
	/* FIXME : need to determine level of error, perhaps recovery */
	if (status & ~WFR_SEND_DMA_ENG_ERR_STATUS_SDMA_HALT_ERR_SMASK) {
		dd_dev_err(sde->dd,
			"SDMA (%u) engine error: 0x%llx state %s\n",
			sde->this_idx,
			(unsigned long long)status,
			sdma_state_names[sde->state.current_state]);
		dump_sdma_state(sde);
	}
	spin_lock_irqsave(&sde->lock, flags);

	switch (sde->state.current_state) {
	case sdma_state_s00_hw_down:
		break;

	case sdma_state_s10_hw_start_up_halt_wait:
		if (status & WFR_SEND_DMA_ENG_ERR_STATUS_SDMA_HALT_ERR_SMASK)
			__sdma_process_event(sde, sdma_event_e15_hw_started1);
		break;

	case sdma_state_s15_hw_start_up_clean_wait:
		break;

	case sdma_state_s20_idle:
		break;

	case sdma_state_s30_sw_clean_up_wait:
		break;

	case sdma_state_s40_hw_clean_up_wait:
		break;

	case sdma_state_s50_hw_halt_wait:
		break;

	case sdma_state_s99_running:
		break;
	}

	spin_unlock_irqrestore(&sde->lock, flags);
}

static void sdma_sendctrl(struct sdma_engine *sde, unsigned op)
{
#ifdef JAG_SDMA_VERBOSITY
	struct hfi_devdata *dd = sde->dd;
#endif
	u64 set_senddmactrl = 0;
	u64 clr_senddmactrl = 0;

#ifdef JAG_SDMA_VERBOSITY
	dd_dev_err(sde->dd, "JAG SDMA(%u) %s:%d %s()\n", sde->this_idx,
		slashstrip(__FILE__), __LINE__, __func__);
	dd_dev_err(dd, "JAG SDMA senddmactrl E=%d I=%d H=%d C=%d\n",
		(op & QIB_SDMA_SENDCTRL_OP_ENABLE) ? 1 : 0,
		(op & QIB_SDMA_SENDCTRL_OP_INTENABLE) ? 1 : 0,
		(op & QIB_SDMA_SENDCTRL_OP_HALT) ? 1 : 0,
		(op & QIB_SDMA_SENDCTRL_OP_CLEANUP) ? 1 : 0);
#endif

	if (op & QIB_SDMA_SENDCTRL_OP_ENABLE)
		set_senddmactrl |= WFR_SEND_DMA_CTRL_SDMA_ENABLE_SMASK;
	else
		clr_senddmactrl |= WFR_SEND_DMA_CTRL_SDMA_ENABLE_SMASK;

	if (op & QIB_SDMA_SENDCTRL_OP_INTENABLE)
		set_senddmactrl |= WFR_SEND_DMA_CTRL_SDMA_INT_ENABLE_SMASK;
	else
		clr_senddmactrl |= WFR_SEND_DMA_CTRL_SDMA_INT_ENABLE_SMASK;

	if (op & QIB_SDMA_SENDCTRL_OP_HALT)
		set_senddmactrl |= WFR_SEND_DMA_CTRL_SDMA_HALT_SMASK;
	else
		clr_senddmactrl |= WFR_SEND_DMA_CTRL_SDMA_HALT_SMASK;

	/* JAG TODO: OP_DRAIN */

	spin_lock(&sde->senddmactrl_lock);

	/* JAG TODO: OP_DRAIN */

	sde->p_senddmactrl |= set_senddmactrl;
	sde->p_senddmactrl &= ~clr_senddmactrl;

	if (op & QIB_SDMA_SENDCTRL_OP_CLEANUP)
		write_sde_csr(sde, WFR_SEND_DMA_CTRL,
			sde->p_senddmactrl |
			WFR_SEND_DMA_CTRL_SDMA_CLEANUP_SMASK);
	else
		write_sde_csr(sde, WFR_SEND_DMA_CTRL, sde->p_senddmactrl);
	/* JAG XXX: do we have kr_scratch need/equivalent */

	/* JAG TODO: OP_DRAIN */

	spin_unlock(&sde->senddmactrl_lock);

	/* JAG TODO: OP_DRAIN */
#ifdef JAG_SDMA_VERBOSITY
	sdma_dumpstate(sde);
#endif
}

static void sdma_hw_clean_up(struct sdma_engine *sde)
{

#ifdef JAG_SDMA_VERBOSITY
	dd_dev_err(sde->dd, "JAG SDMA(%u) %s:%d %s()\n",
		sde->this_idx, slashstrip(__FILE__), __LINE__, __func__);
#endif

	if (print_unimplemented)
		dd_dev_info(sde->dd, "%s: not implemented\n", __func__);

#ifdef JAG_SDMA_VERBOSITY
	sdma_dumpstate(sde);
#endif
}

static void sdma_setlengen(struct sdma_engine *sde)
{
#ifdef JAG_SDMA_VERBOSITY
	dd_dev_err(sde->dd, "JAG SDMA(%u) %s:%d %s()\n",
		sde->this_idx, slashstrip(__FILE__), __LINE__, __func__);
#endif

	/*
	 * Set SendDmaLenGen and clear-then-set the MSB of the generation
	 * count to enable generation checking and load the internal
	 * generation counter.
	 */
	write_sde_csr(sde, WFR_SEND_DMA_LEN_GEN,
		(sde->descq_cnt/64) << WFR_SEND_DMA_LEN_GEN_LENGTH_SHIFT
	);
	write_sde_csr(sde, WFR_SEND_DMA_LEN_GEN,
		((sde->descq_cnt/64) << WFR_SEND_DMA_LEN_GEN_LENGTH_SHIFT)
		| (4ULL << WFR_SEND_DMA_LEN_GEN_GENERATION_SHIFT)
	);
}

void sdma_update_tail(struct sdma_engine *sde, u16 tail)
{
#ifdef JAG_SDMA_VERBOSITY
	dd_dev_err(sde->dd, "JAG SDMA(%u) %s:%d %s()\n", sde->this_idx,
		slashstrip(__FILE__), __LINE__, __func__);
	dd_dev_err(sde->dd, "tail: 0x%x --> 0x%x\n", sde->descq_tail, tail);

	do {
		u16 i, start, end;
		start = sde->descq_tail;
		end = tail;
		if (start < end) {
			for (i = start; i < end; ++i) {
				dd_dev_err(sde->dd, "desc[%d] = %016llx %016llx\n",
					i,
					le64_to_cpu(sde->descq[i].qw[0]),
					le64_to_cpu(sde->descq[i].qw[1])
				);
			}
		} else if (end < start) {
			for (i = start; i < sde->descq_cnt; ++i) {
				dd_dev_err(sde->dd, "desc[%d] = %016llx %016llx\n",
					i,
					le64_to_cpu(sde->descq[i].qw[0]),
					le64_to_cpu(sde->descq[i].qw[1])
				);
			}
			for (i = 0; i < end; ++i) {
				dd_dev_err(sde->dd, "desc[%d] = %016llx %016llx\n",
					i,
					le64_to_cpu(sde->descq[i].qw[0]),
					le64_to_cpu(sde->descq[i].qw[1])
				);
			}
		} else {
			dd_dev_err(sde->dd, "Empty!???\n");
		}
	} while (0);
#endif

	sde->descq_tail = tail;
	/* Commit writes to memory and advance the tail on the chip */
	smp_wmb();
	write_sde_csr(sde, WFR_SEND_DMA_TAIL, tail);
}

/*
 * This is called when changing to state s10_hw_start_up_halt_wait as
 * a result of send buffer errors or send DMA descriptor errors.
 */
static void sdma_hw_start_up(struct sdma_engine *sde)
{
#ifdef JAG_SDMA_VERBOSITY
	dd_dev_err(sde->dd, "JAG SDMA(%u) %s:%d %s()\n",
		sde->this_idx, slashstrip(__FILE__), __LINE__, __func__);
#endif

	sdma_setlengen(sde);
	sdma_update_tail(sde, 0); /* Set SendDmaTail */
	*sde->head_dma = 0;
}

static void sdma_set_desc_cnt(struct sdma_engine *sde, unsigned cnt)
{
	u64 reg = cnt;

#ifdef JAG_SDMA_VERBOSITY
	dd_dev_err(sde->dd, "JAG SDMA %s:%d %s()\n",
		slashstrip(__FILE__), __LINE__, __func__);
#endif

	reg &= WFR_SEND_DMA_DESC_CNT_CNT_MASK;
	reg <<= WFR_SEND_DMA_RELOAD_CNT_CNT_SHIFT;
	write_sde_csr(sde, WFR_SEND_DMA_DESC_CNT, reg);
}

static void init_sdma_regs(struct sdma_engine *sde, u32 credits)
{
#ifdef JAG_SDMA_VERBOSITY
	dd_dev_err(sde->dd, "JAG SDMA(%u) %s:%d %s()\n",
		sde->this_idx, slashstrip(__FILE__), __LINE__, __func__);
#endif

	write_sde_csr(sde, WFR_SEND_DMA_BASE_ADDR, sde->descq_phys);
	sdma_setlengen(sde);
	sdma_update_tail(sde, 0); /* Set SendDmaTail */
	write_sde_csr(sde, WFR_SEND_DMA_RELOAD_CNT, sdma_idle_cnt);
	write_sde_csr(sde, WFR_SEND_DMA_DESC_CNT, 0);
	write_sde_csr(sde, WFR_SEND_DMA_HEAD_ADDR, sde->head_phys);
	write_sde_csr(sde, WFR_SEND_DMA_MEMORY,
		((u64)credits <<
			WFR_SEND_DMA_MEMORY_SDMA_MEMORY_CNT_SHIFT) |
		((u64)(credits * sde->this_idx) <<
			WFR_SEND_DMA_MEMORY_SDMA_MEMORY_INDEX_SHIFT));
	write_sde_csr(sde, WFR_SEND_DMA_ENG_ERR_MASK, ~0ull);
	write_sde_csr(sde, WFR_SEND_DMA_CHECK_ENABLE,
			HFI_PKT_BASE_SDMA_INTEGRITY);
}

#ifdef JAG_SDMA_VERBOSITY

#define sdma_dumpstate_helper0(reg) do { \
		csr = read_csr(sde->dd, reg); \
		dd_dev_err(sde->dd, "%36s     0x%016llx\n", #reg, csr); \
	} while (0)

#define sdma_dumpstate_helper(reg) do { \
		csr = read_sde_csr(sde, reg); \
		dd_dev_err(sde->dd, "%36s[%02u] 0x%016llx\n", \
			#reg, sde->this_idx, csr); \
	} while (0)

#define sdma_dumpstate_helper2(reg) do { \
		csr = read_csr(sde->dd, reg + (8 * i)); \
		dd_dev_err(sde->dd, "%33s_%02u     0x%016llx\n", \
				#reg, i, csr); \
	} while (0)

void sdma_dumpstate(struct sdma_engine *sde)
{
	u64 csr;
	unsigned i;

	sdma_dumpstate_helper(WFR_SEND_DMA_CTRL);
	sdma_dumpstate_helper(WFR_SEND_DMA_STATUS);
	sdma_dumpstate_helper0(WFR_SEND_DMA_ERR_STATUS);
	sdma_dumpstate_helper0(WFR_SEND_DMA_ERR_MASK);
	sdma_dumpstate_helper(WFR_SEND_DMA_ENG_ERR_STATUS);
	sdma_dumpstate_helper(WFR_SEND_DMA_ENG_ERR_MASK);

	for (i = 0; i < WFR_CCE_NUM_INT_CSRS; ++i) {
		sdma_dumpstate_helper2(WFR_CCE_INT_STATUS);
		sdma_dumpstate_helper2(WFR_CCE_INT_MASK);
		sdma_dumpstate_helper2(WFR_CCE_INT_BLOCKED);
	}

	sdma_dumpstate_helper(WFR_SEND_DMA_TAIL);
	sdma_dumpstate_helper(WFR_SEND_DMA_HEAD);
	sdma_dumpstate_helper(WFR_SEND_DMA_PRIORITY_THLD);
	sdma_dumpstate_helper(WFR_SEND_DMA_IDLE_CNT);
	sdma_dumpstate_helper(WFR_SEND_DMA_RELOAD_CNT);
	sdma_dumpstate_helper(WFR_SEND_DMA_DESC_CNT);
	sdma_dumpstate_helper(WFR_SEND_DMA_DESC_FETCHED_CNT);
	sdma_dumpstate_helper(WFR_SEND_DMA_MEMORY);
	sdma_dumpstate_helper0(WFR_SEND_DMA_ENGINES);
	sdma_dumpstate_helper0(WFR_SEND_DMA_MEM_SIZE);
	/* sdma_dumpstate_helper(WFR_SEND_EGRESS_SEND_DMA_STATUS);  */
	sdma_dumpstate_helper(WFR_SEND_DMA_BASE_ADDR);
	sdma_dumpstate_helper(WFR_SEND_DMA_LEN_GEN);
	sdma_dumpstate_helper(WFR_SEND_DMA_HEAD_ADDR);
	sdma_dumpstate_helper(WFR_SEND_DMA_CHECK_ENABLE);
	sdma_dumpstate_helper(WFR_SEND_DMA_CHECK_VL);
	sdma_dumpstate_helper(WFR_SEND_DMA_CHECK_JOB_KEY);
	sdma_dumpstate_helper(WFR_SEND_DMA_CHECK_PARTITION_KEY);
	sdma_dumpstate_helper(WFR_SEND_DMA_CHECK_SLID);
	sdma_dumpstate_helper(WFR_SEND_DMA_CHECK_OPCODE);
}
#endif

/*
 * sdma_lock should be acquired before calling this routine
 */
static void dump_sdma_state(struct sdma_engine *sde)
{
	struct hw_sdma_desc *descq;
	struct sdma_txreq *txp, *txpnext;
	struct hw_sdma_desc *descqp;
	u64 desc[2];
	u64 addr;
	u8 gen;
	u16 len;
	u16 head, tail, cnt;

	head = sde->descq_head;
	tail = sde->descq_tail;
	cnt = sdma_descq_freecnt(sde);
	descq = sde->descq;

	dd_dev_err(sde->dd, 
		"SDMA (%u) descq_head: %u\n",
		sde->this_idx,
		head);
	dd_dev_err(sde->dd, 
		"SDMA (%u) descq_tail: %u\n",
		sde->this_idx,
		tail);
	dd_dev_err(sde->dd, 
		"SDMA (%u) freecnt: %u\n",
		sde->this_idx,
		cnt);

	/* print info for each entry in the descriptor queue */
	while (head != tail) {
		char flags[6] = { 'x', 'x', 'x', 'x', 0 };

		descqp = &sde->descq[head];
		desc[0] = le64_to_cpu(descqp->qw[0]);
		desc[1] = le64_to_cpu(descqp->qw[1]);
		flags[0] = (desc[1] & SDMA_DESC1_INT_REQ_FLAG) ? 'I' : '-';
		flags[1] = (desc[1] & SDMA_DESC1_HEAD_TO_HOST_FLAG) ?
				'H' : '-';
		flags[2] = (desc[0] & SDMA_DESC0_FIRST_DESC_FLAG) ? 'F' : '-';
		flags[3] = (desc[0] & SDMA_DESC0_LAST_DESC_FLAG) ? 'L' : '-';
		addr = (desc[0] >> SDMA_DESC0_PHY_ADDR_SHIFT)
			& SDMA_DESC0_PHY_ADDR_MASK;
		gen = (desc[1] >> SDMA_DESC1_GENERATION_SHIFT)
			& SDMA_DESC1_GENERATION_MASK;
		len = (desc[0] >> SDMA_DESC0_BYTE_COUNT_SHIFT)
			& SDMA_DESC0_BYTE_COUNT_MASK;
		dd_dev_err(sde->dd,
			"SDMA sdmadesc[%u]: flags:%s addr:0x%016llx gen:%u len:%u bytes\n",
			 head, flags, addr, gen, len);
		if (++head == sde->descq_cnt)
			head = 0;
	}

	/* print dma descriptor indices from the TX requests */
	list_for_each_entry_safe(txp, txpnext, &sde->activelist, list) {
		dd_dev_err(sde->dd,
			"SDMA txp->start_idx: %u txp->next_descq_idx: %u\n",
			txp->start_idx, txp->next_descq_idx);
	}
}

#define SDE_FMT \
	"SDE %u STE %s C 0x%llx S 0x%016llx E 0x%llx T(HW) 0x%llx T(SW) 0x%x H(HW) 0x%llx H(SW) 0x%x H(D) 0x%llx DM 0x%llx\n"
/**
 * sdma_seqfile_dump_sde() - debugfs dump of sde
 * @s: seq file
 * @sde: send dma engine to dump
 *
 * This routine dumps the sde to the indicated seq file.
 */
void sdma_seqfile_dump_sde(struct seq_file *s, struct sdma_engine *sde)
{
	u16 head, tail;
	struct hw_sdma_desc *descqp;
	u64 desc[2];
	u64 addr;
	u8 gen;
	u16 len;

	head = sde->descq_head;
	tail = sde->descq_tail;
	seq_printf(s, SDE_FMT, sde->this_idx,
		sdma_state_name(sde->state.current_state),
		(unsigned long long)read_sde_csr(sde, WFR_SEND_DMA_CTRL),
		(unsigned long long)read_sde_csr(sde, WFR_SEND_DMA_STATUS),
		(unsigned long long)read_sde_csr(sde,
			WFR_SEND_DMA_ENG_ERR_STATUS),
		(unsigned long long)read_sde_csr(sde, WFR_SEND_DMA_TAIL),
		tail,
		(unsigned long long)read_sde_csr(sde, WFR_SEND_DMA_HEAD),
		head,
		(unsigned long long)*sde->head_dma,
		(unsigned long long)read_sde_csr(sde, WFR_SEND_DMA_MEMORY));

	/* print info for each entry in the descriptor queue */
	while (head != tail) {
		char flags[6] = { 'x', 'x', 'x', 'x', 0 };

		descqp = &sde->descq[head];
		desc[0] = le64_to_cpu(descqp->qw[0]);
		desc[1] = le64_to_cpu(descqp->qw[1]);
		flags[0] = (desc[1] & SDMA_DESC1_INT_REQ_FLAG) ? 'I' : '-';
		flags[1] = (desc[1] & SDMA_DESC1_HEAD_TO_HOST_FLAG) ?
				'H' : '-';
		flags[2] = (desc[0] & SDMA_DESC0_FIRST_DESC_FLAG) ? 'F' : '-';
		flags[3] = (desc[0] & SDMA_DESC0_LAST_DESC_FLAG) ? 'L' : '-';
		addr = (desc[0] >> SDMA_DESC0_PHY_ADDR_SHIFT)
			& SDMA_DESC0_PHY_ADDR_MASK;
		gen = (desc[1] >> SDMA_DESC1_GENERATION_SHIFT)
			& SDMA_DESC1_GENERATION_MASK;
		len = (desc[0] >> SDMA_DESC0_BYTE_COUNT_SHIFT)
			& SDMA_DESC0_BYTE_COUNT_MASK;
		seq_printf(s,
			"\tdesc[%u]: flags:%s addr:0x%016llx gen:%u len:%u bytes\n",
			head, flags, addr, gen, len);
		if (++head == sde->descq_cnt)
			head = 0;
	}
}

/*
 * Complete a request when sdma not running; likely only request
 * but to simplify the code, always queue it, then process the full
 * activelist.  We process the entire list to ensure that this particular
 * request does get it's callback, but in the correct order.
 * Must be called with sdma_lock held
 */
/* FIXME - old routine */
static void complete_sdma_err_req(struct sdma_engine *sde,
				  struct qib_verbs_txreq *tx)
{
	atomic_inc(&tx->qp->s_iowait.sdma_busy);
	/* no sdma descriptors, so no unmap_desc */
	tx->txreq.start_idx = 0;
	tx->txreq.next_descq_idx = 0;
	list_add_tail(&tx->txreq.list, &sde->activelist);
	clear_sdma_activelist(sde);
}

/*
 * This function queues one IB packet onto the send DMA queue per call.
 * The caller is responsible for checking:
 * 1) The number of send DMA descriptor entries is less than the size of
 *    the descriptor queue.
 * 2) The IB SGE addresses and lengths are 32-bit aligned
 *    (except possibly the last SGE's length)
 * 3) The SGE addresses are suitable for passing to dma_map_single().
 */
/* FIXME - old routine, move to verbs
 * and re-implement in terms of new API
 */
int qib_sdma_verbs_send(struct sdma_engine *sde,
			struct qib_sge_state *ss, u32 dwords,
			struct qib_verbs_txreq *tx)
{
	unsigned long flags;
	struct qib_sge *sge;
	struct qib_qp *qp;
	int ret = 0;
	u16 tail;
	struct hw_sdma_desc *descqp;
	u64 sdmadesc[2];
	dma_addr_t addr;

	spin_lock_irqsave(&sde->lock, flags);

retry:
	if (unlikely(!__sdma_running(sde))) {
		complete_sdma_err_req(sde, tx);
		goto unlock;
	}
#ifdef JAG_SDMA_VERBOSITY
	dd_dev_err(sde->dd, "JAG SDMA(%u) %s:%d %s() running\n",
		sde->this_idx, slashstrip(__FILE__), __LINE__, __func__);
#endif

	if (tx->txreq.num_desc > sdma_descq_freecnt(sde)) {
		if (qib_sdma_make_progress(sde))
			goto retry;
		if (sde->dd->flags & QIB_HAS_SDMA_TIMEOUT)
			sdma_set_desc_cnt(sde, sde->descq_cnt / 2);
		goto busy;
	}

	make_sdma_desc(sde, sdmadesc, (u64) tx->addr, tx->hdr_dwords);

	sdmadesc[0] |= SDMA_DESC0_FIRST_DESC_FLAG;

	/* write to the descq */
	tail = sde->descq_tail;
	descqp = &sde->descq[tail];
	trace_hfi_sdma_descriptor(sde, sdmadesc[0], sdmadesc[1], tail, descqp);
#ifdef JAG_SDMA_VERBOSITY
	dd_dev_err(sde->dd, "JAG SDMA(%u) writing desc %016llx %016llx to %016llx\n",
		sde->this_idx,
		sdmadesc[0], sdmadesc[1], (unsigned long long)descqp);
#endif
	descqp->qw[0] = cpu_to_le64(sdmadesc[0]);
	descqp->qw[1] = cpu_to_le64(sdmadesc[1]);
	descqp++;

	/* increment the tail */
	if (++tail == sde->descq_cnt) {
		tail = 0;
		descqp = &sde->descq[0];
		++sde->generation;
	}

	tx->txreq.start_idx = tail;

	sge = &ss->sge;
	while (dwords) {
		u32 dw;
		u32 len;

		len = dwords << 2;
		if (len > sge->length)
			len = sge->length;
		if (len > sge->sge_length)
			len = sge->sge_length;
		BUG_ON(len == 0);
		dw = (len + 3) >> 2;
		addr = dma_map_single(&sde->dd->pcidev->dev, sge->vaddr,
				      dw << 2, DMA_TO_DEVICE);
		if (dma_mapping_error(&sde->dd->pcidev->dev, addr))
			goto unmap;
		make_sdma_desc(sde, sdmadesc, (u64) addr, dw);
		/* write to the descq */
		trace_hfi_sdma_descriptor(sde, sdmadesc[0], sdmadesc[1],
			tail, descqp);
#ifdef JAG_SDMA_VERBOSITY
		dd_dev_err(sde->dd, "JAG SDMA(%u) writing desc %016llx %016llx to %016llx\n",
			sde->this_idx,
			sdmadesc[0], sdmadesc[1], (unsigned long long)descqp);
#endif
		descqp->qw[0] = cpu_to_le64(sdmadesc[0]);
		descqp->qw[1] = cpu_to_le64(sdmadesc[1]);
		descqp++;

		/* increment the tail */
		if (++tail == sde->descq_cnt) {
			tail = 0;
			descqp = &sde->descq[0];
			++sde->generation;
		}
		sge->vaddr += len;
		sge->length -= len;
		sge->sge_length -= len;
		if (sge->sge_length == 0) {
			if (--ss->num_sge)
				*sge = *ss->sg_list++;
		} else if (sge->length == 0 && sge->mr->lkey) {
			if (++sge->n >= QIB_SEGSZ) {
				if (++sge->m >= sge->mr->mapsz)
					break;
				sge->n = 0;
			}
			sge->vaddr =
				sge->mr->map[sge->m]->segs[sge->n].vaddr;
			sge->length =
				sge->mr->map[sge->m]->segs[sge->n].length;
		}

		dwords -= dw;
	}

	if (!tail)
		descqp = &sde->descq[sde->descq_cnt];
	descqp--;
	descqp->qw[0] |= cpu_to_le64(SDMA_DESC0_LAST_DESC_FLAG);
	descqp->qw[1] |= sde->default_desc1;

	/* JAG SDMA TODO determine if there's a problem if not applied */
	descqp->qw[1] |= cpu_to_le64(SDMA_DESC1_HEAD_TO_HOST_FLAG);
	descqp->qw[1] |= cpu_to_le64(SDMA_DESC1_INT_REQ_FLAG);

	atomic_inc(&tx->qp->s_iowait.sdma_busy);
	tx->txreq.next_descq_idx = tail;
	sdma_update_tail(sde, tail);
	sde->descq_added += tx->txreq.num_desc;
	list_add_tail(&tx->txreq.list, &sde->activelist);
	goto unlock;

unmap:
	for (;;) {
		if (!tail)
			tail = sde->descq_cnt - 1;
		else
			tail--;
		if (tail == sde->descq_tail)
			break;
		unmap_desc(sde, tail);
	}
	qp = tx->qp;
	qib_put_txreq(tx);
	spin_lock(&qp->r_lock);
	spin_lock(&qp->s_lock);
	if (qp->ibqp.qp_type == IB_QPT_RC) {
		/* XXX what about error sending RDMA read responses? */
		if (ib_qib_state_ops[qp->state] & QIB_PROCESS_RECV_OK)
			qib_error_qp(qp, IB_WC_GENERAL_ERR);
	} else if (qp->s_wqe)
		qib_send_complete(qp, qp->s_wqe, IB_WC_GENERAL_ERR);
	spin_unlock(&qp->s_lock);
	spin_unlock(&qp->r_lock);
	/* return zero to process the next send work request */
	goto unlock;

busy:
	qp = tx->qp;
	spin_lock(&qp->s_lock);
	if (ib_qib_state_ops[qp->state] & QIB_PROCESS_RECV_OK) {
		struct qib_ibdev *dev;

		/*
		 * If we couldn't queue the DMA request, save the info
		 * and try again later rather than destroying the
		 * buffer and undoing the side effects of the copy.
		 */
		tx->ss = ss;
		tx->dwords = dwords;
		list_add_tail(&tx->txreq.list, &qp->s_iowait.tx_head);
		dev = &sde->dd->verbs_dev;
		spin_lock(&dev->pending_lock);
		if (list_empty(&qp->s_iowait.list)) {
			struct qib_ibport *ibp;

			ibp = &sde->ppd->ibport_data;
			ibp->n_dmawait++;
			qp->s_flags |= QIB_S_WAIT_DMA_DESC;
			list_add_tail(&qp->s_iowait.list, &sde->dmawait);
			trace_hfi_qpsleep(qp, QIB_S_WAIT_DMA_DESC);
			atomic_inc(&qp->refcount);
		}
		spin_unlock(&dev->pending_lock);
		qp->s_flags &= ~QIB_S_BUSY;
		spin_unlock(&qp->s_lock);
		ret = -EBUSY;
	} else {
		spin_unlock(&qp->s_lock);
		qib_put_txreq(tx);
	}
unlock:
	spin_unlock_irqrestore(&sde->lock, flags);
	return ret;
}

/**
 * sdma_send_txreq() - submit a tx req to ring
 * @sde: sdma engine to use
 * @wait: wait structure to use when full (may be NULL)
 * @busycb: callback when send hits full ring (may be NULL)
 * @tx: sdma_txreq to submit
 *
 * The call submits the tx into the ring.  If a iowait struture is non-NULL
 * the packet will be queued to the list in wait.
 *
 * If the busycb is non-NULL, the routine will be called providing the caller
 * via the  callback with the capability of "waiting" via a queuing mechanism.
 *
 * Return:
 * 0 - Success, -EBUSY - no space in ring (wait == NULL)
 * -EIOCBQUEUED - tx queued to iowait, -ECOMM bad sdma state
 */
int sdma_send_txreq(struct sdma_engine *sde,
		    struct iowait *wait,
		    void (*busycb)(
			struct sdma_txreq *tx,
			struct iowait *wait),
		    struct sdma_txreq *tx)
{
	int ret = 0;
	int i;
	u16 tail;
	unsigned long flags;
	struct sdma_desc *descp = tx->descp;

	/* user should have supplied entire packet */
	BUG_ON(tx->tlen);
	spin_lock_irqsave(&sde->lock, flags);
retry:
	tail = sde->descq_tail;
	tx->start_idx = tail;
	if (unlikely(!__sdma_running(sde)))
		goto unlock_noconn;
	if (unlikely(tx->num_desc > sdma_descq_freecnt(sde)))
		goto nodesc;
	for (i = 0; i < tx->num_desc; i++, descp++) {
		sde->descq[tail].qw[0] = descp->qw[0];
		/* replace generation with real one */
		descp->qw[1] &= ~(3ULL << SDMA_DESC1_GENERATION_SHIFT);
		descp->qw[1] |=
			(sde->generation & SDMA_DESC1_GENERATION_MASK)
			<< SDMA_DESC1_GENERATION_SHIFT;
		sde->descq[tail].qw[1] = descp->qw[1];
		trace_hfi_sdma_descriptor(sde, descp->qw[0], descp->qw[1],
			tail, &sde->descq[tail]);
		if (++tail == sde->descq_cnt) {
			++sde->generation;
			tail = 0;
		}
	}
	sde->descq_added += tx->num_desc;
	tx->next_descq_idx = tail;
	list_add_tail(&tx->list, &sde->activelist);
	sdma_update_tail(sde, tail);
	if (wait)
		atomic_inc(&wait->sdma_busy);
unlock:
	spin_unlock_irqrestore(&sde->lock, flags);
	return ret;
unlock_noconn:
	if (wait)
		atomic_inc(&wait->sdma_busy);
	tx->start_idx = tx->next_descq_idx = 0;
	list_add_tail(&tx->list, &sde->activelist);
	if (wait) {
		wait->tx_count++;
		wait->count += tx->num_desc;
	}
	clear_sdma_activelist(sde);
	ret = -ECOMM;
	goto unlock;
nodesc:
	if (sdma_make_progress(sde))
		goto retry;
	if (sde->dd->flags & QIB_HAS_SDMA_TIMEOUT)
		sdma_set_desc_cnt(sde, sde->descq_cnt / 2);
	if (busycb && wait)
		busycb(tx, wait);
	ret = -EBUSY;
	goto unlock;
}

/**
 * sdma_send_txlist() - submit a list of tx req to ring
 * @sde: sdma engine to use
 * @wait: wait structure to use when full (may be NULL)
 * @busycb: callback when send hits full ring (may be NULL)
 * @tx_list: list of sdma_txreqs to submit
 *
 * The call submits the list into the ring.
 *
 * If the iowait struture is non-NULL and not equal to the iowait list
 * the unprocessed part of the list  will be appended to the list in wait.
 *
 * In all cases, the tx_list will be updated so the head of the tx_list is
 * the list of descriptors that have yet to be transmitted.
 *
 * If the busycb is non-NULL, the routine will be called providing the caller
 * via the  callback with the capability of "waiting" via a queuing mechanism.
 *
 * The intent of this call is to provide a more efficient
 * way of submitting multiple packets to SDMA while holding the tail
 * side locking.
 *
 * Return:
 * 0 - Success, -EBUSY - no space in ring (wait == NULL)
 * -EIOCBQUEUED - tx queued to iowait, -ECOMM bad sdma state
 */
int sdma_send_txlist(struct sdma_engine *sde,
		    struct iowait *wait,
		    void (*busycb)(
			struct sdma_txreq *tx,
			struct iowait *wait),
		    struct list_head *tx_list)
{
	BUG_ON(1);
}

static void sdma_process_event(struct sdma_engine *sde,
	enum sdma_events event)
{
	unsigned long flags;

	spin_lock_irqsave(&sde->lock, flags);

	__sdma_process_event(sde, event);

	if (sde->state.current_state == sdma_state_s99_running)
		sdma_desc_avail(sde, sdma_descq_freecnt(sde));

	spin_unlock_irqrestore(&sde->lock, flags);
}

static void __sdma_process_event(struct sdma_engine *sde,
	enum sdma_events event)
{
	struct sdma_state *ss = &sde->state;

	/* JAG SDMA temporary */
#ifdef JAG_SDMA_VERBOSITY
	dd_dev_err(sde->dd, "JAG SDMA(%u) %s:%d %s()\n", sde->this_idx,
		slashstrip(__FILE__), __LINE__, __func__);
	dd_dev_err(sde->dd, "IB%u:%u [%s] %s\n", sde->dd->unit,
		sde->dd->pport->port,
		sdma_state_names[ss->current_state],
	sdma_event_names[event]);
#endif

	switch (ss->current_state) {
	case sdma_state_s00_hw_down:
		switch (event) {
		case sdma_event_e00_go_hw_down:
			break;
		case sdma_event_e30_go_running:
			/*
			 * If down, but running requested (usually result
			 * of link up, then we need to start up.
			 * This can happen when hw down is requested while
			 * bringing the link up with traffic active on
			 * 7220, e.g. */
			ss->go_s99_running = 1;
			/* fall through and start dma engine */
		case sdma_event_e10_go_hw_start:
			/* This reference means the state machine is started */
			sdma_get(&sde->state);
			sdma_set_state(sde,
				sdma_state_s10_hw_start_up_halt_wait);
			break;
		case sdma_event_e15_hw_started1:
			break;
		case sdma_event_e25_hw_started2:
			break;
		case sdma_event_e40_sw_cleaned:
			sdma_sw_tear_down(sde);
			break;
		case sdma_event_e50_hw_cleaned:
			break;
		case sdma_event_e60_hw_halted:
			break;
		case sdma_event_e70_go_idle:
			break;
		case sdma_event_e7220_err_halted:
			break;
		case sdma_event_e7322_err_halted:
			break;
		case sdma_event_e90_timer_tick:
			break;
		}
		break;

	case sdma_state_s10_hw_start_up_halt_wait:
		switch (event) {
		case sdma_event_e00_go_hw_down:
			sdma_set_state(sde, sdma_state_s00_hw_down);
			sdma_sw_tear_down(sde);
			break;
		case sdma_event_e10_go_hw_start:
			break;
		case sdma_event_e15_hw_started1:
			sdma_set_state(sde,
				sdma_state_s15_hw_start_up_clean_wait);
			sdma_start_hw_clean_up(sde);
			break;
		case sdma_event_e25_hw_started2:
			break;
		case sdma_event_e30_go_running:
			ss->go_s99_running = 1;
			break;
		case sdma_event_e40_sw_cleaned:
			break;
		case sdma_event_e50_hw_cleaned:
			break;
		case sdma_event_e60_hw_halted:
			break;
		case sdma_event_e70_go_idle:
			ss->go_s99_running = 0;
			break;
		case sdma_event_e7220_err_halted:
			break;
		case sdma_event_e7322_err_halted:
			break;
		case sdma_event_e90_timer_tick:
			break;
		}
		break;

	case sdma_state_s15_hw_start_up_clean_wait:
		switch (event) {
		case sdma_event_e00_go_hw_down:
			sdma_set_state(sde, sdma_state_s00_hw_down);
			sdma_sw_tear_down(sde);
			break;
		case sdma_event_e10_go_hw_start:
			break;
		case sdma_event_e15_hw_started1:
			break;
		case sdma_event_e25_hw_started2:
			sdma_hw_start_up(sde);
			sdma_set_state(sde, ss->go_s99_running ?
				       sdma_state_s99_running :
				       sdma_state_s20_idle);
			break;
		case sdma_event_e30_go_running:
			ss->go_s99_running = 1;
			break;
		case sdma_event_e40_sw_cleaned:
			break;
		case sdma_event_e50_hw_cleaned:
			break;
		case sdma_event_e60_hw_halted:
			break;
		case sdma_event_e70_go_idle:
			ss->go_s99_running = 0;
			break;
		case sdma_event_e7220_err_halted:
			break;
		case sdma_event_e7322_err_halted:
			break;
		case sdma_event_e90_timer_tick:
			break;
		}
		break;

	case sdma_state_s20_idle:
		switch (event) {
		case sdma_event_e00_go_hw_down:
			sdma_set_state(sde, sdma_state_s00_hw_down);
			sdma_sw_tear_down(sde);
			break;
		case sdma_event_e10_go_hw_start:
			break;
		case sdma_event_e15_hw_started1:
			break;
		case sdma_event_e25_hw_started2:
			break;
		case sdma_event_e30_go_running:
			sdma_set_state(sde, sdma_state_s99_running);
			ss->go_s99_running = 1;
			break;
		case sdma_event_e40_sw_cleaned:
			break;
		case sdma_event_e50_hw_cleaned:
			break;
		case sdma_event_e60_hw_halted:
			break;
		case sdma_event_e70_go_idle:
			break;
		case sdma_event_e7220_err_halted:
			break;
		case sdma_event_e7322_err_halted:
			break;
		case sdma_event_e90_timer_tick:
			break;
		}
		break;

	case sdma_state_s30_sw_clean_up_wait:
		switch (event) {
		case sdma_event_e00_go_hw_down:
			sdma_set_state(sde, sdma_state_s00_hw_down);
			break;
		case sdma_event_e10_go_hw_start:
			break;
		case sdma_event_e15_hw_started1:
			break;
		case sdma_event_e25_hw_started2:
			break;
		case sdma_event_e30_go_running:
			ss->go_s99_running = 1;
			break;
		case sdma_event_e40_sw_cleaned:
			sdma_set_state(sde,
				sdma_state_s10_hw_start_up_halt_wait);
			sdma_hw_start_up(sde);
			break;
		case sdma_event_e50_hw_cleaned:
			break;
		case sdma_event_e60_hw_halted:
			break;
		case sdma_event_e70_go_idle:
			ss->go_s99_running = 0;
			break;
		case sdma_event_e7220_err_halted:
			break;
		case sdma_event_e7322_err_halted:
			break;
		case sdma_event_e90_timer_tick:
			break;
		}
		break;

	case sdma_state_s40_hw_clean_up_wait:
		switch (event) {
		case sdma_event_e00_go_hw_down:
			sdma_set_state(sde, sdma_state_s00_hw_down);
			sdma_start_sw_clean_up(sde);
			break;
		case sdma_event_e10_go_hw_start:
			break;
		case sdma_event_e15_hw_started1:
			break;
		case sdma_event_e25_hw_started2:
			break;
		case sdma_event_e30_go_running:
			ss->go_s99_running = 1;
			break;
		case sdma_event_e40_sw_cleaned:
			break;
		case sdma_event_e50_hw_cleaned:
			sdma_set_state(sde,
				       sdma_state_s30_sw_clean_up_wait);
			sdma_start_sw_clean_up(sde);
			break;
		case sdma_event_e60_hw_halted:
			break;
		case sdma_event_e70_go_idle:
			ss->go_s99_running = 0;
			break;
		case sdma_event_e7220_err_halted:
			break;
		case sdma_event_e7322_err_halted:
			break;
		case sdma_event_e90_timer_tick:
			break;
		}
		break;

	case sdma_state_s50_hw_halt_wait:
		switch (event) {
		case sdma_event_e00_go_hw_down:
			sdma_set_state(sde, sdma_state_s00_hw_down);
			sdma_start_sw_clean_up(sde);
			break;
		case sdma_event_e10_go_hw_start:
			break;
		case sdma_event_e15_hw_started1:
			break;
		case sdma_event_e25_hw_started2:
			break;
		case sdma_event_e30_go_running:
			ss->go_s99_running = 1;
			break;
		case sdma_event_e40_sw_cleaned:
			break;
		case sdma_event_e50_hw_cleaned:
			break;
		case sdma_event_e60_hw_halted:
			sdma_set_state(sde,
				       sdma_state_s40_hw_clean_up_wait);
			sdma_hw_clean_up(sde);
			break;
		case sdma_event_e70_go_idle:
			ss->go_s99_running = 0;
			break;
		case sdma_event_e7220_err_halted:
			break;
		case sdma_event_e7322_err_halted:
			break;
		case sdma_event_e90_timer_tick:
			break;
		}
		break;

	case sdma_state_s99_running:
		switch (event) {
		case sdma_event_e00_go_hw_down:
			sdma_set_state(sde, sdma_state_s00_hw_down);
			sdma_start_sw_clean_up(sde);
			break;
		case sdma_event_e10_go_hw_start:
			break;
		case sdma_event_e15_hw_started1:
			break;
		case sdma_event_e25_hw_started2:
			break;
		case sdma_event_e30_go_running:
			break;
		case sdma_event_e40_sw_cleaned:
			break;
		case sdma_event_e50_hw_cleaned:
			break;
		case sdma_event_e60_hw_halted:
			sdma_set_state(sde,
				       sdma_state_s30_sw_clean_up_wait);
			sdma_start_sw_clean_up(sde);
			break;
		case sdma_event_e70_go_idle:
			sdma_set_state(sde, sdma_state_s50_hw_halt_wait);
			ss->go_s99_running = 0;
			break;
		case sdma_event_e7220_err_halted:
			sdma_set_state(sde,
				       sdma_state_s30_sw_clean_up_wait);
			sdma_start_sw_clean_up(sde);
			break;
		case sdma_event_e7322_err_halted:
			sdma_set_state(sde, sdma_state_s50_hw_halt_wait);
			break;
		case sdma_event_e90_timer_tick:
			break;
		}
		break;
	}

	ss->last_event = event;
}

/* helper to extend txreq */
int _extend_sdma_tx_descs(struct hfi_devdata *dd, struct sdma_txreq *tx)
{
	BUG_ON(1);
	return 0;
}
