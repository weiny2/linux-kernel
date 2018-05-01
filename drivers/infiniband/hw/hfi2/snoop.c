/*
 * Copyright(c) 2015, 2016 Intel Corporation.
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * BSD LICENSE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Intel Corporation nor the names of its
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
 */

/*
 * This file contains support for diagnostic functions.  It is accessed by
 * opening the hfi1_diag device, normally minor number 129.  Diagnostic use
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
#include "hfi2.h"
#include "verbs/verbs.h"
#include "verbs/packet.h"
#include "trace.h"
#include "chip/fxr_linkmux_defs.h"

#undef pr_fmt
#define pr_fmt(fmt) DRIVER_NAME ": " fmt
#if 1
#define snoop_dbg(fmt, ...) \
	hfi2_cdbg(SNOOP, fmt, ##__VA_ARGS__)
#else
#define snoop_dbg(fmt, ...) \
	pr_debug(fmt, ##__VA_ARGS__)
#endif

extern ssize_t snoop_diagpkt_send(struct hfi_devdata *dd,
				  const char __user *data, size_t count,
				  u8 port, u64 pbc);

union rhf_gen1 {
	struct {
		u64 pktlen                  : 12;
		u64 rcvtype                 : 3;
		u64 u                       : 1;
		u64 egrindex                : 11;
		u64 sc4                     : 1;
		u64 egroffset               : 16;
		u64 hdrqoffset              : 9;
		u64 khdrlenerr              : 1;
		u64 dcerr                   : 2;
		u64 pkterr                  : 5;
		u64 reserved                : 2;
		u64 icrcerr                 : 1;
	};
	u64 val;
};

/*
 * Construct Gen1 RHF for compatibility; Gen2 RHF provides almost same content
 */
static u64 to_hfi1_rhf(u64 rhf)
{
	union rhf_gen1 out_rhf;

	out_rhf.val = 0;
	out_rhf.pktlen = rhf_pkt_len(rhf) >> 2;
	out_rhf.rcvtype = rhf_rcv_type(rhf);
	out_rhf.u = !!rhf_use_egr_bfr(rhf);
	out_rhf.egrindex = rhf_egr_index(rhf);
	out_rhf.sc4 = !!rhf_sc4(rhf);
	out_rhf.egroffset = rhf_egr_buf_offset(rhf);
	out_rhf.hdrqoffset = rhf_hdr_len(rhf) >> 2;
	out_rhf.khdrlenerr = !!(rhf_err_flags(rhf) & RHF_K_HDR_LEN_ERR);
	out_rhf.pkterr = (rhf_err_flags(rhf) >> RHF_ERROR_SHIFT);
	out_rhf.icrcerr = !!(rhf_err_flags(rhf) & RHF_ICRC_ERR);

	return out_rhf.val;
}

/* Construct Gen1 PBC: for 16B, PBC's bypass bit must be set */
static u64 to_hfi1_pbc(bool use_16b)
{
	return (use_16b) ? BIT(28) : 0;
}

/* Snoop option mask */
#define SNOOP_DROP_SEND		BIT(0)
#define SNOOP_USE_METADATA	BIT(1)

static u8 snoop_flags;

/*
 * Extract packet length from LRH header.
 * This is in Dwords so multiply by 4 to get size in bytes
 */
#define HFI1_GET_PKT_LEN(x)      (((be16_to_cpu((x)->lrh[2]) & 0xFFF)) << 2)

enum hfi1_filter_status {
	HFI1_FILTER_HIT,
	HFI1_FILTER_ERR,
	HFI1_FILTER_MISS
};

static void snoop_invalid_handler(struct hfi2_ib_packet *packet)
{
	struct hfi2_ibdev *ibd = packet->ibp->ibd;

	/* take no action except call normal handler (if any) */
	if (ibd->rhf_rcv_functions[packet->etype])
		ibd->rhf_rcv_functions[packet->etype](packet);
}

static void snoop_recv_handler(struct hfi2_ib_packet *packet);

/* snoop processing functions */
static rhf_rcv_function_ptr snoop_rhf_rcv_functions[HFI2_RHF_RCV_TYPES] = {
	[RHF_RCV_TYPE_EXPECTED] = snoop_recv_handler,
	[RHF_RCV_TYPE_EAGER]    = snoop_recv_handler,
	[RHF_RCV_TYPE_IB]       = snoop_recv_handler,
	[RHF_RCV_TYPE_ERROR]    = snoop_recv_handler,
	[RHF_RCV_TYPE_BYPASS]   = snoop_recv_handler,
	[RHF_RCV_TYPE_INVALID5] = snoop_invalid_handler,
	[RHF_RCV_TYPE_INVALID6] = snoop_invalid_handler,
	[RHF_RCV_TYPE_INVALID7] = snoop_invalid_handler
};

/* Snoop packet structure */
struct snoop_packet {
	struct list_head list;
	u32 total_len;
	u8 data[];
};

static int snoop_send_wqe(struct hfi2_ibport *ibp, struct hfi2_qp_priv *priv,
			  bool allow_sleep);
static int snoop_send_ack(struct hfi2_ibport *ibp, struct hfi2_qp_priv *priv,
			  union hfi2_packet_header *from, size_t hwords);

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
 * This starts our ioctl sequence numbers *way* off from the ones
 * defined in ib_core.
 */
#define SNOOP_CAPTURE_VERSION 0x1

#define HFI1_SNOOP_IOC_MAGIC IB_IOCTL_MAGIC
#define HFI1_SNOOP_IOC_BASE_SEQ 0x80

#define HFI1_SNOOP_IOCCLEARQUEUE \
	_IO(HFI1_SNOOP_IOC_MAGIC, HFI1_SNOOP_IOC_BASE_SEQ + 2)
