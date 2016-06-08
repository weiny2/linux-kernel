// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Thu Jun  2 19:11:24 2016
//

#ifndef ___FXR_lm_fpc_CSRS_H__
#define ___FXR_lm_fpc_CSRS_H__

// FPC_ERR_STS desc:
typedef union {
    struct {
        uint64_t              spare_0  :  1; // reserved
        uint64_t           bad_l2_err  :  1; // Illegal L2 opcode
        uint64_t           bad_sc_err  :  1; // Unconfigured SC
        uint64_t     bad_mid_tail_err  :  1; // Unexpected Mid/Tail flit received ( headless )
        uint64_t   bad_preemption_err  :  1; // Number of preemption nesting level exceeded.
        uint64_t       preemption_err  :  1; // Preempting with same VL
        uint64_t   preemptionvl15_err  :  1; // Preempting a VL15 packet
        uint64_t    bad_vl_marker_err  :  1; // Received VL Marker control flit.
        uint64_t              spare_8  :  1; // reserved
        uint64_t  bad_dlid_target_err  :  1; // Does not match target dlid. ( HFI )
        uint64_t         bad_lver_err  :  1; // Illegal LVer in header
        uint64_t             spare_11  :  1; // reserved
        uint64_t             spare_12  :  1; // reserved
        uint64_t    uncorrectable_err  :  1; // Received an un-correctable error.(MBE)
        uint64_t     bad_crdt_ack_err  :  1; // Credit acks returned on illegal VL's, 8-14.
        uint64_t       unsup_pkt_type  :  1; // Received a packet type that was not configured.
        uint64_t    bad_ctrl_flit_err  :  1; // Unknown or reserved control flit received
        uint64_t event_cntr_parity_err  :  1; // Event counter has taken a parity error.
        uint64_t event_cntr_rollover_err  :  1; // Event counter rollover.
        uint64_t             link_err  :  1; // Link went from INIT/ARM/ACTIVE to DOWN.
        uint64_t misc_cntr_rollover_err  :  1; // One of the following counters rolled over. portrcv_err_cnt rcv_multicast_pkt_cnt fmconfig_cnt xmit_multicast_pkt_cnt dropped_pkt_cnt rcvremote_phy_err_cnt
        uint64_t    bad_ctrl_dist_err  :  1; // Control flit violation
        uint64_t    bad_tail_dist_err  :  1; // Tail distance violation
        uint64_t    bad_head_dist_err  :  1; // Head distance violation
        uint64_t    nonvl15_state_err  :  1; // Received Non-VL15 Pkt when ink state ==Init.
        uint64_t       vl15_multi_err  :  1; // Pkt contained VL15 and multicast DLID
        uint64_t   bad_pkt_length_err  :  1; // Packet length compare based on packet type.
        uint64_t         unsup_vl_err  :  1; // Pkt contained VL not configured - discard packet *** packet not sent to Rbuf. - report error
        uint64_t       perm_nvl15_err  :  1; // Pkt DLID=16'hFFFF & !VL15 this is (permissve and !VL15)
        uint64_t        slid_zero_err  :  1; // Pkt contained SLID == 0
        uint64_t        dlid_zero_err  :  1; // Pkt contained DLID == 0
        uint64_t       length_mtu_err  :  1; // Pkt contained LRH:Length > MTU_Cap
        uint64_t             spare_32  :  1; // reserved
        uint64_t    rx_early_drop_err  :  1; // Flag indicates there was an early error on the packet and the packet was dropped.
        uint64_t       late_short_err  :  1; // Actual Packet Length was less than LRH:Pkt_Length.
        uint64_t        late_long_err  :  1; // Actual Packet Length was greater than LRH:Pkt_Length.
        uint64_t         late_ebp_err  :  1; // Packet arriving contained EBP, marked EBP by remote device.
        uint64_t          spare_44_37  :  8; // reserved
        uint64_t       csr_parity_err  :  1; // Parity error on csr write data.
        uint64_t       csr_inval_addr  :  1; // dc_common recieved an illegal csr address.
        uint64_t          spare_53_47  :  7; // reserved
        uint64_t         fmconfig_err  :  1; // One of the following errors occurred. [E0] BadHeadDist: Distance violation between two head flits [E1] BadTailDist: Distance violation between two tail flits [E2] BadCtrlDist: Distance violation between two credit control flits [E3] BadCrdtAck: Credits return for unsupported VL [E4] UnsupportedVLMarker: Received VL Marker. [E5] BadPreempt: Exceeded the preemption nesting level [E6] BadControlFlit: Received unsupported control flit. [E7] reserved [E8] reserved See the following CSR for additional error information. See
        uint64_t          rcvport_err  :  1; // One of the following errors occurred. [E0] Reserved [E1] BadPktLen: Illegal PktLen [E2] PktLenTooLong: Packet longer than PktLen [E3] PktLenTooShort: Packet shorter than PktLen with normal tail [E4] BadSLID: Illegal SLID (0, using multicast as SLID. Does not include security validation of SLID) [E5] BadDLID: Illegal DLID (0, doesn't match HFI) [E6] BadL2: Illegal L2 opcode [E7] BadSC: Unsupported SC [E8] Reserved [E9] Headless: Tail or Body before Head. [E10] Reserved [E11] PreemptError: Preempting with same VL [E12] PreemptVL15: Preempting a VL15 packet [E13] BadSC Marker [E14] Reserved [E15] Reserved When this flag is asserted the following CSRs provide additional error information. Section 17.4.4.6, ' FPC_ERR_INFO_PORTRCV ' Section 17.4.4.7, ' FPC_ERR_INFO_PORTRCV_HDR0_A ' Section 17.4.4.9, ' FPC_ERR_INFO_PORTRCV_HDR1_A '
        uint64_t       reserved_63_56  :  8; // reserved
    } field;
    uint64_t val;
} FPC_ERR_STS_t;

