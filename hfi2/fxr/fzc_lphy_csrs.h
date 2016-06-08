// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Thu Jun  2 19:11:24 2016
//

#ifndef ___FZC_lphy_CSRS_H__
#define ___FZC_lphy_CSRS_H__

// FZC_LPHY_LCTL desc:
typedef union {
    struct {
        uint32_t                 FRSE  :  1; // Free Running Strobe Enable (FRSE)
        uint32_t                 RCSE  :  1; // RECENTER State Entry (RCSE)
        uint32_t                RSVD0  :  1; // Reserved (RSVD)
        uint32_t                 HAIE  :  1; // Hardware-Autonomous IDLE_L1 Enable (HAIE)
        uint32_t                   SE  :  1; // Scrambler Enable (SE)
        uint32_t                  SSE  :  1; // SPEED State Entry (SSE)
        uint32_t                 LISE  :  1; // LINKINIT State Entry (LISE)
        uint32_t                  LSE  :  1; // LOOPBACK State Entry (LSE)
        uint32_t                  DSE  :  1; // DISABLED State Entry (DSE)
        uint32_t                 LRSE  :  1; // LINKRESET State Entry (LRSE)
        uint32_t               ILTRAD  :  1; // IDLE_L1 To RECENTER Arc Disable (ILTRAD)
        uint32_t                RSVD1  :  1; // Reserved (RSVD)
        uint32_t                   LM  :  1; // Loopback Mode (LM)
        uint32_t                RSVD2  :  3; // Reserved (RSVD)
        uint32_t                 FLME  :  1; // Forced Loopback Mode Enable (FLME)
        uint32_t                FLMSM  :  1; // Forced Loopback Master or Slave Mode (FLMSM)
        uint32_t                 CQNS  :  1; // CENTER.QUIET Next State (CQNS)
        uint32_t                 EWME  :  1; // Eye Width Margining Enable (EWME)
        uint32_t                REWME  :  1; // RECENTER Eye Width Margining Enable (REWME)
        uint32_t                  IQH  :  1; // INIT.QUIET Hold (IQH)
        uint32_t                  REH  :  1; // RESET.EIDLE Hold (REH)
        uint32_t             EWMOL1ED  :  1; // Eye Width Margining on L1 Exit Disable (EWMOL1ED)
        uint32_t                 RRER  :  1; // Remote RECENTER Entry Ready (RRER)
        uint32_t                RSVD3  :  7; // Reserved (RSVD)
    } field;
    uint32_t val;
} FZC_LPHY_LCTL_t;

// FZC_LPHY_PAD desc:
typedef union {
    struct {
        uint32_t                  PAD  :  4; // Post-Amble Duration (PAD)
        uint32_t                 RSVD  : 28; // Reserved (RSVD)
    } field;
    uint32_t val;
} FZC_LPHY_PAD_t;

// FZC_LPHY_TLS desc:
typedef union {
    struct {
        uint32_t                  TLS  :  4; // Target Link Speed (TLS)
        uint32_t                 RSVD  : 28; // Reserved (RSVD)
    } field;
    uint32_t val;
} FZC_LPHY_TLS_t;

// FZC_LPHY_STC desc:
typedef union {
    struct {
        uint32_t                  SMD  : 16; // Sideband Minimum Duration (SMD)
        uint32_t                 RSVD  : 16; // Reserved (RSVD)
    } field;
    uint32_t val;
} FZC_LPHY_STC_t;

