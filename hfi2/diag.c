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

#include "opa_hfi.h"
#include "verbs/verbs.h"
#include <rdma/hfi_args.h>
#include <rdma/hfi_eq.h>
#include <rdma/hfi_pt.h>
#include <rdma/hfi_rx.h>
#include <rdma/hfi_tx.h>

#define HFI2_NUM_DIAGPKT_EVT 2048
#define HFI2_DIAG_TIMEOUT_MS 100
#define HFI2_DIAG_DEFAULT_MTU 8192
#define PBC_VL_SHIFT 12
#define PBC_VL_MASK 0xfull
#define HFI_BYPASS_L2_OFFSET    1
#define HFI_BYPASS_L2_SHFT      29
#define HFI_BYPASS_L2_MASK      0x3ull

#define HFI_BYPASS_GET_L2_TYPE(data) \
	((*((u32 *)(data) + HFI_BYPASS_L2_OFFSET) >> \
	  HFI_BYPASS_L2_SHFT) & HFI_BYPASS_L2_MASK)

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

/* diag_pkt flags */
#define F_DIAGPKT_WAIT 0x1      /* wait until packet is sent */

static int diagpkt_xmit(struct hfi_devdata *dd, void *buf,
			u32 pkt_len, struct diag_pkt *dp, u8 sl)
{
	struct hfi2_diagpkt_data *diag = &dd->hfi2_diag;
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, dp->port);
	struct hfi_ctx *ctx = diag->ctx;
	struct hfi_cq *tx = diag->cq_tx;
	struct hfi_cq *rx = diag->cq_rx;
	u64 *eq_entry = NULL;
	union base_iovec iov;
	unsigned long sflags;
	int rc, ms_delay = 0;
	hfi_eq_handle_t eq = HFI_EQ_NONE;

	/* Cannot send packets till port is not active */
	if (ppd->lstate != IB_PORT_ACTIVE) {
		rc = -ENODEV;
		dd_dev_err(dd, "%s %d lstate not active %d\n",
			   __func__, __LINE__, ppd->lstate);
		goto err;
	}
	iov.val[0] = 0x0;
	iov.val[1] = 0x0;
	iov.start = (u64)buf;
	iov.length = pkt_len;
	iov.ep = 1;
	iov.sp = 1;
	iov.v = 1;
	if (iov.length > HFI2_DIAG_DEFAULT_MTU) {
		rc = -ENOSPC;
		dd_dev_err(dd, "%s %d iov.length %d > max MTU %d\n",
			   __func__, __LINE__, iov.length,
			   HFI2_DIAG_DEFAULT_MTU);
		goto err;
	}

	/* Wait for an event only if requested by the user */
	if (dp->flags & F_DIAGPKT_WAIT)
		eq = &diag->eq_tx;

	spin_lock_irqsave(diag->cq_tx_lock, sflags);
retry:
	rc = hfi_tx_cmd_bypass_dma(tx, ctx, (u64)&iov, 1, 0xdead,
				   PTL_MD_RESERVED_IOV,
				   eq, HFI_CT_NONE,
				   dp->port - 1,
				   sl, ppd->lid,
				   HFI_BYPASS_GET_L2_TYPE(buf),
				   GENERAL_DMA);
	if (rc < 0) {
		mdelay(1);
		if (ms_delay++ > HFI2_DIAG_TIMEOUT_MS) {
			rc = -ETIME;
			dd_dev_err(dd, "%s %d rc %d\n",
				   __func__, __LINE__, rc);
			goto unlock;
		}
		goto retry;
	}

	if (eq == HFI_EQ_NONE)
		goto unlock;
	rc = hfi_eq_wait_timed(ctx, &diag->eq_tx, HFI2_DIAG_TIMEOUT_MS,
			       &eq_entry);
	if (eq_entry) {
		spin_lock(diag->cq_rx_lock);
		hfi_eq_advance(ctx, rx, &diag->eq_tx, eq_entry);
		spin_unlock(diag->cq_rx_lock);
		rc = 0;
	} else {
		dd_dev_err(dd, "TX event 1 failure, %d L2 %lld\n", rc,
			   HFI_BYPASS_GET_L2_TYPE(buf));
	}
