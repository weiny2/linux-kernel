// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Wed Jun 22 19:30:15 2016
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
        uint64_t vlf_txe_cfg_par_error  :  1; // Length fifo parity error
        uint64_t vlff_txe_cfg_sm_par_error  :  1; // Length fifo fsm parity error
        uint64_t   timeout_sm_par_err  :  1; // Timeout FSM parity error
        uint64_t         overflow_err  :  1; // Overflow error
        uint64_t     pcc_head_sop_sbe  :  1; // Head sop sbe error
        uint64_t     pcc_head_sop_mbe  :  1; // Head sop mbe error
        uint64_t vl_len_mem_arb_pcvlarb_q2_rdata_ecc_sbe  :  1; // Length memory arbiter read data sbe error
        uint64_t vl_len_mem_arb_pcvlarb_q2_rdata_ecc_mbe  :  1; // Length memory arbiter read data mbe error
        uint64_t pkt_ctrl_fifo_rdata_sbe  :  1; // Packet fifo read data sbe error
        uint64_t pkt_ctrl_fifo_rdata_mbe  :  1; // Packet fifo read data mbe error
        uint64_t      inv_write_flush  :  1; // A write occurred tio a CQ that was being drained. The write did not occur
        uint64_t    qword_cnt_err_mbe  :  1; // QWORD Count memory tracking MBE error
        uint64_t    qword_cnt_err_sbe  :  1; // QWORD Count memory tracking SBE error
        uint64_t       Reserved_63_13  : 51; // Reserved
    } field;
    uint64_t val;
} TXCIC_ERR_STS_t;

// TXCIC_ERR_CLR desc:
typedef union {
    struct {
        uint64_t vlf_txe_cfg_par_error  :  1; // Length fifo parity error
        uint64_t vlff_txe_cfg_sm_par_error  :  1; // Length fifo fsm parity error
        uint64_t   timeout_sm_par_err  :  1; // Timeout FSM parity error
        uint64_t         overflow_err  :  1; // Overflow error
        uint64_t     pcc_head_sop_sbe  :  1; // Head sop sbe error
        uint64_t     pcc_head_sop_mbe  :  1; // Head sop mbe error
        uint64_t vl_len_mem_arb_pcvlarb_q2_rdata_ecc_sbe  :  1; // Length memory arbiter read data sbe error
        uint64_t vl_len_mem_arb_pcvlarb_q2_rdata_ecc_mbe  :  1; // Length memory arbiter read data mbe error
        uint64_t pkt_ctrl_fifo_rdata_sbe  :  1; // Packet fifo read data sbe error
        uint64_t pkt_ctrl_fifo_rdata_mbe  :  1; // Packet fifo read data mbe error
        uint64_t      inv_write_flush  :  1; // A write occurred tio a CQ that was being drained. The write did not occur
        uint64_t    qword_cnt_err_mbe  :  1; // QWORD Count memory tracking MBE error
        uint64_t    qword_cnt_err_sbe  :  1; // QWORD Count memory tracking SBE error
        uint64_t       Reserved_63_13  : 51; // Reserved
    } field;
    uint64_t val;
} TXCIC_ERR_CLR_t;

// TXCIC_ERR_FRC desc:
typedef union {
    struct {
        uint64_t vlf_txe_cfg_par_error  :  1; // Length fifo parity error
        uint64_t vlff_txe_cfg_sm_par_error  :  1; // Length fifo fsm parity error
        uint64_t   timeout_sm_par_err  :  1; // Timeout FSM parity error
        uint64_t         overflow_err  :  1; // Overflow error
        uint64_t     pcc_head_sop_sbe  :  1; // Head sop sbe error
        uint64_t     pcc_head_sop_mbe  :  1; // Head sop mbe error
        uint64_t vl_len_mem_arb_pcvlarb_q2_rdata_ecc_sbe  :  1; // Length memory arbiter read data sbe error
        uint64_t vl_len_mem_arb_pcvlarb_q2_rdata_ecc_mbe  :  1; // Length memory arbiter read data mbe error
        uint64_t pkt_ctrl_fifo_rdata_sbe  :  1; // Packet fifo read data sbe error
        uint64_t pkt_ctrl_fifo_rdata_mbe  :  1; // Packet fifo read data mbe error
        uint64_t      inv_write_flush  :  1; // A write occurred tio a CQ that was being drained. The write did not occur
        uint64_t    qword_cnt_err_mbe  :  1; // QWORD Count memory tracking MBE error
        uint64_t    qword_cnt_err_sbe  :  1; // QWORD Count memory tracking SBE error
        uint64_t       Reserved_63_13  : 51; // Reserved
    } field;
    uint64_t val;
} TXCIC_ERR_FRC_t;

