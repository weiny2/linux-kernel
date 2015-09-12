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

/*
 * Trap opcodes sent from VNIC
 */

#define OPA_VESWPORT_TRAP_NOOP 0
#define OPA_VESWPORT_TRAP_IFACE_UCAST_MAC_CHANGE 0x1
#define OPA_VESWPORT_TRAP_IFACE_MCAST_MAC_CHANGE 0x2
#define OPA_VESWPORT_TRAP_ETH_LINK_STATUS_CHANGE 0x3

/* vnic port mode of operation */
enum {
	OPA_VESWPORT_MODE_NOOP,
	OPA_VESWPORT_MODE_NORMAL,
	OPA_VESWPORT_MODE_PROMISCUOUS,
	OPA_VESWPORT_MODE_ALLMULTI,
};

/**
 * struct opa_vnic_state - state of vnic switch or port
 * @state : state
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
 * @rc : route control
 */
struct opa_vnic_encap_excpt_rc {
	u8  rc : 3;
	u8  rsvd0 : 5;
};

/**
 * union opa_vnic_encap_rc - OPA vnic route control information
 * @proto0 : protocol 0 - default
 * @proto1 : protocol 1 - IPv4
 * @proto2 : protocol 2 - IPv4 + UDP
 * @proto3 : protocol 3 - IPv4 + TDP
 * @proto4 : protocol 4 - IPv6
 * @proto5 : protocol 5 - IPv6 + UDP
 * @proto6 : protocol 6 - IPv6 + TDP
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
 * union opa_vnic_ngbr_vl_mtu - OPA vnic neighbor MTU per VL
 * @vl0_mtu : neighbor mtu for vl0
 * @vl1_mtu : neighbor mtu for vl1
 * @vl2_mtu : neighbor mtu for vl2
 * @vl3_mtu : neighbor mtu for vl3
 * @vl4_mtu : neighbor mtu for vl4
 * @vl5_mtu : neighbor mtu for vl5
 * @vl6_mtu : neighbor mtu for vl6
 * @vl7_mtu : neighbor mtu for vl7
 * @vl8_mtu : neighbor mtu for vl8
 * @vl9_mtu : neighbor mtu for vl9
 * @vl10_mtu : neighbor mtu for vl10
 * @vl11_mtu : neighbor mtu for vl11
 * @vl12_mtu : neighbor mtu for vl12
 * @vl13_mtu : neighbor mtu for vl13
 * @vl14_mtu : neighbor mtu for vl14
 * @vl15_mtu : neighbor mtu for vl15
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
 * union opa_vnic_gw_enable - OPA vnic gateway enable
 * @encap_smac_lookup : enable smac lookup
 * @encap_dmac_lookup : enable dmac lookup
 */
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
 * @fabric_id : 10-bit fabric id
 * @vesw_id : 12-bit virtual ethernet switch id
 * @state : virtual ethernet switch state
 * @mcast_mask : mask to check if the lid is mcast
 * @def_gw_aggr : default gw agrregated
 * @def_fw_id_mask : bitmask of default GW ports
 * @encap_format : opa encap format; 0=10B, 1=16B
 * @port_down : port is down if no default GW link is active
 * @pkey : partition key
 * @u_mcast_dlid : unknown multicast dlid
 * @u_ucast_did : array of unknown unicast dlids
 * @rc : routing control
 * @encap_excpt_prot : exception protocol match array
 * @excpt_hndlr_lid : LID of exception packet handler
 * @excpt_rc : routing control for exception packet
 * @eth_mtu : MTUs for each vlan PCP
 * @eth_mtu_non_vlan : MTU for non vlan packets
 */
/* TODO: only define fields that are required */
struct opa_vesw_info {
	__be16  fabric_id;
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

	__be32  rsvd3;
	__be32  u_mcast_dlid;
	__be32  u_ucast_dlid[OPA_VESW_MAX_NUM_DEF_GW];

	union opa_vnic_encap_rc  rc;

	__be16  encap_excpt_prot[OPA_VESW_NUM_ENCAP_PROTO];

	__be32  excpt_hndlr_lid;

	struct opa_vnic_encap_excpt_rc excpt_rc;
	u8      rsvd4[3];

	__be16  eth_mtu[OPA_VNIC_MAX_NUM_PCP];

	__be16  eth_mtu_non_vlan;
	u8      rsvd5[2];
} __packed;

/**
 * struct opa_per_veswport_info - OPA vnic per port information
 * @port_num: port number
 * @is_def_gw : is a default GW link
 * @def_gw_id_mask : accepted default GW number bitmask
 * @base_mac_addr : base mac address
 * @config_state : configured port state
 * @oper_state : operational port state
 * @max_mac_tbl_ent : max number of mac table entries
 * @max_smac_ent : max smac entries in mac table
 * @encap_slid : base slid for the port
 * @pcp_to_sc_uc : sc by pcp index for unicast ethernet packets
 * @pcp_to_vl_uc : vl by pcp index for unicast ethernet packets
 * @pcp_to_sc_mc : sc by pcp index for multicast ethernet packets
 * @pcp_to_vl_mc : vl by pcp index for multicast ethernet packets
 * @non_vlan_sc_uc : sc for non-vlan unicast ethernet packets
 * @non_vlan_vl_uc : vl for non-vlan unicast ethernet packets
 * @non_vlan_sc_mc : sc for non-vlan multicast ethernet packets
 * @non_vlan_vl_mc : vl for non-vlan multicast ethernet packets
 * @excpt_sc : sc for ethernet exception packets
 * @excpt_vl : vl for ethernet exception packets
 * @ngbr_mtu : neighbor mtu by vl
 * @gw_en : enable flags
 * @gw_flags : gateway flags
 */
