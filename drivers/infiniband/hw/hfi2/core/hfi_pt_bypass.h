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

#ifndef _HFI_PT_BYPASS_H
#define _HFI_PT_BYPASS_H

#include "hfi_pt_common.h"

#define HFI_PT_TEST_FLAG(FLAGS, FLAGBIT)	(!!((FLAGS) & (FLAGBIT)))

#define HFI_PT_EAGER_FLAG_DONT_DROP_EAGER_FULL	BIT(0)
#define HFI_PT_EAGER_FLAG_ENA_JOB_KEY		BIT(1)
#define HFI_PT_EAGER_FLAG_DONT_DROP_RHQF	BIT(2)

/**
 * struct hfi_pt_alloc_eager_args - alloc args for PT Eager receive
 * @eager_order: number of eager buffers expressed as log2/8
 * @job_key: job_key for receiving KDETH packets
 * @job_key_mask: job_key mask for receiving KDETH packets
 * @eq_handle: EQ handle to receive RX events
 * @flags: Various flag_t's in Eager PTE (see above).
 */
struct hfi_pt_alloc_eager_args {
	u8	eager_order;
	u16	job_key;
	u16	job_key_mask;
	struct hfi_eq   *eq_handle;
	u64	flags;
};

#define HFI_PT_EXPECTED_FLAG_ENA_HDR_SUPP		BIT(0)
#define HFI_PT_EXPECTED_FLAG_KEEP_AFTER_SEQ_ERR		BIT(1)
#define HFI_PT_EXPECTED_FLAG_KEEP_ON_GEN_ERR		BIT(2)
#define HFI_PT_EXPECTED_FLAG_KEEP_PAYLOAD_ON_GEN_ERR	BIT(3)
#define HFI_PT_EXPECTED_FLAG_ENA_EXT_PSN		BIT(4)
#define HFI_PT_EXPECTED_FLAG_ENA_JOB_KEY		BIT(5)
#define HFI_PT_EXPECTED_FLAG_DONT_DROP_RHQF		BIT(6)

/**
 * struct hfi_pt_alloc_expected_args - alloc args for PT Expected receive
 * @flow_id: expected receive TID flow
 * @tidpair_order: number of expected TID pairs (MEs) used is (4*2^(order-1))
 * @me_base: ME base for the TID entries of this TID flow
 * @job_key: job_key for receiving KDETH packets
 * @job_key_mask: job_key mask for receiving KDETH packets
 * @seq_num: Sequence Number
 * @gen_val: Generation Value
 * @eq_handle: EQ handle to receive RX events
 * @flags: Various flag_t's in Expected PTE (see above).
 */
struct hfi_pt_alloc_expected_args {
	u8	flow_id;
	u8	tidpair_order;
	u16	me_base;
	u16	job_key;
	u16	job_key_mask;
	u16	seq_num;
	u32	gen_val;
	struct hfi_eq   *eq_handle;
	u64	flags;
};

static inline
int hfi_pt_eager_entry_init(union ptentry_fp0_bc1_et0 *ptentry,
			    struct hfi_pt_alloc_eager_args *args)
{
	u16 eq_handle;

	if (args->eager_order > HFI_RX_EAGER_MAX_ORDER)
		return -EINVAL;

	eq_handle = args->eq_handle ? args->eq_handle->idx : PTL_EQ_NONE;

	if (eq_handle >= HFI_NUM_NIS * HFI_NUM_EVENT_HANDLES)
		return -EINVAL;

	/*
	 * Initialize the PT entry
	 */
	ptentry->val[0] = 0;
	ptentry->val[1] = 0;
	ptentry->eager_head = 0;
	ptentry->eager_tail = 0;
	ptentry->rcv_hdr_entry_sz = 2; /* 64 byte event */
	ptentry->eq_handle = eq_handle;
	ptentry->rcvhdrsz = 9; /* TODO ctx->kdeth_size; */
	ptentry->eager_cnt = args->eager_order;
	/* allow dropped eager payload to result in RHF.TidErr */
	ptentry->dont_drop_eager_full = HFI_PT_TEST_FLAG(
		args->flags, HFI_PT_EAGER_FLAG_DONT_DROP_EAGER_FULL);
	ptentry->enable = 1;
	ptentry->job_key_mask = args->job_key_mask;
	ptentry->job_key = args->job_key;
	ptentry->jk = HFI_PT_TEST_FLAG(
		args->flags, HFI_PT_EAGER_FLAG_ENA_JOB_KEY);
	ptentry->re = 0; /* do not redirect errors to context 0 */
	/* with fc=1, firmware will properly test if space in RHQ */
	ptentry->fc = 1;
	ptentry->bc = 1; /* bypass */
	ptentry->etid = 0; /* eager */
	ptentry->dont_drop_rhqf = HFI_PT_TEST_FLAG(
		args->flags, HFI_PT_EAGER_FLAG_DONT_DROP_RHQF);
	ptentry->v = 1;