// FZC_LPHY_LPMC desc:
typedef union {
    struct {
        uint32_t                DTQEL  :  8; // Dynamic Transmit Quiet Entry Latency (DTQEL)
        uint32_t                 DTQE  :  1; // Dynamic Transmit Quiet Enable (DTQE)
        uint32_t                RSVD0  :  3; // Reserved (RSVD)
        uint32_t               DPPGPB  :  1; // Dynamic PHY Power Gating Policy Bit (DPPGPB)
        uint32_t                RSVD1  :  3; // Reserved (RSVD)
        uint32_t               HAILET  :  8; // Hardware Autonomous IDLE_L1 Entry Timer (HAILET)
        uint32_t                DPPGE  :  1; // Dynamic PHY Power Gate Enable (DPPGE)
        uint32_t                RSVD2  :  2; // Reserved (RSVD)
        uint32_t                LCTGE  :  1; // Link Clock Trunk Gate Enable (LCTGE)
        uint32_t                DSCGE  :  1; // Dynamic Secondary Clock Gate Enable (DSCGE)
        uint32_t                DLCGE  :  1; // Dynamic Link Clock Gate Enable (DLCGE)
        uint32_t                 VSTE  :  1; // Valid/Strobe Tristate Enable (VSTE)
        uint32_t                 DSTE  :  1; // Data/StreamID Tristate Enable (DSTE)
    } field;
    uint32_t val;
} FZC_LPHY_LPMC_t;

// FZC_LPHY_LW desc:
typedef union {
    struct {
        uint32_t                 RXLW  :  8; // Receive Link Width (RXLW)
        uint32_t                RSVD0  :  8; // Reserved (RSVD)
        uint32_t                 TXLW  :  8; // Transmit Link Width (TXLW)
        uint32_t                RSVD1  :  8; // Reserved (RSVD)
    } field;
    uint32_t val;
} FZC_LPHY_LW_t;

// FZC_LPHY_LCFG desc:
typedef union {
    struct {
        uint32_t                 HARB  :  1; // HW Autonomous RECENTER Disable (HARD)
        uint32_t                 HALD  :  1; // HW Autonomous LINKRESET Disable (HALD)
        uint32_t                RSVD0  :  8; // Reserved (RSVD)
        uint32_t                ASLRT  :  2; // ACTIVE State Link Reset Timer (ASLRT)
        uint32_t                RSVD1  : 10; // Reserved (RSVD)
        uint32_t                  LTE  :  1; // Loopback Training Enable (LTE)
        uint32_t               DNELBE  :  1; // Digital Near-End Loopback Enable (DNELBE)
        uint32_t                NESLE  :  1; // Near-End Sideband Loopback Enable (NESLE)
        uint32_t               DFELBE  :  1; // Digital Far-End Loopback Enable (DFELBE)
        uint32_t                  SRL  :  1; // Secure Register Lock (SRL)
        uint32_t                  AME  :  1; // Aging Mitigation Enable (AME)
        uint32_t                   LR  :  1; // Lane Reversal (LR)
        uint32_t                RSVD2  :  3; // Reserved (RSVD)
    } field;
    uint32_t val;
} FZC_LPHY_LCFG_t;

// FZC_LPHY_STS desc:
typedef union {
    struct {
        uint32_t               PLTERR  :  1; // Physical Layer Training Error (PLTERR)
        uint32_t               PLDERR  :  1; // Physical Layer Detected Error (PLDERR)
        uint32_t               LPDERR  :  1; // Logical Phy Detected Error (LPDERR)
        uint32_t            STRMIDERR  :  1; // STREAMID Error (STRMIDERR)
        uint32_t                  SCF  :  1; // Speed Change Failure (SCF)
        uint32_t                  SES  :  1; // Scrambler Enable Status (SES)
        uint32_t            RLSBBPERR  :  1; // RLINK SB Bridge Parity Error (RLSBBPERR)
        uint32_t               HALERR  :  1; // HW Autonomous LINKRESET Error (HALERR):
        uint32_t                 RSVD  :  8; // Reserved (RSVD)
        uint32_t                 LSMS  : 16; // LSM State (LSMS): 
    } field;
    uint32_t val;
} FZC_LPHY_STS_t;

