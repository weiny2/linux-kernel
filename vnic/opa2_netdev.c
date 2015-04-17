/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2014 - 2015 Intel Corporation.
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
 * Copyright (c) 2014 - 2015 Intel Corporation.
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
 * Intel(R) Omni-Path Gen2 network driver
 */
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <rdma/opa_core.h>
#include <rdma/hfi_misc.h>
#include <rdma/hfi_tx_base.h>
#include <rdma/hfi_tx_cmds.h>
#include <rdma/hfi_rx_cmds.h>
#include <rdma/hfi_cq_cmd.h>
#include <rdma/hfi_tx_pio_put.h>
#include <rdma/hfi_tx.h>
#include <rdma/hfi_rx.h>
#include <rdma/hfi_args.h>
#include <rdma/hfi_ct.h>

static int opa_netdev_probe(struct opa_core_device *odev);
static void opa_netdev_remove(struct opa_core_device *odev);

static struct opa_core_client opa_netdev = {
	.name = KBUILD_MODNAME,
	.add = opa_netdev_probe,
	.remove = opa_netdev_remove
};

/*
 * struct opa_netdev - OPA2 net device specific fields
 *
 * @pdev - PCIe device backing this net device
 * @ndev - Net device registered
 * @ctx - HFI context
 * @ctx_assign - pointer of the HFI CT table (RO)
 * @cq_idx - command queue index
 * @tx - TX command queue
 * @rx - RX command queue
 */
struct opa_netdev {
	struct pci_dev		*pdev;
	struct net_device	*ndev;
	struct hfi_ctx		ctx;
	u16			cq_idx;
	struct hfi_cq		tx;
	struct hfi_cq		rx;
};

#define OPA2_RXQ_SIZE 0
#define OPA2_NET_ME_COUNT 256
#define OPA2_NET_UNEX_COUNT 512
#define OPA2_TX_TIMEOUT_MS 1000

enum {
	OPA_LINK_DOWN = 0,
	OPA_LINK_UP,
};

static netdev_tx_t opa_netdev_start_xmit(struct sk_buff *skb,
					 struct net_device *ndev)
{
	/* TODO: Use the TX command queue to send this skb */
	netdev_printk(KERN_INFO, ndev, "%s %d\n", __func__, __LINE__);
	return NETDEV_TX_OK;
}

static int opa_netdev_open(struct net_device *ndev)
{
	struct sk_buff *skb;
	int rc, i;

	netdev_printk(KERN_INFO, ndev, "%s %d\n", __func__, __LINE__);
	for (i = 0; i < OPA2_RXQ_SIZE; i++) {
		/* Allocate some RX buffers */
		skb = netdev_alloc_skb(ndev, ndev->mtu + ETH_HLEN);
		if (!skb) {
			rc = -ENOMEM;
			goto err;
		}
		/* TODO: Use the RX command queue to post RX buffers */
	}

	netif_carrier_off(ndev);
	netdev_printk(KERN_INFO, ndev, "%s %d\n", __func__, __LINE__);
	return 0;
err:
	/* TODO: Free RX buffers allocated */
	return rc;
}

static int opa_netdev_close(struct net_device *ndev)
{
	/* TODO: Free RX buffers allocated */
	netdev_printk(KERN_INFO, ndev, "%s %d\n", __func__, __LINE__);
	return 0;
}

static int opa_netdev_change_mtu(struct net_device *ndev, int new_mtu)
{
	/* TODO: Free RX buffers previously allocated and allocate new ones */
	netdev_printk(KERN_INFO, ndev, "%s %d\n", __func__, __LINE__);
	return 0;
}

static const struct net_device_ops opa_netdev_ops = {
	.ndo_open = opa_netdev_open,
	.ndo_stop = opa_netdev_close,
	.ndo_start_xmit = opa_netdev_start_xmit,
	.ndo_change_mtu = opa_netdev_change_mtu,
	.ndo_set_mac_address = eth_mac_addr,
};

#if 0
 /* Disabling since it is resulting in crashes on rhel 7.0 */
static void opa_get_drvinfo(struct net_device *ndev,
			    struct ethtool_drvinfo *info)
{
	struct opa_netdev *dev = netdev_priv(ndev);

	netdev_printk(KERN_INFO, ndev, "%s %d\n", __func__, __LINE__);
	strlcpy(info->driver, KBUILD_MODNAME, sizeof(info->driver));
	strlcpy(info->bus_info, pci_name(dev->pdev), sizeof(info->bus_info));
}
#endif

