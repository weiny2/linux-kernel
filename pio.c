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
 */

#include "hfi.h"
#include <linux/delay.h>

/*
 * Send Context functions
 */

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
#define SCC_PER_NUMA -1
#define SCC_PER_CPU  -2

/* default send context sizes */
static struct sc_config_sizes sc_config_sizes[SC_MAX] = {
	[SC_KERNEL] = { .size  = SCS_POOL_0,	/* even divide, pool 0 */
			.count = SCC_PER_NUMA },/* one per NUMA */
	/* TODO: decide on size and counts */
	[SC_ACK   ] = { .size  = 0,
			.count = 0 },
	[SC_USER  ] = { .size  = SCS_POOL_0,	/* even divide, pool 0 */
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
	int total_blocks = dd->chip_pio_mem_size;
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
		if (count == SCC_PER_NUMA) {
			count = num_online_nodes();
		} else if (count == SCC_PER_CPU) {
			count = num_online_cpus();
		} else if (count < 0) {
			dd_dev_err(dd, "%s send context invalid count wildcard %d\n", sc_type_name(i), count);
			return -EINVAL;
		}
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
	u32 release_credits, thresh;
	u16 base;
	int ret, i, j, context;

	spin_lock_init(&dd->sc_init_lock);

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
	all = WFR_SEND_CTXT_CHECK_ENABLE_DISALLOW_BYPASS_BAD_PKT_LEN_SMASK
		| WFR_SEND_CTXT_CHECK_ENABLE_DISALLOW_PBC_STATIC_RATE_CONTROL_SMASK
		| WFR_SEND_CTXT_CHECK_ENABLE_DISALLOW_TOO_LONG_BYPASS_PACKETS_SMASK
		| WFR_SEND_CTXT_CHECK_ENABLE_DISALLOW_TOO_LONG_IB_PACKETS_SMASK
		| WFR_SEND_CTXT_CHECK_ENABLE_DISALLOW_BAD_PKT_LEN_SMASK
		| WFR_SEND_CTXT_CHECK_ENABLE_DISALLOW_PBC_TEST_SMASK
		| WFR_SEND_CTXT_CHECK_ENABLE_DISALLOW_TOO_SMALL_BYPASS_PACKETS_SMASK
		| WFR_SEND_CTXT_CHECK_ENABLE_DISALLOW_TOO_SMALL_IB_PACKETS_SMASK
		| WFR_SEND_CTXT_CHECK_ENABLE_DISALLOW_BYPASS_SMASK
		| WFR_SEND_CTXT_CHECK_ENABLE_DISALLOW_RAW_IPV6_SMASK
		| WFR_SEND_CTXT_CHECK_ENABLE_DISALLOW_RAW_SMASK
		| WFR_SEND_CTXT_CHECK_ENABLE_CHECK_BYPASS_VL_MAPPING_SMASK
		| WFR_SEND_CTXT_CHECK_ENABLE_CHECK_VL_MAPPING_SMASK
		| WFR_SEND_CTXT_CHECK_ENABLE_CHECK_OPCODE_SMASK
		| WFR_SEND_CTXT_CHECK_ENABLE_CHECK_SLID_SMASK
		| WFR_SEND_CTXT_CHECK_ENABLE_CHECK_PARTITION_KEY_SMASK
		| WFR_SEND_CTXT_CHECK_ENABLE_CHECK_JOB_KEY_SMASK
		| WFR_SEND_CTXT_CHECK_ENABLE_CHECK_VL_SMASK
		| WFR_SEND_CTXT_CHECK_ENABLE_CHECK_ENABLE_SMASK;

	for (i = 0; i < dd->num_send_contexts; i++) {
		/* per context type checks */
		if (dd->send_contexts[i].type == SC_USER) {
			reg =  all | WFR_SEND_CTXT_CHECK_ENABLE_DISALLOW_NON_KDETH_PACKETS_SMASK;
		} else {
			reg = all | WFR_SEND_CTXT_CHECK_ENABLE_DISALLOW_KDETH_PACKETS_SMASK;
		}
		write_kctxt_csr(dd, i, WFR_SEND_CTXT_CHECK_ENABLE, reg);
		/* unmask all errors */
		write_kctxt_csr(dd, i, WFR_SEND_CTXT_ERR_MASK, (u64)-1);
		/*
		 * Set up a threshold credit return when one MTU of space
		 * remains.  At the moment all VLs have the same MTU.
		 * That will change.
		 *
		 * TODO: We may want a "credit return MTU factor" to change
		 * the threshold in terms of 10ths of an MTU.  E.g.
		release_credits = (dd->pport[0].ibmtu * cr_mtu_factor) /
					(10 * WFR_PIO_BLOCK_SIZE);
		 */
		release_credits = dd->pport[0].ibmtu / WFR_PIO_BLOCK_SIZE;
		if (dd->send_contexts[i].credits <= release_credits)
			thresh = 1;
		else
			thresh = dd->send_contexts[i].credits - release_credits;
		reg = thresh << WFR_SEND_CTXT_CREDIT_CTRL_THRESHOLD_SHIFT;
		/* turn on credit interrupt if kernel thread */
		if (dd->send_contexts[i].type == SC_KERNEL)
			reg |= WFR_SEND_CTXT_CREDIT_CTRL_CREDIT_INTR_SMASK;
		write_kctxt_csr(dd, i, WFR_SEND_CTXT_CREDIT_CTRL, reg);

		/* set the default partition key */
		write_kctxt_csr(dd, i, WFR_SEND_CTXT_CHECK_PARTITION_KEY,
			(DEFAULT_PKEY &
				WFR_SEND_CTXT_CHECK_PARTITION_KEY_VALUE_MASK)
			    << WFR_SEND_CTXT_CHECK_PARTITION_KEY_VALUE_SHIFT);

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
int sc_hw_alloc(struct hfi_devdata *dd, int type, u32 *contextp)
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
void sc_hw_free(struct hfi_devdata *dd, u32 context)
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

/* obtain the credit return addresses, kernel virtual and physical */
static void cr_group_addresses(struct hfi_devdata *dd, int numa, u32 context,
			u32 group, volatile __le64 **va, unsigned long *pa)
{
	u32 gc = group_context(context, group);

	*va = &dd->cr_base[numa].va[gc].cr[context & 0x7];
	*pa = (unsigned long)&((struct credit_return *)dd->cr_base[numa].pa)[gc].cr[context & 0x7];
}

/*
 * Allocate a per-NUMA send context structure of the given type along
 * with a HW context.
 */
struct send_context *sc_alloc(struct hfi_devdata *dd, int type, int numa)
{
	struct send_context_info *sci;
	struct send_context *sc;
	unsigned long pa;
	u64 reg;
	u32 context;
	int ret;

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
	spin_lock_init(&sc->alloc_lock);
	spin_lock_init(&sc->release_lock);

	/* TBD: group set-up.  Make it always 0 for now. */
	sc->group = 0;

	cr_group_addresses(dd, numa, context, sc->group, &sc->hw_free, &pa);
	sc->context = context;
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

	dd = sc->dd;
	context = sc->context;
	sc_disable(sc);	/* make sure the HW is disabled */
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

	if (!sc)
		return;

	reg = read_kctxt_csr(sc->dd, sc->context, WFR_SEND_CTXT_CTRL);
	if ((reg & WFR_SEND_CTXT_CTRL_CTXT_ENABLE_SMASK)) {
		reg &= ~WFR_SEND_CTXT_CTRL_CTXT_ENABLE_SMASK;
		spin_lock_irqsave(&sc->alloc_lock, flags);
		write_kctxt_csr(sc->dd, sc->context, WFR_SEND_CTXT_CTRL, reg);
		sc->enabled = 0;
		spin_unlock_irqrestore(&sc->alloc_lock, flags);
	}
}

/* enable the context */
int sc_enable(struct send_context *sc)
{
	u64 reg;
	int ret = 0;

	if (!sc) {
		ret = -EINVAL;
		goto done;
	}

	reg = read_kctxt_csr(sc->dd, sc->context, WFR_SEND_CTXT_CTRL);
	/* IMPORTANT: only clear free and fill if transitioning 0 -> 1 */
	if ((reg & WFR_SEND_CTXT_CTRL_CTXT_ENABLE_SMASK) == 0) {
		u64 pio;
		unsigned long flags;
		struct hfi_devdata *dd = sc->dd;

		// FIXME: obtain the locks?
		*sc->hw_free = 0;
		sc->free = 0;
		sc->alloc_free = 0;
		sc->fill = 0;
		sc->sr_head = 0;
		sc->sr_tail = 0;

		/*
		 * The HW PIO initialization engine can handle only one
		 * init request at a time. Serialize access to each device's
		 * engine.
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
		write_csr(sc->dd, WFR_SEND_PIO_INIT_CTXT, pio);
		/*
		 * Wait until the engine is done.
		 * Give the chip the required time so, hopefully, we read the
		 * register just once.
		 */
		udelay(2);
		pio = read_csr(sc->dd, WFR_SEND_PIO_INIT_CTXT);
		while(pio & WFR_SEND_PIO_INIT_CTXT_PIO_INIT_IN_PROGRESS_SMASK) {
			msleep(20);
			pio = read_csr(sc->dd, WFR_SEND_PIO_INIT_CTXT);
		}
		spin_unlock_irqrestore(&dd->sc_init_lock, flags);
		/*
		 * Initialization is done. The register holds a valid error
		 * status.
		 */
		if (pio & WFR_SEND_PIO_INIT_CTXT_PIO_INIT_ERR_SMASK) {
                        dd_dev_err(sc->dd,
				   "ctxt%u: Ctxt not enabled due to init failure\n",
				   sc->context);
			ret = -EFAULT;
		} else {
			/*
			 * All is well. Enable the context.
			 * We get the allocator lock to guards against any
			 * allocation attempts (which should not happen prior
			 * to context being enabled). On the release/disable
			 * side we don't need to worry about locking since the
			 * releaser will not do anything if the context
			 * accounting values have not changed.
			 */
			reg |= WFR_SEND_CTXT_CTRL_CTXT_ENABLE_SMASK;
			spin_lock_irqsave(&sc->alloc_lock, flags);
			write_kctxt_csr(sc->dd, sc->context, WFR_SEND_CTXT_CTRL, reg);
			sc->enabled = 1;
			spin_unlock_irqrestore(&sc->alloc_lock, flags);
		}
	}
done:
	return ret;
}

/* force a credit return on the context */
void sc_return_credits(struct send_context *sc)
{
	if (!sc)
		return;

	/* a 0->1 transition schedules a credit return */
	write_kctxt_csr(sc->dd, sc->context, WFR_SEND_CTXT_CREDIT_FORCE,
		WFR_SEND_CTXT_CREDIT_FORCE_FORCE_RETURN_SMASK);
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

u64 create_pbc(struct send_context *sc, u64 flags, u32 srate,
							u32 vl, u32 dw_len)
{
	u64 pbc;
	// FIXME: calculate rate
	u32 rate = 0;

	pbc = flags
		| (vl & WFR_PBC_VL_MASK) << WFR_PBC_VL_SHIFT
		| (rate & WFR_PBC_STATIC_RATE_CONTROL_COUNT_MASK)
			<< WFR_PBC_STATIC_RATE_CONTROL_COUNT_SHIFT
		| (dw_len & WFR_PBC_LENGTH_DWS_MASK)
			<< WFR_PBC_LENGTH_DWS_SHIFT;

	return pbc;
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
	if (!sc->enabled) {
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

	/* call sent buffer callbacks */
	head = ACCESS_ONCE(sc->sr_head);	/* snapshot the head */
	tail = sc->sr_tail;
	while (head != tail) {
		pbuf = &sc->sr[tail].pbuf;

		if (sent_before(sc->free, pbuf->sent_at)) {
			/* not sent yet */
			break;
		}
		if (pbuf->cb)
			(*pbuf->cb)(pbuf->arg);

		tail++;
		if (tail >= sc->sr_size)
			tail = 0;
	}
	/* update tail, in case we moved it */
	sc->sr_tail = tail;
	spin_unlock_irqrestore(&sc->release_lock, flags);
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

int init_credit_return(struct hfi_devdata *dd)
{
	int ret;
	int num_numa;
	int i;
	int original_node_id;

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
	}
	original_node_id = dev_to_node(&dd->pcidev->dev);
	for (i = 0; i < num_numa; i++) {
		int bytes = dd->num_send_contexts*sizeof(struct credit_return);

		set_dev_node(&dd->pcidev->dev, i);
		dd->cr_base[i].va = dma_alloc_coherent(
					&dd->pcidev->dev,
					bytes,
					&dd->cr_base[i].pa,
					GFP_KERNEL);
		if (dd->cr_base[i].va == NULL) {
			set_dev_node(&dd->pcidev->dev, original_node_id);
			dd_dev_err(dd, "Unable to allocate credit return "
				"DMA range for NUMA %d\n", i);
			ret = -ENOMEM;
			goto done;
		}
		memset(dd->cr_base[i].va, 0, bytes);
	}
	set_dev_node(&dd->pcidev->dev, original_node_id);

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
