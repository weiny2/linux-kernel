/*
 * Copyright (c) 2012 Intel Corporation.  All rights reserved.
 * Copyright (c) 2006 - 2012 QLogic Corporation. All rights reserved.
 * Copyright (c) 2005, 2006 PathScale, Inc. All rights reserved.
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

#include <rdma/ib_smi.h>
#include <rdma/stl_smi.h>
#define STL_NUM_PKEY_BLOCKS_PER_SMP (STL_SMP_DR_DATA_SIZE \
			/ (STL_PARTITION_TABLE_BLK_SIZE * sizeof(u16)))

#include "hfi.h"
#include "mad.h"
#include "trace.h"

#define STL_LINK_WIDTH_ALL_SUPPORTED \
		(STL_LINK_WIDTH_1X | STL_LINK_WIDTH_2X \
		| STL_LINK_WIDTH_3X | STL_LINK_WIDTH_4X)

static int reply(void *arg)
{
	/* XXX change all callers so that they all pass a
	 * struct ib_mad_hdr * */
	struct ib_mad_hdr *smp = (struct ib_mad_hdr *)arg;
	/*
	 * The verbs framework will handle the directed/LID route
	 * packet changes.
	 */
	smp->method = IB_MGMT_METHOD_GET_RESP;
	if (smp->mgmt_class == IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE)
		smp->status |= IB_SMP_DIRECTION;
	return IB_MAD_RESULT_SUCCESS | IB_MAD_RESULT_REPLY;
}

static inline void clear_stl_smp_data(struct stl_smp *smp)
{
	void *data = stl_get_smp_data(smp);
	size_t size = stl_get_smp_data_size(smp);
	memset(data, 0, size);
}

static void qib_send_trap(struct qib_ibport *ibp, void *data, unsigned len)
{
	struct ib_mad_send_buf *send_buf;
	struct ib_mad_agent *agent;
	struct ib_smp *smp;
	int ret;
	unsigned long flags;
	unsigned long timeout;

	agent = ibp->send_agent;
	if (!agent)
		return;

	/* o14-3.2.1 */
	if (ppd_from_ibp(ibp)->lstate != IB_PORT_ACTIVE)
		return;

	/* o14-2 */
	if (ibp->trap_timeout && time_before(jiffies, ibp->trap_timeout))
		return;

	send_buf = ib_create_send_mad(agent, 0, 0, 0, IB_MGMT_MAD_HDR,
				      IB_MGMT_MAD_DATA, GFP_ATOMIC);
	if (IS_ERR(send_buf))
		return;

	smp = send_buf->mad;
	smp->base_version = IB_MGMT_BASE_VERSION;
	smp->mgmt_class = IB_MGMT_CLASS_SUBN_LID_ROUTED;
	smp->class_version = 1;
	smp->method = IB_MGMT_METHOD_TRAP;
	ibp->tid++;
	smp->tid = cpu_to_be64(ibp->tid);
	smp->attr_id = IB_SMP_ATTR_NOTICE;
	/* o14-1: smp->mkey = 0; */
	memcpy(smp->data, data, len);

	spin_lock_irqsave(&ibp->lock, flags);
	if (!ibp->sm_ah) {
		if (ibp->sm_lid != be16_to_cpu(IB_LID_PERMISSIVE)) {
			struct ib_ah *ah;

			ah = qib_create_qp0_ah(ibp, ibp->sm_lid);
			if (IS_ERR(ah))
				ret = PTR_ERR(ah);
			else {
				send_buf->ah = ah;
				ibp->sm_ah = to_iah(ah);
				ret = 0;
			}
		} else
			ret = -EINVAL;
	} else {
		send_buf->ah = &ibp->sm_ah->ibah;
		ret = 0;
	}
	spin_unlock_irqrestore(&ibp->lock, flags);

	if (!ret)
		ret = ib_post_send_mad(send_buf, NULL);
	if (!ret) {
		/* 4.096 usec. */
		timeout = (4096 * (1UL << ibp->subnet_timeout)) / 1000;
		ibp->trap_timeout = jiffies + usecs_to_jiffies(timeout);
	} else {
		ib_free_send_mad(send_buf);
		ibp->trap_timeout = 0;
	}
}

/*
 * Send a bad [PQ]_Key trap (ch. 14.3.8).
 */
void qib_bad_pqkey(struct qib_ibport *ibp, __be16 trap_num, u32 key, u32 sl,
		   u32 qp1, u32 qp2, __be16 lid1, __be16 lid2)
{
	struct ib_mad_notice_attr data;

	if (trap_num == IB_NOTICE_TRAP_BAD_PKEY)
		ibp->pkey_violations++;
	else
		ibp->qkey_violations++;
	ibp->n_pkt_drops++;

	/* Send violation trap */
	data.generic_type = IB_NOTICE_TYPE_SECURITY;
	data.prod_type_msb = 0;
	data.prod_type_lsb = IB_NOTICE_PROD_CA;
	data.trap_num = trap_num;
	data.issuer_lid = cpu_to_be16(ppd_from_ibp(ibp)->lid);
	data.toggle_count = 0;
	memset(&data.details, 0, sizeof data.details);
	data.details.ntc_257_258.lid1 = lid1;
	data.details.ntc_257_258.lid2 = lid2;
	data.details.ntc_257_258.key = cpu_to_be32(key);
	data.details.ntc_257_258.sl_qp1 = cpu_to_be32((sl << 28) | qp1);
	data.details.ntc_257_258.qp2 = cpu_to_be32(qp2);

	qib_send_trap(ibp, &data, sizeof data);
}

/*
 * Send a bad M_Key trap (ch. 14.3.9).
 */
static void qib_bad_mkey(struct qib_ibport *ibp, struct ib_mad_hdr *mad,
			 u64 mkey, u32 dr_slid, u8 return_path[], u8 hop_cnt)
{
	struct ib_mad_notice_attr data;

	/* Send violation trap */
	data.generic_type = IB_NOTICE_TYPE_SECURITY;
	data.prod_type_msb = 0;
	data.prod_type_lsb = IB_NOTICE_PROD_CA;
	data.trap_num = IB_NOTICE_TRAP_BAD_MKEY;
	data.issuer_lid = cpu_to_be16(ppd_from_ibp(ibp)->lid);
	data.toggle_count = 0;
	memset(&data.details, 0, sizeof data.details);
	data.details.ntc_256.lid = data.issuer_lid;
	data.details.ntc_256.method = mad->method;
	data.details.ntc_256.attr_id = mad->attr_id;
	data.details.ntc_256.attr_mod = mad->attr_mod;
	data.details.ntc_256.mkey = mkey;
	if (mad->mgmt_class == IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE) {

		data.details.ntc_256.dr_slid = dr_slid;
		data.details.ntc_256.dr_trunc_hop = IB_NOTICE_TRAP_DR_NOTICE;
		if (hop_cnt > ARRAY_SIZE(data.details.ntc_256.dr_rtn_path)) {
			data.details.ntc_256.dr_trunc_hop |=
				IB_NOTICE_TRAP_DR_TRUNC;
			hop_cnt = ARRAY_SIZE(data.details.ntc_256.dr_rtn_path);
		}
		data.details.ntc_256.dr_trunc_hop |= hop_cnt;
		memcpy(data.details.ntc_256.dr_rtn_path, return_path,
		       hop_cnt);
	}

	qib_send_trap(ibp, &data, sizeof data);
}

/*
 * Send a Port Capability Mask Changed trap (ch. 14.3.11).
 */
void qib_cap_mask_chg(struct qib_ibport *ibp)
{
	struct ib_mad_notice_attr data;

	data.generic_type = IB_NOTICE_TYPE_INFO;
	data.prod_type_msb = 0;
	data.prod_type_lsb = IB_NOTICE_PROD_CA;
	data.trap_num = IB_NOTICE_TRAP_CAP_MASK_CHG;
	data.issuer_lid = cpu_to_be16(ppd_from_ibp(ibp)->lid);
	data.toggle_count = 0;
	memset(&data.details, 0, sizeof data.details);
	data.details.ntc_144.lid = data.issuer_lid;
	data.details.ntc_144.new_cap_mask = cpu_to_be32(ibp->port_cap_flags);

	qib_send_trap(ibp, &data, sizeof data);
}

/*
 * Send a System Image GUID Changed trap (ch. 14.3.12).
 */
void qib_sys_guid_chg(struct qib_ibport *ibp)
{
	struct ib_mad_notice_attr data;

	data.generic_type = IB_NOTICE_TYPE_INFO;
	data.prod_type_msb = 0;
	data.prod_type_lsb = IB_NOTICE_PROD_CA;
	data.trap_num = IB_NOTICE_TRAP_SYS_GUID_CHG;
	data.issuer_lid = cpu_to_be16(ppd_from_ibp(ibp)->lid);
	data.toggle_count = 0;
	memset(&data.details, 0, sizeof data.details);
	data.details.ntc_145.lid = data.issuer_lid;
	data.details.ntc_145.new_sys_guid = ib_qib_sys_image_guid;

	qib_send_trap(ibp, &data, sizeof data);
}

/*
 * Send a Node Description Changed trap (ch. 14.3.13).
 */
void qib_node_desc_chg(struct qib_ibport *ibp)
{
	struct ib_mad_notice_attr data;

	data.generic_type = IB_NOTICE_TYPE_INFO;
	data.prod_type_msb = 0;
	data.prod_type_lsb = IB_NOTICE_PROD_CA;
	data.trap_num = IB_NOTICE_TRAP_CAP_MASK_CHG;
	data.issuer_lid = cpu_to_be16(ppd_from_ibp(ibp)->lid);
	data.toggle_count = 0;
	memset(&data.details, 0, sizeof data.details);
	data.details.ntc_144.lid = data.issuer_lid;
	data.details.ntc_144.local_changes = 1;
	data.details.ntc_144.change_flags = IB_NOTICE_TRAP_NODE_DESC_CHG;

	qib_send_trap(ibp, &data, sizeof data);
}

static int __subn_get_stl_nodedesc(struct stl_smp *smp, u32 am,
				   u8 *data, struct ib_device *ibdev,
				   u8 port, u32 *resp_len)
{
	struct stl_node_description *nd;

	clear_stl_smp_data(smp);

	if (am) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}
 
	nd = (struct stl_node_description *)data;
 
	memcpy(nd->data, ibdev->node_desc, sizeof(nd->data));

	if (resp_len)
		*resp_len += sizeof(*nd);
 
	return reply(smp);
}

static int __subn_get_stl_nodeinfo(struct stl_smp *smp, u32 am, u8 *data,
				   struct ib_device *ibdev, u8 port,
				   u32 *resp_len)
{
	struct stl_node_info *ni;
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	u32 vendor, majrev, minrev;
	unsigned pidx = port - 1; /* IB number port from 1, hdw from 0 */
	ni = (struct stl_node_info *)data;

	clear_stl_smp_data(smp);

	/* GUID 0 is illegal */
	if (am || pidx >= dd->num_pports || dd->pport[pidx].guid == 0) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}

	ni->port_guid = dd->pport[pidx].guid;
	ni->base_version = JUMBO_MGMT_BASE_VERSION;
	ni->class_version = STL_SMI_CLASS_VERSION;
	ni->node_type = 1;     /* channel adapter */
	ni->num_ports = ibdev->phys_port_cnt;
	/* This is already in network order */
	ni->system_image_guid = ib_qib_sys_image_guid;
	ni->node_guid = dd->pport->guid; /* Use first-port GUID as node */
	ni->partition_cap = cpu_to_be16(qib_get_npkeys(dd));
	ni->device_id = cpu_to_be16(dd->deviceid);
	majrev = dd->majrev;
	minrev = dd->minrev;
	ni->revision = cpu_to_be32((majrev << 16) | minrev);
	ni->local_port_num = port;
	vendor = dd->vendorid;
	ni->vendor_id[0] = WFR_SRC_OUI_1;
	ni->vendor_id[1] = WFR_SRC_OUI_2;
	ni->vendor_id[2] = WFR_SRC_OUI_3;

	if (resp_len)
		*resp_len += sizeof(*ni);

	return reply(smp);
}

static int subn_get_nodeinfo(struct ib_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	struct ib_node_info *nip = (struct ib_node_info *)&smp->data;
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	u32 vendor, majrev, minrev;
	unsigned pidx = port - 1; /* IB number port from 1, hdw from 0 */

	/* GUID 0 is illegal */
	if (smp->attr_mod || pidx >= dd->num_pports ||
	    dd->pport[pidx].guid == 0)
		smp->status |= IB_SMP_INVALID_FIELD;
	else
		nip->port_guid = dd->pport[pidx].guid;

	nip->base_version = JUMBO_MGMT_BASE_VERSION;
	nip->class_version = STL_SMI_CLASS_VERSION;
	nip->node_type = 1;     /* channel adapter */
	nip->num_ports = ibdev->phys_port_cnt;
	/* This is already in network order */
	nip->sys_guid = ib_qib_sys_image_guid;
	nip->node_guid = dd->pport->guid; /* Use first-port GUID as node */
	nip->partition_cap = cpu_to_be16(qib_get_npkeys(dd));
	nip->device_id = cpu_to_be16(dd->deviceid);
	majrev = dd->majrev;
	minrev = dd->minrev;
	nip->revision = cpu_to_be32((majrev << 16) | minrev);
	nip->local_port_num = port;
	vendor = dd->vendorid;
	nip->vendor_id[0] = QIB_SRC_OUI_1;
	nip->vendor_id[1] = QIB_SRC_OUI_2;
	nip->vendor_id[2] = QIB_SRC_OUI_3;

	return reply(smp);
}

static void set_link_width_enabled(struct qib_pportdata *ppd, u32 w)
{
	(void) ppd->dd->f_set_ib_cfg(ppd, QIB_IB_CFG_LWID_ENB, w);
}

static void set_link_width_downgrade_enabled(struct qib_pportdata *ppd, u32 w)
{
	(void) ppd->dd->f_set_ib_cfg(ppd, QIB_IB_CFG_LWID_DG_ENB, w);
}

static void set_link_speed_enabled(struct qib_pportdata *ppd, u32 s)
{
	(void) ppd->dd->f_set_ib_cfg(ppd, QIB_IB_CFG_SPD_ENB, s);
}

static int get_overrunthreshold(struct qib_pportdata *ppd)
{
	return ppd->dd->f_get_ib_cfg(ppd, QIB_IB_CFG_OVERRUN_THRESH);
}

/**
 * set_overrunthreshold - set the overrun threshold
 * @ppd: the physical port data
 * @n: the new threshold
 *
 * Note that this will only take effect when the link state changes.
 */
static int set_overrunthreshold(struct qib_pportdata *ppd, unsigned n)
{
	(void) ppd->dd->f_set_ib_cfg(ppd, QIB_IB_CFG_OVERRUN_THRESH,
					 (u32)n);
	return 0;
}

static int get_phyerrthreshold(struct qib_pportdata *ppd)
{
	return ppd->dd->f_get_ib_cfg(ppd, QIB_IB_CFG_PHYERR_THRESH);
}

/**
 * set_phyerrthreshold - set the physical error threshold
 * @ppd: the physical port data
 * @n: the new threshold
 *
 * Note that this will only take effect when the link state changes.
 */
static int set_phyerrthreshold(struct qib_pportdata *ppd, unsigned n)
{
	(void) ppd->dd->f_set_ib_cfg(ppd, QIB_IB_CFG_PHYERR_THRESH,
					 (u32)n);
	return 0;
}

/**
 * get_linkdowndefaultstate - get the default linkdown state
 * @ppd: the physical port data
 *
 * Returns zero if the default is POLL, 1 if the default is SLEEP.
 */
static int get_linkdowndefaultstate(struct qib_pportdata *ppd)
{
	return ppd->dd->f_get_ib_cfg(ppd, QIB_IB_CFG_LINKDEFAULT) ==
		HLS_DN_SLEEP;
}

static int check_mkey(struct qib_ibport *ibp, struct ib_mad_hdr *mad,
		      int mad_flags, u64 mkey, u32 dr_slid,
		      u8 return_path[], u8 hop_cnt)
{
	int valid_mkey = 0;
	int ret = 0;

	/* Is the mkey in the process of expiring? */
	if (ibp->mkey_lease_timeout &&
	    time_after_eq(jiffies, ibp->mkey_lease_timeout)) {
		/* Clear timeout and mkey protection field. */
		ibp->mkey_lease_timeout = 0;
		ibp->mkeyprot = 0;
	}

	if ((mad_flags & IB_MAD_IGNORE_MKEY) ||  ibp->mkey == 0 ||
	    ibp->mkey == mkey)
		valid_mkey = 1;

	/* Unset lease timeout on any valid Get/Set/TrapRepress */
	if (valid_mkey && ibp->mkey_lease_timeout &&
	    (mad->method == IB_MGMT_METHOD_GET ||
	     mad->method == IB_MGMT_METHOD_SET ||
	     mad->method == IB_MGMT_METHOD_TRAP_REPRESS))
		ibp->mkey_lease_timeout = 0;

	if (!valid_mkey) {
		switch (mad->method) {
		case IB_MGMT_METHOD_GET:
			/* Bad mkey not a violation below level 2 */
			if (ibp->mkeyprot < 2)
				break;
		case IB_MGMT_METHOD_SET:
		case IB_MGMT_METHOD_TRAP_REPRESS:
			if (ibp->mkey_violations != 0xFFFF)
				++ibp->mkey_violations;
			if (!ibp->mkey_lease_timeout && ibp->mkey_lease_period)
				ibp->mkey_lease_timeout = jiffies +
					ibp->mkey_lease_period * HZ;
			/* Generate a trap notice. */
			qib_bad_mkey(ibp, mad, mkey, dr_slid, return_path,
				     hop_cnt);
			ret = 1;
		}
	}

	return ret;
}

