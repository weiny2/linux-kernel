#ifndef _HFI_SDMA_H
#define _HFI_SDMA_H
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

#include <linux/types.h>
#include <linux/list.h>
#include <asm/byteorder.h>
#include <linux/workqueue.h>

#include "hfi.h"
#include "verbs.h"

/* increased for AHG */
#define NUM_DESC 6

#define SDMA_TXREQ_S_OK        0
#define SDMA_TXREQ_S_SENDERROR 1
#define SDMA_TXREQ_S_ABORTED   2
#define SDMA_TXREQ_S_SHUTDOWN  3

/* flags bits */
#define SDMA_TXREQ_F_URGENT       0x0001
#define SDMA_TXREQ_F_AHG_COPY     0x0002
#define SDMA_TXREQ_F_USE_AHG      0x0004
/* FIXME - remove when verbs ready */
#define SDMA_TXREQ_F_FREEDESC 0x0008
#define SDMA_TXREQ_F_FREEBUF  0x0010

#define SDMA_MAP_NONE          0
#define SDMA_MAP_SINGLE        1
#define SDMA_MAP_PAGE          2

#define SDMA_AHG_VALUE_MASK          0xff
#define SDMA_AHG_VALUE_SHIFT         0
#define SDMA_AHG_INDEX_MASK          0xf
#define SDMA_AHG_INDEX_SHIFT         16
#define SDMA_AHG_FIELD_LEN_MASK      0xf
#define SDMA_AHG_FIELD_LEN_SHIFT     20
#define SDMA_AHG_FIELD_START_MASK    0x1f
#define SDMA_AHG_FIELD_START_SHIFT   24
#define SDMA_AHG_UPDATE_ENABLE_MASK  0x1
#define SDMA_AHG_UPDATE_ENABLE_SHIFT 31

/*
 * Bits defined in the send DMA descriptor.
 */
#define SDMA_DESC0_FIRST_DESC_FLAG      (1ULL<<63)
#define SDMA_DESC0_LAST_DESC_FLAG       (1ULL<<62)
#define SDMA_DESC0_BYTE_COUNT_SHIFT     48
#define SDMA_DESC0_BYTE_COUNT_WIDTH     14
#define SDMA_DESC0_BYTE_COUNT_MASK \
	((1ULL<<SDMA_DESC0_BYTE_COUNT_WIDTH)-1ULL)
#define SDMA_DESC0_BYTE_COUNT_SMASK \
	(SDMA_DESC0_BYTE_COUNT_MASK<<SDMA_DESC0_BYTE_COUNT_SHIFT)
#define SDMA_DESC0_PHY_ADDR_SHIFT       0
#define SDMA_DESC0_PHY_ADDR_WIDTH       48
#define SDMA_DESC0_PHY_ADDR_MASK \
	((1ULL<<SDMA_DESC0_PHY_ADDR_WIDTH)-1ULL)
#define SDMA_DESC0_PHY_ADDR_SMASK \
	(SDMA_DESC0_PHY_ADDR_MASK<<SDMA_DESC0_PHY_ADDR_SHIFT)

#define SDMA_DESC1_HEADER_UPDATE1_SHIFT 32
#define SDMA_DESC1_HEADER_UPDATE1_WIDTH 32
#define SDMA_DESC1_HEADER_UPDATE1_MASK \
	((1ULL<<SDMA_DESC1_HEADER_UPDATE1_WIDTH)-1ULL)
#define SDMA_DESC1_HEADER_UPDATE1_SMASK \
	(SDMA_DESC1_HEADER_UPDATE1_MASK<<SDMA_DESC1_HEADER_UPDATE1_SHIFT)
#define SDMA_DESC1_HEADER_MODE_SHIFT    13
#define SDMA_DESC1_HEADER_MODE_WIDTH    3
#define SDMA_DESC1_HEADER_MODE_MASK \
	((1ULL<<SDMA_DESC1_HEADER_MODE_WIDTH)-1ULL)