// FPC_ERR_CLR desc:
typedef union {
    struct {
        uint64_t              spare_0  :  1; // reserved
        uint64_t           bad_l2_err  :  1; // 
        uint64_t           bad_sc_err  :  1; // 
        uint64_t     bad_mid_tail_err  :  1; // 
        uint64_t   bad_preemption_err  :  1; // 
        uint64_t       preemption_err  :  1; // 
        uint64_t   preemptionvl15_err  :  1; // 
        uint64_t    bad_vl_marker_err  :  1; // 
        uint64_t              spare_8  :  1; // reserved
        uint64_t  bad_dlid_target_err  :  1; // 
        uint64_t         bad_lver_err  :  1; // 
        uint64_t             spare_11  :  1; // reserved
        uint64_t             spare_12  :  1; // reserved
        uint64_t    uncorrectable_err  :  1; // 
        uint64_t     bad_crdt_ack_err  :  1; // 
        uint64_t       unsup_pkt_type  :  1; // 
        uint64_t    bad_ctrl_flit_err  :  1; // 
        uint64_t event_cntr_parity_err  :  1; // 
        uint64_t event_cntr_rollover_err  :  1; // 
        uint64_t             link_err  :  1; // 
        uint64_t misc_cntr_rollover_err  :  1; // 
        uint64_t    bad_ctrl_dist_err  :  1; // 
        uint64_t    bad_tail_dist_err  :  1; // 
        uint64_t    bad_head_dist_err  :  1; // 
        uint64_t    nonvl15_state_err  :  1; // 
        uint64_t       vl15_multi_err  :  1; // 
        uint64_t   bad_pkt_length_err  :  1; // 
        uint64_t         unsup_vl_err  :  1; // 
        uint64_t       perm_nvl15_err  :  1; // 
        uint64_t        slid_zero_err  :  1; // 
        uint64_t        dlid_zero_err  :  1; // 
        uint64_t       length_mtu_err  :  1; // 
        uint64_t             spare_32  :  1; // reserved
        uint64_t    rx_early_drop_err  :  1; // 
        uint64_t       late_short_err  :  1; // 
        uint64_t        late_long_err  :  1; // 
        uint64_t         late_ebp_err  :  1; // 
        uint64_t          spare_44_37  :  8; // reserved
        uint64_t       csr_parity_err  :  1; // 
        uint64_t       csr_inval_addr  :  1; // 
        uint64_t          spare_53_47  :  7; // reserved
        uint64_t         fmconfig_err  :  1; // 
        uint64_t          rcvport_err  :  1; // This register is used to clear error flags in DCC_ERR_FLG. e.g Writing a '1' to bit[55] clears rcvport_err in DCC_ERR_FLG. Clearing of the error flags also re-arms the info registers.
        uint64_t       reserved_63_56  :  8; // reserved
    } field;
    uint64_t val;
} FPC_ERR_CLR_t;

