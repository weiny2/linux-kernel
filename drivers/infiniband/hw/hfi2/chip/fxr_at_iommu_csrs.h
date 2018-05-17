// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Wed May 16 16:02:36 2018
//

#ifndef ___FXR_at_iommu_CSRS_H__
#define ___FXR_at_iommu_CSRS_H__

// FXR_AT_IOMMU_CFG_MAW desc:
typedef union {
    struct {
        uint64_t                  maw  :  1; // 0=48 bit Maximum Virtual Address width, 1=57 bit Maximum Virtual Address width
        uint64_t        Reserved_63_1  : 63; // Unused
    } field;
    uint64_t val;
} FXR_AT_IOMMU_CFG_MAW_t;

// FXR_AT_IOMMU_CFG_DEFEATURE desc:
typedef union {
    struct {
        uint64_t     iommu_defeatures  : 64; // Data read or to be written to the IOMMU Defeature registers
    } field;
    uint64_t val;
} FXR_AT_IOMMU_CFG_DEFEATURE_t;

// FXR_AT_IOMMU_ERR_STS_1 desc:
typedef union {
    struct {
        uint64_t iommu_tag_parity_err  :  1; // IOMMU tag register file parity error ERR_CATEGORY_HFI
        uint64_t iommu_data_parity_err  :  1; // IOMMU data register file parity error ERR_CATEGORY_HFI
        uint64_t iommu_pwt_parity_err  :  1; // IOMMU PWT register file parity error ERR_CATEGORY_HFI
        uint64_t    iommu_seq_msg_err  :  1; // IOMMU Sequencer Interface message error ERR_CATEGORY_HFI
        uint64_t    iommu_msi_msg_err  :  1; // IOMMU Sequencer MSI message reserved decode error ERR_CATEGORY_HFI
        uint64_t        Reserved_63_5  : 59; // Unused
    } field;
    uint64_t val;
} FXR_AT_IOMMU_ERR_STS_1_t;

// FXR_AT_IOMMU_ERR_CLR_1 desc:
typedef union {
    struct {
        uint64_t               events  :  5; // Write 1's to clear corresponding status bits.
        uint64_t        Reserved_63_5  : 59; // Unused
    } field;
    uint64_t val;
} FXR_AT_IOMMU_ERR_CLR_1_t;

// FXR_AT_IOMMU_ERR_FRC_1 desc:
typedef union {
    struct {
        uint64_t               events  :  5; // Write 1's to clear corresponding status bits.
        uint64_t        Reserved_63_5  : 59; // Unused
    } field;
    uint64_t val;
} FXR_AT_IOMMU_ERR_FRC_1_t;

// FXR_AT_IOMMU_ERR_EN_HOST_1 desc:
typedef union {
    struct {
        uint64_t               events  :  5; // Write 1's to enable interrupt.
        uint64_t        Reserved_63_5  : 59; // Unused
    } field;
    uint64_t val;
} FXR_AT_IOMMU_ERR_EN_HOST_1_t;

// FXR_AT_IOMMU_ERR_FIRST_HOST_1 desc:
typedef union {
    struct {
        uint64_t               events  :  5; // Write 1's to clear corresponding status bits.
        uint64_t        Reserved_63_5  : 59; // Unused
    } field;
    uint64_t val;
} FXR_AT_IOMMU_ERR_FIRST_HOST_1_t;

// FXR_AT_IOMMU_ERR_EN_BMC_1 desc:
typedef union {
    struct {
        uint64_t               events  :  5; // Write 1's to enable interrupt.
        uint64_t        Reserved_63_5  : 59; // Unused
    } field;
    uint64_t val;
} FXR_AT_IOMMU_ERR_EN_BMC_1_t;

// FXR_AT_IOMMU_ERR_FIRST_BMC_1 desc:
typedef union {
    struct {
        uint64_t               events  :  5; // Write 1's to clear corresponding status bits.
        uint64_t        Reserved_63_5  : 59; // Unused
    } field;
    uint64_t val;
} FXR_AT_IOMMU_ERR_FIRST_BMC_1_t;

// FXR_AT_IOMMU_ERR_EN_QUAR_1 desc:
typedef union {
    struct {
        uint64_t               events  :  5; // Write 1's to enable interrupt.
        uint64_t        Reserved_63_5  : 59; // Unused
    } field;
    uint64_t val;
} FXR_AT_IOMMU_ERR_EN_QUAR_1_t;

// FXR_AT_IOMMU_ERR_FIRST_QUAR_1 desc:
typedef union {
    struct {
        uint64_t               events  :  5; // Write 1's to clear corresponding status bits.
        uint64_t        Reserved_63_5  : 59; // Unused
    } field;
    uint64_t val;
} FXR_AT_IOMMU_ERR_FIRST_QUAR_1_t;

// FXR_AT_IOMMU_ERR_INFO_1A desc:
typedef union {
    struct {
        uint64_t            iommu_pe0  :  1; // IOMMU Parity Error RF0
        uint64_t          iommu_pe0_1  :  1; // IOMMU Parity Error RF1
        uint64_t          iommu_pe0_2  :  1; // IOMMU Parity Error RF2
        uint64_t        Reserved_63_3  : 61; // Unused
    } field;
    uint64_t val;
} FXR_AT_IOMMU_ERR_INFO_1A_t;

// FXR_AT_IOMMU_STS_GENERAL_1 desc:
typedef union {
    struct {
        uint64_t          dtlb_id_err  :  1; // Device TLB ID error at IOMMU Seq. interface
        uint64_t        Reserved_63_1  : 63; // Unused
    } field;
    uint64_t val;
} FXR_AT_IOMMU_STS_GENERAL_1_t;

// FXR_AT_IOMMU_DBG_GENERAL_1 desc:
typedef union {
    struct {
        uint64_t        mem_req_abort  :  1; // IOMMU memory request received an abort from HIFIS
        uint64_t        Reserved_63_1  : 63; // Unused
    } field;
    uint64_t val;
} FXR_AT_IOMMU_DBG_GENERAL_1_t;

#endif /* ___FXR_at_iommu_CSRS_H__ */