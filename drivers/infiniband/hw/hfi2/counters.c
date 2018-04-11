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

#include "counters.h"
#include "trace.h"
#include "chip/fxr_perfmon_defs.h"

#define FXR_SCNTRS_SHIFT	48

const char *portcntr_names[] = {
	"xmit_data",
	"xmit_pkts",
	"rcv_data",
	"rcv_pkts",
	"mcast_rcvpkts",
	"mcast_xmitpkts",
	"xmit_wait",
	"rcv_fecn",
	"rcv_becn",
	"rcv_constraint_err",
	"remote_physical_err",
	"port_rcv_err",
	"fm_config_err",
	"uncorrectable_err",
	"xmit_data_vl0",
	"xmit_data_vl1",
	"xmit_data_vl2",
	"xmit_data_vl3",
	"xmit_data_vl4",
	"xmit_data_vl5",
	"xmit_data_vl6",
	"xmit_data_vl7",
	"xmit_data_vl8",
	"xmit_data_vl15",
	"xmit_pkts_vl0",
	"xmit_pkts_vl1",
	"xmit_pkts_vl2",
	"xmit_pkts_vl3",
	"xmit_pkts_vl4",
	"xmit_pkts_vl5",
	"xmit_pkts_vl6",
	"xmit_pkts_vl7",
	"xmit_pkts_vl8",
	"xmit_pkts_vl15",
	"rcv_data_vl0",
	"rcv_data_vl1",
	"rcv_data_vl2",
	"rcv_data_vl3",
	"rcv_data_vl4",
	"rcv_data_vl5",
	"rcv_data_vl6",
	"rcv_data_vl7",
	"rcv_data_vl8",
	"rcv_data_vl15",
	"rcv_pkts_vl0",
	"rcv_pkts_vl1",
	"rcv_pkts_vl2",
	"rcv_pkts_vl3",
	"rcv_pkts_vl4",
	"rcv_pkts_vl5",
	"rcv_pkts_vl6",
	"rcv_pkts_vl7",
	"rcv_pkts_vl8",
	"rcv_pkts_vl15",
	"xmit_wait_vl0",
	"xmit_wait_vl1",
	"xmit_wait_vl2",
	"xmit_wait_vl3",
	"xmit_wait_vl4",
	"xmit_wait_vl5",
	"xmit_wait_vl6",
	"xmit_wait_vl7",
	"xmit_wait_vl8",
	"xmit_wait_vl15",
	"rcv_fecn_vl0",
	"rcv_fecn_vl1",
	"rcv_fecn_vl2",
	"rcv_fecn_vl3",
	"rcv_fecn_vl4",
	"rcv_fecn_vl5",
	"rcv_fecn_vl6",
	"rcv_fecn_vl7",
	"rcv_fecn_vl8",
	"rcv_fecn_vl15",
	"rcv_becn_vl0",
	"rcv_becn_vl1",
	"rcv_becn_vl2",
	"rcv_becn_vl3",
	"rcv_becn_vl4",
	"rcv_becn_vl5",
	"rcv_becn_vl6",
	"rcv_becn_vl7",
	"rcv_becn_vl8",
	"rcv_becn_vl15",
	"xmit_constraint_err",
	"loop_pkts",
	"rc_resend",
	"rnr_nak",
	"other_nak",
	"rc_time_out",
	"pkt_drop",
	"rc_seq_nak",
	"rc_dup_rew",
	"rdma_seq",
	"seq_nak",
	"rc_acks",
	"rc_qacks",
	"rc_delay_comp"
};