static int __subn_get_stl_portinfo(struct stl_smp *smp, u32 am, u8 *data,
				   struct ib_device *ibdev, u8 port,
				   u32 *resp_len)
{
	int i;
	struct hfi_devdata *dd;
	struct qib_pportdata *ppd;
	struct qib_ibport *ibp;
	struct stl_port_info *pi = (struct stl_port_info *)data;
	u8 mtu;
	u8 credit_rate;
	int ret;
	u32 state;
	u32 port_num = STL_AM_PORTNUM(am);
	u32 num_ports = STL_AM_NPORT(am);
	u32 start_of_sm_config = STL_AM_START_SM_CONF(am);
	u32 buffer_units;
	u64 tmp;

	/* Clear all fields.  Only set the non-zero fields. */
	clear_stl_smp_data(smp);

	if (num_ports != 1) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}
	if (port_num == 0)
		port_num = port;
	else {
		if (port_num > ibdev->phys_port_cnt) {
			smp->status |= IB_SMP_INVALID_FIELD;
			return reply(smp);
		}
		if (port_num != port) {
			ibp = to_iport(ibdev, port_num);
			ret = check_mkey(ibp, (struct ib_mad_hdr *)smp,
					 0, smp->mkey,
					 smp->route.dr.dr_slid,
					 smp->route.dr.return_path,
					 smp->hop_cnt);
			if (ret) {
				return IB_MAD_RESULT_FAILURE;
			}
		}
	}

	dd = dd_from_ibdev(ibdev);
	/* IB numbers ports from 1, hdw from 0 */
	ppd = dd->pport + (port_num - 1);
	ibp = &ppd->ibport_data;

	pi->lid = cpu_to_be32(ppd->lid);

	/* Only return the mkey if the protection field allows it. */
	if (!(smp->method == IB_MGMT_METHOD_GET &&
	      ibp->mkey != smp->mkey &&
	      ibp->mkeyprot == 1))
		pi->mkey = ibp->mkey;

	pi->subnet_prefix = ibp->gid_prefix;
	pi->sm_lid = cpu_to_be32(ibp->sm_lid);
	pi->ib_cap_mask = cpu_to_be32(ibp->port_cap_flags);
	pi->mkey_lease_period = cpu_to_be16(ibp->mkey_lease_period);

	pi->link_width.enabled = cpu_to_be16(ppd->link_width_enabled);
	pi->link_width.supported = cpu_to_be16(ppd->link_width_supported);
	pi->link_width.active = cpu_to_be16(ppd->link_width_active);

	pi->link_width_downgrade.supported =
			cpu_to_be16(ppd->link_width_downgrade_supported);
	pi->link_width_downgrade.enabled =
			cpu_to_be16(ppd->link_width_downgrade_enabled);
	pi->link_width_downgrade.active =
			cpu_to_be16(ppd->link_width_downgrade_active);

	pi->link_speed.supported = cpu_to_be16(ppd->link_speed_supported);
	pi->link_speed.active = cpu_to_be16(ppd->link_speed_active);
	pi->link_speed.enabled = cpu_to_be16(ppd->link_speed_enabled);

	state = dd->f_iblink_state(ppd);

	if (start_of_sm_config && (state == IB_PORT_INIT))
		ppd->is_sm_config_started = 1;

	pi->port_states.offline_reason = ppd->neighbor_normal << 4;
	pi->port_states.offline_reason |= ppd->is_sm_config_started << 5;
	pi->port_states.unsleepstate_downdefstate =
		(get_linkdowndefaultstate(ppd) ? 1 : 2);

	pi->port_states.portphysstate_portstate =
		(dd->f_ibphys_portstate(ppd) << 4) | state;

	pi->mkeyprotect_lmc = (ibp->mkeyprot << 6) | ppd->lmc;

	memset(pi->neigh_mtu.pvlx_to_mtu, 0, sizeof(pi->neigh_mtu.pvlx_to_mtu));
	for (i = 0; i < hfi_num_vls(ppd->vls_supported); i++) {
		mtu = mtu_to_enum(dd->vld[i].mtu, HFI_DEFAULT_ACTIVE_MTU);
		if ((i % 2) == 0)
			pi->neigh_mtu.pvlx_to_mtu[i/2] |= (mtu << 4);
		else
			pi->neigh_mtu.pvlx_to_mtu[i/2] |= mtu;
	}
	/* don't forget VL 15 */
	mtu = mtu_to_enum(dd->vld[15].mtu, 2048);
	pi->neigh_mtu.pvlx_to_mtu[15/2] |= mtu;
	pi->smsl = ibp->sm_sl & STL_PI_MASK_SMSL;
	pi->operational_vls =
		hfi_num_vls(dd->f_get_ib_cfg(ppd, QIB_IB_CFG_OP_VLS));

	pi->mkey_violations = cpu_to_be16(ibp->mkey_violations);
	/* P_KeyViolations are counted by hardware. */
	pi->pkey_violations = cpu_to_be16(ibp->pkey_violations);
	pi->qkey_violations = cpu_to_be16(ibp->qkey_violations);

	pi->vl.cap = hfi_num_vls(ppd->vls_supported);
	pi->vl.high_limit = cpu_to_be16(ibp->vl_high_limit);
	pi->vl.arb_high_cap = (u8)dd->f_get_ib_cfg(ppd, QIB_IB_CFG_VL_HIGH_CAP);
	pi->vl.arb_low_cap = (u8)dd->f_get_ib_cfg(ppd, QIB_IB_CFG_VL_LOW_CAP);

	pi->clientrereg_subnettimeout = ibp->subnet_timeout;
	pi->localphy_overrun_errors =
		(get_phyerrthreshold(ppd) << 4) |
		get_overrunthreshold(ppd);

	pi->port_link_mode  = STL_PORT_LINK_MODE_STL << 10;
	pi->port_link_mode |= STL_PORT_LINK_MODE_STL << 5;
	pi->port_link_mode |= STL_PORT_LINK_MODE_STL;
	pi->port_link_mode = cpu_to_be16(pi->port_link_mode);

	pi->port_ltp_crc_mode = cpu_to_be16(ppd->port_ltp_crc_mode);

	/* TODO: other modes */
	pi->port_mode = cpu_to_be16(
				ppd->is_active_optimize_enabled ?
					STL_PI_MASK_PORT_ACTIVE_OPTOMIZE : 0);

	pi->port_packet_format.supported =
		cpu_to_be16(STL_PORT_PACKET_FORMAT_9B);
	pi->port_packet_format.enabled =
		cpu_to_be16(STL_PORT_PACKET_FORMAT_9B);

	/* flit_control.interleave is (STL V1, version .76):
	 * bits		use
	 * ----		---
	 * 2		res
	 * 2		DistanceSupported
	 * 2		DistanceEnabled
	 * 5		MaxNextLevelTxEnabled
	 * 5		MaxNestLevelRxSupported
	 *
	 * WFR supports only "distance mode 1" (see STL V1, version .76,
	 * section 9.6.2), so set DistanceSupported, DistanceEnabled
	 * to 0x1.
	 */
	pi->flit_control.interleave = cpu_to_be16(0x1400);

	pi->link_down_reason = STL_LINKDOWN_REASON_NONE;

	pi->mtucap = mtu_to_enum(max_mtu, IB_MTU_4096);

	/* 32.768 usec. response time (guessing) */
	pi->resptimevalue = 3;

	pi->local_port_num = port;

	/* buffer info for FM */
	pi->overall_buffer_space = cpu_to_be16(dd->link_credits);

	pi->neigh_node_guid = ppd->neighbor_guid;
	pi->port_neigh_mode =
		ppd->neighbor_type & STL_PI_MASK_NEIGH_NODE_TYPE;
	if (ppd->mgmt_allowed)
		pi->port_neigh_mode |= STL_PI_MASK_NEIGH_MGMT_ALLOWED;
	if ((pi->port_neigh_mode & STL_PI_MASK_NEIGH_NODE_TYPE) == 1)
		credit_rate = 0;
	else
		credit_rate = 18;
	buffer_units  = (dd->vau) & STL_PI_MASK_BUF_UNIT_BUF_ALLOC;
	buffer_units |= (dd->vcu << 3) & STL_PI_MASK_BUF_UNIT_CREDIT_ACK;
	buffer_units |= (credit_rate << 6) &
				STL_PI_MASK_BUF_UNIT_VL15_CREDIT_RATE;
	buffer_units |= (dd->vl15_init << 11) & STL_PI_MASK_BUF_UNIT_VL15_INIT;
	pi->buffer_units = cpu_to_be32(buffer_units);

	pi->stl_cap_mask = cpu_to_be16(STL_CAP_MASK3_IsSharedSpaceSupported);

	pi->replay_depth.buffer = 0x80;
	/* WFR supports a replay buffer 128 LTPs in size */
	if (acquire_lcb_access(dd) == 0) {
		tmp = read_csr(dd, DC_LCB_STS_ROUND_TRIP_LTP_CNT);
		tmp >>= DC_LCB_STS_ROUND_TRIP_LTP_CNT_VAL_SHIFT;
		tmp &= DC_LCB_STS_ROUND_TRIP_LTP_CNT_VAL_MASK;
		release_lcb_access(dd);
	} else {
		/* TODO: return an error? */
		tmp = 0;
	}
	/* this counter is 16 bits wide, but the replay_depth.wire
	 * variable is only 8 bits */
	if (tmp > 0xff)
		tmp = 0xff;
	pi->replay_depth.wire = tmp;

	if (resp_len)
		*resp_len += sizeof(struct stl_port_info);

	return reply(smp);
}

/**
 * get_pkeys - return the PKEY table
 * @dd: the qlogic_ib device
 * @port: the IB port number
 * @pkeys: the pkey table is placed here
 */
static int get_pkeys(struct hfi_devdata *dd, u8 port, u16 *pkeys)
{
	struct qib_pportdata *ppd = dd->pport + port - 1;

	memcpy(pkeys, ppd->pkeys, sizeof(ppd->pkeys));

	return 0;
}

static int __subn_get_stl_pkeytable(struct stl_smp *smp, u32 am, u8 *data,
				    struct ib_device *ibdev, u8 port,
				    u32 *resp_len)
{
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	u32 n_blocks_req = STL_AM_NBLK(am);
	u32 am_port = (am & 0x00ff0000) >> 16;
	u32 start_block = am & 0x7ff;
	__be16 *p;
	int i;
	u16 n_blocks_avail;
	unsigned npkeys = qib_get_npkeys(dd);
	size_t size;

	clear_stl_smp_data(smp);

	if (am_port == 0)
		am_port = port;

	if (am_port != port || n_blocks_req == 0) {
		pr_warn("STL Get PKey AM Invalid : "
			"P = %d; B = 0x%x; N = 0x%x\n", am_port,
			start_block, n_blocks_req);
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}

	n_blocks_avail = (u16) (npkeys/STL_PARTITION_TABLE_BLK_SIZE) + 1;

	size = (n_blocks_req * STL_PARTITION_TABLE_BLK_SIZE) * sizeof(u16);

	if (start_block + n_blocks_req > n_blocks_avail ||
	    n_blocks_req > STL_NUM_PKEY_BLOCKS_PER_SMP) {
		pr_warn("STL Get PKey AM Invalid : s 0x%x; req 0x%x; "
			"avail 0x%x; blk/smp 0x%lx\n",
			start_block, n_blocks_req, n_blocks_avail,
			STL_NUM_PKEY_BLOCKS_PER_SMP);
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}

	p = (__be16 *) data;
	/* get the real pkeys if we are requesting the first block */
	if (start_block == 0) {
		get_pkeys(dd, port, p);
		for (i = 0; i < npkeys; i++)
			p[i] = cpu_to_be16(p[i]);
		if (resp_len)
			*resp_len += size;
	} else
		smp->status |= IB_SMP_INVALID_FIELD;

	return reply(smp);
}

static int set_port_states(struct qib_pportdata *ppd, struct stl_smp *smp,
			   u32 state, u32 lstate, int suppress_idle_sma)
{
	struct hfi_devdata *dd = ppd->dd;
	int ret;

	if (lstate && !(state == IB_PORT_DOWN || state == IB_PORT_NOP)) {
		pr_warn("SubnSet(STL_PortInfo) port state invalid; state 0x%x link state 0x%x\n",
			state, lstate);
		smp->status |= IB_SMP_INVALID_FIELD;
	}

	/*
	 * Only state changes of DOWN, ARM, and ACTIVE are valid
	 * and must be in the correct state to take effect (see 7.2.6).
	 */
	switch (state) {
	case IB_PORT_NOP:
		if (lstate == 0)
			break;
		/* FALLTHROUGH */
	case IB_PORT_DOWN:
		if (lstate == 0)
			lstate = HLS_DN_DOWNDEF;
		else if (lstate == 2)
			lstate = HLS_DN_POLL;
		else if (lstate == 3)
			lstate = HLS_DN_DISABLE;
		else {
			pr_warn("SubnSet(STL_PortInfo) invalid Physical state 0x%x\n",
				lstate);
			smp->status |= IB_SMP_INVALID_FIELD;
			break;
		}

		set_link_state(ppd, lstate);
		/*
		 * Don't send a reply if the response would be sent
		 * through the disabled port.
		 */
		if (lstate == HLS_DN_DISABLE && smp->hop_cnt)
			return IB_MAD_RESULT_SUCCESS | IB_MAD_RESULT_CONSUMED;
		/* XXX ??? qib_wait_linkstate(ppd, QIBL_LINKV, 10); */
		break;
	case IB_PORT_ARMED:
		ret = set_link_state(ppd, HLS_UP_ARMED);
		if ((ret == 0) && (suppress_idle_sma == 0))
			send_idle_sma(dd, SMA_IDLE_ARM);
		break;
	case IB_PORT_ACTIVE:
		if (ppd->neighbor_normal) {
			ret = set_link_state(ppd, HLS_UP_ACTIVE);
			if (ret == 0)
				send_idle_sma(dd, SMA_IDLE_ACTIVE);
		} else {
			pr_warn("SubnSet(STL_PortInfo) Cannot move to Active with NeighborNormal 0\n");
			smp->status |= IB_SMP_INVALID_FIELD;
		}
		break;
	default:
		pr_warn("SubnSet(STL_PortInfo) invalid state 0x%x\n", state);
		smp->status |= IB_SMP_INVALID_FIELD;
	}

	return 0;
}

/**
 * subn_set_stl_portinfo - set port information
 * @smp: the incoming SM packet
 * @ibdev: the infiniband device
 * @port: the port on the device
 *
 */
