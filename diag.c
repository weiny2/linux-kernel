/*
 * Copyright (c) 2012 Intel Corporation. All rights reserved.
 * Copyright (c) 2006 - 2012 QLogic Corporation. All rights reserved.
 * Copyright (c) 2003, 2004, 2005, 2006 PathScale, Inc. All rights reserved.
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

/*
 * This file contains support for diagnostic functions.  It is accessed by
 * opening the hfi_diag device, normally minor number 129.  Diagnostic use
 * of the chip may render the chip or board unusable until the driver
 * is unloaded, or in some cases, until the system is rebooted.
 *
 * Accesses to the chip through this interface are not similar to going
 * through the /sys/bus/pci resource mmap interface.
 */

#include <linux/io.h>
#include <linux/pci.h>
#include <linux/poll.h>
#include <linux/vmalloc.h>
#include <linux/export.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "hfi.h"
#include "common.h"

#undef pr_fmt
#define pr_fmt(fmt) DRIVER_NAME ": " fmt

/*
 * Each client that opens the diag device must read then write
 * offset 0, to prevent lossage from random cat or od. diag_state
 * sequences this "handshake".
 */
enum diag_state { UNUSED = 0, OPENED, INIT, READY };

/* State for an individual client. PID so children cannot abuse handshake */
static struct diag_client {
	struct diag_client *next;
	struct hfi_devdata *dd;
	pid_t pid;
	enum diag_state state;
} *client_pool;

/*
 * Get a client struct. Recycled if possible, else kmalloc.
 * Must be called with qib_mutex held
 */
static struct diag_client *get_client(struct hfi_devdata *dd)
{
	struct diag_client *dc;

	dc = client_pool;
	if (dc)
		/* got from pool remove it and use */
		client_pool = dc->next;
	else
		/* None in pool, alloc and init */
		dc = kmalloc(sizeof *dc, GFP_KERNEL);

	if (dc) {
		dc->next = NULL;
		dc->dd = dd;
		dc->pid = current->pid;
		dc->state = OPENED;
	}
	return dc;
}

/*
 * Return to pool. Must be called with qib_mutex held
 */
static void return_client(struct diag_client *dc)
{
	struct hfi_devdata *dd = dc->dd;
	struct diag_client *tdc, *rdc;

	rdc = NULL;
	if (dc == dd->diag_client) {
		dd->diag_client = dc->next;
		rdc = dc;
	} else {
		tdc = dc->dd->diag_client;
		while (tdc) {
			if (dc == tdc->next) {
				tdc->next = dc->next;
				rdc = dc;
				break;
			}
			tdc = tdc->next;
		}
	}
	if (rdc) {
		rdc->state = UNUSED;
		rdc->dd = NULL;
		rdc->pid = 0;
		rdc->next = client_pool;
		client_pool = rdc;
	}
}

static int diag_open(struct inode *in, struct file *fp);
static int diag_release(struct inode *in, struct file *fp);
static ssize_t diag_read(struct file *fp, char __user *data,
			     size_t count, loff_t *off);
static ssize_t diag_write(struct file *fp, const char __user *data,
			      size_t count, loff_t *off);

static const struct file_operations diag_file_ops = {
	.owner = THIS_MODULE,
	.write = diag_write,
	.read = diag_read,
	.open = diag_open,
	.release = diag_release,
	.llseek = default_llseek,
};

static atomic_t diagpkt_count = ATOMIC_INIT(0);
static struct cdev diagpkt_cdev;
static struct device *diagpkt_device;

static ssize_t diagpkt_write(struct file *fp, const char __user *data,
				 size_t count, loff_t *off);

static const struct file_operations diagpkt_file_ops = {
	.owner = THIS_MODULE,
	.write = diagpkt_write,
	.llseek = noop_llseek,
};

