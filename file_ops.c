/*
 * Copyright (c) 2013 Intel Corporation.  All rights reserved.
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
#include <linux/pci.h>
#include <linux/poll.h>
#include <linux/cdev.h>
#include <linux/swap.h>
#include <linux/vmalloc.h>
#include <linux/highmem.h>
#include <linux/io.h>
#include <linux/uio.h>
#include <linux/jiffies.h>
#include <asm/pgtable.h>
#include <linux/delay.h>
#include <linux/export.h>
#include <linux/module.h>
#include <linux/cred.h>

#include "hfi.h"
#include "pio.h"
#include "device.h"
#include "common.h"
#include "trace.h"

#undef pr_fmt
#define pr_fmt(fmt) DRIVER_NAME ": " fmt

uint hdrsup_enable = 1;
module_param(hdrsup_enable, uint, S_IRUGO);
MODULE_PARM_DESC(hdrsup_enable, "Enable/disable header suppression");

/*
 * File operation functions
 */
static int hfi_open(struct inode *, struct file *);
static int hfi_close(struct inode *, struct file *);
static ssize_t hfi_write(struct file *, const char __user *, size_t, loff_t *);
static ssize_t hfi_aio_write(struct kiocb *, const struct iovec *,
			     unsigned long, loff_t);
static unsigned int hfi_poll(struct file *, struct poll_table_struct *);
static int hfi_mmap(struct file *, struct vm_area_struct *);

static u64 kvirt_to_phys(void *);
static int assign_ctxt(struct file *, struct hfi_user_info *);
static int init_subctxts(struct qib_ctxtdata *, const struct hfi_user_info *);
static int user_init(struct file *);
static int get_ctxt_info(struct file *, void __user *, __u32);
static int get_base_info(struct file *, void __user *, __u32);
static int setup_ctxt(struct file *, struct hfi_ctxt_setup *);
static int setup_subctxt(struct qib_ctxtdata *);
static int get_user_context(struct file *, struct hfi_user_info *,
			    int, unsigned);
static int find_shared_ctxt(struct file *, const struct hfi_user_info *);
static int allocate_ctxt(struct file *, struct hfi_devdata *,
			 struct hfi_user_info *);
static unsigned int poll_urgent(struct file *, struct poll_table_struct *);
static unsigned int poll_next(struct file *, struct poll_table_struct *);
static int user_event_ack(struct qib_ctxtdata *, int, unsigned long);
static int set_ctxt_pkey(struct qib_ctxtdata *, unsigned, u16);
static int manage_rcvq(struct qib_ctxtdata *, unsigned, int);
static int vma_fault(struct vm_area_struct *, struct vm_fault *);
static int exp_tid_setup(struct file *, struct hfi_tid_info *);
static int exp_tid_free(struct file *, struct hfi_tid_info *);
static void unlock_exp_tids(struct qib_ctxtdata *);

static const struct file_operations qib_file_ops = {
	.owner = THIS_MODULE,
	.write = hfi_write,
	.aio_write = hfi_aio_write,
	.open = hfi_open,
	.release = hfi_close,
	.poll = hfi_poll,
	.mmap = hfi_mmap,
	.llseek = noop_llseek,
};

static struct vm_operations_struct vm_ops = {
	.fault = vma_fault,
};

/*
 * Types of memories mapped into user processes' space
 */
enum mmap_types {
	PIO_BUFS = 1,
	PIO_BUFS_SOP,
	PIO_CRED,
	RCV_HDRQ,
	RCV_EGRBUF,
	UREGS,
	EVENTS,
	STATUS,
	RTAIL,
	SUBCTXT_UREGS,
	SUBCTXT_RCV_HDRQ,
	SUBCTXT_EGRBUF,
	SDMA_COMP
};

/*
 * Masks and offsets defining the mmap tokens
 */
#define HFI_MMAP_OFFSET_MASK   0xfffULL
#define HFI_MMAP_OFFSET_SHIFT  0
#define HFI_MMAP_SUBCTXT_MASK  0xfULL
#define HFI_MMAP_SUBCTXT_SHIFT 12
#define HFI_MMAP_CTXT_MASK     0xffULL
#define HFI_MMAP_CTXT_SHIFT    16
#define HFI_MMAP_TYPE_MASK     0xfULL
#define HFI_MMAP_TYPE_SHIFT    24
#define HFI_MMAP_MAGIC_MASK    0xffffffffULL
#define HFI_MMAP_MAGIC_SHIFT   32

#define HFI_MMAP_MAGIC         0xdabbad00