const char *devcntr_names[] = {
	"txci_stall_hcq_empty_events",
	"txci_stall_rcq_empty_events",
	"txci_stall_cq_empty_events",
	"txci_nonfull_slot_write",
	"txci_stall_otr_credits_w",
	"txci_stall_otr_credits_x",
	"txci_stall_otr_credits_y",
	"txci_stall_otr_credits_z",
	"otr_rsp_special_acks",
	"otr_stall_omb_entries_x",
	"otr_stall_omb_entries_y",
	"otr_stall_opb_entries_x",
	"otr_stall_opb_entries_y",
	"otr_stall_txdma_credits_x",
	"otr_stall_txdma_credits_y",
	"otr_stall_rxdma_credits_x",
	"otr_stall_rxdma_credits_y",
	"otr_stall_cts",
	"otr_opb_stall_mem_credits",
	"otr_opb_stall_mem_rsp_events",
	"otr_opb_stall_mem_rsp_cycles",
	"otr_opb_stall_at_credits",
	"otr_opb_stall_at_rsp_events",
	"otr_opb_stall_at_rsp_cycles",
	"otr_m_to_p_stall_credits_x",
	"otr_m_to_p_stall_credits_y",
	"otr_p_to_m_stall_credits_x",
	"otr_p_to_m_stall_credits_y",
	"otr_prefrag_stall_credits_x",
	"otr_prefrag_stall_credits_y",
	"otr_fastpath_req",
	"otr_fragpe_req",
	"otr_txdma_cmds",
	"otr_messages_opened_x",
	"otr_messages_closed_x",
	"otr_packets_opened_x",
	"otr_packets_closed_x",
	"otr_packets_retrans",
	"otr_latency_0_a",
	"otr_latency_a_b",
	"otr_latency_b_c",
	"otr_latency_c_d",
	"otr_latency_d_max",
	"txdma_tx_reads",
	"txdma_mc1_packet_match0",
	"txdma_mc1_packet_match1",
	"txdma_tx_flits_tcmc_w",
	"txdma_tx_flits_tcmc_x",
	"txdma_tx_flits_tcmc_y",
	"txdma_tx_flits_tcmc_z",
	"txdma_stall_mem_credits",
	"txdma_stall_mem_rsp_events",
	"txdma_stall_mem_rsp_cycles",
	"txdma_stall_at_credits",
	"txdma_stall_at_rsp_events",
	"txdma_stall_at_rsp_cycles",
	"txdma_stall_empty_events_x",
	"txdma_stall_empty_events_y",
	"txdma_stall_credits_x",
	"txdma_stall_credits_y",
	"txdma_mc0_packet_match0",
	"txdma_mc0_packet_match1",
	"txdma_mc0_packet_match2",
	"txdma_mc0_packet_match3",
	"rxci_stall_cq_empty_events",
	"rxci_stall_otr_empty_events",
	"rxci_nonfull_slot_write",
	"rxci_stall_rxhp_credits",
	"rxci_cmds",
	"otr_retrans_limit_rchd_x",
	"otr_cleanup_cmd_detect",
	"otr_local_seq_stall_x",
	"rxdma_rx_writes",
	"rxdma_rx_reads",
	"rxdma_rx_flits_tcmc_x",
	"rxdma_rx_flits_tcmc_y",
	"rxdma_stall_mem_credits_cycles",
	"rxdma_stall_mem_rsp_events",
	"rxdma_stall_mem_rsp_cycles",
	"rxdma_stall_empty_events_x",
	"rxdma_stall_empty_events_y",
	"rxdma_stall_credits_x",
	"rxdma_stall_credits_y",
	"rxdma_stall_no_cmd_x",
	"rxdma_stall_no_cmd_y",
	"rxdma_stall_no_txci_cred_x",
	"rxdma_stall_no_txci_cred_y",
	"rxdma_stall_no_txdma_cred_x",
	"rxdma_stall_no_txdma_cred_y",
	"rxdma_stall_to_cred_x",
	"rxdma_stall_to_cred_y",
	"rxdma_stall_tid_full_x",
	"rxdma_stall_tid_full_y",
	"rxdma_stall_ack_full_x",
	"rxdma_stall_ack_full_y",
	"rxdma_stall_war_full_x",
	"rxdma_stall_war_full_y",
	"rxdma_trig_op_issue_x",
	"rxdma_trig_op_issue_y",
	"rxdma_trig_op_queue_x",
	"rxdma_trig_op_queue_y",
	"rxdma_trig_op_append",
	"rxhiarb_rxdma_reqs",
	"rxhiarb_rxcid_reqs",
	"rxhiarb_rxhp_reqs",
	"rxhiarb_rxet_reqs",
	"rxhiarb_rxe2e_reqs",
	"rxhiarb_hi_flits",
	"rxhiarb_hi_stall_cycles",
	"rxhiarb_hi_stall_events",
	"rxhiarb_at_flush",
	"rxhiarb_at_flush_stalls",
	"rxhiarb_stall_at_req_events",
	"rxhiarb_stall_at_req_cycles",
	"rxhiarb_stall_at_rsp_events",
	"rxhiarb_stall_at_rsp_cycles",
	"rxhiarb_tlb_hits",
	"rxhiarb_tlb_present_hits",
	"rxhiarb_slow_resp_stalls",
	"rxhiarb_resp_cnt_rxdma",
	"rxhiarb_resp_cnt_rxhp",
	"rxhiarb_resp_cnt_rxet",
	"rxhiarb_resp_cnt_rxe2e",
	"rxhiarb_pcb_lookups",
	"rxhiarb_stall_hq_events",
	"rxhiarb_stall_hq_cycles",
	"rxhiarb_at_match_cnt",
	"rxhiarb_at_match_dly",
	"rxhiarb_mrd_match_cnt",
	"rxhiarb_mrd_match_dly",
	"hi_pcim_hpi_req_cnt",
	"hi_pcim_p2sb_np_req_cnt",
	"hi_pcim_p2sb_pc_req_cnt",
	"hi_pcim_iommu_req_cnt",
	"hi_pcim_ur_req_cnt",
	"hi_pcim_hpi_splt_req_cnt",
	"hi_pcim_p2sb_splt_req_cnt",
	"hi_pcim_hpi_fc_cnt",
	"hi_pcim_p2sb_fc_cnt",
	"hi_pcim_marb_p_fc_cnt",
	"hi_pcim_marb_np_fc_cnt",
	"hi_pcim_marb_cpl_fc_cnt",
	"hi_pcim_marb_p_req_cnt",
	"hi_pcim_marb_np_req_cnt",
	"hi_pcim_marb_cpl_req_cnt",
	"hi_pcim_sb2p_p_req_cnt",
	"hi_pcim_sb2p_np_req_cnt",
	"hi_pcim_sb2p_cpl_req_cnt",
	"hi_pcim_io2p_np_req_cnt",
	"hi_pcim_init_p_req_cnt",
	"hi_pcim_hpi_cpl_req_cnt",
	"rxhp_fp_ects",
	"rxhp_pe_cts",
	"rxhp_pe_ects",
	"rxhp_fastpath",
	"rxhp_pe0",
	"rxhp_pe1",
	"rxhp_pe2",
	"rxhp_pe3",
	"rxhp_pe4",
	"rxhp_pe5",
	"rxhp_pe6",
	"rxhp_pe7",
	"rxhp_pe0_idle",
	"rxhp_pe1_idle",
	"rxhp_pe2_idle",
	"rxhp_pe3_idle",
	"rxhp_pe4_idle",
	"rxhp_pe5_idle",
	"rxhp_pe6_idle",
	"rxhp_pe7_idle",
	"rxhp_ni0",
	"rxhp_ni1",
	"rxhp_ni2",
	"rxhp_ni3",
	"rxhp_put",
	"rxhp_get",
	"rxhp_atomic",
	"rxhp_stall_rxdma_credits_x",
	"rxhp_stall_rxdma_credits_y",
	"rxhp_pid_conflict_stall",
	"rxhp_pe_busy_stall",
	"rxhp_psc0_req_stall",
	"rxhp_psc1_req_stall",
	"rxe2e_no_big_scoreboard",
	"rxe2e_beyond_scoreboard",
	"rxe2e_psn_dist_0_31",
	"rxe2e_psn_dist_32_63",
	"rxe2e_psn_dist_64_127",
	"rxe2e_psn_dist_128_255",
	"rxe2e_psn_dist_256_511",
	"rxe2e_psn_dist_512_767",
	"rxe2e_psn_dist_768_plus",
	"rxe2e_mc0_packet_match0",
	"rxe2e_mc0_packet_match1",
	"rxe2e_mc0_packet_match2",
	"rxe2e_mc0_packet_match3",
	"rxe2e_mc1_packet_match0",
	"rxe2e_mc1_packet_match1",
	"lm_fecn_rcvd_pt0",
	"lm_becn_rcvd_pt0",
	"lm_pkts_sent_pt0",
	"lm_pkts_rcvd_pt0",
	"lm_stall_rm_fifo_pt0",
	"lm_byp_pkt_x",
	"lm_byp_pkt_y",
	"lm_byp_pkt_z",
	"lm_byp_qflit_x",
	"lm_byp_qflit_y",
	"lm_byp_qflit_z",
	"lm_byp_wait_x",
	"lm_byp_wait_y",
	"lm_byp_wait_z",
	"at_tlb_accesses",
	"at_tlb_hits_1g",
	"at_tlb_miss_1g",
	"at_tlb_evicts_1g",
	"at_tlb_hits_2m",
	"at_tlb_miss_2m",
	"at_tlb_evicts_2m",
	"at_tlb_hits_4k",
	"at_tlb_miss_4k",
	"at_tlb_evicts_4k",
	"at_tlb_req_stall",
	"at_tlb_shootdowns",
	"txpsn_hits",
	"txpsn_miss",
	"txpsn_fill_stall",
	"txpsn_evict_stall",
	"txpsn_evicts",
	"txpsn_membus_stall",
	"rxpsn_hits",
	"rxpsn_miss",
	"rxpsn_fill_stall",
	"rxpsn_evict_stall",
	"rxpsn_evicts",
	"rxpsn_membus_stall",
	"pte_cache_hits",
	"pte_cache_miss",
	"pte_cache_fill_stall",
	"pte_cache_evict_stall",
	"pte_cache_evicts",
	"pte_cache_membus_stall",
	"eqd_cache_hits",
	"eqd_cache_miss",
	"eqd_cache_fill_stall",
	"eqd_cache_evict_stall",
	"eqd_cache_evicts",
	"eqd_cache_membus_stall",
	"trig_op_cache_hits",
	"trig_op_cache_miss",
	"trig_op_cache_fill_stall",
	"trig_op_cache_evict_stall",
	"trig_op_cache_evicts",
	"trig_op_cache_membus_stall",
	"psc0_cache_hits",
	"psc0_cache_miss",
	"psc0_cache_fill_stall",
	"psc0_cache_evict_stall",
	"psc0_cache_evicts",
	"psc0_cache_membus_stall",
	"psc1_cache_hits",
	"psc1_cache_miss",
	"psc1_cache_fill_stall",
	"psc1_cache_evict_stall",
	"psc1_cache_evicts",
	"psc1_cache_membus_stall"
};

