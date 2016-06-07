// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Thu May 19 17:39:38 2016
//

#ifndef ___FXR_tx_ci_cid_CSRS_H__
#define ___FXR_tx_ci_cid_CSRS_H__

// TXCID_CFG_AUTHENTICATION_CSR desc:
typedef union {
    struct {
        uint64_t              USER_ID  : 32; // USER_ID
        uint64_t                SRANK  : 32; // SRANK
    } field;
    uint64_t val;
} TXCID_CFG_AUTHENTICATION_CSR_t;

// TXCID_CFG_CSR desc:
typedef union {
    struct {
        uint64_t                  pid  : 12; // Process ID associated with this command queue. Default is to assign access to the kernel (reserved PID 0). Commands must use this PID as their IPID unless the CQ is privileged. See note in CSR description regarding the symmetrical pairing requirements between this and the Rx.
        uint64_t           priv_level  :  1; // Supervisor=0, User=1. Supervisor privilege level is required to Write the DLID relocation table. See note in CSR description regarding the symmetrical pairing requirements between this and the Rx.
        uint64_t               enable  :  1; // A CQ that is not enabled can be written, but slot full counts will always be zero. See note in CSR description regarding the symmetrical pairing requirements between this and the Rx.
        uint64_t          reserved_14  :  1; // Reserved
        uint64_t            phys_dlid  :  1; // Allow physical DLID.
        uint64_t            dlid_base  : 16; // DLID relocation table base address. This value is left shifted based on the DLID relocation table block granularity. After shifting, it is added to the DLID specified in the command. The sum is used to index the DLID relocation table. Not used for Physical DLIDs.
        uint64_t            sl_enable  : 32; // Service level enable, specifying which service levels commands are allowed to use.
    } field;
    uint64_t val;
} TXCID_CFG_CSR_t;

// TXCID_CFG_SL0_TO_TC desc:
typedef union {
    struct {
        uint64_t            sl0_p0_tc  :  2; // Service level 0 and port 0 to traffic class mapping
        uint64_t            sl0_p0_mc  :  1; // Service level 0 and port 0 to message class mapping
        uint64_t                  rsv  :  1; // 
        uint64_t            sl1_p0_tc  :  2; // Service level 1 and port 0 to traffic class mapping
        uint64_t            sl1_p0_mc  :  1; // Service level 1 and port 0 to message class mapping
        uint64_t                rsv_7  :  1; // 
        uint64_t            sl2_p0_tc  :  2; // Service level 2 and port 0 to traffic class mapping
        uint64_t            sl2_p0_mc  :  1; // Service level 2 and port 0 to message class mapping
        uint64_t               rsv_11  :  1; // 
        uint64_t            sl3_p0_tc  :  2; // Service level 3 and port 0 to traffic class mapping
        uint64_t            sl3_p0_mc  :  1; // Service level 3 and port 0 to message class mapping
        uint64_t               rsv_15  :  1; // 
        uint64_t            sl4_p0_tc  :  2; // Service level 4 and port 0 to traffic class mapping
        uint64_t            sl4_p0_mc  :  1; // Service level 4 and port 0 to message class mapping
        uint64_t               rsv_19  :  1; // 
        uint64_t            sl5_p0_tc  :  2; // Service level 5 and port 0 to traffic class mapping
        uint64_t            sl5_p0_mc  :  1; // Service level 5 and port 0 to message class mapping
        uint64_t               rsv_23  :  1; // 
        uint64_t            sl6_p0_tc  :  2; // Service level 6 and port 0 to traffic class mapping
        uint64_t            sl6_p0_mc  :  1; // Service level 6 and port 0 to message class mapping
        uint64_t               rsv_27  :  1; // 
        uint64_t            sl7_p0_tc  :  2; // Service level 7 and port 0 to traffic class mapping
        uint64_t            sl7_p0_mc  :  1; // Service level 7 and port 0 to message class mapping
        uint64_t               rsv_31  :  1; // 
        uint64_t            sl8_p0_tc  :  2; // Service level 8 and port 0 to traffic class
        uint64_t            sl8_p0_mc  :  1; // Service level 8 and port 0 to message class
        uint64_t               rsv_35  :  1; // 
        uint64_t            sl9_p0_tc  :  2; // Service level 9 and port 0 to traffic class
        uint64_t            sl9_p0_mc  :  1; // Service level 9 and port 0 to message class
        uint64_t               rsv_39  :  1; // 
        uint64_t           sl10_p0_tc  :  2; // Service level 10 and port 0 to traffic class
        uint64_t           sl10_p0_mc  :  1; // Service level 10 and port 0 to message class
        uint64_t               rsv_43  :  1; // 
        uint64_t           sl11_p0_tc  :  2; // Service level 11 and port 0 to traffic class
        uint64_t           sl11_p0_mc  :  1; // Service level 11 and port 0 to message class
        uint64_t               rsv_47  :  1; // 
        uint64_t           sl12_p0_tc  :  2; // Service level 12 and port 0 to traffic class
        uint64_t           sl12_p0_mc  :  1; // Service level 12 and port 0 to message class
        uint64_t               rsv_51  :  1; // 
        uint64_t           sl13_p0_tc  :  2; // Service level 13 and port 0 to traffic class
        uint64_t           sl13_p0_mc  :  1; // Service level 13 and port 0 to message class
        uint64_t               rsv_55  :  1; // 
        uint64_t           sl14_p0_tc  :  2; // Service level 14 and port 0 to traffic class
        uint64_t           sl14_p0_mc  :  1; // Service level 14 and port 0 to message class
        uint64_t               rsv_59  :  1; // 
        uint64_t           sl15_p0_tc  :  2; // Service level 15 and port 0 to traffic class
        uint64_t           sl15_p0_mc  :  1; // Service level 15 and port 0 to message class
        uint64_t               rsv_63  :  1; // 
    } field;
    uint64_t val;
} TXCID_CFG_SL0_TO_TC_t;

