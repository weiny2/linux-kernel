/*
 * Copyright (c) 2012 - 2015 Intel Corporation.  All rights reserved.
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
#include <linux/hrtimer.h>

#include "hfi.h"
#include "device.h"
#include "common.h"
#include "mad.h"
#include "sdma.h"
#include "debugfs.h"
#include "verbs.h"

#undef pr_fmt
#define pr_fmt(fmt) DRIVER_NAME ": " fmt

/*
 * min buffers we want to have per context, after driver
 */
#define QIB_MIN_USER_CTXT_BUFCNT 7

#define QLOGIC_IB_R_SOFTWARE_MASK 0xFF
#define QLOGIC_IB_R_SOFTWARE_SHIFT 24
#define QLOGIC_IB_R_EMULATOR_MASK (1ULL<<62)

#define HFI_MIN_HDRQ_EGRBUF_CNT 2
#define HFI_MIN_EAGER_BUFFER_SIZE (4 * 1024) /* 4KB */
#define HFI_MAX_EAGER_BUFFER_SIZE (256 * 1024) /* 256KB */

/*
 * Number of receive contexts we are configured to use (to allow for more pio
 * buffers per ctxt, etc.)  Zero means use chip value.
 */
uint num_rcv_contexts;
module_param_named(num_rcv_contexts, num_rcv_contexts, uint, S_IRUGO);
MODULE_PARM_DESC(num_rcv_contexts, "Set max number of receive contexts to use");

u8 krcvqs[WFR_RXE_NUM_DATA_VL];
int krcvqsset;
module_param_array(krcvqs, byte, &krcvqsset, S_IRUGO);
MODULE_PARM_DESC(krcvqs, "Array of the number of kernel receive queues by VL");

/* computed based on above array */
unsigned n_krcvqs;

/* interrupt testing */
static unsigned int test_interrupts;
module_param_named(test_interrupts, test_interrupts, uint, S_IRUGO);

static unsigned hfi_rcvarr_split = 25;
module_param_named(rcvarr_split, hfi_rcvarr_split, uint, S_IRUGO);
MODULE_PARM_DESC(rcvarr_split, "Percent of context's RcvArray entries used for Eager buffers");

static uint eager_buffer_size = (2 << 20); /* 2MB */
module_param(eager_buffer_size, uint, S_IRUGO);
MODULE_PARM_DESC(eager_buffer_size, "Size of the eager buffers, default: 2MB");

static uint rcvhdrcnt = 2048; /* 2x the max eager buffer count */
module_param_named(rcvhdrcnt, rcvhdrcnt, uint, S_IRUGO);
MODULE_PARM_DESC(rcvhdrcnt, "Receive header queue count (default 2048)");

static uint hfi_hdrq_entsize = 32;
module_param_named(hdrq_entsize, hfi_hdrq_entsize, uint, S_IRUGO);
MODULE_PARM_DESC(hdrq_entsize, "Size of header queue entries: 2 - 8B, 16 - 64B (default), 32 - 128B");

unsigned int user_credit_return_threshold = 33;	/* default is 33% */
module_param(user_credit_return_threshold, uint, S_IRUGO);
MODULE_PARM_DESC(user_credit_return_theshold, "Credit return threshold for user send contexts, return when unreturned credits passes this many blocks (in percent of allocated blocks, 0 is off)");

static inline u64 encode_rcv_header_entry_size(u16);

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
	int local_node_id = pcibus_to_node(dd->pcidev->bus);

	if (local_node_id < 0)
		local_node_id = numa_node_id();
	dd->assigned_node_id = local_node_id;

	dd->rcd = kcalloc(dd->num_rcv_contexts, sizeof(*dd->rcd), GFP_KERNEL);
	if (!dd->rcd) {
		dd_dev_err(dd,
			"Unable to allocate receive context array, failing\n");
		goto nomem;
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
			goto nomem;
		}
		/*
		 * Set up the kernel context flags here and now because they
		 * use default values for all receive side memories.  User
		 * contexts will be handled as they are created.
		 */
		rcd->flags = HFI_CAP_KGET(MULTI_PKT_EGR) |
			HFI_CAP_KGET(NODROP_RHQ_FULL) |
			HFI_CAP_KGET(NODROP_EGR_FULL) |
			HFI_CAP_KGET(DMA_RTAIL);
		ret = dd->f_init_ctxt(rcd);
		if (ret < 0) {
			dd_dev_err(dd,
				   "Failed to setup kernel receive context, failing\n");
			dd->rcd[rcd->ctxt] = NULL;
			qib_free_ctxtdata(dd, rcd);
			ret = -EFAULT;
			goto bail;
		}
		rcd->seq_cnt = 1;

		rcd->sc = sc_alloc(dd, SC_ACK, rcd->rcvhdrqentsize, dd->node);
		if (!rcd->sc) {
			dd_dev_err(dd,
				"Unable to allocate kernel send context, failing\n");
			goto nomem;
		}
	}

	return 0;
nomem:
	ret = -ENOMEM;
bail:
	kfree(dd->rcd);
	dd->rcd = NULL;
	return ret;
}

/*
 * Common code for user and kernel context setup.
 */
struct qib_ctxtdata *qib_create_ctxtdata(struct qib_pportdata *ppd, u32 ctxt)
{
	struct hfi_devdata *dd = ppd->dd;
	struct qib_ctxtdata *rcd;
	unsigned kctxt_ngroups = 0;
	u32 base;

