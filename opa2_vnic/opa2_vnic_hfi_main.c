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
 *
 * Intel(R) Omni-Path Gen2 specific support for VNIC functionality.
 */
#include <linux/etherdevice.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/utsname.h>
#include <linux/kthread.h>
#include <rdma/opa_core.h>
#include <rdma/hfi_tx.h>
#include <rdma/hfi_rx.h>
#include <rdma/hfi_args.h>
#include <rdma/hfi_ct.h>
#include <rdma/hfi_eq.h>
#include <rdma/hfi_pt.h>
#include <rdma/hfi_me.h>
#include <linux/opa_vnic.h>

/* TODO: Remove this module param later, EM will provide this info */
uint num_veswports = 2;
module_param(num_veswports, uint, S_IRUGO);
MODULE_PARM_DESC(num_veswports, "number of vESWPorts");

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

static void opa_netdev_event_notify(struct opa_core_device *odev,
				    enum opa_core_event event, u8 port)
{
	/* FXRTODO: Add event handling */
	dev_info(&odev->dev, "%s port %d event %d\n", __func__, port, event);
}

#define OPA2_NET_ME_COUNT 256
#define OPA2_NET_UNEX_COUNT 512
#define OPA2_NET_TIMEOUT_MS 100
#define OPA2_NET_RX_POLL_MS 1
#define OPA2_NET_PT 20
#define OPA2_NET_NUM_RX_BUFS 128
/* FXRTODO: Obtain MTU from vdev once supported */
#define OPA2_NET_DEFAULT_MTU 2048

/*
 * struct opa_veswport - OPA2 virtual ethernet switch port specific fields
 *
 * @ctx: HFI context
 * @cq_idx: command queue index
 * @tx: TX command queue
 * @rx: RX command queue
 * @skb: Array of socket buffers
 * @me_handle: Array of ME handles
 * @ct_tx: Counting TX event
 * @eq_tx: TX event handle
 * @eq_rx: RX event handle
 * @ni: portals network interface
 * @eq_alloc_tx: Event queue alloc args for TX
 * @eq_alloc_rx: Event queue alloc args for RX
 * @num_tx: Number of packets transmitted
 * @dlid: destination LID
 * @rx_thread: kernel thread for receive work
 * @vdev: back pointer to opa vnic device
 * @npdev: back pointer to opa net device port
 * @odev: back pointer to opa core device
 * @vport_num: virtual ethernet switch port id
 * @init: true if veswport is initialized
 */
struct opa_veswport {
	struct hfi_ctx		ctx;
	u16			cq_idx;
	struct hfi_cq		tx;
	struct hfi_cq		rx;
	struct sk_buff		*skb[OPA2_NET_NUM_RX_BUFS];
	hfi_me_handle_t		me_handle[OPA2_NET_ME_COUNT];
	hfi_ct_handle_t		ct_tx;
	hfi_eq_handle_t		eq_tx;
	hfi_eq_handle_t		eq_rx;
	hfi_ni_t		ni;
	struct opa_ev_assign	eq_alloc_tx;
	struct opa_ev_assign	eq_alloc_rx;
	int			num_tx;
	int			dlid;
	struct task_struct	*rx_thread;
	struct opa_vnic_device	*vdev;
	struct opa_netdev_port	*npdev;
	struct opa_core_device	*odev;
	u8			vport_num;
	bool			init;
};

/*
 * struct opa_netdev_port - Per physical port net device fields
 *
 * @odev: OPA core device
 * @ndev: OPA2 net device
 * @vport: Array of virtual ethernet switch ports
 * @num_vport: Number of virtual ethernet switch ports
 * @port_num: Port id number
 * @init: true if port is initialized
 */
struct opa_netdev_port {
	struct opa_core_device	*odev;
	struct opa_netdev	*ndev;
	struct opa_veswport	*vport;
	int			num_vports;
	u8			port_num;
	bool			init;
};

/*
 * struct opa_netdev - OPA2 net device specific fields
 * @odev: OPA core device
 * @pport: Array of physical port specific fields
 * @num_pport: number of physical ports
 */
struct opa_netdev {
	struct opa_core_device	*odev;
	struct opa_netdev_port	*pport;
	int			num_ports;
};

static
int hfi_tx_write(struct hfi_cq *tx, struct hfi_ctx *ctx,
		 hfi_ni_t ni, void *start, hfi_size_t length,
		 hfi_process_t target_id, uint32_t pt_index,
		 hfi_user_ptr_t user_ptr,
		 hfi_match_bits_t match_bits, hfi_hdr_data_t hdr_data,
		 hfi_ack_req_t ack_req, hfi_md_options_t md_options,
		 hfi_eq_handle_t eq_handle, hfi_ct_handle_t ct_handle,
		 hfi_size_t remote_offset, hfi_tx_handle_t tx_handle)
{
	union hfi_tx_cq_command cmd;
	int cmd_slots, rc;

        if (hdr_data || HFI_MATCHING_NI(ni))
                /* Must use the longer TX command format */
                cmd_slots = hfi_format_put_match(ctx, ni,
                                                 start, length,
                                                 target_id, pt_index,
                                                 user_ptr, match_bits, hdr_data,
                                                 ack_req, md_options,
                                                 eq_handle, ct_handle,
                                                 remote_offset,
                                                 tx_handle, &cmd);
        else
                /* Can use the shorter TX command format */
                cmd_slots = hfi_format_put(ctx, ni,
                                           start, length,
                                           target_id, pt_index,
                                           user_ptr,
                                           ack_req, md_options,
                                           eq_handle, ct_handle,
                                           remote_offset,
                                           tx_handle, &cmd);

	do {
                if (length <= HFI_TX_MAX_BUFFERED) {
                        rc = hfi_tx_cmd_put_buff_match(tx, cmd.command,
                                        cmd_slots);
                } else if (length <= HFI_TX_MAX_PIO) {
                        if (hdr_data || HFI_MATCHING_NI(ni)) {
                                rc = hfi_tx_cmd_put_pio_match(tx, cmd.command,
                                                start, length, cmd_slots);
                        } else {
                                rc = hfi_tx_cmd_put_pio(tx, cmd.command,
                                                start, length, cmd_slots);
                        }
                } else {
                        rc = hfi_tx_cmd_put_dma_match(tx, cmd.command,
                                        cmd_slots);
                }
	} while (rc == -EAGAIN);

	return rc;
}