// TXCID_CFG_SL1_TO_TC desc:
typedef union {
    struct {
        uint64_t           sl16_p0_tc  :  2; // Service level 16 and port 0 to traffic class mapping
        uint64_t           sl16_p0_mc  :  1; // Service level 16 and port 0 to message class mapping
        uint64_t                  rsv  :  1; // 
        uint64_t           sl17_p0_tc  :  2; // Service level 17 and port 0 to traffic class mapping
        uint64_t           sl17_p0_mc  :  1; // Service level 17 and port 0 to message class mapping
        uint64_t                rsv_7  :  1; // 
        uint64_t           sl18_p0_tc  :  2; // Service level 18 and port 0 to traffic class mapping
        uint64_t           sl18_p0_mc  :  1; // Service level 18 and port 0 to message class mapping
        uint64_t               rsv_11  :  1; // 
        uint64_t           sl19_p0_tc  :  2; // Service level 19 and port 0 to traffic class mapping
        uint64_t           sl19_p0_mc  :  1; // Service level 19 and port 0 to message class mapping
        uint64_t               rsv_15  :  1; // 
        uint64_t           sl20_p0_tc  :  2; // Service level 20 and port 0 to traffic class mapping
        uint64_t           sl20_p0_mc  :  1; // Service level 20 and port 0 to message class mapping
        uint64_t               rsv_19  :  1; // 
        uint64_t           sl21_p0_tc  :  2; // Service level 21 and port 0 to traffic class mapping
        uint64_t           sl21_p0_mc  :  1; // Service level 21 and port 0 to message class mapping
        uint64_t               rsv_23  :  1; // 
        uint64_t           sl22_p0_tc  :  2; // Service level 22 and port 0 to traffic class mapping
        uint64_t           sl22_p0_mc  :  1; // Service level 22 and port 0 to message class mapping
        uint64_t               rsv_27  :  1; // 
        uint64_t           sl23_p0_tc  :  2; // Service level 23 and port 0 to traffic class mapping
        uint64_t           sl23_p0_mc  :  1; // Service level 23 and port 0 to message class mapping
        uint64_t               rsv_31  :  1; // 
        uint64_t           sl24_p0_tc  :  2; // Service level 24 and port 0 to traffic class
        uint64_t           sl24_p0_mc  :  1; // Service level 24 and port 0 to message class
        uint64_t               rsv_35  :  1; // 
        uint64_t           sl25_p0_tc  :  2; // Service level 25 and port 0 to traffic class
        uint64_t           sl25_p0_mc  :  1; // Service level 25 and port 0 to message class
        uint64_t               rsv_39  :  1; // 
        uint64_t           sl26_p0_tc  :  2; // Service level 26 and port 0 to traffic class
        uint64_t           sl26_p0_mc  :  1; // Service level 26 and port 0 to message class
        uint64_t               rsv_43  :  1; // 
        uint64_t           sl27_p0_tc  :  2; // Service level 27 and port 0 to traffic class
        uint64_t           sl27_p0_mc  :  1; // Service level 27 and port 0 to message class
        uint64_t               rsv_47  :  1; // 
        uint64_t           sl28_p0_tc  :  2; // Service level 28 and port 0 to traffic class
        uint64_t           sl28_p0_mc  :  1; // Service level 28 and port 0 to message class
        uint64_t               rsv_51  :  1; // 
        uint64_t           sl29_p0_tc  :  2; // Service level 29 and port 0 to traffic class
        uint64_t           sl29_p0_mc  :  1; // Service level 29 and port 0 to message class
        uint64_t               rsv_55  :  1; // 
        uint64_t           sl30_p0_tc  :  2; // Service level 30 and port 0 to traffic class
        uint64_t           sl30_p0_mc  :  1; // Service level 30 and port 0 to message class
        uint64_t               rsv_59  :  1; // 
        uint64_t           sl31_p0_tc  :  2; // Service level 31 and port 0 to traffic class
        uint64_t           sl31_p0_mc  :  1; // Service level 31 and port 0 to message class
        uint64_t               rsv_63  :  1; // 
    } field;
    uint64_t val;
} TXCID_CFG_SL1_TO_TC_t;

