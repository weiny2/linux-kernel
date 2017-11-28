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

#include "verbs.h"
#include "packet.h"

int hfi2_check_ah(struct ib_device *ibdev, struct rdma_ah_attr *ah_attr)
{
	struct hfi2_ibport *ibp;
	struct hfi_pportdata *ppd;
	int num_sls;
	u8 sc5;

	/*
	 * update the ah attributes before checking validity of lid
	 */
	hfi2_update_ah_attr(ibdev, ah_attr);
	hfi2_make_opa_lid(ah_attr);

	if (hfi2_check_mcast(rdma_ah_get_dlid(ah_attr)) &&
	    !(rdma_ah_get_ah_flags(ah_attr) & IB_AH_GRH))
		return -EINVAL;
	ibp = to_hfi_ibp(ibdev, rdma_ah_get_port_num(ah_attr));
	if (!ibp)
		return -EINVAL;
	ppd = ibp->ppd;

	if (rdma_ah_get_dlid(ah_attr) == 0)
		return -EINVAL;
	num_sls = ARRAY_SIZE(ppd->sl_pairs);
	if (rdma_ah_get_sl(ah_attr) >= num_sls ||
	    rdma_ah_get_sl(ah_attr) == HFI_INVALID_RESP_SL)
		return -EINVAL;
	/* test the mapping for validity */
	sc5 = ppd->sl_to_sc[rdma_ah_get_sl(ah_attr)];
	if (ppd->sc_to_vlt[sc5] > ppd->vls_supported &&
	    ppd->sc_to_vlt[sc5] != 0xf)
		return -EINVAL;

	/*
	 * HFI2 allows transmitting verbs packets on the response SL
	 * but does not allow receiving anything but Portals on the
	 * response SL due to the way the hardware control pipeline is plumbed.
	 * This check during AH creation ensures that a response SL is
	 * not used by verbs.
	 */
	if (hfi2_is_verbs_resp_sl(ibp->ppd, rdma_ah_get_sl(ah_attr)))
		return -EINVAL;
	return 0;
}

void hfi2_notify_new_ah(struct ib_device *ibdev,
			struct rdma_ah_attr *ah_attr,
			struct rvt_ah *ah)
{
	struct hfi2_ibport *ibp;
	struct hfi_pportdata *ppd;
	u8 sc, vl;

	ibp = to_hfi_ibp(ibdev, rdma_ah_get_port_num(ah_attr));
	ppd = ibp->ppd;
	/*
	 * No SL maps to VL15. IB_MAD passes in a bogus SL for QPT_SMI's AH.
	 * This yields a likely wrong log_pmtu for QPT_SMI, but this is
	 * okay since ah->log_pmtu is unused in check_send_wqe() for QPT_SMI.
	 */
	sc = ppd->sl_to_sc[rdma_ah_get_sl(ah_attr)];
	vl = ppd->sc_to_vlt[sc];
	if (vl < ppd->vls_supported) {
		u16 mtu;

		ah->vl = vl;
		/* rdmavt requires MTU to be power of 2 */
		mtu = min_t(u16, ppd->vl_mtu[vl], HFI_VERBS_MAX_MTU);
		WARN_ON(mtu && !is_power_of_2(mtu));
		ah->log_pmtu = ilog2(mtu);
	}
}
