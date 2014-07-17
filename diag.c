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
#include <linux/module.h>
#include <rdma/ib_smi.h>
#include "hfi.h"
#include "device.h"
#include "common.h"
#include "trace.h"

#undef pr_fmt
#define pr_fmt(fmt) DRIVER_NAME ": " fmt
#define snoop_dbg(fmt, ...) \
	hfi_cdbg(SNOOP, fmt, ##__VA_ARGS__)
#define HFI_PORT_SNOOP_MODE     1U
#define HFI_PORT_CAPTURE_MODE   2U

unsigned int snoop_enable = 0; /* By default (0) snooping is disabled */
unsigned int snoop_drop_send = 0; /* Drop outgoing PIO/SDMA requests */
unsigned int snoop_force_capture = 0; /* Force into capture */

module_param_named(snoop_enable, snoop_enable , int, 0644);
MODULE_PARM_DESC(snoop_enable, "snooping mode ");

module_param_named(snoop_drop_send, snoop_drop_send , int, 0644);
MODULE_PARM_DESC(snoop_drop_send, "drop outgoing snooped PIO/DMA packets ");

module_param_named(snoop_force_capture, snoop_force_capture, int, 0644);
MODULE_PARM_DESC(snoop_force_capture, "force snoop to capture mode ");

/*
 * Extract packet length from LRH header.
 * Why & 0x7FF? Because len is only 11 bits in case it wasn't 0'd we throw the
 * bogus bits away. This is in Dwords so multiply by 4 to get size in bytes
 */
#define HFI_GET_PKT_LEN(x)      (((be16_to_cpu((x)->lrh[2]) & 0x7FF)) << 2)

enum hfi_filter_status {
	HFI_FILTER_HIT,
	HFI_FILTER_ERR,
	HFI_FILTER_MISS
};

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

/* Snoop packet structure */
struct snoop_packet {
	struct list_head list;
	u32 total_len;
	u8 data[];
};

/* Do not make these an enum or it will blow up the capture_md */
#define PKT_DIR_EGRESS 0x0
#define PKT_DIR_INGRESS 0x1

/* Packet capture metadata returned to the user with the packet. */
struct capture_md {
	u8 port;
	u8 dir;
	u8 reserved[6];
	union {
		u64 pbc;
		u64 rhf;
	} u;
};

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

/*
 * This is used for communication with user space for snoop extended IOCTLs
 */
struct hfi_link_info {
	 __be64 node_guid;
	u8 port_mode;
	u8 port_state;
	__be16 link_speed_active;
	__be16 link_width_active;
	__be16 vl15_init;
	u8 port_number;
	/*
	 * Add padding to make this a full IB SMP payload. Note: changing the
	 * size of this structure will make the IOCTLs creaed with _IOWR change.
	 * Be sure to run tests on all IOCTLs when making changes to this
	 * structure.
	 */
	u8 res[47];
};

/*
 * This starts our ioctl sequence numbers *way* off from the ones
 * defined in ib_core and matches what as used in the qib driver.
 */
#define SNOOP_CAPTURE_VERSION 0x1
#define IB_IOCTL_MAGIC          0x1b /* Present in Qib file ib_user_mad.h */
#define HFI_SNOOP_IOC_MAGIC IB_IOCTL_MAGIC
#define HFI_SNOOP_IOC_BASE_SEQ 0x80

#define HFI_SNOOP_IOCGETLINKSTATE \
	_IO(HFI_SNOOP_IOC_MAGIC, HFI_SNOOP_IOC_BASE_SEQ)
#define HFI_SNOOP_IOCSETLINKSTATE \
	_IO(HFI_SNOOP_IOC_MAGIC, HFI_SNOOP_IOC_BASE_SEQ+1)
#define HFI_SNOOP_IOCCLEARQUEUE \
	_IO(HFI_SNOOP_IOC_MAGIC, HFI_SNOOP_IOC_BASE_SEQ+2)
#define HFI_SNOOP_IOCCLEARFILTER \
	_IO(HFI_SNOOP_IOC_MAGIC, HFI_SNOOP_IOC_BASE_SEQ+3)
#define HFI_SNOOP_IOCSETFILTER \
	_IO(HFI_SNOOP_IOC_MAGIC, HFI_SNOOP_IOC_BASE_SEQ+4)
#define HFI_SNOOP_IOCGETVERSION \
	_IO(HFI_SNOOP_IOC_MAGIC, HFI_SNOOP_IOC_BASE_SEQ+5)

/* The following get very high IOCTL numbers for some reason */
#define HFI_SNOOP_IOCGETLINKSTATE_EXTRA \
	_IOWR(HFI_SNOOP_IOC_MAGIC, HFI_SNOOP_IOC_BASE_SEQ+6, \
		struct hfi_link_info)
#define HFI_SNOOP_IOCSETLINKSTATE_EXTRA \
	_IOWR(HFI_SNOOP_IOC_MAGIC, HFI_SNOOP_IOC_BASE_SEQ+7, \
		struct hfi_link_info)

static int hfi_snoop_open(struct inode *in, struct file *fp);
static ssize_t hfi_snoop_read(struct file *fp, char __user *data,
				size_t pkt_len, loff_t *off);
static ssize_t hfi_snoop_write(struct file *fp, const char __user *data,
				 size_t count, loff_t *off);
static long hfi_ioctl(struct file *fp, unsigned int cmd, unsigned long arg);
static unsigned int hfi_snoop_poll(struct file *fp,
					struct poll_table_struct *wait);
static int hfi_snoop_release(struct inode *in, struct file *fp);

static void dump_ioctl_table(void);

struct hfi_packet_filter_command {
	int opcode;
	int length;
	void *value_ptr;
};

enum hfi_packet_filter_opcodes {
	FILTER_BY_LID,
	FILTER_BY_DLID,
	FILTER_BY_MAD_MGMT_CLASS,
	FILTER_BY_QP_NUMBER,
	FILTER_BY_PKT_TYPE,
	FILTER_BY_SERVICE_LEVEL,
	FILTER_BY_PKEY
};

static const struct file_operations snoop_file_ops = {
	.owner = THIS_MODULE,
	.open = hfi_snoop_open,
	.read = hfi_snoop_read,
	.unlocked_ioctl = hfi_ioctl,
	.poll = hfi_snoop_poll,
	.write = hfi_snoop_write,
	.release = hfi_snoop_release
};

struct hfi_filter_array {
	int (*filter)(void *, void *, void *);
};

static int hfi_filter_lid(void *ibhdr, void *packet_data, void *value);
static int hfi_filter_dlid(void *ibhdr, void *packet_data, void *value);
static int hfi_filter_mad_mgmt_class(void *ibhdr, void *packet_data,
				     void *value);
static int hfi_filter_qp_number(void *ibhdr, void *packet_data, void *value);
static int hfi_filter_ibpacket_type(void *ibhdr, void *packet_data,
				    void *value);
static int hfi_filter_ib_service_level(void *ibhdr, void *packet_data,
				       void *value);
static int hfi_filter_ib_pkey(void *ibhdr, void *packet_data, void *value);

static struct hfi_filter_array hfi_filters[] = {
	{ hfi_filter_lid },
	{ hfi_filter_dlid },
	{ hfi_filter_mad_mgmt_class },
	{ hfi_filter_qp_number },
	{ hfi_filter_ibpacket_type },
	{ hfi_filter_ib_service_level },
	{ hfi_filter_ib_pkey }
};

#define HFI_MAX_FILTERS	ARRAY_SIZE(hfi_filters)
#define HFI_DIAG_MINOR_BASE	129

static int hfi_snoop_add(struct hfi_devdata *dd, const char *name);

int qib_diag_add(struct hfi_devdata *dd)
{
	char name[16];
	int ret = 0;

	/*
	 * When snoop/capture is on, hijack the diagpkt device
	 */

	if (snoop_enable) {
		snprintf(name, sizeof(name), "%s_diagpkt%d", class_name(),
			 dd->unit);
		/*
		 * Do this for each device as opposed to the normal diagpkt
		 * interface which is one per host
		 */
		ret = hfi_snoop_add(dd, name);
	} else {
		snprintf(name, sizeof(name), "%s_diagpkt", class_name());
		if (atomic_inc_return(&diagpkt_count) == 1) {
			ret = hfi_cdev_init(HFI_DIAGPKT_MINOR, name,
				    &diagpkt_file_ops, &diagpkt_cdev,
				    &diagpkt_device);
		}
	}

	if (ret)
		goto done;

	snprintf(name, sizeof(name), "%s_diag%d", class_name(), dd->unit);
	ret = hfi_cdev_init(HFI_DIAG_MINOR_BASE + dd->unit, name,
			    &diag_file_ops, &dd->diag_cdev,
			    &dd->diag_device);
done:
	return ret;
}

static void unregister_observers(struct hfi_devdata *dd);

/* this must be called w/ dd->snoop_in_lock held */
static void drain_snoop_list(struct list_head *queue)
{
	struct list_head *pos, *q;
	struct snoop_packet *packet;

	list_for_each_safe(pos, q, queue) {
		packet = list_entry(pos, struct snoop_packet, list);
		list_del(pos);
		kfree(packet);
	}
}

void hfi_snoop_remove(struct hfi_devdata *dd)
{
	unsigned long flags = 0;

	spin_lock_irqsave(&dd->hfi_snoop.snoop_lock, flags);
	drain_snoop_list(&dd->hfi_snoop.queue);
	hfi_cdev_cleanup(&dd->hfi_snoop.cdev, &dd->hfi_snoop.class_dev);
	spin_unlock_irqrestore(&dd->hfi_snoop.snoop_lock, flags);
}

void qib_diag_remove(struct hfi_devdata *dd)
{
	struct diag_client *dc;

	if (snoop_enable) {
		hfi_snoop_remove(dd);
	} else {
		if (atomic_dec_and_test(&diagpkt_count))
			hfi_cdev_cleanup(&diagpkt_cdev, &diagpkt_device);
	}
	hfi_cdev_cleanup(&dd->diag_cdev, &dd->diag_device);

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
	int unit = iminor(in) - HFI_DIAG_MINOR_BASE;
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
static ssize_t diagpkt_send(struct diag_pkt *dp)
{
	struct hfi_devdata *dd;
	struct send_context *sc;
	struct pio_buf *pbuf;
	u32 *tmpbuf = NULL;
	ssize_t ret = 0;
	u32 pkt_len, total_len;

	dd = qib_lookup(dp->unit);
	if (!dd || !(dd->flags & QIB_PRESENT) || !dd->kregbase) {
		ret = -ENODEV;
		goto bail;
	}
	if (!(dd->flags & QIB_INITTED)) {
		/* no hardware, freeze, etc. */
		ret = -ENODEV;
		goto bail;
	}

	if (dp->version != _DIAG_PKT_VERS) {
		dd_dev_err(dd, "Invalid version %u for diagpkt_write\n",
			    dp->version);
		ret = -EINVAL;
		goto bail;
	}

	/* send count must be an exact number of dwords */
	if (dp->len & 3) {
		ret = -EINVAL;
		goto bail;
	}

	/* need a valid context */
	if (dp->context >= dd->num_send_contexts) {
		ret = -EINVAL;
		goto bail;
	}
	/* can only use kernel contexts */
	if (dd->send_contexts[dp->context].type != SC_KERNEL) {
		ret = -EINVAL;
		goto bail;
	}
	/* must be allocated */
	sc = dd->send_contexts[dp->context].sc;
	if (!sc) {
		ret = -EINVAL;
		goto bail;
	}
	/* TODO: check for enabled?  Or should that be in the buffer
	   allocator? */

	/* allocate a buffer and copy the data in */
	tmpbuf = vmalloc(dp->len);
	if (!tmpbuf) {
		dd_dev_info(dd, "Unable to allocate tmp buffer, failing\n");
		ret = -ENOMEM;
		goto bail;
	}

	if (copy_from_user(tmpbuf,
			   (const void __user *) (unsigned long) dp->data,
			   dp->len)) {
		ret = -EFAULT;
		goto bail;
	}

	/*
	 * pkt_len is how much data we have to write, includes header and data.
	 * total_len is length of the packet in Dwords plus the PBC should not
	 * include the CRC.
	 */
	pkt_len = dp->len >> 2;
	total_len = pkt_len + 2; /* PBC + packet */

	/* if 0, fill in a default */
	if (dp->pbc == 0)
		dp->pbc = create_pbc(0, 0, 0, total_len);

	pbuf = sc_buffer_alloc(sc, total_len, NULL, 0);
	if (!pbuf) {
		ret = -ENOSPC;
		goto bail;
	}

	pio_copy(pbuf, dp->pbc, tmpbuf, pkt_len);
	/* no flush needed as the HW knows the packet size */

	ret = sizeof(*dp);

bail:
	vfree(tmpbuf);
	return ret;
}

static ssize_t diagpkt_write(struct file *fp, const char __user *data,
				 size_t count, loff_t *off)
{
	struct diag_pkt dp;

	if (count != sizeof(dp))
		return -EINVAL;

	if (copy_from_user(&dp, data, sizeof(dp)))
		return -EFAULT;

	return diagpkt_send(&dp);
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

/*
 * In qib this was per port per device. For hfi we are making this per device
 * only.
 */
static int hfi_snoop_add(struct hfi_devdata *dd, const char *name)
{
	int ret = 0;

	dd->hfi_snoop.mode_flag = 0;
	spin_lock_init(&dd->hfi_snoop.snoop_lock);
	INIT_LIST_HEAD(&(dd->hfi_snoop.queue));
	init_waitqueue_head(&dd->hfi_snoop.waitq);

	ret = hfi_cdev_init(HFI_SNOOP_CAPTURE_BASE + dd->unit, name,
			    &snoop_file_ops,
			    &dd->hfi_snoop.cdev, &dd->hfi_snoop.class_dev);

	if (ret) {
		dd_dev_err(dd, "Couldn't create %s device: %d", name, ret);
		hfi_cdev_cleanup(&dd->hfi_snoop.cdev,
				 &dd->hfi_snoop.class_dev);
	} else {
		dump_ioctl_table();
	}

	return ret;
}

static struct hfi_devdata *hfi_dd_from_sc_inode(struct inode *in)
{
	int unit = iminor(in) - HFI_SNOOP_CAPTURE_BASE;
	struct hfi_devdata *dd = NULL;

	dd = qib_lookup(unit);
	return dd;

}

static int hfi_snoop_open(struct inode *in, struct file *fp)
{
	int ret, i;
	int mode_flag = 0;
	unsigned long flags = 0;
	struct hfi_devdata *dd;
	struct list_head *queue;
	u64 reg_cur, reg_new;

	mutex_lock(&qib_mutex);

	dd = hfi_dd_from_sc_inode(in);
	if (dd == NULL)
		return -ENODEV;

	/*
	 * File mode determines snoop or capture. Some exisitng user
	 * applications expect the capture device to be able to be opened RDWR
	 * because they expect a dedicated capture device. For this reason we
	 * support a module param to force capture mode even if the file open
	 * mode matches snoop.
	 */
	if (((fp->f_flags & O_ACCMODE) == O_RDONLY) ||
	      snoop_force_capture){
		snoop_dbg("Capture Enabled\n");
		mode_flag = HFI_PORT_CAPTURE_MODE;
	} else if ((fp->f_flags & O_ACCMODE) == O_RDWR) {
		snoop_dbg("Snoop Enabled\n");
		mode_flag = HFI_PORT_SNOOP_MODE;
	} else {
		snoop_dbg("Invalid\n");
		ret =  -EINVAL;
		goto bail;
	}
	queue = &dd->hfi_snoop.queue;

	/*
	 * We are not supporting snoop and capture at the same time.
	 */
	spin_lock_irqsave(&dd->hfi_snoop.snoop_lock, flags);
	if (dd->hfi_snoop.mode_flag) {
		ret = -EBUSY;
		spin_unlock_irqrestore(&dd->hfi_snoop.snoop_lock, flags);
		goto bail;
	}

	dd->hfi_snoop.mode_flag = mode_flag;
	drain_snoop_list(queue);

	dd->hfi_snoop.filter_callback = NULL;
	dd->hfi_snoop.filter_value = NULL;

	/*
	 * Send side packet integrity checks are not helpful when snooping so
	 * disable and re-enable when we stop snooping.
	 */
	for (i = 0; i < dd->num_send_contexts; i++) {
		reg_cur = read_kctxt_csr(dd, i, WFR_SEND_CTXT_CHECK_ENABLE);
		reg_new = ~(HFI_PKT_BASE_SC_INTEGRITY) & reg_cur;
		write_kctxt_csr(dd, i, WFR_SEND_CTXT_CHECK_ENABLE, reg_new);
	}

	/*
	 * As soon as we set these function pointers the recv and send handlers
	 * are active. This is a race condition so we must make sure to drain
	 * the queue and init filter values above. Technically we should add
	 * locking here but all that will happen is on recv a packet will get
	 * allocated and get stuck on the snoop_lock before getting added to the
	 * queue. Same goes for send.
	 */
	rhf_rcv_function_map[RHF_RCV_TYPE_IB] = snoop_recv_handler;
	rhf_rcv_function_map[RHF_RCV_TYPE_BYPASS] = snoop_recv_handler;
	rhf_rcv_function_map[RHF_RCV_TYPE_ERROR] = snoop_recv_handler;
	rhf_rcv_function_map[RHF_RCV_TYPE_EXPECTED] = snoop_recv_handler;
	rhf_rcv_function_map[RHF_RCV_TYPE_EAGER] = snoop_recv_handler;

	dd->process_pio_send = snoop_send_pio_handler;
	dd->process_dma_send = snoop_send_pio_handler;

	spin_unlock_irqrestore(&dd->hfi_snoop.snoop_lock, flags);
	ret = 0;

bail:
	mutex_unlock(&qib_mutex);

	return ret;
}

static int hfi_snoop_release(struct inode *in, struct file *fp)
{
	unsigned long flags = 0;
	struct hfi_devdata *dd;
	int i;
	u64 reg_cur, reg_new;

	dd = hfi_dd_from_sc_inode(in);
	if (dd == NULL)
		return -ENODEV;

	spin_lock_irqsave(&dd->hfi_snoop.snoop_lock, flags);

	/*
	 * Drain the queue and clear the filters we are done with it. Don't
	 * forget to restore the packet integrity checks
	 */
	drain_snoop_list(&dd->hfi_snoop.queue);
	for (i = 0; i < dd->num_send_contexts; i++) {
		reg_cur = read_kctxt_csr(dd, i, WFR_SEND_CTXT_CHECK_ENABLE);
		reg_new = HFI_PKT_BASE_SC_INTEGRITY | reg_cur;
		write_kctxt_csr(dd, i, WFR_SEND_CTXT_CHECK_ENABLE, reg_new);
	}

	dd->hfi_snoop.filter_callback = NULL;
	kfree(dd->hfi_snoop.filter_value);
	dd->hfi_snoop.filter_value = NULL;

	/*
	 * User is done snooping and capturing, return control to the normal
	 * handler. Re-enable SDMA handling.
	 */
	dd->hfi_snoop.mode_flag = 0;

	rhf_rcv_function_map[RHF_RCV_TYPE_IB] = process_receive_ib;
	rhf_rcv_function_map[RHF_RCV_TYPE_BYPASS] = process_receive_bypass;
	rhf_rcv_function_map[RHF_RCV_TYPE_ERROR] = process_receive_error;
	rhf_rcv_function_map[RHF_RCV_TYPE_EAGER] = process_receive_eager;
	rhf_rcv_function_map[RHF_RCV_TYPE_EXPECTED] = process_receive_expected;

	dd->process_pio_send = qib_verbs_send_pio;
	dd->process_dma_send = qib_verbs_send_dma;

	spin_unlock_irqrestore(&dd->hfi_snoop.snoop_lock, flags);

	snoop_dbg("snoop/capture device released\n");

	return 0;
}

static unsigned int hfi_snoop_poll(struct file *fp,
		struct poll_table_struct *wait)
{
	int ret = 0;
	unsigned long flags = 0;

	struct hfi_devdata *dd;

	dd = hfi_dd_from_sc_inode(fp->f_inode);
	if (dd == NULL)
		return -ENODEV;

	spin_lock_irqsave(&dd->hfi_snoop.snoop_lock, flags);

	poll_wait(fp, &dd->hfi_snoop.waitq, wait);
	if (!list_empty(&dd->hfi_snoop.queue))
		ret |= POLLIN | POLLRDNORM;

	spin_unlock_irqrestore(&dd->hfi_snoop.snoop_lock, flags);
	return ret;

}

static ssize_t hfi_snoop_write(struct file *fp, const char __user *data,
				 size_t count, loff_t *off)
{
	struct diag_pkt dpkt;
	struct hfi_devdata *dd;
	size_t ret;
	u8 byte_one, vl;
	u32 len;
	u64 pbc;

	dd = hfi_dd_from_sc_inode(fp->f_inode);
	if (dd == NULL)
		return -ENODEV;

	snoop_dbg("received %lu bytes from user\n", count);

	dpkt.context = 0; /* throw everything out send context 0 for now */
	dpkt.version = _DIAG_PKT_VERS;
	dpkt.unit = dd->unit;
	dpkt.len = count;
	dpkt.data = (unsigned long)data;

	/*
	 * We need to generate the PBC and not let diagpkt_send do it,
	 * to do this we need the VL and the length in dwords
	 */
	if (copy_from_user(&byte_one, data, 1))
		return -EINVAL;

	vl = (byte_one >> 4) & 0xf;
	len = (count >> 2) + 2; /* Add in PBC */
	pbc = create_pbc(0, 0, vl, len);
	dpkt.pbc = pbc;
	ret = diagpkt_send(&dpkt);
	/*
	 * Qib snoop code returns the number of bytes written but the
	 * diagpkt_send only returns number of bytes in the diagpkt so patch
	 * that up here before returning.
	 */
	if (ret == sizeof(dpkt))
		return count;

	return ret;
}

static ssize_t hfi_snoop_read(struct file *fp, char __user *data,
			       size_t pkt_len, loff_t *off)
{
	ssize_t ret = 0;
	unsigned long flags = 0;
	struct snoop_packet *packet = NULL;
	struct hfi_devdata *dd;

	dd = hfi_dd_from_sc_inode(fp->f_inode);
	if (dd == NULL)
		return -ENODEV;

	spin_lock_irqsave(&dd->hfi_snoop.snoop_lock, flags);

	while (list_empty(&dd->hfi_snoop.queue)) {
		spin_unlock_irqrestore(&dd->hfi_snoop.snoop_lock, flags);

		if (fp->f_flags & O_NONBLOCK)
			return -EAGAIN;

		if (wait_event_interruptible(dd->hfi_snoop.waitq,
					!list_empty(&dd->hfi_snoop.queue)))
			return -EINTR;

		spin_lock_irqsave(&dd->hfi_snoop.snoop_lock, flags);
	}

	if (!list_empty(&(dd->hfi_snoop.queue))) {
		packet = list_entry(dd->hfi_snoop.queue.next,
				struct snoop_packet, list);
		list_del(&packet->list);
		spin_unlock_irqrestore(&dd->hfi_snoop.snoop_lock, flags);
		if (pkt_len >= packet->total_len) {
			if (copy_to_user(data, packet->data,
				packet->total_len))
				ret = -EFAULT;
			else
				ret = packet->total_len;
		} else
			ret = -EINVAL;

		kfree(packet);
	} else
		spin_unlock_irqrestore(&dd->hfi_snoop.snoop_lock, flags);

	return ret;
}

#define IB_PHYSPORTSTATE_SLEEP 1

static long hfi_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
	struct hfi_devdata *dd;
	void *filter_value = NULL;
	long ret = 0;
	int value = 0;
	u8 physState = 0;
	u8 linkState = 0;
	u16 devState = 0;
	unsigned long flags = 0;
	unsigned long *argp = NULL;
	struct hfi_packet_filter_command filter_cmd = {0};
	int mode_flag = 0;
	struct qib_pportdata *ppd = NULL;
	unsigned int index;
	struct hfi_link_info link_info;

	dd = hfi_dd_from_sc_inode(fp->f_inode);
	if (dd == NULL)
		return -ENODEV;

	spin_lock_irqsave(&dd->hfi_snoop.snoop_lock, flags);

	mode_flag = dd->hfi_snoop.mode_flag;

	if (((_IOC_DIR(cmd) & _IOC_READ)
	    && !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd)))
	    || ((_IOC_DIR(cmd) & _IOC_WRITE)
	    && !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd)))) {
		ret = -EFAULT;
	} else if (!capable(CAP_SYS_ADMIN)) {
		ret = -EPERM;
	} else if ((mode_flag & HFI_PORT_CAPTURE_MODE)
		&& cmd != HFI_SNOOP_IOCCLEARQUEUE
		&& cmd != HFI_SNOOP_IOCCLEARFILTER
		&& cmd != HFI_SNOOP_IOCSETFILTER) {
		/* Capture devices are allowed only 3 operations
		 * 1.Clear capture queue
		 * 2.Clear capture filter
		 * 3.Set capture filter
		 * Other are invalid.
		 */
		ret = -EINVAL;
	} else {
		switch (cmd) {
		case HFI_SNOOP_IOCSETLINKSTATE:
			snoop_dbg("HFI_SNOOP_IOCSETLINKSTATE is not valid");
			ret = -EINVAL;
			break;

		case HFI_SNOOP_IOCSETLINKSTATE_EXTRA:
			memset(&link_info, 0, sizeof(link_info));

			ret = copy_from_user(&link_info,
				(struct hfi_link_info __user *)arg,
				sizeof(link_info));
			if (ret)
				break;

			value = link_info.port_state;
			index = link_info.port_number;
			if (index > dd->num_pports - 1) {
				ret = -EINVAL;
				break;
			}

			ppd = &dd->pport[index];
			if (!ppd) {
				ret = -EINVAL;
				break;
			}

			/* What we want to transition to */
			physState = (value >> 4) & 0xF;
			linkState = value & 0xF;
			snoop_dbg("Setting link state 0x%x\n", value);

			switch (linkState) {
			case IB_PORT_NOP:
				if (physState == 0)
					break;
					/* fall through */
			case IB_PORT_DOWN:
				switch (physState) {
				case 0:
					if (dd->f_ibphys_portstate &&
					    (dd->f_ibphys_portstate(ppd)
					    & 0xF & IB_PHYSPORTSTATE_SLEEP))
						devState =
						QIB_IB_LINKDOWN_SLEEP;
					else
						devState =
						QIB_IB_LINKDOWN;
						break;
				case 1:
					devState = QIB_IB_LINKDOWN_SLEEP;
					break;
				case 2:
					devState = QIB_IB_LINKDOWN;
					break;
				case 3:
					devState = QIB_IB_LINKDOWN_DISABLE;
					break;
				default:
					ret = -EINVAL;
					goto done;
					break;
				}
				ret = qib_set_linkstate(ppd, devState);
				break;
			case IB_PORT_ARMED:
				ret = qib_set_linkstate(ppd, QIB_IB_LINKARM);
				break;
			case IB_PORT_ACTIVE:
				ret = qib_set_linkstate(ppd, QIB_IB_LINKACTIVE);
				break;
			default:
				ret = -EINVAL;
				break;
			}

			if (ret)
				break;
			/* fall through */
		case HFI_SNOOP_IOCGETLINKSTATE:
		case HFI_SNOOP_IOCGETLINKSTATE_EXTRA:
			if (cmd == HFI_SNOOP_IOCGETLINKSTATE_EXTRA) {
				memset(&link_info, 0, sizeof(link_info));
				ret = copy_from_user(&link_info,
					(struct hfi_link_info __user *)arg,
					sizeof(link_info));
				index = link_info.port_number;
			} else {
				ret = __get_user(index, (int __user *) arg);
				if (ret !=  0)
					break;
			}

			if (index > dd->num_pports - 1) {
				ret = -EINVAL;
				break;
			}

			ppd = &dd->pport[index];
			if (!ppd) {
				ret = -EINVAL;
				break;
			}
			value = dd->f_ibphys_portstate(ppd);
			value <<= 4;
			value |= dd->f_iblink_state(ppd);

			snoop_dbg("Link port | Link State: %d\n", value);

			if ((cmd == HFI_SNOOP_IOCGETLINKSTATE_EXTRA) ||
			    (cmd == HFI_SNOOP_IOCSETLINKSTATE_EXTRA)) {
				link_info.port_state = value;
				link_info.node_guid = ppd->guid;
				link_info.link_speed_active =
							ppd->link_speed_active;
				link_info.link_width_active =
							ppd->link_width_active;
				/* FIXME what about port_mode and vl15_init? */
				ret = copy_to_user(
					(struct hfi_link_info __user *)arg,
					&link_info, sizeof(link_info));
			} else {
				ret = __put_user(value, (int __user *)arg);
			}
			break;

		case HFI_SNOOP_IOCCLEARQUEUE:
			drain_snoop_list(&dd->hfi_snoop.queue);
			break;

		case HFI_SNOOP_IOCCLEARFILTER:
			if (dd->hfi_snoop.filter_callback) {
				/* Drain packets first */
				drain_snoop_list(&dd->hfi_snoop.queue);
				dd->hfi_snoop.filter_callback = NULL;
			}
			kfree(dd->hfi_snoop.filter_value);
			dd->hfi_snoop.filter_value = NULL;
			break;

		case HFI_SNOOP_IOCSETFILTER:
			/* just copy command structure */
			argp = (unsigned long *)arg;
			ret = copy_from_user(&filter_cmd, (u8 *)argp,
				sizeof(filter_cmd));
			if (ret < 0) {
				pr_alert("Error copying filter command\n");
				break;
			}
			if (filter_cmd.opcode >= HFI_MAX_FILTERS) {
				pr_alert("Invalid opcode in request\n");
				ret = -EINVAL;
				break;
			}

			snoop_dbg("Opcode %d Len %d Ptr %p\n",
				   filter_cmd.opcode, filter_cmd.length,
				   filter_cmd.value_ptr);

			filter_value = kzalloc(
						filter_cmd.length * sizeof(u8),
						GFP_KERNEL);
			if (!filter_value) {
				pr_alert("Not enough memory\n");
				ret = -ENOMEM;
				break;
			}
			/* copy remaining data from userspace */
			ret = copy_from_user((u8 *)filter_value,
					(u8 *)filter_cmd.value_ptr,
					filter_cmd.length);
			if (ret < 0) {
				kfree(filter_value);
				pr_alert("Error copying filter data\n");
				break;
			}
			/* Drain packets first */
			drain_snoop_list(&dd->hfi_snoop.queue);
			dd->hfi_snoop.filter_callback =
				hfi_filters[filter_cmd.opcode].filter;
			/* just in case we see back to back sets */
			kfree(dd->hfi_snoop.filter_value);
			dd->hfi_snoop.filter_value = filter_value;

			break;
		case HFI_SNOOP_IOCGETVERSION:
			value = SNOOP_CAPTURE_VERSION;
			ret = __put_user(value, (int __user *)arg);
			break;
		default:
			ret = -ENOTTY;
			break;
		}
	}
