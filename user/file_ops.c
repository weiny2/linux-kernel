/*
 * Copyright (c) 2013 - 2014 Intel Corporation.  All rights reserved.
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
#include <linux/cdev.h>
#include <linux/vmalloc.h>
#include <linux/io.h>
#include <linux/uio.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/cred.h>
#include <linux/version.h>
#include "../common/opa.h"
#include "../common/hfi_token.h"
#include "device.h"

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
					   struct hfi_info, miscdev);

	ud = kzalloc(sizeof(struct hfi_userdata), GFP_KERNEL);
	if (!ud)
		return -ENOMEM;
	fp->private_data = ud;
	ud->hi = hi;

	/* lookup and store pointer to HFI device data */
	/* TODO */
	ud->bus_ops = hi->odev->bus_ops;
	ud->devdata = hi->odev->dd;

	/* no cpu affinity by default */
	ud->rec_cpu_num = -1;

	ud->pid = task_pid_nr(current);
	ud->sid = task_session_vnr(current);
	/* default Portals PID and UID */
	ud->ptl_pid = HFI_PID_NONE;
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0)
	ud->ptl_uid = current_uid();
#else
	ud->ptl_uid = current_uid().val;
#endif
	INIT_LIST_HEAD(&ud->mpin_head);
	spin_lock_init(&ud->mpin_lock);

	ud->bus_ops->job_init(ud);

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
	struct hfi_dlid_assign_args dlid_assign;
	struct hfi_ptl_attach_args ptl_attach;
	struct hfi_job_info_args job_info;
	struct hfi_job_setup_args job_setup;
	struct hfi_mpin_args mpin;
	struct hfi_munpin_args munpin;
	int need_admin = 0;
	ssize_t consumed = 0, copy_in = 0, copy_out = 0, ret = 0;
	void *dest = NULL;
	void __user *user_data = NULL;
	struct opa_core_ops *ops;

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
		dest = &cq_assign;
		break;
	case HFI_CMD_CQ_UPDATE:
		copy_in = sizeof(cq_update);
		dest = &cq_update;
		break;
	case HFI_CMD_CQ_RELEASE:
		copy_in = sizeof(cq_release);
		dest = &cq_release;
		break;
	case HFI_CMD_PTL_ATTACH:
		copy_in = sizeof(ptl_attach);
		copy_out = copy_in;
		dest = &ptl_attach;
		break;
	case HFI_CMD_PTL_DETACH:
		copy_in = 0;
		break;
	case HFI_CMD_DLID_ASSIGN:
		copy_in = sizeof(dlid_assign);
		dest = &dlid_assign;
		need_admin = 1;
		break;
	case HFI_CMD_DLID_RELEASE:
		copy_in = 0;
		need_admin = 1;
		break;
	case HFI_CMD_JOB_INFO:
		copy_out = sizeof(job_info);
		dest = &job_info;
		break;
	case HFI_CMD_JOB_SETUP:
		copy_in = sizeof(job_setup);
		dest = &job_setup;
		need_admin = 1;
		break;
	case HFI_CMD_MPIN:
		copy_in = sizeof(mpin);
		copy_out = copy_in;
		dest = &mpin;
		break;
	case HFI_CMD_MUNPIN:
		copy_in = sizeof(munpin);
		dest = &munpin;
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
		if (copy_from_user(dest, user_data, copy_in)) {
			ret = -EFAULT;
			goto err_cmd;
		}
		consumed += copy_in;
	}

	switch (cmd.type) {
	case HFI_CMD_CQ_ASSIGN:
		ret = ops->cq_assign(ud, &cq_assign);
		break;
	case HFI_CMD_CQ_UPDATE:
		ret = ops->cq_update(ud, &cq_update);
		break;
	case HFI_CMD_CQ_RELEASE:
		ret = ops->cq_release(ud, cq_release.cq_idx);
		break;
	case HFI_CMD_DLID_ASSIGN:
		ret = ops->dlid_assign(ud, &dlid_assign);
		break;
	case HFI_CMD_DLID_RELEASE:
		ret = ops->dlid_release(ud);
		break;
	case HFI_CMD_PTL_ATTACH:
		ret = ops->ctxt_assign(ud, &ptl_attach);
		break;
	case HFI_CMD_PTL_DETACH:
		/* release our assigned PID */
		ops->ctxt_release(ud);
		break;
	case HFI_CMD_JOB_INFO:
		ret = ops->job_info(ud, &job_info);
		break;
	case HFI_CMD_JOB_SETUP:
		ret = ops->job_setup(ud, &job_setup);
		break;
	case HFI_CMD_MPIN:
		ret = hfi_mpin(ud, &mpin);
		break;
	case HFI_CMD_MUNPIN:
		ret = hfi_munpin(ud, &munpin);
		break;
	default:
		ret = -EINVAL;
		goto err_cmd;
	}

	if (ret == 0) {
		ret = consumed;
		if (copy_out && copy_to_user(user_data, dest, copy_out)) {
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
	if (ud->ptl_pid == HFI_PID_NONE) {
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
	ret = ops->ctxt_addr(ud, type, ctxt, &remap_addr, &memlen);
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
		kvaddr = remap_addr;
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
	ops->ctxt_release(ud);
	/* release any held PID reservations */
	ops->job_free(ud);

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
 * This handles vvmalloc'ed or kmalloc'ed addresses.
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

	mdev = &hi->miscdev;
	mdev->minor = MISC_DYNAMIC_MINOR;
	snprintf(hi->name, sizeof(hi->name), "%s%d",
		 DRIVER_DEVICE_PREFIX, odev->index);
	mdev->name = hi->name;
	mdev->fops = &hfi_file_ops;
	mdev->parent = &odev->dev;

	rc = misc_register(mdev);
	if (rc)
		dev_err(&odev->dev, "%s failed rc %d\n", __func__, rc);
	return rc;
}

void hfi_user_remove(struct hfi_info *hi)
{
	misc_deregister(&hi->miscdev);
}
