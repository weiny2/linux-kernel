/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2013 - 2015 Intel Corporation.
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
 * Copyright (c) 2013 - 2015 Intel Corporation.
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

#include <linux/device.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/pagemap.h>
#include <linux/sched.h>
#include <linux/version.h>
#include "device.h"
#include "opa_user.h"

/* TODO - turn off when RX unit is implemented in simulation */
static uint psb_rw = 1;
module_param(psb_rw, uint, S_IRUGO);
MODULE_PARM_DESC(psb_rw, "PSB mmaps allow RW");

/*
 * File operation functions
 */
static int hfi_open(struct inode *, struct file *);
static int hfi_close(struct inode *, struct file *);
static ssize_t hfi_write(struct file *, const char __user *, size_t, loff_t *);
static ssize_t hfi_aio_write(struct kiocb *, const struct iovec *,
			     unsigned long, loff_t);
static int hfi_mmap(struct file *, struct vm_area_struct *);
static u64 kvirt_to_phys(void *, int*);
static int vma_fault(struct vm_area_struct *, struct vm_fault *);

static const struct file_operations hfi_file_ops = {
	.owner = THIS_MODULE,
	.write = hfi_write,
	.aio_write = hfi_aio_write,
	.open = hfi_open,
	.release = hfi_close,
	.mmap = hfi_mmap,
	.llseek = noop_llseek,
};

static struct vm_operations_struct vm_ops = {
	.fault = vma_fault,
};

static inline int is_valid_mmap(u64 token)
{
	return (HFI_MMAP_TOKEN_GET(MAGIC, token) == HFI_MMAP_MAGIC);
}

static int hfi_open(struct inode *inode, struct file *fp)
{
	struct hfi_userdata *ud;
	struct hfi_info *hi = container_of(fp->private_data,
					   struct hfi_info, user_miscdev);

	ud = kzalloc(sizeof(struct hfi_userdata), GFP_KERNEL);
	if (!ud)
		return -ENOMEM;
	fp->private_data = ud;

	/* store HFI HW device pointers */
	ud->bus_ops = hi->odev->bus_ops;
	ud->ctx.devdata = hi->odev->dd;

	ud->pid = task_pid_nr(current);
	ud->sid = task_session_vnr(current);

	/* default Portals PID and UID */
	ud->ctx.ptl_pid = HFI_PID_NONE;
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0)
	ud->ctx.ptl_uid = current_uid();
#else
	ud->ctx.ptl_uid = current_uid().val;
#endif

	INIT_LIST_HEAD(&ud->mpin_head);
	spin_lock_init(&ud->mpin_lock);

	/* inherit PID reservation if present */
	hfi_job_init(ud);

	return 0;
}

