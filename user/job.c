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
 * Intel(R) Omni-Path User RDMA Driver
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

	ud->ctx.dlid_base = HFI_LID_NONE;
	ud->ctx.pid_base = HFI_PID_NONE;
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
			ud->ctx.mode = job_info->ctx.mode;
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
	job_info->pid_mode = ud->ctx.mode;
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
			ud->ctx.pid_base = HFI_PID_NONE;
			ud->ctx.pid_count = 0;
			ud->ctx.dlid_base = HFI_LID_NONE;
			return;
		}

		down_write(&hfi_job_sem);
		BUG_ON(list_empty(&ud->job_list));
		list_del(&ud->job_list);
		up_write(&hfi_job_sem);
		ops->ctx_unreserve(&ud->ctx);

		/* clear DLID entries */
		ops->dlid_release(&ud->ctx, ud->ctx.dlid_base,
				  ud->ctx.lid_count);

		pr_info("release PID group [%u - %u] tag (%u,%u)\n",
			ud->ctx.pid_base, ud->ctx.pid_base + ud->ctx.pid_count - 1,
			ud->sid, ud->pid);
	} else {
		BUG_ON(!list_empty(&ud->job_list));
	}
}
