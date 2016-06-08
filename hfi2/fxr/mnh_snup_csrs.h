// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Thu Jun  2 19:11:24 2016
//

#ifndef ___MNH_snup_CSRS_H__
#define ___MNH_snup_CSRS_H__

// DP_UPSTRM_CFG_FIFOS_RESET desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // Reset the upstream (from the serdes) fifo read and write pointers.
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} DP_UPSTRM_CFG_FIFOS_RESET_t;

// DP_UPSTRM_CFG_LOOPBACK desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // Currently just Dclk loopback is supported, the SerDes upstream clock must be active to pace the data.
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} DP_UPSTRM_CFG_LOOPBACK_t;

// SNP_UPSTRM_CFG_FRAMING_RESET desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // Reset the upstream (away from the serdes) snooper.
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_CFG_FRAMING_RESET_t;

// SNP_UPSTRM_CFG_FIFOS_RADR desc:
typedef union {
    struct {
        uint64_t              RST_VAL  :  4; // reset value. Increase toward 15 to reduce latency (this will cause bad crcs), decrease to add latency. 12: will be tested post Silicon for functionality to minimize latency 11: min latency, radr follows wadr with min delay 10: more latency than 11 9: more latency than 10 13,14,15,0,1,2,3,4,5,6,7,8: Undefined
        uint64_t       OK_TO_JUMP_VAL  :  4; // OK to jump the read address at the next skip opportunity when <= to this value. Keep equal to RST_VAL
        uint64_t      DO_NOT_JUMP_VAL  :  4; // Not OK to jump the read address when >= to this value. Set to OK_TO_JUMP_VAL + 1
        uint64_t         Unused_63_12  : 52; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_CFG_FIFOS_RADR_t;

// SNP_UPSTRM_CFG_IGNORE_LOST_RCLK desc:
typedef union {
    struct {
        uint64_t                   EN  :  1; // 
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_CFG_IGNORE_LOST_RCLK_t;

// SNP_UPSTRM_CFG_REINIT_PAUSE desc:
typedef union {
    struct {
        uint64_t              RX_MODE  :  2; // 0:no pause 1: reinit pause at 1 before align 2: reinit pause at 2 after lane test, before deskew, kill ready_for_deskew 3: reinit pause at 3 after deskew, kill 'framed' signal Clearing these should allow the reinit to proceed to completion or to the next pause location. After setting this register SNP_UPSTRM_CFG_FORCE_RECOVER_SEQUENCE should be used to trigger the reinit.
        uint64_t           Unused_3_2  :  2; // Unused
        uint64_t              TX_MODE  :  1; // Use on gen2 HFI for OPIO re-center. Switch side of link must have CFG_REINIT_AS_SLAVE set. This will not activate until a neg edge of LTP mode occurs. After setting this register use SNP_UPSTRM_CFG_FORCE_RECOVER_SEQUENCE to trigger the reinit. Poll STS_LINK_PAUSE_ACTIVE to make sure it is safe to start the OPIO re-center. After completion clearing this will release the pause. Link timer is still active so release must occur prior to expiration.
        uint64_t           Unused_7_5  :  3; // Unused
        uint64_t            ON_REINIT  :  1; // Level sensitive.Will pause during the next reinit state for debug on any kind of reinit trigger. Clearing may (skips could be an issue) allow continuation without errors. aka hold_composite_reinit
        uint64_t          Unused_63_9  : 55; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_CFG_REINIT_PAUSE_t;

// SNP_UPSTRM_CFG_REINIT_AS_SLAVE desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // Set this on the switch side. Do not use this in any internal or external loopback mode.
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_CFG_REINIT_AS_SLAVE_t;

// SNP_UPSTRM_CFG_FORCE_RECOVER_SEQUENCE desc:
typedef union {
    struct {
        uint64_t                   EN  :  1; // On the Tx side write a 1 to force a single reinit. Auto clears.
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_CFG_FORCE_RECOVER_SEQUENCE_t;

// SNP_UPSTRM_CFG_CNT_FOR_SKIP_STALL desc:
typedef union {
    struct {
        uint64_t           PPM_SELECT  :  2; // This must be the same in both link directions. 0: 200 ppm (Should be able to handle up/close to 244 ppm as a safety margin) 1: 300 ppm (Should be able to handle up/close to 312 ppm) 2: 400 ppm (Should be able to handle up/close to 488 ppm) 3: 1000 ppm (Should be able to handle up/close to 976 ppm)
        uint64_t           Unused_3_2  :  2; // Unused
        uint64_t           DISABLE_TX  :  1; // Might be set for gen 1 connected to gen 2.
        uint64_t           Unused_7_5  :  3; // Unused
        uint64_t           DISABLE_RX  :  1; // Used for gen 1 DV only. Might be set as default on gen 2.
        uint64_t          Unused_63_9  : 55; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_CFG_CNT_FOR_SKIP_STALL_t;

// SNP_UPSTRM_CFG_RX_FSS desc:
typedef union {
    struct {
        uint64_t                   EN  :  1; // 
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_CFG_RX_FSS_t;

// SNP_UPSTRM_CFG_CRC_MODE desc:
typedef union {
    struct {
        uint64_t               TX_VAL  :  2; // 0: 16 bit CRC. 1: 14 bit CRC. 2: 48 bit CRC. 3: CRC per lane mode.
        uint64_t           Unused_3_2  :  2; // Unused
        uint64_t       USE_RX_CFG_VAL  :  1; // ignore the value delivered in training pattern from the Tx side
        uint64_t           Unused_7_5  :  3; // Unused
        uint64_t               RX_VAL  :  2; // Used only when USE_RX_CFG_VAL is set. 0: 16 bit CRC. 1: 14 bit CRC. 2: 48 bit CRC. 3: CRC per lane mode.
        uint64_t         Unused_63_10  : 54; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_CFG_CRC_MODE_t;

// SNP_UPSTRM_CFG_LN_TEST_TIME_TO_PASS desc:
typedef union {
    struct {
        uint64_t                  VAL  : 12; // number of Tx side lane fifo clocks. Behavior is undefined for values other than the default.
        uint64_t         Unused_63_12  : 52; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_CFG_LN_TEST_TIME_TO_PASS_t;

// SNP_UPSTRM_CFG_LN_TEST_REQ_PASS_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  :  3; // 0: 0x80 1: 0xF0 (enable reset for failed test) 2: 0xFD (enable reset for failed test) 3: 0xFD 4: 0xFF (enable reset for failed test) 5: 0xFF 6: 0x100 (enable reset for failed test) 7: 0x100
        uint64_t          Unused_63_3  : 61; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_CFG_LN_TEST_REQ_PASS_CNT_t;

// SNP_UPSTRM_CFG_LN_RE_ENABLE desc:
typedef union {
    struct {
        uint64_t            ON_REINIT  :  1; // re-enable lanes that previously failed reinit testing on all reinit sequences
        uint64_t           LTO_REINIT  :  1; // re-enable lanes that previously failed reinit testing when the link times out
        uint64_t          LTO_DEGRADE  :  1; // re-enable degraded lanes when the link times out
        uint64_t          Unused_63_3  : 61; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_CFG_LN_RE_ENABLE_t;

// SNP_UPSTRM_CFG_LN_EN desc:
typedef union {
    struct {
        uint64_t                  VAL  :  4; // Clear to disable. This is best used prior to setting the SNP_UPSTRM_CFG_RUN CSR. Changing this field while the link is active produces undefined behavior.
        uint64_t          Unused_63_4  : 60; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_CFG_LN_EN_t;

// SNP_UPSTRM_CFG_REINITS_BEFORE_LINK_LOW desc:
typedef union {
    struct {
        uint64_t                  VAL  :  2; // 
        uint64_t          Unused_63_2  : 62; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_CFG_REINITS_BEFORE_LINK_LOW_t;

// SNP_UPSTRM_CFG_LOOPBACK desc:
typedef union {
    struct {
        uint64_t                  VAL  :  2; // 0: normal mode 1: loopback using i_lane*_dclk for tx_fifo read. Requires SNP_DWNSTRM_CFG_LANE_WIDTH VAL set to 0 or 3. 2,3: loopback using cclk for tx_fifo read. Requires SNP_DWNSTRM_CFG_LANE_WIDTH VAL set to 0. CFG_REINIT_AS_SLAVE must be clear in any internal or external loopback mode
        uint64_t          Unused_63_2  : 62; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_CFG_LOOPBACK_t;

// SNP_UPSTRM_CFG_INCOMPLT_RND_TRIP_DISABLE desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // 
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_CFG_INCOMPLT_RND_TRIP_DISABLE_t;

// SNP_UPSTRM_CFG_RND_TRIP_MAX desc:
typedef union {
    struct {
        uint64_t                  VAL  : 16; // 
        uint64_t         Unused_63_16  : 48; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_CFG_RND_TRIP_MAX_t;

// SNP_UPSTRM_CFG_LINK_TIMER_DISABLE desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // 
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_CFG_LINK_TIMER_DISABLE_t;

// SNP_UPSTRM_CFG_LINK_TIMER_MAX desc:
typedef union {
    struct {
        uint64_t                  VAL  : 32; // Number of cclks at which lack of forward progress will take down link transfer active.
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_CFG_LINK_TIMER_MAX_t;

// SNP_UPSTRM_CFG_DESKEW desc:
typedef union {
    struct {
        uint64_t          ALL_DISABLE  :  1; // 
        uint64_t           Unused_3_1  :  3; // Unused
        uint64_t        TIMER_DISABLE  :  1; // 
        uint64_t          Unused_63_5  : 59; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_CFG_DESKEW_t;

// SNP_UPSTRM_CFG_LN_DEGRADE desc:
typedef union {
    struct {
        uint64_t                   EN  :  1; // Use to enable or disable degrades.
        uint64_t           Unused_3_1  :  3; // Unused
        uint64_t          NUM_ALLOWED  :  2; // Number of degrade reinits allowed. 0 will prevent reinits triggered from lane degrades. The first degrade will transition from 4 lanes to 3. The second degrade will transition from 3 lanes to 2. The third degrade will transition from 2 lanes to 1. If only 2 lanes are active coming out of initialization than the first degrade will transition to a single lane. In this case the degrade cnt should never exceed 1 because we do not do any more degrades, virtual or otherwise, when only 1 lane remains active.
        uint64_t           Unused_7_6  :  2; // Unused
        uint64_t      DURATION_SELECT  :  3; // The duration (in number of LTPs divided by the number of active lanes) of the sliding window in which if a particular lane triggers sequential CRC errors its event count will increment. If a duration completes (including one that ends due to a sequential CRC error on a different lane) without a sequential error a lanes event count will decrement. If a lanes event count becomes equal to the number set in the EVENTS_TO_TRIGGER field it will be removed. 0: 0x408 (for DV) 1: 0x1008 2: 0x10008 3: 0x80004 4: 0xFF000 5: 0x400008 6: 0x800004 7: 0xFFFFFC
        uint64_t            Unused_11  :  1; // Unused
        uint64_t    EVENTS_TO_TRIGGER  :  3; // The number of sequential CRC error events (for a particular lane) (within the sliding window duration) required to trigger a lane removal. Don't use 0, it does not imply 8.
        uint64_t            Unused_15  :  1; // Unused
        uint64_t       RESET_SETTINGS  :  1; // reset degrade control settings, level sensitive. Should be taken high (long enough to be seen by both tx_cclk and rx_cclk,11 lclks to be safe in 1/8 BW mode), then low to load a new duration or if the EVENTS_TO_TRIGGER or NUM_ALLOWED needs to be changed without rebooting. This is for DV convenience only.
        uint64_t         Unused_19_17  :  3; // Unused
        uint64_t  RESET_DEGRADE_LN_EN  :  1; // auto clears. This forces a reinit sequence during which DEGRADE_LN_EN is reset to 4'b1111 re enabling any previously degraded lanes. This also does everything that reset_settings does.
        uint64_t         Unused_63_21  : 43; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_CFG_LN_DEGRADE_t;

// SNP_UPSTRM_CFG_CRC_INTERRUPT desc:
typedef union {
    struct {
        uint64_t              ERR_CNT  : 32; // Error count to interrupt on
        uint64_t             MATCH_EN  :  6; // multiple bits can be set to trigger an interrupt when any of the enabled error cnts reaches (exact match only) the ERR_CNT field. 0: lane 0 CRC error cnt 1: lane 1 CRC error cnt 2: lane 2 CRC error cnt 3: lane 3 CRC error cnt 4: total CRC error cnt 5: multiple CRC error cnt (more than 1 culprit).
        uint64_t         Unused_63_38  : 26; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_CFG_CRC_INTERRUPT_t;

// SNP_UPSTRM_CFG_LANE_WIDTH desc:
typedef union {
    struct {
        uint64_t                  VAL  :  2; // 0: Tx 32b, Rx 32b Gen 2 default. Used by DV to verify Gen 2 interop. 1: Tx 40b, Rx 32b Unlikely in the real world but good for DV. 2: Tx 32b, Rx 40b Unlikely in the real world but good for DV. 3: Tx 40b, Rx 40b Gen 1 default.
        uint64_t          Unused_63_2  : 62; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_CFG_LANE_WIDTH_t;

// SNP_UPSTRM_CFG_MISC desc:
typedef union {
    struct {
        uint64_t          FORCE_LN_EN  :  4; // 
        uint64_t  BCK_CH_SEL_PRIORITY  :  2; // 
        uint64_t           Unused_7_6  :  2; // Unused
        uint64_t  RX_CLK_SEL_PRIORITY  :  2; // 
        uint64_t         Unused_11_10  :  2; // Unused
        uint64_t SEL_REQUIRED_TOS_CNT  :  2; // 0:19, 1:27, 2:29, 3:31
        uint64_t         Unused_63_14  : 50; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_CFG_MISC_t;

// SNP_UPSTRM_CFG_CRC_CNTR_RESET desc:
typedef union {
    struct {
        uint64_t    CLR_CRC_ERR_CNTRS  :  1; // clears the total, lanes, multiple errors, replays and good, accepted LTP counts must be held high for at least a few clks for a reliable clear.
        uint64_t           Unused_3_1  :  3; // Unused
        uint64_t   HOLD_CRC_ERR_CNTRS  :  1; // simultaneously stops and holds the total, lanes, multiple errors, replays and good LTP counts
        uint64_t          Unused_63_5  : 59; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_CFG_CRC_CNTR_RESET_t;

// SNP_UPSTRM_CFG_LINK_KILL_EN desc:
typedef union {
    struct {
        uint64_t       CSR_PARITY_ERR  :  1; // see error flags
        uint64_t     INVALID_CSR_ADDR  :  1; // see error flags
        uint64_t RST_FOR_FAILED_DESKEW  :  1; // see error flags
        uint64_t ALL_LNS_FAILED_REINIT_TEST  :  1; // see error flags
        uint64_t LOST_REINIT_STALL_OR_TOS  :  1; // see error flags
        uint64_t RX_LESS_THAN_FOUR_LNS  :  1; // see error flags
        uint64_t          SEQ_CRC_ERR  :  1; // see error flags
        uint64_t     REINIT_FROM_PEER  :  1; // see error flags
        uint64_t REINIT_FOR_LN_DEGRADE  :  1; // see error flags
        uint64_t CRC_ERR_CNT_HIT_LIMIT  :  1; // see error flags
        uint64_t         RCLK_STOPPED  :  1; // see error flags. This enable should not be cleared. Doing so would expose the design to the possibility of silent data corruption.
        uint64_t UNEXPECTED_REPLAY_MARKER  :  1; // see error flags
        uint64_t UNEXPECTED_ROUND_TRIP_MARKER  :  1; // see error flags
        uint64_t VL_ACK_INPUT_BUF_OFLW  :  1; // see error flags
        uint64_t VL_ACK_INPUT_PARITY_ERR  :  1; // see error flags
        uint64_t VL_ACK_INPUT_WRONG_CRC_MODE  :  1; // see error flags
        uint64_t CREDIT_RETURN_FLIT_MBE  :  1; // see error flags
        uint64_t RST_FOR_LINK_TIMEOUT  :  1; // see error flags
        uint64_t RST_FOR_INCOMPLT_RND_TRIP  :  1; // see error flags
        uint64_t          HOLD_REINIT  :  1; // see error flags
        uint64_t NEG_EDGE_LINK_TRANSFER_ACTIVE  :  1; // see error flags
        uint64_t REDUNDANT_FLIT_PARITY_ERR  :  1; // see error flags. This enable should not be cleared as this results in undefined behavior inconsistent with the Storm Lake Architecture.
        uint64_t         Unused_63_22  : 42; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_CFG_LINK_KILL_EN_t;

// SNP_UPSTRM_CFG_ALLOW_LINK_UP desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // 
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_CFG_ALLOW_LINK_UP_t;

// SNP_UPSTRM_CFG_RESET_FROM_CSR desc:
typedef union {
    struct {
        uint64_t                   EN  :  1; // 
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_CFG_RESET_FROM_CSR_t;

// SNP_UPSTRM_CFG_SNDWN_RESET desc:
typedef union {
    struct {
        uint64_t                   EN  :  1; // 
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_CFG_SNDWN_RESET_t;

// SNP_UPSTRM_CFG_BCC_RESET desc:
typedef union {
    struct {
        uint64_t                   EN  :  1; // 
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_CFG_BCC_RESET_t;

// SNP_UPSTRM_CFG_8051_RESET desc:
typedef union {
    struct {
        uint64_t                   EN  :  1; // 
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_CFG_8051_RESET_t;

// SNP_UPSTRM_DBG_INIT_STATE_0 desc:
typedef union {
    struct {
        uint64_t       ALIGN_COMPLETE  :  4; // 1 for each lane
        uint64_t    POLARITY_COMPLETE  :  4; // 1 for each lane
        uint64_t             POLARITY  :  4; // 1 = true. 1 for each lane
        uint64_t  LOGICAL_ID_COMPLETE  :  4; // 1 for each lane
        uint64_t    FRAMING1_COMPLETE  :  4; // 1 for each lane
        uint64_t     CORSE_ALIGN_PTR0  :  2; // lane 0
        uint64_t         Unused_23_22  :  2; // Unused
        uint64_t      FINE_ALIGN_PTR0  :  3; // lane 0
        uint64_t            Unused_27  :  1; // Unused
        uint64_t     CORSE_ALIGN_PTR1  :  2; // lane 1
        uint64_t         Unused_31_30  :  2; // Unused
        uint64_t      FINE_ALIGN_PTR1  :  3; // lane 1
        uint64_t            Unused_35  :  1; // Unused
        uint64_t     CORSE_ALIGN_PTR2  :  2; // lane 2
        uint64_t         Unused_39_38  :  2; // Unused
        uint64_t      FINE_ALIGN_PTR2  :  3; // lane 2
        uint64_t            Unused_43  :  1; // Unused
        uint64_t     CORSE_ALIGN_PTR3  :  2; // lane 3
        uint64_t         Unused_47_46  :  2; // Unused
        uint64_t      FINE_ALIGN_PTR3  :  3; // lane 3
        uint64_t            Unused_51  :  1; // Unused
        uint64_t         Unused_63_52  : 12; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_DBG_INIT_STATE_0_t;

// SNP_UPSTRM_DBG_INIT_STATE_1 desc:
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
} SNP_UPSTRM_DBG_INIT_STATE_1_t;

// SNP_UPSTRM_DBG_INIT_STATE_2 desc:
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
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_DBG_INIT_STATE_2_t;

// SNP_UPSTRM_DBG_CFG_GOOD_BAD_DATA desc:
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
} SNP_UPSTRM_DBG_CFG_GOOD_BAD_DATA_t;

// SNP_UPSTRM_ERR_STS desc:
typedef union {
    struct {
        uint64_t       CSR_PARITY_ERR  :  1; // self explanatory
        uint64_t     INVALID_CSR_ADDR  :  1; // self explanatory
        uint64_t RST_FOR_FAILED_DESKEW  :  1; // self explanatory
        uint64_t ALL_LNS_FAILED_REINIT_TEST  :  1; // All four lanes failed testing during reinit, triggers a reset.
        uint64_t LOST_REINIT_STALL_OR_TOS  :  1; // lost clock stall or turn on signal during reinit. This is catastrophic and will take down the link.
        uint64_t RX_LESS_THAN_FOUR_LNS  :  1; // self explanatory
        uint64_t          SEQ_CRC_ERR  :  1; // A sequential CRC error was encountered. This triggers a reinit sequence.
        uint64_t     REINIT_FROM_PEER  :  1; // A reinit sequence was triggered by the peer.
        uint64_t REINIT_FOR_LN_DEGRADE  :  1; // A reinit sequence was triggered during which a lane was removed from operation.
        uint64_t CRC_ERR_CNT_HIT_LIMIT  :  1; // Programmed using the SNP_UPSTRM_CFG_CRC_INTERRUPT CSR.
        uint64_t         RCLK_STOPPED  :  1; // The (1 of 4) lane receive clock that is being used on the Rx side/pipe during LTP mode stopped toggling.This is catastrophic and will take down the link.
        uint64_t UNEXPECTED_REPLAY_MARKER  :  1; // self explanatory
        uint64_t UNEXPECTED_ROUND_TRIP_MARKER  :  1; // self explanatory
        uint64_t VL_ACK_INPUT_BUF_OFLW  :  1; // self explanatory
        uint64_t VL_ACK_INPUT_PARITY_ERR  :  1; // self explanatory
        uint64_t VL_ACK_INPUT_WRONG_CRC_MODE  :  1; // VC ack input valid and not in 14 bit CRC mode
        uint64_t CREDIT_RETURN_FLIT_MBE  :  1; // A credit return control flit had a MBE.
        uint64_t RST_FOR_LINK_TIMEOUT  :  1; // A reinit sequence is triggered in a last ditch attempt at keeping the link up when the timer expires. If this succeeds the NEG_EDGE_LINK_TRANSFER_ACTIVE and HOLD_REINIT flags will remain clear.
        uint64_t RST_FOR_INCOMPLT_RND_TRIP  :  1; // A reinit sequence is triggered in a last ditch attempt at keeping the link up when a round trip marker fails to return. If this succeeds the NEG_EDGE_LINK_TRANSFER_ACTIVE and HOLD_REINIT flags will remain clear.
        uint64_t          HOLD_REINIT  :  1; // This indicates the link will not come up. Useful when the link has never been up and the NEG_EDGE_LINK_TRANSFER_ACTIVE flag will be clear.
        uint64_t NEG_EDGE_LINK_TRANSFER_ACTIVE  :  1; // self explanatory
        uint64_t REDUNDANT_FLIT_PARITY_ERR  :  1; // The redundant flit info had a parity error.
        uint64_t         Unused_63_22  : 42; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_ERR_STS_t;

// SNP_UPSTRM_ERR_CLR desc:
typedef union {
    struct {
        uint64_t       CSR_PARITY_ERR  :  1; // see error flags
        uint64_t     INVALID_CSR_ADDR  :  1; // see error flags
        uint64_t RST_FOR_FAILED_DESKEW  :  1; // see error flags
        uint64_t ALL_LNS_FAILED_REINIT_TEST  :  1; // see error flags
        uint64_t LOST_REINIT_STALL_OR_TOS  :  1; // see error flags
        uint64_t RX_LESS_THAN_FOUR_LNS  :  1; // see error flags
        uint64_t          SEQ_CRC_ERR  :  1; // see error flags
        uint64_t     REINIT_FROM_PEER  :  1; // see error flags
        uint64_t REINIT_FOR_LN_DEGRADE  :  1; // see error flags
        uint64_t CRC_ERR_CNT_HIT_LIMIT  :  1; // see error flags
        uint64_t         RCLK_STOPPED  :  1; // see error flags
        uint64_t UNEXPECTED_REPLAY_MARKER  :  1; // see error flags
        uint64_t UNEXPECTED_ROUND_TRIP_MARKER  :  1; // see error flags
        uint64_t VL_ACK_INPUT_BUF_OFLW  :  1; // see error flags
        uint64_t VL_ACK_INPUT_PARITY_ERR  :  1; // see error flags
        uint64_t VL_ACK_INPUT_WRONG_CRC_MODE  :  1; // see error flags
        uint64_t CREDIT_RETURN_FLIT_MBE  :  1; // see error flags
        uint64_t RST_FOR_LINK_TIMEOUT  :  1; // see error flags
        uint64_t RST_FOR_INCOMPLT_RND_TRIP  :  1; // see error flags
        uint64_t          HOLD_REINIT  :  1; // see error flags
        uint64_t NEG_EDGE_LINK_TRANSFER_ACTIVE  :  1; // see error flags
        uint64_t REDUNDANT_FLIT_PARITY_ERR  :  1; // see error flags
        uint64_t         Unused_63_22  : 42; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_ERR_CLR_t;

// SNP_UPSTRM_ERR_FRC desc:
typedef union {
    struct {
        uint64_t       CSR_PARITY_ERR  :  1; // see error flags
        uint64_t     INVALID_CSR_ADDR  :  1; // see error flags
        uint64_t RST_FOR_FAILED_DESKEW  :  1; // see error flags
        uint64_t ALL_LNS_FAILED_REINIT_TEST  :  1; // see error flags
        uint64_t LOST_REINIT_STALL_OR_TOS  :  1; // see error flags
        uint64_t RX_LESS_THAN_FOUR_LNS  :  1; // see error flags
        uint64_t          SEQ_CRC_ERR  :  1; // see error flags
        uint64_t     REINIT_FROM_PEER  :  1; // see error flags
        uint64_t REINIT_FOR_LN_DEGRADE  :  1; // see error flags
        uint64_t CRC_ERR_CNT_HIT_LIMIT  :  1; // see error flags
        uint64_t         RCLK_STOPPED  :  1; // see error flags
        uint64_t UNEXPECTED_REPLAY_MARKER  :  1; // see error flags
        uint64_t UNEXPECTED_ROUND_TRIP_MARKER  :  1; // see error flags
        uint64_t VL_ACK_INPUT_BUF_OFLW  :  1; // see error flags
        uint64_t VL_ACK_INPUT_PARITY_ERR  :  1; // see error flags
        uint64_t VL_ACK_INPUT_WRONG_CRC_MODE  :  1; // see error flags
        uint64_t CREDIT_RETURN_FLIT_MBE  :  1; // see error flags
        uint64_t RST_FOR_LINK_TIMEOUT  :  1; // see error flags
        uint64_t RST_FOR_INCOMPLT_RND_TRIP  :  1; // see error flags
        uint64_t          HOLD_REINIT  :  1; // see error flags
        uint64_t NEG_EDGE_LINK_TRANSFER_ACTIVE  :  1; // see error flags
        uint64_t REDUNDANT_FLIT_PARITY_ERR  :  1; // see error flags
        uint64_t         Unused_63_22  : 42; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_ERR_FRC_t;

// SNP_UPSTRM_ERR_EN_HOST desc:
typedef union {
    struct {
        uint64_t       CSR_PARITY_ERR  :  1; // see error flags
        uint64_t     INVALID_CSR_ADDR  :  1; // see error flags
        uint64_t RST_FOR_FAILED_DESKEW  :  1; // see error flags
        uint64_t ALL_LNS_FAILED_REINIT_TEST  :  1; // see error flags
        uint64_t LOST_REINIT_STALL_OR_TOS  :  1; // see error flags
        uint64_t RX_LESS_THAN_FOUR_LNS  :  1; // see error flags
        uint64_t          SEQ_CRC_ERR  :  1; // see error flags
        uint64_t     REINIT_FROM_PEER  :  1; // see error flags
        uint64_t REINIT_FOR_LN_DEGRADE  :  1; // see error flags
        uint64_t CRC_ERR_CNT_HIT_LIMIT  :  1; // see error flags
        uint64_t         RCLK_STOPPED  :  1; // see error flags
        uint64_t UNEXPECTED_REPLAY_MARKER  :  1; // see error flags
        uint64_t UNEXPECTED_ROUND_TRIP_MARKER  :  1; // see error flags
        uint64_t VL_ACK_INPUT_BUF_OFLW  :  1; // see error flags
        uint64_t VL_ACK_INPUT_PARITY_ERR  :  1; // see error flags
        uint64_t VL_ACK_INPUT_WRONG_CRC_MODE  :  1; // see error flags
        uint64_t CREDIT_RETURN_FLIT_MBE  :  1; // see error flags
        uint64_t RST_FOR_LINK_TIMEOUT  :  1; // see error flags
        uint64_t RST_FOR_INCOMPLT_RND_TRIP  :  1; // see error flags
        uint64_t          HOLD_REINIT  :  1; // see error flags
        uint64_t NEG_EDGE_LINK_TRANSFER_ACTIVE  :  1; // see error flags
        uint64_t REDUNDANT_FLIT_PARITY_ERR  :  1; // see error flags
        uint64_t         Unused_63_22  : 42; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_ERR_EN_HOST_t;

// SNP_UPSTRM_ERR_FIRST_HOST desc:
typedef union {
    struct {
        uint64_t       CSR_PARITY_ERR  :  1; // see error flags
        uint64_t     INVALID_CSR_ADDR  :  1; // see error flags
        uint64_t RST_FOR_FAILED_DESKEW  :  1; // see error flags
        uint64_t ALL_LNS_FAILED_REINIT_TEST  :  1; // see error flags
        uint64_t LOST_REINIT_STALL_OR_TOS  :  1; // see error flags
        uint64_t RX_LESS_THAN_FOUR_LNS  :  1; // see error flags
        uint64_t          SEQ_CRC_ERR  :  1; // see error flags
        uint64_t     REINIT_FROM_PEER  :  1; // see error flags
        uint64_t REINIT_FOR_LN_DEGRADE  :  1; // see error flags
        uint64_t CRC_ERR_CNT_HIT_LIMIT  :  1; // see error flags
        uint64_t         RCLK_STOPPED  :  1; // see error flags
        uint64_t UNEXPECTED_REPLAY_MARKER  :  1; // see error flags
        uint64_t UNEXPECTED_ROUND_TRIP_MARKER  :  1; // see error flags
        uint64_t VL_ACK_INPUT_BUF_OFLW  :  1; // see error flags
        uint64_t VL_ACK_INPUT_PARITY_ERR  :  1; // see error flags
        uint64_t VL_ACK_INPUT_WRONG_CRC_MODE  :  1; // see error flags
        uint64_t CREDIT_RETURN_FLIT_MBE  :  1; // see error flags
        uint64_t RST_FOR_LINK_TIMEOUT  :  1; // see error flags
        uint64_t RST_FOR_INCOMPLT_RND_TRIP  :  1; // see error flags
        uint64_t          HOLD_REINIT  :  1; // see error flags
        uint64_t NEG_EDGE_LINK_TRANSFER_ACTIVE  :  1; // see error flags
        uint64_t REDUNDANT_FLIT_PARITY_ERR  :  1; // see error flags
        uint64_t         Unused_63_22  : 42; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_ERR_FIRST_HOST_t;

// SNP_UPSTRM_ERR_INFO_TOTAL_CRC_ERR desc:
typedef union {
    struct {
        uint64_t                  CNT  : 64; // 
    } field;
    uint64_t val;
} SNP_UPSTRM_ERR_INFO_TOTAL_CRC_ERR_t;

// SNP_UPSTRM_ERR_INFO_CRC_ERR_LN0 desc:
typedef union {
    struct {
        uint64_t                  CNT  : 64; // Rx physical lane 0. Cleared with CLR_CRC_ERR_CNTRS.
    } field;
    uint64_t val;
} SNP_UPSTRM_ERR_INFO_CRC_ERR_LN0_t;

// SNP_UPSTRM_ERR_INFO_CRC_ERR_LN1 desc:
typedef union {
    struct {
        uint64_t                  CNT  : 64; // Rx physical lane 1. Cleared with CLR_CRC_ERR_CNTRS.
    } field;
    uint64_t val;
} SNP_UPSTRM_ERR_INFO_CRC_ERR_LN1_t;

// SNP_UPSTRM_ERR_INFO_CRC_ERR_LN2 desc:
typedef union {
    struct {
        uint64_t                  CNT  : 64; // Rx physical lane 2. Cleared with CLR_CRC_ERR_CNTRS.
    } field;
    uint64_t val;
} SNP_UPSTRM_ERR_INFO_CRC_ERR_LN2_t;

// SNP_UPSTRM_ERR_INFO_CRC_ERR_LN3 desc:
typedef union {
    struct {
        uint64_t                  CNT  : 64; // Rx physical lane 3. Cleared with CLR_CRC_ERR_CNTRS.
    } field;
    uint64_t val;
} SNP_UPSTRM_ERR_INFO_CRC_ERR_LN3_t;

// SNP_UPSTRM_ERR_INFO_CRC_ERR_MULTI_LN desc:
typedef union {
    struct {
        uint64_t                  CNT  : 20; // Cleared with CLR_CRC_ERR_CNTRS.
        uint64_t         Unused_63_20  : 44; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_ERR_INFO_CRC_ERR_MULTI_LN_t;

// SNP_UPSTRM_ERR_INFO_RX_REPLAY_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // Cleared with CLR_CRC_ERR_CNTRS.
    } field;
    uint64_t val;
} SNP_UPSTRM_ERR_INFO_RX_REPLAY_CNT_t;

// SNP_UPSTRM_ERR_INFO_SEQ_CRC_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  : 32; // 
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_ERR_INFO_SEQ_CRC_CNT_t;

// SNP_UPSTRM_ERR_INFO_ESCAPE_0_ONLY_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  : 32; // Cleared with CLR_CRC_ERR_CNTRS.
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_ERR_INFO_ESCAPE_0_ONLY_CNT_t;

// SNP_UPSTRM_ERR_INFO_ESCAPE_0_PLUS1_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  : 32; // Cleared with CLR_CRC_ERR_CNTRS.
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_ERR_INFO_ESCAPE_0_PLUS1_CNT_t;

// SNP_UPSTRM_ERR_INFO_ESCAPE_0_PLUS2_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  : 16; // Cleared with CLR_CRC_ERR_CNTRS.
        uint64_t         Unused_63_16  : 48; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_ERR_INFO_ESCAPE_0_PLUS2_CNT_t;

// SNP_UPSTRM_ERR_INFO_REINIT_FROM_PEER_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  : 32; // 
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_ERR_INFO_REINIT_FROM_PEER_CNT_t;

// SNP_UPSTRM_ERR_INFO_MISC_FLG_CNT desc:
typedef union {
    struct {
        uint64_t RST_FOR_FAILED_DESKEW  :  8; // see ERR_FLG register
        uint64_t ALL_LNS_FAILED_REINIT_TEST  :  8; // see ERR_FLG register
        uint64_t RST_FOR_LINK_TIMEOUT  :  8; // see ERR_FLG register
        uint64_t RST_FOR_INCOMPLT_RND_TRIP  :  8; // see ERR_FLG register
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_ERR_INFO_MISC_FLG_CNT_t;

// SNP_UPSTRM_PRF_RX_FLIT_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  : 52; // 
        uint64_t         Unused_63_52  : 12; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_PRF_RX_FLIT_CNT_t;

// SNP_UPSTRM_STS_LINK_PAUSE_ACTIVE desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // It is safe to start the OPIO re-center when this is set.
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_STS_LINK_PAUSE_ACTIVE_t;

// SNP_UPSTRM_STS_LINK_TRANSFER_ACTIVE desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // 
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_STS_LINK_TRANSFER_ACTIVE_t;

// SNP_UPSTRM_STS_LN_DEGRADE desc:
typedef union {
    struct {
        uint64_t            EXHAUSTED  :  1; // CNT >= CFG_LN_DEGRADE_NUM_ALLOWED
        uint64_t                  CNT  :  2; // The number of lane degrade events. This still operates with degrades disabled so it can be used for diagnostic purposes.
        uint64_t             Unused_3  :  1; // Unused
        uint64_t     LN_012_RACES_WON  :  3; // These are the shifted lane numbers, not the physical lane numbers. This is the number of races won by the proposed team of shifted lanes 0/1/2 with lane 3 disabled.This cnt will not have meaning if less than 4 lanes are active. This still operates with degrades disabled so it can be used for diagnostic purposes.
        uint64_t     LN_013_RACES_WON  :  3; // These are the shifted lane numbers, not the physical lane numbers. This is the number of races won by the proposed team of shifted lanes 0/1/3 with lane 2 disabled. If only three lanes are active this is the number of races won by the proposed cfg of shifted lane 0 and 1. This cnt will not have meaning if only 2 lanes are active. This still operates with degrades disabled so it can be used for diagnostic purposes.
        uint64_t     LN_023_RACES_WON  :  3; // These are the shifted lane numbers, not the physical lane numbers. This is the number of races won by the proposed team of shifted lanes 0/2/3 with lane 1 disabled. If only three lanes are active this is the number of races won by the proposed cfg of shifted lane 0 and 2. If only two lanes are active this is the number of races won by the proposed cfg of shifted lane 0. This still operates with degrades disabled so it can be used for diagnostic purposes.
        uint64_t     LN_123_RACES_WON  :  3; // These are the shifted lane numbers, not the physical lane numbers. This is the number of races won by the proposed team of shifted lanes 1/2/3 with lane 0 disabled. If only three lanes are active this is the number of races won by the proposed cfg of shifted lane 1and 2. If only two lanes are active this is the number of races won by the proposed cfg of shifted lane 1. This still operates with degrades disabled so it can be used for diagnostic purposes.
        uint64_t CURRENT_CFG_RACES_WON  : 48; // number of races won by the current lane configuration. Sticks at 48'hffffffffffff. Cleared by a degrade reinit or CLR_CRC_ERR_CNTRS. This still operates with degrades disabled so it can be used for diagnostic purposes.
    } field;
    uint64_t val;
} SNP_UPSTRM_STS_LN_DEGRADE_t;

// SNP_UPSTRM_STS_TX_LTP_MODE desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // When set LTPs are being transmitted, when clear a reinit sequence is underway.
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_STS_TX_LTP_MODE_t;

// SNP_UPSTRM_STS_RX_LTP_MODE desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // When set LTPs are being received, when clear a reinit sequence is underway.
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_STS_RX_LTP_MODE_t;

// SNP_UPSTRM_STS_RX_CRC_MODE desc:
typedef union {
    struct {
        uint64_t                  VAL  :  2; // 
        uint64_t          Unused_63_2  : 62; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_STS_RX_CRC_MODE_t;

// SNP_UPSTRM_STS_RX_LOGICAL_ID desc:
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
} SNP_UPSTRM_STS_RX_LOGICAL_ID_t;

// SNP_UPSTRM_STS_RX_SHIFTED_LN_NUM desc:
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
} SNP_UPSTRM_STS_RX_SHIFTED_LN_NUM_t;

// SNP_UPSTRM_STS_RX_PHY_LN_EN desc:
typedef union {
    struct {
        uint64_t             COMBINED  :  4; // Combined lane enable = CFG_LN_EN & INIT & DEGRADE
        uint64_t                 INIT  :  4; // Initialization lane enable. (some lanes fail the training sequence)
        uint64_t              DEGRADE  :  4; // Degrade lane enable. (lanes are removed for high BER etc.) After a degrade occurs this field takes on the value of the new combined field. By observing the INIT field one can deduce why each lane was removed. If INIT is clear it was removed during initialization, otherwise it was removed by a degrade during LTP mode.
        uint64_t         Unused_63_12  : 52; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_STS_RX_PHY_LN_EN_t;

// SNP_UPSTRM_STS_TX_PHY_LN_EN desc:
typedef union {
    struct {
        uint64_t                  VAL  :  4; // {lane3,lane2,lane1,lane0}
        uint64_t          Unused_63_4  : 60; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_STS_TX_PHY_LN_EN_t;

// SNP_UPSTRM_STS_RCV_SMA_MSG desc:
typedef union {
    struct {
        uint64_t                  VAL  : 48; // 
        uint64_t         Unused_63_48  : 16; // Unused
    } field;
    uint64_t val;
} SNP_UPSTRM_STS_RCV_SMA_MSG_t;

#endif /* ___MNH_snup_CSRS_H__ */