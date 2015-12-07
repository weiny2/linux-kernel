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
 * Intel(R) Omni-Path Gen2 HFI PCIe Driver
 */
#include "opa_hfi.h"
#include <rdma/opa_smi.h>
#include <rdma/ib_mad.h>
#include "attr.h"
#include <rdma/opa_core_ib.h>
#include "link.h"

static inline void hfi_invalid_attr(struct opa_smp *smp)
{
	smp->status |=
		cpu_to_be16(IB_MGMT_MAD_STATUS_INVALID_ATTRIB_VALUE);
}

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

static int __subn_get_hfi_portinfo(struct hfi_devdata *dd, struct opa_smp *smp,
		u32 am, u8 *data, u8 port, u32 *resp_len, u8 *sma_status)
{
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	struct opa_port_info *pi = (struct opa_port_info *)data;
	int i;
	u32 num_ports = OPA_AM_NPORT(am);
	u32 start_of_sm_config = OPA_AM_START_SM_CFG(am);
	u32 lstate;
	u32 buffer_units;
	u8 mtu, credit_rate;

	if (num_ports != 1) {
		hfi_invalid_attr(smp);
		*sma_status = OPA_SMA_FAIL_WITH_NO_DATA;
		return hfi_reply(ibh);
	}

	lstate = hfi_driver_lstate(ppd);

	if (start_of_sm_config && (lstate == IB_PORT_INIT))
		ppd->is_sm_config_started = 1;

	pi->lid = cpu_to_be32(ppd->lid);
	pi->sm_lid = cpu_to_be32(ppd->sm_lid);
	pi->smsl = ppd->smsl & OPA_PI_MASK_SMSL;
	pi->mkeyprotect_lmc |= ppd->lmc;

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

	pi->port_phys_conf = hfi_porttype(ppd) & 0xf;

	/*
	 * FXTODO: WFR  has ledenable_offlinereason as alternative
	 * to offline_reason. Why ?
	 */
	pi->port_states.offline_reason = ppd->neighbor_normal << NNORMAL_SHIFT;
	pi->port_states.offline_reason |=
			ppd->is_sm_config_started << SM_CONFIG_SHIFT;
	pi->port_states.offline_reason |= ppd->offline_disabled_reason &
				OPA_PI_MASK_OFFLINE_REASON;

	pi->operational_vls =
		hfi_get_ib_cfg(ppd, HFI_IB_CFG_OP_VLS);

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

	pi->vl.cap = ppd->vls_supported;
	/* VL Arbitration table doesn't exist for FXR */

	pi->mtucap = opa_mtu_to_enum(HFI_DEFAULT_MAX_MTU);
	pi->port_ltp_crc_mode = cpu_to_be16(ppd->port_ltp_crc_mode);
	pi->port_states.portphysstate_portstate =
		(hfi_ibphys_portstate(ppd) << PHYSPORTSTATE_SHIFT) | lstate;

	pi->port_mode = cpu_to_be16(
				ppd->is_active_optimize_enabled ?
					OPA_PI_MASK_PORT_ACTIVE_OPTOMIZE : 0);
	if (ppd->part_enforce & HFI_PART_ENFORCE_IN)
		pi->partenforce_filterraw |= OPA_PI_MASK_PARTITION_ENFORCE_IN;
	if (ppd->part_enforce & HFI_PART_ENFORCE_OUT)
		pi->partenforce_filterraw |= OPA_PI_MASK_PARTITION_ENFORCE_OUT;
	/*
	 * FXRTODO:
	 * 1. STL2 spec says that the enabled field is
	 *    to be refered to verify that the ingress packet matches
	 *    the formats in this mask. This is not done in WFR
	 *    for some reason. If there is a valid reason then
	 *    would that apply even for FXR ?
	 * 2. If we should implement this check according to STL2
	 *    spec (section 9.10.7), then probably that needs
	 *    to be done in the verbs layer. This will make this
	 *    field part of per port IB structure and be queried
	 *    by get_port_desc opa_core op. STL-1660
	 * 3. Once we add 16B support in the driver then driver
	 *    should advertize this in the supported field.
	 *    STL-1661
	 */
	pi->port_packet_format.supported =
		cpu_to_be16(OPA_PORT_PACKET_FORMAT_9B);
	pi->port_packet_format.enabled =
		cpu_to_be16(OPA_PORT_PACKET_FORMAT_9B);
	pi->link_down_reason = ppd->local_link_down_reason.sma;
	pi->neigh_link_down_reason = ppd->neigh_link_down_reason.sma;
	pi->port_error_action = cpu_to_be32(ppd->port_error_action);
	/* 32.768 usec. response time (guessing). Magic number ported from WFR*/
	pi->resptimevalue = 3;
	pi->neigh_node_guid = ppd->neighbor_guid;
	pi->neigh_port_num = ppd->neighbor_port_number;
	pi->port_neigh_mode =
		(ppd->neighbor_type & OPA_PI_MASK_NEIGH_NODE_TYPE) |
		(ppd->mgmt_allowed ? OPA_PI_MASK_NEIGH_MGMT_ALLOWED : 0) |
		(ppd->neighbor_fm_security ?
			OPA_PI_MASK_NEIGH_FW_AUTH_BYPASS : 0);

	/* HFIs shall always return VL15 credits to their
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
	/* buffer info for FM */
	pi->overall_buffer_space = cpu_to_be16(ppd->link_credits);

	return hfi_reply(ibh);
}

