/*
 * Copyright (c) 2014 Intel Corporation.  All rights reserved.
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
#include "hfi_token.h"
#include "fxr.h"

static void hfi_ptl_pid_free(struct hfi_devdata *dd, hfi_ptl_pid_t ptl_pid);

static int hfi_cq_assign_next(struct hfi_userdata *ud,
			      hfi_ptl_pid_t ptl_pid, int *out_cq_idx)
{
	struct hfi_devdata *dd = ud->devdata;
	unsigned cq_idx, num_cqs = HFI_CQ_COUNT;

	/* TODO - for now allow just one CQ pair.
	 * We can introduce HFI limits later.
	 */
	if (ud->cq_pair_num_assigned > 0)
		return -ENOSPC;

	/* search the whole CQ array, starting with next_unused */
	while (num_cqs > 0) {
		cq_idx = dd->cq_pair_next_unused++ % HFI_CQ_COUNT;
		if (dd->cq_pair[cq_idx] == HFI_PTL_PID_NONE)
			break;
		num_cqs--;
	}
	if (num_cqs == 0) {
		/* all CQs are assigned */
		return -EBUSY;
	}

	*out_cq_idx = cq_idx;
	dd->cq_pair[cq_idx] = ptl_pid;
	ud->cq_pair_num_assigned++;
	return 0;
}

int hfi_cq_assign(struct hfi_userdata *ud, struct hfi_cq_assign_args *cq_assign)
{
	struct hfi_devdata *dd = ud->devdata;
	u64 addr;
	int cq_idx, ret;
	unsigned long flags;

	/* verify we are attached to Portals */
	if (ud->ptl_pid == HFI_PTL_PID_NONE)
		return -EPERM;

	spin_lock_irqsave(&dd->cq_lock, flags);
	ret = hfi_cq_assign_next(ud, ud->ptl_pid, &cq_idx);
	spin_unlock_irqrestore(&dd->cq_lock, flags);
	if (ret)
		return ret;

	dd_dev_info(dd, "CQ pair %u assigned\n", cq_idx);
	cq_assign->cq_idx = cq_idx;
	addr = 0; /* segment address only needed if not page-aligned */
	cq_assign->cq_tx_token = HFI_MMAP_TOKEN(TOK_CQ_TX, cq_idx, addr,
						HFI_CQ_TX_SIZE);
	cq_assign->cq_rx_token = HFI_MMAP_TOKEN(TOK_CQ_RX, cq_idx, addr,
						HFI_CQ_RX_SIZE);
	addr = (u64)HFI_CQ_HEAD_ADDR(dd->cq_head_base, cq_idx);
	cq_assign->cq_head_token = HFI_MMAP_TOKEN(TOK_CQ_HEAD, cq_idx, addr,
						  PAGE_SIZE);

	/* write CQ config in HFI CSRs */
	hfi_cq_config(ud, cq_idx, dd->cq_head_base, &cq_assign->auth_idx);

	return 0;
}

int hfi_cq_release(struct hfi_userdata *ud, u16 cq_idx)
{
	struct hfi_devdata *dd = ud->devdata;
	int ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&dd->cq_lock, flags);
	if (ud->ptl_pid != dd->cq_pair[cq_idx]) {
		ret = -EINVAL;
	} else {
		hfi_cq_disable(dd, cq_idx);
		dd->cq_pair[cq_idx] = HFI_PTL_PID_NONE;
		ud->cq_pair_num_assigned--;
		/* TODO - remove any CQ head mappings */
	}
	spin_unlock_irqrestore(&dd->cq_lock, flags);

	return ret;
}

static void hfi_cq_cleanup(struct hfi_userdata *ud)
{
	struct hfi_devdata *dd = ud->devdata;
	u64 ptl_pid = ud->ptl_pid;
	int i;
	unsigned long flags;

	spin_lock_irqsave(&dd->cq_lock, flags);
	for (i = 0; i < HFI_CQ_COUNT; i++) {
		if (ptl_pid == dd->cq_pair[i]) {
			hfi_cq_disable(dd, i);
			dd->cq_pair[i] = HFI_PTL_PID_NONE;
		}
	}
	ud->cq_pair_num_assigned = 0;
	spin_unlock_irqrestore(&dd->cq_lock, flags);
}

/*
 * Associate this process with a Portals PID.
 * Note, hfi_open() sets:
 *   ud->pid = current->pid
 *   ud->ptl_pid = HFI_PTL_PID_NONE
 *   ud->ptl_uid = current_uid()
 */
