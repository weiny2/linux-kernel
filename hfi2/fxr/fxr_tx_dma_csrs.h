// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Wed Jun 22 19:30:15 2016
//

#ifndef ___FXR_tx_dma_CSRS_H__
#define ___FXR_tx_dma_CSRS_H__

// TXDMA_CFG_THRESHOLD desc:
typedef union {
    struct {
        uint64_t            threshold  : 10; // Count of beginning slots required to be assembled for packets before starting to send them out to the LM. A slot is 32 bytes. The default value of 0x20 indicates the first 1024 bytes of packets must be assembled in the TXDMA packet buffer before starting to send the packets to the LM.
        uint64_t       Reserved_63_10  : 54; // Unused
    } field;
    uint64_t val;
} TXDMA_CFG_THRESHOLD_t;

// TXDMA_CFG_BWMETER_MC0TC0 desc:
typedef union {
    struct {
        uint64_t leak_amount_fractional  :  8; // Fractional portion of leak amount
        uint64_t leak_amount_integral  :  3; // Integral portion of leak amount
        uint64_t       Reserved_14_11  :  4; // Unused
        uint64_t       enable_capping  :  1; // Enable bandwidth capping
        uint64_t      bandwidth_limit  : 16; // Bandwidth limit
        uint64_t       Reserved_63_32  : 32; // Unused
    } field;
    uint64_t val;
} TXDMA_CFG_BWMETER_MC0TC0_t;

// TXDMA_CFG_BWMETER_MC0TC1 desc:
typedef union {
    struct {
        uint64_t leak_amount_fractional  :  8; // Fractional portion of leak amount
        uint64_t leak_amount_integral  :  3; // Integral portion of leak amount
        uint64_t       Reserved_14_11  :  4; // Unused
        uint64_t       enable_capping  :  1; // Enable bandwidth capping
        uint64_t      bandwidth_limit  : 16; // Bandwidth limit
        uint64_t       Reserved_63_32  : 32; // Unused
    } field;
    uint64_t val;
} TXDMA_CFG_BWMETER_MC0TC1_t;

// TXDMA_CFG_BWMETER_MC0TC2 desc:
typedef union {
    struct {
        uint64_t leak_amount_fractional  :  8; // Fractional portion of leak amount
        uint64_t leak_amount_integral  :  3; // Integral portion of leak amount
        uint64_t       Reserved_14_11  :  4; // Unused
        uint64_t       enable_capping  :  1; // Enable bandwidth capping
        uint64_t      bandwidth_limit  : 16; // Bandwidth limit
        uint64_t       Reserved_63_32  : 32; // Unused
    } field;
    uint64_t val;
} TXDMA_CFG_BWMETER_MC0TC2_t;

// TXDMA_CFG_BWMETER_MC0TC3 desc:
typedef union {
    struct {
        uint64_t leak_amount_fractional  :  8; // Fractional portion of leak amount
        uint64_t leak_amount_integral  :  3; // Integral portion of leak amount
        uint64_t       Reserved_14_11  :  4; // Unused
        uint64_t       enable_capping  :  1; // Enable bandwidth capping
        uint64_t      bandwidth_limit  : 16; // Bandwidth limit
        uint64_t       Reserved_63_32  : 32; // Unused
    } field;
    uint64_t val;
} TXDMA_CFG_BWMETER_MC0TC3_t;

// TXDMA_CFG_BWMETER_MC1TC0 desc:
typedef union {
    struct {
        uint64_t leak_amount_fractional  :  8; // Fractional portion of leak amount
        uint64_t leak_amount_integral  :  3; // Integral portion of leak amount
        uint64_t       Reserved_14_11  :  4; // Unused
        uint64_t       enable_capping  :  1; // Enable bandwidth capping
        uint64_t      bandwidth_limit  : 16; // Bandwidth limit
        uint64_t       Reserved_63_32  : 32; // Unused
    } field;
    uint64_t val;
} TXDMA_CFG_BWMETER_MC1TC0_t;

// TXDMA_CFG_BWMETER_MC1TC1 desc:
typedef union {
    struct {
        uint64_t leak_amount_fractional  :  8; // Fractional portion of leak amount
        uint64_t leak_amount_integral  :  3; // Integral portion of leak amount
        uint64_t       Reserved_14_11  :  4; // Unused
        uint64_t       enable_capping  :  1; // Enable bandwidth capping
        uint64_t      bandwidth_limit  : 16; // Bandwidth limit
        uint64_t       Reserved_63_32  : 32; // Unused
    } field;
    uint64_t val;
} TXDMA_CFG_BWMETER_MC1TC1_t;

// TXDMA_CFG_BWMETER_MC1TC2 desc:
typedef union {
    struct {
        uint64_t leak_amount_fractional  :  8; // Fractional portion of leak amount
        uint64_t leak_amount_integral  :  3; // Integral portion of leak amount
        uint64_t       Reserved_14_11  :  4; // Unused
        uint64_t       enable_capping  :  1; // Enable bandwidth capping
        uint64_t      bandwidth_limit  : 16; // Bandwidth limit
        uint64_t       Reserved_63_32  : 32; // Unused
    } field;
    uint64_t val;
} TXDMA_CFG_BWMETER_MC1TC2_t;

// TXDMA_CFG_BWMETER_MC1TC3 desc:
typedef union {
    struct {
        uint64_t leak_amount_fractional  :  8; // Fractional portion of leak amount
        uint64_t leak_amount_integral  :  3; // Integral portion of leak amount
        uint64_t       Reserved_14_11  :  4; // Unused
        uint64_t       enable_capping  :  1; // Enable bandwidth capping
        uint64_t      bandwidth_limit  : 16; // Bandwidth limit
        uint64_t       Reserved_63_32  : 32; // Unused
    } field;
    uint64_t val;
} TXDMA_CFG_BWMETER_MC1TC3_t;

// TXDMA_CFG_PKT_DESC_CREDITS desc:
typedef union {
    struct {
        uint64_t      mc0_tc0_credits  :  6; // Maximum number of credits available for MC0, TC0
        uint64_t      mc0_tc1_credits  :  6; // Maximum number of credits available for MC0, TC1
        uint64_t      mc0_tc2_credits  :  6; // Maximum number of credits available for MC0, TC2
        uint64_t      mc0_tc3_credits  :  6; // Maximum number of credits available for MC0, TC3
        uint64_t      mc1_tc0_credits  :  6; // Maximum number of credits available for MC1, TC0
        uint64_t      mc1_tc1_credits  :  6; // Maximum number of credits available for MC1, TC1
        uint64_t      mc1_tc2_credits  :  6; // Maximum number of credits available for MC1, TC2
        uint64_t      mc1_tc3_credits  :  6; // Maximum number of credits available for MC1, TC3
        uint64_t       Reserved_63_48  : 16; // Unused
    } field;
    uint64_t val;
} TXDMA_CFG_PKT_DESC_CREDITS_t;

