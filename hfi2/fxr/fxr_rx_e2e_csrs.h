// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Thu May 19 17:39:38 2016
//

#ifndef ___FXR_rx_e2e_CSRS_H__
#define ___FXR_rx_e2e_CSRS_H__

// RXE2E_CFG_PSN_BASE_ADDR_P0_TC desc:
typedef union {
    struct {
        uint64_t        Reserved_11_0  : 12; // Reserved
        uint64_t              address  : 45; // Host Memory Base Address, aligned to page boundary
        uint64_t       Reserved_63_57  :  7; // Reserved
    } field;
    uint64_t val;
} RXE2E_CFG_PSN_BASE_ADDR_P0_TC_t;

// RXE2E_CFG_PSN_BASE_ADDR_P1_TC desc:
typedef union {
    struct {
        uint64_t        Reserved_11_0  : 12; // Reserved
        uint64_t              address  : 45; // Host Memory Base Address, aligned to page boundary
        uint64_t       Reserved_63_57  :  7; // Reserved
    } field;
    uint64_t val;
} RXE2E_CFG_PSN_BASE_ADDR_P1_TC_t;

// RXE2E_CFG_VALID_TC_SLID desc:
typedef union {
    struct {
        uint64_t          max_slid_p0  : 24; // Port 0 max valid slid, incoming packets with slid > than this are dropped and no PSN cache lookup is done. Note: slid > max_slid_p0 will abort CACHE_CMD_RD, CACHE_CMD_WR in RXE2E_CFG_PSN_CACHE_ACCESS_CTL and set bad_addr field in RXE2E_CFG_PSN_CACHE_ACCESS_CTL
        uint64_t          tc_valid_p0  :  4; // Port 0 TC valid bits, 1 bit per TC, invalid incoming TC packets are dropped and no PSN cache lookup is done. Note: Invalid tc will abort CACHE_CMD_RD, CACHE_CMD_WR in RXE2E_CFG_PSN_CACHE_ACCESS_CTL and set bad_addr field in RXE2E_CFG_PSN_CACHE_ACCESS_CTL
        uint64_t          max_slid_p1  : 24; // Port 1 max valid slid, incoming packets with slid > than this are dropped and no PSN cache lookup is done. Note: slid > max_slid_p1 will abort CACHE_CMD_RD, CACHE_CMD_WR in RXE2E_CFG_PSN_CACHE_ACCESS_CTL and set bad_addr field in RXE2E_CFG_PSN_CACHE_ACCESS_CTL
        uint64_t          tc_valid_p1  :  4; // Port 1 TC valid bits, 1 bit per TC, invalid incoming TC packets are dropped and no PSN cache lookup is done. Note: Invalid tc will abort CACHE_CMD_RD, CACHE_CMD_WR in RXE2E_CFG_PSN_CACHE_ACCESS_CTL and set bad_addr field in RXE2E_CFG_PSN_CACHE_ACCESS_CTL
        uint64_t       Reserved_63_56  :  8; // Reserved
    } field;
    uint64_t val;
} RXE2E_CFG_VALID_TC_SLID_t;

// RXE2E_CFG_SPECIAL_ACK_RATE desc:
typedef union {
    struct {
        uint64_t special_ack_rate_code  :  3; // The meaning of the following formula is whenever the expected unordered psn moves to or past a 2**special_ack_rate_code multiple due to a scoreboard shift, a special ack is sent. Formula for when to do a special ack: special_ack_rate[11:0] = 2 ** special_ack_rate_code; special_ack_mask[11:0] = -special_ack_rate; do_special_ack = (old_unordered_psn[11:0] & ~special_ack_mask[11:0]) != (new_unordered_psn[11:0] & ~special_ack_mask[11:0]) if all unordered packets arrived in order, the special ack rate would be every (2 ** special_ack_rate_code) packets. Note: If SW writes a 6, or 7 for special_ack_rate_code, it will be interpreted as 5 which is a rate of 32. 5 may cause a small amount of dropped packets if E2E is not using a big scoreboard. Again, both big_max_psn_distance and small_max_psn_distance must be >= 2 ** special_ack_rate_code.
        uint64_t        Reserved_63_3  : 61; // Reserved
    } field;
    uint64_t val;
} RXE2E_CFG_SPECIAL_ACK_RATE_t;

