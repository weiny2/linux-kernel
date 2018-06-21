/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2015 Intel Corporation.
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

/*
 * Intel(R) Omni-Path Gen2 HFI PCIe Driver
 */

#include "hfi2.h"
#include "hfi_cmdq.h"

#define HFI_JKEY_MASK_SMASK	0xFFFF0000
#define HFI_JKEY_MASK		0xFFFF
#define HFI_ADMIN_JKEY_RANGE	32

/*
 * For bypass packets, JKEYs are separated into 3 buckets:
 * 1. admin privilege users use job key in range 0 to 31;
 * 2. kernel ULPs use hardcoded job key in range 32 to 63,
 *    this function is not used by kernel ULPs.
 * 3. all other users use job key 64 and above, if a job
 *    key generated is less than 64, we turn on bit 15 to
 *    push it to 32K and up.
 */
u16 hfi_generate_jkey(void)
{
	u16 jkey = from_kuid(current_user_ns(), current_uid()) & HFI_JKEY_MASK;

	if (capable(CAP_SYS_ADMIN))
		jkey &= HFI_ADMIN_JKEY_RANGE - 1;
	else if (jkey < 64)
		jkey |= BIT(15);

	return jkey;
}

static uint cq_alloc_cyclic;

/* non-fast path PTE for Eager and TID operations */
union ptentry_fp0_bc1 {
	union ptentry_fp0_bc1_et0 et0;
	union ptentry_fp0_bc1_et1 et1;
};

/*
 * Validate user-supplied @auth_table against state that may have been
 * preset by the the resource manager (RM) and is stored in @ctx.
 * If the @auth_table requested driver to assign the UID, then the table
 * is updated with the appropriate default UID, which should be from the
 * set provided by RM if any were preset.
 * On success, @auth_table must contain valid values as driver will
 * write them into the CMDQ configuration register.
 */
static int hfi_cmdq_validate_tuples(struct hfi_job_res *res,
				    struct hfi_auth_tuple *auth_table,
				    bool is_kdeth)
{
	int i, j;
	u32 auth_uid;
	u32 last_job_uid = HFI_UID_ANY;

	for (i = 0; i < HFI_NUM_AUTH_TUPLES; i++) {
		/*
		 * If using KDETH, SRANK must be RANK_ANY to note that
		 * the UID field contains a KDETH job_key;
		 */
		if (is_kdeth) {
			auth_table[i].srank = PTL_RANK_ANY;
			auth_table[i].uid = HFI_JKEY_MASK_SMASK |
					    hfi_generate_jkey();

			/*
			 * RM isn't expected to correctly form KDETH JKEYs,
			 * so always continue
			 */
			continue;
		}

		/* RANK_ANY is invalid for native protocol */
		if (auth_table[i].srank == PTL_RANK_ANY)
			return -EINVAL;

		/* user may request to let driver select UID */
		if (auth_table[i].uid == HFI_UID_ANY ||
		    auth_table[i].uid == 0) {
			/* set to appropriate default UID and continue */
			if (res->auth_uid[0])
				auth_table[i].uid = res->auth_uid[0];
			else
				auth_table[i].uid = HFI_DEFAULT_PTL_UID;
			continue;
		}

		/* if RM didn't set UIDs, require default */
		if (!res->auth_mask) {
			if (auth_table[i].uid != HFI_DEFAULT_PTL_UID)
				return -EINVAL;
			continue;
		}

		/*
		 * Else look for match in RM assigned UIDs, but try to
		 * short-circuit this search.  It is an error if user-supplied
		 * UID is not present in the set assigned by RM.
		 */
		auth_uid = auth_table[i].uid;
		if (auth_uid == last_job_uid)
			continue;
		for (j = 0; j < HFI_NUM_AUTH_TUPLES; j++) {
			if (auth_uid == res->auth_uid[j]) {
				last_job_uid = auth_uid;
				break;
			}
		}
		if (j == HFI_NUM_AUTH_TUPLES)
			return -EINVAL;
	}

	return 0;
}

static int __hfi_cmdq_assign(struct hfi_ctx *ctx, u16 *cmdq_idx)
{
	struct hfi_devdata *dd = ctx->devdata;
	int ret;
	unsigned long flags;

	idr_preload(GFP_KERNEL);
	spin_lock_irqsave(&dd->cmdq_lock, flags);
	if (cq_alloc_cyclic)
		ret = idr_alloc_cyclic(&dd->cmdq_pair, ctx, 0, HFI_CMDQ_COUNT,
				       GFP_NOWAIT);
	else
		ret = idr_alloc(&dd->cmdq_pair, ctx, 0, HFI_CMDQ_COUNT,
				GFP_NOWAIT);
	if (ret >= 0) {
		dd->cmdq_pair_num_assigned++;
		ctx->cmdq_pair_num_assigned++;
	}
	spin_unlock_irqrestore(&dd->cmdq_lock, flags);
	idr_preload_end();
	if (ret < 0)
		return ret;

	*cmdq_idx = ret;
	dd_dev_info(dd, "CMDQ pair %u assigned\n", ret);

	return 0;
}

