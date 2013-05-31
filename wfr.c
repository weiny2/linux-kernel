/*
 * Copyright (c) 2012 Intel Corporation.  All rights reserved.
 * Copyright (c) 2008 - 2012 QLogic Corporation. All rights reserved.
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

/*
 * This file contains all of the code that is specific to the WFR chip
 */

#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/module.h>

#include "hfi.h"

#define NUM_IB_PORTS 1


static uint mod_num_sdma;
module_param_named(num_sdma, mod_num_sdma, uint, S_IRUGO);
MODULE_PARM_DESC(num_sdma, "Set max number SDMA engines to use");

/* TODO: temporary */
static uint print_unimplemented = 0;
module_param_named(print_unimplemented, print_unimplemented, uint, S_IRUGO);
MODULE_PARM_DESC(print_unimplemented, "Have unimplemented functions print when called");

static inline u64 read_csr(const struct qib_devdata *dd, u32 offset)
{
	if (dd->flags & QIB_PRESENT)
		return readq((void *)dd->kregbase + offset);
	return -1;
}

static inline void write_csr(const struct qib_devdata *dd,
				  u32 offset, u64 value)
{
	if (dd->flags & QIB_PRESENT)
		writeq(value, (void *)dd->kregbase + offset);
}

/*
 * Generic interrupt handler.  This is able to correctly handle
 * all interrupts in case INTx is used.
 */
static irqreturn_t generic_interrupt(int irq, void *data)
{
	struct qib_devdata *dd = data;
	u64 regs[WFR_CCE_NUM_INT_CSRS];
	int i;

	/* phase 1: scan and clear all handled interrupts */
	for (i = 0; i < WFR_CCE_NUM_INT_CSRS; i++) {
		if (dd->gi_mask[i] == 0) {
			regs[i] = 0;	/* used later */
			continue;
		}
		regs[i] = read_csr(dd, WFR_CCE_INT_STATUS + (8 * i)) &
				dd->gi_mask[i];
		/* only clear if anything is set */
		if (regs[i])
			write_csr(dd, WFR_CCE_INT_STATUS + (8 * i), regs[i]);
	}

	/* phase 2: call the apropriate handler */
	for (i = 0; i < WFR_CCE_NUM_INT_CSRS; i++) {
		u64 val;
		int j, intr;

		/* TODO: no 64-bit ffs? */
		for (j = 0, val = regs[i]; val && j < 64; j++) {
			if (__test_and_clear_bit(j,
					(volatile unsigned long *)&val)) {
				intr = (i*8) + j;
				/* TODO: call the real handler */
				printk("hfi: generic interrupt %d\n", intr);
			}
		}
	}

	return IRQ_HANDLED;
}

static irqreturn_t sdma_interrupt(int irq, void *data)
{
	struct sdma_engine *per_sdma = data;

	/* TODO: actually do something */
	printk("hfi: sdma_interrupt, engine #%d\n", per_sdma->which);

	return IRQ_HANDLED;
}

static irqreturn_t receive_context_interrupt(int irq, void *data)
{
	struct qib_ctxtdata *rcd = data;

	/* TODO: actually do something */
	printk("hfi: receive_context_interrupt, ctxt %u\n", rcd->ctxt);

	return IRQ_HANDLED;
}


/* ========================================================================= */

static void sdma_sendctrl(struct qib_pportdata *ppd, unsigned op)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
}

static void sdma_hw_clean_up(struct qib_pportdata *ppd)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
}

static void sdma_update_tail(struct qib_pportdata *ppd, u16 tail)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
}

static void sdma_hw_start_up(struct qib_pportdata *ppd)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
}

static void set_armlaunch(struct qib_devdata *dd, u32 enable)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
}

static int bringup_serdes(struct qib_pportdata *ppd)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
	return 0;
}

static void quiet_serdes(struct qib_pportdata *ppd)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
}

static void setextled(struct qib_pportdata *ppd, u32 on)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
}

static void stop_irq(struct qib_devdata *dd)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
}

static void wantpiobuf_intr(struct qib_devdata *dd, u32 needint)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
}

static int reset(struct qib_devdata *dd)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
	return 0;
}

