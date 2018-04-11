// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Thu Apr  5 21:41:55 2018
//

#ifndef ___FXR_tx_ci_cid_CSRS_H__
#define ___FXR_tx_ci_cid_CSRS_H__

// TXCID_CFG_AUTHENTICATION_CSR desc:
typedef union {
    struct {
        uint64_t              USER_ID  : 32; // For Portals commands, this field contains the USER_ID that is inserted into the packet. For KDETH packets, this field contains the jkey (user_id[15:0]) and jkey_mask (user_id[31:16]) to be used to validate the KDETH command. The jkey in the entry is compared with the jkey from the command, and is masked with the mask bits (1 means ignore). In case of masked mis-compare, the packet will be failed and sent to otr. the error and cq_num_will be flagged.
        uint64_t                SRANK  : 32; // If this field is set to PTL_RANK_ANY, the entry is considered a KDETH entry and Portals commands that target the entry are failed. If this field is set to another value (not PTL_RANK_ANY), the entry is considered a Portals entry and KDETH commands that target the entry are failed. For Portals commands, this field contains the SRANK that is inserted into the packet.
    } field;
    uint64_t val;
} TXCID_CFG_AUTHENTICATION_CSR_t;

// TXCID_CFG_CSR desc:
typedef union {
    struct {
        uint64_t                  pid  : 12; // Process ID associated with this command queue. Default is to assign access to the kernel (reserved PID 0). Commands must use this PID as their IPID unless the CQ is privileged. See note in CSR description regarding the symmetrical pairing requirements between this and the Rx.
        uint64_t           priv_level  :  1; // Supervisor=0, User=1. Supervisor privilege level is required to write the DLID relocation table. See note in CSR description regarding the symmetrical pairing requirements between this and the Rx.
        uint64_t               enable  :  1; // A CQ that is not enabled can be written, but slot full counts will always be zero. See note in CSR description regarding the symmetrical pairing requirements between this and the Rx.
        uint64_t          reserved_14  :  1; // Reserved
        uint64_t            phys_dlid  :  1; // Allow physical DLID.
        uint64_t            dlid_base  : 16; // DLID relocation table base address. This value is left shifted based on the DLID relocation table block granularity. After shifting, it is added to the DLID specified in the command. The sum is used to index the DLID relocation table. Not used for Physical DLIDs.
        uint64_t            sl_enable  : 32; // Service level enable, specifying which service levels commands are allowed to use.
    } field;
    uint64_t val;
} TXCID_CFG_CSR_t;

// TXCID_CFG_PID_MASK desc:
typedef union {
    struct {
        uint64_t             pid_mask  : 12; // Process ID mask associated with this command queue. Default is to allow only a single Process ID for the command queue. See note in CSR description regarding the symmetrical pairing requirements between this and the Rx.
        uint64_t       Reserved_63_12  : 52; // Unused
    } field;
    uint64_t val;
} TXCID_CFG_PID_MASK_t;

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

// TXCID_CFG_MAX_CONTEXTS desc:
typedef union {
    struct {
        uint64_t                count  :  8; // Number of command queues
        uint64_t          unused_63_8  : 56; // Unused
    } field;
    uint64_t val;
} TXCID_CFG_MAX_CONTEXTS_t;

// TXCID_CFG_SENDBTHQP desc:
typedef union {
    struct {
        uint64_t            SendBthQP  :  8; // Compared against BTH.DestQP[23:16] to determine KDETH packets
        uint64_t      Disallowedkdeth  :  1; // Disallowed sending of KDETH packets
        uint64_t          unused_63_9  : 55; // Unused
    } field;
    uint64_t val;
} TXCID_CFG_SENDBTHQP_t;

