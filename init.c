/*
 * Copyright (c) 2012, 2013 Intel Corporation.  All rights reserved.
 * Copyright (c) 2006 - 2012 QLogic Corporation. All rights reserved.
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

#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/idr.h>
#include <linux/module.h>
#include <linux/printk.h>

#include "hfi.h"
#include "device.h"
#include "common.h"
#include "mad.h"

#undef pr_fmt
#define pr_fmt(fmt) DRIVER_NAME ": " fmt

/*
 * min buffers we want to have per context, after driver
 */
#define QIB_MIN_USER_CTXT_BUFCNT 7

#define QLOGIC_IB_R_SOFTWARE_MASK 0xFF
#define QLOGIC_IB_R_SOFTWARE_SHIFT 24
#define QLOGIC_IB_R_EMULATOR_MASK (1ULL<<62)

/*
 * Number of receive contexts we are configured to use (to allow for more pio
 * buffers per ctxt, etc.)  Zero means use chip value.
 */
uint num_rcv_contexts;
module_param_named(num_rcv_contexts, num_rcv_contexts, uint, S_IRUGO);
MODULE_PARM_DESC(num_rcv_contexts, "Set max number of receive contexts to use");

unsigned qib_n_krcv_queues;
module_param_named(krcvqs, qib_n_krcv_queues, uint, S_IRUGO);
MODULE_PARM_DESC(krcvqs, "number of kernel receive queues per IB port");

unsigned qib_cc_table_size;
module_param_named(cc_table_size, qib_cc_table_size, uint, S_IRUGO);
MODULE_PARM_DESC(cc_table_size, "Congestion control table entries 0 (CCA disabled - default), min = 128, max = 1984");

unsigned hfi_egrbuf_alloc_size = 0x8000;
module_param_named(egrbuf_alloc_size, hfi_egrbuf_alloc_size, uint, S_IRUGO);
MODULE_PARM_DESC(egrbuf_alloc_size, "Chunk size for Eager buffer allocation");

/* interrupt testing */
unsigned int test_interrupts;
module_param_named(test_interrupts, test_interrupts, uint, S_IRUGO);

struct workqueue_struct *qib_cq_wq;

static void verify_interrupt(struct work_struct *work);

static struct idr qib_unit_table;
u32 qib_cpulist_count;
unsigned long *qib_cpulist;

/*
 * Common code for creating the receive context array.
 */
int qib_create_ctxts(struct hfi_devdata *dd)
{
	unsigned i;
	int ret;

	dd->rcd = kzalloc(sizeof(*dd->rcd) * dd->num_rcv_contexts, GFP_KERNEL);
	if (!dd->rcd) {
		dd_dev_err(dd, 
			"Unable to allocate receive context array, failing\n");
		ret = -ENOMEM;
		goto done;
	}

	/* create one or more kernel contexts */
	for (i = 0; i < dd->first_user_ctxt; ++i) {
		struct qib_pportdata *ppd;
		struct qib_ctxtdata *rcd;

		ppd = dd->pport + (i % dd->num_pports);
		rcd = qib_create_ctxtdata(ppd, i);
		if (!rcd) {
			dd_dev_err(dd,
				"Unable to allocate kernel receive context, failing\n");
			ret = -ENOMEM;
			goto done;
		}
		/*
		 * We can setup the kernel contexts here and now because they use
		 * default values for all receive side memories. User contexts will
		 * be handled differently.
		 */
		/* XXX (Mitko): the devdata structure stores the RcvHdrQ entry size
		 * as DWords. However, hfi_setup_ctxt takes bytes from PSM and
		 * converts to DWords. Should we just use bytes in hfi_devdata? */
		ret = hfi_setup_ctxt(rcd, dd->rcv_entries / 2, dd->rcvegrbufsize,
				     dd->rcvhdrcnt, dd->rcvhdrentsize << 2);
		if (ret < 0) {
			dd_dev_err(dd,
				   "Failed to setup kernel receive context, failing\n");
			dd->rcd[rcd->ctxt] = NULL;
			qib_free_ctxtdata(dd, rcd);
			ret = -EFAULT;
			goto done;
		}
		rcd->pkeys[0] = QIB_DEFAULT_P_KEY;
		rcd->seq_cnt = 1;

		/* TODO: add NUMA information */
		rcd->sc = sc_alloc(dd, SC_KERNEL, 0 /*numa*/);
		if (!rcd->sc) {
			dd_dev_err(dd,
				"Unable to allocate kernel send context, failing\n");
			ret = -ENOMEM;
			goto done;
		}
	}
	ret = 0;
done:
	return ret;
}

/*
 * Common code for user and kernel context setup.
 */
struct qib_ctxtdata *qib_create_ctxtdata(struct qib_pportdata *ppd, u32 ctxt)
{
	struct hfi_devdata *dd = ppd->dd;
	struct qib_ctxtdata *rcd;

	rcd = kzalloc(sizeof(*rcd), GFP_KERNEL);
	if (rcd) {
		INIT_LIST_HEAD(&rcd->qp_wait_list);
		rcd->ppd = ppd;
		rcd->dd = dd;
		rcd->cnt = 1;
		rcd->ctxt = ctxt;
		dd->rcd[ctxt] = rcd;
	}
	return rcd;
}

int hfi_setup_ctxt(struct qib_ctxtdata *uctxt, u16 egrtids, u16 egrsize,
		   u16 hdrqcnt, u16 hdrqentsize)
{
	struct hfi_devdata *dd = uctxt->dd;
	int ret = 0;

	dd_dev_info(dd, "%s: setting up context %d\n", __func__,
		    uctxt->ctxt);

	if (egrtids > dd->rcv_entries) {
		dd_dev_err(dd,
			   "%s: requesting too many egrtids %u (limit: %u)\n",
			   __func__, egrtids, dd->rcv_entries);
		ret = -EINVAL;
		goto done;
	}
	uctxt->eager_count = egrtids;

	ret = dd->f_init_ctxt(uctxt);
	if (ret)
		goto done;

	if (!hfi_rcvbuf_validate(egrsize, PT_EAGER, NULL)) {
		ret = -EINVAL;
		goto done;
	}
	/*
	 * To avoid wasting a lot of memory, we allocate 32KB chunks
	 * of physically contiguous memory, advance through it until
	 * used up and then allocate more.  Of course, we need
	 * memory to store those extra pointers, now.  32KB seems to
	 * be the most that is "safe" under memory pressure
	 * (creating large files and then copying them over
	 * NFS while doing lots of MPI jobs).  The OOM killer can
	 * get invoked, even though we say we can sleep and this can
	 * cause significant system problems....
	 */
	uctxt->rcvegrbuf_chunksize = hfi_egrbuf_alloc_size;
	uctxt->rcvegrbuf_size = egrsize;
	uctxt->rcvegrbufs_perchunk =
		uctxt->rcvegrbuf_chunksize / uctxt->rcvegrbuf_size;
	if (!is_power_of_2(uctxt->rcvegrbufs_perchunk)) {
		ret = -EFAULT;
		goto done;
	}
	uctxt->rcvegrbuf_chunks = (uctxt->rcvegrbuf_size *
				   uctxt->eager_count) /
		uctxt->rcvegrbuf_chunksize;
	uctxt->rcvegrbufs_perchunk_shift =
		ilog2(uctxt->rcvegrbufs_perchunk);
	uctxt->rcvhdrq_cnt = hdrqcnt;
	/* RcvHdrQ Entry Size is in DWords */
	uctxt->rcvhdrqentsize = hdrqentsize >> 2;
done:
	return ret;
}


