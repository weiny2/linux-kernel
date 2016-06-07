// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Thu May 19 17:39:38 2016
//

#ifndef ___FXR_rx_ci_cid_CSRS_H__
#define ___FXR_rx_ci_cid_CSRS_H__

// RXCID_CFG_CNTRL desc:
typedef union {
    struct {
        uint64_t                  pid  : 12; // Process ID associated with this command queue. Default is to assign access to the kernel (reserved PID 0). Commands must use this PID as their IPID unless the CQ is privileged. See note in CSR description regarding the symmetrical pairing requirements between this and the Tx.
        uint64_t           priv_level  :  1; // Supervisor=0, User=1 See note in CSR description regarding the symmetrical pairing requirements between this and the Tx.
        uint64_t               enable  :  1; // A CQ that is not enabled can be written, but slot full counts will always be zero. See note in CSR description regarding the symmetrical pairing requirements between this and the Tx.
        uint64_t       Reserved_63_14  : 50; // Unused
    } field;
    uint64_t val;
} RXCID_CFG_CNTRL_t;

// RXCID_CFG_HEAD_UPDATE_ADDR desc:
typedef union {
    struct {
        uint64_t                valid  :  1; // Address is valid. Head Pointer updates are dropped unless the entry is valid. TBD: Add error bit for this.
        uint64_t         Reserved_3_1  :  3; // Unused
        uint64_t hd_ptr_host_addr_56_4  : 53; // 16B-aligned host address to write the updated TX head pointer. Updated RX head pointers are written to this address plus 0x08.
        uint64_t       Reserved_63_57  :  7; // Unused
    } field;
    uint64_t val;
} RXCID_CFG_HEAD_UPDATE_ADDR_t;

// RXCID_CFG_HEAD_UPDATE_CNTRL desc:
typedef union {
    struct {
        uint64_t            rate_ctrl  :  3; // Head pointer update frequency control, sets number of head pointer increments before head pointer is updated in memory. Value of 0, 1, 2, 3 and 4 correspond to updating on increments of 1, 2, 4, 8 and 16. Note that hardware interprets unused values of 5-7 as the maximum value of 4.
        uint64_t        Reserved_63_3  : 61; // Unused
    } field;
    uint64_t val;
} RXCID_CFG_HEAD_UPDATE_CNTRL_t;