// TXCIC_ERR_EN_HOST desc:
typedef union {
    struct {
        uint64_t vlf_txe_cfg_par_error  :  1; // Length fifo parity error
        uint64_t vlff_txe_cfg_sm_par_error  :  1; // Length fifo fsm parity error
        uint64_t   timeout_sm_par_err  :  1; // Timeout FSM parity error
        uint64_t         overflow_err  :  1; // Overflow error
        uint64_t     pcc_head_sop_sbe  :  1; // Head sop sbe error
        uint64_t     pcc_head_sop_mbe  :  1; // Head sop mbe error
        uint64_t vl_len_mem_arb_pcvlarb_q2_rdata_ecc_sbe  :  1; // Length memory arbiter read data sbe error
        uint64_t vl_len_mem_arb_pcvlarb_q2_rdata_ecc_mbe  :  1; // Length memory arbiter read data mbe error
        uint64_t pkt_ctrl_fifo_rdata_sbe  :  1; // Packet fifo read data sbe error
        uint64_t pkt_ctrl_fifo_rdata_mbe  :  1; // Packet fifo read data mbe error
        uint64_t      inv_write_flush  :  1; // A write occurred tio a CQ that was being drained. The write did not occur
        uint64_t    qword_cnt_err_mbe  :  1; // QWORD Count memory tracking MBE error
        uint64_t    qword_cnt_err_sbe  :  1; // QWORD Count memory tracking SBE error
        uint64_t       Reserved_63_13  : 51; // Reserved
    } field;
    uint64_t val;
} TXCIC_ERR_EN_HOST_t;

// TXCIC_ERR_FIRST_HOST desc:
typedef union {
    struct {
        uint64_t vlf_txe_cfg_par_error  :  1; // Length fifo parity error
        uint64_t vlff_txe_cfg_sm_par_error  :  1; // Length fifo fsm parity error
        uint64_t   timeout_sm_par_err  :  1; // Timeout FSM parity error
        uint64_t         overflow_err  :  1; // Overflow error
        uint64_t     pcc_head_sop_sbe  :  1; // Head sop sbe error
        uint64_t     pcc_head_sop_mbe  :  1; // Head sop mbe error
        uint64_t vl_len_mem_arb_pcvlarb_q2_rdata_ecc_sbe  :  1; // Length memory arbiter read data sbe error
        uint64_t vl_len_mem_arb_pcvlarb_q2_rdata_ecc_mbe  :  1; // Length memory arbiter read data mbe error
        uint64_t pkt_ctrl_fifo_rdata_sbe  :  1; // Packet fifo read data sbe error
        uint64_t pkt_ctrl_fifo_rdata_mbe  :  1; // Packet fifo read data mbe error
        uint64_t      inv_write_flush  :  1; // A write occurred tio a CQ that was being drained. The write did not occur
        uint64_t    qword_cnt_err_mbe  :  1; // QWORD Count memory tracking MBE error
        uint64_t    qword_cnt_err_sbe  :  1; // QWORD Count memory tracking SBE error
        uint64_t       Reserved_63_13  : 51; // Reserved
    } field;
    uint64_t val;
} TXCIC_ERR_FIRST_HOST_t;

// TXCIC_ERR_EN_BMC desc:
typedef union {
    struct {
        uint64_t vlf_txe_cfg_par_error  :  1; // Length fifo parity error
        uint64_t vlff_txe_cfg_sm_par_error  :  1; // Length fifo fsm parity error
        uint64_t   timeout_sm_par_err  :  1; // Timeout FSM parity error
        uint64_t         overflow_err  :  1; // Overflow error
        uint64_t     pcc_head_sop_sbe  :  1; // Head sop sbe error
        uint64_t     pcc_head_sop_mbe  :  1; // Head sop mbe error
        uint64_t vl_len_mem_arb_pcvlarb_q2_rdata_ecc_sbe  :  1; // Length memory arbiter read data sbe error
        uint64_t vl_len_mem_arb_pcvlarb_q2_rdata_ecc_mbe  :  1; // Length memory arbiter read data mbe error
        uint64_t pkt_ctrl_fifo_rdata_sbe  :  1; // Packet fifo read data sbe error
        uint64_t pkt_ctrl_fifo_rdata_mbe  :  1; // Packet fifo read data mbe error
        uint64_t      inv_write_flush  :  1; // A write occurred tio a CQ that was being drained. The write did not occur
        uint64_t    qword_cnt_err_mbe  :  1; // QWORD Count memory tracking MBE error
        uint64_t    qword_cnt_err_sbe  :  1; // QWORD Count memory tracking SBE error
        uint64_t       Reserved_63_13  : 51; // Reserved
    } field;
    uint64_t val;
} TXCIC_ERR_EN_BMC_t;

