// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Thu Mar 29 15:03:56 2018
//

#ifndef ___FXR_rx_ci_fast_CSRS_H__
#define ___FXR_rx_ci_fast_CSRS_H__

// RX_CQ_ENTRY desc:
typedef union {
    struct {
        uint64_t          RX_CQ_ENTRY  : 64; // Entry in the receive command queue.
    } field;
    uint64_t val;
} RX_CQ_ENTRY_t;

#endif /* ___FXR_rx_ci_fast_CSRS_H__ */