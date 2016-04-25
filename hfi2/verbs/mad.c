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
 */

/*
 * Intel(R) OPA Gen2 IB Driver
 */
#include <linux/pci.h>
#include "verbs.h"
#include "mad.h"
#include "packet.h"
#include "../link.h"
#include "../opa_hfi.h"
#include <rdma/fxr/fxr_linkmux_fpc_defs.h>
#include <rdma/fxr/fxr_linkmux_tp_defs.h>
#include <rdma/fxr/fxr_fc_defs.h>

static inline void hfi_invalid_attr(struct opa_smp *smp)
{
	smp->status |=
		cpu_to_be16(IB_MGMT_MAD_STATUS_INVALID_ATTRIB_VALUE);
}

static int reply(struct ib_mad_hdr *ibh)
{
	/*
	 * The verbs framework will handle the directed/LID route
	 * packet changes.
	 */
	ibh->method = IB_MGMT_METHOD_GET_RESP;
	if (ibh->mgmt_class == IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE)
		ibh->status |= IB_SMP_DIRECTION;
	return IB_MAD_RESULT_SUCCESS | IB_MAD_RESULT_REPLY;
}

static inline void clear_opa_smp_data(struct opa_smp *smp)
{
	void *data = opa_get_smp_data(smp);
	size_t size = opa_get_smp_data_size(smp);

	memset(data, 0, size);
}

static void hfi2_update_sm_ah_attr(struct hfi2_ibport *ibp, u32 lid)
{
	ibp->rvp.sm_ah->attr.ah_flags = IB_AH_GRH;
	ibp->rvp.sm_ah->attr.dlid = OPA_TO_IB_UCAST_LID(lid);
	ibp->rvp.sm_ah->attr.grh.sgid_index = OPA_GID_INDEX;
	ibp->rvp.sm_ah->attr.grh.dgid.global.subnet_prefix = ibp->gid_prefix;
	ibp->rvp.sm_ah->attr.grh.dgid.global.interface_id = OPA_MAKE_GID(lid);
}

static void send_trap(struct hfi2_ibport *ibp, void *data, unsigned len)
{
	struct ib_mad_send_buf *send_buf;
	struct ib_mad_agent *agent;
	struct opa_smp *smp;
	int ret = 0;
	unsigned long flags;
	unsigned long timeout;
	int pkey_idx;
	struct hfi_pportdata *ppd = ibp->ppd;
	u32 qpn = ppd->sm_trap_qp;

	agent = ibp->rvp.send_agent;
	if (!agent)
		return;

	/* o14-3.2.1 */
	if (ppd->lstate != IB_PORT_ACTIVE)
		return;

	/* o14-2 */
	if (ibp->trap_timeout && time_before(jiffies, ibp->trap_timeout))
		return;

	pkey_idx = hfi2_lookup_pkey_idx(ibp, OPA_LIM_MGMT_PKEY);
	if (pkey_idx < 0) {
		pr_warn("%s: failed to find limited mgmt pkey, defaulting 0x%x\n",
			__func__, hfi2_get_pkey(ibp, 1));
		pkey_idx = 1;
	}

	send_buf = ib_create_send_mad(agent, qpn, pkey_idx, 0,
				      IB_MGMT_MAD_HDR, IB_MGMT_MAD_DATA,
				      GFP_ATOMIC, OPA_MGMT_BASE_VERSION);
	if (IS_ERR(send_buf))
		return;

	smp = send_buf->mad;
	smp->base_version = OPA_MGMT_BASE_VERSION;
	smp->mgmt_class = IB_MGMT_CLASS_SUBN_LID_ROUTED;
	smp->class_version = OPA_SMI_CLASS_VERSION;
	smp->method = IB_MGMT_METHOD_TRAP;
	ibp->tid++;
	smp->tid = cpu_to_be64(ibp->tid);
	smp->attr_id = IB_SMP_ATTR_NOTICE;
	/* o14-1: smp->mkey = 0; */
	memcpy(smp->route.lid.data, data, len);

	spin_lock_irqsave(&ibp->lock, flags);
	if (!ibp->rvp.sm_ah) {
		if (ibp->sm_lid != be16_to_cpu(IB_LID_PERMISSIVE)) {
			struct ib_ah *ah;

			ah = hfi2_create_qp0_ah(ibp, ibp->sm_lid);
			if (IS_ERR(ah)) {
				ret = PTR_ERR(ah);
			} else {
				send_buf->ah = ah;
				ibp->rvp.sm_ah = ibah_to_rvtah(ah);
				hfi2_update_sm_ah_attr(ibp, ibp->sm_lid);
			}
		} else {
			ret = -EINVAL;
		}
	} else {
		send_buf->ah = &ibp->rvp.sm_ah->ibah;
	}
	spin_unlock_irqrestore(&ibp->lock, flags);

	if (!ret)
		ret = ib_post_send_mad(send_buf, NULL);
	if (!ret) {
		timeout = (4096 * (1UL << ibp->subnet_timeout)) / 1000;
		ibp->trap_timeout = jiffies + usecs_to_jiffies(timeout);
	} else {
		ib_free_send_mad(send_buf);
		ibp->trap_timeout = 0;
	}
}

/*
 * Send a bad [PQ]_Key trap (ch. 14.3.8).
 * FXRTODO:
 * STL-6007: hfi2_bad_pqkey calls needs to implemented
 */
void hfi2_bad_pqkey(struct hfi2_ibport *ibp, __be16 trap_num, u32 key, u32 sl,
		    u32 qp1, u32 qp2, u32 lid1, u32 lid2)
{
	struct opa_mad_notice_attr data;
	u32 lid = ibp->ppd->lid;

	memset(&data, 0, sizeof(data));

	if (trap_num == OPA_TRAP_BAD_P_KEY)
		ibp->pkey_violations++;
	else
		ibp->qkey_violations++;
	ibp->n_pkt_drops++;

	/* Send violation trap */
	data.generic_type = IB_NOTICE_TYPE_SECURITY;
	data.prod_type_lsb = IB_NOTICE_PROD_CA;
	data.trap_num = trap_num;
	data.issuer_lid = cpu_to_be32(lid);
	/* FXRTODO: These castings are not correct; fix later */
	data.ntc_257_258.lid1 = (u16)lid1;
	data.ntc_257_258.lid2 = (u16)lid2;
	data.ntc_257_258.key = cpu_to_be32(key);
	data.ntc_257_258.sl = sl << 3;
	data.ntc_257_258.qp1 = cpu_to_be32(qp1);
	data.ntc_257_258.qp2 = cpu_to_be32(qp2);

	send_trap(ibp, &data, sizeof(data));
}

/*
 * Send a bad M_Key trap (ch. 14.3.9).
 */
static void bad_mkey(struct hfi2_ibport *ibp, struct ib_mad_hdr *mad,
		     __be64 mkey, __be32 dr_slid, u8 return_path[], u8 hop_cnt)
{
	struct opa_mad_notice_attr data;
	u32 lid = ibp->ppd->lid;

	memset(&data, 0, sizeof(data));
	/* Send violation trap */
	data.generic_type = IB_NOTICE_TYPE_SECURITY;
	data.prod_type_lsb = IB_NOTICE_PROD_CA;
	data.trap_num = OPA_TRAP_BAD_M_KEY;
	data.issuer_lid = cpu_to_be32(lid);
	data.ntc_256.lid = data.issuer_lid;
	data.ntc_256.method = mad->method;
	data.ntc_256.attr_id = mad->attr_id;
	data.ntc_256.attr_mod = mad->attr_mod;
	data.ntc_256.mkey = mkey;
	if (mad->mgmt_class == IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE) {
		data.ntc_256.dr_slid = dr_slid;
		data.ntc_256.dr_trunc_hop = IB_NOTICE_TRAP_DR_NOTICE;
		if (hop_cnt > ARRAY_SIZE(data.ntc_256.dr_rtn_path)) {
			data.ntc_256.dr_trunc_hop |=
				IB_NOTICE_TRAP_DR_TRUNC;
			hop_cnt = ARRAY_SIZE(data.ntc_256.dr_rtn_path);
		}
		data.ntc_256.dr_trunc_hop |= hop_cnt;
		memcpy(data.ntc_256.dr_rtn_path, return_path,
		       hop_cnt);
	}
	send_trap(ibp, &data, sizeof(data));
}

void hfi2_cap_mask_chg(struct hfi2_ibport *ibp)
{
	struct opa_mad_notice_attr data;
	u32 lid = ibp->ppd->lid;

	memset(&data, 0, sizeof(data));

	data.generic_type = IB_NOTICE_TYPE_INFO;
	data.prod_type_lsb = IB_NOTICE_PROD_CA;
	data.trap_num = OPA_TRAP_CHANGE_CAPABILITY;
	data.issuer_lid = cpu_to_be32(lid);
	data.ntc_144.lid = data.issuer_lid;
	data.ntc_144.new_cap_mask = cpu_to_be32(ibp->port_cap_flags);

	send_trap(ibp, &data, sizeof(data));
}

/*
 * Send a System Image GUID Changed trap (ch. 14.3.12).
 */
void hfi2_sys_guid_chg(struct hfi2_ibport *ibp)
{
	struct opa_mad_notice_attr data;
	u32 lid = ibp->ppd->lid;

	memset(&data, 0, sizeof(data));

	data.generic_type = IB_NOTICE_TYPE_INFO;
	data.prod_type_lsb = IB_NOTICE_PROD_CA;
	data.trap_num = OPA_TRAP_CHANGE_SYSGUID;
	data.issuer_lid = cpu_to_be32(lid);
	data.ntc_145.new_sys_guid = hfi2_sys_guid;
	data.ntc_145.lid = data.issuer_lid;

	send_trap(ibp, &data, sizeof(data));
}

/*
 * Send a Node Description Changed trap (ch. 14.3.13).
 */
void hfi2_node_desc_chg(struct hfi2_ibport *ibp)
{
	struct opa_mad_notice_attr data;
	u32 lid = ibp->ppd->lid;

	memset(&data, 0, sizeof(data));

	data.generic_type = IB_NOTICE_TYPE_INFO;
	data.prod_type_lsb = IB_NOTICE_PROD_CA;
	data.trap_num = OPA_TRAP_CHANGE_CAPABILITY;
	data.issuer_lid = cpu_to_be32(lid);
	data.ntc_144.lid = data.issuer_lid;
	data.ntc_144.change_flags =
		cpu_to_be16(OPA_NOTICE_TRAP_NODE_DESC_CHG);

	send_trap(ibp, &data, sizeof(data));
}

static int check_mkey(struct hfi2_ibport *ibp, struct ib_mad_hdr *mad,
		      int mad_flags, __be64 mkey, __be32 dr_slid,
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
			bad_mkey(ibp, mad, mkey, dr_slid, return_path,
				 hop_cnt);
			ret = 1;
		}
	}

	return ret;
}

/* This attribute is implemented only here (sma-ib) */
static int __subn_get_ib_nodeinfo(struct ib_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	struct ib_node_info *ni;
	struct hfi2_ibdev *ibd = to_hfi_ibd(ibdev);
	struct hfi_devdata *dd = ibd->dd;
	struct hfi2_ibport *ibp = to_hfi_ibp(ibdev, port);
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;

	ni = (struct ib_node_info *)smp->data;

	/* GUID 0 is illegal */
	if (smp->attr_mod || ibd->node_guid == 0
				|| !ibp) {
		smp->status |=
			cpu_to_be16(IB_MGMT_MAD_STATUS_INVALID_ATTRIB_VALUE);
		return reply(ibh);
	}
	ni->base_version = OPA_MGMT_BASE_VERSION;
	ni->class_version = OPA_SMI_CLASS_VERSION;
	ni->node_type = 1;     /* channel adapter */
	ni->num_ports = ibd->num_pports;
	/* This is already in network order */
	ni->sys_guid = hfi2_sys_guid;
	ni->node_guid = ibd->node_guid;
	ni->port_guid = ibp->ppd->pguid;
	ni->partition_cap = cpu_to_be16(HFI_MAX_PKEYS);
	ni->local_port_num = port;
	memcpy(ni->vendor_id, ibd->oui, ARRAY_SIZE(ni->vendor_id));
	ni->device_id = cpu_to_be16(dd->bus_id.device);
	ni->revision = cpu_to_be32(dd->bus_id.revision);

	return reply(ibh);
}

static int process_subn(struct ib_device *ibdev, int mad_flags,
			u8 port, const struct ib_mad *in_mad,
			struct ib_mad *out_mad)
{
	struct ib_smp *smp = (struct ib_smp *)out_mad;
	struct hfi2_ibport *ibp = to_hfi_ibp(ibdev, port);
	int ret = 0;

	*out_mad = *in_mad;
	if (!ibp) {
		smp->status |=
			cpu_to_be16(IB_MGMT_MAD_STATUS_INVALID_ATTRIB_VALUE);
		ret = reply((struct ib_mad_hdr *)smp);
		goto bail;
	}

	if (smp->class_version != 1) {
		smp->status |= cpu_to_be16(IB_MGMT_MAD_STATUS_BAD_VERSION);
		ret = reply((struct ib_mad_hdr *)smp);
		goto bail;
	}

	ret = check_mkey(ibp, (struct ib_mad_hdr *)smp, mad_flags,
			 smp->mkey, smp->dr_slid, smp->return_path,
			 smp->hop_cnt);
	if (ret) {
		ret = IB_MAD_RESULT_FAILURE;
		goto bail;
	}

	switch (smp->method) {
	case IB_MGMT_METHOD_GET:
		switch (smp->attr_id) {
		case IB_SMP_ATTR_NODE_INFO:
			ret = __subn_get_ib_nodeinfo(smp, ibdev, port);
			goto bail;
		default:
			smp->status |= cpu_to_be16(
			IB_MGMT_MAD_STATUS_UNSUPPORTED_METHOD_ATTRIB);
			ret = reply((struct ib_mad_hdr *)smp);
			goto bail;
		}
	}

bail:
	return ret;
}

static int __subn_get_opa_nodedesc(struct opa_smp *smp, u32 am, u8 *data,
					struct ib_device *ibdev, u8 port,
							u32 *resp_len)
{
	struct opa_node_description *nd;

	if (am) {
		smp->status |=
			cpu_to_be16(IB_MGMT_MAD_STATUS_INVALID_ATTRIB_VALUE);
		return reply((struct ib_mad_hdr *)smp);
	}

	nd = (struct opa_node_description *)data;

	memcpy(nd->data, ibdev->node_desc, sizeof(nd->data));

	if (resp_len)
		*resp_len += sizeof(*nd);

	return reply((struct ib_mad_hdr *)smp);
}

 /* This attribute is only implemented here (sma-ib)*/
static int __subn_get_opa_nodeinfo(struct opa_smp *smp, u32 am, u8 *data,
				   struct ib_device *ibdev, u8 port,
				   u32 *resp_len)
{
	struct opa_node_info *ni;
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	struct hfi2_ibdev *ibd = to_hfi_ibd(ibdev);
	struct hfi_devdata *dd = ibd->dd;
	struct hfi2_ibport *ibp = to_hfi_ibp(ibdev, port);

	ni = (struct opa_node_info *)data;

	/* GUID 0 is illegal */
	if (am || ibd->node_guid == 0 || !ibp) {
		smp->status |=
			cpu_to_be16(IB_MGMT_MAD_STATUS_INVALID_ATTRIB_VALUE);
		return reply(ibh);
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 3, 0)
	ni->base_version = OPA_MGMT_BASE_VERSION;
#else
	ni->base_version = JUMBO_MGMT_BASE_VERSION;
#endif
	ni->class_version = OPA_SMI_CLASS_VERSION;
	ni->node_type = 1;     /* channel adapter */
	ni->num_ports = ibd->num_pports;
	/* This is already in network order */
	ni->system_image_guid = hfi2_sys_guid;
	ni->node_guid = ibd->node_guid;
	ni->port_guid = ibp->ppd->pguid;
	ni->partition_cap = cpu_to_be16(HFI_MAX_PKEYS);
	ni->local_port_num = port;
	memcpy(ni->vendor_id, ibd->oui, ARRAY_SIZE(ni->vendor_id));
	ni->device_id = cpu_to_be16(dd->bus_id.device);
	ni->revision = cpu_to_be16(dd->bus_id.revision);

	if (resp_len)
		*resp_len += sizeof(*ni);

	return reply(ibh);
}

