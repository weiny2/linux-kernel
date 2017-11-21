// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Fri May 26 13:04:25 2017
//

#ifndef ___FXR_lm_fpc_CSRS_H__
#define ___FXR_lm_fpc_CSRS_H__

// FPC_ERR_STS desc:
typedef union {
    struct {
        uint64_t              spare_0  :  1; // spare
        uint64_t               l2_err  :  1; // PortRcv_err.BadL2 Illegal L2 opcode
        uint64_t               sc_err  :  1; // PortRcv_err.BadSC Unconfigured SC
        uint64_t         headless_err  :  1; // PortRcv_err.Headless Unexpected Mid/Tail flit received ( headless )
        uint64_t             pkey_err  :  1; // PortConstraint_err Pkey error
        uint64_t  preempt_same_vl_err  :  1; // PortRcv_err.PreemptError Preempting with same VL
        uint64_t            l2hdr_err  :  1; // PortRcv_err.PreemptL2Header Received a sop/eop or SC marker before previous sop was satisfied.
        uint64_t           sc_mkr_err  :  1; // PortRcv_err.BadSCMarker SC Marker received for inactive VL
        uint64_t         unsup_vl_err  :  1; // FMConfig_err.UnsuportedVLMarker SC-VL table returned a VL of 9-14
        uint64_t              spare_9  :  1; // spare
        uint64_t             spare_10  :  1; // spare
        uint64_t             spare_11  :  1; // spare
        uint64_t             spare_12  :  1; // spare
        uint64_t         mbe_outbound  :  1; // Uncorrectable Received an un-correctable error on outbound data path.(MBE)
        uint64_t         crdt_ack_err  :  1; // FMConfig_err.BadCrdtAck Credit acks returned on illegal VL's, 8-14.
        uint64_t       unsup_pkt_type  :  1; // PortRcv_err.BadL2 Received a packet type that was not configured.
        uint64_t   crdt_sb_parity_err  :  1; // Uncorrectable credit sideband parity error
        uint64_t          mbe_inbound  :  1; // Uncorrectable MBE on incoming data path. Inbound data path is locked down. A Link bounce is required.
        uint64_t event_cntr_rollover_err  :  1; // Not mapped to portrcv nor fmconfig. Event counter rollover.
        uint64_t             link_err  :  1; // Not mapped to portrcv nor fmconfig. Link went from INIT/ARM/ACTIVE to DOWN.
        uint64_t         mkr_dist_err  :  1; // FMConfig_err.BadMarkerDist. SC Marker distance violation
        uint64_t        ctrl_dist_err  :  1; // FMConfig_err.BadCtrlDist. Congest or Credit flit distance violation.
        uint64_t        tail_dist_err  :  1; // FMConfig_err.BadTailDist. Tail distance violation
        uint64_t        head_dist_err  :  1; // FMConfig_err.BadHeadDist. Head distance violation
        uint64_t    nonvl15_state_err  :  1; // PortRcv_err.BadSC Received Non-VL15 Pkt when ink state ==Init.
        uint64_t       vl15_multi_err  :  1; // PortRcv_err.BadDLID Pkt contained VL15 and multicast DLID
        uint64_t       pkt_length_err  :  1; // PortRcv_err.BadPktLen Packet length violated - min length.
        uint64_t             spare_27  :  1; // spare
        uint64_t       perm_nvl15_err  :  1; // PortRcv_err.BadSC Permissive SLID and SC!15
        uint64_t        slid_zero_err  :  1; // PortRcv_err.BADSLID Pkt contained SLID == 0
        uint64_t        dlid_zero_err  :  1; // PortRcv_err.BadDLID Pkt contained DLID == 0
        uint64_t       length_mtu_err  :  1; // PortRcv_err.BadPktLen Pkt contained LRH:Length > MTU_Cap
        uint64_t         slid_sec_err  :  1; // PortConstraint_err slid security error
        uint64_t             spare_33  :  1; // spare
        uint64_t       late_short_err  :  1; // Portrcv_err.PktLenTooShort Actual Packet Length was less than LRH:Pkt_Length. If both late_long and late_short errors are both set in ERROR_FIRST, simultaneous errors occurred in the same cycle. To determine which error occurred first look ERR_INFO_HDR0. If this contains a tail ,short_err occurred first. If no tail then long occurred first.
        uint64_t        late_long_err  :  1; // PortRcv_err.PktLenTooLong Actual Packet Length was greater than LRH:Pkt_Length. If both late_long and late_short errors are set in ERROR_FIRST, simultaneous errors occurred in the same cycle. To determine which error occurred first look ERR_INFO_HDR0. If this contains a tail ,short_err occurred first. If no tail then long occurred first.
        uint64_t             spare_36  :  1; // spare
        uint64_t       ltp_buf_unflow  :  1; // Uncorrectable ltp buffer over flow
        uint64_t       ltp_buf_ovflow  :  1; // Uncorrectable ltp buffer over flow
        uint64_t          spare_51_39  : 13; // spare
        uint64_t   portconstraint_err  :  1; // [0] reserved [1] Pkey Violation [2] Slid Security Violation [3] Switch Port 0 Pkey Violation ( see rpipe error csrs) See Section 21.16.4.14, ' FPC_ERR_INFO_PORTRCVCONSTRAINT '
        uint64_t    uncorrectable_err  :  1; // [0] BadHead: uncorrectable error in head flit [1] BadBody: uncorrectable error in body flit [2] BadTail: uncorrectable error in tail flit [3] BadCtrl: uncorrectable error in credit flit. [4] Internal: Internal logic error , unrecoverable See Section 21.16.4.13, ' FPC_ERR_INFO_UNCORRECTABLE '
        uint64_t         fmconfig_err  :  1; // One of the following errors occurred. [E0] BadHeadDist: Distance violation between two head flits [E1] BadTailDist: Distance violation between two tail flits [E2] BadCtrlDist: Distance violation between two credit control flits [E3] BadCrdtAck: Credits return for unsupported VL [4] UnsupportedVLMarker: SC Marker received for unsupported SC or when SC marker not enabled for port [5] BadPreempt: Exceeded the interleaving level or receive implicit interleaving sequence when only explicit interleaving is enabled. [6] BadControlFlit: unknown or reserved control flit received - Deprecated [7] ExceedMulticastLimit [8] BadMarkerDist: Distance violation between two VL Markers, VL Marker and Head. See Section 21.16.4.11, ' FPC_ERR_INFO_FMCONFIG ' for additional error information.
        uint64_t          rcvport_err  :  1; // One of the following errors occurred. [E0] Reserved [E1] BadPktLen: Illegal PktLen [E2] PktLenTooLong: Packet longer than PktLen [E3] PktLenTooShort: Packet shorter than PktLen with normal tail [E4] BadSLID: Illegal SLID (0, using multicast as SLID. Does not include security validation of SLID) [E5] BadDLID: Illegal DLID (0, doesn't match HFI) [E6] BadL2: Illegal L2 opcode [E7] BadSC: Unsupported SC [E8] Reserved [E9] Headless: Tail or Body before Head. [E10] Reserved [E11] PreemptError: Preempting with same VL [E12] PreemptVL15: Preempting a VL15 packet [E13] BadSC Marker: Inactive VL [E14] PreemptL2Header: Interleaving L2 header When this flag is asserted the following CSRs provide additional error information. Section 21.16.4.6, ' FPC_ERR_INFO_PORTRCV ' Section 21.16.4.7, ' FPC_ERR_INFO_PORTRCV_HDR0_A ' Section 21.16.4.8, ' FPC_ERR_INFO_PORTRCV_HDR0_B ' Section 21.16.4.9, ' FPC_ERR_INFO_PORTRCV_HDR1_A ' Section 21.16.4.10, ' FPC_ERR_INFO_PORTRCV_HDR1_B '
        uint64_t       reserved_63_56  :  8; // reserved
    } field;
    uint64_t val;
} FPC_ERR_STS_t;