// RXCID_CFG_SL0_TO_TC desc:
typedef union {
    struct {
        uint64_t            sl0_p0_tc  :  2; // Service level 0 and port 0 to traffic class mapping
        uint64_t            sl0_p0_mc  :  1; // Service level 0 and port 0 to message class mapping
        uint64_t           Reserved_3  :  1; // Unused
        uint64_t            sl1_p0_tc  :  2; // Service level 1 and port 0 to message class mapping
        uint64_t            sl1_p0_mc  :  1; // Service level 1 and port 0 to message class mapping
        uint64_t           Reserved_7  :  1; // Unused
        uint64_t            sl2_p0_tc  :  2; // Service level 2 and port 0 to traffic class mapping
        uint64_t            sl2_p0_mc  :  1; // Service level 2 and port 0 to message class mapping
        uint64_t          Reserved_11  :  1; // Unused
        uint64_t            sl3_p0_tc  :  2; // Service level 3 and port 0 to message class mapping
        uint64_t            sl3_p0_mc  :  1; // Service level 3 and port 0 to message class mapping
        uint64_t          Reserved_15  :  1; // Unused
        uint64_t            sl4_p0_tc  :  2; // Service level 4 and port 0 to traffic class mapping
        uint64_t            sl4_p0_mc  :  1; // Service level 4 and port 0 to message class mapping
        uint64_t          Reserved_19  :  1; // Unused
        uint64_t            sl5_p0_tc  :  2; // Service level 5 and port 0 to message class mapping
        uint64_t            sl5_p0_mc  :  1; // Service level 5 and port 0 to message class mapping
        uint64_t          Reserved_23  :  1; // Unused
        uint64_t            sl6_p0_tc  :  2; // Service level 6 and port 0 to traffic class mapping
        uint64_t            sl6_p0_mc  :  1; // Service level 6 and port 0 to message class mapping
        uint64_t          Reserved_27  :  1; // Unused
        uint64_t            sl7_p0_tc  :  2; // Service level 7 and port 0 to message class mapping
        uint64_t            sl7_p0_mc  :  1; // Service level 7 and port 0 to message class mapping
        uint64_t          Reserved_31  :  1; // Unused
        uint64_t            sl8_p0_tc  :  2; // Service level 8 and port 0 to traffic class mapping
        uint64_t            sl8_p0_mc  :  1; // Service level 8 and port 0 to message class mapping
        uint64_t          Reserved_35  :  1; // Unused
        uint64_t            sl9_p0_tc  :  2; // Service level 9 and port 0 to message class mapping
        uint64_t            sl9_p0_mc  :  1; // Service level 9 and port 0 to message class mapping
        uint64_t          Reserved_39  :  1; // Unused
        uint64_t           sl10_p0_tc  :  2; // Service level 10 and port 0 to traffic class mapping
        uint64_t           sl10_p0_mc  :  1; // Service level 10 and port 0 to message class mapping
        uint64_t          Reserved_43  :  1; // Unused
        uint64_t           sl11_p0_tc  :  2; // Service level 11 and port 0 to message class mapping
        uint64_t           sl11_p0_mc  :  1; // Service level 11 and port 0 to message class mapping
        uint64_t          Reserved_47  :  1; // Unused
        uint64_t           sl12_p0_tc  :  2; // Service level 12 and port 0 to traffic class mapping
        uint64_t           sl12_p0_mc  :  1; // Service level 12 and port 0 to message class mapping
        uint64_t          Reserved_51  :  1; // Unused
        uint64_t           sl13_p0_tc  :  2; // Service level 13 and port 0 to message class mapping
        uint64_t           sl13_p0_mc  :  1; // Service level 13 and port 0 to message class mapping
        uint64_t          Reserved_55  :  1; // Unused
        uint64_t           sl14_p0_tc  :  2; // Service level 14 and port 0 to traffic class mapping
        uint64_t           sl14_p0_mc  :  1; // Service level 14 and port 0 to message class mapping
        uint64_t          Reserved_59  :  1; // Unused
        uint64_t           sl15_p0_tc  :  2; // Service level 15 and port 0 to message class mapping
        uint64_t           sl15_p0_mc  :  1; // Service level 15 and port 0 to message class mapping
        uint64_t          Reserved_63  :  1; // Unused
    } field;
    uint64_t val;
} RXCID_CFG_SL0_TO_TC_t;

