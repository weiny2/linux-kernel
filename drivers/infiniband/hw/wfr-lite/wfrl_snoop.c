/*
 * Copyright (c) 2006, 2007, 2008 QLogic Corporation. All rights reserved.
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
 * This file implements a raw read/raw write interface for snooping raw
 * packets from the wire and injecting raw packets to the wire.
 *
 * Other things that this interface could do at somepoint are:
 *   - Allow packets to be injected back into the stack
 *   - Provide an intercept for packets coming from the upper layers to
 *     move them back into user-space.
 */

#include <linux/pci.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <linux/module.h>

#include <rdma/ib_user_mad.h> /* for ioctl constants */
#include <rdma/ib_smi.h>


#include "wfrl.h"
#include "wfrl_verbs.h"
#include "wfrl_common.h"
#include <linux/poll.h>

#define QIB_SNOOP_IOC_MAGIC IB_IOCTL_MAGIC
#define QIB_SNOOP_IOC_BASE_SEQ 0x80
/* This starts our ioctl sequence
 * numbers *way* off from the ones
 * defined in ib_core
					*/
#define QIB_SNOOP_IOCGETLINKSTATE \
	_IO(QIB_SNOOP_IOC_MAGIC, QIB_SNOOP_IOC_BASE_SEQ)
#define QIB_SNOOP_IOCSETLINKSTATE \
	_IO(QIB_SNOOP_IOC_MAGIC, QIB_SNOOP_IOC_BASE_SEQ+1)
#define QIB_SNOOP_IOCCLEARQUEUE \
	_IO(QIB_SNOOP_IOC_MAGIC, QIB_SNOOP_IOC_BASE_SEQ+2)
#define QIB_SNOOP_IOCCLEARFILTER \
	_IO(QIB_SNOOP_IOC_MAGIC, QIB_SNOOP_IOC_BASE_SEQ+3)
#define QIB_SNOOP_IOCSETFILTER \
	_IO(QIB_SNOOP_IOC_MAGIC, QIB_SNOOP_IOC_BASE_SEQ+4)

#define QIB_SNOOP_IOCGETLINKSTATE_WFR \
	_IOWR(QIB_SNOOP_IOC_MAGIC, QIB_SNOOP_IOC_BASE_SEQ+5, \
		struct wfr_link_info)
#define QIB_SNOOP_IOCSETLINKSTATE_WFR \
	_IOWR(QIB_SNOOP_IOC_MAGIC, QIB_SNOOP_IOC_BASE_SEQ+6, \
		struct wfr_link_info)

/* local prototypes */
static int qib_snoop_open(struct inode *in, struct file *fp);
static unsigned int qib_snoop_poll(struct file *fp,
					struct poll_table_struct *wait);
static ssize_t qib_snoop_read(struct file *fp, char __user *data,
				size_t pkt_len, loff_t *off);
static int qib_snoop_release(struct inode *in, struct file *fp);

static long qib_ioctl(struct file *fp, unsigned int cmd, unsigned long arg);

static ssize_t qib_snoop_write(struct file *fp, const char __user *data,
				  size_t pkt_len, loff_t *off);

#include <linux/delay.h>

struct qib_packet_filter_command {
	int opcode;
	int length;
	void *value_ptr;
};

enum qib_packet_filter_opcodes {
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
	.open = qib_snoop_open,
	.read = qib_snoop_read,
	.unlocked_ioctl = qib_ioctl,
	.poll = qib_snoop_poll,
	.write = qib_snoop_write,
	.release = qib_snoop_release
};

struct qib_filter_array {
	int (*filter)(void *, void *, void *);
};

static int qib_filter_lid(void *ibhdr, void *packet_data, void *value);
static int qib_filter_dlid(void *ibhdr, void *packet_data, void *value);
static int qib_filter_mad_mgmt_class(void *ibhdr, void *packet_data,
				     void *value);
static int qib_filter_qp_number(void *ibhdr, void *packet_data, void *value);
static int qib_filter_ibpacket_type(void *ibhdr, void *packet_data,
				    void *value);
static int qib_filter_ib_service_level(void *ibhdr, void *packet_data,
				       void *value);
static int qib_filter_ib_pkey(void *ibhdr, void *packet_data, void *value);

static struct qib_filter_array qib_filters[] = {
	{ qib_filter_lid },
	{ qib_filter_dlid },
	{ qib_filter_mad_mgmt_class },
	{ qib_filter_qp_number },
	{ qib_filter_ibpacket_type },
	{ qib_filter_ib_service_level },
	{ qib_filter_ib_pkey }
};

