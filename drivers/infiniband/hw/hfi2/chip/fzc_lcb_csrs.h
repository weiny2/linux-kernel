// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Thu Mar 29 15:03:56 2018
//

#ifndef ___FZC_lcb_CSRS_H__
#define ___FZC_lcb_CSRS_H__

// LCB_CFG_RESET_FROM_CSR desc:
typedef union {
    struct {
        uint64_t                   EN  :  1; // This CSR generates the equivalent of a soft reset. It resets everything in the LCB except sticky CSRs.
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} LCB_CFG_RESET_FROM_CSR_t;

// LCB_CFG_RUN desc:
typedef union {
    struct {
        uint64_t                   EN  :  1; // 
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} LCB_CFG_RUN_t;

// LCB_CFG_TX_FIFOS_RESET desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // Clear after the lane clocks are on and prior to setting LCB_CFG_RUN.
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} LCB_CFG_TX_FIFOS_RESET_t;

// LCB_CFG_PORT desc:
typedef union {
    struct {
        uint64_t           link_state  :  3; // Link State 0x0 = Reserved 0x1 = Down (read only) 0x2 = Init (ready only) 0x3 = Armed (only writable to 0x3 if currently in INIT state) 0x4 = Active (only writable to 0x4 if currently in ARM state) 0x5-0x7 = NOP ( link state does not change)
        uint64_t             Unused_3  :  1; // Unused
        uint64_t disable_auto_arm_active  :  1; // Setting this to a '1' will disable auto arm->active transition based on a non SC15 packet.
        uint64_t           Unused_7_5  :  3; // Unused
        uint64_t       send_in_arm_en  :  1; // Enables management and data packet to be transmitted from TX while the logic link state == ARM. set for switch ports not HFI. To enable this feature: Set send_in_arm_en Set neighbor_normal Logical Link State == ARM
        uint64_t          Unused_11_9  :  3; // Unused
        uint64_t      neighbor_normal  :  1; // Enables management and data packet to be transmitted from TX while the logic link state == ARM. set for switch ports not HFI. To enable this feature: Set send_in_arm_en Set neighbor_normal Logical Link State == ARM
        uint64_t         Unused_63_13  : 51; // Unused
    } field;
    uint64_t val;
} LCB_CFG_PORT_t;

// LCB_CFG_IGNORE_LOST_RCLK desc:
typedef union {
    struct {
        uint64_t                   EN  :  1; // 
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} LCB_CFG_IGNORE_LOST_RCLK_t;

// LCB_CFG_REINIT_PAUSE desc:
typedef union {
    struct {
        uint64_t              RX_MODE  :  2; // 0:no pause 1: reinit pause at 1 before align 2: reinit pause at 2 after lane test, before deskew, kill ready_for_deskew 3: reinit pause at 3 after deskew, kill 'framed' signal Clearing these should allow the reinit to proceed to completion or to the next pause location. After setting this register LCB_CFG_FORCE_RECOVER_SEQUENCE should be used to trigger the reinit.
        uint64_t           Unused_3_2  :  2; // Unused
        uint64_t              TX_MODE  :  1; // Use on gen2 HFI for OPIO re-center. Switch side of link must have CFG_REINIT_AS_SLAVE set. This will not activate until a neg edge of LTP mode occurs. After setting this register use LCB_CFG_FORCE_RECOVER_SEQUENCE to trigger the reinit. Poll STS_LINK_PAUSE_ACTIVE to make sure it is safe to start the OPIO re-center. After completion clearing this will release the pause. Link timer is still active so release must occur prior to expiration.
        uint64_t           Unused_7_5  :  3; // Unused
        uint64_t            ON_REINIT  :  1; // Level sensitive.Will pause during the next reinit state for debug on any kind of reinit trigger. Clearing may (skips could be an issue) allow continuation without errors. aka hold_composite_reinit
        uint64_t          Unused_63_9  : 55; // Unused
    } field;
    uint64_t val;
} LCB_CFG_REINIT_PAUSE_t;

// LCB_CFG_REINIT_AS_SLAVE desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // Set this on the switch side. Do not use this in any internal or external loopback mode.
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} LCB_CFG_REINIT_AS_SLAVE_t;

// LCB_CFG_FORCE_RECOVER_SEQUENCE desc:
typedef union {
    struct {
        uint64_t                   EN  :  1; // On the Tx side write a 1 to force a single reinit. Auto clears.
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} LCB_CFG_FORCE_RECOVER_SEQUENCE_t;

// LCB_CFG_CNT_FOR_SKIP_STALL desc:
typedef union {
    struct {
        uint64_t           PPM_SELECT  :  2; // This must be the same in both link directions. 0: 200 ppm (Should be able to handle up/close to 244 ppm as a safety margin) 1: 300 ppm (Should be able to handle up/close to 312 ppm) 2: 400 ppm (Should be able to handle up/close to 488 ppm) 3: 1000 ppm (Should be able to handle up/close to 976 ppm)
        uint64_t           Unused_3_2  :  2; // Unused
        uint64_t           DISABLE_TX  :  1; // Turns on/off sending skips to the link partner. The sent skips are gen1 (32-bit) or gen2 (64-bit) skips depending on the value of CRK_LCB_CFG_LINK_PARTNER_GEN.GEN2.
        uint64_t           Unused_7_5  :  3; // Unused
        uint64_t           DISABLE_RX  :  1; // Set as default on gen 2. Should be cleared to enable gen2 skips when in port mirror modes.
        uint64_t          Unused_11_9  :  3; // Unused
        uint64_t         DISABLE_INIT  :  1; // Disable only the training pattern skips. LTP mode skips unaffected.
        uint64_t         Unused_63_13  : 51; // Unused
    } field;
    uint64_t val;
} LCB_CFG_CNT_FOR_SKIP_STALL_t;

// LCB_CFG_TX_FSS desc:
typedef union {
    struct {
        uint64_t                   EN  :  1; // 
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} LCB_CFG_TX_FSS_t;

// LCB_CFG_RX_FSS desc:
typedef union {
    struct {
        uint64_t                   EN  :  1; // 
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} LCB_CFG_RX_FSS_t;

// LCB_CFG_CRC_MODE desc:
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
} LCB_CFG_CRC_MODE_t;

// LCB_CFG_LN_DCLK desc:
typedef union {
    struct {
        uint64_t        INVERT_SAMPLE  :  1; // A way to shift the phase of the sampled version of the selected dclk in case it is being sampled consistently at the edge and this is causing unexpected problems. Potentially required because dclk and cclk run off the same Ref Clk and are at a exact 5/4 frequency ratio with unknown phase. All other versions of the dclk are unaffected. The phase of the data being presented to the SERDES lanes is unaffected.
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} LCB_CFG_LN_DCLK_t;

// LCB_CFG_LN_TEST_TIME_TO_PASS desc:
typedef union {
    struct {
        uint64_t                  VAL  : 12; // number of Tx side lane fifo clocks.
        uint64_t         Unused_63_12  : 52; // Unused
    } field;
    uint64_t val;
} LCB_CFG_LN_TEST_TIME_TO_PASS_t;

// LCB_CFG_LN_TEST_REQ_PASS_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  :  3; // 0: 0x80 1: 0xF0 (enable reset for failed test) 2: 0xFD (enable reset for failed test) 3: 0xFD 4: 0xFF (enable reset for failed test) 5: 0xFF 6: 0x100 (enable reset for failed test) 7: 0x100
        uint64_t             Unused_3  :  1; // Unused
        uint64_t  DISABLE_REINIT_TEST  :  1; // This test is disabled for Gen2 to help running at high BER. Clear this for debug only.
        uint64_t          Unused_63_5  : 59; // Unused
    } field;
    uint64_t val;
} LCB_CFG_LN_TEST_REQ_PASS_CNT_t;

// LCB_CFG_LN_RE_ENABLE desc:
typedef union {
    struct {
        uint64_t            ON_REINIT  :  1; // re-enable lanes that previously failed reinit testing on all reinit sequences
        uint64_t           LTO_REINIT  :  1; // re-enable lanes that previously failed reinit testing when the link times out
        uint64_t          LTO_DEGRADE  :  1; // re-enable degraded lanes when the link times out Not implemented. The LCB reset sequence that follows a link time out re-enables previously degraded lanes. If it is required to retain lane degrade state through an LCB reset sequence, a FW solution using lcb_cfg_rx_ln_en and lcb_sts_rx_phy_ln_en can be implemented.
        uint64_t          Unused_63_3  : 61; // Unused
    } field;
    uint64_t val;
} LCB_CFG_LN_RE_ENABLE_t;

// LCB_CFG_RX_LN_EN desc:
typedef union {
    struct {
        uint64_t                  VAL  :  4; // Clear to disable. This is best used prior to setting the LCB_CFG_RUN CSR. Changing this field while the link is active produces undefined behavior.
        uint64_t          Unused_63_4  : 60; // Unused
    } field;
    uint64_t val;
} LCB_CFG_RX_LN_EN_t;

// LCB_CFG_NULLS_REQUIRED_BASE desc:
typedef union {
    struct {
        uint64_t                 VAL0  :  4; // rx_num_lanes > tx_num_lanes
        uint64_t                 VAL1  :  4; // rx_num_lanes == tx_num_lanes && rx_num_lanes < 3'd4
        uint64_t                 VAL2  :  4; // rx_num_lanes == 3'd4 && tx_num_lanes == 3'd4
        uint64_t                 VAL3  :  4; // rx_num_lanes == 3'd3 && tx_num_lanes == 3'd4
        uint64_t                 VAL4  :  4; // rx_num_lanes == 3'd2 && tx_num_lanes == 3'd3
        uint64_t                 VAL5  :  4; // rx_num_lanes == 3'd2 && tx_num_lanes == 3'd4
        uint64_t                 VAL6  :  4; // rx_num_lanes == 3'd1 && tx_num_lanes == 3'd2
        uint64_t                 VAL7  :  4; // rx_num_lanes == 3'd1 && tx_num_lanes == 3'd3
        uint64_t                 VAL8  :  4; // rx_num_lanes == 3'd1 && tx_num_lanes == 3'd4
        uint64_t            SMALL_FEC  :  4; // Adjustment for small FEC latency
        uint64_t              BIG_FEC  :  4; // Adjustment for big FEC latency
        uint64_t         Unused_63_44  : 20; // Unused
    } field;
    uint64_t val;
} LCB_CFG_NULLS_REQUIRED_BASE_t;

// LCB_CFG_REINITS_BEFORE_LINK_LOW desc:
typedef union {
    struct {
        uint64_t                  VAL  :  2; // With higher PAM4 error rates this gives the link one additional reinit cycle to come up.
        uint64_t          Unused_63_2  : 62; // Unused
    } field;
    uint64_t val;
} LCB_CFG_REINITS_BEFORE_LINK_LOW_t;

// LCB_CFG_REMOVE_RANDOM_NULLS desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // 
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} LCB_CFG_REMOVE_RANDOM_NULLS_t;

// LCB_CFG_REPLAY_BUF_MAX_LTP desc:
typedef union {
    struct {
        uint64_t                  NUM  : 15; // This is used for DV purposes only. It can be used to test the Rx side logic to verify it can work with arbitrary replay buffer sizes from a minimum of 15'h10 LTPs up to the maximum (size in LTPs - 1) of the replay buffer, 15'h7f in gen 1.
        uint64_t         Unused_63_15  : 49; // Unused
    } field;
    uint64_t val;
} LCB_CFG_REPLAY_BUF_MAX_LTP_t;

// LCB_CFG_THROTTLE_TX_FLITS desc:
typedef union {
    struct {
        uint64_t                   EN  :  8; // [0] send idles in LTP flit locations 0,1 [1] send idles in LTP flit locations 2,3 [2] send idles in LTP flit locations 4,5 [3] send idles in LTP flit locations 6,7 [4] send idles in LTP flit locations 8,9 [5] send idles in LTP flit locations 10,11 [6] send idles in LTP flit locations 12,13 [7] send idles in LTP flit locations 14,15
        uint64_t             ONE_SHOT  :  1; // Use to force one shot throttling. Auto clears this field and the EN field when complete.
        uint64_t          Unused_11_9  :  3; // Unused
        uint64_t             AFIOW_TX  :  1; // Do not set. For debug only. allow forced idles (9'h002) to be transmitted on the wire as a pure/plain idle.
        uint64_t         Unused_15_13  :  3; // Unused
        uint64_t             AFIOW_RX  :  1; // Do not set. For debug only. prevents quad idles from being put into rcv_buf to prevent overflow if slow cclk is being used for debug
        uint64_t         Unused_63_17  : 47; // Unused
    } field;
    uint64_t val;
} LCB_CFG_THROTTLE_TX_FLITS_t;

// LCB_CFG_SEND_SMA_MSG desc:
typedef union {
    struct {
        uint64_t                  VAL  : 48; // 
        uint64_t         Unused_63_48  : 16; // Unused
    } field;
    uint64_t val;
} LCB_CFG_SEND_SMA_MSG_t;

// LCB_CFG_INCOMPLT_RND_TRIP_DISABLE desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // 
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} LCB_CFG_INCOMPLT_RND_TRIP_DISABLE_t;

// LCB_CFG_RND_TRIP_MAX desc:
typedef union {
    struct {
        uint64_t                  VAL  : 16; // 
        uint64_t         Unused_63_16  : 48; // Unused
    } field;
    uint64_t val;
} LCB_CFG_RND_TRIP_MAX_t;

// LCB_CFG_LINK_TIMER_DISABLE desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // 
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} LCB_CFG_LINK_TIMER_DISABLE_t;

// LCB_CFG_LINK_TIMER_MAX desc:
typedef union {
    struct {
        uint64_t                  VAL  : 32; // Number of cclks at which lack of forward progress will either take down link transfer active or initiate a series of REINIT attempts. This timer activates when the LCB enters the Rx Tossing State or is replaying link transfer packets. If the LCB does not make forward progress (does not exit from the Rx Tossing state) within the allowed timeout period, the appropriate action is taken.
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} LCB_CFG_LINK_TIMER_MAX_t;

// LCB_CFG_DESKEW desc:
typedef union {
    struct {
        uint64_t     TIME_TO_COMPLETE  : 16; // LSB resolution is 1 rx clock
        uint64_t          ALL_DISABLE  :  1; // 
        uint64_t         Unused_19_17  :  3; // Unused
        uint64_t        TIMER_DISABLE  :  1; // 
        uint64_t         Unused_23_21  :  3; // Unused
        uint64_t         FRAMED_DELAY  : 12; // This delay is inserted after deskew completes and before lane enables are transmitted. The primary intent of this delay is to give the MNH snoopers additional time to deskew before FZC and OC/DC send their turn on signal. When applied on the FZC side, FZC's turn-on signal is delayed, which subsequently delays OC's or DC's turn-on. The result is added deskew time for both the upstream and downstream Snoopers on MNH. When configuring topologies that do not include MNH the value of this field remains at default. In the fzc_mnh, fzc_mnh_oc, and fzc_mnh_dc topologies the recommended value is 12'h100.
        uint64_t         Unused_63_36  : 28; // Unused
    } field;
    uint64_t val;
} LCB_CFG_DESKEW_t;

// LCB_CFG_LN_DEGRADE desc:
typedef union {
    struct {
        uint64_t                   EN  :  1; // Use to enable or disable degrades.
        uint64_t           Unused_3_1  :  3; // Unused
        uint64_t          NUM_ALLOWED  :  2; // Number of degrade reinits allowed. 0 will prevent reinits triggered from lane degrades. The first degrade will transition from 4 lanes to 3. The second degrade will transition from 3 lanes to 2. The third degrade will transition from 2 lanes to 1. If only 2 lanes are active coming out of initialization than the first degrade will transition to a single lane. In this case the degrade cnt should never exceed 1 because we do not do any more degrades, virtual or otherwise, when only 1 lane remains active. When this field is set to '0, it will not apply to FEC based lane degrades. To disable FEC lane degrade clear lcb_cfg_fec_ln_degrade.en
        uint64_t           Unused_7_6  :  2; // Unused
        uint64_t      DURATION_SELECT  :  3; // The duration (in number of LTPs divided by the number of active lanes) of the sliding window in which if a particular lane triggers sequential CRC errors its event count will increment. If a duration completes (including one that ends due to a sequential CRC error on a different lane) without a sequential error a lanes event count will decrement. If a lanes event count becomes equal to the number set in the EVENTS_TO_TRIGGER field it will be removed. 0: 0x408 (for DV) 1: 0x1008 2: 0x10008 3: 0x80004 4: 0xFF000 5: 0x400008 6: 0x800004 7: 0xFFFFFC
        uint64_t            Unused_11  :  1; // Unused
        uint64_t    EVENTS_TO_TRIGGER  :  3; // The number of sequential CRC error events (for a particular lane) (within the sliding window duration) required to trigger a lane removal. Don't use 0, it does not imply 8.
        uint64_t            Unused_15  :  1; // Unused
        uint64_t       RESET_SETTINGS  :  1; // reset degrade control settings, level sensitive. Should be taken high (long enough to be seen by both tx_cclk and rx_cclk,11 lclks to be safe in 1/8 BW mode), then low to load a new duration or if the EVENTS_TO_TRIGGER or NUM_ALLOWED needs to be changed without rebooting. This is for DV convenience only.
        uint64_t         Unused_19_17  :  3; // Unused
        uint64_t  RESET_DEGRADE_LN_EN  :  1; // auto clears. This forces a reinit sequence during which DEGRADE_LN_EN is reset to 4'b1111 re enabling any previously degraded lanes. This applies to both the CRC and FEC based lane degrade mask enables. This also does everything that reset_settings does.
        uint64_t         Unused_63_21  : 43; // Unused
    } field;
    uint64_t val;
} LCB_CFG_LN_DEGRADE_t;

// LCB_CFG_CRC_INTERRUPT desc:
typedef union {
    struct {
        uint64_t              ERR_CNT  : 32; // Error count to interrupt on
        uint64_t             MATCH_EN  :  6; // multiple bits can be set to trigger an interrupt when any of the enabled error cnts reaches (exact match only) the ERR_CNT field. 0: lane 0 CRC error cnt 1: lane 1 CRC error cnt 2: lane 2 CRC error cnt 3: lane 3 CRC error cnt 4: total CRC error cnt 5: multiple CRC error cnt (more than 1 culprit).
        uint64_t         Unused_63_38  : 26; // Unused
    } field;
    uint64_t val;
} LCB_CFG_CRC_INTERRUPT_t;

// LCB_CFG_CLK_DURING_RESET desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // NA: This feature was removed. cclk/2 is no longer supported. If set, the LCB uses cclk/2 instead of cclk to clock the dclk and rclk logic domains during reset, loopback and dfx modes.
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} LCB_CFG_CLK_DURING_RESET_t;