// RXCID_CFG_SL1_TO_TC desc:
typedef union {
    struct {
        uint64_t           sl16_p0_tc  :  2; // Service level 16 and port 0 to traffic class mapping
        uint64_t           sl16_p0_mc  :  1; // Service level 16 and port 0 to message class mapping
        uint64_t           Reserved_3  :  1; // Unused
        uint64_t           sl17_p0_tc  :  2; // Service level 17 and port 0 to message class mapping
        uint64_t           sl17_p0_mc  :  1; // Service level 17 and port 0 to message class mapping
        uint64_t           Reserved_7  :  1; // Unused
        uint64_t           sl18_p0_tc  :  2; // Service level 18 and port 0 to traffic class mapping
        uint64_t           sl18_p0_mc  :  1; // Service level 18 and port 0 to message class mapping
        uint64_t          Reserved_11  :  1; // Unused
        uint64_t           sl19_p0_tc  :  2; // Service level 19 and port 0 to message class mapping
        uint64_t           sl19_p0_mc  :  1; // Service level 19 and port 0 to message class mapping
        uint64_t          Reserved_15  :  1; // Unused
        uint64_t           sl20_p0_tc  :  2; // Service level 20 and port 0 to traffic class mapping
        uint64_t           sl20_p0_mc  :  1; // Service level 20 and port 0 to message class mapping
        uint64_t          Reserved_19  :  1; // Unused
        uint64_t           sl21_p0_tc  :  2; // Service level 21 and port 0 to message class mapping
        uint64_t           sl21_p0_mc  :  1; // Service level 21 and port 0 to message class mapping
        uint64_t          Reserved_23  :  1; // Unused
        uint64_t           sl22_p0_tc  :  2; // Service level 22 and port 0 to traffic class mapping
        uint64_t           sl22_p0_mc  :  1; // Service level 22 and port 0 to message class mapping
        uint64_t          Reserved_27  :  1; // Unused
        uint64_t           sl23_p0_tc  :  2; // Service level 23 and port 0 to message class mapping
        uint64_t           sl23_p0_mc  :  1; // Service level 23 and port 0 to message class mapping
        uint64_t          Reserved_31  :  1; // Unused
        uint64_t           sl24_p0_tc  :  2; // Service level 24 and port 0 to traffic class mapping
        uint64_t           sl24_p0_mc  :  1; // Service level 24 and port 0 to message class mapping
        uint64_t          Reserved_35  :  1; // Unused
        uint64_t           sl25_p0_tc  :  2; // Service level 25 and port 0 to message class mapping
        uint64_t           sl25_p0_mc  :  1; // Service level 25 and port 0 to message class mapping
        uint64_t          Reserved_39  :  1; // Unused
        uint64_t           sl26_p0_tc  :  2; // Service level 26 and port 0 to traffic class mapping
        uint64_t           sl26_p0_mc  :  1; // Service level 26 and port 0 to message class mapping
        uint64_t          Reserved_43  :  1; // Unused
        uint64_t           sl27_p0_tc  :  2; // Service level 27 and port 0 to message class mapping
        uint64_t           sl27_p0_mc  :  1; // Service level 27 and port 0 to message class mapping
        uint64_t          Reserved_47  :  1; // Unused
        uint64_t           sl28_p0_tc  :  2; // Service level 28 and port 0 to traffic class mapping
        uint64_t           sl28_p0_mc  :  1; // Service level 28 and port 0 to message class mapping
        uint64_t          Reserved_51  :  1; // Unused
        uint64_t           sl29_p0_tc  :  2; // Service level 29 and port 0 to message class mapping
        uint64_t           sl29_p0_mc  :  1; // Service level 29 and port 0 to message class mapping
        uint64_t          Reserved_55  :  1; // Unused
        uint64_t           sl30_p0_tc  :  2; // Service level 30 and port 0 to traffic class mapping
        uint64_t           sl30_p0_mc  :  1; // Service level 30 and port 0 to message class mapping
        uint64_t          Reserved_59  :  1; // Unused
        uint64_t           sl31_p0_tc  :  2; // Service level 31 and port 0 to message class mapping
        uint64_t           sl31_p0_mc  :  1; // Service level 31 and port 0 to message class mapping
        uint64_t          Reserved_63  :  1; // Unused
    } field;
    uint64_t val;
} RXCID_CFG_SL1_TO_TC_t;

