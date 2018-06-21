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
#include "../core/hfi_kclient.h"
#include "../core/hfi_ct.h"
#include "kfi_pt.h"
#include "kfi_tx.h"

static void opa_kfi_probe(struct ib_device *ibdev);
static void opa_kfi_remove(struct ib_device *ibdev, void *client_data);

static struct ib_client opa_kfi_clnt = {
	.name = KBUILD_MODNAME,
	.add = opa_kfi_probe,
	.remove = opa_kfi_remove,
};

/*
 * struct opa_kfi - OPA2 KFI context
 *
 * @ibdev - IB device
 * @uc - IB ucontext
 * @ctx - HFI context
 * @cmdq - Pair of command queues (TX and RX)
 * @eh - registers IB event handler callback
 */
struct opa_kfi {
	struct ib_device	*ibdev;
	struct hfi_ibcontext	*uc;
	struct hfi_ctx		ctx;
	struct hfi_cmdq_pair	cmdq;
	struct ib_event_handler eh;
};

static void opa_kfi_event_notify(struct ib_event_handler *eh,
				 struct ib_event *e)
{
	struct opa_kfi *dev = container_of(eh, struct opa_kfi, eh);
	struct ib_device *ibdev = e->device;

	/* FXRTODO: Add event handling */
	dev_info(&ibdev->dev, "%s port %d pid %d event %d\n", __func__,
		 e->element.port_num, dev->ctx.pid, e->event);
}

#define OPA2_NET_ME_COUNT 256
#define OPA2_NET_UNEX_COUNT 512
#define OPA2_TX_TIMEOUT_MS 100

