/*
* Table #2 of 240_CSR_pcslpe.xml - pcs_66_reg_0
* See description below for individual field definition
*/
#define DC_PCS_66_REG_0                                  (DC_PCSLPE_PCS_66 + 0x000000000000)
#define DC_PCS_66_REG_0_RESETCSR                         0x00000000ull
#define DC_PCS_66_REG_0_UNUSED_31_0_SHIFT                0
#define DC_PCS_66_REG_0_UNUSED_31_0_MASK                 0xFFFFFFFFull
#define DC_PCS_66_REG_0_UNUSED_31_0_SMASK                0xFFFFFFFFull
/*
* Table #3 of 240_CSR_pcslpe.xml - pcs_66_reg_1
* See description below for individual field definition
*/
#define DC_PCS_66_REG_1                                  (DC_PCSLPE_PCS_66 + 0x000000000008)
#define DC_PCS_66_REG_1_RESETCSR                         0x00000000ull
#define DC_PCS_66_REG_1_UNUSED_31_12_SHIFT               12
#define DC_PCS_66_REG_1_UNUSED_31_12_MASK                0xFFFFFull
#define DC_PCS_66_REG_1_UNUSED_31_12_SMASK               0xFFFFF000ull
#define DC_PCS_66_REG_1_PRBS_ERROR_SHIFT                 8
#define DC_PCS_66_REG_1_PRBS_ERROR_MASK                  0xFull
#define DC_PCS_66_REG_1_PRBS_ERROR_SMASK                 0xF00ull
#define DC_PCS_66_REG_1_UNUSED_7_4_SHIFT                 4
#define DC_PCS_66_REG_1_UNUSED_7_4_MASK                  0xFull
#define DC_PCS_66_REG_1_UNUSED_7_4_SMASK                 0xF0ull
#define DC_PCS_66_REG_1_SMF_OVERFLOW_SHIFT               0
#define DC_PCS_66_REG_1_SMF_OVERFLOW_MASK                0xFull
#define DC_PCS_66_REG_1_SMF_OVERFLOW_SMASK               0xFull
/*
* Table #4 of 240_CSR_pcslpe.xml - pcs_66_reg_2
* See description below for individual field definition
*/
#define DC_PCS_66_REG_2                                  (DC_PCSLPE_PCS_66 + 0x000000000010)
#define DC_PCS_66_REG_2_RESETCSR                         0x00000E40ull
#define DC_PCS_66_REG_2_UNUSED_31_12_SHIFT               12
#define DC_PCS_66_REG_2_UNUSED_31_12_MASK                0xFFFFFull
#define DC_PCS_66_REG_2_UNUSED_31_12_SMASK               0xFFFFF000ull
#define DC_PCS_66_REG_2_TX_LANE_ORDER_D_SHIFT            10
#define DC_PCS_66_REG_2_TX_LANE_ORDER_D_MASK             0x3ull
#define DC_PCS_66_REG_2_TX_LANE_ORDER_D_SMASK            0xC00ull
#define DC_PCS_66_REG_2_TX_LANE_ORDER_C_SHIFT            8
#define DC_PCS_66_REG_2_TX_LANE_ORDER_C_MASK             0x3ull
#define DC_PCS_66_REG_2_TX_LANE_ORDER_C_SMASK            0x300ull
#define DC_PCS_66_REG_2_TX_LANE_ORDER_B_SHIFT            6
#define DC_PCS_66_REG_2_TX_LANE_ORDER_B_MASK             0x3ull
#define DC_PCS_66_REG_2_TX_LANE_ORDER_B_SMASK            0xC0ull
#define DC_PCS_66_REG_2_TX_LANE_ORDER_A_SHIFT            4
#define DC_PCS_66_REG_2_TX_LANE_ORDER_A_MASK             0x3ull
#define DC_PCS_66_REG_2_TX_LANE_ORDER_A_SMASK            0x30ull
#define DC_PCS_66_REG_2_TX_LANE_POLARITY_SHIFT           0
#define DC_PCS_66_REG_2_TX_LANE_POLARITY_MASK            0xFull
#define DC_PCS_66_REG_2_TX_LANE_POLARITY_SMASK           0xFull
/*
* Table #5 of 240_CSR_pcslpe.xml - pcs_66_reg_3
* See description below for individual field definition
*/
#define DC_PCS_66_REG_3                                  (DC_PCSLPE_PCS_66 + 0x000000000018)
#define DC_PCS_66_REG_3_RESETCSR                         0x00000000ull
#define DC_PCS_66_REG_3_UNUSED_31_4_SHIFT                4
#define DC_PCS_66_REG_3_UNUSED_31_4_MASK                 0xFFFFFFFull
#define DC_PCS_66_REG_3_UNUSED_31_4_SMASK                0xFFFFFFF0ull
#define DC_PCS_66_REG_3_RX_LOCK_SHIFT                    0
#define DC_PCS_66_REG_3_RX_LOCK_MASK                     0xFull
#define DC_PCS_66_REG_3_RX_LOCK_SMASK                    0xFull
/*
* Table #6 of 240_CSR_pcslpe.xml - pcs_66_reg_4
* See description below for individual field definition
*/
#define DC_PCS_66_REG_4                                  (DC_PCSLPE_PCS_66 + 0x000000000020)
#define DC_PCS_66_REG_4_RESETCSR                         0x00000100ull
#define DC_PCS_66_REG_4_UNUSED_31_9_SHIFT                9
#define DC_PCS_66_REG_4_UNUSED_31_9_MASK                 0x7FFFFFull
#define DC_PCS_66_REG_4_UNUSED_31_9_SMASK                0xFFFFFE00ull
#define DC_PCS_66_REG_4_AUTO_POLARITY_REV_EN_SHIFT       8
#define DC_PCS_66_REG_4_AUTO_POLARITY_REV_EN_MASK        0x1ull
#define DC_PCS_66_REG_4_AUTO_POLARITY_REV_EN_SMASK       0x100ull
#define DC_PCS_66_REG_4_UNUSED_7_2_SHIFT                 2
#define DC_PCS_66_REG_4_UNUSED_7_2_MASK                  0x3Full
#define DC_PCS_66_REG_4_UNUSED_7_2_SMASK                 0xFCull
#define DC_PCS_66_REG_4_PCS_DATA_SYNC_DIS_SHIFT          1
#define DC_PCS_66_REG_4_PCS_DATA_SYNC_DIS_MASK           0x1ull
#define DC_PCS_66_REG_4_PCS_DATA_SYNC_DIS_SMASK          0x2ull
#define DC_PCS_66_REG_4_UNUSED_0_SHIFT                   0
#define DC_PCS_66_REG_4_UNUSED_0_MASK                    0x1ull
#define DC_PCS_66_REG_4_UNUSED_0_SMASK                   0x1ull
/*
* Table #7 of 240_CSR_pcslpe.xml - pcs_66_reg_5
* See description below for individual field definition
*/
#define DC_PCS_66_REG_5                                  (DC_PCSLPE_PCS_66 + 0x000000000028)
#define DC_PCS_66_REG_5_RESETCSR                         0x00000E40ull
#define DC_PCS_66_REG_5_UNUSED_31_12_SHIFT               12
#define DC_PCS_66_REG_5_UNUSED_31_12_MASK                0xFFFFFull
#define DC_PCS_66_REG_5_UNUSED_31_12_SMASK               0xFFFFF000ull
#define DC_PCS_66_REG_5_RX_LANE_ORDER_D_SHIFT            10
#define DC_PCS_66_REG_5_RX_LANE_ORDER_D_MASK             0x3ull
#define DC_PCS_66_REG_5_RX_LANE_ORDER_D_SMASK            0xC00ull
#define DC_PCS_66_REG_5_RX_LANE_ORDER_C_SHIFT            8
#define DC_PCS_66_REG_5_RX_LANE_ORDER_C_MASK             0x3ull
#define DC_PCS_66_REG_5_RX_LANE_ORDER_C_SMASK            0x300ull
#define DC_PCS_66_REG_5_RX_LANE_ORDER_B_SHIFT            6
#define DC_PCS_66_REG_5_RX_LANE_ORDER_B_MASK             0x3ull
#define DC_PCS_66_REG_5_RX_LANE_ORDER_B_SMASK            0xC0ull
#define DC_PCS_66_REG_5_RX_LANE_ORDER_A_SHIFT            4
#define DC_PCS_66_REG_5_RX_LANE_ORDER_A_MASK             0x3ull
#define DC_PCS_66_REG_5_RX_LANE_ORDER_A_SMASK            0x30ull
#define DC_PCS_66_REG_5_RX_LANE_POLARITY_SHIFT           0
#define DC_PCS_66_REG_5_RX_LANE_POLARITY_MASK            0xFull
#define DC_PCS_66_REG_5_RX_LANE_POLARITY_SMASK           0xFull
/*
* Table #8 of 240_CSR_pcslpe.xml - pcs_66_reg_6
* See description below for individual field definition
*/
#define DC_PCS_66_REG_6                                  (DC_PCSLPE_PCS_66 + 0x000000000030)
#define DC_PCS_66_REG_6_RESETCSR                         0x00000000ull
#define DC_PCS_66_REG_6_UNUSED_31_3_SHIFT                3
#define DC_PCS_66_REG_6_UNUSED_31_3_MASK                 0x1FFFFFFFull
#define DC_PCS_66_REG_6_UNUSED_31_3_SMASK                0xFFFFFFF8ull
#define DC_PCS_66_REG_6_PRBS_POLARITY_SHIFT              2
#define DC_PCS_66_REG_6_PRBS_POLARITY_MASK               0x1ull
#define DC_PCS_66_REG_6_PRBS_POLARITY_SMASK              0x4ull
#define DC_PCS_66_REG_6_PRBS_MODE_SEL_SHIFT              1
#define DC_PCS_66_REG_6_PRBS_MODE_SEL_MASK               0x1ull
#define DC_PCS_66_REG_6_PRBS_MODE_SEL_SMASK              0x2ull
#define DC_PCS_66_REG_6_PRBS_ENABLE_SHIFT                0
#define DC_PCS_66_REG_6_PRBS_ENABLE_MASK                 0x1ull
#define DC_PCS_66_REG_6_PRBS_ENABLE_SMASK                0x1ull
/*
* Table #9 of 240_CSR_pcslpe.xml - pcs_66_reg_7
* See description below for individual field definition
*/
#define DC_PCS_66_REG_7                                  (DC_PCSLPE_PCS_66 + 0x000000000038)
#define DC_PCS_66_REG_7_RESETCSR                         0x00000000ull
#define DC_PCS_66_REG_7_UNUSED_31_4_SHIFT                4
#define DC_PCS_66_REG_7_UNUSED_31_4_MASK                 0xFFFFFFFull
#define DC_PCS_66_REG_7_UNUSED_31_4_SMASK                0xFFFFFFF0ull
#define DC_PCS_66_REG_7_TRAIN_TX_PHS_FIFO_SHIFT          3
#define DC_PCS_66_REG_7_TRAIN_TX_PHS_FIFO_MASK           0x1ull
#define DC_PCS_66_REG_7_TRAIN_TX_PHS_FIFO_SMASK          0x8ull
#define DC_PCS_66_REG_7_SMF_DIAG_BUSY_SHIFT              2
#define DC_PCS_66_REG_7_SMF_DIAG_BUSY_MASK               0x1ull
#define DC_PCS_66_REG_7_SMF_DIAG_BUSY_SMASK              0x4ull
#define DC_PCS_66_REG_7_FORCE_SMF_DELETE_SHIFT           1
#define DC_PCS_66_REG_7_FORCE_SMF_DELETE_MASK            0x1ull
#define DC_PCS_66_REG_7_FORCE_SMF_DELETE_SMASK           0x2ull
#define DC_PCS_66_REG_7_FORCE_SMF_INSERT_SHIFT           0
#define DC_PCS_66_REG_7_FORCE_SMF_INSERT_MASK            0x1ull
#define DC_PCS_66_REG_7_FORCE_SMF_INSERT_SMASK           0x1ull
/*
* Table #10 of 240_CSR_pcslpe.xml - pcs_66_reg_8
* See description below for individual field definition
*/
#define DC_PCS_66_REG_8                                  (DC_PCSLPE_PCS_66 + 0x000000000040)
#define DC_PCS_66_REG_8_RESETCSR                         0x00000000ull
#define DC_PCS_66_REG_8_UNUSED_31_9_SHIFT                9
#define DC_PCS_66_REG_8_UNUSED_31_9_MASK                 0x7FFFFFull
#define DC_PCS_66_REG_8_UNUSED_31_9_SMASK                0xFFFFFE00ull
#define DC_PCS_66_REG_8_RST_ON_READ_SHIFT                8
#define DC_PCS_66_REG_8_RST_ON_READ_MASK                 0x1ull
#define DC_PCS_66_REG_8_RST_ON_READ_SMASK                0x100ull
#define DC_PCS_66_REG_8_UNUSED_7_1_SHIFT                 1
#define DC_PCS_66_REG_8_UNUSED_7_1_MASK                  0x7Full
#define DC_PCS_66_REG_8_UNUSED_7_1_SMASK                 0xFEull
#define DC_PCS_66_REG_8_FEC_ENABLE_SHIFT                 0
#define DC_PCS_66_REG_8_FEC_ENABLE_MASK                  0x1ull
#define DC_PCS_66_REG_8_FEC_ENABLE_SMASK                 0x1ull
/*
* Table #11 of 240_CSR_pcslpe.xml - pcs_66_reg_9
* See description below for individual field definition
*/
#define DC_PCS_66_REG_9                                  (DC_PCSLPE_PCS_66 + 0x000000000048)
#define DC_PCS_66_REG_9_RESETCSR                         0x00000000ull
#define DC_PCS_66_REG_9_UNUSED_31_0_SHIFT                0
#define DC_PCS_66_REG_9_UNUSED_31_0_MASK                 0xFFFFFFFFull
#define DC_PCS_66_REG_9_UNUSED_31_0_SMASK                0xFFFFFFFFull
/*
* Table #12 of 240_CSR_pcslpe.xml - pcs_66_reg_10
* See description below for individual field definition
*/
#define DC_PCS_66_REG_10                                 (DC_PCSLPE_PCS_66 + 0x000000000080)
#define DC_PCS_66_REG_10_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_10_L0_FEC_CORRECTED_BLOCKS_SHIFT   0
#define DC_PCS_66_REG_10_L0_FEC_CORRECTED_BLOCKS_MASK    0xFFFFFFFFull
#define DC_PCS_66_REG_10_L0_FEC_CORRECTED_BLOCKS_SMASK   0xFFFFFFFFull
/*
* Table #13 of 240_CSR_pcslpe.xml - pcs_66_reg_11
* See description below for individual field definition
*/
#define DC_PCS_66_REG_11                                 (DC_PCSLPE_PCS_66 + 0x000000000088)
#define DC_PCS_66_REG_11_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_11_L0_FEC_UNCORRECTED_BLOCKS_SHIFT 0
#define DC_PCS_66_REG_11_L0_FEC_UNCORRECTED_BLOCKS_MASK  0xFFFFFFFFull
#define DC_PCS_66_REG_11_L0_FEC_UNCORRECTED_BLOCKS_SMASK 0xFFFFFFFFull
/*
* Table #14 of 240_CSR_pcslpe.xml - pcs_66_reg_12
* See description below for individual field definition
*/
#define DC_PCS_66_REG_12                                 (DC_PCSLPE_PCS_66 + 0x000000000090)
#define DC_PCS_66_REG_12_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_12_L1_FEC_CORRECTED_BLOCKS_SHIFT   0
#define DC_PCS_66_REG_12_L1_FEC_CORRECTED_BLOCKS_MASK    0xFFFFFFFFull
#define DC_PCS_66_REG_12_L1_FEC_CORRECTED_BLOCKS_SMASK   0xFFFFFFFFull
/*
* Table #15 of 240_CSR_pcslpe.xml - pcs_66_reg_13
* See description below for individual field definition
*/
#define DC_PCS_66_REG_13                                 (DC_PCSLPE_PCS_66 + 0x000000000098)
#define DC_PCS_66_REG_13_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_13_L1_FEC_UNCORRECTED_BLOCKS_SHIFT 0
#define DC_PCS_66_REG_13_L1_FEC_UNCORRECTED_BLOCKS_MASK  0xFFFFFFFFull
#define DC_PCS_66_REG_13_L1_FEC_UNCORRECTED_BLOCKS_SMASK 0xFFFFFFFFull
/*
* Table #16 of 240_CSR_pcslpe.xml - pcs_66_reg_14
* See description below for individual field definition
*/
#define DC_PCS_66_REG_14                                 (DC_PCSLPE_PCS_66 + 0x0000000000A0)
#define DC_PCS_66_REG_14_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_14_L2_FEC_CORRECTED_BLOCKS_SHIFT   0
#define DC_PCS_66_REG_14_L2_FEC_CORRECTED_BLOCKS_MASK    0xFFFFFFFFull
#define DC_PCS_66_REG_14_L2_FEC_CORRECTED_BLOCKS_SMASK   0xFFFFFFFFull
/*
* Table #17 of 240_CSR_pcslpe.xml - pcs_66_reg_15
* See description below for individual field definition
*/
#define DC_PCS_66_REG_15                                 (DC_PCSLPE_PCS_66 + 0x0000000000A8)
#define DC_PCS_66_REG_15_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_15_L2_FEC_UNCORRECTED_BLOCKS_SHIFT 0
#define DC_PCS_66_REG_15_L2_FEC_UNCORRECTED_BLOCKS_MASK  0xFFFFFFFFull
#define DC_PCS_66_REG_15_L2_FEC_UNCORRECTED_BLOCKS_SMASK 0xFFFFFFFFull
/*
* Table #18 of 240_CSR_pcslpe.xml - pcs_66_reg_16
* See description below for individual field definition
*/
#define DC_PCS_66_REG_16                                 (DC_PCSLPE_PCS_66 + 0x0000000000B0)
#define DC_PCS_66_REG_16_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_16_L3_FEC_CORRECTED_BLOCKS_SHIFT   0
#define DC_PCS_66_REG_16_L3_FEC_CORRECTED_BLOCKS_MASK    0xFFFFFFFFull
#define DC_PCS_66_REG_16_L3_FEC_CORRECTED_BLOCKS_SMASK   0xFFFFFFFFull
/*
* Table #19 of 240_CSR_pcslpe.xml - pcs_66_reg_17
* See description below for individual field definition
*/
#define DC_PCS_66_REG_17                                 (DC_PCSLPE_PCS_66 + 0x0000000000B8)
#define DC_PCS_66_REG_17_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_17_L3_FEC_UNCORRECTED_BLOCKS_SHIFT 0
#define DC_PCS_66_REG_17_L3_FEC_UNCORRECTED_BLOCKS_MASK  0xFFFFFFFFull
#define DC_PCS_66_REG_17_L3_FEC_UNCORRECTED_BLOCKS_SMASK 0xFFFFFFFFull
/*
* Table #20 of 240_CSR_pcslpe.xml - pcs_66_reg_18
* See description below for individual field definition
*/
#define DC_PCS_66_REG_18                                 (DC_PCSLPE_PCS_66 + 0x0000000000C0)
#define DC_PCS_66_REG_18_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_18_UNUSED_31_16_SHIFT              16
#define DC_PCS_66_REG_18_UNUSED_31_16_MASK               0xFFFFull
#define DC_PCS_66_REG_18_UNUSED_31_16_SMASK              0xFFFF0000ull
#define DC_PCS_66_REG_18_INVALID_CTRL_BLOCK_CNT_SHIFT    8
#define DC_PCS_66_REG_18_INVALID_CTRL_BLOCK_CNT_MASK     0xFFull
#define DC_PCS_66_REG_18_INVALID_CTRL_BLOCK_CNT_SMASK    0xFF00ull
#define DC_PCS_66_REG_18_INVALID_SYNC_HDR_CNT_SHIFT      0
#define DC_PCS_66_REG_18_INVALID_SYNC_HDR_CNT_MASK       0xFFull
#define DC_PCS_66_REG_18_INVALID_SYNC_HDR_CNT_SMASK      0xFFull
/*
* Table #21 of 240_CSR_pcslpe.xml - pcs_66_reg_19
* See description below for individual field definition
*/
#define DC_PCS_66_REG_19                                 (DC_PCSLPE_PCS_66 + 0x0000000000C8)
#define DC_PCS_66_REG_19_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_19_UNUSED_31_0_SHIFT               0
#define DC_PCS_66_REG_19_UNUSED_31_0_MASK                0xFFFFFFFFull
#define DC_PCS_66_REG_19_UNUSED_31_0_SMASK               0xFFFFFFFFull
/*
* Table #22 of 240_CSR_pcslpe.xml - pcs_66_reg_1A
* See description below for individual field definition
*/
#define DC_PCS_66_REG_1A                                 (DC_PCSLPE_PCS_66 + 0x0000000000D0)
#define DC_PCS_66_REG_1A_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_1A_UNUSED_31_0_SHIFT               0
#define DC_PCS_66_REG_1A_UNUSED_31_0_MASK                0xFFFFFFFFull
#define DC_PCS_66_REG_1A_UNUSED_31_0_SMASK               0xFFFFFFFFull
/*
* Table #23 of 240_CSR_pcslpe.xml - pcs_66_reg_20
* See description below for individual field definition
*/
#define DC_PCS_66_REG_20                                 (DC_PCSLPE_PCS_66 + 0x000000000100)
#define DC_PCS_66_REG_20_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_20_ERROR_DETECTION_COUNT_SHIFT     0
#define DC_PCS_66_REG_20_ERROR_DETECTION_COUNT_MASK      0xFFFFFFFFull
#define DC_PCS_66_REG_20_ERROR_DETECTION_COUNT_SMASK     0xFFFFFFFFull
/*
* Table #24 of 240_CSR_pcslpe.xml - pcs_66_reg_38
* See description below for individual field definition
*/
#define DC_PCS_66_REG_38                                 (DC_PCSLPE_PCS_66 + 0x0000000001C0)
#define DC_PCS_66_REG_38_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_38_UNUSED_31_5_SHIFT               5
#define DC_PCS_66_REG_38_UNUSED_31_5_MASK                0x7FFFFFFull
#define DC_PCS_66_REG_38_UNUSED_31_5_SMASK               0xFFFFFFE0ull
#define DC_PCS_66_REG_38_ONE_SHOT_PKT_ERR_SHIFT          0
#define DC_PCS_66_REG_38_ONE_SHOT_PKT_ERR_MASK           0x1Full
#define DC_PCS_66_REG_38_ONE_SHOT_PKT_ERR_SMASK          0x1Full
/*
* Table #25 of 240_CSR_pcslpe.xml - pcs_66_reg_39
* See description below for individual field definition
*/
#define DC_PCS_66_REG_39                                 (DC_PCSLPE_PCS_66 + 0x0000000001C8)
#define DC_PCS_66_REG_39_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_39_UNUSED_31_12_SHIFT              12
#define DC_PCS_66_REG_39_UNUSED_31_12_MASK               0xFFFFFull
#define DC_PCS_66_REG_39_UNUSED_31_12_SMASK              0xFFFFF000ull
#define DC_PCS_66_REG_39_ONE_SHOT_66B_CNT_SHIFT          4
#define DC_PCS_66_REG_39_ONE_SHOT_66B_CNT_MASK           0xFFull
#define DC_PCS_66_REG_39_ONE_SHOT_66B_CNT_SMASK          0xFF0ull
#define DC_PCS_66_REG_39_ONE_SHOT_66B_ERR_SHIFT          0
#define DC_PCS_66_REG_39_ONE_SHOT_66B_ERR_MASK           0xFull
#define DC_PCS_66_REG_39_ONE_SHOT_66B_ERR_SMASK          0xFull
/*
* Table #26 of 240_CSR_pcslpe.xml - pcs_66_reg_3A
* See description below for individual field definition
*/
#define DC_PCS_66_REG_3A                                 (DC_PCSLPE_PCS_66 + 0x0000000001D0)
#define DC_PCS_66_REG_3A_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_3A_UNUSED_31_8_SHIFT               8
#define DC_PCS_66_REG_3A_UNUSED_31_8_MASK                0xFFFFFFull
#define DC_PCS_66_REG_3A_UNUSED_31_8_SMASK               0xFFFFFF00ull
#define DC_PCS_66_REG_3A_ONE_SHOT_CTRL_BLK_ERR_SHIFT     0
#define DC_PCS_66_REG_3A_ONE_SHOT_CTRL_BLK_ERR_MASK      0xFFull
#define DC_PCS_66_REG_3A_ONE_SHOT_CTRL_BLK_ERR_SMASK     0xFFull
/*
* Table #27 of 240_CSR_pcslpe.xml - pcs_66_reg_3B
* See description below for individual field definition
*/
#define DC_PCS_66_REG_3B                                 (DC_PCSLPE_PCS_66 + 0x0000000001D8)
#define DC_PCS_66_REG_3B_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_3B_CTRL_BLK_0123_SHIFT             0
#define DC_PCS_66_REG_3B_CTRL_BLK_0123_MASK              0xFFFFFFFFull
#define DC_PCS_66_REG_3B_CTRL_BLK_0123_SMASK             0xFFFFFFFFull
/*
* Table #28 of 240_CSR_pcslpe.xml - pcs_66_reg_3C
* See description below for individual field definition
*/
#define DC_PCS_66_REG_3C                                 (DC_PCSLPE_PCS_66 + 0x0000000001E0)
#define DC_PCS_66_REG_3C_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_3C_CTRL_BLK_4567_SHIFT             0
#define DC_PCS_66_REG_3C_CTRL_BLK_4567_MASK              0xFFFFFFFFull
#define DC_PCS_66_REG_3C_CTRL_BLK_4567_SMASK             0xFFFFFFFFull
/*
* Table #2 of 240_CSR_pcslpe.xml - pcs_66_reg_0
* See description below for individual field definition
*/
#define DC_PCS_66_REG_0                                  (DC_PCSLPE_PCS_66 + 0x000000000000)
#define DC_PCS_66_REG_0_RESETCSR                         0x00000000ull
#define DC_PCS_66_REG_0_UNUSED_31_0_SHIFT                0
#define DC_PCS_66_REG_0_UNUSED_31_0_MASK                 0xFFFFFFFFull
#define DC_PCS_66_REG_0_UNUSED_31_0_SMASK                0xFFFFFFFFull
/*
* Table #3 of 240_CSR_pcslpe.xml - pcs_66_reg_1
* See description below for individual field definition
*/
#define DC_PCS_66_REG_1                                  (DC_PCSLPE_PCS_66 + 0x000000000008)
#define DC_PCS_66_REG_1_RESETCSR                         0x00000000ull
#define DC_PCS_66_REG_1_UNUSED_31_12_SHIFT               12
#define DC_PCS_66_REG_1_UNUSED_31_12_MASK                0xFFFFFull
#define DC_PCS_66_REG_1_UNUSED_31_12_SMASK               0xFFFFF000ull
#define DC_PCS_66_REG_1_PRBS_ERROR_SHIFT                 8
#define DC_PCS_66_REG_1_PRBS_ERROR_MASK                  0xFull
#define DC_PCS_66_REG_1_PRBS_ERROR_SMASK                 0xF00ull
#define DC_PCS_66_REG_1_UNUSED_7_4_SHIFT                 4
#define DC_PCS_66_REG_1_UNUSED_7_4_MASK                  0xFull
#define DC_PCS_66_REG_1_UNUSED_7_4_SMASK                 0xF0ull
#define DC_PCS_66_REG_1_SMF_OVERFLOW_SHIFT               0
#define DC_PCS_66_REG_1_SMF_OVERFLOW_MASK                0xFull
#define DC_PCS_66_REG_1_SMF_OVERFLOW_SMASK               0xFull
/*
* Table #4 of 240_CSR_pcslpe.xml - pcs_66_reg_2
* See description below for individual field definition
*/
#define DC_PCS_66_REG_2                                  (DC_PCSLPE_PCS_66 + 0x000000000010)
#define DC_PCS_66_REG_2_RESETCSR                         0x00000E40ull
#define DC_PCS_66_REG_2_UNUSED_31_12_SHIFT               12
#define DC_PCS_66_REG_2_UNUSED_31_12_MASK                0xFFFFFull
#define DC_PCS_66_REG_2_UNUSED_31_12_SMASK               0xFFFFF000ull
#define DC_PCS_66_REG_2_TX_LANE_ORDER_D_SHIFT            10
#define DC_PCS_66_REG_2_TX_LANE_ORDER_D_MASK             0x3ull
#define DC_PCS_66_REG_2_TX_LANE_ORDER_D_SMASK            0xC00ull
#define DC_PCS_66_REG_2_TX_LANE_ORDER_C_SHIFT            8
#define DC_PCS_66_REG_2_TX_LANE_ORDER_C_MASK             0x3ull
#define DC_PCS_66_REG_2_TX_LANE_ORDER_C_SMASK            0x300ull
#define DC_PCS_66_REG_2_TX_LANE_ORDER_B_SHIFT            6
#define DC_PCS_66_REG_2_TX_LANE_ORDER_B_MASK             0x3ull
#define DC_PCS_66_REG_2_TX_LANE_ORDER_B_SMASK            0xC0ull
#define DC_PCS_66_REG_2_TX_LANE_ORDER_A_SHIFT            4
#define DC_PCS_66_REG_2_TX_LANE_ORDER_A_MASK             0x3ull
#define DC_PCS_66_REG_2_TX_LANE_ORDER_A_SMASK            0x30ull
#define DC_PCS_66_REG_2_TX_LANE_POLARITY_SHIFT           0
#define DC_PCS_66_REG_2_TX_LANE_POLARITY_MASK            0xFull
#define DC_PCS_66_REG_2_TX_LANE_POLARITY_SMASK           0xFull
/*
* Table #5 of 240_CSR_pcslpe.xml - pcs_66_reg_3
* See description below for individual field definition
*/
#define DC_PCS_66_REG_3                                  (DC_PCSLPE_PCS_66 + 0x000000000018)
#define DC_PCS_66_REG_3_RESETCSR                         0x00000000ull
#define DC_PCS_66_REG_3_UNUSED_31_4_SHIFT                4
#define DC_PCS_66_REG_3_UNUSED_31_4_MASK                 0xFFFFFFFull
#define DC_PCS_66_REG_3_UNUSED_31_4_SMASK                0xFFFFFFF0ull
#define DC_PCS_66_REG_3_RX_LOCK_SHIFT                    0
#define DC_PCS_66_REG_3_RX_LOCK_MASK                     0xFull
#define DC_PCS_66_REG_3_RX_LOCK_SMASK                    0xFull
/*
* Table #6 of 240_CSR_pcslpe.xml - pcs_66_reg_4
* See description below for individual field definition
*/
#define DC_PCS_66_REG_4                                  (DC_PCSLPE_PCS_66 + 0x000000000020)
#define DC_PCS_66_REG_4_RESETCSR                         0x00000100ull
#define DC_PCS_66_REG_4_UNUSED_31_9_SHIFT                9
#define DC_PCS_66_REG_4_UNUSED_31_9_MASK                 0x7FFFFFull
#define DC_PCS_66_REG_4_UNUSED_31_9_SMASK                0xFFFFFE00ull
#define DC_PCS_66_REG_4_AUTO_POLARITY_REV_EN_SHIFT       8
#define DC_PCS_66_REG_4_AUTO_POLARITY_REV_EN_MASK        0x1ull
#define DC_PCS_66_REG_4_AUTO_POLARITY_REV_EN_SMASK       0x100ull
#define DC_PCS_66_REG_4_UNUSED_7_2_SHIFT                 2
#define DC_PCS_66_REG_4_UNUSED_7_2_MASK                  0x3Full
#define DC_PCS_66_REG_4_UNUSED_7_2_SMASK                 0xFCull
#define DC_PCS_66_REG_4_PCS_DATA_SYNC_DIS_SHIFT          1
#define DC_PCS_66_REG_4_PCS_DATA_SYNC_DIS_MASK           0x1ull
#define DC_PCS_66_REG_4_PCS_DATA_SYNC_DIS_SMASK          0x2ull
#define DC_PCS_66_REG_4_UNUSED_0_SHIFT                   0
#define DC_PCS_66_REG_4_UNUSED_0_MASK                    0x1ull
#define DC_PCS_66_REG_4_UNUSED_0_SMASK                   0x1ull
/*
* Table #7 of 240_CSR_pcslpe.xml - pcs_66_reg_5
* See description below for individual field definition
*/
#define DC_PCS_66_REG_5                                  (DC_PCSLPE_PCS_66 + 0x000000000028)
#define DC_PCS_66_REG_5_RESETCSR                         0x00000E40ull
#define DC_PCS_66_REG_5_UNUSED_31_12_SHIFT               12
#define DC_PCS_66_REG_5_UNUSED_31_12_MASK                0xFFFFFull
#define DC_PCS_66_REG_5_UNUSED_31_12_SMASK               0xFFFFF000ull
#define DC_PCS_66_REG_5_RX_LANE_ORDER_D_SHIFT            10
#define DC_PCS_66_REG_5_RX_LANE_ORDER_D_MASK             0x3ull
#define DC_PCS_66_REG_5_RX_LANE_ORDER_D_SMASK            0xC00ull
#define DC_PCS_66_REG_5_RX_LANE_ORDER_C_SHIFT            8
#define DC_PCS_66_REG_5_RX_LANE_ORDER_C_MASK             0x3ull
#define DC_PCS_66_REG_5_RX_LANE_ORDER_C_SMASK            0x300ull
#define DC_PCS_66_REG_5_RX_LANE_ORDER_B_SHIFT            6
#define DC_PCS_66_REG_5_RX_LANE_ORDER_B_MASK             0x3ull
#define DC_PCS_66_REG_5_RX_LANE_ORDER_B_SMASK            0xC0ull
#define DC_PCS_66_REG_5_RX_LANE_ORDER_A_SHIFT            4
#define DC_PCS_66_REG_5_RX_LANE_ORDER_A_MASK             0x3ull
#define DC_PCS_66_REG_5_RX_LANE_ORDER_A_SMASK            0x30ull
#define DC_PCS_66_REG_5_RX_LANE_POLARITY_SHIFT           0
#define DC_PCS_66_REG_5_RX_LANE_POLARITY_MASK            0xFull
#define DC_PCS_66_REG_5_RX_LANE_POLARITY_SMASK           0xFull
/*
* Table #8 of 240_CSR_pcslpe.xml - pcs_66_reg_6
* See description below for individual field definition
*/
#define DC_PCS_66_REG_6                                  (DC_PCSLPE_PCS_66 + 0x000000000030)
#define DC_PCS_66_REG_6_RESETCSR                         0x00000000ull
#define DC_PCS_66_REG_6_UNUSED_31_3_SHIFT                3
#define DC_PCS_66_REG_6_UNUSED_31_3_MASK                 0x1FFFFFFFull
#define DC_PCS_66_REG_6_UNUSED_31_3_SMASK                0xFFFFFFF8ull
#define DC_PCS_66_REG_6_PRBS_POLARITY_SHIFT              2
#define DC_PCS_66_REG_6_PRBS_POLARITY_MASK               0x1ull
#define DC_PCS_66_REG_6_PRBS_POLARITY_SMASK              0x4ull
#define DC_PCS_66_REG_6_PRBS_MODE_SEL_SHIFT              1
#define DC_PCS_66_REG_6_PRBS_MODE_SEL_MASK               0x1ull
#define DC_PCS_66_REG_6_PRBS_MODE_SEL_SMASK              0x2ull
#define DC_PCS_66_REG_6_PRBS_ENABLE_SHIFT                0
#define DC_PCS_66_REG_6_PRBS_ENABLE_MASK                 0x1ull
#define DC_PCS_66_REG_6_PRBS_ENABLE_SMASK                0x1ull
/*
* Table #9 of 240_CSR_pcslpe.xml - pcs_66_reg_7
* See description below for individual field definition
*/
#define DC_PCS_66_REG_7                                  (DC_PCSLPE_PCS_66 + 0x000000000038)
#define DC_PCS_66_REG_7_RESETCSR                         0x00000000ull
#define DC_PCS_66_REG_7_UNUSED_31_4_SHIFT                4
#define DC_PCS_66_REG_7_UNUSED_31_4_MASK                 0xFFFFFFFull
#define DC_PCS_66_REG_7_UNUSED_31_4_SMASK                0xFFFFFFF0ull
#define DC_PCS_66_REG_7_TRAIN_TX_PHS_FIFO_SHIFT          3
#define DC_PCS_66_REG_7_TRAIN_TX_PHS_FIFO_MASK           0x1ull
#define DC_PCS_66_REG_7_TRAIN_TX_PHS_FIFO_SMASK          0x8ull
#define DC_PCS_66_REG_7_SMF_DIAG_BUSY_SHIFT              2
#define DC_PCS_66_REG_7_SMF_DIAG_BUSY_MASK               0x1ull
#define DC_PCS_66_REG_7_SMF_DIAG_BUSY_SMASK              0x4ull
#define DC_PCS_66_REG_7_FORCE_SMF_DELETE_SHIFT           1
#define DC_PCS_66_REG_7_FORCE_SMF_DELETE_MASK            0x1ull
#define DC_PCS_66_REG_7_FORCE_SMF_DELETE_SMASK           0x2ull
#define DC_PCS_66_REG_7_FORCE_SMF_INSERT_SHIFT           0
#define DC_PCS_66_REG_7_FORCE_SMF_INSERT_MASK            0x1ull
#define DC_PCS_66_REG_7_FORCE_SMF_INSERT_SMASK           0x1ull
/*
* Table #10 of 240_CSR_pcslpe.xml - pcs_66_reg_8
* See description below for individual field definition
*/
#define DC_PCS_66_REG_8                                  (DC_PCSLPE_PCS_66 + 0x000000000040)
#define DC_PCS_66_REG_8_RESETCSR                         0x00000000ull
#define DC_PCS_66_REG_8_UNUSED_31_9_SHIFT                9
#define DC_PCS_66_REG_8_UNUSED_31_9_MASK                 0x7FFFFFull
#define DC_PCS_66_REG_8_UNUSED_31_9_SMASK                0xFFFFFE00ull
#define DC_PCS_66_REG_8_RST_ON_READ_SHIFT                8
#define DC_PCS_66_REG_8_RST_ON_READ_MASK                 0x1ull
#define DC_PCS_66_REG_8_RST_ON_READ_SMASK                0x100ull
#define DC_PCS_66_REG_8_UNUSED_7_1_SHIFT                 1
#define DC_PCS_66_REG_8_UNUSED_7_1_MASK                  0x7Full
#define DC_PCS_66_REG_8_UNUSED_7_1_SMASK                 0xFEull
#define DC_PCS_66_REG_8_FEC_ENABLE_SHIFT                 0
#define DC_PCS_66_REG_8_FEC_ENABLE_MASK                  0x1ull
#define DC_PCS_66_REG_8_FEC_ENABLE_SMASK                 0x1ull
/*
* Table #11 of 240_CSR_pcslpe.xml - pcs_66_reg_9
* See description below for individual field definition
*/
#define DC_PCS_66_REG_9                                  (DC_PCSLPE_PCS_66 + 0x000000000048)
#define DC_PCS_66_REG_9_RESETCSR                         0x00000000ull
#define DC_PCS_66_REG_9_UNUSED_31_0_SHIFT                0
#define DC_PCS_66_REG_9_UNUSED_31_0_MASK                 0xFFFFFFFFull
#define DC_PCS_66_REG_9_UNUSED_31_0_SMASK                0xFFFFFFFFull
/*
* Table #12 of 240_CSR_pcslpe.xml - pcs_66_reg_10
* See description below for individual field definition
*/
#define DC_PCS_66_REG_10                                 (DC_PCSLPE_PCS_66 + 0x000000000080)
#define DC_PCS_66_REG_10_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_10_L0_FEC_CORRECTED_BLOCKS_SHIFT   0
#define DC_PCS_66_REG_10_L0_FEC_CORRECTED_BLOCKS_MASK    0xFFFFFFFFull
#define DC_PCS_66_REG_10_L0_FEC_CORRECTED_BLOCKS_SMASK   0xFFFFFFFFull
/*
* Table #13 of 240_CSR_pcslpe.xml - pcs_66_reg_11
* See description below for individual field definition
*/
#define DC_PCS_66_REG_11                                 (DC_PCSLPE_PCS_66 + 0x000000000088)
#define DC_PCS_66_REG_11_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_11_L0_FEC_UNCORRECTED_BLOCKS_SHIFT 0
#define DC_PCS_66_REG_11_L0_FEC_UNCORRECTED_BLOCKS_MASK  0xFFFFFFFFull
#define DC_PCS_66_REG_11_L0_FEC_UNCORRECTED_BLOCKS_SMASK 0xFFFFFFFFull
/*
* Table #14 of 240_CSR_pcslpe.xml - pcs_66_reg_12
* See description below for individual field definition
*/
#define DC_PCS_66_REG_12                                 (DC_PCSLPE_PCS_66 + 0x000000000090)
#define DC_PCS_66_REG_12_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_12_L1_FEC_CORRECTED_BLOCKS_SHIFT   0
#define DC_PCS_66_REG_12_L1_FEC_CORRECTED_BLOCKS_MASK    0xFFFFFFFFull
#define DC_PCS_66_REG_12_L1_FEC_CORRECTED_BLOCKS_SMASK   0xFFFFFFFFull
/*
* Table #15 of 240_CSR_pcslpe.xml - pcs_66_reg_13
* See description below for individual field definition
*/
#define DC_PCS_66_REG_13                                 (DC_PCSLPE_PCS_66 + 0x000000000098)
#define DC_PCS_66_REG_13_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_13_L1_FEC_UNCORRECTED_BLOCKS_SHIFT 0
#define DC_PCS_66_REG_13_L1_FEC_UNCORRECTED_BLOCKS_MASK  0xFFFFFFFFull
#define DC_PCS_66_REG_13_L1_FEC_UNCORRECTED_BLOCKS_SMASK 0xFFFFFFFFull
/*
* Table #16 of 240_CSR_pcslpe.xml - pcs_66_reg_14
* See description below for individual field definition
*/
#define DC_PCS_66_REG_14                                 (DC_PCSLPE_PCS_66 + 0x0000000000A0)
#define DC_PCS_66_REG_14_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_14_L2_FEC_CORRECTED_BLOCKS_SHIFT   0
#define DC_PCS_66_REG_14_L2_FEC_CORRECTED_BLOCKS_MASK    0xFFFFFFFFull
#define DC_PCS_66_REG_14_L2_FEC_CORRECTED_BLOCKS_SMASK   0xFFFFFFFFull
/*
* Table #17 of 240_CSR_pcslpe.xml - pcs_66_reg_15
* See description below for individual field definition
*/
#define DC_PCS_66_REG_15                                 (DC_PCSLPE_PCS_66 + 0x0000000000A8)
#define DC_PCS_66_REG_15_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_15_L2_FEC_UNCORRECTED_BLOCKS_SHIFT 0
#define DC_PCS_66_REG_15_L2_FEC_UNCORRECTED_BLOCKS_MASK  0xFFFFFFFFull
#define DC_PCS_66_REG_15_L2_FEC_UNCORRECTED_BLOCKS_SMASK 0xFFFFFFFFull
/*
* Table #18 of 240_CSR_pcslpe.xml - pcs_66_reg_16
* See description below for individual field definition
*/
#define DC_PCS_66_REG_16                                 (DC_PCSLPE_PCS_66 + 0x0000000000B0)
#define DC_PCS_66_REG_16_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_16_L3_FEC_CORRECTED_BLOCKS_SHIFT   0
#define DC_PCS_66_REG_16_L3_FEC_CORRECTED_BLOCKS_MASK    0xFFFFFFFFull
#define DC_PCS_66_REG_16_L3_FEC_CORRECTED_BLOCKS_SMASK   0xFFFFFFFFull
/*
* Table #19 of 240_CSR_pcslpe.xml - pcs_66_reg_17
* See description below for individual field definition
*/
#define DC_PCS_66_REG_17                                 (DC_PCSLPE_PCS_66 + 0x0000000000B8)
#define DC_PCS_66_REG_17_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_17_L3_FEC_UNCORRECTED_BLOCKS_SHIFT 0
#define DC_PCS_66_REG_17_L3_FEC_UNCORRECTED_BLOCKS_MASK  0xFFFFFFFFull
#define DC_PCS_66_REG_17_L3_FEC_UNCORRECTED_BLOCKS_SMASK 0xFFFFFFFFull
/*
* Table #20 of 240_CSR_pcslpe.xml - pcs_66_reg_18
* See description below for individual field definition
*/
#define DC_PCS_66_REG_18                                 (DC_PCSLPE_PCS_66 + 0x0000000000C0)
#define DC_PCS_66_REG_18_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_18_UNUSED_31_16_SHIFT              16
#define DC_PCS_66_REG_18_UNUSED_31_16_MASK               0xFFFFull
#define DC_PCS_66_REG_18_UNUSED_31_16_SMASK              0xFFFF0000ull
#define DC_PCS_66_REG_18_INVALID_CTRL_BLOCK_CNT_SHIFT    8
#define DC_PCS_66_REG_18_INVALID_CTRL_BLOCK_CNT_MASK     0xFFull
#define DC_PCS_66_REG_18_INVALID_CTRL_BLOCK_CNT_SMASK    0xFF00ull
#define DC_PCS_66_REG_18_INVALID_SYNC_HDR_CNT_SHIFT      0
#define DC_PCS_66_REG_18_INVALID_SYNC_HDR_CNT_MASK       0xFFull
#define DC_PCS_66_REG_18_INVALID_SYNC_HDR_CNT_SMASK      0xFFull
/*
* Table #21 of 240_CSR_pcslpe.xml - pcs_66_reg_19
* See description below for individual field definition
*/
#define DC_PCS_66_REG_19                                 (DC_PCSLPE_PCS_66 + 0x0000000000C8)
#define DC_PCS_66_REG_19_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_19_UNUSED_31_0_SHIFT               0
#define DC_PCS_66_REG_19_UNUSED_31_0_MASK                0xFFFFFFFFull
#define DC_PCS_66_REG_19_UNUSED_31_0_SMASK               0xFFFFFFFFull
/*
* Table #22 of 240_CSR_pcslpe.xml - pcs_66_reg_1A
* See description below for individual field definition
*/
#define DC_PCS_66_REG_1A                                 (DC_PCSLPE_PCS_66 + 0x0000000000D0)
#define DC_PCS_66_REG_1A_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_1A_UNUSED_31_0_SHIFT               0
#define DC_PCS_66_REG_1A_UNUSED_31_0_MASK                0xFFFFFFFFull
#define DC_PCS_66_REG_1A_UNUSED_31_0_SMASK               0xFFFFFFFFull
/*
* Table #23 of 240_CSR_pcslpe.xml - pcs_66_reg_20
* See description below for individual field definition
*/
#define DC_PCS_66_REG_20                                 (DC_PCSLPE_PCS_66 + 0x000000000100)
#define DC_PCS_66_REG_20_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_20_ERROR_DETECTION_COUNT_SHIFT     0
#define DC_PCS_66_REG_20_ERROR_DETECTION_COUNT_MASK      0xFFFFFFFFull
#define DC_PCS_66_REG_20_ERROR_DETECTION_COUNT_SMASK     0xFFFFFFFFull
/*
* Table #24 of 240_CSR_pcslpe.xml - pcs_66_reg_38
* See description below for individual field definition
*/
#define DC_PCS_66_REG_38                                 (DC_PCSLPE_PCS_66 + 0x0000000001C0)
#define DC_PCS_66_REG_38_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_38_UNUSED_31_5_SHIFT               5
#define DC_PCS_66_REG_38_UNUSED_31_5_MASK                0x7FFFFFFull
#define DC_PCS_66_REG_38_UNUSED_31_5_SMASK               0xFFFFFFE0ull
#define DC_PCS_66_REG_38_ONE_SHOT_PKT_ERR_SHIFT          0
#define DC_PCS_66_REG_38_ONE_SHOT_PKT_ERR_MASK           0x1Full
#define DC_PCS_66_REG_38_ONE_SHOT_PKT_ERR_SMASK          0x1Full
/*
* Table #25 of 240_CSR_pcslpe.xml - pcs_66_reg_39
* See description below for individual field definition
*/
#define DC_PCS_66_REG_39                                 (DC_PCSLPE_PCS_66 + 0x0000000001C8)
#define DC_PCS_66_REG_39_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_39_UNUSED_31_12_SHIFT              12
#define DC_PCS_66_REG_39_UNUSED_31_12_MASK               0xFFFFFull
#define DC_PCS_66_REG_39_UNUSED_31_12_SMASK              0xFFFFF000ull
#define DC_PCS_66_REG_39_ONE_SHOT_66B_CNT_SHIFT          4
#define DC_PCS_66_REG_39_ONE_SHOT_66B_CNT_MASK           0xFFull
#define DC_PCS_66_REG_39_ONE_SHOT_66B_CNT_SMASK          0xFF0ull
#define DC_PCS_66_REG_39_ONE_SHOT_66B_ERR_SHIFT          0
#define DC_PCS_66_REG_39_ONE_SHOT_66B_ERR_MASK           0xFull
#define DC_PCS_66_REG_39_ONE_SHOT_66B_ERR_SMASK          0xFull
/*
* Table #26 of 240_CSR_pcslpe.xml - pcs_66_reg_3A
* See description below for individual field definition
*/
#define DC_PCS_66_REG_3A                                 (DC_PCSLPE_PCS_66 + 0x0000000001D0)
#define DC_PCS_66_REG_3A_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_3A_UNUSED_31_8_SHIFT               8
#define DC_PCS_66_REG_3A_UNUSED_31_8_MASK                0xFFFFFFull
#define DC_PCS_66_REG_3A_UNUSED_31_8_SMASK               0xFFFFFF00ull
#define DC_PCS_66_REG_3A_ONE_SHOT_CTRL_BLK_ERR_SHIFT     0
#define DC_PCS_66_REG_3A_ONE_SHOT_CTRL_BLK_ERR_MASK      0xFFull
#define DC_PCS_66_REG_3A_ONE_SHOT_CTRL_BLK_ERR_SMASK     0xFFull
/*
* Table #27 of 240_CSR_pcslpe.xml - pcs_66_reg_3B
* See description below for individual field definition
*/
#define DC_PCS_66_REG_3B                                 (DC_PCSLPE_PCS_66 + 0x0000000001D8)
#define DC_PCS_66_REG_3B_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_3B_CTRL_BLK_0123_SHIFT             0
#define DC_PCS_66_REG_3B_CTRL_BLK_0123_MASK              0xFFFFFFFFull
#define DC_PCS_66_REG_3B_CTRL_BLK_0123_SMASK             0xFFFFFFFFull
/*
* Table #28 of 240_CSR_pcslpe.xml - pcs_66_reg_3C
* See description below for individual field definition
*/
#define DC_PCS_66_REG_3C                                 (DC_PCSLPE_PCS_66 + 0x0000000001E0)
#define DC_PCS_66_REG_3C_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_3C_CTRL_BLK_4567_SHIFT             0
#define DC_PCS_66_REG_3C_CTRL_BLK_4567_MASK              0xFFFFFFFFull
#define DC_PCS_66_REG_3C_CTRL_BLK_4567_SMASK             0xFFFFFFFFull
/*
* Table #2 of 240_CSR_pcslpe.xml - pcs_66_reg_0
* See description below for individual field definition
*/
#define DC_PCS_66_REG_0                                  (DC_PCSLPE_PCS_66 + 0x000000000000)
#define DC_PCS_66_REG_0_RESETCSR                         0x00000000ull
#define DC_PCS_66_REG_0_UNUSED_31_0_SHIFT                0
#define DC_PCS_66_REG_0_UNUSED_31_0_MASK                 0xFFFFFFFFull
#define DC_PCS_66_REG_0_UNUSED_31_0_SMASK                0xFFFFFFFFull
/*
* Table #3 of 240_CSR_pcslpe.xml - pcs_66_reg_1
* See description below for individual field definition
*/
#define DC_PCS_66_REG_1                                  (DC_PCSLPE_PCS_66 + 0x000000000008)
#define DC_PCS_66_REG_1_RESETCSR                         0x00000000ull
#define DC_PCS_66_REG_1_UNUSED_31_12_SHIFT               12
#define DC_PCS_66_REG_1_UNUSED_31_12_MASK                0xFFFFFull
#define DC_PCS_66_REG_1_UNUSED_31_12_SMASK               0xFFFFF000ull
#define DC_PCS_66_REG_1_PRBS_ERROR_SHIFT                 8
#define DC_PCS_66_REG_1_PRBS_ERROR_MASK                  0xFull
#define DC_PCS_66_REG_1_PRBS_ERROR_SMASK                 0xF00ull
#define DC_PCS_66_REG_1_UNUSED_7_4_SHIFT                 4
#define DC_PCS_66_REG_1_UNUSED_7_4_MASK                  0xFull
#define DC_PCS_66_REG_1_UNUSED_7_4_SMASK                 0xF0ull
#define DC_PCS_66_REG_1_SMF_OVERFLOW_SHIFT               0
#define DC_PCS_66_REG_1_SMF_OVERFLOW_MASK                0xFull
#define DC_PCS_66_REG_1_SMF_OVERFLOW_SMASK               0xFull
/*
* Table #4 of 240_CSR_pcslpe.xml - pcs_66_reg_2
* See description below for individual field definition
*/
#define DC_PCS_66_REG_2                                  (DC_PCSLPE_PCS_66 + 0x000000000010)
#define DC_PCS_66_REG_2_RESETCSR                         0x00000E40ull
#define DC_PCS_66_REG_2_UNUSED_31_12_SHIFT               12
#define DC_PCS_66_REG_2_UNUSED_31_12_MASK                0xFFFFFull
#define DC_PCS_66_REG_2_UNUSED_31_12_SMASK               0xFFFFF000ull
#define DC_PCS_66_REG_2_TX_LANE_ORDER_D_SHIFT            10
#define DC_PCS_66_REG_2_TX_LANE_ORDER_D_MASK             0x3ull
#define DC_PCS_66_REG_2_TX_LANE_ORDER_D_SMASK            0xC00ull
#define DC_PCS_66_REG_2_TX_LANE_ORDER_C_SHIFT            8
#define DC_PCS_66_REG_2_TX_LANE_ORDER_C_MASK             0x3ull
#define DC_PCS_66_REG_2_TX_LANE_ORDER_C_SMASK            0x300ull
#define DC_PCS_66_REG_2_TX_LANE_ORDER_B_SHIFT            6
#define DC_PCS_66_REG_2_TX_LANE_ORDER_B_MASK             0x3ull
#define DC_PCS_66_REG_2_TX_LANE_ORDER_B_SMASK            0xC0ull
#define DC_PCS_66_REG_2_TX_LANE_ORDER_A_SHIFT            4
#define DC_PCS_66_REG_2_TX_LANE_ORDER_A_MASK             0x3ull
#define DC_PCS_66_REG_2_TX_LANE_ORDER_A_SMASK            0x30ull
#define DC_PCS_66_REG_2_TX_LANE_POLARITY_SHIFT           0
#define DC_PCS_66_REG_2_TX_LANE_POLARITY_MASK            0xFull
#define DC_PCS_66_REG_2_TX_LANE_POLARITY_SMASK           0xFull
/*
* Table #5 of 240_CSR_pcslpe.xml - pcs_66_reg_3
* See description below for individual field definition
*/
#define DC_PCS_66_REG_3                                  (DC_PCSLPE_PCS_66 + 0x000000000018)
#define DC_PCS_66_REG_3_RESETCSR                         0x00000000ull
#define DC_PCS_66_REG_3_UNUSED_31_4_SHIFT                4
#define DC_PCS_66_REG_3_UNUSED_31_4_MASK                 0xFFFFFFFull
#define DC_PCS_66_REG_3_UNUSED_31_4_SMASK                0xFFFFFFF0ull
#define DC_PCS_66_REG_3_RX_LOCK_SHIFT                    0
#define DC_PCS_66_REG_3_RX_LOCK_MASK                     0xFull
#define DC_PCS_66_REG_3_RX_LOCK_SMASK                    0xFull
/*
* Table #6 of 240_CSR_pcslpe.xml - pcs_66_reg_4
* See description below for individual field definition
*/
#define DC_PCS_66_REG_4                                  (DC_PCSLPE_PCS_66 + 0x000000000020)
#define DC_PCS_66_REG_4_RESETCSR                         0x00000100ull
#define DC_PCS_66_REG_4_UNUSED_31_9_SHIFT                9
#define DC_PCS_66_REG_4_UNUSED_31_9_MASK                 0x7FFFFFull
#define DC_PCS_66_REG_4_UNUSED_31_9_SMASK                0xFFFFFE00ull
#define DC_PCS_66_REG_4_AUTO_POLARITY_REV_EN_SHIFT       8
#define DC_PCS_66_REG_4_AUTO_POLARITY_REV_EN_MASK        0x1ull
#define DC_PCS_66_REG_4_AUTO_POLARITY_REV_EN_SMASK       0x100ull
#define DC_PCS_66_REG_4_UNUSED_7_2_SHIFT                 2
#define DC_PCS_66_REG_4_UNUSED_7_2_MASK                  0x3Full
#define DC_PCS_66_REG_4_UNUSED_7_2_SMASK                 0xFCull
#define DC_PCS_66_REG_4_PCS_DATA_SYNC_DIS_SHIFT          1
#define DC_PCS_66_REG_4_PCS_DATA_SYNC_DIS_MASK           0x1ull
#define DC_PCS_66_REG_4_PCS_DATA_SYNC_DIS_SMASK          0x2ull
#define DC_PCS_66_REG_4_UNUSED_0_SHIFT                   0
#define DC_PCS_66_REG_4_UNUSED_0_MASK                    0x1ull
#define DC_PCS_66_REG_4_UNUSED_0_SMASK                   0x1ull
/*
* Table #7 of 240_CSR_pcslpe.xml - pcs_66_reg_5
* See description below for individual field definition
*/
#define DC_PCS_66_REG_5                                  (DC_PCSLPE_PCS_66 + 0x000000000028)
#define DC_PCS_66_REG_5_RESETCSR                         0x00000E40ull
#define DC_PCS_66_REG_5_UNUSED_31_12_SHIFT               12
#define DC_PCS_66_REG_5_UNUSED_31_12_MASK                0xFFFFFull
#define DC_PCS_66_REG_5_UNUSED_31_12_SMASK               0xFFFFF000ull
#define DC_PCS_66_REG_5_RX_LANE_ORDER_D_SHIFT            10
#define DC_PCS_66_REG_5_RX_LANE_ORDER_D_MASK             0x3ull
#define DC_PCS_66_REG_5_RX_LANE_ORDER_D_SMASK            0xC00ull
#define DC_PCS_66_REG_5_RX_LANE_ORDER_C_SHIFT            8
#define DC_PCS_66_REG_5_RX_LANE_ORDER_C_MASK             0x3ull
#define DC_PCS_66_REG_5_RX_LANE_ORDER_C_SMASK            0x300ull
#define DC_PCS_66_REG_5_RX_LANE_ORDER_B_SHIFT            6
#define DC_PCS_66_REG_5_RX_LANE_ORDER_B_MASK             0x3ull
#define DC_PCS_66_REG_5_RX_LANE_ORDER_B_SMASK            0xC0ull
#define DC_PCS_66_REG_5_RX_LANE_ORDER_A_SHIFT            4
#define DC_PCS_66_REG_5_RX_LANE_ORDER_A_MASK             0x3ull
#define DC_PCS_66_REG_5_RX_LANE_ORDER_A_SMASK            0x30ull
#define DC_PCS_66_REG_5_RX_LANE_POLARITY_SHIFT           0
#define DC_PCS_66_REG_5_RX_LANE_POLARITY_MASK            0xFull
#define DC_PCS_66_REG_5_RX_LANE_POLARITY_SMASK           0xFull
/*
* Table #8 of 240_CSR_pcslpe.xml - pcs_66_reg_6
* See description below for individual field definition
*/
#define DC_PCS_66_REG_6                                  (DC_PCSLPE_PCS_66 + 0x000000000030)
#define DC_PCS_66_REG_6_RESETCSR                         0x00000000ull
#define DC_PCS_66_REG_6_UNUSED_31_3_SHIFT                3
#define DC_PCS_66_REG_6_UNUSED_31_3_MASK                 0x1FFFFFFFull
#define DC_PCS_66_REG_6_UNUSED_31_3_SMASK                0xFFFFFFF8ull
#define DC_PCS_66_REG_6_PRBS_POLARITY_SHIFT              2
#define DC_PCS_66_REG_6_PRBS_POLARITY_MASK               0x1ull
#define DC_PCS_66_REG_6_PRBS_POLARITY_SMASK              0x4ull
#define DC_PCS_66_REG_6_PRBS_MODE_SEL_SHIFT              1
#define DC_PCS_66_REG_6_PRBS_MODE_SEL_MASK               0x1ull
#define DC_PCS_66_REG_6_PRBS_MODE_SEL_SMASK              0x2ull
#define DC_PCS_66_REG_6_PRBS_ENABLE_SHIFT                0
#define DC_PCS_66_REG_6_PRBS_ENABLE_MASK                 0x1ull
#define DC_PCS_66_REG_6_PRBS_ENABLE_SMASK                0x1ull
/*
* Table #9 of 240_CSR_pcslpe.xml - pcs_66_reg_7
* See description below for individual field definition
*/
#define DC_PCS_66_REG_7                                  (DC_PCSLPE_PCS_66 + 0x000000000038)
#define DC_PCS_66_REG_7_RESETCSR                         0x00000000ull
#define DC_PCS_66_REG_7_UNUSED_31_4_SHIFT                4
#define DC_PCS_66_REG_7_UNUSED_31_4_MASK                 0xFFFFFFFull
#define DC_PCS_66_REG_7_UNUSED_31_4_SMASK                0xFFFFFFF0ull
#define DC_PCS_66_REG_7_TRAIN_TX_PHS_FIFO_SHIFT          3
#define DC_PCS_66_REG_7_TRAIN_TX_PHS_FIFO_MASK           0x1ull
#define DC_PCS_66_REG_7_TRAIN_TX_PHS_FIFO_SMASK          0x8ull
#define DC_PCS_66_REG_7_SMF_DIAG_BUSY_SHIFT              2
#define DC_PCS_66_REG_7_SMF_DIAG_BUSY_MASK               0x1ull
#define DC_PCS_66_REG_7_SMF_DIAG_BUSY_SMASK              0x4ull
#define DC_PCS_66_REG_7_FORCE_SMF_DELETE_SHIFT           1
#define DC_PCS_66_REG_7_FORCE_SMF_DELETE_MASK            0x1ull
#define DC_PCS_66_REG_7_FORCE_SMF_DELETE_SMASK           0x2ull
#define DC_PCS_66_REG_7_FORCE_SMF_INSERT_SHIFT           0
#define DC_PCS_66_REG_7_FORCE_SMF_INSERT_MASK            0x1ull
#define DC_PCS_66_REG_7_FORCE_SMF_INSERT_SMASK           0x1ull
/*
* Table #10 of 240_CSR_pcslpe.xml - pcs_66_reg_8
* See description below for individual field definition
*/
#define DC_PCS_66_REG_8                                  (DC_PCSLPE_PCS_66 + 0x000000000040)
#define DC_PCS_66_REG_8_RESETCSR                         0x00000000ull
#define DC_PCS_66_REG_8_UNUSED_31_9_SHIFT                9
#define DC_PCS_66_REG_8_UNUSED_31_9_MASK                 0x7FFFFFull
#define DC_PCS_66_REG_8_UNUSED_31_9_SMASK                0xFFFFFE00ull
#define DC_PCS_66_REG_8_RST_ON_READ_SHIFT                8
#define DC_PCS_66_REG_8_RST_ON_READ_MASK                 0x1ull
#define DC_PCS_66_REG_8_RST_ON_READ_SMASK                0x100ull
#define DC_PCS_66_REG_8_UNUSED_7_1_SHIFT                 1
#define DC_PCS_66_REG_8_UNUSED_7_1_MASK                  0x7Full
#define DC_PCS_66_REG_8_UNUSED_7_1_SMASK                 0xFEull
#define DC_PCS_66_REG_8_FEC_ENABLE_SHIFT                 0
#define DC_PCS_66_REG_8_FEC_ENABLE_MASK                  0x1ull
#define DC_PCS_66_REG_8_FEC_ENABLE_SMASK                 0x1ull
/*
* Table #11 of 240_CSR_pcslpe.xml - pcs_66_reg_9
* See description below for individual field definition
*/
#define DC_PCS_66_REG_9                                  (DC_PCSLPE_PCS_66 + 0x000000000048)
#define DC_PCS_66_REG_9_RESETCSR                         0x00000000ull
#define DC_PCS_66_REG_9_UNUSED_31_0_SHIFT                0
#define DC_PCS_66_REG_9_UNUSED_31_0_MASK                 0xFFFFFFFFull
#define DC_PCS_66_REG_9_UNUSED_31_0_SMASK                0xFFFFFFFFull
/*
* Table #12 of 240_CSR_pcslpe.xml - pcs_66_reg_10
* See description below for individual field definition
*/
#define DC_PCS_66_REG_10                                 (DC_PCSLPE_PCS_66 + 0x000000000080)
#define DC_PCS_66_REG_10_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_10_L0_FEC_CORRECTED_BLOCKS_SHIFT   0
#define DC_PCS_66_REG_10_L0_FEC_CORRECTED_BLOCKS_MASK    0xFFFFFFFFull
#define DC_PCS_66_REG_10_L0_FEC_CORRECTED_BLOCKS_SMASK   0xFFFFFFFFull
/*
* Table #13 of 240_CSR_pcslpe.xml - pcs_66_reg_11
* See description below for individual field definition
*/
#define DC_PCS_66_REG_11                                 (DC_PCSLPE_PCS_66 + 0x000000000088)
#define DC_PCS_66_REG_11_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_11_L0_FEC_UNCORRECTED_BLOCKS_SHIFT 0
#define DC_PCS_66_REG_11_L0_FEC_UNCORRECTED_BLOCKS_MASK  0xFFFFFFFFull
#define DC_PCS_66_REG_11_L0_FEC_UNCORRECTED_BLOCKS_SMASK 0xFFFFFFFFull
/*
* Table #14 of 240_CSR_pcslpe.xml - pcs_66_reg_12
* See description below for individual field definition
*/
#define DC_PCS_66_REG_12                                 (DC_PCSLPE_PCS_66 + 0x000000000090)
#define DC_PCS_66_REG_12_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_12_L1_FEC_CORRECTED_BLOCKS_SHIFT   0
#define DC_PCS_66_REG_12_L1_FEC_CORRECTED_BLOCKS_MASK    0xFFFFFFFFull
#define DC_PCS_66_REG_12_L1_FEC_CORRECTED_BLOCKS_SMASK   0xFFFFFFFFull
/*
* Table #15 of 240_CSR_pcslpe.xml - pcs_66_reg_13
* See description below for individual field definition
*/
#define DC_PCS_66_REG_13                                 (DC_PCSLPE_PCS_66 + 0x000000000098)
#define DC_PCS_66_REG_13_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_13_L1_FEC_UNCORRECTED_BLOCKS_SHIFT 0
#define DC_PCS_66_REG_13_L1_FEC_UNCORRECTED_BLOCKS_MASK  0xFFFFFFFFull
#define DC_PCS_66_REG_13_L1_FEC_UNCORRECTED_BLOCKS_SMASK 0xFFFFFFFFull
/*
* Table #16 of 240_CSR_pcslpe.xml - pcs_66_reg_14
* See description below for individual field definition
*/
#define DC_PCS_66_REG_14                                 (DC_PCSLPE_PCS_66 + 0x0000000000A0)
#define DC_PCS_66_REG_14_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_14_L2_FEC_CORRECTED_BLOCKS_SHIFT   0
#define DC_PCS_66_REG_14_L2_FEC_CORRECTED_BLOCKS_MASK    0xFFFFFFFFull
#define DC_PCS_66_REG_14_L2_FEC_CORRECTED_BLOCKS_SMASK   0xFFFFFFFFull
/*
* Table #17 of 240_CSR_pcslpe.xml - pcs_66_reg_15
* See description below for individual field definition
*/
#define DC_PCS_66_REG_15                                 (DC_PCSLPE_PCS_66 + 0x0000000000A8)
#define DC_PCS_66_REG_15_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_15_L2_FEC_UNCORRECTED_BLOCKS_SHIFT 0
#define DC_PCS_66_REG_15_L2_FEC_UNCORRECTED_BLOCKS_MASK  0xFFFFFFFFull
#define DC_PCS_66_REG_15_L2_FEC_UNCORRECTED_BLOCKS_SMASK 0xFFFFFFFFull
/*
* Table #18 of 240_CSR_pcslpe.xml - pcs_66_reg_16
* See description below for individual field definition
*/
#define DC_PCS_66_REG_16                                 (DC_PCSLPE_PCS_66 + 0x0000000000B0)
#define DC_PCS_66_REG_16_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_16_L3_FEC_CORRECTED_BLOCKS_SHIFT   0
#define DC_PCS_66_REG_16_L3_FEC_CORRECTED_BLOCKS_MASK    0xFFFFFFFFull
#define DC_PCS_66_REG_16_L3_FEC_CORRECTED_BLOCKS_SMASK   0xFFFFFFFFull
/*
* Table #19 of 240_CSR_pcslpe.xml - pcs_66_reg_17
* See description below for individual field definition
*/
#define DC_PCS_66_REG_17                                 (DC_PCSLPE_PCS_66 + 0x0000000000B8)
#define DC_PCS_66_REG_17_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_17_L3_FEC_UNCORRECTED_BLOCKS_SHIFT 0
#define DC_PCS_66_REG_17_L3_FEC_UNCORRECTED_BLOCKS_MASK  0xFFFFFFFFull
#define DC_PCS_66_REG_17_L3_FEC_UNCORRECTED_BLOCKS_SMASK 0xFFFFFFFFull
/*
* Table #20 of 240_CSR_pcslpe.xml - pcs_66_reg_18
* See description below for individual field definition
*/
#define DC_PCS_66_REG_18                                 (DC_PCSLPE_PCS_66 + 0x0000000000C0)
#define DC_PCS_66_REG_18_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_18_UNUSED_31_16_SHIFT              16
#define DC_PCS_66_REG_18_UNUSED_31_16_MASK               0xFFFFull
#define DC_PCS_66_REG_18_UNUSED_31_16_SMASK              0xFFFF0000ull
#define DC_PCS_66_REG_18_INVALID_CTRL_BLOCK_CNT_SHIFT    8
#define DC_PCS_66_REG_18_INVALID_CTRL_BLOCK_CNT_MASK     0xFFull
#define DC_PCS_66_REG_18_INVALID_CTRL_BLOCK_CNT_SMASK    0xFF00ull
#define DC_PCS_66_REG_18_INVALID_SYNC_HDR_CNT_SHIFT      0
#define DC_PCS_66_REG_18_INVALID_SYNC_HDR_CNT_MASK       0xFFull
#define DC_PCS_66_REG_18_INVALID_SYNC_HDR_CNT_SMASK      0xFFull
/*
* Table #21 of 240_CSR_pcslpe.xml - pcs_66_reg_19
* See description below for individual field definition
*/
#define DC_PCS_66_REG_19                                 (DC_PCSLPE_PCS_66 + 0x0000000000C8)
#define DC_PCS_66_REG_19_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_19_UNUSED_31_0_SHIFT               0
#define DC_PCS_66_REG_19_UNUSED_31_0_MASK                0xFFFFFFFFull
#define DC_PCS_66_REG_19_UNUSED_31_0_SMASK               0xFFFFFFFFull
/*
* Table #22 of 240_CSR_pcslpe.xml - pcs_66_reg_1A
* See description below for individual field definition
*/
#define DC_PCS_66_REG_1A                                 (DC_PCSLPE_PCS_66 + 0x0000000000D0)
#define DC_PCS_66_REG_1A_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_1A_UNUSED_31_0_SHIFT               0
#define DC_PCS_66_REG_1A_UNUSED_31_0_MASK                0xFFFFFFFFull
#define DC_PCS_66_REG_1A_UNUSED_31_0_SMASK               0xFFFFFFFFull
/*
* Table #23 of 240_CSR_pcslpe.xml - pcs_66_reg_20
* See description below for individual field definition
*/
#define DC_PCS_66_REG_20                                 (DC_PCSLPE_PCS_66 + 0x000000000100)
#define DC_PCS_66_REG_20_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_20_ERROR_DETECTION_COUNT_SHIFT     0
#define DC_PCS_66_REG_20_ERROR_DETECTION_COUNT_MASK      0xFFFFFFFFull
#define DC_PCS_66_REG_20_ERROR_DETECTION_COUNT_SMASK     0xFFFFFFFFull
/*
* Table #24 of 240_CSR_pcslpe.xml - pcs_66_reg_38
* See description below for individual field definition
*/
#define DC_PCS_66_REG_38                                 (DC_PCSLPE_PCS_66 + 0x0000000001C0)
#define DC_PCS_66_REG_38_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_38_UNUSED_31_5_SHIFT               5
#define DC_PCS_66_REG_38_UNUSED_31_5_MASK                0x7FFFFFFull
#define DC_PCS_66_REG_38_UNUSED_31_5_SMASK               0xFFFFFFE0ull
#define DC_PCS_66_REG_38_ONE_SHOT_PKT_ERR_SHIFT          0
#define DC_PCS_66_REG_38_ONE_SHOT_PKT_ERR_MASK           0x1Full
#define DC_PCS_66_REG_38_ONE_SHOT_PKT_ERR_SMASK          0x1Full
/*
* Table #25 of 240_CSR_pcslpe.xml - pcs_66_reg_39
* See description below for individual field definition
*/
#define DC_PCS_66_REG_39                                 (DC_PCSLPE_PCS_66 + 0x0000000001C8)
#define DC_PCS_66_REG_39_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_39_UNUSED_31_12_SHIFT              12
#define DC_PCS_66_REG_39_UNUSED_31_12_MASK               0xFFFFFull
#define DC_PCS_66_REG_39_UNUSED_31_12_SMASK              0xFFFFF000ull
#define DC_PCS_66_REG_39_ONE_SHOT_66B_CNT_SHIFT          4
#define DC_PCS_66_REG_39_ONE_SHOT_66B_CNT_MASK           0xFFull
#define DC_PCS_66_REG_39_ONE_SHOT_66B_CNT_SMASK          0xFF0ull
#define DC_PCS_66_REG_39_ONE_SHOT_66B_ERR_SHIFT          0
#define DC_PCS_66_REG_39_ONE_SHOT_66B_ERR_MASK           0xFull
#define DC_PCS_66_REG_39_ONE_SHOT_66B_ERR_SMASK          0xFull
/*
* Table #26 of 240_CSR_pcslpe.xml - pcs_66_reg_3A
* See description below for individual field definition
*/
#define DC_PCS_66_REG_3A                                 (DC_PCSLPE_PCS_66 + 0x0000000001D0)
#define DC_PCS_66_REG_3A_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_3A_UNUSED_31_8_SHIFT               8
#define DC_PCS_66_REG_3A_UNUSED_31_8_MASK                0xFFFFFFull
#define DC_PCS_66_REG_3A_UNUSED_31_8_SMASK               0xFFFFFF00ull
#define DC_PCS_66_REG_3A_ONE_SHOT_CTRL_BLK_ERR_SHIFT     0
#define DC_PCS_66_REG_3A_ONE_SHOT_CTRL_BLK_ERR_MASK      0xFFull
#define DC_PCS_66_REG_3A_ONE_SHOT_CTRL_BLK_ERR_SMASK     0xFFull
/*
* Table #27 of 240_CSR_pcslpe.xml - pcs_66_reg_3B
* See description below for individual field definition
*/
#define DC_PCS_66_REG_3B                                 (DC_PCSLPE_PCS_66 + 0x0000000001D8)
#define DC_PCS_66_REG_3B_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_3B_CTRL_BLK_0123_SHIFT             0
#define DC_PCS_66_REG_3B_CTRL_BLK_0123_MASK              0xFFFFFFFFull
#define DC_PCS_66_REG_3B_CTRL_BLK_0123_SMASK             0xFFFFFFFFull
/*
* Table #28 of 240_CSR_pcslpe.xml - pcs_66_reg_3C
* See description below for individual field definition
*/
#define DC_PCS_66_REG_3C                                 (DC_PCSLPE_PCS_66 + 0x0000000001E0)
#define DC_PCS_66_REG_3C_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_3C_CTRL_BLK_4567_SHIFT             0
#define DC_PCS_66_REG_3C_CTRL_BLK_4567_MASK              0xFFFFFFFFull
#define DC_PCS_66_REG_3C_CTRL_BLK_4567_SMASK             0xFFFFFFFFull
/*
* Table #2 of 240_CSR_pcslpe.xml - pcs_66_reg_0
* See description below for individual field definition
*/
#define DC_PCS_66_REG_0                                  (DC_PCSLPE_PCS_66 + 0x000000000000)
#define DC_PCS_66_REG_0_RESETCSR                         0x00000000ull
#define DC_PCS_66_REG_0_UNUSED_31_0_SHIFT                0
#define DC_PCS_66_REG_0_UNUSED_31_0_MASK                 0xFFFFFFFFull
#define DC_PCS_66_REG_0_UNUSED_31_0_SMASK                0xFFFFFFFFull
/*
* Table #3 of 240_CSR_pcslpe.xml - pcs_66_reg_1
* See description below for individual field definition
*/
#define DC_PCS_66_REG_1                                  (DC_PCSLPE_PCS_66 + 0x000000000008)
#define DC_PCS_66_REG_1_RESETCSR                         0x00000000ull
#define DC_PCS_66_REG_1_UNUSED_31_12_SHIFT               12
#define DC_PCS_66_REG_1_UNUSED_31_12_MASK                0xFFFFFull
#define DC_PCS_66_REG_1_UNUSED_31_12_SMASK               0xFFFFF000ull
#define DC_PCS_66_REG_1_PRBS_ERROR_SHIFT                 8
#define DC_PCS_66_REG_1_PRBS_ERROR_MASK                  0xFull
#define DC_PCS_66_REG_1_PRBS_ERROR_SMASK                 0xF00ull
#define DC_PCS_66_REG_1_UNUSED_7_4_SHIFT                 4
#define DC_PCS_66_REG_1_UNUSED_7_4_MASK                  0xFull
#define DC_PCS_66_REG_1_UNUSED_7_4_SMASK                 0xF0ull
#define DC_PCS_66_REG_1_SMF_OVERFLOW_SHIFT               0
#define DC_PCS_66_REG_1_SMF_OVERFLOW_MASK                0xFull
#define DC_PCS_66_REG_1_SMF_OVERFLOW_SMASK               0xFull
/*
* Table #4 of 240_CSR_pcslpe.xml - pcs_66_reg_2
* See description below for individual field definition
*/
#define DC_PCS_66_REG_2                                  (DC_PCSLPE_PCS_66 + 0x000000000010)
#define DC_PCS_66_REG_2_RESETCSR                         0x00000E40ull
#define DC_PCS_66_REG_2_UNUSED_31_12_SHIFT               12
#define DC_PCS_66_REG_2_UNUSED_31_12_MASK                0xFFFFFull
#define DC_PCS_66_REG_2_UNUSED_31_12_SMASK               0xFFFFF000ull
#define DC_PCS_66_REG_2_TX_LANE_ORDER_D_SHIFT            10
#define DC_PCS_66_REG_2_TX_LANE_ORDER_D_MASK             0x3ull
#define DC_PCS_66_REG_2_TX_LANE_ORDER_D_SMASK            0xC00ull
#define DC_PCS_66_REG_2_TX_LANE_ORDER_C_SHIFT            8
#define DC_PCS_66_REG_2_TX_LANE_ORDER_C_MASK             0x3ull
#define DC_PCS_66_REG_2_TX_LANE_ORDER_C_SMASK            0x300ull
#define DC_PCS_66_REG_2_TX_LANE_ORDER_B_SHIFT            6
#define DC_PCS_66_REG_2_TX_LANE_ORDER_B_MASK             0x3ull
#define DC_PCS_66_REG_2_TX_LANE_ORDER_B_SMASK            0xC0ull
#define DC_PCS_66_REG_2_TX_LANE_ORDER_A_SHIFT            4
#define DC_PCS_66_REG_2_TX_LANE_ORDER_A_MASK             0x3ull
#define DC_PCS_66_REG_2_TX_LANE_ORDER_A_SMASK            0x30ull
#define DC_PCS_66_REG_2_TX_LANE_POLARITY_SHIFT           0
#define DC_PCS_66_REG_2_TX_LANE_POLARITY_MASK            0xFull
#define DC_PCS_66_REG_2_TX_LANE_POLARITY_SMASK           0xFull
/*
* Table #5 of 240_CSR_pcslpe.xml - pcs_66_reg_3
* See description below for individual field definition
*/
#define DC_PCS_66_REG_3                                  (DC_PCSLPE_PCS_66 + 0x000000000018)
#define DC_PCS_66_REG_3_RESETCSR                         0x00000000ull
#define DC_PCS_66_REG_3_UNUSED_31_4_SHIFT                4
#define DC_PCS_66_REG_3_UNUSED_31_4_MASK                 0xFFFFFFFull
#define DC_PCS_66_REG_3_UNUSED_31_4_SMASK                0xFFFFFFF0ull
#define DC_PCS_66_REG_3_RX_LOCK_SHIFT                    0
#define DC_PCS_66_REG_3_RX_LOCK_MASK                     0xFull
#define DC_PCS_66_REG_3_RX_LOCK_SMASK                    0xFull
/*
* Table #6 of 240_CSR_pcslpe.xml - pcs_66_reg_4
* See description below for individual field definition
*/
#define DC_PCS_66_REG_4                                  (DC_PCSLPE_PCS_66 + 0x000000000020)
#define DC_PCS_66_REG_4_RESETCSR                         0x00000100ull
#define DC_PCS_66_REG_4_UNUSED_31_9_SHIFT                9
#define DC_PCS_66_REG_4_UNUSED_31_9_MASK                 0x7FFFFFull
#define DC_PCS_66_REG_4_UNUSED_31_9_SMASK                0xFFFFFE00ull
#define DC_PCS_66_REG_4_AUTO_POLARITY_REV_EN_SHIFT       8
#define DC_PCS_66_REG_4_AUTO_POLARITY_REV_EN_MASK        0x1ull
#define DC_PCS_66_REG_4_AUTO_POLARITY_REV_EN_SMASK       0x100ull
#define DC_PCS_66_REG_4_UNUSED_7_2_SHIFT                 2
#define DC_PCS_66_REG_4_UNUSED_7_2_MASK                  0x3Full
#define DC_PCS_66_REG_4_UNUSED_7_2_SMASK                 0xFCull
#define DC_PCS_66_REG_4_PCS_DATA_SYNC_DIS_SHIFT          1
#define DC_PCS_66_REG_4_PCS_DATA_SYNC_DIS_MASK           0x1ull
#define DC_PCS_66_REG_4_PCS_DATA_SYNC_DIS_SMASK          0x2ull
#define DC_PCS_66_REG_4_UNUSED_0_SHIFT                   0
#define DC_PCS_66_REG_4_UNUSED_0_MASK                    0x1ull
#define DC_PCS_66_REG_4_UNUSED_0_SMASK                   0x1ull
/*
* Table #7 of 240_CSR_pcslpe.xml - pcs_66_reg_5
* See description below for individual field definition
*/
#define DC_PCS_66_REG_5                                  (DC_PCSLPE_PCS_66 + 0x000000000028)
#define DC_PCS_66_REG_5_RESETCSR                         0x00000E40ull
#define DC_PCS_66_REG_5_UNUSED_31_12_SHIFT               12
#define DC_PCS_66_REG_5_UNUSED_31_12_MASK                0xFFFFFull
#define DC_PCS_66_REG_5_UNUSED_31_12_SMASK               0xFFFFF000ull
#define DC_PCS_66_REG_5_RX_LANE_ORDER_D_SHIFT            10
#define DC_PCS_66_REG_5_RX_LANE_ORDER_D_MASK             0x3ull
#define DC_PCS_66_REG_5_RX_LANE_ORDER_D_SMASK            0xC00ull
#define DC_PCS_66_REG_5_RX_LANE_ORDER_C_SHIFT            8
#define DC_PCS_66_REG_5_RX_LANE_ORDER_C_MASK             0x3ull
#define DC_PCS_66_REG_5_RX_LANE_ORDER_C_SMASK            0x300ull
#define DC_PCS_66_REG_5_RX_LANE_ORDER_B_SHIFT            6
#define DC_PCS_66_REG_5_RX_LANE_ORDER_B_MASK             0x3ull
#define DC_PCS_66_REG_5_RX_LANE_ORDER_B_SMASK            0xC0ull
#define DC_PCS_66_REG_5_RX_LANE_ORDER_A_SHIFT            4
#define DC_PCS_66_REG_5_RX_LANE_ORDER_A_MASK             0x3ull
#define DC_PCS_66_REG_5_RX_LANE_ORDER_A_SMASK            0x30ull
#define DC_PCS_66_REG_5_RX_LANE_POLARITY_SHIFT           0
#define DC_PCS_66_REG_5_RX_LANE_POLARITY_MASK            0xFull
#define DC_PCS_66_REG_5_RX_LANE_POLARITY_SMASK           0xFull
/*
* Table #8 of 240_CSR_pcslpe.xml - pcs_66_reg_6
* See description below for individual field definition
*/
#define DC_PCS_66_REG_6                                  (DC_PCSLPE_PCS_66 + 0x000000000030)
#define DC_PCS_66_REG_6_RESETCSR                         0x00000000ull
#define DC_PCS_66_REG_6_UNUSED_31_3_SHIFT                3
#define DC_PCS_66_REG_6_UNUSED_31_3_MASK                 0x1FFFFFFFull
#define DC_PCS_66_REG_6_UNUSED_31_3_SMASK                0xFFFFFFF8ull
#define DC_PCS_66_REG_6_PRBS_POLARITY_SHIFT              2
#define DC_PCS_66_REG_6_PRBS_POLARITY_MASK               0x1ull
#define DC_PCS_66_REG_6_PRBS_POLARITY_SMASK              0x4ull
#define DC_PCS_66_REG_6_PRBS_MODE_SEL_SHIFT              1
#define DC_PCS_66_REG_6_PRBS_MODE_SEL_MASK               0x1ull
#define DC_PCS_66_REG_6_PRBS_MODE_SEL_SMASK              0x2ull
#define DC_PCS_66_REG_6_PRBS_ENABLE_SHIFT                0
#define DC_PCS_66_REG_6_PRBS_ENABLE_MASK                 0x1ull
#define DC_PCS_66_REG_6_PRBS_ENABLE_SMASK                0x1ull
/*
* Table #9 of 240_CSR_pcslpe.xml - pcs_66_reg_7
* See description below for individual field definition
*/
#define DC_PCS_66_REG_7                                  (DC_PCSLPE_PCS_66 + 0x000000000038)
#define DC_PCS_66_REG_7_RESETCSR                         0x00000000ull
#define DC_PCS_66_REG_7_UNUSED_31_4_SHIFT                4
#define DC_PCS_66_REG_7_UNUSED_31_4_MASK                 0xFFFFFFFull
#define DC_PCS_66_REG_7_UNUSED_31_4_SMASK                0xFFFFFFF0ull
#define DC_PCS_66_REG_7_TRAIN_TX_PHS_FIFO_SHIFT          3
#define DC_PCS_66_REG_7_TRAIN_TX_PHS_FIFO_MASK           0x1ull
#define DC_PCS_66_REG_7_TRAIN_TX_PHS_FIFO_SMASK          0x8ull
#define DC_PCS_66_REG_7_SMF_DIAG_BUSY_SHIFT              2
#define DC_PCS_66_REG_7_SMF_DIAG_BUSY_MASK               0x1ull
#define DC_PCS_66_REG_7_SMF_DIAG_BUSY_SMASK              0x4ull
#define DC_PCS_66_REG_7_FORCE_SMF_DELETE_SHIFT           1
#define DC_PCS_66_REG_7_FORCE_SMF_DELETE_MASK            0x1ull
#define DC_PCS_66_REG_7_FORCE_SMF_DELETE_SMASK           0x2ull
#define DC_PCS_66_REG_7_FORCE_SMF_INSERT_SHIFT           0
#define DC_PCS_66_REG_7_FORCE_SMF_INSERT_MASK            0x1ull
#define DC_PCS_66_REG_7_FORCE_SMF_INSERT_SMASK           0x1ull
/*
* Table #10 of 240_CSR_pcslpe.xml - pcs_66_reg_8
* See description below for individual field definition
*/
#define DC_PCS_66_REG_8                                  (DC_PCSLPE_PCS_66 + 0x000000000040)
#define DC_PCS_66_REG_8_RESETCSR                         0x00000000ull
#define DC_PCS_66_REG_8_UNUSED_31_9_SHIFT                9
#define DC_PCS_66_REG_8_UNUSED_31_9_MASK                 0x7FFFFFull
#define DC_PCS_66_REG_8_UNUSED_31_9_SMASK                0xFFFFFE00ull
#define DC_PCS_66_REG_8_RST_ON_READ_SHIFT                8
#define DC_PCS_66_REG_8_RST_ON_READ_MASK                 0x1ull
#define DC_PCS_66_REG_8_RST_ON_READ_SMASK                0x100ull
#define DC_PCS_66_REG_8_UNUSED_7_1_SHIFT                 1
#define DC_PCS_66_REG_8_UNUSED_7_1_MASK                  0x7Full
#define DC_PCS_66_REG_8_UNUSED_7_1_SMASK                 0xFEull
#define DC_PCS_66_REG_8_FEC_ENABLE_SHIFT                 0
#define DC_PCS_66_REG_8_FEC_ENABLE_MASK                  0x1ull
#define DC_PCS_66_REG_8_FEC_ENABLE_SMASK                 0x1ull
/*
* Table #11 of 240_CSR_pcslpe.xml - pcs_66_reg_9
* See description below for individual field definition
*/
#define DC_PCS_66_REG_9                                  (DC_PCSLPE_PCS_66 + 0x000000000048)
#define DC_PCS_66_REG_9_RESETCSR                         0x00000000ull
#define DC_PCS_66_REG_9_UNUSED_31_0_SHIFT                0
#define DC_PCS_66_REG_9_UNUSED_31_0_MASK                 0xFFFFFFFFull
#define DC_PCS_66_REG_9_UNUSED_31_0_SMASK                0xFFFFFFFFull
/*
* Table #12 of 240_CSR_pcslpe.xml - pcs_66_reg_10
* See description below for individual field definition
*/
#define DC_PCS_66_REG_10                                 (DC_PCSLPE_PCS_66 + 0x000000000080)
#define DC_PCS_66_REG_10_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_10_L0_FEC_CORRECTED_BLOCKS_SHIFT   0
#define DC_PCS_66_REG_10_L0_FEC_CORRECTED_BLOCKS_MASK    0xFFFFFFFFull
#define DC_PCS_66_REG_10_L0_FEC_CORRECTED_BLOCKS_SMASK   0xFFFFFFFFull
/*
* Table #13 of 240_CSR_pcslpe.xml - pcs_66_reg_11
* See description below for individual field definition
*/
#define DC_PCS_66_REG_11                                 (DC_PCSLPE_PCS_66 + 0x000000000088)
#define DC_PCS_66_REG_11_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_11_L0_FEC_UNCORRECTED_BLOCKS_SHIFT 0
#define DC_PCS_66_REG_11_L0_FEC_UNCORRECTED_BLOCKS_MASK  0xFFFFFFFFull
#define DC_PCS_66_REG_11_L0_FEC_UNCORRECTED_BLOCKS_SMASK 0xFFFFFFFFull
/*
* Table #14 of 240_CSR_pcslpe.xml - pcs_66_reg_12
* See description below for individual field definition
*/
#define DC_PCS_66_REG_12                                 (DC_PCSLPE_PCS_66 + 0x000000000090)
#define DC_PCS_66_REG_12_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_12_L1_FEC_CORRECTED_BLOCKS_SHIFT   0
#define DC_PCS_66_REG_12_L1_FEC_CORRECTED_BLOCKS_MASK    0xFFFFFFFFull
#define DC_PCS_66_REG_12_L1_FEC_CORRECTED_BLOCKS_SMASK   0xFFFFFFFFull
/*
* Table #15 of 240_CSR_pcslpe.xml - pcs_66_reg_13
* See description below for individual field definition
*/
#define DC_PCS_66_REG_13                                 (DC_PCSLPE_PCS_66 + 0x000000000098)
#define DC_PCS_66_REG_13_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_13_L1_FEC_UNCORRECTED_BLOCKS_SHIFT 0
#define DC_PCS_66_REG_13_L1_FEC_UNCORRECTED_BLOCKS_MASK  0xFFFFFFFFull
#define DC_PCS_66_REG_13_L1_FEC_UNCORRECTED_BLOCKS_SMASK 0xFFFFFFFFull
/*
* Table #16 of 240_CSR_pcslpe.xml - pcs_66_reg_14
* See description below for individual field definition
*/
#define DC_PCS_66_REG_14                                 (DC_PCSLPE_PCS_66 + 0x0000000000A0)
#define DC_PCS_66_REG_14_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_14_L2_FEC_CORRECTED_BLOCKS_SHIFT   0
#define DC_PCS_66_REG_14_L2_FEC_CORRECTED_BLOCKS_MASK    0xFFFFFFFFull
#define DC_PCS_66_REG_14_L2_FEC_CORRECTED_BLOCKS_SMASK   0xFFFFFFFFull
/*
* Table #17 of 240_CSR_pcslpe.xml - pcs_66_reg_15
* See description below for individual field definition
*/
#define DC_PCS_66_REG_15                                 (DC_PCSLPE_PCS_66 + 0x0000000000A8)
#define DC_PCS_66_REG_15_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_15_L2_FEC_UNCORRECTED_BLOCKS_SHIFT 0
#define DC_PCS_66_REG_15_L2_FEC_UNCORRECTED_BLOCKS_MASK  0xFFFFFFFFull
#define DC_PCS_66_REG_15_L2_FEC_UNCORRECTED_BLOCKS_SMASK 0xFFFFFFFFull
/*
* Table #18 of 240_CSR_pcslpe.xml - pcs_66_reg_16
* See description below for individual field definition
*/
#define DC_PCS_66_REG_16                                 (DC_PCSLPE_PCS_66 + 0x0000000000B0)
#define DC_PCS_66_REG_16_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_16_L3_FEC_CORRECTED_BLOCKS_SHIFT   0
#define DC_PCS_66_REG_16_L3_FEC_CORRECTED_BLOCKS_MASK    0xFFFFFFFFull
#define DC_PCS_66_REG_16_L3_FEC_CORRECTED_BLOCKS_SMASK   0xFFFFFFFFull
/*
* Table #19 of 240_CSR_pcslpe.xml - pcs_66_reg_17
* See description below for individual field definition
*/
#define DC_PCS_66_REG_17                                 (DC_PCSLPE_PCS_66 + 0x0000000000B8)
#define DC_PCS_66_REG_17_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_17_L3_FEC_UNCORRECTED_BLOCKS_SHIFT 0
#define DC_PCS_66_REG_17_L3_FEC_UNCORRECTED_BLOCKS_MASK  0xFFFFFFFFull
#define DC_PCS_66_REG_17_L3_FEC_UNCORRECTED_BLOCKS_SMASK 0xFFFFFFFFull
/*
* Table #20 of 240_CSR_pcslpe.xml - pcs_66_reg_18
* See description below for individual field definition
*/
#define DC_PCS_66_REG_18                                 (DC_PCSLPE_PCS_66 + 0x0000000000C0)
#define DC_PCS_66_REG_18_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_18_UNUSED_31_16_SHIFT              16
#define DC_PCS_66_REG_18_UNUSED_31_16_MASK               0xFFFFull
#define DC_PCS_66_REG_18_UNUSED_31_16_SMASK              0xFFFF0000ull
#define DC_PCS_66_REG_18_INVALID_CTRL_BLOCK_CNT_SHIFT    8
#define DC_PCS_66_REG_18_INVALID_CTRL_BLOCK_CNT_MASK     0xFFull
#define DC_PCS_66_REG_18_INVALID_CTRL_BLOCK_CNT_SMASK    0xFF00ull
#define DC_PCS_66_REG_18_INVALID_SYNC_HDR_CNT_SHIFT      0
#define DC_PCS_66_REG_18_INVALID_SYNC_HDR_CNT_MASK       0xFFull
#define DC_PCS_66_REG_18_INVALID_SYNC_HDR_CNT_SMASK      0xFFull
/*
* Table #21 of 240_CSR_pcslpe.xml - pcs_66_reg_19
* See description below for individual field definition
*/
#define DC_PCS_66_REG_19                                 (DC_PCSLPE_PCS_66 + 0x0000000000C8)
#define DC_PCS_66_REG_19_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_19_UNUSED_31_0_SHIFT               0
#define DC_PCS_66_REG_19_UNUSED_31_0_MASK                0xFFFFFFFFull
#define DC_PCS_66_REG_19_UNUSED_31_0_SMASK               0xFFFFFFFFull
/*
* Table #22 of 240_CSR_pcslpe.xml - pcs_66_reg_1A
* See description below for individual field definition
*/
#define DC_PCS_66_REG_1A                                 (DC_PCSLPE_PCS_66 + 0x0000000000D0)
#define DC_PCS_66_REG_1A_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_1A_UNUSED_31_0_SHIFT               0
#define DC_PCS_66_REG_1A_UNUSED_31_0_MASK                0xFFFFFFFFull
#define DC_PCS_66_REG_1A_UNUSED_31_0_SMASK               0xFFFFFFFFull
/*
* Table #23 of 240_CSR_pcslpe.xml - pcs_66_reg_20
* See description below for individual field definition
*/
#define DC_PCS_66_REG_20                                 (DC_PCSLPE_PCS_66 + 0x000000000100)
#define DC_PCS_66_REG_20_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_20_ERROR_DETECTION_COUNT_SHIFT     0
#define DC_PCS_66_REG_20_ERROR_DETECTION_COUNT_MASK      0xFFFFFFFFull
#define DC_PCS_66_REG_20_ERROR_DETECTION_COUNT_SMASK     0xFFFFFFFFull
/*
* Table #24 of 240_CSR_pcslpe.xml - pcs_66_reg_38
* See description below for individual field definition
*/
#define DC_PCS_66_REG_38                                 (DC_PCSLPE_PCS_66 + 0x0000000001C0)
#define DC_PCS_66_REG_38_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_38_UNUSED_31_5_SHIFT               5
#define DC_PCS_66_REG_38_UNUSED_31_5_MASK                0x7FFFFFFull
#define DC_PCS_66_REG_38_UNUSED_31_5_SMASK               0xFFFFFFE0ull
#define DC_PCS_66_REG_38_ONE_SHOT_PKT_ERR_SHIFT          0
#define DC_PCS_66_REG_38_ONE_SHOT_PKT_ERR_MASK           0x1Full
#define DC_PCS_66_REG_38_ONE_SHOT_PKT_ERR_SMASK          0x1Full
/*
* Table #25 of 240_CSR_pcslpe.xml - pcs_66_reg_39
* See description below for individual field definition
*/
#define DC_PCS_66_REG_39                                 (DC_PCSLPE_PCS_66 + 0x0000000001C8)
#define DC_PCS_66_REG_39_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_39_UNUSED_31_12_SHIFT              12
#define DC_PCS_66_REG_39_UNUSED_31_12_MASK               0xFFFFFull
#define DC_PCS_66_REG_39_UNUSED_31_12_SMASK              0xFFFFF000ull
#define DC_PCS_66_REG_39_ONE_SHOT_66B_CNT_SHIFT          4
#define DC_PCS_66_REG_39_ONE_SHOT_66B_CNT_MASK           0xFFull
#define DC_PCS_66_REG_39_ONE_SHOT_66B_CNT_SMASK          0xFF0ull
#define DC_PCS_66_REG_39_ONE_SHOT_66B_ERR_SHIFT          0
#define DC_PCS_66_REG_39_ONE_SHOT_66B_ERR_MASK           0xFull
#define DC_PCS_66_REG_39_ONE_SHOT_66B_ERR_SMASK          0xFull
/*
* Table #26 of 240_CSR_pcslpe.xml - pcs_66_reg_3A
* See description below for individual field definition
*/
#define DC_PCS_66_REG_3A                                 (DC_PCSLPE_PCS_66 + 0x0000000001D0)
#define DC_PCS_66_REG_3A_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_3A_UNUSED_31_8_SHIFT               8
#define DC_PCS_66_REG_3A_UNUSED_31_8_MASK                0xFFFFFFull
#define DC_PCS_66_REG_3A_UNUSED_31_8_SMASK               0xFFFFFF00ull
#define DC_PCS_66_REG_3A_ONE_SHOT_CTRL_BLK_ERR_SHIFT     0
#define DC_PCS_66_REG_3A_ONE_SHOT_CTRL_BLK_ERR_MASK      0xFFull
#define DC_PCS_66_REG_3A_ONE_SHOT_CTRL_BLK_ERR_SMASK     0xFFull
/*
* Table #27 of 240_CSR_pcslpe.xml - pcs_66_reg_3B
* See description below for individual field definition
*/
#define DC_PCS_66_REG_3B                                 (DC_PCSLPE_PCS_66 + 0x0000000001D8)
#define DC_PCS_66_REG_3B_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_3B_CTRL_BLK_0123_SHIFT             0
#define DC_PCS_66_REG_3B_CTRL_BLK_0123_MASK              0xFFFFFFFFull
#define DC_PCS_66_REG_3B_CTRL_BLK_0123_SMASK             0xFFFFFFFFull
/*
* Table #28 of 240_CSR_pcslpe.xml - pcs_66_reg_3C
* See description below for individual field definition
*/
#define DC_PCS_66_REG_3C                                 (DC_PCSLPE_PCS_66 + 0x0000000001E0)
#define DC_PCS_66_REG_3C_RESETCSR                        0x00000000ull
#define DC_PCS_66_REG_3C_CTRL_BLK_4567_SHIFT             0
#define DC_PCS_66_REG_3C_CTRL_BLK_4567_MASK              0xFFFFFFFFull
#define DC_PCS_66_REG_3C_CTRL_BLK_4567_SMASK             0xFFFFFFFFull