static int __subn_set_stl_portinfo(struct stl_smp *smp, u32 am, u8 *data,
				   struct ib_device *ibdev, u8 port,
				   u32 *resp_len)
{
	struct stl_port_info *pi = (struct stl_port_info *)data;
	struct ib_event event;
	struct hfi_devdata *dd;
	struct qib_pportdata *ppd;
	struct qib_ibport *ibp;
	u8 clientrereg;
	unsigned long flags;
	u32 stl_lid; /* Temp to hold STL LID values */
	u16 lid, smlid;
	u8 ls_old, ls_new, ps_new;
	u8 vls;
	u8 msl;
	u16 lse, lwe, mtu;
	u32 port_num = STL_AM_PORTNUM(am);
	u32 num_ports = STL_AM_NPORT(am);
	u32 start_of_sm_config = STL_AM_START_SM_CONF(am);
	int ret, ore, i, invalid;

	if (num_ports != 1) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}

	/* PortInfo is only valid on receiving port
	 * or 0 for wildcard
	 */
	if (port_num == 0)
		port_num = port;
	else {
		if (port_num != port)
			goto get_only;
	}

	stl_lid = be32_to_cpu(pi->lid);
	if (stl_lid & 0xFFFF0000) {
		pr_warn("STL_PortInfo lid out of range: %X\n", stl_lid);
		smp->status |= IB_SMP_INVALID_FIELD;
		goto get_only;
	}

	lid = (u16)(stl_lid & 0x0000FFFF);

	stl_lid = be32_to_cpu(pi->sm_lid);
	if (stl_lid & 0xFFFF0000) {
		pr_warn("STL_PortInfo SM lid out of range: %X\n", stl_lid);
		smp->status |= IB_SMP_INVALID_FIELD;
		goto get_only;
	}
	smlid = (u16)(stl_lid & 0x0000FFFF);

	clientrereg = (pi->clientrereg_subnettimeout &
			STL_PI_MASK_CLIENT_REREGISTER);

	dd = dd_from_ibdev(ibdev);
	/* IB numbers ports from 1, hdw from 0 */
	ppd = dd->pport + (port_num - 1);
	ibp = &ppd->ibport_data;
	event.device = ibdev;
	event.element.port_num = port;

	ibp->mkey = pi->mkey;
	ibp->gid_prefix = pi->subnet_prefix;
	ibp->mkey_lease_period = be16_to_cpu(pi->mkey_lease_period);

	/* Must be a valid unicast LID address. */
	if (lid == 0 || lid >= QIB_MULTICAST_LID_BASE) {
		smp->status |= IB_SMP_INVALID_FIELD;
		pr_warn("SubnSet(STL_PortInfo) lid invalid 0x%x\n",
			lid);
	} else if (ppd->lid != lid ||
		 ppd->lmc != (pi->mkeyprotect_lmc & STL_PI_MASK_LMC)) {
		if (ppd->lid != lid)
			qib_set_uevent_bits(ppd, _QIB_EVENT_LID_CHANGE_BIT);
		if (ppd->lmc != (pi->mkeyprotect_lmc & STL_PI_MASK_LMC))
			qib_set_uevent_bits(ppd, _QIB_EVENT_LMC_CHANGE_BIT);
		qib_set_lid(ppd, lid, pi->mkeyprotect_lmc & STL_PI_MASK_LMC);
		event.event = IB_EVENT_LID_CHANGE;
		ib_dispatch_event(&event);
	}

	msl = pi->smsl & STL_PI_MASK_SMSL;
	/* Must be a valid unicast LID address. */
	if (smlid == 0 || smlid >= QIB_MULTICAST_LID_BASE) {
		smp->status |= IB_SMP_INVALID_FIELD;
		pr_warn("SubnSet(STL_PortInfo) smlid invalid 0x%x\n", smlid);
	} else if (smlid != ibp->sm_lid || msl != ibp->sm_sl) {
		pr_warn("SubnSet(STL_PortInfo) smlid 0x%x\n", smlid);
		spin_lock_irqsave(&ibp->lock, flags);
		if (ibp->sm_ah) {
			if (smlid != ibp->sm_lid)
				ibp->sm_ah->attr.dlid = smlid;
			if (msl != ibp->sm_sl)
				ibp->sm_ah->attr.sl = msl;
		}
		spin_unlock_irqrestore(&ibp->lock, flags);
		if (smlid != ibp->sm_lid)
			ibp->sm_lid = smlid;
		if (msl != ibp->sm_sl)
			ibp->sm_sl = msl;
		event.event = IB_EVENT_SM_CHANGE;
		ib_dispatch_event(&event);
	}

	lwe = be16_to_cpu(pi->link_width.enabled);
	if (lwe) {
		if ((lwe & STL_LINK_WIDTH_ALL_SUPPORTED) == STL_LINK_WIDTH_ALL_SUPPORTED) {
			set_link_width_enabled(ppd, STL_LINK_WIDTH_ALL_SUPPORTED);
		} else if (lwe & be16_to_cpu(pi->link_width.supported)) {
			set_link_width_enabled(ppd, lwe);
		} else
			smp->status |= IB_SMP_INVALID_FIELD;
	}
	lwe = be16_to_cpu(pi->link_width_downgrade.enabled);
	if (lwe) {
		if ((lwe & STL_LINK_WIDTH_ALL_SUPPORTED)
					== STL_LINK_WIDTH_ALL_SUPPORTED) {
			set_link_width_downgrade_enabled(ppd,
					STL_LINK_WIDTH_ALL_SUPPORTED);
		} else if (lwe
			    & be16_to_cpu(pi->link_width_downgrade.supported)) {
			set_link_width_downgrade_enabled(ppd, lwe);
		} else
			smp->status |= IB_SMP_INVALID_FIELD;
	}
	lse = be16_to_cpu(pi->link_speed.enabled);
	if (lse) {
		if ((lse & STL_LINK_SPEED_ALL_SUPPORTED) == STL_LINK_SPEED_ALL_SUPPORTED) {
			set_link_speed_enabled(ppd, STL_LINK_SPEED_ALL_SUPPORTED);
		} else if (lse & be16_to_cpu(pi->link_speed.supported)) {
			set_link_speed_enabled(ppd, lse);
		} else
			smp->status |= IB_SMP_INVALID_FIELD;
	}

	ibp->mkeyprot = (pi->mkeyprotect_lmc & STL_PI_MASK_MKEY_PROT_BIT) >> 6;
	ibp->vl_high_limit = be16_to_cpu(pi->vl.high_limit) & 0xFF;
	(void) dd->f_set_ib_cfg(ppd, QIB_IB_CFG_VL_HIGH_LIMIT,
				    ibp->vl_high_limit);

	for (i = 0; i < hfi_num_vls(ppd->vls_supported); i++) {
		if ((i % 2) == 0)
			mtu = enum_to_mtu((pi->neigh_mtu.pvlx_to_mtu[i/2] >> 4)
					  & 0xF);
		else
			mtu = enum_to_mtu(pi->neigh_mtu.pvlx_to_mtu[i/2] & 0xF);
		if (mtu == -1) {
			printk(KERN_WARNING
			       "SubnSet(STL_PortInfo) mtu invalid %d (0x%x)\n", mtu,
			       (pi->neigh_mtu.pvlx_to_mtu[0] >> 4) & 0xF);
			smp->status |= IB_SMP_INVALID_FIELD;
		}
		dd->vld[i].mtu = mtu;
	}
	/* don't forget VL 15 */
	dd->vld[15].mtu = enum_to_mtu(pi->neigh_mtu.pvlx_to_mtu[15/2] & 0xF);
	set_mtu(ppd);

	/* Set operational VLs */
	vls = pi->operational_vls & STL_PI_MASK_OPERATIONAL_VL;
	if (vls) {
		int vl_enum = hfi_vls_to_ib_enum(vls);
		if (vls > hfi_num_vls(ppd->vls_supported) || vl_enum < 0) {
			pr_warn("SubnSet(STL_PortInfo) VL's supported invalid %d\n",
				pi->operational_vls);
			smp->status |= IB_SMP_INVALID_FIELD;
		} else
			(void) dd->f_set_ib_cfg(ppd, QIB_IB_CFG_OP_VLS,
						vl_enum);
	}

	if (pi->mkey_violations == 0)
		ibp->mkey_violations = 0;

	if (pi->pkey_violations == 0)
		ibp->pkey_violations = 0;

	if (pi->qkey_violations == 0)
		ibp->qkey_violations = 0;

	ore = pi->localphy_overrun_errors;
	if (set_phyerrthreshold(ppd, (ore >> 4) & 0xF)) {
		pr_warn("SubnSet(STL_PortInfo) phyerrthreshold invalid 0x%x\n",
			(ore >> 4) & 0xF);
		smp->status |= IB_SMP_INVALID_FIELD;
	}

	if (set_overrunthreshold(ppd, (ore & 0xF))) {
		pr_warn("SubnSet(STL_PortInfo) overrunthreshold invalid 0x%x\n",
			ore & 0xF);
		smp->status |= IB_SMP_INVALID_FIELD;
	}

	ibp->subnet_timeout = pi->clientrereg_subnettimeout & STL_PI_MASK_SUBNET_TIMEOUT;

	/* TODO: other modes */
	ppd->is_active_optimize_enabled =
			!!(be16_to_cpu(pi->port_mode)
					& STL_PI_MASK_PORT_ACTIVE_OPTOMIZE);

	ls_old = dd->f_iblink_state(ppd);

	ls_new = pi->port_states.portphysstate_portstate &
			STL_PI_MASK_PORT_STATE;
	ps_new = (pi->port_states.portphysstate_portstate &
			STL_PI_MASK_PORT_PHYSICAL_STATE) >> 4;

	if (ls_old == IB_PORT_INIT) {
		if (start_of_sm_config) {
			if (ls_new == ls_old || (ls_new == IB_PORT_ARMED))
				ppd->is_sm_config_started = 1;
		} else if (ls_new == IB_PORT_ARMED) {
			if (ppd->is_sm_config_started == 0)
				invalid = 1;
		}
	}

	/*
	 * Do the port state change now that the other link parameters
	 * have been set.
	 * Changing the port physical state only makes sense if the link
	 * is down or is being set to down.
	 */

	ret = set_port_states(ppd, smp, ls_new, ps_new, invalid);
	if (ret)
		return ret;

	if (clientrereg) {
		event.event = IB_EVENT_CLIENT_REREGISTER;
		ib_dispatch_event(&event);
	}

	ret = __subn_get_stl_portinfo(smp, am, data, ibdev, port, resp_len);

	/* restore re-reg bit per o14-12.2.1 */
	pi->clientrereg_subnettimeout |= clientrereg;

	return ret;

get_only:
	return __subn_get_stl_portinfo(smp, am, data, ibdev, port, resp_len);
}

/**
 * set_pkeys - set the PKEY table for ctxt 0
 * @dd: the qlogic_ib device
 * @port: the IB port number
 * @pkeys: the PKEY table
 */
static int set_pkeys(struct hfi_devdata *dd, u8 port, u16 *pkeys)
{
	struct qib_pportdata *ppd;
	int i;
	int changed = 0;
	int update_includes_mgmt_partition = 0;

	/*
	 * IB port one/two always maps to context zero/one,
	 * always a kernel context, no locking needed
	 * If we get here with ppd setup, no need to check
	 * that rcd is valid.
	 */
	ppd = dd->pport + (port - 1);
	/*
	 * If the update does not include the management pkey, don't do it.
	 */
	for (i = 0; i < ARRAY_SIZE(ppd->pkeys); i++) {
		if (pkeys[i] == WFR_LIM_MGMT_P_KEY) {
			update_includes_mgmt_partition = 1;
			break;
		}
	}

	if (!update_includes_mgmt_partition)
		return 1;

	for (i = 0; i < ARRAY_SIZE(ppd->pkeys); i++) {
		u16 key = pkeys[i];
		u16 okey = ppd->pkeys[i];

		if (key == okey)
			continue;
		/*
		 * The SM gives us the complete PKey table. We have
		 * to ensure that we put the PKeys in the matching
		 * slots.
		 */
		ppd->pkeys[i] = key;
		changed = 1;
	}

	if (changed) {
		struct ib_event event;

		(void) dd->f_set_ib_cfg(ppd, QIB_IB_CFG_PKEYS, 0);

		event.event = IB_EVENT_PKEY_CHANGE;
		event.device = &dd->verbs_dev.ibdev;
		event.element.port_num = port;
		ib_dispatch_event(&event);
	}
	return 0;
}

static int __subn_set_stl_pkeytable(struct stl_smp *smp, u32 am, u8 *data,
				    struct ib_device *ibdev, u8 port,
				    u32 *resp_len)
{
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	u32 n_blocks_sent = STL_AM_NBLK(am);
	u32 am_port = (am & 0x00ff0000) >> 16;
	u32 start_block = am & 0x7ff;
	u16 *p = (u16 *) data;
	int i;
	u16 n_blocks_avail;
	unsigned npkeys = qib_get_npkeys(dd);
	if (am_port == 0)
		am_port = port;

	if (am_port != port || n_blocks_sent == 0) {
		pr_warn("STL Get PKey AM Invalid : P = %d; B = 0x%x; N = 0x%x\n",
			am_port, start_block, n_blocks_sent);
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}

	n_blocks_avail = (u16)(npkeys/STL_PARTITION_TABLE_BLK_SIZE) + 1;

	if (start_block + n_blocks_sent > n_blocks_avail ||
	    n_blocks_sent > STL_NUM_PKEY_BLOCKS_PER_SMP) {
		pr_warn("STL Set PKey AM Invalid : s 0x%x; req 0x%x; avail 0x%x; blk/smp 0x%lx\n",
			start_block, n_blocks_sent, n_blocks_avail,
			STL_NUM_PKEY_BLOCKS_PER_SMP);
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}

	for (i = 0; i < n_blocks_sent * STL_PARTITION_TABLE_BLK_SIZE; i++)
		p[i] = be16_to_cpu(p[i]);

	if (start_block == 0 && set_pkeys(dd, port, p) != 0) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}

	return __subn_get_stl_pkeytable(smp, am, data, ibdev, port, resp_len);
}

static int get_sc2vlt_tables(struct hfi_devdata *dd, void *data)
{
	u64 *val = (u64 *)data;
	*val++ = read_csr(dd, WFR_SEND_SC2VLT0);
	*val++ = read_csr(dd, WFR_SEND_SC2VLT1);
	*val++ = read_csr(dd, WFR_SEND_SC2VLT2);
	*val++ = read_csr(dd, WFR_SEND_SC2VLT3);
	return 0;
}

static int set_sc2vlt_tables(struct hfi_devdata *dd, void *data)
{
	u64 *val = (u64 *)data;
	write_csr(dd, WFR_SEND_SC2VLT0, *val++);
	write_csr(dd, WFR_SEND_SC2VLT1, *val++);
	write_csr(dd, WFR_SEND_SC2VLT2, *val++);
	write_csr(dd, WFR_SEND_SC2VLT3, *val++);
	write_seqlock_irq(&dd->sc2vl_lock);
	memcpy(dd->sc2vl, (u64 *)data, sizeof(dd->sc2vl));
	write_sequnlock_irq(&dd->sc2vl_lock);
	return 0;
}

static int __subn_get_stl_sl_to_sc(struct stl_smp *smp, u32 am, u8 *data,
				   struct ib_device *ibdev, u8 port,
				   u32 *resp_len)
{
	struct qib_ibport *ibp = to_iport(ibdev, port);
	u8 *p = (u8 *)data;
	size_t size = ARRAY_SIZE(ibp->sl_to_sc); /* == 32 */
	unsigned i;

	clear_stl_smp_data(smp);

	if (am) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}

	for (i = 0; i < ARRAY_SIZE(ibp->sl_to_sc); i++)
		*p++ = ibp->sl_to_sc[i];

	if (resp_len)
		*resp_len += size;

	return reply(smp);
}

static int __subn_set_stl_sl_to_sc(struct stl_smp *smp, u32 am, u8 *data,
				   struct ib_device *ibdev, u8 port,
				   u32 *resp_len)
{
	struct qib_ibport *ibp = to_iport(ibdev, port);
	u8 *p = (u8 *)data;
	int i;

	if (am) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}

	for (i = 0; i <  ARRAY_SIZE(ibp->sl_to_sc); i++)
		ibp->sl_to_sc[i] = *p++;

	return __subn_get_stl_sl_to_sc(smp, am, data, ibdev, port, resp_len);
}

static int __subn_get_stl_sc_to_sl(struct stl_smp *smp, u32 am, u8 *data,
				   struct ib_device *ibdev, u8 port,
				   u32 *resp_len)
{
	struct qib_ibport *ibp = to_iport(ibdev, port);
	u8 *p = (u8 *)data;
	size_t size = ARRAY_SIZE(ibp->sc_to_sl); /* == 32 */
	unsigned i;

	clear_stl_smp_data(smp);

	if (am) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}

	for (i = 0; i < ARRAY_SIZE(ibp->sc_to_sl); i++)
		*p++ = ibp->sc_to_sl[i];

	if (resp_len)
		*resp_len += size;

	return reply(smp);
}

static int __subn_set_stl_sc_to_sl(struct stl_smp *smp, u32 am, u8 *data,
				   struct ib_device *ibdev, u8 port,
				   u32 *resp_len)
{
	struct qib_ibport *ibp = to_iport(ibdev, port);
	u8 *p = (u8 *)data;
	int i;

	if (am) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}

	for (i = 0; i < ARRAY_SIZE(ibp->sc_to_sl); i++)
		ibp->sc_to_sl[i] = *p++;

	return __subn_get_stl_sc_to_sl(smp, am, data, ibdev, port, resp_len);
}

static int __subn_get_stl_sc_to_vlt(struct stl_smp *smp, u32 am, u8 *data,
				    struct ib_device *ibdev, u8 port,
				    u32 *resp_len)
{
	u32 n_blocks = STL_AM_NBLK(am);
	u32 am_port = STL_AM_PORTNUM(am);
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	void *vp = (void *) data;
	size_t size = 4 * sizeof(u64);

	clear_stl_smp_data(smp);

	if (am_port != port || port != 1 || n_blocks != 1) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}

	get_sc2vlt_tables(dd, vp);

	if (resp_len)
		*resp_len += size;

	return reply(smp);
}

static int __subn_set_stl_sc_to_vlt(struct stl_smp *smp, u32 am, u8 *data,
				    struct ib_device *ibdev, u8 port,
				    u32 *resp_len)
{
	u32 n_blocks = STL_AM_NBLK(am);
	u32 am_port = STL_AM_PORTNUM(am);
	int async_update = STL_AM_ASYNC(am);
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	void *vp = (void *) data;
	struct qib_pportdata *ppd;
	int lstate;

	if (am_port != port || port != 1 || n_blocks != 1 || async_update) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}

	/* IB numbers ports from 1, hdw from 0 */
	ppd = dd->pport + (port - 1);
	lstate = dd->f_iblink_state(ppd);
	/* it's known that async_update is 0 by this point, but include
	 * the explicit check for clarity */
	if (!async_update &&
	    (lstate == IB_PORT_ARMED || lstate == IB_PORT_ACTIVE)) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}

	set_sc2vlt_tables(dd, vp);

	return __subn_get_stl_sc_to_vlt(smp, am, data, ibdev, port, resp_len);
}

static int __subn_get_stl_sc_to_vlnt(struct stl_smp *smp, u32 am, u8 *data,
				     struct ib_device *ibdev, u8 port,
				     u32 *resp_len)
{
	u32 n_blocks = STL_AM_NPORT(am);
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	struct qib_pportdata *ppd;
	void *vp = (void *) data;
	int size;

	clear_stl_smp_data(smp);

	if (port != 1 || n_blocks != 1) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}

	ppd = dd->pport + (port - 1);

	size = fm_get_table(ppd, FM_TBL_SC2VLNT, vp);

	if (resp_len)
		*resp_len += size;

	return reply(smp);
}

static int __subn_set_stl_sc_to_vlnt(struct stl_smp *smp, u32 am, u8 *data,
				     struct ib_device *ibdev, u8 port,
				     u32 *resp_len)
{
	u32 n_blocks = STL_AM_NPORT(am);
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	struct qib_pportdata *ppd;
	void *vp = (void *) data;
	int lstate;

	if (port != 1 || n_blocks != 1) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}

	/* IB numbers ports from 1, hdw from 0 */
	ppd = dd->pport + (port - 1);
	lstate = dd->f_iblink_state(ppd);
	if (lstate == IB_PORT_ARMED || lstate == IB_PORT_ACTIVE) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}

	ppd = dd->pport + (port - 1);

	fm_set_table(ppd, FM_TBL_SC2VLNT, vp);

	return __subn_get_stl_sc_to_vlnt(smp, am, data, ibdev, port,
					 resp_len);
}

static int __subn_get_stl_psi(struct stl_smp *smp, u32 am, u8 *data,
			      struct ib_device *ibdev, u8 port,
			      u32 *resp_len)
{
	u32 nports = STL_AM_NPORT(am);
	u32 port_num = STL_AM_PORTNUM(am);
	u32 start_of_sm_config = STL_AM_START_SM_CONF(am);
	u32 lstate;
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	struct qib_ibport *ibp;
	struct qib_pportdata *ppd;
	struct stl_port_states *psi = (struct stl_port_states *) data;

	clear_stl_smp_data(smp);

	if (port_num == 0)
		port_num = port;

	if (nports != 1 || port_num != port) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}

	ibp = to_iport(ibdev, port);
	ppd = ppd_from_ibp(ibp);

	lstate = dd->f_iblink_state(ppd);

	if (start_of_sm_config && (lstate == IB_PORT_INIT))
		ppd->is_sm_config_started = 1;

	psi->offline_reason = ppd->neighbor_normal << 4;
	psi->offline_reason |= ppd->is_sm_config_started << 5;
	psi->unsleepstate_downdefstate =
		(get_linkdowndefaultstate(ppd) ? 1 : 2);

	psi->portphysstate_portstate =
		(dd->f_ibphys_portstate(ppd) << 4) | (lstate & 0xf);

	if (resp_len)
		*resp_len += sizeof(struct stl_port_states);

	return reply(smp);
}

