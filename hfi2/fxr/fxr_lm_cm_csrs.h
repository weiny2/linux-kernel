// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Thu Jun  2 19:11:24 2016
//

#ifndef ___FXR_lm_cm_CSRS_H__
#define ___FXR_lm_cm_CSRS_H__

// TP_CFG_CM_RESET desc:
typedef union {
    struct {
        uint64_t             UNUSED_0  :  1; // 
        uint64_t           soft_reset  :  1; // Software reset of TPORT block, excluding Tag buffer block. Tagbuf soft reset is in CSR 0xCE00, if the tagbuf is being soft reset as well, it should be set prior to Tport soft_reset. 1 = Resets TPORT block, excluding Tagbuf. 0 = Has no affect Reset does not affect TPORT common registers (CSR's). This bit is read as zero.
        uint64_t      crdt_rtrn_reset  : 10; // Software reset of Credit return block. 1 = Clears the accumulating credit counters in the credit return block, one bit per VL. 0 = Has no affect This bit is read as zero.
        uint64_t         UNUSED_63_12  : 52; // 
    } field;
    uint64_t val;
} TP_CFG_CM_RESET_t;

// TP_CFG_CM_CRDT_SEND desc:
typedef union {
    struct {
        uint64_t                   AU  :  3; // The value is written by firmware with the allocation unit (vAU) received from a remote device 0 = 8 Bytes Not supported 1 = 16 Bytes 2 = 32 Bytes 3 = 64 Bytes 4 = 128 Bytes 5 = 256 Bytes 6 = 512 Bytes 7 = 1k Bytes
        uint64_t             UNUSED_3  :  1; // 
        uint64_t           VL15_Index  :  3; // If non-zero, this is used to select one of the entries in the local table. This only affects VL15.
        uint64_t             UNUSED_7  :  1; // 
        uint64_t       SideBand_limit  :  2; // Number of entries allowed in the sideband Fifo. Limit is 2.
        uint64_t        Force_CR_Mode  :  1; // If clear force the credit return mode to 'credit return control flits', if set, force sideband credit control.
        uint64_t         UNUSED_15_11  :  5; // 
        uint64_t Return_Credit_Status  : 10; // VL15, VL8:0 return credit status. A value of 1 indicates that the associated VL is in shared space. This is normally used when during credit reallocation
        uint64_t  crdt_updt_threshold  :  4; // Number of clock cycles from the time a request is made until the credits are updated.
        uint64_t         UNUSED_63_30  : 34; // 
    } field;
    uint64_t val;
} TP_CFG_CM_CRDT_SEND_t;

// TP_CFG_CM_CRDT_RETURN_TIMER desc:
typedef union {
    struct {
        uint64_t                Count  : 32; // Count 1 to 4 billion clock cycles
        uint64_t         UNUSED_63_32  : 32; // 
    } field;
    uint64_t val;
} TP_CFG_CM_CRDT_RETURN_TIMER_t;

// TP_CFG_CM_CRDT_VL15_DOS_TIMER desc:
typedef union {
    struct {
        uint64_t                Count  : 32; // Count 1 to 4 billion clock cycles
        uint64_t         UNUSED_63_32  : 32; // 
    } field;
    uint64_t val;
} TP_CFG_CM_CRDT_VL15_DOS_TIMER_t;

// TP_CFG_CM_LOCAL_CRDT_RETURN_TBL0 desc:
typedef union {
    struct {
        uint64_t               Entry0  : 14; // Local return credit value n CU*CR
        uint64_t         UNUSED_15_14  :  2; // 
        uint64_t               Entry1  : 14; // Local return credit value n+1 CU*CR
        uint64_t         UNUSED_31_30  :  2; // 
        uint64_t               Entry2  : 14; // Local return credit value n+2 CU*CR
        uint64_t         UNUSED_47_46  :  2; // 
        uint64_t               Entry3  : 14; // Local return credit value n+3 CU*CR
        uint64_t         UNUSED_63_62  :  2; // 
    } field;
    uint64_t val;
} TP_CFG_CM_LOCAL_CRDT_RETURN_TBL0_t;

