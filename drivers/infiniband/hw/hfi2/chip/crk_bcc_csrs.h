// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Thu Mar 29 15:03:56 2018
//

#ifndef ___CRK_bcc_CSRS_H__
#define ___CRK_bcc_CSRS_H__

// CRKBCC_CFG_SCRATCHPAD desc:
typedef union {
    struct {
        uint64_t                 DATA  : 64; // Scratch Pad Data Field
    } field;
    uint64_t val;
} CRKBCC_CFG_SCRATCHPAD_t;

// CRKBCC_STS_BCC0_FRAME_RX0 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC0_FRAME_RX0_t;

// CRKBCC_STS_BCC0_FRAME_RX1 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC0_FRAME_RX1_t;

// CRKBCC_STS_BCC0_FRAME_RX2 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC0_FRAME_RX2_t;

// CRKBCC_STS_BCC0_FRAME_RX3 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC0_FRAME_RX3_t;

// CRKBCC_STS_BCC0_FRAME_RX4 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC0_FRAME_RX4_t;

// CRKBCC_STS_BCC0_FRAME_RX5 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC0_FRAME_RX5_t;

// CRKBCC_STS_BCC0_FRAME_RX6 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC0_FRAME_RX6_t;

// CRKBCC_STS_BCC0_FRAME_RX7 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC0_FRAME_RX7_t;

// CRKBCC_STS_BCC0_FRAME_RX8 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC0_FRAME_RX8_t;

// CRKBCC_STS_BCC0_FRAME_RX9 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC0_FRAME_RX9_t;