int hfi_cmdq_assign(struct hfi_cmdq_pair *cmdq, struct hfi_ctx *ctx,
		    struct hfi_auth_tuple *auth_table)
{
	int ret;
	bool priv;

	down_read(&ctx->ctx_rwsem);

	/* verify we have an assigned context */
	if (ctx->pid == HFI_PID_NONE) {
		ret = -EPERM;
		goto unlock;
	}

	/* General and OFED DMA commands require privileged CMDQ */
	priv = (ctx->type == HFI_CTX_TYPE_KERNEL) &&
	       (ctx->mode & HFI_CTX_MODE_USE_BYPASS);

	/*
	 * some kernel clients may not need to specify UID,SRANK
	 * if no auth_table, driver will set all to default
	 */
	if (auth_table) {
		ret = hfi_cmdq_validate_tuples(&ctx->res, auth_table,
					       IS_PID_BYPASS(ctx));
		if (ret)
			goto unlock;
	}

	/* thread-safe because threads get different 'cmdq_idx' */
	ret = __hfi_cmdq_assign(ctx, &cmdq->idx);
	if (!ret) {
		hfi_cmdq_config(ctx, cmdq->idx, auth_table, !priv);
		cmdq->ctx = ctx;
	}

unlock:
	up_read(&ctx->ctx_rwsem);
	return ret;
}

int hfi_cmdq_assign_privileged(struct hfi_cmdq_pair *cmdq, struct hfi_ctx *ctx)
{
	int ret;

	/* verify system PID */
	if (ctx->pid != HFI_PID_SYSTEM)
		return -EPERM;

	ret = __hfi_cmdq_assign(ctx, &cmdq->idx);
	if (!ret) {
		hfi_cmdq_config(ctx, cmdq->idx, NULL, 0);
		cmdq->ctx = ctx;
	}
	return ret;
}

int hfi_cmdq_update(struct hfi_cmdq_pair *cmdq,
		    struct hfi_auth_tuple *auth_table)
{
	struct hfi_ctx *ctx = cmdq->ctx;
	struct hfi_devdata *dd = ctx->devdata;
	struct hfi_ctx *cmdq_ctx;
	unsigned long flags;
	int ret = 0;

	down_read(&ctx->ctx_rwsem);
	spin_lock_irqsave(&dd->cmdq_lock, flags);

	/* verify we own specified CMDQ */
	cmdq_ctx = idr_find(&dd->cmdq_pair, cmdq->idx);
	if (cmdq_ctx != ctx) {
		ret = -EINVAL;
		goto unlock;
	}

	/*
	 * 'cmdq_lock' required because two threads may
	 * come here with the same 'cmdq_idx'.
	 */
	ret = hfi_cmdq_validate_tuples(&ctx->res, auth_table,
				       IS_PID_BYPASS(ctx));
	if (!ret)
		/* write CMDQ tuple config in HFI CSRs */
		hfi_cmdq_config_tuples(ctx, cmdq->idx, auth_table);

unlock:
	spin_unlock_irqrestore(&dd->cmdq_lock, flags);
	up_read(&ctx->ctx_rwsem);

	return ret;
}

int hfi_cmdq_release(struct hfi_cmdq_pair *cmdq)
{
	struct hfi_ctx *ctx = cmdq->ctx;
	struct hfi_devdata *dd = ctx->devdata;
	int ret = 0;
	unsigned long flags;

	down_read(&ctx->ctx_rwsem);

	spin_lock_irqsave(&dd->cmdq_lock, flags);
	if (idr_find(&dd->cmdq_pair, cmdq->idx) != ctx) {
		ret = -EINVAL;
	} else {
		/* FXRTODO: Do not reuse a CQ till DV has validated CQ reuse */
		if (!dd->emulation)
			hfi_cmdq_disable(dd, cmdq->idx);
		idr_remove(&dd->cmdq_pair, cmdq->idx);
		ctx->cmdq_pair_num_assigned--;
		dd->cmdq_pair_num_assigned--;
	}
	spin_unlock_irqrestore(&dd->cmdq_lock, flags);

	up_read(&ctx->ctx_rwsem);
	return ret;
}