static void put_tid(struct qib_devdata *dd, u64 __iomem *tidptr,
			     u32 type, unsigned long pa)
{
	static int called;
	if (!called) {
		called = 1;
		if (print_unimplemented)
			printk("%s: not implemented\n", __func__);
	}
}

static void clear_tids(struct qib_devdata *dd,
				struct qib_ctxtdata *rcd)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
}

static int get_base_info(struct qib_ctxtdata *rcd,
				  struct qib_base_info *kinfo)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
	return 0;
}

static struct qib_message_header *get_msgheader(
				struct qib_devdata *dd, __le32 *rhf_addr)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
	return (struct qib_message_header *)rhf_addr;
}

static int get_ib_cfg(struct qib_pportdata *ppd, int which)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
	return 0;
}

static int set_ib_cfg(struct qib_pportdata *ppd, int which, u32 val)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
	return 0;
}

static int set_ib_loopback(struct qib_pportdata *ppd, const char *what)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
	return 0;
}

static int get_ib_table(struct qib_pportdata *ppd, int which, void *t)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
	return 0;
}

static int set_ib_table(struct qib_pportdata *ppd, int which, void *t)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
	return 0;
}

static void update_usrhead(struct qib_ctxtdata *rcd, u64 hd,
				    u32 updegr, u32 egrhd, u32 npkts)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
}

static u32 hdrqempty(struct qib_ctxtdata *rcd)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
	return 0; /* not empty */
}

static void rcvctrl(struct qib_pportdata *ppd, unsigned int op,
			     int ctxt)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
}

static void sendctrl(struct qib_pportdata *ppd, u32 op)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
}

static u64 portcntr(struct qib_pportdata *ppd, u32 reg)
{
	static int called;
	if (!called) {
		called = 1;
		if (print_unimplemented)
			printk("%s: not implemented\n", __func__);
	}
	return 0;
}

static u32 read_cntrs(struct qib_devdata *dd, loff_t pos, char **namep,
			      u64 **cntrp)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
	return 0; /* final read after getting everything */
}

static u32 read_portcntrs(struct qib_devdata *dd, loff_t pos, u32 port,
				  char **namep, u64 **cntrp)
{
	static int called;
	if (!called) {
		called = 1;
		if (print_unimplemented)
			printk("%s: not implemented\n", __func__);
	}
	return 0; /* final read after getting everything */
}

static void get_faststats(unsigned long opaque)
{
	static int called;
	struct qib_devdata *dd = (struct qib_devdata *) opaque;
	if (!called) {
		called = 1;
		if (print_unimplemented)
			printk("%s: not implemented\n", __func__);
	}
	mod_timer(&dd->stats_timer, jiffies + HZ * ACTIVITY_TIMER);
}

static int intr_fallback(struct qib_devdata *dd)
{
	/*
	 * FIXME: This is called from verify_interrupt() and tries
	 * to fall back to intx.  The 7322 just turned off
	 * msix and tried to go with intx.  This won't currently
	 * work on wfr for 2 reasons:
	 *	1. it is not currently written to do that switch
	 *	2. I changed qib_nomsix to call the real pci function
	 *	   which _requires_ that all the interrupts be
	 *	   unallocated first.  This is a more thorough
	 *	   cleanup than the 7322 did and requires more
	 *	   careful code.
	 */
	static int called;
	if (!called) {
		called = 1;
		if (print_unimplemented)
			printk("%s: not implemented\n", __func__);
	}
	return 1;
}

static void xgxs_reset(struct qib_pportdata *ppd)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
}

static u32 iblink_state(u64 ibcs)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
	return IB_PORT_DOWN;
}

static u8 ibphys_portstate(u64 ibcs)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
	return 0;
}

static int ib_updown(struct qib_pportdata *ppd, int ibup, u64 ibcs)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
	return 1; /* no other IB status change processing */
}

static int gpio_mod(struct qib_devdata *dd, u32 out, u32 dir, u32 mask)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
	/* return non-zero to indicate positive progress */
	return 1;
}

static int late_initreg(struct qib_devdata *dd)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
	return 0;
}

static u32 __iomem *getsendbuf(struct qib_pportdata *ppd, u64 pbc,
					u32 *pbufnum)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
	return NULL;
}

