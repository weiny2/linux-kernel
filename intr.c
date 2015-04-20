/*
 * Copyright (c) 2014 Intel Corporation. All rights reserved.
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
#include "sdma.h"

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
	 * registered.  HFI_INITED is set iff the driver has successfully
	 * registered with the IB core.
	 */
	if (!(dd->flags & HFI_INITTED))
		return;
	event.device = &dd->verbs_dev.ibdev;
	event.element.port_num = ppd->port;
	event.event = ev;
	ib_dispatch_event(&event);
}

/*
 * Handle a linkup or link down notification.
 * This is called outside an interrupt.
 */
void handle_linkup_change(struct hfi_devdata *dd, u32 linkup)
{
	struct qib_pportdata *ppd = &dd->pport[0];
	enum ib_event_type ev;

	if (!(ppd->linkup ^ !!linkup))
		return;	/* no change, nothing to do */

	if (linkup) {
		/*
		 * Quick linkup and all link up on the simulator does not
		 * trigger or implement:
		 *	- VerifyCap interrupt
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
		if (quick_linkup
			    || dd->icode == WFR_ICODE_FUNCTIONAL_SIMULATOR) {
			set_up_vl15(dd, dd->vau, dd->vl15_init);
			assign_remote_cm_au_table(dd, dd->vcu);
			ppd->neighbor_guid =
				cpu_to_be64(read_csr(dd,
					DC_DC8051_STS_REMOTE_GUID));
			ppd->neighbor_type =
				read_csr(dd, DC_DC8051_STS_REMOTE_NODE_TYPE) &
					DC_DC8051_STS_REMOTE_NODE_TYPE_VAL_MASK;
			dd_dev_info(dd,
				"Neighbor GUID: %llx Neighbor type %d\n",
				be64_to_cpu(ppd->neighbor_guid),
				ppd->neighbor_type);
		}

		/* physical link went up */
		ppd->linkup = 1;
		ppd->offline_disabled_reason = OPA_LINKDOWN_REASON_NONE;

		/* link widths are not avaiable until the link is fully up */
		get_linkup_link_widths(ppd);

	} else {
		/* physical link went down */
		ppd->linkup = 0;

		/* clear HW details of the previous connection */
		reset_link_credits(dd);

		/* freeze after a link down to guarantee a clean egress */
		start_freeze_handling(ppd,
					WFR_FREEZE_SELF|WFR_FREEZE_LINK_DOWN);

		ev = IB_EVENT_PORT_ERR;

		qib_set_uevent_bits(ppd, _HFI_EVENT_LINKDOWN_BIT);

		/* if we are down, the neighbor is down */
		ppd->neighbor_normal = 0;

		/* notify IB of the link change */
		signal_ib_event(ppd, ev);
	}


}

/*
 * Handle receive or urgent interrupts for user contexts.  This means a user
 * process was waiting for a packet to arrive, and didn't want to poll.
 */
void handle_user_interrupt(struct qib_ctxtdata *rcd)
{
	struct hfi_devdata *dd = rcd->dd;
	unsigned long flags;

	spin_lock_irqsave(&dd->uctxt_lock, flags);
	if (!rcd->cnt)
		goto done;

	if (test_and_clear_bit(QIB_CTXT_WAITING_RCV, &rcd->event_flags)) {
		wake_up_interruptible(&rcd->wait);
		hfi1_rcvctrl(dd, QIB_RCVCTRL_INTRAVAIL_DIS, rcd->ctxt);
	} else if (test_and_clear_bit(QIB_CTXT_WAITING_URG,
							&rcd->event_flags)) {
		rcd->urgent++;
		wake_up_interruptible(&rcd->wait);
	}
done:
	spin_unlock_irqrestore(&dd->uctxt_lock, flags);
}