// RXE2E_CFG_PORT_MODE desc:
typedef union {
    struct {
        uint64_t                 MODE  :  2; // Port Operational Mode. See <blue text>Section 10.9.8, 'Integrated Multi-Rail Support' of the Portals Transport Layer Specification for a description of each mode of operation. 2'b00 - Disjoint Mode. 2'b01 - Grouped Mode
        uint64_t        Reserved_63_2  : 62; // Unused
    } field;
    uint64_t val;
} RXE2E_CFG_PORT_MODE_t;

// RXE2E_CFG_LMC desc:
typedef union {
    struct {
        uint64_t               lmc_p0  :  2; // Port 0 number of low DLID bits masked by network for packet delivery.
        uint64_t         Reserved_3_2  :  2; // Unused
        uint64_t               lmc_p1  :  2; // Port 1 number of low DLID bits masked by network for packet delivery.
        uint64_t        Reserved_63_6  : 58; // Unused
    } field;
    uint64_t val;
} RXE2E_CFG_LMC_t;

// RXE2E_CFG_LM_CREDIT_FUND_MAX desc:
typedef union {
    struct {
        uint64_t MC0_TC0_CRDT_FUND_MAX  :  5; // MC0 TC0 Credit Fund Max
        uint64_t MC0_TC1_CRDT_FUND_MAX  :  5; // MC0 TC1 Credit Fund Max
        uint64_t MC0_TC2_CRDT_FUND_MAX  :  5; // MC0 TC2 Credit Fund Max
        uint64_t MC0_TC3_CRDT_FUND_MAX  :  5; // MC0 TC3 Credit Fund Max
        uint64_t MC1_TC0_CRDT_FUND_MAX  :  5; // MC1 TC0 Credit Fund Max
        uint64_t MC1_TC1_CRDT_FUND_MAX  :  5; // MC2 TC1 Credit Fund Max
        uint64_t MC1_TC2_CRDT_FUND_MAX  :  5; // MC1 TC2 Credit Fund Max
        uint64_t MC1_TC3_CRDT_FUND_MAX  :  5; // MC1 TC3 Credit Fund Max
        uint64_t       Reserved_63_40  : 24; // Unused
    } field;
    uint64_t val;
} RXE2E_CFG_LM_CREDIT_FUND_MAX_t;

// RXE2E_CFG_PSN_CACHE_CLIENT_DISABLE desc:
typedef union {
    struct {
        uint64_t client_psn_cache_request_disable  :  1; // Client psn cache request disable.
        uint64_t        Reserved_63_1  : 63; // Unused
    } field;
    uint64_t val;
} RXE2E_CFG_PSN_CACHE_CLIENT_DISABLE_t;

// RXE2E_CFG_CREDIT_ENABLE desc:
typedef union {
    struct {
        uint64_t rxhp_hdr_flit_credit_enable  :  1; // Enable rxhp hdr flit credits.
        uint64_t rxhp_pkt_status_credit_enable  :  1; // Enable rxhp pkt status credits.
        uint64_t otr_hdr_flit_credit_enable  :  1; // Enable otr hdr flit credits.
        uint64_t otr_pkt_status_credit_enable  :  1; // Enable otr pkt status credits.
        uint64_t  hiarb_credit_enable  :  1; // Enable hiarb request credits.
        uint64_t        Reserved_63_5  : 59; // Unused
    } field;
    uint64_t val;
} RXE2E_CFG_CREDIT_ENABLE_t;

// RXE2E_CFG_RC_ORDERED_MAP desc:
typedef union {
    struct {
        uint64_t              ordered  :  8; // 1 bit = ordered packet 0 bit = unordered packet rc mapping to ordered
        uint64_t        Reserved_63_8  : 56; // Unused
    } field;
    uint64_t val;
} RXE2E_CFG_RC_ORDERED_MAP_t;

// RXE2E_CFG_PORT_MIRROR desc:
typedef union {
    struct {
        uint64_t       port_mirror_on  :  1; // Turn on if in Port Mirroring Mode. This should be set identically to fxr_rx_dma equivalent (in RXDMA_CFG_PORT_MIRROR ) and fxr_rx_hp equivalent (in RXHP_CFG_CTL ).
        uint64_t        Reserved_63_1  : 63; // Unused
    } field;
    uint64_t val;
} RXE2E_CFG_PORT_MIRROR_t;