// TXCID_CFG_SL2_TO_TC desc:
typedef union {
    struct {
        uint64_t            sl0_p1_tc  :  2; // Service level 0 and port 1 to traffic class mapping
        uint64_t            sl0_p1_mc  :  1; // Service level 0 and port 1 to message class mapping
        uint64_t                  rsv  :  1; // 
        uint64_t            sl1_p1_tc  :  2; // Service level 1 and port 1 to traffic class mapping
        uint64_t            sl1_p1_mc  :  1; // Service level 1 and port 1 to message class mapping
        uint64_t                rsv_7  :  1; // 
        uint64_t            sl2_p1_tc  :  2; // Service level 2 and port 1 to traffic class mapping
        uint64_t            sl2_p1_mc  :  1; // Service level 2 and port 1 to message class mapping
        uint64_t               rsv_11  :  1; // 
        uint64_t            sl3_p1_tc  :  2; // Service level 3 and port 1 to traffic class mapping
        uint64_t            sl3_p1_mc  :  1; // Service level 3 and port 1 to message class mapping
        uint64_t               rsv_15  :  1; // 
        uint64_t            sl4_p1_tc  :  2; // Service level 4 and port 1 to traffic class mapping
        uint64_t            sl4_p1_mc  :  1; // Service level 4 and port 1 to message class mapping
        uint64_t               rsv_19  :  1; // 
        uint64_t            sl5_p1_tc  :  2; // Service level 5 and port 1 to traffic class mapping
        uint64_t            sl5_p1_mc  :  1; // Service level 5 and port 1 to message class mapping
        uint64_t               rsv_23  :  1; // 
        uint64_t            sl6_p1_tc  :  2; // Service level 6 and port 1 to traffic class mapping
        uint64_t            sl6_p1_mc  :  1; // Service level 6 and port 1 to message class mapping
        uint64_t               rsv_27  :  1; // 
        uint64_t            sl7_p1_tc  :  2; // Service level 7 and port 1 to traffic class mapping
        uint64_t            sl7_p1_mc  :  1; // Service level 7 and port 1 to message class mapping
        uint64_t               rsv_31  :  1; // 
        uint64_t            sl8_p1_tc  :  2; // Service level 8 and port 1 to traffic class
        uint64_t            sl8_p1_mc  :  1; // Service level 8 and port 1 to message class
        uint64_t               rsv_35  :  1; // 
        uint64_t            sl9_p1_tc  :  2; // Service level 9 and port 1 to traffic class
        uint64_t            sl9_p1_mc  :  1; // Service level 9 and port 1 to message class
        uint64_t               rsv_39  :  1; // 
        uint64_t           sl10_p1_tc  :  2; // Service level 10 and port 1 to traffic class
        uint64_t           sl10_p1_mc  :  1; // Service level 10 and port 1 to message class
        uint64_t               rsv_43  :  1; // 
        uint64_t           sl11_p1_tc  :  2; // Service level 11 and port 1 to traffic class
        uint64_t           sl11_p1_mc  :  1; // Service level 11 and port 1 to message class
        uint64_t               rsv_47  :  1; // 
        uint64_t           sl12_p1_tc  :  2; // Service level 12 and port 1 to traffic class
        uint64_t           sl12_p1_mc  :  1; // Service level 12 and port 1 to message class
        uint64_t               rsv_51  :  1; // 
        uint64_t           sl13_p1_tc  :  2; // Service level 13 and port 1 to traffic class
        uint64_t           sl13_p1_mc  :  1; // Service level 13 and port 1 to message class
        uint64_t               rsv_55  :  1; // 
        uint64_t           sl14_p1_tc  :  2; // Service level 14 and port 1 to traffic class
        uint64_t           sl14_p1_mc  :  1; // Service level 14 and port 1 to message class
        uint64_t               rsv_59  :  1; // 
        uint64_t           sl15_p1_tc  :  2; // Service level 15 and port 1 to traffic class
        uint64_t           sl15_p1_mc  :  1; // Service level 15 and port 1 to message class
        uint64_t               rsv_63  :  1; // 
    } field;
    uint64_t val;
} TXCID_CFG_SL2_TO_TC_t;

