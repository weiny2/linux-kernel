/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2014 - 2015 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * BSD LICENSE
 *
 * Copyright (c) 2014 - 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Intel Corporation nor the names of its
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
 * Intel(R) Omni-Path Gen2 HFI PCIe Driver
 */

#include <linux/sched.h>
#include <rdma/opa_core.h>
#include <rdma/hfi_tx.h>
#include <rdma/hfi_args.h>
#include <rdma/hfi_ct.h>
#include <rdma/hfi_eq.h>
#include "opa_hfi.h"

/* Format an E2E control message, transmit it and wait for an acknowledgment */
static int hfi_put_e2e_ctrl(struct hfi_devdata *dd, int slid, int dlid,
			    int tc, int port, enum ptl_op_e2e_ctrl op)
{
	union hfi_tx_cq_command tx_cmd;
	hfi_tx_e2e_control_t *command = &tx_cmd.e2e_ctrl;
	struct hfi_ctx *ctx = &dd->priv_ctx;
	hfi_cmd_t cmd;
	u16 cmd_length = 8;
	u16 cmd_slots = _HFI_CMD_SLOTS(cmd_length);
	hfi_process_t target_id;
	hfi_ack_req_t ack_req = PTL_CT_ACK_REQ;
	hfi_md_options_t md_options = PTL_MD_EVENT_CT_ACK;
	hfi_ni_t ni = PTL_NONMATCHING_PHYSICAL;
	u64 *eq_entry;
	unsigned long flags;
	int rc;

	memset(&tx_cmd, 0x0, sizeof(tx_cmd));
	target_id.phys.slid = dlid;
	target_id.phys.ipid = dd->priv_ctx.pid;
	cmd.ptl_opcode_low = op;
	cmd.ttype = BUFFERED;

	_hfi_format_base_put_flit0(ctx, ni, &command->flit0, cmd,
				   E2E_CTRL, cmd_length, target_id, port, 0,
				   RC_IN_ORDER_0, tc, 0, 0, 0, 0, 0,
				   FXR_TRUE, ack_req, md_options, dd->e2e_eq,
				   PTL_CT_NONE, 0, 0);

	command->flit1.e.max_dist = 27; /* TODO: Why? */

	spin_lock_irqsave(&dd->priv_tx_cq_lock, flags);
	/* Transmit the E2E message */
	do {
		rc = hfi_tx_cmd_put_buff(&dd->priv_tx_cq,
					 command->command,
					 cmd_slots);
	} while (rc == -EAGAIN);
	spin_unlock_irqrestore(&dd->priv_tx_cq_lock, flags);
	/* PTL_SINGLE_DESTROY does not initiate events at the initiator */
	if (op != PTL_SINGLE_CONNECT)
		goto done;
	/* Check on EQ 0 NI 0 for a PTL_EVENT_INITIATOR_CONNECT event */
	hfi_eq_wait_timed(ctx, dd->e2e_eq, HFI_TX_TIMEOUT_MS,
			  (void **)&eq_entry);
	if (eq_entry) {
		union initiator_EQEntry *txe =
			(union initiator_EQEntry *)eq_entry;

		if (txe->event_kind == PTL_EVENT_INITIATOR_CONNECT) {
			if (txe->fail_type) {
				dd_dev_info(dd, "E2E EQ fail kind %d ft %d\n",
					    txe->event_kind, txe->fail_type);
				rc = -EIO;
			} else {
				dd_dev_info(dd, "E2E EQ success kind %d\n",
					    txe->event_kind);
			}
			hfi_eq_advance(ctx, &dd->priv_rx_cq,
				       dd->e2e_eq, eq_entry);
		} else {
			rc = -EIO;
			dd_dev_err(dd, "Invalid E2E event %d\n",
				   txe->event_kind);
		}
	} else {
		rc = -EIO;
		dd_dev_err(dd, "E2E EQ failure rc %d\n", rc);
	}
done:
	return rc;
}