#define HFI1_SNOOP_IOCCLEARFILTER \
	_IO(HFI1_SNOOP_IOC_MAGIC, HFI1_SNOOP_IOC_BASE_SEQ + 3)
#define HFI1_SNOOP_IOCSETFILTER \
	_IO(HFI1_SNOOP_IOC_MAGIC, HFI1_SNOOP_IOC_BASE_SEQ + 4)
#define HFI1_SNOOP_IOCGETVERSION \
	_IO(HFI1_SNOOP_IOC_MAGIC, HFI1_SNOOP_IOC_BASE_SEQ + 5)
#define HFI1_SNOOP_IOCSET_OPTS \
	_IO(HFI1_SNOOP_IOC_MAGIC, HFI1_SNOOP_IOC_BASE_SEQ + 6)

static int hfi1_snoop_open(struct inode *in, struct file *fp);
static ssize_t hfi1_snoop_read(struct file *fp, char __user *data,
			       size_t pkt_len, loff_t *off);
static ssize_t hfi1_snoop_write(struct file *fp, const char __user *data,
				size_t count, loff_t *off);
static long hfi1_ioctl(struct file *fp, unsigned int cmd, unsigned long arg);
static unsigned int hfi1_snoop_poll(struct file *fp,
				    struct poll_table_struct *wait);
static int hfi1_snoop_release(struct inode *in, struct file *fp);

struct hfi1_packet_filter_command {
	int opcode;
	int length;
	void *value_ptr;
};

/* Can't re-use PKT_DIR_*GRESS here because 0 means no packets for this */
#define HFI1_SNOOP_INGRESS 0x1
#define HFI1_SNOOP_EGRESS  0x2

enum hfi1_packet_filter_opcodes {
	FILTER_BY_LID,
	FILTER_BY_DLID,
	FILTER_BY_MAD_MGMT_CLASS,
	FILTER_BY_QP_NUMBER,
	FILTER_BY_PKT_TYPE,
	FILTER_BY_SERVICE_LEVEL,
	FILTER_BY_PKEY,
	FILTER_BY_DIRECTION,
};

static const struct file_operations snoop_file_ops = {
	.owner = THIS_MODULE,
	.open = hfi1_snoop_open,
	.read = hfi1_snoop_read,
	.unlocked_ioctl = hfi1_ioctl,
	.poll = hfi1_snoop_poll,
	.write = hfi1_snoop_write,
	.release = hfi1_snoop_release
};

struct hfi1_filter_array {
	int (*filter)(void *, void *, void *, bool);
};

static int hfi1_filter_lid(void *ibhdr, void *packet_data,
			   void *value, bool bypass);
static int hfi1_filter_dlid(void *ibhdr, void *packet_data,
			    void *value, bool bypass);
static int hfi1_filter_mad_mgmt_class(void *ibhdr, void *packet_data,
				      void *value, bool bypass);
static int hfi1_filter_qp_number(void *ibhdr, void *packet_data,
				 void *value, bool bypass);
static int hfi1_filter_ibpacket_type(void *ibhdr, void *packet_data,
				     void *value, bool bypass);
static int hfi1_filter_ib_service_level(void *ibhdr, void *packet_data,
					void *value, bool bypass);
static int hfi1_filter_ib_pkey(void *ibhdr, void *packet_data,
			       void *value, bool bypass);
static int hfi1_filter_direction(void *ibhdr, void *packet_data,
				 void *value, bool bypass);

static const struct hfi1_filter_array hfi1_filters[] = {
	{ hfi1_filter_lid },
	{ hfi1_filter_dlid },
	{ hfi1_filter_mad_mgmt_class },
	{ hfi1_filter_qp_number },
	{ hfi1_filter_ibpacket_type },
	{ hfi1_filter_ib_service_level },
	{ hfi1_filter_ib_pkey },
	{ hfi1_filter_direction },
};

#define HFI1_MAX_FILTERS	ARRAY_SIZE(hfi1_filters)

static struct ib_other_headers *get_ohdr(union hfi2_packet_header *pkt_hdr,
					 bool bypass)
{
	if (bypass) {
		struct hfi2_16b_header *hdr = (struct hfi2_16b_header *)pkt_hdr;

		u8 l4 = hfi2_16B_get_l4(hdr);

		if (l4 == HFI1_L4_IB_LOCAL)
			return &hdr->u.oth;
		else
			return &hdr->u.l.oth;
	} else {
		struct ib_header *hdr = (struct ib_header *)pkt_hdr;
		/* Check for GRH */
		if ((be16_to_cpu(hdr->lrh[0]) & 3) == HFI1_LRH_BTH)
			return &hdr->u.oth; /* LRH + BTH + DETH */
		else
			return &hdr->u.l.oth; /* LRH + GRH + BTH + DETH */
	}
}

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
	mutex_lock(&dd->hfi1_snoop.snoop_lock);
	drain_snoop_list(&dd->hfi1_snoop.queue);
	misc_deregister(&dd->hfi1_snoop.miscdev);
	mutex_unlock(&dd->hfi1_snoop.snoop_lock);
}

int hfi_snoop_add(struct hfi_devdata *dd)
{
	char name[16];
	int ret = 0;
	struct miscdevice *mdev;

	snprintf(name, sizeof(name), "%s_diagpkt%d", DRIVER_NAME,
		 dd->unit);

	dd->hfi1_snoop.mode_flag = 0;
	mutex_init(&dd->hfi1_snoop.snoop_lock);
	INIT_LIST_HEAD(&dd->hfi1_snoop.queue);
	init_waitqueue_head(&dd->hfi1_snoop.waitq);

	mdev = &dd->hfi1_snoop.miscdev;
	mdev->minor = MISC_DYNAMIC_MINOR;
	mdev->name = name,
	mdev->fops = &snoop_file_ops,
	mdev->parent = &dd->pdev->dev;

	ret = misc_register(mdev);
	if (ret)
		dd_dev_err(dd, "Unable to create snoop/capture device: %d",
			   ret);

	return ret;
}

