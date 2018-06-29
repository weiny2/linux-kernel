/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2017-2018 Intel Corporation.
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
 * Copyright (c) 2017-2018 Intel Corporation.
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
#include "../core/hfi_core.h"
#include "uverbs_obj.h"
#include "hfi2.h"
#include "verbs.h"

static LIST_HEAD(ib_ujob_objects);
static DECLARE_RWSEM(hfi_job_sem);

bool hfi_job_init(struct hfi_ctx *ctx, u16 res_mode, u64 cookie)
{
	bool inherit = false;
	struct hfi_ctx *tmp_ctx;
	struct ib_ujob_object *tmp_obj;
	u16 sid = task_session_vnr(current);

	ctx->dlid_base = HFI_LID_NONE;
	ctx->pid_base = HFI_PID_NONE;

	/* search job_list for PID reservation to inherit */
	/* TODO - may need to implement reference count */
	down_read(&hfi_job_sem);
	list_for_each_entry(tmp_obj, &ib_ujob_objects, obj_list) {
		tmp_ctx = tmp_obj->uobject.object;
		if (res_mode != tmp_obj->job_res_mode) {
			/* unsupported mode to inherit Job reservation */
			continue;
		}

		if ((res_mode == HFI_JOB_RES_SESSION) &&
		    (sid == tmp_obj->sid))
			inherit = true;
		else if ((res_mode == HFI_JOB_RES_USER_COOKIE) &&
			 (HFI_JOB_MAKE_COOKIE(ctx->ptl_uid, cookie) ==
			  tmp_obj->job_res_cookie))
			inherit = true;
		else
			inherit = false;

		if (inherit) {
			ctx->dlid_base = tmp_ctx->dlid_base;
			ctx->lid_offset = tmp_ctx->lid_offset;
			ctx->lid_count = tmp_ctx->lid_count;
			ctx->pid_base = tmp_ctx->pid_base;
			ctx->pid_count = tmp_ctx->pid_count;
			ctx->pid_total = tmp_ctx->pid_total;
			ctx->mode = tmp_ctx->mode;
			memcpy(ctx->auth_uid, tmp_ctx->auth_uid,
			       sizeof(ctx->auth_uid));
			ctx->auth_mask = tmp_ctx->auth_mask;
			/*
			 * If auth UIDs are set by the resource manager, we
			 * replace the default UID (ctx->ptl_uid) with the
			 * first UID from this set.
			 */
			if (ctx->auth_mask)
				ctx->ptl_uid = ctx->auth_uid[0];
			pr_info("joined PID group [%u - %u] tag (%u) UID %d\n",
				tmp_ctx->pid_base,
				tmp_ctx->pid_base + tmp_ctx->pid_count - 1,
				sid, ctx->ptl_uid);
			break;
		}
	}
	up_read(&hfi_job_sem);
	return inherit;
}

int hfi_job_attach(struct hfi_ibcontext *uc, int mode, u64 cookie)
{
	/*
	 * Only allow JOB_RES_USER_COOKIE for manual job_attach.
	 * Set conditions to find job reservation during ctx_attach.
	 */
	switch (mode) {
	case HFI_JOB_RES_USER_COOKIE:
		uc->job_res_mode = mode;
		uc->job_res_cookie = cookie;
		return 0;
	default:
		return -EINVAL;
	}
}

int hfi_job_info(struct hfi_job_info *job_info)
{
	struct hfi_ctx *tmp_ctx;
	struct ib_ujob_object *tmp_obj;
	u16 res_mode;

	/*
	 * search job_list for PID reservation to inherit
	 * TODO - may need to implement reference count
	 */
	down_read(&hfi_job_sem);
	list_for_each_entry(tmp_obj, &ib_ujob_objects, obj_list) {
		tmp_ctx = tmp_obj->uobject.object;
		res_mode = tmp_obj->job_res_mode;
		if (res_mode != HFI_JOB_RES_SESSION) {
			/*
			 * not one of supported modes to inherit Portals
			 * reservation
			 */
			continue;
		}

		if ((res_mode == HFI_JOB_RES_SESSION) &&
		    (task_session_vnr(current) == tmp_obj->sid)) {
			job_info->dlid_base = tmp_ctx->dlid_base;
			job_info->lid_offset = tmp_ctx->lid_offset;
			job_info->lid_count = tmp_ctx->lid_count;
			job_info->pid_base = tmp_ctx->pid_base;
			job_info->pid_count = tmp_ctx->pid_count;
			job_info->pid_total = tmp_ctx->pid_total;
			job_info->pid_mode = tmp_ctx->mode;
			memcpy(job_info->auth_uid, tmp_ctx->auth_uid,
			       sizeof(job_info->auth_uid));
		}
	}
	up_read(&hfi_job_sem);
	return 0;
}