static void set_cntr_sample(struct qib_pportdata *ppd, u32 intv,
				     u32 start)
{
	static int called;
	if (!called) {
		called = 1;
		if (print_unimplemented)
			printk("%s: not implemented\n", __func__);
	}
}

static void sdma_set_desc_cnt(struct qib_pportdata *ppd, unsigned cnt)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
}

static void sdma_init_early(struct qib_pportdata *ppd)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
}

static int init_sdma_regs(struct qib_pportdata *ppd)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
	return 0;
}

static u16 sdma_gethead(struct qib_pportdata *ppd)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
	return 0;
}

static int sdma_busy(struct qib_pportdata *ppd)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
	return 0;
}

static u32 setpbc_control(struct qib_pportdata *ppd, u32 plen,
				   u8 srate, u8 vl)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
	return 0;
}

static void initvl15_bufs(struct qib_devdata *dd)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
}

static void init_ctxt(struct qib_ctxtdata *rcd)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
	/* FIXME: must set these values, on 7322 the values depend
	   on whether the context # was < than NUM_IB_PORTS */
	rcd->rcvegrcnt = 1024;
	rcd->rcvegr_tid_base = 0; /* this value should be OK for now */
}

static void txchk_change(struct qib_devdata *dd, u32 start,
				  u32 len, u32 which, struct qib_ctxtdata *rcd)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
}

static int tempsense_rd(struct qib_devdata *dd, int regnum)
{
	if (print_unimplemented)
		printk("%s: not implemented\n", __func__);
	return -ENXIO;
}

/* ========================================================================= */

/*
 * Enable/disable chip from delivering interrupts.
 */
static void set_intr_state(struct qib_devdata *dd, u32 enable)
{
	int i;

	/*
	 * In WFR, the mask needs to be 1 to allow interupts.
	 */
	if (enable) {
		/* TODO: QIB_BADINTR check needed? */
		if (dd->flags & QIB_BADINTR)
			return;
		/* enable all interrupts */
		for (i = 0; i < WFR_CCE_NUM_INT_CSRS; i++)
			write_csr(dd, WFR_CCE_INT_MASK + (8*i), ~(u64)0);
		/*
		 * TODO: the 7322 wrote to INTCLEAR to "cause any
		 * pending interrupts to be redelivered".  The
		 * WFR HAS does not indicate that this this occurs
		 * for WFR.
		 *
		 * TODO: the 7322 also does a read and write of INTGRANTED
		 * for MSI-X interrupts.  Is there an equivalent for
		 * WFR?
		 * See qib_7322_set_intr_state() for details.
		 */

	} else {
		for (i = 0; i < WFR_CCE_NUM_INT_CSRS; i++)
			write_csr(dd, WFR_CCE_INT_MASK + (8*i), 0ull);
	}
}

/*
 * Clear all interrupt sources on the chip.
 */
static void clear_all_interrupts(struct qib_devdata *dd)
{
	int i;
	for (i = 0; i < WFR_CCE_NUM_INT_CSRS; i++)
		write_csr(dd, WFR_CCE_INT_CLEAR + (8*i), ~(u64)0);
}

/* TODO: Move to pcie.c? */
static void disable_intx(struct pci_dev *pdev)
{
	pci_intx(pdev, 0);
}

static void clean_up_interrupts(struct qib_devdata *dd)
{
	int i;

	/* remove irqs - must happen before disabling/turning off */
	if (dd->num_msix_entries) {
		/* MSI-X */
		struct qib_msix_entry *me = dd->msix_entries;
		for (i = 0; i < dd->num_msix_entries; i++, me++) {
			if (me->arg == NULL) /* no irq, no affinity */
				break;
			irq_set_affinity_hint(dd->msix_entries[i].msix.vector,
					NULL);
			free_irq(me->msix.vector, me->arg);
		}
	} else {
		/* INTx */
		free_irq(dd->pcidev->irq, dd);
	}

	/* turn off interrupts */
	if (dd->num_msix_entries == 0) {
		/* INTx */
		disable_intx(dd->pcidev);
	} else {
		/* MSI-X */
		qib_nomsix(dd);
	}

	/* clean structures */
	for (i = 0; i < dd->num_msix_entries; i++)
		free_cpumask_var(dd->msix_entries[i].mask);
	kfree(dd->msix_entries);
	dd->msix_entries = NULL;
	dd->num_msix_entries = 0;
}