unlock:
	spin_unlock_irqrestore(diag->cq_tx_lock, sflags);
err:
	return rc;
}

/**
 * diagpkt_send - send a packet
 * @dp: diag packet descriptor
 */
static ssize_t diagpkt_send(struct diag_pkt *dp, struct hfi_devdata *dd, u8 sl)
{
	u32 *tmpbuf = NULL;
	ssize_t ret = 0;
	u32 pkt_len;

	if (!dd || !dd->kregbase) {
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

	if (copy_from_user(tmpbuf,
			   (const void __user *)(unsigned long)dp->data,
			   dp->len)) {
		ret = -EFAULT;
		goto free;
	}

	/* pkt_len is how much data to write, includes header and data */
	pkt_len = dp->len >> 2;
	diagpkt_xmit(dd, tmpbuf, pkt_len, dp, sl);
free:
	vfree(tmpbuf);
bail:
	if (ret < 0 && dd != NULL)
		dd_dev_err(dd, "%s %d err %ld\n", __func__, __LINE__, ret);
	return ret;
}

static int compute_sl_from_vl(struct hfi_devdata *dd, u8 vl, u8 port)
{
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	int i;

	for (i = 0; i < ARRAY_SIZE(ppd->sc_to_vlt); i++) {
		if (ppd->sc_to_vlt[i] == vl)
			return ppd->sc_to_sl[i];
	}
	return -EINVAL;
}

static ssize_t diagpkt_write(struct file *fp, const char __user *data,
			     size_t count, loff_t *off)
{
	struct hfi_devdata *dd = container_of(fp->private_data,
					      struct hfi_devdata, hfi2_diag);
	u8 vl, sl = 0;
	struct diag_pkt dp;
	int ret;

	if (count != sizeof(dp))
		return -EINVAL;

	if (copy_from_user(&dp, data, sizeof(dp)))
		return -EFAULT;

	/* there is a maximum of 2 ports */
	if (!dp.port || dp.port > 2)
		return -EINVAL;

	/* the SL is computed from the VL */
	if (dp.pbc) {
		vl = (dp.pbc >> PBC_VL_SHIFT) & PBC_VL_MASK;
		ret = compute_sl_from_vl(dd, vl, dp.port);
		if (ret < 0)
			return ret;
		sl = ret;
	}
	return diagpkt_send(&dp, dd, sl);
}

static const struct file_operations diagpkt_file_ops = {
	.owner = THIS_MODULE,
	/* TODO: Convert to IOCTL? */
	.write = diagpkt_write,
	.llseek = noop_llseek,
};

/* TODO - delete and make common for all kernel clients */
static int _hfi_eq_alloc(struct hfi_ctx *ctx, struct hfi_cq *cq,
			 spinlock_t *cq_lock,
			 struct opa_ev_assign *eq_alloc,
			 struct hfi_eq *eq)
{
	u32 *eq_head_array, *eq_head_addr;
	u64 *eq_entry = NULL;
	int rc;

	eq_alloc->base = (u64)vzalloc(eq_alloc->count * HFI_EQ_ENTRY_SIZE);
	if (!eq_alloc->base)
		return -ENOMEM;
	eq_alloc->mode = OPA_EV_MODE_BLOCKING;
	rc = ctx->ops->ev_assign(ctx, eq_alloc);
	if (rc < 0) {
		vfree((void *)eq_alloc->base);
		goto err;
	}
	eq_head_array = ctx->eq_head_addr;
	/* Reset the EQ SW head */
	eq_head_addr = &eq_head_array[eq_alloc->ev_idx];
	*eq_head_addr = 0;
	eq->idx = eq_alloc->ev_idx;
	eq->base = (void *)eq_alloc->base;
	eq->count = eq_alloc->count;

	/* Check on EQ 0 NI 0 for a PTL_CMD_COMPLETE event */
	hfi_eq_wait_timed(ctx, &ctx->eq_zero[0], HFI2_DIAG_TIMEOUT_MS,
			  &eq_entry);
	if (eq_entry) {
		unsigned long flags;

		spin_lock_irqsave(cq_lock, flags);
		hfi_eq_advance(ctx, cq, &ctx->eq_zero[0], eq_entry);
		spin_unlock_irqrestore(cq_lock, flags);
	} else {
		rc = -EIO;
	}
err:
	return rc;
}

/* TODO - delete and make common for all kernel clients */
static void _hfi_eq_release(struct hfi_ctx *ctx, struct hfi_cq *cq,
			    spinlock_t *cq_lock, struct hfi_eq *eq)
{
	u64 *eq_entry = NULL, done;

	ctx->ops->ev_release(ctx, 0, eq->idx, (u64)&done);
	/* Check on EQ 0 NI 0 for a PTL_CMD_COMPLETE event */
	hfi_eq_wait_timed(ctx, &ctx->eq_zero[0], HFI2_DIAG_TIMEOUT_MS,
			  &eq_entry);
	if (eq_entry) {
		unsigned long flags;

		spin_lock_irqsave(cq_lock, flags);
		hfi_eq_advance(ctx, cq, &ctx->eq_zero[0], eq_entry);
		spin_unlock_irqrestore(cq_lock, flags);
	}

	vfree(eq->base);
	eq->base = NULL;
}

static int hfi2_diag_init(struct hfi_devdata *dd)
{
	struct hfi2_diagpkt_data *diag = &dd->hfi2_diag;
	struct opa_ev_assign eq_alloc_tx = {0};
	/*
	 * The HW resources are shared with VPD's port 0. The CQ can be used
	 * to drive transfers on both ports so this should not be a problem.
	 * An optimization is to use the per port VPD resources to perform
	 * the transfers if this becomes a bottleneck.
	 */
	struct hfi2_ibport *ibp = &dd->ibd->pport[0];
	int rc;

	/* Reuse verbs context and command queue for diag */
	diag->ctx = &dd->ibd->sm_ctx;
	diag->cq_tx = &ibp->cmdq_tx;
	diag->cq_rx = &ibp->cmdq_rx;
	diag->cq_tx_lock = &ibp->cmdq_tx_lock;
	diag->cq_rx_lock = &ibp->cmdq_rx_lock;

	/* Allocate a new EQ to track sent packets */
	eq_alloc_tx.ni = HFI_NI_BYPASS;
	eq_alloc_tx.user_data = 0xdeadbeef;
	eq_alloc_tx.count = HFI2_NUM_DIAGPKT_EVT;
	eq_alloc_tx.cookie = dd;
	rc = _hfi_eq_alloc(diag->ctx, diag->cq_rx, diag->cq_rx_lock,
			   &eq_alloc_tx, &diag->eq_tx);
	return rc;
}

static void hfi2_diag_uninit(struct hfi_devdata *dd)
{
	struct hfi2_diagpkt_data *diag = &dd->hfi2_diag;

	_hfi_eq_release(diag->ctx, diag->cq_rx, diag->cq_rx_lock, &diag->eq_tx);
}

int hfi2_diag_add(struct hfi_devdata *dd)
{
	char name[16];
	int ret = 0;
	struct miscdevice *mdev;

	/* TODO - delete hfi1 sometime? */
	snprintf(name, sizeof(name), "%s_diagpkt", "hfi1");

	mdev = &dd->hfi2_diag.miscdev;
	mdev->minor = MISC_DYNAMIC_MINOR;
	mdev->name = name,
	mdev->fops = &diagpkt_file_ops,
	mdev->parent = &dd->pcidev->dev;

	hfi2_diag_init(dd);

	ret = misc_register(mdev);
	if (ret)
		dd_dev_err(dd, "Unable to create diagpkt device: %d", ret);

	return ret;
}

void hfi2_diag_remove(struct hfi_devdata *dd)
{
	misc_deregister(&dd->hfi2_diag.miscdev);
	hfi2_diag_uninit(dd);
}