/* TODO: only define fields that are required */
struct opa_per_veswport_info {
	__be32  port_num;

	u8      rsvd1;
	u8      is_def_gw;
	__be16  def_gw_id_mask;

	u8      base_mac_addr[ETH_ALEN];
	u8      config_state;
	u8      oper_state;

	__be16  max_mac_tbl_ent;
	__be16  max_smac_ent;

	__be32  encap_slid;

	u8      pcp_to_sc_uc[OPA_VNIC_MAX_NUM_PCP];
	u8      pcp_to_vl_uc[OPA_VNIC_MAX_NUM_PCP];
	u8      pcp_to_sc_mc[OPA_VNIC_MAX_NUM_PCP];
	u8      pcp_to_vl_mc[OPA_VNIC_MAX_NUM_PCP];

	u8      non_vlan_sc_uc;
	u8      non_vlan_vl_uc;
	u8      non_vlan_sc_mc;
	u8      non_vlan_vl_mc;

	u8      excpt_sc;
	u8      excpt_vl;
	u8      rsvd2[2];

	union opa_vnic_ngbr_vl_mtu  ngbr_mtu;

	__be32  rsvd3;

	union opa_vnic_gw_enable    gw_en;
	__be16  gw_flags;
} __packed;

/**
 * struct opa_veswport_info - OPA vnic port information
 *   On host, each of the virtual ethernet ports belongs
 *   to a different virtual ethernet switches.
 * @vesw : OPA vnic switch information
 * @vport : OPA vnic per port information
 */
struct opa_veswport_info {
	struct opa_vesw_info            vesw;
	struct opa_per_veswport_info    vport;
};

/**
 * union opa_dlid_sd - dlid and side data needed.
 * @dlid : Destination lid corresponding to MAC addr
 * @sd_excpt_dlid : 1 = use exception DLID
 * @sd_excpt_dlid_prot_match :  1 = use except dlid if prot match
 * @sd_is_src_mac : 1 = entry is SMAC, 0 = not SMAC
 *
 */
union opa_dlid_sd {
	struct {
		__be32  dlid                      : 24;
		__be32  sd_excpt_dlid             : 1;
		__be32  sd_excpt_dlid_prot_match  : 1;
		__be32  sd_is_src_mac             : 1;
		__be32  sd_reserved               : 5;
	};
	__be32 dw;
};

/**
 * struct opa_veswport_mactable_entry:  single entry in the forwarding table
 * @mac_addr : MAC address
 * @mac_addr_mask : MAC address bit mask
 * @dlid_sd  : Matching DLID and side data
 *
 * On the host each virtual ethernet port will have
 * a forwarding table. These tables are used to
 * map a MAC to a LID and other data. For more
 * details see struct opa_veswport_mactable_entries.
 * This is the structure of a single mactable entry
 *
 */

struct opa_veswport_mactable_entry {
	u8			mac_addr[ETH_ALEN];
	u8			mac_addr_mask[ETH_ALEN];
	union   opa_dlid_sd	dlid_sd;
} __packed;

/**
 * struct opa_veswport_mactable : Forwarding table array
 * @offset : Mactable starting offset
 * @num_entries : Number of entries to get or set
 * @tbl_entries[] : Array of table entries
 *
 * The EM sends down this structure in a MAD indicating
 * the starting offset in the forwarding table that this
 * entry is to be loaded into and the number of entries
 * that that this MAD instance contains
 */

struct opa_veswport_mactable {
	__be16					offset;
	__be16					num_entries;
	struct opa_veswport_mactable_entry	tbl_entries[0];
} __packed;

/**
 * struct opa_stl_veswport_trap - Trap message sent to EM by VNIC
 * @fabric_id :  10 bit fabric id
 * @veswid    :  12 bit virtual ethernet switch id
 * @veswportnum : logical port number on the Virtual switch
 * @opaportnum :  physical port num (redundant on host)
 * @veswportindex: switch port index on hfi port 0 based
 * @opcode : operation
 * @reserved : 32 bit for alignment
 *
 * The VNIC will send trap messages to the Ethernet manager to
 * inform it about changes to the VNIC config, behaviour etc.
 * This is the format of the trap payload.
 *
 */

struct opa_stl_veswport_trap {
	__be16	fabric_id;
	__be16	veswid;
	__be32	vewsportnum;
	__be16	opaportnum;
	u8	veswportindex;
	u8	opcode;
	__be32	reserved;
} __packed;

/**
 * struct opa_hfi_veswport_iface_macs - Msg to set globally administered MAC
 * @start_idx : position of first entry (0 based)
 * @num_macs_in_msg: number of MACs in this message
 * @tot_macs_in_lst : The total number of MACs the agent has
 * @gen_count :  gen_count to indicate change
 * @mac_addrs : The mac address array = 6 * num_macs_in_msg bytes
 *
 * Same attribute IDS and attribute modifiers as in locally administered
 * addresses used to set globally administered addresses
 *
 */

struct opa_hfi_veswport_iface_macs {
	__be16	start_idx;
	__be16	num_macs_in_msg;
	__be16  gen_count;
	__be16	tot_macs_in_lst;
	u8	mac_addrs[0];
} __packed;


#endif /* _OPA_VNIC_ENCAP_H */