static int hfi1_snoop_open(struct inode *in, struct file *fp)
{
	int ret;
	int mode_flag = 0;
	struct list_head *queue;
	struct hfi_devdata *dd = container_of(fp->private_data,
					      struct hfi_devdata, hfi1_snoop);

	/*
	 * File mode determines snoop or capture. Some existing user
	 * applications expect the capture device to be able to be opened RDWR
	 * because they expect a dedicated capture device. For this reason we
	 * support a module param to force capture mode even if the file open
	 * mode matches snoop.
	 */
	if ((fp->f_flags & O_ACCMODE) == O_RDONLY) {
		snoop_dbg("Capture Enabled");
		mode_flag = HFI1_PORT_CAPTURE_MODE;
	} else if ((fp->f_flags & O_ACCMODE) == O_RDWR) {
		snoop_dbg("Snoop Enabled");
		mode_flag = HFI1_PORT_SNOOP_MODE;
	} else {
		snoop_dbg("Invalid");
		ret =  -EINVAL;
		goto bail;
	}

	/*
	 * We are not supporting snoop and capture at the same time.
	 */
	mutex_lock(&dd->hfi1_snoop.snoop_lock);
	if (dd->hfi1_snoop.mode_flag) {
		ret = -EBUSY;
		mutex_unlock(&dd->hfi1_snoop.snoop_lock);
		goto bail;
	}

	dd->hfi1_snoop.mode_flag = mode_flag;
	queue = &dd->hfi1_snoop.queue;
	drain_snoop_list(queue);

	dd->hfi1_snoop.filter_callback = NULL;
	dd->hfi1_snoop.filter_value = NULL;

	if (mode_flag == HFI1_PORT_SNOOP_MODE) {
		u64 reg;
		/*
		 * We do not want to be doing the DLID LMC check for
		 * ingressed packets.
		 */
		reg = read_csr(dd, FXR_LM_CFG_PORT0);

		dd->hfi1_snoop.lm_cfg = reg;

		reg |= FXR_LM_CFG_PORT0_LMC_ENABLE_SMASK |
				FXR_LM_CFG_PORT0_LMC_WIDTH_SMASK;
		write_csr(dd, FXR_LM_CFG_PORT0, reg);

		reg = read_csr(dd, FXR_LM_CFG_CONFIG);

		reg |= FXR_LM_CFG_CONFIG_NEAR_LOOPBACK_DISABLE_SMASK;

		write_csr(dd, FXR_LM_CFG_CONFIG, reg);
	}

	/*
	 * As soon as we set these function pointers the recv and send handlers
	 * are active. This is a race condition so we must make sure to drain
	 * the queue and init filter values above. Technically we should add
	 * locking here but all that will happen is on recv a packet will get
	 * allocated and get stuck on the snoop_lock before getting added to the
	 * queue. Same goes for send.
	 */
	dd->ibd->rhf_rcv_function_map = snoop_rhf_rcv_functions;
	dd->ibd->send_wqe = snoop_send_wqe;
	dd->ibd->send_ack = snoop_send_ack;

	mutex_unlock(&dd->hfi1_snoop.snoop_lock);
	ret = 0;
	fp->private_data = dd;

bail:
	return ret;
}

static int hfi1_snoop_release(struct inode *in, struct file *fp)
{
	struct hfi_devdata *dd = fp->private_data;
	int mode_flag;

	mutex_lock(&dd->hfi1_snoop.snoop_lock);

	/* clear the snoop mode before re-adjusting send context CSRs */
	mode_flag = dd->hfi1_snoop.mode_flag;
	dd->hfi1_snoop.mode_flag = 0;

	/* Enable original DLID settings */
	if (mode_flag == HFI1_PORT_SNOOP_MODE) {
		u64 reg;

		reg = read_csr(dd, FXR_LM_CFG_CONFIG);

		reg &= ~FXR_LM_CFG_CONFIG_NEAR_LOOPBACK_DISABLE_SMASK;
		write_csr(dd, FXR_LM_CFG_CONFIG, reg);

		write_csr(dd, FXR_LM_CFG_PORT0, dd->hfi1_snoop.lm_cfg);
	}

	/* Drain the queue and clear the filters we are done with it. */
	drain_snoop_list(&dd->hfi1_snoop.queue);

	dd->hfi1_snoop.filter_callback = NULL;
	kfree(dd->hfi1_snoop.filter_value);
	dd->hfi1_snoop.filter_value = NULL;

	/*
	 * User is done snooping and capturing, return control to the normal
	 * handler.
	 */
	dd->ibd->rhf_rcv_function_map = dd->ibd->rhf_rcv_functions;
	dd->ibd->send_wqe = hfi2_send_wqe;
	dd->ibd->send_ack = hfi2_send_ack;

	mutex_unlock(&dd->hfi1_snoop.snoop_lock);
	snoop_dbg("snoop/capture device released");

	return 0;
}

static unsigned int hfi1_snoop_poll(struct file *fp,
				    struct poll_table_struct *wait)
{
	int ret = 0;
	struct hfi_devdata *dd = fp->private_data;

	mutex_lock(&dd->hfi1_snoop.snoop_lock);

