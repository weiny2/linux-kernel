// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Fri May 26 13:04:25 2017
//

#ifndef ___FXR_tx_ci_cic_CSRS_H__
#define ___FXR_tx_ci_cic_CSRS_H__

// TXCIC_CFG_DRAIN_RESET desc:
typedef union {
    struct {
        uint64_t             drain_cq  :  8; // CQ for drain and reset operation
        uint64_t                drain  :  1; // Drain the CQ of commands.
        uint64_t                reset  :  1; // Reset the CQ. A drain is performed prior to the reset, regardless of whether the drain bit is set.
        uint64_t                 busy  :  1; // Drain or reset operation active. Set by hardware when this CSR is written, cleared when the specified operations finish.
        uint64_t                  rsv  : 52; // Reserved
    } field;
    uint64_t val;
} TXCIC_CFG_DRAIN_RESET_t;

// TXCIC_CFG_HEAD_UPDATE_CNTRL desc:
typedef union {
    struct {
        uint64_t            rate_ctrl  :  3; // Head pointer update frequency control. This register sets the number of head pointer increments that must occur before a head pointer update is scheduled. If multiple updates are scheduled before the update write is sent, they are combined. When the update write occurs, the current value of the head pointer is used, which may be more recent than the value that caused the update to be scheduled. Values of 0, 1, 2, 3, 4, 5, 6, and 7 correspond to updating on increments of 1, 2, 4, 8, 16, 32, 64, and 128.
        uint64_t                  rsv  : 61; // Reserved
    } field;
    uint64_t val;
} TXCIC_CFG_HEAD_UPDATE_CNTRL_t;

// TXCIC_STS_TAIL desc:
typedef union {
    struct {
        uint64_t                 tail  :  7; // Command queue tail slot
        uint64_t                  rsv  :  1; // Reserved
        uint64_t                 head  :  7; // Command queue head slot
        uint64_t               rsv_63  : 49; // Reserved
    } field;
    uint64_t val;
} TXCIC_STS_TAIL_t;

// TXCIC_ARBITRATION_DISABLE desc:
typedef union {
    struct {
        uint64_t           disable_tc  :  4; // Disable TC 3:0 arbitration. Bit[x] = 1 TCx will not participate in arbitration (X =0, 1, 2, 3)
        uint64_t                  rsv  : 60; // Reserved
    } field;
    uint64_t val;
} TXCIC_ARBITRATION_DISABLE_t;

// TXCIC_CFG_TO_LIMIT desc:
typedef union {
    struct {
        uint64_t                  rsv  : 20; // Reserved, timeout value is limited to increments of one million
        uint64_t             to_limit  : 17; // Timeout value, in number of FXR clock ticks. Timeout will happen when the total packet is not completed for {to_limit, 20'h04000} clocks, defalult is 1M + 16000 clocks.
        uint64_t               rsv_63  : 27; // Reserved
    } field;
    uint64_t val;
} TXCIC_CFG_TO_LIMIT_t;

// TXCIC_ERR_STS desc:
typedef union {
    struct {
        uint64_t vlf_txe_cfg_par_error  :  1; // Length fifo parity error ERR_CATEGORY_PROCESS
        uint64_t vlff_txe_cfg_sm_par_error  :  1; // Length fifo fsm parity error ERR_CATEGORY_PROCESS
        uint64_t   timeout_sm_par_err  :  1; // Timeout FSM parity error ERR_CATEGORY_PROCESS
        uint64_t         overflow_err  :  1; // Overflow error ERR_CATEGORY_PROCESS
        uint64_t     pcc_head_sop_sbe  :  1; // Head sop sbe error
        uint64_t     pcc_head_sop_mbe  :  1; // Head sop mbe error ERR_CATEGORY_PROCESS
        uint64_t vl_len_mem_arb_pcvlarb_q2_rdata_ecc_sbe  :  1; // Length memory arbiter read data sbe error ERR_CATEGORY_CORRECTABLE
        uint64_t vl_len_mem_arb_pcvlarb_q2_rdata_ecc_mbe  :  1; // Length memory arbiter read data mbe error ERR_CATEGORY_PROCESS
        uint64_t pkt_ctrl_fifo_rdata_sbe  :  1; // Packet fifo read data sbe error ERR_CATEGORY_CORRECTABLE
        uint64_t pkt_ctrl_fifo_rdata_mbe  :  1; // Packet fifo read data mbe error ERR_CATEGORY_PROCESS
        uint64_t      inv_write_flush  :  1; // A write occurred tio a CQ that was being drained. The write did not occur ERR_CATEGORY_INFO
        uint64_t    qword_cnt_err_mbe  :  1; // QWORD Count memory tracking MBE error ERR_CATEGORY_PROCESS
        uint64_t    qword_cnt_err_sbe  :  1; // QWORD Count memory tracking SBE errorERR_CATEGORY_CORRECTABLE
        uint64_t         ctxt_timeout  :  1; // Command queue is timed out, The timeout infor will be automatically reset after all the slots of the timeout packet is received by the HFI. Once the last slot of the packet is received it will send the head update for the last slot. The error status can be reset after that. ERR_CATEGORY_INFO
        uint64_t       Reserved_63_14  : 50; // Unused
    } field;
    uint64_t val;
} TXCIC_ERR_STS_t;