// LCB_CFG_LOOPBACK desc:
typedef union {
    struct {
        uint64_t                  VAL  :  2; // 0: normal mode 1: loopback using i_lane*_dclk for tx_fifo read. 2,3: loopback using cclk for tx_fifo read. CFG_REINIT_AS_SLAVE must be clear in any internal or external loopback mode
        uint64_t          Unused_63_2  : 62; // Unused
    } field;
    uint64_t val;
} LCB_CFG_LOOPBACK_t;

// LCB_CFG_MISC desc:
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
        uint64_t         Unused_27_25  :  3; // Unused
        uint64_t      CORRECT_1SYMBOL  :  1; // When set 1 symbol error is forward corrected when the small FEC is enabled. If two or more symbol errors are detected a replay is initiated. The default is to forward correct one symbol errors per codeword, and initiate replay if more than one symbol errors are detected.
        uint64_t         Unused_31_29  :  3; // Unused
        uint64_t    HOLD_FEC_LTP_MODE  :  1; // Hold off FEC turn off until an entire FEC CW has been processed in the FEC encoder.
        uint64_t         Unused_35_33  :  3; // Unused
        uint64_t    FEC_Q_FULL_STOP_Q  :  1; // Default =0 . Mode where the fec queue stops taking in the error masks until the whole queue is cleared.
        uint64_t         Unused_39_37  :  3; // Unused
        uint64_t     CORRECT_NSYMBOLS  :  4; // N-1 symbol errors are forward corrected when the big FEC is enabled. If N or more symbols errors are detected a replay is initiated. Default is 7 symbol error correction. Legal values for N are: 1, 2, 3, 4, 5, 6, 7, and 8
        uint64_t         Unused_63_44  : 20; // Unused
    } field;
    uint64_t val;
} LCB_CFG_MISC_t;

// LCB_CFG_CLK_CNTR desc:
typedef union {
    struct {
        uint64_t               SELECT  :  5; // [4:3] = 00 and [2:0] = 0: gated tx_cclk divby8 1: GND, default for o_clk_observe 2: raw tx fifo clk divby8 (lane used for tx fifo clk selected by 8051) 3: raw tx fifo clk divby8 after loopback mux [4:3] = 01 and [2:0] = 0: raw rx fifo clk0 divby8 1: raw rx fifo clk0 divby8 after loopback mux 2: raw rx fifo clk1 divby8 3: raw rx fifo clk1 divby8 after loopback mux 4: raw rx fifo clk2 divby8 5: raw rx fifo clk2 divby8 after loopback mux 6: raw rx fifo clk3 divby8 7: raw rx fifo clk3 divby8 after loopback mux [4:3] = 10 and [2:0] = 0: gated locked2lane0 divby8 1: gated selected rx_cclk0 divby8 2: gated locked2lane1 divby8 3: gated selected rx_cclk1 divby8 4: gated locked2lane2 divby8 5: gated selected rx_cclk2 divby8 6: gated locked2lane3 divby8 7: gated selected rx_cclk3 divby8 [4:3] = 11 and [2:0] = * sbus clk
        uint64_t           Unused_7_5  :  3; // Unused
        uint64_t  STROBE_ON_CCLK_CNTR  :  1; // clear during performance monitor applications. Set for accurate clk count after 2**24 cclk counts.
        uint64_t          Unused_63_9  : 55; // Unused
    } field;
    uint64_t val;
} LCB_CFG_CLK_CNTR_t;

// LCB_CFG_CRC_CNTR_RESET desc:
typedef union {
    struct {
        uint64_t    CLR_CRC_ERR_CNTRS  :  1; // clears the total, lanes, multiple errors, replays and good, accepted LTP counts must be held high for at least a few clks for a reliable clear. lcb_sts_ln_degrade.current_cfg_races_won lcb_prf_good_ltp_cnt.val lcb_prf_accepted_ltp_cnt.val lcb_prf_tx_reliable_ltp_cnt.val lcb_err_info_crc_err_ln0.cnt lcb_err_info_crc_err_ln1.cnt lcb_err_info_crc_err_ln2.cnt lcb_err_info_crc_err_ln3.cnt lcb_err_info_crc_err_multi_ln.cnt lcb_err_info_tx_replay_cnt.val lcb_err_info_rx_replay_cnt.val lcb_err_info_escape_0_only_cnt.val lcb_err_info_escape_0_plus1_cnt.val lcb_err_info_escape_0_plus2_cnt.val lcb_err_info_total_crc_err.cnt
        uint64_t           Unused_3_1  :  3; // Unused
        uint64_t   HOLD_CRC_ERR_CNTRS  :  1; // simultaneously stops and holds the total, lanes, multiple errors, replays and good LTP counts. lcb_prf_good_ltp_cnt.val lcb_prf_accepted_ltp_cnt.val lcb_prf_tx_reliable_ltp_cnt.val lcb_err_info_crc_err_ln0.cnt lcb_err_info_crc_err_ln1.cnt lcb_err_info_crc_err_ln2.cnt lcb_err_info_crc_err_ln3.cnt lcb_err_info_crc_err_multi_ln.cnt lcb_err_info_tx_replay_cnt.val lcb_err_info_rx_replay_cnt.val lcb_err_info_escape_0_only_cnt.val lcb_err_info_escape_0_plus1_cnt.val lcb_err_info_escape_0_plus2_cnt.val lcb_err_info_total_crc_err.cnt
        uint64_t           Unused_7_5  :  3; // Unused
        uint64_t CLR_SEQ_CRC_ERR_CNTRS  :  1; // clears the err_info_seq_crc_cnt and reinit_from_peer_cnt
        uint64_t          Unused_11_9  :  3; // Unused
        uint64_t    CLR_FEC_ERR_CNTRS  :  1; // Clear the ERR_INFO_FEC counters: LCB_ERR_INFO_FEC_CERR_CNT_1 LCB_ERR_INFO_FEC_CERR_CNT_2 LCB_ERR_INFO_FEC_CERR_CNT_3 LCB_ERR_INFO_FEC_CERR_CNT_4 LCB_ERR_INFO_FEC_CERR_CNT_5 LCB_ERR_INFO_FEC_CERR_CNT_6 LCB_ERR_INFO_FEC_CERR_CNT_7 LCB_ERR_INFO_FEC_CERR_CNT_8 LCB_ERR_INFO_FEC_UERR_CNT LCB_ERR_INFO_FEC_CERR_MASK_MISS_CNT LCB_ERR_INFO_FEC_ERR_LN0 LCB_ERR_INFO_FEC_ERR_LN1 LCB_ERR_INFO_FEC_ERR_LN2 LCB_ERR_INFO_FEC_ERR_LN3
        uint64_t         Unused_15_13  :  3; // Unused
        uint64_t   HOLD_FEC_ERR_CNTRS  :  1; // Stop and hold the ERR_INFO_FEC counters. CLR_FEC_ERR_CNTRS has priority.
        uint64_t         Unused_63_17  : 47; // Unused
    } field;
    uint64_t val;
} LCB_CFG_CRC_CNTR_RESET_t;

// LCB_CFG_LINK_KILL_EN desc:
typedef union {
    struct {
        uint64_t RST_FOR_FAILED_DESKEW  :  1; // see error flags
        uint64_t ALL_LNS_FAILED_REINIT_TEST  :  1; // see error flags
        uint64_t LOST_REINIT_STALL_OR_TOS  :  1; // see error flags
        uint64_t TX_LESS_THAN_FOUR_LNS  :  1; // see error flags
        uint64_t RX_LESS_THAN_FOUR_LNS  :  1; // see error flags
        uint64_t          SEQ_CRC_ERR  :  1; // see error flags
        uint64_t     REINIT_FROM_PEER  :  1; // see error flags
        uint64_t REINIT_FOR_LN_DEGRADE  :  1; // see error flags
        uint64_t CRC_ERR_CNT_HIT_LIMIT  :  1; // see error flags
        uint64_t         RCLK_STOPPED  :  1; // see error flags. This enable should not be cleared. Doing so would expose the design to the possibility of silent data corruption.
        uint64_t UNEXPECTED_REPLAY_MARKER  :  1; // see error flags
        uint64_t UNEXPECTED_ROUND_TRIP_MARKER  :  1; // see error flags
        uint64_t     ILLEGAL_NULL_LTP  :  1; // see error flags
        uint64_t ILLEGAL_FLIT_ENCODING  :  1; // see error flags
        uint64_t  FLIT_INPUT_BUF_OFLW  :  1; // see error flags
        uint64_t VL_ACK_INPUT_BUF_OFLW  :  1; // see error flags
        uint64_t VL_ACK_INPUT_PARITY_ERR  :  1; // see error flags
        uint64_t VL_ACK_INPUT_WRONG_CRC_MODE  :  1; // see error flags
        uint64_t   FLIT_INPUT_BUF_MBE  :  1; // see error flags
        uint64_t   FLIT_INPUT_BUF_SBE  :  1; // see error flags
        uint64_t       REPLAY_BUF_MBE  :  1; // see error flags
        uint64_t       REPLAY_BUF_SBE  :  1; // see error flags
        uint64_t RST_FOR_LINK_TIMEOUT  :  1; // see error flags
        uint64_t RST_FOR_INCOMPLT_RND_TRIP  :  1; // see error flags
        uint64_t          HOLD_REINIT  :  1; // see error flags
        uint64_t NEG_EDGE_LINK_TRANSFER_ACTIVE  :  1; // see error flags
        uint64_t UNEXPECTED_MASTER_TIME_FLIT  :  1; // see error flags
        uint64_t           QUARANTINE  :  1; // see error flags
        uint64_t CSR_CHAIN_PARITY_ERR  :  1; // see error flags
        uint64_t CSR_CPORT_ADDRESS_MISS  :  1; // see error flags
        uint64_t               PM_MBE  :  1; // see error flags
        uint64_t               PM_SBE  :  1; // see error flags
        uint64_t FEC_ERR_CNT_HIT_LIMIT  :  1; // see error flags
        uint64_t         Unused_63_33  : 31; // Unused
    } field;
    uint64_t val;
} LCB_CFG_LINK_KILL_EN_t;

// LCB_CFG_ALLOW_LINK_UP desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // 
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} LCB_CFG_ALLOW_LINK_UP_t;

// LCB_CFG_LED desc:
typedef union {
    struct {
        uint64_t        sw_blink_rate  :  4; // 0x0 - LED off 0x1 - reserved 0x2 - reserved 0x3 - LED blink(128ms on, 128msoff) 0x4 - 0xE - reserved 0xF - LED on
        uint64_t                cntrl  :  1; // 0x0 - Hardware logic controls LED Link DOWN, led = off. Link UP, led = on. Slow rate < 10K packets/s 384 ms on 384 ms off Fast Rate > 10K packets/s 128ms on 128ms off 0x1 - Port LED controlled by 'led_sw_blink_rate'
        uint64_t           Unused_7_5  :  3; // Unused
        uint64_t      invert_polarity  :  1; // survivability
        uint64_t          Unused_63_9  : 55; // Unused
    } field;
    uint64_t val;
} LCB_CFG_LED_t;

// LCB_CFG_LINK_PARTNER_GEN desc:
typedef union {
    struct {
        uint64_t                 GEN2  :  1; // Link partner is a gen2 device if this is 1. When CRK_LCB_CNT_FOR_SKIP_STALL.DISABLE_TX is 0, this field dictates whether the sent skips are gen1 (32-bit) or gen2 (64-bit) skips
        uint64_t           Unused_3_1  :  3; // Unused
        uint64_t                  SCR  :  1; // If set the training pattern is scrambled. This field applies only if GEN2 and PAT2 are set. Both ends of a Gen2 link must be set to the same value. The primary intent of this field is to disable scrambling during pre-silicon validation for debug.
        uint64_t           Unused_7_5  :  3; // Unused
        uint64_t                 PAT2  :  1; // If set the Gen2 training pattern is transmitted and expected to be received. If a 1'b0 the Gen1 training pattern is selected. If the link partner is a Gen2 device both ends of the link must have the same setting. If the link partner is a Gen1 device the value in this field is overridden by GEN2==1'b0 in the RTL, and is forced to select the Gen1 training pattern. This field is intended to be a pre-silicon validation debug vehicle where GEN2 devices can be linked using the Gen1 training pattern.
        uint64_t          Unused_11_9  :  3; // Unused
        uint64_t                 PRQS  :  1; // Send PRQS PAM4 symbols if set to 1'b1. Sends effective NRZ(PAM4 symbols 0 and 3) if 1'b0 and gray coding is disabled. Sends effective NRZ(PAM4 symbols 0 and 2) if 1'b0 and gray coding is enabled.
        uint64_t         Unused_15_13  :  3; // Unused
        uint64_t                 GRAY  :  1; // If Gray coding is enabled in the SerDes this field must be set.
        uint64_t         Unused_63_17  : 47; // Unused
    } field;
    uint64_t val;
} LCB_CFG_LINK_PARTNER_GEN_t;

// LCB_CFG_REPLAY_PROTOCOL desc:
typedef union {
    struct {
        uint64_t                 GEN2  :  1; // 
        uint64_t           Unused_3_1  :  3; // Unused
        uint64_t      USE_RETRYSEQNUM  :  1; // Not connected. RetrySeqNum's are always used or enabled in Gen2 mode.
        uint64_t           Unused_7_5  :  3; // Unused
        uint64_t  RESYNC_NULL_CNT_MAX  :  3; // set REINIT_NULL_CNT_MAX and RESYNC_NULL_CNT_MAX to the same value greater than 3'd2
        uint64_t            Unused_11  :  1; // Unused
        uint64_t  REINIT_NULL_CNT_MAX  :  3; // set REINIT_NULL_CNT_MAX and RESYNC_NULL_CNT_MAX to the same value greater than 3'd2
        uint64_t            Unused_15  :  1; // Unused
        uint64_t         Unused_63_16  : 48; // Unused
    } field;
    uint64_t val;
} LCB_CFG_REPLAY_PROTOCOL_t;