	if (dd->rcv_entries.nctxt_extra >
	    dd->num_rcv_contexts - dd->first_user_ctxt)
		kctxt_ngroups = (dd->rcv_entries.nctxt_extra -
				 (dd->num_rcv_contexts - dd->first_user_ctxt));
	rcd = kzalloc(sizeof(*rcd), GFP_KERNEL);
	if (rcd) {
		u32 rcvtids, max_entries;

		dd_dev_info(dd, "%s: setting up context %u\n", __func__, ctxt);

		INIT_LIST_HEAD(&rcd->qp_wait_list);
		rcd->ppd = ppd;
		rcd->dd = dd;
		rcd->cnt = 1;
		rcd->ctxt = ctxt;
		dd->rcd[ctxt] = rcd;
		rcd->numa_id = numa_node_id();
		rcd->rcv_array_groups = dd->rcv_entries.ngroups;

		spin_lock_init(&rcd->exp_lock);

		/*
		 * Calculate the context's RcvArray entry starting point.
		 * We do this here because we have to take into account all
		 * the RcvArray entries that previous context would have
		 * taken and we have to account for any extra groups
		 * assigned to the kernel or user contexts.
		 */
		if (ctxt < dd->first_user_ctxt) {
			if (ctxt < kctxt_ngroups) {
				base = ctxt * (dd->rcv_entries.ngroups + 1);
				rcd->rcv_array_groups++;
			} else
				base = kctxt_ngroups +
					(ctxt * dd->rcv_entries.ngroups);
		} else {
			u16 ct = ctxt - dd->first_user_ctxt;

			base = ((dd->n_krcv_queues * dd->rcv_entries.ngroups) +
				kctxt_ngroups);
			if (ct < dd->rcv_entries.nctxt_extra) {
				base += ct * (dd->rcv_entries.ngroups + 1);
				rcd->rcv_array_groups++;
			} else
				base += dd->rcv_entries.nctxt_extra +
					(ct * dd->rcv_entries.ngroups);
		}
		rcd->eager_base = base * dd->rcv_entries.group_size;

		/* Validate and initialize Rcv Hdr Q variables */
		if (rcvhdrcnt & WFR_HDRQ_INCREMENT) {
			dd_dev_err(dd,
				   "ctxt%u: header queue count %d must be divisible by %d\n",
				   rcd->ctxt, rcvhdrcnt, WFR_HDRQ_INCREMENT);
			goto bail;
		}
		rcd->rcvhdrq_cnt = rcvhdrcnt;
		rcd->rcvhdrqentsize = hfi_hdrq_entsize;
		/*
		 * Simple Eager buffer allocation: we have already pre-allocated
		 * the number of RcvArray entry groups. Each ctxtdata structure
		 * holds the number of groups for that context.
		 *
		 * To follow CSR requirements and maintain cacheline alignment,
		 * make sure all sizes and bases are multiples of group_size.
		 *
		 * The expected entry count is what is left after assigning
		 * eager.
		 */
		max_entries = rcd->rcv_array_groups *
			dd->rcv_entries.group_size;
		rcvtids = ((max_entries * hfi_rcvarr_split) / 100);
		rcd->egrbufs.count = round_down(rcvtids,
						dd->rcv_entries.group_size);
		if (rcd->egrbufs.count > WFR_MAX_EAGER_ENTRIES) {
			dd_dev_err(dd, "ctxt%u: requested too many RcvArray entries.\n",
				   rcd->ctxt);
			rcd->egrbufs.count = WFR_MAX_EAGER_ENTRIES;
		}
		dd_dev_info(dd, "ctxt%u: max Eager buffer RcvArray entries: %u\n",
			    rcd->ctxt, rcd->egrbufs.count);

		/*
		 * Allocate array that will hold the eager buffer accounting
		 * data.
		 * This will allocate the maximum possible buffer count based
		 * on the value of the RcvArray split parameter.
		 * The resulting value will be rounded down to the closest
		 * multiple of dd->rcv_entries.group_size.
		 */
		rcd->egrbufs.buffers = kzalloc(sizeof(*rcd->egrbufs.buffers) *
					       rcd->egrbufs.count, GFP_KERNEL);
		if (!rcd->egrbufs.buffers)
			goto bail;
		rcd->egrbufs.rcvtids = kzalloc(sizeof(*rcd->egrbufs.rcvtids) *
					       rcd->egrbufs.count, GFP_KERNEL);
		if (!rcd->egrbufs.rcvtids)
			goto bail;
		rcd->egrbufs.size = eager_buffer_size;
		/*
		 * The size of the buffers programmed into the RcvArray
		 * entries needs to be big enough to handle the highest
		 * MTU supported.
		 */
		if (rcd->egrbufs.size < max_mtu) {
			rcd->egrbufs.size = __roundup_pow_of_two(max_mtu);
			dd_dev_info(dd,
				    "ctxt%u: eager bufs size too small. Adjusting to %zu\n",
				    rcd->ctxt, rcd->egrbufs.size);
		}
		rcd->egrbufs.rcvtid_size = HFI_MAX_EAGER_BUFFER_SIZE;

		if (ctxt < dd->first_user_ctxt) { /* N/A for PSM contexts */
			rcd->opstats = kzalloc(sizeof(*rcd->opstats),
				GFP_KERNEL);
			if (!rcd->opstats) {
				dd_dev_err(dd,
					   "ctxt%u: Unable to allocate per ctxt stats buffer\n",
					   rcd->ctxt);
				goto bail;
			}
		}
	}
	return rcd;
bail:
	kfree(rcd->opstats);
	kfree(rcd->egrbufs.rcvtids);
	kfree(rcd->egrbufs.buffers);
	kfree(rcd);
	return NULL;
}

/*
 * Convert a receive header entry size that to the encoding used in the CSR.
 *
 * Return a zero if the given size is invalid.
 */
static inline u64 encode_rcv_header_entry_size(u16 size)
{
	/* there are only 3 valid receive header entry sizes */
	if (size == 2)
		return 1;
	if (size == 16)
		return 2;
	else if (size == 32)
		return 4;
	return 0; /* invalid */
}

/*
 * Select the largest ccti value over all SLs to determine the intra-
 * packet gap for the link.
 *
 * called with cca_timer_lock held (to protect access to cca_timer
 * array), and rcu_read_lock() (to protect access to cc_state).
 */
