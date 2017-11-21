/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2017 Intel Corporation.
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
 * Copyright (c) 2017 Intel Corporation.
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
 * Intel(R) OPA Gen2 IB Driver
 */
#include "uverbs_obj.h"
#include "verbs.h"

int hfi2_ctx_attach(struct ib_device *ib_dev,
		    struct ib_uverbs_file *file,
		    struct uverbs_attr_array *attrs,
		    size_t num)
{
	struct hfi_ctx_attach_cmd cmd;
	struct hfi_ctx_attach_resp resp;
	struct uverbs_attr_array *common = &attrs[0];
	struct opa_ctx_assign ctx_assign = {0};
	struct hfi_devdata *dd = hfi_dd_from_ibdev(ib_dev);
	struct hfi_ctx *ctx;
	struct ib_uctx_object *obj;
	struct ib_ucontext *ucontext = file->ucontext;
	int ret = 0;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	hfi_ctx_init(ctx, dd);
	ctx->type = HFI_CTX_TYPE_USER;
	ctx->ptl_uid = current_uid().val;

#if 0
	INIT_LIST_HEAD(ctx->vma_head);
#endif
	hfi_job_init(ctx);

	ret = uverbs_copy_from(&cmd, common, HFI2_CTX_ATTACH_CMD);
	if (ret)
		goto err_attach;

	ctx_assign.pid = cmd.pid;
	ctx_assign.flags = cmd.flags;
	ctx_assign.le_me_count = cmd.le_me_count;
	ctx_assign.unexpected_count = cmd.unexpected_count;
	ctx_assign.trig_op_count = cmd.trig_op_count;

	ret = hfi_ctx_attach(ctx, &ctx_assign);
	if (ret)
		goto err_attach;

	resp.ct_token =
		HFI_MMAP_PSB_TOKEN(TOK_EVENTS_CT,
				   ctx->pid,
				   HFI_PSB_CT_SIZE);
	resp.eq_desc_token =
		HFI_MMAP_PSB_TOKEN(TOK_EVENTS_EQ_DESC,
				   ctx->pid,
				   HFI_PSB_EQ_DESC_SIZE);
	resp.eq_head_token =
		HFI_MMAP_PSB_TOKEN(TOK_EVENTS_EQ_HEAD,
				   ctx->pid,
				   HFI_PSB_EQ_HEAD_SIZE);
	resp.pt_token =
		HFI_MMAP_PSB_TOKEN(TOK_PORTALS_TABLE,
				   ctx->pid,
				   HFI_PSB_PT_SIZE);
	resp.le_me_token =
		HFI_MMAP_PSB_TOKEN(TOK_LE_ME,
				   ctx->pid,
				   ctx->le_me_size);
	resp.unexpected_token =
		HFI_MMAP_PSB_TOKEN(TOK_UNEXPECTED,
				   ctx->pid,
				   ctx->unexpected_size);
	resp.trig_op_token =
		HFI_MMAP_PSB_TOKEN(TOK_TRIG_OP,
				   ctx->pid,
				   ctx->trig_op_size);
	resp.pid = ctx->pid;
	resp.pid_base = ctx->pid_base;
	resp.pid_count = ctx->pid_count;
	resp.pid_mode = ctx->mode;
	resp.uid = ctx->ptl_uid;

	obj = container_of(common->attrs[HFI2_CTX_ATTACH_IDX].obj_attr.uobject,
			   typeof(*obj), uobject);
	obj->verbs_file = ucontext->ufile;
	obj->uobject.object = ctx;

	ctx->uobject = &obj->uobject;

	ret = uverbs_copy_to(common,
			     HFI2_CTX_PTL_UID,
			     &ctx->ptl_uid);
	ret += uverbs_copy_to(common,
			      HFI2_CTX_ATTACH_RESP,
			      &resp);
	if (ret)
		goto err_attach;

	return 0;

err_attach:
	kfree(ctx);
	return ret;
}