// TXCIC_ERR_FIRST_BMC desc:
typedef union {
    struct {
        uint64_t vlf_txe_cfg_par_error  :  1; // Length fifo parity error
        uint64_t vlff_txe_cfg_sm_par_error  :  1; // Length fifo fsm parity error
        uint64_t   timeout_sm_par_err  :  1; // Timeout FSM parity error
        uint64_t         overflow_err  :  1; // Overflow error
        uint64_t     pcc_head_sop_sbe  :  1; // Head sop sbe error
        uint64_t     pcc_head_sop_mbe  :  1; // Head sop mbe error
        uint64_t vl_len_mem_arb_pcvlarb_q2_rdata_ecc_sbe  :  1; // Length memory arbiter read data sbe error
        uint64_t vl_len_mem_arb_pcvlarb_q2_rdata_ecc_mbe  :  1; // Length memory arbiter read data mbe error
        uint64_t pkt_ctrl_fifo_rdata_sbe  :  1; // Packet fifo read data sbe error
        uint64_t pkt_ctrl_fifo_rdata_mbe  :  1; // Packet fifo read data mbe error
        uint64_t      inv_write_flush  :  1; // A write occurred tio a CQ that was being drained. The write did not occur
        uint64_t    qword_cnt_err_mbe  :  1; // QWORD Count memory tracking MBE error
        uint64_t    qword_cnt_err_sbe  :  1; // QWORD Count memory tracking SBE error
        uint64_t       Reserved_63_13  : 51; // Reserved
    } field;
    uint64_t val;
} TXCIC_ERR_FIRST_BMC_t;

// TXCIC_ERR_EN_QUAR desc:
typedef union {
    struct {
        uint64_t vlf_txe_cfg_par_error  :  1; // Length fifo parity error
        uint64_t vlff_txe_cfg_sm_par_error  :  1; // Length fifo fsm parity error
        uint64_t   timeout_sm_par_err  :  1; // Timeout FSM parity error
        uint64_t         overflow_err  :  1; // Overflow error
        uint64_t     pcc_head_sop_sbe  :  1; // Head sop sbe error
        uint64_t     pcc_head_sop_mbe  :  1; // Head sop mbe error
        uint64_t vl_len_mem_arb_pcvlarb_q2_rdata_ecc_sbe  :  1; // Length memory arbiter read data sbe error
        uint64_t vl_len_mem_arb_pcvlarb_q2_rdata_ecc_mbe  :  1; // Length memory arbiter read data mbe error
        uint64_t pkt_ctrl_fifo_rdata_sbe  :  1; // Packet fifo read data sbe error
        uint64_t pkt_ctrl_fifo_rdata_mbe  :  1; // Packet fifo read data mbe error
        uint64_t      inv_write_flush  :  1; // A write occurred tio a CQ that was being drained. The write did not occur
        uint64_t    qword_cnt_err_mbe  :  1; // QWORD Count memory tracking MBE error
        uint64_t    qword_cnt_err_sbe  :  1; // QWORD Count memory tracking SBE error
        uint64_t       Reserved_63_13  : 51; // Reserved
    } field;
    uint64_t val;
} TXCIC_ERR_EN_QUAR_t;

