/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2015-2016 Intel Corporation.
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
 * Copyright (c) 2015 Intel Corporation.
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
 */

#ifndef _HFI_PT_COMMON_H
#define _HFI_PT_COMMON_H

#include "hfi_cmdq.h"
#include "hfi_eq_common.h"

/*
 * Issue requested RX command to update the PT Entry cache with
 * new value.
 */
static inline
int _hfi_pt_issue_cmd(struct hfi_ctx *ctx, struct hfi_cmdq *rx_cq,
		      enum rx_cq_cmd pt_cmd, u8 ni,
		      u32 pt_idx, u64 *val)
{
	union hfi_rx_cq_command cmd;
	int cmd_slots, rc;
	u64 done = 0;

	cmd_slots = hfi_format_rx_write64(ctx,
					  ni, pt_idx,
					  pt_cmd,
					  HFI_CT_NONE, /* Unused */
					  val,         /* payload[0-3] */
					  (u64)&done,
					  HFI_GEN_CC, &cmd);

	do {
		rc = hfi_rx_command(rx_cq, (u64 *)&cmd, cmd_slots);
	} while (rc == -EAGAIN);

	if (rc < 0)
		return -EIO;

	/* Busy poll for the PT_WRITE command completion on EQD=0 (NI=0) */
	rc = hfi_eq_poll_cmd_complete(ctx, &done);

	return rc;
}

static inline
int hfi_pt_enable(struct hfi_ctx *ctx, struct hfi_cmdq *rx_cq, u8 ni,
		  u32 pt_idx)
{
	union ptentry_fp0 ptentry;
	int rc;

	if (HFI_CTX_INVALID(ctx))
		return -ENODEV;
	if (ni >= HFI_NUM_NIS || !rx_cq)
		return -EINVAL;
	if (pt_idx == HFI_PT_ANY && pt_idx >= HFI_NUM_PT_ENTRIES)
		return -EINVAL;

	/*
	 * Initialise the PT entry
	 */
	ptentry.val[0] = 0;
	ptentry.val[1] = 0;
	ptentry.val[2] = 0;
	ptentry.val[3] = 0;
	ptentry.enable = 1;
	/* Mask for just the PT entry enable bit. */
	ptentry.val[2] = PTENTRY_FP0_ENABLE_MASK;

	/* Issue the PT_UPDATE_LOWER command via the RX CQ */
	rc = _hfi_pt_issue_cmd(ctx, rx_cq, PT_UPDATE_LOWER, ni, pt_idx,
			       &ptentry.val[0]);

	if (rc < 0)
		return -EIO;

	return 0;
}

static inline
int hfi_pt_disable(struct hfi_ctx *ctx, struct hfi_cmdq *rx_cq, u8 ni,
		   u32 pt_idx)
{
	union ptentry_fp0 ptentry;
	int rc;

	if (HFI_CTX_INVALID(ctx))
		return -ENODEV;
	if (ni >= HFI_NUM_NIS || !rx_cq)
		return -EINVAL;
	if (pt_idx == HFI_PT_ANY && pt_idx >= HFI_NUM_PT_ENTRIES)
		return -EINVAL;

	/*
	 * Initialise the PT entry
	 */
	ptentry.val[0] = 0;
	ptentry.val[1] = 0;
	ptentry.val[2] = 0;
	ptentry.val[3] = 0;
	ptentry.enable = 0;
	/* Mask for just the PT entry enable bit. */
	ptentry.val[2] = PTENTRY_FP0_ENABLE_MASK;

	/* Issue the PT_UPDATE_LOWER command via the RX CQ */
	rc = _hfi_pt_issue_cmd(ctx, rx_cq, PT_UPDATE_LOWER, ni, pt_idx,
			       &ptentry.val[0]);

	if (rc < 0)
		return -EIO;

	return 0;
}
#endif