// FPC_ERR_FRC desc:
typedef union {
    struct {
        uint64_t              spare_0  :  1; // reserved
        uint64_t           bad_l2_err  :  1; // 
        uint64_t           bad_sc_err  :  1; // 
        uint64_t     bad_mid_tail_err  :  1; // 
        uint64_t   bad_preemption_err  :  1; // 
        uint64_t       preemption_err  :  1; // 
        uint64_t   preemptionvl15_err  :  1; // 
        uint64_t    bad_vl_marker_err  :  1; // 
        uint64_t              spare_8  :  1; // reserved
        uint64_t  bad_dlid_target_err  :  1; // 
        uint64_t         bad_lver_err  :  1; // 
        uint64_t             spare_11  :  1; // reserved
        uint64_t             spare_12  :  1; // reserved
        uint64_t    uncorrectable_err  :  1; // 
        uint64_t     bad_crdt_ack_err  :  1; // 
        uint64_t       unsup_pkt_type  :  1; // 
        uint64_t    bad_ctrl_flit_err  :  1; // 
        uint64_t event_cntr_parity_err  :  1; // 
        uint64_t event_cntr_rollover_err  :  1; // 
        uint64_t             link_err  :  1; // 
        uint64_t misc_cntr_rollover_err  :  1; // 
        uint64_t    bad_ctrl_dist_err  :  1; // 
        uint64_t    bad_tail_dist_err  :  1; // 
        uint64_t    bad_head_dist_err  :  1; // 
        uint64_t    nonvl15_state_err  :  1; // 
        uint64_t       vl15_multi_err  :  1; // 
        uint64_t   bad_pkt_length_err  :  1; // 
        uint64_t         unsup_vl_err  :  1; // 
        uint64_t       perm_nvl15_err  :  1; // 
        uint64_t        slid_zero_err  :  1; // 
        uint64_t        dlid_zero_err  :  1; // 
        uint64_t       length_mtu_err  :  1; // 
        uint64_t             spare_32  :  1; // reserved
        uint64_t    rx_early_drop_err  :  1; // 
        uint64_t       late_short_err  :  1; // 
        uint64_t        late_long_err  :  1; // 
        uint64_t         late_ebp_err  :  1; // 
        uint64_t          spare_44_37  :  8; // reserved
        uint64_t       csr_parity_err  :  1; // 
        uint64_t       csr_inval_addr  :  1; // 
        uint64_t          spare_53_47  :  7; // reserved
        uint64_t         fmconfig_err  :  1; // 
        uint64_t          rcvport_err  :  1; // This register is used to clear error flags in DCC_ERR_FLG. e.g Writing a '1' to bit[55] clears rcvport_err in DCC_ERR_FLG. Clearing of the error flags also re-arms the info registers.
        uint64_t       reserved_63_56  :  8; // reserved
    } field;
    uint64_t val;
} FPC_ERR_FRC_t;

// FPC_ERR_EN_HOST desc:
typedef union {
    struct {
        uint64_t              spare_0  :  1; // reserved
        uint64_t           bad_l2_err  :  1; // 
        uint64_t           bad_sc_err  :  1; // 
        uint64_t     bad_mid_tail_err  :  1; // 
        uint64_t   bad_preemption_err  :  1; // 
        uint64_t       preemption_err  :  1; // 
        uint64_t   preemptionvl15_err  :  1; // 
        uint64_t    bad_vl_marker_err  :  1; // 
        uint64_t              spare_8  :  1; // reserved
        uint64_t  bad_dlid_target_err  :  1; // 
        uint64_t         bad_lver_err  :  1; // 
        uint64_t             spare_11  :  1; // reserved
        uint64_t             spare_12  :  1; // reserved
        uint64_t    uncorrectable_err  :  1; // 
        uint64_t     bad_crdt_ack_err  :  1; // 
        uint64_t       unsup_pkt_type  :  1; // 
        uint64_t    bad_ctrl_flit_err  :  1; // 
        uint64_t event_cntr_parity_err  :  1; // 
        uint64_t event_cntr_rollover_err  :  1; // 
        uint64_t             link_err  :  1; // 
        uint64_t misc_cntr_rollover_err  :  1; // 
        uint64_t    bad_ctrl_dist_err  :  1; // 
        uint64_t    bad_tail_dist_err  :  1; // 
        uint64_t    bad_head_dist_err  :  1; // 
        uint64_t    nonvl15_state_err  :  1; // 
        uint64_t       vl15_multi_err  :  1; // 
        uint64_t   bad_pkt_length_err  :  1; // 
        uint64_t         unsup_vl_err  :  1; // 
        uint64_t       perm_nvl15_err  :  1; // 
        uint64_t        slid_zero_err  :  1; // 
        uint64_t        dlid_zero_err  :  1; // 
        uint64_t       length_mtu_err  :  1; // 
        uint64_t             spare_32  :  1; // reserved
        uint64_t    rx_early_drop_err  :  1; // 
        uint64_t       late_short_err  :  1; // 
        uint64_t        late_long_err  :  1; // 
        uint64_t         late_ebp_err  :  1; // 
        uint64_t          spare_44_37  :  8; // reserved
        uint64_t       csr_parity_err  :  1; // 
        uint64_t       csr_inval_addr  :  1; // 
        uint64_t          spare_53_47  :  7; // reserved
        uint64_t         fmconfig_err  :  1; // 
        uint64_t          rcvport_err  :  1; // Interrupt enable Setting to a 1 will enable interrupts to propagate to higher level logic.
        uint64_t       reserved_63_56  :  8; // reserved
    } field;
    uint64_t val;
} FPC_ERR_EN_HOST_t;

