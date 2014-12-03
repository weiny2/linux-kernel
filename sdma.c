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
#include <linux/bitops.h>

#include "hfi.h"
#include "common.h"
#include "qp.h"
#include "sdma.h"
#include "iowait.h"
#include "trace.h"

/* must be a power of 2 >= 64 <= 32768 */
#define SDMA_DESCQ_CNT 1024
#define INVALID_TAIL 0xffff
/* default pio off, sdma on */
uint sdma_descq_cnt = SDMA_DESCQ_CNT;
module_param(sdma_descq_cnt, uint, S_IRUGO);
MODULE_PARM_DESC(sdma_descq_cnt, "Number of SDMA descq entries");

static uint sdma_idle_cnt = 250;
module_param(sdma_idle_cnt, uint, S_IRUGO);
MODULE_PARM_DESC(sdma_idle_cnt, "sdma interrupt idle delay (ns,default 250)");

static uint use_dma_head;
module_param(use_dma_head, uint, S_IRUGO);
MODULE_PARM_DESC(use_dma_head, "Read CSR vs. DMA for hardware head");

uint mod_num_sdma;
module_param_named(num_sdma, mod_num_sdma, uint, S_IRUGO);
MODULE_PARM_DESC(num_sdma, "Set max number SDMA engines to use");

uint use_sdma_ahg;
module_param(use_sdma_ahg, uint, S_IRUGO);
MODULE_PARM_DESC(use_sdma_ahg, "Turn on/off use of AHG");

#define SDMA_WAIT_BATCH_SIZE 20
/* max wait time for a SDMA engine to indicate it has halted */
#define SDMA_ERR_HALT_TIMEOUT 10 /* ms */
/* all SDMA engine errors that cause a halt */
#define ALL_SDMA_ENG_HALT_ERRS \
	(WFR_SEND_DMA_ENG_ERR_STATUS_SDMA_WRONG_DW_ERR_SMASK \
	| WFR_SEND_DMA_ENG_ERR_STATUS_SDMA_GEN_MISMATCH_ERR_SMASK \
	| WFR_SEND_DMA_ENG_ERR_STATUS_SDMA_TOO_LONG_ERR_SMASK \
	| WFR_SEND_DMA_ENG_ERR_STATUS_SDMA_TAIL_OUT_OF_BOUNDS_ERR_SMASK \
	| WFR_SEND_DMA_ENG_ERR_STATUS_SDMA_FIRST_DESC_ERR_SMASK \
	| WFR_SEND_DMA_ENG_ERR_STATUS_SDMA_MEM_READ_ERR_SMASK \
	| WFR_SEND_DMA_ENG_ERR_STATUS_SDMA_HALT_ERR_SMASK \
	| WFR_SEND_DMA_ENG_ERR_STATUS_SDMA_LENGTH_MISMATCH_ERR_SMASK \
	| WFR_SEND_DMA_ENG_ERR_STATUS_SDMA_PACKET_DESC_OVERFLOW_ERR_SMASK \
	| WFR_SEND_DMA_ENG_ERR_STATUS_SDMA_HEADER_SELECT_ERR_SMASK \
	| WFR_SEND_DMA_ENG_ERR_STATUS_SDMA_HEADER_ADDRESS_ERR_SMASK \
	| WFR_SEND_DMA_ENG_ERR_STATUS_SDMA_HEADER_LENGTH_ERR_SMASK \
	| WFR_SEND_DMA_ENG_ERR_STATUS_SDMA_TIMEOUT_ERR_SMASK \
	| WFR_SEND_DMA_ENG_ERR_STATUS_SDMA_DESC_TABLE_UNC_ERR_SMASK \
	| WFR_SEND_DMA_ENG_ERR_STATUS_SDMA_ASSEMBLY_UNC_ERR_SMASK \
	| WFR_SEND_DMA_ENG_ERR_STATUS_SDMA_PACKET_TRACKING_UNC_ERR_SMASK \
	| WFR_SEND_DMA_ENG_ERR_STATUS_SDMA_HEADER_STORAGE_UNC_ERR_SMASK \
	| WFR_SEND_DMA_ENG_ERR_STATUS_SDMA_HEADER_REQUEST_FIFO_UNC_ERR_SMASK)

static const char * const sdma_state_names[] = {
	[sdma_state_s00_hw_down]                = "s00_HwDown",
	[sdma_state_s10_hw_start_up_halt_wait]  = "s10_HwStartUpHaltWait",
	[sdma_state_s15_hw_start_up_clean_wait] = "s15_HwStartUpCleanWait",
	[sdma_state_s20_idle]                   = "s20_Idle",
	[sdma_state_s30_sw_clean_up_wait]       = "s30_SwCleanUpWait",
	[sdma_state_s40_hw_clean_up_wait]       = "s40_HwCleanUpWait",
	[sdma_state_s50_hw_halt_wait]           = "s50_HwHaltWait",
	[sdma_state_s60_idle_halt_wait]         = "s60_IdleHaltWait",
	[sdma_state_s99_running]                = "s99_Running",
};