/*
 * Common code for initializing the physical port structure.
 */
void qib_init_pportdata(struct qib_pportdata *ppd, struct hfi_devdata *dd,
			u8 hw_pidx, u8 port)
{
	int size;
	ppd->dd = dd;
	ppd->hw_pidx = hw_pidx;
	ppd->port = port; /* IB port number, not index */

	spin_lock_init(&ppd->sdma_lock);

	init_timer(&ppd->symerr_clear_timer);
	ppd->symerr_clear_timer.function = qib_clear_symerror_on_linkup;
	ppd->symerr_clear_timer.data = (unsigned long)ppd;

	ppd->qib_wq = NULL;

	spin_lock_init(&ppd->cc_shadow_lock);

	if (qib_cc_table_size < IB_CCT_MIN_ENTRIES)
		goto bail;

	ppd->cc_supported_table_entries = min(max_t(int, qib_cc_table_size,
		IB_CCT_MIN_ENTRIES), IB_CCT_ENTRIES*IB_CC_TABLE_CAP_DEFAULT);

	ppd->cc_max_table_entries =
		ppd->cc_supported_table_entries/IB_CCT_ENTRIES;

	size = IB_CC_TABLE_CAP_DEFAULT * sizeof(struct ib_cc_table_entry)
		* IB_CCT_ENTRIES;
	ppd->ccti_entries = kzalloc(size, GFP_KERNEL);
	if (!ppd->ccti_entries) {
		dd_dev_err(dd,
		  "failed to allocate congestion control table for port %d!\n",
		  port);
		goto bail;
	}

	size = IB_CC_CCS_ENTRIES * sizeof(struct ib_cc_congestion_entry);
	ppd->congestion_entries = kzalloc(size, GFP_KERNEL);
	if (!ppd->congestion_entries) {
		dd_dev_err(dd,
		 "failed to allocate congestion setting list for port %d!\n",
		 port);
		goto bail_1;
	}

	size = sizeof(struct cc_table_shadow);
	ppd->ccti_entries_shadow = kzalloc(size, GFP_KERNEL);
	if (!ppd->ccti_entries_shadow) {
		dd_dev_err(dd,
		 "failed to allocate shadow ccti list for port %d!\n",
		 port);
		goto bail_2;
	}

	size = sizeof(struct ib_cc_congestion_setting_attr);
	ppd->congestion_entries_shadow = kzalloc(size, GFP_KERNEL);
	if (!ppd->congestion_entries_shadow) {
		dd_dev_err(dd,
		 "failed to allocate shadow congestion setting list for port %d!\n",
		 port);
		goto bail_3;
	}

	return;

bail_3:
	kfree(ppd->ccti_entries_shadow);
	ppd->ccti_entries_shadow = NULL;
bail_2:
	kfree(ppd->congestion_entries);
	ppd->congestion_entries = NULL;
bail_1:
	kfree(ppd->ccti_entries);
	ppd->ccti_entries = NULL;
bail:
	/* User is intentionally disabling the congestion control agent */
	if (!qib_cc_table_size)
		return;

	if (qib_cc_table_size < IB_CCT_MIN_ENTRIES) {
		qib_cc_table_size = 0;
		dd_dev_err(dd,
		 "Congestion Control table size %d less than minimum %d for port %d\n",
		 qib_cc_table_size, IB_CCT_MIN_ENTRIES, port);
	}

	dd_dev_err(dd, "Congestion Control Agent disabled for port %d\n",
		port);
	return;
}

/*
 * init_shadow_tids - allocate the shadow TID array
 * @dd: device data
 *
 * Allocate the shadow TID array, so we can qib_munlock previous
 * entries.  Parts of the shadow array will be unused as it is
 * combined with the eager array.  For now, live with it for
 * simplicity.
 *
 * We don't want failures here to prevent use of the driver/chip,
 * so no return value.
 */
static void init_shadow_tids(struct hfi_devdata *dd)
{
	struct page **pages;
	dma_addr_t *addrs;

	pages = vzalloc(dd->chip_rcv_array_count * sizeof(struct page *));
	if (!pages) {
		dd_dev_err(dd,
			"failed to allocate shadow page * array, no expected sends!\n");
		goto bail;
	}

	addrs = vzalloc(dd->chip_rcv_array_count * sizeof(dma_addr_t));
	if (!addrs) {
		dd_dev_err(dd,
			"failed to allocate shadow dma handle array, no expected sends!\n");
		goto bail_free;
	}

	dd->pageshadow = pages;
	dd->physshadow = addrs;
	return;

bail_free:
	vfree(pages);
bail:
	dd->pageshadow = NULL;
}

/*
 * Do initialization for device that is only needed on
 * first detect, not on resets.
 */
static int loadtime_init(struct hfi_devdata *dd)
{
	int ret = 0;

/* FIXME: needed? */
#if 0
	if (((dd->revision >> QLOGIC_IB_R_SOFTWARE_SHIFT) &
	     QLOGIC_IB_R_SOFTWARE_MASK) != QIB_CHIP_SWVERSION) {
		dd_dev_err(dd,
			"Driver only handles version %d, chip swversion is %d (%llx), failng\n",
			QIB_CHIP_SWVERSION,
			(int)(dd->revision >>
				QLOGIC_IB_R_SOFTWARE_SHIFT) &
				QLOGIC_IB_R_SOFTWARE_MASK,
			(unsigned long long) dd->revision);
		ret = -ENOSYS;
		goto done;
	}
#endif

	if (dd->revision & QLOGIC_IB_R_EMULATOR_MASK)
		dd_dev_info(dd, "%s", dd->boardversion);

	spin_lock_init(&dd->sendctrl_lock);
	spin_lock_init(&dd->uctxt_lock);
	spin_lock_init(&dd->dc8051_lock);
	spin_lock_init(&dd->qib_diag_trans_lock);
	mutex_init(&dd->qsfp_lock);

	init_shadow_tids(dd);

	/* set up worker (don't start yet) to verify interrupts are working */
	INIT_DELAYED_WORK(&dd->interrupt_check_worker, verify_interrupt);
	/* set this flag so we know we can clean up */
	dd->flags |= ICHECK_WORKER_INITED;

#if 0
done:
#endif
	return ret;
}

