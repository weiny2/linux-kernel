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
 * Intel(R) Omni-Path User RDMA Driver
 */

#include <linux/rwsem.h>
#include "opa_user.h"

static LIST_HEAD(hfi_job_list);
static DECLARE_RWSEM(hfi_job_sem);

bool hfi_job_init(struct hfi_userdata *ud, u16 res_mode, u64 cookie)
{
	bool inherit = false;
	struct hfi_userdata *job_info;

	ud->ctx.dlid_base = HFI_LID_NONE;
	ud->ctx.pid_base = HFI_PID_NONE;
	INIT_LIST_HEAD(&ud->job_list);

	/* search job_list for PID reservation to inherit */
	/* TODO - may need to implement reference count */
	down_read(&hfi_job_sem);
	list_for_each_entry(job_info, &hfi_job_list, job_list) {
		if (res_mode != job_info->job_res_mode) {
			/* unsupported mode to inherit Job reservation */
			continue;
		}

		if ((res_mode == HFI_JOB_RES_SESSION) &&
		    (ud->sid == job_info->sid))
			inherit = true;
		else if ((res_mode == HFI_JOB_RES_USER_COOKIE) &&
			 (HFI_JOB_MAKE_COOKIE(ud->ctx.ptl_uid, cookie) ==
			  job_info->job_res_cookie))
			inherit = true;
		else
			inherit = false;

		if (inherit) {
			ud->ctx.dlid_base = job_info->ctx.dlid_base;
			ud->ctx.lid_offset = job_info->ctx.lid_offset;
			ud->ctx.lid_count = job_info->ctx.lid_count;
			ud->ctx.pid_base = job_info->ctx.pid_base;
			ud->ctx.pid_count = job_info->ctx.pid_count;
			ud->ctx.pid_total = job_info->ctx.pid_total;
			ud->ctx.mode = job_info->ctx.mode;
			ud->ctx.auth_mask = job_info->ctx.auth_mask;
			memcpy(ud->ctx.auth_uid, job_info->ctx.auth_uid,
			       sizeof(ud->ctx.auth_uid));
			/*
			 * replace default UID (used for TPID_CAM if PIDs
			 * virtualized)
			 */
			ud->ctx.ptl_uid = TPID_UID(&ud->ctx);
			pr_info("joined PID group [%u - %u] tag (%u) UID %d\n",
				ud->ctx.pid_base,
				ud->ctx.pid_base + ud->ctx.pid_count - 1,
				job_info->sid, ud->ctx.ptl_uid);
			break;
		} else {
			pr_info("skipped PID group [%u - %u] tag (%u,%u) != (%u,%u)\n",
				ud->ctx.pid_base,
				ud->ctx.pid_base + ud->ctx.pid_count - 1,
				job_info->sid, job_info->pid,
				ud->sid, ud->pid);
		}
	}
	up_read(&hfi_job_sem);

	return inherit;
}

int hfi_job_info(struct hfi_userdata *ud, struct hfi_job_info *job_info)
{
	job_info->dlid_base = ud->ctx.dlid_base;
	job_info->lid_offset = ud->ctx.lid_offset;
	job_info->lid_count = ud->ctx.lid_count;
	job_info->pid_base = ud->ctx.pid_base;
	job_info->pid_count = ud->ctx.pid_count;
	job_info->pid_total = ud->ctx.pid_total;
	job_info->pid_mode = ud->ctx.mode;
	memcpy(job_info->auth_uid, ud->ctx.auth_uid, sizeof(ud->ctx.auth_uid));
	return 0;
}

int hfi_job_setup(struct hfi_userdata *ud, struct hfi_job_setup_args *job_setup)
{
	int ret;
	u16 pid_base;
	struct opa_core_ops *ops = ud->uc->ops;

	if (!capable(CAP_SYS_ADMIN))
		return -EACCES;

	mutex_lock(&ud->lock);
	if (!list_empty(&ud->job_list)) {
		ret = -EBUSY;
		goto done;
	}

	/*
	 * Set Authentication PTL_UIDs allowed for job, set here
	 * before reserving PIDs
	 */
	ret = ops->ctx_set_allowed_uids(&ud->ctx, job_setup->auth_uid,
					HFI_NUM_AUTH_TUPLES);
	if (ret)
		goto done;

	pid_base = job_setup->pid_base;
	ret = ops->ctx_reserve(&ud->ctx, &pid_base, job_setup->pid_count,
			       job_setup->pid_align, job_setup->flags);
	if (ret)
		goto done;
	ud->ctx.pid_base = pid_base;
	ud->ctx.pid_count = job_setup->pid_count;

	/*
	 * Store other resource manager parameters
	 * These are to assist the application in computing hfi_process_t
	 * for target endpoints when LIDs and PIDs are virtualized.
	 */
	ud->ctx.pid_total = job_setup->pid_total;
	ud->ctx.lid_offset = job_setup->lid_offset;
	ud->ctx.lid_count = job_setup->lid_count;

	/* insert into job_list (active job state) */
	ud->job_res_mode = job_setup->res_mode;
	ud->job_res_cookie = job_setup->res_cookie;
	down_write(&hfi_job_sem);
	list_add(&ud->job_list, &hfi_job_list);
	up_write(&hfi_job_sem);

	pr_info("created PID group [%u - %u] UID [%u] tag (%u:%u,%u)\n",
		ud->ctx.pid_base, ud->ctx.pid_base + ud->ctx.pid_count - 1,
		TPID_UID(&ud->ctx), ud->job_res_mode, ud->sid, ud->pid);
done:
	mutex_unlock(&ud->lock);

	return ret;
}

void hfi_job_free(struct hfi_userdata *ud)
{
	struct opa_core_ops *ops = ud->uc->ops;

	mutex_lock(&ud->lock);
	if (!list_empty(&ud->job_list)) {
		pr_info("release PID group [%u - %u] count %u tag (%u,%u)\n",
			ud->ctx.pid_base,
			ud->ctx.pid_base + ud->ctx.pid_count - 1,
			ud->ctx.pid_count, ud->sid, ud->pid);

		/* release PID reservation */
		down_write(&hfi_job_sem);
		list_del(&ud->job_list);
		up_write(&hfi_job_sem);
		ops->ctx_unreserve(&ud->ctx);

		/* clear DLID entries */
		ops->dlid_release(&ud->ctx);
	} else if (ud->ctx.pid_count) {
		/* not reservation owner, just cleanup */
		ud->ctx.pid_base = HFI_PID_NONE;
		ud->ctx.pid_count = 0;
		ud->ctx.dlid_base = HFI_LID_NONE;
	}
	mutex_unlock(&ud->lock);
}