// LCB_CFG_PORT_MIRROR desc:
typedef union {
    struct {
        uint64_t            TX_LUT_NE  :  1; // Link under test. Transmitted flits are put on the NE port mirror output bus. Not used on FZC.
        uint64_t            RX_LUT_SW  :  1; // Link under test. Received flits are put on the SW port mirror output bus. Not used on FZC.
        uint64_t            TX_LUT_SW  :  1; // Link under test. Transmitted flits are put on the SW port mirror output bus. Not used on FZC.
        uint64_t            RX_LUT_NE  :  1; // Link under test. Received flits are put on the NE port mirror output bus. Not used on FZC.
        uint64_t         DN_MIRROR_NE  :  1; // NE port mirror input bus DnStrm port. Tx side uses NE port mirror bus. Rx side ignores CRC errors (LCB_CFG_CRC_MODE field DISABLE_CRC_CHK needs to be set), doesn't drive FPC, doesn't drive the mirror buses. If both DN_MIRROR buses are selected, DN_MIRROR_NE takes precedence. Not used on FZC.
        uint64_t         DN_MIRROR_SW  :  1; // SW port mirror input bus DnStrm port. Tx side uses SW port mirror bus. Rx side ignores CRC errors (LCB_CFG_CRC_MODE field DISABLE_CRC_CHK needs to be set), doesn't drive FPC, doesn't drive the mirror buses. If both DN_MIRROR buses are selected, DN_MIRROR_NE takes precedence. Not used on FZC.
        uint64_t           Unused_7_6  :  2; // Unused
        uint64_t         UP_MIRROR_NE  :  1; // UpStrm port mirror sent to NE port mirror output bus. The Rx is receiving mirrored flits and ignoring CRC errors. (LCB_CFG_CRC_MODE field DISABLE_CRC_CHK needs to be set). If APR-OC the mirrored flits can be put on both port mirror buses by setting both UP_MIRROR_* fields. The mirrored flits need to be killed and not sent to the tile/FPC. If FXR-FZC the mirrored flits are sent to the link mux as normal. The Tx side of the LCB behaves normally even though it will not be used and will not be receiving dflit requests from the APR FPC or FXR link mux. Either UP_MIRROR_SW or UP_MIRROR_NE must be set on a FXR-FZC debug port.
        uint64_t         UP_MIRROR_SW  :  1; // UpStrm port mirror sent to SW port mirror output bus. The Rx is receiving mirrored flits and ignoring CRC errors. (LCB_CFG_CRC_MODE field DISABLE_CRC_CHK needs to be set). If APR-OC the mirrored flits can be put on both port mirror buses by setting both UP_MIRROR_* fields. The mirrored flits need to be killed and not sent to the tile/FPC. If FXR-FZC the mirrored flits are sent to the link mux as normal. The Tx side of the LCB behaves normally even though it will not be used and will not be receiving dflit requests from the APR FPC or FXR link mux. Either UP_MIRROR_SW or UP_MIRROR_NE must be set on a FXR-FZC debug port.
        uint64_t         Unused_63_10  : 54; // Unused
    } field;
    uint64_t val;
} LCB_CFG_PORT_MIRROR_t;

// LCB_CFG_TIME_SYNC_0 desc:
typedef union {
    struct {
        uint64_t      FLIT_GEN_PERIOD  : 12; // master sync flit generation periodicity - 100ms default period must be configurable in the range of 1 mSec to 1 Sec. bits [31:20] of clock counter when a new sync flit pair is generated. Default configuration is for 1.2Ghz. For APR/950Mhz use 12'h05A
        uint64_t            MASTER_ID  :  2; // master clock ID
        uint64_t         Unused_15_14  :  2; // Unused
        uint64_t        MASTER_ACTIVE  :  1; // Active Master, when set time sync flit pairs will be generated
        uint64_t         Unused_19_17  :  3; // Unused
        uint64_t       MASTER_CAPABLE  :  1; // Ports advertise to the Fabric Manager whether they support Master Clock functions
        uint64_t         Unused_23_21  :  3; // Unused
        uint64_t           UP_TREE_EN  :  4; // UpStrm clock tree ID 4 bit enable field for the 4 possible trees. MasterTime flits that arrive at a switch port that is not configured as an upstream port for the indicated clock tree are ignored, and an error is logged. MasterTime flits are put on both port mirror buses. Each port can be enabled to upstream zero through four clock trees. For each clock tree a port can either upstream, downstream, or do neither. A port cannot be both upstream and downstream on the same clock tree.
        uint64_t    MASTER_DN_TREE_EN  :  4; // When the port is an Active Master use this for the DnStrm clock tree ID 4 bit enable field for the 4 possible trees. It is one-hot field that must match the MASTER_ID value. For example, if MASTER_ID=2'b10, this field must be = 4'b0100 for the node to be ending Master TimeSync flits through the TX wire.
        uint64_t        SW_DN_TREE_EN  :  4; // SW port mirror input bus DnStrm clock tree ID 4 bit enable field for the 4 possible trees Each port can be enabled to downstream zero through four time sync clock trees. If a master time sync flit belonging to an enabled tree arrives on the SW port mirror bus, it is transmitted onto the wire.
        uint64_t        NE_DN_TREE_EN  :  4; // NE port mirror input bus DnStrm clock tree ID 4 bit enable field for the 4 possible trees. Each port can be enabled to downstream zero through four time sync clock trees. If a master time sync flit belonging to an enabled tree arrives on the NE port mirror bus, it is transmitted onto the wire.
        uint64_t        DISABLE_SNAPS  :  4; // Disable snaps for atomicity, effects MasterTimeLow, MasterTimeHigh, LocalTime, ARTCopy
        uint64_t             SNAP_NOW  :  4; // snap both the local and ART counter, for debug, auto clears. Overrides DISABLE_SNAPS. Hold MasterTimeLow, MasterTimeHigh.
        uint64_t         RST_LCL_CNTR  :  1; // Reset the local Master/Slave counter, level sensitive, set then release
        uint64_t         Unused_51_49  :  3; // Unused
        uint64_t         RST_ART_CNTR  :  1; // Reset the ART counter in FZC, level sensitive, set then release
        uint64_t         Unused_63_53  : 11; // Unused
    } field;
    uint64_t val;
} LCB_CFG_TIME_SYNC_0_t;

// LCB_CFG_TIME_SYNC_1 desc:
typedef union {
    struct {
        uint64_t             TSYN_INC  : 38; // TSYN_INC clock inc 38 bits. Default configuration is for 1.2Ghz. For APR/950Mhz use 38'h010D79435E
        uint64_t         Unused_63_38  : 26; // Unused
    } field;
    uint64_t val;
} LCB_CFG_TIME_SYNC_1_t;

// LCB_CFG_TIME_SYNC_2 desc:
typedef union {
    struct {
        uint64_t             TSYN_ADJ  : 32; // [31] sign bit - 0 = positive, 1 = negative [30:0] value to be added to or subtracted from tsyn_time.
        uint64_t         TSYN_ADJ_CNT  : 16; // Number of counts TSYN_ADJ is applied to tsyn_time. This field decrements each time TSYN_ADJ it is applied.
        uint64_t         Unused_63_48  : 16; // Unused
    } field;
    uint64_t val;
} LCB_CFG_TIME_SYNC_2_t;

