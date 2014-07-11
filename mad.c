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
#ifdef CONFIG_STL_MGMT
#include <rdma/stl_smi.h>
#define STL_NUM_PKEY_BLOCKS_PER_SMP (STL_SMP_DR_DATA_SIZE \
			/ (STL_PARTITION_TABLE_BLK_SIZE * sizeof(u16)))
#endif /* CONFIG_STL_MGMT */

#include "hfi.h"
#include "mad.h"
#include "trace.h"

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

static int reply_failure(void *arg)
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
	return IB_MAD_RESULT_FAILURE | IB_MAD_RESULT_REPLY;
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

#ifdef CONFIG_STL_MGMT
static int __subn_get_stl_nodedesc(struct stl_smp *smp, u32 am,
				   u8 *data, struct ib_device *ibdev,
				   u8 port, u32 *resp_len)
{
	struct stl_node_description *nd;

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
#else /* !CONFIG_STL_MGMT */

static int subn_get_nodedescription(struct ib_smp *smp,
				    struct ib_device *ibdev)
{
	if (smp->attr_mod)
		smp->status |= IB_SMP_INVALID_FIELD;

	memcpy(smp->data, ibdev->node_desc, sizeof(smp->data));

	return reply(smp);
}
#endif /* !CONFIG_STL_MGMT */

#ifdef CONFIG_STL_MGMT
static int __subn_get_stl_nodeinfo(struct stl_smp *smp, u32 am, u8 *data,
				   struct ib_device *ibdev, u8 port,
				   u32 *resp_len)
{
	struct stl_node_info *ni;
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	u32 vendor, majrev, minrev;
	unsigned pidx = port - 1; /* IB number port from 1, hdw from 0 */
	ni = (struct stl_node_info *)data;

	memset(ni, 0, sizeof(*ni));

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
#endif /* CONFIG_STL_MGMT */

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
#ifdef CONFIG_STL_MGMT
	nip->base_version = JUMBO_MGMT_BASE_VERSION;
	nip->class_version = STL_SMI_CLASS_VERSION;
#else
	nip->base_version = 1;
	nip->class_version = 1;
#endif /* CONFIG_STL_MGMT */
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

#ifndef CONFIG_STL_MGMT
static int subn_get_guidinfo(struct ib_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	u32 startgx = 8 * be32_to_cpu(smp->attr_mod);
	__be64 *p = (__be64 *) smp->data;
	unsigned pidx = port - 1; /* IB number port from 1, hdw from 0 */

	/* 32 blocks of 8 64-bit GUIDs per block */

	memset(smp->data, 0, sizeof(smp->data));

	if (startgx == 0 && pidx < dd->num_pports) {
		struct qib_pportdata *ppd = dd->pport + pidx;
		struct qib_ibport *ibp = &ppd->ibport_data;
		__be64 g = ppd->guid;
		unsigned i;

		/* GUID 0 is illegal */
		if (g == 0)
			smp->status |= IB_SMP_INVALID_FIELD;
		else {
			/* The first is a copy of the read-only HW GUID. */
			p[0] = g;
			for (i = 1; i < QIB_GUIDS_PER_PORT; i++)
				p[i] = ibp->guids[i - 1];
		}
	} else
		smp->status |= IB_SMP_INVALID_FIELD;

	return reply(smp);
}
#endif /* !CONFIG_STL_MGMT */

static void set_link_width_enabled(struct qib_pportdata *ppd, u32 w)
{
	(void) ppd->dd->f_set_ib_cfg(ppd, QIB_IB_CFG_LWID_ENB, w);
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
		IB_LINKINITCMD_SLEEP;
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

#ifdef CONFIG_STL_MGMT
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
	u32 port_num = am & 0xff;
	u32 num_ports = am >> 24;
	u32 buffer_units;

	/* Clear all fields.  Only set the non-zero fields. */
	memset(pi, 0, sizeof(*pi));

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

	pi->link_speed.supported = cpu_to_be16(ppd->link_speed_supported);
	pi->link_speed.active = cpu_to_be16(ppd->link_speed_active);
	pi->link_speed.enabled = cpu_to_be16(ppd->link_speed_enabled);

	/* FIXME make sure that this default state matches */
	pi->port_states.offline_reason = 0;
	pi->port_states.unsleepstate_downdefstate =
		(get_linkdowndefaultstate(ppd) ? 1 : 2);

	state = dd->f_iblink_state(ppd);
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

	/* FIXME supported, enabled, active all == STL */
	pi->port_link_mode  = STL_PORT_LINK_MODE_STL << 10;
	pi->port_link_mode |= STL_PORT_LINK_MODE_STL << 5;
	pi->port_link_mode |= STL_PORT_LINK_MODE_STL;
	pi->port_link_mode = cpu_to_be16(pi->port_link_mode);

	/* FIXME supported, enabled, active all == 16-bit */
	pi->port_ltp_crc_mode  = STL_PORT_LTP_CRC_MODE_16 << 8;
	pi->port_ltp_crc_mode |= STL_PORT_LTP_CRC_MODE_16 << 4;
	pi->port_ltp_crc_mode |= STL_PORT_LTP_CRC_MODE_16;
	pi->port_ltp_crc_mode = cpu_to_be16(pi->port_ltp_crc_mode);

	pi->port_packet_format.supported =
		cpu_to_be16(STL_PORT_PACKET_FORMAT_9B);
	pi->port_packet_format.enabled =
		cpu_to_be16(STL_PORT_PACKET_FORMAT_9B);

	pi->link_down_reason = STL_LINKDOWN_REASON_NONE;

	pi->link_width_downgrade.supported = 0;
	pi->link_width_downgrade.enabled = 0;
	pi->link_width_downgrade.active = 0;

	pi->mtucap = mtu_to_enum(max_mtu, IB_MTU_4096);

	/* 32.768 usec. response time (guessing) */
	pi->resptimevalue = 3;

	pi->local_port_num = port;

	/* buffer info for FM */
	pi->overall_buffer_space = cpu_to_be16(dd->link_credits);

	pi->neigh_node_guid = ppd->neighbor_guid;
	pi->port_neigh_mode = ppd->neighbor_type & STL_PI_MASK_NEIGH_NODE_TYPE;
	if (pi->port_neigh_mode == 1)
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

	if (resp_len)
		*resp_len += sizeof(struct stl_port_info);

	return reply(smp);
}
#else /* !CONFIG_STL_MGMT */

static int subn_get_portinfo(struct ib_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	struct hfi_devdata *dd;
	struct qib_pportdata *ppd;
	struct qib_ibport *ibp;
	struct ib_port_info *pip = (struct ib_port_info *)smp->data;
	u8 mtu;
	u16 link_speed_supported;
	u16 link_speed_active;
	u16 link_speed_enabled;
	int ret;
	u32 state;
	u32 port_num = be32_to_cpu(smp->attr_mod);

	if (port_num == 0)
		port_num = port;
	else {
		if (port_num > ibdev->phys_port_cnt) {
			smp->status |= IB_SMP_INVALID_FIELD;
			ret = reply(smp);
			goto bail;
		}
		if (port_num != port) {
			ibp = to_iport(ibdev, port_num);
			ret = check_mkey(ibp, (struct ib_mad_hdr *)smp, 0,
					 smp->mkey, smp->dr_slid,
					 smp->return_path, smp->hop_cnt);
			if (ret) {
				ret = IB_MAD_RESULT_FAILURE;
				goto bail;
			}
		}
	}

	dd = dd_from_ibdev(ibdev);
	/* IB numbers ports from 1, hdw from 0 */
	ppd = dd->pport + (port_num - 1);
	ibp = &ppd->ibport_data;

	/* Clear all fields.  Only set the non-zero fields. */
	memset(smp->data, 0, sizeof(smp->data));

	/* Only return the mkey if the protection field allows it. */
	if (!(smp->method == IB_MGMT_METHOD_GET &&
	      ibp->mkey != smp->mkey &&
	      ibp->mkeyprot == 1))
		pip->mkey = ibp->mkey;
	pip->gid_prefix = ibp->gid_prefix;
	pip->lid = cpu_to_be16(ppd->lid);
	pip->sm_lid = cpu_to_be16(ibp->sm_lid);
	pip->cap_mask = cpu_to_be32(ibp->port_cap_flags);
	/* pip->diag_code; */
	pip->mkey_lease_period = cpu_to_be16(ibp->mkey_lease_period);
	pip->local_port_num = port;

	/* will drop STL 1,3 */
	pip->link_width_enabled = (u8)ppd->link_width_enabled;
	pip->link_width_supported = (u8)ppd->link_width_supported;
	pip->link_width_active = (u8)ppd->link_width_active;
	state = dd->f_iblink_state(ppd);
	link_speed_supported = ppd->link_speed_supported;
	link_speed_active = ppd->link_speed_active;
	link_speed_enabled = ppd->link_speed_enabled;

	pip->linkspeed_portstate = link_speed_supported << 4 | state;

	pip->portphysstate_linkdown =
		(dd->f_ibphys_portstate(ppd) << 4) |
		(get_linkdowndefaultstate(ppd) ? 1 : 2);
	pip->mkeyprot_resv_lmc = (ibp->mkeyprot << 6) | ppd->lmc;
	pip->linkspeedactive_enabled = ((u8)link_speed_active << 4) |
		(u8)link_speed_enabled;
	mtu = mtu_to_enum(ppd->ibmtu, IB_MTU_4096);
	pip->neighbormtu_mastersmsl = (mtu << 4) | ibp->sm_sl;
	pip->vlcap_inittype = ppd->vls_supported << 4;  /* InitType = 0 */
	pip->vl_high_limit = ibp->vl_high_limit;
	pip->vl_arb_high_cap =
		dd->f_get_ib_cfg(ppd, QIB_IB_CFG_VL_HIGH_CAP);
	pip->vl_arb_low_cap =
		dd->f_get_ib_cfg(ppd, QIB_IB_CFG_VL_LOW_CAP);
	/* InitTypeReply = 0 */
	pip->inittypereply_mtucap = mtu_to_enum(max_mtu, IB_MTU_4096);
	/* HCAs ignore VLStallCount and HOQLife */
	/* pip->vlstallcnt_hoqlife; */
	pip->operationalvl_pei_peo_fpi_fpo =
		dd->f_get_ib_cfg(ppd, QIB_IB_CFG_OP_VLS) << 4;
	pip->mkey_violations = cpu_to_be16(ibp->mkey_violations);
	/* P_KeyViolations are counted by hardware. */
	pip->pkey_violations = cpu_to_be16(ibp->pkey_violations);
	pip->qkey_violations = cpu_to_be16(ibp->qkey_violations);
	/* Only the hardware GUID is supported for now */
	pip->guid_cap = QIB_GUIDS_PER_PORT;
	pip->clientrereg_resv_subnetto = ibp->subnet_timeout;
	/* 32.768 usec. response time (guessing) */
	pip->resv_resptimevalue = 3;
	pip->localphyerrors_overrunerrors =
		(get_phyerrthreshold(ppd) << 4) |
		get_overrunthreshold(ppd);
	/* pip->max_credit_hint; */
	if (ibp->port_cap_flags & IB_PORT_LINK_LATENCY_SUP) {
		u32 v;

		v = dd->f_get_ib_cfg(ppd, QIB_IB_CFG_LINKLATENCY);
		pip->link_roundtrip_latency[0] = v >> 16;
		pip->link_roundtrip_latency[1] = v >> 8;
		pip->link_roundtrip_latency[2] = v;
	}

	ret = reply(smp);

bail:
	return ret;
}
#endif /* !CONFIG_STL_MGMT */

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

#ifdef CONFIG_STL_MGMT
static int __subn_get_stl_pkeytable(struct stl_smp *smp, u32 am, u8 *data,
				    struct ib_device *ibdev, u8 port,
				    u32 *resp_len)
{
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	u32 n_blocks_req = (am & 0xff000000) >> 24;
	u32 am_port = (am & 0x00ff0000) >> 16;
	u32 start_block = am & 0x7ff;
	__be16 *p;
	int i;
	u16 n_blocks_avail;
	unsigned npkeys = qib_get_npkeys(dd);
	size_t size;

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
	memset(data, 0, size);

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
#else /* !CONFIG_STL_MGMT */

static int subn_get_pkeytable(struct ib_smp *smp, struct ib_device *ibdev,
			      u8 port)
{
	u32 startpx = 32 * (be32_to_cpu(smp->attr_mod) & 0xffff);
	u16 *p = (u16 *) smp->data;
	__be16 *q = (__be16 *) smp->data;

	/* 64 blocks of 32 16-bit P_Key entries */

	memset(smp->data, 0, sizeof(smp->data));
	if (startpx == 0) {
		struct hfi_devdata *dd = dd_from_ibdev(ibdev);
		unsigned i, n = qib_get_npkeys(dd);

		get_pkeys(dd, port, p);

		for (i = 0; i < n; i++)
			q[i] = cpu_to_be16(p[i]);
	} else
		smp->status |= IB_SMP_INVALID_FIELD;

	return reply(smp);
}

static int subn_set_guidinfo(struct ib_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	u32 startgx = 8 * be32_to_cpu(smp->attr_mod);
	__be64 *p = (__be64 *) smp->data;
	unsigned pidx = port - 1; /* IB number port from 1, hdw from 0 */

	/* 32 blocks of 8 64-bit GUIDs per block */

	if (startgx == 0 && pidx < dd->num_pports) {
		struct qib_pportdata *ppd = dd->pport + pidx;
		struct qib_ibport *ibp = &ppd->ibport_data;
		unsigned i;

		/* The first entry is read-only. */
		for (i = 1; i < QIB_GUIDS_PER_PORT; i++)
			ibp->guids[i - 1] = p[i];
	} else
		smp->status |= IB_SMP_INVALID_FIELD;

	/* The only GUID we support is the first read-only entry. */
	return subn_get_guidinfo(smp, ibdev, port);
}
#endif /* !CONFIG_STL_MGMT */

#ifdef CONFIG_STL_MGMT
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
	u8 state;
	u8 vls;
	u8 msl;
	u16 lse, lwe, mtu;
	u16 lstate;
	int ret, ore, i;
	u32 port_num = am & 0xff;
	u32 num_ports = am >> 24;

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
	lse = be16_to_cpu(pi->link_speed.enabled);
	if (lse) {
		if ((lse & STL_LINK_SPEED_ALL_SUPPORTED) == STL_LINK_SPEED_ALL_SUPPORTED) {
			set_link_speed_enabled(ppd, STL_LINK_SPEED_ALL_SUPPORTED);
		} else if (lse & be16_to_cpu(pi->link_speed.supported)) {
			set_link_speed_enabled(ppd, lse);
		} else
			smp->status |= IB_SMP_INVALID_FIELD;
	}

	/* Set link down default state. */
	/* Again make this virtual.  Only IB PortInfo controls the actual port
	 * states */
	switch (pi->port_states.unsleepstate_downdefstate & STL_PI_MASK_DOWNDEF_STATE) {
	case 0: /* NOP */
		break;
	case 1: /* SLEEP */
	/*
		(void) dd->f_set_ib_cfg(ppd, QIB_IB_CFG_LINKDEFAULT,
					IB_LINKINITCMD_SLEEP);
					*/
		break;
	case 2: /* POLL */
	/*
		(void) dd->f_set_ib_cfg(ppd, QIB_IB_CFG_LINKDEFAULT,
					IB_LINKINITCMD_POLL);
					*/
		break;
	default:
		pr_warn("SubnSet(STL_PortInfo) Default state invalid 0x%x\n",
			pi->port_states.unsleepstate_downdefstate & STL_PI_MASK_DOWNDEF_STATE);
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
			mtu = enum_to_mtu(pi->neigh_mtu.pvlx_to_mtu[i/2]);
		if (mtu == -1) {
			printk(KERN_WARNING
			       "SubnSet(STL_PortInfo) mtu invalid %d (0x%x)\n", mtu,
			       (pi->neigh_mtu.pvlx_to_mtu[0] >> 4) & 0xF);
			smp->status |= IB_SMP_INVALID_FIELD;
		}
	}
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

	/*
	 * Do the port state change now that the other link parameters
	 * have been set.
	 * Changing the port physical state only makes sense if the link
	 * is down or is being set to down.
	 */
	state = pi->port_states.portphysstate_portstate & STL_PI_MASK_PORT_STATE;
	lstate = (pi->port_states.portphysstate_portstate & STL_PI_MASK_PORT_PHYSICAL_STATE) >> 4;
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
#if 0
		if (lstate == 0)
			//lstate = QIB_IB_LINKDOWN_ONLY;
		else if (lstate == 1)
			//lstate = QIB_IB_LINKDOWN_SLEEP;
		else if (lstate == 2)
			//lstate = QIB_IB_LINKDOWN;
		else if (lstate == 3)
			//lstate = QIB_IB_LINKDOWN_DISABLE;
		else {
#endif
		/* FIXME allow other STL states */
		if (lstate > 3) {
			pr_warn("SubnSet(STL_PortInfo) invalid Physical state 0x%x\n",
				lstate);
			smp->status |= IB_SMP_INVALID_FIELD;
			break;
		}

		qib_set_linkstate(ppd, lstate);
		/*
		 * Don't send a reply if the response would be sent
		 * through the disabled port.
		 */
		if (lstate == QIB_IB_LINKDOWN_DISABLE && smp->hop_cnt) {
			ret = IB_MAD_RESULT_SUCCESS | IB_MAD_RESULT_CONSUMED;
			return ret;
		}
		/* XXX ??? qib_wait_linkstate(ppd, QIBL_LINKV, 10); */
		break;
	case IB_PORT_ARMED:
		qib_set_linkstate(ppd, QIB_IB_LINKARM);
		break;
	case IB_PORT_ACTIVE:
		qib_set_linkstate(ppd, QIB_IB_LINKACTIVE);
		break;
	default:
		pr_warn("SubnSet(STL_PortInfo) invalid state 0x%x\n", state);
		smp->status |= IB_SMP_INVALID_FIELD;
	}

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
#else /* !CONFIG_STL_MGMT */

/**
 * subn_set_portinfo - set port information
 * @smp: the incoming SM packet
 * @ibdev: the infiniband device
 * @port: the port on the device
 *
 * Set Portinfo (see ch. 14.2.5.6).
 */
static int subn_set_portinfo(struct ib_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	struct ib_port_info *pip = (struct ib_port_info *)smp->data;
	struct ib_event event;
	struct hfi_devdata *dd;
	struct qib_pportdata *ppd;
	struct qib_ibport *ibp;
	u8 clientrereg = (pip->clientrereg_resv_subnetto & 0x80);
	unsigned long flags;
	u16 lid, smlid;
	u8 lwe;
	u8 lse;
	u8 state;
	u8 vls;
	u8 msl;
	u16 lstate;
	int ret, ore, mtu;
	u32 port_num = be32_to_cpu(smp->attr_mod);

	if (port_num == 0)
		port_num = port;
	else {
		if (port_num > ibdev->phys_port_cnt)
			goto err;
		/* Port attributes can only be set on the receiving port */
		if (port_num != port)
			goto get_only;
	}

	dd = dd_from_ibdev(ibdev);
	/* IB numbers ports from 1, hdw from 0 */
	ppd = dd->pport + (port_num - 1);
	ibp = &ppd->ibport_data;
	event.device = ibdev;
	event.element.port_num = port;

	ibp->mkey = pip->mkey;
	ibp->gid_prefix = pip->gid_prefix;
	ibp->mkey_lease_period = be16_to_cpu(pip->mkey_lease_period);

	lid = be16_to_cpu(pip->lid);
	/* Must be a valid unicast LID address. */
	if (lid == 0 || lid >= QIB_MULTICAST_LID_BASE)
		smp->status |= IB_SMP_INVALID_FIELD;
	else if (ppd->lid != lid || ppd->lmc != (pip->mkeyprot_resv_lmc & 7)) {
		if (ppd->lid != lid)
			qib_set_uevent_bits(ppd, _QIB_EVENT_LID_CHANGE_BIT);
		if (ppd->lmc != (pip->mkeyprot_resv_lmc & 7))
			qib_set_uevent_bits(ppd, _QIB_EVENT_LMC_CHANGE_BIT);
		qib_set_lid(ppd, lid, pip->mkeyprot_resv_lmc & 7);
		event.event = IB_EVENT_LID_CHANGE;
		ib_dispatch_event(&event);
	}

	smlid = be16_to_cpu(pip->sm_lid);
	msl = pip->neighbormtu_mastersmsl & 0xF;
	/* Must be a valid unicast LID address. */
	if (smlid == 0 || smlid >= QIB_MULTICAST_LID_BASE)
		smp->status |= IB_SMP_INVALID_FIELD;
	else if (smlid != ibp->sm_lid || msl != ibp->sm_sl) {
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

	/* Allow 1x or 4x to be set (see 14.2.6.6). */
	lwe = pip->link_width_enabled;
	if (lwe) {
		if (lwe == 0xFF)
			set_link_width_enabled(ppd, ppd->link_width_supported);
		else if (lwe >= 16 || (lwe & ~ppd->link_width_supported))
			smp->status |= IB_SMP_INVALID_FIELD;
		else if (lwe != ppd->link_width_enabled)
			set_link_width_enabled(ppd, lwe);
	}

	lse = pip->linkspeedactive_enabled & 0xF;
	if (lse) {
		/*
		 * The IB 1.2 spec. only allows link speed values
		 * 1, 3, 5, 7, 15.  1.2.1 extended to allow specific
		 * speeds.
		 */
		if (lse == 15)
			set_link_speed_enabled(ppd,
					       ppd->link_speed_supported);
		else if (lse >= 8 || (lse & ~ppd->link_speed_supported))
			smp->status |= IB_SMP_INVALID_FIELD;
		else if (lse != ppd->link_speed_enabled)
			set_link_speed_enabled(ppd, lse);
	}

	/* Set link down default state. */
	switch (pip->portphysstate_linkdown & 0xF) {
	case 0: /* NOP */
		break;
	case 1: /* SLEEP */
		if (dd->f_set_ib_cfg(ppd, QIB_IB_CFG_LINKDEFAULT,
					IB_LINKINITCMD_SLEEP) < 0)
			smp->status |= IB_SMP_INVALID_FIELD;
		break;
	case 2: /* POLL */
		if (dd->f_set_ib_cfg(ppd, QIB_IB_CFG_LINKDEFAULT,
					IB_LINKINITCMD_POLL) < 0)
			smp->status |= IB_SMP_INVALID_FIELD;
		break;
	default:
		smp->status |= IB_SMP_INVALID_FIELD;
	}

	ibp->mkeyprot = pip->mkeyprot_resv_lmc >> 6;
	ibp->vl_high_limit = pip->vl_high_limit;
	(void) dd->f_set_ib_cfg(ppd, QIB_IB_CFG_VL_HIGH_LIMIT,
				    ibp->vl_high_limit);

	mtu = enum_to_mtu((pip->neighbormtu_mastersmsl >> 4) & 0xF);
	if (mtu == -1)
		smp->status |= IB_SMP_INVALID_FIELD;
	else {
		int i;
		/*
		 * Non-STL SM's will set the MTU per port, so set all VLs
		 * to the same MTU.
		 */
		for (i = 0; i < hfi_num_vls(ppd->vls_supported); i++)
			dd->vld[i].mtu = mtu;
		set_mtu(ppd);
	}

	/* Set operational VLs */
	vls = (pip->operationalvl_pei_peo_fpi_fpo >> 4) & 0xF;
	if (vls) {
		if (vls > ppd->vls_supported)
			smp->status |= IB_SMP_INVALID_FIELD;
		else
			(void) dd->f_set_ib_cfg(ppd, QIB_IB_CFG_OP_VLS, vls);
	}

	if (pip->mkey_violations == 0)
		ibp->mkey_violations = 0;

	if (pip->pkey_violations == 0)
		ibp->pkey_violations = 0;

	if (pip->qkey_violations == 0)
		ibp->qkey_violations = 0;

	ore = pip->localphyerrors_overrunerrors;
	if (set_phyerrthreshold(ppd, (ore >> 4) & 0xF))
		smp->status |= IB_SMP_INVALID_FIELD;

	if (set_overrunthreshold(ppd, (ore & 0xF)))
		smp->status |= IB_SMP_INVALID_FIELD;

	ibp->subnet_timeout = pip->clientrereg_resv_subnetto & 0x1F;

	/*
	 * Do the port state change now that the other link parameters
	 * have been set.
	 * Changing the port physical state only makes sense if the link
	 * is down or is being set to down.
	 */
	state = pip->linkspeed_portstate & 0xF;
	lstate = (pip->portphysstate_linkdown >> 4) & 0xF;
	if (lstate && !(state == IB_PORT_DOWN || state == IB_PORT_NOP))
		smp->status |= IB_SMP_INVALID_FIELD;

	/*
	 * Only state changes of DOWN, ARM, and ACTIVE are valid
	 * and must be in the correct state to take effect (see 7.2.6).
	 */
	switch (state) {
	case IB_PORT_NOP:
		if (lstate == IB_PORTPHYSSTATE_NO_CHANGE)
			break;
		/* FALLTHROUGH */
	case IB_PORT_DOWN:
		if (lstate == IB_PORTPHYSSTATE_NO_CHANGE)
			lstate = QIB_IB_LINKDOWN_ONLY;
		else if (lstate == IB_PORTPHYSSTATE_SLEEP)
			lstate = QIB_IB_LINKDOWN_SLEEP;
		else if (lstate == IB_PORTPHYSSTATE_POLL)
			lstate = QIB_IB_LINKDOWN;
		else if (lstate == IB_PORTPHYSSTATE_DISABLED)
			lstate = QIB_IB_LINKDOWN_DISABLE;
		else {
			/* TODO: The spec say other values are ignored... */
			smp->status |= IB_SMP_INVALID_FIELD;
			break;
		}
		qib_set_linkstate(ppd, lstate);
		/*
		 * Don't send a reply if the response would be sent
		 * through the disabled port.
		 */
		if (lstate == QIB_IB_LINKDOWN_DISABLE && smp->hop_cnt) {
			ret = IB_MAD_RESULT_SUCCESS | IB_MAD_RESULT_CONSUMED;
			goto done;
		}
		break;
	case IB_PORT_ARMED:
		qib_set_linkstate(ppd, QIB_IB_LINKARM);
		break;
	case IB_PORT_ACTIVE:
		qib_set_linkstate(ppd, QIB_IB_LINKACTIVE);
		break;
	default:
		smp->status |= IB_SMP_INVALID_FIELD;
	}

	if (clientrereg) {
		event.event = IB_EVENT_CLIENT_REREGISTER;
		ib_dispatch_event(&event);
	}

	ret = subn_get_portinfo(smp, ibdev, port);

	/* restore re-reg bit per o14-12.2.1 */
	pip->clientrereg_resv_subnetto |= clientrereg;

	goto get_only;

err:
	smp->status |= IB_SMP_INVALID_FIELD;
get_only:
	ret = subn_get_portinfo(smp, ibdev, port);
done:
	return ret;
}
#endif /* !CONFIG_STL_MGMT */

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

#ifdef CONFIG_STL_MGMT
static int __subn_set_stl_pkeytable(struct stl_smp *smp, u32 am, u8 *data,
				    struct ib_device *ibdev, u8 port,
				    u32 *resp_len)
{
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	u32 n_blocks_sent = (am & 0xff000000) >> 24;
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
#else /* !CONFIG_STL_MGMT */

static int subn_set_pkeytable(struct ib_smp *smp, struct ib_device *ibdev,
			      u8 port)
{
	u32 startpx = 32 * (be32_to_cpu(smp->attr_mod) & 0xffff);
	__be16 *p = (__be16 *) smp->data;
	u16 *q = (u16 *) smp->data;
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	unsigned i, n = qib_get_npkeys(dd);

	for (i = 0; i < n; i++)
		q[i] = be16_to_cpu(p[i]);

	if (startpx != 0 || set_pkeys(dd, port, q) != 0)
		smp->status |= IB_SMP_INVALID_FIELD;

	return subn_get_pkeytable(smp, ibdev, port);
}
#endif /* !CONFIG_STL_MGMT */

#ifdef CONFIG_STL_MGMT
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
	memcpy(dd->sc2vl, (u64 *)data, sizeof(dd->sc2vl));
	return 0;
}

static int __subn_get_stl_sc_to_vlt(struct stl_smp *smp, u32 am, u8 *data,
				    struct ib_device *ibdev, u8 port,
				    u32 *resp_len)
{
	u32 n_blocks = (am & 0xff000000) >> 24;
	u32 am_port = (am & 0x000000ff);
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	void *vp = (void *) data;
	size_t size = 4 * sizeof(u64);

	memset(vp, 0, size);

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
	u32 n_blocks = (am & 0xff000000) >> 24;
	u32 am_port = (am & 0x000000ff);
	int async_update = !!(am & 0x1000);
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	void *vp = (void *) data;

	if (am_port != port || port != 1 || n_blocks != 1 || async_update) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}

	set_sc2vlt_tables(dd, vp);

	return __subn_get_stl_sc_to_vlt(smp, am, data, ibdev, port, resp_len);
}

static int __subn_get_stl_bct(struct stl_smp *smp, u32 am, u8 *data,
			      struct ib_device *ibdev, u8 port, u32 *resp_len)
{
	u32 num_ports = (am & 0xff000000) >> 24;
	u32 port_num = am & 0x000000ff;
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	struct qib_pportdata *ppd;
	struct buffer_control *p = (struct buffer_control *) data;
	memset(p, 0, sizeof(*p));

	if (num_ports != 1 || port_num != port) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}

	ppd = dd->pport + (port_num - 1);
	fm_get_table(ppd, FM_TBL_BUFFER_CONTROL, p);
	trace_bct_get(dd, p);
	if (resp_len)
		*resp_len += sizeof(struct buffer_control);

	return reply(smp);
}

static int __subn_set_stl_bct(struct stl_smp *smp, u32 am, u8 *data,
			      struct ib_device *ibdev, u8 port, u32 *resp_len)
{
	u32 num_ports = (am & 0xff000000) >> 24;
	u32 port_num = am & 0x000000ff;
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

#else /* !CONFIG_STL_MGMT */

static int subn_get_sl_to_vl(struct ib_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	struct qib_ibport *ibp = to_iport(ibdev, port);
	u8 *p = (u8 *) smp->data;
	unsigned i;

	memset(smp->data, 0, sizeof(smp->data));

	if (!(ibp->port_cap_flags & IB_PORT_SL_MAP_SUP))
		smp->status |= IB_SMP_UNSUP_METHOD;
	else
		for (i = 0; i < ARRAY_SIZE(ibp->sl_to_vl); i += 2)
			*p++ = (ibp->sl_to_vl[i] << 4) | ibp->sl_to_vl[i + 1];

	return reply(smp);
}

static int subn_set_sl_to_vl(struct ib_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	struct qib_ibport *ibp = to_iport(ibdev, port);
	u8 *p = (u8 *) smp->data;
	unsigned i;

	if (!(ibp->port_cap_flags & IB_PORT_SL_MAP_SUP)) {
		smp->status |= IB_SMP_UNSUP_METHOD;
		return reply(smp);
	}

	for (i = 0; i < ARRAY_SIZE(ibp->sl_to_vl); i += 2, p++) {
		ibp->sl_to_vl[i] = *p >> 4;
		ibp->sl_to_vl[i + 1] = *p & 0xF;
	}
	qib_set_uevent_bits(ppd_from_ibp(to_iport(ibdev, port)),
			    _QIB_EVENT_SL2VL_CHANGE_BIT);

	return subn_get_sl_to_vl(smp, ibdev, port);
}
#endif /* !CONFIG_STL_MGMT */

#ifdef CONFIG_STL_MGMT
static int __subn_get_stl_vl_arb(struct stl_smp *smp, u32 am, u8 *data,
				 struct ib_device *ibdev, u8 port,
				 u32 *resp_len)
{
	struct qib_pportdata *ppd = ppd_from_ibp(to_iport(ibdev, port));
	u32 num_ports = (am & 0xff000000) >> 24;
	u8 section = (am & 0x00ff0000) >> 16;
	u32 port_num = am & 0x000000ff;
	u8 *p = data;

	if (port_num == 0)
		port_num = port;

	if (num_ports != 1 || port_num != port) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(smp);
	}

	switch (section) {
	case STL_VLARB_LOW_ELEMENTS:
		(void) fm_get_table(ppd, FM_TBL_VL_LOW_ARB, p);
		if (resp_len)
			*resp_len += WFR_VL_ARB_LOW_PRIO_TABLE_SIZE * 2;
		break;
	case STL_VLARB_HIGH_ELEMENTS:
		(void) fm_get_table(ppd, FM_TBL_VL_HIGH_ARB, p);
		if (resp_len)
			*resp_len += WFR_VL_ARB_HIGH_PRIO_TABLE_SIZE * 2;
		break;
	case STL_VLARB_PREEMPT_ELEMENTS:
		/* XXX FIXME */
		break;
	case STL_VLARB_PREEMPT_MATRIX:
		/* XXX FIXME */
		break;
	default:
		pr_warn("STL SubnGet(VL Arb) AM Invalid : 0x%x\n",
			be32_to_cpu(smp->attr_mod));
		smp->status |= IB_SMP_INVALID_FIELD;
		break;
	}

	return reply(smp);
}
#else /* !CONFIG_STL_MGMT */

static int subn_get_vl_arb(struct ib_smp *smp, struct ib_device *ibdev,
			   u8 port)
{
	unsigned which = be32_to_cpu(smp->attr_mod) >> 16;
	struct qib_pportdata *ppd = ppd_from_ibp(to_iport(ibdev, port));

	memset(smp->data, 0, sizeof(smp->data));

	if (ppd->vls_supported == IB_VL_VL0)
		smp->status |= IB_SMP_UNSUP_METHOD;
	else if (which == IB_VLARB_LOWPRI_0_31)
		(void) fm_get_table(ppd, FM_TBL_VL_LOW_ARB, smp->data);
	else if (which == IB_VLARB_HIGHPRI_0_31)
		(void) fm_get_table(ppd, FM_TBL_VL_HIGH_ARB, smp->data);
	else
		smp->status |= IB_SMP_INVALID_FIELD;

	return reply(smp);
}
#endif /* !CONFIG_STL_MGMT */

#ifdef CONFIG_STL_MGMT
static int __subn_set_stl_vl_arb(struct stl_smp *smp, u32 am, u8 *data,
				 struct ib_device *ibdev, u8 port,
				 u32 *resp_len)
{
	struct qib_pportdata *ppd = ppd_from_ibp(to_iport(ibdev, port));
	u32 num_ports = (am & 0xff000000) >> 24;
	u8 section = (am & 0x00ff0000) >> 16;
	u32 port_num = am & 0x000000ff;
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
	case STL_VLARB_PREEMPT_ELEMENTS:
		/* XXX FIXME */
		break;
	case STL_VLARB_PREEMPT_MATRIX:
		/* XXX FIXME */
		break;
	default:
		pr_warn("STL SubnSet(VL Arb) AM Invalid : 0x%x\n",
			be32_to_cpu(smp->attr_mod));
		smp->status |= IB_SMP_INVALID_FIELD;
		break;
	}

	return __subn_get_stl_vl_arb(smp, am, data, ibdev, port, resp_len);
}
#else /* !CONFIG_STL_MGMT */

static int subn_set_vl_arb(struct ib_smp *smp, struct ib_device *ibdev,
			   u8 port)
{
	unsigned which = be32_to_cpu(smp->attr_mod) >> 16;
	struct qib_pportdata *ppd = ppd_from_ibp(to_iport(ibdev, port));

	if (ppd->vls_supported == IB_VL_VL0)
		smp->status |= IB_SMP_UNSUP_METHOD;
	else if (which == IB_VLARB_LOWPRI_0_31)
		(void) fm_set_table(ppd, FM_TBL_VL_LOW_ARB, smp->data);
	else if (which == IB_VLARB_HIGHPRI_0_31)
		(void) fm_set_table(ppd, FM_TBL_VL_HIGH_ARB, smp->data);
	else
		smp->status |= IB_SMP_INVALID_FIELD;

	return subn_get_vl_arb(smp, ibdev, port);
}

static int subn_trap_repress(struct ib_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	/*
	 * For now, we only send the trap once so no need to process this.
	 * o13-6, o13-7,
	 * o14-3.a4 The SMA shall not send any message in response to a valid
	 * SubnTrapRepress() message.
	 */
	return IB_MAD_RESULT_SUCCESS | IB_MAD_RESULT_CONSUMED;
}
#endif /* !CONFIG_STL_MGMT */

#ifdef CONFIG_STL_MGMT
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

	u8 link_quality_indicator; /* 4res, 4bit */
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
	cs_port_xmit_data			= (1 << 0),
	cs_port_rcv_data			= (1 << 1),
	cs_port_xmit_pkts			= (1 << 2),
	cs_port_rcv_pkts			= (1 << 3),
	cs_port_mcast_xmit_pkts			= (1 << 4),
	cs_port_mcast_rcv_pkts			= (1 << 5),
	cs_port_xmit_wait			= (1 << 6),
	cs_sw_port_congestion			= (1 << 7),
	cs_port_rcv_fecn			= (1 << 8),
	cs_port_rcv_becn			= (1 << 9),
	cs_port_xmit_time_cong			= (1 << 10),
	cs_port_xmit_wasted_bw			= (1 << 11),
	cs_port_xmit_wait_data			= (1 << 12),
	cs_port_rcv_bubble			= (1 << 13),
	cs_port_mark_fecn			= (1 << 14),
	cs_port_rcv_constraint_errors		= (1 << 15),
	cs_port_rcv_switch_relay_errors		= (1 << 16),
	cs_port_xmit_discards			= (1 << 17),
	cs_port_xmit_constraint_errors		= (1 << 18),
	cs_port_rcv_remote_physical_errors	= (1 << 19),
	cs_local_link_integrity_errors		= (1 << 20),
	cs_port_rcv_errors			= (1 << 21),
	cs_excessive_buffer_overruns		= (1 << 22),
	cs_fm_config_errors			= (1 << 23),
	cs_link_error_recovery			= (1 << 24),
	cs_link_downed				= (1 << 25),
	cs_uncorrectable_errors			= (1 << 26),
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
		__be32 link_quality_indicator; /* 4res, 4bit */

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
	int vfi;

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

	if (nports != 1 || (req->port_num && req->port_num != port)
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
	/* rsp->port_xmit_discards ??? */
	/* port_xmit_constraint_errors - driver (table 13-11 WFR spec) */
	rsp->port_rcv_remote_physical_errors =
		cpu_to_be64(read_csr(dd, DCC_ERR_RCVREMOTE_PHY_ERR_CNT));
	/* local_link_integrity_errors - DC (table 13-11 WFR spec)
	 * is this LCB_ERR_INFO_TX_REPLAY_CNT ??? */
	rsp->port_rcv_errors =
		cpu_to_be64(read_csr(dd, DCC_ERR_PORTRCV_ERR_CNT));
	/* rsp->excessive_buffer_overruns - SPC RXE block
	 * (table 13-11 WFR spec) */
	rsp->fm_config_errors =
		cpu_to_be64(read_csr(dd, DCC_ERR_FMCONFIG_ERR_CNT));
	/* rsp->link_error_recovery - DC (table 13-11 WFR spec) */
	/* rsp->link_downed - DC (table 13-11 WFR spec) */
	rsp->uncorrectable_errors = /* XXX why is this only 8 bits? */
		cpu_to_be64(read_csr(dd, DCC_ERR_UNCORRECTABLE_CNT));
	/* rsp->link_quality_indicator ??? */
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
		/* rsp->vls[vfi].port_vl_xmit_pkts ??? (tbl 13-9 WFR spec) */
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
	u64 error_counter_summary = 0;
	/* FIXME
	 * some of the counters are not implemented. if the WFR spec
	 * indicates the source of the value (e.g., driver, DC, etc.)
	 * that's noted. If I don't have a clue how to get the counter,
	 * a '???' appears.
	 */

	/* port_rcv_constraint_errors ??? */
	/* port_rcv_switch_relay_errors is 0 for HFIs */
	/* port_xmit_discards ??? */
	/* port_xmit_constraint_errors - driver (table 13-11 WFR spec) */
	error_counter_summary +=
		cpu_to_be64(read_csr(dd, DCC_ERR_RCVREMOTE_PHY_ERR_CNT));
	/* local_link_integrity_errors - DC (table 13-11 WFR spec)
	 * is this LCB_ERR_INFO_TX_REPLAY_CNT ??? */
	error_counter_summary +=
		cpu_to_be64(read_csr(dd, DCC_ERR_PORTRCV_ERR_CNT));
	/* excessive_buffer_overruns - SPC RXE block
	 * (table 13-11 WFR spec) */
	error_counter_summary +=
		cpu_to_be64(read_csr(dd, DCC_ERR_FMCONFIG_ERR_CNT));
	/* link_error_recovery - DC (table 13-11 WFR spec) */
	/* link_downed - DC (table 13-11 WFR spec) */
	error_counter_summary +=
		cpu_to_be64(read_csr(dd, DCC_ERR_UNCORRECTABLE_CNT));
	/* link_quality_indicator ??? */

	return error_counter_summary;
}

static int pma_get_stl_datacounters(struct stl_pma_mad *pmp,
				    struct ib_device *ibdev, u8 port)
{
	struct stl_port_data_counters_msg *req =
		(struct stl_port_data_counters_msg *)pmp->data;
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
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
	rsp->link_quality_indicator = 0; /* FIXME */

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
		get_error_counter_summary(ibdev, port);

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
		/* rsp->vls[vfi].port_vl_xmit_pkts ??? (tbl 13-9 WFR spec) */
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
	u64 port_mask;
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
	/* rsp->port_xmit_discards = ??? */
	/* rsp->port_xmit_constraint_errors - driver (table 13-11 WFR spec) */
	rsp->port_rcv_remote_physical_errors =
		cpu_to_be64(read_csr(dd, DCC_ERR_RCVREMOTE_PHY_ERR_CNT));
	/* local_link_integrity_errors - DC (table 13-11 WFR spec)
	 * is this LCB_ERR_INFO_TX_REPLAY_CNT ??? */
	/* rsp->excessive_buffer_overruns - SPC RXE block
	 * (table 13-11 WFR spec) */
	rsp->fm_config_errors =
		cpu_to_be64(read_csr(dd, DCC_ERR_FMCONFIG_ERR_CNT));
	/* rsp->link_error_recovery - DC (table 13-11 WFR spec) */
	/* rsp->link_downed - DC (table 13-11 WFR spec) */
	rsp->uncorrectable_errors = /* XXX why is this only 8 bits? */
		cpu_to_be64(read_csr(dd, DCC_ERR_UNCORRECTABLE_CNT));

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

static int pma_set_stl_portstatus(struct stl_pma_mad *pmp,
				  struct ib_device *ibdev, u8 port)
{
	struct stl_clear_port_status *req =
		(struct stl_clear_port_status *)pmp->data;
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	u32 nports = be32_to_cpu(pmp->mad_hdr.attr_mod) >> 24;
	u64 portn = be64_to_cpu(req->port_select_mask[3]);
	u32 counter_select = be32_to_cpu(req->counter_select_mask);
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
	if (counter_select & cs_port_xmit_data)
		write_csr(dd, DCC_PRF_PORT_XMIT_DATA_CNT, 0);

	if (counter_select & cs_port_rcv_data)
		write_csr(dd, DCC_PRF_PORT_RCV_DATA_CNT, 0);

	if (counter_select & cs_port_xmit_pkts)
		write_csr(dd, DCC_PRF_PORT_XMIT_PKTS_CNT, 0);

	if (counter_select & cs_port_rcv_pkts)
		write_csr(dd, DCC_PRF_PORT_RCV_PKTS_CNT, 0);

	if (counter_select & cs_port_mcast_xmit_pkts)
		write_csr(dd, DCC_PRF_PORT_XMIT_MULTICAST_CNT, 0);

	if (counter_select & cs_port_mcast_rcv_pkts)
		write_csr(dd, DCC_PRF_PORT_RCV_MULTICAST_PKT_CNT, 0);

	if (counter_select & cs_port_xmit_wait) {
		offset = SEND_WAIT_CNT * 8 + WFR_SEND_COUNTER_ARRAY64;
		write_csr(dd, offset, 0);
	}

	/* ignore cs_sw_portCongestion for HFIs */

	if (counter_select & cs_port_rcv_fecn)
		write_csr(dd, DCC_PRF_PORT_RCV_FECN_CNT, 0);
	if (counter_select & cs_port_rcv_becn)
		write_csr(dd, DCC_PRF_PORT_RCV_BECN_CNT, 0);

	/* ignore cs_port_xmit_time_cong for HFIs */
	/* ignore cs_port_xmit_wasted_bw for now */
	/* ignore cs_port_xmit_wait_data for now */
	if (counter_select & cs_port_rcv_bubble)
		write_csr(dd, DCC_PRF_PORT_RCV_BUBBLE_CNT, 0);
	if (counter_select & cs_port_mark_fecn)
		write_csr(dd, DCC_PRF_PORT_MARK_FECN_CNT, 0);
	/* ignore cs_port_rcv_constraint_errors for now */
	/* ignore cs_port_rcv_switch_relay_errors for HFIs */
	/* ignore cs_port_xmit_discards for now */
	/* ignore cs_port_xmit_constraint_errors for now */
	if (counter_select & cs_port_rcv_remote_physical_errors)
		write_csr(dd, DCC_ERR_RCVREMOTE_PHY_ERR_CNT, 0);
	/* ignore cs_local_link_integrity_errors for now */
	if (counter_select & cs_port_rcv_errors)
		write_csr(dd, DCC_ERR_PORTRCV_ERR_CNT, 0);
	/* ignore cs_excessive_buffer_overruns for now */
	if (counter_select & cs_fm_config_errors)
		write_csr(dd, DCC_ERR_FMCONFIG_ERR_CNT, 0);
	/* ignore cs_link_error_recovery for now */
	/* ignore cs_link_downed for now */
	if (counter_select & cs_uncorrectable_errors)
		write_csr(dd, DCC_ERR_UNCORRECTABLE_CNT, 0);

	return reply(pmp);
}
#else /* !CONFIG_STL_MGMT */

static int pma_get_classportinfo(struct ib_pma_mad *pmp,
				 struct ib_device *ibdev)
{
	struct ib_class_port_info *p =
		(struct ib_class_port_info *)pmp->data;
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);

	memset(pmp->data, 0, sizeof(pmp->data));

	if (pmp->mad_hdr.attr_mod != 0)
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;

	/* Note that AllPortSelect is not valid */
	p->base_version = 1;
	p->class_version = 1;
	p->capability_mask = IB_PMA_CLASS_CAP_EXT_WIDTH;
	/*
	 * Set the most significant bit of CM2 to indicate support for
	 * congestion statistics
	 */
	p->reserved[0] = dd->psxmitwait_supported << 7;
	/*
	 * Expected response time is 4.096 usec. * 2^18 == 1.073741824 sec.
	 */
	p->resp_time_value = 18;

	return reply((struct ib_smp *) pmp);
}

static int pma_get_portsamplescontrol(struct ib_pma_mad *pmp,
				      struct ib_device *ibdev, u8 port)
{
	struct ib_pma_portsamplescontrol *p =
		(struct ib_pma_portsamplescontrol *)pmp->data;
	struct qib_ibdev *dev = to_idev(ibdev);
	struct hfi_devdata *dd = dd_from_dev(dev);
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	unsigned long flags;
	u8 port_select = p->port_select;

	memset(pmp->data, 0, sizeof(pmp->data));

	p->port_select = port_select;
	if (pmp->mad_hdr.attr_mod != 0 || port_select != port) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		goto bail;
	}
	spin_lock_irqsave(&ibp->lock, flags);
	p->tick = dd->f_get_ib_cfg(ppd, QIB_IB_CFG_PMA_TICKS);
	p->sample_status = dd->f_portcntr(ppd, QIBPORTCNTR_PSSTAT);
	p->counter_width = 4;   /* 32 bit counters */
	p->counter_mask0_9 = COUNTER_MASK0_9;
	p->sample_start = cpu_to_be32(ibp->pma_sample_start);
	p->sample_interval = cpu_to_be32(ibp->pma_sample_interval);
	p->tag = cpu_to_be16(ibp->pma_tag);
	p->counter_select[0] = ibp->pma_counter_select[0];
	p->counter_select[1] = ibp->pma_counter_select[1];
	p->counter_select[2] = ibp->pma_counter_select[2];
	p->counter_select[3] = ibp->pma_counter_select[3];
	p->counter_select[4] = ibp->pma_counter_select[4];
	spin_unlock_irqrestore(&ibp->lock, flags);

bail:
	return reply((struct ib_smp *) pmp);
}

