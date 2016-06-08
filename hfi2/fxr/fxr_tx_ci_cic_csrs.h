// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Thu Jun  2 19:11:24 2016
//

#ifndef ___FXR_tx_ci_cic_CSRS_H__
#define ___FXR_tx_ci_cic_CSRS_H__

// TXCIC_CFG_DRAIN_RESET desc:
typedef union {
    struct {
        uint64_t             drain_cq  :  8; // CQ for drain and reset operation
        uint64_t                drain  :  1; // Drain the CQ of commands
        uint64_t                reset  :  1; // Reset the CQ
        uint64_t                 busy  :  1; // Drain or reset operation active. Set by hardware when this CSR is written, cleared when the specified operations finish.
        uint64_t                  rsv  : 52; // Reserved
    } field;
    uint64_t val;
} TXCIC_CFG_DRAIN_RESET_t;

// TXCIC_CFG_HEAD_UPDATE_CNTRL desc:
typedef union {
    struct {
        uint64_t            rate_ctrl  :  3; // Head pointer update frequency control, sets number of head pointer increments before head pointer is updated in memory. Value of 0, 1, 2, 3 , 4,5,6 and 7 correspond to updating on increments of 1, 2, 4, 8, 16,32, 64 and 128.
        uint64_t                  rsv  : 61; // Reserved
    } field;
    uint64_t val;
} TXCIC_CFG_HEAD_UPDATE_CNTRL_t;

// TXCIC_STS_TAIL desc:
typedef union {
    struct {
        uint64_t                 tail  :  7; // Command queue tail slot
        uint64_t                  rsv  : 57; // Reserved
    } field;
    uint64_t val;
} TXCIC_STS_TAIL_t;

// TXCIC_ARBITRATION_DISABLE desc:
typedef union {
    struct {
        uint64_t           disable_tc  :  4; // Disable TC 3:0 arbitration
        uint64_t                  rsv  : 60; // Reserved
    } field;
    uint64_t val;
} TXCIC_ARBITRATION_DISABLE_t;

// TXCIC_CFG_TO_LIMIT desc:
typedef union {
    struct {
        uint64_t                  rsv  : 20; // Reserved, timeout value is limited to increments of one million
        uint64_t             to_limit  : 17; // Timeout value, in number of FXR clock ticks
        uint64_t               rsv_63  : 27; // Reserved
    } field;
    uint64_t val;
} TXCIC_CFG_TO_LIMIT_t;

// TXCIC_ERR_STS desc:
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
} TXCIC_ERR_STS_t;

// TXCIC_ERR_CLR desc:
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
} TXCIC_ERR_CLR_t;

// TXCIC_ERR_FRC desc:
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
} TXCIC_ERR_FRC_t;

// TXCIC_ERR_EN_HOST desc:
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
} TXCIC_ERR_EN_HOST_t;

// TXCIC_ERR_FIRST_HOST desc:
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
} TXCIC_ERR_FIRST_HOST_t;

// TXCIC_ERR_EN_BMC desc:
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
} TXCIC_ERR_EN_BMC_t;

// TXCIC_ERR_FIRST_BMC desc:
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
} TXCIC_ERR_FIRST_BMC_t;

// TXCIC_ERR_EN_QUAR desc:
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
} TXCIC_ERR_EN_QUAR_t;

// TXCIC_ERR_FIRST_QUAR desc:
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
} TXCIC_ERR_FIRST_QUAR_t;

// TXCIC_ERROR_INFO_SBE_MBE desc:
typedef union {
    struct {
        uint64_t syndrome_qword_cnt_mem_mbe  :  8; // Syndrome of the last MBE for QWORD count memory
        uint64_t syndrome_qword_cnt_mem_sbe  :  8; // Syndrome of the last SBE QWORD count memory
        uint64_t cq_num_inv_write_flush  :  8; // CQ number for the last invalid write flush
        uint64_t slot_num_inv_write_flush  :  4; // slot number for the last invalid write flush
        uint64_t       Reserved_63_28  : 36; // Unused
    } field;
    uint64_t val;
} TXCIC_ERROR_INFO_SBE_MBE_t;

// TXCIC_ERR_PER_CQ_INFO_GENERAL desc:
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
} TXCIC_ERR_PER_CQ_INFO_GENERAL_t;

// TXCIC_DBG_ERR_INJECT desc:
typedef union {
    struct {
        uint64_t inject_qword_cnt_err_mask  :  4; // Error injection mask for QWORD count memory ECC error
        uint64_t inject_qword_cnt_err  :  1; // Error injection enable for QWORD count memory ECC error
        uint64_t        Reserved_63_5  : 59; // Unused
    } field;
    uint64_t val;
} TXCIC_DBG_ERR_INJECT_t;

#endif /* ___FXR_tx_ci_cic_CSRS_H__ */