int num_dev_cntrs = ARRAY_SIZE(devcntr_names);
int num_port_cntrs = ARRAY_SIZE(portcntr_names);

static u64 hfi_port_access_tp_csr(const struct cntr_entry *entry, void *context,
				  int vl)
{
	struct hfi_pportdata *ppd = (struct hfi_pportdata *)context;
	u64 csr = entry->csr;

	if (entry->flags & CNTR_VL) {
		return hfi_read_lm_tp_prf_csr(ppd,
		csr + idx_from_vl(vl) + 1);
	} else {
		return hfi_read_lm_tp_prf_csr(ppd, csr);
	}
}

static u64 hfi_port_access_fpc_csr(const struct cntr_entry *entry,
				   void *context, int vl)
{
	struct hfi_pportdata *ppd = (struct hfi_pportdata *)context;
	u64 csr = entry->csr;

	if (entry->flags & CNTR_VL) {
		return hfi_read_lm_fpc_prf_per_vl_csr(ppd,
		csr, idx_from_vl(vl));
	} else {
		return read_csr(ppd->dd, csr);
	}
}

static u64 hfi_access_pmon_csr(const struct hfi_devdata *dd, u32 index)
{
	u64 counter_value = hfi_read_pmon_csr(dd, index);
	u64 scntr_value = (u64)(*(dd->scntrs + index));

	return ((counter_value & FXR_PMON_CFG_ARRAY_COUNT_MASK)
		| scntr_value << FXR_SCNTRS_SHIFT);
}

#define DEV_CNTR_ELEM(name, index, offset, accessor)\
{ \
	name, \
	index, \
	offset, \
	accessor \
}

#define CNTR_ELEM(name, csr, offset, flags, accessor)\
{ \
	name, \
	csr, \
	offset, \
	flags, \
	accessor \
}

#define TP_CNTR(name, counter, flags) \
CNTR_ELEM(#name, \
	  counter, \
	  0, \
	  flags, \
	  hfi_port_access_tp_csr)

#define FPC_CNTR(name, counter, flags) \
CNTR_ELEM(#name, \
	  counter, \
	  0, \
	  flags, \
	  hfi_port_access_fpc_csr)

#define PERFMON_CNTR(name, index) \
DEV_CNTR_ELEM(#name, \
	  index, \
	  0, \
	  hfi_access_pmon_csr)

#define SW_IBP_CNTR(name, counter) \
CNTR_ELEM(#name, \
	  0, \
	  0, \
	  CNTR_NORMAL, \
	  access_ibp_##counter)

#define def_access_sw_cpu(cntr) \
static u64 access_sw_cpu_##cntr(const struct cntr_entry *entry,	\
				void *context, int vl) \
{ \
	struct hfi_pportdata *ppd = (struct hfi_pportdata *)context; \
	struct hfi2_ibport *ibp = ppd->dd->ibd->pport;	\
	return hfi_read_per_cpu_counter(&ibp->rvp.z_ ##cntr, ibp->rvp.cntr); \
}

def_access_sw_cpu(rc_acks);
def_access_sw_cpu(rc_qacks);
def_access_sw_cpu(rc_delayed_comp);

#define def_access_ibp_counter(cntr) \
static u64 access_ibp_##cntr(const struct cntr_entry *entry, \
			     void *context, int vl) \
{ \
	struct hfi_pportdata *ppd = (struct hfi_pportdata *)context; \
	struct hfi2_ibdev *ibd = ppd->dd->ibd;	\
	struct hfi2_ibport *ibp = ibd->pport; \
	return ibp->rvp.n_ ##cntr; \
}