	return 0;
}

static inline
int hfi_pt_expected_entry_init(union ptentry_fp0_bc1_et1 *ptentry,
			       struct hfi_pt_alloc_expected_args *args)
{
	u16 eq_handle;

	if (args->flow_id >= HFI_PT_BYPASS_EXPECTED_COUNT)
		return -EINVAL;
	if (args->tidpair_order > HFI_RX_EXPECTED_MAX_ORDER)
		return -EINVAL;

	eq_handle = args->eq_handle ? args->eq_handle->idx : PTL_EQ_NONE;

	if (eq_handle >= HFI_NUM_NIS * HFI_NUM_EVENT_HANDLES)
		return -EINVAL;

	/*
	 * Initialize the PT entry
	 */
	ptentry->val[0] = 0;
	ptentry->val[1] = 0;
	ptentry->val[2] = 0;
	ptentry->val[3] = 0;
	ptentry->seq_num = args->seq_num;
	ptentry->gen_val = args->gen_val;
	ptentry->flow_valid = 1;
	ptentry->hdr_supp_enabled = HFI_PT_TEST_FLAG(
		args->flags, HFI_PT_EXPECTED_FLAG_ENA_HDR_SUPP);
	ptentry->keep_after_seq_err = HFI_PT_TEST_FLAG(
		args->flags, HFI_PT_EXPECTED_FLAG_KEEP_AFTER_SEQ_ERR);
	ptentry->keep_on_gen_error = HFI_PT_TEST_FLAG(
		args->flags, HFI_PT_EXPECTED_FLAG_KEEP_ON_GEN_ERR);
	ptentry->keep_payload_on_gen_err = HFI_PT_TEST_FLAG(
		args->flags, HFI_PT_EXPECTED_FLAG_KEEP_PAYLOAD_ON_GEN_ERR);
	ptentry->ext_psn_enable = HFI_PT_TEST_FLAG(
		args->flags, HFI_PT_EXPECTED_FLAG_ENA_EXT_PSN);
	ptentry->enable = 1;
	ptentry->job_key_mask = args->job_key_mask;
	ptentry->job_key = args->job_key;
	ptentry->jk = HFI_PT_TEST_FLAG(
		args->flags, HFI_PT_EXPECTED_FLAG_ENA_JOB_KEY);
	ptentry->re = 0; /* do not redirect errors to context 0 */
	/* with fc=1, firmware will properly test if space in RHQ */
	ptentry->fc = 1;
	ptentry->bc = 1; /* bypass */
	ptentry->etid = 1; /* expected */
	ptentry->dont_drop_rhqf = HFI_PT_TEST_FLAG(
		args->flags, HFI_PT_EXPECTED_FLAG_DONT_DROP_RHQF);
	ptentry->v = 1;
	ptentry->tidbaseindex = args->me_base;
	ptentry->tidpaircnt = args->tidpair_order;
	ptentry->rcv_hdr_entry_sz = 2; /* 64 byte event */
	ptentry->eq_handle = eq_handle;
	ptentry->rcvhdrsz = 9; /* TODO ctx->kdeth_size; */

	return 0;
}

static inline
int hfi_pt_update_eager(struct hfi_ctx *ctx, struct hfi_cmdq *rx_cq,
			u16 eager_head)
{
	union ptentry_fp0_bc1_et0 ptentry;
	int rc;

	if (HFI_CTX_INVALID(ctx))
		return -ENODEV;
	if (!rx_cq)
		return -EINVAL;

	/* Initialize the PT entry */
	ptentry.val[0] = 0;
	ptentry.val[1] = 0;
	ptentry.val[2] = 0;
	ptentry.eager_head = eager_head;
	/* Mask for just the PT entry EagerHead */
	ptentry.val[3] = PTENTRY_FP0_BC1_ET0_EAGER_HEAD_MASK;

	/* Issue the PT_UPDATE_LOWER command via the RX CQ */
	do {
		rc = _hfi_pt_issue_cmd(ctx, rx_cq, PT_UPDATE_LOWER,
				       HFI_NI_BYPASS, HFI_PT_BYPASS_EAGER,
				       &ptentry.val[0]);
	} while (rc == -EAGAIN);

	if (rc < 0)
		return -EIO;

	return 0;
}