static int __subn_get_hfi_psi(struct hfi_devdata *dd, struct opa_smp *smp,
		u32 am, u8 *data, u8 port, u32 *resp_len, u8 *sma_status)
{
	/* FXRTODO: to be implemented */
	return IB_MAD_RESULT_FAILURE;
}

static int __subn_get_hfi_pkeytable(struct hfi_devdata *dd, struct opa_smp *smp,
		u32 am, u8 *data, u8 port, u32 *resp_len, u8 *sma_status)
{
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
		*sma_status = OPA_SMA_FAIL_WITH_NO_DATA;
		return hfi_reply(ibh);
	}

	for (i = start_block, k = 0; i < end_block; i++)
		for (j = 0; j < OPA_PKEY_TABLE_BLK_COUNT; j++, k++)
			pkeys[k] = cpu_to_be16(ppd->pkeys[i *
					OPA_PKEY_TABLE_BLK_COUNT + j]);

	if (resp_len)
		*resp_len += size;

	return hfi_reply(ibh);
}

static int __subn_get_hfi_sl_to_sc(struct hfi_devdata *dd, struct opa_smp *smp,
		u32 am, u8 *data, u8 port, u32 *resp_len, u8 *sma_status)
{
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	size_t size = sizeof(ppd->sl_to_sc);
	unsigned i;
	u8 *p = (u8 *)data;

	if (am) {
		hfi_invalid_attr(smp);
		*sma_status = OPA_SMA_FAIL_WITH_NO_DATA;
		return hfi_reply(ibh);
	}

	for (i = 0; i < ARRAY_SIZE(ppd->sl_to_sc); i++)
		p[i] = ppd->sl_to_sc[i];

	if (resp_len)
		*resp_len += size;

	return hfi_reply(ibh);
}

static int __subn_get_hfi_sc_to_sl(struct hfi_devdata *dd, struct opa_smp *smp,
		u32 am, u8 *data, u8 port, u32 *resp_len, u8 *sma_status)
{
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	size_t size = sizeof(ppd->sc_to_sl);
	unsigned i;
	u8 *p = (u8 *)data;

	if (am) {
		hfi_invalid_attr(smp);
		*sma_status = OPA_SMA_FAIL_WITH_NO_DATA;
		return hfi_reply(ibh);
	}

	for (i = 0; i < ARRAY_SIZE(ppd->sc_to_sl); i++)
		p[i] = ppd->sc_to_sl[i];

	if (resp_len)
		*resp_len += size;

	return hfi_reply(ibh);
}

static int __subn_get_hfi_sc_to_vlr(struct hfi_devdata *dd,
				    struct opa_smp *smp, u32 am,
				    u8 *data, u8 port, u32 *resp_len,
				    u8 *sma_status)
{
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	u32 n_blocks = OPA_AM_NBLK(am);
	u8 *p = (void *)data;
	size_t size = sizeof(ppd->sc_to_vlr);

	if (n_blocks != 1) {
		hfi_invalid_attr(smp);
		*sma_status = OPA_SMA_FAIL_WITH_NO_DATA;
		return hfi_reply(ibh);
	}

	memcpy(p, ppd->sc_to_vlr, size);

	if (resp_len)
		*resp_len += size;

	return hfi_reply(ibh);
}

static int __subn_get_hfi_sc_to_vlt(struct hfi_devdata *dd,
					struct opa_smp *smp, u32 am,
					u8 *data, u8 port, u32 *resp_len,
					u8 *sma_status)
{

	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	u32 n_blocks = OPA_AM_NBLK(am);
	u8 *p = (void *) data;
	size_t size = sizeof(ppd->sc_to_vlt);

	if (n_blocks != 1) {
		hfi_invalid_attr(smp);
		*sma_status = OPA_SMA_FAIL_WITH_NO_DATA;
		return hfi_reply(ibh);
	}

	memcpy(p, ppd->sc_to_vlt, size);

	if (resp_len)
		*resp_len += size;

	return hfi_reply(ibh);
}

static int __subn_get_hfi_sc_to_vlnt(struct hfi_devdata *dd,
		struct opa_smp *smp, u32 am, u8 *data, u8 port, u32 *resp_len,
							u8 *sma_status)
{

	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	u32 n_blocks = OPA_AM_NBLK(am);
	u8 *p = (void *) data;
	size_t size = sizeof(ppd->sc_to_vlnt);

	if (n_blocks != 1) {
		hfi_invalid_attr(smp);
		*sma_status = OPA_SMA_FAIL_WITH_NO_DATA;
		return hfi_reply(ibh);
	}

	memcpy(p, ppd->sc_to_vlnt, size);

	if (resp_len)
		*resp_len += size;

	return hfi_reply(ibh);
}

