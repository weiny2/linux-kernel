// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Wed Jun 22 19:30:15 2016
//

#ifndef ___FXR_rx_dma_CSRS_H__
#define ___FXR_rx_dma_CSRS_H__

// RXDMA_CFG_BUFFER desc:
typedef union {
    struct {
        uint64_t          depth_limit  :  8; // Limit the depth of the linked lists, along with the depth of the buffer used. Value of 0xFF allows 255 handles.
        uint64_t        Reserved_15_8  :  8; // Unused
        uint64_t        almost_buffer  :  5; // Almost full to full flit setting for MC0 Data Queues. Value is (flits/2). So a value of 0x6 gives an 12 flit buffer. Min is 2, max is 0xC. A single flit packet uses 2 flits of the buffer, so 0x6 allows 6 single flit packets.
        uint64_t       Reserved_63_21  : 43; // Unused
    } field;
    uint64_t val;
} RXDMA_CFG_BUFFER_t;

// RXDMA_CFG_CONTROL desc:
typedef union {
    struct {
        uint64_t              ext_ack  :  1; // Acknowledgment type select. 0=Use Basic Ack, 1=Force Extended Ack
        uint64_t         Reserved_7_1  :  7; // Unused
        uint64_t       ct_event_check  :  1; // If 0, Counting Events trigger when Success count exceeds Threshold. When 1, sum of Failure and Success counts is used.
        uint64_t      ct_int_block_to  :  1; // Block the Triggered Ops from being delivered from the RxDMA if the Counting Event has a valid Interrupt.
        uint64_t      ct_block_all_to  :  1; // Block all Triggered Ops from being delivered from the RxDMA. Even if a Counting Event exceeds the threshold count.
        uint64_t       Reserved_15_11  :  5; // Unused
        uint64_t        so_always_off  :  1; // If 1, Strictly Ordered bit to the Host will always be 0. This take priority over the RDO and TDO bits
        uint64_t         so_always_on  :  1; // If 1, Strictly Ordered bit to the Host will always be 1. This takes priority over the always off bit.
        uint64_t     so_on_all_writes  :  1; // If 1, Strictly Ordered bit to the Host will always be 1 for any write. This takes priority over the always off, TDO and SDO bits.
        uint64_t      so_on_all_reads  :  1; // If 1, Strictly Ordered bit to the Host will always be 1 for any read or atomic. This takes priority over the always off and RDO bit.
        uint64_t                  tdo  :  1; // Total Data Ordering bit. Determines the setting of the SO bit on Ordered write or atomics that are not the last cache line of the packet.
        uint64_t                  rco  :  1; // RC Ordering bit. Determines the setting of the SO bit on Ordered cmds.
        uint64_t                  sdo  :  1; // Spontaneous Data Ordering bit. Determines the setting of the SO bit on Spontaneous Events.
        uint64_t              gen1_do  :  1; // Gen-1 Packet Data Ordering bit. Determines the setting of the SO bit on Gen-1 Put Events.
        uint64_t       Reserved_63_24  : 40; // Unused
    } field;
    uint64_t val;
} RXDMA_CFG_CONTROL_t;

// RXDMA_CFG_CMD_CREDITS desc:
typedef union {
    struct {
        uint64_t          mc0_credits  :  7; // Credits given to the HP's for the MC0 DMA Cmds. There are physically 2 for each TC (5 TC's) and a pool of 32 for a max value of 42.
        uint64_t           Reserved_7  :  1; // Unused
        uint64_t          mc1_credits  :  5; // Credits given to the OTR for the MC1 DMA Cmds. There are physically 2 for each TC and a pool of 8 for a max value of 16.
        uint64_t       Reserved_63_13  : 51; // Unused
    } field;
    uint64_t val;
} RXDMA_CFG_CMD_CREDITS_t;

// RXDMA_CFG_CH_CTRL desc:
typedef union {
    struct {
        uint64_t               ch_put  :  2; // Cache Hints for a put
        uint64_t         ch_put_small  :  2; // Cache Hints for a small (<64B) put
        uint64_t               ch_get  :  2; // Cache Hints for a get (64B only)
        uint64_t         ch_get_small  :  2; // Cache Hints for a small (<64 B) get
        uint64_t                ch_ct  :  2; // Cache Hints for a Counting Event Atomic
        uint64_t            ch_ct_set  :  2; // Cache Hints for a CT Set and Threshold Update
        uint64_t             ch_iovec  :  2; // Cache Hints for a IOVEC put
        uint64_t        ch_trig_queue  :  2; // Cache Hints for a Trigger Op Spill Queue put
        uint64_t                ch_eq  :  2; // Cache Hints for an Event Queue put
        uint64_t       Reserved_63_18  : 46; // Unused
    } field;
    uint64_t val;
} RXDMA_CFG_CH_CTRL_t;

// RXDMA_CFG_BW_SHAPE_B0 desc:
typedef union {
    struct {
        uint64_t             BW_LIMIT  : 16; // Bandwidth limit
        uint64_t        LEAK_FRACTION  :  8; // Fractional portion of leak amount
        uint64_t         LEAK_INTEGER  :  3; // Integral portion of leak amount
        uint64_t         UNUSED_31_27  :  5; // Unused
        uint64_t         METER_CONFIG  :  8; // Meter Configuration
        uint64_t         UNUSED_63_40  : 24; // Unused
    } field;
    uint64_t val;
} RXDMA_CFG_BW_SHAPE_B0_t;

// RXDMA_CFG_BW_SHAPE_B1 desc:
typedef union {
    struct {
        uint64_t             BW_LIMIT  : 16; // Bandwidth limit
        uint64_t        LEAK_FRACTION  :  8; // Fractional portion of leak amount
        uint64_t         LEAK_INTEGER  :  3; // Integral portion of leak amount
        uint64_t         UNUSED_31_27  :  5; // Unused
        uint64_t         METER_CONFIG  :  8; // Meter Configuration
        uint64_t         UNUSED_63_40  : 24; // Unused
    } field;
    uint64_t val;
} RXDMA_CFG_BW_SHAPE_B1_t;

// RXDMA_CFG_BW_SHAPE_B2 desc:
typedef union {
    struct {
        uint64_t             BW_LIMIT  : 16; // Bandwidth limit
        uint64_t        LEAK_FRACTION  :  8; // Fractional portion of leak amount
        uint64_t         LEAK_INTEGER  :  3; // Integral portion of leak amount
        uint64_t         UNUSED_31_27  :  5; // Unused
        uint64_t         METER_CONFIG  :  8; // Meter Configuration
        uint64_t         UNUSED_63_40  : 24; // Unused
    } field;
    uint64_t val;
} RXDMA_CFG_BW_SHAPE_B2_t;

