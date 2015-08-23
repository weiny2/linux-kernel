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
 * Intel(R) Omni-Path Virtual Network Interface Controller (VNIC) driver
 */

#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/module.h>
#include <linux/if_vlan.h>
#include <linux/opa_vnic.h>

#include "opa_vnic_internal.h"

#define DRV_VERSION "1.0"
char opa_vnic_driver_name[] = "opa_vnic";
const char opa_vnic_driver_version[] = DRV_VERSION;

#define OPA_TX_TIMEOUT_MS 1000

/* opa_netdev_start_xmit - transmit function */
static netdev_tx_t opa_netdev_start_xmit(struct sk_buff *skb,
					 struct net_device *netdev)
{
	struct opa_vnic_adapter *adapter = netdev_priv(netdev);
	struct opa_vnic_device *vdev = adapter->vdev;
	int rc;

	/* TODO: Use the TX command queue to send this skb */
	v_dbg("xmit: skb len %d\n", skb->len);
#ifdef OPA_DEBUG
	print_hex_dump(KERN_INFO, "skb: ", DUMP_PREFIX_OFFSET,
		       16, 1, skb->data, skb->len, 0);
#endif
	opa_vnic_encap_skb(adapter, skb);

	rc = vdev->bus_ops->hfi_put_skb(vdev, skb);
	if (!rc) {
		netdev->stats.tx_packets++;
		netdev->stats.tx_bytes += skb->len;
	} else {
		netdev->stats.tx_dropped++;
	}
	dev_kfree_skb_any(skb);
	return NETDEV_TX_OK;
}

/* vnic_handle_rx - handle skb receive */
static void vnic_handle_rx(struct opa_vnic_adapter *adapter,
			   int *work_done, int work_to_do)
{
	struct opa_vnic_device *vdev = adapter->vdev;
	struct sk_buff *skb;

	while (1) {
		if (*work_done >= work_to_do)
			break;

		skb = vdev->bus_ops->hfi_get_skb(vdev);
		if (!skb)
			break;

		opa_vnic_decap_skb(adapter, skb);
#ifdef OPA_DEBUG
		print_hex_dump(KERN_INFO, "skb: ", DUMP_PREFIX_OFFSET,
			       16, 1, skb->data, skb->len, 0);
#endif
		skb->ip_summed = CHECKSUM_UNNECESSARY;
		skb->protocol = eth_type_trans(skb, vdev->netdev);

		adapter->netdev->stats.rx_packets++;
		adapter->netdev->stats.rx_bytes += skb->len;
		napi_gro_receive(&adapter->napi, skb);
		(*work_done)++;
	}
}

/* vnic_napi - napi receive polling callback function */
static int vnic_napi(struct napi_struct *napi, int budget)
{
	struct opa_vnic_adapter *adapter = container_of(napi,
						struct opa_vnic_adapter, napi);
	int work_done = 0;

	v_dbg("budget %d\n", budget);
	vnic_handle_rx(adapter, &work_done, budget);

	v_dbg("work_done %d\n", work_done);
	/* TODO: need to disable/enable event notification */
	if (work_done < budget)
		napi_complete(napi);

	return work_done;
}

/* vnic_event_cb - handle events from vnic hfi driver */
void vnic_event_cb(struct net_device *netdev, enum opa_vnic_hfi_evt evt)
{
	struct opa_vnic_adapter *adapter = netdev_priv(netdev);

	v_dbg("received event %d\n", evt);
	switch (evt) {
	case OPA_VNIC_HFI_EVT_RX:
		if (napi_schedule_prep(&adapter->napi)) {
			v_dbg("napi scheduled\n");
			__napi_schedule(&adapter->napi);
		}
		break;

	default:
		v_err("Invalid event\n");
		break;
	}
}

/* opa_netdev_open - activate network interface */
static int opa_netdev_open(struct net_device *netdev)
{
	struct opa_vnic_adapter *adapter = netdev_priv(netdev);
	struct opa_vnic_device *vdev = adapter->vdev;
	int rc;

	rc = vdev->bus_ops->hfi_open(vdev, vnic_event_cb);
	if (rc)
		return rc;

	netif_carrier_on(netdev);
	napi_enable(&adapter->napi);
	v_info("opened\n");

	return 0;
}

/* opa_netdev_close - disable network interface */
static int opa_netdev_close(struct net_device *netdev)
{
	struct opa_vnic_adapter *adapter = netdev_priv(netdev);
	struct opa_vnic_device *vdev = adapter->vdev;

	napi_disable(&adapter->napi);
	netif_carrier_off(netdev);
	vdev->bus_ops->hfi_close(vdev);
	v_info("closed\n");

	return 0;
}

