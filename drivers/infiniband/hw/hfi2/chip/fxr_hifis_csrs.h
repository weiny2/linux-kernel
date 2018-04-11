// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Wed Apr 11 12:49:08 2018
//

#ifndef ___FXR_hifis_CSRS_H__
#define ___FXR_hifis_CSRS_H__

// HIFIS_IN_CONTROL desc:
typedef union {
    struct {
        uint64_t          debug_spare  : 45; // For verification use
        uint64_t       Reserved_63_45  : 19; // Unused
    } field;
    uint64_t val;
} HIFIS_IN_CONTROL_t;

// HIFIS_IN_CFG desc:
typedef union {
    struct {
        uint64_t        Reserved_31_0  : 32; // Unused
        uint64_t at_hifis_pkt_credit_max  :  5; // AT to HIFIS interface Packet Credits. Packet credits on AT-to-HI interface. The Maximum value supported by HIFIS is 16 credits.
        uint64_t tx_hifis_pkt_credit_max  :  5; // TX to HIFIS interface Packet Credits. Packet credits on TX-to-HI interface. The Maximum value supported by HIFIS is 16 credits.
        uint64_t rx_hifis_pkt_credit_max  :  5; // RX to HIFIS interface Packet Credits. Packet credits on RX-to-HI interface. The Maximum value supported by HIFIS is 16 credits.
        uint64_t       Reserved_63_47  : 17; // Unused
    } field;
    uint64_t val;
} HIFIS_IN_CFG_t;

// HIFIS_OUT_CFG1 desc:
typedef union {
    struct {
        uint64_t          arb_cntl_at  : 16; // Defines arbitration weighting between loca ADM and loca IMI response paths to AT. Each set bit gives ADM path priority, and each clear bit gives IMI path priority. Default setting of 0xAAAA gives each source equal priority.
        uint64_t pcim_hifis_pkt_credit_max  :  6; // PCIM to HIFIS interface Packet Credits. Packet credits on PCIM-to-HIFIS interface.
        uint64_t credits_avail_at_rsp  :  7; // AT interface Packet Credits. Packet credits available on HI-to-AT response interface.
        uint64_t         credit_en_at  :  1; // Enable Credit Counter on HIFIS to AT INERFACE.
        uint64_t credits_avail_tx_rsp  :  6; // TX interface RSP Packet Credits. Packet credits available on HI-to-TX response interface.
        uint64_t credits_avail_tx_req  :  6; // TX interface REQ Packet Credits. Packet credits available on HI-to-TX request interface.
        uint64_t         credit_en_tx  :  1; // Enable Credit Counter on HIFIS to TX INERFACE.
        uint64_t credits_avail_rx_rsp  :  6; // RX interface Packet Credits. Packet credits available on HI-to-RX response interface.
        uint64_t credits_avail_rx_req  :  6; // RX interface Packet Credits. Packet credits available on HI-to-RX request interface.
        uint64_t         credit_en_rx  :  1; // Enable Credit Counter on HIFIS to AT INERFACE.
        uint64_t     port_arb_credits  :  7; // Credits for leaky arbitration.
        uint64_t          Reserved_63  :  1; // Unused
    } field;
    uint64_t val;
} HIFIS_OUT_CFG1_t;

// HIFIS_OUT_CFG2 desc:
typedef union {
    struct {
        uint64_t        arb_cntl_tx_b  : 16; // Defines arbitration weighting between loca (ADM and IMI) response paths and PCIM request path to TX. Each set bit give PCIM path priority, and each clear bit gives loca paths priority. Default setting of 0x4924, in conjunction with arb_cntl_tx_a default setting of 0xAAAA, gives each source PCIM, ADM, and IMI equal priority.
        uint64_t        arb_cntl_tx_a  : 16; // Defines arbitration weighting between loca ADM and loca IMI response paths to TX. Each set bit gives ADM path priority, and each clear bit gives IMI path priority. Default setting of 0xAAAA gives each source equal priority.
        uint64_t        arb_cntl_rx_b  : 16; // Defines arbitration weighting between loca (ADM and IMI) response paths and PCIM request path to RX. Each set bit give PCIM path priority, and each clear bit gives loca paths priority. Default setting of 0x4924, in conjunction with arb_cntl_rx_a default setting of 0xAAAA, gives each source PCIM, ADM, and IMI equal priority
        uint64_t        arb_cntl_rx_a  : 16; // Defines arbitration weighting between loca ADM and loca IMI response paths to RX. Each set bit gives ADM path priority, and each clear bit gives IMI path priority. Default setting of 0xAAAA gives each source equal priority.
    } field;
    uint64_t val;
} HIFIS_OUT_CFG2_t;