// LCB_CFG_BUBBLE_SQUASHING desc:
typedef union {
    struct {
        uint64_t                   EN  :  1; // When set to a 1'b1 bubble squashing is enabled.
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} LCB_CFG_BUBBLE_SQUASHING_t;

// LCB_CFG_FEC_MODE desc:
typedef union {
    struct {
        uint64_t               TX_VAL  :  2; // 0: FEC is disabled 1: RS(132,128) is enabled 2: RS(528,512) is enabled 3: Reserved
        uint64_t           Unused_3_2  :  2; // Unused
        uint64_t       USE_RX_CFG_VAL  :  1; // ignore the value delivered in training pattern from the Tx side
        uint64_t           Unused_7_5  :  3; // Unused
        uint64_t               RX_VAL  :  2; // Used only when USE_RX_CFG_VAL is set. 0: FEC is disabled 1: RS(132,128) is enabled 2: RS(528,512) is enabled 3: Reserved
        uint64_t         Unused_11_10  :  2; // Unused
        uint64_t      DISABLE_DEC_ERR  :  1; // Used in port mirror applications. FEC decode error output is disabled and will not generate link level replays.
        uint64_t         Unused_15_13  :  3; // Unused
        uint64_t       UCERR_FEC_CTRS  :  1; // Default: OFF. FEC counters stop counting when uncorrectable errors are seen or correctable errors =2(SMALL FEC) and =8(BIG FEC).Only lane error counters and error mask captures are affected .
        uint64_t         Unused_63_17  : 47; // Unused
    } field;
    uint64_t val;
} LCB_CFG_FEC_MODE_t;

// LCB_CFG_FEC_INTERRUPT desc:
typedef union {
    struct {
        uint64_t              ERR_CNT  : 48; // Error count to interrupt on
        uint64_t             MATCH_EN  : 14; // multiple bits can be set to trigger an interrupt when any of the enabled error cnts reaches (exact match only) the ERR_CNT field. 0: lane 3 FEC error cnt[47:0] 1: lane 2 FEC error cnt[47:0] 2: lane 1 FEC error cnt[47:0] 3: lane 0 FEC error cnt[47:0] 4: FEC uncorrectable error cnt[31:0] 5: FEC eight symbols corrected error cnt[31:0] 6: FEC seven symbols corrected error cnt[31:0] 7: FEC six symbols corrected error cnt[31:0] 8: FEC five symbols corrected error cnt[31:0] 9: FEC four symbols corrected error cnt[47:0] 10: FEC three symbols corrected error cnt[47:0] 11: FEC two symbols corrected error cnt[47:0] 12: FEC one symbol corrected error cnt[47:0] 13: FEC correctable error mask miss counter[31:0]
        uint64_t         Unused_63_62  :  2; // Unused
    } field;
    uint64_t val;
} LCB_CFG_FEC_INTERRUPT_t;

// LCB_CFG_FEC_LN_DEGRADE desc:
typedef union {
    struct {
        uint64_t                   EN  :  1; // Enable
        uint64_t                  SEQ  :  1; // Sequential (1'b1) or Random Walk (1'b0) mode.
        uint64_t           Unused_3_2  :  2; // Unused
        uint64_t              ERR_LIM  : 24; // The threshold for each lanes error limit counter. This counter along with WIN_LIM establishes a BER threshold for each sliding window. ERR_LIM = WIN_LIM*80*BER. Default values set the BER lane degrade threshold to ~1e-5. Increasing WIN_LIM and/or DEGRADE_LIM improves accuracy but with longer time constants. The settings automatically scale with bit rate and lane width.
        uint64_t          DEGRADE_LIM  :  4; // The threshold for each lanes degrade limit counter. When a counter reaches DEGRADE_LIM a lane degrade is executed via a forced Recovery. If SEQ is enabled ERR_LIM must be reached in sequential sliding windows in culprit lane(s). If random walk mode is enabled, the degrade counter in each lane increments if it is a culprit, and decrements if it is not. The degrade limit counters starts at 4'h0, and can only increment from 4'h0.
        uint64_t              WIN_LIM  : 32; // Sliding window length in units of valid FEC decoder outputs (each valid output is 80bits for big FEC, 64bits for small FEC). The window is terminated and restarted by one of two events. Either the natural window interval set by WIN_LIM expires or an error limit counter reaches it's threshold ERR_LIM. 1.) When the counter expires naturally, the BER threshold has not been reached by any lanes (no culprits). 2.) If termination is due to an error limit counter reaching ERR_LIM the BER threshold has been reached in one or more culprit lanes. When this happens the degrade limit counter is incremented in the culprit lane(s).
    } field;
    uint64_t val;
} LCB_CFG_FEC_LN_DEGRADE_t;

// LCB_CFG_MISCA desc:
typedef union {
    struct {
        uint64_t    REQUIRED_MODE_CNT  :  6; // TRUE_CNT and MISC_CNT threshold for q_crc_mode, q_fec_mode, and tx_lane_en_from_peer
        uint64_t           Unused_7_6  :  2; // Unused
        uint64_t    REQUIRED_KILL_CNT  :  5; // If tx_ton count is >= required_kill_count and < required_tos_cnt, then the count falls into a region of uncertainty and the link is killed for a lost turn on signal.
        uint64_t         Unused_15_13  :  3; // Unused
        uint64_t     LANE_ID_DET_GEN1  :  5; // Per lane ID detection threshold for Gen1 training pattern enabled
        uint64_t         Unused_23_21  :  3; // Unused
        uint64_t     LANE_ID_DET_GEN2  :  5; // Per lane ID detection threshold for Gen2 training pattern enabled
        uint64_t         Unused_31_29  :  3; // Unused
        uint64_t      LENIENT_FRAMING  :  1; // Select lenient framing
        uint64_t         Unused_35_33  :  3; // Unused
        uint64_t REQUIRED_STALL_KILL_CNT  :  5; // If rx_stall_cnt is >= required_stall_kill_cnt and < required_stall_cnt, then the count falls into a region of uncertainty and the link is killed for a lost stall signal.
        uint64_t         Unused_63_41  : 23; // Unused
    } field;
    uint64_t val;
} LCB_CFG_MISCA_t;

// LCB_ERR_STS desc:
typedef union {
    struct {
        uint64_t RST_FOR_FAILED_DESKEW  :  1; // self explanatory
        uint64_t ALL_LNS_FAILED_REINIT_TEST  :  1; // All four lanes failed testing during reinit, triggers a reset.
        uint64_t LOST_REINIT_STALL_OR_TOS  :  1; // lost clock stall or turn on signal during reinit. Only active when just 1 lane operating on Rx side.
        uint64_t TX_LESS_THAN_FOUR_LNS  :  1; // self explanatory
        uint64_t RX_LESS_THAN_FOUR_LNS  :  1; // self explanatory
        uint64_t          SEQ_CRC_ERR  :  1; // A sequential CRC error was encountered. This triggers a reinit sequence.
        uint64_t     REINIT_FROM_PEER  :  1; // A reinit sequence was triggered by the peer.
        uint64_t REINIT_FOR_LN_DEGRADE  :  1; // A reinit sequence was triggered during which a lane was removed from operation.
        uint64_t CRC_ERR_CNT_HIT_LIMIT  :  1; // Programmed using the LCB_CFG_CRC_INTERRUPT CSR.
        uint64_t         RCLK_STOPPED  :  1; // The (1 of 4) lane receive clock that is being used on the Rx side/pipe during LTP mode stopped toggling.This is catastrophic and will take down the link.
        uint64_t UNEXPECTED_REPLAY_MARKER  :  1; // self explanatory
        uint64_t UNEXPECTED_ROUND_TRIP_MARKER  :  1; // self explanatory
        uint64_t     ILLEGAL_NULL_LTP  :  1; // link transfer LTPs
        uint64_t ILLEGAL_FLIT_ENCODING  :  1; // The only legal flits from the FPE are head, body, tail, Idle, CrdtRet, HeadBadPkt, BodyBadPkt, TailBadPkt, SPC, SCMrkr, and the architecturally hidden ForceIdle ([64:56] = 9'h002).
        uint64_t  FLIT_INPUT_BUF_OFLW  :  1; // self explanatory
        uint64_t VL_ACK_INPUT_BUF_OFLW  :  1; // self explanatory
        uint64_t VL_ACK_INPUT_PARITY_ERR  :  1; // self explanatory
        uint64_t VL_ACK_INPUT_WRONG_CRC_MODE  :  1; // VC ack input valid and not in 14 bit CRC mode
        uint64_t   FLIT_INPUT_BUF_MBE  :  1; // self explanatory
        uint64_t   FLIT_INPUT_BUF_SBE  :  1; // self explanatory
        uint64_t       REPLAY_BUF_MBE  :  1; // self explanatory
        uint64_t       REPLAY_BUF_SBE  :  1; // self explanatory
        uint64_t RST_FOR_LINK_TIMEOUT  :  1; // A reinit sequence is triggered in a last ditch attempt at keeping the link up when the timer expires. If this succeeds the NEG_EDGE_LINK_TRANSFER_ACTIVE and HOLD_REINIT flags will remain clear.
        uint64_t RST_FOR_INCOMPLT_RND_TRIP  :  1; // A reinit sequence is triggered in a last ditch attempt at keeping the link up when a round trip marker fails to return. If this succeeds the NEG_EDGE_LINK_TRANSFER_ACTIVE and HOLD_REINIT flags will remain clear.
        uint64_t          HOLD_REINIT  :  1; // This indicates the link will not come up. Useful when the link has never been up and the NEG_EDGE_LINK_TRANSFER_ACTIVE flag will be clear.
        uint64_t NEG_EDGE_LINK_TRANSFER_ACTIVE  :  1; // self explanatory
        uint64_t UNEXPECTED_MASTER_TIME_FLIT  :  1; // MasterTime flits that arrive at a switch port that is not configured as an upstream port for the indicated clock tree are ignored, and an error is logged.
        uint64_t           QUARANTINE  :  1; // KNH/FXR viral event to take down the link. Tied low in OC.
        uint64_t CSR_CHAIN_PARITY_ERR  :  1; // OC CSR chain parity error on the input request bus. Tied low on FZC.
        uint64_t CSR_CPORT_ADDRESS_MISS  :  1; // OC CSR address miss or timeout on a request from Cport. Tied low on FZC.
        uint64_t               PM_MBE  :  1; // Multi bit error detected in a high priority DN stream MasterTime flit.
        uint64_t               PM_SBE  :  1; // Single bit error detected in a high priority DN stream MasterTime flit.
        uint64_t FEC_ERR_CNT_HIT_LIMIT  :  1; // Programmed using the LCB_CFG_FEC_INTERRUPT CSR.
        uint64_t         Unused_63_33  : 31; // Unused
    } field;
    uint64_t val;
} LCB_ERR_STS_t;

// LCB_ERR_CLR desc:
typedef union {
    struct {
        uint64_t RST_FOR_FAILED_DESKEW  :  1; // see error flags
        uint64_t ALL_LNS_FAILED_REINIT_TEST  :  1; // see error flags
        uint64_t LOST_REINIT_STALL_OR_TOS  :  1; // see error flags
        uint64_t TX_LESS_THAN_FOUR_LNS  :  1; // see error flags
        uint64_t RX_LESS_THAN_FOUR_LNS  :  1; // see error flags
        uint64_t          SEQ_CRC_ERR  :  1; // see error flags
        uint64_t     REINIT_FROM_PEER  :  1; // see error flags
        uint64_t REINIT_FOR_LN_DEGRADE  :  1; // see error flags
        uint64_t CRC_ERR_CNT_HIT_LIMIT  :  1; // see error flags
        uint64_t         RCLK_STOPPED  :  1; // see error flags
        uint64_t UNEXPECTED_REPLAY_MARKER  :  1; // see error flags
        uint64_t UNEXPECTED_ROUND_TRIP_MARKER  :  1; // see error flags
        uint64_t     ILLEGAL_NULL_LTP  :  1; // see error flags
        uint64_t ILLEGAL_FLIT_ENCODING  :  1; // see error flags
        uint64_t  FLIT_INPUT_BUF_OFLW  :  1; // see error flags
        uint64_t VL_ACK_INPUT_BUF_OFLW  :  1; // see error flags
        uint64_t VL_ACK_INPUT_PARITY_ERR  :  1; // see error flags
        uint64_t VL_ACK_INPUT_WRONG_CRC_MODE  :  1; // see error flags
        uint64_t   FLIT_INPUT_BUF_MBE  :  1; // see error flags
        uint64_t   FLIT_INPUT_BUF_SBE  :  1; // see error flags
        uint64_t       REPLAY_BUF_MBE  :  1; // see error flags
        uint64_t       REPLAY_BUF_SBE  :  1; // see error flags
        uint64_t RST_FOR_LINK_TIMEOUT  :  1; // see error flags
        uint64_t RST_FOR_INCOMPLT_RND_TRIP  :  1; // see error flags
        uint64_t          HOLD_REINIT  :  1; // see error flags
        uint64_t NEG_EDGE_LINK_TRANSFER_ACTIVE  :  1; // see error flags
        uint64_t UNEXPECTED_MASTER_TIME_FLIT  :  1; // see error flags
        uint64_t           QUARANTINE  :  1; // see error flags
        uint64_t CSR_CHAIN_PARITY_ERR  :  1; // see error flags
        uint64_t CSR_CPORT_ADDRESS_MISS  :  1; // see error flags
        uint64_t               PM_MBE  :  1; // see error flags
        uint64_t               PM_SBE  :  1; // see error flags
        uint64_t FEC_ERR_CNT_HIT_LIMIT  :  1; // see error flags
        uint64_t         Unused_63_33  : 31; // Unused
    } field;
    uint64_t val;
} LCB_ERR_CLR_t;

// LCB_ERR_FRC desc:
typedef union {
    struct {
        uint64_t RST_FOR_FAILED_DESKEW  :  1; // see error flags
        uint64_t ALL_LNS_FAILED_REINIT_TEST  :  1; // see error flags
        uint64_t LOST_REINIT_STALL_OR_TOS  :  1; // see error flags
        uint64_t TX_LESS_THAN_FOUR_LNS  :  1; // see error flags
        uint64_t RX_LESS_THAN_FOUR_LNS  :  1; // see error flags
        uint64_t          SEQ_CRC_ERR  :  1; // see error flags
        uint64_t     REINIT_FROM_PEER  :  1; // see error flags
        uint64_t REINIT_FOR_LN_DEGRADE  :  1; // see error flags
        uint64_t CRC_ERR_CNT_HIT_LIMIT  :  1; // see error flags
        uint64_t         RCLK_STOPPED  :  1; // see error flags
        uint64_t UNEXPECTED_REPLAY_MARKER  :  1; // see error flags
        uint64_t UNEXPECTED_ROUND_TRIP_MARKER  :  1; // see error flags
        uint64_t     ILLEGAL_NULL_LTP  :  1; // see error flags
        uint64_t ILLEGAL_FLIT_ENCODING  :  1; // see error flags
        uint64_t  FLIT_INPUT_BUF_OFLW  :  1; // see error flags
        uint64_t VL_ACK_INPUT_BUF_OFLW  :  1; // see error flags
        uint64_t VL_ACK_INPUT_PARITY_ERR  :  1; // see error flags
        uint64_t VL_ACK_INPUT_WRONG_CRC_MODE  :  1; // see error flags
        uint64_t   FLIT_INPUT_BUF_MBE  :  1; // see error flags
        uint64_t   FLIT_INPUT_BUF_SBE  :  1; // see error flags
        uint64_t       REPLAY_BUF_MBE  :  1; // see error flags
        uint64_t       REPLAY_BUF_SBE  :  1; // see error flags
        uint64_t RST_FOR_LINK_TIMEOUT  :  1; // see error flags
        uint64_t RST_FOR_INCOMPLT_RND_TRIP  :  1; // see error flags
        uint64_t          HOLD_REINIT  :  1; // see error flags
        uint64_t NEG_EDGE_LINK_TRANSFER_ACTIVE  :  1; // see error flags
        uint64_t UNEXPECTED_MASTER_TIME_FLIT  :  1; // see error flags
        uint64_t           QUARANTINE  :  1; // see error flags
        uint64_t CSR_CHAIN_PARITY_ERR  :  1; // see error flags
        uint64_t CSR_CPORT_ADDRESS_MISS  :  1; // see error flags
        uint64_t               PM_MBE  :  1; // see error flags
        uint64_t               PM_SBE  :  1; // see error flags
        uint64_t FEC_ERR_CNT_HIT_LIMIT  :  1; // see error flags
        uint64_t         Unused_63_33  : 31; // Unused
    } field;
    uint64_t val;
} LCB_ERR_FRC_t;

// LCB_ERR_EN_HOST desc:
typedef union {
    struct {
        uint64_t RST_FOR_FAILED_DESKEW  :  1; // see error flags
        uint64_t ALL_LNS_FAILED_REINIT_TEST  :  1; // see error flags
        uint64_t LOST_REINIT_STALL_OR_TOS  :  1; // see error flags
        uint64_t TX_LESS_THAN_FOUR_LNS  :  1; // see error flags
        uint64_t RX_LESS_THAN_FOUR_LNS  :  1; // see error flags
        uint64_t          SEQ_CRC_ERR  :  1; // see error flags
        uint64_t     REINIT_FROM_PEER  :  1; // see error flags
        uint64_t REINIT_FOR_LN_DEGRADE  :  1; // see error flags
        uint64_t CRC_ERR_CNT_HIT_LIMIT  :  1; // see error flags
        uint64_t         RCLK_STOPPED  :  1; // see error flags
        uint64_t UNEXPECTED_REPLAY_MARKER  :  1; // see error flags
        uint64_t UNEXPECTED_ROUND_TRIP_MARKER  :  1; // see error flags
        uint64_t     ILLEGAL_NULL_LTP  :  1; // see error flags
        uint64_t ILLEGAL_FLIT_ENCODING  :  1; // see error flags
        uint64_t  FLIT_INPUT_BUF_OFLW  :  1; // see error flags
        uint64_t VL_ACK_INPUT_BUF_OFLW  :  1; // see error flags
        uint64_t VL_ACK_INPUT_PARITY_ERR  :  1; // see error flags
        uint64_t VL_ACK_INPUT_WRONG_CRC_MODE  :  1; // see error flags
        uint64_t   FLIT_INPUT_BUF_MBE  :  1; // see error flags
        uint64_t   FLIT_INPUT_BUF_SBE  :  1; // see error flags
        uint64_t       REPLAY_BUF_MBE  :  1; // see error flags
        uint64_t       REPLAY_BUF_SBE  :  1; // see error flags
        uint64_t RST_FOR_LINK_TIMEOUT  :  1; // see error flags
        uint64_t RST_FOR_INCOMPLT_RND_TRIP  :  1; // see error flags
        uint64_t          HOLD_REINIT  :  1; // see error flags
        uint64_t NEG_EDGE_LINK_TRANSFER_ACTIVE  :  1; // see error flags
        uint64_t UNEXPECTED_MASTER_TIME_FLIT  :  1; // see error flags
        uint64_t           QUARANTINE  :  1; // see error flags
        uint64_t CSR_CHAIN_PARITY_ERR  :  1; // see error flags
        uint64_t CSR_CPORT_ADDRESS_MISS  :  1; // see error flags
        uint64_t               PM_MBE  :  1; // see error flags
        uint64_t               PM_SBE  :  1; // see error flags
        uint64_t FEC_ERR_CNT_HIT_LIMIT  :  1; // see error flags
        uint64_t         Unused_63_33  : 31; // Unused
    } field;
    uint64_t val;
} LCB_ERR_EN_HOST_t;

// LCB_ERR_FIRST_HOST desc:
typedef union {
    struct {
        uint64_t RST_FOR_FAILED_DESKEW  :  1; // see error flags
        uint64_t ALL_LNS_FAILED_REINIT_TEST  :  1; // see error flags
        uint64_t LOST_REINIT_STALL_OR_TOS  :  1; // see error flags
        uint64_t TX_LESS_THAN_FOUR_LNS  :  1; // see error flags
        uint64_t RX_LESS_THAN_FOUR_LNS  :  1; // see error flags
        uint64_t          SEQ_CRC_ERR  :  1; // see error flags
        uint64_t     REINIT_FROM_PEER  :  1; // see error flags
        uint64_t REINIT_FOR_LN_DEGRADE  :  1; // see error flags
        uint64_t CRC_ERR_CNT_HIT_LIMIT  :  1; // see error flags
        uint64_t         RCLK_STOPPED  :  1; // see error flags
        uint64_t UNEXPECTED_REPLAY_MARKER  :  1; // see error flags
        uint64_t UNEXPECTED_ROUND_TRIP_MARKER  :  1; // see error flags
        uint64_t     ILLEGAL_NULL_LTP  :  1; // see error flags
        uint64_t ILLEGAL_FLIT_ENCODING  :  1; // see error flags
        uint64_t  FLIT_INPUT_BUF_OFLW  :  1; // see error flags
        uint64_t VL_ACK_INPUT_BUF_OFLW  :  1; // see error flags
        uint64_t VL_ACK_INPUT_PARITY_ERR  :  1; // see error flags
        uint64_t VL_ACK_INPUT_WRONG_CRC_MODE  :  1; // see error flags
        uint64_t   FLIT_INPUT_BUF_MBE  :  1; // see error flags
        uint64_t   FLIT_INPUT_BUF_SBE  :  1; // see error flags
        uint64_t       REPLAY_BUF_MBE  :  1; // see error flags
        uint64_t       REPLAY_BUF_SBE  :  1; // see error flags
        uint64_t RST_FOR_LINK_TIMEOUT  :  1; // see error flags
        uint64_t RST_FOR_INCOMPLT_RND_TRIP  :  1; // see error flags
        uint64_t          HOLD_REINIT  :  1; // see error flags
        uint64_t NEG_EDGE_LINK_TRANSFER_ACTIVE  :  1; // see error flags
        uint64_t UNEXPECTED_MASTER_TIME_FLIT  :  1; // see error flags
        uint64_t           QUARANTINE  :  1; // see error flags
        uint64_t CSR_CHAIN_PARITY_ERR  :  1; // see error flags
        uint64_t CSR_CPORT_ADDRESS_MISS  :  1; // see error flags
        uint64_t               PM_MBE  :  1; // see error flags
        uint64_t               PM_SBE  :  1; // see error flags
        uint64_t FEC_ERR_CNT_HIT_LIMIT  :  1; // see error flags
        uint64_t         Unused_63_33  : 31; // Unused
    } field;
    uint64_t val;
} LCB_ERR_FIRST_HOST_t;

// LCB_DBG_ERRINJ_CRC_0 desc:
typedef union {
    struct {
        uint64_t       CRC_ERR_XFR_EN  : 34; // Tx, lane(s) xfr enable for CRC errors. This is the location(s) in the LTP where the errors will occur. Lane(s) selection is dependent on the number of active lanes. CRC_ERR_LN_EN also must be set for any particular lane to have errors.
        uint64_t         Unused_35_34  :  2; // Unused
        uint64_t        CRC_ERR_LN_EN  :  4; // Tx, lane(s) enable for CRC errors. Appropriate CRC_ERR_XFR_EN must also be set for any particular lane to have errors.
        uint64_t       RANDOM_XFR_ERR  :  1; // Tx, if random is set have CRC_ERR_XFR_EN all high, else if random clear use CRC_ERR_XFR_EN directly where only one bit should be set Using random set will not guarantee that at least one xfr in a LTP will have an error.
        uint64_t         Unused_43_41  :  3; // Unused
        uint64_t   LTP_CNT_MAX_SELECT  :  2; // LTP_CNT_MAX 0: 0x3f 1:0x3ff 2: 0x7fff 3: 0xfffff
        uint64_t         Unused_47_46  :  2; // Unused
        uint64_t     NUM_CRC_LTP_ERRS  :  4; // Tx, max number or LTP CRC errors within LTP_CNT_MAX LTPs. 0 disables errors.
        uint64_t  CRC_ERR_PROBABILITY  :  4; // Tx, probability of forcing any LTP to have an error. This is on a LTP basis, not xfr basis. 3'd7=1:1,2,4,8,16,32,64,3'd0=1:128 4'd15 thru 4'd8= 1:2^9,2^12,2^15,2^18,2^21,2^24,2^27,2^30. 3'd7 sets sequential errors of num_crc_ltp_errs These probabilities are accurate only when RANDOM_XFR_ERR is clear and we are guaranteeing that a xfr in each LTP will be enabled for an error via CRC_ERR_XFR_EN
        uint64_t DISABLE_SEQUENTIAL_ERR  :  1; // Use to prevent sequential errors and the reinits they trigger.
        uint64_t         Unused_63_57  :  7; // Unused
    } field;
    uint64_t val;
} LCB_DBG_ERRINJ_CRC_0_t;

// LCB_DBG_ERRINJ_CRC_1 desc:
typedef union {
    struct {
        uint64_t       CRC_ERR_BIT_EN  : 32; // Enables bit flips in the masked positions, when the lane error injection signal strobes.
        uint64_t                 SEED  : 31; // 
        uint64_t            Unused_63  :  1; // Unused
    } field;
    uint64_t val;
} LCB_DBG_ERRINJ_CRC_1_t;

// LCB_DBG_ERRINJ_ECC desc:
typedef union {
    struct {
        uint64_t                   EN  :  1; // Enable Error Injection
        uint64_t           Unused_3_1  :  3; // Unused
        uint64_t                 MODE  :  2; // Mode 0: Inject error once, clear enable to rearm Mode 1: Inject error always Mode 2: Inject error once if address matches, clear enable to rearm Mode 3: Inject error always if address matches Mode 0,2 will wait until the replay buffer is being used.
        uint64_t           Unused_7_6  :  2; // Unused
        uint64_t            RAMSELECT  : 17; // -- If injection is happening to the Replay Buffer, then REPLAY_BUF_ADDRESS points to a specific Qflit, and the fields here are 1-hot fields to select one of the 4-flits, for which the error detection syndrome is injected with errors according to the CHECKBYTE mask. [11:8]: Enable errinj for Replay_Buf RAM 0 [15:12]: Enable errinj for Replay_Buf RAM 1 [16] Enable errinj for Replay Buffer side RAM [20:17]: Enable errinj for input Buf [24:21] Enable errinj for DN stream MasterTime flits.
        uint64_t         Unused_27_25  :  3; // Unused
        uint64_t            CHECKBYTE  :  8; // When an error is injected, each bit that is set to one in this field causes the corresponding bit of the error detection syndrome for the memory address read to be inverted.
        uint64_t    INPUT_BUF_ADDRESS  :  5; // This field indicates the address within the input buffer for which error injection is to occur. This field is used only when the error injection mode is 2 or 3.
        uint64_t         Unused_43_41  :  3; // Unused
        uint64_t   REPLAY_BUF_ADDRESS  :  9; // -- Indicates the address within the replay buffer rams for which error injection is to occur, when MODE is 2 or 3. -- There are 2 RAM's handling flit data for replay (and sidedata RAM). -- Every one of these RAM's outputs 1 Qflit every cycle that the replay buffer is read. -- RAM's take 9-bit address and output 1 Qflit. -- 1 9-bit address will match a spot in RAM_0 and RAM_1, so RAMSELECT below selects which RAM and selects the specific flit out of the Qflit.
        uint64_t         Unused_55_53  :  3; // Unused
        uint64_t     CLR_ECC_ERR_CNTS  :  1; // Clear all ECC error counts
        uint64_t         Unused_63_57  :  7; // Unused
    } field;
    uint64_t val;
} LCB_DBG_ERRINJ_ECC_t;

// LCB_DBG_ERRINJ_MISC desc:
typedef union {
    struct {
        uint64_t     FORCE_MISCOMPARE  :  1; // On the Rx side write a 1 to force a single bad CRC to a good CRC. Auto clears when a bad CRC occurs. Use force_bad_crc_reinit to inject an error for auto-clear. This will not result in data corruption. To generate corrupted data, use LCB_DBG_ERRINJ_CRC_*. The first LTP with bit errors will be accepted, resulting in data corruption.
        uint64_t           Unused_3_1  :  3; // Unused
        uint64_t FORCE_EXTRA_FLIT_ACK  :  1; // On the Rx side write a 1 to force a single extra flit ack. Auto clears immediately. Tport will have one extra credit as a result of this error injection.
        uint64_t           Unused_7_5  :  3; // Unused
        uint64_t  FORCE_LOST_FLIT_ACK  :  1; // On the Rx side write a 1 to force a single lost flit ack. Auto clears when an Ack is generated. Otter Creek needs to be sending traffic to the input buffer, to generate Acks. Tport will be short one credit as a result of this error injection.
        uint64_t          Unused_11_9  :  3; // Unused
        uint64_t FORCE_BAD_CRC_REINIT  :  1; // On the Rx side write a 1 to force continuous bad CRCs until a single sequential bad CRC reinit is triggered. Auto clears immediately. LCB_ERR_INFO_SEQ_CRC_CNT will increment.
        uint64_t         Unused_15_13  :  3; // Unused
        uint64_t         FORCE_REPLAY  :  1; // On the Rx side write a 1 to force a single replay request. Auto clears immediately. LCB_ERR_INFO_RX_REPLAY_CNT will increment.
        uint64_t         Unused_19_17  :  3; // Unused
        uint64_t FORCE_VL_ACK_INPUT_PBE  :  1; // Write a 1 to force a single parity error on the fpe_lcb_vl_ack bus. Auto clears and injects when a side band credit message is sent to the LCB. LCB_ERR_STS.vl_ack_input_parity_err will set. The LCB needs to be in CRC-14 mode (lcb_cfg_crc_mode = 2'd1)
        uint64_t         Unused_63_21  : 43; // Unused
    } field;
    uint64_t val;
} LCB_DBG_ERRINJ_MISC_t;

// LCB_DBG_INIT_STATE_0 desc:
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
} LCB_DBG_INIT_STATE_0_t;