#define SDMA_DESC1_HEADER_MODE_SMASK \
	(SDMA_DESC1_HEADER_MODE_MASK<<SDMA_DESC1_HEADER_MODE_SHIFT)
#define SDMA_DESC1_HEADER_INDEX_SHIFT   8
#define SDMA_DESC1_HEADER_INDEX_WIDTH   5
#define SDMA_DESC1_HEADER_INDEX_MASK \
	((1ULL<<SDMA_DESC1_HEADER_INDEX_WIDTH)-1ULL)
#define SDMA_DESC1_HEADER_INDEX_SMASK \
	(SDMA_DESC1_HEADER_INDEX_MASK<<SDMA_DESC1_HEADER_INDEX_SHIFT)
#define SDMA_DESC1_HEADER_DWS_SHIFT     4
#define SDMA_DESC1_HEADER_DWS_WIDTH     4
#define SDMA_DESC1_HEADER_DWS_MASK \
	((1ULL<<SDMA_DESC1_HEADER_DWS_WIDTH)-1ULL)
#define SDMA_DESC1_HEADER_DWS_SMASK \
	(SDMA_DESC1_HEADER_DWS_MASK<<SDMA_DESC1_HEADER_DWS_SHIFT)
#define SDMA_DESC1_GENERATION_SHIFT     2
#define SDMA_DESC1_GENERATION_WIDTH     2
#define SDMA_DESC1_GENERATION_MASK \
	((1ULL<<SDMA_DESC1_GENERATION_WIDTH)-1ULL)
#define SDMA_DESC1_GENERATION_SMASK \
	(SDMA_DESC1_GENERATION_MASK<<SDMA_DESC1_GENERATION_SHIFT)
#define SDMA_DESC1_INT_REQ_FLAG         (1ULL<<1)
#define SDMA_DESC1_HEAD_TO_HOST_FLAG    (1ULL<<0)

enum sdma_states {
	sdma_state_s00_hw_down,
	sdma_state_s10_hw_start_up_halt_wait,
	sdma_state_s15_hw_start_up_clean_wait,
	sdma_state_s20_idle,
	sdma_state_s30_sw_clean_up_wait,
	sdma_state_s40_hw_clean_up_wait,
	sdma_state_s50_hw_halt_wait,
	sdma_state_s99_running,
};

enum sdma_events {
	sdma_event_e00_go_hw_down,
	sdma_event_e10_go_hw_start,
	sdma_event_e15_hw_started1,
	sdma_event_e25_hw_started2,
	sdma_event_e30_go_running,
	sdma_event_e40_sw_cleaned,
	sdma_event_e50_hw_cleaned,
	sdma_event_e60_hw_halted,
	sdma_event_e70_go_idle,
	sdma_event_e7220_err_halted,
	sdma_event_e7322_err_halted,
	sdma_event_e90_timer_tick,
};

struct sdma_set_state_action {
	unsigned op_enable:1;
	unsigned op_intenable:1;
	unsigned op_halt:1;
	unsigned op_drain:1;
	unsigned op_cleanup:1;
	unsigned go_s99_running_tofalse:1;
	unsigned go_s99_running_totrue:1;
};

struct sdma_state {
	struct kref          kref;
	struct completion    comp;
	enum sdma_states current_state;
	unsigned             current_op;
	unsigned             go_s99_running;
	/* debugging/devel */
	enum sdma_states previous_state;
	unsigned             previous_op;
	enum sdma_events last_event;
};

/**
 * DOC: sdma exported routines
 *
 * These sdma routines fit into three categories:
 * - The SDMA API for building and submitting packets
 *   to the ring
 *
 * - Intialization and teardown routines to buildup
 *   and teardown SDMA
 *
 * - ISR entrances to handle interrupts, state changes
 *   and errors
 */

