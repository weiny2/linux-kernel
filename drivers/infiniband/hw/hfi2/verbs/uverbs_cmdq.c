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
 * Intel(R) OPA Gen2 IB Driver
 */
#include "uverbs_obj.h"
#include "verbs.h"

int hfi2_cmdq_assign_handler(struct ib_device *ib_dev,
			     struct ib_uverbs_file *file,
			     struct uverbs_attr_bundle *attrs)
{
	struct ib_ucontext *ucontext = file->ucontext;
	struct ib_ucmdq_object  *obj;
	struct hfi2_cmdq_auth_table auth_table;
	struct hfi_cmdq_pair *cmdq = NULL;
	struct hfi_ctx *ctx;
	const struct uverbs_attr *uattr;
	u64 cmdq_head_token, cmdq_rx_token, cmdq_tx_token;
	ssize_t toff, tlen;
	int ret;

	ret = uverbs_copy_from(&auth_table, attrs,
			       HFI2_ASSIGN_CMDQ_AUTH_TABLE);
	if (ret)
		goto err_cmdq_assign;

	cmdq = kzalloc(sizeof(*cmdq), GFP_KERNEL);
	if (!cmdq) {
		ret = -ENOMEM;
		goto err_cmdq_mem;
	}

	uattr = uverbs_attr_get(attrs,HFI2_ASSIGN_CMDQ_CTX_IDX);
	if (unlikely(IS_ERR(uattr)))
		return PTR_ERR(uattr);

	ctx = uattr->obj_attr.uobject->object;
	if (unlikely(!ctx)) {
		pr_err("%s: Encountered a NULL hfi context\n", __func__);
		return -ENOENT;
	}

	ret = hfi_cmdq_assign(cmdq, ctx, &auth_table.auth_table[0]);
	if (ret)
		goto err_cmdq_assign;

	ret = hfi_ctx_hw_addr(ctx, TOK_CMDQ_HEAD, cmdq->idx,
			      (void *)&toff, &tlen);
	if (ret)
		goto err_ctx_hw_addr;

	cmdq_head_token = HFI_MMAP_TOKEN(TOK_CMDQ_HEAD, cmdq->idx,
					 toff, tlen);
	cmdq_tx_token = HFI_MMAP_TOKEN(TOK_CMDQ_TX, cmdq->idx, 0,
				       HFI_CMDQ_TX_SIZE);
	cmdq_rx_token = HFI_MMAP_TOKEN(TOK_CMDQ_RX, cmdq->idx, 0,
				       PAGE_ALIGN(HFI_CMDQ_RX_SIZE));

	uattr = uverbs_attr_get(attrs, HFI2_ASSIGN_CMDQ_IDX);
	if (unlikely(IS_ERR(uattr)))
		return PTR_ERR(uattr);

	obj = container_of(uattr->obj_attr.uobject, typeof(*obj), uobject);
	obj->verbs_file = ucontext->ufile;

	cmdq->device = ib_dev;
	cmdq->uobject = &obj->uobject;
	obj->uobject.object = cmdq;

	ret = uverbs_copy_to(attrs, HFI2_ASSIGN_CMDQ_TX_TOKEN,
			     &cmdq_tx_token, sizeof(cmdq_tx_token));
	ret += uverbs_copy_to(attrs, HFI2_ASSIGN_CMDQ_RX_TOKEN,
			      &cmdq_rx_token, sizeof(cmdq_rx_token));
	ret += uverbs_copy_to(attrs, HFI2_ASSIGN_CMDQ_HEAD_TOKEN,
			      &cmdq_head_token, sizeof(cmdq_head_token));
	if (ret)
		goto err_ctx_hw_addr;

	return 0;

err_ctx_hw_addr:
	hfi_cmdq_release(cmdq);
err_cmdq_assign:
	kfree(cmdq);
err_cmdq_mem:
	return ret;
}

int hfi2_cmdq_update_handler(struct ib_device *ib_dev,
			     struct ib_uverbs_file *file,
			     struct uverbs_attr_bundle *attrs)
{
	struct hfi2_cmdq_auth_table auth_table;
	const struct uverbs_attr *uattr;
	struct hfi_cmdq_pair *cmdq;
	int ret;

	uattr = uverbs_attr_get(attrs, HFI2_UPDATE_CMDQ_IDX);
	if (unlikely(IS_ERR(uattr)))
		return PTR_ERR(uattr);

	cmdq = uattr->obj_attr.uobject->object;
	if (unlikely(!cmdq)) {
		pr_err("%s: Encountered a NULL cmdq object\n", __func__);
		return -ENOENT;
	}

	ret = uverbs_copy_from(&auth_table, attrs,
			       HFI2_UPDATE_CMDQ_AUTH_TABLE);
	if (ret)
		return ret;