// TXCID_CFG_SL3_TO_TC desc:
typedef union {
    struct {
        uint64_t           sl16_p1_tc  :  2; // Service level 16 and port 1 to traffic class mapping
        uint64_t           sl16_p1_mc  :  1; // Service level 16 and port 1 to message class mapping
        uint64_t                  rsv  :  1; // 
        uint64_t           sl17_p1_tc  :  2; // Service level 17 and port 1 to traffic class mapping
        uint64_t           sl17_p1_mc  :  1; // Service level 17 and port 1 to message class mapping
        uint64_t                rsv_7  :  1; // 
        uint64_t           sl18_p1_tc  :  2; // Service level 18 and port 1 to traffic class mapping
        uint64_t           sl18_p1_mc  :  1; // Service level 18 and port 1 to message class mapping
        uint64_t               rsv_11  :  1; // 
        uint64_t           sl19_p1_tc  :  2; // Service level 19 and port 1 to traffic class mapping
        uint64_t           sl19_p1_mc  :  1; // Service level 19 and port 1 to message class mapping
        uint64_t               rsv_15  :  1; // 
        uint64_t           sl20_p1_tc  :  2; // Service level 20 and port 1 to traffic class mapping
        uint64_t           sl20_p1_mc  :  1; // Service level 20 and port 1 to message class mapping
        uint64_t               rsv_19  :  1; // 
        uint64_t           sl21_p1_tc  :  2; // Service level 21 and port 1 to traffic class mapping
        uint64_t           sl21_p1_mc  :  1; // Service level 21 and port 1 to message class mapping
        uint64_t               rsv_23  :  1; // 
        uint64_t           sl22_p1_tc  :  2; // Service level 22 and port 1 to traffic class mapping
        uint64_t           sl22_p1_mc  :  1; // Service level 22 and port 1 to message class mapping
        uint64_t               rsv_27  :  1; // 
        uint64_t           sl23_p1_tc  :  2; // Service level 23 and port 1 to traffic class mapping
        uint64_t           sl23_p1_mc  :  1; // Service level 23 and port 1 to message class mapping
        uint64_t               rsv_31  :  1; // 
        uint64_t           sl24_p1_tc  :  2; // Service level 24 and port 1 to traffic class
        uint64_t           sl24_p1_mc  :  1; // Service level 24 and port 1 to message class
        uint64_t               rsv_35  :  1; // 
        uint64_t           sl25_p1_tc  :  2; // Service level 25 and port 1 to traffic class
        uint64_t           sl25_p1_mc  :  1; // Service level 25 and port 1 to message class
        uint64_t               rsv_39  :  1; // 
        uint64_t           sl26_p1_tc  :  2; // Service level 26 and port 1 to traffic class
        uint64_t           sl26_p1_mc  :  1; // Service level 26 and port 1 to message class
        uint64_t               rsv_43  :  1; // 
        uint64_t           sl27_p1_tc  :  2; // Service level 27 and port 1 to traffic class
        uint64_t           sl27_p1_mc  :  1; // Service level 27 and port 1 to message class
        uint64_t               rsv_47  :  1; // 
        uint64_t           sl28_p1_tc  :  2; // Service level 28 and port 1 to traffic class
        uint64_t           sl28_p1_mc  :  1; // Service level 28 and port 1 to message class
        uint64_t               rsv_51  :  1; // 
        uint64_t           sl29_p1_tc  :  2; // Service level 29 and port 1 to traffic class
        uint64_t           sl29_p1_mc  :  1; // Service level 29 and port 1 to message class
        uint64_t               rsv_55  :  1; // 
        uint64_t           sl30_p1_tc  :  2; // Service level 30 and port 1 to traffic class
        uint64_t           sl30_p1_mc  :  1; // Service level 30 and port 1 to message class
        uint64_t               rsv_59  :  1; // 
        uint64_t           sl31_p1_tc  :  2; // Service level 31 and port 1 to traffic class
        uint64_t           sl31_p1_mc  :  1; // Service level 31 and port 1 to message class
        uint64_t               rsv_63  :  1; // 
    } field;
    uint64_t val;
} TXCID_CFG_SL3_TO_TC_t;

