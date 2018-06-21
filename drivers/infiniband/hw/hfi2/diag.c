/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2016 Intel Corporation.
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
 * Copyright (c) 2016 Intel Corporation.
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
#include <linux/io.h>
#include <linux/pci.h>
#include <linux/poll.h>
#include <linux/vmalloc.h>
#include <linux/export.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "hfi2.h"
#include "verbs/verbs.h"
#include "hfi_kclient.h"
#include "hfi_tx_bypass.h"

#define HFI2_NUM_DIAGPKT_EVT 2048
#define HFI2_DIAG_TIMEOUT_MS 100
#define HFI2_DIAG_DEFAULT_MTU 8192
#define PBC_DCINFO_SHIFT 30
#define PBC_DCINFO_MASK   1
#define PBC_DCINFO_SMASK (PBC_DCINFO_MASK << PBC_DCINFO_SHIFT)
#define PBC_VL_SHIFT 12
#define PBC_VL_MASK 0xfull
#define HFI_BYPASS_L2_OFFSET    1
#define HFI_BYPASS_SC_OFFSET    1
#define HFI_BYPASS_L2_SHFT      29
#define HFI_BYPASS_SC_SHFT      20
#define HFI_BYPASS_L2_MASK      0x3ull
#define HFI_BYPASS_SC_MASK      0x1Full
#define HFI_9B_L2		0x3

#define HFI2_PBC_PACKETBYPASS_SMASK BIT(28)

#define HFI_BYPASS_GET_L2_TYPE(data) \
	((*((u32 *)(data) + HFI_BYPASS_L2_OFFSET) >> \
	  HFI_BYPASS_L2_SHFT) & HFI_BYPASS_L2_MASK)

#define HFI_BYPASS_GET_SC(data) \
	((*((u32 *)(data) + HFI_BYPASS_SC_OFFSET) >> \
	  HFI_BYPASS_SC_SHFT) & HFI_BYPASS_SC_MASK)

/*
 * Diagnostics can send a packet by writing the following
 * struct to the diag packet special file.
 *
 * This allows a custom PBC qword, so that special modes and deliberate
 * changes to CRCs can be used.
 */
#define _DIAG_PKT_VERS 1
struct diag_pkt {
	u16 version;	/* structure version */
	u16 unit;	/* which device */
	u16 sw_index;	/* send sw index to use */
	u16 len;	/* data length, in bytes */
	u16 port;	/* port number */
	u16 unused;
	u32 flags;	/* call flags */
	u64 data;	/* user data pointer */
	u64 pbc;	/* PBC for the packet */
};

struct miscdevice diagpkt_miscdev;

ssize_t snoop_diagpkt_send(struct hfi_devdata *dd, const char __user *data,
			   size_t count, u8 port, u64 pbc);

static inline u8 hfi_lrh_vl_to_sc(u8 vl, bool dc)
{
	return ((vl >> 4) & 0xF) | (!!dc << 4);
}

