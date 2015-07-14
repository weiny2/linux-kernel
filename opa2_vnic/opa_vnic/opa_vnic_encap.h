#ifndef _OPA_VNIC_ENCAP_H
#define _OPA_VNIC_ENCAP_H
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
 * This file contains all OPA VNIC declaration required for encapsulation
 * and decapsulation of Ethernet packets.
 */

#include <linux/types.h>

/* TODO: only define fields that are required */

/* EM attribute IDs */
#define OPA_EM_ATTR_CLASS_PORT_INFO        0x0001
#define OPA_EM_ATTR_NOTICE                 0x0002
#define OPA_EM_ATTR_INFORM_INFO            0x0003
#define OPA_EM_ATTR_VESWPORT_INFO          0x0011
#define OPA_EM_ATTR_VESWPORT_DMAC_ENTRIES  0x0012
#define OPA_EM_ATTR_VESWPORT_SMAC_ENTRIES  0x0013
#define OPA_EM_ATTR_IFACE_UCAST_MACS       0x0014
#define OPA_EM_ATTR_IFACE_MCAST_MACS       0x0015

#define OPA_VESW_MAX_NUM_DEF_GW    16
#define OPA_VESW_NUM_ENCAP_PROTO   16
#define OPA_VNIC_MAX_NUM_PCP        8

/* vnic port mode of operation */
enum {
	OPA_VESWPORT_MODE_NOOP,
	OPA_VESWPORT_MODE_NORMAL,
	OPA_VESWPORT_MODE_PROMISCUOUS,
	OPA_VESWPORT_MODE_ALLMULTI,
};

/**
 * struct opa_vnic_state - state of vnic switch or port
 * @state: state
 */
union opa_vnic_state {
	struct {
		__be16 state : 8;
		__be16 rsvd0 : 8;
	};
	__be16 w;
};

/**
 * struct opa_vnic_encap_excpt_rc - exception route control
 * @rc: route control
 */
struct opa_vnic_encap_excpt_rc {
	u8  rc : 3;
	u8  rsvd0 : 5;
};

/**
 * struct opa_vnic_encap_rc - OPA vnic route control information
 * @proto0: protocol 0 - default
 * @proto1: protocol 1 - IPv4
 * @proto2: protocol 2 - IPv4 + UDP
 * @proto3: protocol 3 - IPv4 + TDP
 * @proto4: protocol 4 - IPv6
 * @proto5: protocol 5 - IPv6 + UDP
 * @proto6: protocol 6 - IPv6 + TDP
 */
union opa_vnic_encap_rc {
	struct {
		__be32  proto0 : 3;
		__be32  rsvd0  : 1;
		__be32  proto1 : 3;
		__be32  rsvd1  : 1;
		__be32  proto2 : 3;
		__be32  rsvd2  : 1;
		__be32  proto3 : 3;
		__be32  rsvd3  : 1;
		__be32  proto4 : 3;
		__be32  rsvd4  : 1;
		__be32  proto5 : 3;
		__be32  rsvd5  : 1;
		__be32  proto6 : 3;
		__be32  rsvd6  : 5;
	};
	__be32 dw;
};

/**
 * struct opa_vnic_ngbr_vl_mtu - OPA vnic neighbor MTU per VL
 * @vlx_mtu: neighbor mtu for vl 'x' (where x=0-15)
 */
union opa_vnic_ngbr_vl_mtu {
	struct {
		__be64  vl0_mtu : 4;
		__be64  vl1_mtu : 4;
		__be64  vl2_mtu : 4;
		__be64  vl3_mtu : 4;
		__be64  vl4_mtu : 4;
		__be64  vl5_mtu : 4;
		__be64  vl6_mtu : 4;
		__be64  vl7_mtu : 4;
		__be64  vl8_mtu : 4;
		__be64  vl9_mtu : 4;
		__be64  vl10_mtu : 4;
		__be64  vl11_mtu : 4;
		__be64  vl12_mtu : 4;
		__be64  vl13_mtu : 4;
		__be64  vl14_mtu : 4;
		__be64  vl15_mtu : 4;

	};
	__be64 qw;
};

/**
 * struct opa_vnic_gw_enable - OPA vnic gateway enable
 * @encap_smac_lookup: enable smac lookup
 * @encap_dmac_lookup: enable dmac lookup
 */
/* TODO: Check if smac lookup is needed */
union opa_vnic_gw_enable {
	struct {
		__be16  encap_smac_lookup : 1;
		__be16  encap_dmac_lookup : 1;
		__be16  rsvd0 : 14;

	};
	__be16 w;
};