// FPC_ERR_CLR desc:
typedef union {
    struct {
        uint64_t              err_clr  : 56; // Clear the associated bit in the FPC_ERR_STS
        uint64_t       reserved_63_56  :  8; // reserved
    } field;
    uint64_t val;
} FPC_ERR_CLR_t;

// FPC_ERR_FRC desc:
typedef union {
    struct {
        uint64_t              err_frc  : 56; // Force the associated bit in FPC_ERR_STS
        uint64_t       reserved_63_56  :  8; // reserved
    } field;
    uint64_t val;
} FPC_ERR_FRC_t;

// FPC_ERR_EN_HOST desc:
typedef union {
    struct {
        uint64_t               err_en  : 56; // Enable the associated bit in the FPC_ERR_STS to propagate interrupts
        uint64_t       reserved_63_56  :  8; // reserved
    } field;
    uint64_t val;
} FPC_ERR_EN_HOST_t;

// FPC_ERR_FIRST_HOST desc:
typedef union {
    struct {
        uint64_t            err_first  : 56; // The associated bit in the FPC_ERR_STS CSR was the first error captured.
        uint64_t       reserved_63_56  :  8; // reserved
    } field;
    uint64_t val;
} FPC_ERR_FIRST_HOST_t;

// FPC_ERR_INFO_PORTRCV desc:
typedef union {
    struct {
        uint64_t               e_code  :  8; // One of the following errors occurred. [E0] Reserved [E1] BadPktLen: Illegal PktLen [E2] PktLenTooLong: Packet longer than PktLen [E3] PktLenTooShort: Packet shorter than PktLen with normal tail [E4] BadSLID: Illegal SLID (0, using multicast as SLID. Does not include security validation of SLID) [E5] BadDLID: Illegal DLID (0, doesn't match HFI) [E6] BadL2: Illegal L2 opcode [E7] BadSC: Unsupported SC [E8] Reserved [E9] Headless: Tail or Body before Head. [E10] Reserved [E11] PreemptError: Preempting with same VL [E12] PreemptVL15: Preempting a VL15 packet [E13] BadSC Maker [E14] PreemptL2Header:Interleaving L2 Header [E15] Reserved
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
        uint64_t               e_code  :  8; // [E0] BadHeadDist: Distance violation between two head flits [E1] BadTailDist: Distance violation between two tail flits [E2] BadCtrlDist: Distance violation between two credit control flits [E3] BadCrdtAck: Credits return for unsupported VL see FPC_ERR_INFO_FLOW_CTRL [E4] Unsupported VL: VL Marker received for unsupported VL [E5] BadPreempt: Exceeded the preemption nesting level (gen1), or implict vl when only explicit is allowed (gen 2). This results in a headless in gen2. [E6] BadCtrlFlit: Received an illegal control flit. Depreciated [E7] ExceedMulitcast Limit: [E8] BadMarkerDist: Distance violation between two VL Markers, VL Marker and Head.
        uint64_t             distance  :  3; // distance for error codes [0],[2],[8]
        uint64_t                   vl  :  4; // vl for error codes [4]
        uint64_t         unused_63_15  : 49; // unused
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
        uint64_t               e_code  :  4; // [0] BadHead: uncorrectable error in head flit [1] BadBody: uncorrectable error in body flit [2] BadTail: uncorrectable error in tail flit [3] BadCtrl: uncorrectable error in credit flit. [4] Internal: Internal logic error , unrecoverable.
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

// FPC_ERR_DROP_ENABLE desc:
typedef union {
    struct {
        uint64_t             unused_0  :  1; // unused
        uint64_t               l2_err  :  1; // Setting to zero will prevent the dropping of this type
        uint64_t               sc_err  :  1; // Setting to zero will prevent the dropping of this type
        uint64_t         headless_err  :  1; // Not Configurable
        uint64_t             pkey_err  :  1; // Pkey error
        uint64_t  preempt_same_vl_err  :  1; // Not Configurable
        uint64_t            l2hdr_err  :  1; // Not Configurable
        uint64_t           sc_mkr_err  :  1; // Setting to zero will prevent the dropping of this type.
        uint64_t         unsup_vl_err  :  1; // Not Configurable
        uint64_t             unused_9  :  1; // unused
        uint64_t            unused_10  :  1; // unused
        uint64_t            unused_11  :  1; // unused
        uint64_t            unused_12  :  1; // unused
        uint64_t    uncorrectable_err  :  1; // Not Configurable
        uint64_t         crdt_ack_err  :  1; // Not Configurable
        uint64_t       unsup_pkt_type  :  1; // Setting to zero will prevent the dropping of this type
        uint64_t   crdt_sb_parity_err  :  1; // Not Configurable
        uint64_t          mbe_inbound  :  1; // Not Configurable
        uint64_t event_cntr_rollover_err  :  1; // Not Configurable
        uint64_t             link_err  :  1; // Not Configurable
        uint64_t         mkr_dist_err  :  1; // Not Configurable
        uint64_t        ctrl_dist_err  :  1; // Not Configurable
        uint64_t        tail_dist_err  :  1; // Not Configurable
        uint64_t        head_dist_err  :  1; // Not Configurable
        uint64_t    nonvl15_state_err  :  1; // Setting to zero will prevent the dropping of this type.
        uint64_t       vl15_multi_err  :  1; // Setting to zero will prevent the dropping of this type.
        uint64_t       pkt_length_err  :  1; // Setting to zero will prevent the dropping of this type.
        uint64_t            unused_27  :  1; // unused
        uint64_t       perm_nvl15_err  :  1; // Setting to zero will prevent the dropping of this type.
        uint64_t        slid_zero_err  :  1; // Setting to zero will prevent the dropping of this type.
        uint64_t        dlid_zero_err  :  1; // Setting to zero will prevent the dropping of this type.
        uint64_t       length_mtu_err  :  1; // Setting to zero will prevent the dropping of this type.
        uint64_t         slid_sec_err  :  1; // Setting to zero will prevent the dropping of this type.
        uint64_t            unused_33  :  1; // unused
        uint64_t       late_short_err  :  1; // Not Configurable
        uint64_t        late_long_err  :  1; // Not Configurable
        uint64_t            unused_36  :  1; // Not Configurable
        uint64_t       ltp_buf_unflow  :  1; // Not Configurable
        uint64_t       ltp_buf_ovflow  :  1; // Not Configurable
        uint64_t         unused_51_39  : 13; // unused
        uint64_t   portconstraint_err  :  1; // Not Configurable
        uint64_t uncorrectable_err_53  :  1; // Not Configurable
        uint64_t         fmconfig_err  :  1; // Not Configurable
        uint64_t          rcvport_err  :  1; // Not Configurable
        uint64_t         unused_63_56  :  8; // unused
    } field;
    uint64_t val;
} FPC_ERR_DROP_ENABLE_t;

// FPC_CFG_PORT_CONFIG desc:
typedef union {
    struct {
        uint64_t pkt_formats_supported  :  4; // Bit Mask - Supported packet format types bit 0 - 8B bit 1 - 9B bit 2 - 10B bit 3 - 16B
        uint64_t  pkt_formats_enabled  :  4; // Bit Mask - Enabled packet format types bit 0 - 8B bit 1 - 9B bit 2 - 10B bit 3 - 16B
        uint64_t            freeze_en  :  1; // When enabled: For the following errors FPC will terminate all outstanding packets and block new flits from OC. distance errors headless error preemption error inactive_vl_mkr error pkt_too_long When disabled: When one of the above errors occur outstanding packets are terminated, flits/per vl are dropped until head boundary.
        uint64_t inbound_mbe_freeze_en  :  1; // When enabled: FPC will block new packets from entering from OC. It will terminate packets in flight to INQ. A port bounce is require to un-freeze FPC. When disabled: flits with MBE's will be forwarded.
        uint64_t       reserved_15_10  :  6; // reserved
        uint64_t              pkey_en  :  1; // Enable Pkey checking
        uint64_t            pkey_mode  :  1; // Switch = 0, HFI = 1. For switches: If the membership bit is 1 in the resulting P_Key and the membership bit is not 1 in the corresponding P_Key table entry For HFIs: If the membership bit is 0 in the resulting P_Key and the membership bit is 0 in the corresponding P_Key table entry (eg. limited talking to limited)
        uint64_t       reserved_31_18  : 14; // reserved
        uint64_t              vl0_mtu  :  3; // MTU for this VL, Packets containing LRH length greater than the MTU_CAP will be discarded. 0x0 = 0 (vl disabled, no packets allowed) 0x1 = 256 bytes + Header: 8B,9B,10B,16B = 48 flits 0x2 = 512 bytes + Header: 8B,9B,10B,16B = 80 flits 0x3 = 1024 bytes + Header: 8B,9B,10B,16B = 144 flits 0x4 = 2048 bytes + header: 8B,9B,10B,16B = 272 flits 0x5 = 4096 bytes + header: 8B,9B,10B,16B = 528 flits 0x6 = 8192 bytes + Header: 8B,9B,10B,16B = 1040 flits 0x7 = 10240 bytes + Header:8B,9B,10B,16B = 1296 flits
        uint64_t              vl1_mtu  :  3; // MTU for VL1
        uint64_t              vl2_mtu  :  3; // MTU for VL2
        uint64_t              vl3_mtu  :  3; // MTU for VL3
        uint64_t              vl4_mtu  :  3; // MTU for VL4
        uint64_t              vl5_mtu  :  3; // MTU for VL5
        uint64_t              vl6_mtu  :  3; // MTU for VL6
        uint64_t              vl7_mtu  :  3; // MTU for VL7
        uint64_t              vl8_mtu  :  3; // MTU for VL8
        uint64_t             vl15_mtu  :  3; // MTU for VL15
        uint64_t       reserved_63_62  :  2; // reserved
    } field;
    uint64_t val;
} FPC_CFG_PORT_CONFIG_t;

// FPC_CFG_PORT_CONFIG1 desc:
typedef union {
    struct {
        uint64_t          multi_mask8  :  7; // 7 bit mask used to filter 8/10B packets for multicast lids. (pkt.dlid[20:14] & Mask8) ^ Mask8) == 0
        uint64_t           reserved_7  :  1; // reserved
        uint64_t          multi_mask9  :  7; // 7 bit mask used to filter 9B packets for multicast lids. ((pkt.dlid[15:9] & Mask9) ^ Mask9) == 0 Note bit [11:8] must be zero. FW should only manipulate bits [14:12]
        uint64_t          reserved_15  :  1; // reserved
        uint64_t         multi_mask16  :  7; // 7 bit mask used to filter 16B packets for multicast lids. ((pkt.dlid[23:17] & Mask16) ^ Mask16) == 0
        uint64_t       reserved_31_23  :  9; // reserved
        uint64_t          b_sc_enable  : 32; // Enable BECN interrupts, per SC ( HFI only ) bit[32] - SC0, bit [63] - SC31
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
        uint64_t         unused_19_16  :  4; // 
        uint64_t             pkey_10b  : 12; // The Pkey Check Control register is ignored for STL 9B and 16B packets. STL 10B: Packet Pkey = {Pkey_Check_10B[31:20],Pkt:Pkey[3:0]}
        uint64_t         unused_63_32  : 32; // 
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
        uint64_t      parity_err_addr  :  7; // Memory address where the parity error occurred.
        uint64_t          errinj_addr  :  7; // Ram address to inject error on. 0x0 - 0x7F
        uint64_t        errinj_parity  :  4; // Parity mask.
        uint64_t          errinj_mode  :  2; // 0x0 - Inject error once 0x1- 0x3 reserved
        uint64_t            errinj_en  :  1; // Error injection enabled.
        uint64_t         errinj_reset  :  1; // Resets error injection control logic. When written to a '1' a single pulse is generated and a '0' is written back to the register.This needs to be asserted between errors injected.
        uint64_t     errinj_triggered  :  1; // Error was injected.
        uint64_t       reserved_63_26  : 38; // reserved
    } field;
    uint64_t val;
} FPC_CFG_EVENT_CNTR_CTRL_t;

// FPC_CFG_RESET desc:
typedef union {
    struct {
        uint64_t            blk_reset  :  1; // Reset to FPC blocks, excluding fpc_common
        uint64_t        reserved_63_1  : 63; // reserved
    } field;
    uint64_t val;
} FPC_CFG_RESET_t;

// FPC_CFG_SLID_SECURITY desc:
typedef union {
    struct {
        uint64_t        neighbor_slid  : 24; // Neighbor SLID filter register
        uint64_t             slid_lmc  :  4; // Neighbor SLID LMC register. 0x0 = Full SLID compare 0x1 = disable check on SLID bit 0 0x2 = disable check on SLID bits 1:0 :: 0xF = disable check on SLID bits 14:0
        uint64_t      neighbor_device  :  1; // Type of Neighbor device connected to this port. 0 = Neighbor port is a switch port 1 = Neighbor port is an HFI
        uint64_t         unused_30_29  :  2; // unused
        uint64_t               enable  :  1; // 0 = Disable Security Feature 1 = Enable Security Feature N/A for HFI, must be disabled.
        uint64_t         unused_63_32  : 32; // unused
    } field;
    uint64_t val;
} FPC_CFG_SLID_SECURITY_t;

// FPC_STS_BECN_SC_RCVD desc:
typedef union {
    struct {
        uint64_t           b_sc0_rcvd  :  1; // Becn packet received with (SC == n) e.g SC-0 bit[0] SC-31 [bit31]
        uint64_t           b_sc1_rcvd  :  1; // 
        uint64_t           b_sc2_rcvd  :  1; // 
        uint64_t           b_sc3_rcvd  :  1; // 
        uint64_t           b_sc4_rcvd  :  1; // 
        uint64_t           b_sc5_rcvd  :  1; // 
        uint64_t           b_sc6_rcvd  :  1; // 
        uint64_t           b_sc7_rcvd  :  1; // 
        uint64_t           b_sc8_rcvd  :  1; // 
        uint64_t           b_sc9_rcvd  :  1; // 
        uint64_t          b_sc10_rcvd  :  1; // 
        uint64_t          b_sc11_rcvd  :  1; // 
        uint64_t          b_sc12_rcvd  :  1; // 
        uint64_t          b_sc13_rcvd  :  1; // 
        uint64_t          b_sc14_rcvd  :  1; // 
        uint64_t          b_sc15_rcvd  :  1; // 
        uint64_t          b_sc16_rcvd  :  1; // 
        uint64_t          b_sc17_rcvd  :  1; // 
        uint64_t          b_sc18_rcvd  :  1; // 
        uint64_t          b_sc19_rcvd  :  1; // 
        uint64_t          b_sc20_rcvd  :  1; // 
        uint64_t          b_sc21_rcvd  :  1; // 
        uint64_t          b_sc22_rcvd  :  1; // 
        uint64_t          b_sc23_rcvd  :  1; // 
        uint64_t          b_sc24_rcvd  :  1; // 
        uint64_t          b_sc25_rcvd  :  1; // 
        uint64_t          b_sc26_rcvd  :  1; // 
        uint64_t          b_sc27_rcvd  :  1; // 
        uint64_t          b_sc28_rcvd  :  1; // 
        uint64_t          b_sc29_rcvd  :  1; // 
        uint64_t          b_sc30_rcvd  :  1; // 
        uint64_t          b_sc31_rcvd  :  1; // 
        uint64_t         unused_63_32  : 32; // unused
    } field;
    uint64_t val;
} FPC_STS_BECN_SC_RCVD_t;

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

// FPC_PRF_PORTRCV_SC_BECN desc:
typedef union {
    struct {
        uint64_t                  cnt  : 64; // number of packets received with the becn bit set.
    } field;
    uint64_t val;
} FPC_PRF_PORTRCV_SC_BECN_t;

// FPC_PRF_PORTRCV_SBE_IN desc:
typedef union {
    struct {
        uint64_t                  cnt  : 64; // Total number of SBE
    } field;
    uint64_t val;
} FPC_PRF_PORTRCV_SBE_IN_t;

// FPC_PRF_PORTRCV_SBE_OUT desc:
typedef union {
    struct {
        uint64_t                  cnt  : 64; // Total number of SBE
    } field;
    uint64_t val;
} FPC_PRF_PORTRCV_SBE_OUT_t;

// FPC_PRF_PORTRCV_MBE_IN desc:
typedef union {
    struct {
        uint64_t                  cnt  : 64; // Number of MBE's
    } field;
    uint64_t val;
} FPC_PRF_PORTRCV_MBE_IN_t;

#endif /* ___FXR_lm_fpc_CSRS_H__ */