/**
 * init_after_reset - re-initialize after a reset
 * @dd: the qlogic_ib device
 *
 * sanity check at least some of the values after reset, and
 * ensure no receive or transmit (explicitly, in case reset
 * failed
 */
static int init_after_reset(struct hfi_devdata *dd)
{
	int i;

	/*
	 * Ensure chip does no sends or receives, tail updates, or
	 * pioavail updates while we re-initialize.  This is mostly
	 * for the driver data structures, not chip registers.
	 */
	for (i = 0; i < dd-> num_rcv_contexts; i++)
		dd->f_rcvctrl(dd, QIB_RCVCTRL_CTXT_DIS |
				  QIB_RCVCTRL_INTRAVAIL_DIS |
				  QIB_RCVCTRL_TAILUPD_DIS, i);
	pio_send_control(dd, PSC_GLOBAL_DISABLE);
	for (i = 0; i < dd->num_send_contexts; i++)
		sc_disable(dd->send_contexts[i].sc);

	return 0;
}

static void enable_chip(struct hfi_devdata *dd)
{
	u32 rcvmask;
	u32 i;

	/* enable PIO send */
	pio_send_control(dd, PSC_GLOBAL_ENABLE);

	/*
	 * Enable kernel ctxts' receive and receive interrupt.
	 * Other ctxts done as user opens and inits them.
	 */
	rcvmask = QIB_RCVCTRL_CTXT_ENB | QIB_RCVCTRL_INTRAVAIL_ENB;
	rcvmask |= (dd->flags & QIB_NODMA_RTAIL) ?
		  QIB_RCVCTRL_TAILUPD_DIS : QIB_RCVCTRL_TAILUPD_ENB;
	for (i = 0; i < dd->first_user_ctxt; ++i) {
		dd->f_rcvctrl(dd, rcvmask, i);
		/* XXX (Mitko): Do we care about the result of this?
		 * sc_enable() will display an error message. */
		sc_enable(dd->rcd[i]->sc);
	}
}

static void verify_interrupt(struct work_struct *work)
{
        struct hfi_devdata *dd = container_of(work, struct hfi_devdata,
						interrupt_check_worker.work);

	/*
	 * We should have interrupts by now.  If not, try falling back.
	 */
	if (dd->int_counter == 0) {
		if (!dd->f_intr_fallback(dd))
			dev_err(&dd->pcidev->dev,
				"No interrupts detected, not usable.\n");
		else /* re-arm the worker to see if the fall back works */
			mod_delayed_work(system_unbound_wq,
					&dd->interrupt_check_worker, HZ/2);
	}
}

/**
 * qib_create_workqueues - create per port workqueues
 * @dd: the qlogic_ib device
 */
static int qib_create_workqueues(struct hfi_devdata *dd)
{
	int pidx;
	struct qib_pportdata *ppd;

	for (pidx = 0; pidx < dd->num_pports; ++pidx) {
		ppd = dd->pport + pidx;
		if (!ppd->qib_wq) {
			char wq_name[8]; /* 3 + 2 + 1 + 1 + 1 */
			snprintf(wq_name, sizeof(wq_name), "qib%d_%d",
				dd->unit, pidx);
			ppd->qib_wq =
				create_singlethread_workqueue(wq_name);
			if (!ppd->qib_wq)
				goto wq_error;
		}
	}
	return 0;
wq_error:
	pr_err("create_singlethread_workqueue failed for port %d\n",
		pidx + 1);
	for (pidx = 0; pidx < dd->num_pports; ++pidx) {
		ppd = dd->pport + pidx;
		if (ppd->qib_wq) {
			destroy_workqueue(ppd->qib_wq);
			ppd->qib_wq = NULL;
		}
	}
	return -ENOMEM;
}

/**
 * qib_init - do the actual initialization sequence on the chip
 * @dd: the qlogic_ib device
 * @reinit: reinitializing, so don't allocate new memory
 *
 * Do the actual initialization sequence on the chip.  This is done
 * both from the init routine called from the PCI infrastructure, and
 * when we reset the chip, or detect that it was reset internally,
 * or it's administratively re-enabled.
 *
 * Memory allocation here and in called routines is only done in
 * the first case (reinit == 0).  We have to be careful, because even
 * without memory allocation, we need to re-write all the chip registers
 * TIDs, etc. after the reset or enable has completed.
 */