/**
 * DOC: sdma PSM/verbs API
 *
 * The sdma API is designed to be used by both PSM
 * and verbs to supply packets to the SDMA ring.
 *
 * The usage of the API is as follows:
 *
 * Embed a struct iowait in the QP or
 * PQ.  The iowait should be initialized with a
 * call to iowait_init().
 *
 * The user of the API should create an allocation method
 * for their version of the txreq. slabs, pre-allocated lists,
 * and dma pools can be used.  Once the user's overload of
 * the sdma_txreq has been allocated, the sdma_txreq member
 * must be initialized with sdma_txinit() or sdma_txinit_ahg().
 *
 * The txreq must be declared with the sdma_txreq first.
 *
 * The tx request, once initialized,  is manipulated with calls to
 * sdma_txadd_daddr(), sdma_txadd_page(), or sdma_txadd_kvaddr()
 * for each disjoint memory location.  It is the user's responsibility
 * to understand the packet boundaries and page boundaries to do the
 * appropriate number of sdma_txadd_* calls..  The user
 * must be prepared to deal with failures from these routines due to
 * either memory allocation or dma_mapping failures.
 *
 * The mapping specifics for each memory location are recorded
 * in the tx. Memory locations added with sdma_txadd_page()
 * and sdma_txadd_kvaddr() are automatically mapped when added
 * to the tx and nmapped as part of the progess processing in the
 * SDMA interrupt handling.
 *
 * sdma_txadd_daddr() is used to add an dma_addr_t memory to the
 * tx.   An example of a use case would be a pre-allocated
 * set of headers allocated via dma_pool_alloc() or
 * dma_alloc_coherent().  For these memory locations, it
 * is the responsibility of the user to handle that unmapping.
 * (This would usually be at an unload or job termination.)
 *
 * The routine sdma_send_txreq() is used to submit
 * a tx to the ring after the appropriate nubmer of
 * sdma_txadd_* have been done.
 *
 * If it is desired to send a burst of sdma_txreqs, sdma_send_txlist()
 * can be used to submit a list of packets.
 *
 * The user is free to use the link overhead in the struct sdma_txreq as
 * long as the tx isn't in flight.
 *
 * The extreme degenerate case of the number of descriptors
 * exceeding the ring size is automatically handled as
 * memory locations are added.  An overflow of the descriptor
 * array that is part of the sdma_txreq is also automatically
 * handled.
 *
 */

/**
 * DOC: Infrastructure calls
 *
 * sdma_init() is used to initialize data structures and
 * csrs for the desired number of SDMA engines.
 *
 * sdma_start() is used to kick the SDMA engines initialized
 * with sdma_init().   Interrupts must be enabled at this
 * point since aspects of the state machine are interrupt
 * driven.
 *
 * sdma_engine_error() and sdma_engine_interrupt() are
 * entrances for interrupts.
 *
 * sdma_map_init() is for the management of the mapping
 * table when the number of vls is changed.
 *
 */

/*
 * struct hw_sdma_desc - raw 128 bit SDMA descriptor
 *
 * This is the raw descriptor in the SDMA ring
 */
struct hw_sdma_desc {
	/* private:  don't use directly */
	__le64 qw[2];
};

/*
 * struct sdma_desc - canonical fragment descriptor
 *
 * This is the descriptor carried in the tx request
 * cooresponding to each fragment.
 *
 */
struct sdma_desc {
	/* private:  don't use directly */
	u64 qw[2];
};

struct sdma_txreq;
typedef void (*callback_t)(struct sdma_txreq *, int);

/**
 * struct sdma_txreq - the sdma_txreq structure (one per packet)
 * @list: for use by user and by queuing for wait
 *
 * This is the representation of a packet which consists of some
 * number of fragments.   Storage is provided to within the structure.
 * for all fragments.
 *
 * The storage for the descriptors are automatically extended as needed
 * when the currently allocation is exceeded.
 *
 * The user (Verbs or PSM) may overload this structure with fields
 * specific to their use by putting this struct first in their struct.
 * The method of allocation of the overloaded structure is user dependent
 *
 * The list is the only public field in the structure.
 *
 */