static const char * const sdma_event_names[] = {
	[sdma_event_e00_go_hw_down]   = "e00_GoHwDown",
	[sdma_event_e10_go_hw_start]  = "e10_GoHwStart",
	[sdma_event_e15_hw_halt_done] = "e15_HwHaltDone",
	[sdma_event_e25_hw_clean_up_done] = "e25_HwCleanUpDone",
	[sdma_event_e30_go_running]   = "e30_GoRunning",
	[sdma_event_e40_sw_cleaned]   = "e40_SwCleaned",
	[sdma_event_e50_hw_cleaned]   = "e50_HwCleaned",
	[sdma_event_e60_hw_halted]    = "e60_HwHalted",
	[sdma_event_e70_go_idle]      = "e70_GoIdle",
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
		.op_intenable = 0,
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
		.op_intenable = 0,
		.op_halt = 0,
		.op_cleanup = 0,
		.op_drain = 0,
	},
	[sdma_state_s40_hw_clean_up_wait] = {
		.op_enable = 0,
		.op_intenable = 0,
		.op_halt = 0,
		.op_cleanup = 1,
		.op_drain = 0,
	},
	[sdma_state_s50_hw_halt_wait] = {
		.op_enable = 0,
		.op_intenable = 0,
		.op_halt = 0,
		.op_cleanup = 0,
		.op_drain = 1,
	},
	[sdma_state_s60_idle_halt_wait] = {
		.go_s99_running_tofalse = 1,
		.op_enable = 0,
		.op_intenable = 0,
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
static void sdma_sendctrl(struct sdma_engine *, unsigned);
static void init_sdma_regs(struct sdma_engine *, u32, uint);
static void sdma_process_event(
	struct sdma_engine *sde,
	enum sdma_events event);
static void __sdma_process_event(
	struct sdma_engine *sde,
	enum sdma_events event);
static void dump_sdma_state(struct sdma_engine *sde);
static int sdma_make_progress(struct sdma_engine *sde, u64 status);

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
		int drained = 0;
		/* protect against complete modifying */
		struct iowait *wait = txp->wait;

		list_del_init(&txp->list);
		sdma_txclean(sde->dd, txp);
		if (wait)
			drained = atomic_dec_and_test(&wait->sdma_busy);
		if (txp->complete)
			(*txp->complete)(txp, SDMA_TXREQ_S_ABORTED, drained);
		if (drained)
			iowait_drain_wakeup(wait);
	}
}

void sdma_err_halt_wait(struct work_struct *work)
{
	struct sdma_engine *sde = container_of(work, struct sdma_engine,
						err_halt_worker);
	u64 statuscsr;
	unsigned long timeout;

	timeout = jiffies + msecs_to_jiffies(SDMA_ERR_HALT_TIMEOUT);
	while (1) {
		statuscsr = read_sde_csr(sde, WFR_SEND_DMA_STATUS);
		statuscsr &= WFR_SEND_DMA_STATUS_ENG_HALTED_SMASK;
		if (statuscsr)
			break;
		if (time_after(jiffies, timeout)) {
			dd_dev_err(sde->dd,
				"SDMA engine %d - timeout waiting for engine to halt\n",
				sde->this_idx);
			/*
			 * Continue anyway.  This could happen if there was
			 * an uncorrectable error in the wrong spot.
			 */
			break;
		}
		usleep_range(80, 120);
	}

	sdma_process_event(sde, sdma_event_e15_hw_halt_done);
}

void sdma_start_err_halt_wait(struct sdma_engine *sde)
{
	queue_work(sde->ppd->qib_wq, &sde->err_halt_worker);
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
		if (statuscsr)
			break;
		udelay(10);
	}

	sdma_process_event(sde, sdma_event_e25_hw_clean_up_done);
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
	sdma_make_progress(sde, 0);

	clear_sdma_activelist(sde);

	spin_unlock(&sde->dd->pport->sdma_alllock);

	/*
	 * Reset our notion of head and tail.
	 * Note that the HW registers have been reset via an earlier
	 * clean up.
	 */
	sde->descq_tail = 0;
	sde->descq_head = 0;
	sde->desc_avail = sdma_descq_freecnt(sde);
	*sde->head_dma = 0;

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
	uint idle_cnt = sdma_idle_cnt;

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

	if (dd->icode == WFR_ICODE_FUNCTIONAL_SIMULATOR && dd->irev < 46)
		idle_cnt = 0; /* work around a wfr-event bug */
	else
		idle_cnt = ns_to_cclock(dd, idle_cnt);
	if (dd->icode == WFR_ICODE_FUNCTIONAL_SIMULATOR && dd->irev < 47)
		use_sdma_ahg = 0;
	/* Allocate memory for SendDMA descriptor FIFOs */
	for (this_idx = 0; this_idx < num_engines; ++this_idx) {
		sde = &dd->per_sdma[this_idx];
		sde->dd = dd;
		sde->ppd = ppd;
		sde->this_idx = this_idx;
		sde->descq_cnt = descq_cnt;
		sde->desc_avail = sdma_descq_freecnt(sde);
		sde->sdma_shift = ilog2(descq_cnt);
		sde->sdma_mask = (1 << sde->sdma_shift) - 1;

		spin_lock_init(&sde->lock);
		spin_lock_init(&sde->senddmactrl_lock);
		/* insure there is always a zero bit */
		sde->ahg_bits = 0xfffffffe00000000;

		spin_lock_irqsave(&sde->lock, flags);
		sdma_set_state(sde, sdma_state_s00_hw_down);
		spin_unlock_irqrestore(&sde->lock, flags);

		/* set up reference counting */
		kref_init(&sde->state.kref);
		init_completion(&sde->state.comp);

		INIT_LIST_HEAD(&sde->activelist);
		INIT_LIST_HEAD(&sde->dmawait);

		sde->tail_csr =
			get_kctxt_csr_addr(dd, this_idx, WFR_SEND_DMA_TAIL);

		if (idle_cnt)
			dd->default_desc1 =
				cpu_to_le64(SDMA_DESC1_HEAD_TO_HOST_FLAG);
		else
			dd->default_desc1 =
				cpu_to_le64(SDMA_DESC1_INT_REQ_FLAG);

		tasklet_init(&sde->sdma_hw_clean_up_task, sdma_hw_clean_up_task,
			(unsigned long)sde);

		tasklet_init(&sde->sdma_sw_clean_up_task, sdma_sw_clean_up_task,
			(unsigned long)sde);
		INIT_WORK(&sde->err_halt_worker, sdma_err_halt_wait);

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
	dd->sdma_heads_dma = dma_zalloc_coherent(
		&dd->pcidev->dev,
		dd->sdma_heads_size,
		&dd->sdma_heads_phys,
		GFP_KERNEL
	);
	if (!dd->sdma_heads_dma) {
		dd_dev_err(dd, "failed to allocate SendDMA head memory\n");
		goto cleanup_descq;
	}

	/* Allocate memory for pad */
	dd->sdma_pad_dma = dma_zalloc_coherent(
		&dd->pcidev->dev,
		sizeof(u32),
		&dd->sdma_pad_phys,
		GFP_KERNEL
	);
	if (!dd->sdma_pad_dma) {
		dd_dev_err(dd, "failed to allocate SendDMA pad memory\n");
		goto pad_fail;
	}

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
		init_sdma_regs(sde, per_sdma_credits, idle_cnt);
	}
	dd->flags |= QIB_HAS_SEND_DMA;
	dd->flags |= idle_cnt ? QIB_HAS_SDMA_TIMEOUT : 0;
	dd->num_sdma = num_engines;
	sdma_map_init(dd, port, hfi_num_vls(ppd->vls_operational));
	return 0;