// TXDMA_CFG_MEM_RSP_CREDITS desc:
typedef union {
    struct {
        uint64_t              credits  :  4; // Maximum number of credits available.
        uint64_t        Reserved_63_4  : 60; // Unused
    } field;
    uint64_t val;
} TXDMA_CFG_MEM_RSP_CREDITS_t;

// TXDMA_CFG_PKT_ACK_CREDITS desc:
typedef union {
    struct {
        uint64_t          tc0_credits  :  5; // Maximum number of credits available for TC0
        uint64_t          tc1_credits  :  5; // Maximum number of credits available for TC1
        uint64_t          tc2_credits  :  5; // Maximum number of credits available for TC2
        uint64_t          tc3_credits  :  5; // Maximum number of credits available for TC3
        uint64_t       Reserved_63_20  : 44; // Unused
    } field;
    uint64_t val;
} TXDMA_CFG_PKT_ACK_CREDITS_t;

// TXDMA_CFG_TIMEOUT desc:
typedef union {
    struct {
        uint64_t          timeout_cnt  : 32; // Default value is set to 100ms (100 ms / .833 ns)
        uint64_t       Reserved_63_32  : 32; // Unused
    } field;
    uint64_t val;
} TXDMA_CFG_TIMEOUT_t;

// TXDMA_CFG_CLEANUP desc:
typedef union {
    struct {
        uint64_t              cleanup  :  1; // Writing a 1 to this field initiates a TXDMA cleanup
        uint64_t        Reserved_63_1  : 63; // Unused
    } field;
    uint64_t val;
} TXDMA_CFG_CLEANUP_t;

