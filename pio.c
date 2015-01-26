/*
 * Copyright (c) 2013,2014 Intel Corporation.  All rights reserved.
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

#include <linux/delay.h>
#include "hfi.h"
#include "qp.h"
#include "trace.h"

#define SC_CTXT_PACKET_EGRESS_TIMEOUT 350 /* in chip cycles */
/*
 * Send Context functions
 */
static void sc_wait_for_packet_egress(struct send_context *);

/*
 * Set the CM reset bit and wait for it to clear.  Use the provided
 * sendctrl register.  This routine has no locking.
 */
void __cm_reset(struct hfi_devdata *dd, u64 sendctrl)
{
	write_csr(dd, WFR_SEND_CTRL, sendctrl | WFR_SEND_CTRL_CM_RESET_SMASK);
	while (1) {
		udelay(1);
		sendctrl = read_csr(dd, WFR_SEND_CTRL);
		if ((sendctrl & WFR_SEND_CTRL_CM_RESET_SMASK) == 0)
			break;
	}
}

/* global control of PIO send */
void pio_send_control(struct hfi_devdata *dd, int op)
{
	u64 reg;

//FIXME: QIB held sendctrl_lock when changing the global ctrl
	reg = read_csr(dd, WFR_SEND_CTRL);
	switch (op) {
	case PSC_GLOBAL_ENABLE:
		reg |= WFR_SEND_CTRL_SEND_ENABLE_SMASK;
		break;
	case PSC_GLOBAL_DISABLE:
		reg &= ~WFR_SEND_CTRL_SEND_ENABLE_SMASK;
		break;
	case PSC_GLOBAL_VLARB_ENABLE:
		reg |= WFR_SEND_CTRL_VL_ARBITER_ENABLE_SMASK;
		break;
	case PSC_GLOBAL_VLARB_DISABLE:
		reg &= ~WFR_SEND_CTRL_VL_ARBITER_ENABLE_SMASK;
		break;
	case PSC_CM_RESET:
		__cm_reset(dd, reg);
		return; /* NOTE: return, not break */
	default:
		dd_dev_err(dd, "%s: invalid control %d\n", __func__, op);
		return;
	}
	write_csr(dd, WFR_SEND_CTRL, reg);
}

/* number of send context memory pools */
#define NUM_SC_POOLS 2

/* Send Context Size (SCS) wildcards */
#define SCS_POOL_0 -1
#define SCS_POOL_1 -2
/* Send Context Count (SCC) wildcards */
#define SCC_PER_VL -1
#define SCC_PER_CPU  -2

/* TODO: decide on size and counts */
#define SCC_PER_KRCVQ  -3
#define SCC_ACK_CREDITS  32

#define PIO_WAIT_BATCH_SIZE 5

/* default send context sizes */
static struct sc_config_sizes sc_config_sizes[SC_MAX] = {
	[SC_KERNEL] = { .size  = SCS_POOL_0,	/* even divide, pool 0 */
			.count = SCC_PER_VL },/* one per NUMA */
	[SC_ACK]    = { .size  = SCC_ACK_CREDITS,
			.count = SCC_PER_KRCVQ },
	[SC_USER]   = { .size  = SCS_POOL_0,	/* even divide, pool 0 */
			.count = SCC_PER_CPU },	/* one per CPU */

};

/* send context memory pool configuration */
struct mem_pool_config {
	int centipercent;	/* % of memory, in 100ths of 1% */
	int absolute_blocks;	/* absolut block count */
};

/* default memory pool configuration: 100% in pool 0 */
static struct mem_pool_config sc_mem_pool_config[NUM_SC_POOLS] = {
	/* centi%, abs blocks */
	{  10000,     -1 },		/* pool 0 */
	{      0,     -1 },		/* pool 1 */
};

/* memory pool information, used when calculating final sizes */
struct mem_pool_info {
	int centipercent;	/* 100th of 1% of memory to use, -1 if blocks
				   already set */
	int count;		/* count of contexts in the pool */
	int blocks;		/* block size of the pool */
	int size;		/* context size, in blocks */
};

/*
 * Convert a pool wildcard to a valid pool index.  The wildcards
 * start at -1 and increase negatively.  Map them as:
 *	-1 => 0
 *	-2 => 1
 *	etc.
 *
 * Return -1 on non-wildcard input, otherwise convert to a pool number.
 */
static int wildcard_to_pool(int wc)
{
	if (wc >= 0)
		return -1;	/* non-wildcard */
	return -wc - 1;
}

static const char *sc_type_names[SC_MAX] = {
	"kernel",
	"ack",
	"user"
};

static const char *sc_type_name(int index)
{
	if (index < 0 || index >= SC_MAX)
		return "unknown";
	return sc_type_names[index];
}

/*
 * Read the send context memory pool configuration and send context
 * size configuration.  Replace any wildcards and come up with final
 * counts and sizes for the send context types.
 *
 * TODO: this needs to be much more sophisticated for group allocations.
 */