def_access_ibp_counter(loop_pkts);
def_access_ibp_counter(rc_resends);
def_access_ibp_counter(rnr_naks);
def_access_ibp_counter(other_naks);
def_access_ibp_counter(rc_timeouts);
def_access_ibp_counter(pkt_drops);
def_access_ibp_counter(rc_seqnak);
def_access_ibp_counter(rc_dupreq);
def_access_ibp_counter(rdma_seq);
def_access_ibp_counter(seq_naks);

static struct cntr_entry port_cntrs[PORT_CNTR_LAST] = {
[XMIT_DATA] = TP_CNTR(XmitFlits, TP_PRF_XMIT_DATA, CNTR_NORMAL),
[XMIT_PKTS] = TP_CNTR(XmitPkts, TP_PRF_XMIT_PKTS, CNTR_NORMAL),
[RCV_DATA] = FPC_CNTR(RcvFlits, FXR_FPC_PRF_PORTRCV_DATA_CNT, CNTR_NORMAL),
[RCV_PKTS] = FPC_CNTR(RcvPkts, FXR_FPC_PRF_PORTRCV_PKT_CNT, CNTR_NORMAL),
[MCAST_RCVPKTS] = FPC_CNTR(RcvMCastPkts, FXR_FPC_PRF_PORTRCV_MCAST_PKT_CNT,
			   CNTR_NORMAL),
[MCAST_XMITPKTS] = TP_CNTR(XmitMCastPkts, TP_PRF_MULTICAST_XMIT_PKTS,
			   CNTR_NORMAL),
[XMIT_WAIT] = TP_CNTR(XmitWait, TP_PRF_XMIT_WAIT, CNTR_NORMAL),
[XMIT_CONSTRAINT_ERR] = TP_CNTR(XmitConstriantErr,
				TP_ERR_XMIT_CONSTRAINT_ERROR,
				CNTR_NORMAL),
[RCV_FECN] = FPC_CNTR(RcvFecn, FXR_FPC_PRF_PORTRCV_FECN, CNTR_NORMAL),
[RCV_BECN] = FPC_CNTR(RcvBecn, FXR_FPC_PRF_PORTRCV_BECN, CNTR_NORMAL),
[RCV_CONSTRAINT_ERR] = FPC_CNTR(RcvConstraintErr,
				FXR_FPC_ERR_PORTRCV_CONSTRAINT_ERROR,
				CNTR_NORMAL),
[REMOTE_PHYSICAL_ERR] = FPC_CNTR(RemotePhyErr,
				FXR_FPC_ERR_PORTRCV_PHY_REMOTE_ERROR,
				CNTR_NORMAL),
[PORT_RCV_ERR] = FPC_CNTR(PortRcvErr, FXR_FPC_ERR_PORTRCV_ERROR,
			  CNTR_NORMAL),
[FM_CONFIG_ERR] = FPC_CNTR(FmConfigErr, FXR_FPC_ERR_FMCONFIG_ERROR,
			   CNTR_NORMAL),
[UNCORRECTABLE_ERR] = FPC_CNTR(UnCorrectableErr,
			       FXR_FPC_ERR_UNCORRECTABLE_ERROR,
			       CNTR_NORMAL),
[XMIT_DATA_VL] = TP_CNTR(XmitFlits, TP_PRF_XMIT_DATA, CNTR_VL),
[XMIT_PKTS_VL] = TP_CNTR(XmitPkts, TP_PRF_XMIT_PKTS, CNTR_VL),
[RCV_DATA_VL] = FPC_CNTR(RcvFlits, FXR_FPC_PRF_PORT_VL_RCV_DATA_CNT, CNTR_VL),
[RCV_PKTS_VL] = FPC_CNTR(RcvPkts, FXR_FPC_PRF_PORT_VL_RCV_PKT_CNT, CNTR_VL),
[XMIT_WAIT_VL] = TP_CNTR(XmitWait, TP_PRF_XMIT_WAIT, CNTR_VL),
[RCV_FECN_VL] = FPC_CNTR(RcvFecn, FXR_FPC_PRF_PORT_VL_RCV_FECN, CNTR_VL),
[RCV_BECN_VL] = FPC_CNTR(RcvBecn, FXR_FPC_PRF_PORT_VL_RCV_BECN, CNTR_VL),
[SW_IBP_LOOP_PKTS] = SW_IBP_CNTR(LoopPkts, loop_pkts),
[SW_IBP_RC_RESENDS] = SW_IBP_CNTR(RcResend, rc_resends),
[SW_IBP_RNR_NAKS] = SW_IBP_CNTR(RnrNak, rnr_naks),
[SW_IBP_OTHER_NAKS] = SW_IBP_CNTR(OtherNak, other_naks),
[SW_IBP_RC_TIMEOUTS] = SW_IBP_CNTR(RcTimeOut, rc_timeouts),
[SW_IBP_PKT_DROPS] = SW_IBP_CNTR(PktDrop, pkt_drops),
[SW_IBP_RC_SEQNAK] = SW_IBP_CNTR(RcSeqNak, rc_seqnak),
[SW_IBP_RC_DUPREQ] = SW_IBP_CNTR(RcDupRew, rc_dupreq),
[SW_IBP_RDMA_SEQ] = SW_IBP_CNTR(RdmaSeq, rdma_seq),
[SW_IBP_SEQ_NAK] = SW_IBP_CNTR(SeqNak, seq_naks),
[SW_CPU_RC_ACKS] = CNTR_ELEM("RcAcks", 0, 0, CNTR_NORMAL,
			       access_sw_cpu_rc_acks),
[SW_CPU_RC_QACKS] = CNTR_ELEM("RcQacks", 0, 0, CNTR_NORMAL,
				access_sw_cpu_rc_qacks),
[SW_CPU_RC_DELAYED_COMP] = CNTR_ELEM("RcDelayComp", 0, 0, CNTR_NORMAL,
				       access_sw_cpu_rc_delayed_comp),

};