	poll_wait(fp, &dd->hfi1_snoop.waitq, wait);
	if (!list_empty(&dd->hfi1_snoop.queue))
		ret |= POLLIN | POLLRDNORM;

	mutex_unlock(&dd->hfi1_snoop.snoop_lock);
	return ret;
}

static ssize_t hfi1_snoop_write(struct file *fp, const char __user *data,
				size_t count, loff_t *off)
{
	struct hfi_devdata *dd = fp->private_data;
	u64 pbc = 0;
	u8 port = 1;
	int ret;

	snoop_dbg("received %lu bytes from user", count);

	if (snoop_flags & SNOOP_USE_METADATA) {
		/* read user-provided PBC */
		ret = get_user(pbc, (u64 __user *)data);
		if (ret < 0)
			return ret;
		count -= sizeof(pbc);
		data += sizeof(pbc);
	}

	ret = snoop_diagpkt_send(dd, data, count, port, pbc);
	if (ret < 0)
		return ret;
	return count;
}

static ssize_t hfi1_snoop_read(struct file *fp, char __user *data,
			       size_t pkt_len, loff_t *off)
{
	ssize_t ret = 0;
	struct snoop_packet *packet = NULL;
	struct hfi_devdata *dd = fp->private_data;

	mutex_lock(&dd->hfi1_snoop.snoop_lock);

	while (list_empty(&dd->hfi1_snoop.queue)) {
		mutex_unlock(&dd->hfi1_snoop.snoop_lock);

		if (fp->f_flags & O_NONBLOCK)
			return -EAGAIN;

		if (wait_event_interruptible(
				dd->hfi1_snoop.waitq,
				!list_empty(&dd->hfi1_snoop.queue)))
			return -EINTR;

		mutex_lock(&dd->hfi1_snoop.snoop_lock);
	}

	if (!list_empty(&dd->hfi1_snoop.queue)) {
		packet = list_entry(dd->hfi1_snoop.queue.next,
				    struct snoop_packet, list);
		list_del(&packet->list);
		mutex_unlock(&dd->hfi1_snoop.snoop_lock);
		if (pkt_len >= packet->total_len) {
			if (copy_to_user(data, packet->data,
					 packet->total_len))
				ret = -EFAULT;
			else
				ret = packet->total_len;
		} else {
			ret = -EINVAL;
		}

		kfree(packet);
	} else {
		mutex_unlock(&dd->hfi1_snoop.snoop_lock);
	}

	return ret;
}

static long hfi1_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
	struct hfi_devdata *dd = fp->private_data;
	void *filter_value = NULL;
	long ret = 0;
	int value = 0;
	unsigned long *argp = NULL;
	struct hfi1_packet_filter_command filter_cmd = {0};
	int mode_flag = 0;
	int read_cmd, write_cmd, read_ok, write_ok;

	mode_flag = dd->hfi1_snoop.mode_flag;
	read_cmd = _IOC_DIR(cmd) & _IOC_READ;
	write_cmd = _IOC_DIR(cmd) & _IOC_WRITE;
	write_ok = access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	read_ok = access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));

	if ((read_cmd && !write_ok) || (write_cmd && !read_ok)) {
		ret = -EFAULT;
	} else if (!capable(CAP_SYS_ADMIN)) {
		ret = -EPERM;
	} else if ((mode_flag & HFI1_PORT_CAPTURE_MODE) &&
		   (cmd != HFI1_SNOOP_IOCCLEARQUEUE) &&
		   (cmd != HFI1_SNOOP_IOCCLEARFILTER) &&
		   (cmd != HFI1_SNOOP_IOCSETFILTER)) {
		/*
		 * Capture devices are allowed only 3 operations
		 * 1.Clear capture queue
		 * 2.Clear capture filter
		 * 3.Set capture filter
		 * Others are invalid.
		 */
		return -EINVAL;
	}

	switch (cmd) {
	case HFI1_SNOOP_IOCCLEARQUEUE:
		snoop_dbg("Clearing snoop queue");
		mutex_lock(&dd->hfi1_snoop.snoop_lock);
		drain_snoop_list(&dd->hfi1_snoop.queue);
		mutex_unlock(&dd->hfi1_snoop.snoop_lock);
		break;

	case HFI1_SNOOP_IOCCLEARFILTER:
		snoop_dbg("Clearing filter");
		mutex_lock(&dd->hfi1_snoop.snoop_lock);
		if (dd->hfi1_snoop.filter_callback) {
			/* Drain packets first */
			drain_snoop_list(&dd->hfi1_snoop.queue);
			dd->hfi1_snoop.filter_callback = NULL;
		}
		kfree(dd->hfi1_snoop.filter_value);
		dd->hfi1_snoop.filter_value = NULL;
		mutex_unlock(&dd->hfi1_snoop.snoop_lock);
		break;

	case HFI1_SNOOP_IOCSETFILTER:
		snoop_dbg("Setting filter");
		/* just copy command structure */
		argp = (unsigned long *)arg;
		if (copy_from_user(&filter_cmd, (void __user *)argp,
				   sizeof(filter_cmd)))
			return -EFAULT;

		if (filter_cmd.opcode >= HFI1_MAX_FILTERS)
			return -EINVAL;

		snoop_dbg("Opcode %d Len %d Ptr %p",
			  filter_cmd.opcode, filter_cmd.length,
			  filter_cmd.value_ptr);

		filter_value = kzalloc(filter_cmd.length, GFP_KERNEL);
		if (!filter_value)
			return -ENOMEM;

		/* copy remaining data from userspace */
		if (copy_from_user((u8 *)filter_value,
				   (void __user *)filter_cmd.value_ptr,
				   filter_cmd.length)) {
			kfree(filter_value);
			return -EFAULT;
		}

		/* Drain packets first */
		mutex_lock(&dd->hfi1_snoop.snoop_lock);
		drain_snoop_list(&dd->hfi1_snoop.queue);
		dd->hfi1_snoop.filter_callback =
			hfi1_filters[filter_cmd.opcode].filter;
		/* just in case we see back to back sets */
		kfree(dd->hfi1_snoop.filter_value);
		dd->hfi1_snoop.filter_value = filter_value;
		mutex_unlock(&dd->hfi1_snoop.snoop_lock);
		break;
	case HFI1_SNOOP_IOCGETVERSION:
		value = SNOOP_CAPTURE_VERSION;
		snoop_dbg("Getting version: %d", value);
		return __put_user(value, (int __user *)arg);
	case HFI1_SNOOP_IOCSET_OPTS:
		snoop_flags = 0;
		ret = __get_user(value, (int __user *)arg);
		if (ret != 0)
			break;

		snoop_dbg("Setting snoop option %d", value);
		if (value & SNOOP_DROP_SEND)
			snoop_flags |= SNOOP_DROP_SEND;
		if (value & SNOOP_USE_METADATA)
			snoop_flags |= SNOOP_USE_METADATA;
		break;
	default:
		ret = -ENOTTY;
		break;
	}

	return ret;
}

