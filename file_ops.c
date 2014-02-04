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

#include "hfi.h"
#include "pio.h"
#include "device.h"
#include "common.h"
#include "trace.h"

#undef pr_fmt
#define pr_fmt(fmt) DRIVER_NAME ": " fmt

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
static int user_init(struct file *);
static int get_ctxt_info(struct file *, void __user *, __u32);
static int get_base_info(struct file *, void __user *, __u32);
static int setup_ctxt(struct file *, struct hfi_ctxt_setup *);
static int find_free_ctxt(unsigned, struct file *, struct hfi_user_info *);
static int get_a_ctxt(struct file *, struct hfi_user_info *, unsigned);
static int allocate_ctxt(struct file *, struct hfi_devdata *,
			 struct hfi_user_info *);
static unsigned int poll_urgent(struct file *, struct poll_table_struct *);
static unsigned int poll_next(struct file *, struct poll_table_struct *);
static int user_event_ack(struct qib_ctxtdata *, int, unsigned long);
static int manage_rcvq(struct qib_ctxtdata *, unsigned, int);

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
		copy = 0;
		user_val = ucmd->addr;
		break;
	default:
		ret = -EINVAL;
		goto bail;
	}

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
	case HFI_CMD_TID_FREE:
		printk(KERN_WARNING " %s: TID update/free unimplemented\n",
		       __func__);
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
	unsigned long flags, pfn, memaddr = 0;
	u64 token = vma->vm_pgoff << PAGE_SHIFT;
	u8 subctxt, mapio = 0, type;
	ssize_t memlen = 0;
	int ret = 0;
	u16 ctxt, numa;

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
		memaddr = ((dd->physaddr + WFR_TXE_PIO_SEND) +
			   (uctxt->ctxt * (1 << 16))) +
			(type == PIO_BUFS_SOP ? (WFR_TXE_PIO_SIZE / 2) : 0);
		/*
		 * Map only the amount allocated to the context, not the
		 * entire available context's PIO space.
		 */
		memlen = ALIGN(uctxt->sc->credits * WFR_PIO_BLOCK_SIZE,
			       PAGE_SIZE);
		mapio = 1;
		flags &= ~VM_MAYREAD;
		flags |= VM_DONTCOPY | VM_DONTEXPAND;
		// XXX (Mitko): how do we deal with write-combining?
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
		break;
	case PIO_CRED:
		numa = numa_node_id();
		/*
		 * The credit return location for this context could be on the
		 * second of two pages allocated for credit returns (if number
		 * of enabled contexts > 128).
		 */
		memaddr = dd->cr_base[numa].pa +
			((uctxt->ctxt * sizeof(*dd->cr_base[numa].va)) > PAGE_SIZE ?
			 PAGE_SIZE : 0);
		memlen = PAGE_SIZE;
		mapio = 1;
		if (flags & VM_WRITE) {
			ret = -EINVAL;
			goto done;
		}
		flags &= ~VM_MAYWRITE;
		flags |= VM_DONTCOPY | VM_DONTEXPAND;
		/*
		 * The driver has already allocated memory for credit
		 * returns and programmed it into the chip. Has that
		 * memory been flaged as non-cached?
		 */
		//vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
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
		if (((vma->vm_end - vma->vm_start) < memlen) ||
		    vma->vm_flags & VM_WRITE) {
			ret = -EINVAL;
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
		memlen = PAGE_SIZE;
		mapio = 1;
		flags |= VM_DONTCOPY | VM_DONTEXPAND;
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
		break;
	case EVENTS:
		/*
		 * One page containing events for all ctxts. User level
		 * knows where it's own bitmap is.
		 */
		memaddr = kvirt_to_phys(dd->events);
		memlen = PAGE_SIZE;
		/*
		 * v3.7 removes VM_RESERVED but the effect is kept by
		 * using VM_IO.
		 */
		flags |= VM_IO | VM_DONTEXPAND;
		break;
	case STATUS:
		memaddr = kvirt_to_phys((void *)dd->status);
		memlen = PAGE_SIZE;
		flags |= VM_IO | VM_DONTEXPAND;
		break;
	case RTAIL:
		memaddr = uctxt->rcvhdrqtailaddr_phys;
		memlen = PAGE_SIZE;
		if (flags & VM_WRITE) {
			ret = -EINVAL;
			goto done;
		}
		flags &= ~VM_MAYWRITE;
		break;
	case SUBCTXT_UREGS:
	case SUBCTXT_RCV_HDRQ:
	case SDMA_COMP:
	default:
		ret = -EINVAL;
		break;
	}

	if ((vma->vm_end - vma->vm_start) < memlen) {
		ret = -EINVAL;
		goto done;
	}

	vma->vm_flags = flags;
	dd_dev_info(dd, "%s: type:%u io:%d, addr:0x%lx, len:%lu, flags:0x%lx\n",
		    __func__, type, mapio, memaddr, memlen, vma->vm_flags);
	pfn = memaddr >> PAGE_SHIFT;
	if (mapio) {
		ret = io_remap_pfn_range(vma, vma->vm_start, pfn, memlen,
					 vma->vm_page_prot);
	} else {
		ret = remap_pfn_range(vma, vma->vm_start, pfn, memlen,
				      vma->vm_page_prot);
	}