// TXCID_CFG_STALL_OTR_CREDITS_W desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  4; // 3'd15 - Any MC 3'd14 - Any MC1p 3'd13 - Any MC1 3'd12 - Any MC0 3'd11 - MC1pTC3 3'd10 - MC1pTC2 3'd9 - MC1pTC1 3'd8 - MC1pTC0 3'd7 - MC1TC3 3'd6 - MC1TC2 3'd5 - MC1TC1 3'd4 - MC1TC0 3'd3 - MC0TC3 3'd2 - MC0TC2 3'd1 - MC0TC1 3'd0 - MC0TC0
        uint64_t          UNUSED_63_4  : 60; // Unused
    } field;
    uint64_t val;
} TXCID_CFG_STALL_OTR_CREDITS_W_t;

// TXCID_CFG_STALL_OTR_CREDITS_X desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  4; // 3'd15 - Any MC 3'd14 - Any MC1p 3'd13 - Any MC1 3'd12 - Any MC0 3'd11 - MC1pTC3 3'd10 - MC1pTC2 3'd9 - MC1pTC1 3'd8 - MC1pTC0 3'd7 - MC1TC3 3'd6 - MC1TC2 3'd5 - MC1TC1 3'd4 - MC1TC0 3'd3 - MC0TC3 3'd2 - MC0TC2 3'd1 - MC0TC1 3'd0 - MC0TC0
        uint64_t          UNUSED_63_4  : 60; // Unused
    } field;
    uint64_t val;
} TXCID_CFG_STALL_OTR_CREDITS_X_t;

// TXCID_CFG_STALL_OTR_CREDITS_Y desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  4; // 3'd15 - Any MC 3'd14 - Any MC1p 3'd13 - Any MC1 3'd12 - Any MC0 3'd11 - MC1pTC3 3'd10 - MC1pTC2 3'd9 - MC1pTC1 3'd8 - MC1pTC0 3'd7 - MC1TC3 3'd6 - MC1TC2 3'd5 - MC1TC1 3'd4 - MC1TC0 3'd3 - MC0TC3 3'd2 - MC0TC2 3'd1 - MC0TC1 3'd0 - MC0TC0
        uint64_t          UNUSED_63_4  : 60; // Unused
    } field;
    uint64_t val;
} TXCID_CFG_STALL_OTR_CREDITS_Y_t;

// TXCID_CFG_STALL_OTR_CREDITS_Z desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  4; // 3'd15 - Any MC 3'd14 - Any MC1p 3'd13 - Any MC1 3'd12 - Any MC0 3'd11 - MC1pTC3 3'd10 - MC1pTC2 3'd9 - MC1pTC1 3'd8 - MC1pTC0 3'd7 - MC1TC3 3'd6 - MC1TC2 3'd5 - MC1TC1 3'd4 - MC1TC0 3'd3 - MC0TC3 3'd2 - MC0TC2 3'd1 - MC0TC1 3'd0 - MC0TC0
        uint64_t          UNUSED_63_4  : 60; // Unused
    } field;
    uint64_t val;
} TXCID_CFG_STALL_OTR_CREDITS_Z_t;