static void snoop_list_add_tail(struct snoop_packet *packet,
				struct hfi_devdata *dd)
{
	mutex_lock(&dd->hfi1_snoop.snoop_lock);
	if (likely((dd->hfi1_snoop.mode_flag & HFI1_PORT_SNOOP_MODE) ||
		   (dd->hfi1_snoop.mode_flag & HFI1_PORT_CAPTURE_MODE))) {
		list_add_tail(&packet->list, &dd->hfi1_snoop.queue);
		snoop_dbg("Added packet to list");
	}

	/*
	 * Technically we can could have closed the snoop device while waiting
	 * on the above lock and it is gone now. The snoop mode_flag will
	 * prevent us from adding the packet to the queue though.
	 */

	mutex_unlock(&dd->hfi1_snoop.snoop_lock);
	wake_up_interruptible(&dd->hfi1_snoop.waitq);
}

static inline int hfi1_filter_check(void *val, const char *msg)
{
	if (!val) {
		snoop_dbg("Error invalid %s value for filter", msg);
		return HFI1_FILTER_ERR;
	}
	return 0;
}

static int hfi1_filter_lid(void *ibhdr, void *packet_data,
			   void *value, bool bypass)
{
	int ret;
	u32 slid;

	ret = hfi1_filter_check(ibhdr, "header");
	if (ret)
		return ret;
	ret = hfi1_filter_check(value, "user");
	if (ret)
		return ret;

	if (bypass) {
		struct hfi2_16b_header *hdr = (struct hfi2_16b_header *)ibhdr;

		slid = hfi2_16B_get_slid(hdr);
	} else {
		struct ib_header *hdr = (struct ib_header *)ibhdr;

		slid = ib_get_slid(hdr);
	}

	if (*((u32 *)value) == slid) /* matches slid */
		return HFI1_FILTER_HIT; /* matched */

	return HFI1_FILTER_MISS; /* Not matched */
}

static int hfi1_filter_dlid(void *ibhdr, void *packet_data,
			    void *value, bool bypass)
{
	int ret;
	u32 dlid;

	ret = hfi1_filter_check(ibhdr, "header");
	if (ret)
		return ret;
	ret = hfi1_filter_check(value, "user");
	if (ret)
		return ret;
	if (bypass) {
		struct hfi2_16b_header *hdr = (struct hfi2_16b_header *)ibhdr;

		dlid = hfi2_16B_get_dlid(hdr);
	} else {
		struct ib_header *hdr = (struct ib_header *)ibhdr;

		dlid = ib_get_dlid(hdr);
	}

	if (*((u32 *)value) == dlid)
		return HFI1_FILTER_HIT;

	return HFI1_FILTER_MISS;
}

/* Not valid for outgoing packets, send handler passes null for data*/
static int hfi1_filter_mad_mgmt_class(void *ibhdr, void *packet_data,
				      void *value, bool bypass)
{
	struct ib_smp *smp = NULL;
	u32 qpn = 0;
	union hfi2_packet_header *hdr = (union hfi2_packet_header *)ibhdr;
	struct ib_other_headers *ohdr = get_ohdr(hdr, bypass);
	int ret;

	ret = hfi1_filter_check(ibhdr, "header");
	if (ret)
		return ret;
	ret = hfi1_filter_check(packet_data, "packet_data");
	if (ret)
		return ret;
	ret = hfi1_filter_check(value, "user");
	if (ret)
		return ret;

	qpn = be32_to_cpu(ohdr->bth[1]) & 0x00FFFFFF;

	if (qpn <= 1) {
		smp = (struct ib_smp *)packet_data;
		if (*((u8 *)value) == smp->mgmt_class)
			return HFI1_FILTER_HIT;
		else
			return HFI1_FILTER_MISS;
	}
	return HFI1_FILTER_ERR;
}

static int hfi1_filter_qp_number(void *ibhdr, void *packet_data,
				 void *value, bool bypass)
{
	u32 qpn = 0;
	union hfi2_packet_header *hdr = (union hfi2_packet_header *)ibhdr;
	struct ib_other_headers *ohdr = get_ohdr(hdr, bypass);
	int ret;

	ret = hfi1_filter_check(ibhdr, "header");
	if (ret)
		return ret;
	ret = hfi1_filter_check(value, "user");
	if (ret)
		return ret;

	qpn = be32_to_cpu(ohdr->bth[1]) & 0x00FFFFFF;

	if (*((u32 *)value) == qpn)
		return HFI1_FILTER_HIT;

	return HFI1_FILTER_MISS;
}