#define QIB_MAX_FILTERS	ARRAY_SIZE(qib_filters)
#define QIB_DRV_NAME		"ib_qib"
#define QIB_MAJOR		233
#define QIB_USER_MINOR_BASE	0
#define QIB_DIAG_MINOR_BASE	129
#define QIB_SNOOP_MINOR_BASE	160
#define QIB_CAPTURE_MINOR_BASE	200
#define QIB_NMINORS		255
#define PORT_BITS		2
#define PORT_MASK		((1U << PORT_BITS) - 1)
#define GET_HCA(x)		((unsigned int)((x) >> PORT_BITS))
#define GET_PORT(x)		((unsigned int)((x) & PORT_MASK))

int qib_snoop_add(struct qib_devdata *dd)
{
	char name[32];
	int ret = 0;
	int i;
	int j;
	int minor = 0;

	for (i = 0; i < dd->num_pports; i++) {
		spin_lock_init(&dd->pport[i].snoop_write_lock);
		for (j = 0; j < QIB_CHAR_DEVICES_PER_PORT; j++) {
			spin_lock_init(&dd->pport[i].sc_device[j].snoop_lock);
			INIT_LIST_HEAD(
				&(dd->pport[i].sc_device[j].snoop_queue));
			init_waitqueue_head(
				&dd->pport[i].sc_device[j].snoop_waitq);

			if (j == 0) {
				minor = (((dd->unit << PORT_BITS) | i)) +
					QIB_SNOOP_MINOR_BASE;
				snprintf(name, sizeof(name),
					"ipath_snoop_%02d_%02d", dd->unit, i+1);
			} else {
				minor = (((dd->unit << PORT_BITS) | i)) +
						QIB_CAPTURE_MINOR_BASE;
				snprintf(name, sizeof(name),
					"ipath_capture_%02d_%02d",
					dd->unit, i+1);
			}

			ret = qib_cdev_init(
				minor, name,
				&snoop_file_ops,
				&dd->pport[i].sc_device[j].snoop_cdev,
				&dd->pport[i].sc_device[j].snoop_class_dev);
			if (ret)
				goto bail;
		}
		printk(KERN_INFO "ib_wfr_lite%d: snoop device for hca %02d enabled for Port %02d\n"
				"ib_wfr_lite%d: capture device for hca %02d enabled for port %02d\n",
			dd->unit, dd->unit, i+1, dd->unit, dd->unit, i+1);
		dd->pport[i].mode_flag = 0;
	}
out:
	return ret;
bail:
	qib_dev_err(dd, "Couldn't create %s device: %d", name, ret);
	i--;
	if (i != dd->num_pports) {
		for (; i >= 0 ; i--) {
			for (j = 0; j < QIB_CHAR_DEVICES_PER_PORT; j++)
				qib_cdev_cleanup(
					 &dd->pport[i].
					 sc_device[j].
					 snoop_cdev,
					 &dd->pport[i].
					 sc_device[j].
					 snoop_class_dev);
			dd->pport[i].mode_flag = 0;
		}
	}
	goto out;
}

/* this must be called w/ dd->snoop_in_lock held */
static void drain_snoop_list(struct qib_aux_device *sc_device)
{
	struct list_head *pos, *q;
	struct snoop_packet *packet;

	list_for_each_safe(pos, q, &(sc_device->snoop_queue)) {
		packet = list_entry(pos, struct snoop_packet, list);
		list_del(pos);
		kfree(packet);
	}
}

void qib_snoop_remove(struct qib_devdata *dd)
{
	unsigned long flags = 0;
	int i;
	int j;

	for (i = 0; i < dd->num_pports; i++) {
		dd->pport[i].mode_flag = 0;
		for (j = 0; j < QIB_CHAR_DEVICES_PER_PORT; j++) {
			spin_lock_irqsave(&dd->pport[i].sc_device[j].snoop_lock,
				flags);
			drain_snoop_list(&dd->pport[i].sc_device[j]);
			qib_cdev_cleanup(&dd->pport[i].sc_device[j].snoop_cdev,
				&dd->pport[i].sc_device[j].snoop_class_dev);
			spin_unlock_irqrestore(
				&dd->pport[i].sc_device[j].snoop_lock,
				flags);
		}
	}
}

