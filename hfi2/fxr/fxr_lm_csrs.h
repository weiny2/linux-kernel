// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Wed Jun 22 19:30:15 2016
//

#ifndef ___FXR_lm_CSRS_H__
#define ___FXR_lm_CSRS_H__

// LM_CONFIG desc:
typedef union {
    struct {
        uint64_t NEAR_LOOPBACK_DISABLE  :  1; // Disables the DLID checking for near loopback
        uint64_t      SYMMETRIC_PORTS  :  1; // Sets the HFI into symmetric port mode, where the two ports are treated as if they are on symmetric planes with the same connectivity. This bit enables adaptive routing across the two ports.
        uint64_t     ENABLE_FAIL_OVER  :  1; // Enables all outbound traffic to a failed port to be routed to the other port. Defaults to off.
        uint64_t       FORCE_LOOPBACK  :  1; // When set, forces all traffic to loopback (bypassing DLID and PKEY checks)
        uint64_t        Reserved_63_4  : 60; // Reserved
    } field;
    uint64_t val;
} LM_CONFIG_t;

// LM_CONFIG_PORT0 desc:
typedef union {
    struct {
        uint64_t           Reserved_0  :  1; // Reserved
        uint64_t           LMC_ENABLE  :  1; // Indicates that this port is assigned multiple consecutive DLIDs. Effectively masks low order bits for DLID check.
        uint64_t           Reserved_2  :  1; // Reserved
        uint64_t           Reserved_3  :  1; // Reserved
        uint64_t            LMC_WIDTH  :  4; // Width of the LMC mask (in bits). 0=>1 LID, 1=>2 LIDs, etc.
        uint64_t                 DLID  : 24; // DLID for this port
        uint64_t       Reserved_43_32  : 12; // Reserved
        uint64_t       Reserved_47_44  :  4; // Reserved
        uint64_t       Reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} LM_CONFIG_PORT0_t;

// LM_FP_TIMER_PORT0 desc:
typedef union {
    struct {
        uint64_t              TIMEOUT  : 48; // Time-out counter threshold to trigger a forward progress drop. Time is inactive when set to 0.
        uint64_t       Reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} LM_FP_TIMER_PORT0_t;

// LM_CFG_PORT0_SL2SC0 desc:
typedef union {
    struct {
        uint64_t            SL0_TO_SC  :  5; // The mapping from SL0 to the SC to be used.
        uint64_t         Reserved_7_5  :  3; // Reserved
        uint64_t            SL1_TO_SC  :  5; // The mapping from SL1 to the SC to be used.
        uint64_t       Reserved_15_13  :  3; // Reserved
        uint64_t            SL2_TO_SC  :  5; // The mapping from SL2 to the SC to be used.
        uint64_t       Reserved_23_21  :  3; // Reserved
        uint64_t            SL3_TO_SC  :  5; // The mapping from SL3 to the SC to be used.
        uint64_t       Reserved_31_29  :  3; // Reserved
        uint64_t            SL4_TO_SC  :  5; // The mapping from SL4 to the SC to be used.
        uint64_t       Reserved_39_37  :  3; // Reserved
        uint64_t            SL5_TO_SC  :  5; // The mapping from SL5 to the SC to be used.
        uint64_t       Reserved_47_45  :  3; // Reserved
        uint64_t            SL6_TO_SC  :  5; // The mapping from SL6 to the SC to be used.
        uint64_t       Reserved_55_53  :  3; // Reserved
        uint64_t            SL7_TO_SC  :  5; // The mapping from SL7 to the SC to be used.
        uint64_t       Reserved_63_61  :  3; // Reserved
    } field;
    uint64_t val;
} LM_CFG_PORT0_SL2SC0_t;

// LM_CFG_PORT0_SL2SC1 desc:
typedef union {
    struct {
        uint64_t            SL8_TO_SC  :  5; // The mapping from SL8 to the SC to be used.
        uint64_t         Reserved_7_5  :  3; // Reserved
        uint64_t            SL9_TO_SC  :  5; // The mapping from SL9 to the SC to be used.
        uint64_t       Reserved_15_13  :  3; // Reserved
        uint64_t           SL10_TO_SC  :  5; // The mapping from SL10 to the SC to be used.
        uint64_t       Reserved_23_21  :  3; // Reserved
        uint64_t           SL11_TO_SC  :  5; // The mapping from SL11 to the SC to be used.
        uint64_t       Reserved_31_29  :  3; // Reserved
        uint64_t           SL12_TO_SC  :  5; // The mapping from SL12 to the SC to be used.
        uint64_t       Reserved_39_37  :  3; // Reserved
        uint64_t           SL13_TO_SC  :  5; // The mapping from SL13 to the SC to be used.
        uint64_t       Reserved_47_45  :  3; // Reserved
        uint64_t           SL14_TO_SC  :  5; // The mapping from SL14 to the SC to be used.
        uint64_t       Reserved_55_53  :  3; // Reserved
        uint64_t           SL15_TO_SC  :  5; // The mapping from SL15 to the SC to be used.
        uint64_t       Reserved_63_61  :  3; // Reserved
    } field;
    uint64_t val;
} LM_CFG_PORT0_SL2SC1_t;

// LM_CFG_PORT0_SL2SC2 desc:
typedef union {
    struct {
        uint64_t           SL16_TO_SC  :  5; // The mapping from SL16 to the SC to be used.
        uint64_t         Reserved_7_5  :  3; // Reserved
        uint64_t           SL17_TO_SC  :  5; // The mapping from SL17 to the SC to be used.
        uint64_t       Reserved_15_13  :  3; // Reserved
        uint64_t           SL18_TO_SC  :  5; // The mapping from SL18 to the SC to be used.
        uint64_t       Reserved_23_21  :  3; // Reserved
        uint64_t           SL19_TO_SC  :  5; // The mapping from SL19 to the SC to be used.
        uint64_t       Reserved_31_29  :  3; // Reserved
        uint64_t           SL20_TO_SC  :  5; // The mapping from SL20 to the SC to be used.
        uint64_t       Reserved_39_37  :  3; // Reserved
        uint64_t           SL21_TO_SC  :  5; // The mapping from SL21 to the SC to be used.
        uint64_t       Reserved_47_45  :  3; // Reserved
        uint64_t           SL22_TO_SC  :  5; // The mapping from SL22 to the SC to be used.
        uint64_t       Reserved_55_53  :  3; // Reserved
        uint64_t           SL23_TO_SC  :  5; // The mapping from SL23 to the SC to be used.
        uint64_t       Reserved_63_61  :  3; // Reserved
    } field;
    uint64_t val;
} LM_CFG_PORT0_SL2SC2_t;

// LM_CFG_PORT0_SL2SC3 desc:
typedef union {
    struct {
        uint64_t           SL24_TO_SC  :  5; // The mapping from SL24 to the SC to be used.
        uint64_t         Reserved_7_5  :  3; // Reserved
        uint64_t           SL25_TO_SC  :  5; // The mapping from SL25 to the SC to be used.
        uint64_t       Reserved_15_13  :  3; // Reserved
        uint64_t           SL26_TO_SC  :  5; // The mapping from SL26 to the SC to be used.
        uint64_t       Reserved_23_21  :  3; // Reserved
        uint64_t           SL27_TO_SC  :  5; // The mapping from SL27 to the SC to be used.
        uint64_t       Reserved_31_29  :  3; // Reserved
        uint64_t           SL28_TO_SC  :  5; // The mapping from SL28 to the SC to be used.
        uint64_t       Reserved_39_37  :  3; // Reserved
        uint64_t           SL29_TO_SC  :  5; // The mapping from SL29 to the SC to be used.
        uint64_t       Reserved_47_45  :  3; // Reserved
        uint64_t           SL30_TO_SC  :  5; // The mapping from SL30 to the SC to be used.
        uint64_t       Reserved_55_53  :  3; // Reserved
        uint64_t           SL31_TO_SC  :  5; // The mapping from SL31 to the SC to be used.
        uint64_t       Reserved_63_61  :  3; // Reserved
    } field;
    uint64_t val;
} LM_CFG_PORT0_SL2SC3_t;

// LM_CFG_PORT0_SC2SL0 desc:
typedef union {
    struct {
        uint64_t            SC0_TO_SL  :  5; // The mapping from SC0 to the SL to be used.
        uint64_t         Reserved_7_5  :  3; // Reserved
        uint64_t            SC1_TO_SL  :  5; // The mapping from SC1 to the SL to be used.
        uint64_t       Reserved_15_13  :  3; // Reserved
        uint64_t            SC2_TO_SL  :  5; // The mapping from SC2 to the SL to be used.
        uint64_t       Reserved_23_21  :  3; // Reserved
        uint64_t            SC3_TO_SL  :  5; // The mapping from SC3 to the SL to be used.
        uint64_t       Reserved_31_29  :  3; // Reserved
        uint64_t            SC4_TO_SL  :  5; // The mapping from SC4 to the SL to be used.
        uint64_t       Reserved_39_37  :  3; // Reserved
        uint64_t            SC5_TO_SL  :  5; // The mapping from SC5 to the SL to be used.
        uint64_t       Reserved_47_45  :  3; // Reserved
        uint64_t            SC6_TO_SL  :  5; // The mapping from SC6 to the SL to be used.
        uint64_t       Reserved_55_53  :  3; // Reserved
        uint64_t            SC7_TO_SL  :  5; // The mapping from SC7 to the SL to be used.
        uint64_t       Reserved_63_61  :  3; // Reserved
    } field;
    uint64_t val;
} LM_CFG_PORT0_SC2SL0_t;

// LM_CFG_PORT0_SC2SL1 desc:
typedef union {
    struct {
        uint64_t            SC8_TO_SL  :  5; // The mapping from SC8 to the SL to be used.
        uint64_t         Reserved_7_5  :  3; // Reserved
        uint64_t            SC9_TO_SL  :  5; // The mapping from SC9 to the SL to be used.
        uint64_t       Reserved_15_13  :  3; // Reserved
        uint64_t           SC10_TO_SL  :  5; // The mapping from SC10 to the SL to be used.
        uint64_t       Reserved_23_21  :  3; // Reserved
        uint64_t           SC11_TO_SL  :  5; // The mapping from SC11 to the SL to be used.
        uint64_t       Reserved_31_29  :  3; // Reserved
        uint64_t           SC12_TO_SL  :  5; // The mapping from SC12 to the SL to be used.
        uint64_t       Reserved_39_37  :  3; // Reserved
        uint64_t           SC13_TO_SL  :  5; // The mapping from SC13 to the SL to be used.
        uint64_t       Reserved_47_45  :  3; // Reserved
        uint64_t           SC14_TO_SL  :  5; // The mapping from SC14 to the SL to be used.
        uint64_t       Reserved_55_53  :  3; // Reserved
        uint64_t           SC15_TO_SL  :  5; // The mapping from SC15 to the SL to be used.
        uint64_t       Reserved_63_61  :  3; // Reserved
    } field;
    uint64_t val;
} LM_CFG_PORT0_SC2SL1_t;

// LM_CFG_PORT0_SC2SL2 desc:
typedef union {
    struct {
        uint64_t           SC16_TO_SL  :  5; // The mapping from SC16 to the SL to be used.
        uint64_t         Reserved_7_5  :  3; // Reserved
        uint64_t           SC17_TO_SL  :  5; // The mapping from SC17 to the SL to be used.
        uint64_t       Reserved_15_13  :  3; // Reserved
        uint64_t           SC18_TO_SL  :  5; // The mapping from SC18 to the SL to be used.
        uint64_t       Reserved_23_21  :  3; // Reserved
        uint64_t           SC19_TO_SL  :  5; // The mapping from SC19 to the SL to be used.
        uint64_t       Reserved_31_29  :  3; // Reserved
        uint64_t           SC20_TO_SL  :  5; // The mapping from SC20 to the SL to be used.
        uint64_t       Reserved_39_37  :  3; // Reserved
        uint64_t           SC21_TO_SL  :  5; // The mapping from SC21 to the SL to be used.
        uint64_t       Reserved_47_45  :  3; // Reserved
        uint64_t           SC22_TO_SL  :  5; // The mapping from SC22 to the SL to be used.
        uint64_t       Reserved_55_53  :  3; // Reserved
        uint64_t           SC23_TO_SL  :  5; // The mapping from SC23 to the SL to be used.
        uint64_t       Reserved_63_61  :  3; // Reserved
    } field;
    uint64_t val;
} LM_CFG_PORT0_SC2SL2_t;

// LM_CFG_PORT0_SC2SL3 desc:
typedef union {
    struct {
        uint64_t           SC24_TO_SL  :  5; // The mapping from SC24 to the SL to be used.
        uint64_t         Reserved_7_5  :  3; // Reserved
        uint64_t           SC25_TO_SL  :  5; // The mapping from SC25 to the SL to be used.
        uint64_t       Reserved_15_13  :  3; // Reserved
        uint64_t           SC26_TO_SL  :  5; // The mapping from SC26 to the SL to be used.
        uint64_t       Reserved_23_21  :  3; // Reserved
        uint64_t           SC27_TO_SL  :  5; // The mapping from SC27 to the SL to be used.
        uint64_t       Reserved_31_29  :  3; // Reserved
        uint64_t           SC28_TO_SL  :  5; // The mapping from SC28 to the SL to be used.
        uint64_t       Reserved_39_37  :  3; // Reserved
        uint64_t           SC29_TO_SL  :  5; // The mapping from SC29 to the SL to be used.
        uint64_t       Reserved_47_45  :  3; // Reserved
        uint64_t           SC30_TO_SL  :  5; // The mapping from SC30 to the SL to be used.
        uint64_t       Reserved_55_53  :  3; // Reserved
        uint64_t           SC31_TO_SL  :  5; // The mapping from SC31 to the SL to be used.
        uint64_t       Reserved_63_61  :  3; // Reserved
    } field;
    uint64_t val;
} LM_CFG_PORT0_SC2SL3_t;

// LM_CFG_PORT0_SC2MCTC0 desc:
typedef union {
    struct {
        uint64_t          SC0_TO_MCTC  :  3; // The mapping from SC0 to the MCTC to be used.
        uint64_t           Reserved_3  :  1; // Reserved
        uint64_t          SC1_TO_MCTC  :  3; // The mapping from SC1 to the MCTC to be used.
        uint64_t           Reserved_7  :  1; // Reserved
        uint64_t          SC2_TO_MCTC  :  3; // The mapping from SC2 to the MCTC to be used.
        uint64_t          Reserved_11  :  1; // Reserved
        uint64_t          SC3_TO_MCTC  :  3; // The mapping from SC3 to the MCTC to be used.
        uint64_t          Reserved_15  :  1; // Reserved
        uint64_t          SC4_TO_MCTC  :  3; // The mapping from SC4 to the MCTC to be used.
        uint64_t          Reserved_19  :  1; // Reserved
        uint64_t          SC5_TO_MCTC  :  3; // The mapping from SC5 to the MCTC to be used.
        uint64_t          Reserved_23  :  1; // Reserved
        uint64_t          SC6_TO_MCTC  :  3; // The mapping from SC6 to the MCTC to be used.
        uint64_t          Reserved_27  :  1; // Reserved
        uint64_t          SC7_TO_MCTC  :  3; // The mapping from SC7 to the MCTC to be used.
        uint64_t          Reserved_31  :  1; // Reserved
        uint64_t          SC8_TO_MCTC  :  3; // The mapping from SC8 to the MCTC to be used.
        uint64_t          Reserved_35  :  1; // Reserved
        uint64_t          SC9_TO_MCTC  :  3; // The mapping from SC9 to the MCTC to be used.
        uint64_t          Reserved_39  :  1; // Reserved
        uint64_t         SC10_TO_MCTC  :  3; // The mapping from SC10 to the MCTC to be used.
        uint64_t          Reserved_43  :  1; // Reserved
        uint64_t         SC11_TO_MCTC  :  3; // The mapping from SC11 to the MCTC to be used.
        uint64_t          Reserved_47  :  1; // Reserved
        uint64_t         SC12_TO_MCTC  :  3; // The mapping from SC12 to the MCTC to be used.
        uint64_t          Reserved_51  :  1; // Reserved
        uint64_t         SC13_TO_MCTC  :  3; // The mapping from SC13 to the MCTC to be used.
        uint64_t          Reserved_55  :  1; // Reserved
        uint64_t         SC14_TO_MCTC  :  3; // The mapping from SC014 to the MCTC to be used.
        uint64_t          Reserved_59  :  1; // Reserved
        uint64_t         SC15_TO_MCTC  :  3; // The mapping from SC15 to the MCTC to be used.
        uint64_t          Reserved_63  :  1; // Reserved
    } field;
    uint64_t val;
} LM_CFG_PORT0_SC2MCTC0_t;

// LM_CFG_PORT0_SC2MCTC1 desc:
typedef union {
    struct {
        uint64_t         SC16_TO_MCTC  :  3; // The mapping from SC0 to the MCTC to be used.
        uint64_t           Reserved_3  :  1; // Reserved
        uint64_t         SC17_TO_MCTC  :  3; // The mapping from SC17 to the MCTC to be used.
        uint64_t           Reserved_7  :  1; // Reserved
        uint64_t         SC18_TO_MCTC  :  3; // The mapping from SC18 to the MCTC to be used.
        uint64_t          Reserved_11  :  1; // Reserved
        uint64_t         SC19_TO_MCTC  :  3; // The mapping from SC19 to the MCTC to be used.
        uint64_t          Reserved_15  :  1; // Reserved
        uint64_t         SC20_TO_MCTC  :  3; // The mapping from SC20 to the MCTC to be used.
        uint64_t          Reserved_19  :  1; // Reserved
        uint64_t         SC21_TO_MCTC  :  3; // The mapping from SC21 to the MCTC to be used.
        uint64_t          Reserved_23  :  1; // Reserved
        uint64_t         SC22_TO_MCTC  :  3; // The mapping from SC22 to the MCTC to be used.
        uint64_t          Reserved_27  :  1; // Reserved
        uint64_t         SC23_TO_MCTC  :  3; // The mapping from SC23 to the MCTC to be used.
        uint64_t          Reserved_31  :  1; // Reserved
        uint64_t         SC24_TO_MCTC  :  3; // The mapping from SC24 to the MCTC to be used.
        uint64_t          Reserved_35  :  1; // Reserved
        uint64_t         SC25_TO_MCTC  :  3; // The mapping from SC25 to the MCTC to be used.
        uint64_t          Reserved_39  :  1; // Reserved
        uint64_t         SC26_TO_MCTC  :  3; // The mapping from SC26 to the MCTC to be used.
        uint64_t          Reserved_43  :  1; // Reserved
        uint64_t         SC27_TO_MCTC  :  3; // The mapping from SC27 to the MCTC to be used.
        uint64_t          Reserved_47  :  1; // Reserved
        uint64_t         SC28_TO_MCTC  :  3; // The mapping from SC28 to the MCTC to be used.
        uint64_t          Reserved_51  :  1; // Reserved
        uint64_t         SC29_TO_MCTC  :  3; // The mapping from SC29 to the MCTC to be used.
        uint64_t          Reserved_55  :  1; // Reserved
        uint64_t         SC30_TO_MCTC  :  3; // The mapping from SC30 to the MCTC to be used.
        uint64_t          Reserved_59  :  1; // Reserved
        uint64_t         SC31_TO_MCTC  :  3; // The mapping from SC31 to the MCTC to be used.
        uint64_t          Reserved_63  :  1; // Reserved
    } field;
    uint64_t val;
} LM_CFG_PORT0_SC2MCTC1_t;

// LM_CONFIG_PORT1 desc:
typedef union {
    struct {
        uint64_t           Reserved_0  :  1; // Enables PKEY checking. Packets that
        uint64_t           LMC_ENABLE  :  1; // Indicates that this port is assigned multiple consecutive DLIDs. Effectively masks low order bits for DLID check.
        uint64_t           Reserved_2  :  1; // Reserved
        uint64_t           Reserved_3  :  1; // Reserved
        uint64_t            LMC_WIDTH  :  4; // Width of the LMC mask (in bits)
        uint64_t                 DLID  : 24; // DLID for this port
        uint64_t       Reserved_43_32  : 12; // Reserved
        uint64_t       Reserved_47_44  :  4; // Reserved
        uint64_t       Reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} LM_CONFIG_PORT1_t;

// LM_FP_TIMER_PORT1 desc:
typedef union {
    struct {
        uint64_t              TIMEOUT  : 48; // Time-out counter threshold to trigger a forward progress drop. Time is inactive when set to 0.
        uint64_t       Reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} LM_FP_TIMER_PORT1_t;

// LM_CFG_PORT1_SL2SC0 desc:
typedef union {
    struct {
        uint64_t            SL0_TO_SC  :  5; // The mapping from SL0 to the SC to be used.
        uint64_t         Reserved_7_5  :  3; // Reserved
        uint64_t            SL1_TO_SC  :  5; // The mapping from SL1 to the SC to be used.
        uint64_t       Reserved_15_13  :  3; // Reserved
        uint64_t            SL2_TO_SC  :  5; // The mapping from SL2 to the SC to be used.
        uint64_t       Reserved_23_21  :  3; // Reserved
        uint64_t            SL3_TO_SC  :  5; // The mapping from SL3 to the SC to be used.
        uint64_t       Reserved_31_29  :  3; // Reserved
        uint64_t            SL4_TO_SC  :  5; // The mapping from SL4 to the SC to be used.
        uint64_t       Reserved_39_37  :  3; // Reserved
        uint64_t            SL5_TO_SC  :  5; // The mapping from SL5 to the SC to be used.
        uint64_t       Reserved_47_45  :  3; // Reserved
        uint64_t            SL6_TO_SC  :  5; // The mapping from SL6 to the SC to be used.
        uint64_t       Reserved_55_53  :  3; // Reserved
        uint64_t            SL7_TO_SC  :  5; // The mapping from SL7 to the SC to be used.
        uint64_t       Reserved_63_61  :  3; // Reserved
    } field;
    uint64_t val;
} LM_CFG_PORT1_SL2SC0_t;

// LM_CFG_PORT1_SL2SC1 desc:
typedef union {
    struct {
        uint64_t            SL8_TO_SC  :  5; // The mapping from SL8 to the SC to be used.
        uint64_t         Reserved_7_5  :  3; // Reserved
        uint64_t            SL9_TO_SC  :  5; // The mapping from SL9 to the SC to be used.
        uint64_t       Reserved_15_13  :  3; // Reserved
        uint64_t           SL10_TO_SC  :  5; // The mapping from SL10 to the SC to be used.
        uint64_t       Reserved_23_21  :  3; // Reserved
        uint64_t           SL11_TO_SC  :  5; // The mapping from SL11 to the SC to be used.
        uint64_t       Reserved_31_29  :  3; // Reserved
        uint64_t           SL12_TO_SC  :  5; // The mapping from SL12 to the SC to be used.
        uint64_t       Reserved_39_37  :  3; // Reserved
        uint64_t           SL13_TO_SC  :  5; // The mapping from SL13 to the SC to be used.
        uint64_t       Reserved_47_45  :  3; // Reserved
        uint64_t           SL14_TO_SC  :  5; // The mapping from SL14 to the SC to be used.
        uint64_t       Reserved_55_53  :  3; // Reserved
        uint64_t           SL15_TO_SC  :  5; // The mapping from SL15 to the SC to be used.
        uint64_t       Reserved_63_61  :  3; // Reserved
    } field;
    uint64_t val;
} LM_CFG_PORT1_SL2SC1_t;

// LM_CFG_PORT1_SL2SC2 desc:
typedef union {
    struct {
        uint64_t           SL16_TO_SC  :  5; // The mapping from SL16 to the SC to be used.
        uint64_t         Reserved_7_5  :  3; // Reserved
        uint64_t           SL17_TO_SC  :  5; // The mapping from SL17 to the SC to be used.
        uint64_t       Reserved_15_13  :  3; // Reserved
        uint64_t           SL18_TO_SC  :  5; // The mapping from SL18 to the SC to be used.
        uint64_t       Reserved_23_21  :  3; // Reserved
        uint64_t           SL19_TO_SC  :  5; // The mapping from SL19 to the SC to be used.
        uint64_t       Reserved_31_29  :  3; // Reserved
        uint64_t           SL20_TO_SC  :  5; // The mapping from SL20 to the SC to be used.
        uint64_t       Reserved_39_37  :  3; // Reserved
        uint64_t           SL21_TO_SC  :  5; // The mapping from SL21 to the SC to be used.
        uint64_t       Reserved_47_45  :  3; // Reserved
        uint64_t           SL22_TO_SC  :  5; // The mapping from SL22 to the SC to be used.
        uint64_t       Reserved_55_53  :  3; // Reserved
        uint64_t           SL23_TO_SC  :  5; // The mapping from SL23 to the SC to be used.
        uint64_t       Reserved_63_61  :  3; // Reserved
    } field;
    uint64_t val;
} LM_CFG_PORT1_SL2SC2_t;

// LM_CFG_PORT1_SL2SC3 desc:
typedef union {
    struct {
        uint64_t           SL24_TO_SC  :  5; // The mapping from SL24 to the SC to be used.
        uint64_t         Reserved_7_5  :  3; // Reserved
        uint64_t           SL25_TO_SC  :  5; // The mapping from SL25 to the SC to be used.
        uint64_t       Reserved_15_13  :  3; // Reserved
        uint64_t           SL26_TO_SC  :  5; // The mapping from SL26 to the SC to be used.
        uint64_t       Reserved_23_21  :  3; // Reserved
        uint64_t           SL27_TO_SC  :  5; // The mapping from SL27 to the SC to be used.
        uint64_t       Reserved_31_29  :  3; // Reserved
        uint64_t           SL28_TO_SC  :  5; // The mapping from SL28 to the SC to be used.
        uint64_t       Reserved_39_37  :  3; // Reserved
        uint64_t           SL29_TO_SC  :  5; // The mapping from SL29 to the SC to be used.
        uint64_t       Reserved_47_45  :  3; // Reserved
        uint64_t           SL30_TO_SC  :  5; // The mapping from SL30 to the SC to be used.
        uint64_t       Reserved_55_53  :  3; // Reserved
        uint64_t           SL31_TO_SC  :  5; // The mapping from SL31 to the SC to be used.
        uint64_t       Reserved_63_61  :  3; // Reserved
    } field;
    uint64_t val;
} LM_CFG_PORT1_SL2SC3_t;

// LM_CFG_PORT1_SC2SL0 desc:
typedef union {
    struct {
        uint64_t            SC0_TO_SL  :  5; // The mapping from SC0 to the SL to be used.
        uint64_t         Reserved_7_5  :  3; // Reserved
        uint64_t            SC1_TO_SL  :  5; // The mapping from SC1 to the SL to be used.
        uint64_t       Reserved_15_13  :  3; // Reserved
        uint64_t            SC2_TO_SL  :  5; // The mapping from SC2 to the SL to be used.
        uint64_t       Reserved_23_21  :  3; // Reserved
        uint64_t            SC3_TO_SL  :  5; // The mapping from SC3 to the SL to be used.
        uint64_t       Reserved_31_29  :  3; // Reserved
        uint64_t            SC4_TO_SL  :  5; // The mapping from SC4 to the SL to be used.
        uint64_t       Reserved_39_37  :  3; // Reserved
        uint64_t            SC5_TO_SL  :  5; // The mapping from SC5 to the SL to be used.
        uint64_t       Reserved_47_45  :  3; // Reserved
        uint64_t            SC6_TO_SL  :  5; // The mapping from SC6 to the SL to be used.
        uint64_t       Reserved_55_53  :  3; // Reserved
        uint64_t            SC7_TO_SL  :  5; // The mapping from SC7 to the SL to be used.
        uint64_t       Reserved_63_61  :  3; // Reserved
    } field;
    uint64_t val;
} LM_CFG_PORT1_SC2SL0_t;

// LM_CFG_PORT1_SC2SL1 desc:
typedef union {
    struct {
        uint64_t            SC8_TO_SL  :  5; // The mapping from SC8 to the SL to be used.
        uint64_t         Reserved_7_5  :  3; // Reserved
        uint64_t            SC9_TO_SL  :  5; // The mapping from SC9 to the SL to be used.
        uint64_t       Reserved_15_13  :  3; // Reserved
        uint64_t           SC10_TO_SL  :  5; // The mapping from SC10 to the SL to be used.
        uint64_t       Reserved_23_21  :  3; // Reserved
        uint64_t           SC11_TO_SL  :  5; // The mapping from SC11 to the SL to be used.
        uint64_t       Reserved_31_29  :  3; // Reserved
        uint64_t           SC12_TO_SL  :  5; // The mapping from SC12 to the SL to be used.
        uint64_t       Reserved_39_37  :  3; // Reserved
        uint64_t           SC13_TO_SL  :  5; // The mapping from SC13 to the SL to be used.
        uint64_t       Reserved_47_45  :  3; // Reserved
        uint64_t           SC14_TO_SL  :  5; // The mapping from SC14 to the SL to be used.
        uint64_t       Reserved_55_53  :  3; // Reserved
        uint64_t           SC15_TO_SL  :  5; // The mapping from SC15 to the SL to be used.
        uint64_t       Reserved_63_61  :  3; // Reserved
    } field;
    uint64_t val;
} LM_CFG_PORT1_SC2SL1_t;

// LM_CFG_PORT1_SC2SL2 desc:
typedef union {
    struct {
        uint64_t           SC16_TO_SL  :  5; // The mapping from SC16 to the SL to be used.
        uint64_t         Reserved_7_5  :  3; // Reserved
        uint64_t           SC17_TO_SL  :  5; // The mapping from SC17 to the SL to be used.
        uint64_t       Reserved_15_13  :  3; // Reserved
        uint64_t           SC18_TO_SL  :  5; // The mapping from SC18 to the SL to be used.
        uint64_t       Reserved_23_21  :  3; // Reserved
        uint64_t           SC19_TO_SL  :  5; // The mapping from SC19 to the SL to be used.
        uint64_t       Reserved_31_29  :  3; // Reserved
        uint64_t           SC20_TO_SL  :  5; // The mapping from SC20 to the SL to be used.
        uint64_t       Reserved_39_37  :  3; // Reserved
        uint64_t           SC21_TO_SL  :  5; // The mapping from SC21 to the SL to be used.
        uint64_t       Reserved_47_45  :  3; // Reserved
        uint64_t           SC22_TO_SL  :  5; // The mapping from SC22 to the SL to be used.
        uint64_t       Reserved_55_53  :  3; // Reserved
        uint64_t           SC23_TO_SL  :  5; // The mapping from SC23 to the SL to be used.
        uint64_t       Reserved_63_61  :  3; // Reserved
    } field;
    uint64_t val;
} LM_CFG_PORT1_SC2SL2_t;

// LM_CFG_PORT1_SC2SL3 desc:
typedef union {
    struct {
        uint64_t           SC24_TO_SL  :  5; // The mapping from SC24 to the SL to be used.
        uint64_t         Reserved_7_5  :  3; // Reserved
        uint64_t           SC25_TO_SL  :  5; // The mapping from SC25 to the SL to be used.
        uint64_t       Reserved_15_13  :  3; // Reserved
        uint64_t           SC26_TO_SL  :  5; // The mapping from SC26 to the SL to be used.
        uint64_t       Reserved_23_21  :  3; // Reserved
        uint64_t           SC27_TO_SL  :  5; // The mapping from SC27 to the SL to be used.
        uint64_t       Reserved_31_29  :  3; // Reserved
        uint64_t           SC28_TO_SL  :  5; // The mapping from SC28 to the SL to be used.
        uint64_t       Reserved_39_37  :  3; // Reserved
        uint64_t           SC29_TO_SL  :  5; // The mapping from SC29 to the SL to be used.
        uint64_t       Reserved_47_45  :  3; // Reserved
        uint64_t           SC30_TO_SL  :  5; // The mapping from SC30 to the SL to be used.
        uint64_t       Reserved_55_53  :  3; // Reserved
        uint64_t           SC31_TO_SL  :  5; // The mapping from SC31 to the SL to be used.
        uint64_t       Reserved_63_61  :  3; // Reserved
    } field;
    uint64_t val;
} LM_CFG_PORT1_SC2SL3_t;

// LM_CFG_PORT1_SC2MCTC0 desc:
typedef union {
    struct {
        uint64_t          SC0_TO_MCTC  :  3; // The mapping from SC0 to the MCTC to be used.
        uint64_t           Reserved_3  :  1; // Reserved
        uint64_t          SC1_TO_MCTC  :  3; // The mapping from SC1 to the MCTC to be used.
        uint64_t           Reserved_7  :  1; // Reserved
        uint64_t          SC2_TO_MCTC  :  3; // The mapping from SC2 to the MCTC to be used.
        uint64_t          Reserved_11  :  1; // Reserved
        uint64_t          SC3_TO_MCTC  :  3; // The mapping from SC3 to the MCTC to be used.
        uint64_t          Reserved_15  :  1; // Reserved
        uint64_t          SC4_TO_MCTC  :  3; // The mapping from SC4 to the MCTC to be used.
        uint64_t          Reserved_19  :  1; // Reserved
        uint64_t          SC5_TO_MCTC  :  3; // The mapping from SC5 to the MCTC to be used.
        uint64_t          Reserved_23  :  1; // Reserved
        uint64_t          SC6_TO_MCTC  :  3; // The mapping from SC6 to the MCTC to be used.
        uint64_t          Reserved_27  :  1; // Reserved
        uint64_t          SC7_TO_MCTC  :  3; // The mapping from SC7 to the MCTC to be used.
        uint64_t          Reserved_31  :  1; // Reserved
        uint64_t          SC8_TO_MCTC  :  3; // The mapping from SC8 to the MCTC to be used.
        uint64_t          Reserved_35  :  1; // Reserved
        uint64_t          SC9_TO_MCTC  :  3; // The mapping from SC9 to the MCTC to be used.
        uint64_t          Reserved_39  :  1; // Reserved
        uint64_t         SC10_TO_MCTC  :  3; // The mapping from SC10 to the MCTC to be used.
        uint64_t          Reserved_43  :  1; // Reserved
        uint64_t         SC11_TO_MCTC  :  3; // The mapping from SC11 to the MCTC to be used.
        uint64_t          Reserved_47  :  1; // Reserved
        uint64_t         SC12_TO_MCTC  :  3; // The mapping from SC12 to the MCTC to be used.
        uint64_t          Reserved_51  :  1; // Reserved
        uint64_t         SC13_TO_MCTC  :  3; // The mapping from SC13 to the MCTC to be used.
        uint64_t          Reserved_55  :  1; // Reserved
        uint64_t         SC14_TO_MCTC  :  3; // The mapping from SC014 to the MCTC to be used.
        uint64_t          Reserved_59  :  1; // Reserved
        uint64_t         SC15_TO_MCTC  :  3; // The mapping from SC15 to the MCTC to be used.
        uint64_t          Reserved_63  :  1; // Reserved
    } field;
    uint64_t val;
} LM_CFG_PORT1_SC2MCTC0_t;

// LM_CFG_PORT1_SC2MCTC1 desc:
typedef union {
    struct {
        uint64_t         SC16_TO_MCTC  :  3; // The mapping from SC0 to the MCTC to be used.
        uint64_t           Reserved_3  :  1; // Reserved
        uint64_t         SC17_TO_MCTC  :  3; // The mapping from SC17 to the MCTC to be used.
        uint64_t           Reserved_7  :  1; // Reserved
        uint64_t         SC18_TO_MCTC  :  3; // The mapping from SC18 to the MCTC to be used.
        uint64_t          Reserved_11  :  1; // Reserved
        uint64_t         SC19_TO_MCTC  :  3; // The mapping from SC19 to the MCTC to be used.
        uint64_t          Reserved_15  :  1; // Reserved
        uint64_t         SC20_TO_MCTC  :  3; // The mapping from SC20 to the MCTC to be used.
        uint64_t          Reserved_19  :  1; // Reserved
        uint64_t         SC21_TO_MCTC  :  3; // The mapping from SC21 to the MCTC to be used.
        uint64_t          Reserved_23  :  1; // Reserved
        uint64_t         SC22_TO_MCTC  :  3; // The mapping from SC22 to the MCTC to be used.
        uint64_t          Reserved_27  :  1; // Reserved
        uint64_t         SC23_TO_MCTC  :  3; // The mapping from SC23 to the MCTC to be used.
        uint64_t          Reserved_31  :  1; // Reserved
        uint64_t         SC24_TO_MCTC  :  3; // The mapping from SC24 to the MCTC to be used.
        uint64_t          Reserved_35  :  1; // Reserved
        uint64_t         SC25_TO_MCTC  :  3; // The mapping from SC25 to the MCTC to be used.
        uint64_t          Reserved_39  :  1; // Reserved
        uint64_t         SC26_TO_MCTC  :  3; // The mapping from SC26 to the MCTC to be used.
        uint64_t          Reserved_43  :  1; // Reserved
        uint64_t         SC27_TO_MCTC  :  3; // The mapping from SC27 to the MCTC to be used.
        uint64_t          Reserved_47  :  1; // Reserved
        uint64_t         SC28_TO_MCTC  :  3; // The mapping from SC28 to the MCTC to be used.
        uint64_t          Reserved_51  :  1; // Reserved
        uint64_t         SC29_TO_MCTC  :  3; // The mapping from SC29 to the MCTC to be used.
        uint64_t          Reserved_55  :  1; // Reserved
        uint64_t         SC30_TO_MCTC  :  3; // The mapping from SC30 to the MCTC to be used.
        uint64_t          Reserved_59  :  1; // Reserved
        uint64_t         SC31_TO_MCTC  :  3; // The mapping from SC31 to the MCTC to be used.
        uint64_t          Reserved_63  :  1; // Reserved
    } field;
    uint64_t val;
} LM_CFG_PORT1_SC2MCTC1_t;

// LM_ERR_STS desc:
typedef union {
    struct {
        uint64_t           DIAGNOSTIC  :  1; // DUMMY PLACEHOLDER
        uint64_t        Reserved_63_1  : 63; // Reserved
    } field;
    uint64_t val;
} LM_ERR_STS_t;

// LM_ERR_CLR desc:
typedef union {
    struct {
        uint64_t           DIAGNOSTIC  :  1; // DUMMY PLACEHOLDER
        uint64_t        Reserved_63_1  : 63; // Reserved
    } field;
    uint64_t val;
} LM_ERR_CLR_t;

// LM_ERR_FRC desc:
typedef union {
    struct {
        uint64_t           DIAGNOSTIC  :  1; // DUMMY PLACEHOLDER
        uint64_t        Reserved_63_1  : 63; // Reserved
    } field;
    uint64_t val;
} LM_ERR_FRC_t;

// LM_ERR_EN_HOST desc:
typedef union {
    struct {
        uint64_t           DIAGNOSTIC  :  1; // DUMMY PLACEHOLDER
        uint64_t        Reserved_63_1  : 63; // Reserved
    } field;
    uint64_t val;
} LM_ERR_EN_HOST_t;

// LM_ERR_FIRST_HOST desc:
typedef union {
    struct {
        uint64_t           DIAGNOSTIC  :  1; // DUMMY PLACEHOLDER
        uint64_t        Reserved_63_1  : 63; // Reserved
    } field;
    uint64_t val;
} LM_ERR_FIRST_HOST_t;

// LM_ERR_EN_BMC desc:
typedef union {
    struct {
        uint64_t           DIAGNOSTIC  :  1; // DUMMY PLACEHOLDER
        uint64_t        Reserved_63_1  : 63; // Reserved
    } field;
    uint64_t val;
} LM_ERR_EN_BMC_t;

// LM_ERR_FIRST_BMC desc:
typedef union {
    struct {
        uint64_t           DIAGNOSTIC  :  1; // DUMMY PLACEHOLDER
        uint64_t        Reserved_63_1  : 63; // Reserved
    } field;
    uint64_t val;
} LM_ERR_FIRST_BMC_t;

// LM_ERR_EN_QUAR desc:
typedef union {
    struct {
        uint64_t           DIAGNOSTIC  :  1; // DUMMY PLACEHOLDER
        uint64_t        Reserved_63_1  : 63; // Reserved
    } field;
    uint64_t val;
} LM_ERR_EN_QUAR_t;

// LM_ERR_FIRST_QUAR desc:
typedef union {
    struct {
        uint64_t           DIAGNOSTIC  :  1; // DUMMY PLACEHOLDER
        uint64_t        Reserved_63_1  : 63; // Reserved
    } field;
    uint64_t val;
} LM_ERR_FIRST_QUAR_t;

// LM_ERR_INFO desc:
typedef union {
    struct {
        uint64_t               STATUS  :  1; // PLACEHOLDER
        uint64_t        Reserved_63_1  : 63; // Reserved
    } field;
    uint64_t val;
} LM_ERR_INFO_t;

#endif /* ___FXR_lm_CSRS_H__ */