struct sdma_txreq {
	struct list_head list;
	/* private: */
	struct sdma_desc *descp;
	/* private: */
	void *coalesce_buf;
	/* private: */
	callback_t                  complete;
	/* private: - used in coalesce processing */
	u16                         packet_len;
	/* private: - downcounteded to trigger last */
	u16                         tlen;
	/* private: flags */
	u16                         flags;
	/* private: */
	u16                         num_desc;
	/* private: */
	u16                         desc_limit;
	/* private: */
	u16                         start_idx;
	/* private: */
	u16                         next_descq_idx;
	/* private: */
	struct sdma_desc descs[NUM_DESC];
};

/* FIXME - remove when verbs done */
struct qib_verbs_txreq {
	struct sdma_txreq       txreq;
	struct qib_qp           *qp;
	struct qib_swqe         *wqe;
	dma_addr_t              addr;
	u32                     dwords;
	u16                     hdr_dwords;
	u16                     hdr_inx;
	struct qib_pio_header	*align_buf;
	struct qib_mregion	*mr;
	struct qib_sge_state    *ss;
};

/**
 * struct sdma_engine - Data pertaining to each SDMA engine.
 * @dd: a backpointer to the device data
 * @ppd: per port backpointer
 * @imask: make for irq manipulation
 *
 * This structure has the start for each sdma_engine.
 *
 * Accessing to non public fields are not supported
 * since the private members are subject to change.
 */
struct sdma_engine {
	/* read mostly */
	struct hfi_devdata *dd;
	struct qib_pportdata *ppd;
	u64 imask;			/* clear interrupt mask */
	/* private: */
	__le64 default_desc1;
	/* private: */
	struct workqueue_struct *wq;
	/* add sdma fields here... */
	/* private: */
	/* protect changes to senddmactrl shadow */
	spinlock_t senddmactrl_lock;
	/* private: */
	u64 p_senddmactrl;		/* shadow per-engine SendDmaCtrl */
	/* private: */
	u16 descq_cnt;

	/* JAG SDMA Im Bau - everything above this line suspect */
	/* private: */
	u8              this_idx; /* zero relative engine */
	/* private: */
	struct hw_sdma_desc *descq;
	/* private: */
	dma_addr_t            descq_phys;
	/* private: */
	volatile __le64      *head_dma;
	/* private: */
	dma_addr_t            head_phys;
	/* private: */
	struct sdma_state state;
	/* private: */
	u8                    generation;
	/* read/write using lock */
	/* private: */
	spinlock_t            lock ____cacheline_aligned_in_smp;
	/* private: */
	struct list_head      activelist;
	/* private: */
	u64                   descq_added;
	/* private: */
	u64                   descq_removed;
	/* private: */
	u16                   descq_tail;
	/* private: */
	u16                   descq_head;
	/* private: */
	struct list_head      dmawait;

	/* JAG SDMA for now, just blindly duplicate */
	/* private: */
	struct tasklet_struct sdma_hw_clean_up_task
		____cacheline_aligned_in_smp;

	/* private: */
	struct tasklet_struct sdma_sw_clean_up_task
		____cacheline_aligned_in_smp;
};


int sdma_init(struct hfi_devdata *dd, u8 port, size_t num_engines);
void sdma_start(struct hfi_devdata *dd);
void sdma_exit(struct hfi_devdata *dd);

/**
 * sdma_empty() - idle engine test
 * @engine: sdma engine
 *
 * Currently used by verbs as a latency optimization.
 *
 * Return:
 * 1 - empty, 0 - non-empty
 */
static inline int sdma_empty(struct sdma_engine *engine)
{
	return engine->descq_added == engine->descq_removed;
}

