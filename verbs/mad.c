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

static int process_subn(struct ib_device *ibdev, int mad_flags,
			u8 port, struct ib_mad *in_mad,
			struct ib_mad *out_mad)
{
	return IB_MAD_RESULT_FAILURE;
}

static int __subn_get_opa_nodeinfo(struct opa_smp *smp, u32 am, u8 *data,
				   struct ib_device *ibdev, u8 port,
				   u32 *resp_len)
{
	struct opa_node_info *ni;
	struct opa_ib_portdata *ibp = to_opa_ibportdata(ibdev, port);
	struct pci_dev *pdev = container_of(ibdev->dma_device,
				struct pci_dev, dev);
	struct opa_ib_portdata *ibp_first = to_opa_ibportdata(ibdev, 1);
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;

	ni = (struct opa_node_info *)data;

	/* GUID 0 is illegal */
	/* TODO GUID is currently zero. enable this check once this
	 * is fixed
	 */
#warning "guid check bypassed"
#if 0
	if (am || ibp->guid == 0) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply(ibh);
	}
#endif
#warning "get node info HW details needs to be fetched from opa2_hfi"

	ni->port_guid = ibp->guid;
	ni->base_version = JUMBO_MGMT_BASE_VERSION;
	ni->class_version = OPA_SMI_CLASS_VERSION;
	ni->node_type = 1;     /* channel adapter */
	ni->num_ports = ibdev->phys_port_cnt;
	/* This is already in network order */
	ni->system_image_guid = opa_ib_sys_guid;
	ni->node_guid = ibp_first->guid; /* Use first-port GUID as node */
	ni->local_port_num = port;
	ni->device_id = cpu_to_be16(pdev->device);
#if 0
	ni->partition_cap = cpu_to_be16(qib_get_npkeys(dd));
	ni->revision = cpu_to_be32(dd->minrev);
	ni->vendor_id[0] = dd->oui1;
	ni->vendor_id[1] = dd->oui2;
	ni->vendor_id[2] = dd->oui3;
#else
	/* TODO - stubs*/
	ni->partition_cap = OPA_IB_PORT_NUM_PKEYS;
	ni->revision = 0;
	/* TODO -  in WFR these are obtained from base_guid
	 * which is read from csr. Need to find the equivalent in
	 * FXR
	 */
	ni->vendor_id[0] = 0;
	ni->vendor_id[1] = 0;
	ni->vendor_id[2] = 0;
#endif

	if (resp_len)
		*resp_len += sizeof(*ni);

	return reply(ibh);
}