// RXCID_CFG_SL2_TO_TC desc:
typedef union {
    struct {
        uint64_t            sl0_p1_tc  :  2; // Service level 0 and port 1 to traffic class mapping
        uint64_t            sl0_p1_mc  :  1; // Service level 0 and port 1 to message class mapping
        uint64_t           Reserved_3  :  1; // Unused
        uint64_t            sl1_p1_tc  :  2; // Service level 1 and port 1 to message class mapping
        uint64_t            sl1_p1_mc  :  1; // Service level 1 and port 1 to message class mapping
        uint64_t           Reserved_7  :  1; // Unused
        uint64_t            sl2_p1_tc  :  2; // Service level 2 and port 1 to traffic class mapping
        uint64_t            sl2_p1_mc  :  1; // Service level 2 and port 1 to message class mapping
        uint64_t          Reserved_11  :  1; // Unused
        uint64_t            sl3_p1_tc  :  2; // Service level 3 and port 1 to message class mapping
        uint64_t            sl3_p1_mc  :  1; // Service level 3 and port 1 to message class mapping
        uint64_t          Reserved_15  :  1; // Unused
        uint64_t            sl4_p1_tc  :  2; // Service level 4 and port 1 to traffic class mapping
        uint64_t            sl4_p1_mc  :  1; // Service level 4 and port 1 to message class mapping
        uint64_t          Reserved_19  :  1; // Unused
        uint64_t            sl5_p1_tc  :  2; // Service level 5 and port 1 to message class mapping
        uint64_t            sl5_p1_mc  :  1; // Service level 5 and port 1 to message class mapping
        uint64_t          Reserved_23  :  1; // Unused
        uint64_t            sl6_p1_tc  :  2; // Service level 6 and port 1 to traffic class mapping
        uint64_t            sl6_p1_mc  :  1; // Service level 6 and port 1 to message class mapping
        uint64_t          Reserved_27  :  1; // Unused
        uint64_t            sl7_p1_tc  :  2; // Service level 7 and port 1 to message class mapping
        uint64_t            sl7_p1_mc  :  1; // Service level 7 and port 1 to message class mapping
        uint64_t          Reserved_31  :  1; // Unused
        uint64_t            sl8_p1_tc  :  2; // Service level 8 and port 1 to traffic class mapping
        uint64_t            sl8_p1_mc  :  1; // Service level 8 and port 1 to message class mapping
        uint64_t          Reserved_35  :  1; // Unused
        uint64_t            sl9_p1_tc  :  2; // Service level 9 and port 1 to message class mapping
        uint64_t            sl9_p1_mc  :  1; // Service level 9 and port 1 to message class mapping
        uint64_t          Reserved_39  :  1; // Unused
        uint64_t           sl10_p1_tc  :  2; // Service level 10 and port 1 to traffic class mapping
        uint64_t           sl10_p1_mc  :  1; // Service level 10 and port 1 to message class mapping
        uint64_t          Reserved_43  :  1; // Unused
        uint64_t           sl11_p1_tc  :  2; // Service level 11 and port 1 to message class mapping
        uint64_t           sl11_p1_mc  :  1; // Service level 11 and port 1 to message class mapping
        uint64_t          Reserved_47  :  1; // Unused
        uint64_t           sl12_p1_tc  :  2; // Service level 12 and port 1 to traffic class mapping
        uint64_t           sl12_p1_mc  :  1; // Service level 12 and port 1 to message class mapping
        uint64_t          Reserved_51  :  1; // Unused
        uint64_t           sl13_p1_tc  :  2; // Service level 13 and port 1 to message class mapping
        uint64_t           sl13_p1_mc  :  1; // Service level 13 and port 1 to message class mapping
        uint64_t          Reserved_55  :  1; // Unused
        uint64_t           sl14_p1_tc  :  2; // Service level 14 and port 1 to traffic class mapping
        uint64_t           sl14_p1_mc  :  1; // Service level 14 and port 1 to message class mapping
        uint64_t          Reserved_59  :  1; // Unused
        uint64_t           sl15_p1_tc  :  2; // Service level 15 and port 1 to message class mapping
        uint64_t           sl15_p1_mc  :  1; // Service level 15 and port 1 to message class mapping
        uint64_t          Reserved_63  :  1; // Unused
    } field;
    uint64_t val;
} RXCID_CFG_SL2_TO_TC_t;