static int __subn_set_stl_psi(struct stl_smp *smp, u32 am, u8 *data,
			      struct ib_device *ibdev, u8 port,
			      u32 *resp_len)
{
	u32 nports = STL_AM_NPORT(am);
	u32 port_num = STL_AM_PORTNUM(am);
	u32 start_of_sm_config = STL_AM_START_SM_CONF(am);
	u32 ls_old;
	u8 ls_new, ps_new;
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	struct qib_ibport *ibp;
	struct qib_pportdata *ppd;
	struct stl_port_states *psi = (struct stl_port_states *) data;
	int ret, invalid = 0;

	if (port_num == 0)
		port_num = port;

	if (nports != 1 || port_num != port) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}

	ibp = to_iport(ibdev, port);
	ppd = ppd_from_ibp(ibp);

	ls_old = dd->f_iblink_state(ppd);

	ls_new = port_states_to_logical_state(psi);
	ps_new = port_states_to_phys_state(psi);

	if (ls_old == IB_PORT_INIT) {
		if (start_of_sm_config) {
			if (ls_new == ls_old || (ls_new == IB_PORT_ARMED))
				ppd->is_sm_config_started = 1;
		} else if (ls_new == IB_PORT_ARMED) {
			if (ppd->is_sm_config_started == 0)
				invalid = 1;
		}
	}

	ret = set_port_states(ppd, smp, ls_new, ps_new, invalid);
	if (ret)
		return ret;

	if (invalid)
		smp->status |= IB_SMP_INVALID_FIELD;

	return __subn_get_stl_psi(smp, am, data, ibdev, port, resp_len);
}

static int __subn_get_stl_bct(struct stl_smp *smp, u32 am, u8 *data,
			      struct ib_device *ibdev, u8 port, u32 *resp_len)
{
	u32 num_ports = STL_AM_NPORT(am);
	u32 port_num = STL_AM_PORTNUM(am);
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	struct qib_pportdata *ppd;
	struct buffer_control *p = (struct buffer_control *) data;
	int size;

	clear_stl_smp_data(smp);

	if (num_ports != 1 || port_num != port) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}

	ppd = dd->pport + (port_num - 1);
	size = fm_get_table(ppd, FM_TBL_BUFFER_CONTROL, p);
	trace_bct_get(dd, p);
	if (resp_len)
		*resp_len += size;

	return reply(smp);
}

static int __subn_set_stl_bct(struct stl_smp *smp, u32 am, u8 *data,
			      struct ib_device *ibdev, u8 port, u32 *resp_len)
{
	u32 num_ports = STL_AM_NPORT(am);
	u32 port_num = STL_AM_PORTNUM(am);
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	struct qib_pportdata *ppd;
	struct buffer_control *p = (struct buffer_control *) data;

	if (num_ports != 1 || port_num != port) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}
	ppd = dd->pport + (port_num - 1);
	trace_bct_set(dd, p);
	if (fm_set_table(ppd, FM_TBL_BUFFER_CONTROL, p) < 0) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}

	return __subn_get_stl_bct(smp, am, data, ibdev, port, resp_len);
}

static int __subn_get_stl_vl_arb(struct stl_smp *smp, u32 am, u8 *data,
				 struct ib_device *ibdev, u8 port,
				 u32 *resp_len)
{
	struct qib_pportdata *ppd = ppd_from_ibp(to_iport(ibdev, port));
	u32 num_ports = STL_AM_NPORT(am);
	u8 section = (am & 0x00ff0000) >> 16;
	u32 port_num = STL_AM_PORTNUM(am);
	u8 *p = data;
	int size = 0;

	clear_stl_smp_data(smp);

	if (port_num == 0)
		port_num = port;

	if (num_ports != 1 || port_num != port) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}

	switch (section) {
	case STL_VLARB_LOW_ELEMENTS:
		size = fm_get_table(ppd, FM_TBL_VL_LOW_ARB, p);
		break;
	case STL_VLARB_HIGH_ELEMENTS:
		size = fm_get_table(ppd, FM_TBL_VL_HIGH_ARB, p);
		break;
	case STL_VLARB_PREEMPT_ELEMENTS:
		size = fm_get_table(ppd, FM_TBL_VL_PREEMPT_ELEMS, p);
		break;
	case STL_VLARB_PREEMPT_MATRIX:
		size = fm_get_table(ppd, FM_TBL_VL_PREEMPT_MATRIX, p);
		break;
	default:
		pr_warn("STL SubnGet(VL Arb) AM Invalid : 0x%x\n",
			be32_to_cpu(smp->attr_mod));
		smp->status |= IB_SMP_INVALID_FIELD;
		break;
	}

	if (size > 0 && resp_len)
		*resp_len += size;

	return reply(smp);
}

static int __subn_set_stl_vl_arb(struct stl_smp *smp, u32 am, u8 *data,
				 struct ib_device *ibdev, u8 port,
				 u32 *resp_len)
{
	struct qib_pportdata *ppd = ppd_from_ibp(to_iport(ibdev, port));
	u32 num_ports = STL_AM_NPORT(am);
	u8 section = (am & 0x00ff0000) >> 16;
	u32 port_num = STL_AM_PORTNUM(am);
	u8 *p = data;

	if (port_num == 0)
		port_num = port;

	if (num_ports != 1 || port_num != port) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}

	switch (section) {
	case STL_VLARB_LOW_ELEMENTS:
		(void) fm_set_table(ppd, FM_TBL_VL_LOW_ARB, p);
		break;
	case STL_VLARB_HIGH_ELEMENTS:
		(void) fm_set_table(ppd, FM_TBL_VL_HIGH_ARB, p);
		break;
	/* neither STL_VLARB_PREEMPT_ELEMENTS, or STL_VLARB_PREEMPT_MATRIX
	 * can be changed from the default values */
	case STL_VLARB_PREEMPT_ELEMENTS:
		/* FALLTHROUGH */
	case STL_VLARB_PREEMPT_MATRIX:
		smp->status |= IB_SMP_UNSUP_METH_ATTR;
		break;
	default:
		pr_warn("STL SubnSet(VL Arb) AM Invalid : 0x%x\n",
			be32_to_cpu(smp->attr_mod));
		smp->status |= IB_SMP_INVALID_FIELD;
		break;
	}

	return __subn_get_stl_vl_arb(smp, am, data, ibdev, port, resp_len);
}

struct stl_pma_mad {
	struct ib_mad_hdr mad_hdr;
	u8 data[2024];
} __packed;

struct stl_port_status_req {
	__u8 port_num;
	__u8 reserved[3];
	__be32 vl_select_mask;
};

#define WFR_VL_MASK_ALL		0x000080ff

struct stl_port_status_rsp {
	__u8 port_num;
	__u8 reserved[3];
	__be32  vl_select_mask;

	/* Data counters */
	__be64 port_xmit_data;
	__be64 port_rcv_data;
	__be64 port_xmit_pkts;
	__be64 port_rcv_pkts;
	__be64 port_multicast_xmit_pkts;
	__be64 port_multicast_rcv_pkts;
	__be64 port_xmit_wait;
	__be64 sw_port_congestion;
	__be64 port_rcv_fecn;
	__be64 port_rcv_becn;
	__be64 port_xmit_time_cong;
	__be64 port_xmit_wasted_bw;
	__be64 port_xmit_wait_data;
	__be64 port_rcv_bubble;
	__be64 port_mark_fecn;
	/* Error counters */
	__be64 port_rcv_constraint_errors;
	__be64 port_rcv_switch_relay_errors;
	__be64 port_xmit_discards;
	__be64 port_xmit_constraint_errors;
	__be64 port_rcv_remote_physical_errors;
	__be64 local_link_integrity_errors;
	__be64 port_rcv_errors;
	__be64 excessive_buffer_overruns;
	__be64 fm_config_errors;
	__be32 link_error_recovery;
	__be32 link_downed;
	u8 uncorrectable_errors;

	u8 link_quality_indicator; /* 5res, 3bit */
	u8 res2[6];
	struct _vls_pctrs {
		/* per-VL Data counters */
		__be64 port_vl_xmit_data;
		__be64 port_vl_rcv_data;
		__be64 port_vl_xmit_pkts;
		__be64 port_vl_rcv_pkts;
		__be64 port_vl_xmit_wait;
		__be64 sw_port_vl_congestion;
		__be64 port_vl_rcv_fecn;
		__be64 port_vl_rcv_becn;
		__be64 port_xmit_time_cong;
		__be64 port_vl_xmit_wasted_bw;
		__be64 port_vl_xmit_wait_data;
		__be64 port_vl_rcv_bubble;
		__be64 port_vl_mark_fecn;
		__be64 port_vl_xmit_discards;
	} vls[0]; /* real array size defined by # bits set in vl_select_mask */
};

enum counter_selects {
	CS_PORT_XMIT_DATA			= (1 << 0),
	CS_PORT_RCV_DATA			= (1 << 1),
	CS_PORT_XMIT_PKTS			= (1 << 2),
	CS_PORT_RCV_PKTS			= (1 << 3),
	CS_PORT_MCAST_XMIT_PKTS			= (1 << 4),
	CS_PORT_MCAST_RCV_PKTS			= (1 << 5),
	CS_PORT_XMIT_WAIT			= (1 << 6),
	CS_SW_PORT_CONGESTION			= (1 << 7),
	CS_PORT_RCV_FECN			= (1 << 8),
	CS_PORT_RCV_BECN			= (1 << 9),
	CS_PORT_XMIT_TIME_CONG			= (1 << 10),
	CS_PORT_XMIT_WASTED_BW			= (1 << 11),
	CS_PORT_XMIT_WAIT_DATA			= (1 << 12),
	CS_PORT_RCV_BUBBLE			= (1 << 13),
	CS_PORT_MARK_FECN			= (1 << 14),
	CS_PORT_RCV_CONSTRAINT_ERRORS		= (1 << 15),
	CS_PORT_RCV_SWITCH_RELAY_ERRORS		= (1 << 16),
	CS_PORT_XMIT_DISCARDS			= (1 << 17),
	CS_PORT_XMIT_CONSTRAINT_ERRORS		= (1 << 18),
	CS_PORT_RCV_REMOTE_PHYSICAL_ERRORS	= (1 << 19),
	CS_LOCAL_LINK_INTEGRITY_ERRORS		= (1 << 20),
	CS_PORT_RCV_ERRORS			= (1 << 21),
	CS_EXCESSIVE_BUFFER_OVERRUNS		= (1 << 22),
	CS_FM_CONFIG_ERRORS			= (1 << 23),
	CS_LINK_ERROR_RECOVERY			= (1 << 24),
	CS_LINK_DOWNED				= (1 << 25),
	CS_UNCORRECTABLE_ERRORS			= (1 << 26),
};

struct stl_clear_port_status {
	__be64 port_select_mask[4];
	__be32 counter_select_mask;
};

struct stl_aggregate {
	__be16 attr_id;
	__be16 err_reqlength;	/* 1 bit, 8 res, 7 bit */
	__be32 attr_mod;
	u8 data[0];
};

/* Request contains first two fields, response contains those plus the rest */
struct stl_port_data_counters_msg {
	__be64 port_select_mask[4];
	__be32 vl_select_mask;

	/* Response fields follow */
	__be32 reserved1;
	struct _port_dctrs {
		u8 port_number;
		u8 reserved2[3];
		__be32 link_quality_indicator; /* 29res, 3bit */

		/* Data counters */
		__be64 port_xmit_data;
		__be64 port_rcv_data;
		__be64 port_xmit_pkts;
		__be64 port_rcv_pkts;
		__be64 port_multicast_xmit_pkts;
		__be64 port_multicast_rcv_pkts;
		__be64 port_xmit_wait;
		__be64 sw_port_congestion;
		__be64 port_rcv_fecn;
		__be64 port_rcv_becn;
		__be64 port_xmit_time_cong;
		__be64 port_xmit_wasted_bw;
		__be64 port_xmit_wait_data;
		__be64 port_rcv_bubble;
		__be64 port_mark_fecn;

		__be64 port_error_counter_summary;
		/* Sum of error counts/port */

		struct _vls_dctrs {
			/* per-VL Data counters */
			__be64 port_vl_xmit_data;
			__be64 port_vl_rcv_data;
			__be64 port_vl_xmit_pkts;
			__be64 port_vl_rcv_pkts;
			__be64 port_vl_xmit_wait;
			__be64 sw_port_vl_congestion;
			__be64 port_vl_rcv_fecn;
			__be64 port_vl_rcv_becn;
			__be64 port_xmit_time_cong;
			__be64 port_vl_xmit_wasted_bw;
			__be64 port_vl_xmit_wait_data;
			__be64 port_vl_rcv_bubble;
			__be64 port_vl_mark_fecn;
		} vls[0];
		/* array size defined by #bits set in vl_select_mask*/
	} port[1]; /* array size defined by  #ports in attribute modifier */
};

#define COUNTER_SIZE_MODE_ALL64		0
#define COUNTER_SIZE_MODE_ALL32		1
#define COUNTER_SIZE_MODE_MIXED		2

struct stl_port_error_counters64_msg {
	/* Request contains first two fields, response contains the
	 * whole magilla */
	__be64 port_select_mask[4];
	__be32 vl_select_mask;

	/* Response-only fields follow */
	__be32 reserved1;
	struct _port_ectrs {
		u8 port_number;
		u8 reserved2[7];
		__be64 port_rcv_constraint_errors;
		__be64 port_rcv_switch_relay_errors;
		__be64 port_xmit_discards;
		__be64 port_xmit_constraint_errors;
		__be64 port_rcv_remote_physical_errors;
		__be64 local_link_integrity_errors;
		__be64 port_rcv_errors;
		__be64 excessive_buffer_overruns;
		__be64 fm_config_errors;
		__be32 link_error_recovery;
		__be32 link_downed;
		u8 uncorrectable_errors;
		u8 reserved3[7];
		struct _vls_ectrs {
			__be64 port_vl_xmit_discards;
		} vls[0];
		/* array size defined by #bits set in vl_select_mask */
	} port[1]; /* array size defined by #ports in attribute modifier */
};

struct stl_port_error_info_msg {
	__be64 port_select_mask[4];
	__be32 error_info_select_mask;
	__be32 reserved1;
	struct _port_ei {

		u8 port_number;
		u8 reserved2[7];

		/* PortRcvErrorInfo */
		struct {
			u8 status_and_code;
			union {
				u8 raw[17];
				struct {
					/* EI1to12 format */
					u8 packet_flit1[8];
					u8 packet_flit2[8];
					u8 remaining_flit_bits12;
				} ei1to12;
				struct {
					u8 packet_bytes[8];
					u8 remaining_flit_bits;
				} ei13;
			} ei;
			u8 reserved3[6];
		} port_rcv_ei;

		/* ExcessiveBufferOverrunInfo */
		struct {
			u8 status_and_sc;
			u8 reserved4[7];
		} excessive_buffer_overrun_ei;

		/* PortXmitConstraintErrorInfo */
		struct {
			u8 status;
			u8 reserved5;
			__be16 pkey;
			__be32 slid;
		} port_xmit_constraint_ei;

		/* PortRcvConstraintErrorInfo */
		struct {
			u8 status;
			u8 reserved6;
			__be16 pkey;
			__be32 slid;
		} port_rcv_constraint_ei;

		/* PortRcvSwitchRelayErrorInfo */
		struct {
			u8 status_and_code;
			u8 reserved7[3];
			__u32 error_info;
		} port_rcv_switch_relay_ei;

		/* UncorrectableErrorInfo */
		struct {
			u8 status_and_code;
			u8 reserved8;
		} uncorrectable_ei;

		/* FMConfigErrorInfo */
		struct {
			u8 status_and_code;
			u8 error_info;
			__u32 reserved9;
		} fm_config_ei;
	} port[1]; /* actual array size defined by #ports in attr modifier */
};

/* stl_port_error_info_msg error_info_select_mask bit definitions */
enum error_info_selects {
	ES_PORT_RCV_ERROR_INFO			= (1 << 31),
	ES_EXCESSIVE_BUFFER_OVERRUN_INFO	= (1 << 30),
	ES_PORT_XMIT_CONSTRAINT_ERROR_INFO	= (1 << 29),
	ES_PORT_RCV_CONSTRAINT_ERROR_INFO	= (1 << 28),
	ES_PORT_RCV_SWITCH_RELAY_ERROR_INFO	= (1 << 27),
	ES_UNCORRECTABLE_ERROR_INFO		= (1 << 26),
	ES_FM_CONFIG_ERROR_INFO			= (1 << 25)
};

static int pma_get_stl_classportinfo(struct stl_pma_mad *pmp,
				     struct ib_device *ibdev)
{
	struct ib_class_port_info *p =
		(struct ib_class_port_info *)pmp->data;
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);

	memset(pmp->data, 0, sizeof(pmp->data));

	if (pmp->mad_hdr.attr_mod != 0)
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;

	p->base_version = JUMBO_MGMT_BASE_VERSION;
	p->class_version = STL_SMI_CLASS_VERSION;
	/*
	 * Set the most significant bit of CM2 to indicate support for
	 * congestion statistics
	 */
	p->reserved[0] = dd->psxmitwait_supported << 7;
	/*
	 * Expected response time is 4.096 usec. * 2^18 == 1.073741824 sec.
	 */
	p->resp_time_value = 18;

	return reply(pmp);
}

static int pma_get_stl_portstatus(struct stl_pma_mad *pmp,
				  struct ib_device *ibdev, u8 port)
{
	struct stl_port_status_req *req =
		(struct stl_port_status_req *)pmp->data;
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	struct stl_port_status_rsp *rsp;
	u32 vl_select_mask = be32_to_cpu(req->vl_select_mask);
	unsigned long vl;
	size_t response_data_size;
	u32 nports = be32_to_cpu(pmp->mad_hdr.attr_mod) >> 24;
	u8 port_num = req->port_num;
	u8 num_vls = hweight32(vl_select_mask);
	struct _vls_pctrs *vlinfo;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	int vfi;
	u64 tmp;

