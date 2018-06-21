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

#include <linux/device.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/pagemap.h>
#include <linux/vmalloc.h>
#include <linux/mutex.h>
#include <linux/version.h>
#include "device.h"
#include "opa_user.h"

/* TODO - turn off when RX unit is implemented in simulation */
static uint psb_rw = 1;
module_param(psb_rw, uint, 0444);
MODULE_PARM_DESC(psb_rw, "PSB mmaps allow RW");

/*
 * File operation functions
 */
static int hfi_open(struct inode *, struct file *);
static int hfi_close(struct inode *, struct file *);
static ssize_t hfi_write(struct file *, const char __user *, size_t, loff_t *);
static int hfi_mmap(struct file *, struct vm_area_struct *);
static u64 kvirt_to_phys(void *, int*);

static const struct file_operations hfi_file_ops = {
	.owner = THIS_MODULE,
	.write = hfi_write,
	.open = hfi_open,
	.release = hfi_close,
	.mmap = hfi_mmap,
	.llseek = noop_llseek,
};

/*
 * Zap each element in the vma_list and remove it.
 */
static void hfi_zap_vma_list(struct hfi_userdata *ud, uint16_t cmdq_idx)
{
	struct list_head *pos, *temp;
	struct hfi_vma *v;
	struct vm_area_struct *vv;

	mutex_lock(&ud->lock);
	list_for_each_safe(pos, temp, &ud->vma_head) {
		v = list_entry(pos, struct hfi_vma, vma_list);
		if (cmdq_idx == v->cmdq_idx) {
			vv = v->vma;
			zap_vma_ptes(vv, vv->vm_start,
				     vv->vm_end - vv->vm_start);
			list_del(pos);
			kfree(v);
		}
	}
	mutex_unlock(&ud->lock);
}

/*
 * Build the vma_list so that zap_vma_ptes may be called later on the list.
 */
static int hfi_vma_list_add(struct hfi_userdata *ud,
			    struct vm_area_struct *vma,
			    uint16_t cmdq_idx)
{
	struct hfi_vma *v;

	v = kmalloc(sizeof(*v), GFP_KERNEL);

	if (!v)
		return -ENOMEM;

	mutex_lock(&ud->lock);
	list_add(&v->vma_list, &ud->vma_head);
	v->vma = vma;
	v->cmdq_idx = cmdq_idx;
	vma->vm_private_data = ud;
	mutex_unlock(&ud->lock);
	return 0;
}

/*
 * Remove vma from the vma_list without calling zap_vma_ptes.
 */
static void hfi_vma_list_remove(struct hfi_userdata *ud,
				struct vm_area_struct *vma,
				uint16_t cmdq_idx)
{
	struct list_head *pos, *temp;
	struct hfi_vma *v;
	struct vm_area_struct *vv;

	mutex_lock(&ud->lock);
	list_for_each_safe(pos, temp, &ud->vma_head) {
		v = list_entry(pos, struct hfi_vma, vma_list);
		vv = v->vma;
		if (vv == vma && cmdq_idx == v->cmdq_idx) {
			list_del(pos);
			kfree(v);
			vma->vm_private_data = NULL;
			break;
		}
	}
	mutex_unlock(&ud->lock);
}

static inline int is_valid_mmap(u64 token)
{
	return (HFI_MMAP_TOKEN_GET(MAGIC, token) == HFI_MMAP_MAGIC);
}