// RXE2E_CFG_PSN_CACHE_ACCESS_CTL desc:
typedef union {
    struct {
        uint64_t              address  : 29; // The contents of this field differ based on cmd. psn_cache_addr_t for CACHE_CMD_RD, CACHE_CMD_WR,CACHE_CMD_INVALIDATE,CACHE_CMD_FLUSH* 10:0 used for CACHE_CMD_TAG_RD,CACHE_CMD_TAG_WR 14:0 used for CACHE_CMD_DATA_RD, CACHE_CMD_DATA_WR Note: See <blue text>Section 7.1.2.2.3, 'Packet Sequence Number Table Structures' for how client addresses are formed.
        uint64_t         mask_address  : 29; // cache cmd mask address. 1 bits are don't care. only used for CACHE_CMD_INVALIDATE, CACHE_CMD_FLUSH_INVALID, CACHE_CMD_FLUSH_VALID The form of this field is psn_cache_addr_t. initially all 1's. Note: If this field is 0, a normal single lookup is done. If non-zero, the entire cache tag memory is read and tag entries match/masked.
        uint64_t                  cmd  :  4; // cache cmd. see <blue text>Section A.1.1, 'FXR Cache Cmd enums' initially CACHE_CMD_INVALIDATE
        uint64_t                 busy  :  1; // SW sets busy when writing this csr. HW clears busy when cmd is complete. busy must be clear before writing this csr. If this busy is set , HW is busy on a previous cmd. Coming out of reset, this will be 1 so as to initiate the psn cache tag invalidation. Then in 2k clks or so, it will go to 0.
        uint64_t             bad_addr  :  1; // if cmd == CACHE_CMD_RD or CACHE_CMD_WR, the address.tc is checked for valid and address.lid is checked for in range. See RXE2E_CFG_VALID_TC_SLID . No other cache_cmd sets this bit. If the check fails, no psn cache lookup occurs and this bit gets set.
    } field;
    uint64_t val;
} RXE2E_CFG_PSN_CACHE_ACCESS_CTL_t;

// RXE2E_CFG_PSN_CACHE_ACCESS_DATA desc:
typedef union {
    struct {
        uint64_t     small_scoreboard  : 29; // The dedicated small scoreboard for tracking a window of out-of-order PSNs or a 10b pointer (in bits 9:0) into a larger scoreboard table depending on the big_in_use bit. If representing a small scoreboard, the 29 bits map to the psn offsets (from unordered_psn) 29:1. (offset 0, where the unordered_psn points, has an assumed scoreboard value of 0.).
        uint64_t           big_in_use  :  1; // Whether the small_scoreboard field contains a small scoreboard (0) or a pointer to larger scoreboard (1)
        uint64_t        unordered_psn  : 16; // expected PSN (window base) for unordered packets
        uint64_t          ordered_psn  : 16; // expected PSN for ordered packets
        uint64_t missing_ordered_seen  :  1; // This indicates whether a NACK has been sent to indicate that an ordered packet was received out-of-sequence. It is initialized to 0 and reset to 0 upon each successful in-sequence packet. It is set to 1 when an out-of-sequence NACK is sent. Subsequent out-of-sequence packets are dropped silently when they observe the field set to 1.
        uint64_t            connected  :  1; // current connection state
    } field;
    uint64_t val;
} RXE2E_CFG_PSN_CACHE_ACCESS_DATA_t;