void hfi_cmdq_cleanup(struct hfi_ctx *ctx)
{
	struct hfi_devdata *dd = ctx->devdata;
	struct hfi_ctx *cmdq_ctx;
	int i;
	unsigned long flags;

	spin_lock_irqsave(&dd->cmdq_lock, flags);
	idr_for_each_entry(&dd->cmdq_pair, cmdq_ctx, i) {
		if (cmdq_ctx == ctx) {
			/* FXRTODO: Do not reuse a CQ till DV has validated */
			if (!dd->emulation)
				hfi_cmdq_disable(dd, i);
			idr_remove(&dd->cmdq_pair, i);
			ctx->cmdq_pair_num_assigned--;
			dd->cmdq_pair_num_assigned--;
		}
	}
	ctx->cmdq_pair_num_assigned = 0;
	spin_unlock_irqrestore(&dd->cmdq_lock, flags);
}

int hfi_cmdq_map(struct hfi_cmdq_pair *cmdq)
{
	struct hfi_cmdq *tx = &cmdq->tx;
	struct hfi_cmdq *rx = &cmdq->rx;
	ssize_t head_size;
	int rc;

	/* stash pointer to CMDQ HEAD */
	rc = hfi_ctx_hw_addr(cmdq->ctx, TOK_CMDQ_HEAD, cmdq->idx,
			     (void **)&tx->head_addr, &head_size);
	if (rc)
		goto err1;

	/* stash pointer to TX CMDQ */
	rc = hfi_ctx_hw_addr(cmdq->ctx, TOK_CMDQ_TX, cmdq->idx,
			     &tx->base, (ssize_t *)&tx->size);
	if (rc)
		goto err1;

	/* stash pointer to RX CMDQ */
	rc = hfi_ctx_hw_addr(cmdq->ctx, TOK_CMDQ_RX, cmdq->idx,
			     &rx->base, (ssize_t *)&rx->size);
	if (rc)
		goto err1;

	tx->cmdq_idx = cmdq->idx;
	tx->slots_total = HFI_CMDQ_TX_ENTRIES;
	tx->slots_avail = tx->slots_total - 1;
	tx->slot_idx = (*tx->head_addr);
	tx->sw_head_idx = tx->slot_idx;
	spin_lock_init(&tx->lock);

	rx->cmdq_idx = cmdq->idx;
	rx->head_addr = tx->head_addr + 8;
	rx->slots_total = HFI_CMDQ_RX_ENTRIES;
	rx->slots_avail = rx->slots_total - 1;
	rx->slot_idx = (*rx->head_addr);
	rx->sw_head_idx = rx->slot_idx;
	spin_lock_init(&rx->lock);

	return 0;
err1:
	return rc;
}

void hfi_cmdq_unmap(struct hfi_cmdq_pair *cmdq)
{
}

int hfi_pt_update_lower(struct hfi_ctx *ctx, u8 ni, u32 pt_idx,
			u64 *val, u64 user_data)
{
	struct hfi_devdata *dd = ctx->devdata;
	union hfi_rx_cq_command cmd;
	int cmd_slots, rc;
	u64 pt_val[4];
	union ptentry_fp0_bc1 *pte;

	pte = (union ptentry_fp0_bc1 *)val;
	/* BC=1 and FC=1 PTs only */
	if (!pte->et0.bc || !pte->et0.fc)
		return -EINVAL;
	/* Non-fast path PTs only*/
	if (pte->et0.fp)
		return -EINVAL;
	/* Don't allow user contexts to set dont_drop bits */
	if (pt_idx == HFI_PT_BYPASS_EAGER) {
		if ((ctx->type == HFI_CTX_TYPE_USER) &&
		    (pte->et0.dont_drop_rhqf ||
		     pte->et0.dont_drop_eager_full))
			return -EINVAL;
	} else {
		if ((ctx->type == HFI_CTX_TYPE_USER) &&
		    pte->et1.dont_drop_rhqf)
			return -EINVAL;
	}
	pt_val[0] = val[0];
	pt_val[1] = val[1];
	pt_val[2] = ~0ULL;
	pt_val[3] = ~0ULL;

	cmd_slots = hfi_format_rx_write64(ctx,
					  ni,
					  pt_idx,
					  PT_UPDATE_LOWER_PRIV,
					  HFI_CT_NONE,
					  pt_val,
					  user_data,
					  HFI_GEN_CC,
					  &cmd);

	rc = hfi_pend_cmd_queue(&dd->pend_cmdq, &dd->priv_cmdq.rx, NULL,
				(u64 *)&cmd, cmd_slots, GFP_KERNEL);
	return rc;
}

void __hfi_cmdq_config_all(struct hfi_devdata *dd)
{
	/* FXRTODO: Do not reuse a CQ till DV has validated CQ reuse */
	if (dd->emulation)
		cq_alloc_cyclic = 1;
}