	/* FIXME
	 * some of the counters are not implemented. if the WFR spec
	 * indicates the source of the value (e.g., driver, DC, etc.)
	 * that's noted. If I don't have a clue how to get the counter,
	 * a '???' appears.
	 */
	response_data_size = sizeof(struct stl_port_status_rsp) +
				num_vls * sizeof(struct _vls_pctrs);
	if (response_data_size > sizeof(pmp->data)) {
		pmp->mad_hdr.status |= STL_PM_STATUS_REQUEST_TOO_LARGE;
		return reply(pmp);
	}

	if (nports != 1 || (port_num && port_num != port)
	    || num_vls > STL_MAX_VLS || (vl_select_mask & ~WFR_VL_MASK_ALL)) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		return reply(pmp);
	}

	memset(pmp->data, 0, sizeof(pmp->data));

	rsp = (struct stl_port_status_rsp *)pmp->data;
	if (port_num)
		rsp->port_num = port_num;
	else
		rsp->port_num = port;

	rsp->vl_select_mask = cpu_to_be32(vl_select_mask);

	rsp->port_xmit_data =
		cpu_to_be64(read_csr(dd, DCC_PRF_PORT_XMIT_DATA_CNT));
	rsp->port_rcv_data =
		cpu_to_be64(read_csr(dd, DCC_PRF_PORT_RCV_DATA_CNT));
	rsp->port_xmit_pkts =
		cpu_to_be64(read_csr(dd, DCC_PRF_PORT_XMIT_PKTS_CNT));
	rsp->port_rcv_pkts =
		cpu_to_be64(read_csr(dd, DCC_PRF_PORT_RCV_PKTS_CNT));
	rsp->port_multicast_xmit_pkts =
		cpu_to_be64(read_csr(dd, DCC_PRF_PORT_XMIT_MULTICAST_CNT));
	rsp->port_multicast_rcv_pkts =
		cpu_to_be64(read_csr(dd, DCC_PRF_PORT_RCV_MULTICAST_PKT_CNT));
	rsp->port_xmit_wait =
		cpu_to_be64(read_csr(dd,
			(SEND_WAIT_CNT * 8 + WFR_SEND_COUNTER_ARRAY64)));
	/* rsp->sw_port_congestion is 0 for HFIs */
	rsp->port_rcv_fecn =
		cpu_to_be64(read_csr(dd, DCC_PRF_PORT_RCV_FECN_CNT));
	rsp->port_rcv_becn =
		cpu_to_be64(read_csr(dd, DCC_PRF_PORT_RCV_BECN_CNT));
	/* rsp->port_xmit_time_cong is 0 for HFIs */
	/* rsp->port_xmit_wasted_bw ??? */
	/* rsp->port_xmit_wait_data ??? */
	rsp->port_rcv_bubble =
		cpu_to_be64(read_csr(dd, DCC_PRF_PORT_RCV_BUBBLE_CNT));
	rsp->port_mark_fecn =
		cpu_to_be64(read_csr(dd, DCC_PRF_PORT_MARK_FECN_CNT));
	/* rsp->port_rcv_constraint_errors ??? */
	/* rsp->port_rcv_switch_relay_errors is 0 for HFIs */
	rsp->port_xmit_discards = cpu_to_be64(ppd->port_xmit_discards);
	/* port_xmit_constraint_errors - driver (table 13-11 WFR spec) */
	rsp->port_rcv_remote_physical_errors =
		cpu_to_be64(read_csr(dd, DCC_ERR_RCVREMOTE_PHY_ERR_CNT));
	if (acquire_lcb_access(dd) == 0) {
		rsp->local_link_integrity_errors = cpu_to_be64(
			read_csr(dd, DC_LCB_ERR_INFO_TX_REPLAY_CNT));
		release_lcb_access(dd);
	}
	rsp->port_rcv_errors =
		cpu_to_be64(read_csr(dd, DCC_ERR_PORTRCV_ERR_CNT));
	rsp->excessive_buffer_overruns =
		cpu_to_be64(read_csr(dd, WFR_RCV_COUNTER_ARRAY32 +
			    8 * RCV_BUF_OVFL_CNT));
	rsp->fm_config_errors =
		cpu_to_be64(read_csr(dd, DCC_ERR_FMCONFIG_ERR_CNT));
	/* rsp->link_error_recovery - DC (table 13-11 WFR spec) */
	rsp->link_downed = cpu_to_be32(ppd->link_downed);
	/* rsp->uncorrectable_errors is 8 bits wide, and it pegs at 0xff */
	tmp = read_csr(dd, DCC_ERR_UNCORRECTABLE_CNT);
	rsp->uncorrectable_errors = tmp < 0x100 ? (tmp & 0xff) : 0xff;
	rsp->link_quality_indicator = ppd->link_quality;
	vlinfo = &(rsp->vls[0]);
	vfi = 0;
	/* The vl_select_mask has been checked above, and we know
	 * that it contains only entries which represent valid VLs.
	 * So in the for_each_set_bit() loop below, we don't need
	 * any additional checks for vl.
	 */
	for_each_set_bit(vl, (unsigned long *)&(vl_select_mask),
			 8 * sizeof(vl_select_mask)) {
		unsigned offset;
		memset(vlinfo, 0, sizeof(*vlinfo));
		/* for data VLs, the byte offset from the associated VL0
		 * register is 8 * vl, but for VL15 it's 8 * 8 */
		offset = 8 * (unsigned)((vl == 15) ? 8 : vl);
		rsp->vls[vfi].port_vl_xmit_data =
			cpu_to_be64(read_csr(dd, WFR_SEND_COUNTER_ARRAY64 +
					8 * SEND_DATA_VL0_CNT + offset));
		rsp->vls[vfi].port_vl_rcv_data =
			cpu_to_be64(read_csr(dd, DCC_PRF_PORT_VL_RCV_DATA_CNT
					+ offset));
		rsp->vls[vfi].port_vl_xmit_pkts =
			cpu_to_be64(read_csr(dd, WFR_SEND_COUNTER_ARRAY64 +
					8 * SEND_DATA_PKT_VL0_CNT + offset));
		rsp->vls[vfi].port_vl_rcv_pkts =
			cpu_to_be64(read_csr(dd, DCC_PRF_PORT_VL_RCV_PKTS_CNT
					+ offset));
		rsp->vls[vfi].port_vl_xmit_wait =
			cpu_to_be64(read_csr(dd, WFR_SEND_COUNTER_ARRAY64 +
					8 * SEND_WAIT_VL0_CNT + offset));
		/* rsp->vls[vfi].sw_port_vl_congestion is 0 for HFIs */
		rsp->vls[vfi].port_vl_rcv_fecn =
			cpu_to_be64(read_csr(dd, DCC_PRF_PORT_VL_RCV_FECN_CNT
					+ offset));
		rsp->vls[vfi].port_vl_rcv_becn =
			cpu_to_be64(read_csr(dd, DCC_PRF_PORT_VL_RCV_BECN_CNT
					+ offset));
		/* rsp->port_vl_xmit_time_cong is 0 for HFIs */
		/* rsp->port_vl_xmit_wasted_bw ??? */
		/* port_vl_xmit_wait_data - TXE (table 13-9 WFR spec) ???
		 * does this differ from rsp->vls[vfi].port_vl_xmit_wait */
		rsp->vls[vfi].port_vl_rcv_bubble =
			cpu_to_be64(read_csr(dd, DCC_PRF_PORT_VL_RCV_BUBBLE_CNT
					+ offset));
		rsp->vls[vfi].port_vl_mark_fecn =
			cpu_to_be64(read_csr(dd, DCC_PRF_PORT_VL_MARK_FECN_CNT
					 + offset));
		/* rsp->vls[vfi].port_vl_xmit_discards ??? */
		vlinfo++;
		vfi++;
	}

	return reply(pmp);
}

static u64 get_error_counter_summary(struct ib_device *ibdev, u8 port)
{
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	u64 error_counter_summary = 0, tmp;
	/* FIXME
	 * some of the counters are not implemented. if the WFR spec
	 * indicates the source of the value (e.g., driver, DC, etc.)
	 * that's noted. If I don't have a clue how to get the counter,
	 * a '???' appears.
	 */

	/* port_rcv_constraint_errors ??? */
	/* port_rcv_switch_relay_errors is 0 for HFIs */
	error_counter_summary += ppd->port_xmit_discards;
	/* port_xmit_constraint_errors - driver (table 13-11 WFR spec) */
	error_counter_summary += read_csr(dd, DCC_ERR_RCVREMOTE_PHY_ERR_CNT);
	if (acquire_lcb_access(dd) == 0) {
		error_counter_summary +=
			read_csr(dd, DC_LCB_ERR_INFO_TX_REPLAY_CNT);
		release_lcb_access(dd);
	}
	error_counter_summary += read_csr(dd, DCC_ERR_PORTRCV_ERR_CNT);
	error_counter_summary += read_csr(dd, WFR_RCV_COUNTER_ARRAY32 +
					  8 * RCV_BUF_OVFL_CNT);
	error_counter_summary += read_csr(dd, DCC_ERR_FMCONFIG_ERR_CNT);
	/* link_error_recovery - DC (table 13-11 WFR spec) */
	/* ppd->link_downed is a 32-bit value */
	error_counter_summary += (u64)ppd->link_downed;
	tmp = read_csr(dd, DCC_ERR_UNCORRECTABLE_CNT);
	/* this is an 8-bit quantity */
	error_counter_summary += tmp < 0x100 ? (tmp & 0xff) : 0xff;

	return error_counter_summary;
}

static int pma_get_stl_datacounters(struct stl_pma_mad *pmp,
				    struct ib_device *ibdev, u8 port)
{
	struct stl_port_data_counters_msg *req =
		(struct stl_port_data_counters_msg *)pmp->data;
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	struct _port_dctrs *rsp;
	struct _vls_dctrs *vlinfo;
	size_t response_data_size;
	u32 num_ports;
	u8 num_pslm;
	u8 num_vls;
	u64 port_mask;
	unsigned long port_num;
	unsigned long vl;
	u32 vl_select_mask;
	int vfi;

	num_ports = be32_to_cpu(pmp->mad_hdr.attr_mod) >> 24;
	num_pslm = hweight64(be64_to_cpu(req->port_select_mask[3]));
	num_vls = hweight32(be32_to_cpu(req->vl_select_mask));
	vl_select_mask = cpu_to_be32(req->vl_select_mask);

	if (num_ports != 1 || (vl_select_mask & ~WFR_VL_MASK_ALL)) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		return reply(pmp);
	}

	/* Sanity check */
	response_data_size = sizeof(struct stl_port_data_counters_msg) +
				num_vls * sizeof(struct _vls_dctrs);

	if (response_data_size > sizeof(pmp->data)) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		return reply(pmp);
	}

	/*
	 * The bit set in the mask needs to be consistent with the
	 * port the request came in on.
	 */
	port_mask = be64_to_cpu(req->port_select_mask[3]);
	port_num = find_first_bit((unsigned long *)&port_mask,
				  sizeof(port_mask));

	if ((u8)port_num != port) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		return reply(pmp);
	}

	rsp = (struct _port_dctrs *)&(req->port[0]);
	memset(rsp, 0, sizeof(*rsp));

	rsp->port_number = port;
	/*
	 * Note that link_quality_indicator is a 32 bit quantity in
	 * 'datacounters' queries (as opposed to 'portinfo' queries,
	 * where it's a byte).
	 */
	rsp->link_quality_indicator = cpu_to_be32(ppd->link_quality);

	/* FIXME
	 * some of the counters are not implemented. if the WFR spec
	 * indicates the source of the value (e.g., driver, DC, etc.)
	 * that's noted. If I don't have a clue how to get the counter,
	 * a '???' appears.
	 */

	rsp->port_xmit_data =
		cpu_to_be64(read_csr(dd, DCC_PRF_PORT_XMIT_DATA_CNT));
	rsp->port_rcv_data =
		cpu_to_be64(read_csr(dd, DCC_PRF_PORT_RCV_DATA_CNT));
	rsp->port_xmit_pkts =
		cpu_to_be64(read_csr(dd, DCC_PRF_PORT_XMIT_PKTS_CNT));
	rsp->port_rcv_pkts =
		cpu_to_be64(read_csr(dd, DCC_PRF_PORT_RCV_PKTS_CNT));
	rsp->port_multicast_xmit_pkts =
		cpu_to_be64(read_csr(dd, DCC_PRF_PORT_XMIT_MULTICAST_CNT));
	rsp->port_multicast_rcv_pkts =
		cpu_to_be64(read_csr(dd, DCC_PRF_PORT_RCV_MULTICAST_PKT_CNT));
	rsp->port_xmit_wait =
		cpu_to_be64(read_csr(dd,
			    (SEND_WAIT_CNT * 8 + WFR_SEND_COUNTER_ARRAY64)));
	/* rsp->sw_port_congestion is 0 for HFIs */
	rsp->port_rcv_fecn =
		cpu_to_be64(read_csr(dd, DCC_PRF_PORT_RCV_FECN_CNT));
	rsp->port_rcv_becn =
		cpu_to_be64(read_csr(dd, DCC_PRF_PORT_RCV_BECN_CNT));
	/* rsp->port_xmit_time_cong is 0 for HFIs */
	/* rsp->port_xmit_wasted_bw ??? */
	/* rsp->port_xmit_wait_data ??? */
	rsp->port_rcv_bubble =
		cpu_to_be64(read_csr(dd, DCC_PRF_PORT_RCV_BUBBLE_CNT));
	rsp->port_mark_fecn =
		cpu_to_be64(read_csr(dd, DCC_PRF_PORT_MARK_FECN_CNT));

	rsp->port_error_counter_summary =
		cpu_to_be64(get_error_counter_summary(ibdev, port));

	vlinfo = &(rsp->vls[0]);
	vfi = 0;
	/* The vl_select_mask has been checked above, and we know
	 * that it contains only entries which represent valid VLs.
	 * So in the for_each_set_bit() loop below, we don't need
	 * any additional checks for vl.
	 */
	for_each_set_bit(vl, (unsigned long *)&(vl_select_mask),
		 8 * sizeof(req->vl_select_mask)) {
		unsigned offset;
		memset(vlinfo, 0, sizeof(*vlinfo));
		/* for data VLs, the byte offset from the associated VL0
		 * register is 8 * vl, but for VL15 it's 8 * 8 */
		offset = 8 * (unsigned)((vl == 15) ? 8 :  vl);
		rsp->vls[vfi].port_vl_xmit_data =
			cpu_to_be64(read_csr(dd, WFR_SEND_COUNTER_ARRAY64 +
				8 * SEND_DATA_VL0_CNT + offset));
		rsp->vls[vfi].port_vl_rcv_data =
			cpu_to_be64(read_csr(dd, DCC_PRF_PORT_VL_RCV_DATA_CNT
				+ offset));
		rsp->vls[vfi].port_vl_xmit_pkts =
			cpu_to_be64(read_csr(dd, WFR_SEND_COUNTER_ARRAY64 +
					8 * SEND_DATA_PKT_VL0_CNT + offset));
		rsp->vls[vfi].port_vl_rcv_pkts =
			cpu_to_be64(read_csr(dd, DCC_PRF_PORT_VL_RCV_PKTS_CNT
					+ offset));
		rsp->vls[vfi].port_vl_xmit_wait =
			cpu_to_be64(read_csr(dd, WFR_SEND_COUNTER_ARRAY64 +
				8 * SEND_WAIT_VL0_CNT + offset));
		/* rsp->vls[vfi].sw_port_vl_congestion is 0 for HFIs */
		rsp->vls[vfi].port_vl_rcv_fecn =
			cpu_to_be64(read_csr(dd, DCC_PRF_PORT_VL_RCV_FECN_CNT
				+ offset));
		rsp->vls[vfi].port_vl_rcv_becn =
			cpu_to_be64(read_csr(dd, DCC_PRF_PORT_VL_RCV_BECN_CNT
				+ offset));
		/* rsp->port_vl_xmit_time_cong is 0 for HFIs */
		/* rsp->port_vl_xmit_wasted_bw ??? */
		/* port_vl_xmit_wait_data - TXE (table 13-9 WFR spec) ???
		 * does this differ from rsp->vls[vfi].port_vl_xmit_wait */
		rsp->vls[vfi].port_vl_rcv_bubble =
			cpu_to_be64(read_csr(dd, DCC_PRF_PORT_VL_RCV_BUBBLE_CNT
				+ offset));
		rsp->vls[vfi].port_vl_mark_fecn =
			cpu_to_be64(read_csr(dd, DCC_PRF_PORT_VL_MARK_FECN_CNT
				+ offset));
		vlinfo++;
		vfi++;
	}

	return reply(pmp);
}

static int pma_get_stl_porterrors(struct stl_pma_mad *pmp,
				  struct ib_device *ibdev, u8 port)
{
	size_t response_data_size;
	struct _port_ectrs *rsp;
	unsigned long port_num;
	struct stl_port_error_counters64_msg *req;
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	u32 num_ports;
	u32 counter_size_mode;
	u8 num_pslm;
	u8 num_vls;
	struct qib_ibport *ibp;
	struct qib_pportdata *ppd;
	struct _vls_ectrs *vlinfo;
	unsigned long vl;
	u64 port_mask, tmp;
	u32 vl_select_mask;
	int vfi;

	req = (struct stl_port_error_counters64_msg *)pmp->data;

	num_ports = be32_to_cpu(pmp->mad_hdr.attr_mod) >> 24;
	counter_size_mode = (be32_to_cpu(pmp->mad_hdr.attr_mod) >> 22) & 0x3;

	num_pslm = hweight64(req->port_select_mask[3]);
	num_vls = hweight32(req->vl_select_mask);