int hfi2_ctx_event_cmd_handler(struct ib_device *ib_dev,
			       struct ib_uverbs_file *file,
			       struct uverbs_attr_array *attrs,
			       size_t num)
{
	struct opa_ev_assign ev_assign = {0};
	struct hfi_event_args cmd;
	struct hfi_event_args resp;
	struct uverbs_attr_array *common = &attrs[0];
	struct hfi_ctx *ctx;
	u32 type;
	int ret;

	ret = uverbs_copy_from(&cmd, common, HFI2_CTX_EVT_CMD);
	ret += uverbs_copy_from(&type, common, HFI2_CTX_EVT_TYPE);
	if (ret) {
		pr_debug("%s: Error copying data, ret = %d\n", __func__, ret);
		return ret;
	}

	ctx = common->attrs[HFI2_CTX_ATTACH_IDX].obj_attr.uobject->object;

	switch (type) {
	case HFI2_CMD_EQ_ASSIGN:
		ev_assign.ni = cmd.idx0;
		ev_assign.base = cmd.data0;
		ev_assign.count = cmd.count;
		ev_assign.user_data = cmd.data1;

		if (!access_ok(VERIFY_WRITE, (__user void *)ev_assign.base,
			       ev_assign.count * HFI_EQ_ENTRY_SIZE))
			return -EFAULT;

		ret = hfi_cteq_assign(ctx, &ev_assign);
		if (ret)
			break;

		memcpy(&resp, &cmd, sizeof(struct hfi_event_args));
		resp.idx1 = ev_assign.ev_idx;

		ret = uverbs_copy_to(common, HFI2_CTX_EVT_RESP, &resp);
		break;
	case HFI2_CMD_EQ_RELEASE:
		ret = hfi_cteq_release(ctx, 0, cmd.idx1, cmd.data1);
		break;
	case HFI2_CMD_EC_WAIT_EQ:
		ret = hfi_ec_wait_event(ctx, cmd.idx0,
					(int)cmd.count,
					&cmd.data0,
					&cmd.data1);
		if (ret == -ERESTARTSYS) {
			ret = -EINTR;
			break;
		}

		memcpy(&resp, &cmd, sizeof(struct hfi_event_args));

		ret = uverbs_copy_to(common, HFI2_CTX_EVT_RESP, &resp);
		break;
	case HFI2_CMD_EC_SET_EQ:
		ret = hfi_ev_set_channel(ctx, cmd.idx0,
					 cmd.idx1,
					 cmd.data0,
					 cmd.data1);
		break;
	case HFI2_CMD_EQ_WAIT:
		ret = hfi_ev_wait_single(ctx, 1, cmd.idx1,
					 (int)cmd.count,
					 &cmd.data0,
					 &cmd.data1);
		if (ret == -ERESTARTSYS) {
			ret = -EINTR;
			break;
		}

		memcpy(&resp, &cmd, sizeof(struct hfi_event_args));

		ret = uverbs_copy_to(common, HFI2_CTX_EVT_RESP, &resp);
		break;
	case HFI2_CMD_EQ_ACK:
		ret = hfi_eq_ack_event(ctx, cmd.idx1, cmd.count);
		break;
	case HFI2_CMD_EC_ASSIGN:
		ret = hfi_ec_assign(ctx, &cmd.idx0);

		memcpy(&resp, &cmd, sizeof(struct hfi_event_args));

		ret = uverbs_copy_to(common, HFI2_CTX_EVT_RESP, &resp);
		break;
	case HFI2_CMD_EC_RELEASE:
		ret = hfi_ec_release(ctx, cmd.idx0);
		break;
	case HFI2_CMD_CT_ASSIGN:
		ev_assign.ni = cmd.idx0;
		ev_assign.mode = OPA_EV_MODE_COUNTER;
		ev_assign.base = cmd.data0;
		ev_assign.user_data = cmd.data1;

		ret = hfi_cteq_assign(ctx, &ev_assign);
		if (ret)
			break;

		memcpy(&resp, &cmd, sizeof(struct hfi_event_args));
		resp.idx1 = ev_assign.ev_idx;
		ret = uverbs_copy_to(common, HFI2_CTX_EVT_RESP, &resp);
		break;
	case HFI2_CMD_CT_RELEASE:
		ret = hfi_cteq_release(ctx, OPA_EV_MODE_COUNTER,
				       cmd.idx1, 0);
		break;
	case HFI2_CMD_EC_WAIT_CT:
		ret = hfi_ec_wait_event(ctx, cmd.idx0,
					(int)cmd.count,
					&cmd.data0,
					&cmd.data1);
		if (ret == -ERESTARTSYS) {
			ret = -EINTR;
			break;
		}

		memcpy(&resp, &cmd, sizeof(struct hfi_event_args));

		ret = uverbs_copy_to(common, HFI2_CTX_EVT_RESP, &resp);
		break;
	case HFI2_CMD_EC_SET_CT:
		ret = hfi_ev_set_channel(ctx, cmd.idx0,
					 cmd.idx1,
					 0, 0);
		break;
	case HFI2_CMD_CT_WAIT:
		ret = hfi_ev_wait_single(ctx, 0, cmd.idx1,
					 (int)cmd.count, NULL,
					 &cmd.data1);
		if (ret == -ERESTARTSYS) {
			ret = -EINTR;
			break;
		}
		memcpy(&resp, &cmd, sizeof(struct hfi_event_args));

		ret = uverbs_copy_to(common, HFI2_CTX_EVT_RESP, &resp);
		break;
	case HFI2_CMD_CT_ACK:
		ret = hfi_ct_ack_event(ctx, cmd.idx1, cmd.count);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

int hfi2_ctx_detach(struct ib_device *ib_dev,
		    struct ib_uverbs_file *file,
		    struct uverbs_attr_array *attrs,
		    size_t num)
{
	struct uverbs_attr_array *common = &attrs[0];
	struct ib_uctx_object *obj;
	struct ib_uobject *uobj;

	uobj = common->attrs[HFI2_CTX_DETACH_IDX].obj_attr.uobject;
	obj = container_of(uobj, struct ib_uctx_object, uobject);

#if 0
	if (!(ib_dev->uverbs_cmd_mask & 1ULL << IB_USER_VERBS_CMD_DESTROY_CTXT))
		return -EOPNOTSUPP;
#endif
	/*
	 * Since the caller has explicitly sent a command to detach the
	 * ctx, we need to clean up the underlying ib_uobject
	 * and free any resources that we do not need. This will result
	 * in a call to hfi2_ctx_free with the rdma_remove_reason
	 * RDMA_REMOVE_DESTROY
	 */
	return rdma_explicit_destroy(uobj);
}

int hfi2_ctx_free(struct ib_uobject *uobject,
		  enum rdma_remove_reason why)
{
	struct hfi_ctx *ctx = uobject->object;

	if (!ctx)
		return -EINVAL;

	hfi_ctx_cleanup(ctx);
	kfree(ctx);

	return 0;
}

int hfi2_pt_update_lower(struct ib_device *ib_dev,
			 struct ib_uverbs_file *file,
			 struct uverbs_attr_array *attrs,
			 size_t num)
{
	struct uverbs_attr_array *common = &attrs[0];
	struct hfi_ctx *ctx;
	struct hfi_pt_update_lower_args pt_update_args;
	int ret;

	ctx = common->attrs[HFI2_PT_UPDATE_CTX_IDX].obj_attr.uobject->object;

	ret = uverbs_copy_from(&pt_update_args, common,
			       HFI2_PT_UPDATE_LOWER_ARGS);
	if (ret)
		return ret;

	return hfi_pt_update_lower(ctx, pt_update_args.ni,
				   pt_update_args.pt_idx,
				   pt_update_args.val,
				   pt_update_args.user_data);
}

static DECLARE_UVERBS_ATTR_SPEC(
			hfi2_ctx_attach_spec,
			UVERBS_ATTR_IDR(
				 HFI2_CTX_ATTACH_IDX,
				 UVERBS_CREATE_TYPE_INDEX(HFI2_TYPE_CTX,
							  HFI2_OBJECT_TYPES),
				 UVERBS_ACCESS_NEW,
				 UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)),
			UVERBS_ATTR_PTR_IN(
				HFI2_CTX_ATTACH_CMD,
				struct hfi_ctx_attach_cmd,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)),
			UVERBS_ATTR_PTR_OUT(
				HFI2_CTX_ATTACH_RESP,
				struct hfi_ctx_attach_resp,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)),
			UVERBS_ATTR_PTR_OUT(
				HFI2_CTX_PTL_UID, u32,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)));