done:
	return ret;
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

	printk(KERN_INFO " freeing ctxt %u\n", uctxt->ctxt);
	dd = uctxt->dd;
	mutex_lock(&qib_mutex);

	qib_flush_wc();
	/* drain user sdma queue */
	/*if (fdata->pq) {
		qib_user_sdma_queue_drain(uctxt->ppd, fdata->pq);
		qib_user_sdma_queue_destroy(fdata->pq);
		}*/

	spin_lock_irqsave(&dd->uctxt_lock, flags);
	/* disable receive context and interrupt available */
	dd->f_rcvctrl(dd, QIB_RCVCTRL_CTXT_DIS | QIB_RCVCTRL_TIDFLOW_DIS |
		      QIB_RCVCTRL_INTRAVAIL_DIS, uctxt->ctxt);
	sc_disable(uctxt->sc);
	uctxt->pid = 0;
	spin_unlock_irqrestore(&dd->uctxt_lock, flags);

	dd->rcd[uctxt->ctxt] = NULL;
	uctxt->rcvwait_to = 0;
        uctxt->piowait_to = 0;
        uctxt->rcvnowait = 0;
        uctxt->pionowait = 0;
        uctxt->flag = 0;

        dd->f_clear_tids(uctxt);

	qib_stats.sps_ctxts--;
	dd->freectxts++;
	mutex_unlock(&qib_mutex);
	qib_free_ctxtdata(dd, uctxt);
done:
	kfree(fdata);
	return 0;
}

/*
 * Convert kernel virtual addresses to physical addresses so they don't
 * potentially conflict with the chip addresses used as mmap offsets.
 * It doesn't really matter what mmap offset we use as long as we can
 * interpret it correctly.
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
	int i_minor, ret;
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
	i_minor = iminor(file_inode(fp)) - HFI_USER_MINOR_BASE;
	if (i_minor)
		ret = find_free_ctxt(i_minor - 1, fp, uinfo);
	else
		ret = get_a_ctxt(fp, uinfo, alg);
	mutex_unlock(&qib_mutex);
done:
	return ret;
}

static int find_free_ctxt(unsigned devno, struct file *fp,
			  struct hfi_user_info *uinfo)
{
	struct hfi_devdata *dd = qib_lookup(devno);
	int ret = 0;
	if (!dd)
		ret = -ENODEV;
	if (!ret && !dd->freectxts)
		ret = -EBUSY;
	return ret ? ret : allocate_ctxt(fp, dd, uinfo);
}

static int get_a_ctxt(struct file *fp, struct hfi_user_info *uinfo,
		      unsigned alg)
{
	struct hfi_devdata *udd = NULL;
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
	if (alg == HFI_ALG_ACROSS) {
		unsigned free = 0U;
		for (dev = 0; dev < devmax; dev++) {
			struct hfi_devdata *dd = qib_lookup(dev);
			if (!dd)
				continue;
			if (!dd->freectxts)
				continue;
			if (dd->freectxts > free) {
				udd = dd;
				free = dd->freectxts;
			}
		}
		if (udd)
			ret = allocate_ctxt(fp, udd, uinfo);
		else
			ret = -EBUSY;
		goto done;

	} else {
		for (dev = 0; dev < devmax; dev++) {
			udd = qib_lookup(dev);
			if (udd) {
				ret = allocate_ctxt(fp, udd, uinfo);
				if (!ret)
					goto done;
			}
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
	void *ptmp = NULL;
	int ret = 0;

	for (ctxt = dd->first_user_ctxt;
	     ctxt < dd->num_rcv_contexts && dd->rcd[ctxt]; ctxt++)
	if (ctxt == dd->num_rcv_contexts) {
		ret = -EBUSY;
		goto done;
	}

	uctxt = qib_create_ctxtdata(dd->pport, ctxt);
	if (uctxt)
		ptmp = kmalloc(uctxt->expected_count * sizeof(struct page **) +
			       uctxt->expected_count * sizeof(u16), GFP_KERNEL);
	if (!uctxt || !ptmp) {
		dd_dev_err(dd,
			   "Unable to allocate ctxtdata memory, failing open\n");
		ret = -ENOMEM;
		goto done;
	}
	/*
	 * Allocate and enable a PIO send context.
	 */
	//FIXME: Add the numa the sc_alloc call
	uctxt->sc = sc_alloc(dd, SC_USER, 0 /*numa*/);
	if (!uctxt->sc) {
		ret = -ENOMEM;
		goto done;
	}
	printk(KERN_INFO " allocated send context %d\n", uctxt->sc->context);
	sc_enable(uctxt->sc);

	uctxt->userversion = uinfo->userversion;
	/* subctxts not supported yet
	ret = init_subctxts(dd, uctxt, uinfo);
	if (ret)
	goto bail;
	*/
	uctxt->tid_pg_list = ptmp;
	uctxt->pid = current->pid;
	init_waitqueue_head(&uctxt->wait);
	strlcpy(uctxt->comm, current->comm, sizeof(uctxt->comm));
	qib_stats.sps_ctxts++;
	dd->freectxts--;
	ctxt_fp(fp) = uctxt;
	ret = 0;