	/* TODO add support for:
	 *	COUNTER_SIZE_MODE_ALL32
	 *	COUNTER_SIZE_MODE_MIXED
	 */
	if (num_ports != 1 || num_ports != num_pslm ||
		(counter_size_mode != COUNTER_SIZE_MODE_ALL64)) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		return reply(pmp);
	}

	response_data_size = sizeof(struct stl_port_error_counters64_msg) +
				num_vls * sizeof(struct _vls_ectrs);

	if (response_data_size > sizeof(pmp->data)) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		return reply(pmp);
	}
	/*
	 * The bit set in the mask needs to be consistent with the
	 * port the request came in on.
	 */
	port_mask = be64_to_cpu(req->port_select_mask[3]);
	port_num = find_first_bit((unsigned long *)&port_mask,
					sizeof(port_mask));

	if ((u8)port_num != port) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		return reply(pmp);
	}

	rsp = (struct _port_ectrs *)&(req->port[0]);

	ibp = to_iport(ibdev, port_num);
	ppd = ppd_from_ibp(ibp);

	memset(rsp, 0, sizeof(*rsp));
	rsp->port_number = (u8)port_num;

	/* FIXME
	 * some of the counters are not implemented. if the WFR spec
	 * indicates the source of the value (e.g., driver, DC, etc.)
	 * that's noted. If I don't have a clue how to get the counter,
	 * a '???' appears.
	 */

	/* rsp->port_rcv_constraint_errors = ??? */
	/* port_rcv_switch_relay_errors is 0 for HFIs */
	rsp->port_xmit_discards = cpu_to_be64(ppd->port_xmit_discards);
	/* rsp->port_xmit_constraint_errors - driver (table 13-11 WFR spec) */
	rsp->port_rcv_remote_physical_errors =
		cpu_to_be64(read_csr(dd, DCC_ERR_RCVREMOTE_PHY_ERR_CNT));
	if (acquire_lcb_access(dd) == 0) {
		rsp->local_link_integrity_errors = cpu_to_be64(
			read_csr(dd, DC_LCB_ERR_INFO_TX_REPLAY_CNT));
		release_lcb_access(dd);
	}
	rsp->excessive_buffer_overruns =
		cpu_to_be64(read_csr(dd, WFR_RCV_COUNTER_ARRAY32 +
			    8 * RCV_BUF_OVFL_CNT));
	rsp->fm_config_errors =
		cpu_to_be64(read_csr(dd, DCC_ERR_FMCONFIG_ERR_CNT));
	/* rsp->link_error_recovery - DC (table 13-11 WFR spec) */
	rsp->link_downed = cpu_to_be32(ppd->link_downed);
	tmp = read_csr(dd, DCC_ERR_UNCORRECTABLE_CNT);
	rsp->uncorrectable_errors = tmp < 0x100 ? (tmp & 0xff) : 0xff;

	vlinfo = (struct _vls_ectrs *)&(rsp->vls[0]);
	vfi = 0;
	vl_select_mask = cpu_to_be32(req->vl_select_mask);
	for_each_set_bit(vl, (unsigned long *)&(vl_select_mask),
			 8 * sizeof(req->vl_select_mask)) {
		memset(vlinfo, 0, sizeof(*vlinfo));
		/* vlinfo->vls[vfi].port_vl_xmit_discards ??? */
		vlinfo += 1;
		vfi++;
	}

	return reply(pmp);
}

static int pma_get_stl_errorinfo(struct stl_pma_mad *pmp,
				 struct ib_device *ibdev, u8 port)
{
	size_t response_data_size;
	struct _port_ei *rsp;
	struct stl_port_error_info_msg *req;
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	u64 port_mask;
	u32 num_ports;
	unsigned long port_num;
	u8 num_pslm;
	u64 reg;

	req = (struct stl_port_error_info_msg *)pmp->data;
	rsp = (struct _port_ei *)&(req->port[0]);

	num_ports = STL_AM_NPORT(be32_to_cpu(pmp->mad_hdr.attr_mod));
	num_pslm = hweight64(be64_to_cpu(req->port_select_mask[3]));

	memset(rsp, 0, sizeof(*rsp));

	if (num_ports != 1 || num_ports != num_pslm) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		return reply(pmp);
	}

	/* Sanity check */
	response_data_size = sizeof(struct stl_port_error_info_msg);

	if (response_data_size > sizeof(pmp->data)) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		return reply(pmp);
	}

	/*
	 * The bit set in the mask needs to be consistent with the port
	 * the request came in on.
	 */
	port_mask = be64_to_cpu(req->port_select_mask[3]);
	port_num = find_first_bit((unsigned long *)&port_mask,
				  sizeof(port_mask));

	if ((u8)port_num != port) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		return reply(pmp);
	}

	/* PortRcvErrorInfo */
	rsp->port_rcv_ei.status_and_code =
		dd->err_info_rcvport.status_and_code;
	memcpy(&rsp->port_rcv_ei.ei.ei1to12.packet_flit1,
		&dd->err_info_rcvport.packet_flit1, sizeof(u64));
	memcpy(&rsp->port_rcv_ei.ei.ei1to12.packet_flit2,
		&dd->err_info_rcvport.packet_flit2, sizeof(u64));

	/* ExcessiverBufferOverrunInfo */
	reg = read_csr(dd, WFR_RCV_ERR_INFO);
	if (reg & WFR_RCV_ERR_INFO_RCV_EXCESS_BUFFER_OVERRUN_SMASK) {
		/* if the RcvExcessBufferOverrun bit is set, save SC of
		 * first pkt that encountered an excess buffer overrun */
		u8 tmp = (u8)reg;
		tmp &=  WFR_RCV_ERR_INFO_RCV_EXCESS_BUFFER_OVERRUN_SC_SMASK;
		tmp <<= 2;
		rsp->excessive_buffer_overrun_ei.status_and_sc = tmp;
		/* set the status bit */
		rsp->excessive_buffer_overrun_ei.status_and_sc |= 0x80;
	}

	/* PortXmitConstraintErrorInfo */
	/* FIXME this error counter isn't implemented yet */

	/* PortRcvConstraintErrorInfo */
	/* FIXME this error counter isn't implemented yet */

	/* PortRcvSwitchRelayErrorInfo */
	/* FIXME this error counter isn't relevant to HFIs */

	/* UncorrectableErrorInfo */
	rsp->uncorrectable_ei.status_and_code = dd->err_info_uncorrectable;

	/* FMConfigErrorInfo */
	rsp->fm_config_ei.status_and_code = dd->err_info_fmconfig;

	return reply(pmp);
}

static int pma_set_stl_portstatus(struct stl_pma_mad *pmp,
				  struct ib_device *ibdev, u8 port)
{
	struct stl_clear_port_status *req =
		(struct stl_clear_port_status *)pmp->data;
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	u32 nports = be32_to_cpu(pmp->mad_hdr.attr_mod) >> 24;
	u64 portn = be64_to_cpu(req->port_select_mask[3]);
	u32 counter_select = be32_to_cpu(req->counter_select_mask);
	u32 vl_select_mask = WFR_VL_MASK_ALL; /* clear all per-vl cnts */
	unsigned long vl;
	u32 offset;

	if ((nports != 1) || (portn != 1 << port)) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		return reply(pmp);
	}
	/*
	 * only counters returned by pma_get_stl_portstatus() are
	 * handled, so when pma_get_stl_portstatus() gets a fix,
	 * the corresponding change should be made here as well.
	 */
	if (counter_select & CS_PORT_XMIT_DATA)
		write_csr(dd, DCC_PRF_PORT_XMIT_DATA_CNT, 0);

	if (counter_select & CS_PORT_RCV_DATA)
		write_csr(dd, DCC_PRF_PORT_RCV_DATA_CNT, 0);

	if (counter_select & CS_PORT_XMIT_PKTS)
		write_csr(dd, DCC_PRF_PORT_XMIT_PKTS_CNT, 0);

	if (counter_select & CS_PORT_RCV_PKTS)
		write_csr(dd, DCC_PRF_PORT_RCV_PKTS_CNT, 0);

	if (counter_select & CS_PORT_MCAST_XMIT_PKTS)
		write_csr(dd, DCC_PRF_PORT_XMIT_MULTICAST_CNT, 0);

	if (counter_select & CS_PORT_MCAST_RCV_PKTS)
		write_csr(dd, DCC_PRF_PORT_RCV_MULTICAST_PKT_CNT, 0);

	if (counter_select & CS_PORT_XMIT_WAIT) {
		offset = SEND_WAIT_CNT * 8 + WFR_SEND_COUNTER_ARRAY64;
		write_csr(dd, offset, 0);
	}

	/* ignore cs_sw_portCongestion for HFIs */

	if (counter_select & CS_PORT_RCV_FECN)
		write_csr(dd, DCC_PRF_PORT_RCV_FECN_CNT, 0);
	if (counter_select & CS_PORT_RCV_BECN)
		write_csr(dd, DCC_PRF_PORT_RCV_BECN_CNT, 0);

	/* ignore cs_port_xmit_time_cong for HFIs */
	/* ignore cs_port_xmit_wasted_bw for now */
	/* ignore cs_port_xmit_wait_data for now */
	if (counter_select & CS_PORT_RCV_BUBBLE)
		write_csr(dd, DCC_PRF_PORT_RCV_BUBBLE_CNT, 0);
	if (counter_select & CS_PORT_MARK_FECN)
		write_csr(dd, DCC_PRF_PORT_MARK_FECN_CNT, 0);
	/* ignore cs_port_rcv_constraint_errors for now */
	/* ignore cs_port_rcv_switch_relay_errors for HFIs */
	if (counter_select & CS_PORT_XMIT_DISCARDS)
		ppd->port_xmit_discards = 0;
	/* ignore cs_port_xmit_constraint_errors for now */
	if (counter_select & CS_PORT_RCV_REMOTE_PHYSICAL_ERRORS)
		write_csr(dd, DCC_ERR_RCVREMOTE_PHY_ERR_CNT, 0);
	if (counter_select & CS_LOCAL_LINK_INTEGRITY_ERRORS) {
		if (acquire_lcb_access(dd) == 0) {
			write_csr(dd, DC_LCB_ERR_INFO_TX_REPLAY_CNT, 0);
			release_lcb_access(dd);
		}
		/* else return an error? ??? */
	}
	if (counter_select & CS_PORT_RCV_ERRORS)
		write_csr(dd, DCC_ERR_PORTRCV_ERR_CNT, 0);
	if (counter_select & CS_EXCESSIVE_BUFFER_OVERRUNS)
		write_csr(dd, WFR_RCV_COUNTER_ARRAY32 +
			  8 * RCV_BUF_OVFL_CNT, 0);
	if (counter_select & CS_FM_CONFIG_ERRORS)
		write_csr(dd, DCC_ERR_FMCONFIG_ERR_CNT, 0);
	/* ignore cs_link_error_recovery for now */
	if (counter_select & CS_LINK_DOWNED)
		ppd->link_downed = 0;
	if (counter_select & CS_UNCORRECTABLE_ERRORS)
		write_csr(dd, DCC_ERR_UNCORRECTABLE_CNT, 0);

	for_each_set_bit(vl, (unsigned long *)&(vl_select_mask),
			 8 * sizeof(vl_select_mask)) {
		/* for data VLs, the byte offset from the associated VL0
		 * register is 8 * vl, but for VL15 it's 8 * 8 */
		offset = 8 * (unsigned)((vl == 15) ? 8 : vl);
		if (counter_select & CS_PORT_XMIT_DATA)
			write_csr(dd, WFR_SEND_COUNTER_ARRAY64 +
					8 * SEND_DATA_VL0_CNT + offset, 0);
		if (counter_select & CS_PORT_RCV_DATA)
			write_csr(dd, DCC_PRF_PORT_VL_RCV_DATA_CNT + offset, 0);
		if (counter_select & CS_PORT_XMIT_PKTS)
			write_csr(dd, WFR_SEND_COUNTER_ARRAY64 +
					8 * SEND_DATA_PKT_VL0_CNT + offset, 0);
		if (counter_select & CS_PORT_RCV_PKTS)
			write_csr(dd, DCC_PRF_PORT_VL_RCV_PKTS_CNT + offset, 0);
		if (counter_select & CS_PORT_XMIT_WAIT)
			write_csr(dd, WFR_SEND_COUNTER_ARRAY64 +
					8 * SEND_WAIT_VL0_CNT + offset, 0);
		/* sw_port_vl_congestion is 0 for HFIs */
		if (counter_select & CS_PORT_RCV_FECN)
			write_csr(dd, DCC_PRF_PORT_VL_RCV_FECN_CNT + offset, 0);
		if (counter_select & CS_PORT_RCV_BECN)
			write_csr(dd, DCC_PRF_PORT_VL_RCV_BECN_CNT + offset, 0);
		/* port_vl_xmit_time_cong is 0 for HFIs */
		/* port_vl_xmit_wasted_bw ??? */
		/* port_vl_xmit_wait_data - TXE (table 13-9 WFR spec) ??? */
		if (counter_select & CS_PORT_RCV_BUBBLE)
			write_csr(dd, DCC_PRF_PORT_VL_RCV_BUBBLE_CNT + offset, 0);
		if (counter_select & CS_PORT_MARK_FECN)
			write_csr(dd, DCC_PRF_PORT_VL_MARK_FECN_CNT + offset, 0);
		/* port_vl_xmit_discards ??? */
	}

	return reply(pmp);
}

static int pma_set_stl_errorinfo(struct stl_pma_mad *pmp,
				 struct ib_device *ibdev, u8 port)
{
	struct _port_ei *rsp;
	struct stl_port_error_info_msg *req;
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	u64 port_mask;
	u32 num_ports;
	unsigned long port_num;
	u8 num_pslm;
	u32 error_info_select;

	req = (struct stl_port_error_info_msg *)pmp->data;
	rsp = (struct _port_ei *)&(req->port[0]);

	num_ports = STL_AM_NPORT(be32_to_cpu(pmp->mad_hdr.attr_mod));
	num_pslm = hweight64(be64_to_cpu(req->port_select_mask[3]));

	memset(rsp, 0, sizeof(*rsp));

	if (num_ports != 1 || num_ports != num_pslm) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		return reply(pmp);
	}

	/*
	 * The bit set in the mask needs to be consistent with the port
	 * the request came in on.
	 */
	port_mask = be64_to_cpu(req->port_select_mask[3]);
	port_num = find_first_bit((unsigned long *)&port_mask,
				  sizeof(port_mask));

	if ((u8)port_num != port) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		return reply(pmp);
	}

	error_info_select = be32_to_cpu(req->error_info_select_mask);

	/* PortRcvErrorInfo */
	if (error_info_select & ES_PORT_RCV_ERROR_INFO)
		/* turn off status bit */
		dd->err_info_rcvport.status_and_code &= ~0x80;

	/* ExcessiverBufferOverrunInfo */
	if (error_info_select & ES_EXCESSIVE_BUFFER_OVERRUN_INFO)
		/* status bit is essentially kept in the h/w - bit 5 of
		 * WFR_RCV_ERR_INFO */
		write_csr(dd, WFR_RCV_ERR_INFO,
			  WFR_RCV_ERR_INFO_RCV_EXCESS_BUFFER_OVERRUN_SMASK);

	/* PortXmitConstraintErrorInfo */
	/* FIXME this error counter isn't implemented yet */

	/* PortRcvConstraintErrorInfo */
	/* FIXME this error counter isn't implemented yet */

	/* PortRcvSwitchRelayErrorInfo */
	/* FIXME this error counter isn't relevant to HFIs */

	/* UncorrectableErrorInfo */
	if (error_info_select & ES_UNCORRECTABLE_ERROR_INFO)
		/* turn off status bit */
		dd->err_info_uncorrectable &= ~0x80;

	/* FMConfigErrorInfo */
	if (error_info_select & ES_FM_CONFIG_ERROR_INFO)
		/* turn off status bit */
		dd->err_info_fmconfig &= ~0x80;

	return reply(pmp);
}

static u64 get_counter(struct qib_ibport *ibp, struct qib_pportdata *ppd,
		       __be16 sel)
{
	u64 ret;

	switch (sel) {
	case IB_PMA_PORT_XMIT_DATA:
		ret = ppd->dd->f_portcntr(ppd, QIBPORTCNTR_PSXMITDATA);
		break;
	case IB_PMA_PORT_RCV_DATA:
		ret = ppd->dd->f_portcntr(ppd, QIBPORTCNTR_PSRCVDATA);
		break;
	case IB_PMA_PORT_XMIT_PKTS:
		ret = ppd->dd->f_portcntr(ppd, QIBPORTCNTR_PSXMITPKTS);
		break;
	case IB_PMA_PORT_RCV_PKTS:
		ret = ppd->dd->f_portcntr(ppd, QIBPORTCNTR_PSRCVPKTS);
		break;
	case IB_PMA_PORT_XMIT_WAIT:
		ret = ppd->dd->f_portcntr(ppd, QIBPORTCNTR_PSXMITWAIT);
		break;
	default:
		ret = 0;
	}

	return ret;
}

/* This function assumes that the xmit_wait lock is already held */
static u64 xmit_wait_get_value_delta(struct qib_pportdata *ppd)
{
	u32 delta;

	delta = get_counter(&ppd->ibport_data, ppd,
			    IB_PMA_PORT_XMIT_WAIT);
	return ppd->cong_stats.counter + delta;
}

static void cache_hw_sample_counters(struct qib_pportdata *ppd)
{
	struct qib_ibport *ibp = &ppd->ibport_data;

	ppd->cong_stats.counter_cache.psxmitdata =
		get_counter(ibp, ppd, IB_PMA_PORT_XMIT_DATA);
	ppd->cong_stats.counter_cache.psrcvdata =
		get_counter(ibp, ppd, IB_PMA_PORT_RCV_DATA);
	ppd->cong_stats.counter_cache.psxmitpkts =
		get_counter(ibp, ppd, IB_PMA_PORT_XMIT_PKTS);
	ppd->cong_stats.counter_cache.psrcvpkts =
		get_counter(ibp, ppd, IB_PMA_PORT_RCV_PKTS);
	ppd->cong_stats.counter_cache.psxmitwait =
		get_counter(ibp, ppd, IB_PMA_PORT_XMIT_WAIT);
}

struct stl_congestion_info_attr {
	__be16 congestion_info;
	u8 control_table_cap;	/* Multiple of 64 entry unit CCTs */
	u8 congestion_log_length;
} __packed;