pad_fail:
	dma_free_coherent(&dd->pcidev->dev, dd->sdma_heads_size,
			  (void *)dd->sdma_heads_dma,
			  dd->sdma_heads_phys);
	dd->sdma_heads_dma = NULL;
	dd->sdma_heads_phys = 0;
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

/*
 * sdma_link_up() - called when the link goes up
 * @dd: hfi_devdata
 *
 * This routine moves all engines to the running state.
 */
void sdma_link_up(struct hfi_devdata *dd)
{
	struct sdma_engine *sde;
	unsigned int i;

	/* move all engines to running */
	for (i = 0; i < dd->num_sdma; ++i) {
		sde = &dd->per_sdma[i];
		sdma_process_event(sde, sdma_event_e30_go_running);
	}
}

/*
 * sdma_link_down() - called when the link goes down
 * @dd: hfi_devdata
 *
 * This routine moves all engines to the idle state.
 */
void sdma_link_down(struct hfi_devdata *dd)
{
	struct sdma_engine *sde;
	unsigned int i;

	/* idle all engines */
	for (i = 0; i < dd->num_sdma; ++i) {
		sde = &dd->per_sdma[i];
		sdma_process_event(sde, sdma_event_e70_go_idle);
	}
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
	if (dd->sdma_pad_dma) {
		dma_free_coherent(&dd->pcidev->dev, 4,
				  (void *)dd->sdma_pad_dma,
				  dd->sdma_pad_phys);
		dd->sdma_pad_dma = NULL;
		dd->sdma_pad_phys = 0;
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
 * This is used in the progress routine to clean the tx or
 * by the ULP to toss an inprocess tx build.
 *
 * The code can be called multiple times without issue.
 *
 */
void sdma_txclean(
	struct hfi_devdata *dd,
	struct sdma_txreq *tx)
{
	u16 i;

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
	tx->num_desc = 0;
	kfree(tx->coalesce_buf);
	tx->coalesce_buf = NULL;
	/* kmalloc'ed descp */
	if (unlikely(tx->desc_limit > ARRAY_SIZE(tx->descs))) {
		tx->desc_limit = ARRAY_SIZE(tx->descs);
		kfree(tx->descp);
	}
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

	use_dmahead = use_dma_head && __sdma_running(sde)
			&& (dd->flags & QIB_HAS_SDMA_TIMEOUT);
retry:
	hwhead = use_dmahead ?
		(u16) le64_to_cpu(*sde->head_dma) :
		(u16) read_sde_csr(sde, WFR_SEND_DMA_HEAD);

	swhead = sde->descq_head & sde->sdma_mask;
	/* this code is really bad for cache line trading */
	swtail = sde->descq_tail & sde->sdma_mask;
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
	unsigned i, n = 0;
	struct sdma_txreq *stx;
	struct qib_ibdev *dev = &sde->dd->verbs_dev;

#ifdef JAG_SDMA_VERBOSITY
	dd_dev_err(sde->dd, "JAG SDMA(%u) %s:%d %s()\n", sde->this_idx,
		slashstrip(__FILE__), __LINE__, __func__);
	dd_dev_err(sde->dd, "avail: %u\n", avail);
#endif

	spin_lock(&dev->pending_lock);
	/* Search wait list for first QP wanting DMA descriptors. */
	list_for_each_entry_safe(wait, nw, &sde->dmawait, list) {
		u16 num_desc = 0;
		if (!wait->wakeup)
			continue;
		if (n == ARRAY_SIZE(waits))
			break;
		if (!list_empty(&wait->tx_head)) {
			stx = list_first_entry(
				&wait->tx_head,
				struct sdma_txreq,
				list);
			num_desc = stx->num_desc;
		}
		if (num_desc > avail)
			break;
		avail -= num_desc;
		list_del_init(&wait->list);
		waits[n++] = wait;
	}
	spin_unlock(&dev->pending_lock);

	for (i = 0; i < n; i++)
		waits[i]->wakeup(waits[i], SDMA_AVAIL_REASON);
}

/* sdma_lock must be held */
/* FIXME - convert to new routine */
static int sdma_make_progress(struct sdma_engine *sde, u64 status)
{
	struct sdma_txreq *txp = NULL;
	int progress = 0;
	u16 hwhead, swhead;

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

	if (!list_empty(&sde->activelist))
		txp = list_first_entry(&sde->activelist,
			struct sdma_txreq, list);
	swhead = sde->descq_head & sde->sdma_mask;
	while (swhead != hwhead) {
		/* advance head, wrap if needed */
		swhead = ++sde->descq_head & sde->sdma_mask;

		/* if now past this txp's descs, do the callback */
		/* FIXME - old api code fragment */
		if (txp && txp->next_descq_idx == swhead) {
			int drained = 0;
			/* protect against complete modifying */
			struct iowait *wait = txp->wait;

			/* remove from active list */
			list_del_init(&txp->list);
			if (wait)
				drained = atomic_dec_and_test(
						&wait->sdma_busy);
			sdma_txclean(sde->dd, txp);
			if (txp->complete)
				(*txp->complete)(
					txp,
					SDMA_TXREQ_S_OK,
					drained);
			if (drained)
				iowait_drain_wakeup(wait);
			/* see if there is another txp */
			if (list_empty(&sde->activelist))
				txp = NULL;
			else
				txp = list_first_entry(&sde->activelist,
					struct sdma_txreq, list);
		}
		progress++;
	}
	sde->last_status = status;
	if (progress)
		sdma_desc_avail(sde, sdma_descq_freecnt(sde));
	return progress;
}

/* sdma_lock must be held */
/* Old API - delete */
int qib_sdma_make_progress(struct sdma_engine *sde)
{
	return sdma_make_progress(sde, 0);
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

	trace_hfi_sdma_engine_interrupt(sde, status);
	spin_lock_irqsave(&sde->lock, flags);
	sdma_make_progress(sde, status);
	spin_unlock_irqrestore(&sde->lock, flags);
}

/**
 * sdma_engine_error() - error handler for engine
 * @sde: sdma engine
 * @status: sdma interrupt reason
 */
void sdma_engine_error(struct sdma_engine *sde, u64 status)
{
#ifdef JAG_SDMA_VERBOSITY
	dd_dev_err(sde->dd, "JAG SDMA(%u) error status 0x%llx state %s\n",
		sde->this_idx,
		(unsigned long long)status,
		sdma_state_names[sde->state.current_state]);
#endif
	if (status & ~WFR_SEND_DMA_ENG_ERR_STATUS_SDMA_HALT_ERR_SMASK) {
		dd_dev_err(sde->dd,
			"SDMA (%u) engine error: 0x%llx state %s\n",
			sde->this_idx,
			(unsigned long long)status,
			sdma_state_names[sde->state.current_state]);
		dump_sdma_state(sde);
	}
	if (status & ALL_SDMA_ENG_HALT_ERRS)
		sdma_process_event(sde, sdma_event_e60_hw_halted);
}

static void sdma_sendctrl(struct sdma_engine *sde, unsigned op)
{
	u64 set_senddmactrl = 0;
	u64 clr_senddmactrl = 0;

#ifdef JAG_SDMA_VERBOSITY
	dd_dev_err(sde->dd, "JAG SDMA(%u) senddmactrl E=%d I=%d H=%d C=%d\n",
		sde->this_idx,
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

static inline void sdma_update_tail(struct sdma_engine *sde, u16 tail)
{
	/* Commit writes to memory and advance the tail on the chip */
	smp_wmb();
	writeq(tail, sde->tail_csr);
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
	dd_dev_err(sde->dd, "JAG SDMA(%u) %s:%d %s()\n", sde->this_idx,
		slashstrip(__FILE__), __LINE__, __func__);
#endif

	reg &= WFR_SEND_DMA_DESC_CNT_CNT_MASK;
	reg <<= WFR_SEND_DMA_DESC_CNT_CNT_SHIFT;
	write_sde_csr(sde, WFR_SEND_DMA_DESC_CNT, reg);
}

static void init_sdma_regs(
	struct sdma_engine *sde,
	u32 credits,
	uint idle_cnt)
{
	u8 opval, opmask;

#ifdef JAG_SDMA_VERBOSITY
	dd_dev_err(sde->dd, "JAG SDMA(%u) %s:%d %s()\n",
		sde->this_idx, slashstrip(__FILE__), __LINE__, __func__);
#endif

	write_sde_csr(sde, WFR_SEND_DMA_BASE_ADDR, sde->descq_phys);
	sdma_setlengen(sde);
	sdma_update_tail(sde, 0); /* Set SendDmaTail */
	write_sde_csr(sde, WFR_SEND_DMA_RELOAD_CNT, idle_cnt);
	write_sde_csr(sde, WFR_SEND_DMA_DESC_CNT, 0);
	write_sde_csr(sde, WFR_SEND_DMA_HEAD_ADDR, sde->head_phys);
	write_sde_csr(sde, WFR_SEND_DMA_MEMORY,
		((u64)credits <<
			WFR_SEND_DMA_MEMORY_SDMA_MEMORY_CNT_SHIFT) |
		((u64)(credits * sde->this_idx) <<
			WFR_SEND_DMA_MEMORY_SDMA_MEMORY_INDEX_SHIFT));
	write_sde_csr(sde, WFR_SEND_DMA_ENG_ERR_MASK, ~0ull);
	if (likely(!disable_integrity))
		write_sde_csr(sde, WFR_SEND_DMA_CHECK_ENABLE,
				HFI_PKT_BASE_SDMA_INTEGRITY);
	opmask = WFR_OPCODE_CHECK_MASK_DISABLED;
	opval = WFR_OPCODE_CHECK_VAL_DISABLED;
	write_sde_csr(sde, WFR_SEND_DMA_CHECK_OPCODE,
		(opmask << WFR_SEND_CTXT_CHECK_OPCODE_MASK_SHIFT) |
		(opval << WFR_SEND_CTXT_CHECK_OPCODE_VALUE_SHIFT));
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

	head = sde->descq_head & sde->sdma_mask;
	tail = sde->descq_tail & sde->sdma_mask;
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
		dd_dev_err(sde->dd,
			"\tdesc0:0x%016llx desc1 0x%016llx\n",
			 desc[0], desc[1]);
		if (desc[0] & SDMA_DESC0_FIRST_DESC_FLAG)
			dd_dev_err(sde->dd,
				"\taidx: %u amode: %u alen: %u\n",
				(u8)((desc[1] & SDMA_DESC1_HEADER_INDEX_SMASK)
					>> SDMA_DESC1_HEADER_INDEX_MASK),
				(u8)((desc[1] & SDMA_DESC1_HEADER_MODE_SMASK)
					>> SDMA_DESC1_HEADER_MODE_SHIFT),
				(u8)((desc[1] & SDMA_DESC1_HEADER_DWS_SMASK)
					>> SDMA_DESC1_HEADER_DWS_SHIFT));
		if (++head == sde->descq_cnt)
			head = 0;
	}

	/* print dma descriptor indices from the TX requests */
	list_for_each_entry_safe(txp, txpnext, &sde->activelist, list) {
		dd_dev_err(sde->dd,
			"SDMA txp->next_descq_idx: %u\n",
			txp->next_descq_idx);
	}
}

/* TODO augment this to dump slid check register */
#define SDE_FMT \
	"SDE %u STE %s C 0x%llx S 0x%016llx E 0x%llx T(HW) 0x%llx T(SW) 0x%x H(HW) 0x%llx H(SW) 0x%x H(D) 0x%llx DM 0x%llx GL 0x%llx R 0x%llx LIS 0x%llx AHGI 0x%llx\n"
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

	head = sde->descq_head & sde->sdma_mask;
	tail = sde->descq_tail & sde->sdma_mask;
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
		(unsigned long long)read_sde_csr(sde, WFR_SEND_DMA_MEMORY),
		(unsigned long long)read_sde_csr(sde, WFR_SEND_DMA_LEN_GEN),
		(unsigned long long)read_sde_csr(sde, WFR_SEND_DMA_RELOAD_CNT),
		(unsigned long long)sde->last_status,
		(unsigned long long)sde->ahg_bits);

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
		if (desc[0] & SDMA_DESC0_FIRST_DESC_FLAG)
			seq_printf(s, "\t\tahgidx: %u ahgmode: %u\n",
				(u8)((desc[1] & SDMA_DESC1_HEADER_INDEX_SMASK)
					>> SDMA_DESC1_HEADER_INDEX_MASK),
				(u8)((desc[1] & SDMA_DESC1_HEADER_MODE_SMASK)
					>> SDMA_DESC1_HEADER_MODE_SHIFT));
		if (++head == sde->descq_cnt)
			head = 0;
	}
}

