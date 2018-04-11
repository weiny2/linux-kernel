// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Thu Apr  5 20:29:40 2018
//

#ifndef ___FXR_lm_CSRS_H__
#define ___FXR_lm_CSRS_H__

// LM_CFG_CONFIG desc:
typedef union {
    struct {
        uint64_t NEAR_LOOPBACK_DISABLE  :  1; // Disables the DLID checking for near loopback
        uint64_t       FORCE_LOOPBACK  :  1; // When set, forces all traffic to loopback (bypassing DLID and PKEY checks)
        uint64_t   Drain_all_incoming  :  1; // When activated, all incoming traffic will be drained from all VL's. This bit should remain active until empty status is indicated by LM_STS_INQ_EMPTY .
        uint64_t        Reserved_63_3  : 61; // Reserved
    } field;
    uint64_t val;
} LM_CFG_CONFIG_t;

// LM_CFG_PERFMON_CTRL desc:
typedef union {
    struct {
        uint64_t      pm_lm_byp_pkt_x  :  4; // Select the MCTC for monitoring the bypass packet for the X counter. default MC0TC0
        uint64_t      pm_lm_byp_pkt_y  :  4; // Select the MCTC for monitoring the bypass packet for the Y counter. default MC1TC0.
        uint64_t      pm_lm_byp_pkt_z  :  4; // Select the MCTC for monitoring the bypass packet for the Z counter. default anyMC.
        uint64_t   pm_lm_byp_qflits_x  :  4; // Select the MCTC for monitoring the bypass qflits for the X counter. default MC0TC0
        uint64_t   pm_lm_byp_qflits_y  :  4; // Select the MCTC for monitoring the bypass qflits for the Y counter. default MC1TC0.
        uint64_t   pm_lm_byp_qflits_z  :  4; // Select the MCTC for monitoring the bypass qflits for the Z counter. default anyMC.
        uint64_t     pm_lm_byp_wait_x  :  4; // Select the MCTC for monitoring the bypass fifo bypass data available but fifo is full for the X counter. default MC0TC0
        uint64_t     pm_lm_byp_wait_y  :  4; // Select the MCTC for monitoring the bypass fifo bypass data available but fifo is full for the Y counter. default MC1TC0
        uint64_t     pm_lm_byp_wait_z  :  4; // Select the MCTC for monitoring the bypass fifo bypass data available but fifo is full for the Z counter. default anyMC.
        uint64_t       Reserved_63_36  : 28; // Reserved
    } field;
    uint64_t val;
} LM_CFG_PERFMON_CTRL_t;

// LM_FXR_SCRATCH desc:
typedef union {
    struct {
        uint64_t              scratch  : 64; // Scratch Registers for Software
    } field;
    uint64_t val;
} LM_FXR_SCRATCH_t;

// LM_CFG_PORT0 desc:
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
} LM_CFG_PORT0_t;

// LM_CFG_FP_TIMER_PORT0 desc:
typedef union {
    struct {
        uint64_t              timeout  : 48; // Time-out counter threshold to trigger a forward progress drop. Time is inactive when set to 0.
        uint64_t        clear_timeout  : 10; // Write a one to the appropriate bit to clear the forward progress timeout for that VL. VL9:0. Hardware will clear this field after clearing the timeout.
        uint64_t       Reserved_63_58  :  6; // Reserved
    } field;
    uint64_t val;
} LM_CFG_FP_TIMER_PORT0_t;

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

// LM_STS_INQ_EMPTY desc:
typedef union {
    struct {
        uint64_t           Link_State  :  3; // LINK_STATE_DOWN = 3'd1 LINK_STATE_INIT = 3'd2 LINK_STATE_ARMED = 3'd3 LINK_STATE_ACTIVE = 3'd4 LINK_STATE_ARMED_EN = 3'd7
        uint64_t           Reserved_3  :  1; // Reserved
        uint64_t             VL_empty  : 10; // There are no packets in the INQ or VL buffers. Bit 0=VL0, Bit 9=VL9.
        uint64_t          Reserved_14  :  1; // Reserved
        uint64_t        All_VLs_empty  :  1; // There are no packets in the INQ or VL buffers
        uint64_t       Reserved_63_16  : 48; // Reserved
    } field;
    uint64_t val;
} LM_STS_INQ_EMPTY_t;

// LM_DBG_LMON_SELECT desc:
typedef union {
    struct {
        uint64_t             sel_64_0  :  4; // LM TX Logic Monitor Select 63:0 of the debug monitor from various buses
        uint64_t             sel_64_1  :  4; // LM TX Logic Monitor Select 127:64 of the debug monitor from various buses
        uint64_t             sel_64_2  :  4; // LM TX Logic Monitor Select 191:128 of the debug monitor from various buses
        uint64_t             sel_64_3  :  4; // LM TX Logic Monitor Select 255:192 of the debug monitor from various buses
        uint64_t             sel_64_4  :  4; // LM TX Logic Monitor Select 319:256 of the debug monitor from various buses
        uint64_t               mon_tc  :  2; // LM TX Logic Monitor TC Select
        uint64_t               mon_mc  :  1; // LM TX Logic Monitor MC Select
        uint64_t      sel_credit_lmon  :  2; // select one out of the 3 logic monitor vector
        uint64_t              lmon_en  :  1; // Enable logic monitor for credit block
        uint64_t     sel_rx_tx_vector  :  1; // Select path for lmon 0: Slect tx path 1: Select Rx path
        uint64_t       Reserved_31_27  :  5; // Reserved
        uint64_t       Reserved_63_32  : 32; // Reserved
    } field;
    uint64_t val;
} LM_DBG_LMON_SELECT_t;

