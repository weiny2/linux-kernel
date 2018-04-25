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

#include <linux/kernel.h>
#include "hfi2.h"
#include "hfi_cmdq.h"
#include "chip/fxr_tx_ci_cid_csrs.h"
#include "chip/fxr_tx_ci_cid_csrs_defs.h"

/*
 * When added to the 8 Bytes required for the DLID relocation command,
 * this value is used to maximize the number of command slots used
 * for each individual DLID relocation table update while complying
 * with the 32B alignment requirement.
 */
#define HFI_MAX_DLIDRELOC_CMD_LEN	12

union hfi_tx_dlid_reloctable_update_t {
	struct {
		union tx_cq_a1	a;
		TXCID_CFG_DLID_RT_DATA_t payload[HFI_MAX_DLIDRELOC_CMD_LEN];
	} __attribute__ ((__packed__));
	u64	command[HFI_MAX_DLIDRELOC_CMD_LEN + 1];
} __aligned(64);

static inline
int hfi_dlid_reloc_check_entry(u64 val)
{
	if ((val & FXR_TXCID_CFG_DLID_RT_DATA_RPLC_BLK_SIZE_SMASK) ||
	    (val & FXR_TXCID_CFG_DLID_RT_DATA_RPLC_MATCH_SMASK))
		return -EINVAL;

	return 0;
}

/* Format DLID relocation table message and write it to the command queue */
static int hfi_write_dlid_reloc_cmd(struct hfi_ctx *ctx,
				    u32 dlid_base, u32 count,
				    u64 *dlid_entries_ptr)
{
	struct hfi_devdata *dd = ctx->devdata;
	union hfi_tx_dlid_reloctable_update_t dlid_reloc_cmd;
	u16 num_entries;
	u16 cmd_slots;
	u64 *p = dlid_entries_ptr;
	u8 index;
	int rc = 0;
	u8 sl;

	/* first command entry must start from a 32B aligned location */
	if ((dlid_base & 0x3) ||
	    (dlid_base >= HFI_DLID_TABLE_SIZE) ||
	    (dlid_base + count > HFI_DLID_TABLE_SIZE))
		return -EINVAL;

	/* use sl_to_mctc table on port 0 */
	rc = hfi_get_sl_for_mctc(dd->pport, 0, 0, &sl);
	if (rc < 0)
		return rc;

	memset(&dlid_reloc_cmd, 0, sizeof(dlid_reloc_cmd));

	dlid_reloc_cmd.a.dlid = dlid_base;
	dlid_reloc_cmd.a.ctype = LocalCommand;
	dlid_reloc_cmd.a.sl = sl;
	dlid_reloc_cmd.a.cmd = LOCAL_CMD_DLID_RTT;

	while (count > 0) {
		num_entries = (count > HFI_MAX_DLIDRELOC_CMD_LEN) ?
			HFI_MAX_DLIDRELOC_CMD_LEN : count;
		cmd_slots = _HFI_CMD_SLOTS(num_entries + 1);

		if (p) {
			for (index = 0; index < num_entries; index++) {
				/* They must be 0 when granularity = 1 */
				if (hfi_dlid_reloc_check_entry(*p))
					return -EINVAL;

				dlid_reloc_cmd.payload[index].val = *p;
				p++;
			}
		}

		dlid_reloc_cmd.a.cmd_length = num_entries + 1;

		/* Queue it to be written, wait for completion */
		rc = hfi_pend_cq_queue_wait(&dd->pend_cq,
					    &dd->priv_tx_cq,
					    NULL,
					    &dlid_reloc_cmd.command,
					    cmd_slots);
		if (rc < 0) {
			dd_dev_err(dd,
				   "DLID relocation table write failed for entry %d\n",
				   dlid_reloc_cmd.a.dlid);
			break;
		}

		count -= num_entries;
		dlid_reloc_cmd.a.dlid += num_entries;
	}

	return rc;
}

int hfi_dlid_assign(struct hfi_ctx *ctx,
		    struct hfi_dlid_assign_args *dlid_assign)
{
	int ret;

	WARN_ON(!capable(CAP_SYS_ADMIN));

	mutex_lock(&ctx->dlid_mutex);

	/* can only write DLID mapping once */
	if (ctx->dlid_base != HFI_LID_NONE) {
		ret = -EPERM;
		goto unlock;
	}

	/*
	 * if HFI_LID_ANY, select partition of DLID table based on TPID index
	 * TODO - this is maybe temporary for simics testing
	 */
	if (dlid_assign->dlid_base == HFI_LID_ANY) {
		u32 lid_part_size = HFI_DLID_TABLE_SIZE / HFI_TPID_ENTRIES;

		if (!IS_PID_VIRTUALIZED(ctx) ||
		    dlid_assign->count > lid_part_size) {
			ret = -EINVAL;
			goto unlock;
		}
		dlid_assign->dlid_base = ctx->tpid_idx * lid_part_size;
	}

