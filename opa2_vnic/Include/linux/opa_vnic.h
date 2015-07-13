#ifndef _OPA_VNIC_H
#define _OPA_VNIC_H
/*
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright(c) 2015 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * BSD LICENSE
 *
 * Copyright(c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Intel Corporation nor the names of its
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
 */

/*
 * Intel(R) Omni-Path Vitual Network Interface Controller driver interfaces
 */

#include <linux/etherdevice.h>

enum opa_vnic_hfi_evt {
	OPA_VNIC_HFI_EVT_RX,      /* Packet received event */
	OPA_VNIC_HFI_NUM_EVTS
};

typedef void (*opa_vnic_hfi_evt_cb_fn)(struct net_device *netdev,
				       enum opa_vnic_hfi_evt evt);

struct opa_vnic_device;

/**
 * struct opa_vnic_hfi_ops - OPA VNIC HFI functions
 * @hfi_init: Initialize the hfi device
 * @hfi_deinit: De-initialize the hfi device
 * @hfi_open: Open vnic hfi device for vnic traffic
 * @hfi_close: Close vnic hfi device for vnic traffic
 * @hfi_put_skb: transmit an skb
 * @hfi_get_skb: receive an skb
 */
struct opa_vnic_hfi_ops {
	int (*hfi_init)(struct opa_vnic_device *vdev);
	void (*hfi_deinit)(struct opa_vnic_device *vdev);
	int (*hfi_open)(struct opa_vnic_device *vdev,
			opa_vnic_hfi_evt_cb_fn cb);
	void (*hfi_close)(struct opa_vnic_device *vdev);
	int (*hfi_put_skb)(struct opa_vnic_device *vdev, struct sk_buff *skb);
	struct sk_buff *(*hfi_get_skb)(struct opa_vnic_device *vdev);
};

/**
 * struct opa_vnic_device - OPA virtual NIC device
 * @dev: device
 * @id: unique opa vnic device instance
 * @netdev: pointer to associated netdev
 * @bus_ops: opa vnic bus operations
 * @vnic_cb: vnic callback function
 * @hfi_priv: hfi private data pointer
 * @opa_mtu: MTU of opa link
 * @lid: LID of opa port
 */
struct opa_vnic_device {
	struct device                dev;
	int                          id;
	struct net_device           *netdev;

	struct opa_vnic_hfi_ops     *bus_ops;

	opa_vnic_hfi_evt_cb_fn       vnic_cb;
	void                        *hfi_priv;

	/* TODO: mtu and lid can change dynamically, need to fix */
	u32                          opa_mtu;
	u32                          lid;
};

/* OPA virtual NIC driver */
struct opa_vnic_driver {
	struct device_driver         driver;
};

/* Interface functions */
int opa_vnic_driver_register(struct opa_vnic_driver *drv);
void opa_vnic_driver_unregister(struct opa_vnic_driver *drv);

struct opa_vnic_device *opa_vnic_device_register(int id, void *priv,
						 struct opa_vnic_hfi_ops *ops);
void opa_vnic_device_unregister(struct opa_vnic_device *vdev);

struct opa_vnic_device *opa_vnic_get_dev(int id);

#endif /* _OPA_VNIC_H */
