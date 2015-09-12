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
 * Intel(R) Omni-Path VNIC encapsulation/decapsulation function.
 */

#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/module.h>
#include <linux/if_ether.h>
#include <linux/if_vlan.h>
#include <linux/opa_vnic.h>

#include "opa_vnic_internal.h"

/**
 * struct opa_bypass10_pkt - opa bypass10 (10B) packet format
 * @slid : source lid
 * @length : length of packet
 * @b : BCEN
 * @dlid : destination lid
 * @sc : service class
 * @rc : route control
 * @f : FCEN
 * @l2 : L2 type (1=10B, 2=16B)
 * @lt : link transfer field
 * @l4 : L4 type
 * @pkey : partition key
 * @entropy : entropy
 * @l4_hdr : L4 header
 * @payload : payload
 */
union opa_bypass10_pkt {
	struct {
	struct {
		uint64_t        slid    : 20;
		uint64_t        length  : 11;
		uint64_t        b       : 1;
		uint64_t        dlid    : 20;
		uint64_t        sc      : 5;
		uint64_t        rc      : 3;
		uint64_t        f       : 1;
		uint64_t        l2      : 2;
		uint64_t        lt      : 1;
	};
	struct {
		uint32_t        l4      : 4;
		uint32_t        pkey    : 4;
		uint32_t        entropy : 8;
		uint32_t        l4_hdr  : 16;
	};
	} __packed;
	uint32_t dw[3];
};

#define OPA_VNIC_ICRC_LEN   4
#define OPA_VNIC_TAIL_LEN   1
#define OPA_VNIC_ICRC_TAIL_LEN  (OPA_VNIC_ICRC_LEN + OPA_VNIC_TAIL_LEN)

#define OPA_VNIC_IS_DMAC_MCAST(mac_hdr)  ((mac_hdr)->h_dest[0] & 0x01)
#define OPA_VNIC_IS_DMAC_LOCAL(mac_hdr)  ((mac_hdr)->h_dest[0] & 0x02)

#define OPA_VNIC_VLAN_PCP(vlan_tci)  \
			(((vlan_tci) & VLAN_PRIO_MASK) >> VLAN_PRIO_SHIFT)

/* TODO: add 16B support; 10B dlid has 20 bits */
#define OPA_VNIC_DLID_MASK 0xfffff
#define OPA_VNIC_SC_MASK 0x1f

/* opa_vnic_get_sc - return the service class */
static u8 opa_vnic_get_sc(struct opa_veswport_info *info,
			  struct sk_buff *skb)
{
	struct ethhdr *mac_hdr = (struct ethhdr *)skb_mac_header(skb);
	u16 vlan_tci;
	u8 sc;

	if (!__vlan_get_tag(skb, &vlan_tci)) {
		u8 pcp = OPA_VNIC_VLAN_PCP(vlan_tci);

		if (OPA_VNIC_IS_DMAC_MCAST(mac_hdr))
			sc = info->vport.pcp_to_sc_mc[pcp];
		else
			sc = info->vport.pcp_to_sc_uc[pcp];
	} else {
		if (OPA_VNIC_IS_DMAC_MCAST(mac_hdr))
			sc = info->vport.non_vlan_sc_mc;
		else
			sc = info->vport.non_vlan_sc_uc;
	}

	return sc & OPA_VNIC_SC_MASK;
}

/* opa_vnic_get_dlid - return the dlid */
static uint32_t opa_vnic_get_dlid(struct opa_veswport_info *info,
				  struct sk_buff *skb)
{
	struct ethhdr *mac_hdr = (struct ethhdr *)skb_mac_header(skb);
	uint32_t dlid;

	if (OPA_VNIC_IS_DMAC_MCAST(mac_hdr)) {
		/* TODO: use dlid miss rule for now, fix later */
		dlid = info->vesw.u_mcast_dlid;
	} else if (OPA_VNIC_IS_DMAC_LOCAL(mac_hdr)) {
		/* locally administered mac address */
		dlid = ((uint32_t)mac_hdr->h_dest[5] << 16) |
		       ((uint32_t)mac_hdr->h_dest[4] << 8)  |
		       mac_hdr->h_dest[3];
	} else {
		/* globally administered mac address */
		/* TODO: use dlid miss rule for now, fix later */
		dlid = info->vesw.u_ucast_dlid[0];
	}

	return dlid & OPA_VNIC_DLID_MASK;
}

/* opa_vnic_populate_hdr - populate opa header */
static void opa_vnic_populate_hdr(struct opa_veswport_info *info, void *opa_hdr,
				  struct sk_buff *skb, u32 len, u32 lid)
{
	union opa_bypass10_pkt *hdr = opa_hdr;

	/* TODO: minimum header parms for now, update later */
	hdr->slid = lid;
	hdr->dlid = opa_vnic_get_dlid(info, skb);

	BUG_ON(len & 0x7);
	hdr->length = len >> 3;

	hdr->sc = opa_vnic_get_sc(info, skb);

	/* TODO: only 10B is supported now, add 16B support later */
	hdr->l2 = 1;

	hdr->pkey = info->vesw.pkey;
	hdr->l4_hdr = info->vesw.vesw_id;
}

/* opa_vnic_encap_skb - encap skb packet */
void opa_vnic_encap_skb(struct opa_vnic_adapter *adapter, struct sk_buff *skb)
{
	unsigned int pad_len;
	struct opa_vnic_device *vdev = adapter->vdev;
	struct opa_veswport_info *info = &adapter->info;
	void *opa_hdr;

	opa_hdr = skb->data - OPA_VNIC_HDR_LEN;
	memset(opa_hdr, 0, OPA_VNIC_HDR_LEN);

	/* padding for 8 bytes size alignment */
	pad_len = (skb->len + OPA_VNIC_HDR_LEN + OPA_VNIC_ICRC_TAIL_LEN) & 0x7;
	if (pad_len)
		pad_len = 8 - pad_len;

	pad_len += OPA_VNIC_ICRC_TAIL_LEN;

	opa_vnic_populate_hdr(info, opa_hdr, skb,
		      skb->len + OPA_VNIC_HDR_LEN + pad_len, vdev->lid);
	skb_push(skb, OPA_VNIC_HDR_LEN);
}

/* opa_vnic_decap_skb - decap skb packet */
void opa_vnic_decap_skb(struct opa_vnic_adapter *adapter, struct sk_buff *skb)
{
	skb_pull(skb, OPA_VNIC_HDR_LEN);
}