// RXE2E_CFG_PSN_CACHE_ACCESS_DATA_BIT_ENABLE desc:
typedef union {
    struct {
        uint64_t     small_scoreboard  : 29; // The dedicated small scoreboard for tracking a window of out-of-order PSNs or a 10b pointer (in bits 9:0) into a larger scoreboard table depending on the big_in_use bit. If representing a small scoreboard, the 29 bits map to the psn offsets (from unordered_psn) 29:1. (offset 0, where the unordered_psn points, has an assumed scoreboard value of 0.).
        uint64_t           big_in_use  :  1; // Whether the small_scoreboard field contains a small scoreboard (0) or a pointer to larger scoreboard (1)
        uint64_t        unordered_psn  : 16; // expected PSN (window base) for unordered packets
        uint64_t          ordered_psn  : 16; // expected PSN for ordered packets
        uint64_t missing_ordered_seen  :  1; // This indicates whether a NACK has been sent to indicate that an ordered packet was received out-of-sequence. It is initialized to 0 and reset to 0 upon each successful in-sequence packet. It is set to 1 when an out-of-sequence NACK is sent. Subsequent out-of-sequence packets are dropped silently when they observe the field set to 1.
        uint64_t            connected  :  1; // current connection state
    } field;
    uint64_t val;
} RXE2E_CFG_PSN_CACHE_ACCESS_DATA_BIT_ENABLE_t;

// RXE2E_CFG_PSN_CACHE_ACCESS_TAG desc:
typedef union {
    struct {
        uint64_t      tag_way_addr_lo  : 29; // psn cache tag way address, format is psn_cache_addr_t
        uint64_t     tag_way_valid_lo  :  1; // psn cache tag way valid
        uint64_t           reserve_lo  :  2; // 
        uint64_t      tag_way_addr_hi  : 29; // psn cache tag way address, format is psn_cache_addr_t
        uint64_t     tag_way_valid_hi  :  1; // psn cache tag way valid
        uint64_t           reserve_hi  :  2; // 
    } field;
    uint64_t val;
} RXE2E_CFG_PSN_CACHE_ACCESS_TAG_t;

// RXE2E_CFG_MC0_HDR_FLIT_CNT_CAM desc:
typedef union {
    struct {
        uint64_t               cnt_m1  :  3; // Number of header flits to send to RXHP - 1
        uint64_t          opcode_mask  :  7; // opcode mask (1 bits are don't care)
        uint64_t              l4_mask  :  8; // L4 mask (1 bits are don't care)
        uint64_t            ni_1_mask  :  1; // NI[1] mask (1 bits are don't care)
        uint64_t              hd_mask  :  1; // HD mask (1 bits are don't care)
        uint64_t           l2ext_mask  :  4; // L2ext mask (1 bits are don't care)
        uint64_t              l2_mask  :  2; // L2 mask (1 bits are don't care)
        uint64_t               opcode  :  7; // opcode matching
        uint64_t                   l4  :  8; // L4 matching
        uint64_t                 ni_1  :  1; // NI[1] matching
        uint64_t                   hd  :  1; // HD matching
        uint64_t                l2ext  :  4; // L2ext matching, use when L2 == HDR_EXT
        uint64_t                   l2  :  2; // L2 matching
        uint64_t                valid  :  1; // Cam entry is valid
        uint64_t       Reserved_63_50  : 14; // Reserved
    } field;
    uint64_t val;
} RXE2E_CFG_MC0_HDR_FLIT_CNT_CAM_t;

