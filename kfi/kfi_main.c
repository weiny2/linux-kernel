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
 *
 * Intel(R) Omni-Path Gen2 specific support for KFI
 */
#include <linux/etherdevice.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <rdma/opa_core.h>
#include <rdma/hfi_tx.h>
#include <rdma/hfi_rx.h>
#include <rdma/hfi_args.h>
#include <rdma/hfi_ct.h>
#include <rdma/hfi_pt.h>

static int opa_kfi_probe(struct opa_core_device *odev);
static void opa_kfi_remove(struct opa_core_device *odev);
static void opa_kfi_event_notify(struct opa_core_device *odev,
				 enum opa_core_event event, u8 port);

static struct opa_core_client opa_kfi_clnt = {
	.name = KBUILD_MODNAME,
	.add = opa_kfi_probe,
	.remove = opa_kfi_remove,
	.event_notify = opa_kfi_event_notify
};

static void opa_kfi_event_notify(struct opa_core_device *odev,
				    enum opa_core_event event, u8 port)
{
	/* FXRTODO: Add event handling */
	dev_info(&odev->dev, "%s port %d event %d\n", __func__, port, event);
}

/*
 * struct opa_kfi - OPA2 KFI context
 *
 * @ctx - HFI context
 * @cq_idx - command queue index
 * @tx - TX command queue
 * @rx - RX command queue
 */
struct opa_kfi {
	struct hfi_ctx		ctx;
	u16			cq_idx;
	struct hfi_cq		tx;
	struct hfi_cq		rx;
};

#define OPA2_NET_ME_COUNT 256
#define OPA2_NET_UNEX_COUNT 512
#define OPA2_TX_TIMEOUT_MS 100

static
int hfi_tx_write(struct hfi_cq *tx, struct hfi_ctx *ctx,
		 hfi_ni_t ni, void *start, hfi_size_t length,
		 hfi_process_t target_id, hfi_port_t port,
		 uint32_t pt_index, hfi_rc_t rc, hfi_service_level_t sl,
		 hfi_becn_t becn, hfi_pkey_t pkey,
		 hfi_slid_low_t slid_low, hfi_auth_idx_t auth_idx,
		 hfi_user_ptr_t user_ptr,
		 hfi_match_bits_t match_bits, hfi_hdr_data_t hdr_data,
		 hfi_ack_req_t ack_req, hfi_md_options_t md_options,
		 hfi_eq_handle_t eq_handle, hfi_ct_handle_t ct_handle,
		 hfi_size_t remote_offset, hfi_tx_handle_t tx_handle)
{
	union hfi_tx_cq_command cmd;
	int cmd_slots, ret;

	if (hdr_data || HFI_MATCHING_NI(ni))
		/* Must use the longer TX command format */
		cmd_slots = hfi_format_put_match(ctx, ni,
						 start, length,
						 target_id, port,
						 pt_index, rc, sl,
						 becn, pkey,
						 slid_low, auth_idx,
						 user_ptr, match_bits, hdr_data,
						 ack_req, md_options,
						 eq_handle, ct_handle,
						 remote_offset,
						 tx_handle, &cmd);
	else
		/* Can use the shorter TX command format */
		cmd_slots = hfi_format_put(ctx, ni,
					   start, length,
					   target_id, port,
					   pt_index, rc, sl,
					   becn, pkey,
					   slid_low, auth_idx,
					   user_ptr,
					   ack_req, md_options,
					   eq_handle, ct_handle,
					   remote_offset,
					   tx_handle, &cmd);

	do {
		if (length <= HFI_TX_MAX_BUFFERED) {
			ret = hfi_tx_cmd_put_buff_match(tx, cmd.command,
						       cmd_slots);
		} else if (length <= HFI_TX_MAX_PIO) {
			if (hdr_data || HFI_MATCHING_NI(ni)) {
				ret = hfi_tx_cmd_put_pio_match(tx, cmd.command,
							       start, length,
							       cmd_slots);
			} else {
				ret = hfi_tx_cmd_put_pio(tx, cmd.command,
							 start, length,
							 cmd_slots);
			}
		} else {
			ret = hfi_tx_cmd_put_dma_match(tx, cmd.command,
						       cmd_slots);
		}
	} while (ret == -EAGAIN);

	return ret;
}

