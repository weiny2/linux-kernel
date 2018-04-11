// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Wed Apr 11 12:49:08 2018
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

// RXCID_CFG_PID_MASK desc:
typedef union {
    struct {
        uint64_t             pid_mask  : 12; // Process ID mask associated with this command queue. Default is to allow only a single Process ID for the command queue. See note in CSR description regarding the symmetrical pairing requirements between this and the Tx.
        uint64_t       Reserved_63_12  : 52; // Unused
    } field;
    uint64_t val;
} RXCID_CFG_PID_MASK_t;

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
        uint64_t            rate_ctrl  :  3; // Head pointer update frequency control. This register sets the number of head pointer increments that must occur before a head pointer update is scheduled. If multiple updates are scheduled before the update write is sent, they are combined. When the update write occurs, the current value of the head pointer is used, which may be more recent than the value that caused the update to be scheduled. Values of 0, 1, 2, 3, 4 and 5 correspond to updating on increments of 1, 2, 4, 8, 16 and 32. Values 6-7 are reserved for future use and the hardware currently interprets them the same as 5 to prevent undefined behavior.
        uint64_t                chint  :  4; // chint[1] = allocating indicator (table default value = 1) chint[0] = temporal indicator (table default value = 1)
        uint64_t        Reserved_63_7  : 57; // Unused
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

// RXCID_CFG_MAX_CONTEXTS desc:
typedef union {
    struct {
        uint64_t      fxr_max_context  :  9; // Maximum number of CQ's supported by FXR
        uint64_t        Reserved_63_9  : 55; // Unused
    } field;
    uint64_t val;
} RXCID_CFG_MAX_CONTEXTS_t;