// LM_DBG_PORT_MIRROR desc:
typedef union {
    struct {
        uint64_t             allow_sc  : 32; // Bit 0=SC0...Bit 31=SC31. 0=Block, 1=Allow
        uint64_t          port_mirror  :  1; // Turn on to activate port mirroring in the LM
        uint64_t       Reserved_63_33  : 31; // Reserved
    } field;
    uint64_t val;
} LM_DBG_PORT_MIRROR_t;

// LM_ERR_STS desc:
typedef union {
    struct {
        uint64_t           timeout_p0  :  1; // Port 0 Link timeout
        uint64_t excessive_buffer_overrun  :  1; // Excessive Buffer Overrun Errors. Activated whenever a INQ buffer overrun occurs. Count of overruns is in the Error Info register.
        uint64_t    rx_freelist_0_sbe  :  1; // RX Free List 0 ECC SBE
        uint64_t    rx_freelist_1_sbe  :  1; // RX Free List 1 ECC SBE
        uint64_t    rx_freelist_0_mbe  :  1; // RX Free List 0 ECC MBE
        uint64_t    rx_freelist_1_mbe  :  1; // RX Free List 1 ECC MBE
        uint64_t         rx_inq_sbe_0  :  1; // RX Input Queue Banks 0-7 ECC SBE
        uint64_t         rx_inq_sbe_1  :  1; // RX Input Queue Banks 8-15 ECC SBE
        uint64_t         rx_inq_mbe_0  :  1; // RX Input Queue Banks 0-7 ECC MBE
        uint64_t         rx_inq_mbe_1  :  1; // RX Input Queue Banks 8-15 ECC MBE
        uint64_t     rx_tracker_0_sbe  :  1; // RX Tracker Array 0 ECC SBE
        uint64_t     rx_tracker_1_sbe  :  1; // RX Tracker Array 1 ECC SBE
        uint64_t     rx_tracker_0_mbe  :  1; // RX Tracker Array 0 ECC MBE
        uint64_t     rx_tracker_1_mbe  :  1; // RX Tracker Array 1 ECC MBE
        uint64_t          Always_zero  : 18; // Unused future expansion
        uint64_t payld_info_fifo_overflow_mc0  :  1; // Payload info fifo overflow detected for any of the TC for MC0
        uint64_t payld_info_fifo_underflow_mc0  :  1; // Payload info fifo underflow detected for any of the TC for MC0
        uint64_t       bad_sc_map_mc0  :  1; // sc15 not mapped to vl15 MC0
        uint64_t       bad_sl_map_mc0  :  1; // sc15 not pre translated is illegal MC0
        uint64_t drop_all_vl_packet_mc0  :  1; // packet being dropped because all vl drop set MC0
        uint64_t payld_info_fifo_overflow_mc1  :  1; // Payload info fifo overflow detected for any of the TC for MC1
        uint64_t payld_info_fifo_underflow_mc1  :  1; // Payload info fifo underflow detected for any of the TC for MC1
        uint64_t       bad_sc_map_mc1  :  1; // sc15 not mapped to vl15 mc1
        uint64_t       bad_sl_map_mc1  :  1; // sc15 not pre translated is illegal mc1
        uint64_t drop_all_vl_packet_mc1  :  1; // packet being dropped because all vl drop set mc1
        uint64_t any_vl_buf_underflow  :  1; // any_vl_buffer underflow detected
        uint64_t  any_vl_buf_overflow  :  1; // any_vl_buffer overflow detected
        uint64_t   prd_buff_underflow  :  1; // prd_fifo underflow detected
        uint64_t    prd_buff_overflow  :  1; // prd_fifo overflow detected
        uint64_t rm_info_fifo_par_err  :  1; // rm_info fifo pointer parity error
        uint64_t rm_info_fifo_underflow  :  1; // Rate matching info fifo underflow
        uint64_t rm_info_fifo_overflow  :  1; // Rate matching info fifo overflow
        uint64_t       Dlid_error_mc0  :  1; // Dlid error occurred in MC0
        uint64_t       Dlid_error_mc1  :  1; // Dlid error occurred in MC1
        uint64_t       Reserved_63_51  : 13; // Reserved
    } field;
    uint64_t val;
} LM_ERR_STS_t;

// LM_ERR_CLR desc:
typedef union {
    struct {
        uint64_t               errors  : 51; // Errors matching the ERR_STS CSR
        uint64_t       Reserved_63_51  : 13; // Reserved
    } field;
    uint64_t val;
} LM_ERR_CLR_t;

