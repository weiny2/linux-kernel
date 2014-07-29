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
#include <linux/sched.h>
#include "hfi.h"
#include "hfi_cmd.h"
#include "hfi_token.h"
#include "device.h"

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
	struct hfi_devdata *dd;
	struct hfi_userdata *ud;
	unsigned i_minor = iminor(inode);

	BUG_ON(!i_minor);

	ud = kzalloc(sizeof(struct hfi_userdata), GFP_KERNEL);
	if (!ud)
		return -ENOMEM;
	fp->private_data = ud;

	/* lookup and store pointer to HFI device data */
	dd = container_of(inode->i_cdev, struct hfi_devdata, user_cdev);
	ud->devdata = dd;

	/* no cpu affinity by default */
	ud->rec_cpu_num = -1;

	return 0;
}


static ssize_t hfi_write(struct file *fp, const char __user *data, size_t count,
			 loff_t *offset)
{
	struct hfi_userdata *ud;
	const struct hfi_cmd __user *ucmd;
	struct hfi_cmd cmd;
	ssize_t consumed = 0, copy = 0, ret = 0;
	void *dest = NULL;

	if (count < sizeof(cmd)) {
		ret = -EINVAL;
		goto err_cmd;
	}
	ud = fp->private_data;
	BUG_ON(!ud || !ud->devdata);

	ucmd = (const struct hfi_cmd __user *)data;
	if (copy_from_user(&cmd, ucmd, sizeof(cmd))) {
		ret = -EFAULT;
		goto err_cmd;
	}

	consumed = sizeof(cmd);

	switch (cmd.type) {
	default:
		ret = -EINVAL;
		goto err_cmd;
	}

	/* If the command comes with user data, copy it. */
	if (copy) {
		if (copy_from_user(dest, (void __user *)ucmd->context, copy)) {
			ret = -EFAULT;
			goto err_cmd;
		}
		consumed += copy;
	}

	switch (cmd.type) {
	default:
		ret = -EINVAL;
		goto err_cmd;
	}

	if (ret == 0)
		ret = consumed;
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
	struct hfi_devdata *dd;
	unsigned long flags, pfn;
	void *kvaddr = NULL;
	u64 token = vma->vm_pgoff << PAGE_SHIFT;
	u64 memaddr = 0;
	int high = 0, mapio = 0, vm_fault = 0, type;
	ssize_t memlen = 0;
	int ret = 0;
	u16 ctxt;

	ud = fp->private_data;
	BUG_ON(!ud || !ud->devdata);
	dd = ud->devdata;

	if (!is_valid_mmap(token) ||
	    !(vma->vm_flags & VM_SHARED)) {
		ret = -EINVAL;
		goto done;
	}

	ctxt = HFI_MMAP_TOKEN_GET(CTXT, token);
	type = HFI_MMAP_TOKEN_GET(TYPE, token);
	flags = vma->vm_flags;

	switch (type) {
	default:
		ret = -EINVAL;
		goto done;
	}

	if ((vma->vm_end - vma->vm_start) != memlen) {
		ret = -EINVAL;
		goto done;
	}

	vma->vm_flags = flags;
	dd_dev_info(dd,
		    "%s: %u type:%u io/vf/h:%d/%d/%d, addr:0x%llx, len:%lu(%lu), flags:0x%lx\n",
		    __func__, ctxt, type, mapio, vm_fault, high,
		    memaddr, memlen, vma->vm_end - vma->vm_start,
		    vma->vm_flags);
	pfn = (unsigned long)(memaddr >> PAGE_SHIFT);
	if (vm_fault) {
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

void hfi_user_remove(struct hfi_devdata *dd)
{
	hfi_cdev_cleanup(&dd->user_cdev, &dd->user_device);
}

int hfi_user_add(struct hfi_devdata *dd)
{
	char name[10];
	int ret;

	/* Note minor=0 is reserved; hence we use unit+1 */
	snprintf(name, sizeof(name), "%s%d", DRIVER_DEVICE_PREFIX, dd->unit);
	ret = hfi_cdev_init(dd->unit + 1, name, &hfi_file_ops,
			    &dd->user_cdev, &dd->user_device);
	if (ret)
		goto done;

	dev_set_drvdata(dd->user_device, dd);

	return 0;

done:
	hfi_user_remove(dd);
	return ret;
}