static int __subn_get_stl_cong_info(struct stl_smp *smp, u32 am, u8 *data,
				    struct ib_device *ibdev, u8 port,
				    u32 *resp_len)
{
	struct stl_congestion_info_attr *p =
		(struct stl_congestion_info_attr *)data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);

	clear_stl_smp_data(smp);

	p->congestion_info = 0;
	p->control_table_cap = ppd->cc_max_table_entries;
	p->congestion_log_length = 0; /* FIXME */

	if (resp_len)
		*resp_len += sizeof(*p);

	return reply(smp);
}

static int __subn_get_stl_cong_setting(struct stl_smp *smp, u32 am,
					     u8 *data,
					     struct ib_device *ibdev,
					     u8 port, u32 *resp_len)
{
	int i;
	struct stl_congestion_setting_attr *p =
		(struct stl_congestion_setting_attr *) data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	struct stl_congestion_setting_entry_shadow *entries;
	struct cc_state *cc_state;

	clear_stl_smp_data(smp);

	rcu_read_lock();

	cc_state = get_cc_state(ppd);

	if (cc_state == NULL) {
		rcu_read_unlock();
		return reply(smp);
	}

	entries = cc_state->cong_setting.entries;
	p->port_control = cpu_to_be16(cc_state->cong_setting.port_control);
	p->control_map = cpu_to_be32(cc_state->cong_setting.control_map);
	for (i = 0; i < STL_MAX_SLS; i++) {
		p->entries[i].ccti_increase = entries[i].ccti_increase;
		p->entries[i].ccti_timer = cpu_to_be16(entries[i].ccti_timer);
		p->entries[i].trigger_threshold =
			entries[i].trigger_threshold;
		p->entries[i].ccti_min = entries[i].ccti_min;
	}

	rcu_read_unlock();

	if (resp_len)
		*resp_len += sizeof(*p);

	return reply(smp);
}

static int __subn_set_stl_cong_setting(struct stl_smp *smp, u32 am, u8 *data,
				       struct ib_device *ibdev, u8 port,
				       u32 *resp_len)
{
	struct stl_congestion_setting_attr *p =
		(struct stl_congestion_setting_attr *) data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	struct stl_congestion_setting_entry_shadow *entries;
	int i;

	ppd->cc_sl_control_map = be32_to_cpu(p->control_map);

	entries = ppd->congestion_entries;
	for (i = 0; i < STL_MAX_SLS; i++) {
		entries[i].ccti_increase = p->entries[i].ccti_increase;
		entries[i].ccti_timer = be16_to_cpu(p->entries[i].ccti_timer);
		entries[i].trigger_threshold =
			p->entries[i].trigger_threshold;
		entries[i].ccti_min = p->entries[i].ccti_min;
	}

	return __subn_get_stl_cong_setting(smp, am, data, ibdev, port,
					   resp_len);
}

static int __subn_get_stl_cc_table(struct stl_smp *smp, u32 am, u8 *data,
				   struct ib_device *ibdev, u8 port,
				   u32 *resp_len)
{
	struct ib_cc_table_attr *cc_table_attr =
		(struct ib_cc_table_attr *) data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	u32 start_block = STL_AM_START_BLK(am);
	u32 n_blocks = STL_AM_NBLK(am);
	struct ib_cc_table_entry_shadow *entries;
	int i, j;
	u32 sentry, eentry;
	struct cc_state *cc_state;

	clear_stl_smp_data(smp);

	/* sanity check n_blocks, start_block */
	if (n_blocks == 0 ||
	    start_block + n_blocks > ppd->cc_max_table_entries) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}

	rcu_read_lock();

	cc_state = get_cc_state(ppd);

	if (cc_state == NULL) {
		rcu_read_unlock();
		return reply(smp);
	}

	sentry = start_block * IB_CCT_ENTRIES;
	eentry = sentry + (IB_CCT_ENTRIES * n_blocks);

	cc_table_attr->ccti_limit = cpu_to_be16(cc_state->cct.ccti_limit);

	entries = cc_state->cct.entries;

	/* return n_blocks, though the last block may not be full */
	for (j = 0, i = sentry; i < eentry; j++, i++)
		cc_table_attr->ccti_entries[j].entry =
			cpu_to_be16(entries[i].entry);

	rcu_read_unlock();

	if (resp_len)
		*resp_len += sizeof(u16)*(IB_CCT_ENTRIES * n_blocks + 1);

	return reply(smp);
}

static void cc_state_reclaim(struct rcu_head *rcu)
{
	struct cc_state *cc_state = container_of(rcu, struct cc_state, rcu);
	kfree(cc_state);
}

static int __subn_set_stl_cc_table(struct stl_smp *smp, u32 am, u8 *data,
				   struct ib_device *ibdev, u8 port,
				   u32 *resp_len)
{
	struct ib_cc_table_attr *p = (struct ib_cc_table_attr *) data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	u32 start_block = STL_AM_START_BLK(am);
	u32 n_blocks = STL_AM_NBLK(am);
	struct ib_cc_table_entry_shadow *entries;
	int i, j;
	u32 sentry, eentry;
	u16 ccti_limit;
	struct cc_state *old_cc_state, *new_cc_state;

	/* sanity check n_blocks, start_block */
	if (n_blocks == 0 ||
	    start_block + n_blocks > ppd->cc_max_table_entries) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}

	sentry = start_block * IB_CCT_ENTRIES;
	eentry = sentry + ((n_blocks - 1) * IB_CCT_ENTRIES) +
		 (be16_to_cpu(p->ccti_limit)) % IB_CCT_ENTRIES + 1;

	/* sanity check ccti_limit */
	ccti_limit = be16_to_cpu(p->ccti_limit);
	if (ccti_limit + 1 > eentry) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}

	new_cc_state = kzalloc(sizeof(*new_cc_state), GFP_KERNEL);
	if (new_cc_state == NULL)
		goto getit;

	spin_lock(&ppd->cc_state_lock);

	old_cc_state = get_cc_state(ppd);

	if (old_cc_state == NULL) {
		spin_unlock(&ppd->cc_state_lock);
		return reply(smp);
	}

	*new_cc_state = *old_cc_state;

	new_cc_state->cct.ccti_limit = ccti_limit;

	entries = ppd->ccti_entries;
	ppd->total_cct_entry = (ccti_limit % IB_CCT_ENTRIES) + 1;

	for (j = 0, i = sentry; i < eentry; j++, i++)
		entries[i].entry = be16_to_cpu(p->ccti_entries[j].entry);

	memcpy(new_cc_state->cct.entries, entries,
	       eentry * sizeof(struct ib_cc_table_entry));

	new_cc_state->cong_setting.port_control = IB_CC_CCS_PC_SL_BASED;
	new_cc_state->cong_setting.control_map = ppd->cc_sl_control_map;
	memcpy(new_cc_state->cong_setting.entries, ppd->congestion_entries,
	       STL_MAX_SLS * sizeof(struct stl_congestion_setting_entry));

	rcu_assign_pointer(ppd->cc_state, new_cc_state);

	spin_unlock(&ppd->cc_state_lock);

	call_rcu(&old_cc_state->rcu, cc_state_reclaim);

getit:
	return __subn_get_stl_cc_table(smp, am, data, ibdev, port, resp_len);
}

struct stl_led_info {
	__be32 rsvd_led_mask;
};

#define STL_LED_SHIFT	31
#define STL_LED_MASK	(1 << STL_LED_SHIFT)

static int __subn_get_stl_led_info(struct stl_smp *smp, u32 am, u8 *data,
				   struct ib_device *ibdev, u8 port,
				   u32 *resp_len)
{
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	struct stl_led_info *p = (struct stl_led_info *) data;
	u32 nport = STL_AM_NPORT(am);
	u64 reg;

	clear_stl_smp_data(smp);

	if (port != 1 || nport != 1 || STL_AM_PORTNUM(am)) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}

	reg = cpu_to_be64(read_csr(dd, DCC_CFG_LED_CNTRL));
	if ((reg & DCC_CFG_LED_CNTRL_LED_CNTRL_SMASK) &&
		((reg & DCC_CFG_LED_CNTRL_LED_SW_BLINK_RATE_SMASK) == 0xf))
			p->rsvd_led_mask = cpu_to_be32(STL_LED_MASK);

	if (resp_len)
		*resp_len += sizeof(struct stl_led_info);

	return reply(smp);
}

static int __subn_set_stl_led_info(struct stl_smp *smp, u32 am, u8 *data,
				   struct ib_device *ibdev, u8 port,
				   u32 *resp_len)
{
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	struct stl_led_info *p = (struct stl_led_info *) data;
	u32 nport = STL_AM_NPORT(am);
	int on = !!(be32_to_cpu(p->rsvd_led_mask) & STL_LED_MASK);
	u64 reg, rate = 0;

	if (port != 1 || nport != 1 || STL_AM_PORTNUM(am)) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}

	if (on)
		rate = 0xf;

	reg = DCC_CFG_LED_CNTRL_LED_CNTRL_SMASK |
		rate << DCC_CFG_LED_CNTRL_LED_SW_BLINK_RATE_SHIFT;

	write_csr(dd, DCC_CFG_LED_CNTRL, reg);

	return __subn_get_stl_led_info(smp, am, data, ibdev, port, resp_len);
}

static int subn_get_stl_sma(u16 attr_id, struct stl_smp *smp, u32 am,
			    u8 *data, struct ib_device *ibdev, u8 port,
			    u32 *resp_len)
{
	int ret;
	struct qib_ibport *ibp = to_iport(ibdev, port);

	switch (attr_id) {
	case IB_SMP_ATTR_NODE_DESC:
		ret = __subn_get_stl_nodedesc(smp, am, data, ibdev, port,
					      resp_len);
		break;
	case IB_SMP_ATTR_NODE_INFO:
		ret = __subn_get_stl_nodeinfo(smp, am, data, ibdev, port,
					      resp_len);
		break;
	case IB_SMP_ATTR_PORT_INFO:
		ret = __subn_get_stl_portinfo(smp, am, data, ibdev, port,
					      resp_len);
		break;
	case IB_SMP_ATTR_PKEY_TABLE:
		ret = __subn_get_stl_pkeytable(smp, am, data, ibdev, port,
					       resp_len);
		break;
	case STL_ATTRIB_ID_SL_TO_SC_MAP:
		ret = __subn_get_stl_sl_to_sc(smp, am, data, ibdev, port,
					      resp_len);
		break;
	case STL_ATTRIB_ID_SC_TO_SL_MAP:
		ret = __subn_get_stl_sc_to_sl(smp, am, data, ibdev, port,
					      resp_len);
		break;
	case STL_ATTRIB_ID_SC_TO_VLT_MAP:
		ret = __subn_get_stl_sc_to_vlt(smp, am, data, ibdev, port,
					       resp_len);
		break;
	case STL_ATTRIB_ID_SC_TO_VLNT_MAP:
		ret = __subn_get_stl_sc_to_vlnt(smp, am, data, ibdev, port,
					       resp_len);
		break;
	case STL_ATTRIB_ID_PORT_STATE_INFO:
		ret = __subn_get_stl_psi(smp, am, data, ibdev, port,
					 resp_len);
		break;
	case STL_ATTRIB_ID_BUFFER_CONTROL_TABLE:
		ret = __subn_get_stl_bct(smp, am, data, ibdev, port,
					 resp_len);
		break;
	case IB_SMP_ATTR_VL_ARB_TABLE:
		ret = __subn_get_stl_vl_arb(smp, am, data, ibdev, port,
					    resp_len);
		break;
	case STL_ATTRIB_ID_CONGESTION_INFO:
		ret = __subn_get_stl_cong_info(smp, am, data, ibdev, port,
					       resp_len);
		break;
	case STL_ATTRIB_ID_HFI_CONGESTION_SETTING:
		ret = __subn_get_stl_cong_setting(smp, am, data, ibdev,
						  port, resp_len);
		break;
	case STL_ATTRIB_ID_CONGESTION_CONTROL_TABLE:
		ret = __subn_get_stl_cc_table(smp, am, data, ibdev, port,
					      resp_len);
		break;
	case IB_SMP_ATTR_LED_INFO:
		ret = __subn_get_stl_led_info(smp, am, data, ibdev, port,
					      resp_len);
		break;
	case IB_SMP_ATTR_SM_INFO:
		if (ibp->port_cap_flags & IB_PORT_SM_DISABLED)
			return IB_MAD_RESULT_SUCCESS | IB_MAD_RESULT_CONSUMED;
		if (ibp->port_cap_flags & IB_PORT_SM)
			return IB_MAD_RESULT_SUCCESS;
		/* FALLTHROUGH */
	default:
		smp->status |= IB_SMP_UNSUP_METH_ATTR;
		ret = reply(smp);
		break;
	}
	return ret;
}

static int subn_set_stl_sma(u16 attr_id, struct stl_smp *smp, u32 am,
			    u8 *data, struct ib_device *ibdev, u8 port,
			    u32 *resp_len)
{
	int ret;
	struct qib_ibport *ibp = to_iport(ibdev, port);

	switch (attr_id) {
	case IB_SMP_ATTR_PORT_INFO:
		ret = __subn_set_stl_portinfo(smp, am, data, ibdev, port,
					      resp_len);
		break;
	case IB_SMP_ATTR_PKEY_TABLE:
		ret = __subn_set_stl_pkeytable(smp, am, data, ibdev, port,
					       resp_len);
		break;
	case STL_ATTRIB_ID_SL_TO_SC_MAP:
		ret = __subn_set_stl_sl_to_sc(smp, am, data, ibdev, port,
					      resp_len);
		break;
	case STL_ATTRIB_ID_SC_TO_SL_MAP:
		ret = __subn_set_stl_sc_to_sl(smp, am, data, ibdev, port,
					      resp_len);
		break;
	case STL_ATTRIB_ID_SC_TO_VLT_MAP:
		ret = __subn_set_stl_sc_to_vlt(smp, am, data, ibdev, port,
					       resp_len);
		break;
	case STL_ATTRIB_ID_SC_TO_VLNT_MAP:
		ret = __subn_set_stl_sc_to_vlnt(smp, am, data, ibdev, port,
					       resp_len);
		break;
	case STL_ATTRIB_ID_PORT_STATE_INFO:
		ret = __subn_set_stl_psi(smp, am, data, ibdev, port,
					 resp_len);
		break;
	case STL_ATTRIB_ID_BUFFER_CONTROL_TABLE:
		ret = __subn_set_stl_bct(smp, am, data, ibdev, port,
					 resp_len);
		break;
	case IB_SMP_ATTR_VL_ARB_TABLE:
		ret = __subn_set_stl_vl_arb(smp, am, data, ibdev, port,
					    resp_len);
		break;
	case STL_ATTRIB_ID_HFI_CONGESTION_SETTING:
		ret = __subn_set_stl_cong_setting(smp, am, data, ibdev,
						  port, resp_len);
		break;
	case STL_ATTRIB_ID_CONGESTION_CONTROL_TABLE:
		ret = __subn_set_stl_cc_table(smp, am, data, ibdev, port,
					      resp_len);
		break;
	case IB_SMP_ATTR_LED_INFO:
		ret = __subn_set_stl_led_info(smp, am, data, ibdev, port,
					      resp_len);
		break;
	case IB_SMP_ATTR_SM_INFO:
		if (ibp->port_cap_flags & IB_PORT_SM_DISABLED)
			return IB_MAD_RESULT_SUCCESS | IB_MAD_RESULT_CONSUMED;
		if (ibp->port_cap_flags & IB_PORT_SM)
			return IB_MAD_RESULT_SUCCESS;
		/* FALLTHROUGH */
	default:
		smp->status |= IB_SMP_UNSUP_METH_ATTR;
		ret = reply(smp);
		break;
	}
	return ret;
}

static inline void set_aggr_error(struct stl_aggregate *ag)
{
	ag->err_reqlength |= cpu_to_be16(0x8000);
}

static int subn_get_stl_aggregate(struct stl_smp *smp,
				  struct ib_device *ibdev, u8 port,
				  u32 *resp_len)
{
	int i;
	u32 num_attr = be32_to_cpu(smp->attr_mod) & 0x000000ff;
	u8 *next_smp = stl_get_smp_data(smp);

	if (num_attr < 1 || num_attr > 117) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}

	for (i = 0; i < num_attr; i++) {
		struct stl_aggregate *agg;
		size_t agg_data_len;
		size_t agg_size;
		u32 am;

		agg = (struct stl_aggregate *)next_smp;
		agg_data_len = (be16_to_cpu(agg->err_reqlength) & 0x007f) * 8;
		agg_size = sizeof(*agg) + agg_data_len;
		am = be32_to_cpu(agg->attr_mod);

		*resp_len += agg_size;

		if (next_smp + agg_size > ((u8 *)smp) + sizeof(*smp)) {
			smp->status |= IB_SMP_INVALID_FIELD;
			return reply(smp);
		}

		(void) subn_get_stl_sma(agg->attr_id, smp, am, agg->data,
					ibdev, port, NULL);
		if (smp->status & ~IB_SMP_DIRECTION) {
			set_aggr_error(agg);
			return reply(smp);
		}
		next_smp += agg_size;

	}

	return reply(smp);
}

static int subn_set_stl_aggregate(struct stl_smp *smp,
				  struct ib_device *ibdev, u8 port,
				  u32 *resp_len)
{
	int i;
	u32 num_attr = be32_to_cpu(smp->attr_mod) & 0x000000ff;
	u8 *next_smp = stl_get_smp_data(smp);

	if (num_attr < 1 || num_attr > 117) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}

	for (i = 0; i < num_attr; i++) {
		struct stl_aggregate *agg;
		size_t agg_data_len;
		size_t agg_size;
		u32 am;

		agg = (struct stl_aggregate *)next_smp;
		agg_data_len = (be16_to_cpu(agg->err_reqlength) & 0x007f) * 8;
		agg_size = sizeof(*agg) + agg_data_len;
		am = be32_to_cpu(agg->attr_mod);

		*resp_len += agg_size;

		if (next_smp + agg_size > ((u8 *)smp) + sizeof(*smp)) {
			smp->status |= IB_SMP_INVALID_FIELD;
			return reply(smp);
		}

		(void) subn_set_stl_sma(agg->attr_id, smp, am, agg->data,
					ibdev, port, NULL);
		if (smp->status & ~IB_SMP_DIRECTION) {
			set_aggr_error(agg);
			return reply(smp);
		}
		next_smp += agg_size;

	}

	return reply(smp);
}