#define HFI_MMAP_TOKEN_SET(field, val)	\
	(((val) & HFI_MMAP_##field##_MASK) << HFI_MMAP_##field##_SHIFT)
#define HFI_MMAP_TOKEN_GET(field, token) \
	(((token) >> HFI_MMAP_##field##_SHIFT) & HFI_MMAP_##field##_MASK)
#define HFI_MMAP_TOKEN(type, ctxt, subctxt, addr)   \
	HFI_MMAP_TOKEN_SET(MAGIC, HFI_MMAP_MAGIC) | \
	HFI_MMAP_TOKEN_SET(TYPE, type) | \
	HFI_MMAP_TOKEN_SET(CTXT, ctxt) | \
	HFI_MMAP_TOKEN_SET(SUBCTXT, subctxt) | \
	HFI_MMAP_TOKEN_SET(OFFSET, ((unsigned long)addr & ~PAGE_MASK))

#define EXP_TID_TIDLEN_MASK   0x7FFULL
#define EXP_TID_TIDLEN_SHIFT  0
#define EXP_TID_TIDCTRL_MASK  0x3ULL
#define EXP_TID_TIDCTRL_SHIFT 20
#define EXP_TID_TIDIDX_MASK   0x7FFULL
#define EXP_TID_TIDIDX_SHIFT  22
#define EXP_TID_GET(tid, field)				\
	(((tid) >> EXP_TID_TID##field##_SHIFT) &	\
	 EXP_TID_TID##field##_MASK)
#define EXP_TID_SET(field, value)			\
	(((value) & EXP_TID_TID##field##_MASK) <<	\
	 EXP_TID_TID##field##_SHIFT)
#define EXP_TID_CLEAR(tid, field) {					\
		(tid) &= ~(EXP_TID_TID##field##_MASK <<			\
			   EXP_TID_TID##field##_SHIFT);			\
			}
#define EXP_TID_RESET(tid, field, value) do {				\
		EXP_TID_CLEAR(tid, field);				\
		(tid) |= EXP_TID_SET(field, value);			\
	} while (0)

#define dbg(fmt, ...)				\
	pr_info(fmt, ##__VA_ARGS__)


static inline int is_valid_mmap(u64 token)
{
	return (HFI_MMAP_TOKEN_GET(MAGIC, token) == HFI_MMAP_MAGIC);
}

static int hfi_open(struct inode *inode, struct file *fp)
{
	/* The real work is performed later in assign_ctxt() */
	fp->private_data = kzalloc(sizeof(struct hfi_filedata), GFP_KERNEL);
	if (fp->private_data) /* no cpu affinity by default */
		((struct hfi_filedata *)fp->private_data)->rec_cpu_num = -1;
	return fp->private_data ? 0 : -ENOMEM;	
}

static ssize_t hfi_write(struct file *fp, const char __user *data, size_t count,
			 loff_t *offset)
{
	const struct hfi_cmd __user *ucmd;
	struct qib_ctxtdata *uctxt = ctxt_fp(fp);
	struct hfi_cmd cmd;
	struct hfi_user_info uinfo;
	struct hfi_tid_info tinfo;
	struct hfi_ctxt_setup setup;
	ssize_t consumed = 0, copy = 0, ret = 0;
	void *dest = NULL;
	__u64 user_val = 0;

	if (count < sizeof(cmd)) {
		ret = -EINVAL;
		goto bail;
	}

	ucmd = (const struct hfi_cmd __user *)data;
	if (copy_from_user(&cmd, ucmd, sizeof(cmd))) {
		ret = -EFAULT;
		goto bail;
	}

	consumed = sizeof(cmd);

	switch (cmd.type) {
	case HFI_CMD_ASSIGN_CTXT:
		copy = sizeof(uinfo);
		dest = &uinfo;
		break;
	case HFI_CMD_CTXT_SETUP:
		copy = sizeof(setup);
		dest = &setup;
		break;
	case HFI_CMD_SDMA_STATUS_UPD:
	case HFI_CMD_CREDIT_UPD:
		copy = 0;
		break;
	case HFI_CMD_TID_UPDATE:
	case HFI_CMD_TID_FREE:
		copy = sizeof(tinfo);
		dest = &tinfo;
		break;
	case HFI_CMD_USER_INFO:
	case HFI_CMD_RECV_CTRL:
	case HFI_CMD_POLL_TYPE:
	case HFI_CMD_ACK_EVENT:
	case HFI_CMD_CTXT_INFO:
	case HFI_CMD_SET_PKEY:
		copy = 0;
		user_val = ucmd->addr;
		break;
	default:
		ret = -EINVAL;
		goto bail;
	}

	/* If the command comes with user data, copy it. */
	if (copy) {
		if (copy_from_user(dest, (void __user *)ucmd->addr, copy)) {
			ret = -EFAULT;
			goto bail;
		}
		consumed += copy;
	}

	/*
	 * There is only one case where it is OK not to have
	 * a qib_ctxtdata structure yet.
	 */
	if (!uctxt && cmd.type != HFI_CMD_ASSIGN_CTXT) {
		ret = -EINVAL;
		goto bail;
	}

	switch (cmd.type) {
	case HFI_CMD_ASSIGN_CTXT:
		ret = assign_ctxt(fp, &uinfo);
		if (ret)
			goto bail;
		break;
	case HFI_CMD_CTXT_INFO:
		ret = get_ctxt_info(fp, (void __user *)(unsigned long)
				    user_val, cmd.len);
		break;
	case HFI_CMD_CTXT_SETUP:
		ret = setup_ctxt(fp, &setup);
		if (ret)
			goto bail;
		ret = user_init(fp);
		break;
	case HFI_CMD_USER_INFO:
		ret = get_base_info(fp, (void __user *)(unsigned long)
				    user_val, cmd.len);
		break;
	case HFI_CMD_SDMA_STATUS_UPD:
		break;
	case HFI_CMD_CREDIT_UPD:
		if (uctxt && uctxt->sc)
			sc_return_credits(uctxt->sc);
		break;
	case HFI_CMD_TID_UPDATE:
		ret = exp_tid_setup(fp, &tinfo);
		if (!ret) {
			unsigned long addr;
			/* Copy the mapped length back to user space */
			addr = (unsigned long)ucmd->addr +
				offsetof(struct hfi_tid_info, length);
			if (copy_to_user((void __user *)addr, &tinfo.length,
					 sizeof(tinfo.length))) {
				ret = -EFAULT;
				goto bail;
			}
			/* Now, copy the number of tidlist entries we used */
			addr = (unsigned long)ucmd->addr +
				offsetof(struct hfi_tid_info, tidcnt);
			if (copy_to_user((void __user *)addr, &tinfo.tidcnt,
					 sizeof(tinfo.tidcnt)))
				ret = -EFAULT;
		}
		break;
	case HFI_CMD_TID_FREE:
		ret = exp_tid_free(fp, &tinfo);
		break;
	case HFI_CMD_RECV_CTRL:
		ret = manage_rcvq(uctxt, subctxt_fp(fp), (int)user_val);
		break;
	case HFI_CMD_POLL_TYPE:
		uctxt->poll_type = (typeof(uctxt->poll_type))user_val;
		break;
	case HFI_CMD_ACK_EVENT:
		ret = user_event_ack(uctxt, subctxt_fp(fp), user_val);
		break;
	case HFI_CMD_SET_PKEY:
		ret = set_ctxt_pkey(uctxt, subctxt_fp(fp), user_val);
	}

	if (ret >= 0)
		ret = consumed;
bail:
	return ret;
}

static ssize_t hfi_aio_write(struct kiocb *kiocb, const struct iovec *iovec,
		     unsigned long dim, loff_t offset)
{
	//struct qib_ctxtdata *uctxt = ctxt_fp(kiocb->ki_filp);
	//struct hfi_sdma_request_ring *sdmar = uctxt->sdma_ring;

	//if (!dim || !sdmar)
	//	return -EINVAL;
	return 0;
}

static int hfi_mmap(struct file *fp, struct vm_area_struct *vma)
{
	struct qib_ctxtdata *uctxt;
	struct hfi_devdata *dd;
	unsigned long flags, pfn;
	u64 token = vma->vm_pgoff << PAGE_SHIFT,
		memaddr = 0;
	u8 subctxt, mapio = 0, vmf = 0, type;
	ssize_t memlen = 0;
	int ret = 0;
	u16 ctxt;

	uctxt = ctxt_fp(fp);
	if (!is_valid_mmap(token) || !uctxt ||
	    !(vma->vm_flags & VM_SHARED)) {
		ret = -EINVAL;
		goto done;
	}
	dd = uctxt->dd;
	ctxt = HFI_MMAP_TOKEN_GET(CTXT, token);
	subctxt = HFI_MMAP_TOKEN_GET(SUBCTXT, token);
	type = HFI_MMAP_TOKEN_GET(TYPE, token);
	if (ctxt != uctxt->ctxt || subctxt != subctxt_fp(fp)) {
		ret = -EINVAL;
		goto done;
	}

	flags = vma->vm_flags;

	switch(type) {
	case PIO_BUFS:
	case PIO_BUFS_SOP:
		memaddr = ((dd->physaddr + WFR_TXE_PIO_SEND) + /* chip pio base */
			   (uctxt->sc->context * (1 << 16))) + /* 64K PIO space / ctxt */
			(type == PIO_BUFS_SOP ? (WFR_TXE_PIO_SIZE / 2) : 0); /* sop? */
		/*
		 * Map only the amount allocated to the context, not the
		 * entire available context's PIO space.
		 */
		memlen = ALIGN(uctxt->sc->credits * WFR_PIO_BLOCK_SIZE,
			       PAGE_SIZE);
		flags &= ~VM_MAYREAD;
		flags |= VM_DONTCOPY | VM_DONTEXPAND;
		// XXX (Mitko): how do we deal with write-combining?
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
		mapio = 1;
		break;
	case PIO_CRED:
		if (flags & VM_WRITE) {
			ret = -EPERM;
			goto done;
		}
		/*
		 * The credit return location for this context could be on the
		 * second or third page allocated for credit returns (if number
		 * of enabled contexts > 64 and 128 respectively).
		 */
		memaddr = dd->cr_base[uctxt->numa_id].pa +
			(((u64)uctxt->sc->hw_free -
			  (u64)dd->cr_base[uctxt->numa_id].va) & PAGE_MASK);
		memlen = PAGE_SIZE;
		flags &= ~VM_MAYWRITE;
		flags |= VM_DONTCOPY | VM_DONTEXPAND;
		/*
		 * The driver has already allocated memory for credit
		 * returns and programmed it into the chip. Has that
		 * memory been flaged as non-cached?
		 */
		//vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
		mapio = 1;
		break;
	case RCV_HDRQ:
		memaddr = uctxt->rcvhdrq_phys;
		memlen = uctxt->rcvhdrq_size;
		break;
	case RCV_EGRBUF: {
		ssize_t size;
		unsigned long addr;
		int i;
		/*
		 * The RcvEgr buffer need to be handled differently
		 * as multiple non-contiguous pages need to be mapped
		 * into the user process.
		 */
		size = uctxt->rcvegrbuf_chunksize;
		memlen = uctxt->rcvegrbuf_chunks * size;
		if ((vma->vm_end - vma->vm_start) < memlen) {
			ret = -EINVAL;
			goto done;
		}
		if (vma->vm_flags & VM_WRITE) {
			ret = -EPERM;
			goto done;
		}
		vma->vm_flags &= ~VM_MAYWRITE;
		addr = vma->vm_start;
		for (i=0 ; i < uctxt->rcvegrbuf_chunks; i++, addr += size) {
			ret = remap_pfn_range(vma, addr,
					 uctxt->rcvegrbuf_phys[i] >> PAGE_SHIFT,
					 size, vma->vm_page_prot);
			if (ret < 0)
				goto done;
		}
		ret = 0;
		goto done;
	}
	case UREGS:
		/*
		 * Map only the page that contains this context's user
		 * registers.
		 */
		memaddr = (unsigned long)(dd->physaddr + dd->uregbase) +
			(uctxt->ctxt * dd->ureg_align);
		/*
		 * TidFlow table is on the same page as the rest of the
		 * user registers.
		 */
		memlen = PAGE_SIZE;
		flags |= VM_DONTCOPY | VM_DONTEXPAND;
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
		mapio = 1;
		break;
	case EVENTS:
		/*
		 * One page containing events for all ctxts. User level
		 * knows where it's own bitmap is.
		 */
		memaddr = (u64)dd->events;
		memlen = PAGE_SIZE;
		/*
		 * v3.7 removes VM_RESERVED but the effect is kept by
		 * using VM_IO.
		 */
		flags |= VM_IO | VM_DONTEXPAND;
		vmf = 1;
		break;
	case STATUS:
		memaddr = kvirt_to_phys((void *)dd->status);
		memlen = PAGE_SIZE;
		flags |= VM_IO | VM_DONTEXPAND;
		break;
	case RTAIL:
		if (flags & VM_WRITE) {
			ret = -EPERM;
			goto done;
		}
		memaddr = uctxt->rcvhdrqtailaddr_phys;
		memlen = PAGE_SIZE;
		flags &= ~VM_MAYWRITE;
		break;
	case SUBCTXT_UREGS:
		memaddr = (u64)uctxt->subctxt_uregbase;
		memlen = PAGE_SIZE;
		flags |= VM_IO | VM_DONTEXPAND;
		vmf = 1;
		break;
	case SUBCTXT_RCV_HDRQ:
		memaddr = (u64)uctxt->subctxt_rcvhdr_base;
		memlen = uctxt->rcvhdrq_size * uctxt->subctxt_cnt;
		flags |= VM_IO | VM_DONTEXPAND;
		vmf = 1;
		break;
	case SUBCTXT_EGRBUF:
		memaddr = (u64)uctxt->subctxt_rcvegrbuf;
		memlen = uctxt->rcvegrbuf_chunks * uctxt->rcvegrbuf_chunksize *
			uctxt->subctxt_cnt;
		flags |= VM_IO | VM_DONTEXPAND;
		flags &= ~VM_MAYWRITE;
		vmf = 1;
		break;
	case SDMA_COMP:
	default:
		ret = -EINVAL;
		break;
	}

	if ((vma->vm_end - vma->vm_start) != memlen) {
		ret = -EINVAL;
		goto done;
	}

	vma->vm_flags = flags;
	dd_dev_info(dd,
		    "%s: %u:%u type:%u io/vf:%d/%d, addr:0x%llx, len:%lu(%lu), flags:0x%lx\n",
		    __func__, ctxt, subctxt, type, mapio, vmf, memaddr, memlen,
		    vma->vm_end - vma->vm_start, vma->vm_flags);
	pfn = (unsigned long)(memaddr >> PAGE_SHIFT);
	if (vmf) {
		vma->vm_pgoff = pfn;
		vma->vm_ops = &vm_ops;
		ret = 0;
	} else if (mapio) {
		ret = io_remap_pfn_range(vma, vma->vm_start, pfn, memlen,
					 vma->vm_page_prot);
	} else {
		ret = remap_pfn_range(vma, vma->vm_start, pfn, memlen,
				      vma->vm_page_prot);
	}
done:
	return ret;
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

static unsigned int hfi_poll(struct file *fp, struct poll_table_struct *pt)
{
	struct qib_ctxtdata *uctxt;
	unsigned pollflag;

	uctxt = ctxt_fp(fp);
	if (!uctxt)
		pollflag = POLLERR;
	else if (uctxt->poll_type == QIB_POLL_TYPE_URGENT)
		pollflag = poll_urgent(fp, pt);
	else  if (uctxt->poll_type == QIB_POLL_TYPE_ANYRCV)
		pollflag = poll_next(fp, pt);
	else /* invalid */
		pollflag = POLLERR;

	return pollflag;
}

static int hfi_close(struct inode *inode, struct file *fp)
{
	struct hfi_filedata *fdata = fp->private_data;
	struct qib_ctxtdata *uctxt = fdata->uctxt;
	struct hfi_devdata *dd;
	unsigned long flags;

	fp->private_data = NULL;

	if (!uctxt)
		goto done;

	dbg("freeing ctxt %u:%u\n", uctxt->ctxt, fdata->subctxt);
	dd = uctxt->dd;
	mutex_lock(&qib_mutex);

	qib_flush_wc();
	/* drain user sdma queue */
	/*if (fdata->pq) {
		qib_user_sdma_queue_drain(uctxt->ppd, fdata->pq);
		qib_user_sdma_queue_destroy(fdata->pq);
		}*/

	if (--uctxt->cnt) {
		/*
		 * XXX If the master closes the context before the slave(s),
		 * revoke the mmap for the eager receive queue so
		 * the slave(s) don't wait for receive data forever.
		 */
		uctxt->active_slaves &= ~(1 << fdata->subctxt);
		uctxt->subpid[fdata->subctxt] = 0;
		mutex_unlock(&qib_mutex);
		goto done;
	}

	spin_lock_irqsave(&dd->uctxt_lock, flags);
	/*
	 * Disable receive context and interrupt available, reset all
	 * RcvCtxtCtrl bits to default values.
	 */
	dd->f_rcvctrl(dd, QIB_RCVCTRL_CTXT_DIS | QIB_RCVCTRL_TIDFLOW_DIS |
		      QIB_RCVCTRL_INTRAVAIL_DIS | QIB_RCVCTRL_ONE_PKT_EGR_DIS |
		      QIB_RCVCTRL_NO_RHQ_DROP_DIS | QIB_RCVCTRL_NO_EGR_DROP_DIS,
		      uctxt->ctxt);
	/* Clear the context's J_KEY */
	dd->f_clear_ctxt_jkey(dd, uctxt->ctxt);
	sc_disable(uctxt->sc);
	uctxt->pid = 0;
	spin_unlock_irqrestore(&dd->uctxt_lock, flags);

	dd->rcd[uctxt->ctxt] = NULL;
	uctxt->rcvwait_to = 0;
	uctxt->piowait_to = 0;
	uctxt->rcvnowait = 0;
	uctxt->pionowait = 0;
	uctxt->event_flags = 0;

	dd->f_clear_tids(uctxt);
	dd->f_clear_ctxt_pkey(dd, uctxt->ctxt);

	if (uctxt->tid_pg_list)
		unlock_exp_tids(uctxt);

	qib_stats.sps_ctxts--;
	dd->freectxts++;
	mutex_unlock(&qib_mutex);
	qib_free_ctxtdata(dd, uctxt);
done:
	kfree(fdata);
	return 0;
}

/*
 * Convert kernel *virtual* addresses to physical addresses.
 * This is used to vmalloc'ed addresses.
 */
static u64 kvirt_to_phys(void *addr)
{
	struct page *page;
	u64 paddr = 0;

	page = vmalloc_to_page(addr);
	if (page)
		paddr = page_to_pfn(page) << PAGE_SHIFT;

	return paddr;
}

static int assign_ctxt(struct file *fp, struct hfi_user_info *uinfo)
{
	int i_minor, ret = 0;
	unsigned swmajor, swminor, alg = HFI_ALG_ACROSS;

	swmajor = uinfo->userversion >> 16;
	if (swmajor != QIB_USER_SWMAJOR) {
		ret = -ENODEV;
		goto done;
	}

	swminor = uinfo->userversion & 0xffff;

	if (uinfo->hfi_alg < HFI_ALG_COUNT)
		alg = uinfo->hfi_alg;

	mutex_lock(&qib_mutex);
	/* First, lets check if we need to setup a shared context? */
	if (uinfo->subctxt_cnt)
		ret = find_shared_ctxt(fp, uinfo);

	/*
	 * We execute the following block if we couldn't find a
	 * shared context or if context sharing is not required.
	 */
	if (!ret) {
		i_minor = iminor(file_inode(fp)) - HFI_USER_MINOR_BASE;
		ret = get_user_context(fp, uinfo, i_minor - 1, alg);
	}
	mutex_unlock(&qib_mutex);
done:
	return ret;
}

static int get_user_context(struct file *fp, struct hfi_user_info *uinfo,
			    int devno, unsigned alg)
{
	struct hfi_devdata *dd = NULL;
	int ret = 0, devmax, npresent, nup, dev;

	devmax = qib_count_units(&npresent, &nup);
	if (!npresent) {
		ret = -ENXIO;
		goto done;
	}
	if (!nup) {
		ret = -ENETDOWN;
		goto done;
	}
	if (devno >= 0) {
		dd = qib_lookup(devno);
		if (!dd)
			ret = -ENODEV;
		else if (!dd->freectxts)
			ret = -EBUSY;
	} else {
		struct hfi_devdata *pdd;
		if (alg == HFI_ALG_ACROSS) {
			unsigned free = 0U;
			for (dev = 0; dev < devmax; dev++) {
				pdd = qib_lookup(dev);
				if (pdd && pdd->freectxts &&
				    pdd->freectxts > free) {
					dd = pdd;
					free = pdd->freectxts;
				}
			}
		} else {
			for (dev = 0; dev < devmax; dev++) {
				pdd = qib_lookup(dev);
				if (pdd && pdd->freectxts) {
					dd = pdd;
					break;
				}
			}
		}
		if (!dd)
			ret = -EBUSY;
	}
done:
	return ret ? ret : allocate_ctxt(fp, dd, uinfo);
}

static int find_shared_ctxt(struct file *fp,
			    const struct hfi_user_info *uinfo)
{
	int devmax, ndev, i;
	int ret = 0;

	devmax = qib_count_units(NULL, NULL);

	for (ndev = 0; ndev < devmax; ndev++) {
		struct hfi_devdata *dd = qib_lookup(ndev);

		/* device portion of usable() */
		if (!(dd && (dd->flags & QIB_PRESENT) && dd->kregbase))
			continue;
		for (i = dd->first_user_ctxt; i < dd->num_rcv_contexts; i++) {
			struct qib_ctxtdata *uctxt = dd->rcd[i];

			/* Skip ctxts which are not yet open */
			if (!uctxt || !uctxt->cnt)
				continue;
			/* Skip ctxt if it doesn't match the requested one */
			if (memcmp(uctxt->uuid, uinfo->uuid,
				   sizeof(uctxt->uuid)) ||
			    uctxt->subctxt_id != uinfo->subctxt_id)
				continue;
			/* Verify the sharing process matches the master */
			if (uctxt->subctxt_cnt != uinfo->subctxt_cnt ||
			    uctxt->userversion != uinfo->userversion ||
			    uctxt->cnt >= uctxt->subctxt_cnt) {
				ret = -EINVAL;
				goto done;
			}
			ctxt_fp(fp) = uctxt;
			subctxt_fp(fp) = uctxt->cnt++;
			uctxt->subpid[subctxt_fp(fp)] = current->pid;
			uctxt->active_slaves |= 1 << subctxt_fp(fp);
			ret = 1;
			goto done;
		}
	}

done:
	return ret;
}

static int allocate_ctxt(struct file *fp, struct hfi_devdata *dd,
			 struct hfi_user_info *uinfo)
{
	struct qib_ctxtdata *uctxt;
	unsigned ctxt;
	int ret = 0;

	for (ctxt = dd->first_user_ctxt;
	     ctxt < dd->num_rcv_contexts && dd->rcd[ctxt]; ctxt++);
	if (ctxt == dd->num_rcv_contexts) {
		ret = -EBUSY;
		goto done;
	}
	uctxt = qib_create_ctxtdata(dd->pport, ctxt);
	if (!uctxt) {
		dd_dev_err(dd,
			   "Unable to allocate ctxtdata memory, failing open\n");
		ret = -ENOMEM;
		goto done;
	}
	/*
	 * Allocate and enable a PIO send context.
	 */
	uctxt->sc = sc_alloc(dd, SC_USER, uctxt->numa_id);
	if (!uctxt->sc) {
		ret = -ENOMEM;
		goto done;
	}
	dbg("allocated send context %d\n", uctxt->sc->context);
	ret = sc_enable(uctxt->sc);
	if (ret)
		goto done;
	/*
	 * Setup shared context resources if the user-level has requested
	 * shared contexts and this is the 'master' process.
	 * This has to be done here so the rest of the subcontexts find the
	 * proper master.
	 */
	if (uinfo->subctxt_cnt && !subctxt_fp(fp)) {
		ret = init_subctxts(uctxt, uinfo);
		/*
		 * On error, we don't need to disable and de-allocate the
		 * send context because it will be done during file close
		 */
		if (ret)
			goto done;
	}
	uctxt->userversion = uinfo->userversion;
	uctxt->pid = current->pid;
	uctxt->flags = uinfo->flags;
	init_waitqueue_head(&uctxt->wait);
	strlcpy(uctxt->comm, current->comm, sizeof(uctxt->comm));
	memcpy(uctxt->uuid, uinfo->uuid, sizeof(uctxt->uuid));
	uctxt->jkey = generate_jkey(current_uid());
	qib_stats.sps_ctxts++;
	dd->freectxts--;
	ctxt_fp(fp) = uctxt;
	ret = 0;
done:
	return ret;
}

static int init_subctxts(struct qib_ctxtdata *uctxt,
			 const struct hfi_user_info *uinfo)
{
	int ret = 0;
	unsigned num_subctxts;

	num_subctxts = uinfo->subctxt_cnt;
	if (num_subctxts > QLOGIC_IB_MAX_SUBCTXT) {
		ret = -EINVAL;
		goto bail;
	}

	uctxt->subctxt_cnt = uinfo->subctxt_cnt;
	uctxt->subctxt_id = uinfo->subctxt_id;
	uctxt->active_slaves = 1;
	uctxt->redirect_seq_cnt = 1;
	set_bit(QIB_CTXT_MASTER_UNINIT, &uctxt->event_flags);
bail:
	return ret;
}

static int setup_subctxt(struct qib_ctxtdata *uctxt)
{
	int ret = 0;
	unsigned num_subctxts = uctxt->subctxt_cnt;

	uctxt->subctxt_uregbase = vmalloc_user(PAGE_SIZE);
	if (!uctxt->subctxt_uregbase) {
		ret = -ENOMEM;
		goto bail;
	}
	/* We can take the size of the RcvHdr Queue from the master */
	uctxt->subctxt_rcvhdr_base = vmalloc_user(uctxt->rcvhdrq_size *
						  num_subctxts);
	if (!uctxt->subctxt_rcvhdr_base) {
		ret = -ENOMEM;
		goto bail_ureg;
	}

	uctxt->subctxt_rcvegrbuf = vmalloc_user(uctxt->rcvegrbuf_chunks *
						uctxt->rcvegrbuf_chunksize *
						num_subctxts);
	if (!uctxt->subctxt_rcvegrbuf) {
		ret = -ENOMEM;
		goto bail_rhdr;
	}
	goto bail;
bail_rhdr:
	vfree(uctxt->subctxt_rcvhdr_base);
bail_ureg:
	vfree(uctxt->subctxt_uregbase);
	uctxt->subctxt_uregbase = NULL;
bail:
	return ret;
}

static int user_init(struct file *fp)
{
	int ret;
	unsigned int rcvctrl_ops = 0;
	struct qib_ctxtdata *uctxt = ctxt_fp(fp);

	/* make sure that the context has already been setup */
	if (!test_bit(QIB_CTXT_SETUP_DONE, &uctxt->event_flags)) {
		ret = -EFAULT;
		goto done;
	}

	/*
	 * Subctxts don't need to initialize anything since master
	 * has done it.
	 */
	if (subctxt_fp(fp)) {
		ret = wait_event_interruptible(uctxt->wait,
			!test_bit(QIB_CTXT_MASTER_UNINIT, &uctxt->event_flags));
		goto done;
	}

	/* initialize poll variables... */
	uctxt->urgent = 0;
	uctxt->urgent_poll = 0;

	/*
	 * Now enable the ctxt for receive.
	 * For chips that are set to DMA the tail register to memory
	 * when they change (and when the update bit transitions from
	 * 0 to 1.  So for those chips, we turn it off and then back on.
	 * This will (very briefly) affect any other open ctxts, but the
	 * duration is very short, and therefore isn't an issue.  We
	 * explicitly set the in-memory tail copy to 0 beforehand, so we
	 * don't have to wait to be sure the DMA update has happened
	 * (chip resets head/tail to 0 on transition to enable).
	 */
	if (uctxt->rcvhdrtail_kvaddr)
		qib_clear_rcvhdrtail(uctxt);

	/* Setup J_KEY before enabling the context */
	uctxt->dd->f_set_ctxt_jkey(uctxt->dd, uctxt->ctxt, uctxt->jkey);

	rcvctrl_ops = QIB_RCVCTRL_CTXT_ENB;
	if (hdrsup_enable && (uctxt->flags & HFI_CTXTFLAG_TIDFLOWENABLE))
		rcvctrl_ops |= QIB_RCVCTRL_TIDFLOW_ENB;
	/*
	 * Ignore the bit in the flags for now until proper
	 * support for multiple packet per rcv array entry is
	 * added.
	 */
#if 0
	 if (uctxt->flags & HFI_CTXTFLAG_ONEPKTPEREGRBUF)
#endif
	rcvctrl_ops |= QIB_RCVCTRL_ONE_PKT_EGR_ENB;
	if (uctxt->flags & HFI_CTXTFLAG_DONTDROPEGRFULL)
		rcvctrl_ops |= QIB_RCVCTRL_NO_EGR_DROP_ENB;
	if (uctxt->flags & HFI_CTXTFLAG_DONTDROPHDRQFULL)
		rcvctrl_ops |= QIB_RCVCTRL_NO_RHQ_DROP_ENB;
	if (!(uctxt->dd->flags & QIB_NODMA_RTAIL))
		rcvctrl_ops |= QIB_RCVCTRL_TAILUPD_ENB;
	uctxt->dd->f_rcvctrl(uctxt->dd, rcvctrl_ops, uctxt->ctxt);

	/* Notify any waiting slaves */
	if (uctxt->subctxt_cnt) {
		clear_bit(QIB_CTXT_MASTER_UNINIT, &uctxt->event_flags);
		wake_up(&uctxt->wait);
	}
	ret = 0;

done:
	return ret;
}

static int get_ctxt_info(struct file *fp, void __user *ubase, __u32 len)
{
	struct hfi_ctxt_info cinfo;
	struct qib_ctxtdata *uctxt = ctxt_fp(fp);
	struct hfi_filedata *fd = fp->private_data;
	int ret = 0;

	cinfo.num_active = qib_count_active_units();
	cinfo.unit = uctxt->dd->unit;
	cinfo.ctxt = uctxt->ctxt;
	cinfo.subctxt = subctxt_fp(fp);
	cinfo.rcvtids = uctxt->rcv_array_groups *
		uctxt->dd->rcv_entries.group_size;
	cinfo.credits = uctxt->sc->credits;
	/* FIXME: set proper numa node */
	cinfo.numa_node = uctxt->numa_id;
	cinfo.rec_cpu = fd->rec_cpu_num;

	if (copy_to_user(ubase, &cinfo, sizeof(cinfo)))
		ret = -EFAULT;
	return ret;
}

static int setup_ctxt(struct file *fp, struct hfi_ctxt_setup *cinfo)
{
	struct qib_ctxtdata *uctxt = ctxt_fp(fp);
	struct hfi_devdata *dd = uctxt->dd;
	int ret = 0;

	trace_hfi_ctxt_setup(dd, uctxt->ctxt, subctxt_fp(fp),
			     cinfo);

	/*
	 * Context should be set up only once (including allocation and
	 * programming of eager buffers. This is done if context sharing
	 * is not requested or by the master process.
	 */
	if (!uctxt->subctxt_cnt || !subctxt_fp(fp)) {
		/*
		 * Even though we have the safeguard in f_init_ctxt, we
		 * error out if the Eager buffer count is too high so
		 * user level can clean up.
		 */
		if (cinfo->egrtids > WFR_MAX_EAGER_ENTRIES) {
			ret = -EINVAL;
			goto done;
		}
		/*
		 * Setup partitioning of RcvArray between Eager and Expected
		 * receives.
		 */
		ret = hfi_setup_ctxt(uctxt, cinfo->egrtids, cinfo->rcvegr_size,
				     cinfo->rcvhdrq_cnt, cinfo->rcvhdrq_entsize);
		if (ret)
			goto done;

		/* Now allocate the RcvHdr queue and eager buffers. */
		ret = qib_create_rcvhdrq(dd, uctxt);
		if (ret)
			goto done;
		if ((ret = qib_setup_eagerbufs(uctxt)))
			goto done;
		if (uctxt->subctxt_cnt && !subctxt_fp(fp)) {
			ret = setup_subctxt(uctxt);
			if (ret)
				goto done;
		}
		/* Setup Expected Rcv memories */
		uctxt->tid_pg_list = vzalloc(uctxt->expected_count *
					     sizeof(struct page **));
		if (!uctxt->tid_pg_list) {
			ret = -ENOMEM;
			goto done;
		}
		uctxt->physshadow = vzalloc(uctxt->expected_count *
					    sizeof(*uctxt->physshadow));
		if (!uctxt->physshadow) {
			ret = -ENOMEM;
			goto done;
		}
		/* allocate expected TID map and initialize the cursor */
		uctxt->tidcursor = 0;
		uctxt->numtidgroups = uctxt->expected_count /
			dd->rcv_entries.group_size;
		uctxt->tidmapcnt = uctxt->numtidgroups / BITS_PER_LONG +
			!!(uctxt->numtidgroups % BITS_PER_LONG);
		uctxt->tidusemap = kzalloc_node(uctxt->tidmapcnt *
						sizeof(*uctxt->tidusemap),
						GFP_KERNEL, uctxt->numa_id);
		if (!uctxt->tidusemap)
			ret = -ENOMEM;
	}
	if (ret)
		goto done;

	set_bit(QIB_CTXT_SETUP_DONE, &uctxt->event_flags);
done:
	return ret;
}

static int get_base_info(struct file *fp, void __user *ubase, __u32 len)
{
	struct hfi_base_info binfo;
	struct qib_ctxtdata *uctxt = ctxt_fp(fp);
	struct hfi_devdata *dd = uctxt->dd;
	ssize_t sz;
	unsigned offset;
	int ret = 0;

	trace_hfi_uctxtdata(uctxt->dd, uctxt);

	memset(&binfo, 0, sizeof(binfo));
	ret = dd->f_get_base_info(uctxt, &binfo);
	if (ret < 0)
		goto done;
	/* XXX MITKO: This runtime flag should be set in
	 * wfr.c:get_base_info(). However, for now set it here
	 * in accordance with the hdrsup_enable flag */
	if (hdrsup_enable)
		binfo.runtime_flags |= HFI_RUNTIME_HDRSUPP;
	/* XXX MITKO: When expTID caching is implemented, properly,
	 * handle this flag. */
	binfo.runtime_flags |= HFI_RUNTIME_TID_UNMAP;
	binfo.hw_version = dd->revision;
	binfo.sw_version = QIB_KERN_SWVERSION;
	binfo.bthqp = kdeth_qp;
	binfo.jkey = uctxt->jkey;
	/* XXX (Mitko): We are hard-coding the pport index since WFR
	 * has only one port and the expectation is that ppd will go
	 * away. */
	binfo.mtu = dd->pport[0].ibmtu;

	/*
	 * If more than 64 contexts are enabled the allocated credit
	 * return will span two or three contiguous pages. Since we only
	 * map the page containing the context's credit return address,
	 * we need to calculate the offset in the proper page.
	 */
	offset = ((u64)uctxt->sc->hw_free -
		  (u64)dd->cr_base[uctxt->numa_id].va) % PAGE_SIZE;
	binfo.sc_credits_addr = HFI_MMAP_TOKEN(PIO_CRED, uctxt->ctxt,
					       subctxt_fp(fp), offset);
	binfo.pio_bufbase = HFI_MMAP_TOKEN(PIO_BUFS, uctxt->ctxt,
					    subctxt_fp(fp),
					    uctxt->sc->base_addr);
	binfo.pio_bufbase_sop = HFI_MMAP_TOKEN(PIO_BUFS_SOP,
						uctxt->ctxt,
						subctxt_fp(fp),
						uctxt->sc->base_addr);
	binfo.rcvhdr_bufbase = HFI_MMAP_TOKEN(RCV_HDRQ, uctxt->ctxt,
					       subctxt_fp(fp),
					       uctxt->rcvhdrq);
	binfo.rcvegr_bufbase = HFI_MMAP_TOKEN(RCV_EGRBUF, uctxt->ctxt,
					       subctxt_fp(fp),
					       uctxt->rcvegr_phys);
	binfo.sdma_comp_bufbase = HFI_MMAP_TOKEN(SDMA_COMP, uctxt->ctxt,
						 subctxt_fp(fp), 0);
	// user regs are at
	//   (WFR_RXE_PER_CONTEXT_USER + (ctxt * WFR_RXE_PER_CONTEXT_SIZE))
	// or
	//   ((char *)dd->uregbase + (ctxt * dd->ureg_align))
	binfo.user_regbase = HFI_MMAP_TOKEN(UREGS, uctxt->ctxt,
					    subctxt_fp(fp), 0);
	offset = ((uctxt->ctxt * QLOGIC_IB_MAX_SUBCTXT) +
		  subctxt_fp(fp)) * sizeof(*dd->events);
	binfo.events_bufbase = HFI_MMAP_TOKEN(EVENTS, uctxt->ctxt,
					      subctxt_fp(fp),
					      offset);
	binfo.status_bufbase = HFI_MMAP_TOKEN(STATUS, uctxt->ctxt,
					      subctxt_fp(fp),
					      dd->status);
	if (!(uctxt->dd->flags & QIB_NODMA_RTAIL))
		binfo.rcvhdrtail_base = HFI_MMAP_TOKEN(RTAIL, uctxt->ctxt,
						       subctxt_fp(fp), 0);
	if (uctxt->subctxt_cnt) {
		binfo.subctxt_uregbase = HFI_MMAP_TOKEN(SUBCTXT_UREGS,
							uctxt->ctxt,
							subctxt_fp(fp), 0);
		binfo.subctxt_rcvhdrbuf = HFI_MMAP_TOKEN(SUBCTXT_RCV_HDRQ,
							 uctxt->ctxt,
							 subctxt_fp(fp), 0);
		binfo.subctxt_rcvegrbuf = HFI_MMAP_TOKEN(SUBCTXT_EGRBUF,
							 uctxt->ctxt,
							 subctxt_fp(fp), 0);
	}
	sz = (len < sizeof(binfo)) ? len : sizeof(binfo);
	if (copy_to_user(ubase, &binfo, sz))
		ret = -EFAULT;
done:
	return ret;
}

static unsigned int poll_urgent(struct file *fp,
				struct poll_table_struct *pt)
{
	struct qib_ctxtdata *uctxt = ctxt_fp(fp);
	struct hfi_devdata *dd = uctxt->dd;
	unsigned pollflag;

	poll_wait(fp, &uctxt->wait, pt);

	spin_lock_irq(&dd->uctxt_lock);
	if (uctxt->urgent != uctxt->urgent_poll) {
		pollflag = POLLIN | POLLRDNORM;
		uctxt->urgent_poll = uctxt->urgent;
	} else {
		pollflag = 0;
		set_bit(QIB_CTXT_WAITING_URG, &uctxt->event_flags);
	}
	spin_unlock_irq(&dd->uctxt_lock);

	return pollflag;
}

static unsigned int poll_next(struct file *fp,
			      struct poll_table_struct *pt)
{
	struct qib_ctxtdata *uctxt = ctxt_fp(fp);
	struct hfi_devdata *dd = uctxt->dd;
	unsigned pollflag;

	poll_wait(fp, &uctxt->wait, pt);

	spin_lock_irq(&dd->uctxt_lock);
	if (dd->f_hdrqempty(uctxt)) {
		set_bit(QIB_CTXT_WAITING_RCV, &uctxt->event_flags);
		dd->f_rcvctrl(dd, QIB_RCVCTRL_INTRAVAIL_ENB, uctxt->ctxt);
		pollflag = 0;
	} else
		pollflag = POLLIN | POLLRDNORM;
	spin_unlock_irq(&dd->uctxt_lock);

	return pollflag;
}

/*
 * Find all user contexts in use, and set the specified bit in their
 * event mask.
 * See also find_ctxt() for a similar use, that is specific to send buffers.
 */
int qib_set_uevent_bits(struct qib_pportdata *ppd, const int evtbit)
{
	struct qib_ctxtdata *uctxt;
	unsigned ctxt;
	int ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&ppd->dd->uctxt_lock, flags);
	for (ctxt = ppd->dd->first_user_ctxt; ctxt < ppd->dd->num_rcv_contexts;
	     ctxt++) {
		uctxt = ppd->dd->rcd[ctxt];
		if (!uctxt)
			continue;
		if (uctxt->user_event_mask) {
			int i;
			/*
			 * subctxt_cnt is 0 if not shared, so do base
			 * separately, first, then remaining subctxt, if any
			 */
			set_bit(evtbit, &uctxt->user_event_mask[0]);
			for (i = 1; i < uctxt->subctxt_cnt; i++)
				set_bit(evtbit, &uctxt->user_event_mask[i]);
		}
		ret = 1;
		break;
	}
	spin_unlock_irqrestore(&ppd->dd->uctxt_lock, flags);

	return ret;
}

/**
 * qib_manage_rcvq - manage a context's receive queue
 * @uctxt: the context
 * @subctxt: the subcontext
 * @start_stop: action to carry out
 *
 * start_stop == 0 disables receive on the context, for use in queue
 * overflow conditions.  start_stop==1 re-enables, to be used to
 * re-init the software copy of the head register
 */
static int manage_rcvq(struct qib_ctxtdata *uctxt, unsigned subctxt,
			   int start_stop)
{
	struct hfi_devdata *dd = uctxt->dd;
	unsigned int rcvctrl_op;

	if (subctxt)
		goto bail;
	/* atomically clear receive enable ctxt. */
	if (start_stop) {
		/*
		 * On enable, force in-memory copy of the tail register to
		 * 0, so that protocol code doesn't have to worry about
		 * whether or not the chip has yet updated the in-memory
		 * copy or not on return from the system call. The chip
		 * always resets it's tail register back to 0 on a
		 * transition from disabled to enabled.
		 */
		if (uctxt->rcvhdrtail_kvaddr)
			qib_clear_rcvhdrtail(uctxt);
		rcvctrl_op = QIB_RCVCTRL_CTXT_ENB;
	} else
		rcvctrl_op = QIB_RCVCTRL_CTXT_DIS;
	dd->f_rcvctrl(dd, rcvctrl_op, uctxt->ctxt);
	/* always; new head should be equal to new tail; see above */
bail:
	return 0;
}

/*
 * clear the event notifier events for this context.
 * User process then performs actions appropriate to bit having been
 * set, if desired, and checks again in future.
 */
static int user_event_ack(struct qib_ctxtdata *uctxt, int subctxt,
			      unsigned long events)
{
	int ret = 0, i;

	for (i = 0; i <= _QIB_MAX_EVENT_BIT; i++) {
		if (!test_bit(i, &events))
			continue;
		clear_bit(i, &uctxt->user_event_mask[subctxt]);
	}
	return ret;
}

#define num_user_pages(vaddr, len)					\
	(1 + (((((unsigned long)(vaddr) +				\
		 (unsigned long)(len) - 1) & PAGE_MASK) -		\
	       ((unsigned long)vaddr & PAGE_MASK)) >> PAGE_SHIFT))

/**
 * tzcnt - count the number of trailing zeros in a 64bit value
 * @value: the value to be examined
 *
 * Returns the number of trailing least significant zeros in the
 * the input value. If the value is zero, return the number of
 * bits of the value.
 */
static inline u8 tzcnt(u64 value)
{
	return value ? __builtin_ctzl(value) : sizeof(value) * 8;
}

static int exp_tid_setup(struct file *fp, struct hfi_tid_info *tinfo)
{
	int ret = 0;
	struct qib_ctxtdata *uctxt = ctxt_fp(fp);
	struct hfi_devdata *dd = uctxt->dd;
	unsigned tid, mapped, npages, ngroups, exp_groups,
		tidpairs = uctxt->expected_count / 2;
	unsigned long vaddr;
	struct page **pages;
	unsigned long tidmap[uctxt->tidmapcnt], map, flags;
	dma_addr_t *phys;
	u32 tidlist[tidpairs], pairidx = 0;
	u16 useidx, idx, bitidx, tidcnt = 0;

	vaddr = tinfo->vaddr;

	npages = num_user_pages(vaddr, tinfo->length);
	if (!npages) {
		ret = -EINVAL;
		goto done;
	}
	if (!access_ok(VERIFY_WRITE, (void __user *)vaddr,
		       npages * PAGE_SIZE)) {
		dd_dev_err(dd, "Fail vaddr %p, %u pages, !access_ok\n",
			   (void *)vaddr, npages);
		ret = -EFAULT;
		goto done;
	}
	memset(tidmap, 0, sizeof(tidmap));
	memset(tidlist, 0, sizeof(tidlist));

	exp_groups = uctxt->expected_count / dd->rcv_entries.group_size;
	/*
	 * From this point on, we need exclusive access to the context's
	 * Expected TID/RcvArray data. Since this is a per-context lock
	 * a maximum of QLOGIC_IB_MAX_SUBCTXT processes will be contending
	 * to access it at any given time.
	 */
	spin_lock_irqsave(&uctxt->exp_lock, flags);
	/* which group set do we look at first? */
	useidx = uctxt->tidcursor / BITS_PER_LONG;
	bitidx = uctxt->tidcursor % BITS_PER_LONG;
	/*
	 * Keep going until we've mapped all pages or we've exhausted all
	 * RcvArray entries.
	 */
	for (mapped = 0, idx = 0; mapped < npages && idx < uctxt->tidmapcnt; ) {
		u64 i, offset = 0;
		unsigned free, pinned, pmapped = 0, used_groups;
		u16 grp;

		if (uctxt->tidusemap[useidx] == -1ULL ||
		    bitidx >= BITS_PER_LONG) {
			/* no free groups in the set, use the next */
			useidx = (useidx + 1) % uctxt->tidmapcnt;
			idx++;
			bitidx = 0;
			continue;
		}
		/*
		 * If we've gotten here, the current set of groups does have
		 * one or more free groups.
		 */
		map = uctxt->tidusemap[useidx];
		/* "Turn off" any bits set before our bit index */
		map &= ~((1ULL << bitidx) - 1);
		free = tzcnt(map) - bitidx;
		while (!free) {
			/* Zero out the last set bit so we look at the rest */
			map &= ~(1ULL << bitidx);
			/*
			 * Account for the previously checked bits and advance
			 * the bit index. We don't have to check for bitidx
			 * getting bigger than BITS_PER_LONG here as it would
			 * mean extra instructions that we don't need. If it
			 * did happen, it would push free to a negative value
			 * which will break the loop.
			 */
			free = tzcnt(map) - ++bitidx;
		}
		/* If the entire map is used up, move to the next one. */
		if (bitidx > BITS_PER_LONG)
			continue;
		used_groups = ((useidx * BITS_PER_LONG) + bitidx);
		/*
		 * If the number of groups does not fit nicely into the number
		 * of bits in the bitmap, we could get wrong impression of the
		 * number of free groups. Check if this is the case and
		 * adjust the count accordingly.
		 */
		if (used_groups + free > exp_groups) {
			free = exp_groups - used_groups;
			/*
			 * We've adjusted to the "real" free. Do we have any?
			 * Here, we break out of the loop instead of moving on
			 * to the next bitmask because this could only happen
			 * at the end of the range. Therefore, there is no
			 * reason to continue.
			 */
			if (!free)
				break;
		}

		/*
		 * At this point, we know where in the map we have free bits.
		 * properly offset into the various "shadow" arrays and compute
		 * the RcvArray entry index.
		 */
		offset = used_groups * dd->rcv_entries.group_size;
		pages = uctxt->tid_pg_list + offset;
		phys = uctxt->physshadow + offset;
		tid = uctxt->expected_base + offset;

		/* Calculate how many pages we can pin based on free bits */
		pinned = min((free * dd->rcv_entries.group_size),
			     (npages - mapped));
		/*
		 * Now that we know how many free RcvArray entries we have,
		 * we can pin that many user pages.
		 */
		ret = qib_get_user_pages(vaddr + (mapped * PAGE_SIZE),
					 pinned, pages);
		if (ret) {
			/*
			 * We can't continue because the pages array won't be
			 * initialized. This should never happen,
			 * unless perhaps the user has mpin'ed the pages
			 * themselves.
			 */
			dd_dev_info(dd,
				    "Failed to lock addr %p, %u pages: errno %d\n",
				    (void *) vaddr, pinned, -ret);
			/* XXX Mitko: What do we do with any RcvArray entries
			 * and pages already programmed. */
			spin_unlock_irqrestore(&uctxt->exp_lock, flags);
			goto done;
		}
		/*
		 * How many groups do we need based on how many pages we have
		 * pinned?
		 */
		ngroups = (pinned / dd->rcv_entries.group_size) +
			!!(pinned % dd->rcv_entries.group_size);
		/*
		 * Keep programming RcvArray entries for all the <ngroups> free
		 * groups.
		 */
		for (i = 0, grp = 0; grp < ngroups; i++, grp++) {
			unsigned grpidx = bitidx + i, j;
			u32 pair_size = 0, tidsize;
			/*
			 * This inner loop will program an entire group or the
			 * array of pinned pages (which ever limit is hit
			 * first).
			 */
			for (j = 0; j < dd->rcv_entries.group_size &&
				     pmapped < pinned; j++, pmapped++, tid++) {
				tidsize = PAGE_SIZE;
				phys[pmapped] = qib_map_page(dd->pcidev,
						   pages[pmapped], 0,
						   tidsize, PCI_DMA_FROMDEVICE);
				trace_hfi_exp_rcv_set(uctxt->ctxt,
						      subctxt_fp(fp),
						      tid, vaddr, phys[pmapped],
						      pages[pmapped]);
				/*
				 * Each RcvArray entry is programmed with one page
				 * worth of memory. This will handle the 8K MTU
				 * as well as anything smaller due to the fact that
				 * both entries in the RcvTidPair are programmed
				 * with a page.
				 * PSM currently does not handle anything bigger
				 * than 8K MTU, so should we even worry about 10K
				 * here?
				 */
				/* XXX MITKO: better determine the order of the
				 * TID */
				dd->f_put_tid(dd, tid, PT_EXPECTED,
					      phys[pmapped],
					      tidsize >> PAGE_SHIFT);
				pair_size += tidsize >> PAGE_SHIFT;
				EXP_TID_RESET(tidlist[pairidx], LEN, pair_size);
				if (!(tid % 2)) {
					tidlist[pairidx] |=
					   EXP_TID_SET(IDX,
						(tid - uctxt->expected_base)
						       / 2);
					tidlist[pairidx] |=
						EXP_TID_SET(CTRL, 1);
					tidcnt++;
				} else {
					tidlist[pairidx] |=
						EXP_TID_SET(CTRL, 2);
					pair_size = 0;
					pairidx++;
				}
			}
			set_bit(grpidx, &tidmap[useidx]);
			/*
			 * We've programmed the entire group (or as much of the
			 * group as we'll use. Now, it's time to push it out...
			 */
			qib_flush_wc();
		}
		mapped += pinned;
		uctxt->tidcursor = (uctxt->tidcursor + i) % uctxt->numtidgroups;
		uctxt->tidusemap[useidx] |= tidmap[useidx];

	}
	spin_unlock_irqrestore(&uctxt->exp_lock, flags);
	/*
	 * Calculate mapped length. New Exp TID protocol does not "unwind" and
	 * report an error if it can't map the entire buffer. It just reports
	 * the length that was mapped.
	 */
	tinfo->length = mapped * PAGE_SIZE;
	tinfo->tidcnt = tidcnt;
	if (copy_to_user((void __user *)(unsigned long)tinfo->tidlist,
			 tidlist, sizeof(tidlist[0]) * tidcnt)) {
		ret = -EFAULT;
		goto done;
	}
	/* copy TID info to user */
	if (copy_to_user((void __user *)(unsigned long)tinfo->tidmap,
			 tidmap, sizeof(tidmap)))
		ret = -EFAULT;
done:
	return ret;
}

static int exp_tid_free(struct file *fp, struct hfi_tid_info *tinfo)
{
	struct qib_ctxtdata *uctxt = ctxt_fp(fp);
	struct hfi_devdata *dd = uctxt->dd;
	unsigned long tidmap[uctxt->tidmapcnt], flags;
	struct page **pages;
	dma_addr_t *phys;
	u16 idx, bitidx, tid;
	int ret = 0;

	if (copy_from_user(&tidmap, (void __user *)(unsigned long)
			   tinfo->tidmap, sizeof(tidmap))) {
		ret = -EFAULT;
		goto done;
	}
	for (idx = 0; idx < uctxt->tidmapcnt; idx++) {
		unsigned long map;
		bitidx = 0;
		if (!tidmap[idx])
			continue;
		map = tidmap[idx];
		while ((bitidx = tzcnt(map)) < BITS_PER_LONG) {
			int i;
			unsigned offset = ((idx * BITS_PER_LONG) + bitidx);
			pages = uctxt->tid_pg_list +
				(offset * dd->rcv_entries.group_size);
			phys = uctxt->physshadow +
				(offset * dd->rcv_entries.group_size);
			tid = uctxt->expected_base +
				(offset * dd->rcv_entries.group_size);
			for (i = 0; i < dd->rcv_entries.group_size;
			     i++, tid++) {
				if (pages[i]) {
					dd->f_put_tid(dd, tid, PT_INVALID,
						      0, 0);
					trace_hfi_exp_rcv_free(uctxt->ctxt,
							       subctxt_fp(fp),
							       tid, phys[i],
							       pages[i]);
					pci_unmap_page(dd->pcidev, phys[i],
					      PAGE_SIZE, PCI_DMA_FROMDEVICE);
					qib_release_user_pages(&pages[i], 1);
					pages[i] = NULL;
					phys[i] = 0;
				}
			}
			spin_lock_irqsave(&uctxt->exp_lock, flags);
			clear_bit(bitidx, &uctxt->tidusemap[idx]);
			spin_unlock_irqrestore(&uctxt->exp_lock, flags);
			map &= ~(1ULL<<bitidx);
		};
	}
done:
	return ret;
}

static void unlock_exp_tids(struct qib_ctxtdata *uctxt)
{
	struct hfi_devdata *dd = uctxt->dd;
	unsigned tid;

	dd_dev_info(dd, "ctxt %u unlocking any locked expTID pages\n",
		    uctxt->ctxt);
	for (tid = 0; tid < uctxt->expected_count; tid++) {
		struct page *p = uctxt->tid_pg_list[tid];
		dma_addr_t phys;

		if (!p)
			continue;

		phys = uctxt->physshadow[tid];
		uctxt->physshadow[tid] = 0;
		uctxt->tid_pg_list[tid] = NULL;
		pci_unmap_page(dd->pcidev, phys, PAGE_SIZE, PCI_DMA_FROMDEVICE);
		qib_release_user_pages(&p, 1);
	}
}

static int set_ctxt_pkey(struct qib_ctxtdata *uctxt, unsigned subctxt, u16 pkey)
{
	int ret = -ENOENT, i, intable = 0;
	struct qib_pportdata *ppd = uctxt->ppd;
	struct hfi_devdata *dd = uctxt->dd;

	if (pkey == WFR_LIM_MGMT_P_KEY || pkey == WFR_FULL_MGMT_P_KEY) {
		ret = -EINVAL;
		goto done;
	}

	for (i = 0; i < ARRAY_SIZE(ppd->pkeys); i++)
		if (pkey == ppd->pkeys[i]) {
			intable = 1;
			break;
		}

	if (intable)
		ret = dd->f_set_ctxt_pkey(dd, uctxt->ctxt, pkey);
done:
	return ret;
}

static int ui_open(struct inode *inode, struct file *filp)
{
	struct hfi_devdata *dd;

	dd = container_of(inode->i_cdev, struct hfi_devdata, ui_cdev);
	filp->private_data = dd; /* for other methods */
	return 0;
}

static int ui_release(struct inode *inode, struct file *filp)
{
	/* nothing to do */
	return 0;
}

static loff_t ui_lseek(struct file *filp, loff_t offset, int whence)
{
	struct hfi_devdata *dd = filp->private_data;

	switch (whence) {
	case SEEK_SET:
		break;
	case SEEK_CUR:
		offset += filp->f_pos;
		break;
	case SEEK_END:
		offset = (dd->kregend - dd->kregbase) - offset;
		break;
	default:
		return -EINVAL;
	}

	if (offset < 0)
		return -EINVAL;

	if (offset >= dd->kregend - dd->kregbase)
		return -EINVAL;

	filp->f_pos = offset;

	return filp->f_pos;
}

/* NOTE: assumes unsigned long is 8 bytes */
static ssize_t ui_read(struct file *filp, char __user *buf, size_t count,
			loff_t *f_pos)
{
	struct hfi_devdata *dd = filp->private_data;
	void *base;
	unsigned long total, data;

	/* only read 8 byte quantities */
	if ((count % 8) != 0)
		return -EINVAL;
	/* offset must be 8-byte aligned */
	if ((*f_pos % 8) != 0)
		return -EINVAL;
	/* destination buffer must be 8-byte aligned */
	if ((unsigned long)buf % 8 != 0)
		return -EINVAL;
	/* must be in range */
	if (*f_pos + count > dd->kregend - dd->kregbase)
		return -EINVAL;
	base = (void *)dd->kregbase + *f_pos;
	for (total = 0; total < count; total += 8) {
		data = readq(base + total);
		if (put_user(data, (unsigned long *)(buf + total)))
			break;
	}
	*f_pos += total;
	return total;
}

/* NOTE: assumes unsigned long is 8 bytes */
static ssize_t ui_write(struct file *filp, const char __user *buf,
			size_t count, loff_t *f_pos)
{
	struct hfi_devdata *dd = filp->private_data;
	void *base;
	unsigned long total, data;

	/* only write 8 byte quantities */
	if ((count % 8) != 0)
		return -EINVAL;
	/* offset must be 8-byte aligned */
	if ((*f_pos % 8) != 0)
		return -EINVAL;
	/* source buffer must be 8-byte aligned */
	if ((unsigned long)buf % 8 != 0)
		return -EINVAL;
	/* must be in range */
	if (*f_pos + count > dd->kregend - dd->kregbase)
		return -EINVAL;

	base = (void *)dd->kregbase + *f_pos;
	for (total = 0; total < count; total += 8) {
		if (get_user(data, (unsigned long *)(buf + total)))
			break;
		writeq(data, base + total);
	}
	*f_pos += total;
	return total;
}

static const struct file_operations ui_file_ops = {
	.owner = THIS_MODULE,
	.llseek = ui_lseek,
	.read = ui_read,
	.write = ui_write,
	.open = ui_open,
	.release = ui_release,
};
#define UI_OFFSET 192	/* device minor offset for UI devices */
int create_ui = 1;

static struct cdev wildcard_cdev;
static struct device *wildcard_device;

static atomic_t user_count = ATOMIC_INIT(0);

static void qib_user_remove(struct hfi_devdata *dd)
{
	if (atomic_dec_return(&user_count) == 0)
		hfi_cdev_cleanup(&wildcard_cdev, &wildcard_device);

	hfi_cdev_cleanup(&dd->user_cdev, &dd->user_device);
	hfi_cdev_cleanup(&dd->ui_cdev, &dd->ui_device);
}

static int qib_user_add(struct hfi_devdata *dd)
{
	char name[10];
	int ret;

	if (atomic_inc_return(&user_count) == 1) {
		ret = hfi_cdev_init(0, class_name(), &qib_file_ops,
				    &wildcard_cdev, &wildcard_device);
		if (ret)
			goto done;
	}

	snprintf(name, sizeof(name), "%s%d", class_name(), dd->unit);
	ret = hfi_cdev_init(dd->unit + 1, name, &qib_file_ops,
			    &dd->user_cdev, &dd->user_device);
	if (ret)
		goto done;

	if (create_ui) {
		snprintf(name, sizeof(name),
			 "%s%d_ui", class_name(), dd->unit);
		ret = hfi_cdev_init(dd->unit + UI_OFFSET, name, &ui_file_ops,
				    &dd->ui_cdev, &dd->ui_device);
		if (ret)
			goto done;
	}

	return 0;
done:
	qib_user_remove(dd);
	return ret;
}

/*
 * Create per-unit files in /dev
 */
int hfi_device_create(struct hfi_devdata *dd)
{
	int r, ret;

	r = qib_user_add(dd);
	ret = qib_diag_add(dd);
	if (r && !ret)
		ret = r;
	return ret;
}

/*
 * Remove per-unit files in /dev
 * void, core kernel returns no errors for this stuff
 */
void hfi_device_remove(struct hfi_devdata *dd)
{
	qib_user_remove(dd);
	qib_diag_remove(dd);
}