done:
	spin_unlock_irqrestore(&dd->hfi_snoop.snoop_lock, flags);
	return ret;
}

static void qib_snoop_list_add_tail(struct snoop_packet *packet,
				    struct hfi_devdata *dd)
{
	unsigned long flags = 0;

	spin_lock_irqsave(&dd->hfi_snoop.snoop_lock, flags);
	if (likely((dd->hfi_snoop.mode_flag & HFI_PORT_SNOOP_MODE) ||
	    (dd->hfi_snoop.mode_flag & HFI_PORT_CAPTURE_MODE)))
		list_add_tail(&(packet->list), &dd->hfi_snoop.queue);

	/*
	 * Technically we can could have closed the snoop device while waiting
	 * on the above lock and it is gone now. The snoop mode_flag will
	 * prevent us from adding the packet to the queue though.
	 */

	spin_unlock_irqrestore(&dd->hfi_snoop.snoop_lock, flags);
	wake_up_interruptible(&dd->hfi_snoop.waitq);
}

#define HFI_FILTER_CHECK(val, msg)					\
do {									\
	if (val == NULL) {						\
		snoop_dbg("Error invalid " msg " value for filter\n");	\
		return HFI_FILTER_ERR;					\
	}								\
} while (0)

static int hfi_filter_lid(void *ibhdr, void *packet_data, void *value)
{
	struct qib_ib_header *hdr;

	HFI_FILTER_CHECK(ibhdr, "header");
	HFI_FILTER_CHECK(value, "user");

	hdr = (struct qib_ib_header *)ibhdr;

	if (*((u16 *)value) == be16_to_cpu(hdr->lrh[3])) /* matches slid */
		return HFI_FILTER_HIT; /* matched */

	return HFI_FILTER_MISS; /* Not matched */
}