static int opa_xfer_test(struct opa_kfi *dev)
{
	struct opa_core_ops *ops = dev->uc->ops;
	struct hfi_ctx *ctx = &dev->ctx;
	struct hfi_cmdq *tx = &dev->cmdq.tx;
	struct hfi_cmdq *rx = &dev->cmdq.rx;
	struct ib_device *ibdev = dev->ibdev;
	struct ib_port_attr port_attr;
	int rc;
	u8 ni = PTL_NONMATCHING_PHYSICAL;
	u64 hdr_data = 0x1;
	enum ptl_L4_ack_req ack_req = PTL_NO_ACK_REQ;
	u32 md_options = PTL_MD_EVENT_CT_SEND;
	u16 tx_handle = 0xc;
	size_t remote_offset = 0;
	union hfi_process target_id, match_id;
	u64 flags = (PTL_OP_PUT | PTL_OP_GET | PTL_EVENT_CT_COMM);
	u16 ct_rx, ct_tx;
	struct hfi_pt_fast_args pt_falloc = {0};
	struct hfi_e2e_conn e2e;
	void *tx_base, *rx_base;
	u32 pt_idx;
	u16 mtu;

	ibdev->query_port(ibdev, 1, &port_attr);
	mtu = ib_mtu_enum_to_int(port_attr.active_mtu);

	e2e.slid = port_attr.lid;
	e2e.dlid = port_attr.lid;
	e2e.port_num = 1;
	e2e.sl = 0;

	/* Use the first portals pkey, usually at index 4 */
	ibdev->query_pkey(ibdev, 1, 4, &e2e.pkey);

	rc = ops->e2e_ctrl(dev->uc, &e2e);
	if (rc)
		return rc;

	target_id.phys.slid = port_attr.lid;
	target_id.phys.ipid = ctx->pid;
	match_id.phys.slid = port_attr.lid;
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

	rc = hfi_ct_alloc(ctx, ni, &ct_tx);
	if (rc < 0)
		goto err2;
	rc = hfi_ct_alloc(ctx, ni, &ct_rx);
	if (rc < 0)
		goto err2;

	pt_falloc.flags = flags;
	pt_falloc.start = (uint64_t)rx_base;
	pt_falloc.length = PAYLOAD_SIZE;
	pt_falloc.ct_handle = ct_rx;
	pt_falloc.eq_handle = HFI_EQ_NONE;
	/* Populate an RX buffer into a PT entry */
	rc = hfi_pt_fast_alloc(ctx, rx, PTL_NONMATCHING_PHYSICAL,
			       HFI_TEST_PT, &pt_idx, pt_falloc);
	if (rc < 0)
		goto err2;

	/* TX data to the fast PTE */
	rc = hfi2_tx_write(tx, ctx, ni, tx_base,
			   PAYLOAD_SIZE, mtu,
			   target_id, 0,
			   HFI_TEST_PT,
			   0, e2e.sl, 0, e2e.pkey, 0, 0,
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
		dev_err(&ibdev->dev, "TX CT event 1 failure, %d\n", rc);
		goto err2;
	} else {
		dev_info(&ibdev->dev, "TX CT event 1 success\n");
	}

	/* Wait to receive from peer */
	rc = hfi_ct_wait_timed(ctx, ct_rx, 1, OPA2_TX_TIMEOUT_MS, NULL);
	if (rc < 0) {
		dev_err(&ibdev->dev, "RX CT failure, %d\n", rc);
		goto err2;
	} else {
		dev_info(&ibdev->dev, "RX CT success\n");
	}

	/* verify the data transfer succeeded for PTE */
	if (!memcmp(tx_base, rx_base, PAYLOAD_SIZE))
		dev_info(&ibdev->dev, "basic PTE data transfer test passed\n");
	else
		dev_err(&ibdev->dev, "basic PTE data transfer test failed\n");

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

static int opa_kfi_setup(struct opa_kfi *dev)
{
	struct hfi_ctx *ctx = &dev->ctx;
	struct opa_ctx_assign ctx_assign = {0};
	struct opa_core_ops *ops = dev->uc->ops;
	int rc;

	ops->ctx_init(dev->ibdev, ctx);
	ctx_assign.pid = HFI_PID_ANY;
	ctx_assign.le_me_count = OPA2_NET_ME_COUNT;
	ctx_assign.unexpected_count = OPA2_NET_UNEX_COUNT;
	rc = ops->ctx_assign(ctx, &ctx_assign);
	if (rc)
		return rc;

	/* Obtain a pair of command queues and setup */
	rc = ops->cmdq_assign(&dev->cmdq, ctx, NULL);
	if (rc)
		goto err;
	rc = ops->cmdq_map(&dev->cmdq);
	if (rc)
		goto err1;

	rc = opa_xfer_test(dev);
	if (rc)
		goto err2;
	return 0;
err2:
	ops->cmdq_unmap(&dev->cmdq);
err1:
	ops->cmdq_release(&dev->cmdq);
err:
	ops->ctx_release(ctx);
	return rc;
}

static void opa_kfi_cleanup(struct opa_kfi *dev)
{
	struct opa_core_ops *ops = dev->uc->ops;
	int rc;

	ops->cmdq_unmap(&dev->cmdq);
	ops->cmdq_release(&dev->cmdq);
	rc = ops->ctx_release(&dev->ctx);
	if (rc)
		dev_warn(&dev->ibdev->dev, "Unable to release ctx\n");
}

static void opa_kfi_probe(struct ib_device *ibdev)
{
	struct opa_kfi *dev;
	struct ib_ucontext *ucontext;
	int rc;

	/* ignore IB devices except hfi2 devices */
	if (!(ibdev->attrs.vendor_id == PCI_VENDOR_ID_INTEL &&
	      ibdev->attrs.vendor_part_id == PCI_DEVICE_ID_INTEL_FXR0))
		return;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return;
	dev->ibdev = ibdev;

	ucontext = ibdev->alloc_ucontext(ibdev, NULL);
	if (IS_ERR(ucontext))
		goto err;
	/* store IB context handle which contains ops table */
	dev->uc = (struct hfi_ibcontext *)ucontext;

	INIT_IB_EVENT_HANDLER(&dev->eh, dev->ibdev, opa_kfi_event_notify);
	ib_register_event_handler(&dev->eh);

	rc = opa_kfi_setup(dev);
	if (rc)
		goto setup_err;

	ib_set_client_data(ibdev, &opa_kfi_clnt, dev);
	return;

setup_err:
	ib_unregister_event_handler(&dev->eh);
	ibdev->dealloc_ucontext(ucontext);
err:
	kfree(dev);
}

static void opa_kfi_remove(struct ib_device *ibdev, void *client_data)
{
	struct opa_kfi *dev = client_data;

	if (!dev)
		return;
	opa_kfi_cleanup(dev);
	ib_unregister_event_handler(&dev->eh);
	ibdev->dealloc_ucontext(&dev->uc->ibuc);
	kfree(dev);
}

static int __init opa_kfi_init_module(void)
{
	return ib_register_client(&opa_kfi_clnt);
}
module_init(opa_kfi_init_module);

static void __exit opa_kfi_exit_module(void)
{
	ib_unregister_client(&opa_kfi_clnt);
}
module_exit(opa_kfi_exit_module);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Intel Corporation");
MODULE_DESCRIPTION("Intel(R) Omni-Path Gen2 KFI Driver");