static int __subn_get_opa_portinfo(struct opa_smp *smp, u32 am, u8 *data,
					struct ib_device *ibdev, u8 port,
					u32 *resp_len)
{
	int i;
	struct hfi_devdata *dd;
	struct hfi_pportdata *ppd;
	struct hfi2_ibport *ibp;
	struct opa_port_info *pi = (struct opa_port_info *)data;
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	u8 mtu;
	u8 credit_rate;
	u32 state;
	u32 num_ports = OPA_AM_NPORT(am);
	u32 start_of_sm_config = OPA_AM_START_SM_CFG(am);
	u32 buffer_units;

	if (num_ports != 1) {
		hfi_invalid_attr(smp);
		return reply(ibh);
	}

	dd = hfi_dd_from_ibdev(ibdev);
	/* IB numbers ports from 1, hw from 0 */
	ppd = to_hfi_ppd(dd, port);
	ibp = to_hfi_ibp(ibdev, port);

	if (ppd->vls_supported / 2 > ARRAY_SIZE(pi->neigh_mtu.pvlx_to_mtu) ||
	    ppd->vls_supported > ARRAY_SIZE(ppd->vl_mtu)) {
		hfi_invalid_attr(smp);
		return reply(ibh);
	}

	pi->lid = cpu_to_be32(ppd->lid);
	pi->max_lid = cpu_to_be32(ppd->max_lid);

	/* Only return the mkey if the protection field allows it. */
	if (!(smp->method == IB_MGMT_METHOD_GET &&
	      ibp->mkey != smp->mkey &&
	      ibp->mkeyprot == 1))
		pi->mkey = ibp->mkey;

	pi->subnet_prefix = ibp->gid_prefix;
	pi->sm_lid = cpu_to_be32(ibp->sm_lid);
	pi->ib_cap_mask = cpu_to_be32(ibp->port_cap_flags);
	pi->mkey_lease_period = cpu_to_be16(ibp->mkey_lease_period);
	pi->sm_trap_qp = cpu_to_be32(ppd->sm_trap_qp);
	pi->sa_qp = cpu_to_be32(ppd->sa_qp);

	pi->link_width.enabled = cpu_to_be16(ppd->link_width_enabled);
	pi->link_width.supported = cpu_to_be16(ppd->link_width_supported);
	pi->link_width.active = cpu_to_be16(ppd->link_width_active);

	pi->link_width_downgrade.supported =
			cpu_to_be16(ppd->link_width_downgrade_supported);
	pi->link_width_downgrade.enabled =
			cpu_to_be16(ppd->link_width_downgrade_enabled);
	pi->link_width_downgrade.tx_active =
			cpu_to_be16(ppd->link_width_downgrade_tx_active);
	pi->link_width_downgrade.rx_active =
			cpu_to_be16(ppd->link_width_downgrade_rx_active);

	pi->link_speed.supported = cpu_to_be16(ppd->link_speed_supported);
	pi->link_speed.active = cpu_to_be16(ppd->link_speed_active);
	pi->link_speed.enabled = cpu_to_be16(ppd->link_speed_enabled);

	state = hfi_driver_lstate(ppd);

	if (start_of_sm_config && (state == IB_PORT_INIT))
		ppd->is_sm_config_started = 1;

	pi->port_phys_conf = (ppd->port_type & 0xf);

#if PI_LED_ENABLE_SUP
	pi->port_states.ledenable_offlinereason = ppd->neighbor_normal << 4;
	pi->port_states.ledenable_offlinereason |=
		ppd->is_sm_config_started << 5;
	pi->port_states.ledenable_offlinereason |=
		ppd->offline_disabled_reason;
#else
	pi->port_states.offline_reason = ppd->neighbor_normal << 4;
	pi->port_states.offline_reason |= ppd->is_sm_config_started << 5;
	pi->port_states.offline_reason |= ppd->offline_disabled_reason;
#endif /* PI_LED_ENABLE_SUP */

	pi->port_states.portphysstate_portstate =
		(hfi_ibphys_portstate(ppd) << 4) | state;

	pi->mkeyprotect_lmc = (ibp->mkeyprot << 6) | ppd->lmc;

	memset(pi->neigh_mtu.pvlx_to_mtu, 0, sizeof(pi->neigh_mtu.pvlx_to_mtu));
	for (i = 0; i < ppd->vls_supported; i++) {
		mtu = opa_mtu_to_enum_safe(ppd->vl_mtu[i], OPA_MTU_10240);
		if ((i % 2) == 0)
			pi->neigh_mtu.pvlx_to_mtu[i / 2] |= (mtu << 4);
		else
			pi->neigh_mtu.pvlx_to_mtu[i / 2] |= mtu;
	}
	/* don't forget VL 15 */
	mtu = opa_mtu_to_enum(ppd->vl_mtu[15]);
	pi->neigh_mtu.pvlx_to_mtu[15 / 2] |= mtu;
	pi->smsl = ibp->sm_sl & OPA_PI_MASK_SMSL;
	pi->operational_vls = hfi_get_ib_cfg(ppd, HFI_IB_CFG_OP_VLS, 0, NULL);
	pi->partenforce_filterraw |=
		(ppd->linkinit_reason & OPA_PI_MASK_LINKINIT_REASON);
	if (ppd->part_enforce & HFI_PART_ENFORCE_IN)
		pi->partenforce_filterraw |= OPA_PI_MASK_PARTITION_ENFORCE_IN;
	if (ppd->part_enforce & HFI_PART_ENFORCE_OUT)
		pi->partenforce_filterraw |= OPA_PI_MASK_PARTITION_ENFORCE_OUT;
	pi->mkey_violations = cpu_to_be16(ibp->mkey_violations);
	/* P_KeyViolations are counted by hardware. */
	pi->pkey_violations = cpu_to_be16(ibp->pkey_violations);
	pi->qkey_violations = cpu_to_be16(ibp->qkey_violations);

	pi->vl.cap = ppd->vls_supported;
	pi->vl.high_limit = cpu_to_be16(ibp->vl_high_limit);
	/*
	 * BW arb update not yet done to struct opa_port_info.
	 * So send BwGroupCap using arb_high_cap.
	 */
	pi->vl.arb_high_cap = OPA_NUM_BW_GROUP_SUPPORTED;

	pi->clientrereg_subnettimeout = ibp->subnet_timeout;

	pi->port_link_mode  = cpu_to_be16(OPA_PORT_LINK_MODE_OPA << 10 |
					  OPA_PORT_LINK_MODE_OPA << 5 |
					  OPA_PORT_LINK_MODE_OPA);

	pi->port_ltp_crc_mode = cpu_to_be16(ppd->port_ltp_crc_mode);

	pi->port_mode = cpu_to_be16(
				ppd->is_active_optimize_enabled ?
					OPA_PI_MASK_PORT_ACTIVE_OPTOMIZE : 0);
	pi->port_mode |= cpu_to_be16(
				ppd->is_vl_marker_enabled ?
					OPA_PI_MASK_PORT_MODE_VL_MARKER : 0);

	pi->port_packet_format.supported =
		cpu_to_be16(ppd->packet_format_supported);
	pi->port_packet_format.enabled =
		cpu_to_be16(ppd->packet_format_enabled);

	/*
	 * flit_control.interleave is (OPA V1, version .76):
	 * bits		use
	 * ----		---
	 * 2		res
	 * 2		DistanceSupported
	 * 2		DistanceEnabled
	 * 5		MaxNextLevelTxEnabled
	 * 5		MaxNestLevelRxSupported
	 *
	 * HFI supports only "distance mode 1" (see OPA V1, version .76,
	 * section 9.6.2), so set DistanceSupported, DistanceEnabled
	 * to 0x1.
	 */
	pi->flit_control.interleave = cpu_to_be16(0x1400);

	pi->link_down_reason = ppd->local_link_down_reason.sma;
	pi->neigh_link_down_reason = ppd->neigh_link_down_reason.sma;
	pi->port_error_action = cpu_to_be32(ppd->port_error_action);
	pi->mtucap = opa_mtu_to_enum(HFI_DEFAULT_MAX_MTU);

	/* 32.768 usec. response time (guessing) */
	pi->resptimevalue = 3;

	pi->local_port_num = port;

	/* buffer info for FM */
	pi->overall_buffer_space = cpu_to_be16(ppd->link_credits);

	pi->neigh_node_guid = cpu_to_be64(ppd->neighbor_guid);
	pi->neigh_port_num = ppd->neighbor_port_number;
	pi->port_neigh_mode =
		(ppd->neighbor_type & OPA_PI_MASK_NEIGH_NODE_TYPE) |
		(ppd->mgmt_allowed ? OPA_PI_MASK_NEIGH_MGMT_ALLOWED : 0) |
		(ppd->neighbor_fm_security ?
			OPA_PI_MASK_NEIGH_FW_AUTH_BYPASS : 0);

	/*
	 * HFIs shall always return VL15 credits to their
	 * neighbor in a timely manner, without any credit return pacing.
	 */
	credit_rate = 0;
	buffer_units  = (ppd->vau) & OPA_PI_MASK_BUF_UNIT_BUF_ALLOC;
	buffer_units |= (ppd->vcu << 3) & OPA_PI_MASK_BUF_UNIT_CREDIT_ACK;
	buffer_units |= (credit_rate << 6) &
				OPA_PI_MASK_BUF_UNIT_VL15_CREDIT_RATE;
	buffer_units |= (ppd->vl15_init << 11) & OPA_PI_MASK_BUF_UNIT_VL15_INIT;
	pi->buffer_units = cpu_to_be32(buffer_units);

	pi->opa_cap_mask = cpu_to_be16(OPA_CAP_MASK3_IsSharedSpaceSupported);
	pi->opa_cap_mask |= cpu_to_be16(OPA_CAP_MASK3_IsVLrSupported);
	pi->opa_cap_mask |= cpu_to_be16(OPA_CAP_MASK3_IsMAXLIDSupported);
	pi->opa_cap_mask |= cpu_to_be16(OPA_CAP_MASK3_IsBwMeterSupported);
	pi->opa_cap_mask |= cpu_to_be16(OPA_CAP_MASK3_IsVLMarkerSupported);

	/* Driver does not support mcast/collective configuration */
	pi->opa_cap_mask &=
		cpu_to_be16(~OPA_CAP_MASK3_IsAddrRangeConfigSupported);
	pi->collectivemask_multicastmask = ((HFI1_COLLECTIVE_NR & 0x7)
					    << 3 | (HFI1_MCAST_NR & 0x7));
#if 0

	/* FXRTODO: replay depth buffer for FXR */
	/* HFI supports a replay buffer 128 LTPs in size */
	pi->replay_depth.buffer = 0x80;
	/* read the cached value of DC_LCB_STS_ROUND_TRIP_LTP_CNT */
	read_lcb_cache(DC_LCB_STS_ROUND_TRIP_LTP_CNT, &tmp);

	/* this counter is 16 bits wide, but the replay_depth.wire
	 * variable is only 8 bits
	 */
	if (tmp > 0xff)
		tmp = 0xff;
	pi->replay_depth.wire = tmp;
#endif

	if (resp_len)
		*resp_len += sizeof(struct opa_port_info);

	return reply((struct ib_mad_hdr *)smp);
}

static int __subn_get_opa_psi(struct opa_smp *smp, u32 am, u8 *data,
					struct ib_device *ibdev, u8 port,
							u32 *resp_len)
{
	u32 nports = OPA_AM_NPORT(am);
	u32 start_of_sm_config = OPA_AM_START_SM_CFG(am);
	u32 lstate;
	struct hfi_devdata *dd;
	struct hfi_pportdata *ppd;
	struct opa_port_state_info *psi = (struct opa_port_state_info *)data;

	if (nports != 1) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply((struct ib_mad_hdr *)smp);
	}

	dd = hfi_dd_from_ibdev(ibdev);
	/* IB numbers ports from 1, hw from 0 */
	ppd = to_hfi_ppd(dd, port);

	lstate = hfi_driver_lstate(ppd);

	if (start_of_sm_config && (lstate == IB_PORT_INIT))
		ppd->is_sm_config_started = 1;

#if PI_LED_ENABLE_SUP
	psi->port_states.ledenable_offlinereason = ppd->neighbor_normal << 4;
	psi->port_states.ledenable_offlinereason |=
		ppd->is_sm_config_started << 5;
	psi->port_states.ledenable_offlinereason |=
		ppd->offline_disabled_reason;
#else
	psi->port_states.offline_reason = ppd->neighbor_normal << 4;
	psi->port_states.offline_reason |= ppd->is_sm_config_started << 5;
	psi->port_states.offline_reason |= ppd->offline_disabled_reason;
#endif /* PI_LED_ENABLE_SUP */

	psi->port_states.portphysstate_portstate =
		(hfi_ibphys_portstate(ppd) << 4) | (lstate & 0xf);
	psi->link_width_downgrade_tx_active =
		cpu_to_be16(ppd->link_width_downgrade_tx_active);
	psi->link_width_downgrade_rx_active =
		cpu_to_be16(ppd->link_width_downgrade_rx_active);
	if (resp_len)
		*resp_len += sizeof(struct opa_port_state_info);

	return reply((struct ib_mad_hdr *)smp);
}

static int __subn_get_opa_pkeytable(struct opa_smp *smp, u32 am, u8 *data,
					struct ib_device *ibdev, u8 port,
							u32 *resp_len)
{
	struct hfi_devdata *dd = hfi_dd_from_ibdev(ibdev);
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	size_t size;
	u32 n_blocks_req = OPA_AM_NBLK(am);
	u32 start_block = OPA_AM_START_BLK(am);
	u32 end_block;
	__be16 *pkeys = (__be16 *)data;
	int i, j, k;

	end_block = start_block + n_blocks_req;
	size = (n_blocks_req * OPA_PKEY_TABLE_BLK_COUNT) * sizeof(u16);

	if (n_blocks_req == 0 || end_block > HFI_PKEY_BLOCKS_AVAIL ||
	    n_blocks_req > OPA_NUM_PKEY_BLOCKS_PER_SMP) {
		pr_warn("OPA Get PKey AM Invalid : s 0x%x; req 0x%x; ",
			start_block, n_blocks_req);
		pr_warn(" avail 0x%x; blk/smp 0x%lx\n", HFI_PKEY_BLOCKS_AVAIL,
			OPA_NUM_PKEY_BLOCKS_PER_SMP);
		hfi_invalid_attr(smp);
		return reply(ibh);
	}

	for (i = start_block, k = 0; i < end_block; i++)
		for (j = 0; j < OPA_PKEY_TABLE_BLK_COUNT; j++, k++)
			pkeys[k] = cpu_to_be16(ppd->pkeys[i *
					OPA_PKEY_TABLE_BLK_COUNT + j]);

	if (resp_len)
		*resp_len += size;

	return reply(ibh);
}

static int __subn_get_opa_sl_to_sc(struct opa_smp *smp, u32 am, u8 *data,
					struct ib_device *ibdev, u8 port,
							u32 *resp_len)
{
	struct hfi_devdata *dd = hfi_dd_from_ibdev(ibdev);
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	size_t size = sizeof(ppd->sl_to_sc);
	unsigned i;
	u8 *p = (u8 *)data;

	if (am) {
		hfi_invalid_attr(smp);
		return reply(ibh);
	}

	for (i = 0; i < ARRAY_SIZE(ppd->sl_to_sc); i++)
		p[i] = ppd->sl_to_sc[i];

	if (resp_len)
		*resp_len += size;

	return reply(ibh);
}

static int __subn_get_opa_sc_to_sl(struct opa_smp *smp, u32 am, u8 *data,
					struct ib_device *ibdev, u8 port,
							u32 *resp_len)
{
	struct hfi_devdata *dd = hfi_dd_from_ibdev(ibdev);
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	size_t size = sizeof(ppd->sc_to_sl);
	unsigned i;
	u8 *p = (u8 *)data;

	if (am) {
		hfi_invalid_attr(smp);
		return reply(ibh);
	}

	for (i = 0; i < ARRAY_SIZE(ppd->sc_to_sl); i++)
		p[i] = ppd->sc_to_sl[i];

	if (resp_len)
		*resp_len += size;

	return reply(ibh);
}

static int __subn_get_opa_sc_to_vlr(struct opa_smp *smp, u32 am, u8 *data,
				    struct ib_device *ibdev, u8 port,
				    u32 *resp_len)
{
	struct hfi_devdata *dd = hfi_dd_from_ibdev(ibdev);
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	u32 n_blocks = OPA_AM_NBLK(am);
	u8 *p = (void *)data;
	size_t size = sizeof(ppd->sc_to_vlr);

	if (n_blocks != 1) {
		hfi_invalid_attr(smp);
		return reply(ibh);
	}

	memcpy(p, ppd->sc_to_vlr, size);

	if (resp_len)
		*resp_len += size;

	return reply(ibh);
}

