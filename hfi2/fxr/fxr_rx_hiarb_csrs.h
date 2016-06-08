// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Thu Jun  2 19:11:24 2016
//

#ifndef ___FXR_rx_hiarb_CSRS_H__
#define ___FXR_rx_hiarb_CSRS_H__

// RXHIARB_CFG_PCB_LOW desc:
typedef union {
    struct {
        uint64_t                valid  :  1; // Valid. This entry is valid.
        uint64_t        Reserved_11_1  : 11; // Reserved
        uint64_t   portals_state_base  : 45; // Portals State Base. A 4k aligned address describing the base of the Portals State region in memory
        uint64_t       Reserved_63_57  :  7; // Reserved
    } field;
    uint64_t val;
} RXHIARB_CFG_PCB_LOW_t;

// RXHIARB_CFG_PCB_HIGH desc:
typedef union {
    struct {
        uint64_t    triggered_op_size  : 11; // Triggered Operation Size. The number of 4k pages assigned to Triggered Operation Entries. Each page holds 32 128B entries. This number is defined as the number of 4K pages being reserved minus 1. Therefore a value of 0 is interpreted as 1 4K page, a value of 1 = 2 4K pages, a value of 2 = 3 4K pages. and so on until a value of 2047 = 2048 4K pages. This restricts the range of 4K pages to 1-2048.
        uint64_t       Reserved_15_11  :  5; // Reserved
        uint64_t      unexpected_size  : 11; // Unexpected Size. The number of 4k pages assigned to Unexpected Headers and Tracking Entries. Each page holds 32 128B entries. This number is defined as the number of 4K pages being reserved minus 1. Therefore a value of 0 is interpreted as 1 4K page, a value of 1 = 2 4K pages, a value of 2 = 3 4K pages. and so on until a value of 2047 = 2048 4K pages. This restricts the range of 4K pages to 1-2048.
        uint64_t       Reserved_31_27  :  5; // Reserved
        uint64_t           le_me_size  : 11; // LE/ME Size. The number of 4k pages assigned to LE/ME entries. Each page holds 32 128B entries. This number is defined as the number of 4K pages being reserved minus 1. Therefore a value of 0 is interpreted as 1 4K page, a value of 1 = 2 4K pages, a value of 2 = 3 4K pages. and so on until a value of 2047 = 2048 4K pages. This restricts the range of 4K pages to 1-2048.
        uint64_t       Reserved_63_43  : 21; // Reserved
    } field;
    uint64_t val;
} RXHIARB_CFG_PCB_HIGH_t;