// FPC_ERR_FIRST_HOST desc:
typedef union {
    struct {
        uint64_t              spare_0  :  1; // reserved
        uint64_t           bad_l2_err  :  1; // 
        uint64_t           bad_sc_err  :  1; // 
        uint64_t     bad_mid_tail_err  :  1; // 
        uint64_t   bad_preemption_err  :  1; // 
        uint64_t       preemption_err  :  1; // 
        uint64_t   preemptionvl15_err  :  1; // 
        uint64_t    bad_vl_marker_err  :  1; // 
        uint64_t              spare_8  :  1; // reserved
        uint64_t  bad_dlid_target_err  :  1; // 
        uint64_t         bad_lver_err  :  1; // 
        uint64_t             spare_11  :  1; // reserved
        uint64_t             spare_12  :  1; // reserved
        uint64_t    uncorrectable_err  :  1; // 
        uint64_t     bad_crdt_ack_err  :  1; // 
        uint64_t       unsup_pkt_type  :  1; // 
        uint64_t    bad_ctrl_flit_err  :  1; // 
        uint64_t event_cntr_parity_err  :  1; // 
        uint64_t event_cntr_rollover_err  :  1; // 
        uint64_t             link_err  :  1; // 
        uint64_t misc_cntr_rollover_err  :  1; // 
        uint64_t    bad_ctrl_dist_err  :  1; // 
        uint64_t    bad_tail_dist_err  :  1; // 
        uint64_t    bad_head_dist_err  :  1; // 
        uint64_t    nonvl15_state_err  :  1; // 
        uint64_t       vl15_multi_err  :  1; // 
        uint64_t   bad_pkt_length_err  :  1; // 
        uint64_t         unsup_vl_err  :  1; // 
        uint64_t       perm_nvl15_err  :  1; // 
        uint64_t        slid_zero_err  :  1; // 
        uint64_t        dlid_zero_err  :  1; // 
        uint64_t       length_mtu_err  :  1; // 
        uint64_t             spare_32  :  1; // reserved
        uint64_t    rx_early_drop_err  :  1; // 
        uint64_t       late_short_err  :  1; // 
        uint64_t        late_long_err  :  1; // 
        uint64_t         late_ebp_err  :  1; // 
        uint64_t          spare_44_37  :  8; // reserved
        uint64_t       csr_parity_err  :  1; // 
        uint64_t       csr_inval_addr  :  1; // 
        uint64_t          spare_53_47  :  7; // reserved
        uint64_t         fmconfig_err  :  1; // 
        uint64_t          rcvport_err  :  1; // Interrupt enable Setting to a 1 will enable interrupts to propagate to higher level logic.
        uint64_t       reserved_63_56  :  8; // reserved
    } field;
    uint64_t val;
} FPC_ERR_FIRST_HOST_t;

// FPC_ERR_INFO_PORTRCV desc:
typedef union {
    struct {
        uint64_t               e_code  :  8; // One of the following errors occurred. [E0] Reserved [E1] BadPktLen: Illegal PktLen [E2] PktLenTooLong: Packet longer than PktLen [E3] PktLenTooShort: Packet shorter than PktLen with normal tail [E4] BadSLID: Illegal SLID (0, using multicast as SLID. Does not include security validation of SLID) [E5] BadDLID: Illegal DLID (0, doesn't match HFI) [E6] BadL2: Illegal L2 opcode [E7] BadSC: Unsupported SC [E8] Reserved [E9] Headless: Tail or Body before Head. [E10] Reserved [E11] PreemptError: Preempting with same VL [E12] PreemptVL15: Preempting a VL15 packet [E13] BadSC Maker [E14] Reserved [E15] Reserved
        uint64_t        reserved_63_8  : 56; // reserved
    } field;
    uint64_t val;
} FPC_ERR_INFO_PORTRCV_t;

// FPC_ERR_INFO_PORTRCV_HDR0_A desc:
typedef union {
    struct {
        uint64_t            head_flit  : 64; // Packet header flit 0 (even)
    } field;
    uint64_t val;
} FPC_ERR_INFO_PORTRCV_HDR0_A_t;

// FPC_ERR_INFO_PORTRCV_HDR0_B desc:
typedef union {
    struct {
        uint64_t     head_flit_bit_64  :  1; // bit 64 of packet data.
        uint64_t        reserved_63_1  : 63; // reserved
    } field;
    uint64_t val;
} FPC_ERR_INFO_PORTRCV_HDR0_B_t;

// FPC_ERR_INFO_PORTRCV_HDR1_A desc:
typedef union {
    struct {
        uint64_t            head_flit  : 64; // Packet header flit
    } field;
    uint64_t val;
} FPC_ERR_INFO_PORTRCV_HDR1_A_t;