void set_link_ipg(struct qib_pportdata *ppd)
{
	struct hfi_devdata *dd = ppd->dd;
	struct cc_state *cc_state;
	int i;
	u16 cce, ccti_limit, max_ccti = 0;
	u16 shift, mult;
	u64 src;
	u16 link_speed = ppd->link_speed_active;
	u16 link_width = ppd->link_width_active;
	u32 pkt_egress_rate; /* Mbits /sec */
	u32 max_pkt_time;
	/*
	 * max_pkt_time is the maximum packet egress time in units
	 * of the fabric clock period 1/(805 MHz).
	 */

	cc_state = get_cc_state(ppd);

	if (cc_state == NULL)
		/*
		 * This should _never_ happen - rcu_read_lock() is held,
		 * and set_link_ipg() should not be called if cc_state
		 * is NULL.
		 */
		return;

	for (i = 0; i < OPA_MAX_SLS; i++) {
		u16 ccti = ppd->cca_timer[i].ccti;

		if (ccti > max_ccti)
			max_ccti = ccti;
	}

	ccti_limit = cc_state->cct.ccti_limit;
	if (max_ccti > ccti_limit)
		max_ccti = ccti_limit;

	cce = cc_state->cct.entries[max_ccti].entry;
	shift = (cce & 0xc000) >> 14;
	mult = (cce & 0x3fff);

	if (link_speed == OPA_LINK_SPEED_25G)
		pkt_egress_rate = 25000;
	else /* assume OPA_LINK_SPEED_12_5G */
		pkt_egress_rate = 12500;

	switch (link_width) {
	case OPA_LINK_WIDTH_4X:
		pkt_egress_rate *= 4;
		break;
	case OPA_LINK_WIDTH_3X:
		pkt_egress_rate *= 3;
		break;
	case OPA_LINK_WIDTH_2X:
		pkt_egress_rate *= 2;
		break;
	default:
		/* assume IB_WIDTH_1X */
		break;
	}

	/*
	 * max_pkt_time is:
	 *
	 *  (max_packet_length) [bits] / (pkt_egress_rate) [bits/sec]
	 *  ---------------------------------------------------------
	 *    fabric_clock_period == (1 / 805 * 10^6) [1/sec]
	 */
	max_pkt_time = (ppd->ibmaxlen * 8);
	max_pkt_time *= 805;
	max_pkt_time /= pkt_egress_rate;

	src = (max_pkt_time >> shift) * mult;

	src &= WFR_SEND_STATIC_RATE_CONTROL_CSR_SRC_RELOAD_SMASK;
	src <<= WFR_SEND_STATIC_RATE_CONTROL_CSR_SRC_RELOAD_SHIFT;

	write_csr(dd, WFR_SEND_STATIC_RATE_CONTROL, src);
}

static enum hrtimer_restart cca_timer_fn(struct hrtimer *t)
{
	struct cca_timer *cca_timer;
	struct qib_pportdata *ppd;
	int sl;
	u16 ccti, ccti_timer, ccti_min;
	struct cc_state *cc_state;

	cca_timer = container_of(t, struct cca_timer, hrtimer);
	ppd = cca_timer->ppd;
	sl = cca_timer->sl;

	rcu_read_lock();

	cc_state = get_cc_state(ppd);

	if (cc_state == NULL) {
		rcu_read_unlock();
		return HRTIMER_NORESTART;
	}

	/*
	 * 1) decrement ccti for SL
	 * 2) calculate IPG for link (set_link_ipg())
	 * 3) restart timer, unless ccti is at min value
	 */

	ccti_min = cc_state->cong_setting.entries[sl].ccti_min;
	ccti_timer = cc_state->cong_setting.entries[sl].ccti_timer;

	spin_lock(&ppd->cca_timer_lock);

	ccti = cca_timer->ccti;

	if (ccti > ccti_min) {
		cca_timer->ccti--;
		set_link_ipg(ppd);
	}

	spin_unlock(&ppd->cca_timer_lock);

	rcu_read_unlock();

	if (ccti > ccti_min) {
		unsigned long nsec = 1024 * ccti_timer;
		/* ccti_timer is in units of 1.024 usec */
		hrtimer_forward_now(t, ns_to_ktime(nsec));
		return HRTIMER_RESTART;
	}
	return HRTIMER_NORESTART;
}

/*
 * Common code for initializing the physical port structure.
 */
void qib_init_pportdata(struct pci_dev *pdev, struct qib_pportdata *ppd,
			struct hfi_devdata *dd, u8 hw_pidx, u8 port)
{
	int i, size;
	uint default_pkey_idx;

	ppd->dd = dd;
	ppd->hw_pidx = hw_pidx;
	ppd->port = port; /* IB port number, not index */

	default_pkey_idx = 1;

	ppd->pkeys[default_pkey_idx] = WFR_DEFAULT_P_KEY;
	if (loopback) {
		qib_early_err(&pdev->dev,
			      "Faking data partition 0x8001 in idx %u\n",
			      !default_pkey_idx);
		ppd->pkeys[!default_pkey_idx] = 0x8001;
	}

	INIT_WORK(&ppd->link_vc_work, handle_verify_cap);
	INIT_WORK(&ppd->link_up_work, handle_link_up);
	INIT_WORK(&ppd->link_down_work, handle_link_down);
	INIT_WORK(&ppd->freeze_work, handle_freeze);
	INIT_WORK(&ppd->link_downgrade_work, handle_link_downgrade);
	INIT_WORK(&ppd->sma_message_work, handle_sma_message);
	mutex_init(&ppd->hls_lock);
	spin_lock_init(&ppd->sdma_alllock);
	spin_lock_init(&ppd->qsfp_info.qsfp_lock);

	ppd->sm_trap_qp = 0x0;
	ppd->sa_qp = 0x1;

	ppd->qib_wq = NULL;

	spin_lock_init(&ppd->cca_timer_lock);

	for (i = 0; i < OPA_MAX_SLS; i++) {
		hrtimer_init(&ppd->cca_timer[i].hrtimer, CLOCK_MONOTONIC,
			     HRTIMER_MODE_REL);
		ppd->cca_timer[i].ppd = ppd;
		ppd->cca_timer[i].sl = i;
		ppd->cca_timer[i].ccti = 0;
		ppd->cca_timer[i].hrtimer.function = cca_timer_fn;
	}

	ppd->cc_max_table_entries = IB_CC_TABLE_CAP_DEFAULT;

	spin_lock_init(&ppd->cc_state_lock);
	spin_lock_init(&ppd->cc_log_lock);
	size = sizeof(struct cc_state);
	ppd->cc_state = kzalloc(size, GFP_KERNEL);
	if (!ppd->cc_state)
		goto bail;
	return;

bail:

