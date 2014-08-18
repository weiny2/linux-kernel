/*
 * Copyright (c) 2006, 2007, 2008, 2009, 2010 QLogic Corporation.
 * All rights reserved.
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
#include <linux/delay.h>

#include "hfi.h"
#include "common.h"

/**
 * qib_format_hwmsg - format a single hwerror message
 * @msg message buffer
 * @msgl length of message buffer
 * @hwmsg message to add to message buffer
 */
static void qib_format_hwmsg(char *msg, size_t msgl, const char *hwmsg)
{
	strlcat(msg, "[", msgl);
	strlcat(msg, hwmsg, msgl);
	strlcat(msg, "]", msgl);
}

/**
 * qib_format_hwerrors - format hardware error messages for display
 * @hwerrs hardware errors bit vector
 * @hwerrmsgs hardware error descriptions
 * @nhwerrmsgs number of hwerrmsgs
 * @msg message buffer
 * @msgl message buffer length
 */
void qib_format_hwerrors(u64 hwerrs, const struct qib_hwerror_msgs *hwerrmsgs,
			 size_t nhwerrmsgs, char *msg, size_t msgl)
{
	int i;

	for (i = 0; i < nhwerrmsgs; i++)
		if (hwerrs & hwerrmsgs[i].mask)
			qib_format_hwmsg(msg, msgl, hwerrmsgs[i].msg);
}

static void signal_ib_event(struct qib_pportdata *ppd, enum ib_event_type ev)
{
	struct ib_event event;
	struct hfi_devdata *dd = ppd->dd;

	/*
	 * Only call ib_dispatch_event() if the IB device has been
	 * registered.  Right now, QIB_INITED is set iff the driver
	 * has successfully registered with IB core.
	 * TODO: Make a separate flag for registering with IB?
	 */
	if (!(dd->flags & QIB_INITTED))
		return;
	event.device = &dd->verbs_dev.ibdev;
	event.element.port_num = ppd->port;
	event.event = ev;
	ib_dispatch_event(&event);
}

/*
 * Handle a linkup or link down notification.  This is called both inside
 * and outside an interrupt.
 */
void handle_linkup_change(struct hfi_devdata *dd, u32 linkup)
{
	struct qib_pportdata *ppd = &dd->pport[0];
	enum ib_event_type ev;

/* see when we are called */
dd_dev_info(dd, "%s: linkup %u\n", __func__, linkup);
	if (!(ppd->linkup ^ !!linkup))
		return;	/* no change, nothing to do */

	if (linkup) {
		/*
		 * The simulator does not implement:
		 *	- VerifyCap interupt
		 *	- VerifyCap frames
		 * But rather moves directly to LinkUp.
		 *
		 * Do the work of the VerifyCap interrupt handler,
		 * handle_verify_cap(), but do not try moving the state to
		 * LinkUp as we are already there.
		 *
		 * NOTE: This uses this device's vAU, vCU, and vl15_init for
		 * the remote values.  Both sides must be using the values.
		 */
		if (dd->icode == WFR_ICODE_FUNCTIONAL_SIMULATOR) {
			set_up_vl15(dd, dd->vau, dd->vl15_init);
			assign_remote_cm_au_table(dd, dd->vcu);
			ppd->neighbor_guid =
				cpu_to_be64(read_csr(dd,
					DC_DC8051_STS_REMOTE_GUID));
			/* FIXME when simulator works */
			ppd->neighbor_type = 0;
			dd_dev_info(dd, "Neighbor GUID: %llx Neighbor type %d\n",
				be64_to_cpu(ppd->neighbor_guid),
					ppd->neighbor_type);
		}

		/* physical link went up */
		ppd->linkup = 1;

		ev = IB_EVENT_PORT_ACTIVE;

		/* start a 75msec timer to clear symbol errors */
		mod_timer(&ppd->symerr_clear_timer, msecs_to_jiffies(75));

		/*
		 * The STL FM is not yet available.  Fake a BufferControlTable
		 * MAD packet so that all VLs have credits.
		 */
		if (set_link_credits)
			assign_link_credits(dd);
	} else {
		/* physical link went down */
		ppd->linkup = 0;

		/* clear HW details of the prevoius connection */
		reset_link_credits(dd);

		ev = IB_EVENT_PORT_ERR;

		qib_set_uevent_bits(ppd, _QIB_EVENT_LINKDOWN_BIT);

		/* restart the link after a delay */
		schedule_delayed_work(&ppd->link_restart_work,
						msecs_to_jiffies(1000));
	}

	/* notify IB of the link change */
	signal_ib_event(ppd, ev);
}

void qib_clear_symerror_on_linkup(unsigned long opaque)
{
	struct qib_pportdata *ppd = (struct qib_pportdata *)opaque;

	if (ppd->lstate == IB_PORT_ACTIVE)
		return;

	ppd->ibport_data.z_symbol_error_counter =
		ppd->dd->f_portcntr(ppd, QIBPORTCNTR_IBSYMBOLERR);
}

/*
 * Handle receive interrupts for user ctxts; this means a user
 * process was waiting for a packet to arrive, and didn't want
 * to poll.
 */
void qib_handle_urcv(struct hfi_devdata *dd, u64 ctxtr)
{
	struct qib_ctxtdata *rcd;
	unsigned long flags;
	int i;

	spin_lock_irqsave(&dd->uctxt_lock, flags);
	for (i = dd->first_user_ctxt; dd->rcd && i < dd->num_rcv_contexts; i++) {
		if (!(ctxtr & (1ULL << i)))
			continue;
		rcd = dd->rcd[i];
		if (!rcd || !rcd->cnt)
			continue;

		if (test_and_clear_bit(QIB_CTXT_WAITING_RCV,
				       &rcd->event_flags)) {
			wake_up_interruptible(&rcd->wait);
			dd->f_rcvctrl(dd, QIB_RCVCTRL_INTRAVAIL_DIS, rcd->ctxt);
		} else if (test_and_clear_bit(QIB_CTXT_WAITING_URG,
					      &rcd->event_flags)) {
			rcd->urgent++;
			wake_up_interruptible(&rcd->wait);
		}
	}
	spin_unlock_irqrestore(&dd->uctxt_lock, flags);
}

void qib_bad_intrstatus(struct hfi_devdata *dd)
{
	static int allbits;

	/* separate routine, for better optimization of qib_intr() */

	/*
	 * We print the message and disable interrupts, in hope of
	 * having a better chance of debugging the problem.
	 */
	dd_dev_err(dd,
		"Read of chip interrupt status failed disabling interrupts\n");
	if (allbits++) {
		/* disable interrupt delivery, something is very wrong */
		if (allbits == 2)
			dd->f_set_intr_state(dd, 0);
		if (allbits == 3) {
			dd_dev_err(dd,
				"2nd bad interrupt status, unregistering interrupts\n");
			dd->flags |= QIB_BADINTR;
			dd->flags &= ~QIB_INITTED;
			dd->f_free_irq(dd);
		}
	}
}