int hfi_ptl_attach(struct hfi_userdata *ud,
		   struct hfi_ptl_attach_args *ptl_attach)
{
	struct hfi_devdata *dd = ud->devdata;
	hfi_ptl_pid_t ptl_pid;
	size_t psb_size;
	u16 trig_op_entries;
	unsigned long flags;

	/* only one Portals PID allowed */
	if (ud->ptl_pid != HFI_PTL_PID_NONE)
		return -EPERM;

	/* PID is user-specified, validate and acquire */
	/* TODO - will likely change when we understand Resource Manager */
	ptl_pid = ptl_attach->pid;
	if (ptl_pid >= HFI_NUM_PTL_PIDS)
		return -EINVAL;

	spin_lock_irqsave(&dd->ptl_control_lock, flags);
	if (dd->ptl_pid_user[ptl_pid] != 0) {
		spin_unlock_irqrestore(&dd->ptl_control_lock, flags);
		return -EBUSY;
	}
	dd->ptl_pid_user[ptl_pid] = ud;
	spin_unlock_irqrestore(&dd->ptl_control_lock, flags);

	/* TODO - init IOMMU PASID <-> PID mapping */
	ud->pasid = ptl_pid;
	ud->ptl_pid = ptl_pid;
	ud->srank = ptl_attach->srank;
	dd_dev_info(dd, "Portals PID %u assigned\n", ptl_pid);

	/* TODO compute Portals State Base size, for now use a minimum */
	trig_op_entries = dd->trig_op_min_entries;
	psb_size = dd->ptl_state_min_size;

	/* vmalloc Portals State memory, store in PCB */
	ud->ptl_state_base = vmalloc_user(psb_size);
	if (!ud->ptl_state_base) {
		hfi_ptl_pid_free(dd, ptl_pid);
		return -ENOMEM;
	}
	ud->ptl_state_size = psb_size;

	/* write PCB (host memory) */
	/* TODO PCB overflow for LE/MEs */
	dd->ptl_control[ptl_pid].trig_op_size = trig_op_entries;
	dd->ptl_control[ptl_pid].le_me_base = 0;
	dd->ptl_control[ptl_pid].le_me_size = 0;
	dd->ptl_control[ptl_pid].unexpected_base = 0;
	dd->ptl_control[ptl_pid].unexpected_size = 0;
	dd->ptl_control[ptl_pid].portals_base = ((u64)ud->ptl_state_base >> PAGE_SHIFT);
	dd->ptl_control[ptl_pid].v = 1;

	/* return mmap tokens of PSB items */
	ptl_attach->ct_token = HFI_MMAP_PSB_TOKEN(TOK_EVENTS_CT,
						ptl_pid, HFI_PSB_CT_SIZE);
	ptl_attach->eq_desc_token = HFI_MMAP_PSB_TOKEN(TOK_EVENTS_EQ_DESC,
						ptl_pid, HFI_PSB_EQ_DESC_SIZE);
	ptl_attach->eq_head_token = HFI_MMAP_PSB_TOKEN(TOK_EVENTS_EQ_HEAD,
						ptl_pid, HFI_PSB_EQ_HEAD_SIZE);
	ptl_attach->pt_token = HFI_MMAP_PSB_TOKEN(TOK_PORTALS_TABLE,
						ptl_pid, HFI_PSB_PT_SIZE);

	return 0;
}

static void hfi_ptl_pid_free(struct hfi_devdata *dd, hfi_ptl_pid_t ptl_pid)
{
	unsigned long flags;

	spin_lock_irqsave(&dd->ptl_control_lock, flags);
	memset(&dd->ptl_control[ptl_pid], 0, sizeof(hfi_ptl_control_t));
	dd->ptl_pid_user[ptl_pid] = 0;
	spin_unlock_irqrestore(&dd->ptl_control_lock, flags);
	dd_dev_info(dd, "Portals PID %u released\n", ptl_pid);
}

void hfi_ptl_cleanup(struct hfi_userdata *ud)
{
	struct hfi_devdata *dd = ud->devdata;
	u64 ptl_pid = ud->ptl_pid;

	if (ud->ptl_pid == HFI_PTL_PID_NONE)
		return;

	/* verify PID is in correct state */
	BUG_ON(dd->ptl_pid_user[ptl_pid] != ud);

	/* first release any assigned CQs */
	hfi_cq_cleanup(ud);

	/* release assigned PID */
	hfi_ptl_pid_free(dd, ptl_pid);

	if (ud->ptl_state_base)
		vfree(ud->ptl_state_base);
	ud->ptl_state_base = NULL;
}