	qib_early_err(&pdev->dev,
		      "Congestion Control Agent disabled for port %d\n", port);
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
	for (i = 0; i < dd->num_rcv_contexts; i++)
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
	for (i = 0; i < dd->first_user_ctxt; ++i) {
		rcvmask |= HFI_CAP_KGET_MASK(dd->rcd[i]->flags, DMA_RTAIL) ?
			QIB_RCVCTRL_TAILUPD_ENB : QIB_RCVCTRL_TAILUPD_DIS;
		if (!HFI_CAP_KGET_MASK(dd->rcd[i]->flags, MULTI_PKT_EGR))
			rcvmask |= QIB_RCVCTRL_ONE_PKT_EGR_ENB;
		if (HFI_CAP_KGET_MASK(dd->rcd[i]->flags, NODROP_RHQ_FULL))
			rcvmask |= QIB_RCVCTRL_NO_RHQ_DROP_ENB;
		if (HFI_CAP_KGET_MASK(dd->rcd[i]->flags, NODROP_EGR_FULL))
			rcvmask |= QIB_RCVCTRL_NO_EGR_DROP_ENB;
		dd->f_rcvctrl(dd, rcvmask, i);
		/* XXX (Mitko): Do we care about the result of this?
		 * sc_enable() will display an error message. */
		sc_enable(dd->rcd[i]->sc);
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
	unsigned i, len;
	struct qib_ctxtdata *rcd;
	struct qib_pportdata *ppd;

	/* Set up recv low level handlers */
	rhf_rcv_function_map[RHF_RCV_TYPE_IB] = process_receive_ib;
	rhf_rcv_function_map[RHF_RCV_TYPE_BYPASS] = process_receive_bypass;
	rhf_rcv_function_map[RHF_RCV_TYPE_ERROR] = process_receive_error;
	rhf_rcv_function_map[RHF_RCV_TYPE_EXPECTED] = process_receive_expected;
	rhf_rcv_function_map[RHF_RCV_TYPE_EAGER] = process_receive_eager;

	/* Set up send low level handlers */
	dd->process_pio_send = qib_verbs_send_pio;
	dd->process_dma_send = qib_verbs_send_dma;
	dd->pio_inline_send = pio_copy;

	/*
	 * A0 erratum 291500: Here due to ASIC reset
	 * power-on. The first packet to be recieved
	 * in the future might be corrupted. Mark it
	 * to be dropped.
	 */
	if (is_a0(dd)) {
		atomic_set(&dd->drop_packet, DROP_PACKET_ON);
		dd->do_drop = 1;
	} else {
		atomic_set(&dd->drop_packet, DROP_PACKET_OFF);
		dd->do_drop = 0;
	}

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
		if (lastfail)
			dd_dev_err(dd,
				"failed to allocate kernel ctxt's rcvhdrq and/or egr bufs\n");
	}
	if (lastfail)
		ret = lastfail;

	/* Allocate enough memory for user event notifiction. */
	len = ALIGN(dd->chip_rcv_contexts * HFI_MAX_SHARED_CTXTS *
		    sizeof(*dd->events), PAGE_SIZE);
	dd->events = vmalloc_user(len);
	if (!dd->events)
		dd_dev_err(dd, "Failed to allocate user events page\n");
	/*
	 * Allocate a page for device and port status.
	 * Page will be shared amongst all user proceses.
	 */
	dd->status = vmalloc_user(PAGE_SIZE);
	if (!dd->status)
		dd_dev_err(dd, "Failed to allocate dev status page\n");
	else
		dd->freezelen = PAGE_SIZE - (sizeof(*dd->status) -
					     sizeof(dd->status->freezemsg));
	for (pidx = 0; pidx < dd->num_pports; ++pidx) {
		ppd = dd->pport + pidx;
		if (dd->status)
			/* Currently, we only have one port */
			ppd->statusp = &dd->status->port;

		set_mtu(ppd);
	}

	/* enable chip even if we have an error, so we can debug cause */
	enable_chip(dd);

	ret = qib_cq_init(dd);
done:
	/*
	 * Set status even if port serdes is not initialized
	 * so that diags will work.
	 */
	if (dd->status)
		dd->status->dev |= HFI_STATUS_CHIP_PRESENT |
			HFI_STATUS_INITTED;
	if (!ret) {
		/* enable all interrupts from the chip */
		set_intr_state(dd, 1);

		/* chip is OK for user apps; mark it as initialized */
		for (pidx = 0; pidx < dd->num_pports; ++pidx) {
			ppd = dd->pport + pidx;

			/* initialize the qsfp if it exists
			 * Requires interrupts to be enabled so we are notified
			 * when the QSFP completes reset, and has
			 * to be done before bringing up the SERDES
			 * TODO: Call init_qsfp only if QIB_HAS_QSFP
			 * is set in ppd->qsfp_info.flags
			 */
			init_qsfp(ppd);

			/* start the serdes - must be after interrupts are
			   enabled so we are notified when the link goes up */
			lastfail = bringup_serdes(ppd);
			if (lastfail)
				dd_dev_info(dd,
					"Failed to bring up port %u\n",
					ppd->port);

			/*
			 * Set status even if port serdes is not initialized
			 * so that diags will work.
			 */
			if (ppd->statusp)
				*ppd->statusp |= HFI_STATUS_CHIP_PRESENT |
							HFI_STATUS_INITTED;
			if (!ppd->link_speed_enabled)
				continue;
		}
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

	for (pidx = 0; pidx < dd->num_pports; ++pidx) {
		ppd = dd->pport + pidx;
		if (ppd->led_override_timer.data) {
			del_timer_sync(&ppd->led_override_timer);
			atomic_set(&ppd->led_override_timer_active, 0);
		}
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
			*ppd->statusp &= ~(HFI_STATUS_IB_CONF |
					   HFI_STATUS_IB_READY);
	}
	dd->flags &= ~HFI_INITTED;

	/* mask interrupts, but not errors */
	set_intr_state(dd, 0);

	for (pidx = 0; pidx < dd->num_pports; ++pidx) {
		ppd = dd->pport + pidx;
		for (i = 0; i < dd->num_rcv_contexts; i++)
			dd->f_rcvctrl(dd, QIB_RCVCTRL_TAILUPD_DIS |
					  QIB_RCVCTRL_CTXT_DIS |
					  QIB_RCVCTRL_INTRAVAIL_DIS |
					  QIB_RCVCTRL_PKEY_DIS |
					  QIB_RCVCTRL_ONE_PKT_EGR_DIS, i);
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
	sdma_exit(dd);
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
					  (void *)rcd->rcvhdrtail_kvaddr,
					  rcd->rcvhdrqtailaddr_phys);
			rcd->rcvhdrtail_kvaddr = NULL;
		}
	}

	if (rcd->egrbufs.rcvtids)
		/* all the RcvArray entries should have been cleared by now */
		kfree(rcd->egrbufs.rcvtids);

	if (rcd->egrbufs.buffers) {
		unsigned e;

		for (e = 0; e < rcd->egrbufs.alloced; e++) {
			if (rcd->egrbufs.buffers[e].phys)
				dma_free_coherent(&dd->pcidev->dev,
						  rcd->egrbufs.buffers[e].len,
						  rcd->egrbufs.buffers[e].addr,
						  rcd->egrbufs.buffers[e].phys);
		}
		kfree(rcd->egrbufs.buffers);
	}

	if (rcd->sc) {
		sc_free(rcd->sc);
		rcd->sc = NULL;
	}
	vfree(rcd->physshadow);
	vfree(rcd->tid_pg_list);
	vfree(rcd->user_event_mask);
	vfree(rcd->subctxt_uregbase);
	vfree(rcd->subctxt_rcvegrbuf);
	vfree(rcd->subctxt_rcvhdr_base);
	kfree(rcd->tidusemap);
	kfree(rcd->opstats);
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

