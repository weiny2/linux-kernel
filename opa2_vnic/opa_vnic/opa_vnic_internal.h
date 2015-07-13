#ifndef _OPA_VNIC_INTERNAL_H
#define _OPA_VNIC_INTERNAL_H
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
 * Intel(R) Omni-Path VNIC driver internal declarations
 */

#include <linux/etherdevice.h>

#define OPA_VNIC_L2_HDR_LEN   10
#define OPA_VNIC_L4_HDR_LEN   2
#define OPA_VNIC_MAX_PAD_LEN  16

#define OPA_VNIC_HDR_LEN      (OPA_VNIC_L2_HDR_LEN + \
			       OPA_VNIC_L4_HDR_LEN)

/**
 * struct opa_vnic_adapter - OPA VNIC netdev private data structure
 * @netdev: pointer to associated netdev
 * @vdev: pointer to opa vnic device
 * @napi: netdev napi structure
 */
struct opa_vnic_adapter {
	struct net_device        *netdev;
	struct opa_vnic_device   *vdev;

	struct napi_struct        napi;
};

#define v_dbg(format, arg...) \
	netdev_dbg(adapter->netdev, format, ## arg)
#define v_err(format, arg...) \
	netdev_err(adapter->netdev, format, ## arg)
#define v_info(format, arg...) \
	netdev_info(adapter->netdev, format, ## arg)
#define v_warn(format, arg...) \
	netdev_warn(adapter->netdev, format, ## arg)
#define v_notice(format, arg...) \
	netdev_notice(adapter->netdev, format, ## arg)

extern char opa_vnic_driver_name[];
extern const char opa_vnic_driver_version[];

void opa_vnic_encap_skb(struct opa_vnic_adapter *adapter, struct sk_buff *skb);
void opa_vnic_decap_skb(struct opa_vnic_adapter *adapter, struct sk_buff *skb);

void opa_vnic_set_ethtool_ops(struct net_device *ndev);

#endif /* _OPA_VNIC_INTERNAL_H */
