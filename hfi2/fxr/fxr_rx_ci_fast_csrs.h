// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Thu Jun  2 19:11:24 2016
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