/*
 * return the mode as indicated by the first
 * descriptor in the tx.
 */
static inline u8 ahg_mode(struct sdma_txreq *tx)
{
	return (tx->descp[0].qw[1] & SDMA_DESC1_HEADER_MODE_SMASK)
		>> SDMA_DESC1_HEADER_MODE_SHIFT;
}

/*
 * add the generation number into
 * the qw1 and return
 */
static inline u64 add_gen(struct sdma_engine *sde, u64 qw1)
{
	u8 generation = (sde->descq_tail >> sde->sdma_shift) & 3;
	qw1 &= ~SDMA_DESC1_GENERATION_SMASK;
	qw1 |= ((u64)generation & SDMA_DESC1_GENERATION_MASK)
			<< SDMA_DESC1_GENERATION_SHIFT;
	return qw1;
}

/*
 * This routine submits the indicated tx
 *
 * Space has already been guaranteed and
 * tail side of ring is locked.
 *
 * The hardware tail update is done
 * in the caller and that is facilitated
 * by returning the new tail.
 *
 * There is special case logic for ahg
 * to not add the generation number for
 * up to 2 descriptors that follow the
 * first descriptor.
 *
 */
static inline u16 submit_tx(struct sdma_engine *sde, struct sdma_txreq *tx)
{
	int i;
	u16 tail;
	struct sdma_desc *descp = tx->descp;
	u8 skip = 0, mode = ahg_mode(tx);

	tail = sde->descq_tail & sde->sdma_mask;
	sde->descq[tail].qw[0] = cpu_to_le64(descp->qw[0]);
	sde->descq[tail].qw[1] = cpu_to_le64(add_gen(sde, descp->qw[1]));
	trace_hfi_sdma_descriptor(sde, descp->qw[0], descp->qw[1],
		tail, &sde->descq[tail]);
	tail = ++sde->descq_tail & sde->sdma_mask;
	descp++;
	if (mode > SDMA_AHG_APPLY_UPDATE1)
		skip = mode >> 1;
	for (i = 1; i < tx->num_desc; i++, descp++) {
		u64 qw1;
		sde->descq[tail].qw[0] = cpu_to_le64(descp->qw[0]);
		if (skip) {
			/* edits don't have generation */
			qw1 = descp->qw[1];
			skip--;
		} else {
			/* replace generation with real one for non-edits */
			qw1 = add_gen(sde, descp->qw[1]);
		}
		sde->descq[tail].qw[1] = cpu_to_le64(qw1);
		trace_hfi_sdma_descriptor(sde, descp->qw[0], qw1,
			tail, &sde->descq[tail]);
		tail = ++sde->descq_tail & sde->sdma_mask;
	}
	tx->next_descq_idx = tail;
	list_add_tail(&tx->list, &sde->activelist);
	sde->desc_avail -= tx->num_desc;
	return tail;
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
 * 0 - Success, -EINVAL - sdma_txreq incomplete, -EBUSY - no space in
 * ring (wait == NULL)
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
	u16 tail;
	unsigned long flags;

	/* user should have supplied entire packet */
	if (unlikely(tx->tlen))
		return -EINVAL;
	spin_lock_irqsave(&sde->lock, flags);
retry:
	if (unlikely(!__sdma_running(sde)))
		goto unlock_noconn;
	if (unlikely(tx->num_desc > sde->desc_avail))
		goto nodesc;
	tail = submit_tx(sde, tx);
	tx->wait = wait;
	if (wait)
		atomic_inc(&wait->sdma_busy);
	sdma_update_tail(sde, tail);
unlock:
	spin_unlock_irqrestore(&sde->lock, flags);
	return ret;
unlock_noconn:
	if (wait)
		atomic_inc(&wait->sdma_busy);
	tx->next_descq_idx = 0;
	list_add_tail(&tx->list, &sde->activelist);
	tx->wait = wait;
	if (wait) {
		wait->tx_count++;
		wait->count += tx->num_desc;
	}
	clear_sdma_activelist(sde);
	ret = -ECOMM;
	goto unlock;
nodesc:
	sde->desc_avail = sdma_descq_freecnt(sde);
	if (tx->num_desc <= sde->desc_avail)
		goto retry;
	if (sdma_make_progress(sde, 0)) {
		sde->desc_avail = sdma_descq_freecnt(sde);
		goto retry;
	}
	if (sde->dd->flags & QIB_HAS_SDMA_TIMEOUT)
		sdma_set_desc_cnt(sde, sde->descq_cnt / 2);
	if (busycb && wait)
		busycb(tx, wait);
	if (wait && wait->sleep)
		ret = wait->sleep(wait, tx);
	else
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
 * 0 - Success, -EINVAL - sdma_txreq incomplete, -EBUSY - no space in ring
 * (wait == NULL)
 * -EIOCBQUEUED - tx queued to iowait, -ECOMM bad sdma state
 */
int sdma_send_txlist(struct sdma_engine *sde,
		    struct iowait *wait,
		    void (*busycb)(
			struct sdma_txreq *tx,
			struct iowait *wait),
		    struct list_head *tx_list)
{
	struct sdma_txreq *tx, *tx_next;
	int ret = 0;
	unsigned long flags;
	u16 freecnt;
	u16 tail = INVALID_TAIL;

	spin_lock_irqsave(&sde->lock, flags);
retry:
	freecnt = sdma_descq_freecnt(sde);
	list_for_each_entry_safe(tx, tx_next, tx_list, list) {
		if (unlikely(!__sdma_running(sde)))
			goto unlock_noconn;
		if (unlikely(tx->num_desc > sde->desc_avail))
			goto nodesc;
		if (unlikely(tx->tlen)) {
			ret = -EINVAL;
			goto update_tail;
		}
		list_del_init(&tx->list);
		tail = submit_tx(sde, tx);
		tx->wait = wait;
		if (wait)
			atomic_inc(&wait->sdma_busy);
	}
update_tail:
	if (tail != INVALID_TAIL)
		sdma_update_tail(sde, tail);
unlock:
	spin_unlock_irqrestore(&sde->lock, flags);
	return ret;
unlock_noconn:
	tx->wait = wait;
	if (wait)
		atomic_inc(&wait->sdma_busy);
	tx->next_descq_idx = 0;
	list_add_tail(&tx->list, &sde->activelist);
	if (wait) {
		wait->tx_count++;
		wait->count += tx->num_desc;
	}
	clear_sdma_activelist(sde);
	ret = -ECOMM;
	goto unlock;
nodesc:
	sde->desc_avail = sdma_descq_freecnt(sde);
	if (tx->num_desc <= sde->desc_avail)
		goto retry;
	if (sdma_make_progress(sde, 0)) {
		sde->desc_avail = sdma_descq_freecnt(sde);
		goto retry;
	}
	if (sde->dd->flags & QIB_HAS_SDMA_TIMEOUT)
		sdma_set_desc_cnt(sde, sde->descq_cnt / 2);
	if (busycb && wait)
		busycb(tx, wait);
	if (wait && wait->sleep)
		ret = wait->sleep(wait, tx);
	else
		ret = -EBUSY;
	goto update_tail;
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
#if defined(JAG_SDMA_VERBOSITY) || defined(JAG_SDMA_STATE_TRACE)
	dd_dev_err(sde->dd, "JAG SDMA(%u) [%s] %s\n", sde->this_idx,
		sdma_state_names[ss->current_state], sdma_event_names[event]);
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
			sdma_start_err_halt_wait(sde);
			break;
		case sdma_event_e15_hw_halt_done:
			break;
		case sdma_event_e25_hw_clean_up_done:
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
		case sdma_event_e15_hw_halt_done:
			sdma_set_state(sde,
				sdma_state_s15_hw_start_up_clean_wait);
			sdma_start_hw_clean_up(sde);
			break;
		case sdma_event_e25_hw_clean_up_done:
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
		case sdma_event_e15_hw_halt_done:
			break;
		case sdma_event_e25_hw_clean_up_done:
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
		case sdma_event_e15_hw_halt_done:
			break;
		case sdma_event_e25_hw_clean_up_done:
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
			sdma_set_state(sde, sdma_state_s50_hw_halt_wait);
			sdma_start_err_halt_wait(sde);
			break;
		case sdma_event_e70_go_idle:
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
		case sdma_event_e15_hw_halt_done:
			break;
		case sdma_event_e25_hw_clean_up_done:
			break;
		case sdma_event_e30_go_running:
			ss->go_s99_running = 1;
			break;
		case sdma_event_e40_sw_cleaned:
			sdma_hw_start_up(sde);
			sdma_set_state(sde, ss->go_s99_running ?
				       sdma_state_s99_running :
				       sdma_state_s20_idle);
			break;
		case sdma_event_e50_hw_cleaned:
			break;
		case sdma_event_e60_hw_halted:
			break;
		case sdma_event_e70_go_idle:
			ss->go_s99_running = 0;
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
		case sdma_event_e15_hw_halt_done:
			break;
		case sdma_event_e25_hw_clean_up_done:
			sdma_set_state(sde, sdma_state_s30_sw_clean_up_wait);
			sdma_start_sw_clean_up(sde);
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
		case sdma_event_e15_hw_halt_done:
			sdma_set_state(sde, sdma_state_s40_hw_clean_up_wait);
			sdma_start_hw_clean_up(sde);
			break;
		case sdma_event_e25_hw_clean_up_done:
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
		}
		break;

	case sdma_state_s60_idle_halt_wait:
		switch (event) {
		case sdma_event_e00_go_hw_down:
			sdma_set_state(sde, sdma_state_s00_hw_down);
			sdma_start_sw_clean_up(sde);
			break;
		case sdma_event_e10_go_hw_start:
			break;
		case sdma_event_e15_hw_halt_done:
			sdma_set_state(sde, sdma_state_s40_hw_clean_up_wait);
			sdma_start_hw_clean_up(sde);
			break;
		case sdma_event_e25_hw_clean_up_done:
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
		case sdma_event_e15_hw_halt_done:
			break;
		case sdma_event_e25_hw_clean_up_done:
			break;
		case sdma_event_e30_go_running:
			break;
		case sdma_event_e40_sw_cleaned:
			break;
		case sdma_event_e50_hw_cleaned:
			break;
		case sdma_event_e60_hw_halted:
			sdma_set_state(sde, sdma_state_s50_hw_halt_wait);
			sdma_start_err_halt_wait(sde);
			break;
		case sdma_event_e70_go_idle:
			sdma_set_state(sde, sdma_state_s60_idle_halt_wait);
			sdma_start_err_halt_wait(sde);
			break;
		}
		break;
	}

	ss->last_event = event;
}