// CRKBCC_STS_BCC0_GEN_STATUS desc:
typedef union {
    struct {
        uint64_t                 SYNC  :  1; // Indicates valid frame sync
        uint64_t            SYNC_FAIL  :  1; // Indicates failure of frame sync algorithm
        uint64_t ERROR_COUNT_COMPLETE  :  1; // Count of high rate bit errors is complete
        uint64_t             Unused_3  :  1; // Unused
        uint64_t            SELF_GUID  :  1; // Incoming GUID == outgoing GUID. Indicates received signal is crosstalk, back channel should be ignored.
        uint64_t        PRBS31_DETECT  :  1; // Indicates detection of an un-modulated PRBS31 bit stream by the BCC receiver
        uint64_t            BIT_ERROR  :  1; // Indicates one or more bits set in register field CRKBCC_STS_ERRORS.ERROR_COUNT
        uint64_t      LOCAL_NODE_TYPE  :  2; // local node type
        uint64_t          Unused_15_9  :  7; // Unused
        uint64_t       TX_FRAME_VALID  :  8; // Status of transmission enables of frames in each of the 8 TX frame buffers. One bit per one TX frame buffer.
        uint64_t         Unused_63_24  : 40; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC0_GEN_STATUS_t;

// CRKBCC_STS_BCC0_ERRORS desc:
typedef union {
    struct {
        uint64_t          ERROR_COUNT  : 32; // Bit error count. Cleared on new enable error count.
        uint64_t    FRAME_ERROR_COUNT  :  8; // Count of invalid frames since reset. Saturates at 255. Cleared on read.
        uint64_t DISCARDED_FRAME_COUNT  :  8; // The number of times that an RX frame was discarded by the BCC RX frame loading process. Not an Cleared on a read by the 8051.
        uint64_t         Unused_63_48  : 16; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC0_ERRORS_t;

// CRKBCC_STS_BCC1_FRAME_RX0 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC1_FRAME_RX0_t;

// CRKBCC_STS_BCC1_FRAME_RX1 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC1_FRAME_RX1_t;

// CRKBCC_STS_BCC1_FRAME_RX2 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC1_FRAME_RX2_t;

// CRKBCC_STS_BCC1_FRAME_RX3 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC1_FRAME_RX3_t;

// CRKBCC_STS_BCC1_FRAME_RX4 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC1_FRAME_RX4_t;

// CRKBCC_STS_BCC1_FRAME_RX5 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC1_FRAME_RX5_t;

// CRKBCC_STS_BCC1_FRAME_RX6 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC1_FRAME_RX6_t;

// CRKBCC_STS_BCC1_FRAME_RX7 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC1_FRAME_RX7_t;

// CRKBCC_STS_BCC1_FRAME_RX8 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC1_FRAME_RX8_t;

// CRKBCC_STS_BCC1_FRAME_RX9 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC1_FRAME_RX9_t;

// CRKBCC_STS_BCC1_GEN_STATUS desc:
typedef union {
    struct {
        uint64_t                 SYNC  :  1; // Indicates valid frame sync
        uint64_t            SYNC_FAIL  :  1; // Indicates failure of frame sync algorithm
        uint64_t ERROR_COUNT_COMPLETE  :  1; // Count of high rate bit errors is complete
        uint64_t             Unused_3  :  1; // Unused
        uint64_t            SELF_GUID  :  1; // Incoming GUID == outgoing GUID. Indicates received signal is crosstalk, back channel should be ignored.
        uint64_t        PRBS31_DETECT  :  1; // Indicates detection of an un-modulated PRBS31 bit stream by the BCC receiver
        uint64_t            BIT_ERROR  :  1; // Indicates one or more bits set in register field CRKBCC_STS_ERRORS.ERROR_COUNT
        uint64_t      LOCAL_NODE_TYPE  :  2; // local node type
        uint64_t          Unused_15_9  :  7; // Unused
        uint64_t       TX_FRAME_VALID  :  8; // Status of transmission enables of frames in each of the 8 TX frame buffers. One bit per one TX frame buffer.
        uint64_t         Unused_63_24  : 40; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC1_GEN_STATUS_t;

// CRKBCC_STS_BCC1_ERRORS desc:
typedef union {
    struct {
        uint64_t          ERROR_COUNT  : 32; // Bit error count. Cleared on new enable error count.
        uint64_t    FRAME_ERROR_COUNT  :  8; // Count of invalid frames since reset. Saturates at 255. Cleared on read.
        uint64_t DISCARDED_FRAME_COUNT  :  8; // The number of times that an RX frame was discarded by the BCC RX frame loading process. Not an Cleared on a read by the 8051.
        uint64_t         Unused_63_48  : 16; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC1_ERRORS_t;

// CRKBCC_STS_BCC2_FRAME_RX0 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC2_FRAME_RX0_t;

// CRKBCC_STS_BCC2_FRAME_RX1 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC2_FRAME_RX1_t;

// CRKBCC_STS_BCC2_FRAME_RX2 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC2_FRAME_RX2_t;

// CRKBCC_STS_BCC2_FRAME_RX3 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC2_FRAME_RX3_t;

// CRKBCC_STS_BCC2_FRAME_RX4 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC2_FRAME_RX4_t;

// CRKBCC_STS_BCC2_FRAME_RX5 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC2_FRAME_RX5_t;

// CRKBCC_STS_BCC2_FRAME_RX6 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC2_FRAME_RX6_t;

// CRKBCC_STS_BCC2_FRAME_RX7 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC2_FRAME_RX7_t;

// CRKBCC_STS_BCC2_FRAME_RX8 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC2_FRAME_RX8_t;

// CRKBCC_STS_BCC2_FRAME_RX9 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC2_FRAME_RX9_t;

// CRKBCC_STS_BCC2_GEN_STATUS desc:
typedef union {
    struct {
        uint64_t                 SYNC  :  1; // Indicates valid frame sync
        uint64_t            SYNC_FAIL  :  1; // Indicates failure of frame sync algorithm
        uint64_t ERROR_COUNT_COMPLETE  :  1; // Count of high rate bit errors is complete
        uint64_t             Unused_3  :  1; // Unused
        uint64_t            SELF_GUID  :  1; // Incoming GUID == outgoing GUID. Indicates received signal is crosstalk, back channel should be ignored.
        uint64_t        PRBS31_DETECT  :  1; // Indicates detection of an un-modulated PRBS31 bit stream by the BCC receiver
        uint64_t            BIT_ERROR  :  1; // Indicates one or more bits set in register field CRKBCC_STS_ERRORS.ERROR_COUNT
        uint64_t      LOCAL_NODE_TYPE  :  2; // local node type
        uint64_t          Unused_15_9  :  7; // Unused
        uint64_t       TX_FRAME_VALID  :  8; // Status of transmission enables of frames in each of the 8 TX frame buffers. One bit per one TX frame buffer
        uint64_t         Unused_63_24  : 40; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC2_GEN_STATUS_t;

// CRKBCC_STS_BCC2_ERRORS desc:
typedef union {
    struct {
        uint64_t          ERROR_COUNT  : 32; // Bit error count. Cleared on new enable error count.
        uint64_t    FRAME_ERROR_COUNT  :  8; // Count of invalid frames since reset. Saturates at 255. Cleared on read.
        uint64_t DISCARDED_FRAME_COUNT  :  8; // The number of times that an RX frame was discarded by the BCC RX frame loading process. Not an Cleared on a read by the 8051.
        uint64_t         Unused_63_48  : 16; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC2_ERRORS_t;

// CRKBCC_STS_BCC3_FRAME_RX0 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC3_FRAME_RX0_t;

// CRKBCC_STS_BCC3_FRAME_RX1 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC3_FRAME_RX1_t;

// CRKBCC_STS_BCC3_FRAME_RX2 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC3_FRAME_RX2_t;

// CRKBCC_STS_BCC3_FRAME_RX3 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC3_FRAME_RX3_t;

// CRKBCC_STS_BCC3_FRAME_RX4 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC3_FRAME_RX4_t;

// CRKBCC_STS_BCC3_FRAME_RX5 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC3_FRAME_RX5_t;

// CRKBCC_STS_BCC3_FRAME_RX6 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC3_FRAME_RX6_t;

// CRKBCC_STS_BCC3_FRAME_RX7 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC3_FRAME_RX7_t;

// CRKBCC_STS_BCC3_FRAME_RX8 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC3_FRAME_RX8_t;

// CRKBCC_STS_BCC3_FRAME_RX9 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Frame Data
        uint64_t                  TYP  :  8; // Frame Type
        uint64_t                COUNT  :  8; // Up/Dn Counter Value
        uint64_t                  ARM  :  1; // ARM to accept new frame data
        uint64_t         Unused_63_49  : 15; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC3_FRAME_RX9_t;

// CRKBCC_STS_BCC3_GEN_STATUS desc:
typedef union {
    struct {
        uint64_t                 SYNC  :  1; // Indicates valid frame sync
        uint64_t            SYNC_FAIL  :  1; // Indicates failure of frame sync algorithm
        uint64_t ERROR_COUNT_COMPLETE  :  1; // Count of high rate bit errors is complete
        uint64_t             Unused_3  :  1; // Unused
        uint64_t            SELF_GUID  :  1; // Incoming GUID == outgoing GUID. Indicates received signal is crosstalk, back channel should be ignored.
        uint64_t        PRBS31_DETECT  :  1; // Indicates detection of an un-modulated PRBS31 bit stream by the BCC receiver
        uint64_t            BIT_ERROR  :  1; // Indicates one or more bits set in register field CRKBCC_STS_ERRORS.ERROR_COUNT
        uint64_t      LOCAL_NODE_TYPE  :  2; // local node type
        uint64_t          Unused_15_9  :  7; // Unused
        uint64_t       TX_FRAME_VALID  :  8; // Status of transmission enables of frames in each of the 8 TX frame buffers. One bit per one TX frame buffer
        uint64_t         Unused_63_24  : 40; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC3_GEN_STATUS_t;

// CRKBCC_STS_BCC3_ERRORS desc:
typedef union {
    struct {
        uint64_t          ERROR_COUNT  : 32; // Bit error count. Cleared on new enable error count.
        uint64_t    FRAME_ERROR_COUNT  :  8; // Count of invalid frames since reset. Saturates at 255. Cleared on read.
        uint64_t DISCARDED_FRAME_COUNT  :  8; // The number of times that an RX frame was discarded by the BCC RX frame loading process. Not an Cleared on a read by the 8051.
        uint64_t         Unused_63_48  : 16; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC3_ERRORS_t;

// CRKBCC_CFG_BCC0_FRAMES desc:
typedef union {
    struct {
        uint64_t               TYPE_0  :  8; // First frame type
        uint64_t               TYPE_1  :  8; // Second frame type
        uint64_t               TYPE_2  :  8; // Third frame type
        uint64_t               TYPE_3  :  8; // Fourth frame type
        uint64_t         Unused_63_32  : 32; // unused
    } field;
    uint64_t val;
} CRKBCC_CFG_BCC0_FRAMES_t;

// CRKBCC_CFG_BCC1_FRAMES desc:
typedef union {
    struct {
        uint64_t               TYPE_0  :  8; // First frame type
        uint64_t               TYPE_1  :  8; // Second frame type
        uint64_t               TYPE_2  :  8; // Third frame type
        uint64_t               TYPE_3  :  8; // Fourth frame type
        uint64_t         Unused_63_32  : 32; // unused
    } field;
    uint64_t val;
} CRKBCC_CFG_BCC1_FRAMES_t;

// CRKBCC_CFG_BCC2_FRAMES desc:
typedef union {
    struct {
        uint64_t               TYPE_0  :  8; // First frame type
        uint64_t               TYPE_1  :  8; // Second frame type
        uint64_t               TYPE_2  :  8; // Third frame type
        uint64_t               TYPE_3  :  8; // Fourth frame type
        uint64_t         Unused_63_32  : 32; // unused
    } field;
    uint64_t val;
} CRKBCC_CFG_BCC2_FRAMES_t;

// CRKBCC_CFG_BCC3_FRAMES desc:
typedef union {
    struct {
        uint64_t               TYPE_0  :  8; // First frame type
        uint64_t               TYPE_1  :  8; // Second frame type
        uint64_t               TYPE_2  :  8; // Third frame type
        uint64_t               TYPE_3  :  8; // Fourth frame type
        uint64_t         Unused_63_32  : 32; // unused
    } field;
    uint64_t val;
} CRKBCC_CFG_BCC3_FRAMES_t;

// CRKBCC_STS_BCC0_FRAME_0 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Payload Data
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC0_FRAME_0_t;

// CRKBCC_STS_BCC0_FRAME_1 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Payload Data
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC0_FRAME_1_t;

// CRKBCC_STS_BCC0_FRAME_2 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Payload Data
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC0_FRAME_2_t;

// CRKBCC_STS_BCC0_FRAME_3 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Payload Data
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC0_FRAME_3_t;

// CRKBCC_STS_BCC1_FRAME_0 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Payload Data
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC1_FRAME_0_t;

// CRKBCC_STS_BCC1_FRAME_1 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Payload Data
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC1_FRAME_1_t;

// CRKBCC_STS_BCC1_FRAME_2 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Payload Data
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC1_FRAME_2_t;

// CRKBCC_STS_BCC1_FRAME_3 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Payload Data
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC1_FRAME_3_t;

// CRKBCC_STS_BCC2_FRAME_0 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Payload Data
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC2_FRAME_0_t;

// CRKBCC_STS_BCC2_FRAME_1 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Payload Data
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC2_FRAME_1_t;

// CRKBCC_STS_BCC2_FRAME_2 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Payload Data
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC2_FRAME_2_t;

// CRKBCC_STS_BCC2_FRAME_3 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Payload Data
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC2_FRAME_3_t;

// CRKBCC_STS_BCC3_FRAME_0 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Payload Data
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC3_FRAME_0_t;

// CRKBCC_STS_BCC3_FRAME_1 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Payload Data
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC3_FRAME_1_t;

// CRKBCC_STS_BCC3_FRAME_2 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Payload Data
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC3_FRAME_2_t;

// CRKBCC_STS_BCC3_FRAME_3 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 32; // Payload Data
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} CRKBCC_STS_BCC3_FRAME_3_t;

#endif /* ___CRK_bcc_CSRS_H__ */