// TXCIC_ERR_CLR desc:
typedef union {
    struct {
        uint64_t                value  : 14; // error clear value
        uint64_t       Reserved_63_14  : 50; // Unused
    } field;
    uint64_t val;
} TXCIC_ERR_CLR_t;

// TXCIC_ERR_FRC desc:
typedef union {
    struct {
        uint64_t                value  : 14; // error force value
        uint64_t       Reserved_63_14  : 50; // Unused
    } field;
    uint64_t val;
} TXCIC_ERR_FRC_t;

// TXCIC_ERR_EN_HOST desc:
typedef union {
    struct {
        uint64_t                value  : 14; // error enable host value
        uint64_t       Reserved_63_14  : 50; // Unused
    } field;
    uint64_t val;
} TXCIC_ERR_EN_HOST_t;

// TXCIC_ERR_FIRST_HOST desc:
typedef union {
    struct {
        uint64_t                value  : 14; // Error first host value
        uint64_t       Reserved_63_14  : 50; // Unused
    } field;
    uint64_t val;
} TXCIC_ERR_FIRST_HOST_t;

// TXCIC_ERR_EN_BMC desc:
typedef union {
    struct {
        uint64_t                value  : 14; // Error enable bmc value
        uint64_t       Reserved_63_14  : 50; // Unused
    } field;
    uint64_t val;
} TXCIC_ERR_EN_BMC_t;

// TXCIC_ERR_FIRST_BMC desc:
typedef union {
    struct {
        uint64_t                value  : 14; // error first bmc value
        uint64_t       Reserved_63_14  : 50; // Unused
    } field;
    uint64_t val;
} TXCIC_ERR_FIRST_BMC_t;

// TXCIC_ERR_EN_QUAR desc:
typedef union {
    struct {
        uint64_t                value  : 14; // Error enable quarantine value
        uint64_t       Reserved_63_14  : 50; // Unused
    } field;
    uint64_t val;
} TXCIC_ERR_EN_QUAR_t;

// TXCIC_ERR_FIRST_QUAR desc:
typedef union {
    struct {
        uint64_t                value  : 14; // Error first quarantine value
        uint64_t       Reserved_63_14  : 50; // Unused
    } field;
    uint64_t val;
} TXCIC_ERR_FIRST_QUAR_t;

// TXCIC_ERROR_INFO_SBE_MBE_0 desc:
typedef union {
    struct {
        uint64_t   inv_write_flush_cq  :  8; // CQ number for the first invalid flush error
        uint64_t inv_write_flush_addr  :  7; // Slot number for the first invalid flush error
        uint64_t qword_cnt_mem_sbe_addr  :  7; // Slot number for the first Qword count memory SBE error
        uint64_t syndrome_qword_cnt_mem_sbe  :  4; // Syndrome number for the first Qword count memory SBE error
        uint64_t pcc_head_sop_sbe_set  :  6; // Set number of the first SBE for head sop
        uint64_t len_mem_arb_pcvlarb_q2_rdata_ecc_sbe_set  :  7; // Set number of the first SBE for length arbiter read data memory
        uint64_t pkt_ctrl_fifo_rdata_sbe_tc  :  4; // Traffic class of the first SBE for the packet control fifo read data
        uint64_t                  cnt  :  8; // Saturation counter for MBE's from all sources
        uint64_t       Reserved_63_51  : 13; // Unused
    } field;
    uint64_t val;
} TXCIC_ERROR_INFO_SBE_MBE_0_t;