// RXCID_CFG_SL3_TO_TC desc:
typedef union {
    struct {
        uint64_t           sl16_p1_tc  :  2; // Service level 16 and port 1 to traffic class mapping
        uint64_t           sl16_p1_mc  :  1; // Service level 16 and port 1 to message class mapping
        uint64_t           Reserved_3  :  1; // Unused
        uint64_t           sl17_p1_tc  :  2; // Service level 17 and port 1 to message class mapping
        uint64_t           sl17_p1_mc  :  1; // Service level 17 and port 1 to message class mapping
        uint64_t           Reserved_7  :  1; // Unused
        uint64_t           sl18_p1_tc  :  2; // Service level 18 and port 1 to traffic class mapping
        uint64_t           sl18_p1_mc  :  1; // Service level 18 and port 1 to message class mapping
        uint64_t          Reserved_11  :  1; // Unused
        uint64_t           sl19_p1_tc  :  2; // Service level 19 and port 1 to message class mapping
        uint64_t           sl19_p1_mc  :  1; // Service level 19 and port 1 to message class mapping
        uint64_t          Reserved_15  :  1; // Unused
        uint64_t           sl20_p1_tc  :  2; // Service level 20 and port 1 to traffic class mapping
        uint64_t           sl20_p1_mc  :  1; // Service level 20 and port 1 to message class mapping
        uint64_t          Reserved_19  :  1; // Unused
        uint64_t           sl21_p1_tc  :  2; // Service level 21 and port 1 to message class mapping
        uint64_t           sl21_p1_mc  :  1; // Service level 21 and port 1 to message class mapping
        uint64_t          Reserved_23  :  1; // Unused
        uint64_t           sl22_p1_tc  :  2; // Service level 22 and port 1 to traffic class mapping
        uint64_t           sl22_p1_mc  :  1; // Service level 22 and port 1 to message class mapping
        uint64_t          Reserved_27  :  1; // Unused
        uint64_t           sl23_p1_tc  :  2; // Service level 23 and port 1 to message class mapping
        uint64_t           sl23_p1_mc  :  1; // Service level 23 and port 1 to message class mapping
        uint64_t          Reserved_31  :  1; // Unused
        uint64_t           sl24_p1_tc  :  2; // Service level 24 and port 1 to traffic class mapping
        uint64_t           sl24_p1_mc  :  1; // Service level 24 and port 1 to message class mapping
        uint64_t          Reserved_35  :  1; // Unused
        uint64_t           sl25_p1_tc  :  2; // Service level 25 and port 1 to message class mapping
        uint64_t           sl25_p1_mc  :  1; // Service level 25 and port 1 to message class mapping
        uint64_t          Reserved_39  :  1; // Unused
        uint64_t           sl26_p1_tc  :  2; // Service level 26 and port 1 to traffic class mapping
        uint64_t           sl26_p1_mc  :  1; // Service level 26 and port 1 to message class mapping
        uint64_t          Reserved_43  :  1; // Unused
        uint64_t           sl27_p1_tc  :  2; // Service level 27 and port 1 to message class mapping
        uint64_t           sl27_p1_mc  :  1; // Service level 27 and port 1 to message class mapping
        uint64_t          Reserved_47  :  1; // Unused
        uint64_t           sl28_p1_tc  :  2; // Service level 28 and port 1 to traffic class mapping
        uint64_t           sl28_p1_mc  :  1; // Service level 28 and port 1 to message class mapping
        uint64_t          Reserved_51  :  1; // Unused
        uint64_t           sl29_p1_tc  :  2; // Service level 29 and port 1 to message class mapping
        uint64_t           sl29_p1_mc  :  1; // Service level 29 and port 1 to message class mapping
        uint64_t          Reserved_55  :  1; // Unused
        uint64_t           sl30_p1_tc  :  2; // Service level 30 and port 1 to traffic class mapping
        uint64_t           sl30_p1_mc  :  1; // Service level 30 and port 1 to message class mapping
        uint64_t          Reserved_59  :  1; // Unused
        uint64_t           sl31_p1_tc  :  2; // Service level 31 and port 1 to message class mapping
        uint64_t           sl31_p1_mc  :  1; // Service level 31 and port 1 to message class mapping
        uint64_t          Reserved_63  :  1; // Unused
    } field;
    uint64_t val;
} RXCID_CFG_SL3_TO_TC_t;