static struct devcntr_entry dev_cntrs[NUM_DEV_CNTRS] = {
PERFMON_CNTR(txci_stall_hcq_empty_events, 0x0),
PERFMON_CNTR(txci_stall_rcq_empty_events, 0x1),
PERFMON_CNTR(txci_stall_cq_empty_events, 0x2),
PERFMON_CNTR(txci_nonfull_slot_write, 0x8),
PERFMON_CNTR(txci_stall_otr_credits_w, 0x9),
PERFMON_CNTR(txci_stall_otr_credits_x, 0xa),
PERFMON_CNTR(txci_stall_otr_credits_y, 0xb),
PERFMON_CNTR(txci_stall_otr_credits_z, 0xc),
PERFMON_CNTR(otr_rsp_special_acks, 0xf),
PERFMON_CNTR(otr_stall_omb_entries_x, 0x10),
PERFMON_CNTR(otr_stall_omb_entries_y, 0x11),
PERFMON_CNTR(otr_stall_opb_entries_x, 0x12),
PERFMON_CNTR(otr_stall_opb_entries_y, 0x13),
PERFMON_CNTR(otr_stall_txdma_credits_x, 0x14),
PERFMON_CNTR(otr_stall_txdma_credits_y, 0x15),
PERFMON_CNTR(otr_stall_rxdma_credits_x, 0x16),
PERFMON_CNTR(otr_stall_rxdma_credits_y, 0x17),
PERFMON_CNTR(otr_stall_cts, 0x18),
PERFMON_CNTR(otr_opb_stall_mem_credits, 0x19),
PERFMON_CNTR(otr_opb_stall_mem_rsp_events, 0x1a),
PERFMON_CNTR(otr_opb_stall_mem_rsp_cycles, 0x1b),
PERFMON_CNTR(otr_opb_stall_at_credits, 0x1c),
PERFMON_CNTR(otr_opb_stall_at_rsp_events, 0x1d),
PERFMON_CNTR(otr_opb_stall_at_rsp_cycles, 0x1e),
PERFMON_CNTR(otr_m_to_p_stall_credits_x, 0x1f),
PERFMON_CNTR(otr_m_to_p_stall_credits_y, 0x20),
PERFMON_CNTR(otr_p_to_m_stall_credits_x, 0x21),
PERFMON_CNTR(otr_p_to_m_stall_credits_y, 0x22),
PERFMON_CNTR(otr_prefrag_stall_credits_x, 0x23),
PERFMON_CNTR(otr_prefrag_stall_credits_y, 0x24),
PERFMON_CNTR(otr_fastpath_req, 0x25),
PERFMON_CNTR(otr_fragpe_req, 0x26),
PERFMON_CNTR(otr_txdma_cmds, 0x27),
PERFMON_CNTR(otr_messages_opened_x, 0x28),
PERFMON_CNTR(otr_messages_closed_x, 0x29),
PERFMON_CNTR(otr_packets_opened_x, 0x2a),
PERFMON_CNTR(otr_packets_closed_x, 0x2b),
PERFMON_CNTR(otr_packets_retrans, 0x2c),
PERFMON_CNTR(otr_latency_0_a, 0x2d),
PERFMON_CNTR(otr_latency_a_b, 0x2e),
PERFMON_CNTR(otr_latency_b_c, 0x2f),
PERFMON_CNTR(otr_latency_c_d, 0x30),
PERFMON_CNTR(otr_latency_d_max, 0x31),
PERFMON_CNTR(txdma_tx_reads, 0x32),
PERFMON_CNTR(txdma_mc1_packet_match0, 0x33),
PERFMON_CNTR(txdma_mc1_packet_match1, 0x34),
PERFMON_CNTR(txdma_tx_flits_tcmc_w, 0x35),
PERFMON_CNTR(txdma_tx_flits_tcmc_x, 0x36),
PERFMON_CNTR(txdma_tx_flits_tcmc_y, 0x37),
PERFMON_CNTR(txdma_tx_flits_tcmc_z, 0x38),
PERFMON_CNTR(txdma_stall_mem_credits, 0x39),
PERFMON_CNTR(txdma_stall_mem_rsp_events, 0x3a),
PERFMON_CNTR(txdma_stall_mem_rsp_cycles, 0x3b),
PERFMON_CNTR(txdma_stall_at_credits, 0x3c),
PERFMON_CNTR(txdma_stall_at_rsp_events, 0x3d),
PERFMON_CNTR(txdma_stall_at_rsp_cycles, 0x3e),
PERFMON_CNTR(txdma_stall_empty_events_x, 0x3f),
PERFMON_CNTR(txdma_stall_empty_events_y, 0x40),
PERFMON_CNTR(txdma_stall_credits_x, 0x41),
PERFMON_CNTR(txdma_stall_credits_y, 0x42),
PERFMON_CNTR(txdma_mc0_packet_match0, 0x43),
PERFMON_CNTR(txdma_mc0_packet_match1, 0x44),
PERFMON_CNTR(txdma_mc0_packet_match2, 0x45),
PERFMON_CNTR(txdma_mc0_packet_match3, 0x46),
PERFMON_CNTR(rxci_stall_cq_empty_events, 0x48),
PERFMON_CNTR(rxci_stall_otr_empty_events, 0x49),
PERFMON_CNTR(rxci_nonfull_slot_write, 0x4a),
PERFMON_CNTR(rxci_stall_rxhp_credits, 0x4b),
PERFMON_CNTR(rxci_cmds, 0x4c),
PERFMON_CNTR(otr_retrans_limit_rchd_x, 0x4d),
PERFMON_CNTR(otr_cleanup_cmd_detect, 0x4e),
PERFMON_CNTR(otr_local_seq_stall_x, 0x4f),
PERFMON_CNTR(rxdma_rx_writes, 0x50),
PERFMON_CNTR(rxdma_rx_reads, 0x51),
PERFMON_CNTR(rxdma_rx_flits_tcmc_x, 0x52),
PERFMON_CNTR(rxdma_rx_flits_tcmc_y, 0x53),
PERFMON_CNTR(rxdma_stall_mem_credits_cycles, 0x54),
PERFMON_CNTR(rxdma_stall_mem_rsp_events, 0x55),
PERFMON_CNTR(rxdma_stall_mem_rsp_cycles, 0x56),
PERFMON_CNTR(rxdma_stall_empty_events_x, 0x58),
PERFMON_CNTR(rxdma_stall_empty_events_y, 0x59),
PERFMON_CNTR(rxdma_stall_credits_x, 0x5a),
PERFMON_CNTR(rxdma_stall_credits_y, 0x5b),
PERFMON_CNTR(rxdma_stall_no_cmd_x, 0x5c),
PERFMON_CNTR(rxdma_stall_no_cmd_y, 0x5d),
PERFMON_CNTR(rxdma_stall_no_txci_cred_x, 0x5e),
PERFMON_CNTR(rxdma_stall_no_txci_cred_y, 0x5f),
PERFMON_CNTR(rxdma_stall_no_txdma_cred_x, 0x60),
PERFMON_CNTR(rxdma_stall_no_txdma_cred_y, 0x61),
PERFMON_CNTR(rxdma_stall_to_cred_x, 0x62),
PERFMON_CNTR(rxdma_stall_to_cred_y, 0x63),
PERFMON_CNTR(rxdma_stall_tid_full_x, 0x64),
PERFMON_CNTR(rxdma_stall_tid_full_y, 0x65),
PERFMON_CNTR(rxdma_stall_ack_full_x, 0x66),
PERFMON_CNTR(rxdma_stall_ack_full_y, 0x67),
PERFMON_CNTR(rxdma_stall_war_full_x, 0x68),
PERFMON_CNTR(rxdma_stall_war_full_y, 0x69),
PERFMON_CNTR(rxdma_trig_op_issue_x, 0x6a),
PERFMON_CNTR(rxdma_trig_op_issue_y, 0x6b),
PERFMON_CNTR(rxdma_trig_op_queue_x, 0x6c),
PERFMON_CNTR(rxdma_trig_op_queue_y, 0x6d),
PERFMON_CNTR(rxdma_trig_op_append, 0x6e),
PERFMON_CNTR(rxhiarb_rxdma_reqs, 0x80),
PERFMON_CNTR(rxhiarb_rxcid_reqs, 0x81),
PERFMON_CNTR(rxhiarb_rxhp_reqs, 0x82),
PERFMON_CNTR(rxhiarb_rxet_reqs, 0x83),
PERFMON_CNTR(rxhiarb_rxe2e_reqs, 0x84),
PERFMON_CNTR(rxhiarb_hi_flits, 0x85),
PERFMON_CNTR(rxhiarb_hi_stall_cycles, 0x86),
PERFMON_CNTR(rxhiarb_hi_stall_events, 0x87),
PERFMON_CNTR(rxhiarb_at_flush, 0x88),
PERFMON_CNTR(rxhiarb_at_flush_stalls, 0x89),
PERFMON_CNTR(rxhiarb_stall_at_req_events, 0x8a),
PERFMON_CNTR(rxhiarb_stall_at_req_cycles, 0x8b),
PERFMON_CNTR(rxhiarb_stall_at_rsp_events, 0x8c),
PERFMON_CNTR(rxhiarb_stall_at_rsp_cycles, 0x8d),
PERFMON_CNTR(rxhiarb_tlb_hits, 0x8e),
PERFMON_CNTR(rxhiarb_tlb_present_hits, 0x8f),
PERFMON_CNTR(rxhiarb_slow_resp_stalls, 0x90),
PERFMON_CNTR(rxhiarb_resp_cnt_rxdma, 0x91),
PERFMON_CNTR(rxhiarb_resp_cnt_rxhp, 0x92),
PERFMON_CNTR(rxhiarb_resp_cnt_rxet, 0x93),
PERFMON_CNTR(rxhiarb_resp_cnt_rxe2e, 0x94),
PERFMON_CNTR(rxhiarb_pcb_lookups, 0x95),
PERFMON_CNTR(rxhiarb_stall_hq_events, 0x97),
PERFMON_CNTR(rxhiarb_stall_hq_cycles, 0x98),
PERFMON_CNTR(rxhiarb_at_match_cnt, 0x99),
PERFMON_CNTR(rxhiarb_at_match_dly, 0x9a),
PERFMON_CNTR(rxhiarb_mrd_match_cnt, 0x9c),
PERFMON_CNTR(rxhiarb_mrd_match_dly, 0x9d),
PERFMON_CNTR(hi_pcim_hpi_req_cnt, 0x110),
PERFMON_CNTR(hi_pcim_p2sb_np_req_cnt, 0x111),
PERFMON_CNTR(hi_pcim_p2sb_pc_req_cnt, 0x112),
PERFMON_CNTR(hi_pcim_iommu_req_cnt, 0x113),
PERFMON_CNTR(hi_pcim_ur_req_cnt, 0x114),
PERFMON_CNTR(hi_pcim_hpi_splt_req_cnt, 0x115),
PERFMON_CNTR(hi_pcim_p2sb_splt_req_cnt, 0x116),
PERFMON_CNTR(hi_pcim_hpi_fc_cnt, 0x117),
PERFMON_CNTR(hi_pcim_p2sb_fc_cnt, 0x118),
PERFMON_CNTR(hi_pcim_marb_p_fc_cnt, 0x119),
PERFMON_CNTR(hi_pcim_marb_np_fc_cnt, 0x11a),
PERFMON_CNTR(hi_pcim_marb_cpl_fc_cnt, 0x11b),
PERFMON_CNTR(hi_pcim_marb_p_req_cnt, 0x11c),
PERFMON_CNTR(hi_pcim_marb_np_req_cnt, 0x11d),
PERFMON_CNTR(hi_pcim_marb_cpl_req_cnt, 0x11e),
PERFMON_CNTR(hi_pcim_sb2p_p_req_cnt, 0x11f),
PERFMON_CNTR(hi_pcim_sb2p_np_req_cnt, 0x120),
PERFMON_CNTR(hi_pcim_sb2p_cpl_req_cnt, 0x121),
PERFMON_CNTR(hi_pcim_io2p_np_req_cnt, 0x122),
PERFMON_CNTR(hi_pcim_init_p_req_cnt, 0x123),
PERFMON_CNTR(hi_pcim_hpi_cpl_req_cnt, 0x124),
PERFMON_CNTR(rxhp_fp_ects, 0x13d),
PERFMON_CNTR(rxhp_pe_cts, 0x13e),
PERFMON_CNTR(rxhp_pe_ects, 0x13f),
PERFMON_CNTR(rxhp_fastpath, 0x140),
PERFMON_CNTR(rxhp_pe0, 0x141),
PERFMON_CNTR(rxhp_pe1, 0x142),
PERFMON_CNTR(rxhp_pe2, 0x143),
PERFMON_CNTR(rxhp_pe3, 0x144),
PERFMON_CNTR(rxhp_pe4, 0x145),
PERFMON_CNTR(rxhp_pe5, 0x146),
PERFMON_CNTR(rxhp_pe6, 0x147),
PERFMON_CNTR(rxhp_pe7, 0x148),
PERFMON_CNTR(rxhp_pe0_idle, 0x149),
PERFMON_CNTR(rxhp_pe1_idle, 0x14a),
PERFMON_CNTR(rxhp_pe2_idle, 0x14b),
PERFMON_CNTR(rxhp_pe3_idle, 0x14c),
PERFMON_CNTR(rxhp_pe4_idle, 0x14d),
PERFMON_CNTR(rxhp_pe5_idle, 0x14e),
PERFMON_CNTR(rxhp_pe6_idle, 0x14f),
PERFMON_CNTR(rxhp_pe7_idle, 0x150),
PERFMON_CNTR(rxhp_ni0, 0x151),
PERFMON_CNTR(rxhp_ni1, 0x152),
PERFMON_CNTR(rxhp_ni2, 0x153),
PERFMON_CNTR(rxhp_ni3, 0x154),
PERFMON_CNTR(rxhp_put, 0x155),
PERFMON_CNTR(rxhp_get, 0x156),
PERFMON_CNTR(rxhp_atomic, 0x157),
PERFMON_CNTR(rxhp_stall_rxdma_credits_x, 0x158),
PERFMON_CNTR(rxhp_stall_rxdma_credits_y, 0x159),
PERFMON_CNTR(rxhp_pid_conflict_stall, 0x15a),
PERFMON_CNTR(rxhp_pe_busy_stall, 0x15b),
PERFMON_CNTR(rxhp_psc0_req_stall, 0x15c),
PERFMON_CNTR(rxhp_psc1_req_stall, 0x15d),
PERFMON_CNTR(rxe2e_no_big_scoreboard, 0x160),
PERFMON_CNTR(rxe2e_beyond_scoreboard, 0x161),
PERFMON_CNTR(rxe2e_psn_dist_0_31, 0x162),
PERFMON_CNTR(rxe2e_psn_dist_32_63, 0x163),
PERFMON_CNTR(rxe2e_psn_dist_64_127, 0x164),
PERFMON_CNTR(rxe2e_psn_dist_128_255, 0x165),
PERFMON_CNTR(rxe2e_psn_dist_256_511, 0x166),
PERFMON_CNTR(rxe2e_psn_dist_512_767, 0x167),
PERFMON_CNTR(rxe2e_psn_dist_768_plus, 0x168),
PERFMON_CNTR(rxe2e_mc0_packet_match0, 0x169),
PERFMON_CNTR(rxe2e_mc0_packet_match1, 0x16a),
PERFMON_CNTR(rxe2e_mc0_packet_match2, 0x16b),
PERFMON_CNTR(rxe2e_mc0_packet_match3, 0x16c),
PERFMON_CNTR(rxe2e_mc1_packet_match0, 0x16d),
PERFMON_CNTR(rxe2e_mc1_packet_match1, 0x16e),
PERFMON_CNTR(lm_fecn_rcvd_pt0, 0x170),
PERFMON_CNTR(lm_becn_rcvd_pt0, 0x171),
PERFMON_CNTR(lm_pkts_sent_pt0, 0x172),
PERFMON_CNTR(lm_pkts_rcvd_pt0, 0x173),
PERFMON_CNTR(lm_stall_rm_fifo_pt0, 0x174),
PERFMON_CNTR(lm_byp_pkt_x, 0x175),
PERFMON_CNTR(lm_byp_pkt_y, 0x176),
PERFMON_CNTR(lm_byp_pkt_z, 0x177),
PERFMON_CNTR(lm_byp_qflit_x, 0x178),
PERFMON_CNTR(lm_byp_qflit_y, 0x179),
PERFMON_CNTR(lm_byp_qflit_z, 0x17a),
PERFMON_CNTR(lm_byp_wait_x, 0x17b),
PERFMON_CNTR(lm_byp_wait_y, 0x17c),
PERFMON_CNTR(lm_byp_wait_z, 0x17d),
PERFMON_CNTR(at_tlb_accesses, 0x180),
PERFMON_CNTR(at_tlb_hits_1g, 0x181),
PERFMON_CNTR(at_tlb_miss_1g, 0x182),
PERFMON_CNTR(at_tlb_evicts_1g, 0x183),
PERFMON_CNTR(at_tlb_hits_2m, 0x184),
PERFMON_CNTR(at_tlb_miss_2m, 0x185),
PERFMON_CNTR(at_tlb_evicts_2m, 0x186),
PERFMON_CNTR(at_tlb_hits_4k, 0x187),
PERFMON_CNTR(at_tlb_miss_4k, 0x188),
PERFMON_CNTR(at_tlb_evicts_4k, 0x189),
PERFMON_CNTR(at_tlb_req_stall, 0x18a),
PERFMON_CNTR(at_tlb_shootdowns, 0x18b),
PERFMON_CNTR(txpsn_hits, 0x190),
PERFMON_CNTR(txpsn_miss, 0x191),
PERFMON_CNTR(txpsn_fill_stall, 0x192),
PERFMON_CNTR(txpsn_evict_stall, 0x193),
PERFMON_CNTR(txpsn_evicts, 0x194),
PERFMON_CNTR(txpsn_membus_stall, 0x195),
PERFMON_CNTR(rxpsn_hits, 0x196),
PERFMON_CNTR(rxpsn_miss, 0x197),
PERFMON_CNTR(rxpsn_fill_stall, 0x198),
PERFMON_CNTR(rxpsn_evict_stall, 0x199),
PERFMON_CNTR(rxpsn_evicts, 0x19a),
PERFMON_CNTR(rxpsn_membus_stall, 0x19b),
PERFMON_CNTR(pte_cache_hits, 0x19c),
PERFMON_CNTR(pte_cache_miss, 0x19d),
PERFMON_CNTR(pte_cache_fill_stall, 0x19e),
PERFMON_CNTR(pte_cache_evict_stall, 0x19f),
PERFMON_CNTR(pte_cache_evicts, 0x1a0),
PERFMON_CNTR(pte_cache_membus_stall, 0x1a1),
PERFMON_CNTR(eqd_cache_hits, 0x1a2),
PERFMON_CNTR(eqd_cache_miss, 0x1a3),
PERFMON_CNTR(eqd_cache_fill_stall, 0x1a4),
PERFMON_CNTR(eqd_cache_evict_stall, 0x1a5),
PERFMON_CNTR(eqd_cache_evicts, 0x1a6),
PERFMON_CNTR(eqd_cache_membus_stall, 0x1a7),
PERFMON_CNTR(trig_op_cache_hits, 0x1a8),
PERFMON_CNTR(trig_op_cache_miss, 0x1a9),
PERFMON_CNTR(trig_op_cache_fill_stall, 0x1aa),
PERFMON_CNTR(trig_op_cache_evict_stall, 0x1ab),
PERFMON_CNTR(trig_op_cache_evicts, 0x1ac),
PERFMON_CNTR(trig_op_cache_membus_stall, 0x1ad),
PERFMON_CNTR(psc0_cache_hits, 0x1ae),
PERFMON_CNTR(psc0_cache_miss, 0x1af),
PERFMON_CNTR(psc0_cache_fill_stall, 0x1b0),
PERFMON_CNTR(psc0_cache_evict_stall, 0x1b1),
PERFMON_CNTR(psc0_cache_evicts, 0x1b2),
PERFMON_CNTR(psc0_cache_membus_stall, 0x1b3),
PERFMON_CNTR(psc1_cache_hits, 0x1b4),
PERFMON_CNTR(psc1_cache_miss, 0x1b5),
PERFMON_CNTR(psc1_cache_fill_stall, 0x1b6),
PERFMON_CNTR(psc1_cache_evict_stall, 0x1b7),
PERFMON_CNTR(psc1_cache_evicts, 0x1b8),
PERFMON_CNTR(psc1_cache_membus_stall, 0x1b9),
};