// TXCIC_ERROR_INFO_SBE_MBE_1 desc:
typedef union {
    struct {
        uint64_t syndrome_pcc_head_sop_sbe_0  :  5; // Syndrome of the first SBE for head sop for set number 0
        uint64_t syndrome_pcc_head_sop_sbe_1  :  5; // Syndrome of the first SBE for head sop for set number 1
        uint64_t syndrome_pcc_head_sop_sbe_2  :  5; // Syndrome of the first SBE for head sop for set number 2
        uint64_t syndrome_pcc_head_sop_sbe_3  :  5; // Syndrome of the first SBE for head sop for set number 3
        uint64_t syndrome_pcc_head_sop_sbe_4  :  5; // Syndrome of the first SBE for head sop for set number 4
        uint64_t syndrome_pcc_head_sop_sbe_5  :  5; // Syndrome of the first SBE for head sop for set number 5
        uint64_t syndrome_len_mem_arb_pcvlarb_q2_rdata_ecc_sbe_0  :  5; // Syndrome of the first SBE for length arbiter read data memory for set number 0
        uint64_t syndrome_len_mem_arb_pcvlarb_q2_rdata_ecc_sbe_1  :  5; // Syndrome of the first SBE for length arbiter read data memory for set number 1
        uint64_t syndrome_len_mem_arb_pcvlarb_q2_rdata_ecc_sbe_2  :  5; // Syndrome of the first SBE for length arbiter read data memory for set number 2
        uint64_t syndrome_len_mem_arb_pcvlarb_q2_rdata_ecc_sbe_3  :  5; // Syndrome of the first SBE for length arbiter read data memory for set number 3
        uint64_t syndrome_len_mem_arb_pcvlarb_q2_rdata_ecc_sbe_4  :  5; // Syndrome of the first SBE for length arbiter read data memory for set number 4
        uint64_t syndrome_len_mem_arb_pcvlarb_q2_rdata_ecc_sbe_5  :  5; // Syndrome of the first SBE for length arbiter read data memory for set number 5
        uint64_t       Reserved_63_60  :  4; // Unused
    } field;
    uint64_t val;
} TXCIC_ERROR_INFO_SBE_MBE_1_t;

// TXCIC_ERROR_INFO_SBE_MBE_2 desc:
typedef union {
    struct {
        uint64_t syndrome_pkt_ctrl_fifo_rdata_sbe_0  :  6; // Syndrome of the first SBE for the packet control fifo read data for TC 0
        uint64_t syndrome_pkt_ctrl_fifo_rdata_sbe_1  :  6; // Syndrome of the first SBE for the packet control fifo read data for TC 1
        uint64_t syndrome_pkt_ctrl_fifo_rdata_sbe_2  :  6; // Syndrome of the first SBE for the packet control fifo read data for TC 0
        uint64_t syndrome_pkt_ctrl_fifo_rdata_sbe_3  :  6; // Syndrome of the first SBE for the packet control fifo read data for TC 0
        uint64_t       Reserved_63_24  : 40; // Unused
    } field;
    uint64_t val;
} TXCIC_ERROR_INFO_SBE_MBE_2_t;

// TXCIC_ENC_ERR_CTXT desc:
typedef union {
    struct {
        uint64_t         err_ctxt_num  :  8; // If multiple context in error, lowest context will be reported. This filed is valid only with the bit 8 of the register.
        uint64_t      any_ctxt_in_err  :  1; // This bit indicate if any context is in error
        uint64_t     timeout_ctxt_num  :  8; // If multiple context in timeout, lowest context will be reported. This filed is valid only with the bit 17 of the register.
        uint64_t     any_ctxt_timeout  :  1; // This bit indicate any context is timed out
        uint64_t       Reserved_63_18  : 46; // Unused
    } field;
    uint64_t val;
} TXCIC_ENC_ERR_CTXT_t;

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
        uint64_t    qword_cnt_err_mbe  :  1; // QWORD Count memory tracking MBE error
        uint64_t        ctxt_in_error  :  1; // Context in error and is currently stalled
        uint64_t         ctxt_timeout  :  1; // Command queue is timed out
        uint64_t       Reserved_63_10  : 54; // Unused
    } field;
    uint64_t val;
} TXCIC_ERR_PER_CQ_INFO_GENERAL_t;

// TXCIC_DBG_ERR_INJECT desc:
typedef union {
    struct {
        uint64_t inject_pcc_head_sop_err_mask  :  5; // Error injection mask for head sop memory ECC error
        uint64_t inject_pcc_head_sop_err  :  6; // Error injection enable for head sop memory ECC error
        uint64_t inject_vl_len_mem_arb_pcvlarb_q2_rdata_ecc_mask  :  5; // Error injection mask for length memory arbiter ECC error
        uint64_t inject_vl_len_mem_arb_pcvlarb_q2_rdata_ecc  :  6; // Error injection enable for length memory arbiter ECC error
        uint64_t inject_pkt_ctrl_fifo_rdata_mask  :  6; // Error injection mask for packet control fifo ECC error
        uint64_t inject_pkt_ctrl_fifo_rdata  :  4; // Error injection enable for packet control fifo ECC error
        uint64_t inject_qword_cnt_err_mask  :  4; // Error injection mask for QWORD count memory ECC error
        uint64_t inject_qword_cnt_err  :  1; // Error injection enable for QWORD count memory ECC error
        uint64_t       Reserved_63_37  : 27; // Unused
    } field;
    uint64_t val;
} TXCIC_DBG_ERR_INJECT_t;

// TXCIC_DBG_VISA_MON desc:
typedef union {
    struct {
        uint64_t        visa_mon_ctxt  :  8; // context to be monitored
        uint64_t          visa_mon_tc  :  2; // tc to be monitored
        uint64_t       Reserved_63_10  : 54; // Unused
    } field;
    uint64_t val;
} TXCIC_DBG_VISA_MON_t;

#endif /* ___FXR_tx_ci_cic_CSRS_H__ */
