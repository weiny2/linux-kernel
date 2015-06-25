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
#include <rdma/opa_core.h>
#include <rdma/hfi_tx.h>
#include <rdma/hfi_rx.h>
#include <rdma/hfi_args.h>
#include <rdma/hfi_ct.h>
#include <linux/opa_vnic.h>

#include "opa2_vnic_hfi.h"

static int opa_netdev_probe(struct opa_core_device *odev);
static void opa_netdev_remove(struct opa_core_device *odev);

static struct opa_core_client opa_vnic_clnt = {
	.name = KBUILD_MODNAME,
	.add = opa_netdev_probe,
	.remove = opa_netdev_remove
};

/*
 * struct opa_netdev - OPA2 net device specific fields
 *
 * @ctx - HFI context
 * @cq_idx - command queue index
 * @tx - TX command queue
 * @rx - RX command queue
 */
struct opa_netdev {
	struct hfi_ctx		ctx;
	u16			cq_idx;
	struct hfi_cq		tx;
	struct hfi_cq		rx;
};

#define OPA2_NET_ME_COUNT 256
#define OPA2_NET_UNEX_COUNT 512
#define OPA2_TX_TIMEOUT_MS 1000

static inline int
__hfi_ct_wait(struct hfi_ctx *ctx, hfi_ct_handle_t ct_h,
	      unsigned long threshold, unsigned long timeout_ms,
	      unsigned long *ct_val)
{
	unsigned long val;
	unsigned long exit_jiffies = jiffies + msecs_to_jiffies(timeout_ms);

	while (1) {
		if (hfi_ct_get_failure(ctx, ct_h))
			return -EFAULT;
		val = hfi_ct_get_success(ctx, ct_h);
		if (val >= threshold)
			break;
		if (time_after(jiffies, exit_jiffies))
			return -ETIME;
		schedule();
	}

	if (ct_val)
		*ct_val = val;

	return 0;
}

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

	cmd_slots = hfi_format_put_match(ctx, ni,
					 start, length,
					 target_id, pt_index,
					 user_ptr, 0, hdr_data,
					 ack_req, md_options,
					 eq_handle, ct_handle,
					 remote_offset,
					 tx_handle, &cmd);

	do {
		rc = hfi_tx_cmd_put_match(tx, cmd.command, start,
					  length, cmd_slots);
	} while (rc == -EAGAIN);

	return rc;
}

int opa2_vnic_hfi_put_skb(struct opa_vnic_device *vdev, struct sk_buff *skb)
{
	return 0;
}

struct sk_buff *opa2_vnic_hfi_get_skb(struct opa_vnic_device *vdev)
{
	return NULL;
}

int opa2_vnic_hfi_open(struct opa_vnic_device *vdev, opa_vnic_hfi_evt_cb_fn cb)
{
	vdev->vnic_cb = cb;
	return 0;
}

void opa2_vnic_hfi_close(struct opa_vnic_device *vdev)
{
	vdev->vnic_cb = NULL;
}

int opa2_vnic_hfi_init(struct opa_vnic_device *vdev)
{
	return 0;
}

void opa2_vnic_hfi_deinit(struct opa_vnic_device *vdev) { }

