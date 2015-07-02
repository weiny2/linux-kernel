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
#include "opa_hfi.h"

/* Poll on a counting event to return success, failure or time out */
static int
__hfi_ct_wait(struct hfi_ctx *ctx, hfi_ct_handle_t ct_h,
	      unsigned long threshold, unsigned long timeout_ms,
	      unsigned long *ct_val)
{
	unsigned long val;
	unsigned long exit_jiffies = jiffies + msecs_to_jiffies(timeout_ms);

	while (1) {
		if (hfi_ct_get_failure(ctx, ct_h))
			return -EFAULT;
		val = hfi_ct_get_success(ctx, ct_h);
		if (val >= threshold)
			break;
		if (time_after(jiffies, exit_jiffies))
			return -ETIME;
		schedule();
	}

	if (ct_val)
		*ct_val = val;

	return 0;
}

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
	hfi_ct_handle_t ct_tx;
	hfi_ct_alloc_args_t ct_alloc = {0};
	hfi_ni_t ni = PTL_NONMATCHING_PHYSICAL;
	unsigned long flags;
	int rc;

	memset(&tx_cmd, 0x0, sizeof(tx_cmd));
	target_id.phys.slid = dlid;
	target_id.phys.ipid = dd->priv_ctx.pid;
	cmd.ptl_opcode_low = op;
	cmd.ttype = BUFFERED;
	ct_alloc.ni = ni;
	rc = hfi_ct_alloc(ctx, &ct_alloc, &ct_tx);
	if (rc < 0)
		goto err;

	_hfi_format_base_put_flit0(ctx, ni, &command->flit0, cmd,
				   E2E_CTRL, cmd_length, target_id, 0, 0,
				   FXR_TRUE, ack_req, md_options, PTL_EQ_NONE,
				   ct_tx, 0, 0);
	command->flit0.a.sh = 0;
	command->flit0.a.sl = tc;
	command->flit0.a.rc = RC_DETERMINISTIC_0;
	command->flit0.a.pt = port;

	command->flit1.e.max_dist = 27; /* TODO: Why? */

	spin_lock_irqsave(&dd->priv_tx_cq_lock, flags);
	/* Transmit the E2E message */
	do {
		rc = hfi_tx_cmd_put_buff(&dd->priv_tx_cq,
					 command->command,
					 cmd_slots);
	} while (rc == -EAGAIN);
	spin_unlock_irqrestore(&dd->priv_tx_cq_lock, flags);

	/* Wait for an acknowledgment */
	rc = __hfi_ct_wait(ctx, ct_tx, 1, HFI_TX_TIMEOUT_MS, NULL);
	if (rc < 0) {
		/* TODO: Should failure and timeout be handled the same */
		dd_dev_err(dd, "e2e op %d slid %d dlid %d tc %d port %d fail rc %d\n",
			   op, slid, dlid, tc, port, rc);
		goto err;
	} else {
		dd_dev_info(dd, "e2e op %d slid %d dlid %d tc %d port %d success\n",
			    op, slid, dlid, tc, port);
	}
err:
	return rc;
}

int hfi_e2e_ctrl(struct hfi_ctx *ctx, struct opa_e2e_ctrl *e2e)
{
	struct hfi_devdata *dd = ctx->devdata;
	u8 port, tc;

	if (e2e->slid == dd->pport[0].lid)
		port = 0;
	else if (e2e->slid == dd->pport[1].lid)
		port = 1;
	else
		return -EINVAL;

	if (e2e->dlid >= HFI_MAX_LID_SUPP)
		return -EINVAL;

	if (e2e->op > PTL_SINGLE_DESTROY)
		return -EINVAL;
	/*
	 * TODO: The SL <-> TC mapping is currently hard coded in
	 * Simics. At some point we need to determine the mapping
	 * from the CSR registers instead.
	 */
	tc = e2e->sl % HFI_MAX_TC;
	/*
	 * TODO: Query if E2E connection is already setup
	 * via PTL_E2E_STATUS_REQ once supported before
	 * initiating a new connection or teardown
	 */
	return hfi_put_e2e_ctrl(dd, e2e->slid, e2e->dlid, tc, port, e2e->op);
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