/* must be called under lock */
static inline u16 sdma_descq_freecnt(struct sdma_engine *engine)
{
	return engine->descq_cnt -
		(engine->descq_added -
		 ACCESS_ONCE(engine->descq_removed)) - 1;
}

static inline int __sdma_running(struct sdma_engine *engine)
{
	return engine->state.current_state == sdma_state_s99_running;
}

/**
 * sdma_running() - state suitability test
 * @engine: sdma engine
 *
 * sdma_running probes the internal state to determine if it is suitable
 * for submitting packets.
 *
 * Return:
 * 1 - ok to submit, 0 - not ok to submit
 *
 */
static inline int sdma_running(struct sdma_engine *engine)
{
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&engine->lock, flags);
	ret = __sdma_running(engine);
	spin_unlock_irqrestore(&engine->lock, flags);
	return ret;
}


/**
 * sdma_txinit_ahg() - initialize an sdma_txreq struct with AHG
 * @tx: tx request to init
 * @flags: flags to key last descriptor additions
 * @tlen: total packet length (pbc + headers + data)
 * @ahg_entry: ahg entry to use  (0 - 31)
 * @num_ahg: ahg descriptor for first descriptor (0 - 9)
 * @ahg: array of AHG descriptors (up to 9 entries)
 * @cb: callback
 *
 * The allocation of the sdma_txreq and it enclosing structure is user
 * dependent.  This routine must be called to initialize the user independent
 * fields.
 *
 * The currently supported flags are SDMA_TXREQ_F_URGENT,
 * SDMA_TXREQ_F_AHG_COPY, and SDMA_TXREQ_F_USE_AHG.
 *
 * SDMA_TXREQ_F_URGENT is used for latency sensitive situations where the
 * completion is desired as soon as possible.
 *
 * SDMA_TXREQ_F_AHG_COPY causes the header in the first descriptor to be
 * copied to chip entry. SDMA_TXREQ_F_USE_AHG causes the code to add in
 * the AHG descriptors into the first 1 to 3 descriptors.
 *
 * Completions of submitted requests can be gotten on selected
 * txreqs by giving a completion routine callback to sdma_txinit() or
 * sdma_txinit_ahg().  The environment in which the callback runs
 * can be from an ISR, a tasklet, or a thread, so no sleepable
 * kernel routines can be used.   Aspects of the sdma ring may
 * be locked so care should be taken with locking.
 *
 * The callback pointer can be NULL to avoid any callback for the packet
 * being submitted. The callback will be provided this tx and a status.  The
 * status will be one of SDMA_TXREQ_S_OK, SDMA_TXREQ_S_SENDERROR,
 * SDMA_TXREQ_S_ABORTED, or SDMA_TXREQ_S_SHUTDOWN.
 *
 */
static inline void sdma_txinit_ahg(
	struct sdma_txreq *tx,
	u16 flags,
	u16 tlen,
	u8 ahg_entry,
	u8 num_ahg,
	u32 *ahg,
	void (*cb)(struct sdma_txreq *, int))
{
	tx->desc_limit = ARRAY_SIZE(tx->descs);
	tx->descp = &tx->descs[0];
	tx->num_desc = 0;
	tx->flags = flags;
	tx->complete = cb;
	tx->coalesce_buf = NULL;
	BUG_ON(tlen == 0);
	tx->tlen = tx->packet_len = tlen;
	tx->descs[0].qw[0] = SDMA_DESC0_FIRST_DESC_FLAG;
}