// RXE2E_ERR_STS_1 desc:
typedef union {
    struct {
        uint64_t           diagnostic  :  1; // Diagnostic Error Flag
        uint64_t        mc0_lm_in_mbe  :  1; // MC0 LINKMUX input mbe Error information: Section 29.13.4.11, 'RXE2E Error Info MC0 LM Input SBE/MBE'
        uint64_t        mc0_lm_in_sbe  :  1; // MC0 LINKMUX input sbe Error information: . Section 29.13.4.11, 'RXE2E Error Info MC0 LM Input SBE/MBE'
        uint64_t        mc1_lm_in_mbe  :  1; // MC1 LINKMUX input mbe Error information: Section 29.13.4.12, 'RXE2E Error Info MC1 LM Input SBE/MBE'
        uint64_t        mc1_lm_in_sbe  :  1; // MC1 LINKMUX input sbe Error information: . Section 29.13.4.12, 'RXE2E Error Info MC1 LM Input SBE/MBE'
        uint64_t   mc0_input_fifo_mbe  :  1; // MC0 input fifo mbe Error information: Section 29.13.4.16, 'RXE2E Error Info PSN Cache Tag MBE'
        uint64_t   mc0_input_fifo_sbe  :  1; // MC0 input fifo sbe Error information: . Section 29.13.4.16, 'RXE2E Error Info PSN Cache Tag MBE'
        uint64_t   mc1_input_fifo_mbe  :  1; // MC1 input fifo mbe Error information: Section 29.13.4.13, 'RXE2E Error Info MC0 Input Fifo SBE/MBE'
        uint64_t   mc1_input_fifo_sbe  :  1; // MC1 input fifo sbe Error information: . Section 29.13.4.13, 'RXE2E Error Info MC0 Input Fifo SBE/MBE'
        uint64_t    psn_cache_tag_mbe  :  1; // PSN Cache tag mbe Error information: Section 29.13.4.16, 'RXE2E Error Info PSN Cache Tag MBE' Note: these are fairly fatal as you don't know what connection is bad.
        uint64_t    psn_cache_tag_sbe  :  1; // PSN Cache tag sbe Error information: . Section 29.13.4.15, 'RXE2E Error Info PSN Cache Tag SBE'
        uint64_t   psn_cache_data_mbe  :  1; // PSN Cache data mbe Error information: Section 29.13.4.17, 'RXE2E Error Info PSN Cache Data SBE/MBE' Note: This will result in auto-disconnect for the connection. This may also remove a big scoreboard slot from being re-allocated as you can't rely on either the big_in_use bit or the big_scoreboard pointer in the data. So if a big scoreboard was in use, that slot will remain unavailabe for re-use until the next hard reset.
        uint64_t   psn_cache_data_sbe  :  1; // PSN Cache data sbe Error information: . Section 29.13.4.17, 'RXE2E Error Info PSN Cache Data SBE/MBE'
        uint64_t               MC0crc  :  1; // CRC error on MC0 Error information: . Section 29.13.4.20, 'RXE2E Error Info CRC'
        uint64_t               MC1crc  :  1; // CRC error on MC1 Error information: . Section 29.13.4.20, 'RXE2E Error Info CRC'
        uint64_t   big_scoreboard_mbe  :  1; // big scoreboard mbe. Error information: Section 29.13.4.19, 'RXE2E Error Info Big Scoreboard MBE'
        uint64_t   big_scoreboard_sbe  :  1; // big scoreboard sbe. Error information: Section 29.13.4.18, 'RXE2E Error Info Big Scoreboard SBE'
        uint64_t     any_mbe_cntr_max  :  1; // Some mbe cntr is saturated (all 1's). Error information: Section 29.13.4.10, 'RXE2E SBE/MBE Err Counter Summary Status'
        uint64_t any_mbe_cntr_non_zero  :  1; // Some mbe cntr is non-zero. Error information: Section 29.13.4.10, 'RXE2E SBE/MBE Err Counter Summary Status'
        uint64_t     any_sbe_cntr_max  :  1; // Some sbe cntr is saturated (all 1's). Error information: Section 29.13.4.10, 'RXE2E SBE/MBE Err Counter Summary Status'
        uint64_t any_sbe_cntr_non_zero  :  1; // Some sbe cntr is non-zero. Error information: Section 29.13.4.10, 'RXE2E SBE/MBE Err Counter Summary Status'
        uint64_t       Reserved_63_21  : 43; // Reserved
    } field;
    uint64_t val;
} RXE2E_ERR_STS_1_t;

// RXE2E_ERR_CLR_1 desc:
typedef union {
    struct {
        uint64_t               events  : 21; // Write 1's to clear corresponding status bits.
        uint64_t       Reserved_63_21  : 43; // Reserved
    } field;
    uint64_t val;
} RXE2E_ERR_CLR_1_t;

// RXE2E_ERR_FRC_1 desc:
typedef union {
    struct {
        uint64_t               events  : 21; // Write 1 to set corresponding status bits.
        uint64_t       Reserved_63_21  : 43; // Reserved
    } field;
    uint64_t val;
} RXE2E_ERR_FRC_1_t;

// RXE2E_ERR_EN_HOST_1 desc:
typedef union {
    struct {
        uint64_t               events  : 21; // Enables corresponding status bits to generate host interrupt signal.
        uint64_t       Reserved_63_21  : 43; // Reserved
    } field;
    uint64_t val;
} RXE2E_ERR_EN_HOST_1_t;

// RXE2E_ERR_FIRST_HOST_1 desc:
typedef union {
    struct {
        uint64_t               events  : 21; // Snapshot of status bits when host interrupt signal transitions from 0 to 1.
        uint64_t       Reserved_63_21  : 43; // Reserved
    } field;
    uint64_t val;
} RXE2E_ERR_FIRST_HOST_1_t;