int init_sc_pools_and_sizes(struct hfi_devdata *dd)
{
	struct mem_pool_info mem_pool_info[NUM_SC_POOLS] = { { 0 } };
	int total_blocks = dd->chip_pio_mem_size / WFR_PIO_BLOCK_SIZE;
	int total_contexts = 0;
	int fixed_blocks;
	int pool_blocks;
	int used_blocks;
	int cp_total;		/* centipercent total */
	int ab_total;		/* absolute block total */
	int extra;
	int i;

	/*
	 * Step 0:
	 *	- copy the centipercents/absolute sizes from the pool config
	 *	- sanity check these values
	 *	- add up centipercents, then later check for full value
	 *	- add up absolute blocks, then later check for overcommit
	 */
	cp_total = 0;
	ab_total = 0;
	for (i = 0; i < NUM_SC_POOLS; i++) {
		int cp = sc_mem_pool_config[i].centipercent;
		int ab = sc_mem_pool_config[i].absolute_blocks;

		/*
		 * A negative value is "unused" or "invalid".  Both *can*
		 * be valid, but centipercent wins, so check that first
		 */
		if (cp >= 0) {			/* centipercent valid */
			cp_total += cp;
		} else if (ab >= 0) {		/* absolute blocks valid */
			ab_total += ab;
		} else {			/* neither valid */
			dd_dev_err(dd, "Send context memory pool %d: both the block count and centipercent are invalid\n", i);
			return -EINVAL;
		}

		mem_pool_info[i].centipercent = cp;
		mem_pool_info[i].blocks = ab;
	}

	/* do not use both % and absolute blocks for different pools */
	if (cp_total != 0 && ab_total != 0) {
		dd_dev_err(dd, "All send context memory pools must be described as either centipercent or blocks, no mixing between pools\n");
		return -EINVAL;
	}

	/* if any percentages are present, they must add up to 100% x 100 */
	if (cp_total != 0 && cp_total != 10000) {
		dd_dev_err(dd, "Send context memory pool centipercent is %d, expecting 10000\n", cp_total);
		return -EINVAL;
	}

	/* the absolute pool total cannot be more than the mem total */
	if (ab_total > total_blocks) {
		dd_dev_err(dd, "Send context memory pool absolute block count %d is larger than the memory size %d\n", ab_total, total_blocks);
		return -EINVAL;
	}

	/*
	 * Step 2:
	 *	- copy from the context size config
	 *	- replace context type wildcard counts with real values
	 *	- add up non-memory pool block sizes
	 *	- add up memory pool user counts
	 */
	fixed_blocks = 0;
	for (i = 0; i < SC_MAX; i++) {
		int count = sc_config_sizes[i].count;
		int size = sc_config_sizes[i].size;
		int pool;

		/*
		 * Sanity check count: Either a positive value or
		 * one of the expected wildcards is valid.  The positive
		 * value is checked later when we compare against total
		 * memory available.
		 */
		if (i == SC_ACK) {
			count = dd->n_krcv_queues;
		} else if (i == SC_KERNEL) {
			count = num_vls + 1 /* VL15 */;
		} else if (count == SCC_PER_CPU) {
			count = num_online_cpus();
		} else if (count < 0) {
			dd_dev_err(dd, "%s send context invalid count wildcard %d\n", sc_type_name(i), count);
			return -EINVAL;
		}
		if (total_contexts + count > dd->chip_send_contexts)
			count = dd->chip_send_contexts - total_contexts;

		total_contexts += count;

		/*
		 * Sanity check pool: The conversion will return a pool
		 * number or -1 if a fixed (non-negative) value.  The fixed
		 * value is is checked later when we compare against
		 * total memory available.
		 */
		pool = wildcard_to_pool(size);
		if (pool == -1) {			/* non-wildcard */
			fixed_blocks += size * count;
		} else if (pool < NUM_SC_POOLS) {	/* valid wildcard */
			mem_pool_info[pool].count += count;
		} else {				/* invalid wildcard */
			dd_dev_err(dd, "%s send context invalid pool wildcard %d\n", sc_type_name(i), size);
			return -EINVAL;
		}

		dd->sc_sizes[i].count = count;
		dd->sc_sizes[i].size = size;
	}
	if (fixed_blocks > total_blocks) {
		dd_dev_err(dd, "Send context fixed block count, %u, larger than total block count %u\n", fixed_blocks, total_blocks);
		return -EINVAL;
	}

	/* step 3: calculate the blocks in the pools, and pool context sizes */
	pool_blocks = total_blocks - fixed_blocks;
	if (ab_total > pool_blocks) {
		dd_dev_err(dd, "Send context fixed pool sizes, %u, larger than pool block count %u\n", ab_total, pool_blocks);
		return -EINVAL;
	}
	/* subtract off the fixed pool blocks */
	pool_blocks -= ab_total;

	for (i = 0; i < NUM_SC_POOLS; i++) {
		struct mem_pool_info *pi = &mem_pool_info[i];

		/* % beats absolute blocks */
		if (pi->centipercent >= 0)
			pi->blocks = (pool_blocks * pi->centipercent) / 10000;

		if (pi->blocks == 0 && pi->count != 0) {
			dd_dev_err(dd, "Send context memory pool %d has %u contexts, but no blocks\n", i, pi->count);
			return -EINVAL;
		}
		if (pi->count == 0) {
			/* warn about wasted blocks */
			if (pi->blocks != 0)
				dd_dev_err(dd, "Send context memory pool %d has %u blocks, but zero contexts\n", i, pi->blocks);
			pi->size = 0;
		} else {
			pi->size = pi->blocks / pi->count;
		}
	}

	/* step 4: fill in the context type sizes from the pool sizes */
	used_blocks = 0;
	for (i = 0; i < SC_MAX; i++) {
		if (dd->sc_sizes[i].size < 0) {
			int pool = wildcard_to_pool(dd->sc_sizes[i].size);

			dd->sc_sizes[i].size = mem_pool_info[pool].size;
		}
		/* make sure we are not larger than what is allowed by the HW */
#define WFR_PIO_MAX_BLOCKS 1024
		if (dd->sc_sizes[i].size > WFR_PIO_MAX_BLOCKS)
			dd->sc_sizes[i].size = WFR_PIO_MAX_BLOCKS;

		/* calculate our total usage */
		used_blocks += dd->sc_sizes[i].size * dd->sc_sizes[i].count;
	}
	extra = total_blocks - used_blocks;
	if (extra != 0)
		dd_dev_info(dd, "unused send context blocks: %d\n", extra);

	return total_contexts;
}