int qib_diag_add(struct hfi_devdata *dd)
{
	char name[16];
	int ret = 0;

	if (atomic_inc_return(&diagpkt_count) == 1) {
		ret = qib_cdev_init(QIB_DIAGPKT_MINOR, "hfi_diagpkt",
				    &diagpkt_file_ops, &diagpkt_cdev,
				    &diagpkt_device);
		if (ret)
			goto done;
	}

	snprintf(name, sizeof(name), "hfi_diag%d", dd->unit);
	ret = qib_cdev_init(QIB_DIAG_MINOR_BASE + dd->unit, name,
			    &diag_file_ops, &dd->diag_cdev,
			    &dd->diag_device);
done:
	return ret;
}

static void unregister_observers(struct hfi_devdata *dd);

void qib_diag_remove(struct hfi_devdata *dd)
{
	struct diag_client *dc;

	if (atomic_dec_and_test(&diagpkt_count))
		qib_cdev_cleanup(&diagpkt_cdev, &diagpkt_device);

	qib_cdev_cleanup(&dd->diag_cdev, &dd->diag_device);

	/*
	 * Return all diag_clients of this device. There should be none,
	 * as we are "guaranteed" that no clients are still open
	 */
	while (dd->diag_client)
		return_client(dd->diag_client);

	/* Now clean up all unused client structs */
	while (client_pool) {
		dc = client_pool;
		client_pool = dc->next;
		kfree(dc);
	}
	/* Clean up observer list */
	unregister_observers(dd);
}

/*
 * get_ioaddr - find the IO address given the chip offset
 *
 * @dd: the device data
 * @offset: the offset in chip-space
 * @cntp: pointer to max byte count for transfer starting at offset
 *
 * This routine is needed because the logically contiguous address-space
 * of the chip may be split into virtually non-contiguous spaces.
 * See pcie_ddinit() for details.
 *
 * Returns 0 if the offset is not mapped.
 */
static u64 __iomem *get_ioaddr(struct hfi_devdata *dd, u32 offset, u32 *cntp)
{
	u8 __iomem *map;
	u32 cnt;

	if (offset < WFR_TXE_PIO_SEND) {
		/* offset is within the CSR map */
		map = dd->kregbase + offset;
		cnt = WFR_TXE_PIO_SEND - offset;
	} else if (offset < (WFR_TXE_PIO_SEND + WFR_TXE_PIO_SIZE)) {
		/* offset is within the PIO map */
		offset -= WFR_TXE_PIO_SEND;
		map = dd->piobase + offset;
		cnt = WFR_TXE_PIO_SIZE - offset;
	} else {
		/* offset is outside anything */
		map = NULL;
		cnt = 0;
	}

	*cntp = cnt;
	return (u64 __iomem *)map;
}

/*
 * read_umem64 - read a 64-bit quantity from the chip into user space
 * @dd: the device
 * @uaddr: the location to store the data in user memory
 * @regoffs: the offset from BAR0
 * @count: number of bytes to copy (multiple of 64 bits)
 *
 * This function also localizes all chip memory accesses.
 * The copy should be written such that we read full cacheline packets
 * from the chip.  This is usually used for a single qword
 *
 * NOTE:  This assumes the chip address is 64-bit aligned.
 */
static int read_umem64(struct hfi_devdata *dd, void __user *uaddr,
			   u32 regoffs, size_t count)
{
	const u64 __iomem *reg_addr;
	const u64 __iomem *reg_end;
	u32 limit;
	int ret;

	reg_addr = get_ioaddr(dd, regoffs, &limit);
	if (reg_addr == NULL || limit == 0 || !(dd->flags & QIB_PRESENT)) {
		ret = -EINVAL;
		goto bail;
	}
	if (count >= limit)
		count = limit;
	reg_end = reg_addr + (count / sizeof(u64));

	/* not very efficient, but it works for now */
	while (reg_addr < reg_end) {
		u64 data = readq(reg_addr);

		if (copy_to_user(uaddr, &data, sizeof(u64))) {
			ret = -EFAULT;
			goto bail;
		}
		reg_addr++;
		uaddr += sizeof(u64);
	}
	ret = 0;
bail:
	return ret;
}