static int hfi_filter_dlid(void *ibhdr, void *packet_data, void *value)
{
	struct qib_ib_header *hdr;

	HFI_FILTER_CHECK(ibhdr, "header");
	HFI_FILTER_CHECK(value, "user");

	hdr = (struct qib_ib_header *)ibhdr;

	if (*((u16 *)value) == be16_to_cpu(hdr->lrh[1]))
		return HFI_FILTER_HIT;

	return HFI_FILTER_MISS;
}

/* Not valid for outgoing packets, send handler passes null for data*/
static int hfi_filter_mad_mgmt_class(void *ibhdr, void *packet_data,
					 void *value)
{
	struct qib_ib_header *hdr;
	struct qib_other_headers *ohdr = NULL;
	struct ib_smp *smp = NULL;
	u32 qpn = 0;

	HFI_FILTER_CHECK(ibhdr, "header");
	HFI_FILTER_CHECK(packet_data, "packet_data");
	HFI_FILTER_CHECK(value, "user");

	hdr = (struct qib_ib_header *)ibhdr;

	/* Check for GRH */
	if ((be16_to_cpu(hdr->lrh[0]) & 3) == QIB_LRH_BTH)
		ohdr = &hdr->u.oth; /* LRH + BTH + DETH */
	else
		ohdr = &hdr->u.l.oth; /* LRH + GRH + BTH + DETH */

	qpn = be32_to_cpu(ohdr->bth[1]) & 0x00FFFFFF;
	if (qpn <= 1) {
		smp = (struct ib_smp *)packet_data;
		if (*((u8 *)value) == smp->mgmt_class)
			return HFI_FILTER_HIT;
		else
			return HFI_FILTER_MISS;
	}
	return HFI_FILTER_ERR;
}

