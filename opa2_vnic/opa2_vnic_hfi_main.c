/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2015 Intel Corporation.
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
 * Copyright (c) 2015 Intel Corporation.
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

/*
 * Intel(R) Omni-Path Gen2 specific support for VNIC functionality.
 */
#include <linux/etherdevice.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/vmalloc.h>
#include <rdma/opa_core.h>
#include <rdma/hfi_tx.h>
#include <rdma/hfi_rx.h>
#include <rdma/hfi_args.h>
#include <rdma/hfi_ct.h>
#include <rdma/hfi_eq.h>
#include <rdma/hfi_pt.h>
#include <rdma/hfi_me.h>
#include <linux/opa_vnic.h>

static int opa_netdev_probe(struct opa_core_device *odev);
static void opa_netdev_remove(struct opa_core_device *odev);
static void opa_netdev_event_notify(struct opa_core_device *odev,
				    enum opa_core_event event, u8 port);

static struct opa_core_client opa_vnic_clnt = {
	.name = KBUILD_MODNAME,
	.add = opa_netdev_probe,
	.remove = opa_netdev_remove,
	.event_notify = opa_netdev_event_notify
};

#define OPA2_MAX_NUM_PORTS 2
#define OPA2_NET_TIMEOUT_MS 100
#define OPA2_NET_RX_POLL_MS 1
#define OPA2_NET_NUM_RX_BUFS 2048
/*
 * FXRTODO:
 * Obtain MTU from vdev once supported
 * We could also use the default MAX OPA MTU once Simics supports
 * transfers larger than 8192 bytes.
 */
#define OPA2_NET_DEFAULT_MTU 8192
#define OPA2_NET_EAGER_SIZE (PAGE_SIZE * 4)

#define OPA_VNIC_ICRC_LEN   4
#define OPA_VNIC_TAIL_LEN   1
#define OPA_VNIC_ICRC_TAIL_LEN  (OPA_VNIC_ICRC_LEN + OPA_VNIC_TAIL_LEN)

#define OPA_VNIC_L4_TYPE_OFFSET 8
#define OPA_VNIC_L4_TYPE_MASK   0xf
#define OPA_VNIC_VESWID_OFFSET 10
#define OPA_VNIC_SC_OFFSET 6
#define OPA_VNIC_SC_SHIFT 4

#define OPA_VNIC_L4_ETHR 0
#define OPA_VNIC_L4_EEPH 1

#define OPA_VNIC_GET_L4_TYPE(data)   \
	(*((u8 *)(data) + OPA_VNIC_L4_TYPE_OFFSET) & OPA_VNIC_L4_TYPE_MASK)

#define OPA_VNIC_GET_VESWID(data)   \
	(*((u8 *)(data) + OPA_VNIC_VESWID_OFFSET))

/*
 * struct opa_veswport - OPA2 virtual ethernet switch port specific fields
 *
 * @slid: source LID
 * @vdev: back pointer to opa vnic device
 * @odev: back pointer to opa core device
 * @ndev: back pointer to opa net device
 * @vport_num: virtual ethernet switch port id
 * @port_num: physical port number
 * @init: true if veswport is initialized
 * @vnic_cb: vnic callback function
 * @skbq: Queue of received socket buffers
 */
struct opa_veswport {
	int			slid;
	struct opa_vnic_device	*vdev;
	struct opa_core_device	*odev;
	struct opa_netdev	*ndev;
	u8			vport_num;
	u8			port_num;
	bool			init;
	opa_vnic_hfi_evt_cb_fn	__rcu vnic_cb;
	struct sk_buff_head	skbq;
};

/*
 * struct opa_netdev - OPA2 net device specific fields
 * @odev: OPA core device
 * @num_ports: number of physical ports
 * @num_vports: number of virtual switch ports including VNIC and EEPH
 * @vnic_ctrl_dev: VNIC control device for device instantiation
 * @ctrl_lock: Lock to synchronize setup and teardown of the ctrl device
 * @ctx_lock: Lock to synchronize setup and teardown of the bypass context
 * @tx_lock: Lock to synchronize access to the TX CQ
 * @rx_lock: Lock to synchronize access to the RX CQ
 * @ctx: HFI context
 * @cq_idx: command queue index
 * @tx: TX command queue
 * @rx: RX command queue
 * @buf: Array of receive buffers
 * @ni: portals network interface
 * @eq_tx: TX event handle
 * @eq_alloc_tx_base: buffer for TX EQ descriptors
 * @eq_rx: RX event handle
 * @eq_alloc_rx_base: buffer for RX EQ descriptors
 * @vesw_idr: IDR for looking up vesw information
 */