// RXE2E_ERR_EN_BMC_1 desc:
typedef union {
    struct {
        uint64_t               events  : 21; // Enable corresponding status bits to generate BMC interrupt signal.
        uint64_t       Reserved_63_21  : 43; // Reserved
    } field;
    uint64_t val;
} RXE2E_ERR_EN_BMC_1_t;

// RXE2E_ERR_FIRST_BMC_1 desc:
typedef union {
    struct {
        uint64_t               events  : 21; // Snapshot of status bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t       Reserved_63_21  : 43; // Reserved
    } field;
    uint64_t val;
} RXE2E_ERR_FIRST_BMC_1_t;

// RXE2E_ERR_EN_QUAR_1 desc:
typedef union {
    struct {
        uint64_t               events  : 21; // Enable corresponding status bits to generate quarantine signal.
        uint64_t       Reserved_63_21  : 43; // Reserved
    } field;
    uint64_t val;
} RXE2E_ERR_EN_QUAR_1_t;

// RXE2E_ERR_FIRST_QUAR_1 desc:
typedef union {
    struct {
        uint64_t               events  : 21; // Snapshot of status bits when quarantine signal transitions from 0 to 1.
        uint64_t       Reserved_63_21  : 43; // Reserved
    } field;
    uint64_t val;
} RXE2E_ERR_FIRST_QUAR_1_t;

// RXE2E_ERR_SBE_MBE_CNTR_SUMMARY_STATUS desc:
typedef union {
    struct {
        uint64_t some_mbe_cntr_non_zero  :  1; // Some mbe cntr in the error info regs is non-zero.
        uint64_t    some_mbe_cntr_max  :  1; // Some mbe cntr in the error info regs has saturated to all 1's
        uint64_t some_sbe_cntr_non_zero  :  1; // Some sbe cntr in the error info regs is non-zero.
        uint64_t    some_sbe_cntr_max  :  1; // Some sbe cntr in the error info regs has saturated to all 1's
        uint64_t        Reserved_63_4  : 60; // Reserved
    } field;
    uint64_t val;
} RXE2E_ERR_SBE_MBE_CNTR_SUMMARY_STATUS_t;

// RXE2E_ERR_INFO_MC0_LM_INPUT_SBE_MBE desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain mbe
        uint64_t      mbe_last_domain  :  2; // ecc domain of last least significant mbe
        uint64_t                  mbe  :  4; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            mbe_count  :  8; // saturating counter of mbes. The increment signal is the 'or' of the 4 mbe signals.
        uint64_t       Reserved_31_22  : 10; // 
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain sbe
        uint64_t      sbe_last_domain  :  2; // ecc domain of last least significant sbe
        uint64_t                  sbe  :  4; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            sbe_count  :  8; // saturating counter of sbes. The increment signal is the 'or' of the 4 sbe signals.
        uint64_t       Reserved_63_54  : 10; // Reserved
    } field;
    uint64_t val;
} RXE2E_ERR_INFO_MC0_LM_INPUT_SBE_MBE_t;

// RXE2E_ERR_INFO_MC1_LM_INPUT_SBE_MBE desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain mbe
        uint64_t      mbe_last_domain  :  2; // ecc domain of last least significant mbe
        uint64_t                  mbe  :  4; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            mbe_count  :  4; // saturating counter of mbes. The increment signal is the 'or' of the 4 mbe signals.
        uint64_t       Reserved_31_18  : 14; // 
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain sbe
        uint64_t      sbe_last_domain  :  2; // ecc domain of last least significant sbe
        uint64_t                  sbe  :  4; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            sbe_count  : 12; // saturating counter of sbes. The increment signal is the 'or' of the 4 sbe signals.
        uint64_t       Reserved_63_58  :  6; // Reserved
    } field;
    uint64_t val;
} RXE2E_ERR_INFO_MC1_LM_INPUT_SBE_MBE_t;