/**
 * sdma_txinit() - initialize an sdma_txreq struct (no AHG)
 * @tx: tx request to init
 * @flags: flags to key last descriptor additions
 * @tlen: total packet length (pbc + headers + data)
 * @cb: callback pointer
 *
 * The allocation of the sdma_txreq and it enclosing structure is user
 * dependent.  This routine must be called to initialize the user
 * independent fields.
 *
 * The currently supported flags is SDMA_TXREQ_F_URGENT.
 *
 * SDMA_TXREQ_F_URGENT is used for latency sensitive situations where the
 * completion is desired as soon as possible.
 *
 * Completions of submitted requests can be gotten on selected
 * txreqs by giving a completion routine callback to sdma_txinit() or
 * sdma_txinit_ahg().  The environment in which the callback runs
 * can be from an ISR, a tasklet, or a thread, so no sleepable
 * kernel routines can be used.   The head size of the sdma ring may
 * be locked so care should be taken with locking.
 *
 * The callback pointer can be NULL to avoid any callback for the packet
 * being submitted.
 *
 * The callback, if non-NULL,  will be provided this tx and a status.  The
 * status will be one of SDMA_TXREQ_S_OK, SDMA_TXREQ_S_SENDERROR,
 * SDMA_TXREQ_S_ABORTED, or SDMA_TXREQ_S_SHUTDOWN.
 *
 */
static inline void sdma_txinit(
	struct sdma_txreq *tx,
	u16 flags,
	u16 tlen,
	void (*cb)(struct sdma_txreq *, int status))
{
	sdma_txinit_ahg(tx, flags, tlen, 0, 0, NULL, cb);
}

/* helpers - don't use */
static inline int sdma_mapping_type(struct sdma_desc *d)
{
	return (d->qw[1] & SDMA_DESC1_GENERATION_SMASK)
		>> SDMA_DESC1_GENERATION_SHIFT;
}

static inline size_t sdma_mapping_len(struct sdma_desc *d)
{
	return (d->qw[0] & SDMA_DESC0_BYTE_COUNT_SMASK)
		>> SDMA_DESC0_BYTE_COUNT_SHIFT;
}

static inline dma_addr_t sdma_mapping_addr(struct sdma_desc *d)
{
	return (d->qw[0] & SDMA_DESC0_PHY_ADDR_SMASK)
		>> SDMA_DESC0_PHY_ADDR_SHIFT;
}

static inline void make_tx_sdma_desc(
	int type,
	struct sdma_desc *desc,
	dma_addr_t addr,
	size_t len)
{
	if (desc->qw[0] & SDMA_DESC0_FIRST_DESC_FLAG)
		desc->qw[0] |= ((u64)len & SDMA_DESC0_BYTE_COUNT_MASK)
				<< SDMA_DESC0_BYTE_COUNT_SHIFT;
	else
		desc->qw[0] = ((u64)len & SDMA_DESC0_BYTE_COUNT_MASK)
				<< SDMA_DESC0_BYTE_COUNT_SHIFT;
	desc->qw[0] |= ((u64)addr & SDMA_DESC0_PHY_ADDR_MASK)
				<< SDMA_DESC0_PHY_ADDR_SHIFT;
	desc->qw[1] = ((u64)type & SDMA_DESC1_GENERATION_MASK)
				<< SDMA_DESC1_GENERATION_SHIFT;
}

/* helper to extend txreq */
int _extend_sdma_tx_descs(struct hfi_devdata *, struct sdma_txreq *);
void sdma_txclean(struct hfi_devdata *, struct sdma_txreq *);

/* helper used by public routines */
static inline int _sdma_txadd_daddr(
	struct hfi_devdata *dd,
	int type,
	struct sdma_txreq *tx,
	dma_addr_t addr,
	u16 len)
{
	int rval = 0;

	if ((unlikely(tx->num_desc == tx->desc_limit))) {
		rval = _extend_sdma_tx_descs(dd, tx);
		if (rval)
			return rval;
	}
	make_tx_sdma_desc(
		type,
		&tx->descp[tx->num_desc],
		addr, len);
	BUG_ON(len > tx->tlen);
	tx->tlen -= len;
	/* special cases for last */
	if (!tx->tlen) {
		tx->descp[tx->num_desc].qw[0] |=
			SDMA_DESC0_LAST_DESC_FLAG;
		if (tx->flags & SDMA_TXREQ_F_URGENT)
			tx->descp[tx->num_desc].qw[1] |=
				(SDMA_DESC1_HEAD_TO_HOST_FLAG|
				 SDMA_DESC1_INT_REQ_FLAG);
	}
	tx->num_desc++;
	return rval;
}