int qib_init(struct hfi_devdata *dd, int reinit)
{
	int ret = 0, pidx, lastfail = 0;
	unsigned i;
	struct qib_ctxtdata *rcd;
	struct qib_pportdata *ppd;

	/* make sure the link is not "up" */
	for (pidx = 0; pidx < dd->num_pports; ++pidx) {
		ppd = dd->pport + pidx;
		ppd->linkup = 0;
	}

	if (reinit)
		ret = init_after_reset(dd);
	else
		ret = loadtime_init(dd);
	if (ret)
		goto done;

	/* dd->rcd can be NULL if early init failed */
	for (i = 0; dd->rcd && i < dd->first_user_ctxt; ++i) {
		/*
		 * Set up the (kernel) rcvhdr queue and egr TIDs.  If doing
		 * re-init, the simplest way to handle this is to free
		 * existing, and re-allocate.
		 * Need to re-create rest of ctxt 0 ctxtdata as well.
		 */
		rcd = dd->rcd[i];
		if (!rcd)
			continue;

		lastfail = qib_create_rcvhdrq(dd, rcd);
		if (!lastfail)
			lastfail = qib_setup_eagerbufs(rcd);
		if (lastfail) {
			dd_dev_err(dd,
				"failed to allocate kernel ctxt's rcvhdrq and/or egr bufs\n");
			continue;
		}
	}
	if (lastfail)
		ret = lastfail;

	/* Allocate a page for event notifiction. */
	dd->events = vmalloc_user(PAGE_SIZE);
	if (!dd->events)
		dd_dev_err(dd, "Failed to allocate user events page\n");
	else
		memset(dd->events, 0, PAGE_SIZE);
	/*
	 * Allocate a page for device and port status.
	 * Page will be shared amongst all user proceses.
	 */
	dd->status = vmalloc_user(PAGE_SIZE);
	if (!dd->status)
		dd_dev_err(dd, "Failed to allocate dev status page\n");
	else {
		memset(dd->status, 0, PAGE_SIZE);
		dd->freezelen = PAGE_SIZE - (sizeof(*dd->status) -
					     sizeof(dd->status->freezemsg));
	}
	for (pidx = 0; pidx < dd->num_pports; ++pidx) {
		ppd = dd->pport + pidx;
		if (dd->status)
			/* Currently, we only have one port */
			ppd->statusp = &dd->status->port;

		set_mtu(ppd, default_mtu);
	}

	/* enable chip even if we have an error, so we can debug cause */
	enable_chip(dd);

done:
	/*
	 * Set status even if port serdes is not initialized
	 * so that diags will work.
	 */
	if (dd->status)
		dd->status->dev |= QIB_STATUS_CHIP_PRESENT |
			QIB_STATUS_INITTED;
	if (!ret) {
		/* enable all interrupts from the chip */
		dd->f_set_intr_state(dd, 1);

		/* chip is OK for user apps; mark it as initialized */
		for (pidx = 0; pidx < dd->num_pports; ++pidx) {
			ppd = dd->pport + pidx;

			/* start the serdes - must be after interrupts are
			   enabled so we are notified when the link goes up */
			lastfail = dd->f_bringup_serdes(ppd);
			if (lastfail) {
				dd_dev_info(dd,
					"Failed to bringup IB port %u\n",
					ppd->port);
			}
			/*
			 * Set status even if port serdes is not initialized
			 * so that diags will work.
			 */
			if (ppd->statusp)
				*ppd->statusp |= QIB_STATUS_CHIP_PRESENT |
							QIB_STATUS_INITTED;
			if (!ppd->link_speed_enabled)
				continue;
			if (dd->flags & QIB_HAS_SEND_DMA)
				ret = qib_setup_sdma(ppd);
		}

		/*
		 * Verify that we get an interrupt, fall back to an alternate if
		 * necessary and possible.
		 */
		mod_delayed_work(system_unbound_wq,
					&dd->interrupt_check_worker, HZ/2);
		/* start stats retrieval timer */
		mod_timer(&dd->stats_timer, jiffies + HZ * ACTIVITY_TIMER);
	}

	/* if ret is non-zero, we probably should do some cleanup here... */
	return ret;
}

static inline struct hfi_devdata *__qib_lookup(int unit)
{
	return idr_find(&qib_unit_table, unit);
}

struct hfi_devdata *qib_lookup(int unit)
{
	struct hfi_devdata *dd;
	unsigned long flags;

	spin_lock_irqsave(&qib_devs_lock, flags);
	dd = __qib_lookup(unit);
	spin_unlock_irqrestore(&qib_devs_lock, flags);

	return dd;
}

/*
 * Stop the timers during unit shutdown, or after an error late
 * in initialization.
 */
static void qib_stop_timers(struct hfi_devdata *dd)
{
	struct qib_pportdata *ppd;
	int pidx;

	if (dd->stats_timer.data) {
		del_timer_sync(&dd->stats_timer);
		dd->stats_timer.data = 0;
	}
	if (dd->flags & ICHECK_WORKER_INITED)
		cancel_delayed_work_sync(&dd->interrupt_check_worker);
	for (pidx = 0; pidx < dd->num_pports; ++pidx) {
		ppd = dd->pport + pidx;
		if (ppd->led_override_timer.data) {
			del_timer_sync(&ppd->led_override_timer);
			atomic_set(&ppd->led_override_timer_active, 0);
		}
		if (ppd->symerr_clear_timer.data)
			del_timer_sync(&ppd->symerr_clear_timer);
	}
}

/**
 * qib_shutdown_device - shut down a device
 * @dd: the qlogic_ib device
 *
 * This is called to make the device quiet when we are about to
 * unload the driver, and also when the device is administratively
 * disabled.   It does not free any data structures.
 * Everything it does has to be setup again by qib_init(dd, 1)
 */
static void qib_shutdown_device(struct hfi_devdata *dd)
{
	struct qib_pportdata *ppd;
	unsigned pidx;
	int i;

	for (pidx = 0; pidx < dd->num_pports; ++pidx) {
		ppd = dd->pport + pidx;

		ppd->linkup = 0;
		if (ppd->statusp)
			*ppd->statusp &= ~(QIB_STATUS_IB_CONF | QIB_STATUS_IB_READY);
	}
	dd->flags &= ~QIB_INITTED;

	/* mask interrupts, but not errors */
	dd->f_set_intr_state(dd, 0);

	for (pidx = 0; pidx < dd->num_pports; ++pidx) {
		ppd = dd->pport + pidx;
		for (i = 0; i < dd-> num_rcv_contexts; i++)
			dd->f_rcvctrl(dd, QIB_RCVCTRL_TAILUPD_DIS |
					  QIB_RCVCTRL_CTXT_DIS |
					  QIB_RCVCTRL_INTRAVAIL_DIS |
					  QIB_RCVCTRL_PKEY_ENB, i);
		/*
		 * Gracefully stop all sends allowing any in progress to
		 * trickle out first.
		 */
		for (i = 0; i < dd->num_send_contexts; i++)
			sc_flush(dd->send_contexts[i].sc);
	}

	/*
	 * Enough for anything that's going to trickle out to have actually
	 * done so.
	 */
	udelay(20);

	for (pidx = 0; pidx < dd->num_pports; ++pidx) {
		ppd = dd->pport + pidx;
		dd->f_setextled(ppd, 0); /* make sure LEDs are off */

		if (dd->flags & QIB_HAS_SEND_DMA)
			qib_teardown_sdma(ppd);

		/* disable all contexts */
		for (i = 0; i < dd->num_send_contexts; i++)
			sc_disable(dd->send_contexts[i].sc);
		/* disable the send device */
		pio_send_control(dd, PSC_GLOBAL_DISABLE);

		/*
		 * Clear SerdesEnable.
		 * We can't count on interrupts since we are stopping.
		 */
		dd->f_quiet_serdes(ppd);

		if (ppd->qib_wq) {
			destroy_workqueue(ppd->qib_wq);
			ppd->qib_wq = NULL;
		}
	}
}

/**
 * qib_free_ctxtdata - free a context's allocated data
 * @dd: the qlogic_ib device
 * @rcd: the ctxtdata structure
 *
 * free up any allocated data for a context
 * This should not touch anything that would affect a simultaneous
 * re-allocation of context data, because it is called after qib_mutex
 * is released (and can be called from reinit as well).
 * It should never change any chip state, or global driver state.
 */