// TXCID_CFG_MAD_TO_TC desc:
typedef union {
    struct {
        uint64_t                   tc  :  2; // Traffic class for management packets
        uint64_t                   mc  :  1; // Message class for management packets
        uint64_t        Reserved_63_3  : 61; // Unused
    } field;
    uint64_t val;
} TXCID_CFG_MAD_TO_TC_t;

// TXCID_CREDITS_TO_RX desc:
typedef union {
    struct {
        uint64_t               mc0tc0  :  4; // MC0/TC0 Fast Path OPB Input Queue Credits
        uint64_t               mc0tc1  :  4; // MC0/TC1 Fast Path OPB Input Queue Credits
        uint64_t               mc0tc2  :  4; // MC0/TC2 Fast Path OPB Input Queue Credits
        uint64_t               mc0tc3  :  4; // MC0/TC3 Fast Path OPB Input Queue Credits
        uint64_t               mc1tc0  :  4; // MC1/TC0 Fast Path OPB Input Queue Credits
        uint64_t               mc1tc1  :  4; // MC1/TC1 Fast Path OPB Input Queue Credits
        uint64_t               mc1tc2  :  4; // MC1/TC2 Fast Path OPB Input Queue Credits
        uint64_t               mc1tc3  :  4; // MC1/TC3 Fast Path OPB Input Queue Credits
        uint64_t              mc1ptc0  :  4; // MC1'/TC0 Fast Path OPB Input Queue Credits
        uint64_t              mc1ptc1  :  4; // MC1'/TC1 Fast Path OPB Input Queue Credits
        uint64_t              mc1ptc2  :  4; // MC1'/TC2 Fast Path OPB Input Queue Credits
        uint64_t              mc1ptc3  :  4; // MC1'/TC3 Fast Path OPB Input Queue Credits
        uint64_t         unused_63_48  : 16; // Unused
    } field;
    uint64_t val;
} TXCID_CREDITS_TO_RX_t;

// TXCID_CFG_HI_CRDTS desc:
typedef union {
    struct {
        uint64_t                  tc0  :  5; // Request Input Queue Credits
        uint64_t          unused_63_5  : 59; // Unused
    } field;
    uint64_t val;
} TXCID_CFG_HI_CRDTS_t;