static int opa_xfer_test(struct opa_core_device *odev, struct opa_kfi *dev)
{
	struct opa_core_ops *ops = odev->bus_ops;
	struct hfi_ctx *ctx = &dev->ctx;
	struct hfi_cq *tx = &dev->tx;
	struct hfi_cq *rx = &dev->rx;
	int rc;
	hfi_ni_t ni = PTL_NONMATCHING_PHYSICAL;
	hfi_hdr_data_t hdr_data = 0x1;
	hfi_ack_req_t ack_req = PTL_NO_ACK_REQ;
	hfi_md_options_t md_options = PTL_MD_EVENT_CT_SEND;
	hfi_tx_handle_t tx_handle = 0xc;
	hfi_size_t remote_offset = 0;
	hfi_process_t target_id, match_id;
	uint64_t flags = (PTL_OP_PUT | PTL_OP_GET | PTL_EVENT_CT_COMM);
	hfi_ct_handle_t ct_rx, ct_tx;
	hfi_ct_alloc_args_t ct_alloc = {0};
	hfi_pt_fast_alloc_args_t pt_falloc = {0};
	struct opa_pport_desc pdesc;
	struct opa_e2e_ctrl e2e;
	void *tx_base, *rx_base;
	uint32_t pt_idx;

	ops->get_port_desc(odev, &pdesc, 1);

	e2e.slid = pdesc.lid;
	e2e.dlid = pdesc.lid;
	e2e.port_num = 1;
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

	ct_alloc.ni = ni;
	rc = hfi_ct_alloc(ctx, &ct_alloc, &ct_tx);
	if (rc < 0)
		goto err2;
	rc = hfi_ct_alloc(ctx, &ct_alloc, &ct_rx);
	if (rc < 0)
		goto err2;

	pt_falloc.ni = PTL_NONMATCHING_PHYSICAL;
	pt_falloc.pt_idx = HFI_TEST_PT;
	pt_falloc.flags = flags;
	pt_falloc.start = (uint64_t)rx_base;
	pt_falloc.length = PAYLOAD_SIZE;
	pt_falloc.ct_handle = ct_rx;
	pt_falloc.eq_handle = HFI_EQ_NONE;
	/* Populate an RX buffer into a PT entry */
	rc = hfi_pt_fast_alloc(ctx, rx, &pt_falloc, &pt_idx);
	if (rc < 0)
		goto err2;

	/* TX data to the fast PTE */
	rc = hfi_tx_write(tx, ctx, ni, tx_base,
			  PAYLOAD_SIZE,
			  target_id, 0,
			  HFI_TEST_PT,
			  0, 0, 0, 0, 0, 0,
			  0xdead, 0,
			  hdr_data, ack_req, md_options,
			  HFI_EQ_NONE,
			  ct_tx,
			  remote_offset,
			  tx_handle);
	if (rc < 0)
		goto err2;

	/* Wait for TX command 1 to complete */
	rc = hfi_ct_wait_timed(ctx, ct_tx, 1, OPA2_TX_TIMEOUT_MS, NULL);
	if (rc < 0) {
		dev_err(&odev->dev, "TX CT event 1 failure, %d\n", rc);
		goto err2;
	} else {
		dev_info(&odev->dev, "TX CT event 1 success\n");
	}

	/* Wait to receive from peer */
	rc = hfi_ct_wait_timed(ctx, ct_rx, 1, OPA2_TX_TIMEOUT_MS, NULL);
	if (rc < 0) {
		dev_err(&odev->dev, "RX CT failure, %d\n", rc);
		goto err2;
	} else {
		dev_info(&odev->dev, "RX CT success\n");
	}

	/* verify the data transfer succeeded for PTE */
	if (!memcmp(tx_base, rx_base, PAYLOAD_SIZE))
		dev_info(&odev->dev, "basic PTE data transfer test passed\n");
	else
		dev_err(&odev->dev, "basic PTE data transfer test failed\n");

	kfree(rx_base);
	kfree(tx_base);
	return 0;
err2:
	kfree(rx_base);
err1:
	kfree(tx_base);
err0:
	return rc;
}

int opa_kfi_setup(struct opa_core_device *odev)
{
	struct opa_kfi *dev = opa_core_get_priv_data(&opa_kfi_clnt, odev);
	struct hfi_ctx *ctx = &dev->ctx;
	struct opa_ctx_assign ctx_assign = {0};
	struct opa_core_ops *ops = odev->bus_ops;
	int rc;

	HFI_CTX_INIT(ctx, odev->dd, odev->bus_ops);
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

	rc = opa_xfer_test(odev, dev);
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

void opa_kfi_cleanup(struct opa_core_device *odev)
{
	struct opa_kfi *dev = opa_core_get_priv_data(&opa_kfi_clnt, odev);
	struct hfi_ctx *ctx = &dev->ctx;
	struct opa_core_ops *ops = odev->bus_ops;

	ops->cq_unmap(&dev->tx, &dev->rx);
	ops->cq_release(ctx, dev->cq_idx);
	odev->bus_ops->ctx_release(ctx);
}

static int opa_kfi_probe(struct opa_core_device *odev)
{
	struct opa_kfi *dev;
	int rc;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		rc = -ENOMEM;
		goto err;
	}
	rc = opa_core_set_priv_data(&opa_kfi_clnt, odev, dev);
	if (rc)
		goto priv_err;

	rc = opa_kfi_setup(odev);
	if (rc)
		goto setup_err;

	return 0;
setup_err:
	opa_core_clear_priv_data(&opa_kfi_clnt, odev);
priv_err:
	kfree(dev);
err:
	return rc;
}

static void opa_kfi_remove(struct opa_core_device *odev)
{
	struct opa_kfi *dev = opa_core_get_priv_data(&opa_kfi_clnt, odev);

	if (!dev)
		return;
	opa_kfi_cleanup(odev);
	opa_core_clear_priv_data(&opa_kfi_clnt, odev);
	kfree(dev);
}

static int __init opa_kfi_init_module(void)
{
	return opa_core_client_register(&opa_kfi_clnt);
}
module_init(opa_kfi_init_module);

static void __exit opa_kfi_exit_module(void)
{
	opa_core_client_unregister(&opa_kfi_clnt);
}
module_exit(opa_kfi_exit_module);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Intel Corporation");
MODULE_DESCRIPTION("Intel(R) Omni-Path Gen2 KFI Driver");