static int pma_set_portsamplescontrol(struct ib_pma_mad *pmp,
				      struct ib_device *ibdev, u8 port)
{
	struct ib_pma_portsamplescontrol *p =
		(struct ib_pma_portsamplescontrol *)pmp->data;
	struct qib_ibdev *dev = to_idev(ibdev);
	struct hfi_devdata *dd = dd_from_dev(dev);
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	unsigned long flags;
	u8 status, xmit_flags;
	int ret;

	if (pmp->mad_hdr.attr_mod != 0 || p->port_select != port) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		ret = reply((struct ib_smp *) pmp);
		goto bail;
	}

	spin_lock_irqsave(&ibp->lock, flags);

	/* Port Sampling code owns the PS* HW counters */
	xmit_flags = ppd->cong_stats.flags;
	ppd->cong_stats.flags = IB_PMA_CONG_HW_CONTROL_SAMPLE;
	status = dd->f_portcntr(ppd, QIBPORTCNTR_PSSTAT);
	if (status == IB_PMA_SAMPLE_STATUS_DONE ||
	    (status == IB_PMA_SAMPLE_STATUS_RUNNING &&
	     xmit_flags == IB_PMA_CONG_HW_CONTROL_TIMER)) {
		ibp->pma_sample_start = be32_to_cpu(p->sample_start);
		ibp->pma_sample_interval = be32_to_cpu(p->sample_interval);
		ibp->pma_tag = be16_to_cpu(p->tag);
		ibp->pma_counter_select[0] = p->counter_select[0];
		ibp->pma_counter_select[1] = p->counter_select[1];
		ibp->pma_counter_select[2] = p->counter_select[2];
		ibp->pma_counter_select[3] = p->counter_select[3];
		ibp->pma_counter_select[4] = p->counter_select[4];
		dd->f_set_cntr_sample(ppd, ibp->pma_sample_interval,
				      ibp->pma_sample_start);
	}
	spin_unlock_irqrestore(&ibp->lock, flags);

	ret = pma_get_portsamplescontrol(pmp, ibdev, port);