	return hfi_cmdq_update(cmdq, &auth_table.auth_table[0]);
}

int hfi2_cmdq_release_handler(struct ib_device *ib_dev,
			      struct ib_uverbs_file *file,
			      struct uverbs_attr_bundle *attrs)
{
	const struct uverbs_attr *uattr;
	struct ib_uobject *uobj;

	uattr = uverbs_attr_get(attrs, HFI2_RELEASE_CMDQ_IDX);
	if (unlikely(IS_ERR(uattr)))
		return PTR_ERR(uattr);

	uobj = uattr->obj_attr.uobject;

	/*
	 * Since the caller has explicitly sent a release cmdq command,
	 * we need to clean up the underlying ib_uobject and free the
	 * resources that we do not need. This will result in a call to
	 * hfi2_free_cmdq with the rdma remove reason
	 * RDMA_REMOVE_DESTROY
	 */
	return rdma_explicit_destroy(uobj);
}

int hfi2_cmdq_free(struct ib_uobject *uobject,
		   enum rdma_remove_reason why)
{
	struct hfi_cmdq_pair *cmdq = uobject->object;
	struct ib_ucontext *context = uobject->context;
	struct hfi_ibcontext *hfi_context = container_of(context,
							 struct hfi_ibcontext,
							 ibuc);
	int ret;

	if (!cmdq)
		return -EINVAL;

	if (!hfi_context)
		return -EINVAL;

	if (why == RDMA_REMOVE_DESTROY)
		hfi_zap_vma_list(hfi_context, cmdq->idx);

	ret = hfi_cmdq_release(cmdq);

	kfree(cmdq);

	return ret;
}

/*
 * TODO: Consider passing in a pointer to a variable sized array for
 * auth table and a length of the array instead of a struct with a
 * fixed size array
 */
DECLARE_UVERBS_METHOD(hfi2_cmdq_assign, HFI2_CMDQ_ASSIGN,
		      hfi2_cmdq_assign_handler,
		      &UVERBS_ATTR_IDR(
				HFI2_ASSIGN_CMDQ_IDX,
				UVERBS_CREATE_NS_INDEX(HFI2_OBJECT_CMDQ,
						       HFI2_OBJECTS),
				UVERBS_ACCESS_NEW,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)),
		      &UVERBS_ATTR_IDR(
				HFI2_ASSIGN_CMDQ_CTX_IDX,
				UVERBS_CREATE_NS_INDEX(HFI2_OBJECT_CTX,
						       HFI2_OBJECTS),
				UVERBS_ACCESS_READ,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)),
		      &UVERBS_ATTR_PTR_IN(
				HFI2_ASSIGN_CMDQ_AUTH_TABLE,
				struct hfi2_cmdq_auth_table,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)),
		      &UVERBS_ATTR_PTR_OUT(
				HFI2_ASSIGN_CMDQ_TX_TOKEN,
				u64, UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)),
		      &UVERBS_ATTR_PTR_OUT(
				HFI2_ASSIGN_CMDQ_RX_TOKEN,
				u64, UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)),
		      &UVERBS_ATTR_PTR_OUT(
				HFI2_ASSIGN_CMDQ_HEAD_TOKEN,
				u64, UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)));

DECLARE_UVERBS_METHOD(hfi2_cmdq_release, HFI2_CMDQ_RELEASE,
		      hfi2_cmdq_release_handler,
		      &UVERBS_ATTR_IDR(
				HFI2_RELEASE_CMDQ_IDX,
				UVERBS_CREATE_NS_INDEX(HFI2_OBJECT_CMDQ,
						       HFI2_OBJECTS),
				UVERBS_ACCESS_DESTROY,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)));

DECLARE_UVERBS_METHOD(hfi2_cmdq_update, HFI2_CMDQ_UPDATE,
		      hfi2_cmdq_update_handler,
		      &UVERBS_ATTR_IDR(
				HFI2_UPDATE_CMDQ_IDX,
				UVERBS_CREATE_NS_INDEX(HFI2_OBJECT_CMDQ,
						       HFI2_OBJECTS),
				UVERBS_ACCESS_WRITE,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)),
		      &UVERBS_ATTR_PTR_IN(
				HFI2_UPDATE_CMDQ_AUTH_TABLE,
				struct hfi2_cmdq_auth_table,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)));

DECLARE_UVERBS_OBJECT(hfi2_object_cmdq,
		      UVERBS_CREATE_NS_INDEX(HFI2_OBJECT_CMDQ,
					     HFI2_OBJECTS),
		      &UVERBS_TYPE_ALLOC_IDR_SZ(sizeof(struct ib_ucmdq_object),
						0, hfi2_cmdq_free),
		      &hfi2_cmdq_assign,
		      &hfi2_cmdq_release,
		      &hfi2_cmdq_update);