static int hfi_open(struct inode *inode, struct file *fp)
{
	struct hfi_userdata *ud;
	struct hfi_info *hi = container_of(fp->private_data,
					   struct hfi_info, user_miscdev);
	struct ib_ucontext *ucontext;

	ud = kzalloc(sizeof(*ud), GFP_KERNEL);
	if (!ud)
		return -ENOMEM;
	fp->private_data = ud;

	/*
	 * TODO
	 * Review how/if we need to hold the device so it cannot be removed
	 * until user calls close(), see ib_uverbs_open and review STL-3033
	 * about earlier race here.
	 */
	/* Pass in ud so hfi2 understands this is a user-space context */
	ucontext = hi->ibdev->alloc_ucontext(hi->ibdev, (void *)ud);
	if (IS_ERR(ucontext)) {
		kfree(ud);
		return PTR_ERR(ucontext);
	}
	/* store IB context handle which contains ops table */
	ud->uc = (struct hfi_ibcontext *)ucontext;

	mutex_init(&ud->lock);
	/* Setup list to zap vmas on release */
	INIT_LIST_HEAD(&ud->vma_head);

	ud->pid = task_pid_nr(current);
	ud->sid = task_session_vnr(current);

	/* initialize hfi_ctx */
	ud->uc->ops->ctx_init(hi->ibdev, &ud->ctx);
	ud->ctx.type = HFI_CTX_TYPE_USER;

	return 0;
}

static int hfi_e2e_conn(struct hfi_ibcontext *uc,
			struct hfi_e2e_conn_args __user *e2e_args)
{
	struct opa_core_ops *ops = uc->ops;
	struct hfi_e2e_conn e2e_conn;
	struct hfi_e2e_conn_args e2e;
	int i, num_conn = 0;

	/* Copy the details for this array of connection requests */
	if (copy_from_user(&e2e, e2e_args, sizeof(*e2e_args)))
		return -EFAULT;
	for (i = 0; i < e2e.num_req; i++) {
		/* Copy the details for this connection */
		if (copy_from_user(&e2e_conn, &e2e.e2e_req[i],
				   sizeof(e2e_conn)))
			return -EFAULT;
		/* Initiate the E2E connection */
		e2e_conn.conn_status = ops->e2e_ctrl(uc, &e2e_conn);
		/* Inform user space about the connection status */
		if (copy_to_user(&e2e.e2e_req[i].conn_status,
				 &e2e_conn.conn_status,
				 sizeof(e2e_conn.conn_status)))
			return -EFAULT;
		if (!e2e_conn.conn_status)
			num_conn++;
	}
	/* Inform user space about the number of connections established */
	if (copy_to_user(&e2e_args->num_conn, &num_conn, sizeof(num_conn)))
		return -EFAULT;
	return 0;
}

