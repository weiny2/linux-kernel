// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Thu Jun  2 19:11:24 2016
//

#ifndef ___FXR_rx_ci_cic_CSRS_H__
#define ___FXR_rx_ci_cic_CSRS_H__

// RXCIC_CFG_CQ_DRAIN_RESET desc:
typedef union {
    struct {
        uint64_t             drain_cq  :  8; // CQ for drain and reset operation
        uint64_t                drain  :  1; // Drain the CQ of commands
        uint64_t                reset  :  1; // Reset the CQ
        uint64_t                 busy  :  1; // Drain or reset operation active. Set by hardware when this CSR is written, cleared when the specified operations finish.
        uint64_t       Reserved_63_11  : 53; // Unused
    } field;
    uint64_t val;
} RXCIC_CFG_CQ_DRAIN_RESET_t;

// RXCIC_CFG_DISABLE_ARBITRATION desc:
typedef union {
    struct {
        uint64_t  arbitration_disable  :  1; // Disable arbitration between the CQ's
        uint64_t        Reserved_63_1  : 63; // Unused
    } field;
    uint64_t val;
} RXCIC_CFG_DISABLE_ARBITRATION_t;

// RXCIC_CFG_TO_LIMIT desc:
typedef union {
    struct {
        uint64_t                  rsv  : 20; // Reserved, timeout value is limited to increments of one million
        uint64_t             to_limit  : 17; // Timeout value, in number of FXR clock ticks
        uint64_t               rsv_63  : 27; // Reserved
    } field;
    uint64_t val;
} RXCIC_CFG_TO_LIMIT_t;

// RXCIC_STS_CQ_TAIL desc:
typedef union {
    struct {
        uint64_t                 tail  :  4; // Command queue tail slot
        uint64_t        Reserved_63_4  : 60; // Unused
    } field;
    uint64_t val;
} RXCIC_STS_CQ_TAIL_t;

// RXCIC_ERR_STS desc:
typedef union {
    struct {
        uint64_t      inv_write_flush  :  1; // A write occurred tio a CQ that was being drained. The write did not occur
        uint64_t    qword_cnt_err_mbe  :  1; // QWORD Count memory tracking MBE error
        uint64_t    qword_cnt_err_sbe  :  1; // QWORD Count memory tracking SBE error
        uint64_t cqslot_full_vec_error  :  1; // Parity error in the Slot full tracking logic
        uint64_t cqslot_len_vec_error  :  1; // Parity error in the command length tracking logic
        uint64_t           cq_timeout  :  1; // CQ timeout has occurred
        uint64_t     qword_cnt_ovrflw  :  1; // QWORD count overflow
        uint64_t     head_ptr_par_err  :  1; // Head pointer parity error
        uint64_t        Reserved_63_8  : 56; // Unused
    } field;
    uint64_t val;
} RXCIC_ERR_STS_t;

// RXCIC_ERR_CLR desc:
typedef union {
    struct {
        uint64_t      inv_write_flush  :  1; // A write occurred tio a CQ that was being drained. The write did not occur
        uint64_t    qword_cnt_err_mbe  :  1; // QWORD Count memory tracking MBE error
        uint64_t    qword_cnt_err_sbe  :  1; // QWORD Count memory tracking SBE error
        uint64_t cqslot_full_vec_error  :  1; // Parity error in the Slot full tracking logic
        uint64_t cqslot_len_vec_error  :  1; // Parity error in the command length tracking logic
        uint64_t           cq_timeout  :  1; // CQ timeout has occurred
        uint64_t     qword_cnt_ovrflw  :  1; // QWORD count overflow
        uint64_t     head_ptr_par_err  :  1; // Head pointer parity error
        uint64_t        Reserved_63_8  : 56; // Unused
    } field;
    uint64_t val;
} RXCIC_ERR_CLR_t;

// RXCIC_ERR_FRC desc:
typedef union {
    struct {
        uint64_t      inv_write_flush  :  1; // A write occurred tio a CQ that was being drained. The write did not occur
        uint64_t    qword_cnt_err_mbe  :  1; // QWORD Count memory tracking MBE error
        uint64_t    qword_cnt_err_sbe  :  1; // QWORD Count memory tracking SBE error
        uint64_t cqslot_full_vec_error  :  1; // Parity error in the Slot full tracking logic
        uint64_t cqslot_len_vec_error  :  1; // Parity error in the command length tracking logic
        uint64_t           cq_timeout  :  1; // CQ timeout has occurred
        uint64_t     qword_cnt_ovrflw  :  1; // QWORD count overflow
        uint64_t     head_ptr_par_err  :  1; // Head pointer parity error
        uint64_t        Reserved_63_8  : 56; // Unused
    } field;
    uint64_t val;
} RXCIC_ERR_FRC_t;