static int _hfi_eq_alloc(struct opa_core_device *odev,
			 struct hfi_ctx *ctx,
			 struct opa_ev_assign *eq_alloc,
			 hfi_eq_handle_t *eq)
{
	struct opa_core_ops *ops = odev->bus_ops;
	u32 *eq_head_array, *eq_head_addr;
	int rc;

	eq_alloc->base = (u64)kzalloc(eq_alloc->size * HFI_EQ_ENTRY_SIZE,
				      GFP_KERNEL);
	if (!eq_alloc->base)
		return -ENOMEM;
	eq_alloc->mode = OPA_EV_MODE_BLOCKING;
	rc = ops->ev_assign(ctx, eq_alloc);
	if (rc < 0)
		goto err;
	eq_head_array = ctx->eq_head_addr;
	/* Reset the EQ SW head */
	eq_head_addr = &eq_head_array[eq_alloc->ev_idx];
	*eq_head_addr = 0;
	*eq = eq_alloc->ev_idx;
err:
	return rc;
}

static void _hfi_eq_release(struct opa_ev_assign *eq_alloc)
{
	kfree((void *)eq_alloc->base);
}

/* Append a single socket buffer */
static int opa2_vnic_append_skb(struct opa_veswport *dev, int idx)
{
	struct hfi_ctx *ctx = &dev->ctx;
	struct hfi_cq *rx = &dev->rx;
	hfi_match_bits_t match_bits = 0x0;
	hfi_match_bits_t ignore_bits = 0;
	hfi_me_options_t me_options = (PTL_OP_PUT | PTL_USE_ONCE |
				       PTL_EVENT_LINK_DISABLE |
				       PTL_EVENT_UNLINK_DISABLE |
				       PTL_ME_NO_TRUNCATE |
				       PTL_EVENT_CT_COMM);
	hfi_size_t min_free = 0;
	hfi_rx_cq_command_t rx_cmd;
	hfi_process_t match_id;
	int n_slots;

	match_id.phys.slid = dev->dlid;
	match_id.phys.ipid = ctx->pid;
	n_slots = hfi_format_rx_append(ctx, dev->ni,
				       dev->skb[idx]->data,
				       OPA2_NET_DEFAULT_MTU,
				       OPA2_NET_PT,
				       match_id, 0x0,
				       match_bits, ignore_bits,
				       me_options, HFI_CT_NONE,
				       PTL_PRIORITY_LIST, min_free,
				       idx, dev->me_handle[idx], &rx_cmd);

	return hfi_rx_command(rx, (uint64_t *)&rx_cmd, n_slots);
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
	struct hfi_ctx *ctx = &dev->ctx;
	struct hfi_cq *tx = &dev->tx;
	int rc;
	hfi_process_t target_id;
	hfi_hdr_data_t hdr_data = 0x1;
	hfi_ack_req_t ack_req = PTL_NO_ACK_REQ;
	/*
	 * TX send events are disabled but we still allocate an EQ TX
	 * in case it is required for figuring out failure reasons
	 * for debug purposes
	 */
	hfi_md_options_t md_options = PTL_MD_EVENT_SEND_DISABLE |
					PTL_MD_EVENT_CT_SEND;
	hfi_tx_handle_t tx_handle = 0xc;
	hfi_size_t remote_offset = 0;

	/* FXRTODO: DLID, PTE and PID are hard coded for now */
	target_id.phys.slid = dev->dlid;
	target_id.phys.ipid = ctx->pid;

	/*
	 * FXRTODO: Transmit EoSTL packets via Portals 16B instead
	 * of using the STLEEP command formats for now
	 */
	rc = hfi_tx_write(tx, ctx, dev->ni, skb->data,
			  skb->len,
			  target_id,
			  OPA2_NET_PT, 0xdead, 0,
			  hdr_data, ack_req, md_options,
			  dev->eq_tx,
			  dev->ct_tx,
			  remote_offset,
			  tx_handle);
	if (rc < 0)
		goto err1;

	/* Wait for TX command 1 to complete */
	rc = hfi_ct_wait_timed(ctx, dev->ct_tx, dev->num_tx + 1,
			       OPA2_NET_TIMEOUT_MS, NULL);
	if (rc < 0)
		dev_err(&odev->dev, "TX CT event 1 failure, %d\n", rc);
	else
		dev->num_tx++;
	dev_dbg(&odev->dev, "%s %d vport %d port %d num_tx %d skb->len %d\n",
		__func__, __LINE__, dev->vport_num,
		dev->npdev->port_num, dev->num_tx, skb->len);
err1:
	return rc;
}

