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
#include "opa_user.h"

static LIST_HEAD(hfi_job_list);
static DECLARE_RWSEM(hfi_job_sem);

void hfi_job_init(struct hfi_userdata *ud)
{
	struct hfi_userdata *job_info;
	u16 res_mode;

	ud->ctx.dlid_base = -1;
	ud->ctx.pid_base = -1;
	ud->ctx.allow_phys_dlid = 1;	/* TODO */
	ud->ctx.sl_mask = -1;		/* TODO - default allowed SLs */
	INIT_LIST_HEAD(&ud->job_list);

	/* search job_list for PID reservation to inherit */
	/* TODO - need to implement reference count */
	down_read(&hfi_job_sem);
	list_for_each_entry(job_info, &hfi_job_list, job_list) {
		res_mode = job_info->job_res_mode;
		if (res_mode != HFI_JOB_RES_SESSION) {
			/* not one of supported modes to inherit Portals reservation */
			continue;
		}

		if ((res_mode == HFI_JOB_RES_SESSION) &&
		    (ud->sid == job_info->sid)) {
			ud->ctx.dlid_base = job_info->ctx.dlid_base;
			ud->ctx.lid_offset = job_info->ctx.lid_offset;
			ud->ctx.lid_count = job_info->ctx.lid_count;
			ud->ctx.pid_base = job_info->ctx.pid_base;
			ud->ctx.pid_count = job_info->ctx.pid_count;
			ud->ctx.pid_mode = job_info->ctx.pid_mode;
			ud->ctx.sl_mask = job_info->ctx.sl_mask;
			ud->ctx.auth_mask = job_info->ctx.auth_mask;
			memcpy(ud->ctx.auth_uid, job_info->ctx.auth_uid,
			       sizeof(ud->ctx.auth_uid));
			pr_info("joined PID group [%u - %u] tag (%u)\n",
				ud->ctx.pid_base, ud->ctx.pid_base + ud->ctx.pid_count - 1,
				job_info->sid);
		} else {
			pr_info("skipped PID group [%u - %u] tag (%u,%u) != (%u,%u)\n",
				ud->ctx.pid_base, ud->ctx.pid_base + ud->ctx.pid_count - 1,
				job_info->sid, job_info->pid,
				ud->sid, ud->pid);
		}
	}
	up_read(&hfi_job_sem);
}

int hfi_job_info(struct hfi_userdata *ud, struct hfi_job_info_args *job_info)
{
	job_info->dlid_base = ud->ctx.dlid_base;
	job_info->lid_offset = ud->ctx.lid_offset;
	job_info->lid_count = ud->ctx.lid_count;
	job_info->pid_base = ud->ctx.pid_base;
	job_info->pid_count = ud->ctx.pid_count;
	job_info->pid_mode = ud->ctx.pid_mode;
	job_info->sl_mask = ud->ctx.sl_mask;
	memcpy(job_info->auth_uid, ud->ctx.auth_uid, sizeof(ud->ctx.auth_uid));
	return 0;
}

int hfi_job_setup(struct hfi_userdata *ud, struct hfi_job_setup_args *job_setup)
{
	/* job_id, pid_base, count */
	int i, ret;
	u16 pid_base, count;
	struct opa_core_ops *ops = ud->bus_ops;

	pid_base = job_setup->pid_base;
	count = job_setup->pid_count;
	ret = 0;

	BUG_ON(!capable(CAP_SYS_ADMIN));

	ret = ops->ctx_reserve(&ud->ctx, &pid_base, count);
	if (ret)
		return ret;
	ud->ctx.pid_base = pid_base;
	ud->ctx.pid_count = count;

	/* store other resource manager parameters */
	/* TODO - TPID_CAM setup (pid_mode flag?) */
	ud->ctx.pid_mode = job_setup->pid_mode;
	ud->ctx.lid_offset = job_setup->lid_offset;
	ud->ctx.lid_count = job_setup->lid_count;
	ud->ctx.sl_mask = job_setup->sl_mask;

	for (i = 0; i < HFI_NUM_AUTH_TUPLES; i++) {
		if (job_setup->auth_uid[i]) {
			ud->ctx.auth_mask |= (1 << i);
			ud->ctx.auth_uid[i] = job_setup->auth_uid[i];
		}
	}

	/* insert into job_list (active job state) */
	ud->job_res_mode = job_setup->res_mode;
	down_write(&hfi_job_sem);
	BUG_ON(!list_empty(&ud->job_list));
	list_add(&ud->job_list, &hfi_job_list);
	up_write(&hfi_job_sem);
	pr_info("created PID group [%u - %u] UID [%u] tag (%u:%u,%u)\n",
		ud->ctx.pid_base, ud->ctx.pid_base + ud->ctx.pid_count - 1,
		(ud->ctx.auth_mask & 0x1) ? ud->ctx.auth_uid[0] : ud->ctx.ptl_uid,
		ud->job_res_mode, ud->sid, ud->pid);

	return ret;
}

void hfi_job_free(struct hfi_userdata *ud)
{
	struct opa_core_ops *ops = ud->bus_ops;

	if (ud->ctx.pid_count) {
		/* release PID reservation */

		if (ud->job_res_mode == 0) {
			/* not reservation owner, so just return */
			/* TODO - likely need to implement reference count */
			BUG_ON(!list_empty(&ud->job_list));
			ud->ctx.pid_base = -1;
			ud->ctx.pid_count = 0;
			ud->ctx.dlid_base = -1;
			return;
		}

		down_write(&hfi_job_sem);
		BUG_ON(list_empty(&ud->job_list));
		list_del(&ud->job_list);
		up_write(&hfi_job_sem);
		ops->ctx_unreserve(&ud->ctx);

		/* clear DLID entries */
		ops->dlid_release(&ud->ctx);

		pr_info("release PID group [%u - %u] tag (%u,%u)\n",
			ud->ctx.pid_base, ud->ctx.pid_base + ud->ctx.pid_count - 1,
			ud->sid, ud->pid);
	} else {
		BUG_ON(!list_empty(&ud->job_list));
	}
}