static int hfi_filter_qp_number(void *ibhdr, void *packet_data, void *value)
{

	struct qib_ib_header *hdr;
	struct qib_other_headers *ohdr = NULL;

	HFI_FILTER_CHECK(ibhdr, "header");
	HFI_FILTER_CHECK(value, "user");

	hdr = (struct qib_ib_header *)ibhdr;

	/* Check for GRH */
	if ((be16_to_cpu(hdr->lrh[0]) & 3) == QIB_LRH_BTH)
		ohdr = &hdr->u.oth; /* LRH + BTH + DETH */
	else
		ohdr = &hdr->u.l.oth; /* LRH + GRH + BTH + DETH */
	if (*((u32 *)value) == (be32_to_cpu(ohdr->bth[1]) & 0x00FFFFFF))
		return HFI_FILTER_HIT;

	return HFI_FILTER_MISS;
}

static int hfi_filter_ibpacket_type(void *ibhdr, void *packet_data,
					void *value)
{
	u32 lnh = 0;
	u8 opcode = 0;
	struct qib_ib_header *hdr;
	struct qib_other_headers *ohdr = NULL;

	HFI_FILTER_CHECK(ibhdr, "header");
	HFI_FILTER_CHECK(value, "user");

	hdr = (struct qib_ib_header *)ibhdr;

	lnh = (be16_to_cpu(hdr->lrh[0]) & 3);

	if (lnh == QIB_LRH_BTH)
		ohdr = &hdr->u.oth;
	else if (lnh == QIB_LRH_GRH)
		ohdr = &hdr->u.l.oth;
	else
		return HFI_FILTER_ERR;

	opcode = be32_to_cpu(ohdr->bth[0]) >> 24;

	if (*((u8 *)value) == ((opcode >> 5) & 0x7))
		return HFI_FILTER_HIT;

	return HFI_FILTER_MISS;
}