static int opa_netdev_change_mtu(struct net_device *netdev, int new_mtu)
{
	struct opa_vnic_adapter *adapter = netdev_priv(netdev);
	int max_frame = new_mtu + VLAN_HLEN + ETH_HLEN +
			OPA_VNIC_HDR_LEN + OPA_VNIC_MAX_PAD_LEN;

	/* Supported frame sizes */
	if ((new_mtu < ETH_ZLEN + VLAN_HLEN) ||
	    (max_frame > adapter->vdev->opa_mtu)) {
		v_err("Unsupported MTU setting\n");
		return -EINVAL;
	}

	v_info("changing MTU from %d to %d\n", netdev->mtu, new_mtu);
	netdev->mtu = new_mtu;

	return 0;
}

/* netdev ops */
static const struct net_device_ops opa_netdev_ops = {
	.ndo_open = opa_netdev_open,
	.ndo_stop = opa_netdev_close,
	.ndo_start_xmit = opa_netdev_start_xmit,
	.ndo_change_mtu = opa_netdev_change_mtu,
	.ndo_set_mac_address = eth_mac_addr,
};

/* opa_vnic_drv_probe - device initialization routine */
static int opa_vnic_drv_probe(struct device *dev)
{
	struct net_device *netdev;
	struct opa_vnic_adapter *adapter;
	struct opa_vnic_device *vdev = container_of(dev,
					    struct opa_vnic_device, dev);
	int rc;

	netdev = alloc_etherdev(sizeof(struct opa_vnic_adapter));
	if (!netdev)
		return -ENOMEM;

	adapter = netdev_priv(netdev);
	adapter->netdev = netdev;
	adapter->vdev = vdev;
	vdev->netdev = netdev;
	netdev->features = NETIF_F_HIGHDMA;
	netdev->priv_flags |= IFF_LIVE_ADDR_CHANGE;
	netdev->hw_features = netdev->features;
	netdev->watchdog_timeo = msecs_to_jiffies(OPA_TX_TIMEOUT_MS);
	random_ether_addr(netdev->perm_addr);
	memcpy(netdev->dev_addr, netdev->perm_addr, netdev->addr_len);
	netdev->netdev_ops = &opa_netdev_ops;
	netdev->hard_header_len += 16;
	netdev->needed_tailroom += 16;
	strcpy(netdev->name, "veth%d");

	opa_vnic_set_ethtool_ops(netdev);

	netif_napi_add(netdev, &adapter->napi, vnic_napi, 64);

	rc = vdev->bus_ops->hfi_init(vdev);
	if (rc)
		goto hw_err;

	rc = register_netdev(netdev);
	if (rc)
		goto netdev_err;

	netif_carrier_off(netdev);
	opa_vnic_dbg_vport_init(adapter);
	v_info("initialized\n");

	return 0;

netdev_err:
	vdev->bus_ops->hfi_deinit(vdev);
hw_err:
	free_netdev(netdev);
	dev_err(dev, "initialization failed %d\n", rc);

	return rc;
}

/* opa_vnic_drv_remove - device removal routine */
static int opa_vnic_drv_remove(struct device *dev)
{
	struct opa_vnic_device *vdev = container_of(dev,
					    struct opa_vnic_device, dev);
	struct opa_vnic_adapter *adapter = netdev_priv(vdev->netdev);

	opa_vnic_dbg_vport_exit(adapter);
	unregister_netdev(vdev->netdev);
	vdev->bus_ops->hfi_deinit(vdev);
	free_netdev(vdev->netdev);

	dev_info(dev, "removed\n");
	return 0;
}

/* Omni-Path Virtual Network Driver */
/* TODO: probably have bus implement driver routines */
static struct opa_vnic_driver opa_vnic_drv = {
	.driver = {
		.name   = opa_vnic_driver_name,
		.probe  = opa_vnic_drv_probe,
		.remove = opa_vnic_drv_remove
	}
};

/* opa_vnic_init_module - driver registration routine */
static int __init opa_vnic_init_module(void)
{
	int rc;

	pr_info("Intel(R) Omni-Path Virtual Network Driver - %s\n",
		opa_vnic_driver_version);
	opa_vnic_dbg_init();
	rc = opa_vnic_driver_register(&opa_vnic_drv);
	if (rc)
		pr_err("Driver register failed %d\n", rc);

	return rc;
}
module_init(opa_vnic_init_module);

/* opa_vnic_exit_module - driver Exit cleanup routine */
static void __exit opa_vnic_exit_module(void)
{
	opa_vnic_driver_unregister(&opa_vnic_drv);
	opa_vnic_dbg_exit();
}
module_exit(opa_vnic_exit_module);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Intel Corporation");
MODULE_DESCRIPTION("Intel(R) Omni-Path Virtual Network Controller driver");
MODULE_VERSION(DRV_VERSION);