// RXCID_CFG_HI_CRDTS desc:
typedef union {
    struct {
        uint64_t           hi_credits  :  5; // Request Input Queue Credits
        uint64_t          unused_63_5  : 59; // Unused
    } field;
    uint64_t val;
} RXCID_CFG_HI_CRDTS_t;

// RXCID_ERR_STS desc:
typedef union {
    struct {
        uint64_t   inv_write_inactive  :  1; // A CQ write occurred to a CQ that was inactive
        uint64_t     cmdq_csr_err_mbe  :  1; // Command queue MBE CSR error
        uint64_t     cmdq_csr_err_sbe  :  1; // Command queue SBE CSR error
        uint64_t       cq_mem_mbe_err  :  1; // CQ memory MBE error
        uint64_t       cq_mem_sbe_err  :  1; // CQ memory SBE error
        uint64_t     out_of_bound_err  :  1; // Detected a HIFIS write which crosses the 64byte boundary
        uint64_t     pid_mismatch_err  :  1; // Detected a PID mis match
        uint64_t        Reserved_63_7  : 57; // Unused
    } field;
    uint64_t val;
} RXCID_ERR_STS_t;

// RXCID_ERR_CLR desc:
typedef union {
    struct {
        uint64_t   inv_write_inactive  :  1; // A CQ write occurred to a CQ that was inactive
        uint64_t     cmdq_csr_err_mbe  :  1; // Command queue MBE CSR error
        uint64_t     cmdq_csr_err_sbe  :  1; // Command queue SBE CSR error
        uint64_t       cq_mem_mbe_err  :  1; // CQ memory MBE error
        uint64_t       cq_mem_sbe_err  :  1; // CQ memory SBE error
        uint64_t     out_of_bound_err  :  1; // Detected a HIFIS write which crosses the 64byte boundary
        uint64_t     pid_mismatch_err  :  1; // Detected a PID mis match
        uint64_t        Reserved_63_7  : 57; // Unused
    } field;
    uint64_t val;
} RXCID_ERR_CLR_t;

// RXCID_ERR_FRC desc:
typedef union {
    struct {
        uint64_t   inv_write_inactive  :  1; // A CQ write occurred to a CQ that was inactive
        uint64_t     cmdq_csr_err_mbe  :  1; // Command queue MBE CSR error
        uint64_t     cmdq_csr_err_sbe  :  1; // Command queue SBE CSR error
        uint64_t       cq_mem_mbe_err  :  1; // CQ memory MBE error
        uint64_t       cq_mem_sbe_err  :  1; // CQ memory SBE error
        uint64_t     out_of_bound_err  :  1; // Detected a HIFIS write which crosses the 64byte boundary
        uint64_t     pid_mismatch_err  :  1; // Detected a PID mis match
        uint64_t        Reserved_63_7  : 57; // Unused
    } field;
    uint64_t val;
} RXCID_ERR_FRC_t;

// RXCID_ERR_EN_HOST desc:
typedef union {
    struct {
        uint64_t   inv_write_inactive  :  1; // A CQ write occurred to a CQ that was inactive
        uint64_t     cmdq_csr_err_mbe  :  1; // Command queue MBE CSR error
        uint64_t     cmdq_csr_err_sbe  :  1; // Command queue SBE CSR error
        uint64_t       cq_mem_mbe_err  :  1; // CQ memory MBE error
        uint64_t       cq_mem_sbe_err  :  1; // CQ memory SBE error
        uint64_t     out_of_bound_err  :  1; // Detected a HIFIS write which crosses the 64byte boundary
        uint64_t     pid_mismatch_err  :  1; // Detected a PID mis match
        uint64_t        Reserved_63_7  : 57; // Unused
    } field;
    uint64_t val;
} RXCID_ERR_EN_HOST_t;