// TP_CFG_CM_LOCAL_CRDT_RETURN_TBL1 desc:
typedef union {
    struct {
        uint64_t               Entry4  : 14; // Local return credit value n CU*CR
        uint64_t         UNUSED_15_14  :  2; // 
        uint64_t               Entry5  : 14; // Local return credit value n+1 CU*CR
        uint64_t         UNUSED_31_30  :  2; // 
        uint64_t               Entry6  : 14; // Local return credit value n+2 CU*CR
        uint64_t         UNUSED_47_46  :  2; // 
        uint64_t               Entry7  : 14; // Local return credit value n+3 CU*CR
        uint64_t         UNUSED_63_62  :  2; // 
    } field;
    uint64_t val;
} TP_CFG_CM_LOCAL_CRDT_RETURN_TBL1_t;

// TP_CFG_CM_REMOTE_CRDT_RETURN_TBL0 desc:
typedef union {
    struct {
        uint64_t               Entry0  : 14; // Local return credit value n CU*CR
        uint64_t         UNUSED_15_14  :  2; // 
        uint64_t               Entry1  : 14; // Local return credit value n+1 CU*CR
        uint64_t         UNUSED_31_30  :  2; // 
        uint64_t               Entry2  : 14; // Local return credit value n CU*CR
        uint64_t         UNUSED_47_46  :  2; // 
        uint64_t               Entry3  : 14; // Local return credit value n+1 CU*CR
        uint64_t         UNUSED_63_62  :  2; // 
    } field;
    uint64_t val;
} TP_CFG_CM_REMOTE_CRDT_RETURN_TBL0_t;

// TP_CFG_CM_REMOTE_CRDT_RETURN_TBL1 desc:
typedef union {
    struct {
        uint64_t               Entry4  : 14; // Local return credit value n CU*CR
        uint64_t         UNUSED_15_14  :  2; // 
        uint64_t               Entry5  : 14; // Local return credit value n+1 CU*CR
        uint64_t         UNUSED_31_30  :  2; // 
        uint64_t               Entry6  : 14; // Local return credit value n CU*CR
        uint64_t         UNUSED_47_46  :  2; // 
        uint64_t               Entry7  : 14; // Local return credit value n+1 CU*CR
        uint64_t         UNUSED_63_62  :  2; // 
    } field;
    uint64_t val;
} TP_CFG_CM_REMOTE_CRDT_RETURN_TBL1_t;

// TP_CFG_CM_SC_TO_VLT_MAP desc:
typedef union {
    struct {
        uint64_t               entry0  :  4; // SC n+0 to VL
        uint64_t               entry1  :  4; // SC n+1 to VL
        uint64_t               entry2  :  4; // SC n+2 to VL
        uint64_t               entry3  :  4; // SC n+3 to VL
        uint64_t               entry4  :  4; // SC n+4 to VL
        uint64_t               entry5  :  4; // SC n+5 to VL
        uint64_t               entry6  :  4; // SC n+6 to VL
        uint64_t               entry7  :  4; // SC n+7 to VL
        uint64_t               entry8  :  4; // SC n+8 to VL
        uint64_t               entry9  :  4; // SC n+9 to VL
        uint64_t              entry10  :  4; // SC n+10 to VL
        uint64_t              entry11  :  4; // SC n+11 to VL
        uint64_t              entry12  :  4; // SC n+12 to VL
        uint64_t              entry13  :  4; // SC n+13 to VL
        uint64_t              entry14  :  4; // SC n+14 to VL
        uint64_t              entry15  :  4; // SC n+15 to VL
    } field;
    uint64_t val;
} TP_CFG_CM_SC_TO_VLT_MAP_t;