/* diag_pkt flags */
#define F_DIAGPKT_WAIT 0x1      /* wait until packet is sent */
static int diagpkt_xmit(struct hfi_devdata *dd, void *buf,
			struct diag_pkt *dp, u8 sc, int l2)
{
	struct hfi2_diagpkt_data *diag = &dd->hfi2_diag;
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, dp->port);
	struct hfi_ctx *ctx = diag->ctx;
	struct hfi_cmdq *tx = diag->cmdq_tx;
	union hfi_tx_general_dma cmd;
	union initiator_EQEntry *txe;
	u64 *eq_entry;
	union base_iovec iov __aligned(64);
	unsigned long sflags;
	int rc, slots;
	struct hfi_eq *eq = HFI_EQ_NONE;

	dd_dev_dbg(dd, "xmit sc %d len %d pt %d pbc 0x%llx f %d\n",
		   sc, dp->len, dp->port, dp->pbc, dp->flags);
	/* Cannot send packets unless port is active or unless using SC15 */
	if ((ppd->host_link_state != HLS_UP_ACTIVE) && sc != 15) {
		rc = -ENODEV;
		dd_dev_err(dd, "%s %d port is not active\n",
			   __func__, __LINE__);
		goto done;
	}

	iov.val[0] = 0x0;
	iov.val[1] = 0x0;
	iov.start = (u64)buf;
	iov.length = dp->len;
	iov.ep = 1;
	iov.sp = 1;
	iov.v = 1;
	if (iov.length > HFI2_DIAG_DEFAULT_MTU) {
		rc = -ENOSPC;
		dd_dev_err(dd, "%s %d iov.length %d > max MTU %d\n",
			   __func__, __LINE__, iov.length,
			   HFI2_DIAG_DEFAULT_MTU);
		goto done;
	}

	/* Use 9B in base_iovec if it is a 9B packet */
	iov.use_9b = l2 == HFI_9B_L2;

	/* Wait for an event only if requested by the user */
	if (dp->flags & F_DIAGPKT_WAIT)
		eq = &diag->eq_tx;

	spin_lock_irqsave(&diag->cmdq_tx_lock, sflags);

	slots = hfi_format_bypass_dma_cmd(ctx, (u64)&iov, 1, 0xdead,
					  PTL_MD_RESERVED_IOV, eq, HFI_CT_NONE,
					  dp->port - 1, ppd->sc_to_sl[sc],
					  ppd->lid, l2,
					  (sc == 15) ? MGMT_DMA : GENERAL_DMA,
					  &cmd);

	/* Queue write, don't wait. */
	rc = hfi_pend_cmd_queue(diag->pend_cmdq, tx, eq, &cmd, slots,
				GFP_KERNEL);
	if (rc) {
		dd_dev_err(dd, "%s: hfi_pend_cmd_queue_wait failed %d\n",
			   __func__, rc);
		goto unlock;
	}

	if (eq == HFI_EQ_NONE)
		goto unlock;

	rc = hfi_eq_wait_timed(&diag->eq_tx, HFI2_DIAG_TIMEOUT_MS,
			       &eq_entry);
	if (!rc) {
		txe = (union initiator_EQEntry *)eq_entry;
		dd_dev_err(dd, "TX evt success %d L2 %lld txe->event_kind %d\n",
			   rc, HFI_BYPASS_GET_L2_TYPE(buf), txe->event_kind);
		hfi_eq_advance(&diag->eq_tx, eq_entry);
	} else {
		dd_dev_err(dd, "TX event 1 failure, %d L2 %d\n", rc, l2);
	}
unlock:
	spin_unlock_irqrestore(&diag->cmdq_tx_lock, sflags);
done:
	return rc;
}

/**
 * diagpkt_send - send a packet
 * @dp: diag packet descriptor
 */