void qib_free_ctxtdata(struct hfi_devdata *dd, struct qib_ctxtdata *rcd)
{
	if (!rcd)
		return;

	if (rcd->rcvhdrq) {
		dma_free_coherent(&dd->pcidev->dev, rcd->rcvhdrq_size,
				  rcd->rcvhdrq, rcd->rcvhdrq_phys);
		rcd->rcvhdrq = NULL;
		if (rcd->rcvhdrtail_kvaddr) {
			dma_free_coherent(&dd->pcidev->dev, PAGE_SIZE,
					  rcd->rcvhdrtail_kvaddr,
					  rcd->rcvhdrqtailaddr_phys);
			rcd->rcvhdrtail_kvaddr = NULL;
		}
	}
	if (rcd->rcvegrbuf) {
		unsigned e;

		for (e = 0; e < rcd->rcvegrbuf_chunks; e++) {
			void *base = rcd->rcvegrbuf[e];
			size_t size = rcd->rcvegrbuf_size;

			dma_free_coherent(&dd->pcidev->dev, size,
					  base, rcd->rcvegrbuf_phys[e]);
		}
		kfree(rcd->rcvegrbuf);
		rcd->rcvegrbuf = NULL;
		kfree(rcd->rcvegrbuf_phys);
		rcd->rcvegrbuf_phys = NULL;
		rcd->rcvegrbuf_chunks = 0;
	}

	if (rcd->sc) {
		sc_free(rcd->sc);
		rcd->sc = NULL;
	}
	kfree(rcd->tid_pg_list);
	vfree(rcd->user_event_mask);
	vfree(rcd->subctxt_uregbase);
	vfree(rcd->subctxt_rcvegrbuf);
	vfree(rcd->subctxt_rcvhdr_base);
	kfree(rcd);
}

/*
 * Perform a PIO buffer bandwidth write test, to verify proper system
 * configuration.  Even when all the setup calls work, occasionally
 * BIOS or other issues can prevent write combining from working, or
 * can cause other bandwidth problems to the chip.
 *
 * This test simply writes a buffer over and over again, and
 * measures close to the peak bandwidth to the chip (not testing
 * data bandwidth to the wire).  The header of the buffer is
 * invalid and will generate an error, but we ignore it.
 */
int fake_early_return = 0;	//FIXME: always zero so we return early
static void qib_verify_pioperf(struct hfi_devdata *dd)
{
	struct send_context *sc;
	struct pio_buf *pbuf;
	u64 pbc, msecs, emsecs;
	u32 cnt, lcnt, dw;
	u32 *addr;

	if (dd->num_send_contexts == 0) {
		dd_dev_info(dd, "Performance check: "
			"No contexts for checking perf, skipping\n");
		return;
	}

	/* FIXME: not ready yet */
	if (fake_early_return == 0)
		return;

	/* only run this on unit 0 */
	if (dd->unit != 0)
		return;

	// TEMPORARY: force a non-context PIO error and see what happens...
	//write_csr(dd, WFR_SEND_PIO_ERR_FORCE, WFR_SEND_PIO_ERR_FORCE_PIO_WRITE_BAD_CTXT_ERR_SMASK);

	/*
	 * Enough to give us a reasonable test, less than piobuf size, and
	 * likely multiple of store buffer length.
	 */
	cnt = 1024;
	/* dword count - includes PBC */
	dw = (cnt + sizeof(u64)) / sizeof(u32);

	/* use the first context */
	sc = dd->rcd[0]->sc;
	if (!sc) {
		dd_dev_info(dd, "Performance check: No send context\n");
		return;
	}

	addr = kzalloc(cnt, GFP_KERNEL);
	if (!addr) {
		dd_dev_info(dd,
			 "Performance check: Could not get memory for "
			 "checking PIO perf, skipping\n");
		return;
	}

	/* zero'ed lrh/bth above will generate an error, disable it */
	dd->f_set_armlaunch(dd, 0);

	/* minimal PBC - just the length in DW */
	pbc = create_pbc(sc, 0, 0, 0, dw);

	preempt_disable();  /* we want reasonably accurate elapsed time */

	msecs = 1 + jiffies_to_msecs(jiffies);
	for (lcnt = 0; lcnt < 10000U; lcnt++) {
		/* wait until we cross msec boundary */
		if (jiffies_to_msecs(jiffies) >= msecs)
			break;
		udelay(1);
	}

	/*
	 * This is only roughly accurate, since even with preempt we
	 * still take interrupts that could take a while.   Running for
	 * >= 5 msec seems to get us "close enough" to accurate values.
	 */
	msecs = jiffies_to_msecs(jiffies);
	for (emsecs = lcnt = 0; emsecs <= 5UL && lcnt < 1; lcnt++) {
		pbuf = sc_buffer_alloc(sc, dw, NULL, 0);
		if (!pbuf) {
			preempt_enable();
			dd_dev_info(dd, "Performance check: PIO buffer "
				"allocation failure at count %u,"
				" skipping\n", lcnt);
			goto done;
		}
		pio_copy(pbuf, pbc, addr, cnt >> 2);
		emsecs = jiffies_to_msecs(jiffies) - msecs;
	}
	preempt_enable();

	/* 1 GiB/sec, slightly over IB SDR line rate */
	if (lcnt < (emsecs * 1024U))
		dd_dev_err(dd,
			    "Performance problem: bandwidth to PIO buffers is only %u MiB/sec\n",
			    lcnt / (u32) emsecs);

done:
	kfree(addr);
	dd->f_set_armlaunch(dd, 1);
}


void qib_free_devdata(struct hfi_devdata *dd)
{
	unsigned long flags;

	spin_lock_irqsave(&qib_devs_lock, flags);
	idr_remove(&qib_unit_table, dd->unit);
	list_del(&dd->list);
	spin_unlock_irqrestore(&qib_devs_lock, flags);

	ib_dealloc_device(&dd->verbs_dev.ibdev);
}

/*
 * Allocate our primary per-unit data structure.  Must be done via verbs
 * allocator, because the verbs cleanup process both does cleanup and
 * free of the data structure.
 * "extra" is for chip-specific data.
 *
 * Use the idr mechanism to get a unit number for this unit.
 */
struct hfi_devdata *qib_alloc_devdata(struct pci_dev *pdev, size_t extra)
{
	unsigned long flags;
	struct hfi_devdata *dd;
	int ret;