// TP_CFG_CM_DEDICATED_VL_CRDT_LIMIT desc:
typedef union {
    struct {
        uint64_t            Dedicated  : 16; // VL dedicated remote credit limit. Range 0 to 65535 remote credits (PRR limit=2044) Note: Cannot exceed Total_Credit_Limit
        uint64_t         UNUSED_63_16  : 48; // 
    } field;
    uint64_t val;
} TP_CFG_CM_DEDICATED_VL_CRDT_LIMIT_t;

// TP_CFG_CM_SHARED_VL_CRDT_LIMIT desc:
typedef union {
    struct {
        uint64_t               Shared  : 16; // VL shared remote credit pool limit. Range 0 to 65535 remote credits (PRR limit=2044) Note: Cannot exceed Total_Credit_Limit Note: Cannot exceed Global_Credit_Limit
        uint64_t         UNUSED_63_16  : 48; // 
    } field;
    uint64_t val;
} TP_CFG_CM_SHARED_VL_CRDT_LIMIT_t;

// TP_CFG_CM_GLOBAL_SHRD_CRDT_LIMIT desc:
typedef union {
    struct {
        uint64_t               Shared  : 16; // Total global shared remote credit limit. Range 0 to 65535 remote credits Note: Cannot exceed Total_Credit_Limit
        uint64_t   Total_Credit_Limit  : 16; // Total remote AU limit. Range is 0 to 65535. If total dedicated used + total shared used AUs exceeds this value, bit Credit_Overflow is set in CSR INT_STATUS_1. The default is set to the PRR limit of 2044 since PRR uses 64 byte AUs. WFR limit is 2368 since WFR uses 64 Byte AUs.
        uint64_t         UNUSED_63_32  : 32; // 
    } field;
    uint64_t val;
} TP_CFG_CM_GLOBAL_SHRD_CRDT_LIMIT_t;

// TP_DBG_CM_VL_CRDTS_USED desc:
typedef union {
    struct {
        uint64_t            Dedicated  : 16; // Per VL, total number of dedicated credits used in the remote RBUF dedicated pool. Range 0 to 65535 remote credits (PRR limit=2044) Note: Cannot exceed Total_Credit_Limit Note: Cannot exceed Allocated_ Dedicated_Credit
        uint64_t               Shared  : 16; // Per VL, total number of shared credits used in the remote RBUF shared pool. Range 0 to 65535 remote credits (PRR limit=2044) Note: Cannot exceed Total_Credit_Limit Note: Cannot exceed Global_Credit_Limit Note: Cannot exceed Allocated_ Shared_Credit
        uint64_t         UNUSED_63_32  : 32; // 
    } field;
    uint64_t val;
} TP_DBG_CM_VL_CRDTS_USED_t;

// TP_DBG_CM_GLOBAL_SHRD_CRDTS_USED desc:
typedef union {
    struct {
        uint64_t               Shared  : 16; // Total number of remote shared AUs used, includes all VLs. Range 0 to 65535 remote AUs (PRR limit=2044) Note: Cannot exceed Total_Credit_Limit
        uint64_t         UNUSED_63_16  : 48; // 
    } field;
    uint64_t val;
} TP_DBG_CM_GLOBAL_SHRD_CRDTS_USED_t;

// TP_DBG_CM_TOTAL_CRDTS_USED desc:
typedef union {
    struct {
        uint64_t            Dedicated  : 16; // Total number of dedicated credits used in the remote RBUF dedicated pool, includes all VLs. Note: Cannot exceed Total_Credit_Limit
        uint64_t               Shared  : 16; // Total number of shared credits used in the remote RBUF shared pool, includes all VLs. Note: Cannot exceed Total_Credit_Limit Note: Cannot exceed Global_Credit_Limit
        uint64_t         UNUSED_63_32  : 32; // 
    } field;
    uint64_t val;
} TP_DBG_CM_TOTAL_CRDTS_USED_t;

#endif /* ___FXR_lm_cm_CSRS_H__ */