// RXCID_ERR_FIRST_HOST desc:
typedef union {
    struct {
        uint64_t   inv_write_inactive  :  1; // A CQ write occurred to a CQ that was inactive
        uint64_t     cmdq_csr_err_mbe  :  1; // Command queue MBE CSR error
        uint64_t     cmdq_csr_err_sbe  :  1; // Command queue SBE CSR error
        uint64_t       cq_mem_mbe_err  :  1; // CQ memory MBE error
        uint64_t       cq_mem_sbe_err  :  1; // CQ memory SBE error
        uint64_t     out_of_bound_err  :  1; // Detected a HIFIS write which crosses the 64byte boundary
        uint64_t     pid_mismatch_err  :  1; // Detected a PID mis match
        uint64_t        Reserved_63_7  : 57; // Unused
    } field;
    uint64_t val;
} RXCID_ERR_FIRST_HOST_t;

// RXCID_ERR_EN_BMC desc:
typedef union {
    struct {
        uint64_t   inv_write_inactive  :  1; // A CQ write occurred to a CQ that was inactive
        uint64_t     cmdq_csr_err_mbe  :  1; // Command queue MBE CSR error
        uint64_t     cmdq_csr_err_sbe  :  1; // Command queue SBE CSR error
        uint64_t       cq_mem_mbe_err  :  1; // CQ memory MBE error
        uint64_t       cq_mem_sbe_err  :  1; // CQ memory SBE error
        uint64_t     out_of_bound_err  :  1; // Detected a HIFIS write which crosses the 64byte boundary
        uint64_t     pid_mismatch_err  :  1; // Detected a PID mis match
        uint64_t        Reserved_63_7  : 57; // Unused
    } field;
    uint64_t val;
} RXCID_ERR_EN_BMC_t;

// RXCID_ERR_FIRST_BMC desc:
typedef union {
    struct {
        uint64_t   inv_write_inactive  :  1; // A CQ write occurred to a CQ that was inactive
        uint64_t     cmdq_csr_err_mbe  :  1; // Command queue MBE CSR error
        uint64_t     cmdq_csr_err_sbe  :  1; // Command queue SBE CSR error
        uint64_t       cq_mem_mbe_err  :  1; // CQ memory MBE error
        uint64_t       cq_mem_sbe_err  :  1; // CQ memory SBE error
        uint64_t     out_of_bound_err  :  1; // Detected a HIFIS write which crosses the 64byte boundary
        uint64_t     pid_mismatch_err  :  1; // Detected a PID mis match
        uint64_t        Reserved_63_7  : 57; // Unused
    } field;
    uint64_t val;
} RXCID_ERR_FIRST_BMC_t;

// RXCID_ERR_EN_QUAR desc:
typedef union {
    struct {
        uint64_t   inv_write_inactive  :  1; // A CQ write occurred to a CQ that was inactive
        uint64_t     cmdq_csr_err_mbe  :  1; // Command queue MBE CSR error
        uint64_t     cmdq_csr_err_sbe  :  1; // Command queue SBE CSR error
        uint64_t       cq_mem_mbe_err  :  1; // CQ memory MBE error
        uint64_t       cq_mem_sbe_err  :  1; // CQ memory SBE error
        uint64_t     out_of_bound_err  :  1; // Detected a HIFIS write which crosses the 64byte boundary
        uint64_t     pid_mismatch_err  :  1; // Detected a PID mis match
        uint64_t        Reserved_63_7  : 57; // Unused
    } field;
    uint64_t val;
} RXCID_ERR_EN_QUAR_t;

