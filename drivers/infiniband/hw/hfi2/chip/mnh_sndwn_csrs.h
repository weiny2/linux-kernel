// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Thu Mar 29 15:03:56 2018
//

#ifndef ___MNH_sndwn_CSRS_H__
#define ___MNH_sndwn_CSRS_H__

// DP_DWNSTRM_CFG_FIFOS_RADR desc:
typedef union {
    struct {
        uint64_t              RST_VAL  :  4; // 3:0
        uint64_t          Unused_63_4  : 60; // 63:4
    } field;
    uint64_t val;
} DP_DWNSTRM_CFG_FIFOS_RADR_t;

// SNP_DWNSTRM_CFG_FRAMING_RESET desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // Reset the downstream (towards the serdes) snooper.
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_CFG_FRAMING_RESET_t;

// SNP_DWNSTRM_CFG_FIFOS_RADR desc:
typedef union {
    struct {
        uint64_t              RST_VAL  :  4; // reset value. Increase toward 15 to reduce latency (this will cause bad crcs), decrease to add latency. 12: will be tested post Silicon for functionality to minimize latency 11: min latency, radr follows wadr with min delay 10: more latency than 11 9: more latency than 10 13,14,15,0,1,2,3,4,5,6,7,8: Undefined
        uint64_t       OK_TO_JUMP_VAL  :  4; // OK to jump the read address at the next skip opportunity when <= to this value. Keep equal to RST_VAL
        uint64_t      DO_NOT_JUMP_VAL  :  4; // Not OK to jump the read address when >= to this value. Set to OK_TO_JUMP_VAL + 1
        uint64_t         Unused_63_12  : 52; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_CFG_FIFOS_RADR_t;

// SNP_DWNSTRM_CFG_IGNORE_LOST_RCLK desc:
typedef union {
    struct {
        uint64_t                   EN  :  1; // 
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_CFG_IGNORE_LOST_RCLK_t;

// SNP_DWNSTRM_CFG_CNT_FOR_SKIP_STALL desc:
typedef union {
    struct {
        uint64_t           PPM_SELECT  :  2; // This must be the same in both link directions. 0: 200 ppm (Should be able to handle up/close to 244 ppm as a safety margin) 1: 300 ppm (Should be able to handle up/close to 312 ppm) 2: 400 ppm (Should be able to handle up/close to 488 ppm) 3: 1000 ppm (Should be able to handle up/close to 976 ppm)
        uint64_t           Unused_3_2  :  2; // Unused
        uint64_t           DISABLE_TX  :  1; // Set as default on gen 2. Needs to cleared for G2 Tx connected to G1 Rx.
        uint64_t           Unused_7_5  :  3; // Unused
        uint64_t           DISABLE_RX  :  1; // Set as default on gen 2. Default was clear on G1.
        uint64_t          Unused_11_9  :  3; // Unused
        uint64_t         DISABLE_INIT  :  1; // Disable only the training pattern skips. LTP mode skips unaffected.
        uint64_t         Unused_63_13  : 51; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_CFG_CNT_FOR_SKIP_STALL_t;

// SNP_DWNSTRM_CFG_RX_FSS desc:
typedef union {
    struct {
        uint64_t                   EN  :  1; // 
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_CFG_RX_FSS_t;

// SNP_DWNSTRM_CFG_CRC_MODE desc:
typedef union {
    struct {
        uint64_t               TX_VAL  :  2; // 0: 16 bit CRC. 1: 14 bit CRC. 2: 48 bit CRC. 3: CRC per lane mode.
        uint64_t           Unused_3_2  :  2; // Unused
        uint64_t       USE_RX_CFG_VAL  :  1; // ignore the value delivered in training pattern from the Tx side
        uint64_t           Unused_7_5  :  3; // Unused
        uint64_t               RX_VAL  :  2; // Used only when USE_RX_CFG_VAL is set. 0: 16 bit CRC. 1: 14 bit CRC. 2: 48 bit CRC. 3: CRC per lane mode.
        uint64_t         Unused_11_10  :  2; // Unused
        uint64_t      DISABLE_CRC_CHK  :  1; // Used in port mirror applications.
        uint64_t         Unused_63_13  : 51; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_CFG_CRC_MODE_t;

// SNP_DWNSTRM_CFG_LN_TEST_TIME_TO_PASS desc:
typedef union {
    struct {
        uint64_t                  VAL  : 12; // number of Tx side lane fifo clocks. Behavior is undefined for values other than the default.
        uint64_t         Unused_63_12  : 52; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_CFG_LN_TEST_TIME_TO_PASS_t;

// SNP_DWNSTRM_CFG_LN_TEST_REQ_PASS_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  :  3; // 0: 0x80 1: 0xF0 (enable reset for failed test) 2: 0xFD (enable reset for failed test) 3: 0xFD 4: 0xFF (enable reset for failed test) 5: 0xFF 6: 0x100 (enable reset for failed test) 7: 0x100
        uint64_t             Unused_3  :  1; // Unused
        uint64_t  DISABLE_REINIT_TEST  :  1; // This test is disabled for Gen2 to help running at high BER. Clear this for debug only.
        uint64_t          Unused_63_5  : 59; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_CFG_LN_TEST_REQ_PASS_CNT_t;

// SNP_DWNSTRM_CFG_LN_RE_ENABLE desc:
typedef union {
    struct {
        uint64_t            ON_REINIT  :  1; // re-enable lanes that previously failed reinit testing on all reinit sequences
        uint64_t           LTO_REINIT  :  1; // re-enable lanes that previously failed reinit testing when the link times out
        uint64_t          LTO_DEGRADE  :  1; // re-enable degraded lanes when the link times out
        uint64_t          Unused_63_3  : 61; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_CFG_LN_RE_ENABLE_t;

// SNP_DWNSTRM_CFG_LN_EN desc:
typedef union {
    struct {
        uint64_t                  VAL  :  4; // Clear to disable. This is best used prior to setting the SNP_DWNSTRM_CFG_RUN CSR. Changing this field while the link is active produces undefined behavior.
        uint64_t          Unused_63_4  : 60; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_CFG_LN_EN_t;

// SNP_DWNSTRM_CFG_REINITS_BEFORE_LINK_LOW desc:
typedef union {
    struct {
        uint64_t                  VAL  :  2; // 
        uint64_t          Unused_63_2  : 62; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_CFG_REINITS_BEFORE_LINK_LOW_t;

// SNP_DWNSTRM_CFG_LINK_TIMER_DISABLE desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // 
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_CFG_LINK_TIMER_DISABLE_t;

// SNP_DWNSTRM_CFG_LINK_TIMER_MAX desc:
typedef union {
    struct {
        uint64_t                  VAL  : 32; // Number of cclks at which lack of forward progress will take down link transfer active.
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_CFG_LINK_TIMER_MAX_t;

// SNP_DWNSTRM_CFG_DESKEW desc:
typedef union {
    struct {
        uint64_t          ALL_DISABLE  :  1; // 
        uint64_t           Unused_3_1  :  3; // Unused
        uint64_t        TIMER_DISABLE  :  1; // 
        uint64_t          Unused_63_5  : 59; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_CFG_DESKEW_t;

// SNP_DWNSTRM_CFG_CRC_INTERRUPT desc:
typedef union {
    struct {
        uint64_t              ERR_CNT  : 32; // Error count to interrupt on
        uint64_t             MATCH_EN  :  6; // multiple bits can be set to trigger an interrupt when any of the enabled error cnts reaches (exact match only) the ERR_CNT field. 0: lane 0 CRC error cnt 1: lane 1 CRC error cnt 2: lane 2 CRC error cnt 3: lane 3 CRC error cnt 4: total CRC error cnt 5: multiple CRC error cnt (more than 1 culprit).
        uint64_t         Unused_63_38  : 26; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_CFG_CRC_INTERRUPT_t;

// SNP_DWNSTRM_CFG_LANE_WIDTH desc:
typedef union {
    struct {
        uint64_t                  VAL  :  2; // 0: Tx 32b, Rx 32b Gen 2 default. Used by DV to verify Gen 2 interop. 1: Tx 40b, Rx 32b Unlikely in the real world but good for DV. 2: Tx 32b, Rx 40b Unlikely in the real world but good for DV. 3: Tx 40b, Rx 40b Gen 1 default.
        uint64_t          Unused_63_2  : 62; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_CFG_LANE_WIDTH_t;

// SNP_DWNSTRM_CFG_MISC desc:
typedef union {
    struct {
        uint64_t          FORCE_LN_EN  :  4; // 
        uint64_t  BCK_CH_SEL_PRIORITY  :  2; // 
        uint64_t           Unused_7_6  :  2; // Unused
        uint64_t  RX_CLK_SEL_PRIORITY  :  2; // 
        uint64_t         Unused_11_10  :  2; // Unused
        uint64_t     REQUIRED_TOS_CNT  :  5; // default allows 3 errors out of 26
        uint64_t         Unused_19_17  :  3; // Unused
        uint64_t   REQUIRED_STALL_CNT  :  5; // default allows 4 errors out of 29
        uint64_t         Unused_63_25  : 39; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_CFG_MISC_t;

// SNP_DWNSTRM_CFG_CRC_CNTR_RESET desc:
typedef union {
    struct {
        uint64_t    CLR_CRC_ERR_CNTRS  :  1; // clears the total, lanes, multiple errors, replays and good, accepted LTP counts must be held high for at least a few clks for a reliable clear.
        uint64_t           Unused_3_1  :  3; // Unused
        uint64_t   HOLD_CRC_ERR_CNTRS  :  1; // simultaneously stops and holds the total, lanes, multiple errors, replays and good LTP counts
        uint64_t           Unused_7_5  :  3; // Unused
        uint64_t CLR_SEQ_CRC_ERR_CNTRS  :  1; // clears the err_info_seq_crc_cnt and reinit_from_peer_cnt
        uint64_t          Unused_11_9  :  3; // Unused
        uint64_t    CLR_FEC_ERR_CNTRS  :  1; // Clear the ERR_INFO_FEC counters: ERR_INFO_FEC_CERR_CNT_1 ERR_INFO_FEC_CERR_CNT_2 ERR_INFO_FEC_CERR_CNT_3 ERR_INFO_FEC_CERR_CNT_4 ERR_INFO_FEC_CERR_CNT_5 ERR_INFO_FEC_CERR_CNT_6 ERR_INFO_FEC_CERR_CNT_7 ERR_INFO_FEC_CERR_CNT_8 ERR_INFO_FEC_UERR_CNT ERR_INFO_FEC_CERR_MASK_MISS_CNT ERR_INFO_FEC_ERR_LN0 ERR_INFO_FEC_ERR_LN1 ERR_INFO_FEC_ERR_LN2 ERR_INFO_FEC_ERR_LN3
        uint64_t         Unused_15_13  :  3; // Unused
        uint64_t   HOLD_FEC_ERR_CNTRS  :  1; // Stop and hold the ERR_INFO_FEC counters. CLR_FEC_ERR_CNTRS has priority.
        uint64_t         Unused_63_17  : 47; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_CFG_CRC_CNTR_RESET_t;

// SNP_DWNSTRM_CFG_REPLAY_PROTOCOL desc:
typedef union {
    struct {
        uint64_t                 GEN2  :  1; // 
        uint64_t           Unused_3_1  :  3; // Unused
        uint64_t  REINIT_NULL_CNT_MAX  :  3; // set REINIT_NULL_CNT_MAX to a value greater than 3'd2 and consistent with FZC and link partner OC
        uint64_t          Unused_63_7  : 57; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_CFG_REPLAY_PROTOCOL_t;

// SNP_DWNSTRM_CFG_FEC_MODE desc:
typedef union {
    struct {
        uint64_t               TX_VAL  :  2; // 0: FEC is disabled 1: RS(132,128) is enabled 2: RS(528,512) is enabled 3: Reserved
        uint64_t           Unused_3_2  :  2; // Unused
        uint64_t       USE_RX_CFG_VAL  :  1; // ignore the value delivered in training pattern from the Tx side
        uint64_t           Unused_7_5  :  3; // Unused
        uint64_t               RX_VAL  :  2; // Used only when USE_RX_CFG_VAL is set. 0: FEC is disabled 1: RS(132,128) is enabled 2: RS(528,512) is enabled 3: Reserved
        uint64_t         Unused_11_10  :  2; // Unused
        uint64_t      DISABLE_DEC_ERR  :  1; // Used in port mirror applications. FEC decode error output is disabled and will not generate link level replays.
        uint64_t         Unused_63_13  : 51; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_CFG_FEC_MODE_t;

// SNP_DWNSTRM_CFG_CLK_DURING_RESET desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // Selects cclk/2 instead of cclk for logic in the serdes_dwnstrm/upstrm_clk domains during reset/loopback/dfx_mode.
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_CFG_CLK_DURING_RESET_t;

// SNP_DWNSTRM_CFG_FEC_INTERRUPT desc:
typedef union {
    struct {
        uint64_t              ERR_CNT  : 48; // Error count to interrupt on
        uint64_t             MATCH_EN  : 14; // multiple bits can be set to trigger an interrupt when any of the enabled error cnts reaches (exact match only) the ERR_CNT field. 0: lane 3 FEC error cnt[47:0] 1: lane 2 FEC error cnt[47:0] 2: lane 1 FEC error cnt[47:0] 3: lane 0 FEC error cnt[47:0] 4: FEC uncorrectable error cnt[31:0] 5: FEC eight symbols corrected error cnt[31:0] 6: FEC seven symbols corrected error cnt[31:0] 7: FEC six symbols corrected error cnt[31:0] 8: FEC five symbols corrected error cnt[31:0] 9: FEC four symbols corrected error cnt[47:0] 10: FEC three symbols corrected error cnt[47:0] 11: FEC two symbols corrected error cnt[47:0] 12: FEC one symbol corrected error cnt[47:0] 13: FEC correctable error mask miss counter[31:0]
        uint64_t         Unused_63_62  :  2; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_CFG_FEC_INTERRUPT_t;

// SNP_DWNSTRM_CFG_LINK_PARTNER_GEN desc:
typedef union {
    struct {
        uint64_t                 GEN2  :  1; // Link partner is a gen2 device if this is 1.
        uint64_t           Unused_3_1  :  3; // Unused
        uint64_t                  SCR  :  1; // If set the training pattern is scrambled. This field applies only if GEN2 is set. Both ends of a Gen2 link must be set to the same value. The primary intent of this field is to disable scrambling during pre-silicon validation for debug.
        uint64_t           Unused_7_5  :  3; // Unused
        uint64_t                 PAT2  :  1; // If set the Gen2 training pattern is transmitted and expected to be received. If a 1'b0 the Gen1 training pattern is selected. If the link partner is a Gen2 device both ends of the link must have the same setting. If the link partner is a Gen1 device the value in this field is overridden by GEN2==1'b0 in the RTL, and is forced to select the Gen1 training pattern. This field is intended to be a pre-silicon validation debug vehicle where GEN2 devices can be linked using the Gen1 training pattern.
        uint64_t          Unused_11_9  :  3; // Unused
        uint64_t                 PQRS  :  1; // Send PQRS PAM4 symbols if set to 1'b1. Send effective NRZ(PAM4 symbols 0 and 3) if set to 1'b0.
        uint64_t         Unused_63_13  : 51; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_CFG_LINK_PARTNER_GEN_t;

// SNP_DWNSTRM_CFG_LM_MUXSEL desc:
typedef union {
    struct {
        uint64_t        SNPDWN_SELECT  :  4; // Logic monitor Mux select for downstream snooper modules 15: TBD ... 0:TBD
        uint64_t          Unused_63_4  : 60; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_CFG_LM_MUXSEL_t;

// SNP_DWNSTRM_CFG_MISCA desc:
typedef union {
    struct {
        uint64_t    REQUIRED_MODE_CNT  :  6; // TRUE_CNT and MISC_CNT threshold for q_crc_mode, q_fec_mode, and tx_lane_en_from_peer
        uint64_t          Unused_15_6  : 10; // Unused
        uint64_t     LANE_ID_DET_GEN1  :  5; // Per lane ID detection threshold for Gen1 training pattern enabled
        uint64_t         Unused_23_21  :  3; // Unused
        uint64_t     LANE_ID_DET_GEN2  :  5; // Per lane ID detection threshold for Gen2 training pattern enabled
        uint64_t         Unused_31_29  :  3; // Unused
        uint64_t      LENIENT_FRAMING  :  1; // Select lenient framing
        uint64_t         Unused_63_33  : 31; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_CFG_MISCA_t;

// SNP_DWNSTRM_DBG_INIT_STATE_0 desc:
typedef union {
    struct {
        uint64_t       ALIGN_COMPLETE  :  4; // 1 for each lane
        uint64_t    POLARITY_COMPLETE  :  4; // 1 for each lane
        uint64_t             POLARITY  :  4; // 1 = true. 1 for each lane
        uint64_t  LOGICAL_ID_COMPLETE  :  4; // 1 for each lane
        uint64_t    FRAMING1_COMPLETE  :  4; // 1 for each lane
        uint64_t      FINE_ALIGN_PTR0  :  2; // lane 0
        uint64_t         Unused_23_22  :  2; // Unused
        uint64_t     CORSE_ALIGN_PTR0  :  3; // lane 0
        uint64_t            Unused_27  :  1; // Unused
        uint64_t      FINE_ALIGN_PTR1  :  2; // lane 1
        uint64_t         Unused_31_30  :  2; // Unused
        uint64_t     CORSE_ALIGN_PTR1  :  3; // lane 1
        uint64_t            Unused_35  :  1; // Unused
        uint64_t      FINE_ALIGN_PTR2  :  2; // lane 2
        uint64_t         Unused_39_38  :  2; // Unused
        uint64_t     CORSE_ALIGN_PTR2  :  3; // lane 2
        uint64_t            Unused_43  :  1; // Unused
        uint64_t      FINE_ALIGN_PTR3  :  2; // lane 3
        uint64_t         Unused_47_46  :  2; // Unused
        uint64_t     CORSE_ALIGN_PTR3  :  3; // lane 3
        uint64_t            Unused_51  :  1; // Unused
        uint64_t         Unused_63_52  : 12; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_DBG_INIT_STATE_0_t;

// SNP_DWNSTRM_DBG_INIT_STATE_1 desc:
typedef union {
    struct {
        uint64_t  LN_TESTING_COMPLETE  :  4; // 1 for each lane
        uint64_t         PASS_CNT_LN0  :  9; // lane testing compare pass count, out of 256 pass if > LN_TEST_REQ_MATCH_CNT. Read after LN_TESTING_COMPLETE
        uint64_t         Unused_15_13  :  3; // Unused
        uint64_t         PASS_CNT_LN1  :  9; // lane testing compare pass count, out of 256 pass if > LN_TEST_REQ_MATCH_CNT. Read after LN_TESTING_COMPLETE
        uint64_t         Unused_27_25  :  3; // Unused
        uint64_t         PASS_CNT_LN2  :  9; // lane testing compare pass count, out of 256 pass if > LN_TEST_REQ_MATCH_CNT. Read after LN_TESTING_COMPLETE
        uint64_t         Unused_39_37  :  3; // Unused
        uint64_t         PASS_CNT_LN3  :  9; // lane testing compare pass count, out of 256 pass if > LN_TEST_REQ_MATCH_CNT. Read after LN_TESTING_COMPLETE
        uint64_t         Unused_51_49  :  3; // Unused
        uint64_t    LN_PASSED_TESTING  :  4; // First lane to pass starts LN_TEST_TIMER
        uint64_t LN_TEST_TIMER_COMPLETE  :  1; // Time for other lanes to pass has expired
        uint64_t         Unused_63_57  :  7; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_DBG_INIT_STATE_1_t;

// SNP_DWNSTRM_DBG_INIT_STATE_2 desc:
typedef union {
    struct {
        uint64_t      DESKEW_COMPLETE  :  1; // deskew complete.
        uint64_t           Unused_3_1  :  3; // Unused
        uint64_t  FAILED_TEST_CNT_LN0  :  3; // The number of resets back to align triggered when lane 0 does not pass testing. Read after RX_FRAMED
        uint64_t             Unused_7  :  1; // Unused
        uint64_t  FAILED_TEST_CNT_LN1  :  3; // The number of resets back to align triggered when lane 1 does not pass testing. Read after RX_FRAMED
        uint64_t            Unused_11  :  1; // Unused
        uint64_t  FAILED_TEST_CNT_LN2  :  3; // The number of resets back to align triggered when lane 2 does not pass testing. Read after RX_FRAMED
        uint64_t            Unused_15  :  1; // Unused
        uint64_t  FAILED_TEST_CNT_LN3  :  3; // The number of resets back to align triggered when lane 3 does not pass testing. Read after RX_FRAMED
        uint64_t            Unused_19  :  1; // Unused
        uint64_t RADR_SKIP4DESKEW_CNT_LN0  :  4; // rx fifo read pointer skips for deskew Read after RX_FRAMED
        uint64_t RADR_SKIP4DESKEW_CNT_LN1  :  4; // rx fifo read pointer skips for deskew Read after RX_FRAMED
        uint64_t RADR_SKIP4DESKEW_CNT_LN2  :  4; // rx fifo read pointer skips for deskew Read after RX_FRAMED
        uint64_t RADR_SKIP4DESKEW_CNT_LN3  :  4; // rx fifo read pointer skips for deskew Read after RX_FRAMED
        uint64_t BACK_CHAN_REPORTING_LN  :  2; // lane being used for back channel reporting. Read after RX_FRAMED
        uint64_t         Unused_39_38  :  2; // Unused
        uint64_t     RCLK_SAMPLING_LN  :  2; // lane being used for Rclk sampling Read after RX_FRAMED
        uint64_t         Unused_43_42  :  2; // Unused
        uint64_t  PEER_LN_EN_DETECTED  :  1; // peer lane enable detected, helps trigger tx_ltp_mode start
        uint64_t         Unused_47_45  :  3; // Unused
        uint64_t           TX_TON_SIG  :  1; // Tx sending the tos to peer, triggers tx_ltp_mode start. After PEER_LN_EN_DETECTED and min_f_eq1_cnt
        uint64_t         Unused_51_49  :  3; // Unused
        uint64_t        PHASE_ALIGNED  :  4; // Set if upper and lower 32 bits were swapped to achieve bit alignment when in Gen 1 mode. One per rx_fifo[3:0].
        uint64_t    PHASE_ALIGN_COMPL  :  4; // Phase alignment testing or swapping of upper and lower 32 bits has completed. One per rx_fifo[3:0]. Forced to a 1'b1 in Gen 2 mode.
        uint64_t         Unused_63_60  :  4; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_DBG_INIT_STATE_2_t;

// SNP_DWNSTRM_DBG_CFG_GOOD_BAD_DATA desc:
typedef union {
    struct {
        uint64_t                  ARM  :  1; // level sensitive, must set first and then clear to arm
        uint64_t           Unused_3_1  :  3; // Unused
        uint64_t               SELECT  :  2; // 0: raw good unscrambled LTP data buffer 1: raw bad unscrambled LTP data buffer 2: raw bad scrambled LTP data buffer
        uint64_t           Unused_7_6  :  2; // Unused
        uint64_t                 ADDR  :  4; // Address within the buffer. First clock of data is at address 0.
        uint64_t   SAVE_MULTIPLE_ONLY  :  1; // Only save data when there is a multiple lane error. Supersedes SAVE_ESCAPE_ONLY.
        uint64_t         Unused_15_13  :  3; // Unused
        uint64_t     SAVE_ESCAPE_ONLY  :  3; // 0: disable 1: 1 bad 3 good (probably a bad CRC bit) 2: 2 bad 2 good (probably 2 bad CRC bits) 3: 3 bad 1 good (3 bad CRC bits or a escape) 4: set ignore_crc_bits_for_culprits only 5: set ignore_crc_bits_for_culprits, escape_0_only 6: set ignore_crc_bits_for_culprits, escape_0_plus1 7: set ignore_crc_bits_for_culprits, escape_0_plus2 SAVE_MULTIPLE_ONLY must be clear to use all of the above.
        uint64_t            Unused_19  :  1; // Unused
        uint64_t             SAVE_ALL  :  1; // This will turn off escape counting if enabled. This is needed only when crc_mode is 2 and you want to prevent rearming for escape counting so you can look at routine failing patterns.
        uint64_t         Unused_23_21  :  3; // Unused
        uint64_t        ERR_STAT_MODE  :  1; // if set the bad/data is not saved as it is being used to generate the xfr err cnts
        uint64_t         Unused_27_25  :  3; // Unused
        uint64_t SHIFTED_LN_ERR_CNT_EN  :  4; // This should only be used when 4 lanes are active.The enabled shifted lane(s) participate in the required error cnt and in the STS_XFR_ERR_STATS
        uint64_t RAW_REQUIRED_ERR_CNT  :  5; // If the xfr err cnt is less than this value the LP will not be saved.
        uint64_t         Unused_63_37  : 27; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_DBG_CFG_GOOD_BAD_DATA_t;

// SNP_DWNSTRM_ERR_STS desc:
typedef union {
    struct {
        uint64_t RST_FOR_FAILED_DESKEW  :  1; // self explanatory
        uint64_t ALL_LNS_FAILED_REINIT_TEST  :  1; // All four lanes failed testing during reinit, triggers a reset.
        uint64_t RX_LESS_THAN_FOUR_LNS  :  1; // self explanatory
        uint64_t          SEQ_CRC_ERR  :  1; // A sequential CRC error was encountered. This triggers a reinit sequence.
        uint64_t     REINIT_FROM_PEER  :  1; // A reinit sequence was triggered by the peer.
        uint64_t CRC_ERR_CNT_HIT_LIMIT  :  1; // Programmed using the SNP_UPSTRM_CFG_CRC_INTERRUPT CSR.
        uint64_t         RCLK_STOPPED  :  1; // The (1 of 4) lane receive clock that is being used on the Rx side/pipe during LTP mode stopped toggling.This is catastrophic and will take down the link.
        uint64_t FEC_ERR_CNT_HIT_LIMIT  :  1; // Programmed using the CFG_FEC_INTERRUPT CSR.
        uint64_t          Unused_63_8  : 56; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_STS_t;

// SNP_DWNSTRM_ERR_CLR desc:
typedef union {
    struct {
        uint64_t RST_FOR_FAILED_DESKEW  :  1; // see error flags
        uint64_t ALL_LNS_FAILED_REINIT_TEST  :  1; // see error flags
        uint64_t RX_LESS_THAN_FOUR_LNS  :  1; // see error flags
        uint64_t          SEQ_CRC_ERR  :  1; // see error flags
        uint64_t     REINIT_FROM_PEER  :  1; // see error flags
        uint64_t CRC_ERR_CNT_HIT_LIMIT  :  1; // see error flags
        uint64_t         RCLK_STOPPED  :  1; // see error flags
        uint64_t FEC_ERR_CNT_HIT_LIMIT  :  1; // Programmed using the CFG_FEC_INTERRUPT CSR.
        uint64_t          Unused_63_8  : 56; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_CLR_t;

// SNP_DWNSTRM_ERR_FRC desc:
typedef union {
    struct {
        uint64_t RST_FOR_FAILED_DESKEW  :  1; // see error flags
        uint64_t ALL_LNS_FAILED_REINIT_TEST  :  1; // see error flags
        uint64_t RX_LESS_THAN_FOUR_LNS  :  1; // see error flags
        uint64_t          SEQ_CRC_ERR  :  1; // see error flags
        uint64_t     REINIT_FROM_PEER  :  1; // see error flags
        uint64_t CRC_ERR_CNT_HIT_LIMIT  :  1; // see error flags
        uint64_t         RCLK_STOPPED  :  1; // see error flags
        uint64_t FEC_ERR_CNT_HIT_LIMIT  :  1; // see error flags
        uint64_t          Unused_63_8  : 56; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_FRC_t;

// SNP_DWNSTRM_ERR_EN_HOST desc:
typedef union {
    struct {
        uint64_t RST_FOR_FAILED_DESKEW  :  1; // see error flags
        uint64_t ALL_LNS_FAILED_REINIT_TEST  :  1; // see error flags
        uint64_t RX_LESS_THAN_FOUR_LNS  :  1; // see error flags
        uint64_t          SEQ_CRC_ERR  :  1; // see error flags
        uint64_t     REINIT_FROM_PEER  :  1; // see error flags
        uint64_t CRC_ERR_CNT_HIT_LIMIT  :  1; // see error flags
        uint64_t         RCLK_STOPPED  :  1; // see error flags
        uint64_t FEC_ERR_CNT_HIT_LIMIT  :  1; // see error flags
        uint64_t          Unused_63_8  : 56; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_EN_HOST_t;

// SNP_DWNSTRM_ERR_FIRST_HOST desc:
typedef union {
    struct {
        uint64_t RST_FOR_FAILED_DESKEW  :  1; // see error flags
        uint64_t ALL_LNS_FAILED_REINIT_TEST  :  1; // see error flags
        uint64_t RX_LESS_THAN_FOUR_LNS  :  1; // see error flags
        uint64_t          SEQ_CRC_ERR  :  1; // see error flags
        uint64_t     REINIT_FROM_PEER  :  1; // see error flags
        uint64_t CRC_ERR_CNT_HIT_LIMIT  :  1; // see error flags
        uint64_t         RCLK_STOPPED  :  1; // see error flags
        uint64_t FEC_ERR_CNT_HIT_LIMIT  :  1; // see error flags
        uint64_t          Unused_63_8  : 56; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_FIRST_HOST_t;

// SNP_DWNSTRM_ERR_INFO_TOTAL_CRC_ERR desc:
typedef union {
    struct {
        uint64_t                  CNT  : 64; // 
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_INFO_TOTAL_CRC_ERR_t;

// SNP_DWNSTRM_ERR_INFO_CRC_ERR_LN0 desc:
typedef union {
    struct {
        uint64_t                  CNT  : 64; // Rx physical lane 0. Cleared with CLR_CRC_ERR_CNTRS.
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_INFO_CRC_ERR_LN0_t;

// SNP_DWNSTRM_ERR_INFO_CRC_ERR_LN1 desc:
typedef union {
    struct {
        uint64_t                  CNT  : 64; // Rx physical lane 1. Cleared with CLR_CRC_ERR_CNTRS.
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_INFO_CRC_ERR_LN1_t;

// SNP_DWNSTRM_ERR_INFO_CRC_ERR_LN2 desc:
typedef union {
    struct {
        uint64_t                  CNT  : 64; // Rx physical lane 2. Cleared with CLR_CRC_ERR_CNTRS.
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_INFO_CRC_ERR_LN2_t;

// SNP_DWNSTRM_ERR_INFO_CRC_ERR_LN3 desc:
typedef union {
    struct {
        uint64_t                  CNT  : 64; // Rx physical lane 3. Cleared with CLR_CRC_ERR_CNTRS.
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_INFO_CRC_ERR_LN3_t;

// SNP_DWNSTRM_ERR_INFO_CRC_ERR_MULTI_LN desc:
typedef union {
    struct {
        uint64_t                  CNT  : 20; // Cleared with CLR_CRC_ERR_CNTRS.
        uint64_t         Unused_63_20  : 44; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_INFO_CRC_ERR_MULTI_LN_t;

// SNP_DWNSTRM_ERR_INFO_RX_REPLAY_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // Cleared with CLR_CRC_ERR_CNTRS.
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_INFO_RX_REPLAY_CNT_t;

// SNP_DWNSTRM_ERR_INFO_SEQ_CRC_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  : 32; // 
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_INFO_SEQ_CRC_CNT_t;

// SNP_DWNSTRM_ERR_INFO_ESCAPE_0_ONLY_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  : 32; // Cleared with CLR_CRC_ERR_CNTRS.
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_INFO_ESCAPE_0_ONLY_CNT_t;

// SNP_DWNSTRM_ERR_INFO_ESCAPE_0_PLUS1_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  : 32; // Cleared with CLR_CRC_ERR_CNTRS.
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_INFO_ESCAPE_0_PLUS1_CNT_t;

// SNP_DWNSTRM_ERR_INFO_ESCAPE_0_PLUS2_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  : 16; // Cleared with CLR_CRC_ERR_CNTRS.
        uint64_t         Unused_63_16  : 48; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_INFO_ESCAPE_0_PLUS2_CNT_t;

// SNP_DWNSTRM_ERR_INFO_REINIT_FROM_PEER_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  : 32; // 
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_INFO_REINIT_FROM_PEER_CNT_t;

// SNP_DWNSTRM_ERR_INFO_MISC_FLG_CNT desc:
typedef union {
    struct {
        uint64_t RST_FOR_FAILED_DESKEW  :  8; // see ERR_FLG register
        uint64_t ALL_LNS_FAILED_REINIT_TEST  :  8; // see ERR_FLG register
        uint64_t         Unused_63_16  : 48; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_INFO_MISC_FLG_CNT_t;

// SNP_DWNSTRM_ERR_INFO_FEC_CERR_CNT_1 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 48; // The number of received code words with one corrected symbol error.
        uint64_t         Unused_63_48  : 16; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_INFO_FEC_CERR_CNT_1_t;

// SNP_DWNSTRM_ERR_INFO_FEC_CERR_CNT_2 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 48; // The number of received code words with two corrected symbol errors.
        uint64_t         Unused_63_48  : 16; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_INFO_FEC_CERR_CNT_2_t;

// SNP_DWNSTRM_ERR_INFO_FEC_CERR_CNT_3 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 48; // The number of received code words with three corrected symbol errors.
        uint64_t         Unused_63_48  : 16; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_INFO_FEC_CERR_CNT_3_t;

// SNP_DWNSTRM_ERR_INFO_FEC_CERR_CNT_4 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 48; // The number of received code words with four corrected symbol errors.
        uint64_t         Unused_63_48  : 16; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_INFO_FEC_CERR_CNT_4_t;

// SNP_DWNSTRM_ERR_INFO_FEC_CERR_CNT_5 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // The number of received code words with five corrected symbol errors.
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_INFO_FEC_CERR_CNT_5_t;

// SNP_DWNSTRM_ERR_INFO_FEC_CERR_CNT_6 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // The number of received code words with six corrected symbol errors.
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_INFO_FEC_CERR_CNT_6_t;

// SNP_DWNSTRM_ERR_INFO_FEC_CERR_CNT_7 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // The number of received code words with seven corrected symbol errors.
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_INFO_FEC_CERR_CNT_7_t;

// SNP_DWNSTRM_ERR_INFO_FEC_CERR_CNT_8 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // The number of received code words with eight corrected symbol errors.
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_INFO_FEC_CERR_CNT_8_t;

// SNP_DWNSTRM_ERR_INFO_FEC_UERR_CNT desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // The number of received code words with uncorrectable errors
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_INFO_FEC_UERR_CNT_t;

// SNP_DWNSTRM_ERR_INFO_FEC_CERR_MASK_0 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 64; // Bits [63:0] of the error correction vector.
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_INFO_FEC_CERR_MASK_0_t;

// SNP_DWNSTRM_ERR_INFO_FEC_CERR_MASK_1 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 64; // Bits [127:64] of the error correction vector.
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_INFO_FEC_CERR_MASK_1_t;

// SNP_DWNSTRM_ERR_INFO_FEC_CERR_MASK_2 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 64; // Bits [191:128] of the error correction vector.
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_INFO_FEC_CERR_MASK_2_t;

// SNP_DWNSTRM_ERR_INFO_FEC_CERR_MASK_3 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 64; // Bits [255:192] of the error correction vector.
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_INFO_FEC_CERR_MASK_3_t;

// SNP_DWNSTRM_ERR_INFO_FEC_CERR_MASK_4 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 64; // Bits [319:256] of the error correction vector.
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_INFO_FEC_CERR_MASK_4_t;

// SNP_DWNSTRM_ERR_INFO_FEC_CERR_MASK_VLD desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // ERR_INFO_FEC_CERR_MASK is valid.
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_INFO_FEC_CERR_MASK_VLD_t;

// SNP_DWNSTRM_ERR_INFO_FEC_CERR_MASK_MISS_CNT desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Number of missed error mask captures.
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_INFO_FEC_CERR_MASK_MISS_CNT_t;

// SNP_DWNSTRM_ERR_INFO_FEC_ERR_LN0 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 48; // Number of corrected error events in lane 0
        uint64_t         Unused_63_48  : 16; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_INFO_FEC_ERR_LN0_t;

// SNP_DWNSTRM_ERR_INFO_FEC_ERR_LN1 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 48; // Number of corrected error events in lane 1
        uint64_t         Unused_63_48  : 16; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_INFO_FEC_ERR_LN1_t;

// SNP_DWNSTRM_ERR_INFO_FEC_ERR_LN2 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 48; // Number of corrected error events in lane 2
        uint64_t         Unused_63_48  : 16; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_INFO_FEC_ERR_LN2_t;

// SNP_DWNSTRM_ERR_INFO_FEC_ERR_LN3 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 48; // Number of corrected error events in lane 3
        uint64_t         Unused_63_48  : 16; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_ERR_INFO_FEC_ERR_LN3_t;

// SNP_DWNSTRM_PRF_GOOD_LTP_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // Counts LTPs with a good CRC, rx_ltp_mode only. Cleared with CLR_CRC_ERR_CNTRS.
    } field;
    uint64_t val;
} SNP_DWNSTRM_PRF_GOOD_LTP_CNT_t;

// SNP_DWNSTRM_PRF_ACCEPTED_LTP_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // Counts LTPs accepted into the receive buffer. Nulls and tossed good LTPs don't add to the count. Cleared with CLR_CRC_ERR_CNTRS.
    } field;
    uint64_t val;
} SNP_DWNSTRM_PRF_ACCEPTED_LTP_CNT_t;

// SNP_DWNSTRM_PRF_GOOD_FECCW_CNT desc:
typedef union {
    struct {
        uint64_t                  CNT  : 64; // The number of received code words without errors.
    } field;
    uint64_t val;
} SNP_DWNSTRM_PRF_GOOD_FECCW_CNT_t;

// SNP_DWNSTRM_STS_RX_LTP_MODE desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // When set LTPs are being received, when clear a reinit sequence is underway.
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_STS_RX_LTP_MODE_t;

// SNP_DWNSTRM_STS_RX_CRC_FEC_MODE desc:
typedef union {
    struct {
        uint64_t             CRC_MODE  :  2; // 
        uint64_t           Unused_3_2  :  2; // Unused
        uint64_t             FEC_MODE  :  2; // RS(528,512) = 2'b10, RS(132,128) = 2'b01, FEC disabled = 2'b00
        uint64_t          Unused_63_6  : 58; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_STS_RX_CRC_FEC_MODE_t;

// SNP_DWNSTRM_STS_RX_LOGICAL_ID desc:
typedef union {
    struct {
        uint64_t                  LN0  :  3; // > 3 indicates this lane is disabled.
        uint64_t             Unused_3  :  1; // Unused
        uint64_t                  LN1  :  3; // > 3 indicates this lane is disabled.
        uint64_t             Unused_7  :  1; // Unused
        uint64_t                  LN2  :  3; // > 3 indicates this lane is disabled.
        uint64_t            Unused_11  :  1; // Unused
        uint64_t                  LN3  :  3; // > 3 indicates this lane is disabled.
        uint64_t         Unused_63_15  : 49; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_STS_RX_LOGICAL_ID_t;

// SNP_DWNSTRM_STS_RX_SHIFTED_LN_NUM desc:
typedef union {
    struct {
        uint64_t                  LN0  :  3; // > 3 indicates this lane is disabled.
        uint64_t             Unused_3  :  1; // Unused
        uint64_t                  LN1  :  3; // > 3 indicates this lane is disabled.
        uint64_t             Unused_7  :  1; // Unused
        uint64_t                  LN2  :  3; // > 3 indicates this lane is disabled.
        uint64_t            Unused_11  :  1; // Unused
        uint64_t                  LN3  :  3; // > 3 indicates this lane is disabled.
        uint64_t         Unused_63_15  : 49; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_STS_RX_SHIFTED_LN_NUM_t;

// SNP_DWNSTRM_STS_RX_PHY_LN_EN desc:
typedef union {
    struct {
        uint64_t             COMBINED  :  4; // Combined lane enable = CFG_LN_EN & INIT & DEGRADE
        uint64_t                 INIT  :  4; // Initialization lane enable. (some lanes fail the training sequence)
        uint64_t              DEGRADE  :  4; // Degrade lane enable. (lanes are removed for high BER etc.) After a degrade occurs this field takes on the value of the new combined field. By observing the INIT field one can deduce why each lane was removed. If INIT is clear it was removed during initialization, otherwise it was removed by a degrade during LTP mode.
        uint64_t         Unused_63_12  : 52; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_STS_RX_PHY_LN_EN_t;

// SNP_DWNSTRM_STS_TX_PHY_LN_EN desc:
typedef union {
    struct {
        uint64_t                  VAL  :  4; // {lane3,lane2,lane1,lane0}
        uint64_t          Unused_63_4  : 60; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_STS_TX_PHY_LN_EN_t;

// SNP_DWNSTRM_STS_RCV_SMA_MSG desc:
typedef union {
    struct {
        uint64_t                  VAL  : 48; // 
        uint64_t         Unused_63_48  : 16; // Unused
    } field;
    uint64_t val;
} SNP_DWNSTRM_STS_RCV_SMA_MSG_t;

#endif /* ___MNH_sndwn_CSRS_H__ */