/*
 * write_umem64 - write a 64-bit quantity to the chip from user space
 * @dd: the device
 * @regoffs: the offset from BAR0
 * @uaddr: the source of the data in user memory
 * @count: the number of bytes to copy (multiple of 64 bits)
 *
 * This is usually used for a single qword
 * NOTE:  This assumes the chip address is 64-bit aligned.
 */
static int write_umem64(struct hfi_devdata *dd, u32 regoffs,
			    const void __user *uaddr, size_t count)
{
	u64 __iomem *reg_addr;
	u64 __iomem *reg_end;
	u32 limit;
	int ret;

	reg_addr = get_ioaddr(dd, regoffs, &limit);
	if (reg_addr == NULL || limit == 0 || !(dd->flags & QIB_PRESENT)) {
		ret = -EINVAL;
		goto bail;
	}
	if (count >= limit)
		count = limit;
	reg_end = reg_addr + (count / sizeof(u64));

	/* not very efficient, but it works for now */
	while (reg_addr < reg_end) {
		u64 data;
		if (copy_from_user(&data, uaddr, sizeof(data))) {
			ret = -EFAULT;
			goto bail;
		}
		writeq(data, reg_addr);

		reg_addr++;
		uaddr += sizeof(u64);
	}
	ret = 0;
bail:
	return ret;
}

static int diag_open(struct inode *in, struct file *fp)
{
	int unit = iminor(in) - QIB_DIAG_MINOR_BASE;
	struct hfi_devdata *dd;
	struct diag_client *dc;
	int ret;

	mutex_lock(&qib_mutex);

	dd = qib_lookup(unit);

	if (dd == NULL || !(dd->flags & QIB_PRESENT) ||
	    !dd->kregbase) {
		ret = -ENODEV;
		goto bail;
	}

	dc = get_client(dd);
	if (!dc) {
		ret = -ENOMEM;
		goto bail;
	}
	dc->next = dd->diag_client;
	dd->diag_client = dc;
	fp->private_data = dc;
	ret = 0;
bail:
	mutex_unlock(&qib_mutex);

	return ret;
}

/**
 * diagpkt_write - write an IB packet
 * @fp: the diag data device file pointer
 * @data: diag_pkt structure saying where to get the packet
 * @count: size of data to write
 * @off: unused by this code
 */