// RXCID_ERR_STS desc:
typedef union {
    struct {
        uint64_t   inv_write_inactive  :  1; // A CQ write occurred to a CQ that was inactive. ERR_CATEGORY_PROCESS
        uint64_t     cmdq_csr_err_mbe  :  1; // Command queue MBE CSR error. ERR_CATEGORY_HFI
        uint64_t     cmdq_csr_err_sbe  :  1; // Command queue SBE CSR error. ERR_CATEGORY_CORRECTABLE
        uint64_t       cq_mem_mbe_err  :  1; // CQ memory MBE error. ERR_CATEGORY_HFI
        uint64_t       cq_mem_sbe_err  :  1; // CQ memory SBE error. ERR_CATEGORY_CORRECTABLE
        uint64_t     out_of_bound_err  :  1; // Detected a HIFIS write which crosses the 64byte boundary. ERR_CATEGORY_COMMAND
        uint64_t     pid_mismatch_err  :  1; // Detected a PID mis match. ERR_CATEGORY_COMMAND
        uint64_t    csr_maddr_mbe_err  :  1; // Detected a MADDR MBE error. ERR_CATEGORY_HFI
        uint64_t    csr_maddr_sbe_err  :  1; // Detected a MADDR SBE error. ERR_CATEGORY_CORRECTABLE
        uint64_t   csr_tx_cnt_mbe_err  :  1; // Detected a TX count MBE error. ERR_CATEGORY_HFI
        uint64_t   csr_tx_cnt_sbe_err  :  1; // Detected a TX count SBE error. ERR_CATEGORY_CORRECTABLE
        uint64_t   csr_rx_cnt_mbe_err  :  1; // Detected a RX count MBE error. ERR_CATEGORY_HFI
        uint64_t   csr_rx_cnt_sbe_err  :  1; // Detected a RX count SBE error. ERR_CATEGORY_CORRECTABLE
        uint64_t   csr_tx_ptr_mbe_err  :  1; // Detected a TX pointer MBE error. ERR_CATEGORY_HFI
        uint64_t   csr_tx_ptr_sbe_err  :  1; // Detected a TX pointer SBE error. ERR_CATEGORY_CORRECTABLE
        uint64_t   csr_rx_ptr_mbe_err  :  1; // Detected a RX pointer MBE error. ERR_CATEGORY_HFI
        uint64_t   csr_rx_ptr_sbe_err  :  1; // Detected a RX pointer SBE error. ERR_CATEGORY_CORRECTABLE
        uint64_t csr_req_fifo_mbe_err  :  1; // Detected a Request FIFO MBE error. ERR_CATEGORY_HFI
        uint64_t csr_req_fifo_sbe_err  :  1; // Detected a Request FIFO SBE error. ERR_CATEGORY_CORRECTABLE
        uint64_t csr_tx_pend_inval_err  :  1; // Detected a dropped TX updates due to a CQ with a non-valid CSR configuration head pointer memory address. ERR_CATEGORY_INFO
        uint64_t csr_rx_pend_inval_err  :  1; // Detected a dropped RX updates due to a CQ with a non-valid CSR configuration head pointer memory address. ERR_CATEGORY_INFO
        uint64_t hifis_hdr_ecc_mbe_err  :  1; // Detected a HIFIS HDR MBE error for reads from the hifis fifo. ERR_CATEGORY_HFI
        uint64_t hifis_hdr_ecc_sbe_err  :  1; // Detected a HIFIS HDR SBE error for reads from the hifis fifo. ERR_CATEGORY_CORRECTABLE
        uint64_t hifis_reqrsp_dval_par_err  :  1; // Detected a HIFIS dval parity error for reads from the hifis fifo. ERR_CATEGORY_HFI
        uint64_t hifis_reqrsp_tval_par_err  :  1; // Detected a HIFIS tval parity error for reads from the hifis fifo. ERR_CATEGORY_HFI
        uint64_t hifis_reqrsp_hval_par_err  :  1; // Detected a HIFIS hval parity error for reads from the hifis fifo. ERR_CATEGORY_HFI
        uint64_t cq_raw_cor255_192_mbe_err  :  1; // Detected a HIFIS data MBE error for reads from the hifis fifo. ERR_CATEGORY_HFI
        uint64_t cq_raw_cor255_192_sbe_err  :  1; // Detected a HIFIS data SBE error for reads from the hifis fifo ERR_CATEGORY_CORRECTABLE
        uint64_t    trig_opps_len_err  :  1; // Detected a triggered opps length error. ERR_CATEGORY_COMMAND IF command is TRIGGERED_APPEND or ORDERED_TRIG_APPEND and the command is RX_TRIG, the CMD_LEN can be 2'b10 (96 bytes) or less - Command dropped if the length is 2'b11. IF command is TRIGGERED_DISABLE, the CMD_LEN can be 2'b01 (64bytes) or less - CMD dropped if the length is 2'b10 or 2'b11.
        uint64_t       Reserved_63_29  : 35; // Unused
    } field;
    uint64_t val;
} RXCID_ERR_STS_t;

// RXCID_ERR_CLR desc:
typedef union {
    struct {
        uint64_t               events  : 29; // Write 1's to clear corresponding status bits.
        uint64_t       Reserved_63_29  : 35; // Unused
    } field;
    uint64_t val;
} RXCID_ERR_CLR_t;

// RXCID_ERR_FRC desc:
typedef union {
    struct {
        uint64_t               events  : 29; // Write 1 to set corresponding status bits.
        uint64_t       Reserved_63_29  : 35; // Unused
    } field;
    uint64_t val;
} RXCID_ERR_FRC_t;

// RXCID_ERR_EN_HOST desc:
typedef union {
    struct {
        uint64_t               events  : 29; // Enables corresponding status bits to generate host interrupt signal.
        uint64_t       Reserved_63_29  : 35; // Unused
    } field;
    uint64_t val;
} RXCID_ERR_EN_HOST_t;

// RXCID_ERR_FIRST_HOST desc:
typedef union {
    struct {
        uint64_t               events  : 29; // Snapshot of status bits when host interrupt signal transitions from 0 to 1.
        uint64_t       Reserved_63_29  : 35; // Unused
    } field;
    uint64_t val;
} RXCID_ERR_FIRST_HOST_t;

// RXCID_ERR_EN_BMC desc:
typedef union {
    struct {
        uint64_t               events  : 29; // Enables corresponding status bits to generate quarantine interrupt signal.
        uint64_t       Reserved_63_29  : 35; // Unused
    } field;
    uint64_t val;
} RXCID_ERR_EN_BMC_t;