static int __subn_get_opa_sc_to_vlt(struct opa_smp *smp, u32 am, u8 *data,
					struct ib_device *ibdev, u8 port,
							u32 *resp_len)
{
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	struct hfi_devdata *dd = hfi_dd_from_ibdev(ibdev);
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	u32 n_blocks = OPA_AM_NBLK(am);
	u8 *p = (void *)data;
	size_t size = sizeof(ppd->sc_to_vlt);

	if (n_blocks != 1) {
		hfi_invalid_attr(smp);
		return reply(ibh);
	}

	memcpy(p, ppd->sc_to_vlt, size);

	if (resp_len)
		*resp_len += size;

	return reply(ibh);
}

static int __subn_get_opa_sc_to_vlnt(struct opa_smp *smp, u32 am, u8 *data,
					struct ib_device *ibdev, u8 port,
							u32 *resp_len)
{
	struct hfi_devdata *dd = hfi_dd_from_ibdev(ibdev);
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	u32 n_blocks = OPA_AM_NBLK(am);
	u8 *p = (void *)data;
	size_t size = sizeof(ppd->sc_to_vlnt);

	if (n_blocks != 1) {
		hfi_invalid_attr(smp);
		return reply(ibh);
	}

	memcpy(p, ppd->sc_to_vlnt, size);

	if (resp_len)
		*resp_len += size;

	return reply(ibh);
}

static int __subn_get_opa_bw_arb(struct opa_smp *smp, u32 am, u8 *data,
				 struct ib_device *ibdev, u8 port,
				 u32 *resp_len)
{
	struct hfi_devdata *dd = hfi_dd_from_ibdev(ibdev);
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	u32 num_ports = OPA_AM_NPORT(am);
	u8 section = OPA_BW_SECTION(am);
	int size = sizeof(struct opa_bw_arb) +  sizeof(union opa_bw_arb_table);
	struct opa_bw_arb *bw_arb = (struct opa_bw_arb *)data;

	if ((num_ports != 1) || (section != OPA_BWARB_GROUP &&
				 section != OPA_BWARB_PREEMPT_MATRIX))
		goto err;

	bw_arb->port_sel_mask[3] = cpu_to_be64((uint64_t)(1) << (port - 1));
	hfi_get_ib_cfg(ppd, HFI_IB_CFG_BW_ARB, section, data);

	if (resp_len)
		*resp_len += size;

	goto done;
err:
	hfi_invalid_attr(smp);
done:
	return reply(ibh);
}

static int __subn_get_opa_led_info(struct opa_smp *smp, u32 am, u8 *data,
					struct ib_device *ibdev, u8 port,
							u32 *resp_len)
{
	/* FXRTODO: to be implemented */
	return IB_MAD_RESULT_FAILURE;
}

static int __subn_get_opa_cable_info(struct opa_smp *smp, u32 am, u8 *data,
					struct ib_device *ibdev, u8 port,
							u32 *resp_len)
{
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	u32 len = OPA_AM_CI_LEN(am);

	/* FXRTODO: Implement as part of QSFP STL-3910 */
	memset(data, 0, len);

	if (resp_len)
		*resp_len += len;

	return reply(ibh);
}

static int __subn_get_opa_bct(struct opa_smp *smp, u32 am, u8 *data,
					struct ib_device *ibdev, u8 port,
							u32 *resp_len)
{
	struct hfi_devdata *dd = hfi_dd_from_ibdev(ibdev);
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	struct buffer_control *p = (struct buffer_control *)data;
	u32 num_ports = OPA_AM_NPORT(am);
	int size = sizeof(*p);

	if (num_ports != 1) {
		hfi_invalid_attr(smp);
		return reply(ibh);
	}

	hfi_get_buffer_control(ppd, p, NULL);

	if (resp_len)
		*resp_len += size;

	return reply(ibh);
}

static int __subn_get_opa_cong_info(struct opa_smp *smp, u32 am, u8 *data,
					struct ib_device *ibdev, u8 port,
							u32 *resp_len)
{
	struct hfi_devdata *dd = hfi_dd_from_ibdev(ibdev);
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	struct opa_congestion_info_attr *p =
		(struct opa_congestion_info_attr *)data;
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);

	p->congestion_info = 0;
	p->control_table_cap = ppd->cc_max_table_entries;
	p->congestion_log_length = OPA_CONG_LOG_ELEMS;

	if (resp_len)
		*resp_len += sizeof(*p);

	return reply(ibh);
}

static int __subn_get_opa_hfi_cong_log(struct opa_smp *smp, u32 am, u8 *data,
					struct ib_device *ibdev, u8 port,
							u32 *resp_len)
{
	/* FXRTODO: to be implemented */
	return IB_MAD_RESULT_FAILURE;
}

void hfi_cc_state_reclaim(struct rcu_head *rcu)
{
	struct cc_state *cc_state = container_of(rcu, struct cc_state, rcu);

	kfree(cc_state);
}

static int __subn_get_opa_cong_setting(struct opa_smp *smp, u32 am, u8 *data,
					struct ib_device *ibdev, u8 port,
							u32 *resp_len)
{
	int i;
	struct hfi_devdata *dd = hfi_dd_from_ibdev(ibdev);
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	struct opa_congestion_setting_attr *p =
		(struct opa_congestion_setting_attr *)data;
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	struct opa_congestion_setting_entry_shadow *entries;
	struct cc_state *cc_state;

	rcu_read_lock();

	cc_state = hfi_get_cc_state(ppd);

	if (!cc_state) {
		rcu_read_unlock();
		/* FXRTODO: Should error attribute be set */
		return reply(ibh);
	}

	entries = cc_state->cong_setting.entries;
	p->port_control = cpu_to_be16(cc_state->cong_setting.port_control);
	p->control_map = cpu_to_be32(cc_state->cong_setting.control_map);
	for (i = 0; i < OPA_MAX_SLS; i++) {
		p->entries[i].ccti_increase = entries[i].ccti_increase;
		p->entries[i].ccti_timer = cpu_to_be16(entries[i].ccti_timer);
		p->entries[i].trigger_threshold =
			entries[i].trigger_threshold;
		p->entries[i].ccti_min = entries[i].ccti_min;
	}

	rcu_read_unlock();

	if (resp_len)
		*resp_len += sizeof(*p);

	return reply(ibh);
}

static int __subn_get_opa_cc_table(struct opa_smp *smp, u32 am, u8 *data,
					struct ib_device *ibdev, u8 port,
							u32 *resp_len)
{
	struct hfi_devdata *dd = hfi_dd_from_ibdev(ibdev);
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	struct ib_cc_table_attr *cc_table_attr =
		(struct ib_cc_table_attr *)data;
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	u32 start_block = OPA_AM_START_BLK(am);
	u32 n_blocks = OPA_AM_NBLK(am);
	struct ib_cc_table_entry_shadow *entries;
	int i, j;
	u32 sentry, eentry;
	struct cc_state *cc_state;

	/* sanity check n_blocks, start_block */
	if (n_blocks == 0 ||
	    start_block + n_blocks > ppd->cc_max_table_entries) {
		hfi_invalid_attr(smp);
		return reply(ibh);
	}

	rcu_read_lock();

	cc_state = hfi_get_cc_state(ppd);

	if (!cc_state) {
		rcu_read_unlock();
		return reply(ibh);
	}

	sentry = start_block * HFI_IB_CCT_ENTRIES;
	eentry = sentry + (HFI_IB_CCT_ENTRIES * n_blocks);

	cc_table_attr->ccti_limit = cpu_to_be16(cc_state->cct.ccti_limit);

	entries = cc_state->cct.entries;

	/* return n_blocks, though the last block may not be full */
	for (j = 0, i = sentry; i < eentry; j++, i++)
		cc_table_attr->ccti_entries[j].entry =
			cpu_to_be16(entries[i].entry);

	rcu_read_unlock();

	if (resp_len)
		*resp_len += sizeof(u16) * (HFI_IB_CCT_ENTRIES * n_blocks + 1);

	return reply(ibh);
}

static int subn_get_opa_sma(u16 attr_id, struct opa_smp *smp, u32 am,
			    u8 *data, struct ib_device *ibdev, u8 port,
			    u32 *resp_len)
{
	int ret;
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	struct hfi2_ibport *ibp = to_hfi_ibp(ibdev, port);
	/*
	 * FXRTODO: Only get node info supported in MAD methods.
	 * Others yet to be implemented.
	 */
	switch (attr_id) {
	case IB_SMP_ATTR_NODE_DESC:
		ret = __subn_get_opa_nodedesc(smp, am, data, ibdev, port,
					      resp_len);
		break;
	case IB_SMP_ATTR_NODE_INFO:
		ret = __subn_get_opa_nodeinfo(smp, am, data, ibdev, port,
					      resp_len);
		break;
	case IB_SMP_ATTR_PORT_INFO:
		ret = __subn_get_opa_portinfo(smp, am, data, ibdev, port,
					      resp_len);
		break;
	case OPA_ATTRIB_ID_PORT_STATE_INFO:
		ret = __subn_get_opa_psi(smp, am, data, ibdev, port,
					 resp_len);
		break;
	case IB_SMP_ATTR_PKEY_TABLE:
		ret = __subn_get_opa_pkeytable(smp, am, data, ibdev, port,
					       resp_len);
		break;
	case OPA_ATTRIB_ID_SL_TO_SC_MAP:
		ret = __subn_get_opa_sl_to_sc(smp, am, data, ibdev, port,
					      resp_len);
		break;
	case OPA_ATTRIB_ID_SC_TO_SL_MAP:
		ret = __subn_get_opa_sc_to_sl(smp, am, data, ibdev, port,
					      resp_len);
		break;
	case OPA_ATTRIB_ID_SC_TO_VLR_MAP:
		ret = __subn_get_opa_sc_to_vlr(smp, am, data, ibdev, port,
					       resp_len);
		break;
	case OPA_ATTRIB_ID_SC_TO_VLT_MAP:
		ret = __subn_get_opa_sc_to_vlt(smp, am, data, ibdev, port,
					       resp_len);
		break;
	case OPA_ATTRIB_ID_SC_TO_VLNT_MAP:
		ret = __subn_get_opa_sc_to_vlnt(smp, am, data, ibdev, port,
					       resp_len);
		break;
	case OPA_ATTRIB_ID_BW_ARBITRATION:
		ret = __subn_get_opa_bw_arb(smp, am, data, ibdev, port,
					    resp_len);
		break;
	case IB_SMP_ATTR_LED_INFO:
		ret = __subn_get_opa_led_info(smp, am, data, ibdev, port,
					      resp_len);
		break;
	case OPA_ATTRIB_ID_CABLE_INFO:
		ret = __subn_get_opa_cable_info(smp, am, data, ibdev, port,
						resp_len);
		break;
	case OPA_ATTRIB_ID_BUFFER_CONTROL_TABLE:
		ret = __subn_get_opa_bct(smp, am, data, ibdev, port,
					 resp_len);
		break;
	case OPA_ATTRIB_ID_CONGESTION_INFO:
		ret = __subn_get_opa_cong_info(smp, am, data, ibdev, port,
					       resp_len);
		break;
	case OPA_ATTRIB_ID_HFI_CONGESTION_LOG:
		ret = __subn_get_opa_hfi_cong_log(smp, am, data, ibdev,
						  port, resp_len);
		break;
	case OPA_ATTRIB_ID_HFI_CONGESTION_SETTING:
		ret = __subn_get_opa_cong_setting(smp, am, data, ibdev,
						  port, resp_len);
		break;
	case OPA_ATTRIB_ID_CONGESTION_CONTROL_TABLE:
		ret = __subn_get_opa_cc_table(smp, am, data, ibdev, port,
					      resp_len);
		break;
	case IB_SMP_ATTR_SM_INFO:
		if (ibp->port_cap_flags & IB_PORT_SM_DISABLED)
			return IB_MAD_RESULT_SUCCESS | IB_MAD_RESULT_CONSUMED;
		if (ibp->port_cap_flags & IB_PORT_SM)
			return IB_MAD_RESULT_SUCCESS;
		/* FALLTHROUGH */
	default:
		smp->status |=
		cpu_to_be16(IB_MGMT_MAD_STATUS_UNSUPPORTED_METHOD_ATTRIB);
		ret = reply(ibh);
		break;
	}
	return ret;
}

static void hfi_set_link_width_enabled(struct hfi_pportdata *ppd, u32 w)
{
	hfi_set_ib_cfg(ppd, HFI_IB_CFG_LWID_ENB, w, NULL);
}

static void hfi_set_link_width_downgrade_enabled(struct hfi_pportdata *ppd,
						 u32 w)
{
	hfi_set_ib_cfg(ppd, HFI_IB_CFG_LWID_DG_ENB, w, NULL);
}

static void hfi_set_link_speed_enabled(struct hfi_pportdata *ppd, u32 s)
{
	hfi_set_ib_cfg(ppd, HFI_IB_CFG_SPD_ENB, s, NULL);
}

static int set_port_states(struct hfi_pportdata *ppd,
			   struct opa_smp *smp, u32 lstate, u32 pstate,
			   u8 suppress_idle_sma)
{
	/* HFI driver clubs pstate and lstate to Host Link State (HLS) */
	u32 hls;
	int ret;

	if (pstate && !(lstate == IB_PORT_DOWN || lstate == IB_PORT_NOP)) {
		pr_warn("SubnSet(OPA_PortInfo) port state invalid; port physstate 0x%x link state 0x%x\n"
			, pstate, lstate);
		hfi_invalid_attr(smp);
	}

	/*
	 * Only state changes of DOWN, ARM, and ACTIVE are valid
	 * and must be in the correct state to take effect (see 7.2.6).
	 */
	switch (lstate) {
	case IB_PORT_NOP:
		if (pstate == IB_PORTPHYSSTATE_NOP)
			break;
		/* FALLTHROUGH */
	case IB_PORT_DOWN:
		switch (pstate) {
		case IB_PORTPHYSSTATE_NOP:
			hls = HLS_DN_DOWNDEF;
			break;
		case IB_PORTPHYSSTATE_POLLING:
			hls = HLS_DN_POLL;
			hfi_set_link_down_reason(ppd,
						 OPA_LINKDOWN_REASON_FM_BOUNCE,
						 0,
						 OPA_LINKDOWN_REASON_FM_BOUNCE);
			break;
		case IB_PORTPHYSSTATE_DISABLED:
			hls = HLS_DN_DISABLE;
			break;
		default:
			pr_warn("SubnSet(OPA_PortInfo) invalid Physical state 0x%x\n",
				lstate);
			hfi_invalid_attr(smp);
			goto done;
		}

		hfi_set_link_state(ppd, hls);
		if (hls == HLS_DN_DISABLE && (ppd->offline_disabled_reason >
		    OPA_LINKDOWN_REASON_SMA_DISABLED ||
		    ppd->offline_disabled_reason == OPA_LINKDOWN_REASON_NONE))
			ppd->offline_disabled_reason =
			OPA_LINKDOWN_REASON_SMA_DISABLED;
		/*
		 * Don't send a reply if the response would be sent
		 * through the disabled port.
		 */
		if (hls == HLS_DN_DISABLE && smp->hop_cnt)
			return IB_MAD_RESULT_SUCCESS | IB_MAD_RESULT_CONSUMED;
		break;
	case IB_PORT_ARMED:
		ret = hfi_set_link_state(ppd, HLS_UP_ARMED);
		if ((ret == 0) && (suppress_idle_sma == 0))
			hfi_send_idle_sma(ppd, SMA_IDLE_ARM);
		break;
	case IB_PORT_ACTIVE:
		if (quick_linkup || ppd->neighbor_normal) {
			ret = hfi_set_link_state(ppd, HLS_UP_ACTIVE);
			if (ret == 0)
				hfi_send_idle_sma(ppd, SMA_IDLE_ACTIVE);
		} else {
			pr_warn("SubnSet(OPA_PortInfo) Cannot move to Active with NeighborNormal 0\n");
			hfi_invalid_attr(smp);
		}
		break;
	default:
		pr_warn("SubnSet(OPA_PortInfo) invalid state 0x%x\n", lstate);
		hfi_invalid_attr(smp);
	}
done:
	return 0;
}