int init_send_contexts(struct hfi_devdata *dd)
{
	u64 reg, all;
	u8 opval, opmask;
	u16 base;
	int ret, i, j, context;


	ret = init_credit_return(dd);
	if (ret)
		return ret;

	dd->send_contexts = kzalloc(sizeof(struct send_context_info) * dd->num_send_contexts, GFP_KERNEL);
	if (!dd->send_contexts) {
		dd_dev_err(dd, "Unable to allocate send context array\n");
		free_credit_return(dd);
		return -ENOMEM;
	}

	/*
	 * For now, all are group 0, so ordering and grouping does not
	 * matter - just allocate them one after another.  If we start
	 * to use grouping, we will want to be more sophisticated.
	 * E.g. If there are extra HW contexts, arrange them in per-NUMA
	 * groups, perhaps per type.
	 */
	context = 0;
	base = 0;
	for (i = 0; i < SC_MAX; i++) {
		struct sc_config_sizes *scs = &dd->sc_sizes[i];

		for (j = 0; j < scs->count; j++) {
			struct send_context_info *sci =
						&dd->send_contexts[context];
			sci->type = i;
			sci->base = base;
			sci->credits = scs->size;

			reg = ((sci->credits & WFR_SEND_CTXT_CTRL_CTXT_DEPTH_MASK) << WFR_SEND_CTXT_CTRL_CTXT_DEPTH_SHIFT)
				| ((sci->base & WFR_SEND_CTXT_CTRL_CTXT_BASE_MASK) << WFR_SEND_CTXT_CTRL_CTXT_BASE_SHIFT);
			write_kctxt_csr(dd, context, WFR_SEND_CTXT_CTRL, reg);

			context++;
			base += scs->size;
		}
	}

	/* set up packet checking */

	/* checks/disallow to set on all context types */
	all = HFI_PKT_BASE_SC_INTEGRITY;

	for (i = 0; i < dd->num_send_contexts; i++) {
		/* per context type checks */
		if (dd->send_contexts[i].type == SC_USER) {
			reg =  all | HFI_PKT_USER_SC_INTEGRITY;
			opval = WFR_USER_OPCODE_CHECK_VAL;
			opmask = WFR_USER_OPCODE_CHECK_MASK;
		} else {
			reg = all | HFI_PKT_KERNEL_SC_INTEGRITY;
			opval = WFR_OPCODE_CHECK_VAL_DISABLED;
			opmask = WFR_OPCODE_CHECK_MASK_DISABLED;
		}

		if (likely(!HFI_CAP_IS_KSET(NO_INTEGRITY)))
			write_kctxt_csr(dd, i, WFR_SEND_CTXT_CHECK_ENABLE, reg);

		/* unmask all errors */
		write_kctxt_csr(dd, i, WFR_SEND_CTXT_ERR_MASK, (u64)-1);

		/* set the default partition key */
		write_kctxt_csr(dd, i, WFR_SEND_CTXT_CHECK_PARTITION_KEY,
			(DEFAULT_PKEY &
				WFR_SEND_CTXT_CHECK_PARTITION_KEY_VALUE_MASK)
			    << WFR_SEND_CTXT_CHECK_PARTITION_KEY_VALUE_SHIFT);

		/* set the send context check opcode mask and value */
		write_kctxt_csr(dd, i, WFR_SEND_CTXT_CHECK_OPCODE,
			((u64)opmask << WFR_SEND_CTXT_CHECK_OPCODE_MASK_SHIFT) |
			((u64)opval << WFR_SEND_CTXT_CHECK_OPCODE_VALUE_SHIFT));
	}

	return 0;
}

/*
 * Use compare and exchange to locklessly allocate and release HW
 * context entries.
 */
#define reserve_hw_context(ap) (cmpxchg(ap, 0, 1) == 0)
#define release_hw_context(ap) (cmpxchg(ap, 1, 0) == 1)

/* allocate a hardware context of the given type */
static int sc_hw_alloc(struct hfi_devdata *dd, int type, u32 *contextp)
{
	struct send_context_info *sci;
	u32 context;

	for (context = 0, sci = &dd->send_contexts[0];
			context < dd->num_send_contexts; context++, sci++) {
		if (sci->type == type && reserve_hw_context(&sci->allocated)) {
			/* allocated */
			*contextp = context;
			return 0; /* success */
		}
	}
	dd_dev_err(dd, "Unable to locate a free HW send context\n");

	return -ENOSPC;
}

/* free the given hardware context */
static void sc_hw_free(struct hfi_devdata *dd, u32 context)
{
	struct send_context_info *sci = &dd->send_contexts[context];

	if (release_hw_context(&sci->allocated))
		return;	/* success */

	dd_dev_err(dd, "%s: context %u not allocated?\n", __func__, context);
}

/* return the base context of a context in a group */
static inline u32 group_context(u32 context, u32 group)
{
	return (context >> group) << group;
}

/* return the size of a group */
static inline u32 group_size(u32 group)
{
	return 1 << group;
}

/* obtain the credit return addresses, kernel virtual and physical for sc */
static void cr_group_addresses(struct send_context *sc, dma_addr_t *pa)
{
	u32 gc = group_context(sc->context, sc->group);

	sc->hw_free = &sc->dd->cr_base[sc->node].va[gc].cr[sc->context & 0x7];
	*pa = (unsigned long)
	       &((struct credit_return *)
	       sc->dd->cr_base[sc->node].pa)[gc].cr[sc->context & 0x7];
}

/*
 * Work queue function triggered in error interrupt routine for
 * kernel contexts.
 */
static void sc_halted(struct work_struct *work)
{
	struct send_context *sc;

	sc = container_of(work, struct send_context, halt_work);
	sc_restart(sc);
	/* idea: add a timed retry if the restart fails */
}

/*
 * Allocate a NUMA relative send context structure of the given type along
 * with a HW context.
 */
struct send_context *sc_alloc(struct hfi_devdata *dd, int type, int numa)
{
	struct send_context_info *sci;
	struct send_context *sc;
	dma_addr_t pa;
	u64 reg;
	u32 release_credits, thresh;
	u32 context;
	int ret;

	/* do not allocate while frozen */
	if (dd->flags & HFI_FROZEN)
		return NULL;

	sc = kzalloc_node(sizeof(struct send_context), GFP_KERNEL, numa);
	if (!sc) {
		dd_dev_err(dd, "Cannot allocate send context structure\n");
		return NULL;
	}

	ret = sc_hw_alloc(dd, type, &context);
	if (ret)
		goto no_context;

	sci = &dd->send_contexts[context];
	sci->sc = sc;

	sc->dd = dd;
	sc->node = numa;
	sc->type = type;
	spin_lock_init(&sc->alloc_lock);
	spin_lock_init(&sc->release_lock);
	spin_lock_init(&sc->credit_ctrl_lock);
	INIT_LIST_HEAD(&sc->piowait);
	INIT_WORK(&sc->halt_work, sc_halted);
	atomic_set(&sc->buffers_allocated, 0);
	init_waitqueue_head(&sc->halt_wait);

	/* TBD: group set-up.  Make it always 0 for now. */
	sc->group = 0;

	sc->context = context;
	cr_group_addresses(sc, &pa);
	sc->credits = sci->credits;

/* PIO Send Memory Address details */
#define WFR_PIO_ADDR_CONTEXT_MASK 0xfful
#define WFR_PIO_ADDR_CONTEXT_SHIFT 16
	sc->base_addr = dd->piobase + ((context & WFR_PIO_ADDR_CONTEXT_MASK)
					<< WFR_PIO_ADDR_CONTEXT_SHIFT);