/*
 * Remap the interupt source from the generic handler to the given MSI-X
 * interrupt.
 */
static void remap_intr(struct qib_devdata *dd, int isrc, int msix_intr)
{
	u64 reg;
	int m, n;

	/* clear from the handled mask of the generic interrupt */
	m = isrc / 64;
	n = isrc % 64;
	dd->gi_mask[m] &= ~((u64)1 << n);

	/* direct the chip source to the given MSI-X interrupt */
	m = isrc / 8;
	n = isrc % 8;
	reg = read_csr(dd, WFR_CCE_INT_MAP + (8*m));
	reg = (reg & ~((u64)0xff << n)) | (((u64)msix_intr & 0xff) << n);
	write_csr(dd, WFR_CCE_INT_MAP + (8*m), reg);
}

static void remap_sdma_interrupts(struct qib_devdata *dd,
						int engine, int msix_intr)
{
	int sdma_base = WFR_IS_SDMAINT_START + engine;

	/*
	 * SDMA engine interrupt sources grouped by type, rather than
	 * engine.  Per-engine interupts are as follows, we want the
	 * first 3:
	 *	SDMAInt		- fast path
	 *	SDMAIdleInt	- fast path
	 *	SDMAProgressInt - fast path
	 *	SDMACleanupDone	- slow path
	 * 
	 */
	remap_intr(dd, sdma_base + (0 * WFR_TXE_NUM_SDMA_ENGINES), msix_intr);
	remap_intr(dd, sdma_base + (1 * WFR_TXE_NUM_SDMA_ENGINES), msix_intr);
	remap_intr(dd, sdma_base + (2 * WFR_TXE_NUM_SDMA_ENGINES), msix_intr);
}

static void remap_receive_context_interrupt(struct qib_devdata *dd,
						int rx, int msix_intr)
{
	remap_intr(dd, WFR_IS_RCVAVAILINT_START + rx, msix_intr);
}