static int __subn_set_opa_portinfo(struct opa_smp *smp, u32 am, u8 *data,
				struct ib_device *ibdev, u8 port,
					      u32 *resp_len)
{
	struct opa_port_info *pi = (struct opa_port_info *)data;
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	struct ib_event event;
	struct hfi_devdata *dd;
	struct hfi_pportdata *ppd;
	struct hfi2_ibport *ibp;
	u8 clientrereg;
	u32 smlid;
	u32 lid;
	u8 ls_old, ls_new, ps_new;
	u8 vls;
	u8 msl;
	u8 crc_enabled;
	u8 pkt_formats_enabled;
	u16 lse, lwe, mtu;
	unsigned long flags;
	u32 num_ports = OPA_AM_NPORT(am);
	u32 start_of_sm_config = OPA_AM_START_SM_CFG(am);
	int ret, i, invalid = 0, call_set_mtu = 0;
	int call_link_downgrade_policy = 0;

	if (num_ports != 1) {
		hfi_invalid_attr(smp);
		return reply(ibh);
	}

	/* bail out early if the LID and SMLID are invalid */
	lid = be32_to_cpu(pi->lid);
	if (lid & 0xFF000000) {
		pr_warn("OPA_PortInfo lid out of range: %X\n", lid);
		pr_warn("(> 24b LIDs not supported)\n");
		hfi_invalid_attr(smp);
		goto get_only;
	}

	smlid = be32_to_cpu(pi->sm_lid);
	if (smlid & 0xFF000000) {
		pr_warn("OPA_PortInfo SM lid out of range: %X\n", smlid);
		pr_warn("(> 24b LIDs not supported)\n");
		hfi_invalid_attr(smp);
		goto get_only;
	}

	clientrereg = (pi->clientrereg_subnettimeout &
			OPA_PI_MASK_CLIENT_REREGISTER);

	dd = hfi_dd_from_ibdev(ibdev);
	/* IB numbers ports from 1, hw from 0 */
	ppd = to_hfi_ppd(dd, port);
	ibp = to_hfi_ibp(ibdev, port);
	event.device = ibdev;
	event.element.port_num = port;

	ls_old = hfi_driver_lstate(ppd);

	ibp->mkey = pi->mkey;
	ibp->gid_prefix = pi->subnet_prefix;
	ibp->mkey_lease_period = be16_to_cpu(pi->mkey_lease_period);

	/* Must be a valid unicast LID address. */
	if ((lid == 0 && ls_old > IB_PORT_INIT) ||
	    (lid >= HFI1_16B_MULTICAST_LID_BASE)) {
		hfi_invalid_attr(smp);
		pr_warn("SubnSet(OPA_PortInfo) lid invalid 0x%x\n", lid);
	} else if (ppd->lid != lid ||
		 ppd->lmc != (pi->mkeyprotect_lmc & OPA_PI_MASK_LMC)) {
		/* FXRTODO: implement uvents for FXR */
#if 0
		if (ppd->lid != lid)
			hfi1_set_uevent_bits(ppd, _HFI1_EVENT_LID_CHANGE_BIT);
		if (ppd->lmc != (pi->mkeyprotect_lmc & OPA_PI_MASK_LMC))
			hfi1_set_uevent_bits(ppd, _HFI1_EVENT_LMC_CHANGE_BIT);
#endif
		hfi_set_lid(ppd, lid, pi->mkeyprotect_lmc & OPA_PI_MASK_LMC);
		event.event = IB_EVENT_LID_CHANGE;
		ib_dispatch_event(&event);

		/* Manufacture GID from LID to support extended addresses */
		ibp->guids[0] = OPA_MAKE_GID(lid);
		event.event = IB_EVENT_GID_CHANGE;
		ib_dispatch_event(&event);
	}
	ret = hfi_set_max_lid(ppd, be32_to_cpu(pi->max_lid));
	if (ret) {
		hfi_invalid_attr(smp);
		goto get_only;
	}

	msl = pi->smsl & OPA_PI_MASK_SMSL;
	if (pi->partenforce_filterraw & OPA_PI_MASK_LINKINIT_REASON)
		ppd->linkinit_reason =
			(pi->partenforce_filterraw &
			 OPA_PI_MASK_LINKINIT_REASON);
	/* enable/disable SW pkey checking as per FM control */
	if (pi->partenforce_filterraw & OPA_PI_MASK_PARTITION_ENFORCE_IN) {
		ppd->part_enforce |= HFI_PART_ENFORCE_IN;
		hfi_cfg_in_pkey_check(ppd, 1);
	} else {
		/*
		 * FXRTODO: Ignore FM value if neighbhor is HFI. Currently
		 * FM always sends 0. Design discussion still pending
		 */
		if (!neigh_is_hfi(ppd)) {
			ppd->part_enforce &= ~HFI_PART_ENFORCE_IN;
			hfi_cfg_in_pkey_check(ppd, 0);
		}
	}

	if (pi->partenforce_filterraw & OPA_PI_MASK_PARTITION_ENFORCE_OUT) {
		ppd->part_enforce |= HFI_PART_ENFORCE_OUT;
		hfi_cfg_out_pkey_check(ppd, 1);
	} else {
		/*
		 * FXRTODO: Ignore FM value if neighbhor is HFI. Currently
		 * FM always sends 0. Design discussion still pending
		 */
		if (!neigh_is_hfi(ppd)) {
			ppd->part_enforce &= ~HFI_PART_ENFORCE_OUT;
			hfi_cfg_out_pkey_check(ppd, 0);
		}
	}

	/* Must be a valid unicast LID address. */
	if ((smlid == 0 && ls_old > IB_PORT_INIT) ||
	    (smlid >= HFI1_16B_MULTICAST_LID_BASE)) {
		hfi_invalid_attr(smp);
		pr_warn("SubnSet(OPA_PortInfo) smlid invalid 0x%x\n", smlid);
	} else if (smlid != ibp->sm_lid || msl != ibp->sm_sl) {
		pr_warn("SubnSet(OPA_PortInfo) smlid 0x%x\n", smlid);
		spin_lock_irqsave(&ibp->lock, flags);
		if (ibp->rvp.sm_ah) {
			if (smlid != ibp->sm_lid)
				hfi2_update_sm_ah_attr(ibp, smlid);
			if (msl != ibp->sm_sl)
				ibp->rvp.sm_ah->attr.sl = msl;
		}
		spin_unlock_irqrestore(&ibp->lock, flags);
		if (smlid != ibp->sm_lid)
			ibp->sm_lid = smlid;
		if (msl != ibp->sm_sl)
			ibp->sm_sl = msl;
		event.event = IB_EVENT_SM_CHANGE;
		ib_dispatch_event(&event);
	}

	if (pi->link_down_reason == 0) {
		ppd->local_link_down_reason.sma = 0;
		ppd->local_link_down_reason.latest = 0;
	}

	if (pi->neigh_link_down_reason == 0) {
		ppd->neigh_link_down_reason.sma = 0;
		ppd->neigh_link_down_reason.latest = 0;
	}

	ppd->sm_trap_qp = be32_to_cpu(pi->sm_trap_qp);
	ppd->sa_qp = be32_to_cpu(pi->sa_qp);

	/*
	 * FXRTODO: This mask is to be read during FC/MNH error interrupts
	 * and if the error matches the bits in this mask then link
	 * bounce is initiated by the driver. See JIRA STL-70.
	 */
	ppd->port_error_action = be32_to_cpu(pi->port_error_action);
	lwe = be16_to_cpu(pi->link_width.enabled);
	if (lwe) {
		if (lwe == HFI_LINK_WIDTH_RESET ||
		    lwe == HFI_LINK_WIDTH_RESET_OLD)
			hfi_set_link_width_enabled(ppd,
						   ppd->link_width_supported);
		else if ((lwe & ~ppd->link_width_supported) == 0)
			hfi_set_link_width_enabled(ppd, lwe);
		else
			hfi_invalid_attr(smp);
	}
	lwe = be16_to_cpu(pi->link_width_downgrade.enabled);
	/* LWD.E is always applied - 0 means "disabled" */
	if (lwe == HFI_LINK_WIDTH_RESET ||
	    lwe == HFI_LINK_WIDTH_RESET_OLD) {
		hfi_set_link_width_downgrade_enabled(ppd,
				ppd->link_width_downgrade_supported);
	} else if ((lwe & ~ppd->link_width_downgrade_supported) == 0) {
		/* only set and apply if something changed */
		if (lwe != ppd->link_width_downgrade_enabled) {
			hfi_set_link_width_downgrade_enabled(ppd, lwe);
			call_link_downgrade_policy = 1;
		}
	} else {
		hfi_invalid_attr(smp);
	}

	lse = be16_to_cpu(pi->link_speed.enabled);
	if (lse) {
		if (lse & be16_to_cpu(pi->link_speed.supported))
			hfi_set_link_speed_enabled(ppd, lse);
		else
			hfi_invalid_attr(smp);
	}

	ibp->mkeyprot = (pi->mkeyprotect_lmc & OPA_PI_MASK_MKEY_PROT_BIT) >> 6;
	ibp->vl_high_limit = be16_to_cpu(pi->vl.high_limit) & 0xFF;
	hfi_set_ib_cfg(ppd, HFI_IB_CFG_VL_HIGH_LIMIT,
		       ibp->vl_high_limit, NULL);

	pkt_formats_enabled = be16_to_cpu(pi->port_packet_format.enabled);
	if (!(pkt_formats_enabled & OPA_PORT_PACKET_FORMAT_16B) &&
	    (ppd->max_lid >= HFI1_MULTICAST_LID_BASE)) {
		ppd_dev_warn(ppd, "Cannot disable 16B with max_lid 0x%#x\n",
			     ppd->max_lid);
		hfi_invalid_attr(smp);
		pkt_formats_enabled |= OPA_PORT_PACKET_FORMAT_16B;
	}

	if (~ppd->packet_format_supported & pkt_formats_enabled) {
		ppd_dev_warn(ppd, "Cannot enable packet format that is not supported (sup: %#x, enb: %#x\n"
		, ppd->packet_format_supported, pkt_formats_enabled);
		hfi_invalid_attr(smp);
		pkt_formats_enabled &= ppd->packet_format_supported;
	}

	hfi_set_ib_cfg(ppd, HFI_IB_CFG_PKT_FORMAT, 0, &pkt_formats_enabled);

	if (ppd->vls_supported / 2 > ARRAY_SIZE(pi->neigh_mtu.pvlx_to_mtu) ||
	    ppd->vls_supported > ARRAY_SIZE(ppd->vl_mtu)) {
		hfi_invalid_attr(smp);
		return reply(ibh);
	}

	for (i = 0; i < ppd->vls_supported; i++) {
		mtu = opa_pi_to_mtu(pi, i);

		if (mtu == INVALID_MTU) {
			hfi_invalid_attr(smp);
			/* use the existing mtu */
			continue;
		}

		if (ppd->vl_mtu[i] != mtu) {
			ppd_dev_info(ppd,
				     "MTU change on vl %d from %d to %d\n",
				i, ppd->vl_mtu[i], mtu);
			ppd->vl_mtu[i] = mtu;
			call_set_mtu++;
		}

		if (ppd->ibmtu < ppd->vl_mtu[i])
			ppd->ibmtu = ppd->vl_mtu[i];
	}

	/*
	 * As per OPAV1 spec: VL15 must support and be configured
	 * for operation with a 2048 or larger MTU.
	 */
	mtu = opa_enum_to_mtu(pi->neigh_mtu.pvlx_to_mtu[15 / 2] & 0xF);
	if (mtu < HFI_MIN_VL_15_MTU || mtu == INVALID_MTU) {
		hfi_invalid_attr(smp);
		/* use the existing VL15 MTU */
	} else {
		if (ppd->vl_mtu[15] != mtu) {
			ppd_dev_info(ppd,
				     "MTU change on vl 15 from %d to %d\n",
				ppd->vl_mtu[15], mtu);
			ppd->vl_mtu[15] = mtu;
			call_set_mtu++;
		}
	}

	if (call_set_mtu)
		hfi_set_mtu(ppd);

	/* Set operational VLs */
	vls = pi->operational_vls & OPA_PI_MASK_OPERATIONAL_VL;
	if (vls) {
		if (vls > ppd->vls_supported) {
			pr_warn("SubnSet(OPA_PortInfo) VL's supported invalid %d\n",
				pi->operational_vls);
			hfi_invalid_attr(smp);
		} else
			hfi_set_ib_cfg(ppd, HFI_IB_CFG_OP_VLS,
				       vls, NULL);
	}

	if (pi->mkey_violations == 0)
		ibp->mkey_violations = 0;

	if (pi->pkey_violations == 0)
		ibp->pkey_violations = 0;

	if (pi->qkey_violations == 0)
		ibp->qkey_violations = 0;

	ibp->subnet_timeout =
		pi->clientrereg_subnettimeout & OPA_PI_MASK_SUBNET_TIMEOUT;

	crc_enabled = be16_to_cpu(pi->port_ltp_crc_mode);
	crc_enabled = HFI_LTP_CRC_ENABLED(crc_enabled);

	if (crc_enabled)
		ppd->port_crc_mode_enabled = hfi_port_ltp_to_cap(crc_enabled);

	ppd->is_active_optimize_enabled =
			!!(be16_to_cpu(pi->port_mode)
					& OPA_PI_MASK_PORT_ACTIVE_OPTOMIZE);
	ppd->is_vl_marker_enabled =
			!!(be16_to_cpu(pi->port_mode)
					& OPA_PI_MASK_PORT_MODE_VL_MARKER);

	ls_new = pi->port_states.portphysstate_portstate &
			OPA_PI_MASK_PORT_STATE;
	ps_new = (pi->port_states.portphysstate_portstate &
			OPA_PI_MASK_PORT_PHYSICAL_STATE) >> 4;

	if (ls_old == IB_PORT_INIT) {
		if (start_of_sm_config) {
			if (ls_new == ls_old || (ls_new == IB_PORT_ARMED))
				ppd->is_sm_config_started = 1;
		} else if (ls_new == IB_PORT_ARMED) {
			if (ppd->is_sm_config_started == 0)
				invalid = 1;
		}
	}

	/* Handle CLIENT_REREGISTER event b/c SM asked us for it */
	if (clientrereg) {
		event.event = IB_EVENT_CLIENT_REREGISTER;
		ib_dispatch_event(&event);
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

	ret = __subn_get_opa_portinfo(smp, am, data, ibdev, port, resp_len);

	/* restore re-reg bit per o14-12.2.1 */
	pi->clientrereg_subnettimeout |= clientrereg;

	/*
	 * Apply the new link downgrade policy.  This may result in a link
	 * bounce.  Do this after everything else so things are settled.
	 * Possible problem: if setting the port state above fails, then
	 * the policy change is not applied.
	 */
	if (call_link_downgrade_policy)
		hfi_apply_link_downgrade_policy(ppd, 0);

	return ret;

get_only:
	return __subn_get_opa_portinfo(smp, am, data, ibdev, port, resp_len);
}

 /*
 * @returns the index of lim_pkey if found or -1 if not found
 */
int hfi_lookup_lim_pkey_idx(struct hfi_pportdata *ppd)
{
	unsigned i;

	for (i = 0; i < HFI_MAX_PKEYS; i++)
		if (ppd->pkeys[i] == OPA_LIM_MGMT_PKEY)
			return i;

	/* no match...  */
	return -1;
}

static int __subn_set_opa_pkeytable(struct opa_smp *smp, u32 am, u8 *data,
				struct ib_device *ibdev, u8 port,
					       u32 *resp_len)
{
	struct hfi_devdata *dd = hfi_dd_from_ibdev(ibdev);
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	u32 n_blocks_sent = OPA_AM_NBLK(am);
	u32 start_block = OPA_AM_START_BLK(am);
	u32 end_block;
	__be16 *pkeys = (__be16 *)data;
	int i, j, k;
	u8 update_mgmt_pkey = 0, pkey_changed = 0;

	end_block = start_block + n_blocks_sent;

	if (n_blocks_sent == 0 || end_block > HFI_PKEY_BLOCKS_AVAIL ||
	    n_blocks_sent > OPA_NUM_PKEY_BLOCKS_PER_SMP) {
		pr_warn("OPA Set PKey AM Invalid : s 0x%x; req 0x%x; ",
			start_block, n_blocks_sent);
		pr_warn(" avail 0x%x; blk/smp 0x%lx\n", HFI_PKEY_BLOCKS_AVAIL,
			OPA_NUM_PKEY_BLOCKS_PER_SMP);
		hfi_invalid_attr(smp);
		return reply(ibh);
	}

	 /* Table update should contain update to OPA_LIM_MGMT_PKEY */
	for (i = 0; i < n_blocks_sent * OPA_PKEY_TABLE_BLK_COUNT; i++) {
		if (be16_to_cpu(pkeys[i]) == OPA_LIM_MGMT_PKEY) {
			update_mgmt_pkey = 1;
			break;
		}
	}

	if (!update_mgmt_pkey) {
		int mgmt_idx = hfi_lookup_lim_pkey_idx(ppd);

		/* we should always have mgmt pkey set in the pkeyTable */
		if (mgmt_idx == -1) {
			WARN_ONCE(1, "Mgmt Pkey not present in PkeyTable\n");
			return IB_MAD_RESULT_FAILURE;
		}

		/*
		 * Since the setPkeyTable can set tables in blocks
		 * it is possible that FM is trying to change
		 * a blockin pkeytable that doesn't contain mgmt pkey.
		 * Check for that condition here.
		 */
		if (mgmt_idx >= (start_block * OPA_PKEY_TABLE_BLK_COUNT) &&
		    mgmt_idx < (end_block * OPA_PKEY_TABLE_BLK_COUNT)) {
			/*
			 * The FM was trying to replace a pkey table block
			 * which will leave no mgmt pkey in the entire
			 * pkey table. Donot allow that.
			 */
			pr_warn("Update does not contain mgmt pkey\n");
			hfi_invalid_attr(smp);
			return reply(ibh);
		}
	}

	for (i = start_block, k = 0; i < end_block; i++) {
		for (j = 0; j < OPA_PKEY_TABLE_BLK_COUNT; j++, k++) {
			u16 pidx = i * OPA_PKEY_TABLE_BLK_COUNT + j;
			u16 curr_pkey = ppd->pkeys[pidx];
			u16 new_pkey = be16_to_cpu(pkeys[k]);

			if (curr_pkey == new_pkey)
				continue;

			ppd->pkeys[pidx] = new_pkey;
			pkey_changed = 1;
		}
	}

	if (pkey_changed) {
		(void)hfi_set_ib_cfg(ppd, HFI_IB_CFG_PKEYS, 0, NULL);
	/*
	 * FXRTODO: Need a notification path from opa_core device
	 * to opa_core clients
	 */
#if 0
		struct ib_event event;
		event.event = IB_EVENT_PKEY_CHANGE;
		event.device = ibdev;
		event.element.port_num = port;
		ib_dispatch_event(&event);
#endif
	}

	return __subn_get_opa_pkeytable(smp, am, data, ibdev, port, resp_len);
}

static int __subn_set_opa_sl_to_sc(struct opa_smp *smp, u32 am, u8 *data,
				struct ib_device *ibdev, u8 port,
					      u32 *resp_len)
{
	struct hfi_devdata *dd = hfi_dd_from_ibdev(ibdev);
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	int i;
	int sl_len = ARRAY_SIZE(ppd->sl_to_sc);
	u8 *p = (u8 *)data;
	u8 map_changed = 0;

	if (am)
		goto err;

	/* sc entry should be 5 bits long */
	for (i = 0; i < sl_len; i++)
		if (p[i] >= OPA_MAX_SCS)
			goto err;

	for (i = 0; i < sl_len; i++) {
		if (ppd->sl_to_sc[i] != p[i]) {
			map_changed = 1;
			ppd->sl_to_sc[i] = p[i];
		}
	}

	if (map_changed)
		hfi_set_ib_cfg(ppd, HFI_IB_CFG_SL_TO_SC, 0, NULL);

	goto done;

err:
	hfi_invalid_attr(smp);
	return reply(ibh);
done:
	return __subn_get_opa_sl_to_sc(smp, am, data, ibdev, port, resp_len);
}

static int __subn_set_opa_sc_to_sl(struct opa_smp *smp, u32 am, u8 *data,
				struct ib_device *ibdev, u8 port,
					      u32 *resp_len)
{
	struct hfi_devdata *dd = hfi_dd_from_ibdev(ibdev);
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	int i;
	int sc_len = ARRAY_SIZE(ppd->sc_to_sl);
	u8 *p = (u8 *)data;
	u8 map_changed = 0;

	if (am)
		goto err;

	/* sl entry should be 5 bits long */
	for (i = 0; i < sc_len; i++) {
		if (p[i] >= OPA_MAX_SLS)
			goto err;
		if (ppd->sc_to_sl[i] != p[i])
			map_changed = 1;
	}

	if (map_changed) {
		for (i = 0; i < sc_len; i++)
			ppd->sc_to_sl[i] = p[i];
		hfi_set_ib_cfg(ppd, HFI_IB_CFG_SC_TO_RESP_SL, 0, p);
		hfi_set_ib_cfg(ppd, HFI_IB_CFG_SC_TO_MCTC, 0, p);
	}

	goto done;

err:
	hfi_invalid_attr(smp);
	return reply(ibh);
done:
	return __subn_get_opa_sc_to_sl(smp, am, data, ibdev, port, resp_len);
}

static int __subn_set_opa_sc_to_vlr(struct opa_smp *smp, u32 am, u8 *data,
				    struct ib_device *ibdev, u8 port,
				    u32 *resp_len)
{
	struct hfi_devdata *dd = hfi_dd_from_ibdev(ibdev);
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	u32 n_blocks = OPA_AM_NBLK(am);
	u8 *p = (void *)data;
	int i, lstate, map_changed = 0;
	int sc_len = ARRAY_SIZE(ppd->sc_to_vlr);

	if (n_blocks != 1)
		goto err;

	lstate = hfi_driver_lstate(ppd);


	for (i = 0; i < sc_len; i++) {
		if (p[i] >= OPA_MAX_VLS)
			goto err;
		if (ppd->sc_to_vlr[i] != p[i])
			map_changed = 1;
	}

	if (map_changed)
		hfi_set_ib_cfg(ppd, HFI_IB_CFG_SC_TO_VLR, 0, p);

	goto done;
err:
	hfi_invalid_attr(smp);
	return reply(ibh);
done:
	return  __subn_get_opa_sc_to_vlr(smp, am, data, ibdev, port,
					       resp_len);
}

static int __subn_set_opa_sc_to_vlt(struct opa_smp *smp, u32 am, u8 *data,
				struct ib_device *ibdev, u8 port,
					       u32 *resp_len)
{
	struct hfi_devdata *dd = hfi_dd_from_ibdev(ibdev);
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	u32 n_blocks = OPA_AM_NBLK(am);
	int async_update = OPA_AM_ASYNC(am);
	u8 *p = (void *)data;
	int i, lstate, map_changed = 0;
	int sc_len = ARRAY_SIZE(ppd->sc_to_vlt);

	if (n_blocks != 1 || async_update)
		goto err;

	lstate = hfi_driver_lstate(ppd);

	if (!async_update &&
	    (lstate == IB_PORT_ARMED || lstate == IB_PORT_ACTIVE))
		goto err;

	for (i = 0; i < sc_len; i++) {
		if (p[i] >= OPA_MAX_VLS)
			goto err;
		if (ppd->sc_to_vlt[i] != p[i])
			map_changed = 1;
	}

	if (map_changed)
		hfi_set_ib_cfg(ppd, HFI_IB_CFG_SC_TO_VLT, 0, p);

	goto done;
err:
	hfi_invalid_attr(smp);
	return reply(ibh);
done:
	return __subn_get_opa_sc_to_vlt(smp, am, data, ibdev, port, resp_len);
}

static int __subn_set_opa_sc_to_vlnt(struct opa_smp *smp, u32 am, u8 *data,
				struct ib_device *ibdev, u8 port,
					       u32 *resp_len)
{
	struct hfi_devdata *dd = hfi_dd_from_ibdev(ibdev);
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	u32 n_blocks = OPA_AM_NBLK(am);
	u8 *p = (void *)data;
	int i, lstate, map_changed = 0;
	int sc_len = ARRAY_SIZE(ppd->sc_to_vlnt);

	if (n_blocks != 1)
		goto err;

	lstate = hfi_driver_lstate(ppd);

	if (lstate == IB_PORT_ARMED || lstate == IB_PORT_ACTIVE)
		goto err;

	for (i = 0; i < sc_len; i++) {
		if (p[i] >= OPA_MAX_VLS)
			goto err;
		if (ppd->sc_to_vlnt[i] != p[i])
			map_changed = 1;
	}

	if (map_changed)
		hfi_set_ib_cfg(ppd, HFI_IB_CFG_SC_TO_VLNT, 0, p);
	goto done;

err:
	hfi_invalid_attr(smp);
	return reply(ibh);
done:
	return __subn_get_opa_sc_to_vlnt(smp, am, data, ibdev, port,
					 resp_len);
}

static int __subn_set_opa_psi(struct opa_smp *smp, u32 am, u8 *data,
				struct ib_device *ibdev, u8 port,
					 u32 *resp_len)
{
	u32 nports = OPA_AM_NPORT(am);
	u32 start_of_sm_config = OPA_AM_START_SM_CFG(am);
	u32 ls_old;
	u8 ls_new, ps_new;
	struct hfi_devdata *dd;
	struct hfi_pportdata *ppd;
	struct opa_port_state_info *psi = (struct opa_port_state_info *)data;
	int ret, invalid = 0;

	if (nports != 1) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply((struct ib_mad_hdr *)smp);
	}

	dd = hfi_dd_from_ibdev(ibdev);
	/* IB numbers ports from 1, hw from 0 */
	ppd = to_hfi_ppd(dd, port);

	ls_old = hfi_driver_lstate(ppd);

	ls_new = psi->port_states.portphysstate_portstate &
			OPA_PI_MASK_PORT_STATE;
	ps_new = (psi->port_states.portphysstate_portstate &
			OPA_PI_MASK_PORT_PHYSICAL_STATE) >> 4;

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

	return __subn_get_opa_psi(smp, am, data, ibdev, port, resp_len);
}