bail:
	return ret;
}
#endif /* !CONFIG_STL_MGMT */

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

#ifndef CONFIG_STL_MGMT
static u64 get_cache_hw_sample_counters(struct qib_pportdata *ppd,
					__be16 sel)
{
	u64 ret;

	switch (sel) {
	case IB_PMA_PORT_XMIT_DATA:
		ret = ppd->cong_stats.counter_cache.psxmitdata;
		break;
	case IB_PMA_PORT_RCV_DATA:
		ret = ppd->cong_stats.counter_cache.psrcvdata;
		break;
	case IB_PMA_PORT_XMIT_PKTS:
		ret = ppd->cong_stats.counter_cache.psxmitpkts;
		break;
	case IB_PMA_PORT_RCV_PKTS:
		ret = ppd->cong_stats.counter_cache.psrcvpkts;
		break;
	case IB_PMA_PORT_XMIT_WAIT:
		ret = ppd->cong_stats.counter_cache.psxmitwait;
		break;
	default:
		ret = 0;
	}

	return ret;
}

static int pma_get_portsamplesresult(struct ib_pma_mad *pmp,
				     struct ib_device *ibdev, u8 port)
{
	struct ib_pma_portsamplesresult *p =
		(struct ib_pma_portsamplesresult *)pmp->data;
	struct qib_ibdev *dev = to_idev(ibdev);
	struct hfi_devdata *dd = dd_from_dev(dev);
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	unsigned long flags;
	u8 status;
	int i;