// TXDMA_ERR_STS desc:
typedef union {
    struct {
        uint64_t  pkt_desc_cor_sb_err  :  1; // Packet descriptor from OTR contained a correctable sb error
        uint64_t  pkt_desc_unc_sb_err  :  1; // Packet descriptor from OTR contained a uncorrectable sb error
        uint64_t pkt_desc_cor_dat_err  :  1; // Packet descriptor from OTR contained a correctable data error
        uint64_t pkt_desc_unc_dat_err  :  1; // Packet descriptor from OTR contained a uncorrectable data error
        uint64_t     pkt_desc_gen_err  :  1; // Packet descriptor generation error received from OTR
        uint64_t         inpq_cor_err  :  1; // Input queue FIFO encountered a correctable error
        uint64_t         inpq_unc_err  :  1; // Input queue FIFO encountered a uncorrectable error
        uint64_t       xlateq_cor_err  :  1; // Translation queue FIFO encountered a correctable error
        uint64_t       xlateq_unc_err  :  1; // Translation queue FIFO encountered a uncorrectable error
        uint64_t        inpq_overflow  :  1; // Input queue FIFO overflowed
        uint64_t        at_status_err  :  1; // Translation status error
        uint64_t          timeout_err  :  1; // Timeout error
        uint64_t  mem_rsp_hdr_cor_err  :  1; // Memory response header contained a correctable error
        uint64_t  mem_rsp_hdr_unc_err  :  1; // Memory response header contained a uncorrectable error
        uint64_t  mem_rsp_dat_cor_err  :  1; // Memory response data contained a correctable error
        uint64_t  mem_rsp_dat_unc_err  :  1; // Memory response data contained a uncorrectable error
        uint64_t          orb_cor_err  :  1; // ORB memory encountered a correctable error
        uint64_t          orb_unc_err  :  1; // ORB memory encountered a uncorrectable error
        uint64_t          wce_cor_err  :  1; // Write combining memory encountered a correctable error
        uint64_t          wce_unc_err  :  1; // Write combining memory encountered a uncorrectable error
        uint64_t      pkt_buf_cor_err  :  1; // Packet buffer memory encountered a correctable error
        uint64_t      pkt_buf_unc_err  :  1; // Packet buffer memory encountered a uncorrectable error
        uint64_t         rspq_cor_err  :  1; // Response queue FIFO encountered a correctable error
        uint64_t         rspq_unc_err  :  1; // Response queue FIFO encountered a uncorrectable error
        uint64_t        rspq_overflow  :  1; // Response queue FIFO overflowed
        uint64_t       lm0_credit_err  :  1; // The credit acknowledge from LM0 contained a parity error
        uint64_t       lm1_credit_err  :  1; // The credit acknowledge from LM1 contained a parity error
        uint64_t      buf_trk_cor_err  :  1; // Buffer tracking memory encountered a correctable error
        uint64_t      buf_trk_unc_err  :  1; // Buffer tracking memory encountered a uncorrectable error
        uint64_t      pkt_trk_cor_err  :  1; // Packet tracking memory encountered a correctable error
        uint64_t      pkt_trk_unc_err  :  1; // Packet tracking memory encountered a uncorrectable error
        uint64_t       Reserved_63_31  : 33; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_STS_t;

// TXDMA_ERR_CLR desc:
typedef union {
    struct {
        uint64_t            error_clr  : 31; // Clear the error
        uint64_t       Reserved_63_31  : 33; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_CLR_t;

// TXDMA_ERR_FRC desc:
typedef union {
    struct {
        uint64_t            force_err  : 31; // Force an error
        uint64_t       Reserved_63_31  : 33; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_FRC_t;

// TXDMA_ERR_EN_HOST desc:
typedef union {
    struct {
        uint64_t              host_en  : 31; // Enable host interrupt
        uint64_t       Reserved_63_31  : 33; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_EN_HOST_t;

// TXDMA_ERR_FIRST_HOST desc:
typedef union {
    struct {
        uint64_t           first_host  : 31; // First host error
        uint64_t       Reserved_63_31  : 33; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_FIRST_HOST_t;

// TXDMA_ERR_EN_BMC desc:
typedef union {
    struct {
        uint64_t               bmc_en  : 31; // BMC interrupt enable
        uint64_t       Reserved_63_31  : 33; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_EN_BMC_t;

// TXDMA_ERR_FIRST_BMC desc:
typedef union {
    struct {
        uint64_t            first_bmc  : 31; // First BMC error
        uint64_t       Reserved_63_31  : 33; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_FIRST_BMC_t;

// TXDMA_ERR_EN_QUAR desc:
typedef union {
    struct {
        uint64_t              quar_en  : 31; // Quarantine interrupt enable
        uint64_t       Reserved_63_31  : 33; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_EN_QUAR_t;

// TXDMA_ERR_FIRST_QUAR desc:
typedef union {
    struct {
        uint64_t           first_quar  : 31; // First quarantine error
        uint64_t       Reserved_63_31  : 33; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_FIRST_QUAR_t;

// TXDMA_ERR_INFO_PKT_DESC_SB_COR desc:
typedef union {
    struct {
        uint64_t             syndrome  :  7; // Err syndrome
        uint64_t        Reserved_63_7  : 57; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_INFO_PKT_DESC_SB_COR_t;

// TXDMA_ERR_INFO_PKT_DESC_SB_UNC desc:
typedef union {
    struct {
        uint64_t             syndrome  :  7; // Error syndrome
        uint64_t        Reserved_63_7  : 57; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_INFO_PKT_DESC_SB_UNC_t;

// TXDMA_ERR_INFO_PKT_DESC_DAT_COR desc:
typedef union {
    struct {
        uint64_t               domain  :  4; // Mask of the domains which contained an error
        uint64_t             syndrome  :  8; // Syndrome corresponding to the lowest bit set in domain
        uint64_t       Reserved_63_12  : 52; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_INFO_PKT_DESC_DAT_COR_t;

// TXDMA_ERR_INFO_PKT_DESC_DAT_UNC desc:
typedef union {
    struct {
        uint64_t               domain  :  4; // Mask of the domains which contained an error
        uint64_t             syndrome  :  8; // Syndrome corresponding to the lowest bit set in domain
        uint64_t       Reserved_63_12  : 52; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_INFO_PKT_DESC_DAT_UNC_t;

// TXDMA_ERR_INFO_INPQ_COR desc:
typedef union {
    struct {
        uint64_t              address  :  8; // Address corresponding to the lowest bit set in domain
        uint64_t               domain  :  5; // Mask of the domains which contained an error
        uint64_t             syndrome  :  8; // Syndrome corresponding to the lowest bit set in domain
        uint64_t       Reserved_63_21  : 43; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_INFO_INPQ_COR_t;

// TXDMA_ERR_INFO_INPQ_UNC desc:
typedef union {
    struct {
        uint64_t              address  :  8; // Address corresponding to the lowest bit set in domain
        uint64_t               domain  :  5; // Mask of the domains which contained an error
        uint64_t             syndrome  :  8; // Syndrome corresponding to the lowest bit set in domain
        uint64_t       Reserved_63_21  : 43; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_INFO_INPQ_UNC_t;

// TXDMA_ERR_INFO_XLATEQ_COR desc:
typedef union {
    struct {
        uint64_t              address  :  5; // Address corresponding to the lowest bit set in domain
        uint64_t               domain  :  2; // Mask of the domains which contained an error
        uint64_t             syndrome  :  8; // Syndrome corresponding to the lowest bit set in domain
        uint64_t       Reserved_63_15  : 49; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_INFO_XLATEQ_COR_t;

// TXDMA_ERR_INFO_XLATEQ_UNC desc:
typedef union {
    struct {
        uint64_t              address  :  5; // Address corresponding to the lowest bit set in domain
        uint64_t               domain  :  2; // Mask of the domains which contained an error
        uint64_t             syndrome  :  8; // Syndrome corresponding to the lowest bit set in domain
        uint64_t       Reserved_63_15  : 49; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_INFO_XLATEQ_UNC_t;

// TXDMA_ERR_INFO_INPQ_OVF desc:
typedef union {
    struct {
        uint64_t                 FIFO  :  8; // The FIFO which overflowed, one FIFO per context
        uint64_t        Reserved_63_8  : 56; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_INFO_INPQ_OVF_t;

// TXDMA_ERR_INFO_AT_STATUS desc:
typedef union {
    struct {
        uint64_t               status  :  4; // The status which was returned from the AT
        uint64_t        Reserved_63_4  : 60; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_INFO_AT_STATUS_t;

// TXDMA_ERR_INFO_TIMEOUT desc:
typedef union {
    struct {
        uint64_t                  TID  :  8; // The TID which timed out
        uint64_t        Reserved_63_8  : 56; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_INFO_TIMEOUT_t;

// TXDMA_ERR_INFO_MEM_RSP_HDR_COR desc:
typedef union {
    struct {
        uint64_t             syndrome  :  8; // Syndrome of the error
        uint64_t        Reserved_63_8  : 56; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_INFO_MEM_RSP_HDR_COR_t;

// TXDMA_ERR_INFO_MEM_RSP_HDR_UNC desc:
typedef union {
    struct {
        uint64_t             syndrome  :  8; // Syndrome of the error
        uint64_t        Reserved_63_8  : 56; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_INFO_MEM_RSP_HDR_UNC_t;

// TXDMA_ERR_INFO_MEM_RSP_DAT_COR desc:
typedef union {
    struct {
        uint64_t               domain  :  4; // Mask of the domains which contained an error
        uint64_t             syndrome  :  8; // Syndrome corresponding to the lowest bit set in domain
        uint64_t       Reserved_63_12  : 52; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_INFO_MEM_RSP_DAT_COR_t;

// TXDMA_ERR_INFO_MEM_RSP_DAT_UNC desc:
typedef union {
    struct {
        uint64_t               domain  :  4; // Mask of the domains which contained an error
        uint64_t             syndrome  :  8; // Syndrome corresponding to the lowest bit set in domain
        uint64_t       Reserved_63_12  : 52; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_INFO_MEM_RSP_DAT_UNC_t;

// TXDMA_ERR_INFO_ORB_COR desc:
typedef union {
    struct {
        uint64_t              address  :  8; // Address of the error
        uint64_t             syndrome  :  8; // Syndrome of the error
        uint64_t       Reserved_63_16  : 48; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_INFO_ORB_COR_t;

// TXDMA_ERR_INFO_ORB_UNC desc:
typedef union {
    struct {
        uint64_t              address  :  8; // Address of the error
        uint64_t             syndrome  :  8; // Syndrome of the error
        uint64_t       Reserved_63_16  : 48; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_INFO_ORB_UNC_t;

// TXDMA_ERR_INFO_WCE_COR desc:
typedef union {
    struct {
        uint64_t              address  :  8; // Address corresponding to the lowest bit set in domain
        uint64_t               domain  :  5; // Mask of the domains which encountered an error
        uint64_t             syndrome  :  8; // Syndrome corresponding to the lowest bits set in domain
        uint64_t       Reserved_63_21  : 43; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_INFO_WCE_COR_t;

// TXDMA_ERR_INFO_WCE_UNC desc:
typedef union {
    struct {
        uint64_t              address  :  8; // Address corresponding to the lowest bit set in domain
        uint64_t               domain  :  5; // Mask of error domains which encountered an error
        uint64_t             syndrome  :  8; // Syndrome corresponding to the lowest bit set in domain
        uint64_t       Reserved_63_21  : 43; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_INFO_WCE_UNC_t;

// TXDMA_ERR_INFO_PKT_BUF_COR desc:
typedef union {
    struct {
        uint64_t              address  : 12; // Address corresponding to the lowest bit set in domain
        uint64_t               domain  :  4; // Mask of the domains which contained an error
        uint64_t             syndrome  :  8; // Syndrome corresponding to the lowest bit set in domain
        uint64_t       Reserved_63_24  : 40; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_INFO_PKT_BUF_COR_t;

// TXDMA_ERR_INFO_PKT_BUF_UNC desc:
typedef union {
    struct {
        uint64_t              address  : 12; // Address corresponding to the lowest bit set in domain
        uint64_t               domain  :  4; // Mask of the domains which contained an error
        uint64_t             syndrome  :  8; // Syndrome corresponding to the lowest bit set in domain
        uint64_t       Reserved_63_24  : 40; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_INFO_PKT_BUF_UNC_t;

// TXDMA_ERR_INFO_RSPQ_COR desc:
typedef union {
    struct {
        uint64_t               domain  :  4; // Mask of the domains which contained an error
        uint64_t             syndrome  :  8; // Syndrome corresponding to the lowest bit set in domain
        uint64_t       Reserved_63_12  : 52; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_INFO_RSPQ_COR_t;

// TXDMA_ERR_INFO_RSPQ_UNC desc:
typedef union {
    struct {
        uint64_t               domain  :  4; // Mask of the domains which contained an error
        uint64_t             syndrome  :  8; // Syndrome corresponding to the lowest bit set in domain
        uint64_t       Reserved_63_12  : 52; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_INFO_RSPQ_UNC_t;

// TXDMA_ERR_INFO_RSPQ_OVF desc:
typedef union {
    struct {
        uint64_t                 FIFO  :  4; // The FIFO which overflowed, one FIFO per context
        uint64_t        Reserved_63_4  : 60; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_INFO_RSPQ_OVF_t;

// TXDMA_ERR_INFO_LM0_PAR desc:
typedef union {
    struct {
        uint64_t           credit_ack  :  4; // The credit_ack received from LM0
        uint64_t    credit_ack_parity  :  1; // The credit_ack parity received from LM0
        uint64_t        Reserved_63_5  : 59; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_INFO_LM0_PAR_t;

// TXDMA_ERR_INFO_LM1_PAR desc:
typedef union {
    struct {
        uint64_t           credit_ack  :  4; // The credit_ack received from LM1
        uint64_t    credit_ack_parity  :  1; // The credit_ack parity received from LM1
        uint64_t        Reserved_63_5  : 59; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_INFO_LM1_PAR_t;

// TXDMA_ERR_INFO_BUF_TRK_COR desc:
typedef union {
    struct {
        uint64_t              address  : 12; // Address of the error
        uint64_t             syndrome  :  5; // Syndrome of the error
        uint64_t       Reserved_63_17  : 47; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_INFO_BUF_TRK_COR_t;

// TXDMA_ERR_INFO_BUF_TRK_UNC desc:
typedef union {
    struct {
        uint64_t              address  : 12; // Address of the error
        uint64_t             syndrome  :  5; // Syndrome of the error
        uint64_t       Reserved_63_17  : 47; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_INFO_BUF_TRK_UNC_t;

// TXDMA_ERR_INFO_PKT_TRK_COR desc:
typedef union {
    struct {
        uint64_t              address  :  7; // Address of the error
        uint64_t             syndrome  :  6; // Syndrome of the error
        uint64_t       Reserved_63_13  : 51; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_INFO_PKT_TRK_COR_t;

// TXDMA_ERR_INFO_PKT_TRK_UNC desc:
typedef union {
    struct {
        uint64_t              address  :  7; // Address of the error
        uint64_t             syndrome  :  6; // Syndrome of the error
        uint64_t       Reserved_63_13  : 51; // Unused
    } field;
    uint64_t val;
} TXDMA_ERR_INFO_PKT_TRK_UNC_t;

// TXDMA_STS_XLATE_CREDITS desc:
typedef union {
    struct {
        uint64_t              credits  :  4; // Credits available
        uint64_t        Reserved_63_4  : 60; // Unused
    } field;
    uint64_t val;
} TXDMA_STS_XLATE_CREDITS_t;

// TXDMA_STS_MEM_REQ_CREDITS desc:
typedef union {
    struct {
        uint64_t              credits  :  4; // Credits available
        uint64_t        Reserved_63_4  : 60; // Unused
    } field;
    uint64_t val;
} TXDMA_STS_MEM_REQ_CREDITS_t;

// TXDMA_STS_MC0_PKT_CREDITS desc:
typedef union {
    struct {
        uint64_t          tc0_credits  :  5; // TC0 credits available
        uint64_t          tc1_credits  :  5; // TC1 credits available
        uint64_t          tc2_credits  :  5; // TC2 credits available
        uint64_t          tc3_credits  :  5; // TC3 credits available
        uint64_t       Reserved_63_20  : 44; // Unused
    } field;
    uint64_t val;
} TXDMA_STS_MC0_PKT_CREDITS_t;

// TXDMA_STS_MC1_PKT_CREDITS desc:
typedef union {
    struct {
        uint64_t          tc0_credits  :  5; // TC0 credits available
        uint64_t          tc1_credits  :  5; // TC1 credits available
        uint64_t          tc2_credits  :  5; // TC2 credits available
        uint64_t          tc3_credits  :  5; // TC3 credits available
        uint64_t       Reserved_63_20  : 44; // Unused
    } field;
    uint64_t val;
} TXDMA_STS_MC1_PKT_CREDITS_t;

// TXDMA_STS_OTR_ACK_CREDITS desc:
typedef union {
    struct {
        uint64_t      mc0_tc0_credits  :  3; // Credits available for MC0 TC0
        uint64_t      mc0_tc1_credits  :  3; // Credits available for MC0 TC1
        uint64_t      mc0_tc2_credits  :  3; // Credits available for MC0 TC2
        uint64_t      mc0_tc3_credits  :  3; // Credits available for MC0 TC3
        uint64_t      mc1_tc0_credits  :  3; // Credits available for MC1 TC0
        uint64_t      mc1_tc1_credits  :  3; // Credits available for MC1 TC1
        uint64_t      mc1_tc2_credits  :  3; // Credits available for MC1 TC2
        uint64_t      mc1_tc3_credits  :  3; // Credits available for MC1 TC3
        uint64_t       Reserved_63_24  : 40; // Unused
    } field;
    uint64_t val;
} TXDMA_STS_OTR_ACK_CREDITS_t;

// TXDMA_STS_MC0_TC0 desc:
typedef union {
    struct {
        uint64_t      inpq_fill_count  :  6; // Number of descriptor flits being held in input queue FIFO
        uint64_t        inpq_va_count  :  3; // Input queues count of virtual addresses in xlate FIFO
        uint64_t     xlate_fill_count  :  3; // Xlate blocks count of virtual addresses in xlate FIFO
        uint64_t         slots_in_use  : 10; // Count of slots allocated
        uint64_t       packet_fill_ip  :  1; // A packet is being assembled from memory
        uint64_t     packet_egress_ip  :  1; // A packet is being sent from the packet buffer to the LM
        uint64_t        packet_enable  :  1; // The packet at the head of the packet buffer is enabled to send to the LM
        uint64_t            slot_full  :  1; // The slot at the head of the packet buffer is full
        uint64_t       Reserved_63_26  : 38; // Unused
    } field;
    uint64_t val;
} TXDMA_STS_MC0_TC0_t;

// TXDMA_STS_MC0_TC1 desc:
typedef union {
    struct {
        uint64_t      inpq_fill_count  :  6; // Number of descriptor flits being held in input queue FIFO
        uint64_t        inpq_va_count  :  3; // Input queues count of virtual addresses in xlate FIFO
        uint64_t     xlate_fill_count  :  3; // Xlate blocks count of virtual addresses in xlate FIFO
        uint64_t         slots_in_use  : 10; // Count of slots allocated
        uint64_t       packet_fill_ip  :  1; // A packet is being assembled from memory
        uint64_t     packet_egress_ip  :  1; // A packet is being sent from the packet buffer to the LM
        uint64_t        packet_enable  :  1; // The packet at the head of the packet buffer is enabled to send to the LM
        uint64_t            slot_full  :  1; // The slot at the head of the packet buffer is full
        uint64_t       Reserved_63_26  : 38; // Unused
    } field;
    uint64_t val;
} TXDMA_STS_MC0_TC1_t;

// TXDMA_STS_MC0_TC2 desc:
typedef union {
    struct {
        uint64_t      inpq_fill_count  :  6; // Number of descriptor flits being held in input queue FIFO
        uint64_t        inpq_va_count  :  3; // Input queues count of virtual addresses in xlate FIFO
        uint64_t     xlate_fill_count  :  3; // Xlate blocks count of virtual addresses in xlate FIFO
        uint64_t         slots_in_use  : 10; // Count of slots allocated
        uint64_t       packet_fill_ip  :  1; // A packet is being assembled from memory
        uint64_t     packet_egress_ip  :  1; // A packet is being sent from the packet buffer to the LM
        uint64_t        packet_enable  :  1; // The packet at the head of the packet buffer is enabled to send to the LM
        uint64_t            slot_full  :  1; // The slot at the head of the packet buffer is full
        uint64_t       Reserved_63_26  : 38; // Unused
    } field;
    uint64_t val;
} TXDMA_STS_MC0_TC2_t;

// TXDMA_STS_MC0_TC3 desc:
typedef union {
    struct {
        uint64_t      inpq_fill_count  :  6; // Number of descriptor flits being held in input queue FIFO
        uint64_t        inpq_va_count  :  3; // Input queues count of virtual addresses in xlate FIFO
        uint64_t     xlate_fill_count  :  3; // Xlate blocks count of virtual addresses in xlate FIFO
        uint64_t         slots_in_use  : 10; // Count of slots allocated
        uint64_t       packet_fill_ip  :  1; // A packet is being assembled from memory
        uint64_t     packet_egress_ip  :  1; // A packet is being sent from the packet buffer to the LM
        uint64_t        packet_enable  :  1; // The packet at the head of the packet buffer is enabled to send to the LM
        uint64_t            slot_full  :  1; // The slot at the head of the packet buffer is full
        uint64_t       Reserved_63_26  : 38; // Unused
    } field;
    uint64_t val;
} TXDMA_STS_MC0_TC3_t;

// TXDMA_STS_MC1_TC0 desc:
typedef union {
    struct {
        uint64_t      inpq_fill_count  :  6; // Number of descriptor flits being held in input queue FIFO
        uint64_t        inpq_va_count  :  3; // Input queues count of virtual addresses in xlate FIFO
        uint64_t     xlate_fill_count  :  3; // Xlate blocks count of virtual addresses in xlate FIFO
        uint64_t         slots_in_use  : 10; // Count of slots allocated
        uint64_t       packet_fill_ip  :  1; // A packet is being assembled from memory
        uint64_t     packet_egress_ip  :  1; // A packet is being sent from the packet buffer to the LM
        uint64_t        packet_enable  :  1; // The packet at the head of the packet buffer is enabled to send to the LM
        uint64_t            slot_full  :  1; // The slot at the head of the packet buffer is full
        uint64_t         ack_fill_cnt  :  5; // Count of acknowledge packets in FIFO
        uint64_t       Reserved_63_31  : 33; // Unused
    } field;
    uint64_t val;
} TXDMA_STS_MC1_TC0_t;

// TXDMA_STS_MC1_TC1 desc:
typedef union {
    struct {
        uint64_t      inpq_fill_count  :  6; // Number of descriptor flits being held in input queue FIFO
        uint64_t        inpq_va_count  :  3; // Input queues count of virtual addresses in xlate FIFO
        uint64_t     xlate_fill_count  :  3; // Xlate blocks count of virtual addresses in xlate FIFO
        uint64_t         slots_in_use  : 10; // Count of slots allocated
        uint64_t       packet_fill_ip  :  1; // A packet is being assembled from memory
        uint64_t     packet_egress_ip  :  1; // A packet is being sent from the packet buffer to the LM
        uint64_t        packet_enable  :  1; // The packet at the head of the packet buffer is enabled to send to the LM
        uint64_t            slot_full  :  1; // The slot at the head of the packet buffer is full
        uint64_t         ack_fill_cnt  :  5; // Count of acknowledge packets in FIFO
        uint64_t       Reserved_63_31  : 33; // Unused
    } field;
    uint64_t val;
} TXDMA_STS_MC1_TC1_t;

// TXDMA_STS_MC1_TC2 desc:
typedef union {
    struct {
        uint64_t      inpq_fill_count  :  6; // Number of descriptor flits being held in input queue FIFO
        uint64_t        inpq_va_count  :  3; // Input queues count of virtual addresses in xlate FIFO
        uint64_t     xlate_fill_count  :  3; // Xlate blocks count of virtual addresses in xlate FIFO
        uint64_t         slots_in_use  : 10; // Count of slots allocated
        uint64_t       packet_fill_ip  :  1; // A packet is being assembled from memory
        uint64_t     packet_egress_ip  :  1; // A packet is being sent from the packet buffer to the LM
        uint64_t        packet_enable  :  1; // The packet at the head of the packet buffer is enabled to send to the LM
        uint64_t            slot_full  :  1; // The slot at the head of the packet buffer is full
        uint64_t         ack_fill_cnt  :  5; // Count of acknowledge packets in FIFO
        uint64_t       Reserved_63_31  : 33; // Unused
    } field;
    uint64_t val;
} TXDMA_STS_MC1_TC2_t;

// TXDMA_STS_MC1_TC3 desc:
typedef union {
    struct {
        uint64_t      inpq_fill_count  :  6; // Number of descriptor flits being held in input queue FIFO
        uint64_t        inpq_va_count  :  3; // Input queues count of virtual addresses in xlate FIFO
        uint64_t     xlate_fill_count  :  3; // Xlate blocks count of virtual addresses in xlate FIFO
        uint64_t         slots_in_use  : 10; // Count of slots allocated
        uint64_t       packet_fill_ip  :  1; // A packet is being assembled from memory
        uint64_t     packet_egress_ip  :  1; // A packet is being sent from the packet buffer to the LM
        uint64_t        packet_enable  :  1; // The packet at the head of the packet buffer is enabled to send to the LM
        uint64_t            slot_full  :  1; // The slot at the head of the packet buffer is full
        uint64_t         ack_fill_cnt  :  5; // Count of acknowledge packets in FIFO
        uint64_t       Reserved_63_31  : 33; // Unused
    } field;
    uint64_t val;
} TXDMA_STS_MC1_TC3_t;

// TXDMA_STS_PKT_IDX0 desc:
typedef union {
    struct {
        uint64_t      pkt_idx_val_msk  : 64; // Bits 63:0 of the packet index valid mask.
    } field;
    uint64_t val;
} TXDMA_STS_PKT_IDX0_t;

// TXDMA_STS_PKT_IDX1 desc:
typedef union {
    struct {
        uint64_t      pkt_idx_val_msk  : 64; // Bits 127:64 of the packet index valid mask.
    } field;
    uint64_t val;
} TXDMA_STS_PKT_IDX1_t;

// TXDMA_STS_WCE0 desc:
typedef union {
    struct {
        uint64_t          wce_val_msk  : 64; // Bits 63:0 of the write combining entry valid mask.
    } field;
    uint64_t val;
} TXDMA_STS_WCE0_t;

// TXDMA_STS_WCE1 desc:
typedef union {
    struct {
        uint64_t          wce_val_msk  : 64; // Bits 127:64 of the write combining entry valid mask.
    } field;
    uint64_t val;
} TXDMA_STS_WCE1_t;

// TXDMA_STS_WCE2 desc:
typedef union {
    struct {
        uint64_t          wce_val_msk  : 64; // Bits 191:128 of the write combining entry valid mask.
    } field;
    uint64_t val;
} TXDMA_STS_WCE2_t;

// TXDMA_STS_WCE3 desc:
typedef union {
    struct {
        uint64_t          wce_val_msk  : 64; // Bits 255:192 of the write combining entry valid mask.
    } field;
    uint64_t val;
} TXDMA_STS_WCE3_t;

// TXDMA_STS_MEM_REQ_TID0 desc:
typedef union {
    struct {
        uint64_t          tid_val_msk  : 64; // Bits 63:0 of the memory request TID valid mask.
    } field;
    uint64_t val;
} TXDMA_STS_MEM_REQ_TID0_t;

// TXDMA_STS_MEM_REQ_TID1 desc:
typedef union {
    struct {
        uint64_t          tid_val_msk  : 64; // Bits 127:64 of the memory request TID valid mask.
    } field;
    uint64_t val;
} TXDMA_STS_MEM_REQ_TID1_t;

// TXDMA_STS_MEM_REQ_TID2 desc:
typedef union {
    struct {
        uint64_t          tid_val_msk  : 64; // Bits 191:128 of the memory request TID valid mask.
    } field;
    uint64_t val;
} TXDMA_STS_MEM_REQ_TID2_t;

// TXDMA_STS_MEM_REQ_TID3 desc:
typedef union {
    struct {
        uint64_t          tid_val_msk  : 64; // Bits 255:192 of the memory request TID valid mask.
    } field;
    uint64_t val;
} TXDMA_STS_MEM_REQ_TID3_t;

// TXDMA_STS_PA_ID desc:
typedef union {
    struct {
        uint64_t           id_val_msk  : 32; // The physical address ID valid mask.
        uint64_t       Reserved_63_32  : 32; // Unused
    } field;
    uint64_t val;
} TXDMA_STS_PA_ID_t;

// TXDMA_STS_FROZE desc:
typedef union {
    struct {
        uint64_t                froze  :  1; // The TXDMA is frozen
        uint64_t            at_req_ip  :  1; // There is at least one translation request outstanding
        uint64_t           mem_req_ip  :  1; // There is at least one memory request outstanding
        uint64_t        Reserved_63_3  : 61; // Unused
    } field;
    uint64_t val;
} TXDMA_STS_FROZE_t;

// TXDMA_DBG_INPQ_CTRL desc:
typedef union {
    struct {
        uint64_t              Address  :  8; // Address for read or write, bits 7:5 are decoded as {MC,TC}
        uint64_t        Reserved_51_8  : 44; // Unused
        uint64_t         Payload_regs  :  8; // Number of payload registers which follow
        uint64_t          Reserved_60  :  1; // Unused
        uint64_t                  ECC  :  1; // 0=read/write raw data 1=generate ECC on write, correct data on read
        uint64_t                Write  :  1; // 0=read, 1=write
        uint64_t                Valid  :  1; // Set by software, cleared by hardware when complete
    } field;
    uint64_t val;
} TXDMA_DBG_INPQ_CTRL_t;

// TXDMA_DBG_INPQ_PAYLOAD0 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data
    } field;
    uint64_t val;
} TXDMA_DBG_INPQ_PAYLOAD0_t;

// TXDMA_DBG_INPQ_PAYLOAD1 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data
    } field;
    uint64_t val;
} TXDMA_DBG_INPQ_PAYLOAD1_t;

// TXDMA_DBG_INPQ_PAYLOAD2 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data
    } field;
    uint64_t val;
} TXDMA_DBG_INPQ_PAYLOAD2_t;

// TXDMA_DBG_INPQ_PAYLOAD3 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data
    } field;
    uint64_t val;
} TXDMA_DBG_INPQ_PAYLOAD3_t;

// TXDMA_DBG_INPQ_PAYLOAD4 desc:
typedef union {
    struct {
        uint64_t                 Data  : 34; // Data
        uint64_t       Reserved_63_34  : 30; // Unused
    } field;
    uint64_t val;
} TXDMA_DBG_INPQ_PAYLOAD4_t;

// TXDMA_DBG_INPQ_ECC desc:
typedef union {
    struct {
        uint64_t                 ECC0  :  8; // ECC for data[63:0]
        uint64_t                 ECC1  :  8; // ECC for data[127:64]
        uint64_t                 ECC2  :  8; // ECC for data[191:128]
        uint64_t                 ECC3  :  8; // ECC for data[255:192]
        uint64_t                 ECC4  :  7; // ECC for data[294:256]
        uint64_t       Reserved_63_39  : 25; // Unused
    } field;
    uint64_t val;
} TXDMA_DBG_INPQ_ECC_t;

// TXDMA_DBG_XLATE_CTRL desc:
typedef union {
    struct {
        uint64_t              Address  :  5; // Address for read or write, bits 5:3 are decoded as {MC,TC}
        uint64_t        Reserved_51_5  : 47; // Unused
        uint64_t         Payload_regs  :  8; // Number of payload registers which follow
        uint64_t          Reserved_60  :  1; // Unused
        uint64_t                  ECC  :  1; // 0=read/write raw data 1=generate ECC on write, correct data on read
        uint64_t                Write  :  1; // 0=read, 1=write
        uint64_t                Valid  :  1; // Set by software, cleared by hardware when complete
    } field;
    uint64_t val;
} TXDMA_DBG_XLATE_CTRL_t;

// TXDMA_DBG_XLATE_PAYLOAD0 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data
    } field;
    uint64_t val;
} TXDMA_DBG_XLATE_PAYLOAD0_t;

// TXDMA_DBG_XLATE_PAYLOAD1 desc:
typedef union {
    struct {
        uint64_t                 Data  : 63; // Data
        uint64_t          Reserved_63  :  1; // Unused
    } field;
    uint64_t val;
} TXDMA_DBG_XLATE_PAYLOAD1_t;

// TXDMA_DBG_XLATE_ECC desc:
typedef union {
    struct {
        uint64_t                 ECC0  :  8; // ECC for data[63:0]
        uint64_t                 ECC1  :  8; // ECC for data[126:64]
        uint64_t       Reserved_63_16  : 48; // Unused
    } field;
    uint64_t val;
} TXDMA_DBG_XLATE_ECC_t;

// TXDMA_DBG_ORB_CTRL desc:
typedef union {
    struct {
        uint64_t              Address  :  8; // Address for read or write
        uint64_t        Reserved_51_8  : 44; // Unused
        uint64_t         Payload_regs  :  8; // Number of payload registers which follow
        uint64_t          Reserved_60  :  1; // Unused
        uint64_t                  ECC  :  1; // 0=read/write raw data 1=generate ECC on write, correct data on read
        uint64_t                Write  :  1; // 0=read, 1=write
        uint64_t                Valid  :  1; // Set by software, cleared by hardware when complete
    } field;
    uint64_t val;
} TXDMA_DBG_ORB_CTRL_t;

// TXDMA_DBG_ORB_PAYLOAD0 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data
    } field;
    uint64_t val;
} TXDMA_DBG_ORB_PAYLOAD0_t;

// TXDMA_DBG_ORB_PAYLOAD1 desc:
typedef union {
    struct {
        uint64_t                 Data  :  3; // Data
        uint64_t        Reserved_63_3  : 61; // Unused
    } field;
    uint64_t val;
} TXDMA_DBG_ORB_PAYLOAD1_t;

// TXDMA_DBG_ORB_ECC desc:
typedef union {
    struct {
        uint64_t                  ECC  :  8; // ECC for data[65:0]
        uint64_t        Reserved_63_8  : 56; // Unused
    } field;
    uint64_t val;
} TXDMA_DBG_ORB_ECC_t;

// TXDMA_DBG_WCE_CTRL desc:
typedef union {
    struct {
        uint64_t              Address  :  8; // Address for read or write
        uint64_t        Reserved_51_8  : 44; // Unused
        uint64_t         Payload_regs  :  8; // Number of payload registers which follow
        uint64_t          Reserved_60  :  1; // Unused
        uint64_t                  ECC  :  1; // 0=read/write raw data 1=generate ECC on write, correct data on read
        uint64_t                Write  :  1; // 0=read, 1=write
        uint64_t                Valid  :  1; // Set by software, cleared by hardware when complete
    } field;
    uint64_t val;
} TXDMA_DBG_WCE_CTRL_t;

// TXDMA_DBG_WCE_PAYLOAD0 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data
    } field;
    uint64_t val;
} TXDMA_DBG_WCE_PAYLOAD0_t;

// TXDMA_DBG_WCE_PAYLOAD1 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data
    } field;
    uint64_t val;
} TXDMA_DBG_WCE_PAYLOAD1_t;

// TXDMA_DBG_WCE_PAYLOAD2 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data
    } field;
    uint64_t val;
} TXDMA_DBG_WCE_PAYLOAD2_t;

// TXDMA_DBG_WCE_PAYLOAD3 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data
    } field;
    uint64_t val;
} TXDMA_DBG_WCE_PAYLOAD3_t;

// TXDMA_DBG_WCE_PAYLOAD4 desc:
typedef union {
    struct {
        uint64_t                 Data  : 18; // Data
        uint64_t       Reserved_63_18  : 46; // Unused
    } field;
    uint64_t val;
} TXDMA_DBG_WCE_PAYLOAD4_t;

// TXDMA_DBG_WCE_ECC desc:
typedef union {
    struct {
        uint64_t                 ECC0  :  8; // ECC for data[63:0]
        uint64_t                 ECC1  :  8; // ECC for data[127:64]
        uint64_t                 ECC2  :  8; // ECC for data[191:128]
        uint64_t                 ECC3  :  8; // ECC for data[255:192]
        uint64_t                 ECC4  :  6; // ECC for data[270:256]
        uint64_t       Reserved_63_38  : 26; // Unused
    } field;
    uint64_t val;
} TXDMA_DBG_WCE_ECC_t;

// TXDMA_DBG_PKT_CTRL desc:
typedef union {
    struct {
        uint64_t              Address  : 12; // Address for read or write, bits 11:9 decode as {MC,TC}
        uint64_t       Reserved_51_12  : 40; // Unused
        uint64_t         Payload_regs  :  8; // Number of payload registers which follow
        uint64_t          Reserved_60  :  1; // Unused
        uint64_t                  ECC  :  1; // 0=read/write raw data 1=generate ECC on write, correct data on read
        uint64_t                Write  :  1; // 0=read, 1=write
        uint64_t                Valid  :  1; // Set by software, cleared by hardware when complete
    } field;
    uint64_t val;
} TXDMA_DBG_PKT_CTRL_t;

// TXDMA_DBG_PKT_PAYLOAD0 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data
    } field;
    uint64_t val;
} TXDMA_DBG_PKT_PAYLOAD0_t;

// TXDMA_DBG_PKT_PAYLOAD1 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data
    } field;
    uint64_t val;
} TXDMA_DBG_PKT_PAYLOAD1_t;

// TXDMA_DBG_PKT_PAYLOAD2 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data
    } field;
    uint64_t val;
} TXDMA_DBG_PKT_PAYLOAD2_t;

// TXDMA_DBG_PKT_PAYLOAD3 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data
    } field;
    uint64_t val;
} TXDMA_DBG_PKT_PAYLOAD3_t;

// TXDMA_DBG_PKT_ECC desc:
typedef union {
    struct {
        uint64_t                 ECC0  :  8; // ECC for data[63:0]
        uint64_t                 ECC1  :  8; // ECC for data[127:64]
        uint64_t                 ECC2  :  8; // ECC for data[191:128]
        uint64_t                 ECC3  :  8; // ECC for data[255:192]
        uint64_t       Reserved_63_32  : 32; // Unused
    } field;
    uint64_t val;
} TXDMA_DBG_PKT_ECC_t;

// TXDMA_DBG_RSP_CTRL desc:
typedef union {
    struct {
        uint64_t              Address  :  6; // Address for read or write, bits 5:4 decode as TC
        uint64_t        Reserved_51_6  : 46; // Unused
        uint64_t         Payload_regs  :  8; // Number of payload registers which follow
        uint64_t          Reserved_60  :  1; // Unused
        uint64_t                  ECC  :  1; // 0=read/write raw data 1=generate ECC on write, correct data on read
        uint64_t                Write  :  1; // 0=read, 1=write
        uint64_t                Valid  :  1; // Set by software, cleared by hardware when complete
    } field;
    uint64_t val;
} TXDMA_DBG_RSP_CTRL_t;

// TXDMA_DBG_RSP_PAYLOAD0 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data
    } field;
    uint64_t val;
} TXDMA_DBG_RSP_PAYLOAD0_t;

// TXDMA_DBG_RSP_PAYLOAD1 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data
    } field;
    uint64_t val;
} TXDMA_DBG_RSP_PAYLOAD1_t;

// TXDMA_DBG_RSP_PAYLOAD2 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data
    } field;
    uint64_t val;
} TXDMA_DBG_RSP_PAYLOAD2_t;

// TXDMA_DBG_RSP_PAYLOAD3 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data
    } field;
    uint64_t val;
} TXDMA_DBG_RSP_PAYLOAD3_t;

// TXDMA_DBG_RSP_ECC desc:
typedef union {
    struct {
        uint64_t                 ECC0  :  8; // ECC for data[63:0]
        uint64_t                 ECC1  :  8; // ECC for data[127:64]
        uint64_t                 ECC2  :  8; // ECC for data[191:128]
        uint64_t                 ECC3  :  8; // ECC for data[255:192]
        uint64_t       Reserved_63_32  : 32; // Unused
    } field;
    uint64_t val;
} TXDMA_DBG_RSP_ECC_t;

// TXDMA_DBG_BTRK_CTRL desc:
typedef union {
    struct {
        uint64_t              Address  : 12; // Address for read or write
        uint64_t       Reserved_51_12  : 40; // Unused
        uint64_t         Payload_regs  :  8; // Number of payload registers which follow
        uint64_t          Reserved_60  :  1; // Unused
        uint64_t                  ECC  :  1; // 0=read/write raw data 1=generate ECC on write, correct data on read
        uint64_t                Write  :  1; // 0=read, 1=write
        uint64_t                Valid  :  1; // Set by software, cleared by hardware when complete
    } field;
    uint64_t val;
} TXDMA_DBG_BTRK_CTRL_t;

// TXDMA_DBG_BTRK_PAYLOAD0 desc:
typedef union {
    struct {
        uint64_t                 Data  : 11; // Data
        uint64_t       Reserved_63_11  : 53; // Unused
    } field;
    uint64_t val;
} TXDMA_DBG_BTRK_PAYLOAD0_t;

// TXDMA_DBG_BTRK_ECC desc:
typedef union {
    struct {
        uint64_t                  ECC  :  5; // ECC
        uint64_t        Reserved_63_5  : 59; // Unused
    } field;
    uint64_t val;
} TXDMA_DBG_BTRK_ECC_t;

// TXDMA_DBG_PTRK_CTRL desc:
typedef union {
    struct {
        uint64_t              Address  :  7; // Address for read or write
        uint64_t        Reserved_51_7  : 45; // Unused
        uint64_t         Payload_regs  :  8; // Number of payload registers which follow
        uint64_t          Reserved_60  :  1; // Unused
        uint64_t                  ECC  :  1; // 0=read/write raw data 1=generate ECC on write, correct data on read
        uint64_t                Write  :  1; // 0=read, 1=write
        uint64_t                Valid  :  1; // Set by software, cleared by hardware when complete
    } field;
    uint64_t val;
} TXDMA_DBG_PTRK_CTRL_t;

// TXDMA_DBG_PTRK_PAYLOAD0 desc:
typedef union {
    struct {
        uint64_t                 Data  : 21; // Data
        uint64_t       Reserved_63_21  : 43; // Unused
    } field;
    uint64_t val;
} TXDMA_DBG_PTRK_PAYLOAD0_t;

// TXDMA_DBG_PTRK_ECC desc:
typedef union {
    struct {
        uint64_t                  ECC  :  6; // ECC
        uint64_t        Reserved_63_6  : 58; // Unused
    } field;
    uint64_t val;
} TXDMA_DBG_PTRK_ECC_t;

#endif /* ___FXR_tx_dma_CSRS_H__ */