static int subn_get_opa_sma(u16 attr_id, struct opa_smp *smp, u32 am,
			    u8 *data, struct ib_device *ibdev, u8 port,
			    u32 *resp_len)
{
	int ret;
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
#if 0
	struct qib_ibport *ibp = to_iport(ibdev, port);
#endif

#warning "Only get node info supported in MAD methods"
	switch (attr_id) {
	case IB_SMP_ATTR_NODE_INFO:
		ret = __subn_get_opa_nodeinfo(smp, am, data, ibdev, port,
					      resp_len);
		break;
#if 0
	case IB_SMP_ATTR_NODE_DESC:
		ret = __subn_get_opa_nodedesc(smp, am, data, ibdev, port,
					      resp_len);
		break;
	case IB_SMP_ATTR_PORT_INFO:
		ret = __subn_get_opa_portinfo(smp, am, data, ibdev, port,
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
	case OPA_ATTRIB_ID_SC_TO_VLT_MAP:
		ret = __subn_get_opa_sc_to_vlt(smp, am, data, ibdev, port,
					       resp_len);
		break;
	case OPA_ATTRIB_ID_SC_TO_VLNT_MAP:
		ret = __subn_get_opa_sc_to_vlnt(smp, am, data, ibdev, port,
					       resp_len);
		break;
	case OPA_ATTRIB_ID_PORT_STATE_INFO:
		ret = __subn_get_opa_psi(smp, am, data, ibdev, port,
					 resp_len);
		break;
	case OPA_ATTRIB_ID_BUFFER_CONTROL_TABLE:
		ret = __subn_get_opa_bct(smp, am, data, ibdev, port,
					 resp_len);
		break;
	case IB_SMP_ATTR_VL_ARB_TABLE:
		ret = __subn_get_opa_vl_arb(smp, am, data, ibdev, port,
					    resp_len);
		break;
	case OPA_ATTRIB_ID_CONGESTION_INFO:
		ret = __subn_get_opa_cong_info(smp, am, data, ibdev, port,
					       resp_len);
		break;
	case OPA_ATTRIB_ID_HFI_CONGESTION_SETTING:
		ret = __subn_get_opa_cong_setting(smp, am, data, ibdev,
						  port, resp_len);
		break;
	case OPA_ATTRIB_ID_HFI_CONGESTION_LOG:
		ret = __subn_get_opa_hfi_cong_log(smp, am, data, ibdev,
						  port, resp_len);
		break;
	case OPA_ATTRIB_ID_CONGESTION_CONTROL_TABLE:
		ret = __subn_get_opa_cc_table(smp, am, data, ibdev, port,
					      resp_len);
		break;
	case IB_SMP_ATTR_LED_INFO:
		ret = __subn_get_opa_led_info(smp, am, data, ibdev, port,
					      resp_len);
		break;
	case IB_SMP_ATTR_SM_INFO:
		if (ibp->port_cap_flags & IB_PORT_SM_DISABLED)
			return IB_MAD_RESULT_SUCCESS | IB_MAD_RESULT_CONSUMED;
		if (ibp->port_cap_flags & IB_PORT_SM)
			return IB_MAD_RESULT_SUCCESS;
		/* FALLTHROUGH */
#endif
	default:
		smp->status |= IB_SMP_UNSUP_METH_ATTR;
		ret = reply(ibh);
		break;
	}
	return ret;
}

static int process_subn_stl(struct ib_device *ibdev, int mad_flags,
			    u8 port, struct jumbo_mad *in_mad,
			    struct jumbo_mad *out_mad,
			    u32 *resp_len)
{
	struct opa_smp *smp = (struct opa_smp *)out_mad;
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
#if 0
	struct qib_ibport *ibp = to_iport(ibdev, port);
	/* XXX struct qib_pportdata *ppd = ppd_from_ibp(ibp);*/
#endif
	u8 *data;
	u32 am;
	__be16 attr_id;
	int ret = IB_MAD_RESULT_FAILURE;

	*out_mad = *in_mad;
	data = opa_get_smp_data(smp);

	am = be32_to_cpu(smp->attr_mod);
	attr_id = smp->attr_id;
	if (smp->class_version != OPA_SMI_CLASS_VERSION) {
		smp->status |= IB_SMP_UNSUP_VERSION;
		ret = reply(ibh);
		goto bail;
	}
#warning "mkey check not implemented"
#if 0
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
#endif
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
#warning "subn_get_opa_aggregate  not implemented"
#if 0
			ret = subn_get_opa_aggregate(smp, ibdev, port,
						     resp_len);
#endif
			goto bail;
		}
	case IB_MGMT_METHOD_SET:
#warning "MAD Set() and SetResp()  not implemented"
#if 0
		switch (attr_id) {
		default:
			ret = subn_set_opa_sma(attr_id, smp, am, data,
					       ibdev, port, resp_len);
			goto bail;
		case OPA_ATTRIB_ID_AGGREGATE:
			ret = subn_set_opa_aggregate(smp, ibdev, port,
						     resp_len);
			goto bail;
		}
#endif
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
		smp->status |= IB_SMP_UNSUP_METHOD;
		ret = reply(ibh);
	}

bail:
	return ret;
}

static int process_stl_mad(struct ib_device *ibdev, int mad_flags,
			       u8 port, struct ib_wc *in_wc,
			       struct ib_grh *in_grh,
			       struct jumbo_mad *in_mad,
			       struct jumbo_mad *out_mad)
{
	int ret = IB_MAD_RESULT_FAILURE;
	u32 resp_len = 0;
#if 0
	int pkey_idx;
	struct qib_ibport *ibp = to_iport(ibdev, port);

	pkey_idx = wfr_lookup_pkey_idx(ibp, WFR_LIM_MGMT_P_KEY);
	if (pkey_idx < 0) {
		pr_warn("failed to find limited mgmt pkey, defaulting 0x%x\n",
			qib_get_pkey(ibp, 1));
		pkey_idx = 1;
	}
	in_wc->pkey_index = (u16)pkey_idx;
#endif
#warning "pkey check not implemented"

	switch (in_mad->mad_hdr.mgmt_class) {
	case IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE:
	case IB_MGMT_CLASS_SUBN_LID_ROUTED:
		ret = process_subn_stl(ibdev, mad_flags, port, in_mad,
				       out_mad, &resp_len);
		goto bail;
	case IB_MGMT_CLASS_PERF_MGMT:
#warning "process_perf_stl not implemented"
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