// RXE2E_ERR_INFO_MC0_INPUT_SBE_MBE desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain mbe
        uint64_t      mbe_last_domain  :  2; // ecc domain of last least significant mbe
        uint64_t                  mbe  :  4; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            mbe_count  :  4; // saturating counter of mbes. The increment signal is the 'or' of the 4 mbe signals.
        uint64_t     mbe_last_address  :  6; // address of the last least significant ecc domain mbe
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain sbe
        uint64_t      sbe_last_domain  :  2; // ecc domain of last least significant sbe
        uint64_t                  sbe  :  4; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            sbe_count  : 12; // saturating counter of sbes. The increment signal is the 'or' of the 4 sbe signals.
        uint64_t     sbe_last_address  :  6; // address of the last least significant ecc domain sbe
        uint64_t       Reserved_63_56  :  8; // Reserved
    } field;
    uint64_t val;
} RXE2E_ERR_INFO_MC0_INPUT_SBE_MBE_t;

// RXE2E_ERR_INFO_MC1_INPUT_SBE_MBE desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain mbe
        uint64_t      mbe_last_domain  :  2; // ecc domain of last least significant mbe
        uint64_t                  mbe  :  4; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            mbe_count  :  4; // saturating counter of mbes. The increment signal is the 'or' of the 4 mbe signals.
        uint64_t     mbe_last_address  :  6; // address of the last least significant ecc domain mbe
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain sbe
        uint64_t      sbe_last_domain  :  2; // ecc domain of last least significant sbe
        uint64_t                  sbe  :  4; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            sbe_count  : 12; // saturating counter of sbes. The increment signal is the 'or' of the 4 sbe signals.
        uint64_t     sbe_last_address  :  6; // address of the last least significant ecc domain sbe
        uint64_t       Reserved_63_56  :  8; // Reserved
    } field;
    uint64_t val;
} RXE2E_ERR_INFO_MC1_INPUT_SBE_MBE_t;

// RXE2E_ERR_INFO_PSN_CACHE_TAG_SBE desc:
typedef union {
    struct {
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain sbe
        uint64_t      sbe_last_domain  :  3; // ecc domain of last least significant sbe
        uint64_t                  sbe  :  8; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            sbe_count  : 12; // saturating counter of sbes. The increment signal is the 'or' of the 8 sbe signals.
        uint64_t     sbe_last_address  : 11; // address of the last least significant ecc domain sbe
        uint64_t       Reserved_63_42  : 22; // Reserved
    } field;
    uint64_t val;
} RXE2E_ERR_INFO_PSN_CACHE_TAG_SBE_t;

// RXE2E_ERR_INFO_PSN_CACHE_TAG_MBE desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain mbe
        uint64_t      mbe_last_domain  :  3; // ecc domain of last least significant mbe
        uint64_t                  mbe  :  8; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            mbe_count  :  4; // saturating counter of mbes. The increment signal is the 'or' of the 8 mbe signals.
        uint64_t     mbe_last_address  : 11; // address of the last least significant ecc domain mbe
        uint64_t       Reserved_63_34  : 30; // Reserved
    } field;
    uint64_t val;
} RXE2E_ERR_INFO_PSN_CACHE_TAG_MBE_t;

// RXE2E_ERR_INFO_PSN_CACHE_DATA_SBE_MBE desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain mbe
        uint64_t            mbe_count  :  4; // saturating counter of mbes.
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain sbe
        uint64_t            sbe_count  : 12; // saturating counter of sbes.
        uint64_t       Reserved_63_32  : 32; // Reserved
    } field;
    uint64_t val;
} RXE2E_ERR_INFO_PSN_CACHE_DATA_SBE_MBE_t;

// RXE2E_ERR_INFO_BIG_SCOREBOARD_SBE desc:
typedef union {
    struct {
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain sbe
        uint64_t      sbe_last_domain  :  4; // ecc domain of last least significant sbe
        uint64_t                  sbe  : 16; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            sbe_count  : 12; // saturating counter of sbes. The increment signal is the 'or' of the 16 sbe signals.
        uint64_t     sbe_last_address  : 10; // address of the last least significant ecc domain sbe
        uint64_t       Reserved_63_50  : 14; // Reserved
    } field;
    uint64_t val;
} RXE2E_ERR_INFO_BIG_SCOREBOARD_SBE_t;