	dd = (struct hfi_devdata *) ib_alloc_device(sizeof(*dd) + extra);
	if (!dd) {
		dd = ERR_PTR(-ENOMEM);
		goto bail;
	}

	idr_preload(GFP_KERNEL);
	spin_lock_irqsave(&qib_devs_lock, flags);

	ret = idr_alloc(&qib_unit_table, dd, 0, 0, GFP_NOWAIT);
	if (ret >= 0) {
		dd->unit = ret;
		list_add(&dd->list, &qib_dev_list);
	}

	spin_unlock_irqrestore(&qib_devs_lock, flags);
	idr_preload_end();

	if (ret < 0) {
		qib_early_err(&pdev->dev,
			      "Could not allocate unit ID: error %d\n", -ret);
		ib_dealloc_device(&dd->verbs_dev.ibdev);
		dd = ERR_PTR(ret);
		goto bail;
	}

	if (!qib_cpulist_count) {
		u32 count = num_online_cpus();
		qib_cpulist = kzalloc(BITS_TO_LONGS(count) *
				      sizeof(long), GFP_KERNEL);
		if (qib_cpulist)
			qib_cpulist_count = count;
		else
			qib_early_err(&pdev->dev,
				"Could not alloc cpulist info, cpu affinity might be wrong\n");
	}

bail:
	return dd;
}

/*
 * Called from freeze mode handlers, and from PCI error
 * reporting code.  Should be paranoid about state of
 * system and data structures.
 */
void qib_disable_after_error(struct hfi_devdata *dd)
{
	if (dd->flags & QIB_INITTED) {
		u32 pidx;

		dd->flags &= ~QIB_INITTED;
		if (dd->pport)
			for (pidx = 0; pidx < dd->num_pports; ++pidx) {
				struct qib_pportdata *ppd;

				ppd = dd->pport + pidx;
				if (dd->flags & QIB_PRESENT) {
					qib_set_linkstate(ppd,
						QIB_IB_LINKDOWN_DISABLE);
					dd->f_setextled(ppd, 0);
				}
				if (ppd->statusp)
					*ppd->statusp &= ~QIB_STATUS_IB_READY;
			}
	}

	/*
	 * Mark as having had an error for driver, and also
	 * for /sys and status word mapped to user programs.
	 * This marks unit as not usable, until reset.
	 */
	if (dd->status)
		dd->status->dev |= QIB_STATUS_HWERROR;
}

static void qib_remove_one(struct pci_dev *);
static int qib_init_one(struct pci_dev *, const struct pci_device_id *);

#define DRIVER_LOAD_MSG "Intel " DRIVER_NAME " loaded: "
#define PFX DRIVER_NAME ": "

static DEFINE_PCI_DEVICE_TABLE(qib_pci_tbl) = {
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_WFR0) },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_WFR1) },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_WFR2) },
	{ 0, }
};

MODULE_DEVICE_TABLE(pci, qib_pci_tbl);

struct pci_driver qib_driver = {
	.name = DRIVER_NAME,
	.probe = qib_init_one,
	.remove = qib_remove_one,
	.id_table = qib_pci_tbl,
	.err_handler = &qib_pci_err_handler,
};

/*
 * Do all the generic driver unit- and chip-independent memory
 * allocation and initialization.
 */
static int __init qlogic_ib_init(void)
{
	int ret;

	ret = dev_init();
	if (ret)
		goto bail;

	/* validate max and default MTUs before any devices start */
	if (!valid_mtu(default_mtu))
		default_mtu = HFI_DEFAULT_ACTIVE_MTU;
	if (!valid_mtu(max_mtu))
		max_mtu = HFI_DEFAULT_MAX_MTU;

	qib_cq_wq = create_singlethread_workqueue("qib_cq");
	if (!qib_cq_wq) {
		ret = -ENOMEM;
		goto bail_dev;
	}

	/*
	 * These must be called before the driver is registered with
	 * the PCI subsystem.
	 */
	idr_init(&qib_unit_table);

	ret = pci_register_driver(&qib_driver);
	if (ret < 0) {
		pr_err("Unable to register driver: error %d\n", -ret);
		goto bail_unit;
	}

	/* not fatal if it doesn't work */
	if (qib_init_qibfs())
		pr_err("Unable to register ipathfs\n");
	goto bail; /* all OK */

bail_unit:
	idr_destroy(&qib_unit_table);
	destroy_workqueue(qib_cq_wq);
bail_dev:
	dev_cleanup();
bail:
	return ret;
}

module_init(qlogic_ib_init);

/*
 * Do the non-unit driver cleanup, memory free, etc. at unload.
 */
static void __exit qlogic_ib_cleanup(void)
{
	int ret;

	ret = qib_exit_qibfs();
	if (ret)
		pr_err("Unable to cleanup counter filesystem: error %d\n", ret);

	pci_unregister_driver(&qib_driver);

	qib_cpulist_count = 0;
	kfree(qib_cpulist);

	idr_destroy(&qib_unit_table);
	dispose_firmware();	/* asymmetric with obtain_firmware() */
	destroy_workqueue(qib_cq_wq);
	dev_cleanup();
}

module_exit(qlogic_ib_cleanup);