static int hfi_filter_ib_service_level(void *ibhdr, void *packet_data,
					   void *value)
{
	struct qib_ib_header *hdr;

	HFI_FILTER_CHECK(ibhdr, "header");
	HFI_FILTER_CHECK(value, "user");

	hdr = (struct qib_ib_header *)ibhdr;

	if ((*((u8 *)value)) == (be16_to_cpu(hdr->lrh[0] >> 4) & 0xF))
		return HFI_FILTER_HIT;

	return HFI_FILTER_MISS;
}

static int hfi_filter_ib_pkey(void *ibhdr, void *packet_data, void *value)
{

	u32 lnh = 0;
	struct qib_ib_header *hdr;
	struct qib_other_headers *ohdr = NULL;

	HFI_FILTER_CHECK(ibhdr, "header");
	HFI_FILTER_CHECK(value, "user");

	hdr = (struct qib_ib_header *)ibhdr;

	lnh = (be16_to_cpu(hdr->lrh[0]) & 3);
	if (lnh == QIB_LRH_BTH)
		ohdr = &hdr->u.oth;
	else if (lnh == QIB_LRH_GRH)
		ohdr = &hdr->u.l.oth;
	else
		return HFI_FILTER_ERR;

	/* P_key is 16-bit entity, however top most bit indicates
	 * type of membership. 0 for limited and 1 for Full.
	 * Limited members cannot accept information from other
	 * Limited members, but communication is allowed between
	 * every other combination of membership.
	 * Hence we'll omitt comparing top-most bit while filtering
	 */

	if ((*(u16 *)value & 0x7FFF) ==
		((be32_to_cpu(ohdr->bth[0])) & 0x7FFF))
		return HFI_FILTER_HIT;

	return HFI_FILTER_MISS;
}