// HIFIS_ERR_STS desc:
typedef union {
    struct {
        uint64_t           pcim_q_err  :  1; // Underflow/overflow of PCIM Request Queue. ERR_CATEGORY_HFI
        uint64_t          imi_hdr_mbe  :  1; // IMI Header MBE. See imi_hdr_synd in HIFIS Error Info . ERR_CATEGORY_HFI
        uint64_t          imi_hdr_sbe  :  1; // IMI Header SBE. See imi_hdr_synd in HIFIS Error Info . ERR_CATEGORY_CORRECTABLE
        uint64_t          adm_hdr_mbe  :  1; // ADM Header MBE See adm_hdr_synd in HIFIS Error Info . ERR_CATEGORY_HFI
        uint64_t          adm_hdr_sbe  :  1; // ADM Header SBE. See adm_hdr_synd in HIFIS Error Info . ERR_CATEGORY_CORRECTABLE
        uint64_t     rx_req_inq_oflow  :  1; // RX Request In Queue Overflow error. ERR_CATEGORY_HFI
        uint64_t     tx_req_inq_oflow  :  1; // TX Request In Queue Overflow error. ERR_CATEGORY_HFI
        uint64_t     at_req_inq_oflow  :  1; // AT Request In Queue Overflow error. ERR_CATEGORY_HFI
        uint64_t        at_in_hdr_mbe  :  1; // AT in Header MBE ERR_CATEGORY_HFI
        uint64_t        at_in_hdr_sbe  :  1; // AT in Header SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t        tx_in_hdr_mbe  :  1; // TX in Header MBE ERR_CATEGORY_HFI
        uint64_t        tx_in_hdr_sbe  :  1; // TX in Header SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t        rx_in_hdr_mbe  :  1; // RX in Header MBE ERR_CATEGORY_HFI
        uint64_t        rx_in_hdr_sbe  :  1; // RX in Header SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t       Reserved_63_14  : 50; // Unused
    } field;
    uint64_t val;
} HIFIS_ERR_STS_t;

// HIFIS_ERR_CLR desc:
typedef union {
    struct {
        uint64_t               events  : 14; // Write 1's to clear corresponding HIFIS_ERR_STS bits.
        uint64_t       Reserved_63_14  : 50; // Unused
    } field;
    uint64_t val;
} HIFIS_ERR_CLR_t;

// HIFIS_ERR_FRC desc:
typedef union {
    struct {
        uint64_t               events  : 14; // Write 1 to set corresponding HIFIS_ERR_STS bits.
        uint64_t       Reserved_63_14  : 50; // Unused
    } field;
    uint64_t val;
} HIFIS_ERR_FRC_t;

// HIFIS_ERR_EN_HOST desc:
typedef union {
    struct {
        uint64_t               events  : 14; // Enables associated HIFIS_ERR_STS bits to generate host interrupt signal.
        uint64_t       Reserved_63_14  : 50; // Unused
    } field;
    uint64_t val;
} HIFIS_ERR_EN_HOST_t;

// HIFIS_ERR_FIRST_HOST desc:
typedef union {
    struct {
        uint64_t               events  : 14; // Snapshot of HIFIS_ERR_STS bits when host interrupt signal transitions from 0 to 1.
        uint64_t       Reserved_63_14  : 50; // Unused
    } field;
    uint64_t val;
} HIFIS_ERR_FIRST_HOST_t;

// HIFIS_ERR_EN_BMC desc:
typedef union {
    struct {
        uint64_t               events  : 14; // Enables associated HIFIS_ERR_STS bits to generate BMC interrupt signal.
        uint64_t       Reserved_63_14  : 50; // Unused
    } field;
    uint64_t val;
} HIFIS_ERR_EN_BMC_t;

// HIFIS_ERR_FIRST_BMC desc:
typedef union {
    struct {
        uint64_t               events  : 14; // Snapshot of status bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t       Reserved_63_14  : 50; // Unused
    } field;
    uint64_t val;
} HIFIS_ERR_FIRST_BMC_t;

// HIFIS_ERR_EN_QUAR desc:
typedef union {
    struct {
        uint64_t               events  : 14; // Enables associated HIFIS_ERR_STS bits to generate quarantine signal.
        uint64_t       Reserved_63_14  : 50; // Unused
    } field;
    uint64_t val;
} HIFIS_ERR_EN_QUAR_t;

// HIFIS_ERR_FIRST_QUAR desc:
typedef union {
    struct {
        uint64_t               events  : 14; // Snapshot of status bits when quarantine signal transitions from 0 to 1.
        uint64_t       Reserved_63_14  : 50; // Unused
    } field;
    uint64_t val;
} HIFIS_ERR_FIRST_QUAR_t;

// HIFIS_ERR_INFO desc:
typedef union {
    struct {
        uint64_t         imi_hdr_synd  :  8; // Header syndrome associated with imi_hdr_sbe / imi_hdr_mbe
        uint64_t         adm_hdr_synd  :  8; // Header syndrome associated with adm_hdr_sbe / adm_hdr_mbe
        uint64_t       at_in_hdr_synd  :  8; // Header syndrome associated with at_in_hdr_sbe/at_in_hdr_mbe
        uint64_t       tx_in_hdr_synd  :  8; // Header syndrome associated with tx_in_hdr_sbe/tx_in_hdr_mbe
        uint64_t       rx_in_hdr_synd  :  8; // Header syndrome associated with rx_in_hdr_sbe/rx_in_hdr_mbe
        uint64_t       Reserved_63_40  : 24; // Unused
    } field;
    uint64_t val;
} HIFIS_ERR_INFO_t;

#endif /* ___FXR_hifis_CSRS_H__ */