	/*
	 * User contexts do not get a shadow ring.  All is
	 * handled in user space.
	 */
	if (type != SC_USER) {
		/*
		 * Size the shadow ring 1 larger so head == tail can mean
		 * empty.
		 */
		sc->sr_size = sci->credits + 1;
		sc->sr = kzalloc_node(sizeof(union pio_shadow_ring) *
				sc->sr_size, GFP_KERNEL, numa);
		if (!sc->sr) {
			dd_dev_err(dd, "Cannot allocate send context shadow ring structure\n");
			goto no_shadow;
		}
	}

	/* set up credit return */
	reg = pa & WFR_SEND_CTXT_CREDIT_RETURN_ADDR_ADDRESS_SMASK;
	write_kctxt_csr(dd, context, WFR_SEND_CTXT_CREDIT_RETURN_ADDR, reg);

	/*
	 * Set up a threshold credit return when one MTU of space
	 * remains.  At the moment all VLs have the same MTU.
	 * That will change.
	 *
	 * TODO: We may want a "credit return MTU factor" to change
	 * the threshold in terms of 10ths of an MTU.  E.g.
	release_credits = DIV_ROUND_UP((dd->pport[0].ibmtu * cr_mtu_factor)
				+ (dd->rcvhdrentsize<<2),
				10 * WFR_PIO_BLOCK_SIZE);
	 * PROBLEM: ibmtu is not yet set up when the kernel send
	 * contexts are created.
	 */
	release_credits = DIV_ROUND_UP(
			(type == SC_ACK ?
				(SCC_ACK_CREDITS * WFR_PIO_BLOCK_SIZE)/2 :
				enum_to_mtu(STL_MTU_10240)) +
			(dd->rcvhdrentsize<<2), WFR_PIO_BLOCK_SIZE);
	if (sc->credits <= release_credits)
		thresh = 1;
	else
		thresh = sc->credits - release_credits;
	reg = thresh << WFR_SEND_CTXT_CREDIT_CTRL_THRESHOLD_SHIFT;
	/* set up write-through credit_ctrl */
	sc->credit_ctrl = reg;
	write_kctxt_csr(dd, context, WFR_SEND_CTXT_CREDIT_CTRL, reg);

	dd_dev_info(dd,
		"Send context %u %s group %u credits %u credit_ctrl 0x%llx threshold %u\n",
		context,
		sc_type_name(type),
		sc->group,
		sc->credits,
		sc->credit_ctrl,
		thresh);

	return sc;

no_shadow:
	sci->sc = NULL;
	sc_hw_free(dd, context);
no_context:
	kfree(sc);
	return NULL;
}

/* free a per-NUMA send context structure */
void sc_free(struct send_context *sc)
{
	struct hfi_devdata *dd;
	u32 context;

	if (!sc)
		return;

	sc->flags |= SCF_IN_FREE;	/* ensure no restarts */
	dd = sc->dd;
	if (!list_empty(&sc->piowait))
		dd_dev_err(dd, "piowait list not empty!\n");
	context = sc->context;
	sc_disable(sc);	/* make sure the HW is disabled */
	flush_work(&sc->halt_work);
	dd->send_contexts[context].sc = NULL;
	sc_hw_free(dd, context);

	kfree(sc->sr);
	kfree(sc);
}

/* disable the context */
void sc_disable(struct send_context *sc)
{
	u64 reg;
	unsigned long flags;
	struct pio_buf *pbuf;

	if (!sc)
		return;

	/* do all steps, even if already disabled */
	reg = read_kctxt_csr(sc->dd, sc->context, WFR_SEND_CTXT_CTRL);
	reg &= ~WFR_SEND_CTXT_CTRL_CTXT_ENABLE_SMASK;
	spin_lock_irqsave(&sc->alloc_lock, flags);
	sc->flags &= ~SCF_ENABLED;
	sc_wait_for_packet_egress(sc);
	write_kctxt_csr(sc->dd, sc->context, WFR_SEND_CTXT_CTRL, reg);
	spin_unlock_irqrestore(&sc->alloc_lock, flags);

	/*
	 * Flush any waiters.  Once the context is disabled,
	 * credit return interrupts are stopped (although there
	 * could be one in-process when the context is disabled).
	 * Wait one microsecond for any lingering interrupts, then
	 * proceed with the flush.
	 */
	udelay(1);
	spin_lock_irqsave(&sc->release_lock, flags);
	if (sc->sr) {	/* this context has a shadow ring */
		while (sc->sr_tail != sc->sr_head) {
			pbuf = &sc->sr[sc->sr_tail].pbuf;
			if (pbuf->cb)
				(*pbuf->cb)(pbuf->arg, PRC_SC_DISABLE);
			sc->sr_tail++;
			if (sc->sr_tail >= sc->sr_size)
				sc->sr_tail = 0;
		}
	}
	spin_unlock_irqrestore(&sc->release_lock, flags);
}

/* return SendEgressCtxtStatus.PacketOccupancy */
#define packet_occupancy(r) \
	(((r) & WFR_SEND_EGRESS_CTXT_STATUS_CTXT_EGRESS_PACKET_OCCUPANCY_SMASK)\
	>> WFR_SEND_EGRESS_CTXT_STATUS_CTXT_EGRESS_PACKET_OCCUPANCY_SHIFT)

/* wait for packet egress */
static void sc_wait_for_packet_egress(struct send_context *sc)
{
	struct hfi_devdata *dd = sc->dd;
	u64 reg;
	u32 loop = 0;

	while (1) {
		reg = read_csr(dd, sc->context * 8 +
			       WFR_SEND_EGRESS_CTXT_STATUS);
		reg = packet_occupancy(reg);
		if (reg == 0)
			break;
		if (loop > 100) {
			dd_dev_err(dd,
				"%s: context %u timeout waiting for packets to egress, remaining count %u\n",
				__func__, sc->context, (u32)reg);
			break;
		}
		loop++;
		udelay(1);
	}
	/*
	 * Add additional delay (at least, 1us) to ensure chip returns all
	 * credits
	 */
	/* TODO: The cclock_to_ns conversion only makes sense on FPGA since
	 * 350cclock on ASIC is less than 1us. */
	loop = cclock_to_ns(dd, SC_CTXT_PACKET_EGRESS_TIMEOUT) / 1000;
	udelay(loop ? loop : 1);
}

/*
 * Restart a context after it has been halted due to error.
 *
 * This follows the ordering given in the HAS. If the first step fails
 * - wait for the halt to be asserted, return early.  Otherwise complain
 * about timeouts but keep going.
 *
 * It is expected that allocations (enabled flag bit) have been shut off
 * already (only applies to kernel contexts).
 */