// RXDMA_CFG_BW_SHAPE_B3 desc:
typedef union {
    struct {
        uint64_t             BW_LIMIT  : 16; // Bandwidth limit
        uint64_t        LEAK_FRACTION  :  8; // Fractional portion of leak amount
        uint64_t         LEAK_INTEGER  :  3; // Integral portion of leak amount
        uint64_t         UNUSED_31_27  :  5; // Unused
        uint64_t         METER_CONFIG  :  8; // Meter Configuration
        uint64_t         UNUSED_63_40  : 24; // Unused
    } field;
    uint64_t val;
} RXDMA_CFG_BW_SHAPE_B3_t;

// RXDMA_CFG_BW_SHAPE_B4 desc:
typedef union {
    struct {
        uint64_t             BW_LIMIT  : 16; // Bandwidth limit
        uint64_t        LEAK_FRACTION  :  8; // Fractional portion of leak amount
        uint64_t         LEAK_INTEGER  :  3; // Integral portion of leak amount
        uint64_t         UNUSED_31_27  :  5; // Unused
        uint64_t         METER_CONFIG  :  8; // Meter Configuration
        uint64_t         UNUSED_63_40  : 24; // Unused
    } field;
    uint64_t val;
} RXDMA_CFG_BW_SHAPE_B4_t;

// RXDMA_CFG_BW_SHAPE_B5 desc:
typedef union {
    struct {
        uint64_t             BW_LIMIT  : 16; // Bandwidth limit
        uint64_t        LEAK_FRACTION  :  8; // Fractional portion of leak amount
        uint64_t         LEAK_INTEGER  :  3; // Integral portion of leak amount
        uint64_t         UNUSED_31_27  :  5; // Unused
        uint64_t         METER_CONFIG  :  8; // Meter Configuration
        uint64_t         UNUSED_63_40  : 24; // Unused
    } field;
    uint64_t val;
} RXDMA_CFG_BW_SHAPE_B5_t;

// RXDMA_CFG_BW_SHAPE_B6 desc:
typedef union {
    struct {
        uint64_t             BW_LIMIT  : 16; // Bandwidth limit
        uint64_t        LEAK_FRACTION  :  8; // Fractional portion of leak amount
        uint64_t         LEAK_INTEGER  :  3; // Integral portion of leak amount
        uint64_t         UNUSED_31_27  :  5; // Unused
        uint64_t         METER_CONFIG  :  8; // Meter Configuration
        uint64_t         UNUSED_63_40  : 24; // Unused
    } field;
    uint64_t val;
} RXDMA_CFG_BW_SHAPE_B6_t;

// RXDMA_CFG_BW_SHAPE_B7 desc:
typedef union {
    struct {
        uint64_t             BW_LIMIT  : 16; // Bandwidth limit
        uint64_t        LEAK_FRACTION  :  8; // Fractional portion of leak amount
        uint64_t         LEAK_INTEGER  :  3; // Integral portion of leak amount
        uint64_t         UNUSED_31_27  :  5; // Unused
        uint64_t         METER_CONFIG  :  8; // Meter Configuration
        uint64_t         UNUSED_63_40  : 24; // Unused
    } field;
    uint64_t val;
} RXDMA_CFG_BW_SHAPE_B7_t;

// RXDMA_CFG_TO_BASE_TC0 desc:
typedef union {
    struct {
        uint64_t             cl_bytes  :  6; // Aligned to a cache Line. These Address bits must be 0.
        uint64_t                 addr  : 51; // Base pointer for the triggered op user memory list.
        uint64_t       Reserved_63_57  :  7; // Unused
    } field;
    uint64_t val;
} RXDMA_CFG_TO_BASE_TC0_t;

// RXDMA_CFG_TO_BOUNDS_TC0 desc:
typedef union {
    struct {
        uint64_t             cl_bytes  :  6; // Aligned to a cache Line. These Address bits must be 0.
        uint64_t                 addr  : 51; // Pointer to the end of the triggered op user memory list.
        uint64_t       Reserved_63_57  :  7; // Unused
    } field;
    uint64_t val;
} RXDMA_CFG_TO_BOUNDS_TC0_t;

// RXDMA_CFG_TO_BASE_TC1 desc:
typedef union {
    struct {
        uint64_t             cl_bytes  :  6; // Aligned to a cache Line. These Address bits must be 0.
        uint64_t                 addr  : 51; // Base pointer for the triggered op user memory list.
        uint64_t       Reserved_63_57  :  7; // Unused
    } field;
    uint64_t val;
} RXDMA_CFG_TO_BASE_TC1_t;

// RXDMA_CFG_TO_BOUNDS_TC1 desc:
typedef union {
    struct {
        uint64_t             cl_bytes  :  6; // Aligned to a cache Line. These Address bits must be 0.
        uint64_t                 addr  : 51; // Pointer to the end of the triggered op user memory list.
        uint64_t       Reserved_63_57  :  7; // Unused
    } field;
    uint64_t val;
} RXDMA_CFG_TO_BOUNDS_TC1_t;

// RXDMA_CFG_TO_BASE_TC2 desc:
typedef union {
    struct {
        uint64_t             cl_bytes  :  6; // Aligned to a cache Line. These Address bits must be 0.
        uint64_t                 addr  : 51; // Base pointer for the triggered op user memory list.
        uint64_t       Reserved_63_57  :  7; // Unused
    } field;
    uint64_t val;
} RXDMA_CFG_TO_BASE_TC2_t;

// RXDMA_CFG_TO_BOUNDS_TC2 desc:
typedef union {
    struct {
        uint64_t             cl_bytes  :  6; // Aligned to a cache Line. These Address bits must be 0.
        uint64_t                 addr  : 51; // Pointer to the end of the triggered op user memory list.
        uint64_t       Reserved_63_57  :  7; // Unused
    } field;
    uint64_t val;
} RXDMA_CFG_TO_BOUNDS_TC2_t;

// RXDMA_CFG_TO_BASE_TC3 desc:
typedef union {
    struct {
        uint64_t             cl_bytes  :  6; // Aligned to a cache Line. These Address bits must be 0.
        uint64_t                 addr  : 51; // Base pointer for the triggered op user memory list.
        uint64_t       Reserved_63_57  :  7; // Unused
    } field;
    uint64_t val;
} RXDMA_CFG_TO_BASE_TC3_t;

// RXDMA_CFG_TO_BOUNDS_TC3 desc:
typedef union {
    struct {
        uint64_t             cl_bytes  :  6; // Aligned to a cache Line. These Address bits must be 0.
        uint64_t                 addr  : 51; // Pointer to the end of the triggered op user memory list.
        uint64_t       Reserved_63_57  :  7; // Unused
    } field;
    uint64_t val;
} RXDMA_CFG_TO_BOUNDS_TC3_t;

// RXDMA_CFG_TO_BASE_HP desc:
typedef union {
    struct {
        uint64_t             cl_bytes  :  6; // Aligned to a cache Line. These Address bits must be 0.
        uint64_t                 addr  : 51; // Base pointer for the triggered op user memory list.
        uint64_t       Reserved_63_57  :  7; // Unused
    } field;
    uint64_t val;
} RXDMA_CFG_TO_BASE_HP_t;