static int set_up_interrupts(struct qib_devdata *dd)
{
	struct qib_msix_entry *entries;
	u32 total, request;
	int i, ret;
	int single_interrupt = 0; /* we expect to have all the interrupts */

	/*
	 * General interrupt scheme:
	 *	1 generic, "slow path" interupt (includes SDMA engine slow
	 *		source, SDMACleanupDone)
	 *	1 interrupt per used SDMA engine (handles the 3 fast sources)
	 *	1 interupt per used rx context
	 *	TODO: pio (tx) contexts?
	 */
	total = 1 + dd->num_sdma + dd->cfgctxts;

	entries = kzalloc(sizeof(*entries) * total, GFP_KERNEL);
	if (!entries) {
		qib_dev_err(dd, "cannot allocate msix table\n");
		ret = -ENOMEM;
		goto fail;
	}
	/* 1-1 MSI-X entry assignment */
	for (i = 0; i < total; i++)
		entries[i].msix.entry = i;

	/* expect a width of 16 */
	request = total;
	ret = qib_pcie_params(dd, 16, &request, entries);
	if (ret)
		goto fail;

	if (request == 0) {
		/* using INTx */
		/* dd->num_msix_entries already zero */
		kfree(entries);
		/* qib_pcie_params() will print if using INTx */
		single_interrupt = 1;
		/* FIXME: set something on chip to use INTx? */
	} else if (request == total) {
		/* using MSI-X */
		dd->num_msix_entries = total;
		dd->msix_entries = entries;
		qib_devinfo(dd->pcidev, "%u MSI-X interrupts allocated\n",
			total);
		/* FIXME: set something on chip to use MSI-X? */
	} else {
		/* using MSI-X, with reduced interrupts */
		/* TODO: handle reduced interrupt case?  Need scheme to
		   decide who shares. */
		qib_dev_err(dd,
			"cannot handle reduced interrupt case, want %u, got %u\n",
			total, request);
		ret = -EINVAL;
		goto fail;
	}

	/* mask all interrupts */
	set_intr_state(dd, 0);
	/* clear all pending interrupts */
	clear_all_interrupts(dd);
	/* TODO: clear diagnostic reg? */

	/*
	 * By default, all interrupts go to the generic handler.
	 * We will remap the fast interrupts on a 1 by 1 basis.
	 */
	/* all interrupts accepted */
	for (i = 0; i < WFR_CCE_NUM_INT_CSRS; i++)
		dd->gi_mask[i] = ~(u64)0;
	/* all interrupts map to MSI-X 0 */
	for (i = 0; i < WFR_CCE_NUM_INT_MAP_CSRS; i++)
		write_csr(dd, WFR_CCE_INT_MAP + (8*i), 0);

	/*
	 * Intialize per-SDMA data, so we have a unique pointer to hand to
	 * the handler.
	 */
	for (i = 0; i < dd->num_sdma; i++) {
		dd->per_sdma[i].dd = dd;
		dd->per_sdma[i].which = i;
	}

	if (single_interrupt) {
		ret = request_irq(dd->pcidev->irq, generic_interrupt,
				  IRQF_SHARED, DRIVER_NAME, dd);
		if (ret) {
			qib_dev_err(dd,
				"unable to request INTx interrupt, err %d\n",
				ret);
			goto fail;
		}
	} else {
		const struct cpumask *local_mask;
		int first_cpu, restart_cpu = 0, curr_cpu = 0;
		int local_node = pcibus_to_node(dd->pcidev->bus);
		int first_generic, last_generic;
		int first_sdma, last_sdma;
		int first_rx, last_rx;

		/* calculate the ranges we are going to use */
		first_generic = 0;
		first_sdma = last_generic = first_generic + 1;
		first_rx = last_sdma = first_sdma + dd->num_sdma;
		last_rx = first_rx + dd->cfgctxts;

		/*
		 * Interrupt affinity.
		 *
		 * The "slow" interrupt can be shared with the rest of the
		 * interupts clustered on the boot processor.  After
		 * that, distribute the rest of the "fast" interrupts
		 * on the remaining CPUs of the NUMA closest to the
		 * device.
		 *
		 * If on NUMA 0:
		 *	- place the slow interupt on the first CPU
		 *	- distribute the rest, round robin, starting on
		 *	  the second CPU, avoiding cpu 0
		 *
		 * If not on NUMA 0:
		 *	- place the slow interrupt on the first CPU
		 *	- distribute the rest, round robin, including
		 *	  the first CPU
		 *
		 * Reasoning: If not on NUMA 0, then the first CPU
		 * does not have "everything else" on it and can
		 * be part of the interrupt distribution.
		 */
		local_mask = cpumask_of_pcibus(dd->pcidev->bus);
		first_cpu = cpumask_first(local_mask);
		/* TODO: What is the point of the cpumask_weight check? */
		if (first_cpu >= nr_cpu_ids ||
			cpumask_weight(local_mask) == num_online_cpus()) {
			local_mask = topology_core_cpumask(0);
			first_cpu = cpumask_first(local_mask);
		}
		if (first_cpu < nr_cpu_ids) {
			restart_cpu = cpumask_next(first_cpu, local_mask);
			if (restart_cpu >= nr_cpu_ids)
				restart_cpu = first_cpu;
		}
		/* decide the restart point */
		if (local_node > 0) {	/* not NUMA 0 */
			/* restart is the first */
			restart_cpu = first_cpu;
		}
		/*
		 * Start at the first cpu - we *know* the first
		 * interrupt is the slow interrupt.
		 */
		curr_cpu = first_cpu;

		for (i = 0; i < dd->num_msix_entries; i++) {
			struct qib_msix_entry *me = &dd->msix_entries[i];
			const char *err_info;
			irq_handler_t handler;
			void *arg;
			int idx;

			/*
			 * TODO:
			 * o Why isn't IRQF_SHARED used for the slow interrupt
			 *   here?
			 * o Should we use IRQF_SHARED for non-NUMA 0?
			 * o If we truly wrapped, don't we need IRQF_SHARED?
			*/
			/* obtain the arguments to request_irq */
			if (first_generic <= i && i < last_generic) {
				idx = i - first_generic;
				handler = generic_interrupt;
				arg = dd;
				snprintf(me->name, sizeof(me->name),
					DRIVER_NAME"%d", dd->unit);
				err_info = "generic";
			} else if (first_sdma <= i && i < last_sdma) {
				idx = i - first_sdma;
				handler = sdma_interrupt;
				arg = &dd->per_sdma[idx];
				snprintf(me->name, sizeof(me->name),
					DRIVER_NAME"%d sdma%d", dd->unit, idx);
				err_info = "sdma";
				remap_sdma_interrupts(dd, idx, i);
			} else if (first_rx <= i && i < last_rx) {
				idx = i - first_rx;
				/* no interrupt for user contexts */
				if (idx >= dd->first_user_ctxt)
					continue;
				handler = receive_context_interrupt;
				arg = dd->rcd[idx];
				snprintf(me->name, sizeof(me->name),
					DRIVER_NAME"%d kctxt%d", dd->unit, idx);
				err_info = "receive context";
				remap_receive_context_interrupt(dd, idx, i);
			} else {
				BUG();
			}
			/* no argument, no interrupt */
			if (arg == NULL)
				continue;
			/* make sure the name is terminated */
			me->name[sizeof(me->name)-1] = 0;

			ret = request_irq(me->msix.vector, handler, 0,
							me->name, arg);
			if (ret) {
				qib_dev_err(dd,
					"unable to allocate %s interrupt, vector %d, index %d, err %d\n",
					err_info, me->msix.vector, idx, ret);

				goto fail;
			}
			/*
			 * assign arg after request_irq call, so it will be
			 * cleaned up
			 */
			me->arg = arg;

			/* set the affinity hint */
			if (first_cpu < nr_cpu_ids &&
				zalloc_cpumask_var(
					&dd->msix_entries[i].mask,
					GFP_KERNEL)) {
				cpumask_set_cpu(curr_cpu,
					dd->msix_entries[i].mask);
				curr_cpu = cpumask_next(curr_cpu, local_mask);
				if (curr_cpu >= nr_cpu_ids)
					curr_cpu = restart_cpu;
				irq_set_affinity_hint(
					dd->msix_entries[i].msix.vector,
					dd->msix_entries[i].mask);
			}
		}

	}


	return 0;

fail:
	clean_up_interrupts(dd);
	return ret;
}