// FZC_LPHY_EIC desc:
typedef union {
    struct {
        uint32_t                  EBL  :  8; // Error Bit Location (EBL)
        uint32_t                  EIT  :  1; // Error Injection Trigger (EIT)
        uint32_t                 RSVD  : 23; // Reserved (RSVD)
    } field;
    uint32_t val;
} FZC_LPHY_EIC_t;

// FZC_LPHY_IOSFSBCS desc:
typedef union {
    struct {
        uint32_t                 SBIC  :  2; // IOSF Sideband ISM Idle Counter (SBIC)
        uint32_t                 SIID  :  2; // IOSF Sideband Interface Idle Counter (SIID)
        uint32_t                RSVD0  :  1; // Reserved (RSVD)
        uint32_t              SEOSCGE  :  1; // Sideband Endpoint Oscillator/Side Dynamic Clock Gating Enable (SEOSCGE)
        uint32_t              SCPTCGE  :  1; // Side Clock Partition/Trunk Clock Gating Enable (SCPTCGE)
        uint32_t                RSVD1  : 25; // Reserved (RSVD)
    } field;
    uint32_t val;
} FZC_LPHY_IOSFSBCS_t;

// FZC_LPHY_CP desc:
typedef union {
    struct {
        uint64_t                V0SAI  : 32; // Valid SAI (V0SAI)
        uint64_t                V1SAI  : 32; // Valid SAI (V1SAI)
    } field;
    uint64_t val;
} FZC_LPHY_CP_t;

// FZC_LPHY_RAC desc:
typedef union {
    struct {
        uint64_t                V0SAI  : 32; // Valid SAI (V0SAI)
        uint64_t                V1SAI  : 32; // Valid SAI (V1SAI)
    } field;
    uint64_t val;
} FZC_LPHY_RAC_t;

// FZC_LPHY_WAC desc:
typedef union {
    struct {
        uint64_t                V0SAI  : 32; // Valid SAI (V0SAI)
        uint64_t                V1SAI  : 32; // Valid SAI (V1SAI)
    } field;
    uint64_t val;
} FZC_LPHY_WAC_t;

// FZC_LPHY_SRV desc:
typedef union {
    struct {
        uint32_t                  LPD  :  1; // LINKINIT PLP Handshake Disable (LPD)
        uint32_t                  FSD  :  1; // Force Scrambling Disable (FSD)
        uint32_t                  FSE  :  1; // Force Scrambling Enable (FSE)
        uint32_t                  RSD  :  1; // RXSTREAMID SECDED Disable (RSD)
        uint32_t            EXLSMSBHD  :  1; // Extended LSM Sideband Handshake Disable (EXLSMSBHD)
        uint32_t                 RSVD  : 27; // Reserved (RSVD)
    } field;
    uint32_t val;
} FZC_LPHY_SRV_t;

// FZC_LPHY_LPIF0CTL desc:
typedef union {
    struct {
        uint32_t                   EN  :  1; // Enable (EN)
        uint32_t                PLPGC  :  3; // Physical Layer Packet Grant Count (PLPGC)
        uint32_t                 DPGC  :  4; // Data Packet Grant Count (DPGC)
        uint32_t                 RSVD  :  8; // Reserved (RSVD)
        uint32_t            VRXSTRMID  : 16; // Valid Receive STREAMID (VRXSTRMID)
    } field;
    uint32_t val;
} FZC_LPHY_LPIF0CTL_t;

// FZC_LPHY_LPIF0STS desc:
typedef union {
    struct {
        uint32_t             STATEREQ  :  4; // State Request (STATEREQ)
        uint32_t             STATESTS  :  4; // State Status (STATESTS)
        uint32_t                 VLSM  :  8; // Virtual LSM State (VLSM)
        uint32_t                 RSVD  : 16; // Reserved (RSVD)
    } field;
    uint32_t val;
} FZC_LPHY_LPIF0STS_t;

#endif /* ___FZC_lphy_CSRS_H__ */