done:
	return ret;
}

static int user_init(struct file *fp)
{
	int ret;
	unsigned int rcvctrl_ops = 0;
	struct qib_ctxtdata *uctxt = ctxt_fp(fp);
	//struct hfi_devdata *dd;

	/* make sure that the context has already been setup */
	if (!test_bit(QIB_CTXT_SETUP_DONE, &uctxt->flag)) {
		ret = -EFAULT;
		goto done;
	}

	/* Subctxts don't need to initialize anything since master did it.
	   Subctxt are not supported yet
	if (subctxt_fp(fp)) {
		ret = wait_event_interruptible(uctxt->wait,
			!test_bit(QIB_CTXT_MASTER_UNINIT, &uctxt->flag));
		goto bail;
	}
	*/

	tidcursor_fp(fp) = 0; /* start at beginning after open */

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

	rcvctrl_ops = QIB_RCVCTRL_CTXT_ENB | QIB_RCVCTRL_TIDFLOW_ENB;
	if (!(uctxt->dd->flags & QIB_NODMA_RTAIL))
		rcvctrl_ops |= QIB_RCVCTRL_TAILUPD_ENB;
	uctxt->dd->f_rcvctrl(uctxt->dd, rcvctrl_ops, uctxt->ctxt);

	/* Notify any waiting slaves */
	if (uctxt->subctxt_cnt) {
		clear_bit(QIB_CTXT_MASTER_UNINIT, &uctxt->flag);
		wake_up(&uctxt->wait);
	}
	return 0;

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
	cinfo.rcvtids = uctxt->dd->rcv_entries;
	cinfo.credits = uctxt->sc->credits;
	/* FIXME: set proper numa node */
	cinfo.numa_node = 0;
	cinfo.rec_cpu = fd->rec_cpu_num;

	if (copy_to_user(ubase, &cinfo, sizeof(cinfo)))
		ret = -EFAULT;
	return ret;
}

static int setup_ctxt(struct file *fp, struct hfi_ctxt_setup *cinfo)
{
	struct qib_ctxtdata *uctxt = ctxt_fp(fp);
	int ret = 0;

	trace_hfi_ctxt_setup(uctxt->dd, uctxt->ctxt, subctxt_fp(fp),
			     cinfo);

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

	// rcv eager array is eagrcnt * rcvegrentsize (rcvegr_entsize)
	// the rcvhdrq array is rcvhdr_entries * rcvhdr_entsize (64)
	
	/*
	 * Now allocate the rcvhdr Q and eager TIDs; skip the TID
	 * array for time being.  If uctxt->ctxt > chip-supported,
	 * we need to do extra stuff here to handle by handling overflow
	 * through ctxt 0, someday
	 */
	ret = qib_create_rcvhdrq(uctxt->dd, uctxt);
	if (!ret)
		ret = qib_setup_eagerbufs(uctxt);
	if (ret)
		goto done;

	set_bit(QIB_CTXT_SETUP_DONE, &uctxt->flag);
done:
	return ret;
}

static int get_base_info(struct file *fp, void __user *ubase, __u32 len)
{
	struct hfi_base_info binfo;
	struct qib_ctxtdata *uctxt = ctxt_fp(fp);
	struct hfi_devdata *dd = uctxt->dd;
	ssize_t sz;
	unsigned offset, numa;
	int ret = 0;

	trace_hfi_uctxtdata(uctxt->dd, uctxt);

	memset(&binfo, 0, sizeof(binfo));
	ret = dd->f_get_base_info(uctxt, &binfo);
	if (ret < 0)
		goto done;
	binfo.hw_version = dd->revision;
	binfo.sw_version = QIB_KERN_SWVERSION;
	binfo.context = uctxt->ctxt;
	binfo.bthqp = kdeth_qp;
	/* XXX (Mitko): We are hard-coding the pport index since WFR
	 * has only one port and the expectation is that ppd will go
	 * away. */
	binfo.mtu = dd->pport[0].ibmtu;

	numa = numa_node_id();
	/*
	 * If more than 128 contexts are enabled the allocated credit
	 * return will span two contiguous pages. Since we only map
	 * the page containing the context's credit return address,
	 * we need to calculate the offset in the proper page.
	 */
	offset = (u64)uctxt->sc->hw_free - (u64)dd->cr_base[numa].va;
	offset -= (offset > PAGE_SIZE) ? PAGE_SIZE : 0;
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
		set_bit(QIB_CTXT_WAITING_URG, &uctxt->flag);
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
		set_bit(QIB_CTXT_WAITING_RCV, &uctxt->flag);
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
 * @rcd: the context
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
	int r = 0, ret;

	ret = qib_user_add(dd);
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
}