// LM_ERR_FRC desc:
typedef union {
    struct {
        uint64_t               errors  : 51; // Errors matching the ERR_STS CSR
        uint64_t       Reserved_63_51  : 13; // Reserved
    } field;
    uint64_t val;
} LM_ERR_FRC_t;

// LM_ERR_EN_HOST desc:
typedef union {
    struct {
        uint64_t               errors  : 51; // Errors matching the ERR_STS CSR
        uint64_t       Reserved_63_51  : 13; // Reserved
    } field;
    uint64_t val;
} LM_ERR_EN_HOST_t;

// LM_ERR_FIRST_HOST desc:
typedef union {
    struct {
        uint64_t               errors  : 51; // Errors matching the ERR_STS CSR
        uint64_t       Reserved_63_51  : 13; // Reserved
    } field;
    uint64_t val;
} LM_ERR_FIRST_HOST_t;

// LM_ERR_EN_BMC desc:
typedef union {
    struct {
        uint64_t               errors  : 51; // Errors matching the ERR_STS CSR
        uint64_t       Reserved_63_51  : 13; // Reserved
    } field;
    uint64_t val;
} LM_ERR_EN_BMC_t;

// LM_ERR_FIRST_BMC desc:
typedef union {
    struct {
        uint64_t               errors  : 51; // Errors matching the ERR_STS CSR
        uint64_t       Reserved_63_51  : 13; // Reserved
    } field;
    uint64_t val;
} LM_ERR_FIRST_BMC_t;

// LM_ERR_EN_QUAR desc:
typedef union {
    struct {
        uint64_t               errors  : 51; // Errors matching the ERR_STS CSR
        uint64_t       Reserved_63_51  : 13; // Reserved
    } field;
    uint64_t val;
} LM_ERR_EN_QUAR_t;

// LM_ERR_FIRST_QUAR desc:
typedef union {
    struct {
        uint64_t               errors  : 51; // Errors matching the ERR_STS CSR
        uint64_t       Reserved_63_51  : 13; // Reserved
    } field;
    uint64_t val;
} LM_ERR_FIRST_QUAR_t;

// LM_ERR_INFO1 desc:
typedef union {
    struct {
        uint64_t        inq0_syndrome  :  8; // Syndrome bits for last Input Queue 0 ECC SBE or MBE.
        uint64_t        inq1_syndrome  :  8; // Syndrome bits for last Input Queue 1 ECC SBE or MBE.
        uint64_t         fl0_syndrome  : 16; // Syndrome bits for last Free List 0 ECC SBE or MBE.
        uint64_t         fl1_syndrome  : 16; // Syndrome bits for last Free List 1 ECC SBE or MBE.
        uint64_t       reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} LM_ERR_INFO1_t;

// LM_ERR_INFO2 desc:
typedef union {
    struct {
        uint64_t    tracker0_syndrome  : 10; // Syndrome bits for last Tracker Array 0 ECC SBE or MBE.
        uint64_t    tracker1_syndrome  : 10; // Syndrome bits for last Tracker Array 1 ECC SBE or MBE.
        uint64_t           vl_timeout  : 10; // Indicates which VL caused the Forward Progress Timeout interrupt.
        uint64_t       Reserved_63_30  : 34; // Reserved
    } field;
    uint64_t val;
} LM_ERR_INFO2_t;

// LM_ERR_INFO3 desc:
typedef union {
    struct {
        uint64_t          overrun_cnt  : 64; // Count of the Excessive_Buffer_overruns. Write 0 to clear counter.
    } field;
    uint64_t val;
} LM_ERR_INFO3_t;

// LM_ERR_INJECTION_INBUF desc:
typedef union {
    struct {
        uint64_t                armed  :  1; // Arm Error injection. Injects 1 error at the first opportunity. If this clears then a trigger occurred. Clearing required internally
        uint64_t           buffer_num  :  1; // Introduce error in buffer number 0: introduce error in buffer 0 1: introduce error in buffer 1
        uint64_t           Reserved_2  :  1; // Reserved
        uint64_t             err_type  :  1; // Selects the block in which the error is injected 0 - introduce error in data 1 - introduce error in data parity dont use check byte
        uint64_t         Reserved_5_4  :  2; // Reserved
        uint64_t         Reserved_7_6  :  2; // Reserved
        uint64_t          check_byte0  :  8; // When an error is injected, each bit that is set to one in this field causes the corresponding bit of the error detection syndrome of flit 0 to be inverted.
        uint64_t          check_byte1  :  8; // When an error is injected, each bit that is set to one in this field causes the corresponding bit of the error detection syndrome of flit 1 to be inverted
        uint64_t          check_byte2  :  8; // When an error is injected, each bit that is set to one in this field causes the corresponding bit of the error detection syndrome of flit 2 to be inverted.
        uint64_t          check_byte3  :  8; // When an error is injected, each bit that is set to one in this field causes the corresponding bit of the error detection syndrome of flit 3 to be inverted
        uint64_t       Reserved_63_40  : 24; // 
    } field;
    uint64_t val;
} LM_ERR_INJECTION_INBUF_t;

#endif /* ___FXR_lm_CSRS_H__ */