// RXDMA_CFG_TO_BOUNDS_HP desc:
typedef union {
    struct {
        uint64_t             cl_bytes  :  6; // Aligned to a cache Line. These Address bits must be 0.
        uint64_t                 addr  : 51; // Pointer to the end of the memory for the triggered op user memory list.
        uint64_t       Reserved_63_57  :  7; // Unused
    } field;
    uint64_t val;
} RXDMA_CFG_TO_BOUNDS_HP_t;

// RXDMA_CFG_PORT_MIRROR desc:
typedef union {
    struct {
        uint64_t           time_stamp  : 32; // Time Stamp for Port Mirroring Header.
        uint64_t       port_mirror_on  :  1; // Turn on if in Port Mirroring Mode.
        uint64_t       Reserved_63_33  : 31; // Unused
    } field;
    uint64_t val;
} RXDMA_CFG_PORT_MIRROR_t;

// RXDMA_CFG_PERFMON_CTRL0 desc:
typedef union {
    struct {
        uint64_t           war_mctc_y  :  4; // Select the MCTC for the RXDMA_STALL_WAR_FULL_Y Event. 0-8 allowed.
        uint64_t           war_mctc_x  :  4; // Select the MCTC for the RXDMA_STALL_WAR_FULL_X Event. 0-8 allowed.
        uint64_t           ack_mctc_y  :  4; // Select the MCTC for the RXDMA_STALL_ACK_FULL_Y Event. 0-8 allowed.
        uint64_t           ack_mctc_x  :  4; // Select the MCTC for the RXDMA_STALL_ACK_FULL_X Event. 0-8 allowed.
        uint64_t           tid_mctc_y  :  4; // Select the MCTC for the RXDMA_STALL_TID_FULL_Y Event. 0-8 allowed.
        uint64_t           tid_mctc_x  :  4; // Select the MCTC for the RXDMA_STALL_TID_FULL_X Event. 0-8 allowed.
        uint64_t            to_mctc_y  :  4; // Select the MCTC for the RXDMA_STALL_TO_CRED_Y Event. 0-4 allowed.
        uint64_t            to_mctc_x  :  4; // Select the MCTC for the RXDMA_STALL_TO_CRED_X Event. 0-4 allowed.
        uint64_t        tx_dma_mctc_y  :  4; // Select the MCTC for the RXDMA_STALL_NO_TXDMA_CRED_Y Events. 4-7 allowed.
        uint64_t        tx_dma_mctc_x  :  4; // Select the MCTC for the RXDMA_STALL_NO_TXDMA_CRED_X Events. 4-7 allowed.
        uint64_t         tx_ci_mctc_y  :  4; // Select the MCTC for the RXDMA_STALL_NO_TXCI_CRED_Y Events. 0-11 allowed.
        uint64_t         tx_ci_mctc_x  :  4; // Select the MCTC for the RXDMA_STALL_NO_TXCI_CRED_X Events. 0-11 allowed.
        uint64_t        no_cmd_mctc_y  :  4; // Select the MCTC for the RXDMA_STALL_NO_CMD_Y Event. 0-8 allowed.
        uint64_t        no_cmd_mctc_x  :  4; // Select the MCTC for the RXDMA_STALL_NO_CMD_Y Event. 0-8 allowed.
        uint64_t       Reserved_63_56  :  8; // Unused
    } field;
    uint64_t val;
} RXDMA_CFG_PERFMON_CTRL0_t;

// RXDMA_CFG_PERFMON_CTRL1 desc:
typedef union {
    struct {
        uint64_t    no_credits_mctc_y  :  4; // Select the MCTC for the RXDMA_STALL_CREDITS_Y and the RXDMA_STALL_EMPTY_EVENTS_Y counts. 0-9 allowed.
        uint64_t    no_credits_mctc_x  :  4; // Select the MCTC for the RXDMA_STALL_CREDITS_X and the RXDMA_STALL_EMPTY_EVENTS_X counts. 0-9 allowed.
        uint64_t      rx_flits_mctc_y  :  4; // Select the MCTC for the RXDMA_RX_FLITS_TCMC_Y Event. 0-8 allowed.
        uint64_t      rx_flits_mctc_x  :  4; // Select the MCTC for the RXDMA_RX_FLITS_TCMC_X Event. 0-8 allowed.
        uint64_t      to_issue_mctc_y  :  4; // Select the MCTC for the RXDMA_TRIG_OP_ISSUE_Y Event. 0-4 allowed.
        uint64_t      to_issue_mctc_x  :  4; // Select the MCTC for the RXDMA_TRIG_OP_ISSUE_X Event. 0-4 allowed.
        uint64_t      to_queue_mctc_y  :  4; // Select the MCTC for the RXDMA_TRIG_OP_QUEUE_Y Event. 0-4 allowed.
        uint64_t      to_queue_mctc_x  :  4; // Select the MCTC for the RXDMA_TRIG_OP_QUEUE_X Event. 0-4 allowed.
        uint64_t       Reserved_63_32  : 32; // Unused
    } field;
    uint64_t val;
} RXDMA_CFG_PERFMON_CTRL1_t;

// RXDMA_STS_HOST_CREDITS desc:
typedef union {
    struct {
        uint64_t         mc0_tc0_cred  :  7; // Credits for MCTC 0
        uint64_t         mc0_tc1_cred  :  7; // Credits for MCTC 1
        uint64_t         mc0_tc2_cred  :  7; // Credits for MCTC 2
        uint64_t         mc0_tc3_cred  :  7; // Credits for MCTC 3
        uint64_t         mc1_tc0_cred  :  7; // Credits for MCTC 4
        uint64_t         mc1_tc1_cred  :  7; // Credits for MCTC 5
        uint64_t         mc1_tc2_cred  :  7; // Credits for MCTC 6
        uint64_t         mc1_tc3_cred  :  7; // Credits for MCTC 7
        uint64_t            rxci_cred  :  7; // Credits for MCTC 8
        uint64_t          Reserved_63  :  1; // Unused
    } field;
    uint64_t val;
} RXDMA_STS_HOST_CREDITS_t;

// RXDMA_STS_CI_ACK_CREDITS desc:
typedef union {
    struct {
        uint64_t      ci_mc0_tc0_cred  :  5; // TxCI Credits for MC0 TC 0
        uint64_t      ci_mc0_tc1_cred  :  5; // TxCI Credits for MC0 TC 1
        uint64_t      ci_mc0_tc2_cred  :  5; // TxCI Credits for MC0 TC 2
        uint64_t      ci_mc0_tc3_cred  :  5; // TxCI Credits for MC0 TC 3
        uint64_t      ci_mc1_tc0_cred  :  5; // TxCI Credits for MC1 TC 0
        uint64_t      ci_mc1_tc1_cred  :  5; // TxCI Credits for MC1 TC 1
        uint64_t      ci_mc1_tc2_cred  :  5; // TxCI Credits for MC1 TC 2
        uint64_t      ci_mc1_tc3_cred  :  5; // TxCI Credits for MC1 TC 3
        uint64_t     ci_mc1p_tc0_cred  :  5; // TxCI Credits for MC1 Prime TC 0
        uint64_t     ci_mc1p_tc1_cred  :  5; // TxCI Credits for MC1 Prime TC 1
        uint64_t     ci_mc1p_tc2_cred  :  5; // TxCI Credits for MC1 Prime TC 2
        uint64_t     ci_mc1p_tc3_cred  :  5; // TxCI Credits for MC1 Prime TC 3
        uint64_t       Reserved_63_60  :  4; // Unused
    } field;
    uint64_t val;
} RXDMA_STS_CI_ACK_CREDITS_t;

