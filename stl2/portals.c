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

#include <linux/bitmap.h>
#include "../common/hfi.h"
#include "../common/hfi_token.h"

/* TODO - temporary as FXR model has no IOMMU yet */
static int ptl_phys_pages = 1;

static int hfi_pid_alloc(struct hfi_userdata *ud, u16 *ptl_pid);
static void hfi_pid_free(struct hfi_devdata *dd, u16 ptl_pid);

static int hfi_cq_assign_next(struct hfi_userdata *ud,
			      u16 ptl_pid, int *out_cq_idx)
{
	struct hfi_devdata *dd = ud->devdata;
	unsigned cq_idx, num_cqs = HFI_CQ_COUNT;

#if 0
	/* TODO - for now allow just one CQ pair.
	 * We can introduce HFI limits later.
	 */
	if (ud->cq_pair_num_assigned > 0)
		return -ENOSPC;
#endif

	/* search the whole CQ array, starting with next_unused */
	while (num_cqs > 0) {
		cq_idx = dd->cq_pair_next_unused++ % HFI_CQ_COUNT;
		if (dd->cq_pair[cq_idx] == HFI_PID_NONE)
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

static int hfi_cq_validate_tuples(struct hfi_userdata *ud,
				  struct hfi_auth_tuple *auth_table)
{
	int i, j;
	u32 auth_uid, last_job_uid = HFI_UID_ANY;

	/* validate auth_tuples */
	/* TODO - some rework here when we fully understand UID management */
	for (i = 0; i < HFI_NUM_AUTH_TUPLES; i++) {
		auth_uid = auth_table[i].uid;

		/* user may request to let driver select UID */
		if (auth_uid == HFI_UID_ANY || auth_uid == 0)
			auth_uid = auth_table[i].uid = ud->ptl_uid;

		/* if job_launcher didn't set UIDs, this must match default */
		if (ud->auth_mask == 0) {
			if (auth_uid != ud->ptl_uid)
				return -EINVAL;
			continue;
		}

		/* else look for match in job_launcher set UIDs, 
		 * but try to short-circuit this search.
		 */
		if (auth_uid == last_job_uid)
			continue;
		for (j = 0; j < HFI_NUM_AUTH_TUPLES; j++) {
			if (auth_uid == ud->auth_uid[j]) {
				last_job_uid = auth_uid;
				break;
			}
		}
		if (j == HFI_NUM_AUTH_TUPLES)
			return -EINVAL;
	}

	return 0;
}

int hfi_cq_assign(struct hfi_userdata *ud, struct hfi_cq_assign_args *cq_assign)
{
	struct hfi_devdata *dd = ud->devdata;
	u64 addr;
	int cq_idx, ret;
	unsigned long flags;

	/* verify we are attached to Portals */
	if (ud->ptl_pid == HFI_PID_NONE)
		return -EPERM;

	ret = hfi_cq_validate_tuples(ud, cq_assign->auth_table);
	if (ret)
		return ret;

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
						PAGE_ALIGN(HFI_CQ_RX_SIZE));
	addr = (u64)HFI_CQ_HEAD_ADDR(dd->cq_head_base, cq_idx);
	cq_assign->cq_head_token = HFI_MMAP_TOKEN(TOK_CQ_HEAD, cq_idx, addr,
						  PAGE_SIZE);

	/* write CQ config in HFI CSRs */
	hfi_cq_config(ud, cq_idx, dd->cq_head_base, cq_assign->auth_table);

	return 0;
}

int hfi_cq_update(struct hfi_userdata *ud, struct hfi_cq_update_args *cq_update)
{
	struct hfi_devdata *dd = ud->devdata;
	int ret = 0;

	/* verify we are attached to Portals */
	if (ud->ptl_pid == HFI_PID_NONE)
		return -EPERM;

	/* verify we own specified CQ */
	if (ud->ptl_pid != dd->cq_pair[cq_update->cq_idx])
		return -EINVAL;

	ret = hfi_cq_validate_tuples(ud, cq_update->auth_table);
	if (ret)
		return ret;

	/* write CQ tuple config in HFI CSRs */
	hfi_cq_config_tuples(ud, cq_update->cq_idx, cq_update->auth_table);

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
		dd->cq_pair[cq_idx] = HFI_PID_NONE;
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
			dd->cq_pair[i] = HFI_PID_NONE;
		}
	}
	ud->cq_pair_num_assigned = 0;
	spin_unlock_irqrestore(&dd->cq_lock, flags);
}