/* FIXME: always zero so we return early */
static int fake_early_return;

static void qib_verify_pioperf(struct hfi_devdata *dd)
{
	struct send_context *sc;
	struct pio_buf *pbuf;
	u64 pbc, msecs, emsecs;
	u32 cnt, lcnt, dw;
	u32 *addr;

	if (dd->num_send_contexts == 0) {
		dd_dev_info(dd,
			"Performance check: No contexts for checking perf, skipping\n");
		return;
	}

	/* FIXME: not ready yet */
	if (fake_early_return == 0)
		return;

	/* only run this on unit 0 */
	if (dd->unit != 0)
		return;

	/* TEMPORARY: force a non-context PIO error and see what happens... */
	/* write_csr(dd, WFR_SEND_PIO_ERR_FORCE,
		WFR_SEND_PIO_ERR_FORCE_PIO_WRITE_BAD_CTXT_ERR_SMASK); */

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
			 "Performance check: Could not get memory for checking PIO perf, skipping\n");
		return;
	}

	/* zero'ed lrh/bth above will generate an error, disable it */
	dd->f_set_armlaunch(dd, 0);

	/* minimal PBC - just the length in DW */
	pbc = create_pbc(0, 0, 0, dw);

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
		pbuf = sc_buffer_alloc(sc, dw, NULL, NULL);
		if (!pbuf) {
			preempt_enable();
			dd_dev_info(dd,
				"Performance check: PIO buffer allocation failure at count %u, skipping\n",
				lcnt);
			goto done;
		}
		pio_copy(dd, pbuf, pbc, addr, cnt >> 2);
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
	hfi_dbg_ibdev_exit(&dd->verbs_dev);
	rcu_barrier(); /* wait for rcu callbacks to complete */
	free_percpu(dd->int_counter);
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
	if (!dd)
		return ERR_PTR(-ENOMEM);
	/* extra is * number of ports */
	dd->num_pports = extra/sizeof(struct qib_pportdata);
	dd->pport = (struct qib_pportdata *)(dd + 1);

	INIT_LIST_HEAD(&dd->list);
	dd->node = dev_to_node(&pdev->dev);
	if (dd->node < 0)
		dd->node = 0;
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
		goto bail;
	}
	/*
	 * Initialize all locks for the device. This needs to be as early as
	 * possible so locks are usable.
	 */
	spin_lock_init(&dd->sc_lock);
	spin_lock_init(&dd->sendctrl_lock);
	spin_lock_init(&dd->uctxt_lock);
	spin_lock_init(&dd->qib_diag_trans_lock);
	spin_lock_init(&dd->sc_init_lock);
	spin_lock_init(&dd->dc8051_lock);
	mutex_init(&dd->qsfp_i2c_mutex);
	seqlock_init(&dd->sc2vl_lock);
	spin_lock_init(&dd->sde_map_lock);
	init_waitqueue_head(&dd->event_queue);

	dd->int_counter = alloc_percpu(u64);
	if (!dd->int_counter) {
		ret = -ENOMEM;
		qib_early_err(&pdev->dev,
			      "Could not allocate per-cpu int_counter\n");
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
	hfi_dbg_ibdev_init(&dd->verbs_dev);
	return dd;

bail:
	if (!list_empty(&dd->list))
		list_del_init(&dd->list);
	ib_dealloc_device(&dd->verbs_dev.ibdev);
	return ERR_PTR(ret);
}

/*
 * Called from freeze mode handlers, and from PCI error
 * reporting code.  Should be paranoid about state of
 * system and data structures.
 */
void qib_disable_after_error(struct hfi_devdata *dd)
{
	if (dd->flags & HFI_INITTED) {
		u32 pidx;

		dd->flags &= ~HFI_INITTED;
		if (dd->pport)
			for (pidx = 0; pidx < dd->num_pports; ++pidx) {
				struct qib_pportdata *ppd;

				ppd = dd->pport + pidx;
				if (dd->flags & HFI_PRESENT) {
					set_link_state(ppd, HLS_DN_DISABLE);
					dd->f_setextled(ppd, 0);
				}
				if (ppd->statusp)
					*ppd->statusp &= ~HFI_STATUS_IB_READY;
			}
	}

	/*
	 * Mark as having had an error for driver, and also
	 * for /sys and status word mapped to user programs.
	 * This marks unit as not usable, until reset.
	 */
	if (dd->status)
		dd->status->dev |= HFI_STATUS_HWERROR;
}