// LCB_DBG_INIT_STATE_1 desc:
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
} LCB_DBG_INIT_STATE_1_t;

// LCB_DBG_INIT_STATE_2 desc:
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
} LCB_DBG_INIT_STATE_2_t;

// LCB_DBG_CFG_GOOD_BAD_DATA desc:
typedef union {
    struct {
        uint64_t                  ARM  :  1; // level sensitive, must set first and then clear to arm
        uint64_t           Unused_3_1  :  3; // Unused
        uint64_t               SELECT  :  2; // 0: raw good unscrambled LTP data buffer 1: raw bad unscrambled LTP data buffer 2: raw bad scrambled LTP data buffer 3: low power mode
        uint64_t           Unused_7_6  :  2; // Unused
        uint64_t                 ADDR  :  3; // Address within the buffer. First clock of data is at address 0. Refer to OC MAS state tables.
        uint64_t            Unused_11  :  1; // Unused
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
} LCB_DBG_CFG_GOOD_BAD_DATA_t;

// LCB_DBG_STS_GOOD_BAD_DATA desc:
typedef union {
    struct {
        uint64_t                READY  :  1; // 
        uint64_t           Unused_3_1  :  3; // Unused
        uint64_t         LTP_CNT_GOOD  :  3; // The captured good LTP number. Refer to OC MAS state tables.
        uint64_t             Unused_7  :  1; // Unused
        uint64_t          LTP_CNT_BAD  :  3; // The captured bad LTP number. Refer to OC MAS state tables.
        uint64_t            Unused_11  :  1; // Unused
        uint64_t          TOT_BAD_XFR  :  5; // total number of bad XFRs, for debug of error stat logic.
        uint64_t         Unused_63_17  : 47; // Unused
    } field;
    uint64_t val;
} LCB_DBG_STS_GOOD_BAD_DATA_t;

// LCB_DBG_STS_GOOD_BAD_DAT0 desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // Queued or captured LTP data in lane 0
    } field;
    uint64_t val;
} LCB_DBG_STS_GOOD_BAD_DAT0_t;

// LCB_DBG_STS_GOOD_BAD_DAT1 desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // Queued or captured LTP data in lane 1
    } field;
    uint64_t val;
} LCB_DBG_STS_GOOD_BAD_DAT1_t;

// LCB_DBG_STS_GOOD_BAD_DAT2 desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // Queued or captured LTP data in lane 2
    } field;
    uint64_t val;
} LCB_DBG_STS_GOOD_BAD_DAT2_t;

// LCB_DBG_STS_GOOD_BAD_DAT3 desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // Queued or captured LTP data in lane 3
    } field;
    uint64_t val;
} LCB_DBG_STS_GOOD_BAD_DAT3_t;

// LCB_DBG_STS_XFR_ERR_STATS desc:
typedef union {
    struct {
        uint64_t             XFR_1BAD  : 32; // clear by turning error stat mode off
        uint64_t             XFR_2BAD  : 16; // clear by turning error stat mode off
        uint64_t             XFR_3BAD  : 10; // clear by turning error stat mode off
        uint64_t             XFR_4BAD  :  6; // four or more. clear by turning error stat mode off
    } field;
    uint64_t val;
} LCB_DBG_STS_XFR_ERR_STATS_t;

// LCB_DBG_CFG_FEC_LN_0 desc:
typedef union {
    struct {
        uint64_t                  ROW  : 33; // Enables error injection at bit positions set to a 1'b1. The length of this field equals the length (in clock cycles) of the four lane code word beat sequence. This allows targeted error injection through out the FEC code word rows in this lane column. The beat sequence symbol map is an array of 33 rows and 4 columns. Along with the symbol mapping tables in the architecture specification the user can precisely place a known number of symbol errors within code words and physical lanes.
        uint64_t            ROW_SHIFT  :  1; // Enable circular shift of ROW at the start of each pass through the ROW sequence. Only use to randomize error locations when INJ_RATE is enabled.
        uint64_t                  SYM  :  8; // Represents the eight symbol positions in this lane. The ERR pattern is injected into symbol positions defined in this field.
        uint64_t                  ERR  : 10; // Error pattern injected into enabled symbol locations. RS(528,512) uses all ten bits. M=10 RS(132,128) uses the low eight bits. M=8
        uint64_t            SYM_SHIFT  :  1; // Enable circular shift of SYM each cycle of the beat sequence.
        uint64_t              RAN_ERR  :  1; // Use a random error pattern source
        uint64_t             INJ_RATE  :  5; // The baseline symbol error injection rate. Auto re-arms the EN one-shot which basically sets the beat sequence error injection rate. Each beat sequence contains 33 rows x 8 symbols per = 264 symbols. 0: Disabled 1-31: 1/264 * nchoosek(31,n) * 2^-31 Approximate SER in this lane is [SYM count] * [ROW count] * INJ_RATE BER values from ~1e-8 on up can be injected. Each lanes random number generator is seeded differently. The random number generator (LFSR) advances one cycle each beat sequence.
        uint64_t            LFSR_SEED  :  3; // Used to initialize the FEC LFSR.
        uint64_t         Unused_63_62  :  2; // Unused
    } field;
    uint64_t val;
} LCB_DBG_CFG_FEC_LN_0_t;

// LCB_DBG_CFG_FEC_LN_1 desc:
typedef union {
    struct {
        uint64_t                  ROW  : 33; // Enables error injection at bit positions set to a 1'b1. The length of this field equals the length (in clock cycles) of the four lane code word beat sequence. This allows targeted error injection through out the FEC code word rows in this lane column. The beat sequence symbol map is an array of 33 rows and 4 columns. Along with the symbol mapping tables in the architecture specification the user can precisely place a known number of symbol errors within code words and physical lanes.
        uint64_t            ROW_SHIFT  :  1; // Enable circular shift of ROW each cclk clock cycle.
        uint64_t                  SYM  :  8; // Represents the eight symbol positions in this lane. The ERR pattern is injected into symbol positions defined in this field.
        uint64_t                  ERR  : 10; // Error pattern injected into enabled symbol locations. RS(528,512) uses all ten bits. M=10 RS(132,128) uses the low eight bits. M=8
        uint64_t            SYM_SHIFT  :  1; // Enable circular shift of SYM each cclk clock cycle.
        uint64_t              RAN_ERR  :  1; // Use a random error pattern source
        uint64_t             INJ_RATE  :  5; // The baseline symbol error injection rate. Auto re-arms the EN one-shot which basically sets the beat sequence error injection rate. Each beat sequence contains 33 rows x 8 symbols per = 264 symbols. 0: Disabled 1-31: 1/264 * nchoosek(31,n) * 2^-31 Approximate SER in this lane is [SYM count] * [ROW count] * INJ_RATE BER values from ~1e-8 on up can be injected. Each lanes random number generator is seeded differently. The random number generator (LFSR) advances one cycle each beat sequence.
        uint64_t            LFSR_SEED  :  3; // Used to initialize the FEC LFSR.
        uint64_t         Unused_63_62  :  2; // Unused
    } field;
    uint64_t val;
} LCB_DBG_CFG_FEC_LN_1_t;

// LCB_DBG_CFG_FEC_LN_2 desc:
typedef union {
    struct {
        uint64_t                  ROW  : 33; // Enables error injection at bit positions set to a 1'b1. The length of this field equals the length (in clock cycles) of the four lane code word beat sequence. This allows targeted error injection through out the FEC code word rows in this lane column. The beat sequence symbol map is an array of 33 rows and 4 columns. Along with the symbol mapping tables in the architecture specification the user can precisely place a known number of symbol errors within code words and physical lanes.
        uint64_t            ROW_SHIFT  :  1; // Enable circular shift of ROW each cclk clock cycle.
        uint64_t                  SYM  :  8; // Represents the eight symbol positions in this lane. The ERR pattern is injected into symbol positions defined in this field.
        uint64_t                  ERR  : 10; // Error pattern injected into enabled symbol locations. RS(528,512) uses all ten bits. M=10 RS(132,128) uses the low eight bits. M=8
        uint64_t            SYM_SHIFT  :  1; // Enable circular shift of SYM each cclk clock cycle.
        uint64_t              RAN_ERR  :  1; // Use a random error pattern source
        uint64_t             INJ_RATE  :  5; // The baseline symbol error injection rate. Auto re-arms the EN one-shot which basically sets the beat sequence error injection rate. Each beat sequence contains 33 rows x 8 symbols per = 264 symbols. 0: Disabled 1-31: 1/264 * nchoosek(31,n) * 2^-31 Approximate SER in this lane is [SYM count] * [ROW count] * INJ_RATE BER values from ~1e-8 on up can be injected. Each lanes random number generator is seeded differently. The random number generator (LFSR) advances one cycle each beat sequence.
        uint64_t            LFSR_SEED  :  3; // Used to initialize the FEC LFSR.
        uint64_t         Unused_63_62  :  2; // Unused
    } field;
    uint64_t val;
} LCB_DBG_CFG_FEC_LN_2_t;

// LCB_DBG_CFG_FEC_LN_3 desc:
typedef union {
    struct {
        uint64_t                  ROW  : 33; // Enables error injection at bit positions set to a 1'b1. The length of this field equals the length (in clock cycles) of the four lane code word beat sequence. This allows targeted error injection through out the FEC code word rows in this lane column. The beat sequence symbol map is an array of 33 rows and 4 columns. Along with the symbol mapping tables in the architecture specification the user can precisely place a known number of symbol errors within code words and physical lanes.
        uint64_t            ROW_SHIFT  :  1; // Enable circular shift of ROW each cclk clock cycle.
        uint64_t                  SYM  :  8; // Represents the eight symbol positions in this lane. The ERR pattern is injected into symbol positions defined in this field.
        uint64_t                  ERR  : 10; // Error pattern injected into enabled symbol locations. RS(528,512) uses all ten bits. M=10 RS(132,128) uses the low eight bits. M=8
        uint64_t            SYM_SHIFT  :  1; // Enable circular shift of SYM each cclk clock cycle.
        uint64_t              RAN_ERR  :  1; // Use a random error pattern source
        uint64_t             INJ_RATE  :  5; // The baseline symbol error injection rate. Auto re-arms the EN one-shot which basically sets the beat sequence error injection rate. Each beat sequence contains 33 rows x 8 symbols per = 264 symbols. 0: Disabled 1-31: 1/264 * nchoosek(31,n) * 2^-31 Approximate SER in this lane is [SYM count] * [ROW count] * INJ_RATE BER values from ~1e-8 on up can be injected. Each lanes random number generator is seeded differently. The random number generator (LFSR) advances one cycle each beat sequence.
        uint64_t            LFSR_SEED  :  3; // Used to initialize the FEC LFSR.
        uint64_t         Unused_63_62  :  2; // Unused
    } field;
    uint64_t val;
} LCB_DBG_CFG_FEC_LN_3_t;

// LCB_DBG_CFG_ERRINJ_FEC desc:
typedef union {
    struct {
        uint64_t             LANE_EN0  :  1; // Physical lane enable mask. When set enables ROW error injection in the respective lane as described in LCB_DBG_CFG_FEC_LN_0/1/2/3. Error injection aligns with the code word beat sequence. This bit auto clears giving default one-shot functionality. Set these four bits simultaneously to start error injection synchronously into all four lanes.
        uint64_t             LANE_EN1  :  1; // 
        uint64_t             LANE_EN2  :  1; // 
        uint64_t             LANE_EN3  :  1; // 
        uint64_t          Unused_63_4  : 60; // Unused
    } field;
    uint64_t val;
} LCB_DBG_CFG_ERRINJ_FEC_t;