// RXHIARB_CFG_HQUE_WM desc:
typedef union {
    struct {
        uint64_t         hque_wm_low0  :  4; // Low watermark value for holding queue 0. Note that the legal supported values are 0, 2, 4. or 8. Attempting to assign this to an unsupported value may result in an unreliable system, especially if the the sum of all 9 low watermarks if greater then the value of the high watermark.
        uint64_t         hque_wm_low1  :  4; // Low watermark value for holding queue 1. See hque_wm_low0 for supported values.
        uint64_t         hque_wm_low2  :  4; // Low watermark value for holding queue 2, See hque_wm_low0 for supported values.
        uint64_t         hque_wm_low3  :  4; // Low watermark value for holding queue 3. See hque_wm_low0 for supported values.
        uint64_t         hque_wm_low4  :  4; // Low watermark value for holding queue 4. See hque_wm_low0 for supported values.
        uint64_t         hque_wm_low5  :  4; // Low watermark value for holding queue 5, See hque_wm_low0 for supported values.
        uint64_t         hque_wm_low6  :  4; // Low watermark value for holding queue 6. See hque_wm_low0 for supported values.
        uint64_t         hque_wm_low7  :  4; // Low watermark value for holding queue 7. See hque_wm_low0 for supported values.
        uint64_t         hque_wm_low8  :  4; // Low watermark value for holding queue 8, See hque_wm_low0 for supported values.
        uint64_t       Reserved_39_36  :  4; // Reserved
        uint64_t       hque_wm_hi_all  :  8; // Total number of requests allowed to reside within all holding queues. Though this may hold values of up to 128, the supported legal values are a maximum of 112 and a minimum of 72 in increments of 8, hence 72, 80, 88, 96, 104, and 112, Attempting to assign this unsupported values may result in an unreliable system, especially if the value of the high watermark is less than the sum of all 9 low watermarks. NOTE: If the system has had any errors which cause HID leaks, attempting to redefine this value will be ignored by the logic until the system has been reset. Otherwise if no HID leak has occurred, the logic will accept the change in this value once the corresponding logic has become idle.
        uint64_t       Reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} RXHIARB_CFG_HQUE_WM_t;

// RXHIARB_CFG_PERF_MATCH desc:
typedef union {
    struct {
        uint64_t   at_dly_match_index  :  7; // The mini-TLB index which will be used as the criteria for counting, such that when a miss occurs ,the HPA will be fetched from the AT. If the miss index (mini-TLB cache line that is being allocated/reallocated) matches this value, the counts will begin and continue until the corresponding response returns from the AT.
        uint64_t           Reserved_7  :  1; // Reserved
        uint64_t          at_dly_mask  :  7; // AT match mask. This provides a mask that will be applied to the Mini-TLB index matching. If a bit is asserted, it will be checked during the match testing, otherwise if a bit is de-asserted, it will not be checked. A de-asserted bit acts like a wild-card, allowing for matches of multiple conditions, For example, if the at_dly_match_index is set to 0x00 and this mask is set to 0x7e, matches will occur for both 0x00 and 0x01.
        uint64_t      at_dly_match_en  :  1; // Enable counting. When asserted, counts will occur for all misses when the mini-TLB miss index matches the criteria below, otherwise if de-asserted, no counting will occur.
        uint64_t     rd_dly_match_tid  : 12; // The request's full TID will be used as the criteria for counting such that any read being sent to HIFIs whose TID matches this will start counting and continue until the corresponding response returns from HIFIs. Note that this TID contains both the requesting client ID (in bits [11:8]) and the client's TID (bits [7:0]), This allows the matching criteria to isolate requests based on requester. The upper TIDs [11:8] are defined as follows: 0-3=Rsvd (unused), 4-7 = RxE2E (where the lower 2-bits [9:8] correspond to the lowest 2-bits of the 8B aligned address (used for data alignment) purposes) 8=RxHP 9=RxCID (unused here since RxCID never issues reads) 10-11=RxET where lowest bit [8] corresponds to the lowest 16B aligned address bit (used for data alignment purposes) 12-14=Rsvd (unused) 15=RxDMA Attempting to define TID bits [11:8] to Rsvd/unused values will result in no counts being generated. Note that all clients provide an 8-bit TID [7:0] EXCEPT the RxE2E which only sends a 6-bit TID, therefore TID bits [7:6] must be defined with zeros whenever [11:8] are set to values 4-7, otherwise no counts will be generated.
        uint64_t       Reserved_31_28  :  4; // Reserved
        uint64_t          rd_dly_mask  : 12; // Read match mask. This provides a mask that will be applied to the TID matching. If a bit is asserted, it will be checked during the match testing, otherwise if a bit is de-asserted, it will not be checked. A de-asserted bit acts like a wild-card, allowing for matches of multiple conditions. This can be useful when working with the upper client ID bits (tid[11:8]), For example, attempting to match all RxE2E reads with a client TID of zero (regardless of 8B data alignment), the rd_dly_match_tid would be set to 0x400 and this mask would be set to 0xcff such that matches will occur for 0x400, 0x500, 0x600, and 0x700.
        uint64_t       Reserved_46_44  :  3; // Reserved
        uint64_t      rd_dly_match_en  :  1; // Enable counting. When asserted, counts will occur for all reads sent to HIFIs when the TID matches the criteria below, otherwise if de-asserted, no counting will occur.
        uint64_t       Reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} RXHIARB_CFG_PERF_MATCH_t;

// RXHIARB_STS_MAX_HI_STALL desc:
typedef union {
    struct {
        uint64_t         max_hi_stall  : 16; // Max number of clock cycles that the HI exerts back-pressure. Count saturates without wrapping.
        uint64_t       Reserved_63_16  : 48; // Reserved
    } field;
    uint64_t val;
} RXHIARB_STS_MAX_HI_STALL_t;

// RXHIARB_STS_MAX_AT_STALL desc:
typedef union {
    struct {
        uint64_t         max_at_stall  : 16; // Max number of clock cycles that the AT exerts back-pressure. Count saturates without wrapping.
        uint64_t       Reserved_63_16  : 48; // Reserved
    } field;
    uint64_t val;
} RXHIARB_STS_MAX_AT_STALL_t;

// RXHIARB_STS_RXDMA_IN_0_3 desc:
typedef union {
    struct {
        uint64_t           rxdma_cnt0  :  6; // Number of flits in RxDMA input queue 0
        uint64_t          rxdma_full0  :  1; // RxDMA input queue 0 full
        uint64_t         rxdma_empty0  :  1; // RxDMA input queue 0 empty
        uint64_t           rxdma_cnt1  :  6; // Number of flits in RxDMA input queue 1
        uint64_t          rxdma_full1  :  1; // RxDMA input queue 1 full
        uint64_t         rxdma_empty1  :  1; // RxDMA input queue 1 empty
        uint64_t           rxdma_cnt2  :  6; // Number of flits in RxDMA input queue 2
        uint64_t          rxdma_full2  :  1; // RxDMA input queue 2 full
        uint64_t         rxdma_empty2  :  1; // RxDMA input queue 2 empty
        uint64_t           rxdma_cnt3  :  6; // Number of flits in RxDMA input queue 3
        uint64_t          rxdma_full3  :  1; // RxDMA input queue 3 full
        uint64_t         rxdma_empty3  :  1; // RxDMA input queue 3 empty
        uint64_t       Reserved_63_32  : 32; // Reserved
    } field;
    uint64_t val;
} RXHIARB_STS_RXDMA_IN_0_3_t;

// RXHIARB_STS_RXDMA_IN_4_8 desc:
typedef union {
    struct {
        uint64_t           rxdma_cnt4  :  6; // Number of flits in RxDMA input queue 4
        uint64_t          rxdma_full4  :  1; // RxDMA input queue 4 full
        uint64_t         rxdma_empty4  :  1; // RxDMA input queue 4 empty
        uint64_t           rxdma_cnt5  :  6; // Number of flits in RxDMA input queue 5
        uint64_t          rxdma_full5  :  1; // RxDMA input queue 5 full
        uint64_t         rxdma_empty5  :  1; // RxDMA input queue 5 empty
        uint64_t           rxdma_cnt6  :  6; // Number of flits in RxDMA input queue 6
        uint64_t          rxdma_full6  :  1; // RxDMA input queue 6 full
        uint64_t         rxdma_empty6  :  1; // RxDMA input queue 6 empty
        uint64_t           rxdma_cnt7  :  6; // Number of flits in RxDMA input queue 7
        uint64_t          rxdma_full7  :  1; // RxDMA input queue 7 full
        uint64_t         rxdma_empty7  :  1; // RxDMA input queue 7 empty
        uint64_t           rxdma_cnt8  :  6; // Number of flits in RxDMA input queue 8
        uint64_t          rxdma_full8  :  1; // RxDMA input queue 8 full
        uint64_t         rxdma_empty8  :  1; // RxDMA input queue 8 empty
        uint64_t       Reserved_63_40  : 24; // Reserved
    } field;
    uint64_t val;
} RXHIARB_STS_RXDMA_IN_4_8_t;

// RXHIARB_STS_MISC_IN desc:
typedef union {
    struct {
        uint64_t             rxhp_cnt  :  6; // Number of requests in RxHP input queue 0
        uint64_t            rxhp_full  :  1; // RxHP input queue full
        uint64_t           rxhp_empty  :  1; // RxHP input queue empty
        uint64_t             rxet_cnt  :  6; // Number of requests in RxET input queue
        uint64_t            rxet_full  :  1; // RxET input queue full
        uint64_t           rxet_empty  :  1; // RxET input queue empty
        uint64_t            rxe2e_cnt  :  6; // Number of requests in RxE2E input queue
        uint64_t           rxe2e_full  :  1; // RxE2E input queue full
        uint64_t          rxe2e_empty  :  1; // RxE2E input queue empty
        uint64_t            rxcid_cnt  :  6; // Number of requests in RxCID input queue
        uint64_t           rxcid_full  :  1; // RxCID input queue full
        uint64_t          rxcid_empty  :  1; // RxCID input queue empty
        uint64_t       Reserved_63_32  : 32; // Reserved
    } field;
    uint64_t val;
} RXHIARB_STS_MISC_IN_t;

// RXHIARB_STS_AT_CRED desc:
typedef union {
    struct {
        uint64_t          at_cred_cnt  :  8; // Number of credits available on AT interface
        uint64_t         at_have_cred  :  1; // Have credits available on AT request interface
        uint64_t        Reserved_63_9  : 55; // Reserved
    } field;
    uint64_t val;
} RXHIARB_STS_AT_CRED_t;

// RXHIARB_STS_HI_CRED desc:
typedef union {
    struct {
        uint64_t          hi_cred_cnt  :  8; // Number of credits available on HIFIs interface
        uint64_t         hi_have_cred  :  1; // Have credits available on HIFIs request interface
        uint64_t        Reserved_63_9  : 55; // Reserved
    } field;
    uint64_t val;
} RXHIARB_STS_HI_CRED_t;

// RXHIARB_ERR_STS_1 desc:
typedef union {
    struct {
        uint64_t         pcb_err_nval  :  1; // PCB access to a non-valid entry. Indicates that the requester attempted to access an invalid entry in the PCB table.
        uint64_t         pcb_err_bvio  :  1; // PCB boundary violation. Indicates that either a PCB entry has overlapping regions or that te address calculation for a specific region overlapped into the subsequent PCB region.
        uint64_t    pcb_err_addr_oflw  :  1; // PCB address calculation overflow. Indicates that the requester attempted to access an address beyond the maximum supported address range (e.g. 57-bits for virtual address)
        uint64_t          pcb_err_sbe  :  1; // PCB table single bit error detected,
        uint64_t          pcb_err_mbe  :  1; // PCB table multiple bit error detected,
        uint64_t      mtlb_err_at_rsp  :  1; // Mini-TLB received an error response fro the AT while requesting a physical address.
        uint64_t      rxdma_err_frame  :  1; // Framing error detected on the RxDMA interface. Indicates that a head/tail mismatch occurred during a request from the RxDMA.
        uint64_t        rxdma_err_sbe  :  1; // Single bit error detected on the RxDMA interface.
        uint64_t        rxdma_err_mbe  :  1; // Multiple bit error detected on the RxDMA interface.
        uint64_t       rxhp_err_frame  :  1; // Framing error detected on the RxHP interface. Indicates that the length does not match the number of dval flits between req_valids.
        uint64_t        rxchp_err_sbe  :  1; // Single bit error detected on the RxHP interface.
        uint64_t        rxchp_err_mbe  :  1; // Multiple bit error detected on the RxHP interface.
        uint64_t       rxet_err_frame  :  1; // Framing error detected on the RxET interface. Indicates that the length does not match the number of dval flits between req_valids.
        uint64_t         rxet_err_sbe  :  1; // Single bit error detected on the RxET interface.
        uint64_t         rxet_err_mbe  :  1; // Multiple bit error detected on the RxET interface.
        uint64_t        rxe2e_err_sbe  :  1; // Single bit error detected on the RxE2E interface.
        uint64_t        rxe2e_err_mbe  :  1; // Multiple bit error detected on the RxE2E interface.
        uint64_t        rxcid_err_sbe  :  1; // Single bit error detected on the RxCID interface.
        uint64_t        rxcid_err_mbe  :  1; // Multiple bit error detected on the RxCID interface.
        uint64_t        slrsp_err_sbe  :  1; // Slow response queue single bit error detected.
        uint64_t        slrsp_err_mbe  :  1; // Slow response queue multiple bit error detected.
        uint64_t          fpfifo_perr  :  1; // Free pool FIFO parity error detected.
        uint64_t        payld_err_sbe  :  1; // Payload SRAM single bit error detected.
        uint64_t        payld_err_mbe  :  1; // Payload SRAM multiple bit error detected.
        uint64_t        hque0_err_sbe  :  1; // Holding queue 0 (MCTC=0) single bit error detected.
        uint64_t        hque0_err_mbe  :  1; // Holding queue 0 (MCTC=0) multiple bit error detected.
        uint64_t        hque1_err_sbe  :  1; // Holding queue 1(MCTC=1) single bit error detected.
        uint64_t        hque1_err_mbe  :  1; // Holding queue 1(MCTC=1) multiple bit error detected.
        uint64_t        hque2_err_sbe  :  1; // Holding queue 2 (MCTC=2) single bit error detected.
        uint64_t        hque2_err_mbe  :  1; // Holding queue 2 (MCTC=2) multiple bit error detected.
        uint64_t        hque3_err_sbe  :  1; // Holding queue 3 (MCTC=3) single bit error detected.
        uint64_t        hque3_err_mbe  :  1; // Holding queue 3 (MCTC=3) multiple bit error detected.
        uint64_t        hque4_err_sbe  :  1; // Holding queue 4 (MCTC=4) single bit error detected.
        uint64_t        hque4_err_mbe  :  1; // Holding queue 4 (MCTC=4) multiple bit error detected.
        uint64_t        hque5_err_sbe  :  1; // Holding queue 5 (MCTC=5) single bit error detected.
        uint64_t        hque5_err_mbe  :  1; // Holding queue 5 (MCTC=5) multiple bit error detected.
        uint64_t        hque6_err_sbe  :  1; // Holding queue 6 (MCTC=6) single bit error detected.
        uint64_t        hque6_err_mbe  :  1; // Holding queue 6 (MCTC=6) multiple bit error detected.
        uint64_t        hque7_err_sbe  :  1; // Holding queue 7 (MCTC=7) single bit error detected.
        uint64_t        hque7_err_mbe  :  1; // Holding queue 7 (MCTC=7) multiple bit error detected.
        uint64_t        hque8_err_sbe  :  1; // Holding queue 8 (misc) single bit error detected.
        uint64_t        hque8_err_mbe  :  1; // Holding queue 8 (misc) multiple bit error detected.
        uint64_t       hque_vect_perr  :  1; // Holding queues 0-8 present vector parity error
        uint64_t         mtlb_err_sbe  :  1; // Mini-TLB single bit error detected.
        uint64_t         mtlb_err_mbe  :  1; // Mini-TLB multiple bit error detected.
        uint64_t       mtlb_vect_perr  :  1; // Mini-TLB state vector parity error detected
        uint64_t           atfifo_sbe  :  1; // AT request FIFO single bit error detected
        uint64_t           atfifo_mbe  :  1; // AT request FIFO multiple bit error detected
        uint64_t           hififo_sbe  :  1; // HI request FIFO single bit error detected
        uint64_t           hififo_mbe  :  1; // HI request FIFO multiple bit error detected
        uint64_t       hifi_err_frame  :  1; // HIFIs interface framing error detected
        uint64_t         hifi_err_sbe  :  1; // HIFIs interface single bit error detected
        uint64_t         hifi_err_mbe  :  1; // HIFIs interface multiple bit error detected
        uint64_t         nack_err_sbe  :  1; // Nack queue single bit error detected
        uint64_t         nack_err_mbe  :  1; // Nack queue multiple bit error detected
        uint64_t       Reserved_63_55  :  9; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_STS_1_t;

// RXHIARB_ERR_CLR_1 desc:
typedef union {
    struct {
        uint64_t              err_clr  : 56; // Error clear bits. See error status register for bit assignments.
        uint64_t       Reserved_63_56  :  8; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_CLR_1_t;

// RXHIARB_ERR_FRC_1 desc:
typedef union {
    struct {
        uint64_t              err_frc  : 56; // Error force bits. See error status register for bit assignments.
        uint64_t       Reserved_63_56  :  8; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_FRC_1_t;

// RXHIARB_ERR_EN_HOST_1 desc:
typedef union {
    struct {
        uint64_t          err_host_en  : 56; // Host interrupt enable mask. See error status register for bit assignments.
        uint64_t       Reserved_63_56  :  8; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_EN_HOST_1_t;

// RXHIARB_ERR_FIRST_HOST_1 desc:
typedef union {
    struct {
        uint64_t       err_host_first  : 56; // First host interrupt detected. See error status register for bit assignments.
        uint64_t       Reserved_63_56  :  8; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_FIRST_HOST_1_t;

// RXHIARB_ERR_EN_BMC_1 desc:
typedef union {
    struct {
        uint64_t          err_host_en  : 56; // BMC interrupt enable mask. See error status register for bit assignments.
        uint64_t       Reserved_63_56  :  8; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_EN_BMC_1_t;

// RXHIARB_ERR_FIRST_BMC_1 desc:
typedef union {
    struct {
        uint64_t       err_host_first  : 56; // First BMC interrupt detected. See error status register for bit assignments.
        uint64_t       Reserved_63_56  :  8; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_FIRST_BMC_1_t;

// RXHIARB_ERR_EN_QUAR_1 desc:
typedef union {
    struct {
        uint64_t          err_host_en  : 56; // Quarantine interrupt enable mask. See error status register for bit assignments.
        uint64_t       Reserved_63_56  :  8; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_EN_QUAR_1_t;

// RXHIARB_ERR_FIRST_QUAR_1 desc:
typedef union {
    struct {
        uint64_t       err_host_first  : 56; // First quarantine interrupt detected. See error status register for bit assignments.
        uint64_t       Reserved_63_56  :  8; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_FIRST_QUAR_1_t;

// RXHIARB_ERR_INFO_PCB_NVAL desc:
typedef union {
    struct {
        uint64_t               handle  : 12; // equester handle of the most recent error.
        uint64_t            rx_src_id  :  4; // Source requester ID for most recent error.This indicates the requester who caused the error. 0x0-0x3=rsvd, =0x4=RxE2E, 0x5-0x7=rsvd, 0x8=RxHP. 0x9=RxCID, 0xa=RxET, 0xb-0xe=rsvd, 0xf=RxDMA Note that only the handle-based requesters of RxDMA, RxHP, and RxET will cause this error
        uint64_t              element  : 16; // Requester element of most recent error.
        uint64_t               region  :  3; // PCB region being accessed during most recent error 0=PTE, 1=CT, 2-EQ, 3=SWEQ, 4-TrigOp, 5=LE/ME, 6-UnexpHdr, 7=rsvd Note that SWEQ not used by hardware.
        uint64_t                   ni  :  2; // Requester network number during the most recemt error
        uint64_t       Reserved_39_37  :  3; // Reserved
        uint64_t                count  :  8; // Saturating count PCB nval errors
        uint64_t       Reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INFO_PCB_NVAL_t;

// RXHIARB_ERR_INFO_PCB_BVIO desc:
typedef union {
    struct {
        uint64_t               handle  : 12; // Requester handleof the most recent error.
        uint64_t            rx_src_id  :  4; // Source requester ID for most recent error.This indicates the requester who caused the error. 0x0-0x3=rsvd, =0x4=RxE2E, 0x5-0x7=rsvd, 0x8=RxHP. 0x9=RxCID, 0xa=RxET, 0xb-0xe=rsvd, 0xf=RxDMA Note that only the handle-based requesters of RxDMA, RxHP, and RxET will cause this error
        uint64_t              element  : 16; // Requester element being accessed during most recent error.
        uint64_t               region  :  3; // PCB region being accessed during most recent error 0=PTE, 1=CT, 2-EQ, 3=SWEQ, 4-TrigOp, 5=LE/ME, 6-UnexpHdr, 7=rsvd
        uint64_t                   ni  :  2; // Requester network number duringmost recent error
        uint64_t       Reserved_39_37  :  3; // Reserved
        uint64_t                count  :  8; // Saturating count of bvio errors
        uint64_t       Reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INFO_PCB_BVIO_t;

// RXHIARB_ERR_INFO_PCB_OFLW desc:
typedef union {
    struct {
        uint64_t               handle  : 12; // Requester handle of the most recent error.
        uint64_t            rx_src_id  :  4; // Source requester ID for most recent error.This indicates the requester who caused the error. 0x0-0x3=rsvd, =0x4=RxE2E, 0x5-0x7=rsvd, 0x8=RxHP. 0x9=RxCID, 0xa=RxET, 0xb-0xe=rsvd, 0xf=RxDMA Note that only the handle-based requesters of RxDMA, RxHP, and RxET will cause this error.
        uint64_t              element  : 16; // Requester element being accessed during most recent error.
        uint64_t               region  :  3; // PCB region being accessed during most recent error 0=PTE, 1=CT, 2-EQ, 3=SWEQ, 4-TrigOp, 5=LE/ME, 6-UnexpHdr, 7=rsvd
        uint64_t                   ni  :  2; // Requester network number during most recent error
        uint64_t       Reserved_39_37  :  3; // Reserved
        uint64_t                count  :  8; // Saturating count of overlow errors
        uint64_t       Reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INFO_PCB_OFLW_t;

// RXHIARB_ERR_INFO_PCB_ECC desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the most recent mbe
        uint64_t       mbe_last_index  : 12; // PCB index (addr) of last mbe
        uint64_t       Reserved_23_20  :  4; // Reserved
        uint64_t            mbe_count  :  8; // saturating counter of mbes.
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the most recent sbe
        uint64_t       sbe_last_index  : 12; // PCB index (addr) of last sbe
        uint64_t       Reserved_55_52  :  4; // Reserved
        uint64_t            sbe_count  :  8; // saturating counter of sbes.
    } field;
    uint64_t val;
} RXHIARB_ERR_INFO_PCB_ECC_t;

// RXHIARB_ERR_INJECT_PCB desc:
typedef union {
    struct {
        uint64_t  err_inject_ecc_mask  :  8; // ECC error injection mask, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t    err_inject_ecc_en  :  1; // ECC error injection enable.
        uint64_t  err_inject_ovflw_en  :  1; // Overflow error injection enable.
        uint64_t   err_inject_bvio_en  :  1; // Boundary violation error injection enable
        uint64_t   err_inject_nval_en  :  1; // Non-valid error injec tion enable
        uint64_t       Reserved_63_12  : 52; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INJECT_PCB_t;

// RXHIARB_ERR_INFO_AT_RSP desc:
typedef union {
    struct {
        uint64_t               at_tid  :  7; // AT transaction ID of most recent error (HIARB only uses a 7-bit TID).
        uint64_t           Reserved_7  :  1; // Reserved.
        uint64_t               status  :  4; // AT status field of most recent error.
        uint64_t       Reserved_15_12  :  4; // Reserved.
        uint64_t                count  : 16; // Saturating count of AT errors.
        uint64_t       Reserved_63_32  : 32; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INFO_AT_RSP_t;

// RXHIARB_ERR_INFO_RXDMA_FRAME desc:
typedef union {
    struct {
        uint64_t                  tid  :  8; // Last transaction ID.
        uint64_t               domain  :  4; // Last domain
        uint64_t       Reserved_15_12  :  4; // Reserved.
        uint64_t                 ferr  :  9; // framing error domain bit vector - 1 bit per domain.
        uint64_t       Reserved_31_25  :  7; // Reserved
        uint64_t                count  :  8; // Saturating count of ferrThe increment signal is the 'or' of the 9 domain ferr signals.
        uint64_t       Reserved_63_40  : 24; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INFO_RXDMA_FRAME_t;

// RXHIARB_ERR_INFO_RXDMA_ECC desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the most recent least significant ecc domain mbe
        uint64_t      mbe_last_domain  :  4; // ecc domain of the most recent least significant mbe
        uint64_t                  mbe  :  9; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t       Reserved_23_21  :  3; // Reserved.
        uint64_t            mbe_count  :  8; // saturating counter of mbes. The increment signal is the 'or' of the 9 mbe domain signals.
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the most recent least significant ecc domain sbe
        uint64_t      sbe_last_domain  :  4; // ecc domain of most recent least significant sbe
        uint64_t                  sbe  :  9; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t       Reserved_55_53  :  3; // Reserved
        uint64_t            sbe_count  :  8; // saturating counter of sbes. The increment signal is the 'or' of the 9 domain sbe signals.
    } field;
    uint64_t val;
} RXHIARB_ERR_INFO_RXDMA_ECC_t;

// RXHIARB_ERR_INJECT_RXDMA_0_3 desc:
typedef union {
    struct {
        uint64_t     err_inject_mask0  :  8; // Error injection mask for input queue 0, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t     err_inject_mask1  :  8; // Error injection mask for input queue 1, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t     err_inject_mask2  :  8; // Error injection mask for input queue 2, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t     err_inject_mask3  :  8; // Error injection mask for input queue 3, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t       Reserved_39_32  :  8; // Reserved
        uint64_t       err_inject_en0  :  1; // Error injection enable for input queue 0
        uint64_t       err_inject_en1  :  1; // Error injection enable for input queue 1
        uint64_t       err_inject_en2  :  1; // Error injection enable for input queue 2
        uint64_t       err_inject_en3  :  1; // Error injection enable for input queue 3
        uint64_t       Reserved_63_44  : 20; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INJECT_RXDMA_0_3_t;

// RXHIARB_ERR_INJECT_RXDMA_4_7 desc:
typedef union {
    struct {
        uint64_t     err_inject_mask4  :  8; // Error injection mask for input queue 4, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t     err_inject_mask5  :  8; // Error injection mask for input queue 5, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t     err_inject_mask6  :  8; // Error injection mask for input queue 6, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t     err_inject_mask7  :  8; // Error injection mask for input queue 7, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t       Reserved_39_32  :  8; // Reserved
        uint64_t       err_inject_en4  :  1; // Error injection enable for input queue 4
        uint64_t       err_inject_en5  :  1; // Error injection enable for input queue 5
        uint64_t       err_inject_en6  :  1; // Error injection enable for input queue 6
        uint64_t       err_inject_en7  :  1; // Error injection enable for input queue 7
        uint64_t       Reserved_63_44  : 20; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INJECT_RXDMA_4_7_t;

// RXHIARB_ERR_INJECT_RXDMA_8F desc:
typedef union {
    struct {
        uint64_t     err_inject_mask8  :  8; // ECC error injection mask for input queue 8, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t       err_inject_en8  :  1; // ECC error injection enable for input queue 8
        uint64_t   err_inject_ferr_en  :  1; // Framing error injection enable
        uint64_t       Reserved_63_10  : 54; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INJECT_RXDMA_8F_t;

// RXHIARB_ERR_INFO_RXHP_FRAME desc:
typedef union {
    struct {
        uint64_t                  tid  :  8; // Transaction ID of most recent error.
        uint64_t                count  :  8; // Saturating count of ferr.
        uint64_t       Reserved_63_16  : 48; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INFO_RXHP_FRAME_t;

// RXHIARB_ERR_INFO_RXHP_ECC desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the most recent mbe
        uint64_t            mbe_count  :  8; // saturating counter of mbes.
        uint64_t       Reserved_31_16  : 16; // Reserved.
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the most recent sbe
        uint64_t            sbe_count  :  8; // saturating counter of sbes.
        uint64_t       Reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INFO_RXHP_ECC_t;

// RXHIARB_ERR_INJECT_RXHP desc:
typedef union {
    struct {
        uint64_t  err_inject_ecc_mask  :  8; // ECC error injection mask, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t    err_inject_ecc_en  :  1; // ECC error injection enable
        uint64_t   err_inject_ferr_en  :  1; // Framing error injection enable
        uint64_t       Reserved_63_10  : 54; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INJECT_RXHP_t;

// RXHIARB_ERR_INFO_RXET_FRAME desc:
typedef union {
    struct {
        uint64_t                  tid  :  8; // Transaction ID of most recent error.
        uint64_t                count  :  8; // Saturating count of ferr.
        uint64_t       Reserved_63_16  : 48; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INFO_RXET_FRAME_t;

// RXHIARB_ERR_INFO_RXET_ECC desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the most recent mbe
        uint64_t            mbe_count  :  8; // saturating counter of mbes.
        uint64_t       Reserved_31_16  : 16; // Reserved.
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the most recent sbe
        uint64_t            sbe_count  :  8; // saturating counter of sbes.
        uint64_t       Reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INFO_RXET_ECC_t;

// RXHIARB_ERR_INJECT_RXET desc:
typedef union {
    struct {
        uint64_t  err_inject_ecc_mask  :  8; // ECC error injection mask, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t    err_inject_ecc_en  :  1; // ECC error injection enable
        uint64_t   err_inject_ferr_en  :  1; // Framing error injection enable
        uint64_t       Reserved_63_10  : 54; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INJECT_RXET_t;

// RXHIARB_ERR_INFO_RXE2E_ECC desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the most recent mbe
        uint64_t            mbe_count  :  8; // saturating counter of mbes.
        uint64_t       Reserved_31_16  : 16; // Reserved.
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the most recent sbe
        uint64_t            sbe_count  :  8; // saturating counter of sbes.
        uint64_t       Reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INFO_RXE2E_ECC_t;

// RXHIARB_ERR_INJECT_RXE2E desc:
typedef union {
    struct {
        uint64_t      err_inject_mask  :  8; // Error injection mask, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t        err_inject_en  :  1; // Error injection enable
        uint64_t        Reserved_63_9  : 55; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INJECT_RXE2E_t;

// RXHIARB_ERR_INFO_RXCID_ECC desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the most recent mbe
        uint64_t            mbe_count  :  8; // saturating counter of mbes.
        uint64_t       Reserved_31_16  : 16; // Reserved.
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the most recent sbe
        uint64_t            sbe_count  :  8; // saturating counter of sbes.
        uint64_t       Reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INFO_RXCID_ECC_t;

// RXHIARB_ERR_INJECT_RXCID desc:
typedef union {
    struct {
        uint64_t      err_inject_mask  :  8; // Error injection mask, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t        err_inject_en  :  1; // Error injection enable
        uint64_t        Reserved_63_9  : 55; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INJECT_RXCID_t;

// RXHIARB_ERR_INFO_SLRSP_ECC desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  6; // syndrome of the most recent mbe
        uint64_t         Reserved_7_6  :  2; // Reserved.
        uint64_t            mbe_count  :  8; // saturating counter of mbes.
        uint64_t       Reserved_31_16  : 16; // Reserved.
        uint64_t    sbe_last_syndrome  :  6; // syndrome of the most recent sbe
        uint64_t       Reserved_39_38  :  2; // Reserved.
        uint64_t            sbe_count  :  8; // saturating counter of sbes.
        uint64_t       Reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INFO_SLRSP_ECC_t;

// RXHIARB_ERR_INJECT_SLRSP desc:
typedef union {
    struct {
        uint64_t      err_inject_mask  :  6; // Error injection mask, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t         Reserved_7_6  :  2; // Reserved
        uint64_t        err_inject_en  :  1; // Error injection enable
        uint64_t        Reserved_63_9  : 55; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INJECT_SLRSP_t;

// RXHIARB_ERR_INFO_FPFIFO_PERR desc:
typedef union {
    struct {
        uint64_t           perr_count  :  7; // saturating counter of parity errors.
        uint64_t        Reserved_63_7  : 57; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INFO_FPFIFO_PERR_t;

// RXHIARB_ERR_INJECT_FPFIFO desc:
typedef union {
    struct {
        uint64_t        err_inject_en  :  1; // Error injection enable
        uint64_t        Reserved_63_1  : 63; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INJECT_FPFIFO_t;

// RXHIARB_ERR_INFO_PYLD_ECC desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the most recent mbe
        uint64_t       mbe_last_index  :  7; // Payload index (addr) of last mbe
        uint64_t          Reserved_15  :  1; // Reserved
        uint64_t            mbe_count  :  8; // saturating counter of mbes.
        uint64_t       Reserved_31_24  :  8; // Reserved.
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the most recent sbe
        uint64_t       sbe_last_index  :  7; // Payload index (addr) of last sbe
        uint64_t          Reserved_47  :  1; // Reserved
        uint64_t            sbe_count  :  8; // saturating counter of sbes.
        uint64_t       Reserved_63_56  :  8; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INFO_PYLD_ECC_t;

// RXHIARB_ERR_INJECT_PYLD desc:
typedef union {
    struct {
        uint64_t      err_inject_mask  :  8; // Error injection mask, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t        err_inject_en  :  1; // Error injection enable
        uint64_t        Reserved_63_9  : 55; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INJECT_PYLD_t;

// RXHIARB_ERR_INFO_HQUE0_ECC desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  6; // syndrome of the most recent least significant ecc domain mbe
        uint64_t      mbe_last_domain  :  1; // ecc domain of the most recent least significant mbe
        uint64_t                  mbe  :  2; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t        Reserved_15_9  :  7; // Reserved.
        uint64_t            mbe_count  :  8; // saturating counter of mbes. The increment signal is the 'or' of the 2 mbe domain signals.
        uint64_t       Reserved_31_24  :  8; // Reserved.
        uint64_t    sbe_last_syndrome  :  6; // syndrome of the most recent least significant ecc domain sbe
        uint64_t      sbe_last_domain  :  1; // ecc domain of most recent least significant sbe
        uint64_t                  sbe  :  2; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t       Reserved_47_41  :  7; // Reserved
        uint64_t            sbe_count  :  8; // saturating counter of sbes. The increment signal is the 'or' of the 2 domain sbe signals.
        uint64_t       Reserved_63_56  :  8; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INFO_HQUE0_ECC_t;

// RXHIARB_ERR_INFO_HQUE1_ECC desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  6; // syndrome of the most recent least significant ecc domain mbe
        uint64_t      mbe_last_domain  :  1; // ecc domain of the most recent least significant mbe
        uint64_t                  mbe  :  2; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t        Reserved_15_9  :  7; // Reserved.
        uint64_t            mbe_count  :  8; // saturating counter of mbes. The increment signal is the 'or' of the 2 mbe domain signals.
        uint64_t       Reserved_31_24  :  8; // Reserved.
        uint64_t    sbe_last_syndrome  :  6; // syndrome of the most recent least significant ecc domain sbe
        uint64_t      sbe_last_domain  :  1; // ecc domain of most recent least significant sbe
        uint64_t                  sbe  :  2; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t       Reserved_47_41  :  7; // Reserved
        uint64_t            sbe_count  :  8; // saturating counter of sbes. The increment signal is the 'or' of the 2 domain sbe signals.
        uint64_t       Reserved_63_56  :  8; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INFO_HQUE1_ECC_t;

// RXHIARB_ERR_INFO_HQUE2_ECC desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  6; // syndrome of the most recent least significant ecc domain mbe
        uint64_t      mbe_last_domain  :  1; // ecc domain of the most recent least significant mbe
        uint64_t                  mbe  :  2; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t        Reserved_15_9  :  7; // Reserved.
        uint64_t            mbe_count  :  8; // saturating counter of mbes. The increment signal is the 'or' of the 2 mbe domain signals.
        uint64_t       Reserved_31_24  :  8; // Reserved.
        uint64_t    sbe_last_syndrome  :  6; // syndrome of the most recent least significant ecc domain sbe
        uint64_t      sbe_last_domain  :  1; // ecc domain of most recent least significant sbe
        uint64_t                  sbe  :  2; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t       Reserved_47_41  :  7; // Reserved
        uint64_t            sbe_count  :  8; // saturating counter of sbes. The increment signal is the 'or' of the 2 domain sbe signals.
        uint64_t       Reserved_63_56  :  8; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INFO_HQUE2_ECC_t;

// RXHIARB_ERR_INFO_HQUE3_ECC desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  6; // syndrome of the most recent least significant ecc domain mbe
        uint64_t      mbe_last_domain  :  1; // ecc domain of the most recent least significant mbe
        uint64_t                  mbe  :  2; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t        Reserved_15_9  :  7; // Reserved.
        uint64_t            mbe_count  :  8; // saturating counter of mbes. The increment signal is the 'or' of the 2 mbe domain signals.
        uint64_t       Reserved_31_24  :  8; // Reserved.
        uint64_t    sbe_last_syndrome  :  6; // syndrome of the most recent least significant ecc domain sbe
        uint64_t      sbe_last_domain  :  1; // ecc domain of most recent least significant sbe
        uint64_t                  sbe  :  2; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t       Reserved_47_41  :  7; // Reserved
        uint64_t            sbe_count  :  8; // saturating counter of sbes. The increment signal is the 'or' of the 2 domain sbe signals.
        uint64_t       Reserved_63_56  :  8; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INFO_HQUE3_ECC_t;

// RXHIARB_ERR_INFO_HQUE4_ECC desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  6; // syndrome of the most recent least significant ecc domain mbe
        uint64_t      mbe_last_domain  :  1; // ecc domain of the most recent least significant mbe
        uint64_t                  mbe  :  2; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t        Reserved_15_9  :  7; // Reserved.
        uint64_t            mbe_count  :  8; // saturating counter of mbes. The increment signal is the 'or' of the 2 mbe domain signals.
        uint64_t       Reserved_31_24  :  8; // Reserved.
        uint64_t    sbe_last_syndrome  :  6; // syndrome of the most recent least significant ecc domain sbe
        uint64_t      sbe_last_domain  :  1; // ecc domain of most recent least significant sbe
        uint64_t                  sbe  :  2; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t       Reserved_47_41  :  7; // Reserved
        uint64_t            sbe_count  :  8; // saturating counter of sbes. The increment signal is the 'or' of the 2 domain sbe signals.
        uint64_t       Reserved_63_56  :  8; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INFO_HQUE4_ECC_t;

// RXHIARB_ERR_INFO_HQUE5_ECC desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  6; // syndrome of the most recent least significant ecc domain mbe
        uint64_t      mbe_last_domain  :  1; // ecc domain of the most recent least significant mbe
        uint64_t                  mbe  :  2; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t        Reserved_15_9  :  7; // Reserved.
        uint64_t            mbe_count  :  8; // saturating counter of mbes. The increment signal is the 'or' of the 2 mbe domain signals.
        uint64_t       Reserved_31_24  :  8; // Reserved.
        uint64_t    sbe_last_syndrome  :  6; // syndrome of the most recent least significant ecc domain sbe
        uint64_t      sbe_last_domain  :  1; // ecc domain of most recent least significant sbe
        uint64_t                  sbe  :  2; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t       Reserved_47_41  :  7; // Reserved
        uint64_t            sbe_count  :  8; // saturating counter of sbes. The increment signal is the 'or' of the 2 domain sbe signals.
        uint64_t       Reserved_63_56  :  8; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INFO_HQUE5_ECC_t;

// RXHIARB_ERR_INFO_HQUE6_ECC desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  6; // syndrome of the most recent least significant ecc domain mbe
        uint64_t      mbe_last_domain  :  1; // ecc domain of the most recent least significant mbe
        uint64_t                  mbe  :  2; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t        Reserved_15_9  :  7; // Reserved.
        uint64_t            mbe_count  :  8; // saturating counter of mbes. The increment signal is the 'or' of the 2 mbe domain signals.
        uint64_t       Reserved_31_24  :  8; // Reserved.
        uint64_t    sbe_last_syndrome  :  6; // syndrome of the most recent least significant ecc domain sbe
        uint64_t      sbe_last_domain  :  1; // ecc domain of most recent least significant sbe
        uint64_t                  sbe  :  2; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t       Reserved_47_41  :  7; // Reserved
        uint64_t            sbe_count  :  8; // saturating counter of sbes. The increment signal is the 'or' of the 2 domain sbe signals.
        uint64_t       Reserved_63_56  :  8; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INFO_HQUE6_ECC_t;

// RXHIARB_ERR_INFO_HQUE7_ECC desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  6; // syndrome of the most recent least significant ecc domain mbe
        uint64_t      mbe_last_domain  :  1; // ecc domain of the most recent least significant mbe
        uint64_t                  mbe  :  2; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t        Reserved_15_9  :  7; // Reserved.
        uint64_t            mbe_count  :  8; // saturating counter of mbes. The increment signal is the 'or' of the 2 mbe domain signals.
        uint64_t       Reserved_31_24  :  8; // Reserved.
        uint64_t    sbe_last_syndrome  :  6; // syndrome of the most recent least significant ecc domain sbe
        uint64_t      sbe_last_domain  :  1; // ecc domain of most recent least significant sbe
        uint64_t                  sbe  :  2; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t       Reserved_47_41  :  7; // Reserved
        uint64_t            sbe_count  :  8; // saturating counter of sbes. The increment signal is the 'or' of the 2 domain sbe signals.
        uint64_t       Reserved_63_56  :  8; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INFO_HQUE7_ECC_t;

// RXHIARB_ERR_INFO_HQUE8_ECC desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  6; // syndrome of the most recent least significant ecc domain mbe
        uint64_t      mbe_last_domain  :  1; // ecc domain of the most recent least significant mbe
        uint64_t                  mbe  :  2; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t        Reserved_15_9  :  7; // Reserved.
        uint64_t            mbe_count  :  8; // saturating counter of mbes. The increment signal is the 'or' of the 2 mbe domain signals.
        uint64_t       Reserved_31_24  :  8; // Reserved.
        uint64_t    sbe_last_syndrome  :  6; // syndrome of the most recent least significant ecc domain sbe
        uint64_t      sbe_last_domain  :  1; // ecc domain of most recent least significant sbe
        uint64_t                  sbe  :  2; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t       Reserved_47_41  :  7; // Reserved
        uint64_t            sbe_count  :  8; // saturating counter of sbes. The increment signal is the 'or' of the 2 domain sbe signals.
        uint64_t       Reserved_63_56  :  8; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INFO_HQUE8_ECC_t;

// RXHIARB_ERR_INJECT_HQUE_0_3 desc:
typedef union {
    struct {
        uint64_t err_inject_hid0_mask  :  5; // Error injection mask for holding queue 0's HID queue, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t err_inject_laddr0_mask  :  6; // Error injection mask for holding queue 0's LADDR queue, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t err_inject_hid1_mask  :  5; // Error injection mask for holding queue 1's HID queue, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t err_inject_laddr1_mask  :  6; // Error injection mask for holding queue 1's LADDR queue, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t err_inject_hid2_mask  :  5; // Error injection mask for holding queue 2's HID queue, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t err_inject_laddr2_mask  :  6; // Error injection mask for holding queue 2's LADDR queue, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t err_inject_hid3_mask  :  5; // Error injection mask for holding queue 3's HID queue, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t err_inject_laddr3_mask  :  6; // Error injection mask for holding queue 3's LADDR queue, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t       Reserved_47_44  :  4; // Reserved
        uint64_t   err_inject_hid0_en  :  1; // Error injection enable for holding queue 0's HID queue.
        uint64_t err_inject_laddr0_en  :  1; // Error injection enable for holding queue 0's LADDR queue.
        uint64_t   err_inject_hid1_en  :  1; // Error injection enable for holding queue 1's HID queue.
        uint64_t err_inject_laddr1_en  :  1; // Error injection enable for holding queue 1's LADDR queue.
        uint64_t   err_inject_hid2_en  :  1; // Error injection enable for holding queue 2's HID queue.
        uint64_t err_inject_laddr2_en  :  1; // Error injection enable for holding queue 2's LADDR queue.
        uint64_t   err_inject_hid3_en  :  1; // Error injection enable for holding queue 3's HID queue.
        uint64_t err_inject_laddr3_en  :  1; // Error injection enable for holding queue 3's LADDR queue.
        uint64_t       Reserved_63_56  :  8; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INJECT_HQUE_0_3_t;

// RXHIARB_ERR_INJECT_HQUE_4_7 desc:
typedef union {
    struct {
        uint64_t err_inject_hid4_mask  :  5; // Error injection mask for holding queue 4's HID queue, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t err_inject_laddr4_mask  :  6; // Error injection mask for holding queue 4's LADDR queue, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t err_inject_hid5_mask  :  5; // Error injection mask for holding queue 5's HID queue, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t err_inject_laddr5_mask  :  6; // Error injection mask for holding queue 5's LADDR queue, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t err_inject_hid6_mask  :  5; // Error injection mask for holding queue 6's HID queue, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t err_inject_laddr6_mask  :  6; // Error injection mask for holding queue 6's LADDR queue, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t err_inject_hid7_mask  :  5; // Error injection mask for holding queue 7's HID queue, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t err_inject_laddr7_mask  :  6; // Error injection mask for holding queue 7's LADDR queue, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t       Reserved_47_44  :  4; // Reserved
        uint64_t   err_inject_hid4_en  :  1; // Error injection enable for holding queue 4's HID queue.
        uint64_t err_inject_laddr4_en  :  1; // Error injection enable for holding queue 4's LADDR queue.
        uint64_t   err_inject_hid5_en  :  1; // Error injection enable for holding queue 5's HID queue.
        uint64_t err_inject_laddr5_en  :  1; // Error injection enable for holding queue 5's LADDR queue.
        uint64_t   err_inject_hid6_en  :  1; // Error injection enable for holding queue 6's HID queue.
        uint64_t err_inject_laddr6_en  :  1; // Error injection enable for holding queue 6's LADDR queue.
        uint64_t   err_inject_hid7_en  :  1; // Error injection enable for holding queue 7's HID queue.
        uint64_t err_inject_laddr7_en  :  1; // Error injection enable for holding queue 7's LADDR queue.
        uint64_t       Reserved_63_56  :  8; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INJECT_HQUE_4_7_t;

// RXHIARB_ERR_INJECT_HQUE_8 desc:
typedef union {
    struct {
        uint64_t err_inject_hid8_mask  :  5; // Error injection mask for holding queue 8's HID queue, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t err_inject_laddr8_mask  :  6; // Error injection mask for holding queue 8's LADDR queue, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t       Reserved_47_11  : 37; // Reserved
        uint64_t   err_inject_hid8_en  :  1; // Error injection enable for holding queue 8's HID queue.
        uint64_t err_inject_laddr8_en  :  1; // Error injection enable for holding queue 8's LADDR queue.
        uint64_t       Reserved_63_50  : 14; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INJECT_HQUE_8_t;

// RXHIARB_ERR_INFO_HQUE_PVECT desc:
typedef union {
    struct {
        uint64_t     verr_last_domain  :  4; // domain of the most recent least significant vector parity error
        uint64_t         Reserved_7_4  :  4; // Reserved.
        uint64_t                 verr  :  9; // per domain single bit set whenever a parity error is detected for that domain. This helps find more significant errors when multiple domains have parity errors in the same clock and only the least significant domain is recorded.
        uint64_t       Reserved_63_17  : 47; // Reserved.
    } field;
    uint64_t val;
} RXHIARB_ERR_INFO_HQUE_PVECT_t;

// RXHIARB_ERR_INJECT_HQUE_PVECT desc:
typedef union {
    struct {
        uint64_t  err_inject_vect0_en  :  1; // Error injection enableholding queue 0's state vector
        uint64_t  err_inject_vect1_en  :  1; // Error injection enableholding queue 1's state vector
        uint64_t  err_inject_vect2_en  :  1; // Error injection enableholding queue 2's state vector
        uint64_t  err_inject_vect3_en  :  1; // Error injection enableholding queue 3's state vector
        uint64_t  err_inject_vect4_en  :  1; // Error injection enableholding queue 4's state vector
        uint64_t  err_inject_vect5_en  :  1; // Error injection enableholding queue 5's state vector
        uint64_t  err_inject_vect6_en  :  1; // Error injection enableholding queue 6's state vector
        uint64_t  err_inject_vect7_en  :  1; // Error injection enableholding queue 7's state vector
        uint64_t  err_inject_vect8_en  :  1; // Error injection enableholding queue 8's state vector
        uint64_t        Reserved_63_9  : 55; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INJECT_HQUE_PVECT_t;

// RXHIARB_ERR_INFO_MTLB_ECC desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  7; // syndrome of the most recent least significant ecc domain mbe
        uint64_t           Reserved_7  :  1; // Reserved.
        uint64_t       mbe_last_index  :  7; // index of the most recent least significant ecc domain mbe
        uint64_t          Reserved_15  :  1; // Reserved.
        uint64_t      mbe_last_domain  :  2; // ecc domain of the most recent least significant mbe
        uint64_t                  mbe  :  4; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t       Reserved_23_22  :  2; // Reserved.
        uint64_t            mbe_count  :  8; // saturating counter of mbes. The increment signal is the 'or' of the 4 mbe domain signals.
        uint64_t    sbe_last_syndrome  :  7; // syndrome of the most recent least significant ecc domain sbe
        uint64_t          Reserved_39  :  1; // Reserved.
        uint64_t       sbe_last_index  :  7; // index of the most recent least significant ecc domain sbe
        uint64_t          Reserved_47  :  1; // Reserved.
        uint64_t      sbe_last_domain  :  2; // ecc domain of most recent least significant sbe
        uint64_t                  sbe  :  4; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t       Reserved_55_54  :  2; // Reserved
        uint64_t            sbe_count  :  8; // saturating counter of sbes. The increment signal is the 'or' of the 2 domain sbe signals.
    } field;
    uint64_t val;
} RXHIARB_ERR_INFO_MTLB_ECC_t;

// RXHIARB_ERR_INFO_MTLB_VECTS desc:
typedef union {
    struct {
        uint64_t                 verr  :  5; // per domain single bit set whenever a parity error is detected for that domain. This helps find more significant errors when multiple domains have parity errors in the same clock and only the least significant domain is recorded.
        uint64_t        Reserved_63_5  : 59; // Reserved.
    } field;
    uint64_t val;
} RXHIARB_ERR_INFO_MTLB_VECTS_t;

// RXHIARB_ERR_INJECT_MTLB desc:
typedef union {
    struct {
        uint64_t  err_inject_hpa_mask  :  7; // Error injection mask for HPA, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t           Reserved_7  :  1; // Reserved
        uint64_t err_inject_cnt1_mask  :  5; // Error injection mask for counter port 1, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t       Reserved_15_13  :  3; // Reserved
        uint64_t err_inject_cnt0_mask  :  5; // Error injection mask for counter port 0, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t       Reserved_31_21  : 11; // Reserved
        uint64_t    err_inject_hpa_en  :  1; // Error injection enable for HPA.
        uint64_t   err_inject_cnt1_en  :  1; // Error injection enable for counter port 1.
        uint64_t   err_inject_cnt0_en  :  1; // Error injection enable for counter port 0.
        uint64_t    err_inject_tag_en  :  1; // Error injection enable for tag memory.
        uint64_t   err_inject_fbad_en  :  1; // Error injection for fbad (bad fill) vector
        uint64_t   err_inject_cbad_en  :  1; // Error injection for cbad (corrupt) vector
        uint64_t   err_inject_zero_en  :  1; // Error injection for zero vector
        uint64_t   err_inject_pres_en  :  1; // Error injection for present vector
        uint64_t    err_inject_val_en  :  1; // Error injection for valid vector
        uint64_t       Reserved_63_41  : 23; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INJECT_MTLB_t;

// RXHIARB_ERR_INFO_ATFIFO_ECC desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the most recent least significant ecc domain mbe
        uint64_t            mbe_count  :  8; // saturating counter of mbes.
        uint64_t       Reserved_31_16  : 16; // Reserved.
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the most recent least significant ecc domain sbe
        uint64_t            sbe_count  :  8; // saturating counter of sbes
        uint64_t       Reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INFO_ATFIFO_ECC_t;

// RXHIARB_ERR_INJECT_ATFIFO desc:
typedef union {
    struct {
        uint64_t      err_inject_mask  :  8; // Error injection mask, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t        err_inject_en  :  1; // Error injection enable
        uint64_t        Reserved_63_9  : 55; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INJECT_ATFIFO_t;

// RXHIARB_ERR_INFO_HIFIFO_ECC desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the most recent least significant ecc domain mbe
        uint64_t            mbe_count  :  8; // saturating counter of mbes.
        uint64_t       Reserved_31_16  : 16; // Reserved.
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the most recent least significant ecc domain sbe
        uint64_t            sbe_count  :  8; // saturating counter of sbes
        uint64_t       Reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INFO_HIFIFO_ECC_t;

// RXHIARB_ERR_INJECT_HIFIFO desc:
typedef union {
    struct {
        uint64_t      err_inject_mask  :  8; // Error injection mask, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t        err_inject_en  :  1; // Error injection enable
        uint64_t        Reserved_63_9  : 55; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INJECT_HIFIFO_t;

// RXHIARB_ERR_INFO_HIFIS_ECC desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the most recent least significant ecc domain mbe
        uint64_t            mbe_count  :  8; // saturating counter of mbes.
        uint64_t       Reserved_31_16  : 16; // Reserved.
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the most recent least significant ecc domain sbe
        uint64_t            sbe_count  :  8; // saturating counter of sbes
        uint64_t       Reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INFO_HIFIS_ECC_t;

// RXHIARB_ERR_INFO_HIFIS_FRAME desc:
typedef union {
    struct {
        uint64_t                count  :  8; // Saturating count of framing errors
        uint64_t        Reserved_63_8  : 56; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INFO_HIFIS_FRAME_t;

// RXHIARB_ERR_INJECT_HIFIS desc:
typedef union {
    struct {
        uint64_t      err_inject_mask  :  8; // ECC error injection mask, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t        err_inject_en  :  1; // ECC error injection enable
        uint64_t  err_inject_frame_en  :  1; // Framing error injection enable
        uint64_t       Reserved_63_10  : 54; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INJECT_HIFIS_t;

// RXHIARB_ERR_INFO_NACK_ECC desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  6; // syndrome of the most recent least significant ecc domain mbe
        uint64_t         Reserved_7_6  :  2; // Reserved.
        uint64_t            mbe_count  :  8; // saturating counter of mbes.
        uint64_t       Reserved_31_16  : 16; // Reserved.
        uint64_t    sbe_last_syndrome  :  6; // syndrome of the most recent least significant ecc domain sbe
        uint64_t       Reserved_39_38  :  2; // Reserved.
        uint64_t            sbe_count  :  8; // saturating counter of sbes
        uint64_t       Reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INFO_NACK_ECC_t;

// RXHIARB_ERR_INJECT_NACK desc:
typedef union {
    struct {
        uint64_t      err_inject_mask  :  6; // Error injection mask, flipping the corresponding bit(s) in the ECC check bits.
        uint64_t         Reserved_7_6  :  2; // Reserved
        uint64_t        err_inject_en  :  1; // Error injection enable
        uint64_t        Reserved_63_9  : 55; // Reserved
    } field;
    uint64_t val;
} RXHIARB_ERR_INJECT_NACK_t;

// RXHIARB_DBG_PYLD_HDR0 desc:
typedef union {
    struct {
        uint64_t           payld_hval  :  1; // Head Valid
        uint64_t           payld_tval  :  1; // Tail Valid, if tali valid is not set, it indicates that this request is a two flit transaction
        uint64_t         payld_opcode  :  3; // Opcode
        uint64_t            payld_pid  : 12; // Process ID
        uint64_t          payld_chint  :  2; // Cache Hints
        uint64_t           payld_tmod  :  8; // Transaction modifiers
        uint64_t            payld_len  :  7; // Byte Length
        uint64_t            payld_tid  : 12; // Transaction ID
        uint64_t             payld_tc  :  4; // Traffic Class
        uint64_t            rx_src_id  :  4; // Client ID, 0xf-RxDMA, 0x6=RxCID, 0x8-RxHP, 0xa=RxET, 0x4=RxE2E, and all other values rsvd
        uint64_t       Reserved_63_54  : 10; // Reserved
    } field;
    uint64_t val;
} RXHIARB_DBG_PYLD_HDR0_t;

// RXHIARB_DBG_PYLD_HDR1_CREG desc:
typedef union {
    struct {
        uint64_t           payld_asop  :  5; // Atomic operation
        uint64_t            payld_adt  :  5; // Atomic
        uint64_t            payld_art  :  1; // Atomic
        uint64_t       Reserved_15_11  :  5; // Reserved
        uint64_t           payld_hecc  :  8; // Header ECC
        uint64_t       Reserved_31_24  :  8; // Reserved
        uint64_t          payld_dval0  :  1; // Data valid for first flit [255:0]
        uint64_t          payld_dval1  :  1; // Data valid for second flit [511:256]
        uint64_t       Reserved_63_34  : 30; // Reserved
    } field;
    uint64_t val;
} RXHIARB_DBG_PYLD_HDR1_CREG_t;

// RXHIARB_DBG_PYLD_DAT0_CREG desc:
typedef union {
    struct {
        uint64_t          payld_data0  : 64; // Write Data [63:0]
    } field;
    uint64_t val;
} RXHIARB_DBG_PYLD_DAT0_CREG_t;

// RXHIARB_DBG_PYLD_DAT1_CREG desc:
typedef union {
    struct {
        uint64_t          payld_data1  : 64; // Write Data [127:64]
    } field;
    uint64_t val;
} RXHIARB_DBG_PYLD_DAT1_CREG_t;

// RXHIARB_DBG_PYLD_DAT2_CREG desc:
typedef union {
    struct {
        uint64_t          payld_data2  : 64; // Write Data [191:128]
    } field;
    uint64_t val;
} RXHIARB_DBG_PYLD_DAT2_CREG_t;

// RXHIARB_DBG_PYLD_DAT3_CREG desc:
typedef union {
    struct {
        uint64_t          payld_data3  : 64; // Write Data [255:192]
    } field;
    uint64_t val;
} RXHIARB_DBG_PYLD_DAT3_CREG_t;

// RXHIARB_DBG_PYLD_DAT4_CREG desc:
typedef union {
    struct {
        uint64_t          payld_data4  : 64; // Write Data [319:256]
    } field;
    uint64_t val;
} RXHIARB_DBG_PYLD_DAT4_CREG_t;

// RXHIARB_DBG_PYLD_DAT5_CREG desc:
typedef union {
    struct {
        uint64_t          payld_data5  : 64; // Write Data [383:320]
    } field;
    uint64_t val;
} RXHIARB_DBG_PYLD_DAT5_CREG_t;

// RXHIARB_DBG_PYLD_DAT6_CREG desc:
typedef union {
    struct {
        uint64_t          payld_data6  : 64; // Write Data [447:384]
    } field;
    uint64_t val;
} RXHIARB_DBG_PYLD_DAT6_CREG_t;

// RXHIARB_DBG_PYLD_DAT7_CREG desc:
typedef union {
    struct {
        uint64_t          payld_data7  : 64; // Write Data [511:448]
    } field;
    uint64_t val;
} RXHIARB_DBG_PYLD_DAT7_CREG_t;

// RXHIARB_DBG_PYLD_DAT8_CREG desc:
typedef union {
    struct {
        uint64_t          payld_decc0  : 32; // Write Data ECC 0 where [7:0] corresponds to payld_data0 CSR, [15:8] corresponds to payld_data1 CSR, [23:16] to payld_data2, and [31:24] to payld_data3
        uint64_t          payld_decc1  : 32; // Write Data ECC 1 where [7:0] corresponds to payld_data4 CSR, [15:8] corresponds to payld_data5 CSR, [23:16] to payld_data6, and [31:24] to payld_data7
    } field;
    uint64_t val;
} RXHIARB_DBG_PYLD_DAT8_CREG_t;

// RXHIARB_DBG_MTLB_TAG desc:
typedef union {
    struct {
        uint64_t                   va  : 45; // Virtual Address [56:12]
        uint64_t                  pid  : 12; // Process ID
        uint64_t               access  :  2; // Access, 00=Read, 01=Write, 10=ZLR, 11=Atomic
        uint64_t           priv_level  :  1; // Privilege Level (user/supervisor)
        uint64_t             par_even  :  1; // Striped Parity, even bits
        uint64_t              par_odd  :  1; // Striped Parity, odd bits
        uint64_t       Reserved_63_62  :  2; // Reserved
    } field;
    uint64_t val;
} RXHIARB_DBG_MTLB_TAG_t;

// RXHIARB_DBG_MTLB_DATA desc:
typedef union {
    struct {
        uint64_t                   pa  : 40; // Physical Address [51:12]
        uint64_t          Reserved_40  :  1; // Reserved
        uint64_t                  ecc  :  7; // ECC bits, covers [39:0]
        uint64_t       Reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} RXHIARB_DBG_MTLB_DATA_t;

// RXHIARB_DBG_MTLB_VVECT_LOW desc:
typedef union {
    struct {
        uint64_t             vect_low  : 64; // Lower half of state vector
    } field;
    uint64_t val;
} RXHIARB_DBG_MTLB_VVECT_LOW_t;

// RXHIARB_DBG_MTLB_VVECT_HIGH desc:
typedef union {
    struct {
        uint64_t            vect_high  : 64; // Upper half of state vector
    } field;
    uint64_t val;
} RXHIARB_DBG_MTLB_VVECT_HIGH_t;

// RXHIARB_DBG_MTLB_PVECT_LOW desc:
typedef union {
    struct {
        uint64_t             vect_low  : 64; // Lower half of state vector
    } field;
    uint64_t val;
} RXHIARB_DBG_MTLB_PVECT_LOW_t;

// RXHIARB_DBG_MTLB_PVECT_HIGH desc:
typedef union {
    struct {
        uint64_t            vect_high  : 64; // Upper half of state vector
    } field;
    uint64_t val;
} RXHIARB_DBG_MTLB_PVECT_HIGH_t;

// RXHIARB_DBG_MTLB_ZVECT_LOW desc:
typedef union {
    struct {
        uint64_t             vect_low  : 64; // Lower half of state vector
    } field;
    uint64_t val;
} RXHIARB_DBG_MTLB_ZVECT_LOW_t;

// RXHIARB_DBG_MTLB_ZVECT_HIGH desc:
typedef union {
    struct {
        uint64_t            vect_high  : 64; // Upper half of state vector
    } field;
    uint64_t val;
} RXHIARB_DBG_MTLB_ZVECT_HIGH_t;

// RXHIARB_DBG_MTLB_CVECT_LOW desc:
typedef union {
    struct {
        uint64_t             vect_low  : 64; // Lower half of state vector
    } field;
    uint64_t val;
} RXHIARB_DBG_MTLB_CVECT_LOW_t;

// RXHIARB_DBG_MTLB_CVECT_HIGH desc:
typedef union {
    struct {
        uint64_t            vect_high  : 64; // Upper half of state vector
    } field;
    uint64_t val;
} RXHIARB_DBG_MTLB_CVECT_HIGH_t;

// RXHIARB_DBG_MTLB_FVECT_LOW desc:
typedef union {
    struct {
        uint64_t             vect_low  : 64; // Lower half of state vector
    } field;
    uint64_t val;
} RXHIARB_DBG_MTLB_FVECT_LOW_t;

// RXHIARB_DBG_MTLB_FVECT_HIGH desc:
typedef union {
    struct {
        uint64_t            vect_high  : 64; // Upper half of state vector
    } field;
    uint64_t val;
} RXHIARB_DBG_MTLB_FVECT_HIGH_t;

// RXHIARB_DBG_MTLB_CNT desc:
typedef union {
    struct {
        uint64_t                count  :  8; // Count value
        uint64_t                  ecc  :  5; // ECC bits, covers [7:0]
        uint64_t       Reserved_63_13  : 51; // Reserved
    } field;
    uint64_t val;
} RXHIARB_DBG_MTLB_CNT_t;

// RXHIARB_DBG_MTLB_FLSH desc:
typedef union {
    struct {
        uint64_t           flush_busy  :  1; // Flush busy - waiting on GO point
        uint64_t             ref_busy  :  1; // Refetch busy - HPAs in processes of being refetched
        uint64_t             nop_busy  :  1; // NOP busy - NOP sent and waiting on its ack
        uint64_t        Reserved_63_3  : 61; // Reserved
    } field;
    uint64_t val;
} RXHIARB_DBG_MTLB_FLSH_t;

// RXHIARB_DBG_MTLB_RVECT_LOW desc:
typedef union {
    struct {
        uint64_t             vect_low  : 64; // Lower half of state vector
    } field;
    uint64_t val;
} RXHIARB_DBG_MTLB_RVECT_LOW_t;

// RXHIARB_DBG_MTLB_RVECT_HIGH desc:
typedef union {
    struct {
        uint64_t            vect_high  : 64; // Upper half of state vector
    } field;
    uint64_t val;
} RXHIARB_DBG_MTLB_RVECT_HIGH_t;

// RXHIARB_DBG_HQUE0 desc:
typedef union {
    struct {
        uint64_t                  hid  :  8; // HIARB ID, corresponds to payload address
        uint64_t              hid_ecc  :  5; // HIARB ID ECC, covers [7:0]
        uint64_t       Reserved_15_13  :  3; // Reserved
        uint64_t                laddr  : 12; // Lower Address [11:0] (both virtual and physical)
        uint64_t            tlb_index  :  7; // Mini-TLB index, corresponds to address of Mini-TLB
        uint64_t  laddr_tlb_index_ecc  :  6; // Combined ECC for laddr and tlb_index, covers [34:16]
        uint64_t             pres_out  :  1; // Present out indicator
        uint64_t       Reserved_63_42  : 22; // Reserved
    } field;
    uint64_t val;
} RXHIARB_DBG_HQUE0_t;

// RXHIARB_DBG_HQUE1 desc:
typedef union {
    struct {
        uint64_t                  hid  :  8; // HIARB ID, corresponds to payload address
        uint64_t              hid_ecc  :  5; // HIARB ID ECC, covers [7:0]
        uint64_t       Reserved_15_13  :  3; // Reserved
        uint64_t                laddr  : 12; // Lower Address [11:0] (both virtual and physical)
        uint64_t            tlb_index  :  7; // Mini-TLB index, corresponds to address of Mini-TLB
        uint64_t  laddr_tlb_index_ecc  :  6; // Combined ECC for laddr and tlb_index, covers [34:16]
        uint64_t             pres_out  :  1; // Present out indicator
        uint64_t       Reserved_63_42  : 22; // Reserved
    } field;
    uint64_t val;
} RXHIARB_DBG_HQUE1_t;

// RXHIARB_DBG_HQUE2 desc:
typedef union {
    struct {
        uint64_t                  hid  :  8; // HIARB ID, corresponds to payload address
        uint64_t              hid_ecc  :  5; // HIARB ID ECC, covers [7:0]
        uint64_t       Reserved_15_13  :  3; // Reserved
        uint64_t                laddr  : 12; // Lower Address [11:0] (both virtual and physical)
        uint64_t            tlb_index  :  7; // Mini-TLB index, corresponds to address of Mini-TLB
        uint64_t  laddr_tlb_index_ecc  :  6; // Combined ECC for laddr and tlb_index, covers [34:16]
        uint64_t             pres_out  :  1; // Present out indicator
        uint64_t       Reserved_63_42  : 22; // Reserved
    } field;
    uint64_t val;
} RXHIARB_DBG_HQUE2_t;

// RXHIARB_DBG_HQUE3 desc:
typedef union {
    struct {
        uint64_t                  hid  :  8; // HIARB ID, corresponds to payload address
        uint64_t              hid_ecc  :  5; // HIARB ID ECC, covers [7:0]
        uint64_t       Reserved_15_13  :  3; // Reserved
        uint64_t                laddr  : 12; // Lower Address [11:0] (both virtual and physical)
        uint64_t            tlb_index  :  7; // Mini-TLB index, corresponds to address of Mini-TLB
        uint64_t  laddr_tlb_index_ecc  :  6; // Combined ECC for laddr and tlb_index, covers [34:16]
        uint64_t             pres_out  :  1; // Present out indicator
        uint64_t       Reserved_63_42  : 22; // Reserved
    } field;
    uint64_t val;
} RXHIARB_DBG_HQUE3_t;

// RXHIARB_DBG_HQUE4 desc:
typedef union {
    struct {
        uint64_t                  hid  :  8; // HIARB ID, corresponds to payload address
        uint64_t              hid_ecc  :  5; // HIARB ID ECC, covers [7:0]
        uint64_t       Reserved_15_13  :  3; // Reserved
        uint64_t                laddr  : 12; // Lower Address [11:0] (both virtual and physical)
        uint64_t            tlb_index  :  7; // Mini-TLB index, corresponds to address of Mini-TLB
        uint64_t  laddr_tlb_index_ecc  :  6; // Combined ECC for laddr and tlb_index, covers [34:16]
        uint64_t             pres_out  :  1; // Present out indicator
        uint64_t       Reserved_63_42  : 22; // Reserved
    } field;
    uint64_t val;
} RXHIARB_DBG_HQUE4_t;

// RXHIARB_DBG_HQUE5 desc:
typedef union {
    struct {
        uint64_t                  hid  :  8; // HIARB ID, corresponds to payload address
        uint64_t              hid_ecc  :  5; // HIARB ID ECC, covers [7:0]
        uint64_t       Reserved_15_13  :  3; // Reserved
        uint64_t                laddr  : 12; // Lower Address [11:0] (both virtual and physical)
        uint64_t            tlb_index  :  7; // Mini-TLB index, corresponds to address of Mini-TLB
        uint64_t  laddr_tlb_index_ecc  :  6; // Combined ECC for laddr and tlb_index, covers [34:16]
        uint64_t             pres_out  :  1; // Present out indicator
        uint64_t       Reserved_63_42  : 22; // Reserved
    } field;
    uint64_t val;
} RXHIARB_DBG_HQUE5_t;

// RXHIARB_DBG_HQUE6 desc:
typedef union {
    struct {
        uint64_t                  hid  :  8; // HIARB ID, corresponds to payload address
        uint64_t              hid_ecc  :  5; // HIARB ID ECC, covers [7:0]
        uint64_t       Reserved_15_13  :  3; // Reserved
        uint64_t                laddr  : 12; // Lower Address [11:0] (both virtual and physical)
        uint64_t            tlb_index  :  7; // Mini-TLB index, corresponds to address of Mini-TLB
        uint64_t  laddr_tlb_index_ecc  :  6; // Combined ECC for laddr and tlb_index, covers [34:16]
        uint64_t             pres_out  :  1; // Present out indicator
        uint64_t       Reserved_63_42  : 22; // Reserved
    } field;
    uint64_t val;
} RXHIARB_DBG_HQUE6_t;

// RXHIARB_DBG_HQUE7 desc:
typedef union {
    struct {
        uint64_t                  hid  :  8; // HIARB ID, corresponds to payload address
        uint64_t              hid_ecc  :  5; // HIARB ID ECC, covers [7:0]
        uint64_t       Reserved_15_13  :  3; // Reserved
        uint64_t                laddr  : 12; // Lower Address [11:0] (both virtual and physical)
        uint64_t            tlb_index  :  7; // Mini-TLB index, corresponds to address of Mini-TLB
        uint64_t  laddr_tlb_index_ecc  :  6; // Combined ECC for laddr and tlb_index, covers [34:16]
        uint64_t             pres_out  :  1; // Present out indicator
        uint64_t       Reserved_63_42  : 22; // Reserved
    } field;
    uint64_t val;
} RXHIARB_DBG_HQUE7_t;

// RXHIARB_DBG_HQUE8 desc:
typedef union {
    struct {
        uint64_t                  hid  :  8; // HIARB ID, corresponds to payload address
        uint64_t              hid_ecc  :  5; // HIARB ID ECC, covers [7:0]
        uint64_t       Reserved_15_13  :  3; // Reserved
        uint64_t                laddr  : 12; // Lower Address [11:0] (both virtual and physical)
        uint64_t            tlb_index  :  7; // Mini-TLB index, corresponds to address of Mini-TLB
        uint64_t  laddr_tlb_index_ecc  :  6; // Combined ECC for laddr and tlb_index, covers [34:16]
        uint64_t             pres_out  :  1; // Present out indicator
        uint64_t       Reserved_63_42  : 22; // Reserved
    } field;
    uint64_t val;
} RXHIARB_DBG_HQUE8_t;

// RXHIARB_DBG_HQUE0_PVECT_LOW desc:
typedef union {
    struct {
        uint64_t             vect_low  : 64; // Lower half of HQUE local present state vector
    } field;
    uint64_t val;
} RXHIARB_DBG_HQUE0_PVECT_LOW_t;

// RXHIARB_DBG_HQUE0_PVECT_HIGH desc:
typedef union {
    struct {
        uint64_t              vect_hi  : 64; // Upper half of HQUE local present state vector
    } field;
    uint64_t val;
} RXHIARB_DBG_HQUE0_PVECT_HIGH_t;

// RXHIARB_DBG_HQUE1_PVECT_LOW desc:
typedef union {
    struct {
        uint64_t             vect_low  : 64; // Lower half of HQUE local present state vector
    } field;
    uint64_t val;
} RXHIARB_DBG_HQUE1_PVECT_LOW_t;

// RXHIARB_DBG_HQUE1_PVECT_HIGH desc:
typedef union {
    struct {
        uint64_t              vect_hi  : 64; // Upper half of HQUE local present state vector
    } field;
    uint64_t val;
} RXHIARB_DBG_HQUE1_PVECT_HIGH_t;

// RXHIARB_DBG_HQUE2_PVECT_LOW desc:
typedef union {
    struct {
        uint64_t             vect_low  : 64; // Lower half of HQUE local present state vector
    } field;
    uint64_t val;
} RXHIARB_DBG_HQUE2_PVECT_LOW_t;

// RXHIARB_DBG_HQUE2_PVECT_HIGH desc:
typedef union {
    struct {
        uint64_t              vect_hi  : 64; // Upper half of HQUE local present state vector
    } field;
    uint64_t val;
} RXHIARB_DBG_HQUE2_PVECT_HIGH_t;

// RXHIARB_DBG_HQUE3_PVECT_LOW desc:
typedef union {
    struct {
        uint64_t             vect_low  : 64; // Lower half of HQUE local present state vector
    } field;
    uint64_t val;
} RXHIARB_DBG_HQUE3_PVECT_LOW_t;

// RXHIARB_DBG_HQUE3_PVECT_HIGH desc:
typedef union {
    struct {
        uint64_t              vect_hi  : 64; // Upper half of HQUE local present state vector
    } field;
    uint64_t val;
} RXHIARB_DBG_HQUE3_PVECT_HIGH_t;

// RXHIARB_DBG_HQUE4_PVECT_LOW desc:
typedef union {
    struct {
        uint64_t             vect_low  : 64; // Lower half of HQUE local present state vector
    } field;
    uint64_t val;
} RXHIARB_DBG_HQUE4_PVECT_LOW_t;

// RXHIARB_DBG_HQUE4_PVECT_HIGH desc:
typedef union {
    struct {
        uint64_t              vect_hi  : 64; // Upper half of HQUE local present state vector
    } field;
    uint64_t val;
} RXHIARB_DBG_HQUE4_PVECT_HIGH_t;

// RXHIARB_DBG_HQUE5_PVECT_LOW desc:
typedef union {
    struct {
        uint64_t             vect_low  : 64; // Lower half of HQUE local present state vector
    } field;
    uint64_t val;
} RXHIARB_DBG_HQUE5_PVECT_LOW_t;

// RXHIARB_DBG_HQUE5_PVECT_HIGH desc:
typedef union {
    struct {
        uint64_t              vect_hi  : 64; // Upper half of HQUE local present state vector
    } field;
    uint64_t val;
} RXHIARB_DBG_HQUE5_PVECT_HIGH_t;

// RXHIARB_DBG_HQUE6_PVECT_LOW desc:
typedef union {
    struct {
        uint64_t             vect_low  : 64; // Lower half of HQUE local present state vector
    } field;
    uint64_t val;
} RXHIARB_DBG_HQUE6_PVECT_LOW_t;

// RXHIARB_DBG_HQUE6_PVECT_HIGH desc:
typedef union {
    struct {
        uint64_t              vect_hi  : 64; // Upper half of HQUE local present state vector
    } field;
    uint64_t val;
} RXHIARB_DBG_HQUE6_PVECT_HIGH_t;

// RXHIARB_DBG_HQUE7_PVECT_LOW desc:
typedef union {
    struct {
        uint64_t             vect_low  : 64; // Lower half of HQUE local present state vector
    } field;
    uint64_t val;
} RXHIARB_DBG_HQUE7_PVECT_LOW_t;

// RXHIARB_DBG_HQUE7_PVECT_HIGH desc:
typedef union {
    struct {
        uint64_t              vect_hi  : 64; // Upper half of HQUE local present state vector
    } field;
    uint64_t val;
} RXHIARB_DBG_HQUE7_PVECT_HIGH_t;

// RXHIARB_DBG_HQUE8_PVECT_LOW desc:
typedef union {
    struct {
        uint64_t             vect_low  : 64; // Lower half of HQUE local present state vector
    } field;
    uint64_t val;
} RXHIARB_DBG_HQUE8_PVECT_LOW_t;

// RXHIARB_DBG_HQUE8_PVECT_HIGH desc:
typedef union {
    struct {
        uint64_t              vect_hi  : 64; // Upper half of HQUE local present state vector
    } field;
    uint64_t val;
} RXHIARB_DBG_HQUE8_PVECT_HIGH_t;

// RXHIARB_DBG_ARB_IN_STATE desc:
typedef union {
    struct {
        uint64_t          dma_arb_req  :  9; // Request vector from 9 RxDMA input queues to the pre-rxdma input arbiter. This indicates that a request is available at the head of each input queue. Bit 0 represents MCTC0 queue, bit 1 represents MCTC1 queue, etc...
        uint64_t          dma_arb_gnt  :  9; // Grant vector to 9 RxDMA input queues. See dma_arb_req for bit definitions
        uint64_t       Reserved_31_18  : 14; // Reserved
        uint64_t              arb_req  :  5; // Request vector from the 5 clients vying for access to the input arbiter. Bit 0 represents the RxDMA, bit 1 represents RxE2E, and so on for RxCID, RxET, and RxHP as the MSB.
        uint64_t              arb_gnt  :  5; // Grant vector to 5 clients vying for access to the input arbiter. See arb_req for bit definitions.
        uint64_t              arb_ack  :  5; // Ack vector to 5 clients vying for access to the input arbiter. See arb_req for bit definitions.
        uint64_t          Reserved_47  :  1; // Reserved
        uint64_t            dma_empty  :  9; // Empties for 9 RxDMA input queues (prior to the queue output staging).
        uint64_t            e2e_empty  :  1; // Empty of RxE2E input queue.
        uint64_t            cid_empty  :  1; // Empty of RxCID input queue.
        uint64_t             et_empty  :  1; // Empty of RxET input queue.
        uint64_t             hp_empty  :  1; // Empty of RxHP input queue.
        uint64_t       Reserved_63_61  :  3; // Reserved
    } field;
    uint64_t val;
} RXHIARB_DBG_ARB_IN_STATE_t;

// RXHIARB_DBG_ARB_IN_HQUE_LVL_0_3 desc:
typedef union {
    struct {
        uint64_t          hque_depth0  :  8; // Indicates the number of location currently filled in holding queue 0
        uint64_t          hque_depth1  :  8; // Indicates the number of location currently filled in holding queue 1
        uint64_t          hque_depth2  :  8; // Indicates the number of location currently filled in holding queue 2
        uint64_t          hque_depth3  :  8; // Indicates the number of location currently filled in holding queue 3
        uint64_t             hque_wm0  :  4; // Current low watermark value for holding queue 0
        uint64_t             hque_wm1  :  4; // Current low watermark value for holding queue 1
        uint64_t             hque_wm2  :  4; // Current low watermark value for holding queue 2
        uint64_t             hque_wm3  :  4; // Current low watermark value for holding queue 3
        uint64_t           hque_rsvd0  :  4; // Number of reserved locations in holding queue 1
        uint64_t           hque_rsvd1  :  4; // Number of reserved locations in holding queue 2
        uint64_t           hque_rsvd2  :  4; // Number of reserved locations in holding queue 3
        uint64_t           hque_rsvd3  :  4; // Number of reserved locations in holding queue 4
    } field;
    uint64_t val;
} RXHIARB_DBG_ARB_IN_HQUE_LVL_0_3_t;

// RXHIARB_DBG_ARB_IN_HQUE_LVL_4_7 desc:
typedef union {
    struct {
        uint64_t          hque_depth4  :  8; // Indicates the number of location currently filled in holding queue 4
        uint64_t          hque_depth5  :  8; // Indicates the number of location currently filled in holding queue 5
        uint64_t          hque_depth6  :  8; // Indicates the number of location currently filled in holding queue 6
        uint64_t          hque_depth7  :  8; // Indicates the number of location currently filled in holding queue 7
        uint64_t             hque_wm4  :  4; // Current low watermark value for holding queue 4
        uint64_t             hque_wm5  :  4; // Current low watermark value for holding queue 5
        uint64_t             hque_wm6  :  4; // Current low watermark value for holding queue 6
        uint64_t             hque_wm7  :  4; // Current low watermark value for holding queue 7
        uint64_t           hque_rsvd4  :  4; // Number of reserved locations in holding queue 4
        uint64_t           hque_rsvd5  :  4; // Number of reserved locations in holding queue 5
        uint64_t           hque_rsvd6  :  4; // Number of reserved locations in holding queue 6
        uint64_t           hque_rsvd7  :  4; // Number of reserved locations in holding queue 7
    } field;
    uint64_t val;
} RXHIARB_DBG_ARB_IN_HQUE_LVL_4_7_t;

// RXHIARB_DBG_ARB_IN_HQUE_LVL_8M desc:
typedef union {
    struct {
        uint64_t          hque_depth8  :  8; // Indicates the number of location currently filled in holding queue 8
        uint64_t             hque_wm8  :  4; // Current low watermark value for holding queue 8
        uint64_t           hque_rsvd8  :  4; // Number of reserved locations in holding queue 8
        uint64_t           hids_avail  :  8; // Number of HIDs available in free pool/
        uint64_t            pcb_stall  :  1; // Stall indicator from PCB logic
        uint64_t           mtlb_stall  :  1; // Stall indicator from MTLB logic
        uint64_t       Reserved_63_26  : 38; // Reserved
    } field;
    uint64_t val;
} RXHIARB_DBG_ARB_IN_HQUE_LVL_8M_t;

// RXHIARB_DBG_ARB_OUT_STATE desc:
typedef union {
    struct {
        uint64_t              arb_req  :  9; // Request vector from 9 holding queues to the output arbiter. This indicates that a request is available at the head of each holding queue. Bit 0 represents MCTC0 queue, bit 1 represents MCTC1 queue, etc...
        uint64_t              arb_gnt  :  9; // Grant vector to 9 holding queues. See arb_req for bit definitions
        uint64_t              arb_ack  :  9; // Acknowledge vector to 9 holding queue. cSee arb_req for bit definitions.
        uint64_t       Reserved_31_27  :  5; // Reserved
        uint64_t        hi_fifo_stall  :  1; // Stall due to downstream HI FIFO fullness state (affects output arbiter).
        uint64_t          hi_fifo_cnt  :  5; // Inidctaes how many flits reside in the HI queue
        uint64_t       Reserved_39_38  :  2; // Reserved
        uint64_t           nack_stall  :  1; // Stall due to Nack queue fullness state (affects output arbiter).
        uint64_t        nack_fifo_cnt  :  4; // Indicates how many Nacks reside in Nack queue.
        uint64_t       Reserved_47_45  :  3; // Reserved
        uint64_t           flush_busy  :  1; // Flush busy state (affects output arbiter).
        uint64_t       Reserved_63_49  : 15; // Reserved
    } field;
    uint64_t val;
} RXHIARB_DBG_ARB_OUT_STATE_t;

// RXHIARB_DBG_SLRSP_STATE desc:
typedef union {
    struct {
        uint64_t             slow_cnt  :  6; // Number of slow response flits in slow queue
        uint64_t         Reserved_7_6  :  2; // Reserved
        uint64_t        slow_cred_cnt  :  5; // Number of pending response credits awaiting form slow path/
        uint64_t       Reserved_63_13  : 51; // Reserved
    } field;
    uint64_t val;
} RXHIARB_DBG_SLRSP_STATE_t;

// RXHIARB_DBG_AT_REQ_STATE desc:
typedef union {
    struct {
        uint64_t          at_fifo_cnt  :  5; // Number of requests pending in AT queue.
        uint64_t               at_rdy  :  1; // AT ready signal indicating that credits are available
        uint64_t             ref_busy  :  1; // Refetch busy
        uint64_t        Reserved_63_7  : 57; // Reserved
    } field;
    uint64_t val;
} RXHIARB_DBG_AT_REQ_STATE_t;

#endif /* ___FXR_rx_hiarb_CSRS_H__ */