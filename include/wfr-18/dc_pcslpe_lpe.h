/*
* Table #2 of 240_CSR_pcslpe.xml - lpe_reg_0
* Register containing various mode and behavior signals. Flow Control Timer can 
* be disabled or accelerated.
*/
#define DC_LPE_REG_0                                            (DC_PCSLPE_LPE + 0x000000000000)
#define DC_LPE_REG_0_RESETCSR                                   0x11110000ull
#define DC_LPE_REG_0_VL_76_FC_PERIOD_SHIFT                      28
#define DC_LPE_REG_0_VL_76_FC_PERIOD_MASK                       0xFull
#define DC_LPE_REG_0_VL_76_FC_PERIOD_SMASK                      0xF0000000ull
#define DC_LPE_REG_0_VL_54_FC_PERIOD_SHIFT                      24
#define DC_LPE_REG_0_VL_54_FC_PERIOD_MASK                       0xFull
#define DC_LPE_REG_0_VL_54_FC_PERIOD_SMASK                      0xF000000ull
#define DC_LPE_REG_0_VL_32_FC_PERIOD_SHIFT                      20
#define DC_LPE_REG_0_VL_32_FC_PERIOD_MASK                       0xFull
#define DC_LPE_REG_0_VL_32_FC_PERIOD_SMASK                      0xF00000ull
#define DC_LPE_REG_0_VL_10_FC_PERIOD_SHIFT                      16
#define DC_LPE_REG_0_VL_10_FC_PERIOD_MASK                       0xFull
#define DC_LPE_REG_0_VL_10_FC_PERIOD_SMASK                      0xF0000ull
#define DC_LPE_REG_0_SEND_ALL_FLOW_CONTROL_SHIFT                15
#define DC_LPE_REG_0_SEND_ALL_FLOW_CONTROL_MASK                 0x1ull
#define DC_LPE_REG_0_SEND_ALL_FLOW_CONTROL_SMASK                0x8000ull
#define DC_LPE_REG_0_EXTERNAL_CCFT_SHIFT                        14
#define DC_LPE_REG_0_EXTERNAL_CCFT_MASK                         0x1ull
#define DC_LPE_REG_0_EXTERNAL_CCFT_SMASK                        0x4000ull
#define DC_LPE_REG_0_EXTERNAL_TS3_SHIFT                         13
#define DC_LPE_REG_0_EXTERNAL_TS3_MASK                          0x1ull
#define DC_LPE_REG_0_EXTERNAL_TS3_SMASK                         0x2000ull
#define DC_LPE_REG_0_HB_TIMER_ACCEL_SHIFT                       12
#define DC_LPE_REG_0_HB_TIMER_ACCEL_MASK                        0x1ull
#define DC_LPE_REG_0_HB_TIMER_ACCEL_SMASK                       0x1000ull
#define DC_LPE_REG_0_FC_TIMER_INHIBIT_SHIFT                     11
#define DC_LPE_REG_0_FC_TIMER_INHIBIT_MASK                      0x1ull
#define DC_LPE_REG_0_FC_TIMER_INHIBIT_SMASK                     0x800ull
#define DC_LPE_REG_0_FC_TIMER_ACCEL_SHIFT                       10
#define DC_LPE_REG_0_FC_TIMER_ACCEL_MASK                        0x1ull
#define DC_LPE_REG_0_FC_TIMER_ACCEL_SMASK                       0x400ull
#define DC_LPE_REG_0_LRH_LVER_EN_SHIFT                          9
#define DC_LPE_REG_0_LRH_LVER_EN_MASK                           0x1ull
#define DC_LPE_REG_0_LRH_LVER_EN_SMASK                          0x200ull
#define DC_LPE_REG_0_SKIP_ACCELERATE_SHIFT                      8
#define DC_LPE_REG_0_SKIP_ACCELERATE_MASK                       0x1ull
#define DC_LPE_REG_0_SKIP_ACCELERATE_SMASK                      0x100ull
#define DC_LPE_REG_0_TX_TEST_MODE_EN_SHIFT                      7
#define DC_LPE_REG_0_TX_TEST_MODE_EN_MASK                       0x1ull
#define DC_LPE_REG_0_TX_TEST_MODE_EN_SMASK                      0x80ull
#define DC_LPE_REG_0_LPE_TX_ECC_MARK_DIS_SHIFT                  6
#define DC_LPE_REG_0_LPE_TX_ECC_MARK_DIS_MASK                   0x1ull
#define DC_LPE_REG_0_LPE_TX_ECC_MARK_DIS_SMASK                  0x40ull
#define DC_LPE_REG_0_FORCE_RX_ACT_TRIG_SHIFT                    5
#define DC_LPE_REG_0_FORCE_RX_ACT_TRIG_MASK                     0x1ull
#define DC_LPE_REG_0_FORCE_RX_ACT_TRIG_SMASK                    0x20ull
#define DC_LPE_REG_0_AUTO_ACT_ENABLE_SHIFT                      4
#define DC_LPE_REG_0_AUTO_ACT_ENABLE_MASK                       0x1ull
#define DC_LPE_REG_0_AUTO_ACT_ENABLE_SMASK                      0x10ull
#define DC_LPE_REG_0_AUTO_ARM_ENABLE_SHIFT                      3
#define DC_LPE_REG_0_AUTO_ARM_ENABLE_MASK                       0x1ull
#define DC_LPE_REG_0_AUTO_ARM_ENABLE_SMASK                      0x8ull
#define DC_LPE_REG_0_LPE_LOOPBACK_SHIFT                         2
#define DC_LPE_REG_0_LPE_LOOPBACK_MASK                          0x1ull
#define DC_LPE_REG_0_LPE_LOOPBACK_SMASK                         0x4ull
#define DC_LPE_REG_0_LPE_LINK_ECC_GEN_EN_SHIFT                  1
#define DC_LPE_REG_0_LPE_LINK_ECC_GEN_EN_MASK                   0x1ull
#define DC_LPE_REG_0_LPE_LINK_ECC_GEN_EN_SMASK                  0x2ull
#define DC_LPE_REG_0_ACT_DEFER_PKT_FLOW_EN_SHIFT                0
#define DC_LPE_REG_0_ACT_DEFER_PKT_FLOW_EN_MASK                 0x1ull
#define DC_LPE_REG_0_ACT_DEFER_PKT_FLOW_EN_SMASK                0x1ull
/*
* Table #3 of 240_CSR_pcslpe.xml - lpe_reg_1
* See description below for individual field definition
*/
#define DC_LPE_REG_1                                            (DC_PCSLPE_LPE + 0x000000000008)
#define DC_LPE_REG_1_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_1_UNUSED_31_5_SHIFT                          5
#define DC_LPE_REG_1_UNUSED_31_5_MASK                           0x7FFFFFFull
#define DC_LPE_REG_1_UNUSED_31_5_SMASK                          0xFFFFFFE0ull
#define DC_LPE_REG_1_LOCAL_BUS_PE_SHIFT                         4
#define DC_LPE_REG_1_LOCAL_BUS_PE_MASK                          0x1ull
#define DC_LPE_REG_1_LOCAL_BUS_PE_SMASK                         0x10ull
#define DC_LPE_REG_1_REG_RD_PE_SHIFT                            3
#define DC_LPE_REG_1_REG_RD_PE_MASK                             0x1ull
#define DC_LPE_REG_1_REG_RD_PE_SMASK                            0x8ull
#define DC_LPE_REG_1_CP_ACTIVE_TRIGGER_SHIFT                    2
#define DC_LPE_REG_1_CP_ACTIVE_TRIGGER_MASK                     0x1ull
#define DC_LPE_REG_1_CP_ACTIVE_TRIGGER_SMASK                    0x4ull
#define DC_LPE_REG_1_RX_ACTIVE_TRIGGER_SHIFT                    1
#define DC_LPE_REG_1_RX_ACTIVE_TRIGGER_MASK                     0x1ull
#define DC_LPE_REG_1_RX_ACTIVE_TRIGGER_SMASK                    0x2ull
#define DC_LPE_REG_1_ACTIVE_ENABLE_SHIFT                        0
#define DC_LPE_REG_1_ACTIVE_ENABLE_MASK                         0x1ull
#define DC_LPE_REG_1_ACTIVE_ENABLE_SMASK                        0x1ull
/*
* Table #4 of 240_CSR_pcslpe.xml - lpe_reg_2
* See description below for individual field definition
*/
#define DC_LPE_REG_2                                            (DC_PCSLPE_LPE + 0x000000000010)
#define DC_LPE_REG_2_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_2_UNUSED_31_1_SHIFT                          1
#define DC_LPE_REG_2_UNUSED_31_1_MASK                           0x7FFFFFFFull
#define DC_LPE_REG_2_UNUSED_31_1_SMASK                          0xFFFFFFFEull
#define DC_LPE_REG_2_LPE_EB_RESET_SHIFT                         0
#define DC_LPE_REG_2_LPE_EB_RESET_MASK                          0x1ull
#define DC_LPE_REG_2_LPE_EB_RESET_SMASK                         0x1ull
/*
* Table #5 of 240_CSR_pcslpe.xml - lpe_reg_3
* See description below for individual field definition
*/
#define DC_LPE_REG_3                                            (DC_PCSLPE_LPE + 0x000000000018)
#define DC_LPE_REG_3_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_3_UNUSED_31_3_SHIFT                          3
#define DC_LPE_REG_3_UNUSED_31_3_MASK                           0x1FFFFFFFull
#define DC_LPE_REG_3_UNUSED_31_3_SMASK                          0xFFFFFFF8ull
#define DC_LPE_REG_3_EN_TX_ICRC_GEN_SHIFT                       2
#define DC_LPE_REG_3_EN_TX_ICRC_GEN_MASK                        0x1ull
#define DC_LPE_REG_3_EN_TX_ICRC_GEN_SMASK                       0x4ull
#define DC_LPE_REG_3_DIS_TX_ICRC_CHK_SHIFT                      1
#define DC_LPE_REG_3_DIS_TX_ICRC_CHK_MASK                       0x1ull
#define DC_LPE_REG_3_DIS_TX_ICRC_CHK_SMASK                      0x2ull
#define DC_LPE_REG_3_DIS_RX_ICRC_CHK_SHIFT                      0
#define DC_LPE_REG_3_DIS_RX_ICRC_CHK_MASK                       0x1ull
#define DC_LPE_REG_3_DIS_RX_ICRC_CHK_SMASK                      0x1ull
/*
* Table #6 of 240_CSR_pcslpe.xml - lpe_reg_5
* This register controls the 8b10b Scramble feature that uses the TS3 exchange 
* to negotiate enabling Scrambling of the 8b data prior to conversion to 10b 
* during transmission and after the 10b to 8b conversion in the receive 
* direction. This mode enable must be coordinated from both sides of the link to 
* assure that there is data coherency after the mode is entered and exited. The 
* scrambling is self-synchronizing in that the endpoints automatically adjust to 
* the flow without strict bit-wise endpoint coordination. If the TS3 byte 9[2] 
* is set on the received TS3, the remote node has the capability to engage in 
* Scrambling. The Force_Scramble mode bit overrides the received TS3 byte 9[2]. 
* We must set our transmitted TS3 byte 9[2] (Scramble Capability) and set our 
* local scramble capability bit. Then when we enter link state Config.Idle and 
* both received and transmitted TS3 bit 9 is set, we enable Scrambling and this 
* is persistent until we go back to polling.
*/
#define DC_LPE_REG_5                                            (DC_PCSLPE_LPE + 0x000000000028)
#define DC_LPE_REG_5_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_5_UNUSED_31_4_SHIFT                          4
#define DC_LPE_REG_5_UNUSED_31_4_MASK                           0xFFFFFFFull
#define DC_LPE_REG_5_UNUSED_31_4_SMASK                          0xFFFFFFF0ull
#define DC_LPE_REG_5_SCRAMBLING_ACTIVE_SHIFT                    3
#define DC_LPE_REG_5_SCRAMBLING_ACTIVE_MASK                     0x1ull
#define DC_LPE_REG_5_SCRAMBLING_ACTIVE_SMASK                    0x8ull
#define DC_LPE_REG_5_LOCAL_SCRAMBLE_CAPABILITY_SHIFT            2
#define DC_LPE_REG_5_LOCAL_SCRAMBLE_CAPABILITY_MASK             0x1ull
#define DC_LPE_REG_5_LOCAL_SCRAMBLE_CAPABILITY_SMASK            0x4ull
#define DC_LPE_REG_5_REMOTE_SCRAMBLE_CAPABILITY_SHIFT           1
#define DC_LPE_REG_5_REMOTE_SCRAMBLE_CAPABILITY_MASK            0x1ull
#define DC_LPE_REG_5_REMOTE_SCRAMBLE_CAPABILITY_SMASK           0x2ull
#define DC_LPE_REG_5_FORCE_SCRAMBLE_SHIFT                       0
#define DC_LPE_REG_5_FORCE_SCRAMBLE_MASK                        0x1ull
#define DC_LPE_REG_5_FORCE_SCRAMBLE_SMASK                       0x1ull
/*
* Table #7 of 240_CSR_pcslpe.xml - lpe_reg_8
* See description below for individual field definition
*/
#define DC_LPE_REG_8                                            (DC_PCSLPE_LPE + 0x000000000040)
#define DC_LPE_REG_8_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_8_UNUSED_31_20_SHIFT                         20
#define DC_LPE_REG_8_UNUSED_31_20_MASK                          0xFFFull
#define DC_LPE_REG_8_UNUSED_31_20_SMASK                         0xFFF00000ull
#define DC_LPE_REG_8_POLLING_HOLD_TIMEOUT_SHIFT                 18
#define DC_LPE_REG_8_POLLING_HOLD_TIMEOUT_MASK                  0x3ull
#define DC_LPE_REG_8_POLLING_HOLD_TIMEOUT_SMASK                 0xC0000ull
#define DC_LPE_REG_8_POLLING_ACTIVE_TIMEOUT_SHIFT               16
#define DC_LPE_REG_8_POLLING_ACTIVE_TIMEOUT_MASK                0x3ull
#define DC_LPE_REG_8_POLLING_ACTIVE_TIMEOUT_SMASK               0x30000ull
#define DC_LPE_REG_8_POLLING_QUIET_TIMEOUT_SHIFT                14
#define DC_LPE_REG_8_POLLING_QUIET_TIMEOUT_MASK                 0x3ull
#define DC_LPE_REG_8_POLLING_QUIET_TIMEOUT_SMASK                0xC000ull
#define DC_LPE_REG_8_DEBOUNCE_TIMEOUT_SHIFT                     12
#define DC_LPE_REG_8_DEBOUNCE_TIMEOUT_MASK                      0x3ull
#define DC_LPE_REG_8_DEBOUNCE_TIMEOUT_SMASK                     0x3000ull
#define DC_LPE_REG_8_RCVR_CFG_HOLD_TIMEOUT_SHIFT                10
#define DC_LPE_REG_8_RCVR_CFG_HOLD_TIMEOUT_MASK                 0x3ull
#define DC_LPE_REG_8_RCVR_CFG_HOLD_TIMEOUT_SMASK                0xC00ull
#define DC_LPE_REG_8_RCVR_CFG_TIMEOUT_SHIFT                     8
#define DC_LPE_REG_8_RCVR_CFG_TIMEOUT_MASK                      0x3ull
#define DC_LPE_REG_8_RCVR_CFG_TIMEOUT_SMASK                     0x300ull
#define DC_LPE_REG_8_WAIT_RMT_TIMEOUT_SHIFT                     6
#define DC_LPE_REG_8_WAIT_RMT_TIMEOUT_MASK                      0x3ull
#define DC_LPE_REG_8_WAIT_RMT_TIMEOUT_SMASK                     0xC0ull
#define DC_LPE_REG_8_WAIT_RMT_HOLD_TIMEOUT_SHIFT                4
#define DC_LPE_REG_8_WAIT_RMT_HOLD_TIMEOUT_MASK                 0x3ull
#define DC_LPE_REG_8_WAIT_RMT_HOLD_TIMEOUT_SMASK                0x30ull
#define DC_LPE_REG_8_TX_REV_LANES_HOLD_TIMEOUT_SHIFT            2
#define DC_LPE_REG_8_TX_REV_LANES_HOLD_TIMEOUT_MASK             0x3ull
#define DC_LPE_REG_8_TX_REV_LANES_HOLD_TIMEOUT_SMASK            0xCull
#define DC_LPE_REG_8_TX_REV_LANES_TIMEOUT_SHIFT                 0
#define DC_LPE_REG_8_TX_REV_LANES_TIMEOUT_MASK                  0x3ull
#define DC_LPE_REG_8_TX_REV_LANES_TIMEOUT_SMASK                 0x3ull
/*
* Table #8 of 240_CSR_pcslpe.xml - lpe_reg_9
* See description below for individual field definition
*/
#define DC_LPE_REG_9                                            (DC_PCSLPE_LPE + 0x000000000048)
#define DC_LPE_REG_9_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_9_UNUSED_31_26_SHIFT                         26
#define DC_LPE_REG_9_UNUSED_31_26_MASK                          0x3Full
#define DC_LPE_REG_9_UNUSED_31_26_SMASK                         0xFC000000ull
#define DC_LPE_REG_9_CFG_ENHANCED_TIMEOUT_SHIFT                 24
#define DC_LPE_REG_9_CFG_ENHANCED_TIMEOUT_MASK                  0x3ull
#define DC_LPE_REG_9_CFG_ENHANCED_TIMEOUT_SMASK                 0x3000000ull
#define DC_LPE_REG_9_CFG_ENHANCED_HOLD_TIMEOUT_SHIFT            22
#define DC_LPE_REG_9_CFG_ENHANCED_HOLD_TIMEOUT_MASK             0x3ull
#define DC_LPE_REG_9_CFG_ENHANCED_HOLD_TIMEOUT_SMASK            0xC00000ull
#define DC_LPE_REG_9_WAIT_CFG_ENHANCED_TIMEOUT_SHIFT            20
#define DC_LPE_REG_9_WAIT_CFG_ENHANCED_TIMEOUT_MASK             0x3ull
#define DC_LPE_REG_9_WAIT_CFG_ENHANCED_TIMEOUT_SMASK            0x300000ull
#define DC_LPE_REG_9_WAIT_RMT_TEST_TIMEOUT_SHIFT                18
#define DC_LPE_REG_9_WAIT_RMT_TEST_TIMEOUT_MASK                 0x3ull
#define DC_LPE_REG_9_WAIT_RMT_TEST_TIMEOUT_SMASK                0xC0000ull
#define DC_LPE_REG_9_CFG_IDLE_TIMEOUT_SHIFT                     16
#define DC_LPE_REG_9_CFG_IDLE_TIMEOUT_MASK                      0x3ull
#define DC_LPE_REG_9_CFG_IDLE_TIMEOUT_SMASK                     0x30000ull
#define DC_LPE_REG_9_CFG_TEST_TIMEOUT_SHIFT                     14
#define DC_LPE_REG_9_CFG_TEST_TIMEOUT_MASK                      0x3ull
#define DC_LPE_REG_9_CFG_TEST_TIMEOUT_SMASK                     0xC000ull
#define DC_LPE_REG_9_CFG_IDLE_HOLD_TIMEOUT_SHIFT                12
#define DC_LPE_REG_9_CFG_IDLE_HOLD_TIMEOUT_MASK                 0x3ull
#define DC_LPE_REG_9_CFG_IDLE_HOLD_TIMEOUT_SMASK                0x3000ull
#define DC_LPE_REG_9_LINK_UP_DETECTED_HOLD_TIMEOUT_SHIFT        10
#define DC_LPE_REG_9_LINK_UP_DETECTED_HOLD_TIMEOUT_MASK         0x3ull
#define DC_LPE_REG_9_LINK_UP_DETECTED_HOLD_TIMEOUT_SMASK        0xC00ull
#define DC_LPE_REG_9_LINK_UP_HOLD_TIMEOUT_SHIFT                 8
#define DC_LPE_REG_9_LINK_UP_HOLD_TIMEOUT_MASK                  0x3ull
#define DC_LPE_REG_9_LINK_UP_HOLD_TIMEOUT_SMASK                 0x300ull
#define DC_LPE_REG_9_REC_RE_TRAIN_HOLD_TIMEOUT_SHIFT            6
#define DC_LPE_REG_9_REC_RE_TRAIN_HOLD_TIMEOUT_MASK             0x3ull
#define DC_LPE_REG_9_REC_RE_TRAIN_HOLD_TIMEOUT_SMASK            0xC0ull
#define DC_LPE_REG_9_REC_WAIT_RMT_HOLD_TIMEOUT_SHIFT            4
#define DC_LPE_REG_9_REC_WAIT_RMT_HOLD_TIMEOUT_MASK             0x3ull
#define DC_LPE_REG_9_REC_WAIT_RMT_HOLD_TIMEOUT_SMASK            0x30ull
#define DC_LPE_REG_9_REC_IDLE_HOLD_TIMEOUT_SHIFT                2
#define DC_LPE_REG_9_REC_IDLE_HOLD_TIMEOUT_MASK                 0x3ull
#define DC_LPE_REG_9_REC_IDLE_HOLD_TIMEOUT_SMASK                0xCull
#define DC_LPE_REG_9_RECOVERY_TIMEOUT_SHIFT                     0
#define DC_LPE_REG_9_RECOVERY_TIMEOUT_MASK                      0x3ull
#define DC_LPE_REG_9_RECOVERY_TIMEOUT_SMASK                     0x3ull
/*
* Table #9 of 240_CSR_pcslpe.xml - lpe_reg_A
* See description below for individual field definition
*/
#define DC_LPE_REG_A                                            (DC_PCSLPE_LPE + 0x000000000050)
#define DC_LPE_REG_A_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_A_UNUSED_31_19_SHIFT                         19
#define DC_LPE_REG_A_UNUSED_31_19_MASK                          0x1FFFull
#define DC_LPE_REG_A_UNUSED_31_19_SMASK                         0xFFF80000ull
#define DC_LPE_REG_A_RCV_TS_T_VALID_SHIFT                       18
#define DC_LPE_REG_A_RCV_TS_T_VALID_MASK                        0x1ull
#define DC_LPE_REG_A_RCV_TS_T_VALID_SMASK                       0x40000ull
#define DC_LPE_REG_A_RCV_TS_T_LANE_SHIFT                        16
#define DC_LPE_REG_A_RCV_TS_T_LANE_MASK                         0x3ull
#define DC_LPE_REG_A_RCV_TS_T_LANE_SMASK                        0x30000ull
#define DC_LPE_REG_A_RCV_TS_T_SPEEDS_SHIFT                      8
#define DC_LPE_REG_A_RCV_TS_T_SPEEDS_MASK                       0xFFull
#define DC_LPE_REG_A_RCV_TS_T_SPEEDS_SMASK                      0xFF00ull
#define DC_LPE_REG_A_RCV_TS_T_OPCODE_SHIFT                      0
#define DC_LPE_REG_A_RCV_TS_T_OPCODE_MASK                       0xFFull
#define DC_LPE_REG_A_RCV_TS_T_OPCODE_SMASK                      0xFFull
/*
* Table #10 of 240_CSR_pcslpe.xml - lpe_reg_B
* See description below for individual field definition
*/
#define DC_LPE_REG_B                                            (DC_PCSLPE_LPE + 0x000000000058)
#define DC_LPE_REG_B_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_B_RCV_TS_T_TX_CFG_SHIFT                      16
#define DC_LPE_REG_B_RCV_TS_T_TX_CFG_MASK                       0xFFFFull
#define DC_LPE_REG_B_RCV_TS_T_TX_CFG_SMASK                      0xFFFF0000ull
#define DC_LPE_REG_B_RCV_TS_T_RX_CFG_SHIFT                      0
#define DC_LPE_REG_B_RCV_TS_T_RX_CFG_MASK                       0xFFFFull
#define DC_LPE_REG_B_RCV_TS_T_RX_CFG_SMASK                      0xFFFFull
/*
* Table #11 of 240_CSR_pcslpe.xml - lpe_reg_C
* See description below for individual field definition
*/
#define DC_LPE_REG_C                                            (DC_PCSLPE_LPE + 0x000000000060)
#define DC_LPE_REG_C_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_C_UNUSED_31_19_SHIFT                         19
#define DC_LPE_REG_C_UNUSED_31_19_MASK                          0x1FFFull
#define DC_LPE_REG_C_UNUSED_31_19_SMASK                         0xFFF80000ull
#define DC_LPE_REG_C_XMIT_TS_T_VALID_SHIFT                      18
#define DC_LPE_REG_C_XMIT_TS_T_VALID_MASK                       0x1ull
#define DC_LPE_REG_C_XMIT_TS_T_VALID_SMASK                      0x40000ull
#define DC_LPE_REG_C_XMIT_TS_T_LANE_SHIFT                       16
#define DC_LPE_REG_C_XMIT_TS_T_LANE_MASK                        0x3ull
#define DC_LPE_REG_C_XMIT_TS_T_LANE_SMASK                       0x30000ull
#define DC_LPE_REG_C_XMIT_TS_T_SPEEDS_SHIFT                     8
#define DC_LPE_REG_C_XMIT_TS_T_SPEEDS_MASK                      0xFFull
#define DC_LPE_REG_C_XMIT_TS_T_SPEEDS_SMASK                     0xFF00ull
#define DC_LPE_REG_C_XMIT_TS_T_OPCODE_SHIFT                     0
#define DC_LPE_REG_C_XMIT_TS_T_OPCODE_MASK                      0xFFull
#define DC_LPE_REG_C_XMIT_TS_T_OPCODE_SMASK                     0xFFull
/*
* Table #12 of 240_CSR_pcslpe.xml - lpe_reg_D
* See description below for individual field definition
*/
#define DC_LPE_REG_D                                            (DC_PCSLPE_LPE + 0x000000000068)
#define DC_LPE_REG_D_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_D_XMIT_TS_T_TX_CFG_SHIFT                     16
#define DC_LPE_REG_D_XMIT_TS_T_TX_CFG_MASK                      0xFFFFull
#define DC_LPE_REG_D_XMIT_TS_T_TX_CFG_SMASK                     0xFFFF0000ull
#define DC_LPE_REG_D_XMIT_TS_T_RX_CFG_SHIFT                     0
#define DC_LPE_REG_D_XMIT_TS_T_RX_CFG_MASK                      0xFFFFull
#define DC_LPE_REG_D_XMIT_TS_T_RX_CFG_SMASK                     0xFFFFull
/*
* Table #13 of 240_CSR_pcslpe.xml - lpe_reg_E
* Spare Register at this time.
*/
#define DC_LPE_REG_E                                            (DC_PCSLPE_LPE + 0x000000000070)
#define DC_LPE_REG_E_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_E_UNUSED_31_0_SHIFT                          0
#define DC_LPE_REG_E_UNUSED_31_0_MASK                           0xFFFFFFFFull
#define DC_LPE_REG_E_UNUSED_31_0_SMASK                          0xFFFFFFFFull
/*
* Table #14 of 240_CSR_pcslpe.xml - lpe_reg_F
* Heartbeats SND packets may be sent manually using the Heartbeat request or 
* sent automatically using Heartbeat auto control.  The PORT and GUID values are 
* placed in the Heartbeat SND packet.  Heartbeat ACK packets are sent when a 
* Heartbeat SND packet has been received.  The Heartbeat ACK packet must contain 
* the same GUID value from the Heartbeat SND packet received.  Heartbeat ACK 
* packets received must contain GUID value matching the GUID of the Heartbeat 
* SND packet.  If the GUID's do not match the ACK packet is ignored.  If a 
* Heartbeat SND packet is received with a GUID matching the port GUID, this 
* indicates channel crosstalk.
*/
#define DC_LPE_REG_F                                            (DC_PCSLPE_LPE + 0x000000000078)
#define DC_LPE_REG_F_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_F_HB_SND_XMIT_VALID_SHIFT                    31
#define DC_LPE_REG_F_HB_SND_XMIT_VALID_MASK                     0x1ull
#define DC_LPE_REG_F_HB_SND_XMIT_VALID_SMASK                    0x80000000ull
#define DC_LPE_REG_F_HB_SND_RCVD_VALID_SHIFT                    30
#define DC_LPE_REG_F_HB_SND_RCVD_VALID_MASK                     0x1ull
#define DC_LPE_REG_F_HB_SND_RCVD_VALID_SMASK                    0x40000000ull
#define DC_LPE_REG_F_HB_ACK_XMIT_VALID_SHIFT                    29
#define DC_LPE_REG_F_HB_ACK_XMIT_VALID_MASK                     0x1ull
#define DC_LPE_REG_F_HB_ACK_XMIT_VALID_SMASK                    0x20000000ull
#define DC_LPE_REG_F_HB_ACK_RCVD_VALID_SHIFT                    28
#define DC_LPE_REG_F_HB_ACK_RCVD_VALID_MASK                     0x1ull
#define DC_LPE_REG_F_HB_ACK_RCVD_VALID_SMASK                    0x10000000ull
#define DC_LPE_REG_F_HB_TIMED_OUT_SHIFT                         27
#define DC_LPE_REG_F_HB_TIMED_OUT_MASK                          0x1ull
#define DC_LPE_REG_F_HB_TIMED_OUT_SMASK                         0x8000000ull
#define DC_LPE_REG_F_UNUSED_26_18_SHIFT                         18
#define DC_LPE_REG_F_UNUSED_26_18_MASK                          0x1FFull
#define DC_LPE_REG_F_UNUSED_26_18_SMASK                         0x7FC0000ull
#define DC_LPE_REG_F_UNUSED_17_16_SHIFT                         16
#define DC_LPE_REG_F_UNUSED_17_16_MASK                          0x3ull
#define DC_LPE_REG_F_UNUSED_17_16_SMASK                         0x30000ull
#define DC_LPE_REG_F_RCV_SND_HB_PORT_NUM_SHIFT                  8
#define DC_LPE_REG_F_RCV_SND_HB_PORT_NUM_MASK                   0xFFull
#define DC_LPE_REG_F_RCV_SND_HB_PORT_NUM_SMASK                  0xFF00ull
#define DC_LPE_REG_F_RCV_ACK_HB_PORT_NUM_SHIFT                  0
#define DC_LPE_REG_F_RCV_ACK_HB_PORT_NUM_MASK                   0xFFull
#define DC_LPE_REG_F_RCV_ACK_HB_PORT_NUM_SMASK                  0xFFull
/*
* Table #15 of 240_CSR_pcslpe.xml - lpe_reg_10
* See description below for individual field definition
*/
#define DC_LPE_REG_10                                           (DC_PCSLPE_LPE + 0x000000000080)
#define DC_LPE_REG_10_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_10_RCV_SND_HB_GUID_U_SHIFT                   0
#define DC_LPE_REG_10_RCV_SND_HB_GUID_U_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_10_RCV_SND_HB_GUID_U_SMASK                   0xFFFFFFFFull
/*
* Table #16 of 240_CSR_pcslpe.xml - lpe_reg_11
* See description below for individual field definition
*/
#define DC_LPE_REG_11                                           (DC_PCSLPE_LPE + 0x000000000088)
#define DC_LPE_REG_11_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_11_RCV_SND_HB_GUID_L_SHIFT                   0
#define DC_LPE_REG_11_RCV_SND_HB_GUID_L_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_11_RCV_SND_HB_GUID_L_SMASK                   0xFFFFFFFFull
/*
* Table #17 of 240_CSR_pcslpe.xml - lpe_reg_12
* See description below for individual field definition
*/
#define DC_LPE_REG_12                                           (DC_PCSLPE_LPE + 0x000000000090)
#define DC_LPE_REG_12_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_12_RCV_ACK_HB_GUID_U_SHIFT                   0
#define DC_LPE_REG_12_RCV_ACK_HB_GUID_U_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_12_RCV_ACK_HB_GUID_U_SMASK                   0xFFFFFFFFull
/*
* Table #18 of 240_CSR_pcslpe.xml - lpe_reg_13
* See description below for individual field definition
*/
#define DC_LPE_REG_13                                           (DC_PCSLPE_LPE + 0x000000000098)
#define DC_LPE_REG_13_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_13_RCV_ACK_HB_GUID_L_SHIFT                   0
#define DC_LPE_REG_13_RCV_ACK_HB_GUID_L_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_13_RCV_ACK_HB_GUID_L_SMASK                   0xFFFFFFFFull
/*
* Table #19 of 240_CSR_pcslpe.xml - lpe_reg_14
* See description below for individual field definition
*/
#define DC_LPE_REG_14                                           (DC_PCSLPE_LPE + 0x0000000000A0)
#define DC_LPE_REG_14_RESETCSR                                  0x000F0000ull
#define DC_LPE_REG_14_UNUSED_31_28_SHIFT                        28
#define DC_LPE_REG_14_UNUSED_31_28_MASK                         0xFull
#define DC_LPE_REG_14_UNUSED_31_28_SMASK                        0xF0000000ull
#define DC_LPE_REG_14_HB_CROSSTALK_SHIFT                        24
#define DC_LPE_REG_14_HB_CROSSTALK_MASK                         0xFull
#define DC_LPE_REG_14_HB_CROSSTALK_SMASK                        0xF000000ull
#define DC_LPE_REG_14_UNUSED_23_SHIFT                           23
#define DC_LPE_REG_14_UNUSED_23_MASK                            0x1ull
#define DC_LPE_REG_14_UNUSED_23_SMASK                           0x800000ull
#define DC_LPE_REG_14_XMIT_HB_ENABLE_SHIFT                      22
#define DC_LPE_REG_14_XMIT_HB_ENABLE_MASK                       0x1ull
#define DC_LPE_REG_14_XMIT_HB_ENABLE_SMASK                      0x400000ull
#define DC_LPE_REG_14_XMIT_HB_REQUEST_SHIFT                     21
#define DC_LPE_REG_14_XMIT_HB_REQUEST_MASK                      0x1ull
#define DC_LPE_REG_14_XMIT_HB_REQUEST_SMASK                     0x200000ull
#define DC_LPE_REG_14_XMIT_HB_AUTO_SHIFT                        20
#define DC_LPE_REG_14_XMIT_HB_AUTO_MASK                         0x1ull
#define DC_LPE_REG_14_XMIT_HB_AUTO_SMASK                        0x100000ull
#define DC_LPE_REG_14_XMIT_HB_LANE_MASK_SHIFT                   16
#define DC_LPE_REG_14_XMIT_HB_LANE_MASK_MASK                    0xFull
#define DC_LPE_REG_14_XMIT_HB_LANE_MASK_SMASK                   0xF0000ull
#define DC_LPE_REG_14_XMIT_HB_OPCODE_SHIFT                      8
#define DC_LPE_REG_14_XMIT_HB_OPCODE_MASK                       0xFFull
#define DC_LPE_REG_14_XMIT_HB_OPCODE_SMASK                      0xFF00ull
#define DC_LPE_REG_14_XMIT_HB_PORT_NUM_SHIFT                    0
#define DC_LPE_REG_14_XMIT_HB_PORT_NUM_MASK                     0xFFull
#define DC_LPE_REG_14_XMIT_HB_PORT_NUM_SMASK                    0xFFull
/*
* Table #20 of 240_CSR_pcslpe.xml - lpe_reg_15
* See description below for individual field definition
*/
#define DC_LPE_REG_15                                           (DC_PCSLPE_LPE + 0x0000000000A8)
#define DC_LPE_REG_15_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_15_XMIT_HB_GUID_UPPR_SHIFT                   0
#define DC_LPE_REG_15_XMIT_HB_GUID_UPPR_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_15_XMIT_HB_GUID_UPPR_SMASK                   0xFFFFFFFFull
/*
* Table #21 of 240_CSR_pcslpe.xml - lpe_reg_16
* See description below for individual field definition
*/
#define DC_LPE_REG_16                                           (DC_PCSLPE_LPE + 0x0000000000B0)
#define DC_LPE_REG_16_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_16_XMIT_HB_GUID_LOWR_SHIFT                   0
#define DC_LPE_REG_16_XMIT_HB_GUID_LOWR_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_16_XMIT_HB_GUID_LOWR_SMASK                   0xFFFFFFFFull
/*
* Table #22 of 240_CSR_pcslpe.xml - lpe_reg_17
* See description below for individual field definition
*/
#define DC_LPE_REG_17                                           (DC_PCSLPE_LPE + 0x0000000000B8)
#define DC_LPE_REG_17_RESETCSR                                  0x07FFFFFFull
#define DC_LPE_REG_17_UNUSED_31_27_SHIFT                        27
#define DC_LPE_REG_17_UNUSED_31_27_MASK                         0x1Full
#define DC_LPE_REG_17_UNUSED_31_27_SMASK                        0xF8000000ull
#define DC_LPE_REG_17_HB_LATENCY_SHIFT                          0
#define DC_LPE_REG_17_HB_LATENCY_MASK                           0x7FFFFFFull
#define DC_LPE_REG_17_HB_LATENCY_SMASK                          0x7FFFFFFull
/*
* Table #23 of 240_CSR_pcslpe.xml - lpe_reg_18
* See description below for individual field definition
*/
#define DC_LPE_REG_18                                           (DC_PCSLPE_LPE + 0x0000000000C0)
#define DC_LPE_REG_18_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_18_TS_T_OS_DETECTED_SHIFT                    28
#define DC_LPE_REG_18_TS_T_OS_DETECTED_MASK                     0xFull
#define DC_LPE_REG_18_TS_T_OS_DETECTED_SMASK                    0xF0000000ull
#define DC_LPE_REG_18_TS12_OS_ERR_DETECTED_SHIFT                24
#define DC_LPE_REG_18_TS12_OS_ERR_DETECTED_MASK                 0xFull
#define DC_LPE_REG_18_TS12_OS_ERR_DETECTED_SMASK                0xF000000ull
#define DC_LPE_REG_18_TS2_SEQ_DETECTED_SHIFT                    20
#define DC_LPE_REG_18_TS2_SEQ_DETECTED_MASK                     0xFull
#define DC_LPE_REG_18_TS2_SEQ_DETECTED_SMASK                    0xF00000ull
#define DC_LPE_REG_18_TS1_SEQ_DETECTED_SHIFT                    16
#define DC_LPE_REG_18_TS1_SEQ_DETECTED_MASK                     0xFull
#define DC_LPE_REG_18_TS1_SEQ_DETECTED_SMASK                    0xF0000ull
#define DC_LPE_REG_18_SKP_OS_DETECTED_SHIFT                     12
#define DC_LPE_REG_18_SKP_OS_DETECTED_MASK                      0xFull
#define DC_LPE_REG_18_SKP_OS_DETECTED_SMASK                     0xF000ull
#define DC_LPE_REG_18_TS3_OS_DETECTED_SHIFT                     8
#define DC_LPE_REG_18_TS3_OS_DETECTED_MASK                      0xFull
#define DC_LPE_REG_18_TS3_OS_DETECTED_SMASK                     0xF00ull
#define DC_LPE_REG_18_TS2_OS_DETECTED_SHIFT                     4
#define DC_LPE_REG_18_TS2_OS_DETECTED_MASK                      0xFull
#define DC_LPE_REG_18_TS2_OS_DETECTED_SMASK                     0xF0ull
#define DC_LPE_REG_18_TS1_OS_DETECTED_SHIFT                     0
#define DC_LPE_REG_18_TS1_OS_DETECTED_MASK                      0xFull
#define DC_LPE_REG_18_TS1_OS_DETECTED_SMASK                     0xFull
/*
* Table #24 of 240_CSR_pcslpe.xml - lpe_reg_19
* See description below for individual field definition
*/
#define DC_LPE_REG_19                                           (DC_PCSLPE_LPE + 0x0000000000C8)
#define DC_LPE_REG_19_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_19_L0_TS1_CNT_SHIFT                          24
#define DC_LPE_REG_19_L0_TS1_CNT_MASK                           0xFFull
#define DC_LPE_REG_19_L0_TS1_CNT_SMASK                          0xFF000000ull
/*
* Table #25 of 240_CSR_pcslpe.xml - lpe_reg_1A
* See description below for individual field definition
*/
#define DC_LPE_REG_1A                                           (DC_PCSLPE_LPE + 0x0000000000D0)
#define DC_LPE_REG_1A_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_1A_L0_TS2_CNT_SHIFT                          24
#define DC_LPE_REG_1A_L0_TS2_CNT_MASK                           0xFFull
#define DC_LPE_REG_1A_L0_TS2_CNT_SMASK                          0xFF000000ull
/*
* Table #26 of 240_CSR_pcslpe.xml - lpe_reg_1B
* See description below for individual field definition
*/
#define DC_LPE_REG_1B                                           (DC_PCSLPE_LPE + 0x0000000000D8)
#define DC_LPE_REG_1B_RESETCSR                                  0x00080000ull
#define DC_LPE_REG_1B_TS12_ALIVE_CNT_THRES_SHIFT                16
#define DC_LPE_REG_1B_TS12_ALIVE_CNT_THRES_MASK                 0xFFFFull
#define DC_LPE_REG_1B_TS12_ALIVE_CNT_THRES_SMASK                0xFFFF0000ull
#define DC_LPE_REG_1B_LANE_ALIVE_SHIFT                          12
#define DC_LPE_REG_1B_LANE_ALIVE_MASK                           0xFull
#define DC_LPE_REG_1B_LANE_ALIVE_SMASK                          0xF000ull
#define DC_LPE_REG_1B_HOLD_TIME_LANE_ALIVE_SHIFT                8
#define DC_LPE_REG_1B_HOLD_TIME_LANE_ALIVE_MASK                 0xFull
#define DC_LPE_REG_1B_HOLD_TIME_LANE_ALIVE_SMASK                0xF00ull
#define DC_LPE_REG_1B_UNUSED_7_2_SHIFT                          2
#define DC_LPE_REG_1B_UNUSED_7_2_MASK                           0x3Full
#define DC_LPE_REG_1B_UNUSED_7_2_SMASK                          0xFCull
#define DC_LPE_REG_1B_OS_SEQ_CNT_SHIFT                          0
#define DC_LPE_REG_1B_OS_SEQ_CNT_MASK                           0x3ull
#define DC_LPE_REG_1B_OS_SEQ_CNT_SMASK                          0x3ull
/*
* Table #27 of 240_CSR_pcslpe.xml - lpe_reg_1C
* See description below for individual field definition
*/
#define DC_LPE_REG_1C                                           (DC_PCSLPE_LPE + 0x0000000000E0)
#define DC_LPE_REG_1C_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_1C_UNUSED_31_25_SHIFT                        25
#define DC_LPE_REG_1C_UNUSED_31_25_MASK                         0x7Full
#define DC_LPE_REG_1C_UNUSED_31_25_SMASK                        0xFE000000ull
#define DC_LPE_REG_1C_TX_MOD_TS1_EN_SHIFT                       24
#define DC_LPE_REG_1C_TX_MOD_TS1_EN_MASK                        0x1ull
#define DC_LPE_REG_1C_TX_MOD_TS1_EN_SMASK                       0x1000000ull
#define DC_LPE_REG_1C_TX_MOD_TS1_DATA_SHIFT                     0
#define DC_LPE_REG_1C_TX_MOD_TS1_DATA_MASK                      0xFFFFFFull
#define DC_LPE_REG_1C_TX_MOD_TS1_DATA_SMASK                     0xFFFFFFull
/*
* Table #28 of 240_CSR_pcslpe.xml - lpe_reg_1D
* See description below for individual field definition
*/
#define DC_LPE_REG_1D                                           (DC_PCSLPE_LPE + 0x0000000000E8)
#define DC_LPE_REG_1D_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_1D_UNUSED_31_25_SHIFT                        25
#define DC_LPE_REG_1D_UNUSED_31_25_MASK                         0x7Full
#define DC_LPE_REG_1D_UNUSED_31_25_SMASK                        0xFE000000ull
#define DC_LPE_REG_1D_RX_MOD_TS1_SAMPLED_SHIFT                  24
#define DC_LPE_REG_1D_RX_MOD_TS1_SAMPLED_MASK                   0x1ull
#define DC_LPE_REG_1D_RX_MOD_TS1_SAMPLED_SMASK                  0x1000000ull
#define DC_LPE_REG_1D_RX_MOD_TS1_DATA_SHIFT                     0
#define DC_LPE_REG_1D_RX_MOD_TS1_DATA_MASK                      0xFFFFFFull
#define DC_LPE_REG_1D_RX_MOD_TS1_DATA_SMASK                     0xFFFFFFull
/*
* Table #29 of 240_CSR_pcslpe.xml - lpe_reg_1E
* See description below for individual field definition
*/
#define DC_LPE_REG_1E                                           (DC_PCSLPE_LPE + 0x0000000000F0)
#define DC_LPE_REG_1E_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_1E_UNUSED_31_25_SHIFT                        25
#define DC_LPE_REG_1E_UNUSED_31_25_MASK                         0x7Full
#define DC_LPE_REG_1E_UNUSED_31_25_SMASK                        0xFE000000ull
#define DC_LPE_REG_1E_CFFT_TX_REQ_SHIFT                         24
#define DC_LPE_REG_1E_CFFT_TX_REQ_MASK                          0x1ull
#define DC_LPE_REG_1E_CFFT_TX_REQ_SMASK                         0x1000000ull
#define DC_LPE_REG_1E_CCFT_TX_CMD_SHIFT                         16
#define DC_LPE_REG_1E_CCFT_TX_CMD_MASK                          0xFFull
#define DC_LPE_REG_1E_CCFT_TX_CMD_SMASK                         0xFF0000ull
#define DC_LPE_REG_1E_CCFT_TX_STATUS_SHIFT                      8
#define DC_LPE_REG_1E_CCFT_TX_STATUS_MASK                       0xFFull
#define DC_LPE_REG_1E_CCFT_TX_STATUS_SMASK                      0xFF00ull
#define DC_LPE_REG_1E_CCFT_TX_LANE_SHIFT                        0
#define DC_LPE_REG_1E_CCFT_TX_LANE_MASK                         0xFFull
#define DC_LPE_REG_1E_CCFT_TX_LANE_SMASK                        0xFFull
/*
* Table #30 of 240_CSR_pcslpe.xml - lpe_reg_1F
* See description below for individual field definition
*/
#define DC_LPE_REG_1F                                           (DC_PCSLPE_LPE + 0x0000000000F8)
#define DC_LPE_REG_1F_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_1F_UNUSED_31_25_SHIFT                        25
#define DC_LPE_REG_1F_UNUSED_31_25_MASK                         0x7Full
#define DC_LPE_REG_1F_UNUSED_31_25_SMASK                        0xFE000000ull
#define DC_LPE_REG_1F_CCFT_RX_REQ_SHIFT                         24
#define DC_LPE_REG_1F_CCFT_RX_REQ_MASK                          0x1ull
#define DC_LPE_REG_1F_CCFT_RX_REQ_SMASK                         0x1000000ull
#define DC_LPE_REG_1F_CCFT_RX_CMD_SHIFT                         16
#define DC_LPE_REG_1F_CCFT_RX_CMD_MASK                          0xFFull
#define DC_LPE_REG_1F_CCFT_RX_CMD_SMASK                         0xFF0000ull
#define DC_LPE_REG_1F_CCFT_RX_STATUS_SHIFT                      8
#define DC_LPE_REG_1F_CCFT_RX_STATUS_MASK                       0xFFull
#define DC_LPE_REG_1F_CCFT_RX_STATUS_SMASK                      0xFF00ull
#define DC_LPE_REG_1F_CCFT_RX_LANE_SHIFT                        0
#define DC_LPE_REG_1F_CCFT_RX_LANE_MASK                         0xFFull
#define DC_LPE_REG_1F_CCFT_RX_LANE_SMASK                        0xFFull
/*
* Table #31 of 240_CSR_pcslpe.xml - lpe_reg_20
* See description below for individual field definition
*/
#define DC_LPE_REG_20                                           (DC_PCSLPE_LPE + 0x000000000100)
#define DC_LPE_REG_20_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_20_TS3_SEQ_RX_DATA_89_SHIFT                  31
#define DC_LPE_REG_20_TS3_SEQ_RX_DATA_89_MASK                   0x1ull
#define DC_LPE_REG_20_TS3_SEQ_RX_DATA_89_SMASK                  0x80000000ull
#define DC_LPE_REG_20_UNUSED_30_16_SHIFT                        16
#define DC_LPE_REG_20_UNUSED_30_16_MASK                         0x7FFFull
#define DC_LPE_REG_20_UNUSED_30_16_SMASK                        0x7FFF0000ull
#define DC_LPE_REG_20_TS3BYTE8_SHIFT                            8
#define DC_LPE_REG_20_TS3BYTE8_MASK                             0xFFull
#define DC_LPE_REG_20_TS3BYTE8_SMASK                            0xFF00ull
#define DC_LPE_REG_20_TS3BYTE9_SHIFT                            0
#define DC_LPE_REG_20_TS3BYTE9_MASK                             0xFFull
#define DC_LPE_REG_20_TS3BYTE9_SMASK                            0xFFull
/*
* Table #32 of 240_CSR_pcslpe.xml - lpe_reg_21
* See description below for individual field definition
*/
#define DC_LPE_REG_21                                           (DC_PCSLPE_LPE + 0x000000000108)
#define DC_LPE_REG_21_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_21_TS3_RX_DATA_10_LANE_0_SHIFT               24
#define DC_LPE_REG_21_TS3_RX_DATA_10_LANE_0_MASK                0xFFull
#define DC_LPE_REG_21_TS3_RX_DATA_10_LANE_0_SMASK               0xFF000000ull
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_1_SHIFT               16
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_1_MASK                0xFFull
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_1_SMASK               0xFF0000ull
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_2_SHIFT               8
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_2_MASK                0xFFull
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_2_SMASK               0xFF00ull
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_3_SHIFT               0
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_3_MASK                0xFFull
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_3_SMASK               0xFFull
/*
* Table #33 of 240_CSR_pcslpe.xml - lpe_reg_22
* See description below for individual field definition
*/
#define DC_LPE_REG_22                                           (DC_PCSLPE_LPE + 0x000000000110)
#define DC_LPE_REG_22_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_0_SHIFT               24
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_0_MASK                0xFFull
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_0_SMASK               0xFF000000ull
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_1_SHIFT               16
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_1_MASK                0xFFull
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_1_SMASK               0xFF0000ull
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_2_SHIFT               8
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_2_MASK                0xFFull
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_2_SMASK               0xFF00ull
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_3_SHIFT               0
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_3_MASK                0xFFull
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_3_SMASK               0xFFull
/*
* Table #34 of 240_CSR_pcslpe.xml - lpe_reg_23
* See description below for individual field definition
*/
#define DC_LPE_REG_23                                           (DC_PCSLPE_LPE + 0x000000000118)
#define DC_LPE_REG_23_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_23_TS3_RX_BYTE_12_LANE_0_SHIFT               24
#define DC_LPE_REG_23_TS3_RX_BYTE_12_LANE_0_MASK                0xFFull
#define DC_LPE_REG_23_TS3_RX_BYTE_12_LANE_0_SMASK               0xFF000000ull
#define DC_LPE_REG_23_TS3_RX_BYTE_13_LANE_1_SHIFT               16
#define DC_LPE_REG_23_TS3_RX_BYTE_13_LANE_1_MASK                0xFFull
#define DC_LPE_REG_23_TS3_RX_BYTE_13_LANE_1_SMASK               0xFF0000ull
#define DC_LPE_REG_23_TS3_RX_BYTE_14_LANE_2_SHIFT               8
#define DC_LPE_REG_23_TS3_RX_BYTE_14_LANE_2_MASK                0xFFull
#define DC_LPE_REG_23_TS3_RX_BYTE_14_LANE_2_SMASK               0xFF00ull
#define DC_LPE_REG_23_TS3_RX_BYTE_15_LANE_3_SHIFT               0
#define DC_LPE_REG_23_TS3_RX_BYTE_15_LANE_3_MASK                0xFFull
#define DC_LPE_REG_23_TS3_RX_BYTE_15_LANE_3_SMASK               0xFFull
/*
* Table #35 of 240_CSR_pcslpe.xml - lpe_reg_24
* The following TX TS3 bytes are used when sending TS3's. See description below 
* for individual field definition#%%#Address = Base Address + Offset
*/
#define DC_LPE_REG_24                                           (DC_PCSLPE_LPE + 0x000000000120)
#define DC_LPE_REG_24_RESETCSR                                  0x00001040ull
#define DC_LPE_REG_24_UNUSED_31_16_SHIFT                        16
#define DC_LPE_REG_24_UNUSED_31_16_MASK                         0xFFFFull
#define DC_LPE_REG_24_UNUSED_31_16_SMASK                        0xFFFF0000ull
#define DC_LPE_REG_24_TS3_TX_BYTE_8_SHIFT                       8
#define DC_LPE_REG_24_TS3_TX_BYTE_8_MASK                        0xFFull
#define DC_LPE_REG_24_TS3_TX_BYTE_8_SMASK                       0xFF00ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_TT_SHIFT                    4
#define DC_LPE_REG_24_TS3_TX_BYTE_9_TT_MASK                     0xFull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_TT_SMASK                    0xF0ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_FT_SHIFT                    3
#define DC_LPE_REG_24_TS3_TX_BYTE_9_FT_MASK                     0x1ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_FT_SMASK                    0x8ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_SC_SHIFT                    2
#define DC_LPE_REG_24_TS3_TX_BYTE_9_SC_MASK                     0x1ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_SC_SMASK                    0x4ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_HBR_SHIFT                   1
#define DC_LPE_REG_24_TS3_TX_BYTE_9_HBR_MASK                    0x1ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_HBR_SMASK                   0x2ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_ADD_SHIFT                   0
#define DC_LPE_REG_24_TS3_TX_BYTE_9_ADD_MASK                    0x1ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_ADD_SMASK                   0x1ull
/*
* Table #36 of 240_CSR_pcslpe.xml - lpe_reg_25
* See description below for individual field definition
*/
#define DC_LPE_REG_25                                           (DC_PCSLPE_LPE + 0x000000000128)
#define DC_LPE_REG_25_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_25_TS3_TX_DATA_10_LANE_0_SHIFT               24
#define DC_LPE_REG_25_TS3_TX_DATA_10_LANE_0_MASK                0xFFull
#define DC_LPE_REG_25_TS3_TX_DATA_10_LANE_0_SMASK               0xFF000000ull
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_1_SHIFT               16
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_1_MASK                0xFFull
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_1_SMASK               0xFF0000ull
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_2_SHIFT               8
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_2_MASK                0xFFull
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_2_SMASK               0xFF00ull
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_3_SHIFT               0
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_3_MASK                0xFFull
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_3_SMASK               0xFFull
/*
* Table #37 of 240_CSR_pcslpe.xml - lpe_reg_26
* See description below for individual field definition
*/
#define DC_LPE_REG_26                                           (DC_PCSLPE_LPE + 0x000000000130)
#define DC_LPE_REG_26_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_0_SHIFT               24
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_0_MASK                0xFFull
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_0_SMASK               0xFF000000ull
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_1_SHIFT               16
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_1_MASK                0xFFull
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_1_SMASK               0xFF0000ull
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_2_SHIFT               8
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_2_MASK                0xFFull
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_2_SMASK               0xFF00ull
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_3_SHIFT               0
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_3_MASK                0xFFull
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_3_SMASK               0xFFull
/*
* Table #38 of 240_CSR_pcslpe.xml - lpe_reg_27
* See description below for individual field definition
*/
#define DC_LPE_REG_27                                           (DC_PCSLPE_LPE + 0x000000000138)
#define DC_LPE_REG_27_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_27_TS3_TX_BYTE_12_SHIFT                      24
#define DC_LPE_REG_27_TS3_TX_BYTE_12_MASK                       0xFFull
#define DC_LPE_REG_27_TS3_TX_BYTE_12_SMASK                      0xFF000000ull
#define DC_LPE_REG_27_TS3_TX_BYTE_13_SHIFT                      16
#define DC_LPE_REG_27_TS3_TX_BYTE_13_MASK                       0xFFull
#define DC_LPE_REG_27_TS3_TX_BYTE_13_SMASK                      0xFF0000ull
#define DC_LPE_REG_27_TS3_TX_BYTE_14_SHIFT                      8
#define DC_LPE_REG_27_TS3_TX_BYTE_14_MASK                       0xFFull
#define DC_LPE_REG_27_TS3_TX_BYTE_14_SMASK                      0xFF00ull
#define DC_LPE_REG_27_TS3_TX_BYTE_15_SHIFT                      0
#define DC_LPE_REG_27_TS3_TX_BYTE_15_MASK                       0xFFull
#define DC_LPE_REG_27_TS3_TX_BYTE_15_SMASK                      0xFFull
/*
* Table #39 of 240_CSR_pcslpe.xml - lpe_reg_28
* This register shows the status of the 'rx_lane_order_a,b,c,d' and 
* 'rx_lane_polarity'. These are the calculated values needed to 'Un-Swizzle' the 
* data lanes into the required order of 3,2,1,0. These values are provided to 
* the PCSs so that they can use them in the 'Auto_Polarity_Rev' mode of 
* operation. NOTE: For EDR-Only mode of operation, the 'Auto_Polarity_Rev' bit 
* in the PCS_66 Common register must be set to a '0' and then the 
* 'Cmn_Rx_Lane_Order' and 'Cmn_Rx_Lane_Polarity' values must be prorammed to the 
* desired values for correct operation. This is necessary since we do not have 
* the 8b10b speed helping to discover the Swizzle solution - it must be provided 
* by the programmer. See description below for individual field 
* definition.
*/
#define DC_LPE_REG_28                                           (DC_PCSLPE_LPE + 0x000000000140)
#define DC_LPE_REG_28_RESETCSR                                  0x2E40E402ull
#define DC_LPE_REG_28_UNUSED_31_SHIFT                           31
#define DC_LPE_REG_28_UNUSED_31_MASK                            0x1ull
#define DC_LPE_REG_28_UNUSED_31_SMASK                           0x80000000ull
#define DC_LPE_REG_28_RX_TRAINED_TIMEOUT_SHIFT                  30
#define DC_LPE_REG_28_RX_TRAINED_TIMEOUT_MASK                   0x1ull
#define DC_LPE_REG_28_RX_TRAINED_TIMEOUT_SMASK                  0x40000000ull
#define DC_LPE_REG_28_RX_TRAINED_TST_EN_SHIFT                   29
#define DC_LPE_REG_28_RX_TRAINED_TST_EN_MASK                    0x1ull
#define DC_LPE_REG_28_RX_TRAINED_TST_EN_SMASK                   0x20000000ull
#define DC_LPE_REG_28_RX_TRAINED_SHIFT                          28
#define DC_LPE_REG_28_RX_TRAINED_MASK                           0x1ull
#define DC_LPE_REG_28_RX_TRAINED_SMASK                          0x10000000ull
#define DC_LPE_REG_28_RX_LANE_ORDER_D_SHIFT                     26
#define DC_LPE_REG_28_RX_LANE_ORDER_D_MASK                      0x3ull
#define DC_LPE_REG_28_RX_LANE_ORDER_D_SMASK                     0xC000000ull
#define DC_LPE_REG_28_RX_LANE_ORDER_C_SHIFT                     24
#define DC_LPE_REG_28_RX_LANE_ORDER_C_MASK                      0x3ull
#define DC_LPE_REG_28_RX_LANE_ORDER_C_SMASK                     0x3000000ull
#define DC_LPE_REG_28_RX_LANE_ORDER_B_SHIFT                     22
#define DC_LPE_REG_28_RX_LANE_ORDER_B_MASK                      0x3ull
#define DC_LPE_REG_28_RX_LANE_ORDER_B_SMASK                     0xC00000ull
#define DC_LPE_REG_28_RX_LANE_ORDER_A_SHIFT                     20
#define DC_LPE_REG_28_RX_LANE_ORDER_A_MASK                      0x3ull
#define DC_LPE_REG_28_RX_LANE_ORDER_A_SMASK                     0x300000ull
#define DC_LPE_REG_28_RX_LANE_POLARITY_SHIFT                    16
#define DC_LPE_REG_28_RX_LANE_POLARITY_MASK                     0xFull
#define DC_LPE_REG_28_RX_LANE_POLARITY_SMASK                    0xF0000ull
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_D_SHIFT                 14
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_D_MASK                  0x3ull
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_D_SMASK                 0xC000ull
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_C_SHIFT                 12
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_C_MASK                  0x3ull
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_C_SMASK                 0x3000ull
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_B_SHIFT                 10
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_B_MASK                  0x3ull
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_B_SMASK                 0xC00ull
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_A_SHIFT                 8
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_A_MASK                  0x3ull
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_A_SMASK                 0x300ull
#define DC_LPE_REG_28_RX_RAW_LANE_POLARITY_SHIFT                4
#define DC_LPE_REG_28_RX_RAW_LANE_POLARITY_MASK                 0xFull
#define DC_LPE_REG_28_RX_RAW_LANE_POLARITY_SMASK                0xF0ull
#define DC_LPE_REG_28_DISABLE_LINK_REC_SHIFT                    3
#define DC_LPE_REG_28_DISABLE_LINK_REC_MASK                     0x1ull
#define DC_LPE_REG_28_DISABLE_LINK_REC_SMASK                    0x8ull
#define DC_LPE_REG_28_TEST_IDLE_JMP_SHIFT                       2
#define DC_LPE_REG_28_TEST_IDLE_JMP_MASK                        0x1ull
#define DC_LPE_REG_28_TEST_IDLE_JMP_SMASK                       0x4ull
#define DC_LPE_REG_28_POLARITY_SWAP_SUPPORTED_SHIFT             1
#define DC_LPE_REG_28_POLARITY_SWAP_SUPPORTED_MASK              0x1ull
#define DC_LPE_REG_28_POLARITY_SWAP_SUPPORTED_SMASK             0x2ull
#define DC_LPE_REG_28_TX_LANE_REVERSE_SUPPORTED_SHIFT           0
#define DC_LPE_REG_28_TX_LANE_REVERSE_SUPPORTED_MASK            0x1ull
#define DC_LPE_REG_28_TX_LANE_REVERSE_SUPPORTED_SMASK           0x1ull
/*
* Table #40 of 240_CSR_pcslpe.xml - lpe_reg_29
* See description below for individual field definition
*/
#define DC_LPE_REG_29                                           (DC_PCSLPE_LPE + 0x000000000148)
#define DC_LPE_REG_29_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_29_VL_FC_TIMEOUT_SHIFT                       24
#define DC_LPE_REG_29_VL_FC_TIMEOUT_MASK                        0xFFull
#define DC_LPE_REG_29_VL_FC_TIMEOUT_SMASK                       0xFF000000ull
#define DC_LPE_REG_29_UNUSED_23_21_SHIFT                        21
#define DC_LPE_REG_29_UNUSED_23_21_MASK                         0x7ull
#define DC_LPE_REG_29_UNUSED_23_21_SMASK                        0xE00000ull
#define DC_LPE_REG_29_TX_REV_LANES_DETECTED_SHIFT               20
#define DC_LPE_REG_29_TX_REV_LANES_DETECTED_MASK                0x1ull
#define DC_LPE_REG_29_TX_REV_LANES_DETECTED_SMASK               0x100000ull
#define DC_LPE_REG_29_LPE_RX_LATE_LOST_LINK_SHIFT               19
#define DC_LPE_REG_29_LPE_RX_LATE_LOST_LINK_MASK                0x1ull
#define DC_LPE_REG_29_LPE_RX_LATE_LOST_LINK_SMASK               0x80000ull
#define DC_LPE_REG_29_LPE_RX_LATE_CTRL_ERR_SHIFT                18
#define DC_LPE_REG_29_LPE_RX_LATE_CTRL_ERR_MASK                 0x1ull
#define DC_LPE_REG_29_LPE_RX_LATE_CTRL_ERR_SMASK                0x40000ull
#define DC_LPE_REG_29_UNUSED_17_SHIFT                           17
#define DC_LPE_REG_29_UNUSED_17_MASK                            0x1ull
#define DC_LPE_REG_29_UNUSED_17_SMASK                           0x20000ull
#define DC_LPE_REG_29_LPE_RX_LATE_CODE_ERR_SHIFT                16
#define DC_LPE_REG_29_LPE_RX_LATE_CODE_ERR_MASK                 0x1ull
#define DC_LPE_REG_29_LPE_RX_LATE_CODE_ERR_SMASK                0x10000ull
#define DC_LPE_REG_29_RX_ABR_ERROR_SHIFT                        8
#define DC_LPE_REG_29_RX_ABR_ERROR_MASK                         0xFFull
#define DC_LPE_REG_29_RX_ABR_ERROR_SMASK                        0xFF00ull
#define DC_LPE_REG_29_IB_STAT_FC_VIOLATION_SHIFT                0
#define DC_LPE_REG_29_IB_STAT_FC_VIOLATION_MASK                 0xFFull
#define DC_LPE_REG_29_IB_STAT_FC_VIOLATION_SMASK                0xFFull
/*
* Table #41 of 240_CSR_pcslpe.xml - lpe_reg_2A
* See description below for individual field definition
*/
#define DC_LPE_REG_2A                                           (DC_PCSLPE_LPE + 0x000000000150)
#define DC_LPE_REG_2A_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_2A_LPE_RX_MAJOR_ERROR_DISABLE_MASK_SHIFT     24
#define DC_LPE_REG_2A_LPE_RX_MAJOR_ERROR_DISABLE_MASK_MASK      0xFFull
#define DC_LPE_REG_2A_LPE_RX_MAJOR_ERROR_DISABLE_MASK_SMASK     0xFF000000ull
#define DC_LPE_REG_2A_LPE_RX_EARLY_ERROR_ENABLE_MASK_ADV_SHIFT  17
#define DC_LPE_REG_2A_LPE_RX_EARLY_ERROR_ENABLE_MASK_ADV_MASK   0x1ull
#define DC_LPE_REG_2A_LPE_RX_EARLY_ERROR_ENABLE_MASK_ADV_SMASK  0x20000ull
#define DC_LPE_REG_2A_LPE_RX_LATE_ERROR_ENABLE_MASK_SHIFT       8
#define DC_LPE_REG_2A_LPE_RX_LATE_ERROR_ENABLE_MASK_MASK        0x1FFull
#define DC_LPE_REG_2A_LPE_RX_LATE_ERROR_ENABLE_MASK_SMASK       0x1FF00ull
#define DC_LPE_REG_2A_LPE_RX_EARLY_ERROR_ENABLE_MASK_SHIFT      0
#define DC_LPE_REG_2A_LPE_RX_EARLY_ERROR_ENABLE_MASK_MASK       0xFFull
#define DC_LPE_REG_2A_LPE_RX_EARLY_ERROR_ENABLE_MASK_SMASK      0xFFull
/*
* Table #42 of 240_CSR_pcslpe.xml - lpe_reg_2B
* See description below for individual field definition
*/
#define DC_LPE_REG_2B                                           (DC_PCSLPE_LPE + 0x000000000158)
#define DC_LPE_REG_2B_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_2B_UNUSED_31_8_SHIFT                         8
#define DC_LPE_REG_2B_UNUSED_31_8_MASK                          0xFFFFFFull
#define DC_LPE_REG_2B_UNUSED_31_8_SMASK                         0xFFFFFF00ull
#define DC_LPE_REG_2B_LPE_RX_EARLY_ERROR_REPORT_MASK_SHIFT      0
#define DC_LPE_REG_2B_LPE_RX_EARLY_ERROR_REPORT_MASK_MASK       0xFFull
#define DC_LPE_REG_2B_LPE_RX_EARLY_ERROR_REPORT_MASK_SMASK      0xFFull
/*
* Table #43 of 240_CSR_pcslpe.xml - lpe_reg_2C
* See description below for individual field definition
*/
#define DC_LPE_REG_2C                                           (DC_PCSLPE_LPE + 0x000000000160)
#define DC_LPE_REG_2C_RESETCSR                                  0x00000101ull
#define DC_LPE_REG_2C_UNUSED_31_10_SHIFT                        10
#define DC_LPE_REG_2C_UNUSED_31_10_MASK                         0x3FFFFFull
#define DC_LPE_REG_2C_UNUSED_31_10_SMASK                        0xFFFFFC00ull
#define DC_LPE_REG_2C_IB_ENHANCE_MODE10_SHIFT                   8
#define DC_LPE_REG_2C_IB_ENHANCE_MODE10_MASK                    0x3ull
#define DC_LPE_REG_2C_IB_ENHANCE_MODE10_SMASK                   0x300ull
#define DC_LPE_REG_2C_UNUSED_7_2_SHIFT                          2
#define DC_LPE_REG_2C_UNUSED_7_2_MASK                           0x3Full
#define DC_LPE_REG_2C_UNUSED_7_2_SMASK                          0xFCull
#define DC_LPE_REG_2C_LPE_PKT_LENGTH_B11_DISABLE_SHIFT          1
#define DC_LPE_REG_2C_LPE_PKT_LENGTH_B11_DISABLE_MASK           0x1ull
#define DC_LPE_REG_2C_LPE_PKT_LENGTH_B11_DISABLE_SMASK          0x2ull
#define DC_LPE_REG_2C_IB_FORWARD_IN_ARM_SHIFT                   0
#define DC_LPE_REG_2C_IB_FORWARD_IN_ARM_MASK                    0x1ull
#define DC_LPE_REG_2C_IB_FORWARD_IN_ARM_SMASK                   0x1ull
/*
* Table #44 of 240_CSR_pcslpe.xml - lpe_reg_2D
* See description below for individual field definition
*/
#define DC_LPE_REG_2D                                           (DC_PCSLPE_LPE + 0x000000000168)
#define DC_LPE_REG_2D_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_2D_UNUSED_31_16_SHIFT                        16
#define DC_LPE_REG_2D_UNUSED_31_16_MASK                         0xFFFFull
#define DC_LPE_REG_2D_UNUSED_31_16_SMASK                        0xFFFF0000ull
#define DC_LPE_REG_2D_LIN_FDB_TOP_SHIFT                         0
#define DC_LPE_REG_2D_LIN_FDB_TOP_MASK                          0xFFFFull
#define DC_LPE_REG_2D_LIN_FDB_TOP_SMASK                         0xFFFFull
/*
* Table #45 of 240_CSR_pcslpe.xml - lpe_reg_2E
* See description below for individual field definition
*/
#define DC_LPE_REG_2E                                           (DC_PCSLPE_LPE + 0x000000000170)
#define DC_LPE_REG_2E_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_2E_UNUSED_31_6_SHIFT                         6
#define DC_LPE_REG_2E_UNUSED_31_6_MASK                          0x3FFFFFFull
#define DC_LPE_REG_2E_UNUSED_31_6_SMASK                         0xFFFFFFC0ull
#define DC_LPE_REG_2E_CFG_TEST_ACTIVE_SHIFT                     5
#define DC_LPE_REG_2E_CFG_TEST_ACTIVE_MASK                      0x1ull
#define DC_LPE_REG_2E_CFG_TEST_ACTIVE_SMASK                     0x20ull
#define DC_LPE_REG_2E_CFG_TEST_FRONT_PORCH_ACTIVE_SHIFT         4
#define DC_LPE_REG_2E_CFG_TEST_FRONT_PORCH_ACTIVE_MASK          0x1ull
#define DC_LPE_REG_2E_CFG_TEST_FRONT_PORCH_ACTIVE_SMASK         0x10ull
#define DC_LPE_REG_2E_CFG_TEST_BACK_PORCH_ACTIVE_SHIFT          3
#define DC_LPE_REG_2E_CFG_TEST_BACK_PORCH_ACTIVE_MASK           0x1ull
#define DC_LPE_REG_2E_CFG_TEST_BACK_PORCH_ACTIVE_SMASK          0x8ull
#define DC_LPE_REG_2E_LPE_FRONT_PORCH_INTERVAL_SHIFT            0
#define DC_LPE_REG_2E_LPE_FRONT_PORCH_INTERVAL_MASK             0x7ull
#define DC_LPE_REG_2E_LPE_FRONT_PORCH_INTERVAL_SMASK            0x7ull
/*
* Table #46 of 240_CSR_pcslpe.xml - lpe_reg_2F
* See description below for individual field definition
*/
#define DC_LPE_REG_2F                                           (DC_PCSLPE_LPE + 0x000000000178)
#define DC_LPE_REG_2F_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_2F_UNUSED_31_5_SHIFT                         5
#define DC_LPE_REG_2F_UNUSED_31_5_MASK                          0x7FFFFFFull
#define DC_LPE_REG_2F_UNUSED_31_5_SMASK                         0xFFFFFFE0ull
#define DC_LPE_REG_2F_SPEED_DISABLE_MASK_SHIFT                  0
#define DC_LPE_REG_2F_SPEED_DISABLE_MASK_MASK                   0x1Full
#define DC_LPE_REG_2F_SPEED_DISABLE_MASK_SMASK                  0x1Full
/*
* Table #47 of 240_CSR_pcslpe.xml - lpe_reg_30
* See description below for individual field definition
*/
#define DC_LPE_REG_30                                           (DC_PCSLPE_LPE + 0x000000000180)
#define DC_LPE_REG_30_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_30_UNUSED_31_11_SHIFT                        11
#define DC_LPE_REG_30_UNUSED_31_11_MASK                         0x1FFFFFull
#define DC_LPE_REG_30_UNUSED_31_11_SMASK                        0xFFFFF800ull
#define DC_LPE_REG_30_CFG_TEST_FAILED_SHIFT                     10
#define DC_LPE_REG_30_CFG_TEST_FAILED_MASK                      0x1ull
#define DC_LPE_REG_30_CFG_TEST_FAILED_SMASK                     0x400ull
#define DC_LPE_REG_30_EN_CFG_TEST_FAILED_SHIFT                  9
#define DC_LPE_REG_30_EN_CFG_TEST_FAILED_MASK                   0x1ull
#define DC_LPE_REG_30_EN_CFG_TEST_FAILED_SMASK                  0x200ull
#define DC_LPE_REG_30_FORCE_SPEED_DOWN_GRADE_SHIFT              8
#define DC_LPE_REG_30_FORCE_SPEED_DOWN_GRADE_MASK               0x1ull
#define DC_LPE_REG_30_FORCE_SPEED_DOWN_GRADE_SMASK              0x100ull
#define DC_LPE_REG_30_UNUSED_7_5_SHIFT                          5
#define DC_LPE_REG_30_UNUSED_7_5_MASK                           0x7ull
#define DC_LPE_REG_30_UNUSED_7_5_SMASK                          0xE0ull
#define DC_LPE_REG_30_TS1_LOCK_FOREVER_SHIFT                    4
#define DC_LPE_REG_30_TS1_LOCK_FOREVER_MASK                     0x1ull
#define DC_LPE_REG_30_TS1_LOCK_FOREVER_SMASK                    0x10ull
#define DC_LPE_REG_30_DEBOUNCE_SEND_IDLE_ENABLE_SHIFT           3
#define DC_LPE_REG_30_DEBOUNCE_SEND_IDLE_ENABLE_MASK            0x1ull
#define DC_LPE_REG_30_DEBOUNCE_SEND_IDLE_ENABLE_SMASK           0x8ull
#define DC_LPE_REG_30_ENBL_LINKUP_TO_DISABLE_SHIFT              2
#define DC_LPE_REG_30_ENBL_LINKUP_TO_DISABLE_MASK               0x1ull
#define DC_LPE_REG_30_ENBL_LINKUP_TO_DISABLE_SMASK              0x4ull
#define DC_LPE_REG_30_DSBL_TS3_TRAINING_SHIFT                   1
#define DC_LPE_REG_30_DSBL_TS3_TRAINING_MASK                    0x1ull
#define DC_LPE_REG_30_DSBL_TS3_TRAINING_SMASK                   0x2ull
#define DC_LPE_REG_30_DSBL_LINK_RECOVERY_SHIFT                  0
#define DC_LPE_REG_30_DSBL_LINK_RECOVERY_MASK                   0x1ull
#define DC_LPE_REG_30_DSBL_LINK_RECOVERY_SMASK                  0x1ull
/*
* Table #48 of 240_CSR_pcslpe.xml - lpe_reg_31
* See description below for individual field definition
*/
#define DC_LPE_REG_31                                           (DC_PCSLPE_LPE + 0x000000000188)
#define DC_LPE_REG_31_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_31_UNUSED_31_0_SHIFT                         0
#define DC_LPE_REG_31_UNUSED_31_0_MASK                          0xFFFFFFFFull
#define DC_LPE_REG_31_UNUSED_31_0_SMASK                         0xFFFFFFFFull
/*
* Table #49 of 240_CSR_pcslpe.xml - lpe_reg_32
* See description below for individual field definition
*/
#define DC_LPE_REG_32                                           (DC_PCSLPE_LPE + 0x000000000190)
#define DC_LPE_REG_32_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_32_UNUSED_31_2_SHIFT                         2
#define DC_LPE_REG_32_UNUSED_31_2_MASK                          0x3FFFFFFFull
#define DC_LPE_REG_32_UNUSED_31_2_SMASK                         0xFFFFFFFCull
#define DC_LPE_REG_32_RX_FORCE_MAJOR_ERROR_SHIFT                0
#define DC_LPE_REG_32_RX_FORCE_MAJOR_ERROR_MASK                 0x3ull
#define DC_LPE_REG_32_RX_FORCE_MAJOR_ERROR_SMASK                0x3ull
/*
* Table #50 of 240_CSR_pcslpe.xml - lpe_reg_33
* See description below for individual field definition
*/
#define DC_LPE_REG_33                                           (DC_PCSLPE_LPE + 0x000000000198)
#define DC_LPE_REG_33_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_33_UNUSED_31_9_SHIFT                         9
#define DC_LPE_REG_33_UNUSED_31_9_MASK                          0x7FFFFFull
#define DC_LPE_REG_33_UNUSED_31_9_SMASK                         0xFFFFFE00ull
#define DC_LPE_REG_33_VL_PKT_DISCARD_MASK_SHIFT                 0
#define DC_LPE_REG_33_VL_PKT_DISCARD_MASK_MASK                  0x1FFull
#define DC_LPE_REG_33_VL_PKT_DISCARD_MASK_SMASK                 0x1FFull
/*
* Table #51 of 240_CSR_pcslpe.xml - lpe_reg_36
* See description below for individual field definition
*/
#define DC_LPE_REG_36                                           (DC_PCSLPE_LPE + 0x0000000001B0)
#define DC_LPE_REG_36_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_36_UNUSED_31_0_SHIFT                         0
#define DC_LPE_REG_36_UNUSED_31_0_MASK                          0xFFFFFFFFull
#define DC_LPE_REG_36_UNUSED_31_0_SMASK                         0xFFFFFFFFull
/*
* Table #52 of 240_CSR_pcslpe.xml - lpe_reg_37
* See description below for individual field definition
*/
#define DC_LPE_REG_37                                           (DC_PCSLPE_LPE + 0x0000000001B8)
#define DC_LPE_REG_37_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_37_UNUSED_31_27_SHIFT                        27
#define DC_LPE_REG_37_UNUSED_31_27_MASK                         0x1Full
#define DC_LPE_REG_37_UNUSED_31_27_SMASK                        0xF8000000ull
#define DC_LPE_REG_37_FREEZE_LAST_SHIFT                         26
#define DC_LPE_REG_37_FREEZE_LAST_MASK                          0x1ull
#define DC_LPE_REG_37_FREEZE_LAST_SMASK                         0x4000000ull
#define DC_LPE_REG_37_CLEAR_LAST_SHIFT                          25
#define DC_LPE_REG_37_CLEAR_LAST_MASK                           0x1ull
#define DC_LPE_REG_37_CLEAR_LAST_SMASK                          0x2000000ull
#define DC_LPE_REG_37_CLEAR_FIRST_SHIFT                         24
#define DC_LPE_REG_37_CLEAR_FIRST_MASK                          0x1ull
#define DC_LPE_REG_37_CLEAR_FIRST_SMASK                         0x1000000ull
#define DC_LPE_REG_37_ERROR_STATE_SHIFT                         0
#define DC_LPE_REG_37_ERROR_STATE_MASK                          0xFFFFFFull
#define DC_LPE_REG_37_ERROR_STATE_SMASK                         0xFFFFFFull
/*
* Table #53 of 240_CSR_pcslpe.xml - lpe_reg_38
* See description below for individual field definition
*/
#define DC_LPE_REG_38                                           (DC_PCSLPE_LPE + 0x0000000001C0)
#define DC_LPE_REG_38_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_38_FIRST_ERR_LRH_0_SHIFT                     0
#define DC_LPE_REG_38_FIRST_ERR_LRH_0_MASK                      0xFFFFFFFFull
#define DC_LPE_REG_38_FIRST_ERR_LRH_0_SMASK                     0xFFFFFFFFull
/*
* Table #54 of 240_CSR_pcslpe.xml - lpe_reg_39
* See description below for individual field definition
*/
#define DC_LPE_REG_39                                           (DC_PCSLPE_LPE + 0x0000000001C8)
#define DC_LPE_REG_39_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_39_FIRST_ERR_LRH_1_SHIFT                     0
#define DC_LPE_REG_39_FIRST_ERR_LRH_1_MASK                      0xFFFFFFFFull
#define DC_LPE_REG_39_FIRST_ERR_LRH_1_SMASK                     0xFFFFFFFFull
/*
* Table #55 of 240_CSR_pcslpe.xml - lpe_reg_3A
* See description below for individual field definition
*/
#define DC_LPE_REG_3A                                           (DC_PCSLPE_LPE + 0x0000000001D0)
#define DC_LPE_REG_3A_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_3A_FIRST_ERR_BTH_0_SHIFT                     0
#define DC_LPE_REG_3A_FIRST_ERR_BTH_0_MASK                      0xFFFFFFFFull
#define DC_LPE_REG_3A_FIRST_ERR_BTH_0_SMASK                     0xFFFFFFFFull
/*
* Table #56 of 240_CSR_pcslpe.xml - lpe_reg_3B
* See description below for individual field definition
*/
#define DC_LPE_REG_3B                                           (DC_PCSLPE_LPE + 0x0000000001D8)
#define DC_LPE_REG_3B_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_3B_FIRST_ERR_BTH_1_SHIFT                     0
#define DC_LPE_REG_3B_FIRST_ERR_BTH_1_MASK                      0xFFFFFFFFull
#define DC_LPE_REG_3B_FIRST_ERR_BTH_1_SMASK                     0xFFFFFFFFull
/*
* Table #57 of 240_CSR_pcslpe.xml - lpe_reg_3C
* See description below for individual field definition
*/
#define DC_LPE_REG_3C                                           (DC_PCSLPE_LPE + 0x0000000001E0)
#define DC_LPE_REG_3C_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_3C_LAST_ERR_LRH_0_SHIFT                      0
#define DC_LPE_REG_3C_LAST_ERR_LRH_0_MASK                       0xFFFFFFFFull
#define DC_LPE_REG_3C_LAST_ERR_LRH_0_SMASK                      0xFFFFFFFFull
/*
* Table #58 of 240_CSR_pcslpe.xml - lpe_reg_3D
* See description below for individual field definition
*/
#define DC_LPE_REG_3D                                           (DC_PCSLPE_LPE + 0x0000000001E8)
#define DC_LPE_REG_3D_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_3D_LAST_ERR_LRH_1_SHIFT                      0
#define DC_LPE_REG_3D_LAST_ERR_LRH_1_MASK                       0xFFFFFFFFull
#define DC_LPE_REG_3D_LAST_ERR_LRH_1_SMASK                      0xFFFFFFFFull
/*
* Table #59 of 240_CSR_pcslpe.xml - lpe_reg_3E
* See description below for individual field definition
*/
#define DC_LPE_REG_3E                                           (DC_PCSLPE_LPE + 0x0000000001F0)
#define DC_LPE_REG_3E_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_3E_LAST_ERR_BTH_0_SHIFT                      0
#define DC_LPE_REG_3E_LAST_ERR_BTH_0_MASK                       0xFFFFFFFFull
#define DC_LPE_REG_3E_LAST_ERR_BTH_0_SMASK                      0xFFFFFFFFull
/*
* Table #60 of 240_CSR_pcslpe.xml - lpe_reg_3F
* See description below for individual field definition
*/
#define DC_LPE_REG_3F                                           (DC_PCSLPE_LPE + 0x0000000001F8)
#define DC_LPE_REG_3F_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_3F_LAST_ERR_BTH_1_SHIFT                      0
#define DC_LPE_REG_3F_LAST_ERR_BTH_1_MASK                       0xFFFFFFFFull
#define DC_LPE_REG_3F_LAST_ERR_BTH_1_SMASK                      0xFFFFFFFFull
/*
* Table #61 of 240_CSR_pcslpe.xml - lpe_reg_40
* See description below for individual field definition
*/
#define DC_LPE_REG_40                                           (DC_PCSLPE_LPE + 0x000000000200)
#define DC_LPE_REG_40_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_40_UNUSED_31_10_SHIFT                        10
#define DC_LPE_REG_40_UNUSED_31_10_MASK                         0x3FFFFFull
#define DC_LPE_REG_40_UNUSED_31_10_SMASK                        0xFFFFFC00ull
#define DC_LPE_REG_40_TRACE_BUFFER_FROZEN_SHIFT                 9
#define DC_LPE_REG_40_TRACE_BUFFER_FROZEN_MASK                  0x1ull
#define DC_LPE_REG_40_TRACE_BUFFER_FROZEN_SMASK                 0x200ull
#define DC_LPE_REG_40_SM_MATCH_INT_SHIFT                        8
#define DC_LPE_REG_40_SM_MATCH_INT_MASK                         0x1ull
#define DC_LPE_REG_40_SM_MATCH_INT_SMASK                        0x100ull
#define DC_LPE_REG_40_PORT_SM_MATCH_REG_SHIFT                   5
#define DC_LPE_REG_40_PORT_SM_MATCH_REG_MASK                    0x7ull
#define DC_LPE_REG_40_PORT_SM_MATCH_REG_SMASK                   0xE0ull
#define DC_LPE_REG_40_TRAINING_SM_MATCH_REG_SHIFT               0
#define DC_LPE_REG_40_TRAINING_SM_MATCH_REG_MASK                0x1Full
#define DC_LPE_REG_40_TRAINING_SM_MATCH_REG_SMASK               0x1Full
/*
* Table #62 of 240_CSR_pcslpe.xml - lpe_reg_41
* See description below for individual field definition
*/
#define DC_LPE_REG_41                                           (DC_PCSLPE_LPE + 0x000000000208)
#define DC_LPE_REG_41_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_41_UNUSED_31_13_SHIFT                        13
#define DC_LPE_REG_41_UNUSED_31_13_MASK                         0x7FFFFull
#define DC_LPE_REG_41_UNUSED_31_13_SMASK                        0xFFFFE000ull
#define DC_LPE_REG_41_TRACE_BUFFER_WRAP_EN_SHIFT                12
#define DC_LPE_REG_41_TRACE_BUFFER_WRAP_EN_MASK                 0x1ull
#define DC_LPE_REG_41_TRACE_BUFFER_WRAP_EN_SMASK                0x1000ull
#define DC_LPE_REG_41_UNUSED_11_10_SHIFT                        10
#define DC_LPE_REG_41_UNUSED_11_10_MASK                         0x3ull
#define DC_LPE_REG_41_UNUSED_11_10_SMASK                        0xC00ull
#define DC_LPE_REG_41_TRACE_BUFFER_LD_DIS_MASK_SHIFT            4
#define DC_LPE_REG_41_TRACE_BUFFER_LD_DIS_MASK_MASK             0x3Full
#define DC_LPE_REG_41_TRACE_BUFFER_LD_DIS_MASK_SMASK            0x3F0ull
#define DC_LPE_REG_41_RESET_TRACE_BUFFER_SHIFT                  3
#define DC_LPE_REG_41_RESET_TRACE_BUFFER_MASK                   0x1ull
#define DC_LPE_REG_41_RESET_TRACE_BUFFER_SMASK                  0x8ull
#define DC_LPE_REG_41_INT_ON_MATCH_SHIFT                        2
#define DC_LPE_REG_41_INT_ON_MATCH_MASK                         0x1ull
#define DC_LPE_REG_41_INT_ON_MATCH_SMASK                        0x4ull
#define DC_LPE_REG_41_FREEZE_ON_MATCH_SHIFT                     1
#define DC_LPE_REG_41_FREEZE_ON_MATCH_MASK                      0x1ull
#define DC_LPE_REG_41_FREEZE_ON_MATCH_SMASK                     0x2ull
#define DC_LPE_REG_41_FREEZE_TRACE_BUFFER_SHIFT                 0
#define DC_LPE_REG_41_FREEZE_TRACE_BUFFER_MASK                  0x1ull
#define DC_LPE_REG_41_FREEZE_TRACE_BUFFER_SMASK                 0x1ull
/*
* Table #63 of 240_CSR_pcslpe.xml - lpe_reg_42
* See description below for individual field definition
*/
#define DC_LPE_REG_42                                           (DC_PCSLPE_LPE + 0x000000000210)
#define DC_LPE_REG_42_RESETCSR                                  0x22000000ull
#define DC_LPE_REG_42_TB_PORT_STATE_SHIFT                       29
#define DC_LPE_REG_42_TB_PORT_STATE_MASK                        0x7ull
#define DC_LPE_REG_42_TB_PORT_STATE_SMASK                       0xE0000000ull
#define DC_LPE_REG_42_TB_TRAINING_STATE_SHIFT                   24
#define DC_LPE_REG_42_TB_TRAINING_STATE_MASK                    0x1Full
#define DC_LPE_REG_42_TB_TRAINING_STATE_SMASK                   0x1F000000ull
#define DC_LPE_REG_42_UNUSED_23_16_SHIFT                        16
#define DC_LPE_REG_42_UNUSED_23_16_MASK                         0xFFull
#define DC_LPE_REG_42_UNUSED_23_16_SMASK                        0xFF0000ull
#define DC_LPE_REG_42_SPARE_RA_SHIFT                            13
#define DC_LPE_REG_42_SPARE_RA_MASK                             0x7ull
#define DC_LPE_REG_42_SPARE_RA_SMASK                            0xE000ull
#define DC_LPE_REG_42_LINK_UP_GOT_TS_SHIFT                      12
#define DC_LPE_REG_42_LINK_UP_GOT_TS_MASK                       0x1ull
#define DC_LPE_REG_42_LINK_UP_GOT_TS_SMASK                      0x1000ull
#define DC_LPE_REG_42_TS3_OS_DETECTED_SHIFT                     8
#define DC_LPE_REG_42_TS3_OS_DETECTED_MASK                      0xFull
#define DC_LPE_REG_42_TS3_OS_DETECTED_SMASK                     0xF00ull
#define DC_LPE_REG_42_TS2_OS_DETECTED_SHIFT                     4
#define DC_LPE_REG_42_TS2_OS_DETECTED_MASK                      0xFull
#define DC_LPE_REG_42_TS2_OS_DETECTED_SMASK                     0xF0ull
#define DC_LPE_REG_42_TS1_OS_DETECTED_SHIFT                     0
#define DC_LPE_REG_42_TS1_OS_DETECTED_MASK                      0xFull
#define DC_LPE_REG_42_TS1_OS_DETECTED_SMASK                     0xFull
/*
* Table #64 of 240_CSR_pcslpe.xml - lpe_reg_43
* See description below for individual field definition
*/
#define DC_LPE_REG_43                                           (DC_PCSLPE_LPE + 0x000000000218)
#define DC_LPE_REG_43_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_43_TB_LINK_MAJOR_ERRORS_SHIFT                24
#define DC_LPE_REG_43_TB_LINK_MAJOR_ERRORS_MASK                 0xFFull
#define DC_LPE_REG_43_TB_LINK_MAJOR_ERRORS_SMASK                0xFF000000ull
#define DC_LPE_REG_43_TB_FC_ERRORS_SHIFT                        21
#define DC_LPE_REG_43_TB_FC_ERRORS_MASK                         0x7ull
#define DC_LPE_REG_43_TB_FC_ERRORS_SMASK                        0xE00000ull
#define DC_LPE_REG_43_TB_PKT_LATE_ERRORS_SHIFT                  12
#define DC_LPE_REG_43_TB_PKT_LATE_ERRORS_MASK                   0x1FFull
#define DC_LPE_REG_43_TB_PKT_LATE_ERRORS_SMASK                  0x1FF000ull
#define DC_LPE_REG_43_TB_PKT_EARLY_ERRORS_SHIFT                 0
#define DC_LPE_REG_43_TB_PKT_EARLY_ERRORS_MASK                  0xFFFull
#define DC_LPE_REG_43_TB_PKT_EARLY_ERRORS_SMASK                 0xFFFull
/*
* Table #65 of 240_CSR_pcslpe.xml - lpe_reg_44
* See description below for individual field definition
*/
#define DC_LPE_REG_44                                           (DC_PCSLPE_LPE + 0x000000000220)
#define DC_LPE_REG_44_RESETCSR                                  0x00000006ull
#define DC_LPE_REG_44_TB_TIME_STAMP_SHIFT                       0
#define DC_LPE_REG_44_TB_TIME_STAMP_MASK                        0xFFFFFFFFull
#define DC_LPE_REG_44_TB_TIME_STAMP_SMASK                       0xFFFFFFFFull
/*
* Table #66 of 240_CSR_pcslpe.xml - lpe_reg_48
* See description below for individual field definition
*/
#define DC_LPE_REG_48                                           (DC_PCSLPE_LPE + 0x000000000240)
#define DC_LPE_REG_48_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_48_UNUSED_31_7_SHIFT                         7
#define DC_LPE_REG_48_UNUSED_31_7_MASK                          0x1FFFFFFull
#define DC_LPE_REG_48_UNUSED_31_7_SMASK                         0xFFFFFF80ull
#define DC_LPE_REG_48_ENABLE_SAMPLE_PERIOD_SHIFT                6
#define DC_LPE_REG_48_ENABLE_SAMPLE_PERIOD_MASK                 0x1ull
#define DC_LPE_REG_48_ENABLE_SAMPLE_PERIOD_SMASK                0x40ull
#define DC_LPE_REG_48_FREEZE_IDLE_CNT_SHIFT                     5
#define DC_LPE_REG_48_FREEZE_IDLE_CNT_MASK                      0x1ull
#define DC_LPE_REG_48_FREEZE_IDLE_CNT_SMASK                     0x20ull
#define DC_LPE_REG_48_CLEAR_IDLE_CNT_SHIFT                      4
#define DC_LPE_REG_48_CLEAR_IDLE_CNT_MASK                       0x1ull
#define DC_LPE_REG_48_CLEAR_IDLE_CNT_SMASK                      0x10ull
#define DC_LPE_REG_48_IDLE_SAMPLE_PERIOD_SHIFT                  0
#define DC_LPE_REG_48_IDLE_SAMPLE_PERIOD_MASK                   0xFull
#define DC_LPE_REG_48_IDLE_SAMPLE_PERIOD_SMASK                  0xFull
/*
* Table #67 of 240_CSR_pcslpe.xml - lpe_reg_49
* See description below for individual field definition
*/
#define DC_LPE_REG_49                                           (DC_PCSLPE_LPE + 0x000000000248)
#define DC_LPE_REG_49_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_49_RX_IDLE_CNT_UPPER_SHIFT                   0
#define DC_LPE_REG_49_RX_IDLE_CNT_UPPER_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_49_RX_IDLE_CNT_UPPER_SMASK                   0xFFFFFFFFull
/*
* Table #68 of 240_CSR_pcslpe.xml - lpe_reg_4A
* See description below for individual field definition
*/
#define DC_LPE_REG_4A                                           (DC_PCSLPE_LPE + 0x000000000250)
#define DC_LPE_REG_4A_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_4A_RX_IDLE_CNT_LOWER_SHIFT                   0
#define DC_LPE_REG_4A_RX_IDLE_CNT_LOWER_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_4A_RX_IDLE_CNT_LOWER_SMASK                   0xFFFFFFFFull
/*
* Table #69 of 240_CSR_pcslpe.xml - lpe_reg_4B
* See description below for individual field definition
*/
#define DC_LPE_REG_4B                                           (DC_PCSLPE_LPE + 0x000000000258)
#define DC_LPE_REG_4B_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_4B_TX_IDLE_CNT_UPPER_SHIFT                   0
#define DC_LPE_REG_4B_TX_IDLE_CNT_UPPER_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_4B_TX_IDLE_CNT_UPPER_SMASK                   0xFFFFFFFFull
/*
* Table #70 of 240_CSR_pcslpe.xml - lpe_reg_4C
* See description below for individual field definition
*/
#define DC_LPE_REG_4C                                           (DC_PCSLPE_LPE + 0x000000000260)
#define DC_LPE_REG_4C_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_4C_TX_IDLE_CNT_LOWER_SHIFT                   0
#define DC_LPE_REG_4C_TX_IDLE_CNT_LOWER_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_4C_TX_IDLE_CNT_LOWER_SMASK                   0xFFFFFFFFull
/*
* Table #71 of 240_CSR_pcslpe.xml - lpe_reg_50
* See description below for individual field definition
*/
#define DC_LPE_REG_50                                           (DC_PCSLPE_LPE + 0x000000000280)
#define DC_LPE_REG_50_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_50_UNUSED_31_SHIFT                           31
#define DC_LPE_REG_50_UNUSED_31_MASK                            0x1ull
#define DC_LPE_REG_50_UNUSED_31_SMASK                           0x80000000ull
#define DC_LPE_REG_50_REG_0X40_INT_SHIFT                        30
#define DC_LPE_REG_50_REG_0X40_INT_MASK                         0x1ull
#define DC_LPE_REG_50_REG_0X40_INT_SMASK                        0x40000000ull
#define DC_LPE_REG_50_REG_0X30_INT_SHIFT                        29
#define DC_LPE_REG_50_REG_0X30_INT_MASK                         0x1ull
#define DC_LPE_REG_50_REG_0X30_INT_SMASK                        0x20000000ull
#define DC_LPE_REG_50_REG_0X2E_INT_SHIFT                        27
#define DC_LPE_REG_50_REG_0X2E_INT_MASK                         0x3ull
#define DC_LPE_REG_50_REG_0X2E_INT_SMASK                        0x18000000ull
#define DC_LPE_REG_50_REG_0X28_INT_SHIFT                        25
#define DC_LPE_REG_50_REG_0X28_INT_MASK                         0x3ull
#define DC_LPE_REG_50_REG_0X28_INT_SMASK                        0x6000000ull
#define DC_LPE_REG_50_REG_0X19_INT_SHIFT                        24
#define DC_LPE_REG_50_REG_0X19_INT_MASK                         0x1ull
#define DC_LPE_REG_50_REG_0X19_INT_SMASK                        0x1000000ull
#define DC_LPE_REG_50_REG_0X14_INT_SHIFT                        20
#define DC_LPE_REG_50_REG_0X14_INT_MASK                         0xFull
#define DC_LPE_REG_50_REG_0X14_INT_SMASK                        0xF00000ull
#define DC_LPE_REG_50_UNUSED_19_17_SHIFT                        17
#define DC_LPE_REG_50_UNUSED_19_17_MASK                         0x7ull
#define DC_LPE_REG_50_UNUSED_19_17_SMASK                        0xE0000ull
#define DC_LPE_REG_50_REG_0X0F_INT_SHIFT                        12
#define DC_LPE_REG_50_REG_0X0F_INT_MASK                         0x1Full
#define DC_LPE_REG_50_REG_0X0F_INT_SMASK                        0x1F000ull
#define DC_LPE_REG_50_UNUSED_11_10_SHIFT                        10
#define DC_LPE_REG_50_UNUSED_11_10_MASK                         0x3ull
#define DC_LPE_REG_50_UNUSED_11_10_SMASK                        0xC00ull
#define DC_LPE_REG_50_REG_0X0C_INT_SHIFT                        9
#define DC_LPE_REG_50_REG_0X0C_INT_MASK                         0x1ull
#define DC_LPE_REG_50_REG_0X0C_INT_SMASK                        0x200ull
#define DC_LPE_REG_50_REG_0X0A_INT_SHIFT                        8
#define DC_LPE_REG_50_REG_0X0A_INT_MASK                         0x1ull
#define DC_LPE_REG_50_REG_0X0A_INT_SMASK                        0x100ull
#define DC_LPE_REG_50_REG_0X01_INT_SHIFT                        0
#define DC_LPE_REG_50_REG_0X01_INT_MASK                         0xFFull
#define DC_LPE_REG_50_REG_0X01_INT_SMASK                        0xFFull
/*
* Table #72 of 240_CSR_pcslpe.xml - lpe_reg_51
* See description below for individual field definition
*/
#define DC_LPE_REG_51                                           (DC_PCSLPE_LPE + 0x000000000288)
#define DC_LPE_REG_51_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_51_TS_T_OS_DETECTED_INT_SHIFT                28
#define DC_LPE_REG_51_TS_T_OS_DETECTED_INT_MASK                 0xFull
#define DC_LPE_REG_51_TS_T_OS_DETECTED_INT_SMASK                0xF0000000ull
#define DC_LPE_REG_51_TS12_OS_ERR_DETECTED_INT_SHIFT            24
#define DC_LPE_REG_51_TS12_OS_ERR_DETECTED_INT_MASK             0xFull
#define DC_LPE_REG_51_TS12_OS_ERR_DETECTED_INT_SMASK            0xF000000ull
#define DC_LPE_REG_51_TS2_SEQ_DETECTED_INT_SHIFT                20
#define DC_LPE_REG_51_TS2_SEQ_DETECTED_INT_MASK                 0xFull
#define DC_LPE_REG_51_TS2_SEQ_DETECTED_INT_SMASK                0xF00000ull
#define DC_LPE_REG_51_TS1_SEQ_DETECTED_INT_SHIFT                16
#define DC_LPE_REG_51_TS1_SEQ_DETECTED_INT_MASK                 0xFull
#define DC_LPE_REG_51_TS1_SEQ_DETECTED_INT_SMASK                0xF0000ull
#define DC_LPE_REG_51_SKP_OS_DETECTED_INT_SHIFT                 12
#define DC_LPE_REG_51_SKP_OS_DETECTED_INT_MASK                  0xFull
#define DC_LPE_REG_51_SKP_OS_DETECTED_INT_SMASK                 0xF000ull
#define DC_LPE_REG_51_TS3_OS_DETECTED_INT_SHIFT                 8
#define DC_LPE_REG_51_TS3_OS_DETECTED_INT_MASK                  0xFull
#define DC_LPE_REG_51_TS3_OS_DETECTED_INT_SMASK                 0xF00ull
#define DC_LPE_REG_51_TS2_OS_DETECTED_INT_SHIFT                 4
#define DC_LPE_REG_51_TS2_OS_DETECTED_INT_MASK                  0xFull
#define DC_LPE_REG_51_TS2_OS_DETECTED_INT_SMASK                 0xF0ull
#define DC_LPE_REG_51_TS1_OS_DETECTED_INT_SHIFT                 0
#define DC_LPE_REG_51_TS1_OS_DETECTED_INT_MASK                  0xFull
#define DC_LPE_REG_51_TS1_OS_DETECTED_INT_SMASK                 0xFull
/*
* Table #73 of 240_CSR_pcslpe.xml - lpe_reg_52
* See description below for individual field definition
*/
#define DC_LPE_REG_52                                           (DC_PCSLPE_LPE + 0x000000000290)
#define DC_LPE_REG_52_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_52_UNUSED_31_22_SHIFT                        22
#define DC_LPE_REG_52_UNUSED_31_22_MASK                         0x3FFull
#define DC_LPE_REG_52_UNUSED_31_22_SMASK                        0xFFC00000ull
#define DC_LPE_REG_52_TX_REV_LANES_DETECTED_INT_SHIFT           20
#define DC_LPE_REG_52_TX_REV_LANES_DETECTED_INT_MASK            0x1ull
#define DC_LPE_REG_52_TX_REV_LANES_DETECTED_INT_SMASK           0x100000ull
#define DC_LPE_REG_52_LPE_RX_LATE_LOST_LINK_INT_SHIFT           19
#define DC_LPE_REG_52_LPE_RX_LATE_LOST_LINK_INT_MASK            0x1ull
#define DC_LPE_REG_52_LPE_RX_LATE_LOST_LINK_INT_SMASK           0x80000ull
#define DC_LPE_REG_52_LPE_RX_LATE_UNEXPECTED_CHAR_ERR_INT_SHIFT 18
#define DC_LPE_REG_52_LPE_RX_LATE_UNEXPECTED_CHAR_ERR_INT_MASK  0x1ull
#define DC_LPE_REG_52_LPE_RX_LATE_UNEXPECTED_CHAR_ERR_INT_SMASK 0x40000ull
#define DC_LPE_REG_52_UNUSED_17_SHIFT                           17
#define DC_LPE_REG_52_UNUSED_17_MASK                            0x1ull
#define DC_LPE_REG_52_UNUSED_17_SMASK                           0x20000ull
#define DC_LPE_REG_52_LPE_RX_LATE_CODE_ERR_INT_SHIFT            16
#define DC_LPE_REG_52_LPE_RX_LATE_CODE_ERR_INT_MASK             0x1ull
#define DC_LPE_REG_52_LPE_RX_LATE_CODE_ERR_INT_SMASK            0x10000ull
#define DC_LPE_REG_52_RX_ABR_ERROR_INT_SHIFT                    8
#define DC_LPE_REG_52_RX_ABR_ERROR_INT_MASK                     0xFFull
#define DC_LPE_REG_52_RX_ABR_ERROR_INT_SMASK                    0xFF00ull
#define DC_LPE_REG_52_IB_STAT_FC_VIOLATION_INT_SHIFT            0
#define DC_LPE_REG_52_IB_STAT_FC_VIOLATION_INT_MASK             0xFFull
#define DC_LPE_REG_52_IB_STAT_FC_VIOLATION_INT_SMASK            0xFFull
/*
* Table #74 of 240_CSR_pcslpe.xml - lpe_reg_54
* See description below for individual field definition
*/
#define DC_LPE_REG_54                                           (DC_PCSLPE_LPE + 0x0000000002A0)
#define DC_LPE_REG_54_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_54_UNUSED_31_SHIFT                           31
#define DC_LPE_REG_54_UNUSED_31_MASK                            0x1ull
#define DC_LPE_REG_54_UNUSED_31_SMASK                           0x80000000ull
#define DC_LPE_REG_54_REG_0X40_INT_MASK_SHIFT                   30
#define DC_LPE_REG_54_REG_0X40_INT_MASK_MASK                    0x1ull
#define DC_LPE_REG_54_REG_0X40_INT_MASK_SMASK                   0x40000000ull
#define DC_LPE_REG_54_REG_0X2F_INT_MASK_SHIFT                   28
#define DC_LPE_REG_54_REG_0X2F_INT_MASK_MASK                    0x3ull
#define DC_LPE_REG_54_REG_0X2F_INT_MASK_SMASK                   0x30000000ull
#define DC_LPE_REG_54_UNUSED_27_26_SHIFT                        26
#define DC_LPE_REG_54_UNUSED_27_26_MASK                         0x3ull
#define DC_LPE_REG_54_UNUSED_27_26_SMASK                        0xC000000ull
#define DC_LPE_REG_54_REG_0X28_INT_MASK_SHIFT                   25
#define DC_LPE_REG_54_REG_0X28_INT_MASK_MASK                    0x1ull
#define DC_LPE_REG_54_REG_0X28_INT_MASK_SMASK                   0x2000000ull
#define DC_LPE_REG_54_REG_0X19_INT_MASK_SHIFT                   24
#define DC_LPE_REG_54_REG_0X19_INT_MASK_MASK                    0x1ull
#define DC_LPE_REG_54_REG_0X19_INT_MASK_SMASK                   0x1000000ull
#define DC_LPE_REG_54_REG_0X14_INT_MASK_SHIFT                   20
#define DC_LPE_REG_54_REG_0X14_INT_MASK_MASK                    0xFull
#define DC_LPE_REG_54_REG_0X14_INT_MASK_SMASK                   0xF00000ull
#define DC_LPE_REG_54_UNUSED_19_17_SHIFT                        17
#define DC_LPE_REG_54_UNUSED_19_17_MASK                         0x7ull
#define DC_LPE_REG_54_UNUSED_19_17_SMASK                        0xE0000ull
#define DC_LPE_REG_54_REG_0X0F_INT_MASK_SHIFT                   12
#define DC_LPE_REG_54_REG_0X0F_INT_MASK_MASK                    0x1Full
#define DC_LPE_REG_54_REG_0X0F_INT_MASK_SMASK                   0x1F000ull
#define DC_LPE_REG_54_UNUSED_11_10_SHIFT                        10
#define DC_LPE_REG_54_UNUSED_11_10_MASK                         0x3ull
#define DC_LPE_REG_54_UNUSED_11_10_SMASK                        0xC00ull
#define DC_LPE_REG_54_REG_0X0C_INT_MASK_SHIFT                   9
#define DC_LPE_REG_54_REG_0X0C_INT_MASK_MASK                    0x1ull
#define DC_LPE_REG_54_REG_0X0C_INT_MASK_SMASK                   0x200ull
#define DC_LPE_REG_54_REG_0X0A_INT_MASK_SHIFT                   8
#define DC_LPE_REG_54_REG_0X0A_INT_MASK_MASK                    0x1ull
#define DC_LPE_REG_54_REG_0X0A_INT_MASK_SMASK                   0x100ull
#define DC_LPE_REG_54_REG_0X01_INT_MASK_SHIFT                   0
#define DC_LPE_REG_54_REG_0X01_INT_MASK_MASK                    0xFFull
#define DC_LPE_REG_54_REG_0X01_INT_MASK_SMASK                   0xFFull
/*
* Table #75 of 240_CSR_pcslpe.xml - lpe_reg_55
* See description below for individual field definition
*/
#define DC_LPE_REG_55                                           (DC_PCSLPE_LPE + 0x0000000002A8)
#define DC_LPE_REG_55_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_55_TS_T_OS_DETECTED_INT_MASK_SHIFT           28
#define DC_LPE_REG_55_TS_T_OS_DETECTED_INT_MASK_MASK            0xFull
#define DC_LPE_REG_55_TS_T_OS_DETECTED_INT_MASK_SMASK           0xF0000000ull
#define DC_LPE_REG_55_TS12_OS_ERR_DETECTED_INT_MASK_SHIFT       24
#define DC_LPE_REG_55_TS12_OS_ERR_DETECTED_INT_MASK_MASK        0xFull
#define DC_LPE_REG_55_TS12_OS_ERR_DETECTED_INT_MASK_SMASK       0xF000000ull
#define DC_LPE_REG_55_TS2_SEQ_DETECTED_INT_MASK_SHIFT           20
#define DC_LPE_REG_55_TS2_SEQ_DETECTED_INT_MASK_MASK            0xFull
#define DC_LPE_REG_55_TS2_SEQ_DETECTED_INT_MASK_SMASK           0xF00000ull
#define DC_LPE_REG_55_TS1_SEQ_DETECTED_INT_MASK_SHIFT           16
#define DC_LPE_REG_55_TS1_SEQ_DETECTED_INT_MASK_MASK            0xFull
#define DC_LPE_REG_55_TS1_SEQ_DETECTED_INT_MASK_SMASK           0xF0000ull
#define DC_LPE_REG_55_SKP_OS_DETECTED_INT_MASK_SHIFT            12
#define DC_LPE_REG_55_SKP_OS_DETECTED_INT_MASK_MASK             0xFull
#define DC_LPE_REG_55_SKP_OS_DETECTED_INT_MASK_SMASK            0xF000ull
#define DC_LPE_REG_55_TS3_OS_DETECTED_INT_MASK_SHIFT            8
#define DC_LPE_REG_55_TS3_OS_DETECTED_INT_MASK_MASK             0xFull
#define DC_LPE_REG_55_TS3_OS_DETECTED_INT_MASK_SMASK            0xF00ull
#define DC_LPE_REG_55_TS2_OS_DETECTED_INT_MASK_SHIFT            4
#define DC_LPE_REG_55_TS2_OS_DETECTED_INT_MASK_MASK             0xFull
#define DC_LPE_REG_55_TS2_OS_DETECTED_INT_MASK_SMASK            0xF0ull
#define DC_LPE_REG_55_TS1_OS_DETECTED_INT_MASK_SHIFT            0
#define DC_LPE_REG_55_TS1_OS_DETECTED_INT_MASK_MASK             0xFull
#define DC_LPE_REG_55_TS1_OS_DETECTED_INT_MASK_SMASK            0xFull
/*
* Table #76 of 240_CSR_pcslpe.xml - lpe_reg_56
* See description below for individual field definition
*/
#define DC_LPE_REG_56                                           (DC_PCSLPE_LPE + 0x0000000002B0)
#define DC_LPE_REG_56_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_56_UNUSED_31_22_SHIFT                        22
#define DC_LPE_REG_56_UNUSED_31_22_MASK                         0x3FFull
#define DC_LPE_REG_56_UNUSED_31_22_SMASK                        0xFFC00000ull
#define DC_LPE_REG_56_TX_REV_LANES_DETECTED_INT_MASK_SHIFT      20
#define DC_LPE_REG_56_TX_REV_LANES_DETECTED_INT_MASK_MASK       0x1ull
#define DC_LPE_REG_56_TX_REV_LANES_DETECTED_INT_MASK_SMASK      0x100000ull
#define DC_LPE_REG_56_LPE_RX_LATE_LOST_LINK_INT_MASK_SHIFT      19
#define DC_LPE_REG_56_LPE_RX_LATE_LOST_LINK_INT_MASK_MASK       0x1ull
#define DC_LPE_REG_56_LPE_RX_LATE_LOST_LINK_INT_MASK_SMASK      0x80000ull
#define DC_LPE_REG_56_LPE_RX_LATE_CTRL_ERR_INT_MASK_SHIFT       18
#define DC_LPE_REG_56_LPE_RX_LATE_CTRL_ERR_INT_MASK_MASK        0x1ull
#define DC_LPE_REG_56_LPE_RX_LATE_CTRL_ERR_INT_MASK_SMASK       0x40000ull
#define DC_LPE_REG_56_UNUSED_17_SHIFT                           17
#define DC_LPE_REG_56_UNUSED_17_MASK                            0x1ull
#define DC_LPE_REG_56_UNUSED_17_SMASK                           0x20000ull
#define DC_LPE_REG_56_LPE_RX_LATE_CODE_ERR_INT_MASK_SHIFT       16
#define DC_LPE_REG_56_LPE_RX_LATE_CODE_ERR_INT_MASK_MASK        0x1ull
#define DC_LPE_REG_56_LPE_RX_LATE_CODE_ERR_INT_MASK_SMASK       0x10000ull
#define DC_LPE_REG_56_RX_ABR_ERROR_INT_MASK_SHIFT               8
#define DC_LPE_REG_56_RX_ABR_ERROR_INT_MASK_MASK                0xFFull
#define DC_LPE_REG_56_RX_ABR_ERROR_INT_MASK_SMASK               0xFF00ull
#define DC_LPE_REG_56_IB_STAT_FC_VIOLATION_INT_MASK_SHIFT       0
#define DC_LPE_REG_56_IB_STAT_FC_VIOLATION_INT_MASK_MASK        0xFFull
#define DC_LPE_REG_56_IB_STAT_FC_VIOLATION_INT_MASK_SMASK       0xFFull
/*
* Table #2 of 240_CSR_pcslpe.xml - lpe_reg_0
* Register containing various mode and behavior signals. Flow Control Timer can 
* be disabled or accelerated.
*/
#define DC_LPE_REG_0                                            (DC_PCSLPE_LPE + 0x000000000000)
#define DC_LPE_REG_0_RESETCSR                                   0x11110000ull
#define DC_LPE_REG_0_VL_76_FC_PERIOD_SHIFT                      28
#define DC_LPE_REG_0_VL_76_FC_PERIOD_MASK                       0xFull
#define DC_LPE_REG_0_VL_76_FC_PERIOD_SMASK                      0xF0000000ull
#define DC_LPE_REG_0_VL_54_FC_PERIOD_SHIFT                      24
#define DC_LPE_REG_0_VL_54_FC_PERIOD_MASK                       0xFull
#define DC_LPE_REG_0_VL_54_FC_PERIOD_SMASK                      0xF000000ull
#define DC_LPE_REG_0_VL_32_FC_PERIOD_SHIFT                      20
#define DC_LPE_REG_0_VL_32_FC_PERIOD_MASK                       0xFull
#define DC_LPE_REG_0_VL_32_FC_PERIOD_SMASK                      0xF00000ull
#define DC_LPE_REG_0_VL_10_FC_PERIOD_SHIFT                      16
#define DC_LPE_REG_0_VL_10_FC_PERIOD_MASK                       0xFull
#define DC_LPE_REG_0_VL_10_FC_PERIOD_SMASK                      0xF0000ull
#define DC_LPE_REG_0_SEND_ALL_FLOW_CONTROL_SHIFT                15
#define DC_LPE_REG_0_SEND_ALL_FLOW_CONTROL_MASK                 0x1ull
#define DC_LPE_REG_0_SEND_ALL_FLOW_CONTROL_SMASK                0x8000ull
#define DC_LPE_REG_0_EXTERNAL_CCFT_SHIFT                        14
#define DC_LPE_REG_0_EXTERNAL_CCFT_MASK                         0x1ull
#define DC_LPE_REG_0_EXTERNAL_CCFT_SMASK                        0x4000ull
#define DC_LPE_REG_0_EXTERNAL_TS3_SHIFT                         13
#define DC_LPE_REG_0_EXTERNAL_TS3_MASK                          0x1ull
#define DC_LPE_REG_0_EXTERNAL_TS3_SMASK                         0x2000ull
#define DC_LPE_REG_0_HB_TIMER_ACCEL_SHIFT                       12
#define DC_LPE_REG_0_HB_TIMER_ACCEL_MASK                        0x1ull
#define DC_LPE_REG_0_HB_TIMER_ACCEL_SMASK                       0x1000ull
#define DC_LPE_REG_0_FC_TIMER_INHIBIT_SHIFT                     11
#define DC_LPE_REG_0_FC_TIMER_INHIBIT_MASK                      0x1ull
#define DC_LPE_REG_0_FC_TIMER_INHIBIT_SMASK                     0x800ull
#define DC_LPE_REG_0_FC_TIMER_ACCEL_SHIFT                       10
#define DC_LPE_REG_0_FC_TIMER_ACCEL_MASK                        0x1ull
#define DC_LPE_REG_0_FC_TIMER_ACCEL_SMASK                       0x400ull
#define DC_LPE_REG_0_LRH_LVER_EN_SHIFT                          9
#define DC_LPE_REG_0_LRH_LVER_EN_MASK                           0x1ull
#define DC_LPE_REG_0_LRH_LVER_EN_SMASK                          0x200ull
#define DC_LPE_REG_0_SKIP_ACCELERATE_SHIFT                      8
#define DC_LPE_REG_0_SKIP_ACCELERATE_MASK                       0x1ull
#define DC_LPE_REG_0_SKIP_ACCELERATE_SMASK                      0x100ull
#define DC_LPE_REG_0_TX_TEST_MODE_EN_SHIFT                      7
#define DC_LPE_REG_0_TX_TEST_MODE_EN_MASK                       0x1ull
#define DC_LPE_REG_0_TX_TEST_MODE_EN_SMASK                      0x80ull
#define DC_LPE_REG_0_LPE_TX_ECC_MARK_DIS_SHIFT                  6
#define DC_LPE_REG_0_LPE_TX_ECC_MARK_DIS_MASK                   0x1ull
#define DC_LPE_REG_0_LPE_TX_ECC_MARK_DIS_SMASK                  0x40ull
#define DC_LPE_REG_0_FORCE_RX_ACT_TRIG_SHIFT                    5
#define DC_LPE_REG_0_FORCE_RX_ACT_TRIG_MASK                     0x1ull
#define DC_LPE_REG_0_FORCE_RX_ACT_TRIG_SMASK                    0x20ull
#define DC_LPE_REG_0_AUTO_ACT_ENABLE_SHIFT                      4
#define DC_LPE_REG_0_AUTO_ACT_ENABLE_MASK                       0x1ull
#define DC_LPE_REG_0_AUTO_ACT_ENABLE_SMASK                      0x10ull
#define DC_LPE_REG_0_AUTO_ARM_ENABLE_SHIFT                      3
#define DC_LPE_REG_0_AUTO_ARM_ENABLE_MASK                       0x1ull
#define DC_LPE_REG_0_AUTO_ARM_ENABLE_SMASK                      0x8ull
#define DC_LPE_REG_0_LPE_LOOPBACK_SHIFT                         2
#define DC_LPE_REG_0_LPE_LOOPBACK_MASK                          0x1ull
#define DC_LPE_REG_0_LPE_LOOPBACK_SMASK                         0x4ull
#define DC_LPE_REG_0_LPE_LINK_ECC_GEN_EN_SHIFT                  1
#define DC_LPE_REG_0_LPE_LINK_ECC_GEN_EN_MASK                   0x1ull
#define DC_LPE_REG_0_LPE_LINK_ECC_GEN_EN_SMASK                  0x2ull
#define DC_LPE_REG_0_ACT_DEFER_PKT_FLOW_EN_SHIFT                0
#define DC_LPE_REG_0_ACT_DEFER_PKT_FLOW_EN_MASK                 0x1ull
#define DC_LPE_REG_0_ACT_DEFER_PKT_FLOW_EN_SMASK                0x1ull
/*
* Table #3 of 240_CSR_pcslpe.xml - lpe_reg_1
* See description below for individual field definition
*/
#define DC_LPE_REG_1                                            (DC_PCSLPE_LPE + 0x000000000008)
#define DC_LPE_REG_1_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_1_UNUSED_31_5_SHIFT                          5
#define DC_LPE_REG_1_UNUSED_31_5_MASK                           0x7FFFFFFull
#define DC_LPE_REG_1_UNUSED_31_5_SMASK                          0xFFFFFFE0ull
#define DC_LPE_REG_1_LOCAL_BUS_PE_SHIFT                         4
#define DC_LPE_REG_1_LOCAL_BUS_PE_MASK                          0x1ull
#define DC_LPE_REG_1_LOCAL_BUS_PE_SMASK                         0x10ull
#define DC_LPE_REG_1_REG_RD_PE_SHIFT                            3
#define DC_LPE_REG_1_REG_RD_PE_MASK                             0x1ull
#define DC_LPE_REG_1_REG_RD_PE_SMASK                            0x8ull
#define DC_LPE_REG_1_CP_ACTIVE_TRIGGER_SHIFT                    2
#define DC_LPE_REG_1_CP_ACTIVE_TRIGGER_MASK                     0x1ull
#define DC_LPE_REG_1_CP_ACTIVE_TRIGGER_SMASK                    0x4ull
#define DC_LPE_REG_1_RX_ACTIVE_TRIGGER_SHIFT                    1
#define DC_LPE_REG_1_RX_ACTIVE_TRIGGER_MASK                     0x1ull
#define DC_LPE_REG_1_RX_ACTIVE_TRIGGER_SMASK                    0x2ull
#define DC_LPE_REG_1_ACTIVE_ENABLE_SHIFT                        0
#define DC_LPE_REG_1_ACTIVE_ENABLE_MASK                         0x1ull
#define DC_LPE_REG_1_ACTIVE_ENABLE_SMASK                        0x1ull
/*
* Table #4 of 240_CSR_pcslpe.xml - lpe_reg_2
* See description below for individual field definition
*/
#define DC_LPE_REG_2                                            (DC_PCSLPE_LPE + 0x000000000010)
#define DC_LPE_REG_2_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_2_UNUSED_31_1_SHIFT                          1
#define DC_LPE_REG_2_UNUSED_31_1_MASK                           0x7FFFFFFFull
#define DC_LPE_REG_2_UNUSED_31_1_SMASK                          0xFFFFFFFEull
#define DC_LPE_REG_2_LPE_EB_RESET_SHIFT                         0
#define DC_LPE_REG_2_LPE_EB_RESET_MASK                          0x1ull
#define DC_LPE_REG_2_LPE_EB_RESET_SMASK                         0x1ull
/*
* Table #5 of 240_CSR_pcslpe.xml - lpe_reg_3
* See description below for individual field definition
*/
#define DC_LPE_REG_3                                            (DC_PCSLPE_LPE + 0x000000000018)
#define DC_LPE_REG_3_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_3_UNUSED_31_3_SHIFT                          3
#define DC_LPE_REG_3_UNUSED_31_3_MASK                           0x1FFFFFFFull
#define DC_LPE_REG_3_UNUSED_31_3_SMASK                          0xFFFFFFF8ull
#define DC_LPE_REG_3_EN_TX_ICRC_GEN_SHIFT                       2
#define DC_LPE_REG_3_EN_TX_ICRC_GEN_MASK                        0x1ull
#define DC_LPE_REG_3_EN_TX_ICRC_GEN_SMASK                       0x4ull
#define DC_LPE_REG_3_DIS_TX_ICRC_CHK_SHIFT                      1
#define DC_LPE_REG_3_DIS_TX_ICRC_CHK_MASK                       0x1ull
#define DC_LPE_REG_3_DIS_TX_ICRC_CHK_SMASK                      0x2ull
#define DC_LPE_REG_3_DIS_RX_ICRC_CHK_SHIFT                      0
#define DC_LPE_REG_3_DIS_RX_ICRC_CHK_MASK                       0x1ull
#define DC_LPE_REG_3_DIS_RX_ICRC_CHK_SMASK                      0x1ull
/*
* Table #6 of 240_CSR_pcslpe.xml - lpe_reg_5
* This register controls the 8b10b Scramble feature that uses the TS3 exchange 
* to negotiate enabling Scrambling of the 8b data prior to conversion to 10b 
* during transmission and after the 10b to 8b conversion in the receive 
* direction. This mode enable must be coordinated from both sides of the link to 
* assure that there is data coherency after the mode is entered and exited. The 
* scrambling is self-synchronizing in that the endpoints automatically adjust to 
* the flow without strict bit-wise endpoint coordination. If the TS3 byte 9[2] 
* is set on the received TS3, the remote node has the capability to engage in 
* Scrambling. The Force_Scramble mode bit overrides the received TS3 byte 9[2]. 
* We must set our transmitted TS3 byte 9[2] (Scramble Capability) and set our 
* local scramble capability bit. Then when we enter link state Config.Idle and 
* both received and transmitted TS3 bit 9 is set, we enable Scrambling and this 
* is persistent until we go back to polling.
*/
#define DC_LPE_REG_5                                            (DC_PCSLPE_LPE + 0x000000000028)
#define DC_LPE_REG_5_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_5_UNUSED_31_4_SHIFT                          4
#define DC_LPE_REG_5_UNUSED_31_4_MASK                           0xFFFFFFFull
#define DC_LPE_REG_5_UNUSED_31_4_SMASK                          0xFFFFFFF0ull
#define DC_LPE_REG_5_SCRAMBLING_ACTIVE_SHIFT                    3
#define DC_LPE_REG_5_SCRAMBLING_ACTIVE_MASK                     0x1ull
#define DC_LPE_REG_5_SCRAMBLING_ACTIVE_SMASK                    0x8ull
#define DC_LPE_REG_5_LOCAL_SCRAMBLE_CAPABILITY_SHIFT            2
#define DC_LPE_REG_5_LOCAL_SCRAMBLE_CAPABILITY_MASK             0x1ull
#define DC_LPE_REG_5_LOCAL_SCRAMBLE_CAPABILITY_SMASK            0x4ull
#define DC_LPE_REG_5_REMOTE_SCRAMBLE_CAPABILITY_SHIFT           1
#define DC_LPE_REG_5_REMOTE_SCRAMBLE_CAPABILITY_MASK            0x1ull
#define DC_LPE_REG_5_REMOTE_SCRAMBLE_CAPABILITY_SMASK           0x2ull
#define DC_LPE_REG_5_FORCE_SCRAMBLE_SHIFT                       0
#define DC_LPE_REG_5_FORCE_SCRAMBLE_MASK                        0x1ull
#define DC_LPE_REG_5_FORCE_SCRAMBLE_SMASK                       0x1ull
/*
* Table #7 of 240_CSR_pcslpe.xml - lpe_reg_8
* See description below for individual field definition
*/
#define DC_LPE_REG_8                                            (DC_PCSLPE_LPE + 0x000000000040)
#define DC_LPE_REG_8_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_8_UNUSED_31_20_SHIFT                         20
#define DC_LPE_REG_8_UNUSED_31_20_MASK                          0xFFFull
#define DC_LPE_REG_8_UNUSED_31_20_SMASK                         0xFFF00000ull
#define DC_LPE_REG_8_POLLING_HOLD_TIMEOUT_SHIFT                 18
#define DC_LPE_REG_8_POLLING_HOLD_TIMEOUT_MASK                  0x3ull
#define DC_LPE_REG_8_POLLING_HOLD_TIMEOUT_SMASK                 0xC0000ull
#define DC_LPE_REG_8_POLLING_ACTIVE_TIMEOUT_SHIFT               16
#define DC_LPE_REG_8_POLLING_ACTIVE_TIMEOUT_MASK                0x3ull
#define DC_LPE_REG_8_POLLING_ACTIVE_TIMEOUT_SMASK               0x30000ull
#define DC_LPE_REG_8_POLLING_QUIET_TIMEOUT_SHIFT                14
#define DC_LPE_REG_8_POLLING_QUIET_TIMEOUT_MASK                 0x3ull
#define DC_LPE_REG_8_POLLING_QUIET_TIMEOUT_SMASK                0xC000ull
#define DC_LPE_REG_8_DEBOUNCE_TIMEOUT_SHIFT                     12
#define DC_LPE_REG_8_DEBOUNCE_TIMEOUT_MASK                      0x3ull
#define DC_LPE_REG_8_DEBOUNCE_TIMEOUT_SMASK                     0x3000ull
#define DC_LPE_REG_8_RCVR_CFG_HOLD_TIMEOUT_SHIFT                10
#define DC_LPE_REG_8_RCVR_CFG_HOLD_TIMEOUT_MASK                 0x3ull
#define DC_LPE_REG_8_RCVR_CFG_HOLD_TIMEOUT_SMASK                0xC00ull
#define DC_LPE_REG_8_RCVR_CFG_TIMEOUT_SHIFT                     8
#define DC_LPE_REG_8_RCVR_CFG_TIMEOUT_MASK                      0x3ull
#define DC_LPE_REG_8_RCVR_CFG_TIMEOUT_SMASK                     0x300ull
#define DC_LPE_REG_8_WAIT_RMT_TIMEOUT_SHIFT                     6
#define DC_LPE_REG_8_WAIT_RMT_TIMEOUT_MASK                      0x3ull
#define DC_LPE_REG_8_WAIT_RMT_TIMEOUT_SMASK                     0xC0ull
#define DC_LPE_REG_8_WAIT_RMT_HOLD_TIMEOUT_SHIFT                4
#define DC_LPE_REG_8_WAIT_RMT_HOLD_TIMEOUT_MASK                 0x3ull
#define DC_LPE_REG_8_WAIT_RMT_HOLD_TIMEOUT_SMASK                0x30ull
#define DC_LPE_REG_8_TX_REV_LANES_HOLD_TIMEOUT_SHIFT            2
#define DC_LPE_REG_8_TX_REV_LANES_HOLD_TIMEOUT_MASK             0x3ull
#define DC_LPE_REG_8_TX_REV_LANES_HOLD_TIMEOUT_SMASK            0xCull
#define DC_LPE_REG_8_TX_REV_LANES_TIMEOUT_SHIFT                 0
#define DC_LPE_REG_8_TX_REV_LANES_TIMEOUT_MASK                  0x3ull
#define DC_LPE_REG_8_TX_REV_LANES_TIMEOUT_SMASK                 0x3ull
/*
* Table #8 of 240_CSR_pcslpe.xml - lpe_reg_9
* See description below for individual field definition
*/
#define DC_LPE_REG_9                                            (DC_PCSLPE_LPE + 0x000000000048)
#define DC_LPE_REG_9_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_9_UNUSED_31_26_SHIFT                         26
#define DC_LPE_REG_9_UNUSED_31_26_MASK                          0x3Full
#define DC_LPE_REG_9_UNUSED_31_26_SMASK                         0xFC000000ull
#define DC_LPE_REG_9_CFG_ENHANCED_TIMEOUT_SHIFT                 24
#define DC_LPE_REG_9_CFG_ENHANCED_TIMEOUT_MASK                  0x3ull
#define DC_LPE_REG_9_CFG_ENHANCED_TIMEOUT_SMASK                 0x3000000ull
#define DC_LPE_REG_9_CFG_ENHANCED_HOLD_TIMEOUT_SHIFT            22
#define DC_LPE_REG_9_CFG_ENHANCED_HOLD_TIMEOUT_MASK             0x3ull
#define DC_LPE_REG_9_CFG_ENHANCED_HOLD_TIMEOUT_SMASK            0xC00000ull
#define DC_LPE_REG_9_WAIT_CFG_ENHANCED_TIMEOUT_SHIFT            20
#define DC_LPE_REG_9_WAIT_CFG_ENHANCED_TIMEOUT_MASK             0x3ull
#define DC_LPE_REG_9_WAIT_CFG_ENHANCED_TIMEOUT_SMASK            0x300000ull
#define DC_LPE_REG_9_WAIT_RMT_TEST_TIMEOUT_SHIFT                18
#define DC_LPE_REG_9_WAIT_RMT_TEST_TIMEOUT_MASK                 0x3ull
#define DC_LPE_REG_9_WAIT_RMT_TEST_TIMEOUT_SMASK                0xC0000ull
#define DC_LPE_REG_9_CFG_IDLE_TIMEOUT_SHIFT                     16
#define DC_LPE_REG_9_CFG_IDLE_TIMEOUT_MASK                      0x3ull
#define DC_LPE_REG_9_CFG_IDLE_TIMEOUT_SMASK                     0x30000ull
#define DC_LPE_REG_9_CFG_TEST_TIMEOUT_SHIFT                     14
#define DC_LPE_REG_9_CFG_TEST_TIMEOUT_MASK                      0x3ull
#define DC_LPE_REG_9_CFG_TEST_TIMEOUT_SMASK                     0xC000ull
#define DC_LPE_REG_9_CFG_IDLE_HOLD_TIMEOUT_SHIFT                12
#define DC_LPE_REG_9_CFG_IDLE_HOLD_TIMEOUT_MASK                 0x3ull
#define DC_LPE_REG_9_CFG_IDLE_HOLD_TIMEOUT_SMASK                0x3000ull
#define DC_LPE_REG_9_LINK_UP_DETECTED_HOLD_TIMEOUT_SHIFT        10
#define DC_LPE_REG_9_LINK_UP_DETECTED_HOLD_TIMEOUT_MASK         0x3ull
#define DC_LPE_REG_9_LINK_UP_DETECTED_HOLD_TIMEOUT_SMASK        0xC00ull
#define DC_LPE_REG_9_LINK_UP_HOLD_TIMEOUT_SHIFT                 8
#define DC_LPE_REG_9_LINK_UP_HOLD_TIMEOUT_MASK                  0x3ull
#define DC_LPE_REG_9_LINK_UP_HOLD_TIMEOUT_SMASK                 0x300ull
#define DC_LPE_REG_9_REC_RE_TRAIN_HOLD_TIMEOUT_SHIFT            6
#define DC_LPE_REG_9_REC_RE_TRAIN_HOLD_TIMEOUT_MASK             0x3ull
#define DC_LPE_REG_9_REC_RE_TRAIN_HOLD_TIMEOUT_SMASK            0xC0ull
#define DC_LPE_REG_9_REC_WAIT_RMT_HOLD_TIMEOUT_SHIFT            4
#define DC_LPE_REG_9_REC_WAIT_RMT_HOLD_TIMEOUT_MASK             0x3ull
#define DC_LPE_REG_9_REC_WAIT_RMT_HOLD_TIMEOUT_SMASK            0x30ull
#define DC_LPE_REG_9_REC_IDLE_HOLD_TIMEOUT_SHIFT                2
#define DC_LPE_REG_9_REC_IDLE_HOLD_TIMEOUT_MASK                 0x3ull
#define DC_LPE_REG_9_REC_IDLE_HOLD_TIMEOUT_SMASK                0xCull
#define DC_LPE_REG_9_RECOVERY_TIMEOUT_SHIFT                     0
#define DC_LPE_REG_9_RECOVERY_TIMEOUT_MASK                      0x3ull
#define DC_LPE_REG_9_RECOVERY_TIMEOUT_SMASK                     0x3ull
/*
* Table #9 of 240_CSR_pcslpe.xml - lpe_reg_A
* See description below for individual field definition
*/
#define DC_LPE_REG_A                                            (DC_PCSLPE_LPE + 0x000000000050)
#define DC_LPE_REG_A_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_A_UNUSED_31_19_SHIFT                         19
#define DC_LPE_REG_A_UNUSED_31_19_MASK                          0x1FFFull
#define DC_LPE_REG_A_UNUSED_31_19_SMASK                         0xFFF80000ull
#define DC_LPE_REG_A_RCV_TS_T_VALID_SHIFT                       18
#define DC_LPE_REG_A_RCV_TS_T_VALID_MASK                        0x1ull
#define DC_LPE_REG_A_RCV_TS_T_VALID_SMASK                       0x40000ull
#define DC_LPE_REG_A_RCV_TS_T_LANE_SHIFT                        16
#define DC_LPE_REG_A_RCV_TS_T_LANE_MASK                         0x3ull
#define DC_LPE_REG_A_RCV_TS_T_LANE_SMASK                        0x30000ull
#define DC_LPE_REG_A_RCV_TS_T_SPEEDS_SHIFT                      8
#define DC_LPE_REG_A_RCV_TS_T_SPEEDS_MASK                       0xFFull
#define DC_LPE_REG_A_RCV_TS_T_SPEEDS_SMASK                      0xFF00ull
#define DC_LPE_REG_A_RCV_TS_T_OPCODE_SHIFT                      0
#define DC_LPE_REG_A_RCV_TS_T_OPCODE_MASK                       0xFFull
#define DC_LPE_REG_A_RCV_TS_T_OPCODE_SMASK                      0xFFull
/*
* Table #10 of 240_CSR_pcslpe.xml - lpe_reg_B
* See description below for individual field definition
*/
#define DC_LPE_REG_B                                            (DC_PCSLPE_LPE + 0x000000000058)
#define DC_LPE_REG_B_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_B_RCV_TS_T_TX_CFG_SHIFT                      16
#define DC_LPE_REG_B_RCV_TS_T_TX_CFG_MASK                       0xFFFFull
#define DC_LPE_REG_B_RCV_TS_T_TX_CFG_SMASK                      0xFFFF0000ull
#define DC_LPE_REG_B_RCV_TS_T_RX_CFG_SHIFT                      0
#define DC_LPE_REG_B_RCV_TS_T_RX_CFG_MASK                       0xFFFFull
#define DC_LPE_REG_B_RCV_TS_T_RX_CFG_SMASK                      0xFFFFull
/*
* Table #11 of 240_CSR_pcslpe.xml - lpe_reg_C
* See description below for individual field definition
*/
#define DC_LPE_REG_C                                            (DC_PCSLPE_LPE + 0x000000000060)
#define DC_LPE_REG_C_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_C_UNUSED_31_19_SHIFT                         19
#define DC_LPE_REG_C_UNUSED_31_19_MASK                          0x1FFFull
#define DC_LPE_REG_C_UNUSED_31_19_SMASK                         0xFFF80000ull
#define DC_LPE_REG_C_XMIT_TS_T_VALID_SHIFT                      18
#define DC_LPE_REG_C_XMIT_TS_T_VALID_MASK                       0x1ull
#define DC_LPE_REG_C_XMIT_TS_T_VALID_SMASK                      0x40000ull
#define DC_LPE_REG_C_XMIT_TS_T_LANE_SHIFT                       16
#define DC_LPE_REG_C_XMIT_TS_T_LANE_MASK                        0x3ull
#define DC_LPE_REG_C_XMIT_TS_T_LANE_SMASK                       0x30000ull
#define DC_LPE_REG_C_XMIT_TS_T_SPEEDS_SHIFT                     8
#define DC_LPE_REG_C_XMIT_TS_T_SPEEDS_MASK                      0xFFull
#define DC_LPE_REG_C_XMIT_TS_T_SPEEDS_SMASK                     0xFF00ull
#define DC_LPE_REG_C_XMIT_TS_T_OPCODE_SHIFT                     0
#define DC_LPE_REG_C_XMIT_TS_T_OPCODE_MASK                      0xFFull
#define DC_LPE_REG_C_XMIT_TS_T_OPCODE_SMASK                     0xFFull
/*
* Table #12 of 240_CSR_pcslpe.xml - lpe_reg_D
* See description below for individual field definition
*/
#define DC_LPE_REG_D                                            (DC_PCSLPE_LPE + 0x000000000068)
#define DC_LPE_REG_D_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_D_XMIT_TS_T_TX_CFG_SHIFT                     16
#define DC_LPE_REG_D_XMIT_TS_T_TX_CFG_MASK                      0xFFFFull
#define DC_LPE_REG_D_XMIT_TS_T_TX_CFG_SMASK                     0xFFFF0000ull
#define DC_LPE_REG_D_XMIT_TS_T_RX_CFG_SHIFT                     0
#define DC_LPE_REG_D_XMIT_TS_T_RX_CFG_MASK                      0xFFFFull
#define DC_LPE_REG_D_XMIT_TS_T_RX_CFG_SMASK                     0xFFFFull
/*
* Table #13 of 240_CSR_pcslpe.xml - lpe_reg_E
* Spare Register at this time.
*/
#define DC_LPE_REG_E                                            (DC_PCSLPE_LPE + 0x000000000070)
#define DC_LPE_REG_E_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_E_UNUSED_31_0_SHIFT                          0
#define DC_LPE_REG_E_UNUSED_31_0_MASK                           0xFFFFFFFFull
#define DC_LPE_REG_E_UNUSED_31_0_SMASK                          0xFFFFFFFFull
/*
* Table #14 of 240_CSR_pcslpe.xml - lpe_reg_F
* Heartbeats SND packets may be sent manually using the Heartbeat request or 
* sent automatically using Heartbeat auto control.  The PORT and GUID values are 
* placed in the Heartbeat SND packet.  Heartbeat ACK packets are sent when a 
* Heartbeat SND packet has been received.  The Heartbeat ACK packet must contain 
* the same GUID value from the Heartbeat SND packet received.  Heartbeat ACK 
* packets received must contain GUID value matching the GUID of the Heartbeat 
* SND packet.  If the GUID's do not match the ACK packet is ignored.  If a 
* Heartbeat SND packet is received with a GUID matching the port GUID, this 
* indicates channel crosstalk.
*/
#define DC_LPE_REG_F                                            (DC_PCSLPE_LPE + 0x000000000078)
#define DC_LPE_REG_F_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_F_HB_SND_XMIT_VALID_SHIFT                    31
#define DC_LPE_REG_F_HB_SND_XMIT_VALID_MASK                     0x1ull
#define DC_LPE_REG_F_HB_SND_XMIT_VALID_SMASK                    0x80000000ull
#define DC_LPE_REG_F_HB_SND_RCVD_VALID_SHIFT                    30
#define DC_LPE_REG_F_HB_SND_RCVD_VALID_MASK                     0x1ull
#define DC_LPE_REG_F_HB_SND_RCVD_VALID_SMASK                    0x40000000ull
#define DC_LPE_REG_F_HB_ACK_XMIT_VALID_SHIFT                    29
#define DC_LPE_REG_F_HB_ACK_XMIT_VALID_MASK                     0x1ull
#define DC_LPE_REG_F_HB_ACK_XMIT_VALID_SMASK                    0x20000000ull
#define DC_LPE_REG_F_HB_ACK_RCVD_VALID_SHIFT                    28
#define DC_LPE_REG_F_HB_ACK_RCVD_VALID_MASK                     0x1ull
#define DC_LPE_REG_F_HB_ACK_RCVD_VALID_SMASK                    0x10000000ull
#define DC_LPE_REG_F_HB_TIMED_OUT_SHIFT                         27
#define DC_LPE_REG_F_HB_TIMED_OUT_MASK                          0x1ull
#define DC_LPE_REG_F_HB_TIMED_OUT_SMASK                         0x8000000ull
#define DC_LPE_REG_F_UNUSED_26_18_SHIFT                         18
#define DC_LPE_REG_F_UNUSED_26_18_MASK                          0x1FFull
#define DC_LPE_REG_F_UNUSED_26_18_SMASK                         0x7FC0000ull
#define DC_LPE_REG_F_UNUSED_17_16_SHIFT                         16
#define DC_LPE_REG_F_UNUSED_17_16_MASK                          0x3ull
#define DC_LPE_REG_F_UNUSED_17_16_SMASK                         0x30000ull
#define DC_LPE_REG_F_RCV_SND_HB_PORT_NUM_SHIFT                  8
#define DC_LPE_REG_F_RCV_SND_HB_PORT_NUM_MASK                   0xFFull
#define DC_LPE_REG_F_RCV_SND_HB_PORT_NUM_SMASK                  0xFF00ull
#define DC_LPE_REG_F_RCV_ACK_HB_PORT_NUM_SHIFT                  0
#define DC_LPE_REG_F_RCV_ACK_HB_PORT_NUM_MASK                   0xFFull
#define DC_LPE_REG_F_RCV_ACK_HB_PORT_NUM_SMASK                  0xFFull
/*
* Table #15 of 240_CSR_pcslpe.xml - lpe_reg_10
* See description below for individual field definition
*/
#define DC_LPE_REG_10                                           (DC_PCSLPE_LPE + 0x000000000080)
#define DC_LPE_REG_10_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_10_RCV_SND_HB_GUID_U_SHIFT                   0
#define DC_LPE_REG_10_RCV_SND_HB_GUID_U_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_10_RCV_SND_HB_GUID_U_SMASK                   0xFFFFFFFFull
/*
* Table #16 of 240_CSR_pcslpe.xml - lpe_reg_11
* See description below for individual field definition
*/
#define DC_LPE_REG_11                                           (DC_PCSLPE_LPE + 0x000000000088)
#define DC_LPE_REG_11_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_11_RCV_SND_HB_GUID_L_SHIFT                   0
#define DC_LPE_REG_11_RCV_SND_HB_GUID_L_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_11_RCV_SND_HB_GUID_L_SMASK                   0xFFFFFFFFull
/*
* Table #17 of 240_CSR_pcslpe.xml - lpe_reg_12
* See description below for individual field definition
*/
#define DC_LPE_REG_12                                           (DC_PCSLPE_LPE + 0x000000000090)
#define DC_LPE_REG_12_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_12_RCV_ACK_HB_GUID_U_SHIFT                   0
#define DC_LPE_REG_12_RCV_ACK_HB_GUID_U_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_12_RCV_ACK_HB_GUID_U_SMASK                   0xFFFFFFFFull
/*
* Table #18 of 240_CSR_pcslpe.xml - lpe_reg_13
* See description below for individual field definition
*/
#define DC_LPE_REG_13                                           (DC_PCSLPE_LPE + 0x000000000098)
#define DC_LPE_REG_13_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_13_RCV_ACK_HB_GUID_L_SHIFT                   0
#define DC_LPE_REG_13_RCV_ACK_HB_GUID_L_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_13_RCV_ACK_HB_GUID_L_SMASK                   0xFFFFFFFFull
/*
* Table #19 of 240_CSR_pcslpe.xml - lpe_reg_14
* See description below for individual field definition
*/
#define DC_LPE_REG_14                                           (DC_PCSLPE_LPE + 0x0000000000A0)
#define DC_LPE_REG_14_RESETCSR                                  0x000F0000ull
#define DC_LPE_REG_14_UNUSED_31_28_SHIFT                        28
#define DC_LPE_REG_14_UNUSED_31_28_MASK                         0xFull
#define DC_LPE_REG_14_UNUSED_31_28_SMASK                        0xF0000000ull
#define DC_LPE_REG_14_HB_CROSSTALK_SHIFT                        24
#define DC_LPE_REG_14_HB_CROSSTALK_MASK                         0xFull
#define DC_LPE_REG_14_HB_CROSSTALK_SMASK                        0xF000000ull
#define DC_LPE_REG_14_UNUSED_23_SHIFT                           23
#define DC_LPE_REG_14_UNUSED_23_MASK                            0x1ull
#define DC_LPE_REG_14_UNUSED_23_SMASK                           0x800000ull
#define DC_LPE_REG_14_XMIT_HB_ENABLE_SHIFT                      22
#define DC_LPE_REG_14_XMIT_HB_ENABLE_MASK                       0x1ull
#define DC_LPE_REG_14_XMIT_HB_ENABLE_SMASK                      0x400000ull
#define DC_LPE_REG_14_XMIT_HB_REQUEST_SHIFT                     21
#define DC_LPE_REG_14_XMIT_HB_REQUEST_MASK                      0x1ull
#define DC_LPE_REG_14_XMIT_HB_REQUEST_SMASK                     0x200000ull
#define DC_LPE_REG_14_XMIT_HB_AUTO_SHIFT                        20
#define DC_LPE_REG_14_XMIT_HB_AUTO_MASK                         0x1ull
#define DC_LPE_REG_14_XMIT_HB_AUTO_SMASK                        0x100000ull
#define DC_LPE_REG_14_XMIT_HB_LANE_MASK_SHIFT                   16
#define DC_LPE_REG_14_XMIT_HB_LANE_MASK_MASK                    0xFull
#define DC_LPE_REG_14_XMIT_HB_LANE_MASK_SMASK                   0xF0000ull
#define DC_LPE_REG_14_XMIT_HB_OPCODE_SHIFT                      8
#define DC_LPE_REG_14_XMIT_HB_OPCODE_MASK                       0xFFull
#define DC_LPE_REG_14_XMIT_HB_OPCODE_SMASK                      0xFF00ull
#define DC_LPE_REG_14_XMIT_HB_PORT_NUM_SHIFT                    0
#define DC_LPE_REG_14_XMIT_HB_PORT_NUM_MASK                     0xFFull
#define DC_LPE_REG_14_XMIT_HB_PORT_NUM_SMASK                    0xFFull
/*
* Table #20 of 240_CSR_pcslpe.xml - lpe_reg_15
* See description below for individual field definition
*/
#define DC_LPE_REG_15                                           (DC_PCSLPE_LPE + 0x0000000000A8)
#define DC_LPE_REG_15_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_15_XMIT_HB_GUID_UPPR_SHIFT                   0
#define DC_LPE_REG_15_XMIT_HB_GUID_UPPR_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_15_XMIT_HB_GUID_UPPR_SMASK                   0xFFFFFFFFull
/*
* Table #21 of 240_CSR_pcslpe.xml - lpe_reg_16
* See description below for individual field definition
*/
#define DC_LPE_REG_16                                           (DC_PCSLPE_LPE + 0x0000000000B0)
#define DC_LPE_REG_16_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_16_XMIT_HB_GUID_LOWR_SHIFT                   0
#define DC_LPE_REG_16_XMIT_HB_GUID_LOWR_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_16_XMIT_HB_GUID_LOWR_SMASK                   0xFFFFFFFFull
/*
* Table #22 of 240_CSR_pcslpe.xml - lpe_reg_17
* See description below for individual field definition
*/
#define DC_LPE_REG_17                                           (DC_PCSLPE_LPE + 0x0000000000B8)
#define DC_LPE_REG_17_RESETCSR                                  0x07FFFFFFull
#define DC_LPE_REG_17_UNUSED_31_27_SHIFT                        27
#define DC_LPE_REG_17_UNUSED_31_27_MASK                         0x1Full
#define DC_LPE_REG_17_UNUSED_31_27_SMASK                        0xF8000000ull
#define DC_LPE_REG_17_HB_LATENCY_SHIFT                          0
#define DC_LPE_REG_17_HB_LATENCY_MASK                           0x7FFFFFFull
#define DC_LPE_REG_17_HB_LATENCY_SMASK                          0x7FFFFFFull
/*
* Table #23 of 240_CSR_pcslpe.xml - lpe_reg_18
* See description below for individual field definition
*/
#define DC_LPE_REG_18                                           (DC_PCSLPE_LPE + 0x0000000000C0)
#define DC_LPE_REG_18_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_18_TS_T_OS_DETECTED_SHIFT                    28
#define DC_LPE_REG_18_TS_T_OS_DETECTED_MASK                     0xFull
#define DC_LPE_REG_18_TS_T_OS_DETECTED_SMASK                    0xF0000000ull
#define DC_LPE_REG_18_TS12_OS_ERR_DETECTED_SHIFT                24
#define DC_LPE_REG_18_TS12_OS_ERR_DETECTED_MASK                 0xFull
#define DC_LPE_REG_18_TS12_OS_ERR_DETECTED_SMASK                0xF000000ull
#define DC_LPE_REG_18_TS2_SEQ_DETECTED_SHIFT                    20
#define DC_LPE_REG_18_TS2_SEQ_DETECTED_MASK                     0xFull
#define DC_LPE_REG_18_TS2_SEQ_DETECTED_SMASK                    0xF00000ull
#define DC_LPE_REG_18_TS1_SEQ_DETECTED_SHIFT                    16
#define DC_LPE_REG_18_TS1_SEQ_DETECTED_MASK                     0xFull
#define DC_LPE_REG_18_TS1_SEQ_DETECTED_SMASK                    0xF0000ull
#define DC_LPE_REG_18_SKP_OS_DETECTED_SHIFT                     12
#define DC_LPE_REG_18_SKP_OS_DETECTED_MASK                      0xFull
#define DC_LPE_REG_18_SKP_OS_DETECTED_SMASK                     0xF000ull
#define DC_LPE_REG_18_TS3_OS_DETECTED_SHIFT                     8
#define DC_LPE_REG_18_TS3_OS_DETECTED_MASK                      0xFull
#define DC_LPE_REG_18_TS3_OS_DETECTED_SMASK                     0xF00ull
#define DC_LPE_REG_18_TS2_OS_DETECTED_SHIFT                     4
#define DC_LPE_REG_18_TS2_OS_DETECTED_MASK                      0xFull
#define DC_LPE_REG_18_TS2_OS_DETECTED_SMASK                     0xF0ull
#define DC_LPE_REG_18_TS1_OS_DETECTED_SHIFT                     0
#define DC_LPE_REG_18_TS1_OS_DETECTED_MASK                      0xFull
#define DC_LPE_REG_18_TS1_OS_DETECTED_SMASK                     0xFull
/*
* Table #24 of 240_CSR_pcslpe.xml - lpe_reg_19
* See description below for individual field definition
*/
#define DC_LPE_REG_19                                           (DC_PCSLPE_LPE + 0x0000000000C8)
#define DC_LPE_REG_19_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_19_L0_TS1_CNT_SHIFT                          24
#define DC_LPE_REG_19_L0_TS1_CNT_MASK                           0xFFull
#define DC_LPE_REG_19_L0_TS1_CNT_SMASK                          0xFF000000ull
/*
* Table #25 of 240_CSR_pcslpe.xml - lpe_reg_1A
* See description below for individual field definition
*/
#define DC_LPE_REG_1A                                           (DC_PCSLPE_LPE + 0x0000000000D0)
#define DC_LPE_REG_1A_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_1A_L0_TS2_CNT_SHIFT                          24
#define DC_LPE_REG_1A_L0_TS2_CNT_MASK                           0xFFull
#define DC_LPE_REG_1A_L0_TS2_CNT_SMASK                          0xFF000000ull
/*
* Table #26 of 240_CSR_pcslpe.xml - lpe_reg_1B
* See description below for individual field definition
*/
#define DC_LPE_REG_1B                                           (DC_PCSLPE_LPE + 0x0000000000D8)
#define DC_LPE_REG_1B_RESETCSR                                  0x00080000ull
#define DC_LPE_REG_1B_TS12_ALIVE_CNT_THRES_SHIFT                16
#define DC_LPE_REG_1B_TS12_ALIVE_CNT_THRES_MASK                 0xFFFFull
#define DC_LPE_REG_1B_TS12_ALIVE_CNT_THRES_SMASK                0xFFFF0000ull
#define DC_LPE_REG_1B_LANE_ALIVE_SHIFT                          12
#define DC_LPE_REG_1B_LANE_ALIVE_MASK                           0xFull
#define DC_LPE_REG_1B_LANE_ALIVE_SMASK                          0xF000ull
#define DC_LPE_REG_1B_HOLD_TIME_LANE_ALIVE_SHIFT                8
#define DC_LPE_REG_1B_HOLD_TIME_LANE_ALIVE_MASK                 0xFull
#define DC_LPE_REG_1B_HOLD_TIME_LANE_ALIVE_SMASK                0xF00ull
#define DC_LPE_REG_1B_UNUSED_7_2_SHIFT                          2
#define DC_LPE_REG_1B_UNUSED_7_2_MASK                           0x3Full
#define DC_LPE_REG_1B_UNUSED_7_2_SMASK                          0xFCull
#define DC_LPE_REG_1B_OS_SEQ_CNT_SHIFT                          0
#define DC_LPE_REG_1B_OS_SEQ_CNT_MASK                           0x3ull
#define DC_LPE_REG_1B_OS_SEQ_CNT_SMASK                          0x3ull
/*
* Table #27 of 240_CSR_pcslpe.xml - lpe_reg_1C
* See description below for individual field definition
*/
#define DC_LPE_REG_1C                                           (DC_PCSLPE_LPE + 0x0000000000E0)
#define DC_LPE_REG_1C_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_1C_UNUSED_31_25_SHIFT                        25
#define DC_LPE_REG_1C_UNUSED_31_25_MASK                         0x7Full
#define DC_LPE_REG_1C_UNUSED_31_25_SMASK                        0xFE000000ull
#define DC_LPE_REG_1C_TX_MOD_TS1_EN_SHIFT                       24
#define DC_LPE_REG_1C_TX_MOD_TS1_EN_MASK                        0x1ull
#define DC_LPE_REG_1C_TX_MOD_TS1_EN_SMASK                       0x1000000ull
#define DC_LPE_REG_1C_TX_MOD_TS1_DATA_SHIFT                     0
#define DC_LPE_REG_1C_TX_MOD_TS1_DATA_MASK                      0xFFFFFFull
#define DC_LPE_REG_1C_TX_MOD_TS1_DATA_SMASK                     0xFFFFFFull
/*
* Table #28 of 240_CSR_pcslpe.xml - lpe_reg_1D
* See description below for individual field definition
*/
#define DC_LPE_REG_1D                                           (DC_PCSLPE_LPE + 0x0000000000E8)
#define DC_LPE_REG_1D_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_1D_UNUSED_31_25_SHIFT                        25
#define DC_LPE_REG_1D_UNUSED_31_25_MASK                         0x7Full
#define DC_LPE_REG_1D_UNUSED_31_25_SMASK                        0xFE000000ull
#define DC_LPE_REG_1D_RX_MOD_TS1_SAMPLED_SHIFT                  24
#define DC_LPE_REG_1D_RX_MOD_TS1_SAMPLED_MASK                   0x1ull
#define DC_LPE_REG_1D_RX_MOD_TS1_SAMPLED_SMASK                  0x1000000ull
#define DC_LPE_REG_1D_RX_MOD_TS1_DATA_SHIFT                     0
#define DC_LPE_REG_1D_RX_MOD_TS1_DATA_MASK                      0xFFFFFFull
#define DC_LPE_REG_1D_RX_MOD_TS1_DATA_SMASK                     0xFFFFFFull
/*
* Table #29 of 240_CSR_pcslpe.xml - lpe_reg_1E
* See description below for individual field definition
*/
#define DC_LPE_REG_1E                                           (DC_PCSLPE_LPE + 0x0000000000F0)
#define DC_LPE_REG_1E_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_1E_UNUSED_31_25_SHIFT                        25
#define DC_LPE_REG_1E_UNUSED_31_25_MASK                         0x7Full
#define DC_LPE_REG_1E_UNUSED_31_25_SMASK                        0xFE000000ull
#define DC_LPE_REG_1E_CFFT_TX_REQ_SHIFT                         24
#define DC_LPE_REG_1E_CFFT_TX_REQ_MASK                          0x1ull
#define DC_LPE_REG_1E_CFFT_TX_REQ_SMASK                         0x1000000ull
#define DC_LPE_REG_1E_CCFT_TX_CMD_SHIFT                         16
#define DC_LPE_REG_1E_CCFT_TX_CMD_MASK                          0xFFull
#define DC_LPE_REG_1E_CCFT_TX_CMD_SMASK                         0xFF0000ull
#define DC_LPE_REG_1E_CCFT_TX_STATUS_SHIFT                      8
#define DC_LPE_REG_1E_CCFT_TX_STATUS_MASK                       0xFFull
#define DC_LPE_REG_1E_CCFT_TX_STATUS_SMASK                      0xFF00ull
#define DC_LPE_REG_1E_CCFT_TX_LANE_SHIFT                        0
#define DC_LPE_REG_1E_CCFT_TX_LANE_MASK                         0xFFull
#define DC_LPE_REG_1E_CCFT_TX_LANE_SMASK                        0xFFull
/*
* Table #30 of 240_CSR_pcslpe.xml - lpe_reg_1F
* See description below for individual field definition
*/
#define DC_LPE_REG_1F                                           (DC_PCSLPE_LPE + 0x0000000000F8)
#define DC_LPE_REG_1F_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_1F_UNUSED_31_25_SHIFT                        25
#define DC_LPE_REG_1F_UNUSED_31_25_MASK                         0x7Full
#define DC_LPE_REG_1F_UNUSED_31_25_SMASK                        0xFE000000ull
#define DC_LPE_REG_1F_CCFT_RX_REQ_SHIFT                         24
#define DC_LPE_REG_1F_CCFT_RX_REQ_MASK                          0x1ull
#define DC_LPE_REG_1F_CCFT_RX_REQ_SMASK                         0x1000000ull
#define DC_LPE_REG_1F_CCFT_RX_CMD_SHIFT                         16
#define DC_LPE_REG_1F_CCFT_RX_CMD_MASK                          0xFFull
#define DC_LPE_REG_1F_CCFT_RX_CMD_SMASK                         0xFF0000ull
#define DC_LPE_REG_1F_CCFT_RX_STATUS_SHIFT                      8
#define DC_LPE_REG_1F_CCFT_RX_STATUS_MASK                       0xFFull
#define DC_LPE_REG_1F_CCFT_RX_STATUS_SMASK                      0xFF00ull
#define DC_LPE_REG_1F_CCFT_RX_LANE_SHIFT                        0
#define DC_LPE_REG_1F_CCFT_RX_LANE_MASK                         0xFFull
#define DC_LPE_REG_1F_CCFT_RX_LANE_SMASK                        0xFFull
/*
* Table #31 of 240_CSR_pcslpe.xml - lpe_reg_20
* See description below for individual field definition
*/
#define DC_LPE_REG_20                                           (DC_PCSLPE_LPE + 0x000000000100)
#define DC_LPE_REG_20_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_20_TS3_SEQ_RX_DATA_89_SHIFT                  31
#define DC_LPE_REG_20_TS3_SEQ_RX_DATA_89_MASK                   0x1ull
#define DC_LPE_REG_20_TS3_SEQ_RX_DATA_89_SMASK                  0x80000000ull
#define DC_LPE_REG_20_UNUSED_30_16_SHIFT                        16
#define DC_LPE_REG_20_UNUSED_30_16_MASK                         0x7FFFull
#define DC_LPE_REG_20_UNUSED_30_16_SMASK                        0x7FFF0000ull
#define DC_LPE_REG_20_TS3BYTE8_SHIFT                            8
#define DC_LPE_REG_20_TS3BYTE8_MASK                             0xFFull
#define DC_LPE_REG_20_TS3BYTE8_SMASK                            0xFF00ull
#define DC_LPE_REG_20_TS3BYTE9_SHIFT                            0
#define DC_LPE_REG_20_TS3BYTE9_MASK                             0xFFull
#define DC_LPE_REG_20_TS3BYTE9_SMASK                            0xFFull
/*
* Table #32 of 240_CSR_pcslpe.xml - lpe_reg_21
* See description below for individual field definition
*/
#define DC_LPE_REG_21                                           (DC_PCSLPE_LPE + 0x000000000108)
#define DC_LPE_REG_21_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_21_TS3_RX_DATA_10_LANE_0_SHIFT               24
#define DC_LPE_REG_21_TS3_RX_DATA_10_LANE_0_MASK                0xFFull
#define DC_LPE_REG_21_TS3_RX_DATA_10_LANE_0_SMASK               0xFF000000ull
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_1_SHIFT               16
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_1_MASK                0xFFull
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_1_SMASK               0xFF0000ull
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_2_SHIFT               8
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_2_MASK                0xFFull
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_2_SMASK               0xFF00ull
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_3_SHIFT               0
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_3_MASK                0xFFull
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_3_SMASK               0xFFull
/*
* Table #33 of 240_CSR_pcslpe.xml - lpe_reg_22
* See description below for individual field definition
*/
#define DC_LPE_REG_22                                           (DC_PCSLPE_LPE + 0x000000000110)
#define DC_LPE_REG_22_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_0_SHIFT               24
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_0_MASK                0xFFull
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_0_SMASK               0xFF000000ull
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_1_SHIFT               16
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_1_MASK                0xFFull
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_1_SMASK               0xFF0000ull
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_2_SHIFT               8
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_2_MASK                0xFFull
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_2_SMASK               0xFF00ull
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_3_SHIFT               0
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_3_MASK                0xFFull
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_3_SMASK               0xFFull
/*
* Table #34 of 240_CSR_pcslpe.xml - lpe_reg_23
* See description below for individual field definition
*/
#define DC_LPE_REG_23                                           (DC_PCSLPE_LPE + 0x000000000118)
#define DC_LPE_REG_23_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_23_TS3_RX_BYTE_12_LANE_0_SHIFT               24
#define DC_LPE_REG_23_TS3_RX_BYTE_12_LANE_0_MASK                0xFFull
#define DC_LPE_REG_23_TS3_RX_BYTE_12_LANE_0_SMASK               0xFF000000ull
#define DC_LPE_REG_23_TS3_RX_BYTE_13_LANE_1_SHIFT               16
#define DC_LPE_REG_23_TS3_RX_BYTE_13_LANE_1_MASK                0xFFull
#define DC_LPE_REG_23_TS3_RX_BYTE_13_LANE_1_SMASK               0xFF0000ull
#define DC_LPE_REG_23_TS3_RX_BYTE_14_LANE_2_SHIFT               8
#define DC_LPE_REG_23_TS3_RX_BYTE_14_LANE_2_MASK                0xFFull
#define DC_LPE_REG_23_TS3_RX_BYTE_14_LANE_2_SMASK               0xFF00ull
#define DC_LPE_REG_23_TS3_RX_BYTE_15_LANE_3_SHIFT               0
#define DC_LPE_REG_23_TS3_RX_BYTE_15_LANE_3_MASK                0xFFull
#define DC_LPE_REG_23_TS3_RX_BYTE_15_LANE_3_SMASK               0xFFull
/*
* Table #35 of 240_CSR_pcslpe.xml - lpe_reg_24
* The following TX TS3 bytes are used when sending TS3's. See description below 
* for individual field definition#%%#Address = Base Address + Offset
*/
#define DC_LPE_REG_24                                           (DC_PCSLPE_LPE + 0x000000000120)
#define DC_LPE_REG_24_RESETCSR                                  0x00001040ull
#define DC_LPE_REG_24_UNUSED_31_16_SHIFT                        16
#define DC_LPE_REG_24_UNUSED_31_16_MASK                         0xFFFFull
#define DC_LPE_REG_24_UNUSED_31_16_SMASK                        0xFFFF0000ull
#define DC_LPE_REG_24_TS3_TX_BYTE_8_SHIFT                       8
#define DC_LPE_REG_24_TS3_TX_BYTE_8_MASK                        0xFFull
#define DC_LPE_REG_24_TS3_TX_BYTE_8_SMASK                       0xFF00ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_TT_SHIFT                    4
#define DC_LPE_REG_24_TS3_TX_BYTE_9_TT_MASK                     0xFull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_TT_SMASK                    0xF0ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_FT_SHIFT                    3
#define DC_LPE_REG_24_TS3_TX_BYTE_9_FT_MASK                     0x1ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_FT_SMASK                    0x8ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_SC_SHIFT                    2
#define DC_LPE_REG_24_TS3_TX_BYTE_9_SC_MASK                     0x1ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_SC_SMASK                    0x4ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_HBR_SHIFT                   1
#define DC_LPE_REG_24_TS3_TX_BYTE_9_HBR_MASK                    0x1ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_HBR_SMASK                   0x2ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_ADD_SHIFT                   0
#define DC_LPE_REG_24_TS3_TX_BYTE_9_ADD_MASK                    0x1ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_ADD_SMASK                   0x1ull
/*
* Table #36 of 240_CSR_pcslpe.xml - lpe_reg_25
* See description below for individual field definition
*/
#define DC_LPE_REG_25                                           (DC_PCSLPE_LPE + 0x000000000128)
#define DC_LPE_REG_25_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_25_TS3_TX_DATA_10_LANE_0_SHIFT               24
#define DC_LPE_REG_25_TS3_TX_DATA_10_LANE_0_MASK                0xFFull
#define DC_LPE_REG_25_TS3_TX_DATA_10_LANE_0_SMASK               0xFF000000ull
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_1_SHIFT               16
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_1_MASK                0xFFull
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_1_SMASK               0xFF0000ull
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_2_SHIFT               8
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_2_MASK                0xFFull
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_2_SMASK               0xFF00ull
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_3_SHIFT               0
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_3_MASK                0xFFull
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_3_SMASK               0xFFull
/*
* Table #37 of 240_CSR_pcslpe.xml - lpe_reg_26
* See description below for individual field definition
*/
#define DC_LPE_REG_26                                           (DC_PCSLPE_LPE + 0x000000000130)
#define DC_LPE_REG_26_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_0_SHIFT               24
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_0_MASK                0xFFull
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_0_SMASK               0xFF000000ull
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_1_SHIFT               16
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_1_MASK                0xFFull
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_1_SMASK               0xFF0000ull
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_2_SHIFT               8
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_2_MASK                0xFFull
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_2_SMASK               0xFF00ull
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_3_SHIFT               0
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_3_MASK                0xFFull
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_3_SMASK               0xFFull
/*
* Table #38 of 240_CSR_pcslpe.xml - lpe_reg_27
* See description below for individual field definition
*/
#define DC_LPE_REG_27                                           (DC_PCSLPE_LPE + 0x000000000138)
#define DC_LPE_REG_27_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_27_TS3_TX_BYTE_12_SHIFT                      24
#define DC_LPE_REG_27_TS3_TX_BYTE_12_MASK                       0xFFull
#define DC_LPE_REG_27_TS3_TX_BYTE_12_SMASK                      0xFF000000ull
#define DC_LPE_REG_27_TS3_TX_BYTE_13_SHIFT                      16
#define DC_LPE_REG_27_TS3_TX_BYTE_13_MASK                       0xFFull
#define DC_LPE_REG_27_TS3_TX_BYTE_13_SMASK                      0xFF0000ull
#define DC_LPE_REG_27_TS3_TX_BYTE_14_SHIFT                      8
#define DC_LPE_REG_27_TS3_TX_BYTE_14_MASK                       0xFFull
#define DC_LPE_REG_27_TS3_TX_BYTE_14_SMASK                      0xFF00ull
#define DC_LPE_REG_27_TS3_TX_BYTE_15_SHIFT                      0
#define DC_LPE_REG_27_TS3_TX_BYTE_15_MASK                       0xFFull
#define DC_LPE_REG_27_TS3_TX_BYTE_15_SMASK                      0xFFull
/*
* Table #39 of 240_CSR_pcslpe.xml - lpe_reg_28
* This register shows the status of the 'rx_lane_order_a,b,c,d' and 
* 'rx_lane_polarity'. These are the calculated values needed to 'Un-Swizzle' the 
* data lanes into the required order of 3,2,1,0. These values are provided to 
* the PCSs so that they can use them in the 'Auto_Polarity_Rev' mode of 
* operation. NOTE: For EDR-Only mode of operation, the 'Auto_Polarity_Rev' bit 
* in the PCS_66 Common register must be set to a '0' and then the 
* 'Cmn_Rx_Lane_Order' and 'Cmn_Rx_Lane_Polarity' values must be prorammed to the 
* desired values for correct operation. This is necessary since we do not have 
* the 8b10b speed helping to discover the Swizzle solution - it must be provided 
* by the programmer. See description below for individual field 
* definition.
*/
#define DC_LPE_REG_28                                           (DC_PCSLPE_LPE + 0x000000000140)
#define DC_LPE_REG_28_RESETCSR                                  0x2E40E402ull
#define DC_LPE_REG_28_UNUSED_31_SHIFT                           31
#define DC_LPE_REG_28_UNUSED_31_MASK                            0x1ull
#define DC_LPE_REG_28_UNUSED_31_SMASK                           0x80000000ull
#define DC_LPE_REG_28_RX_TRAINED_TIMEOUT_SHIFT                  30
#define DC_LPE_REG_28_RX_TRAINED_TIMEOUT_MASK                   0x1ull
#define DC_LPE_REG_28_RX_TRAINED_TIMEOUT_SMASK                  0x40000000ull
#define DC_LPE_REG_28_RX_TRAINED_TST_EN_SHIFT                   29
#define DC_LPE_REG_28_RX_TRAINED_TST_EN_MASK                    0x1ull
#define DC_LPE_REG_28_RX_TRAINED_TST_EN_SMASK                   0x20000000ull
#define DC_LPE_REG_28_RX_TRAINED_SHIFT                          28
#define DC_LPE_REG_28_RX_TRAINED_MASK                           0x1ull
#define DC_LPE_REG_28_RX_TRAINED_SMASK                          0x10000000ull
#define DC_LPE_REG_28_RX_LANE_ORDER_D_SHIFT                     26
#define DC_LPE_REG_28_RX_LANE_ORDER_D_MASK                      0x3ull
#define DC_LPE_REG_28_RX_LANE_ORDER_D_SMASK                     0xC000000ull
#define DC_LPE_REG_28_RX_LANE_ORDER_C_SHIFT                     24
#define DC_LPE_REG_28_RX_LANE_ORDER_C_MASK                      0x3ull
#define DC_LPE_REG_28_RX_LANE_ORDER_C_SMASK                     0x3000000ull
#define DC_LPE_REG_28_RX_LANE_ORDER_B_SHIFT                     22
#define DC_LPE_REG_28_RX_LANE_ORDER_B_MASK                      0x3ull
#define DC_LPE_REG_28_RX_LANE_ORDER_B_SMASK                     0xC00000ull
#define DC_LPE_REG_28_RX_LANE_ORDER_A_SHIFT                     20
#define DC_LPE_REG_28_RX_LANE_ORDER_A_MASK                      0x3ull
#define DC_LPE_REG_28_RX_LANE_ORDER_A_SMASK                     0x300000ull
#define DC_LPE_REG_28_RX_LANE_POLARITY_SHIFT                    16
#define DC_LPE_REG_28_RX_LANE_POLARITY_MASK                     0xFull
#define DC_LPE_REG_28_RX_LANE_POLARITY_SMASK                    0xF0000ull
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_D_SHIFT                 14
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_D_MASK                  0x3ull
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_D_SMASK                 0xC000ull
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_C_SHIFT                 12
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_C_MASK                  0x3ull
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_C_SMASK                 0x3000ull
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_B_SHIFT                 10
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_B_MASK                  0x3ull
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_B_SMASK                 0xC00ull
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_A_SHIFT                 8
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_A_MASK                  0x3ull
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_A_SMASK                 0x300ull
#define DC_LPE_REG_28_RX_RAW_LANE_POLARITY_SHIFT                4
#define DC_LPE_REG_28_RX_RAW_LANE_POLARITY_MASK                 0xFull
#define DC_LPE_REG_28_RX_RAW_LANE_POLARITY_SMASK                0xF0ull
#define DC_LPE_REG_28_DISABLE_LINK_REC_SHIFT                    3
#define DC_LPE_REG_28_DISABLE_LINK_REC_MASK                     0x1ull
#define DC_LPE_REG_28_DISABLE_LINK_REC_SMASK                    0x8ull
#define DC_LPE_REG_28_TEST_IDLE_JMP_SHIFT                       2
#define DC_LPE_REG_28_TEST_IDLE_JMP_MASK                        0x1ull
#define DC_LPE_REG_28_TEST_IDLE_JMP_SMASK                       0x4ull
#define DC_LPE_REG_28_POLARITY_SWAP_SUPPORTED_SHIFT             1
#define DC_LPE_REG_28_POLARITY_SWAP_SUPPORTED_MASK              0x1ull
#define DC_LPE_REG_28_POLARITY_SWAP_SUPPORTED_SMASK             0x2ull
#define DC_LPE_REG_28_TX_LANE_REVERSE_SUPPORTED_SHIFT           0
#define DC_LPE_REG_28_TX_LANE_REVERSE_SUPPORTED_MASK            0x1ull
#define DC_LPE_REG_28_TX_LANE_REVERSE_SUPPORTED_SMASK           0x1ull
/*
* Table #40 of 240_CSR_pcslpe.xml - lpe_reg_29
* See description below for individual field definition
*/
#define DC_LPE_REG_29                                           (DC_PCSLPE_LPE + 0x000000000148)
#define DC_LPE_REG_29_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_29_VL_FC_TIMEOUT_SHIFT                       24
#define DC_LPE_REG_29_VL_FC_TIMEOUT_MASK                        0xFFull
#define DC_LPE_REG_29_VL_FC_TIMEOUT_SMASK                       0xFF000000ull
#define DC_LPE_REG_29_UNUSED_23_21_SHIFT                        21
#define DC_LPE_REG_29_UNUSED_23_21_MASK                         0x7ull
#define DC_LPE_REG_29_UNUSED_23_21_SMASK                        0xE00000ull
#define DC_LPE_REG_29_TX_REV_LANES_DETECTED_SHIFT               20
#define DC_LPE_REG_29_TX_REV_LANES_DETECTED_MASK                0x1ull
#define DC_LPE_REG_29_TX_REV_LANES_DETECTED_SMASK               0x100000ull
#define DC_LPE_REG_29_LPE_RX_LATE_LOST_LINK_SHIFT               19
#define DC_LPE_REG_29_LPE_RX_LATE_LOST_LINK_MASK                0x1ull
#define DC_LPE_REG_29_LPE_RX_LATE_LOST_LINK_SMASK               0x80000ull
#define DC_LPE_REG_29_LPE_RX_LATE_CTRL_ERR_SHIFT                18
#define DC_LPE_REG_29_LPE_RX_LATE_CTRL_ERR_MASK                 0x1ull
#define DC_LPE_REG_29_LPE_RX_LATE_CTRL_ERR_SMASK                0x40000ull
#define DC_LPE_REG_29_UNUSED_17_SHIFT                           17
#define DC_LPE_REG_29_UNUSED_17_MASK                            0x1ull
#define DC_LPE_REG_29_UNUSED_17_SMASK                           0x20000ull
#define DC_LPE_REG_29_LPE_RX_LATE_CODE_ERR_SHIFT                16
#define DC_LPE_REG_29_LPE_RX_LATE_CODE_ERR_MASK                 0x1ull
#define DC_LPE_REG_29_LPE_RX_LATE_CODE_ERR_SMASK                0x10000ull
#define DC_LPE_REG_29_RX_ABR_ERROR_SHIFT                        8
#define DC_LPE_REG_29_RX_ABR_ERROR_MASK                         0xFFull
#define DC_LPE_REG_29_RX_ABR_ERROR_SMASK                        0xFF00ull
#define DC_LPE_REG_29_IB_STAT_FC_VIOLATION_SHIFT                0
#define DC_LPE_REG_29_IB_STAT_FC_VIOLATION_MASK                 0xFFull
#define DC_LPE_REG_29_IB_STAT_FC_VIOLATION_SMASK                0xFFull
/*
* Table #41 of 240_CSR_pcslpe.xml - lpe_reg_2A
* See description below for individual field definition
*/
#define DC_LPE_REG_2A                                           (DC_PCSLPE_LPE + 0x000000000150)
#define DC_LPE_REG_2A_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_2A_LPE_RX_MAJOR_ERROR_DISABLE_MASK_SHIFT     24
#define DC_LPE_REG_2A_LPE_RX_MAJOR_ERROR_DISABLE_MASK_MASK      0xFFull
#define DC_LPE_REG_2A_LPE_RX_MAJOR_ERROR_DISABLE_MASK_SMASK     0xFF000000ull
#define DC_LPE_REG_2A_LPE_RX_EARLY_ERROR_ENABLE_MASK_ADV_SHIFT  17
#define DC_LPE_REG_2A_LPE_RX_EARLY_ERROR_ENABLE_MASK_ADV_MASK   0x1ull
#define DC_LPE_REG_2A_LPE_RX_EARLY_ERROR_ENABLE_MASK_ADV_SMASK  0x20000ull
#define DC_LPE_REG_2A_LPE_RX_LATE_ERROR_ENABLE_MASK_SHIFT       8
#define DC_LPE_REG_2A_LPE_RX_LATE_ERROR_ENABLE_MASK_MASK        0x1FFull
#define DC_LPE_REG_2A_LPE_RX_LATE_ERROR_ENABLE_MASK_SMASK       0x1FF00ull
#define DC_LPE_REG_2A_LPE_RX_EARLY_ERROR_ENABLE_MASK_SHIFT      0
#define DC_LPE_REG_2A_LPE_RX_EARLY_ERROR_ENABLE_MASK_MASK       0xFFull
#define DC_LPE_REG_2A_LPE_RX_EARLY_ERROR_ENABLE_MASK_SMASK      0xFFull
/*
* Table #42 of 240_CSR_pcslpe.xml - lpe_reg_2B
* See description below for individual field definition
*/
#define DC_LPE_REG_2B                                           (DC_PCSLPE_LPE + 0x000000000158)
#define DC_LPE_REG_2B_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_2B_UNUSED_31_8_SHIFT                         8
#define DC_LPE_REG_2B_UNUSED_31_8_MASK                          0xFFFFFFull
#define DC_LPE_REG_2B_UNUSED_31_8_SMASK                         0xFFFFFF00ull
#define DC_LPE_REG_2B_LPE_RX_EARLY_ERROR_REPORT_MASK_SHIFT      0
#define DC_LPE_REG_2B_LPE_RX_EARLY_ERROR_REPORT_MASK_MASK       0xFFull
#define DC_LPE_REG_2B_LPE_RX_EARLY_ERROR_REPORT_MASK_SMASK      0xFFull
/*
* Table #43 of 240_CSR_pcslpe.xml - lpe_reg_2C
* See description below for individual field definition
*/
#define DC_LPE_REG_2C                                           (DC_PCSLPE_LPE + 0x000000000160)
#define DC_LPE_REG_2C_RESETCSR                                  0x00000101ull
#define DC_LPE_REG_2C_UNUSED_31_10_SHIFT                        10
#define DC_LPE_REG_2C_UNUSED_31_10_MASK                         0x3FFFFFull
#define DC_LPE_REG_2C_UNUSED_31_10_SMASK                        0xFFFFFC00ull
#define DC_LPE_REG_2C_IB_ENHANCE_MODE10_SHIFT                   8
#define DC_LPE_REG_2C_IB_ENHANCE_MODE10_MASK                    0x3ull
#define DC_LPE_REG_2C_IB_ENHANCE_MODE10_SMASK                   0x300ull
#define DC_LPE_REG_2C_UNUSED_7_2_SHIFT                          2
#define DC_LPE_REG_2C_UNUSED_7_2_MASK                           0x3Full
#define DC_LPE_REG_2C_UNUSED_7_2_SMASK                          0xFCull
#define DC_LPE_REG_2C_LPE_PKT_LENGTH_B11_DISABLE_SHIFT          1
#define DC_LPE_REG_2C_LPE_PKT_LENGTH_B11_DISABLE_MASK           0x1ull
#define DC_LPE_REG_2C_LPE_PKT_LENGTH_B11_DISABLE_SMASK          0x2ull
#define DC_LPE_REG_2C_IB_FORWARD_IN_ARM_SHIFT                   0
#define DC_LPE_REG_2C_IB_FORWARD_IN_ARM_MASK                    0x1ull
#define DC_LPE_REG_2C_IB_FORWARD_IN_ARM_SMASK                   0x1ull
/*
* Table #44 of 240_CSR_pcslpe.xml - lpe_reg_2D
* See description below for individual field definition
*/
#define DC_LPE_REG_2D                                           (DC_PCSLPE_LPE + 0x000000000168)
#define DC_LPE_REG_2D_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_2D_UNUSED_31_16_SHIFT                        16
#define DC_LPE_REG_2D_UNUSED_31_16_MASK                         0xFFFFull
#define DC_LPE_REG_2D_UNUSED_31_16_SMASK                        0xFFFF0000ull
#define DC_LPE_REG_2D_LIN_FDB_TOP_SHIFT                         0
#define DC_LPE_REG_2D_LIN_FDB_TOP_MASK                          0xFFFFull
#define DC_LPE_REG_2D_LIN_FDB_TOP_SMASK                         0xFFFFull
/*
* Table #45 of 240_CSR_pcslpe.xml - lpe_reg_2E
* See description below for individual field definition
*/
#define DC_LPE_REG_2E                                           (DC_PCSLPE_LPE + 0x000000000170)
#define DC_LPE_REG_2E_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_2E_UNUSED_31_6_SHIFT                         6
#define DC_LPE_REG_2E_UNUSED_31_6_MASK                          0x3FFFFFFull
#define DC_LPE_REG_2E_UNUSED_31_6_SMASK                         0xFFFFFFC0ull
#define DC_LPE_REG_2E_CFG_TEST_ACTIVE_SHIFT                     5
#define DC_LPE_REG_2E_CFG_TEST_ACTIVE_MASK                      0x1ull
#define DC_LPE_REG_2E_CFG_TEST_ACTIVE_SMASK                     0x20ull
#define DC_LPE_REG_2E_CFG_TEST_FRONT_PORCH_ACTIVE_SHIFT         4
#define DC_LPE_REG_2E_CFG_TEST_FRONT_PORCH_ACTIVE_MASK          0x1ull
#define DC_LPE_REG_2E_CFG_TEST_FRONT_PORCH_ACTIVE_SMASK         0x10ull
#define DC_LPE_REG_2E_CFG_TEST_BACK_PORCH_ACTIVE_SHIFT          3
#define DC_LPE_REG_2E_CFG_TEST_BACK_PORCH_ACTIVE_MASK           0x1ull
#define DC_LPE_REG_2E_CFG_TEST_BACK_PORCH_ACTIVE_SMASK          0x8ull
#define DC_LPE_REG_2E_LPE_FRONT_PORCH_INTERVAL_SHIFT            0
#define DC_LPE_REG_2E_LPE_FRONT_PORCH_INTERVAL_MASK             0x7ull
#define DC_LPE_REG_2E_LPE_FRONT_PORCH_INTERVAL_SMASK            0x7ull
/*
* Table #46 of 240_CSR_pcslpe.xml - lpe_reg_2F
* See description below for individual field definition
*/
#define DC_LPE_REG_2F                                           (DC_PCSLPE_LPE + 0x000000000178)
#define DC_LPE_REG_2F_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_2F_UNUSED_31_5_SHIFT                         5
#define DC_LPE_REG_2F_UNUSED_31_5_MASK                          0x7FFFFFFull
#define DC_LPE_REG_2F_UNUSED_31_5_SMASK                         0xFFFFFFE0ull
#define DC_LPE_REG_2F_SPEED_DISABLE_MASK_SHIFT                  0
#define DC_LPE_REG_2F_SPEED_DISABLE_MASK_MASK                   0x1Full
#define DC_LPE_REG_2F_SPEED_DISABLE_MASK_SMASK                  0x1Full
/*
* Table #47 of 240_CSR_pcslpe.xml - lpe_reg_30
* See description below for individual field definition
*/
#define DC_LPE_REG_30                                           (DC_PCSLPE_LPE + 0x000000000180)
#define DC_LPE_REG_30_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_30_UNUSED_31_11_SHIFT                        11
#define DC_LPE_REG_30_UNUSED_31_11_MASK                         0x1FFFFFull
#define DC_LPE_REG_30_UNUSED_31_11_SMASK                        0xFFFFF800ull
#define DC_LPE_REG_30_CFG_TEST_FAILED_SHIFT                     10
#define DC_LPE_REG_30_CFG_TEST_FAILED_MASK                      0x1ull
#define DC_LPE_REG_30_CFG_TEST_FAILED_SMASK                     0x400ull
#define DC_LPE_REG_30_EN_CFG_TEST_FAILED_SHIFT                  9
#define DC_LPE_REG_30_EN_CFG_TEST_FAILED_MASK                   0x1ull
#define DC_LPE_REG_30_EN_CFG_TEST_FAILED_SMASK                  0x200ull
#define DC_LPE_REG_30_FORCE_SPEED_DOWN_GRADE_SHIFT              8
#define DC_LPE_REG_30_FORCE_SPEED_DOWN_GRADE_MASK               0x1ull
#define DC_LPE_REG_30_FORCE_SPEED_DOWN_GRADE_SMASK              0x100ull
#define DC_LPE_REG_30_UNUSED_7_5_SHIFT                          5
#define DC_LPE_REG_30_UNUSED_7_5_MASK                           0x7ull
#define DC_LPE_REG_30_UNUSED_7_5_SMASK                          0xE0ull
#define DC_LPE_REG_30_TS1_LOCK_FOREVER_SHIFT                    4
#define DC_LPE_REG_30_TS1_LOCK_FOREVER_MASK                     0x1ull
#define DC_LPE_REG_30_TS1_LOCK_FOREVER_SMASK                    0x10ull
#define DC_LPE_REG_30_DEBOUNCE_SEND_IDLE_ENABLE_SHIFT           3
#define DC_LPE_REG_30_DEBOUNCE_SEND_IDLE_ENABLE_MASK            0x1ull
#define DC_LPE_REG_30_DEBOUNCE_SEND_IDLE_ENABLE_SMASK           0x8ull
#define DC_LPE_REG_30_ENBL_LINKUP_TO_DISABLE_SHIFT              2
#define DC_LPE_REG_30_ENBL_LINKUP_TO_DISABLE_MASK               0x1ull
#define DC_LPE_REG_30_ENBL_LINKUP_TO_DISABLE_SMASK              0x4ull
#define DC_LPE_REG_30_DSBL_TS3_TRAINING_SHIFT                   1
#define DC_LPE_REG_30_DSBL_TS3_TRAINING_MASK                    0x1ull
#define DC_LPE_REG_30_DSBL_TS3_TRAINING_SMASK                   0x2ull
#define DC_LPE_REG_30_DSBL_LINK_RECOVERY_SHIFT                  0
#define DC_LPE_REG_30_DSBL_LINK_RECOVERY_MASK                   0x1ull
#define DC_LPE_REG_30_DSBL_LINK_RECOVERY_SMASK                  0x1ull
/*
* Table #48 of 240_CSR_pcslpe.xml - lpe_reg_31
* See description below for individual field definition
*/
#define DC_LPE_REG_31                                           (DC_PCSLPE_LPE + 0x000000000188)
#define DC_LPE_REG_31_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_31_UNUSED_31_0_SHIFT                         0
#define DC_LPE_REG_31_UNUSED_31_0_MASK                          0xFFFFFFFFull
#define DC_LPE_REG_31_UNUSED_31_0_SMASK                         0xFFFFFFFFull
/*
* Table #49 of 240_CSR_pcslpe.xml - lpe_reg_32
* See description below for individual field definition
*/
#define DC_LPE_REG_32                                           (DC_PCSLPE_LPE + 0x000000000190)
#define DC_LPE_REG_32_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_32_UNUSED_31_2_SHIFT                         2
#define DC_LPE_REG_32_UNUSED_31_2_MASK                          0x3FFFFFFFull
#define DC_LPE_REG_32_UNUSED_31_2_SMASK                         0xFFFFFFFCull
#define DC_LPE_REG_32_RX_FORCE_MAJOR_ERROR_SHIFT                0
#define DC_LPE_REG_32_RX_FORCE_MAJOR_ERROR_MASK                 0x3ull
#define DC_LPE_REG_32_RX_FORCE_MAJOR_ERROR_SMASK                0x3ull
/*
* Table #50 of 240_CSR_pcslpe.xml - lpe_reg_33
* See description below for individual field definition
*/
#define DC_LPE_REG_33                                           (DC_PCSLPE_LPE + 0x000000000198)
#define DC_LPE_REG_33_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_33_UNUSED_31_9_SHIFT                         9
#define DC_LPE_REG_33_UNUSED_31_9_MASK                          0x7FFFFFull
#define DC_LPE_REG_33_UNUSED_31_9_SMASK                         0xFFFFFE00ull
#define DC_LPE_REG_33_VL_PKT_DISCARD_MASK_SHIFT                 0
#define DC_LPE_REG_33_VL_PKT_DISCARD_MASK_MASK                  0x1FFull
#define DC_LPE_REG_33_VL_PKT_DISCARD_MASK_SMASK                 0x1FFull
/*
* Table #51 of 240_CSR_pcslpe.xml - lpe_reg_36
* See description below for individual field definition
*/
#define DC_LPE_REG_36                                           (DC_PCSLPE_LPE + 0x0000000001B0)
#define DC_LPE_REG_36_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_36_UNUSED_31_0_SHIFT                         0
#define DC_LPE_REG_36_UNUSED_31_0_MASK                          0xFFFFFFFFull
#define DC_LPE_REG_36_UNUSED_31_0_SMASK                         0xFFFFFFFFull
/*
* Table #52 of 240_CSR_pcslpe.xml - lpe_reg_37
* See description below for individual field definition
*/
#define DC_LPE_REG_37                                           (DC_PCSLPE_LPE + 0x0000000001B8)
#define DC_LPE_REG_37_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_37_UNUSED_31_27_SHIFT                        27
#define DC_LPE_REG_37_UNUSED_31_27_MASK                         0x1Full
#define DC_LPE_REG_37_UNUSED_31_27_SMASK                        0xF8000000ull
#define DC_LPE_REG_37_FREEZE_LAST_SHIFT                         26
#define DC_LPE_REG_37_FREEZE_LAST_MASK                          0x1ull
#define DC_LPE_REG_37_FREEZE_LAST_SMASK                         0x4000000ull
#define DC_LPE_REG_37_CLEAR_LAST_SHIFT                          25
#define DC_LPE_REG_37_CLEAR_LAST_MASK                           0x1ull
#define DC_LPE_REG_37_CLEAR_LAST_SMASK                          0x2000000ull
#define DC_LPE_REG_37_CLEAR_FIRST_SHIFT                         24
#define DC_LPE_REG_37_CLEAR_FIRST_MASK                          0x1ull
#define DC_LPE_REG_37_CLEAR_FIRST_SMASK                         0x1000000ull
#define DC_LPE_REG_37_ERROR_STATE_SHIFT                         0
#define DC_LPE_REG_37_ERROR_STATE_MASK                          0xFFFFFFull
#define DC_LPE_REG_37_ERROR_STATE_SMASK                         0xFFFFFFull
/*
* Table #53 of 240_CSR_pcslpe.xml - lpe_reg_38
* See description below for individual field definition
*/
#define DC_LPE_REG_38                                           (DC_PCSLPE_LPE + 0x0000000001C0)
#define DC_LPE_REG_38_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_38_FIRST_ERR_LRH_0_SHIFT                     0
#define DC_LPE_REG_38_FIRST_ERR_LRH_0_MASK                      0xFFFFFFFFull
#define DC_LPE_REG_38_FIRST_ERR_LRH_0_SMASK                     0xFFFFFFFFull
/*
* Table #54 of 240_CSR_pcslpe.xml - lpe_reg_39
* See description below for individual field definition
*/
#define DC_LPE_REG_39                                           (DC_PCSLPE_LPE + 0x0000000001C8)
#define DC_LPE_REG_39_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_39_FIRST_ERR_LRH_1_SHIFT                     0
#define DC_LPE_REG_39_FIRST_ERR_LRH_1_MASK                      0xFFFFFFFFull
#define DC_LPE_REG_39_FIRST_ERR_LRH_1_SMASK                     0xFFFFFFFFull
/*
* Table #55 of 240_CSR_pcslpe.xml - lpe_reg_3A
* See description below for individual field definition
*/
#define DC_LPE_REG_3A                                           (DC_PCSLPE_LPE + 0x0000000001D0)
#define DC_LPE_REG_3A_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_3A_FIRST_ERR_BTH_0_SHIFT                     0
#define DC_LPE_REG_3A_FIRST_ERR_BTH_0_MASK                      0xFFFFFFFFull
#define DC_LPE_REG_3A_FIRST_ERR_BTH_0_SMASK                     0xFFFFFFFFull
/*
* Table #56 of 240_CSR_pcslpe.xml - lpe_reg_3B
* See description below for individual field definition
*/
#define DC_LPE_REG_3B                                           (DC_PCSLPE_LPE + 0x0000000001D8)
#define DC_LPE_REG_3B_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_3B_FIRST_ERR_BTH_1_SHIFT                     0
#define DC_LPE_REG_3B_FIRST_ERR_BTH_1_MASK                      0xFFFFFFFFull
#define DC_LPE_REG_3B_FIRST_ERR_BTH_1_SMASK                     0xFFFFFFFFull
/*
* Table #57 of 240_CSR_pcslpe.xml - lpe_reg_3C
* See description below for individual field definition
*/
#define DC_LPE_REG_3C                                           (DC_PCSLPE_LPE + 0x0000000001E0)
#define DC_LPE_REG_3C_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_3C_LAST_ERR_LRH_0_SHIFT                      0
#define DC_LPE_REG_3C_LAST_ERR_LRH_0_MASK                       0xFFFFFFFFull
#define DC_LPE_REG_3C_LAST_ERR_LRH_0_SMASK                      0xFFFFFFFFull
/*
* Table #58 of 240_CSR_pcslpe.xml - lpe_reg_3D
* See description below for individual field definition
*/
#define DC_LPE_REG_3D                                           (DC_PCSLPE_LPE + 0x0000000001E8)
#define DC_LPE_REG_3D_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_3D_LAST_ERR_LRH_1_SHIFT                      0
#define DC_LPE_REG_3D_LAST_ERR_LRH_1_MASK                       0xFFFFFFFFull
#define DC_LPE_REG_3D_LAST_ERR_LRH_1_SMASK                      0xFFFFFFFFull
/*
* Table #59 of 240_CSR_pcslpe.xml - lpe_reg_3E
* See description below for individual field definition
*/
#define DC_LPE_REG_3E                                           (DC_PCSLPE_LPE + 0x0000000001F0)
#define DC_LPE_REG_3E_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_3E_LAST_ERR_BTH_0_SHIFT                      0
#define DC_LPE_REG_3E_LAST_ERR_BTH_0_MASK                       0xFFFFFFFFull
#define DC_LPE_REG_3E_LAST_ERR_BTH_0_SMASK                      0xFFFFFFFFull
/*
* Table #60 of 240_CSR_pcslpe.xml - lpe_reg_3F
* See description below for individual field definition
*/
#define DC_LPE_REG_3F                                           (DC_PCSLPE_LPE + 0x0000000001F8)
#define DC_LPE_REG_3F_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_3F_LAST_ERR_BTH_1_SHIFT                      0
#define DC_LPE_REG_3F_LAST_ERR_BTH_1_MASK                       0xFFFFFFFFull
#define DC_LPE_REG_3F_LAST_ERR_BTH_1_SMASK                      0xFFFFFFFFull
/*
* Table #61 of 240_CSR_pcslpe.xml - lpe_reg_40
* See description below for individual field definition
*/
#define DC_LPE_REG_40                                           (DC_PCSLPE_LPE + 0x000000000200)
#define DC_LPE_REG_40_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_40_UNUSED_31_10_SHIFT                        10
#define DC_LPE_REG_40_UNUSED_31_10_MASK                         0x3FFFFFull
#define DC_LPE_REG_40_UNUSED_31_10_SMASK                        0xFFFFFC00ull
#define DC_LPE_REG_40_TRACE_BUFFER_FROZEN_SHIFT                 9
#define DC_LPE_REG_40_TRACE_BUFFER_FROZEN_MASK                  0x1ull
#define DC_LPE_REG_40_TRACE_BUFFER_FROZEN_SMASK                 0x200ull
#define DC_LPE_REG_40_SM_MATCH_INT_SHIFT                        8
#define DC_LPE_REG_40_SM_MATCH_INT_MASK                         0x1ull
#define DC_LPE_REG_40_SM_MATCH_INT_SMASK                        0x100ull
#define DC_LPE_REG_40_PORT_SM_MATCH_REG_SHIFT                   5
#define DC_LPE_REG_40_PORT_SM_MATCH_REG_MASK                    0x7ull
#define DC_LPE_REG_40_PORT_SM_MATCH_REG_SMASK                   0xE0ull
#define DC_LPE_REG_40_TRAINING_SM_MATCH_REG_SHIFT               0
#define DC_LPE_REG_40_TRAINING_SM_MATCH_REG_MASK                0x1Full
#define DC_LPE_REG_40_TRAINING_SM_MATCH_REG_SMASK               0x1Full
/*
* Table #62 of 240_CSR_pcslpe.xml - lpe_reg_41
* See description below for individual field definition
*/
#define DC_LPE_REG_41                                           (DC_PCSLPE_LPE + 0x000000000208)
#define DC_LPE_REG_41_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_41_UNUSED_31_13_SHIFT                        13
#define DC_LPE_REG_41_UNUSED_31_13_MASK                         0x7FFFFull
#define DC_LPE_REG_41_UNUSED_31_13_SMASK                        0xFFFFE000ull
#define DC_LPE_REG_41_TRACE_BUFFER_WRAP_EN_SHIFT                12
#define DC_LPE_REG_41_TRACE_BUFFER_WRAP_EN_MASK                 0x1ull
#define DC_LPE_REG_41_TRACE_BUFFER_WRAP_EN_SMASK                0x1000ull
#define DC_LPE_REG_41_UNUSED_11_10_SHIFT                        10
#define DC_LPE_REG_41_UNUSED_11_10_MASK                         0x3ull
#define DC_LPE_REG_41_UNUSED_11_10_SMASK                        0xC00ull
#define DC_LPE_REG_41_TRACE_BUFFER_LD_DIS_MASK_SHIFT            4
#define DC_LPE_REG_41_TRACE_BUFFER_LD_DIS_MASK_MASK             0x3Full
#define DC_LPE_REG_41_TRACE_BUFFER_LD_DIS_MASK_SMASK            0x3F0ull
#define DC_LPE_REG_41_RESET_TRACE_BUFFER_SHIFT                  3
#define DC_LPE_REG_41_RESET_TRACE_BUFFER_MASK                   0x1ull
#define DC_LPE_REG_41_RESET_TRACE_BUFFER_SMASK                  0x8ull
#define DC_LPE_REG_41_INT_ON_MATCH_SHIFT                        2
#define DC_LPE_REG_41_INT_ON_MATCH_MASK                         0x1ull
#define DC_LPE_REG_41_INT_ON_MATCH_SMASK                        0x4ull
#define DC_LPE_REG_41_FREEZE_ON_MATCH_SHIFT                     1
#define DC_LPE_REG_41_FREEZE_ON_MATCH_MASK                      0x1ull
#define DC_LPE_REG_41_FREEZE_ON_MATCH_SMASK                     0x2ull
#define DC_LPE_REG_41_FREEZE_TRACE_BUFFER_SHIFT                 0
#define DC_LPE_REG_41_FREEZE_TRACE_BUFFER_MASK                  0x1ull
#define DC_LPE_REG_41_FREEZE_TRACE_BUFFER_SMASK                 0x1ull
/*
* Table #63 of 240_CSR_pcslpe.xml - lpe_reg_42
* See description below for individual field definition
*/
#define DC_LPE_REG_42                                           (DC_PCSLPE_LPE + 0x000000000210)
#define DC_LPE_REG_42_RESETCSR                                  0x22000000ull
#define DC_LPE_REG_42_TB_PORT_STATE_SHIFT                       29
#define DC_LPE_REG_42_TB_PORT_STATE_MASK                        0x7ull
#define DC_LPE_REG_42_TB_PORT_STATE_SMASK                       0xE0000000ull
#define DC_LPE_REG_42_TB_TRAINING_STATE_SHIFT                   24
#define DC_LPE_REG_42_TB_TRAINING_STATE_MASK                    0x1Full
#define DC_LPE_REG_42_TB_TRAINING_STATE_SMASK                   0x1F000000ull
#define DC_LPE_REG_42_UNUSED_23_16_SHIFT                        16
#define DC_LPE_REG_42_UNUSED_23_16_MASK                         0xFFull
#define DC_LPE_REG_42_UNUSED_23_16_SMASK                        0xFF0000ull
#define DC_LPE_REG_42_SPARE_RA_SHIFT                            13
#define DC_LPE_REG_42_SPARE_RA_MASK                             0x7ull
#define DC_LPE_REG_42_SPARE_RA_SMASK                            0xE000ull
#define DC_LPE_REG_42_LINK_UP_GOT_TS_SHIFT                      12
#define DC_LPE_REG_42_LINK_UP_GOT_TS_MASK                       0x1ull
#define DC_LPE_REG_42_LINK_UP_GOT_TS_SMASK                      0x1000ull
#define DC_LPE_REG_42_TS3_OS_DETECTED_SHIFT                     8
#define DC_LPE_REG_42_TS3_OS_DETECTED_MASK                      0xFull
#define DC_LPE_REG_42_TS3_OS_DETECTED_SMASK                     0xF00ull
#define DC_LPE_REG_42_TS2_OS_DETECTED_SHIFT                     4
#define DC_LPE_REG_42_TS2_OS_DETECTED_MASK                      0xFull
#define DC_LPE_REG_42_TS2_OS_DETECTED_SMASK                     0xF0ull
#define DC_LPE_REG_42_TS1_OS_DETECTED_SHIFT                     0
#define DC_LPE_REG_42_TS1_OS_DETECTED_MASK                      0xFull
#define DC_LPE_REG_42_TS1_OS_DETECTED_SMASK                     0xFull
/*
* Table #64 of 240_CSR_pcslpe.xml - lpe_reg_43
* See description below for individual field definition
*/
#define DC_LPE_REG_43                                           (DC_PCSLPE_LPE + 0x000000000218)
#define DC_LPE_REG_43_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_43_TB_LINK_MAJOR_ERRORS_SHIFT                24
#define DC_LPE_REG_43_TB_LINK_MAJOR_ERRORS_MASK                 0xFFull
#define DC_LPE_REG_43_TB_LINK_MAJOR_ERRORS_SMASK                0xFF000000ull
#define DC_LPE_REG_43_TB_FC_ERRORS_SHIFT                        21
#define DC_LPE_REG_43_TB_FC_ERRORS_MASK                         0x7ull
#define DC_LPE_REG_43_TB_FC_ERRORS_SMASK                        0xE00000ull
#define DC_LPE_REG_43_TB_PKT_LATE_ERRORS_SHIFT                  12
#define DC_LPE_REG_43_TB_PKT_LATE_ERRORS_MASK                   0x1FFull
#define DC_LPE_REG_43_TB_PKT_LATE_ERRORS_SMASK                  0x1FF000ull
#define DC_LPE_REG_43_TB_PKT_EARLY_ERRORS_SHIFT                 0
#define DC_LPE_REG_43_TB_PKT_EARLY_ERRORS_MASK                  0xFFFull
#define DC_LPE_REG_43_TB_PKT_EARLY_ERRORS_SMASK                 0xFFFull
/*
* Table #65 of 240_CSR_pcslpe.xml - lpe_reg_44
* See description below for individual field definition
*/
#define DC_LPE_REG_44                                           (DC_PCSLPE_LPE + 0x000000000220)
#define DC_LPE_REG_44_RESETCSR                                  0x00000006ull
#define DC_LPE_REG_44_TB_TIME_STAMP_SHIFT                       0
#define DC_LPE_REG_44_TB_TIME_STAMP_MASK                        0xFFFFFFFFull
#define DC_LPE_REG_44_TB_TIME_STAMP_SMASK                       0xFFFFFFFFull
/*
* Table #66 of 240_CSR_pcslpe.xml - lpe_reg_48
* See description below for individual field definition
*/
#define DC_LPE_REG_48                                           (DC_PCSLPE_LPE + 0x000000000240)
#define DC_LPE_REG_48_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_48_UNUSED_31_7_SHIFT                         7
#define DC_LPE_REG_48_UNUSED_31_7_MASK                          0x1FFFFFFull
#define DC_LPE_REG_48_UNUSED_31_7_SMASK                         0xFFFFFF80ull
#define DC_LPE_REG_48_ENABLE_SAMPLE_PERIOD_SHIFT                6
#define DC_LPE_REG_48_ENABLE_SAMPLE_PERIOD_MASK                 0x1ull
#define DC_LPE_REG_48_ENABLE_SAMPLE_PERIOD_SMASK                0x40ull
#define DC_LPE_REG_48_FREEZE_IDLE_CNT_SHIFT                     5
#define DC_LPE_REG_48_FREEZE_IDLE_CNT_MASK                      0x1ull
#define DC_LPE_REG_48_FREEZE_IDLE_CNT_SMASK                     0x20ull
#define DC_LPE_REG_48_CLEAR_IDLE_CNT_SHIFT                      4
#define DC_LPE_REG_48_CLEAR_IDLE_CNT_MASK                       0x1ull
#define DC_LPE_REG_48_CLEAR_IDLE_CNT_SMASK                      0x10ull
#define DC_LPE_REG_48_IDLE_SAMPLE_PERIOD_SHIFT                  0
#define DC_LPE_REG_48_IDLE_SAMPLE_PERIOD_MASK                   0xFull
#define DC_LPE_REG_48_IDLE_SAMPLE_PERIOD_SMASK                  0xFull
/*
* Table #67 of 240_CSR_pcslpe.xml - lpe_reg_49
* See description below for individual field definition
*/
#define DC_LPE_REG_49                                           (DC_PCSLPE_LPE + 0x000000000248)
#define DC_LPE_REG_49_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_49_RX_IDLE_CNT_UPPER_SHIFT                   0
#define DC_LPE_REG_49_RX_IDLE_CNT_UPPER_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_49_RX_IDLE_CNT_UPPER_SMASK                   0xFFFFFFFFull
/*
* Table #68 of 240_CSR_pcslpe.xml - lpe_reg_4A
* See description below for individual field definition
*/
#define DC_LPE_REG_4A                                           (DC_PCSLPE_LPE + 0x000000000250)
#define DC_LPE_REG_4A_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_4A_RX_IDLE_CNT_LOWER_SHIFT                   0
#define DC_LPE_REG_4A_RX_IDLE_CNT_LOWER_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_4A_RX_IDLE_CNT_LOWER_SMASK                   0xFFFFFFFFull
/*
* Table #69 of 240_CSR_pcslpe.xml - lpe_reg_4B
* See description below for individual field definition
*/
#define DC_LPE_REG_4B                                           (DC_PCSLPE_LPE + 0x000000000258)
#define DC_LPE_REG_4B_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_4B_TX_IDLE_CNT_UPPER_SHIFT                   0
#define DC_LPE_REG_4B_TX_IDLE_CNT_UPPER_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_4B_TX_IDLE_CNT_UPPER_SMASK                   0xFFFFFFFFull
/*
* Table #70 of 240_CSR_pcslpe.xml - lpe_reg_4C
* See description below for individual field definition
*/
#define DC_LPE_REG_4C                                           (DC_PCSLPE_LPE + 0x000000000260)
#define DC_LPE_REG_4C_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_4C_TX_IDLE_CNT_LOWER_SHIFT                   0
#define DC_LPE_REG_4C_TX_IDLE_CNT_LOWER_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_4C_TX_IDLE_CNT_LOWER_SMASK                   0xFFFFFFFFull
/*
* Table #71 of 240_CSR_pcslpe.xml - lpe_reg_50
* See description below for individual field definition
*/
#define DC_LPE_REG_50                                           (DC_PCSLPE_LPE + 0x000000000280)
#define DC_LPE_REG_50_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_50_UNUSED_31_SHIFT                           31
#define DC_LPE_REG_50_UNUSED_31_MASK                            0x1ull
#define DC_LPE_REG_50_UNUSED_31_SMASK                           0x80000000ull
#define DC_LPE_REG_50_REG_0X40_INT_SHIFT                        30
#define DC_LPE_REG_50_REG_0X40_INT_MASK                         0x1ull
#define DC_LPE_REG_50_REG_0X40_INT_SMASK                        0x40000000ull
#define DC_LPE_REG_50_REG_0X30_INT_SHIFT                        29
#define DC_LPE_REG_50_REG_0X30_INT_MASK                         0x1ull
#define DC_LPE_REG_50_REG_0X30_INT_SMASK                        0x20000000ull
#define DC_LPE_REG_50_REG_0X2E_INT_SHIFT                        27
#define DC_LPE_REG_50_REG_0X2E_INT_MASK                         0x3ull
#define DC_LPE_REG_50_REG_0X2E_INT_SMASK                        0x18000000ull
#define DC_LPE_REG_50_REG_0X28_INT_SHIFT                        25
#define DC_LPE_REG_50_REG_0X28_INT_MASK                         0x3ull
#define DC_LPE_REG_50_REG_0X28_INT_SMASK                        0x6000000ull
#define DC_LPE_REG_50_REG_0X19_INT_SHIFT                        24
#define DC_LPE_REG_50_REG_0X19_INT_MASK                         0x1ull
#define DC_LPE_REG_50_REG_0X19_INT_SMASK                        0x1000000ull
#define DC_LPE_REG_50_REG_0X14_INT_SHIFT                        20
#define DC_LPE_REG_50_REG_0X14_INT_MASK                         0xFull
#define DC_LPE_REG_50_REG_0X14_INT_SMASK                        0xF00000ull
#define DC_LPE_REG_50_UNUSED_19_17_SHIFT                        17
#define DC_LPE_REG_50_UNUSED_19_17_MASK                         0x7ull
#define DC_LPE_REG_50_UNUSED_19_17_SMASK                        0xE0000ull
#define DC_LPE_REG_50_REG_0X0F_INT_SHIFT                        12
#define DC_LPE_REG_50_REG_0X0F_INT_MASK                         0x1Full
#define DC_LPE_REG_50_REG_0X0F_INT_SMASK                        0x1F000ull
#define DC_LPE_REG_50_UNUSED_11_10_SHIFT                        10
#define DC_LPE_REG_50_UNUSED_11_10_MASK                         0x3ull
#define DC_LPE_REG_50_UNUSED_11_10_SMASK                        0xC00ull
#define DC_LPE_REG_50_REG_0X0C_INT_SHIFT                        9
#define DC_LPE_REG_50_REG_0X0C_INT_MASK                         0x1ull
#define DC_LPE_REG_50_REG_0X0C_INT_SMASK                        0x200ull
#define DC_LPE_REG_50_REG_0X0A_INT_SHIFT                        8
#define DC_LPE_REG_50_REG_0X0A_INT_MASK                         0x1ull
#define DC_LPE_REG_50_REG_0X0A_INT_SMASK                        0x100ull
#define DC_LPE_REG_50_REG_0X01_INT_SHIFT                        0
#define DC_LPE_REG_50_REG_0X01_INT_MASK                         0xFFull
#define DC_LPE_REG_50_REG_0X01_INT_SMASK                        0xFFull
/*
* Table #72 of 240_CSR_pcslpe.xml - lpe_reg_51
* See description below for individual field definition
*/
#define DC_LPE_REG_51                                           (DC_PCSLPE_LPE + 0x000000000288)
#define DC_LPE_REG_51_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_51_TS_T_OS_DETECTED_INT_SHIFT                28
#define DC_LPE_REG_51_TS_T_OS_DETECTED_INT_MASK                 0xFull
#define DC_LPE_REG_51_TS_T_OS_DETECTED_INT_SMASK                0xF0000000ull
#define DC_LPE_REG_51_TS12_OS_ERR_DETECTED_INT_SHIFT            24
#define DC_LPE_REG_51_TS12_OS_ERR_DETECTED_INT_MASK             0xFull
#define DC_LPE_REG_51_TS12_OS_ERR_DETECTED_INT_SMASK            0xF000000ull
#define DC_LPE_REG_51_TS2_SEQ_DETECTED_INT_SHIFT                20
#define DC_LPE_REG_51_TS2_SEQ_DETECTED_INT_MASK                 0xFull
#define DC_LPE_REG_51_TS2_SEQ_DETECTED_INT_SMASK                0xF00000ull
#define DC_LPE_REG_51_TS1_SEQ_DETECTED_INT_SHIFT                16
#define DC_LPE_REG_51_TS1_SEQ_DETECTED_INT_MASK                 0xFull
#define DC_LPE_REG_51_TS1_SEQ_DETECTED_INT_SMASK                0xF0000ull
#define DC_LPE_REG_51_SKP_OS_DETECTED_INT_SHIFT                 12
#define DC_LPE_REG_51_SKP_OS_DETECTED_INT_MASK                  0xFull
#define DC_LPE_REG_51_SKP_OS_DETECTED_INT_SMASK                 0xF000ull
#define DC_LPE_REG_51_TS3_OS_DETECTED_INT_SHIFT                 8
#define DC_LPE_REG_51_TS3_OS_DETECTED_INT_MASK                  0xFull
#define DC_LPE_REG_51_TS3_OS_DETECTED_INT_SMASK                 0xF00ull
#define DC_LPE_REG_51_TS2_OS_DETECTED_INT_SHIFT                 4
#define DC_LPE_REG_51_TS2_OS_DETECTED_INT_MASK                  0xFull
#define DC_LPE_REG_51_TS2_OS_DETECTED_INT_SMASK                 0xF0ull
#define DC_LPE_REG_51_TS1_OS_DETECTED_INT_SHIFT                 0
#define DC_LPE_REG_51_TS1_OS_DETECTED_INT_MASK                  0xFull
#define DC_LPE_REG_51_TS1_OS_DETECTED_INT_SMASK                 0xFull
/*
* Table #73 of 240_CSR_pcslpe.xml - lpe_reg_52
* See description below for individual field definition
*/
#define DC_LPE_REG_52                                           (DC_PCSLPE_LPE + 0x000000000290)
#define DC_LPE_REG_52_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_52_UNUSED_31_22_SHIFT                        22
#define DC_LPE_REG_52_UNUSED_31_22_MASK                         0x3FFull
#define DC_LPE_REG_52_UNUSED_31_22_SMASK                        0xFFC00000ull
#define DC_LPE_REG_52_TX_REV_LANES_DETECTED_INT_SHIFT           20
#define DC_LPE_REG_52_TX_REV_LANES_DETECTED_INT_MASK            0x1ull
#define DC_LPE_REG_52_TX_REV_LANES_DETECTED_INT_SMASK           0x100000ull
#define DC_LPE_REG_52_LPE_RX_LATE_LOST_LINK_INT_SHIFT           19
#define DC_LPE_REG_52_LPE_RX_LATE_LOST_LINK_INT_MASK            0x1ull
#define DC_LPE_REG_52_LPE_RX_LATE_LOST_LINK_INT_SMASK           0x80000ull
#define DC_LPE_REG_52_LPE_RX_LATE_UNEXPECTED_CHAR_ERR_INT_SHIFT 18
#define DC_LPE_REG_52_LPE_RX_LATE_UNEXPECTED_CHAR_ERR_INT_MASK  0x1ull
#define DC_LPE_REG_52_LPE_RX_LATE_UNEXPECTED_CHAR_ERR_INT_SMASK 0x40000ull
#define DC_LPE_REG_52_UNUSED_17_SHIFT                           17
#define DC_LPE_REG_52_UNUSED_17_MASK                            0x1ull
#define DC_LPE_REG_52_UNUSED_17_SMASK                           0x20000ull
#define DC_LPE_REG_52_LPE_RX_LATE_CODE_ERR_INT_SHIFT            16
#define DC_LPE_REG_52_LPE_RX_LATE_CODE_ERR_INT_MASK             0x1ull
#define DC_LPE_REG_52_LPE_RX_LATE_CODE_ERR_INT_SMASK            0x10000ull
#define DC_LPE_REG_52_RX_ABR_ERROR_INT_SHIFT                    8
#define DC_LPE_REG_52_RX_ABR_ERROR_INT_MASK                     0xFFull
#define DC_LPE_REG_52_RX_ABR_ERROR_INT_SMASK                    0xFF00ull
#define DC_LPE_REG_52_IB_STAT_FC_VIOLATION_INT_SHIFT            0
#define DC_LPE_REG_52_IB_STAT_FC_VIOLATION_INT_MASK             0xFFull
#define DC_LPE_REG_52_IB_STAT_FC_VIOLATION_INT_SMASK            0xFFull
/*
* Table #74 of 240_CSR_pcslpe.xml - lpe_reg_54
* See description below for individual field definition
*/
#define DC_LPE_REG_54                                           (DC_PCSLPE_LPE + 0x0000000002A0)
#define DC_LPE_REG_54_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_54_UNUSED_31_SHIFT                           31
#define DC_LPE_REG_54_UNUSED_31_MASK                            0x1ull
#define DC_LPE_REG_54_UNUSED_31_SMASK                           0x80000000ull
#define DC_LPE_REG_54_REG_0X40_INT_MASK_SHIFT                   30
#define DC_LPE_REG_54_REG_0X40_INT_MASK_MASK                    0x1ull
#define DC_LPE_REG_54_REG_0X40_INT_MASK_SMASK                   0x40000000ull
#define DC_LPE_REG_54_REG_0X2F_INT_MASK_SHIFT                   28
#define DC_LPE_REG_54_REG_0X2F_INT_MASK_MASK                    0x3ull
#define DC_LPE_REG_54_REG_0X2F_INT_MASK_SMASK                   0x30000000ull
#define DC_LPE_REG_54_UNUSED_27_26_SHIFT                        26
#define DC_LPE_REG_54_UNUSED_27_26_MASK                         0x3ull
#define DC_LPE_REG_54_UNUSED_27_26_SMASK                        0xC000000ull
#define DC_LPE_REG_54_REG_0X28_INT_MASK_SHIFT                   25
#define DC_LPE_REG_54_REG_0X28_INT_MASK_MASK                    0x1ull
#define DC_LPE_REG_54_REG_0X28_INT_MASK_SMASK                   0x2000000ull
#define DC_LPE_REG_54_REG_0X19_INT_MASK_SHIFT                   24
#define DC_LPE_REG_54_REG_0X19_INT_MASK_MASK                    0x1ull
#define DC_LPE_REG_54_REG_0X19_INT_MASK_SMASK                   0x1000000ull
#define DC_LPE_REG_54_REG_0X14_INT_MASK_SHIFT                   20
#define DC_LPE_REG_54_REG_0X14_INT_MASK_MASK                    0xFull
#define DC_LPE_REG_54_REG_0X14_INT_MASK_SMASK                   0xF00000ull
#define DC_LPE_REG_54_UNUSED_19_17_SHIFT                        17
#define DC_LPE_REG_54_UNUSED_19_17_MASK                         0x7ull
#define DC_LPE_REG_54_UNUSED_19_17_SMASK                        0xE0000ull
#define DC_LPE_REG_54_REG_0X0F_INT_MASK_SHIFT                   12
#define DC_LPE_REG_54_REG_0X0F_INT_MASK_MASK                    0x1Full
#define DC_LPE_REG_54_REG_0X0F_INT_MASK_SMASK                   0x1F000ull
#define DC_LPE_REG_54_UNUSED_11_10_SHIFT                        10
#define DC_LPE_REG_54_UNUSED_11_10_MASK                         0x3ull
#define DC_LPE_REG_54_UNUSED_11_10_SMASK                        0xC00ull
#define DC_LPE_REG_54_REG_0X0C_INT_MASK_SHIFT                   9
#define DC_LPE_REG_54_REG_0X0C_INT_MASK_MASK                    0x1ull
#define DC_LPE_REG_54_REG_0X0C_INT_MASK_SMASK                   0x200ull
#define DC_LPE_REG_54_REG_0X0A_INT_MASK_SHIFT                   8
#define DC_LPE_REG_54_REG_0X0A_INT_MASK_MASK                    0x1ull
#define DC_LPE_REG_54_REG_0X0A_INT_MASK_SMASK                   0x100ull
#define DC_LPE_REG_54_REG_0X01_INT_MASK_SHIFT                   0
#define DC_LPE_REG_54_REG_0X01_INT_MASK_MASK                    0xFFull
#define DC_LPE_REG_54_REG_0X01_INT_MASK_SMASK                   0xFFull
/*
* Table #75 of 240_CSR_pcslpe.xml - lpe_reg_55
* See description below for individual field definition
*/
#define DC_LPE_REG_55                                           (DC_PCSLPE_LPE + 0x0000000002A8)
#define DC_LPE_REG_55_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_55_TS_T_OS_DETECTED_INT_MASK_SHIFT           28
#define DC_LPE_REG_55_TS_T_OS_DETECTED_INT_MASK_MASK            0xFull
#define DC_LPE_REG_55_TS_T_OS_DETECTED_INT_MASK_SMASK           0xF0000000ull
#define DC_LPE_REG_55_TS12_OS_ERR_DETECTED_INT_MASK_SHIFT       24
#define DC_LPE_REG_55_TS12_OS_ERR_DETECTED_INT_MASK_MASK        0xFull
#define DC_LPE_REG_55_TS12_OS_ERR_DETECTED_INT_MASK_SMASK       0xF000000ull
#define DC_LPE_REG_55_TS2_SEQ_DETECTED_INT_MASK_SHIFT           20
#define DC_LPE_REG_55_TS2_SEQ_DETECTED_INT_MASK_MASK            0xFull
#define DC_LPE_REG_55_TS2_SEQ_DETECTED_INT_MASK_SMASK           0xF00000ull
#define DC_LPE_REG_55_TS1_SEQ_DETECTED_INT_MASK_SHIFT           16
#define DC_LPE_REG_55_TS1_SEQ_DETECTED_INT_MASK_MASK            0xFull
#define DC_LPE_REG_55_TS1_SEQ_DETECTED_INT_MASK_SMASK           0xF0000ull
#define DC_LPE_REG_55_SKP_OS_DETECTED_INT_MASK_SHIFT            12
#define DC_LPE_REG_55_SKP_OS_DETECTED_INT_MASK_MASK             0xFull
#define DC_LPE_REG_55_SKP_OS_DETECTED_INT_MASK_SMASK            0xF000ull
#define DC_LPE_REG_55_TS3_OS_DETECTED_INT_MASK_SHIFT            8
#define DC_LPE_REG_55_TS3_OS_DETECTED_INT_MASK_MASK             0xFull
#define DC_LPE_REG_55_TS3_OS_DETECTED_INT_MASK_SMASK            0xF00ull
#define DC_LPE_REG_55_TS2_OS_DETECTED_INT_MASK_SHIFT            4
#define DC_LPE_REG_55_TS2_OS_DETECTED_INT_MASK_MASK             0xFull
#define DC_LPE_REG_55_TS2_OS_DETECTED_INT_MASK_SMASK            0xF0ull
#define DC_LPE_REG_55_TS1_OS_DETECTED_INT_MASK_SHIFT            0
#define DC_LPE_REG_55_TS1_OS_DETECTED_INT_MASK_MASK             0xFull
#define DC_LPE_REG_55_TS1_OS_DETECTED_INT_MASK_SMASK            0xFull
/*
* Table #76 of 240_CSR_pcslpe.xml - lpe_reg_56
* See description below for individual field definition
*/
#define DC_LPE_REG_56                                           (DC_PCSLPE_LPE + 0x0000000002B0)
#define DC_LPE_REG_56_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_56_UNUSED_31_22_SHIFT                        22
#define DC_LPE_REG_56_UNUSED_31_22_MASK                         0x3FFull
#define DC_LPE_REG_56_UNUSED_31_22_SMASK                        0xFFC00000ull
#define DC_LPE_REG_56_TX_REV_LANES_DETECTED_INT_MASK_SHIFT      20
#define DC_LPE_REG_56_TX_REV_LANES_DETECTED_INT_MASK_MASK       0x1ull
#define DC_LPE_REG_56_TX_REV_LANES_DETECTED_INT_MASK_SMASK      0x100000ull
#define DC_LPE_REG_56_LPE_RX_LATE_LOST_LINK_INT_MASK_SHIFT      19
#define DC_LPE_REG_56_LPE_RX_LATE_LOST_LINK_INT_MASK_MASK       0x1ull
#define DC_LPE_REG_56_LPE_RX_LATE_LOST_LINK_INT_MASK_SMASK      0x80000ull
#define DC_LPE_REG_56_LPE_RX_LATE_CTRL_ERR_INT_MASK_SHIFT       18
#define DC_LPE_REG_56_LPE_RX_LATE_CTRL_ERR_INT_MASK_MASK        0x1ull
#define DC_LPE_REG_56_LPE_RX_LATE_CTRL_ERR_INT_MASK_SMASK       0x40000ull
#define DC_LPE_REG_56_UNUSED_17_SHIFT                           17
#define DC_LPE_REG_56_UNUSED_17_MASK                            0x1ull
#define DC_LPE_REG_56_UNUSED_17_SMASK                           0x20000ull
#define DC_LPE_REG_56_LPE_RX_LATE_CODE_ERR_INT_MASK_SHIFT       16
#define DC_LPE_REG_56_LPE_RX_LATE_CODE_ERR_INT_MASK_MASK        0x1ull
#define DC_LPE_REG_56_LPE_RX_LATE_CODE_ERR_INT_MASK_SMASK       0x10000ull
#define DC_LPE_REG_56_RX_ABR_ERROR_INT_MASK_SHIFT               8
#define DC_LPE_REG_56_RX_ABR_ERROR_INT_MASK_MASK                0xFFull
#define DC_LPE_REG_56_RX_ABR_ERROR_INT_MASK_SMASK               0xFF00ull
#define DC_LPE_REG_56_IB_STAT_FC_VIOLATION_INT_MASK_SHIFT       0
#define DC_LPE_REG_56_IB_STAT_FC_VIOLATION_INT_MASK_MASK        0xFFull
#define DC_LPE_REG_56_IB_STAT_FC_VIOLATION_INT_MASK_SMASK       0xFFull
/*
* Table #2 of 240_CSR_pcslpe.xml - lpe_reg_0
* Register containing various mode and behavior signals. Flow Control Timer can 
* be disabled or accelerated.
*/
#define DC_LPE_REG_0                                            (DC_PCSLPE_LPE + 0x000000000000)
#define DC_LPE_REG_0_RESETCSR                                   0x11110000ull
#define DC_LPE_REG_0_VL_76_FC_PERIOD_SHIFT                      28
#define DC_LPE_REG_0_VL_76_FC_PERIOD_MASK                       0xFull
#define DC_LPE_REG_0_VL_76_FC_PERIOD_SMASK                      0xF0000000ull
#define DC_LPE_REG_0_VL_54_FC_PERIOD_SHIFT                      24
#define DC_LPE_REG_0_VL_54_FC_PERIOD_MASK                       0xFull
#define DC_LPE_REG_0_VL_54_FC_PERIOD_SMASK                      0xF000000ull
#define DC_LPE_REG_0_VL_32_FC_PERIOD_SHIFT                      20
#define DC_LPE_REG_0_VL_32_FC_PERIOD_MASK                       0xFull
#define DC_LPE_REG_0_VL_32_FC_PERIOD_SMASK                      0xF00000ull
#define DC_LPE_REG_0_VL_10_FC_PERIOD_SHIFT                      16
#define DC_LPE_REG_0_VL_10_FC_PERIOD_MASK                       0xFull
#define DC_LPE_REG_0_VL_10_FC_PERIOD_SMASK                      0xF0000ull
#define DC_LPE_REG_0_SEND_ALL_FLOW_CONTROL_SHIFT                15
#define DC_LPE_REG_0_SEND_ALL_FLOW_CONTROL_MASK                 0x1ull
#define DC_LPE_REG_0_SEND_ALL_FLOW_CONTROL_SMASK                0x8000ull
#define DC_LPE_REG_0_EXTERNAL_CCFT_SHIFT                        14
#define DC_LPE_REG_0_EXTERNAL_CCFT_MASK                         0x1ull
#define DC_LPE_REG_0_EXTERNAL_CCFT_SMASK                        0x4000ull
#define DC_LPE_REG_0_EXTERNAL_TS3_SHIFT                         13
#define DC_LPE_REG_0_EXTERNAL_TS3_MASK                          0x1ull
#define DC_LPE_REG_0_EXTERNAL_TS3_SMASK                         0x2000ull
#define DC_LPE_REG_0_HB_TIMER_ACCEL_SHIFT                       12
#define DC_LPE_REG_0_HB_TIMER_ACCEL_MASK                        0x1ull
#define DC_LPE_REG_0_HB_TIMER_ACCEL_SMASK                       0x1000ull
#define DC_LPE_REG_0_FC_TIMER_INHIBIT_SHIFT                     11
#define DC_LPE_REG_0_FC_TIMER_INHIBIT_MASK                      0x1ull
#define DC_LPE_REG_0_FC_TIMER_INHIBIT_SMASK                     0x800ull
#define DC_LPE_REG_0_FC_TIMER_ACCEL_SHIFT                       10
#define DC_LPE_REG_0_FC_TIMER_ACCEL_MASK                        0x1ull
#define DC_LPE_REG_0_FC_TIMER_ACCEL_SMASK                       0x400ull
#define DC_LPE_REG_0_LRH_LVER_EN_SHIFT                          9
#define DC_LPE_REG_0_LRH_LVER_EN_MASK                           0x1ull
#define DC_LPE_REG_0_LRH_LVER_EN_SMASK                          0x200ull
#define DC_LPE_REG_0_SKIP_ACCELERATE_SHIFT                      8
#define DC_LPE_REG_0_SKIP_ACCELERATE_MASK                       0x1ull
#define DC_LPE_REG_0_SKIP_ACCELERATE_SMASK                      0x100ull
#define DC_LPE_REG_0_TX_TEST_MODE_EN_SHIFT                      7
#define DC_LPE_REG_0_TX_TEST_MODE_EN_MASK                       0x1ull
#define DC_LPE_REG_0_TX_TEST_MODE_EN_SMASK                      0x80ull
#define DC_LPE_REG_0_LPE_TX_ECC_MARK_DIS_SHIFT                  6
#define DC_LPE_REG_0_LPE_TX_ECC_MARK_DIS_MASK                   0x1ull
#define DC_LPE_REG_0_LPE_TX_ECC_MARK_DIS_SMASK                  0x40ull
#define DC_LPE_REG_0_FORCE_RX_ACT_TRIG_SHIFT                    5
#define DC_LPE_REG_0_FORCE_RX_ACT_TRIG_MASK                     0x1ull
#define DC_LPE_REG_0_FORCE_RX_ACT_TRIG_SMASK                    0x20ull
#define DC_LPE_REG_0_AUTO_ACT_ENABLE_SHIFT                      4
#define DC_LPE_REG_0_AUTO_ACT_ENABLE_MASK                       0x1ull
#define DC_LPE_REG_0_AUTO_ACT_ENABLE_SMASK                      0x10ull
#define DC_LPE_REG_0_AUTO_ARM_ENABLE_SHIFT                      3
#define DC_LPE_REG_0_AUTO_ARM_ENABLE_MASK                       0x1ull
#define DC_LPE_REG_0_AUTO_ARM_ENABLE_SMASK                      0x8ull
#define DC_LPE_REG_0_LPE_LOOPBACK_SHIFT                         2
#define DC_LPE_REG_0_LPE_LOOPBACK_MASK                          0x1ull
#define DC_LPE_REG_0_LPE_LOOPBACK_SMASK                         0x4ull
#define DC_LPE_REG_0_LPE_LINK_ECC_GEN_EN_SHIFT                  1
#define DC_LPE_REG_0_LPE_LINK_ECC_GEN_EN_MASK                   0x1ull
#define DC_LPE_REG_0_LPE_LINK_ECC_GEN_EN_SMASK                  0x2ull
#define DC_LPE_REG_0_ACT_DEFER_PKT_FLOW_EN_SHIFT                0
#define DC_LPE_REG_0_ACT_DEFER_PKT_FLOW_EN_MASK                 0x1ull
#define DC_LPE_REG_0_ACT_DEFER_PKT_FLOW_EN_SMASK                0x1ull
/*
* Table #3 of 240_CSR_pcslpe.xml - lpe_reg_1
* See description below for individual field definition
*/
#define DC_LPE_REG_1                                            (DC_PCSLPE_LPE + 0x000000000008)
#define DC_LPE_REG_1_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_1_UNUSED_31_5_SHIFT                          5
#define DC_LPE_REG_1_UNUSED_31_5_MASK                           0x7FFFFFFull
#define DC_LPE_REG_1_UNUSED_31_5_SMASK                          0xFFFFFFE0ull
#define DC_LPE_REG_1_LOCAL_BUS_PE_SHIFT                         4
#define DC_LPE_REG_1_LOCAL_BUS_PE_MASK                          0x1ull
#define DC_LPE_REG_1_LOCAL_BUS_PE_SMASK                         0x10ull
#define DC_LPE_REG_1_REG_RD_PE_SHIFT                            3
#define DC_LPE_REG_1_REG_RD_PE_MASK                             0x1ull
#define DC_LPE_REG_1_REG_RD_PE_SMASK                            0x8ull
#define DC_LPE_REG_1_CP_ACTIVE_TRIGGER_SHIFT                    2
#define DC_LPE_REG_1_CP_ACTIVE_TRIGGER_MASK                     0x1ull
#define DC_LPE_REG_1_CP_ACTIVE_TRIGGER_SMASK                    0x4ull
#define DC_LPE_REG_1_RX_ACTIVE_TRIGGER_SHIFT                    1
#define DC_LPE_REG_1_RX_ACTIVE_TRIGGER_MASK                     0x1ull
#define DC_LPE_REG_1_RX_ACTIVE_TRIGGER_SMASK                    0x2ull
#define DC_LPE_REG_1_ACTIVE_ENABLE_SHIFT                        0
#define DC_LPE_REG_1_ACTIVE_ENABLE_MASK                         0x1ull
#define DC_LPE_REG_1_ACTIVE_ENABLE_SMASK                        0x1ull
/*
* Table #4 of 240_CSR_pcslpe.xml - lpe_reg_2
* See description below for individual field definition
*/
#define DC_LPE_REG_2                                            (DC_PCSLPE_LPE + 0x000000000010)
#define DC_LPE_REG_2_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_2_UNUSED_31_1_SHIFT                          1
#define DC_LPE_REG_2_UNUSED_31_1_MASK                           0x7FFFFFFFull
#define DC_LPE_REG_2_UNUSED_31_1_SMASK                          0xFFFFFFFEull
#define DC_LPE_REG_2_LPE_EB_RESET_SHIFT                         0
#define DC_LPE_REG_2_LPE_EB_RESET_MASK                          0x1ull
#define DC_LPE_REG_2_LPE_EB_RESET_SMASK                         0x1ull
/*
* Table #5 of 240_CSR_pcslpe.xml - lpe_reg_3
* See description below for individual field definition
*/
#define DC_LPE_REG_3                                            (DC_PCSLPE_LPE + 0x000000000018)
#define DC_LPE_REG_3_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_3_UNUSED_31_3_SHIFT                          3
#define DC_LPE_REG_3_UNUSED_31_3_MASK                           0x1FFFFFFFull
#define DC_LPE_REG_3_UNUSED_31_3_SMASK                          0xFFFFFFF8ull
#define DC_LPE_REG_3_EN_TX_ICRC_GEN_SHIFT                       2
#define DC_LPE_REG_3_EN_TX_ICRC_GEN_MASK                        0x1ull
#define DC_LPE_REG_3_EN_TX_ICRC_GEN_SMASK                       0x4ull
#define DC_LPE_REG_3_DIS_TX_ICRC_CHK_SHIFT                      1
#define DC_LPE_REG_3_DIS_TX_ICRC_CHK_MASK                       0x1ull
#define DC_LPE_REG_3_DIS_TX_ICRC_CHK_SMASK                      0x2ull
#define DC_LPE_REG_3_DIS_RX_ICRC_CHK_SHIFT                      0
#define DC_LPE_REG_3_DIS_RX_ICRC_CHK_MASK                       0x1ull
#define DC_LPE_REG_3_DIS_RX_ICRC_CHK_SMASK                      0x1ull
/*
* Table #6 of 240_CSR_pcslpe.xml - lpe_reg_5
* This register controls the 8b10b Scramble feature that uses the TS3 exchange 
* to negotiate enabling Scrambling of the 8b data prior to conversion to 10b 
* during transmission and after the 10b to 8b conversion in the receive 
* direction. This mode enable must be coordinated from both sides of the link to 
* assure that there is data coherency after the mode is entered and exited. The 
* scrambling is self-synchronizing in that the endpoints automatically adjust to 
* the flow without strict bit-wise endpoint coordination. If the TS3 byte 9[2] 
* is set on the received TS3, the remote node has the capability to engage in 
* Scrambling. The Force_Scramble mode bit overrides the received TS3 byte 9[2]. 
* We must set our transmitted TS3 byte 9[2] (Scramble Capability) and set our 
* local scramble capability bit. Then when we enter link state Config.Idle and 
* both received and transmitted TS3 bit 9 is set, we enable Scrambling and this 
* is persistent until we go back to polling.
*/
#define DC_LPE_REG_5                                            (DC_PCSLPE_LPE + 0x000000000028)
#define DC_LPE_REG_5_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_5_UNUSED_31_4_SHIFT                          4
#define DC_LPE_REG_5_UNUSED_31_4_MASK                           0xFFFFFFFull
#define DC_LPE_REG_5_UNUSED_31_4_SMASK                          0xFFFFFFF0ull
#define DC_LPE_REG_5_SCRAMBLING_ACTIVE_SHIFT                    3
#define DC_LPE_REG_5_SCRAMBLING_ACTIVE_MASK                     0x1ull
#define DC_LPE_REG_5_SCRAMBLING_ACTIVE_SMASK                    0x8ull
#define DC_LPE_REG_5_LOCAL_SCRAMBLE_CAPABILITY_SHIFT            2
#define DC_LPE_REG_5_LOCAL_SCRAMBLE_CAPABILITY_MASK             0x1ull
#define DC_LPE_REG_5_LOCAL_SCRAMBLE_CAPABILITY_SMASK            0x4ull
#define DC_LPE_REG_5_REMOTE_SCRAMBLE_CAPABILITY_SHIFT           1
#define DC_LPE_REG_5_REMOTE_SCRAMBLE_CAPABILITY_MASK            0x1ull
#define DC_LPE_REG_5_REMOTE_SCRAMBLE_CAPABILITY_SMASK           0x2ull
#define DC_LPE_REG_5_FORCE_SCRAMBLE_SHIFT                       0
#define DC_LPE_REG_5_FORCE_SCRAMBLE_MASK                        0x1ull
#define DC_LPE_REG_5_FORCE_SCRAMBLE_SMASK                       0x1ull
/*
* Table #7 of 240_CSR_pcslpe.xml - lpe_reg_8
* See description below for individual field definition
*/
#define DC_LPE_REG_8                                            (DC_PCSLPE_LPE + 0x000000000040)
#define DC_LPE_REG_8_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_8_UNUSED_31_20_SHIFT                         20
#define DC_LPE_REG_8_UNUSED_31_20_MASK                          0xFFFull
#define DC_LPE_REG_8_UNUSED_31_20_SMASK                         0xFFF00000ull
#define DC_LPE_REG_8_POLLING_HOLD_TIMEOUT_SHIFT                 18
#define DC_LPE_REG_8_POLLING_HOLD_TIMEOUT_MASK                  0x3ull
#define DC_LPE_REG_8_POLLING_HOLD_TIMEOUT_SMASK                 0xC0000ull
#define DC_LPE_REG_8_POLLING_ACTIVE_TIMEOUT_SHIFT               16
#define DC_LPE_REG_8_POLLING_ACTIVE_TIMEOUT_MASK                0x3ull
#define DC_LPE_REG_8_POLLING_ACTIVE_TIMEOUT_SMASK               0x30000ull
#define DC_LPE_REG_8_POLLING_QUIET_TIMEOUT_SHIFT                14
#define DC_LPE_REG_8_POLLING_QUIET_TIMEOUT_MASK                 0x3ull
#define DC_LPE_REG_8_POLLING_QUIET_TIMEOUT_SMASK                0xC000ull
#define DC_LPE_REG_8_DEBOUNCE_TIMEOUT_SHIFT                     12
#define DC_LPE_REG_8_DEBOUNCE_TIMEOUT_MASK                      0x3ull
#define DC_LPE_REG_8_DEBOUNCE_TIMEOUT_SMASK                     0x3000ull
#define DC_LPE_REG_8_RCVR_CFG_HOLD_TIMEOUT_SHIFT                10
#define DC_LPE_REG_8_RCVR_CFG_HOLD_TIMEOUT_MASK                 0x3ull
#define DC_LPE_REG_8_RCVR_CFG_HOLD_TIMEOUT_SMASK                0xC00ull
#define DC_LPE_REG_8_RCVR_CFG_TIMEOUT_SHIFT                     8
#define DC_LPE_REG_8_RCVR_CFG_TIMEOUT_MASK                      0x3ull
#define DC_LPE_REG_8_RCVR_CFG_TIMEOUT_SMASK                     0x300ull
#define DC_LPE_REG_8_WAIT_RMT_TIMEOUT_SHIFT                     6
#define DC_LPE_REG_8_WAIT_RMT_TIMEOUT_MASK                      0x3ull
#define DC_LPE_REG_8_WAIT_RMT_TIMEOUT_SMASK                     0xC0ull
#define DC_LPE_REG_8_WAIT_RMT_HOLD_TIMEOUT_SHIFT                4
#define DC_LPE_REG_8_WAIT_RMT_HOLD_TIMEOUT_MASK                 0x3ull
#define DC_LPE_REG_8_WAIT_RMT_HOLD_TIMEOUT_SMASK                0x30ull
#define DC_LPE_REG_8_TX_REV_LANES_HOLD_TIMEOUT_SHIFT            2
#define DC_LPE_REG_8_TX_REV_LANES_HOLD_TIMEOUT_MASK             0x3ull
#define DC_LPE_REG_8_TX_REV_LANES_HOLD_TIMEOUT_SMASK            0xCull
#define DC_LPE_REG_8_TX_REV_LANES_TIMEOUT_SHIFT                 0
#define DC_LPE_REG_8_TX_REV_LANES_TIMEOUT_MASK                  0x3ull
#define DC_LPE_REG_8_TX_REV_LANES_TIMEOUT_SMASK                 0x3ull
/*
* Table #8 of 240_CSR_pcslpe.xml - lpe_reg_9
* See description below for individual field definition
*/
#define DC_LPE_REG_9                                            (DC_PCSLPE_LPE + 0x000000000048)
#define DC_LPE_REG_9_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_9_UNUSED_31_26_SHIFT                         26
#define DC_LPE_REG_9_UNUSED_31_26_MASK                          0x3Full
#define DC_LPE_REG_9_UNUSED_31_26_SMASK                         0xFC000000ull
#define DC_LPE_REG_9_CFG_ENHANCED_TIMEOUT_SHIFT                 24
#define DC_LPE_REG_9_CFG_ENHANCED_TIMEOUT_MASK                  0x3ull
#define DC_LPE_REG_9_CFG_ENHANCED_TIMEOUT_SMASK                 0x3000000ull
#define DC_LPE_REG_9_CFG_ENHANCED_HOLD_TIMEOUT_SHIFT            22
#define DC_LPE_REG_9_CFG_ENHANCED_HOLD_TIMEOUT_MASK             0x3ull
#define DC_LPE_REG_9_CFG_ENHANCED_HOLD_TIMEOUT_SMASK            0xC00000ull
#define DC_LPE_REG_9_WAIT_CFG_ENHANCED_TIMEOUT_SHIFT            20
#define DC_LPE_REG_9_WAIT_CFG_ENHANCED_TIMEOUT_MASK             0x3ull
#define DC_LPE_REG_9_WAIT_CFG_ENHANCED_TIMEOUT_SMASK            0x300000ull
#define DC_LPE_REG_9_WAIT_RMT_TEST_TIMEOUT_SHIFT                18
#define DC_LPE_REG_9_WAIT_RMT_TEST_TIMEOUT_MASK                 0x3ull
#define DC_LPE_REG_9_WAIT_RMT_TEST_TIMEOUT_SMASK                0xC0000ull
#define DC_LPE_REG_9_CFG_IDLE_TIMEOUT_SHIFT                     16
#define DC_LPE_REG_9_CFG_IDLE_TIMEOUT_MASK                      0x3ull
#define DC_LPE_REG_9_CFG_IDLE_TIMEOUT_SMASK                     0x30000ull
#define DC_LPE_REG_9_CFG_TEST_TIMEOUT_SHIFT                     14
#define DC_LPE_REG_9_CFG_TEST_TIMEOUT_MASK                      0x3ull
#define DC_LPE_REG_9_CFG_TEST_TIMEOUT_SMASK                     0xC000ull
#define DC_LPE_REG_9_CFG_IDLE_HOLD_TIMEOUT_SHIFT                12
#define DC_LPE_REG_9_CFG_IDLE_HOLD_TIMEOUT_MASK                 0x3ull
#define DC_LPE_REG_9_CFG_IDLE_HOLD_TIMEOUT_SMASK                0x3000ull
#define DC_LPE_REG_9_LINK_UP_DETECTED_HOLD_TIMEOUT_SHIFT        10
#define DC_LPE_REG_9_LINK_UP_DETECTED_HOLD_TIMEOUT_MASK         0x3ull
#define DC_LPE_REG_9_LINK_UP_DETECTED_HOLD_TIMEOUT_SMASK        0xC00ull
#define DC_LPE_REG_9_LINK_UP_HOLD_TIMEOUT_SHIFT                 8
#define DC_LPE_REG_9_LINK_UP_HOLD_TIMEOUT_MASK                  0x3ull
#define DC_LPE_REG_9_LINK_UP_HOLD_TIMEOUT_SMASK                 0x300ull
#define DC_LPE_REG_9_REC_RE_TRAIN_HOLD_TIMEOUT_SHIFT            6
#define DC_LPE_REG_9_REC_RE_TRAIN_HOLD_TIMEOUT_MASK             0x3ull
#define DC_LPE_REG_9_REC_RE_TRAIN_HOLD_TIMEOUT_SMASK            0xC0ull
#define DC_LPE_REG_9_REC_WAIT_RMT_HOLD_TIMEOUT_SHIFT            4
#define DC_LPE_REG_9_REC_WAIT_RMT_HOLD_TIMEOUT_MASK             0x3ull
#define DC_LPE_REG_9_REC_WAIT_RMT_HOLD_TIMEOUT_SMASK            0x30ull
#define DC_LPE_REG_9_REC_IDLE_HOLD_TIMEOUT_SHIFT                2
#define DC_LPE_REG_9_REC_IDLE_HOLD_TIMEOUT_MASK                 0x3ull
#define DC_LPE_REG_9_REC_IDLE_HOLD_TIMEOUT_SMASK                0xCull
#define DC_LPE_REG_9_RECOVERY_TIMEOUT_SHIFT                     0
#define DC_LPE_REG_9_RECOVERY_TIMEOUT_MASK                      0x3ull
#define DC_LPE_REG_9_RECOVERY_TIMEOUT_SMASK                     0x3ull
/*
* Table #9 of 240_CSR_pcslpe.xml - lpe_reg_A
* See description below for individual field definition
*/
#define DC_LPE_REG_A                                            (DC_PCSLPE_LPE + 0x000000000050)
#define DC_LPE_REG_A_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_A_UNUSED_31_19_SHIFT                         19
#define DC_LPE_REG_A_UNUSED_31_19_MASK                          0x1FFFull
#define DC_LPE_REG_A_UNUSED_31_19_SMASK                         0xFFF80000ull
#define DC_LPE_REG_A_RCV_TS_T_VALID_SHIFT                       18
#define DC_LPE_REG_A_RCV_TS_T_VALID_MASK                        0x1ull
#define DC_LPE_REG_A_RCV_TS_T_VALID_SMASK                       0x40000ull
#define DC_LPE_REG_A_RCV_TS_T_LANE_SHIFT                        16
#define DC_LPE_REG_A_RCV_TS_T_LANE_MASK                         0x3ull
#define DC_LPE_REG_A_RCV_TS_T_LANE_SMASK                        0x30000ull
#define DC_LPE_REG_A_RCV_TS_T_SPEEDS_SHIFT                      8
#define DC_LPE_REG_A_RCV_TS_T_SPEEDS_MASK                       0xFFull
#define DC_LPE_REG_A_RCV_TS_T_SPEEDS_SMASK                      0xFF00ull
#define DC_LPE_REG_A_RCV_TS_T_OPCODE_SHIFT                      0
#define DC_LPE_REG_A_RCV_TS_T_OPCODE_MASK                       0xFFull
#define DC_LPE_REG_A_RCV_TS_T_OPCODE_SMASK                      0xFFull
/*
* Table #10 of 240_CSR_pcslpe.xml - lpe_reg_B
* See description below for individual field definition
*/
#define DC_LPE_REG_B                                            (DC_PCSLPE_LPE + 0x000000000058)
#define DC_LPE_REG_B_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_B_RCV_TS_T_TX_CFG_SHIFT                      16
#define DC_LPE_REG_B_RCV_TS_T_TX_CFG_MASK                       0xFFFFull
#define DC_LPE_REG_B_RCV_TS_T_TX_CFG_SMASK                      0xFFFF0000ull
#define DC_LPE_REG_B_RCV_TS_T_RX_CFG_SHIFT                      0
#define DC_LPE_REG_B_RCV_TS_T_RX_CFG_MASK                       0xFFFFull
#define DC_LPE_REG_B_RCV_TS_T_RX_CFG_SMASK                      0xFFFFull
/*
* Table #11 of 240_CSR_pcslpe.xml - lpe_reg_C
* See description below for individual field definition
*/
#define DC_LPE_REG_C                                            (DC_PCSLPE_LPE + 0x000000000060)
#define DC_LPE_REG_C_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_C_UNUSED_31_19_SHIFT                         19
#define DC_LPE_REG_C_UNUSED_31_19_MASK                          0x1FFFull
#define DC_LPE_REG_C_UNUSED_31_19_SMASK                         0xFFF80000ull
#define DC_LPE_REG_C_XMIT_TS_T_VALID_SHIFT                      18
#define DC_LPE_REG_C_XMIT_TS_T_VALID_MASK                       0x1ull
#define DC_LPE_REG_C_XMIT_TS_T_VALID_SMASK                      0x40000ull
#define DC_LPE_REG_C_XMIT_TS_T_LANE_SHIFT                       16
#define DC_LPE_REG_C_XMIT_TS_T_LANE_MASK                        0x3ull
#define DC_LPE_REG_C_XMIT_TS_T_LANE_SMASK                       0x30000ull
#define DC_LPE_REG_C_XMIT_TS_T_SPEEDS_SHIFT                     8
#define DC_LPE_REG_C_XMIT_TS_T_SPEEDS_MASK                      0xFFull
#define DC_LPE_REG_C_XMIT_TS_T_SPEEDS_SMASK                     0xFF00ull
#define DC_LPE_REG_C_XMIT_TS_T_OPCODE_SHIFT                     0
#define DC_LPE_REG_C_XMIT_TS_T_OPCODE_MASK                      0xFFull
#define DC_LPE_REG_C_XMIT_TS_T_OPCODE_SMASK                     0xFFull
/*
* Table #12 of 240_CSR_pcslpe.xml - lpe_reg_D
* See description below for individual field definition
*/
#define DC_LPE_REG_D                                            (DC_PCSLPE_LPE + 0x000000000068)
#define DC_LPE_REG_D_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_D_XMIT_TS_T_TX_CFG_SHIFT                     16
#define DC_LPE_REG_D_XMIT_TS_T_TX_CFG_MASK                      0xFFFFull
#define DC_LPE_REG_D_XMIT_TS_T_TX_CFG_SMASK                     0xFFFF0000ull
#define DC_LPE_REG_D_XMIT_TS_T_RX_CFG_SHIFT                     0
#define DC_LPE_REG_D_XMIT_TS_T_RX_CFG_MASK                      0xFFFFull
#define DC_LPE_REG_D_XMIT_TS_T_RX_CFG_SMASK                     0xFFFFull
/*
* Table #13 of 240_CSR_pcslpe.xml - lpe_reg_E
* Spare Register at this time.
*/
#define DC_LPE_REG_E                                            (DC_PCSLPE_LPE + 0x000000000070)
#define DC_LPE_REG_E_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_E_UNUSED_31_0_SHIFT                          0
#define DC_LPE_REG_E_UNUSED_31_0_MASK                           0xFFFFFFFFull
#define DC_LPE_REG_E_UNUSED_31_0_SMASK                          0xFFFFFFFFull
/*
* Table #14 of 240_CSR_pcslpe.xml - lpe_reg_F
* Heartbeats SND packets may be sent manually using the Heartbeat request or 
* sent automatically using Heartbeat auto control.  The PORT and GUID values are 
* placed in the Heartbeat SND packet.  Heartbeat ACK packets are sent when a 
* Heartbeat SND packet has been received.  The Heartbeat ACK packet must contain 
* the same GUID value from the Heartbeat SND packet received.  Heartbeat ACK 
* packets received must contain GUID value matching the GUID of the Heartbeat 
* SND packet.  If the GUID's do not match the ACK packet is ignored.  If a 
* Heartbeat SND packet is received with a GUID matching the port GUID, this 
* indicates channel crosstalk.
*/
#define DC_LPE_REG_F                                            (DC_PCSLPE_LPE + 0x000000000078)
#define DC_LPE_REG_F_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_F_HB_SND_XMIT_VALID_SHIFT                    31
#define DC_LPE_REG_F_HB_SND_XMIT_VALID_MASK                     0x1ull
#define DC_LPE_REG_F_HB_SND_XMIT_VALID_SMASK                    0x80000000ull
#define DC_LPE_REG_F_HB_SND_RCVD_VALID_SHIFT                    30
#define DC_LPE_REG_F_HB_SND_RCVD_VALID_MASK                     0x1ull
#define DC_LPE_REG_F_HB_SND_RCVD_VALID_SMASK                    0x40000000ull
#define DC_LPE_REG_F_HB_ACK_XMIT_VALID_SHIFT                    29
#define DC_LPE_REG_F_HB_ACK_XMIT_VALID_MASK                     0x1ull
#define DC_LPE_REG_F_HB_ACK_XMIT_VALID_SMASK                    0x20000000ull
#define DC_LPE_REG_F_HB_ACK_RCVD_VALID_SHIFT                    28
#define DC_LPE_REG_F_HB_ACK_RCVD_VALID_MASK                     0x1ull
#define DC_LPE_REG_F_HB_ACK_RCVD_VALID_SMASK                    0x10000000ull
#define DC_LPE_REG_F_HB_TIMED_OUT_SHIFT                         27
#define DC_LPE_REG_F_HB_TIMED_OUT_MASK                          0x1ull
#define DC_LPE_REG_F_HB_TIMED_OUT_SMASK                         0x8000000ull
#define DC_LPE_REG_F_UNUSED_26_18_SHIFT                         18
#define DC_LPE_REG_F_UNUSED_26_18_MASK                          0x1FFull
#define DC_LPE_REG_F_UNUSED_26_18_SMASK                         0x7FC0000ull
#define DC_LPE_REG_F_UNUSED_17_16_SHIFT                         16
#define DC_LPE_REG_F_UNUSED_17_16_MASK                          0x3ull
#define DC_LPE_REG_F_UNUSED_17_16_SMASK                         0x30000ull
#define DC_LPE_REG_F_RCV_SND_HB_PORT_NUM_SHIFT                  8
#define DC_LPE_REG_F_RCV_SND_HB_PORT_NUM_MASK                   0xFFull
#define DC_LPE_REG_F_RCV_SND_HB_PORT_NUM_SMASK                  0xFF00ull
#define DC_LPE_REG_F_RCV_ACK_HB_PORT_NUM_SHIFT                  0
#define DC_LPE_REG_F_RCV_ACK_HB_PORT_NUM_MASK                   0xFFull
#define DC_LPE_REG_F_RCV_ACK_HB_PORT_NUM_SMASK                  0xFFull
/*
* Table #15 of 240_CSR_pcslpe.xml - lpe_reg_10
* See description below for individual field definition
*/
#define DC_LPE_REG_10                                           (DC_PCSLPE_LPE + 0x000000000080)
#define DC_LPE_REG_10_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_10_RCV_SND_HB_GUID_U_SHIFT                   0
#define DC_LPE_REG_10_RCV_SND_HB_GUID_U_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_10_RCV_SND_HB_GUID_U_SMASK                   0xFFFFFFFFull
/*
* Table #16 of 240_CSR_pcslpe.xml - lpe_reg_11
* See description below for individual field definition
*/
#define DC_LPE_REG_11                                           (DC_PCSLPE_LPE + 0x000000000088)
#define DC_LPE_REG_11_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_11_RCV_SND_HB_GUID_L_SHIFT                   0
#define DC_LPE_REG_11_RCV_SND_HB_GUID_L_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_11_RCV_SND_HB_GUID_L_SMASK                   0xFFFFFFFFull
/*
* Table #17 of 240_CSR_pcslpe.xml - lpe_reg_12
* See description below for individual field definition
*/
#define DC_LPE_REG_12                                           (DC_PCSLPE_LPE + 0x000000000090)
#define DC_LPE_REG_12_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_12_RCV_ACK_HB_GUID_U_SHIFT                   0
#define DC_LPE_REG_12_RCV_ACK_HB_GUID_U_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_12_RCV_ACK_HB_GUID_U_SMASK                   0xFFFFFFFFull
/*
* Table #18 of 240_CSR_pcslpe.xml - lpe_reg_13
* See description below for individual field definition
*/
#define DC_LPE_REG_13                                           (DC_PCSLPE_LPE + 0x000000000098)
#define DC_LPE_REG_13_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_13_RCV_ACK_HB_GUID_L_SHIFT                   0
#define DC_LPE_REG_13_RCV_ACK_HB_GUID_L_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_13_RCV_ACK_HB_GUID_L_SMASK                   0xFFFFFFFFull
/*
* Table #19 of 240_CSR_pcslpe.xml - lpe_reg_14
* See description below for individual field definition
*/
#define DC_LPE_REG_14                                           (DC_PCSLPE_LPE + 0x0000000000A0)
#define DC_LPE_REG_14_RESETCSR                                  0x000F0000ull
#define DC_LPE_REG_14_UNUSED_31_28_SHIFT                        28
#define DC_LPE_REG_14_UNUSED_31_28_MASK                         0xFull
#define DC_LPE_REG_14_UNUSED_31_28_SMASK                        0xF0000000ull
#define DC_LPE_REG_14_HB_CROSSTALK_SHIFT                        24
#define DC_LPE_REG_14_HB_CROSSTALK_MASK                         0xFull
#define DC_LPE_REG_14_HB_CROSSTALK_SMASK                        0xF000000ull
#define DC_LPE_REG_14_UNUSED_23_SHIFT                           23
#define DC_LPE_REG_14_UNUSED_23_MASK                            0x1ull
#define DC_LPE_REG_14_UNUSED_23_SMASK                           0x800000ull
#define DC_LPE_REG_14_XMIT_HB_ENABLE_SHIFT                      22
#define DC_LPE_REG_14_XMIT_HB_ENABLE_MASK                       0x1ull
#define DC_LPE_REG_14_XMIT_HB_ENABLE_SMASK                      0x400000ull
#define DC_LPE_REG_14_XMIT_HB_REQUEST_SHIFT                     21
#define DC_LPE_REG_14_XMIT_HB_REQUEST_MASK                      0x1ull
#define DC_LPE_REG_14_XMIT_HB_REQUEST_SMASK                     0x200000ull
#define DC_LPE_REG_14_XMIT_HB_AUTO_SHIFT                        20
#define DC_LPE_REG_14_XMIT_HB_AUTO_MASK                         0x1ull
#define DC_LPE_REG_14_XMIT_HB_AUTO_SMASK                        0x100000ull
#define DC_LPE_REG_14_XMIT_HB_LANE_MASK_SHIFT                   16
#define DC_LPE_REG_14_XMIT_HB_LANE_MASK_MASK                    0xFull
#define DC_LPE_REG_14_XMIT_HB_LANE_MASK_SMASK                   0xF0000ull
#define DC_LPE_REG_14_XMIT_HB_OPCODE_SHIFT                      8
#define DC_LPE_REG_14_XMIT_HB_OPCODE_MASK                       0xFFull
#define DC_LPE_REG_14_XMIT_HB_OPCODE_SMASK                      0xFF00ull
#define DC_LPE_REG_14_XMIT_HB_PORT_NUM_SHIFT                    0
#define DC_LPE_REG_14_XMIT_HB_PORT_NUM_MASK                     0xFFull
#define DC_LPE_REG_14_XMIT_HB_PORT_NUM_SMASK                    0xFFull
/*
* Table #20 of 240_CSR_pcslpe.xml - lpe_reg_15
* See description below for individual field definition
*/
#define DC_LPE_REG_15                                           (DC_PCSLPE_LPE + 0x0000000000A8)
#define DC_LPE_REG_15_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_15_XMIT_HB_GUID_UPPR_SHIFT                   0
#define DC_LPE_REG_15_XMIT_HB_GUID_UPPR_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_15_XMIT_HB_GUID_UPPR_SMASK                   0xFFFFFFFFull
/*
* Table #21 of 240_CSR_pcslpe.xml - lpe_reg_16
* See description below for individual field definition
*/
#define DC_LPE_REG_16                                           (DC_PCSLPE_LPE + 0x0000000000B0)
#define DC_LPE_REG_16_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_16_XMIT_HB_GUID_LOWR_SHIFT                   0
#define DC_LPE_REG_16_XMIT_HB_GUID_LOWR_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_16_XMIT_HB_GUID_LOWR_SMASK                   0xFFFFFFFFull
/*
* Table #22 of 240_CSR_pcslpe.xml - lpe_reg_17
* See description below for individual field definition
*/
#define DC_LPE_REG_17                                           (DC_PCSLPE_LPE + 0x0000000000B8)
#define DC_LPE_REG_17_RESETCSR                                  0x07FFFFFFull
#define DC_LPE_REG_17_UNUSED_31_27_SHIFT                        27
#define DC_LPE_REG_17_UNUSED_31_27_MASK                         0x1Full
#define DC_LPE_REG_17_UNUSED_31_27_SMASK                        0xF8000000ull
#define DC_LPE_REG_17_HB_LATENCY_SHIFT                          0
#define DC_LPE_REG_17_HB_LATENCY_MASK                           0x7FFFFFFull
#define DC_LPE_REG_17_HB_LATENCY_SMASK                          0x7FFFFFFull
/*
* Table #23 of 240_CSR_pcslpe.xml - lpe_reg_18
* See description below for individual field definition
*/
#define DC_LPE_REG_18                                           (DC_PCSLPE_LPE + 0x0000000000C0)
#define DC_LPE_REG_18_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_18_TS_T_OS_DETECTED_SHIFT                    28
#define DC_LPE_REG_18_TS_T_OS_DETECTED_MASK                     0xFull
#define DC_LPE_REG_18_TS_T_OS_DETECTED_SMASK                    0xF0000000ull
#define DC_LPE_REG_18_TS12_OS_ERR_DETECTED_SHIFT                24
#define DC_LPE_REG_18_TS12_OS_ERR_DETECTED_MASK                 0xFull
#define DC_LPE_REG_18_TS12_OS_ERR_DETECTED_SMASK                0xF000000ull
#define DC_LPE_REG_18_TS2_SEQ_DETECTED_SHIFT                    20
#define DC_LPE_REG_18_TS2_SEQ_DETECTED_MASK                     0xFull
#define DC_LPE_REG_18_TS2_SEQ_DETECTED_SMASK                    0xF00000ull
#define DC_LPE_REG_18_TS1_SEQ_DETECTED_SHIFT                    16
#define DC_LPE_REG_18_TS1_SEQ_DETECTED_MASK                     0xFull
#define DC_LPE_REG_18_TS1_SEQ_DETECTED_SMASK                    0xF0000ull
#define DC_LPE_REG_18_SKP_OS_DETECTED_SHIFT                     12
#define DC_LPE_REG_18_SKP_OS_DETECTED_MASK                      0xFull
#define DC_LPE_REG_18_SKP_OS_DETECTED_SMASK                     0xF000ull
#define DC_LPE_REG_18_TS3_OS_DETECTED_SHIFT                     8
#define DC_LPE_REG_18_TS3_OS_DETECTED_MASK                      0xFull
#define DC_LPE_REG_18_TS3_OS_DETECTED_SMASK                     0xF00ull
#define DC_LPE_REG_18_TS2_OS_DETECTED_SHIFT                     4
#define DC_LPE_REG_18_TS2_OS_DETECTED_MASK                      0xFull
#define DC_LPE_REG_18_TS2_OS_DETECTED_SMASK                     0xF0ull
#define DC_LPE_REG_18_TS1_OS_DETECTED_SHIFT                     0
#define DC_LPE_REG_18_TS1_OS_DETECTED_MASK                      0xFull
#define DC_LPE_REG_18_TS1_OS_DETECTED_SMASK                     0xFull
/*
* Table #24 of 240_CSR_pcslpe.xml - lpe_reg_19
* See description below for individual field definition
*/
#define DC_LPE_REG_19                                           (DC_PCSLPE_LPE + 0x0000000000C8)
#define DC_LPE_REG_19_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_19_L0_TS1_CNT_SHIFT                          24
#define DC_LPE_REG_19_L0_TS1_CNT_MASK                           0xFFull
#define DC_LPE_REG_19_L0_TS1_CNT_SMASK                          0xFF000000ull
/*
* Table #25 of 240_CSR_pcslpe.xml - lpe_reg_1A
* See description below for individual field definition
*/
#define DC_LPE_REG_1A                                           (DC_PCSLPE_LPE + 0x0000000000D0)
#define DC_LPE_REG_1A_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_1A_L0_TS2_CNT_SHIFT                          24
#define DC_LPE_REG_1A_L0_TS2_CNT_MASK                           0xFFull
#define DC_LPE_REG_1A_L0_TS2_CNT_SMASK                          0xFF000000ull
/*
* Table #26 of 240_CSR_pcslpe.xml - lpe_reg_1B
* See description below for individual field definition
*/
#define DC_LPE_REG_1B                                           (DC_PCSLPE_LPE + 0x0000000000D8)
#define DC_LPE_REG_1B_RESETCSR                                  0x00080000ull
#define DC_LPE_REG_1B_TS12_ALIVE_CNT_THRES_SHIFT                16
#define DC_LPE_REG_1B_TS12_ALIVE_CNT_THRES_MASK                 0xFFFFull
#define DC_LPE_REG_1B_TS12_ALIVE_CNT_THRES_SMASK                0xFFFF0000ull
#define DC_LPE_REG_1B_LANE_ALIVE_SHIFT                          12
#define DC_LPE_REG_1B_LANE_ALIVE_MASK                           0xFull
#define DC_LPE_REG_1B_LANE_ALIVE_SMASK                          0xF000ull
#define DC_LPE_REG_1B_HOLD_TIME_LANE_ALIVE_SHIFT                8
#define DC_LPE_REG_1B_HOLD_TIME_LANE_ALIVE_MASK                 0xFull
#define DC_LPE_REG_1B_HOLD_TIME_LANE_ALIVE_SMASK                0xF00ull
#define DC_LPE_REG_1B_UNUSED_7_2_SHIFT                          2
#define DC_LPE_REG_1B_UNUSED_7_2_MASK                           0x3Full
#define DC_LPE_REG_1B_UNUSED_7_2_SMASK                          0xFCull
#define DC_LPE_REG_1B_OS_SEQ_CNT_SHIFT                          0
#define DC_LPE_REG_1B_OS_SEQ_CNT_MASK                           0x3ull
#define DC_LPE_REG_1B_OS_SEQ_CNT_SMASK                          0x3ull
/*
* Table #27 of 240_CSR_pcslpe.xml - lpe_reg_1C
* See description below for individual field definition
*/
#define DC_LPE_REG_1C                                           (DC_PCSLPE_LPE + 0x0000000000E0)
#define DC_LPE_REG_1C_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_1C_UNUSED_31_25_SHIFT                        25
#define DC_LPE_REG_1C_UNUSED_31_25_MASK                         0x7Full
#define DC_LPE_REG_1C_UNUSED_31_25_SMASK                        0xFE000000ull
#define DC_LPE_REG_1C_TX_MOD_TS1_EN_SHIFT                       24
#define DC_LPE_REG_1C_TX_MOD_TS1_EN_MASK                        0x1ull
#define DC_LPE_REG_1C_TX_MOD_TS1_EN_SMASK                       0x1000000ull
#define DC_LPE_REG_1C_TX_MOD_TS1_DATA_SHIFT                     0
#define DC_LPE_REG_1C_TX_MOD_TS1_DATA_MASK                      0xFFFFFFull
#define DC_LPE_REG_1C_TX_MOD_TS1_DATA_SMASK                     0xFFFFFFull
/*
* Table #28 of 240_CSR_pcslpe.xml - lpe_reg_1D
* See description below for individual field definition
*/
#define DC_LPE_REG_1D                                           (DC_PCSLPE_LPE + 0x0000000000E8)
#define DC_LPE_REG_1D_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_1D_UNUSED_31_25_SHIFT                        25
#define DC_LPE_REG_1D_UNUSED_31_25_MASK                         0x7Full
#define DC_LPE_REG_1D_UNUSED_31_25_SMASK                        0xFE000000ull
#define DC_LPE_REG_1D_RX_MOD_TS1_SAMPLED_SHIFT                  24
#define DC_LPE_REG_1D_RX_MOD_TS1_SAMPLED_MASK                   0x1ull
#define DC_LPE_REG_1D_RX_MOD_TS1_SAMPLED_SMASK                  0x1000000ull
#define DC_LPE_REG_1D_RX_MOD_TS1_DATA_SHIFT                     0
#define DC_LPE_REG_1D_RX_MOD_TS1_DATA_MASK                      0xFFFFFFull
#define DC_LPE_REG_1D_RX_MOD_TS1_DATA_SMASK                     0xFFFFFFull
/*
* Table #29 of 240_CSR_pcslpe.xml - lpe_reg_1E
* See description below for individual field definition
*/
#define DC_LPE_REG_1E                                           (DC_PCSLPE_LPE + 0x0000000000F0)
#define DC_LPE_REG_1E_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_1E_UNUSED_31_25_SHIFT                        25
#define DC_LPE_REG_1E_UNUSED_31_25_MASK                         0x7Full
#define DC_LPE_REG_1E_UNUSED_31_25_SMASK                        0xFE000000ull
#define DC_LPE_REG_1E_CFFT_TX_REQ_SHIFT                         24
#define DC_LPE_REG_1E_CFFT_TX_REQ_MASK                          0x1ull
#define DC_LPE_REG_1E_CFFT_TX_REQ_SMASK                         0x1000000ull
#define DC_LPE_REG_1E_CCFT_TX_CMD_SHIFT                         16
#define DC_LPE_REG_1E_CCFT_TX_CMD_MASK                          0xFFull
#define DC_LPE_REG_1E_CCFT_TX_CMD_SMASK                         0xFF0000ull
#define DC_LPE_REG_1E_CCFT_TX_STATUS_SHIFT                      8
#define DC_LPE_REG_1E_CCFT_TX_STATUS_MASK                       0xFFull
#define DC_LPE_REG_1E_CCFT_TX_STATUS_SMASK                      0xFF00ull
#define DC_LPE_REG_1E_CCFT_TX_LANE_SHIFT                        0
#define DC_LPE_REG_1E_CCFT_TX_LANE_MASK                         0xFFull
#define DC_LPE_REG_1E_CCFT_TX_LANE_SMASK                        0xFFull
/*
* Table #30 of 240_CSR_pcslpe.xml - lpe_reg_1F
* See description below for individual field definition
*/
#define DC_LPE_REG_1F                                           (DC_PCSLPE_LPE + 0x0000000000F8)
#define DC_LPE_REG_1F_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_1F_UNUSED_31_25_SHIFT                        25
#define DC_LPE_REG_1F_UNUSED_31_25_MASK                         0x7Full
#define DC_LPE_REG_1F_UNUSED_31_25_SMASK                        0xFE000000ull
#define DC_LPE_REG_1F_CCFT_RX_REQ_SHIFT                         24
#define DC_LPE_REG_1F_CCFT_RX_REQ_MASK                          0x1ull
#define DC_LPE_REG_1F_CCFT_RX_REQ_SMASK                         0x1000000ull
#define DC_LPE_REG_1F_CCFT_RX_CMD_SHIFT                         16
#define DC_LPE_REG_1F_CCFT_RX_CMD_MASK                          0xFFull
#define DC_LPE_REG_1F_CCFT_RX_CMD_SMASK                         0xFF0000ull
#define DC_LPE_REG_1F_CCFT_RX_STATUS_SHIFT                      8
#define DC_LPE_REG_1F_CCFT_RX_STATUS_MASK                       0xFFull
#define DC_LPE_REG_1F_CCFT_RX_STATUS_SMASK                      0xFF00ull
#define DC_LPE_REG_1F_CCFT_RX_LANE_SHIFT                        0
#define DC_LPE_REG_1F_CCFT_RX_LANE_MASK                         0xFFull
#define DC_LPE_REG_1F_CCFT_RX_LANE_SMASK                        0xFFull
/*
* Table #31 of 240_CSR_pcslpe.xml - lpe_reg_20
* See description below for individual field definition
*/
#define DC_LPE_REG_20                                           (DC_PCSLPE_LPE + 0x000000000100)
#define DC_LPE_REG_20_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_20_TS3_SEQ_RX_DATA_89_SHIFT                  31
#define DC_LPE_REG_20_TS3_SEQ_RX_DATA_89_MASK                   0x1ull
#define DC_LPE_REG_20_TS3_SEQ_RX_DATA_89_SMASK                  0x80000000ull
#define DC_LPE_REG_20_UNUSED_30_16_SHIFT                        16
#define DC_LPE_REG_20_UNUSED_30_16_MASK                         0x7FFFull
#define DC_LPE_REG_20_UNUSED_30_16_SMASK                        0x7FFF0000ull
#define DC_LPE_REG_20_TS3BYTE8_SHIFT                            8
#define DC_LPE_REG_20_TS3BYTE8_MASK                             0xFFull
#define DC_LPE_REG_20_TS3BYTE8_SMASK                            0xFF00ull
#define DC_LPE_REG_20_TS3BYTE9_SHIFT                            0
#define DC_LPE_REG_20_TS3BYTE9_MASK                             0xFFull
#define DC_LPE_REG_20_TS3BYTE9_SMASK                            0xFFull
/*
* Table #32 of 240_CSR_pcslpe.xml - lpe_reg_21
* See description below for individual field definition
*/
#define DC_LPE_REG_21                                           (DC_PCSLPE_LPE + 0x000000000108)
#define DC_LPE_REG_21_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_21_TS3_RX_DATA_10_LANE_0_SHIFT               24
#define DC_LPE_REG_21_TS3_RX_DATA_10_LANE_0_MASK                0xFFull
#define DC_LPE_REG_21_TS3_RX_DATA_10_LANE_0_SMASK               0xFF000000ull
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_1_SHIFT               16
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_1_MASK                0xFFull
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_1_SMASK               0xFF0000ull
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_2_SHIFT               8
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_2_MASK                0xFFull
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_2_SMASK               0xFF00ull
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_3_SHIFT               0
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_3_MASK                0xFFull
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_3_SMASK               0xFFull
/*
* Table #33 of 240_CSR_pcslpe.xml - lpe_reg_22
* See description below for individual field definition
*/
#define DC_LPE_REG_22                                           (DC_PCSLPE_LPE + 0x000000000110)
#define DC_LPE_REG_22_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_0_SHIFT               24
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_0_MASK                0xFFull
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_0_SMASK               0xFF000000ull
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_1_SHIFT               16
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_1_MASK                0xFFull
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_1_SMASK               0xFF0000ull
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_2_SHIFT               8
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_2_MASK                0xFFull
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_2_SMASK               0xFF00ull
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_3_SHIFT               0
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_3_MASK                0xFFull
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_3_SMASK               0xFFull
/*
* Table #34 of 240_CSR_pcslpe.xml - lpe_reg_23
* See description below for individual field definition
*/
#define DC_LPE_REG_23                                           (DC_PCSLPE_LPE + 0x000000000118)
#define DC_LPE_REG_23_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_23_TS3_RX_BYTE_12_LANE_0_SHIFT               24
#define DC_LPE_REG_23_TS3_RX_BYTE_12_LANE_0_MASK                0xFFull
#define DC_LPE_REG_23_TS3_RX_BYTE_12_LANE_0_SMASK               0xFF000000ull
#define DC_LPE_REG_23_TS3_RX_BYTE_13_LANE_1_SHIFT               16
#define DC_LPE_REG_23_TS3_RX_BYTE_13_LANE_1_MASK                0xFFull
#define DC_LPE_REG_23_TS3_RX_BYTE_13_LANE_1_SMASK               0xFF0000ull
#define DC_LPE_REG_23_TS3_RX_BYTE_14_LANE_2_SHIFT               8
#define DC_LPE_REG_23_TS3_RX_BYTE_14_LANE_2_MASK                0xFFull
#define DC_LPE_REG_23_TS3_RX_BYTE_14_LANE_2_SMASK               0xFF00ull
#define DC_LPE_REG_23_TS3_RX_BYTE_15_LANE_3_SHIFT               0
#define DC_LPE_REG_23_TS3_RX_BYTE_15_LANE_3_MASK                0xFFull
#define DC_LPE_REG_23_TS3_RX_BYTE_15_LANE_3_SMASK               0xFFull
/*
* Table #35 of 240_CSR_pcslpe.xml - lpe_reg_24
* The following TX TS3 bytes are used when sending TS3's. See description below 
* for individual field definition#%%#Address = Base Address + Offset
*/
#define DC_LPE_REG_24                                           (DC_PCSLPE_LPE + 0x000000000120)
#define DC_LPE_REG_24_RESETCSR                                  0x00001040ull
#define DC_LPE_REG_24_UNUSED_31_16_SHIFT                        16
#define DC_LPE_REG_24_UNUSED_31_16_MASK                         0xFFFFull
#define DC_LPE_REG_24_UNUSED_31_16_SMASK                        0xFFFF0000ull
#define DC_LPE_REG_24_TS3_TX_BYTE_8_SHIFT                       8
#define DC_LPE_REG_24_TS3_TX_BYTE_8_MASK                        0xFFull
#define DC_LPE_REG_24_TS3_TX_BYTE_8_SMASK                       0xFF00ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_TT_SHIFT                    4
#define DC_LPE_REG_24_TS3_TX_BYTE_9_TT_MASK                     0xFull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_TT_SMASK                    0xF0ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_FT_SHIFT                    3
#define DC_LPE_REG_24_TS3_TX_BYTE_9_FT_MASK                     0x1ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_FT_SMASK                    0x8ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_SC_SHIFT                    2
#define DC_LPE_REG_24_TS3_TX_BYTE_9_SC_MASK                     0x1ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_SC_SMASK                    0x4ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_HBR_SHIFT                   1
#define DC_LPE_REG_24_TS3_TX_BYTE_9_HBR_MASK                    0x1ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_HBR_SMASK                   0x2ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_ADD_SHIFT                   0
#define DC_LPE_REG_24_TS3_TX_BYTE_9_ADD_MASK                    0x1ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_ADD_SMASK                   0x1ull
/*
* Table #36 of 240_CSR_pcslpe.xml - lpe_reg_25
* See description below for individual field definition
*/
#define DC_LPE_REG_25                                           (DC_PCSLPE_LPE + 0x000000000128)
#define DC_LPE_REG_25_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_25_TS3_TX_DATA_10_LANE_0_SHIFT               24
#define DC_LPE_REG_25_TS3_TX_DATA_10_LANE_0_MASK                0xFFull
#define DC_LPE_REG_25_TS3_TX_DATA_10_LANE_0_SMASK               0xFF000000ull
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_1_SHIFT               16
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_1_MASK                0xFFull
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_1_SMASK               0xFF0000ull
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_2_SHIFT               8
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_2_MASK                0xFFull
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_2_SMASK               0xFF00ull
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_3_SHIFT               0
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_3_MASK                0xFFull
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_3_SMASK               0xFFull
/*
* Table #37 of 240_CSR_pcslpe.xml - lpe_reg_26
* See description below for individual field definition
*/
#define DC_LPE_REG_26                                           (DC_PCSLPE_LPE + 0x000000000130)
#define DC_LPE_REG_26_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_0_SHIFT               24
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_0_MASK                0xFFull
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_0_SMASK               0xFF000000ull
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_1_SHIFT               16
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_1_MASK                0xFFull
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_1_SMASK               0xFF0000ull
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_2_SHIFT               8
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_2_MASK                0xFFull
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_2_SMASK               0xFF00ull
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_3_SHIFT               0
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_3_MASK                0xFFull
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_3_SMASK               0xFFull
/*
* Table #38 of 240_CSR_pcslpe.xml - lpe_reg_27
* See description below for individual field definition
*/
#define DC_LPE_REG_27                                           (DC_PCSLPE_LPE + 0x000000000138)
#define DC_LPE_REG_27_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_27_TS3_TX_BYTE_12_SHIFT                      24
#define DC_LPE_REG_27_TS3_TX_BYTE_12_MASK                       0xFFull
#define DC_LPE_REG_27_TS3_TX_BYTE_12_SMASK                      0xFF000000ull
#define DC_LPE_REG_27_TS3_TX_BYTE_13_SHIFT                      16
#define DC_LPE_REG_27_TS3_TX_BYTE_13_MASK                       0xFFull
#define DC_LPE_REG_27_TS3_TX_BYTE_13_SMASK                      0xFF0000ull
#define DC_LPE_REG_27_TS3_TX_BYTE_14_SHIFT                      8
#define DC_LPE_REG_27_TS3_TX_BYTE_14_MASK                       0xFFull
#define DC_LPE_REG_27_TS3_TX_BYTE_14_SMASK                      0xFF00ull
#define DC_LPE_REG_27_TS3_TX_BYTE_15_SHIFT                      0
#define DC_LPE_REG_27_TS3_TX_BYTE_15_MASK                       0xFFull
#define DC_LPE_REG_27_TS3_TX_BYTE_15_SMASK                      0xFFull
/*
* Table #39 of 240_CSR_pcslpe.xml - lpe_reg_28
* This register shows the status of the 'rx_lane_order_a,b,c,d' and 
* 'rx_lane_polarity'. These are the calculated values needed to 'Un-Swizzle' the 
* data lanes into the required order of 3,2,1,0. These values are provided to 
* the PCSs so that they can use them in the 'Auto_Polarity_Rev' mode of 
* operation. NOTE: For EDR-Only mode of operation, the 'Auto_Polarity_Rev' bit 
* in the PCS_66 Common register must be set to a '0' and then the 
* 'Cmn_Rx_Lane_Order' and 'Cmn_Rx_Lane_Polarity' values must be prorammed to the 
* desired values for correct operation. This is necessary since we do not have 
* the 8b10b speed helping to discover the Swizzle solution - it must be provided 
* by the programmer. See description below for individual field 
* definition.
*/
#define DC_LPE_REG_28                                           (DC_PCSLPE_LPE + 0x000000000140)
#define DC_LPE_REG_28_RESETCSR                                  0x2E40E402ull
#define DC_LPE_REG_28_UNUSED_31_SHIFT                           31
#define DC_LPE_REG_28_UNUSED_31_MASK                            0x1ull
#define DC_LPE_REG_28_UNUSED_31_SMASK                           0x80000000ull
#define DC_LPE_REG_28_RX_TRAINED_TIMEOUT_SHIFT                  30
#define DC_LPE_REG_28_RX_TRAINED_TIMEOUT_MASK                   0x1ull
#define DC_LPE_REG_28_RX_TRAINED_TIMEOUT_SMASK                  0x40000000ull
#define DC_LPE_REG_28_RX_TRAINED_TST_EN_SHIFT                   29
#define DC_LPE_REG_28_RX_TRAINED_TST_EN_MASK                    0x1ull
#define DC_LPE_REG_28_RX_TRAINED_TST_EN_SMASK                   0x20000000ull
#define DC_LPE_REG_28_RX_TRAINED_SHIFT                          28
#define DC_LPE_REG_28_RX_TRAINED_MASK                           0x1ull
#define DC_LPE_REG_28_RX_TRAINED_SMASK                          0x10000000ull
#define DC_LPE_REG_28_RX_LANE_ORDER_D_SHIFT                     26
#define DC_LPE_REG_28_RX_LANE_ORDER_D_MASK                      0x3ull
#define DC_LPE_REG_28_RX_LANE_ORDER_D_SMASK                     0xC000000ull
#define DC_LPE_REG_28_RX_LANE_ORDER_C_SHIFT                     24
#define DC_LPE_REG_28_RX_LANE_ORDER_C_MASK                      0x3ull
#define DC_LPE_REG_28_RX_LANE_ORDER_C_SMASK                     0x3000000ull
#define DC_LPE_REG_28_RX_LANE_ORDER_B_SHIFT                     22
#define DC_LPE_REG_28_RX_LANE_ORDER_B_MASK                      0x3ull
#define DC_LPE_REG_28_RX_LANE_ORDER_B_SMASK                     0xC00000ull
#define DC_LPE_REG_28_RX_LANE_ORDER_A_SHIFT                     20
#define DC_LPE_REG_28_RX_LANE_ORDER_A_MASK                      0x3ull
#define DC_LPE_REG_28_RX_LANE_ORDER_A_SMASK                     0x300000ull
#define DC_LPE_REG_28_RX_LANE_POLARITY_SHIFT                    16
#define DC_LPE_REG_28_RX_LANE_POLARITY_MASK                     0xFull
#define DC_LPE_REG_28_RX_LANE_POLARITY_SMASK                    0xF0000ull
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_D_SHIFT                 14
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_D_MASK                  0x3ull
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_D_SMASK                 0xC000ull
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_C_SHIFT                 12
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_C_MASK                  0x3ull
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_C_SMASK                 0x3000ull
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_B_SHIFT                 10
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_B_MASK                  0x3ull
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_B_SMASK                 0xC00ull
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_A_SHIFT                 8
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_A_MASK                  0x3ull
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_A_SMASK                 0x300ull
#define DC_LPE_REG_28_RX_RAW_LANE_POLARITY_SHIFT                4
#define DC_LPE_REG_28_RX_RAW_LANE_POLARITY_MASK                 0xFull
#define DC_LPE_REG_28_RX_RAW_LANE_POLARITY_SMASK                0xF0ull
#define DC_LPE_REG_28_DISABLE_LINK_REC_SHIFT                    3
#define DC_LPE_REG_28_DISABLE_LINK_REC_MASK                     0x1ull
#define DC_LPE_REG_28_DISABLE_LINK_REC_SMASK                    0x8ull
#define DC_LPE_REG_28_TEST_IDLE_JMP_SHIFT                       2
#define DC_LPE_REG_28_TEST_IDLE_JMP_MASK                        0x1ull
#define DC_LPE_REG_28_TEST_IDLE_JMP_SMASK                       0x4ull
#define DC_LPE_REG_28_POLARITY_SWAP_SUPPORTED_SHIFT             1
#define DC_LPE_REG_28_POLARITY_SWAP_SUPPORTED_MASK              0x1ull
#define DC_LPE_REG_28_POLARITY_SWAP_SUPPORTED_SMASK             0x2ull
#define DC_LPE_REG_28_TX_LANE_REVERSE_SUPPORTED_SHIFT           0
#define DC_LPE_REG_28_TX_LANE_REVERSE_SUPPORTED_MASK            0x1ull
#define DC_LPE_REG_28_TX_LANE_REVERSE_SUPPORTED_SMASK           0x1ull
/*
* Table #40 of 240_CSR_pcslpe.xml - lpe_reg_29
* See description below for individual field definition
*/
#define DC_LPE_REG_29                                           (DC_PCSLPE_LPE + 0x000000000148)
#define DC_LPE_REG_29_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_29_VL_FC_TIMEOUT_SHIFT                       24
#define DC_LPE_REG_29_VL_FC_TIMEOUT_MASK                        0xFFull
#define DC_LPE_REG_29_VL_FC_TIMEOUT_SMASK                       0xFF000000ull
#define DC_LPE_REG_29_UNUSED_23_21_SHIFT                        21
#define DC_LPE_REG_29_UNUSED_23_21_MASK                         0x7ull
#define DC_LPE_REG_29_UNUSED_23_21_SMASK                        0xE00000ull
#define DC_LPE_REG_29_TX_REV_LANES_DETECTED_SHIFT               20
#define DC_LPE_REG_29_TX_REV_LANES_DETECTED_MASK                0x1ull
#define DC_LPE_REG_29_TX_REV_LANES_DETECTED_SMASK               0x100000ull
#define DC_LPE_REG_29_LPE_RX_LATE_LOST_LINK_SHIFT               19
#define DC_LPE_REG_29_LPE_RX_LATE_LOST_LINK_MASK                0x1ull
#define DC_LPE_REG_29_LPE_RX_LATE_LOST_LINK_SMASK               0x80000ull
#define DC_LPE_REG_29_LPE_RX_LATE_CTRL_ERR_SHIFT                18
#define DC_LPE_REG_29_LPE_RX_LATE_CTRL_ERR_MASK                 0x1ull
#define DC_LPE_REG_29_LPE_RX_LATE_CTRL_ERR_SMASK                0x40000ull
#define DC_LPE_REG_29_UNUSED_17_SHIFT                           17
#define DC_LPE_REG_29_UNUSED_17_MASK                            0x1ull
#define DC_LPE_REG_29_UNUSED_17_SMASK                           0x20000ull
#define DC_LPE_REG_29_LPE_RX_LATE_CODE_ERR_SHIFT                16
#define DC_LPE_REG_29_LPE_RX_LATE_CODE_ERR_MASK                 0x1ull
#define DC_LPE_REG_29_LPE_RX_LATE_CODE_ERR_SMASK                0x10000ull
#define DC_LPE_REG_29_RX_ABR_ERROR_SHIFT                        8
#define DC_LPE_REG_29_RX_ABR_ERROR_MASK                         0xFFull
#define DC_LPE_REG_29_RX_ABR_ERROR_SMASK                        0xFF00ull
#define DC_LPE_REG_29_IB_STAT_FC_VIOLATION_SHIFT                0
#define DC_LPE_REG_29_IB_STAT_FC_VIOLATION_MASK                 0xFFull
#define DC_LPE_REG_29_IB_STAT_FC_VIOLATION_SMASK                0xFFull
/*
* Table #41 of 240_CSR_pcslpe.xml - lpe_reg_2A
* See description below for individual field definition
*/
#define DC_LPE_REG_2A                                           (DC_PCSLPE_LPE + 0x000000000150)
#define DC_LPE_REG_2A_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_2A_LPE_RX_MAJOR_ERROR_DISABLE_MASK_SHIFT     24
#define DC_LPE_REG_2A_LPE_RX_MAJOR_ERROR_DISABLE_MASK_MASK      0xFFull
#define DC_LPE_REG_2A_LPE_RX_MAJOR_ERROR_DISABLE_MASK_SMASK     0xFF000000ull
#define DC_LPE_REG_2A_LPE_RX_EARLY_ERROR_ENABLE_MASK_ADV_SHIFT  17
#define DC_LPE_REG_2A_LPE_RX_EARLY_ERROR_ENABLE_MASK_ADV_MASK   0x1ull
#define DC_LPE_REG_2A_LPE_RX_EARLY_ERROR_ENABLE_MASK_ADV_SMASK  0x20000ull
#define DC_LPE_REG_2A_LPE_RX_LATE_ERROR_ENABLE_MASK_SHIFT       8
#define DC_LPE_REG_2A_LPE_RX_LATE_ERROR_ENABLE_MASK_MASK        0x1FFull
#define DC_LPE_REG_2A_LPE_RX_LATE_ERROR_ENABLE_MASK_SMASK       0x1FF00ull
#define DC_LPE_REG_2A_LPE_RX_EARLY_ERROR_ENABLE_MASK_SHIFT      0
#define DC_LPE_REG_2A_LPE_RX_EARLY_ERROR_ENABLE_MASK_MASK       0xFFull
#define DC_LPE_REG_2A_LPE_RX_EARLY_ERROR_ENABLE_MASK_SMASK      0xFFull
/*
* Table #42 of 240_CSR_pcslpe.xml - lpe_reg_2B
* See description below for individual field definition
*/
#define DC_LPE_REG_2B                                           (DC_PCSLPE_LPE + 0x000000000158)
#define DC_LPE_REG_2B_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_2B_UNUSED_31_8_SHIFT                         8
#define DC_LPE_REG_2B_UNUSED_31_8_MASK                          0xFFFFFFull
#define DC_LPE_REG_2B_UNUSED_31_8_SMASK                         0xFFFFFF00ull
#define DC_LPE_REG_2B_LPE_RX_EARLY_ERROR_REPORT_MASK_SHIFT      0
#define DC_LPE_REG_2B_LPE_RX_EARLY_ERROR_REPORT_MASK_MASK       0xFFull
#define DC_LPE_REG_2B_LPE_RX_EARLY_ERROR_REPORT_MASK_SMASK      0xFFull
/*
* Table #43 of 240_CSR_pcslpe.xml - lpe_reg_2C
* See description below for individual field definition
*/
#define DC_LPE_REG_2C                                           (DC_PCSLPE_LPE + 0x000000000160)
#define DC_LPE_REG_2C_RESETCSR                                  0x00000101ull
#define DC_LPE_REG_2C_UNUSED_31_10_SHIFT                        10
#define DC_LPE_REG_2C_UNUSED_31_10_MASK                         0x3FFFFFull
#define DC_LPE_REG_2C_UNUSED_31_10_SMASK                        0xFFFFFC00ull
#define DC_LPE_REG_2C_IB_ENHANCE_MODE10_SHIFT                   8
#define DC_LPE_REG_2C_IB_ENHANCE_MODE10_MASK                    0x3ull
#define DC_LPE_REG_2C_IB_ENHANCE_MODE10_SMASK                   0x300ull
#define DC_LPE_REG_2C_UNUSED_7_2_SHIFT                          2
#define DC_LPE_REG_2C_UNUSED_7_2_MASK                           0x3Full
#define DC_LPE_REG_2C_UNUSED_7_2_SMASK                          0xFCull
#define DC_LPE_REG_2C_LPE_PKT_LENGTH_B11_DISABLE_SHIFT          1
#define DC_LPE_REG_2C_LPE_PKT_LENGTH_B11_DISABLE_MASK           0x1ull
#define DC_LPE_REG_2C_LPE_PKT_LENGTH_B11_DISABLE_SMASK          0x2ull
#define DC_LPE_REG_2C_IB_FORWARD_IN_ARM_SHIFT                   0
#define DC_LPE_REG_2C_IB_FORWARD_IN_ARM_MASK                    0x1ull
#define DC_LPE_REG_2C_IB_FORWARD_IN_ARM_SMASK                   0x1ull
/*
* Table #44 of 240_CSR_pcslpe.xml - lpe_reg_2D
* See description below for individual field definition
*/
#define DC_LPE_REG_2D                                           (DC_PCSLPE_LPE + 0x000000000168)
#define DC_LPE_REG_2D_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_2D_UNUSED_31_16_SHIFT                        16
#define DC_LPE_REG_2D_UNUSED_31_16_MASK                         0xFFFFull
#define DC_LPE_REG_2D_UNUSED_31_16_SMASK                        0xFFFF0000ull
#define DC_LPE_REG_2D_LIN_FDB_TOP_SHIFT                         0
#define DC_LPE_REG_2D_LIN_FDB_TOP_MASK                          0xFFFFull
#define DC_LPE_REG_2D_LIN_FDB_TOP_SMASK                         0xFFFFull
/*
* Table #45 of 240_CSR_pcslpe.xml - lpe_reg_2E
* See description below for individual field definition
*/
#define DC_LPE_REG_2E                                           (DC_PCSLPE_LPE + 0x000000000170)
#define DC_LPE_REG_2E_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_2E_UNUSED_31_6_SHIFT                         6
#define DC_LPE_REG_2E_UNUSED_31_6_MASK                          0x3FFFFFFull
#define DC_LPE_REG_2E_UNUSED_31_6_SMASK                         0xFFFFFFC0ull
#define DC_LPE_REG_2E_CFG_TEST_ACTIVE_SHIFT                     5
#define DC_LPE_REG_2E_CFG_TEST_ACTIVE_MASK                      0x1ull
#define DC_LPE_REG_2E_CFG_TEST_ACTIVE_SMASK                     0x20ull
#define DC_LPE_REG_2E_CFG_TEST_FRONT_PORCH_ACTIVE_SHIFT         4
#define DC_LPE_REG_2E_CFG_TEST_FRONT_PORCH_ACTIVE_MASK          0x1ull
#define DC_LPE_REG_2E_CFG_TEST_FRONT_PORCH_ACTIVE_SMASK         0x10ull
#define DC_LPE_REG_2E_CFG_TEST_BACK_PORCH_ACTIVE_SHIFT          3
#define DC_LPE_REG_2E_CFG_TEST_BACK_PORCH_ACTIVE_MASK           0x1ull
#define DC_LPE_REG_2E_CFG_TEST_BACK_PORCH_ACTIVE_SMASK          0x8ull
#define DC_LPE_REG_2E_LPE_FRONT_PORCH_INTERVAL_SHIFT            0
#define DC_LPE_REG_2E_LPE_FRONT_PORCH_INTERVAL_MASK             0x7ull
#define DC_LPE_REG_2E_LPE_FRONT_PORCH_INTERVAL_SMASK            0x7ull
/*
* Table #46 of 240_CSR_pcslpe.xml - lpe_reg_2F
* See description below for individual field definition
*/
#define DC_LPE_REG_2F                                           (DC_PCSLPE_LPE + 0x000000000178)
#define DC_LPE_REG_2F_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_2F_UNUSED_31_5_SHIFT                         5
#define DC_LPE_REG_2F_UNUSED_31_5_MASK                          0x7FFFFFFull
#define DC_LPE_REG_2F_UNUSED_31_5_SMASK                         0xFFFFFFE0ull
#define DC_LPE_REG_2F_SPEED_DISABLE_MASK_SHIFT                  0
#define DC_LPE_REG_2F_SPEED_DISABLE_MASK_MASK                   0x1Full
#define DC_LPE_REG_2F_SPEED_DISABLE_MASK_SMASK                  0x1Full
/*
* Table #47 of 240_CSR_pcslpe.xml - lpe_reg_30
* See description below for individual field definition
*/
#define DC_LPE_REG_30                                           (DC_PCSLPE_LPE + 0x000000000180)
#define DC_LPE_REG_30_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_30_UNUSED_31_11_SHIFT                        11
#define DC_LPE_REG_30_UNUSED_31_11_MASK                         0x1FFFFFull
#define DC_LPE_REG_30_UNUSED_31_11_SMASK                        0xFFFFF800ull
#define DC_LPE_REG_30_CFG_TEST_FAILED_SHIFT                     10
#define DC_LPE_REG_30_CFG_TEST_FAILED_MASK                      0x1ull
#define DC_LPE_REG_30_CFG_TEST_FAILED_SMASK                     0x400ull
#define DC_LPE_REG_30_EN_CFG_TEST_FAILED_SHIFT                  9
#define DC_LPE_REG_30_EN_CFG_TEST_FAILED_MASK                   0x1ull
#define DC_LPE_REG_30_EN_CFG_TEST_FAILED_SMASK                  0x200ull
#define DC_LPE_REG_30_FORCE_SPEED_DOWN_GRADE_SHIFT              8
#define DC_LPE_REG_30_FORCE_SPEED_DOWN_GRADE_MASK               0x1ull
#define DC_LPE_REG_30_FORCE_SPEED_DOWN_GRADE_SMASK              0x100ull
#define DC_LPE_REG_30_UNUSED_7_5_SHIFT                          5
#define DC_LPE_REG_30_UNUSED_7_5_MASK                           0x7ull
#define DC_LPE_REG_30_UNUSED_7_5_SMASK                          0xE0ull
#define DC_LPE_REG_30_TS1_LOCK_FOREVER_SHIFT                    4
#define DC_LPE_REG_30_TS1_LOCK_FOREVER_MASK                     0x1ull
#define DC_LPE_REG_30_TS1_LOCK_FOREVER_SMASK                    0x10ull
#define DC_LPE_REG_30_DEBOUNCE_SEND_IDLE_ENABLE_SHIFT           3
#define DC_LPE_REG_30_DEBOUNCE_SEND_IDLE_ENABLE_MASK            0x1ull
#define DC_LPE_REG_30_DEBOUNCE_SEND_IDLE_ENABLE_SMASK           0x8ull
#define DC_LPE_REG_30_ENBL_LINKUP_TO_DISABLE_SHIFT              2
#define DC_LPE_REG_30_ENBL_LINKUP_TO_DISABLE_MASK               0x1ull
#define DC_LPE_REG_30_ENBL_LINKUP_TO_DISABLE_SMASK              0x4ull
#define DC_LPE_REG_30_DSBL_TS3_TRAINING_SHIFT                   1
#define DC_LPE_REG_30_DSBL_TS3_TRAINING_MASK                    0x1ull
#define DC_LPE_REG_30_DSBL_TS3_TRAINING_SMASK                   0x2ull
#define DC_LPE_REG_30_DSBL_LINK_RECOVERY_SHIFT                  0
#define DC_LPE_REG_30_DSBL_LINK_RECOVERY_MASK                   0x1ull
#define DC_LPE_REG_30_DSBL_LINK_RECOVERY_SMASK                  0x1ull
/*
* Table #48 of 240_CSR_pcslpe.xml - lpe_reg_31
* See description below for individual field definition
*/
#define DC_LPE_REG_31                                           (DC_PCSLPE_LPE + 0x000000000188)
#define DC_LPE_REG_31_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_31_UNUSED_31_0_SHIFT                         0
#define DC_LPE_REG_31_UNUSED_31_0_MASK                          0xFFFFFFFFull
#define DC_LPE_REG_31_UNUSED_31_0_SMASK                         0xFFFFFFFFull
/*
* Table #49 of 240_CSR_pcslpe.xml - lpe_reg_32
* See description below for individual field definition
*/
#define DC_LPE_REG_32                                           (DC_PCSLPE_LPE + 0x000000000190)
#define DC_LPE_REG_32_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_32_UNUSED_31_2_SHIFT                         2
#define DC_LPE_REG_32_UNUSED_31_2_MASK                          0x3FFFFFFFull
#define DC_LPE_REG_32_UNUSED_31_2_SMASK                         0xFFFFFFFCull
#define DC_LPE_REG_32_RX_FORCE_MAJOR_ERROR_SHIFT                0
#define DC_LPE_REG_32_RX_FORCE_MAJOR_ERROR_MASK                 0x3ull
#define DC_LPE_REG_32_RX_FORCE_MAJOR_ERROR_SMASK                0x3ull
/*
* Table #50 of 240_CSR_pcslpe.xml - lpe_reg_33
* See description below for individual field definition
*/
#define DC_LPE_REG_33                                           (DC_PCSLPE_LPE + 0x000000000198)
#define DC_LPE_REG_33_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_33_UNUSED_31_9_SHIFT                         9
#define DC_LPE_REG_33_UNUSED_31_9_MASK                          0x7FFFFFull
#define DC_LPE_REG_33_UNUSED_31_9_SMASK                         0xFFFFFE00ull
#define DC_LPE_REG_33_VL_PKT_DISCARD_MASK_SHIFT                 0
#define DC_LPE_REG_33_VL_PKT_DISCARD_MASK_MASK                  0x1FFull
#define DC_LPE_REG_33_VL_PKT_DISCARD_MASK_SMASK                 0x1FFull
/*
* Table #51 of 240_CSR_pcslpe.xml - lpe_reg_36
* See description below for individual field definition
*/
#define DC_LPE_REG_36                                           (DC_PCSLPE_LPE + 0x0000000001B0)
#define DC_LPE_REG_36_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_36_UNUSED_31_0_SHIFT                         0
#define DC_LPE_REG_36_UNUSED_31_0_MASK                          0xFFFFFFFFull
#define DC_LPE_REG_36_UNUSED_31_0_SMASK                         0xFFFFFFFFull
/*
* Table #52 of 240_CSR_pcslpe.xml - lpe_reg_37
* See description below for individual field definition
*/
#define DC_LPE_REG_37                                           (DC_PCSLPE_LPE + 0x0000000001B8)
#define DC_LPE_REG_37_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_37_UNUSED_31_27_SHIFT                        27
#define DC_LPE_REG_37_UNUSED_31_27_MASK                         0x1Full
#define DC_LPE_REG_37_UNUSED_31_27_SMASK                        0xF8000000ull
#define DC_LPE_REG_37_FREEZE_LAST_SHIFT                         26
#define DC_LPE_REG_37_FREEZE_LAST_MASK                          0x1ull
#define DC_LPE_REG_37_FREEZE_LAST_SMASK                         0x4000000ull
#define DC_LPE_REG_37_CLEAR_LAST_SHIFT                          25
#define DC_LPE_REG_37_CLEAR_LAST_MASK                           0x1ull
#define DC_LPE_REG_37_CLEAR_LAST_SMASK                          0x2000000ull
#define DC_LPE_REG_37_CLEAR_FIRST_SHIFT                         24
#define DC_LPE_REG_37_CLEAR_FIRST_MASK                          0x1ull
#define DC_LPE_REG_37_CLEAR_FIRST_SMASK                         0x1000000ull
#define DC_LPE_REG_37_ERROR_STATE_SHIFT                         0
#define DC_LPE_REG_37_ERROR_STATE_MASK                          0xFFFFFFull
#define DC_LPE_REG_37_ERROR_STATE_SMASK                         0xFFFFFFull
/*
* Table #53 of 240_CSR_pcslpe.xml - lpe_reg_38
* See description below for individual field definition
*/
#define DC_LPE_REG_38                                           (DC_PCSLPE_LPE + 0x0000000001C0)
#define DC_LPE_REG_38_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_38_FIRST_ERR_LRH_0_SHIFT                     0
#define DC_LPE_REG_38_FIRST_ERR_LRH_0_MASK                      0xFFFFFFFFull
#define DC_LPE_REG_38_FIRST_ERR_LRH_0_SMASK                     0xFFFFFFFFull
/*
* Table #54 of 240_CSR_pcslpe.xml - lpe_reg_39
* See description below for individual field definition
*/
#define DC_LPE_REG_39                                           (DC_PCSLPE_LPE + 0x0000000001C8)
#define DC_LPE_REG_39_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_39_FIRST_ERR_LRH_1_SHIFT                     0
#define DC_LPE_REG_39_FIRST_ERR_LRH_1_MASK                      0xFFFFFFFFull
#define DC_LPE_REG_39_FIRST_ERR_LRH_1_SMASK                     0xFFFFFFFFull
/*
* Table #55 of 240_CSR_pcslpe.xml - lpe_reg_3A
* See description below for individual field definition
*/
#define DC_LPE_REG_3A                                           (DC_PCSLPE_LPE + 0x0000000001D0)
#define DC_LPE_REG_3A_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_3A_FIRST_ERR_BTH_0_SHIFT                     0
#define DC_LPE_REG_3A_FIRST_ERR_BTH_0_MASK                      0xFFFFFFFFull
#define DC_LPE_REG_3A_FIRST_ERR_BTH_0_SMASK                     0xFFFFFFFFull
/*
* Table #56 of 240_CSR_pcslpe.xml - lpe_reg_3B
* See description below for individual field definition
*/
#define DC_LPE_REG_3B                                           (DC_PCSLPE_LPE + 0x0000000001D8)
#define DC_LPE_REG_3B_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_3B_FIRST_ERR_BTH_1_SHIFT                     0
#define DC_LPE_REG_3B_FIRST_ERR_BTH_1_MASK                      0xFFFFFFFFull
#define DC_LPE_REG_3B_FIRST_ERR_BTH_1_SMASK                     0xFFFFFFFFull
/*
* Table #57 of 240_CSR_pcslpe.xml - lpe_reg_3C
* See description below for individual field definition
*/
#define DC_LPE_REG_3C                                           (DC_PCSLPE_LPE + 0x0000000001E0)
#define DC_LPE_REG_3C_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_3C_LAST_ERR_LRH_0_SHIFT                      0
#define DC_LPE_REG_3C_LAST_ERR_LRH_0_MASK                       0xFFFFFFFFull
#define DC_LPE_REG_3C_LAST_ERR_LRH_0_SMASK                      0xFFFFFFFFull
/*
* Table #58 of 240_CSR_pcslpe.xml - lpe_reg_3D
* See description below for individual field definition
*/
#define DC_LPE_REG_3D                                           (DC_PCSLPE_LPE + 0x0000000001E8)
#define DC_LPE_REG_3D_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_3D_LAST_ERR_LRH_1_SHIFT                      0
#define DC_LPE_REG_3D_LAST_ERR_LRH_1_MASK                       0xFFFFFFFFull
#define DC_LPE_REG_3D_LAST_ERR_LRH_1_SMASK                      0xFFFFFFFFull
/*
* Table #59 of 240_CSR_pcslpe.xml - lpe_reg_3E
* See description below for individual field definition
*/
#define DC_LPE_REG_3E                                           (DC_PCSLPE_LPE + 0x0000000001F0)
#define DC_LPE_REG_3E_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_3E_LAST_ERR_BTH_0_SHIFT                      0
#define DC_LPE_REG_3E_LAST_ERR_BTH_0_MASK                       0xFFFFFFFFull
#define DC_LPE_REG_3E_LAST_ERR_BTH_0_SMASK                      0xFFFFFFFFull
/*
* Table #60 of 240_CSR_pcslpe.xml - lpe_reg_3F
* See description below for individual field definition
*/
#define DC_LPE_REG_3F                                           (DC_PCSLPE_LPE + 0x0000000001F8)
#define DC_LPE_REG_3F_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_3F_LAST_ERR_BTH_1_SHIFT                      0
#define DC_LPE_REG_3F_LAST_ERR_BTH_1_MASK                       0xFFFFFFFFull
#define DC_LPE_REG_3F_LAST_ERR_BTH_1_SMASK                      0xFFFFFFFFull
/*
* Table #61 of 240_CSR_pcslpe.xml - lpe_reg_40
* See description below for individual field definition
*/
#define DC_LPE_REG_40                                           (DC_PCSLPE_LPE + 0x000000000200)
#define DC_LPE_REG_40_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_40_UNUSED_31_10_SHIFT                        10
#define DC_LPE_REG_40_UNUSED_31_10_MASK                         0x3FFFFFull
#define DC_LPE_REG_40_UNUSED_31_10_SMASK                        0xFFFFFC00ull
#define DC_LPE_REG_40_TRACE_BUFFER_FROZEN_SHIFT                 9
#define DC_LPE_REG_40_TRACE_BUFFER_FROZEN_MASK                  0x1ull
#define DC_LPE_REG_40_TRACE_BUFFER_FROZEN_SMASK                 0x200ull
#define DC_LPE_REG_40_SM_MATCH_INT_SHIFT                        8
#define DC_LPE_REG_40_SM_MATCH_INT_MASK                         0x1ull
#define DC_LPE_REG_40_SM_MATCH_INT_SMASK                        0x100ull
#define DC_LPE_REG_40_PORT_SM_MATCH_REG_SHIFT                   5
#define DC_LPE_REG_40_PORT_SM_MATCH_REG_MASK                    0x7ull
#define DC_LPE_REG_40_PORT_SM_MATCH_REG_SMASK                   0xE0ull
#define DC_LPE_REG_40_TRAINING_SM_MATCH_REG_SHIFT               0
#define DC_LPE_REG_40_TRAINING_SM_MATCH_REG_MASK                0x1Full
#define DC_LPE_REG_40_TRAINING_SM_MATCH_REG_SMASK               0x1Full
/*
* Table #62 of 240_CSR_pcslpe.xml - lpe_reg_41
* See description below for individual field definition
*/
#define DC_LPE_REG_41                                           (DC_PCSLPE_LPE + 0x000000000208)
#define DC_LPE_REG_41_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_41_UNUSED_31_13_SHIFT                        13
#define DC_LPE_REG_41_UNUSED_31_13_MASK                         0x7FFFFull
#define DC_LPE_REG_41_UNUSED_31_13_SMASK                        0xFFFFE000ull
#define DC_LPE_REG_41_TRACE_BUFFER_WRAP_EN_SHIFT                12
#define DC_LPE_REG_41_TRACE_BUFFER_WRAP_EN_MASK                 0x1ull
#define DC_LPE_REG_41_TRACE_BUFFER_WRAP_EN_SMASK                0x1000ull
#define DC_LPE_REG_41_UNUSED_11_10_SHIFT                        10
#define DC_LPE_REG_41_UNUSED_11_10_MASK                         0x3ull
#define DC_LPE_REG_41_UNUSED_11_10_SMASK                        0xC00ull
#define DC_LPE_REG_41_TRACE_BUFFER_LD_DIS_MASK_SHIFT            4
#define DC_LPE_REG_41_TRACE_BUFFER_LD_DIS_MASK_MASK             0x3Full
#define DC_LPE_REG_41_TRACE_BUFFER_LD_DIS_MASK_SMASK            0x3F0ull
#define DC_LPE_REG_41_RESET_TRACE_BUFFER_SHIFT                  3
#define DC_LPE_REG_41_RESET_TRACE_BUFFER_MASK                   0x1ull
#define DC_LPE_REG_41_RESET_TRACE_BUFFER_SMASK                  0x8ull
#define DC_LPE_REG_41_INT_ON_MATCH_SHIFT                        2
#define DC_LPE_REG_41_INT_ON_MATCH_MASK                         0x1ull
#define DC_LPE_REG_41_INT_ON_MATCH_SMASK                        0x4ull
#define DC_LPE_REG_41_FREEZE_ON_MATCH_SHIFT                     1
#define DC_LPE_REG_41_FREEZE_ON_MATCH_MASK                      0x1ull
#define DC_LPE_REG_41_FREEZE_ON_MATCH_SMASK                     0x2ull
#define DC_LPE_REG_41_FREEZE_TRACE_BUFFER_SHIFT                 0
#define DC_LPE_REG_41_FREEZE_TRACE_BUFFER_MASK                  0x1ull
#define DC_LPE_REG_41_FREEZE_TRACE_BUFFER_SMASK                 0x1ull
/*
* Table #63 of 240_CSR_pcslpe.xml - lpe_reg_42
* See description below for individual field definition
*/
#define DC_LPE_REG_42                                           (DC_PCSLPE_LPE + 0x000000000210)
#define DC_LPE_REG_42_RESETCSR                                  0x22000000ull
#define DC_LPE_REG_42_TB_PORT_STATE_SHIFT                       29
#define DC_LPE_REG_42_TB_PORT_STATE_MASK                        0x7ull
#define DC_LPE_REG_42_TB_PORT_STATE_SMASK                       0xE0000000ull
#define DC_LPE_REG_42_TB_TRAINING_STATE_SHIFT                   24
#define DC_LPE_REG_42_TB_TRAINING_STATE_MASK                    0x1Full
#define DC_LPE_REG_42_TB_TRAINING_STATE_SMASK                   0x1F000000ull
#define DC_LPE_REG_42_UNUSED_23_16_SHIFT                        16
#define DC_LPE_REG_42_UNUSED_23_16_MASK                         0xFFull
#define DC_LPE_REG_42_UNUSED_23_16_SMASK                        0xFF0000ull
#define DC_LPE_REG_42_SPARE_RA_SHIFT                            13
#define DC_LPE_REG_42_SPARE_RA_MASK                             0x7ull
#define DC_LPE_REG_42_SPARE_RA_SMASK                            0xE000ull
#define DC_LPE_REG_42_LINK_UP_GOT_TS_SHIFT                      12
#define DC_LPE_REG_42_LINK_UP_GOT_TS_MASK                       0x1ull
#define DC_LPE_REG_42_LINK_UP_GOT_TS_SMASK                      0x1000ull
#define DC_LPE_REG_42_TS3_OS_DETECTED_SHIFT                     8
#define DC_LPE_REG_42_TS3_OS_DETECTED_MASK                      0xFull
#define DC_LPE_REG_42_TS3_OS_DETECTED_SMASK                     0xF00ull
#define DC_LPE_REG_42_TS2_OS_DETECTED_SHIFT                     4
#define DC_LPE_REG_42_TS2_OS_DETECTED_MASK                      0xFull
#define DC_LPE_REG_42_TS2_OS_DETECTED_SMASK                     0xF0ull
#define DC_LPE_REG_42_TS1_OS_DETECTED_SHIFT                     0
#define DC_LPE_REG_42_TS1_OS_DETECTED_MASK                      0xFull
#define DC_LPE_REG_42_TS1_OS_DETECTED_SMASK                     0xFull
/*
* Table #64 of 240_CSR_pcslpe.xml - lpe_reg_43
* See description below for individual field definition
*/
#define DC_LPE_REG_43                                           (DC_PCSLPE_LPE + 0x000000000218)
#define DC_LPE_REG_43_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_43_TB_LINK_MAJOR_ERRORS_SHIFT                24
#define DC_LPE_REG_43_TB_LINK_MAJOR_ERRORS_MASK                 0xFFull
#define DC_LPE_REG_43_TB_LINK_MAJOR_ERRORS_SMASK                0xFF000000ull
#define DC_LPE_REG_43_TB_FC_ERRORS_SHIFT                        21
#define DC_LPE_REG_43_TB_FC_ERRORS_MASK                         0x7ull
#define DC_LPE_REG_43_TB_FC_ERRORS_SMASK                        0xE00000ull
#define DC_LPE_REG_43_TB_PKT_LATE_ERRORS_SHIFT                  12
#define DC_LPE_REG_43_TB_PKT_LATE_ERRORS_MASK                   0x1FFull
#define DC_LPE_REG_43_TB_PKT_LATE_ERRORS_SMASK                  0x1FF000ull
#define DC_LPE_REG_43_TB_PKT_EARLY_ERRORS_SHIFT                 0
#define DC_LPE_REG_43_TB_PKT_EARLY_ERRORS_MASK                  0xFFFull
#define DC_LPE_REG_43_TB_PKT_EARLY_ERRORS_SMASK                 0xFFFull
/*
* Table #65 of 240_CSR_pcslpe.xml - lpe_reg_44
* See description below for individual field definition
*/
#define DC_LPE_REG_44                                           (DC_PCSLPE_LPE + 0x000000000220)
#define DC_LPE_REG_44_RESETCSR                                  0x00000006ull
#define DC_LPE_REG_44_TB_TIME_STAMP_SHIFT                       0
#define DC_LPE_REG_44_TB_TIME_STAMP_MASK                        0xFFFFFFFFull
#define DC_LPE_REG_44_TB_TIME_STAMP_SMASK                       0xFFFFFFFFull
/*
* Table #66 of 240_CSR_pcslpe.xml - lpe_reg_48
* See description below for individual field definition
*/
#define DC_LPE_REG_48                                           (DC_PCSLPE_LPE + 0x000000000240)
#define DC_LPE_REG_48_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_48_UNUSED_31_7_SHIFT                         7
#define DC_LPE_REG_48_UNUSED_31_7_MASK                          0x1FFFFFFull
#define DC_LPE_REG_48_UNUSED_31_7_SMASK                         0xFFFFFF80ull
#define DC_LPE_REG_48_ENABLE_SAMPLE_PERIOD_SHIFT                6
#define DC_LPE_REG_48_ENABLE_SAMPLE_PERIOD_MASK                 0x1ull
#define DC_LPE_REG_48_ENABLE_SAMPLE_PERIOD_SMASK                0x40ull
#define DC_LPE_REG_48_FREEZE_IDLE_CNT_SHIFT                     5
#define DC_LPE_REG_48_FREEZE_IDLE_CNT_MASK                      0x1ull
#define DC_LPE_REG_48_FREEZE_IDLE_CNT_SMASK                     0x20ull
#define DC_LPE_REG_48_CLEAR_IDLE_CNT_SHIFT                      4
#define DC_LPE_REG_48_CLEAR_IDLE_CNT_MASK                       0x1ull
#define DC_LPE_REG_48_CLEAR_IDLE_CNT_SMASK                      0x10ull
#define DC_LPE_REG_48_IDLE_SAMPLE_PERIOD_SHIFT                  0
#define DC_LPE_REG_48_IDLE_SAMPLE_PERIOD_MASK                   0xFull
#define DC_LPE_REG_48_IDLE_SAMPLE_PERIOD_SMASK                  0xFull
/*
* Table #67 of 240_CSR_pcslpe.xml - lpe_reg_49
* See description below for individual field definition
*/
#define DC_LPE_REG_49                                           (DC_PCSLPE_LPE + 0x000000000248)
#define DC_LPE_REG_49_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_49_RX_IDLE_CNT_UPPER_SHIFT                   0
#define DC_LPE_REG_49_RX_IDLE_CNT_UPPER_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_49_RX_IDLE_CNT_UPPER_SMASK                   0xFFFFFFFFull
/*
* Table #68 of 240_CSR_pcslpe.xml - lpe_reg_4A
* See description below for individual field definition
*/
#define DC_LPE_REG_4A                                           (DC_PCSLPE_LPE + 0x000000000250)
#define DC_LPE_REG_4A_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_4A_RX_IDLE_CNT_LOWER_SHIFT                   0
#define DC_LPE_REG_4A_RX_IDLE_CNT_LOWER_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_4A_RX_IDLE_CNT_LOWER_SMASK                   0xFFFFFFFFull
/*
* Table #69 of 240_CSR_pcslpe.xml - lpe_reg_4B
* See description below for individual field definition
*/
#define DC_LPE_REG_4B                                           (DC_PCSLPE_LPE + 0x000000000258)
#define DC_LPE_REG_4B_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_4B_TX_IDLE_CNT_UPPER_SHIFT                   0
#define DC_LPE_REG_4B_TX_IDLE_CNT_UPPER_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_4B_TX_IDLE_CNT_UPPER_SMASK                   0xFFFFFFFFull
/*
* Table #70 of 240_CSR_pcslpe.xml - lpe_reg_4C
* See description below for individual field definition
*/
#define DC_LPE_REG_4C                                           (DC_PCSLPE_LPE + 0x000000000260)
#define DC_LPE_REG_4C_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_4C_TX_IDLE_CNT_LOWER_SHIFT                   0
#define DC_LPE_REG_4C_TX_IDLE_CNT_LOWER_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_4C_TX_IDLE_CNT_LOWER_SMASK                   0xFFFFFFFFull
/*
* Table #71 of 240_CSR_pcslpe.xml - lpe_reg_50
* See description below for individual field definition
*/
#define DC_LPE_REG_50                                           (DC_PCSLPE_LPE + 0x000000000280)
#define DC_LPE_REG_50_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_50_UNUSED_31_SHIFT                           31
#define DC_LPE_REG_50_UNUSED_31_MASK                            0x1ull
#define DC_LPE_REG_50_UNUSED_31_SMASK                           0x80000000ull
#define DC_LPE_REG_50_REG_0X40_INT_SHIFT                        30
#define DC_LPE_REG_50_REG_0X40_INT_MASK                         0x1ull
#define DC_LPE_REG_50_REG_0X40_INT_SMASK                        0x40000000ull
#define DC_LPE_REG_50_REG_0X30_INT_SHIFT                        29
#define DC_LPE_REG_50_REG_0X30_INT_MASK                         0x1ull
#define DC_LPE_REG_50_REG_0X30_INT_SMASK                        0x20000000ull
#define DC_LPE_REG_50_REG_0X2E_INT_SHIFT                        27
#define DC_LPE_REG_50_REG_0X2E_INT_MASK                         0x3ull
#define DC_LPE_REG_50_REG_0X2E_INT_SMASK                        0x18000000ull
#define DC_LPE_REG_50_REG_0X28_INT_SHIFT                        25
#define DC_LPE_REG_50_REG_0X28_INT_MASK                         0x3ull
#define DC_LPE_REG_50_REG_0X28_INT_SMASK                        0x6000000ull
#define DC_LPE_REG_50_REG_0X19_INT_SHIFT                        24
#define DC_LPE_REG_50_REG_0X19_INT_MASK                         0x1ull
#define DC_LPE_REG_50_REG_0X19_INT_SMASK                        0x1000000ull
#define DC_LPE_REG_50_REG_0X14_INT_SHIFT                        20
#define DC_LPE_REG_50_REG_0X14_INT_MASK                         0xFull
#define DC_LPE_REG_50_REG_0X14_INT_SMASK                        0xF00000ull
#define DC_LPE_REG_50_UNUSED_19_17_SHIFT                        17
#define DC_LPE_REG_50_UNUSED_19_17_MASK                         0x7ull
#define DC_LPE_REG_50_UNUSED_19_17_SMASK                        0xE0000ull
#define DC_LPE_REG_50_REG_0X0F_INT_SHIFT                        12
#define DC_LPE_REG_50_REG_0X0F_INT_MASK                         0x1Full
#define DC_LPE_REG_50_REG_0X0F_INT_SMASK                        0x1F000ull
#define DC_LPE_REG_50_UNUSED_11_10_SHIFT                        10
#define DC_LPE_REG_50_UNUSED_11_10_MASK                         0x3ull
#define DC_LPE_REG_50_UNUSED_11_10_SMASK                        0xC00ull
#define DC_LPE_REG_50_REG_0X0C_INT_SHIFT                        9
#define DC_LPE_REG_50_REG_0X0C_INT_MASK                         0x1ull
#define DC_LPE_REG_50_REG_0X0C_INT_SMASK                        0x200ull
#define DC_LPE_REG_50_REG_0X0A_INT_SHIFT                        8
#define DC_LPE_REG_50_REG_0X0A_INT_MASK                         0x1ull
#define DC_LPE_REG_50_REG_0X0A_INT_SMASK                        0x100ull
#define DC_LPE_REG_50_REG_0X01_INT_SHIFT                        0
#define DC_LPE_REG_50_REG_0X01_INT_MASK                         0xFFull
#define DC_LPE_REG_50_REG_0X01_INT_SMASK                        0xFFull
/*
* Table #72 of 240_CSR_pcslpe.xml - lpe_reg_51
* See description below for individual field definition
*/
#define DC_LPE_REG_51                                           (DC_PCSLPE_LPE + 0x000000000288)
#define DC_LPE_REG_51_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_51_TS_T_OS_DETECTED_INT_SHIFT                28
#define DC_LPE_REG_51_TS_T_OS_DETECTED_INT_MASK                 0xFull
#define DC_LPE_REG_51_TS_T_OS_DETECTED_INT_SMASK                0xF0000000ull
#define DC_LPE_REG_51_TS12_OS_ERR_DETECTED_INT_SHIFT            24
#define DC_LPE_REG_51_TS12_OS_ERR_DETECTED_INT_MASK             0xFull
#define DC_LPE_REG_51_TS12_OS_ERR_DETECTED_INT_SMASK            0xF000000ull
#define DC_LPE_REG_51_TS2_SEQ_DETECTED_INT_SHIFT                20
#define DC_LPE_REG_51_TS2_SEQ_DETECTED_INT_MASK                 0xFull
#define DC_LPE_REG_51_TS2_SEQ_DETECTED_INT_SMASK                0xF00000ull
#define DC_LPE_REG_51_TS1_SEQ_DETECTED_INT_SHIFT                16
#define DC_LPE_REG_51_TS1_SEQ_DETECTED_INT_MASK                 0xFull
#define DC_LPE_REG_51_TS1_SEQ_DETECTED_INT_SMASK                0xF0000ull
#define DC_LPE_REG_51_SKP_OS_DETECTED_INT_SHIFT                 12
#define DC_LPE_REG_51_SKP_OS_DETECTED_INT_MASK                  0xFull
#define DC_LPE_REG_51_SKP_OS_DETECTED_INT_SMASK                 0xF000ull
#define DC_LPE_REG_51_TS3_OS_DETECTED_INT_SHIFT                 8
#define DC_LPE_REG_51_TS3_OS_DETECTED_INT_MASK                  0xFull
#define DC_LPE_REG_51_TS3_OS_DETECTED_INT_SMASK                 0xF00ull
#define DC_LPE_REG_51_TS2_OS_DETECTED_INT_SHIFT                 4
#define DC_LPE_REG_51_TS2_OS_DETECTED_INT_MASK                  0xFull
#define DC_LPE_REG_51_TS2_OS_DETECTED_INT_SMASK                 0xF0ull
#define DC_LPE_REG_51_TS1_OS_DETECTED_INT_SHIFT                 0
#define DC_LPE_REG_51_TS1_OS_DETECTED_INT_MASK                  0xFull
#define DC_LPE_REG_51_TS1_OS_DETECTED_INT_SMASK                 0xFull
/*
* Table #73 of 240_CSR_pcslpe.xml - lpe_reg_52
* See description below for individual field definition
*/
#define DC_LPE_REG_52                                           (DC_PCSLPE_LPE + 0x000000000290)
#define DC_LPE_REG_52_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_52_UNUSED_31_22_SHIFT                        22
#define DC_LPE_REG_52_UNUSED_31_22_MASK                         0x3FFull
#define DC_LPE_REG_52_UNUSED_31_22_SMASK                        0xFFC00000ull
#define DC_LPE_REG_52_TX_REV_LANES_DETECTED_INT_SHIFT           20
#define DC_LPE_REG_52_TX_REV_LANES_DETECTED_INT_MASK            0x1ull
#define DC_LPE_REG_52_TX_REV_LANES_DETECTED_INT_SMASK           0x100000ull
#define DC_LPE_REG_52_LPE_RX_LATE_LOST_LINK_INT_SHIFT           19
#define DC_LPE_REG_52_LPE_RX_LATE_LOST_LINK_INT_MASK            0x1ull
#define DC_LPE_REG_52_LPE_RX_LATE_LOST_LINK_INT_SMASK           0x80000ull
#define DC_LPE_REG_52_LPE_RX_LATE_UNEXPECTED_CHAR_ERR_INT_SHIFT 18
#define DC_LPE_REG_52_LPE_RX_LATE_UNEXPECTED_CHAR_ERR_INT_MASK  0x1ull
#define DC_LPE_REG_52_LPE_RX_LATE_UNEXPECTED_CHAR_ERR_INT_SMASK 0x40000ull
#define DC_LPE_REG_52_UNUSED_17_SHIFT                           17
#define DC_LPE_REG_52_UNUSED_17_MASK                            0x1ull
#define DC_LPE_REG_52_UNUSED_17_SMASK                           0x20000ull
#define DC_LPE_REG_52_LPE_RX_LATE_CODE_ERR_INT_SHIFT            16
#define DC_LPE_REG_52_LPE_RX_LATE_CODE_ERR_INT_MASK             0x1ull
#define DC_LPE_REG_52_LPE_RX_LATE_CODE_ERR_INT_SMASK            0x10000ull
#define DC_LPE_REG_52_RX_ABR_ERROR_INT_SHIFT                    8
#define DC_LPE_REG_52_RX_ABR_ERROR_INT_MASK                     0xFFull
#define DC_LPE_REG_52_RX_ABR_ERROR_INT_SMASK                    0xFF00ull
#define DC_LPE_REG_52_IB_STAT_FC_VIOLATION_INT_SHIFT            0
#define DC_LPE_REG_52_IB_STAT_FC_VIOLATION_INT_MASK             0xFFull
#define DC_LPE_REG_52_IB_STAT_FC_VIOLATION_INT_SMASK            0xFFull
/*
* Table #74 of 240_CSR_pcslpe.xml - lpe_reg_54
* See description below for individual field definition
*/
#define DC_LPE_REG_54                                           (DC_PCSLPE_LPE + 0x0000000002A0)
#define DC_LPE_REG_54_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_54_UNUSED_31_SHIFT                           31
#define DC_LPE_REG_54_UNUSED_31_MASK                            0x1ull
#define DC_LPE_REG_54_UNUSED_31_SMASK                           0x80000000ull
#define DC_LPE_REG_54_REG_0X40_INT_MASK_SHIFT                   30
#define DC_LPE_REG_54_REG_0X40_INT_MASK_MASK                    0x1ull
#define DC_LPE_REG_54_REG_0X40_INT_MASK_SMASK                   0x40000000ull
#define DC_LPE_REG_54_REG_0X2F_INT_MASK_SHIFT                   28
#define DC_LPE_REG_54_REG_0X2F_INT_MASK_MASK                    0x3ull
#define DC_LPE_REG_54_REG_0X2F_INT_MASK_SMASK                   0x30000000ull
#define DC_LPE_REG_54_UNUSED_27_26_SHIFT                        26
#define DC_LPE_REG_54_UNUSED_27_26_MASK                         0x3ull
#define DC_LPE_REG_54_UNUSED_27_26_SMASK                        0xC000000ull
#define DC_LPE_REG_54_REG_0X28_INT_MASK_SHIFT                   25
#define DC_LPE_REG_54_REG_0X28_INT_MASK_MASK                    0x1ull
#define DC_LPE_REG_54_REG_0X28_INT_MASK_SMASK                   0x2000000ull
#define DC_LPE_REG_54_REG_0X19_INT_MASK_SHIFT                   24
#define DC_LPE_REG_54_REG_0X19_INT_MASK_MASK                    0x1ull
#define DC_LPE_REG_54_REG_0X19_INT_MASK_SMASK                   0x1000000ull
#define DC_LPE_REG_54_REG_0X14_INT_MASK_SHIFT                   20
#define DC_LPE_REG_54_REG_0X14_INT_MASK_MASK                    0xFull
#define DC_LPE_REG_54_REG_0X14_INT_MASK_SMASK                   0xF00000ull
#define DC_LPE_REG_54_UNUSED_19_17_SHIFT                        17
#define DC_LPE_REG_54_UNUSED_19_17_MASK                         0x7ull
#define DC_LPE_REG_54_UNUSED_19_17_SMASK                        0xE0000ull
#define DC_LPE_REG_54_REG_0X0F_INT_MASK_SHIFT                   12
#define DC_LPE_REG_54_REG_0X0F_INT_MASK_MASK                    0x1Full
#define DC_LPE_REG_54_REG_0X0F_INT_MASK_SMASK                   0x1F000ull
#define DC_LPE_REG_54_UNUSED_11_10_SHIFT                        10
#define DC_LPE_REG_54_UNUSED_11_10_MASK                         0x3ull
#define DC_LPE_REG_54_UNUSED_11_10_SMASK                        0xC00ull
#define DC_LPE_REG_54_REG_0X0C_INT_MASK_SHIFT                   9
#define DC_LPE_REG_54_REG_0X0C_INT_MASK_MASK                    0x1ull
#define DC_LPE_REG_54_REG_0X0C_INT_MASK_SMASK                   0x200ull
#define DC_LPE_REG_54_REG_0X0A_INT_MASK_SHIFT                   8
#define DC_LPE_REG_54_REG_0X0A_INT_MASK_MASK                    0x1ull
#define DC_LPE_REG_54_REG_0X0A_INT_MASK_SMASK                   0x100ull
#define DC_LPE_REG_54_REG_0X01_INT_MASK_SHIFT                   0
#define DC_LPE_REG_54_REG_0X01_INT_MASK_MASK                    0xFFull
#define DC_LPE_REG_54_REG_0X01_INT_MASK_SMASK                   0xFFull
/*
* Table #75 of 240_CSR_pcslpe.xml - lpe_reg_55
* See description below for individual field definition
*/
#define DC_LPE_REG_55                                           (DC_PCSLPE_LPE + 0x0000000002A8)
#define DC_LPE_REG_55_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_55_TS_T_OS_DETECTED_INT_MASK_SHIFT           28
#define DC_LPE_REG_55_TS_T_OS_DETECTED_INT_MASK_MASK            0xFull
#define DC_LPE_REG_55_TS_T_OS_DETECTED_INT_MASK_SMASK           0xF0000000ull
#define DC_LPE_REG_55_TS12_OS_ERR_DETECTED_INT_MASK_SHIFT       24
#define DC_LPE_REG_55_TS12_OS_ERR_DETECTED_INT_MASK_MASK        0xFull
#define DC_LPE_REG_55_TS12_OS_ERR_DETECTED_INT_MASK_SMASK       0xF000000ull
#define DC_LPE_REG_55_TS2_SEQ_DETECTED_INT_MASK_SHIFT           20
#define DC_LPE_REG_55_TS2_SEQ_DETECTED_INT_MASK_MASK            0xFull
#define DC_LPE_REG_55_TS2_SEQ_DETECTED_INT_MASK_SMASK           0xF00000ull
#define DC_LPE_REG_55_TS1_SEQ_DETECTED_INT_MASK_SHIFT           16
#define DC_LPE_REG_55_TS1_SEQ_DETECTED_INT_MASK_MASK            0xFull
#define DC_LPE_REG_55_TS1_SEQ_DETECTED_INT_MASK_SMASK           0xF0000ull
#define DC_LPE_REG_55_SKP_OS_DETECTED_INT_MASK_SHIFT            12
#define DC_LPE_REG_55_SKP_OS_DETECTED_INT_MASK_MASK             0xFull
#define DC_LPE_REG_55_SKP_OS_DETECTED_INT_MASK_SMASK            0xF000ull
#define DC_LPE_REG_55_TS3_OS_DETECTED_INT_MASK_SHIFT            8
#define DC_LPE_REG_55_TS3_OS_DETECTED_INT_MASK_MASK             0xFull
#define DC_LPE_REG_55_TS3_OS_DETECTED_INT_MASK_SMASK            0xF00ull
#define DC_LPE_REG_55_TS2_OS_DETECTED_INT_MASK_SHIFT            4
#define DC_LPE_REG_55_TS2_OS_DETECTED_INT_MASK_MASK             0xFull
#define DC_LPE_REG_55_TS2_OS_DETECTED_INT_MASK_SMASK            0xF0ull
#define DC_LPE_REG_55_TS1_OS_DETECTED_INT_MASK_SHIFT            0
#define DC_LPE_REG_55_TS1_OS_DETECTED_INT_MASK_MASK             0xFull
#define DC_LPE_REG_55_TS1_OS_DETECTED_INT_MASK_SMASK            0xFull
/*
* Table #76 of 240_CSR_pcslpe.xml - lpe_reg_56
* See description below for individual field definition
*/
#define DC_LPE_REG_56                                           (DC_PCSLPE_LPE + 0x0000000002B0)
#define DC_LPE_REG_56_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_56_UNUSED_31_22_SHIFT                        22
#define DC_LPE_REG_56_UNUSED_31_22_MASK                         0x3FFull
#define DC_LPE_REG_56_UNUSED_31_22_SMASK                        0xFFC00000ull
#define DC_LPE_REG_56_TX_REV_LANES_DETECTED_INT_MASK_SHIFT      20
#define DC_LPE_REG_56_TX_REV_LANES_DETECTED_INT_MASK_MASK       0x1ull
#define DC_LPE_REG_56_TX_REV_LANES_DETECTED_INT_MASK_SMASK      0x100000ull
#define DC_LPE_REG_56_LPE_RX_LATE_LOST_LINK_INT_MASK_SHIFT      19
#define DC_LPE_REG_56_LPE_RX_LATE_LOST_LINK_INT_MASK_MASK       0x1ull
#define DC_LPE_REG_56_LPE_RX_LATE_LOST_LINK_INT_MASK_SMASK      0x80000ull
#define DC_LPE_REG_56_LPE_RX_LATE_CTRL_ERR_INT_MASK_SHIFT       18
#define DC_LPE_REG_56_LPE_RX_LATE_CTRL_ERR_INT_MASK_MASK        0x1ull
#define DC_LPE_REG_56_LPE_RX_LATE_CTRL_ERR_INT_MASK_SMASK       0x40000ull
#define DC_LPE_REG_56_UNUSED_17_SHIFT                           17
#define DC_LPE_REG_56_UNUSED_17_MASK                            0x1ull
#define DC_LPE_REG_56_UNUSED_17_SMASK                           0x20000ull
#define DC_LPE_REG_56_LPE_RX_LATE_CODE_ERR_INT_MASK_SHIFT       16
#define DC_LPE_REG_56_LPE_RX_LATE_CODE_ERR_INT_MASK_MASK        0x1ull
#define DC_LPE_REG_56_LPE_RX_LATE_CODE_ERR_INT_MASK_SMASK       0x10000ull
#define DC_LPE_REG_56_RX_ABR_ERROR_INT_MASK_SHIFT               8
#define DC_LPE_REG_56_RX_ABR_ERROR_INT_MASK_MASK                0xFFull
#define DC_LPE_REG_56_RX_ABR_ERROR_INT_MASK_SMASK               0xFF00ull
#define DC_LPE_REG_56_IB_STAT_FC_VIOLATION_INT_MASK_SHIFT       0
#define DC_LPE_REG_56_IB_STAT_FC_VIOLATION_INT_MASK_MASK        0xFFull
#define DC_LPE_REG_56_IB_STAT_FC_VIOLATION_INT_MASK_SMASK       0xFFull
/*
* Table #2 of 240_CSR_pcslpe.xml - lpe_reg_0
* Register containing various mode and behavior signals. Flow Control Timer can 
* be disabled or accelerated.
*/
#define DC_LPE_REG_0                                            (DC_PCSLPE_LPE + 0x000000000000)
#define DC_LPE_REG_0_RESETCSR                                   0x11110000ull
#define DC_LPE_REG_0_VL_76_FC_PERIOD_SHIFT                      28
#define DC_LPE_REG_0_VL_76_FC_PERIOD_MASK                       0xFull
#define DC_LPE_REG_0_VL_76_FC_PERIOD_SMASK                      0xF0000000ull
#define DC_LPE_REG_0_VL_54_FC_PERIOD_SHIFT                      24
#define DC_LPE_REG_0_VL_54_FC_PERIOD_MASK                       0xFull
#define DC_LPE_REG_0_VL_54_FC_PERIOD_SMASK                      0xF000000ull
#define DC_LPE_REG_0_VL_32_FC_PERIOD_SHIFT                      20
#define DC_LPE_REG_0_VL_32_FC_PERIOD_MASK                       0xFull
#define DC_LPE_REG_0_VL_32_FC_PERIOD_SMASK                      0xF00000ull
#define DC_LPE_REG_0_VL_10_FC_PERIOD_SHIFT                      16
#define DC_LPE_REG_0_VL_10_FC_PERIOD_MASK                       0xFull
#define DC_LPE_REG_0_VL_10_FC_PERIOD_SMASK                      0xF0000ull
#define DC_LPE_REG_0_SEND_ALL_FLOW_CONTROL_SHIFT                15
#define DC_LPE_REG_0_SEND_ALL_FLOW_CONTROL_MASK                 0x1ull
#define DC_LPE_REG_0_SEND_ALL_FLOW_CONTROL_SMASK                0x8000ull
#define DC_LPE_REG_0_EXTERNAL_CCFT_SHIFT                        14
#define DC_LPE_REG_0_EXTERNAL_CCFT_MASK                         0x1ull
#define DC_LPE_REG_0_EXTERNAL_CCFT_SMASK                        0x4000ull
#define DC_LPE_REG_0_EXTERNAL_TS3_SHIFT                         13
#define DC_LPE_REG_0_EXTERNAL_TS3_MASK                          0x1ull
#define DC_LPE_REG_0_EXTERNAL_TS3_SMASK                         0x2000ull
#define DC_LPE_REG_0_HB_TIMER_ACCEL_SHIFT                       12
#define DC_LPE_REG_0_HB_TIMER_ACCEL_MASK                        0x1ull
#define DC_LPE_REG_0_HB_TIMER_ACCEL_SMASK                       0x1000ull
#define DC_LPE_REG_0_FC_TIMER_INHIBIT_SHIFT                     11
#define DC_LPE_REG_0_FC_TIMER_INHIBIT_MASK                      0x1ull
#define DC_LPE_REG_0_FC_TIMER_INHIBIT_SMASK                     0x800ull
#define DC_LPE_REG_0_FC_TIMER_ACCEL_SHIFT                       10
#define DC_LPE_REG_0_FC_TIMER_ACCEL_MASK                        0x1ull
#define DC_LPE_REG_0_FC_TIMER_ACCEL_SMASK                       0x400ull
#define DC_LPE_REG_0_LRH_LVER_EN_SHIFT                          9
#define DC_LPE_REG_0_LRH_LVER_EN_MASK                           0x1ull
#define DC_LPE_REG_0_LRH_LVER_EN_SMASK                          0x200ull
#define DC_LPE_REG_0_SKIP_ACCELERATE_SHIFT                      8
#define DC_LPE_REG_0_SKIP_ACCELERATE_MASK                       0x1ull
#define DC_LPE_REG_0_SKIP_ACCELERATE_SMASK                      0x100ull
#define DC_LPE_REG_0_TX_TEST_MODE_EN_SHIFT                      7
#define DC_LPE_REG_0_TX_TEST_MODE_EN_MASK                       0x1ull
#define DC_LPE_REG_0_TX_TEST_MODE_EN_SMASK                      0x80ull
#define DC_LPE_REG_0_LPE_TX_ECC_MARK_DIS_SHIFT                  6
#define DC_LPE_REG_0_LPE_TX_ECC_MARK_DIS_MASK                   0x1ull
#define DC_LPE_REG_0_LPE_TX_ECC_MARK_DIS_SMASK                  0x40ull
#define DC_LPE_REG_0_FORCE_RX_ACT_TRIG_SHIFT                    5
#define DC_LPE_REG_0_FORCE_RX_ACT_TRIG_MASK                     0x1ull
#define DC_LPE_REG_0_FORCE_RX_ACT_TRIG_SMASK                    0x20ull
#define DC_LPE_REG_0_AUTO_ACT_ENABLE_SHIFT                      4
#define DC_LPE_REG_0_AUTO_ACT_ENABLE_MASK                       0x1ull
#define DC_LPE_REG_0_AUTO_ACT_ENABLE_SMASK                      0x10ull
#define DC_LPE_REG_0_AUTO_ARM_ENABLE_SHIFT                      3
#define DC_LPE_REG_0_AUTO_ARM_ENABLE_MASK                       0x1ull
#define DC_LPE_REG_0_AUTO_ARM_ENABLE_SMASK                      0x8ull
#define DC_LPE_REG_0_LPE_LOOPBACK_SHIFT                         2
#define DC_LPE_REG_0_LPE_LOOPBACK_MASK                          0x1ull
#define DC_LPE_REG_0_LPE_LOOPBACK_SMASK                         0x4ull
#define DC_LPE_REG_0_LPE_LINK_ECC_GEN_EN_SHIFT                  1
#define DC_LPE_REG_0_LPE_LINK_ECC_GEN_EN_MASK                   0x1ull
#define DC_LPE_REG_0_LPE_LINK_ECC_GEN_EN_SMASK                  0x2ull
#define DC_LPE_REG_0_ACT_DEFER_PKT_FLOW_EN_SHIFT                0
#define DC_LPE_REG_0_ACT_DEFER_PKT_FLOW_EN_MASK                 0x1ull
#define DC_LPE_REG_0_ACT_DEFER_PKT_FLOW_EN_SMASK                0x1ull
/*
* Table #3 of 240_CSR_pcslpe.xml - lpe_reg_1
* See description below for individual field definition
*/
#define DC_LPE_REG_1                                            (DC_PCSLPE_LPE + 0x000000000008)
#define DC_LPE_REG_1_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_1_UNUSED_31_5_SHIFT                          5
#define DC_LPE_REG_1_UNUSED_31_5_MASK                           0x7FFFFFFull
#define DC_LPE_REG_1_UNUSED_31_5_SMASK                          0xFFFFFFE0ull
#define DC_LPE_REG_1_LOCAL_BUS_PE_SHIFT                         4
#define DC_LPE_REG_1_LOCAL_BUS_PE_MASK                          0x1ull
#define DC_LPE_REG_1_LOCAL_BUS_PE_SMASK                         0x10ull
#define DC_LPE_REG_1_REG_RD_PE_SHIFT                            3
#define DC_LPE_REG_1_REG_RD_PE_MASK                             0x1ull
#define DC_LPE_REG_1_REG_RD_PE_SMASK                            0x8ull
#define DC_LPE_REG_1_CP_ACTIVE_TRIGGER_SHIFT                    2
#define DC_LPE_REG_1_CP_ACTIVE_TRIGGER_MASK                     0x1ull
#define DC_LPE_REG_1_CP_ACTIVE_TRIGGER_SMASK                    0x4ull
#define DC_LPE_REG_1_RX_ACTIVE_TRIGGER_SHIFT                    1
#define DC_LPE_REG_1_RX_ACTIVE_TRIGGER_MASK                     0x1ull
#define DC_LPE_REG_1_RX_ACTIVE_TRIGGER_SMASK                    0x2ull
#define DC_LPE_REG_1_ACTIVE_ENABLE_SHIFT                        0
#define DC_LPE_REG_1_ACTIVE_ENABLE_MASK                         0x1ull
#define DC_LPE_REG_1_ACTIVE_ENABLE_SMASK                        0x1ull
/*
* Table #4 of 240_CSR_pcslpe.xml - lpe_reg_2
* See description below for individual field definition
*/
#define DC_LPE_REG_2                                            (DC_PCSLPE_LPE + 0x000000000010)
#define DC_LPE_REG_2_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_2_UNUSED_31_1_SHIFT                          1
#define DC_LPE_REG_2_UNUSED_31_1_MASK                           0x7FFFFFFFull
#define DC_LPE_REG_2_UNUSED_31_1_SMASK                          0xFFFFFFFEull
#define DC_LPE_REG_2_LPE_EB_RESET_SHIFT                         0
#define DC_LPE_REG_2_LPE_EB_RESET_MASK                          0x1ull
#define DC_LPE_REG_2_LPE_EB_RESET_SMASK                         0x1ull
/*
* Table #5 of 240_CSR_pcslpe.xml - lpe_reg_3
* See description below for individual field definition
*/
#define DC_LPE_REG_3                                            (DC_PCSLPE_LPE + 0x000000000018)
#define DC_LPE_REG_3_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_3_UNUSED_31_3_SHIFT                          3
#define DC_LPE_REG_3_UNUSED_31_3_MASK                           0x1FFFFFFFull
#define DC_LPE_REG_3_UNUSED_31_3_SMASK                          0xFFFFFFF8ull
#define DC_LPE_REG_3_EN_TX_ICRC_GEN_SHIFT                       2
#define DC_LPE_REG_3_EN_TX_ICRC_GEN_MASK                        0x1ull
#define DC_LPE_REG_3_EN_TX_ICRC_GEN_SMASK                       0x4ull
#define DC_LPE_REG_3_DIS_TX_ICRC_CHK_SHIFT                      1
#define DC_LPE_REG_3_DIS_TX_ICRC_CHK_MASK                       0x1ull
#define DC_LPE_REG_3_DIS_TX_ICRC_CHK_SMASK                      0x2ull
#define DC_LPE_REG_3_DIS_RX_ICRC_CHK_SHIFT                      0
#define DC_LPE_REG_3_DIS_RX_ICRC_CHK_MASK                       0x1ull
#define DC_LPE_REG_3_DIS_RX_ICRC_CHK_SMASK                      0x1ull
/*
* Table #6 of 240_CSR_pcslpe.xml - lpe_reg_5
* This register controls the 8b10b Scramble feature that uses the TS3 exchange 
* to negotiate enabling Scrambling of the 8b data prior to conversion to 10b 
* during transmission and after the 10b to 8b conversion in the receive 
* direction. This mode enable must be coordinated from both sides of the link to 
* assure that there is data coherency after the mode is entered and exited. The 
* scrambling is self-synchronizing in that the endpoints automatically adjust to 
* the flow without strict bit-wise endpoint coordination. If the TS3 byte 9[2] 
* is set on the received TS3, the remote node has the capability to engage in 
* Scrambling. The Force_Scramble mode bit overrides the received TS3 byte 9[2]. 
* We must set our transmitted TS3 byte 9[2] (Scramble Capability) and set our 
* local scramble capability bit. Then when we enter link state Config.Idle and 
* both received and transmitted TS3 bit 9 is set, we enable Scrambling and this 
* is persistent until we go back to polling.
*/
#define DC_LPE_REG_5                                            (DC_PCSLPE_LPE + 0x000000000028)
#define DC_LPE_REG_5_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_5_UNUSED_31_4_SHIFT                          4
#define DC_LPE_REG_5_UNUSED_31_4_MASK                           0xFFFFFFFull
#define DC_LPE_REG_5_UNUSED_31_4_SMASK                          0xFFFFFFF0ull
#define DC_LPE_REG_5_SCRAMBLING_ACTIVE_SHIFT                    3
#define DC_LPE_REG_5_SCRAMBLING_ACTIVE_MASK                     0x1ull
#define DC_LPE_REG_5_SCRAMBLING_ACTIVE_SMASK                    0x8ull
#define DC_LPE_REG_5_LOCAL_SCRAMBLE_CAPABILITY_SHIFT            2
#define DC_LPE_REG_5_LOCAL_SCRAMBLE_CAPABILITY_MASK             0x1ull
#define DC_LPE_REG_5_LOCAL_SCRAMBLE_CAPABILITY_SMASK            0x4ull
#define DC_LPE_REG_5_REMOTE_SCRAMBLE_CAPABILITY_SHIFT           1
#define DC_LPE_REG_5_REMOTE_SCRAMBLE_CAPABILITY_MASK            0x1ull
#define DC_LPE_REG_5_REMOTE_SCRAMBLE_CAPABILITY_SMASK           0x2ull
#define DC_LPE_REG_5_FORCE_SCRAMBLE_SHIFT                       0
#define DC_LPE_REG_5_FORCE_SCRAMBLE_MASK                        0x1ull
#define DC_LPE_REG_5_FORCE_SCRAMBLE_SMASK                       0x1ull
/*
* Table #7 of 240_CSR_pcslpe.xml - lpe_reg_8
* See description below for individual field definition
*/
#define DC_LPE_REG_8                                            (DC_PCSLPE_LPE + 0x000000000040)
#define DC_LPE_REG_8_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_8_UNUSED_31_20_SHIFT                         20
#define DC_LPE_REG_8_UNUSED_31_20_MASK                          0xFFFull
#define DC_LPE_REG_8_UNUSED_31_20_SMASK                         0xFFF00000ull
#define DC_LPE_REG_8_POLLING_HOLD_TIMEOUT_SHIFT                 18
#define DC_LPE_REG_8_POLLING_HOLD_TIMEOUT_MASK                  0x3ull
#define DC_LPE_REG_8_POLLING_HOLD_TIMEOUT_SMASK                 0xC0000ull
#define DC_LPE_REG_8_POLLING_ACTIVE_TIMEOUT_SHIFT               16
#define DC_LPE_REG_8_POLLING_ACTIVE_TIMEOUT_MASK                0x3ull
#define DC_LPE_REG_8_POLLING_ACTIVE_TIMEOUT_SMASK               0x30000ull
#define DC_LPE_REG_8_POLLING_QUIET_TIMEOUT_SHIFT                14
#define DC_LPE_REG_8_POLLING_QUIET_TIMEOUT_MASK                 0x3ull
#define DC_LPE_REG_8_POLLING_QUIET_TIMEOUT_SMASK                0xC000ull
#define DC_LPE_REG_8_DEBOUNCE_TIMEOUT_SHIFT                     12
#define DC_LPE_REG_8_DEBOUNCE_TIMEOUT_MASK                      0x3ull
#define DC_LPE_REG_8_DEBOUNCE_TIMEOUT_SMASK                     0x3000ull
#define DC_LPE_REG_8_RCVR_CFG_HOLD_TIMEOUT_SHIFT                10
#define DC_LPE_REG_8_RCVR_CFG_HOLD_TIMEOUT_MASK                 0x3ull
#define DC_LPE_REG_8_RCVR_CFG_HOLD_TIMEOUT_SMASK                0xC00ull
#define DC_LPE_REG_8_RCVR_CFG_TIMEOUT_SHIFT                     8
#define DC_LPE_REG_8_RCVR_CFG_TIMEOUT_MASK                      0x3ull
#define DC_LPE_REG_8_RCVR_CFG_TIMEOUT_SMASK                     0x300ull
#define DC_LPE_REG_8_WAIT_RMT_TIMEOUT_SHIFT                     6
#define DC_LPE_REG_8_WAIT_RMT_TIMEOUT_MASK                      0x3ull
#define DC_LPE_REG_8_WAIT_RMT_TIMEOUT_SMASK                     0xC0ull
#define DC_LPE_REG_8_WAIT_RMT_HOLD_TIMEOUT_SHIFT                4
#define DC_LPE_REG_8_WAIT_RMT_HOLD_TIMEOUT_MASK                 0x3ull
#define DC_LPE_REG_8_WAIT_RMT_HOLD_TIMEOUT_SMASK                0x30ull
#define DC_LPE_REG_8_TX_REV_LANES_HOLD_TIMEOUT_SHIFT            2
#define DC_LPE_REG_8_TX_REV_LANES_HOLD_TIMEOUT_MASK             0x3ull
#define DC_LPE_REG_8_TX_REV_LANES_HOLD_TIMEOUT_SMASK            0xCull
#define DC_LPE_REG_8_TX_REV_LANES_TIMEOUT_SHIFT                 0
#define DC_LPE_REG_8_TX_REV_LANES_TIMEOUT_MASK                  0x3ull
#define DC_LPE_REG_8_TX_REV_LANES_TIMEOUT_SMASK                 0x3ull
/*
* Table #8 of 240_CSR_pcslpe.xml - lpe_reg_9
* See description below for individual field definition
*/
#define DC_LPE_REG_9                                            (DC_PCSLPE_LPE + 0x000000000048)
#define DC_LPE_REG_9_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_9_UNUSED_31_26_SHIFT                         26
#define DC_LPE_REG_9_UNUSED_31_26_MASK                          0x3Full
#define DC_LPE_REG_9_UNUSED_31_26_SMASK                         0xFC000000ull
#define DC_LPE_REG_9_CFG_ENHANCED_TIMEOUT_SHIFT                 24
#define DC_LPE_REG_9_CFG_ENHANCED_TIMEOUT_MASK                  0x3ull
#define DC_LPE_REG_9_CFG_ENHANCED_TIMEOUT_SMASK                 0x3000000ull
#define DC_LPE_REG_9_CFG_ENHANCED_HOLD_TIMEOUT_SHIFT            22
#define DC_LPE_REG_9_CFG_ENHANCED_HOLD_TIMEOUT_MASK             0x3ull
#define DC_LPE_REG_9_CFG_ENHANCED_HOLD_TIMEOUT_SMASK            0xC00000ull
#define DC_LPE_REG_9_WAIT_CFG_ENHANCED_TIMEOUT_SHIFT            20
#define DC_LPE_REG_9_WAIT_CFG_ENHANCED_TIMEOUT_MASK             0x3ull
#define DC_LPE_REG_9_WAIT_CFG_ENHANCED_TIMEOUT_SMASK            0x300000ull
#define DC_LPE_REG_9_WAIT_RMT_TEST_TIMEOUT_SHIFT                18
#define DC_LPE_REG_9_WAIT_RMT_TEST_TIMEOUT_MASK                 0x3ull
#define DC_LPE_REG_9_WAIT_RMT_TEST_TIMEOUT_SMASK                0xC0000ull
#define DC_LPE_REG_9_CFG_IDLE_TIMEOUT_SHIFT                     16
#define DC_LPE_REG_9_CFG_IDLE_TIMEOUT_MASK                      0x3ull
#define DC_LPE_REG_9_CFG_IDLE_TIMEOUT_SMASK                     0x30000ull
#define DC_LPE_REG_9_CFG_TEST_TIMEOUT_SHIFT                     14
#define DC_LPE_REG_9_CFG_TEST_TIMEOUT_MASK                      0x3ull
#define DC_LPE_REG_9_CFG_TEST_TIMEOUT_SMASK                     0xC000ull
#define DC_LPE_REG_9_CFG_IDLE_HOLD_TIMEOUT_SHIFT                12
#define DC_LPE_REG_9_CFG_IDLE_HOLD_TIMEOUT_MASK                 0x3ull
#define DC_LPE_REG_9_CFG_IDLE_HOLD_TIMEOUT_SMASK                0x3000ull
#define DC_LPE_REG_9_LINK_UP_DETECTED_HOLD_TIMEOUT_SHIFT        10
#define DC_LPE_REG_9_LINK_UP_DETECTED_HOLD_TIMEOUT_MASK         0x3ull
#define DC_LPE_REG_9_LINK_UP_DETECTED_HOLD_TIMEOUT_SMASK        0xC00ull
#define DC_LPE_REG_9_LINK_UP_HOLD_TIMEOUT_SHIFT                 8
#define DC_LPE_REG_9_LINK_UP_HOLD_TIMEOUT_MASK                  0x3ull
#define DC_LPE_REG_9_LINK_UP_HOLD_TIMEOUT_SMASK                 0x300ull
#define DC_LPE_REG_9_REC_RE_TRAIN_HOLD_TIMEOUT_SHIFT            6
#define DC_LPE_REG_9_REC_RE_TRAIN_HOLD_TIMEOUT_MASK             0x3ull
#define DC_LPE_REG_9_REC_RE_TRAIN_HOLD_TIMEOUT_SMASK            0xC0ull
#define DC_LPE_REG_9_REC_WAIT_RMT_HOLD_TIMEOUT_SHIFT            4
#define DC_LPE_REG_9_REC_WAIT_RMT_HOLD_TIMEOUT_MASK             0x3ull
#define DC_LPE_REG_9_REC_WAIT_RMT_HOLD_TIMEOUT_SMASK            0x30ull
#define DC_LPE_REG_9_REC_IDLE_HOLD_TIMEOUT_SHIFT                2
#define DC_LPE_REG_9_REC_IDLE_HOLD_TIMEOUT_MASK                 0x3ull
#define DC_LPE_REG_9_REC_IDLE_HOLD_TIMEOUT_SMASK                0xCull
#define DC_LPE_REG_9_RECOVERY_TIMEOUT_SHIFT                     0
#define DC_LPE_REG_9_RECOVERY_TIMEOUT_MASK                      0x3ull
#define DC_LPE_REG_9_RECOVERY_TIMEOUT_SMASK                     0x3ull
/*
* Table #9 of 240_CSR_pcslpe.xml - lpe_reg_A
* See description below for individual field definition
*/
#define DC_LPE_REG_A                                            (DC_PCSLPE_LPE + 0x000000000050)
#define DC_LPE_REG_A_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_A_UNUSED_31_19_SHIFT                         19
#define DC_LPE_REG_A_UNUSED_31_19_MASK                          0x1FFFull
#define DC_LPE_REG_A_UNUSED_31_19_SMASK                         0xFFF80000ull
#define DC_LPE_REG_A_RCV_TS_T_VALID_SHIFT                       18
#define DC_LPE_REG_A_RCV_TS_T_VALID_MASK                        0x1ull
#define DC_LPE_REG_A_RCV_TS_T_VALID_SMASK                       0x40000ull
#define DC_LPE_REG_A_RCV_TS_T_LANE_SHIFT                        16
#define DC_LPE_REG_A_RCV_TS_T_LANE_MASK                         0x3ull
#define DC_LPE_REG_A_RCV_TS_T_LANE_SMASK                        0x30000ull
#define DC_LPE_REG_A_RCV_TS_T_SPEEDS_SHIFT                      8
#define DC_LPE_REG_A_RCV_TS_T_SPEEDS_MASK                       0xFFull
#define DC_LPE_REG_A_RCV_TS_T_SPEEDS_SMASK                      0xFF00ull
#define DC_LPE_REG_A_RCV_TS_T_OPCODE_SHIFT                      0
#define DC_LPE_REG_A_RCV_TS_T_OPCODE_MASK                       0xFFull
#define DC_LPE_REG_A_RCV_TS_T_OPCODE_SMASK                      0xFFull
/*
* Table #10 of 240_CSR_pcslpe.xml - lpe_reg_B
* See description below for individual field definition
*/
#define DC_LPE_REG_B                                            (DC_PCSLPE_LPE + 0x000000000058)
#define DC_LPE_REG_B_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_B_RCV_TS_T_TX_CFG_SHIFT                      16
#define DC_LPE_REG_B_RCV_TS_T_TX_CFG_MASK                       0xFFFFull
#define DC_LPE_REG_B_RCV_TS_T_TX_CFG_SMASK                      0xFFFF0000ull
#define DC_LPE_REG_B_RCV_TS_T_RX_CFG_SHIFT                      0
#define DC_LPE_REG_B_RCV_TS_T_RX_CFG_MASK                       0xFFFFull
#define DC_LPE_REG_B_RCV_TS_T_RX_CFG_SMASK                      0xFFFFull
/*
* Table #11 of 240_CSR_pcslpe.xml - lpe_reg_C
* See description below for individual field definition
*/
#define DC_LPE_REG_C                                            (DC_PCSLPE_LPE + 0x000000000060)
#define DC_LPE_REG_C_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_C_UNUSED_31_19_SHIFT                         19
#define DC_LPE_REG_C_UNUSED_31_19_MASK                          0x1FFFull
#define DC_LPE_REG_C_UNUSED_31_19_SMASK                         0xFFF80000ull
#define DC_LPE_REG_C_XMIT_TS_T_VALID_SHIFT                      18
#define DC_LPE_REG_C_XMIT_TS_T_VALID_MASK                       0x1ull
#define DC_LPE_REG_C_XMIT_TS_T_VALID_SMASK                      0x40000ull
#define DC_LPE_REG_C_XMIT_TS_T_LANE_SHIFT                       16
#define DC_LPE_REG_C_XMIT_TS_T_LANE_MASK                        0x3ull
#define DC_LPE_REG_C_XMIT_TS_T_LANE_SMASK                       0x30000ull
#define DC_LPE_REG_C_XMIT_TS_T_SPEEDS_SHIFT                     8
#define DC_LPE_REG_C_XMIT_TS_T_SPEEDS_MASK                      0xFFull
#define DC_LPE_REG_C_XMIT_TS_T_SPEEDS_SMASK                     0xFF00ull
#define DC_LPE_REG_C_XMIT_TS_T_OPCODE_SHIFT                     0
#define DC_LPE_REG_C_XMIT_TS_T_OPCODE_MASK                      0xFFull
#define DC_LPE_REG_C_XMIT_TS_T_OPCODE_SMASK                     0xFFull
/*
* Table #12 of 240_CSR_pcslpe.xml - lpe_reg_D
* See description below for individual field definition
*/
#define DC_LPE_REG_D                                            (DC_PCSLPE_LPE + 0x000000000068)
#define DC_LPE_REG_D_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_D_XMIT_TS_T_TX_CFG_SHIFT                     16
#define DC_LPE_REG_D_XMIT_TS_T_TX_CFG_MASK                      0xFFFFull
#define DC_LPE_REG_D_XMIT_TS_T_TX_CFG_SMASK                     0xFFFF0000ull
#define DC_LPE_REG_D_XMIT_TS_T_RX_CFG_SHIFT                     0
#define DC_LPE_REG_D_XMIT_TS_T_RX_CFG_MASK                      0xFFFFull
#define DC_LPE_REG_D_XMIT_TS_T_RX_CFG_SMASK                     0xFFFFull
/*
* Table #13 of 240_CSR_pcslpe.xml - lpe_reg_E
* Spare Register at this time.
*/
#define DC_LPE_REG_E                                            (DC_PCSLPE_LPE + 0x000000000070)
#define DC_LPE_REG_E_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_E_UNUSED_31_0_SHIFT                          0
#define DC_LPE_REG_E_UNUSED_31_0_MASK                           0xFFFFFFFFull
#define DC_LPE_REG_E_UNUSED_31_0_SMASK                          0xFFFFFFFFull
/*
* Table #14 of 240_CSR_pcslpe.xml - lpe_reg_F
* Heartbeats SND packets may be sent manually using the Heartbeat request or 
* sent automatically using Heartbeat auto control.  The PORT and GUID values are 
* placed in the Heartbeat SND packet.  Heartbeat ACK packets are sent when a 
* Heartbeat SND packet has been received.  The Heartbeat ACK packet must contain 
* the same GUID value from the Heartbeat SND packet received.  Heartbeat ACK 
* packets received must contain GUID value matching the GUID of the Heartbeat 
* SND packet.  If the GUID's do not match the ACK packet is ignored.  If a 
* Heartbeat SND packet is received with a GUID matching the port GUID, this 
* indicates channel crosstalk.
*/
#define DC_LPE_REG_F                                            (DC_PCSLPE_LPE + 0x000000000078)
#define DC_LPE_REG_F_RESETCSR                                   0x00000000ull
#define DC_LPE_REG_F_HB_SND_XMIT_VALID_SHIFT                    31
#define DC_LPE_REG_F_HB_SND_XMIT_VALID_MASK                     0x1ull
#define DC_LPE_REG_F_HB_SND_XMIT_VALID_SMASK                    0x80000000ull
#define DC_LPE_REG_F_HB_SND_RCVD_VALID_SHIFT                    30
#define DC_LPE_REG_F_HB_SND_RCVD_VALID_MASK                     0x1ull
#define DC_LPE_REG_F_HB_SND_RCVD_VALID_SMASK                    0x40000000ull
#define DC_LPE_REG_F_HB_ACK_XMIT_VALID_SHIFT                    29
#define DC_LPE_REG_F_HB_ACK_XMIT_VALID_MASK                     0x1ull
#define DC_LPE_REG_F_HB_ACK_XMIT_VALID_SMASK                    0x20000000ull
#define DC_LPE_REG_F_HB_ACK_RCVD_VALID_SHIFT                    28
#define DC_LPE_REG_F_HB_ACK_RCVD_VALID_MASK                     0x1ull
#define DC_LPE_REG_F_HB_ACK_RCVD_VALID_SMASK                    0x10000000ull
#define DC_LPE_REG_F_HB_TIMED_OUT_SHIFT                         27
#define DC_LPE_REG_F_HB_TIMED_OUT_MASK                          0x1ull
#define DC_LPE_REG_F_HB_TIMED_OUT_SMASK                         0x8000000ull
#define DC_LPE_REG_F_UNUSED_26_18_SHIFT                         18
#define DC_LPE_REG_F_UNUSED_26_18_MASK                          0x1FFull
#define DC_LPE_REG_F_UNUSED_26_18_SMASK                         0x7FC0000ull
#define DC_LPE_REG_F_UNUSED_17_16_SHIFT                         16
#define DC_LPE_REG_F_UNUSED_17_16_MASK                          0x3ull
#define DC_LPE_REG_F_UNUSED_17_16_SMASK                         0x30000ull
#define DC_LPE_REG_F_RCV_SND_HB_PORT_NUM_SHIFT                  8
#define DC_LPE_REG_F_RCV_SND_HB_PORT_NUM_MASK                   0xFFull
#define DC_LPE_REG_F_RCV_SND_HB_PORT_NUM_SMASK                  0xFF00ull
#define DC_LPE_REG_F_RCV_ACK_HB_PORT_NUM_SHIFT                  0
#define DC_LPE_REG_F_RCV_ACK_HB_PORT_NUM_MASK                   0xFFull
#define DC_LPE_REG_F_RCV_ACK_HB_PORT_NUM_SMASK                  0xFFull
/*
* Table #15 of 240_CSR_pcslpe.xml - lpe_reg_10
* See description below for individual field definition
*/
#define DC_LPE_REG_10                                           (DC_PCSLPE_LPE + 0x000000000080)
#define DC_LPE_REG_10_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_10_RCV_SND_HB_GUID_U_SHIFT                   0
#define DC_LPE_REG_10_RCV_SND_HB_GUID_U_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_10_RCV_SND_HB_GUID_U_SMASK                   0xFFFFFFFFull
/*
* Table #16 of 240_CSR_pcslpe.xml - lpe_reg_11
* See description below for individual field definition
*/
#define DC_LPE_REG_11                                           (DC_PCSLPE_LPE + 0x000000000088)
#define DC_LPE_REG_11_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_11_RCV_SND_HB_GUID_L_SHIFT                   0
#define DC_LPE_REG_11_RCV_SND_HB_GUID_L_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_11_RCV_SND_HB_GUID_L_SMASK                   0xFFFFFFFFull
/*
* Table #17 of 240_CSR_pcslpe.xml - lpe_reg_12
* See description below for individual field definition
*/
#define DC_LPE_REG_12                                           (DC_PCSLPE_LPE + 0x000000000090)
#define DC_LPE_REG_12_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_12_RCV_ACK_HB_GUID_U_SHIFT                   0
#define DC_LPE_REG_12_RCV_ACK_HB_GUID_U_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_12_RCV_ACK_HB_GUID_U_SMASK                   0xFFFFFFFFull
/*
* Table #18 of 240_CSR_pcslpe.xml - lpe_reg_13
* See description below for individual field definition
*/
#define DC_LPE_REG_13                                           (DC_PCSLPE_LPE + 0x000000000098)
#define DC_LPE_REG_13_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_13_RCV_ACK_HB_GUID_L_SHIFT                   0
#define DC_LPE_REG_13_RCV_ACK_HB_GUID_L_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_13_RCV_ACK_HB_GUID_L_SMASK                   0xFFFFFFFFull
/*
* Table #19 of 240_CSR_pcslpe.xml - lpe_reg_14
* See description below for individual field definition
*/
#define DC_LPE_REG_14                                           (DC_PCSLPE_LPE + 0x0000000000A0)
#define DC_LPE_REG_14_RESETCSR                                  0x000F0000ull
#define DC_LPE_REG_14_UNUSED_31_28_SHIFT                        28
#define DC_LPE_REG_14_UNUSED_31_28_MASK                         0xFull
#define DC_LPE_REG_14_UNUSED_31_28_SMASK                        0xF0000000ull
#define DC_LPE_REG_14_HB_CROSSTALK_SHIFT                        24
#define DC_LPE_REG_14_HB_CROSSTALK_MASK                         0xFull
#define DC_LPE_REG_14_HB_CROSSTALK_SMASK                        0xF000000ull
#define DC_LPE_REG_14_UNUSED_23_SHIFT                           23
#define DC_LPE_REG_14_UNUSED_23_MASK                            0x1ull
#define DC_LPE_REG_14_UNUSED_23_SMASK                           0x800000ull
#define DC_LPE_REG_14_XMIT_HB_ENABLE_SHIFT                      22
#define DC_LPE_REG_14_XMIT_HB_ENABLE_MASK                       0x1ull
#define DC_LPE_REG_14_XMIT_HB_ENABLE_SMASK                      0x400000ull
#define DC_LPE_REG_14_XMIT_HB_REQUEST_SHIFT                     21
#define DC_LPE_REG_14_XMIT_HB_REQUEST_MASK                      0x1ull
#define DC_LPE_REG_14_XMIT_HB_REQUEST_SMASK                     0x200000ull
#define DC_LPE_REG_14_XMIT_HB_AUTO_SHIFT                        20
#define DC_LPE_REG_14_XMIT_HB_AUTO_MASK                         0x1ull
#define DC_LPE_REG_14_XMIT_HB_AUTO_SMASK                        0x100000ull
#define DC_LPE_REG_14_XMIT_HB_LANE_MASK_SHIFT                   16
#define DC_LPE_REG_14_XMIT_HB_LANE_MASK_MASK                    0xFull
#define DC_LPE_REG_14_XMIT_HB_LANE_MASK_SMASK                   0xF0000ull
#define DC_LPE_REG_14_XMIT_HB_OPCODE_SHIFT                      8
#define DC_LPE_REG_14_XMIT_HB_OPCODE_MASK                       0xFFull
#define DC_LPE_REG_14_XMIT_HB_OPCODE_SMASK                      0xFF00ull
#define DC_LPE_REG_14_XMIT_HB_PORT_NUM_SHIFT                    0
#define DC_LPE_REG_14_XMIT_HB_PORT_NUM_MASK                     0xFFull
#define DC_LPE_REG_14_XMIT_HB_PORT_NUM_SMASK                    0xFFull
/*
* Table #20 of 240_CSR_pcslpe.xml - lpe_reg_15
* See description below for individual field definition
*/
#define DC_LPE_REG_15                                           (DC_PCSLPE_LPE + 0x0000000000A8)
#define DC_LPE_REG_15_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_15_XMIT_HB_GUID_UPPR_SHIFT                   0
#define DC_LPE_REG_15_XMIT_HB_GUID_UPPR_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_15_XMIT_HB_GUID_UPPR_SMASK                   0xFFFFFFFFull
/*
* Table #21 of 240_CSR_pcslpe.xml - lpe_reg_16
* See description below for individual field definition
*/
#define DC_LPE_REG_16                                           (DC_PCSLPE_LPE + 0x0000000000B0)
#define DC_LPE_REG_16_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_16_XMIT_HB_GUID_LOWR_SHIFT                   0
#define DC_LPE_REG_16_XMIT_HB_GUID_LOWR_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_16_XMIT_HB_GUID_LOWR_SMASK                   0xFFFFFFFFull
/*
* Table #22 of 240_CSR_pcslpe.xml - lpe_reg_17
* See description below for individual field definition
*/
#define DC_LPE_REG_17                                           (DC_PCSLPE_LPE + 0x0000000000B8)
#define DC_LPE_REG_17_RESETCSR                                  0x07FFFFFFull
#define DC_LPE_REG_17_UNUSED_31_27_SHIFT                        27
#define DC_LPE_REG_17_UNUSED_31_27_MASK                         0x1Full
#define DC_LPE_REG_17_UNUSED_31_27_SMASK                        0xF8000000ull
#define DC_LPE_REG_17_HB_LATENCY_SHIFT                          0
#define DC_LPE_REG_17_HB_LATENCY_MASK                           0x7FFFFFFull
#define DC_LPE_REG_17_HB_LATENCY_SMASK                          0x7FFFFFFull
/*
* Table #23 of 240_CSR_pcslpe.xml - lpe_reg_18
* See description below for individual field definition
*/
#define DC_LPE_REG_18                                           (DC_PCSLPE_LPE + 0x0000000000C0)
#define DC_LPE_REG_18_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_18_TS_T_OS_DETECTED_SHIFT                    28
#define DC_LPE_REG_18_TS_T_OS_DETECTED_MASK                     0xFull
#define DC_LPE_REG_18_TS_T_OS_DETECTED_SMASK                    0xF0000000ull
#define DC_LPE_REG_18_TS12_OS_ERR_DETECTED_SHIFT                24
#define DC_LPE_REG_18_TS12_OS_ERR_DETECTED_MASK                 0xFull
#define DC_LPE_REG_18_TS12_OS_ERR_DETECTED_SMASK                0xF000000ull
#define DC_LPE_REG_18_TS2_SEQ_DETECTED_SHIFT                    20
#define DC_LPE_REG_18_TS2_SEQ_DETECTED_MASK                     0xFull
#define DC_LPE_REG_18_TS2_SEQ_DETECTED_SMASK                    0xF00000ull
#define DC_LPE_REG_18_TS1_SEQ_DETECTED_SHIFT                    16
#define DC_LPE_REG_18_TS1_SEQ_DETECTED_MASK                     0xFull
#define DC_LPE_REG_18_TS1_SEQ_DETECTED_SMASK                    0xF0000ull
#define DC_LPE_REG_18_SKP_OS_DETECTED_SHIFT                     12
#define DC_LPE_REG_18_SKP_OS_DETECTED_MASK                      0xFull
#define DC_LPE_REG_18_SKP_OS_DETECTED_SMASK                     0xF000ull
#define DC_LPE_REG_18_TS3_OS_DETECTED_SHIFT                     8
#define DC_LPE_REG_18_TS3_OS_DETECTED_MASK                      0xFull
#define DC_LPE_REG_18_TS3_OS_DETECTED_SMASK                     0xF00ull
#define DC_LPE_REG_18_TS2_OS_DETECTED_SHIFT                     4
#define DC_LPE_REG_18_TS2_OS_DETECTED_MASK                      0xFull
#define DC_LPE_REG_18_TS2_OS_DETECTED_SMASK                     0xF0ull
#define DC_LPE_REG_18_TS1_OS_DETECTED_SHIFT                     0
#define DC_LPE_REG_18_TS1_OS_DETECTED_MASK                      0xFull
#define DC_LPE_REG_18_TS1_OS_DETECTED_SMASK                     0xFull
/*
* Table #24 of 240_CSR_pcslpe.xml - lpe_reg_19
* See description below for individual field definition
*/
#define DC_LPE_REG_19                                           (DC_PCSLPE_LPE + 0x0000000000C8)
#define DC_LPE_REG_19_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_19_L0_TS1_CNT_SHIFT                          24
#define DC_LPE_REG_19_L0_TS1_CNT_MASK                           0xFFull
#define DC_LPE_REG_19_L0_TS1_CNT_SMASK                          0xFF000000ull
/*
* Table #25 of 240_CSR_pcslpe.xml - lpe_reg_1A
* See description below for individual field definition
*/
#define DC_LPE_REG_1A                                           (DC_PCSLPE_LPE + 0x0000000000D0)
#define DC_LPE_REG_1A_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_1A_L0_TS2_CNT_SHIFT                          24
#define DC_LPE_REG_1A_L0_TS2_CNT_MASK                           0xFFull
#define DC_LPE_REG_1A_L0_TS2_CNT_SMASK                          0xFF000000ull
/*
* Table #26 of 240_CSR_pcslpe.xml - lpe_reg_1B
* See description below for individual field definition
*/
#define DC_LPE_REG_1B                                           (DC_PCSLPE_LPE + 0x0000000000D8)
#define DC_LPE_REG_1B_RESETCSR                                  0x00080000ull
#define DC_LPE_REG_1B_TS12_ALIVE_CNT_THRES_SHIFT                16
#define DC_LPE_REG_1B_TS12_ALIVE_CNT_THRES_MASK                 0xFFFFull
#define DC_LPE_REG_1B_TS12_ALIVE_CNT_THRES_SMASK                0xFFFF0000ull
#define DC_LPE_REG_1B_LANE_ALIVE_SHIFT                          12
#define DC_LPE_REG_1B_LANE_ALIVE_MASK                           0xFull
#define DC_LPE_REG_1B_LANE_ALIVE_SMASK                          0xF000ull
#define DC_LPE_REG_1B_HOLD_TIME_LANE_ALIVE_SHIFT                8
#define DC_LPE_REG_1B_HOLD_TIME_LANE_ALIVE_MASK                 0xFull
#define DC_LPE_REG_1B_HOLD_TIME_LANE_ALIVE_SMASK                0xF00ull
#define DC_LPE_REG_1B_UNUSED_7_2_SHIFT                          2
#define DC_LPE_REG_1B_UNUSED_7_2_MASK                           0x3Full
#define DC_LPE_REG_1B_UNUSED_7_2_SMASK                          0xFCull
#define DC_LPE_REG_1B_OS_SEQ_CNT_SHIFT                          0
#define DC_LPE_REG_1B_OS_SEQ_CNT_MASK                           0x3ull
#define DC_LPE_REG_1B_OS_SEQ_CNT_SMASK                          0x3ull
/*
* Table #27 of 240_CSR_pcslpe.xml - lpe_reg_1C
* See description below for individual field definition
*/
#define DC_LPE_REG_1C                                           (DC_PCSLPE_LPE + 0x0000000000E0)
#define DC_LPE_REG_1C_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_1C_UNUSED_31_25_SHIFT                        25
#define DC_LPE_REG_1C_UNUSED_31_25_MASK                         0x7Full
#define DC_LPE_REG_1C_UNUSED_31_25_SMASK                        0xFE000000ull
#define DC_LPE_REG_1C_TX_MOD_TS1_EN_SHIFT                       24
#define DC_LPE_REG_1C_TX_MOD_TS1_EN_MASK                        0x1ull
#define DC_LPE_REG_1C_TX_MOD_TS1_EN_SMASK                       0x1000000ull
#define DC_LPE_REG_1C_TX_MOD_TS1_DATA_SHIFT                     0
#define DC_LPE_REG_1C_TX_MOD_TS1_DATA_MASK                      0xFFFFFFull
#define DC_LPE_REG_1C_TX_MOD_TS1_DATA_SMASK                     0xFFFFFFull
/*
* Table #28 of 240_CSR_pcslpe.xml - lpe_reg_1D
* See description below for individual field definition
*/
#define DC_LPE_REG_1D                                           (DC_PCSLPE_LPE + 0x0000000000E8)
#define DC_LPE_REG_1D_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_1D_UNUSED_31_25_SHIFT                        25
#define DC_LPE_REG_1D_UNUSED_31_25_MASK                         0x7Full
#define DC_LPE_REG_1D_UNUSED_31_25_SMASK                        0xFE000000ull
#define DC_LPE_REG_1D_RX_MOD_TS1_SAMPLED_SHIFT                  24
#define DC_LPE_REG_1D_RX_MOD_TS1_SAMPLED_MASK                   0x1ull
#define DC_LPE_REG_1D_RX_MOD_TS1_SAMPLED_SMASK                  0x1000000ull
#define DC_LPE_REG_1D_RX_MOD_TS1_DATA_SHIFT                     0
#define DC_LPE_REG_1D_RX_MOD_TS1_DATA_MASK                      0xFFFFFFull
#define DC_LPE_REG_1D_RX_MOD_TS1_DATA_SMASK                     0xFFFFFFull
/*
* Table #29 of 240_CSR_pcslpe.xml - lpe_reg_1E
* See description below for individual field definition
*/
#define DC_LPE_REG_1E                                           (DC_PCSLPE_LPE + 0x0000000000F0)
#define DC_LPE_REG_1E_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_1E_UNUSED_31_25_SHIFT                        25
#define DC_LPE_REG_1E_UNUSED_31_25_MASK                         0x7Full
#define DC_LPE_REG_1E_UNUSED_31_25_SMASK                        0xFE000000ull
#define DC_LPE_REG_1E_CFFT_TX_REQ_SHIFT                         24
#define DC_LPE_REG_1E_CFFT_TX_REQ_MASK                          0x1ull
#define DC_LPE_REG_1E_CFFT_TX_REQ_SMASK                         0x1000000ull
#define DC_LPE_REG_1E_CCFT_TX_CMD_SHIFT                         16
#define DC_LPE_REG_1E_CCFT_TX_CMD_MASK                          0xFFull
#define DC_LPE_REG_1E_CCFT_TX_CMD_SMASK                         0xFF0000ull
#define DC_LPE_REG_1E_CCFT_TX_STATUS_SHIFT                      8
#define DC_LPE_REG_1E_CCFT_TX_STATUS_MASK                       0xFFull
#define DC_LPE_REG_1E_CCFT_TX_STATUS_SMASK                      0xFF00ull
#define DC_LPE_REG_1E_CCFT_TX_LANE_SHIFT                        0
#define DC_LPE_REG_1E_CCFT_TX_LANE_MASK                         0xFFull
#define DC_LPE_REG_1E_CCFT_TX_LANE_SMASK                        0xFFull
/*
* Table #30 of 240_CSR_pcslpe.xml - lpe_reg_1F
* See description below for individual field definition
*/
#define DC_LPE_REG_1F                                           (DC_PCSLPE_LPE + 0x0000000000F8)
#define DC_LPE_REG_1F_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_1F_UNUSED_31_25_SHIFT                        25
#define DC_LPE_REG_1F_UNUSED_31_25_MASK                         0x7Full
#define DC_LPE_REG_1F_UNUSED_31_25_SMASK                        0xFE000000ull
#define DC_LPE_REG_1F_CCFT_RX_REQ_SHIFT                         24
#define DC_LPE_REG_1F_CCFT_RX_REQ_MASK                          0x1ull
#define DC_LPE_REG_1F_CCFT_RX_REQ_SMASK                         0x1000000ull
#define DC_LPE_REG_1F_CCFT_RX_CMD_SHIFT                         16
#define DC_LPE_REG_1F_CCFT_RX_CMD_MASK                          0xFFull
#define DC_LPE_REG_1F_CCFT_RX_CMD_SMASK                         0xFF0000ull
#define DC_LPE_REG_1F_CCFT_RX_STATUS_SHIFT                      8
#define DC_LPE_REG_1F_CCFT_RX_STATUS_MASK                       0xFFull
#define DC_LPE_REG_1F_CCFT_RX_STATUS_SMASK                      0xFF00ull
#define DC_LPE_REG_1F_CCFT_RX_LANE_SHIFT                        0
#define DC_LPE_REG_1F_CCFT_RX_LANE_MASK                         0xFFull
#define DC_LPE_REG_1F_CCFT_RX_LANE_SMASK                        0xFFull
/*
* Table #31 of 240_CSR_pcslpe.xml - lpe_reg_20
* See description below for individual field definition
*/
#define DC_LPE_REG_20                                           (DC_PCSLPE_LPE + 0x000000000100)
#define DC_LPE_REG_20_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_20_TS3_SEQ_RX_DATA_89_SHIFT                  31
#define DC_LPE_REG_20_TS3_SEQ_RX_DATA_89_MASK                   0x1ull
#define DC_LPE_REG_20_TS3_SEQ_RX_DATA_89_SMASK                  0x80000000ull
#define DC_LPE_REG_20_UNUSED_30_16_SHIFT                        16
#define DC_LPE_REG_20_UNUSED_30_16_MASK                         0x7FFFull
#define DC_LPE_REG_20_UNUSED_30_16_SMASK                        0x7FFF0000ull
#define DC_LPE_REG_20_TS3BYTE8_SHIFT                            8
#define DC_LPE_REG_20_TS3BYTE8_MASK                             0xFFull
#define DC_LPE_REG_20_TS3BYTE8_SMASK                            0xFF00ull
#define DC_LPE_REG_20_TS3BYTE9_SHIFT                            0
#define DC_LPE_REG_20_TS3BYTE9_MASK                             0xFFull
#define DC_LPE_REG_20_TS3BYTE9_SMASK                            0xFFull
/*
* Table #32 of 240_CSR_pcslpe.xml - lpe_reg_21
* See description below for individual field definition
*/
#define DC_LPE_REG_21                                           (DC_PCSLPE_LPE + 0x000000000108)
#define DC_LPE_REG_21_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_21_TS3_RX_DATA_10_LANE_0_SHIFT               24
#define DC_LPE_REG_21_TS3_RX_DATA_10_LANE_0_MASK                0xFFull
#define DC_LPE_REG_21_TS3_RX_DATA_10_LANE_0_SMASK               0xFF000000ull
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_1_SHIFT               16
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_1_MASK                0xFFull
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_1_SMASK               0xFF0000ull
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_2_SHIFT               8
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_2_MASK                0xFFull
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_2_SMASK               0xFF00ull
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_3_SHIFT               0
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_3_MASK                0xFFull
#define DC_LPE_REG_21_TS3_RX_BYTE_10_LANE_3_SMASK               0xFFull
/*
* Table #33 of 240_CSR_pcslpe.xml - lpe_reg_22
* See description below for individual field definition
*/
#define DC_LPE_REG_22                                           (DC_PCSLPE_LPE + 0x000000000110)
#define DC_LPE_REG_22_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_0_SHIFT               24
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_0_MASK                0xFFull
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_0_SMASK               0xFF000000ull
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_1_SHIFT               16
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_1_MASK                0xFFull
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_1_SMASK               0xFF0000ull
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_2_SHIFT               8
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_2_MASK                0xFFull
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_2_SMASK               0xFF00ull
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_3_SHIFT               0
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_3_MASK                0xFFull
#define DC_LPE_REG_22_TS3_RX_BYTE_11_LANE_3_SMASK               0xFFull
/*
* Table #34 of 240_CSR_pcslpe.xml - lpe_reg_23
* See description below for individual field definition
*/
#define DC_LPE_REG_23                                           (DC_PCSLPE_LPE + 0x000000000118)
#define DC_LPE_REG_23_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_23_TS3_RX_BYTE_12_LANE_0_SHIFT               24
#define DC_LPE_REG_23_TS3_RX_BYTE_12_LANE_0_MASK                0xFFull
#define DC_LPE_REG_23_TS3_RX_BYTE_12_LANE_0_SMASK               0xFF000000ull
#define DC_LPE_REG_23_TS3_RX_BYTE_13_LANE_1_SHIFT               16
#define DC_LPE_REG_23_TS3_RX_BYTE_13_LANE_1_MASK                0xFFull
#define DC_LPE_REG_23_TS3_RX_BYTE_13_LANE_1_SMASK               0xFF0000ull
#define DC_LPE_REG_23_TS3_RX_BYTE_14_LANE_2_SHIFT               8
#define DC_LPE_REG_23_TS3_RX_BYTE_14_LANE_2_MASK                0xFFull
#define DC_LPE_REG_23_TS3_RX_BYTE_14_LANE_2_SMASK               0xFF00ull
#define DC_LPE_REG_23_TS3_RX_BYTE_15_LANE_3_SHIFT               0
#define DC_LPE_REG_23_TS3_RX_BYTE_15_LANE_3_MASK                0xFFull
#define DC_LPE_REG_23_TS3_RX_BYTE_15_LANE_3_SMASK               0xFFull
/*
* Table #35 of 240_CSR_pcslpe.xml - lpe_reg_24
* The following TX TS3 bytes are used when sending TS3's. See description below 
* for individual field definition#%%#Address = Base Address + Offset
*/
#define DC_LPE_REG_24                                           (DC_PCSLPE_LPE + 0x000000000120)
#define DC_LPE_REG_24_RESETCSR                                  0x00001040ull
#define DC_LPE_REG_24_UNUSED_31_16_SHIFT                        16
#define DC_LPE_REG_24_UNUSED_31_16_MASK                         0xFFFFull
#define DC_LPE_REG_24_UNUSED_31_16_SMASK                        0xFFFF0000ull
#define DC_LPE_REG_24_TS3_TX_BYTE_8_SHIFT                       8
#define DC_LPE_REG_24_TS3_TX_BYTE_8_MASK                        0xFFull
#define DC_LPE_REG_24_TS3_TX_BYTE_8_SMASK                       0xFF00ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_TT_SHIFT                    4
#define DC_LPE_REG_24_TS3_TX_BYTE_9_TT_MASK                     0xFull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_TT_SMASK                    0xF0ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_FT_SHIFT                    3
#define DC_LPE_REG_24_TS3_TX_BYTE_9_FT_MASK                     0x1ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_FT_SMASK                    0x8ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_SC_SHIFT                    2
#define DC_LPE_REG_24_TS3_TX_BYTE_9_SC_MASK                     0x1ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_SC_SMASK                    0x4ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_HBR_SHIFT                   1
#define DC_LPE_REG_24_TS3_TX_BYTE_9_HBR_MASK                    0x1ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_HBR_SMASK                   0x2ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_ADD_SHIFT                   0
#define DC_LPE_REG_24_TS3_TX_BYTE_9_ADD_MASK                    0x1ull
#define DC_LPE_REG_24_TS3_TX_BYTE_9_ADD_SMASK                   0x1ull
/*
* Table #36 of 240_CSR_pcslpe.xml - lpe_reg_25
* See description below for individual field definition
*/
#define DC_LPE_REG_25                                           (DC_PCSLPE_LPE + 0x000000000128)
#define DC_LPE_REG_25_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_25_TS3_TX_DATA_10_LANE_0_SHIFT               24
#define DC_LPE_REG_25_TS3_TX_DATA_10_LANE_0_MASK                0xFFull
#define DC_LPE_REG_25_TS3_TX_DATA_10_LANE_0_SMASK               0xFF000000ull
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_1_SHIFT               16
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_1_MASK                0xFFull
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_1_SMASK               0xFF0000ull
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_2_SHIFT               8
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_2_MASK                0xFFull
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_2_SMASK               0xFF00ull
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_3_SHIFT               0
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_3_MASK                0xFFull
#define DC_LPE_REG_25_TS3_TX_BYTE_10_LANE_3_SMASK               0xFFull
/*
* Table #37 of 240_CSR_pcslpe.xml - lpe_reg_26
* See description below for individual field definition
*/
#define DC_LPE_REG_26                                           (DC_PCSLPE_LPE + 0x000000000130)
#define DC_LPE_REG_26_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_0_SHIFT               24
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_0_MASK                0xFFull
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_0_SMASK               0xFF000000ull
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_1_SHIFT               16
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_1_MASK                0xFFull
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_1_SMASK               0xFF0000ull
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_2_SHIFT               8
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_2_MASK                0xFFull
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_2_SMASK               0xFF00ull
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_3_SHIFT               0
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_3_MASK                0xFFull
#define DC_LPE_REG_26_TS3_RX_BYTE_11_LANE_3_SMASK               0xFFull
/*
* Table #38 of 240_CSR_pcslpe.xml - lpe_reg_27
* See description below for individual field definition
*/
#define DC_LPE_REG_27                                           (DC_PCSLPE_LPE + 0x000000000138)
#define DC_LPE_REG_27_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_27_TS3_TX_BYTE_12_SHIFT                      24
#define DC_LPE_REG_27_TS3_TX_BYTE_12_MASK                       0xFFull
#define DC_LPE_REG_27_TS3_TX_BYTE_12_SMASK                      0xFF000000ull
#define DC_LPE_REG_27_TS3_TX_BYTE_13_SHIFT                      16
#define DC_LPE_REG_27_TS3_TX_BYTE_13_MASK                       0xFFull
#define DC_LPE_REG_27_TS3_TX_BYTE_13_SMASK                      0xFF0000ull
#define DC_LPE_REG_27_TS3_TX_BYTE_14_SHIFT                      8
#define DC_LPE_REG_27_TS3_TX_BYTE_14_MASK                       0xFFull
#define DC_LPE_REG_27_TS3_TX_BYTE_14_SMASK                      0xFF00ull
#define DC_LPE_REG_27_TS3_TX_BYTE_15_SHIFT                      0
#define DC_LPE_REG_27_TS3_TX_BYTE_15_MASK                       0xFFull
#define DC_LPE_REG_27_TS3_TX_BYTE_15_SMASK                      0xFFull
/*
* Table #39 of 240_CSR_pcslpe.xml - lpe_reg_28
* This register shows the status of the 'rx_lane_order_a,b,c,d' and 
* 'rx_lane_polarity'. These are the calculated values needed to 'Un-Swizzle' the 
* data lanes into the required order of 3,2,1,0. These values are provided to 
* the PCSs so that they can use them in the 'Auto_Polarity_Rev' mode of 
* operation. NOTE: For EDR-Only mode of operation, the 'Auto_Polarity_Rev' bit 
* in the PCS_66 Common register must be set to a '0' and then the 
* 'Cmn_Rx_Lane_Order' and 'Cmn_Rx_Lane_Polarity' values must be prorammed to the 
* desired values for correct operation. This is necessary since we do not have 
* the 8b10b speed helping to discover the Swizzle solution - it must be provided 
* by the programmer. See description below for individual field 
* definition.
*/
#define DC_LPE_REG_28                                           (DC_PCSLPE_LPE + 0x000000000140)
#define DC_LPE_REG_28_RESETCSR                                  0x2E40E402ull
#define DC_LPE_REG_28_UNUSED_31_SHIFT                           31
#define DC_LPE_REG_28_UNUSED_31_MASK                            0x1ull
#define DC_LPE_REG_28_UNUSED_31_SMASK                           0x80000000ull
#define DC_LPE_REG_28_RX_TRAINED_TIMEOUT_SHIFT                  30
#define DC_LPE_REG_28_RX_TRAINED_TIMEOUT_MASK                   0x1ull
#define DC_LPE_REG_28_RX_TRAINED_TIMEOUT_SMASK                  0x40000000ull
#define DC_LPE_REG_28_RX_TRAINED_TST_EN_SHIFT                   29
#define DC_LPE_REG_28_RX_TRAINED_TST_EN_MASK                    0x1ull
#define DC_LPE_REG_28_RX_TRAINED_TST_EN_SMASK                   0x20000000ull
#define DC_LPE_REG_28_RX_TRAINED_SHIFT                          28
#define DC_LPE_REG_28_RX_TRAINED_MASK                           0x1ull
#define DC_LPE_REG_28_RX_TRAINED_SMASK                          0x10000000ull
#define DC_LPE_REG_28_RX_LANE_ORDER_D_SHIFT                     26
#define DC_LPE_REG_28_RX_LANE_ORDER_D_MASK                      0x3ull
#define DC_LPE_REG_28_RX_LANE_ORDER_D_SMASK                     0xC000000ull
#define DC_LPE_REG_28_RX_LANE_ORDER_C_SHIFT                     24
#define DC_LPE_REG_28_RX_LANE_ORDER_C_MASK                      0x3ull
#define DC_LPE_REG_28_RX_LANE_ORDER_C_SMASK                     0x3000000ull
#define DC_LPE_REG_28_RX_LANE_ORDER_B_SHIFT                     22
#define DC_LPE_REG_28_RX_LANE_ORDER_B_MASK                      0x3ull
#define DC_LPE_REG_28_RX_LANE_ORDER_B_SMASK                     0xC00000ull
#define DC_LPE_REG_28_RX_LANE_ORDER_A_SHIFT                     20
#define DC_LPE_REG_28_RX_LANE_ORDER_A_MASK                      0x3ull
#define DC_LPE_REG_28_RX_LANE_ORDER_A_SMASK                     0x300000ull
#define DC_LPE_REG_28_RX_LANE_POLARITY_SHIFT                    16
#define DC_LPE_REG_28_RX_LANE_POLARITY_MASK                     0xFull
#define DC_LPE_REG_28_RX_LANE_POLARITY_SMASK                    0xF0000ull
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_D_SHIFT                 14
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_D_MASK                  0x3ull
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_D_SMASK                 0xC000ull
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_C_SHIFT                 12
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_C_MASK                  0x3ull
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_C_SMASK                 0x3000ull
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_B_SHIFT                 10
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_B_MASK                  0x3ull
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_B_SMASK                 0xC00ull
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_A_SHIFT                 8
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_A_MASK                  0x3ull
#define DC_LPE_REG_28_RX_RAW_LANE_ORDER_A_SMASK                 0x300ull
#define DC_LPE_REG_28_RX_RAW_LANE_POLARITY_SHIFT                4
#define DC_LPE_REG_28_RX_RAW_LANE_POLARITY_MASK                 0xFull
#define DC_LPE_REG_28_RX_RAW_LANE_POLARITY_SMASK                0xF0ull
#define DC_LPE_REG_28_DISABLE_LINK_REC_SHIFT                    3
#define DC_LPE_REG_28_DISABLE_LINK_REC_MASK                     0x1ull
#define DC_LPE_REG_28_DISABLE_LINK_REC_SMASK                    0x8ull
#define DC_LPE_REG_28_TEST_IDLE_JMP_SHIFT                       2
#define DC_LPE_REG_28_TEST_IDLE_JMP_MASK                        0x1ull
#define DC_LPE_REG_28_TEST_IDLE_JMP_SMASK                       0x4ull
#define DC_LPE_REG_28_POLARITY_SWAP_SUPPORTED_SHIFT             1
#define DC_LPE_REG_28_POLARITY_SWAP_SUPPORTED_MASK              0x1ull
#define DC_LPE_REG_28_POLARITY_SWAP_SUPPORTED_SMASK             0x2ull
#define DC_LPE_REG_28_TX_LANE_REVERSE_SUPPORTED_SHIFT           0
#define DC_LPE_REG_28_TX_LANE_REVERSE_SUPPORTED_MASK            0x1ull
#define DC_LPE_REG_28_TX_LANE_REVERSE_SUPPORTED_SMASK           0x1ull
/*
* Table #40 of 240_CSR_pcslpe.xml - lpe_reg_29
* See description below for individual field definition
*/
#define DC_LPE_REG_29                                           (DC_PCSLPE_LPE + 0x000000000148)
#define DC_LPE_REG_29_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_29_VL_FC_TIMEOUT_SHIFT                       24
#define DC_LPE_REG_29_VL_FC_TIMEOUT_MASK                        0xFFull
#define DC_LPE_REG_29_VL_FC_TIMEOUT_SMASK                       0xFF000000ull
#define DC_LPE_REG_29_UNUSED_23_21_SHIFT                        21
#define DC_LPE_REG_29_UNUSED_23_21_MASK                         0x7ull
#define DC_LPE_REG_29_UNUSED_23_21_SMASK                        0xE00000ull
#define DC_LPE_REG_29_TX_REV_LANES_DETECTED_SHIFT               20
#define DC_LPE_REG_29_TX_REV_LANES_DETECTED_MASK                0x1ull
#define DC_LPE_REG_29_TX_REV_LANES_DETECTED_SMASK               0x100000ull
#define DC_LPE_REG_29_LPE_RX_LATE_LOST_LINK_SHIFT               19
#define DC_LPE_REG_29_LPE_RX_LATE_LOST_LINK_MASK                0x1ull
#define DC_LPE_REG_29_LPE_RX_LATE_LOST_LINK_SMASK               0x80000ull
#define DC_LPE_REG_29_LPE_RX_LATE_CTRL_ERR_SHIFT                18
#define DC_LPE_REG_29_LPE_RX_LATE_CTRL_ERR_MASK                 0x1ull
#define DC_LPE_REG_29_LPE_RX_LATE_CTRL_ERR_SMASK                0x40000ull
#define DC_LPE_REG_29_UNUSED_17_SHIFT                           17
#define DC_LPE_REG_29_UNUSED_17_MASK                            0x1ull
#define DC_LPE_REG_29_UNUSED_17_SMASK                           0x20000ull
#define DC_LPE_REG_29_LPE_RX_LATE_CODE_ERR_SHIFT                16
#define DC_LPE_REG_29_LPE_RX_LATE_CODE_ERR_MASK                 0x1ull
#define DC_LPE_REG_29_LPE_RX_LATE_CODE_ERR_SMASK                0x10000ull
#define DC_LPE_REG_29_RX_ABR_ERROR_SHIFT                        8
#define DC_LPE_REG_29_RX_ABR_ERROR_MASK                         0xFFull
#define DC_LPE_REG_29_RX_ABR_ERROR_SMASK                        0xFF00ull
#define DC_LPE_REG_29_IB_STAT_FC_VIOLATION_SHIFT                0
#define DC_LPE_REG_29_IB_STAT_FC_VIOLATION_MASK                 0xFFull
#define DC_LPE_REG_29_IB_STAT_FC_VIOLATION_SMASK                0xFFull
/*
* Table #41 of 240_CSR_pcslpe.xml - lpe_reg_2A
* See description below for individual field definition
*/
#define DC_LPE_REG_2A                                           (DC_PCSLPE_LPE + 0x000000000150)
#define DC_LPE_REG_2A_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_2A_LPE_RX_MAJOR_ERROR_DISABLE_MASK_SHIFT     24
#define DC_LPE_REG_2A_LPE_RX_MAJOR_ERROR_DISABLE_MASK_MASK      0xFFull
#define DC_LPE_REG_2A_LPE_RX_MAJOR_ERROR_DISABLE_MASK_SMASK     0xFF000000ull
#define DC_LPE_REG_2A_LPE_RX_EARLY_ERROR_ENABLE_MASK_ADV_SHIFT  17
#define DC_LPE_REG_2A_LPE_RX_EARLY_ERROR_ENABLE_MASK_ADV_MASK   0x1ull
#define DC_LPE_REG_2A_LPE_RX_EARLY_ERROR_ENABLE_MASK_ADV_SMASK  0x20000ull
#define DC_LPE_REG_2A_LPE_RX_LATE_ERROR_ENABLE_MASK_SHIFT       8
#define DC_LPE_REG_2A_LPE_RX_LATE_ERROR_ENABLE_MASK_MASK        0x1FFull
#define DC_LPE_REG_2A_LPE_RX_LATE_ERROR_ENABLE_MASK_SMASK       0x1FF00ull
#define DC_LPE_REG_2A_LPE_RX_EARLY_ERROR_ENABLE_MASK_SHIFT      0
#define DC_LPE_REG_2A_LPE_RX_EARLY_ERROR_ENABLE_MASK_MASK       0xFFull
#define DC_LPE_REG_2A_LPE_RX_EARLY_ERROR_ENABLE_MASK_SMASK      0xFFull
/*
* Table #42 of 240_CSR_pcslpe.xml - lpe_reg_2B
* See description below for individual field definition
*/
#define DC_LPE_REG_2B                                           (DC_PCSLPE_LPE + 0x000000000158)
#define DC_LPE_REG_2B_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_2B_UNUSED_31_8_SHIFT                         8
#define DC_LPE_REG_2B_UNUSED_31_8_MASK                          0xFFFFFFull
#define DC_LPE_REG_2B_UNUSED_31_8_SMASK                         0xFFFFFF00ull
#define DC_LPE_REG_2B_LPE_RX_EARLY_ERROR_REPORT_MASK_SHIFT      0
#define DC_LPE_REG_2B_LPE_RX_EARLY_ERROR_REPORT_MASK_MASK       0xFFull
#define DC_LPE_REG_2B_LPE_RX_EARLY_ERROR_REPORT_MASK_SMASK      0xFFull
/*
* Table #43 of 240_CSR_pcslpe.xml - lpe_reg_2C
* See description below for individual field definition
*/
#define DC_LPE_REG_2C                                           (DC_PCSLPE_LPE + 0x000000000160)
#define DC_LPE_REG_2C_RESETCSR                                  0x00000101ull
#define DC_LPE_REG_2C_UNUSED_31_10_SHIFT                        10
#define DC_LPE_REG_2C_UNUSED_31_10_MASK                         0x3FFFFFull
#define DC_LPE_REG_2C_UNUSED_31_10_SMASK                        0xFFFFFC00ull
#define DC_LPE_REG_2C_IB_ENHANCE_MODE10_SHIFT                   8
#define DC_LPE_REG_2C_IB_ENHANCE_MODE10_MASK                    0x3ull
#define DC_LPE_REG_2C_IB_ENHANCE_MODE10_SMASK                   0x300ull
#define DC_LPE_REG_2C_UNUSED_7_2_SHIFT                          2
#define DC_LPE_REG_2C_UNUSED_7_2_MASK                           0x3Full
#define DC_LPE_REG_2C_UNUSED_7_2_SMASK                          0xFCull
#define DC_LPE_REG_2C_LPE_PKT_LENGTH_B11_DISABLE_SHIFT          1
#define DC_LPE_REG_2C_LPE_PKT_LENGTH_B11_DISABLE_MASK           0x1ull
#define DC_LPE_REG_2C_LPE_PKT_LENGTH_B11_DISABLE_SMASK          0x2ull
#define DC_LPE_REG_2C_IB_FORWARD_IN_ARM_SHIFT                   0
#define DC_LPE_REG_2C_IB_FORWARD_IN_ARM_MASK                    0x1ull
#define DC_LPE_REG_2C_IB_FORWARD_IN_ARM_SMASK                   0x1ull
/*
* Table #44 of 240_CSR_pcslpe.xml - lpe_reg_2D
* See description below for individual field definition
*/
#define DC_LPE_REG_2D                                           (DC_PCSLPE_LPE + 0x000000000168)
#define DC_LPE_REG_2D_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_2D_UNUSED_31_16_SHIFT                        16
#define DC_LPE_REG_2D_UNUSED_31_16_MASK                         0xFFFFull
#define DC_LPE_REG_2D_UNUSED_31_16_SMASK                        0xFFFF0000ull
#define DC_LPE_REG_2D_LIN_FDB_TOP_SHIFT                         0
#define DC_LPE_REG_2D_LIN_FDB_TOP_MASK                          0xFFFFull
#define DC_LPE_REG_2D_LIN_FDB_TOP_SMASK                         0xFFFFull
/*
* Table #45 of 240_CSR_pcslpe.xml - lpe_reg_2E
* See description below for individual field definition
*/
#define DC_LPE_REG_2E                                           (DC_PCSLPE_LPE + 0x000000000170)
#define DC_LPE_REG_2E_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_2E_UNUSED_31_6_SHIFT                         6
#define DC_LPE_REG_2E_UNUSED_31_6_MASK                          0x3FFFFFFull
#define DC_LPE_REG_2E_UNUSED_31_6_SMASK                         0xFFFFFFC0ull
#define DC_LPE_REG_2E_CFG_TEST_ACTIVE_SHIFT                     5
#define DC_LPE_REG_2E_CFG_TEST_ACTIVE_MASK                      0x1ull
#define DC_LPE_REG_2E_CFG_TEST_ACTIVE_SMASK                     0x20ull
#define DC_LPE_REG_2E_CFG_TEST_FRONT_PORCH_ACTIVE_SHIFT         4
#define DC_LPE_REG_2E_CFG_TEST_FRONT_PORCH_ACTIVE_MASK          0x1ull
#define DC_LPE_REG_2E_CFG_TEST_FRONT_PORCH_ACTIVE_SMASK         0x10ull
#define DC_LPE_REG_2E_CFG_TEST_BACK_PORCH_ACTIVE_SHIFT          3
#define DC_LPE_REG_2E_CFG_TEST_BACK_PORCH_ACTIVE_MASK           0x1ull
#define DC_LPE_REG_2E_CFG_TEST_BACK_PORCH_ACTIVE_SMASK          0x8ull
#define DC_LPE_REG_2E_LPE_FRONT_PORCH_INTERVAL_SHIFT            0
#define DC_LPE_REG_2E_LPE_FRONT_PORCH_INTERVAL_MASK             0x7ull
#define DC_LPE_REG_2E_LPE_FRONT_PORCH_INTERVAL_SMASK            0x7ull
/*
* Table #46 of 240_CSR_pcslpe.xml - lpe_reg_2F
* See description below for individual field definition
*/
#define DC_LPE_REG_2F                                           (DC_PCSLPE_LPE + 0x000000000178)
#define DC_LPE_REG_2F_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_2F_UNUSED_31_5_SHIFT                         5
#define DC_LPE_REG_2F_UNUSED_31_5_MASK                          0x7FFFFFFull
#define DC_LPE_REG_2F_UNUSED_31_5_SMASK                         0xFFFFFFE0ull
#define DC_LPE_REG_2F_SPEED_DISABLE_MASK_SHIFT                  0
#define DC_LPE_REG_2F_SPEED_DISABLE_MASK_MASK                   0x1Full
#define DC_LPE_REG_2F_SPEED_DISABLE_MASK_SMASK                  0x1Full
/*
* Table #47 of 240_CSR_pcslpe.xml - lpe_reg_30
* See description below for individual field definition
*/
#define DC_LPE_REG_30                                           (DC_PCSLPE_LPE + 0x000000000180)
#define DC_LPE_REG_30_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_30_UNUSED_31_11_SHIFT                        11
#define DC_LPE_REG_30_UNUSED_31_11_MASK                         0x1FFFFFull
#define DC_LPE_REG_30_UNUSED_31_11_SMASK                        0xFFFFF800ull
#define DC_LPE_REG_30_CFG_TEST_FAILED_SHIFT                     10
#define DC_LPE_REG_30_CFG_TEST_FAILED_MASK                      0x1ull
#define DC_LPE_REG_30_CFG_TEST_FAILED_SMASK                     0x400ull
#define DC_LPE_REG_30_EN_CFG_TEST_FAILED_SHIFT                  9
#define DC_LPE_REG_30_EN_CFG_TEST_FAILED_MASK                   0x1ull
#define DC_LPE_REG_30_EN_CFG_TEST_FAILED_SMASK                  0x200ull
#define DC_LPE_REG_30_FORCE_SPEED_DOWN_GRADE_SHIFT              8
#define DC_LPE_REG_30_FORCE_SPEED_DOWN_GRADE_MASK               0x1ull
#define DC_LPE_REG_30_FORCE_SPEED_DOWN_GRADE_SMASK              0x100ull
#define DC_LPE_REG_30_UNUSED_7_5_SHIFT                          5
#define DC_LPE_REG_30_UNUSED_7_5_MASK                           0x7ull
#define DC_LPE_REG_30_UNUSED_7_5_SMASK                          0xE0ull
#define DC_LPE_REG_30_TS1_LOCK_FOREVER_SHIFT                    4
#define DC_LPE_REG_30_TS1_LOCK_FOREVER_MASK                     0x1ull
#define DC_LPE_REG_30_TS1_LOCK_FOREVER_SMASK                    0x10ull
#define DC_LPE_REG_30_DEBOUNCE_SEND_IDLE_ENABLE_SHIFT           3
#define DC_LPE_REG_30_DEBOUNCE_SEND_IDLE_ENABLE_MASK            0x1ull
#define DC_LPE_REG_30_DEBOUNCE_SEND_IDLE_ENABLE_SMASK           0x8ull
#define DC_LPE_REG_30_ENBL_LINKUP_TO_DISABLE_SHIFT              2
#define DC_LPE_REG_30_ENBL_LINKUP_TO_DISABLE_MASK               0x1ull
#define DC_LPE_REG_30_ENBL_LINKUP_TO_DISABLE_SMASK              0x4ull
#define DC_LPE_REG_30_DSBL_TS3_TRAINING_SHIFT                   1
#define DC_LPE_REG_30_DSBL_TS3_TRAINING_MASK                    0x1ull
#define DC_LPE_REG_30_DSBL_TS3_TRAINING_SMASK                   0x2ull
#define DC_LPE_REG_30_DSBL_LINK_RECOVERY_SHIFT                  0
#define DC_LPE_REG_30_DSBL_LINK_RECOVERY_MASK                   0x1ull
#define DC_LPE_REG_30_DSBL_LINK_RECOVERY_SMASK                  0x1ull
/*
* Table #48 of 240_CSR_pcslpe.xml - lpe_reg_31
* See description below for individual field definition
*/
#define DC_LPE_REG_31                                           (DC_PCSLPE_LPE + 0x000000000188)
#define DC_LPE_REG_31_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_31_UNUSED_31_0_SHIFT                         0
#define DC_LPE_REG_31_UNUSED_31_0_MASK                          0xFFFFFFFFull
#define DC_LPE_REG_31_UNUSED_31_0_SMASK                         0xFFFFFFFFull
/*
* Table #49 of 240_CSR_pcslpe.xml - lpe_reg_32
* See description below for individual field definition
*/
#define DC_LPE_REG_32                                           (DC_PCSLPE_LPE + 0x000000000190)
#define DC_LPE_REG_32_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_32_UNUSED_31_2_SHIFT                         2
#define DC_LPE_REG_32_UNUSED_31_2_MASK                          0x3FFFFFFFull
#define DC_LPE_REG_32_UNUSED_31_2_SMASK                         0xFFFFFFFCull
#define DC_LPE_REG_32_RX_FORCE_MAJOR_ERROR_SHIFT                0
#define DC_LPE_REG_32_RX_FORCE_MAJOR_ERROR_MASK                 0x3ull
#define DC_LPE_REG_32_RX_FORCE_MAJOR_ERROR_SMASK                0x3ull
/*
* Table #50 of 240_CSR_pcslpe.xml - lpe_reg_33
* See description below for individual field definition
*/
#define DC_LPE_REG_33                                           (DC_PCSLPE_LPE + 0x000000000198)
#define DC_LPE_REG_33_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_33_UNUSED_31_9_SHIFT                         9
#define DC_LPE_REG_33_UNUSED_31_9_MASK                          0x7FFFFFull
#define DC_LPE_REG_33_UNUSED_31_9_SMASK                         0xFFFFFE00ull
#define DC_LPE_REG_33_VL_PKT_DISCARD_MASK_SHIFT                 0
#define DC_LPE_REG_33_VL_PKT_DISCARD_MASK_MASK                  0x1FFull
#define DC_LPE_REG_33_VL_PKT_DISCARD_MASK_SMASK                 0x1FFull
/*
* Table #51 of 240_CSR_pcslpe.xml - lpe_reg_36
* See description below for individual field definition
*/
#define DC_LPE_REG_36                                           (DC_PCSLPE_LPE + 0x0000000001B0)
#define DC_LPE_REG_36_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_36_UNUSED_31_0_SHIFT                         0
#define DC_LPE_REG_36_UNUSED_31_0_MASK                          0xFFFFFFFFull
#define DC_LPE_REG_36_UNUSED_31_0_SMASK                         0xFFFFFFFFull
/*
* Table #52 of 240_CSR_pcslpe.xml - lpe_reg_37
* See description below for individual field definition
*/
#define DC_LPE_REG_37                                           (DC_PCSLPE_LPE + 0x0000000001B8)
#define DC_LPE_REG_37_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_37_UNUSED_31_27_SHIFT                        27
#define DC_LPE_REG_37_UNUSED_31_27_MASK                         0x1Full
#define DC_LPE_REG_37_UNUSED_31_27_SMASK                        0xF8000000ull
#define DC_LPE_REG_37_FREEZE_LAST_SHIFT                         26
#define DC_LPE_REG_37_FREEZE_LAST_MASK                          0x1ull
#define DC_LPE_REG_37_FREEZE_LAST_SMASK                         0x4000000ull
#define DC_LPE_REG_37_CLEAR_LAST_SHIFT                          25
#define DC_LPE_REG_37_CLEAR_LAST_MASK                           0x1ull
#define DC_LPE_REG_37_CLEAR_LAST_SMASK                          0x2000000ull
#define DC_LPE_REG_37_CLEAR_FIRST_SHIFT                         24
#define DC_LPE_REG_37_CLEAR_FIRST_MASK                          0x1ull
#define DC_LPE_REG_37_CLEAR_FIRST_SMASK                         0x1000000ull
#define DC_LPE_REG_37_ERROR_STATE_SHIFT                         0
#define DC_LPE_REG_37_ERROR_STATE_MASK                          0xFFFFFFull
#define DC_LPE_REG_37_ERROR_STATE_SMASK                         0xFFFFFFull
/*
* Table #53 of 240_CSR_pcslpe.xml - lpe_reg_38
* See description below for individual field definition
*/
#define DC_LPE_REG_38                                           (DC_PCSLPE_LPE + 0x0000000001C0)
#define DC_LPE_REG_38_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_38_FIRST_ERR_LRH_0_SHIFT                     0
#define DC_LPE_REG_38_FIRST_ERR_LRH_0_MASK                      0xFFFFFFFFull
#define DC_LPE_REG_38_FIRST_ERR_LRH_0_SMASK                     0xFFFFFFFFull
/*
* Table #54 of 240_CSR_pcslpe.xml - lpe_reg_39
* See description below for individual field definition
*/
#define DC_LPE_REG_39                                           (DC_PCSLPE_LPE + 0x0000000001C8)
#define DC_LPE_REG_39_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_39_FIRST_ERR_LRH_1_SHIFT                     0
#define DC_LPE_REG_39_FIRST_ERR_LRH_1_MASK                      0xFFFFFFFFull
#define DC_LPE_REG_39_FIRST_ERR_LRH_1_SMASK                     0xFFFFFFFFull
/*
* Table #55 of 240_CSR_pcslpe.xml - lpe_reg_3A
* See description below for individual field definition
*/
#define DC_LPE_REG_3A                                           (DC_PCSLPE_LPE + 0x0000000001D0)
#define DC_LPE_REG_3A_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_3A_FIRST_ERR_BTH_0_SHIFT                     0
#define DC_LPE_REG_3A_FIRST_ERR_BTH_0_MASK                      0xFFFFFFFFull
#define DC_LPE_REG_3A_FIRST_ERR_BTH_0_SMASK                     0xFFFFFFFFull
/*
* Table #56 of 240_CSR_pcslpe.xml - lpe_reg_3B
* See description below for individual field definition
*/
#define DC_LPE_REG_3B                                           (DC_PCSLPE_LPE + 0x0000000001D8)
#define DC_LPE_REG_3B_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_3B_FIRST_ERR_BTH_1_SHIFT                     0
#define DC_LPE_REG_3B_FIRST_ERR_BTH_1_MASK                      0xFFFFFFFFull
#define DC_LPE_REG_3B_FIRST_ERR_BTH_1_SMASK                     0xFFFFFFFFull
/*
* Table #57 of 240_CSR_pcslpe.xml - lpe_reg_3C
* See description below for individual field definition
*/
#define DC_LPE_REG_3C                                           (DC_PCSLPE_LPE + 0x0000000001E0)
#define DC_LPE_REG_3C_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_3C_LAST_ERR_LRH_0_SHIFT                      0
#define DC_LPE_REG_3C_LAST_ERR_LRH_0_MASK                       0xFFFFFFFFull
#define DC_LPE_REG_3C_LAST_ERR_LRH_0_SMASK                      0xFFFFFFFFull
/*
* Table #58 of 240_CSR_pcslpe.xml - lpe_reg_3D
* See description below for individual field definition
*/
#define DC_LPE_REG_3D                                           (DC_PCSLPE_LPE + 0x0000000001E8)
#define DC_LPE_REG_3D_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_3D_LAST_ERR_LRH_1_SHIFT                      0
#define DC_LPE_REG_3D_LAST_ERR_LRH_1_MASK                       0xFFFFFFFFull
#define DC_LPE_REG_3D_LAST_ERR_LRH_1_SMASK                      0xFFFFFFFFull
/*
* Table #59 of 240_CSR_pcslpe.xml - lpe_reg_3E
* See description below for individual field definition
*/
#define DC_LPE_REG_3E                                           (DC_PCSLPE_LPE + 0x0000000001F0)
#define DC_LPE_REG_3E_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_3E_LAST_ERR_BTH_0_SHIFT                      0
#define DC_LPE_REG_3E_LAST_ERR_BTH_0_MASK                       0xFFFFFFFFull
#define DC_LPE_REG_3E_LAST_ERR_BTH_0_SMASK                      0xFFFFFFFFull
/*
* Table #60 of 240_CSR_pcslpe.xml - lpe_reg_3F
* See description below for individual field definition
*/
#define DC_LPE_REG_3F                                           (DC_PCSLPE_LPE + 0x0000000001F8)
#define DC_LPE_REG_3F_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_3F_LAST_ERR_BTH_1_SHIFT                      0
#define DC_LPE_REG_3F_LAST_ERR_BTH_1_MASK                       0xFFFFFFFFull
#define DC_LPE_REG_3F_LAST_ERR_BTH_1_SMASK                      0xFFFFFFFFull
/*
* Table #61 of 240_CSR_pcslpe.xml - lpe_reg_40
* See description below for individual field definition
*/
#define DC_LPE_REG_40                                           (DC_PCSLPE_LPE + 0x000000000200)
#define DC_LPE_REG_40_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_40_UNUSED_31_10_SHIFT                        10
#define DC_LPE_REG_40_UNUSED_31_10_MASK                         0x3FFFFFull
#define DC_LPE_REG_40_UNUSED_31_10_SMASK                        0xFFFFFC00ull
#define DC_LPE_REG_40_TRACE_BUFFER_FROZEN_SHIFT                 9
#define DC_LPE_REG_40_TRACE_BUFFER_FROZEN_MASK                  0x1ull
#define DC_LPE_REG_40_TRACE_BUFFER_FROZEN_SMASK                 0x200ull
#define DC_LPE_REG_40_SM_MATCH_INT_SHIFT                        8
#define DC_LPE_REG_40_SM_MATCH_INT_MASK                         0x1ull
#define DC_LPE_REG_40_SM_MATCH_INT_SMASK                        0x100ull
#define DC_LPE_REG_40_PORT_SM_MATCH_REG_SHIFT                   5
#define DC_LPE_REG_40_PORT_SM_MATCH_REG_MASK                    0x7ull
#define DC_LPE_REG_40_PORT_SM_MATCH_REG_SMASK                   0xE0ull
#define DC_LPE_REG_40_TRAINING_SM_MATCH_REG_SHIFT               0
#define DC_LPE_REG_40_TRAINING_SM_MATCH_REG_MASK                0x1Full
#define DC_LPE_REG_40_TRAINING_SM_MATCH_REG_SMASK               0x1Full
/*
* Table #62 of 240_CSR_pcslpe.xml - lpe_reg_41
* See description below for individual field definition
*/
#define DC_LPE_REG_41                                           (DC_PCSLPE_LPE + 0x000000000208)
#define DC_LPE_REG_41_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_41_UNUSED_31_13_SHIFT                        13
#define DC_LPE_REG_41_UNUSED_31_13_MASK                         0x7FFFFull
#define DC_LPE_REG_41_UNUSED_31_13_SMASK                        0xFFFFE000ull
#define DC_LPE_REG_41_TRACE_BUFFER_WRAP_EN_SHIFT                12
#define DC_LPE_REG_41_TRACE_BUFFER_WRAP_EN_MASK                 0x1ull
#define DC_LPE_REG_41_TRACE_BUFFER_WRAP_EN_SMASK                0x1000ull
#define DC_LPE_REG_41_UNUSED_11_10_SHIFT                        10
#define DC_LPE_REG_41_UNUSED_11_10_MASK                         0x3ull
#define DC_LPE_REG_41_UNUSED_11_10_SMASK                        0xC00ull
#define DC_LPE_REG_41_TRACE_BUFFER_LD_DIS_MASK_SHIFT            4
#define DC_LPE_REG_41_TRACE_BUFFER_LD_DIS_MASK_MASK             0x3Full
#define DC_LPE_REG_41_TRACE_BUFFER_LD_DIS_MASK_SMASK            0x3F0ull
#define DC_LPE_REG_41_RESET_TRACE_BUFFER_SHIFT                  3
#define DC_LPE_REG_41_RESET_TRACE_BUFFER_MASK                   0x1ull
#define DC_LPE_REG_41_RESET_TRACE_BUFFER_SMASK                  0x8ull
#define DC_LPE_REG_41_INT_ON_MATCH_SHIFT                        2
#define DC_LPE_REG_41_INT_ON_MATCH_MASK                         0x1ull
#define DC_LPE_REG_41_INT_ON_MATCH_SMASK                        0x4ull
#define DC_LPE_REG_41_FREEZE_ON_MATCH_SHIFT                     1
#define DC_LPE_REG_41_FREEZE_ON_MATCH_MASK                      0x1ull
#define DC_LPE_REG_41_FREEZE_ON_MATCH_SMASK                     0x2ull
#define DC_LPE_REG_41_FREEZE_TRACE_BUFFER_SHIFT                 0
#define DC_LPE_REG_41_FREEZE_TRACE_BUFFER_MASK                  0x1ull
#define DC_LPE_REG_41_FREEZE_TRACE_BUFFER_SMASK                 0x1ull
/*
* Table #63 of 240_CSR_pcslpe.xml - lpe_reg_42
* See description below for individual field definition
*/
#define DC_LPE_REG_42                                           (DC_PCSLPE_LPE + 0x000000000210)
#define DC_LPE_REG_42_RESETCSR                                  0x22000000ull
#define DC_LPE_REG_42_TB_PORT_STATE_SHIFT                       29
#define DC_LPE_REG_42_TB_PORT_STATE_MASK                        0x7ull
#define DC_LPE_REG_42_TB_PORT_STATE_SMASK                       0xE0000000ull
#define DC_LPE_REG_42_TB_TRAINING_STATE_SHIFT                   24
#define DC_LPE_REG_42_TB_TRAINING_STATE_MASK                    0x1Full
#define DC_LPE_REG_42_TB_TRAINING_STATE_SMASK                   0x1F000000ull
#define DC_LPE_REG_42_UNUSED_23_16_SHIFT                        16
#define DC_LPE_REG_42_UNUSED_23_16_MASK                         0xFFull
#define DC_LPE_REG_42_UNUSED_23_16_SMASK                        0xFF0000ull
#define DC_LPE_REG_42_SPARE_RA_SHIFT                            13
#define DC_LPE_REG_42_SPARE_RA_MASK                             0x7ull
#define DC_LPE_REG_42_SPARE_RA_SMASK                            0xE000ull
#define DC_LPE_REG_42_LINK_UP_GOT_TS_SHIFT                      12
#define DC_LPE_REG_42_LINK_UP_GOT_TS_MASK                       0x1ull
#define DC_LPE_REG_42_LINK_UP_GOT_TS_SMASK                      0x1000ull
#define DC_LPE_REG_42_TS3_OS_DETECTED_SHIFT                     8
#define DC_LPE_REG_42_TS3_OS_DETECTED_MASK                      0xFull
#define DC_LPE_REG_42_TS3_OS_DETECTED_SMASK                     0xF00ull
#define DC_LPE_REG_42_TS2_OS_DETECTED_SHIFT                     4
#define DC_LPE_REG_42_TS2_OS_DETECTED_MASK                      0xFull
#define DC_LPE_REG_42_TS2_OS_DETECTED_SMASK                     0xF0ull
#define DC_LPE_REG_42_TS1_OS_DETECTED_SHIFT                     0
#define DC_LPE_REG_42_TS1_OS_DETECTED_MASK                      0xFull
#define DC_LPE_REG_42_TS1_OS_DETECTED_SMASK                     0xFull
/*
* Table #64 of 240_CSR_pcslpe.xml - lpe_reg_43
* See description below for individual field definition
*/
#define DC_LPE_REG_43                                           (DC_PCSLPE_LPE + 0x000000000218)
#define DC_LPE_REG_43_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_43_TB_LINK_MAJOR_ERRORS_SHIFT                24
#define DC_LPE_REG_43_TB_LINK_MAJOR_ERRORS_MASK                 0xFFull
#define DC_LPE_REG_43_TB_LINK_MAJOR_ERRORS_SMASK                0xFF000000ull
#define DC_LPE_REG_43_TB_FC_ERRORS_SHIFT                        21
#define DC_LPE_REG_43_TB_FC_ERRORS_MASK                         0x7ull
#define DC_LPE_REG_43_TB_FC_ERRORS_SMASK                        0xE00000ull
#define DC_LPE_REG_43_TB_PKT_LATE_ERRORS_SHIFT                  12
#define DC_LPE_REG_43_TB_PKT_LATE_ERRORS_MASK                   0x1FFull
#define DC_LPE_REG_43_TB_PKT_LATE_ERRORS_SMASK                  0x1FF000ull
#define DC_LPE_REG_43_TB_PKT_EARLY_ERRORS_SHIFT                 0
#define DC_LPE_REG_43_TB_PKT_EARLY_ERRORS_MASK                  0xFFFull
#define DC_LPE_REG_43_TB_PKT_EARLY_ERRORS_SMASK                 0xFFFull
/*
* Table #65 of 240_CSR_pcslpe.xml - lpe_reg_44
* See description below for individual field definition
*/
#define DC_LPE_REG_44                                           (DC_PCSLPE_LPE + 0x000000000220)
#define DC_LPE_REG_44_RESETCSR                                  0x00000006ull
#define DC_LPE_REG_44_TB_TIME_STAMP_SHIFT                       0
#define DC_LPE_REG_44_TB_TIME_STAMP_MASK                        0xFFFFFFFFull
#define DC_LPE_REG_44_TB_TIME_STAMP_SMASK                       0xFFFFFFFFull
/*
* Table #66 of 240_CSR_pcslpe.xml - lpe_reg_48
* See description below for individual field definition
*/
#define DC_LPE_REG_48                                           (DC_PCSLPE_LPE + 0x000000000240)
#define DC_LPE_REG_48_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_48_UNUSED_31_7_SHIFT                         7
#define DC_LPE_REG_48_UNUSED_31_7_MASK                          0x1FFFFFFull
#define DC_LPE_REG_48_UNUSED_31_7_SMASK                         0xFFFFFF80ull
#define DC_LPE_REG_48_ENABLE_SAMPLE_PERIOD_SHIFT                6
#define DC_LPE_REG_48_ENABLE_SAMPLE_PERIOD_MASK                 0x1ull
#define DC_LPE_REG_48_ENABLE_SAMPLE_PERIOD_SMASK                0x40ull
#define DC_LPE_REG_48_FREEZE_IDLE_CNT_SHIFT                     5
#define DC_LPE_REG_48_FREEZE_IDLE_CNT_MASK                      0x1ull
#define DC_LPE_REG_48_FREEZE_IDLE_CNT_SMASK                     0x20ull
#define DC_LPE_REG_48_CLEAR_IDLE_CNT_SHIFT                      4
#define DC_LPE_REG_48_CLEAR_IDLE_CNT_MASK                       0x1ull
#define DC_LPE_REG_48_CLEAR_IDLE_CNT_SMASK                      0x10ull
#define DC_LPE_REG_48_IDLE_SAMPLE_PERIOD_SHIFT                  0
#define DC_LPE_REG_48_IDLE_SAMPLE_PERIOD_MASK                   0xFull
#define DC_LPE_REG_48_IDLE_SAMPLE_PERIOD_SMASK                  0xFull
/*
* Table #67 of 240_CSR_pcslpe.xml - lpe_reg_49
* See description below for individual field definition
*/
#define DC_LPE_REG_49                                           (DC_PCSLPE_LPE + 0x000000000248)
#define DC_LPE_REG_49_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_49_RX_IDLE_CNT_UPPER_SHIFT                   0
#define DC_LPE_REG_49_RX_IDLE_CNT_UPPER_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_49_RX_IDLE_CNT_UPPER_SMASK                   0xFFFFFFFFull
/*
* Table #68 of 240_CSR_pcslpe.xml - lpe_reg_4A
* See description below for individual field definition
*/
#define DC_LPE_REG_4A                                           (DC_PCSLPE_LPE + 0x000000000250)
#define DC_LPE_REG_4A_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_4A_RX_IDLE_CNT_LOWER_SHIFT                   0
#define DC_LPE_REG_4A_RX_IDLE_CNT_LOWER_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_4A_RX_IDLE_CNT_LOWER_SMASK                   0xFFFFFFFFull
/*
* Table #69 of 240_CSR_pcslpe.xml - lpe_reg_4B
* See description below for individual field definition
*/
#define DC_LPE_REG_4B                                           (DC_PCSLPE_LPE + 0x000000000258)
#define DC_LPE_REG_4B_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_4B_TX_IDLE_CNT_UPPER_SHIFT                   0
#define DC_LPE_REG_4B_TX_IDLE_CNT_UPPER_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_4B_TX_IDLE_CNT_UPPER_SMASK                   0xFFFFFFFFull
/*
* Table #70 of 240_CSR_pcslpe.xml - lpe_reg_4C
* See description below for individual field definition
*/
#define DC_LPE_REG_4C                                           (DC_PCSLPE_LPE + 0x000000000260)
#define DC_LPE_REG_4C_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_4C_TX_IDLE_CNT_LOWER_SHIFT                   0
#define DC_LPE_REG_4C_TX_IDLE_CNT_LOWER_MASK                    0xFFFFFFFFull
#define DC_LPE_REG_4C_TX_IDLE_CNT_LOWER_SMASK                   0xFFFFFFFFull
/*
* Table #71 of 240_CSR_pcslpe.xml - lpe_reg_50
* See description below for individual field definition
*/
#define DC_LPE_REG_50                                           (DC_PCSLPE_LPE + 0x000000000280)
#define DC_LPE_REG_50_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_50_UNUSED_31_SHIFT                           31
#define DC_LPE_REG_50_UNUSED_31_MASK                            0x1ull
#define DC_LPE_REG_50_UNUSED_31_SMASK                           0x80000000ull
#define DC_LPE_REG_50_REG_0X40_INT_SHIFT                        30
#define DC_LPE_REG_50_REG_0X40_INT_MASK                         0x1ull
#define DC_LPE_REG_50_REG_0X40_INT_SMASK                        0x40000000ull
#define DC_LPE_REG_50_REG_0X30_INT_SHIFT                        29
#define DC_LPE_REG_50_REG_0X30_INT_MASK                         0x1ull
#define DC_LPE_REG_50_REG_0X30_INT_SMASK                        0x20000000ull
#define DC_LPE_REG_50_REG_0X2E_INT_SHIFT                        27
#define DC_LPE_REG_50_REG_0X2E_INT_MASK                         0x3ull
#define DC_LPE_REG_50_REG_0X2E_INT_SMASK                        0x18000000ull
#define DC_LPE_REG_50_REG_0X28_INT_SHIFT                        25
#define DC_LPE_REG_50_REG_0X28_INT_MASK                         0x3ull
#define DC_LPE_REG_50_REG_0X28_INT_SMASK                        0x6000000ull
#define DC_LPE_REG_50_REG_0X19_INT_SHIFT                        24
#define DC_LPE_REG_50_REG_0X19_INT_MASK                         0x1ull
#define DC_LPE_REG_50_REG_0X19_INT_SMASK                        0x1000000ull
#define DC_LPE_REG_50_REG_0X14_INT_SHIFT                        20
#define DC_LPE_REG_50_REG_0X14_INT_MASK                         0xFull
#define DC_LPE_REG_50_REG_0X14_INT_SMASK                        0xF00000ull
#define DC_LPE_REG_50_UNUSED_19_17_SHIFT                        17
#define DC_LPE_REG_50_UNUSED_19_17_MASK                         0x7ull
#define DC_LPE_REG_50_UNUSED_19_17_SMASK                        0xE0000ull
#define DC_LPE_REG_50_REG_0X0F_INT_SHIFT                        12
#define DC_LPE_REG_50_REG_0X0F_INT_MASK                         0x1Full
#define DC_LPE_REG_50_REG_0X0F_INT_SMASK                        0x1F000ull
#define DC_LPE_REG_50_UNUSED_11_10_SHIFT                        10
#define DC_LPE_REG_50_UNUSED_11_10_MASK                         0x3ull
#define DC_LPE_REG_50_UNUSED_11_10_SMASK                        0xC00ull
#define DC_LPE_REG_50_REG_0X0C_INT_SHIFT                        9
#define DC_LPE_REG_50_REG_0X0C_INT_MASK                         0x1ull
#define DC_LPE_REG_50_REG_0X0C_INT_SMASK                        0x200ull
#define DC_LPE_REG_50_REG_0X0A_INT_SHIFT                        8
#define DC_LPE_REG_50_REG_0X0A_INT_MASK                         0x1ull
#define DC_LPE_REG_50_REG_0X0A_INT_SMASK                        0x100ull
#define DC_LPE_REG_50_REG_0X01_INT_SHIFT                        0
#define DC_LPE_REG_50_REG_0X01_INT_MASK                         0xFFull
#define DC_LPE_REG_50_REG_0X01_INT_SMASK                        0xFFull
/*
* Table #72 of 240_CSR_pcslpe.xml - lpe_reg_51
* See description below for individual field definition
*/
#define DC_LPE_REG_51                                           (DC_PCSLPE_LPE + 0x000000000288)
#define DC_LPE_REG_51_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_51_TS_T_OS_DETECTED_INT_SHIFT                28
#define DC_LPE_REG_51_TS_T_OS_DETECTED_INT_MASK                 0xFull
#define DC_LPE_REG_51_TS_T_OS_DETECTED_INT_SMASK                0xF0000000ull
#define DC_LPE_REG_51_TS12_OS_ERR_DETECTED_INT_SHIFT            24
#define DC_LPE_REG_51_TS12_OS_ERR_DETECTED_INT_MASK             0xFull
#define DC_LPE_REG_51_TS12_OS_ERR_DETECTED_INT_SMASK            0xF000000ull
#define DC_LPE_REG_51_TS2_SEQ_DETECTED_INT_SHIFT                20
#define DC_LPE_REG_51_TS2_SEQ_DETECTED_INT_MASK                 0xFull
#define DC_LPE_REG_51_TS2_SEQ_DETECTED_INT_SMASK                0xF00000ull
#define DC_LPE_REG_51_TS1_SEQ_DETECTED_INT_SHIFT                16
#define DC_LPE_REG_51_TS1_SEQ_DETECTED_INT_MASK                 0xFull
#define DC_LPE_REG_51_TS1_SEQ_DETECTED_INT_SMASK                0xF0000ull
#define DC_LPE_REG_51_SKP_OS_DETECTED_INT_SHIFT                 12
#define DC_LPE_REG_51_SKP_OS_DETECTED_INT_MASK                  0xFull
#define DC_LPE_REG_51_SKP_OS_DETECTED_INT_SMASK                 0xF000ull
#define DC_LPE_REG_51_TS3_OS_DETECTED_INT_SHIFT                 8
#define DC_LPE_REG_51_TS3_OS_DETECTED_INT_MASK                  0xFull
#define DC_LPE_REG_51_TS3_OS_DETECTED_INT_SMASK                 0xF00ull
#define DC_LPE_REG_51_TS2_OS_DETECTED_INT_SHIFT                 4
#define DC_LPE_REG_51_TS2_OS_DETECTED_INT_MASK                  0xFull
#define DC_LPE_REG_51_TS2_OS_DETECTED_INT_SMASK                 0xF0ull
#define DC_LPE_REG_51_TS1_OS_DETECTED_INT_SHIFT                 0
#define DC_LPE_REG_51_TS1_OS_DETECTED_INT_MASK                  0xFull
#define DC_LPE_REG_51_TS1_OS_DETECTED_INT_SMASK                 0xFull
/*
* Table #73 of 240_CSR_pcslpe.xml - lpe_reg_52
* See description below for individual field definition
*/
#define DC_LPE_REG_52                                           (DC_PCSLPE_LPE + 0x000000000290)
#define DC_LPE_REG_52_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_52_UNUSED_31_22_SHIFT                        22
#define DC_LPE_REG_52_UNUSED_31_22_MASK                         0x3FFull
#define DC_LPE_REG_52_UNUSED_31_22_SMASK                        0xFFC00000ull
#define DC_LPE_REG_52_TX_REV_LANES_DETECTED_INT_SHIFT           20
#define DC_LPE_REG_52_TX_REV_LANES_DETECTED_INT_MASK            0x1ull
#define DC_LPE_REG_52_TX_REV_LANES_DETECTED_INT_SMASK           0x100000ull
#define DC_LPE_REG_52_LPE_RX_LATE_LOST_LINK_INT_SHIFT           19
#define DC_LPE_REG_52_LPE_RX_LATE_LOST_LINK_INT_MASK            0x1ull
#define DC_LPE_REG_52_LPE_RX_LATE_LOST_LINK_INT_SMASK           0x80000ull
#define DC_LPE_REG_52_LPE_RX_LATE_UNEXPECTED_CHAR_ERR_INT_SHIFT 18
#define DC_LPE_REG_52_LPE_RX_LATE_UNEXPECTED_CHAR_ERR_INT_MASK  0x1ull
#define DC_LPE_REG_52_LPE_RX_LATE_UNEXPECTED_CHAR_ERR_INT_SMASK 0x40000ull
#define DC_LPE_REG_52_UNUSED_17_SHIFT                           17
#define DC_LPE_REG_52_UNUSED_17_MASK                            0x1ull
#define DC_LPE_REG_52_UNUSED_17_SMASK                           0x20000ull
#define DC_LPE_REG_52_LPE_RX_LATE_CODE_ERR_INT_SHIFT            16
#define DC_LPE_REG_52_LPE_RX_LATE_CODE_ERR_INT_MASK             0x1ull
#define DC_LPE_REG_52_LPE_RX_LATE_CODE_ERR_INT_SMASK            0x10000ull
#define DC_LPE_REG_52_RX_ABR_ERROR_INT_SHIFT                    8
#define DC_LPE_REG_52_RX_ABR_ERROR_INT_MASK                     0xFFull
#define DC_LPE_REG_52_RX_ABR_ERROR_INT_SMASK                    0xFF00ull
#define DC_LPE_REG_52_IB_STAT_FC_VIOLATION_INT_SHIFT            0
#define DC_LPE_REG_52_IB_STAT_FC_VIOLATION_INT_MASK             0xFFull
#define DC_LPE_REG_52_IB_STAT_FC_VIOLATION_INT_SMASK            0xFFull
/*
* Table #74 of 240_CSR_pcslpe.xml - lpe_reg_54
* See description below for individual field definition
*/
#define DC_LPE_REG_54                                           (DC_PCSLPE_LPE + 0x0000000002A0)
#define DC_LPE_REG_54_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_54_UNUSED_31_SHIFT                           31
#define DC_LPE_REG_54_UNUSED_31_MASK                            0x1ull
#define DC_LPE_REG_54_UNUSED_31_SMASK                           0x80000000ull
#define DC_LPE_REG_54_REG_0X40_INT_MASK_SHIFT                   30
#define DC_LPE_REG_54_REG_0X40_INT_MASK_MASK                    0x1ull
#define DC_LPE_REG_54_REG_0X40_INT_MASK_SMASK                   0x40000000ull
#define DC_LPE_REG_54_REG_0X2F_INT_MASK_SHIFT                   28
#define DC_LPE_REG_54_REG_0X2F_INT_MASK_MASK                    0x3ull
#define DC_LPE_REG_54_REG_0X2F_INT_MASK_SMASK                   0x30000000ull
#define DC_LPE_REG_54_UNUSED_27_26_SHIFT                        26
#define DC_LPE_REG_54_UNUSED_27_26_MASK                         0x3ull
#define DC_LPE_REG_54_UNUSED_27_26_SMASK                        0xC000000ull
#define DC_LPE_REG_54_REG_0X28_INT_MASK_SHIFT                   25
#define DC_LPE_REG_54_REG_0X28_INT_MASK_MASK                    0x1ull
#define DC_LPE_REG_54_REG_0X28_INT_MASK_SMASK                   0x2000000ull
#define DC_LPE_REG_54_REG_0X19_INT_MASK_SHIFT                   24
#define DC_LPE_REG_54_REG_0X19_INT_MASK_MASK                    0x1ull
#define DC_LPE_REG_54_REG_0X19_INT_MASK_SMASK                   0x1000000ull
#define DC_LPE_REG_54_REG_0X14_INT_MASK_SHIFT                   20
#define DC_LPE_REG_54_REG_0X14_INT_MASK_MASK                    0xFull
#define DC_LPE_REG_54_REG_0X14_INT_MASK_SMASK                   0xF00000ull
#define DC_LPE_REG_54_UNUSED_19_17_SHIFT                        17
#define DC_LPE_REG_54_UNUSED_19_17_MASK                         0x7ull
#define DC_LPE_REG_54_UNUSED_19_17_SMASK                        0xE0000ull
#define DC_LPE_REG_54_REG_0X0F_INT_MASK_SHIFT                   12
#define DC_LPE_REG_54_REG_0X0F_INT_MASK_MASK                    0x1Full
#define DC_LPE_REG_54_REG_0X0F_INT_MASK_SMASK                   0x1F000ull
#define DC_LPE_REG_54_UNUSED_11_10_SHIFT                        10
#define DC_LPE_REG_54_UNUSED_11_10_MASK                         0x3ull
#define DC_LPE_REG_54_UNUSED_11_10_SMASK                        0xC00ull
#define DC_LPE_REG_54_REG_0X0C_INT_MASK_SHIFT                   9
#define DC_LPE_REG_54_REG_0X0C_INT_MASK_MASK                    0x1ull
#define DC_LPE_REG_54_REG_0X0C_INT_MASK_SMASK                   0x200ull
#define DC_LPE_REG_54_REG_0X0A_INT_MASK_SHIFT                   8
#define DC_LPE_REG_54_REG_0X0A_INT_MASK_MASK                    0x1ull
#define DC_LPE_REG_54_REG_0X0A_INT_MASK_SMASK                   0x100ull
#define DC_LPE_REG_54_REG_0X01_INT_MASK_SHIFT                   0
#define DC_LPE_REG_54_REG_0X01_INT_MASK_MASK                    0xFFull
#define DC_LPE_REG_54_REG_0X01_INT_MASK_SMASK                   0xFFull
/*
* Table #75 of 240_CSR_pcslpe.xml - lpe_reg_55
* See description below for individual field definition
*/
#define DC_LPE_REG_55                                           (DC_PCSLPE_LPE + 0x0000000002A8)
#define DC_LPE_REG_55_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_55_TS_T_OS_DETECTED_INT_MASK_SHIFT           28
#define DC_LPE_REG_55_TS_T_OS_DETECTED_INT_MASK_MASK            0xFull
#define DC_LPE_REG_55_TS_T_OS_DETECTED_INT_MASK_SMASK           0xF0000000ull
#define DC_LPE_REG_55_TS12_OS_ERR_DETECTED_INT_MASK_SHIFT       24
#define DC_LPE_REG_55_TS12_OS_ERR_DETECTED_INT_MASK_MASK        0xFull
#define DC_LPE_REG_55_TS12_OS_ERR_DETECTED_INT_MASK_SMASK       0xF000000ull
#define DC_LPE_REG_55_TS2_SEQ_DETECTED_INT_MASK_SHIFT           20
#define DC_LPE_REG_55_TS2_SEQ_DETECTED_INT_MASK_MASK            0xFull
#define DC_LPE_REG_55_TS2_SEQ_DETECTED_INT_MASK_SMASK           0xF00000ull
#define DC_LPE_REG_55_TS1_SEQ_DETECTED_INT_MASK_SHIFT           16
#define DC_LPE_REG_55_TS1_SEQ_DETECTED_INT_MASK_MASK            0xFull
#define DC_LPE_REG_55_TS1_SEQ_DETECTED_INT_MASK_SMASK           0xF0000ull
#define DC_LPE_REG_55_SKP_OS_DETECTED_INT_MASK_SHIFT            12
#define DC_LPE_REG_55_SKP_OS_DETECTED_INT_MASK_MASK             0xFull
#define DC_LPE_REG_55_SKP_OS_DETECTED_INT_MASK_SMASK            0xF000ull
#define DC_LPE_REG_55_TS3_OS_DETECTED_INT_MASK_SHIFT            8
#define DC_LPE_REG_55_TS3_OS_DETECTED_INT_MASK_MASK             0xFull
#define DC_LPE_REG_55_TS3_OS_DETECTED_INT_MASK_SMASK            0xF00ull
#define DC_LPE_REG_55_TS2_OS_DETECTED_INT_MASK_SHIFT            4
#define DC_LPE_REG_55_TS2_OS_DETECTED_INT_MASK_MASK             0xFull
#define DC_LPE_REG_55_TS2_OS_DETECTED_INT_MASK_SMASK            0xF0ull
#define DC_LPE_REG_55_TS1_OS_DETECTED_INT_MASK_SHIFT            0
#define DC_LPE_REG_55_TS1_OS_DETECTED_INT_MASK_MASK             0xFull
#define DC_LPE_REG_55_TS1_OS_DETECTED_INT_MASK_SMASK            0xFull
/*
* Table #76 of 240_CSR_pcslpe.xml - lpe_reg_56
* See description below for individual field definition
*/
#define DC_LPE_REG_56                                           (DC_PCSLPE_LPE + 0x0000000002B0)
#define DC_LPE_REG_56_RESETCSR                                  0x00000000ull
#define DC_LPE_REG_56_UNUSED_31_22_SHIFT                        22
#define DC_LPE_REG_56_UNUSED_31_22_MASK                         0x3FFull
#define DC_LPE_REG_56_UNUSED_31_22_SMASK                        0xFFC00000ull
#define DC_LPE_REG_56_TX_REV_LANES_DETECTED_INT_MASK_SHIFT      20
#define DC_LPE_REG_56_TX_REV_LANES_DETECTED_INT_MASK_MASK       0x1ull
#define DC_LPE_REG_56_TX_REV_LANES_DETECTED_INT_MASK_SMASK      0x100000ull
#define DC_LPE_REG_56_LPE_RX_LATE_LOST_LINK_INT_MASK_SHIFT      19
#define DC_LPE_REG_56_LPE_RX_LATE_LOST_LINK_INT_MASK_MASK       0x1ull
#define DC_LPE_REG_56_LPE_RX_LATE_LOST_LINK_INT_MASK_SMASK      0x80000ull
#define DC_LPE_REG_56_LPE_RX_LATE_CTRL_ERR_INT_MASK_SHIFT       18
#define DC_LPE_REG_56_LPE_RX_LATE_CTRL_ERR_INT_MASK_MASK        0x1ull
#define DC_LPE_REG_56_LPE_RX_LATE_CTRL_ERR_INT_MASK_SMASK       0x40000ull
#define DC_LPE_REG_56_UNUSED_17_SHIFT                           17
#define DC_LPE_REG_56_UNUSED_17_MASK                            0x1ull
#define DC_LPE_REG_56_UNUSED_17_SMASK                           0x20000ull
#define DC_LPE_REG_56_LPE_RX_LATE_CODE_ERR_INT_MASK_SHIFT       16
#define DC_LPE_REG_56_LPE_RX_LATE_CODE_ERR_INT_MASK_MASK        0x1ull
#define DC_LPE_REG_56_LPE_RX_LATE_CODE_ERR_INT_MASK_SMASK       0x10000ull
#define DC_LPE_REG_56_RX_ABR_ERROR_INT_MASK_SHIFT               8
#define DC_LPE_REG_56_RX_ABR_ERROR_INT_MASK_MASK                0xFFull
#define DC_LPE_REG_56_RX_ABR_ERROR_INT_MASK_SMASK               0xFF00ull
#define DC_LPE_REG_56_IB_STAT_FC_VIOLATION_INT_MASK_SHIFT       0
#define DC_LPE_REG_56_IB_STAT_FC_VIOLATION_INT_MASK_MASK        0xFFull
#define DC_LPE_REG_56_IB_STAT_FC_VIOLATION_INT_MASK_SMASK       0xFFull
