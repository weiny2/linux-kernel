/*
* Table #2 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_0
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_0                            (DC_PCSLPE_PCS_8B10B + 0x000000000000)
#define DC_PCS_8B10B_REG_0_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_0_UNUSED_31_0_SHIFT          0
#define DC_PCS_8B10B_REG_0_UNUSED_31_0_MASK           0xFFFFFFFFull
#define DC_PCS_8B10B_REG_0_UNUSED_31_0_SMASK          0xFFFFFFFFull
/*
* Table #3 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_1
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_1                            (DC_PCSLPE_PCS_8B10B + 0x000000000008)
#define DC_PCS_8B10B_REG_1_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_1_UNUSED_31_4_SHIFT          4
#define DC_PCS_8B10B_REG_1_UNUSED_31_4_MASK           0xFFFFFFFull
#define DC_PCS_8B10B_REG_1_UNUSED_31_4_SMASK          0xFFFFFFF0ull
#define DC_PCS_8B10B_REG_1_SMF_OVERFLOW_SHIFT         0
#define DC_PCS_8B10B_REG_1_SMF_OVERFLOW_MASK          0xFull
#define DC_PCS_8B10B_REG_1_SMF_OVERFLOW_SMASK         0xFull
/*
* Table #4 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_2
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_2                            (DC_PCSLPE_PCS_8B10B + 0x000000000010)
#define DC_PCS_8B10B_REG_2_RESETCSR                   0x00000E40ull
#define DC_PCS_8B10B_REG_2_UNUSED_31_12_SHIFT         12
#define DC_PCS_8B10B_REG_2_UNUSED_31_12_MASK          0xFFFFFull
#define DC_PCS_8B10B_REG_2_UNUSED_31_12_SMASK         0xFFFFF000ull
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_D_SHIFT      10
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_D_MASK       0x3ull
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_D_SMASK      0xC00ull
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_C_SHIFT      8
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_C_MASK       0x3ull
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_C_SMASK      0x300ull
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_B_SHIFT      6
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_B_MASK       0x3ull
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_B_SMASK      0xC0ull
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_A_SHIFT      4
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_A_MASK       0x3ull
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_A_SMASK      0x30ull
#define DC_PCS_8B10B_REG_2_TX_LANE_POLARITY_SHIFT     0
#define DC_PCS_8B10B_REG_2_TX_LANE_POLARITY_MASK      0xFull
#define DC_PCS_8B10B_REG_2_TX_LANE_POLARITY_SMASK     0xFull
/*
* Table #5 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_3
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_3                            (DC_PCSLPE_PCS_8B10B + 0x000000000018)
#define DC_PCS_8B10B_REG_3_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_3_UNUSED_31_4_SHIFT          4
#define DC_PCS_8B10B_REG_3_UNUSED_31_4_MASK           0xFFFFFFFull
#define DC_PCS_8B10B_REG_3_UNUSED_31_4_SMASK          0xFFFFFFF0ull
#define DC_PCS_8B10B_REG_3_RX_LOCK_SHIFT              0
#define DC_PCS_8B10B_REG_3_RX_LOCK_MASK               0xFull
#define DC_PCS_8B10B_REG_3_RX_LOCK_SMASK              0xFull
/*
* Table #6 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_4
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_4                            (DC_PCSLPE_PCS_8B10B + 0x000000000020)
#define DC_PCS_8B10B_REG_4_RESETCSR                   0x00000100ull
#define DC_PCS_8B10B_REG_4_UNUSED_31_9_SHIFT          9
#define DC_PCS_8B10B_REG_4_UNUSED_31_9_MASK           0x7FFFFFull
#define DC_PCS_8B10B_REG_4_UNUSED_31_9_SMASK          0xFFFFFE00ull
#define DC_PCS_8B10B_REG_4_AUTO_POLARITY_REV_EN_SHIFT 8
#define DC_PCS_8B10B_REG_4_AUTO_POLARITY_REV_EN_MASK  0x1ull
#define DC_PCS_8B10B_REG_4_AUTO_POLARITY_REV_EN_SMASK 0x100ull
#define DC_PCS_8B10B_REG_4_UNUSED_7_2_SHIFT           2
#define DC_PCS_8B10B_REG_4_UNUSED_7_2_MASK            0x3Full
#define DC_PCS_8B10B_REG_4_UNUSED_7_2_SMASK           0xFCull
#define DC_PCS_8B10B_REG_4_PCS_DATA_SYNC_DIS_SHIFT    1
#define DC_PCS_8B10B_REG_4_PCS_DATA_SYNC_DIS_MASK     0x1ull
#define DC_PCS_8B10B_REG_4_PCS_DATA_SYNC_DIS_SMASK    0x2ull
#define DC_PCS_8B10B_REG_4_UNUSED_0_SHIFT             0
#define DC_PCS_8B10B_REG_4_UNUSED_0_MASK              0x1ull
#define DC_PCS_8B10B_REG_4_UNUSED_0_SMASK             0x1ull
/*
* Table #7 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_5
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_5                            (DC_PCSLPE_PCS_8B10B + 0x000000000028)
#define DC_PCS_8B10B_REG_5_RESETCSR                   0x00000E40ull
#define DC_PCS_8B10B_REG_5_UNUSED_31_12_SHIFT         12
#define DC_PCS_8B10B_REG_5_UNUSED_31_12_MASK          0xFFFFFull
#define DC_PCS_8B10B_REG_5_UNUSED_31_12_SMASK         0xFFFFF000ull
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_D_SHIFT      10
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_D_MASK       0x3ull
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_D_SMASK      0xC00ull
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_C_SHIFT      8
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_C_MASK       0x3ull
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_C_SMASK      0x300ull
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_B_SHIFT      6
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_B_MASK       0x3ull
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_B_SMASK      0xC0ull
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_A_SHIFT      4
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_A_MASK       0x3ull
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_A_SMASK      0x30ull
#define DC_PCS_8B10B_REG_5_RX_LANE_POLARITY_SHIFT     0
#define DC_PCS_8B10B_REG_5_RX_LANE_POLARITY_MASK      0xFull
#define DC_PCS_8B10B_REG_5_RX_LANE_POLARITY_SMASK     0xFull
/*
* Table #8 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_7
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_7                            (DC_PCSLPE_PCS_8B10B + 0x000000000038)
#define DC_PCS_8B10B_REG_7_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_7_UNUSED_31_4_SHIFT          4
#define DC_PCS_8B10B_REG_7_UNUSED_31_4_MASK           0xFFFFFFFull
#define DC_PCS_8B10B_REG_7_UNUSED_31_4_SMASK          0xFFFFFFF0ull
#define DC_PCS_8B10B_REG_7_TRAIN_TX_PHS_FIFO_SHIFT    3
#define DC_PCS_8B10B_REG_7_TRAIN_TX_PHS_FIFO_MASK     0x1ull
#define DC_PCS_8B10B_REG_7_TRAIN_TX_PHS_FIFO_SMASK    0x8ull
#define DC_PCS_8B10B_REG_7_SMF_DIAG_BUSY_SHIFT        2
#define DC_PCS_8B10B_REG_7_SMF_DIAG_BUSY_MASK         0x1ull
#define DC_PCS_8B10B_REG_7_SMF_DIAG_BUSY_SMASK        0x4ull
#define DC_PCS_8B10B_REG_7_FORCE_SMF_DELETE_SHIFT     1
#define DC_PCS_8B10B_REG_7_FORCE_SMF_DELETE_MASK      0x1ull
#define DC_PCS_8B10B_REG_7_FORCE_SMF_DELETE_SMASK     0x2ull
#define DC_PCS_8B10B_REG_7_FORCE_SMF_INSERT_SHIFT     0
#define DC_PCS_8B10B_REG_7_FORCE_SMF_INSERT_MASK      0x1ull
#define DC_PCS_8B10B_REG_7_FORCE_SMF_INSERT_SMASK     0x1ull
/*
* Table #9 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_8
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_8                            (DC_PCSLPE_PCS_8B10B + 0x000000000040)
#define DC_PCS_8B10B_REG_8_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_8_UNUSED_31_9_SHIFT          9
#define DC_PCS_8B10B_REG_8_UNUSED_31_9_MASK           0x7FFFFFull
#define DC_PCS_8B10B_REG_8_UNUSED_31_9_SMASK          0xFFFFFE00ull
#define DC_PCS_8B10B_REG_8_RST_ON_READ_SHIFT          8
#define DC_PCS_8B10B_REG_8_RST_ON_READ_MASK           0x1ull
#define DC_PCS_8B10B_REG_8_RST_ON_READ_SMASK          0x100ull
#define DC_PCS_8B10B_REG_8_UNUSED_7_0_SHIFT           0
#define DC_PCS_8B10B_REG_8_UNUSED_7_0_MASK            0xFFull
#define DC_PCS_8B10B_REG_8_UNUSED_7_0_SMASK           0xFFull
/*
* Table #10 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_9
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_9                            (DC_PCSLPE_PCS_8B10B + 0x000000000048)
#define DC_PCS_8B10B_REG_9_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_9_UNUSED_31_0_SHIFT          0
#define DC_PCS_8B10B_REG_9_UNUSED_31_0_MASK           0xFFFFFFFFull
#define DC_PCS_8B10B_REG_9_UNUSED_31_0_SMASK          0xFFFFFFFFull
/*
* Table #11 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_A
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_A                            (DC_PCSLPE_PCS_8B10B + 0x000000000050)
#define DC_PCS_8B10B_REG_A_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_A_RX_L0_DECODE_ERR_CNT_SHIFT 16
#define DC_PCS_8B10B_REG_A_RX_L0_DECODE_ERR_CNT_MASK  0xFFFFull
#define DC_PCS_8B10B_REG_A_RX_L0_DECODE_ERR_CNT_SMASK 0xFFFF0000ull
#define DC_PCS_8B10B_REG_A_RX_L1_DECODE_ERR_CNT_SHIFT 0
#define DC_PCS_8B10B_REG_A_RX_L1_DECODE_ERR_CNT_MASK  0xFFFFull
#define DC_PCS_8B10B_REG_A_RX_L1_DECODE_ERR_CNT_SMASK 0xFFFFull
/*
* Table #12 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_B
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_B                            (DC_PCSLPE_PCS_8B10B + 0x000000000058)
#define DC_PCS_8B10B_REG_B_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_B_RX_L2_DECODE_ERR_CNT_SHIFT 16
#define DC_PCS_8B10B_REG_B_RX_L2_DECODE_ERR_CNT_MASK  0xFFFFull
#define DC_PCS_8B10B_REG_B_RX_L2_DECODE_ERR_CNT_SMASK 0xFFFF0000ull
#define DC_PCS_8B10B_REG_B_RX_L3_DECODE_ERR_CNT_SHIFT 0
#define DC_PCS_8B10B_REG_B_RX_L3_DECODE_ERR_CNT_MASK  0xFFFFull
#define DC_PCS_8B10B_REG_B_RX_L3_DECODE_ERR_CNT_SMASK 0xFFFFull
/*
* Table #13 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_C
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_C                            (DC_PCSLPE_PCS_8B10B + 0x000000000060)
#define DC_PCS_8B10B_REG_C_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_C_UNUSED_31_14_SHIFT         14
#define DC_PCS_8B10B_REG_C_UNUSED_31_14_MASK          0x3FFFFull
#define DC_PCS_8B10B_REG_C_UNUSED_31_14_SMASK         0xFFFFC000ull
#define DC_PCS_8B10B_REG_C_MINOR_ERRORS_SHIFT         0
#define DC_PCS_8B10B_REG_C_MINOR_ERRORS_MASK          0x3FFFull
#define DC_PCS_8B10B_REG_C_MINOR_ERRORS_SMASK         0x3FFFull
/*
* Table #14 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_38
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_38                           (DC_PCSLPE_PCS_8B10B + 0x0000000001C0)
#define DC_PCS_8B10B_REG_38_RESETCSR                  0x00000000ull
#define DC_PCS_8B10B_REG_38_UNUSED_31_27_SHIFT        27
#define DC_PCS_8B10B_REG_38_UNUSED_31_27_MASK         0x1Full
#define DC_PCS_8B10B_REG_38_UNUSED_31_27_SMASK        0xF8000000ull
#define DC_PCS_8B10B_REG_38_ONE_SHOT_10B_CNT_SHIFT    24
#define DC_PCS_8B10B_REG_38_ONE_SHOT_10B_CNT_MASK     0x7ull
#define DC_PCS_8B10B_REG_38_ONE_SHOT_10B_CNT_SMASK    0x7000000ull
#define DC_PCS_8B10B_REG_38_ONE_SHOT_10B_ERR_SHIFT    16
#define DC_PCS_8B10B_REG_38_ONE_SHOT_10B_ERR_MASK     0xFFull
#define DC_PCS_8B10B_REG_38_ONE_SHOT_10B_ERR_SMASK    0xFF0000ull
#define DC_PCS_8B10B_REG_38_ONE_SHOT_PE_ERR_SHIFT     8
#define DC_PCS_8B10B_REG_38_ONE_SHOT_PE_ERR_MASK      0xFFull
#define DC_PCS_8B10B_REG_38_ONE_SHOT_PE_ERR_SMASK     0xFF00ull
#define DC_PCS_8B10B_REG_38_UNUSED_7_5_SHIFT          5
#define DC_PCS_8B10B_REG_38_UNUSED_7_5_MASK           0x7ull
#define DC_PCS_8B10B_REG_38_UNUSED_7_5_SMASK          0xE0ull
#define DC_PCS_8B10B_REG_38_ONE_SHOT_PKT_ERR_SHIFT    0
#define DC_PCS_8B10B_REG_38_ONE_SHOT_PKT_ERR_MASK     0x1Full
#define DC_PCS_8B10B_REG_38_ONE_SHOT_PKT_ERR_SMASK    0x1Full
/*
* Table #2 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_0
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_0                            (DC_PCSLPE_PCS_8B10B + 0x000000000000)
#define DC_PCS_8B10B_REG_0_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_0_UNUSED_31_0_SHIFT          0
#define DC_PCS_8B10B_REG_0_UNUSED_31_0_MASK           0xFFFFFFFFull
#define DC_PCS_8B10B_REG_0_UNUSED_31_0_SMASK          0xFFFFFFFFull
/*
* Table #3 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_1
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_1                            (DC_PCSLPE_PCS_8B10B + 0x000000000008)
#define DC_PCS_8B10B_REG_1_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_1_UNUSED_31_4_SHIFT          4
#define DC_PCS_8B10B_REG_1_UNUSED_31_4_MASK           0xFFFFFFFull
#define DC_PCS_8B10B_REG_1_UNUSED_31_4_SMASK          0xFFFFFFF0ull
#define DC_PCS_8B10B_REG_1_SMF_OVERFLOW_SHIFT         0
#define DC_PCS_8B10B_REG_1_SMF_OVERFLOW_MASK          0xFull
#define DC_PCS_8B10B_REG_1_SMF_OVERFLOW_SMASK         0xFull
/*
* Table #4 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_2
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_2                            (DC_PCSLPE_PCS_8B10B + 0x000000000010)
#define DC_PCS_8B10B_REG_2_RESETCSR                   0x00000E40ull
#define DC_PCS_8B10B_REG_2_UNUSED_31_12_SHIFT         12
#define DC_PCS_8B10B_REG_2_UNUSED_31_12_MASK          0xFFFFFull
#define DC_PCS_8B10B_REG_2_UNUSED_31_12_SMASK         0xFFFFF000ull
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_D_SHIFT      10
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_D_MASK       0x3ull
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_D_SMASK      0xC00ull
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_C_SHIFT      8
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_C_MASK       0x3ull
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_C_SMASK      0x300ull
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_B_SHIFT      6
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_B_MASK       0x3ull
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_B_SMASK      0xC0ull
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_A_SHIFT      4
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_A_MASK       0x3ull
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_A_SMASK      0x30ull
#define DC_PCS_8B10B_REG_2_TX_LANE_POLARITY_SHIFT     0
#define DC_PCS_8B10B_REG_2_TX_LANE_POLARITY_MASK      0xFull
#define DC_PCS_8B10B_REG_2_TX_LANE_POLARITY_SMASK     0xFull
/*
* Table #5 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_3
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_3                            (DC_PCSLPE_PCS_8B10B + 0x000000000018)
#define DC_PCS_8B10B_REG_3_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_3_UNUSED_31_4_SHIFT          4
#define DC_PCS_8B10B_REG_3_UNUSED_31_4_MASK           0xFFFFFFFull
#define DC_PCS_8B10B_REG_3_UNUSED_31_4_SMASK          0xFFFFFFF0ull
#define DC_PCS_8B10B_REG_3_RX_LOCK_SHIFT              0
#define DC_PCS_8B10B_REG_3_RX_LOCK_MASK               0xFull
#define DC_PCS_8B10B_REG_3_RX_LOCK_SMASK              0xFull
/*
* Table #6 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_4
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_4                            (DC_PCSLPE_PCS_8B10B + 0x000000000020)
#define DC_PCS_8B10B_REG_4_RESETCSR                   0x00000100ull
#define DC_PCS_8B10B_REG_4_UNUSED_31_9_SHIFT          9
#define DC_PCS_8B10B_REG_4_UNUSED_31_9_MASK           0x7FFFFFull
#define DC_PCS_8B10B_REG_4_UNUSED_31_9_SMASK          0xFFFFFE00ull
#define DC_PCS_8B10B_REG_4_AUTO_POLARITY_REV_EN_SHIFT 8
#define DC_PCS_8B10B_REG_4_AUTO_POLARITY_REV_EN_MASK  0x1ull
#define DC_PCS_8B10B_REG_4_AUTO_POLARITY_REV_EN_SMASK 0x100ull
#define DC_PCS_8B10B_REG_4_UNUSED_7_2_SHIFT           2
#define DC_PCS_8B10B_REG_4_UNUSED_7_2_MASK            0x3Full
#define DC_PCS_8B10B_REG_4_UNUSED_7_2_SMASK           0xFCull
#define DC_PCS_8B10B_REG_4_PCS_DATA_SYNC_DIS_SHIFT    1
#define DC_PCS_8B10B_REG_4_PCS_DATA_SYNC_DIS_MASK     0x1ull
#define DC_PCS_8B10B_REG_4_PCS_DATA_SYNC_DIS_SMASK    0x2ull
#define DC_PCS_8B10B_REG_4_UNUSED_0_SHIFT             0
#define DC_PCS_8B10B_REG_4_UNUSED_0_MASK              0x1ull
#define DC_PCS_8B10B_REG_4_UNUSED_0_SMASK             0x1ull
/*
* Table #7 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_5
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_5                            (DC_PCSLPE_PCS_8B10B + 0x000000000028)
#define DC_PCS_8B10B_REG_5_RESETCSR                   0x00000E40ull
#define DC_PCS_8B10B_REG_5_UNUSED_31_12_SHIFT         12
#define DC_PCS_8B10B_REG_5_UNUSED_31_12_MASK          0xFFFFFull
#define DC_PCS_8B10B_REG_5_UNUSED_31_12_SMASK         0xFFFFF000ull
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_D_SHIFT      10
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_D_MASK       0x3ull
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_D_SMASK      0xC00ull
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_C_SHIFT      8
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_C_MASK       0x3ull
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_C_SMASK      0x300ull
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_B_SHIFT      6
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_B_MASK       0x3ull
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_B_SMASK      0xC0ull
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_A_SHIFT      4
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_A_MASK       0x3ull
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_A_SMASK      0x30ull
#define DC_PCS_8B10B_REG_5_RX_LANE_POLARITY_SHIFT     0
#define DC_PCS_8B10B_REG_5_RX_LANE_POLARITY_MASK      0xFull
#define DC_PCS_8B10B_REG_5_RX_LANE_POLARITY_SMASK     0xFull
/*
* Table #8 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_7
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_7                            (DC_PCSLPE_PCS_8B10B + 0x000000000038)
#define DC_PCS_8B10B_REG_7_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_7_UNUSED_31_4_SHIFT          4
#define DC_PCS_8B10B_REG_7_UNUSED_31_4_MASK           0xFFFFFFFull
#define DC_PCS_8B10B_REG_7_UNUSED_31_4_SMASK          0xFFFFFFF0ull
#define DC_PCS_8B10B_REG_7_TRAIN_TX_PHS_FIFO_SHIFT    3
#define DC_PCS_8B10B_REG_7_TRAIN_TX_PHS_FIFO_MASK     0x1ull
#define DC_PCS_8B10B_REG_7_TRAIN_TX_PHS_FIFO_SMASK    0x8ull
#define DC_PCS_8B10B_REG_7_SMF_DIAG_BUSY_SHIFT        2
#define DC_PCS_8B10B_REG_7_SMF_DIAG_BUSY_MASK         0x1ull
#define DC_PCS_8B10B_REG_7_SMF_DIAG_BUSY_SMASK        0x4ull
#define DC_PCS_8B10B_REG_7_FORCE_SMF_DELETE_SHIFT     1
#define DC_PCS_8B10B_REG_7_FORCE_SMF_DELETE_MASK      0x1ull
#define DC_PCS_8B10B_REG_7_FORCE_SMF_DELETE_SMASK     0x2ull
#define DC_PCS_8B10B_REG_7_FORCE_SMF_INSERT_SHIFT     0
#define DC_PCS_8B10B_REG_7_FORCE_SMF_INSERT_MASK      0x1ull
#define DC_PCS_8B10B_REG_7_FORCE_SMF_INSERT_SMASK     0x1ull
/*
* Table #9 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_8
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_8                            (DC_PCSLPE_PCS_8B10B + 0x000000000040)
#define DC_PCS_8B10B_REG_8_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_8_UNUSED_31_9_SHIFT          9
#define DC_PCS_8B10B_REG_8_UNUSED_31_9_MASK           0x7FFFFFull
#define DC_PCS_8B10B_REG_8_UNUSED_31_9_SMASK          0xFFFFFE00ull
#define DC_PCS_8B10B_REG_8_RST_ON_READ_SHIFT          8
#define DC_PCS_8B10B_REG_8_RST_ON_READ_MASK           0x1ull
#define DC_PCS_8B10B_REG_8_RST_ON_READ_SMASK          0x100ull
#define DC_PCS_8B10B_REG_8_UNUSED_7_0_SHIFT           0
#define DC_PCS_8B10B_REG_8_UNUSED_7_0_MASK            0xFFull
#define DC_PCS_8B10B_REG_8_UNUSED_7_0_SMASK           0xFFull
/*
* Table #10 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_9
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_9                            (DC_PCSLPE_PCS_8B10B + 0x000000000048)
#define DC_PCS_8B10B_REG_9_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_9_UNUSED_31_0_SHIFT          0
#define DC_PCS_8B10B_REG_9_UNUSED_31_0_MASK           0xFFFFFFFFull
#define DC_PCS_8B10B_REG_9_UNUSED_31_0_SMASK          0xFFFFFFFFull
/*
* Table #11 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_A
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_A                            (DC_PCSLPE_PCS_8B10B + 0x000000000050)
#define DC_PCS_8B10B_REG_A_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_A_RX_L0_DECODE_ERR_CNT_SHIFT 16
#define DC_PCS_8B10B_REG_A_RX_L0_DECODE_ERR_CNT_MASK  0xFFFFull
#define DC_PCS_8B10B_REG_A_RX_L0_DECODE_ERR_CNT_SMASK 0xFFFF0000ull
#define DC_PCS_8B10B_REG_A_RX_L1_DECODE_ERR_CNT_SHIFT 0
#define DC_PCS_8B10B_REG_A_RX_L1_DECODE_ERR_CNT_MASK  0xFFFFull
#define DC_PCS_8B10B_REG_A_RX_L1_DECODE_ERR_CNT_SMASK 0xFFFFull
/*
* Table #12 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_B
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_B                            (DC_PCSLPE_PCS_8B10B + 0x000000000058)
#define DC_PCS_8B10B_REG_B_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_B_RX_L2_DECODE_ERR_CNT_SHIFT 16
#define DC_PCS_8B10B_REG_B_RX_L2_DECODE_ERR_CNT_MASK  0xFFFFull
#define DC_PCS_8B10B_REG_B_RX_L2_DECODE_ERR_CNT_SMASK 0xFFFF0000ull
#define DC_PCS_8B10B_REG_B_RX_L3_DECODE_ERR_CNT_SHIFT 0
#define DC_PCS_8B10B_REG_B_RX_L3_DECODE_ERR_CNT_MASK  0xFFFFull
#define DC_PCS_8B10B_REG_B_RX_L3_DECODE_ERR_CNT_SMASK 0xFFFFull
/*
* Table #13 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_C
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_C                            (DC_PCSLPE_PCS_8B10B + 0x000000000060)
#define DC_PCS_8B10B_REG_C_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_C_UNUSED_31_14_SHIFT         14
#define DC_PCS_8B10B_REG_C_UNUSED_31_14_MASK          0x3FFFFull
#define DC_PCS_8B10B_REG_C_UNUSED_31_14_SMASK         0xFFFFC000ull
#define DC_PCS_8B10B_REG_C_MINOR_ERRORS_SHIFT         0
#define DC_PCS_8B10B_REG_C_MINOR_ERRORS_MASK          0x3FFFull
#define DC_PCS_8B10B_REG_C_MINOR_ERRORS_SMASK         0x3FFFull
/*
* Table #14 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_38
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_38                           (DC_PCSLPE_PCS_8B10B + 0x0000000001C0)
#define DC_PCS_8B10B_REG_38_RESETCSR                  0x00000000ull
#define DC_PCS_8B10B_REG_38_UNUSED_31_27_SHIFT        27
#define DC_PCS_8B10B_REG_38_UNUSED_31_27_MASK         0x1Full
#define DC_PCS_8B10B_REG_38_UNUSED_31_27_SMASK        0xF8000000ull
#define DC_PCS_8B10B_REG_38_ONE_SHOT_10B_CNT_SHIFT    24
#define DC_PCS_8B10B_REG_38_ONE_SHOT_10B_CNT_MASK     0x7ull
#define DC_PCS_8B10B_REG_38_ONE_SHOT_10B_CNT_SMASK    0x7000000ull
#define DC_PCS_8B10B_REG_38_ONE_SHOT_10B_ERR_SHIFT    16
#define DC_PCS_8B10B_REG_38_ONE_SHOT_10B_ERR_MASK     0xFFull
#define DC_PCS_8B10B_REG_38_ONE_SHOT_10B_ERR_SMASK    0xFF0000ull
#define DC_PCS_8B10B_REG_38_ONE_SHOT_PE_ERR_SHIFT     8
#define DC_PCS_8B10B_REG_38_ONE_SHOT_PE_ERR_MASK      0xFFull
#define DC_PCS_8B10B_REG_38_ONE_SHOT_PE_ERR_SMASK     0xFF00ull
#define DC_PCS_8B10B_REG_38_UNUSED_7_5_SHIFT          5
#define DC_PCS_8B10B_REG_38_UNUSED_7_5_MASK           0x7ull
#define DC_PCS_8B10B_REG_38_UNUSED_7_5_SMASK          0xE0ull
#define DC_PCS_8B10B_REG_38_ONE_SHOT_PKT_ERR_SHIFT    0
#define DC_PCS_8B10B_REG_38_ONE_SHOT_PKT_ERR_MASK     0x1Full
#define DC_PCS_8B10B_REG_38_ONE_SHOT_PKT_ERR_SMASK    0x1Full
/*
* Table #2 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_0
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_0                            (DC_PCSLPE_PCS_8B10B + 0x000000000000)
#define DC_PCS_8B10B_REG_0_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_0_UNUSED_31_0_SHIFT          0
#define DC_PCS_8B10B_REG_0_UNUSED_31_0_MASK           0xFFFFFFFFull
#define DC_PCS_8B10B_REG_0_UNUSED_31_0_SMASK          0xFFFFFFFFull
/*
* Table #3 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_1
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_1                            (DC_PCSLPE_PCS_8B10B + 0x000000000008)
#define DC_PCS_8B10B_REG_1_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_1_UNUSED_31_4_SHIFT          4
#define DC_PCS_8B10B_REG_1_UNUSED_31_4_MASK           0xFFFFFFFull
#define DC_PCS_8B10B_REG_1_UNUSED_31_4_SMASK          0xFFFFFFF0ull
#define DC_PCS_8B10B_REG_1_SMF_OVERFLOW_SHIFT         0
#define DC_PCS_8B10B_REG_1_SMF_OVERFLOW_MASK          0xFull
#define DC_PCS_8B10B_REG_1_SMF_OVERFLOW_SMASK         0xFull
/*
* Table #4 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_2
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_2                            (DC_PCSLPE_PCS_8B10B + 0x000000000010)
#define DC_PCS_8B10B_REG_2_RESETCSR                   0x00000E40ull
#define DC_PCS_8B10B_REG_2_UNUSED_31_12_SHIFT         12
#define DC_PCS_8B10B_REG_2_UNUSED_31_12_MASK          0xFFFFFull
#define DC_PCS_8B10B_REG_2_UNUSED_31_12_SMASK         0xFFFFF000ull
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_D_SHIFT      10
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_D_MASK       0x3ull
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_D_SMASK      0xC00ull
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_C_SHIFT      8
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_C_MASK       0x3ull
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_C_SMASK      0x300ull
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_B_SHIFT      6
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_B_MASK       0x3ull
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_B_SMASK      0xC0ull
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_A_SHIFT      4
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_A_MASK       0x3ull
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_A_SMASK      0x30ull
#define DC_PCS_8B10B_REG_2_TX_LANE_POLARITY_SHIFT     0
#define DC_PCS_8B10B_REG_2_TX_LANE_POLARITY_MASK      0xFull
#define DC_PCS_8B10B_REG_2_TX_LANE_POLARITY_SMASK     0xFull
/*
* Table #5 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_3
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_3                            (DC_PCSLPE_PCS_8B10B + 0x000000000018)
#define DC_PCS_8B10B_REG_3_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_3_UNUSED_31_4_SHIFT          4
#define DC_PCS_8B10B_REG_3_UNUSED_31_4_MASK           0xFFFFFFFull
#define DC_PCS_8B10B_REG_3_UNUSED_31_4_SMASK          0xFFFFFFF0ull
#define DC_PCS_8B10B_REG_3_RX_LOCK_SHIFT              0
#define DC_PCS_8B10B_REG_3_RX_LOCK_MASK               0xFull
#define DC_PCS_8B10B_REG_3_RX_LOCK_SMASK              0xFull
/*
* Table #6 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_4
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_4                            (DC_PCSLPE_PCS_8B10B + 0x000000000020)
#define DC_PCS_8B10B_REG_4_RESETCSR                   0x00000100ull
#define DC_PCS_8B10B_REG_4_UNUSED_31_9_SHIFT          9
#define DC_PCS_8B10B_REG_4_UNUSED_31_9_MASK           0x7FFFFFull
#define DC_PCS_8B10B_REG_4_UNUSED_31_9_SMASK          0xFFFFFE00ull
#define DC_PCS_8B10B_REG_4_AUTO_POLARITY_REV_EN_SHIFT 8
#define DC_PCS_8B10B_REG_4_AUTO_POLARITY_REV_EN_MASK  0x1ull
#define DC_PCS_8B10B_REG_4_AUTO_POLARITY_REV_EN_SMASK 0x100ull
#define DC_PCS_8B10B_REG_4_UNUSED_7_2_SHIFT           2
#define DC_PCS_8B10B_REG_4_UNUSED_7_2_MASK            0x3Full
#define DC_PCS_8B10B_REG_4_UNUSED_7_2_SMASK           0xFCull
#define DC_PCS_8B10B_REG_4_PCS_DATA_SYNC_DIS_SHIFT    1
#define DC_PCS_8B10B_REG_4_PCS_DATA_SYNC_DIS_MASK     0x1ull
#define DC_PCS_8B10B_REG_4_PCS_DATA_SYNC_DIS_SMASK    0x2ull
#define DC_PCS_8B10B_REG_4_UNUSED_0_SHIFT             0
#define DC_PCS_8B10B_REG_4_UNUSED_0_MASK              0x1ull
#define DC_PCS_8B10B_REG_4_UNUSED_0_SMASK             0x1ull
/*
* Table #7 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_5
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_5                            (DC_PCSLPE_PCS_8B10B + 0x000000000028)
#define DC_PCS_8B10B_REG_5_RESETCSR                   0x00000E40ull
#define DC_PCS_8B10B_REG_5_UNUSED_31_12_SHIFT         12
#define DC_PCS_8B10B_REG_5_UNUSED_31_12_MASK          0xFFFFFull
#define DC_PCS_8B10B_REG_5_UNUSED_31_12_SMASK         0xFFFFF000ull
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_D_SHIFT      10
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_D_MASK       0x3ull
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_D_SMASK      0xC00ull
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_C_SHIFT      8
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_C_MASK       0x3ull
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_C_SMASK      0x300ull
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_B_SHIFT      6
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_B_MASK       0x3ull
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_B_SMASK      0xC0ull
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_A_SHIFT      4
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_A_MASK       0x3ull
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_A_SMASK      0x30ull
#define DC_PCS_8B10B_REG_5_RX_LANE_POLARITY_SHIFT     0
#define DC_PCS_8B10B_REG_5_RX_LANE_POLARITY_MASK      0xFull
#define DC_PCS_8B10B_REG_5_RX_LANE_POLARITY_SMASK     0xFull
/*
* Table #8 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_7
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_7                            (DC_PCSLPE_PCS_8B10B + 0x000000000038)
#define DC_PCS_8B10B_REG_7_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_7_UNUSED_31_4_SHIFT          4
#define DC_PCS_8B10B_REG_7_UNUSED_31_4_MASK           0xFFFFFFFull
#define DC_PCS_8B10B_REG_7_UNUSED_31_4_SMASK          0xFFFFFFF0ull
#define DC_PCS_8B10B_REG_7_TRAIN_TX_PHS_FIFO_SHIFT    3
#define DC_PCS_8B10B_REG_7_TRAIN_TX_PHS_FIFO_MASK     0x1ull
#define DC_PCS_8B10B_REG_7_TRAIN_TX_PHS_FIFO_SMASK    0x8ull
#define DC_PCS_8B10B_REG_7_SMF_DIAG_BUSY_SHIFT        2
#define DC_PCS_8B10B_REG_7_SMF_DIAG_BUSY_MASK         0x1ull
#define DC_PCS_8B10B_REG_7_SMF_DIAG_BUSY_SMASK        0x4ull
#define DC_PCS_8B10B_REG_7_FORCE_SMF_DELETE_SHIFT     1
#define DC_PCS_8B10B_REG_7_FORCE_SMF_DELETE_MASK      0x1ull
#define DC_PCS_8B10B_REG_7_FORCE_SMF_DELETE_SMASK     0x2ull
#define DC_PCS_8B10B_REG_7_FORCE_SMF_INSERT_SHIFT     0
#define DC_PCS_8B10B_REG_7_FORCE_SMF_INSERT_MASK      0x1ull
#define DC_PCS_8B10B_REG_7_FORCE_SMF_INSERT_SMASK     0x1ull
/*
* Table #9 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_8
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_8                            (DC_PCSLPE_PCS_8B10B + 0x000000000040)
#define DC_PCS_8B10B_REG_8_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_8_UNUSED_31_9_SHIFT          9
#define DC_PCS_8B10B_REG_8_UNUSED_31_9_MASK           0x7FFFFFull
#define DC_PCS_8B10B_REG_8_UNUSED_31_9_SMASK          0xFFFFFE00ull
#define DC_PCS_8B10B_REG_8_RST_ON_READ_SHIFT          8
#define DC_PCS_8B10B_REG_8_RST_ON_READ_MASK           0x1ull
#define DC_PCS_8B10B_REG_8_RST_ON_READ_SMASK          0x100ull
#define DC_PCS_8B10B_REG_8_UNUSED_7_0_SHIFT           0
#define DC_PCS_8B10B_REG_8_UNUSED_7_0_MASK            0xFFull
#define DC_PCS_8B10B_REG_8_UNUSED_7_0_SMASK           0xFFull
/*
* Table #10 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_9
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_9                            (DC_PCSLPE_PCS_8B10B + 0x000000000048)
#define DC_PCS_8B10B_REG_9_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_9_UNUSED_31_0_SHIFT          0
#define DC_PCS_8B10B_REG_9_UNUSED_31_0_MASK           0xFFFFFFFFull
#define DC_PCS_8B10B_REG_9_UNUSED_31_0_SMASK          0xFFFFFFFFull
/*
* Table #11 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_A
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_A                            (DC_PCSLPE_PCS_8B10B + 0x000000000050)
#define DC_PCS_8B10B_REG_A_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_A_RX_L0_DECODE_ERR_CNT_SHIFT 16
#define DC_PCS_8B10B_REG_A_RX_L0_DECODE_ERR_CNT_MASK  0xFFFFull
#define DC_PCS_8B10B_REG_A_RX_L0_DECODE_ERR_CNT_SMASK 0xFFFF0000ull
#define DC_PCS_8B10B_REG_A_RX_L1_DECODE_ERR_CNT_SHIFT 0
#define DC_PCS_8B10B_REG_A_RX_L1_DECODE_ERR_CNT_MASK  0xFFFFull
#define DC_PCS_8B10B_REG_A_RX_L1_DECODE_ERR_CNT_SMASK 0xFFFFull
/*
* Table #12 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_B
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_B                            (DC_PCSLPE_PCS_8B10B + 0x000000000058)
#define DC_PCS_8B10B_REG_B_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_B_RX_L2_DECODE_ERR_CNT_SHIFT 16
#define DC_PCS_8B10B_REG_B_RX_L2_DECODE_ERR_CNT_MASK  0xFFFFull
#define DC_PCS_8B10B_REG_B_RX_L2_DECODE_ERR_CNT_SMASK 0xFFFF0000ull
#define DC_PCS_8B10B_REG_B_RX_L3_DECODE_ERR_CNT_SHIFT 0
#define DC_PCS_8B10B_REG_B_RX_L3_DECODE_ERR_CNT_MASK  0xFFFFull
#define DC_PCS_8B10B_REG_B_RX_L3_DECODE_ERR_CNT_SMASK 0xFFFFull
/*
* Table #13 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_C
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_C                            (DC_PCSLPE_PCS_8B10B + 0x000000000060)
#define DC_PCS_8B10B_REG_C_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_C_UNUSED_31_14_SHIFT         14
#define DC_PCS_8B10B_REG_C_UNUSED_31_14_MASK          0x3FFFFull
#define DC_PCS_8B10B_REG_C_UNUSED_31_14_SMASK         0xFFFFC000ull
#define DC_PCS_8B10B_REG_C_MINOR_ERRORS_SHIFT         0
#define DC_PCS_8B10B_REG_C_MINOR_ERRORS_MASK          0x3FFFull
#define DC_PCS_8B10B_REG_C_MINOR_ERRORS_SMASK         0x3FFFull
/*
* Table #14 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_38
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_38                           (DC_PCSLPE_PCS_8B10B + 0x0000000001C0)
#define DC_PCS_8B10B_REG_38_RESETCSR                  0x00000000ull
#define DC_PCS_8B10B_REG_38_UNUSED_31_27_SHIFT        27
#define DC_PCS_8B10B_REG_38_UNUSED_31_27_MASK         0x1Full
#define DC_PCS_8B10B_REG_38_UNUSED_31_27_SMASK        0xF8000000ull
#define DC_PCS_8B10B_REG_38_ONE_SHOT_10B_CNT_SHIFT    24
#define DC_PCS_8B10B_REG_38_ONE_SHOT_10B_CNT_MASK     0x7ull
#define DC_PCS_8B10B_REG_38_ONE_SHOT_10B_CNT_SMASK    0x7000000ull
#define DC_PCS_8B10B_REG_38_ONE_SHOT_10B_ERR_SHIFT    16
#define DC_PCS_8B10B_REG_38_ONE_SHOT_10B_ERR_MASK     0xFFull
#define DC_PCS_8B10B_REG_38_ONE_SHOT_10B_ERR_SMASK    0xFF0000ull
#define DC_PCS_8B10B_REG_38_ONE_SHOT_PE_ERR_SHIFT     8
#define DC_PCS_8B10B_REG_38_ONE_SHOT_PE_ERR_MASK      0xFFull
#define DC_PCS_8B10B_REG_38_ONE_SHOT_PE_ERR_SMASK     0xFF00ull
#define DC_PCS_8B10B_REG_38_UNUSED_7_5_SHIFT          5
#define DC_PCS_8B10B_REG_38_UNUSED_7_5_MASK           0x7ull
#define DC_PCS_8B10B_REG_38_UNUSED_7_5_SMASK          0xE0ull
#define DC_PCS_8B10B_REG_38_ONE_SHOT_PKT_ERR_SHIFT    0
#define DC_PCS_8B10B_REG_38_ONE_SHOT_PKT_ERR_MASK     0x1Full
#define DC_PCS_8B10B_REG_38_ONE_SHOT_PKT_ERR_SMASK    0x1Full
/*
* Table #2 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_0
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_0                            (DC_PCSLPE_PCS_8B10B + 0x000000000000)
#define DC_PCS_8B10B_REG_0_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_0_UNUSED_31_0_SHIFT          0
#define DC_PCS_8B10B_REG_0_UNUSED_31_0_MASK           0xFFFFFFFFull
#define DC_PCS_8B10B_REG_0_UNUSED_31_0_SMASK          0xFFFFFFFFull
/*
* Table #3 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_1
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_1                            (DC_PCSLPE_PCS_8B10B + 0x000000000008)
#define DC_PCS_8B10B_REG_1_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_1_UNUSED_31_4_SHIFT          4
#define DC_PCS_8B10B_REG_1_UNUSED_31_4_MASK           0xFFFFFFFull
#define DC_PCS_8B10B_REG_1_UNUSED_31_4_SMASK          0xFFFFFFF0ull
#define DC_PCS_8B10B_REG_1_SMF_OVERFLOW_SHIFT         0
#define DC_PCS_8B10B_REG_1_SMF_OVERFLOW_MASK          0xFull
#define DC_PCS_8B10B_REG_1_SMF_OVERFLOW_SMASK         0xFull
/*
* Table #4 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_2
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_2                            (DC_PCSLPE_PCS_8B10B + 0x000000000010)
#define DC_PCS_8B10B_REG_2_RESETCSR                   0x00000E40ull
#define DC_PCS_8B10B_REG_2_UNUSED_31_12_SHIFT         12
#define DC_PCS_8B10B_REG_2_UNUSED_31_12_MASK          0xFFFFFull
#define DC_PCS_8B10B_REG_2_UNUSED_31_12_SMASK         0xFFFFF000ull
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_D_SHIFT      10
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_D_MASK       0x3ull
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_D_SMASK      0xC00ull
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_C_SHIFT      8
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_C_MASK       0x3ull
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_C_SMASK      0x300ull
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_B_SHIFT      6
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_B_MASK       0x3ull
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_B_SMASK      0xC0ull
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_A_SHIFT      4
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_A_MASK       0x3ull
#define DC_PCS_8B10B_REG_2_TX_LANE_ORDER_A_SMASK      0x30ull
#define DC_PCS_8B10B_REG_2_TX_LANE_POLARITY_SHIFT     0
#define DC_PCS_8B10B_REG_2_TX_LANE_POLARITY_MASK      0xFull
#define DC_PCS_8B10B_REG_2_TX_LANE_POLARITY_SMASK     0xFull
/*
* Table #5 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_3
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_3                            (DC_PCSLPE_PCS_8B10B + 0x000000000018)
#define DC_PCS_8B10B_REG_3_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_3_UNUSED_31_4_SHIFT          4
#define DC_PCS_8B10B_REG_3_UNUSED_31_4_MASK           0xFFFFFFFull
#define DC_PCS_8B10B_REG_3_UNUSED_31_4_SMASK          0xFFFFFFF0ull
#define DC_PCS_8B10B_REG_3_RX_LOCK_SHIFT              0
#define DC_PCS_8B10B_REG_3_RX_LOCK_MASK               0xFull
#define DC_PCS_8B10B_REG_3_RX_LOCK_SMASK              0xFull
/*
* Table #6 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_4
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_4                            (DC_PCSLPE_PCS_8B10B + 0x000000000020)
#define DC_PCS_8B10B_REG_4_RESETCSR                   0x00000100ull
#define DC_PCS_8B10B_REG_4_UNUSED_31_9_SHIFT          9
#define DC_PCS_8B10B_REG_4_UNUSED_31_9_MASK           0x7FFFFFull
#define DC_PCS_8B10B_REG_4_UNUSED_31_9_SMASK          0xFFFFFE00ull
#define DC_PCS_8B10B_REG_4_AUTO_POLARITY_REV_EN_SHIFT 8
#define DC_PCS_8B10B_REG_4_AUTO_POLARITY_REV_EN_MASK  0x1ull
#define DC_PCS_8B10B_REG_4_AUTO_POLARITY_REV_EN_SMASK 0x100ull
#define DC_PCS_8B10B_REG_4_UNUSED_7_2_SHIFT           2
#define DC_PCS_8B10B_REG_4_UNUSED_7_2_MASK            0x3Full
#define DC_PCS_8B10B_REG_4_UNUSED_7_2_SMASK           0xFCull
#define DC_PCS_8B10B_REG_4_PCS_DATA_SYNC_DIS_SHIFT    1
#define DC_PCS_8B10B_REG_4_PCS_DATA_SYNC_DIS_MASK     0x1ull
#define DC_PCS_8B10B_REG_4_PCS_DATA_SYNC_DIS_SMASK    0x2ull
#define DC_PCS_8B10B_REG_4_UNUSED_0_SHIFT             0
#define DC_PCS_8B10B_REG_4_UNUSED_0_MASK              0x1ull
#define DC_PCS_8B10B_REG_4_UNUSED_0_SMASK             0x1ull
/*
* Table #7 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_5
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_5                            (DC_PCSLPE_PCS_8B10B + 0x000000000028)
#define DC_PCS_8B10B_REG_5_RESETCSR                   0x00000E40ull
#define DC_PCS_8B10B_REG_5_UNUSED_31_12_SHIFT         12
#define DC_PCS_8B10B_REG_5_UNUSED_31_12_MASK          0xFFFFFull
#define DC_PCS_8B10B_REG_5_UNUSED_31_12_SMASK         0xFFFFF000ull
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_D_SHIFT      10
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_D_MASK       0x3ull
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_D_SMASK      0xC00ull
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_C_SHIFT      8
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_C_MASK       0x3ull
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_C_SMASK      0x300ull
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_B_SHIFT      6
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_B_MASK       0x3ull
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_B_SMASK      0xC0ull
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_A_SHIFT      4
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_A_MASK       0x3ull
#define DC_PCS_8B10B_REG_5_RX_LANE_ORDER_A_SMASK      0x30ull
#define DC_PCS_8B10B_REG_5_RX_LANE_POLARITY_SHIFT     0
#define DC_PCS_8B10B_REG_5_RX_LANE_POLARITY_MASK      0xFull
#define DC_PCS_8B10B_REG_5_RX_LANE_POLARITY_SMASK     0xFull
/*
* Table #8 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_7
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_7                            (DC_PCSLPE_PCS_8B10B + 0x000000000038)
#define DC_PCS_8B10B_REG_7_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_7_UNUSED_31_4_SHIFT          4
#define DC_PCS_8B10B_REG_7_UNUSED_31_4_MASK           0xFFFFFFFull
#define DC_PCS_8B10B_REG_7_UNUSED_31_4_SMASK          0xFFFFFFF0ull
#define DC_PCS_8B10B_REG_7_TRAIN_TX_PHS_FIFO_SHIFT    3
#define DC_PCS_8B10B_REG_7_TRAIN_TX_PHS_FIFO_MASK     0x1ull
#define DC_PCS_8B10B_REG_7_TRAIN_TX_PHS_FIFO_SMASK    0x8ull
#define DC_PCS_8B10B_REG_7_SMF_DIAG_BUSY_SHIFT        2
#define DC_PCS_8B10B_REG_7_SMF_DIAG_BUSY_MASK         0x1ull
#define DC_PCS_8B10B_REG_7_SMF_DIAG_BUSY_SMASK        0x4ull
#define DC_PCS_8B10B_REG_7_FORCE_SMF_DELETE_SHIFT     1
#define DC_PCS_8B10B_REG_7_FORCE_SMF_DELETE_MASK      0x1ull
#define DC_PCS_8B10B_REG_7_FORCE_SMF_DELETE_SMASK     0x2ull
#define DC_PCS_8B10B_REG_7_FORCE_SMF_INSERT_SHIFT     0
#define DC_PCS_8B10B_REG_7_FORCE_SMF_INSERT_MASK      0x1ull
#define DC_PCS_8B10B_REG_7_FORCE_SMF_INSERT_SMASK     0x1ull
/*
* Table #9 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_8
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_8                            (DC_PCSLPE_PCS_8B10B + 0x000000000040)
#define DC_PCS_8B10B_REG_8_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_8_UNUSED_31_9_SHIFT          9
#define DC_PCS_8B10B_REG_8_UNUSED_31_9_MASK           0x7FFFFFull
#define DC_PCS_8B10B_REG_8_UNUSED_31_9_SMASK          0xFFFFFE00ull
#define DC_PCS_8B10B_REG_8_RST_ON_READ_SHIFT          8
#define DC_PCS_8B10B_REG_8_RST_ON_READ_MASK           0x1ull
#define DC_PCS_8B10B_REG_8_RST_ON_READ_SMASK          0x100ull
#define DC_PCS_8B10B_REG_8_UNUSED_7_0_SHIFT           0
#define DC_PCS_8B10B_REG_8_UNUSED_7_0_MASK            0xFFull
#define DC_PCS_8B10B_REG_8_UNUSED_7_0_SMASK           0xFFull
/*
* Table #10 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_9
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_9                            (DC_PCSLPE_PCS_8B10B + 0x000000000048)
#define DC_PCS_8B10B_REG_9_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_9_UNUSED_31_0_SHIFT          0
#define DC_PCS_8B10B_REG_9_UNUSED_31_0_MASK           0xFFFFFFFFull
#define DC_PCS_8B10B_REG_9_UNUSED_31_0_SMASK          0xFFFFFFFFull
/*
* Table #11 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_A
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_A                            (DC_PCSLPE_PCS_8B10B + 0x000000000050)
#define DC_PCS_8B10B_REG_A_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_A_RX_L0_DECODE_ERR_CNT_SHIFT 16
#define DC_PCS_8B10B_REG_A_RX_L0_DECODE_ERR_CNT_MASK  0xFFFFull
#define DC_PCS_8B10B_REG_A_RX_L0_DECODE_ERR_CNT_SMASK 0xFFFF0000ull
#define DC_PCS_8B10B_REG_A_RX_L1_DECODE_ERR_CNT_SHIFT 0
#define DC_PCS_8B10B_REG_A_RX_L1_DECODE_ERR_CNT_MASK  0xFFFFull
#define DC_PCS_8B10B_REG_A_RX_L1_DECODE_ERR_CNT_SMASK 0xFFFFull
/*
* Table #12 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_B
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_B                            (DC_PCSLPE_PCS_8B10B + 0x000000000058)
#define DC_PCS_8B10B_REG_B_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_B_RX_L2_DECODE_ERR_CNT_SHIFT 16
#define DC_PCS_8B10B_REG_B_RX_L2_DECODE_ERR_CNT_MASK  0xFFFFull
#define DC_PCS_8B10B_REG_B_RX_L2_DECODE_ERR_CNT_SMASK 0xFFFF0000ull
#define DC_PCS_8B10B_REG_B_RX_L3_DECODE_ERR_CNT_SHIFT 0
#define DC_PCS_8B10B_REG_B_RX_L3_DECODE_ERR_CNT_MASK  0xFFFFull
#define DC_PCS_8B10B_REG_B_RX_L3_DECODE_ERR_CNT_SMASK 0xFFFFull
/*
* Table #13 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_C
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_C                            (DC_PCSLPE_PCS_8B10B + 0x000000000060)
#define DC_PCS_8B10B_REG_C_RESETCSR                   0x00000000ull
#define DC_PCS_8B10B_REG_C_UNUSED_31_14_SHIFT         14
#define DC_PCS_8B10B_REG_C_UNUSED_31_14_MASK          0x3FFFFull
#define DC_PCS_8B10B_REG_C_UNUSED_31_14_SMASK         0xFFFFC000ull
#define DC_PCS_8B10B_REG_C_MINOR_ERRORS_SHIFT         0
#define DC_PCS_8B10B_REG_C_MINOR_ERRORS_MASK          0x3FFFull
#define DC_PCS_8B10B_REG_C_MINOR_ERRORS_SMASK         0x3FFFull
/*
* Table #14 of 240_CSR_pcslpe.xml - pcs_8b10b_reg_38
* See description below for individual field definition
*/
#define DC_PCS_8B10B_REG_38                           (DC_PCSLPE_PCS_8B10B + 0x0000000001C0)
#define DC_PCS_8B10B_REG_38_RESETCSR                  0x00000000ull
#define DC_PCS_8B10B_REG_38_UNUSED_31_27_SHIFT        27
#define DC_PCS_8B10B_REG_38_UNUSED_31_27_MASK         0x1Full
#define DC_PCS_8B10B_REG_38_UNUSED_31_27_SMASK        0xF8000000ull
#define DC_PCS_8B10B_REG_38_ONE_SHOT_10B_CNT_SHIFT    24
#define DC_PCS_8B10B_REG_38_ONE_SHOT_10B_CNT_MASK     0x7ull
#define DC_PCS_8B10B_REG_38_ONE_SHOT_10B_CNT_SMASK    0x7000000ull
#define DC_PCS_8B10B_REG_38_ONE_SHOT_10B_ERR_SHIFT    16
#define DC_PCS_8B10B_REG_38_ONE_SHOT_10B_ERR_MASK     0xFFull
#define DC_PCS_8B10B_REG_38_ONE_SHOT_10B_ERR_SMASK    0xFF0000ull
#define DC_PCS_8B10B_REG_38_ONE_SHOT_PE_ERR_SHIFT     8
#define DC_PCS_8B10B_REG_38_ONE_SHOT_PE_ERR_MASK      0xFFull
#define DC_PCS_8B10B_REG_38_ONE_SHOT_PE_ERR_SMASK     0xFF00ull
#define DC_PCS_8B10B_REG_38_UNUSED_7_5_SHIFT          5
#define DC_PCS_8B10B_REG_38_UNUSED_7_5_MASK           0x7ull
#define DC_PCS_8B10B_REG_38_UNUSED_7_5_SMASK          0xE0ull
#define DC_PCS_8B10B_REG_38_ONE_SHOT_PKT_ERR_SHIFT    0
#define DC_PCS_8B10B_REG_38_ONE_SHOT_PKT_ERR_MASK     0x1Full
#define DC_PCS_8B10B_REG_38_ONE_SHOT_PKT_ERR_SMASK    0x1Full
