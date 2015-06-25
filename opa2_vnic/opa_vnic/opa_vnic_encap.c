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

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/module.h>
#include <linux/opa_vnic.h>

#include "opa_vnic_internal.h"

/*
 * NOTE: OPA header (L2+L4) is only 12 bytes.
 * This structure has overlay over ethernet packet data.
 */
union bypass10_pkt {
	struct {
	struct {
		uint64_t        slid    : 20;
		uint64_t        length  : 11;
		uint64_t        b       : 1;
		uint64_t        dlid    : 20;
		uint64_t        sc      : 5;
		uint64_t        rc      : 3;
		uint64_t        f       : 1;
		uint64_t        l2      : 2;  /* 1=10B, 2=16B */
		uint64_t        lt      : 1;
	};
	struct {
		uint64_t        l4      : 4;
		uint64_t        pkey    : 4;
		uint64_t        entropy : 8;
		uint64_t        l4_hdr  : 16;
		uint64_t        payload : 32;
	};
	} __attribute__ ((__packed__));
	uint64_t val[2];
};

/* vnic_insert_hdr - insert opa header */
static void vnic_insert_hdr(void *data, u32 len, u8 vswt_id)
{
	union bypass10_pkt *hdr = data;

	/* TODO: dummy header parms for now, fix later */
	hdr->slid = 0;
	hdr->dlid = 2;
	hdr->length = len >> 3;
	if (len & 0x7)
		hdr->length += 1;

	hdr->l2 = 1; /* 10B */
	hdr->l4_hdr = vswt_id;
}

/* opa_vnic_encap_skb - encap skb packet */
void opa_vnic_encap_skb(struct opa_vnic_adapter *adapter, struct sk_buff *skb)
{
	unsigned int pad_len;
	struct opa_vnic_device *vdev = adapter->vdev;

	skb_push(skb, OPA_VNIC_HDR_LEN);
	memset(skb->data, 0, OPA_VNIC_HDR_LEN);

	/* padding for 8 bytes size alignment and additional 8 bytes for ICRC */
	/* TODO: Only do required minimum padding */
	pad_len = skb->len & 0x7;
	if (pad_len)
		pad_len = 8 - pad_len;

	pad_len += 8;

	vnic_insert_hdr(skb->data, skb->len + pad_len, vdev->id);
}

/* opa_vnic_decap_skb - decap skb packet */
void opa_vnic_decap_skb(struct opa_vnic_adapter *adapter, struct sk_buff *skb)
{
	skb_pull(skb, OPA_VNIC_HDR_LEN);
}