int hfi_ptl_reserve(struct hfi_devdata *dd, u16 *base, u16 count)
{
	u16 start, n;
	int ret = 0;
	unsigned long flags;

	start = (*base == HFI_PID_ANY) ? 0 : *base;

	spin_lock_irqsave(&dd->ptl_lock, flags);
	n = bitmap_find_next_zero_area(dd->ptl_map, HFI_NUM_PIDS,
				       start, count, 0);
	if (n == -1)
		ret = -EBUSY;
	if (*base != HFI_PID_ANY && n != *base)
		ret = -EBUSY;
	if (ret) {
		spin_unlock_irqrestore(&dd->ptl_lock, flags);
		return ret;
	}
	bitmap_set(dd->ptl_map, n, count);
	spin_unlock_irqrestore(&dd->ptl_lock, flags);

	*base = n;
	return 0;
}

void hfi_ptl_unreserve(struct hfi_devdata *dd, u16 base, u16 count)
{
	unsigned long flags;

	spin_lock_irqsave(&dd->ptl_lock, flags);
	bitmap_clear(dd->ptl_map, base, count);
	spin_unlock_irqrestore(&dd->ptl_lock, flags);
	return;
}

/*
 * Associate this process with a Portals PID.
 * Note, hfi_open() sets:
 *   ud->pid = current->pid
 *   ud->ptl_pid = HFI_PID_NONE
 *   ud->ptl_uid = current_uid()
 */
int hfi_ptl_attach(struct hfi_userdata *ud,
		   struct hfi_ptl_attach_args *ptl_attach)
{
	struct hfi_devdata *dd = ud->devdata;
	u16 ptl_pid;
	u32 psb_size, trig_op_size, le_me_off, le_me_size, unexp_size;
	int ret;

	/* only one Portals PID allowed */
	if (ud->ptl_pid != HFI_PID_NONE)
		return -EPERM;

	ptl_pid = ptl_attach->pid;
	ret = hfi_pid_alloc(ud, &ptl_pid);
	if (ret)
		return ret;

	/* set ptl_pid, hfi_ptl_cleanup() can handle all errors below */
	ud->ptl_pid = ptl_pid;

	/* verify range of inputs */
	if (ptl_attach->trig_op_count > HFI_TRIG_OP_MAX_COUNT)
		ptl_attach->trig_op_count = HFI_TRIG_OP_MAX_COUNT;
	if (ptl_attach->le_me_count > HFI_LE_ME_MAX_COUNT)
		ptl_attach->le_me_count = HFI_LE_ME_MAX_COUNT;
	if (ptl_attach->unexpected_count > HFI_UNEXP_MAX_COUNT)
		ptl_attach->unexpected_count = HFI_UNEXP_MAX_COUNT;

	/* compute total Portals State Base size */
	trig_op_size = PAGE_ALIGN(ptl_attach->trig_op_count * HFI_TRIG_OP_SIZE);
	le_me_size = PAGE_ALIGN(ptl_attach->le_me_count * HFI_LE_ME_SIZE);
	unexp_size = PAGE_ALIGN(ptl_attach->unexpected_count * HFI_UNEXP_SIZE);
	le_me_off = HFI_PSB_FIXED_TOTAL_MEM + trig_op_size;
	psb_size = le_me_off + le_me_size + unexp_size;

	/* vmalloc Portals State memory, will store in PCB */
	if (!ptl_phys_pages)
		ud->ptl_state_base = vmalloc_user(psb_size);
	else
		ud->ptl_state_base = (void *)__get_free_pages(
							GFP_KERNEL | __GFP_ZERO,
							get_order(psb_size));
	if (!ud->ptl_state_base) {
		ret = -ENOMEM;
		goto err_vmalloc;
	}
	ud->ptl_state_size = psb_size;
	ud->ptl_trig_op_size = trig_op_size;
	ud->ptl_le_me_base = ud->ptl_state_base + le_me_off;
	ud->ptl_le_me_size = le_me_size;
	ud->ptl_unexpected_size = unexp_size;

	/* TODO - init IOMMU PASID <-> PID mapping */
	ud->pasid = ptl_pid;
	dd_dev_info(dd, "Portals PID %u assigned PCB:[%d, %d, %d, %d]\n", ptl_pid,
		    psb_size, trig_op_size, le_me_size, unexp_size);

	/* write PCB (host memory) */
	hfi_pcb_write(ud, ptl_pid, ptl_phys_pages);