static void qib_remove_one(struct pci_dev *);
static int qib_init_one(struct pci_dev *, const struct pci_device_id *);

#define DRIVER_LOAD_MSG "Intel " DRIVER_NAME " loaded: "
#define PFX DRIVER_NAME ": "

static const struct pci_device_id qib_pci_tbl[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_WFR0) },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_WFR1) },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_WFR2) },
	{ 0, }
};

MODULE_DEVICE_TABLE(pci, qib_pci_tbl);

static struct pci_driver qib_driver = {
	.name = DRIVER_NAME,
	.probe = qib_init_one,
	.remove = qib_remove_one,
	.id_table = qib_pci_tbl,
	.err_handler = &qib_pci_err_handler,
};

static void __init compute_krcvqs(void)
{
	int i;

	for (i = 0; i < krcvqsset; i++)
		n_krcvqs += krcvqs[i];
}

/*
 * Do all the generic driver unit- and chip-independent memory
 * allocation and initialization.
 */
static int __init qib_ib_init(void)
{
	int ret;

	ret = dev_init();
	if (ret)
		goto bail;

	/* validate max and default MTUs before any devices start */
	if (!valid_stl_mtu(default_mtu))
		default_mtu = HFI_DEFAULT_ACTIVE_MTU;
	if (!valid_stl_mtu(max_mtu))
		max_mtu = HFI_DEFAULT_MAX_MTU;
	/* valid CUs run from 1-128 in powers of 2 */
	if (hfi_cu > 128 || !is_power_of_2(hfi_cu))
		hfi_cu = 1;
	/* valid credit return threshold is 0-100, variable is unsigned */
	if (user_credit_return_threshold > 100)
		user_credit_return_threshold = 100;

	compute_krcvqs();
	/* sanitize receive interrupt count, time must wait until after
	   the hardware type is known */
	if (rcv_intr_count > WFR_RCV_HDR_HEAD_COUNTER_MASK)
		rcv_intr_count = WFR_RCV_HDR_HEAD_COUNTER_MASK;
	/* reject invalid combinations */
	if (rcv_intr_count == 0 && rcv_intr_timeout == 0) {
		pr_err("Invalid mode: both receive interrupt count and available timeout are zero - setting interrupt count to 1\n");
		rcv_intr_count = 1;
	}
	if (rcv_intr_count > 1 && rcv_intr_timeout == 0) {
		/*
		 * Avoid indefinite packet delivery by requiring a timeout
		 * if count is > 1.
		 */
		pr_err("Invalid mode: receive interrupt count greater than 1 and available timeout is zero - setting available timeout to 1\n");
		rcv_intr_timeout = 1;
	}
	if (rcv_intr_dynamic && !(rcv_intr_count > 1 && rcv_intr_timeout > 0)) {
		/*
		 * The dynamic algorithm expects a non-zero timeout
		 * and a count > 1.
		 */
		pr_err("Invalid mode: dynamic receive interrupt mitigation with invalid count and timeout - turning dyanmic off\n");
		rcv_intr_dynamic = 0;
	}

	/* sanitize link CRC options */
	link_crc_mask &= WFR_SUPPORTED_CRCS;

	/*
	 * These must be called before the driver is registered with
	 * the PCI subsystem.
	 */
	idr_init(&qib_unit_table);

	hfi_dbg_init();
	ret = pci_register_driver(&qib_driver);
	if (ret < 0) {
		pr_err("Unable to register driver: error %d\n", -ret);
		goto bail_dev;
	}
	goto bail; /* all OK */

bail_dev:
	hfi_dbg_exit();
	idr_destroy(&qib_unit_table);
	dev_cleanup();
bail:
	return ret;
}

module_init(qib_ib_init);

/*
 * Do the non-unit driver cleanup, memory free, etc. at unload.
 */
static void __exit qib_ib_cleanup(void)
{
	pci_unregister_driver(&qib_driver);
	hfi_dbg_exit();
	qib_cpulist_count = 0;
	kfree(qib_cpulist);

	idr_destroy(&qib_unit_table);
	dispose_firmware();	/* asymmetric with obtain_firmware() */
	dev_cleanup();
}

module_exit(qib_ib_cleanup);

/* this can only be called after a successful initialization */
static void cleanup_device_data(struct hfi_devdata *dd)
{
	int ctxt;
	int pidx;
	struct qib_ctxtdata **tmp;
	unsigned long flags;

	/* users can't do anything more with chip */
	for (pidx = 0; pidx < dd->num_pports; ++pidx) {
		struct qib_pportdata *ppd = &dd->pport[pidx];
		struct cc_state *cc_state;
		int i;

		if (ppd->statusp)
			*ppd->statusp &= ~HFI_STATUS_CHIP_PRESENT;

		for (i = 0; i < OPA_MAX_SLS; i++)
			hrtimer_cancel(&ppd->cca_timer[i].hrtimer);

		spin_lock(&ppd->cc_state_lock);
		cc_state = get_cc_state(ppd);
		rcu_assign_pointer(ppd->cc_state, NULL);
		spin_unlock(&ppd->cc_state_lock);

		if (cc_state)
			call_rcu(&cc_state->rcu, cc_state_reclaim);
	}

	free_credit_return(dd);

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
		if (rcd) {
			dd->f_clear_tids(rcd);
			qib_free_ctxtdata(dd, rcd);
		}
	}
	kfree(tmp);
	/* must follow rcv context free - need to remove rcv's hooks */
	for (ctxt = 0; ctxt < dd->num_send_contexts; ctxt++)
		sc_free(dd->send_contexts[ctxt].sc);
	dd->num_send_contexts = 0;
	kfree(dd->send_contexts);
	dd->send_contexts = NULL;
	kfree(dd->boardname);
	vfree(dd->events);
	vfree(dd->status);
	qib_cq_exit(dd);
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
	hfi_pcie_cleanup(dd->pcidev);

	cleanup_device_data(dd);

	qib_free_devdata(dd);
}