// TXCID_CFG_RC_REMAP desc:
typedef union {
    struct {
        uint64_t          remap_val_0  :  3; // Remap_value_for RC = 0 in command
        uint64_t                rsv_0  :  1; // Reserved
        uint64_t          remap_val_1  :  3; // Remap_value_for RC = 1 in command
        uint64_t                rsv_1  :  1; // Reserved
        uint64_t          remap_val_2  :  3; // Remap_value_for RC = 2 in command
        uint64_t                rsv_2  :  1; // Reserved
        uint64_t          remap_val_3  :  3; // Remap_value_for RC = 3 in command
        uint64_t                rsv_3  :  1; // Reserved
        uint64_t          remap_val_4  :  3; // Remap_value_for RC = 4 in command
        uint64_t                rsv_4  :  1; // Reserved
        uint64_t          remap_val_5  :  3; // Remap_value_for RC = 5 in command
        uint64_t                rsv_5  :  1; // Reserved
        uint64_t          remap_val_6  :  3; // Remap_value_for RC = 6 in command
        uint64_t                rsv_6  :  1; // Reserved
        uint64_t          remap_val_7  :  3; // Remap_value_for RC = 7 in command
        uint64_t                  rsv  : 33; // Reserved
    } field;
    uint64_t val;
} TXCID_CFG_RC_REMAP_t;

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
        uint64_t                write  :  1; // 0=Read, 1=Write
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
        uint64_t         RPLC_DISABLE  :  1; // Replacement disable
        uint64_t       Reserved_63_60  :  4; // Reserved
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
        uint64_t    dlid_perm_vio_err  :  1; // DLID violation error ACTION: Error reported CQ_NUM reported, packet failed ERR_CATEGORY_TRANSACTION
        uint64_t   kdeth_perm_vio_err  :  1; // KDETH violation error kdeth permission violation error ACTION: Error reported CQ_NUM reported, packet failed ERR_CATEGORY_TRANSACTION
        uint64_t     laddr_too_hi_err  :  1; // Address too high error ACTION: Error reported CQ_NUM reported, packet failed ERR_CATEGORY_TRANSACTION
        uint64_t        blk_ovflw_err  :  1; // Block over flow error ACTION: Error reported CQ_NUM reported, packet failed ERR_CATEGORY_TRANSACTION
        uint64_t   csr_dlid_table_err  :  1; // CSR DLID table error, when config read is performed ACTION: Error reported syndrome reported ERR_CATEGORY_TRANSACTION
        uint64_t dlid_csr_cfg_mbe_err  :  1; // DLID CSR CFG MBE error ACTION: Error reported syndrome reported ERR_CATEGORY_TRANSACTION
        uint64_t dlid_csr_cfg_sbe_err  :  1; // DLID CSR CFG SBE error ACTION: Error reported syndrome reported ERR_CATEGORY_CORRECTABLE
        uint64_t  dlid_table_size_err  :  1; // DLID table size error ACTION: Error reported CQ_NUM reported, packet failed ERR_CATEGORY_TRANSACTION
        uint64_t      laddr_ovflw_err  :  1; // Address overflow error ACTION: Error reported CQ_NUM reported, packet failed ERR_CATEGORY_TRANSACTION
        uint64_t        rpl_ovflw_err  :  1; // Response overflow error ACTION: Error reported CQ_NUM reported, packet failed ERR_CATEGORY_TRANSACTION
        uint64_t        table_mbe_err  :  1; // Table MBE error, mbe in the DLID table memory ACTION: Error reported CQ_NUM reported, packet failed ERR_CATEGORY_TRANSACTION
        uint64_t        table_sbe_err  :  1; // Table SBE error, sbe in the DLID table memory ACTION: Error reported CQ_NUM reported ERR_CATEGORY_CORRECTABLE
        uint64_t    txci_head_mbe_err  :  1; // TXCI head MBE error, mbe detected in the head flit of the packet ACTION: Error reported CQ_NUM reported, packet dropped ERR_CATEGORY_HFI
        uint64_t    txci_head_sbe_err  :  1; // TXCI head SBE error, sbe detected in the head flit of the packet ACTION: Error reported CQ_NUM reported ERR_CATEGORY_CORRECTABLE
        uint64_t low_overhead_inconsistent_2nd_pkt_err  :  1; // Low overhead inconsistent 2nd packet error ACTION: Error reported CQ_NUM reported, packet Failed ERR_CATEGORY_TRANSACTION
        uint64_t         auth_mem_mbe  :  1; // AUTH memory MBE error ACTION: Error reported CQ_NUM reported, packet dropped ERR_CATEGORY_TRANSACTION
        uint64_t         auth_mem_sbe  :  1; // AUTH memory SBE error ACTION: Error reported CQ_NUM reported, ERR_CATEGORY_CORRECTABLE
        uint64_t       length_odd_mbe  :  1; // Length Odd memory MBE error ACTION: Error reported CQ_NUM reported, Nothing done to packet, the error will be caught later in the pipe. ERR_CATEGORY_TRANSACTION
        uint64_t       length_odd_sbe  :  1; // Length Odd memory SBE error ACTION: Error reported CQ_NUM reported, ERR_CATEGORY_CORRECTABLE
        uint64_t    cq_cfg_mem_p1_mbe  :  1; // CQ configuration memory P1 MBE error ACTION: Error reported CQ_NUM reported, packet dropped ERR_CATEGORY_TRANSACTION
        uint64_t    cq_cfg_mem_p1_sbe  :  1; // CQ configuration memory P1 SBE error ACTION: Error reported CQ_NUM reported ERR_CATEGORY_CORRECTABLE
        uint64_t mad_pkt_perm_vio_err  :  1; // MAD packet permission violation error ACTION: Error reported CQ_NUM reported, packet failed ERR_CATEGORY_TRANSACTION
        uint64_t     pid_mtch_vio_err  :  1; // PID match failed in non-privilege mode ACTION: Error reported CQ_NUM reported, packet failed ERR_CATEGORY_TRANSACTION
        uint64_t drop_due_to_sl_disable  :  1; // drop packet due to SL disable ACTION: Error reported CQ_NUM reported, packet failed ERR_CATEGORY_TRANSACTION
        uint64_t hifis_hdr_ecc_sbe_err  :  1; // hifis req interface header sbe error ACTION: Error reported, syndrome reported ERR_CATEGORY_CORRECTABLE
        uint64_t hifis_hdr_ecc_mbe_err  :  1; // hifis req interface data valid parity error Action Error reported. Packet dropped. No other information can be reported ERR_CATEGORY_HFI
        uint64_t fxr_hifis_reqrsp_hval_par_err  :  1; // hifis req interface data valid parity error Action Error reported. Packet dropped No other information can be reported ERR_CATEGORY_HFI
        uint64_t fxr_hifis_reqrsp_tval_par_err  :  1; // hifis req interface data valid parity error Action Error reported. Packet dropped No other information can be reported ERR_CATEGORY_HFI
        uint64_t fxr_hifis_reqrsp_dval_par_err  :  1; // hifis req interface data valid parity error Action Error reported. Packet dropped No other information can be reported ERR_CATEGORY_HFI
        uint64_t hifis_rsp_2ds_any_sbe  :  1; // hifis req interface sbe error at 2nd pipe ACTION: Error reported and CQ_NUM is flagged ERR_CATEGORY_CORRECTABLE
        uint64_t hifis_rsp_2ds_any_mbe  :  1; // hifis req interface mbe error at 2nd pipe; this error will drop the write and hence the transaction for the CQ which got dropped will be hang. ACTION: WRITE DROPPED hence the CQ will be hang. Error reported and CQ_NUM is flagged ERR_CATEGORY_PROCESS
        uint64_t        jkey_mtch_err  :  1; // if for the kdeth packet the masked job key extracted out of the auth index mismatches with the packet jkey. ACTIONS: Packet failed, Error flagged, CQ_NUM reported ERR_CATEGORY_TRANSACTION
        uint64_t     outofbound_error  :  1; // In case of the single write occupy 2 credits this error is flagged ACTION: WRITE DROPPED hence the CQ will be hang. Error reported and CQ_NUM is flagged ERR_CATEGORY_PROCESS
        uint64_t write_to_disabled_cq  :  1; // Write happened to disabled cq and dropped ACTION: WRITE DROPPED hence the CQ will be hang error reported and CQ_NUM is flagged ERR_CATEGORY_PROCESS
        uint64_t raw_hifis_rsp_2ds_any_sbe  :  1; // hifis req interface sbe error at 2nd pipe ACTION: Error reported and CQ_NUM is flagged ERR_CATEGORY_CORRECTABLE
        uint64_t raw_hifis_rsp_2ds_any_mbe  :  1; // hifis req interface mbe error at 2nd pipe; this error will drop the write and hence the transaction for the CQ which got dropped will be hang. ACTION: WRITE DROPPED hence the CQ will be hang. Error reported and CQ_NUM is flagged ERR_CATEGORY_PROCESS
        uint64_t       auth_index_err  :  1; // illegal auth index used. the output of auth table is srank and user ID error is flagged when auth.srank == PTL_RANK_ANY & portal packet | auth.srank != PTL_RANK_ANY & psm packet. ACTION: PACKET FAIL, CQ number is flagged in JKEY_MTCH_ERR_CQ_NUM ERROR_CATAGORY_TRANSACTION
        uint64_t          lnh_vio_err  :  1; // in case of the 9b packet LNH = 0 and 1 are illegal. following are legal combination. Now only 9B OFED DMA with GRH can contain GRH and hence come with LNH = 3 '9B OFED DMA' and LNH = 2 '9B OFED DMA with GRH' and LNH = 3 and privileged CQ '9B PSM PIO' and LNH = 2 '9B PSM DMA' and LNH = 2 The cq which encounters this issue is flagged in txcid_err_info_other_error_1.kdeth_perm_vio_err_cq_num ERROR_CATAGORY_TRANSACTION
        uint64_t       Reserved_63_38  : 26; // Reserved
    } field;
    uint64_t val;
} TXCID_ERR_STS_t;