// RXCID_ERR_FIRST_QUAR desc:
typedef union {
    struct {
        uint64_t   inv_write_inactive  :  1; // A CQ write occurred to a CQ that was inactive
        uint64_t     cmdq_csr_err_mbe  :  1; // Command queue MBE CSR error
        uint64_t     cmdq_csr_err_sbe  :  1; // Command queue SBE CSR error
        uint64_t       cq_mem_mbe_err  :  1; // CQ memory MBE error
        uint64_t       cq_mem_sbe_err  :  1; // CQ memory SBE error
        uint64_t     out_of_bound_err  :  1; // Detected a HIFIS write which crosses the 64byte boundary
        uint64_t     pid_mismatch_err  :  1; // Detected a PID mis match
        uint64_t        Reserved_63_7  : 57; // Unused
    } field;
    uint64_t val;
} RXCID_ERR_FIRST_QUAR_t;

// RXCID_ERR_INFO_SBE_MBE desc:
typedef union {
    struct {
        uint64_t syndrome_cmd_csr_mbe  :  8; // Syndrome of the last MBE of command queue CSR
        uint64_t syndrome_cmd_csr_sbe  :  8; // Syndrome of the last SBE of command queue CSR
        uint64_t syndrome_cq_qword0_mbe  :  8; // Syndrome of last MBE for the CQ memory QWORD 0
        uint64_t syndrome_cq_qword0_sbe  :  8; // Syndrome of last SBE for the CQ memory QWORD 0
        uint64_t syndrome_cq_qword3_mbe  :  8; // Syndrome of last MBE for the CQ memory QWORD 3
        uint64_t syndrome_cq_qword3_sbe  :  8; // Syndrome of last SBE for the CQ memory QWORD 3
        uint64_t syndrome_cq_qword4_mbe  :  8; // Syndrome of last MBE for the CQ memory QWORD 4
        uint64_t syndrome_cq_qword4_sbe  :  8; // Syndrome of last SBE for the CQ memory QWORD 4
    } field;
    uint64_t val;
} RXCID_ERR_INFO_SBE_MBE_t;

// RXCID_ERR_CQ_GENERAL desc:
typedef union {
    struct {
        uint64_t inv_write_inactive_addr  :  4; // slot within CQ for inv_write_inactive error
        uint64_t inv_write_inactive_cq  :  8; // CQ for inv_write_inactive error
        uint64_t     cmdq_csr_err_mbe  :  8; // CQ number for the last Command queue MBE CSR error
        uint64_t      addr_cq_mem_err  :  4; // Slot Address for the last Command queue MBE CSR error
        uint64_t           cq_mem_err  :  8; // CQ number for the last command queue memory MBE error
        uint64_t     qword_cq_mem_err  :  4; // Qword position of the last command queue memory MBE error
        uint64_t                  cnt  :  8; // Saturating counter of MBE's from all sources
        uint64_t  outofbound_err_slot  :  4; // Slot number for the last out of bound error
        uint64_t    outofbound_err_cq  :  8; // CQ number for the last out of bound error
        uint64_t  pid_mismatch_err_cq  :  8; // CQ number for the last PID mismatch err
    } field;
    uint64_t val;
} RXCID_ERR_CQ_GENERAL_t;

// RXCID_DBG_ERR_INJECT desc:
typedef union {
    struct {
        uint64_t  inject_csr_err_mask  :  6; // Error injection mask for CSR ECC error
        uint64_t       inject_csr_err  :  1; // Error injection enable for CSR ECC error
        uint64_t           Reserved_7  :  1; // Unused
        uint64_t   inject_cq_err_mask  :  8; // Error injection mask for CQ memory ECC error
        uint64_t        inject_cq_err  :  1; // Error injection enable for CQ memory ECC error
        uint64_t       Reserved_63_17  : 47; // Unused
    } field;
    uint64_t val;
} RXCID_DBG_ERR_INJECT_t;

// RXCID_DBG_CQ_DATA_ACCESS desc:
typedef union {
    struct {
        uint64_t          rx_cq_qword  : 64; // Quad word in the receive command queue.
    } field;
    uint64_t val;
} RXCID_DBG_CQ_DATA_ACCESS_t;

#endif /* ___FXR_rx_ci_cid_CSRS_H__ */