/*
 * FXRTODO: This attribute will not be supported in FXR.
 * This will need to be reported as unsupported attribute in
 * the SMPs status field. Refer STL-2109
 */
static int __subn_get_hfi_vl_arb(struct hfi_devdata *dd, struct opa_smp *smp,
		u32 am, u8 *data, u8 port, u32 *resp_len, u8 *sma_status)
{
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	u32 num_ports = OPA_AM_NPORT(am);
	u8 section = (am & 0x00ff0000) >> 16;
	u8 *p = data;
	int size = 256;
	int table_size =
		HFI_VL_ARB_TABLE_SIZE * sizeof(struct ib_vl_weight_elem);

	if (num_ports != 1)
		goto err;

	switch (section) {
	case OPA_VLARB_LOW_ELEMENTS:
		memcpy(p, ppd->vl_arb_low, table_size);
		break;
	case OPA_VLARB_HIGH_ELEMENTS:
		memcpy(p, ppd->vl_arb_high, table_size);
		break;
	case OPA_VLARB_PREEMPT_ELEMENTS:
		memcpy(p, ppd->vl_arb_prempt_ele, table_size);
		break;
	case OPA_VLARB_PREEMPT_MATRIX:
		memcpy(p, ppd->vl_arb_prempt_mat, table_size);
		break;
	default:
		pr_warn("OPA SubnGet(VL Arb) AM Invalid : 0x%x\n",
			be32_to_cpu(smp->attr_mod));
		goto err;
	}

	if (resp_len)
		*resp_len += size;

	goto done;
err:
	hfi_invalid_attr(smp);
	*sma_status = OPA_SMA_FAIL_WITH_NO_DATA;
done:
	return hfi_reply(ibh);
}

static int __subn_get_hfi_led_info(struct hfi_devdata *dd, struct opa_smp *smp,
		u32 am, u8 *data, u8 port, u32 *resp_len, u8 *sma_status)
{
	/* FXRTODO: to be implemented */
	return IB_MAD_RESULT_FAILURE;
}

static int __subn_get_hfi_cable_info(struct hfi_devdata *dd,
		struct opa_smp *smp, u32 am, u8 *data, u8 port, u32 *resp_len,
							u8 *sma_status)
{
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	u32 len = OPA_AM_CI_LEN(am);

	/* FXRTODO: Implement as part of QSFP STL-3910 */
	memset(data, 0, len);

	if (resp_len)
		*resp_len += len;

	return hfi_reply(ibh);
}

static int __subn_get_hfi_bct(struct hfi_devdata *dd, struct opa_smp *smp,
		u32 am, u8 *data, u8 port, u32 *resp_len, u8 *sma_status)
{
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	struct buffer_control *p = (struct buffer_control *) data;
	u32 num_ports = OPA_AM_NPORT(am);
	int size = sizeof(*p);

	if (num_ports != 1) {
		hfi_invalid_attr(smp);
		*sma_status = OPA_SMA_FAIL_WITH_NO_DATA;
		return hfi_reply(ibh);
	}

	hfi_get_buffer_control(ppd, p, NULL);

	if (resp_len)
		*resp_len += size;

	return hfi_reply(ibh);
}

static int __subn_get_hfi_cong_info(struct hfi_devdata *dd, struct opa_smp *smp,
		u32 am, u8 *data, u8 port, u32 *resp_len, u8 *sma_status)
{
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	struct opa_congestion_info_attr *p =
		(struct opa_congestion_info_attr *)data;
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);

	p->congestion_info = 0;
	p->control_table_cap = ppd->cc_max_table_entries;
	p->congestion_log_length = OPA_CONG_LOG_ELEMS;

	if (resp_len)
		*resp_len += sizeof(*p);

	return hfi_reply(ibh);
}

static int __subn_get_hfi_hfi_cong_log(struct hfi_devdata *dd,
		struct opa_smp *smp, u32 am, u8 *data, u8 port, u32 *resp_len,
							u8 *sma_status)
{
	/* FXRTODO: to be implemented */
	return IB_MAD_RESULT_FAILURE;
}

void hfi_cc_state_reclaim(struct rcu_head *rcu)
{
	struct cc_state *cc_state = container_of(rcu, struct cc_state, rcu);

	kfree(cc_state);
}