// TXCID_ERR_CLR desc:
typedef union {
    struct {
        uint64_t                value  : 38; // Error clear value
        uint64_t       Reserved_63_38  : 26; // Reserved
    } field;
    uint64_t val;
} TXCID_ERR_CLR_t;

// TXCID_ERR_FRC desc:
typedef union {
    struct {
        uint64_t                value  : 37; // error force value
        uint64_t       Reserved_63_37  : 27; // Reserved
    } field;
    uint64_t val;
} TXCID_ERR_FRC_t;

// TXCID_ERR_EN_HOST desc:
typedef union {
    struct {
        uint64_t                value  : 38; // err en host value
        uint64_t       Reserved_63_38  : 26; // Reserved
    } field;
    uint64_t val;
} TXCID_ERR_EN_HOST_t;

// TXCID_ERR_FIRST_HOST desc:
typedef union {
    struct {
        uint64_t                value  : 37; // err first host
        uint64_t       Reserved_63_37  : 27; // Reserved
    } field;
    uint64_t val;
} TXCID_ERR_FIRST_HOST_t;

// TXCID_ERR_EN_BMC desc:
typedef union {
    struct {
        uint64_t                value  : 38; // err en bmc
        uint64_t       Reserved_63_38  : 26; // Reserved
    } field;
    uint64_t val;
} TXCID_ERR_EN_BMC_t;