static int qib_init_one(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	int ret = 0, j, pidx, initfail;
	struct hfi_devdata *dd = NULL;

	/* First, lock the non-writable module parameters */
	HFI_CAP_LOCK();

	/* Validate some global module parameters */
	if (rcvhdrcnt <= HFI_MIN_HDRQ_EGRBUF_CNT) {
		qib_early_err(&pdev->dev, "Header queue  count too small\n");
		ret = -EINVAL;
		goto bail;
	}
	/* use the encoding function as a sanitization check */
	if (!encode_rcv_header_entry_size(hfi_hdrq_entsize)) {
		qib_early_err(&pdev->dev, "Invalid HdrQ Entry size %u\n",
			     hfi_hdrq_entsize);
		goto bail;
	}

	/* The receive eager buffer size must be set before the receive
	 * contexts are created.
	 *
	 * Set the eager bufer size.  Validate that it falls in a range
	 * allowed by the hardware - all powers of 2 between the min and
	 * max.  The maximum valid MTU is within the eager buffer range
	 * so we do not need to cap the max_mtu by an eager buffer size
	 * setting.
	 */
	if (eager_buffer_size) {
		if (!is_power_of_2(eager_buffer_size))
			eager_buffer_size =
				roundup_pow_of_two(eager_buffer_size);
		eager_buffer_size =
			clamp_val(eager_buffer_size,
				  WFR_MIN_EAGER_BUFFER * 8,
				  WFR_MAX_EAGER_BUFFER_TOTAL);
		qib_early_info(&pdev->dev, "Eager buffer size %u\n",
			       eager_buffer_size);
	} else {
		qib_early_err(&pdev->dev, "Invalid Eager buffer size of 0\n");
		ret = -EINVAL;
		goto bail;
	}

	/* restrict value of hfi_rcvarr_split */
	hfi_rcvarr_split = clamp_val(hfi_rcvarr_split, 0, 100);

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
		goto clean_bail; /* error already printed */

	ret = qib_create_workqueues(dd);
	if (ret)
		goto clean_bail;

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
		dd->flags |= HFI_INITTED;

	j = hfi_device_create(dd);
	if (j)
		dd_dev_err(dd, "Failed to create /dev devices: %d\n", -j);

	if (initfail || ret) {
		qib_stop_timers(dd);
		flush_workqueue(ib_wq);
		for (pidx = 0; pidx < dd->num_pports; ++pidx)
			dd->f_quiet_serdes(dd->pport + pidx);
		if (!j)
			hfi_device_remove(dd);
		if (!ret)
			qib_unregister_ib_device(dd);
		qib_postinit_cleanup(dd);
		if (initfail)
			ret = initfail;
		goto bail;	/* everything already cleaned */
	}

	sdma_start(dd);

	qib_verify_pioperf(dd);

	/* interrupt testing */
	if (test_interrupts) {
		static atomic_t tested;

		if (atomic_inc_return(&tested) <= test_interrupts)
			force_all_interrupts(dd);
	}

	return 0;

clean_bail:
	hfi_pcie_cleanup(pdev);
bail:
	return ret;
}

