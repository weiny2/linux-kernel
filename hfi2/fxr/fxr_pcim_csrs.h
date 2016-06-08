// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Thu Jun  2 19:11:24 2016
//

#ifndef ___FXR_pcim_CSRS_H__
#define ___FXR_pcim_CSRS_H__

// PCIM_CFG desc:
typedef union {
    struct {
        uint64_t           ITR_ECOUNT  :  8; // ITR Entry Count (max entries used-1)
        uint64_t           ITR_PERIOD  : 11; // ITR update period. Countdown counter to control time period between ITR updates.
        uint64_t       Reserved_23_19  :  5; // Reserved
        uint64_t     REMAP_REQ_THRESH  :  3; // Remap Request Credit threshold. Max number of interrupt remap request credits used by PCIM.Max allowed value is 4'h4.
        uint64_t          Reserved_27  :  1; // Reserved
        uint64_t      ENA_PCIERROR_CA  :  1; // Enable PCIError signaling for p2sb_np_unsup_req event
        uint64_t      ENA_PCIERROR_UR  :  1; // Enable PCIError signaling for p2sb_p_unsup_req event
        uint64_t    ENA_PCIERROR_CMPL  :  1; // Enable PCIError signaling for p2sb_np_addr_err event
        uint64_t          Reserved_31  :  1; // Reserved
        uint64_t       MIN_FLR_PERIOD  : 10; // Minimum FLR Period. Defines the minimum period in 1.2Ghz clocks that Driver Reset or FLR reset is asserted to HW.
        uint64_t       Reserved_62_42  : 21; // Reserved
        uint64_t         DRIVER_RESET  :  1; // Driver Reset. A write of 1 initiates Driver Level Reset to FXR. The value read by software for this bit is always 0.
    } field;
    uint64_t val;
} PCIM_CFG_t;

// PCIM_CFG_TARB desc:
typedef union {
    struct {
        uint64_t       POSTED_CREDITS  :  8; // TARB Posted Credits
        uint64_t    NONPOSTED_CREDITS  :  8; // TARB NonPosted Credits
        uint64_t   COMPLETION_CREDITS  :  8; // TARB Completion Credits
        uint64_t        TOTAL_CREDITS  :  8; // TARB Total Credits
        uint64_t       Reserved_63_32  : 32; // Reserved
    } field;
    uint64_t val;
} PCIM_CFG_TARB_t;

// PCIM_CFG_MARB0 desc:
typedef union {
    struct {
        uint64_t      HEAD_CMP_THRESH  :  6; // Completion Header threshold
        uint64_t         Reserved_7_6  :  2; // Reserved
        uint64_t       HEAD_NP_THRESH  :  6; // Non-Posted Header threshold
        uint64_t       Reserved_15_14  :  2; // Reserved
        uint64_t        HEAD_P_THRESH  :  6; // Posted Header threshold
        uint64_t       Reserved_23_22  :  2; // Reserved
        uint64_t      DATA_CMP_THRESH  :  6; // Completion Data threshold
        uint64_t       Reserved_31_30  :  2; // Reserved
        uint64_t       DATA_NP_THRESH  :  6; // Non-Posted Data threshold
        uint64_t       Reserved_39_38  :  2; // Reserved
        uint64_t        DATA_P_THRESH  :  6; // Posted Data threshold
        uint64_t       Reserved_63_46  : 18; // Reserved
    } field;
    uint64_t val;
} PCIM_CFG_MARB0_t;

