// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Thu May 19 17:39:38 2016
//

#ifndef ___FXR_at_CSRS_H__
#define ___FXR_at_CSRS_H__

// FXR_AT_CFG_CACHE desc:
typedef union {
    struct {
        uint64_t       cache_seg_mode  :  1; // 0=Segment by page size, 1=Combine 4k/2m entries in same cache segment
        uint64_t         Reserved_7_1  :  7; // Reserved
        uint64_t          num_4k_ways  :  5; // This is the number of ways that should be reserved for 4K pages. It has a range of 0 to 16.If CACHE_SEG_MODE=1, this parameter is used for the number of 4k/2m entry ways.
        uint64_t       Reserved_15_13  :  3; // Reserved
        uint64_t          num_2m_ways  :  5; // This is the number of ways that should be reserved for 2M pages. It has a range of 0 to 16.If CACHE_SEG_MODE=1, this parameter is ignored
        uint64_t       Reserved_23_21  :  3; // Reserved
        uint64_t          num_1g_ways  :  2; // This is the number of ways that should be reserved for 1 GB pages. It has a range of 0 to 2
        uint64_t       Reserved_63_26  : 38; // Reserved
    } field;
    uint64_t val;
} FXR_AT_CFG_CACHE_t;

// FXR_AT_CFG_PW_ALLOC desc:
typedef union {
    struct {
        uint64_t       shared_pw_pool  :  5; // The number of Page Walks shared between AT clients
        uint64_t        Reserved_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} FXR_AT_CFG_PW_ALLOC_t;

// FXR_AT_CFG_INPUT_CREDIT desc:
typedef union {
    struct {
        uint64_t    txdma_req_credits  :  4; // The number of input request buffers for TX OTR Address Translation requests. It has a range of 0 to 4
        uint64_t         Reserved_7_4  :  4; // Reserved
        uint64_t    txotr_req_credits  :  4; // The number of input request buffers for TX OTR Address Translation requests. It has a range of 0 to 4
        uint64_t       Reserved_15_12  :  4; // Reserved
        uint64_t  rxhiarb_req_credits  :  4; // The number of input request buffers for RX HIARB Address Translation requests. It has a range of 0 to 4
        uint64_t       Reserved_23_20  :  4; // Reserved
        uint64_t     pcim_req_credits  :  3; // The number of input request buffers for PCIM Interrupt Remapping requests. It has a range of 0 to 4
        uint64_t       Reserved_63_27  : 37; // Reserved
    } field;
    uint64_t val;
} FXR_AT_CFG_INPUT_CREDIT_t;

// FXR_AT_CFG_USE_SYSTEM_PASID desc:
typedef union {
    struct {
        uint64_t               enable  :  1; // Enable use of System PASID
        uint64_t         system_pasid  : 20; // System PASID
        uint64_t       Reserved_63_21  : 43; // Reserved
    } field;
    uint64_t val;
} FXR_AT_CFG_USE_SYSTEM_PASID_t;

// FXR_AT_CFG_DRAIN_PASID desc:
typedef union {
    struct {
        uint64_t                 busy  :  1; // SW sets when writing the PASID field. HW clears when all page requests have been serviced. SW polls for it to be clear before considering the PASID drained.
        uint64_t                pasid  : 20; // The PASID to drain.
        uint64_t       Reserved_63_21  : 43; // Reserved
    } field;
    uint64_t val;
} FXR_AT_CFG_DRAIN_PASID_t;

// FXR_AT_CFG_LTDPR desc:
typedef union {
    struct {
        uint64_t           Reserved_0  :  1; // Reserved
        uint64_t           ProtPegSts  :  1; // Protected Region Status - 1=Protected, 0=Not Protected
        uint64_t           CommandBit  :  1; // 1=enable protection, 0=disable protection
        uint64_t           Reserved_3  :  1; // Reserved
        uint64_t                 Size  :  8; // DMA Protected Range Size - Max. 255MB
        uint64_t       Reserved_19_12  :  8; // Reserved
        uint64_t             TopOfDPR  : 12; // Top Address + 1 of the DMA Protected Region
        uint64_t       Reserved_63_32  : 32; // Reserved
    } field;
    uint64_t val;
} FXR_AT_CFG_LTDPR_t;

// FXR_AT_CFG_GENPROT_BASE desc:
typedef union {
    struct {
        uint64_t        Reserved_15_0  : 16; // Reserved
        uint64_t                 ADDR  : 36; // Base Address [51:16] of generic memory address range that needs to be protected from inbound dma accesses. The protected memory range can be anywhere in the memory space addressable by the processor. Addresses that fall in this range i.e. GenProtRange.Base[63:16] <= Address [63:16] <= GenProtRange.Limit [63:16], are completer aborted by IIO. Setting the Protected range base address greater than the limit address disables the protected memory region. Note that this range is orthogonal to VT-d spec defined protected address range. Since this register provides for a generic range, it can be used to protect any system dram region or MMIO region from DMA accesses. But the expected usage for this range is to abort all PCIe accesses to the PCI-Segments region.
        uint64_t       Reserved_63_52  : 12; // Reserved
    } field;
    uint64_t val;
} FXR_AT_CFG_GENPROT_BASE_t;

// FXR_AT_CFG_GENPROT_LIMIT desc:
typedef union {
    struct {
        uint64_t        Reserved_15_0  : 16; // Reserved
        uint64_t                 ADDR  : 36; // Limit Address [51:16] of generic memory address range that needs to be protected from inbound dma accesses. The protected memory range can be anywhere in the memory space addressable by the processor. Addresses that fall in this range i.e. GenProtRange.Base[63:16] <= Address [63:16] <= GenProtRange.Limit [63:16], are completer aborted by IIO. Setting the Protected range base address greater than the limit address disables the protected memory region. Note that this range is orthogonal to VT-d spec defined protected address range. Since this register provides for a generic range, it can be used to protect any system dram region or MMIO region from DMA accesses. But the expected usage for this range is to abort all PCIe accesses to the PCI-Segments region.
        uint64_t       Reserved_63_52  : 12; // Reserved
    } field;
    uint64_t val;
} FXR_AT_CFG_GENPROT_LIMIT_t;

// FXR_AT_CFG_IOMMU_DEFEATURE desc:
typedef union {
    struct {
        uint64_t     iommu_defeatures  : 64; // Data read or to be written to the IOMMU Defeature registers
    } field;
    uint64_t val;
} FXR_AT_CFG_IOMMU_DEFEATURE_t;

// FXR_AT_CFG_PASID_LUT desc:
typedef union {
    struct {
        uint64_t               enable  :  1; // Enable PID to PASID mapping for this PID
        uint64_t      privilege_level  :  1; // Privilege Level (0 = Supervisor, 1 = User)
        uint64_t                pasid  : 20; // PASID associated with this PID
        uint64_t       reserved_63_22  : 42; // Data read or to be written to the AT PASID LUT entry
    } field;
    uint64_t val;
} FXR_AT_CFG_PASID_LUT_t;

// FXR_AT_ERR_STS_1 desc:
typedef union {
    struct {
        uint64_t       iommu_disabled  :  1; // IOMMU disabled, no device PASID memory access
        uint64_t         ats_disabled  :  1; // Address Translation Disabled, no device PASID memory access
        uint64_t       PASID_disabled  :  1; // PASID access disabled, no device PASID memory access
        uint64_t       PASID_map_fail  :  1; // PID to PASID mapping failed. Invalid PASID entry
        uint64_t      PASID_priv_fail  :  1; // PID to PASID mapping failed. Request Privilege Level .NE. PASID Map Privilege Level
        uint64_t      pf_pgr_disabled  :  1; // page fault for request that could have initiated Page Group Request
        uint64_t      pw_mem_type_err  :  1; // Page Walk returned a memory type inconsistent with memory types supported by device.
        uint64_t             plmr_err  :  1; // PLMR range check failed
        uint64_t             phmr_err  :  1; // PHMR range check failed
        uint64_t            ltdpr_err  :  1; // LTDPR range check failed
        uint64_t          genprot_err  :  1; // GenProt range check failed
        uint64_t          pgr_rsp_err  :  1; // PageGroup Response Error
        uint64_t       pgr_rsp_err_of  :  1; // PageGroup Response Error Overflow
        uint64_t        iommu_rsp_err  :  1; // Unexpected IOMMU Response
        uint64_t          future_err0  :  1; // Place Holder for new error event
        uint64_t          future_err1  :  1; // Place Holder for new error event
        uint64_t         ptec_tag_mbe  :  1; // PTEC Cache Tag MBE
        uint64_t        ptec_data_mbe  :  1; // PTEC Cache Data MBE
        uint64_t               tr_mbe  :  1; // Translation Request Buffer MBE
        uint64_t          pw_trid_mbe  :  1; // Page Walk TRID Buffer MBE
        uint64_t       pw_trid_of_mbe  :  1; // PageWalk TRID Overflow Buffer MBE
        uint64_t          pw_resp_mbe  :  1; // PageWalk Response Buffer MBE
        uint64_t        pid_pasid_mbe  :  1; // PID to PASID Look Up Table MBE
        uint64_t     pte_lru_2m4k_mbe  :  1; // PTE Cache LRU 2M/4K State Table MBE
        uint64_t       pte_lru_1g_mbe  :  1; // PTE Cache LRU 1G State Table MBE
        uint64_t         ptec_tag_sbe  :  1; // PTEC Cache Tag SBE
        uint64_t        ptec_data_sbe  :  1; // PTEC Cache Data SBE
        uint64_t               tr_sbe  :  1; // Translation Request Buffer SBE
        uint64_t          pw_trid_sbe  :  1; // Page Walk TRID Buffer SBE
        uint64_t       pw_trid_of_sbe  :  1; // PageWalk TRID Overflow Buffer SBE
        uint64_t          pw_resp_sbe  :  1; // PageWalk Response Buffer SBE
        uint64_t        pid_pasid_sbe  :  1; // PID to PASID Look Up Table SBE
        uint64_t     pte_lru_2m4k_sbe  :  1; // PTE Cache LRU 2M/4K State Table SBE
        uint64_t       pte_lru_1g_sbe  :  1; // PTE Cache LRU 1G State Table SBE
        uint64_t       Reserved_63_34  : 30; // Reserved
    } field;
    uint64_t val;
} FXR_AT_ERR_STS_1_t;

// FXR_AT_ERR_CLR_1 desc:
typedef union {
    struct {
        uint64_t               events  : 31; // Write 1's to clear corresponding status bits.
        uint64_t       Reserved_63_31  : 33; // Reserved
    } field;
    uint64_t val;
} FXR_AT_ERR_CLR_1_t;

// FXR_AT_ERR_FRC_1 desc:
typedef union {
    struct {
        uint64_t               events  : 31; // Write 1's to clear corresponding status bits.
        uint64_t       Reserved_63_31  : 33; // Reserved
    } field;
    uint64_t val;
} FXR_AT_ERR_FRC_1_t;

// FXR_AT_ERR_EN_HOST_1 desc:
typedef union {
    struct {
        uint64_t               events  : 31; // Write 1's to clear corresponding status bits.
        uint64_t       Reserved_63_31  : 33; // Reserved
    } field;
    uint64_t val;
} FXR_AT_ERR_EN_HOST_1_t;

// FXR_AT_ERR_FIRST_HOST_1 desc:
typedef union {
    struct {
        uint64_t               events  : 31; // Write 1's to clear corresponding status bits.
        uint64_t       Reserved_63_31  : 33; // Reserved
    } field;
    uint64_t val;
} FXR_AT_ERR_FIRST_HOST_1_t;

// FXR_AT_ERR_EN_BMC_1 desc:
typedef union {
    struct {
        uint64_t               events  : 31; // Write 1's to clear corresponding status bits.
        uint64_t       Reserved_63_31  : 33; // Reserved
    } field;
    uint64_t val;
} FXR_AT_ERR_EN_BMC_1_t;

// FXR_AT_ERR_FIRST_BMC_1 desc:
typedef union {
    struct {
        uint64_t               events  : 31; // Write 1's to clear corresponding status bits.
        uint64_t       Reserved_63_31  : 33; // Reserved
    } field;
    uint64_t val;
} FXR_AT_ERR_FIRST_BMC_1_t;

// FXR_AT_ERR_EN_QUAR_1 desc:
typedef union {
    struct {
        uint64_t               events  : 31; // Write 1's to clear corresponding status bits.
        uint64_t       Reserved_63_31  : 33; // Reserved
    } field;
    uint64_t val;
} FXR_AT_ERR_EN_QUAR_1_t;

// FXR_AT_ERR_FIRST_QUAR_1 desc:
typedef union {
    struct {
        uint64_t               events  : 31; // Write 1's to clear corresponding status bits.
        uint64_t       Reserved_63_31  : 33; // Reserved
    } field;
    uint64_t val;
} FXR_AT_ERR_FIRST_QUAR_1_t;

// FXR_AT_ERR_INFO_1A desc:
typedef union {
    struct {
        uint64_t   ptec_tag_mbe_count  :  8; // ptec_tag_mbe_count
        uint64_t   ptec_tag_sbe_count  :  8; // PTEC(TLB) Entry MBE saturating count
        uint64_t     tag_syndrome_mbe  :  8; // Syndrome of the last tag MBE
        uint64_t     tag_syndrome_sbe  :  8; // Syndrome of the last tag SBE
        uint64_t ptec_entry_mbe_count  :  8; // PTEC(TLB) Entry MBE saturating count
        uint64_t ptec_entry_sbe_count  :  8; // PTEC(TLB) Entry SBE saturating count
        uint64_t    data_syndrome_mbe  :  8; // Syndrome of the last data MBE
        uint64_t    data_syndrome_sbe  :  8; // Syndrome of the last data SBE
    } field;
    uint64_t val;
} FXR_AT_ERR_INFO_1A_t;

// FXR_AT_ERR_INFO_1B desc:
typedef union {
    struct {
        uint64_t      tbufr_mbe_count  :  8; // Translation Request Buffer MBE saturating count
        uint64_t      trbuf_sbe_count  :  8; // Translation Request Buffer SBE saturating count
        uint64_t   trbuf_syndrome_mbe  :  8; // Syndrome of the last trbuf MBE
        uint64_t   trbuf_syndrome_sbe  :  8; // Syndrome of the last trbuf SBE
        uint64_t      pwbuf_mbe_count  :  8; // Page Walk TRID Buffer MBE saturating count
        uint64_t      pwbuf_sbe_count  :  8; // Page Walk TRID Buffer SBE saturating count
        uint64_t   pwbuf_syndrome_mbe  :  8; // Syndrome of the last tpwbufMBE
        uint64_t   pwbuf_syndrome_sbe  :  8; // Syndrome of the last pwbuf SBE
    } field;
    uint64_t val;
} FXR_AT_ERR_INFO_1B_t;

// FXR_AT_ERR_INFO_1C desc:
typedef union {
    struct {
        uint64_t      pw_of_mbe_count  :  8; // PageWalk Overflow Buffer MBE saturating count
        uint64_t      pw_of_sbe_count  :  8; // PageWalk Overflow Buffer SBE saturating count
        uint64_t   pw_of_syndrome_mbe  :  8; // Syndrome of the last pwof buf MBE
        uint64_t   pw_of_syndrome_sbe  :  8; // Syndrome of the last pwof buf SBE
        uint64_t    pw_resp_mbe_count  :  8; // PageWalk Response Buffer MBE saturating count
        uint64_t    pw_resp_sbe_count  :  8; // PageWalk Response Buffer SBE saturating count
        uint64_t pw_resp_syndrome_mbe  :  8; // Syndrome of the last pw resp buf MBE
        uint64_t pw_resp_syndrome_sbe  :  8; // Syndrome of the last pw resp buf SBE
    } field;
    uint64_t val;
} FXR_AT_ERR_INFO_1C_t;

// FXR_AT_ERR_INFO_1D desc:
typedef union {
    struct {
        uint64_t  pid_pasid_mbe_count  :  8; // PID to PASID Look Up Table MBE saturating count
        uint64_t  pid_pasid_sbe_count  :  8; // PID to PASID Look Up Table SBE saturating count
        uint64_t pid_pasid_syndrome_mbe  :  8; // Syndrome of the last pid to pasid MBE
        uint64_t pid_pasid_syndrome_sbe  :  8; // Syndrome of the last pid to pasid SBE
        uint64_t pte_lru_2m4k_mbe_count  :  8; // PTE Cache LRU 2M/4K State Table MBE saturating count
        uint64_t pte_lru_2m4k_sbe_count  :  8; // PTE Cache LRU 2M/4K State Table SBE saturating count
        uint64_t lru_2m4k_syndrome_mbe  :  8; // Syndrome of the last lru2m4k MBE
        uint64_t lru_2m4k_syndrome_sbe  :  8; // Syndrome of the last lru 2m4k SBE
    } field;
    uint64_t val;
} FXR_AT_ERR_INFO_1D_t;

// FXR_AT_ERR_INFO_1E desc:
typedef union {
    struct {
        uint64_t pte_lru_1g_mbe_count  :  8; // PTE Cache LRU 1G State Table MBE saturating count
        uint64_t pte_lru_1g_sbe_count  :  8; // PTE Cache LRU 1G State Table SBE saturating count
        uint64_t  lru_1g_syndrome_mbe  :  8; // Syndrome of the last PTECLRU 1G MBE
        uint64_t  lru_1g_syndrome_sbe  :  8; // Syndrome of the last PTECLRU 1G SBE
        uint64_t       reserved_63_32  : 32; // reserved
    } field;
    uint64_t val;
} FXR_AT_ERR_INFO_1E_t;

// FXR_AT_ERR_INFO_1F desc:
typedef union {
    struct {
        uint64_t      pgr_error_pasid  : 20; // PASID of Page Group Request Error
        uint64_t       reserved_31_20  : 12; // reserved
        uint64_t     pgr_error_client  :  3; // Requesting Client ID of Page Group Request Error. 001=TXDMA 010=TXOTR 100=RXDMA
        uint64_t       reserved_63_35  : 29; // reserved
    } field;
    uint64_t val;
} FXR_AT_ERR_INFO_1F_t;

// FXR_AT_ERR_INFO_1G desc:
typedef union {
    struct {
        uint64_t           reserved_0  :  1; // reserved
        uint64_t          pgr_error_r  :  1; // reserved
        uint64_t          pgr_error_w  :  1; // reserved
        uint64_t         pgr_error_pa  :  1; // reserved
        uint64_t        reserved_11_4  :  8; // reserved
        uint64_t         pgr_error_va  : 40; // Virtual Address of Page Group Request Error
        uint64_t       reserved_63_52  : 12; // reserved
    } field;
    uint64_t val;
} FXR_AT_ERR_INFO_1G_t;

// FXR_AT_STS_PLMR_BASE desc:
typedef union {
    struct {
        uint64_t                 pmem  :  1; // Protected Memory Region Checking Enabled
        uint64_t        reserved_19_1  : 19; // reserved
        uint64_t            plmr_base  : 12; // PLMR Base
        uint64_t       reserved_63_32  : 32; // reserved
    } field;
    uint64_t val;
} FXR_AT_STS_PLMR_BASE_t;

// FXR_AT_STS_PLMR_LIMIT desc:
typedef union {
    struct {
        uint64_t        reserved_19_0  : 20; // reserved
        uint64_t           plmr_limit  : 12; // PLMR Limit
        uint64_t       reserved_63_32  : 32; // reserved
    } field;
    uint64_t val;
} FXR_AT_STS_PLMR_LIMIT_t;

// FXR_AT_STS_PHMR_BASE desc:
typedef union {
    struct {
        uint64_t        reserved_19_0  : 20; // reserved
        uint64_t            phmr_base  : 28; // PHMR Base
        uint64_t       reserved_63_48  : 16; // reserved
    } field;
    uint64_t val;
} FXR_AT_STS_PHMR_BASE_t;

// FXR_AT_STS_PHMR_LIMIT desc:
typedef union {
    struct {
        uint64_t        reserved_19_0  : 20; // reserved
        uint64_t           phmr_limit  : 28; // PHMR Limit
        uint64_t       reserved_63_48  : 16; // reserved
    } field;
    uint64_t val;
} FXR_AT_STS_PHMR_LIMIT_t;

// FXR_AT_STS_GENERAL_1 desc:
typedef union {
    struct {
        uint64_t       available_reqs  :  8; // Available Translation Request slots
        uint64_t           pw_rsp_fsm  :  3; // PW RSP FSM states
        uint64_t          reserved_11  :  1; // reserved
        uint64_t        inval_cmd_fsm  :  3; // Inval Cmd FSM states
        uint64_t       reserved_63_15  : 49; // reserved
    } field;
    uint64_t val;
} FXR_AT_STS_GENERAL_1_t;

// FXR_AT_DBG_TREQ_BUF_ADDR desc:
typedef union {
    struct {
        uint64_t              address  : 52; // Address of Translation Request to be accessed
        uint64_t         payload_regs  :  8; // Constant indicating number of payload registers used for the data.
        uint64_t       Reserved_61_60  :  2; // 
        uint64_t                Write  :  1; // 0=Read, 1=Write
        uint64_t                Valid  :  1; // Set by Software to start command, cleared by Hardware when complete
    } field;
    uint64_t val;
} FXR_AT_DBG_TREQ_BUF_ADDR_t;

// FXR_AT_DBG_TREQ_BUF_DATA0 desc:
typedef union {
    struct {
        uint64_t                 data  : 64; // Data read or to be written to the Translation Request entry
    } field;
    uint64_t val;
} FXR_AT_DBG_TREQ_BUF_DATA0_t;

// FXR_AT_DBG_TREQ_BUF_DATA1 desc:
typedef union {
    struct {
        uint64_t                 data  : 16; // Data read or to be written to the Translation Request entry
        uint64_t       reserved_63_16  : 48; // Reserved
    } field;
    uint64_t val;
} FXR_AT_DBG_TREQ_BUF_DATA1_t;

// FXR_AT_DBG_PW_RSP_BUF_ADDR desc:
typedef union {
    struct {
        uint64_t              address  : 52; // Address of 2M/4Kentry to be accessed
        uint64_t         payload_regs  :  8; // Constant indicating number of payload registers used for the data.
        uint64_t       Reserved_61_60  :  2; // 
        uint64_t                Write  :  1; // 0=Read, 1=Write
        uint64_t                Valid  :  1; // Set by Software to start command, cleared by Hardware when complete
    } field;
    uint64_t val;
} FXR_AT_DBG_PW_RSP_BUF_ADDR_t;

// FXR_AT_DBG_PW_RSP_BUF_DATA0 desc:
typedef union {
    struct {
        uint64_t                 data  : 64; // Data read or to be written to the Cache 2M/4K page LRU entry
    } field;
    uint64_t val;
} FXR_AT_DBG_PW_RSP_BUF_DATA0_t;

// FXR_AT_DBG_PW_RSP_BUF_DATA1 desc:
typedef union {
    struct {
        uint64_t                 data  : 16; // Data read or to be written to the Cache 2M/4K page LRU entry
        uint64_t       reserved_63_16  : 48; // Reserved
    } field;
    uint64_t val;
} FXR_AT_DBG_PW_RSP_BUF_DATA1_t;

// FXR_AT_DBG_LRU2M4K_SETS_ADDR desc:
typedef union {
    struct {
        uint64_t              address  : 52; // Address of 2M/4Kentry to be accessed
        uint64_t         payload_regs  :  8; // Constant indicating number of payload registers used for the data.
        uint64_t       Reserved_61_60  :  2; // 
        uint64_t                Write  :  1; // 0=Read, 1=Write
        uint64_t                Valid  :  1; // Set by Software to start command, cleared by Hardware when complete
    } field;
    uint64_t val;
} FXR_AT_DBG_LRU2M4K_SETS_ADDR_t;

// FXR_AT_DBG_LRU2M4K_SETS_DATA0 desc:
typedef union {
    struct {
        uint64_t                 data  : 64; // Data read or to be written to the Cache 2M/4K page LRU entry
    } field;
    uint64_t val;
} FXR_AT_DBG_LRU2M4K_SETS_DATA0_t;

// FXR_AT_DBG_LRU2M4K_SETS_DATA1 desc:
typedef union {
    struct {
        uint64_t                 data  : 16; // Data read or to be written to the Cache 2M/4K page LRU entry
        uint64_t       reserved_63_16  : 48; // Reserved
    } field;
    uint64_t val;
} FXR_AT_DBG_LRU2M4K_SETS_DATA1_t;

// FXR_AT_DBG_TLB_TAG_ADDR desc:
typedef union {
    struct {
        uint64_t              address  : 48; // Address of the TLB Tag to be accessed
        uint64_t                  way  :  4; // Way index to be accessed
        uint64_t         payload_regs  :  8; // Constant indicating number of payload registers used for the data.
        uint64_t       Reserved_61_60  :  2; // 
        uint64_t                Write  :  1; // 0=Read, 1=Write
        uint64_t                Valid  :  1; // Set by Software to start command, cleared by Hardware when complete
    } field;
    uint64_t val;
} FXR_AT_DBG_TLB_TAG_ADDR_t;

// FXR_AT_DBG_TLB_TAG_DATA0 desc:
typedef union {
    struct {
        uint64_t                 data  : 64; // Data read or to be written to the TLB Tag entry
    } field;
    uint64_t val;
} FXR_AT_DBG_TLB_TAG_DATA0_t;

// FXR_AT_DBG_TLB_TAG_DATA1 desc:
typedef union {
    struct {
        uint64_t                 data  :  8; // Data read or to be written to the TLB Tag entry
        uint64_t        reserved_63_8  : 56; // Reserved
    } field;
    uint64_t val;
} FXR_AT_DBG_TLB_TAG_DATA1_t;

// FXR_AT_DBG_TLB_DATA desc:
typedef union {
    struct {
        uint64_t                 data  : 64; // Data read or to be written to the AT Cache Data(PTE) entry
    } field;
    uint64_t val;
} FXR_AT_DBG_TLB_DATA_t;

// FXR_AT_DBG_PW_ID_BUF desc:
typedef union {
    struct {
        uint64_t          pw_id_entry  :  8; // Page Walk TRID buffer entry
        uint64_t        Reserved_63_8  : 56; // Reserved
    } field;
    uint64_t val;
} FXR_AT_DBG_PW_ID_BUF_t;

// FXR_AT_DBG_PW_ID_OF_BUF desc:
typedef union {
    struct {
        uint64_t       pw_id_of_entry  :  8; // Page Walk TRID Overflow buffer entry
        uint64_t        Reserved_63_8  : 56; // Reserved
    } field;
    uint64_t val;
} FXR_AT_DBG_PW_ID_OF_BUF_t;

// FXR_AT_DBG_LRU1G_SETS desc:
typedef union {
    struct {
        uint64_t          lru1g_entry  :  8; // Cache 1G page LRU state entry(max 2 way config).
        uint64_t        Reserved_63_8  : 56; // Reserved
    } field;
    uint64_t val;
} FXR_AT_DBG_LRU1G_SETS_t;

#endif /* ___FXR_at_CSRS_H__ */