static void qib_remove_one(struct pci_dev *pdev)
{
	struct hfi_devdata *dd = pci_get_drvdata(pdev);

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

		/*
		 * rcvhdrqentsize is in DWs, so we have to convert to bytes
		 * (* sizeof(u32)).
		 */
		amt = ALIGN(rcd->rcvhdrq_cnt * rcd->rcvhdrqentsize *
			    sizeof(u32), PAGE_SIZE);

		gfp_flags = (rcd->ctxt >= dd->first_user_ctxt) ?
			GFP_USER : GFP_KERNEL;
		rcd->rcvhdrq = dma_zalloc_coherent(
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

		if (HFI_CAP_KGET_MASK(rcd->flags, DMA_RTAIL)) {
			rcd->rcvhdrtail_kvaddr = dma_zalloc_coherent(
				&dd->pcidev->dev, PAGE_SIZE, &phys_hdrqtail,
				gfp_flags);
			if (!rcd->rcvhdrtail_kvaddr)
				goto bail_free;
			rcd->rcvhdrqtailaddr_phys = phys_hdrqtail;
		}

		rcd->rcvhdrq_size = amt;
	}
	/*
	 * These values are per-context:
	 *	RcvHdrCnt
	 *	RcvHdrEntSize
	 *	RcvHdrSize
	 */
	reg = ((u64)(rcd->rcvhdrq_cnt >> WFR_HDRQ_SIZE_SHIFT)
			& WFR_RCV_HDR_CNT_CNT_MASK)
		<< WFR_RCV_HDR_CNT_CNT_SHIFT;
	write_kctxt_csr(dd, rcd->ctxt, WFR_RCV_HDR_CNT, reg);
	reg = (encode_rcv_header_entry_size(rcd->rcvhdrqentsize)
			& WFR_RCV_HDR_ENT_SIZE_ENT_SIZE_MASK)
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
	u32 max_entries, egrtop, alloced_bytes = 0, idx = 0;
	gfp_t gfp_flags;
	u16 order;
	int ret = 0;
	u16 round_mtu = roundup_pow_of_two(max_mtu);

	/*
	 * GFP_USER, but without GFP_FS, so buffer cache can be
	 * coalesced (we hope); otherwise, even at order 4,
	 * heavy filesystem activity makes these fail, and we can
	 * use compound pages.
	 */
	gfp_flags = __GFP_WAIT | __GFP_IO | __GFP_COMP;

	/*
	 * The minimum size of the eager buffers is a groups of MTU-sized
	 * buffers.
	 * The global eager_buffer_size parameter is checked against the
	 * theoretical lower limit of the value. Here, we check against the
	 * MTU.
	 */
	if (rcd->egrbufs.size < (round_mtu * dd->rcv_entries.group_size))
		rcd->egrbufs.size = round_mtu * dd->rcv_entries.group_size;
	/*
	 * If using one-pkt-per-egr-buffer, lower the eager buffer
	 * size to the max MTU (page-aligned).
	 */
	if (!HFI_CAP_KGET_MASK(rcd->flags, MULTI_PKT_EGR))
		rcd->egrbufs.rcvtid_size = round_mtu;

	/*
	 * Eager buffers sizes of 1MB or less require smaller TID sizes
	 * to satisfy the "multiple of 8 RcvArray entries" requirement.
	 */
	if (rcd->egrbufs.size <= (1 << 20))
		rcd->egrbufs.rcvtid_size = max((unsigned long)round_mtu,
			rounddown_pow_of_two(rcd->egrbufs.size / 8));

	while (alloced_bytes < rcd->egrbufs.size &&
	       rcd->egrbufs.alloced < rcd->egrbufs.count) {
		rcd->egrbufs.buffers[idx].addr =
			dma_zalloc_coherent(&dd->pcidev->dev,
					    rcd->egrbufs.rcvtid_size,
					    &rcd->egrbufs.buffers[idx].phys,
					    gfp_flags);
		if (rcd->egrbufs.buffers[idx].addr) {
			rcd->egrbufs.buffers[idx].len =
				rcd->egrbufs.rcvtid_size;
			rcd->egrbufs.rcvtids[rcd->egrbufs.alloced].addr =
				rcd->egrbufs.buffers[idx].addr;
			rcd->egrbufs.rcvtids[rcd->egrbufs.alloced].phys =
				rcd->egrbufs.buffers[idx].phys;
			rcd->egrbufs.alloced++;
			alloced_bytes += rcd->egrbufs.rcvtid_size;
			idx++;
		} else {
			u32 new_size, i, j;
			u64 offset = 0;

			/*
			 * Fail the eager buffer allocation if:
			 *   - we are already using the lowest acceptible size
			 *   - we are using one-pkt-per-egr-buffer (this implies
			 *     that we are accepting only one size)
			 */
			if (rcd->egrbufs.rcvtid_size == round_mtu ||
			    !HFI_CAP_KGET_MASK(rcd->flags, MULTI_PKT_EGR)) {
				dd_dev_err(dd, "ctxt%u: Failed to allocate eager buffers\n",
					rcd->ctxt);
				goto bail_rcvegrbuf_phys;
			}

			new_size = rcd->egrbufs.rcvtid_size / 2;

			/*
			 * If the first attempt to allocate memory failed, don't
			 * fail everything but continue with the next lower
			 * size.
			 */
			if (idx == 0) {
				rcd->egrbufs.rcvtid_size = new_size;
				continue;
			}

			/*
			 * Re-partition already allocated buffers to a smaller
			 * size.
			 */
			rcd->egrbufs.alloced = 0;
			for (i = 0, j = 0, offset = 0; j < idx; i++) {
				if (i >= rcd->egrbufs.count)
					break;
				rcd->egrbufs.rcvtids[i].phys =
					rcd->egrbufs.buffers[j].phys + offset;
				rcd->egrbufs.rcvtids[i].addr =
					rcd->egrbufs.buffers[j].addr + offset;
				rcd->egrbufs.alloced++;
				if ((rcd->egrbufs.buffers[j].phys + offset +
				     new_size) ==
				    (rcd->egrbufs.buffers[j].phys +
				     rcd->egrbufs.buffers[j].len)) {
					j++;
					offset = 0;
				} else
					offset += new_size;
			}
			rcd->egrbufs.rcvtid_size = new_size;
		}
	}
	rcd->egrbufs.numbufs = idx;
	rcd->egrbufs.size = alloced_bytes;

	dd_dev_info(dd, "ctxt%u: Alloced %u rcv tid entries @ %uKB, total %zuKB\n",
		rcd->ctxt, rcd->egrbufs.alloced, rcd->egrbufs.rcvtid_size,
		rcd->egrbufs.size);

	/*
	 * Set the contexts rcv array head update threshold to the closest
	 * power of 2 (so we can use a mask instead of modulo) below half
	 * the allocated entries.
	 */
	rcd->egrbufs.threshold =
		rounddown_pow_of_two(rcd->egrbufs.alloced / 2);
	/*
	 * Compute the expecte RcvArray entry base. This is done after
	 * allocating the eager buffers in order to maximize the
	 * expected RcvArray entries for the context.
	 */
	max_entries = rcd->rcv_array_groups * dd->rcv_entries.group_size;
	egrtop = roundup(rcd->egrbufs.alloced, dd->rcv_entries.group_size);
	rcd->expected_count = max_entries - egrtop;
	if (rcd->expected_count > WFR_MAX_TID_PAIR_ENTRIES * 2)
		rcd->expected_count = WFR_MAX_TID_PAIR_ENTRIES * 2;

	rcd->expected_base = rcd->eager_base + egrtop;
	dd_dev_info(dd, "ctxt%u: eager:%u, exp:%u, egrbase:%u, expbase:%u\n",
		    rcd->ctxt, rcd->egrbufs.alloced, rcd->expected_count,
		    rcd->eager_base, rcd->expected_base);

	if (!hfi_rcvbuf_validate(rcd->egrbufs.rcvtid_size, PT_EAGER, &order)) {
		dd_dev_err(dd, "ctxt%u: current Eager buffer size is invalid %u\n",
			   rcd->ctxt, rcd->egrbufs.rcvtid_size);
		ret = -EINVAL;
		goto bail;
	}

	for (idx = 0; idx < rcd->egrbufs.alloced; idx++) {
		dd->f_put_tid(dd, rcd->eager_base + idx, PT_EAGER,
			      rcd->egrbufs.rcvtids[idx].phys, order);
		cond_resched();
	}
	goto bail;

bail_rcvegrbuf_phys:
	for (idx = 0; idx < rcd->egrbufs.alloced &&
		     rcd->egrbufs.buffers[idx].addr;
	     idx++) {
		dma_free_coherent(&dd->pcidev->dev,
				  rcd->egrbufs.buffers[idx].len,
				  rcd->egrbufs.buffers[idx].addr,
				  rcd->egrbufs.buffers[idx].phys);
		rcd->egrbufs.buffers[idx].addr = NULL;
		rcd->egrbufs.buffers[idx].phys = 0;
		rcd->egrbufs.buffers[idx].len = 0;
	}
bail:
	return ret;
}
