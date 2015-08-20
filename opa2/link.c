/*
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright(c) 2015 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * BSD LICENSE
 *
 * Copyright(c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * This file contains all of the code that is specific to
 * Link Negotiation and Initialization(LNI)
 */

#include "opa_hfi.h"

/*
 * Change the physical port and/or logical link state.
 *
 * Returns 0 on success, -errno on failure.
 */
int hfi_set_link_state(struct hfi_pportdata *ppd, u32 state)
{
	/*
	 * FXRTODO: To be implemented as part of LNI
	 * for now this simple state machine logic
	 * should work fine
	 */
	switch (state) {
	case HLS_DN_OFFLINE:
		ppd->host_link_state = HLS_DN_OFFLINE;
	case HLS_DN_DISABLE:
		ppd->host_link_state = HLS_DN_DISABLE;
	case HLS_DN_DOWNDEF:
		ppd->host_link_state = HLS_DN_DOWNDEF;
		ppd->lstate = IB_PORT_DOWN;
		opa_core_notify_clients(ppd->dd->bus_dev,
					OPA_LINK_STATE_CHANGE, ppd->pnum);
		break;
	case HLS_DN_POLL:
		/*
		 * FXRTODO: Fake the transition from
		 * POLL to INIT here by directly setting
		 * the link state to INIT until we
		 * have LNI in place.
		 */
	case HLS_UP_INIT:
		ppd->host_link_state = HLS_UP_INIT;
		ppd->lstate = IB_PORT_INIT;
		break;
	case HLS_UP_ARMED:
		ppd->host_link_state = HLS_UP_ARMED;
		ppd->lstate = IB_PORT_ARMED;
		break;
	case HLS_UP_ACTIVE:
		ppd->host_link_state = HLS_UP_ACTIVE;
		ppd->lstate = IB_PORT_ACTIVE;
		opa_core_notify_clients(ppd->dd->bus_dev,
					OPA_LINK_STATE_CHANGE, ppd->pnum);
		break;
	default:
		ppd->host_link_state = HLS_UP_INIT;
		ppd->lstate = IB_PORT_INIT;
	}

	return -EINVAL;
}

/*
 * Start the link. Schedule a retry if the cable is not
 * present or if unable to start polling. Do not do anything if the
 * link is disabled.  Returns 0 if link is disabled or moved to polling
 */
int hfi_start_link(struct hfi_pportdata *ppd)
{
	return hfi_set_link_state(ppd, HLS_DN_POLL);
}