// LCB_DBG_ERRINJ_CRC_2 desc:
typedef union {
    struct {
        uint64_t       CRC_ERR_BIT_EN  : 32; // Enables bit flips in the masked positions, when the lane error injection signal strobes. This is the upper XFR of the XFR pair.
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} LCB_DBG_ERRINJ_CRC_2_t;

// LCB_ERR_INFO_TOTAL_CRC_ERR desc:
typedef union {
    struct {
        uint64_t                  CNT  : 64; // 
    } field;
    uint64_t val;
} LCB_ERR_INFO_TOTAL_CRC_ERR_t;

// LCB_ERR_INFO_CRC_ERR_LN0 desc:
typedef union {
    struct {
        uint64_t                  CNT  : 64; // Rx physical lane 0. Cleared with CLR_CRC_ERR_CNTRS.
    } field;
    uint64_t val;
} LCB_ERR_INFO_CRC_ERR_LN0_t;

// LCB_ERR_INFO_CRC_ERR_LN1 desc:
typedef union {
    struct {
        uint64_t                  CNT  : 64; // Rx physical lane 1. Cleared with CLR_CRC_ERR_CNTRS.
    } field;
    uint64_t val;
} LCB_ERR_INFO_CRC_ERR_LN1_t;

// LCB_ERR_INFO_CRC_ERR_LN2 desc:
typedef union {
    struct {
        uint64_t                  CNT  : 64; // Rx physical lane 2. Cleared with CLR_CRC_ERR_CNTRS.
    } field;
    uint64_t val;
} LCB_ERR_INFO_CRC_ERR_LN2_t;

// LCB_ERR_INFO_CRC_ERR_LN3 desc:
typedef union {
    struct {
        uint64_t                  CNT  : 64; // Rx physical lane 3. Cleared with CLR_CRC_ERR_CNTRS.
    } field;
    uint64_t val;
} LCB_ERR_INFO_CRC_ERR_LN3_t;

// LCB_ERR_INFO_CRC_ERR_MULTI_LN desc:
typedef union {
    struct {
        uint64_t                  CNT  : 20; // Cleared with CLR_CRC_ERR_CNTRS.
        uint64_t         Unused_63_20  : 44; // Unused
    } field;
    uint64_t val;
} LCB_ERR_INFO_CRC_ERR_MULTI_LN_t;

// LCB_ERR_INFO_TX_REPLAY_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // Cleared with CLR_CRC_ERR_CNTRS.
    } field;
    uint64_t val;
} LCB_ERR_INFO_TX_REPLAY_CNT_t;

// LCB_ERR_INFO_RX_REPLAY_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // Cleared with CLR_CRC_ERR_CNTRS.
    } field;
    uint64_t val;
} LCB_ERR_INFO_RX_REPLAY_CNT_t;

// LCB_ERR_INFO_SEQ_CRC_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  : 32; // 
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} LCB_ERR_INFO_SEQ_CRC_CNT_t;

// LCB_ERR_INFO_ESCAPE_0_ONLY_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  : 32; // Cleared with CLR_CRC_ERR_CNTRS.
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} LCB_ERR_INFO_ESCAPE_0_ONLY_CNT_t;

// LCB_ERR_INFO_ESCAPE_0_PLUS1_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  : 32; // Cleared with CLR_CRC_ERR_CNTRS.
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} LCB_ERR_INFO_ESCAPE_0_PLUS1_CNT_t;

// LCB_ERR_INFO_ESCAPE_0_PLUS2_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  : 16; // Cleared with CLR_CRC_ERR_CNTRS.
        uint64_t         Unused_63_16  : 48; // Unused
    } field;
    uint64_t val;
} LCB_ERR_INFO_ESCAPE_0_PLUS2_CNT_t;

// LCB_ERR_INFO_REINIT_FROM_PEER_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  : 32; // 
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} LCB_ERR_INFO_REINIT_FROM_PEER_CNT_t;

// LCB_ERR_INFO_SBE_CNT desc:
typedef union {
    struct {
        uint64_t         INPUT_BUFFER  :  8; // Number of Single Bit Errors that have occurred in input buffer RAM. Does not roll. Cleared with LCB_DBG_ERRINJ_ECC_CLR_ECC_ERR_CNTS.
        uint64_t       REPLAY_BUFFER0  :  8; // Number of Single Bit Errors that have occurred in replay RAM0. Does not roll. Cleared with LCB_DBG_ERRINJ_ECC_CLR_ECC_ERR_CNTS
        uint64_t       REPLAY_BUFFER1  :  8; // Number of Single Bit Errors that have occurred in replay RAM1. Does not roll. Cleared with LCB_DBG_ERRINJ_ECC_CLR_ECC_ERR_CNTS
        uint64_t   REPLAY_BUFFER_SIDE  :  8; // Number of Single Bit Errors that have occurred in replay side data RAM. Does not roll. Cleared with LCB_DBG_ERRINJ_ECC_CLR_ECC_ERR_CNTS
        uint64_t        PM_TIME_FLITS  :  8; // Number of single bit errors that have occurred in DN stream MasterTime flits.
        uint64_t         Unused_63_40  : 24; // Unused
    } field;
    uint64_t val;
} LCB_ERR_INFO_SBE_CNT_t;

// LCB_ERR_INFO_MISC_FLG_CNT desc:
typedef union {
    struct {
        uint64_t RST_FOR_FAILED_DESKEW  :  8; // see ERR_FLG register Section 30.28.50, 'LCB_ERR_STS'
        uint64_t ALL_LNS_FAILED_REINIT_TEST  :  8; // see ERR_FLG register Section 30.28.50, 'LCB_ERR_STS'
        uint64_t RST_FOR_LINK_TIMEOUT  :  8; // see ERR_FLG register Section 30.28.50, 'LCB_ERR_STS'
        uint64_t RST_FOR_INCOMPLT_RND_TRIP  :  8; // see ERR_FLG register Section 30.28.50, 'LCB_ERR_STS'
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} LCB_ERR_INFO_MISC_FLG_CNT_t;

// LCB_ERR_INFO_ECC_INPUT_BUF desc:
typedef union {
    struct {
        uint64_t                  CHK  :  8; // The content of an ERR_INFO CSR is updated when the corresponding error flag in the ERR_FLG CSR transitions from 0 to 1 due to a hardware detected event. The ERR_INFO CSR is not modified when the corresponding error flag becomes set due to a software write to the ERR_FLG CSR.
        uint64_t             SYNDROME  :  8; // The content of an ERR_INFO CSR is updated when the corresponding error flag in the ERR_FLG CSR transitions from 0 to 1 due to a hardware detected event. The ERR_INFO CSR is not modified when the corresponding error flag becomes set due to a software write to the ERR_FLG CSR.
        uint64_t             LOCATION  :  3; // 0: MBE in flit 0 of the Qflit 1: MBE in flit_1 of the Qflit 2: MBE in flit_2 3: MBE in flit_3 4: SBE in flit_0 5: SBE in flit_1 6: SBE in flit_2 7: SBE in flit_3
        uint64_t            Unused_19  :  1; // Unused
        uint64_t          CSR_CREATED  :  1; // Injected via CSR
        uint64_t         Unused_63_21  : 43; // Unused
    } field;
    uint64_t val;
} LCB_ERR_INFO_ECC_INPUT_BUF_t;

// LCB_ERR_INFO_ECC_INPUT_BUF_FLIT_DATA_UPPER_33BITS desc:
typedef union {
    struct {
        uint64_t                 DATA  : 33; // The content of an ERR_INFO CSR is updated when the corresponding error flag in the ERR_FLG CSR transitions from 0 to 1 due to a hardware detected event. The ERR_INFO CSR is not modified when the corresponding error flag becomes set due to a software write to the ERR_FLG CSR.These are the upper 33 bits of the 65-bit FLIT where the error was detected.
        uint64_t         Unused_63_33  : 31; // Unused
    } field;
    uint64_t val;
} LCB_ERR_INFO_ECC_INPUT_BUF_FLIT_DATA_UPPER_33BITS_t;

// LCB_ERR_INFO_ECC_INPUT_BUF_FLIT_DATA_LOWER_32BITS desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // The content of an ERR_INFO CSR is updated when the corresponding error flag in the ERR_FLG CSR transitions from 0 to 1 due to a hardware detected event. The ERR_INFO CSR is not modified when the corresponding error flag becomes set due to a software write to the ERR_FLG CSR. These are the lower 32 bits of the 65-bit FLIT where the error was detected.
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} LCB_ERR_INFO_ECC_INPUT_BUF_FLIT_DATA_LOWER_32BITS_t;

// LCB_ERR_INFO_ECC_REPLAY_BUF desc:
typedef union {
    struct {
        uint64_t                  CHK  :  8; // The content of an ERR_INFO CSR is updated when the corresponding error flag in the ERR_FLG CSR transitions from 0 to 1 due to a hardware detected event. The ERR_INFO CSR is not modified when the corresponding error flag becomes set due to a software write to the ERR_FLG CSR.
        uint64_t             SYNDROME  :  8; // The content of an ERR_INFO CSR is updated when the corresponding error flag in the ERR_FLG CSR transitions from 0 to 1 due to a hardware detected event. The ERR_INFO CSR is not modified when the corresponding error flag becomes set due to a software write to the ERR_FLG CSR.
        uint64_t             LOCATION  :  5; // 0: MBE ram 0 flit_0 of the read Qdlit 1: MBE ram 0 flit_1 of the read Qflit 2: MBE ram 0 flit_2 3: MBE ram 0 flit_3 4: MBE ram 1 flit_0 5: MBE ram 1 flit_1 6: MBE ram 1 flit_2 7: MBE ram 1 flit_3 8: SBE ram 0 flit_0 9: SBE ram 0 flit_1 10: SBE ram 0 flit_2 11: SBE ram 0 flit_3 12: SBE ram 1 flit_0 13: SBE ram 1 flit_1 14: SBE ram 1 flit_2 15: SBE ram 1 flit_3 16: MBE side 17: SBE side
        uint64_t         Unused_23_21  :  3; // Unused
        uint64_t          CSR_CREATED  :  1; // Injected via CSR
        uint64_t         Unused_63_25  : 39; // Unused
    } field;
    uint64_t val;
} LCB_ERR_INFO_ECC_REPLAY_BUF_t;

// LCB_ERR_INFO_ECC_REPLAY_BUF_FLIT_DATA_UPPER_33BITS desc:
typedef union {
    struct {
        uint64_t                 DATA  : 33; // The content of an ERR_INFO CSR is updated when the corresponding error flag in the ERR_FLG CSR transitions from 0 to 1 due to a hardware detected event. The ERR_INFO CSR is not modified when the corresponding error flag becomes set due to a software write to the ERR_FLG CSR. These are the upper 33 bits of the 65-bit FLIT where the error was detected.
        uint64_t         Unused_63_33  : 31; // Unused
    } field;
    uint64_t val;
} LCB_ERR_INFO_ECC_REPLAY_BUF_FLIT_DATA_UPPER_33BITS_t;

// LCB_ERR_INFO_ECC_REPLAY_BUF_FLIT_DATA_LOWER_32BITS desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // The content of an ERR_INFO CSR is updated when the corresponding error flag in the ERR_FLG CSR transitions from 0 to 1 due to a hardware detected event. The ERR_INFO CSR is not modified when the corresponding error flag becomes set due to a software write to the ERR_FLG CSR. These are the lower 32 bits of the 65-bit FLIT where the error was detected.
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} LCB_ERR_INFO_ECC_REPLAY_BUF_FLIT_DATA_LOWER_32BITS_t;

// LCB_ERR_INFO_ECC_PM_TIME desc:
typedef union {
    struct {
        uint64_t                  CHK  :  8; // The content of an ERR_INFO CSR is updated when the corresponding error flag in the ERR_FLG CSR transitions from 0 to 1 due to a hardware detected event. The ERR_INFO CSR is not modified when the corresponding error flag becomes set due to a software write to the ERR_FLG CSR.
        uint64_t             SYNDROME  :  8; // The content of an ERR_INFO CSR is updated when the corresponding error flag in the ERR_FLG CSR transitions from 0 to 1 due to a hardware detected event. The ERR_INFO CSR is not modified when the corresponding error flag becomes set due to a software write to the ERR_FLG CSR.
        uint64_t             LOCATION  :  8; // For every one of flits 0-3, there are sbe and mbe fields Bit 0: flit 0 sbeBit 1: flit 0 mbe Bit 2: flit 1 sbeBit 3: flit 1 mbe Bit 4: flit 2 sbe Bit 5: flit 2 mbe Bit 6: flit 3 sbe Bit 7: flit 3 mbe
        uint64_t          CSR_CREATED  :  1; // Injected via CSR
        uint64_t         Unused_63_25  : 39; // Unused
    } field;
    uint64_t val;
} LCB_ERR_INFO_ECC_PM_TIME_t;

// LCB_ERR_INFO_ECC_PM_TIME_FLIT_DATA_UPPER_33BITS desc:
typedef union {
    struct {
        uint64_t                 DATA  : 33; // The content of an ERR_INFO CSR is updated when the corresponding error flag in the ERR_FLG CSR transitions from 0 to 1 due to a hardware detected event. The ERR_INFO CSR is not modified when the corresponding error flag becomes set due to a software write to the ERR_FLG CSR. These are the upper 33 bits of the 65-bit FLIT where the error was detected.
        uint64_t         Unused_63_33  : 31; // Unused
    } field;
    uint64_t val;
} LCB_ERR_INFO_ECC_PM_TIME_FLIT_DATA_UPPER_33BITS_t;

// LCB_ERR_INFO_ECC_PM_TIME_FLIT_DATA_LOWER_32BITS desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // The content of an ERR_INFO CSR is updated when the corresponding error flag in the ERR_FLG CSR transitions from 0 to 1 due to a hardware detected event. The ERR_INFO CSR is not modified when the corresponding error flag becomes set due to a software write to the ERR_FLG CSR. These are the lower 32 bits of the 65-bit FLIT where the error was detected.
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} LCB_ERR_INFO_ECC_PM_TIME_FLIT_DATA_LOWER_32BITS_t;

// LCB_ERR_INFO_FEC_CERR_CNT_1 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 48; // The number of received code words with one corrected symbol error.
        uint64_t         Unused_63_48  : 16; // Unused
    } field;
    uint64_t val;
} LCB_ERR_INFO_FEC_CERR_CNT_1_t;

// LCB_ERR_INFO_FEC_CERR_CNT_2 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 48; // The number of received code words with two corrected symbol errors.
        uint64_t         Unused_63_48  : 16; // Unused
    } field;
    uint64_t val;
} LCB_ERR_INFO_FEC_CERR_CNT_2_t;

// LCB_ERR_INFO_FEC_CERR_CNT_3 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 48; // The number of received code words with three corrected symbol errors.
        uint64_t         Unused_63_48  : 16; // Unused
    } field;
    uint64_t val;
} LCB_ERR_INFO_FEC_CERR_CNT_3_t;

// LCB_ERR_INFO_FEC_CERR_CNT_4 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 48; // The number of received code words with four corrected symbol errors.
        uint64_t         Unused_63_48  : 16; // Unused
    } field;
    uint64_t val;
} LCB_ERR_INFO_FEC_CERR_CNT_4_t;

// LCB_ERR_INFO_FEC_CERR_CNT_5 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // The number of received code words with five corrected symbol errors.
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} LCB_ERR_INFO_FEC_CERR_CNT_5_t;

// LCB_ERR_INFO_FEC_CERR_CNT_6 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // The number of received code words with six corrected symbol errors.
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} LCB_ERR_INFO_FEC_CERR_CNT_6_t;

// LCB_ERR_INFO_FEC_CERR_CNT_7 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // The number of received code words with seven corrected symbol errors.
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} LCB_ERR_INFO_FEC_CERR_CNT_7_t;

// LCB_ERR_INFO_FEC_CERR_CNT_8 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // The number of received code words with eight corrected symbol errors.
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} LCB_ERR_INFO_FEC_CERR_CNT_8_t;