/*
 * Receive a packet notification by querying the RX event queue and
 * update the length of the packet received in the SKB. A new SKB is
 * allocated and appended to the free list.
 */
static struct sk_buff *opa2_vnic_hfi_get_skb(struct opa_vnic_device *vdev)
{
	struct opa_veswport *dev = vdev->hfi_priv;
	struct opa_core_device *odev = dev->odev;
	struct hfi_ctx *ctx = &dev->ctx;
	struct hfi_cq *rx = &dev->rx;
	u64 *eq_entry;
	int rc, idx;
	struct sk_buff *skb;
	union target_EQEntry *rx_event;
	struct {
		int64_t start:TARGET_EQENTRY_START_WIDTH;
	} eqd_tmp;
	s64 start_va;

	rc = hfi_eq_peek(ctx, dev->eq_rx, (void **)&eq_entry);
	if (!eq_entry)
		return NULL;
	rx_event = (union target_EQEntry *)eq_entry;
	eqd_tmp.start = rx_event->start;
	start_va = eqd_tmp.start;

	idx = rx_event->user_ptr;
	skb = dev->skb[idx];

	/* FXRTODO: use rlength or mlenth? */
	if (skb)
		skb_put(skb, rx_event->mlength);

	hfi_eq_advance(ctx, rx, dev->eq_rx, eq_entry);

	dev->skb[idx] = netdev_alloc_skb(vdev->netdev, OPA2_NET_DEFAULT_MTU);
	if (dev->skb[idx])
		/* FXRTODO: What if the append fails? */
		opa2_vnic_append_skb(dev, idx);
	else
		dev_err(&odev->dev, "Couldn't allocate skb\n");
	return skb;
}

/* Block for an RX interrupt */
static int opa2_vnic_rx_wait(struct opa_veswport *dev, u64 **rhf_entry)
{
	int rc;
	struct hfi_ctx *ctx = &dev->ctx;

	rc = hfi_eq_wait_irq(ctx, dev->eq_rx, -1, (void **)rhf_entry);
	if (rc == -EAGAIN || rc == -ERESTARTSYS)
		/* timeout or wait interrupted, not abnormal */
		rc = 0;
	else if (rc == HFI_EQ_DROPPED)
		/* driver bug with EQ sizing or IRQ logic */
		rc = -EIO;
	return rc;
}

/* Kernel thread to notify the netdev layer about received packets */
static int opa2_vnic_rx_work(void *data)
{
	struct opa_veswport *dev = data;
	struct opa_vnic_device *vdev = dev->vdev;
	struct opa_core_device *odev = dev->odev;
	u64 *rhf_entry;
	int rc;

	allow_signal(SIGINT);

	/*
	 * FXRTODO: Ideally we need a callback from the ISR instead of
	 * a kthread for RX events.
	 */
	while (!kthread_should_stop()) {
		rhf_entry = NULL;
		rc = opa2_vnic_rx_wait(dev, &rhf_entry);
		if (rc < 0) {
			dev_warn(&odev->dev, "RX EQ failure, %d\n", rc);
			/* TODO - handle this */
			continue;
		}

		if (rhf_entry)
			vdev->vnic_cb(vdev->netdev, OPA_VNIC_HFI_EVT_RX);
	}
	return 0;
}

/*
 * Free any RX buffers previously allocated.
 * FXRTODO: Unlink if appended to the LE list
 */
static void opa2_free_skbs(struct opa_veswport *dev)
{
	int i = 0;

	for (i = 0; i < OPA2_NET_NUM_RX_BUFS; i++) {
		if (dev->skb[i]) {
			dev_kfree_skb_any(dev->skb[i]);
			dev->skb[i] = NULL;
		}
	}
}

/* Allocate and append RX buffers */
static int opa2_alloc_skbs(struct opa_veswport *dev)
{
	int i, rc = 0;
	struct hfi_ctx *ctx = &dev->ctx;
	struct opa_vnic_device *vdev = dev->vdev;

	/* Create RX buffers and append them */
	for (i = 0; i < OPA2_NET_NUM_RX_BUFS; i++) {
		dev->skb[i] = netdev_alloc_skb(vdev->netdev,
					       OPA2_NET_DEFAULT_MTU);
		if (!dev->skb[i]) {
			rc = -ENOMEM;
			goto err1;
		}

		rc = hfi_me_alloc(ctx, &dev->me_handle[i], 0);
		if (rc)
			goto err1;

		rc = opa2_vnic_append_skb(dev, i);
		if (rc)
			goto err1;
	}
	return rc;
err1:
	opa2_free_skbs(dev);
	return rc;
}


/* Initialize and wake up the receive thread */
static int opa2_rx_thread_init(struct opa_veswport *dev)
{
	struct opa_vnic_device *vdev = dev->vdev;
	struct opa_core_device *odev = dev->odev;
	int rc = 0;

	dev->rx_thread = kthread_run(opa2_vnic_rx_work, dev,
				     "opa_vnic_rx%d", vdev->id);
	if (IS_ERR(dev->rx_thread)) {
		rc = PTR_ERR(dev->rx_thread);
		dev_err(&odev->dev, "kthread_run failed %d\n", rc);
	}
	return rc;
}