static int __subn_set_opa_bct(struct opa_smp *smp, u32 am, u8 *data,
				struct ib_device *ibdev, u8 port,
					 u32 *resp_len)
{
	struct hfi_devdata *dd = hfi_dd_from_ibdev(ibdev);
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	struct buffer_control *p = (struct buffer_control *)data;
	u32 num_ports = OPA_AM_NPORT(am);

	if (num_ports != 1)
		goto fail;

	if (hfi_set_buffer_control(ppd, p) < 0)
		goto fail;

	goto done;
fail:
	hfi_invalid_attr(smp);
	return reply(ibh);
done:
	return __subn_get_opa_bct(smp, am, data, ibdev, port, resp_len);
}

static int __subn_set_opa_bw_arb(struct opa_smp *smp, u32 am, u8 *data,
				 struct ib_device *ibdev, u8 port,
				 u32 *resp_len)
{
	struct hfi_devdata *dd = hfi_dd_from_ibdev(ibdev);
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	u32 num_ports = OPA_AM_NPORT(am);
	u8 section = OPA_BW_SECTION(am);

	if ((num_ports != 1) || (section != OPA_BWARB_GROUP &&
				 section != OPA_BWARB_PREEMPT_MATRIX))
		goto err;

	hfi_set_ib_cfg(ppd, HFI_IB_CFG_BW_ARB, section, data);

	goto done;
err:
	hfi_invalid_attr(smp);
	return reply(ibh);
done:
	return __subn_get_opa_bw_arb(smp, am, data, ibdev, port, resp_len);
}

static int __subn_set_opa_cong_setting(struct opa_smp *smp, u32 am, u8 *data,
					struct ib_device *ibdev, u8 port,
						u32 *resp_len)
{
	struct hfi_devdata *dd = hfi_dd_from_ibdev(ibdev);
	struct opa_congestion_setting_attr *p =
		(struct opa_congestion_setting_attr *)data;
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	struct opa_congestion_setting_entry_shadow *entries;
	int i;

	ppd->cc_sl_control_map = be32_to_cpu(p->control_map);

	entries = ppd->congestion_entries;
	for (i = 0; i < OPA_MAX_SLS; i++) {
		entries[i].ccti_increase = p->entries[i].ccti_increase;
		entries[i].ccti_timer = be16_to_cpu(p->entries[i].ccti_timer);
		entries[i].trigger_threshold =
			p->entries[i].trigger_threshold;
		entries[i].ccti_min = p->entries[i].ccti_min;
	}

	return __subn_get_opa_cong_setting(smp, am, data, ibdev, port,
					   resp_len);
}

static int __subn_set_opa_cc_table(struct opa_smp *smp, u32 am, u8 *data,
				struct ib_device *ibdev, u8 port,
					      u32 *resp_len)
{
	struct hfi_devdata *dd = hfi_dd_from_ibdev(ibdev);
	struct ib_cc_table_attr *p = (struct ib_cc_table_attr *)data;
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	u32 start_block = OPA_AM_START_BLK(am);
	u32 n_blocks = OPA_AM_NBLK(am);
	struct ib_cc_table_entry_shadow *entries;
	int i, j;
	u32 sentry, eentry;
	u16 ccti_limit;
	struct cc_state *old_cc_state, *new_cc_state;

	/* sanity check n_blocks, start_block */
	if (n_blocks == 0 ||
	    start_block + n_blocks > ppd->cc_max_table_entries) {
		hfi_invalid_attr(smp);
		goto done;
	}

	sentry = start_block * HFI_IB_CCT_ENTRIES;
	eentry = sentry + ((n_blocks - 1) * HFI_IB_CCT_ENTRIES) +
		 (be16_to_cpu(p->ccti_limit)) % HFI_IB_CCT_ENTRIES + 1;

	/* sanity check ccti_limit */
	ccti_limit = be16_to_cpu(p->ccti_limit);
	if (ccti_limit + 1 > eentry) {
		hfi_invalid_attr(smp);
		goto done;
	}

	new_cc_state = kzalloc(sizeof(*new_cc_state), GFP_KERNEL);
	if (!new_cc_state)
		goto done;

	spin_lock(&ppd->cc_state_lock);

	old_cc_state = hfi_get_cc_state(ppd);

	if (!old_cc_state) {
		spin_unlock(&ppd->cc_state_lock);
		kfree(new_cc_state);
		goto done;
	}

	*new_cc_state = *old_cc_state;

	new_cc_state->cct.ccti_limit = ccti_limit;

	entries = ppd->ccti_entries;
	ppd->total_cct_entry = ccti_limit + 1;

	for (j = 0, i = sentry; i < eentry; j++, i++)
		entries[i].entry = be16_to_cpu(p->ccti_entries[j].entry);

	memcpy(new_cc_state->cct.entries, entries,
	       eentry * sizeof(struct ib_cc_table_entry));

	new_cc_state->cong_setting.port_control = HFI_IB_CC_CCS_PC_SL_BASED;
	new_cc_state->cong_setting.control_map = ppd->cc_sl_control_map;
	memcpy(new_cc_state->cong_setting.entries, ppd->congestion_entries,
	       OPA_MAX_SLS * sizeof(struct opa_congestion_setting_entry));

	rcu_assign_pointer(ppd->cc_state, new_cc_state);

	spin_unlock(&ppd->cc_state_lock);

	call_rcu(&old_cc_state->rcu, hfi_cc_state_reclaim);

done:
	return __subn_get_opa_cc_table(smp, am, data, ibdev, port, resp_len);
}

static int __subn_set_opa_led_info(struct opa_smp *smp, u32 am, u8 *data,
				struct ib_device *ibdev, u8 port,
					      u32 *resp_len)
{
	/* FXRTODO: to be implemented */
	return IB_MAD_RESULT_FAILURE;
}