	memset(pmp->data, 0, sizeof(pmp->data));
	spin_lock_irqsave(&ibp->lock, flags);
	p->tag = cpu_to_be16(ibp->pma_tag);
	if (ppd->cong_stats.flags == IB_PMA_CONG_HW_CONTROL_TIMER)
		p->sample_status = IB_PMA_SAMPLE_STATUS_DONE;
	else {
		status = dd->f_portcntr(ppd, QIBPORTCNTR_PSSTAT);
		p->sample_status = cpu_to_be16(status);
		if (status == IB_PMA_SAMPLE_STATUS_DONE) {
			cache_hw_sample_counters(ppd);
			ppd->cong_stats.counter =
				xmit_wait_get_value_delta(ppd);
			dd->f_set_cntr_sample(ppd,
					      QIB_CONG_TIMER_PSINTERVAL, 0);
			ppd->cong_stats.flags = IB_PMA_CONG_HW_CONTROL_TIMER;
		}
	}
	for (i = 0; i < ARRAY_SIZE(ibp->pma_counter_select); i++)
		p->counter[i] = cpu_to_be32(
			get_cache_hw_sample_counters(
				ppd, ibp->pma_counter_select[i]));
	spin_unlock_irqrestore(&ibp->lock, flags);

	return reply((struct ib_smp *) pmp);
}

