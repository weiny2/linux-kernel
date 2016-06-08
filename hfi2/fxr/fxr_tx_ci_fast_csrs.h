// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Thu Jun  2 19:11:24 2016
//

#ifndef ___FXR_tx_ci_fast_CSRS_H__
#define ___FXR_tx_ci_fast_CSRS_H__

// TX_CQ_ENTRY desc:
typedef union {
    struct {
        uint64_t          TX_CQ_ENTRY  : 64; // Entry in the transmit command queue.
    } field;
    uint64_t val;
} TX_CQ_ENTRY_t;

#endif /* ___FXR_tx_ci_fast_CSRS_H__ */