// PCIM_CFG_MARB1 desc:
typedef union {
    struct {
        uint64_t         ARB_PRIORITY  : 48; // Arbitration Priority
        uint64_t       Reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} PCIM_CFG_MARB1_t;

// PCIM_CFG_HPI desc:
typedef union {
    struct {
        uint64_t       TARB_FC_THRESH  :  5; // TARB Flow control threshold
        uint64_t         Reserved_7_5  :  3; // Reserved
        uint64_t     HIFIS_CREDIT_ENA  :  1; // Enable HIFIS credit control
        uint64_t        Reserved_63_9  : 55; // Reserved
    } field;
    uint64_t val;
} PCIM_CFG_HPI_t;

// PCIM_STS_HPI desc:
typedef union {
    struct {
        uint64_t        HIFIS_CREDITS  :  5; // Current HIFIS credits
        uint64_t        Reserved_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} PCIM_STS_HPI_t;

// PCIM_CFG_P2SB desc:
typedef union {
    struct {
        uint64_t        HEADER_THRESH  :  5; // Header Queue Full Threshold
        uint64_t         Reserved_7_5  :  3; // Reserved
        uint64_t     NP_HEADER_THRESH  :  5; // Non-Posted Header Queue Full Threshold
        uint64_t       Reserved_15_13  :  3; // Reserved
        uint64_t        P_DATA_THRESH  :  5; // Posted Data Queue Full Threshold
        uint64_t       Reserved_23_21  :  3; // Reserved
        uint64_t       NP_DATA_THRESH  :  5; // Non-Posted Data Queue Full threshold
        uint64_t       Reserved_31_29  :  3; // Reserved
        uint64_t      CMP_DATA_THRESH  :  5; // Completion Data Queue Full Threshold
        uint64_t       Reserved_39_37  :  3; // Reserved
        uint64_t         CSR_REQ_TYPE  :  1; // CSR Access type: 1-Memory Mapped, 0-Control Register
        uint64_t           SB_SAI_ENA  :  1; // Enable SAI signaling on FXR-SB
        uint64_t       Reserved_63_42  : 22; // Reserved
    } field;
    uint64_t val;
} PCIM_CFG_P2SB_t;

// PCIM_CFG_SBTO desc:
typedef union {
    struct {
        uint64_t         Reserved_7_0  :  8; // Reserved
        uint64_t                COUNT  : 24; // FXR-SB timeout count (x256 clocks)
        uint64_t       Reserved_63_32  : 32; // Reserved
    } field;
    uint64_t val;
} PCIM_CFG_SBTO_t;

// PCIM_STS_BDF desc:
typedef union {
    struct {
        uint64_t     iosf_function_id  :  3; // IOSF defined Function ID (strap input)
        uint64_t       iosf_device_id  :  5; // IOSF defined Device ID (strap input)
        uint64_t          iosf_bus_id  :  8; // IOSF defined Bus ID (captured)
        uint64_t       Reserved_63_16  : 48; // Reserved
    } field;
    uint64_t val;
} PCIM_STS_BDF_t;

// PCIM_CFG_WAKE desc:
typedef union {
    struct {
        uint64_t     GPSB_HOLD_PERIOD  : 32; // GPSB Wake Hold Period (clks). Defines the number of 1.2Ghz clocks that PCIM will hold the GPSB WAKE signaling to Power Management after going idle at the GPSB interface. Dampens signaling so that WAKE is de-asserted only after N consecutive clocks of being Idle. The default value of 1200 equates to a 1us period.
        uint64_t     IOSF_HOLD_PERIOD  : 32; // IOSF Wake Hold Period (clks). Defines the number of 1.2Ghz clocks that PCIM will hold the IOSF WAKE signaling to Power Management after going idle at the IOSF-P interface. Dampens signaling so that WAKE is de-asserted only after N consecutive clocks of being Idle. The default value of 1200 equates to a 1us period.
    } field;
    uint64_t val;
} PCIM_CFG_WAKE_t;

// PCIM_ERR_STS desc:
typedef union {
    struct {
        uint64_t          itr_read_pe  :  1; // ITR read parity error. See itr_rpe_entry .
        uint64_t        msix_addr_mbe  :  1; // MISX Table Addr MBE
        uint64_t        msix_data_mbe  :  1; // MISX Table Data MBE
        uint64_t        msix_addr_sbe  :  1; // MISX Table Addr SBE
        uint64_t        msix_data_sbe  :  1; // MISX Table Data SBE
        uint64_t      marb_cmp_hdr_uf  :  1; // MARB Completion Header credit Underflow
        uint64_t      marb_cmp_hdr_of  :  1; // MARB Completion Header credit Overflow
        uint64_t       marb_np_hdr_uf  :  1; // MARB NonPosted Header credit Underflow
        uint64_t       marb_np_hdr_of  :  1; // MARB NonPosted Header credit Overflow
        uint64_t        marb_p_hdr_uf  :  1; // MARB Posted Header credit Underflow
        uint64_t        marb_p_hdr_of  :  1; // MARB Posted Header credit Overflow
        uint64_t     marb_cmp_data_uf  :  1; // MARB Completion Data credit Underflow
        uint64_t     marb_cmp_data_of  :  1; // MARB Completion Data credit Overflow
        uint64_t      marb_np_data_uf  :  1; // MARB NonPosted Data credit Underflow
        uint64_t      marb_np_data_of  :  1; // MARB NonPosted Data credit Overflow
        uint64_t       marb_p_data_uf  :  1; // MARB Posted Data credit Underflow
        uint64_t       marb_p_data_of  :  1; // MARB Posted Data credit Overflow
        uint64_t  hpi_data_parity_err  :  1; // HPI Data Parity Error
        uint64_t  hpi_data_poison_err  :  1; // HPI Data Poison Error
        uint64_t           gpsb_error  :  1; // GPSB Endpoint Error
        uint64_t     p2sb_p_unsup_req  :  1; // P2SB Posted Unsupported Request (PCIError asserted)
        uint64_t    p2sb_np_unsup_req  :  1; // P2SB NonPosted Unsupported Request
        uint64_t     p2sb_np_addr_err  :  1; // P2SB NonPosted request Address Error
        uint64_t       p2sb_unexp_cmp  :  1; // P2SB Unexpected Completion
        uint64_t       p2sb_p_data_ep  :  1; // P2SB Posted Data poison
        uint64_t   p2sb_p_data_parity  :  1; // P2SB Posted Data Parity error
        uint64_t      p2sb_np_data_ep  :  1; // P2SB NonPosted Data poison
        uint64_t  p2sb_np_data_parity  :  1; // P2SB NonPosted Data Parity error
        uint64_t      p2sb_p_addr_err  :  1; // P2SB Posted request Address Error
        uint64_t    p2sb_data_len_err  :  1; // P2SB Data Length Error
        uint64_t     p2sb_np_read_err  :  1; // P2SB NonPosted read size error
        uint64_t      p2sb_dataq0_mbe  :  1; // P2SB Data Queue0 MBE (PCIError asserted)
        uint64_t      p2sb_dataq0_sbe  :  1; // P2SB Data Queue0 SBE
        uint64_t      p2sb_dataq1_mbe  :  1; // P2SB Data Queue1 MBE (PCIError asserted)
        uint64_t      p2sb_dataq1_sbe  :  1; // P2SB Data Queue1 SBE
        uint64_t    sb2p_unsup_pc_fmt  :  1; // SB2P Unsupport Posted/Cmp Format
        uint64_t    sb2p_unsup_pc_opc  :  1; // SB2P Unsupport Posted/Cmp Opcode
        uint64_t    sb2p_unsup_np_fmt  :  1; // SB2P Unsupport NonPosted Format
        uint64_t    sb2p_unsup_np_opc  :  1; // SB2P Unsupport NonPosted Opcode
        uint64_t    tarb_hdr_ep_error  :  1; // TARB Request Header with EP (poison) set (PCIError asserted)
        uint64_t tarb_hdr_parity_error  :  1; // TARB Request Header Parity Error (PCIError asserted)
        uint64_t       Reserved_63_41  : 23; // Reserved
    } field;
    uint64_t val;
} PCIM_ERR_STS_t;

// PCIM_ERR_CLR desc:
typedef union {
    struct {
        uint64_t               events  : 43; // Write 1's to clear corresponding PCIM_ERR_STS bits.
        uint64_t       Reserved_63_43  : 21; // Reserved
    } field;
    uint64_t val;
} PCIM_ERR_CLR_t;

// PCIM_ERR_FRC desc:
typedef union {
    struct {
        uint64_t               events  : 43; // Write 1 to set corresponding PCIM_ERR_STS bits.
        uint64_t       Reserved_63_43  : 21; // Reserved
    } field;
    uint64_t val;
} PCIM_ERR_FRC_t;

// PCIM_ERR_EN_HOST desc:
typedef union {
    struct {
        uint64_t               events  : 43; // Enables corresponding PCIM_ERR_STS bits to generate host interrupt signal.
        uint64_t       Reserved_63_43  : 21; // Reserved
    } field;
    uint64_t val;
} PCIM_ERR_EN_HOST_t;

// PCIM_ERR_FIRST_HOST desc:
typedef union {
    struct {
        uint64_t               events  : 43; // Snapshot of PCIM_ERR_STS bits when host interrupt signal transitions from 0 to 1.
        uint64_t       Reserved_63_43  : 21; // Reserved
    } field;
    uint64_t val;
} PCIM_ERR_FIRST_HOST_t;

// PCIM_ERR_EN_BMC desc:
typedef union {
    struct {
        uint64_t               events  : 43; // Enable corresponding PCIM_ERR_STS bits to generate BMC interrupt signal.
        uint64_t       Reserved_63_43  : 21; // Reserved
    } field;
    uint64_t val;
} PCIM_ERR_EN_BMC_t;

// PCIM_ERR_FIRST_BMC desc:
typedef union {
    struct {
        uint64_t               events  : 43; // Snapshot of PCIM_ERR_STS bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t       Reserved_63_43  : 21; // Reserved
    } field;
    uint64_t val;
} PCIM_ERR_FIRST_BMC_t;

// PCIM_ERR_EN_QUAR desc:
typedef union {
    struct {
        uint64_t               events  : 43; // Enable corresponding PCIM_ERR_STS bits to generate quarantine signal.
        uint64_t       Reserved_63_43  : 21; // Reserved
    } field;
    uint64_t val;
} PCIM_ERR_EN_QUAR_t;

// PCIM_ERR_FIRST_QUAR desc:
typedef union {
    struct {
        uint64_t               events  : 43; // Snapshot of PCIM_ERR_STS bits when quarantine signal transitions from 0 to 1.
        uint64_t       Reserved_63_43  : 21; // Reserved
    } field;
    uint64_t val;
} PCIM_ERR_FIRST_QUAR_t;

// PCIM_ERR_INFO desc:
typedef union {
    struct {
        uint64_t       msix_addr_synd  :  8; // MSIX Table Address syndrome associated with msix_addr_sbe / msix_addr_mbe
        uint64_t       msix_data_synd  :  7; // MSIX Table Data syndrome associated with msix_data_sbe / msix_data_mbe
        uint64_t          reserved_15  :  1; // Reserved
        uint64_t           p2sb_synd1  :  7; // P2SB Data1 syndrome associated with p2sb_dataq1_sbe / p2sb_dataq1_mbe
        uint64_t          reserved_23  :  1; // Reserved
        uint64_t           p2sb_synd0  :  9; // P2SB Data0 syndrome associated with p2sb_dataq0_sbe / p2sb_dataq0_mbe
        uint64_t        remap_rsp_tid  :  9; // MISX entry ID associated with remap_dma_fault / remap_int_fault
        uint64_t       Reserved_63_42  : 22; // Reserved
    } field;
    uint64_t val;
} PCIM_ERR_INFO_t;

// PCIM_ERR_INFO1 desc:
typedef union {
    struct {
        uint64_t          hpi_header0  : 64; // HPI Header0 Info
    } field;
    uint64_t val;
} PCIM_ERR_INFO1_t;

// PCIM_ERR_INFO2 desc:
typedef union {
    struct {
        uint64_t          hpi_header1  : 64; // HPI Header1 Info
    } field;
    uint64_t val;
} PCIM_ERR_INFO2_t;

// PCIM_ERR_INFO3 desc:
typedef union {
    struct {
        uint64_t         p2sb_header0  : 64; // P2SB Header0 Info
    } field;
    uint64_t val;
} PCIM_ERR_INFO3_t;

// PCIM_ERR_INFO4 desc:
typedef union {
    struct {
        uint64_t         p2sb_header1  : 64; // P2SB Header1 Info
    } field;
    uint64_t val;
} PCIM_ERR_INFO4_t;

// PCIM_ERR_INFO5 desc:
typedef union {
    struct {
        uint64_t         tarb_header0  : 64; // TARB Header0 Info
    } field;
    uint64_t val;
} PCIM_ERR_INFO5_t;

// PCIM_ERR_INFO6 desc:
typedef union {
    struct {
        uint64_t         tarb_header1  : 64; // TARB Header1 Info
    } field;
    uint64_t val;
} PCIM_ERR_INFO6_t;

// PCIM_ERR_INFO7 desc:
typedef union {
    struct {
        uint64_t        itr_rpe_entry  :  8; // ITR Read Parity Error Entry ID. Valid when itr_read_pe is set
        uint64_t        reserved_63_8  : 56; // Reserved
    } field;
    uint64_t val;
} PCIM_ERR_INFO7_t;

// PCIM_SAI_BOOTW desc:
typedef union {
    struct {
        uint64_t                spare  : 64; // spare
    } field;
    uint64_t val;
} PCIM_SAI_BOOTW_t;

// PCIM_BMC_STS desc:
typedef union {
    struct {
        uint64_t            MNH_ERROR  :  1; // Manor Hill IRQ
        uint64_t           FZC0_ERROR  :  1; // Frozen Creek Port0 IRQ
        uint64_t           FZC1_ERROR  :  1; // Frozen Creek Port1 IRQ
        uint64_t          HIFIS_ERROR  :  1; // HIFIS Domain Error
        uint64_t           LOCA_ERROR  :  1; // HI Domain Error
        uint64_t           PCIM_ERROR  :  1; // PCIM Domain Error
        uint64_t           TXCI_ERROR  :  1; // TXCI Domain Error
        uint64_t            OTR_ERROR  :  1; // OTR Domain Error
        uint64_t          TXDMA_ERROR  :  1; // TXDMA Domain Error
        uint64_t             LM_ERROR  :  1; // LM Domain Error
        uint64_t          RXE2E_ERROR  :  1; // RXE2E Domain Error
        uint64_t           RXHP_ERROR  :  1; // RXHP Domain Error
        uint64_t          RXDMA_ERROR  :  1; // RXDMA Domain Error
        uint64_t           RXET_ERROR  :  1; // RXET Domain Error
        uint64_t        RXHIARB_ERROR  :  1; // RXHIARB Domain Error
        uint64_t             AT_ERROR  :  1; // AT Domain Error
        uint64_t           OPIO_ERROR  :  1; // OPIO Domain Error
        uint64_t           RXCI_ERROR  :  1; // RXCI Domain Error
        uint64_t        LM_FPC0_ERROR  :  1; // LM FPC0 Domain Error
        uint64_t        LM_FPC1_ERROR  :  1; // LM FPC1 Domain Error
        uint64_t         LM_TP0_ERROR  :  1; // LM TP0 Domain Error
        uint64_t         LM_TP1_ERROR  :  1; // LM TP1 Domain Error
        uint64_t      PMON_CORE_ERROR  :  1; // PMON CORE (1.2Ghz) Domain Error
        uint64_t       reserved_63_23  : 41; // Reserved
    } field;
    uint64_t val;
} PCIM_BMC_STS_t;

// PCIM_BMC_CLR desc:
typedef union {
    struct {
        uint64_t               events  : 23; // Write 1's to clear corresponding PCIM_BMC_STS bits.
        uint64_t       Reserved_63_23  : 41; // Reserved
    } field;
    uint64_t val;
} PCIM_BMC_CLR_t;

// PCIM_BMC_FRC desc:
typedef union {
    struct {
        uint64_t               events  : 23; // Write 1 to set corresponding PCIM_BMC_STS bits.
        uint64_t       Reserved_63_23  : 41; // Reserved
    } field;
    uint64_t val;
} PCIM_BMC_FRC_t;

// PCIM_BMC_ENA desc:
typedef union {
    struct {
        uint64_t               events  : 23; // Enables corresponding PCIM_BMC_STS bits to generate BMC interrupt signal.
        uint64_t       Reserved_63_23  : 41; // Reserved
    } field;
    uint64_t val;
} PCIM_BMC_ENA_t;

// PCIM_BMC_FIRST desc:
typedef union {
    struct {
        uint64_t               events  : 23; // Snapshot of PCIM_BMC_STS bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t       Reserved_63_23  : 41; // Reserved
    } field;
    uint64_t val;
} PCIM_BMC_FIRST_t;

// PCIM_INT_STS desc:
typedef union {
    struct {
        uint64_t              INT_STS  : 64; // FXR Interrupt Status (see Table 28-32 )
    } field;
    uint64_t val;
} PCIM_INT_STS_t;

// PCIM_INT_CLR desc:
typedef union {
    struct {
        uint64_t              INT_CLR  : 64; // Clear INT_STS
    } field;
    uint64_t val;
} PCIM_INT_CLR_t;

// PCIM_INT_FRC desc:
typedef union {
    struct {
        uint64_t              INT_FRC  : 64; // Force INT_STS
    } field;
    uint64_t val;
} PCIM_INT_FRC_t;

// PCIM_INT_ITR desc:
typedef union {
    struct {
        uint64_t            INT_DELAY  : 16; // INT Delay. Setting the Delay to a non-zero value enables ITR throttling function for the associated event interrupt. The Delay value represents the minimum time enforced by HW between signaled interrupts for the associated event. The ITR Delay and counter granularity is 1us. Small Delay values yield higher messages rates, large Delay values yield lower message rates. Ex: A Delay value of 1 allows up to 1M signaled messages/sec. A Delay value 100 allows up to 10,000 messages/sec. A Delay value of 0 disables the throttling function to allow the max possible signaling rate.
        uint64_t            ITR_TIMER  : 16; // ITR Timer. Timer is set to INT_DELAY value when the associated interrupt is signaled. Timer is periodically (1us) decremented by HW if count is non-zero.
        uint64_t              ITR_ENA  :  1; // ITR Enabled. Set by HW when INT_CNT reaches zero to enable a pending event or the next received event to signal the associated interrupt. Cleared by HW when the associated interrupt is signaled if INT_THRESH is non-zero.
        uint64_t              ITR_PAR  :  1; // Parity bit for HW use only
        uint64_t       Reserved_63_34  : 30; // Reserved
    } field;
    uint64_t val;
} PCIM_INT_ITR_t;

// PCIM_MSIX_ADDR desc:
typedef union {
    struct {
        uint64_t             MSG_ADDR  : 64; // MSIX Message Address
    } field;
    uint64_t val;
} PCIM_MSIX_ADDR_t;

// PCIM_MSIX_DATACTL desc:
typedef union {
    struct {
        uint64_t             MSG_DATA  : 32; // MSIX Message Data
        uint64_t             MSG_MASK  :  1; // MSIX Message Mask Control
        uint64_t       Reserved_63_33  : 31; // Reserved
    } field;
    uint64_t val;
} PCIM_MSIX_DATACTL_t;

// PCIM_MSIX_PBA desc:
typedef union {
    struct {
        uint64_t             MSIX_PBA  : 64; // MSIX entry PBA bits [64N+63 : 64N]
    } field;
    uint64_t val;
} PCIM_MSIX_PBA_t;

#endif /* ___FXR_pcim_CSRS_H__ */