static inline
int hfi_pt_update_expected(struct hfi_ctx *ctx, struct hfi_cmdq *rx_cq,
			   struct hfi_pt_alloc_expected_args *args)
{
	union ptentry_fp0_bc1_et1 ptentry;
	int rc;

	if (HFI_CTX_INVALID(ctx))
		return -ENODEV;
	if (!args || !rx_cq)
		return -EINVAL;
	if (args->flow_id >= HFI_PT_BYPASS_EXPECTED_COUNT)
		return -EINVAL;

	/* Initialize the PT entry */
	ptentry.val[0] = 0;
	ptentry.val[1] = 0;
	ptentry.val[3] = 0;
	ptentry.seq_num = args->seq_num;
	ptentry.gen_val = args->gen_val;

	/* Mask for just the PT entry seq num and gen val */
	ptentry.val[2] = PTENTRY_FP0_BC1_ET1_SEQ_NUM_MASK |
			 PTENTRY_FP0_BC1_ET1_GEN_VAL_MASK;

	/* Issue the PT_UPDATE_LOWER command via the RX CQ */
	do {
		rc = _hfi_pt_issue_cmd(ctx, rx_cq, PT_UPDATE_LOWER,
				       HFI_NI_BYPASS, args->flow_id,
				       &ptentry.val[0]);
	} while (rc == -EAGAIN);

	if (rc < 0)
		return -EIO;

	return 0;
}

#if !defined(__KERNEL__) && !defined(EFIAPI)
static inline
int hfi_pt_alloc_eager(struct hfi_ctx *ctx, struct hfi_cmdq *rx_cq,
		       struct hfi_pt_alloc_eager_args *args)
{
	union ptentry_fp0_bc1_et0 ptentry;
	int rc;
	u64 done = 0;
	struct hfi_pt_update_lower_args pt_update_lower;

	if (HFI_CTX_INVALID(ctx))
		return -ENODEV;
	if (!args || !rx_cq)
		return -EINVAL;

	rc = hfi_pt_eager_entry_init(&ptentry, args);
	if (rc)
		return rc;

	memcpy(pt_update_lower.val, ptentry.val, sizeof(pt_update_lower.val));
	pt_update_lower.ni = HFI_NI_BYPASS;
	pt_update_lower.pt_idx = HFI_PT_BYPASS_EAGER;
	pt_update_lower.user_data = (u64)&done;

	/* Issue the PT_UPDATE_LOWER command via driver */
	rc = _hfi_pt_update_lower(ctx, &pt_update_lower);
	if (rc < 0)
		return -EIO;

	/*
	 * Busy poll for the PT_UPDATE_LOWER command
	 * completion on EQD=0 (NI=0)
	 */
	rc = hfi_eq_poll_cmd_complete(ctx, &done);
	if (rc)
		return rc;

	return 0;
}

static inline
int hfi_pt_alloc_expected(struct hfi_ctx *ctx, struct hfi_cmdq *rx_cq,
			  struct hfi_pt_alloc_expected_args *args)
{
	union ptentry_fp0_bc1_et1 ptentry;
	int rc;
	u64 done = 0;
	struct hfi_pt_update_lower_args pt_update_lower;

	if (HFI_CTX_INVALID(ctx))
		return -ENODEV;
	if (!args || !rx_cq)
		return -EINVAL;

	rc = hfi_pt_expected_entry_init(&ptentry, args);
	if (rc)
		return rc;

	pt_update_lower.ni = HFI_NI_BYPASS;
	pt_update_lower.pt_idx = args->flow_id;
	memcpy(pt_update_lower.val, ptentry.val, sizeof(pt_update_lower.val));
	pt_update_lower.user_data = (u64)&done;

	/* Issue the PT_UPDATE_LOWER command via driver */
	rc = _hfi_pt_update_lower(ctx, &pt_update_lower);
	if (rc < 0)
		return -EIO;

	/*
	 * Busy poll for the PT_UPDATE_LOWER
	 * command completion on EQD=0 (NI=0)
	 */
	rc = hfi_eq_poll_cmd_complete(ctx, &done);
	if (rc)
		return rc;

	return 0;
}
#endif
#endif