static int hfi1_filter_ibpacket_type(void *ibhdr, void *packet_data,
				     void *value, bool bypass)
{
	u8 opcode = 0;
	union hfi2_packet_header *hdr = (union hfi2_packet_header *)ibhdr;
	struct ib_other_headers *ohdr = get_ohdr(hdr, bypass);
	int ret;

	ret = hfi1_filter_check(ibhdr, "header");
	if (ret)
		return ret;
	ret = hfi1_filter_check(value, "user");
	if (ret)
		return ret;

	opcode = be32_to_cpu(ohdr->bth[0]) >> 24;

	if (*((u8 *)value) == ((opcode >> 5) & 0x7))
		return HFI1_FILTER_HIT;

	return HFI1_FILTER_MISS;
}

static int hfi1_filter_ib_service_level(void *ibhdr, void *packet_data,
					void *value, bool bypass)
{
	struct ib_header *hdr;
	int ret;

	ret = hfi1_filter_check(ibhdr, "header");
	if (ret)
		return ret;
	ret = hfi1_filter_check(value, "user");
	if (ret)
		return ret;

	/* Bypass packets do not have a SL field */
	if (bypass)
		return HFI1_FILTER_MISS;

	hdr = (struct ib_header *)ibhdr;

	if ((*((u8 *)value)) == ((be16_to_cpu(hdr->lrh[0]) >> 4) & 0xF))
		return HFI1_FILTER_HIT;

	return HFI1_FILTER_MISS;
}

static int hfi1_filter_ib_pkey(void *ibhdr, void *packet_data,
			       void *value, bool bypass)
{
	u32 pkey = 0;
	union hfi2_packet_header *hdr = (union hfi2_packet_header *)ibhdr;
	struct ib_other_headers *ohdr = get_ohdr(hdr, bypass);
	int ret;

	ret = hfi1_filter_check(ibhdr, "header");
	if (ret)
		return ret;
	ret = hfi1_filter_check(value, "user");
	if (ret)
		return ret;

	pkey = be32_to_cpu(ohdr->bth[0]) & 0xffff;

	/* P_key is 16-bit entity, however top most bit indicates
	 * type of membership. 0 for limited and 1 for Full.
	 * Limited members cannot accept information from other
	 * Limited members, but communication is allowed between
	 * every other combination of membership.
	 * Hence we'll omit comparing top-most bit while filtering
	 */
	if ((*(u16 *)value & 0x7fff) == (pkey & 0x7fff))
		return HFI1_FILTER_HIT;

	return HFI1_FILTER_MISS;
}

/*
 * If packet_data is NULL then this is coming from one of the send functions.
 * Thus we know if its an ingressed or egressed packet.
 */
static int hfi1_filter_direction(void *ibhdr, void *packet_data,
				 void *value, bool bypass)
{
	u8 user_dir = *(u8 *)value;
	int ret;

	ret = hfi1_filter_check(value, "user");
	if (ret)
		return ret;

	if (packet_data) {
		/* Incoming packet */
		if (user_dir & HFI1_SNOOP_INGRESS)
			return HFI1_FILTER_HIT;
	} else {
		/* Outgoing packet */
		if (user_dir & HFI1_SNOOP_EGRESS)
			return HFI1_FILTER_HIT;
	}

	return HFI1_FILTER_MISS;
}

/*
 * Allocate a snoop packet. The structure that is stored in the ring buffer, not
 * to be confused with an hfi packet type.
 */
static struct snoop_packet *allocate_snoop_packet(u32 length)
{
	struct snoop_packet *packet = NULL;

	packet = kzalloc(sizeof(*packet) + length,
			 GFP_ATOMIC | __GFP_NOWARN);
	if (likely(packet))
		INIT_LIST_HEAD(&packet->list);

	return packet;
}

/*
 * Instead of having snoop and capture code intermixed with the recv functions,
 * both the interrupt handler and hfi2_ib_rcv() we are going to hijack the call
 * and land in here for snoop/capture but if not enabled the call will go
 * through as before. This gives us a single point to constrain all of the snoop
 * snoop recv logic. There is nothing special that needs to happen for bypass
 * packets. This routine should not try to look into the packet. It just copied
 * it. There is no guarantee for filters when it comes to bypass packets as
 * there is no specific support. Bottom line is this routine does now even know
 * what a bypass packet is.
 */