static int opa2_xfer_test(struct opa_core_device *odev, struct opa_netdev *dev)
{
	struct opa_core_ops *ops = odev->bus_ops;
	struct hfi_ctx *ctx = &dev->ctx;
	struct hfi_cq *tx = &dev->tx;
	struct hfi_cq *rx = &dev->rx;
	int n_slots, rc;
	void *tx_base, *rx_base, *rx_base1;
	union ptentry_fp1 *pt_array, *ptentry;
	union ptentry_fp0 *pt0_array, *pt0entry;
	union fp_le_options le_opts;
	hfi_rx_cq_command_t rx_cmd;
	hfi_ni_t ni = PTL_NONMATCHING_PHYSICAL;
	hfi_hdr_data_t hdr_data = 0x1;
	hfi_ack_req_t ack_req = PTL_NO_ACK_REQ;
	hfi_md_options_t md_options = PTL_MD_EVENT_SEND_DISABLE |
					PTL_MD_EVENT_CT_SEND;
	hfi_tx_handle_t tx_handle = 0xc;
	hfi_eq_handle_t eq_handle = 0;
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
	hfi_size_t min_free = 2048;
	hfi_user_ptr_t user_ptr = (hfi_user_ptr_t)&user_ptr;
	hfi_me_handle_t me_handle = 0xc;
	struct opa_pport_desc pdesc;

	ops->get_port_desc(odev, &pdesc, 1);

	target_id.phys.slid = pdesc.lid;
	target_id.phys.ipid = ctx->pid;
	match_id.phys.slid = pdesc.lid;
	match_id.phys.ipid = ctx->pid;
#define PAYLOAD_SIZE 512
#define HFI_TEST_PT 10
	/* Create a test TX buffer and initialize it */
	tx_base = kzalloc(PAYLOAD_SIZE, GFP_KERNEL);
	if (!tx_base)
		return -ENOMEM;
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

	pt_array = (union ptentry_fp1 *)ctx->pt_addr;
	ptentry = &pt_array[HFI_PT_INDEX(PTL_NONMATCHING_PHYSICAL,
					 HFI_TEST_PT)];

	/* Check that the requested PT entry is unused */
	if (ptentry->v) {
		rc = -EBUSY;
		goto err3;
	}

	/* use PTL args->flags */
	if (flags & PTL_OP_PUT)
		le_opts.op_put = 1;
	if (flags & PTL_OP_GET)
		le_opts.op_get = 1;
	if (flags & PTL_IOVEC)
		le_opts.iovec = 1;
	if (flags & PTL_EVENT_CT_COMM) {
		le_opts.event_ct_comm = 1;
		if (flags & PTL_EVENT_CT_BYTES)
			le_opts.event_ct_bytes = 1;
	}
	if (flags & PTL_EVENT_COMM_DISABLE)
		le_opts.event_comm_disable = 1;
	/* TODO - else, test eq_array[eq_handle].v bit? */
	if (flags & PTL_EVENT_SUCCESS_DISABLE)
		le_opts.event_success_disable = 1;
	if (flags & PTL_ACK_DISABLE)
		le_opts.ack_disable = 1;
	if (flags & PTL_NO_ATOMIC)
		le_opts.no_atomic = 1;

	ct_alloc.ni = ni;
	rc = hfi_ct_alloc(ctx, &ct_alloc, &ct_tx);
	if (rc < 0)
		goto err3;
	rc = hfi_ct_alloc(ctx, &ct_alloc, &ct_rx);
	if (rc < 0)
		goto err3;

	memset(ptentry, 0, sizeof(*ptentry));
	/* Populate an RX buffer into a PT entry */
	ptentry->user_id = ctx->ptl_uid;
	ptentry->length = PAYLOAD_SIZE;
	ptentry->ct_handle = ct_rx;
	ptentry->eq_handle = 0;
	ptentry->start = (uint64_t)rx_base;
	ptentry->le_low = le_opts.val & 0x7;
	ptentry->le_mid = (le_opts.val >> 3) & 0x1F;
	ptentry->le_high = (le_opts.val >> 8) & 0x3;
	ptentry->ni = PTL_NONMATCHING_PHYSICAL;
	ptentry->fp = 1;
	ptentry->v = 1;

	pt0_array = (union ptentry_fp0 *)ctx->pt_addr;
	pt0entry = &pt0_array[HFI_PT_INDEX(PTL_NONMATCHING_PHYSICAL,
					   HFI_TEST_PT + 1)];
	/* Check that the requested PT entry is unused */
	if (pt0entry->v) {
		rc = -EBUSY;
		goto err3;
	}

	pt0entry->priority_head = 0;
	pt0entry->priority_tail = 0;
	pt0entry->overflow_head = 0;
	pt0entry->unexpected_tail = 0;
	pt0entry->eq_handle = eq_handle;
	pt0entry->bc = 0;
	pt0entry->ot = 0;
	pt0entry->uo = 0;
	pt0entry->fc = 0;
	pt0entry->ni = ni;
	pt0entry->fp = 0;
	pt0entry->enable = 1;
	pt0entry->v = 1;

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
			  HFI_TEST_PT, 0, 0,
			  hdr_data, ack_req, md_options,
			  eq_handle,
			  ct_tx,
			  remote_offset,
			  tx_handle);
	if (rc < 0)
		goto err3;

	/* Wait for TX command 1 to complete */
	rc = __hfi_ct_wait(ctx, ct_tx, 1, OPA2_TX_TIMEOUT_MS, NULL);
	if (rc < 0) {
		dev_info(&odev->dev, "TX CT event 1 failure, %d\n", rc);
		goto err3;
	} else
		dev_info(&odev->dev, "TX CT event 1 success\n");

	/* TX data to the regular PTE */
	rc = hfi_tx_write(tx, ctx, ni, tx_base,
			  PAYLOAD_SIZE,
			  target_id,
			  HFI_TEST_PT + 1, 0, 0,
			  hdr_data, ack_req, md_options,
			  eq_handle,
			  ct_tx,
			  remote_offset,
			  tx_handle);
	if (rc < 0)
		goto err3;

	/* Wait for TX command 2 to complete */
	rc = __hfi_ct_wait(ctx, ct_tx, 2, OPA2_TX_TIMEOUT_MS, NULL);
	if (rc < 0) {
		dev_info(&odev->dev, "TX CT event 2 failure, %d\n", rc);
		goto err3;
	} else
		dev_info(&odev->dev, "TX CT event 2 success\n");

	/* Wait to receive from peer */
	rc = __hfi_ct_wait(ctx, ct_rx, 1, OPA2_TX_TIMEOUT_MS, NULL);
	if (rc < 0) {
		dev_info(&odev->dev, "RX CT failure, %d\n", rc);
		goto err3;
	} else
		dev_info(&odev->dev, "RX CT success\n");

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

	kfree(rx_base);
	kfree(tx_base);
	return 0;