static int __subn_get_hfi_cong_setting(struct hfi_devdata *dd,
		struct opa_smp *smp, u32 am, u8 *data, u8 port, u32 *resp_len,
							u8 *sma_status)
{
	int i;
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	struct opa_congestion_setting_attr *p =
		(struct opa_congestion_setting_attr *) data;
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	struct opa_congestion_setting_entry_shadow *entries;
	struct cc_state *cc_state;

	rcu_read_lock();

	cc_state = hfi_get_cc_state(ppd);

	if (cc_state == NULL) {
		rcu_read_unlock();
		/* FXRTODO: Should error attribute be set */
		return hfi_reply(ibh);
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

	return hfi_reply(ibh);
}

static int __subn_get_hfi_cc_table(struct hfi_devdata *dd, struct opa_smp *smp,
				u32 am, u8 *data, u8 port, u32 *resp_len,
							u8 *sma_status)
{
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	struct ib_cc_table_attr *cc_table_attr =
		(struct ib_cc_table_attr *) data;
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
		*sma_status = OPA_SMA_FAIL_WITH_NO_DATA;
		return hfi_reply(ibh);
	}

	rcu_read_lock();

	cc_state = hfi_get_cc_state(ppd);

	if (cc_state == NULL) {
		rcu_read_unlock();
		return hfi_reply(ibh);
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
		*resp_len += sizeof(u16)*(HFI_IB_CCT_ENTRIES * n_blocks + 1);

	return hfi_reply(ibh);
}

int hfi_get_sma(struct opa_core_device *odev, u16 attr_id, struct opa_smp *smp,
		u32 am, u8 *data, u8 port, u32 *resp_len, u8 *sma_status)
{
	int ret;
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;

	/*
	 * FXRTODO: Only get node info supported in MAD methods.
	 * Others yet to be implemented.
	 */
	switch (attr_id) {
	case IB_SMP_ATTR_NODE_DESC:
	case IB_SMP_ATTR_NODE_INFO:
		/* Implemented in opa_ib */
		ret = hfi_reply(ibh);
		break;
	case IB_SMP_ATTR_PORT_INFO:
		ret = __subn_get_hfi_portinfo(odev->dd, smp, am, data, port,
					      resp_len, sma_status);
		break;
	case OPA_ATTRIB_ID_PORT_STATE_INFO:
		ret = __subn_get_hfi_psi(odev->dd, smp, am, data, port,
					 resp_len, sma_status);
		break;
	case IB_SMP_ATTR_PKEY_TABLE:
		ret = __subn_get_hfi_pkeytable(odev->dd, smp, am, data, port,
					       resp_len, sma_status);
		break;
	case OPA_ATTRIB_ID_SL_TO_SC_MAP:
		ret = __subn_get_hfi_sl_to_sc(odev->dd, smp, am, data, port,
					      resp_len, sma_status);
		break;
	case OPA_ATTRIB_ID_SC_TO_SL_MAP:
		ret = __subn_get_hfi_sc_to_sl(odev->dd, smp, am, data, port,
					      resp_len, sma_status);
		break;
	case OPA_ATTRIB_ID_SC_TO_VLR_MAP:
		ret = __subn_get_hfi_sc_to_vlr(odev->dd, smp, am, data, port,
					       resp_len, sma_status);
		break;
	case OPA_ATTRIB_ID_SC_TO_VLT_MAP:
		ret = __subn_get_hfi_sc_to_vlt(odev->dd, smp, am, data, port,
					       resp_len, sma_status);
		break;
	case OPA_ATTRIB_ID_SC_TO_VLNT_MAP:
		ret = __subn_get_hfi_sc_to_vlnt(odev->dd, smp, am, data, port,
					       resp_len, sma_status);
		break;
	case IB_SMP_ATTR_VL_ARB_TABLE:
		ret = __subn_get_hfi_vl_arb(odev->dd, smp, am, data, port,
					    resp_len, sma_status);
		break;
	case IB_SMP_ATTR_LED_INFO:
		ret = __subn_get_hfi_led_info(odev->dd, smp, am, data, port,
					      resp_len, sma_status);
		break;
	case OPA_ATTRIB_ID_CABLE_INFO:
		ret = __subn_get_hfi_cable_info(odev->dd, smp, am, data, port,
						resp_len, sma_status);
		break;
	case OPA_ATTRIB_ID_BUFFER_CONTROL_TABLE:
		ret = __subn_get_hfi_bct(odev->dd, smp, am, data, port,
					 resp_len, sma_status);
		break;
	case OPA_ATTRIB_ID_CONGESTION_INFO:
		ret = __subn_get_hfi_cong_info(odev->dd, smp, am, data, port,
					       resp_len, sma_status);
		break;
	case OPA_ATTRIB_ID_HFI_CONGESTION_LOG:
		ret = __subn_get_hfi_hfi_cong_log(odev->dd, smp, am, data,
						  port, resp_len, sma_status);
		break;
	case OPA_ATTRIB_ID_HFI_CONGESTION_SETTING:
		ret = __subn_get_hfi_cong_setting(odev->dd, smp, am, data,
						  port, resp_len, sma_status);
		break;
	case OPA_ATTRIB_ID_CONGESTION_CONTROL_TABLE:
		ret = __subn_get_hfi_cc_table(odev->dd, smp, am, data, port,
					      resp_len, sma_status);
		break;
	case IB_SMP_ATTR_SM_INFO:
		return IB_MAD_RESULT_SUCCESS;
	default:
		smp->status |=
		cpu_to_be16(IB_MGMT_MAD_STATUS_UNSUPPORTED_METHOD_ATTRIB);
		*sma_status = OPA_SMA_FAIL_WITH_NO_DATA;
		ret = hfi_reply(ibh);
		break;
	}

	return ret;
}

static int set_port_states(struct hfi_devdata *dd, struct hfi_pportdata *ppd,
				struct opa_smp *smp, u32 lstate, u32 pstate,
				u8 suppress_idle_sma)
{
	/* HFI driver clubs pstate and lstate to Host Link State (HLS) */
	u32 hls;
	int ret;

	if (pstate && !(lstate == IB_PORT_DOWN || lstate == IB_PORT_NOP)) {
		pr_warn("SubnSet(OPA_PortInfo) port state invalid; port phys\
				state 0x%x link state 0x%x\n", pstate, lstate);
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
			     OPA_LINKDOWN_REASON_FM_BOUNCE, 0,
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
		if (ppd->neighbor_normal) {
			ret = hfi_set_link_state(ppd, HLS_UP_ACTIVE);
			if (ret == 0)
				hfi_send_idle_sma(ppd, SMA_IDLE_ACTIVE);
		} else {
			pr_warn("SubnSet(OPA_PortInfo) Cannot move to Active \
						with NeighborNormal 0\n");
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

static int __subn_set_hfi_portinfo(struct hfi_devdata *dd, struct opa_smp *smp,
		u32 am, u8 *data, u8 port, u32 *resp_len, u8 *sma_status)
{
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	struct opa_port_info *pi = (struct opa_port_info *)data;
	int ret, i, invalid = 0, call_set_mtu = 0;
	int call_link_downgrade_policy = 0;
	u32 num_ports = OPA_AM_NPORT(am);
	u32 start_of_sm_config = OPA_AM_START_SM_CFG(am);
	u32 lid, smlid;
	u16 lse, lwe, mtu;
	u16 crc_enabled;
	u8 ls_old, ls_new, ps_new;
	u8 vls, lmc, smsl;

	if (num_ports != 1) {
		hfi_invalid_attr(smp);
		*sma_status = OPA_SMA_FAIL_WITH_NO_DATA;
		return hfi_reply(ibh);
	}

	/*
	 * FXRTODO:
	 * Currently this check ensures that the LID is 16bit
	 * i.e the packet is 9B
	 *
	 * When we support 16B packets on FXR, there will
	 * be additional check/modifications for 24 bit LID here.
	 *
	 * bail out early if the LID and SMLID are invalid
	 */
	lid = be32_to_cpu(pi->lid);
	if (!IS_VALID_LID_SIZE(lid)) {
		pr_warn("OPA_PortInfo lid out of range: %X ", lid);
		pr_warn("(> 16b LIDs not supported)\n");
		hfi_invalid_attr(smp);
		*sma_status = OPA_SMA_FAIL_WITH_DATA;
		return hfi_reply(ibh);
	}

	smlid = be32_to_cpu(pi->sm_lid);
	if (!IS_VALID_LID_SIZE(smlid)) {
		pr_warn("OPA_PortInfo SM lid out of range: %X ", smlid);
		pr_warn("(> 16b LIDs not supported)\n");
		hfi_invalid_attr(smp);
		*sma_status = OPA_SMA_FAIL_WITH_DATA;
		return hfi_reply(ibh);
	}

	/*
	 * FXRTODO: This mask is to be read during FC/MNH error interrupts
	 * and if the error matches the bits in this mask then link
	 * bounce is initiated by the driver. See JIRA STL-70.
	 */
	ppd->port_error_action = be32_to_cpu(pi->port_error_action);
	lwe = be16_to_cpu(pi->link_width.enabled);
	if (lwe) {
		if (lwe == HFI_LINK_WIDTH_RESET
				|| lwe == HFI_LINK_WIDTH_RESET_OLD)
			hfi_set_link_width_enabled(ppd,
					ppd->link_width_supported);
		else if ((lwe & ~ppd->link_width_supported) == 0)
			hfi_set_link_width_enabled(ppd, lwe);
		else
			hfi_invalid_attr(smp);
	}
	lwe = be16_to_cpu(pi->link_width_downgrade.enabled);
	/* LWD.E is always applied - 0 means "disabled" */
	if (lwe == HFI_LINK_WIDTH_RESET
			|| lwe == HFI_LINK_WIDTH_RESET_OLD) {
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
	smsl = pi->smsl & OPA_PI_MASK_SMSL;
	lmc = pi->mkeyprotect_lmc & OPA_PI_MASK_LMC;
	ls_old = hfi_driver_lstate(ppd);

	/* Must be a valid unicast LID address. */
	if ((lid == 0 && ls_old > IB_PORT_INIT) ||
	     lid >= HFI_MULTICAST_LID_BASE) {
		/*
		 * FXRTODO: HFI_MULTICAST_LID_BASE valid for 9B only.
		 * modify this check for 16B
		 */
		hfi_invalid_attr(smp);
		pr_warn("SubnSet(OPA_PortInfo) lid invalid 0x%x\n", lid);
	} else if (ppd->lid != lid ||
		 ppd->lmc != lmc) {
		/* FXRTODO: implement uevent update ? */
#if 0
		if (ppd->lid != lid)
			hfi1_set_uevent_bits(ppd, _HFI1_EVENT_LID_CHANGE_BIT);
		if (ppd->lmc != lmc)
			hfi1_set_uevent_bits(ppd, _HFI1_EVENT_LMC_CHANGE_BIT);
#endif
		ppd->lid = lid;
		ppd->lmc = lmc;
		hfi_set_ib_cfg(ppd, HFI_IB_CFG_LIDLMC, 0, NULL);
	}

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

	/*
	 * SMA-HFI is handling smlid and smsl because it has a dependency
	 * on old link state which is maintained in HFI.
	 *
	 * FXRTODO: modify this code when adding 16B support
	 *
	 * Must be a valid unicast LID address
	 */
	if ((smlid == 0 && ls_old > IB_PORT_INIT) ||
	     smlid >= HFI_MULTICAST_LID_BASE) {
		hfi_invalid_attr(smp);
		pr_warn("SubnSet(OPA_PortInfo) smlid invalid 0x%x\n", smlid);
	} else if (smlid != ppd->sm_lid || smsl != ppd->smsl) {
		if (smlid != ppd->sm_lid)
			ppd->sm_lid = smlid;
		if (smsl != ppd->smsl)
			ppd->smsl = smsl;
	}

	if (pi->link_down_reason == 0) {
		ppd->local_link_down_reason.sma = 0;
		ppd->local_link_down_reason.latest = 0;
	}

	if (pi->neigh_link_down_reason == 0) {
		ppd->neigh_link_down_reason.sma = 0;
		ppd->neigh_link_down_reason.latest = 0;
	}

	ppd->is_active_optimize_enabled =
			!!(be16_to_cpu(pi->port_mode)
					& OPA_PI_MASK_PORT_ACTIVE_OPTOMIZE);

	ppd->vl_high_limit = be16_to_cpu(pi->vl.high_limit) & 0xFF;
	hfi_set_ib_cfg(ppd, HFI_IB_CFG_VL_HIGH_LIMIT,
				    ppd->vl_high_limit, NULL);

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
			(void)hfi_set_ib_cfg(ppd, HFI_IB_CFG_OP_VLS,
						vls, NULL);
	}

	crc_enabled = be16_to_cpu(pi->port_ltp_crc_mode);
	crc_enabled = HFI_LTP_CRC_ENABLED(crc_enabled);

	if (crc_enabled)
		ppd->port_crc_mode_enabled = hfi_port_ltp_to_cap(crc_enabled);

	ls_new = pi->port_states.portphysstate_portstate &
			OPA_PI_MASK_PORT_STATE;
	ps_new = (pi->port_states.portphysstate_portstate &
		OPA_PI_MASK_PORT_PHYSICAL_STATE) >> PHYSPORTSTATE_SHIFT;

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
	ret = set_port_states(dd, ppd, smp, ls_new, ps_new, invalid);
	if (ret)
		return ret;

	/*
	 * Apply the new link downgrade policy.  This may result in a link
	 * bounce.  Do this after everything else so things are settled.
	 * Possible problem: if setting the port state above fails, then
	 * the policy change is not applied.
	 */
	if (call_link_downgrade_policy)
		hfi_apply_link_downgrade_policy(ppd, 0);

	return hfi_reply(ibh);
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

static int __subn_set_hfi_pkeytable(struct hfi_devdata *dd, struct opa_smp *smp,
		u32 am, u8 *data, u8 port, u32 *resp_len, u8 *sma_status)
{
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
		*sma_status = OPA_SMA_FAIL_WITH_NO_DATA;
		return hfi_reply(ibh);
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
			*sma_status = OPA_SMA_FAIL_WITH_NO_DATA;
			return hfi_reply(ibh);
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

	if (pkey_changed)
		(void)hfi_set_ib_cfg(ppd, HFI_IB_CFG_PKEYS, 0, NULL);

	return hfi_reply(ibh);
}

static int __subn_set_hfi_sl_to_sc(struct hfi_devdata *dd, struct opa_smp *smp,
		u32 am, u8 *data, u8 port, u32 *resp_len, u8 *sma_status)
{
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
	*sma_status = OPA_SMA_FAIL_WITH_NO_DATA;
done:
	return hfi_reply(ibh);
}

static int __subn_set_hfi_sc_to_sl(struct hfi_devdata *dd, struct opa_smp *smp,
		u32 am, u8 *data, u8 port, u32 *resp_len, u8 *sma_status)
{
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
	*sma_status = OPA_SMA_FAIL_WITH_NO_DATA;
done:
	return hfi_reply(ibh);
}

static int __subn_set_hfi_sc_to_vlr(struct hfi_devdata *dd, struct opa_smp *smp,
				    u32 am, u8 *data, u8 port, u32 *resp_len,
				    u8 *sma_status)
{
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	u32 n_blocks = OPA_AM_NBLK(am);
	int async_update = OPA_AM_ASYNC(am);
	u8 *p = (void *)data;
	int i, lstate, map_changed = 0;
	int sc_len = ARRAY_SIZE(ppd->sc_to_vlr);

	if (n_blocks != 1 || async_update)
		goto err;

	lstate = hfi_driver_lstate(ppd);

	if (!async_update &&
	    (lstate == IB_PORT_ARMED || lstate == IB_PORT_ACTIVE))
		goto err;

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
	*sma_status = OPA_SMA_FAIL_WITH_NO_DATA;
done:
	return hfi_reply(ibh);
}

static int __subn_set_hfi_sc_to_vlt(struct hfi_devdata *dd, struct opa_smp *smp,
		u32 am, u8 *data, u8 port, u32 *resp_len, u8 *sma_status)
{
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	u32 n_blocks = OPA_AM_NBLK(am);
	int async_update = OPA_AM_ASYNC(am);
	u8 *p = (void *) data;
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
	*sma_status = OPA_SMA_FAIL_WITH_NO_DATA;
done:
	return hfi_reply(ibh);
}

static int __subn_set_hfi_sc_to_vlnt(struct hfi_devdata *dd,
		struct opa_smp *smp, u32 am, u8 *data, u8 port, u32 *resp_len,
								u8 *sma_status)
{
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	u32 n_blocks = OPA_AM_NBLK(am);
	u8 *p = (void *) data;
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
	*sma_status = OPA_SMA_FAIL_WITH_NO_DATA;
done:
	return hfi_reply(ibh);
}

static int __subn_set_hfi_psi(struct hfi_devdata *dd, struct opa_smp *smp,
		u32 am, u8 *data, u8 port, u32 *resp_len, u8 *sma_status)
{
	/* FXRTODO: to be implemented */
	return IB_MAD_RESULT_FAILURE;
}

static int __subn_set_hfi_bct(struct hfi_devdata *dd, struct opa_smp *smp,
		u32 am, u8 *data, u8 port, u32 *resp_len, u8 *sma_status)
{
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	struct buffer_control *p = (struct buffer_control *) data;
	u32 num_ports = OPA_AM_NPORT(am);

	if (num_ports != 1)
		goto fail;

	if (hfi_set_buffer_control(ppd, p) < 0)
		goto fail;

	goto done;
fail:
	hfi_invalid_attr(smp);
	*sma_status = OPA_SMA_FAIL_WITH_NO_DATA;
done:
	return hfi_reply(ibh);
}

/*
 * FXRTODO: This attribute will not be supported in FXR.
 * This will need to be reported as unsupported attribute in
 * the SMPs status field. Refer STL-2109
 */
static int __subn_set_hfi_vl_arb(struct hfi_devdata *dd, struct opa_smp *smp,
		u32 am, u8 *data, u8 port, u32 *resp_len, u8 *sma_status)
{
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, port);
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	u32 num_ports = OPA_AM_NPORT(am);
	int table_size =
		HFI_VL_ARB_TABLE_SIZE * sizeof(struct ib_vl_weight_elem);
	u8 section = (am & 0x00ff0000) >> 16;
	u8 *p = data;

	if (num_ports != 1)
		goto err;

	switch (section) {
	case OPA_VLARB_LOW_ELEMENTS:
		memcpy(ppd->vl_arb_low, p, table_size);
		break;
	case OPA_VLARB_HIGH_ELEMENTS:
		memcpy(ppd->vl_arb_high, p, table_size);
		break;
	/* neither OPA_VLARB_PREEMPT_ELEMENTS, or OPA_VLARB_PREEMPT_MATRIX
	 * can be changed from the default values */
	case OPA_VLARB_PREEMPT_ELEMENTS:
		/* FALLTHROUGH */
	case OPA_VLARB_PREEMPT_MATRIX:
		smp->status |=
		cpu_to_be16(IB_MGMT_MAD_STATUS_UNSUPPORTED_METHOD_ATTRIB);
		*sma_status = OPA_SMA_FAIL_WITH_DATA;
		break;
	default:
		pr_warn("OPA SubnSet(VL Arb) AM Invalid : 0x%x\n",
			be32_to_cpu(smp->attr_mod));
		goto err;
		break;
	}

	goto done;
err:
	hfi_invalid_attr(smp);
	*sma_status = OPA_SMA_FAIL_WITH_NO_DATA;
done:
	return hfi_reply(ibh);
}

static int __subn_set_hfi_cong_setting(struct hfi_devdata *dd,
		struct opa_smp *smp, u32 am, u8 *data, u8 port, u32 *resp_len,
								u8 *sma_status)
{
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	struct opa_congestion_setting_attr *p =
		(struct opa_congestion_setting_attr *) data;
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

	return hfi_reply(ibh);
}

static int __subn_set_hfi_cc_table(struct hfi_devdata *dd, struct opa_smp *smp,
		u32 am, u8 *data, u8 port, u32 *resp_len, u8 *sma_status)
{
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;
	struct ib_cc_table_attr *p = (struct ib_cc_table_attr *) data;
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
		*sma_status = OPA_SMA_FAIL_WITH_NO_DATA;
		goto done;
	}

	sentry = start_block * HFI_IB_CCT_ENTRIES;
	eentry = sentry + ((n_blocks - 1) * HFI_IB_CCT_ENTRIES) +
		 (be16_to_cpu(p->ccti_limit)) % HFI_IB_CCT_ENTRIES + 1;

	/* sanity check ccti_limit */
	ccti_limit = be16_to_cpu(p->ccti_limit);
	if (ccti_limit + 1 > eentry) {
		hfi_invalid_attr(smp);
		*sma_status = OPA_SMA_FAIL_WITH_NO_DATA;
		goto done;
	}

	new_cc_state = kzalloc(sizeof(*new_cc_state), GFP_KERNEL);
	if (new_cc_state == NULL)
		goto done;

	spin_lock(&ppd->cc_state_lock);

	old_cc_state = hfi_get_cc_state(ppd);

	if (old_cc_state == NULL) {
		spin_unlock(&ppd->cc_state_lock);
		kfree(new_cc_state);
		*sma_status = OPA_SMA_FAIL_WITH_NO_DATA;
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
	return hfi_reply(ibh);
}

static int __subn_set_hfi_led_info(struct hfi_devdata *dd, struct opa_smp *smp,
		u32 am, u8 *data, u8 port, u32 *resp_len, u8 *sma_status)
{
	/* FXRTODO: to be implemented */
	return IB_MAD_RESULT_FAILURE;
}

int hfi_set_sma(struct opa_core_device *odev, u16 attr_id, struct opa_smp *smp,
		u32 am, u8 *data, u8 port, u32 *resp_len, u8 *sma_status)
{
	int ret;
	struct ib_mad_hdr *ibh = (struct ib_mad_hdr *)smp;

	switch (attr_id) {
	case IB_SMP_ATTR_PORT_INFO:
		ret = __subn_set_hfi_portinfo(odev->dd, smp, am, data, port,
					      resp_len, sma_status);
		break;
	case IB_SMP_ATTR_PKEY_TABLE:
		ret = __subn_set_hfi_pkeytable(odev->dd, smp, am, data, port,
					       resp_len, sma_status);
		break;
	case OPA_ATTRIB_ID_SL_TO_SC_MAP:
		ret = __subn_set_hfi_sl_to_sc(odev->dd, smp, am, data, port,
					      resp_len, sma_status);
		break;
	case OPA_ATTRIB_ID_SC_TO_SL_MAP:
		ret = __subn_set_hfi_sc_to_sl(odev->dd, smp, am, data, port,
					      resp_len, sma_status);
		break;
	case OPA_ATTRIB_ID_SC_TO_VLR_MAP:
		ret = __subn_set_hfi_sc_to_vlr(odev->dd, smp, am, data, port,
					       resp_len, sma_status);
		break;
	case OPA_ATTRIB_ID_SC_TO_VLT_MAP:
		ret = __subn_set_hfi_sc_to_vlt(odev->dd, smp, am, data, port,
					       resp_len, sma_status);
		break;
	case OPA_ATTRIB_ID_SC_TO_VLNT_MAP:
		ret = __subn_set_hfi_sc_to_vlnt(odev->dd, smp, am, data, port,
					       resp_len, sma_status);
		break;
	case OPA_ATTRIB_ID_PORT_STATE_INFO:
		ret = __subn_set_hfi_psi(odev->dd, smp, am, data, port,
					 resp_len, sma_status);
		break;
	case OPA_ATTRIB_ID_BUFFER_CONTROL_TABLE:
		ret = __subn_set_hfi_bct(odev->dd, smp, am, data, port,
					 resp_len, sma_status);
		break;
	case IB_SMP_ATTR_VL_ARB_TABLE:
		ret = __subn_set_hfi_vl_arb(odev->dd, smp, am, data, port,
					    resp_len, sma_status);
		break;
	case OPA_ATTRIB_ID_HFI_CONGESTION_SETTING:
		ret = __subn_set_hfi_cong_setting(odev->dd, smp, am, data,
						  port, resp_len, sma_status);
		break;
	case OPA_ATTRIB_ID_CONGESTION_CONTROL_TABLE:
		ret = __subn_set_hfi_cc_table(odev->dd, smp, am, data, port,
					      resp_len, sma_status);
		break;
	case IB_SMP_ATTR_LED_INFO:
		ret = __subn_set_hfi_led_info(odev->dd, smp, am, data, port,
					      resp_len, sma_status);
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
		*sma_status = OPA_SMA_FAIL_WITH_NO_DATA;
		ret = hfi_reply(ibh);
		break;
	}

	return ret;
}