// FPC_ERR_INFO_PORTRCV_HDR1_B desc:
typedef union {
    struct {
        uint64_t     head_flit_bit_64  :  1; // bit 64 of packet data.
        uint64_t        reserved_63_1  : 63; // reserved
    } field;
    uint64_t val;
} FPC_ERR_INFO_PORTRCV_HDR1_B_t;

// FPC_ERR_INFO_FMCONFIG desc:
typedef union {
    struct {
        uint64_t               e_code  :  8; // [E0] BadHeadDist: Distance violation between two head flits [E1] BadTailDist: Distance violation between two tail flits [E2] BadCtrlDist: Distance violation between two credit control flits [E3] BadCrdtAck: Credits return for unsupported VL [E4] BadVLMarker: VL Marker received for unsupported VL [E5] BadPreempt: Exceeded the preemption nesting level [E6] BadCtrlFlit: Received an illegal control flit. [E7] Reserved [E8] Unsupported VL: Received VL that was not supported.
        uint64_t        reserved_63_8  : 56; // reserved
    } field;
    uint64_t val;
} FPC_ERR_INFO_FMCONFIG_t;

// FPC_ERR_INFO_FLOW_CTRL desc:
typedef union {
    struct {
        uint64_t              fc_data  : 64; // credit return flit
    } field;
    uint64_t val;
} FPC_ERR_INFO_FLOW_CTRL_t;

// FPC_ERR_INFO_UNCORRECTABLE desc:
typedef union {
    struct {
        uint64_t               e_code  :  4; // [0] BadHead: uncorrectable error in head flit [1] BadBody: uncorrectable error in body flit [2] BadTail: uncorrectable error in tail flit [3] BadCtrl: uncorrectable error in credit flit.
        uint64_t        reserved_63_4  : 60; // reserved
    } field;
    uint64_t val;
} FPC_ERR_INFO_UNCORRECTABLE_t;

// FPC_ERR_INFO_PORTRCVCONSTRAINT desc:
typedef union {
    struct {
        uint64_t               e_code  :  2; // [0] reserved [1] Pkey Violation [2] Slid Security Violation [3] Switch Port 0 Pkey Violation ( see rpipe error csrs)
        uint64_t           unused_7_2  :  6; // unused
        uint64_t                 pkey  : 16; // Pkey Record for only error codes [1],[3]
        uint64_t                 slid  : 24; // SLID Record for all error codes.
        uint64_t         unused_63_48  : 16; // unused
    } field;
    uint64_t val;
} FPC_ERR_INFO_PORTRCVCONSTRAINT_t;

// FPC_CFG_PORT_CONFIG desc:
typedef union {
    struct {
        uint64_t pkt_formats_supported  :  4; // Bit Mask - Supported packet format types bit 0 - 8B bit 1 - 9B bit 2 - 10B bit 3 - 16B
        uint64_t  pkt_formats_enabled  :  4; // Bit Mask - Enabled packet format types bit 0 - 8B bit 1 - 9B bit 2 - 10B bit 3 - 16B
        uint64_t          vl0_mtu_cap  :  3; // MTU capability for this VL Packets containing LRH length greater than the MTU_CAP will be discarded. 0x0 = 0 (vl disabled, no packets allowed) 0x1 = 256 bytes + Header: 8B,9B,10B,16B = 48 flits 0x2 = 512 bytes + Header: 8B,9B,10B,16B = 80 flits 0x3 = 1024 bytes + Header: 8B,9B,10B,16B = 144 flits 0x4 = 2048 bytes + header: 8B,9B,10B,16B = 272 flits 0x5 = 4096 bytes + header: 8B,9B,10B,16B = 528 flits 0x6 = 8192 bytes + Header: 8B,9B,10B,16B = 1040 flits 0x7 = 10240 bytes + Header:8B,9B,10B,16B = 1296 flits
        uint64_t          vl1_mtu_cap  :  3; // MTU capability for VL1
        uint64_t          vl2_mtu_cap  :  3; // MTU capability for VL2
        uint64_t          vl3_mtu_cap  :  3; // MTU capability for VL3
        uint64_t          vl4_mtu_cap  :  3; // MTU capability for VL4
        uint64_t          vl5_mtu_cap  :  3; // MTU capability for VL5
        uint64_t          vl6_mtu_cap  :  3; // MTU capability for VL6
        uint64_t          vl7_mtu_cap  :  3; // MTU capability for VL7
        uint64_t          vl8_mtu_cap  :  3; // MTU capability for VL8
        uint64_t         vl15_mtu_cap  :  3; // MTU capability for VL15
        uint64_t       reserved_39_38  :  2; // reserved
        uint64_t              pkey_en  :  1; // Enable Pkey checking
        uint64_t       reserved_47_41  :  7; // reserved
        uint64_t       reserved_63_48  : 16; // reserved
    } field;
    uint64_t val;
} FPC_CFG_PORT_CONFIG_t;

