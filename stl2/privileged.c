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

#include <linux/rwsem.h>
#include <linux/sched.h>
#include "../common/hfi.h"

static LIST_HEAD(hfi_job_list);
static DECLARE_RWSEM(hfi_job_sem);

int hfi_dlid_assign(struct hfi_userdata *ud, struct hfi_dlid_assign_args *dlid_assign)
{
	struct hfi_devdata *dd = ud->devdata;
	void *cq_base;

	BUG_ON(!capable(CAP_SYS_ADMIN));

	/* must be called after JOB_SETUP */
	if (list_empty(&ud->job_list) ||
	    (dlid_assign->count != ud->lid_count))
		return -EINVAL;

	/* can only write DLID mapping once */
	if (ud->dlid_base != HFI_LID_ANY)
		return -EPERM;

	/* TODO - write DLID table via TX CQ */
	cq_base = dd->cq_tx_base;

	ud->dlid_base = dlid_assign->dlid_base;
	return 0;
}

int hfi_dlid_release(struct hfi_userdata *ud)
{
	BUG_ON(!capable(CAP_SYS_ADMIN));

	ud->dlid_base = HFI_LID_ANY;
	/* TODO - write DLID table via TX CQ */
	return 0;
}

void hfi_job_init(struct hfi_userdata *ud)
{
	struct hfi_userdata *job_info;
	u16 res_mode;

	ud->dlid_base = -1;
	ud->pid_base = -1;
	ud->allow_phys_dlid = 1;	/* TODO */
	ud->sl_mask = -1;		/* TODO - default allowed SLs */
	INIT_LIST_HEAD(&ud->job_list);

	/* search job_list for PID reservation to inherit */
	/* TODO - need to implement reference count */
	down_read(&hfi_job_sem);
	list_for_each_entry(job_info, &hfi_job_list, job_list) {
		res_mode = job_info->res_mode;
		if (res_mode != HFI_JOB_RES_SESSION) {
			/* not one of supported modes to inherit Portals reservation */
			continue;
		}

		if ((res_mode == HFI_JOB_RES_SESSION) &&
		    (ud->sid == job_info->sid)) {
			ud->dlid_base = job_info->dlid_base;
			ud->lid_offset = job_info->lid_offset;
			ud->lid_count = job_info->lid_count;
			ud->pid_base = job_info->pid_base;
			ud->pid_count = job_info->pid_count;
			ud->pid_mode = job_info->pid_mode;
			ud->sl_mask = job_info->sl_mask;
			ud->auth_mask = job_info->auth_mask;
			memcpy(ud->auth_uid, job_info->auth_uid,
			       sizeof(ud->auth_uid));
			dd_dev_info(ud->devdata,
				    "joined PID group [%u - %u] tag (%u)\n",
				    ud->pid_base, ud->pid_base + ud->pid_count - 1,
				    job_info->sid);
		} else {
			dd_dev_info(ud->devdata,
				    "skipped PID group [%u - %u] tag (%u,%u) != (%u,%u)\n",
				    ud->pid_base, ud->pid_base + ud->pid_count - 1,
				    job_info->sid, job_info->pid,
				    ud->sid, ud->pid);
		}
	}
	up_read(&hfi_job_sem);
}

int hfi_job_info(struct hfi_userdata *ud, struct hfi_job_info_args *job_info)
{
	job_info->dlid_base = ud->dlid_base;
	job_info->lid_offset = ud->lid_offset;
	job_info->lid_count = ud->lid_count;
	job_info->pid_base = ud->pid_base;
	job_info->pid_count = ud->pid_count;
	job_info->pid_mode = ud->pid_mode;
	job_info->sl_mask = ud->sl_mask;
	memcpy(job_info->auth_uid, ud->auth_uid, sizeof(ud->auth_uid));
	return 0;
}

int hfi_job_setup(struct hfi_userdata *ud, struct hfi_job_setup_args *job_setup)
{
	/* job_id, pid_base, count */
	int i, ret;
	u16 pid_base, count;

	pid_base = job_setup->pid_base;
	count = job_setup->pid_count;
	ret = 0;

	BUG_ON(!capable(CAP_SYS_ADMIN));

	/* only one PID reservation */
	if (ud->pid_count)
		return -EPERM;

	ret = hfi_ptl_reserve(ud->devdata, &pid_base, count);
	if (ret)
		return ret;
	ud->pid_base = pid_base;
	ud->pid_count = count;
	/* TODO - TPID_CAM setup (pid_mode flag?) */
	ud->pid_mode = job_setup->pid_mode;
	ud->lid_offset = job_setup->lid_offset;
	ud->lid_count = job_setup->lid_count;
	ud->sl_mask = job_setup->sl_mask;

	for (i = 0; i < HFI_NUM_AUTH_TUPLES; i++) {
		if (job_setup->auth_uid[i]) {
			ud->auth_mask |= (1 << i);
			ud->auth_uid[i] = job_setup->auth_uid[i];
		}
	}

	/* insert into job_list (active job state) */
	ud->res_mode = job_setup->res_mode;
	down_write(&hfi_job_sem);
	BUG_ON(!list_empty(&ud->job_list));
	list_add(&ud->job_list, &hfi_job_list);
	up_write(&hfi_job_sem);
	dd_dev_info(ud->devdata,
		    "created PID group [%u - %u] UID [%u] tag (%u:%u,%u)\n",
		    ud->pid_base, ud->pid_base + ud->pid_count - 1,
		    (ud->auth_mask & 0x1) ? ud->auth_uid[0] : ud->ptl_uid,
		    ud->res_mode, ud->sid, ud->pid);

	return ret;
}

void hfi_job_free(struct hfi_userdata *ud)
{
	if (ud->pid_count) {
		/* release PID reservation */

		if (ud->res_mode == 0) {
			/* not reservation owner, so just return */
			/* TODO - likely need to implement reference count */
			BUG_ON(!list_empty(&ud->job_list));
			goto post_release;
		}

		down_write(&hfi_job_sem);
		BUG_ON(list_empty(&ud->job_list));
		list_del(&ud->job_list);
		up_write(&hfi_job_sem);
		hfi_ptl_unreserve(ud->devdata, ud->pid_base, ud->pid_count);

		/* clear DLID entries */
		hfi_dlid_release(ud);

		dd_dev_info(ud->devdata,
			    "release PID group [%u - %u] tag (%u,%u)\n",
			    ud->pid_base, ud->pid_base + ud->pid_count - 1,
			    ud->sid, ud->pid);
post_release:
		ud->pid_base = -1;
		ud->pid_count = 0;
		ud->dlid_base = -1;
	} else {
		BUG_ON(!list_empty(&ud->job_list));
	}
}