/*
 * Set up context values in dd.  Sets:
 *
 *	ctxtcnt - total number of contexts being used
 *	n_krcv_queues - number of kernel contexts
 *	first_user_ctxt - first non-kernel context in array of contexts
 */
static int set_up_context_variables(struct qib_devdata *dd)
{
	int num_kernel_contexts;
	int num_user_contexts;
	int total_contexts;
	u32 chip_rx_contexts;

	chip_rx_contexts = (u32)read_csr(dd, WFR_RXE_RCVCONTEXTS);

	/*
	 * Kernel contexts: (to be fixed later):
	 * 	- default to 1 kernel context per CPU
	 */
	if (qib_n_krcv_queues) {
		num_kernel_contexts = qib_n_krcv_queues;
	} else {
		num_kernel_contexts = num_online_cpus();
	}

	/*
	 * User contexts: (to be fixed later)
	 *	- default to 1 user context per CPU
	 */
	num_user_contexts = num_online_cpus();

	total_contexts = num_kernel_contexts + num_user_contexts;

	/*
	 * Adjust the counts given a global max.
	 *	- always cut out user contexts before kernel contexts
	 *	- only extend user contexts
	 */
	if (qib_cfgctxts) {
		if (qib_cfgctxts < total_contexts) {
			/* cut back, user first */
			if (qib_cfgctxts < num_kernel_contexts) {
				num_kernel_contexts = qib_cfgctxts;
				num_user_contexts = 0;
			} else {
				num_user_contexts = qib_cfgctxts - num_kernel_contexts;
			}
		} else {
			/* extend the user context count */
			num_user_contexts = qib_cfgctxts - num_kernel_contexts;
		}
		/* recalculate */
		total_contexts = num_kernel_contexts + num_user_contexts;
	}

	if (total_contexts > chip_rx_contexts) {
		/* don't silently adjust, complain and fail */
		qib_dev_err(dd,
			"not enough physical contexts: want %d, have %d\n",
			(int)total_contexts, (int)chip_rx_contexts);
		return -ENOSPC;
	}

	/* the first N are kernel contexts, the rest are user contexts */
	/* TODO: Why is ctxtcnt being used to allocate rcd if we are only
	   going to be using cfgctxts? */
	dd->ctxtcnt = chip_rx_contexts;
	dd->cfgctxts = total_contexts;
	dd->n_krcv_queues = num_kernel_contexts;
	dd->first_user_ctxt = num_kernel_contexts;
	dd->freectxts = num_user_contexts;
	qib_devinfo(dd->pcidev,
		"chip contexts %d, used contexts %d, kernel contexts %d\n",
		(int)dd->ctxtcnt, (int)dd->cfgctxts, (int)dd->n_krcv_queues);

	return 0;
}

