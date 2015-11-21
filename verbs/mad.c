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
 * Intel(R) OPA Gen2 IB Driver
 */
#include <linux/pci.h>
#include "verbs.h"
#include "mad.h"

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

/* This attribute is implemented only here (sma-ib) */
static int __subn_get_ib_nodeinfo(struct ib_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	struct ib_node_info *ni;
	struct opa_ib_data *ibd = to_opa_ibdata(ibdev);
	struct opa_core_device *odev = ibd->odev;
	struct opa_ib_portdata *ibp = to_opa_ibportdata(ibdev, port);
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;

	ni = (struct ib_node_info *)smp->data;

	/* GUID 0 is illegal */
	if (smp->attr_mod || ibd->node_guid == 0
				|| !ibp) {
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
	ni->sys_guid = opa_ib_sys_guid;
	ni->node_guid = ibd->node_guid;
	ni->port_guid = ibp->guid;
	ni->partition_cap = cpu_to_be16(ibp->pkey_tlen);
	ni->local_port_num = port;
	memcpy(ni->vendor_id, ibd->oui, ARRAY_SIZE(ni->vendor_id));
	ni->device_id = cpu_to_be16(odev->id.device);
	ni->revision = cpu_to_be32(odev->id.revision);

	return reply(ibh);
}

/*
 * Send a bad M_Key trap (ch. 14.3.9).
 */
static void bad_mkey(struct opa_ib_portdata *ibp, struct ib_mad_hdr *mad,
		     __be64 mkey, __be32 dr_slid, u8 return_path[], u8 hop_cnt)
{
	struct ib_mad_notice_attr data;

	/* Send violation trap */
	data.generic_type = IB_NOTICE_TYPE_SECURITY;
	data.prod_type_msb = 0;
	data.prod_type_lsb = IB_NOTICE_PROD_CA;
	data.trap_num = IB_NOTICE_TRAP_BAD_MKEY;
	data.issuer_lid = cpu_to_be16(ibp->lid);
	data.toggle_count = 0;
	memset(&data.details, 0, sizeof(data.details));
	data.details.ntc_256.lid = data.issuer_lid;
	data.details.ntc_256.method = mad->method;
	data.details.ntc_256.attr_id = mad->attr_id;
	data.details.ntc_256.attr_mod = mad->attr_mod;
	data.details.ntc_256.mkey = mkey;
	if (mad->mgmt_class == IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE) {

		data.details.ntc_256.dr_slid = (__force __be16)dr_slid;
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

#if 0
	/* FXRTODO: JIRA STL-1138 */
	send_trap(ibp, &data, sizeof(data));
#endif
}

static int check_mkey(struct opa_ib_portdata *ibp, struct ib_mad_hdr *mad,
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

static int process_subn(struct ib_device *ibdev, int mad_flags,
			u8 port, const struct ib_mad *in_mad,
			struct ib_mad *out_mad)
{
	struct ib_smp *smp = (struct ib_smp *)out_mad;
	struct opa_ib_portdata *ibp = to_opa_ibportdata(ibdev, port);
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
	struct opa_ib_data *ibd = to_opa_ibdata(ibdev);
	struct opa_core_device *odev = ibd->odev;
	struct opa_ib_portdata *ibp = to_opa_ibportdata(ibdev, port);

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
	ni->system_image_guid = opa_ib_sys_guid;
	ni->node_guid = ibd->node_guid;
	ni->port_guid = ibp->guid;
	ni->partition_cap = cpu_to_be16(ibp->pkey_tlen);
	ni->local_port_num = port;
	memcpy(ni->vendor_id, ibd->oui, ARRAY_SIZE(ni->vendor_id));
	ni->device_id = cpu_to_be16(odev->id.device);
	ni->revision = cpu_to_be16(odev->id.revision);

	if (resp_len)
		*resp_len += sizeof(*ni);

	return reply(ibh);
}

static int __subn_get_opa_portinfo(struct opa_smp *smp, u32 am, u8 *data,
					struct ib_device *ibdev, u8 port,
					u32 *resp_len)
{
	struct opa_port_info *pi = (struct opa_port_info *)data;
	struct opa_ib_portdata *ibp = to_opa_ibportdata(ibdev, port);

	/* Only return the mkey if the protection field allows it. */
	if (!(ibp->mkey != smp->mkey && ibp->mkeyprot == 1))
		pi->mkey = ibp->mkey;

	pi->subnet_prefix = ibp->gid_prefix;
	pi->ib_cap_mask = cpu_to_be32(ibp->port_cap_flags);
	pi->mkey_lease_period = cpu_to_be16(ibp->mkey_lease_period);
	pi->mkeyprotect_lmc = ibp->mkeyprot << MKEY_SHIFT;
	pi->mkey_violations = cpu_to_be16(ibp->mkey_violations);

	/*
	 * FXRTODO: pkey check in FXR is offloaded to HW. This will
	 * have to be read from a CSR. Ref JIRA STL-2298
	 */
	pi->pkey_violations = cpu_to_be16(ibp->pkey_violations);
	pi->qkey_violations = cpu_to_be16(ibp->qkey_violations);
	pi->clientrereg_subnettimeout = ibp->subnet_timeout;
	pi->port_link_mode  = cpu_to_be16(OPA_PORT_LINK_MODE_OPA << 10 |
					  OPA_PORT_LINK_MODE_OPA << 5 |
					  OPA_PORT_LINK_MODE_OPA);
	pi->sm_trap_qp = cpu_to_be32(ibp->sm_trap_qp);
	pi->sa_qp = cpu_to_be32(ibp->sa_qp);
	pi->local_port_num = port;
	pi->opa_cap_mask = cpu_to_be16(OPA_CAP_MASK3_IsVLrSupported);

	if (resp_len)
		*resp_len += sizeof(struct opa_port_info);

	return reply((struct ib_mad_hdr *)smp);
}

static int __subn_get_opa_psi(struct opa_smp *smp, u32 am, u8 *data,
					struct ib_device *ibdev, u8 port,
							u32 *resp_len)
{
	/* FXRTODO: to be implemented */
	return IB_MAD_RESULT_FAILURE;
}

static int __subn_get_opa_pkeytable(struct opa_smp *smp, u32 am, u8 *data,
					struct ib_device *ibdev, u8 port,
							u32 *resp_len)
{
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;

	/* This is implemented in SMA-HFI*/
	return reply(ibh);
}

static int __subn_get_opa_sl_to_sc(struct opa_smp *smp, u32 am, u8 *data,
					struct ib_device *ibdev, u8 port,
							u32 *resp_len)
{
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;

	/* This is implemented in SMA-HFI*/
	return reply(ibh);
}

static int __subn_get_opa_sc_to_sl(struct opa_smp *smp, u32 am, u8 *data,
					struct ib_device *ibdev, u8 port,
							u32 *resp_len)
{
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;

	/* This is implemented in SMA-HFI*/
	return reply(ibh);
}

static int __subn_get_opa_sc_to_vlr(struct opa_smp *smp, u32 am, u8 *data,
				    struct ib_device *ibdev, u8 port,
				    u32 *resp_len)
{
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;

	/* This is implemented in SMA-HFI*/
	return reply(ibh);
}

static int __subn_get_opa_sc_to_vlt(struct opa_smp *smp, u32 am, u8 *data,
					struct ib_device *ibdev, u8 port,
							u32 *resp_len)
{
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;

	/* This is implemented in SMA-HFI*/
	return reply(ibh);
}

static int __subn_get_opa_sc_to_vlnt(struct opa_smp *smp, u32 am, u8 *data,
					struct ib_device *ibdev, u8 port,
							u32 *resp_len)
{
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;

	/* This is implemented in SMA-HFI*/
	return reply(ibh);
}

static int __subn_get_opa_vl_arb(struct opa_smp *smp, u32 am, u8 *data,
					struct ib_device *ibdev, u8 port,
							u32 *resp_len)
{
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;

	/* This is implemented in SMA-HFI*/
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

	/* This is implemented in SMA-HFI*/
	return reply(ibh);
}

static int __subn_get_opa_bct(struct opa_smp *smp, u32 am, u8 *data,
					struct ib_device *ibdev, u8 port,
							u32 *resp_len)
{
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;

	/* This is implemented in SMA-HFI*/
	return reply(ibh);
}

static int __subn_get_opa_cong_info(struct opa_smp *smp, u32 am, u8 *data,
					struct ib_device *ibdev, u8 port,
							u32 *resp_len)
{
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;

	/* This is implemented in SMA-HFI*/
	return reply(ibh);
}

static int __subn_get_opa_hfi_cong_log(struct opa_smp *smp, u32 am, u8 *data,
					struct ib_device *ibdev, u8 port,
							u32 *resp_len)
{
	/* FXRTODO: to be implemented */
	return IB_MAD_RESULT_FAILURE;
}

static int __subn_get_opa_cong_setting(struct opa_smp *smp, u32 am, u8 *data,
					struct ib_device *ibdev, u8 port,
							u32 *resp_len)
{
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;

	/* This is implemented in SMA-HFI*/
	return reply(ibh);
}

static int __subn_get_opa_cc_table(struct opa_smp *smp, u32 am, u8 *data,
					struct ib_device *ibdev, u8 port,
							u32 *resp_len)
{
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;

	/* This is implemented in SMA-HFI*/
	return reply(ibh);
}

static int __subn_get_opa_sma(u16 attr_id, struct opa_smp *smp, u32 am,
			    u8 *data, struct ib_device *ibdev, u8 port,
			    u32 *resp_len)
{
	int ret;
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
#if 0
	struct qib_ibport *ibp = to_iport(ibdev, port);
#endif
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
	case IB_SMP_ATTR_VL_ARB_TABLE:
		ret = __subn_get_opa_vl_arb(smp, am, data, ibdev, port,
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
#if 0
	/* FXRTODO: figure out if this code is valid for fxr */
	case IB_SMP_ATTR_SM_INFO:
		if (ibp->port_cap_flags & IB_PORT_SM_DISABLED)
			return IB_MAD_RESULT_SUCCESS | IB_MAD_RESULT_CONSUMED;
		if (ibp->port_cap_flags & IB_PORT_SM)
			return IB_MAD_RESULT_SUCCESS;
		/* FALLTHROUGH */
#endif
	default:
		smp->status |=
		cpu_to_be16(IB_MGMT_MAD_STATUS_UNSUPPORTED_METHOD_ATTRIB);
		ret = reply(ibh);
		break;
	}
	return ret;
}


static int subn_get_opa_sma(u16 attr_id, struct opa_smp *smp, u32 am,
			    u8 *data, struct ib_device *ibdev, u8 port,
			    u32 *resp_len)
{
	int ret;
	struct opa_ib_data *ibd = to_opa_ibdata(ibdev);
	struct opa_core_device *odev = ibd->odev;
	struct opa_core_ops *ops = odev->bus_ops;
	u8 sma_status = OPA_SMA_SUCCESS;

	/*
	 * Let the opa*_hfi driver process the MAD attribute first. This way
	 * any methods that are unsupported by the HW can be detected early.
	 */
	ret = ops->get_sma(odev, attr_id, smp, am, data, port, resp_len,
								&sma_status);

	if (ret != IB_MAD_RESULT_FAILURE) {
		switch (sma_status) {
		case OPA_SMA_SUCCESS:
			ret = __subn_get_opa_sma(attr_id, smp, am, data,
						       ibdev, port, resp_len);
			break;
		case OPA_SMA_FAIL_WITH_DATA:
		case OPA_SMA_FAIL_WITH_NO_DATA:
			/*
			 * SMA-HFI failed. The failure is reported in status
			 * field of the MAD header.
			 */
			break;
		default:
			pr_warn("Illegal sma status reported by (Get)SMA\n");
			BUG_ON(1);
		}
	}
	return ret;
}

static int __subn_set_opa_portinfo(struct opa_smp *smp, u32 am, u8 *data,
				struct ib_device *ibdev, u8 port,
					      u32 *resp_len)
{
	struct opa_port_info *pi = (struct opa_port_info *)data;
	struct opa_ib_portdata *ibp = to_opa_ibportdata(ibdev, port);
	u32 lid, smlid;
	struct ib_event event;
	int i, ret;
	u8 smsl, lmc;

	event.device = ibdev;
	event.element.port_num = port;

	ibp->gid_prefix = pi->subnet_prefix;
	ibp->mkey = pi->mkey;
	ibp->mkey_lease_period = be16_to_cpu(pi->mkey_lease_period);
	ibp->mkeyprot = (pi->mkeyprotect_lmc & OPA_PI_MASK_MKEY_PROT_BIT)
							>> MKEY_SHIFT;
	ibp->sm_trap_qp = be32_to_cpu(pi->sm_trap_qp);
	ibp->sa_qp = be32_to_cpu(pi->sa_qp);

	if (pi->mkey_violations == 0)
		ibp->mkey_violations = 0;

	if (pi->pkey_violations == 0)
		ibp->pkey_violations = 0;

	if (pi->qkey_violations == 0)
		ibp->qkey_violations = 0;

	ibp->subnet_timeout =
		pi->clientrereg_subnettimeout & OPA_PI_MASK_SUBNET_TIMEOUT;

	/*
	 * From here on all attributes that are handled in SMA-HFI but cached in
	 * SMA-IB are updated by calling get portinfo.
	 */
	ret = subn_get_opa_sma(IB_SMP_ATTR_PORT_INFO, smp, am, data, ibdev,
				port, resp_len);

	if (ret == IB_MAD_RESULT_FAILURE)
		goto err;

	pi = (struct opa_port_info *)data;

	 /* Update the per VL mtu cached here in IB layer */
	for (i = 0; i < ibp->max_vls; i++) {
		ibp->vl_mtu[i] = opa_pi_to_mtu(pi, i);
		if (ibp->ibmtu < ibp->vl_mtu[i])
			ibp->ibmtu = ibp->vl_mtu[i];
	}

	ibp->vl_mtu[15] = opa_enum_to_mtu(pi->neigh_mtu.pvlx_to_mtu[15 / 2]
									& 0xF);
	lmc = pi->mkeyprotect_lmc & OPA_PI_MASK_LMC;
	lid = be32_to_cpu(pi->lid);

	if (ibp->lid != lid || ibp->lmc !=  lmc) {
		if (ibp->lmc !=  lmc) {
			ibp->lmc = lmc;
			/* FXRTODO: Figure out what these uvents are for */
#if 0
			hfi1_set_uevent_bits(ppd, _HFI1_EVENT_LMC_CHANGE_BIT);
#endif
		}
		if (ibp->lid !=  lid) {
			ibp->lid = lid;
#if 0
			hfi1_set_uevent_bits(ppd, _HFI1_EVENT_LID_CHANGE_BIT);
#endif
		}
		event.event = IB_EVENT_LID_CHANGE;
		ib_dispatch_event(&event);
	}

	smlid = be32_to_cpu(pi->sm_lid);
	smsl = pi->smsl & OPA_PI_MASK_SMSL;

	if (smlid != ibp->sm_lid || smsl != ibp->smsl) {
		/*
		 * FXRTODO: These are needed for sending trap
		 * to FM. Implement this as part of that task.
		 * Idea on allocating sm_ah based on code review
		 * 1. Per port static variable
		 * 2. Allocate here when new SM lid is assigned
		 */
#if 0
		spin_lock_irqsave(&ibp->lock, flags);
		if (ibp->sm_ah) {
			if (smlid != ibp->sm_lid)
				ibp->sm_ah->attr.dlid = smlid;
			if (smsl != ibp->smsl)
				ibp->sm_ah->attr.sl = smsl;
		}
		spin_unlock_irqrestore(&ibp->lock, flags);
#endif
		if (smlid != ibp->sm_lid) {
			pr_info("SubnSet(OPA_PortInfo) smlid 0x%x\n", smlid);
			ibp->sm_lid = smlid;
		}

		if (smsl != ibp->smsl)
			ibp->smsl = smsl;
		event.event = IB_EVENT_SM_CHANGE;
		ib_dispatch_event(&event);
	}
err:
	return ret;
}

static int __subn_set_opa_pkeytable(struct opa_smp *smp, u32 am, u8 *data,
				struct ib_device *ibdev, u8 port,
					       u32 *resp_len)
{
	u32 start_block = OPA_AM_START_BLK(am);
	u32 n_blocks_sent = OPA_AM_NBLK(am);
	u32 end_block;
	int ret;

	end_block = start_block + n_blocks_sent;

	ret = subn_get_opa_sma(IB_SMP_ATTR_PKEY_TABLE, smp, am, data, ibdev,
				port, resp_len);

	if (ret == IB_MAD_RESULT_FAILURE)
		goto err;

	/*
	 * FXRTODO: Need a notification path from opa_core device
	 * to opa_core clients
	 */
#if 0
	if (pkey_changed) {
		struct ib_event event;
		event.event = IB_EVENT_PKEY_CHANGE;
		event.device = ibdev;
		event.element.port_num = port;
		ib_dispatch_event(&event);
	}
#endif

err:
	return ret;
}

static int __subn_set_opa_sl_to_sc(struct opa_smp *smp, u32 am, u8 *data,
				struct ib_device *ibdev, u8 port,
					      u32 *resp_len)
{
	struct opa_ib_portdata *ibp = to_opa_ibportdata(ibdev, port);
	int ret, i;
	u8 *p;

	ret = subn_get_opa_sma(OPA_ATTRIB_ID_SL_TO_SC_MAP, smp, am, data, ibdev,
				port, resp_len);

	if (ret == IB_MAD_RESULT_FAILURE)
		goto err;

	p = (u8 *)data;

	for (i = 0; i < ARRAY_SIZE(ibp->sl_to_sc); i++)
		ibp->sl_to_sc[i] = p[i];

err:
	return ret;
}

static int __subn_set_opa_sc_to_sl(struct opa_smp *smp, u32 am, u8 *data,
				struct ib_device *ibdev, u8 port,
					      u32 *resp_len)
{
	struct opa_ib_portdata *ibp = to_opa_ibportdata(ibdev, port);
	int ret, i;
	u8 *p;

	ret = subn_get_opa_sma(OPA_ATTRIB_ID_SC_TO_SL_MAP, smp, am, data, ibdev,
				port, resp_len);

	if (ret == IB_MAD_RESULT_FAILURE)
		goto err;

	p = (u8 *)data;

	for (i = 0; i < ARRAY_SIZE(ibp->sc_to_sl); i++)
		ibp->sc_to_sl[i] = p[i];

err:
	return ret;
}

static int __subn_set_opa_sc_to_vlr(struct opa_smp *smp, u32 am, u8 *data,
				    struct ib_device *ibdev, u8 port,
				    u32 *resp_len)
{
	return subn_get_opa_sma(OPA_ATTRIB_ID_SC_TO_VLR_MAP, smp, am, data,
				ibdev, port, resp_len);
}

static int __subn_set_opa_sc_to_vlt(struct opa_smp *smp, u32 am, u8 *data,
				struct ib_device *ibdev, u8 port,
					       u32 *resp_len)
{
	struct opa_ib_portdata *ibp = to_opa_ibportdata(ibdev, port);
	int ret;

	ret = subn_get_opa_sma(OPA_ATTRIB_ID_SC_TO_VLT_MAP, smp, am, data,
				ibdev, port, resp_len);

	if (ret == IB_MAD_RESULT_FAILURE)
		goto err;

	memcpy(ibp->sc_to_vl, data, sizeof(ibp->sc_to_vl));
err:
	return ret;
}

static int __subn_set_opa_sc_to_vlnt(struct opa_smp *smp, u32 am, u8 *data,
				struct ib_device *ibdev, u8 port,
					       u32 *resp_len)
{
	return subn_get_opa_sma(OPA_ATTRIB_ID_SC_TO_VLNT_MAP, smp, am, data,
				ibdev, port, resp_len);
}

static int __subn_set_opa_psi(struct opa_smp *smp, u32 am, u8 *data,
				struct ib_device *ibdev, u8 port,
					 u32 *resp_len)
{
	/* FXRTODO: to be implemented */
	return IB_MAD_RESULT_FAILURE;
}

static int __subn_set_opa_bct(struct opa_smp *smp, u32 am, u8 *data,
				struct ib_device *ibdev, u8 port,
					 u32 *resp_len)
{
	return subn_get_opa_sma(OPA_ATTRIB_ID_BUFFER_CONTROL_TABLE, smp, am,
				data, ibdev, port, resp_len);
}

static int __subn_set_opa_vl_arb(struct opa_smp *smp, u32 am, u8 *data,
				struct ib_device *ibdev, u8 port,
					    u32 *resp_len)
{
	return subn_get_opa_sma(OPA_ATTRIB_ID_VL_ARBITRATION, smp, am,
				data, ibdev, port, resp_len);
}

static int __subn_set_opa_cong_setting(struct opa_smp *smp, u32 am, u8 *data,
					struct ib_device *ibdev, u8 port,
						u32 *resp_len)
{
	return subn_get_opa_sma(OPA_ATTRIB_ID_HFI_CONGESTION_SETTING, smp, am,
			data, ibdev, port, resp_len);
}

static int __subn_set_opa_cc_table(struct opa_smp *smp, u32 am, u8 *data,
				struct ib_device *ibdev, u8 port,
					      u32 *resp_len)
{
	return subn_get_opa_sma(OPA_ATTRIB_ID_CONGESTION_CONTROL_TABLE, smp, am,
			data, ibdev, port, resp_len);
}

static int __subn_set_opa_led_info(struct opa_smp *smp, u32 am, u8 *data,
				struct ib_device *ibdev, u8 port,
					      u32 *resp_len)
{
	/* FXRTODO: to be implemented */
	return IB_MAD_RESULT_FAILURE;
}

static int __subn_set_opa_sma(u16 attr_id, struct opa_smp *smp, u32 am,
			    u8 *data, struct ib_device *ibdev, u8 port,
			    u32 *resp_len)
{
	int ret;

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
	case IB_SMP_ATTR_VL_ARB_TABLE:
		ret = __subn_set_opa_vl_arb(smp, am, data, ibdev, port,
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
#if 0
	/* FXRTODO: figure out if this code is valid for fxr */
	case IB_SMP_ATTR_SM_INFO:
		if (ibp->port_cap_flags & IB_PORT_SM_DISABLED)
			return IB_MAD_RESULT_SUCCESS | IB_MAD_RESULT_CONSUMED;
		if (ibp->port_cap_flags & IB_PORT_SM)
			return IB_MAD_RESULT_SUCCESS;
		/* FALLTHROUGH */
#endif
	default:
		smp->status |=
		cpu_to_be16(IB_MGMT_MAD_STATUS_UNSUPPORTED_METHOD_ATTRIB);
		ret = reply((struct ib_mad_hdr *)smp);
		break;
	}

	return ret;
}

static int subn_set_opa_sma(u16 attr_id, struct opa_smp *smp, u32 am,
			    u8 *data, struct ib_device *ibdev, u8 port,
			    u32 *resp_len)
{
	int ret;
	struct opa_ib_data *ibd = to_opa_ibdata(ibdev);
	struct opa_core_device *odev = ibd->odev;
	struct opa_core_ops *ops = odev->bus_ops;
	u8 sma_status = OPA_SMA_SUCCESS;

	/*
	 * Let the opa*_hfi driver process the MAD attribute first. This way
	 * any methods that are unsupported by the HW can be detected early.
	 */
	ret = ops->set_sma(odev, attr_id, smp, am, data, port, resp_len,
								&sma_status);

	/*
	 * Response to Set attributes is by Get, and they are called inside the
	 * corresponding Set attributes. In some failure cases we need to
	 * respond back to FM with the get of the corresponding set attribute.
	 * That is being done here. This is to avoid redundant attribute
	 * component error checking both in SMA-HFI and SMA-IB.
	 */
	if (ret != IB_MAD_RESULT_FAILURE) {
		switch (sma_status) {
		case OPA_SMA_SUCCESS:
			ret = __subn_set_opa_sma(attr_id, smp, am, data,
					       ibdev, port, resp_len);
			break;
		case OPA_SMA_FAIL_WITH_DATA:
			ret = subn_get_opa_sma(smp->attr_id, smp, am, data,
					ibdev, port, resp_len);
			break;
		case OPA_SMA_FAIL_WITH_NO_DATA:
			/*
			 * SMA-HFI failed. The failure is reported in status
			 * field of the MAD header.
			 */
			break;
		default:
			pr_warn("Illegal sma status reported by (Set)SMA\n");
			BUG_ON(1);
		}
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
static int hfi_is_local_mad(struct opa_ib_portdata *ibp, const struct opa_mad *mad,
			    const struct ib_wc *in_wc)
{
	const struct opa_smp *smp = (const struct opa_smp *)mad;

	if (smp->mgmt_class == IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE) {
		return (smp->hop_cnt == 0 &&
			smp->route.dr.dr_slid == OPA_LID_PERMISSIVE &&
			smp->route.dr.dr_dlid == OPA_LID_PERMISSIVE);
	}

	return (in_wc->slid == ibp->lid);
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
static int opa_local_smp_check(struct opa_ib_portdata *ibp,
			       const struct ib_wc *in_wc)
{
	u16 pkey;

	if (in_wc->pkey_index >= ibp->pkey_tlen)
		return 1;

	pkey = ibp->pkeys[in_wc->pkey_index];
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

static int process_subn_stl(struct ib_device *ibdev, int mad_flags,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 3, 0)
			    u8 port, const struct opa_mad *in_mad,
			    struct opa_mad *out_mad,
#else
			    u8 port, const struct jumbo_mad *in_mad,
			    struct jumbo_mad *out_mad,
#endif
			    u32 *resp_len)
{
	struct opa_smp *smp = (struct opa_smp *)out_mad;
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	struct opa_ib_portdata *ibp = to_opa_ibportdata(ibdev, port);
	u8 *data;
	u32 am;
	__be16 attr_id;
	int ret = IB_MAD_RESULT_FAILURE;

	*out_mad = *in_mad;
	data = opa_get_smp_data(smp);

	am = be32_to_cpu(smp->attr_mod);
	attr_id = smp->attr_id;
	if (!ibp) {
		smp->status |=
			cpu_to_be16(IB_MGMT_MAD_STATUS_INVALID_ATTRIB_VALUE);
		ret = reply((struct ib_mad_hdr *)smp);
		goto bail;
	}

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
	/* FXRTODO: to be implemented */
	return IB_MAD_RESULT_FAILURE;
}

static int pma_get_opa_portstatus(struct opa_pma_mad *pmp,
				  struct ib_device *ibdev,
				  u8 port, u32 *resp_len)
{
	struct opa_port_status_req *req =
		(struct opa_port_status_req *)pmp->data;
	u32 vl_select_mask = be32_to_cpu(req->vl_select_mask);
	size_t response_data_size;
	u8 num_vls = hweight32(vl_select_mask);

	response_data_size = sizeof(struct opa_port_status_rsp) +
				num_vls * sizeof(struct _vls_pctrs);

	if (resp_len)
		*resp_len += response_data_size;

	return reply((struct ib_mad_hdr *)pmp);
}

static int pma_get_opa_datacounters(struct opa_pma_mad *pmp,
				    struct ib_device *ibdev, u8 port,
				    u32 *resp_len)
{
	/* FXRTODO: to be implemented */
	return IB_MAD_RESULT_FAILURE;
}

static int pma_get_opa_porterrors(struct opa_pma_mad *pmp,
				  struct ib_device *ibdev, u8 port,
				  u32 *resp_len)
{
	/* FXRTODO: to be implemented */
	return IB_MAD_RESULT_FAILURE;
}

static int pma_get_opa_errorinfo(struct opa_pma_mad *pmp,
				 struct ib_device *ibdev, u8 port,
				 u32 *resp_len)
{
	/* FXRTODO: to be implemented */
	return IB_MAD_RESULT_FAILURE;
}

static int pma_set_opa_portstatus(struct opa_pma_mad *pmp,
				  struct ib_device *ibdev, u8 port,
				  u32 *resp_len)
{
	/* FXRTODO: to be implemented */
	return IB_MAD_RESULT_FAILURE;
}

static int pma_set_opa_errorinfo(struct opa_pma_mad *pmp,
				 struct ib_device *ibdev, u8 port,
				 u32 *resp_len)
{
	/* FXRTODO: to be implemented */
	return IB_MAD_RESULT_FAILURE;
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

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 3, 0)
static int process_opa_mad(struct ib_device *ibdev, int mad_flags,
			   u8 port, const struct ib_wc *in_wc,
			   const struct ib_grh *in_grh,
			   const struct opa_mad *in_mad,
			   struct opa_mad *out_mad, size_t *out_mad_size,
			   u16 *out_mad_pkey_index)
{
	int ret = IB_MAD_RESULT_FAILURE;
	u32 resp_len = 0;
	int pkey_idx;
	struct opa_ib_portdata *ibp = to_opa_ibportdata(ibdev, port);

	pkey_idx = opa_ib_lookup_pkey_idx(ibp, OPA_LIM_MGMT_PKEY);
	if (pkey_idx < 0) {
		pr_warn("failed to find limited mgmt pkey, defaulting 0x%x\n",
			opa_ib_get_pkey(ibp, 1));
		pkey_idx = 1;
	}
	*out_mad_pkey_index = (u16)pkey_idx;

	switch (in_mad->mad_hdr.mgmt_class) {
	case IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE:
	case IB_MGMT_CLASS_SUBN_LID_ROUTED:
		if (hfi_is_local_mad(ibp, in_mad, in_wc)) {
			ret = opa_local_smp_check(ibp, in_wc);
			if (ret)
				return IB_MAD_RESULT_FAILURE;
		}
		ret = process_subn_stl(ibdev, mad_flags, port, in_mad,
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

static int process_ib_mad(struct ib_device *ibdev, int mad_flags, u8 port,
			  const struct ib_wc *in_wc, const struct ib_grh *in_grh,
			  const struct ib_mad *in_mad, struct ib_mad *out_mad)
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

int opa_ib_process_mad(struct ib_device *ibdev, int mad_flags, u8 port,
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
		return process_opa_mad(ibdev, mad_flags, port,
				       in_wc, in_grh,
				       (struct opa_mad *)in_mad,
				       (struct opa_mad *)out_mad,
				       out_mad_size,
				       out_mad_pkey_index);
	case IB_MGMT_BASE_VERSION:
		return process_ib_mad(ibdev, mad_flags, port,
				      in_wc, in_grh,
				      (const struct ib_mad *)in_mad,
				      (struct ib_mad *)out_mad);
	default:
		break;
	}

	return IB_MAD_RESULT_FAILURE;
}
#else
static int process_stl_mad(struct ib_device *ibdev, int mad_flags,
			       u8 port, struct ib_wc *in_wc,
			       struct ib_grh *in_grh,
			       struct jumbo_mad *in_mad,
			       struct jumbo_mad *out_mad)
{
	int ret = IB_MAD_RESULT_FAILURE;
	u32 resp_len = 0;

	/* FXRTODO: Implement pkey check */
#if 0
	int pkey_idx;
	struct opa_ib_portdata *ibp = to_opa_ibportdata(ibdev, port);

	pkey_idx = opa_ib_lookup_pkey_idx(ibp, OPA_LIM_MGMT_PKEY);
	if (pkey_idx < 0) {
		pr_warn("failed to find limited mgmt pkey, defaulting 0x%x\n",
			opa_ib_get_pkey(ibp, 1));
		pkey_idx = 1;
	}
	in_wc->pkey_index = (u16)pkey_idx;
#endif

	switch (in_mad->mad_hdr.mgmt_class) {
	case IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE:
	case IB_MGMT_CLASS_SUBN_LID_ROUTED:
		ret = process_subn_stl(ibdev, mad_flags, port, in_mad,
				       out_mad, &resp_len);
		goto bail;
	case IB_MGMT_CLASS_PERF_MGMT:
	/* FXRTODO: Implement process_perf_stl */
#if 0
		ret = process_perf_stl(ibdev, port, in_mad, out_mad,
				       &resp_len);
#endif
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

static int process_ib_mad(struct ib_device *ibdev, int mad_flags, u8 port,
				struct ib_wc *in_wc, struct ib_grh *in_grh,
				struct ib_mad *in_mad, struct ib_mad *out_mad)
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

int opa_ib_process_mad(struct ib_device *ibdev, int mad_flags, u8 port,
			struct ib_wc *in_wc, struct ib_grh *in_grh,
			struct ib_mad *in_mad, struct ib_mad *out_mad)
{
	switch (in_mad->mad_hdr.base_version) {
	case JUMBO_MGMT_BASE_VERSION:
		return process_stl_mad(ibdev, mad_flags, port,
				in_wc, in_grh,
				(struct jumbo_mad *)in_mad,
				(struct jumbo_mad *)out_mad);
	case IB_MGMT_BASE_VERSION:
		return process_ib_mad(ibdev, mad_flags, port,
				in_wc, in_grh, in_mad, out_mad);
	default:
		break;
	}

	return IB_MAD_RESULT_FAILURE;
}
#endif