/* Interrupt and stop the receive thread */
static void opa2_rx_thread_uninit(struct opa_veswport *dev)
{
	struct opa_core_device *odev = dev->odev;
	int rc;

	if (!IS_ERR_OR_NULL(dev->rx_thread)) {
		rc = send_sig(SIGINT, dev->rx_thread, 0);
		if (rc) {
			dev_err(&odev->dev, "send_sig failed %d\n", rc);
			return;
		}
		kthread_stop(dev->rx_thread);
		dev->rx_thread = NULL;
	}
}

static int opa2_vnic_hfi_open(struct opa_vnic_device *vdev,
			      opa_vnic_hfi_evt_cb_fn cb)
{
	struct opa_veswport *dev = vdev->hfi_priv;
	int rc;

	rc = opa2_alloc_skbs(dev);
	if (rc)
		goto err;
	rc = opa2_rx_thread_init(dev);
	if (rc)
		goto skb_err;

	vdev->vnic_cb = cb;
err:
	return rc;
skb_err:
	opa2_free_skbs(dev);
	return rc;
}

static void opa2_vnic_hfi_close(struct opa_vnic_device *vdev)
{
	struct opa_veswport *dev = vdev->hfi_priv;

	opa2_rx_thread_uninit(dev);
	opa2_free_skbs(dev);
	vdev->vnic_cb = NULL;
}

/*
 * Various initialization tasks for TX/RX data paths. These include:
 * a) Setting up E2E connections
 * b) Allocating CT events for TX & RX
 * c) Allocating EQs for TX & RX
 * d) Allocating a PTE
 * e) Allocating ME handles
 */
static int opa2_init_tx_rx(struct opa_veswport *dev)
{
	struct opa_core_ops *ops = dev->odev->bus_ops;
	struct opa_core_device *odev = dev->odev;
	struct hfi_ctx *ctx = &dev->ctx;
	struct hfi_cq *rx = &dev->rx;
	int rc;
	u32 pt_idx;
	hfi_ct_alloc_args_t ct_alloc = {0};
	hfi_pt_alloc_args_t pt_alloc = {0};
	struct opa_pport_desc pdesc;
	struct opa_e2e_ctrl e2e;
	u64 *eq_entry;

	ops->get_port_desc(odev, &pdesc, dev->npdev->port_num);

	/*
	 * FXRTODO: DLID is hard coded for now but needs to be determined
	 * programmatically from the MAC address or the globally administered
	 * MAD address table eventually
	 */
	if (!strcmp(utsname()->nodename, "viper0"))
		dev->dlid = dev->npdev->port_num == 1 ? 3 : 4;
	else
		dev->dlid = dev->npdev->port_num == 1 ? 1 : 2;

	/*
	 * FXRTODO: E2E messages do not have to be set up for EoSTL traffic
	 * eventually but it is required for now since we are using Portals
	 * 16B packets instead of 10B/16B STLEEP packets.
	 */
	e2e.slid = pdesc.lid;
	e2e.dlid = dev->dlid;
	e2e.sl = 0;

	rc = ops->e2e_ctrl(ctx, &e2e);
	if (rc)
		return rc;
	e2e.slid = pdesc.lid;
	e2e.dlid = pdesc.lid;
	e2e.sl = 0;

	rc = ops->e2e_ctrl(ctx, &e2e);
	if (rc)
		return rc;

	/* FXRTODOD: NI is hard coded for now */
	dev->ni = PTL_NONMATCHING_PHYSICAL;
	ct_alloc.ni = dev->ni;
	rc = hfi_ct_alloc(ctx, &ct_alloc, &dev->ct_tx);
	if (rc < 0)
		goto err1;
	ct_alloc.ni = dev->ni;
	dev->eq_alloc_tx.ni = dev->ni;
	dev->eq_alloc_tx.user_data = 0xdeadbeef;
	dev->eq_alloc_tx.size = 64;
	rc = _hfi_eq_alloc(odev, ctx, &dev->eq_alloc_tx, &dev->eq_tx);
	if (rc)
		goto err2;
	/* Check on EQ 0 NI 0 for a PTL_CMD_COMPLETE event */
	rc = hfi_eq_wait_timed(ctx, 0x0, OPA2_NET_TIMEOUT_MS,
			       (void **)&eq_entry);
	if (eq_entry) {
		union initiator_EQEntry *tx_event =
			(union initiator_EQEntry *)eq_entry;

		dev_info(&odev->dev, "TX EQ success kind %d ptr 0x%llx\n",
			 tx_event->event_kind, tx_event->user_ptr);
		hfi_eq_advance(ctx, rx, 0x0, eq_entry);
	} else {
		dev_info(&odev->dev, "TX EQ failure rc %d\n", rc);
	}
	dev->eq_alloc_rx.ni = dev->ni;
	dev->eq_alloc_rx.user_data = 0xdeadbeef;
	dev->eq_alloc_rx.size = 64;
	rc = _hfi_eq_alloc(odev, ctx, &dev->eq_alloc_rx, &dev->eq_rx);
	if (rc)
		goto err3;
	/* Check on EQ 0 NI 0 for a PTL_CMD_COMPLETE event */
	rc = hfi_eq_wait_timed(ctx, 0x0, OPA2_NET_TIMEOUT_MS,
			       (void **)&eq_entry);
	if (eq_entry) {
		union initiator_EQEntry *tx_event =
			(union initiator_EQEntry *)eq_entry;

		dev_info(&odev->dev, "RX EQ success kind %d ptr 0x%llx\n",
			 tx_event->event_kind, tx_event->user_ptr);
		hfi_eq_advance(ctx, rx, 0x0, eq_entry);
	} else {
		dev_info(&odev->dev, "RX EQ failure rc %d\n", rc);
	}
	pt_alloc.ni = dev->ni;
	pt_alloc.pt_idx = OPA2_NET_PT;
	pt_alloc.eq_handle = dev->eq_rx;
	rc = hfi_pt_alloc(ctx, rx, &pt_alloc, &pt_idx);
	if (rc < 0)
		goto err4;

	dev_dbg(&odev->dev, "%s success\n", __func__);
	return 0;
err4:
	_hfi_eq_release(&dev->eq_alloc_rx);
err3:
	_hfi_eq_release(&dev->eq_alloc_tx);
err2:
	hfi_ct_free(ctx, dev->ct_tx, 0);
err1:
	dev_err(&odev->dev, "%s err %d\n", __func__, rc);
	return rc;
}