#define C_MAX_NAME 13
int hfi_init_cntrs(struct hfi_devdata *dd)
{
	int i, j;
	size_t sz, devcntrs_sz;
	char name[C_MAX_NAME];
	struct hfi_pportdata *ppd;

	devcntrs_sz = 0;

	for (i = 0; i < NUM_DEV_CNTRS; i++) {
		devcntrs_sz += strlen(devcntr_names[i]) + 1;
		dev_cntrs[i].offset = i;
	}
	mutex_init(&dd->devcntrs_lock);
	dd->devcntrs = kcalloc(num_dev_cntrs, sizeof(u64), GFP_KERNEL);
	if (!dd->devcntrs)
		goto bail;

	dd->devcntrnameslen = devcntrs_sz;
	/* allocate space for maintaining last 16 bits of PMON counters */
	dd->scntrs = kcalloc(FXR_PMON_ARRAY_ENTRIES, sizeof(u16), GFP_KERNEL);
	if (!dd->scntrs)
		goto bail;

	sz = 0;

	for (i = 0; i < PORT_CNTR_LAST; i++) {
		port_cntrs[i].offset = i;
		if (port_cntrs[i].flags & CNTR_VL) {
			for (j = 0; j < C_VL_COUNT; j++) {
				sz += snprintf(name, C_MAX_NAME, "%s%d",
						portcntr_names[i],
						vl_from_idx(j)) + 1;
			}
		} else {
			sz += strlen(port_cntrs[i].name) + 1;
		}
	}
	for (i = 0; i < dd->num_pports; i++) {
		ppd = to_hfi_ppd(dd, i + 1);
		mutex_init(&ppd->portcntrs_lock);
		ppd->portcntrs = kcalloc(num_port_cntrs, sizeof(u64),
					 GFP_KERNEL);
		if (!ppd->portcntrs)
			goto bail;
	}

	dd->portcntrnameslen = sz;

	return 0;
bail:
	hfi_free_cntrs(dd);
	return -ENOMEM;
}