// RXCID_ERR_FIRST_BMC desc:
typedef union {
    struct {
        uint64_t               events  : 29; // Snapshot of status bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t       Reserved_63_29  : 35; // Unused
    } field;
    uint64_t val;
} RXCID_ERR_FIRST_BMC_t;

// RXCID_ERR_EN_QUAR desc:
typedef union {
    struct {
        uint64_t               events  : 29; // Enables corresponding status bits to generate BMC interrupt signal.
        uint64_t       Reserved_63_29  : 35; // Unused
    } field;
    uint64_t val;
} RXCID_ERR_EN_QUAR_t;

// RXCID_ERR_FIRST_QUAR desc:
typedef union {
    struct {
        uint64_t               events  : 29; // Snapshot of status bits when quarantine interrupt signal transitions from 0 to 1.
        uint64_t       Reserved_63_29  : 35; // Unused
    } field;
    uint64_t val;
} RXCID_ERR_FIRST_QUAR_t;

// RXCID_ERR_INFO_SBE_MBE_0 desc:
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
} RXCID_ERR_INFO_SBE_MBE_0_t;

// RXCID_ERR_INFO_SBE_MBE_1 desc:
typedef union {
    struct {
        uint64_t syndrome_csr_maddr_mbe  :  7; // Syndrome of the last MBE of MADDR CSR
        uint64_t syndrome_csr_maddr_sbe  :  7; // Syndrome of the last SBE of MADDR CSR
        uint64_t  syndrome_tx_cnt_mbe  :  5; // Syndrome of last MBE for the TX count
        uint64_t  syndrome_tx_cnt_sbe  :  5; // Syndrome of last SBE for the TX count
        uint64_t  syndrome_rx_cnt_mbe  :  5; // Syndrome of last MBE for the RX count
        uint64_t  syndrome_rx_cnt_sbe  :  5; // Syndrome of last SBE for the RX count
        uint64_t  syndrome_tx_ptr_mbe  :  5; // Syndrome of last MBE for the TX pointer
        uint64_t  syndrome_tx_ptr_sbe  :  5; // Syndrome of last SBE for the TX pointer
        uint64_t  syndrome_rx_ptr_mbe  :  5; // Syndrome of last MBE for the RX pointer
        uint64_t  syndrome_rx_ptr_sbe  :  5; // Syndrome of last SBE for the RX pointer
        uint64_t       Reserved_63_54  : 10; // Unused
    } field;
    uint64_t val;
} RXCID_ERR_INFO_SBE_MBE_1_t;

// RXCID_ERR_INFO_SBE_MBE_2 desc:
typedef union {
    struct {
        uint64_t syndrome_req_fifo_mbe  :  8; // Syndrome of the first MBE for the request fifo
        uint64_t syndrome_req_fifo_sbe  :  8; // Syndrome of the first SBE for the request fifo
        uint64_t syndrome_hifis_hdr_ecc_mbe  :  8; // Syndrome of the first MBE for the hifis fifo HDR reads
        uint64_t syndrome_hifis_hdr_ecc_sbe  :  8; // Syndrome of the first SBE for the hifis fifo HDR reads
        uint64_t syndrome_cq_raw_ecc_mbe  :  8; // Syndrome of the first MBE for the hifis fifo data reads
        uint64_t syndrome_cq_raw_ecc_sbe  :  8; // Syndrome of the first SBE for the hifis fifo data reads
        uint64_t       Reserved_63_48  : 16; // Unused
    } field;
    uint64_t val;
} RXCID_ERR_INFO_SBE_MBE_2_t;

