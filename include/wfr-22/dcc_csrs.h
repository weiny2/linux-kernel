/*
* Table #3 of 250_CSR_dc_common.xml - DCC_CFG_RESET
* This CSR is used to reset individual logic blocks in Duncan Creek.
*/
#define DCC_DCC_CFG_RESET                                    (DCC_CSRS + 0x000000000000)
#define DCC_DCC_CFG_RESET_RESETCSR                           0x0000000000000010ull
#define DCC_DCC_CFG_RESET_CLK_EN_BCC_SHIFT                   4
#define DCC_DCC_CFG_RESET_CLK_EN_BCC_MASK                    0x1ull
#define DCC_DCC_CFG_RESET_CLK_EN_BCC_SMASK                   0x10ull
#define DCC_DCC_CFG_RESET_RESET_8051_SHIFT                   3
#define DCC_DCC_CFG_RESET_RESET_8051_MASK                    0x1ull
#define DCC_DCC_CFG_RESET_RESET_8051_SMASK                   0x8ull
#define DCC_DCC_CFG_RESET_RESET_RX_FPE_SHIFT                 2
#define DCC_DCC_CFG_RESET_RESET_RX_FPE_MASK                  0x1ull
#define DCC_DCC_CFG_RESET_RESET_RX_FPE_SMASK                 0x4ull
#define DCC_DCC_CFG_RESET_RESET_TX_FPE_SHIFT                 1
#define DCC_DCC_CFG_RESET_RESET_TX_FPE_MASK                  0x1ull
#define DCC_DCC_CFG_RESET_RESET_TX_FPE_SMASK                 0x2ull
#define DCC_DCC_CFG_RESET_RESET_LCB_SHIFT                    0
#define DCC_DCC_CFG_RESET_RESET_LCB_MASK                     0x1ull
#define DCC_DCC_CFG_RESET_RESET_LCB_SMASK                    0x1ull
/*
* Table #4 of 250_CSR_dc_common.xml - DCC_CFG_PORT_CONFIG
* This CSR is use to configure FPE block in Duncan Creek.
*/
#define DCC_DCC_CFG_PORT_CONFIG                              (DCC_CSRS + 0x000000000008)
#define DCC_DCC_CFG_PORT_CONFIG_RESETCSR                     0x007800050003FFFFull
#define DCC_DCC_CFG_PORT_CONFIG_LINK_STATE_SHIFT             48
#define DCC_DCC_CFG_PORT_CONFIG_LINK_STATE_MASK              0x7ull
#define DCC_DCC_CFG_PORT_CONFIG_LINK_STATE_SMASK             0x7000000000000ull
#define DCC_DCC_CFG_PORT_CONFIG_SL_SELECT_MODE_SHIFT         47
#define DCC_DCC_CFG_PORT_CONFIG_SL_SELECT_MODE_MASK          0x1ull
#define DCC_DCC_CFG_PORT_CONFIG_SL_SELECT_MODE_SMASK         0x800000000000ull
#define DCC_DCC_CFG_PORT_CONFIG_MTU_CAP_SHIFT                32
#define DCC_DCC_CFG_PORT_CONFIG_MTU_CAP_MASK                 0xFull
#define DCC_DCC_CFG_PORT_CONFIG_MTU_CAP_SMASK                0xF00000000ull
#define DCC_DCC_CFG_PORT_CONFIG_OP_VL_LANES_SHIFT            9
#define DCC_DCC_CFG_PORT_CONFIG_OP_VL_LANES_MASK             0x1FFull
#define DCC_DCC_CFG_PORT_CONFIG_OP_VL_LANES_SMASK            0x3FE00ull
#define DCC_DCC_CFG_PORT_CONFIG_SUP_VL_LANES_SHIFT           0
#define DCC_DCC_CFG_PORT_CONFIG_SUP_VL_LANES_MASK            0x1FFull
#define DCC_DCC_CFG_PORT_CONFIG_SUP_VL_LANES_SMASK           0x1FFull
/*
* Table #5 of 250_CSR_dc_common.xml - DCC_CFG_PORT_CONFIG1
* This registers is an extension of PORT_CONFIG, it contains additional port 
* information.
*/
#define DCC_DCC_CFG_PORT_CONFIG1                             (DCC_CSRS + 0x000000000010)
#define DCC_DCC_CFG_PORT_CONFIG1_RESETCSR                    0x00FFC00000000000ull
#define DCC_DCC_CFG_PORT_CONFIG1_PKT_FORMATS_ENABLED_SHIFT   52
#define DCC_DCC_CFG_PORT_CONFIG1_PKT_FORMATS_ENABLED_MASK    0xFull
#define DCC_DCC_CFG_PORT_CONFIG1_PKT_FORMATS_ENABLED_SMASK   0xF0000000000000ull
#define DCC_DCC_CFG_PORT_CONFIG1_PKT_FORMATS_SUPPORTED_SHIFT 48
#define DCC_DCC_CFG_PORT_CONFIG1_PKT_FORMATS_SUPPORTED_MASK  0xFull
#define DCC_DCC_CFG_PORT_CONFIG1_PKT_FORMATS_SUPPORTED_SMASK 0xF000000000000ull
#define DCC_DCC_CFG_PORT_CONFIG1_MULTICAST_MASK_SHIFT        32
#define DCC_DCC_CFG_PORT_CONFIG1_MULTICAST_MASK_MASK         0xFFFFull
#define DCC_DCC_CFG_PORT_CONFIG1_MULTICAST_MASK_SMASK        0xFFFF00000000ull
#define DCC_DCC_CFG_PORT_CONFIG1_LMC_SHIFT                   16
#define DCC_DCC_CFG_PORT_CONFIG1_LMC_MASK                    0xFFFFull
#define DCC_DCC_CFG_PORT_CONFIG1_LMC_SMASK                   0xFFFF0000ull
#define DCC_DCC_CFG_PORT_CONFIG1_LID_SHIFT                   0
#define DCC_DCC_CFG_PORT_CONFIG1_LID_MASK                    0xFFFFull
#define DCC_DCC_CFG_PORT_CONFIG1_LID_SMASK                   0xFFFFull
/*
* Table #6 of 250_CSR_dc_common.xml - DCC_CFG_SC_VL_TABLE_15_0
* This table is a re-mapping table for SC15-0 to VL.
*/
#define DCC_DCC_CFG_SC_VL_TABLE_15_0                         (DCC_CSRS + 0x000000000028)
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_RESETCSR                0x0000000000000000ull
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY15_SHIFT           60
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY15_MASK            0xFull
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY15_SMASK           0xF000000000000000ull
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY14_SHIFT           56
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY14_MASK            0xFull
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY14_SMASK           0xF00000000000000ull
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY13_SHIFT           52
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY13_MASK            0xFull
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY13_SMASK           0xF0000000000000ull
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY12_SHIFT           48
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY12_MASK            0xFull
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY12_SMASK           0xF000000000000ull
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY11_SHIFT           44
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY11_MASK            0xFull
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY11_SMASK           0xF00000000000ull
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY10_SHIFT           40
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY10_MASK            0xFull
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY10_SMASK           0xF0000000000ull
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY9_SHIFT            36
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY9_MASK             0xFull
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY9_SMASK            0xF000000000ull
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY8_SHIFT            32
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY8_MASK             0xFull
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY8_SMASK            0xF00000000ull
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY7_SHIFT            28
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY7_MASK             0xFull
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY7_SMASK            0xF0000000ull
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY6_SHIFT            24
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY6_MASK             0xFull
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY6_SMASK            0xF000000ull
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY5_SHIFT            20
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY5_MASK             0xFull
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY5_SMASK            0xF00000ull
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY4_SHIFT            16
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY4_MASK             0xFull
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY4_SMASK            0xF0000ull
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY3_SHIFT            12
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY3_MASK             0xFull
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY3_SMASK            0xF000ull
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY2_SHIFT            8
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY2_MASK             0xFull
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY2_SMASK            0xF00ull
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY1_SHIFT            4
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY1_MASK             0xFull
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY1_SMASK            0xF0ull
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY0_SHIFT            0
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY0_MASK             0xFull
#define DCC_DCC_CFG_SC_VL_TABLE_15_0_ENTRY0_SMASK            0xFull
/*
* Table #7 of 250_CSR_dc_common.xml - DCC_CFG_SC_VL_TABLE_31_16
* This table is a re-mapping table for SC31_16 to VL.
*/
#define DCC_DCC_CFG_SC_VL_TABLE_31_16                        (DCC_CSRS + 0x000000000030)
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_RESETCSR               0x0000000000000000ull
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY31_SHIFT          60
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY31_MASK           0xFull
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY31_SMASK          0xF000000000000000ull
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY30_SHIFT          56
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY30_MASK           0xFull
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY30_SMASK          0xF00000000000000ull
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY29_SHIFT          52
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY29_MASK           0xFull
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY29_SMASK          0xF0000000000000ull
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY28_SHIFT          48
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY28_MASK           0xFull
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY28_SMASK          0xF000000000000ull
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY27_SHIFT          44
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY27_MASK           0xFull
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY27_SMASK          0xF00000000000ull
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY26_SHIFT          40
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY26_MASK           0xFull
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY26_SMASK          0xF0000000000ull
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY25_SHIFT          36
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY25_MASK           0xFull
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY25_SMASK          0xF000000000ull
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY24_SHIFT          32
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY24_MASK           0xFull
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY24_SMASK          0xF00000000ull
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY23_SHIFT          28
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY23_MASK           0xFull
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY23_SMASK          0xF0000000ull
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY22_SHIFT          24
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY22_MASK           0xFull
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY22_SMASK          0xF000000ull
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY21_SHIFT          20
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY21_MASK           0xFull
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY21_SMASK          0xF00000ull
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY20_SHIFT          16
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY20_MASK           0xFull
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY20_SMASK          0xF0000ull
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY19_SHIFT          12
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY19_MASK           0xFull
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY19_SMASK          0xF000ull
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY18_SHIFT          8
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY18_MASK           0xFull
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY18_SMASK          0xF00ull
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY17_SHIFT          4
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY17_MASK           0xFull
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY17_SMASK          0xF0ull
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY16_SHIFT          0
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY16_MASK           0xFull
#define DCC_DCC_CFG_SC_VL_TABLE_31_16_ENTRY16_SMASK          0xFull
/*
* Table #8 of 250_CSR_dc_common.xml - DCC_CFG_EVENT_CNTR_CNTRL
* This register is used to control the event counter block.
*/
#define DCC_DCC_CFG_EVENT_CNTR_CNTRL                         (DCC_CSRS + 0x000000000038)
#define DCC_DCC_CFG_EVENT_CNTR_CNTRL_RESETCSR                0x0000000000000000ull
#define DCC_DCC_CFG_EVENT_CNTR_CNTRL_ERRINJ_TRIGGERED_SHIFT  23
#define DCC_DCC_CFG_EVENT_CNTR_CNTRL_ERRINJ_TRIGGERED_MASK   0x1ull
#define DCC_DCC_CFG_EVENT_CNTR_CNTRL_ERRINJ_TRIGGERED_SMASK  0x800000ull
#define DCC_DCC_CFG_EVENT_CNTR_CNTRL_ERRINJ_RESET_SHIFT      22
#define DCC_DCC_CFG_EVENT_CNTR_CNTRL_ERRINJ_RESET_MASK       0x1ull
#define DCC_DCC_CFG_EVENT_CNTR_CNTRL_ERRINJ_RESET_SMASK      0x400000ull
#define DCC_DCC_CFG_EVENT_CNTR_CNTRL_ERRINJ_EN_SHIFT         21
#define DCC_DCC_CFG_EVENT_CNTR_CNTRL_ERRINJ_EN_MASK          0x1ull
#define DCC_DCC_CFG_EVENT_CNTR_CNTRL_ERRINJ_EN_SMASK         0x200000ull
#define DCC_DCC_CFG_EVENT_CNTR_CNTRL_ERRINJ_MODE_SHIFT       19
#define DCC_DCC_CFG_EVENT_CNTR_CNTRL_ERRINJ_MODE_MASK        0x3ull
#define DCC_DCC_CFG_EVENT_CNTR_CNTRL_ERRINJ_MODE_SMASK       0x180000ull
#define DCC_DCC_CFG_EVENT_CNTR_CNTRL_ERRINJ_PARITY_SHIFT     15
#define DCC_DCC_CFG_EVENT_CNTR_CNTRL_ERRINJ_PARITY_MASK      0xFull
#define DCC_DCC_CFG_EVENT_CNTR_CNTRL_ERRINJ_PARITY_SMASK     0x78000ull
#define DCC_DCC_CFG_EVENT_CNTR_CNTRL_ERRINJ_ADDR_SHIFT       9
#define DCC_DCC_CFG_EVENT_CNTR_CNTRL_ERRINJ_ADDR_MASK        0x3Full
#define DCC_DCC_CFG_EVENT_CNTR_CNTRL_ERRINJ_ADDR_SMASK       0x7E00ull
#define DCC_DCC_CFG_EVENT_CNTR_CNTRL_PARITY_ERR_ADDR_SHIFT   3
#define DCC_DCC_CFG_EVENT_CNTR_CNTRL_PARITY_ERR_ADDR_MASK    0x3Full
#define DCC_DCC_CFG_EVENT_CNTR_CNTRL_PARITY_ERR_ADDR_SMASK   0x1F8ull
#define DCC_DCC_CFG_EVENT_CNTR_CNTRL_CLR_CNTRS_COMP_SHIFT    2
#define DCC_DCC_CFG_EVENT_CNTR_CNTRL_CLR_CNTRS_COMP_MASK     0x1ull
#define DCC_DCC_CFG_EVENT_CNTR_CNTRL_CLR_CNTRS_COMP_SMASK    0x4ull
#define DCC_DCC_CFG_EVENT_CNTR_CNTRL_CLR_CNTRS_SHIFT         1
#define DCC_DCC_CFG_EVENT_CNTR_CNTRL_CLR_CNTRS_MASK          0x1ull
#define DCC_DCC_CFG_EVENT_CNTR_CNTRL_CLR_CNTRS_SMASK         0x2ull
#define DCC_DCC_CFG_EVENT_CNTR_CNTRL_ENABLE_CNTRS_SHIFT      0
#define DCC_DCC_CFG_EVENT_CNTR_CNTRL_ENABLE_CNTRS_MASK       0x1ull
#define DCC_DCC_CFG_EVENT_CNTR_CNTRL_ENABLE_CNTRS_SMASK      0x1ull
/*
* Table #9 of 250_CSR_dc_common.xml - DCC_CFG_LED_CNTRL
* This register controls the port LED. The rate in which the LED blinks can be 
* controlled via hardware or SW.
*/
#define DCC_DCC_CFG_LED_CNTRL                                (DCC_CSRS + 0x000000000040)
#define DCC_DCC_CFG_LED_CNTRL_RESETCSR                       0x000000000000001Full
#define DCC_DCC_CFG_LED_CNTRL_LED_SW_BLIKN_EN_SHIFT          4
#define DCC_DCC_CFG_LED_CNTRL_LED_SW_BLIKN_EN_MASK           0x1ull
#define DCC_DCC_CFG_LED_CNTRL_LED_SW_BLIKN_EN_SMASK          0x10ull
#define DCC_DCC_CFG_LED_CNTRL_LED_SW_BLINK_RATE_SHIFT        0
#define DCC_DCC_CFG_LED_CNTRL_LED_SW_BLINK_RATE_MASK         0xFull
#define DCC_DCC_CFG_LED_CNTRL_LED_SW_BLINK_RATE_SMASK        0xFull
/*
* Table #10 of 250_CSR_dc_common.xml - DCC_ERR_FLG
* This CSR contains error flags that can generate a interrupt, to mask 
* individual flags see #%%#Section 8.6.9, 'DCC_ERR_FLG_EN'
*/
#define DCC_DCC_ERR_FLG                                      (DCC_CSRS + 0x000000000050)
#define DCC_DCC_ERR_FLG_RESETCSR                             0x0000000000000000ull
#define DCC_DCC_ERR_FLG_RCVPORT_ERR_SHIFT                    55
#define DCC_DCC_ERR_FLG_RCVPORT_ERR_MASK                     0x1ull
#define DCC_DCC_ERR_FLG_RCVPORT_ERR_SMASK                    0x80000000000000ull
#define DCC_DCC_ERR_FLG_FMCONFIG_ERR_SHIFT                   54
#define DCC_DCC_ERR_FLG_FMCONFIG_ERR_MASK                    0x1ull
#define DCC_DCC_ERR_FLG_FMCONFIG_ERR_SMASK                   0x40000000000000ull
#define DCC_DCC_ERR_FLG_FPE_TX_FIFO_OVFLW_ERR_SHIFT          37
#define DCC_DCC_ERR_FLG_FPE_TX_FIFO_OVFLW_ERR_MASK           0x1ull
#define DCC_DCC_ERR_FLG_FPE_TX_FIFO_OVFLW_ERR_SMASK          0x2000000000ull
#define DCC_DCC_ERR_FLG_LATE_EBP_ERR_SHIFT                   36
#define DCC_DCC_ERR_FLG_LATE_EBP_ERR_MASK                    0x1ull
#define DCC_DCC_ERR_FLG_LATE_EBP_ERR_SMASK                   0x1000000000ull
#define DCC_DCC_ERR_FLG_LATE_LONG_ERR_SHIFT                  35
#define DCC_DCC_ERR_FLG_LATE_LONG_ERR_MASK                   0x1ull
#define DCC_DCC_ERR_FLG_LATE_LONG_ERR_SMASK                  0x800000000ull
#define DCC_DCC_ERR_FLG_LATE_SHORT_ERR_SHIFT                 34
#define DCC_DCC_ERR_FLG_LATE_SHORT_ERR_MASK                  0x1ull
#define DCC_DCC_ERR_FLG_LATE_SHORT_ERR_SMASK                 0x400000000ull
#define DCC_DCC_ERR_FLG_RX_EARLY_DROP_ERR_SHIFT              33
#define DCC_DCC_ERR_FLG_RX_EARLY_DROP_ERR_MASK               0x1ull
#define DCC_DCC_ERR_FLG_RX_EARLY_DROP_ERR_SMASK              0x200000000ull
#define DCC_DCC_ERR_FLG_DLID_RANGE_ERR_SHIFT                 32
#define DCC_DCC_ERR_FLG_DLID_RANGE_ERR_MASK                  0x1ull
#define DCC_DCC_ERR_FLG_DLID_RANGE_ERR_SMASK                 0x100000000ull
#define DCC_DCC_ERR_FLG_LENGTH_MTU_ERR_SHIFT                 31
#define DCC_DCC_ERR_FLG_LENGTH_MTU_ERR_MASK                  0x1ull
#define DCC_DCC_ERR_FLG_LENGTH_MTU_ERR_SMASK                 0x80000000ull
#define DCC_DCC_ERR_FLG_DLID_ZERO_ERR_SHIFT                  30
#define DCC_DCC_ERR_FLG_DLID_ZERO_ERR_MASK                   0x1ull
#define DCC_DCC_ERR_FLG_DLID_ZERO_ERR_SMASK                  0x40000000ull
#define DCC_DCC_ERR_FLG_SLID_ZERO_ERR_SHIFT                  29
#define DCC_DCC_ERR_FLG_SLID_ZERO_ERR_MASK                   0x1ull
#define DCC_DCC_ERR_FLG_SLID_ZERO_ERR_SMASK                  0x20000000ull
#define DCC_DCC_ERR_FLG_PERM_NVL15_ERR_SHIFT                 28
#define DCC_DCC_ERR_FLG_PERM_NVL15_ERR_MASK                  0x1ull
#define DCC_DCC_ERR_FLG_PERM_NVL15_ERR_SMASK                 0x10000000ull
#define DCC_DCC_ERR_FLG_UNSUP_VL_ERR_SHIFT                   27
#define DCC_DCC_ERR_FLG_UNSUP_VL_ERR_MASK                    0x1ull
#define DCC_DCC_ERR_FLG_UNSUP_VL_ERR_SMASK                   0x8000000ull
#define DCC_DCC_ERR_FLG_VL15_MULTI_ERR_SHIFT                 25
#define DCC_DCC_ERR_FLG_VL15_MULTI_ERR_MASK                  0x1ull
#define DCC_DCC_ERR_FLG_VL15_MULTI_ERR_SMASK                 0x2000000ull
#define DCC_DCC_ERR_FLG_NONVL15_STATE_ERR_SHIFT              24
#define DCC_DCC_ERR_FLG_NONVL15_STATE_ERR_MASK               0x1ull
#define DCC_DCC_ERR_FLG_NONVL15_STATE_ERR_SMASK              0x1000000ull
#define DCC_DCC_ERR_FLG_BAD_HEAD_DIST_ERR_SHIFT              23
#define DCC_DCC_ERR_FLG_BAD_HEAD_DIST_ERR_MASK               0x1ull
#define DCC_DCC_ERR_FLG_BAD_HEAD_DIST_ERR_SMASK              0x800000ull
#define DCC_DCC_ERR_FLG_BAD_TAIL_DIST_ERR_SHIFT              22
#define DCC_DCC_ERR_FLG_BAD_TAIL_DIST_ERR_MASK               0x1ull
#define DCC_DCC_ERR_FLG_BAD_TAIL_DIST_ERR_SMASK              0x400000ull
#define DCC_DCC_ERR_FLG_BAD_CTRL_DIST_ERR_SHIFT              21
#define DCC_DCC_ERR_FLG_BAD_CTRL_DIST_ERR_MASK               0x1ull
#define DCC_DCC_ERR_FLG_BAD_CTRL_DIST_ERR_SMASK              0x200000ull
#define DCC_DCC_ERR_FLG_EVENT_CNTR_ROLLOVER_ERR_SHIFT        18
#define DCC_DCC_ERR_FLG_EVENT_CNTR_ROLLOVER_ERR_MASK         0x1ull
#define DCC_DCC_ERR_FLG_EVENT_CNTR_ROLLOVER_ERR_SMASK        0x40000ull
#define DCC_DCC_ERR_FLG_EVENT_CNTR_PARITY_ERR_SHIFT          17
#define DCC_DCC_ERR_FLG_EVENT_CNTR_PARITY_ERR_MASK           0x1ull
#define DCC_DCC_ERR_FLG_EVENT_CNTR_PARITY_ERR_SMASK          0x20000ull
#define DCC_DCC_ERR_FLG_BAD_CTRL_FLIT_ERR_SHIFT              16
#define DCC_DCC_ERR_FLG_BAD_CTRL_FLIT_ERR_MASK               0x1ull
#define DCC_DCC_ERR_FLG_BAD_CTRL_FLIT_ERR_SMASK              0x10000ull
#define DCC_DCC_ERR_FLG_BAD_VL_MARKER_SHIFT                  15
#define DCC_DCC_ERR_FLG_BAD_VL_MARKER_MASK                   0x1ull
#define DCC_DCC_ERR_FLG_BAD_VL_MARKER_SMASK                  0x8000ull
#define DCC_DCC_ERR_FLG_BAD_CRDT_ACK_SHIFT                   14
#define DCC_DCC_ERR_FLG_BAD_CRDT_ACK_MASK                    0x1ull
#define DCC_DCC_ERR_FLG_BAD_CRDT_ACK_SMASK                   0x4000ull
#define DCC_DCC_ERR_FLG_LINK_PKT_ERR_SHIFT                   12
#define DCC_DCC_ERR_FLG_LINK_PKT_ERR_MASK                    0x1ull
#define DCC_DCC_ERR_FLG_LINK_PKT_ERR_SMASK                   0x1000ull
#define DCC_DCC_ERR_FLG_BAD_VL_ERR_SHIFT                     11
#define DCC_DCC_ERR_FLG_BAD_VL_ERR_MASK                      0x1ull
#define DCC_DCC_ERR_FLG_BAD_VL_ERR_SMASK                     0x800ull
#define DCC_DCC_ERR_FLG_BAD_LVER_ERR_SHIFT                   10
#define DCC_DCC_ERR_FLG_BAD_LVER_ERR_MASK                    0x1ull
#define DCC_DCC_ERR_FLG_BAD_LVER_ERR_SMASK                   0x400ull
#define DCC_DCC_ERR_FLG_BAD_DLVD_ERR_SHIFT                   9
#define DCC_DCC_ERR_FLG_BAD_DLVD_ERR_MASK                    0x1ull
#define DCC_DCC_ERR_FLG_BAD_DLVD_ERR_SMASK                   0x200ull
#define DCC_DCC_ERR_FLG_BAD_VL_MARKER_ERR_SHIFT              7
#define DCC_DCC_ERR_FLG_BAD_VL_MARKER_ERR_MASK               0x1ull
#define DCC_DCC_ERR_FLG_BAD_VL_MARKER_ERR_SMASK              0x80ull
#define DCC_DCC_ERR_FLG_PREEMPTIONVL15_ERR_SHIFT             6
#define DCC_DCC_ERR_FLG_PREEMPTIONVL15_ERR_MASK              0x1ull
#define DCC_DCC_ERR_FLG_PREEMPTIONVL15_ERR_SMASK             0x40ull
#define DCC_DCC_ERR_FLG_PREEMPTION_ERR_SHIFT                 5
#define DCC_DCC_ERR_FLG_PREEMPTION_ERR_MASK                  0x1ull
#define DCC_DCC_ERR_FLG_PREEMPTION_ERR_SMASK                 0x20ull
#define DCC_DCC_ERR_FLG_BAD_FECN_ERR_SHIFT                   4
#define DCC_DCC_ERR_FLG_BAD_FECN_ERR_MASK                    0x1ull
#define DCC_DCC_ERR_FLG_BAD_FECN_ERR_SMASK                   0x10ull
#define DCC_DCC_ERR_FLG_BAD_MID_TAIL_ERR_SHIFT               3
#define DCC_DCC_ERR_FLG_BAD_MID_TAIL_ERR_MASK                0x1ull
#define DCC_DCC_ERR_FLG_BAD_MID_TAIL_ERR_SMASK               0x8ull
#define DCC_DCC_ERR_FLG_BAD_SC_ERR_SHIFT                     2
#define DCC_DCC_ERR_FLG_BAD_SC_ERR_MASK                      0x1ull
#define DCC_DCC_ERR_FLG_BAD_SC_ERR_SMASK                     0x4ull
#define DCC_DCC_ERR_FLG_BAD_L2_ERR_SHIFT                     1
#define DCC_DCC_ERR_FLG_BAD_L2_ERR_MASK                      0x1ull
#define DCC_DCC_ERR_FLG_BAD_L2_ERR_SMASK                     0x2ull
#define DCC_DCC_ERR_FLG_BAD_LT_ERR_SHIFT                     0
#define DCC_DCC_ERR_FLG_BAD_LT_ERR_MASK                      0x1ull
#define DCC_DCC_ERR_FLG_BAD_LT_ERR_SMASK                     0x1ull
/*
* Table #11 of 250_CSR_dc_common.xml - DCC_ERR_FLG_EN
* This CSR is used to enable interrupts, associated with DCC_ERR_FLG.
*/
#define DCC_DCC_ERR_FLG_EN                                   (DCC_CSRS + 0x000000000058)
#define DCC_DCC_ERR_FLG_EN_RESETCSR                          0x0000000000000000ull
#define DCC_DCC_ERR_FLG_EN_EN_SHIFT                          0
#define DCC_DCC_ERR_FLG_EN_EN_MASK                           0xFFFFFFFFFFFFFFFFull
#define DCC_DCC_ERR_FLG_EN_EN_SMASK                          0xFFFFFFFFFFFFFFFFull
/*
* Table #12 of 250_CSR_dc_common.xml - DCC_ERR_FLG_CLR
* This CSR is used to clear the error flags.
*/
#define DCC_DCC_ERR_FLG_CLR                                  (DCC_CSRS + 0x000000000060)
#define DCC_DCC_ERR_FLG_CLR_RESETCSR                         0x0000000000000000ull
#define DCC_DCC_ERR_FLG_CLR_CLR_SHIFT                        0
#define DCC_DCC_ERR_FLG_CLR_CLR_MASK                         0xFFFFFFFFFFFFFFFFull
#define DCC_DCC_ERR_FLG_CLR_CLR_SMASK                        0xFFFFFFFFFFFFFFFFull
/*
* Table #13 of 250_CSR_dc_common.xml - DCC_ERR_INFO_FLOW_CTRL
* When a flow control error is detected and it's on the even data path the flow 
* control flit is save to this CSR.
*/
#define DCC_DCC_ERR_INFO_FLOW_CTRL                           (DCC_CSRS + 0x000000000068)
#define DCC_DCC_ERR_INFO_FLOW_CTRL_RESETCSR                  0x0000000000000000ull
#define DCC_DCC_ERR_INFO_FLOW_CTRL_FC_DATA_SHIFT             0
#define DCC_DCC_ERR_INFO_FLOW_CTRL_FC_DATA_MASK              0xFFFFFFFFFFFFFFFFull
#define DCC_DCC_ERR_INFO_FLOW_CTRL_FC_DATA_SMASK             0xFFFFFFFFFFFFFFFFull
/*
* Table #14 of 250_CSR_dc_common.xml - DCC_ERR_INFO_PORTRCV
* On the detection of a PortRcv error, the error code is written into this CSR. 
* The header of the packet is also recorded in #%%#Section 8.6.13, 
* 'DCC_ERR_INFO_PORTRCV_HDR0'#%%# and #%%#Section 8.6.14, 'DCC_ERR_INFO_PORTRCV_HDR1'#%%#.
*/
#define DCC_DCC_ERR_INFO_PORTRCV                             (DCC_CSRS + 0x000000000078)
#define DCC_DCC_ERR_INFO_PORTRCV_RESETCSR                    0x0000000000000000ull
#define DCC_DCC_ERR_INFO_PORTRCV_STATE_SHIFT                 8
#define DCC_DCC_ERR_INFO_PORTRCV_STATE_MASK                  0x1ull
#define DCC_DCC_ERR_INFO_PORTRCV_STATE_SMASK                 0x100ull
#define DCC_DCC_ERR_INFO_PORTRCV_E_CODE_SHIFT                0
#define DCC_DCC_ERR_INFO_PORTRCV_E_CODE_MASK                 0xFFull
#define DCC_DCC_ERR_INFO_PORTRCV_E_CODE_SMASK                0xFFull
/*
* Table #15 of 250_CSR_dc_common.xml - DCC_ERR_INFO_PORTRCV_HDR0
* This CSR will record the first 8 bytes of the header flit when a PortRcv error 
* is detected.
*/
#define DCC_DCC_ERR_INFO_PORTRCV_HDR0                        (DCC_CSRS + 0x000000000080)
#define DCC_DCC_ERR_INFO_PORTRCV_HDR0_RESETCSR               0x0000000000000000ull
#define DCC_DCC_ERR_INFO_PORTRCV_HDR0_HEAD_FLIT_SHIFT        0
#define DCC_DCC_ERR_INFO_PORTRCV_HDR0_HEAD_FLIT_MASK         0xFFFFFFFFFFFFFFFFull
#define DCC_DCC_ERR_INFO_PORTRCV_HDR0_HEAD_FLIT_SMASK        0xFFFFFFFFFFFFFFFFull
/*
* Table #16 of 250_CSR_dc_common.xml - DCC_ERR_INFO_PORTRCV_HDR1
* This CSR will record the second 8 bytes of the header flit when a PortRcv 
* error is detected. If the packet is of type 8B this CSR will not be 
* updated.
*/
#define DCC_DCC_ERR_INFO_PORTRCV_HDR1                        (DCC_CSRS + 0x000000000088)
#define DCC_DCC_ERR_INFO_PORTRCV_HDR1_RESETCSR               0x0000000000000000ull
#define DCC_DCC_ERR_INFO_PORTRCV_HDR1_HEAD_FLIT_SHIFT        0
#define DCC_DCC_ERR_INFO_PORTRCV_HDR1_HEAD_FLIT_MASK         0xFFFFFFFFFFFFFFFFull
#define DCC_DCC_ERR_INFO_PORTRCV_HDR1_HEAD_FLIT_SMASK        0xFFFFFFFFFFFFFFFFull
/*
* Table #17 of 250_CSR_dc_common.xml - DCC_ERR_INFO_FMCONFIG
* This CSR records additional error information (error codes) when a FMCONFIG 
* error flag is raised.
*/
#define DCC_DCC_ERR_INFO_FMCONFIG                            (DCC_CSRS + 0x000000000090)
#define DCC_DCC_ERR_INFO_FMCONFIG_RESETCSR                   0x0000000000000000ull
#define DCC_DCC_ERR_INFO_FMCONFIG_STATE_SHIFT                8
#define DCC_DCC_ERR_INFO_FMCONFIG_STATE_MASK                 0x1ull
#define DCC_DCC_ERR_INFO_FMCONFIG_STATE_SMASK                0x100ull
#define DCC_DCC_ERR_INFO_FMCONFIG_E_CODE_SHIFT               0
#define DCC_DCC_ERR_INFO_FMCONFIG_E_CODE_MASK                0xFFull
#define DCC_DCC_ERR_INFO_FMCONFIG_E_CODE_SMASK               0xFFull
/*
* Table #18 of 250_CSR_dc_common.xml - DCC_ERR_INFO_UNCORRECTABLE
* This CSR records the first uncorrectable event detected. 
*/
#define DCC_DCC_ERR_INFO_UNCORRECTABLE                       (DCC_CSRS + 0x000000000098)
#define DCC_DCC_ERR_INFO_UNCORRECTABLE_RESETCSR              0x0000000000000000ull
#define DCC_DCC_ERR_INFO_UNCORRECTABLE_STATE_SHIFT           4
#define DCC_DCC_ERR_INFO_UNCORRECTABLE_STATE_MASK            0x1ull
#define DCC_DCC_ERR_INFO_UNCORRECTABLE_STATE_SMASK           0x10ull
#define DCC_DCC_ERR_INFO_UNCORRECTABLE_E_CODE_SHIFT          0
#define DCC_DCC_ERR_INFO_UNCORRECTABLE_E_CODE_MASK           0xFull
#define DCC_DCC_ERR_INFO_UNCORRECTABLE_E_CODE_SMASK          0xFull
/*
* Table #19 of 250_CSR_dc_common.xml - DCC_ERR_UNCORRECTABLE_CNT
* 
*/
#define DCC_DCC_ERR_UNCORRECTABLE_CNT                        (DCC_CSRS + 0x000000000100)
#define DCC_DCC_ERR_UNCORRECTABLE_CNT_RESETCSR               0x0000000000000000ull
#define DCC_DCC_ERR_UNCORRECTABLE_CNT_CNT_SHIFT              0
#define DCC_DCC_ERR_UNCORRECTABLE_CNT_CNT_MASK               0xFFFFFFFFFFFFFFFFull
#define DCC_DCC_ERR_UNCORRECTABLE_CNT_CNT_SMASK              0xFFFFFFFFFFFFFFFFull
/*
* Table #20 of 250_CSR_dc_common.xml - DCC_ERR_PORTRCV_ERR_CNT
* The number of packets containing one of the following errors: 1 count per 
* packet.
*/
#define DCC_DCC_ERR_PORTRCV_ERR_CNT                          (DCC_CSRS + 0x000000000108)
#define DCC_DCC_ERR_PORTRCV_ERR_CNT_RESETCSR                 0x0000000000000000ull
#define DCC_DCC_ERR_PORTRCV_ERR_CNT_CNT_SHIFT                0
#define DCC_DCC_ERR_PORTRCV_ERR_CNT_CNT_MASK                 0xFFFFFFFFFFFFFFFFull
#define DCC_DCC_ERR_PORTRCV_ERR_CNT_CNT_SMASK                0xFFFFFFFFFFFFFFFFull
/*
* Table #21 of 250_CSR_dc_common.xml - DCC_ERR_FMCONFIG_ERR_CNT
* The number of occurrences due to one of the following errors.
*/
#define DCC_DCC_ERR_FMCONFIG_ERR_CNT                         (DCC_CSRS + 0x000000000110)
#define DCC_DCC_ERR_FMCONFIG_ERR_CNT_RESETCSR                0x0000000000000000ull
#define DCC_DCC_ERR_FMCONFIG_ERR_CNT_CNT_SHIFT               0
#define DCC_DCC_ERR_FMCONFIG_ERR_CNT_CNT_MASK                0xFFFFFFFFFFFFFFFFull
#define DCC_DCC_ERR_FMCONFIG_ERR_CNT_CNT_SMASK               0xFFFFFFFFFFFFFFFFull
/*
* Table #22 of 250_CSR_dc_common.xml - DCC_ERR_RCVREMOTE_PHY_ERR_CNT
* In STL mode: Total number of received packets containing a TailBadPkt, 
* BodyBadPkt or HeadBadPkt flit. Each such packet shall be counted only once, 
* even if multiple BadPkt flits occur in the same packet.
*/
#define DCC_DCC_ERR_RCVREMOTE_PHY_ERR_CNT                    (DCC_CSRS + 0x000000000118)
#define DCC_DCC_ERR_RCVREMOTE_PHY_ERR_CNT_RESETCSR           0x0000000000000000ull
#define DCC_DCC_ERR_RCVREMOTE_PHY_ERR_CNT_CNT_SHIFT          0
#define DCC_DCC_ERR_RCVREMOTE_PHY_ERR_CNT_CNT_MASK           0xFFFFFFFFFFFFFFFFull
#define DCC_DCC_ERR_RCVREMOTE_PHY_ERR_CNT_CNT_SMASK          0xFFFFFFFFFFFFFFFFull
/*
* Table #23 of 250_CSR_dc_common.xml - DCC_ERR_DROPPED_PKT_CNT
* In STL and IB modes: The total number of packets that are either dropped or 
* have its tail marked bad. This includes packets that are discarded when ICRC 
* check fails.
*/
#define DCC_DCC_ERR_DROPPED_PKT_CNT                          (DCC_CSRS + 0x000000000120)
#define DCC_DCC_ERR_DROPPED_PKT_CNT_RESETCSR                 0x0000000000000000ull
#define DCC_DCC_ERR_DROPPED_PKT_CNT_CNT_SHIFT                0
#define DCC_DCC_ERR_DROPPED_PKT_CNT_CNT_MASK                 0xFFFFFFFFFFFFFFFFull
#define DCC_DCC_ERR_DROPPED_PKT_CNT_CNT_SMASK                0xFFFFFFFFFFFFFFFFull
/*
* Table #24 of 250_CSR_dc_common.xml - DCC_PRF_PORT_XMIT_MULTICAST_CNT
* The total number of multicast packets transmitted.
*/
#define DCC_DCC_PRF_PORT_XMIT_MULTICAST_CNT                  (DCC_CSRS + 0x000000000128)
#define DCC_DCC_PRF_PORT_XMIT_MULTICAST_CNT_RESETCSR         0x0000000000000000ull
#define DCC_DCC_PRF_PORT_XMIT_MULTICAST_CNT_CNT_SHIFT        0
#define DCC_DCC_PRF_PORT_XMIT_MULTICAST_CNT_CNT_MASK         0xFFFFFFFFFFFFFFFFull
#define DCC_DCC_PRF_PORT_XMIT_MULTICAST_CNT_CNT_SMASK        0xFFFFFFFFFFFFFFFFull
/*
* Table #25 of 250_CSR_dc_common.xml - DCC_PRF_PORT_RCV_MULTICAST_PKT_CNT
* The number of Multicast packets received.
*/
#define DCC_DCC_PRF_PORT_RCV_MULTICAST_PKT_CNT               (DCC_CSRS + 0x000000000130)
#define DCC_DCC_PRF_PORT_RCV_MULTICAST_PKT_CNT_RESETCSR      0x0000000000000000ull
#define DCC_DCC_PRF_PORT_RCV_MULTICAST_PKT_CNT_CNT_SHIFT     0
#define DCC_DCC_PRF_PORT_RCV_MULTICAST_PKT_CNT_CNT_MASK      0xFFFFFFFFFFFFFFFFull
#define DCC_DCC_PRF_PORT_RCV_MULTICAST_PKT_CNT_CNT_SMASK     0xFFFFFFFFFFFFFFFFull
/*
* Table #26 of 250_CSR_dc_common.xml - DCC_PRF_EVENT_CNTRS
* Total number of packets marked FECN for this VL by this transmitter due to 
* congestion. Does not include packets already marked with a FECN
*/
#define DCC_DCC_PRF_EVENT_CNTRS                              (DCC_CSRS + 0x000000000180)
#define DCC_DCC_PRF_EVENT_CNTRS_RESETCSR                     0x0000000000000000ull
#define DCC_DCC_PRF_EVENT_CNTRS_CNT_SHIFT                    0
#define DCC_DCC_PRF_EVENT_CNTRS_CNT_MASK                     0xFFFFFFFFFFFFFFFFull
#define DCC_DCC_PRF_EVENT_CNTRS_CNT_SMASK                    0xFFFFFFFFFFFFFFFFull
/*
* Table #27 of 250_CSR_dc_common.xml - DCC_PRF_RX_FLOW_CRTL_CNT
* Total number of flow control packets received.
*/
#define DCC_DCC_PRF_RX_FLOW_CRTL_CNT                         (DCC_CSRS + 0x000000000180)
#define DCC_DCC_PRF_RX_FLOW_CRTL_CNT_RESETCSR                0x0000000000000000ull
#define DCC_DCC_PRF_RX_FLOW_CRTL_CNT_CNT_SHIFT               0
#define DCC_DCC_PRF_RX_FLOW_CRTL_CNT_CNT_MASK                0xFFFFFFFFFFFFFFFFull
#define DCC_DCC_PRF_RX_FLOW_CRTL_CNT_CNT_SMASK               0xFFFFFFFFFFFFFFFFull
/*
* Table #28 of 250_CSR_dc_common.xml - DCC_PRF_TX_FLOW_CRTL_CNT
* Total number of flow control packets transmitted.
*/
#define DCC_DCC_PRF_TX_FLOW_CRTL_CNT                         (DCC_CSRS + 0x000000000188)
#define DCC_DCC_PRF_TX_FLOW_CRTL_CNT_RESETCSR                0x0000000000000000ull
#define DCC_DCC_PRF_TX_FLOW_CRTL_CNT_CNT_SHIFT               0
#define DCC_DCC_PRF_TX_FLOW_CRTL_CNT_CNT_MASK                0xFFFFFFFFFFFFFFFFull
#define DCC_DCC_PRF_TX_FLOW_CRTL_CNT_CNT_SMASK               0xFFFFFFFFFFFFFFFFull
/*
* Table #29 of 250_CSR_dc_common.xml - DCC_PRF_PORT_XMIT_DATA_CNT
* The total number of fabric packet flits transmitted. Does not include idle nor 
* other LF command flits..
*/
#define DCC_DCC_PRF_PORT_XMIT_DATA_CNT                       (DCC_CSRS + 0x000000000190)
#define DCC_DCC_PRF_PORT_XMIT_DATA_CNT_RESETCSR              0x0000000000000000ull
#define DCC_DCC_PRF_PORT_XMIT_DATA_CNT_CNT_SHIFT             0
#define DCC_DCC_PRF_PORT_XMIT_DATA_CNT_CNT_MASK              0xFFFFFFFFFFFFFFFFull
#define DCC_DCC_PRF_PORT_XMIT_DATA_CNT_CNT_SMASK             0xFFFFFFFFFFFFFFFFull
/*
* Table #30 of 250_CSR_dc_common.xml - DCC_PRF_PORT_RCV_DATA_CNT
* The total number of fabric packet flits received.
*/
#define DCC_DCC_PRF_PORT_RCV_DATA_CNT                        (DCC_CSRS + 0x000000000198)
#define DCC_DCC_PRF_PORT_RCV_DATA_CNT_RESETCSR               0x0000000000000000ull
#define DCC_DCC_PRF_PORT_RCV_DATA_CNT_CNT_SHIFT              0
#define DCC_DCC_PRF_PORT_RCV_DATA_CNT_CNT_MASK               0xFFFFFFFFFFFFFFFFull
#define DCC_DCC_PRF_PORT_RCV_DATA_CNT_CNT_SMASK              0xFFFFFFFFFFFFFFFFull
/*
* Table #31 of 250_CSR_dc_common.xml - DCC_PRF_PORT_XMIT_PKTS_CNT
* The total number of fabric packets transmitted.
*/
#define DCC_DCC_PRF_PORT_XMIT_PKTS_CNT                       (DCC_CSRS + 0x0000000001A0)
#define DCC_DCC_PRF_PORT_XMIT_PKTS_CNT_RESETCSR              0x0000000000000000ull
#define DCC_DCC_PRF_PORT_XMIT_PKTS_CNT_CNT_SHIFT             0
#define DCC_DCC_PRF_PORT_XMIT_PKTS_CNT_CNT_MASK              0xFFFFFFFFFFFFFFFFull
#define DCC_DCC_PRF_PORT_XMIT_PKTS_CNT_CNT_SMASK             0xFFFFFFFFFFFFFFFFull
/*
* Table #32 of 250_CSR_dc_common.xml - DCC_PRF_PORT_RCV_PKTS_CNT
* The total number of fabric packets received.
*/
#define DCC_DCC_PRF_PORT_RCV_PKTS_CNT                        (DCC_CSRS + 0x0000000001A8)
#define DCC_DCC_PRF_PORT_RCV_PKTS_CNT_RESETCSR               0x0000000000000000ull
#define DCC_DCC_PRF_PORT_RCV_PKTS_CNT_CNT_SHIFT              0
#define DCC_DCC_PRF_PORT_RCV_PKTS_CNT_CNT_MASK               0xFFFFFFFFFFFFFFFFull
#define DCC_DCC_PRF_PORT_RCV_PKTS_CNT_CNT_SMASK              0xFFFFFFFFFFFFFFFFull
/*
* Table #33 of 250_CSR_dc_common.xml - DCC_PRF_PORT_VL_RCV_DATA_CNT
* The total number of fabric packet flits received per VL. Does not include idle 
* nor other LF command flits.
*/
#define DCC_DCC_PRF_PORT_VL_RCV_DATA_CNT                     (DCC_CSRS + 0x0000000001B0)
#define DCC_DCC_PRF_PORT_VL_RCV_DATA_CNT_RESETCSR            0x0000000000000000ull
#define DCC_DCC_PRF_PORT_VL_RCV_DATA_CNT_CNT_SHIFT           0
#define DCC_DCC_PRF_PORT_VL_RCV_DATA_CNT_CNT_MASK            0xFFFFFFFFFFFFFFFFull
#define DCC_DCC_PRF_PORT_VL_RCV_DATA_CNT_CNT_SMASK           0xFFFFFFFFFFFFFFFFull
/*
* Table #34 of 250_CSR_dc_common.xml - DCC_PRF_PORT_VL_RCV_PKTS_CNT
* The total number of packets received per VL
*/
#define DCC_DCC_PRF_PORT_VL_RCV_PKTS_CNT                     (DCC_CSRS + 0x0000000001F8)
#define DCC_DCC_PRF_PORT_VL_RCV_PKTS_CNT_RESETCSR            0x0000000000000000ull
#define DCC_DCC_PRF_PORT_VL_RCV_PKTS_CNT_CNT_SHIFT           0
#define DCC_DCC_PRF_PORT_VL_RCV_PKTS_CNT_CNT_MASK            0xFFFFFFFFFFFFFFFFull
#define DCC_DCC_PRF_PORT_VL_RCV_PKTS_CNT_CNT_SMASK           0xFFFFFFFFFFFFFFFFull
/*
* Table #35 of 250_CSR_dc_common.xml - DCC_PRF_PORT_RCV_FECN_CNT
* Total number of packets received with FECN set.
*/
#define DCC_DCC_PRF_PORT_RCV_FECN_CNT                        (DCC_CSRS + 0x000000000240)
#define DCC_DCC_PRF_PORT_RCV_FECN_CNT_RESETCSR               0x0000000000000000ull
#define DCC_DCC_PRF_PORT_RCV_FECN_CNT_CNT_SHIFT              0
#define DCC_DCC_PRF_PORT_RCV_FECN_CNT_CNT_MASK               0xFFFFFFFFFFFFFFFFull
#define DCC_DCC_PRF_PORT_RCV_FECN_CNT_CNT_SMASK              0xFFFFFFFFFFFFFFFFull
/*
* Table #36 of 250_CSR_dc_common.xml - DCC_PRF_PORT_VL_RCV_FECN_CNT
* Total number of packets received with the FECN bit set per VL.
*/
#define DCC_DCC_PRF_PORT_VL_RCV_FECN_CNT                     (DCC_CSRS + 0x000000000248)
#define DCC_DCC_PRF_PORT_VL_RCV_FECN_CNT_RESETCSR            0x0000000000000000ull
#define DCC_DCC_PRF_PORT_VL_RCV_FECN_CNT_CNT_SHIFT           0
#define DCC_DCC_PRF_PORT_VL_RCV_FECN_CNT_CNT_MASK            0xFFFFFFFFFFFFFFFFull
#define DCC_DCC_PRF_PORT_VL_RCV_FECN_CNT_CNT_SMASK           0xFFFFFFFFFFFFFFFFull
/*
* Table #37 of 250_CSR_dc_common.xml - DCC_PRF_PORT_RCV_BECN_CNT
* Total number of packets received with BECN set.
*/
#define DCC_DCC_PRF_PORT_RCV_BECN_CNT                        (DCC_CSRS + 0x000000000290)
#define DCC_DCC_PRF_PORT_RCV_BECN_CNT_RESETCSR               0x0000000000000000ull
#define DCC_DCC_PRF_PORT_RCV_BECN_CNT_CNT_SHIFT              0
#define DCC_DCC_PRF_PORT_RCV_BECN_CNT_CNT_MASK               0xFFFFFFFFFFFFFFFFull
#define DCC_DCC_PRF_PORT_RCV_BECN_CNT_CNT_SMASK              0xFFFFFFFFFFFFFFFFull
/*
* Table #38 of 250_CSR_dc_common.xml - DCC_PRF_PORT_VL_RCV_BECN_CNT
* Total number of packets received with the BECN bit set per VL.
*/
#define DCC_DCC_PRF_PORT_VL_RCV_BECN_CNT                     (DCC_CSRS + 0x000000000298)
#define DCC_DCC_PRF_PORT_VL_RCV_BECN_CNT_RESETCSR            0x0000000000000000ull
#define DCC_DCC_PRF_PORT_VL_RCV_BECN_CNT_CNT_SHIFT           0
#define DCC_DCC_PRF_PORT_VL_RCV_BECN_CNT_CNT_MASK            0xFFFFFFFFFFFFFFFFull
#define DCC_DCC_PRF_PORT_VL_RCV_BECN_CNT_CNT_SMASK           0xFFFFFFFFFFFFFFFFull
/*
* Table #39 of 250_CSR_dc_common.xml - DCC_PRF_PORT_RCV_BUBBLE_CNT
* The total number of flits times where one or more packets have been started to 
* be received, but receiver received idles.
*/
#define DCC_DCC_PRF_PORT_RCV_BUBBLE_CNT                      (DCC_CSRS + 0x0000000002E0)
#define DCC_DCC_PRF_PORT_RCV_BUBBLE_CNT_RESETCSR             0x0000000000000000ull
#define DCC_DCC_PRF_PORT_RCV_BUBBLE_CNT_CNT_SHIFT            0
#define DCC_DCC_PRF_PORT_RCV_BUBBLE_CNT_CNT_MASK             0xFFFFFFFFFFFFFFFFull
#define DCC_DCC_PRF_PORT_RCV_BUBBLE_CNT_CNT_SMASK            0xFFFFFFFFFFFFFFFFull
/*
* Table #40 of 250_CSR_dc_common.xml - DCC_PRF_PORT_VL_RCV_BUBBLE_CNT
* The total number of flits times stalled where a packet has been started to be 
* received on the given VL and is the current VL in progress, but receiver 
* received idles.
*/
#define DCC_DCC_PRF_PORT_VL_RCV_BUBBLE_CNT                   (DCC_CSRS + 0x0000000002E8)
#define DCC_DCC_PRF_PORT_VL_RCV_BUBBLE_CNT_RESETCSR          0x0000000000000000ull
#define DCC_DCC_PRF_PORT_VL_RCV_BUBBLE_CNT_CNT_SHIFT         0
#define DCC_DCC_PRF_PORT_VL_RCV_BUBBLE_CNT_CNT_MASK          0xFFFFFFFFFFFFFFFFull
#define DCC_DCC_PRF_PORT_VL_RCV_BUBBLE_CNT_CNT_SMASK         0xFFFFFFFFFFFFFFFFull
/*
* Table #41 of 250_CSR_dc_common.xml - DCC_PRF_PORT_MARK_FECN_CNT
* Total number of packets marked FECN by this transmitter due to congestion. 
* Does not include packets already marked with a FECN
*/
#define DCC_DCC_PRF_PORT_MARK_FECN_CNT                       (DCC_CSRS + 0x000000000330)
#define DCC_DCC_PRF_PORT_MARK_FECN_CNT_RESETCSR              0x0000000000000000ull
#define DCC_DCC_PRF_PORT_MARK_FECN_CNT_CNT_SHIFT             0
#define DCC_DCC_PRF_PORT_MARK_FECN_CNT_CNT_MASK              0xFFFFFFFFFFFFFFFFull
#define DCC_DCC_PRF_PORT_MARK_FECN_CNT_CNT_SMASK             0xFFFFFFFFFFFFFFFFull
/*
* Table #42 of 250_CSR_dc_common.xml - DCC_PRF_PORT_VL_MARK_FECN_CNT
* Total number of packets marked FECN for this VL by this transmitter due to 
* congestion. Does not include packets already marked with a FECN
*/
#define DCC_DCC_PRF_PORT_VL_MARK_FECN_CNT                    (DCC_CSRS + 0x000000000338)
#define DCC_DCC_PRF_PORT_VL_MARK_FECN_CNT_RESETCSR           0x0000000000000000ull
#define DCC_DCC_PRF_PORT_VL_MARK_FECN_CNT_CNT_SHIFT          0
#define DCC_DCC_PRF_PORT_VL_MARK_FECN_CNT_CNT_MASK           0xFFFFFFFFFFFFFFFFull
#define DCC_DCC_PRF_PORT_VL_MARK_FECN_CNT_CNT_SMASK          0xFFFFFFFFFFFFFFFFull