static int pma_get_portsamplesresult_ext(struct ib_pma_mad *pmp,
					 struct ib_device *ibdev, u8 port)
{
	struct ib_pma_portsamplesresult_ext *p =
		(struct ib_pma_portsamplesresult_ext *)pmp->data;
	struct qib_ibdev *dev = to_idev(ibdev);
	struct hfi_devdata *dd = dd_from_dev(dev);
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	unsigned long flags;
	u8 status;
	int i;

	/* Port Sampling code owns the PS* HW counters */
	memset(pmp->data, 0, sizeof(pmp->data));
	spin_lock_irqsave(&ibp->lock, flags);
	p->tag = cpu_to_be16(ibp->pma_tag);
	if (ppd->cong_stats.flags == IB_PMA_CONG_HW_CONTROL_TIMER)
		p->sample_status = IB_PMA_SAMPLE_STATUS_DONE;
	else {
		status = dd->f_portcntr(ppd, QIBPORTCNTR_PSSTAT);
		p->sample_status = cpu_to_be16(status);
		/* 64 bits */
		p->extended_width = cpu_to_be32(0x80000000);
		if (status == IB_PMA_SAMPLE_STATUS_DONE) {
			cache_hw_sample_counters(ppd);
			ppd->cong_stats.counter =
				xmit_wait_get_value_delta(ppd);
			dd->f_set_cntr_sample(ppd,
					      QIB_CONG_TIMER_PSINTERVAL, 0);
			ppd->cong_stats.flags = IB_PMA_CONG_HW_CONTROL_TIMER;
		}
	}
	for (i = 0; i < ARRAY_SIZE(ibp->pma_counter_select); i++)
		p->counter[i] = cpu_to_be64(
			get_cache_hw_sample_counters(
				ppd, ibp->pma_counter_select[i]));
	spin_unlock_irqrestore(&ibp->lock, flags);

	return reply((struct ib_smp *) pmp);
}

static int pma_get_portcounters(struct ib_pma_mad *pmp,
				struct ib_device *ibdev, u8 port)
{
	struct ib_pma_portcounters *p = (struct ib_pma_portcounters *)
		pmp->data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	struct qib_verbs_counters cntrs;
	u8 port_select = p->port_select;

	qib_get_counters(ppd, &cntrs);

	/* Adjust counters for any resets done. */
	cntrs.symbol_error_counter -= ibp->z_symbol_error_counter;
	cntrs.link_error_recovery_counter -=
		ibp->z_link_error_recovery_counter;
	cntrs.link_downed_counter -= ibp->z_link_downed_counter;
	cntrs.port_rcv_errors -= ibp->z_port_rcv_errors;
	cntrs.port_rcv_remphys_errors -= ibp->z_port_rcv_remphys_errors;
	cntrs.port_xmit_discards -= ibp->z_port_xmit_discards;
	cntrs.port_xmit_data -= ibp->z_port_xmit_data;
	cntrs.port_rcv_data -= ibp->z_port_rcv_data;
	cntrs.port_xmit_packets -= ibp->z_port_xmit_packets;
	cntrs.port_rcv_packets -= ibp->z_port_rcv_packets;
	cntrs.local_link_integrity_errors -=
		ibp->z_local_link_integrity_errors;
	cntrs.excessive_buffer_overrun_errors -=
		ibp->z_excessive_buffer_overrun_errors;
	cntrs.vl15_dropped -= ibp->z_vl15_dropped;
	cntrs.vl15_dropped += ibp->n_vl15_dropped;

	memset(pmp->data, 0, sizeof(pmp->data));

	p->port_select = port_select;
	if (pmp->mad_hdr.attr_mod != 0 || port_select != port)
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;

	if (cntrs.symbol_error_counter > 0xFFFFUL)
		p->symbol_error_counter = cpu_to_be16(0xFFFF);
	else
		p->symbol_error_counter =
			cpu_to_be16((u16)cntrs.symbol_error_counter);
	if (cntrs.link_error_recovery_counter > 0xFFUL)
		p->link_error_recovery_counter = 0xFF;
	else
		p->link_error_recovery_counter =
			(u8)cntrs.link_error_recovery_counter;
	if (cntrs.link_downed_counter > 0xFFUL)
		p->link_downed_counter = 0xFF;
	else
		p->link_downed_counter = (u8)cntrs.link_downed_counter;
	if (cntrs.port_rcv_errors > 0xFFFFUL)
		p->port_rcv_errors = cpu_to_be16(0xFFFF);
	else
		p->port_rcv_errors =
			cpu_to_be16((u16) cntrs.port_rcv_errors);
	if (cntrs.port_rcv_remphys_errors > 0xFFFFUL)
		p->port_rcv_remphys_errors = cpu_to_be16(0xFFFF);
	else
		p->port_rcv_remphys_errors =
			cpu_to_be16((u16)cntrs.port_rcv_remphys_errors);
	if (cntrs.port_xmit_discards > 0xFFFFUL)
		p->port_xmit_discards = cpu_to_be16(0xFFFF);
	else
		p->port_xmit_discards =
			cpu_to_be16((u16)cntrs.port_xmit_discards);
	if (cntrs.local_link_integrity_errors > 0xFUL)
		cntrs.local_link_integrity_errors = 0xFUL;
	if (cntrs.excessive_buffer_overrun_errors > 0xFUL)
		cntrs.excessive_buffer_overrun_errors = 0xFUL;
	p->link_overrun_errors = (cntrs.local_link_integrity_errors << 4) |
		cntrs.excessive_buffer_overrun_errors;
	if (cntrs.vl15_dropped > 0xFFFFUL)
		p->vl15_dropped = cpu_to_be16(0xFFFF);
	else
		p->vl15_dropped = cpu_to_be16((u16)cntrs.vl15_dropped);
	if (cntrs.port_xmit_data > 0xFFFFFFFFUL)
		p->port_xmit_data = cpu_to_be32(0xFFFFFFFF);
	else
		p->port_xmit_data = cpu_to_be32((u32)cntrs.port_xmit_data);
	if (cntrs.port_rcv_data > 0xFFFFFFFFUL)
		p->port_rcv_data = cpu_to_be32(0xFFFFFFFF);
	else
		p->port_rcv_data = cpu_to_be32((u32)cntrs.port_rcv_data);
	if (cntrs.port_xmit_packets > 0xFFFFFFFFUL)
		p->port_xmit_packets = cpu_to_be32(0xFFFFFFFF);
	else
		p->port_xmit_packets =
			cpu_to_be32((u32)cntrs.port_xmit_packets);
	if (cntrs.port_rcv_packets > 0xFFFFFFFFUL)
		p->port_rcv_packets = cpu_to_be32(0xFFFFFFFF);
	else
		p->port_rcv_packets =
			cpu_to_be32((u32) cntrs.port_rcv_packets);

	return reply((struct ib_smp *) pmp);
}

static int pma_get_portcounters_cong(struct ib_pma_mad *pmp,
				     struct ib_device *ibdev, u8 port)
{
	/* Congestion PMA packets start at offset 24 not 64 */
	struct ib_pma_portcounters_cong *p =
		(struct ib_pma_portcounters_cong *)pmp->reserved;
	struct qib_verbs_counters cntrs;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	struct hfi_devdata *dd = dd_from_ppd(ppd);
	u32 port_select = be32_to_cpu(pmp->mad_hdr.attr_mod) & 0xFF;
	u64 xmit_wait_counter;
	unsigned long flags;

	/*
	 * This check is performed only in the GET method because the
	 * SET method ends up calling this anyway.
	 */
	if (!dd->psxmitwait_supported)
		pmp->mad_hdr.status |= IB_SMP_UNSUP_METH_ATTR;
	if (port_select != port)
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;

	qib_get_counters(ppd, &cntrs);
	spin_lock_irqsave(&ppd->ibport_data.lock, flags);
	xmit_wait_counter = xmit_wait_get_value_delta(ppd);
	spin_unlock_irqrestore(&ppd->ibport_data.lock, flags);