// TXCID_ERR_FIRST_BMC desc:
typedef union {
    struct {
        uint64_t                value  : 38; // err_en_first_bmc
        uint64_t       Reserved_63_38  : 26; // Reserved
    } field;
    uint64_t val;
} TXCID_ERR_FIRST_BMC_t;

// TXCID_ERR_EN_QUAR desc:
typedef union {
    struct {
        uint64_t                value  : 38; // err_en_quar
        uint64_t       Reserved_63_38  : 26; // Reserved
    } field;
    uint64_t val;
} TXCID_ERR_EN_QUAR_t;

// TXCID_ERR_FIRST_QUAR desc:
typedef union {
    struct {
        uint64_t                value  : 38; // err_en_first_quar
        uint64_t       Reserved_63_38  : 26; // Reserved
    } field;
    uint64_t val;
} TXCID_ERR_FIRST_QUAR_t;

// TXCID_ERR_INFO_SBE_MBE desc:
typedef union {
    struct {
        uint64_t syndrome_dlid_csr_cfg_sbe  :  8; // First DLID CSR CFG SBE error syndrome
        uint64_t   syndrome_table_sbe  :  8; // First Table SBE error syndrome
        uint64_t syndrome_txci_head_sbe  :  8; // First TXCI Head SBE error syndrome
        uint64_t syndrome_cq_cfg_mem_p1_sbe  :  8; // First CQ Configuration memory SBE error syndrome
        uint64_t syndrome_length_odd_sbe  :  8; // First length Odd memory SBE error syndrome
        uint64_t syndrome_auth_mem_sbe  :  8; // First AUTH memory SBE error syndrome
        uint64_t                  cnt  :  8; // Saturation counter for MBE from all sources
        uint64_t       Reserved_63_56  :  8; // Reserved
    } field;
    uint64_t val;
} TXCID_ERR_INFO_SBE_MBE_t;