int sc_restart(struct send_context *sc)
{
	struct hfi_devdata *dd = sc->dd;
	u64 reg;
	u32 loop;
	int count;

	/* bounce off if not halted, or being free'd */
	if (!(sc->flags & SCF_HALTED) || (sc->flags & SCF_IN_FREE))
		return -EINVAL;

	dd_dev_info(dd, "restarting context %u\n", sc->context);

	/*
	 * Step 1: Wait for the context to actually halt.
	 *
	 * The error interrupt is asynchronous of actually setting halt
	 * on the context.
	 */
	loop = 0;
	while (1) {
		reg = read_kctxt_csr(dd, sc->context, WFR_SEND_CTXT_STATUS);
		if (reg & WFR_SEND_CTXT_STATUS_CTXT_HALTED_SMASK)
			break;
		if (loop > 100) {
			dd_dev_err(dd, "%s: context %d not halting, skipping\n",
				__func__, sc->context);
			return -ETIME;
		}
		loop++;
		udelay(1);
	}

	/*
	 * Step 2: Ensure no users are still trying to write to PIO.
	 *
	 * For kernel contexts, we have already turned off buffer allocation.
	 * Now wait for the buffer count to go to zero.
	 *
	 * For user contexts, the user handling code has cut off write access
	 * to the context's PIO pages before calling this routine and will
	 * restore write access after this routine returns.
	 */
	if (sc->type != SC_USER) {
		/* kernel context */
		loop = 0;
		while (1) {
			count = atomic_read(&sc->buffers_allocated);
			if (count == 0)
				break;
			if (loop > 100) {
				dd_dev_err(dd,
					"%s: context %u timeout waiting for PIO buffers to zero, remaining %d\n",
					__func__, sc->context, count);
			}
			loop++;
			udelay(1);
		}
	}

	/*
	 * Step 3: Wait for all packets to egress.
	 * This is done while disabling the send context
	 *
	 * Step 4: Disable the context
	 *
	 * This is a superset of the halt.  After the disable, the
	 * errors can be cleared.
	 */
	sc_disable(sc);

	/*
	 * Step 5: Enable the context
	 *
	 * This enable will clear the halted flag and per-send context
	 * error flags.
	 */
	return sc_enable(sc);
}

/*
 * PIO freeze processing.  To be called after the TXE block is fully frozen.
 * Go through all frozen send contexts and disable them.  The contexts are
 * already stopped by the freeze.
 */
void pio_freeze(struct hfi_devdata *dd)
{
	struct send_context *sc;
	int i;

	for (i = 0; i < dd->num_send_contexts; i++) {
		sc = dd->send_contexts[i].sc;
		if (!sc || !(sc->flags & SCF_FROZEN))
			continue;

		/* only need to disable, the context is already stopped */
		sc_disable(sc);
	}
}

/*
 * Unfreeze PIO for kernel send contexts.  The precondtion for calling this
 * is that all PIO send contexts have been disabled and the SPC freeze has
 * been cleared.  Now perform the last step and re-enable each kernel conext.
 * User (PSM) processing will occur when PSM calls into the kernel to
 * acknowledge the freeze.
 */
void pio_kernel_unfreeze(struct hfi_devdata *dd)
{
	struct send_context *sc;
	int i;

	for (i = 0; i < dd->num_send_contexts; i++) {
		sc = dd->send_contexts[i].sc;
		if (!sc || !(sc->flags & SCF_FROZEN) || sc->type == SC_USER)
			continue;

		sc_enable(sc);	/* will clear the sc frozen flag */
	}
}

/*
 * Wait for the SendPioInitCtxt.PioInitInProgress bit to clear.
 * Returns:
 *	-ETIMEDOUT - if we wait too long
 *	-EIO	   - if there was an error
 */
static int pio_init_wait_progress(struct hfi_devdata *dd)
{
	u64 reg;
	int count = 0;

	while (1) {
		reg = read_csr(dd, WFR_SEND_PIO_INIT_CTXT);
		if (!(reg & WFR_SEND_PIO_INIT_CTXT_PIO_INIT_IN_PROGRESS_SMASK))
			break;
		mdelay(20);
		if (count++ > 10)
			return -ETIMEDOUT;
	}

	return reg & WFR_SEND_PIO_INIT_CTXT_PIO_INIT_ERR_SMASK ? -EIO : 0;
}

/*
 * Reset all of the send contexts to their power-on state.  Used
 * only during manual init - no lock against sc_enable needed.
 */
void pio_reset_all(struct hfi_devdata *dd)
{
	int ret;

	/* make sure the init engine is not busy */
	ret = pio_init_wait_progress(dd);
	if (ret == -ETIMEDOUT) {
		dd_dev_err(dd,
			"PIO send context init is stuck, not initializing PIO blocks\n");
		return;
	}

	if (ret == -EIO) {
		/* clear the error */
		write_csr(dd, WFR_SEND_PIO_ERR_CLEAR,
			WFR_SEND_PIO_ERR_CLEAR_PIO_INIT_SM_IN_ERR_SMASK);
	}

	/* reset init all */
	write_csr(dd, WFR_SEND_PIO_INIT_CTXT,
			WFR_SEND_PIO_INIT_CTXT_PIO_ALL_CTXT_INIT_SMASK);
	udelay(2);
	ret = pio_init_wait_progress(dd);
	if (ret < 0) {
		dd_dev_err(dd,
			"PIO send context init %s while initializing all PIO blocks\n",
			ret == -ETIMEDOUT ? "is stuck" : "had an error");
	}
}

