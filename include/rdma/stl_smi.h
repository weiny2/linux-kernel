/*
 * Copyright (c) 2013 Intel Corporation.  All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#if !defined(STL_SMI_H)
#define STL_SMI_H

#include <rdma/ib_mad.h>
#include <rdma/ib_smi.h>

#define STL_SMP_LID_DATA_SIZE			2016
#define STL_SMP_DR_DATA_SIZE			1872
#define STL_SMP_MAX_PATH_HOPS			64

#define STL_MAX_PVLS				32

#define STL_SMI_CLASS_VERSION			0x81

#define STL_LID_PERMISSIVE			cpu_to_be32(0xFFFFFFFF)

struct stl_smp {
	u8	base_version;
	u8	mgmt_class;
	u8	class_version;
	u8	method;
	__be16	status;
	u8	hop_ptr;
	u8	hop_cnt;
	__be64	tid;
	__be16	attr_id;
	__be16	resv;
	__be32	attr_mod;
	__be64	mkey;
	union {
		struct {
			uint8_t data[STL_SMP_LID_DATA_SIZE];
		} lid;
		struct {
			__be32	dr_slid;
			__be32	dr_dlid;
			u8	initial_path[STL_SMP_MAX_PATH_HOPS];
			u8	return_path[STL_SMP_MAX_PATH_HOPS];
			u8	reserved[8];
			u8	data[STL_SMP_DR_DATA_SIZE];
		} dr;
	} route;
} __attribute__ ((packed));


/* Subnet management attributes */
/* ... */
#define STL_ATTRIB_ID_NODE_DESCRIPTION		cpu_to_be16(0x0010)
#define STL_ATTRIB_ID_NODE_INFO			cpu_to_be16(0x8011)
#define STL_ATTRIB_ID_PORT_INFO			cpu_to_be16(0x9015)
/* ... */

struct stl_node_description {
	u8 data[64];
} __attribute__ ((packed));

struct stl_node_info {
	u8      base_version;
	u8      class_version;
	u8      node_type;
	u8      num_ports;
	__be32  reserved;
	__be64  sys_guid;
	__be64  node_guid;
	__be64  port_guid;
	__be16  partition_cap;
	__be16  device_id;
	__be32  revision;
	u8      local_port_num;
	u8      vendor_id[3];   /* network byte order */
} __attribute__ ((packed));

struct stl_port_info {
	__be64 mkey;
	__be64 subnet_prefix;
	__be32 lid;
	__be32 sm_lid;
	__be32 cap_mask;
	__be16 diag_code;
	__be16 mkey_lease_period;
	struct {
		__be16 supported;
		__be16 enabled;
		__be16 active;
	} link_width;
	struct {
		u8     downdefstate;             /* 4 res, 4 bits */
		u8     portphysstate_portstate;  /* 4 bits, 4 bits */
	} link;
	struct {
		u8 supported;
		u8 enabled;
		u8 active;
	} link_speed;
	u8     local_port_num;
	struct {
		u8     inittype;                /* 4 res, 4 bits */
		u8     cap;                     /* 3 res, 5 bits */
		__be16 high_limit;
		__be16 preempt_limit;
		u8     arb_high_cap;
		u8     arb_low_cap;
		__be32 perempt_mask;
	} vl;
	u8     mkeyprotect_res_lmc;             /* 2 bits, 3, 3 */
	u8     smsl;                            /* 3 res, 5 bits */
	u8     inittypereply_mtucap;            /* 4 bits, 4 bits */
	u8     operationalvl_pei_peo_fpi_fpo;	/* 4 bits, 1, 1, 1, 1 */
	__be16 mkey_violations;
	__be16 pkey_violations;
	__be16 qkey_violations;
	u8     clientrereg_resv_subnetto;	/* 1 bit, 2 bits, 5 */
	u8     resv_resptimevalue;		        /* 3 res, 5 bits */
	u8     localphyerrors_overrunerrors;	/* 4 bits, 4 bits */
	u8     resv;
	__be16 max_credit_hint;
	__be32 link_roundtrip_latency;          /* 8 res, 24 bits */
	__be16 capmask2;
	__be32 capmask3;

	struct {
		u8 vlstall_hoqlife;             /* 3 bits, 5 bits */
	} xmit_q[STL_MAX_PVLS];

	struct {
		u8 pvlx_to_mtu[STL_MAX_PVLS/2];             /* 4 bits, 4 bits */
	} neigh_mtu;
	__be64 neigh_node_guid;
	__be32 neigh_lid;
	__be32 sm_trap_qp;                      /* 8 bits, 24 bits */
	__be32 sa_trap_qp;                      /* 8 bits, 24 bits */
	struct {
		u8 supported;                   /* 4 res, 4 bits */
		u8 enabled_active;              /* 4 bits, 4 bits */
	} port_link_mode;
	struct {
		u8 supported;                   /* 4 res, 4 bits */
		u8 enabled_active;              /* 4 bits, 4 bits */
	} port_ltp_mode;

	__be16 port_mode;
	__be16 packet_format_supported;
	__be16 packet_format_enabled;
	__be16 pkey_8b;
	__be16 pkey_10b;
	struct {
		__be16 supported;
		__be16 enabled;
		__be16 active;
	} link_width_downgrade;

	__be32 numpvls_sup_en_active;           /* 14 res, 6, 6, 6 */
	__be32 buf_unit_vl15_credit_alloc;      /* 12 res, 12, 4, 4 */
	__be32 flow_control_mask;
	u8     neigh_node_type;
	u8     lid_mask_coll_multi;             /* 2 res, 3, 3 */
	struct {
		u8 buffer;
		u8 wire;
	} replay_depth;
	__be16 flit_ctl_minhead_maxpretx_maxprerx; /* 2 res, 4, 5, 5 */
	struct {
		u8 egress_port;
		u8 res_drctl;                  /* 7 res, 1 */
	} pass_through;
} __attribute__ ((packed));

static inline u8
stl_get_smp_direction(struct stl_smp *smp)
{
	return ib_get_smp_direction((struct ib_smp *)smp);
}

static inline uint8_t *stl_get_smp_data(struct stl_smp *smp)
{
	if (smp->mgmt_class == IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE)
		return smp->route.dr.data;
	else
		return smp->route.lid.data;
}

#endif /* IB_SMI_H */