/*
 * _extend_sdma_tx_descs() - helper to extend txreq
 *
 * This is called once the initial nominal allocation
 * of descriptors in the sdma_txreq is exhausted.
 *
 * The code will bump the allocation up to the max
 * of MAX_DESC (64) descriptors.  There doesn't seem
 * much point in an interim step.
 *
 * TODO:
 * - Add coalesce processing for > MAX_DESC descriptors
 *
 */
int _extend_sdma_tx_descs(struct hfi_devdata *dd, struct sdma_txreq *tx)
{
	int i;

	tx->descp = kmalloc(MAX_DESC * sizeof(struct sdma_desc) , GFP_ATOMIC);
	if (!tx->descp)
		return -ENOMEM;
	tx->desc_limit = MAX_DESC;
	/* copy ones already built */
	for (i = 0; i < tx->num_desc; i++)
		tx->descp[i] = tx->descs[i];
	return 0;
}

/* Update sdes when the lmc changes */
void sdma_update_lmc(struct hfi_devdata *dd, u64 mask, u32 lid)
{
	struct sdma_engine *sde;
	int i;
	u64 sreg;

	sreg = ((mask & WFR_SEND_DMA_CHECK_SLID_MASK_MASK) <<
		WFR_SEND_DMA_CHECK_SLID_MASK_SHIFT) |
		(((lid & mask) & WFR_SEND_DMA_CHECK_SLID_VALUE_MASK) <<
		WFR_SEND_DMA_CHECK_SLID_VALUE_SHIFT);

	for (i = 0; i < dd->num_sdma; i++) {
		hfi_cdbg(LINKVERB, "SendDmaEngine[%d].SLID_CHECK = 0x%x",
			 i, (u32)sreg);
		sde = &dd->per_sdma[i];
		write_sde_csr(sde, WFR_SEND_DMA_CHECK_SLID, sreg);
	}
}