static void opa2_uninit_tx_rx(struct opa_veswport *dev)
{
	struct hfi_ctx *ctx = &dev->ctx;

	hfi_pt_free(ctx, &dev->rx, dev->ni, OPA2_NET_PT, 0);
	_hfi_eq_release(&dev->eq_alloc_tx);
	_hfi_eq_release(&dev->eq_alloc_rx);
	hfi_ct_free(ctx, dev->ct_tx, 0);
}

static int opa2_xfer_test(struct opa_veswport *dev)
{
	struct opa_core_ops *ops = dev->odev->bus_ops;
	struct opa_core_device *odev = dev->odev;
	struct hfi_ctx *ctx = &dev->ctx;
	struct hfi_cq *tx = &dev->tx;
	struct hfi_cq *rx = &dev->rx;
	int n_slots, rc;
	void *tx_base, *rx_base, *rx_base1;
	uint32_t pt_idx, pt_idx1;
	hfi_rx_cq_command_t rx_cmd;
	hfi_ni_t ni = PTL_NONMATCHING_PHYSICAL;
	hfi_hdr_data_t hdr_data = 0x1;
	hfi_ack_req_t ack_req = PTL_NO_ACK_REQ;
	hfi_md_options_t md_options = PTL_MD_EVENT_CT_SEND;
	hfi_tx_handle_t tx_handle = 0xc;
	hfi_eq_handle_t eq_tx = 0, eq_rx = 0;
	hfi_size_t remote_offset = 0;
	hfi_process_t target_id, match_id;
	uint64_t flags = (PTL_OP_PUT | PTL_OP_GET | PTL_EVENT_CT_COMM);
	hfi_match_bits_t match_bits = 0x0;
	hfi_match_bits_t ignore_bits = 0;
	hfi_me_options_t me_options = (PTL_OP_PUT | PTL_USE_ONCE |
					PTL_EVENT_LINK_DISABLE |
					PTL_EVENT_UNLINK_DISABLE |
					PTL_ME_NO_TRUNCATE | PTL_EVENT_CT_COMM);
	hfi_ct_handle_t ct_rx, ct_tx;
	hfi_ct_alloc_args_t ct_alloc = {0};
	hfi_pt_alloc_args_t pt_alloc = {0};
	hfi_pt_fast_alloc_args_t pt_falloc = {0};
	struct opa_ev_assign eq_alloc_tx = {0}, eq_alloc_rx = {0};
	hfi_size_t min_free = 2048;
	hfi_user_ptr_t user_ptr = 0xc0de;
	hfi_me_handle_t me_handle = 0xc;
	struct opa_pport_desc pdesc;
	struct opa_e2e_ctrl e2e;
	u64 *eq_entry;

	ops->get_port_desc(odev, &pdesc, 1);

	e2e.slid = pdesc.lid;
	e2e.dlid = pdesc.lid;
	e2e.sl = 0;

	rc = ops->e2e_ctrl(ctx, &e2e);
	if (rc)
		return rc;

	target_id.phys.slid = pdesc.lid;
	target_id.phys.ipid = ctx->pid;
	match_id.phys.slid = pdesc.lid;
	match_id.phys.ipid = ctx->pid;
#define PAYLOAD_SIZE 512
#define HFI_TEST_PT 10
	/* Create a test TX buffer and initialize it */
	tx_base = kzalloc(PAYLOAD_SIZE, GFP_KERNEL);
	if (!tx_base) {
		rc = -ENOMEM;
		goto err0;
	}
	memset(tx_base, 0xc0, PAYLOAD_SIZE);

	/* Create a test RX buffer */
	rx_base = kzalloc(PAYLOAD_SIZE, GFP_KERNEL);
	if (!rx_base) {
		rc = -ENOMEM;
		goto err1;
	}
	/* Create a test RX buffer */
	rx_base1 = kzalloc(PAYLOAD_SIZE, GFP_KERNEL);
	if (!rx_base1) {
		rc = -ENOMEM;
		goto err2;
	}

	ct_alloc.ni = ni;
	rc = hfi_ct_alloc(ctx, &ct_alloc, &ct_tx);
	if (rc < 0)
		goto err3;
	rc = hfi_ct_alloc(ctx, &ct_alloc, &ct_rx);
	if (rc < 0)
		goto err3;

	dev->eq_alloc_tx.ni = ni;
	dev->eq_alloc_tx.user_data = 0xdeadbeef;
	dev->eq_alloc_tx.size = 64;
	rc = _hfi_eq_alloc(odev, ctx, &dev->eq_alloc_tx, &eq_tx);
	if (rc)
		goto err4;
	/* Check on EQ 0 NI 0 for a PTL_CMD_COMPLETE event */
	rc = hfi_eq_wait_timed(ctx, 0x0, OPA2_NET_TIMEOUT_MS,
			       (void **)&eq_entry);
	if (eq_entry) {
		union initiator_EQEntry *tx_event =
			(union initiator_EQEntry *)eq_entry;

		dev_info(&odev->dev, "TX EQ success kind %d ptr 0x%llx\n",
			 tx_event->event_kind, tx_event->user_ptr);
		hfi_eq_advance(ctx, rx, 0x0, eq_entry);
	} else {
		dev_info(&odev->dev, "TX EQ failure rc %d\n", rc);
	}
	dev->eq_alloc_rx.ni = ni;
	dev->eq_alloc_rx.user_data = 0xdeadbeef;
	dev->eq_alloc_rx.size = 64;
	rc = _hfi_eq_alloc(odev, ctx, &dev->eq_alloc_rx, &eq_rx);
	if (rc)
		goto err4;
	/* Check on EQ 0 NI 0 for a PTL_CMD_COMPLETE event */
	rc = hfi_eq_wait_timed(ctx, 0x0, OPA2_NET_TIMEOUT_MS,
			       (void **)&eq_entry);
	if (eq_entry) {
		union initiator_EQEntry *tx_event =
			(union initiator_EQEntry *)eq_entry;

		dev_info(&odev->dev, "RX EQ success kind %d ptr 0x%llx\n",
			 tx_event->event_kind, tx_event->user_ptr);
		hfi_eq_advance(ctx, rx, 0x0, eq_entry);
	} else {
		dev_info(&odev->dev, "RX EQ failure rc %d\n", rc);
	}

	pt_falloc.ni = PTL_NONMATCHING_PHYSICAL;
	pt_falloc.pt_idx = HFI_TEST_PT;
	pt_falloc.flags = flags;
	pt_falloc.start = (uint64_t)rx_base;
	pt_falloc.length = PAYLOAD_SIZE;
	pt_falloc.ct_handle = ct_rx;
	pt_falloc.eq_handle = eq_rx;
	/* Populate an RX buffer into a PT entry */
	rc = hfi_pt_fast_alloc(ctx, rx, &pt_falloc, &pt_idx);
	if (rc < 0)
		goto err5;

	pt_alloc.ni = PTL_NONMATCHING_PHYSICAL;
	pt_alloc.pt_idx = HFI_TEST_PT + 1;
	pt_alloc.eq_handle = eq_rx;
	rc = hfi_pt_alloc(ctx, rx, &pt_alloc, &pt_idx1);
	if (rc < 0)
		goto err5;

	n_slots = hfi_format_rx_append(ctx, ni,
				       rx_base1,
				       PAYLOAD_SIZE,
				       HFI_TEST_PT + 1,
				       match_id,
				       0x0,
				       match_bits, ignore_bits,
				       me_options, 0,
				       PTL_PRIORITY_LIST, min_free,
				       user_ptr,
				       me_handle, &rx_cmd);

	/* Single slot command */
	rc = hfi_rx_command(rx, (uint64_t *)&rx_cmd, n_slots);

	/* TX data to the fast PTE */
	rc = hfi_tx_write(tx, ctx, ni, tx_base,
			  PAYLOAD_SIZE,
			  target_id,
			  HFI_TEST_PT, 0xdead, 0,
			  hdr_data, ack_req, md_options,
			  eq_tx,
			  ct_tx,
			  remote_offset,
			  tx_handle);
	if (rc < 0)
		goto err5;

	/* Wait for TX command 1 to complete */
	rc = hfi_ct_wait_timed(ctx, ct_tx, 1, OPA2_NET_TIMEOUT_MS, NULL);
	if (rc < 0) {
		dev_info(&odev->dev, "TX CT event 1 failure, %d\n", rc);
		goto err5;
	} else {
		dev_info(&odev->dev, "TX CT event 1 success\n");
	}

	rc = hfi_eq_wait_timed(ctx, eq_tx, OPA2_NET_TIMEOUT_MS,
			       (void **)&eq_entry);
	if (eq_entry) {
		union initiator_EQEntry *tx_event =
			(union initiator_EQEntry *)eq_entry;

		dev_info(&odev->dev, "TX EQ 1 success kind %d ptr 0x%llx\n",
			 tx_event->event_kind, tx_event->user_ptr);
		hfi_eq_advance(ctx, rx, eq_tx, eq_entry);
	} else {
		dev_info(&odev->dev, "TX EQ 1 failure rc %d\n", rc);
	}

	/* TX data to the regular PTE */
	rc = hfi_tx_write(tx, ctx, ni, tx_base,
			  PAYLOAD_SIZE,
			  target_id,
			  HFI_TEST_PT + 1, 0xbeef, 0,
			  hdr_data, ack_req, md_options,
			  eq_tx,
			  ct_tx,
			  remote_offset,
			  tx_handle);
	if (rc < 0)
		goto err5;

	/* Block for an EQ interrupt */
	rc = hfi_eq_wait_irq(ctx, eq_tx, OPA2_NET_TIMEOUT_MS,
			     (void **)&eq_entry);
	if (rc < 0) {
		dev_info(&odev->dev, "TX EQ 2 intr fail rc %d\n", rc);
		goto err5;
	} else {
		dev_info(&odev->dev, "TX EQ 2 intr success\n");
	}
	if (eq_entry) {
		union initiator_EQEntry *tx_event =
			(union initiator_EQEntry *)eq_entry;

		dev_info(&odev->dev, "TX EQ 2 success kind %d ptr 0x%llx\n",
			 tx_event->event_kind, tx_event->user_ptr);
		hfi_eq_advance(ctx, rx, eq_tx, eq_entry);
	} else {
		dev_info(&odev->dev, "TX EQ 2 failure rc %d\n", rc);
	}

	/* Wait to receive from peer */
	rc = hfi_ct_wait_timed(ctx, ct_rx, 1, OPA2_NET_TIMEOUT_MS, NULL);
	if (rc < 0) {
		dev_info(&odev->dev, "RX CT failure, %d\n", rc);
		goto err5;
	} else {
		dev_info(&odev->dev, "RX CT success\n");
	}
	rc = hfi_eq_wait_timed(ctx, eq_rx, OPA2_NET_TIMEOUT_MS,
			       (void **)&eq_entry);
	if (eq_entry) {
		union target_EQEntry *rx_event =
			(union target_EQEntry *)eq_entry;

		dev_info(&odev->dev, "RX EQ 1 success kind %d ptr 0x%llx\n",
			 rx_event->event_kind, rx_event->user_ptr);
		hfi_eq_advance(ctx, rx, eq_rx, eq_entry);
	} else {
		dev_info(&odev->dev, "RX EQ 1 failure, %d\n", rc);
	}
	rc = hfi_eq_wait_timed(ctx, eq_rx, OPA2_NET_TIMEOUT_MS,
			       (void **)&eq_entry);
	if (eq_entry) {
		union target_EQEntry *rx_event =
			(union target_EQEntry *)eq_entry;

		dev_info(&odev->dev, "RX EQ 2 success kind %d ptr 0x%llx\n",
			 rx_event->event_kind, rx_event->user_ptr);
		hfi_eq_advance(ctx, rx, eq_rx, eq_entry);
	} else {
		dev_info(&odev->dev, "RX EQ 1 failure, %d\n", rc);
	}

	/* verify the data transfer succeeded for PTE */
	if (!memcmp(tx_base, rx_base, PAYLOAD_SIZE))
		dev_info(&odev->dev, "basic PTE data transfer test passed\n");
	else
		dev_info(&odev->dev, "basic PTE data transfer test failed\n");

	/* verify the data transfer succeeded for LE */
	if (!memcmp(tx_base, rx_base1, PAYLOAD_SIZE))
		dev_info(&odev->dev, "basic LE data transfer test passed\n");
	else
		dev_info(&odev->dev, "basic LE data transfer test failed\n");

	_hfi_eq_release(&eq_alloc_rx);
	_hfi_eq_release(&eq_alloc_tx);
	kfree(rx_base);
	kfree(tx_base);
	return 0;
err5:
	_hfi_eq_release(&eq_alloc_rx);
err4:
	_hfi_eq_release(&eq_alloc_tx);
err3:
	kfree(rx_base1);
err2:
	kfree(rx_base);
err1:
	kfree(tx_base);
err0:
	return rc;
}