// LCB_ERR_INFO_FEC_UERR_CNT desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // The number of received code words with uncorrectable errors
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} LCB_ERR_INFO_FEC_UERR_CNT_t;

// LCB_ERR_INFO_FEC_CERR_MASK_0 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 64; // Bits [63:0] of the error correction vector.
    } field;
    uint64_t val;
} LCB_ERR_INFO_FEC_CERR_MASK_0_t;

// LCB_ERR_INFO_FEC_CERR_MASK_1 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 64; // Bits [127:64] of the error correction vector.
    } field;
    uint64_t val;
} LCB_ERR_INFO_FEC_CERR_MASK_1_t;

// LCB_ERR_INFO_FEC_CERR_MASK_2 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 64; // Bits [191:128] of the error correction vector.
    } field;
    uint64_t val;
} LCB_ERR_INFO_FEC_CERR_MASK_2_t;

// LCB_ERR_INFO_FEC_CERR_MASK_3 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 64; // Bits [255:192] of the error correction vector.
    } field;
    uint64_t val;
} LCB_ERR_INFO_FEC_CERR_MASK_3_t;

// LCB_ERR_INFO_FEC_CERR_MASK_4 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 64; // Bits [319:256] of the error correction vector.
    } field;
    uint64_t val;
} LCB_ERR_INFO_FEC_CERR_MASK_4_t;

// LCB_ERR_INFO_FEC_CERR_MASK_VLD desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // LCB_ERR_INFO_FEC_CERR_MASK is valid.
        uint64_t           Unused_3_1  :  3; // Unused
        uint64_t        FEC_3LN_SHIFT  :  2; // Sequence count value used for mapping decoder output lane error vectors to their shifted lane positions, when operating with three lanes. This field applies across all five Error Masks. Expected values are 0,1, and 2.
        uint64_t           Unused_7_6  :  2; // Unused
        uint64_t            STATE_NUM  :  4; // It gives error adjacency information between FEC_DEC_STATE's. Expected values are 4'd0 through 4'd9, and the field applies across all five error masks. If STATE_NUM increments by one, between error masks then the errors occurred in adjacent FEC_DEC_STATE sequences. If the STATE_NUM increments by two between error masks then the errors occurred in non-adjacent states.
        uint64_t        FEC_DEC_STATE  :  6; // FEC decoder state - captures the decoder state value that the error event occurred in. Expected values are 6'd0 through 6'd32. This field applies across all five error masks. It gives error proximity information when multiple errors occurr withing the same decoder state sequence.
        uint64_t         Unused_63_18  : 46; // Unused
    } field;
    uint64_t val;
} LCB_ERR_INFO_FEC_CERR_MASK_VLD_t;

// LCB_ERR_INFO_FEC_CERR_MASK_MISS_CNT desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Number of missed error mask captures.
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} LCB_ERR_INFO_FEC_CERR_MASK_MISS_CNT_t;

// LCB_ERR_INFO_FEC_ERR_LN0 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 48; // Number of corrected error events in lane 0
        uint64_t         Unused_63_48  : 16; // Unused
    } field;
    uint64_t val;
} LCB_ERR_INFO_FEC_ERR_LN0_t;

// LCB_ERR_INFO_FEC_ERR_LN1 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 48; // Number of corrected error events in lane 1
        uint64_t         Unused_63_48  : 16; // Unused
    } field;
    uint64_t val;
} LCB_ERR_INFO_FEC_ERR_LN1_t;

// LCB_ERR_INFO_FEC_ERR_LN2 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 48; // Number of corrected error events in lane 2
        uint64_t         Unused_63_48  : 16; // Unused
    } field;
    uint64_t val;
} LCB_ERR_INFO_FEC_ERR_LN2_t;

// LCB_ERR_INFO_FEC_ERR_LN3 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 48; // Number of corrected error events in lane 3
        uint64_t         Unused_63_48  : 16; // Unused
    } field;
    uint64_t val;
} LCB_ERR_INFO_FEC_ERR_LN3_t;

// LCB_ERR_INFO_RX_RESYNC_CNT desc:
typedef union {
    struct {
        uint64_t                 DATA  : 64; // Cleared with CLR_CRC_ERR_CNTRS
    } field;
    uint64_t val;
} LCB_ERR_INFO_RX_RESYNC_CNT_t;

// LCB_PRF_GOOD_LTP_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // Counts LTPs with a good CRC, rx_ltp_mode only. Cleared with CLR_CRC_ERR_CNTRS.
    } field;
    uint64_t val;
} LCB_PRF_GOOD_LTP_CNT_t;

// LCB_PRF_ACCEPTED_LTP_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // Counts LTPs accepted into the receive buffer. Nulls and tossed good LTPs don't add to the count. Cleared with CLR_CRC_ERR_CNTRS.
    } field;
    uint64_t val;
} LCB_PRF_ACCEPTED_LTP_CNT_t;

// LCB_PRF_TX_RELIABLE_LTP_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // Counts reliable LTPs when a replay is not in progress.
    } field;
    uint64_t val;
} LCB_PRF_TX_RELIABLE_LTP_CNT_t;

// LCB_PRF_RX_FLIT_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  : 52; // 
        uint64_t         Unused_63_52  : 12; // Unused
    } field;
    uint64_t val;
} LCB_PRF_RX_FLIT_CNT_t;

// LCB_PRF_TX_FLIT_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  : 52; // 
        uint64_t         Unused_63_52  : 12; // Unused
    } field;
    uint64_t val;
} LCB_PRF_TX_FLIT_CNT_t;

// LCB_PRF_CLK_CNTR desc:
typedef union {
    struct {
        uint64_t                  CNT  : 48; // If STROBE_ON_LCLK_CNTR is set strobe triggered after lclk cnt of 2**24 lclks, This cntr is also used in performance monitoring and is held if hold_crc_error counters is active. This counter is cleared by both the strobe (after the value is saved in this CSR) and clr_crc_error_counters as well as anytime the select is altered.
        uint64_t  NEW_VAL_WAS_STROBED  :  1; // Set with strobe of new cnt, cleared with clear of cnt.
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} LCB_PRF_CLK_CNTR_t;

// LCB_PRF_GOOD_FECCW_CNT desc:
typedef union {
    struct {
        uint64_t                  CNT  : 64; // The number of received code words without errors.
    } field;
    uint64_t val;
} LCB_PRF_GOOD_FECCW_CNT_t;

// LCB_STS_LINK_PAUSE_ACTIVE desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // It is safe to start the OPIO re-center when this is set.
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} LCB_STS_LINK_PAUSE_ACTIVE_t;

// LCB_STS_LINK_TRANSFER_ACTIVE desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // 
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} LCB_STS_LINK_TRANSFER_ACTIVE_t;

// LCB_STS_LN_DEGRADE desc:
typedef union {
    struct {
        uint64_t            EXHAUSTED  :  1; // CNT >= CFG_LN_DEGRADE_NUM_ALLOWED. This field is not functional when degrades are disabled.
        uint64_t                  CNT  :  2; // The number of lane degrade events. This still operates with degrades disabled so it can be used for diagnostic purposes.
        uint64_t             Unused_3  :  1; // Unused
        uint64_t     LN_012_RACES_WON  :  3; // These are the shifted lane numbers, not the physical lane numbers. This is the number of races won by the proposed team of shifted lanes 0/1/2 with lane 3 disabled.This cnt will not have meaning if less than 4 lanes are active. This still operates with degrades disabled so it can be used for diagnostic purposes.
        uint64_t     LN_013_RACES_WON  :  3; // These are the shifted lane numbers, not the physical lane numbers. This is the number of races won by the proposed team of shifted lanes 0/1/3 with lane 2 disabled. If only three lanes are active this is the number of races won by the proposed cfg of shifted lane 0 and 1. This cnt will not have meaning if only 2 lanes are active. This still operates with degrades disabled so it can be used for diagnostic purposes.
        uint64_t     LN_023_RACES_WON  :  3; // These are the shifted lane numbers, not the physical lane numbers. This is the number of races won by the proposed team of shifted lanes 0/2/3 with lane 1 disabled. If only three lanes are active this is the number of races won by the proposed cfg of shifted lane 0 and 2. If only two lanes are active this is the number of races won by the proposed cfg of shifted lane 0. This still operates with degrades disabled so it can be used for diagnostic purposes.
        uint64_t     LN_123_RACES_WON  :  3; // These are the shifted lane numbers, not the physical lane numbers. This is the number of races won by the proposed team of shifted lanes 1/2/3 with lane 0 disabled. If only three lanes are active this is the number of races won by the proposed cfg of shifted lane 1and 2. If only two lanes are active this is the number of races won by the proposed cfg of shifted lane 1. This still operates with degrades disabled so it can be used for diagnostic purposes.
        uint64_t CURRENT_CFG_RACES_WON  : 48; // number of races won by the current lane configuration. Sticks at 48'hffffffffffff. Cleared by a degrade reinit or CLR_CRC_ERR_CNTRS. This still operates with degrades disabled so it can be used for diagnostic purposes.
    } field;
    uint64_t val;
} LCB_STS_LN_DEGRADE_t;

// LCB_STS_TX_LTP_MODE desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // When set LTPs are being transmitted, when clear a reinit sequence is underway.
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} LCB_STS_TX_LTP_MODE_t;

// LCB_STS_RX_LTP_MODE desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // When set LTPs are being received, when clear a reinit sequence is underway.
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} LCB_STS_RX_LTP_MODE_t;

// LCB_STS_RX_CRC_FEC_MODE desc:
typedef union {
    struct {
        uint64_t             CRC_MODE  :  2; // 
        uint64_t           Unused_3_2  :  2; // Unused
        uint64_t             FEC_MODE  :  2; // RS(132,128) = 2'b01, RS(528,512) = 2'b10, FEC disabled = 2'b00
        uint64_t          Unused_63_6  : 58; // Unused
    } field;
    uint64_t val;
} LCB_STS_RX_CRC_FEC_MODE_t;

// LCB_STS_RX_LOGICAL_ID desc:
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
} LCB_STS_RX_LOGICAL_ID_t;

// LCB_STS_RX_SHIFTED_LN_NUM desc:
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
} LCB_STS_RX_SHIFTED_LN_NUM_t;

// LCB_STS_RX_PHY_LN_EN desc:
typedef union {
    struct {
        uint64_t             COMBINED  :  4; // Combined lane enable = CFG_LN_EN & INIT & DEGRADE & FEC_DEGRADE
        uint64_t                 INIT  :  4; // Initialization lane enable. (some lanes fail the training sequence)
        uint64_t              DEGRADE  :  4; // Degrade lane enable. (lanes are removed for high BER etc.) After a degrade occurs this field takes on the value of the new combined field. By observing the INIT field one can deduce why each lane was removed. If INIT is clear it was removed during initialization, otherwise it was removed by a degrade during LTP mode.
        uint64_t          FEC_DEGRADE  :  4; // FEC degrade lane enable. Records lane removal via the FEC lane degrade process.
        uint64_t         Unused_63_16  : 48; // Unused
    } field;
    uint64_t val;
} LCB_STS_RX_PHY_LN_EN_t;

// LCB_STS_TX_PHY_LN_EN desc:
typedef union {
    struct {
        uint64_t                  VAL  :  4; // {lane3,lane2,lane1,lane0}
        uint64_t               TX_TON  :  1; // TX Turn on Signal
        uint64_t          Unused_63_5  : 59; // Unused
    } field;
    uint64_t val;
} LCB_STS_TX_PHY_LN_EN_t;

// LCB_STS_ROUND_TRIP_LTP_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  : 16; // This count is in units of LTP's. Informational: LSB = 5.28 ns @ 200Gbps, no FEC, CRC16 LSB = 5.45 ns @ 200Gbps, either FEC, CRC16 LSB = 5.44 ns @ 200Gbps, no FEC, CRC48 LSB = 5.61 ns @ 200Gbps, either FEC, CRC48
        uint64_t         Unused_63_16  : 48; // Unused
    } field;
    uint64_t val;
} LCB_STS_ROUND_TRIP_LTP_CNT_t;

// LCB_STS_NULLS_REQUIRED desc:
typedef union {
    struct {
        uint64_t                  CNT  : 16; // 
        uint64_t         Unused_63_16  : 48; // Unused
    } field;
    uint64_t val;
} LCB_STS_NULLS_REQUIRED_t;

// LCB_STS_FLIT_QUIET_CNTR desc:
typedef union {
    struct {
        uint64_t                  VAL  :  3; // Rx side count. 3'd7 means quiet.
        uint64_t          Unused_63_3  : 61; // Unused
    } field;
    uint64_t val;
} LCB_STS_FLIT_QUIET_CNTR_t;

// LCB_STS_RCV_SMA_MSG desc:
typedef union {
    struct {
        uint64_t                  VAL  : 48; // 
        uint64_t         Unused_63_48  : 16; // Unused
    } field;
    uint64_t val;
} LCB_STS_RCV_SMA_MSG_t;

// LCB_STS_MASTER_TIME_LOW_0 desc:
typedef union {
    struct {
        uint64_t                  VAL  : 40; // When a MasterTimeLow flit, with an ID of 0, arrives at a port that supports a Local Clock the Local Clock value is captured in a LocalTime register, and the Master Clock ID and low 40 bits of the Master Time are captured in a MasterTime register. For a debug HFI FZC in port mirroring mode capture the low 40 bits of the LUT time stamp value in this register when a PortMirrorTime idle arrives with the LUT time stamp embedded.
        uint64_t            MASTER_ID  :  2; // When a MasterTimeLow flit, with an ID of 0, arrives at a port that supports a Local Clock the Local Clock value is captured in a LocalTime register, and the Master Clock ID and low 40 bits of the Master Time are captured in a MasterTime register.
        uint64_t         Unused_63_42  : 22; // Unused
    } field;
    uint64_t val;
} LCB_STS_MASTER_TIME_LOW_0_t;

// LCB_STS_MASTER_TIME_HIGH_0 desc:
typedef union {
    struct {
        uint64_t                  VAL  : 32; // When a MasterTimeHigh flit, with an ID of 0, arrives at a port that supports a Local Clock the high 32 bits of the Master Time are captured in a MasterTimeHigh register. For a debug HFI FZC in port mirroring mode capture the high 24 bits of the 56 bit LUT time stamp value in this register when a PortMirrorTime idle arrives with the LUT time stamp embedded.
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} LCB_STS_MASTER_TIME_HIGH_0_t;

// LCB_STS_SNAPPED_LOCAL_TIME_0 desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // When a MasterTimeLow flit, with an ID of 0, arrives at a port that supports a Local Clock the Local Clock value is captured in a LocalTime register. The LocalTime register is not changed when the MasterTimeHigh flit arrives. For a debug HFI FZC in port mirroring mode capture the Local Clock value in this register when a PortMirrorTime arrives with the LUT time stamp embedded. LSB = 1.05ns when APR core clock is 950 MHz
    } field;
    uint64_t val;
} LCB_STS_SNAPPED_LOCAL_TIME_0_t;

// LCB_STS_MASTER_TIME_LOW_1 desc:
typedef union {
    struct {
        uint64_t                  VAL  : 40; // When a MasterTimeLow flit, with an ID of 1, arrives at a port that supports a Local Clock the Local Clock value is captured in a LocalTime register, and the Master Clock ID and low 40 bits of the Master Time are captured in a MasterTime register. For a debug HFI FZC in port mirroring mode capture the low 40 bits of the LUT time stamp value in this register when a PortMirrorTime idle arrives with the LUT time stamp embedded.
        uint64_t            MASTER_ID  :  2; // When a MasterTimeLow flit, with an ID of 1, arrives at a port that supports a Local Clock the Local Clock value is captured in a LocalTime register, and the Master Clock ID and low 40 bits of the Master Time are captured in a MasterTime register.
        uint64_t         Unused_63_42  : 22; // Unused
    } field;
    uint64_t val;
} LCB_STS_MASTER_TIME_LOW_1_t;