// RXDMA_STS_DMA_ACK_CREDITS desc:
typedef union {
    struct {
        uint64_t         dma_tc0_cred  :  5; // TxDMA Credits for MC1 TC 0
        uint64_t         dma_tc1_cred  :  5; // TxDMA Credits for MC1 TC 1
        uint64_t         dma_tc2_cred  :  5; // TxDMA Credits for MC1 TC 2
        uint64_t         dma_tc3_cred  :  5; // TxDMA Credits for MC1 TC 3
        uint64_t       Reserved_63_20  : 44; // Unused
    } field;
    uint64_t val;
} RXDMA_STS_DMA_ACK_CREDITS_t;

// RXDMA_DBG_LINKED_LIST desc:
typedef union {
    struct {
        uint64_t                 data  :  8; // Data to be written, or data read from access
        uint64_t              lparity  :  1; // Parity for data [3:0] being written
        uint64_t              hparity  :  1; // Parity for data [7:4] being written
        uint64_t       Reserved_63_10  : 54; // 
    } field;
    uint64_t val;
} RXDMA_DBG_LINKED_LIST_t;

// RXDMA_DBG_TAIL_LIST desc:
typedef union {
    struct {
        uint64_t                 tail  :  8; // Tail pointer for the head location accessed.
        uint64_t                  ecc  :  5; // ECC for data being written, or parity read from access
        uint64_t       Reserved_63_13  : 51; // 
    } field;
    uint64_t val;
} RXDMA_DBG_TAIL_LIST_t;

// RXDMA_DBG_BUFFER_ADDR desc:
typedef union {
    struct {
        uint64_t              address  : 49; // Address of DMA Buffer location to be accessed
        uint64_t                 mctc  :  3; // Message Class and Traffic Class being accessed {MC,TC}
        uint64_t         payload_regs  :  8; // Constant indicating number of payload registers used for the data.
        uint64_t       Reserved_61_60  :  2; // 
        uint64_t                write  :  1; // 0=Read, 1=Write
        uint64_t                Valid  :  1; // Set by Software to start command, cleared by Hardware when complete
    } field;
    uint64_t val;
} RXDMA_DBG_BUFFER_ADDR_t;

// RXDMA_DBG_BUFFER_DATA0 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // 
    } field;
    uint64_t val;
} RXDMA_DBG_BUFFER_DATA0_t;

// RXDMA_DBG_BUFFER_DATA1 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // 
    } field;
    uint64_t val;
} RXDMA_DBG_BUFFER_DATA1_t;

// RXDMA_DBG_BUFFER_DATA2 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // 
    } field;
    uint64_t val;
} RXDMA_DBG_BUFFER_DATA2_t;

// RXDMA_DBG_BUFFER_DATA3 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // 
    } field;
    uint64_t val;
} RXDMA_DBG_BUFFER_DATA3_t;

// RXDMA_DBG_BUFFER_DATA4 desc:
typedef union {
    struct {
        uint64_t                 Data  : 32; // 
        uint64_t       Reserved_63_32  : 32; // 
    } field;
    uint64_t val;
} RXDMA_DBG_BUFFER_DATA4_t;

// RXDMA_DBG_TID_ACK_CNT desc:
typedef union {
    struct {
        uint64_t         acks_alloced  :  7; // Acks allocated now.
        uint64_t           Reserved_7  :  1; // unused
        uint64_t         tids_alloced  :  8; // TIDs allocated now.
        uint64_t              ack_max  :  8; // Max Ack allocation set.
        uint64_t              tid_max  :  8; // Max TID allocation set.
        uint64_t       Reserved_63_32  : 32; // Reserved
    } field;
    uint64_t val;
} RXDMA_DBG_TID_ACK_CNT_t;

// RXDMA_DBG_MUX_SELECT desc:
typedef union {
    struct {
        uint64_t           tid_select  :  8; // TID number to display in TID_DATA CSR.
        uint64_t           ack_select  :  7; // Ack to display in ACK_DATA CSR
        uint64_t          Reserved_15  :  1; // Reserved
        uint64_t        tid_sel_valid  :  1; // Allow this TID select to update the data CSRs
        uint64_t        ack_sel_valid  :  1; // Allow this Ack select to update the data CSRs
        uint64_t       Reserved_19_18  :  2; // Reserved
        uint64_t          cntx_select  :  4; // Select the context for cntx CSR debug data
        uint64_t       cntx_sel_valid  :  1; // Allow this cntx select to update the data CSRs
        uint64_t       Reserved_63_25  : 39; // Reserved
    } field;
    uint64_t val;
} RXDMA_DBG_MUX_SELECT_t;

// RXDMA_DBG_TID_DATA desc:
typedef union {
    struct {
        uint64_t            tid_valid  :  1; // This TID is Valid.
        uint64_t         tid_all_sent  :  1; // All Commands for this TID have been sent.
        uint64_t         Reserved_3_2  :  2; // Reserved
        uint64_t            tid_error  :  4; // Error Responses for this TID. [3]=AT Error, [2]=Timeout, [1]=Mem Error, [0]=Poison.
        uint64_t              tid_ack  :  7; // Pointer to the ACK for this TID. 0=No Ack
        uint64_t          Reserved_15  :  1; // Reserved
        uint64_t            tid_count  :  8; // Number of outstanding responses needed for this TID
        uint64_t       Reserved_63_24  : 40; // Reserved
    } field;
    uint64_t val;
} RXDMA_DBG_TID_DATA_t;

// RXDMA_DBG_ACK_DATA desc:
typedef union {
    struct {
        uint64_t            ack_valid  :  1; // This Ack is Valid.
        uint64_t             ack_rcvd  :  1; // A complete Ack has been received.
        uint64_t        ack_head_type  :  1; // Head Type of Ack 0=BE, 1=E2E
        uint64_t        ack_tail_type  :  1; // Tail Type of Ack 0=PTL, 1=CTS
        uint64_t             ack_mctc  :  4; // Error Responses for this TID. [3]=AT Error, [2]=Timeout, [1]=Mem Error, [0]=Poison.
        uint64_t              ack_big  :  1; // This is a two cycle Ack
        uint64_t        Reserved_15_9  :  7; // Reserved
        uint64_t            doing_ack  :  1; // We are trying to push an Ack out the TxDMA or TxCI interface.
        uint64_t       doing_fast_ack  :  1; // We are trying to push a fast Ack out the TxDMA or TxCI interface.
        uint64_t      ack_good_credit  :  1; // This MCTC has enough credits for this ack.
        uint64_t          ack_to_txci  :  1; // We are trying to push this Ack out the TxCI interface.
        uint64_t       Reserved_23_20  :  4; // Reserved
        uint64_t              ack_tid  :  8; // TID of the Ack we are trying to send.
        uint64_t           ack_handle  :  7; // Handle of the Ack we are trying to send
        uint64_t       Reserved_63_39  : 25; // Reserved
    } field;
    uint64_t val;
} RXDMA_DBG_ACK_DATA_t;