static ssize_t diagpkt_write(struct file *fp, const char __user *data,
				 size_t count, loff_t *off)
{
	struct hfi_devdata *dd;
	struct send_context *sc;
	struct pio_buf *pbuf;
	struct diag_pkt dp;
	u32 *tmpbuf = NULL;
	ssize_t ret = 0;
	u32 pkt_len, total_len;

	if (count != sizeof(dp)) {
		ret = -EINVAL;
		goto bail;
	}
	if (copy_from_user(&dp, data, sizeof(dp))) {
		ret = -EFAULT;
		goto bail;
	}

	dd = qib_lookup(dp.unit);
	if (!dd || !(dd->flags & QIB_PRESENT) || !dd->kregbase) {
		ret = -ENODEV;
		goto bail;
	}
	if (!(dd->flags & QIB_INITTED)) {
		/* no hardware, freeze, etc. */
		ret = -ENODEV;
		goto bail;
	}

	if (dp.version != _DIAG_PKT_VERS) {
		qib_dev_err(dd, "Invalid version %u for diagpkt_write\n",
			    dp.version);
		ret = -EINVAL;
		goto bail;
	}

	/* send count must be an exact number of dwords */
	if (dp.len & 3) {
		ret = -EINVAL;
		goto bail;
	}

	/* need a valid context */
	if (dp.context >= dd->num_send_contexts) {
		ret = -EINVAL;
		goto bail;
	}
	/* can only use kernel contexts */
	if (dd->send_contexts[dp.context].type != SC_KERNEL) {
		ret = -EINVAL;
		goto bail;
	}
	/* must be allocated */
	sc = dd->send_contexts[dp.context].sc;
	if (!sc) {
		ret = -EINVAL;
		goto bail;
	}
	/* TODO: check for enabled?  Or should that be in the buffer
	   allocator? */

	/* allocate a buffer and copy the data in */
	tmpbuf = vmalloc(dp.len);
	if (!tmpbuf) {
		dd_dev_info(dd, "Unable to allocate tmp buffer, failing\n");
		ret = -ENOMEM;
		goto bail;
	}

	if (copy_from_user(tmpbuf,
			   (const void __user *) (unsigned long) dp.data,
			   dp.len)) {
		ret = -EFAULT;
		goto bail;
	}

	/* calculate the packet and total lengths, in dwords */
	pkt_len = dp.len >> 2;
	total_len = pkt_len + 2;	/* PCB + packet */

	/* if 0, fill in a default */
	if (dp.pbc_wd == 0)
		dp.pbc_wd = create_pbc(sc, 0, 0, 0, total_len);

	pbuf = sc_buffer_alloc(sc, total_len, NULL, 0);
	if (!pbuf) {
		ret = -ENOSPC;
		goto bail;
	}

	pio_copy(pbuf, dp.pbc_wd, tmpbuf, pkt_len);
	/* no flush needed as the HW knows the packet size */

	ret = sizeof(dp);

bail:
	vfree(tmpbuf);
	return ret;
}

static int diag_release(struct inode *in, struct file *fp)
{
	mutex_lock(&qib_mutex);
	return_client(fp->private_data);
	fp->private_data = NULL;
	mutex_unlock(&qib_mutex);
	return 0;
}

/*
 * Chip-specific code calls to register its interest in
 * a specific range.
 */
struct diag_observer_list_elt {
	struct diag_observer_list_elt *next;
	const struct diag_observer *op;
};

int qib_register_observer(struct hfi_devdata *dd,
			  const struct diag_observer *op)
{
	struct diag_observer_list_elt *olp;
	int ret = -EINVAL;

	if (!dd || !op)
		goto bail;
	ret = -ENOMEM;
	olp = vmalloc(sizeof *olp);
	if (!olp) {
		pr_err("vmalloc for observer failed\n");
		goto bail;
	}
	if (olp) {
		unsigned long flags;

		spin_lock_irqsave(&dd->qib_diag_trans_lock, flags);
		olp->op = op;
		olp->next = dd->diag_observer_list;
		dd->diag_observer_list = olp;
		spin_unlock_irqrestore(&dd->qib_diag_trans_lock, flags);
		ret = 0;
	}
bail:
	return ret;
}

/* Remove all registered observers when device is closed */
static void unregister_observers(struct hfi_devdata *dd)
{
	struct diag_observer_list_elt *olp;
	unsigned long flags;

	spin_lock_irqsave(&dd->qib_diag_trans_lock, flags);
	olp = dd->diag_observer_list;
	while (olp) {
		/* Pop one observer, let go of lock */
		dd->diag_observer_list = olp->next;
		spin_unlock_irqrestore(&dd->qib_diag_trans_lock, flags);
		vfree(olp);
		/* try again. */
		spin_lock_irqsave(&dd->qib_diag_trans_lock, flags);
		olp = dd->diag_observer_list;
	}
	spin_unlock_irqrestore(&dd->qib_diag_trans_lock, flags);
}

/*
 * Find the observer, if any, for the specified address. Initial implementation
 * is simple stack of observers. This must be called with diag transaction
 * lock held.
 */
static const struct diag_observer *diag_get_observer(struct hfi_devdata *dd,
						     u32 addr)
{
	struct diag_observer_list_elt *olp;
	const struct diag_observer *op = NULL;

	olp = dd->diag_observer_list;
	while (olp) {
		op = olp->op;
		if (addr >= op->bottom && addr <= op->top)
			break;
		olp = olp->next;
	}
	if (!olp)
		op = NULL;

	return op;
}

