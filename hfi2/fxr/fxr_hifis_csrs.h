// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Wed Jun 22 19:30:15 2016
//

#ifndef ___FXR_hifis_CSRS_H__
#define ___FXR_hifis_CSRS_H__

// HIFIS_IN_CONTROL desc:
typedef union {
    struct {
        uint64_t          Debug_spare  : 45; // For verification use
        uint64_t       reserved_63_45  : 19; // Reserved
    } field;
    uint64_t val;
} HIFIS_IN_CONTROL_t;

// HIFIS_IN_CFG desc:
typedef union {
    struct {
        uint64_t        reserved_31_0  : 32; // Reserved
        uint64_t AT_HIFIS_PKT_CREDIT_MAX  :  5; // AT to HIFIS interface Packet Credits. Packet credits on AT-to-HI interface
        uint64_t TX_HIFIS_PKT_CREDIT_MAX  :  5; // TX to HIFIS interface Packet Credits. Packet credits on TX-to-HI interface
        uint64_t RX_HIFIS_PKT_CREDIT_MAX  :  5; // RX to HIFIS interface Packet Credits. Packet credits on RX-to-HI interface
        uint64_t       reserved_63_47  : 17; // Reserved
    } field;
    uint64_t val;
} HIFIS_IN_CFG_t;

// HIFIS_OUT_CFG1 desc:
typedef union {
    struct {
        uint64_t          ARB_CNTL_AT  : 16; // HIFIS out to AT arbitration Priority.
        uint64_t PCIM_HIFIS_PKT_CREDIT_MAX  :  6; // PCIM to HIFIS interface Packet Credits. Packet credits on PCIM-to-HIFIS interface.
        uint64_t CREDITS_AVAIL_AT_RSP  :  7; // AT interface Packet Credits. Packet credits available on HI-to-AT response interface.
        uint64_t         CREDIT_EN_AT  :  1; // Enable Credit Counter on HIFIS to AT INERFACE.
        uint64_t CREDITS_AVAIL_TX_RSP  :  6; // TX interface RSP Packet Credits. Packet credits available on HI-to-TX response interface.
        uint64_t CREDITS_AVAIL_TX_REQ  :  6; // TX interface REQ Packet Credits. Packet credits available on HI-to-TX request interface.
        uint64_t         CREDIT_EN_TX  :  1; // Enable Credit Counter on HIFIS to TX INERFACE.
        uint64_t CREDITS_AVAIL_RX_RSP  :  6; // RX interface Packet Credits. Packet credits available on HI-to-RX response interface.
        uint64_t CREDITS_AVAIL_RX_REQ  :  6; // RX interface Packet Credits. Packet credits available on HI-to-RX request interface.
        uint64_t         CREDIT_EN_RX  :  1; // Enable Credit Counter on HIFIS to AT INERFACE.
        uint64_t       reserved_63_56  :  8; // Reserved
    } field;
    uint64_t val;
} HIFIS_OUT_CFG1_t;

// HIFIS_OUT_CFG2 desc:
typedef union {
    struct {
        uint64_t        ARB_CNTL_TX_B  : 16; // HIFIS out to TX arbitration Priority
        uint64_t        ARB_CNTL_TX_A  : 16; // HIFIS out to TX arbitration Priority
        uint64_t        ARB_CNTL_RX_B  : 16; // HIFIS out to RX arbitration Priority
        uint64_t        ARB_CNTL_RX_A  : 16; // HIFIS out to RX arbitration Priority
    } field;
    uint64_t val;
} HIFIS_OUT_CFG2_t;

// HIFIS_ERR_STS desc:
typedef union {
    struct {
        uint64_t           pcim_q_err  :  1; // Underflow/overflow of PCIM Request Queue
        uint64_t          imi_hdr_mbe  :  1; // IMI Header MBE. See imi_hdr_synd in HIFIS Error Info
        uint64_t          imi_hdr_sbe  :  1; // IMI Header SBE. See imi_hdr_synd in HIFIS Error Info
        uint64_t          adm_hdr_mbe  :  1; // ADM Header MBE See adm_hdr_synd in HIFIS Error Info
        uint64_t          adm_hdr_sbe  :  1; // ADM Header SBE. See adm_hdr_synd in HIFIS Error Info
        uint64_t        Reserved_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} HIFIS_ERR_STS_t;

// HIFIS_ERR_CLR desc:
typedef union {
    struct {
        uint64_t               events  : 64; // Write 1's to clear corresponding HIFIS_ERR_STS bits.
    } field;
    uint64_t val;
} HIFIS_ERR_CLR_t;

// HIFIS_ERR_FRC desc:
typedef union {
    struct {
        uint64_t               events  : 64; // Write 1 to set corresponding HIFIS_ERR_STS bits.
    } field;
    uint64_t val;
} HIFIS_ERR_FRC_t;

// HIFIS_ERR_EN_HOST desc:
typedef union {
    struct {
        uint64_t               events  :  5; // Enables associated HIFIS_ERR_STS bits to generate host interrupt signal.
        uint64_t        reserved_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} HIFIS_ERR_EN_HOST_t;

// HIFIS_ERR_FIRST_HOST desc:
typedef union {
    struct {
        uint64_t               events  :  5; // Snapshot of HIFIS_ERR_STS bits when host interrupt signal transitions from 0 to 1.
        uint64_t        reserved_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} HIFIS_ERR_FIRST_HOST_t;

// HIFIS_ERR_EN_BMC desc:
typedef union {
    struct {
        uint64_t               events  :  5; // Enables associated HIFIS_ERR_STS bits to generate BMC interrupt signal.
        uint64_t        reserved_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} HIFIS_ERR_EN_BMC_t;

// HIFIS_ERR_FIRST_BMC desc:
typedef union {
    struct {
        uint64_t               events  :  5; // Snapshot of status bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t        reserved_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} HIFIS_ERR_FIRST_BMC_t;

// HIFIS_ERR_EN_QUAR desc:
typedef union {
    struct {
        uint64_t               events  :  5; // Enables associated HIFIS_ERR_STS bits to generate quarantine signal.
        uint64_t        reserved_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} HIFIS_ERR_EN_QUAR_t;

// HIFIS_ERR_FIRST_QUAR desc:
typedef union {
    struct {
        uint64_t               events  :  5; // Snapshot of status bits when quarantine signal transitions from 0 to 1.
        uint64_t        reserved_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} HIFIS_ERR_FIRST_QUAR_t;

// HIFIS_ERR_INFO desc:
typedef union {
    struct {
        uint64_t         imi_hdr_synd  :  8; // MSIX Table Address syndrome associated with imi_hdr_sbe / imi_hdr_mbe
        uint64_t         adm_hdr_synd  :  8; // MSIX Table Data syndrome associated with adm_hdr_sbe / adm_hdr_mbe
        uint64_t       Reserved_63_16  : 48; // Reserved
    } field;
    uint64_t val;
} HIFIS_ERR_INFO_t;

#endif /* ___FXR_hifis_CSRS_H__ */