// RXCIC_ERR_EN_HOST desc:
typedef union {
    struct {
        uint64_t      inv_write_flush  :  1; // A write occurred tio a CQ that was being drained. The write did not occur
        uint64_t    qword_cnt_err_mbe  :  1; // QWORD Count memory tracking MBE error
        uint64_t    qword_cnt_err_sbe  :  1; // QWORD Count memory tracking SBE error
        uint64_t cqslot_full_vec_error  :  1; // Parity error in the Slot full tracking logic
        uint64_t cqslot_len_vec_error  :  1; // Parity error in the command length tracking logic
        uint64_t           cq_timeout  :  1; // CQ timeout has occurred
        uint64_t     qword_cnt_ovrflw  :  1; // QWORD count overflow
        uint64_t     head_ptr_par_err  :  1; // Head pointer parity error
        uint64_t        Reserved_63_8  : 56; // Unused
    } field;
    uint64_t val;
} RXCIC_ERR_EN_HOST_t;

// RXCIC_ERR_FIRST_HOST desc:
typedef union {
    struct {
        uint64_t      inv_write_flush  :  1; // A write occurred tio a CQ that was being drained. The write did not occur
        uint64_t    qword_cnt_err_mbe  :  1; // QWORD Count memory tracking MBE error
        uint64_t    qword_cnt_err_sbe  :  1; // QWORD Count memory tracking SBE error
        uint64_t cqslot_full_vec_error  :  1; // Parity error in the Slot full tracking logic
        uint64_t cqslot_len_vec_error  :  1; // Parity error in the command length tracking logic
        uint64_t           cq_timeout  :  1; // CQ timeout has occurred
        uint64_t     qword_cnt_ovrflw  :  1; // QWORD count overflow
        uint64_t     head_ptr_par_err  :  1; // Head pointer parity error
        uint64_t        Reserved_63_8  : 56; // Unused
    } field;
    uint64_t val;
} RXCIC_ERR_FIRST_HOST_t;

// RXCIC_ERR_EN_BMC desc:
typedef union {
    struct {
        uint64_t      inv_write_flush  :  1; // A write occurred tio a CQ that was being drained. The write did not occur
        uint64_t    qword_cnt_err_mbe  :  1; // QWORD Count memory tracking MBE error
        uint64_t    qword_cnt_err_sbe  :  1; // QWORD Count memory tracking SBE error
        uint64_t cqslot_full_vec_error  :  1; // Parity error in the Slot full tracking logic
        uint64_t cqslot_len_vec_error  :  1; // Parity error in the command length tracking logic
        uint64_t           cq_timeout  :  1; // CQ timeout has occurred
        uint64_t     qword_cnt_ovrflw  :  1; // QWORD count overflow
        uint64_t     head_ptr_par_err  :  1; // Head pointer parity error
        uint64_t        Reserved_63_8  : 56; // Unused
    } field;
    uint64_t val;
} RXCIC_ERR_EN_BMC_t;

// RXCIC_ERR_FIRST_BMC desc:
typedef union {
    struct {
        uint64_t      inv_write_flush  :  1; // A write occurred tio a CQ that was being drained. The write did not occur
        uint64_t    qword_cnt_err_mbe  :  1; // QWORD Count memory tracking MBE error
        uint64_t    qword_cnt_err_sbe  :  1; // QWORD Count memory tracking SBE error
        uint64_t cqslot_full_vec_error  :  1; // Parity error in the Slot full tracking logic
        uint64_t cqslot_len_vec_error  :  1; // Parity error in the command length tracking logic
        uint64_t           cq_timeout  :  1; // CQ timeout has occurred
        uint64_t     qword_cnt_ovrflw  :  1; // QWORD count overflow
        uint64_t     head_ptr_par_err  :  1; // Head pointer parity error
        uint64_t        Reserved_63_8  : 56; // Unused
    } field;
    uint64_t val;
} RXCIC_ERR_FIRST_BMC_t;