// TXCIC_ERR_FIRST_QUAR desc:
typedef union {
    struct {
        uint64_t vlf_txe_cfg_par_error  :  1; // Length fifo parity error
        uint64_t vlff_txe_cfg_sm_par_error  :  1; // Length fifo fsm parity error
        uint64_t   timeout_sm_par_err  :  1; // Timeout FSM parity error
        uint64_t         overflow_err  :  1; // Overflow error
        uint64_t     pcc_head_sop_sbe  :  1; // Head sop sbe error
        uint64_t     pcc_head_sop_mbe  :  1; // Head sop mbe error
        uint64_t vl_len_mem_arb_pcvlarb_q2_rdata_ecc_sbe  :  1; // Length memory arbiter read data sbe error
        uint64_t vl_len_mem_arb_pcvlarb_q2_rdata_ecc_mbe  :  1; // Length memory arbiter read data mbe error
        uint64_t pkt_ctrl_fifo_rdata_sbe  :  1; // Packet fifo read data sbe error
        uint64_t pkt_ctrl_fifo_rdata_mbe  :  1; // Packet fifo read data mbe error
        uint64_t      inv_write_flush  :  1; // A write occurred tio a CQ that was being drained. The write did not occur
        uint64_t    qword_cnt_err_mbe  :  1; // QWORD Count memory tracking MBE error
        uint64_t    qword_cnt_err_sbe  :  1; // QWORD Count memory tracking SBE error
        uint64_t       Reserved_63_13  : 51; // Reserved
    } field;
    uint64_t val;
} TXCIC_ERR_FIRST_QUAR_t;

// TXCIC_ERROR_INFO_SBE_MBE desc:
typedef union {
    struct {
        uint64_t syndrome_pcc_head_sop_mbe  :  5; // Syndrome of the first MBE for head sop
        uint64_t syndrome_len_mem_arb_pcvlarb_q2_rdata_ecc_mbe  :  5; // Syndrome of the first MBE for length arbiter read data memory
        uint64_t syndrome_pkt_ctrl_fifo_rdata_mbe  :  6; // Syndrome of the first MBE for the packet control fifo read data
        uint64_t pcc_head_sop_mbe_set  :  6; // Set number of the first MBE for head sop
        uint64_t len_mem_arb_pcvlarb_q2_rdata_ecc_mbe_set  :  6; // Set number of the first MBE for length arbiter read data memory
        uint64_t pkt_ctrl_fifo_rdata_mbe_tc  :  4; // Traffic class of the first MBE for the packet control fifo read data
        uint64_t       Reserved_63_32  : 32; // Unused
    } field;
    uint64_t val;
} TXCIC_ERROR_INFO_SBE_MBE_t;

// TXCIC_ERR_PER_CQ_INFO_GENERAL desc:
typedef union {
    struct {
        uint64_t vlf_txe_cfg_par_error  :  1; // Length fifo parity error
        uint64_t vlff_txe_cfg_sm_par_error  :  1; // Length fifo fsm parity error
        uint64_t   timeout_sm_par_err  :  1; // Timeout FSM parity error
        uint64_t         overflow_err  :  1; // Overflow error
        uint64_t     pcc_head_sop_mbe  :  1; // Head sop mbe error
        uint64_t vl_len_mem_arb_pcvlarb_q2_rdata_ecc_mbe  :  1; // Length memory arbiter read data mbe error
        uint64_t pkt_ctrl_fifo_rdata_mbe  :  1; // Packet fifo read data mbe error
        uint64_t      inv_write_flush  :  1; // A write occurred to a CQ that was being drained. The write did not occur
        uint64_t    qword_cnt_err_mbe  :  1; // QWORD Count memory tracking MBE error
        uint64_t        ctxt_in_error  :  1; // Context in error and is currently stalled
        uint64_t       Reserved_63_10  : 54; // Unused
    } field;
    uint64_t val;
} TXCIC_ERR_PER_CQ_INFO_GENERAL_t;

// TXCIC_DBG_ERR_INJECT desc:
typedef union {
    struct {
        uint64_t inject_pcc_head_sop_err_mask  :  5; // Error injection mask for head sop memory ECC error
        uint64_t inject_pcc_head_sop_err  :  1; // Error injection enable for head sop memory ECC error
        uint64_t inject_vl_len_mem_arb_pcvlarb_q2_rdata_ecc_mask  :  5; // Error injection mask for length memory arbiter ECC error
        uint64_t inject_vl_len_mem_arb_pcvlarb_q2_rdata_ecc  :  1; // Error injection enable for length memory arbiter ECC error
        uint64_t inject_pkt_ctrl_fifo_rdata_mask  :  6; // Error injection mask for packet control fifo ECC error
        uint64_t inject_pkt_ctrl_fifo_rdata  :  1; // Error injection enable for packet control fifo ECC error
        uint64_t inject_qword_cnt_err_mask  :  4; // Error injection mask for QWORD count memory ECC error
        uint64_t inject_qword_cnt_err  :  1; // Error injection enable for QWORD count memory ECC error
        uint64_t       Reserved_63_24  : 40; // Unused
    } field;
    uint64_t val;
} TXCIC_DBG_ERR_INJECT_t;

#endif /* ___FXR_tx_ci_cic_CSRS_H__ */