/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2014 - 2015 Intel Corporation.
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
 * Copyright (c) 2014 - 2015 Intel Corporation.
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
 * Intel(R) Omni-Path Gen2 HFI PCIe Driver
 */
#include "opa_hfi.h"
#include <rdma/opa_smi.h>
#include <rdma/ib_mad.h>

static int hfi_reply(struct ib_mad_hdr *ibh)
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

static int __subn_get_hfi_nodeinfo(struct hfi_devdata *dd, struct opa_smp *smp,
				u32 am, u8 *data, u8 port, u32 *resp_len)
{
	struct opa_node_info *ni;
	struct hfi_pportdata *ppd = get_ppd_pn(dd, port);
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;

	ni = (struct opa_node_info *)data;

	/* GUID 0 is illegal */
	if (am || dd->nguid == 0) {
		smp->status |=
			cpu_to_be16(IB_MGMT_MAD_STATUS_INVALID_ATTRIB_VALUE);
		return hfi_reply(ibh);
	}

	ni->port_guid = ppd->pguid;
	ni->base_version = JUMBO_MGMT_BASE_VERSION;
	ni->class_version = OPA_SMI_CLASS_VERSION;
	ni->node_type = 1;     /* channel adapter */
	ni->num_ports = dd->num_pports;
	ni->node_guid = dd->nguid;
	ni->local_port_num = port;
	ni->device_id = cpu_to_be16(dd->pcidev->device);
	/* system_image_guid set in opa_ib */
	/* partition_cap set in opa_ib */
	ni->revision = 0;
	memcpy(ni->vendor_id, dd->oui, ARRAY_SIZE(ni->vendor_id));

	if (resp_len)
		*resp_len += sizeof(*ni);

	return hfi_reply(ibh);
}

int hfi_get_sma(struct opa_core_device *odev, u16 attr_id, struct opa_smp *smp,
			u32 am, u8 *data, u8 port, u32 *resp_len)
{
	int ret;
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	/*
	 * FXRTODO: Only get node info supported in MAD methods.
	 * Others yet to be implemented.
	 */
	switch (attr_id) {
	case IB_SMP_ATTR_NODE_INFO:
		ret = __subn_get_hfi_nodeinfo(odev->dd, smp, am, data, port,
					      resp_len);
		break;
	default:
		smp->status |=
		cpu_to_be16(IB_MGMT_MAD_STATUS_UNSUPPORTED_METHOD_ATTRIB);
		ret = hfi_reply(ibh);
		break;
	}
	return ret;
}

int hfi_set_sma(struct opa_core_device *odev, u16 attr_id, struct opa_smp *smp,
				 u32 am, u8 *data, u8 port, u32 *resp_len)
{
	return 1;
}