static int opa2_vnic_hfi_init(struct opa_vnic_device *vdev)
{
	struct opa_veswport *dev = vdev->hfi_priv;
	struct hfi_ctx *ctx = &dev->ctx;
	struct opa_core_device *odev = dev->odev;
	struct opa_core_ops *ops = odev->bus_ops;
	struct opa_ctx_assign ctx_assign = {0};
	int rc;

	dev->vdev = vdev;
	HFI_CTX_INIT(ctx, odev->dd, odev->bus_ops);
	ctx->mode |= HFI_CTX_MODE_BYPASS_10B;
	ctx_assign.pid = HFI_PID_ANY;
	ctx_assign.le_me_count = OPA2_NET_ME_COUNT;
	ctx_assign.unexpected_count = OPA2_NET_UNEX_COUNT;
	rc = ops->ctx_assign(ctx, &ctx_assign);
	if (rc)
		return rc;

	/* Obtain a pair of command queues and setup */
	rc = ops->cq_assign(ctx, NULL, &dev->cq_idx);
	if (rc)
		goto err;
	rc = ops->cq_map(ctx, dev->cq_idx, &dev->tx, &dev->rx);
	if (rc)
		goto err1;

	rc = opa2_xfer_test(dev);
	if (rc)
		goto err2;

	rc = opa2_init_tx_rx(dev);
	if (rc)
		goto err2;
	return 0;
err2:
	ops->cq_unmap(&dev->tx, &dev->rx);
err1:
	ops->cq_release(ctx, dev->cq_idx);
err:
	ops->ctx_release(ctx);
	return rc;
}