// TXCID_CFG_RX_MC0_CRDTS desc:
typedef union {
    struct {
        uint64_t                  tc0  :  5; // TC0 Input Queue Credits
        uint64_t           unused_7_5  :  3; // Unused
        uint64_t                  tc1  :  5; // TC1 Input Queue Credits
        uint64_t         unused_15_13  :  3; // Unused
        uint64_t                  tc2  :  5; // TC2 Input Queue Credits
        uint64_t         unused_23_21  :  3; // Unused
        uint64_t                  tc3  :  5; // TC3 Input Queue Credits
        uint64_t         unused_63_29  : 35; // Unused
    } field;
    uint64_t val;
} TXCID_CFG_RX_MC0_CRDTS_t;

// TXCID_CFG_RX_MC1_CRDTS desc:
typedef union {
    struct {
        uint64_t                  tc0  :  5; // TC0 Input Queue Credits
        uint64_t           unused_7_5  :  3; // Unused
        uint64_t                  tc1  :  5; // TC1 Input Queue Credits
        uint64_t         unused_15_13  :  3; // Unused
        uint64_t                  tc2  :  5; // TC2 Input Queue Credits
        uint64_t         unused_23_21  :  3; // Unused
        uint64_t                  tc3  :  5; // TC3 Input Queue Credits
        uint64_t         unused_63_29  : 35; // Unused
    } field;
    uint64_t val;
} TXCID_CFG_RX_MC1_CRDTS_t;

// TXCID_CFG_RX_MC1P_CRDTS desc:
typedef union {
    struct {
        uint64_t                  tc0  :  5; // TC0 Input Queue Credits
        uint64_t           unused_7_5  :  3; // Unused
        uint64_t                  tc1  :  5; // TC1 Input Queue Credits
        uint64_t         unused_15_13  :  3; // Unused
        uint64_t                  tc2  :  5; // TC2 Input Queue Credits
        uint64_t         unused_23_21  :  3; // Unused
        uint64_t                  tc3  :  5; // TC3 Input Queue Credits
        uint64_t         unused_63_29  : 35; // Unused
    } field;
    uint64_t val;
} TXCID_CFG_RX_MC1P_CRDTS_t;

// TXCID_CFG_CQ_COUNT desc:
typedef union {
    struct {
        uint64_t                count  :  8; // Number of command queues
        uint64_t          unused_63_8  : 56; // Unused
    } field;
    uint64_t val;
} TXCID_CFG_CQ_COUNT_t;

// TXCID_CFG_SENDBTHQP desc:
typedef union {
    struct {
        uint64_t            SendBthQP  :  8; // Compared against BTH.DestQP[23:16] to determine KDETH packets
        uint64_t               enable  :  1; // Enable sending of KDETH packets
        uint64_t          unused_63_9  : 55; // Unused
    } field;
    uint64_t val;
} TXCID_CFG_SENDBTHQP_t;

// TXCID_CFG_DLID_GRANULARITY desc:
typedef union {
    struct {
        uint64_t          granularity  :  4; // DLID replacement granularity 0--1 1--2 2--4 3--8 4--16 5--32 6--64 7--128 8--256
        uint64_t                  rsv  : 60; // Reserved
    } field;
    uint64_t val;
} TXCID_CFG_DLID_GRANULARITY_t;

// TXCID_CFG_DLID_RT_ADDR desc:
typedef union {
    struct {
        uint64_t              address  : 52; // Address of the Transmit DLID table to be accessed
        uint64_t         payload_regs  :  8; // Constant indicating number of payload registers used for the data.
        uint64_t       Reserved_61_60  :  2; // 
        uint64_t                Write  :  1; // 0=Read, 1=Write
        uint64_t                Valid  :  1; // Set by Software to start command, cleared by Hardware when complete
    } field;
    uint64_t val;
} TXCID_CFG_DLID_RT_ADDR_t;

// TXCID_CFG_DLID_RT_DATA desc:
typedef union {
    struct {
        uint64_t        BLK_PHYS_DLID  : 24; // Block physical DLID
        uint64_t       RPLC_PHYS_DLID  : 24; // Replacement physical DLID
        uint64_t           RPLC_MATCH  :  8; // Address match for replacement
        uint64_t        RPLC_BLK_SIZE  :  3; // Replacement block size. 0--1 1--2 2--4 3--8 4--16 5--32 6--64 7--128 RPLC_BLK_SIZE should be less than granularity
        uint64_t             PORT_ONE  :  1; // Network port to use if SINGLE_PORT is set.
        uint64_t          SINGLE_PORT  :  1; // Access to DLID is available through one network port only.
        uint64_t                  RSV  :  3; // Reserved
    } field;
    uint64_t val;
} TXCID_CFG_DLID_RT_DATA_t;