/**
 * sdma_txadd_page() - add a page to the sdma_txreq
 * @dd: the device to use for mapping
 * @tx: tx request to which the page is added
 * @page: page to map
 * @offset: offset within the page
 * @len: length in bytes
 *
 * This is used to add a page/offset/length descriptor.
 *
 * The mapping/unmapping of the page/offset/len is automatically handled.
 *
 * Return:
 * 0 - success, -ENOSPC - mapping fail, -ENOMEM - couldn't
 * extend descriptor array or couldn't allocate coalesse
 * buffer.
 *
 */
static inline int sdma_txadd_page(
	struct hfi_devdata *dd,
	struct sdma_txreq *tx,
	struct page *page,
	unsigned long offset,
	u16 len)
{
	dma_addr_t addr =
		dma_map_page(
			&dd->pcidev->dev,
			page,
			offset,
			len,
			DMA_TO_DEVICE);
	if (unlikely(dma_mapping_error(&dd->pcidev->dev, addr))) {
		sdma_txclean(dd, tx);
		return -ENOSPC;
	}
	return _sdma_txadd_daddr(
			dd, SDMA_MAP_PAGE, tx, addr, len);
}

/**
 * sdma_txadd_daddr() - add a dma address to the sdma_txreq
 * @dd: the device to use for mapping
 * @tx: sdma_txreq to which the page is added
 * @addr: dma address mapped by caller
 * @len: length in bytes
 *
 * This is used to add a descriptor for memory that is already dma mapped.
 *
 * In this case, there is no unmapping as part of the progress processing for
 * this memory location.
 *
 * Return:
 * 0 - success, -ENOMEM - couldn't extend descriptor array
 */

static inline int sdma_txadd_daddr(
	struct hfi_devdata *dd,
	struct sdma_txreq *tx,
	dma_addr_t addr,
	u16 len)
{
	return _sdma_txadd_daddr(dd, SDMA_MAP_NONE, tx, addr, len);
}

/**
 * sdma_txadd_kvaddr() - add a kernel virtual address to sdma_txreq
 * @dd: the device to use for mapping
 * @tx: sdma_txreq to which the page is added
 * @kvaddr: the kernel virtual address
 * @len: length in bytes
 *
 * This is used to add a descriptor referenced by the indicated kvaddr and
 * len.
 *
 * The mapping/unmapping of the kvaddr and len is automatically handled.
 *
 * Return:
 * 0 - success, -ENOSPC - mapping fail, -ENOMEM - couldn't extend
 * descriptor array
 */
static inline int sdma_txadd_kvaddr(
	struct hfi_devdata *dd,
	struct sdma_txreq *tx,
	void *kvaddr,
	u16 len)
{
	dma_addr_t addr =
		dma_map_single(
			&dd->pcidev->dev,
			kvaddr,
			len,
			DMA_TO_DEVICE);
	if (unlikely(dma_mapping_error(&dd->pcidev->dev, addr))) {
		sdma_txclean(dd, tx);
		return -ENOSPC;
	}
	return _sdma_txadd_daddr(
			dd, SDMA_MAP_SINGLE, tx, addr, len);
}

struct iowait;

/*
 * typedef busycb_t - busy callback function
 */
typedef void (*busycb_t)(struct sdma_txreq *tx, struct iowait *wait);

int sdma_send_txreq(struct sdma_engine *engine,
		    struct iowait *wait,
		    busycb_t busycb,
		    struct sdma_txreq *tx);