/**
 * struct opa_vesw_info - OPA vnic switch information
 * @dmac_fabric_id: 10-bit fabric id
 * @vesw_id: 12-bit virtual ethernet switch id
 * @state: virtual ethernet switch state
 * @mcast_mask: mask to check if the lid is mcast
 * @def_gw_aggr: default gw agrregated
 * @def_fw_id_mask: bitmask of default GW ports
 * @encap_format: opa encap format; 0=10B, 1=16B
 * @port_down: port is down if no default GW link is active
 * @pkey: partition key
 * @bcast_mlid: MLID for ethernet broadcast packets
 * @u_mcast_dlid: unknown multicast dlid
 * @u_ucast_did: array of unknown unicast dlids
 * @rc: routing control
 * @encap_excpt_prot: exception protocol match array
 * @excpt_hndlr_lid: LID of exception packet handler
 * @excpt_rc: routing control for exception packet
 * @eth_mtu: MTUs for each vlan PCP
 * @eth_mtu_non_vlan: MTU for non vlan packets
 */
struct opa_vesw_info {
	__be16  dmac_fabric_id;
	__be16  vesw_id;

	union opa_vnic_state state;
	u8      rsvd1;
	u8      mcast_mask;

	u8      def_gw_aggr;
	u8      rsvd2;
	__be16  def_fw_id_mask;

	u8      encap_format;
	u8      port_down;
	__be16  pkey;

	__be32  bcast_mlid;
	__be32  u_mcast_dlid;
	__be32  u_ucast_dlid[OPA_VESW_MAX_NUM_DEF_GW];

	union opa_vnic_encap_rc  rc;

	__be16  encap_excpt_prot[OPA_VESW_NUM_ENCAP_PROTO];

	__be32  excpt_hndlr_lid;

	struct opa_vnic_encap_excpt_rc excpt_rc;
	u8      rsvd3[3];

	__be16  eth_mtu[OPA_VNIC_MAX_NUM_PCP];

	__be16  eth_mtu_non_vlan;
	u8      rsvd4[2];
} __packed;

/**
 * struct opa_per_veswport_info - OPA vnic per port information
 * @port_num: port number
 * @config_state: configured port state
 * @oper_state: operational port state
 * @is_def_gw: is a default GW link
 * @def_gw_id_mask: accepted default GW number bitmask
 * @num_dmac_tbl_ent: num dmac table entries
 * @num_smac_tbl_ent: num smac table entries
 * @encap_slid: base slid for the port
 * @pcp_to_sc_uc: sc by pcp index for unicast ethernet packets
 * @pcp_to_vl_uc: vl by pcp index for unicast ethernet packets
 * @pcp_to_sc_mc: sc by pcp index for multicast ethernet packets
 * @pcp_to_vl_mc: vl by pcp index for multicast ethernet packets
 * @non_vlan_sc_uc: sc for non-vlan unicast ethernet packets
 * @non_vlan_vlt_uc: vlt for non-vlan unicast ethernet packets
 * @non_vlan_sc_mc: sc for non-vlan multicast ethernet packets
 * @non_vlan_vlt_mc: vlt for non-vlan multicast ethernet packets
 * @excpt_uc: sc for ethernet exception packets
 * @excpt_vlt: vlt for ethernet exception packets
 * @ngbr_mtu: neighbor mtu by vl
 * @gw_en: enable flags
 */
struct opa_per_veswport_info {
	__be32  port_num;

	union opa_vnic_state config_state;
	union opa_vnic_state oper_state;

	u8      rsvd1;
	u8      is_def_gw;
	__be16  def_gw_id_mask;

	__be16  num_dmac_tbl_ent;
	__be16  num_smac_tbl_ent;

	__be32  encap_slid;

	u8      pcp_to_sc_uc[OPA_VNIC_MAX_NUM_PCP];
	u8      pcp_to_vl_uc[OPA_VNIC_MAX_NUM_PCP];
	u8      pcp_to_sc_mc[OPA_VNIC_MAX_NUM_PCP];
	u8      pcp_to_vl_mc[OPA_VNIC_MAX_NUM_PCP];

	u8      non_vlan_sc_uc;
	u8      non_vlan_vlt_uc;
	u8      non_vlan_sc_mc;
	u8      non_vlan_vlt_mc;

	u8      excpt_uc;
	u8      excpt_vlt;
	u8      rsvd2[2];

	__be32  rsvd3;

	union opa_vnic_ngbr_vl_mtu  ngbr_mtu;
	union opa_vnic_gw_enable    gw_en;
	__be16  rsvd4;
} __packed;

/**
 * struct opa_veswport_info - OPA vnic port information
 * @vesw: OPA vnic switch information
 * @vport: OPA vnic per port information
 *
 *   On the host, each port belongs to a different virtual
 *   ethernet switch
 */
struct opa_veswport_info {
	struct opa_vesw_info            vesw;
	struct opa_per_veswport_info    vport;
};

#endif /* _OPA_VNIC_ENCAP_H */