// FPC_CFG_PORT_CONFIG1 desc:
typedef union {
    struct {
        uint64_t          multi_mask8  :  7; // 7 bit mask used to 8/10B packets filter for multicast lids. (pkt.dlid[20:14] & Mask8) ^ Mask8) == 0
        uint64_t           reserved_7  :  1; // reserved
        uint64_t          multi_mask9  :  7; // 7 bit mask used to 9B packets filter for multicast lids. ((pkt.dlid[15:9] & Mask9) ^ Mask9) == 0
        uint64_t          reserved_15  :  1; // reserved
        uint64_t         multi_mask16  :  7; // 7 bit mask used to filter 16B packets for multicast lids. ((pkt.dlid[23:17] & Mask16) ^ Mask16) == 0
        uint64_t          reserved_23  :  1; // reserved
        uint64_t          reserved_24  :  1; // reserved
        uint64_t       reserved_31_25  :  7; // reserved
        uint64_t       reserved_63_32  : 32; // reserved
    } field;
    uint64_t val;
} FPC_CFG_PORT_CONFIG1_t;

// FPC_CFG_SC_VL_TABLE_15_0 desc:
typedef union {
    struct {
        uint64_t               entry0  :  4; // Entry 0 of the mapping table, maps to SC 0.
        uint64_t               entry1  :  4; // Entry 1of the mapping table, maps to SC 1
        uint64_t               entry2  :  4; // Entry 2of the mapping table, maps to SC 2
        uint64_t               entry3  :  4; // Entry 3 of the mapping table, maps to SC 3
        uint64_t               entry4  :  4; // Entry 4 of the mapping table, maps to SC 4
        uint64_t               entry5  :  4; // Entry 5 of the mapping table, maps to SC 5
        uint64_t               entry6  :  4; // Entry 6 of the mapping table, maps to SC 6
        uint64_t               entry7  :  4; // Entry 7 of the mapping table, maps to SC 7
        uint64_t               entry8  :  4; // Entry 8 of the mapping table, maps to SC 8
        uint64_t               entry9  :  4; // Entry 9 of the mapping table, maps to SC 9
        uint64_t              entry10  :  4; // Entry 10 of the mapping table, maps to SC 10
        uint64_t              entry11  :  4; // Entry 11 of the mapping table, maps to SC 11
        uint64_t              entry12  :  4; // Entry 12 of the mapping table, maps to SC 12
        uint64_t              entry13  :  4; // Entry 13 of the mapping table, maps to SC 13
        uint64_t              entry14  :  4; // Entry 14 of the mapping table, maps to SC 14
        uint64_t              entry15  :  4; // Entry 15 of the mapping table, maps to SC 15
    } field;
    uint64_t val;
} FPC_CFG_SC_VL_TABLE_15_0_t;

// FPC_CFG_SC_VL_TABLE_31_16 desc:
typedef union {
    struct {
        uint64_t              entry16  :  4; // Entry 16 of the mapping table, maps to SC 16
        uint64_t              entry17  :  4; // Entry 17 of the mapping table, maps to SC 17
        uint64_t              entry18  :  4; // Entry 18 of the mapping table, maps to SC 18
        uint64_t              entry19  :  4; // Entry 19 of the mapping table, maps to SC 19
        uint64_t              entry20  :  4; // Entry 20 of the mapping table, maps to SC 20
        uint64_t              entry21  :  4; // Entry 21 of the mapping table, maps to SC 21
        uint64_t              entry22  :  4; // Entry 22 of the mapping table, maps to SC 22
        uint64_t              entry23  :  4; // Entry 23 of the mapping table, maps to SC 23
        uint64_t              entry24  :  4; // Entry 24 of the mapping table, maps to SC 24
        uint64_t              entry25  :  4; // Entry 25 of the mapping table, maps to SC 25
        uint64_t              entry26  :  4; // Entry 26 of the mapping table, maps to SC 26
        uint64_t              entry27  :  4; // Entry 27 of the mapping table, maps to SC 27
        uint64_t              entry28  :  4; // Entry 28 of the mapping table, maps to SC 28
        uint64_t              entry29  :  4; // Entry 29 of the mapping table, maps to SC 29
        uint64_t              entry30  :  4; // Entry 30 of the mapping table, maps to SC 30
        uint64_t              entry31  :  4; // Entry 31 of the mapping table, maps to SC 31
    } field;
    uint64_t val;
} FPC_CFG_SC_VL_TABLE_31_16_t;