static ssize_t diag_read(struct file *fp, char __user *data,
			     size_t count, loff_t *off)
{
	struct diag_client *dc = fp->private_data;
	struct hfi_devdata *dd = dc->dd;
	void __iomem *kreg_base;
	ssize_t ret;

	if (dc->pid != current->pid) {
		ret = -EPERM;
		goto bail;
	}

	kreg_base = dd->kregbase;

	if (count == 0)
		ret = 0;
	else if ((count % 8) || (*off % 8))
		/* address or length is not 64-bit aligned, hence invalid */
		ret = -EINVAL;
	else if (dc->state < READY && (*off || count != 8))
		ret = -EINVAL;  /* prevent cat /dev/hfi_diag* */
	else {
		unsigned long flags;
		u64 data64 = 0;
		const struct diag_observer *op;

		ret = -1;
		spin_lock_irqsave(&dd->qib_diag_trans_lock, flags);
		/*
		 * Check for observer on this address range.
		 * we only support a single 64-bit read
		 * via observer, currently.
		 */
		op = diag_get_observer(dd, *off);
		if (op) {
			u32 offset = *off;
			ret = op->hook(dd, op, offset, &data64, 0, 0);
		}
		/*
		 * We need to release lock before any copy_to_user(),
		 * whether implicit in read_umem64 or explicit below.
		 */
		spin_unlock_irqrestore(&dd->qib_diag_trans_lock, flags);
		if (!op) {
			ret = read_umem64(dd, data, (u32) *off, count);
		} else if (ret == count) {
			/* Below finishes case where observer existed */
			ret = copy_to_user(data, &data64, sizeof(u64));
			if (ret)
				ret = -EFAULT;
		}
	}

	if (ret >= 0) {
		*off += count;
		ret = count;
		if (dc->state == OPENED)
			dc->state = INIT;
	}
bail:
	return ret;
}

static ssize_t diag_write(struct file *fp, const char __user *data,
			      size_t count, loff_t *off)
{
	struct diag_client *dc = fp->private_data;
	struct hfi_devdata *dd = dc->dd;
	ssize_t ret;

	if (dc->pid != current->pid) {
		ret = -EPERM;
		goto bail;
	}

	if (count == 0)
		ret = 0;
	else if ((count % 8) || (*off % 8))
		/* address or length is not 64-bit aligned, hence invalid */
		ret = -EINVAL;
	else if (dc->state < READY &&
		((*off || count != 8) || dc->state != INIT))
		/* No writes except second-step of init seq */
		ret = -EINVAL;  /* before any other write allowed */
	else {
		unsigned long flags;
		const struct diag_observer *op = NULL;

		/*
		 * Check for observer on this address range.
		 * We only support a single 32 or 64-bit write
		 * via observer, currently. This helps, because
		 * we would otherwise have to jump through hoops
		 * to make "diag transaction" meaningful when we
		 * cannot do a copy_from_user while holding the lock.
		 */
		if (count == 8) {
			u64 data64;
			u32 offset = *off;
			ret = copy_from_user(&data64, data, count);
			if (ret) {
				ret = -EFAULT;
				goto bail;
			}
			spin_lock_irqsave(&dd->qib_diag_trans_lock, flags);
			op = diag_get_observer(dd, *off);
			// FIXME: op->hook()'x last arg used to be use_32
			if (op)
				ret = op->hook(dd, op, offset, &data64, ~0Ull,
					       0);
			spin_unlock_irqrestore(&dd->qib_diag_trans_lock, flags);
		}

		if (!op) {
			ret = write_umem64(dd, (u32) *off, data, count);
		}
	}

	if (ret >= 0) {
		*off += count;
		ret = count;
		if (dc->state == INIT)
			dc->state = READY; /* all read/write OK now */
	}
bail:
	return ret;
}