/* enable the context */
int sc_enable(struct send_context *sc)
{
	u64 sc_ctrl, reg, pio;
	struct hfi_devdata *dd;
	unsigned long flags;
	int ret;

	if (!sc)
		return -EINVAL;
	dd = sc->dd;

	sc_ctrl = read_kctxt_csr(dd, sc->context, WFR_SEND_CTXT_CTRL);
	if ((sc_ctrl & WFR_SEND_CTXT_CTRL_CTXT_ENABLE_SMASK))
		return 0; /* already enabled */

	/* IMPORTANT: only clear free and fill if transitioning 0 -> 1 */

	// FIXME: obtain the locks?
	*sc->hw_free = 0;
	sc->free = 0;
	sc->alloc_free = 0;
	sc->fill = 0;
	sc->sr_head = 0;
	sc->sr_tail = 0;
	sc->flags = 0;
	atomic_set(&sc->buffers_allocated, 0);

	/*
	 * Clear all per-context errors.  Some of these will be set when
	 * we are re-enabling after a context halt.  Now that the context
	 * is disabled, the halt will not clear until after the PIO init
	 * engine runs below.
	 */
	reg = read_kctxt_csr(dd, sc->context, WFR_SEND_CTXT_ERR_STATUS);
	if (reg)
		write_kctxt_csr(dd, sc->context, WFR_SEND_CTXT_ERR_CLEAR, reg);

	/*
	 * The HW PIO initialization engine can handle only one init
	 * request at a time. Serialize access to each device's engine.
	 */
	spin_lock_irqsave(&dd->sc_init_lock, flags);
	/*
	 * Since access to this code block is serialized and
	 * each access waits for the initialization to complete
	 * before releasing the lock, the PIO initialization engine
	 * should not be in use, so we don't have to wait for the
	 * InProgress bit to go down.
	 */
	pio = ((sc->context & WFR_SEND_PIO_INIT_CTXT_PIO_CTXT_NUM_MASK) <<
	       WFR_SEND_PIO_INIT_CTXT_PIO_CTXT_NUM_SHIFT) |
		WFR_SEND_PIO_INIT_CTXT_PIO_SINGLE_CTXT_INIT_SMASK;
	write_csr(dd, WFR_SEND_PIO_INIT_CTXT, pio);
	/*
	 * Wait until the engine is done.  Give the chip the required time
	 * so, hopefully, we read the register just once.
	 */
	udelay(2);
	ret = pio_init_wait_progress(dd);
	spin_unlock_irqrestore(&dd->sc_init_lock, flags);
	if (ret) {
		dd_dev_err(dd,
			   "ctxt%u: Context not enabled due to init failure %d\n",
			   sc->context, ret);
		return ret;
	}

	/*
	 * All is well. Enable the context.
	 * Obtain the allocator lock to guard against any allocation
	 * attempts (which should not happen prior to context being
	 * enabled). On the release/disable side we don't need to
	 * worry about locking since the releaser will not do anything
	 * if the context accounting values have not changed.
	 */
	sc_ctrl |= WFR_SEND_CTXT_CTRL_CTXT_ENABLE_SMASK;
	spin_lock_irqsave(&sc->alloc_lock, flags);
	write_kctxt_csr(dd, sc->context, WFR_SEND_CTXT_CTRL, sc_ctrl);
	/*
	 * Read SendCtxtCtrl to force the write out and prevent a timing
	 * hazard where a PIO write may reach the context before the enable.
	 */
	read_kctxt_csr(dd, sc->context, WFR_SEND_CTXT_CTRL);
	sc->flags |= SCF_ENABLED;
	spin_unlock_irqrestore(&sc->alloc_lock, flags);

	return 0;
}

/* force a credit return on the context */
void sc_return_credits(struct send_context *sc)
{
	if (!sc)
		return;

	/* a 0->1 transition schedules a credit return */
	write_kctxt_csr(sc->dd, sc->context, WFR_SEND_CTXT_CREDIT_FORCE,
		WFR_SEND_CTXT_CREDIT_FORCE_FORCE_RETURN_SMASK);
	/*
	 * Ensure that the write is flushed and the credit return is
	 * schedule. We care more about the 0 -> 1 transition.
	 */
	read_kctxt_csr(sc->dd, sc->context, WFR_SEND_CTXT_CREDIT_FORCE);
	/* set back to 0 for next time */
	write_kctxt_csr(sc->dd, sc->context, WFR_SEND_CTXT_CREDIT_FORCE, 0);
}

/* allow all in-flight packets to drain on the context */
void sc_flush(struct send_context *sc)
{
	if (!sc)
		return;

	dd_dev_info(sc->dd, "%s: context %d - not implemented\n",
			__func__, sc->context);
}

/* drop all packets on the context, no waiting until they are sent */
void sc_drop(struct send_context *sc)
{
	if (!sc)
		return;

	dd_dev_info(sc->dd, "%s: context %d - not implemented\n",
			__func__, sc->context);
}

/*
 * Start the software reaction to a context halt or SPC freeze:
 *	- mark the context as halted or frozen
 *	- stop buffer allocations
 *
 * Called from the error interrupt.  Other work is deferred until
 * out of the interrupt.
 */
void sc_stop(struct send_context *sc, int flag)
{
	unsigned long flags;

	/* mark the context */
	sc->flags |= flag;

	/* stop buffer allocations */
	spin_lock_irqsave(&sc->alloc_lock, flags);
	sc->flags &= ~SCF_ENABLED;
	spin_unlock_irqrestore(&sc->alloc_lock, flags);
	wake_up(&sc->halt_wait);
}

#define BLOCK_DWORDS (WFR_PIO_BLOCK_SIZE/sizeof(u32))
#define dwords_to_blocks(x) DIV_ROUND_UP(x,BLOCK_DWORDS)

/*
 * The send context buffer "allocator".
 *
 * @sc: the PIO send context we are allocating from
 * @len: length of whole packet - including PBC - in dwords
 * @cb: optional callback to call when the buffer is finished sending
 * @arg: argument for cb
 *
 * Return a pointer to a PIO buffer  if successful, NULL if not enough room.
 */