/**
 * sdma_ahg_alloc - allocate an AHG entry
 * @engine: engine to allocate from
 *
 * Return:
 * 0-31 when succesfull, -ENOSPC otherwise
 */
static inline u8 sdma_ahg_alloc(struct sdma_engine *engine)
{
	/* FIXME stub for now */
	return -ENOSPC;
}

/**
 * sdma_ahg_free - free an AHG entry
 * @engine: engine to return AHG entry
 * @ahg_index: index to free
 *
 * This routine frees the indicate AHG entry.
 */
static inline void sdma_ahg_free(struct sdma_engine *engine, u8 ahg_index)
{
	/* FIXME stub for now */
	return;
}

/**
 * sdma_build_ahg - build ahg descriptor
 * @data: per HAS
 * @dwindex: per HAS
 * @startbit: per HAS
 * @bits: per HAS
 *
 * Build and return a 32 bit descriptor.
 */
static inline u32 sdma_build_ahg_descriptor(
	u16 data,
	u8 dwindex,
	u8 startbit,
	u8 bits)
{
	return (u32)(1UL << SDMA_AHG_UPDATE_ENABLE_SHIFT |
		((startbit & SDMA_AHG_FIELD_START_MASK) <<
		SDMA_AHG_FIELD_START_SHIFT) |
		((bits & SDMA_AHG_FIELD_LEN_MASK) <<
		SDMA_AHG_FIELD_LEN_SHIFT) |
		((dwindex & SDMA_AHG_INDEX_MASK) <<
		SDMA_AHG_INDEX_SHIFT) |
		((data & SDMA_AHG_VALUE_MASK) <<
		SDMA_AHG_VALUE_SHIFT));
}

/**
 * sdma_iowait_schedule() - init wait structure
 * @sde: sdma_engine to schedule
 * @wait: wait struct to schedule
 *
 * This function initializes the iowait
 * structure embedded in the QP or PQ.
 *
 */
static inline void sdma_iowait_schedule(
	struct sdma_engine *sde,
	struct iowait *wait)
{
	iowait_schedule(wait, sde->wq);
}

/* for use by interrupt handling */
void sdma_engine_error(struct sdma_engine *sde, u64 status);
void sdma_engine_interrupt(struct sdma_engine *sde, u64 status);
/* for use when the number of vls is changes */
void sdma_map_init(struct hfi_devdata *dd, u8 port, u8 num_vls);

/**
 * sdma_engine_progress_schedule() - schedule progress on engine
 * @sde: sdma_engine to schedule progress
 *
 */
static inline void sdma_engine_progress_schedule(
	struct sdma_engine *sde)
{
	/*
	 * FIXME - need a better mechanism here
	 * that progresses the ring on a
	 * CPU away from receive processing
	 */
	sdma_engine_interrupt(sde, 0);
}

struct sdma_engine *sdma_select_engine_sc(
	struct hfi_devdata *dd,
	u32 selector,
	u8 sc5);

struct sdma_engine *sdma_select_engine_vl(
	struct hfi_devdata *dd,
	u32 selector,
	u8 vl);

/* deprecated for now */
int qib_sdma_make_progress(struct sdma_engine *);
void sdma_update_tail(struct sdma_engine *, u16);

void sdma_seqfile_dump_sde(struct seq_file *s, struct sdma_engine *);

#ifdef JAG_SDMA_VERBOSITY
/* XXX JAG SDMA - Temporary debug/dump routine */
void sdma_dumpstate(struct sdma_engine *);
#endif
static inline char *slashstrip(char *s)
{
	char *r = s;
	while (*s)
		if (*s++ == '/')
			r = s;
	return r;
}

extern uint mod_num_sdma;
u16 sdma_get_descq_cnt(void);

extern void sdma_update_lmc(struct hfi_devdata *dd, u64 mask, u32 lid);


#endif