static ssize_t hfi_write(struct file *fp, const char __user *data, size_t count,
			 loff_t *offset)
{
	struct hfi_userdata *ud;
	const struct hfi_cmd __user *ucmd;
	struct hfi_cmd cmd;
	struct hfi_cmdq_pair cmdq_pair;
	struct hfi_pt_update_lower_args pt_update_lower;
	struct hfi_cmdq_assign_args cmdq_assign;
	struct hfi_cmdq_update_args cmdq_update;
	struct hfi_cmdq_release_args cmdq_release;
	struct hfi_event_args event_args;
	struct hfi_ctxt_attach_args ctxt_attach;
	struct hfi_sl_pair slp;
	struct hfi_hw_limit hwl;
	struct opa_ctx_assign ctx_assign = { 0 };
	struct opa_ev_assign ev_assign = { 0 };
	struct hfi_ts_master_regs master_ts;
	struct hfi_ts_fm_data fm_data;
	struct hfi_async_error ae;
	struct hfi_async_error_args ae_arg;
	struct hfi_at_prefetch_args atpf_arg;
	int need_admin = 0;
	ssize_t consumed = 0, copy_in = 0, copy_out = 0, ret = 0;
	void *copy_ptr = NULL;
	void __user *user_data = NULL;
	struct opa_core_ops *ops;
	ssize_t toff, tlen;

	if (count < sizeof(cmd)) {
		ret = -EINVAL;
		goto err_cmd;
	}
	ud = fp->private_data;
	if (!ud || !ud->uc) {
		ret = -EBADF;
		goto err_cmd;
	}
	ops = ud->uc->ops;

	ucmd = (const struct hfi_cmd __user *)data;
	if (copy_from_user(&cmd, ucmd, sizeof(cmd))) {
		ret = -EFAULT;
		goto err_cmd;
	}

	consumed = sizeof(cmd);
	user_data = (void __user *)cmd.context;

	switch (cmd.type) {
	case HFI_CMD_CMDQ_ASSIGN:
		copy_in = sizeof(cmdq_assign);
		copy_out = copy_in;
		copy_ptr = &cmdq_assign;
		break;
	case HFI_CMD_CMDQ_UPDATE:
		copy_in = sizeof(cmdq_update);
		copy_out = copy_in;
		copy_ptr = &cmdq_update;
		break;
	case HFI_CMD_CMDQ_RELEASE:
		copy_in = sizeof(cmdq_release);
		copy_ptr = &cmdq_release;
		break;
	case HFI_CMD_CTXT_ATTACH:
		copy_in = sizeof(ctxt_attach);
		copy_out = copy_in;
		copy_ptr = &ctxt_attach;
		break;
	case HFI_CMD_CTXT_DETACH:
		break;

	case HFI_CMD_CT_ASSIGN:
	case HFI_CMD_EQ_ASSIGN:
	case HFI_CMD_EC_ASSIGN:
	case HFI_CMD_EC_WAIT_CT:
	case HFI_CMD_EC_WAIT_EQ:
	case HFI_CMD_CT_WAIT:
	case HFI_CMD_EQ_WAIT:
		copy_out = sizeof(event_args);
		/* fallthrough for copy_in */
	case HFI_CMD_CT_RELEASE:
	case HFI_CMD_EQ_RELEASE:
	case HFI_CMD_EC_RELEASE:
	case HFI_CMD_EC_SET_CT:
	case HFI_CMD_CT_ACK:
	case HFI_CMD_EC_SET_EQ:
	case HFI_CMD_EQ_ACK:
		copy_in = sizeof(event_args);
		copy_ptr = &event_args;
		break;
	case HFI_CMD_PT_UPDATE_LOWER:
		copy_in = sizeof(pt_update_lower);
		copy_ptr = &pt_update_lower;
		break;
	case HFI_CMD_E2E_CONN:
		break;
	case HFI_CMD_CHECK_SL_PAIR:
		copy_in = sizeof(slp);
		copy_ptr = &slp;
		break;
	case HFI_CMD_GET_HW_LIMITS:
		copy_out = sizeof(hwl);
		copy_ptr = &hwl;
		break;
	case HFI_CMD_TS_GET_MASTER:
		copy_in = sizeof(master_ts);
		copy_out = copy_in;
		copy_ptr = &master_ts;
		break;
	case HFI_CMD_TS_GET_FM_DATA:
		copy_in = sizeof(fm_data);
		copy_out = copy_in;
		copy_ptr = &fm_data;
		break;
	case HFI_CMD_GET_ASYNC_ERROR:
		copy_in = sizeof(ae_arg);
		copy_ptr = &ae_arg;
		break;
	case HFI_CMD_AT_PREFETCH:
		copy_in = sizeof(atpf_arg);
		copy_ptr = &atpf_arg;
		break;
	default:
		ret = -EINVAL;
		goto err_cmd;
	}

	if (need_admin && !capable(CAP_SYS_ADMIN)) {
		ret = -EACCES;
		goto err_cmd;
	}

	/* if need copy_to_user, verify we have WRITE access upfront */
	if (copy_out) {
		if (!access_ok(VERIFY_WRITE, user_data, copy_out)) {
			ret = -EFAULT;
			goto err_cmd;
		}
	}

	/* If the command comes with user data, copy it. */
	if (copy_in) {
		if (copy_in != cmd.length) {
			ret = -EINVAL;
			goto err_cmd;
		}
		if (copy_from_user(copy_ptr, user_data, copy_in)) {
			ret = -EFAULT;
			goto err_cmd;
		}
		consumed += copy_in;
	}

	switch (cmd.type) {
	case HFI_CMD_CMDQ_ASSIGN:
		ret = ops->cmdq_assign(&cmdq_pair, &ud->ctx,
				       cmdq_assign.auth_table);
		if (ret)
			break;

		/* return mmap tokens of CMDQ items */
		ret = ops->ctx_addr(&ud->ctx, TOK_CMDQ_HEAD, cmdq_pair.idx,
				    (void *)&toff, &tlen);
		if (ret) {
			ops->cmdq_release(&cmdq_pair);
			break;
		}
		cmdq_assign.cmdq_head_token =
			HFI_MMAP_TOKEN(TOK_CMDQ_HEAD, cmdq_pair.idx, toff,
				       tlen);
		cmdq_assign.cmdq_tx_token =
			HFI_MMAP_TOKEN(TOK_CMDQ_TX, cmdq_pair.idx, 0,
				       HFI_CMDQ_TX_SIZE);
		cmdq_assign.cmdq_rx_token =
			HFI_MMAP_TOKEN(TOK_CMDQ_RX, cmdq_pair.idx, 0,
				       PAGE_ALIGN(HFI_CMDQ_RX_SIZE));
		cmdq_assign.cmdq_idx = cmdq_pair.idx;
		break;
	case HFI_CMD_CMDQ_UPDATE:
		cmdq_pair.ctx = &ud->ctx;
		cmdq_pair.idx = cmdq_update.cmdq_idx;
		ret = ops->cmdq_update(&cmdq_pair, cmdq_update.auth_table);
		break;
	case HFI_CMD_CMDQ_RELEASE:
		hfi_zap_vma_list(ud, cmdq_release.cmdq_idx);
		cmdq_pair.ctx = &ud->ctx;
		cmdq_pair.idx = cmdq_release.cmdq_idx;
		ret = ops->cmdq_release(&cmdq_pair);
		break;

	case HFI_CMD_CT_ASSIGN:
		ev_assign.ni = event_args.idx0;
		ev_assign.mode = OPA_EV_MODE_COUNTER;
		ev_assign.base = event_args.data0;
		ev_assign.user_data = event_args.data1;
		ret = ops->ev_assign(&ud->ctx, &ev_assign);
		event_args.idx1 = ev_assign.ev_idx;
		break;
	case HFI_CMD_CT_RELEASE:
		ret = ops->ev_release(&ud->ctx, OPA_EV_MODE_COUNTER,
				      event_args.idx1, 0);
		break;
	case HFI_CMD_CT_WAIT:
		ret = ops->ev_wait_single(&ud->ctx, 0, event_args.idx1,
					  (int)event_args.count,
					  NULL,
					  &event_args.data1);
		/* TODO - support restart? */
		if (ret == -ERESTARTSYS)
			ret = -EINTR;
		break;
	case HFI_CMD_EC_WAIT_CT:
		ret = ops->ec_wait_event(&ud->ctx, event_args.idx0,
					 (int)event_args.count,
					 &event_args.data0,
					 &event_args.data1);
		/* TODO - support restart? */
		if (ret == -ERESTARTSYS)
			ret = -EINTR;
		break;
	case HFI_CMD_EC_SET_CT:
		ret = ops->ev_set_channel(&ud->ctx, event_args.idx0,
					  event_args.idx1,
					  0, 0);
		break;
	case HFI_CMD_CT_ACK:
		ret = ops->ct_ack_event(&ud->ctx, event_args.idx1,
					event_args.count);
		break;
	case HFI_CMD_EQ_ASSIGN:
		ev_assign.ni = event_args.idx0;
		ev_assign.mode = 0;
		ev_assign.jumbo = false;
		ev_assign.base = event_args.data0;
		ev_assign.count = event_args.count;
		ev_assign.user_data = event_args.data1;
		ev_assign.isr_cb = NULL;
		if (!access_ok(VERIFY_WRITE, (__user void *)ev_assign.base,
			       ev_assign.count * HFI_EQ_ENTRY_SIZE)) {
			ret = -EFAULT;
			break;
		}
		ret = ops->ev_assign(&ud->ctx, &ev_assign);
		event_args.idx1 = ev_assign.ev_idx;
		break;
	case HFI_CMD_EQ_RELEASE:
		ret = ops->ev_release(&ud->ctx, 0, event_args.idx1,
				      event_args.data1);
		break;
	case HFI_CMD_EQ_WAIT:
		ret = ops->ev_wait_single(&ud->ctx, 1, event_args.idx1,
					  (int)event_args.count,
					  &event_args.data0,
					  &event_args.data1);
		/* TODO - support restart? */
		if (ret == -ERESTARTSYS)
			ret = -EINTR;
		break;
	case HFI_CMD_EC_WAIT_EQ:
		ret = ops->ec_wait_event(&ud->ctx, event_args.idx0,
					 (int)event_args.count,
					 &event_args.data0,
					 &event_args.data1);
		/* TODO - support restart? */
		if (ret == -ERESTARTSYS)
			ret = -EINTR;
		break;
	case HFI_CMD_EC_SET_EQ:
		ret = ops->ev_set_channel(&ud->ctx, event_args.idx0,
					  event_args.idx1,
					  event_args.data0,
					  event_args.data1);
		break;
	case HFI_CMD_EQ_ACK:
		ret = ops->eq_ack_event(&ud->ctx, event_args.idx1,
					event_args.count);
		break;
	case HFI_CMD_EC_ASSIGN:
		ret = ops->ec_assign(&ud->ctx, &event_args.idx0);
		break;
	case HFI_CMD_EC_RELEASE:
		ret = ops->ec_release(&ud->ctx, event_args.idx0);
		break;

	case HFI_CMD_PT_UPDATE_LOWER:
		ret = ops->pt_update_lower(&ud->ctx, pt_update_lower.ni,
					   pt_update_lower.pt_idx,
					   pt_update_lower.val,
					   pt_update_lower.user_data);
		break;
	case HFI_CMD_CTXT_ATTACH:
		ctx_assign.pid = ctxt_attach.pid;
		ctx_assign.flags = ctxt_attach.flags;
		ctx_assign.le_me_count = ctxt_attach.le_me_count;
		ctx_assign.unexpected_count = ctxt_attach.unexpected_count;
		ctx_assign.trig_op_count = ctxt_attach.trig_op_count;
		ret = ops->ctx_assign(&ud->ctx, &ctx_assign);
		if (ret)
			break;

		/* return mmap tokens of PSB items */
		ctxt_attach.ct_token =
			HFI_MMAP_PSB_TOKEN(TOK_EVENTS_CT,
					   ud->ctx.pid, HFI_PSB_CT_SIZE);
		ctxt_attach.eq_desc_token =
			HFI_MMAP_PSB_TOKEN(TOK_EVENTS_EQ_DESC,
					   ud->ctx.pid, HFI_PSB_EQ_DESC_SIZE);
		ctxt_attach.eq_head_token =
			HFI_MMAP_PSB_TOKEN(TOK_EVENTS_EQ_HEAD,
					   ud->ctx.pid, HFI_PSB_EQ_HEAD_SIZE);
		ctxt_attach.pt_token =
			HFI_MMAP_PSB_TOKEN(TOK_PORTALS_TABLE,
					   ud->ctx.pid, HFI_PSB_PT_SIZE);
		ctxt_attach.le_me_token =
			HFI_MMAP_PSB_TOKEN(TOK_LE_ME,
					   ud->ctx.pid, ud->ctx.le_me_size);
		ctxt_attach.unexpected_token =
			HFI_MMAP_PSB_TOKEN(TOK_UNEXPECTED,
					   ud->ctx.pid,
					   ud->ctx.unexpected_size);
		ctxt_attach.trig_op_token =
			HFI_MMAP_PSB_TOKEN(TOK_TRIG_OP,
					   ud->ctx.pid, ud->ctx.trig_op_size);
		ctxt_attach.pid = ud->ctx.pid;
		ctxt_attach.pid_base = ud->ctx.res.pid_base;
		ctxt_attach.pid_count = ud->ctx.res.pid_count;
		ctxt_attach.pid_mode = ud->ctx.mode;
		ctxt_attach.uid = ud->ctx.ptl_uid;
		break;
	case HFI_CMD_CTXT_DETACH:
		/* release our assigned PID */
		ops->ctx_release(&ud->ctx);
		break;
	case HFI_CMD_E2E_CONN:
		ret = hfi_e2e_conn(ud->uc, user_data);
		break;
	case HFI_CMD_CHECK_SL_PAIR:
		ret = ops->check_sl_pair(ud->uc, &slp);
		break;
	case HFI_CMD_GET_HW_LIMITS:
		ret = ops->get_hw_limits(ud->uc, &hwl);
		break;
	case HFI_CMD_TS_GET_MASTER:
		ret = ops->get_ts_master_regs(ud->uc, &master_ts);
		break;
	case HFI_CMD_TS_GET_FM_DATA:
		ret = ops->get_ts_fm_data(ud->uc, &fm_data);
		break;
	case HFI_CMD_GET_ASYNC_ERROR:
		user_data = (void __user *)ae_arg.ae;
		copy_ptr = &ae;
		copy_out = sizeof(ae);
		ret = ops->get_async_error(&ud->ctx, copy_ptr, ae_arg.timeout);
		break;
	case HFI_CMD_AT_PREFETCH:
		ret = ops->at_prefetch(&ud->ctx, &atpf_arg);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	if (ret == 0) {
		ret = consumed;
		if (copy_out && copy_to_user(user_data, copy_ptr, copy_out)) {
			/* tested WRITE access above, so this shouldn't fail */
			ret = -EFAULT;
		}
	}

err_cmd:
	return ret;
}

static int hfi_mmap(struct file *fp, struct vm_area_struct *vma)
{
	struct hfi_userdata *ud;
	unsigned long flags, pfn;
	void *remap_addr, *kvaddr = NULL;
	u64 token = vma->vm_pgoff << PAGE_SHIFT;
	u64 phys_addr = 0;
	int high = 0, mapio = 0, vm_ro = 0, type;
	ssize_t memlen = 0;
	int ret = 0;
	u16 ctxt;
	struct opa_core_ops *ops;
	bool add_to_vma_list = false;

	ud = fp->private_data;
	if (!ud || !ud->uc) {
		ret = -EBADF;
		goto done;
	}
	ops = ud->uc->ops;

	if (!is_valid_mmap(token) ||
	    !(vma->vm_flags & VM_SHARED)) {
		ret = -EINVAL;
		goto done;
	}

	/* validate we have an assigned Portals PID */
	if (ud->ctx.pid == HFI_PID_NONE) {
		ret = -EINVAL;
		goto done;
	}

	ctxt = HFI_MMAP_TOKEN_GET(CTXT, token);
	type = HFI_MMAP_TOKEN_GET(TYPE, token);
	flags = vma->vm_flags;

	/* these mmaps don't need to litter the MM of forked children */
	flags |= VM_DONTCOPY;

	/* handle errors before we attempt the mapping */
	switch (type) {
	case TOK_CMDQ_TX:
	case TOK_CMDQ_RX:
		mapio = 1;
	case TOK_CMDQ_HEAD:
		/* Save vma for later zapping */
		add_to_vma_list = true;
		ret = hfi_vma_list_add(ud, vma, ctxt);
		if (ret)
			return ret;

		if (ctxt >= HFI_CMDQ_COUNT) {
			ret = -EINVAL;
			goto done;
		}
		if (type == TOK_CMDQ_HEAD)
			vm_ro = 1;
		break;
	case TOK_EVENTS_CT:
	case TOK_EVENTS_EQ_DESC:
	case TOK_PORTALS_TABLE:
	case TOK_TRIG_OP:
	case TOK_LE_ME:
	case TOK_UNEXPECTED:
		if (!psb_rw)
			vm_ro = 1;
		break;
	case TOK_EVENTS_EQ_HEAD:
	default:
		break;
	}

	if (vm_ro) {
		/* enforce no write access to RO mappings */
		if (flags & VM_WRITE) {
			ret = -EPERM;
			goto done;
		}
		flags &= ~VM_MAYWRITE;
	}

	/*
	 * Get address and length from HW driver.
	 * For CMDQ tokens, ownership tested here.
	 */
	ret = ops->ctx_addr(&ud->ctx, type, ctxt, &remap_addr, &memlen);
	if (ret)
		goto done;

	if ((vma->vm_end - vma->vm_start) != memlen) {
		ret = -EINVAL;
		goto done;
	}

	if (mapio) {
		phys_addr = (u64)remap_addr;
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	} else {
		/* align remap_addr as may be offset into PAGE */
		kvaddr = (void *)((u64)remap_addr & PAGE_MASK);
		phys_addr = kvirt_to_phys(kvaddr, &high);
	}

	vma->vm_flags = flags;

	pr_debug("%s: %u type:%u io/h:%d/%d, addr:0x%llx, len:%lu(%lu), flags:0x%lx\n",
		 __func__, ctxt, type, mapio, high,
		 phys_addr, memlen, vma->vm_end - vma->vm_start,
		 vma->vm_flags);

	pfn = (unsigned long)(phys_addr >> PAGE_SHIFT);
	if (mapio) {
		ret = io_remap_pfn_range(vma, vma->vm_start, pfn, memlen,
					 vma->vm_page_prot);
	} else if (high) {
		ret = remap_vmalloc_range_partial(vma, vma->vm_start,
						  kvaddr, memlen);
	} else {
		ret = remap_pfn_range(vma, vma->vm_start, pfn, memlen,
				      vma->vm_page_prot);
	}

	/* Something failed above -- remove vma from zap list */
	if (ret && add_to_vma_list)
		hfi_vma_list_remove(ud, vma, ctxt);
 done:
	return ret;
}

static int hfi_user_cleanup(struct hfi_userdata *ud)
{
	struct opa_core_ops *ops = ud->uc->ops;
	int rc;

	/* release Portals resources acquired by the HFI user */
	rc = ops->ctx_release(&ud->ctx);
	if (rc)
		pr_warn("%s: Could not release ctx\n", __func__);

	return 0;
}

static int hfi_close(struct inode *inode, struct file *fp)
{
	struct hfi_userdata *ud = fp->private_data;
	struct ib_ucontext *ibuc = &ud->uc->ibuc;

	fp->private_data = NULL;
	hfi_user_cleanup(ud);
	ibuc->device->dealloc_ucontext(ibuc);
	kfree(ud);
	return 0;
}

/*
 * Convert kernel *virtual* addresses to physical addresses.
 * This handles vmalloc'ed or kmalloc'ed addresses.
 */
static u64 kvirt_to_phys(void *addr, int *high)
{
	struct page *page;
	u64 paddr = 0;

	if (!is_vmalloc_addr(addr)) {
		*high = 0;
		return virt_to_phys(addr);
	}

	*high = 1;
	page = vmalloc_to_page(addr);
	if (page)
		paddr = page_to_pfn(page) << PAGE_SHIFT;

	return paddr;
}

int hfi_user_add(struct hfi_info *hi)
{
	int rc;
	struct miscdevice *mdev;
	struct ib_device *ibdev = hi->ibdev;

	mdev = &hi->user_miscdev;
	mdev->minor = MISC_DYNAMIC_MINOR;
	mdev->name = ibdev->name;
	mdev->fops = &hfi_file_ops;
	mdev->parent = NULL;

	rc = misc_register(mdev);
	if (rc)
		dev_err(&ibdev->dev, "%s failed rc %d\n", __func__, rc);
	return rc;
}

void hfi_user_remove(struct hfi_info *hi)
{
	misc_deregister(&hi->user_miscdev);
}