	/* Adjust counters for any resets done. */
	cntrs.symbol_error_counter -= ibp->z_symbol_error_counter;
	cntrs.link_error_recovery_counter -=
		ibp->z_link_error_recovery_counter;
	cntrs.link_downed_counter -= ibp->z_link_downed_counter;
	cntrs.port_rcv_errors -= ibp->z_port_rcv_errors;
	cntrs.port_rcv_remphys_errors -=
		ibp->z_port_rcv_remphys_errors;
	cntrs.port_xmit_discards -= ibp->z_port_xmit_discards;
	cntrs.local_link_integrity_errors -=
		ibp->z_local_link_integrity_errors;
	cntrs.excessive_buffer_overrun_errors -=
		ibp->z_excessive_buffer_overrun_errors;
	cntrs.vl15_dropped -= ibp->z_vl15_dropped;
	cntrs.vl15_dropped += ibp->n_vl15_dropped;
	cntrs.port_xmit_data -= ibp->z_port_xmit_data;
	cntrs.port_rcv_data -= ibp->z_port_rcv_data;
	cntrs.port_xmit_packets -= ibp->z_port_xmit_packets;
	cntrs.port_rcv_packets -= ibp->z_port_rcv_packets;

	memset(pmp->reserved, 0, sizeof(pmp->reserved) +
	       sizeof(pmp->data));

	/*
	 * Set top 3 bits to indicate interval in picoseconds in
	 * remaining bits.
	 */
	p->port_check_rate =
		cpu_to_be16((QIB_XMIT_RATE_PICO << 13) |
			    (dd->psxmitwait_check_rate &
			     ~(QIB_XMIT_RATE_PICO << 13)));
	p->port_adr_events = cpu_to_be64(0);
	p->port_xmit_wait = cpu_to_be64(xmit_wait_counter);
	p->port_xmit_data = cpu_to_be64(cntrs.port_xmit_data);
	p->port_rcv_data = cpu_to_be64(cntrs.port_rcv_data);
	p->port_xmit_packets =
		cpu_to_be64(cntrs.port_xmit_packets);
	p->port_rcv_packets =
		cpu_to_be64(cntrs.port_rcv_packets);
	if (cntrs.symbol_error_counter > 0xFFFFUL)
		p->symbol_error_counter = cpu_to_be16(0xFFFF);
	else
		p->symbol_error_counter =
			cpu_to_be16(
				(u16)cntrs.symbol_error_counter);
	if (cntrs.link_error_recovery_counter > 0xFFUL)
		p->link_error_recovery_counter = 0xFF;
	else
		p->link_error_recovery_counter =
			(u8)cntrs.link_error_recovery_counter;
	if (cntrs.link_downed_counter > 0xFFUL)
		p->link_downed_counter = 0xFF;
	else
		p->link_downed_counter =
			(u8)cntrs.link_downed_counter;
	if (cntrs.port_rcv_errors > 0xFFFFUL)
		p->port_rcv_errors = cpu_to_be16(0xFFFF);
	else
		p->port_rcv_errors =
			cpu_to_be16((u16) cntrs.port_rcv_errors);
	if (cntrs.port_rcv_remphys_errors > 0xFFFFUL)
		p->port_rcv_remphys_errors = cpu_to_be16(0xFFFF);
	else
		p->port_rcv_remphys_errors =
			cpu_to_be16(
				(u16)cntrs.port_rcv_remphys_errors);
	if (cntrs.port_xmit_discards > 0xFFFFUL)
		p->port_xmit_discards = cpu_to_be16(0xFFFF);
	else
		p->port_xmit_discards =
			cpu_to_be16((u16)cntrs.port_xmit_discards);
	if (cntrs.local_link_integrity_errors > 0xFUL)
		cntrs.local_link_integrity_errors = 0xFUL;
	if (cntrs.excessive_buffer_overrun_errors > 0xFUL)
		cntrs.excessive_buffer_overrun_errors = 0xFUL;
	p->link_overrun_errors = (cntrs.local_link_integrity_errors << 4) |
		cntrs.excessive_buffer_overrun_errors;
	if (cntrs.vl15_dropped > 0xFFFFUL)
		p->vl15_dropped = cpu_to_be16(0xFFFF);
	else
		p->vl15_dropped = cpu_to_be16((u16)cntrs.vl15_dropped);

	return reply((struct ib_smp *)pmp);
}

static int pma_get_portcounters_ext(struct ib_pma_mad *pmp,
				    struct ib_device *ibdev, u8 port)
{
	struct ib_pma_portcounters_ext *p =
		(struct ib_pma_portcounters_ext *)pmp->data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	u64 swords, rwords, spkts, rpkts, xwait;
	u8 port_select = p->port_select;

	memset(pmp->data, 0, sizeof(pmp->data));

	p->port_select = port_select;
	if (pmp->mad_hdr.attr_mod != 0 || port_select != port) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		goto bail;
	}

	qib_snapshot_counters(ppd, &swords, &rwords, &spkts, &rpkts, &xwait);

	/* Adjust counters for any resets done. */
	swords -= ibp->z_port_xmit_data;
	rwords -= ibp->z_port_rcv_data;
	spkts -= ibp->z_port_xmit_packets;
	rpkts -= ibp->z_port_rcv_packets;

	p->port_xmit_data = cpu_to_be64(swords);
	p->port_rcv_data = cpu_to_be64(rwords);
	p->port_xmit_packets = cpu_to_be64(spkts);
	p->port_rcv_packets = cpu_to_be64(rpkts);
	p->port_unicast_xmit_packets = cpu_to_be64(ibp->n_unicast_xmit);
	p->port_unicast_rcv_packets = cpu_to_be64(ibp->n_unicast_rcv);
	p->port_multicast_xmit_packets = cpu_to_be64(ibp->n_multicast_xmit);
	p->port_multicast_rcv_packets = cpu_to_be64(ibp->n_multicast_rcv);

bail:
	return reply((struct ib_smp *) pmp);
}

static int pma_set_portcounters(struct ib_pma_mad *pmp,
				struct ib_device *ibdev, u8 port)
{
	struct ib_pma_portcounters *p = (struct ib_pma_portcounters *)
		pmp->data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	struct qib_verbs_counters cntrs;

	/*
	 * Since the HW doesn't support clearing counters, we save the
	 * current count and subtract it from future responses.
	 */
	qib_get_counters(ppd, &cntrs);

	if (p->counter_select & IB_PMA_SEL_SYMBOL_ERROR)
		ibp->z_symbol_error_counter = cntrs.symbol_error_counter;

	if (p->counter_select & IB_PMA_SEL_LINK_ERROR_RECOVERY)
		ibp->z_link_error_recovery_counter =
			cntrs.link_error_recovery_counter;

	if (p->counter_select & IB_PMA_SEL_LINK_DOWNED)
		ibp->z_link_downed_counter = cntrs.link_downed_counter;

	if (p->counter_select & IB_PMA_SEL_PORT_RCV_ERRORS)
		ibp->z_port_rcv_errors = cntrs.port_rcv_errors;

	if (p->counter_select & IB_PMA_SEL_PORT_RCV_REMPHYS_ERRORS)
		ibp->z_port_rcv_remphys_errors =
			cntrs.port_rcv_remphys_errors;

	if (p->counter_select & IB_PMA_SEL_PORT_XMIT_DISCARDS)
		ibp->z_port_xmit_discards = cntrs.port_xmit_discards;

	if (p->counter_select & IB_PMA_SEL_LOCAL_LINK_INTEGRITY_ERRORS)
		ibp->z_local_link_integrity_errors =
			cntrs.local_link_integrity_errors;

	if (p->counter_select & IB_PMA_SEL_EXCESSIVE_BUFFER_OVERRUNS)
		ibp->z_excessive_buffer_overrun_errors =
			cntrs.excessive_buffer_overrun_errors;

	if (p->counter_select & IB_PMA_SEL_PORT_VL15_DROPPED) {
		ibp->n_vl15_dropped = 0;
		ibp->z_vl15_dropped = cntrs.vl15_dropped;
	}

	if (p->counter_select & IB_PMA_SEL_PORT_XMIT_DATA)
		ibp->z_port_xmit_data = cntrs.port_xmit_data;

	if (p->counter_select & IB_PMA_SEL_PORT_RCV_DATA)
		ibp->z_port_rcv_data = cntrs.port_rcv_data;

	if (p->counter_select & IB_PMA_SEL_PORT_XMIT_PACKETS)
		ibp->z_port_xmit_packets = cntrs.port_xmit_packets;

	if (p->counter_select & IB_PMA_SEL_PORT_RCV_PACKETS)
		ibp->z_port_rcv_packets = cntrs.port_rcv_packets;

	return pma_get_portcounters(pmp, ibdev, port);
}

static int pma_set_portcounters_cong(struct ib_pma_mad *pmp,
				     struct ib_device *ibdev, u8 port)
{
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	struct hfi_devdata *dd = dd_from_ppd(ppd);
	struct qib_verbs_counters cntrs;
	u32 counter_select = (be32_to_cpu(pmp->mad_hdr.attr_mod) >> 24) & 0xFF;
	int ret = 0;
	unsigned long flags;

	qib_get_counters(ppd, &cntrs);
	/* Get counter values before we save them */
	ret = pma_get_portcounters_cong(pmp, ibdev, port);

	if (counter_select & IB_PMA_SEL_CONG_XMIT) {
		spin_lock_irqsave(&ppd->ibport_data.lock, flags);
		ppd->cong_stats.counter = 0;
		dd->f_set_cntr_sample(ppd, QIB_CONG_TIMER_PSINTERVAL,
				      0x0);
		spin_unlock_irqrestore(&ppd->ibport_data.lock, flags);
	}
	if (counter_select & IB_PMA_SEL_CONG_PORT_DATA) {
		ibp->z_port_xmit_data = cntrs.port_xmit_data;
		ibp->z_port_rcv_data = cntrs.port_rcv_data;
		ibp->z_port_xmit_packets = cntrs.port_xmit_packets;
		ibp->z_port_rcv_packets = cntrs.port_rcv_packets;
	}
	if (counter_select & IB_PMA_SEL_CONG_ALL) {
		ibp->z_symbol_error_counter =
			cntrs.symbol_error_counter;
		ibp->z_link_error_recovery_counter =
			cntrs.link_error_recovery_counter;
		ibp->z_link_downed_counter =
			cntrs.link_downed_counter;
		ibp->z_port_rcv_errors = cntrs.port_rcv_errors;
		ibp->z_port_rcv_remphys_errors =
			cntrs.port_rcv_remphys_errors;
		ibp->z_port_xmit_discards =
			cntrs.port_xmit_discards;
		ibp->z_local_link_integrity_errors =
			cntrs.local_link_integrity_errors;
		ibp->z_excessive_buffer_overrun_errors =
			cntrs.excessive_buffer_overrun_errors;
		ibp->n_vl15_dropped = 0;
		ibp->z_vl15_dropped = cntrs.vl15_dropped;
	}

	return ret;
}

static int pma_set_portcounters_ext(struct ib_pma_mad *pmp,
				    struct ib_device *ibdev, u8 port)
{
	struct ib_pma_portcounters *p = (struct ib_pma_portcounters *)
		pmp->data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	u64 swords, rwords, spkts, rpkts, xwait;

	qib_snapshot_counters(ppd, &swords, &rwords, &spkts, &rpkts, &xwait);

	if (p->counter_select & IB_PMA_SELX_PORT_XMIT_DATA)
		ibp->z_port_xmit_data = swords;

	if (p->counter_select & IB_PMA_SELX_PORT_RCV_DATA)
		ibp->z_port_rcv_data = rwords;

	if (p->counter_select & IB_PMA_SELX_PORT_XMIT_PACKETS)
		ibp->z_port_xmit_packets = spkts;

	if (p->counter_select & IB_PMA_SELX_PORT_RCV_PACKETS)
		ibp->z_port_rcv_packets = rpkts;

	if (p->counter_select & IB_PMA_SELX_PORT_UNI_XMIT_PACKETS)
		ibp->n_unicast_xmit = 0;

	if (p->counter_select & IB_PMA_SELX_PORT_UNI_RCV_PACKETS)
		ibp->n_unicast_rcv = 0;

	if (p->counter_select & IB_PMA_SELX_PORT_MULTI_XMIT_PACKETS)
		ibp->n_multicast_xmit = 0;

	if (p->counter_select & IB_PMA_SELX_PORT_MULTI_RCV_PACKETS)
		ibp->n_multicast_rcv = 0;