// RXCIC_ERR_EN_QUAR desc:
typedef union {
    struct {
        uint64_t      inv_write_flush  :  1; // A write occurred tio a CQ that was being drained. The write did not occur
        uint64_t    qword_cnt_err_mbe  :  1; // QWORD Count memory tracking MBE error
        uint64_t    qword_cnt_err_sbe  :  1; // QWORD Count memory tracking SBE error
        uint64_t cqslot_full_vec_error  :  1; // Parity error in the Slot full tracking logic
        uint64_t cqslot_len_vec_error  :  1; // Parity error in the command length tracking logic
        uint64_t           cq_timeout  :  1; // CQ timeout has occurred
        uint64_t     qword_cnt_ovrflw  :  1; // QWORD count overflow
        uint64_t     head_ptr_par_err  :  1; // Head pointer parity error
        uint64_t        Reserved_63_8  : 56; // Unused
    } field;
    uint64_t val;
} RXCIC_ERR_EN_QUAR_t;

// RXCIC_ERR_FIRST_QUAR desc:
typedef union {
    struct {
        uint64_t      inv_write_flush  :  1; // A write occurred tio a CQ that was being drained. The write did not occur
        uint64_t    qword_cnt_err_mbe  :  1; // QWORD Count memory tracking MBE error
        uint64_t    qword_cnt_err_sbe  :  1; // QWORD Count memory tracking SBE error
        uint64_t cqslot_full_vec_error  :  1; // Parity error in the Slot full tracking logic
        uint64_t cqslot_len_vec_error  :  1; // Parity error in the command length tracking logic
        uint64_t           cq_timeout  :  1; // CQ timeout has occurred
        uint64_t     qword_cnt_ovrflw  :  1; // QWORD count overflow
        uint64_t     head_ptr_par_err  :  1; // Head pointer parity error
        uint64_t        Reserved_63_8  : 56; // Unused
    } field;
    uint64_t val;
} RXCIC_ERR_FIRST_QUAR_t;

// RXCIC_ERROR_INFO_SBE_MBE desc:
typedef union {
    struct {
        uint64_t syndrome_qword_cnt_mem_mbe  :  8; // Syndrome of the last MBE for QWORD count memory
        uint64_t syndrome_qword_cnt_mem_sbe  :  8; // Syndrome of the last SBE QWORD count memory
        uint64_t cq_num_inv_write_flush  :  8; // CQ number for the last invalid write flush
        uint64_t slot_num_inv_write_flush  :  4; // slot number for the last invalid write flush
        uint64_t       Reserved_31_28  :  4; // Unused
        uint64_t                  cnt  :  8; // Saturating counter of MBE's from all sources
        uint64_t       Reserved_63_40  : 24; // Unused
    } field;
    uint64_t val;
} RXCIC_ERROR_INFO_SBE_MBE_t;

// RXCIC_ERR_PER_CQ_INFO_GENERAL desc:
typedef union {
    struct {
        uint64_t           Reserved_0  :  1; // Unused
        uint64_t    qword_cnt_err_mbe  :  1; // QWORD Count memory tracking MBE error
        uint64_t           Reserved_2  :  1; // Unused
        uint64_t cqslot_full_vec_error  :  1; // Parity error in the Slot full tracking logic
        uint64_t cqslot_len_vec_error  :  1; // Parity error in the command length tracking logic
        uint64_t           cq_timeout  :  1; // CQ timeout has occurred
        uint64_t     qword_cnt_ovrflw  :  1; // QWORD count overflow
        uint64_t     head_ptr_par_err  :  1; // Head pointer parity error
        uint64_t        Reserved_63_8  : 56; // Unused
    } field;
    uint64_t val;
} RXCIC_ERR_PER_CQ_INFO_GENERAL_t;

// RXCIC_DBG_ERR_INJECT desc:
typedef union {
    struct {
        uint64_t inject_qword_cnt_err_mask  :  4; // Error injection mask for QWORD count memory ECC error
        uint64_t inject_qword_cnt_err  :  1; // Error injection enable for QWORD count memory ECC error
        uint64_t        Reserved_63_5  : 59; // Unused
    } field;
    uint64_t val;
} RXCIC_DBG_ERR_INJECT_t;

#endif /* ___FXR_rx_ci_cic_CSRS_H__ */