// RXDMA_DBG_HIARB_DATA0 desc:
typedef union {
    struct {
        uint64_t              cmd_req  :  9; // Contexts requesting HIArb arbitration.
        uint64_t           Reserved_9  :  1; // Reserved
        uint64_t              cmd_sel  : 10; // Selected Context in HIArb. [9]=eq
        uint64_t           ack_handle  :  8; // cmd_muxed_q4: Ack Handle
        uint64_t               req_tc  :  4; // cmd_muxed_q4: MCTC
        uint64_t             addr_low  : 32; // cmd_muxed_q4: Lower 32 bits of virtual address
    } field;
    uint64_t val;
} RXDMA_DBG_HIARB_DATA0_t;

// RXDMA_DBG_HIARB_DATA1 desc:
typedef union {
    struct {
        uint64_t              req_pid  : 12; // cmd_muxed_q4: PID
        uint64_t           req_opcode  :  3; // cmd_muxed_q4: req_opcode
        uint64_t             req_spec  :  1; // cmd_muxed_q4: req_spec
        uint64_t               req_ct  :  1; // cmd_muxed_q4: req_ct
        uint64_t              req_art  :  1; // cmd_muxed_q4: req_art
        uint64_t              req_eop  :  1; // cmd_muxed_q4: req_eop
        uint64_t              req_sop  :  1; // cmd_muxed_q4: req_sop
        uint64_t             req_asop  :  5; // cmd_muxed_q4: req_asop
        uint64_t              req_adt  :  5; // cmd_muxed_q4: req_adt
        uint64_t            req_fetch  :  1; // cmd_muxed_q4: req_fetch
        uint64_t          Reserved_31  :  1; // Reserved
        uint64_t              req_len  : 14; // cmd_muxed_q4: req_len
        uint64_t            req_chint  :  2; // cmd_muxed_q4: req_chint
        uint64_t             req_tmod  :  8; // cmd_muxed_q4: req_tmod
        uint64_t       Reserved_63_56  :  8; // Reserved
    } field;
    uint64_t val;
} RXDMA_DBG_HIARB_DATA1_t;

// RXDMA_DBG_CNTX_CMD_DATA desc:
typedef union {
    struct {
        uint64_t            cmd_empty  :  1; // Context Cmd Empty
        uint64_t            xfer_done  :  1; // Context Cmd xfer done
        uint64_t              ct_done  :  1; // Context Cmd ct done
        uint64_t              eq_done  :  1; // Context Cmd eq_done
        uint64_t             ack_done  :  1; // Context Cmd ack_done
        uint64_t            cmd_valid  :  1; // Context Cmd cmd_valid to HIArb
        uint64_t           needs_data  :  1; // Context Cmd data needed to HIArb
        uint64_t           Reserved_7  :  1; // Reserved
        uint64_t               opcode  :  3; // Context Cmd opcode
        uint64_t          Reserved_11  :  1; // Reserved
        uint64_t                  len  : 14; // Context Cmd len
        uint64_t       Reserved_27_26  :  2; // Reserved
        uint64_t           data_valid  :  1; // Context Cmd data valid
        uint64_t            data_tail  :  1; // Context Cmd data tail
        uint64_t          Reserved_30  :  1; // Reserved
        uint64_t             cmd_hold  :  1; // command hold from the HIArb
        uint64_t                 addr  : 32; // Lower 32 bits of Context Address
    } field;
    uint64_t val;
} RXDMA_DBG_CNTX_CMD_DATA_t;