// TXCID_ERR_INFO_SBE_MBE1 desc:
typedef union {
    struct {
        uint64_t hifis_header_sbe_syndrome  :  8; // First hifis header syndrome for sbe
        uint64_t hifis_data_sbe_syndrome  :  8; // First hifis data syndrome for sbe (assuming only one sbe will happen) if multiple sbe hapens, lsb sbe syndrome will get logged
        uint64_t hifis_data_mbe_cq_num  :  8; // context mum for hifis data mbe error
        uint64_t raw_hifis_data_sbe_syndrome  :  8; // First hifis data syndrome for sbe (assuming only one sbe will happen) if multiple sbe hapens, lsb sbe syndrome will get logged
        uint64_t       Reserved_63_32  : 32; // Reserved
    } field;
    uint64_t val;
} TXCID_ERR_INFO_SBE_MBE1_t;

// TXCID_ERR_INFO_OTHER_ERROR_1 desc:
typedef union {
    struct {
        uint64_t dlid_perm_vio_err_cq_num  :  8; // Dlid permission violation cq number
        uint64_t kdeth_perm_vio_err_cq_num  :  8; // Kdeth permission violation cq number
        uint64_t laddr_too_high_cq_num  :  8; // cq_num for laddr too _high
        uint64_t blk_ovflw_err_cq_num  :  8; // cq_num for block overflow
        uint64_t       Reserved_55_32  : 24; // Reserved
        uint64_t dlid_table_size_err_cq_num  :  8; // rpl_blk_size not less than granularity (Error number 7)
    } field;
    uint64_t val;
} TXCID_ERR_INFO_OTHER_ERROR_1_t;

// TXCID_ERR_INFO_OTHER_ERROR_2 desc:
typedef union {
    struct {
        uint64_t laddr_ovflw_err_cq_num  :  8; // laddress overflow cq number
        uint64_t rpl_ovflw_err_cq_num  :  8; // replacement overflow
        uint64_t       Reserved_31_16  : 16; // Reserved
        uint64_t txci_head_mbe_err_cq_num  :  8; // txci_head_data_mbe_err cq_num
        uint64_t txci_head_sbe_err_cq_num  :  8; // txci head data sbe error cq_num
        uint64_t low_overhead_inconsistent_2nd_pkt_err_cq_num  :  8; // low_overhead_inconsistent 2nd pkt err cq num
        uint64_t  auth_mem_mbe_cq_num  :  8; // Auth memory cq number
    } field;
    uint64_t val;
} TXCID_ERR_INFO_OTHER_ERROR_2_t;

// TXCID_ERR_INFO_OTHER_ERROR_3 desc:
typedef union {
    struct {
        uint64_t  auth_mem_sbe_cq_num  :  8; // Cq num for auth memory
        uint64_t length_odd_mbe_cq_num  :  8; // lengh odd mbe cq number
        uint64_t length_odd_sbe_cq_num  :  8; // length odd sbe cq number
        uint64_t cq_cfg_mem_p1_mbe_cq_num  :  8; // cq_cfg_read port1 mbe cq number
        uint64_t cq_cfg_mem_p1_sbe_cq_num  :  8; // cq_cfg_read port1 sbe cq number
        uint64_t mad_pkt_perm_vio_err_cq_num  :  8; // Mad packet permission violation cq number
        uint64_t pid_mtch_vio_err_cq_num  :  8; // pid match violation cq number
        uint64_t drop_due_to_sl_disable_cq_num  :  8; // Drop due to sl disable cq number
    } field;
    uint64_t val;
} TXCID_ERR_INFO_OTHER_ERROR_3_t;