/*
 * Clean up stuff initialized in qib_init_wfr_funcs() - mostly.
 * TODO: Cleanup of stuff done in qib_init_wfr_funcs() gets done elsewhere,
 * so we have an asymmetric cleanup.
 *
 * This is f_cleanup
 * TODO:
 * - Remove indirect call
 * - rename to chip_cleanup() or asic_cleanup()
 * - rename qib_init_wfr_funcs() to chip_init() or asic_init()
 *
 * called from qib_postinit_cleanup()
 */
static void cleanup(struct qib_devdata *dd)
{
	clean_up_interrupts(dd);
}

/**
 * qib_init_wfr_funcs - set up the chip-specific function pointers
 * @dev: the pci_dev for qlogic_ib device
 * @ent: pci_device_id struct for this dev
 *
 * Also allocates, inits, and returns the devdata struct for this
 * device instance
 *
 * This is global, and is called directly at init to set up the
 * chip-specific function pointers for later use.
 */
struct qib_devdata *qib_init_wfr_funcs(struct pci_dev *pdev,
					   const struct pci_device_id *ent)
{
	struct qib_devdata *dd;
	struct qib_pportdata *ppd;
	int i, mtu, ret;

	dd = qib_alloc_devdata(pdev,
		NUM_IB_PORTS * sizeof(struct qib_pportdata));
	if (IS_ERR(dd))
		goto bail;

	/* pport structs are contiguous, allocated after devdata */
	ppd = (struct qib_pportdata *)(dd + 1);
	dd->pport = ppd;
	for (i = 0; i < NUM_IB_PORTS; i++)
		ppd[i].dd = dd;

	dd->f_bringup_serdes    = bringup_serdes;
	dd->f_cleanup           = cleanup;
	dd->f_clear_tids        = clear_tids;
	dd->f_free_irq          = stop_irq;
	dd->f_get_base_info     = get_base_info;
	dd->f_get_msgheader     = get_msgheader;
	dd->f_getsendbuf        = getsendbuf;
	dd->f_gpio_mod          = gpio_mod;
	dd->f_hdrqempty         = hdrqempty;
	dd->f_ib_updown         = ib_updown;
	dd->f_init_ctxt         = init_ctxt;
	dd->f_initvl15_bufs     = initvl15_bufs;
	dd->f_intr_fallback     = intr_fallback;
	dd->f_late_initreg      = late_initreg;
	dd->f_setpbc_control    = setpbc_control;
	dd->f_portcntr          = portcntr;
	dd->f_put_tid           = put_tid;
	dd->f_quiet_serdes      = quiet_serdes;
	dd->f_rcvctrl           = rcvctrl;
	dd->f_read_cntrs        = read_cntrs;
	dd->f_read_portcntrs    = read_portcntrs;
	dd->f_reset             = reset;
	dd->f_init_sdma_regs    = init_sdma_regs;
	dd->f_sdma_busy         = sdma_busy;
	dd->f_sdma_gethead      = sdma_gethead;
	dd->f_sdma_sendctrl     = sdma_sendctrl;
	dd->f_sdma_set_desc_cnt = sdma_set_desc_cnt;
	dd->f_sdma_update_tail  = sdma_update_tail;
	dd->f_sendctrl          = sendctrl;
	dd->f_set_armlaunch     = set_armlaunch;
	dd->f_set_cntr_sample   = set_cntr_sample;
	dd->f_iblink_state      = iblink_state;
	dd->f_ibphys_portstate  = ibphys_portstate;
	dd->f_get_ib_cfg        = get_ib_cfg;
	dd->f_set_ib_cfg        = set_ib_cfg;
	dd->f_set_ib_loopback   = set_ib_loopback;
	dd->f_get_ib_table      = get_ib_table;
	dd->f_set_ib_table      = set_ib_table;
	dd->f_set_intr_state    = set_intr_state;
	dd->f_setextled         = setextled;
	dd->f_txchk_change      = txchk_change;
	dd->f_update_usrhead    = update_usrhead;
	dd->f_wantpiobuf_intr   = wantpiobuf_intr;
	dd->f_xgxs_reset        = xgxs_reset;
	dd->f_sdma_hw_clean_up  = sdma_hw_clean_up;
	dd->f_sdma_hw_start_up  = sdma_hw_start_up;
	dd->f_sdma_init_early   = sdma_init_early;
	dd->f_tempsense_rd	= tempsense_rd;
	/*
	 * Do remaining PCIe setup and save PCIe values in dd.
	 * Any error printing is already done by the init code.
	 * On return, we have the chip mapped.
	 */
	ret = qib_pcie_ddinit(dd, pdev, ent);
	if (ret < 0)
		goto bail_free;

	/* verify that reads actually work, save revision for reset check  */
	dd->revision = read_csr(dd, WFR_CCE_REVISION);
	if (dd->revision == ~(u64)0) {
		qib_dev_err(dd, "cannot read chip CSRs\n");
		goto bail_free;
	}

	dd->majrev = (dd->revision >> WFR_CCE_REVISION_MAJOR_SHIFT)
			& WFR_CCE_REVISION_MAJOR_MASK;
	dd->minrev = (dd->revision >> WFR_CCE_REVISION_MINOR_SHIFT)
			& WFR_CCE_REVISION_MINOR_MASK;

	/* read chip values */
	dd->chip_sdma_engines = read_csr(dd, WFR_TXE_SENDDMAENGINES);
	dd->num_sdma = dd->chip_sdma_engines;
	if (mod_num_sdma && mod_num_sdma < dd->chip_sdma_engines)
		dd->num_sdma = mod_num_sdma;


	/* FIXME: fill in with real values */
	dd->palign = 8;
	dd->uregbase = 0;
	dd->rcvtidcnt = 32;
	dd->rcvtidbase = 0;
	dd->rcvegrbase = 0;
	dd->piobufbase = 0;
	dd->pio2k_bufbase = 0;
	dd->piobcnt2k = 32;
	dd->piobcnt4k = 32;
	dd->piosize2k = 32;
	dd->piosize2kmax_dwords = dd->piosize2k >> 2;
	dd->piosize4k = 32;
	dd->ureg_align = 0x10000;  /* 64KB alignment */

	/* rcvegrbufsize must be set before calling qib_create_ctxts() */
        mtu = ib_mtu_enum_to_int(qib_ibmtu);
        if (mtu == -1)
                mtu = QIB_DEFAULT_MTU;
	/* TODO: Validate this */
        dd->rcvegrbufsize = max(mtu, 2048);

	/* TODO: real board name */
	dd->boardname = kmalloc(64, GFP_KERNEL);
	sprintf(dd->boardname, "fake wfr");

	/* only 1 physical port */
	dd->num_pports = 1;

	/* set up the stats timer; the add_timer is done at end of init */
	init_timer(&dd->stats_timer);
	dd->stats_timer.function = get_faststats;
	dd->stats_timer.data = (unsigned long) dd;

	/* TODO: RcvHdrEntSize, RcvHdrCnt, and RcvHdrSize are now
	   per context, rather than global. */
	/* FIXME: arbitrary/old values */
	dd->rcvhdrcnt = 32;
	dd->rcvhdrentsize = QIB_RCVHDR_ENTSIZE;

	ret = set_up_context_variables(dd);
	if (ret)
		goto bail_cleanup;

	ret = qib_create_ctxts(dd);
	if (ret)
		goto bail_cleanup;

	/* use contexts created by qib_create_ctxts */
	ret = set_up_interrupts(dd);
	if (ret)
		goto bail_cleanup;

	/*
	 * TODO: RX init, TX init
	 * 
	 * Set the CSRs to sane, expected values - the driver can be
	 * loaded and unloaded, we can't expect reset values.
	 */

	goto bail;

bail_cleanup:
	qib_pcie_ddcleanup(dd);
bail_free:
	qib_free_devdata(dd);
	dd = ERR_PTR(ret);
bail:
	return dd;
}