// LCB_STS_MASTER_TIME_HIGH_1 desc:
typedef union {
    struct {
        uint64_t                  VAL  : 32; // When a MasterTimeHigh flit, with an ID of 1, arrives at a port that supports a Local Clock the high 32 bits of the Master Time are captured in a MasterTimeHigh register. For a debug HFI FZC in port mirroring mode capture the high 24 bits of the 56 bit LUT time stamp value in this register when a PortMirrorTime idle arrives with the LUT time stamp embedded.
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} LCB_STS_MASTER_TIME_HIGH_1_t;

// LCB_STS_SNAPPED_LOCAL_TIME_1 desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // When a MasterTimeLow flit, with an ID of 1, arrives at a port that supports a Local Clock the Local Clock value is captured in a LocalTime register. The LocalTime register is not changed when the MasterTimeHigh flit arrives. For a debug HFI FZC in port mirroring mode capture the Local Clock value in this register when a PortMirrorTime arrives with the LUT time stamp embedded. LSB = 1.05ns when APR core clock is 950 MHz
    } field;
    uint64_t val;
} LCB_STS_SNAPPED_LOCAL_TIME_1_t;

// LCB_STS_MASTER_TIME_LOW_2 desc:
typedef union {
    struct {
        uint64_t                  VAL  : 40; // When a MasterTimeLow flit, with an ID of 2, arrives at a port that supports a Local Clock the Local Clock value is captured in a LocalTime register, and the Master Clock ID and low 40 bits of the Master Time are captured in a MasterTime register. For a debug HFI FZC in port mirroring mode capture the low 40 bits of the LUT time stamp value in this register when a PortMirrorTime idle arrives with the LUT time stamp embedded.
        uint64_t            MASTER_ID  :  2; // When a MasterTimeLow flit, with an ID of 2, arrives at a port that supports a Local Clock the Local Clock value is captured in a LocalTime register, and the Master Clock ID and low 40 bits of the Master Time are captured in a MasterTime register.
        uint64_t         Unused_63_42  : 22; // Unused
    } field;
    uint64_t val;
} LCB_STS_MASTER_TIME_LOW_2_t;

// LCB_STS_MASTER_TIME_HIGH_2 desc:
typedef union {
    struct {
        uint64_t                  VAL  : 32; // When a MasterTimeHigh flit, with an ID of 2, arrives at a port that supports a Local Clock the high 32 bits of the Master Time are captured in a MasterTimeHigh register. For a debug HFI FZC in port mirroring mode capture the high 24 bits of the 56 bit LUT time stamp value in this register when a PortMirrorTime idle arrives with the LUT time stamp embedded.
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} LCB_STS_MASTER_TIME_HIGH_2_t;

// LCB_STS_SNAPPED_LOCAL_TIME_2 desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // When a MasterTimeLow flit, with an ID of 2, arrives at a port that supports a Local Clock the Local Clock value is captured in a LocalTime register. The LocalTime register is not changed when the MasterTimeHigh flit arrives. For a debug HFI FZC in port mirroring mode capture the Local Clock value in this register when a PortMirrorTime arrives with the LUT time stamp embedded. LSB = 1.05ns when APR core clock is 950 MHz
    } field;
    uint64_t val;
} LCB_STS_SNAPPED_LOCAL_TIME_2_t;

// LCB_STS_MASTER_TIME_LOW_3 desc:
typedef union {
    struct {
        uint64_t                  VAL  : 40; // When a MasterTimeLow flit, with an ID of 3, arrives at a port that supports a Local Clock the Local Clock value is captured in a LocalTime register, and the Master Clock ID and low 40 bits of the Master Time are captured in a MasterTime register. For a debug HFI FZC in port mirroring mode capture the low 40 bits of the LUT time stamp value in this register when a PortMirrorTime idle arrives with the LUT time stamp embedded.
        uint64_t            MASTER_ID  :  2; // When a MasterTimeLow flit, with an ID of 3, arrives at a port that supports a Local Clock the Local Clock value is captured in a LocalTime register, and the Master Clock ID and low 40 bits of the Master Time are captured in a MasterTime register.
        uint64_t         Unused_63_42  : 22; // Unused
    } field;
    uint64_t val;
} LCB_STS_MASTER_TIME_LOW_3_t;

// LCB_STS_MASTER_TIME_HIGH_3 desc:
typedef union {
    struct {
        uint64_t                  VAL  : 32; // When a MasterTimeHigh flit, with an ID of 3, arrives at a port that supports a Local Clock the high 32 bits of the Master Time are captured in a MasterTimeHigh register. For a debug HFI FZC in port mirroring mode capture the high 24 bits of the 56 bit LUT time stamp value in this register when a PortMirrorTime idle arrives with the LUT time stamp embedded.
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} LCB_STS_MASTER_TIME_HIGH_3_t;

// LCB_STS_SNAPPED_LOCAL_TIME_3 desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // When a MasterTimeLow flit, with an ID of 3, arrives at a port that supports a Local Clock the Local Clock value is captured in a LocalTime register. The LocalTime register is not changed when the MasterTimeHigh flit arrives. For a debug HFI FZC in port mirroring mode capture the Local Clock value in this register when a PortMirrorTime arrives with the LUT time stamp embedded. LSB = 1.05ns when APR core clock is 950 MHz
    } field;
    uint64_t val;
} LCB_STS_SNAPPED_LOCAL_TIME_3_t;

// LCB_STS_UPPER_LOCAL_TIME desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // Local time - NS bits
    } field;
    uint64_t val;
} LCB_STS_UPPER_LOCAL_TIME_t;

// LCB_STS_LOWER_LOCAL_TIME desc:
typedef union {
    struct {
        uint64_t                  VAL  : 32; // Local time - lower bits LSB = 1.05ns when APR core clock is 950 MHz
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} LCB_STS_LOWER_LOCAL_TIME_t;

// LCB_PG_CFG_RUN desc:
typedef union {
    struct {
        uint64_t                   EN  :  1; // Set high to give PG control of the inputs.
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} LCB_PG_CFG_RUN_t;

// LCB_PG_CFG_RX_RUN desc:
typedef union {
    struct {
        uint64_t                   EN  :  1; // Receive side run enable.
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} LCB_PG_CFG_RX_RUN_t;

// LCB_PG_CFG_TX_RUN desc:
typedef union {
    struct {
        uint64_t                   EN  :  1; // Transmit side run enable.
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} LCB_PG_CFG_TX_RUN_t;

// LCB_PG_CFG_CAPTURE_DELAY desc:
typedef union {
    struct {
        uint64_t                  VAL  :  4; // Delay time in cclks between the trigger event and the hold of the capture buffers.
        uint64_t          Unused_63_4  : 60; // Unused
    } field;
    uint64_t val;
} LCB_PG_CFG_CAPTURE_DELAY_t;

// LCB_PG_CFG_CAPTURE_RADR desc:
typedef union {
    struct {
        uint64_t                  VAL  :  4; // 
        uint64_t                   EN  :  1; // enable the RAM read port, currently not needed/used for the capture RAMs, leave in case the RAMs change
        uint64_t          Unused_63_5  : 59; // Unused
    } field;
    uint64_t val;
} LCB_PG_CFG_CAPTURE_RADR_t;

// LCB_PG_CFG_CRDTS_LCB desc:
typedef union {
    struct {
        uint64_t                  VAL  :  8; // Maximum outstanding LCB input buffer flits.
        uint64_t          Unused_63_8  : 56; // Unused
    } field;
    uint64_t val;
} LCB_PG_CFG_CRDTS_LCB_t;

// LCB_PG_CFG_LEN desc:
typedef union {
    struct {
        uint64_t                  VAL  : 52; // Length in number of flits. A value of 0 is treated as infinite so the test never ends.
        uint64_t            PAUSE_MSK  :  6; // Flit transmission will pause every 2^pause_mask flits. Default is 1 pause per minute at 1Gflit/sec. 0x3f will prevent pauses.
        uint64_t         Unused_63_58  :  6; // Unused
    } field;
    uint64_t val;
} LCB_PG_CFG_LEN_t;

// LCB_PG_CFG_SEED desc:
typedef union {
    struct {
        uint64_t                  VAL  : 55; // Random seed
        uint64_t         Unused_63_55  :  9; // Unused
    } field;
    uint64_t val;
} LCB_PG_CFG_SEED_t;

// LCB_PG_CFG_FLIT desc:
typedef union {
    struct {
        uint64_t              CONTENT  :  2; // Flit data: 0=random, 1=non-idle flit count, 2=canned pattern, 3=alternate between canned pattern and inverted canned pattern.
        uint64_t           Unused_3_2  :  2; // Unused
        uint64_t ALLOW_UNEXP_TOSSED_FLITS  :  1; // 
        uint64_t          Unused_63_5  : 59; // Unused
    } field;
    uint64_t val;
} LCB_PG_CFG_FLIT_t;

// LCB_PG_CFG_IDLE_INJECT desc:
typedef union {
    struct {
        uint64_t            THRESHOLD  : 16; // Random 15-bit numbers less than or equal to this value select idle flits. Setting equal to 0 disables idle flits for maximum throughput. Random 15-bit numbers greater than or equal to this value select non-idle flits. Setting greater than or equal to 32768 disables non-idle flits.
        uint64_t         Unused_63_16  : 48; // Unused
    } field;
    uint64_t val;
} LCB_PG_CFG_IDLE_INJECT_t;

// LCB_PG_CFG_ERR desc:
typedef union {
    struct {
        uint64_t        SBE_THRESHOLD  : 32; // Random 31-bit numbers less than this value select flits for errors, either single or double bit errors. Setting to 0 disables all errors.
        uint64_t        MBE_THRESHOLD  : 16; // Random 15-bit numbers less than this value select error flits for double bit errors. Otherwise, error flits are single bit errors. This feature is not working properly. Its sole purpose would be to test input buffer MBE behavior, which can be done via ECC control, making this feature non-critical.
        uint64_t            REALISTIC  :  1; // 0=never corrupt flit type encoding bits 48-46, 1=allow corruption of any flit bits
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} LCB_PG_CFG_ERR_t;

// LCB_PG_CFG_MISCOMPARE desc:
typedef union {
    struct {
        uint64_t                   EN  :  5; // {VL_ACK, FLIT3, FLIT2, FLIT1, FLIT0}
        uint64_t          Unused_63_5  : 59; // Unused
    } field;
    uint64_t val;
} LCB_PG_CFG_MISCOMPARE_t;

// LCB_PG_CFG_PATTERN desc:
typedef union {
    struct {
        uint64_t                 DATA  : 56; // Used when LCB_PG_CFG_FLIT_CONTENT set at 2.
        uint64_t         Unused_63_56  :  8; // Unused
    } field;
    uint64_t val;
} LCB_PG_CFG_PATTERN_t;

// LCB_PG_DBG_FLIT_CRDTS_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  :  8; // 
        uint64_t          Unused_63_8  : 56; // Unused
    } field;
    uint64_t val;
} LCB_PG_DBG_FLIT_CRDTS_CNT_t;

// LCB_PG_ERR_INFO_MISCOMPARE_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  :  8; // Miscompare count
        uint64_t          Unused_63_8  : 56; // Unused
    } field;
    uint64_t val;
} LCB_PG_ERR_INFO_MISCOMPARE_CNT_t;

// LCB_PG_STS_CAPTURE_ACTUAL_DAT0 desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // flit0
    } field;
    uint64_t val;
} LCB_PG_STS_CAPTURE_ACTUAL_DAT0_t;

// LCB_PG_STS_CAPTURE_ACTUAL_DAT1 desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // flit1
    } field;
    uint64_t val;
} LCB_PG_STS_CAPTURE_ACTUAL_DAT1_t;

// LCB_PG_STS_CAPTURE_ACTUAL_DAT2 desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // flit2
    } field;
    uint64_t val;
} LCB_PG_STS_CAPTURE_ACTUAL_DAT2_t;

// LCB_PG_STS_CAPTURE_ACTUAL_DAT3 desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // flit3
    } field;
    uint64_t val;
} LCB_PG_STS_CAPTURE_ACTUAL_DAT3_t;

// LCB_PG_STS_CAPTURE_ACTUAL_MISC desc:
typedef union {
    struct {
        uint64_t                B64_0  :  1; // flit 0 bit 64
        uint64_t                B64_1  :  1; // flit 1 bit 64
        uint64_t                B64_2  :  1; // flit 2 bit 64
        uint64_t                B64_3  :  1; // flit 3 bit 64
        uint64_t               VL_ACK  : 10; // [9]: valid, [8]: odd parity, [7:0]: ack data
        uint64_t         Unused_15_14  :  2; // Unused
        uint64_t           MISCOMPARE  :  5; // {ack miscompare, flit 3 miscompare, flit 2 miscompare, flit 1 miscompare, flit 0 miscompare}
        uint64_t         Unused_23_21  :  3; // Unused
        uint64_t                 WADR  :  4; // next address to be written, start read of RAM at this address.
        uint64_t         Unused_63_28  : 36; // Unused
    } field;
    uint64_t val;
} LCB_PG_STS_CAPTURE_ACTUAL_MISC_t;

// LCB_PG_STS_CAPTURE_EXPECT_DAT0 desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // flit0
    } field;
    uint64_t val;
} LCB_PG_STS_CAPTURE_EXPECT_DAT0_t;

// LCB_PG_STS_CAPTURE_EXPECT_DAT1 desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // flit1
    } field;
    uint64_t val;
} LCB_PG_STS_CAPTURE_EXPECT_DAT1_t;

// LCB_PG_STS_CAPTURE_EXPECT_DAT2 desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // flit2
    } field;
    uint64_t val;
} LCB_PG_STS_CAPTURE_EXPECT_DAT2_t;

// LCB_PG_STS_CAPTURE_EXPECT_DAT3 desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // flit3
    } field;
    uint64_t val;
} LCB_PG_STS_CAPTURE_EXPECT_DAT3_t;

// LCB_PG_STS_CAPTURE_EXPECT_MISC desc:
typedef union {
    struct {
        uint64_t                B64_0  :  1; // flit 0 bit 64
        uint64_t                B64_1  :  1; // flit 1 bit 64
        uint64_t                B64_2  :  1; // flit 2 bit 64
        uint64_t                B64_3  :  1; // flit 3 bit 64
        uint64_t               VL_ACK  :  8; // ack expect data
        uint64_t         Unused_63_12  : 52; // Unused
    } field;
    uint64_t val;
} LCB_PG_STS_CAPTURE_EXPECT_MISC_t;

// LCB_PG_STS_FLG desc:
typedef union {
    struct {
        uint64_t           Unused_3_0  :  4; // Unused
        uint64_t       TX_RX_COMPLETE  :  1; // Both Tx and Rx flit counts greater than test length
        uint64_t           Unused_7_5  :  3; // Unused
        uint64_t       TX_END_OF_TEST  :  1; // Length of test reached.
        uint64_t          Unused_11_9  :  3; // Unused
        uint64_t    OUT_OF_FLIT_CRDTS  :  1; // Lack of flit credits
        uint64_t         Unused_15_13  :  3; // Unused
        uint64_t           MISCOMPARE  :  5; // [0] flit 0 bus, [1]: flit 1 bus, [2]: flit 2 bus, [3]: flit 3 bus, [4]: sideband VL ack
        uint64_t         Unused_23_21  :  3; // Unused
        uint64_t        LOST_FLIT_ACK  :  1; // Flit acks pending at tx pause or end of test
        uint64_t         Unused_27_25  :  3; // Unused
        uint64_t       EXTRA_FLIT_ACK  :  1; // Unexpected flit ack detected.
        uint64_t         Unused_63_29  : 35; // Unused
    } field;
    uint64_t val;
} LCB_PG_STS_FLG_t;

// LCB_PG_STS_PAUSE_COMPLETE_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  : 16; // Pause Complete count
        uint64_t         Unused_63_16  : 48; // Unused
    } field;
    uint64_t val;
} LCB_PG_STS_PAUSE_COMPLETE_CNT_t;

// LCB_PG_STS_TX_SBE_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  : 16; // Single bit error count
        uint64_t         Unused_63_16  : 48; // Unused
    } field;
    uint64_t val;
} LCB_PG_STS_TX_SBE_CNT_t;

// LCB_PG_STS_TX_MBE_CNT desc:
typedef union {
    struct {
        uint64_t                  VAL  : 16; // multiple bit error count
        uint64_t         Unused_63_16  : 48; // Unused
    } field;
    uint64_t val;
} LCB_PG_STS_TX_MBE_CNT_t;

#endif /* ___FZC_lcb_CSRS_H__ */