static int qib_snoop_open(struct inode *in, struct file *fp)
{
	int unit = iminor(in);
	int devnum;
	int portnum = 0;
	int ret;
	int mode_flag = 0;
	unsigned long flags;
	struct qib_devdata *dd;

	mutex_lock(&qib_mutex);

	if (unit >= QIB_CAPTURE_MINOR_BASE) {
		unit -= QIB_CAPTURE_MINOR_BASE;
		devnum = 1;
		mode_flag = QIB_PORT_CAPTURE_MODE;
	} else {
		unit -= QIB_SNOOP_MINOR_BASE;
		devnum = 0;
		mode_flag = QIB_PORT_SNOOP_MODE;
	}

	dd = qib_lookup(GET_HCA(unit));
	if (dd == NULL || !(dd->flags & QIB_PRESENT) ||
	    !dd->kregbase) {
		ret = -ENODEV;
		goto bail;
	}
	portnum = GET_PORT(unit);

	spin_lock_irqsave(&dd->pport[portnum].sc_device[devnum].snoop_lock,
		flags);

	if (dd->pport[portnum].mode_flag & mode_flag) {
		ret = -EBUSY;
		spin_unlock_irqrestore(
			&dd->pport[portnum].sc_device[devnum].snoop_lock,
			flags);
		goto bail;
	}

	drain_snoop_list(&dd->pport[portnum].sc_device[devnum]);
	spin_unlock_irqrestore(
		&dd->pport[portnum].sc_device[devnum].snoop_lock, flags);
	if (devnum)
		printk(KERN_ALERT
			"capture device for hca %02d port %02d is opened\n",
			GET_HCA(unit), portnum+1);
	else
		printk(KERN_ALERT
			"snoop device for hca %02d port %02d is opened\n",
			GET_HCA(unit), portnum+1);

	dd->pport[portnum].sc_device[devnum].pport = &dd->pport[portnum];
	fp->private_data = &dd->pport[portnum].sc_device[devnum];
	ret = 0;
	dd->pport[portnum].mode_flag |= mode_flag;

bail:
	mutex_unlock(&qib_mutex);

	return ret;
}

static int qib_snoop_release(struct inode *in, struct file *fp)
{
	struct qib_aux_device *sc_device = fp->private_data;
	struct qib_pportdata *pport = sc_device->pport;
	unsigned long flags = 0;
	int devnum = iminor(in);

	if (devnum >= QIB_CAPTURE_MINOR_BASE)
		devnum = 1;
	else
		devnum = 0;

	spin_lock_irqsave(&sc_device->snoop_lock, flags);
	if (devnum)
		pport->mode_flag = pport->mode_flag & (~QIB_PORT_CAPTURE_MODE);
	else
		pport->mode_flag = pport->mode_flag & (~QIB_PORT_SNOOP_MODE);

	drain_snoop_list(sc_device);
	/* Clear filters before going out */
	pport->filter_callback = NULL;
	kfree(pport->filter_value);
	pport->filter_value = NULL;

	spin_unlock_irqrestore(&sc_device->snoop_lock, flags);

	if (devnum)
		printk(KERN_ALERT"capture device for hca %02d port %02d is closed\n",
				 pport->dd->unit, pport->port);
	else
		printk(KERN_ALERT"snoop device for hca %02d port %02d is closed\n",
				 pport->dd->unit, pport->port);

	fp->private_data = NULL;
	return 0;
}

static unsigned int qib_snoop_poll(struct file *fp,
		struct poll_table_struct *wait)
{
	struct qib_aux_device *sc_device = fp->private_data;
	int ret = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&sc_device->snoop_lock, flags);

	poll_wait(fp, &sc_device->snoop_waitq, wait);
	if (!list_empty(&sc_device->snoop_queue))
		ret |= POLLIN | POLLRDNORM;

	spin_unlock_irqrestore(&sc_device->snoop_lock, flags);
	return ret;

}