// RXCID_ERR_CQ_GENERAL_0 desc:
typedef union {
    struct {
        uint64_t inv_write_inactive_addr  :  4; // slot within CQ for inv_write_inactive error
        uint64_t inv_write_inactive_cq  :  8; // CQ for inv_write_inactive error
        uint64_t     cmdq_csr_err_mbe  :  8; // CQ number for the first Command queue MBE CSR error
        uint64_t      addr_cq_mem_err  :  4; // Slot Address for the first Command queue MBE CSR error
        uint64_t           cq_mem_err  :  8; // CQ number for the first command queue memory MBE error
        uint64_t     qword_cq_mem_err  :  4; // Qword position of the first command queue memory MBE error
        uint64_t                  cnt  :  8; // Saturating counter of MBE's from all sources
        uint64_t  outofbound_err_slot  :  4; // Slot number for the first out of bound error
        uint64_t    outofbound_err_cq  :  8; // CQ number for the first out of bound error
        uint64_t  pid_mismatch_err_cq  :  8; // CQ number for the first PID mismatch err
    } field;
    uint64_t val;
} RXCID_ERR_CQ_GENERAL_0_t;

// RXCID_ERR_CQ_GENERAL_1 desc:
typedef union {
    struct {
        uint64_t     csr_maddr_cq_mbe  :  8; // CQ for the first MADDR MBE error
        uint64_t     csr_maddr_cq_sbe  :  8; // CQ for the first MADDR MBE error
        uint64_t    csr_tx_cnt_cq_mbe  :  8; // CQ for the first TX count MBE error
        uint64_t    csr_tx_cnt_cq_sbe  :  8; // CQ for the first TX count SBE error
        uint64_t    csr_rx_cnt_cq_mbe  :  8; // CQ for the first RX count MBE error
        uint64_t    csr_rx_cnt_cq_sbe  :  8; // CQ for the first RX count SBE error
        uint64_t    csr_tx_ptr_cq_mbe  :  8; // CQ for the first TX pointer MBE error
        uint64_t    csr_tx_ptr_cq_sbe  :  8; // CQ for the first TX pointer SBE error
    } field;
    uint64_t val;
} RXCID_ERR_CQ_GENERAL_1_t;

// RXCID_ERR_CQ_GENERAL_2 desc:
typedef union {
    struct {
        uint64_t    csr_rx_ptr_cq_mbe  :  8; // CQ for the first RX pointer MBE error
        uint64_t    csr_rx_ptr_cq_sbe  :  8; // CQ for the first RX pointer SBE error
        uint64_t trig_opps_len_err_cq  :  8; // CQ for the triggered opps length error
        uint64_t       Reserved_63_24  : 40; // Unused
    } field;
    uint64_t val;
} RXCID_ERR_CQ_GENERAL_2_t;

// RXCID_DBG_ERR_INJECT desc:
typedef union {
    struct {
        uint64_t  inject_csr_err_mask  :  6; // Error injection mask for CSR ECC error
        uint64_t       inject_csr_err  :  1; // Error injection enable for CSR ECC error
        uint64_t   inject_cq_err_mask  :  8; // Error injection mask for CQ memory ECC error
        uint64_t        inject_cq_err  :  1; // Error injection enable for CQ memory ECC error
        uint64_t inject_maddr_err_mask  :  7; // Error injection mask for MADDR ECC error
        uint64_t     inject_maddr_err  :  1; // Error injection enable for MADDR ECC error
        uint64_t   inject_tx_cnt_mask  :  5; // Error injection mask for TX count ECC error
        uint64_t    inject_tx_cnt_err  :  1; // Error injection enable for TX count ECC error
        uint64_t   inject_rx_cnt_mask  :  5; // Error injection mask for RX count ECC error
        uint64_t    inject_rx_cnt_err  :  1; // Error injection enable for RX count ECC error
        uint64_t   inject_tx_ptr_mask  :  5; // Error injection mask for TX pointer ECC error
        uint64_t    inject_tx_ptr_err  :  1; // Error injection enable for TX pointer ECC error
        uint64_t   inject_rx_ptr_mask  :  5; // Error injection mask for RX pointer ECC error
        uint64_t    inject_rx_ptr_err  :  1; // Error injection enable for RX pointer ECC error
        uint64_t inject_req_fifo_mask  :  8; // Error injection mask for Request FIFO ECC error
        uint64_t  inject_req_fifo_err  :  1; // Error injection enable for Request FIFO ECC error
        uint64_t       Reserved_63_57  :  7; // Unused
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