static int process_subn_stl(struct ib_device *ibdev, int mad_flags,
			    u8 port, struct jumbo_mad *in_mad,
			    struct jumbo_mad *out_mad,
			    u32 *resp_len)
{
	struct stl_smp *smp = (struct stl_smp *)out_mad;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	/* XXX struct qib_pportdata *ppd = ppd_from_ibp(ibp);*/
	u8 *data;
	u32 am;
	__be16 attr_id;
	int ret;

	*out_mad = *in_mad;
	data = stl_get_smp_data(smp);

	am = be32_to_cpu(smp->attr_mod);
	attr_id = smp->attr_id;
	if (smp->class_version != STL_SMI_CLASS_VERSION) {
		smp->status |= IB_SMP_UNSUP_VERSION;
		ret = reply(smp);
		goto bail;
	}
	ret = check_mkey(ibp, (struct ib_mad_hdr *)smp, mad_flags, smp->mkey,
			 smp->route.dr.dr_slid, smp->route.dr.return_path,
			 smp->hop_cnt);
	if (ret) {
		u32 port_num = be32_to_cpu(smp->attr_mod);

		/*
		 * If this is a get/set portinfo, we already check the
		 * M_Key if the MAD is for another port and the M_Key
		 * is OK on the receiving port. This check is needed
		 * to increment the error counters when the M_Key
		 * fails to match on *both* ports.
		 */
		if (attr_id == IB_SMP_ATTR_PORT_INFO &&
		    (smp->method == IB_MGMT_METHOD_GET ||
		     smp->method == IB_MGMT_METHOD_SET) &&
		    port_num && port_num <= ibdev->phys_port_cnt &&
		    port != port_num)
			(void) check_mkey(to_iport(ibdev, port_num),
					  (struct ib_mad_hdr *)smp, 0,
					  smp->mkey, smp->route.dr.dr_slid,
					  smp->route.dr.return_path,
					  smp->hop_cnt);
		ret = IB_MAD_RESULT_FAILURE;
		goto bail;
	}

	*resp_len = stl_get_smp_header_size(smp);

	switch (smp->method) {
	case IB_MGMT_METHOD_GET:
		switch (attr_id) {
		default:
			ret = subn_get_stl_sma(attr_id, smp, am, data,
					       ibdev, port, resp_len);
			goto bail;
		case STL_ATTRIB_ID_AGGREGATE:
			ret = subn_get_stl_aggregate(smp, ibdev, port,
						     resp_len);
			goto bail;
		}
	case IB_MGMT_METHOD_SET:
		switch (attr_id) {
		default:
			ret = subn_set_stl_sma(attr_id, smp, am, data,
					       ibdev, port, resp_len);
			goto bail;
		case STL_ATTRIB_ID_AGGREGATE:
			ret = subn_set_stl_aggregate(smp, ibdev, port,
						     resp_len);
			goto bail;
		}
#if 0
	case IB_MGMT_METHOD_TRAP_REPRESS:
		if (smp->attr_id == IB_SMP_ATTR_NOTICE)
			ret = subn_trap_repress(smp, ibdev, port);
		else {
			smp->status |= IB_SMP_UNSUP_METH_ATTR;
			ret = reply(smp);
		}
		goto bail;
#endif /* 01 */
	case IB_MGMT_METHOD_TRAP:
	case IB_MGMT_METHOD_REPORT:
	case IB_MGMT_METHOD_REPORT_RESP:
	case IB_MGMT_METHOD_GET_RESP:
		/*
		 * The ib_mad module will call us to process responses
		 * before checking for other consumers.
		 * Just tell the caller to process it normally.
		 */
		ret = IB_MAD_RESULT_SUCCESS;
		goto bail;
#if 0
	case IB_MGMT_METHOD_SEND:
		if (ib_get_smp_direction(smp) &&
		    smp->attr_id == QIB_VENDOR_IPG) {
			ppd->dd->f_set_ib_cfg(ppd, QIB_IB_CFG_PORT,
					      smp->data[0]);
			ret = IB_MAD_RESULT_SUCCESS | IB_MAD_RESULT_CONSUMED;
		} else
			ret = IB_MAD_RESULT_SUCCESS;
		goto bail;

#endif /* 0 */
	default:
		smp->status |= IB_SMP_UNSUP_METHOD;
		ret = reply(smp);
	}

bail:
	return ret;
}

static int process_subn(struct ib_device *ibdev, int mad_flags,
			u8 port, struct ib_mad *in_mad,
			struct ib_mad *out_mad)
{
	struct ib_smp *smp = (struct ib_smp *)out_mad;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	int ret;

	*out_mad = *in_mad;
	if (smp->class_version != 1) {
		smp->status |= IB_SMP_UNSUP_VERSION;
		ret = reply(smp);
		goto bail;
	}

	ret = check_mkey(ibp, (struct ib_mad_hdr *)smp, mad_flags,
			 smp->mkey, smp->dr_slid, smp->return_path,
			 smp->hop_cnt);
	if (ret) {
		u32 port_num = be32_to_cpu(smp->attr_mod);

		/*
		 * If this is a get/set portinfo, we already check the
		 * M_Key if the MAD is for another port and the M_Key
		 * is OK on the receiving port. This check is needed
		 * to increment the error counters when the M_Key
		 * fails to match on *both* ports.
		 */
		if (in_mad->mad_hdr.attr_id == IB_SMP_ATTR_PORT_INFO &&
		    (smp->method == IB_MGMT_METHOD_GET ||
		     smp->method == IB_MGMT_METHOD_SET) &&
		    port_num && port_num <= ibdev->phys_port_cnt &&
		    port != port_num)
			(void) check_mkey(to_iport(ibdev, port_num),
					  (struct ib_mad_hdr *)smp, 0,
					  smp->mkey, smp->dr_slid,
					  smp->return_path, smp->hop_cnt);
		ret = IB_MAD_RESULT_FAILURE;
		goto bail;
	}

	switch (smp->method) {
	case IB_MGMT_METHOD_GET:
		switch (smp->attr_id) {
		case IB_SMP_ATTR_NODE_INFO:
			ret = subn_get_nodeinfo(smp, ibdev, port);
			goto bail;
		default:
			smp->status |= IB_SMP_UNSUP_METH_ATTR;
			ret = reply(smp);
			goto bail;
		}
	}

bail:
	return ret;
}

static int process_perf_stl(struct ib_device *ibdev, u8 port,
			    struct jumbo_mad *in_mad,
			    struct jumbo_mad *out_mad, u32 *resp_len)
{
	struct stl_pma_mad *pmp = (struct stl_pma_mad *)out_mad;
	int ret;

	*out_mad = *in_mad;

	if (pmp->mad_hdr.class_version != STL_SMI_CLASS_VERSION) {
		pmp->mad_hdr.status |= IB_SMP_UNSUP_VERSION;
		return reply(pmp);
	}

	/* FIXME decide proper length for all these */
	*resp_len = sizeof(struct jumbo_mad);

	switch (pmp->mad_hdr.method) {
	case IB_MGMT_METHOD_GET:
		switch (pmp->mad_hdr.attr_id) {
		case IB_PMA_CLASS_PORT_INFO:
			ret = pma_get_stl_classportinfo(pmp, ibdev);
			goto bail;
		case STL_PM_ATTRIB_ID_PORT_STATUS:
			ret = pma_get_stl_portstatus(pmp, ibdev, port);
			goto bail;
		case STL_PM_ATTRIB_ID_DATA_PORT_COUNTERS:
			ret = pma_get_stl_datacounters(pmp, ibdev, port);
			goto bail;
		case STL_PM_ATTRIB_ID_ERROR_PORT_COUNTERS:
			ret = pma_get_stl_porterrors(pmp, ibdev, port);
			goto bail;
		case STL_PM_ATTRIB_ID_ERROR_INFO:
			ret = pma_get_stl_errorinfo(pmp, ibdev, port);
			goto bail;
#if 0
		case IB_PMA_PORT_SAMPLES_CONTROL:
			ret = pma_get_portsamplescontrol(pmp, ibdev, port);
			goto bail;
		case IB_PMA_PORT_SAMPLES_RESULT:
			ret = pma_get_portsamplesresult(pmp, ibdev, port);
			goto bail;
		case IB_PMA_PORT_SAMPLES_RESULT_EXT:
			ret = pma_get_portsamplesresult_ext(pmp, ibdev, port);
			goto bail;
		case IB_PMA_PORT_COUNTERS:
			ret = pma_get_portcounters(pmp, ibdev, port);
			goto bail;
		case IB_PMA_PORT_COUNTERS_EXT:
			ret = pma_get_portcounters_ext(pmp, ibdev, port);
			goto bail;
		case IB_PMA_PORT_COUNTERS_CONG:
			ret = pma_get_portcounters_cong(pmp, ibdev, port);
			goto bail;
#endif /* 01 */
		default:
			pmp->mad_hdr.status |= IB_SMP_UNSUP_METH_ATTR;
			ret = reply(pmp);
			goto bail;
		}

	case IB_MGMT_METHOD_SET:
		switch (pmp->mad_hdr.attr_id) {
		case STL_PM_ATTRIB_ID_CLEAR_PORT_STATUS:
			ret = pma_set_stl_portstatus(pmp, ibdev, port);
			goto bail;
		case STL_PM_ATTRIB_ID_ERROR_INFO:
			ret = pma_set_stl_errorinfo(pmp, ibdev, port);
			goto bail;
#if 0
		case IB_PMA_PORT_SAMPLES_CONTROL:
			ret = pma_set_portsamplescontrol(pmp, ibdev, port);
			goto bail;
		case IB_PMA_PORT_COUNTERS:
			ret = pma_set_portcounters(pmp, ibdev, port);
			goto bail;
		case IB_PMA_PORT_COUNTERS_EXT:
			ret = pma_set_portcounters_ext(pmp, ibdev, port);
			goto bail;
		case IB_PMA_PORT_COUNTERS_CONG:
			ret = pma_set_portcounters_cong(pmp, ibdev, port);
			goto bail;
#endif /* 01 */
		default:
			pmp->mad_hdr.status |= IB_SMP_UNSUP_METH_ATTR;
			ret = reply(pmp);
			goto bail;
		}

	case IB_MGMT_METHOD_TRAP:
	case IB_MGMT_METHOD_GET_RESP:
		/*
		 * The ib_mad module will call us to process responses
		 * before checking for other consumers.
		 * Just tell the caller to process it normally.
		 */
		ret = IB_MAD_RESULT_SUCCESS;
		goto bail;

	default:
		pmp->mad_hdr.status |= IB_SMP_UNSUP_METHOD;
		ret = reply(pmp);
	}

bail:
	return ret;
}

static int hfi_process_stl_mad(struct ib_device *ibdev, int mad_flags,
			       u8 port, struct ib_wc *in_wc,
			       struct ib_grh *in_grh,
			       struct jumbo_mad *in_mad,
			       struct jumbo_mad *out_mad)
{
	int ret;
	int pkey_idx;
	u32 resp_len = 0;
	struct qib_ibport *ibp = to_iport(ibdev, port);

	pkey_idx = wfr_lookup_pkey_idx(ibp, WFR_LIM_MGMT_P_KEY);
	if (pkey_idx < 0) {
		pr_warn("failed to find limited mgmt pkey, defaulting 0x%x\n",
			qib_get_pkey(ibp, 1));
		pkey_idx = 1;
	}
	in_wc->pkey_index = (u16)pkey_idx;

	switch (in_mad->mad_hdr.mgmt_class) {
	case IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE:
	case IB_MGMT_CLASS_SUBN_LID_ROUTED:
		ret = process_subn_stl(ibdev, mad_flags, port, in_mad,
				       out_mad, &resp_len);
		goto bail;
	case IB_MGMT_CLASS_PERF_MGMT:
		ret = process_perf_stl(ibdev, port, in_mad, out_mad,
				       &resp_len);
		goto bail;

	default:
		ret = IB_MAD_RESULT_SUCCESS;
	}

bail:
	if (ret & IB_MAD_RESULT_REPLY)
		in_wc->byte_len = round_up(resp_len, 8);
	else if (ret & IB_MAD_RESULT_SUCCESS)
		in_wc->byte_len -= sizeof(struct ib_grh);

	return ret;
}

static int hfi_process_ib_mad(struct ib_device *ibdev, int mad_flags, u8 port,
			      struct ib_wc *in_wc, struct ib_grh *in_grh,
			      struct ib_mad *in_mad, struct ib_mad *out_mad)
{
	int ret;

	switch (in_mad->mad_hdr.mgmt_class) {
	case IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE:
	case IB_MGMT_CLASS_SUBN_LID_ROUTED:
		ret = process_subn(ibdev, mad_flags, port, in_mad, out_mad);
		goto bail;
	default:
		ret = IB_MAD_RESULT_SUCCESS;
	}

bail:
	return ret;
}

/**
 * qib_process_mad - process an incoming MAD packet
 * @ibdev: the infiniband device this packet came in on
 * @mad_flags: MAD flags
 * @port: the port number this packet came in on
 * @in_wc: the work completion entry for this packet
 * @in_grh: the global route header for this packet
 * @in_mad: the incoming MAD
 * @out_mad: any outgoing MAD reply
 *
 * Returns IB_MAD_RESULT_SUCCESS if this is a MAD that we are not
 * interested in processing.
 *
 * Note that the verbs framework has already done the MAD sanity checks,
 * and hop count/pointer updating for IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE
 * MADs.
 *
 * This is called by the ib_mad module.
 */
int qib_process_mad(struct ib_device *ibdev, int mad_flags, u8 port,
		    struct ib_wc *in_wc, struct ib_grh *in_grh,
		    struct ib_mad *in_mad, struct ib_mad *out_mad)
{
	switch (in_mad->mad_hdr.base_version) {
	case JUMBO_MGMT_BASE_VERSION:
		return hfi_process_stl_mad(ibdev, mad_flags, port,
					    in_wc, in_grh,
					    (struct jumbo_mad *)in_mad,
					    (struct jumbo_mad *)out_mad);
	case IB_MGMT_BASE_VERSION:
		return hfi_process_ib_mad(ibdev, mad_flags, port,
					  in_wc, in_grh, in_mad, out_mad);
	default:
		break;
	}

	return IB_MAD_RESULT_FAILURE;
}

static void send_handler(struct ib_mad_agent *agent,
			 struct ib_mad_send_wc *mad_send_wc)
{
	ib_free_send_mad(mad_send_wc->send_buf);
}

static void xmit_wait_timer_func(unsigned long opaque)
{
	struct qib_pportdata *ppd = (struct qib_pportdata *)opaque;
	struct hfi_devdata *dd = dd_from_ppd(ppd);
	unsigned long flags;
	u8 status;

	spin_lock_irqsave(&ppd->ibport_data.lock, flags);
	if (ppd->cong_stats.flags == IB_PMA_CONG_HW_CONTROL_SAMPLE) {
		status = dd->f_portcntr(ppd, QIBPORTCNTR_PSSTAT);
		if (status == IB_PMA_SAMPLE_STATUS_DONE) {
			/* save counter cache */
			cache_hw_sample_counters(ppd);
			ppd->cong_stats.flags = IB_PMA_CONG_HW_CONTROL_TIMER;
		} else
			goto done;
	}
	ppd->cong_stats.counter = xmit_wait_get_value_delta(ppd);
	dd->f_set_cntr_sample(ppd, QIB_CONG_TIMER_PSINTERVAL, 0x0);
done:
	spin_unlock_irqrestore(&ppd->ibport_data.lock, flags);
	mod_timer(&ppd->cong_stats.timer, jiffies + HZ);
}

int qib_create_agents(struct qib_ibdev *dev)
{
	struct hfi_devdata *dd = dd_from_dev(dev);
	struct ib_mad_agent *agent;
	struct qib_ibport *ibp;
	int p;
	int ret;

	for (p = 0; p < dd->num_pports; p++) {
		ibp = &dd->pport[p].ibport_data;
		agent = ib_register_mad_agent(&dev->ibdev, p + 1, IB_QPT_SMI,
					      NULL, 0, send_handler,
					      NULL, NULL, 0);
		if (IS_ERR(agent)) {
			ret = PTR_ERR(agent);
			goto err;
		}

		/* Initialize xmit_wait structure */
		dd->pport[p].cong_stats.counter = 0;
		init_timer(&dd->pport[p].cong_stats.timer);
		dd->pport[p].cong_stats.timer.function = xmit_wait_timer_func;
		dd->pport[p].cong_stats.timer.data =
			(unsigned long)(&dd->pport[p]);
		dd->pport[p].cong_stats.timer.expires = 0;
		add_timer(&dd->pport[p].cong_stats.timer);

		ibp->send_agent = agent;
	}

	return 0;

err:
	for (p = 0; p < dd->num_pports; p++) {
		ibp = &dd->pport[p].ibport_data;
		if (ibp->send_agent) {
			agent = ibp->send_agent;
			ibp->send_agent = NULL;
			ib_unregister_mad_agent(agent);
		}
	}

	return ret;
}

void qib_free_agents(struct qib_ibdev *dev)
{
	struct hfi_devdata *dd = dd_from_dev(dev);
	struct ib_mad_agent *agent;
	struct qib_ibport *ibp;
	int p;

	for (p = 0; p < dd->num_pports; p++) {
		ibp = &dd->pport[p].ibport_data;
		if (ibp->send_agent) {
			agent = ibp->send_agent;
			ibp->send_agent = NULL;
			ib_unregister_mad_agent(agent);
		}
		if (ibp->sm_ah) {
			ib_destroy_ah(&ibp->sm_ah->ibah);
			ibp->sm_ah = NULL;
		}
		if (dd->pport[p].cong_stats.timer.data)
			del_timer_sync(&dd->pport[p].cong_stats.timer);
	}
}