	/* return mmap tokens of PSB items */
	ptl_attach->ct_token = HFI_MMAP_PSB_TOKEN(TOK_EVENTS_CT,
						ptl_pid, HFI_PSB_CT_SIZE);
	ptl_attach->eq_desc_token = HFI_MMAP_PSB_TOKEN(TOK_EVENTS_EQ_DESC,
						ptl_pid, HFI_PSB_EQ_DESC_SIZE);
	ptl_attach->eq_head_token = HFI_MMAP_PSB_TOKEN(TOK_EVENTS_EQ_HEAD,
						ptl_pid, HFI_PSB_EQ_HEAD_SIZE);
	ptl_attach->pt_token = HFI_MMAP_PSB_TOKEN(TOK_PORTALS_TABLE,
						ptl_pid, HFI_PSB_PT_SIZE);
	ptl_attach->le_me_token = HFI_MMAP_PSB_TOKEN(TOK_LE_ME,
						ptl_pid, ud->ptl_le_me_size);
	ptl_attach->unexpected_token = HFI_MMAP_PSB_TOKEN(TOK_UNEXPECTED,
						ptl_pid, ud->ptl_unexpected_size);
	ptl_attach->trig_op_token = HFI_MMAP_PSB_TOKEN(TOK_TRIG_OP,
						ptl_pid, ud->ptl_trig_op_size);
	ptl_attach->pid = ptl_pid;
	ptl_attach->pid_base = ud->pid_base;
	ptl_attach->pid_count = ud->pid_count;
	ptl_attach->pid_mode = ud->pid_mode;

	return 0;

err_vmalloc:
	hfi_ptl_cleanup(ud);
	return ret;
}

static int hfi_pid_alloc(struct hfi_userdata *ud, u16 *assigned_pid)
{
	unsigned long flags;
	int ret;
	struct hfi_devdata *dd = ud->devdata;
	u16 ptl_pid = *assigned_pid;

	if (ud->pid_count) {
		/* assign PID from Portals PID reservation */
		if ((ptl_pid != HFI_PID_ANY) &&
		    (ptl_pid >= ud->pid_count))
			return -EINVAL;

		spin_lock_irqsave(&dd->ptl_lock, flags);
		if (ptl_pid == HFI_PID_ANY) {
			/* if PID_ANY, search reserved PIDs for unused one */
			for (ptl_pid = 0; ptl_pid < ud->pid_count; ptl_pid++) {
				if (dd->ptl_user[ud->pid_base + ptl_pid] == NULL)
					break;
			}
			if (ptl_pid >= ud->pid_count) {
				spin_unlock_irqrestore(&dd->ptl_lock, flags);
				return -EBUSY;
			}
			ptl_pid += ud->pid_base;
		} else {
			ptl_pid += ud->pid_base;
			if (dd->ptl_user[ptl_pid] != NULL) {
				spin_unlock_irqrestore(&dd->ptl_lock, flags);
				return -EBUSY;
			}
		}

		/* store PID hfi_userdata pointer */
		dd->ptl_user[ptl_pid] = ud;
		spin_unlock_irqrestore(&dd->ptl_lock, flags);
	} else {
		/* PID is user-specified, validate and acquire */
		if ((ptl_pid != HFI_PID_ANY) &&
		    (ptl_pid >= HFI_NUM_PIDS))
			return -EINVAL;

		ret = hfi_ptl_reserve(ud->devdata, &ptl_pid, 1);
		if (ret)
			return ret;
		dd_dev_info(ud->devdata,
			    "acquired PID orphan [%u]\n", ptl_pid);

		/* store PID hfi_userdata pointer */
		BUG_ON(dd->ptl_user[ptl_pid] != 0);
		dd->ptl_user[ptl_pid] = ud;
	}

	*assigned_pid = ptl_pid;
	return 0;
}

static void hfi_pid_free(struct hfi_devdata *dd, u16 ptl_pid)
{
	unsigned long flags;

	spin_lock_irqsave(&dd->ptl_lock, flags);
	hfi_pcb_reset(dd, ptl_pid);
	dd->ptl_user[ptl_pid] = 0;
	spin_unlock_irqrestore(&dd->ptl_lock, flags);
	dd_dev_info(dd, "Portals PID %u released\n", ptl_pid);
}

/* Release Portals PID resources.
 * Called during close() or explicitly via CMD_PTL_DETACH.
 */
void hfi_ptl_cleanup(struct hfi_userdata *ud)
{
	struct hfi_devdata *dd = ud->devdata;
	u16 ptl_pid = ud->ptl_pid;

	if (ptl_pid == HFI_PID_NONE)
		/* no assigned PID */
		return;

	/* verify PID is in correct state */
	BUG_ON(dd->ptl_user[ptl_pid] != ud);

	/* first release any assigned CQs */
	hfi_cq_cleanup(ud);

	/* release assigned PID */
	hfi_pid_free(dd, ptl_pid);

	if (ud->ptl_state_base) {
		if (!ptl_phys_pages)
			vfree(ud->ptl_state_base);
		else
			free_pages((unsigned long)ud->ptl_state_base,
				   get_order(ud->ptl_state_size));
	}
	ud->ptl_state_base = NULL;
	ud->ptl_le_me_base = NULL;

	if (ud->pid_count == 0) {
		dd_dev_info(ud->devdata,
			    "release PID orphan [%u]\n", ptl_pid);
		hfi_ptl_unreserve(ud->devdata, ptl_pid, 1);
	}

	/* clear last */
	ud->ptl_pid = HFI_PID_NONE;
}