// FPC_CFG_PKEY_CTRL desc:
typedef union {
    struct {
        uint64_t              pkey_8b  : 16; // The Pkey Check Control register is ignored for STL 9B and 16B packets. STL 8B: Packet Pkey = {Pkey_Check_8B[15:0]}
        uint64_t         UNUSED_19_16  :  4; // 
        uint64_t             pkey_10b  : 12; // The Pkey Check Control register is ignored for STL 9B and 16B packets. STL 10B: Packet Pkey = {Pkey_Check_10B[31:20],Pkt:Pkey[3:0]}
        uint64_t         UNUSED_63_32  : 32; // 
    } field;
    uint64_t val;
} FPC_CFG_PKEY_CTRL_t;

// FPC_CFG_PKEY_TABLE desc:
typedef union {
    struct {
        uint64_t               entry0  : 16; // Partition Key table entry 0.
        uint64_t               entry1  : 16; // Partition Key table entry 1.
        uint64_t               entry2  : 16; // Partition Key table entry 2.
        uint64_t               entry3  : 16; // Partition Key table entry 3.
    } field;
    uint64_t val;
} FPC_CFG_PKEY_TABLE_t;

// FPC_CFG_EVENT_CNTR_CTRL desc:
typedef union {
    struct {
        uint64_t         enable_cntrs  :  1; // Enables the event counter block.
        uint64_t            clr_cntrs  :  1; // When writing a '1' to this registers, a single pulse is generated and a '0' is written back to the register. All the counters associated with the event counter block are cleared.
        uint64_t       clr_cntrs_comp  :  1; // When = 0, the event counters are in the process of being cleared. When = 1, the clear sequence has completed.
        uint64_t      parity_err_addr  :  6; // Memory address where the parity error occurred.
        uint64_t          errinj_addr  :  6; // Ram address to inject error on. 0x0 - 0x3F
        uint64_t        errinj_parity  :  4; // Parity mask.
        uint64_t          errinj_mode  :  2; // 0x0 - Inject error once 0x1- 0x3 reserved
        uint64_t            errinj_en  :  1; // Error injection enabled.
        uint64_t         errinj_reset  :  1; // Resets error injection control logic. When written to a '1' a single pulse is generated and a '0' is written back to the register.This needs to be asserted between errors injected.
        uint64_t     errinj_triggered  :  1; // Error was injected.
        uint64_t       reserved_63_24  : 40; // reserved
    } field;
    uint64_t val;
} FPC_CFG_EVENT_CNTR_CTRL_t;

// FPC_CFG_SLID_SECURITY desc:
typedef union {
    struct {
        uint64_t        neighbor_slid  : 24; // Neighbor SLID filter register
        uint64_t             slid_lmc  :  4; // Neighbor SLID LMC register. 0x0 = Full SLID compare 0x1 = disable check on SLID bit 0 0x2 = disable check on SLID bits 1:0 :: 0xF = disable check on SLID bits 14:0
        uint64_t      neighbor_device  :  1; // Type of Neighbor device connected to this port. 0 = Neighbor port is a switch port 1 = Neighbor port is an HFI
        uint64_t         UNUSED_63_29  : 35; // unused
    } field;
    uint64_t val;
} FPC_CFG_SLID_SECURITY_t;

// FPC_DBG_ERROR_INJECTION desc:
typedef union {
    struct {
        uint64_t                  arm  :  1; // This bit is dual purpose. Writing a 1 to this bit will arm the injection mechanism. Once an error is injected this bit is cleared.
        uint64_t            flit_type  :  2; // pntr decode sop = 0; mid = 1; tail =2, reserved = 3
        uint64_t         reserved_7_3  :  5; // reserved
        uint64_t           check_byte  :  8; // The check byte is exclusive or'ed with the packet check byte.
        uint64_t       reserved_63_16  : 48; // reserved
    } field;
    uint64_t val;
} FPC_DBG_ERROR_INJECTION_t;

// FPC_ERR_PORTRCV_ERROR desc:
typedef union {
    struct {
        uint64_t                  cnt  : 64; // 
    } field;
    uint64_t val;
} FPC_ERR_PORTRCV_ERROR_t;

// FPC_ERR_FMCONFIG_ERROR desc:
typedef union {
    struct {
        uint64_t                  cnt  : 64; // 
    } field;
    uint64_t val;
} FPC_ERR_FMCONFIG_ERROR_t;

// FPC_ERR_PORTRCV_CONSTRAINT_ERROR desc:
typedef union {
    struct {
        uint64_t                  cnt  : 64; // 
    } field;
    uint64_t val;
} FPC_ERR_PORTRCV_CONSTRAINT_ERROR_t;