// TXCID_ERR_INFO_OTHER_ERROR_4 desc:
typedef union {
    struct {
        uint64_t hifis_rsp_2ds_any_sbe_cq_num  :  8; // hifis mbe cq num
        uint64_t hifis_rsp_2ds_any_mbe_cq_num  :  8; // hifis sbe cq number
        uint64_t jkey_mtch_err_cq_num  :  8; // jkey match cq number
        uint64_t outofbound_err_cq_num  :  8; // out of bound cq number
        uint64_t wirte_to_disabled_cq_num  :  8; // Write dropped whihc was to the disabled cq
        uint64_t raw_hifis_rsp_2ds_any_sbe_cq_num  :  8; // hifis mbe cq num
        uint64_t raw_hifis_rsp_2ds_any_mbe_cq_num  :  8; // hifis sbe cq number
        uint64_t       Reserved_63_56  :  8; // Reserved
    } field;
    uint64_t val;
} TXCID_ERR_INFO_OTHER_ERROR_4_t;

// TXCID_DBG_ERR_INJECT desc:
typedef union {
    struct {
        uint64_t inject_table_rdata_err_mask  :  8; // Error injection mask for Table read data ECC error
        uint64_t inject_table_rdata_err  :  4; // Error injection enable for Table read data ECC error
        uint64_t inject_txci_flit_ecc_err_mask  :  8; // Error injection mask for TXCI flit ECC error
        uint64_t inject_txci_flit_ecc_err  :  4; // Error injection enable for TXCI flit ECC error
        uint64_t inject_dlid_csr_cfg_ecc_err_mask  :  8; // Error injection mask for DLID CSR CFG ECC error
        uint64_t inject_dlid_csr_cfg_ecc_err  :  1; // Error injection enable for DLID CSR CFG ECC error
        uint64_t inject_cq_cfg_mem_p1_ecc_err_mask  :  8; // Error injection mask for CQ configuration memory P1 ECC error
        uint64_t inject_cq_cfg_mem_p1_ecc_err  :  1; // Error injection enable for CQ configuration memory P1 ECC error
        uint64_t inject_length_odd_ecc_err_mask  :  8; // Error injection mask for length odd memory ECC error
        uint64_t inject_length_odd_ecc_err  :  1; // Error injection enable for length odd memory ECC error
        uint64_t inject_auth_mem_ecc_err_mask  :  8; // Error injection mask for AUTH memory ECC error
        uint64_t inject_auth_mem_ecc_err  :  1; // Error injection enable for AUTH memory ECC error
        uint64_t       Reserved_63_60  :  4; // Unused
    } field;
    uint64_t val;
} TXCID_DBG_ERR_INJECT_t;

// TXCID_DBG_ERR_INJECT1 desc:
typedef union {
    struct {
        uint64_t hifis_hdr_errinj_mask  :  8; // hifis header error injection mask
        uint64_t  hifis_hdr_errinj_en  :  1; // hifis header error injection enable
        uint64_t hifis_data_errinj_mask  :  8; // hifis data error injection mask
        uint64_t hifis_data_errinj_en  :  4; // hifis data error injection enable
        uint64_t       Reserved_63_21  : 43; // Unused
    } field;
    uint64_t val;
} TXCID_DBG_ERR_INJECT1_t;

// TXCID_DBG_VISA_MON_TC desc:
typedef union {
    struct {
        uint64_t         visa_mon_tcx  :  2; // First TC to be monitored valid values 0, 1, 2, 3
        uint64_t         visa_mon_tcy  :  2; // Second TC to be monitored valid values 0, 1, 2, 3
        uint64_t        Reserved_63_4  : 60; // Unused
    } field;
    uint64_t val;
} TXCID_DBG_VISA_MON_TC_t;

#endif /* ___FXR_tx_ci_cid_CSRS_H__ */