/* tx not dword sized - pad */
int _pad_sdma_tx_descs(struct hfi_devdata *dd, struct sdma_txreq *tx)
{
	int rval = 0;

	if ((unlikely(tx->num_desc == tx->desc_limit))) {
		rval = _extend_sdma_tx_descs(dd, tx);
		if (rval)
			return rval;
	}
	/* finish the one just added  */
	tx->num_desc++;
	make_tx_sdma_desc(
		tx,
		SDMA_MAP_NONE,
		dd->sdma_pad_phys,
		sizeof(u32) - (tx->packet_len & (sizeof(u32) - 1)));
	_sdma_close_tx(dd, tx);
	return rval;
}

/*
 * Add ahg to the sdma_txreq
 *
 * The logic will consume up to 3
 * descriptors at the beginning of
 * sdma_txreq.
 */
void _sdma_txreq_ahgadd(
	struct sdma_txreq *tx,
	u8 num_ahg,
	u8 ahg_entry,
	u32 *ahg,
	u8 ahg_hlen)
{
	u32 i, shift = 0, desc = 0;
	u8 mode;

	BUG_ON(num_ahg > 9 || (ahg_hlen & 3) || ahg_hlen == 4);
	/* compute mode */
	if (num_ahg == 1)
		mode = SDMA_AHG_APPLY_UPDATE1;
	else if (num_ahg <= 5)
		mode = SDMA_AHG_APPLY_UPDATE2;
	else
		mode = SDMA_AHG_APPLY_UPDATE3;
	tx->num_desc++;
	/* initialize to consumed descriptors to zero */
	switch (mode) {
	case SDMA_AHG_APPLY_UPDATE3:
		tx->num_desc++;
		tx->descs[2].qw[0] = 0;
		tx->descs[2].qw[1] = 0;
		/* FALLTHROUGH */
	case SDMA_AHG_APPLY_UPDATE2:
		tx->num_desc++;
		tx->descs[1].qw[0] = 0;
		tx->descs[1].qw[1] = 0;
		break;
	}
	ahg_hlen >>= 2;
	tx->descs[0].qw[1] |=
		(((u64)ahg_entry & SDMA_DESC1_HEADER_INDEX_MASK)
			<< SDMA_DESC1_HEADER_INDEX_SHIFT) |
		(((u64)ahg_hlen & SDMA_DESC1_HEADER_DWS_MASK)
			<< SDMA_DESC1_HEADER_DWS_SHIFT) |
		(((u64)mode & SDMA_DESC1_HEADER_MODE_MASK)
			<< SDMA_DESC1_HEADER_MODE_SHIFT) |
		(((u64)ahg[0] & SDMA_DESC1_HEADER_UPDATE1_MASK)
			<< SDMA_DESC1_HEADER_UPDATE1_SHIFT);
	for (i = 0; i < (num_ahg - 1); i++) {
		if (!shift && !(i & 2))
			desc++;
		tx->descs[desc].qw[!!(i & 2)] |=
			cpu_to_le64((((u64)ahg[i + 1])
				<< shift));
		shift = (shift + 32) & 63;
	}
}

