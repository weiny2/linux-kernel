/*
 * Copyright (c) 2008, 2009, 2010 QLogic Corporation. All rights reserved.
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


//FIXME: not currently called
/*
 * Flush all sends that might be in the ready to send state, as well as any
 * that are in the process of being sent.  Used whenever we need to be
 * sure the send side is idle.  Cleans up all buffer state by canceling
 * all pio buffers, and issuing an abort, which cleans up anything in the
 * launch fifo.  The cancel is superfluous on some chip versions, but
 * it's safer to always do it.
 * PIOAvail bits are updated by the chip as if a normal send had happened.
 */
void qib_cancel_sends(struct qib_pportdata *ppd)
{
	struct hfi_devdata *dd = ppd->dd;
	struct qib_ctxtdata *rcd;
	unsigned long flags;
	unsigned ctxt;

	// FIXME: the PSM communication needs to be re-created.
	/*
	 * Tell PSM to disarm buffers again before trying to reuse them.
	 * We need to be sure the rcd doesn't change out from under us
	 * while we do so.  We hold the two locks sequentially.  We might
	 * needlessly set some need_disarm bits as a result, if the
	 * context is closed after we release the uctxt_lock, but that's
	 * fairly benign, and safer than nesting the locks.
	 */
	for (ctxt = dd->first_user_ctxt; ctxt < dd->num_rcv_contexts; ctxt++) {
		rcd = dd->rcd[ctxt];
		if (!rcd)
			continue;
		spin_lock_irqsave(&dd->uctxt_lock, flags);
		sc_drop(rcd->sc);
		spin_unlock_irqrestore(&dd->uctxt_lock, flags);
	}
}