int hfi_job_setup(struct hfi_ctx *ctx, struct hfi_job_setup_args *job_setup)
{
#ifdef CONFIG_HFI2_STLNP
	struct ib_ujob_object *obj;
	int ret;
	u16 pid_base;

	ctx->dlid_base = HFI_LID_NONE;
	ctx->pid_base = HFI_PID_NONE;

	if (!capable(CAP_SYS_ADMIN))
		return -EACCES;

	obj = container_of(ctx->uobject, typeof(*obj), uobject);

	if (!list_empty(&obj->obj_list)) {
		ret = -EBUSY;
		goto done;
	}

	/*
	 * Set Authentication PTL_UIDs allowed for job, set here
	 * before reserving PIDs
	 */
	ret = hfi_ctx_set_allowed_uids(ctx, job_setup->auth_uid,
				       HFI_NUM_AUTH_TUPLES);
	if (ret)
		goto done;

	pid_base = job_setup->pid_base;
	ret = hfi_ctx_reserve(ctx, &pid_base, job_setup->pid_count,
			      job_setup->pid_align, job_setup->flags);
	if (ret)
		goto done;
	ctx->pid_base = pid_base;
	ctx->pid_count = job_setup->pid_count;

	/*
	 * Store other resource manager parameters
	 * These are to assist the application in computing hfi_process_t
	 * for target endpoints when LIDs and PIDs are virtualized.
	 */
	ctx->pid_total = job_setup->pid_total;
	ctx->lid_offset = job_setup->lid_offset;
	ctx->lid_count = job_setup->lid_count;

	obj->job_res_mode = job_setup->res_mode;
	obj->job_res_cookie = job_setup->res_cookie;
	obj->sid = task_session_vnr(current);

	pr_info("created PID group [%u - %u] UID [%u] tag (%u:%u)\n",
		ctx->pid_base, ctx->pid_base + ctx->pid_count - 1,
		ctx->auth_uid[0], obj->job_res_mode, obj->sid);
done:
	return ret;
#endif
	return 0;
}

/*
 * hfi_job_free() will be called from hfi2_job_free()
 * when the ib device is closed.
 *
 * ib_uverbs_close() --> ib_uverbs_cleanup_ucontext() -->
 * uverbs_cleanup_ucontext() --> remove_commit_idr_uobject() -->
 * hfi2_job_free() --> hfi_job_free()
 *
 */
void hfi_job_free(struct hfi_ctx *ctx)
{
#ifdef CONFIG_HFI2_STLNP
	struct ib_ujob_object *obj;

	obj = container_of(ctx->uobject, typeof(*obj), uobject);
	if (!list_empty(&obj->obj_list)) {
		pr_info("release PID group [%u - %u] count %u tag (%u,%u)\n",
			ctx->pid_base,
			ctx->pid_base + ctx->pid_count - 1,
			ctx->pid_count, obj->sid, ctx->pid);

		/* release PID reservation */
		down_write(&hfi_job_sem);
		list_del(&obj->obj_list);
		up_write(&hfi_job_sem);
		hfi_ctx_unreserve(ctx);

		/* clear DLID entries */
		hfi_dlid_release(ctx);
	} else if (ctx->pid_count) {
		/* not reservation owner, just cleanup */
		ctx->pid_base = HFI_PID_NONE;
		ctx->pid_count = 0;
		ctx->dlid_base = HFI_LID_NONE;
	}
#endif
}

int hfi2_job_setup_handler(struct ib_device *ib_dev,
			   struct ib_uverbs_file *file,
			   struct uverbs_attr_bundle *attrs)
{
	struct hfi_ctx *ctx;
	struct ib_ujob_object *obj;
	struct ib_ucontext *ucontext = file->ucontext;
	struct hfi_job_setup_args job_setup;
	struct hfi_devdata *dd = hfi_dd_from_ibdev(ib_dev);
	const struct uverbs_attr *uattr;
	u16 pid_base;
	int ret;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	hfi_ctx_init(ctx, dd);