struct pio_buf *sc_buffer_alloc(struct send_context *sc, u32 dw_len,
				pio_release_cb cb, void *arg)
{
	struct pio_buf *pbuf = NULL;
	unsigned long flags;
	unsigned long avail;
	unsigned long blocks = dwords_to_blocks(dw_len);
	unsigned long start_fill;
	int trycount = 0;
	u32 head, next;

	spin_lock_irqsave(&sc->alloc_lock, flags);
	if (!(sc->flags & SCF_ENABLED)) {
		spin_unlock_irqrestore(&sc->alloc_lock, flags);
		goto done;
	}

retry:
	avail = (unsigned long)sc->credits - (sc->fill - sc->alloc_free);
	if (blocks > avail) {
		/* not enough room */
		if (unlikely(trycount))	{ /* already tried to get more room */
			spin_unlock_irqrestore(&sc->alloc_lock, flags);
			goto done;
		}
		/* copy from receiver cache line and recalculate */
		sc->alloc_free = ACCESS_ONCE(sc->free);
		avail = (unsigned long)sc->credits - (sc->fill - sc->alloc_free);
		if (blocks > avail) {
			/* still no room, actively update */
			spin_unlock_irqrestore(&sc->alloc_lock, flags);
			sc_release_update(sc);
			spin_lock_irqsave(&sc->alloc_lock, flags);
			sc->alloc_free = ACCESS_ONCE(sc->free);
			trycount++;
			goto retry;
		}
	}

	/* there is enough room */

	atomic_inc(&sc->buffers_allocated);

	/* read this once */
	head = sc->sr_head;

	/* "allocate" the buffer */
	start_fill = sc->fill;
	sc->fill += blocks;

	/*
	 * Fill the parts that the releaser looks at before moving the head.
	 * The only necessary piece is the sent_at field.  The credits
	 * we have just allocated cannot have been returned yet, so the
	 * cb and arg will not be looked at for a "while".  Put them
	 * on this side of the memory barrier anyway.
	 */
//FIXME: I have gotten myself confused again.  Why do I need the mb?
// If this is a cache coherent system these writes should be visible
// once I write them. The only issue is to prevent the compiler from
// moving the writes later than the head update.
	pbuf = &sc->sr[head].pbuf;
	pbuf->sent_at = sc->fill;
	pbuf->cb = cb;
	pbuf->arg = arg;
	pbuf->sc = sc;	/* could be filled in at sc->sr init time */
	/* make sure this is in memory before updating the head */
	//FIXME: is this the right barrier call?
	mb();

	/* calculate next head index, do not store */
	next = head + 1;
	if (next >= sc->sr_size)
		next = 0;
	/* update the head - must be last! - the releaser can look at fields
	   in pbuf once we move the head */
	sc->sr_head = next;
	spin_unlock_irqrestore(&sc->alloc_lock, flags);

	/* finish filling in the buffer outside the lock */
	pbuf->start = sc->base_addr + ((start_fill % sc->credits)
							* WFR_PIO_BLOCK_SIZE);
	pbuf->size = sc->credits * WFR_PIO_BLOCK_SIZE;
	pbuf->end = sc->base_addr + pbuf->size;
	pbuf->block_count = blocks;
	pbuf->qw_written = 0;
	pbuf->carry_bytes = 0;
	pbuf->carry.val64 = 0;
done:
	return pbuf;
}

/*
 * There are at least two entities that can turn on credit return
 * interrupts and they can overlap.  Avoid problems by implementing
 * a count scheme that is enforced by a lock.  The lock is needed because
 * the count and CSR write must be paired.
 */

/*
 * Start credit return interrupts.  This is managed by a count.  If already
 * on, just increment the count.
 */
void sc_add_credit_return_intr(struct send_context *sc)
{
	unsigned long flags;

	/* lock must surround both the count change and the CSR update */
	spin_lock_irqsave(&sc->credit_ctrl_lock, flags);
	if (sc->credit_intr_count == 0) {
		sc->credit_ctrl |= WFR_SEND_CTXT_CREDIT_CTRL_CREDIT_INTR_SMASK;
		write_kctxt_csr(sc->dd, sc->context, WFR_SEND_CTXT_CREDIT_CTRL,
			sc->credit_ctrl);
	}
	sc->credit_intr_count++;
	spin_unlock_irqrestore(&sc->credit_ctrl_lock, flags);
}

/*
 * Stop credit return interrupts.  This is managed by a count.  Decrement the
 * count, if the last user, then turn the credit interrupts off.
 */
void sc_del_credit_return_intr(struct send_context *sc)
{
	unsigned long flags;

	BUG_ON(sc->credit_intr_count == 0);

	/* lock must surround both the count change and the CSR update */
	spin_lock_irqsave(&sc->credit_ctrl_lock, flags);
	sc->credit_intr_count--;
	if (sc->credit_intr_count == 0) {
		sc->credit_ctrl &= ~WFR_SEND_CTXT_CREDIT_CTRL_CREDIT_INTR_SMASK;
		write_kctxt_csr(sc->dd, sc->context, WFR_SEND_CTXT_CREDIT_CTRL,
			sc->credit_ctrl);
	}
	spin_unlock_irqrestore(&sc->credit_ctrl_lock, flags);
}

/*
 * The caller must be careful when calling this.  All needint calls
 * must be paried with !needint.
 */
void sc_wantpiobuf_intr(struct send_context *sc, u32 needint)
{
	if (needint)
		sc_add_credit_return_intr(sc);
	else
		sc_del_credit_return_intr(sc);
	trace_hfi_wantpiointr(sc, needint, sc->credit_ctrl);
	if (needint) {
		mmiowb();
		sc_return_credits(sc);
	}
}

/**
 * sc_piobufavail - callback when a PIO buffer is available
 * @sc: the send context
 *
 * This is called from the interrupt handler when a PIO buffer is
 * available after qib_verbs_send() returned an error that no buffers were
 * available. Disable the interrupt if there are no more QPs waiting.
 */
static void sc_piobufavail(struct send_context *sc)
{
	struct hfi_devdata *dd = sc->dd;
	struct qib_ibdev *dev = &dd->verbs_dev;
	struct list_head *list;
	struct qib_qp *qps[PIO_WAIT_BATCH_SIZE];
	struct qib_qp *qp;
	unsigned long flags;
	unsigned i, n = 0;

	if (dd->send_contexts[sc->context].type != SC_KERNEL)
		return;
	list = &sc->piowait;
	/*
	 * Note: checking that the piowait list is empty and clearing
	 * the buffer available interrupt needs to be atomic or we
	 * could end up with QPs on the wait list with the interrupt
	 * disabled.
	 */
	spin_lock_irqsave(&dev->pending_lock, flags);
	while (!list_empty(list)) {
		struct iowait *wait;

		if (n == ARRAY_SIZE(qps))
			goto full;
		wait = list_first_entry(list, struct iowait, list);
		qp = container_of(wait, struct qib_qp, s_iowait);
		list_del_init(&qp->s_iowait.list);
		/* refcount held until actual wakeup */
		qps[n++] = qp;
	}
	/*
	 * Counting: only call wantpiobuf_intr() if there were waiters and they
	 * are now all gone.
	 */
	if (n)
		dd->f_wantpiobuf_intr(sc, 0);
full:
	spin_unlock_irqrestore(&dev->pending_lock, flags);

	for (i = 0; i < n; i++)
		qib_qp_wakeup(qps[i], QIB_S_WAIT_PIO);
}