/**
 * sdma_ahg_alloc - allocate an AHG entry
 * @sde: engine to allocate from
 *
 * Return:
 * 0-31 when succesfull, -EOPNOTSUPP if AHG is not enabled,
 * -ENOSPC if an entry is not available
 */
int sdma_ahg_alloc(struct sdma_engine *sde)
{
	int nr;
	int oldbit;

	if (!sde) {
		trace_hfi_ahg_allocate(sde, -EINVAL);
		return -EINVAL;
	}
	if (!use_sdma_ahg) {
		trace_hfi_ahg_allocate(sde, -EOPNOTSUPP);
		return -EOPNOTSUPP;
	}
	while (1) {
		nr = ffz(ACCESS_ONCE(sde->ahg_bits));
		if (nr > 31) {
			trace_hfi_ahg_allocate(sde, -ENOSPC);
			return -ENOSPC;
		}
		oldbit = test_and_set_bit(nr, &sde->ahg_bits);
		if (!oldbit)
			break;
		cpu_relax();
	}
	trace_hfi_ahg_allocate(sde, nr);
	return nr;
}

/**
 * sdma_ahg_free - free an AHG entry
 * @sde: engine to return AHG entry
 * @ahg_index: index to free
 *
 * This routine frees the indicate AHG entry.
 */
void sdma_ahg_free(struct sdma_engine *sde, int ahg_index)
{
	if (!sde)
		return;
	trace_hfi_ahg_deallocate(sde, ahg_index);
	if (ahg_index < 0 || ahg_index > 31)
		return;
	clear_bit(ahg_index, &sde->ahg_bits);
}