/* this can only be called after a successful initialization */
static void cleanup_device_data(struct hfi_devdata *dd)
{
	int ctxt;
	int pidx;
	struct qib_ctxtdata **tmp;
	unsigned long flags;

	/* users can't do anything more with chip */
	for (pidx = 0; pidx < dd->num_pports; ++pidx) {
		if (dd->pport[pidx].statusp)
			*dd->pport[pidx].statusp &= ~QIB_STATUS_CHIP_PRESENT;

		spin_lock(&dd->pport[pidx].cc_shadow_lock);

		kfree(dd->pport[pidx].congestion_entries);
		dd->pport[pidx].congestion_entries = NULL;
		kfree(dd->pport[pidx].ccti_entries);
		dd->pport[pidx].ccti_entries = NULL;
		kfree(dd->pport[pidx].ccti_entries_shadow);
		dd->pport[pidx].ccti_entries_shadow = NULL;
		kfree(dd->pport[pidx].congestion_entries_shadow);
		dd->pport[pidx].congestion_entries_shadow = NULL;

		spin_unlock(&dd->pport[pidx].cc_shadow_lock);
	}

	free_credit_return(dd);

	if (dd->pageshadow) {
		struct page **tmpp = dd->pageshadow;
		dma_addr_t *tmpd = dd->physshadow;
		int i;

		for (i = 0;  i < dd->chip_rcv_array_count; i++) {
			if (!tmpp[i])
				continue;
			pci_unmap_page(dd->pcidev, tmpd[i],
				       PAGE_SIZE, PCI_DMA_FROMDEVICE);
			qib_release_user_pages(&tmpp[i], 1);
			tmpp[i] = NULL;
		}

		dd->pageshadow = NULL;
		vfree(tmpp);
		dd->physshadow = NULL;
		vfree(tmpd);
	}

	/*
	 * Free any resources still in use (usually just kernel contexts)
	 * at unload; we do for ctxtcnt, because that's what we allocate.
	 * We acquire lock to be really paranoid that rcd isn't being
	 * accessed from some interrupt-related code (that should not happen,
	 * but best to be sure).
	 */
	spin_lock_irqsave(&dd->uctxt_lock, flags);
	tmp = dd->rcd;
	dd->rcd = NULL;
	spin_unlock_irqrestore(&dd->uctxt_lock, flags);
	for (ctxt = 0; tmp && ctxt < dd->num_rcv_contexts; ctxt++) {
		struct qib_ctxtdata *rcd = tmp[ctxt];

		tmp[ctxt] = NULL; /* debugging paranoia */
		qib_free_ctxtdata(dd, rcd);
	}
	kfree(tmp);
	/* must follow rcv context free - need to remove rcv's hooks */
	for (ctxt = 0; ctxt < dd->num_send_contexts; ctxt++)
		sc_free(dd->send_contexts[ctxt].sc);
	kfree(dd->send_contexts);
	dd->send_contexts = NULL;
	kfree(dd->boardname);
}

/*
 * Clean up on unit shutdown, or error during unit load after
 * successful initialization.
 */
static void qib_postinit_cleanup(struct hfi_devdata *dd)
{
	/*
	 * Clean up chip-specific stuff.
	 * We check for NULL here, because it's outside
	 * the kregbase check, and we need to call it
	 * after the free_irq.  Thus it's possible that
	 * the function pointers were never initialized.
	 */
	if (dd->f_cleanup)
		dd->f_cleanup(dd);

	qib_pcie_ddcleanup(dd);

	cleanup_device_data(dd);

	qib_free_devdata(dd);
}

static int qib_init_one(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	int ret, j, pidx, initfail;
	struct hfi_devdata *dd = NULL;

	ret = qib_pcie_init(pdev, ent);
	if (ret)
		goto bail;

	/*
	 * Do device-specific initialiation, function table setup, dd
	 * allocation, etc.
	 */
	switch (ent->device) {
	case PCI_DEVICE_ID_INTEL_WFR0:
	case PCI_DEVICE_ID_INTEL_WFR1:
	case PCI_DEVICE_ID_INTEL_WFR2:
		dd = qib_init_wfr_funcs(pdev, ent);

		break;
	default:
		qib_early_err(&pdev->dev,
			"Failing on unknown Intel deviceid 0x%x\n",
			ent->device);
		ret = -ENODEV;
	}

	if (IS_ERR(dd))
		ret = PTR_ERR(dd);
	if (ret)
		goto bail; /* error already printed */

	ret = qib_create_workqueues(dd);
	if (ret)
		goto bail;

	/* do the generic initialization */
	initfail = qib_init(dd, 0);

	ret = qib_register_ib_device(dd);

	/*
	 * Now ready for use.  this should be cleared whenever we
	 * detect a reset, or initiate one.  If earlier failure,
	 * we still create devices, so diags, etc. can be used
	 * to determine cause of problem.
	 */
	if (!initfail && !ret)
		dd->flags |= QIB_INITTED;

	j = hfi_device_create(dd);
	if (j)
		dd_dev_err(dd, "Failed to create /dev devices: %d\n", -j);
	j = qibfs_add(dd);
	if (j)
		dd_dev_err(dd, "Failed filesystem setup for counters: %d\n",
			    -j);

	if (initfail || ret) {
		qib_stop_timers(dd);
		flush_workqueue(ib_wq);
		for (pidx = 0; pidx < dd->num_pports; ++pidx)
			dd->f_quiet_serdes(dd->pport + pidx);
		if (!j) {
			(void) qibfs_remove(dd);
			hfi_device_remove(dd);
		}
		if (!ret)
			qib_unregister_ib_device(dd);
		qib_postinit_cleanup(dd);
		if (initfail)
			ret = initfail;
		goto bail;
	}

	qib_verify_pioperf(dd);

	/* interrupt testing */
	if (test_interrupts) {
		void force_all_interrupts(struct hfi_devdata *dd);
		static atomic_t tested;

		if (atomic_inc_return(&tested) <= test_interrupts)
			force_all_interrupts(dd);
	}
bail:
	return ret;
}

static void qib_remove_one(struct pci_dev *pdev)
{
	struct hfi_devdata *dd = pci_get_drvdata(pdev);
	int ret;

	/* unregister from IB core */
	qib_unregister_ib_device(dd);

	/*
	 * Disable the IB link, disable interrupts on the device,
	 * clear dma engines, etc.
	 */
	qib_shutdown_device(dd);

	qib_stop_timers(dd);

	/* wait until all of our (qsfp) queue_work() calls complete */
	flush_workqueue(ib_wq);

	ret = qibfs_remove(dd);
	if (ret)
		dd_dev_err(dd, "Failed counters filesystem cleanup: %d\n",
			    -ret);

	hfi_device_remove(dd);

	qib_postinit_cleanup(dd);
}

/**
 * qib_create_rcvhdrq - create a receive header queue
 * @dd: the qlogic_ib device
 * @rcd: the context data
 *
 * This must be contiguous memory (from an i/o perspective), and must be
 * DMA'able (which means for some systems, it will go through an IOMMU,
 * or be forced into a low address range).
 */