static int opa_get_settings(struct net_device *ndev, struct ethtool_cmd *cmd)
{
	netdev_printk(KERN_INFO, ndev, "%s %d\n", __func__, __LINE__);
	cmd->supported = SUPPORTED_Backplane;
	cmd->advertising = ADVERTISED_Backplane;
	ethtool_cmd_speed_set(cmd, SPEED_UNKNOWN);
	cmd->duplex = DUPLEX_FULL;
	cmd->port = PORT_OTHER;
	cmd->phy_address = 0;
	cmd->transceiver = XCVR_DUMMY1;
	cmd->autoneg = AUTONEG_ENABLE;
	cmd->maxtxpkt = 0;
	cmd->maxrxpkt = 0;

	return 0;
}

static const struct ethtool_ops opa_ethtool_ops = {
#if 0
	 /* Disabling since it is resulting in crashes on rhel 7.0 */
	.get_drvinfo = opa_get_drvinfo,
#endif
	.get_link = ethtool_op_get_link,
	.get_settings = opa_get_settings,
};

static inline int
hfi_ct_wait(struct hfi_ctx *ctx, hfi_ct_handle_t ct_h,
	    unsigned long threshold, unsigned long *ct_val)
{
	unsigned long val;

	while (1) {
		if (hfi_ct_get_failure(ctx, ct_h))
			return -EFAULT;
		val = hfi_ct_get_success(ctx, ct_h);
		if (val >= threshold)
			break;
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

static int opa2_xfer_test(struct opa_core_device *odev, struct opa_netdev *dev)
{
	struct hfi_ctx *ctx = &dev->ctx;
	struct hfi_cq *tx = &dev->tx;
	struct hfi_cq *rx = &dev->rx;
	int rc;
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

	target_id.phys.slid = 0;
	target_id.phys.ipid = ctx->ptl_pid;
	match_id.phys.slid = 0;
	match_id.phys.ipid = ctx->ptl_pid;
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

	hfi_format_rx_append(ctx, ni,
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
	rc = hfi_rx_command(rx, (uint64_t *)&rx_cmd);

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
	rc = hfi_ct_wait(ctx, ct_tx, 1, NULL);
	if (rc < 0)
		goto err3;
	else
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
	rc = hfi_ct_wait(ctx, ct_tx, 2, NULL);
	if (rc < 0)
		goto err3;
	else
		dev_info(&odev->dev, "TX CT event 2 success\n");

	/* Wait to receive from peer */
	rc = hfi_ct_wait(ctx, ct_rx, 1, NULL);
	if (rc < 0)
		goto err3;
	else
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

static int opa2_hw_init(struct opa_core_device *odev, struct opa_netdev *dev)
{
	struct hfi_ctx *ctx = &dev->ctx;
	struct opa_ctx_assign ctx_assign = {0};
	struct opa_core_ops *ops = odev->bus_ops;
	struct hfi_cq *tx = &dev->tx;
	struct hfi_cq *rx = &dev->rx;
	struct hfi_auth_tuple auth_table[HFI_NUM_AUTH_TUPLES];
	ssize_t head_size;
	int rc, i;

	ctx->ptl_pid = HFI_PID_NONE;
	ctx->devdata = odev->dd;
	ctx_assign.le_me_count = OPA2_NET_ME_COUNT;
	ctx_assign.unexpected_count = OPA2_NET_UNEX_COUNT;
	ctx->allow_phys_dlid = 1;
	ctx->sl_mask = -1;   /* allow all SLs */

	rc = ops->ctx_assign(ctx, &ctx_assign);
	if (rc)
		return rc;
	/* stash pointer to array of CT events */
	rc = ops->ctx_addr(ctx, TOK_EVENTS_CT, ctx->ptl_pid,
			   &ctx->ct_addr, &ctx->ct_size);
	if (rc)
		goto err;
	/* stash pointer to array of EQ descs */
	rc = ops->ctx_addr(ctx, TOK_EVENTS_EQ_DESC, ctx->ptl_pid,
			   &ctx->eq_addr, &ctx->eq_size);
	if (rc)
		goto err;
	/* stash pointer to array of EQ head pointers */
	rc = ops->ctx_addr(ctx, TOK_EVENTS_EQ_HEAD, ctx->ptl_pid,
			   &ctx->eq_head_addr, &ctx->eq_head_size);
	if (rc)
		goto err;
	/* stash pointer to Portals Table */
	rc = ops->ctx_addr(ctx, TOK_PORTALS_TABLE, ctx->ptl_pid,
			   &ctx->pt_addr, &ctx->pt_size);
	if (rc)
		goto err;
	for (i = 0; i < HFI_NUM_AUTH_TUPLES; i++) {
		auth_table[i].uid = 0;
		ctx->ptl_uid = 0;
		auth_table[i].srank = 0;
	}
	/* Obtain a pair of command queues */
	rc = ops->cq_assign(ctx, auth_table, &dev->cq_idx);
	if (rc)
		goto err;
	/* stash pointer to CQ head */
	rc = ops->ctx_addr(ctx, TOK_CQ_HEAD, dev->cq_idx,
			   (void **)&tx->head_addr, &head_size);
	if (rc)
		goto err1;
	/* stash pointer to TX CQ */
	rc = ops->ctx_addr(ctx, TOK_CQ_TX, dev->cq_idx,
			   &tx->base, (ssize_t *)&tx->size);
	if (rc)
		goto err1;
	tx->base = ioremap((u64)tx->base, tx->size);
	if (!tx->base)
		goto err1;
	/* stash pointer to RX CQ */
	rc = ops->ctx_addr(ctx, TOK_CQ_RX, dev->cq_idx,
			    &rx->base, (ssize_t *)&rx->size);
	if (rc)
		goto err2;
	rx->base = ioremap((u64)rx->base, rx->size);
	if (!rx->base)
		goto err2;

	tx->cq_idx = dev->cq_idx;
	tx->slots_total = HFI_CQ_TX_ENTRIES;
	tx->slots_avail = tx->slots_total - 1;
	tx->slot_idx = (*tx->head_addr);
	tx->sw_head_idx = tx->slot_idx;

	rx->cq_idx = dev->cq_idx;
	rx->head_addr = tx->head_addr + 8;
	rx->slots_total = HFI_CQ_RX_ENTRIES;
	rx->slots_avail = rx->slots_total - 1;
	rx->slot_idx = (*rx->head_addr);
	rx->sw_head_idx = rx->slot_idx;

	return opa2_xfer_test(odev, dev);
err2:
	iounmap(tx->base);
err1:
	ops->cq_release(ctx, dev->cq_idx);
err:
	ops->ctx_release(ctx);
	return rc;
}

static void opa2_hw_uninit(struct opa_core_device *odev, struct opa_netdev *dev)
{
	struct hfi_ctx *ctx = &dev->ctx;
	struct opa_core_ops *ops = odev->bus_ops;
	struct hfi_cq *tx = &dev->tx;
	struct hfi_cq *rx = &dev->rx;

	if (tx->base)
		iounmap(tx->base);
	if (rx->base)
		iounmap(rx->base);
	ops->cq_release(ctx, dev->cq_idx);
	odev->bus_ops->ctx_release(ctx);
}

static int opa_netdev_probe(struct opa_core_device *odev)
{
	struct net_device *ndev;
	struct opa_netdev *dev;
	int rc;

	ndev = alloc_etherdev(sizeof(struct opa_netdev));
	if (!ndev)
		return -ENOMEM;

	dev = netdev_priv(ndev);
	rc = opa_core_set_priv_data(&opa_netdev, odev, dev);
	if (rc)
		goto priv_err;
	dev->ndev = ndev;
	ndev->features = NETIF_F_HIGHDMA;
	ndev->priv_flags |= IFF_LIVE_ADDR_CHANGE;
	ndev->hw_features = ndev->features;
	ndev->watchdog_timeo = msecs_to_jiffies(OPA2_TX_TIMEOUT_MS);
	random_ether_addr(ndev->perm_addr);
	memcpy(ndev->dev_addr, ndev->perm_addr, ndev->addr_len);
	ndev->netdev_ops = &opa_netdev_ops;
	ndev->ethtool_ops = &opa_ethtool_ops;

	rc = opa2_hw_init(odev, dev);
	if (rc)
		goto hw_err;
	rc = register_netdev(ndev);
	if (rc)
		goto netdev_err;
	return 0;
netdev_err:
	opa2_hw_uninit(odev, dev);
hw_err:
	opa_core_clear_priv_data(&opa_netdev, odev);
priv_err:
	free_netdev(ndev);
	dev_err(&odev->dev, "%s error rc %d\n", __func__, rc);
	return rc;
}

static void opa_netdev_remove(struct opa_core_device *odev)
{
	struct net_device *ndev;
	struct opa_netdev *dev = opa_core_get_priv_data(&opa_netdev, odev);

	if (!dev)
		return;
	ndev = dev->ndev;
	unregister_netdev(ndev);
	opa2_hw_uninit(odev, dev);
	free_netdev(ndev);
	opa_core_clear_priv_data(&opa_netdev, odev);
}

static int __init opa_netdev_init_module(void)
{
	return opa_core_client_register(&opa_netdev);
}
module_init(opa_netdev_init_module);

static void __exit opa_netdev_exit_module(void)
{
	opa_core_client_unregister(&opa_netdev);
}
module_exit(opa_netdev_exit_module);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Intel Corporation");
MODULE_DESCRIPTION("Intel(R) Omni-Path Gen2 Network Driver");