// RXE2E_ERR_INFO_BIG_SCOREBOARD_MBE desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain mbe
        uint64_t      mbe_last_domain  :  4; // ecc domain of last least significant mbe
        uint64_t                  mbe  : 16; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            mbe_count  :  4; // saturating counter of mbes. The increment signal is the 'or' of the 16 mbe signals.
        uint64_t     mbe_last_address  : 10; // address of the last least significant ecc domain mbe
        uint64_t       Reserved_63_42  : 22; // Reserved
    } field;
    uint64_t val;
} RXE2E_ERR_INFO_BIG_SCOREBOARD_MBE_t;

// RXE2E_ERR_INFO_CRC desc:
typedef union {
    struct {
        uint64_t          mc0_crc_cnt  : 16; // Saturating count of mc0 crc errors.
        uint64_t          mc1_crc_cnt  : 16; // Saturating count of mc1 crc errors.
        uint64_t       Reserved_63_32  : 32; // Reserved
    } field;
    uint64_t val;
} RXE2E_ERR_INFO_CRC_t;

// RXE2E_ERR_INJECT_SBE_MBE desc:
typedef union {
    struct {
        uint64_t mc0_input_fifo_err_inj_mask  :  8; // mC0 input fifo error inject mask
        uint64_t mc0_input_fifo_err_inj_domain  :  2; // mc0 input fifo error inject domain. Which ecc domain to inject the error.
        uint64_t mc0_input_fifo_err_inj_enable  :  1; // mc0 input fifo error inject enable.
        uint64_t mc1_input_fifo_err_inj_mask  :  8; // mc1 input fifo error inject mask
        uint64_t mc1_input_fifo_err_inj_domain  :  2; // mc1 input fifo error inject domain. Which ecc domain to inject the error.
        uint64_t mc1_input_fifo_err_inj_enable  :  1; // mc1 input fifo error inject enable.
        uint64_t psn_cache_tag_err_inj_mask  :  8; // psn_cache_tag error inject mask
        uint64_t psn_cache_tag_err_inj_domain  :  3; // psn_cache_tag error inject domain. Which ecc domain to inject the error.
        uint64_t psn_cache_tag_err_inj_enable  :  1; // psn_cache_tag error inject enable.
        uint64_t psn_cache_data_err_inj_mask  :  8; // psn_cache_data error inject mask
        uint64_t psn_cache_data_err_inj_enable  :  1; // psn_cache_data error inject enable.
        uint64_t big_scoreboard_err_inj_mask  :  8; // big scoreboard error inject mask
        uint64_t big_scoreboard_err_inj_domain  :  4; // big scoreboard error inject domain. Which ecc domain to inject the error.
        uint64_t big_scoreboard_err_inj_enable  :  1; // big scoreboard error inject enable.
        uint64_t       Reserved_63_56  :  8; // Reserved
    } field;
    uint64_t val;
} RXE2E_ERR_INJECT_SBE_MBE_t;

// RXE2E_DBG_PSN_CACHE_TAG_WAY_ENABLE desc:
typedef union {
    struct {
        uint64_t           way_enable  : 16; // 1 bits enable, 0 bits disable PSN Cache Tag ways.
        uint64_t       Reserved_63_16  : 48; // Reserved
    } field;
    uint64_t val;
} RXE2E_DBG_PSN_CACHE_TAG_WAY_ENABLE_t;

// RXE2E_DBG_BIG_SCOREBOARD_ACCESS_ADDR desc:
typedef union {
    struct {
        uint64_t              address  : 10; // Address of Big Scoreboard location to be accessed
        uint64_t       Reserved_61_10  : 52; // 
        uint64_t                write  :  1; // 0=Read, 1=Write
        uint64_t                 busy  :  1; // Set by Software to start command, cleared by Hardware when complete
    } field;
    uint64_t val;
} RXE2E_DBG_BIG_SCOREBOARD_ACCESS_ADDR_t;

// RXE2E_DBG_BIG_SCOREBOARD_ACCESS_DATA desc:
typedef union {
    struct {
        uint64_t             big_data  : 64; // Big scoreboard data.
    } field;
    uint64_t val;
} RXE2E_DBG_BIG_SCOREBOARD_ACCESS_DATA_t;

#endif /* ___FXR_rx_e2e_CSRS_H__ */