int qib_create_rcvhdrq(struct hfi_devdata *dd, struct qib_ctxtdata *rcd)
{
	unsigned amt;
	u64 reg;

	if (!rcd->rcvhdrq) {
		dma_addr_t phys_hdrqtail;
		gfp_t gfp_flags;

		amt = ALIGN(rcd->rcvhdrq_cnt * rcd->rcvhdrqentsize *
			    sizeof(u32), PAGE_SIZE);

		gfp_flags = (rcd->ctxt >= dd->first_user_ctxt) ?
			GFP_USER : GFP_KERNEL;
		rcd->rcvhdrq = dma_alloc_coherent(
			&dd->pcidev->dev, amt, &rcd->rcvhdrq_phys,
			gfp_flags | __GFP_COMP);

		if (!rcd->rcvhdrq) {
			dd_dev_err(dd,
				"attempt to allocate %d bytes for ctxt %u rcvhdrq failed\n",
				amt, rcd->ctxt);
			goto bail;
		}

		/* Event mask is per device now and is in hfi_devdata */
		/*if (rcd->ctxt >= dd->first_user_ctxt) {
			rcd->user_event_mask = vmalloc_user(PAGE_SIZE);
			if (!rcd->user_event_mask)
				goto bail_free_hdrq;
				}*/

		if (!(dd->flags & QIB_NODMA_RTAIL)) {
			rcd->rcvhdrtail_kvaddr = dma_alloc_coherent(
				&dd->pcidev->dev, PAGE_SIZE, &phys_hdrqtail,
				gfp_flags);
			if (!rcd->rcvhdrtail_kvaddr)
				goto bail_free;
			rcd->rcvhdrqtailaddr_phys = phys_hdrqtail;
		}

		rcd->rcvhdrq_size = amt;
	}

	/* clear for security and sanity on each use */
	memset(rcd->rcvhdrq, 0, rcd->rcvhdrq_size);
	if (rcd->rcvhdrtail_kvaddr)
		memset(rcd->rcvhdrtail_kvaddr, 0, PAGE_SIZE);

	/*
	 * These values are per-context:
	 *	RcvHdrCnt
	 *	RcvHdrEntSize
	 *	RcvHdrSize
	 */
	reg = (rcd->rcvhdrq_cnt & WFR_RCV_HDR_CNT_CNT_MASK)
		<< WFR_RCV_HDR_CNT_CNT_SHIFT;
	write_kctxt_csr(dd, rcd->ctxt, WFR_RCV_HDR_CNT, reg);
	reg = (rcd->rcvhdrqentsize & WFR_RCV_HDR_ENT_SIZE_ENT_SIZE_MASK)
		<< WFR_RCV_HDR_ENT_SIZE_ENT_SIZE_SHIFT;
	write_kctxt_csr(dd, rcd->ctxt, WFR_RCV_HDR_ENT_SIZE, reg);
	reg = (dd->rcvhdrsize & WFR_RCV_HDR_SIZE_HDR_SIZE_MASK)
		<< WFR_RCV_HDR_SIZE_HDR_SIZE_SHIFT;
	write_kctxt_csr(dd, rcd->ctxt, WFR_RCV_HDR_SIZE, reg);
	return 0;

bail_free:
	dd_dev_err(dd,
		"attempt to allocate 1 page for ctxt %u rcvhdrqtailaddr failed\n",
		rcd->ctxt);
	vfree(rcd->user_event_mask);
	rcd->user_event_mask = NULL;
	dma_free_coherent(&dd->pcidev->dev, amt, rcd->rcvhdrq,
			  rcd->rcvhdrq_phys);
	rcd->rcvhdrq = NULL;
bail:
	return -ENOMEM;
}

/**
 * allocate eager buffers, both kernel and user contexts.
 * @rcd: the context we are setting up.
 *
 * Allocate the eager TID buffers and program them into hip.
 * They are no longer completely contiguous, we do multiple allocation
 * calls.  Otherwise we get the OOM code involved, by asking for too
 * much per call, with disastrous results on some kernels.
 */
int qib_setup_eagerbufs(struct qib_ctxtdata *rcd)
{
	struct hfi_devdata *dd = rcd->dd;
	unsigned e, egrcnt, egrperchunk, chunk, egrsize, egroff;
	size_t size;
	gfp_t gfp_flags;

	/*
	 * GFP_USER, but without GFP_FS, so buffer cache can be
	 * coalesced (we hope); otherwise, even at order 4,
	 * heavy filesystem activity makes these fail, and we can
	 * use compound pages.
	 */
	gfp_flags = __GFP_WAIT | __GFP_IO | __GFP_COMP;

	egrcnt = rcd->eager_count;
	egroff = rcd->eager_base;
	egrsize = rcd->rcvegrbuf_size;

	chunk = rcd->rcvegrbuf_chunks;
	egrperchunk = rcd->rcvegrbufs_perchunk;
	size = rcd->rcvegrbuf_chunksize;
	if (!rcd->rcvegrbuf) {
		rcd->rcvegrbuf =
			kzalloc(chunk * sizeof(rcd->rcvegrbuf[0]),
				GFP_KERNEL);
		if (!rcd->rcvegrbuf)
			goto bail;
	}
	if (!rcd->rcvegrbuf_phys) {
		rcd->rcvegrbuf_phys =
			kmalloc(chunk * sizeof(rcd->rcvegrbuf_phys[0]),
				GFP_KERNEL);
		if (!rcd->rcvegrbuf_phys)
			goto bail_rcvegrbuf;
	}
	for (e = 0; e < rcd->rcvegrbuf_chunks; e++) {
		if (rcd->rcvegrbuf[e])
			continue;
		rcd->rcvegrbuf[e] =
			dma_alloc_coherent(&dd->pcidev->dev, size,
					   &rcd->rcvegrbuf_phys[e],
					   gfp_flags);
		if (!rcd->rcvegrbuf[e])
			goto bail_rcvegrbuf_phys;
	}

	rcd->rcvegr_phys = rcd->rcvegrbuf_phys[0];

	/*
	 * If multiple packets per RcvArray buffer is enabled in
	 * the context's RcvCtrl register, use one RcvArray TID
	 * per chunk.
	 */
	if (rcd->rcvctrl & WFR_RCV_CTXT_CTRL_ONE_PACKET_PER_EGR_BUFFER_SMASK)
		egrperchunk = 1;
			
	for (e = chunk = 0; chunk < rcd->rcvegrbuf_chunks; chunk++) {
		dma_addr_t pa = rcd->rcvegrbuf_phys[chunk];
		unsigned i;

		/* clear for security and sanity on each use */
		memset(rcd->rcvegrbuf[chunk], 0, size);

		for (i = 0; e < egrcnt && i < egrperchunk; e++, i++) {
			dd->f_put_tid(dd, e + egroff, PT_EAGER, pa);
			pa += egrsize;
		}
		cond_resched(); /* don't hog the cpu */
	}

	return 0;

bail_rcvegrbuf_phys:
	for (e = 0; e < rcd->rcvegrbuf_chunks && rcd->rcvegrbuf[e]; e++)
		dma_free_coherent(&dd->pcidev->dev, size,
				  rcd->rcvegrbuf[e], rcd->rcvegrbuf_phys[e]);
	kfree(rcd->rcvegrbuf_phys);
	rcd->rcvegrbuf_phys = NULL;
bail_rcvegrbuf:
	kfree(rcd->rcvegrbuf);
	rcd->rcvegrbuf = NULL;
bail:
	return -ENOMEM;
}