int hfi_e2e_ctrl(struct hfi_ctx *ctx, struct opa_e2e_ctrl *e2e)
{
	struct hfi_devdata *dd = ctx->devdata;
	struct hfi_ptcdata *ptc;
	u8 tc;
	struct ida *cache;
	int ret;

	if (e2e->port_num > dd->num_pports)
		return -EINVAL;

	if (e2e->dlid >= HFI_MAX_LID_SUPP)
		return -EINVAL;

	/*
	 * TODO: The SL <-> TC mapping is currently hard coded in
	 * Simics. At some point we need to determine the mapping
	 * from the CSR registers instead.
	 */
	tc = e2e->sl % HFI_MAX_TC;

	ptc = &dd->pport[e2e->port_num - 1].ptc[tc];
	cache = &ptc->e2e_state_cache;

	mutex_lock(&dd->e2e_lock);
	/* Check if a new entry can be inserted into the cache */
	ret = ida_simple_get(cache, e2e->dlid, e2e->dlid + 1, GFP_KERNEL);
	/*
	 * If the entry is allocated then the E2E connection is established
	 * already and there is no need to initiate another E2E connection
	 */
	if (-ENOSPC == ret) {
		ret = 0;
		goto unlock;
	}
	/* Bail out upon other IDA failures */
	if (ret < 0)
		goto unlock;
	/* Initiate an E2E connection if one did not exist */
	ret = hfi_put_e2e_ctrl(dd, e2e->slid, e2e->dlid,
			       tc, e2e->port_num - 1, PTL_SINGLE_CONNECT);
	if (ret < 0)
		/* remove the entry from the cache upon failure */
		ida_remove(cache, e2e->dlid);
	else
		/*
		 * If it succeeded, then the cache is updated and the next
		 * connection request for this SLID, DLID, TC tuple will
		 * hit the cache without initiating an E2E connection
		 * message to the peer. Update the max DLID for this TC
		 */
		ptc->max_e2e_dlid = MAX(ptc->max_e2e_dlid, e2e->dlid);
unlock:
	mutex_unlock(&dd->e2e_lock);
	return ret;
}

/* Tear down all existing E2E connections */
void hfi_e2e_destroy(struct hfi_devdata *dd)
{
	u8 port, tc;
	u32 dlid;
	int ret;

	mutex_lock(&dd->e2e_lock);
	for (port = 0; port < HFI_NUM_PPORTS; port++) {
		u32 slid = dd->pport[port].lid;

		for (tc = 0; tc < HFI_MAX_TC; tc++) {
			struct hfi_ptcdata *ptc = &dd->pport[port].ptc[tc];
			struct ida *cache = &ptc->e2e_state_cache;

			for (dlid = 0; dlid <= ptc->max_e2e_dlid; dlid++) {
				ret = ida_simple_get(cache, dlid, dlid + 1,
						     GFP_KERNEL);
				if (-ENOSPC != ret)
					continue;
				hfi_put_e2e_ctrl(dd, slid, dlid,
						 tc, port,
						 PTL_SINGLE_DESTROY);
				/*
				 * TODO: implement mechanism to inform the
				 * peer to invalidate its cache. Either the
				 * FXR HW has to provide a notification
				 * mechanism or custom node to node
				 * messages need to be implemented in SW
				 * via the system PID TX/RX CQs.
				 */
			}
			ida_destroy(cache);
		}
	}
	mutex_unlock(&dd->e2e_lock);
}

int hfi_dlid_assign(struct hfi_ctx *ctx,
		    struct hfi_dlid_assign_args *dlid_assign)
{
	int ret = 0;

	BUG_ON(!capable(CAP_SYS_ADMIN));

	/* can only write DLID mapping once */
	if (ctx->dlid_base != HFI_LID_NONE)
		return -EPERM;

	/* write DLID relocation table */
	ret = hfi_update_dlid_relocation_table(ctx, dlid_assign);
	if (ret < 0)
		return ret;

	ctx->dlid_base = dlid_assign->dlid_base;
	ctx->mode |= HFI_CTX_MODE_LID_VIRTUALIZED;

	return 0;
}

int hfi_dlid_release(struct hfi_ctx *ctx, u32 dlid_base, u32 count)
{
	int ret = 0;

	BUG_ON(!capable(CAP_SYS_ADMIN));

	ret = hfi_reset_dlid_relocation_table(ctx, dlid_base, count);
	if (ret < 0)
		return ret;

	ctx->dlid_base = HFI_LID_NONE;
	ctx->mode &= ~HFI_CTX_MODE_LID_VIRTUALIZED;

	return 0;
}
