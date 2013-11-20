/*
* Table #2 of 220_CSR_lcb.xml - LCB_CFG_RUN
* Set this to turn on the LCB. See #%%#Section 4#%%#.
*/
#define DC_LCB_CFG_RUN                                                (DC_LCB_CSRS + 0x000000000000)
#define DC_LCB_CFG_RUN_RESETCSR                                       0x0000000000000000ull
#define DC_LCB_CFG_RUN_UNUSED_63_1_SHIFT                              1
#define DC_LCB_CFG_RUN_UNUSED_63_1_MASK                               0x7FFFFFFFFFFFFFFFull
#define DC_LCB_CFG_RUN_UNUSED_63_1_SMASK                              0xFFFFFFFFFFFFFFFEull
#define DC_LCB_CFG_RUN_EN_SHIFT                                       0
#define DC_LCB_CFG_RUN_EN_MASK                                        0x1ull
#define DC_LCB_CFG_RUN_EN_SMASK                                       0x1ull
/*
* Table #3 of 220_CSR_lcb.xml - LCB_CFG_TX_FIFOS_RESET
* This resets the tx fifo read and write pointers. It should be cleared after 
* the lane equalization is complete and the lane fifo clocks are stable and at 
* their final frequency. It is anticipated that the 8051 firmware agents will do 
* the reset release on both sides of the link at the end of the LNI 
* process.
*/
#define DC_LCB_CFG_TX_FIFOS_RESET                                     (DC_LCB_CSRS + 0x000000000008)
#define DC_LCB_CFG_TX_FIFOS_RESET_RESETCSR                            0x0000000000000001ull
#define DC_LCB_CFG_TX_FIFOS_RESET_UNUSED_63_1_SHIFT                   1
#define DC_LCB_CFG_TX_FIFOS_RESET_UNUSED_63_1_MASK                    0x7FFFFFFFFFFFFFFFull
#define DC_LCB_CFG_TX_FIFOS_RESET_UNUSED_63_1_SMASK                   0xFFFFFFFFFFFFFFFEull
#define DC_LCB_CFG_TX_FIFOS_RESET_VAL_SHIFT                           0
#define DC_LCB_CFG_TX_FIFOS_RESET_VAL_MASK                            0x1ull
#define DC_LCB_CFG_TX_FIFOS_RESET_VAL_SMASK                           0x1ull
/*
* Table #4 of 220_CSR_lcb.xml - LCB_CFG_TX_FIFOS_RADR
* This register provides the read address reset value for the tx 
* fifo.
*/
#define DC_LCB_CFG_TX_FIFOS_RADR                                      (DC_LCB_CSRS + 0x000000000010)
#define DC_LCB_CFG_TX_FIFOS_RADR_RESETCSR                             0x0000000000000007ull
#define DC_LCB_CFG_TX_FIFOS_RADR_UNUSED_63_5_SHIFT                    5
#define DC_LCB_CFG_TX_FIFOS_RADR_UNUSED_63_5_MASK                     0x7FFFFFFFFFFFFFFull
#define DC_LCB_CFG_TX_FIFOS_RADR_UNUSED_63_5_SMASK                    0xFFFFFFFFFFFFFFE0ull
#define DC_LCB_CFG_TX_FIFOS_RADR_ON_REINIT_SHIFT                      4
#define DC_LCB_CFG_TX_FIFOS_RADR_ON_REINIT_MASK                       0x1ull
#define DC_LCB_CFG_TX_FIFOS_RADR_ON_REINIT_SMASK                      0x10ull
#define DC_LCB_CFG_TX_FIFOS_RADR_UNUSED_3_SHIFT                       3
#define DC_LCB_CFG_TX_FIFOS_RADR_UNUSED_3_MASK                        0x1ull
#define DC_LCB_CFG_TX_FIFOS_RADR_UNUSED_3_SMASK                       0x8ull
#define DC_LCB_CFG_TX_FIFOS_RADR_RST_VAL_SHIFT                        0
#define DC_LCB_CFG_TX_FIFOS_RADR_RST_VAL_MASK                         0x7ull
#define DC_LCB_CFG_TX_FIFOS_RADR_RST_VAL_SMASK                        0x7ull
/*
* Table #5 of 220_CSR_lcb.xml - LCB_CFG_RX_FIFOS_RADR
* This register provides the read address reset value for the rx 
* fifo.
*/
#define DC_LCB_CFG_RX_FIFOS_RADR                                      (DC_LCB_CSRS + 0x000000000018)
#define DC_LCB_CFG_RX_FIFOS_RADR_RESETCSR                             0x0000000000000CBBull
#define DC_LCB_CFG_RX_FIFOS_RADR_UNUSED_63_12_SHIFT                   12
#define DC_LCB_CFG_RX_FIFOS_RADR_UNUSED_63_12_MASK                    0xFFFFFFFFFFFFFull
#define DC_LCB_CFG_RX_FIFOS_RADR_UNUSED_63_12_SMASK                   0xFFFFFFFFFFFFF000ull
#define DC_LCB_CFG_RX_FIFOS_RADR_DO_NOT_JUMP_VAL_SHIFT                8
#define DC_LCB_CFG_RX_FIFOS_RADR_DO_NOT_JUMP_VAL_MASK                 0xFull
#define DC_LCB_CFG_RX_FIFOS_RADR_DO_NOT_JUMP_VAL_SMASK                0xF00ull
#define DC_LCB_CFG_RX_FIFOS_RADR_OK_TO_JUMP_VAL_SHIFT                 4
#define DC_LCB_CFG_RX_FIFOS_RADR_OK_TO_JUMP_VAL_MASK                  0xFull
#define DC_LCB_CFG_RX_FIFOS_RADR_OK_TO_JUMP_VAL_SMASK                 0xF0ull
#define DC_LCB_CFG_RX_FIFOS_RADR_RST_VAL_SHIFT                        0
#define DC_LCB_CFG_RX_FIFOS_RADR_RST_VAL_MASK                         0xFull
#define DC_LCB_CFG_RX_FIFOS_RADR_RST_VAL_SMASK                        0xFull
/*
* Table #6 of 220_CSR_lcb.xml - LCB_CFG_IGNORE_LOST_RCLK
* This configures the logic to ignore a lost receive clock. Useful only in pre 
* silicon validation.
*/
#define DC_LCB_CFG_IGNORE_LOST_RCLK                                   (DC_LCB_CSRS + 0x000000000020)
#define DC_LCB_CFG_IGNORE_LOST_RCLK_RESETCSR                          0x0000000000000000ull
#define DC_LCB_CFG_IGNORE_LOST_RCLK_UNUSED_63_1_SHIFT                 1
#define DC_LCB_CFG_IGNORE_LOST_RCLK_UNUSED_63_1_MASK                  0x7FFFFFFFFFFFFFFFull
#define DC_LCB_CFG_IGNORE_LOST_RCLK_UNUSED_63_1_SMASK                 0xFFFFFFFFFFFFFFFEull
#define DC_LCB_CFG_IGNORE_LOST_RCLK_EN_SHIFT                          0
#define DC_LCB_CFG_IGNORE_LOST_RCLK_EN_MASK                           0x1ull
#define DC_LCB_CFG_IGNORE_LOST_RCLK_EN_SMASK                          0x1ull
/*
* Table #7 of 220_CSR_lcb.xml - LCB_CFG_REINIT_PAUSE
* This register controls the ability to pause the re-initialization recovery 
* sequence. See also LCB_STS_LINK_PAUSE_ACTIVE, LCB_CFG_REINIT_AS_SLAVE and 
* LCB_CFG_FORCE_RECOVER_SEQUENCE.
*/
#define DC_LCB_CFG_REINIT_PAUSE                                       (DC_LCB_CSRS + 0x000000000028)
#define DC_LCB_CFG_REINIT_PAUSE_RESETCSR                              0x0000000000000000ull
#define DC_LCB_CFG_REINIT_PAUSE_UNUSED_63_9_SHIFT                     9
#define DC_LCB_CFG_REINIT_PAUSE_UNUSED_63_9_MASK                      0x7FFFFFFFFFFFFFull
#define DC_LCB_CFG_REINIT_PAUSE_UNUSED_63_9_SMASK                     0xFFFFFFFFFFFFFE00ull
#define DC_LCB_CFG_REINIT_PAUSE_ON_REINIT_SHIFT                       8
#define DC_LCB_CFG_REINIT_PAUSE_ON_REINIT_MASK                        0x1ull
#define DC_LCB_CFG_REINIT_PAUSE_ON_REINIT_SMASK                       0x100ull
#define DC_LCB_CFG_REINIT_PAUSE_UNUSED_7_5_SHIFT                      5
#define DC_LCB_CFG_REINIT_PAUSE_UNUSED_7_5_MASK                       0x7ull
#define DC_LCB_CFG_REINIT_PAUSE_UNUSED_7_5_SMASK                      0xE0ull
#define DC_LCB_CFG_REINIT_PAUSE_TX_MODE_SHIFT                         4
#define DC_LCB_CFG_REINIT_PAUSE_TX_MODE_MASK                          0x1ull
#define DC_LCB_CFG_REINIT_PAUSE_TX_MODE_SMASK                         0x10ull
#define DC_LCB_CFG_REINIT_PAUSE_UNUSED_3_2_SHIFT                      2
#define DC_LCB_CFG_REINIT_PAUSE_UNUSED_3_2_MASK                       0x3ull
#define DC_LCB_CFG_REINIT_PAUSE_UNUSED_3_2_SMASK                      0xCull
#define DC_LCB_CFG_REINIT_PAUSE_RX_MODE_SHIFT                         0
#define DC_LCB_CFG_REINIT_PAUSE_RX_MODE_MASK                          0x3ull
#define DC_LCB_CFG_REINIT_PAUSE_RX_MODE_SMASK                         0x3ull
/*
* Table #8 of 220_CSR_lcb.xml - LCB_CFG_REINIT_AS_SLAVE
* This register is used to allow a link pause controlled entirely from HFI LCB 
* CSRs. A switch LCB connected to a HFI LCB will perform as a slave LCB to the 
* HFI master LCB. This will needed for OPIO re-center during which the link will 
* be paused under generation 2 HFI control connected to a generation 1 switch. 
* If two HFI are connected together in a testing configuration only one should 
* be the slave. The OPIO re-center can be performed on both simultaneously. See 
* also LCB_STS_LINK_PAUSE_ACTIVE, LCB_CFG_REINIT_PAUSE_TX_MODE and 
* LCB_CFG_FORCE_RECOVER_SEQUENCE.
*/
#define DC_LCB_CFG_REINIT_AS_SLAVE                                    (DC_LCB_CSRS + 0x000000000030)
#define DC_LCB_CFG_REINIT_AS_SLAVE_RESETCSR                           0x0000000000000000ull
#define DC_LCB_CFG_REINIT_AS_SLAVE_UNUSED_63_1_SHIFT                  1
#define DC_LCB_CFG_REINIT_AS_SLAVE_UNUSED_63_1_MASK                   0x7FFFFFFFFFFFFFFFull
#define DC_LCB_CFG_REINIT_AS_SLAVE_UNUSED_63_1_SMASK                  0xFFFFFFFFFFFFFFFEull
#define DC_LCB_CFG_REINIT_AS_SLAVE_VAL_SHIFT                          0
#define DC_LCB_CFG_REINIT_AS_SLAVE_VAL_MASK                           0x1ull
#define DC_LCB_CFG_REINIT_AS_SLAVE_VAL_SMASK                          0x1ull
/*
* Table #9 of 220_CSR_lcb.xml - LCB_CFG_FORCE_RECOVER_SEQUENCE
* This register forces a recovery re-initialization sequence.
*/
#define DC_LCB_CFG_FORCE_RECOVER_SEQUENCE                             (DC_LCB_CSRS + 0x000000000038)
#define DC_LCB_CFG_FORCE_RECOVER_SEQUENCE_RESETCSR                    0x0000000000000000ull
#define DC_LCB_CFG_FORCE_RECOVER_SEQUENCE_UNUSED_63_1_SHIFT           1
#define DC_LCB_CFG_FORCE_RECOVER_SEQUENCE_UNUSED_63_1_MASK            0x7FFFFFFFFFFFFFFFull
#define DC_LCB_CFG_FORCE_RECOVER_SEQUENCE_UNUSED_63_1_SMASK           0xFFFFFFFFFFFFFFFEull
#define DC_LCB_CFG_FORCE_RECOVER_SEQUENCE_EN_SHIFT                    0
#define DC_LCB_CFG_FORCE_RECOVER_SEQUENCE_EN_MASK                     0x1ull
#define DC_LCB_CFG_FORCE_RECOVER_SEQUENCE_EN_SMASK                    0x1ull
/*
* Table #10 of 220_CSR_lcb.xml - LCB_CFG_CNT_FOR_SKIP_STALL
* This configures the skip stall counts for various ppm values.
*/
#define DC_LCB_CFG_CNT_FOR_SKIP_STALL                                 (DC_LCB_CSRS + 0x000000000040)
#define DC_LCB_CFG_CNT_FOR_SKIP_STALL_RESETCSR                        0x0000000000000000ull
#define DC_LCB_CFG_CNT_FOR_SKIP_STALL_UNUSED_63_9_SHIFT               9
#define DC_LCB_CFG_CNT_FOR_SKIP_STALL_UNUSED_63_9_MASK                0x7FFFFFFFFFFFFFull
#define DC_LCB_CFG_CNT_FOR_SKIP_STALL_UNUSED_63_9_SMASK               0xFFFFFFFFFFFFFE00ull
#define DC_LCB_CFG_CNT_FOR_SKIP_STALL_DISABLE_RX_SHIFT                8
#define DC_LCB_CFG_CNT_FOR_SKIP_STALL_DISABLE_RX_MASK                 0x1ull
#define DC_LCB_CFG_CNT_FOR_SKIP_STALL_DISABLE_RX_SMASK                0x100ull
#define DC_LCB_CFG_CNT_FOR_SKIP_STALL_UNUSED_7_5_SHIFT                5
#define DC_LCB_CFG_CNT_FOR_SKIP_STALL_UNUSED_7_5_MASK                 0x7ull
#define DC_LCB_CFG_CNT_FOR_SKIP_STALL_UNUSED_7_5_SMASK                0xE0ull
#define DC_LCB_CFG_CNT_FOR_SKIP_STALL_DISABLE_TX_SHIFT                4
#define DC_LCB_CFG_CNT_FOR_SKIP_STALL_DISABLE_TX_MASK                 0x1ull
#define DC_LCB_CFG_CNT_FOR_SKIP_STALL_DISABLE_TX_SMASK                0x10ull
#define DC_LCB_CFG_CNT_FOR_SKIP_STALL_UNUSED_3_2_SHIFT                2
#define DC_LCB_CFG_CNT_FOR_SKIP_STALL_UNUSED_3_2_MASK                 0x3ull
#define DC_LCB_CFG_CNT_FOR_SKIP_STALL_UNUSED_3_2_SMASK                0xCull
#define DC_LCB_CFG_CNT_FOR_SKIP_STALL_PPM_SELECT_SHIFT                0
#define DC_LCB_CFG_CNT_FOR_SKIP_STALL_PPM_SELECT_MASK                 0x3ull
#define DC_LCB_CFG_CNT_FOR_SKIP_STALL_PPM_SELECT_SMASK                0x3ull
/*
* Table #11 of 220_CSR_lcb.xml - LCB_CFG_TX_FSS
* This register enables the Tx side synchronous scrambler.
*/
#define DC_LCB_CFG_TX_FSS                                             (DC_LCB_CSRS + 0x000000000048)
#define DC_LCB_CFG_TX_FSS_RESETCSR                                    0x0000000000000001ull
#define DC_LCB_CFG_TX_FSS_UNUSED_63_1_SHIFT                           1
#define DC_LCB_CFG_TX_FSS_UNUSED_63_1_MASK                            0x7FFFFFFFFFFFFFFFull
#define DC_LCB_CFG_TX_FSS_UNUSED_63_1_SMASK                           0xFFFFFFFFFFFFFFFEull
#define DC_LCB_CFG_TX_FSS_EN_SHIFT                                    0
#define DC_LCB_CFG_TX_FSS_EN_MASK                                     0x1ull
#define DC_LCB_CFG_TX_FSS_EN_SMASK                                    0x1ull
/*
* Table #12 of 220_CSR_lcb.xml - LCB_CFG_RX_FSS
* This register enables the Rx side synchronous scrambler.
*/
#define DC_LCB_CFG_RX_FSS                                             (DC_LCB_CSRS + 0x000000000050)
#define DC_LCB_CFG_RX_FSS_RESETCSR                                    0x0000000000000001ull
#define DC_LCB_CFG_RX_FSS_UNUSED_63_1_SHIFT                           1
#define DC_LCB_CFG_RX_FSS_UNUSED_63_1_MASK                            0x7FFFFFFFFFFFFFFFull
#define DC_LCB_CFG_RX_FSS_UNUSED_63_1_SMASK                           0xFFFFFFFFFFFFFFFEull
#define DC_LCB_CFG_RX_FSS_EN_SHIFT                                    0
#define DC_LCB_CFG_RX_FSS_EN_MASK                                     0x1ull
#define DC_LCB_CFG_RX_FSS_EN_SMASK                                    0x1ull
/*
* Table #13 of 220_CSR_lcb.xml - LCB_CFG_CRC_MODE
* This register sets the CRC mode on the Tx side of the link. The Rx side is 
* informed via the initialization training pattern. Different CRC modes can be 
* set for each of the two link directions. There is a fallback capability to use 
* a configuration value on the Rx side instead of the value delivered in the 
* training pattern.
*/
#define DC_LCB_CFG_CRC_MODE                                           (DC_LCB_CSRS + 0x000000000058)
#define DC_LCB_CFG_CRC_MODE_RESETCSR                                  0x0000000000000000ull
#define DC_LCB_CFG_CRC_MODE_UNUSED_63_10_SHIFT                        10
#define DC_LCB_CFG_CRC_MODE_UNUSED_63_10_MASK                         0x3FFFFFFFFFFFFFull
#define DC_LCB_CFG_CRC_MODE_UNUSED_63_10_SMASK                        0xFFFFFFFFFFFFFC00ull
#define DC_LCB_CFG_CRC_MODE_RX_VAL_SHIFT                              8
#define DC_LCB_CFG_CRC_MODE_RX_VAL_MASK                               0x3ull
#define DC_LCB_CFG_CRC_MODE_RX_VAL_SMASK                              0x300ull
#define DC_LCB_CFG_CRC_MODE_UNUSED_7_5_SHIFT                          5
#define DC_LCB_CFG_CRC_MODE_UNUSED_7_5_MASK                           0x7ull
#define DC_LCB_CFG_CRC_MODE_UNUSED_7_5_SMASK                          0xE0ull
#define DC_LCB_CFG_CRC_MODE_USE_RX_CFG_VAL_SHIFT                      4
#define DC_LCB_CFG_CRC_MODE_USE_RX_CFG_VAL_MASK                       0x1ull
#define DC_LCB_CFG_CRC_MODE_USE_RX_CFG_VAL_SMASK                      0x10ull
#define DC_LCB_CFG_CRC_MODE_UNUSED_3_2_SHIFT                          2
#define DC_LCB_CFG_CRC_MODE_UNUSED_3_2_MASK                           0x3ull
#define DC_LCB_CFG_CRC_MODE_UNUSED_3_2_SMASK                          0xCull
#define DC_LCB_CFG_CRC_MODE_TX_VAL_SHIFT                              0
#define DC_LCB_CFG_CRC_MODE_TX_VAL_MASK                               0x3ull
#define DC_LCB_CFG_CRC_MODE_TX_VAL_SMASK                              0x3ull
/*
* Table #14 of 220_CSR_lcb.xml - LCB_CFG_LN_DCLK
* This register selects the tx side lane fifo clk to use.
*/
#define DC_LCB_CFG_LN_DCLK                                            (DC_LCB_CSRS + 0x000000000060)
#define DC_LCB_CFG_LN_DCLK_RESETCSR                                   0x0000000000000000ull
#define DC_LCB_CFG_LN_DCLK_UNUSED_63_5_SHIFT                          5
#define DC_LCB_CFG_LN_DCLK_UNUSED_63_5_MASK                           0x7FFFFFFFFFFFFFFull
#define DC_LCB_CFG_LN_DCLK_UNUSED_63_5_SMASK                          0xFFFFFFFFFFFFFFE0ull
#define DC_LCB_CFG_LN_DCLK_INVERT_SAMPLE_SHIFT                        4
#define DC_LCB_CFG_LN_DCLK_INVERT_SAMPLE_MASK                         0x1ull
#define DC_LCB_CFG_LN_DCLK_INVERT_SAMPLE_SMASK                        0x10ull
#define DC_LCB_CFG_LN_DCLK_UNUSED_3_2_SHIFT                           2
#define DC_LCB_CFG_LN_DCLK_UNUSED_3_2_MASK                            0x3ull
#define DC_LCB_CFG_LN_DCLK_UNUSED_3_2_SMASK                           0xCull
#define DC_LCB_CFG_LN_DCLK_SELECT_SHIFT                               0
#define DC_LCB_CFG_LN_DCLK_SELECT_MASK                                0x3ull
#define DC_LCB_CFG_LN_DCLK_SELECT_SMASK                               0x3ull
/*
* Table #15 of 220_CSR_lcb.xml - LCB_CFG_LN_TEST_TIME_TO_PASS
* This register sets the time for a lane to pass testing after the first lane 
* passes.
*/
#define DC_LCB_CFG_LN_TEST_TIME_TO_PASS                               (DC_LCB_CSRS + 0x000000000068)
#define DC_LCB_CFG_LN_TEST_TIME_TO_PASS_RESETCSR                      0x0000000000000060ull
#define DC_LCB_CFG_LN_TEST_TIME_TO_PASS_UNUSED_63_12_SHIFT            12
#define DC_LCB_CFG_LN_TEST_TIME_TO_PASS_UNUSED_63_12_MASK             0xFFFFFFFFFFFFFull
#define DC_LCB_CFG_LN_TEST_TIME_TO_PASS_UNUSED_63_12_SMASK            0xFFFFFFFFFFFFF000ull
#define DC_LCB_CFG_LN_TEST_TIME_TO_PASS_VAL_SHIFT                     0
#define DC_LCB_CFG_LN_TEST_TIME_TO_PASS_VAL_MASK                      0xFFFull
#define DC_LCB_CFG_LN_TEST_TIME_TO_PASS_VAL_SMASK                     0xFFFull
/*
* Table #16 of 220_CSR_lcb.xml - LCB_CFG_LN_TEST_REQ_PASS_CNT
* This register sets the number of matches (out of a total of 256) required for 
* the lane to pass testing during a reinit sequence.
*/
#define DC_LCB_CFG_LN_TEST_REQ_PASS_CNT                               (DC_LCB_CSRS + 0x000000000070)
#define DC_LCB_CFG_LN_TEST_REQ_PASS_CNT_RESETCSR                      0x0000000000000005ull
#define DC_LCB_CFG_LN_TEST_REQ_PASS_CNT_UNUSED_63_3_SHIFT             3
#define DC_LCB_CFG_LN_TEST_REQ_PASS_CNT_UNUSED_63_3_MASK              0x1FFFFFFFFFFFFFFFull
#define DC_LCB_CFG_LN_TEST_REQ_PASS_CNT_UNUSED_63_3_SMASK             0xFFFFFFFFFFFFFFF8ull
#define DC_LCB_CFG_LN_TEST_REQ_PASS_CNT_VAL_SHIFT                     0
#define DC_LCB_CFG_LN_TEST_REQ_PASS_CNT_VAL_MASK                      0x7ull
#define DC_LCB_CFG_LN_TEST_REQ_PASS_CNT_VAL_SMASK                     0x7ull
/*
* Table #17 of 220_CSR_lcb.xml - LCB_CFG_LN_RE_ENABLE
* This register configures the ability to re-enable a lane during a reinit 
* sequence.
*/
#define DC_LCB_CFG_LN_RE_ENABLE                                       (DC_LCB_CSRS + 0x000000000078)
#define DC_LCB_CFG_LN_RE_ENABLE_RESETCSR                              0x0000000000000002ull
#define DC_LCB_CFG_LN_RE_ENABLE_UNUSED_63_3_SHIFT                     3
#define DC_LCB_CFG_LN_RE_ENABLE_UNUSED_63_3_MASK                      0x1FFFFFFFFFFFFFFFull
#define DC_LCB_CFG_LN_RE_ENABLE_UNUSED_63_3_SMASK                     0xFFFFFFFFFFFFFFF8ull
#define DC_LCB_CFG_LN_RE_ENABLE_LTO_DEGRADE_SHIFT                     2
#define DC_LCB_CFG_LN_RE_ENABLE_LTO_DEGRADE_MASK                      0x1ull
#define DC_LCB_CFG_LN_RE_ENABLE_LTO_DEGRADE_SMASK                     0x4ull
#define DC_LCB_CFG_LN_RE_ENABLE_LTO_REINIT_SHIFT                      1
#define DC_LCB_CFG_LN_RE_ENABLE_LTO_REINIT_MASK                       0x1ull
#define DC_LCB_CFG_LN_RE_ENABLE_LTO_REINIT_SMASK                      0x2ull
#define DC_LCB_CFG_LN_RE_ENABLE_ON_REINIT_SHIFT                       0
#define DC_LCB_CFG_LN_RE_ENABLE_ON_REINIT_MASK                        0x1ull
#define DC_LCB_CFG_LN_RE_ENABLE_ON_REINIT_SMASK                       0x1ull
/*
* Table #18 of 220_CSR_lcb.xml - LCB_CFG_RX_LN_EN
* This register sets the Rx side enable for each physical lane.
*/
#define DC_LCB_CFG_RX_LN_EN                                           (DC_LCB_CSRS + 0x000000000080)
#define DC_LCB_CFG_RX_LN_EN_RESETCSR                                  0x000000000000000Full
#define DC_LCB_CFG_RX_LN_EN_UNUSED_63_4_SHIFT                         4
#define DC_LCB_CFG_RX_LN_EN_UNUSED_63_4_MASK                          0xFFFFFFFFFFFFFFFull
#define DC_LCB_CFG_RX_LN_EN_UNUSED_63_4_SMASK                         0xFFFFFFFFFFFFFFF0ull
#define DC_LCB_CFG_RX_LN_EN_VAL_SHIFT                                 0
#define DC_LCB_CFG_RX_LN_EN_VAL_MASK                                  0xFull
#define DC_LCB_CFG_RX_LN_EN_VAL_SMASK                                 0xFull
/*
* Table #19 of 220_CSR_lcb.xml - LCB_CFG_NULLS_REQUIRED_BASE
* This register sets the base value for the number of nulls that need to be 
* inserted on each cycle through the replay buffer.
*/
#define DC_LCB_CFG_NULLS_REQUIRED_BASE                                (DC_LCB_CSRS + 0x000000000088)
#define DC_LCB_CFG_NULLS_REQUIRED_BASE_RESETCSR                       0x0000000753333333ull
#define DC_LCB_CFG_NULLS_REQUIRED_BASE_UNUSED_63_36_SHIFT             36
#define DC_LCB_CFG_NULLS_REQUIRED_BASE_UNUSED_63_36_MASK              0xFFFFFFFull
#define DC_LCB_CFG_NULLS_REQUIRED_BASE_UNUSED_63_36_SMASK             0xFFFFFFF000000000ull
#define DC_LCB_CFG_NULLS_REQUIRED_BASE_VAL8_SHIFT                     32
#define DC_LCB_CFG_NULLS_REQUIRED_BASE_VAL8_MASK                      0xFull
#define DC_LCB_CFG_NULLS_REQUIRED_BASE_VAL8_SMASK                     0xF00000000ull
#define DC_LCB_CFG_NULLS_REQUIRED_BASE_VAL7_SHIFT                     28
#define DC_LCB_CFG_NULLS_REQUIRED_BASE_VAL7_MASK                      0xFull
#define DC_LCB_CFG_NULLS_REQUIRED_BASE_VAL7_SMASK                     0xF0000000ull
#define DC_LCB_CFG_NULLS_REQUIRED_BASE_VAL6_SHIFT                     24
#define DC_LCB_CFG_NULLS_REQUIRED_BASE_VAL6_MASK                      0xFull
#define DC_LCB_CFG_NULLS_REQUIRED_BASE_VAL6_SMASK                     0xF000000ull
#define DC_LCB_CFG_NULLS_REQUIRED_BASE_VAL5_SHIFT                     20
#define DC_LCB_CFG_NULLS_REQUIRED_BASE_VAL5_MASK                      0xFull
#define DC_LCB_CFG_NULLS_REQUIRED_BASE_VAL5_SMASK                     0xF00000ull
#define DC_LCB_CFG_NULLS_REQUIRED_BASE_VAL4_SHIFT                     16
#define DC_LCB_CFG_NULLS_REQUIRED_BASE_VAL4_MASK                      0xFull
#define DC_LCB_CFG_NULLS_REQUIRED_BASE_VAL4_SMASK                     0xF0000ull
#define DC_LCB_CFG_NULLS_REQUIRED_BASE_VAL3_SHIFT                     12
#define DC_LCB_CFG_NULLS_REQUIRED_BASE_VAL3_MASK                      0xFull
#define DC_LCB_CFG_NULLS_REQUIRED_BASE_VAL3_SMASK                     0xF000ull
#define DC_LCB_CFG_NULLS_REQUIRED_BASE_VAL2_SHIFT                     8
#define DC_LCB_CFG_NULLS_REQUIRED_BASE_VAL2_MASK                      0xFull
#define DC_LCB_CFG_NULLS_REQUIRED_BASE_VAL2_SMASK                     0xF00ull
#define DC_LCB_CFG_NULLS_REQUIRED_BASE_VAL1_SHIFT                     4
#define DC_LCB_CFG_NULLS_REQUIRED_BASE_VAL1_MASK                      0xFull
#define DC_LCB_CFG_NULLS_REQUIRED_BASE_VAL1_SMASK                     0xF0ull
#define DC_LCB_CFG_NULLS_REQUIRED_BASE_VAL0_SHIFT                     0
#define DC_LCB_CFG_NULLS_REQUIRED_BASE_VAL0_MASK                      0xFull
#define DC_LCB_CFG_NULLS_REQUIRED_BASE_VAL0_SMASK                     0xFull
/*
* Table #20 of 220_CSR_lcb.xml - LCB_CFG_REINITS_BEFORE_LINK_LOW
* This register sets the number of last ditch reinit attempts before the link 
* goes down. These are triggered when the link times out or a incomplete round 
* trip occurs. The corresponding error flags are set.
*/
#define DC_LCB_CFG_REINITS_BEFORE_LINK_LOW                            (DC_LCB_CSRS + 0x000000000090)
#define DC_LCB_CFG_REINITS_BEFORE_LINK_LOW_RESETCSR                   0x0000000000000003ull
#define DC_LCB_CFG_REINITS_BEFORE_LINK_LOW_UNUSED_63_2_SHIFT          2
#define DC_LCB_CFG_REINITS_BEFORE_LINK_LOW_UNUSED_63_2_MASK           0x3FFFFFFFFFFFFFFFull
#define DC_LCB_CFG_REINITS_BEFORE_LINK_LOW_UNUSED_63_2_SMASK          0xFFFFFFFFFFFFFFFCull
#define DC_LCB_CFG_REINITS_BEFORE_LINK_LOW_VAL_SHIFT                  0
#define DC_LCB_CFG_REINITS_BEFORE_LINK_LOW_VAL_MASK                   0x3ull
#define DC_LCB_CFG_REINITS_BEFORE_LINK_LOW_VAL_SMASK                  0x3ull
/*
* Table #21 of 220_CSR_lcb.xml - LCB_CFG_REMOVE_RANDOM_NULLS
* This register is used to remove the random data present in the reserved bits 
* of Null LTPs.
*/
#define DC_LCB_CFG_REMOVE_RANDOM_NULLS                                (DC_LCB_CSRS + 0x000000000098)
#define DC_LCB_CFG_REMOVE_RANDOM_NULLS_RESETCSR                       0x0000000000000000ull
#define DC_LCB_CFG_REMOVE_RANDOM_NULLS_UNUSED_63_1_SHIFT              1
#define DC_LCB_CFG_REMOVE_RANDOM_NULLS_UNUSED_63_1_MASK               0x7FFFFFFFFFFFFFFFull
#define DC_LCB_CFG_REMOVE_RANDOM_NULLS_UNUSED_63_1_SMASK              0xFFFFFFFFFFFFFFFEull
#define DC_LCB_CFG_REMOVE_RANDOM_NULLS_VAL_SHIFT                      0
#define DC_LCB_CFG_REMOVE_RANDOM_NULLS_VAL_MASK                       0x1ull
#define DC_LCB_CFG_REMOVE_RANDOM_NULLS_VAL_SMASK                      0x1ull
/*
* Table #22 of 220_CSR_lcb.xml - LCB_CFG_REPLAY_BUF_MAX_LTP
* This register sets the maximum replay buffer LTP number sent to the peer Rx to 
* inform it of the replay buffer size on the Tx side of the link.
*/
#define DC_LCB_CFG_REPLAY_BUF_MAX_LTP                                 (DC_LCB_CSRS + 0x0000000000A0)
#define DC_LCB_CFG_REPLAY_BUF_MAX_LTP_RESETCSR                        0x000000000000007Full
#define DC_LCB_CFG_REPLAY_BUF_MAX_LTP_UNUSED_63_15_SHIFT              15
#define DC_LCB_CFG_REPLAY_BUF_MAX_LTP_UNUSED_63_15_MASK               0x1FFFFFFFFFFFFull
#define DC_LCB_CFG_REPLAY_BUF_MAX_LTP_UNUSED_63_15_SMASK              0xFFFFFFFFFFFF8000ull
#define DC_LCB_CFG_REPLAY_BUF_MAX_LTP_NUM_SHIFT                       0
#define DC_LCB_CFG_REPLAY_BUF_MAX_LTP_NUM_MASK                        0x7FFFull
#define DC_LCB_CFG_REPLAY_BUF_MAX_LTP_NUM_SMASK                       0x7FFFull
/*
* Table #23 of 220_CSR_lcb.xml - LCB_CFG_THROTTLE_TX_FLITS
* This register is used to throttle the tx flit stream by forcing idles in 
* specific LTP flit locations. This can be used to emulate 1,2,3 lanes active 
* for L0p power reduction experiments.
*/
#define DC_LCB_CFG_THROTTLE_TX_FLITS                                  (DC_LCB_CSRS + 0x0000000000A8)
#define DC_LCB_CFG_THROTTLE_TX_FLITS_RESETCSR                         0x0000000000000000ull
#define DC_LCB_CFG_THROTTLE_TX_FLITS_UNUSED_63_8_SHIFT                8
#define DC_LCB_CFG_THROTTLE_TX_FLITS_UNUSED_63_8_MASK                 0xFFFFFFFFFFFFFFull
#define DC_LCB_CFG_THROTTLE_TX_FLITS_UNUSED_63_8_SMASK                0xFFFFFFFFFFFFFF00ull
#define DC_LCB_CFG_THROTTLE_TX_FLITS_EN_SHIFT                         0
#define DC_LCB_CFG_THROTTLE_TX_FLITS_EN_MASK                          0xFFull
#define DC_LCB_CFG_THROTTLE_TX_FLITS_EN_SMASK                         0xFFull
/*
* Table #24 of 220_CSR_lcb.xml - LCB_CFG_TX_SWIZZLE
* This register is used to control lane swizzle on the Tx side.
*/
#define DC_LCB_CFG_TX_SWIZZLE                                         (DC_LCB_CSRS + 0x0000000000B0)
#define DC_LCB_CFG_TX_SWIZZLE_RESETCSR                                0x0000000000003210ull
#define DC_LCB_CFG_TX_SWIZZLE_UNUSED_63_15_SHIFT                      15
#define DC_LCB_CFG_TX_SWIZZLE_UNUSED_63_15_MASK                       0x1FFFFFFFFFFFFull
#define DC_LCB_CFG_TX_SWIZZLE_UNUSED_63_15_SMASK                      0xFFFFFFFFFFFF8000ull
#define DC_LCB_CFG_TX_SWIZZLE_LN3_SHIFT                               12
#define DC_LCB_CFG_TX_SWIZZLE_LN3_MASK                                0x7ull
#define DC_LCB_CFG_TX_SWIZZLE_LN3_SMASK                               0x7000ull
#define DC_LCB_CFG_TX_SWIZZLE_UNUSED_11_SHIFT                         11
#define DC_LCB_CFG_TX_SWIZZLE_UNUSED_11_MASK                          0x1ull
#define DC_LCB_CFG_TX_SWIZZLE_UNUSED_11_SMASK                         0x800ull
#define DC_LCB_CFG_TX_SWIZZLE_LN2_SHIFT                               8
#define DC_LCB_CFG_TX_SWIZZLE_LN2_MASK                                0x7ull
#define DC_LCB_CFG_TX_SWIZZLE_LN2_SMASK                               0x700ull
#define DC_LCB_CFG_TX_SWIZZLE_UNUSED_7_SHIFT                          7
#define DC_LCB_CFG_TX_SWIZZLE_UNUSED_7_MASK                           0x1ull
#define DC_LCB_CFG_TX_SWIZZLE_UNUSED_7_SMASK                          0x80ull
#define DC_LCB_CFG_TX_SWIZZLE_LN1_SHIFT                               4
#define DC_LCB_CFG_TX_SWIZZLE_LN1_MASK                                0x7ull
#define DC_LCB_CFG_TX_SWIZZLE_LN1_SMASK                               0x70ull
#define DC_LCB_CFG_TX_SWIZZLE_UNUSED_3_SHIFT                          3
#define DC_LCB_CFG_TX_SWIZZLE_UNUSED_3_MASK                           0x1ull
#define DC_LCB_CFG_TX_SWIZZLE_UNUSED_3_SMASK                          0x8ull
#define DC_LCB_CFG_TX_SWIZZLE_LN0_SHIFT                               0
#define DC_LCB_CFG_TX_SWIZZLE_LN0_MASK                                0x7ull
#define DC_LCB_CFG_TX_SWIZZLE_LN0_SMASK                               0x7ull
/*
* Table #25 of 220_CSR_lcb.xml - LCB_CFG_SEND_SMA_MSG
* This register is used by the subnet management agent (SMA) to send a message 
* to the peer SMA. There is no hardware mechanism provided on the Tx side to 
* indicate if the message has been sent. This must be accomplished by polling 
* the LCB_STS_RCV_SMA_MSG by the peer SMA.
*/
#define DC_LCB_CFG_SEND_SMA_MSG                                       (DC_LCB_CSRS + 0x0000000000B8)
#define DC_LCB_CFG_SEND_SMA_MSG_RESETCSR                              0x0000000000000000ull
#define DC_LCB_CFG_SEND_SMA_MSG_UNUSED_63_48_SHIFT                    48
#define DC_LCB_CFG_SEND_SMA_MSG_UNUSED_63_48_MASK                     0xFFFFull
#define DC_LCB_CFG_SEND_SMA_MSG_UNUSED_63_48_SMASK                    0xFFFF000000000000ull
#define DC_LCB_CFG_SEND_SMA_MSG_VAL_SHIFT                             0
#define DC_LCB_CFG_SEND_SMA_MSG_VAL_MASK                              0xFFFFFFFFFFFFull
#define DC_LCB_CFG_SEND_SMA_MSG_VAL_SMASK                             0xFFFFFFFFFFFFull
/*
* Table #26 of 220_CSR_lcb.xml - LCB_CFG_INCOMPLT_RND_TRIP_DISABLE
* This register can be used to disable the incomplete round trip error 
* handling.
*/
#define DC_LCB_CFG_INCOMPLT_RND_TRIP_DISABLE                          (DC_LCB_CSRS + 0x0000000000C0)
#define DC_LCB_CFG_INCOMPLT_RND_TRIP_DISABLE_RESETCSR                 0x0000000000000000ull
#define DC_LCB_CFG_INCOMPLT_RND_TRIP_DISABLE_UNUSED_63_1_SHIFT        1
#define DC_LCB_CFG_INCOMPLT_RND_TRIP_DISABLE_UNUSED_63_1_MASK         0x7FFFFFFFFFFFFFFFull
#define DC_LCB_CFG_INCOMPLT_RND_TRIP_DISABLE_UNUSED_63_1_SMASK        0xFFFFFFFFFFFFFFFEull
#define DC_LCB_CFG_INCOMPLT_RND_TRIP_DISABLE_VAL_SHIFT                0
#define DC_LCB_CFG_INCOMPLT_RND_TRIP_DISABLE_VAL_MASK                 0x1ull
#define DC_LCB_CFG_INCOMPLT_RND_TRIP_DISABLE_VAL_SMASK                0x1ull
/*
* Table #27 of 220_CSR_lcb.xml - LCB_CFG_RND_TRIP_MAX
* This register can be used to set the value at which the RST_FOR_INCOMPLT_RND_TRIP 
* error flag is triggered.
*/
#define DC_LCB_CFG_RND_TRIP_MAX                                       (DC_LCB_CSRS + 0x0000000000C8)
#define DC_LCB_CFG_RND_TRIP_MAX_RESETCSR                              0x000000000000FFFFull
#define DC_LCB_CFG_RND_TRIP_MAX_UNUSED_63_16_SHIFT                    16
#define DC_LCB_CFG_RND_TRIP_MAX_UNUSED_63_16_MASK                     0xFFFFFFFFFFFFull
#define DC_LCB_CFG_RND_TRIP_MAX_UNUSED_63_16_SMASK                    0xFFFFFFFFFFFF0000ull
#define DC_LCB_CFG_RND_TRIP_MAX_VAL_SHIFT                             0
#define DC_LCB_CFG_RND_TRIP_MAX_VAL_MASK                              0xFFFFull
#define DC_LCB_CFG_RND_TRIP_MAX_VAL_SMASK                             0xFFFFull
/*
* Table #28 of 220_CSR_lcb.xml - LCB_CFG_LINK_TIMER_DISABLE
* This register can be used to disable the link transfer timer.
*/
#define DC_LCB_CFG_LINK_TIMER_DISABLE                                 (DC_LCB_CSRS + 0x0000000000D0)
#define DC_LCB_CFG_LINK_TIMER_DISABLE_RESETCSR                        0x0000000000000000ull
#define DC_LCB_CFG_LINK_TIMER_DISABLE_UNUSED_63_1_SHIFT               1
#define DC_LCB_CFG_LINK_TIMER_DISABLE_UNUSED_63_1_MASK                0x7FFFFFFFFFFFFFFFull
#define DC_LCB_CFG_LINK_TIMER_DISABLE_UNUSED_63_1_SMASK               0xFFFFFFFFFFFFFFFEull
#define DC_LCB_CFG_LINK_TIMER_DISABLE_VAL_SHIFT                       0
#define DC_LCB_CFG_LINK_TIMER_DISABLE_VAL_MASK                        0x1ull
#define DC_LCB_CFG_LINK_TIMER_DISABLE_VAL_SMASK                       0x1ull
/*
* Table #29 of 220_CSR_lcb.xml - LCB_CFG_LINK_TIMER_MAX
* This register sets the link timer maximum value.
*/
#define DC_LCB_CFG_LINK_TIMER_MAX                                     (DC_LCB_CSRS + 0x0000000000D8)
#define DC_LCB_CFG_LINK_TIMER_MAX_RESETCSR                            0x00000000000FFFFFull
#define DC_LCB_CFG_LINK_TIMER_MAX_UNUSED_63_32_SHIFT                  32
#define DC_LCB_CFG_LINK_TIMER_MAX_UNUSED_63_32_MASK                   0xFFFFFFFFull
#define DC_LCB_CFG_LINK_TIMER_MAX_UNUSED_63_32_SMASK                  0xFFFFFFFF00000000ull
#define DC_LCB_CFG_LINK_TIMER_MAX_VAL_SHIFT                           0
#define DC_LCB_CFG_LINK_TIMER_MAX_VAL_MASK                            0xFFFFFFFFull
#define DC_LCB_CFG_LINK_TIMER_MAX_VAL_SMASK                           0xFFFFFFFFull
/*
* Table #30 of 220_CSR_lcb.xml - LCB_CFG_DESKEW
* This register can be used to disable deskew altogether or just the deskew 
* timer.
*/
#define DC_LCB_CFG_DESKEW                                             (DC_LCB_CSRS + 0x0000000000E0)
#define DC_LCB_CFG_DESKEW_RESETCSR                                    0x0000000000000000ull
#define DC_LCB_CFG_DESKEW_UNUSED_63_5_SHIFT                           5
#define DC_LCB_CFG_DESKEW_UNUSED_63_5_MASK                            0x7FFFFFFFFFFFFFFull
#define DC_LCB_CFG_DESKEW_UNUSED_63_5_SMASK                           0xFFFFFFFFFFFFFFE0ull
#define DC_LCB_CFG_DESKEW_TIMER_DISABLE_SHIFT                         4
#define DC_LCB_CFG_DESKEW_TIMER_DISABLE_MASK                          0x1ull
#define DC_LCB_CFG_DESKEW_TIMER_DISABLE_SMASK                         0x10ull
#define DC_LCB_CFG_DESKEW_UNUSED_3_1_SHIFT                            1
#define DC_LCB_CFG_DESKEW_UNUSED_3_1_MASK                             0x7ull
#define DC_LCB_CFG_DESKEW_UNUSED_3_1_SMASK                            0xEull
#define DC_LCB_CFG_DESKEW_ALL_DISABLE_SHIFT                           0
#define DC_LCB_CFG_DESKEW_ALL_DISABLE_MASK                            0x1ull
#define DC_LCB_CFG_DESKEW_ALL_DISABLE_SMASK                           0x1ull
/*
* Table #31 of 220_CSR_lcb.xml - LCB_CFG_LN_DEGRADE
* This register is used to configure the lane degrade removal mechanism. All 
* fields should be configured prior to booting the LCB. If any fields need to be 
* changed after a LCB boot RESET_SETTINGS needs to be used.
*/
#define DC_LCB_CFG_LN_DEGRADE                                         (DC_LCB_CSRS + 0x0000000000E8)
#define DC_LCB_CFG_LN_DEGRADE_RESETCSR                                0x0000000000002731ull
#define DC_LCB_CFG_LN_DEGRADE_UNUSED_63_21_SHIFT                      21
#define DC_LCB_CFG_LN_DEGRADE_UNUSED_63_21_MASK                       0x7FFFFFFFFFFull
#define DC_LCB_CFG_LN_DEGRADE_UNUSED_63_21_SMASK                      0xFFFFFFFFFFE00000ull
#define DC_LCB_CFG_LN_DEGRADE_RESET_DEGRADE_LN_EN_SHIFT               20
#define DC_LCB_CFG_LN_DEGRADE_RESET_DEGRADE_LN_EN_MASK                0x1ull
#define DC_LCB_CFG_LN_DEGRADE_RESET_DEGRADE_LN_EN_SMASK               0x100000ull
#define DC_LCB_CFG_LN_DEGRADE_UNUSED_19_17_SHIFT                      17
#define DC_LCB_CFG_LN_DEGRADE_UNUSED_19_17_MASK                       0x7ull
#define DC_LCB_CFG_LN_DEGRADE_UNUSED_19_17_SMASK                      0xE0000ull
#define DC_LCB_CFG_LN_DEGRADE_RESET_SETTINGS_SHIFT                    16
#define DC_LCB_CFG_LN_DEGRADE_RESET_SETTINGS_MASK                     0x1ull
#define DC_LCB_CFG_LN_DEGRADE_RESET_SETTINGS_SMASK                    0x10000ull
#define DC_LCB_CFG_LN_DEGRADE_UNUSED_15_SHIFT                         15
#define DC_LCB_CFG_LN_DEGRADE_UNUSED_15_MASK                          0x1ull
#define DC_LCB_CFG_LN_DEGRADE_UNUSED_15_SMASK                         0x8000ull
#define DC_LCB_CFG_LN_DEGRADE_EVENTS_TO_TRIGGER_SHIFT                 12
#define DC_LCB_CFG_LN_DEGRADE_EVENTS_TO_TRIGGER_MASK                  0x7ull
#define DC_LCB_CFG_LN_DEGRADE_EVENTS_TO_TRIGGER_SMASK                 0x7000ull
#define DC_LCB_CFG_LN_DEGRADE_UNUSED_11_SHIFT                         11
#define DC_LCB_CFG_LN_DEGRADE_UNUSED_11_MASK                          0x1ull
#define DC_LCB_CFG_LN_DEGRADE_UNUSED_11_SMASK                         0x800ull
#define DC_LCB_CFG_LN_DEGRADE_DURATION_SELECT_SHIFT                   8
#define DC_LCB_CFG_LN_DEGRADE_DURATION_SELECT_MASK                    0x7ull
#define DC_LCB_CFG_LN_DEGRADE_DURATION_SELECT_SMASK                   0x700ull
#define DC_LCB_CFG_LN_DEGRADE_UNUSED_7_6_SHIFT                        6
#define DC_LCB_CFG_LN_DEGRADE_UNUSED_7_6_MASK                         0x3ull
#define DC_LCB_CFG_LN_DEGRADE_UNUSED_7_6_SMASK                        0xC0ull
#define DC_LCB_CFG_LN_DEGRADE_NUM_ALLOWED_SHIFT                       4
#define DC_LCB_CFG_LN_DEGRADE_NUM_ALLOWED_MASK                        0x3ull
#define DC_LCB_CFG_LN_DEGRADE_NUM_ALLOWED_SMASK                       0x30ull
#define DC_LCB_CFG_LN_DEGRADE_UNUSED_3_1_SHIFT                        1
#define DC_LCB_CFG_LN_DEGRADE_UNUSED_3_1_MASK                         0x7ull
#define DC_LCB_CFG_LN_DEGRADE_UNUSED_3_1_SMASK                        0xEull
#define DC_LCB_CFG_LN_DEGRADE_EN_SHIFT                                0
#define DC_LCB_CFG_LN_DEGRADE_EN_MASK                                 0x1ull
#define DC_LCB_CFG_LN_DEGRADE_EN_SMASK                                0x1ull
/*
* Table #32 of 220_CSR_lcb.xml - LCB_CFG_CRC_INTERRUPT
* This register is used to configure which CRC error counter(s) is used to 
* monitor for a potential interrupt condition. The interrupt occurs when the 
* monitored error counter(s) reaches the number of errors in the ERR_CNT 
* field.
*/
#define DC_LCB_CFG_CRC_INTERRUPT                                      (DC_LCB_CSRS + 0x0000000000F0)
#define DC_LCB_CFG_CRC_INTERRUPT_RESETCSR                             0x0000000000000004ull
#define DC_LCB_CFG_CRC_INTERRUPT_UNUSED_63_38_SHIFT                   38
#define DC_LCB_CFG_CRC_INTERRUPT_UNUSED_63_38_MASK                    0x3FFFFFFull
#define DC_LCB_CFG_CRC_INTERRUPT_UNUSED_63_38_SMASK                   0xFFFFFFC000000000ull
#define DC_LCB_CFG_CRC_INTERRUPT_MATCH_EN_SHIFT                       32
#define DC_LCB_CFG_CRC_INTERRUPT_MATCH_EN_MASK                        0x3Full
#define DC_LCB_CFG_CRC_INTERRUPT_MATCH_EN_SMASK                       0x3F00000000ull
#define DC_LCB_CFG_CRC_INTERRUPT_ERR_CNT_SHIFT                        0
#define DC_LCB_CFG_CRC_INTERRUPT_ERR_CNT_MASK                         0xFFFFFFFFull
#define DC_LCB_CFG_CRC_INTERRUPT_ERR_CNT_SMASK                        0xFFFFFFFFull
/*
* Table #33 of 220_CSR_lcb.xml - LCB_CFG_LOOPBACK
* This is use to set the loopback mode.
*/
#define DC_LCB_CFG_LOOPBACK                                           (DC_LCB_CSRS + 0x0000000000F8)
#define DC_LCB_CFG_LOOPBACK_RESETCSR                                  0x0000000000000000ull
#define DC_LCB_CFG_LOOPBACK_UNUSED_63_2_SHIFT                         2
#define DC_LCB_CFG_LOOPBACK_UNUSED_63_2_MASK                          0x3FFFFFFFFFFFFFFFull
#define DC_LCB_CFG_LOOPBACK_UNUSED_63_2_SMASK                         0xFFFFFFFFFFFFFFFCull
#define DC_LCB_CFG_LOOPBACK_VAL_SHIFT                                 0
#define DC_LCB_CFG_LOOPBACK_VAL_MASK                                  0x3ull
#define DC_LCB_CFG_LOOPBACK_VAL_SMASK                                 0x3ull
/*
* Table #34 of 220_CSR_lcb.xml - LCB_CFG_LANE_WIDTH
* Set the width of the interface to the lanes.
*/
#define DC_LCB_CFG_LANE_WIDTH                                         (DC_LCB_CSRS + 0x000000000100)
#define DC_LCB_CFG_LANE_WIDTH_RESETCSR                                0x0000000000000003ull
#define DC_LCB_CFG_LANE_WIDTH_UNUSED_63_2_SHIFT                       2
#define DC_LCB_CFG_LANE_WIDTH_UNUSED_63_2_MASK                        0x3FFFFFFFFFFFFFFFull
#define DC_LCB_CFG_LANE_WIDTH_UNUSED_63_2_SMASK                       0xFFFFFFFFFFFFFFFCull
#define DC_LCB_CFG_LANE_WIDTH_VAL_SHIFT                               0
#define DC_LCB_CFG_LANE_WIDTH_VAL_MASK                                0x3ull
#define DC_LCB_CFG_LANE_WIDTH_VAL_SMASK                               0x3ull
/*
* Table #35 of 220_CSR_lcb.xml - LCB_CFG_MISC
* This register is used to set miscellaneous configuration setting that are 
* unlikely to be needed. This can serve as a catch all for configuration needs 
* that arise late in the design process without adding a completely new 
* CSR.
*/
#define DC_LCB_CFG_MISC                                               (DC_LCB_CSRS + 0x000000000108)
#define DC_LCB_CFG_MISC_RESETCSR                                      0x0000000000001000ull
#define DC_LCB_CFG_MISC_UNUSED_63_14_SHIFT                            14
#define DC_LCB_CFG_MISC_UNUSED_63_14_MASK                             0x3FFFFFFFFFFFFull
#define DC_LCB_CFG_MISC_UNUSED_63_14_SMASK                            0xFFFFFFFFFFFFC000ull
#define DC_LCB_CFG_MISC_SEL_REQUIRED_TOS_CNT_SHIFT                    12
#define DC_LCB_CFG_MISC_SEL_REQUIRED_TOS_CNT_MASK                     0x3ull
#define DC_LCB_CFG_MISC_SEL_REQUIRED_TOS_CNT_SMASK                    0x3000ull
#define DC_LCB_CFG_MISC_UNUSED_11_10_SHIFT                            10
#define DC_LCB_CFG_MISC_UNUSED_11_10_MASK                             0x3ull
#define DC_LCB_CFG_MISC_UNUSED_11_10_SMASK                            0xC00ull
#define DC_LCB_CFG_MISC_RX_CLK_SEL_PRIORITY_SHIFT                     8
#define DC_LCB_CFG_MISC_RX_CLK_SEL_PRIORITY_MASK                      0x3ull
#define DC_LCB_CFG_MISC_RX_CLK_SEL_PRIORITY_SMASK                     0x300ull
#define DC_LCB_CFG_MISC_UNUSED_7_6_SHIFT                              6
#define DC_LCB_CFG_MISC_UNUSED_7_6_MASK                               0x3ull
#define DC_LCB_CFG_MISC_UNUSED_7_6_SMASK                              0xC0ull
#define DC_LCB_CFG_MISC_BCK_CH_SEL_PRIORITY_SHIFT                     4
#define DC_LCB_CFG_MISC_BCK_CH_SEL_PRIORITY_MASK                      0x3ull
#define DC_LCB_CFG_MISC_BCK_CH_SEL_PRIORITY_SMASK                     0x30ull
#define DC_LCB_CFG_MISC_FORCE_LN_EN_SHIFT                             0
#define DC_LCB_CFG_MISC_FORCE_LN_EN_MASK                              0xFull
#define DC_LCB_CFG_MISC_FORCE_LN_EN_SMASK                             0xFull
/*
* Table #36 of 220_CSR_lcb.xml - LCB_CFG_CLK_CNTR
* This register is used to select the clk that is monitored by the PRF_CLK_CNTR 
* and sent to the o_clk_observe output.
*/
#define DC_LCB_CFG_CLK_CNTR                                           (DC_LCB_CSRS + 0x000000000110)
#define DC_LCB_CFG_CLK_CNTR_RESETCSR                                  0x0000000000000001ull
#define DC_LCB_CFG_CLK_CNTR_UNUSED_63_9_SHIFT                         9
#define DC_LCB_CFG_CLK_CNTR_UNUSED_63_9_MASK                          0x7FFFFFFFFFFFFFull
#define DC_LCB_CFG_CLK_CNTR_UNUSED_63_9_SMASK                         0xFFFFFFFFFFFFFE00ull
#define DC_LCB_CFG_CLK_CNTR_STROBE_ON_CCLK_CNTR_SHIFT                 8
#define DC_LCB_CFG_CLK_CNTR_STROBE_ON_CCLK_CNTR_MASK                  0x1ull
#define DC_LCB_CFG_CLK_CNTR_STROBE_ON_CCLK_CNTR_SMASK                 0x100ull
#define DC_LCB_CFG_CLK_CNTR_UNUSED_7_5_SHIFT                          5
#define DC_LCB_CFG_CLK_CNTR_UNUSED_7_5_MASK                           0x7ull
#define DC_LCB_CFG_CLK_CNTR_UNUSED_7_5_SMASK                          0xE0ull
#define DC_LCB_CFG_CLK_CNTR_SELECT_SHIFT                              0
#define DC_LCB_CFG_CLK_CNTR_SELECT_MASK                               0x1Full
#define DC_LCB_CFG_CLK_CNTR_SELECT_SMASK                              0x1Full
/*
* Table #37 of 220_CSR_lcb.xml - LCB_CFG_CRC_CNTR_RESET
* This register is used to hold and reset the CRC error counters.
*/
#define DC_LCB_CFG_CRC_CNTR_RESET                                     (DC_LCB_CSRS + 0x000000000118)
#define DC_LCB_CFG_CRC_CNTR_RESET_RESETCSR                            0x0000000000000000ull
#define DC_LCB_CFG_CRC_CNTR_RESET_UNUSED_63_5_SHIFT                   5
#define DC_LCB_CFG_CRC_CNTR_RESET_UNUSED_63_5_MASK                    0x7FFFFFFFFFFFFFFull
#define DC_LCB_CFG_CRC_CNTR_RESET_UNUSED_63_5_SMASK                   0xFFFFFFFFFFFFFFE0ull
#define DC_LCB_CFG_CRC_CNTR_RESET_HOLD_CRC_ERR_CNTRS_SHIFT            4
#define DC_LCB_CFG_CRC_CNTR_RESET_HOLD_CRC_ERR_CNTRS_MASK             0x1ull
#define DC_LCB_CFG_CRC_CNTR_RESET_HOLD_CRC_ERR_CNTRS_SMASK            0x10ull
#define DC_LCB_CFG_CRC_CNTR_RESET_UNUSED_3_1_SHIFT                    1
#define DC_LCB_CFG_CRC_CNTR_RESET_UNUSED_3_1_MASK                     0x7ull
#define DC_LCB_CFG_CRC_CNTR_RESET_UNUSED_3_1_SMASK                    0xEull
#define DC_LCB_CFG_CRC_CNTR_RESET_CLR_CRC_ERR_CNTRS_SHIFT             0
#define DC_LCB_CFG_CRC_CNTR_RESET_CLR_CRC_ERR_CNTRS_MASK              0x1ull
#define DC_LCB_CFG_CRC_CNTR_RESET_CLR_CRC_ERR_CNTRS_SMASK             0x1ull
/*
* Table #38 of 220_CSR_lcb.xml - LCB_CFG_LINK_KILL_EN
* This register controls whether a particular error condition will take the link 
* down. It has a 1:1 correspondence with the error flags.
*/
#define DC_LCB_CFG_LINK_KILL_EN                                       (DC_LCB_CSRS + 0x000000000120)
#define DC_LCB_CFG_LINK_KILL_EN_RESETCSR                              0x0000000038037810ull
#define DC_LCB_CFG_LINK_KILL_EN_UNUSED_63_30_SHIFT                    30
#define DC_LCB_CFG_LINK_KILL_EN_UNUSED_63_30_MASK                     0x3FFFFFFFFull
#define DC_LCB_CFG_LINK_KILL_EN_UNUSED_63_30_SMASK                    0xFFFFFFFFC0000000ull
#define DC_LCB_CFG_LINK_KILL_EN_REDUNDANT_FLIT_PARITY_ERR_SHIFT       29
#define DC_LCB_CFG_LINK_KILL_EN_REDUNDANT_FLIT_PARITY_ERR_MASK        0x1ull
#define DC_LCB_CFG_LINK_KILL_EN_REDUNDANT_FLIT_PARITY_ERR_SMASK       0x20000000ull
#define DC_LCB_CFG_LINK_KILL_EN_NEG_EDGE_LINK_TRANSFER_ACTIVE_SHIFT   28
#define DC_LCB_CFG_LINK_KILL_EN_NEG_EDGE_LINK_TRANSFER_ACTIVE_MASK    0x1ull
#define DC_LCB_CFG_LINK_KILL_EN_NEG_EDGE_LINK_TRANSFER_ACTIVE_SMASK   0x10000000ull
#define DC_LCB_CFG_LINK_KILL_EN_HOLD_REINIT_SHIFT                     27
#define DC_LCB_CFG_LINK_KILL_EN_HOLD_REINIT_MASK                      0x1ull
#define DC_LCB_CFG_LINK_KILL_EN_HOLD_REINIT_SMASK                     0x8000000ull
#define DC_LCB_CFG_LINK_KILL_EN_RST_FOR_INCOMPLT_RND_TRIP_SHIFT       26
#define DC_LCB_CFG_LINK_KILL_EN_RST_FOR_INCOMPLT_RND_TRIP_MASK        0x1ull
#define DC_LCB_CFG_LINK_KILL_EN_RST_FOR_INCOMPLT_RND_TRIP_SMASK       0x4000000ull
#define DC_LCB_CFG_LINK_KILL_EN_RST_FOR_LINK_TIMEOUT_SHIFT            25
#define DC_LCB_CFG_LINK_KILL_EN_RST_FOR_LINK_TIMEOUT_MASK             0x1ull
#define DC_LCB_CFG_LINK_KILL_EN_RST_FOR_LINK_TIMEOUT_SMASK            0x2000000ull
#define DC_LCB_CFG_LINK_KILL_EN_CREDIT_RETURN_FLIT_MBE_SHIFT          24
#define DC_LCB_CFG_LINK_KILL_EN_CREDIT_RETURN_FLIT_MBE_MASK           0x1ull
#define DC_LCB_CFG_LINK_KILL_EN_CREDIT_RETURN_FLIT_MBE_SMASK          0x1000000ull
#define DC_LCB_CFG_LINK_KILL_EN_REPLAY_BUF_SBE_SHIFT                  23
#define DC_LCB_CFG_LINK_KILL_EN_REPLAY_BUF_SBE_MASK                   0x1ull
#define DC_LCB_CFG_LINK_KILL_EN_REPLAY_BUF_SBE_SMASK                  0x800000ull
#define DC_LCB_CFG_LINK_KILL_EN_REPLAY_BUF_MBE_SHIFT                  22
#define DC_LCB_CFG_LINK_KILL_EN_REPLAY_BUF_MBE_MASK                   0x1ull
#define DC_LCB_CFG_LINK_KILL_EN_REPLAY_BUF_MBE_SMASK                  0x400000ull
#define DC_LCB_CFG_LINK_KILL_EN_FLIT_INPUT_BUF_SBE_SHIFT              21
#define DC_LCB_CFG_LINK_KILL_EN_FLIT_INPUT_BUF_SBE_MASK               0x1ull
#define DC_LCB_CFG_LINK_KILL_EN_FLIT_INPUT_BUF_SBE_SMASK              0x200000ull
#define DC_LCB_CFG_LINK_KILL_EN_FLIT_INPUT_BUF_MBE_SHIFT              20
#define DC_LCB_CFG_LINK_KILL_EN_FLIT_INPUT_BUF_MBE_MASK               0x1ull
#define DC_LCB_CFG_LINK_KILL_EN_FLIT_INPUT_BUF_MBE_SMASK              0x100000ull
#define DC_LCB_CFG_LINK_KILL_EN_VL_ACK_INPUT_WRONG_CRC_MODE_SHIFT     19
#define DC_LCB_CFG_LINK_KILL_EN_VL_ACK_INPUT_WRONG_CRC_MODE_MASK      0x1ull
#define DC_LCB_CFG_LINK_KILL_EN_VL_ACK_INPUT_WRONG_CRC_MODE_SMASK     0x80000ull
#define DC_LCB_CFG_LINK_KILL_EN_VL_ACK_INPUT_PARITY_ERR_SHIFT         18
#define DC_LCB_CFG_LINK_KILL_EN_VL_ACK_INPUT_PARITY_ERR_MASK          0x1ull
#define DC_LCB_CFG_LINK_KILL_EN_VL_ACK_INPUT_PARITY_ERR_SMASK         0x40000ull
#define DC_LCB_CFG_LINK_KILL_EN_VL_ACK_INPUT_BUF_OFLW_SHIFT           17
#define DC_LCB_CFG_LINK_KILL_EN_VL_ACK_INPUT_BUF_OFLW_MASK            0x1ull
#define DC_LCB_CFG_LINK_KILL_EN_VL_ACK_INPUT_BUF_OFLW_SMASK           0x20000ull
#define DC_LCB_CFG_LINK_KILL_EN_FLIT_INPUT_BUF_OFLW_SHIFT             16
#define DC_LCB_CFG_LINK_KILL_EN_FLIT_INPUT_BUF_OFLW_MASK              0x1ull
#define DC_LCB_CFG_LINK_KILL_EN_FLIT_INPUT_BUF_OFLW_SMASK             0x10000ull
#define DC_LCB_CFG_LINK_KILL_EN_ILLEGAL_FLIT_ENCODING_SHIFT           15
#define DC_LCB_CFG_LINK_KILL_EN_ILLEGAL_FLIT_ENCODING_MASK            0x1ull
#define DC_LCB_CFG_LINK_KILL_EN_ILLEGAL_FLIT_ENCODING_SMASK           0x8000ull
#define DC_LCB_CFG_LINK_KILL_EN_ILLEGAL_NULL_LTP_SHIFT                14
#define DC_LCB_CFG_LINK_KILL_EN_ILLEGAL_NULL_LTP_MASK                 0x1ull
#define DC_LCB_CFG_LINK_KILL_EN_ILLEGAL_NULL_LTP_SMASK                0x4000ull
#define DC_LCB_CFG_LINK_KILL_EN_UNEXPECTED_ROUND_TRIP_MARKER_SHIFT    13
#define DC_LCB_CFG_LINK_KILL_EN_UNEXPECTED_ROUND_TRIP_MARKER_MASK     0x1ull
#define DC_LCB_CFG_LINK_KILL_EN_UNEXPECTED_ROUND_TRIP_MARKER_SMASK    0x2000ull
#define DC_LCB_CFG_LINK_KILL_EN_UNEXPECTED_REPLAY_MARKER_SHIFT        12
#define DC_LCB_CFG_LINK_KILL_EN_UNEXPECTED_REPLAY_MARKER_MASK         0x1ull
#define DC_LCB_CFG_LINK_KILL_EN_UNEXPECTED_REPLAY_MARKER_SMASK        0x1000ull
#define DC_LCB_CFG_LINK_KILL_EN_RCLK_STOPPED_SHIFT                    11
#define DC_LCB_CFG_LINK_KILL_EN_RCLK_STOPPED_MASK                     0x1ull
#define DC_LCB_CFG_LINK_KILL_EN_RCLK_STOPPED_SMASK                    0x800ull
#define DC_LCB_CFG_LINK_KILL_EN_CRC_ERR_CNT_HIT_LIMIT_SHIFT           10
#define DC_LCB_CFG_LINK_KILL_EN_CRC_ERR_CNT_HIT_LIMIT_MASK            0x1ull
#define DC_LCB_CFG_LINK_KILL_EN_CRC_ERR_CNT_HIT_LIMIT_SMASK           0x400ull
#define DC_LCB_CFG_LINK_KILL_EN_REINIT_FOR_LN_DEGRADE_SHIFT           9
#define DC_LCB_CFG_LINK_KILL_EN_REINIT_FOR_LN_DEGRADE_MASK            0x1ull
#define DC_LCB_CFG_LINK_KILL_EN_REINIT_FOR_LN_DEGRADE_SMASK           0x200ull
#define DC_LCB_CFG_LINK_KILL_EN_REINIT_FROM_PEER_SHIFT                8
#define DC_LCB_CFG_LINK_KILL_EN_REINIT_FROM_PEER_MASK                 0x1ull
#define DC_LCB_CFG_LINK_KILL_EN_REINIT_FROM_PEER_SMASK                0x100ull
#define DC_LCB_CFG_LINK_KILL_EN_SEQ_CRC_ERR_SHIFT                     7
#define DC_LCB_CFG_LINK_KILL_EN_SEQ_CRC_ERR_MASK                      0x1ull
#define DC_LCB_CFG_LINK_KILL_EN_SEQ_CRC_ERR_SMASK                     0x80ull
#define DC_LCB_CFG_LINK_KILL_EN_RX_LESS_THAN_FOUR_LNS_SHIFT           6
#define DC_LCB_CFG_LINK_KILL_EN_RX_LESS_THAN_FOUR_LNS_MASK            0x1ull
#define DC_LCB_CFG_LINK_KILL_EN_RX_LESS_THAN_FOUR_LNS_SMASK           0x40ull
#define DC_LCB_CFG_LINK_KILL_EN_TX_LESS_THAN_FOUR_LNS_SHIFT           5
#define DC_LCB_CFG_LINK_KILL_EN_TX_LESS_THAN_FOUR_LNS_MASK            0x1ull
#define DC_LCB_CFG_LINK_KILL_EN_TX_LESS_THAN_FOUR_LNS_SMASK           0x20ull
#define DC_LCB_CFG_LINK_KILL_EN_LOST_REINIT_STALL_OR_TOS_SHIFT        4
#define DC_LCB_CFG_LINK_KILL_EN_LOST_REINIT_STALL_OR_TOS_MASK         0x1ull
#define DC_LCB_CFG_LINK_KILL_EN_LOST_REINIT_STALL_OR_TOS_SMASK        0x10ull
#define DC_LCB_CFG_LINK_KILL_EN_ALL_LNS_FAILED_REINIT_TEST_SHIFT      3
#define DC_LCB_CFG_LINK_KILL_EN_ALL_LNS_FAILED_REINIT_TEST_MASK       0x1ull
#define DC_LCB_CFG_LINK_KILL_EN_ALL_LNS_FAILED_REINIT_TEST_SMASK      0x8ull
#define DC_LCB_CFG_LINK_KILL_EN_RST_FOR_FAILED_DESKEW_SHIFT           2
#define DC_LCB_CFG_LINK_KILL_EN_RST_FOR_FAILED_DESKEW_MASK            0x1ull
#define DC_LCB_CFG_LINK_KILL_EN_RST_FOR_FAILED_DESKEW_SMASK           0x4ull
#define DC_LCB_CFG_LINK_KILL_EN_INVALID_CSR_ADDR_SHIFT                1
#define DC_LCB_CFG_LINK_KILL_EN_INVALID_CSR_ADDR_MASK                 0x1ull
#define DC_LCB_CFG_LINK_KILL_EN_INVALID_CSR_ADDR_SMASK                0x2ull
#define DC_LCB_CFG_LINK_KILL_EN_CSR_PARITY_ERR_SHIFT                  0
#define DC_LCB_CFG_LINK_KILL_EN_CSR_PARITY_ERR_MASK                   0x1ull
#define DC_LCB_CFG_LINK_KILL_EN_CSR_PARITY_ERR_SMASK                  0x1ull
/*
* Table #39 of 220_CSR_lcb.xml - LCB_CFG_ALLOW_LINK_UP
* The 8051 will set this when it receives a link transfer active back channel 
* idle message from the peer.
*/
#define DC_LCB_CFG_ALLOW_LINK_UP                                      (DC_LCB_CSRS + 0x000000000128)
#define DC_LCB_CFG_ALLOW_LINK_UP_RESETCSR                             0x0000000000000000ull
#define DC_LCB_CFG_ALLOW_LINK_UP_UNUSED_63_1_SHIFT                    1
#define DC_LCB_CFG_ALLOW_LINK_UP_UNUSED_63_1_MASK                     0x7FFFFFFFFFFFFFFFull
#define DC_LCB_CFG_ALLOW_LINK_UP_UNUSED_63_1_SMASK                    0xFFFFFFFFFFFFFFFEull
#define DC_LCB_CFG_ALLOW_LINK_UP_VAL_SHIFT                            0
#define DC_LCB_CFG_ALLOW_LINK_UP_VAL_MASK                             0x1ull
#define DC_LCB_CFG_ALLOW_LINK_UP_VAL_SMASK                            0x1ull
/*
* Table #40 of 220_CSR_lcb.xml - LCB_DBG_ERRINJ_CRC_0
* This register is used to provide CRC error injection stimulus for 
* debug.
*/
#define DC_LCB_DBG_ERRINJ_CRC_0                                       (DC_LCB_CSRS + 0x000000000200)
#define DC_LCB_DBG_ERRINJ_CRC_0_RESETCSR                              0x000001F3FFFFFFFFull
#define DC_LCB_DBG_ERRINJ_CRC_0_UNUSED_63_57_SHIFT                    57
#define DC_LCB_DBG_ERRINJ_CRC_0_UNUSED_63_57_MASK                     0x7Full
#define DC_LCB_DBG_ERRINJ_CRC_0_UNUSED_63_57_SMASK                    0xFE00000000000000ull
#define DC_LCB_DBG_ERRINJ_CRC_0_DISABLE_SEQUENTIAL_ERR_SHIFT          56
#define DC_LCB_DBG_ERRINJ_CRC_0_DISABLE_SEQUENTIAL_ERR_MASK           0x1ull
#define DC_LCB_DBG_ERRINJ_CRC_0_DISABLE_SEQUENTIAL_ERR_SMASK          0x100000000000000ull
#define DC_LCB_DBG_ERRINJ_CRC_0_UNUSED_55_SHIFT                       55
#define DC_LCB_DBG_ERRINJ_CRC_0_UNUSED_55_MASK                        0x1ull
#define DC_LCB_DBG_ERRINJ_CRC_0_UNUSED_55_SMASK                       0x80000000000000ull
#define DC_LCB_DBG_ERRINJ_CRC_0_CRC_ERR_PROBABILITY_SHIFT             52
#define DC_LCB_DBG_ERRINJ_CRC_0_CRC_ERR_PROBABILITY_MASK              0x7ull
#define DC_LCB_DBG_ERRINJ_CRC_0_CRC_ERR_PROBABILITY_SMASK             0x70000000000000ull
#define DC_LCB_DBG_ERRINJ_CRC_0_NUM_CRC_LTP_ERRS_SHIFT                48
#define DC_LCB_DBG_ERRINJ_CRC_0_NUM_CRC_LTP_ERRS_MASK                 0xFull
#define DC_LCB_DBG_ERRINJ_CRC_0_NUM_CRC_LTP_ERRS_SMASK                0xF000000000000ull
#define DC_LCB_DBG_ERRINJ_CRC_0_UNUSED_47_46_SHIFT                    46
#define DC_LCB_DBG_ERRINJ_CRC_0_UNUSED_47_46_MASK                     0x3ull
#define DC_LCB_DBG_ERRINJ_CRC_0_UNUSED_47_46_SMASK                    0xC00000000000ull
#define DC_LCB_DBG_ERRINJ_CRC_0_LTP_CNT_MAX_SELECT_SHIFT              44
#define DC_LCB_DBG_ERRINJ_CRC_0_LTP_CNT_MAX_SELECT_MASK               0x3ull
#define DC_LCB_DBG_ERRINJ_CRC_0_LTP_CNT_MAX_SELECT_SMASK              0x300000000000ull
#define DC_LCB_DBG_ERRINJ_CRC_0_UNUSED_43_41_SHIFT                    41
#define DC_LCB_DBG_ERRINJ_CRC_0_UNUSED_43_41_MASK                     0x7ull
#define DC_LCB_DBG_ERRINJ_CRC_0_UNUSED_43_41_SMASK                    0xE0000000000ull
#define DC_LCB_DBG_ERRINJ_CRC_0_RANDOM_XFR_ERR_SHIFT                  40
#define DC_LCB_DBG_ERRINJ_CRC_0_RANDOM_XFR_ERR_MASK                   0x1ull
#define DC_LCB_DBG_ERRINJ_CRC_0_RANDOM_XFR_ERR_SMASK                  0x10000000000ull
#define DC_LCB_DBG_ERRINJ_CRC_0_CRC_ERR_LN_EN_SHIFT                   36
#define DC_LCB_DBG_ERRINJ_CRC_0_CRC_ERR_LN_EN_MASK                    0xFull
#define DC_LCB_DBG_ERRINJ_CRC_0_CRC_ERR_LN_EN_SMASK                   0xF000000000ull
#define DC_LCB_DBG_ERRINJ_CRC_0_UNUSED_35_34_SHIFT                    34
#define DC_LCB_DBG_ERRINJ_CRC_0_UNUSED_35_34_MASK                     0x3ull
#define DC_LCB_DBG_ERRINJ_CRC_0_UNUSED_35_34_SMASK                    0xC00000000ull
#define DC_LCB_DBG_ERRINJ_CRC_0_CRC_ERR_XFR_EN_SHIFT                  0
#define DC_LCB_DBG_ERRINJ_CRC_0_CRC_ERR_XFR_EN_MASK                   0x3FFFFFFFFull
#define DC_LCB_DBG_ERRINJ_CRC_0_CRC_ERR_XFR_EN_SMASK                  0x3FFFFFFFFull
/*
* Table #41 of 220_CSR_lcb.xml - LCB_DBG_ERRINJ_CRC_1
* This register is used to provide CRC error injection stimulus for 
* debug.
*/
#define DC_LCB_DBG_ERRINJ_CRC_1                                       (DC_LCB_CSRS + 0x000000000208)
#define DC_LCB_DBG_ERRINJ_CRC_1_RESETCSR                              0x00000001FFFFFFFFull
#define DC_LCB_DBG_ERRINJ_CRC_1_UNUSED_63_44_SHIFT                    44
#define DC_LCB_DBG_ERRINJ_CRC_1_UNUSED_63_44_MASK                     0xFFFFFull
#define DC_LCB_DBG_ERRINJ_CRC_1_UNUSED_63_44_SMASK                    0xFFFFF00000000000ull
#define DC_LCB_DBG_ERRINJ_CRC_1_SEED_SHIFT                            32
#define DC_LCB_DBG_ERRINJ_CRC_1_SEED_MASK                             0xFFFull
#define DC_LCB_DBG_ERRINJ_CRC_1_SEED_SMASK                            0xFFF00000000ull
#define DC_LCB_DBG_ERRINJ_CRC_1_CRC_ERR_BIT_EN_SHIFT                  0
#define DC_LCB_DBG_ERRINJ_CRC_1_CRC_ERR_BIT_EN_MASK                   0xFFFFFFFFull
#define DC_LCB_DBG_ERRINJ_CRC_1_CRC_ERR_BIT_EN_SMASK                  0xFFFFFFFFull
/*
* Table #42 of 220_CSR_lcb.xml - LCB_DBG_ERRINJ_ECC
* This register is used to provide ECC error injection stimulus for 
* debug.
*/
#define DC_LCB_DBG_ERRINJ_ECC                                         (DC_LCB_CSRS + 0x000000000210)
#define DC_LCB_DBG_ERRINJ_ECC_RESETCSR                                0x0000000000000000ull
#define DC_LCB_DBG_ERRINJ_ECC_UNUSED_63_46_SHIFT                      46
#define DC_LCB_DBG_ERRINJ_ECC_UNUSED_63_46_MASK                       0x3FFFFull
#define DC_LCB_DBG_ERRINJ_ECC_UNUSED_63_46_SMASK                      0xFFFFC00000000000ull
#define DC_LCB_DBG_ERRINJ_ECC_CLR_ECC_ERR_CNTS_SHIFT                  45
#define DC_LCB_DBG_ERRINJ_ECC_CLR_ECC_ERR_CNTS_MASK                   0x1ull
#define DC_LCB_DBG_ERRINJ_ECC_CLR_ECC_ERR_CNTS_SMASK                  0x200000000000ull
#define DC_LCB_DBG_ERRINJ_ECC_REPLAY_BUF_ADDRESS_SHIFT                36
#define DC_LCB_DBG_ERRINJ_ECC_REPLAY_BUF_ADDRESS_MASK                 0x1FFull
#define DC_LCB_DBG_ERRINJ_ECC_REPLAY_BUF_ADDRESS_SMASK                0x1FF000000000ull
#define DC_LCB_DBG_ERRINJ_ECC_UNUSED_35_32_SHIFT                      32
#define DC_LCB_DBG_ERRINJ_ECC_UNUSED_35_32_MASK                       0xFull
#define DC_LCB_DBG_ERRINJ_ECC_UNUSED_35_32_SMASK                      0xF00000000ull
#define DC_LCB_DBG_ERRINJ_ECC_INPUT_BUF_ADDRESS_SHIFT                 28
#define DC_LCB_DBG_ERRINJ_ECC_INPUT_BUF_ADDRESS_MASK                  0xFull
#define DC_LCB_DBG_ERRINJ_ECC_INPUT_BUF_ADDRESS_SMASK                 0xF0000000ull
#define DC_LCB_DBG_ERRINJ_ECC_CHECKBYTE_SHIFT                         20
#define DC_LCB_DBG_ERRINJ_ECC_CHECKBYTE_MASK                          0xFFull
#define DC_LCB_DBG_ERRINJ_ECC_CHECKBYTE_SMASK                         0xFF00000ull
#define DC_LCB_DBG_ERRINJ_ECC_UNUSED_19_15_SHIFT                      15
#define DC_LCB_DBG_ERRINJ_ECC_UNUSED_19_15_MASK                       0x1Full
#define DC_LCB_DBG_ERRINJ_ECC_UNUSED_19_15_SMASK                      0xF8000ull
#define DC_LCB_DBG_ERRINJ_ECC_RAMSELECT_SHIFT                         8
#define DC_LCB_DBG_ERRINJ_ECC_RAMSELECT_MASK                          0x7Full
#define DC_LCB_DBG_ERRINJ_ECC_RAMSELECT_SMASK                         0x7F00ull
#define DC_LCB_DBG_ERRINJ_ECC_UNUSED_7_6_SHIFT                        6
#define DC_LCB_DBG_ERRINJ_ECC_UNUSED_7_6_MASK                         0x3ull
#define DC_LCB_DBG_ERRINJ_ECC_UNUSED_7_6_SMASK                        0xC0ull
#define DC_LCB_DBG_ERRINJ_ECC_MODE_SHIFT                              4
#define DC_LCB_DBG_ERRINJ_ECC_MODE_MASK                               0x3ull
#define DC_LCB_DBG_ERRINJ_ECC_MODE_SMASK                              0x30ull
#define DC_LCB_DBG_ERRINJ_ECC_UNUSED_3_1_SHIFT                        1
#define DC_LCB_DBG_ERRINJ_ECC_UNUSED_3_1_MASK                         0x7ull
#define DC_LCB_DBG_ERRINJ_ECC_UNUSED_3_1_SMASK                        0xEull
#define DC_LCB_DBG_ERRINJ_ECC_EN_SHIFT                                0
#define DC_LCB_DBG_ERRINJ_ECC_EN_MASK                                 0x1ull
#define DC_LCB_DBG_ERRINJ_ECC_EN_SMASK                                0x1ull
/*
* Table #43 of 220_CSR_lcb.xml - LCB_DBG_ERRINJ_MISC
* This register is used to provide miscellaneous error injection stimulus for 
* debug.
*/
#define DC_LCB_DBG_ERRINJ_MISC                                        (DC_LCB_CSRS + 0x000000000218)
#define DC_LCB_DBG_ERRINJ_MISC_RESETCSR                               0x0000000000000000ull
#define DC_LCB_DBG_ERRINJ_MISC_UNUSED_63_21_SHIFT                     21
#define DC_LCB_DBG_ERRINJ_MISC_UNUSED_63_21_MASK                      0x7FFFFFFFFFFull
#define DC_LCB_DBG_ERRINJ_MISC_UNUSED_63_21_SMASK                     0xFFFFFFFFFFE00000ull
#define DC_LCB_DBG_ERRINJ_MISC_FORCE_VL_ACK_INPUT_PBE_SHIFT           20
#define DC_LCB_DBG_ERRINJ_MISC_FORCE_VL_ACK_INPUT_PBE_MASK            0x1ull
#define DC_LCB_DBG_ERRINJ_MISC_FORCE_VL_ACK_INPUT_PBE_SMASK           0x100000ull
#define DC_LCB_DBG_ERRINJ_MISC_UNUSED_19_17_SHIFT                     17
#define DC_LCB_DBG_ERRINJ_MISC_UNUSED_19_17_MASK                      0x7ull
#define DC_LCB_DBG_ERRINJ_MISC_UNUSED_19_17_SMASK                     0xE0000ull
#define DC_LCB_DBG_ERRINJ_MISC_FORCE_REPLAY_SHIFT                     16
#define DC_LCB_DBG_ERRINJ_MISC_FORCE_REPLAY_MASK                      0x1ull
#define DC_LCB_DBG_ERRINJ_MISC_FORCE_REPLAY_SMASK                     0x10000ull
#define DC_LCB_DBG_ERRINJ_MISC_UNUSED_15_13_SHIFT                     13
#define DC_LCB_DBG_ERRINJ_MISC_UNUSED_15_13_MASK                      0x7ull
#define DC_LCB_DBG_ERRINJ_MISC_UNUSED_15_13_SMASK                     0xE000ull
#define DC_LCB_DBG_ERRINJ_MISC_FORCE_BAD_CRC_REINIT_SHIFT             12
#define DC_LCB_DBG_ERRINJ_MISC_FORCE_BAD_CRC_REINIT_MASK              0x1ull
#define DC_LCB_DBG_ERRINJ_MISC_FORCE_BAD_CRC_REINIT_SMASK             0x1000ull
#define DC_LCB_DBG_ERRINJ_MISC_UNUSED_11_9_SHIFT                      9
#define DC_LCB_DBG_ERRINJ_MISC_UNUSED_11_9_MASK                       0x7ull
#define DC_LCB_DBG_ERRINJ_MISC_UNUSED_11_9_SMASK                      0xE00ull
#define DC_LCB_DBG_ERRINJ_MISC_FORCE_LOST_FLIT_ACK_SHIFT              8
#define DC_LCB_DBG_ERRINJ_MISC_FORCE_LOST_FLIT_ACK_MASK               0x1ull
#define DC_LCB_DBG_ERRINJ_MISC_FORCE_LOST_FLIT_ACK_SMASK              0x100ull
#define DC_LCB_DBG_ERRINJ_MISC_UNUSED_7_5_SHIFT                       5
#define DC_LCB_DBG_ERRINJ_MISC_UNUSED_7_5_MASK                        0x7ull
#define DC_LCB_DBG_ERRINJ_MISC_UNUSED_7_5_SMASK                       0xE0ull
#define DC_LCB_DBG_ERRINJ_MISC_FORCE_EXTRA_FLIT_ACK_SHIFT             4
#define DC_LCB_DBG_ERRINJ_MISC_FORCE_EXTRA_FLIT_ACK_MASK              0x1ull
#define DC_LCB_DBG_ERRINJ_MISC_FORCE_EXTRA_FLIT_ACK_SMASK             0x10ull
#define DC_LCB_DBG_ERRINJ_MISC_UNUSED_3_1_SHIFT                       1
#define DC_LCB_DBG_ERRINJ_MISC_UNUSED_3_1_MASK                        0x7ull
#define DC_LCB_DBG_ERRINJ_MISC_UNUSED_3_1_SMASK                       0xEull
#define DC_LCB_DBG_ERRINJ_MISC_FORCE_MISCOMPARE_SHIFT                 0
#define DC_LCB_DBG_ERRINJ_MISC_FORCE_MISCOMPARE_MASK                  0x1ull
#define DC_LCB_DBG_ERRINJ_MISC_FORCE_MISCOMPARE_SMASK                 0x1ull
/*
* Table #44 of 220_CSR_lcb.xml - LCB_DBG_INIT_STATE_0
* This register is used to report various initialization status 
* indications.
*/
#define DC_LCB_DBG_INIT_STATE_0                                       (DC_LCB_CSRS + 0x000000000240)
#define DC_LCB_DBG_INIT_STATE_0_RESETCSR                              0x0000000000000000ull
#define DC_LCB_DBG_INIT_STATE_0_UNUSED_63_52_SHIFT                    52
#define DC_LCB_DBG_INIT_STATE_0_UNUSED_63_52_MASK                     0xFFFull
#define DC_LCB_DBG_INIT_STATE_0_UNUSED_63_52_SMASK                    0xFFF0000000000000ull
#define DC_LCB_DBG_INIT_STATE_0_UNUSED_51_SHIFT                       51
#define DC_LCB_DBG_INIT_STATE_0_UNUSED_51_MASK                        0x1ull
#define DC_LCB_DBG_INIT_STATE_0_UNUSED_51_SMASK                       0x8000000000000ull
#define DC_LCB_DBG_INIT_STATE_0_FINE_ALIGN_PTR3_SHIFT                 48
#define DC_LCB_DBG_INIT_STATE_0_FINE_ALIGN_PTR3_MASK                  0x7ull
#define DC_LCB_DBG_INIT_STATE_0_FINE_ALIGN_PTR3_SMASK                 0x7000000000000ull
#define DC_LCB_DBG_INIT_STATE_0_UNUSED_47_46_SHIFT                    46
#define DC_LCB_DBG_INIT_STATE_0_UNUSED_47_46_MASK                     0x3ull
#define DC_LCB_DBG_INIT_STATE_0_UNUSED_47_46_SMASK                    0xC00000000000ull
#define DC_LCB_DBG_INIT_STATE_0_CORSE_ALIGN_PTR3_SHIFT                44
#define DC_LCB_DBG_INIT_STATE_0_CORSE_ALIGN_PTR3_MASK                 0x3ull
#define DC_LCB_DBG_INIT_STATE_0_CORSE_ALIGN_PTR3_SMASK                0x300000000000ull
#define DC_LCB_DBG_INIT_STATE_0_UNUSED_43_SHIFT                       43
#define DC_LCB_DBG_INIT_STATE_0_UNUSED_43_MASK                        0x1ull
#define DC_LCB_DBG_INIT_STATE_0_UNUSED_43_SMASK                       0x80000000000ull
#define DC_LCB_DBG_INIT_STATE_0_FINE_ALIGN_PTR2_SHIFT                 40
#define DC_LCB_DBG_INIT_STATE_0_FINE_ALIGN_PTR2_MASK                  0x7ull
#define DC_LCB_DBG_INIT_STATE_0_FINE_ALIGN_PTR2_SMASK                 0x70000000000ull
#define DC_LCB_DBG_INIT_STATE_0_UNUSED_39_38_SHIFT                    38
#define DC_LCB_DBG_INIT_STATE_0_UNUSED_39_38_MASK                     0x3ull
#define DC_LCB_DBG_INIT_STATE_0_UNUSED_39_38_SMASK                    0xC000000000ull
#define DC_LCB_DBG_INIT_STATE_0_CORSE_ALIGN_PTR2_SHIFT                36
#define DC_LCB_DBG_INIT_STATE_0_CORSE_ALIGN_PTR2_MASK                 0x3ull
#define DC_LCB_DBG_INIT_STATE_0_CORSE_ALIGN_PTR2_SMASK                0x3000000000ull
#define DC_LCB_DBG_INIT_STATE_0_UNUSED_35_SHIFT                       35
#define DC_LCB_DBG_INIT_STATE_0_UNUSED_35_MASK                        0x1ull
#define DC_LCB_DBG_INIT_STATE_0_UNUSED_35_SMASK                       0x800000000ull
#define DC_LCB_DBG_INIT_STATE_0_FINE_ALIGN_PTR1_SHIFT                 32
#define DC_LCB_DBG_INIT_STATE_0_FINE_ALIGN_PTR1_MASK                  0x7ull
#define DC_LCB_DBG_INIT_STATE_0_FINE_ALIGN_PTR1_SMASK                 0x700000000ull
#define DC_LCB_DBG_INIT_STATE_0_UNUSED_31_30_SHIFT                    30
#define DC_LCB_DBG_INIT_STATE_0_UNUSED_31_30_MASK                     0x3ull
#define DC_LCB_DBG_INIT_STATE_0_UNUSED_31_30_SMASK                    0xC0000000ull
#define DC_LCB_DBG_INIT_STATE_0_CORSE_ALIGN_PTR1_SHIFT                28
#define DC_LCB_DBG_INIT_STATE_0_CORSE_ALIGN_PTR1_MASK                 0x3ull
#define DC_LCB_DBG_INIT_STATE_0_CORSE_ALIGN_PTR1_SMASK                0x30000000ull
#define DC_LCB_DBG_INIT_STATE_0_UNUSED_27_SHIFT                       27
#define DC_LCB_DBG_INIT_STATE_0_UNUSED_27_MASK                        0x1ull
#define DC_LCB_DBG_INIT_STATE_0_UNUSED_27_SMASK                       0x8000000ull
#define DC_LCB_DBG_INIT_STATE_0_FINE_ALIGN_PTR0_SHIFT                 24
#define DC_LCB_DBG_INIT_STATE_0_FINE_ALIGN_PTR0_MASK                  0x7ull
#define DC_LCB_DBG_INIT_STATE_0_FINE_ALIGN_PTR0_SMASK                 0x7000000ull
#define DC_LCB_DBG_INIT_STATE_0_UNUSED_23_22_SHIFT                    22
#define DC_LCB_DBG_INIT_STATE_0_UNUSED_23_22_MASK                     0x3ull
#define DC_LCB_DBG_INIT_STATE_0_UNUSED_23_22_SMASK                    0xC00000ull
#define DC_LCB_DBG_INIT_STATE_0_CORSE_ALIGN_PTR0_SHIFT                20
#define DC_LCB_DBG_INIT_STATE_0_CORSE_ALIGN_PTR0_MASK                 0x3ull
#define DC_LCB_DBG_INIT_STATE_0_CORSE_ALIGN_PTR0_SMASK                0x300000ull
#define DC_LCB_DBG_INIT_STATE_0_FRAMING1_COMPLETE_SHIFT               16
#define DC_LCB_DBG_INIT_STATE_0_FRAMING1_COMPLETE_MASK                0xFull
#define DC_LCB_DBG_INIT_STATE_0_FRAMING1_COMPLETE_SMASK               0xF0000ull
#define DC_LCB_DBG_INIT_STATE_0_LOGICAL_ID_COMPLETE_SHIFT             12
#define DC_LCB_DBG_INIT_STATE_0_LOGICAL_ID_COMPLETE_MASK              0xFull
#define DC_LCB_DBG_INIT_STATE_0_LOGICAL_ID_COMPLETE_SMASK             0xF000ull
#define DC_LCB_DBG_INIT_STATE_0_POLARITY_SHIFT                        8
#define DC_LCB_DBG_INIT_STATE_0_POLARITY_MASK                         0xFull
#define DC_LCB_DBG_INIT_STATE_0_POLARITY_SMASK                        0xF00ull
#define DC_LCB_DBG_INIT_STATE_0_POLARITY_COMPLETE_SHIFT               4
#define DC_LCB_DBG_INIT_STATE_0_POLARITY_COMPLETE_MASK                0xFull
#define DC_LCB_DBG_INIT_STATE_0_POLARITY_COMPLETE_SMASK               0xF0ull
#define DC_LCB_DBG_INIT_STATE_0_ALIGN_COMPLETE_SHIFT                  0
#define DC_LCB_DBG_INIT_STATE_0_ALIGN_COMPLETE_MASK                   0xFull
#define DC_LCB_DBG_INIT_STATE_0_ALIGN_COMPLETE_SMASK                  0xFull
/*
* Table #45 of 220_CSR_lcb.xml - LCB_DBG_INIT_STATE_1
* This register is used to report various initialization status 
* indications.
*/
#define DC_LCB_DBG_INIT_STATE_1                                       (DC_LCB_CSRS + 0x000000000248)
#define DC_LCB_DBG_INIT_STATE_1_RESETCSR                              0x0000000000000000ull
#define DC_LCB_DBG_INIT_STATE_1_UNUSED_63_57_SHIFT                    57
#define DC_LCB_DBG_INIT_STATE_1_UNUSED_63_57_MASK                     0x7Full
#define DC_LCB_DBG_INIT_STATE_1_UNUSED_63_57_SMASK                    0xFE00000000000000ull
#define DC_LCB_DBG_INIT_STATE_1_LN_TEST_TIMER_COMPLETE_SHIFT          56
#define DC_LCB_DBG_INIT_STATE_1_LN_TEST_TIMER_COMPLETE_MASK           0x1ull
#define DC_LCB_DBG_INIT_STATE_1_LN_TEST_TIMER_COMPLETE_SMASK          0x100000000000000ull
#define DC_LCB_DBG_INIT_STATE_1_LN_PASSED_TESTING_SHIFT               52
#define DC_LCB_DBG_INIT_STATE_1_LN_PASSED_TESTING_MASK                0xFull
#define DC_LCB_DBG_INIT_STATE_1_LN_PASSED_TESTING_SMASK               0xF0000000000000ull
#define DC_LCB_DBG_INIT_STATE_1_UNUSED_51_49_SHIFT                    49
#define DC_LCB_DBG_INIT_STATE_1_UNUSED_51_49_MASK                     0x7ull
#define DC_LCB_DBG_INIT_STATE_1_UNUSED_51_49_SMASK                    0xE000000000000ull
#define DC_LCB_DBG_INIT_STATE_1_PASS_CNT_LN3_SHIFT                    40
#define DC_LCB_DBG_INIT_STATE_1_PASS_CNT_LN3_MASK                     0x1FFull
#define DC_LCB_DBG_INIT_STATE_1_PASS_CNT_LN3_SMASK                    0x1FF0000000000ull
#define DC_LCB_DBG_INIT_STATE_1_UNUSED_39_37_SHIFT                    37
#define DC_LCB_DBG_INIT_STATE_1_UNUSED_39_37_MASK                     0x7ull
#define DC_LCB_DBG_INIT_STATE_1_UNUSED_39_37_SMASK                    0xE000000000ull
#define DC_LCB_DBG_INIT_STATE_1_PASS_CNT_LN2_SHIFT                    28
#define DC_LCB_DBG_INIT_STATE_1_PASS_CNT_LN2_MASK                     0x1FFull
#define DC_LCB_DBG_INIT_STATE_1_PASS_CNT_LN2_SMASK                    0x1FF0000000ull
#define DC_LCB_DBG_INIT_STATE_1_UNUSED_27_25_SHIFT                    25
#define DC_LCB_DBG_INIT_STATE_1_UNUSED_27_25_MASK                     0x7ull
#define DC_LCB_DBG_INIT_STATE_1_UNUSED_27_25_SMASK                    0xE000000ull
#define DC_LCB_DBG_INIT_STATE_1_PASS_CNT_LN1_SHIFT                    16
#define DC_LCB_DBG_INIT_STATE_1_PASS_CNT_LN1_MASK                     0x1FFull
#define DC_LCB_DBG_INIT_STATE_1_PASS_CNT_LN1_SMASK                    0x1FF0000ull
#define DC_LCB_DBG_INIT_STATE_1_UNUSED_15_13_SHIFT                    13
#define DC_LCB_DBG_INIT_STATE_1_UNUSED_15_13_MASK                     0x7ull
#define DC_LCB_DBG_INIT_STATE_1_UNUSED_15_13_SMASK                    0xE000ull
#define DC_LCB_DBG_INIT_STATE_1_PASS_CNT_LN0_SHIFT                    4
#define DC_LCB_DBG_INIT_STATE_1_PASS_CNT_LN0_MASK                     0x1FFull
#define DC_LCB_DBG_INIT_STATE_1_PASS_CNT_LN0_SMASK                    0x1FF0ull
#define DC_LCB_DBG_INIT_STATE_1_LN_TESTING_COMPLETE_SHIFT             0
#define DC_LCB_DBG_INIT_STATE_1_LN_TESTING_COMPLETE_MASK              0xFull
#define DC_LCB_DBG_INIT_STATE_1_LN_TESTING_COMPLETE_SMASK             0xFull
/*
* Table #46 of 220_CSR_lcb.xml - LCB_DBG_INIT_STATE_2
* This register is used to report various initialization status 
* indications.
*/
#define DC_LCB_DBG_INIT_STATE_2                                       (DC_LCB_CSRS + 0x000000000250)
#define DC_LCB_DBG_INIT_STATE_2_RESETCSR                              0x0000000000000000ull
#define DC_LCB_DBG_INIT_STATE_2_UNUSED_63_49_SHIFT                    49
#define DC_LCB_DBG_INIT_STATE_2_UNUSED_63_49_MASK                     0x7FFFull
#define DC_LCB_DBG_INIT_STATE_2_UNUSED_63_49_SMASK                    0xFFFE000000000000ull
#define DC_LCB_DBG_INIT_STATE_2_TX_TON_SIG_SHIFT                      48
#define DC_LCB_DBG_INIT_STATE_2_TX_TON_SIG_MASK                       0x1ull
#define DC_LCB_DBG_INIT_STATE_2_TX_TON_SIG_SMASK                      0x1000000000000ull
#define DC_LCB_DBG_INIT_STATE_2_UNUSED_47_45_SHIFT                    45
#define DC_LCB_DBG_INIT_STATE_2_UNUSED_47_45_MASK                     0x7ull
#define DC_LCB_DBG_INIT_STATE_2_UNUSED_47_45_SMASK                    0xE00000000000ull
#define DC_LCB_DBG_INIT_STATE_2_PEER_LN_EN_DETECTED_SHIFT             44
#define DC_LCB_DBG_INIT_STATE_2_PEER_LN_EN_DETECTED_MASK              0x1ull
#define DC_LCB_DBG_INIT_STATE_2_PEER_LN_EN_DETECTED_SMASK             0x100000000000ull
#define DC_LCB_DBG_INIT_STATE_2_UNUSED_43_42_SHIFT                    42
#define DC_LCB_DBG_INIT_STATE_2_UNUSED_43_42_MASK                     0x3ull
#define DC_LCB_DBG_INIT_STATE_2_UNUSED_43_42_SMASK                    0xC0000000000ull
#define DC_LCB_DBG_INIT_STATE_2_RCLK_SAMPLING_LN_SHIFT                40
#define DC_LCB_DBG_INIT_STATE_2_RCLK_SAMPLING_LN_MASK                 0x3ull
#define DC_LCB_DBG_INIT_STATE_2_RCLK_SAMPLING_LN_SMASK                0x30000000000ull
#define DC_LCB_DBG_INIT_STATE_2_UNUSED_39_38_SHIFT                    38
#define DC_LCB_DBG_INIT_STATE_2_UNUSED_39_38_MASK                     0x3ull
#define DC_LCB_DBG_INIT_STATE_2_UNUSED_39_38_SMASK                    0xC000000000ull
#define DC_LCB_DBG_INIT_STATE_2_BACK_CHAN_REPORTING_LN_SHIFT          36
#define DC_LCB_DBG_INIT_STATE_2_BACK_CHAN_REPORTING_LN_MASK           0x3ull
#define DC_LCB_DBG_INIT_STATE_2_BACK_CHAN_REPORTING_LN_SMASK          0x3000000000ull
#define DC_LCB_DBG_INIT_STATE_2_RADR_SKIP4DESKEW_CNT_LN3_SHIFT        32
#define DC_LCB_DBG_INIT_STATE_2_RADR_SKIP4DESKEW_CNT_LN3_MASK         0xFull
#define DC_LCB_DBG_INIT_STATE_2_RADR_SKIP4DESKEW_CNT_LN3_SMASK        0xF00000000ull
#define DC_LCB_DBG_INIT_STATE_2_RADR_SKIP4DESKEW_CNT_LN2_SHIFT        28
#define DC_LCB_DBG_INIT_STATE_2_RADR_SKIP4DESKEW_CNT_LN2_MASK         0xFull
#define DC_LCB_DBG_INIT_STATE_2_RADR_SKIP4DESKEW_CNT_LN2_SMASK        0xF0000000ull
#define DC_LCB_DBG_INIT_STATE_2_RADR_SKIP4DESKEW_CNT_LN1_SHIFT        24
#define DC_LCB_DBG_INIT_STATE_2_RADR_SKIP4DESKEW_CNT_LN1_MASK         0xFull
#define DC_LCB_DBG_INIT_STATE_2_RADR_SKIP4DESKEW_CNT_LN1_SMASK        0xF000000ull
#define DC_LCB_DBG_INIT_STATE_2_RADR_SKIP4DESKEW_CNT_LN0_SHIFT        20
#define DC_LCB_DBG_INIT_STATE_2_RADR_SKIP4DESKEW_CNT_LN0_MASK         0xFull
#define DC_LCB_DBG_INIT_STATE_2_RADR_SKIP4DESKEW_CNT_LN0_SMASK        0xF00000ull
#define DC_LCB_DBG_INIT_STATE_2_UNUSED_19_SHIFT                       19
#define DC_LCB_DBG_INIT_STATE_2_UNUSED_19_MASK                        0x1ull
#define DC_LCB_DBG_INIT_STATE_2_UNUSED_19_SMASK                       0x80000ull
#define DC_LCB_DBG_INIT_STATE_2_FAILED_TEST_CNT_LN3_SHIFT             16
#define DC_LCB_DBG_INIT_STATE_2_FAILED_TEST_CNT_LN3_MASK              0x7ull
#define DC_LCB_DBG_INIT_STATE_2_FAILED_TEST_CNT_LN3_SMASK             0x70000ull
#define DC_LCB_DBG_INIT_STATE_2_UNUSED_15_SHIFT                       15
#define DC_LCB_DBG_INIT_STATE_2_UNUSED_15_MASK                        0x1ull
#define DC_LCB_DBG_INIT_STATE_2_UNUSED_15_SMASK                       0x8000ull
#define DC_LCB_DBG_INIT_STATE_2_FAILED_TEST_CNT_LN2_SHIFT             12
#define DC_LCB_DBG_INIT_STATE_2_FAILED_TEST_CNT_LN2_MASK              0x7ull
#define DC_LCB_DBG_INIT_STATE_2_FAILED_TEST_CNT_LN2_SMASK             0x7000ull
#define DC_LCB_DBG_INIT_STATE_2_UNUSED_11_SHIFT                       11
#define DC_LCB_DBG_INIT_STATE_2_UNUSED_11_MASK                        0x1ull
#define DC_LCB_DBG_INIT_STATE_2_UNUSED_11_SMASK                       0x800ull
#define DC_LCB_DBG_INIT_STATE_2_FAILED_TEST_CNT_LN1_SHIFT             8
#define DC_LCB_DBG_INIT_STATE_2_FAILED_TEST_CNT_LN1_MASK              0x7ull
#define DC_LCB_DBG_INIT_STATE_2_FAILED_TEST_CNT_LN1_SMASK             0x700ull
#define DC_LCB_DBG_INIT_STATE_2_UNUSED_7_SHIFT                        7
#define DC_LCB_DBG_INIT_STATE_2_UNUSED_7_MASK                         0x1ull
#define DC_LCB_DBG_INIT_STATE_2_UNUSED_7_SMASK                        0x80ull
#define DC_LCB_DBG_INIT_STATE_2_FAILED_TEST_CNT_LN0_SHIFT             4
#define DC_LCB_DBG_INIT_STATE_2_FAILED_TEST_CNT_LN0_MASK              0x7ull
#define DC_LCB_DBG_INIT_STATE_2_FAILED_TEST_CNT_LN0_SMASK             0x70ull
#define DC_LCB_DBG_INIT_STATE_2_UNUSED_3_1_SHIFT                      1
#define DC_LCB_DBG_INIT_STATE_2_UNUSED_3_1_MASK                       0x7ull
#define DC_LCB_DBG_INIT_STATE_2_UNUSED_3_1_SMASK                      0xEull
#define DC_LCB_DBG_INIT_STATE_2_DESKEW_COMPLETE_SHIFT                 0
#define DC_LCB_DBG_INIT_STATE_2_DESKEW_COMPLETE_MASK                  0x1ull
#define DC_LCB_DBG_INIT_STATE_2_DESKEW_COMPLETE_SMASK                 0x1ull
/*
* Table #47 of 220_CSR_lcb.xml - LCB_DBG_CFG_GOOD_BAD_DATA
* This register is used to control the good, bad and scrambled LTP 
* data.
*/
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA                                  (DC_LCB_CSRS + 0x000000000258)
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_RESETCSR                         0x00000000F0000000ull
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_UNUSED_63_37_SHIFT               37
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_UNUSED_63_37_MASK                0x7FFFFFFull
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_UNUSED_63_37_SMASK               0xFFFFFFE000000000ull
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_RAW_REQUIRED_ERR_CNT_SHIFT       32
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_RAW_REQUIRED_ERR_CNT_MASK        0x1Full
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_RAW_REQUIRED_ERR_CNT_SMASK       0x1F00000000ull
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_SHIFTED_LN_ERR_CNT_EN_SHIFT      28
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_SHIFTED_LN_ERR_CNT_EN_MASK       0xFull
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_SHIFTED_LN_ERR_CNT_EN_SMASK      0xF0000000ull
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_UNUSED_27_25_SHIFT               25
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_UNUSED_27_25_MASK                0x7ull
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_UNUSED_27_25_SMASK               0xE000000ull
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_ERR_STAT_MODE_SHIFT              24
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_ERR_STAT_MODE_MASK               0x1ull
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_ERR_STAT_MODE_SMASK              0x1000000ull
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_UNUSED_23_21_SHIFT               21
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_UNUSED_23_21_MASK                0x7ull
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_UNUSED_23_21_SMASK               0xE00000ull
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_SAVE_ALL_SHIFT                   20
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_SAVE_ALL_MASK                    0x1ull
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_SAVE_ALL_SMASK                   0x100000ull
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_UNUSED_19_SHIFT                  19
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_UNUSED_19_MASK                   0x1ull
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_UNUSED_19_SMASK                  0x80000ull
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_SAVE_ESCAPE_ONLY_SHIFT           16
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_SAVE_ESCAPE_ONLY_MASK            0x7ull
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_SAVE_ESCAPE_ONLY_SMASK           0x70000ull
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_UNUSED_15_13_SHIFT               13
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_UNUSED_15_13_MASK                0x7ull
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_UNUSED_15_13_SMASK               0xE000ull
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_SAVE_MULTIPLE_ONLY_SHIFT         12
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_SAVE_MULTIPLE_ONLY_MASK          0x1ull
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_SAVE_MULTIPLE_ONLY_SMASK         0x1000ull
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_ADDR_SHIFT                       8
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_ADDR_MASK                        0xFull
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_ADDR_SMASK                       0xF00ull
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_UNUSED_7_6_SHIFT                 6
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_UNUSED_7_6_MASK                  0x3ull
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_UNUSED_7_6_SMASK                 0xC0ull
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_SELECT_SHIFT                     4
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_SELECT_MASK                      0x3ull
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_SELECT_SMASK                     0x30ull
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_UNUSED_3_1_SHIFT                 1
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_UNUSED_3_1_MASK                  0x7ull
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_UNUSED_3_1_SMASK                 0xEull
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_ARM_SHIFT                        0
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_ARM_MASK                         0x1ull
#define DC_LCB_DBG_CFG_GOOD_BAD_DATA_ARM_SMASK                        0x1ull
/*
* Table #48 of 220_CSR_lcb.xml - LCB_DBG_STS_GOOD_BAD_DATA
* This register is used to observe status associated with the good, bad and 
* scrambled LTP data.
*/
#define DC_LCB_DBG_STS_GOOD_BAD_DATA                                  (DC_LCB_CSRS + 0x000000000260)
#define DC_LCB_DBG_STS_GOOD_BAD_DATA_RESETCSR                         0x0000000000000000ull
#define DC_LCB_DBG_STS_GOOD_BAD_DATA_UNUSED_63_11_SHIFT               11
#define DC_LCB_DBG_STS_GOOD_BAD_DATA_UNUSED_63_11_MASK                0x1FFFFFFFFFFFFFull
#define DC_LCB_DBG_STS_GOOD_BAD_DATA_UNUSED_63_11_SMASK               0xFFFFFFFFFFFFF800ull
#define DC_LCB_DBG_STS_GOOD_BAD_DATA_LTP_CNT_BAD_SHIFT                8
#define DC_LCB_DBG_STS_GOOD_BAD_DATA_LTP_CNT_BAD_MASK                 0x7ull
#define DC_LCB_DBG_STS_GOOD_BAD_DATA_LTP_CNT_BAD_SMASK                0x700ull
#define DC_LCB_DBG_STS_GOOD_BAD_DATA_UNUSED_7_SHIFT                   7
#define DC_LCB_DBG_STS_GOOD_BAD_DATA_UNUSED_7_MASK                    0x1ull
#define DC_LCB_DBG_STS_GOOD_BAD_DATA_UNUSED_7_SMASK                   0x80ull
#define DC_LCB_DBG_STS_GOOD_BAD_DATA_LTP_CNT_GOOD_SHIFT               4
#define DC_LCB_DBG_STS_GOOD_BAD_DATA_LTP_CNT_GOOD_MASK                0x7ull
#define DC_LCB_DBG_STS_GOOD_BAD_DATA_LTP_CNT_GOOD_SMASK               0x70ull
#define DC_LCB_DBG_STS_GOOD_BAD_DATA_UNUSED_3_1_SHIFT                 1
#define DC_LCB_DBG_STS_GOOD_BAD_DATA_UNUSED_3_1_MASK                  0x7ull
#define DC_LCB_DBG_STS_GOOD_BAD_DATA_UNUSED_3_1_SMASK                 0xEull
#define DC_LCB_DBG_STS_GOOD_BAD_DATA_READY_SHIFT                      0
#define DC_LCB_DBG_STS_GOOD_BAD_DATA_READY_MASK                       0x1ull
#define DC_LCB_DBG_STS_GOOD_BAD_DATA_READY_SMASK                      0x1ull
/*
* Table #49 of 220_CSR_lcb.xml - LCB_DBG_STS_GOOD_BAD_DAT0
* This register is used to observe the good, bad and scrambled LTP 
* data.
*/
#define DC_LCB_DBG_STS_GOOD_BAD_DAT0                                  (DC_LCB_CSRS + 0x000000000268)
#define DC_LCB_DBG_STS_GOOD_BAD_DAT0_RESETCSR                         0x0000000000000000ull
#define DC_LCB_DBG_STS_GOOD_BAD_DAT0_UNUSED_63_32_SHIFT               32
#define DC_LCB_DBG_STS_GOOD_BAD_DAT0_UNUSED_63_32_MASK                0xFFFFFFFFull
#define DC_LCB_DBG_STS_GOOD_BAD_DAT0_UNUSED_63_32_SMASK               0xFFFFFFFF00000000ull
#define DC_LCB_DBG_STS_GOOD_BAD_DAT0_VAL_SHIFT                        0
#define DC_LCB_DBG_STS_GOOD_BAD_DAT0_VAL_MASK                         0xFFFFFFFFull
#define DC_LCB_DBG_STS_GOOD_BAD_DAT0_VAL_SMASK                        0xFFFFFFFFull
/*
* Table #50 of 220_CSR_lcb.xml - LCB_DBG_STS_GOOD_BAD_DAT1
* This register is used to observe the good, bad and scrambled LTP 
* data.
*/
#define DC_LCB_DBG_STS_GOOD_BAD_DAT1                                  (DC_LCB_CSRS + 0x000000000270)
#define DC_LCB_DBG_STS_GOOD_BAD_DAT1_RESETCSR                         0x0000000000000000ull
#define DC_LCB_DBG_STS_GOOD_BAD_DAT1_UNUSED_63_32_SHIFT               32
#define DC_LCB_DBG_STS_GOOD_BAD_DAT1_UNUSED_63_32_MASK                0xFFFFFFFFull
#define DC_LCB_DBG_STS_GOOD_BAD_DAT1_UNUSED_63_32_SMASK               0xFFFFFFFF00000000ull
#define DC_LCB_DBG_STS_GOOD_BAD_DAT1_VAL_SHIFT                        0
#define DC_LCB_DBG_STS_GOOD_BAD_DAT1_VAL_MASK                         0xFFFFFFFFull
#define DC_LCB_DBG_STS_GOOD_BAD_DAT1_VAL_SMASK                        0xFFFFFFFFull
/*
* Table #51 of 220_CSR_lcb.xml - LCB_DBG_STS_GOOD_BAD_DAT2
* This register is used to observe the good, bad and scrambled LTP 
* data.
*/
#define DC_LCB_DBG_STS_GOOD_BAD_DAT2                                  (DC_LCB_CSRS + 0x000000000278)
#define DC_LCB_DBG_STS_GOOD_BAD_DAT2_RESETCSR                         0x0000000000000000ull
#define DC_LCB_DBG_STS_GOOD_BAD_DAT2_UNUSED_63_32_SHIFT               32
#define DC_LCB_DBG_STS_GOOD_BAD_DAT2_UNUSED_63_32_MASK                0xFFFFFFFFull
#define DC_LCB_DBG_STS_GOOD_BAD_DAT2_UNUSED_63_32_SMASK               0xFFFFFFFF00000000ull
#define DC_LCB_DBG_STS_GOOD_BAD_DAT2_VAL_SHIFT                        0
#define DC_LCB_DBG_STS_GOOD_BAD_DAT2_VAL_MASK                         0xFFFFFFFFull
#define DC_LCB_DBG_STS_GOOD_BAD_DAT2_VAL_SMASK                        0xFFFFFFFFull
/*
* Table #52 of 220_CSR_lcb.xml - LCB_DBG_STS_GOOD_BAD_DAT3
* This register is used to observe the good, bad and scrambled LTP 
* data.
*/
#define DC_LCB_DBG_STS_GOOD_BAD_DAT3                                  (DC_LCB_CSRS + 0x000000000280)
#define DC_LCB_DBG_STS_GOOD_BAD_DAT3_RESETCSR                         0x0000000000000000ull
#define DC_LCB_DBG_STS_GOOD_BAD_DAT3_UNUSED_63_32_SHIFT               32
#define DC_LCB_DBG_STS_GOOD_BAD_DAT3_UNUSED_63_32_MASK                0xFFFFFFFFull
#define DC_LCB_DBG_STS_GOOD_BAD_DAT3_UNUSED_63_32_SMASK               0xFFFFFFFF00000000ull
#define DC_LCB_DBG_STS_GOOD_BAD_DAT3_VAL_SHIFT                        0
#define DC_LCB_DBG_STS_GOOD_BAD_DAT3_VAL_MASK                         0xFFFFFFFFull
#define DC_LCB_DBG_STS_GOOD_BAD_DAT3_VAL_SMASK                        0xFFFFFFFFull
/*
* Table #53 of 220_CSR_lcb.xml - LCB_DBG_STS_XFR_ERR_STATS
* This register is used to cnt LTPs with xfr errs within a LTP with a bad 
* CRC.
*/
#define DC_LCB_DBG_STS_XFR_ERR_STATS                                  (DC_LCB_CSRS + 0x000000000288)
#define DC_LCB_DBG_STS_XFR_ERR_STATS_RESETCSR                         0x0000000000000000ull
#define DC_LCB_DBG_STS_XFR_ERR_STATS_XFR_4BAD_SHIFT                   58
#define DC_LCB_DBG_STS_XFR_ERR_STATS_XFR_4BAD_MASK                    0x3Full
#define DC_LCB_DBG_STS_XFR_ERR_STATS_XFR_4BAD_SMASK                   0xFC00000000000000ull
#define DC_LCB_DBG_STS_XFR_ERR_STATS_XFR_3BAD_SHIFT                   48
#define DC_LCB_DBG_STS_XFR_ERR_STATS_XFR_3BAD_MASK                    0x3FFull
#define DC_LCB_DBG_STS_XFR_ERR_STATS_XFR_3BAD_SMASK                   0x3FF000000000000ull
#define DC_LCB_DBG_STS_XFR_ERR_STATS_XFR_2BAD_SHIFT                   32
#define DC_LCB_DBG_STS_XFR_ERR_STATS_XFR_2BAD_MASK                    0xFFFFull
#define DC_LCB_DBG_STS_XFR_ERR_STATS_XFR_2BAD_SMASK                   0xFFFF00000000ull
#define DC_LCB_DBG_STS_XFR_ERR_STATS_XFR_1BAD_SHIFT                   0
#define DC_LCB_DBG_STS_XFR_ERR_STATS_XFR_1BAD_MASK                    0xFFFFFFFFull
#define DC_LCB_DBG_STS_XFR_ERR_STATS_XFR_1BAD_SMASK                   0xFFFFFFFFull
/*
* Table #54 of 220_CSR_lcb.xml - LCB_ERR_FLG
* This is the error flag CSR.
*/
#define DC_LCB_ERR_FLG                                                (DC_LCB_CSRS + 0x000000000300)
#define DC_LCB_ERR_FLG_RESETCSR                                       0x0000000000000000ull
#define DC_LCB_ERR_FLG_UNUSED_63_30_SHIFT                             30
#define DC_LCB_ERR_FLG_UNUSED_63_30_MASK                              0x3FFFFFFFFull
#define DC_LCB_ERR_FLG_UNUSED_63_30_SMASK                             0xFFFFFFFFC0000000ull
#define DC_LCB_ERR_FLG_REDUNDANT_FLIT_PARITY_ERR_SHIFT                29
#define DC_LCB_ERR_FLG_REDUNDANT_FLIT_PARITY_ERR_MASK                 0x1ull
#define DC_LCB_ERR_FLG_REDUNDANT_FLIT_PARITY_ERR_SMASK                0x20000000ull
#define DC_LCB_ERR_FLG_NEG_EDGE_LINK_TRANSFER_ACTIVE_SHIFT            28
#define DC_LCB_ERR_FLG_NEG_EDGE_LINK_TRANSFER_ACTIVE_MASK             0x1ull
#define DC_LCB_ERR_FLG_NEG_EDGE_LINK_TRANSFER_ACTIVE_SMASK            0x10000000ull
#define DC_LCB_ERR_FLG_HOLD_REINIT_SHIFT                              27
#define DC_LCB_ERR_FLG_HOLD_REINIT_MASK                               0x1ull
#define DC_LCB_ERR_FLG_HOLD_REINIT_SMASK                              0x8000000ull
#define DC_LCB_ERR_FLG_RST_FOR_INCOMPLT_RND_TRIP_SHIFT                26
#define DC_LCB_ERR_FLG_RST_FOR_INCOMPLT_RND_TRIP_MASK                 0x1ull
#define DC_LCB_ERR_FLG_RST_FOR_INCOMPLT_RND_TRIP_SMASK                0x4000000ull
#define DC_LCB_ERR_FLG_RST_FOR_LINK_TIMEOUT_SHIFT                     25
#define DC_LCB_ERR_FLG_RST_FOR_LINK_TIMEOUT_MASK                      0x1ull
#define DC_LCB_ERR_FLG_RST_FOR_LINK_TIMEOUT_SMASK                     0x2000000ull
#define DC_LCB_ERR_FLG_CREDIT_RETURN_FLIT_MBE_SHIFT                   24
#define DC_LCB_ERR_FLG_CREDIT_RETURN_FLIT_MBE_MASK                    0x1ull
#define DC_LCB_ERR_FLG_CREDIT_RETURN_FLIT_MBE_SMASK                   0x1000000ull
#define DC_LCB_ERR_FLG_REPLAY_BUF_SBE_SHIFT                           23
#define DC_LCB_ERR_FLG_REPLAY_BUF_SBE_MASK                            0x1ull
#define DC_LCB_ERR_FLG_REPLAY_BUF_SBE_SMASK                           0x800000ull
#define DC_LCB_ERR_FLG_REPLAY_BUF_MBE_SHIFT                           22
#define DC_LCB_ERR_FLG_REPLAY_BUF_MBE_MASK                            0x1ull
#define DC_LCB_ERR_FLG_REPLAY_BUF_MBE_SMASK                           0x400000ull
#define DC_LCB_ERR_FLG_FLIT_INPUT_BUF_SBE_SHIFT                       21
#define DC_LCB_ERR_FLG_FLIT_INPUT_BUF_SBE_MASK                        0x1ull
#define DC_LCB_ERR_FLG_FLIT_INPUT_BUF_SBE_SMASK                       0x200000ull
#define DC_LCB_ERR_FLG_FLIT_INPUT_BUF_MBE_SHIFT                       20
#define DC_LCB_ERR_FLG_FLIT_INPUT_BUF_MBE_MASK                        0x1ull
#define DC_LCB_ERR_FLG_FLIT_INPUT_BUF_MBE_SMASK                       0x100000ull
#define DC_LCB_ERR_FLG_VL_ACK_INPUT_WRONG_CRC_MODE_SHIFT              19
#define DC_LCB_ERR_FLG_VL_ACK_INPUT_WRONG_CRC_MODE_MASK               0x1ull
#define DC_LCB_ERR_FLG_VL_ACK_INPUT_WRONG_CRC_MODE_SMASK              0x80000ull
#define DC_LCB_ERR_FLG_VL_ACK_INPUT_PARITY_ERR_SHIFT                  18
#define DC_LCB_ERR_FLG_VL_ACK_INPUT_PARITY_ERR_MASK                   0x1ull
#define DC_LCB_ERR_FLG_VL_ACK_INPUT_PARITY_ERR_SMASK                  0x40000ull
#define DC_LCB_ERR_FLG_VL_ACK_INPUT_BUF_OFLW_SHIFT                    17
#define DC_LCB_ERR_FLG_VL_ACK_INPUT_BUF_OFLW_MASK                     0x1ull
#define DC_LCB_ERR_FLG_VL_ACK_INPUT_BUF_OFLW_SMASK                    0x20000ull
#define DC_LCB_ERR_FLG_FLIT_INPUT_BUF_OFLW_SHIFT                      16
#define DC_LCB_ERR_FLG_FLIT_INPUT_BUF_OFLW_MASK                       0x1ull
#define DC_LCB_ERR_FLG_FLIT_INPUT_BUF_OFLW_SMASK                      0x10000ull
#define DC_LCB_ERR_FLG_ILLEGAL_FLIT_ENCODING_SHIFT                    15
#define DC_LCB_ERR_FLG_ILLEGAL_FLIT_ENCODING_MASK                     0x1ull
#define DC_LCB_ERR_FLG_ILLEGAL_FLIT_ENCODING_SMASK                    0x8000ull
#define DC_LCB_ERR_FLG_ILLEGAL_NULL_LTP_SHIFT                         14
#define DC_LCB_ERR_FLG_ILLEGAL_NULL_LTP_MASK                          0x1ull
#define DC_LCB_ERR_FLG_ILLEGAL_NULL_LTP_SMASK                         0x4000ull
#define DC_LCB_ERR_FLG_UNEXPECTED_ROUND_TRIP_MARKER_SHIFT             13
#define DC_LCB_ERR_FLG_UNEXPECTED_ROUND_TRIP_MARKER_MASK              0x1ull
#define DC_LCB_ERR_FLG_UNEXPECTED_ROUND_TRIP_MARKER_SMASK             0x2000ull
#define DC_LCB_ERR_FLG_UNEXPECTED_REPLAY_MARKER_SHIFT                 12
#define DC_LCB_ERR_FLG_UNEXPECTED_REPLAY_MARKER_MASK                  0x1ull
#define DC_LCB_ERR_FLG_UNEXPECTED_REPLAY_MARKER_SMASK                 0x1000ull
#define DC_LCB_ERR_FLG_RCLK_STOPPED_SHIFT                             11
#define DC_LCB_ERR_FLG_RCLK_STOPPED_MASK                              0x1ull
#define DC_LCB_ERR_FLG_RCLK_STOPPED_SMASK                             0x800ull
#define DC_LCB_ERR_FLG_CRC_ERR_CNT_HIT_LIMIT_SHIFT                    10
#define DC_LCB_ERR_FLG_CRC_ERR_CNT_HIT_LIMIT_MASK                     0x1ull
#define DC_LCB_ERR_FLG_CRC_ERR_CNT_HIT_LIMIT_SMASK                    0x400ull
#define DC_LCB_ERR_FLG_REINIT_FOR_LN_DEGRADE_SHIFT                    9
#define DC_LCB_ERR_FLG_REINIT_FOR_LN_DEGRADE_MASK                     0x1ull
#define DC_LCB_ERR_FLG_REINIT_FOR_LN_DEGRADE_SMASK                    0x200ull
#define DC_LCB_ERR_FLG_REINIT_FROM_PEER_SHIFT                         8
#define DC_LCB_ERR_FLG_REINIT_FROM_PEER_MASK                          0x1ull
#define DC_LCB_ERR_FLG_REINIT_FROM_PEER_SMASK                         0x100ull
#define DC_LCB_ERR_FLG_SEQ_CRC_ERR_SHIFT                              7
#define DC_LCB_ERR_FLG_SEQ_CRC_ERR_MASK                               0x1ull
#define DC_LCB_ERR_FLG_SEQ_CRC_ERR_SMASK                              0x80ull
#define DC_LCB_ERR_FLG_RX_LESS_THAN_FOUR_LNS_SHIFT                    6
#define DC_LCB_ERR_FLG_RX_LESS_THAN_FOUR_LNS_MASK                     0x1ull
#define DC_LCB_ERR_FLG_RX_LESS_THAN_FOUR_LNS_SMASK                    0x40ull
#define DC_LCB_ERR_FLG_TX_LESS_THAN_FOUR_LNS_SHIFT                    5
#define DC_LCB_ERR_FLG_TX_LESS_THAN_FOUR_LNS_MASK                     0x1ull
#define DC_LCB_ERR_FLG_TX_LESS_THAN_FOUR_LNS_SMASK                    0x20ull
#define DC_LCB_ERR_FLG_LOST_REINIT_STALL_OR_TOS_SHIFT                 4
#define DC_LCB_ERR_FLG_LOST_REINIT_STALL_OR_TOS_MASK                  0x1ull
#define DC_LCB_ERR_FLG_LOST_REINIT_STALL_OR_TOS_SMASK                 0x10ull
#define DC_LCB_ERR_FLG_ALL_LNS_FAILED_REINIT_TEST_SHIFT               3
#define DC_LCB_ERR_FLG_ALL_LNS_FAILED_REINIT_TEST_MASK                0x1ull
#define DC_LCB_ERR_FLG_ALL_LNS_FAILED_REINIT_TEST_SMASK               0x8ull
#define DC_LCB_ERR_FLG_RST_FOR_FAILED_DESKEW_SHIFT                    2
#define DC_LCB_ERR_FLG_RST_FOR_FAILED_DESKEW_MASK                     0x1ull
#define DC_LCB_ERR_FLG_RST_FOR_FAILED_DESKEW_SMASK                    0x4ull
#define DC_LCB_ERR_FLG_INVALID_CSR_ADDR_SHIFT                         1
#define DC_LCB_ERR_FLG_INVALID_CSR_ADDR_MASK                          0x1ull
#define DC_LCB_ERR_FLG_INVALID_CSR_ADDR_SMASK                         0x2ull
#define DC_LCB_ERR_FLG_CSR_PARITY_ERR_SHIFT                           0
#define DC_LCB_ERR_FLG_CSR_PARITY_ERR_MASK                            0x1ull
#define DC_LCB_ERR_FLG_CSR_PARITY_ERR_SMASK                           0x1ull
/*
* Table #55 of 220_CSR_lcb.xml - LCB_ERR_CLR
* This is the error clear CSR.
*/
#define DC_LCB_ERR_CLR                                                (DC_LCB_CSRS + 0x000000000308)
#define DC_LCB_ERR_CLR_RESETCSR                                       0x0000000000000000ull
#define DC_LCB_ERR_CLR_UNUSED_63_30_SHIFT                             30
#define DC_LCB_ERR_CLR_UNUSED_63_30_MASK                              0x3FFFFFFFFull
#define DC_LCB_ERR_CLR_UNUSED_63_30_SMASK                             0xFFFFFFFFC0000000ull
#define DC_LCB_ERR_CLR_REDUNDANT_FLIT_PARITY_ERR_SHIFT                29
#define DC_LCB_ERR_CLR_REDUNDANT_FLIT_PARITY_ERR_MASK                 0x1ull
#define DC_LCB_ERR_CLR_REDUNDANT_FLIT_PARITY_ERR_SMASK                0x20000000ull
#define DC_LCB_ERR_CLR_NEG_EDGE_LINK_TRANSFER_ACTIVE_SHIFT            28
#define DC_LCB_ERR_CLR_NEG_EDGE_LINK_TRANSFER_ACTIVE_MASK             0x1ull
#define DC_LCB_ERR_CLR_NEG_EDGE_LINK_TRANSFER_ACTIVE_SMASK            0x10000000ull
#define DC_LCB_ERR_CLR_HOLD_REINIT_SHIFT                              27
#define DC_LCB_ERR_CLR_HOLD_REINIT_MASK                               0x1ull
#define DC_LCB_ERR_CLR_HOLD_REINIT_SMASK                              0x8000000ull
#define DC_LCB_ERR_CLR_RST_FOR_INCOMPLT_RND_TRIP_SHIFT                26
#define DC_LCB_ERR_CLR_RST_FOR_INCOMPLT_RND_TRIP_MASK                 0x1ull
#define DC_LCB_ERR_CLR_RST_FOR_INCOMPLT_RND_TRIP_SMASK                0x4000000ull
#define DC_LCB_ERR_CLR_RST_FOR_LINK_TIMEOUT_SHIFT                     25
#define DC_LCB_ERR_CLR_RST_FOR_LINK_TIMEOUT_MASK                      0x1ull
#define DC_LCB_ERR_CLR_RST_FOR_LINK_TIMEOUT_SMASK                     0x2000000ull
#define DC_LCB_ERR_CLR_CREDIT_RETURN_FLIT_MBE_SHIFT                   24
#define DC_LCB_ERR_CLR_CREDIT_RETURN_FLIT_MBE_MASK                    0x1ull
#define DC_LCB_ERR_CLR_CREDIT_RETURN_FLIT_MBE_SMASK                   0x1000000ull
#define DC_LCB_ERR_CLR_REPLAY_BUF_SBE_SHIFT                           23
#define DC_LCB_ERR_CLR_REPLAY_BUF_SBE_MASK                            0x1ull
#define DC_LCB_ERR_CLR_REPLAY_BUF_SBE_SMASK                           0x800000ull
#define DC_LCB_ERR_CLR_REPLAY_BUF_MBE_SHIFT                           22
#define DC_LCB_ERR_CLR_REPLAY_BUF_MBE_MASK                            0x1ull
#define DC_LCB_ERR_CLR_REPLAY_BUF_MBE_SMASK                           0x400000ull
#define DC_LCB_ERR_CLR_FLIT_INPUT_BUF_SBE_SHIFT                       21
#define DC_LCB_ERR_CLR_FLIT_INPUT_BUF_SBE_MASK                        0x1ull
#define DC_LCB_ERR_CLR_FLIT_INPUT_BUF_SBE_SMASK                       0x200000ull
#define DC_LCB_ERR_CLR_FLIT_INPUT_BUF_MBE_SHIFT                       20
#define DC_LCB_ERR_CLR_FLIT_INPUT_BUF_MBE_MASK                        0x1ull
#define DC_LCB_ERR_CLR_FLIT_INPUT_BUF_MBE_SMASK                       0x100000ull
#define DC_LCB_ERR_CLR_VL_ACK_INPUT_WRONG_CRC_MODE_SHIFT              19
#define DC_LCB_ERR_CLR_VL_ACK_INPUT_WRONG_CRC_MODE_MASK               0x1ull
#define DC_LCB_ERR_CLR_VL_ACK_INPUT_WRONG_CRC_MODE_SMASK              0x80000ull
#define DC_LCB_ERR_CLR_VL_ACK_INPUT_PARITY_ERR_SHIFT                  18
#define DC_LCB_ERR_CLR_VL_ACK_INPUT_PARITY_ERR_MASK                   0x1ull
#define DC_LCB_ERR_CLR_VL_ACK_INPUT_PARITY_ERR_SMASK                  0x40000ull
#define DC_LCB_ERR_CLR_VL_ACK_INPUT_BUF_OFLW_SHIFT                    17
#define DC_LCB_ERR_CLR_VL_ACK_INPUT_BUF_OFLW_MASK                     0x1ull
#define DC_LCB_ERR_CLR_VL_ACK_INPUT_BUF_OFLW_SMASK                    0x20000ull
#define DC_LCB_ERR_CLR_FLIT_INPUT_BUF_OFLW_SHIFT                      16
#define DC_LCB_ERR_CLR_FLIT_INPUT_BUF_OFLW_MASK                       0x1ull
#define DC_LCB_ERR_CLR_FLIT_INPUT_BUF_OFLW_SMASK                      0x10000ull
#define DC_LCB_ERR_CLR_ILLEGAL_FLIT_ENCODING_SHIFT                    15
#define DC_LCB_ERR_CLR_ILLEGAL_FLIT_ENCODING_MASK                     0x1ull
#define DC_LCB_ERR_CLR_ILLEGAL_FLIT_ENCODING_SMASK                    0x8000ull
#define DC_LCB_ERR_CLR_ILLEGAL_NULL_LTP_SHIFT                         14
#define DC_LCB_ERR_CLR_ILLEGAL_NULL_LTP_MASK                          0x1ull
#define DC_LCB_ERR_CLR_ILLEGAL_NULL_LTP_SMASK                         0x4000ull
#define DC_LCB_ERR_CLR_UNEXPECTED_ROUND_TRIP_MARKER_SHIFT             13
#define DC_LCB_ERR_CLR_UNEXPECTED_ROUND_TRIP_MARKER_MASK              0x1ull
#define DC_LCB_ERR_CLR_UNEXPECTED_ROUND_TRIP_MARKER_SMASK             0x2000ull
#define DC_LCB_ERR_CLR_UNEXPECTED_REPLAY_MARKER_SHIFT                 12
#define DC_LCB_ERR_CLR_UNEXPECTED_REPLAY_MARKER_MASK                  0x1ull
#define DC_LCB_ERR_CLR_UNEXPECTED_REPLAY_MARKER_SMASK                 0x1000ull
#define DC_LCB_ERR_CLR_RCLK_STOPPED_SHIFT                             11
#define DC_LCB_ERR_CLR_RCLK_STOPPED_MASK                              0x1ull
#define DC_LCB_ERR_CLR_RCLK_STOPPED_SMASK                             0x800ull
#define DC_LCB_ERR_CLR_CRC_ERR_CNT_HIT_LIMIT_SHIFT                    10
#define DC_LCB_ERR_CLR_CRC_ERR_CNT_HIT_LIMIT_MASK                     0x1ull
#define DC_LCB_ERR_CLR_CRC_ERR_CNT_HIT_LIMIT_SMASK                    0x400ull
#define DC_LCB_ERR_CLR_REINIT_FOR_LN_DEGRADE_SHIFT                    9
#define DC_LCB_ERR_CLR_REINIT_FOR_LN_DEGRADE_MASK                     0x1ull
#define DC_LCB_ERR_CLR_REINIT_FOR_LN_DEGRADE_SMASK                    0x200ull
#define DC_LCB_ERR_CLR_REINIT_FROM_PEER_SHIFT                         8
#define DC_LCB_ERR_CLR_REINIT_FROM_PEER_MASK                          0x1ull
#define DC_LCB_ERR_CLR_REINIT_FROM_PEER_SMASK                         0x100ull
#define DC_LCB_ERR_CLR_SEQ_CRC_ERR_SHIFT                              7
#define DC_LCB_ERR_CLR_SEQ_CRC_ERR_MASK                               0x1ull
#define DC_LCB_ERR_CLR_SEQ_CRC_ERR_SMASK                              0x80ull
#define DC_LCB_ERR_CLR_RX_LESS_THAN_FOUR_LNS_SHIFT                    6
#define DC_LCB_ERR_CLR_RX_LESS_THAN_FOUR_LNS_MASK                     0x1ull
#define DC_LCB_ERR_CLR_RX_LESS_THAN_FOUR_LNS_SMASK                    0x40ull
#define DC_LCB_ERR_CLR_TX_LESS_THAN_FOUR_LNS_SHIFT                    5
#define DC_LCB_ERR_CLR_TX_LESS_THAN_FOUR_LNS_MASK                     0x1ull
#define DC_LCB_ERR_CLR_TX_LESS_THAN_FOUR_LNS_SMASK                    0x20ull
#define DC_LCB_ERR_CLR_LOST_REINIT_STALL_OR_TOS_SHIFT                 4
#define DC_LCB_ERR_CLR_LOST_REINIT_STALL_OR_TOS_MASK                  0x1ull
#define DC_LCB_ERR_CLR_LOST_REINIT_STALL_OR_TOS_SMASK                 0x10ull
#define DC_LCB_ERR_CLR_ALL_LNS_FAILED_REINIT_TEST_SHIFT               3
#define DC_LCB_ERR_CLR_ALL_LNS_FAILED_REINIT_TEST_MASK                0x1ull
#define DC_LCB_ERR_CLR_ALL_LNS_FAILED_REINIT_TEST_SMASK               0x8ull
#define DC_LCB_ERR_CLR_RST_FOR_FAILED_DESKEW_SHIFT                    2
#define DC_LCB_ERR_CLR_RST_FOR_FAILED_DESKEW_MASK                     0x1ull
#define DC_LCB_ERR_CLR_RST_FOR_FAILED_DESKEW_SMASK                    0x4ull
#define DC_LCB_ERR_CLR_INVALID_CSR_ADDR_SHIFT                         1
#define DC_LCB_ERR_CLR_INVALID_CSR_ADDR_MASK                          0x1ull
#define DC_LCB_ERR_CLR_INVALID_CSR_ADDR_SMASK                         0x2ull
#define DC_LCB_ERR_CLR_CSR_PARITY_ERR_SHIFT                           0
#define DC_LCB_ERR_CLR_CSR_PARITY_ERR_MASK                            0x1ull
#define DC_LCB_ERR_CLR_CSR_PARITY_ERR_SMASK                           0x1ull
/*
* Table #56 of 220_CSR_lcb.xml - LCB_ERR_EN
* This register is used to enable interrupts for the associated error 
* flag.
*/
#define DC_LCB_ERR_EN                                                 (DC_LCB_CSRS + 0x000000000310)
#define DC_LCB_ERR_EN_RESETCSR                                        0x0000000000000000ull
#define DC_LCB_ERR_EN_UNUSED_63_30_SHIFT                              30
#define DC_LCB_ERR_EN_UNUSED_63_30_MASK                               0x3FFFFFFFFull
#define DC_LCB_ERR_EN_UNUSED_63_30_SMASK                              0xFFFFFFFFC0000000ull
#define DC_LCB_ERR_EN_REDUNDANT_FLIT_PARITY_ERR_SHIFT                 29
#define DC_LCB_ERR_EN_REDUNDANT_FLIT_PARITY_ERR_MASK                  0x1ull
#define DC_LCB_ERR_EN_REDUNDANT_FLIT_PARITY_ERR_SMASK                 0x20000000ull
#define DC_LCB_ERR_EN_NEG_EDGE_LINK_TRANSFER_ACTIVE_SHIFT             28
#define DC_LCB_ERR_EN_NEG_EDGE_LINK_TRANSFER_ACTIVE_MASK              0x1ull
#define DC_LCB_ERR_EN_NEG_EDGE_LINK_TRANSFER_ACTIVE_SMASK             0x10000000ull
#define DC_LCB_ERR_EN_HOLD_REINIT_SHIFT                               27
#define DC_LCB_ERR_EN_HOLD_REINIT_MASK                                0x1ull
#define DC_LCB_ERR_EN_HOLD_REINIT_SMASK                               0x8000000ull
#define DC_LCB_ERR_EN_RST_FOR_INCOMPLT_RND_TRIP_SHIFT                 26
#define DC_LCB_ERR_EN_RST_FOR_INCOMPLT_RND_TRIP_MASK                  0x1ull
#define DC_LCB_ERR_EN_RST_FOR_INCOMPLT_RND_TRIP_SMASK                 0x4000000ull
#define DC_LCB_ERR_EN_RST_FOR_LINK_TIMEOUT_SHIFT                      25
#define DC_LCB_ERR_EN_RST_FOR_LINK_TIMEOUT_MASK                       0x1ull
#define DC_LCB_ERR_EN_RST_FOR_LINK_TIMEOUT_SMASK                      0x2000000ull
#define DC_LCB_ERR_EN_CREDIT_RETURN_FLIT_MBE_SHIFT                    24
#define DC_LCB_ERR_EN_CREDIT_RETURN_FLIT_MBE_MASK                     0x1ull
#define DC_LCB_ERR_EN_CREDIT_RETURN_FLIT_MBE_SMASK                    0x1000000ull
#define DC_LCB_ERR_EN_REPLAY_BUF_SBE_SHIFT                            23
#define DC_LCB_ERR_EN_REPLAY_BUF_SBE_MASK                             0x1ull
#define DC_LCB_ERR_EN_REPLAY_BUF_SBE_SMASK                            0x800000ull
#define DC_LCB_ERR_EN_REPLAY_BUF_MBE_SHIFT                            22
#define DC_LCB_ERR_EN_REPLAY_BUF_MBE_MASK                             0x1ull
#define DC_LCB_ERR_EN_REPLAY_BUF_MBE_SMASK                            0x400000ull
#define DC_LCB_ERR_EN_FLIT_INPUT_BUF_SBE_SHIFT                        21
#define DC_LCB_ERR_EN_FLIT_INPUT_BUF_SBE_MASK                         0x1ull
#define DC_LCB_ERR_EN_FLIT_INPUT_BUF_SBE_SMASK                        0x200000ull
#define DC_LCB_ERR_EN_FLIT_INPUT_BUF_MBE_SHIFT                        20
#define DC_LCB_ERR_EN_FLIT_INPUT_BUF_MBE_MASK                         0x1ull
#define DC_LCB_ERR_EN_FLIT_INPUT_BUF_MBE_SMASK                        0x100000ull
#define DC_LCB_ERR_EN_VL_ACK_INPUT_WRONG_CRC_MODE_SHIFT               19
#define DC_LCB_ERR_EN_VL_ACK_INPUT_WRONG_CRC_MODE_MASK                0x1ull
#define DC_LCB_ERR_EN_VL_ACK_INPUT_WRONG_CRC_MODE_SMASK               0x80000ull
#define DC_LCB_ERR_EN_VL_ACK_INPUT_PARITY_ERR_SHIFT                   18
#define DC_LCB_ERR_EN_VL_ACK_INPUT_PARITY_ERR_MASK                    0x1ull
#define DC_LCB_ERR_EN_VL_ACK_INPUT_PARITY_ERR_SMASK                   0x40000ull
#define DC_LCB_ERR_EN_VL_ACK_INPUT_BUF_OFLW_SHIFT                     17
#define DC_LCB_ERR_EN_VL_ACK_INPUT_BUF_OFLW_MASK                      0x1ull
#define DC_LCB_ERR_EN_VL_ACK_INPUT_BUF_OFLW_SMASK                     0x20000ull
#define DC_LCB_ERR_EN_FLIT_INPUT_BUF_OFLW_SHIFT                       16
#define DC_LCB_ERR_EN_FLIT_INPUT_BUF_OFLW_MASK                        0x1ull
#define DC_LCB_ERR_EN_FLIT_INPUT_BUF_OFLW_SMASK                       0x10000ull
#define DC_LCB_ERR_EN_ILLEGAL_FLIT_ENCODING_SHIFT                     15
#define DC_LCB_ERR_EN_ILLEGAL_FLIT_ENCODING_MASK                      0x1ull
#define DC_LCB_ERR_EN_ILLEGAL_FLIT_ENCODING_SMASK                     0x8000ull
#define DC_LCB_ERR_EN_ILLEGAL_NULL_LTP_SHIFT                          14
#define DC_LCB_ERR_EN_ILLEGAL_NULL_LTP_MASK                           0x1ull
#define DC_LCB_ERR_EN_ILLEGAL_NULL_LTP_SMASK                          0x4000ull
#define DC_LCB_ERR_EN_UNEXPECTED_ROUND_TRIP_MARKER_SHIFT              13
#define DC_LCB_ERR_EN_UNEXPECTED_ROUND_TRIP_MARKER_MASK               0x1ull
#define DC_LCB_ERR_EN_UNEXPECTED_ROUND_TRIP_MARKER_SMASK              0x2000ull
#define DC_LCB_ERR_EN_UNEXPECTED_REPLAY_MARKER_SHIFT                  12
#define DC_LCB_ERR_EN_UNEXPECTED_REPLAY_MARKER_MASK                   0x1ull
#define DC_LCB_ERR_EN_UNEXPECTED_REPLAY_MARKER_SMASK                  0x1000ull
#define DC_LCB_ERR_EN_RCLK_STOPPED_SHIFT                              11
#define DC_LCB_ERR_EN_RCLK_STOPPED_MASK                               0x1ull
#define DC_LCB_ERR_EN_RCLK_STOPPED_SMASK                              0x800ull
#define DC_LCB_ERR_EN_CRC_ERR_CNT_HIT_LIMIT_SHIFT                     10
#define DC_LCB_ERR_EN_CRC_ERR_CNT_HIT_LIMIT_MASK                      0x1ull
#define DC_LCB_ERR_EN_CRC_ERR_CNT_HIT_LIMIT_SMASK                     0x400ull
#define DC_LCB_ERR_EN_REINIT_FOR_LN_DEGRADE_SHIFT                     9
#define DC_LCB_ERR_EN_REINIT_FOR_LN_DEGRADE_MASK                      0x1ull
#define DC_LCB_ERR_EN_REINIT_FOR_LN_DEGRADE_SMASK                     0x200ull
#define DC_LCB_ERR_EN_REINIT_FROM_PEER_SHIFT                          8
#define DC_LCB_ERR_EN_REINIT_FROM_PEER_MASK                           0x1ull
#define DC_LCB_ERR_EN_REINIT_FROM_PEER_SMASK                          0x100ull
#define DC_LCB_ERR_EN_SEQ_CRC_ERR_SHIFT                               7
#define DC_LCB_ERR_EN_SEQ_CRC_ERR_MASK                                0x1ull
#define DC_LCB_ERR_EN_SEQ_CRC_ERR_SMASK                               0x80ull
#define DC_LCB_ERR_EN_RX_LESS_THAN_FOUR_LNS_SHIFT                     6
#define DC_LCB_ERR_EN_RX_LESS_THAN_FOUR_LNS_MASK                      0x1ull
#define DC_LCB_ERR_EN_RX_LESS_THAN_FOUR_LNS_SMASK                     0x40ull
#define DC_LCB_ERR_EN_TX_LESS_THAN_FOUR_LNS_SHIFT                     5
#define DC_LCB_ERR_EN_TX_LESS_THAN_FOUR_LNS_MASK                      0x1ull
#define DC_LCB_ERR_EN_TX_LESS_THAN_FOUR_LNS_SMASK                     0x20ull
#define DC_LCB_ERR_EN_LOST_REINIT_STALL_OR_TOS_SHIFT                  4
#define DC_LCB_ERR_EN_LOST_REINIT_STALL_OR_TOS_MASK                   0x1ull
#define DC_LCB_ERR_EN_LOST_REINIT_STALL_OR_TOS_SMASK                  0x10ull
#define DC_LCB_ERR_EN_ALL_LNS_FAILED_REINIT_TEST_SHIFT                3
#define DC_LCB_ERR_EN_ALL_LNS_FAILED_REINIT_TEST_MASK                 0x1ull
#define DC_LCB_ERR_EN_ALL_LNS_FAILED_REINIT_TEST_SMASK                0x8ull
#define DC_LCB_ERR_EN_RST_FOR_FAILED_DESKEW_SHIFT                     2
#define DC_LCB_ERR_EN_RST_FOR_FAILED_DESKEW_MASK                      0x1ull
#define DC_LCB_ERR_EN_RST_FOR_FAILED_DESKEW_SMASK                     0x4ull
#define DC_LCB_ERR_EN_INVALID_CSR_ADDR_SHIFT                          1
#define DC_LCB_ERR_EN_INVALID_CSR_ADDR_MASK                           0x1ull
#define DC_LCB_ERR_EN_INVALID_CSR_ADDR_SMASK                          0x2ull
#define DC_LCB_ERR_EN_CSR_PARITY_ERR_SHIFT                            0
#define DC_LCB_ERR_EN_CSR_PARITY_ERR_MASK                             0x1ull
#define DC_LCB_ERR_EN_CSR_PARITY_ERR_SMASK                            0x1ull
/*
* Table #57 of 220_CSR_lcb.xml - LCB_ERR_FIRST_FLG
* This is a CSR description.
*/
#define DC_LCB_ERR_FIRST_FLG                                          (DC_LCB_CSRS + 0x000000000318)
#define DC_LCB_ERR_FIRST_FLG_RESETCSR                                 0x0000000000000000ull
#define DC_LCB_ERR_FIRST_FLG_UNUSED_63_30_SHIFT                       30
#define DC_LCB_ERR_FIRST_FLG_UNUSED_63_30_MASK                        0x3FFFFFFFFull
#define DC_LCB_ERR_FIRST_FLG_UNUSED_63_30_SMASK                       0xFFFFFFFFC0000000ull
#define DC_LCB_ERR_FIRST_FLG_REDUNDANT_FLIT_PARITY_ERR_SHIFT          29
#define DC_LCB_ERR_FIRST_FLG_REDUNDANT_FLIT_PARITY_ERR_MASK           0x1ull
#define DC_LCB_ERR_FIRST_FLG_REDUNDANT_FLIT_PARITY_ERR_SMASK          0x20000000ull
#define DC_LCB_ERR_FIRST_FLG_NEG_EDGE_LINK_TRANSFER_ACTIVE_SHIFT      28
#define DC_LCB_ERR_FIRST_FLG_NEG_EDGE_LINK_TRANSFER_ACTIVE_MASK       0x1ull
#define DC_LCB_ERR_FIRST_FLG_NEG_EDGE_LINK_TRANSFER_ACTIVE_SMASK      0x10000000ull
#define DC_LCB_ERR_FIRST_FLG_HOLD_REINIT_SHIFT                        27
#define DC_LCB_ERR_FIRST_FLG_HOLD_REINIT_MASK                         0x1ull
#define DC_LCB_ERR_FIRST_FLG_HOLD_REINIT_SMASK                        0x8000000ull
#define DC_LCB_ERR_FIRST_FLG_RST_FOR_INCOMPLT_RND_TRIP_SHIFT          26
#define DC_LCB_ERR_FIRST_FLG_RST_FOR_INCOMPLT_RND_TRIP_MASK           0x1ull
#define DC_LCB_ERR_FIRST_FLG_RST_FOR_INCOMPLT_RND_TRIP_SMASK          0x4000000ull
#define DC_LCB_ERR_FIRST_FLG_RST_FOR_LINK_TIMEOUT_SHIFT               25
#define DC_LCB_ERR_FIRST_FLG_RST_FOR_LINK_TIMEOUT_MASK                0x1ull
#define DC_LCB_ERR_FIRST_FLG_RST_FOR_LINK_TIMEOUT_SMASK               0x2000000ull
#define DC_LCB_ERR_FIRST_FLG_CREDIT_RETURN_FLIT_MBE_SHIFT             24
#define DC_LCB_ERR_FIRST_FLG_CREDIT_RETURN_FLIT_MBE_MASK              0x1ull
#define DC_LCB_ERR_FIRST_FLG_CREDIT_RETURN_FLIT_MBE_SMASK             0x1000000ull
#define DC_LCB_ERR_FIRST_FLG_REPLAY_BUF_SBE_SHIFT                     23
#define DC_LCB_ERR_FIRST_FLG_REPLAY_BUF_SBE_MASK                      0x1ull
#define DC_LCB_ERR_FIRST_FLG_REPLAY_BUF_SBE_SMASK                     0x800000ull
#define DC_LCB_ERR_FIRST_FLG_REPLAY_BUF_MBE_SHIFT                     22
#define DC_LCB_ERR_FIRST_FLG_REPLAY_BUF_MBE_MASK                      0x1ull
#define DC_LCB_ERR_FIRST_FLG_REPLAY_BUF_MBE_SMASK                     0x400000ull
#define DC_LCB_ERR_FIRST_FLG_FLIT_INPUT_BUF_SBE_SHIFT                 21
#define DC_LCB_ERR_FIRST_FLG_FLIT_INPUT_BUF_SBE_MASK                  0x1ull
#define DC_LCB_ERR_FIRST_FLG_FLIT_INPUT_BUF_SBE_SMASK                 0x200000ull
#define DC_LCB_ERR_FIRST_FLG_FLIT_INPUT_BUF_MBE_SHIFT                 20
#define DC_LCB_ERR_FIRST_FLG_FLIT_INPUT_BUF_MBE_MASK                  0x1ull
#define DC_LCB_ERR_FIRST_FLG_FLIT_INPUT_BUF_MBE_SMASK                 0x100000ull
#define DC_LCB_ERR_FIRST_FLG_VL_ACK_INPUT_WRONG_CRC_MODE_SHIFT        19
#define DC_LCB_ERR_FIRST_FLG_VL_ACK_INPUT_WRONG_CRC_MODE_MASK         0x1ull
#define DC_LCB_ERR_FIRST_FLG_VL_ACK_INPUT_WRONG_CRC_MODE_SMASK        0x80000ull
#define DC_LCB_ERR_FIRST_FLG_VL_ACK_INPUT_PARITY_ERR_SHIFT            18
#define DC_LCB_ERR_FIRST_FLG_VL_ACK_INPUT_PARITY_ERR_MASK             0x1ull
#define DC_LCB_ERR_FIRST_FLG_VL_ACK_INPUT_PARITY_ERR_SMASK            0x40000ull
#define DC_LCB_ERR_FIRST_FLG_VL_ACK_INPUT_BUF_OFLW_SHIFT              17
#define DC_LCB_ERR_FIRST_FLG_VL_ACK_INPUT_BUF_OFLW_MASK               0x1ull
#define DC_LCB_ERR_FIRST_FLG_VL_ACK_INPUT_BUF_OFLW_SMASK              0x20000ull
#define DC_LCB_ERR_FIRST_FLG_FLIT_INPUT_BUF_OFLW_SHIFT                16
#define DC_LCB_ERR_FIRST_FLG_FLIT_INPUT_BUF_OFLW_MASK                 0x1ull
#define DC_LCB_ERR_FIRST_FLG_FLIT_INPUT_BUF_OFLW_SMASK                0x10000ull
#define DC_LCB_ERR_FIRST_FLG_ILLEGAL_FLIT_ENCODING_SHIFT              15
#define DC_LCB_ERR_FIRST_FLG_ILLEGAL_FLIT_ENCODING_MASK               0x1ull
#define DC_LCB_ERR_FIRST_FLG_ILLEGAL_FLIT_ENCODING_SMASK              0x8000ull
#define DC_LCB_ERR_FIRST_FLG_ILLEGAL_NULL_LTP_SHIFT                   14
#define DC_LCB_ERR_FIRST_FLG_ILLEGAL_NULL_LTP_MASK                    0x1ull
#define DC_LCB_ERR_FIRST_FLG_ILLEGAL_NULL_LTP_SMASK                   0x4000ull
#define DC_LCB_ERR_FIRST_FLG_UNEXPECTED_ROUND_TRIP_MARKER_SHIFT       13
#define DC_LCB_ERR_FIRST_FLG_UNEXPECTED_ROUND_TRIP_MARKER_MASK        0x1ull
#define DC_LCB_ERR_FIRST_FLG_UNEXPECTED_ROUND_TRIP_MARKER_SMASK       0x2000ull
#define DC_LCB_ERR_FIRST_FLG_UNEXPECTED_REPLAY_MARKER_SHIFT           12
#define DC_LCB_ERR_FIRST_FLG_UNEXPECTED_REPLAY_MARKER_MASK            0x1ull
#define DC_LCB_ERR_FIRST_FLG_UNEXPECTED_REPLAY_MARKER_SMASK           0x1000ull
#define DC_LCB_ERR_FIRST_FLG_RCLK_STOPPED_SHIFT                       11
#define DC_LCB_ERR_FIRST_FLG_RCLK_STOPPED_MASK                        0x1ull
#define DC_LCB_ERR_FIRST_FLG_RCLK_STOPPED_SMASK                       0x800ull
#define DC_LCB_ERR_FIRST_FLG_CRC_ERR_CNT_HIT_LIMIT_SHIFT              10
#define DC_LCB_ERR_FIRST_FLG_CRC_ERR_CNT_HIT_LIMIT_MASK               0x1ull
#define DC_LCB_ERR_FIRST_FLG_CRC_ERR_CNT_HIT_LIMIT_SMASK              0x400ull
#define DC_LCB_ERR_FIRST_FLG_REINIT_FOR_LN_DEGRADE_SHIFT              9
#define DC_LCB_ERR_FIRST_FLG_REINIT_FOR_LN_DEGRADE_MASK               0x1ull
#define DC_LCB_ERR_FIRST_FLG_REINIT_FOR_LN_DEGRADE_SMASK              0x200ull
#define DC_LCB_ERR_FIRST_FLG_REINIT_FROM_PEER_SHIFT                   8
#define DC_LCB_ERR_FIRST_FLG_REINIT_FROM_PEER_MASK                    0x1ull
#define DC_LCB_ERR_FIRST_FLG_REINIT_FROM_PEER_SMASK                   0x100ull
#define DC_LCB_ERR_FIRST_FLG_SEQ_CRC_ERR_SHIFT                        7
#define DC_LCB_ERR_FIRST_FLG_SEQ_CRC_ERR_MASK                         0x1ull
#define DC_LCB_ERR_FIRST_FLG_SEQ_CRC_ERR_SMASK                        0x80ull
#define DC_LCB_ERR_FIRST_FLG_RX_LESS_THAN_FOUR_LNS_SHIFT              6
#define DC_LCB_ERR_FIRST_FLG_RX_LESS_THAN_FOUR_LNS_MASK               0x1ull
#define DC_LCB_ERR_FIRST_FLG_RX_LESS_THAN_FOUR_LNS_SMASK              0x40ull
#define DC_LCB_ERR_FIRST_FLG_TX_LESS_THAN_FOUR_LNS_SHIFT              5
#define DC_LCB_ERR_FIRST_FLG_TX_LESS_THAN_FOUR_LNS_MASK               0x1ull
#define DC_LCB_ERR_FIRST_FLG_TX_LESS_THAN_FOUR_LNS_SMASK              0x20ull
#define DC_LCB_ERR_FIRST_FLG_LOST_REINIT_STALL_OR_TOS_SHIFT           4
#define DC_LCB_ERR_FIRST_FLG_LOST_REINIT_STALL_OR_TOS_MASK            0x1ull
#define DC_LCB_ERR_FIRST_FLG_LOST_REINIT_STALL_OR_TOS_SMASK           0x10ull
#define DC_LCB_ERR_FIRST_FLG_ALL_LNS_FAILED_REINIT_TEST_SHIFT         3
#define DC_LCB_ERR_FIRST_FLG_ALL_LNS_FAILED_REINIT_TEST_MASK          0x1ull
#define DC_LCB_ERR_FIRST_FLG_ALL_LNS_FAILED_REINIT_TEST_SMASK         0x8ull
#define DC_LCB_ERR_FIRST_FLG_RST_FOR_FAILED_DESKEW_SHIFT              2
#define DC_LCB_ERR_FIRST_FLG_RST_FOR_FAILED_DESKEW_MASK               0x1ull
#define DC_LCB_ERR_FIRST_FLG_RST_FOR_FAILED_DESKEW_SMASK              0x4ull
#define DC_LCB_ERR_FIRST_FLG_INVALID_CSR_ADDR_SHIFT                   1
#define DC_LCB_ERR_FIRST_FLG_INVALID_CSR_ADDR_MASK                    0x1ull
#define DC_LCB_ERR_FIRST_FLG_INVALID_CSR_ADDR_SMASK                   0x2ull
#define DC_LCB_ERR_FIRST_FLG_CSR_PARITY_ERR_SHIFT                     0
#define DC_LCB_ERR_FIRST_FLG_CSR_PARITY_ERR_MASK                      0x1ull
#define DC_LCB_ERR_FIRST_FLG_CSR_PARITY_ERR_SMASK                     0x1ull
/*
* Table #58 of 220_CSR_lcb.xml - LCB_ERR_INFO_TOTAL_CRC_ERR
* This register is used to report the total number of Rx side LTPs with a CRC 
* error.
*/
#define DC_LCB_ERR_INFO_TOTAL_CRC_ERR                                 (DC_LCB_CSRS + 0x000000000320)
#define DC_LCB_ERR_INFO_TOTAL_CRC_ERR_RESETCSR                        0x0000000000000000ull
#define DC_LCB_ERR_INFO_TOTAL_CRC_ERR_CNT_SHIFT                       0
#define DC_LCB_ERR_INFO_TOTAL_CRC_ERR_CNT_MASK                        0xFFFFFFFFFFFFFFFFull
#define DC_LCB_ERR_INFO_TOTAL_CRC_ERR_CNT_SMASK                       0xFFFFFFFFFFFFFFFFull
/*
* Table #59 of 220_CSR_lcb.xml - LCB_ERR_INFO_CRC_ERR_LN0
* This register is used to report the total number of Rx side LTPs with a CRC 
* error with lane 0 identified as the culprit.
*/
#define DC_LCB_ERR_INFO_CRC_ERR_LN0                                   (DC_LCB_CSRS + 0x000000000328)
#define DC_LCB_ERR_INFO_CRC_ERR_LN0_RESETCSR                          0x0000000000000000ull
#define DC_LCB_ERR_INFO_CRC_ERR_LN0_CNT_SHIFT                         0
#define DC_LCB_ERR_INFO_CRC_ERR_LN0_CNT_MASK                          0xFFFFFFFFFFFFFFFFull
#define DC_LCB_ERR_INFO_CRC_ERR_LN0_CNT_SMASK                         0xFFFFFFFFFFFFFFFFull
/*
* Table #60 of 220_CSR_lcb.xml - LCB_ERR_INFO_CRC_ERR_LN1
* This register is used to report the total number of Rx side LTPs with a CRC 
* error with lane 1 identified as the culprit.
*/
#define DC_LCB_ERR_INFO_CRC_ERR_LN1                                   (DC_LCB_CSRS + 0x000000000330)
#define DC_LCB_ERR_INFO_CRC_ERR_LN1_RESETCSR                          0x0000000000000000ull
#define DC_LCB_ERR_INFO_CRC_ERR_LN1_CNT_SHIFT                         0
#define DC_LCB_ERR_INFO_CRC_ERR_LN1_CNT_MASK                          0xFFFFFFFFFFFFFFFFull
#define DC_LCB_ERR_INFO_CRC_ERR_LN1_CNT_SMASK                         0xFFFFFFFFFFFFFFFFull
/*
* Table #61 of 220_CSR_lcb.xml - LCB_ERR_INFO_CRC_ERR_LN2
* This register is used to report the total number of Rx side LTPs with a CRC 
* error with lane 2 identified as the culprit.
*/
#define DC_LCB_ERR_INFO_CRC_ERR_LN2                                   (DC_LCB_CSRS + 0x000000000338)
#define DC_LCB_ERR_INFO_CRC_ERR_LN2_RESETCSR                          0x0000000000000000ull
#define DC_LCB_ERR_INFO_CRC_ERR_LN2_CNT_SHIFT                         0
#define DC_LCB_ERR_INFO_CRC_ERR_LN2_CNT_MASK                          0xFFFFFFFFFFFFFFFFull
#define DC_LCB_ERR_INFO_CRC_ERR_LN2_CNT_SMASK                         0xFFFFFFFFFFFFFFFFull
/*
* Table #62 of 220_CSR_lcb.xml - LCB_ERR_INFO_CRC_ERR_LN3
* This register is used to report the total number of Rx side LTPs with a CRC 
* error with lane 3 identified as the culprit.
*/
#define DC_LCB_ERR_INFO_CRC_ERR_LN3                                   (DC_LCB_CSRS + 0x000000000340)
#define DC_LCB_ERR_INFO_CRC_ERR_LN3_RESETCSR                          0x0000000000000000ull
#define DC_LCB_ERR_INFO_CRC_ERR_LN3_CNT_SHIFT                         0
#define DC_LCB_ERR_INFO_CRC_ERR_LN3_CNT_MASK                          0xFFFFFFFFFFFFFFFFull
#define DC_LCB_ERR_INFO_CRC_ERR_LN3_CNT_SMASK                         0xFFFFFFFFFFFFFFFFull
/*
* Table #63 of 220_CSR_lcb.xml - LCB_ERR_INFO_CRC_ERR_MULTI_LN
* This register is used to report the total number of Rx side LTPs with a CRC 
* error with more than one lane identified as the culprit.
*/
#define DC_LCB_ERR_INFO_CRC_ERR_MULTI_LN                              (DC_LCB_CSRS + 0x000000000348)
#define DC_LCB_ERR_INFO_CRC_ERR_MULTI_LN_RESETCSR                     0x0000000000000000ull
#define DC_LCB_ERR_INFO_CRC_ERR_MULTI_LN_UNUSED_63_20_SHIFT           20
#define DC_LCB_ERR_INFO_CRC_ERR_MULTI_LN_UNUSED_63_20_MASK            0xFFFFFFFFFFFull
#define DC_LCB_ERR_INFO_CRC_ERR_MULTI_LN_UNUSED_63_20_SMASK           0xFFFFFFFFFFF00000ull
#define DC_LCB_ERR_INFO_CRC_ERR_MULTI_LN_CNT_SHIFT                    0
#define DC_LCB_ERR_INFO_CRC_ERR_MULTI_LN_CNT_MASK                     0xFFFFFull
#define DC_LCB_ERR_INFO_CRC_ERR_MULTI_LN_CNT_SMASK                    0xFFFFFull
/*
* Table #64 of 220_CSR_lcb.xml - LCB_ERR_INFO_TX_REPLAY_CNT
* This register is used to report the total number of Tx side retransmission 
* sequences.
*/
#define DC_LCB_ERR_INFO_TX_REPLAY_CNT                                 (DC_LCB_CSRS + 0x000000000350)
#define DC_LCB_ERR_INFO_TX_REPLAY_CNT_RESETCSR                        0x0000000000000000ull
#define DC_LCB_ERR_INFO_TX_REPLAY_CNT_VAL_SHIFT                       0
#define DC_LCB_ERR_INFO_TX_REPLAY_CNT_VAL_MASK                        0xFFFFFFFFFFFFFFFFull
#define DC_LCB_ERR_INFO_TX_REPLAY_CNT_VAL_SMASK                       0xFFFFFFFFFFFFFFFFull
/*
* Table #65 of 220_CSR_lcb.xml - LCB_ERR_INFO_RX_REPLAY_CNT
* This register is used to report the total number of Rx side retransmission 
* sequences.
*/
#define DC_LCB_ERR_INFO_RX_REPLAY_CNT                                 (DC_LCB_CSRS + 0x000000000358)
#define DC_LCB_ERR_INFO_RX_REPLAY_CNT_RESETCSR                        0x0000000000000000ull
#define DC_LCB_ERR_INFO_RX_REPLAY_CNT_VAL_SHIFT                       0
#define DC_LCB_ERR_INFO_RX_REPLAY_CNT_VAL_MASK                        0xFFFFFFFFFFFFFFFFull
#define DC_LCB_ERR_INFO_RX_REPLAY_CNT_VAL_SMASK                       0xFFFFFFFFFFFFFFFFull
/*
* Table #66 of 220_CSR_lcb.xml - LCB_ERR_INFO_SEQ_CRC_CNT
* This register is used to report the total number of Rx side sequential CRC 
* error count.
*/
#define DC_LCB_ERR_INFO_SEQ_CRC_CNT                                   (DC_LCB_CSRS + 0x000000000360)
#define DC_LCB_ERR_INFO_SEQ_CRC_CNT_RESETCSR                          0x0000000000000000ull
#define DC_LCB_ERR_INFO_SEQ_CRC_CNT_UNUSED_63_32_SHIFT                32
#define DC_LCB_ERR_INFO_SEQ_CRC_CNT_UNUSED_63_32_MASK                 0xFFFFFFFFull
#define DC_LCB_ERR_INFO_SEQ_CRC_CNT_UNUSED_63_32_SMASK                0xFFFFFFFF00000000ull
#define DC_LCB_ERR_INFO_SEQ_CRC_CNT_VAL_SHIFT                         0
#define DC_LCB_ERR_INFO_SEQ_CRC_CNT_VAL_MASK                          0xFFFFFFFFull
#define DC_LCB_ERR_INFO_SEQ_CRC_CNT_VAL_SMASK                         0xFFFFFFFFull
/*
* Table #67 of 220_CSR_lcb.xml - LCB_ERR_INFO_ESCAPE_0_ONLY_CNT
* This register is used to report the total number of poly 0 only false positive 
* 12 bit CRC checks in 48 bit CRC mode when four lanes are active.
*/
#define DC_LCB_ERR_INFO_ESCAPE_0_ONLY_CNT                             (DC_LCB_CSRS + 0x000000000368)
#define DC_LCB_ERR_INFO_ESCAPE_0_ONLY_CNT_RESETCSR                    0x0000000000000000ull
#define DC_LCB_ERR_INFO_ESCAPE_0_ONLY_CNT_UNUSED_63_32_SHIFT          32
#define DC_LCB_ERR_INFO_ESCAPE_0_ONLY_CNT_UNUSED_63_32_MASK           0xFFFFFFFFull
#define DC_LCB_ERR_INFO_ESCAPE_0_ONLY_CNT_UNUSED_63_32_SMASK          0xFFFFFFFF00000000ull
#define DC_LCB_ERR_INFO_ESCAPE_0_ONLY_CNT_VAL_SHIFT                   0
#define DC_LCB_ERR_INFO_ESCAPE_0_ONLY_CNT_VAL_MASK                    0xFFFFFFFFull
#define DC_LCB_ERR_INFO_ESCAPE_0_ONLY_CNT_VAL_SMASK                   0xFFFFFFFFull
/*
* Table #68 of 220_CSR_lcb.xml - LCB_ERR_INFO_ESCAPE_0_PLUS1_CNT
* This register is used to report the total number of simultaneous poly 0 and 
* any of the other 3 polys (just 1) false positive 12 bit CRC checks in 48 bit 
* CRC mode when four lanes are active.
*/
#define DC_LCB_ERR_INFO_ESCAPE_0_PLUS1_CNT                            (DC_LCB_CSRS + 0x000000000370)
#define DC_LCB_ERR_INFO_ESCAPE_0_PLUS1_CNT_RESETCSR                   0x0000000000000000ull
#define DC_LCB_ERR_INFO_ESCAPE_0_PLUS1_CNT_UNUSED_63_32_SHIFT         32
#define DC_LCB_ERR_INFO_ESCAPE_0_PLUS1_CNT_UNUSED_63_32_MASK          0xFFFFFFFFull
#define DC_LCB_ERR_INFO_ESCAPE_0_PLUS1_CNT_UNUSED_63_32_SMASK         0xFFFFFFFF00000000ull
#define DC_LCB_ERR_INFO_ESCAPE_0_PLUS1_CNT_VAL_SHIFT                  0
#define DC_LCB_ERR_INFO_ESCAPE_0_PLUS1_CNT_VAL_MASK                   0xFFFFFFFFull
#define DC_LCB_ERR_INFO_ESCAPE_0_PLUS1_CNT_VAL_SMASK                  0xFFFFFFFFull
/*
* Table #69 of 220_CSR_lcb.xml - LCB_ERR_INFO_ESCAPE_0_PLUS2_CNT
* This register is used to report the total number of simultaneous poly 0 and 
* any two of the other 3 polys (just 2) false positive 12 bit CRC checks in 48 
* bit CRC mode when four lanes are active.
*/
#define DC_LCB_ERR_INFO_ESCAPE_0_PLUS2_CNT                            (DC_LCB_CSRS + 0x000000000378)
#define DC_LCB_ERR_INFO_ESCAPE_0_PLUS2_CNT_RESETCSR                   0x0000000000000000ull
#define DC_LCB_ERR_INFO_ESCAPE_0_PLUS2_CNT_UNUSED_63_16_SHIFT         16
#define DC_LCB_ERR_INFO_ESCAPE_0_PLUS2_CNT_UNUSED_63_16_MASK          0xFFFFFFFFFFFFull
#define DC_LCB_ERR_INFO_ESCAPE_0_PLUS2_CNT_UNUSED_63_16_SMASK         0xFFFFFFFFFFFF0000ull
#define DC_LCB_ERR_INFO_ESCAPE_0_PLUS2_CNT_VAL_SHIFT                  0
#define DC_LCB_ERR_INFO_ESCAPE_0_PLUS2_CNT_VAL_MASK                   0xFFFFull
#define DC_LCB_ERR_INFO_ESCAPE_0_PLUS2_CNT_VAL_SMASK                  0xFFFFull
/*
* Table #70 of 220_CSR_lcb.xml - LCB_ERR_INFO_REINIT_FROM_PEER_CNT
* This register is used to report the total number of reinit sequences triggered 
* from the peer.
*/
#define DC_LCB_ERR_INFO_REINIT_FROM_PEER_CNT                          (DC_LCB_CSRS + 0x000000000380)
#define DC_LCB_ERR_INFO_REINIT_FROM_PEER_CNT_RESETCSR                 0x0000000000000000ull
#define DC_LCB_ERR_INFO_REINIT_FROM_PEER_CNT_UNUSED_63_32_SHIFT       32
#define DC_LCB_ERR_INFO_REINIT_FROM_PEER_CNT_UNUSED_63_32_MASK        0xFFFFFFFFull
#define DC_LCB_ERR_INFO_REINIT_FROM_PEER_CNT_UNUSED_63_32_SMASK       0xFFFFFFFF00000000ull
#define DC_LCB_ERR_INFO_REINIT_FROM_PEER_CNT_VAL_SHIFT                0
#define DC_LCB_ERR_INFO_REINIT_FROM_PEER_CNT_VAL_MASK                 0xFFFFFFFFull
#define DC_LCB_ERR_INFO_REINIT_FROM_PEER_CNT_VAL_SMASK                0xFFFFFFFFull
/*
* Table #71 of 220_CSR_lcb.xml - LCB_ERR_INFO_SBE_CNT
* This register is used to count ECC SBE errors in the input buffer and replay 
* buffer.
*/
#define DC_LCB_ERR_INFO_SBE_CNT                                       (DC_LCB_CSRS + 0x000000000388)
#define DC_LCB_ERR_INFO_SBE_CNT_RESETCSR                              0x0000000000000000ull
#define DC_LCB_ERR_INFO_SBE_CNT_UNUSED_63_32_SHIFT                    32
#define DC_LCB_ERR_INFO_SBE_CNT_UNUSED_63_32_MASK                     0xFFFFFFFFull
#define DC_LCB_ERR_INFO_SBE_CNT_UNUSED_63_32_SMASK                    0xFFFFFFFF00000000ull
#define DC_LCB_ERR_INFO_SBE_CNT_REPLAY_BUFFER_SIDE_SHIFT              24
#define DC_LCB_ERR_INFO_SBE_CNT_REPLAY_BUFFER_SIDE_MASK               0xFFull
#define DC_LCB_ERR_INFO_SBE_CNT_REPLAY_BUFFER_SIDE_SMASK              0xFF000000ull
#define DC_LCB_ERR_INFO_SBE_CNT_REPLAY_BUFFER1_SHIFT                  16
#define DC_LCB_ERR_INFO_SBE_CNT_REPLAY_BUFFER1_MASK                   0xFFull
#define DC_LCB_ERR_INFO_SBE_CNT_REPLAY_BUFFER1_SMASK                  0xFF0000ull
#define DC_LCB_ERR_INFO_SBE_CNT_REPLAY_BUFFER0_SHIFT                  8
#define DC_LCB_ERR_INFO_SBE_CNT_REPLAY_BUFFER0_MASK                   0xFFull
#define DC_LCB_ERR_INFO_SBE_CNT_REPLAY_BUFFER0_SMASK                  0xFF00ull
#define DC_LCB_ERR_INFO_SBE_CNT_INPUT_BUFFER_SHIFT                    0
#define DC_LCB_ERR_INFO_SBE_CNT_INPUT_BUFFER_MASK                     0xFFull
#define DC_LCB_ERR_INFO_SBE_CNT_INPUT_BUFFER_SMASK                    0xFFull
/*
* Table #72 of 220_CSR_lcb.xml - LCB_ERR_INFO_MISC_FLG_CNT
* This register is used to count error flag events that are informational and 
* not necessarily catastrophic and would tend to occur only at very high BER. 
* The logic attempts to ride through these events.
*/
#define DC_LCB_ERR_INFO_MISC_FLG_CNT                                  (DC_LCB_CSRS + 0x000000000390)
#define DC_LCB_ERR_INFO_MISC_FLG_CNT_RESETCSR                         0x0000000000000000ull
#define DC_LCB_ERR_INFO_MISC_FLG_CNT_UNUSED_63_32_SHIFT               32
#define DC_LCB_ERR_INFO_MISC_FLG_CNT_UNUSED_63_32_MASK                0xFFFFFFFFull
#define DC_LCB_ERR_INFO_MISC_FLG_CNT_UNUSED_63_32_SMASK               0xFFFFFFFF00000000ull
#define DC_LCB_ERR_INFO_MISC_FLG_CNT_RST_FOR_INCOMPLT_RND_TRIP_SHIFT  24
#define DC_LCB_ERR_INFO_MISC_FLG_CNT_RST_FOR_INCOMPLT_RND_TRIP_MASK   0xFFull
#define DC_LCB_ERR_INFO_MISC_FLG_CNT_RST_FOR_INCOMPLT_RND_TRIP_SMASK  0xFF000000ull
#define DC_LCB_ERR_INFO_MISC_FLG_CNT_RST_FOR_LINK_TIMEOUT_SHIFT       16
#define DC_LCB_ERR_INFO_MISC_FLG_CNT_RST_FOR_LINK_TIMEOUT_MASK        0xFFull
#define DC_LCB_ERR_INFO_MISC_FLG_CNT_RST_FOR_LINK_TIMEOUT_SMASK       0xFF0000ull
#define DC_LCB_ERR_INFO_MISC_FLG_CNT_ALL_LNS_FAILED_REINIT_TEST_SHIFT 8
#define DC_LCB_ERR_INFO_MISC_FLG_CNT_ALL_LNS_FAILED_REINIT_TEST_MASK  0xFFull
#define DC_LCB_ERR_INFO_MISC_FLG_CNT_ALL_LNS_FAILED_REINIT_TEST_SMASK 0xFF00ull
#define DC_LCB_ERR_INFO_MISC_FLG_CNT_RST_FOR_FAILED_DESKEW_SHIFT      0
#define DC_LCB_ERR_INFO_MISC_FLG_CNT_RST_FOR_FAILED_DESKEW_MASK       0xFFull
#define DC_LCB_ERR_INFO_MISC_FLG_CNT_RST_FOR_FAILED_DESKEW_SMASK      0xFFull
/*
* Table #73 of 220_CSR_lcb.xml - LCB_ERR_INFO_ECC_INPUT_BUF_HGH
* This register is used to report ECC errors in the input buffer 
* RAMs.
*/
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_HGH                             (DC_LCB_CSRS + 0x000000000398)
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_HGH_RESETCSR                    0x0000000000000000ull
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_HGH_UNUSED_63_57_SHIFT          57
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_HGH_UNUSED_63_57_MASK           0x7Full
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_HGH_UNUSED_63_57_SMASK          0xFE00000000000000ull
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_HGH_CSR_CREATED_SHIFT           56
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_HGH_CSR_CREATED_MASK            0x1ull
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_HGH_CSR_CREATED_SMASK           0x100000000000000ull
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_HGH_UNUSED_55_54_SHIFT          54
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_HGH_UNUSED_55_54_MASK           0x3ull
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_HGH_UNUSED_55_54_SMASK          0xC0000000000000ull
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_HGH_LOCATION_SHIFT              52
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_HGH_LOCATION_MASK               0x3ull
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_HGH_LOCATION_SMASK              0x30000000000000ull
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_HGH_SYNDROME_SHIFT              44
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_HGH_SYNDROME_MASK               0xFFull
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_HGH_SYNDROME_SMASK              0xFF00000000000ull
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_HGH_CHK_SHIFT                   36
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_HGH_CHK_MASK                    0xFFull
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_HGH_CHK_SMASK                   0xFF000000000ull
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_HGH_UNUSED_35_33_SHIFT          33
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_HGH_UNUSED_35_33_MASK           0x7ull
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_HGH_UNUSED_35_33_SMASK          0xE00000000ull
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_HGH_DATA_SHIFT                  0
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_HGH_DATA_MASK                   0x1FFFFFFFFull
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_HGH_DATA_SMASK                  0x1FFFFFFFFull
/*
* Table #74 of 220_CSR_lcb.xml - LCB_ERR_INFO_ECC_INPUT_BUF_LOW
* This register is used to report ECC errors in the input buffer 
* RAMs.
*/
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_LOW                             (DC_LCB_CSRS + 0x0000000003A0)
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_LOW_RESETCSR                    0x0000000000000000ull
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_LOW_UNUSED_63_32_SHIFT          32
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_LOW_UNUSED_63_32_MASK           0xFFFFFFFFull
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_LOW_UNUSED_63_32_SMASK          0xFFFFFFFF00000000ull
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_LOW_DATA_SHIFT                  0
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_LOW_DATA_MASK                   0xFFFFFFFFull
#define DC_LCB_ERR_INFO_ECC_INPUT_BUF_LOW_DATA_SMASK                  0xFFFFFFFFull
/*
* Table #75 of 220_CSR_lcb.xml - LCB_ERR_INFO_ECC_REPLAY_BUF_HGH
* This register is used to report ECC errors in the replay buffer 
* RAMs.
*/
#define DC_LCB_ERR_INFO_ECC_REPLAY_BUF_HGH                            (DC_LCB_CSRS + 0x0000000003A8)
#define DC_LCB_ERR_INFO_ECC_REPLAY_BUF_HGH_RESETCSR                   0x0000000000000000ull
#define DC_LCB_ERR_INFO_ECC_REPLAY_BUF_HGH_UNUSED_63_57_SHIFT         57
#define DC_LCB_ERR_INFO_ECC_REPLAY_BUF_HGH_UNUSED_63_57_MASK          0x7Full
#define DC_LCB_ERR_INFO_ECC_REPLAY_BUF_HGH_UNUSED_63_57_SMASK         0xFE00000000000000ull
#define DC_LCB_ERR_INFO_ECC_REPLAY_BUF_HGH_CSR_CREATED_SHIFT          56
#define DC_LCB_ERR_INFO_ECC_REPLAY_BUF_HGH_CSR_CREATED_MASK           0x1ull
#define DC_LCB_ERR_INFO_ECC_REPLAY_BUF_HGH_CSR_CREATED_SMASK          0x100000000000000ull
#define DC_LCB_ERR_INFO_ECC_REPLAY_BUF_HGH_LOCATION_SHIFT             52
#define DC_LCB_ERR_INFO_ECC_REPLAY_BUF_HGH_LOCATION_MASK              0xFull
#define DC_LCB_ERR_INFO_ECC_REPLAY_BUF_HGH_LOCATION_SMASK             0xF0000000000000ull
#define DC_LCB_ERR_INFO_ECC_REPLAY_BUF_HGH_SYNDROME_SHIFT             44
#define DC_LCB_ERR_INFO_ECC_REPLAY_BUF_HGH_SYNDROME_MASK              0xFFull
#define DC_LCB_ERR_INFO_ECC_REPLAY_BUF_HGH_SYNDROME_SMASK             0xFF00000000000ull
#define DC_LCB_ERR_INFO_ECC_REPLAY_BUF_HGH_CHK_SHIFT                  36
#define DC_LCB_ERR_INFO_ECC_REPLAY_BUF_HGH_CHK_MASK                   0xFFull
#define DC_LCB_ERR_INFO_ECC_REPLAY_BUF_HGH_CHK_SMASK                  0xFF000000000ull
#define DC_LCB_ERR_INFO_ECC_REPLAY_BUF_HGH_UNUSED_35_33_SHIFT         33
#define DC_LCB_ERR_INFO_ECC_REPLAY_BUF_HGH_UNUSED_35_33_MASK          0x7ull
#define DC_LCB_ERR_INFO_ECC_REPLAY_BUF_HGH_UNUSED_35_33_SMASK         0xE00000000ull
#define DC_LCB_ERR_INFO_ECC_REPLAY_BUF_HGH_DATA_SHIFT                 0
#define DC_LCB_ERR_INFO_ECC_REPLAY_BUF_HGH_DATA_MASK                  0x1FFFFFFFFull
#define DC_LCB_ERR_INFO_ECC_REPLAY_BUF_HGH_DATA_SMASK                 0x1FFFFFFFFull
/*
* Table #76 of 220_CSR_lcb.xml - LCB_ERR_INFO_ECC_REPLAY_BUF_LOW
* This register is used to report ECC errors in the replay buffer 
* RAMs.
*/
#define DC_LCB_ERR_INFO_ECC_REPLAY_BUF_LOW                            (DC_LCB_CSRS + 0x0000000003B0)
#define DC_LCB_ERR_INFO_ECC_REPLAY_BUF_LOW_RESETCSR                   0x0000000000000000ull
#define DC_LCB_ERR_INFO_ECC_REPLAY_BUF_LOW_UNUSED_63_32_SHIFT         32
#define DC_LCB_ERR_INFO_ECC_REPLAY_BUF_LOW_UNUSED_63_32_MASK          0xFFFFFFFFull
#define DC_LCB_ERR_INFO_ECC_REPLAY_BUF_LOW_UNUSED_63_32_SMASK         0xFFFFFFFF00000000ull
#define DC_LCB_ERR_INFO_ECC_REPLAY_BUF_LOW_DATA_SHIFT                 0
#define DC_LCB_ERR_INFO_ECC_REPLAY_BUF_LOW_DATA_MASK                  0xFFFFFFFFull
#define DC_LCB_ERR_INFO_ECC_REPLAY_BUF_LOW_DATA_SMASK                 0xFFFFFFFFull
/*
* Table #77 of 220_CSR_lcb.xml - LCB_PRF_GOOD_LTP_CNT
* This register is used to report the LTP count with a good CRC. This will 
* assist with BER calculations. The TOTAL_CRC_ERR_CNT / (GOOD_LTP_CNT + 
* TOTAL_CRC_ERR_CNT) is the event error rate (EER). The BER will equal the 
* EER/1056 if there is only one bit error per event (CRC error), otherwise it is 
* larger.
*/
#define DC_LCB_PRF_GOOD_LTP_CNT                                       (DC_LCB_CSRS + 0x000000000400)
#define DC_LCB_PRF_GOOD_LTP_CNT_RESETCSR                              0x0000000000000000ull
#define DC_LCB_PRF_GOOD_LTP_CNT_VAL_SHIFT                             0
#define DC_LCB_PRF_GOOD_LTP_CNT_VAL_MASK                              0xFFFFFFFFFFFFFFFFull
#define DC_LCB_PRF_GOOD_LTP_CNT_VAL_SMASK                             0xFFFFFFFFFFFFFFFFull
/*
* Table #78 of 220_CSR_lcb.xml - LCB_PRF_ACCEPTED_LTP_CNT
* This register is used to report the LTP count with a good CRC accepted into 
* the receive buffer. This will assist with performance monitor calculations. 
* GOOD_LTP_CNT + TOTAL_CRC_ERR_CNT is the total LTP count. The ACCEPTED_LTP_CNT 
* is the number that carry real payload delivered to the core. Null LTPs (replay 
* buffer full due to a too long of a round trip time), LTPs with CRC errors, and 
* good but tossed (waiting for a retransmission to begin) LTPs are not 
* accepted.
*/
#define DC_LCB_PRF_ACCEPTED_LTP_CNT                                   (DC_LCB_CSRS + 0x000000000408)
#define DC_LCB_PRF_ACCEPTED_LTP_CNT_RESETCSR                          0x0000000000000000ull
#define DC_LCB_PRF_ACCEPTED_LTP_CNT_VAL_SHIFT                         0
#define DC_LCB_PRF_ACCEPTED_LTP_CNT_VAL_MASK                          0xFFFFFFFFFFFFFFFFull
#define DC_LCB_PRF_ACCEPTED_LTP_CNT_VAL_SMASK                         0xFFFFFFFFFFFFFFFFull
/*
* Table #79 of 220_CSR_lcb.xml - LCB_PRF_RX_FLIT_CNT
* This register counts the flits on the lower side of the output 
* bus.
*/
#define DC_LCB_PRF_RX_FLIT_CNT                                        (DC_LCB_CSRS + 0x000000000410)
#define DC_LCB_PRF_RX_FLIT_CNT_RESETCSR                               0x0000000000000000ull
#define DC_LCB_PRF_RX_FLIT_CNT_UNUSED_63_52_SHIFT                     52
#define DC_LCB_PRF_RX_FLIT_CNT_UNUSED_63_52_MASK                      0xFFFull
#define DC_LCB_PRF_RX_FLIT_CNT_UNUSED_63_52_SMASK                     0xFFF0000000000000ull
#define DC_LCB_PRF_RX_FLIT_CNT_VAL_SHIFT                              0
#define DC_LCB_PRF_RX_FLIT_CNT_VAL_MASK                               0xFFFFFFFFFFFFFull
#define DC_LCB_PRF_RX_FLIT_CNT_VAL_SMASK                              0xFFFFFFFFFFFFFull
/*
* Table #80 of 220_CSR_lcb.xml - LCB_PRF_TX_FLIT_CNT
* This register counts the flits on the lower side of the input 
* bus.
*/
#define DC_LCB_PRF_TX_FLIT_CNT                                        (DC_LCB_CSRS + 0x000000000418)
#define DC_LCB_PRF_TX_FLIT_CNT_RESETCSR                               0x0000000000000000ull
#define DC_LCB_PRF_TX_FLIT_CNT_UNUSED_63_52_SHIFT                     52
#define DC_LCB_PRF_TX_FLIT_CNT_UNUSED_63_52_MASK                      0xFFFull
#define DC_LCB_PRF_TX_FLIT_CNT_UNUSED_63_52_SMASK                     0xFFF0000000000000ull
#define DC_LCB_PRF_TX_FLIT_CNT_VAL_SHIFT                              0
#define DC_LCB_PRF_TX_FLIT_CNT_VAL_MASK                               0xFFFFFFFFFFFFFull
#define DC_LCB_PRF_TX_FLIT_CNT_VAL_SMASK                              0xFFFFFFFFFFFFFull
/*
* Table #81 of 220_CSR_lcb.xml - LCB_PRF_CLK_CNTR
* This register is used to report various clk counts. The clk being monitored is 
* set by CFG_CLK_CNTR.
*/
#define DC_LCB_PRF_CLK_CNTR                                           (DC_LCB_CSRS + 0x000000000420)
#define DC_LCB_PRF_CLK_CNTR_RESETCSR                                  0x0000000000000000ull
#define DC_LCB_PRF_CLK_CNTR_UNUSED_63_49_SHIFT                        49
#define DC_LCB_PRF_CLK_CNTR_UNUSED_63_49_MASK                         0x7FFFull
#define DC_LCB_PRF_CLK_CNTR_UNUSED_63_49_SMASK                        0xFFFE000000000000ull
#define DC_LCB_PRF_CLK_CNTR_NEW_VAL_WAS_STROBED_SHIFT                 48
#define DC_LCB_PRF_CLK_CNTR_NEW_VAL_WAS_STROBED_MASK                  0x1ull
#define DC_LCB_PRF_CLK_CNTR_NEW_VAL_WAS_STROBED_SMASK                 0x1000000000000ull
#define DC_LCB_PRF_CLK_CNTR_CNT_SHIFT                                 0
#define DC_LCB_PRF_CLK_CNTR_CNT_MASK                                  0xFFFFFFFFFFFFull
#define DC_LCB_PRF_CLK_CNTR_CNT_SMASK                                 0xFFFFFFFFFFFFull
/*
* Table #82 of 220_CSR_lcb.xml - LCB_STS_LINK_PAUSE_ACTIVE
* This will be set after a LCB_CFG_REINIT_PAUSE_TX_MODE and LCB_CFG_FORCE_RECOVER_SEQUENCE 
* is used. See also LCB_CFG_REINIT_AS_SLAVE. This does not report anything when 
* using only LCB_CFG_REINIT_PAUSE_RX_MODE.
*/
#define DC_LCB_STS_LINK_PAUSE_ACTIVE                                  (DC_LCB_CSRS + 0x000000000460)
#define DC_LCB_STS_LINK_PAUSE_ACTIVE_RESETCSR                         0x0000000000000000ull
#define DC_LCB_STS_LINK_PAUSE_ACTIVE_UNUSED_63_1_SHIFT                1
#define DC_LCB_STS_LINK_PAUSE_ACTIVE_UNUSED_63_1_MASK                 0x7FFFFFFFFFFFFFFFull
#define DC_LCB_STS_LINK_PAUSE_ACTIVE_UNUSED_63_1_SMASK                0xFFFFFFFFFFFFFFFEull
#define DC_LCB_STS_LINK_PAUSE_ACTIVE_VAL_SHIFT                        0
#define DC_LCB_STS_LINK_PAUSE_ACTIVE_VAL_MASK                         0x1ull
#define DC_LCB_STS_LINK_PAUSE_ACTIVE_VAL_SMASK                        0x1ull
/*
* Table #83 of 220_CSR_lcb.xml - LCB_STS_LINK_TRANSFER_ACTIVE
* This will be set if the link is up.
*/
#define DC_LCB_STS_LINK_TRANSFER_ACTIVE                               (DC_LCB_CSRS + 0x000000000468)
#define DC_LCB_STS_LINK_TRANSFER_ACTIVE_RESETCSR                      0x0000000000000000ull
#define DC_LCB_STS_LINK_TRANSFER_ACTIVE_UNUSED_63_1_SHIFT             1
#define DC_LCB_STS_LINK_TRANSFER_ACTIVE_UNUSED_63_1_MASK              0x7FFFFFFFFFFFFFFFull
#define DC_LCB_STS_LINK_TRANSFER_ACTIVE_UNUSED_63_1_SMASK             0xFFFFFFFFFFFFFFFEull
#define DC_LCB_STS_LINK_TRANSFER_ACTIVE_VAL_SHIFT                     0
#define DC_LCB_STS_LINK_TRANSFER_ACTIVE_VAL_MASK                      0x1ull
#define DC_LCB_STS_LINK_TRANSFER_ACTIVE_VAL_SMASK                     0x1ull
/*
* Table #84 of 220_CSR_lcb.xml - LCB_STS_LN_DEGRADE
* This is used to monitor the lane degrade logic.
*/
#define DC_LCB_STS_LN_DEGRADE                                         (DC_LCB_CSRS + 0x000000000470)
#define DC_LCB_STS_LN_DEGRADE_RESETCSR                                0x0000000000000000ull
#define DC_LCB_STS_LN_DEGRADE_CURRENT_CFG_RACES_WON_SHIFT             16
#define DC_LCB_STS_LN_DEGRADE_CURRENT_CFG_RACES_WON_MASK              0xFFFFFFFFFFFFull
#define DC_LCB_STS_LN_DEGRADE_CURRENT_CFG_RACES_WON_SMASK             0xFFFFFFFFFFFF0000ull
#define DC_LCB_STS_LN_DEGRADE_LN_123_RACES_WON_SHIFT                  13
#define DC_LCB_STS_LN_DEGRADE_LN_123_RACES_WON_MASK                   0x7ull
#define DC_LCB_STS_LN_DEGRADE_LN_123_RACES_WON_SMASK                  0xE000ull
#define DC_LCB_STS_LN_DEGRADE_LN_023_RACES_WON_SHIFT                  10
#define DC_LCB_STS_LN_DEGRADE_LN_023_RACES_WON_MASK                   0x7ull
#define DC_LCB_STS_LN_DEGRADE_LN_023_RACES_WON_SMASK                  0x1C00ull
#define DC_LCB_STS_LN_DEGRADE_LN_013_RACES_WON_SHIFT                  7
#define DC_LCB_STS_LN_DEGRADE_LN_013_RACES_WON_MASK                   0x7ull
#define DC_LCB_STS_LN_DEGRADE_LN_013_RACES_WON_SMASK                  0x380ull
#define DC_LCB_STS_LN_DEGRADE_LN_012_RACES_WON_SHIFT                  4
#define DC_LCB_STS_LN_DEGRADE_LN_012_RACES_WON_MASK                   0x7ull
#define DC_LCB_STS_LN_DEGRADE_LN_012_RACES_WON_SMASK                  0x70ull
#define DC_LCB_STS_LN_DEGRADE_UNUSED_3_SHIFT                          3
#define DC_LCB_STS_LN_DEGRADE_UNUSED_3_MASK                           0x1ull
#define DC_LCB_STS_LN_DEGRADE_UNUSED_3_SMASK                          0x8ull
#define DC_LCB_STS_LN_DEGRADE_CNT_SHIFT                               1
#define DC_LCB_STS_LN_DEGRADE_CNT_MASK                                0x3ull
#define DC_LCB_STS_LN_DEGRADE_CNT_SMASK                               0x6ull
#define DC_LCB_STS_LN_DEGRADE_EXHAUSTED_SHIFT                         0
#define DC_LCB_STS_LN_DEGRADE_EXHAUSTED_MASK                          0x1ull
#define DC_LCB_STS_LN_DEGRADE_EXHAUSTED_SMASK                         0x1ull
/*
* Table #85 of 220_CSR_lcb.xml - LCB_STS_TX_LTP_MODE
* This register is used to monitor the status of the Tx side LTP 
* mode.
*/
#define DC_LCB_STS_TX_LTP_MODE                                        (DC_LCB_CSRS + 0x000000000478)
#define DC_LCB_STS_TX_LTP_MODE_RESETCSR                               0x0000000000000000ull
#define DC_LCB_STS_TX_LTP_MODE_UNUSED_63_1_SHIFT                      1
#define DC_LCB_STS_TX_LTP_MODE_UNUSED_63_1_MASK                       0x7FFFFFFFFFFFFFFFull
#define DC_LCB_STS_TX_LTP_MODE_UNUSED_63_1_SMASK                      0xFFFFFFFFFFFFFFFEull
#define DC_LCB_STS_TX_LTP_MODE_VAL_SHIFT                              0
#define DC_LCB_STS_TX_LTP_MODE_VAL_MASK                               0x1ull
#define DC_LCB_STS_TX_LTP_MODE_VAL_SMASK                              0x1ull
/*
* Table #86 of 220_CSR_lcb.xml - LCB_STS_RX_LTP_MODE
* This register is used to monitor the status of the Rx side LTP 
* mode.
*/
#define DC_LCB_STS_RX_LTP_MODE                                        (DC_LCB_CSRS + 0x000000000480)
#define DC_LCB_STS_RX_LTP_MODE_RESETCSR                               0x0000000000000000ull
#define DC_LCB_STS_RX_LTP_MODE_UNUSED_63_1_SHIFT                      1
#define DC_LCB_STS_RX_LTP_MODE_UNUSED_63_1_MASK                       0x7FFFFFFFFFFFFFFFull
#define DC_LCB_STS_RX_LTP_MODE_UNUSED_63_1_SMASK                      0xFFFFFFFFFFFFFFFEull
#define DC_LCB_STS_RX_LTP_MODE_VAL_SHIFT                              0
#define DC_LCB_STS_RX_LTP_MODE_VAL_MASK                               0x1ull
#define DC_LCB_STS_RX_LTP_MODE_VAL_SMASK                              0x1ull
/*
* Table #87 of 220_CSR_lcb.xml - LCB_STS_RX_CRC_MODE
* This register is used to monitor the status of the Rx side CRC 
* mode.
*/
#define DC_LCB_STS_RX_CRC_MODE                                        (DC_LCB_CSRS + 0x000000000488)
#define DC_LCB_STS_RX_CRC_MODE_RESETCSR                               0x0000000000000000ull
#define DC_LCB_STS_RX_CRC_MODE_UNUSED_63_2_SHIFT                      2
#define DC_LCB_STS_RX_CRC_MODE_UNUSED_63_2_MASK                       0x3FFFFFFFFFFFFFFFull
#define DC_LCB_STS_RX_CRC_MODE_UNUSED_63_2_SMASK                      0xFFFFFFFFFFFFFFFCull
#define DC_LCB_STS_RX_CRC_MODE_VAL_SHIFT                              0
#define DC_LCB_STS_RX_CRC_MODE_VAL_MASK                               0x3ull
#define DC_LCB_STS_RX_CRC_MODE_VAL_SMASK                              0x3ull
/*
* Table #88 of 220_CSR_lcb.xml - LCB_STS_RX_LOGICAL_ID
* This register reports the Rx logical ID (Peers Tx physical ID) for each 
* lane.
*/
#define DC_LCB_STS_RX_LOGICAL_ID                                      (DC_LCB_CSRS + 0x000000000490)
#define DC_LCB_STS_RX_LOGICAL_ID_RESETCSR                             0x0000000000000000ull
#define DC_LCB_STS_RX_LOGICAL_ID_UNUSED_63_15_SHIFT                   15
#define DC_LCB_STS_RX_LOGICAL_ID_UNUSED_63_15_MASK                    0x1FFFFFFFFFFFFull
#define DC_LCB_STS_RX_LOGICAL_ID_UNUSED_63_15_SMASK                   0xFFFFFFFFFFFF8000ull
#define DC_LCB_STS_RX_LOGICAL_ID_LN3_SHIFT                            12
#define DC_LCB_STS_RX_LOGICAL_ID_LN3_MASK                             0x7ull
#define DC_LCB_STS_RX_LOGICAL_ID_LN3_SMASK                            0x7000ull
#define DC_LCB_STS_RX_LOGICAL_ID_UNUSED_11_SHIFT                      11
#define DC_LCB_STS_RX_LOGICAL_ID_UNUSED_11_MASK                       0x1ull
#define DC_LCB_STS_RX_LOGICAL_ID_UNUSED_11_SMASK                      0x800ull
#define DC_LCB_STS_RX_LOGICAL_ID_LN2_SHIFT                            8
#define DC_LCB_STS_RX_LOGICAL_ID_LN2_MASK                             0x7ull
#define DC_LCB_STS_RX_LOGICAL_ID_LN2_SMASK                            0x700ull
#define DC_LCB_STS_RX_LOGICAL_ID_UNUSED_7_SHIFT                       7
#define DC_LCB_STS_RX_LOGICAL_ID_UNUSED_7_MASK                        0x1ull
#define DC_LCB_STS_RX_LOGICAL_ID_UNUSED_7_SMASK                       0x80ull
#define DC_LCB_STS_RX_LOGICAL_ID_LN1_SHIFT                            4
#define DC_LCB_STS_RX_LOGICAL_ID_LN1_MASK                             0x7ull
#define DC_LCB_STS_RX_LOGICAL_ID_LN1_SMASK                            0x70ull
#define DC_LCB_STS_RX_LOGICAL_ID_UNUSED_3_SHIFT                       3
#define DC_LCB_STS_RX_LOGICAL_ID_UNUSED_3_MASK                        0x1ull
#define DC_LCB_STS_RX_LOGICAL_ID_UNUSED_3_SMASK                       0x8ull
#define DC_LCB_STS_RX_LOGICAL_ID_LN0_SHIFT                            0
#define DC_LCB_STS_RX_LOGICAL_ID_LN0_MASK                             0x7ull
#define DC_LCB_STS_RX_LOGICAL_ID_LN0_SMASK                            0x7ull
/*
* Table #89 of 220_CSR_lcb.xml - LCB_STS_RX_SHIFTED_LN_NUM
* This register reports the Rx logical ID (Peers Tx physical ID) for each 
* lane.
*/
#define DC_LCB_STS_RX_SHIFTED_LN_NUM                                  (DC_LCB_CSRS + 0x000000000498)
#define DC_LCB_STS_RX_SHIFTED_LN_NUM_RESETCSR                         0x0000000000000000ull
#define DC_LCB_STS_RX_SHIFTED_LN_NUM_UNUSED_63_15_SHIFT               15
#define DC_LCB_STS_RX_SHIFTED_LN_NUM_UNUSED_63_15_MASK                0x1FFFFFFFFFFFFull
#define DC_LCB_STS_RX_SHIFTED_LN_NUM_UNUSED_63_15_SMASK               0xFFFFFFFFFFFF8000ull
#define DC_LCB_STS_RX_SHIFTED_LN_NUM_LN3_SHIFT                        12
#define DC_LCB_STS_RX_SHIFTED_LN_NUM_LN3_MASK                         0x7ull
#define DC_LCB_STS_RX_SHIFTED_LN_NUM_LN3_SMASK                        0x7000ull
#define DC_LCB_STS_RX_SHIFTED_LN_NUM_UNUSED_11_SHIFT                  11
#define DC_LCB_STS_RX_SHIFTED_LN_NUM_UNUSED_11_MASK                   0x1ull
#define DC_LCB_STS_RX_SHIFTED_LN_NUM_UNUSED_11_SMASK                  0x800ull
#define DC_LCB_STS_RX_SHIFTED_LN_NUM_LN2_SHIFT                        8
#define DC_LCB_STS_RX_SHIFTED_LN_NUM_LN2_MASK                         0x7ull
#define DC_LCB_STS_RX_SHIFTED_LN_NUM_LN2_SMASK                        0x700ull
#define DC_LCB_STS_RX_SHIFTED_LN_NUM_UNUSED_7_SHIFT                   7
#define DC_LCB_STS_RX_SHIFTED_LN_NUM_UNUSED_7_MASK                    0x1ull
#define DC_LCB_STS_RX_SHIFTED_LN_NUM_UNUSED_7_SMASK                   0x80ull
#define DC_LCB_STS_RX_SHIFTED_LN_NUM_LN1_SHIFT                        4
#define DC_LCB_STS_RX_SHIFTED_LN_NUM_LN1_MASK                         0x7ull
#define DC_LCB_STS_RX_SHIFTED_LN_NUM_LN1_SMASK                        0x70ull
#define DC_LCB_STS_RX_SHIFTED_LN_NUM_UNUSED_3_SHIFT                   3
#define DC_LCB_STS_RX_SHIFTED_LN_NUM_UNUSED_3_MASK                    0x1ull
#define DC_LCB_STS_RX_SHIFTED_LN_NUM_UNUSED_3_SMASK                   0x8ull
#define DC_LCB_STS_RX_SHIFTED_LN_NUM_LN0_SHIFT                        0
#define DC_LCB_STS_RX_SHIFTED_LN_NUM_LN0_MASK                         0x7ull
#define DC_LCB_STS_RX_SHIFTED_LN_NUM_LN0_SMASK                        0x7ull
/*
* Table #90 of 220_CSR_lcb.xml - LCB_STS_RX_PHY_LN_EN
* This register reports the Rx side physical lane enable.
*/
#define DC_LCB_STS_RX_PHY_LN_EN                                       (DC_LCB_CSRS + 0x0000000004A0)
#define DC_LCB_STS_RX_PHY_LN_EN_RESETCSR                              0x0000000000000000ull
#define DC_LCB_STS_RX_PHY_LN_EN_UNUSED_63_12_SHIFT                    12
#define DC_LCB_STS_RX_PHY_LN_EN_UNUSED_63_12_MASK                     0xFFFFFFFFFFFFFull
#define DC_LCB_STS_RX_PHY_LN_EN_UNUSED_63_12_SMASK                    0xFFFFFFFFFFFFF000ull
#define DC_LCB_STS_RX_PHY_LN_EN_DEGRADE_SHIFT                         8
#define DC_LCB_STS_RX_PHY_LN_EN_DEGRADE_MASK                          0xFull
#define DC_LCB_STS_RX_PHY_LN_EN_DEGRADE_SMASK                         0xF00ull
#define DC_LCB_STS_RX_PHY_LN_EN_INIT_SHIFT                            4
#define DC_LCB_STS_RX_PHY_LN_EN_INIT_MASK                             0xFull
#define DC_LCB_STS_RX_PHY_LN_EN_INIT_SMASK                            0xF0ull
#define DC_LCB_STS_RX_PHY_LN_EN_COMBINED_SHIFT                        0
#define DC_LCB_STS_RX_PHY_LN_EN_COMBINED_MASK                         0xFull
#define DC_LCB_STS_RX_PHY_LN_EN_COMBINED_SMASK                        0xFull
/*
* Table #91 of 220_CSR_lcb.xml - LCB_STS_TX_PHY_LN_EN
* This register reports the Tx side physical lane enable.
*/
#define DC_LCB_STS_TX_PHY_LN_EN                                       (DC_LCB_CSRS + 0x0000000004A8)
#define DC_LCB_STS_TX_PHY_LN_EN_RESETCSR                              0x0000000000000000ull
#define DC_LCB_STS_TX_PHY_LN_EN_UNUSED_63_4_SHIFT                     4
#define DC_LCB_STS_TX_PHY_LN_EN_UNUSED_63_4_MASK                      0xFFFFFFFFFFFFFFFull
#define DC_LCB_STS_TX_PHY_LN_EN_UNUSED_63_4_SMASK                     0xFFFFFFFFFFFFFFF0ull
#define DC_LCB_STS_TX_PHY_LN_EN_VAL_SHIFT                             0
#define DC_LCB_STS_TX_PHY_LN_EN_VAL_MASK                              0xFull
#define DC_LCB_STS_TX_PHY_LN_EN_VAL_SMASK                             0xFull
/*
* Table #92 of 220_CSR_lcb.xml - LCB_STS_ROUND_TRIP_LTP_CNT
* This register reports the link round trip time in terms of a LTP transmit 
* time.
*/
#define DC_LCB_STS_ROUND_TRIP_LTP_CNT                                 (DC_LCB_CSRS + 0x0000000004B0)
#define DC_LCB_STS_ROUND_TRIP_LTP_CNT_RESETCSR                        0x0000000000000000ull
#define DC_LCB_STS_ROUND_TRIP_LTP_CNT_UNUSED_63_16_SHIFT              16
#define DC_LCB_STS_ROUND_TRIP_LTP_CNT_UNUSED_63_16_MASK               0xFFFFFFFFFFFFull
#define DC_LCB_STS_ROUND_TRIP_LTP_CNT_UNUSED_63_16_SMASK              0xFFFFFFFFFFFF0000ull
#define DC_LCB_STS_ROUND_TRIP_LTP_CNT_VAL_SHIFT                       0
#define DC_LCB_STS_ROUND_TRIP_LTP_CNT_VAL_MASK                        0xFFFFull
#define DC_LCB_STS_ROUND_TRIP_LTP_CNT_VAL_SMASK                       0xFFFFull
/*
* Table #93 of 220_CSR_lcb.xml - LCB_STS_NULLS_REQUIRED
* This register reports the number of null LTPs that are required before reusing 
* a replay buffer location. This is necessary to prevent replay buffer overrun 
* when the link round trip is larger than the depth of the replay 
* buffer.
*/
#define DC_LCB_STS_NULLS_REQUIRED                                     (DC_LCB_CSRS + 0x0000000004B8)
#define DC_LCB_STS_NULLS_REQUIRED_RESETCSR                            0x0000000000000000ull
#define DC_LCB_STS_NULLS_REQUIRED_UNUSED_63_16_SHIFT                  16
#define DC_LCB_STS_NULLS_REQUIRED_UNUSED_63_16_MASK                   0xFFFFFFFFFFFFull
#define DC_LCB_STS_NULLS_REQUIRED_UNUSED_63_16_SMASK                  0xFFFFFFFFFFFF0000ull
#define DC_LCB_STS_NULLS_REQUIRED_CNT_SHIFT                           0
#define DC_LCB_STS_NULLS_REQUIRED_CNT_MASK                            0xFFFFull
#define DC_LCB_STS_NULLS_REQUIRED_CNT_SMASK                           0xFFFFull
/*
* Table #94 of 220_CSR_lcb.xml - LCB_STS_FLIT_QUIET_CNTR
* This register reports the number of replay buffer cycles that have transpired 
* since the last valid flit was passed up to the core.
*/
#define DC_LCB_STS_FLIT_QUIET_CNTR                                    (DC_LCB_CSRS + 0x0000000004C0)
#define DC_LCB_STS_FLIT_QUIET_CNTR_RESETCSR                           0x0000000000000000ull
#define DC_LCB_STS_FLIT_QUIET_CNTR_UNUSED_63_3_SHIFT                  3
#define DC_LCB_STS_FLIT_QUIET_CNTR_UNUSED_63_3_MASK                   0x1FFFFFFFFFFFFFFFull
#define DC_LCB_STS_FLIT_QUIET_CNTR_UNUSED_63_3_SMASK                  0xFFFFFFFFFFFFFFF8ull
#define DC_LCB_STS_FLIT_QUIET_CNTR_VAL_SHIFT                          0
#define DC_LCB_STS_FLIT_QUIET_CNTR_VAL_MASK                           0x7ull
#define DC_LCB_STS_FLIT_QUIET_CNTR_VAL_SMASK                          0x7ull
/*
* Table #95 of 220_CSR_lcb.xml - LCB_STS_RCV_SMA_MSG
* This register is used by the subnet management agent (SMA) to receive a 
* message from the peer SMA. The SMA software protocol must provide a means to 
* indicate a new message has been received.
*/
#define DC_LCB_STS_RCV_SMA_MSG                                        (DC_LCB_CSRS + 0x0000000004C8)
#define DC_LCB_STS_RCV_SMA_MSG_RESETCSR                               0x0000000000000000ull
#define DC_LCB_STS_RCV_SMA_MSG_UNUSED_63_48_SHIFT                     48
#define DC_LCB_STS_RCV_SMA_MSG_UNUSED_63_48_MASK                      0xFFFFull
#define DC_LCB_STS_RCV_SMA_MSG_UNUSED_63_48_SMASK                     0xFFFF000000000000ull
#define DC_LCB_STS_RCV_SMA_MSG_VAL_SHIFT                              0
#define DC_LCB_STS_RCV_SMA_MSG_VAL_MASK                               0xFFFFFFFFFFFFull
#define DC_LCB_STS_RCV_SMA_MSG_VAL_SMASK                              0xFFFFFFFFFFFFull
/*
* Table #96 of 220_CSR_lcb.xml - LCB_PG_CFG_RUN
* This register steers the LCB input mux between the BIST pattern generator and 
* the real mission mode inputs.
*/
#define DC_LCB_PG_CFG_RUN                                             (DC_LCB_CSRS + 0x000000000500)
#define DC_LCB_PG_CFG_RUN_RESETCSR                                    0x0000000000000000ull
#define DC_LCB_PG_CFG_RUN_UNUSED_63_1_SHIFT                           1
#define DC_LCB_PG_CFG_RUN_UNUSED_63_1_MASK                            0x7FFFFFFFFFFFFFFFull
#define DC_LCB_PG_CFG_RUN_UNUSED_63_1_SMASK                           0xFFFFFFFFFFFFFFFEull
#define DC_LCB_PG_CFG_RUN_EN_SHIFT                                    0
#define DC_LCB_PG_CFG_RUN_EN_MASK                                     0x1ull
#define DC_LCB_PG_CFG_RUN_EN_SMASK                                    0x1ull
/*
* Table #97 of 220_CSR_lcb.xml - LCB_PG_CFG_RX_RUN
* The receive run enable register is the control for starting and stopping 
* receive side flit and virtual channel acknowledge generation. This bit must be 
* enabled whenever the transmit run bit is enabled in the pattern generator at 
* the other end of the link, otherwise miss compares will occur.
*/
#define DC_LCB_PG_CFG_RX_RUN                                          (DC_LCB_CSRS + 0x000000000508)
#define DC_LCB_PG_CFG_RX_RUN_RESETCSR                                 0x0000000000000000ull
#define DC_LCB_PG_CFG_RX_RUN_UNUSED_63_1_SHIFT                        1
#define DC_LCB_PG_CFG_RX_RUN_UNUSED_63_1_MASK                         0x7FFFFFFFFFFFFFFFull
#define DC_LCB_PG_CFG_RX_RUN_UNUSED_63_1_SMASK                        0xFFFFFFFFFFFFFFFEull
#define DC_LCB_PG_CFG_RX_RUN_EN_SHIFT                                 0
#define DC_LCB_PG_CFG_RX_RUN_EN_MASK                                  0x1ull
#define DC_LCB_PG_CFG_RX_RUN_EN_SMASK                                 0x1ull
/*
* Table #98 of 220_CSR_lcb.xml - LCB_PG_CFG_TX_RUN
* The transmit run enable register is the control for starting and stopping 
* transmit side flit and virtual channel acknowledge verification. This bit must 
* be enabled only when the receive run bit is enabled in the pattern generator 
* at the other end of the link, otherwise miss compares will occur.
*/
#define DC_LCB_PG_CFG_TX_RUN                                          (DC_LCB_CSRS + 0x000000000510)
#define DC_LCB_PG_CFG_TX_RUN_RESETCSR                                 0x0000000000000000ull
#define DC_LCB_PG_CFG_TX_RUN_UNUSED_63_1_SHIFT                        1
#define DC_LCB_PG_CFG_TX_RUN_UNUSED_63_1_MASK                         0x7FFFFFFFFFFFFFFFull
#define DC_LCB_PG_CFG_TX_RUN_UNUSED_63_1_SMASK                        0xFFFFFFFFFFFFFFFEull
#define DC_LCB_PG_CFG_TX_RUN_EN_SHIFT                                 0
#define DC_LCB_PG_CFG_TX_RUN_EN_MASK                                  0x1ull
#define DC_LCB_PG_CFG_TX_RUN_EN_SMASK                                 0x1ull
/*
* Table #99 of 220_CSR_lcb.xml - LCB_PG_CFG_CAPTURE_DELAY
* The capture delay register sets the length in flits between the occurrence of 
* a the miscompare error flag and the hold of the capture buffer.
*/
#define DC_LCB_PG_CFG_CAPTURE_DELAY                                   (DC_LCB_CSRS + 0x000000000518)
#define DC_LCB_PG_CFG_CAPTURE_DELAY_RESETCSR                          0x000000000000000Bull
#define DC_LCB_PG_CFG_CAPTURE_DELAY_UNUSED_63_4_SHIFT                 4
#define DC_LCB_PG_CFG_CAPTURE_DELAY_UNUSED_63_4_MASK                  0xFFFFFFFFFFFFFFFull
#define DC_LCB_PG_CFG_CAPTURE_DELAY_UNUSED_63_4_SMASK                 0xFFFFFFFFFFFFFFF0ull
#define DC_LCB_PG_CFG_CAPTURE_DELAY_VAL_SHIFT                         0
#define DC_LCB_PG_CFG_CAPTURE_DELAY_VAL_MASK                          0xFull
#define DC_LCB_PG_CFG_CAPTURE_DELAY_VAL_SMASK                         0xFull
/*
* Table #100 of 220_CSR_lcb.xml - LCB_PG_CFG_CAPTURE_RADR
* The register controls the capture RAMs read address.
*/
#define DC_LCB_PG_CFG_CAPTURE_RADR                                    (DC_LCB_CSRS + 0x000000000520)
#define DC_LCB_PG_CFG_CAPTURE_RADR_RESETCSR                           0x0000000000000000ull
#define DC_LCB_PG_CFG_CAPTURE_RADR_UNUSED_63_5_SHIFT                  5
#define DC_LCB_PG_CFG_CAPTURE_RADR_UNUSED_63_5_MASK                   0x7FFFFFFFFFFFFFFull
#define DC_LCB_PG_CFG_CAPTURE_RADR_UNUSED_63_5_SMASK                  0xFFFFFFFFFFFFFFE0ull
#define DC_LCB_PG_CFG_CAPTURE_RADR_EN_SHIFT                           4
#define DC_LCB_PG_CFG_CAPTURE_RADR_EN_MASK                            0x1ull
#define DC_LCB_PG_CFG_CAPTURE_RADR_EN_SMASK                           0x10ull
#define DC_LCB_PG_CFG_CAPTURE_RADR_VAL_SHIFT                          0
#define DC_LCB_PG_CFG_CAPTURE_RADR_VAL_MASK                           0xFull
#define DC_LCB_PG_CFG_CAPTURE_RADR_VAL_SMASK                          0xFull
/*
* Table #101 of 220_CSR_lcb.xml - LCB_PG_CFG_CRDTS_LCB
* The initial LCB credits register sets the number of LCB flit credits assumed 
* by the flit generator at reset. The LCB credits represents the space available 
* in the LCB input buffer. This value may be decreased to throttle the flit 
* generation rate or increased to explore LCB input buffer overflow.
*/
#define DC_LCB_PG_CFG_CRDTS_LCB                                       (DC_LCB_CSRS + 0x000000000528)
#define DC_LCB_PG_CFG_CRDTS_LCB_RESETCSR                              0x0000000000000010ull
#define DC_LCB_PG_CFG_CRDTS_LCB_UNUSED_63_8_SHIFT                     8
#define DC_LCB_PG_CFG_CRDTS_LCB_UNUSED_63_8_MASK                      0xFFFFFFFFFFFFFFull
#define DC_LCB_PG_CFG_CRDTS_LCB_UNUSED_63_8_SMASK                     0xFFFFFFFFFFFFFF00ull
#define DC_LCB_PG_CFG_CRDTS_LCB_VAL_SHIFT                             0
#define DC_LCB_PG_CFG_CRDTS_LCB_VAL_MASK                              0xFFull
#define DC_LCB_PG_CFG_CRDTS_LCB_VAL_SMASK                             0xFFull
/*
* Table #102 of 220_CSR_lcb.xml - LCB_PG_CFG_LEN
* This register controls how long testing occurs.
*/
#define DC_LCB_PG_CFG_LEN                                             (DC_LCB_CSRS + 0x000000000530)
#define DC_LCB_PG_CFG_LEN_RESETCSR                                    0x0240000000000000ull
#define DC_LCB_PG_CFG_LEN_UNUSED_63_58_SHIFT                          58
#define DC_LCB_PG_CFG_LEN_UNUSED_63_58_MASK                           0x3Full
#define DC_LCB_PG_CFG_LEN_UNUSED_63_58_SMASK                          0xFC00000000000000ull
#define DC_LCB_PG_CFG_LEN_PAUSE_MSK_SHIFT                             52
#define DC_LCB_PG_CFG_LEN_PAUSE_MSK_MASK                              0x3Full
#define DC_LCB_PG_CFG_LEN_PAUSE_MSK_SMASK                             0x3F0000000000000ull
#define DC_LCB_PG_CFG_LEN_VAL_SHIFT                                   0
#define DC_LCB_PG_CFG_LEN_VAL_MASK                                    0xFFFFFFFFFFFFFull
#define DC_LCB_PG_CFG_LEN_VAL_SMASK                                   0xFFFFFFFFFFFFFull
/*
* Table #103 of 220_CSR_lcb.xml - LCB_PG_CFG_SEED
* The random seed register sets the initial value of all PG LFSRs. Each 
* individual LFSR uniquely alters the seed value such that each LFSR creates a 
* unique sequence. The seed itself need not be random, this determines where in 
* the LFSR sequence the test starts. This is most useful during pre silicon 
* validation.
*/
#define DC_LCB_PG_CFG_SEED                                            (DC_LCB_CSRS + 0x000000000538)
#define DC_LCB_PG_CFG_SEED_RESETCSR                                   0x0000000000000000ull
#define DC_LCB_PG_CFG_SEED_UNUSED_63_55_SHIFT                         55
#define DC_LCB_PG_CFG_SEED_UNUSED_63_55_MASK                          0x1FFull
#define DC_LCB_PG_CFG_SEED_UNUSED_63_55_SMASK                         0xFF80000000000000ull
#define DC_LCB_PG_CFG_SEED_VAL_SHIFT                                  0
#define DC_LCB_PG_CFG_SEED_VAL_MASK                                   0x7FFFFFFFFFFFFFull
#define DC_LCB_PG_CFG_SEED_VAL_SMASK                                  0x7FFFFFFFFFFFFFull
/*
* Table #104 of 220_CSR_lcb.xml - LCB_PG_CFG_FLIT
* The flit parameters register controls the random selection of flit type and 
* content.
*/
#define DC_LCB_PG_CFG_FLIT                                            (DC_LCB_CSRS + 0x000000000540)
#define DC_LCB_PG_CFG_FLIT_RESETCSR                                   0x0000000000000000ull
#define DC_LCB_PG_CFG_FLIT_UNUSED_63_5_SHIFT                          5
#define DC_LCB_PG_CFG_FLIT_UNUSED_63_5_MASK                           0x7FFFFFFFFFFFFFFull
#define DC_LCB_PG_CFG_FLIT_UNUSED_63_5_SMASK                          0xFFFFFFFFFFFFFFE0ull
#define DC_LCB_PG_CFG_FLIT_ALLOW_UNEXP_TOSSED_FLITS_SHIFT             4
#define DC_LCB_PG_CFG_FLIT_ALLOW_UNEXP_TOSSED_FLITS_MASK              0x1ull
#define DC_LCB_PG_CFG_FLIT_ALLOW_UNEXP_TOSSED_FLITS_SMASK             0x10ull
#define DC_LCB_PG_CFG_FLIT_UNUSED_3_2_SHIFT                           2
#define DC_LCB_PG_CFG_FLIT_UNUSED_3_2_MASK                            0x3ull
#define DC_LCB_PG_CFG_FLIT_UNUSED_3_2_SMASK                           0xCull
#define DC_LCB_PG_CFG_FLIT_CONTENT_SHIFT                              0
#define DC_LCB_PG_CFG_FLIT_CONTENT_MASK                               0x3ull
#define DC_LCB_PG_CFG_FLIT_CONTENT_SMASK                              0x3ull
/*
* Table #105 of 220_CSR_lcb.xml - LCB_PG_CFG_IDLE_INJECT
* The idle injection register controls the rate at which idles are injected into 
* the transmitted flit stream. These idles are inserted before flit throttling 
* is taken into consideration. These idles are also the idles that can contain 
* single and double bit errors.
*/
#define DC_LCB_PG_CFG_IDLE_INJECT                                     (DC_LCB_CSRS + 0x000000000548)
#define DC_LCB_PG_CFG_IDLE_INJECT_RESETCSR                            0x0000000000002000ull
#define DC_LCB_PG_CFG_IDLE_INJECT_UNUSED_63_16_SHIFT                  16
#define DC_LCB_PG_CFG_IDLE_INJECT_UNUSED_63_16_MASK                   0xFFFFFFFFFFFFull
#define DC_LCB_PG_CFG_IDLE_INJECT_UNUSED_63_16_SMASK                  0xFFFFFFFFFFFF0000ull
#define DC_LCB_PG_CFG_IDLE_INJECT_THRESHOLD_SHIFT                     0
#define DC_LCB_PG_CFG_IDLE_INJECT_THRESHOLD_MASK                      0xFFFFull
#define DC_LCB_PG_CFG_IDLE_INJECT_THRESHOLD_SMASK                     0xFFFFull
/*
* Table #106 of 220_CSR_lcb.xml - LCB_PG_CFG_ERR
* The error parameters register controls the rate of SBEs and MBEs injected into 
* flits.
*/
#define DC_LCB_PG_CFG_ERR                                             (DC_LCB_CSRS + 0x000000000550)
#define DC_LCB_PG_CFG_ERR_RESETCSR                                    0x0001004000001000ull
#define DC_LCB_PG_CFG_ERR_UNUSED_63_49_SHIFT                          49
#define DC_LCB_PG_CFG_ERR_UNUSED_63_49_MASK                           0x7FFFull
#define DC_LCB_PG_CFG_ERR_UNUSED_63_49_SMASK                          0xFFFE000000000000ull
#define DC_LCB_PG_CFG_ERR_REALISTIC_SHIFT                             48
#define DC_LCB_PG_CFG_ERR_REALISTIC_MASK                              0x1ull
#define DC_LCB_PG_CFG_ERR_REALISTIC_SMASK                             0x1000000000000ull
#define DC_LCB_PG_CFG_ERR_MBE_THRESHOLD_SHIFT                         32
#define DC_LCB_PG_CFG_ERR_MBE_THRESHOLD_MASK                          0xFFFFull
#define DC_LCB_PG_CFG_ERR_MBE_THRESHOLD_SMASK                         0xFFFF00000000ull
#define DC_LCB_PG_CFG_ERR_SBE_THRESHOLD_SHIFT                         0
#define DC_LCB_PG_CFG_ERR_SBE_THRESHOLD_MASK                          0xFFFFFFFFull
#define DC_LCB_PG_CFG_ERR_SBE_THRESHOLD_SMASK                         0xFFFFFFFFull
/*
* Table #107 of 220_CSR_lcb.xml - LCB_PG_CFG_MISCOMPARE
* This register controls the enables for each miscompare type.
*/
#define DC_LCB_PG_CFG_MISCOMPARE                                      (DC_LCB_CSRS + 0x000000000558)
#define DC_LCB_PG_CFG_MISCOMPARE_RESETCSR                             0x0000000000000007ull
#define DC_LCB_PG_CFG_MISCOMPARE_UNUSED_63_3_SHIFT                    3
#define DC_LCB_PG_CFG_MISCOMPARE_UNUSED_63_3_MASK                     0x1FFFFFFFFFFFFFFFull
#define DC_LCB_PG_CFG_MISCOMPARE_UNUSED_63_3_SMASK                    0xFFFFFFFFFFFFFFF8ull
#define DC_LCB_PG_CFG_MISCOMPARE_EN_SHIFT                             0
#define DC_LCB_PG_CFG_MISCOMPARE_EN_MASK                              0x7ull
#define DC_LCB_PG_CFG_MISCOMPARE_EN_SMASK                             0x7ull
/*
* Table #108 of 220_CSR_lcb.xml - LCB_PG_CFG_PATTERN
* The transmit canned flit pattern register supplies the data pattern used when 
* canned data content is selected in the flit configuration register.
*/
#define DC_LCB_PG_CFG_PATTERN                                         (DC_LCB_CSRS + 0x000000000560)
#define DC_LCB_PG_CFG_PATTERN_RESETCSR                                0x0000000000000000ull
#define DC_LCB_PG_CFG_PATTERN_UNUSED_63_56_SHIFT                      56
#define DC_LCB_PG_CFG_PATTERN_UNUSED_63_56_MASK                       0xFFull
#define DC_LCB_PG_CFG_PATTERN_UNUSED_63_56_SMASK                      0xFF00000000000000ull
#define DC_LCB_PG_CFG_PATTERN_DATA_SHIFT                              0
#define DC_LCB_PG_CFG_PATTERN_DATA_MASK                               0xFFFFFFFFFFFFFFull
#define DC_LCB_PG_CFG_PATTERN_DATA_SMASK                              0xFFFFFFFFFFFFFFull
/*
* Table #109 of 220_CSR_lcb.xml - LCB_PG_DBG_FLIT_CRDTS_CNT
* This The LCB credit debug register contains the current number of LCB credits 
* in use waiting for acknowledgement. This represents the amount of the LCB 
* input FIFO in use.
*/
#define DC_LCB_PG_DBG_FLIT_CRDTS_CNT                                  (DC_LCB_CSRS + 0x000000000580)
#define DC_LCB_PG_DBG_FLIT_CRDTS_CNT_RESETCSR                         0x0000000000000000ull
#define DC_LCB_PG_DBG_FLIT_CRDTS_CNT_UNUSED_63_8_SHIFT                8
#define DC_LCB_PG_DBG_FLIT_CRDTS_CNT_UNUSED_63_8_MASK                 0xFFFFFFFFFFFFFFull
#define DC_LCB_PG_DBG_FLIT_CRDTS_CNT_UNUSED_63_8_SMASK                0xFFFFFFFFFFFFFF00ull
#define DC_LCB_PG_DBG_FLIT_CRDTS_CNT_VAL_SHIFT                        0
#define DC_LCB_PG_DBG_FLIT_CRDTS_CNT_VAL_MASK                         0xFFull
#define DC_LCB_PG_DBG_FLIT_CRDTS_CNT_VAL_SMASK                        0xFFull
/*
* Table #110 of 220_CSR_lcb.xml - LCB_PG_ERR_INFO_MISCOMPARE_CNT
* The miscompare count register reports the number of flits received that do not 
* match the expected flit.
*/
#define DC_LCB_PG_ERR_INFO_MISCOMPARE_CNT                             (DC_LCB_CSRS + 0x000000000588)
#define DC_LCB_PG_ERR_INFO_MISCOMPARE_CNT_RESETCSR                    0x0000000000000000ull
#define DC_LCB_PG_ERR_INFO_MISCOMPARE_CNT_UNUSED_63_8_SHIFT           8
#define DC_LCB_PG_ERR_INFO_MISCOMPARE_CNT_UNUSED_63_8_MASK            0xFFFFFFFFFFFFFFull
#define DC_LCB_PG_ERR_INFO_MISCOMPARE_CNT_UNUSED_63_8_SMASK           0xFFFFFFFFFFFFFF00ull
#define DC_LCB_PG_ERR_INFO_MISCOMPARE_CNT_VAL_SHIFT                   0
#define DC_LCB_PG_ERR_INFO_MISCOMPARE_CNT_VAL_MASK                    0xFFull
#define DC_LCB_PG_ERR_INFO_MISCOMPARE_CNT_VAL_SMASK                   0xFFull
/*
* Table #111 of 220_CSR_lcb.xml - LCB_PG_STS_CAPTURE_ACTUAL_DAT0
* This register is used to retrieve the data from the actual capture 
* RAM.
*/
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_DAT0                             (DC_LCB_CSRS + 0x0000000005C0)
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_DAT0_RESETCSR                    0x0000000000000000ull
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_DAT0_VAL_SHIFT                   0
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_DAT0_VAL_MASK                    0xFFFFFFFFFFFFFFFFull
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_DAT0_VAL_SMASK                   0xFFFFFFFFFFFFFFFFull
/*
* Table #112 of 220_CSR_lcb.xml - LCB_PG_STS_CAPTURE_ACTUAL_DAT1
* This register is used to retrieve the data from the actual capture 
* RAM.
*/
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_DAT1                             (DC_LCB_CSRS + 0x0000000005C8)
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_DAT1_RESETCSR                    0x0000000000000000ull
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_DAT1_VAL_SHIFT                   0
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_DAT1_VAL_MASK                    0xFFFFFFFFFFFFFFFFull
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_DAT1_VAL_SMASK                   0xFFFFFFFFFFFFFFFFull
/*
* Table #113 of 220_CSR_lcb.xml - LCB_PG_STS_CAPTURE_ACTUAL_MISC
* This register is used to retrieve the data from the actual capture 
* RAM.
*/
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_MISC                             (DC_LCB_CSRS + 0x0000000005D0)
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_MISC_RESETCSR                    0x0000000000000000ull
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_MISC_UNUSED_63_24_SHIFT          24
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_MISC_UNUSED_63_24_MASK           0xFFFFFFFFFFull
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_MISC_UNUSED_63_24_SMASK          0xFFFFFFFFFF000000ull
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_MISC_WADR_SHIFT                  20
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_MISC_WADR_MASK                   0xFull
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_MISC_WADR_SMASK                  0xF00000ull
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_MISC_UNUSED_19_SHIFT             19
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_MISC_UNUSED_19_MASK              0x1ull
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_MISC_UNUSED_19_SMASK             0x80000ull
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_MISC_MISCOMPARE_SHIFT            16
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_MISC_MISCOMPARE_MASK             0x7ull
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_MISC_MISCOMPARE_SMASK            0x70000ull
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_MISC_UNUSED_15_14_SHIFT          14
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_MISC_UNUSED_15_14_MASK           0x3ull
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_MISC_UNUSED_15_14_SMASK          0xC000ull
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_MISC_VL_ACK_SHIFT                4
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_MISC_VL_ACK_MASK                 0x3FFull
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_MISC_VL_ACK_SMASK                0x3FF0ull
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_MISC_UNUSED_3_2_SHIFT            2
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_MISC_UNUSED_3_2_MASK             0x3ull
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_MISC_UNUSED_3_2_SMASK            0xCull
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_MISC_B64_1_SHIFT                 1
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_MISC_B64_1_MASK                  0x1ull
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_MISC_B64_1_SMASK                 0x2ull
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_MISC_B64_0_SHIFT                 0
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_MISC_B64_0_MASK                  0x1ull
#define DC_LCB_PG_STS_CAPTURE_ACTUAL_MISC_B64_0_SMASK                 0x1ull
/*
* Table #114 of 220_CSR_lcb.xml - LCB_PG_STS_CAPTURE_EXPECT_DAT0
* This register is used to retrieve the data from the expect capture 
* RAM.
*/
#define DC_LCB_PG_STS_CAPTURE_EXPECT_DAT0                             (DC_LCB_CSRS + 0x0000000005D8)
#define DC_LCB_PG_STS_CAPTURE_EXPECT_DAT0_RESETCSR                    0x0000000000000000ull
#define DC_LCB_PG_STS_CAPTURE_EXPECT_DAT0_VAL_SHIFT                   0
#define DC_LCB_PG_STS_CAPTURE_EXPECT_DAT0_VAL_MASK                    0xFFFFFFFFFFFFFFFFull
#define DC_LCB_PG_STS_CAPTURE_EXPECT_DAT0_VAL_SMASK                   0xFFFFFFFFFFFFFFFFull
/*
* Table #115 of 220_CSR_lcb.xml - LCB_PG_STS_CAPTURE_EXPECT_DAT1
* This register is used to retrieve the data from the expect capture 
* RAM.
*/
#define DC_LCB_PG_STS_CAPTURE_EXPECT_DAT1                             (DC_LCB_CSRS + 0x0000000005E0)
#define DC_LCB_PG_STS_CAPTURE_EXPECT_DAT1_RESETCSR                    0x0000000000000000ull
#define DC_LCB_PG_STS_CAPTURE_EXPECT_DAT1_VAL_SHIFT                   0
#define DC_LCB_PG_STS_CAPTURE_EXPECT_DAT1_VAL_MASK                    0xFFFFFFFFFFFFFFFFull
#define DC_LCB_PG_STS_CAPTURE_EXPECT_DAT1_VAL_SMASK                   0xFFFFFFFFFFFFFFFFull
/*
* Table #116 of 220_CSR_lcb.xml - LCB_PG_STS_CAPTURE_EXPECT_MISC
* This register is used to retrieve the data from the expect capture 
* RAM.
*/
#define DC_LCB_PG_STS_CAPTURE_EXPECT_MISC                             (DC_LCB_CSRS + 0x0000000005E8)
#define DC_LCB_PG_STS_CAPTURE_EXPECT_MISC_RESETCSR                    0x0000000000000000ull
#define DC_LCB_PG_STS_CAPTURE_EXPECT_MISC_UNUSED_63_12_SHIFT          12
#define DC_LCB_PG_STS_CAPTURE_EXPECT_MISC_UNUSED_63_12_MASK           0xFFFFFFFFFFFFFull
#define DC_LCB_PG_STS_CAPTURE_EXPECT_MISC_UNUSED_63_12_SMASK          0xFFFFFFFFFFFFF000ull
#define DC_LCB_PG_STS_CAPTURE_EXPECT_MISC_VL_ACK_SHIFT                4
#define DC_LCB_PG_STS_CAPTURE_EXPECT_MISC_VL_ACK_MASK                 0xFFull
#define DC_LCB_PG_STS_CAPTURE_EXPECT_MISC_VL_ACK_SMASK                0xFF0ull
#define DC_LCB_PG_STS_CAPTURE_EXPECT_MISC_UNUSED_3_2_SHIFT            2
#define DC_LCB_PG_STS_CAPTURE_EXPECT_MISC_UNUSED_3_2_MASK             0x3ull
#define DC_LCB_PG_STS_CAPTURE_EXPECT_MISC_UNUSED_3_2_SMASK            0xCull
#define DC_LCB_PG_STS_CAPTURE_EXPECT_MISC_B64_1_SHIFT                 1
#define DC_LCB_PG_STS_CAPTURE_EXPECT_MISC_B64_1_MASK                  0x1ull
#define DC_LCB_PG_STS_CAPTURE_EXPECT_MISC_B64_1_SMASK                 0x2ull
#define DC_LCB_PG_STS_CAPTURE_EXPECT_MISC_B64_0_SHIFT                 0
#define DC_LCB_PG_STS_CAPTURE_EXPECT_MISC_B64_0_MASK                  0x1ull
#define DC_LCB_PG_STS_CAPTURE_EXPECT_MISC_B64_0_SMASK                 0x1ull
/*
* Table #117 of 220_CSR_lcb.xml - LCB_PG_STS_FLG
* The PG status flags register monitors for errors and the first occurrence of 
* some interesting conditions.
*/
#define DC_LCB_PG_STS_FLG                                             (DC_LCB_CSRS + 0x0000000005F0)
#define DC_LCB_PG_STS_FLG_RESETCSR                                    0x0000000000000000ull
#define DC_LCB_PG_STS_FLG_UNUSED_63_25_SHIFT                          25
#define DC_LCB_PG_STS_FLG_UNUSED_63_25_MASK                           0x7FFFFFFFFFull
#define DC_LCB_PG_STS_FLG_UNUSED_63_25_SMASK                          0xFFFFFFFFFE000000ull
#define DC_LCB_PG_STS_FLG_EXTRA_FLIT_ACK_SHIFT                        24
#define DC_LCB_PG_STS_FLG_EXTRA_FLIT_ACK_MASK                         0x1ull
#define DC_LCB_PG_STS_FLG_EXTRA_FLIT_ACK_SMASK                        0x1000000ull
#define DC_LCB_PG_STS_FLG_UNUSED_23_21_SHIFT                          21
#define DC_LCB_PG_STS_FLG_UNUSED_23_21_MASK                           0x7ull
#define DC_LCB_PG_STS_FLG_UNUSED_23_21_SMASK                          0xE00000ull
#define DC_LCB_PG_STS_FLG_LOST_FLIT_ACK_SHIFT                         20
#define DC_LCB_PG_STS_FLG_LOST_FLIT_ACK_MASK                          0x1ull
#define DC_LCB_PG_STS_FLG_LOST_FLIT_ACK_SMASK                         0x100000ull
#define DC_LCB_PG_STS_FLG_UNUSED_19_SHIFT                             19
#define DC_LCB_PG_STS_FLG_UNUSED_19_MASK                              0x1ull
#define DC_LCB_PG_STS_FLG_UNUSED_19_SMASK                             0x80000ull
#define DC_LCB_PG_STS_FLG_MISCOMPARE_SHIFT                            16
#define DC_LCB_PG_STS_FLG_MISCOMPARE_MASK                             0x7ull
#define DC_LCB_PG_STS_FLG_MISCOMPARE_SMASK                            0x70000ull
#define DC_LCB_PG_STS_FLG_UNUSED_15_13_SHIFT                          13
#define DC_LCB_PG_STS_FLG_UNUSED_15_13_MASK                           0x7ull
#define DC_LCB_PG_STS_FLG_UNUSED_15_13_SMASK                          0xE000ull
#define DC_LCB_PG_STS_FLG_OUT_OF_FLIT_CRDTS_SHIFT                     12
#define DC_LCB_PG_STS_FLG_OUT_OF_FLIT_CRDTS_MASK                      0x1ull
#define DC_LCB_PG_STS_FLG_OUT_OF_FLIT_CRDTS_SMASK                     0x1000ull
#define DC_LCB_PG_STS_FLG_UNUSED_11_9_SHIFT                           9
#define DC_LCB_PG_STS_FLG_UNUSED_11_9_MASK                            0x7ull
#define DC_LCB_PG_STS_FLG_UNUSED_11_9_SMASK                           0xE00ull
#define DC_LCB_PG_STS_FLG_TX_END_OF_TEST_SHIFT                        8
#define DC_LCB_PG_STS_FLG_TX_END_OF_TEST_MASK                         0x1ull
#define DC_LCB_PG_STS_FLG_TX_END_OF_TEST_SMASK                        0x100ull
#define DC_LCB_PG_STS_FLG_UNUSED_7_5_SHIFT                            5
#define DC_LCB_PG_STS_FLG_UNUSED_7_5_MASK                             0x7ull
#define DC_LCB_PG_STS_FLG_UNUSED_7_5_SMASK                            0xE0ull
#define DC_LCB_PG_STS_FLG_TX_RX_COMPLETE_SHIFT                        4
#define DC_LCB_PG_STS_FLG_TX_RX_COMPLETE_MASK                         0x1ull
#define DC_LCB_PG_STS_FLG_TX_RX_COMPLETE_SMASK                        0x10ull
#define DC_LCB_PG_STS_FLG_UNUSED_3_0_SHIFT                            0
#define DC_LCB_PG_STS_FLG_UNUSED_3_0_MASK                             0xFull
#define DC_LCB_PG_STS_FLG_UNUSED_3_0_SMASK                            0xFull
/*
* Table #118 of 220_CSR_lcb.xml - LCB_PG_STS_PAUSE_COMPLETE_CNT
* The transmit pause complete count register reports the number of successful 
* pause cycles.
*/
#define DC_LCB_PG_STS_PAUSE_COMPLETE_CNT                              (DC_LCB_CSRS + 0x0000000005F8)
#define DC_LCB_PG_STS_PAUSE_COMPLETE_CNT_RESETCSR                     0x0000000000000000ull
#define DC_LCB_PG_STS_PAUSE_COMPLETE_CNT_UNUSED_63_16_SHIFT           16
#define DC_LCB_PG_STS_PAUSE_COMPLETE_CNT_UNUSED_63_16_MASK            0xFFFFFFFFFFFFull
#define DC_LCB_PG_STS_PAUSE_COMPLETE_CNT_UNUSED_63_16_SMASK           0xFFFFFFFFFFFF0000ull
#define DC_LCB_PG_STS_PAUSE_COMPLETE_CNT_VAL_SHIFT                    0
#define DC_LCB_PG_STS_PAUSE_COMPLETE_CNT_VAL_MASK                     0xFFFFull
#define DC_LCB_PG_STS_PAUSE_COMPLETE_CNT_VAL_SMASK                    0xFFFFull
/*
* Table #119 of 220_CSR_lcb.xml - LCB_PG_STS_TX_SBE_CNT
* The transmit single bit error count register reports the number of flits sent 
* to the LCB with a single bit error.
*/
#define DC_LCB_PG_STS_TX_SBE_CNT                                      (DC_LCB_CSRS + 0x000000000600)
#define DC_LCB_PG_STS_TX_SBE_CNT_RESETCSR                             0x0000000000000000ull
#define DC_LCB_PG_STS_TX_SBE_CNT_UNUSED_63_16_SHIFT                   16
#define DC_LCB_PG_STS_TX_SBE_CNT_UNUSED_63_16_MASK                    0xFFFFFFFFFFFFull
#define DC_LCB_PG_STS_TX_SBE_CNT_UNUSED_63_16_SMASK                   0xFFFFFFFFFFFF0000ull
#define DC_LCB_PG_STS_TX_SBE_CNT_VAL_SHIFT                            0
#define DC_LCB_PG_STS_TX_SBE_CNT_VAL_MASK                             0xFFFFull
#define DC_LCB_PG_STS_TX_SBE_CNT_VAL_SMASK                            0xFFFFull
/*
* Table #120 of 220_CSR_lcb.xml - LCB_PG_STS_TX_MBE_CNT
* The transmit multiple bit error count register reports the number of flits 
* sent to the LCB with a multiple bit error.
*/
#define DC_LCB_PG_STS_TX_MBE_CNT                                      (DC_LCB_CSRS + 0x000000000608)
#define DC_LCB_PG_STS_TX_MBE_CNT_RESETCSR                             0x0000000000000000ull
#define DC_LCB_PG_STS_TX_MBE_CNT_UNUSED_63_16_SHIFT                   16
#define DC_LCB_PG_STS_TX_MBE_CNT_UNUSED_63_16_MASK                    0xFFFFFFFFFFFFull
#define DC_LCB_PG_STS_TX_MBE_CNT_UNUSED_63_16_SMASK                   0xFFFFFFFFFFFF0000ull
#define DC_LCB_PG_STS_TX_MBE_CNT_VAL_SHIFT                            0
#define DC_LCB_PG_STS_TX_MBE_CNT_VAL_MASK                             0xFFFFull
#define DC_LCB_PG_STS_TX_MBE_CNT_VAL_SMASK                            0xFFFFull