static ssize_t qib_snoop_read(struct file *fp, char __user *data,
			       size_t pkt_len, loff_t *off)
{
	struct qib_aux_device *sc_device = fp->private_data;
	ssize_t ret = 0;
	unsigned long flags = 0;
	struct snoop_packet *packet = NULL;

	spin_lock_irqsave(&sc_device->snoop_lock, flags);

	while (list_empty(&sc_device->snoop_queue)) {
		spin_unlock_irqrestore(&sc_device->snoop_lock, flags);

		if (fp->f_flags & O_NONBLOCK)
			return -EAGAIN;


		if (wait_event_interruptible(sc_device->snoop_waitq,
					!list_empty(&sc_device->snoop_queue)))
			return -EINTR;

		spin_lock_irqsave(&sc_device->snoop_lock, flags);
	}

	if (!list_empty(&(sc_device->snoop_queue))) {
		packet = list_entry(sc_device->snoop_queue.next,
				struct snoop_packet, list);
		list_del(&packet->list);
		spin_unlock_irqrestore(&sc_device->snoop_lock, flags);
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
		spin_unlock_irqrestore(&sc_device->snoop_lock, flags);

	return ret;
}

static long qib_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
	struct qib_aux_device *sc_device = fp->private_data;
	struct qib_pportdata *ppd = sc_device->pport;
	struct qib_devdata *dd = ppd->dd;
	void *filter_value = NULL;
	long ret = 0;
	int value = 0;
	u8 physState = 0;
	u8 linkState = 0;
	u16 devState = 0;
	unsigned long flags = 0;
	unsigned long *argp = NULL;
	struct qib_packet_filter_command filter_cmd = {0};
	__be64 local_node_guid = 0;
	u8 local_port_mode = 0;

	if (((_IOC_DIR(cmd) & _IOC_READ)
	&& !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd)))
	|| ((_IOC_DIR(cmd) & _IOC_WRITE)
	&& !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd)))) {
		ret = -EFAULT;
	} else if (!capable(CAP_SYS_ADMIN)) {
		ret = -EPERM;
	} else if (sc_device != (&ppd->sc_device[QIB_SNOOP_DEV_INDEX])
		&& cmd != QIB_SNOOP_IOCCLEARQUEUE
		&& cmd != QIB_SNOOP_IOCCLEARFILTER
		&& cmd != QIB_SNOOP_IOCSETFILTER) {
		/* Capture devices are allowed only 3 operations
		 * 1.Clear capture queue
		 * 2.Clear capture filter
		 * 3.Set capture filter
		 * Other are invalid.
		 */
		ret = -EINVAL;
	} else {
		switch (cmd) {
		case QIB_SNOOP_IOCSETLINKSTATE:
		case QIB_SNOOP_IOCSETLINKSTATE_WFR:
		{
			struct wfr_link_info cur_link_info;
			u8 curLinkState;

			if (cmd == QIB_SNOOP_IOCSETLINKSTATE) {
				ret = __get_user(value, (int __user *) arg);
				if (ret !=  0)
					break;
			} else {
				struct wfr_link_info link_info;
				if (!wfr_vl15_ovl0) {
					ret = -EINVAL;
					break;
				}
				ret = copy_from_user(&link_info,
					(struct wfr_link_info __user *)arg,
					sizeof(link_info));
				if (ret !=  0)
					break;
				value = link_info.port_state;
				local_node_guid = cpu_to_be64(link_info.node_guid);
				local_port_mode = link_info.port_mode;
			}

			physState = (value >> 4) & 0xF;
			linkState = value & 0xF;

			wfrl_get_stl_virtual_link_info(ppd->port, &cur_link_info);
			curLinkState = cur_link_info.port_state & 0xf;

			switch (linkState) {
			case IB_PORT_NOP:
				if (physState == 0)
					break;
					/* fall through */
			case IB_PORT_DOWN:
				/* use virtual state */
				if (wfr_vl15_ovl0) {
					wfrl_set_stl_virtual_link_info(&ppd->dd->verbs_dev.ibdev,
									ppd->port,
									(uint8_t)value & 0xff,
									local_node_guid,
									local_port_mode);
				} else {
					/* preserve IB mode */
					switch (physState) {
					case 0:
						if (dd->f_ibphys_portstate &&
					(dd->f_ibphys_portstate(ppd->lastibcstat)
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
				}
				break;
			case IB_PORT_ARMED:
				if (wfr_vl15_ovl0) {
					if (curLinkState == IB_PORT_INIT || curLinkState == IB_PORT_ACTIVE) {
						wfrl_set_stl_virtual_link_info(&ppd->dd->verbs_dev.ibdev,
									ppd->port,
									(uint8_t)value & 0xff,
									local_node_guid,
									local_port_mode);
					} else {
						/* Can't transition to armed
						 * if not init | active */
						ret = -EINVAL;
						break;
					}
				} else {
					if (!(dd->flags &
					(QIB_IB_LINKARM | QIB_IB_LINKACTIVE))) {
							ret = -EINVAL;
							break;
						}
					ret = qib_set_linkstate(ppd, QIB_IB_LINKARM);
				}
				break;
			case IB_PORT_ACTIVE:
				if (wfr_vl15_ovl0) {
					if (curLinkState != IB_PORT_ARMED) {
						/* Can't transition to active
						 * if not armed */
						ret = -EINVAL;
						break;
					}
					wfrl_set_stl_virtual_link_info(&ppd->dd->verbs_dev.ibdev,
									ppd->port,
									(uint8_t)value & 0xff,
									local_node_guid,
									local_port_mode);
				} else {
					if (!(dd->flags & QIB_IB_LINKARM)) {
						ret = -EINVAL;
						break;
					}
					ret = qib_set_linkstate(ppd, QIB_IB_LINKACTIVE);
				}
				break;
			default:
				ret = -EINVAL;
				break;
			}

			if (ret)
				break;

			/* if not error -- fall through */
		}
		case QIB_SNOOP_IOCGETLINKSTATE:
		case QIB_SNOOP_IOCGETLINKSTATE_WFR:
		{
			struct wfr_link_info link_info;
			memset(&link_info, 0, sizeof(link_info));

			if (wfr_vl15_ovl0) {
				wfrl_get_stl_virtual_link_info(ppd->port, &link_info);
			} else {
				link_info.port_state = dd->f_ibphys_portstate(ppd->lastibcstat) << 4;
				link_info.port_state |= dd->f_iblink_state(ppd->lastibcstat);
				/* FIXME for real WFR
				 * link_info.node_guid =
				 * link_info.port_mode =
				 */
			}
			if (cmd == QIB_SNOOP_IOCGETLINKSTATE
			    || cmd == QIB_SNOOP_IOCSETLINKSTATE) {
				ret = __put_user(link_info.port_state, (int __user *)arg);
			} else {
				ret = copy_to_user((struct wfr_link_info __user *)arg,
					&link_info,
					sizeof(link_info));
			}
			break;
		}
		case QIB_SNOOP_IOCCLEARQUEUE:
			spin_lock_irqsave(&sc_device->snoop_lock, flags);
			drain_snoop_list(sc_device);
			spin_unlock_irqrestore(&sc_device->snoop_lock, flags);
			break;

		case QIB_SNOOP_IOCCLEARFILTER:
			spin_lock_irqsave(&sc_device->snoop_lock, flags);
			if (ppd->filter_callback) {
				/* Drain packets first */
				drain_snoop_list(sc_device);
				ppd->filter_callback = NULL;
			}
			kfree(ppd->filter_value);
			ppd->filter_value = NULL;
			spin_unlock_irqrestore(&sc_device->snoop_lock, flags);
			break;

		case QIB_SNOOP_IOCSETFILTER:
			/* just copy command structure */
			argp = (unsigned long *)arg;
			ret = copy_from_user(&filter_cmd, (u8 *)argp,
				sizeof(filter_cmd));
			if (ret < 0) {
				printk(KERN_ALERT
					"Error copying filter command\n");
				break;
			}
			if (filter_cmd.opcode >= QIB_MAX_FILTERS) {
				printk(KERN_ALERT"Invalid opcode in request\n");
				ret = -EINVAL;
				break;
			}
			filter_value = kzalloc(
						filter_cmd.length * sizeof(u8),
						GFP_KERNEL);
			if (!filter_value) {
				printk(KERN_ALERT"Not enough memory\n");
				ret = -ENOMEM;
				break;
			}
			/* copy remaining data from userspace */
			ret = copy_from_user((u8 *)filter_value,
					(u8 *)filter_cmd.value_ptr,
					filter_cmd.length);
			if (ret < 0) {
				kfree(filter_value);
				printk(KERN_ALERT"Error copying filter data\n");
				break;
			}
			/* Drain packets first */
			spin_lock_irqsave(&sc_device->snoop_lock, flags);
			drain_snoop_list(sc_device);
			ppd->filter_callback =
				qib_filters[filter_cmd.opcode].filter;
			/* just in case we see back to back sets */
			kfree(ppd->filter_value);
			ppd->filter_value = filter_value;
			spin_unlock_irqrestore(&sc_device->snoop_lock, flags);
			break;

		default:
			ret = -ENOTTY;
			break;
		}
	}
done:
	return ret;
}


static ssize_t qib_pio_send_pkt(struct qib_pportdata *ppd,
				u32 *data, u32 pkt_len)
{
	int i;
	u64 pbc;
	u32 __iomem *piobuf;
	u32 pnum, control, len;
	struct qib_devdata *dd = ppd->dd;
	u32 dwords = pkt_len >> 2;
	unsigned long flags;
	ssize_t ret = -EINVAL;
	struct qib_ib_header *hdr = (struct qib_ib_header *)data;

	/* Handle VL15_o_VL0 for Snoop packets */
	if (((((u8 *)data)[0] >> 4) & 0xf) == 0xf) {
		if (pkt_len > 284) {
			if (wfr_vl15_ovl0) {
				/* Set the VL to 0 in the LRH */
				hdr->lrh[0] = cpu_to_be16(be16_to_cpu(hdr->lrh[0])
								& 0x0fff);
				if (hdr->lrh[1] == IB_LID_PERMISSIVE) {
					/* permissive lid specified
					 * override with DLID and set flag for rcv to
					 * reconstruct PermissiveLID in packet
					 */
					hdr->lrh[0] |= WFR_VL15_oVL0_PERMLID_FLAG;
					hdr->lrh[1] = cpu_to_be16(wfr_vl15_ovl0);
				}
			} else {
				/* protect ourselves from writing large buffers into
				 * the VL15 bufs */
				ret = -EINVAL;
				goto Err;
			}
		}
	}
	i = 0;
	len = dwords + 1;
	control = dd->f_setpbc_control(ppd, len, 0,
		  (((u8 *)data)[0] >> 4) & 0xf);
	pbc = ((u64) control << 32) | len;
	while (!(piobuf = dd->f_getsendbuf(ppd, pbc, &pnum))) {
		if (i > 15) {
			printk(KERN_ERR PFX
				"ERROR: snoop failed to get sendbuf\n");
			ret = -ENOMEM;
			goto Err;
		}
		i++;
		/* lets try to flush all of it */
		dd->f_sendctrl(ppd, QIB_SENDCTRL_DISARM_ALL);
		udelay(100);
	}
	spin_lock_irqsave(&ppd->snoop_write_lock, flags);
	/* disable header check on this packet, since it can't be valid */
	dd->f_txchk_change(dd, pnum, 1, TXCHK_CHG_TYPE_DIS1, NULL);
	writeq(pbc, piobuf);
	qib_flush_wc();
	if (dd->flags & QIB_PIO_FLUSH_WC) {
		qib_flush_wc();
		qib_pio_copy(piobuf + 2, data, dwords - 1);
		qib_flush_wc();
		__raw_writel(data[dwords - 1], piobuf + dwords + 1);
	} else
		qib_pio_copy(piobuf + 2, data, dwords);
	if (dd->flags & QIB_USE_SPCL_TRIG) {
		u32 spcl_off = (pnum >= dd->piobcnt2k) ? 2047 : 1023;

		qib_flush_wc();
		__raw_writel(0xaebecede, piobuf + spcl_off);
	}
	qib_sendbuf_done(dd, pnum);
	qib_flush_wc();
	/* and re-enable hdr check */
	dd->f_txchk_change(dd, pnum, 1, TXCHK_CHG_TYPE_ENAB1, NULL);
	spin_unlock_irqrestore(&ppd->snoop_write_lock, flags);
	ret = pkt_len;
Err:
	return ret;
}


static ssize_t qib_snoop_write(struct file *fp, const char __user *data,
					size_t pkt_len, loff_t *off)
{
	struct qib_aux_device *sc_device = fp->private_data;
	struct qib_pportdata *ppd = sc_device->pport;
	struct qib_devdata *dd = ppd->dd;
	ssize_t ret = 0;
	u32 *buffer = NULL;
	u32 plen, clen;

	/* capture device should not be entertaining writes */
	if (sc_device != (&ppd->sc_device[QIB_SNOOP_DEV_INDEX])) {
		ret = -EINVAL;
		goto bail;
	}

	if (pkt_len == 0)
		goto bail;

	if (pkt_len & 3) {
		ret = -EINVAL;
		goto bail;
	}

	clen = pkt_len >> 2;

	if (!dd || !(dd->flags & QIB_PRESENT) ||
			!dd->kregbase) {
		ret = -ENODEV;
		goto bail;
	}

	if (!(dd->flags & QIB_INITTED)) {
		/* no hardware, freeze, etc. */
		ret = -ENODEV;
		goto bail;
	}

	plen = sizeof(u32) + pkt_len;

	if ((plen + 4) > ppd->ibmaxlen) {
		ret = -EINVAL;
		goto bail;  /* before writing pbc */
	}

	buffer = vmalloc(plen);
	if (!buffer) {
		printk(KERN_ERR PFX
			"ERROR: snoop failed vmalloc error\n");
		ret = -ENOMEM;
		goto bail;
	}
	if (copy_from_user(buffer,
		(const void __user *) (unsigned long) data, pkt_len)) {
		ret = -EFAULT;
		goto bail;
	}

	ret = qib_pio_send_pkt(ppd, buffer, pkt_len);

bail:
	vfree(buffer);

	return ret;
}

int snoop_get_header_size(struct qib_devdata *dd,
			struct qib_ib_header *hdr,
			void *data, u32 tlen)
{
	int lnh, header_size = -1;
	u8 opcode, opcode_major;
	struct qib_other_headers *ohdr;

	lnh = (be16_to_cpu(hdr->lrh[0]) & 3);

	if (lnh == QIB_LRH_BTH)
		ohdr = &hdr->u.oth;
	else if (lnh == QIB_LRH_GRH)
		ohdr = &hdr->u.l.oth;
	else
		goto bail;

	opcode = be32_to_cpu(ohdr->bth[0]) >> 24;

	opcode_major = (opcode >> 5) & 0x7;

	switch (opcode_major) {
	case 0x03: /* UD */
		if (lnh == QIB_LRH_BTH)
			header_size = 8 + 12 + 8 /* LRH + BTH + DETH */;
		else if (lnh == QIB_LRH_GRH) {

			/* LRH + GRH + BTH + DETH */;
			header_size = 8 + 40 + 12 + 8;
			/* Some of the header data is in the data segment */
			if (dd->rcvhdrentsize == 16)
				header_size -= 12;
		} else
			header_size = -1;

		break;
	case 0x0: /* RC */
	case 0x1: /* UC */
	case 0x2: /* RD */
	default:
		header_size = -1;
		break;
	}

bail:
	return header_size;
}

static void qib_snoop_list_add_tail(struct snoop_packet *packet,
					struct qib_pportdata *ppd,
					int dev_index)
{
	unsigned long flags = 0;
	struct qib_aux_device *sc_device = &ppd->sc_device[dev_index];

	spin_lock_irqsave(&sc_device->snoop_lock, flags);
	if (likely((dev_index == QIB_CAPTURE_DEV_INDEX &&
		(ppd->mode_flag & QIB_PORT_CAPTURE_MODE)) ||
		(dev_index == QIB_SNOOP_DEV_INDEX &&
		(ppd->mode_flag & QIB_PORT_SNOOP_MODE))))
		list_add_tail(&(packet->list), &sc_device->snoop_queue);
	spin_unlock_irqrestore(&sc_device->snoop_lock, flags);
	wake_up_interruptible(&sc_device->snoop_waitq);
}

/**
 * Check for WFR_LITE control MAD packets
 *
 * WFR_LITE control MADs are IB DR SMP's with attribute_id > IB_SMP_ATTR_WFR_LITE_MIN
 *
 * Returns,
 * 1 if this packet is to be processed by the driver
 * 0 if this packet is to be sent to the snoop interface
 */
static int is_wfr_lite_ctrl_pkt(struct qib_ib_header *hdr, void *data)
{
	u8 vl;
	u32 qp;
	struct ib_smp *smp = (struct ib_smp *)data;

	/* Verify LRH */
	vl = (be16_to_cpu(hdr->lrh[0]) & 0xf000) >> 12;
	if (vl != 15)
		return 0;

	/* Verify BTH */
	qp = be32_to_cpu(hdr->u.oth.bth[1]) & 0x0fff;
	if (qp != 0)
		return 0;

	/* Check SMP */
	if (smp->base_version == IB_MGMT_BASE_VERSION
	    && smp->class_version == 1
	    && smp->mgmt_class == IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE
	    && be16_to_cpu(smp->attr_id) > IB_SMP_ATTR_WFR_LITE_MIN) {
		printk(KERN_INFO "ib_wfr_lite: caught WFR CTRL attr id 0x%x\n", smp->attr_id);
		return 1;
	}

	return 0;
}

void qib_snoop_send_queue_packet(struct qib_pportdata *ppd,
				struct snoop_packet *packet)
{
	struct qib_ib_header *hdr = (struct qib_ib_header *)packet->data;
	int hdr_size = snoop_get_header_size(ppd->dd, hdr, NULL, 0);
	void *data = (void *)&packet->data[hdr_size];

	if (is_wfr_lite_ctrl_pkt(hdr, data))
		return;

	/* If we are dealing with mix mode then we need to make another copy
	 * of same packet and queue it in snoop device as well.
	 * However if we do not get sufficient memory here then we just
	 * add packet to capture queue by default so that we atleast have one
	 * packet with us in capture queue.
	 */
	if (unlikely(ppd->mode_flag ==
		(QIB_PORT_SNOOP_MODE | QIB_PORT_CAPTURE_MODE))) {
		struct snoop_packet *pcopy;
		pcopy = kmalloc(sizeof(*pcopy) + packet->total_len, GFP_ATOMIC);
		if (pcopy != NULL) {
			memcpy(pcopy, packet,
				packet->total_len + sizeof(*pcopy));
			qib_snoop_list_add_tail(pcopy, ppd,
				QIB_SNOOP_DEV_INDEX);
		}
		qib_snoop_list_add_tail(packet, ppd, QIB_CAPTURE_DEV_INDEX);
	} else if (ppd->mode_flag == QIB_PORT_CAPTURE_MODE)
		qib_snoop_list_add_tail(packet, ppd, QIB_CAPTURE_DEV_INDEX);
	else if (ppd->mode_flag == QIB_PORT_SNOOP_MODE)
		qib_snoop_list_add_tail(packet, ppd, QIB_SNOOP_DEV_INDEX);
}

/*
 * qib_snoop_rcv_queue_packet - receive a packet for snoop interface
 * @port - Hca port on which this packet is received.
 * @rhdr - Packet header
 * @data - Packet data/payloaa
 * @tlen - total length of packet including header and payload.
 *
 * Called on for every packet received when snooping/mix mode is turned on
 * Copies received packet to internal buffer and appends it to
 * packet list.
 *
 * Returns,
 * 0 if this packet needs to be forwarded by driver
 * 1 if this packet needs to be dropped by driver
 */

int qib_snoop_rcv_queue_packet(struct qib_pportdata *port, void *rhdr,
				void *data, u32 tlen)
{
	int header_size = 0;
	struct qib_ib_header *hdr = rhdr;
	struct snoop_packet *packet = NULL;

	header_size = snoop_get_header_size(port->dd, hdr, data, tlen);
	if (header_size <= 0)
		return 0;

	/* qib_snoop_send_queue_packet takes care or mix mode,
	 * so just return from here.
	 */
	if (port->mode_flag == (QIB_PORT_SNOOP_MODE | QIB_PORT_CAPTURE_MODE))
		return 0;

	if (is_wfr_lite_ctrl_pkt(hdr, data))
		return 0;

	packet = kmalloc(sizeof(struct snoop_packet) + tlen,
					GFP_ATOMIC);
	if (likely(packet)) {
		memcpy(packet->data, rhdr, header_size);
		memcpy(packet->data + header_size, data,
		tlen - header_size);
		packet->total_len = tlen;
		qib_snoop_list_add_tail(packet, port, QIB_SNOOP_DEV_INDEX);
		return 1;
	}

	return 0;
}

static int qib_filter_lid(void *ibhdr, void *packet_data, void *value)
{
	struct qib_ib_header *hdr = (struct qib_ib_header *)ibhdr;
	if (*((u16 *)value) == be16_to_cpu(hdr->lrh[3]))
		return 0; /* matched */
	return 1; /* Not matched */
}

static int qib_filter_dlid(void *ibhdr, void *packet_data, void *value)
{
	struct qib_ib_header *hdr = (struct qib_ib_header *)ibhdr;
	if (*((u16 *)value) == be16_to_cpu(hdr->lrh[1]))
		return 0;
	return 1;
}

static int qib_filter_mad_mgmt_class(void *ibhdr, void *packet_data,
					 void *value)
{
	struct qib_ib_header *hdr = (struct qib_ib_header *)ibhdr;
	struct qib_other_headers *ohdr = NULL;
	struct ib_smp *smp = NULL;
	u32 qpn = 0;

	/* packet_data could be null if only header is captured */
	if (packet_data == NULL)
		return 1;
	/* Check for GRH */
	if ((be16_to_cpu(hdr->lrh[0]) & 3) == QIB_LRH_BTH)
		ohdr = &hdr->u.oth; /* LRH + BTH + DETH */
	else
		ohdr = &hdr->u.l.oth; /* LRH + GRH + BTH + DETH */
	qpn = be32_to_cpu(ohdr->bth[1]) & 0x00FFFFFF;
	if (qpn <= 1) {
		smp = (struct ib_smp *)packet_data;
		if (*((u8 *)value) == smp->mgmt_class)
			return 0;
		else
			return 1;
		}
	return 1;
}

static int qib_filter_qp_number(void *ibhdr, void *packet_data, void *value)
{

	struct qib_ib_header *hdr = (struct qib_ib_header *)ibhdr;
	struct qib_other_headers *ohdr = NULL;

	/* Check for GRH */
	if ((be16_to_cpu(hdr->lrh[0]) & 3) == QIB_LRH_BTH)
		ohdr = &hdr->u.oth; /* LRH + BTH + DETH */
	else
		ohdr = &hdr->u.l.oth; /* LRH + GRH + BTH + DETH */
	if (*((u32 *)value) == (be32_to_cpu(ohdr->bth[1]) & 0x00FFFFFF))
		return 0;
	return 1;
}


static int qib_filter_ibpacket_type(void *ibhdr, void *packet_data,
					void *value)
{
	u32 lnh = 0;
	u8 opcode = 0;
	struct qib_ib_header *hdr = (struct qib_ib_header *)ibhdr;
	struct qib_other_headers *ohdr = NULL;

	lnh = (be16_to_cpu(hdr->lrh[0]) & 3);

	if (lnh == QIB_LRH_BTH)
		ohdr = &hdr->u.oth;
	else if (lnh == QIB_LRH_GRH)
		ohdr = &hdr->u.l.oth;
	else
		return 1;

	opcode = be32_to_cpu(ohdr->bth[0]) >> 24;

	if (*((u8 *)value) == ((opcode >> 5) & 0x7))
		return 0;
	return 1;
}

static int qib_filter_ib_service_level(void *ibhdr, void *packet_data,
					   void *value)
{
	struct qib_ib_header *hdr = (struct qib_ib_header *)ibhdr;

	if ((*((u8 *)value)) == (be16_to_cpu(hdr->lrh[0] >> 4) & 0xF))
		return 0;
	return 1;
}

static int qib_filter_ib_pkey(void *ibhdr, void *packet_data, void *value)
{

	u32 lnh = 0;
	struct qib_ib_header *hdr = (struct qib_ib_header *)ibhdr;
	struct qib_other_headers *ohdr = NULL;

	lnh = (be16_to_cpu(hdr->lrh[0]) & 3);
	if (lnh == QIB_LRH_BTH)
		ohdr = &hdr->u.oth;
	else if (lnh == QIB_LRH_GRH)
		ohdr = &hdr->u.l.oth;
	else
		return 1;

	/* P_key is 16-bit entity, however top most bit indicates
	 * type of membership. 0 for limited and 1 for Full.
	 * Limited members cannot accept information from other
	 * Limited members, but communication is allowed between
	 * every other combination of membership.
	 * Hence we'll omitt comparing top-most bit while filtering
	 */

	if ((*(u16 *)value & 0x7FFF) ==
		((be32_to_cpu(ohdr->bth[0])) & 0x7FFF))
		return 0;
	return 1;
}