static int subn_set_opa_sma(u16 attr_id, struct opa_smp *smp, u32 am,
			    u8 *data, struct ib_device *ibdev, u8 port,
			    u32 *resp_len)
{
	int ret;
	struct hfi2_ibport *ibp = to_hfi_ibp(ibdev, port);

	switch (attr_id) {
	case IB_SMP_ATTR_PORT_INFO:
		ret = __subn_set_opa_portinfo(smp, am, data, ibdev, port,
					      resp_len);
		break;
	case IB_SMP_ATTR_PKEY_TABLE:
		ret = __subn_set_opa_pkeytable(smp, am, data, ibdev, port,
					       resp_len);
		break;
	case OPA_ATTRIB_ID_SL_TO_SC_MAP:
		ret = __subn_set_opa_sl_to_sc(smp, am, data, ibdev, port,
					      resp_len);
		break;
	case OPA_ATTRIB_ID_SC_TO_SL_MAP:
		ret = __subn_set_opa_sc_to_sl(smp, am, data, ibdev, port,
					      resp_len);
		break;
	case OPA_ATTRIB_ID_SC_TO_VLR_MAP:
		ret = __subn_set_opa_sc_to_vlr(smp, am, data, ibdev, port,
					       resp_len);
		break;
	case OPA_ATTRIB_ID_SC_TO_VLT_MAP:
		ret = __subn_set_opa_sc_to_vlt(smp, am, data, ibdev, port,
					       resp_len);
		break;
	case OPA_ATTRIB_ID_SC_TO_VLNT_MAP:
		ret = __subn_set_opa_sc_to_vlnt(smp, am, data, ibdev, port,
					       resp_len);
		break;
	case OPA_ATTRIB_ID_PORT_STATE_INFO:
		ret = __subn_set_opa_psi(smp, am, data, ibdev, port,
					 resp_len);
		break;
	case OPA_ATTRIB_ID_BUFFER_CONTROL_TABLE:
		ret = __subn_set_opa_bct(smp, am, data, ibdev, port,
					 resp_len);
		break;
	case OPA_ATTRIB_ID_BW_ARBITRATION:
		ret = __subn_set_opa_bw_arb(smp, am, data, ibdev, port,
					    resp_len);
		break;
	case OPA_ATTRIB_ID_HFI_CONGESTION_SETTING:
		ret = __subn_set_opa_cong_setting(smp, am, data, ibdev,
						  port, resp_len);
		break;
	case OPA_ATTRIB_ID_CONGESTION_CONTROL_TABLE:
		ret = __subn_set_opa_cc_table(smp, am, data, ibdev, port,
					      resp_len);
		break;
	case IB_SMP_ATTR_LED_INFO:
		ret = __subn_set_opa_led_info(smp, am, data, ibdev, port,
					      resp_len);
		break;
	case IB_SMP_ATTR_SM_INFO:
		if (ibp->port_cap_flags & IB_PORT_SM_DISABLED)
			return IB_MAD_RESULT_SUCCESS | IB_MAD_RESULT_CONSUMED;
		if (ibp->port_cap_flags & IB_PORT_SM)
			return IB_MAD_RESULT_SUCCESS;
		/* FALLTHROUGH */
	default:
		smp->status |=
		cpu_to_be16(IB_MGMT_MAD_STATUS_UNSUPPORTED_METHOD_ATTRIB);
		ret = reply((struct ib_mad_hdr *)smp);
		break;
	}

	return ret;
}

static int subn_opa_aggregate(struct opa_smp *smp,
				  struct ib_device *ibdev, u8 port,
				  u32 *resp_len)
{
	int i;
	u32 num_attr = OPA_AM_NATTR(be32_to_cpu(smp->attr_mod));
	u8 *next_smp = opa_get_smp_data(smp);
	u8 method = smp->method;

	if (num_attr < OPA_MIN_AGGR_ATTR || num_attr > OPA_MAX_AGGR_ATTR) {
		smp->status |=
			cpu_to_be16(IB_MGMT_MAD_STATUS_INVALID_ATTRIB_VALUE);
		return reply((struct ib_mad_hdr *)smp);
	}

	for (i = 0; i < num_attr; i++) {
		struct opa_aggregate *agg;
		size_t agg_data_len;
		size_t agg_size;
		u32 am;

		agg = (struct opa_aggregate *)next_smp;
		agg_data_len =
			OPA_AGGR_REQ_LEN(be16_to_cpu(agg->err_reqlength));
		agg_data_len *= OPA_AGGR_REQ_BYTES_PER_UNIT;
		agg_size = sizeof(*agg) + agg_data_len;
		am = be32_to_cpu(agg->attr_mod);

		*resp_len += agg_size;

		if (next_smp + agg_size > ((u8 *)smp) + sizeof(*smp)) {
			smp->status |=
			cpu_to_be16(IB_MGMT_MAD_STATUS_INVALID_ATTRIB_VALUE);
			return reply((struct ib_mad_hdr *)smp);
		}

		switch (method) {
		case IB_MGMT_METHOD_GET:
			/* zero the payload for this segment */
			memset(next_smp + sizeof(*agg), 0, agg_data_len);
			subn_get_opa_sma(agg->attr_id, smp, am, agg->data,
					ibdev, port, NULL);
			break;
		case IB_MGMT_METHOD_SET:
			subn_set_opa_sma(agg->attr_id, smp, am, agg->data,
					ibdev, port, NULL);
			break;
		default:
			smp->status |=
			cpu_to_be16(IB_MGMT_MAD_STATUS_UNSUPPORTED_METHOD);
			return reply((struct ib_mad_hdr *)smp);
		}

		if (smp->status & ~IB_SMP_DIRECTION) {
			agg->err_reqlength |= OPA_AGGR_ERROR;
			return reply((struct ib_mad_hdr *)smp);
		}
		next_smp += agg_size;
	}

	return reply((struct ib_mad_hdr *)smp);
}

/*
 * hfi_is_local_mad() returns 1 if 'mad' is sent from, and destined to the
 * local node, 0 otherwise.
 */
static int hfi_is_local_mad(struct hfi2_ibport *ibp, const struct opa_mad *mad,
			    const struct ib_wc *in_wc,
			    const struct ib_grh *in_grh)

{
	const struct opa_smp *smp = (const struct opa_smp *)mad;
	union ib_gid gid;

	if (smp->mgmt_class == IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE) {
		return (smp->hop_cnt == 0 &&
			smp->route.dr.dr_slid == OPA_LID_PERMISSIVE &&
			smp->route.dr.dr_dlid == OPA_LID_PERMISSIVE);
	}

	if (in_grh) {
		gid = in_grh->sgid;
		if (ib_is_opa_gid(&gid))
			return (opa_get_lid_from_gid(&gid) == ibp->ppd->lid);
	}
	return (in_wc->slid == ibp->ppd->lid);
}

/*
 * opa_local_smp_check() should only be called on MADs for which
 * hfi_is_local_mad() returns true. It applies the SMP checks that are
 * specific to SMPs which are sent from, and destined to this node.
 * opa_local_smp_check() returns 0 if the SMP passes its checks, 1
 * otherwise.
 *
 * SMPs which arrive from other nodes are instead checked by
 * opa_smp_check().
 */
static int opa_local_smp_check(struct hfi2_ibport *ibp,
			       const struct ib_wc *in_wc)
{
	u16 pkey;

	if (in_wc->pkey_index >= HFI_MAX_PKEYS)
		return 1;

	pkey = ibp->ppd->pkeys[in_wc->pkey_index];
	/*
	 * We need to do the "node-local" checks specified in OPAv1,
	 * rev 0.90, section 9.10.26, which are:
	 *   - pkey is 0x7fff, or 0xffff
	 *   - Source QPN == 0 || Destination QPN == 0
	 *   - the MAD header's management class is either
	 *     IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE or
	 *     IB_MGMT_CLASS_SUBN_LID_ROUTED
	 *   - SLID != 0
	 *
	 * However, we know (and so don't need to check again) that,
	 * for local SMPs, the MAD stack passes MADs with:
	 *   - Source QPN of 0
	 *   - MAD mgmt_class is IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE
	 *   - SLID is either: OPA_LID_PERMISSIVE (0xFFFFFFFF), or
	 *     our own port's lid
	 *
	 */
	if (pkey == OPA_LIM_MGMT_PKEY || pkey == OPA_FULL_MGMT_PKEY)
		return 0;
	/* FXRTODO: Implement as part of PMA, Refer STL-3620 */
#if 0
	ingress_pkey_table_fail(ppd, pkey, slid);
#endif
	return 1;
}

static int process_subn_opa(struct ib_device *ibdev, int mad_flags,
			    u8 port, const struct opa_mad *in_mad,
			    struct opa_mad *out_mad,
			    u32 *resp_len)
{
	struct opa_smp *smp = (struct opa_smp *)out_mad;
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	struct hfi2_ibport *ibp = to_hfi_ibp(ibdev, port);
	u8 *data;
	u32 am;
	__be16 attr_id;
	int ret = IB_MAD_RESULT_FAILURE;

	*out_mad = *in_mad;
	data = opa_get_smp_data(smp);

	am = be32_to_cpu(smp->attr_mod);
	attr_id = smp->attr_id;
	if (smp->class_version != OPA_SMI_CLASS_VERSION) {
		smp->status |= cpu_to_be16(IB_MGMT_MAD_STATUS_BAD_VERSION);
		ret = reply(ibh);
		goto bail;
	}
	ret = check_mkey(ibp, (struct ib_mad_hdr *)smp, mad_flags, smp->mkey,
			 smp->route.dr.dr_slid, smp->route.dr.return_path,
			 smp->hop_cnt);
	if (ret) {
		ret = IB_MAD_RESULT_FAILURE;
		goto bail;
	}

	*resp_len = opa_get_smp_header_size(smp);

	switch (smp->method) {
	case IB_MGMT_METHOD_GET:
		switch (attr_id) {
		default:
			clear_opa_smp_data(smp);
			ret = subn_get_opa_sma(attr_id, smp, am, data,
					       ibdev, port, resp_len);
			goto bail;
		case OPA_ATTRIB_ID_AGGREGATE:
			ret = subn_opa_aggregate(smp, ibdev, port,
						     resp_len);
			goto bail;
		}
	case IB_MGMT_METHOD_SET:

		switch (attr_id) {
		default:
			ret = subn_set_opa_sma(attr_id, smp, am, data,
					       ibdev, port, resp_len);
			goto bail;
		case OPA_ATTRIB_ID_AGGREGATE:
			ret = subn_opa_aggregate(smp, ibdev, port,
						     resp_len);
			goto bail;
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
	default:
		smp->status |=
			cpu_to_be16(IB_MGMT_MAD_STATUS_UNSUPPORTED_METHOD);
		ret = reply(ibh);
	}

bail:
	return ret;
}

static int pma_get_opa_classportinfo(struct opa_pma_mad *pmp,
				     struct ib_device *ibdev, u32 *resp_len)
{
	struct opa_class_port_info *p =
		(struct opa_class_port_info *)pmp->data;
	memset(pmp->data, 0, sizeof(pmp->data));
	if (pmp->mad_hdr.attr_mod != 0)
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
	p->base_version = OPA_MGMT_BASE_VERSION;
	p->class_version = OPA_SMI_CLASS_VERSION;
	/*
	 * Expected response time is 4.096 usec. * 2^18 == 1.073741824 sec.
	 */
	p->cap_mask2_resp_time = cpu_to_be32(18);
	if (resp_len)
		*resp_len += sizeof(*p);
	return reply((struct ib_mad_hdr *)pmp);

}

static int pma_get_opa_portstatus(struct opa_pma_mad *pmp,
				  struct ib_device *ibdev,
				  u8 port, u32 *resp_len)
{
	struct hfi_devdata *dd;
	struct hfi_pportdata *ppd;
	struct opa_port_status_req *req =
		(struct opa_port_status_req *)pmp->data;
	struct opa_port_status_rsp *rsp;
	u32 vl_select_mask = be32_to_cpu(req->vl_select_mask);
	size_t response_data_size;
	unsigned long vl;
	u32 nports = be32_to_cpu(pmp->mad_hdr.attr_mod) >> 24;
	u8 port_num = req->port_num;
	u8 num_vls = hweight32(vl_select_mask);
	struct _vls_pctrs *vlinfo;
	int vfi;
	u64 tmp, tmp2;

	dd = hfi_dd_from_ibdev(ibdev);
	ppd = to_hfi_ppd(dd, port);
	response_data_size = sizeof(struct opa_port_status_rsp) +
				num_vls * sizeof(struct _vls_pctrs);
	if (response_data_size > sizeof(pmp->data)) {
		pmp->mad_hdr.status |= OPA_PM_STATUS_REQUEST_TOO_LARGE;
		return reply((struct ib_mad_hdr *)pmp);
	}

	if (nports != 1 || (port_num && port_num != port) ||
	    num_vls > OPA_MAX_VLS || (vl_select_mask & ~VL_MASK_ALL)) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		return reply((struct ib_mad_hdr *)pmp);
	}

	memset(pmp->data, 0, sizeof(pmp->data));

	rsp = (struct opa_port_status_rsp *)pmp->data;
	if (port_num)
		rsp->port_num = port_num;
	else
		rsp->port_num = port;

	rsp->vl_select_mask = cpu_to_be32(vl_select_mask);
	rsp->port_xmit_data = cpu_to_be64(hfi_read_lm_tp_prf_csr(ppd,
							TP_XMIT_DATA));
	rsp->port_rcv_data = cpu_to_be64(hfi_read_lm_fpc_csr(ppd,
						FXR_FPC_PRF_PORTRCV_DATA_CNT));
	rsp->port_xmit_pkts = cpu_to_be64(hfi_read_lm_tp_prf_csr(ppd,
								TP_XMIT_PKTS));
	rsp->port_rcv_pkts = cpu_to_be64(hfi_read_lm_fpc_csr(ppd,
						FXR_FPC_PRF_PORTRCV_PKT_CNT));
	rsp->port_multicast_rcv_pkts = cpu_to_be64(hfi_read_lm_fpc_csr(ppd,
						FXR_FPC_PRF_PORTRCV_MCAST_PKT_CNT));
	rsp->port_multicast_xmit_pkts = cpu_to_be64(hfi_read_lm_tp_prf_csr(
						ppd, TP_MULTICAST_XMIT_PKTS));
	rsp->port_xmit_wait = cpu_to_be64(hfi_read_lm_tp_prf_csr(ppd,
								TP_XMIT_WAIT));
	rsp->port_rcv_fecn = cpu_to_be64(hfi_read_lm_fpc_csr(ppd,
							FXR_FPC_PRF_PORTRCV_FECN));
	rsp->port_rcv_becn = cpu_to_be64(hfi_read_lm_fpc_csr(ppd,
							FXR_FPC_PRF_PORTRCV_BECN));
	rsp->port_xmit_discards = 0;
	rsp->port_xmit_constraint_errors = 0;
	rsp->port_rcv_constraint_errors = cpu_to_be64(hfi_read_lm_fpc_csr(ppd,
					FXR_FPC_ERR_PORTRCV_CONSTRAINT_ERROR));
	rsp->port_rcv_remote_physical_errors = cpu_to_be64(hfi_read_lm_fpc_csr(
				ppd, FXR_FPC_ERR_PORTRCV_PHY_REMOTE_ERROR));
	hfi_read_link_quality(ppd, &rsp->link_quality_indicator);

	tmp = read_fzc_csr(ppd, FZC_LCB_ERR_INFO_RX_REPLAY_CNT);
	tmp2 = tmp + read_fzc_csr(ppd, FZC_LCB_ERR_INFO_TX_REPLAY_CNT);
	if (tmp2 < tmp)
		rsp->local_link_integrity_errors = cpu_to_be64(~0);
	else
		rsp->local_link_integrity_errors = cpu_to_be64(tmp2);

	tmp = read_fzc_csr(ppd, FZC_LCB_ERR_INFO_SEQ_CRC_CNT);
	tmp2 = tmp + read_fzc_csr(ppd, FZC_LCB_ERR_INFO_REINIT_FROM_PEER_CNT);
	if (tmp2 < tmp)
		rsp->link_error_recovery = cpu_to_be32(~0);
	else
		rsp->link_error_recovery = cpu_to_be64(tmp2);

	rsp->port_rcv_errors = cpu_to_be64(hfi_read_lm_fpc_csr(ppd,
						FXR_FPC_ERR_PORTRCV_ERROR));
	rsp->excessive_buffer_overruns = 0;
	rsp->fm_config_errors = cpu_to_be64(hfi_read_lm_fpc_csr(ppd,
						FXR_FPC_ERR_FMCONFIG_ERROR));
	rsp->link_downed = cpu_to_be32(ppd->link_downed);
	rsp->uncorrectable_errors = cpu_to_be64(hfi_read_lm_fpc_csr(ppd,
						FXR_FPC_ERR_UNCORRECTABLE_ERROR));

	vlinfo = &rsp->vls[0];
	vfi = 0;
	/*
	 * The vl_select_mask has been checked above, and we know
	 * that it contains only entries which represent valid VLs.
	 * So in the for_each_set_bit() loop below, we don't need
	 * any additional checks for vl.
	 */
	for_each_set_bit(vl, (unsigned long *)&vl_select_mask,
			 8 * sizeof(vl_select_mask)) {
		memset(vlinfo, 0, sizeof(*vlinfo));
		rsp->vls[vfi].port_vl_rcv_data = cpu_to_be64(
					hfi_read_lm_fpc_prf_per_vl_csr(ppd,
					FXR_FPC_PRF_PORT_VL_RCV_DATA_CNT,
					idx_from_vl(vl)));
		rsp->vls[vfi].port_vl_rcv_pkts = cpu_to_be64(
					hfi_read_lm_fpc_prf_per_vl_csr(ppd,
					FXR_FPC_PRF_PORT_VL_RCV_PKT_CNT,
					idx_from_vl(vl)));
		rsp->vls[vfi].port_vl_xmit_data = cpu_to_be64(
					hfi_read_lm_tp_prf_csr(ppd,
						(TP_XMIT_DATA +
						idx_from_vl(vl) + 1)));
		rsp->vls[vfi].port_vl_xmit_pkts = cpu_to_be64(
						hfi_read_lm_tp_prf_csr(ppd,
						(TP_XMIT_PKTS +
						idx_from_vl(vl) + 1)));
		rsp->vls[vfi].port_vl_xmit_wait = cpu_to_be64(
						hfi_read_lm_tp_prf_csr(ppd,
						(TP_XMIT_WAIT +
						idx_from_vl(vl) + 1)));
		rsp->vls[vfi].port_vl_rcv_fecn = cpu_to_be64(
					hfi_read_lm_fpc_prf_per_vl_csr(ppd,
						FXR_FPC_PRF_PORT_VL_RCV_FECN,
						idx_from_vl(vl)));
		rsp->vls[vfi].port_vl_rcv_becn = cpu_to_be64(
					hfi_read_lm_fpc_prf_per_vl_csr(ppd,
						FXR_FPC_PRF_PORT_VL_RCV_BECN,
						idx_from_vl(vl)));
		vlinfo++;
		vfi++;
	}

	if (resp_len)
		*resp_len += response_data_size;

	return reply((struct ib_mad_hdr *)pmp);
}

