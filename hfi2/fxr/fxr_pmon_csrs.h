// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Wed Jun 22 19:30:15 2016
//

#ifndef ___FXR_pmon_CSRS_H__
#define ___FXR_pmon_CSRS_H__

// PMON_CFG_CONTROL desc:
typedef union {
    struct {
        uint64_t               FREEZE  : 16; // Freeze counters. Current time snapshot is captured on the cycle that counters are frozen. This bit does not self-reset. By default, the counters are enabled and free-running. When any bit goes from 0 to 1, PMON_CFG_FREEZE_TIME will be updated. When any bit goes from 1 to 0, PMON_CFG_ENABLE_TIME will be updated.
        uint64_t         LOG_OVERFLOW  : 16; // When set, this causes the uppermost (49th) bit of each count in the group to act as an overflow bit, i.e. once set it stays set until clear or write. Otherwise, the 49th bit always counts.
        uint64_t           RESET_CTRS  : 16; // Resets all counters (automatically cleared when done).
        uint64_t       Reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} PMON_CFG_CONTROL_t;

// PMON_CFG_FREEZE_TIME desc:
typedef union {
    struct {
        uint64_t          FREEZE_TIME  : 56; // The local time at which the counters were frozen
        uint64_t       Reserved_63_56  :  8; // Reserved
    } field;
    uint64_t val;
} PMON_CFG_FREEZE_TIME_t;

// PMON_CFG_ENABLE_TIME desc:
typedef union {
    struct {
        uint64_t          ENABLE_TIME  : 56; // The local time at which the counters were enabled
        uint64_t       Reserved_63_56  :  8; // Reserved
    } field;
    uint64_t val;
} PMON_CFG_ENABLE_TIME_t;

// PMON_CFG_LOCAL_TIME desc:
typedef union {
    struct {
        uint64_t           local_time  : 56; // The current local time.
        uint64_t       Reserved_63_56  :  8; // Reserved
    } field;
    uint64_t val;
} PMON_CFG_LOCAL_TIME_t;

// PMON_ERR_STS_1 desc:
typedef union {
    struct {
        uint64_t           diagnostic  :  1; // Diagnostic Error Flag
        uint64_t             overflow  : 16; // pmon group overflow. some counter[48] within a group of 32 went from 0 to 1. Error information: Section 28.19.4.10, 'PMON Error Info Overflow'
        uint64_t       Reserved_63_17  : 47; // Reserved
    } field;
    uint64_t val;
} PMON_ERR_STS_1_t;

// PMON_ERR_CLR_1 desc:
typedef union {
    struct {
        uint64_t               events  : 17; // Write 1's to clear corresponding status bits.
        uint64_t       Reserved_63_17  : 47; // Reserved
    } field;
    uint64_t val;
} PMON_ERR_CLR_1_t;

// PMON_ERR_FRC_1 desc:
typedef union {
    struct {
        uint64_t               events  : 17; // Write 1 to set corresponding status bits.
        uint64_t       Reserved_63_17  : 47; // Reserved
    } field;
    uint64_t val;
} PMON_ERR_FRC_1_t;

// PMON_ERR_EN_HOST_1 desc:
typedef union {
    struct {
        uint64_t               events  : 17; // Enables corresponding status bits to generate host interrupt signal.
        uint64_t       Reserved_63_17  : 47; // Reserved
    } field;
    uint64_t val;
} PMON_ERR_EN_HOST_1_t;

// PMON_ERR_FIRST_HOST_1 desc:
typedef union {
    struct {
        uint64_t               events  : 17; // Snapshot of status bits when host interrupt signal transitions from 0 to 1.
        uint64_t       Reserved_63_17  : 47; // Reserved
    } field;
    uint64_t val;
} PMON_ERR_FIRST_HOST_1_t;

// PMON_ERR_EN_BMC_1 desc:
typedef union {
    struct {
        uint64_t               events  : 17; // Enable corresponding status bits to generate BMC interrupt signal.
        uint64_t       Reserved_63_17  : 47; // Reserved
    } field;
    uint64_t val;
} PMON_ERR_EN_BMC_1_t;

// PMON_ERR_FIRST_BMC_1 desc:
typedef union {
    struct {
        uint64_t               events  : 17; // Snapshot of status bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t       Reserved_63_17  : 47; // Reserved
    } field;
    uint64_t val;
} PMON_ERR_FIRST_BMC_1_t;

// PMON_ERR_EN_QUAR_1 desc:
typedef union {
    struct {
        uint64_t               events  : 17; // Enable corresponding status bits to generate quarantine signal.
        uint64_t       Reserved_63_17  : 47; // Reserved
    } field;
    uint64_t val;
} PMON_ERR_EN_QUAR_1_t;

// PMON_ERR_FIRST_QUAR_1 desc:
typedef union {
    struct {
        uint64_t               events  : 17; // Snapshot of status bits when quarantine signal transitions from 0 to 1.
        uint64_t       Reserved_63_17  : 47; // Reserved
    } field;
    uint64_t val;
} PMON_ERR_FIRST_QUAR_1_t;

// PMON_ERR_INFO_OVERFLOW desc:
typedef union {
    struct {
        uint64_t            cntr_addr  :  9; // address of first and least significant counter to set overflow.
        uint64_t        Reserved_63_9  : 55; // Reserved
    } field;
    uint64_t val;
} PMON_ERR_INFO_OVERFLOW_t;

// PMON_CFG_ARRAY desc:
typedef union {
    struct {
        uint64_t                COUNT  : 48; // Performance counter
        uint64_t             OVERFLOW  :  1; // Acts as either one more bit of count or as an overflow bit
        uint64_t       Reserved_63_49  : 15; // 
    } field;
    uint64_t val;
} PMON_CFG_ARRAY_t;

#endif /* ___FXR_pmon_CSRS_H__ */