/*
 * This is just for making development and debugging easy. Remove this before
 * submitting code.
 */
static void dump_ioctl_table(void)
{
	pr_alert("\nIOCTL Table\n");
	pr_alert("-----------\n");
	pr_alert("Get Link State: %d\n", HFI_SNOOP_IOCGETLINKSTATE);
	pr_alert("Set Link State: %d\n", HFI_SNOOP_IOCSETLINKSTATE);
	pr_alert("Snoop Clear Queue: %d\n", HFI_SNOOP_IOCCLEARQUEUE);
	pr_alert("Snoop Clear Filter: %d\n", HFI_SNOOP_IOCCLEARFILTER);
	pr_alert("Snoop Set Filter: %d\n", HFI_SNOOP_IOCSETFILTER);
	pr_alert("Get Snoop/Capture Version: %d\n", HFI_SNOOP_IOCGETVERSION);
	pr_alert("Get link state extra: %lu\n",
					HFI_SNOOP_IOCGETLINKSTATE_EXTRA);
	pr_alert("Set link state extra: %lu\n",
					HFI_SNOOP_IOCSETLINKSTATE_EXTRA);

	pr_alert("\nSnoop/Capture Filters\n");
	pr_alert("---------------------\n");
	pr_alert("LID %d\n", FILTER_BY_LID);
	pr_alert("DLID %d\n", FILTER_BY_DLID);
	pr_alert("MAD Mgmt Class %d\n", FILTER_BY_MAD_MGMT_CLASS);
	pr_alert("QP Num %d\n", FILTER_BY_QP_NUMBER);
	pr_alert("Packet Type %d\n", FILTER_BY_PKT_TYPE);
	pr_alert("Service Level %d\n", FILTER_BY_SERVICE_LEVEL);
	pr_alert("PKey %d\n", FILTER_BY_PKEY);
}