static u64 get_error_counter_summary(struct ib_device *ibdev, u8 port,
				     u8 res_lli, u8 res_ler)
{
	struct hfi_devdata *dd;
	struct hfi_pportdata *ppd;
	u64 error_counter_summary = 0, tmp;

	dd = hfi_dd_from_ibdev(ibdev);
	ppd = to_hfi_ppd(dd, port);

	error_counter_summary += hfi_read_lm_fpc_csr(ppd,
					FXR_FPC_ERR_PORTRCV_CONSTRAINT_ERROR);
	error_counter_summary += hfi_read_lm_fpc_csr(ppd,
					FXR_FPC_ERR_PORTRCV_PHY_REMOTE_ERROR);
	tmp = read_fzc_csr(ppd, FZC_LCB_ERR_INFO_RX_REPLAY_CNT);
	tmp += read_fzc_csr(ppd, FZC_LCB_ERR_INFO_TX_REPLAY_CNT);
	error_counter_summary += (tmp >> res_lli);

	tmp = read_fzc_csr(ppd, FZC_LCB_ERR_INFO_SEQ_CRC_CNT);
	tmp += read_fzc_csr(ppd, FZC_LCB_ERR_INFO_REINIT_FROM_PEER_CNT);
	error_counter_summary += (tmp >> res_ler);

	error_counter_summary += hfi_read_lm_fpc_csr(ppd,
						FXR_FPC_ERR_PORTRCV_ERROR);
	/*rsp->excessive_buffer_overruns = 0;*/
	error_counter_summary += hfi_read_lm_fpc_csr(ppd,
						FXR_FPC_ERR_FMCONFIG_ERROR);
	error_counter_summary += ppd->link_downed;
	error_counter_summary += hfi_read_lm_fpc_csr(ppd,
						FXR_FPC_ERR_UNCORRECTABLE_ERROR);
	return error_counter_summary;
}

static int pma_get_opa_datacounters(struct opa_pma_mad *pmp,
				    struct ib_device *ibdev, u8 port,
				    u32 *resp_len)
{
	struct opa_port_data_counters_msg *req =
		(struct opa_port_data_counters_msg *)pmp->data;
	struct hfi_devdata *dd;
	struct hfi_pportdata *ppd;
	struct _port_dctrs *rsp;
	struct _vls_dctrs *vlinfo;
	size_t response_data_size;
	u32 num_ports;
	u8 num_pslm;
	u8 lq, num_vls;
	u8 res_lli, res_ler;
	u64 port_mask;
	unsigned long port_num;
	unsigned long vl;
	u32 vl_select_mask;
	int vfi;

	dd = hfi_dd_from_ibdev(ibdev);
	ppd = to_hfi_ppd(dd, port);
	num_ports = be32_to_cpu(pmp->mad_hdr.attr_mod) >> 24;
	num_pslm = hweight64(be64_to_cpu(req->port_select_mask[3]));
	num_vls = hweight32(be32_to_cpu(req->vl_select_mask));
	vl_select_mask = be32_to_cpu(req->vl_select_mask);
	res_lli = (u8)(be32_to_cpu(req->resolution) & MSK_LLI) >> MSK_LLI_SFT;
	res_lli = res_lli ? res_lli + ADD_LLI : 0;
	res_ler = (u8)(be32_to_cpu(req->resolution) & MSK_LER) >> MSK_LER_SFT;
	res_ler = res_ler ? res_ler + ADD_LER : 0;

	if (num_ports != 1 || (vl_select_mask & ~VL_MASK_ALL)) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		return reply((struct ib_mad_hdr *)pmp);
	}

	/* Sanity check */
	response_data_size = sizeof(struct opa_port_data_counters_msg) +
				num_vls * sizeof(struct _vls_dctrs);

	if (response_data_size > sizeof(pmp->data)) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		return reply((struct ib_mad_hdr *)pmp);
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
		return reply((struct ib_mad_hdr *)pmp);
	}

	rsp = (struct _port_dctrs *)&req->port[0];
	memset(rsp, 0, sizeof(*rsp));
	/* FXRTODO: replace fake values(zeros) with actual register reads */
	rsp->port_number = port;
	rsp->port_xmit_data = cpu_to_be64(hfi_read_lm_tp_prf_csr(ppd,
							TP_XMIT_DATA));
	rsp->port_rcv_data = cpu_to_be64(hfi_read_lm_fpc_csr(ppd,
						FXR_FPC_PRF_PORTRCV_DATA_CNT));
	rsp->port_xmit_pkts = cpu_to_be64(hfi_read_lm_tp_prf_csr(ppd,
								TP_XMIT_PKTS));
	rsp->port_rcv_pkts = cpu_to_be64(hfi_read_lm_fpc_csr(ppd,
						FXR_FPC_PRF_PORTRCV_PKT_CNT));
	rsp->port_multicast_rcv_pkts = cpu_to_be64(hfi_read_lm_fpc_csr(ppd,
						FXR_FPC_PRF_PORTRCV_MCAST_PKT_CNT));
	rsp->port_multicast_xmit_pkts = cpu_to_be64(hfi_read_lm_tp_prf_csr(
						ppd, TP_MULTICAST_XMIT_PKTS));
	rsp->port_xmit_wait = cpu_to_be64(hfi_read_lm_tp_prf_csr(ppd,
								TP_XMIT_WAIT));
	rsp->port_rcv_fecn = cpu_to_be64(hfi_read_lm_fpc_csr(ppd,
							FXR_FPC_PRF_PORTRCV_FECN));
	rsp->port_rcv_becn = cpu_to_be64(hfi_read_lm_fpc_csr(ppd,
							FXR_FPC_PRF_PORTRCV_BECN));
	hfi_read_link_quality(ppd, &lq);
	rsp->link_quality_indicator = cpu_to_be32((u32)lq);

	rsp->port_error_counter_summary =
		cpu_to_be64(get_error_counter_summary(ibdev, port,
						      res_lli, res_ler));

	vlinfo = &rsp->vls[0];
	vfi = 0;
	/*
	 * The vl_select_mask has been checked above, and we know
	 * that it contains only entries which represent valid VLs.
	 * So in the for_each_set_bit() loop below, we don't need
	 * any additional checks for vl.
	 */
	for_each_set_bit(vl, (unsigned long *)&vl_select_mask,
			 8 * sizeof(vl_select_mask)) {
		memset(vlinfo, 0, sizeof(*vlinfo));
		rsp->vls[vfi].port_vl_rcv_data = cpu_to_be64(
					hfi_read_lm_fpc_prf_per_vl_csr(ppd,
					FXR_FPC_PRF_PORT_VL_RCV_DATA_CNT,
					idx_from_vl(vl)));
		rsp->vls[vfi].port_vl_rcv_pkts = cpu_to_be64(
					hfi_read_lm_fpc_prf_per_vl_csr(ppd,
					FXR_FPC_PRF_PORT_VL_RCV_PKT_CNT,
					idx_from_vl(vl)));
		rsp->vls[vfi].port_vl_xmit_data = cpu_to_be64(
						hfi_read_lm_tp_prf_csr(ppd,
						(TP_XMIT_DATA +
						idx_from_vl(vl) + 1)));
		rsp->vls[vfi].port_vl_xmit_pkts = cpu_to_be64(
						hfi_read_lm_tp_prf_csr(ppd,
						(TP_XMIT_PKTS +
						idx_from_vl(vl) + 1)));
		rsp->vls[vfi].port_vl_xmit_wait = cpu_to_be64(
						hfi_read_lm_tp_prf_csr(ppd,
						(TP_XMIT_WAIT +
						idx_from_vl(vl) + 1)));
		rsp->vls[vfi].port_vl_rcv_fecn = cpu_to_be64(
					hfi_read_lm_fpc_prf_per_vl_csr(ppd,
						FXR_FPC_PRF_PORT_VL_RCV_FECN,
						idx_from_vl(vl)));
		rsp->vls[vfi].port_vl_rcv_becn = cpu_to_be64(
					hfi_read_lm_fpc_prf_per_vl_csr(ppd,
						FXR_FPC_PRF_PORT_VL_RCV_BECN,
						idx_from_vl(vl)));
		vlinfo++;
		vfi++;
	}

	if (resp_len)
		*resp_len += response_data_size;

	return reply((struct ib_mad_hdr *)pmp);
}

static int pma_get_opa_porterrors(struct opa_pma_mad *pmp,
				  struct ib_device *ibdev, u8 port,
				  u32 *resp_len)
{
	struct hfi_devdata *dd;
	struct hfi_pportdata *ppd;
	size_t response_data_size;
	struct _port_ectrs *rsp;
	unsigned long port_num;
	struct opa_port_error_counters64_msg *req;
	u32 num_ports;
	u8 num_pslm;
	u8 num_vls;
	struct _vls_ectrs *vlinfo;
	unsigned long vl;
	u64 port_mask, tmp, tmp2;
	u32 vl_select_mask;
	int vfi;

	dd = hfi_dd_from_ibdev(ibdev);
	ppd = to_hfi_ppd(dd, port);
	req = (struct opa_port_error_counters64_msg *)pmp->data;

	num_ports = be32_to_cpu(pmp->mad_hdr.attr_mod) >> 24;

	num_pslm = hweight64(be64_to_cpu(req->port_select_mask[3]));
	num_vls = hweight32(be32_to_cpu(req->vl_select_mask));

	if (num_ports != 1 || num_ports != num_pslm) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		return reply((struct ib_mad_hdr *)pmp);
	}

	response_data_size = sizeof(struct opa_port_error_counters64_msg) +
				num_vls * sizeof(struct _vls_ectrs);

	if (response_data_size > sizeof(pmp->data)) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		return reply((struct ib_mad_hdr *)pmp);
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
		return reply((struct ib_mad_hdr *)pmp);
	}

	rsp = (struct _port_ectrs *)&req->port[0];

	memset(rsp, 0, sizeof(*rsp));
	rsp->port_number = (u8)port_num;
	/* FXRTODO: replace fake values(zeros) with actual register reads */
	rsp->port_xmit_discards = 0;
	rsp->port_xmit_constraint_errors = 0;
	rsp->port_rcv_constraint_errors = cpu_to_be64(hfi_read_lm_fpc_csr(ppd,
					FXR_FPC_ERR_PORTRCV_CONSTRAINT_ERROR));
	rsp->port_rcv_remote_physical_errors = cpu_to_be64(hfi_read_lm_fpc_csr(
				ppd, FXR_FPC_ERR_PORTRCV_PHY_REMOTE_ERROR));
	tmp = read_fzc_csr(ppd, FZC_LCB_ERR_INFO_RX_REPLAY_CNT);
	tmp2 = tmp + read_fzc_csr(ppd, FZC_LCB_ERR_INFO_TX_REPLAY_CNT);
	if (tmp2 < tmp)
		rsp->local_link_integrity_errors = cpu_to_be64(~0);
	else
		rsp->local_link_integrity_errors = cpu_to_be64(tmp2);

	tmp = read_fzc_csr(ppd, FZC_LCB_ERR_INFO_SEQ_CRC_CNT);
	tmp2 = tmp + read_fzc_csr(ppd, FZC_LCB_ERR_INFO_REINIT_FROM_PEER_CNT);
	if (tmp2 < tmp)
		rsp->link_error_recovery = cpu_to_be32(~0);
	else
		rsp->link_error_recovery = cpu_to_be64(tmp2);

	rsp->port_rcv_errors = cpu_to_be64(hfi_read_lm_fpc_csr(ppd,
						FXR_FPC_ERR_PORTRCV_ERROR));
	rsp->excessive_buffer_overruns = 0;
	rsp->fm_config_errors = cpu_to_be64(hfi_read_lm_fpc_csr(ppd,
						FXR_FPC_ERR_FMCONFIG_ERROR));
	rsp->link_downed = cpu_to_be32(ppd->link_downed);
	rsp->uncorrectable_errors = cpu_to_be64(hfi_read_lm_fpc_csr(ppd,
						FXR_FPC_ERR_UNCORRECTABLE_ERROR));

	vlinfo = (struct _vls_ectrs *)&rsp->vls[0];
	vfi = 0;
	vl_select_mask = be32_to_cpu(req->vl_select_mask);
	for_each_set_bit(vl, (unsigned long *)&(vl_select_mask),
			 8 * sizeof(req->vl_select_mask)) {
		memset(vlinfo, 0, sizeof(*vlinfo));
		/* vlinfo->vls[vfi].port_vl_xmit_discards ??? */
		vlinfo += 1;
		vfi++;
	}

	if (resp_len)
		*resp_len += response_data_size;

	return reply((struct ib_mad_hdr *)pmp);
}

static int pma_get_opa_errorinfo(struct opa_pma_mad *pmp,
				 struct ib_device *ibdev, u8 port,
				 u32 *resp_len)
{	size_t response_data_size;
	struct _port_ei *rsp;
	struct opa_port_error_info_msg *req;
	struct hfi_devdata *dd = hfi_dd_from_ibdev(ibdev);
	u64 port_mask;
	u32 num_ports;
	unsigned long port_num;
	u8 num_pslm;

	req = (struct opa_port_error_info_msg *)pmp->data;
	rsp = (struct _port_ei *)&req->port[0];

	num_ports = OPA_AM_NPORT(be32_to_cpu(pmp->mad_hdr.attr_mod));
	num_pslm = hweight64(be64_to_cpu(req->port_select_mask[3]));

	memset(rsp, 0, sizeof(*rsp));

	if (num_ports != 1 || num_ports != num_pslm) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		return reply((struct ib_mad_hdr *)pmp);
	}

	/* Sanity check */
	response_data_size = sizeof(struct opa_port_error_info_msg);

	if (response_data_size > sizeof(pmp->data)) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		return reply((struct ib_mad_hdr *)pmp);
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
		return reply((struct ib_mad_hdr *)pmp);
	}

	/*
	 * FXRTODO: err_info fields needs to be filled with values
	 * in FZC error handler
	 */
	rsp->port_rcv_ei.status_and_code =
		dd->err_info_rcvport.status_and_code;
	memcpy(&rsp->port_rcv_ei.ei.ei1to12.packet_flit1,
	       &dd->err_info_rcvport.packet_flit1, sizeof(u64));
	memcpy(&rsp->port_rcv_ei.ei.ei1to12.packet_flit2,
	       &dd->err_info_rcvport.packet_flit2, sizeof(u64));

	/* ExcessiverBufferOverrunInfo */
	rsp->excessive_buffer_overrun_ei.status_and_sc |= 0x80;

	rsp->port_xmit_constraint_ei.status =
		dd->err_info_xmit_constraint.status;
	rsp->port_xmit_constraint_ei.pkey =
		cpu_to_be16(dd->err_info_xmit_constraint.pkey);
	rsp->port_xmit_constraint_ei.slid =
		cpu_to_be32(dd->err_info_xmit_constraint.slid);

	rsp->port_rcv_constraint_ei.status =
		dd->err_info_rcv_constraint.status;
	rsp->port_rcv_constraint_ei.pkey =
		cpu_to_be16(dd->err_info_rcv_constraint.pkey);
	rsp->port_rcv_constraint_ei.slid =
		cpu_to_be32(dd->err_info_rcv_constraint.slid);

	/* UncorrectableErrorInfo */
	rsp->uncorrectable_ei.status_and_code = dd->err_info_uncorrectable;

	/* FMConfigErrorInfo */
	rsp->fm_config_ei.status_and_code = dd->err_info_fmconfig;

	if (resp_len)
		*resp_len += response_data_size;

	return reply((struct ib_mad_hdr *)pmp);

}