	return pma_get_portcounters_ext(pmp, ibdev, port);
}
#endif /* !CONFIG_STL_MGMT */

#ifdef CONFIG_STL_MGMT

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
	memset(p, 0, sizeof(*p));

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

	memset(p, 0, sizeof(*p));

	spin_lock(&ppd->cc_shadow_lock);

	if (!ppd->congestion_entries_shadow) {
		spin_unlock(&ppd->cc_shadow_lock);
		return reply(smp);
	}

	entries = ppd->congestion_entries_shadow->entries;
	p->port_control = cpu_to_be16(
		ppd->congestion_entries_shadow->port_control);
	p->control_map = cpu_to_be32(
		ppd->congestion_entries_shadow->control_map);
	for (i = 0; i < STL_MAX_SLS; i++) {
		p->entries[i].ccti_increase = entries[i].ccti_increase;
		p->entries[i].ccti_timer = cpu_to_be16(entries[i].ccti_timer);
		p->entries[i].trigger_threshold =
			entries[i].trigger_threshold;
		p->entries[i].ccti_min = entries[i].ccti_min;
	}

	spin_unlock(&ppd->cc_shadow_lock);

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
	int i;

	ppd->cc_sl_control_map = be32_to_cpu(p->control_map);

	for (i = 0; i < STL_MAX_SLS; i++) {
		ppd->congestion_entries[i].ccti_increase =
			p->entries[i].ccti_increase;

		ppd->congestion_entries[i].ccti_timer =
			be16_to_cpu(p->entries[i].ccti_timer);

		ppd->congestion_entries[i].trigger_threshold =
			p->entries[i].trigger_threshold;

		ppd->congestion_entries[i].ccti_min =
			p->entries[i].ccti_min;
	}

	return __subn_get_stl_cong_setting(smp, am, data, ibdev, port,
					   resp_len);
}

static int __subn_get_stl_cc_table(struct stl_smp *smp, u32 am, u8 *data,
				   struct ib_device *ibdev, u8 port,
				   u32 *resp_len)
{
	struct ib_cc_table_attr *p =
		(struct ib_cc_table_attr *) data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	u32 cct_block_index = am & 0xff;
	u32 n_blocks_req = (am & 0xff000000) >> 24;
	u32 max_cct_block;
	u32 cct_entry;
	struct ib_cc_table_entry_shadow *entries;
	int i;

	memset(p, 0, sizeof(*p));

	if (cct_block_index > ppd->cc_max_table_entries) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply_failure(smp);
	}

	if (!ppd->congestion_entries_shadow)
		return reply(smp);

	spin_lock(&ppd->cc_shadow_lock);

	max_cct_block =
		(ppd->ccti_entries_shadow->ccti_last_entry + 1)/IB_CCT_ENTRIES;
	max_cct_block = max_cct_block ? max_cct_block - 1 : 0;

	if (cct_block_index + n_blocks_req - 1 > max_cct_block) {
		smp->status |= IB_SMP_INVALID_FIELD;
		spin_unlock(&ppd->cc_shadow_lock);
		return reply_failure(smp);
	}

	cct_entry = IB_CCT_ENTRIES * n_blocks_req;

	cct_entry--;

	p->ccti_limit = cpu_to_be16(cct_entry);

	entries = &ppd->ccti_entries_shadow->
		entries[IB_CCT_ENTRIES * cct_block_index];

	for (i = 0; i <= cct_entry; i++)
		p->ccti_entries[i].entry = cpu_to_be16(entries[i].entry);

	spin_unlock(&ppd->cc_shadow_lock);

	if (resp_len)
		*resp_len += sizeof(u16)*(IB_CCT_ENTRIES * n_blocks_req + 1);

	return reply(smp);
}

static int __subn_set_stl_cc_table(struct stl_smp *smp, u32 am, u8 *data,
				   struct ib_device *ibdev, u8 port,
				   u32 *resp_len)
{
	struct ib_cc_table_attr *p = (struct ib_cc_table_attr *) data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	u32 cct_block_index = am & 0xff;
	u32 n_blocks_req = (am & 0xff000000) >> 24;
	u32 cct_entry;
	struct ib_cc_table_entry_shadow *entries;
	int i;

	/* Is the table index more than what is supported? */
	if (cct_block_index > ppd->cc_max_table_entries)
		goto bail;

	if (!ppd->congestion_entries_shadow)
		return reply(smp);

	/* If this packet is the first in the sequence then
	 * zero the total table entry count.
	 */
	if (be16_to_cpu(p->ccti_limit) < IB_CCT_ENTRIES)
		ppd->total_cct_entry = 0;

	cct_entry = ((n_blocks_req - 1) * IB_CCT_ENTRIES) +
		    (be16_to_cpu(p->ccti_limit))%IB_CCT_ENTRIES;

	if (ppd->total_cct_entry + (cct_entry + 1) >
	    ppd->cc_supported_table_entries)
		goto bail;

	ppd->total_cct_entry += (cct_entry + 1);

	ppd->ccti_limit = be16_to_cpu(p->ccti_limit);

	entries = ppd->ccti_entries + (IB_CCT_ENTRIES * cct_block_index);

	for (i = 0; i <= cct_entry; i++)
		entries[i].entry = be16_to_cpu(p->ccti_entries[i].entry);

	spin_lock(&ppd->cc_shadow_lock);

	ppd->ccti_entries_shadow->ccti_last_entry = ppd->total_cct_entry - 1;
	memcpy(ppd->ccti_entries_shadow->entries, ppd->ccti_entries,
	       (ppd->total_cct_entry * sizeof(struct ib_cc_table_entry)));

	ppd->congestion_entries_shadow->port_control = IB_CC_CCS_PC_SL_BASED;
	ppd->congestion_entries_shadow->control_map = ppd->cc_sl_control_map;
	memcpy(ppd->congestion_entries_shadow->entries, ppd->congestion_entries,
	       STL_MAX_SLS * sizeof(struct ib_cc_congestion_entry));
	spin_unlock(&ppd->cc_shadow_lock);

	return __subn_get_stl_cc_table(smp, am, data, ibdev, port, resp_len);

bail:
	return reply_failure(smp);
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
	case STL_ATTRIB_ID_SC_TO_VLT_MAP:
		ret = __subn_get_stl_sc_to_vlt(smp, am, data, ibdev, port,
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
	case STL_ATTRIB_ID_SC_TO_VLT_MAP:
		ret = __subn_set_stl_sc_to_vlt(smp, am, data, ibdev, port,
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
#if 0
		case IB_SMP_ATTR_SL_TO_VL_TABLE:
			ret = subn_get_sl_to_vl(smp, ibdev, port);
			goto bail;
#endif /* 01 */
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
#if 0
		case IB_SMP_ATTR_SL_TO_VL_TABLE:
			ret = subn_set_sl_to_vl(smp, ibdev, port);
			goto bail;
#endif /* 01 */
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
#endif /* CONFIG_STL_MGMT */

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
#ifndef CONFIG_STL_MGMT
		case IB_SMP_ATTR_NODE_DESC:
			ret = subn_get_nodedescription(smp, ibdev);
			goto bail;
#endif /* !CONFIG_STL_MGMT */
		case IB_SMP_ATTR_NODE_INFO:
			ret = subn_get_nodeinfo(smp, ibdev, port);
			goto bail;
#ifndef CONFIG_STL_MGMT
		case IB_SMP_ATTR_GUID_INFO:
			ret = subn_get_guidinfo(smp, ibdev, port);
			goto bail;
		case IB_SMP_ATTR_PORT_INFO:
			ret = subn_get_portinfo(smp, ibdev, port);
			goto bail;
		case IB_SMP_ATTR_PKEY_TABLE:
			ret = subn_get_pkeytable(smp, ibdev, port);
			goto bail;
		case IB_SMP_ATTR_SL_TO_VL_TABLE:
			ret = subn_get_sl_to_vl(smp, ibdev, port);
			goto bail;
		case IB_SMP_ATTR_VL_ARB_TABLE:
			ret = subn_get_vl_arb(smp, ibdev, port);
			goto bail;
		case IB_SMP_ATTR_SM_INFO:
			if (ibp->port_cap_flags & IB_PORT_SM_DISABLED) {
				ret = IB_MAD_RESULT_SUCCESS |
					IB_MAD_RESULT_CONSUMED;
				goto bail;
			}
			if (ibp->port_cap_flags & IB_PORT_SM) {
				ret = IB_MAD_RESULT_SUCCESS;
				goto bail;
			}
			/* FALLTHROUGH */
#endif /* !CONFIG_STL_MGMT */
		default:
			smp->status |= IB_SMP_UNSUP_METH_ATTR;
			ret = reply(smp);
			goto bail;
		}

#ifndef CONFIG_STL_MGMT
	case IB_MGMT_METHOD_SET:
		switch (smp->attr_id) {
		case IB_SMP_ATTR_GUID_INFO:
			ret = subn_set_guidinfo(smp, ibdev, port);
			goto bail;
		case IB_SMP_ATTR_PORT_INFO:
			ret = subn_set_portinfo(smp, ibdev, port);
			goto bail;
		case IB_SMP_ATTR_PKEY_TABLE:
			ret = subn_set_pkeytable(smp, ibdev, port);
			goto bail;
		case IB_SMP_ATTR_SL_TO_VL_TABLE:
			ret = subn_set_sl_to_vl(smp, ibdev, port);
			goto bail;
		case IB_SMP_ATTR_VL_ARB_TABLE:
			ret = subn_set_vl_arb(smp, ibdev, port);
			goto bail;
		case IB_SMP_ATTR_SM_INFO:
			if (ibp->port_cap_flags & IB_PORT_SM_DISABLED) {
				ret = IB_MAD_RESULT_SUCCESS |
					IB_MAD_RESULT_CONSUMED;
				goto bail;
			}
			if (ibp->port_cap_flags & IB_PORT_SM) {
				ret = IB_MAD_RESULT_SUCCESS;
				goto bail;
			}
			/* FALLTHROUGH */
		default:
			smp->status |= IB_SMP_UNSUP_METH_ATTR;
			ret = reply(smp);
			goto bail;
		}

	case IB_MGMT_METHOD_TRAP_REPRESS:
		if (smp->attr_id == IB_SMP_ATTR_NOTICE)
			ret = subn_trap_repress(smp, ibdev, port);
		else {
			smp->status |= IB_SMP_UNSUP_METH_ATTR;
			ret = reply(smp);
		}
		goto bail;

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

	case IB_MGMT_METHOD_SEND:
	{
		struct qib_pportdata *ppd = ppd_from_ibp(ibp);
		if (ib_get_smp_direction(smp) &&
		    smp->attr_id == QIB_VENDOR_IPG) {
			ppd->dd->f_set_ib_cfg(ppd, QIB_IB_CFG_PORT,
					      smp->data[0]);
			ret = IB_MAD_RESULT_SUCCESS | IB_MAD_RESULT_CONSUMED;
		} else
			ret = IB_MAD_RESULT_SUCCESS;
		goto bail;
	}
	default:
		smp->status |= IB_SMP_UNSUP_METHOD;
		ret = reply(smp);
#endif /* !CONFIG_STL_MGMT */
	}

bail:
	return ret;
}

#ifdef CONFIG_STL_MGMT
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
#else /* !CONFIG_STL_MGMT */

static int process_perf(struct ib_device *ibdev, u8 port,
			struct ib_mad *in_mad,
			struct ib_mad *out_mad)
{
	struct ib_pma_mad *pmp = (struct ib_pma_mad *)out_mad;
	int ret;

	*out_mad = *in_mad;
	if (pmp->mad_hdr.class_version != 1) {
		pmp->mad_hdr.status |= IB_SMP_UNSUP_VERSION;
		ret = reply((struct ib_smp *) pmp);
		goto bail;
	}

	switch (pmp->mad_hdr.method) {
	case IB_MGMT_METHOD_GET:
		switch (pmp->mad_hdr.attr_id) {
		case IB_PMA_CLASS_PORT_INFO:
			ret = pma_get_classportinfo(pmp, ibdev);
			goto bail;
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
		default:
			pmp->mad_hdr.status |= IB_SMP_UNSUP_METH_ATTR;
			ret = reply((struct ib_smp *) pmp);
			goto bail;
		}

	case IB_MGMT_METHOD_SET:
		switch (pmp->mad_hdr.attr_id) {
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
		default:
			pmp->mad_hdr.status |= IB_SMP_UNSUP_METH_ATTR;
			ret = reply((struct ib_smp *) pmp);
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
		ret = reply((struct ib_smp *) pmp);
	}

bail:
	return ret;
}

static int cc_get_classportinfo(struct ib_cc_mad *ccp,
				struct ib_device *ibdev)
{
	struct ib_cc_classportinfo_attr *p =
		(struct ib_cc_classportinfo_attr *)ccp->mgmt_data;