/* translate a send credit update to a bit code of reasons */
static inline int fill_code(u64 hw_free)
{
	int code = 0;

	if (hw_free & WFR_CR_STATUS_SMASK)
		code |= PRC_STATUS_ERR;
	if (hw_free & WFR_CR_CREDIT_RETURN_DUE_TO_PBC_SMASK)
		code |= PRC_PBC;
	if (hw_free & WFR_CR_CREDIT_RETURN_DUE_TO_THRESHOLD_SMASK)
		code |= PRC_THRESHOLD;
	if (hw_free & WFR_CR_CREDIT_RETURN_DUE_TO_ERR_SMASK)
		code |= PRC_FILL_ERR;
	if (hw_free & WFR_CR_CREDIT_RETURN_DUE_TO_FORCE_SMASK)
		code |= PRC_SC_DISABLE;
	return code;
}

/* use the jiffies compare to get the wrap right */
#define sent_before(a, b) time_before(a, b)	/* a < b */

/*
 * The send context buffer "releaser".
 */
void sc_release_update(struct send_context *sc)
{
	struct pio_buf *pbuf;
	u64 hw_free;
	u32 head, tail;
	unsigned long old_free;
	unsigned long extra;
	unsigned long flags;
	int code;

	if (!sc)
		return;

	spin_lock_irqsave(&sc->release_lock, flags);
	/* update free */
	hw_free = *sc->hw_free;				/* volatile read */
	old_free = sc->free;
	extra = (((hw_free & WFR_CR_COUNTER_SMASK) >> WFR_CR_COUNTER_SHIFT)
			- (old_free & WFR_CR_COUNTER_MASK))
				& WFR_CR_COUNTER_MASK;
	sc->free = old_free + extra;
	trace_hfi_piofree(sc, extra);

	/* call sent buffer callbacks */
	code = -1;				/* code not yet set */
	head = ACCESS_ONCE(sc->sr_head);	/* snapshot the head */
	tail = sc->sr_tail;
	while (head != tail) {
		pbuf = &sc->sr[tail].pbuf;

		if (sent_before(sc->free, pbuf->sent_at)) {
			/* not sent yet */
			break;
		}
		if (pbuf->cb) {
			if (code < 0) /* fill in code on first user */
				code = fill_code(hw_free);
			(*pbuf->cb)(pbuf->arg, code);
		}

		tail++;
		if (tail >= sc->sr_size)
			tail = 0;
	}
	/* update tail, in case we moved it */
	sc->sr_tail = tail;
	spin_unlock_irqrestore(&sc->release_lock, flags);
	sc_piobufavail(sc);
}

/*
 * Send context group releaser.  This is only to be called from
 * the interrupt routine as we only receive one interrupt for the
 * group.  Call release on all contexts in the group.
 */
void sc_group_release_update(struct send_context *sc)
{
	u32 gc, gc_end;

	if (!sc)
		return;

	gc = group_context(sc->context, sc->group);
	gc_end = gc + group_size(sc->group);
	for (; gc < gc_end; gc++)
		sc_release_update(sc->dd->send_contexts[gc].sc);
}

int init_pervl_scs(struct hfi_devdata *dd)
{
	int i;
	u64 mask, all_vl_mask = (u64) 0x80ff; /* VLs 0-7, 15 */
	u32 ctxt;

	dd->vld[15].sc = sc_alloc(dd, SC_KERNEL, dd->node);
	if (!dd->vld[15].sc)
		goto nomem;
	for (i = 0; i < num_vls; i++) {
		dd->vld[i].sc = sc_alloc(dd, SC_KERNEL, dd->node);
		if (!dd->vld[i].sc)
			goto nomem;
	}
	sc_enable(dd->vld[15].sc);
	ctxt = dd->vld[15].sc->context;
	mask = all_vl_mask & ~(1LL << 15);
	write_kctxt_csr(dd, ctxt, WFR_SEND_CTXT_CHECK_VL, mask);
	dd_dev_info(dd,
		    "Using send context %d for VL15\n",
		    dd->vld[15].sc->context);
	for (i = 0; i < num_vls; i++) {
		sc_enable(dd->vld[i].sc);
		ctxt = dd->vld[i].sc->context;
		mask = all_vl_mask & ~(1LL << i);
		write_kctxt_csr(dd, ctxt, WFR_SEND_CTXT_CHECK_VL, mask);
	}
	/* default to vl0 send context for others
	for (; i < 15; i++)
	dd->pervl_scs[i] = dd->pervl_scs[0];*/
	return 0;
nomem:
	sc_free(dd->vld[15].sc);
	for (i = 0; i < num_vls; i++)
		sc_free(dd->vld[i].sc);
	return -ENOMEM;
}

int init_credit_return(struct hfi_devdata *dd)
{
	int ret;
	int num_numa;
	int i;

	num_numa = num_online_nodes();
	/* enforce the expectation that the numas are compact */
	for (i = 0; i < num_numa; i++) {
		if (!node_online(i)) {
			dd_dev_err(dd, "NUMA nodes are not compact\n");
			ret = -EINVAL;
			goto done;
		}
	}

	dd->cr_base = kzalloc(sizeof(struct credit_return_base) * num_numa, GFP_KERNEL);
	if (!dd->cr_base) {
		dd_dev_err(dd, "Unable to allocate credit return base\n");
		ret = -ENOMEM;
		goto done;
	}
	for (i = 0; i < num_numa; i++) {
		int bytes = dd->num_send_contexts*sizeof(struct credit_return);

		set_dev_node(&dd->pcidev->dev, i);
		dd->cr_base[i].va = dma_alloc_coherent(
					&dd->pcidev->dev,
					bytes,
					&dd->cr_base[i].pa,
					GFP_KERNEL);
		if (dd->cr_base[i].va == NULL) {
			set_dev_node(&dd->pcidev->dev, dd->node);
			dd_dev_err(dd, "Unable to allocate credit return "
				"DMA range for NUMA %d\n", i);
			ret = -ENOMEM;
			goto done;
		}
		memset(dd->cr_base[i].va, 0, bytes);
	}
	set_dev_node(&dd->pcidev->dev, dd->node);

	ret = 0;
done:
	return ret;
}

void free_credit_return(struct hfi_devdata *dd)
{
	int num_numa;
	int i;

	if (!dd->cr_base)
		return;

	/* TODO: save this value on allocate in case the number changes? */
	num_numa = num_online_nodes();
	for (i = 0; i < num_numa; i++) {
		if (dd->cr_base[i].va) {
			dma_free_coherent(&dd->pcidev->dev,
				dd->num_send_contexts
					* sizeof(struct credit_return),
				dd->cr_base[i].va,
				dd->cr_base[i].pa);
		}
	}
	kfree(dd->cr_base);
	dd->cr_base = NULL;
}