u32 hfi_read_portcntrs(struct hfi_pportdata *ppd, u64 **cntrp)
{
	int ret;
	u64 val = 0;

	const struct cntr_entry *entry;
	int i, j;

	ret = num_port_cntrs * sizeof(u64);
	*cntrp = ppd->portcntrs;

	mutex_lock(&ppd->portcntrs_lock);
	for (i = 0; i < PORT_CNTR_LAST; i++) {
		entry = &port_cntrs[i];
		if (entry->flags & CNTR_VL) {
			for (j = 0; j < C_VL_COUNT; j++) {
				val = entry->rw_cntr(entry, ppd, j);
				ppd->portcntrs[entry->offset + j] = val;
			}
		} else {
			u64 sw_xmit_cerr;

			sw_xmit_cerr = (i == XMIT_CONSTRAINT_ERR) ?
				       ppd->xmit_constraint_errors : 0;
			val = entry->rw_cntr(entry, ppd,
					     CNTR_INVALID_VL);
			ppd->portcntrs[entry->offset] = val +
							sw_xmit_cerr;
		}
	}
	mutex_unlock(&ppd->portcntrs_lock);
	return ret;
}

u32 hfi_read_devcntrs(struct hfi_devdata *dd, u64 **cntrd)
{
	int ret;
	u64 val = 0;

	const struct devcntr_entry *entry;
	int i;

	ret = num_dev_cntrs * sizeof(u64);
	*cntrd = dd->devcntrs;
	mutex_lock(&dd->devcntrs_lock);
	for (i = 0; i < NUM_DEV_CNTRS; i++) {
		entry = &dev_cntrs[i];
		val = entry->rw_cntr(dd, entry->index);
		dd->devcntrs[entry->offset] = val;
	}
	mutex_unlock(&dd->devcntrs_lock);
	return ret;
}

void hfi_free_cntrs(struct hfi_devdata *dd)
{
	struct hfi_pportdata *ppd;
	int i;

	kfree(dd->devcntrs);
	dd->devcntrs = NULL;
	kfree(dd->scntrs);
	dd->scntrs = NULL;
	for (i = 0; i < dd->num_pports; i++) {
		ppd = to_hfi_ppd(dd, i + 1);
		kfree(ppd->portcntrs);
		ppd->portcntrs = NULL;
	}
}