/*
 * Allocate a snoop packet. The structure that is stored in the ring buffer, not
 * to be confused with an hfi packet type.
 */
static struct snoop_packet *allocate_snoop_packet(u32 hdr_len,
						  u32 data_len,
						  u32 md_len)
{

	struct snoop_packet *packet = NULL;

	packet = kzalloc(sizeof(struct snoop_packet) + hdr_len + data_len
			 + md_len,
			 GFP_ATOMIC);
	if (likely(packet))
		INIT_LIST_HEAD(&packet->list);


	return packet;
}

/*
 * Instead of having snoop and capture code intermixed with the recv functions,
 * both the interrupt handler and qib_ib_rcv() we are going to hijack the call
 * and land in here for snoop/capture but if not enbaled the call will go
 * through as before. This gives us a single point to constrain all of the snoop
 * snoop recv logic. There is nothign special that needs to happen for bypass
 * packets. This routine should not try to look into the packet. It just copied
 * it. There is no gurantee for filters when it comes to bypass packets as there
 * is no specific support. Bottom line is this routine does now even know what a
 * bypass packet is.
 */
void snoop_recv_handler(struct hfi_packet *packet)
{
	struct qib_pportdata *ppd = packet->rcd->ppd;
	struct qib_ib_header *hdr = packet->hdr;
	int header_size = packet->hlen;
	void *data = packet->ebuf;
	u32 tlen = packet->tlen;
	struct snoop_packet *s_packet = NULL;
	int ret;
	int snoop_mode = 0;
	u32 md_len = 0;
	struct capture_md md;

	snoop_dbg("PACKET IN: hdr size %d tlen %d data %p\n", header_size, tlen,
		  data);

#ifdef SNOOP_RING_TRACE
	trace_snoop_capture(ppd->dd, header_size, hdr, tlen - header_size,
			    data);
#endif

	if (!ppd->dd->hfi_snoop.filter_callback) {
		snoop_dbg("filter not set\n");
		ret = HFI_FILTER_HIT;
	} else {
		ret = ppd->dd->hfi_snoop.filter_callback(hdr, data,
					ppd->dd->hfi_snoop.filter_value);
	}

	switch (ret) {
	case HFI_FILTER_ERR:
		snoop_dbg("Error in filter call\n");
		break;
	case HFI_FILTER_MISS:
		snoop_dbg("Filter Miss\n");
		break;
	case HFI_FILTER_HIT:

		if (ppd->dd->hfi_snoop.mode_flag & HFI_PORT_SNOOP_MODE)
			snoop_mode = 1;
		else
			md_len = sizeof(struct capture_md);


		s_packet = allocate_snoop_packet(header_size,
						 tlen - header_size,
						 md_len);
		if (unlikely(s_packet == NULL)) {
			dd_dev_err(ppd->dd,
				"Unable to allocate snoop/capture packet\n");
			break;
		}

		if (!snoop_mode) {
			md.port = 1;
			md.dir = PKT_DIR_INGRESS;
			md.u.rhf = packet->rhf;
			memcpy(s_packet->data, &md, md_len);
		}

		/* We should always have a header */
		if (hdr) {
			memcpy(s_packet->data + md_len, hdr, header_size);
		} else {
			dd_dev_err(ppd->dd, "Unable to copy header to snoop/capture packet\n");
			kfree(s_packet);
			break;
		}

		/*
		 * Packets with no data are possible. If there is no data needed
		 * to take care of the last 4 bytes which are normally included
		 * with data buffers and are included in tlen.  Since we kzalloc
		 * the buffer we do not need to set any values but if we decide
		 * not to use kzalloc we should zero them.
		 */
		if (data)
			memcpy(s_packet->data + header_size + md_len, data,
			       tlen - header_size);

		s_packet->total_len = tlen + md_len;
		qib_snoop_list_add_tail(s_packet, ppd->dd);

		/*
		 * If we are snooping the packet not capturing then throw away
		 * after adding to the list.
		 */
		snoop_dbg("Capturing packet\n");
		if (ppd->dd->hfi_snoop.mode_flag & HFI_PORT_SNOOP_MODE) {
			snoop_dbg("Throwing packet away");
			/*
			 * If we are dropping the packet we still may need to
			 * handle the case where error flags are set, this is
			 * normally done by the type specific handler but that
			 * won't be called in this case.
			 */
			if (unlikely(rhf_err_flags(packet->rhf_addr)))
				handle_eflags(packet);

			return; /* throw the packet on the floor */
		}
		break;
	default:
		break;
	}

	/*
	 * We do not care what type of packet came in here just pass it off to
	 * its usual handler. See qib_init(). We can't just rely on calling into
	 * the function map array becaue snoop/capture has hijacked it. If we
	 * call the IB type specific handler we will be calling ourself
	 * recursively.
	 */
	switch (rhf_rcv_type(packet->rhf_addr)) {
	case RHF_RCV_TYPE_IB:
		process_receive_ib(packet);
		break;
	case RHF_RCV_TYPE_BYPASS:
		process_receive_bypass(packet);
		break;
	case RHF_RCV_TYPE_ERROR:
		process_receive_error(packet);
		break;
	case RHF_RCV_TYPE_EXPECTED:
		process_receive_expected(packet);
		break;
	case RHF_RCV_TYPE_EAGER:
		process_receive_eager(packet);
		break;
	default:
		dd_dev_err(ppd->dd, "Unknown packet type dropping!\n");
	}
}

/*
 * Handle snooping and capturing packets when sdma is being used.
 */