	/* write DLID relocation table */
	ret = hfi_write_dlid_reloc_cmd(ctx, dlid_assign->dlid_base,
				       dlid_assign->count,
				       dlid_assign->dlid_entries_ptr);
	if (ret < 0)
		goto unlock;

	ctx->dlid_base = dlid_assign->dlid_base;
	ctx->mode |= HFI_CTX_MODE_LID_VIRTUALIZED;

unlock:
	mutex_unlock(&ctx->dlid_mutex);
	return ret;
}

int hfi_dlid_release(struct hfi_ctx *ctx)
{
	int ret;

	WARN_ON(!capable(CAP_SYS_ADMIN));

	mutex_lock(&ctx->dlid_mutex);

	if (ctx->dlid_base == HFI_LID_NONE) {
		ret = -EINVAL;
		goto unlock;
	}

	ret = hfi_write_dlid_reloc_cmd(ctx, ctx->dlid_base,
				       ctx->lid_count, NULL);
	if (ret < 0)
		goto unlock;

	ctx->dlid_base = HFI_LID_NONE;
	ctx->mode &= ~HFI_CTX_MODE_LID_VIRTUALIZED;

unlock:
	mutex_unlock(&ctx->dlid_mutex);
	return ret;
}

int hfi_ctx_set_allowed_uids(struct hfi_ctx *ctx, u32 *auth_uid,
			     u8 num_uids)
{
	struct hfi_devdata *dd = ctx->devdata;
	struct hfi_ctx *tpid_ctx;
	int i, ret = 0;

	if (num_uids > HFI_NUM_AUTH_TUPLES)
		return -EINVAL;

	/*
	 * verify list of UIDs does not conflict with UIDs already in TPID_CAM
	 * table in order to prevent remapping into another job's PID range.
	 */
	spin_lock(&dd->ptl_lock);
	idr_for_each_entry(&dd->ptl_tpid, tpid_ctx, i) {
		for (i = 0; i < num_uids; i++) {
			if (auth_uid[i] &&
			    TPID_UID(tpid_ctx) == auth_uid[i]) {
				ret = -EACCES;
				break;
			}
		}
		if (ret < 0)
			break;
	}
	spin_unlock(&dd->ptl_lock);
	if (ret < 0)
		return ret;

	ctx->auth_mask = 0;
	/* store AUTH UIDs in hfi_ctx, ensure unused ones are cleared */
	for (i = 0; i < HFI_NUM_AUTH_TUPLES; i++) {
		if (i < num_uids && auth_uid[i]) {
			ctx->auth_mask |= (1 << i);
			ctx->auth_uid[i] = auth_uid[i];
		} else {
			ctx->auth_uid[i] = 0;
		}
	}

	return 0;
}

int hfi_ctx_set_virtual_pid_range(struct hfi_ctx *ctx)
{
	struct hfi_devdata *dd = ctx->devdata;
	struct hfi_ctx *tpid_ctx;
	int tpid_idx, i, ret = 0;

	/* Find if we have available TPID_CAM entry, first verify is unique */
	idr_preload(GFP_KERNEL);
	spin_lock(&dd->ptl_lock);
	idr_for_each_entry(&dd->ptl_tpid, tpid_ctx, i) {
		if (TPID_UID(tpid_ctx) == TPID_UID(ctx)) {
			ret = -EACCES;
			goto idr_done;
		}
	}

	tpid_idx = idr_alloc(&dd->ptl_tpid, ctx, 0, HFI_TPID_ENTRIES,
			     GFP_NOWAIT);
	if (tpid_idx < 0)
		ret = -EBUSY;
idr_done:
	spin_unlock(&dd->ptl_lock);
	idr_preload_end();
	if (ret)
		return ret;

	/* program TPID_CAM for our reserved PID range */
	hfi_tpid_enable(dd, tpid_idx, ctx->pid_base, TPID_UID(ctx));
	ctx->tpid_idx = tpid_idx;
	/*
	 * inform user which NIs are enabled for TPID_CAM
	 * TODO - driver doesn't yet allow disabling for a given NI
	 */
	ctx->mode |= HFI_CTX_MODE_VTPID_MATCHING;
	ctx->mode |= HFI_CTX_MODE_VTPID_NONMATCHING;

	return 0;
}

void hfi_ctx_unset_virtual_pid_range(struct hfi_ctx *ctx)
{
	struct hfi_devdata *dd = ctx->devdata;

	if (!IS_PID_VIRTUALIZED(ctx))
		return;

	hfi_tpid_disable(dd, ctx->tpid_idx);

	spin_lock(&dd->ptl_lock);
	idr_remove(&dd->ptl_tpid, ctx->tpid_idx);
	spin_unlock(&dd->ptl_lock);
	ctx->mode &= ~HFI_CTX_MODE_VTPID_MATCHING;
	ctx->mode &= ~HFI_CTX_MODE_VTPID_NONMATCHING;
}