// TXCID_STS_CQ_RW_CSR desc:
typedef union {
    struct {
        uint64_t           txci_qword  : 64; // Quad word in the transmit command queue.
    } field;
    uint64_t val;
} TXCID_STS_CQ_RW_CSR_t;

// TXCID_ERR_STS desc:
typedef union {
    struct {
        uint64_t           diagnostic  :  1; // Diagnostic Error Flag
        uint64_t              timeout  :  1; // Timeout of something
        uint64_t                  sbe  :  1; // Correctable SBE. Error information: TXCID_ERR_INFO_SBE_MBE
        uint64_t                  mbe  :  1; // Uncorrectable MBE. Error information: TXCID_ERR_INFO_SBE_MBE .
        uint64_t        Reserved_63_4  : 60; // Reserved
    } field;
    uint64_t val;
} TXCID_ERR_STS_t;

// TXCID_ERR_CLR desc:
typedef union {
    struct {
        uint64_t               events  :  4; // Write 1's to clear corresponding status bits.
        uint64_t        Reserved_63_4  : 60; // Reserved
    } field;
    uint64_t val;
} TXCID_ERR_CLR_t;

// TXCID_ERR_FRC desc:
typedef union {
    struct {
        uint64_t               events  :  4; // Write 1 to set corresponding status bits.
        uint64_t        Reserved_63_4  : 60; // Reserved
    } field;
    uint64_t val;
} TXCID_ERR_FRC_t;

// TXCID_ERR_EN_HOST desc:
typedef union {
    struct {
        uint64_t               events  :  4; // Enables corresponding status bits to generate host interrupt signal.
        uint64_t        Reserved_63_4  : 60; // Reserved
    } field;
    uint64_t val;
} TXCID_ERR_EN_HOST_t;

// TXCID_ERR_FIRST_HOST desc:
typedef union {
    struct {
        uint64_t               events  :  4; // Snapshot of status bits when host interrupt signal transitions from 0 to 1.
        uint64_t        Reserved_63_4  : 60; // Reserved
    } field;
    uint64_t val;
} TXCID_ERR_FIRST_HOST_t;

// TXCID_ERR_EN_BMC desc:
typedef union {
    struct {
        uint64_t               events  :  4; // Enable corresponding status bits to generate BMC interrupt signal.
        uint64_t        Reserved_63_4  : 60; // Reserved
    } field;
    uint64_t val;
} TXCID_ERR_EN_BMC_t;

// TXCID_ERR_FIRST_BMC desc:
typedef union {
    struct {
        uint64_t               events  :  4; // Snapshot of status bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t        Reserved_63_4  : 60; // Reserved
    } field;
    uint64_t val;
} TXCID_ERR_FIRST_BMC_t;

// TXCID_ERR_EN_QUAR desc:
typedef union {
    struct {
        uint64_t               events  :  4; // Enable corresponding status bits to generate quarantine signal.
        uint64_t        Reserved_63_4  : 60; // Reserved
    } field;
    uint64_t val;
} TXCID_ERR_EN_QUAR_t;

// TXCID_ERR_FIRST_QUAR desc:
typedef union {
    struct {
        uint64_t               events  :  4; // Snapshot of status bits when quarantine signal transitions from 0 to 1.
        uint64_t        Reserved_63_4  : 60; // Reserved
    } field;
    uint64_t val;
} TXCID_ERR_FIRST_QUAR_t;

// TXCID_ERR_INFO_SBE_MBE desc:
typedef union {
    struct {
        uint64_t         syndrome_sbe  :  8; // syndrome of the last sbe
        uint64_t         syndrome_mbe  :  8; // syndrome of the last mbe
        uint64_t       Reserved_63_16  : 48; // Reserved
    } field;
    uint64_t val;
} TXCID_ERR_INFO_SBE_MBE_t;

#endif /* ___FXR_tx_ci_cid_CSRS_H__ */