int snoop_send_dma_handler(struct qib_qp *qp, struct qib_ib_header *ibhdr,
			   u32 hdrwords, struct qib_sge_state *ss, u32 len,
			   u32 plen, u32 dwords, u64 pbc)
{
	pr_alert("Snooping/Capture of  Send DMA Packets Is Not Supported!\n");
	snoop_dbg("Unsupported Operation\n");
	return qib_verbs_send_dma(qp, ibhdr, hdrwords, ss, len, plen, dwords,
				  0);
}

/*
 * Handle snooping and capturing packets when pio is being used. Does not handle
 * bypass packets. The only way to send a bypass packet currently is to use the
 * diagpkt interface. When that interface is enable snoop/capture is not.
 */
int snoop_send_pio_handler(struct qib_qp *qp, struct qib_ib_header *ibhdr,
			   u32 hdrwords, struct qib_sge_state *ss, u32 len,
			   u32 plen, u32 dwords, u64 pbc)
{
	struct qib_ibport *ibp = to_iport(qp->ibqp.device, qp->port_num);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	struct snoop_packet *s_packet = NULL;
	u32 *hdr = (u32 *) ibhdr;
	u32 length = 0;
	struct qib_sge_state temp_ss;
	struct qib_sge temp_sge;
	struct qib_sge *sge;
	void *data = NULL;
	void *data_start = NULL;
	int ret;
	int snoop_mode = 0;
	int md_len = 0;
	struct capture_md md;
	u32 vl;
	u32 hdr_len = hdrwords << 2;
	u32 tlen = HFI_GET_PKT_LEN(ibhdr);

	md.u.pbc = 0;

	snoop_dbg("PACKET OUT: hdrword %u len %u plen %u dwords %u lrh[2] %u\n",
		 hdrwords, len, plen, dwords, hdr_len);

	if (ppd->dd->hfi_snoop.mode_flag & HFI_PORT_SNOOP_MODE)
		snoop_mode = 1;
	else
		md_len = sizeof(struct capture_md);

	/* not using ss->total_len as arg 2 b/c that does not count CRC */
	s_packet = allocate_snoop_packet(hdr_len, tlen - hdr_len, md_len);
	s_packet->total_len = tlen + md_len;
	if (unlikely(s_packet == NULL)) {
		dd_dev_err(ppd->dd,
			   "Unable to allocate snoop/capture packet\n");
		goto out;
	}

	if (!snoop_mode) {
		md.port = 1;
		md.dir = PKT_DIR_EGRESS;
		if (likely(pbc == 0)) {
			vl = be16_to_cpu(ibhdr->lrh[0]) >> 12;
			md.u.pbc = create_pbc(0, qp->s_srate, vl, plen);
		} else {
			md.u.pbc = 0;
		}
		memcpy(s_packet->data, &md, md_len);
	} else {
		md.u.pbc = pbc;
	}

	/* Copy header */
	if (likely(hdr)) {
		memcpy(s_packet->data + md_len, hdr, hdr_len);
	} else {
		dd_dev_err(ppd->dd,
			   "Unable to copy header to snoop/capture packet\n");
		kfree(s_packet);
		goto out;
	}

	if (ss) {
		/*
		 * Copy payload, basically a nondestructive version of
		 * qib_from_from_sge() copy sges and work on them instead of the
		 * actual sges.
		 */
		data = s_packet->data + hdr_len + md_len;
		data_start = data;
		length = ss->total_len;
		memcpy(&temp_ss, ss, sizeof(struct qib_sge_state));
		memcpy(&temp_sge, &temp_ss.sge, sizeof(struct qib_sge));
		sge = &temp_sge;

		while (length) {
			u32 len = sge->length;
			if (len > length)
				len = length;
			if (len > sge->sge_length)
				len = sge->sge_length;
			BUG_ON(len == 0);
			memcpy(data, sge->vaddr, len);
			sge->vaddr += len;
			sge->length -= len;
			sge->sge_length -= len;
			if (sge->sge_length == 0) {
				if (--temp_ss.num_sge)
					temp_ss.sg_list++;
				memcpy(&temp_sge, &temp_ss.sg_list,
					sizeof(struct qib_sge));
				sge = &temp_sge;
			} else if (sge->length == 0 && sge->mr->lkey) {
				if (++sge->n >= QIB_SEGSZ) {
					if (++sge->m >= sge->mr->mapsz)
						break;
					sge->n = 0;
				}
				sge->vaddr =
				    sge->mr->map[sge->m]->segs[sge->n].vaddr;
				sge->length =
				    sge->mr->map[sge->m]->segs[sge->n].length;
			}
			data += len;
			length -= len;
		}

#ifdef SNOOP_RING_TRACE
		/*This gets the header and payload, what about the CRC?*/
		trace_snoop_capture(ppd->dd, hdr_len, ibhdr, ss->total_len,
				    data_start);
#endif
	}

	/*
	 * Why do the filter check down here? Because the event tracing has its
	 * own filtering and we need to have the walked the SGE list.
	 */
	if (!ppd->dd->hfi_snoop.filter_callback) {
		snoop_dbg("filter not set\n");
		ret = HFI_FILTER_HIT;
	} else {
		ret = ppd->dd->hfi_snoop.filter_callback(
					ibhdr,
					NULL,
					ppd->dd->hfi_snoop.filter_value);
	}

	switch (ret) {
	case HFI_FILTER_ERR:
		snoop_dbg("Error in filter call\n");
		/* fall through */
	case HFI_FILTER_MISS:
		snoop_dbg("Filter Miss\n");
		kfree(s_packet);
		break;
	case HFI_FILTER_HIT:
		snoop_dbg("Capturing packet\n");
		qib_snoop_list_add_tail(s_packet, ppd->dd);

		if (unlikely(snoop_drop_send &&
			     (ppd->dd->hfi_snoop.mode_flag &
			      HFI_PORT_SNOOP_MODE))) {
			unsigned long flags;
			snoop_dbg("Dropping packet\n");
			if (qp->s_wqe) {
				spin_lock_irqsave(&qp->s_lock, flags);
				qib_send_complete(qp, qp->s_wqe, IB_WC_SUCCESS);
				spin_unlock_irqrestore(&qp->s_lock, flags);
			} else if (qp->ibqp.qp_type == IB_QPT_RC) {
				spin_lock_irqsave(&qp->s_lock, flags);
				qib_rc_send_complete(qp, ibhdr);
				spin_unlock_irqrestore(&qp->s_lock, flags);
			}
			return 0;
		}
		break;
	default:
		kfree(s_packet);
		break;
	}
out:
	return qib_verbs_send_pio(qp, ibhdr, hdrwords, ss, len, plen, dwords,
				  md.u.pbc);
}