struct opa_netdev {
	struct opa_core_device		*odev;
	int				num_ports;
	int				num_vports;
	struct opa_vnic_ctrl_device	*vnic_ctrl_dev;
	struct opa_vnic_device		*eeph_dev;
	struct mutex			ctrl_lock;
	struct mutex			ctx_lock;
	spinlock_t			tx_lock;
	spinlock_t			rx_lock;
	struct hfi_ctx			ctx;
	u16				cq_idx;
	struct hfi_cq			tx;
	struct hfi_cq			rx;
	void				*buf[OPA2_NET_NUM_RX_BUFS];
	hfi_ni_t			ni;
	struct hfi_eq 			eq_tx;
	struct hfi_eq 			eq_rx;
	struct idr			vesw_idr[OPA2_MAX_NUM_PORTS];
};

/* TODO - delete and make common for all kernel clients */
static int _hfi_eq_alloc(struct hfi_ctx *ctx, struct hfi_cq *cq,
			 spinlock_t *cq_lock,
			 struct opa_ev_assign *eq_alloc,
			 struct hfi_eq *eq)
{
	u32 *eq_head_array, *eq_head_addr;
	u64 *eq_entry = NULL, done;
	int rc;

	eq_alloc->base = (u64)vzalloc(eq_alloc->count * HFI_EQ_ENTRY_SIZE);
	if (!eq_alloc->base)
		return -ENOMEM;
	eq_alloc->mode = OPA_EV_MODE_BLOCKING;
	eq_alloc->user_data = (u64)&done;
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
	hfi_eq_wait_timed(ctx, &ctx->eq_zero[0], OPA2_NET_TIMEOUT_MS,
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
	hfi_eq_wait_timed(ctx, &ctx->eq_zero[0], OPA2_NET_TIMEOUT_MS,
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

/* Append a single socket buffer */
static int opa2_vnic_append_skb(struct opa_netdev *ndev, int idx)
{
	struct hfi_ctx *ctx = &ndev->ctx;
	struct hfi_cq *rx = &ndev->rx;
	hfi_rx_cq_command_t rx_cmd;
	int n_slots, rc;
	u64 *eq_entry = NULL, done;
	unsigned long sflags;

	n_slots = hfi_format_rx_bypass(ctx, ndev->ni,
				       ndev->buf[idx],
				       OPA2_NET_EAGER_SIZE,
				       HFI_PT_BYPASS_EAGER,
				       ctx->ptl_uid,
				       PTL_OP_PUT,
				       HFI_CT_NONE,
				       0, (unsigned long)&done,
				       idx, &rx_cmd);

	spin_lock_irqsave(&ndev->rx_lock, sflags);
	rc = hfi_rx_command(rx, (uint64_t *)&rx_cmd, n_slots);
	spin_unlock_irqrestore(&ndev->rx_lock, sflags);
	if (rc < 0) {
		dev_err(&ndev->odev->dev, "%s rc %d\n", __func__, rc);
		return rc;
	}

	/* Check on EQ 0 NI 0 for a PTL_CMD_COMPLETE event */
	hfi_eq_wait_timed(ctx, &ctx->eq_zero[0], OPA2_NET_TIMEOUT_MS,
			  &eq_entry);
	if (eq_entry) {
		spin_lock_irqsave(&ndev->rx_lock, sflags);
		hfi_eq_advance(ctx, rx, &ctx->eq_zero[0], eq_entry);
		spin_unlock_irqrestore(&ndev->rx_lock, sflags);
	} else {
		return -EIO;
	}

	return 0;
}

/*
 * Transmit an SKB and wait for the transmission to complete by
 * polling on the TX counting event
 */
static int opa2_vnic_hfi_put_skb(struct opa_vnic_device *vdev,
				 struct sk_buff *skb)
{
	struct opa_veswport *dev = vdev->hfi_priv;
	struct opa_core_device *odev = dev->odev;
	struct opa_netdev *ndev = dev->ndev;
	struct hfi_ctx *ctx = &ndev->ctx;
	struct hfi_cq *tx = &ndev->tx;
	struct hfi_cq *rx = &ndev->rx;
	u64 *eq_entry = NULL;
	union base_iovec iov;
	unsigned long sflags;
	int rc, ms_delay = 0;

	iov.val[0] = 0x0;
	iov.val[1] = 0x0;
	iov.start = (u64)skb->data;
	if (vdev->is_eeph) {
		unsigned char *pad_info = NULL;

		pad_info = skb->data + skb->len - 1;
		iov.length = skb->len - OPA_VNIC_ICRC_TAIL_LEN -
				(*pad_info & 0x3f);
	} else {
		iov.length = skb->len;
	}
	if (iov.length > OPA2_NET_DEFAULT_MTU) {
		rc = -ENOSPC;
		dev_err(&odev->dev, "%s %d iov.length %d > max MTU %d\n",
			__func__, __LINE__, iov.length, OPA2_NET_DEFAULT_MTU);
		goto err;
	}
	iov.ep = 1;
	iov.sp = 1;
	iov.v = 1;

	spin_lock_irqsave(&ndev->tx_lock, sflags);
retry:
	rc = hfi_tx_cmd_bypass_dma(tx, ctx, (u64)&iov, 1, 0xdead,
				   PTL_MD_RESERVED_IOV,
				   &ndev->eq_tx, HFI_CT_NONE,
				   dev->port_num - 1,
				   0x0, dev->slid, HDR_10B,
				   GENERAL_DMA);
	if (rc < 0) {
		mdelay(1);
		if (ms_delay++ > OPA2_NET_TIMEOUT_MS) {
			rc = -ETIME;
			dev_err(&odev->dev, "%s %d rc %d\n",
				__func__, __LINE__, rc);
			goto err1;
		}
		goto retry;
	}

	/* FXRTODO: need to wait for completion asynchronously */
	rc = hfi_eq_wait_timed(ctx, &ndev->eq_tx, OPA2_NET_TIMEOUT_MS,
			       &eq_entry);
	if (eq_entry) {
		union initiator_EQEntry *txe =
			(union initiator_EQEntry *)eq_entry;
		int flit_len = ((*(u32 *)skb->data) >> 20) << 3;

		spin_lock(&ndev->rx_lock);
		hfi_eq_advance(ctx, rx, &ndev->eq_tx, eq_entry);
		spin_unlock(&ndev->rx_lock);
		rc = 0;
		dev_dbg(&odev->dev,
			"TX kind %d skb->len %4d flit_len %4d vpnum %d port %d\n",
			txe->event_kind, skb->len, flit_len,
			dev->vport_num, dev->port_num);
	} else {
		dev_err(&odev->dev, "TX CT event 1 failure, %d\n", rc);
	}
err1:
	spin_unlock_irqrestore(&ndev->tx_lock, sflags);
err:
	if (rc)
		vdev->hfi_stats.tx_logic_errors++;
	return rc;
}

/* Retrieve a SKB from the head of the list */
static struct sk_buff *opa2_vnic_hfi_get_skb(struct opa_vnic_device *vdev)
{
	struct opa_veswport *dev = vdev->hfi_priv;

	return skb_dequeue(&dev->skbq);
}

/* return 1 if there are buffers available to be read and 0 otherwise */
static int opa2_vnic_hfi_get_read_avail(struct opa_vnic_device *vdev)
{
	struct opa_veswport *dev = vdev->hfi_priv;

	return !skb_queue_empty(&dev->skbq);
}

/*
 * This RX ISR callback does the following:
 * a) queries the EQ for received packets
 * b) figures out the veswport the packet is destined for
 * c) allocates an SKB
 * d) copies the data from the eager buffer into the SKB
 * e) appends the skb to the appropriate RX queue
 * f) notifies the upper layer about a received packet
 * FXRTODO:
 * a) Need an efficient way to enable/disable interrupt notifications
 * b) There is a lot being done in the ISR. do we need to defer the
 * handling to a bottom half instead?
 */
static void opa2_vnic_rx_isr_cb(struct hfi_eq *eq_rx, void *data)
{
	struct opa_netdev *ndev = data;
	struct hfi_ctx *ctx = &ndev->ctx;
	u64 *eq_entry;
	union rhf *rhf;
	int l4_type, id = 0;
	opa_vnic_hfi_evt_cb_fn vnic_cb;
	struct opa_veswport *dev = NULL;
	struct opa_vnic_device *vdev = NULL;
	void *buf;
	unsigned long sflags;
	struct hfi_cq *rx = &ndev->rx;
	int len;
	unsigned char *pad_info = NULL;
	struct sk_buff *skb;

retry:
	skb = NULL;
	hfi_eq_peek(ctx, eq_rx, &eq_entry);
	if (!eq_entry)
		return;
	rhf = (union rhf *)eq_entry;

	if (rhf->egrindex >= OPA2_NET_NUM_RX_BUFS) {
		dev_err(&ndev->odev->dev,
			"packet drop: incorrect eager index %d\n",
			rhf->egrindex);
		spin_lock_irqsave(&ndev->rx_lock, sflags);
		hfi_eq_advance(ctx, &ndev->rx, eq_rx, eq_entry);
		spin_unlock_irqrestore(&ndev->rx_lock, sflags);
		return;
	}
	buf = ndev->buf[rhf->egrindex];
	l4_type = OPA_VNIC_GET_L4_TYPE(buf);
	rcu_read_lock();
	if (l4_type == OPA_VNIC_L4_EEPH) {
		vdev = ndev->eeph_dev;
	} else if (l4_type == OPA_VNIC_L4_ETHR) {
		/* FXRTODO: Check performance impact of idr_find */
		id = OPA_VNIC_GET_VESWID(buf);
		vdev = idr_find(&ndev->vesw_idr[rhf->pt], id);
	}
	/* Drop the packet if a vdev is not available */
	if (!vdev)
		goto pkt_drop;
	dev = vdev->hfi_priv;
	/* RHF packet length is in dwords */
	len = rhf->pktlen << 2;
	if (len > OPA2_NET_DEFAULT_MTU) {
		dev_err(&ndev->odev->dev,
			"packet drop: eager buf len %d > max MTU %d\n",
			len, OPA2_NET_DEFAULT_MTU);
		goto pkt_drop;
	}
	if (!vdev->is_eeph) {
		pad_info = buf + len - 1;
		len -= *pad_info & 0x3f;
	}
	/*
	 * Allocate the skb and then copy the data from the eager
	 * buffer into the skb
	 */
	skb = netdev_alloc_skb(vdev->netdev, len);
	if (!skb) {
		vdev->hfi_stats.rx_missed_errors++;
		goto pkt_drop;
	}
	memcpy(skb->data, buf, len);
	skb_put(skb, len);
	skb_queue_tail(&dev->skbq, skb);
	dev_dbg(&ndev->odev->dev, "RX kind %d skb->len %4d flit_len %4d "
		"(pad & 0x3f) %d vpnum %d port %d egridx %d egroffset %d\n",
		rhf->event_kind, skb->len, rhf->pktlen << 2,
		pad_info ? *pad_info & 0x3f : 0x0,
		dev->vport_num, rhf->pt + 1,
		rhf->egrindex, rhf->egroffset);
pkt_drop:
	spin_lock_irqsave(&ndev->rx_lock, sflags);
	hfi_pt_update_eager(ctx, rx, rhf->egrindex);
	/* FXRTODO: error handling? */
	hfi_eq_advance(ctx, rx, eq_rx, eq_entry);
	spin_unlock_irqrestore(&ndev->rx_lock, sflags);
	/* Notify the upper layer about a received packet */
	if (skb) {
		vnic_cb = rcu_dereference(dev->vnic_cb);
		if (vnic_cb)
			vnic_cb(vdev, OPA_VNIC_HFI_EVT_RX);
	}
	rcu_read_unlock();
	/* Check if there are more events */
	goto retry;
}

/* Free any RX buffers previously allocated */
static void opa2_free_rx_bufs(struct opa_netdev *ndev)
{
	int i = 0;

	for (i = 0; i < OPA2_NET_NUM_RX_BUFS; i++) {
		if (ndev->buf[i]) {
			vfree(ndev->buf[i]);
			ndev->buf[i] = NULL;
		}
	}
}

/* Allocate and append RX buffers */
static int opa2_alloc_rx_bufs(struct opa_netdev *ndev)
{
	int i, rc = 0;

	for (i = 0; i < OPA2_NET_NUM_RX_BUFS; i++) {
		ndev->buf[i] = vzalloc(OPA2_NET_EAGER_SIZE);
		if (!ndev->buf[i]) {
			rc = -ENOMEM;
			goto err1;
		}

		rc = opa2_vnic_append_skb(ndev, i);
		if (rc)
			goto err1;
	}
	return rc;
err1:
	opa2_free_rx_bufs(ndev);
	return rc;
}

/*
 * Initialization tasks for TX/RX data paths. These include:
 * setting up the SLID and allocating EQ for TX
 */
static int opa2_init_tx_rx(struct opa_veswport *dev)
{
	struct opa_core_ops *ops = dev->odev->bus_ops;
	struct opa_core_device *odev = dev->odev;
	struct opa_pport_desc pdesc;

	ops->get_port_desc(odev, &pdesc, dev->port_num);
	dev->slid = pdesc.lid;

	return 0;
}

static void opa2_uninit_tx_rx(struct opa_veswport *dev)
{
}

static int opa2_vnic_hfi_open(struct opa_vnic_device *vdev,
			      opa_vnic_hfi_evt_cb_fn cb)
{
	struct opa_veswport *dev = vdev->hfi_priv;
	struct opa_netdev *ndev = dev->ndev;
	int rc;

	if (!cb)
		return -EINVAL;

	/*
	 * Only get reference count for eeph device. For network device, the
	 * linux network stack decouples ethernet interface and the driver.
	 * Hence, no need to get reference count for network device.
	 */
	if (vdev->is_eeph) {
		rc = hfi_vdev_get(vdev);
		if (rc) {
			dev_err(&ndev->odev->dev, "%s rc %d %d\n",
				__func__, __LINE__, rc);
			goto err;
		}
		ndev->eeph_dev = vdev;
	} else {
		/* ensure virtual eth switch id is valid */
		if (!vdev->vesw_id)
			return -EINVAL;

		rc = idr_alloc(&ndev->vesw_idr[dev->port_num - 1],
			       vdev, vdev->vesw_id,
			       vdev->vesw_id + 1, GFP_NOWAIT);
		if (rc < 0) {
			dev_err(&ndev->odev->dev, "%s %d rc %d vesw %d "
				"port_num %d vport_num %d\n",
				__func__, __LINE__, rc, vdev->vesw_id,
				dev->port_num, dev->vport_num);
			goto err;
		}
	}
	rc = opa2_init_tx_rx(dev);
	if (rc) {
		dev_err(&ndev->odev->dev, "%s rc %d %d\n",
			__func__, __LINE__, rc);
		goto err1;
	}
	skb_queue_head_init(&dev->skbq);
	dev->vdev = vdev;
	rcu_assign_pointer(dev->vnic_cb, cb);
	synchronize_rcu();
	return 0;
err1:
	if (vdev->is_eeph)
		hfi_vdev_put(vdev);
	else
		idr_remove(&ndev->vesw_idr[dev->port_num - 1], vdev->vesw_id);
err:
	return rc;
}

static void opa2_vnic_hfi_close(struct opa_vnic_device *vdev)
{
	struct opa_veswport *dev = vdev->hfi_priv;
	struct opa_netdev *ndev = dev->ndev;

	if (vdev->is_eeph)
		ndev->eeph_dev = NULL;
	else
		idr_remove(&ndev->vesw_idr[dev->port_num - 1], vdev->vesw_id);

	rcu_assign_pointer(dev->vnic_cb, NULL);
	synchronize_rcu();
	skb_queue_purge(&dev->skbq);

	opa2_uninit_tx_rx(dev);
	if (vdev->is_eeph)
		hfi_vdev_put(vdev);
}

static int opa2_vnic_init_ctx(struct opa_vnic_device *vdev)
{
	struct opa_veswport *dev = vdev->hfi_priv;
	struct opa_netdev *ndev = dev->ndev;
	struct hfi_ctx *ctx = &ndev->ctx;
	struct hfi_cq *rx = &ndev->rx;
	struct opa_core_device *odev = dev->odev;
	struct opa_core_ops *ops = odev->bus_ops;
	struct opa_ctx_assign ctx_assign = {0};
	struct opa_ev_assign eq_alloc_rx = {0};
	struct opa_ev_assign eq_alloc_tx = {0};
	hfi_pt_alloc_eager_args_t pt_alloc = {0};
	int rc;

	HFI_CTX_INIT(ctx, odev->dd, odev->bus_ops);
	ctx->mode |= HFI_CTX_MODE_BYPASS_10B;
	ctx_assign.pid = HFI_PID_ANY;
	ctx_assign.le_me_count = OPA2_NET_NUM_RX_BUFS;
	rc = ops->ctx_assign(ctx, &ctx_assign);
	if (rc)
		return rc;
	/*
	 * FXRTODO: Obtain a pair of command queues and set it up.
	 * The pair of CQs is shared across both ports since there are
	 * only 192 CQs for the entire chip. It might be better to have
	 * separate CQs for each port for scalability reasons as a
	 * potential future performance optimization.
	 */
	rc = ops->cq_assign(ctx, NULL, &ndev->cq_idx);
	if (rc)
		goto err;
	rc = ops->cq_map(ctx, ndev->cq_idx, &ndev->tx, &ndev->rx);
	if (rc)
		goto err1;
	ndev->ni = HFI_NI_BYPASS;

	/* TX EQ can be allocated per netdev */
	eq_alloc_tx.ni = ndev->ni;
	eq_alloc_tx.user_data = 0xdeadbeef;
	eq_alloc_tx.count = OPA2_NET_NUM_RX_BUFS;
	eq_alloc_tx.cookie = ndev;
	rc = _hfi_eq_alloc(ctx, rx, &ndev->rx_lock, &eq_alloc_tx,
			   &ndev->eq_tx);
	if (rc)
		goto err2;
	eq_alloc_rx.ni = ndev->ni;
	eq_alloc_rx.user_data = 0xdeadbeef;
	eq_alloc_rx.count = OPA2_NET_NUM_RX_BUFS;
	eq_alloc_rx.isr_cb = opa2_vnic_rx_isr_cb;
	eq_alloc_rx.cookie = ndev;
	rc = _hfi_eq_alloc(ctx, rx, &ndev->rx_lock, &eq_alloc_rx,
			   &ndev->eq_rx);
	if (rc)
		goto err3;
	pt_alloc.eager_order = ilog2(OPA2_NET_NUM_RX_BUFS) - 2;
	pt_alloc.eq_handle = &ndev->eq_rx;
	rc = hfi_pt_alloc_eager(ctx, rx, &pt_alloc);
	if (rc < 0)
		goto err4;
	rc = opa2_alloc_rx_bufs(ndev);
	if (rc < 0)
		goto err5;

	return 0;
err5:
	hfi_pt_disable(ctx, &ndev->rx, ndev->ni, HFI_PT_BYPASS_EAGER);
err4:
	_hfi_eq_release(ctx, rx, &ndev->rx_lock, &ndev->eq_rx);
err3:
	_hfi_eq_release(ctx, rx, &ndev->rx_lock, &ndev->eq_tx);
err2:
	ops->cq_unmap(&ndev->tx, &ndev->rx);
err1:
	ops->cq_release(ctx, ndev->cq_idx);
err:
	ops->ctx_release(ctx);
	dev_err(&odev->dev, "%s rc %d\n", __func__, rc);
	return rc;
}

static void opa2_vnic_uninit_ctx(struct opa_vnic_device *vdev)
{
	struct opa_veswport *dev = vdev->hfi_priv;
	struct opa_netdev *ndev = dev->ndev;
	struct hfi_ctx *ctx = &ndev->ctx;
	struct opa_core_device *odev = dev->odev;
	struct opa_core_ops *ops = odev->bus_ops;

	hfi_pt_disable(ctx, &ndev->rx, ndev->ni, HFI_PT_BYPASS_EAGER);
	opa2_free_rx_bufs(ndev);
	_hfi_eq_release(ctx, &ndev->rx, &ndev->rx_lock, &ndev->eq_rx);
	_hfi_eq_release(ctx, &ndev->rx, &ndev->rx_lock, &ndev->eq_tx);
	ops->cq_unmap(&ndev->tx, &ndev->rx);
	ops->cq_release(ctx, ndev->cq_idx);
	odev->bus_ops->ctx_release(ctx);
}

static int opa2_vnic_hfi_init(struct opa_vnic_device *vdev)
{
	struct opa_veswport *dev = vdev->hfi_priv;
	struct opa_core_device *odev = dev->odev;
	struct opa_netdev *ndev = dev->ndev;
	int rc = 0, i;

	mutex_lock(&ndev->ctx_lock);
	if (ndev->num_vports)
		goto finish;

	for (i = 0; i < ndev->num_ports; i++)
		idr_init(&ndev->vesw_idr[i]);
	rc = opa2_vnic_init_ctx(vdev);
	if (rc) {
		dev_err(&odev->dev, "alloc_ctx fail rc %d\n", rc);
		goto fail;
	}
finish:
	ndev->num_vports++;
fail:
	mutex_unlock(&ndev->ctx_lock);
	if (rc)
		dev_err(&odev->dev, "%s rc %d\n", __func__, rc);
	return rc;
}

static void opa2_vnic_hfi_deinit(struct opa_vnic_device *vdev)
{
	struct opa_veswport *dev = vdev->hfi_priv;
	struct opa_netdev *ndev = dev->ndev;
	int i;

	mutex_lock(&ndev->ctx_lock);
	if (!--ndev->num_vports) {
		opa2_vnic_uninit_ctx(vdev);
		for (i = 0; i < ndev->num_ports; i++)
			idr_destroy(&ndev->vesw_idr[i]);
	}
	mutex_unlock(&ndev->ctx_lock);
}

static struct opa_vnic_hfi_ops vnic_ops = {
	.hfi_init = opa2_vnic_hfi_init,
	.hfi_deinit = opa2_vnic_hfi_deinit,
	.hfi_open = opa2_vnic_hfi_open,
	.hfi_close = opa2_vnic_hfi_close,
	.hfi_put_skb = opa2_vnic_hfi_put_skb,
	.hfi_get_skb = opa2_vnic_hfi_get_skb,
	.hfi_get_read_avail = opa2_vnic_hfi_get_read_avail,
};

/* hfi_vdev_create - add vnic device on vnic bus */
static int hfi_vdev_create(struct opa_vnic_ctrl_device *cdev,
			   u8 port_num, u8 vport_num, u8 is_eeph)
{
	int rc;
	struct opa_netdev *ndev = cdev->hfi_priv;
	struct opa_veswport *vport;
	struct opa_vnic_device *vdev;

	if (!port_num || port_num > ndev->num_ports)
		return -EINVAL;

	vport = kzalloc(sizeof(*vport), GFP_KERNEL);
	if (!vport)
		return -ENOMEM;
	vport->port_num = port_num;
	vport->vport_num = vport_num;
	vport->odev = ndev->odev;
	vport->ndev = opa_core_get_priv_data(&opa_vnic_clnt, ndev->odev);

	vdev = opa_vnic_device_register(cdev, port_num, vport_num,
					vport, &vnic_ops, is_eeph);
	if (IS_ERR(vdev)) {
		rc = PTR_ERR(vdev);
		dev_err(&ndev->odev->dev, "error adding vport %d\n", rc);
		goto reg_err;
	}
	return 0;
reg_err:
	kfree(vport);
	return rc;
}

/* hfi_vdev_destroy - remove vnic device from vnic bus */
static void hfi_vdev_destroy(struct opa_vnic_ctrl_device *cdev,
			     u8 port_num, u8 vport_num, u8 is_eeph)
{
	struct opa_vnic_device *vdev;
	struct opa_veswport *vport;

	if (is_eeph)
		vdev = opa_vnic_get_eeph_dev(cdev, port_num);
	else
		vdev = opa_vnic_get_dev(cdev, port_num, vport_num);

	if (!vdev)
		return;

	vport = vdev->hfi_priv;
	opa_vnic_device_unregister(vdev);
	kfree(vport);
}

/* opa_vnic_hfi_add_vport - add vnic port */
static int opa_vnic_hfi_add_vport(struct opa_vnic_ctrl_device *cdev,
				  u8 port_num, u8 vport_num)
{
	int rc;
	struct opa_netdev *ndev = cdev->hfi_priv;
	struct opa_core_device *odev = ndev->odev;

	rc = hfi_vdev_create(cdev, port_num, vport_num, 0);
	if (rc)
		dev_err(&odev->dev, "error adding vnic port (%d:%d): %d\n",
			port_num, vport_num, rc);
	return rc;
}

/* opa_vnic_hfi_rem_vport - remove vnic port */
static void opa_vnic_hfi_rem_vport(struct opa_vnic_ctrl_device *cdev,
				   u8 port_num, u8 vport_num)
{
	hfi_vdev_destroy(cdev, port_num, vport_num, 0);
}

/* opa_vnic_hfi_rem_eeph_ports - remove vnic eeph ports */
static void opa_vnic_hfi_rem_eeph_ports(struct opa_vnic_ctrl_device *cdev)
{
	int i;

	for (i = 0; i < cdev->num_ports; i++)
		hfi_vdev_destroy(cdev, i + 1, 0, 1);
}

/* opa_vnic_hfi_add_eeph_ports - add vnic eeph ports */
static int opa_vnic_hfi_add_eeph_ports(struct opa_vnic_ctrl_device *cdev)
{
	int i, rc = 0;
	struct opa_netdev *ndev = cdev->hfi_priv;
	struct opa_core_device *odev = ndev->odev;

	for (i = 0; i < cdev->num_ports; i++) {
		rc = hfi_vdev_create(cdev, i + 1, 0, 1);
		if (rc) {
			dev_err(&odev->dev, "error adding eeph port %d: %d\n",
				i + 1, rc);
			break;
		}
	}

	if (rc)
		opa_vnic_hfi_rem_eeph_ports(cdev);

	return rc;
}

/* vnic control operations */
static struct opa_vnic_ctrl_ops vnic_ctrl_ops = {
	.add_vport = opa_vnic_hfi_add_vport,
	.rem_vport = opa_vnic_hfi_rem_vport
};

/* opa_vnic_hfi_add_ctrl_port - Add vnic control port device */
int opa_vnic_hfi_add_ctrl_port(struct opa_netdev *ndev)
{
	struct opa_vnic_ctrl_device *cdev;
	int rc;
	struct opa_core_device *odev = ndev->odev;
	struct opa_dev_desc desc;

	/* Return success if control device is already added */
	if (ndev->vnic_ctrl_dev)
		return 0;

	odev->bus_ops->get_device_desc(odev, &desc);
	ndev->num_ports = desc.num_pports;

	cdev = opa_vnic_ctrl_device_register(odev->dev.parent, desc.ibdev,
					     ndev->num_ports, ndev,
					     &vnic_ctrl_ops);
	if (IS_ERR(cdev)) {
		rc = PTR_ERR(cdev);
		dev_err(&odev->dev, "error adding vnic control port %d\n", rc);
		return rc;
	}
	rc = opa_vnic_hfi_add_eeph_ports(cdev);
	if (rc) {
		opa_vnic_ctrl_device_unregister(cdev);
		return rc;
	}

	ndev->vnic_ctrl_dev = cdev;
	return 0;
}

/* opa_vnic_hfi_rem_ctrl_port - remove vnic control port device */
void opa_vnic_hfi_rem_ctrl_port(struct opa_netdev *ndev)
{
	if (ndev->vnic_ctrl_dev) {
		opa_vnic_hfi_rem_eeph_ports(ndev->vnic_ctrl_dev);
		opa_vnic_ctrl_device_unregister(ndev->vnic_ctrl_dev);
		ndev->vnic_ctrl_dev = NULL;
	}
}

/* Handle link state changes */
static void opa_netdev_link_handling(struct opa_netdev *ndev)
{
	struct opa_core_device *odev = ndev->odev;
	struct opa_core_ops *ops = odev->bus_ops;
	struct opa_dev_desc desc;
	struct opa_pport_desc pdesc;
	int i, active = 0, offline = 0, rc;

	odev->bus_ops->get_device_desc(odev, &desc);
	for (i = 0; i < desc.num_pports; i++) {
		ops->get_port_desc(odev, &pdesc, i + 1);
		if (pdesc.lstate == IB_PORT_ACTIVE)
			active++;
		if (pdesc.lstate == IB_PORT_DOWN)
			offline++;
	}
	mutex_lock(&ndev->ctrl_lock);
	/* Add the VNIC control device if both the ports are active */
	if (active == desc.num_pports) {
		rc = opa_vnic_hfi_add_ctrl_port(ndev);
		if (rc)
			dev_err(&odev->dev, "add_ctrl_dev rc %d\n", rc);
		goto unlock;
	}
	/* Remove the VNIC control device if both ports are offline */
	if (offline == desc.num_pports)
		opa_vnic_hfi_rem_ctrl_port(ndev);
unlock:
	mutex_unlock(&ndev->ctrl_lock);
}

/* Handle event notifications from the OPA core */
static void opa_netdev_event_notify(struct opa_core_device *odev,
				    enum opa_core_event event, u8 port)
{
	struct opa_netdev *ndev = opa_core_get_priv_data(&opa_vnic_clnt, odev);

	if (!ndev)
		return;
	dev_dbg(&odev->dev, "%s port %d event %d\n", __func__, port, event);
	switch (event) {
	case OPA_LINK_STATE_CHANGE:
		opa_netdev_link_handling(ndev);
		break;
	default:
		break;
	};
}

/* per OPA core device probe routine */
static int opa_netdev_probe(struct opa_core_device *odev)
{
	struct opa_netdev *ndev;
	int rc;

	ndev = kzalloc(sizeof(*ndev), GFP_KERNEL);
	if (!ndev)
		return -ENOMEM;

	rc = opa_core_set_priv_data(&opa_vnic_clnt, odev, ndev);
	if (rc)
		goto priv_err;

	ndev->odev = odev;
	mutex_init(&ndev->ctrl_lock);
	mutex_init(&ndev->ctx_lock);
	spin_lock_init(&ndev->tx_lock);
	spin_lock_init(&ndev->rx_lock);
	opa_netdev_link_handling(ndev);
	return rc;
priv_err:
	kfree(ndev);
	dev_err(&odev->dev, "%s rc %d\n", __func__, rc);
	return rc;
}

/* per OPA core device remove routine */
static void opa_netdev_remove(struct opa_core_device *odev)
{
	struct opa_netdev *ndev = opa_core_get_priv_data(&opa_vnic_clnt, odev);

	if (!ndev)
		return;
	opa_vnic_hfi_rem_ctrl_port(ndev);
	opa_core_clear_priv_data(&opa_vnic_clnt, odev);
	kfree(ndev);
}

static int __init opa_netdev_init_module(void)
{
	return opa_core_client_register(&opa_vnic_clnt);
}
module_init(opa_netdev_init_module);

static void __exit opa_netdev_exit_module(void)
{
	opa_core_client_unregister(&opa_vnic_clnt);
}
module_exit(opa_netdev_exit_module);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Intel Corporation");
MODULE_DESCRIPTION("Intel(R) Omni-Path Gen2 Network Driver");