static int pma_set_opa_portstatus(struct opa_pma_mad *pmp,
				  struct ib_device *ibdev, u8 port,
				  u32 *resp_len)
{
	struct hfi_devdata *dd;
	struct hfi_pportdata *ppd;
	struct opa_clear_port_status *req =
		(struct opa_clear_port_status *)pmp->data;
	u32 nports = be32_to_cpu(pmp->mad_hdr.attr_mod) >> 24;
	u64 portn = be64_to_cpu(req->port_select_mask[3]);
	u32 counter_select = be32_to_cpu(req->counter_select_mask);
	u32 vl_select_mask = VL_MASK_ALL;
	unsigned long vl;

	dd = hfi_dd_from_ibdev(ibdev);
	ppd = to_hfi_ppd(dd, port);

	if ((nports != 1) || (portn != 1 << port)) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		return reply((struct ib_mad_hdr *)pmp);
	}

	if (counter_select & CS_PORT_XMIT_DATA)
		hfi_write_lm_tp_prf_csr(ppd, TP_XMIT_DATA, 0);

	if (counter_select & CS_PORT_RCV_DATA)
		hfi_write_lm_fpc_csr(ppd, FXR_FPC_PRF_PORTRCV_DATA_CNT, 0);

	if (counter_select & CS_PORT_XMIT_PKTS)
		hfi_write_lm_tp_prf_csr(ppd, TP_XMIT_PKTS, 0);

	if (counter_select & CS_PORT_RCV_PKTS)
		hfi_write_lm_fpc_csr(ppd, FXR_FPC_PRF_PORTRCV_PKT_CNT, 0);

	if (counter_select & CS_PORT_MCAST_XMIT_PKTS)
		hfi_write_lm_tp_prf_csr(ppd, TP_MULTICAST_XMIT_PKTS, 0);

	if (counter_select & CS_PORT_MCAST_RCV_PKTS)
		hfi_write_lm_fpc_csr(ppd, FXR_FPC_PRF_PORTRCV_MCAST_PKT_CNT, 0);

	if (counter_select & CS_PORT_XMIT_WAIT)
		hfi_write_lm_tp_prf_csr(ppd, TP_XMIT_WAIT, 0);

	if (counter_select & CS_PORT_RCV_FECN)
		hfi_write_lm_fpc_csr(ppd, FXR_FPC_PRF_PORTRCV_FECN, 0);

	if (counter_select & CS_PORT_RCV_BECN)
		hfi_write_lm_fpc_csr(ppd, FXR_FPC_PRF_PORTRCV_BECN, 0);

	if (counter_select & CS_PORT_RCV_BUBBLE)
		hfi_write_lm_fpc_csr(ppd, FXR_FPC_PRF_PORTRCV_BUBBLE, 0);

	if (counter_select & CS_PORT_RCV_CONSTRAINT_ERRORS)
		hfi_write_lm_fpc_csr(ppd, FXR_FPC_ERR_PORTRCV_CONSTRAINT_ERROR, 0);

	if (counter_select & CS_PORT_RCV_REMOTE_PHYSICAL_ERRORS)
		hfi_write_lm_fpc_csr(ppd, FXR_FPC_ERR_PORTRCV_PHY_REMOTE_ERROR, 0);

	if (counter_select & CS_LOCAL_LINK_INTEGRITY_ERRORS) {
		write_fzc_csr(ppd, FZC_LCB_ERR_INFO_TX_REPLAY_CNT, 0);
		write_fzc_csr(ppd, FZC_LCB_ERR_INFO_RX_REPLAY_CNT, 0);
	}

	if (counter_select & CS_LINK_ERROR_RECOVERY) {
		write_fzc_csr(ppd, FZC_LCB_ERR_INFO_SEQ_CRC_CNT, 0);
		write_fzc_csr(ppd, FZC_LCB_ERR_INFO_REINIT_FROM_PEER_CNT, 0);
	}

	if (counter_select & CS_PORT_RCV_ERRORS)
		hfi_write_lm_fpc_csr(ppd, FXR_FPC_ERR_PORTRCV_ERROR, 0);

	if (counter_select & CS_EXCESSIVE_BUFFER_OVERRUNS)

	if (counter_select & CS_FM_CONFIG_ERRORS)
		hfi_write_lm_fpc_csr(ppd, FXR_FPC_ERR_FMCONFIG_ERROR, 0);
	if (counter_select & CS_LINK_DOWNED)
		ppd->link_downed = 0;
	if (counter_select & CS_UNCORRECTABLE_ERRORS)
		hfi_write_lm_fpc_csr(ppd, FXR_FPC_ERR_UNCORRECTABLE_ERROR, 0);

	for_each_set_bit(vl, (unsigned long *)&(vl_select_mask),
			 8 * sizeof(vl_select_mask)) {
		if (counter_select & CS_PORT_XMIT_DATA)
			hfi_write_lm_tp_prf_csr(ppd,
					(TP_XMIT_DATA + idx_from_vl(vl)) + 1, 0);
		if (counter_select & CS_PORT_RCV_DATA)
			hfi_write_lm_fpc_prf_per_vl_csr(ppd,
			FXR_FPC_PRF_PORT_VL_RCV_DATA_CNT, idx_from_vl(vl), 0);
		if (counter_select & CS_PORT_XMIT_PKTS)
			hfi_write_lm_tp_prf_csr(ppd,
			(TP_XMIT_PKTS + idx_from_vl(vl)) + 1, 0);

		if (counter_select & CS_PORT_RCV_PKTS)
			hfi_write_lm_fpc_prf_per_vl_csr(ppd,
			FXR_FPC_PRF_PORT_VL_RCV_PKT_CNT,
			idx_from_vl(vl), 0);

		if (counter_select & CS_PORT_XMIT_WAIT)
			hfi_write_lm_tp_prf_csr(ppd,
			(TP_XMIT_WAIT + idx_from_vl(vl)) + 1, 0);

		/* sw_port_vl_congestion is 0 for HFIs */
		if (counter_select & CS_PORT_RCV_FECN)
			hfi_write_lm_fpc_prf_per_vl_csr(ppd,
				FXR_FPC_PRF_PORT_VL_RCV_FECN,
				idx_from_vl(vl), 0);

		if (counter_select & CS_PORT_RCV_BECN)
			hfi_write_lm_fpc_prf_per_vl_csr(ppd,
				FXR_FPC_PRF_PORT_VL_RCV_BECN,
				idx_from_vl(vl), 0);

		/* port_vl_xmit_time_cong is 0 for HFIs */
		/* port_vl_xmit_wasted_bw ??? */
		/* port_vl_xmit_wait_data - TXE (table 13-9 HFI spec) ??? */
		if (counter_select & CS_PORT_RCV_BUBBLE)
			hfi_write_lm_fpc_prf_per_vl_csr(ppd,
				FXR_FPC_PRF_PORT_VL_RCV_BUBBLE,
				idx_from_vl(vl), 0);
	}
	if (resp_len)
		*resp_len += sizeof(*req);
	return reply((struct ib_mad_hdr *)pmp);

}

static int pma_set_opa_errorinfo(struct opa_pma_mad *pmp,
				 struct ib_device *ibdev, u8 port,
				 u32 *resp_len)
{
	struct _port_ei *rsp;
	struct opa_port_error_info_msg *req;
	struct hfi_devdata *dd = hfi_dd_from_ibdev(ibdev);
	u64 port_mask;
	u32 num_ports;
	unsigned long port_num;
	u8 num_pslm;
	u32 error_info_select;

	req = (struct opa_port_error_info_msg *)pmp->data;
	rsp = (struct _port_ei *)&req->port[0];

	num_ports = OPA_AM_NPORT(be32_to_cpu(pmp->mad_hdr.attr_mod));
	num_pslm = hweight64(be64_to_cpu(req->port_select_mask[3]));

	memset(rsp, 0, sizeof(*rsp));

	if (num_ports != 1 || num_ports != num_pslm) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		return reply((struct ib_mad_hdr *)pmp);
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
		return reply((struct ib_mad_hdr *)pmp);
	}

	error_info_select = be32_to_cpu(req->error_info_select_mask);

	/* PortRcvErrorInfo */
	if (error_info_select & ES_PORT_RCV_ERROR_INFO)
		/* turn off status bit */
		dd->err_info_rcvport.status_and_code &= ~OPA_EI_STATUS_SMASK;

	/* ExcessiverBufferOverrunInfo */

	if (error_info_select & ES_PORT_XMIT_CONSTRAINT_ERROR_INFO)
		dd->err_info_xmit_constraint.status &= ~OPA_EI_STATUS_SMASK;

	if (error_info_select & ES_PORT_RCV_CONSTRAINT_ERROR_INFO)
		dd->err_info_rcv_constraint.status &= ~OPA_EI_STATUS_SMASK;

	/* UncorrectableErrorInfo */
	if (error_info_select & ES_UNCORRECTABLE_ERROR_INFO)
		/* turn off status bit */
		dd->err_info_uncorrectable &= ~OPA_EI_STATUS_SMASK;

	/* FMConfigErrorInfo */
	if (error_info_select & ES_FM_CONFIG_ERROR_INFO)
		/* turn off status bit */
		dd->err_info_fmconfig &= ~OPA_EI_STATUS_SMASK;

	if (resp_len)
		*resp_len += sizeof(*req);

	return reply((struct ib_mad_hdr *)pmp);
}

static int process_perf_opa(struct ib_device *ibdev, u8 port,
			    const struct opa_mad *in_mad,
			    struct opa_mad *out_mad, u32 *resp_len)
{
	struct opa_pma_mad *pmp = (struct opa_pma_mad *)out_mad;
	int ret;
	__be16 st = cpu_to_be16(IB_MGMT_MAD_STATUS_UNSUPPORTED_METHOD_ATTRIB);

	*out_mad = *in_mad;

	if (pmp->mad_hdr.class_version != OPA_SMI_CLASS_VERSION) {
		pmp->mad_hdr.status |=
			cpu_to_be16(IB_MGMT_MAD_STATUS_BAD_VERSION);
		return reply((struct ib_mad_hdr *)pmp);
	}

	*resp_len = sizeof(pmp->mad_hdr);

	switch (pmp->mad_hdr.method) {
	case IB_MGMT_METHOD_GET:
		switch (pmp->mad_hdr.attr_id) {
		case IB_PMA_CLASS_PORT_INFO:
			ret = pma_get_opa_classportinfo(pmp, ibdev, resp_len);
			goto bail;
		case OPA_PM_ATTRIB_ID_PORT_STATUS:
			ret = pma_get_opa_portstatus(pmp, ibdev, port,
						     resp_len);
			goto bail;
		case OPA_PM_ATTRIB_ID_DATA_PORT_COUNTERS:
			ret = pma_get_opa_datacounters(pmp, ibdev, port,
						       resp_len);
			goto bail;
		case OPA_PM_ATTRIB_ID_ERROR_PORT_COUNTERS:
			ret = pma_get_opa_porterrors(pmp, ibdev, port,
						     resp_len);
			goto bail;
		case OPA_PM_ATTRIB_ID_ERROR_INFO:
			ret = pma_get_opa_errorinfo(pmp, ibdev, port,
						    resp_len);
			goto bail;
		default:
			pmp->mad_hdr.status |= st;
			ret = reply((struct ib_mad_hdr *)pmp);
			goto bail;
		}

	case IB_MGMT_METHOD_SET:
		switch (pmp->mad_hdr.attr_id) {
		case OPA_PM_ATTRIB_ID_CLEAR_PORT_STATUS:
			ret = pma_set_opa_portstatus(pmp, ibdev, port,
						     resp_len);
			goto bail;
		case OPA_PM_ATTRIB_ID_ERROR_INFO:
			ret = pma_set_opa_errorinfo(pmp, ibdev, port,
						    resp_len);
			goto bail;
		default:
			pmp->mad_hdr.status |= st;
			ret = reply((struct ib_mad_hdr *)pmp);
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
		pmp->mad_hdr.status |= st;
		ret = reply((struct ib_mad_hdr *)pmp);
	}

bail:
	return ret;
}

static int hfi2_process_opa_mad(struct ib_device *ibdev, int mad_flags,
				u8 port, const struct ib_wc *in_wc,
				const struct ib_grh *in_grh,
				const struct opa_mad *in_mad,
				struct opa_mad *out_mad, size_t *out_mad_size,
				u16 *out_mad_pkey_index)
{
	int ret = IB_MAD_RESULT_FAILURE;
	u32 resp_len = 0;
	int pkey_idx;
	struct hfi2_ibport *ibp = to_hfi_ibp(ibdev, port);

	pkey_idx = hfi2_lookup_pkey_idx(ibp, OPA_LIM_MGMT_PKEY);
	if (pkey_idx < 0) {
		pr_warn("failed to find limited mgmt pkey, defaulting 0x%x\n",
			hfi2_get_pkey(ibp, 1));
		pkey_idx = 1;
	}
	*out_mad_pkey_index = (u16)pkey_idx;

	switch (in_mad->mad_hdr.mgmt_class) {
	case IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE:
	case IB_MGMT_CLASS_SUBN_LID_ROUTED:
		if (hfi_is_local_mad(ibp, in_mad, in_wc, in_grh)) {
			ret = opa_local_smp_check(ibp, in_wc);
			if (ret)
				return IB_MAD_RESULT_FAILURE;
		}
		ret = process_subn_opa(ibdev, mad_flags, port, in_mad,
				       out_mad, &resp_len);
		goto bail;
	case IB_MGMT_CLASS_PERF_MGMT:
		ret = process_perf_opa(ibdev, port, in_mad, out_mad,
				       &resp_len);
		goto bail;

	default:
		ret = IB_MAD_RESULT_SUCCESS;
	}

bail:
	if (ret & IB_MAD_RESULT_REPLY)
		*out_mad_size = round_up(resp_len, 8);
	else if (ret & IB_MAD_RESULT_SUCCESS)
		*out_mad_size = in_wc->byte_len - sizeof(struct ib_grh);

	return ret;
}

static int hfi2_process_ib_mad(struct ib_device *ibdev, int mad_flags, u8 port,
			       const struct ib_wc *in_wc,
			       const struct ib_grh *in_grh,
			       const struct ib_mad *in_mad,
			       struct ib_mad *out_mad)
{
	int ret;

	switch (in_mad->mad_hdr.mgmt_class) {
	case IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE:
	case IB_MGMT_CLASS_SUBN_LID_ROUTED:
		ret = process_subn(ibdev, mad_flags,
				   port, in_mad, out_mad);
		goto bail;
	default:
		ret = IB_MAD_RESULT_SUCCESS;
	}

bail:
	return ret;
}

int hfi2_process_mad(struct ib_device *ibdev, int mad_flags, u8 port,
		     const struct ib_wc *in_wc, const struct ib_grh *in_grh,
		     const struct ib_mad_hdr *in_mad, size_t in_mad_size,
		     struct ib_mad_hdr *out_mad, size_t *out_mad_size,
		     u16 *out_mad_pkey_index)
{
	switch (in_mad->base_version) {
	case OPA_MGMT_BASE_VERSION:
		if (unlikely(in_mad_size != sizeof(struct opa_mad))) {
			dev_err(ibdev->dma_device, "invalid in_mad_size\n");
			return IB_MAD_RESULT_FAILURE;
		}
		return hfi2_process_opa_mad(ibdev, mad_flags, port,
				       in_wc, in_grh,
				       (struct opa_mad *)in_mad,
				       (struct opa_mad *)out_mad,
				       out_mad_size,
				       out_mad_pkey_index);
	case IB_MGMT_BASE_VERSION:
		return hfi2_process_ib_mad(ibdev, mad_flags, port,
				      in_wc, in_grh,
				      (const struct ib_mad *)in_mad,
				      (struct ib_mad *)out_mad);
	default:
		break;
	}

	return IB_MAD_RESULT_FAILURE;
}