// RXDMA_DBG_CNTX_FIFO_PKT desc:
typedef union {
    struct {
        uint64_t           pkt_handle  : 10; // FIFO Cmd packet handle
        uint64_t       Reserved_11_10  :  2; // Reserved
        uint64_t                 mctc  :  3; // FIFO Cmd mctc
        uint64_t          Reserved_15  :  1; // Reserved
        uint64_t                  pid  : 12; // FIFO Cmd PID
        uint64_t                   ni  :  2; // FIFO Cmd NI
        uint64_t          Reserved_30  :  1; // Reserved
        uint64_t                  eop  :  1; // FIFO Cmd EOP
        uint64_t             ack_info  :  6; // Ack_info[197:192]
        uint64_t       Reserved_43_38  :  6; // Reserved
        uint64_t                atype  :  1; // Ack_info.atype
        uint64_t                 tail  :  1; // Ack_info.tail
        uint64_t                 head  :  1; // Ack_info.head
        uint64_t                valid  :  1; // Ack_info.valid
        uint64_t       Reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} RXDMA_DBG_CNTX_FIFO_PKT_t;

// RXDMA_DBG_CNTX_FIFO_OP0 desc:
typedef union {
    struct {
        uint64_t                valid  :  1; // Context op_info.valid
        uint64_t                  top  :  3; // Context op_info.top
        uint64_t                  aso  :  5; // Context op_info.aso
        uint64_t                  adt  :  5; // Context op_info.adt
        uint64_t                  art  :  1; // Context op_info.art
        uint64_t                fetch  :  1; // Context op_info.fetch
        uint64_t                iovec  :  1; // Context op_info.iovec
        uint64_t       Reserved_31_17  : 15; // Reserved
        uint64_t                start  : 14; // Context op_info.start
        uint64_t       Reserved_47_46  :  2; // Reserved
        uint64_t                count  : 14; // Context op_info.count
        uint64_t       Reserved_63_62  :  2; // Reserved
    } field;
    uint64_t val;
} RXDMA_DBG_CNTX_FIFO_OP0_t;

// RXDMA_DBG_CNTX_FIFO_OP1 desc:
typedef union {
    struct {
        uint64_t                vaddr  : 57; // Context op_info.vaddr
        uint64_t       Reserved_63_57  :  7; // Reserved
    } field;
    uint64_t val;
} RXDMA_DBG_CNTX_FIFO_OP1_t;

// RXDMA_DBG_CNTX_FIFO_CTEQ desc:
typedef union {
    struct {
        uint64_t                valid  :  1; // Context cteq_info.vaddr
        uint64_t                   ct  :  1; // Context cteq_info.vaddr
        uint64_t         Reserved_3_2  :  2; // Reserved
        uint64_t               handle  : 11; // Context cteq_info.vaddr
        uint64_t          Reserved_15  :  1; // Reserved
        uint64_t           ct_failure  :  1; // Context cteq_info.vaddr
        uint64_t         handle_valid  :  1; // Context cteq_info.vaddr
        uint64_t       Reserved_19_18  :  2; // Reserved
        uint64_t            eq_handle  : 11; // Context cteq_info.vaddr
        uint64_t          Reserved_31  :  1; // Reserved
        uint64_t             ct_count  : 32; // Context cteq_info.vaddr
    } field;
    uint64_t val;
} RXDMA_DBG_CNTX_FIFO_CTEQ_t;

// RXDMA_DBG_CNTX_FIFO_ACK0 desc:
typedef union {
    struct {
        uint64_t             ack_info  : 64; // Context op_info.vaddr
    } field;
    uint64_t val;
} RXDMA_DBG_CNTX_FIFO_ACK0_t;

// RXDMA_DBG_CNTX_FIFO_ACK1 desc:
typedef union {
    struct {
        uint64_t             ack_info  : 64; // Context op_info.vaddr
    } field;
    uint64_t val;
} RXDMA_DBG_CNTX_FIFO_ACK1_t;

// RXDMA_DBG_CNTX_FIFO_ACK2 desc:
typedef union {
    struct {
        uint64_t             ack_info  : 64; // Context op_info.vaddr
    } field;
    uint64_t val;
} RXDMA_DBG_CNTX_FIFO_ACK2_t;

// RXDMA_DBG_WAR_VALID desc:
typedef union {
    struct {
        uint64_t               valids  : 64; // A Mask Showing which WAR buffers are allocated.
    } field;
    uint64_t val;
} RXDMA_DBG_WAR_VALID_t;

// RXDMA_DBG_DQ_FREE_PTR desc:
typedef union {
    struct {
        uint64_t                  dq0  :  8; // Start of the Free List for DQ0
        uint64_t                  dq1  :  8; // Start of the Free List for DQ1
        uint64_t                  dq2  :  8; // Start of the Free List for DQ2
        uint64_t                  dq3  :  8; // Start of the Free List for DQ3
        uint64_t                  dq4  :  8; // Start of the Free List for DQ4
        uint64_t                  dq5  :  8; // Start of the Free List for DQ5
        uint64_t                  dq6  :  8; // Start of the Free List for DQ6
        uint64_t                  dq7  :  8; // Start of the Free List for DQ7
    } field;
    uint64_t val;
} RXDMA_DBG_DQ_FREE_PTR_t;

// RXDMA_DBG_DQ_END_PTR desc:
typedef union {
    struct {
        uint64_t                  dq0  :  8; // End of the Free List for DQ0
        uint64_t                  dq1  :  8; // End of the Free List for DQ1
        uint64_t                  dq2  :  8; // End of the Free List for DQ2
        uint64_t                  dq3  :  8; // End of the Free List for DQ3
        uint64_t                  dq4  :  8; // End of the Free List for DQ4
        uint64_t                  dq5  :  8; // End of the Free List for DQ5
        uint64_t                  dq6  :  8; // End of the Free List for DQ6
        uint64_t                  dq7  :  8; // End of the Free List for DQ7
    } field;
    uint64_t val;
} RXDMA_DBG_DQ_END_PTR_t;

// RXDMA_DBG_DQ_LAST_PKT desc:
typedef union {
    struct {
        uint64_t                  dq0  :  8; // Last Packet Handle for DQ0
        uint64_t                  dq1  :  8; // Last Packet Handle for DQ1
        uint64_t                  dq2  :  8; // Last Packet Handle for DQ2
        uint64_t                  dq3  :  8; // Last Packet Handle for DQ3
        uint64_t                  dq4  :  8; // Last Packet Handle for DQ4
        uint64_t                  dq5  :  8; // Last Packet Handle for DQ5
        uint64_t                  dq6  :  8; // Last Packet Handle for DQ6
        uint64_t                  dq7  :  8; // Last Packet Handle for DQ7
    } field;
    uint64_t val;
} RXDMA_DBG_DQ_LAST_PKT_t;

// RXDMA_DBG_NEXT_HANDLE desc:
typedef union {
    struct {
        uint64_t                  dq0  :  8; // Next Packet Handle for DQ0
        uint64_t                  dq1  :  8; // Next Packet Handle for DQ1
        uint64_t                  dq2  :  8; // Next Packet Handle for DQ2
        uint64_t                  dq3  :  8; // Next Packet Handle for DQ3
        uint64_t                  dq4  :  8; // Next Packet Handle for DQ4
        uint64_t                  dq5  :  8; // Next Packet Handle for DQ5
        uint64_t                  dq6  :  8; // Next Packet Handle for DQ6
        uint64_t                  dq7  :  8; // Next Packet Handle for DQ7
    } field;
    uint64_t val;
} RXDMA_DBG_NEXT_HANDLE_t;

// RXDMA_DBG_PKT_CNT desc:
typedef union {
    struct {
        uint64_t                  dq0  :  8; // Valid Packets in DQ0
        uint64_t                  dq1  :  8; // Valid Packets in DQ1
        uint64_t                  dq2  :  8; // Valid Packets in DQ2
        uint64_t                  dq3  :  8; // Valid Packets in DQ3
        uint64_t                  dq4  :  8; // Valid Packets in DQ4
        uint64_t                  dq5  :  8; // Valid Packets in DQ5
        uint64_t                  dq6  :  8; // Valid Packets in DQ6
        uint64_t                  dq7  :  8; // Valid Packets in DQ7
    } field;
    uint64_t val;
} RXDMA_DBG_PKT_CNT_t;

// RXDMA_DBG_ERR_INJ desc:
typedef union {
    struct {
        uint64_t             inj_mask  :  8; // Error Inject Mask for Valid injection
        uint64_t          inj_dq_tail  :  8; // Inject error into tail pointer for each Data Queue. 5 bit mask only
        uint64_t          inj_hi_data  :  4; // Inject error into HIarb Data Path. [0]=63:0 ... [3]=255:192
        uint64_t        inj_host_data  :  4; // Inject error into data from Host. [0]=63:0 ... [3]=255:192
        uint64_t           inj_to_pkt  :  4; // Inject error into Trigger Op Packet from RxET. [0]=63:0 ... [3]=255:192
        uint64_t          inj_ack_cmd  :  4; // Inject error into Ack cmd from RxHP or OTR. [0]=head[98:0], [1]=head[197:99], [2]=tail[98:0], [3]=tail[198:99]
        uint64_t       inj_hp_pkt_cmd  :  1; // Inject error into pkt_info and cteq_info cmd from RxHP
        uint64_t      inj_otr_pkt_cmd  :  1; // Inject error into pkt_info and cteq_info cmd from OTR
        uint64_t        inj_hp_op_cmd  :  1; // Inject error into op_info cmd from RxHP
        uint64_t       inj_otr_op_cmd  :  1; // Inject error into op_info cmd from OTR
        uint64_t           inj_ack_cq  :  1; // Inject error into Ack CQ number from RxHP. 5 bit mask only
        uint64_t       Reserved_63_37  : 27; // Unused
    } field;
    uint64_t val;
} RXDMA_DBG_ERR_INJ_t;

// RXDMA_ERR_STS_1 desc:
typedef union {
    struct {
        uint64_t         dq_write_err  :  8; // Write error to the RxDMA Data Queues. Tail with no head or Two heads with no tail. One bit for each Data Queue. [0]=DQ0...[7]=DQ7
        uint64_t          dq_read_err  :  9; // Read Error from the RxDMA Data Queues. Tried to Read or Discard a packet with a packet handle of 0xff, count>,16250 or size >16320 or reading to a section of the packet already discarded or reading past the end of a packet. One bit for each Data Queue. [8]=DQ0...[16]=DQ7
        uint64_t     dq_ll_parity_err  :  7; // Parity Error on the Data Queue Linked List array. One bit for each Data Queue. [17]=DQ0...[23]=DQ7
        uint64_t          dq_tail_sbe  :  8; // Correctable Error on the Data Queue Tail array. One bit for each Data Queue. [24]=DQ0...[31]=DQ7
        uint64_t          dq_tail_mbe  :  8; // Uncorrectable Error on the Data Queue Tail array. One bit for each Data Queue. [24]=DQ0...[31]=DQ7
        uint64_t           ha_ecc_sbe  :  4; // Correctable Error out of the Data Queue. One bit for each 8 bytes of the 32 bytes going to the Host Arbiter. Source DQ set in error info register.
        uint64_t           ha_ecc_mbe  :  4; // Uncorrectable Error out of the Data Queue. One bit for each 8 bytes of the 32 bytes going to the Host Arbiter. Source DQ set in error info register.
        uint64_t       Reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} RXDMA_ERR_STS_1_t;

// RXDMA_ERR_CLR_1 desc:
typedef union {
    struct {
        uint64_t            error_clr  : 48; // Clear the Error
        uint64_t       Reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} RXDMA_ERR_CLR_1_t;

// RXDMA_ERR_FRC_1 desc:
typedef union {
    struct {
        uint64_t            force_err  : 48; // Force an error
        uint64_t       Reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} RXDMA_ERR_FRC_1_t;

// RXDMA_ERR_EN_HOST_1 desc:
typedef union {
    struct {
        uint64_t              host_en  : 48; // 
        uint64_t       Reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} RXDMA_ERR_EN_HOST_1_t;

// RXDMA_ERR_FIRST_HOST_1 desc:
typedef union {
    struct {
        uint64_t           first_host  : 48; // 
        uint64_t       Reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} RXDMA_ERR_FIRST_HOST_1_t;

// RXDMA_ERR_EN_BMC_1 desc:
typedef union {
    struct {
        uint64_t               bmc_en  : 48; // 
        uint64_t       Reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} RXDMA_ERR_EN_BMC_1_t;

// RXDMA_ERR_FIRST_BMC_1 desc:
typedef union {
    struct {
        uint64_t            first_bmc  : 48; // 
        uint64_t       Reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} RXDMA_ERR_FIRST_BMC_1_t;

// RXDMA_ERR_EN_QUAR_1 desc:
typedef union {
    struct {
        uint64_t              quar_en  : 48; // 
        uint64_t       Reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} RXDMA_ERR_EN_QUAR_1_t;

// RXDMA_ERR_FIRST_QUAR_1 desc:
typedef union {
    struct {
        uint64_t           first_quar  : 48; // 
        uint64_t       Reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} RXDMA_ERR_FIRST_QUAR_1_t;

// RXDMA_ERR_STS_2 desc:
typedef union {
    struct {
        uint64_t         host_ecc_sbe  :  4; // Correctable Error on data from the Host interface. One bit for each 8 bytes of the 32 bytes going to the Host Arbiter.
        uint64_t         host_ecc_mbe  :  4; // Uncorrectable Error on data from the Host Interface. One bit for each 8 bytes of the 32 bytes going to the Host Arbiter.
        uint64_t         ack_tail_sbe  :  1; // Correctable Error on a read from the TailAck Array
        uint64_t         ack_tail_mbe  :  1; // Uncorrectable Error on a read from the Tail Ack Array
        uint64_t         ack_head_sbe  :  1; // Correctable Error on a read from the Head Ack Array
        uint64_t         ack_head_mbe  :  1; // Uncorrectable Error on a read from the Head Ack Array
        uint64_t               cq_sbe  :  1; // Correctable Error on a read from the CQ Ack Array
        uint64_t               cq_mbe  :  1; // Uncorrectable Error on a read from the CQ Ack Array
        uint64_t       hp_pool_op_sbe  :  1; // Correctable Error on a read from the HP Pool OP Array
        uint64_t       hp_pool_op_mbe  :  1; // Uncorrectable Error on a read from the HP Pool OP Array
        uint64_t     hp_pool_cteq_sbe  :  1; // Correctable Error on a read from the HP Pool CTEQ Array
        uint64_t     hp_pool_cteq_mbe  :  1; // Uncorrectable Error on a read from the HP Pool CTEQ Array
        uint64_t      otr_pool_op_sbe  :  1; // Correctable Error on a read from the OTR Pool OP Array
        uint64_t      otr_pool_op_mbe  :  1; // Uncorrectable Error on a read from the OTR Pool OP Array
        uint64_t    otr_pool_cteq_sbe  :  1; // Correctable Error on a read from the OTR Pool CTEQ Array
        uint64_t    otr_pool_cteq_mbe  :  1; // Uncorrectable Error on a read from the OTR Pool CTEQ Array
        uint64_t           to_pkt_sbe  :  1; // Correctable Error on a Triggered Op from the RxET
        uint64_t           to_pkt_mbe  :  1; // Uncorrectable Error on a Triggered Op from the RxET
        uint64_t      mc0_dropped_cmd  :  1; // RxDMA Command from RxHP was dropped because command pool was full.
        uint64_t      mc1_dropped_cmd  :  1; // RxDMA Command from RxOTR was dropped because command pool was full.
        uint64_t           eq_hdr_mbe  :  1; // The EQ being delivered from the RxET has a header MBE. The EQ was dropped.
        uint64_t       gen1_write_err  :  1; // The Gen1 EQ being delivered from the RxET has a header MBE or the data queue read had an error or the RxHP Gen1 EQ command requested more than 120 bytes.
        uint64_t             to_error  :  1; // The Triggered Op being delivered from the RxET had an Error. This TO was dropped.
        uint64_t      to_queue_wr_err  :  5; // A Write to the Triggered Op Ptr Queue caused an overflow. [3:0]=>TC3-0, [4]=>HP
        uint64_t      to_queue_rd_err  :  5; // A Read to an empty Triggered Op Ptr Queue or one with Base=0. [3:0]=>TC3-0, [4]=>HP
        uint64_t       Reserved_63_39  : 25; // Reserved
    } field;
    uint64_t val;
} RXDMA_ERR_STS_2_t;

// RXDMA_ERR_CLR_2 desc:
typedef union {
    struct {
        uint64_t            error_clr  : 39; // Clear the Error
        uint64_t       Reserved_63_39  : 25; // Reserved
    } field;
    uint64_t val;
} RXDMA_ERR_CLR_2_t;

// RXDMA_ERR_FRC_2 desc:
typedef union {
    struct {
        uint64_t            force_err  : 39; // Force an error
        uint64_t       Reserved_63_39  : 25; // Reserved
    } field;
    uint64_t val;
} RXDMA_ERR_FRC_2_t;

// RXDMA_ERR_EN_HOST_2 desc:
typedef union {
    struct {
        uint64_t              host_en  : 39; // 
        uint64_t       Reserved_63_39  : 25; // Reserved
    } field;
    uint64_t val;
} RXDMA_ERR_EN_HOST_2_t;

// RXDMA_ERR_FIRST_HOST_2 desc:
typedef union {
    struct {
        uint64_t           first_host  : 39; // 
        uint64_t       Reserved_63_39  : 25; // Reserved
    } field;
    uint64_t val;
} RXDMA_ERR_FIRST_HOST_2_t;

// RXDMA_ERR_EN_BMC_2 desc:
typedef union {
    struct {
        uint64_t               bmc_en  : 39; // 
        uint64_t       Reserved_63_39  : 25; // Reserved
    } field;
    uint64_t val;
} RXDMA_ERR_EN_BMC_2_t;

// RXDMA_ERR_FIRST_BMC_2 desc:
typedef union {
    struct {
        uint64_t            first_bmc  : 39; // 
        uint64_t       Reserved_63_39  : 25; // Reserved
    } field;
    uint64_t val;
} RXDMA_ERR_FIRST_BMC_2_t;

// RXDMA_ERR_EN_QUAR_2 desc:
typedef union {
    struct {
        uint64_t              quar_en  : 39; // 
        uint64_t       Reserved_63_39  : 25; // Reserved
    } field;
    uint64_t val;
} RXDMA_ERR_EN_QUAR_2_t;

// RXDMA_ERR_FIRST_QUAR_2 desc:
typedef union {
    struct {
        uint64_t           first_quar  : 39; // 
        uint64_t       Reserved_63_39  : 25; // Reserved
    } field;
    uint64_t val;
} RXDMA_ERR_FIRST_QUAR_2_t;

// RXDMA_ERR_INFO_1 desc:
typedef union {
    struct {
        uint64_t         dq_tail_synd  :  5; // Syndrome of the last MBE or SBE of a DQ Tail Read
        uint64_t         Reserved_7_5  :  3; // Reserved
        uint64_t    read_error_pkt_id  :  8; // Packet ID of the last DQ Read Error seen.
        uint64_t          dq_err_addr  :  8; // The address of the last DQ MBE or ll parity error.
        uint64_t              addr_dq  :  3; // DQ of the address in dq_err_addr
        uint64_t       Reserved_31_27  :  5; // Reserved
        uint64_t           hp_op_synd  :  8; // Syndrome of the last MBE or SBE of an HP Pool OP Read
        uint64_t         hp_cteq_synd  :  8; // Syndrome of the last MBE or SBE of an HP Pool CTEQ Read
        uint64_t          otr_op_synd  :  8; // Syndrome of the last MBE or SBE of an OTR Pool OP Read
        uint64_t        otr_cteq_synd  :  8; // Syndrome of the last MBE or SBE of an OTR Pool CTEQ Read
    } field;
    uint64_t val;
} RXDMA_ERR_INFO_1_t;

// RXDMA_ERR_INFO_2 desc:
typedef union {
    struct {
        uint64_t      ha_ecc_synd_sbe  :  8; // Syndrome of the last SBE to the Host Arb.
        uint64_t      ha_ecc_synd_mbe  :  8; // Syndrome of the last MBE to the Host Arb.
        uint64_t      hi_ecc_synd_sbe  :  8; // Syndrome of the last SBE from the HIArb.
        uint64_t      hi_ecc_synd_mbe  :  8; // Syndrome of the last MBE from the HIArb.
        uint64_t        ack_tail_synd  :  8; // Syndrome of the last MBE or SBE for an Ack Tail Read
        uint64_t        ack_head_synd  :  8; // Syndrome of the last MBE or SBE for an Ack Head Read
        uint64_t              cq_synd  :  5; // Syndrome of the last MBE or SBE for a CQ Read
        uint64_t            ha_ecc_dq  :  3; // On HA MBE or SBE, point to failing DQ
        uint64_t              to_synd  :  8; // Syndrome of the last MBE or SBE for a TO Pkt
    } field;
    uint64_t val;
} RXDMA_ERR_INFO_2_t;

// RXDMA_ERR_INFO_3 desc:
typedef union {
    struct {
        uint64_t           hi_sbe_cnt  : 16; // Count of SBE's for data to the HIArb from the Data Queues
        uint64_t           hi_mbe_cnt  : 16; // Count of MBE's for data to the HIArb from the Data Queues
        uint64_t         host_sbe_cnt  : 16; // Count of SBE's for data from the HIArb
        uint64_t         host_mbe_cnt  : 16; // Count of MBE's for data from the HIArb
    } field;
    uint64_t val;
} RXDMA_ERR_INFO_3_t;

// RXDMA_ERR_INFO_4 desc:
typedef union {
    struct {
        uint64_t    hp_pkt_op_sbe_cnt  : 16; // Count of SBE's for Pkt and Op commands from HP
        uint64_t    hp_pkt_op_mbe_cnt  : 16; // Count of MBE's for Pkt and Op commands from HP
        uint64_t    hp_ack_cq_sbe_cnt  : 16; // Count of SBE's for Ack and CQ commands from HP or OTR
        uint64_t    hp_ack_cq_mbe_cnt  : 16; // Count of MBE's for Ack and CQ commands from HP or OTR
    } field;
    uint64_t val;
} RXDMA_ERR_INFO_4_t;

// RXDMA_ERR_INFO_5 desc:
typedef union {
    struct {
        uint64_t   otr_pkt_op_sbe_cnt  : 16; // Count of SBE's for Pkt and Op commands from OTR
        uint64_t   otr_pkt_op_mbe_cnt  : 16; // Count of MBE's for Pkt and Op commands from OTR
        uint64_t       Reserved_63_32  : 32; // Reserved
    } field;
    uint64_t val;
} RXDMA_ERR_INFO_5_t;

// RXDMA_ERR_INFO_6 desc:
typedef union {
    struct {
        uint64_t      dq_tail_sbe_cnt  : 16; // Count of SBE's for tail array in the Data Queues
        uint64_t      dq_tail_mbe_cnt  : 16; // Count of MBE's for tail array in the Data Queues
        uint64_t           to_sbe_cnt  : 16; // Count of SBE's for the Triggered Ops from RxET.
        uint64_t           to_mbe_cnt  : 16; // Count of MBE's for the Triggered Ops from RxET.
    } field;
    uint64_t val;
} RXDMA_ERR_INFO_6_t;

#endif /* ___FXR_rx_dma_CSRS_H__ */