err3:
	kfree(rx_base1);
err2:
	kfree(rx_base);
err1:
	kfree(tx_base);
	return rc;
}

int opa2_vnic_hfi_setup(struct opa_core_device *odev)
{
	struct opa_netdev *dev = opa_core_get_priv_data(&opa_vnic_clnt, odev);
	struct hfi_ctx *ctx = &dev->ctx;
	struct opa_ctx_assign ctx_assign = {0};
	struct opa_core_ops *ops = odev->bus_ops;
	int rc;

	HFI_CTX_INIT_BYPASS(ctx, odev->dd);
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

	rc = opa2_xfer_test(odev, dev);
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

void opa2_vnic_hfi_cleanup(struct opa_core_device *odev)
{
	struct opa_netdev *dev = opa_core_get_priv_data(&opa_vnic_clnt, odev);
	struct hfi_ctx *ctx = &dev->ctx;
	struct opa_core_ops *ops = odev->bus_ops;

	ops->cq_unmap(&dev->tx, &dev->rx);
	ops->cq_release(ctx, dev->cq_idx);
	odev->bus_ops->ctx_release(ctx);
}

static int opa_netdev_probe(struct opa_core_device *odev)
{
	struct opa_netdev *dev;
	int rc;

	dev = kzalloc(sizeof(struct opa_netdev), GFP_KERNEL);
	rc = opa_core_set_priv_data(&opa_vnic_clnt, odev, dev);
	if (rc)
		goto priv_err;

	rc = opa2_vnic_hfi_setup(odev);
	if (rc)
		goto vnic_err;

	rc = opa2_vnic_hfi_add_vports(odev);
	if (rc)
		goto vport_err;

	return 0;

vport_err:
	opa2_vnic_hfi_cleanup(odev);
vnic_err:
	opa_core_clear_priv_data(&opa_vnic_clnt, odev);
priv_err:
	dev_err(&odev->dev, "error initializing vnic client %d\n", rc);
	kfree(dev);
	return rc;
}

static void opa_netdev_remove(struct opa_core_device *odev)
{
	struct opa_netdev *dev = opa_core_get_priv_data(&opa_vnic_clnt, odev);

	opa2_vnic_hfi_remove_vports(odev);
	opa2_vnic_hfi_cleanup(odev);
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