	memset(ccp->mgmt_data, 0, sizeof(ccp->mgmt_data));

	p->base_version = 1;
	p->class_version = 1;
	p->cap_mask = 0;

	/*
	 * Expected response time is 4.096 usec. * 2^18 == 1.073741824 sec.
	 */
	p->resp_time_value = 18;

	return reply((struct ib_smp *) ccp);
}

static int cc_get_congestion_info(struct ib_cc_mad *ccp,
				struct ib_device *ibdev, u8 port)
{
	struct ib_cc_info_attr *p =
		(struct ib_cc_info_attr *)ccp->mgmt_data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);

	memset(ccp->mgmt_data, 0, sizeof(ccp->mgmt_data));

	p->congestion_info = 0;
	p->control_table_cap = ppd->cc_max_table_entries;

	return reply((struct ib_smp *) ccp);
}

static int cc_get_congestion_setting(struct ib_cc_mad *ccp,
				struct ib_device *ibdev, u8 port)
{
	int i;
	struct ib_cc_congestion_setting_attr *p =
		(struct ib_cc_congestion_setting_attr *)ccp->mgmt_data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	struct ib_cc_congestion_entry_shadow *entries;

	memset(ccp->mgmt_data, 0, sizeof(ccp->mgmt_data));

	spin_lock(&ppd->cc_shadow_lock);

	entries = ppd->congestion_entries_shadow->entries;
	p->port_control = cpu_to_be16(
		ppd->congestion_entries_shadow->port_control);
	p->control_map = cpu_to_be16(
		ppd->congestion_entries_shadow->control_map & 0xffff);
	for (i = 0; i < IB_CC_CCS_ENTRIES; i++) {
		p->entries[i].ccti_increase = entries[i].ccti_increase;
		p->entries[i].ccti_timer = cpu_to_be16(entries[i].ccti_timer);
		p->entries[i].trigger_threshold = entries[i].trigger_threshold;
		p->entries[i].ccti_min = entries[i].ccti_min;
	}

	spin_unlock(&ppd->cc_shadow_lock);

	return reply((struct ib_smp *) ccp);
}

static int cc_get_congestion_control_table(struct ib_cc_mad *ccp,
				struct ib_device *ibdev, u8 port)
{
	struct ib_cc_table_attr *p =
		(struct ib_cc_table_attr *)ccp->mgmt_data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	u32 cct_block_index = be32_to_cpu(ccp->attr_mod);
	u32 max_cct_block;
	u32 cct_entry;
	struct ib_cc_table_entry_shadow *entries;
	int i;

	/* Is the table index more than what is supported? */
	if (cct_block_index > IB_CC_TABLE_CAP_DEFAULT - 1)
		goto bail;

	memset(ccp->mgmt_data, 0, sizeof(ccp->mgmt_data));

	spin_lock(&ppd->cc_shadow_lock);

	max_cct_block =
		(ppd->ccti_entries_shadow->ccti_last_entry + 1)/IB_CCT_ENTRIES;
	max_cct_block = max_cct_block ? max_cct_block - 1 : 0;

	if (cct_block_index > max_cct_block) {
		spin_unlock(&ppd->cc_shadow_lock);
		goto bail;
	}

	ccp->attr_mod = cpu_to_be32(cct_block_index);

	cct_entry = IB_CCT_ENTRIES * (cct_block_index + 1);

	cct_entry--;

	p->ccti_limit = cpu_to_be16(cct_entry);

	entries = &ppd->ccti_entries_shadow->
			entries[IB_CCT_ENTRIES * cct_block_index];
	cct_entry %= IB_CCT_ENTRIES;

	for (i = 0; i <= cct_entry; i++)
		p->ccti_entries[i].entry = cpu_to_be16(entries[i].entry);

	spin_unlock(&ppd->cc_shadow_lock);

	return reply((struct ib_smp *) ccp);

bail:
	return reply_failure((struct ib_smp *) ccp);
}

static int cc_set_congestion_setting(struct ib_cc_mad *ccp,
				struct ib_device *ibdev, u8 port)
{
	struct ib_cc_congestion_setting_attr *p =
		(struct ib_cc_congestion_setting_attr *)ccp->mgmt_data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	int i;

	ppd->cc_sl_control_map = be16_to_cpu(p->control_map) & 0xffff;

	for (i = 0; i < IB_CC_CCS_ENTRIES; i++) {
		ppd->congestion_entries[i].ccti_increase =
			p->entries[i].ccti_increase;

		ppd->congestion_entries[i].ccti_timer =
			be16_to_cpu(p->entries[i].ccti_timer);

		ppd->congestion_entries[i].trigger_threshold =
			p->entries[i].trigger_threshold;

		ppd->congestion_entries[i].ccti_min =
			p->entries[i].ccti_min;
	}

	return reply((struct ib_smp *) ccp);
}

static int cc_set_congestion_control_table(struct ib_cc_mad *ccp,
				struct ib_device *ibdev, u8 port)
{
	struct ib_cc_table_attr *p =
		(struct ib_cc_table_attr *)ccp->mgmt_data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	u32 cct_block_index = be32_to_cpu(ccp->attr_mod);
	u32 cct_entry;
	struct ib_cc_table_entry_shadow *entries;
	int i;

	/* Is the table index more than what is supported? */
	if (cct_block_index > IB_CC_TABLE_CAP_DEFAULT - 1)
		goto bail;

	/* If this packet is the first in the sequence then
	 * zero the total table entry count.
	 */
	if (be16_to_cpu(p->ccti_limit) < IB_CCT_ENTRIES)
		ppd->total_cct_entry = 0;

	cct_entry = (be16_to_cpu(p->ccti_limit))%IB_CCT_ENTRIES;

	/* ccti_limit is 0 to 63 */
	ppd->total_cct_entry += (cct_entry + 1);

	if (ppd->total_cct_entry > ppd->cc_supported_table_entries)
		goto bail;

	ppd->ccti_limit = be16_to_cpu(p->ccti_limit);

	entries = ppd->ccti_entries + (IB_CCT_ENTRIES * cct_block_index);

	for (i = 0; i <= cct_entry; i++)
		entries[i].entry = be16_to_cpu(p->ccti_entries[i].entry);

	spin_lock(&ppd->cc_shadow_lock);

	ppd->ccti_entries_shadow->ccti_last_entry = ppd->total_cct_entry - 1;
	memcpy(ppd->ccti_entries_shadow->entries, ppd->ccti_entries,
		(ppd->total_cct_entry * sizeof(struct ib_cc_table_entry)));

	ppd->congestion_entries_shadow->port_control = IB_CC_CCS_PC_SL_BASED;
	ppd->congestion_entries_shadow->control_map =
		ppd->cc_sl_control_map & 0xffff;
	memcpy(ppd->congestion_entries_shadow->entries, ppd->congestion_entries,
		IB_CC_CCS_ENTRIES * sizeof(struct ib_cc_congestion_entry));

	spin_unlock(&ppd->cc_shadow_lock);

	return reply((struct ib_smp *) ccp);

bail:
	return reply_failure((struct ib_smp *) ccp);
}

static int check_cc_key(struct qib_ibport *ibp,
			struct ib_cc_mad *ccp, int mad_flags)
{
	return 0;
}

static int process_cc(struct ib_device *ibdev, int mad_flags,
			u8 port, struct ib_mad *in_mad,
			struct ib_mad *out_mad)
{
	struct ib_cc_mad *ccp = (struct ib_cc_mad *)out_mad;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	int ret;

	*out_mad = *in_mad;

	if (ccp->class_version != 2) {
		ccp->status |= IB_SMP_UNSUP_VERSION;
		ret = reply((struct ib_smp *)ccp);
		goto bail;
	}

	ret = check_cc_key(ibp, ccp, mad_flags);
	if (ret)
		goto bail;

	switch (ccp->method) {
	case IB_MGMT_METHOD_GET:
		switch (ccp->attr_id) {
		case IB_CC_ATTR_CLASSPORTINFO:
			ret = cc_get_classportinfo(ccp, ibdev);
			goto bail;

		case IB_CC_ATTR_CONGESTION_INFO:
			ret = cc_get_congestion_info(ccp, ibdev, port);
			goto bail;

		case IB_CC_ATTR_CA_CONGESTION_SETTING:
			ret = cc_get_congestion_setting(ccp, ibdev, port);
			goto bail;

		case IB_CC_ATTR_CONGESTION_CONTROL_TABLE:
			ret = cc_get_congestion_control_table(ccp, ibdev, port);
			goto bail;

			/* FALLTHROUGH */
		default:
			ccp->status |= IB_SMP_UNSUP_METH_ATTR;
			ret = reply((struct ib_smp *) ccp);
			goto bail;
		}

	case IB_MGMT_METHOD_SET:
		switch (ccp->attr_id) {
		case IB_CC_ATTR_CA_CONGESTION_SETTING:
			ret = cc_set_congestion_setting(ccp, ibdev, port);
			goto bail;

		case IB_CC_ATTR_CONGESTION_CONTROL_TABLE:
			ret = cc_set_congestion_control_table(ccp, ibdev, port);
			goto bail;

			/* FALLTHROUGH */
		default:
			ccp->status |= IB_SMP_UNSUP_METH_ATTR;
			ret = reply((struct ib_smp *) ccp);
			goto bail;
		}

	case IB_MGMT_METHOD_GET_RESP:
		/*
		 * The ib_mad module will call us to process responses
		 * before checking for other consumers.
		 * Just tell the caller to process it normally.
		 */
		ret = IB_MAD_RESULT_SUCCESS;
		goto bail;

	case IB_MGMT_METHOD_TRAP:
	default:
		ccp->status |= IB_SMP_UNSUP_METHOD;
		ret = reply((struct ib_smp *) ccp);
	}

bail:
	return ret;
}
#endif /* !CONFIG_STL_MGMT */

#ifdef CONFIG_STL_MGMT
static int hfi_process_stl_mad(struct ib_device *ibdev, int mad_flags,
			       u8 port, struct ib_wc *in_wc,
			       struct ib_grh *in_grh,
			       struct jumbo_mad *in_mad,
			       struct jumbo_mad *out_mad)
{
	int ret;
	u32 resp_len = 0;
	/* XXX struct qib_ibport *ibp = to_iport(ibdev, port); */ 
	/* XXX struct qib_pportdata *ppd = ppd_from_ibp(ibp); */

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

#if 0
	case IB_MGMT_CLASS_CONG_MGMT:
		if (!ppd->congestion_entries_shadow ||
			 !qib_cc_table_size) {
			ret = IB_MAD_RESULT_SUCCESS;
			goto bail;
		}
		ret = process_cc(ibdev, mad_flags, port, in_mad, out_mad);
		goto bail;

#endif /* 0 */
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
#endif /* !CONFIG_STL_MGMT */

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
#ifndef CONFIG_STL_MGMT
	case IB_MGMT_CLASS_PERF_MGMT:
		ret = process_perf(ibdev, port, in_mad, out_mad);
		goto bail;

	case IB_MGMT_CLASS_CONG_MGMT:
	{
		struct qib_ibport *ibp = to_iport(ibdev, port);
		struct qib_pportdata *ppd = ppd_from_ibp(ibp);
		if (!ppd->congestion_entries_shadow ||
			 !qib_cc_table_size) {
			ret = IB_MAD_RESULT_SUCCESS;
			goto bail;
		}
		ret = process_cc(ibdev, mad_flags, port, in_mad, out_mad);
		goto bail;
	}
#endif /* !CONFIG_STL_MGMT */
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
#ifdef CONFIG_STL_MGMT
	case JUMBO_MGMT_BASE_VERSION:
		return hfi_process_stl_mad(ibdev, mad_flags, port,
					    in_wc, in_grh,
					    (struct jumbo_mad *)in_mad,
					    (struct jumbo_mad *)out_mad);
#endif /* CONFIG_STL_MGMT */
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
#ifdef CONFIG_STL_MGMT
					      NULL, NULL, 0);
#else
					      NULL, NULL);
#endif
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