static DECLARE_UVERBS_ATTR_SPEC(
			hfi2_ctx_detach_spec,
			UVERBS_ATTR_IDR(
				HFI2_CTX_DETACH_IDX,
				UVERBS_CREATE_TYPE_INDEX(HFI2_TYPE_CTX,
							 HFI2_OBJECT_TYPES),
				UVERBS_ACCESS_DESTROY,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)));

static DECLARE_UVERBS_ATTR_SPEC(
			hfi2_ctx_event_cmd_spec,
			UVERBS_ATTR_IDR(
				HFI2_CTX_EVT_CTX_IDX,
				UVERBS_CREATE_TYPE_INDEX(HFI2_TYPE_CTX,
							 HFI2_OBJECT_TYPES),
				UVERBS_ACCESS_READ,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)),
			UVERBS_ATTR_PTR_IN(
				HFI2_CTX_EVT_CMD,
				struct hfi_event_args,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)),
			UVERBS_ATTR_PTR_OUT(
				HFI2_CTX_EVT_RESP,
				struct hfi_event_args,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)),
			UVERBS_ATTR_PTR_IN(
				HFI2_CTX_EVT_TYPE,
				u32, UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)));

static DECLARE_UVERBS_ATTR_SPEC(
			hfi2_pt_update_lower_spec,
			UVERBS_ATTR_IDR(
				HFI2_PT_UPDATE_CTX_IDX,
				UVERBS_CREATE_TYPE_INDEX(HFI2_TYPE_CTX,
							 HFI2_OBJECT_TYPES),
				UVERBS_ACCESS_WRITE,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)),
			UVERBS_ATTR_PTR_IN(
				HFI2_PT_UPDATE_LOWER_ARGS,
				struct hfi_pt_update_lower_args,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)));

/*
 * ib_uctx_object 'object' can contain info about uobject that can be used
 * after the underlying uobject is destroyed
 */
DECLARE_UVERBS_TYPE(
	hfi2_type_ctx,
	&UVERBS_TYPE_ALLOC_IDR_SZ(sizeof(struct ib_uctx_object),
				  0, hfi2_ctx_free),
	&UVERBS_ACTIONS(
		ADD_UVERBS_ACTION(HFI2_CTX_ATTACH,
				  hfi2_ctx_attach,
				  &hfi2_ctx_attach_spec),
		ADD_UVERBS_ACTION(HFI2_CTX_DETACH,
				  hfi2_ctx_detach,
				  &hfi2_ctx_detach_spec),
		ADD_UVERBS_ACTION(HFI2_CTX_EVENT_CMD,
				  hfi2_ctx_event_cmd_handler,
				  &hfi2_ctx_event_cmd_spec),
		ADD_UVERBS_ACTION(HFI2_PT_UPDATE_LOWER,
				  hfi2_pt_update_lower,
				  &hfi2_pt_update_lower_spec)));