static ssize_t hfi_write(struct file *fp, const char __user *data, size_t count,
			 loff_t *offset)
{
	struct hfi_userdata *ud;
	const struct hfi_cmd __user *ucmd;
	struct hfi_cmd cmd;
	struct hfi_cq_assign_args cq_assign;
	struct hfi_cq_update_args cq_update;
	struct hfi_cq_release_args cq_release;
	struct hfi_eq_assign_args eq_assign;
	struct hfi_eq_release_args eq_release;
	struct hfi_dlid_assign_args dlid_assign;
	struct hfi_ctxt_attach_args ctxt_attach;
	struct hfi_job_info_args job_info;
	struct hfi_job_setup_args job_setup;
	struct hfi_mpin_args mpin;
	struct hfi_munpin_args munpin;
	struct opa_ctx_assign ctx_assign;
	int need_admin = 0;
	ssize_t consumed = 0, copy_in = 0, copy_out = 0, ret = 0;
	void *copy_ptr = NULL;
	void __user *user_data = NULL;
	struct opa_core_ops *ops;
	ssize_t toff, tlen;
	u16 tmp_cq_idx;

	if (count < sizeof(cmd)) {
		ret = -EINVAL;
		goto err_cmd;
	}
	ud = fp->private_data;
	BUG_ON(!ud || !ud->bus_ops);
	ops = ud->bus_ops;

	ucmd = (const struct hfi_cmd __user *)data;
	if (copy_from_user(&cmd, ucmd, sizeof(cmd))) {
		ret = -EFAULT;
		goto err_cmd;
	}

	consumed = sizeof(cmd);
	user_data = (void __user *)cmd.context;

	switch (cmd.type) {
	case HFI_CMD_CQ_ASSIGN:
		copy_in = sizeof(cq_assign);
		copy_out = copy_in;
		copy_ptr = &cq_assign;
		break;
	case HFI_CMD_CQ_UPDATE:
		copy_in = sizeof(cq_update);
		copy_ptr = &cq_update;
		break;
	case HFI_CMD_CQ_RELEASE:
		copy_in = sizeof(cq_release);
		copy_ptr = &cq_release;
		break;
	case HFI_CMD_CTXT_ATTACH:
		copy_in = sizeof(ctxt_attach);
		copy_out = copy_in;
		copy_ptr = &ctxt_attach;
		break;
	case HFI_CMD_CTXT_DETACH:
		copy_in = 0;
		break;
	case HFI_CMD_DLID_ASSIGN:
		copy_in = sizeof(dlid_assign);
		copy_ptr = &dlid_assign;
		need_admin = 1;
		break;
	case HFI_CMD_DLID_RELEASE:
		copy_in = 0;
		need_admin = 1;
		break;
	case HFI_CMD_EQ_ASSIGN:
		copy_in = sizeof(eq_assign);
		copy_out = copy_in;
		copy_ptr = &eq_assign;
		break;
	case HFI_CMD_EQ_RELEASE:
		copy_in = sizeof(eq_release);
		copy_ptr = &eq_release;
		break;
	case HFI_CMD_JOB_INFO:
		copy_out = sizeof(job_info);
		copy_ptr = &job_info;
		break;
	case HFI_CMD_JOB_SETUP:
		copy_in = sizeof(job_setup);
		copy_ptr = &job_setup;
		need_admin = 1;
		break;
	case HFI_CMD_MPIN:
		copy_in = sizeof(mpin);
		copy_out = copy_in;
		copy_ptr = &mpin;
		break;
	case HFI_CMD_MUNPIN:
		copy_in = sizeof(munpin);
		copy_ptr = &munpin;
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
		if (copy_from_user(copy_ptr, user_data, copy_in)) {
			ret = -EFAULT;
			goto err_cmd;
		}
		consumed += copy_in;
	}

	switch (cmd.type) {
	case HFI_CMD_CQ_ASSIGN:
		ret = ops->cq_assign(&ud->ctx, cq_assign.auth_table, &tmp_cq_idx);
		if (ret)
			break;

		/* return mmap tokens of CQ items */
		ret = ops->ctx_addr(&ud->ctx, TOK_CQ_HEAD, tmp_cq_idx, (void *)&toff, &tlen);
		if (ret) {
			ops->cq_release(&ud->ctx, tmp_cq_idx);
			break;
		}
		cq_assign.cq_head_token = HFI_MMAP_TOKEN(TOK_CQ_HEAD, tmp_cq_idx, toff, tlen);
		cq_assign.cq_tx_token = HFI_MMAP_TOKEN(TOK_CQ_TX, tmp_cq_idx,
						       0, HFI_CQ_TX_SIZE);
		cq_assign.cq_rx_token = HFI_MMAP_TOKEN(TOK_CQ_RX, tmp_cq_idx,
						       0, PAGE_ALIGN(HFI_CQ_RX_SIZE));
		cq_assign.cq_idx = tmp_cq_idx;
		break;
	case HFI_CMD_CQ_UPDATE:
		ret = ops->cq_update(&ud->ctx, cq_update.cq_idx, cq_update.auth_table);
		break;
	case HFI_CMD_CQ_RELEASE:
		ret = ops->cq_release(&ud->ctx, cq_release.cq_idx);
		break;
	case HFI_CMD_EQ_ASSIGN:
		ret = ops->eq_assign(&ud->ctx, &eq_assign);
		break;
	case HFI_CMD_EQ_RELEASE:
		ret = ops->eq_release(&ud->ctx, eq_release.eq_idx);
		break;
	case HFI_CMD_DLID_ASSIGN:
		/* must be called after JOB_SETUP and match total LIDs */
		if (dlid_assign.count != ud->ctx.lid_count) {
			ret = -EINVAL;
			break;
		}
		ret = ops->dlid_assign(&ud->ctx, &dlid_assign);
		break;
	case HFI_CMD_DLID_RELEASE:
		ret = ops->dlid_release(&ud->ctx);
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
		ctxt_attach.ct_token = HFI_MMAP_PSB_TOKEN(TOK_EVENTS_CT,
						ud->ctx.ptl_pid, HFI_PSB_CT_SIZE);
		ctxt_attach.eq_desc_token = HFI_MMAP_PSB_TOKEN(TOK_EVENTS_EQ_DESC,
						ud->ctx.ptl_pid, HFI_PSB_EQ_DESC_SIZE);
		ctxt_attach.eq_head_token = HFI_MMAP_PSB_TOKEN(TOK_EVENTS_EQ_HEAD,
						ud->ctx.ptl_pid, HFI_PSB_EQ_HEAD_SIZE);
		ctxt_attach.pt_token = HFI_MMAP_PSB_TOKEN(TOK_PORTALS_TABLE,
						ud->ctx.ptl_pid, HFI_PSB_PT_SIZE);
		ctxt_attach.le_me_token = HFI_MMAP_PSB_TOKEN(TOK_LE_ME,
						ud->ctx.ptl_pid, ud->ctx.le_me_size);
		ctxt_attach.unexpected_token = HFI_MMAP_PSB_TOKEN(TOK_UNEXPECTED,
						ud->ctx.ptl_pid, ud->ctx.unexpected_size);
		ctxt_attach.trig_op_token = HFI_MMAP_PSB_TOKEN(TOK_TRIG_OP,
						ud->ctx.ptl_pid, ud->ctx.trig_op_size);

		ctxt_attach.pid = ud->ctx.ptl_pid;
		ctxt_attach.pid_base = ud->ctx.pid_base;
		ctxt_attach.pid_count = ud->ctx.pid_count;
		break;
	case HFI_CMD_CTXT_DETACH:
		/* release our assigned PID */
		ops->ctx_release(&ud->ctx);
		break;
	case HFI_CMD_JOB_INFO:
		ret = hfi_job_info(ud, &job_info);
		break;
	case HFI_CMD_JOB_SETUP:
		ret = hfi_job_setup(ud, &job_setup);
		break;
	case HFI_CMD_MPIN:
		ret = hfi_mpin(ud, &mpin);
		break;
	case HFI_CMD_MUNPIN:
		ret = hfi_munpin(ud, &munpin);
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

static ssize_t hfi_aio_write(struct kiocb *kiocb, const struct iovec *iovec,
		     unsigned long dim, loff_t offset)
{
	return -ENOSYS;
}

static int hfi_mmap(struct file *fp, struct vm_area_struct *vma)
{
	struct hfi_userdata *ud;
	unsigned long flags, pfn;
	void *remap_addr, *kvaddr = NULL;
	u64 token = vma->vm_pgoff << PAGE_SHIFT;
	u64 phys_addr = 0;
	int high = 0, mapio = 0, vm_fault = 0, vm_ro = 0, type;
	ssize_t memlen = 0;
	int ret = 0;
	u16 ctxt;
	struct opa_core_ops *ops;

	ud = fp->private_data;
	BUG_ON(!ud || !ud->bus_ops);
	ops = ud->bus_ops;

	if (!is_valid_mmap(token) ||
	    !(vma->vm_flags & VM_SHARED)) {
		ret = -EINVAL;
		goto done;
	}

	/* validate we have an assigned Portals PID */
	if (ud->ctx.ptl_pid == HFI_PID_NONE) {
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
	case TOK_CQ_TX:
	case TOK_CQ_RX:
		mapio = 1;
	case TOK_CQ_HEAD:
		if (ctxt >= HFI_CQ_COUNT) {
			ret = -EINVAL;
			goto done;
		}
		if (type == TOK_CQ_HEAD)
			vm_ro = 1;
		break;
	case TOK_CONTROL_BLOCK:
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
	 * For CQ tokens, ownership tested here.
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
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	} else {
		/* align remap_addr as may be offset into PAGE */
		kvaddr = (void *)((u64)remap_addr & PAGE_MASK);
		phys_addr = kvirt_to_phys(kvaddr, &high);
	}

	vma->vm_flags = flags;
	pr_info("%s: %u type:%u io/vf/h:%d/%d/%d, addr:0x%llx, len:%lu(%lu), flags:0x%lx\n",
		__func__, ctxt, type, mapio, vm_fault, high,
		phys_addr, memlen, vma->vm_end - vma->vm_start,
		vma->vm_flags);
	pfn = (unsigned long)(phys_addr >> PAGE_SHIFT);
	if (vm_fault) {
		/* the fault handler only supports vmalloc */
		BUG_ON(!high);
		vma->vm_pgoff = pfn;
		vma->vm_ops = &vm_ops;
		ret = 0;
	} else if (mapio) {
		ret = io_remap_pfn_range(vma, vma->vm_start, pfn, memlen,
					 vma->vm_page_prot);
	} else if (high) {
		ret = remap_vmalloc_range_partial(vma, vma->vm_start,
						  kvaddr, memlen);
	} else {
		ret = remap_pfn_range(vma, vma->vm_start, pfn, memlen,
				      vma->vm_page_prot);
	}
done:
	return ret;
}

int hfi_user_cleanup(struct hfi_userdata *ud)
{
	struct opa_core_ops *ops = ud->bus_ops;

	/* release Portals resources acquired by the HFI user */
	ops->ctx_release(&ud->ctx);
	/* release any held PID reservations */
	hfi_job_free(ud);

	/* unpin memory that user didn't clean up */
	hfi_munpin_all(ud);

	return 0;
}

static int hfi_close(struct inode *inode, struct file *fp)
{
	struct hfi_userdata *ud = fp->private_data;

	fp->private_data = NULL;
	hfi_user_cleanup(ud);
	kfree(ud);
	return 0;
}

/*
 * Local (non-chip) user memory is not mapped right away but as it is
 * accessed by the user-level code.
 */
static int vma_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct page *page;

	page = vmalloc_to_page((void *)(vmf->pgoff << PAGE_SHIFT));
	if (!page)
		return VM_FAULT_SIGBUS;

	get_page(page);
	vmf->page = page;

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
	struct opa_core_device *odev = hi->odev;

	mdev = &hi->user_miscdev;
	mdev->minor = MISC_DYNAMIC_MINOR;
	snprintf(hi->user_name, sizeof(hi->user_name), "%s%d",
		 DRIVER_DEVICE_PREFIX, odev->index);
	mdev->name = hi->user_name;
	mdev->fops = &hfi_file_ops;
	mdev->parent = &odev->dev;

	rc = misc_register(mdev);
	if (rc)
		dev_err(&odev->dev, "%s failed rc %d\n", __func__, rc);
	return rc;
}

void hfi_user_remove(struct hfi_info *hi)
{
	misc_deregister(&hi->user_miscdev);
}
