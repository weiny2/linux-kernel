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

enum opa_vnic_drv_type {
	OPA_VNIC_CTRL_DRV,
	OPA_VNIC_DRV
};

enum opa_vnic_hfi_evt {
	OPA_VNIC_HFI_EVT_RX,      /* Packet received event */
	OPA_VNIC_HFI_NUM_EVTS
};

#define OPA_VNIC_RCV_ARRAY_SIZE   512
#define OPA_VNIC_RCV_ARRAY_MASK   (OPA_VNIC_RCV_ARRAY_SIZE - 1)

typedef void (*opa_vnic_hfi_evt_cb_fn)(struct net_device *netdev,
				       enum opa_vnic_hfi_evt evt);

struct opa_vnic_device;
struct opa_vnic_ctrl_device;

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
 * struct opa_vnic_ctrl_ops - OPA VNIC control functions
 * @add_vport: add a vnic port device
 * @rem_vport: remove a vnic port device
 */
struct opa_vnic_ctrl_ops {
	int (*add_vport)(struct opa_vnic_ctrl_device *vdev,
			 u8 port_num, u8 vesw_id);
	void (*rem_vport)(struct opa_vnic_ctrl_device *vdev,
			  u8 port_num, u8 vesw_id);
};

/* VNIC_TODO: remove this structure from interface definition */
struct vnic_recv_desc_ring {
	struct sk_buff *skb_array[OPA_VNIC_RCV_ARRAY_SIZE];
	u16 head;
	u16 tail;
};

/**
 * struct opa_vnic_device - OPA virtual NIC device
 * @dev: device
 * @id: unique opa vnic device instance
 * @vesw_id: virtual ethernet switch id
 * @netdev: pointer to associated netdev
 * @cdev: vnic control device pointer
 * @bus_ops: opa vnic bus operations
 * @vnic_cb: vnic callback function
 * @hfi_priv: hfi private data pointer
 * @opa_mtu: MTU of opa link
 * @lid: LID of opa port
 */
struct opa_vnic_device {
	struct device                dev;
	int                          id;
	u8                           vesw_id;
	struct net_device           *netdev;

	struct opa_vnic_ctrl_device *cdev;
	struct opa_vnic_hfi_ops     *bus_ops;

	opa_vnic_hfi_evt_cb_fn       vnic_cb;
	struct vnic_recv_desc_ring   recv;
	void                        *hfi_priv;

	/* TODO: mtu and lid can change dynamically, need to fix */
	u32                          opa_mtu;
	u32                          lid;
};

/**
 * struct opa_vnic_ctrl_device - OPA virtual NIC control device
 * @dev: device
 * @id: unique opa vnic control device instance
 * @ibdev: pointer to ib device
 * @num_ports: number of opa ports
 * @ctrl_ops: opa vnic control operations
 * @hfi_priv: hfi private data pointer
 */
struct opa_vnic_ctrl_device {
	struct device                dev;
	int                          id;

	struct ib_device            *ibdev;
	u8                           num_ports;

	struct opa_vnic_ctrl_ops    *ctrl_ops;
	void                        *hfi_priv;
};

/**
 * struct opa_vnic_drvwrap - OPA vnic driver wrapper
 * @type: driver type
 * @driver: device driver
 */
struct opa_vnic_drvwrap {
	u8                           type;
	struct device_driver         driver;
};

/**
 * struct opa_vnic_driver - OPA virtual NIC driver
 * @drvwrap: driver wrapper
 */
struct opa_vnic_driver {
	struct opa_vnic_drvwrap      drvwrap;
};

/**
 * struct opa_vnic_ctrl_driver -  OPA virtual NIC Control driver
 * @drvwrap: driver wrapper
 */
struct opa_vnic_ctrl_driver {
	struct opa_vnic_drvwrap       drvwrap;
};

/* VNIC device interface functions */
int opa_vnic_driver_register(struct opa_vnic_driver *drv);
void opa_vnic_driver_unregister(struct opa_vnic_driver *drv);

struct opa_vnic_device *opa_vnic_device_register(
					 struct opa_vnic_ctrl_device *cdev,
					 u8 port_num, u8 vport_num, void *priv,
					 struct opa_vnic_hfi_ops *ops);
void opa_vnic_device_unregister(struct opa_vnic_device *vdev);

struct opa_vnic_device *opa_vnic_get_dev(struct opa_vnic_ctrl_device *cdev,
					 u8 port_num, u8 vport_num);

/* VNIC control device interface functions */
int opa_vnic_ctrl_driver_register(struct opa_vnic_ctrl_driver *drv);
void opa_vnic_ctrl_driver_unregister(struct opa_vnic_ctrl_driver *drv);

struct opa_vnic_ctrl_device *opa_vnic_ctrl_device_register(
					   struct ib_device *ibdev,
					   u8 num_ports, void *priv,
					   struct opa_vnic_ctrl_ops *ops);
void opa_vnic_ctrl_device_unregister(struct opa_vnic_ctrl_device *cdev);

#endif /* _OPA_VNIC_H */