static void opa2_vnic_hfi_deinit(struct opa_vnic_device *vdev)
{
	struct opa_veswport *dev = vdev->hfi_priv;
	struct hfi_ctx *ctx = &dev->ctx;
	struct opa_core_device *odev = dev->odev;
	struct opa_core_ops *ops = odev->bus_ops;

	opa2_uninit_tx_rx(dev);
	ops->cq_unmap(&dev->tx, &dev->rx);
	ops->cq_release(ctx, dev->cq_idx);
	odev->bus_ops->ctx_release(ctx);
}

static struct opa_vnic_hfi_ops vnic_ops = {
	.hfi_init = opa2_vnic_hfi_init,
	.hfi_deinit = opa2_vnic_hfi_deinit,
	.hfi_open = opa2_vnic_hfi_open,
	.hfi_close = opa2_vnic_hfi_close,
	.hfi_put_skb = opa2_vnic_hfi_put_skb,
	.hfi_get_skb = opa2_vnic_hfi_get_skb
};

/* Remove virtual ethernet switch port device */
static void opa2_vnic_hfi_remove_vport(struct opa_veswport *dev)
{
	opa_vnic_device_unregister(dev->vdev);
}

/* Add virtual ethernet switch port device */
static int opa2_vnic_hfi_add_vport(struct opa_veswport *dev)
{
	struct opa_core_device *odev = dev->odev;
	int rc = 0;

	dev->vdev = opa_vnic_device_register(dev->vport_num, dev, &vnic_ops);
	if (IS_ERR(dev->vdev)) {
		rc = PTR_ERR(dev->vdev);
		dev_info(&odev->dev, "error adding vport %d\n", rc);
	}
	return rc;
}