	uattr = uverbs_attr_get(attrs, HFI2_JOB_SETUP_CTX_IDX);
	if (unlikely(IS_ERR(uattr)))
		return PTR_ERR(uattr);

	obj = container_of(uattr->obj_attr.uobject,
			   typeof(*obj), uobject);

	obj->verbs_file = ucontext->ufile;
	obj->uobject.object = ctx;
	ctx->uobject = &obj->uobject;
	INIT_LIST_HEAD(&obj->obj_list);

	ret = uverbs_copy_from(&job_setup, attrs, HFI2_JOB_SETUP_CMD);
	if (ret)
		goto err_job_setup;

	ret = hfi_job_setup(ctx, &job_setup);
	if (ret)
		goto err_job_setup;

	pid_base = ctx->pid_base;

	down_write(&hfi_job_sem);
	/* insert into job_list (active job state) */
	list_add(&obj->obj_list, &ib_ujob_objects);
	up_write(&hfi_job_sem);

	ret = uverbs_copy_to(attrs, HFI2_JOB_SETUP_PID_BASE,
			     &pid_base, sizeof(pid_base));
	if (ret)
		goto err_job_setup;

	return 0;

err_job_setup:
	kfree(ctx);
	return ret;
}

int hfi2_job_attach_handler(struct ib_device *ib_dev,
			    struct ib_uverbs_file *file,
			    struct uverbs_attr_bundle *attrs)
{
	struct hfi_ibcontext *uc = (struct hfi_ibcontext *)file->ucontext;
	u16 mode;
	u64 cookie;
	int ret;

	ret = uverbs_copy_from(&mode, attrs, HFI2_JOB_ATTACH_MODE);
	ret += uverbs_copy_from(&cookie, attrs, HFI2_JOB_ATTACH_COOKIE);
	if (ret)
		return ret;
	return hfi_job_attach(uc, mode, cookie);
}

int hfi2_job_info_handler(struct ib_device *ib_dev,
			  struct ib_uverbs_file *file,
			  struct uverbs_attr_bundle *attrs)
{
	struct hfi_job_info job_info = {0};

	hfi_job_info(&job_info);

	return uverbs_copy_to(attrs, HFI2_JOB_INFO_RESP,
			      &job_info, sizeof(job_info));
}

int hfi2_job_free(struct ib_uobject *uobject,
		  enum rdma_remove_reason why)
{
	struct ib_ujob_object *obj;
	struct hfi_ctx *ctx;

	obj = container_of(uobject, struct ib_ujob_object, uobject);
	if (!obj)
		return -EINVAL;

	ctx = obj->uobject.object;

	hfi_job_free(ctx);
	kfree(ctx);

	return 0;
}

int hfi2_dlid_assign_handler(struct ib_device *ib_dev,
			     struct ib_uverbs_file *file,
			     struct uverbs_attr_bundle *attrs)
{
#ifdef CONFIG_HFI2_STLNP
	struct hfi_ctx *ctx;
	struct hfi_dlid_assign_args dlid_assign;
	const struct uverbs_attr *uattr;
	u32 dlid_count;
	u64 *dlid_ptr_user;
	int ret;

	uattr = uverbs_attr_get(attrs, HFI2_DLID_ASSIGN_CTX_IDX);
	if (unlikely(IS_ERR(uattr)))
		return PTR_ERR(uattr);

	ctx = uattr->obj_attr.uobject->object;
	if (unlikely(!ctx)) {
		pr_err("%s: Encuntered a NULL hfi context\n", __func__);
		return -ENOENT;
	}

	ret = uverbs_copy_from(&dlid_assign, attrs, HFI2_DLID_ASSIGN_CMD);
	if (ret)
		return -EINVAL;

	if (dlid_assign.count != ctx->lid_count)
		return -EINVAL;

	dlid_ptr_user = dlid_assign.dlid_entries_ptr;
	dlid_count = dlid_assign.count;

	dlid_assign.count = ALIGN(dlid_count, 4);
	dlid_assign.dlid_entries_ptr =
		kcalloc(dlid_assign.count, sizeof(u64), GFP_KERNEL);

	if (ZERO_OR_NULL_PTR(dlid_assign.dlid_entries_ptr))
		return -ENOMEM;