static void snoop_recv_handler(struct hfi2_ib_packet *packet)
{
	struct hfi2_ibport *ibp = packet->ibp;
	struct hfi2_ibdev *ibd = ibp->ibd;
	struct hfi_devdata *dd = ibd->dd;
	union hfi2_packet_header *hdr = packet->hdr;
	u8 *pkt_header = (u8 *)hdr;
	int header_size = packet->hlen;
	void *data = packet->payload;
	u32 tlen = packet->tlen;
	struct snoop_packet *s_packet = NULL;
	int ret;
	int snoop_mode = 0;
	u32 md_len = 0;
	struct capture_md md;
	bool bypass = (packet->etype == RHF_RCV_TYPE_BYPASS);

	if (bypass) {
		/*
		 * Since bypass packet header is split across
		 * packet->hdr and packet->ebuf, a header copy is needed
		 * here in order to have the snoop and filter functions
		 * work properly. Since snoop is not in the fast path,
		 * this copy might be acceptable.
		 */
		memcpy(pkt_header + 16, packet->ebuf, header_size - 16);
	}

	snoop_dbg("PACKET IN: hdr size %d tlen %d data %p", header_size, tlen,
		  data);
#if 0
	trace_snoop_capture(dd, header_size, hdr, tlen - header_size,
			    data);
#endif

	if (!dd->hfi1_snoop.filter_callback) {
		snoop_dbg("filter not set");
		ret = HFI1_FILTER_HIT;
	} else {
		ret = dd->hfi1_snoop.filter_callback(hdr, data,
					dd->hfi1_snoop.filter_value,
					bypass);
	}

	switch (ret) {
	case HFI1_FILTER_ERR:
		snoop_dbg("Error in filter call");
		break;
	case HFI1_FILTER_MISS:
		snoop_dbg("Filter Miss");
		break;
	case HFI1_FILTER_HIT:

		if (dd->hfi1_snoop.mode_flag & HFI1_PORT_SNOOP_MODE)
			snoop_mode = 1;
		if ((snoop_mode == 0) ||
		    unlikely(snoop_flags & SNOOP_USE_METADATA))
			md_len = sizeof(struct capture_md);

		/*
		 * The resultant length to be passed to
		 * allocate_snoop_packet is (header_size + (tlen -
		 * header_size + md_len) = md_len + tlen)
		 */
		s_packet = allocate_snoop_packet(tlen + md_len);

		if (unlikely(!s_packet)) {
			dd_dev_warn_ratelimited(dd, "Unable to allocate snoop/capture packet\n");
			break;
		}

		if (md_len > 0) {
			memset(&md, 0, sizeof(struct capture_md));
			md.port = packet->port + 1; /* IB ports start from 1 */
			md.dir = PKT_DIR_INGRESS;
			md.u.rhf = to_hfi1_rhf(packet->rhf);
			memcpy(s_packet->data, &md, md_len);
		}

		/* We should always have a header (not always with BYPASS) */
		if (hdr && header_size) {
			memcpy(s_packet->data + md_len, hdr, header_size);
		} else {
			dd_dev_err(dd, "Unable to copy header to snoop/capture packet\n");
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
		snoop_list_add_tail(s_packet, dd);

		/*
		 * If we are snooping the packet not capturing then throw away
		 * after adding to the list.
		 */
		snoop_dbg("Capturing packet");
		if (dd->hfi1_snoop.mode_flag & HFI1_PORT_SNOOP_MODE) {
			snoop_dbg("Throwing packet away");
#if 0
			/*
			 * If we are dropping the packet we still may need to
			 * handle the case where error flags are set, this is
			 * normally done by the type specific handler but that
			 * won't be called in this case.
			 */
			if (unlikely(rhf_err_flags(packet->rhf)))
				handle_eflags(packet);
#endif
			/* throw the packet on the floor */
			return;
		}
		break;
	default:
		break;
	}

	/*
	 * We do not care what type of packet came in here - just pass it off
	 * to the normal handler.
	 */
	ibd->rhf_rcv_functions[packet->etype](packet);
}

/*
 * Handle snooping and capturing 9B and 16B IB packets.
 */
static int snoop_send_wqe(struct hfi2_ibport *ibp, struct hfi2_qp_priv *priv,
			  bool allow_sleep)
{
	struct rvt_qp *qp = priv->owner;
	u32 hdrwords = qp->s_hdrwords;
	struct rvt_sge_state *ss = qp->s_cur_sge;
	u32 len = qp->s_cur_size;
	u32 dwords = (len + 3) >> 2; /* debug only */
	struct hfi_devdata *dd = ibp->ibd->dd;
	struct snoop_packet *s_packet = NULL;
	void *hdr;
	u32 length = 0;
	struct rvt_sge_state temp_ss;
	void *data = NULL;
	void *data_start = NULL;
	int ret;
	int snoop_mode = 0;
	int md_len = 0;
	struct capture_md md;
	u32 hdr_len = hdrwords << 2;
	u32 tlen;
	bool use_16b = priv->hdr_type;

	if (!use_16b) {
		tlen = HFI1_GET_PKT_LEN(&priv->s_hdr->ph.ibh);
		hdr = &priv->s_hdr->ph.ibh;
	} else {
		hdr = &priv->s_hdr->opah;
		/* tlen should be in bytes */
		tlen = hfi2_16B_get_pkt_len((struct hfi2_16b_header *)hdr) << 3;
	}

	snoop_dbg("PACKET OUT: hdrword %u len %u plen %u dwords %u tlen %u",
		  hdrwords, len, hdrwords + dwords, dwords, tlen);
	if (dd->hfi1_snoop.mode_flag & HFI1_PORT_SNOOP_MODE)
		snoop_mode = 1;
	if ((snoop_mode == 0) ||
	    unlikely(snoop_flags & SNOOP_USE_METADATA))
		md_len = sizeof(struct capture_md);

	/* not using ss->total_len as arg 2 b/c that does not count CRC */
	/*
	 * The resultant length to be passed to
	 * allocate_snoop_packet is (hdr_len + (tlen -
	 * hdr_len + md_len) = md_len + tlen)
	 */
	s_packet = allocate_snoop_packet(tlen + md_len);

	if (unlikely(!s_packet)) {
		dd_dev_warn_ratelimited(dd, "Unable to allocate snoop/capture packet\n");
		goto out;
	}

	s_packet->total_len = tlen + md_len;

	if (md_len > 0) {
		memset(&md, 0, sizeof(struct capture_md));
		md.port = ibp->port_num + 1; /* IB ports start from 1 */
		md.dir = PKT_DIR_EGRESS;
		md.u.pbc = to_hfi1_pbc(use_16b);
		memcpy(s_packet->data, &md, md_len);
	}

	/* Copy header */
	memcpy(s_packet->data + md_len, hdr, hdr_len);

	if (ss) {
		data = s_packet->data + hdr_len + md_len;
		data_start = data;

		/*
		 * Copy SGE State
		 * The update_sge() function below will not modify the
		 * individual SGEs in the array. It will make a copy each time
		 * and operate on that. So we only need to copy this instance
		 * and it won't impact PIO.
		 */
		temp_ss = *ss;
		length = len;

		snoop_dbg("Need to copy %d bytes", length);
		while (length) {
			void *addr = temp_ss.sge.vaddr;
			u32 slen = temp_ss.sge.length;

			if (slen > length) {
				slen = length;
				snoop_dbg("slen %d > len %d", slen, length);
			}
			snoop_dbg("copy %d to %p", slen, addr);
			memcpy(data, addr, slen);
			rvt_update_sge(&temp_ss, slen, false);
			length -= slen;
			data += slen;
			snoop_dbg("data is now %p bytes left %d", data, length);
		}
		snoop_dbg("Completed SGE copy");
	}

	/*
	 * Why do the filter check down here? Because the event tracing has its
	 * own filtering and we need to have the walked the SGE list.
	 */
	if (!dd->hfi1_snoop.filter_callback) {
		snoop_dbg("filter not set\n");
		ret = HFI1_FILTER_HIT;
	} else {
		ret = dd->hfi1_snoop.filter_callback(
					hdr, NULL,
					dd->hfi1_snoop.filter_value,
					use_16b);
	}

	switch (ret) {
	case HFI1_FILTER_ERR:
		snoop_dbg("Error in filter call");
		/* fall through */
	case HFI1_FILTER_MISS:
		snoop_dbg("Filter Miss");
		kfree(s_packet);
		break;
	case HFI1_FILTER_HIT:
		snoop_dbg("Capturing packet");
		snoop_list_add_tail(s_packet, dd);

		if (unlikely((snoop_flags & SNOOP_DROP_SEND) &&
			     (dd->hfi1_snoop.mode_flag &
			      HFI1_PORT_SNOOP_MODE))) {
			unsigned long flags;

			snoop_dbg("Dropping packet");
			if (qp->ibqp.qp_type == IB_QPT_RC) {
				spin_lock_irqsave(&qp->s_lock, flags);
				hfi2_rc_send_complete(qp, priv->opcode,
						      priv->bth2);
				spin_unlock_irqrestore(&qp->s_lock, flags);
			} else if (!qp->s_len && qp->s_wqe) {
				/* !s_len means this is last packet in WQE */
				spin_lock_irqsave(&qp->s_lock, flags);
				hfi2_send_complete(
					qp, qp->s_wqe,
					IB_WC_SUCCESS);
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
	return hfi2_send_wqe(ibp, priv, allow_sleep);
}

/*
 * Callers of this must pass a ib_header type for the from ptr. Currently
 * this can be used anywhere, but the intention is for inline ACKs for RC and
 * CCA packets. We don't restrict this usage though.
 */
static int snoop_send_ack(struct hfi2_ibport *ibp, struct hfi2_qp_priv *priv,
			  union hfi2_packet_header *from, size_t hwords)
{
	struct hfi_devdata *dd = ibp->ibd->dd;
	int snoop_mode = 0;
	int md_len = 0;
	struct capture_md md;
	struct snoop_packet *s_packet = NULL;
	bool use_16b = priv->hdr_type == HFI2_PKT_TYPE_16B;
	int packet_len;
	int ret;

	packet_len = use_16b ? hfi2_16B_get_pkt_len(&from->opah) :
			       HFI1_GET_PKT_LEN(&from->ibh);

	snoop_dbg("ACK OUT: len %d", packet_len);

	if (!dd->hfi1_snoop.filter_callback) {
		snoop_dbg("filter not set");
		ret = HFI1_FILTER_HIT;
	} else {
		ret = dd->hfi1_snoop.filter_callback(
				from, NULL,
				dd->hfi1_snoop.filter_value,
				use_16b);
	}

	switch (ret) {
	case HFI1_FILTER_ERR:
		snoop_dbg("Error in filter call");
		/* fall through */
	case HFI1_FILTER_MISS:
		snoop_dbg("Filter Miss");
		break;
	case HFI1_FILTER_HIT:
		snoop_dbg("Capturing packet");
		if (dd->hfi1_snoop.mode_flag & HFI1_PORT_SNOOP_MODE)
			snoop_mode = 1;
		if ((snoop_mode == 0) ||
		    unlikely(snoop_flags & SNOOP_USE_METADATA))
			md_len = sizeof(struct capture_md);

		s_packet = allocate_snoop_packet(packet_len + md_len);

		if (unlikely(!s_packet)) {
			dd_dev_warn_ratelimited(dd, "Unable to allocate snoop/capture packet\n");
			goto inline_pio_out;
		}

		s_packet->total_len = packet_len + md_len;

		/* Fill in the metadata for the packet */
		if (md_len > 0) {
			memset(&md, 0, sizeof(struct capture_md));
			md.port = 1;
			md.dir = PKT_DIR_EGRESS;
			md.u.pbc = to_hfi1_pbc(use_16b);
			memcpy(s_packet->data, &md, md_len);
		}

		/* Add the packet data which is a single buffer */
		memcpy(s_packet->data + md_len, from, packet_len);

		snoop_list_add_tail(s_packet, dd);

		if (unlikely((snoop_flags & SNOOP_DROP_SEND) && snoop_mode)) {
			snoop_dbg("Dropping packet");
			return 0;
		}
		break;
	default:
		break;
	}

inline_pio_out:
	return hfi2_send_ack(ibp, priv, from, hwords);
}