// FPC_ERR_PORTRCV_PHY_REMOTE_ERROR desc:
typedef union {
    struct {
        uint64_t                  cnt  : 64; // 
    } field;
    uint64_t val;
} FPC_ERR_PORTRCV_PHY_REMOTE_ERROR_t;

// FPC_ERR_PORTRCV_SWITCH_RELAY_ERROR desc:
typedef union {
    struct {
        uint64_t                  cnt  : 64; // 
    } field;
    uint64_t val;
} FPC_ERR_PORTRCV_SWITCH_RELAY_ERROR_t;

// FPC_ERR_UNCORRECTABLE_ERROR desc:
typedef union {
    struct {
        uint64_t                  cnt  : 64; // 
    } field;
    uint64_t val;
} FPC_ERR_UNCORRECTABLE_ERROR_t;

// FPC_PRF_PORTRCV_MCAST_PKT_CNT desc:
typedef union {
    struct {
        uint64_t                  cnt  : 64; // 
    } field;
    uint64_t val;
} FPC_PRF_PORTRCV_MCAST_PKT_CNT_t;

// FPC_PRF_PORTRCV_PKT_CNT desc:
typedef union {
    struct {
        uint64_t                  cnt  : 64; // 
    } field;
    uint64_t val;
} FPC_PRF_PORTRCV_PKT_CNT_t;

// FPC_PRF_PORT_VL_RCV_PKT_CNT desc:
typedef union {
    struct {
        uint64_t                  cnt  : 64; // 
    } field;
    uint64_t val;
} FPC_PRF_PORT_VL_RCV_PKT_CNT_t;

// FPC_PRF_PORTRCV_DATA_CNT desc:
typedef union {
    struct {
        uint64_t                  cnt  : 64; // 
    } field;
    uint64_t val;
} FPC_PRF_PORTRCV_DATA_CNT_t;

// FPC_PRF_PORT_VL_RCV_DATA_CNT desc:
typedef union {
    struct {
        uint64_t                  cnt  : 64; // 
    } field;
    uint64_t val;
} FPC_PRF_PORT_VL_RCV_DATA_CNT_t;

// FPC_PRF_PORTRCV_FECN desc:
typedef union {
    struct {
        uint64_t                  cnt  : 64; // 
    } field;
    uint64_t val;
} FPC_PRF_PORTRCV_FECN_t;

// FPC_PRF_PORT_VL_RCV_FECN desc:
typedef union {
    struct {
        uint64_t                  cnt  : 64; // 
    } field;
    uint64_t val;
} FPC_PRF_PORT_VL_RCV_FECN_t;

// FPC_PRF_PORTRCV_BECN desc:
typedef union {
    struct {
        uint64_t                  cnt  : 64; // 
    } field;
    uint64_t val;
} FPC_PRF_PORTRCV_BECN_t;

// FPC_PRF_PORT_VL_RCV_BECN desc:
typedef union {
    struct {
        uint64_t                  cnt  : 64; // 
    } field;
    uint64_t val;
} FPC_PRF_PORT_VL_RCV_BECN_t;

// FPC_PRF_PORTRCV_BUBBLE desc:
typedef union {
    struct {
        uint64_t                  cnt  : 64; // 
    } field;
    uint64_t val;
} FPC_PRF_PORTRCV_BUBBLE_t;

// FPC_PRF_PORT_VL_RCV_BUBBLE desc:
typedef union {
    struct {
        uint64_t                  cnt  : 64; // 
    } field;
    uint64_t val;
} FPC_PRF_PORT_VL_RCV_BUBBLE_t;

// FPC_PRF_PORT_VL_RCV_INTERLEAVE_LEV desc:
typedef union {
    struct {
        uint64_t                  cnt  : 64; // 
    } field;
    uint64_t val;
} FPC_PRF_PORT_VL_RCV_INTERLEAVE_LEV_t;

// FPC_PRF_PORT_VL_RCVMARKERS desc:
typedef union {
    struct {
        uint64_t                  cnt  : 64; // 
    } field;
    uint64_t val;
} FPC_PRF_PORT_VL_RCVMARKERS_t;

// FPC_PRF_PORTRCV_SPC desc:
typedef union {
    struct {
        uint64_t                  cnt  : 64; // 
    } field;
    uint64_t val;
} FPC_PRF_PORTRCV_SPC_t;

// FPC_PRF_PORTRCV_CRDT desc:
typedef union {
    struct {
        uint64_t                  cnt  : 64; // 
    } field;
    uint64_t val;
} FPC_PRF_PORTRCV_CRDT_t;

// FPC_PRF_PORTRCV_PKT_DROP desc:
typedef union {
    struct {
        uint64_t                  cnt  : 64; // 
    } field;
    uint64_t val;
} FPC_PRF_PORTRCV_PKT_DROP_t;

#endif /* ___FXR_lm_fpc_CSRS_H__ */