static ssize_t diagpkt_send(struct diag_pkt *dp, struct hfi_devdata *dd)
{
	struct hfi2_diagpkt_data *diag = &dd->hfi2_diag;
	u32 *tmpbuf = NULL;
	ssize_t ret = 0;
	int l2;
	bool use_9b;
	u8 sc, vl;

	if (!dd || !dd->kregbase || !diag->ctx) {
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

	/* allocate a buffer and copy the data in */
	tmpbuf = vmalloc(dp->len);
	if (!tmpbuf) {
		ret = -ENOMEM;
		goto bail;
	}

	hfi_at_reg_range(diag->ctx, tmpbuf, dp->len, NULL, false);

	if (copy_from_user(tmpbuf,
			   (const void __user *)(unsigned long)dp->data,
			   dp->len)) {
		ret = -EFAULT;
		goto free;
	}

	use_9b  = !(dp->pbc & HFI2_PBC_PACKETBYPASS_SMASK);

	if (use_9b) {
		bool dcinfo;
		u8 lrh0;

		lrh0 = *(u8 *)tmpbuf;
		dcinfo = !!(dp->pbc & PBC_DCINFO_SMASK);
		sc = hfi_lrh_vl_to_sc(lrh0, dcinfo);
		l2 = HFI_9B_L2;
	} else {
		sc = HFI_BYPASS_GET_SC(tmpbuf);
		l2 = HFI_BYPASS_GET_L2_TYPE(tmpbuf);
	}

	/* SC is checked against the PBC.VL for mgmt packets */
	vl = (dp->pbc >> PBC_VL_SHIFT) & PBC_VL_MASK;
	if (vl == 15 && sc != 15) {
		dd_dev_err(dd, "%s %d Mgmt VL mismatch LRH.SC(%u)\n",
			   __func__, __LINE__, sc);
		ret = -EINVAL;
		goto free;
	}

	ret = diagpkt_xmit(dd, tmpbuf, dp, sc, l2);
free:
	hfi_at_dereg_range(diag->ctx, tmpbuf, dp->len);
	vfree(tmpbuf);
bail:
	if (ret < 0 && dd)
		dd_dev_err(dd, "%s %d err %ld\n", __func__, __LINE__, ret);
	return ret;
}

static ssize_t diagpkt_write(struct file *fp, const char __user *data,
			     size_t count, loff_t *off)
{
	struct hfi_devdata *dd;
	struct diag_pkt dp;

	if (count != sizeof(dp))
		return -EINVAL;

	if (copy_from_user(&dp, data, sizeof(dp)))
		return -EFAULT;

	dd = hfi_get_unit_dd(dp.unit);
	if (!dd)
		return -EINVAL;
	if (!dp.port || dp.port > dd->num_pports)
		return -EINVAL;

	return diagpkt_send(&dp, dd);
}

ssize_t snoop_diagpkt_send(struct hfi_devdata *dd, const char __user *data,
			   size_t count, u8 port, u64 pbc)
{
	struct diag_pkt dp;

	memset(&dp, 0, sizeof(struct diag_pkt));
	dp.version = _DIAG_PKT_VERS;
	dp.unit = dd->unit;
	dp.port = port;
	dp.len = count;
	dp.data = (unsigned long)data;
	dp.pbc = pbc;
	dp.flags = F_DIAGPKT_WAIT;

	return diagpkt_send(&dp, dd);
}

static const struct file_operations diagpkt_file_ops = {
	.owner = THIS_MODULE,
	/* TODO: Convert to IOCTL? */
	.write = diagpkt_write,
	.llseek = noop_llseek,
};

int hfi2_diag_init(struct hfi_devdata *dd)
{
	struct hfi2_diagpkt_data *diag = &dd->hfi2_diag;
	struct opa_ev_assign eq_alloc_tx = {0};
	/*
	 * The HW resources are shared with VPD's port 0. The CMDQ can be used
	 * to drive transfers on both ports so this should not be a problem.
	 * An optimization is to use the per port VPD resources to perform
	 * the transfers if this becomes a bottleneck.
	 */
	struct hfi2_ibtx *ibtx;
	struct hfi_ctx *ctx;
	int rc;

	if (no_verbs)
		return 0;

	ibtx = &dd->ibd->pport[0].port_tx[0];
	ctx = ibtx->ctx;
	/* Reuse verbs context and command queue for diag */
	diag->cmdq_tx = &ibtx->cmdq.tx;
	diag->pend_cmdq = &ibtx->pend_cmdq;

	spin_lock_init(&diag->cmdq_tx_lock);

	/* Allocate a new EQ to track sent packets */
	eq_alloc_tx.ni = HFI_NI_BYPASS;
	eq_alloc_tx.count = HFI2_NUM_DIAGPKT_EVT;
	eq_alloc_tx.cookie = dd;
	rc = _hfi_eq_alloc(ctx, &eq_alloc_tx, &diag->eq_tx);
	if (rc)
		return rc;

	/* set last to mark diag interface as ready for use */
	diag->ctx = ctx;
	return 0;
}

void hfi2_diag_uninit(struct hfi_devdata *dd)
{
	struct hfi2_diagpkt_data *diag = &dd->hfi2_diag;

	if (diag->ctx)
		_hfi_eq_free(&diag->eq_tx);
}

int hfi2_diag_add(void)
{
	char name[16];
	int ret = 0;
	struct miscdevice *mdev;

	snprintf(name, sizeof(name), "%s_diagpkt", DRIVER_NAME);
	mdev = &diagpkt_miscdev;
	mdev->minor = MISC_DYNAMIC_MINOR;
	mdev->name = name,
	mdev->fops = &diagpkt_file_ops,

	ret = misc_register(mdev);
	if (ret)
		pr_err("Unable to create diagpkt device: %d", ret);

	return ret;
}

void hfi2_diag_remove(void)
{
	misc_deregister(&diagpkt_miscdev);
}