	/*
	 * Number of entries must be 32B aligned. Remaining entries
	 * are zeroed.
	 */
	if (copy_from_user(dlid_assign.dlid_entries_ptr,
			   (void __user *)dlid_ptr_user,
			   sizeof(u64) * dlid_count))
		return -EFAULT;

	ret = hfi_dlid_assign(ctx, &dlid_assign);
	kfree(dlid_assign.dlid_entries_ptr);

	return ret;
#endif
	return 0;
}

int hfi2_dlid_release_handler(struct ib_device *ib_dev,
			      struct ib_uverbs_file *file,
			      struct uverbs_attr_bundle *attrs)
{
#ifdef CONFIG_HFI2_STLNP
	struct hfi_ctx *ctx;
	const struct uverbs_attr *uattr;

	uattr = uverbs_attr_get(attrs, HFI2_DLID_RELEASE_CTX_IDX);
	if (unlikely(IS_ERR(uattr)))
		return PTR_ERR(uattr);

	ctx = uattr->obj_attr.uobject->object;
	if (unlikely(!ctx)) {
		pr_err("%s: Encountered a NULL hfi context\n", __func__);
		return -ENOENT;
	}

	return hfi_dlid_release(ctx);
#endif
	return 0;
}

DECLARE_UVERBS_METHOD(hfi2_job_setup, HFI2_JOB_SETUP,
		      hfi2_job_setup_handler,
		      &UVERBS_ATTR_IDR(
				HFI2_JOB_SETUP_CTX_IDX,
				UVERBS_CREATE_NS_INDEX(HFI2_OBJECT_JOB,
						       HFI2_OBJECTS),
				UVERBS_ACCESS_NEW,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)),
		      &UVERBS_ATTR_PTR_IN(
				HFI2_JOB_SETUP_CMD,
				struct hfi_job_setup_args,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)),
		      &UVERBS_ATTR_PTR_OUT(
				HFI2_JOB_SETUP_PID_BASE,
				u16, UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)));

DECLARE_UVERBS_METHOD(hfi2_job_info, HFI2_JOB_INFO,
		      hfi2_job_info_handler,
		      &UVERBS_ATTR_PTR_OUT(
				HFI2_JOB_INFO_RESP,
				struct hfi_job_info,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)));

DECLARE_UVERBS_METHOD(hfi2_dlid_assign, HFI2_DLID_ASSIGN,
		      hfi2_dlid_assign_handler,
		      &UVERBS_ATTR_IDR(
				HFI2_DLID_ASSIGN_CTX_IDX,
				UVERBS_CREATE_NS_INDEX(HFI2_OBJECT_JOB,
						       HFI2_OBJECTS),
				UVERBS_ACCESS_WRITE,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)),
		      &UVERBS_ATTR_PTR_IN(
				HFI2_DLID_ASSIGN_CMD,
				struct hfi_dlid_assign_args,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)));

DECLARE_UVERBS_METHOD(hfi2_dlid_release, HFI2_DLID_RELEASE,
		      hfi2_dlid_release_handler,
		      &UVERBS_ATTR_IDR(
				HFI2_DLID_RELEASE_CTX_IDX,
				UVERBS_CREATE_NS_INDEX(HFI2_OBJECT_JOB,
						       HFI2_OBJECTS),
				UVERBS_ACCESS_WRITE,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)));

DECLARE_UVERBS_METHOD(hfi2_job_attach, HFI2_JOB_ATTACH,
		      hfi2_job_attach_handler,
		      &UVERBS_ATTR_PTR_IN(
				HFI2_JOB_ATTACH_MODE,
				u16, UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)),
		      &UVERBS_ATTR_PTR_IN(
				HFI2_JOB_ATTACH_COOKIE,
				u64, UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)));

DECLARE_UVERBS_OBJECT(hfi2_object_job,
		      UVERBS_CREATE_NS_INDEX(HFI2_OBJECT_JOB,
					     HFI2_OBJECTS),
		      &UVERBS_TYPE_ALLOC_IDR_SZ(sizeof(struct ib_ujob_object),
						0, hfi2_job_free),
		      &hfi2_job_setup,
		      &hfi2_job_info,
		      &hfi2_dlid_assign,
		      &hfi2_dlid_release,
		      &hfi2_job_attach);