/* Per virtual ethernet switch port cleanup */
void opa_netdev_uninit_vport(struct opa_netdev_port *npdev)
{
	int i;

	for (i = 0; i < npdev->num_vports; i++) {
		struct opa_veswport *dev = &npdev->vport[i];

		if (dev->init)
			opa2_vnic_hfi_remove_vport(dev);
	}
	kfree(npdev->vport);
}

/* Per virtual ethernet switch port initialization */
static int opa_netdev_init_vport(struct opa_netdev_port *npdev)
{
	int i, rc = 0;
	struct opa_core_device *odev = npdev->odev;

	npdev->num_vports = num_veswports;
	npdev->vport = kcalloc(npdev->num_vports, sizeof(*npdev->vport),
			       GFP_KERNEL);
	if (!npdev->vport)
		return -ENOMEM;

	for (i = 0; i < npdev->num_vports; i++) {
		struct opa_veswport *dev = &npdev->vport[i];

		dev->vport_num = (odev->index * npdev->num_vports *
					npdev->ndev->num_ports) +
				((npdev->port_num - 1) * npdev->num_vports) + i;
		dev->odev = npdev->odev;
		dev->npdev = npdev;
		rc = opa2_vnic_hfi_add_vport(dev);
		if (rc)
			goto err;
		dev->init = true;
	}
	return rc;
err:
	opa_netdev_uninit_vport(npdev);
	dev_err(&odev->dev, "%s rc %d\n", __func__, rc);
	return rc;
}

/* Per physical port cleanup */
static void opa_netdev_uninit_port(struct opa_netdev *ndev)
{
	int i;

	for (i = 0; i < ndev->num_ports; i++) {
		struct opa_netdev_port *port = &ndev->pport[i];

		if (port->init)
			opa_netdev_uninit_vport(port);
	}
	kfree(ndev->pport);
}

/* Per physical port initialization */
static int opa_netdev_init_port(struct opa_netdev *ndev)
{
	struct opa_core_device *odev = ndev->odev;
	struct opa_dev_desc desc;
	int i, rc = 0;

	odev->bus_ops->get_device_desc(odev, &desc);
#if 0
	/*
	 * FXRTODO: Only one port supported since the TX write
	 * commands do not allow specifying the outgoing port
	 */
	ndev->num_ports = desc.num_pports;
#else
	ndev->num_ports = 1;
#endif
	ndev->pport = kcalloc(ndev->num_ports, sizeof(*ndev->pport),
			      GFP_KERNEL);
	if (!ndev->pport)
		return -ENOMEM;

	for (i = 0; i < ndev->num_ports; i++) {
		struct opa_netdev_port *port = &ndev->pport[i];

		port->port_num = i + 1;
		port->odev = odev;
		port->ndev = ndev;
		rc = opa_netdev_init_vport(port);
		if (rc)
			goto err;
		port->init = true;
	}
	return rc;
err:
	opa_netdev_uninit_port(ndev);
	dev_err(&odev->dev, "%s rc %d\n", __func__, rc);
	return rc;
}

/* per OPA core device probe routine */
static int opa_netdev_probe(struct opa_core_device *odev)
{
	struct opa_netdev *dev;
	int rc;

	dev = kzalloc(sizeof(struct opa_netdev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	rc = opa_core_set_priv_data(&opa_vnic_clnt, odev, dev);
	if (rc)
		goto priv_err;

	dev->odev = odev;
	rc = opa_netdev_init_port(dev);
	if (rc)
		goto port_err;
	return rc;
port_err:
	opa_core_clear_priv_data(&opa_vnic_clnt, odev);
priv_err:
	kfree(dev);
	dev_err(&odev->dev, "%s rc %d\n", __func__, rc);
	return rc;
}

/* per OPA core device remove routine */
static void opa_netdev_remove(struct opa_core_device *odev)
{
	struct opa_netdev *dev = opa_core_get_priv_data(&opa_vnic_clnt, odev);

	if (!dev)
		return;
	opa_netdev_uninit_port(dev);
	opa_core_clear_priv_data(&opa_vnic_clnt, odev);
	kfree(dev);
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
