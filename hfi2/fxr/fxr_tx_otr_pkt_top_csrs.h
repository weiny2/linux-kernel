// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Wed Jun 22 19:30:15 2016
//

#ifndef ___FXR_tx_otr_pkt_top_CSRS_H__
#define ___FXR_tx_otr_pkt_top_CSRS_H__

// TXOTR_PKT_CFG_POSTFRAG_OPB_FIFO_CRDTS desc:
typedef union {
    struct {
        uint64_t               MC0TC0  :  3; // MC0/TC0 Post-fragmentation OPB Input Queue Credits
        uint64_t             UNUSED_3  :  1; // Unused
        uint64_t               MC0TC1  :  3; // MC0/TC1 Post-fragmentation OPB Input Queue Credits
        uint64_t             UNUSED_7  :  1; // Unused
        uint64_t               MC0TC2  :  3; // MC0/TC2 Post-fragmentation OPB Input Queue Credits
        uint64_t            UNUSED_11  :  1; // Unused
        uint64_t               MC0TC3  :  3; // MC0/TC3 Post-fragmentation OPB Input Queue Credits
        uint64_t            UNUSED_15  :  1; // Unused
        uint64_t               MC1TC0  :  3; // MC1/TC0 Post-fragmentation OPB Input Queue Credits
        uint64_t            UNUSED_19  :  1; // Unused
        uint64_t               MC1TC1  :  3; // MC1/TC1 Post-fragmentation OPB Input Queue Credits
        uint64_t            UNUSED_23  :  1; // Unused
        uint64_t               MC1TC2  :  3; // MC1/TC2 Post-fragmentation OPB Input Queue Credits
        uint64_t            UNUSED_27  :  1; // Unused
        uint64_t               MC1TC3  :  3; // MC1/TC3 Post-fragmentation OPB Input Queue Credits
        uint64_t         UNUSED_63_31  : 33; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_POSTFRAG_OPB_FIFO_CRDTS_t;

// TXOTR_PKT_CFG_RETRANSMISSION_FIFO_CRDTS desc:
typedef union {
    struct {
        uint64_t               MC0TC0  :  3; // MC0/TC0 Retransmission OPB Input Queue Credits
        uint64_t             UNUSED_3  :  1; // Unused
        uint64_t               MC0TC1  :  3; // MC0/TC1 Retransmission OPB Input Queue Credits
        uint64_t             UNUSED_7  :  1; // Unused
        uint64_t               MC0TC2  :  3; // MC0/TC2 Retransmission OPB Input Queue Credits
        uint64_t            UNUSED_11  :  1; // Unused
        uint64_t               MC0TC3  :  3; // MC0/TC3 Retransmission OPB Input Queue Credits
        uint64_t            UNUSED_15  :  1; // Unused
        uint64_t               MC1TC0  :  3; // MC1/TC0 Retransmission OPB Input Queue Credits
        uint64_t            UNUSED_19  :  1; // Unused
        uint64_t               MC1TC1  :  3; // MC1/TC1 Retransmission OPB Input Queue Credits
        uint64_t            UNUSED_23  :  1; // Unused
        uint64_t               MC1TC2  :  3; // MC1/TC2 Retransmission OPB Input Queue Credits
        uint64_t            UNUSED_27  :  1; // Unused
        uint64_t               MC1TC3  :  3; // MC1/TC3 Retransmission OPB Input Queue Credits
        uint64_t         UNUSED_63_31  : 33; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_RETRANSMISSION_FIFO_CRDTS_t;

// TXOTR_PKT_CFG_RXE2E_CRDTS desc:
typedef union {
    struct {
        uint64_t                  TC0  :  5; // TC0 Input Queue Credits
        uint64_t           UNUSED_7_5  :  3; // Unused
        uint64_t                  TC1  :  5; // TC1 Input Queue Credits
        uint64_t         UNUSED_15_13  :  3; // Unused
        uint64_t                  TC2  :  5; // TC2 Input Queue Credits
        uint64_t         UNUSED_23_21  :  3; // Unused
        uint64_t                  TC3  :  5; // TC3 Input Queue Credits
        uint64_t         UNUSED_63_29  : 35; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_RXE2E_CRDTS_t;

// TXOTR_PKT_CFG_TXDMA_INT desc:
typedef union {
    struct {
        uint64_t                CRDTS  :  3; // TXDMA to TXOTR Packet Done Credits
        uint64_t          UNUSED_63_3  : 61; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_TXDMA_INT_t;

// TXOTR_PKT_CFG_MEMORY_SCHEDULER desc:
typedef union {
    struct {
        uint64_t      TXCI_RSP_RSPACK  :  5; // TXCI to TXOTR Response Response ACK
        uint64_t           UNUSED_7_5  :  3; // Unused
        uint64_t      TXCI_RSP_REQACK  :  5; // TXCI to TXOTR Response Request ACK
        uint64_t         UNUSED_23_13  : 11; // Unused
        uint64_t     TXDMA_REQ_RSPACK  :  4; // TXDMA to TXOTR Request Response ACK
        uint64_t     TXDMA_REQ_REQACK  :  4; // TXDMA to TXOTR Request Request ACK
        uint64_t            FPE_CHINT  :  2; // Cache hints for Fragmentation Programmable Engine reads from Host Memory.
        uint64_t     FPE_GLOBALFLOWID  :  1; // Global Flow ID hint for Fragmentation Programmable Engine reads from Host Memory.
        uint64_t               FPE_SO  :  1; // Strongly Ordered hint for Fragmentation Programmable Engine reads from Host Memory.
        uint64_t            PSN_CHINT  :  2; // Cache hints for PSN Cache reads and writes to Host Memory.
        uint64_t     PSN_GLOBALFLOWID  :  1; // Global Flow ID hint for PSN Cache reads from Host Memory.
        uint64_t               PSN_SO  :  1; // Strongly Ordered hint for PSN Cache reads from Host Memory.
        uint64_t                  PID  : 12; // This is the PID inserted into Address Translation Requests from the PSN Cache
        uint64_t           PRIV_LEVEL  :  1; // The privilege level inserted into the Address Translation Requests from the PSN Cache. 0 = privileged, 1 = user
        uint64_t         UNUSED_63_53  : 11; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_MEMORY_SCHEDULER_t;

// TXOTR_PKT_CFG_PKTID_1 desc:
typedef union {
    struct {
        uint64_t         RSVD_MC0_TC0  :  8; // The number of MC0/TC0 Reserved Packet Identifiers
        uint64_t         RSVD_MC0_TC1  :  8; // The number of MC0/TC1 Reserved Packet Identifiers
        uint64_t         RSVD_MC0_TC2  :  8; // The number of MC0/TC2 Reserved Packet Identifiers
        uint64_t         RSVD_MC0_TC3  :  8; // The number of MC0/TC3 Reserved Packet Identifiers
        uint64_t         RSVD_MC1_TC0  :  8; // The number of MC1/TC0 Reserved Packet Identifiers
        uint64_t         RSVD_MC1_TC1  :  8; // The number of MC1/TC1 Reserved Packet Identifiers
        uint64_t         RSVD_MC1_TC2  :  8; // The number of MC1/TC2 Reserved Packet Identifiers
        uint64_t         RSVD_MC1_TC3  :  8; // The number of MC1/TC3 Reserved Packet Identifiers
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_PKTID_1_t;

// TXOTR_PKT_CFG_PKTID_2 desc:
typedef union {
    struct {
        uint64_t               SHARED  : 14; // The number of Shared Packet Identifiers
        uint64_t         UNUSED_63_14  : 50; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_PKTID_2_t;

// TXOTR_PKT_CFG_OPB_FIFO_GEN_ARB desc:
typedef union {
    struct {
        uint64_t  retransmit_priority  :  1; // If set, Retransmitted packets get priority over all other packets being transmitted, including Pending ACKs
        uint64_t pending_ack_priority  :  1; // If set, Pending ACK packets get priority over all other packets being transmitted, except for Retransmitted packets.
        uint64_t          UNUSED_63_2  : 62; // TODO
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_GEN_ARB_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC0TC0 desc:
typedef union {
    struct {
        uint64_t             BW_LIMIT  : 16; // Bandwidth limit
        uint64_t        LEAK_FRACTION  :  8; // Fractional portion of leak amount
        uint64_t         LEAK_INTEGER  :  3; // Integral portion of leak amount
        uint64_t         UNUSED_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC0TC0_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC0TC1 desc:
typedef union {
    struct {
        uint64_t             BW_LIMIT  : 16; // Bandwidth limit
        uint64_t        LEAK_FRACTION  :  8; // Fractional portion of leak amount
        uint64_t         LEAK_INTEGER  :  3; // Integral portion of leak amount
        uint64_t         UNUSED_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC0TC1_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC0TC2 desc:
typedef union {
    struct {
        uint64_t             BW_LIMIT  : 16; // Bandwidth limit
        uint64_t        LEAK_FRACTION  :  8; // Fractional portion of leak amount
        uint64_t         LEAK_INTEGER  :  3; // Integral portion of leak amount
        uint64_t         UNUSED_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC0TC2_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC0TC3 desc:
typedef union {
    struct {
        uint64_t             BW_LIMIT  : 16; // Bandwidth limit
        uint64_t        LEAK_FRACTION  :  8; // Fractional portion of leak amount
        uint64_t         LEAK_INTEGER  :  3; // Integral portion of leak amount
        uint64_t         UNUSED_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC0TC3_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC1TC0 desc:
typedef union {
    struct {
        uint64_t             BW_LIMIT  : 16; // Bandwidth limit
        uint64_t        LEAK_FRACTION  :  8; // Fractional portion of leak amount
        uint64_t         LEAK_INTEGER  :  3; // Integral portion of leak amount
        uint64_t         UNUSED_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC1TC0_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC1TC1 desc:
typedef union {
    struct {
        uint64_t             BW_LIMIT  : 16; // Bandwidth limit
        uint64_t        LEAK_FRACTION  :  8; // Fractional portion of leak amount
        uint64_t         LEAK_INTEGER  :  3; // Integral portion of leak amount
        uint64_t         UNUSED_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC1TC1_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC1TC2 desc:
typedef union {
    struct {
        uint64_t             BW_LIMIT  : 16; // Bandwidth limit
        uint64_t        LEAK_FRACTION  :  8; // Fractional portion of leak amount
        uint64_t         LEAK_INTEGER  :  3; // Integral portion of leak amount
        uint64_t         UNUSED_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC1TC2_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC1TC3 desc:
typedef union {
    struct {
        uint64_t             BW_LIMIT  : 16; // Bandwidth limit
        uint64_t        LEAK_FRACTION  :  8; // Fractional portion of leak amount
        uint64_t         LEAK_INTEGER  :  3; // Integral portion of leak amount
        uint64_t         UNUSED_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC1TC3_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC0TC0 desc:
typedef union {
    struct {
        uint64_t             BW_LIMIT  : 16; // Bandwidth limit
        uint64_t        LEAK_FRACTION  :  8; // Fractional portion of leak amount
        uint64_t         LEAK_INTEGER  :  3; // Integral portion of leak amount
        uint64_t         UNUSED_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC0TC0_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC0TC1 desc:
typedef union {
    struct {
        uint64_t             BW_LIMIT  : 16; // Bandwidth limit
        uint64_t        LEAK_FRACTION  :  8; // Fractional portion of leak amount
        uint64_t         LEAK_INTEGER  :  3; // Integral portion of leak amount
        uint64_t         UNUSED_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC0TC1_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC0TC2 desc:
typedef union {
    struct {
        uint64_t             BW_LIMIT  : 16; // Bandwidth limit
        uint64_t        LEAK_FRACTION  :  8; // Fractional portion of leak amount
        uint64_t         LEAK_INTEGER  :  3; // Integral portion of leak amount
        uint64_t         UNUSED_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC0TC2_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC0TC3 desc:
typedef union {
    struct {
        uint64_t             BW_LIMIT  : 16; // Bandwidth limit
        uint64_t        LEAK_FRACTION  :  8; // Fractional portion of leak amount
        uint64_t         LEAK_INTEGER  :  3; // Integral portion of leak amount
        uint64_t         UNUSED_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC0TC3_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC1TC0 desc:
typedef union {
    struct {
        uint64_t             BW_LIMIT  : 16; // Bandwidth limit
        uint64_t        LEAK_FRACTION  :  8; // Fractional portion of leak amount
        uint64_t         LEAK_INTEGER  :  3; // Integral portion of leak amount
        uint64_t         UNUSED_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC1TC0_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC1TC1 desc:
typedef union {
    struct {
        uint64_t             BW_LIMIT  : 16; // Bandwidth limit
        uint64_t        LEAK_FRACTION  :  8; // Fractional portion of leak amount
        uint64_t         LEAK_INTEGER  :  3; // Integral portion of leak amount
        uint64_t         UNUSED_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC1TC1_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC1TC2 desc:
typedef union {
    struct {
        uint64_t             BW_LIMIT  : 16; // Bandwidth limit
        uint64_t        LEAK_FRACTION  :  8; // Fractional portion of leak amount
        uint64_t         LEAK_INTEGER  :  3; // Integral portion of leak amount
        uint64_t         UNUSED_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC1TC2_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC1TC3 desc:
typedef union {
    struct {
        uint64_t             BW_LIMIT  : 16; // Bandwidth limit
        uint64_t        LEAK_FRACTION  :  8; // Fractional portion of leak amount
        uint64_t         LEAK_INTEGER  :  3; // Integral portion of leak amount
        uint64_t         UNUSED_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC1TC3_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_MC0TC0 desc:
typedef union {
    struct {
        uint64_t             BW_LIMIT  : 16; // Bandwidth limit
        uint64_t        LEAK_FRACTION  :  8; // Fractional portion of leak amount
        uint64_t         LEAK_INTEGER  :  3; // Integral portion of leak amount
        uint64_t         UNUSED_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_MC0TC0_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_MC0TC1 desc:
typedef union {
    struct {
        uint64_t             BW_LIMIT  : 16; // Bandwidth limit
        uint64_t        LEAK_FRACTION  :  8; // Fractional portion of leak amount
        uint64_t         LEAK_INTEGER  :  3; // Integral portion of leak amount
        uint64_t         UNUSED_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_MC0TC1_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_MC0TC2 desc:
typedef union {
    struct {
        uint64_t             BW_LIMIT  : 16; // Bandwidth limit
        uint64_t        LEAK_FRACTION  :  8; // Fractional portion of leak amount
        uint64_t         LEAK_INTEGER  :  3; // Integral portion of leak amount
        uint64_t         UNUSED_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_MC0TC2_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_MC0TC3 desc:
typedef union {
    struct {
        uint64_t             BW_LIMIT  : 16; // Bandwidth limit
        uint64_t        LEAK_FRACTION  :  8; // Fractional portion of leak amount
        uint64_t         LEAK_INTEGER  :  3; // Integral portion of leak amount
        uint64_t         UNUSED_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_MC0TC3_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_MC1TC0 desc:
typedef union {
    struct {
        uint64_t             BW_LIMIT  : 16; // Bandwidth limit
        uint64_t        LEAK_FRACTION  :  8; // Fractional portion of leak amount
        uint64_t         LEAK_INTEGER  :  3; // Integral portion of leak amount
        uint64_t         UNUSED_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_MC1TC0_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_MC1TC1 desc:
typedef union {
    struct {
        uint64_t             BW_LIMIT  : 16; // Bandwidth limit
        uint64_t        LEAK_FRACTION  :  8; // Fractional portion of leak amount
        uint64_t         LEAK_INTEGER  :  3; // Integral portion of leak amount
        uint64_t         UNUSED_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_MC1TC1_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_MC1TC2 desc:
typedef union {
    struct {
        uint64_t             BW_LIMIT  : 16; // Bandwidth limit
        uint64_t        LEAK_FRACTION  :  8; // Fractional portion of leak amount
        uint64_t         LEAK_INTEGER  :  3; // Integral portion of leak amount
        uint64_t         UNUSED_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_MC1TC2_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_MC1TC3 desc:
typedef union {
    struct {
        uint64_t             BW_LIMIT  : 16; // Bandwidth limit
        uint64_t        LEAK_FRACTION  :  8; // Fractional portion of leak amount
        uint64_t         LEAK_INTEGER  :  3; // Integral portion of leak amount
        uint64_t         UNUSED_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_MC1TC3_t;

// TXOTR_PKT_CFG_MAX_SEQ_DIST desc:
typedef union {
    struct {
        uint64_t   THRESHOLD_DISTANCE  : 16; // Threshold Distance. Any Maximum Sequence Distance greater than or equal to THRESHOLD_DISTANCE is considered to be a 'large transmit window.' Any Maximum Sequence Distance less than THRESHOLD_DISTANCE is considered to be a 'small transmit window.'
        uint64_t        SKID_DISTANCE  :  8; // Skid Distance. This is the maximum length of the pipeline in OTR between when traffic is stalled due to a Maximum Sequence Distance violation and when the Maximum Sequence Distance violation is checked.
        uint64_t         UNUSED_63_24  : 40; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_MAX_SEQ_DIST_t;

// TXOTR_PKT_CFG_FORCE_RETRANS_PENDING desc:
typedef union {
    struct {
        uint64_t       LOCAL_SEQUENCE  : 64; // Force Local Sequences to be marked as Pending
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_FORCE_RETRANS_PENDING_t;

// TXOTR_PKT_CFG_TIMEOUT desc:
typedef union {
    struct {
        uint64_t               SCALER  : 32; // This register selects the rate at which the TXOTR Timeout Counter increments. If set to 32'd0, the counter is disabled. If set to 1, the counter increments once per clock cycle. If set to 2, the counter increments once every other clock cycle. Et cetera.
        uint64_t          MAX_RETRIES  :  4; // Maximum number of retries allowed for a packet allocated in the Outstanding Packet Buffer. Note that setting this value to 4'd15 disables initial transmits of packets which exceed the Maximum Sequence Distance for a destination since those packets can be stored in the OPB with a max_retry value set to one greater than the configured value of MAX_RETRIES .
        uint64_t              TIMEOUT  : 12; // Timeout comparison value. A timeout has occurred when the difference between the time stamp in a given Hash Table entry and the time current time stamp has exceeded this value.
        uint64_t DISABLE_RETRANS_LIMIT_UNCONNECTED  :  1; // Disable setting a connection state to Unconnected when a retransmit limit is reached.
        uint64_t         UNUSED_63_49  : 15; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_TIMEOUT_t;

// TXOTR_PKT_CFG_SLID_PT0 desc:
typedef union {
    struct {
        uint64_t                 BASE  : 24; // Source Local Identifier Base
        uint64_t         UNUSED_63_24  : 40; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_SLID_PT0_t;

// TXOTR_PKT_CFG_SLID_PT1 desc:
typedef union {
    struct {
        uint64_t                 BASE  : 24; // Source Local Identifier Base. See description in Section 29.7.3.36, 'SLID Port 0 Configuration CSR' .
        uint64_t         UNUSED_63_24  : 40; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_SLID_PT1_t;

// TXOTR_PKT_CFG_LMC desc:
typedef union {
    struct {
        uint64_t               lmc_p0  :  2; // Port 0 number of low SLID bits masked by network for packet delivery.
        uint64_t         Reserved_3_2  :  2; // Unused
        uint64_t               lmc_p1  :  2; // Port 1 number of low SLID bits masked by network for packet delivery.
        uint64_t        Reserved_63_6  : 58; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_LMC_t;

// TXOTR_PKT_CFG_PORT_MODE desc:
typedef union {
    struct {
        uint64_t                 MODE  :  2; // Port Operational Mode. See Section XREF of the Portals Transport Layer Specification for a description of each mode of operation. 2'b00 - Disjoint Mode. 2'b01 - Grouped Mode 2'b10 - Reserved 2'b11 - Reserved
        uint64_t          UNUSED_63_2  : 62; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_PORT_MODE_t;

// TXOTR_PKT_CFG_ADAPIVE_EN desc:
typedef union {
    struct {
        uint64_t        DISABLE_COUNT  :  4; // Disable setting of adaptive_en when the retry count for a packet is less than or equal to this value.
        uint64_t          UNUSED_63_4  : 60; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_ADAPIVE_EN_t;

// TXOTR_PKT_CFG_PORT_FAIL_OVER desc:
typedef union {
    struct {
        uint64_t                COUNT  :  4; // Implement port fail over mode when the retry count for a packet is less than or equal to this value.
        uint64_t          UNUSED_63_4  : 60; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_PORT_FAIL_OVER_t;

// TXOTR_PKT_CFG_FRAG_ENG desc:
typedef union {
    struct {
        uint64_t         MAX_DMA_CMDS  : 16; // The maximum number of DMA Commands that can be generated for a single packet.
        uint64_t     MAX_DMA_PREFETCH  : 16; // The maximum number of DMA Prefetches that can be issued by FPE.
        uint64_t     MSG_ORDERING_OFF  :  1; // If this bit is set, do not check OMB Msg order fields against those of Msg's already in flight before sending it to Fragmentation Engine
        uint64_t       POST_FRAG_PRIO  :  1; // If this bit is set, give priority to flits from the standard Post-fragmentation queues over the Post-fragmentation Retransmission queues.
        uint64_t START_AUTHENTICATION  :  1; // Set this bit to start firmware image authentication. Clear it before writing to the firmware CSR.
        uint64_t AUTHENTICATION_COMPLETE  :  1; // Indicates that authentication completed on the current firmware image.
        uint64_t AUTHENTICATION_SUCCESSFUL  :  1; // Indicates that authentication was successful on the current firmware image.
        uint64_t         UNUSED_63_37  : 27; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_FRAG_ENG_t;

// TXOTR_PKT_CFG_VALID_TC_DLID desc:
typedef union {
    struct {
        uint64_t          max_dlid_p0  : 24; // Port 0 max valid dlid, incoming packets with dlid > than this are dropped and no PSN cache lookup is done. Note: dlid > max_dlid_p0 will abort CACHE_CMD_RD, CACHE_CMD_WR in txotr_pkt_cfg_psn_cache_access_ctl and set bad_addr field in txotr_pkt_cfg_psn_cache_access_ctl
        uint64_t          tc_valid_p0  :  4; // Port 0 TC valid bits, 1 bit per TC, invalid incoming TC packets are dropped and no PSN cache lookup is done. Note: Invalid tc will abort CACHE_CMD_RD, CACHE_CMD_WR in txotr_pkt_cfg_psn_cache_access_ctl and set bad_addr field in txotr_pkt_cfg_psn_cache_access_ctl
        uint64_t          max_dlid_p1  : 24; // Port 1 max valid dlid, PSN Cache requests with dlid > than this are viewed to be unconnected and no PSN cache lookup is done. Note: dlid > max_dlid_p1 will abort CACHE_CMD_RD, CACHE_CMD_WR in txotr_pkt_cfg_psn_cache_access_ctl and set bad_addr field in txotr_pkt_cfg_psn_cache_access_ctl
        uint64_t          tc_valid_p1  :  4; // Port 1 TC valid bits, 1 bit per TC, invalid incoming TC packets are dropped and no PSN cache lookup is done. Note: Invalid tc will abort CACHE_CMD_RD, CACHE_CMD_WR in txotr_pkt_cfg_psn_cache_access_ctl and set bad_addr field in txotr_pkt_cfg_psn_cache_access_ctl
        uint64_t       Reserved_63_56  :  8; // Reserved
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_VALID_TC_DLID_t;

// TXOTR_PKT_CFG_PSN_CACHE_CLIENT_DISABLE desc:
typedef union {
    struct {
        uint64_t client_psn_cache_request_disable  :  1; // Client psn cache request disable.
        uint64_t        Reserved_63_1  : 63; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_PSN_CACHE_CLIENT_DISABLE_t;

// TXOTR_PKT_CFG_PSN_BASE_ADDR_P0_TC desc:
typedef union {
    struct {
        uint64_t        Reserved_11_0  : 12; // Reserved
        uint64_t              address  : 45; // Host Memory Base Address, aligned to page boundary
        uint64_t       Reserved_63_57  :  7; // Reserved
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_PSN_BASE_ADDR_P0_TC_t;

// TXOTR_PKT_CFG_PSN_BASE_ADDR_P1_TC desc:
typedef union {
    struct {
        uint64_t        Reserved_11_0  : 12; // Reserved
        uint64_t              address  : 45; // Host Memory Base Address, aligned to page boundary
        uint64_t       Reserved_63_57  :  7; // Reserved
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_PSN_BASE_ADDR_P1_TC_t;

// TXOTR_PKT_CFG_PSN_CACHE_ACCESS_CTL desc:
typedef union {
    struct {
        uint64_t              address  : 29; // The contents of this field differ based on cmd. psn_cache_addr_t for CACHE_CMD_RD, CACHE_CMD_WR,CACHE_CMD_INVALIDATE,CACHE_CMD_FLUSH* 10:0 used for CACHE_CMD_TAG_RD,CACHE_CMD_TAG_WR 14:0 used for CACHE_CMD_DATA_RD, CACHE_CMD_DATA_WR Note: See <blue text>Section 7.1.2.2.3, 'Packet Sequence Number Table Structures' for how client addresses are formed.
        uint64_t         mask_address  : 29; // cache cmd mask address. 1 bits are don't care. only used for CACHE_CMD_INVALIDATE, CACHE_CMD_FLUSH_INVALID, CACHE_CMD_FLUSH_VALID The form of this field is psn_cache_addr_t. initially all 1's. Note: If this field is 0, a normal single lookup is done. If non-zero, the entire cache tag memory is read and tag entries match/masked.
        uint64_t                  cmd  :  4; // cache cmd. see <blue text>Section A.1.1, 'FXR Cache Cmd enums' initially CACHE_CMD_INVALIDATE
        uint64_t                 busy  :  1; // SW sets busy when writing this csr. HW clears busy when cmd is complete. busy must be clear before writing this csr. If busy is set, HW is busy on a previous cmd. Coming out of reset, busy will be 1 so as to initiate the psn cache tag invalidation. Then in 2k clks or so, it will go to 0.
        uint64_t             bad_addr  :  1; // if cmd == CACHE_CMD_RD or CACHE_CMD_WR, the address.tc is checked for valid and address.lid is checked for in range. See txotr_pkt_cfg_valid_tc_dlid . The port used is address.port anded with use_port_in_psn_state field in txotr_pkt_cfg_valid_tc_dlid . No other cache_cmd sets this bit. If the check fails, no psn cache lookup occurs and this bit gets set.
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_PSN_CACHE_ACCESS_CTL_t;

// TXOTR_PKT_CFG_PSN_CACHE_ACCESS_DATA desc:
typedef union {
    struct {
        uint64_t     next_seq_ordered  : 16; // Next PSN for the ordered packets. format is psn_cache_addr_t
        uint64_t   next_seq_unordered  : 16; // Next PSN for the unordered packets. This includes adaptive routing and deterministic routing that is hashed using any fields other than DLID and SLID. format is psn_cache_addr_t
        uint64_t     oldest_unordered  : 16; // Oldest unordered PSN known to be outstanding. format is psn_cache_addr_t
        uint64_t         max_seq_dist  : 12; // Maximum distance between oldest unordered and next unordered PSNs.
        uint64_t                 ctrl  :  4; // Control state for this destination. format is tx_proto_ctrl_t
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_PSN_CACHE_ACCESS_DATA_t;

// TXOTR_PKT_CFG_PSN_CACHE_ACCESS_DATA_BIT_ENABLE desc:
typedef union {
    struct {
        uint64_t     next_seq_ordered  : 16; // Next PSN for the ordered packets. format is psn_cache_addr_t
        uint64_t   next_seq_unordered  : 16; // Next PSN for the unordered packets. This includes adaptive routing and deterministic routing that is hashed using any fields other than DLID and SLID. format is psn_cache_addr_t
        uint64_t     oldest_unordered  : 16; // Oldest unordered PSN known to be outstanding. format is psn_cache_addr_t
        uint64_t         max_seq_dist  : 12; // Maximum distance between oldest unordered and next unordered PSNs.
        uint64_t                 ctrl  :  4; // Control state for this destination. format is tx_proto_ctrl_t
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_PSN_CACHE_ACCESS_DATA_BIT_ENABLE_t;

// TXOTR_PKT_CFG_PSN_CACHE_ACCESS_TAG desc:
typedef union {
    struct {
        uint64_t      tag_way_addr_lo  : 29; // psn cache tag way address, format is psn_cache_addr_t
        uint64_t     tag_way_valid_lo  :  1; // psn cache tag way valid
        uint64_t           reserve_lo  :  2; // 
        uint64_t      tag_way_addr_hi  : 29; // psn cache tag way address, format is psn_cache_addr_t
        uint64_t     tag_way_valid_hi  :  1; // psn cache tag way valid
        uint64_t           reserve_hi  :  2; // 
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_PSN_CACHE_ACCESS_TAG_t;

// TXOTR_PKT_CFG_RFS desc:
typedef union {
    struct {
        uint64_t         mc0tc0_shift  :  3; // Rendezvous Fragment Size for MC0/TC0. Calculated using the following formula: 1 << (7 + mc*tc*_shift) 3'd0 - 128B 3'd1 - 256B 3'd2 - 512B 3'd3 - 1KB 3'd4 - 2KB 3'd5 - 4KB 3'd6 - 8KB 3'd7 - Not valid: defaults to 8KB
        uint64_t             UNUSED_3  :  1; // Unused
        uint64_t         mc0tc1_shift  :  3; // Rendezvous Fragment Size for MC0/TC1. See calculation below.
        uint64_t             UNUSED_7  :  1; // Unused
        uint64_t         mc0tc2_shift  :  3; // Rendezvous Fragment Size for MC0/TC2. See calculation below.
        uint64_t            UNUSED_11  :  1; // Unused
        uint64_t         mc0tc3_shift  :  3; // Rendezvous Fragment Size for MC0/TC3. See calculation below.
        uint64_t            UNUSED_15  :  1; // Unused
        uint64_t         mc1tc0_shift  :  3; // Rendezvous Fragment Size for MC1/TC0. See calculation below.
        uint64_t            UNUSED_19  :  1; // Unused
        uint64_t         mc1tc1_shift  :  3; // Rendezvous Fragment Size for MC1/TC1. See calculation below.
        uint64_t            UNUSED_23  :  1; // Unused
        uint64_t         mc1tc2_shift  :  3; // Rendezvous Fragment Size for MC1/TC2. See calculation below.
        uint64_t            UNUSED_27  :  1; // Unused
        uint64_t         mc1tc3_shift  :  3; // Rendezvous Fragment Size for MC1/TC3. See calculation below.
        uint64_t         UNUSED_63_31  : 33; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_RFS_t;

// TXOTR_PKT_CFG_RC_ORDERED_MAP desc:
typedef union {
    struct {
        uint64_t              ordered  :  8; // 1 bit = ordered packet 0 bit = unordered packet RC mapping to ordered
        uint64_t        Reserved_63_8  : 56; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_RC_ORDERED_MAP_t;

// TXOTR_PKT_CFG_NACK_RETRAS_DISABLE desc:
typedef union {
    struct {
        uint64_t              oos_tc0  :  1; // Disable retransmits resulting from Out-of-sequence NACK for TC0
        uint64_t              oos_tc1  :  1; // Disable retransmits resulting from Out-of-sequence NACK for TC1
        uint64_t              oos_tc2  :  1; // Disable retransmits resulting from Out-of-sequence NACK for TC2
        uint64_t              oos_tc3  :  1; // Disable retransmits resulting from Out-of-sequence NACK for TC3
        uint64_t         resource_tc0  :  1; // Disable retransmits resulting from Resource NACK for TC0
        uint64_t         resource_tc1  :  1; // Disable retransmits resulting from Resource NACK for TC1
        uint64_t         resource_tc2  :  1; // Disable retransmits resulting from Resource NACK for TC2
        uint64_t         resource_tc3  :  1; // Disable retransmits resulting from Resource NACK for TC3
        uint64_t        Reserved_63_8  : 56; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_NACK_RETRAS_DISABLE_t;

// TXOTR_PKT_CFG_RETRANS_BLOCKING desc:
typedef union {
    struct {
        uint64_t            mc0tc0_fp  :  1; // If set, do not allow disabling MC0TC0 during retransmission for the Fast Path
        uint64_t            mc0tc1_fp  :  1; // If set, do not allow disabling MC0TC1 during retransmission for the Fast Path
        uint64_t            mc0tc2_fp  :  1; // If set, do not allow disabling MC0TC2 during retransmission for the Fast Path
        uint64_t            mc0tc3_fp  :  1; // If set, do not allow disabling MC0TC3 during retransmission for the Fast Path
        uint64_t            mc1tc0_fp  :  1; // If set, do not allow disabling MC1TC0 during retransmission for the Fast Path
        uint64_t            mc1tc1_fp  :  1; // If set, do not allow disabling MC1TC1 during retransmission for the Fast Path
        uint64_t            mc1tc2_fp  :  1; // If set, do not allow disabling MC1TC2 during retransmission for the Fast Path
        uint64_t            mc1tc3_fp  :  1; // If set, do not allow disabling MC1TC3 during retransmission for the Fast Path
        uint64_t            mc0tc0_pf  :  1; // If set, do not allow disabling MC0TC0 during retransmission for the Post Fragmentation Path
        uint64_t            mc0tc1_pf  :  1; // If set, do not allow disabling MC0TC1 during retransmission for the Post Fragmentation Path
        uint64_t            mc0tc2_pf  :  1; // If set, do not allow disabling MC0TC2 during retransmission for the Post Fragmentation Path
        uint64_t            mc0tc3_pf  :  1; // If set, do not allow disabling MC0TC3 during retransmission for the Post Fragmentation Path
        uint64_t            mc1tc0_pf  :  1; // If set, do not allow disabling MC1TC0 during retransmission for the Post Fragmentation Path
        uint64_t            mc1tc1_pf  :  1; // If set, do not allow disabling MC1TC1 during retransmission for the Post Fragmentation Path
        uint64_t            mc1tc2_pf  :  1; // If set, do not allow disabling MC1TC2 during retransmission for the Post Fragmentation Path
        uint64_t            mc1tc3_pf  :  1; // If set, do not allow disabling MC1TC3 during retransmission for the Post Fragmentation Path
        uint64_t       Reserved_63_16  : 48; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_RETRANS_BLOCKING_t;

// TXOTR_PKT_CFG_LPSN_MAX_DIST desc:
typedef union {
    struct {
        uint64_t             MAX_DIST  : 16; // This register determines the maximum distance between the oldest Local Packet Sequence Number and the current Local Packet Sequence Number. Under normal operating conditions, this field should never be set to a value greater than 16'd32768.
        uint64_t         UNUSED_63_16  : 48; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_LPSN_MAX_DIST_t;

// TXOTR_PKT_CFG_FORCE_MSG_TO_FPE desc:
typedef union {
    struct {
        uint64_t                  rts  :  1; // RTS command, with or without IOVEC
        uint64_t            rdv_event  :  1; // Rendezvous Event
        uint64_t               dma_pa  :  1; // DMA Put or Atomic
        uint64_t              buff_pa  :  1; // Buffered Put or Atomic
        uint64_t              dma_get  :  1; // DMA Get
        uint64_t         buff_fatomic  :  1; // Buffered Fetching Atomic
        uint64_t           buff_twoop  :  1; // Buffered Two Operation
        uint64_t          dma_fatomic  :  1; // DMA Fetching Atomic
        uint64_t            dma_twoop  :  1; // DMA Two Operation
        uint64_t         standard_ack  :  1; // Portals ACK, NACK, or Special ACK
        uint64_t           buff_reply  :  1; // Buffered Reply
        uint64_t            dma_reply  :  1; // DMA Reply
        uint64_t            basic_ack  :  1; // Basic ACK or Extended ACK
        uint64_t                  cts  :  1; // CTS or ECTS
        uint64_t             e2e_ctrl  :  1; // E2E Control
        uint64_t         UNUSED_63_15  : 49; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_FORCE_MSG_TO_FPE_t;

// TXOTR_PKT_CFG_ENTROPY desc:
typedef union {
    struct {
        uint64_t                  tc0  : 16; // TC0 Entropy
        uint64_t                  tc1  : 16; // TC1 Entropy
        uint64_t                  tc2  : 16; // TC2 Entropy
        uint64_t                  tc3  : 16; // TC3 Entropy
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_ENTROPY_t;

// TXOTR_PKT_ERR_STS_1 desc:
typedef union {
    struct {
        uint64_t           diagnostic  :  1; // Diagnostic Error Flag
        uint64_t         fpe_firmware  :  1; // Fragmentation Programmable Engine Error Flag. See txotr_pkt_err_info_fpe_firmware CSR for error information.
        uint64_t              opb_mbe  :  1; // Outstanding Packet Buffer MBE Error Flag. See txotr_pkt_err_info_opb_be CSR for error information.
        uint64_t              opb_sbe  :  1; // Outstanding Packet Buffer SBE Error Flag. See txotr_pkt_err_info_opb_be CSR for error information.
        uint64_t       pktid_list_mbe  :  1; // Packet Identifier List MBE Error Flag. See txotr_pkt_err_info_pktid_list_be CSR for error information.
        uint64_t       pktid_list_sbe  :  1; // Packet Identifier List SBE Error Flag. See txotr_pkt_err_info_pktid_list_be CSR for error information.
        uint64_t       hash_table_mbe  :  1; // Hash Table MBE Error Flag. See txotr_pkt_err_info_hash_table_be CSR for error information.
        uint64_t       hash_table_sbe  :  1; // Hash Table SBE Error Flag. See txotr_pkt_err_info_hash_table_be CSR for error information.
        uint64_t       iovec_buff_mbe  :  1; // IOVEC Buffer Space MBE Error Flag. See txotr_pkt_err_info_iovec_buff_be CSR for error information.
        uint64_t       iovec_buff_sbe  :  1; // IOVEC Buffer Space SBE Error Flag. See txotr_pkt_err_info_iovec_buff_be CSR for error information.
        uint64_t              otm_mbe  :  1; // Outstanding Translation Memory MBE Error Flag. See txotr_pkt_err_info_otm_be CSR for error information.
        uint64_t              otm_sbe  :  1; // Outstanding Translation Memory SBE Error Flag. See txotr_pkt_err_info_otm_be CSR for error information.
        uint64_t   psn_cache_data_mbe  :  1; // PSN Data Cache MBE Error Flag. See txotr_pkt_err_info_psn_cache_data_sbe_mbe CSR for error information.
        uint64_t   psn_cache_data_sbe  :  1; // PSN Data Cache SBE Error Flag. See txotr_pkt_err_info_psn_cache_data_sbe_mbe CSR for error information.
        uint64_t    psn_cache_tag_mbe  :  1; // PSN Tag Cache MBE Error Flag. See txotr_pkt_err_info_psn_cache_tag_mbe CSR for error information.
        uint64_t    psn_cache_tag_sbe  :  1; // PSN Tag Cache SBE Error Flag. See txotr_pkt_err_info_psn_cache_tag_sbe CSR for error information.
        uint64_t     fpe_prog_mem_mbe  :  1; // Fragmentation Programmable Engine Program Memory MBE Error Flag. See txotr_pkt_err_info_fpe_prog_mem_be CSR for error information.
        uint64_t     fpe_prog_mem_sbe  :  1; // Fragmentation Programmable Engine Program Memory SBE Error Flag. See txotr_pkt_err_info_fpe_prog_mem_be CSR for error information.
        uint64_t     fpe_data_mem_mbe  :  1; // Fragmentation Programmable Engine Data Memory MBE Error Flag. See txotr_pkt_err_info_fpe_data_mem_be CSR for error information.
        uint64_t     fpe_data_mem_sbe  :  1; // Fragmentation Programmable Engine Data Memory SBE Error Flag. See txotr_pkt_err_info_fpe_data_mem_be CSR for error information.
        uint64_t           rx_pkt_mbe  :  1; // Multiple Bit Error in command from RXE2E Error Flag. See txotr_pkt_err_info_rx_pkt_be CSR for error information.
        uint64_t           rx_pkt_sbe  :  1; // Single Bit Error in packet from RXE2E Error Flag. See txotr_pkt_err_info_rx_pkt_be CSR for error information.
        uint64_t               hi_mbe  :  1; // Multiple Bit Error in response state from Host Memory Error Flag. See txotr_pkt_err_info_hi_be CSR for error information.
        uint64_t               hi_sbe  :  1; // Single Bit Error in response state from Host Memory Error Flag. See txotr_pkt_err_info_hi_be CSR for error information.
        uint64_t        pkt_queue_ovf  :  1; // A queue in the packet partition has overflowed. See txotr_pkt_err_info_pkt_ovf_unf CSR for error information.
        uint64_t        pkt_queue_unf  :  1; // A queue in the packet partition has underflowed. See txotr_pkt_err_info_pkt_ovf_unf CSR for error information.
        uint64_t               at_rsp  :  1; // Address Translation response contains an error. See txotr_pkt_err_info_at_rsp CSR for error information.
        uint64_t        misrouted_pkt  :  1; // Packet received from RXE2E mismatches in one of the following fields: MSG_ID, PSN, DLID, or Port, or the OPB entry which corresponds to the PKT_ID in the packet from RXE2E is not valid. See txotr_pkt_err_info_misrouted_pkt CSR for error information.
        uint64_t     unconnected_dlid  :  1; // Attempt to send a command to an unconnected DLID. See txotr_pkt_err_info_unconnected_dlid CSR for error information.
        uint64_t         hash_timeout  :  1; // Entry at the head of the Hash Table has experienced a timeout. See txotr_pkt_err_info_hash_timeout CSR for error information.
        uint64_t         retrans_nack  :  1; // NACK packet received which generates a retransmit. See txotr_pkt_err_info_retrans_nack CSR for error information.
        uint64_t                txdma  :  1; // Packet experienced an error in TXDMA that caused the packet to be dropped. - Unrecoverable translation fault (cannot retransmit) - Memory Response Timeout (can retransmit - no feedback) - SECDED (can retransmit - no feedback). See txotr_pkt_err_info_txdma CSR for error information.
        uint64_t      local_seq_stall  :  1; // If set, indicates that a local sequence has stalled due to the maximum distance between the oldest outstanding local packet sequence number and the next available local packet sequence number exceeding half of the sequence number space. See txotr_pkt_err_info_local_seq_stall CSR for error information.
        uint64_t        tc0_fail_over  :  1; // MC0TC0 Fail Over Limit reached. See txotr_pkt_err_info_tc0_fail_over_limit CSR for error information.
        uint64_t        tc1_fail_over  :  1; // MC0TC1 Fail Over Limit reached. See txotr_pkt_err_info_tc1_fail_over_limit CSR for error information.
        uint64_t        tc2_fail_over  :  1; // MC0TC2 Fail Over Limit reached. See txotr_pkt_err_info_tc2_fail_over_limit CSR for error information.
        uint64_t        tc3_fail_over  :  1; // MC0TC3 Fail Over Limit reached. See txotr_pkt_err_info_tc3_fail_over_limit CSR for error information.
        uint64_t    tc0_retrans_limit  :  1; // MC0TC0 Retransmit Limit reached. See txotr_pkt_err_info_tc0_retrans_limit CSR for error information.
        uint64_t    tc1_retrans_limit  :  1; // MC0TC1 Retransmit Limit reached. See txotr_pkt_err_info_tc1_retrans_limit CSR for error information.
        uint64_t    tc2_retrans_limit  :  1; // MC0TC2 Retransmit Limit reached. See txotr_pkt_err_info_tc2_retrans_limit CSR for error information.
        uint64_t    tc3_retrans_limit  :  1; // MC0TC3 Retransmit Limit reached. See txotr_pkt_err_info_tc3_retrans_limit CSR for error information.
        uint64_t         UNUSED_63_41  : 23; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_STS_1_t;

// TXOTR_PKT_ERR_CLR_1 desc:
typedef union {
    struct {
        uint64_t               events  : 41; // Write 1's to clear corresponding status bits.
        uint64_t         UNUSED_63_41  : 23; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_CLR_1_t;

// TXOTR_PKT_ERR_FRC_1 desc:
typedef union {
    struct {
        uint64_t               events  : 41; // Write 1 to set corresponding status bits.
        uint64_t         UNUSED_63_41  : 23; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_FRC_1_t;

// TXOTR_PKT_ERR_EN_HOST_1 desc:
typedef union {
    struct {
        uint64_t               events  : 41; // Enables corresponding status bits to generate host interrupt signal.
        uint64_t         UNUSED_63_41  : 23; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_EN_HOST_1_t;

// TXOTR_PKT_ERR_FIRST_HOST_1 desc:
typedef union {
    struct {
        uint64_t               events  : 41; // Snapshot of status bits when host interrupt signal transitions from 0 to 1.
        uint64_t         UNUSED_63_41  : 23; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_FIRST_HOST_1_t;

// TXOTR_PKT_ERR_EN_BMC_1 desc:
typedef union {
    struct {
        uint64_t               events  : 41; // Enables corresponding status bits to generate quarantine interrupt signal.
        uint64_t         UNUSED_63_41  : 23; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_EN_BMC_1_t;

// TXOTR_PKT_ERR_FIRST_BMC_1 desc:
typedef union {
    struct {
        uint64_t               events  : 41; // Snapshot of status bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t         UNUSED_63_41  : 23; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_FIRST_BMC_1_t;

// TXOTR_PKT_ERR_EN_QUAR_1 desc:
typedef union {
    struct {
        uint64_t               events  : 41; // Enables corresponding status bits to generate BMC interrupt signal.
        uint64_t         UNUSED_63_41  : 23; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_EN_QUAR_1_t;

// TXOTR_PKT_ERR_FIRST_QUAR_1 desc:
typedef union {
    struct {
        uint64_t               events  : 41; // Snapshot of status bits when quarantine interrupt signal transitions from 0 to 1.
        uint64_t         UNUSED_63_41  : 23; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_FIRST_QUAR_1_t;

// TXOTR_PKT_ERR_INFO_FPE_FIRMWARE desc:
typedef union {
    struct {
        uint64_t             encoding  :  8; // Error encoding. TODO - delineate encodings - Read or write of an invalid address
        uint64_t                 ctxt  :  5; // Context from which the error originated
        uint64_t         UNUSED_15_13  :  3; // Unused
        uint64_t               msg_id  : 16; // Message Identifier of the command associated with the error
        uint64_t         UNUSED_63_32  : 32; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_FPE_FIRMWARE_t;

// TXOTR_PKT_ERR_INFO_OPB_BE desc:
typedef union {
    struct {
        uint64_t               pkt_id  : 16; // Packet Identifier
        uint64_t               msg_id  : 16; // Message Identifier
        uint64_t             syndrome  :  9; // Syndrome. If multiple bit errors occur on a single read, this field indicates the syndrome of the least significant 64-bit portion of the read data, with priority given to an MBE.
        uint64_t         UNUSED_43_41  :  3; // Unused
        uint64_t                  mbe  :  2; // ECC domain containing the bit error
        uint64_t                  sbe  :  2; // ECC domain containing the bit error
        uint64_t         UNUSED_63_48  : 16; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_OPB_BE_t;

// TXOTR_PKT_ERR_INFO_PKTID_LIST_BE desc:
typedef union {
    struct {
        uint64_t              address  : 16; // Address
        uint64_t             syndrome  :  6; // Syndrome
        uint64_t         UNUSED_23_22  :  2; // Unused
        uint64_t                  mbe  :  1; // If set, indicates MBE, else SBE
        uint64_t         UNUSED_63_25  : 39; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_PKTID_LIST_BE_t;

// TXOTR_PKT_ERR_INFO_HASH_TABLE_BE desc:
typedef union {
    struct {
        uint64_t              address  :  7; // Address
        uint64_t          UNUSED_15_7  :  9; // Unused
        uint64_t             syndrome  :  8; // Syndrome
        uint64_t                  mbe  :  1; // If set, indicates MBE, else SBE
        uint64_t         UNUSED_63_25  : 39; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_HASH_TABLE_BE_t;

// TXOTR_PKT_ERR_INFO_IOVEC_BUFF_BE desc:
typedef union {
    struct {
        uint64_t              address  :  5; // Address
        uint64_t          UNUSED_15_5  : 11; // Unused
        uint64_t             syndrome  :  8; // Syndrome. If multiple bit errors occur on a single read, this field indicates the syndrome of the least significant 64-bit portion of the read data, with priority given to an MBE.
        uint64_t                  mbe  :  8; // ECC domain containing the bit error
        uint64_t                  sbe  :  8; // ECC domain containing the bit error
        uint64_t         UNUSED_63_40  : 24; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_IOVEC_BUFF_BE_t;

// TXOTR_PKT_ERR_INFO_OTM_BE desc:
typedef union {
    struct {
        uint64_t              address  :  7; // Address
        uint64_t          UNUSED_15_7  :  9; // Unused
        uint64_t             syndrome  :  9; // Syndrome
        uint64_t         UNUSED_27_25  :  3; // Unused
        uint64_t                  mbe  :  1; // If set, indicates MBE, else SBE
        uint64_t         UNUSED_63_29  : 35; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_OTM_BE_t;

// TXOTR_PKT_ERR_INFO_PSN_CACHE_TAG_SBE desc:
typedef union {
    struct {
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain sbe
        uint64_t      sbe_last_domain  :  3; // ecc domain of last least significant sbe
        uint64_t                  sbe  :  8; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            sbe_count  : 12; // saturating counter of sbes. The increment signal is the 'or' of the 8 sbe signals.
        uint64_t     sbe_last_address  : 11; // address of the last least significant ecc domain sbe
        uint64_t       Reserved_63_42  : 22; // Reserved
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_PSN_CACHE_TAG_SBE_t;

// TXOTR_PKT_ERR_INFO_PSN_CACHE_TAG_MBE desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain mbe
        uint64_t      mbe_last_domain  :  3; // ecc domain of last least significant mbe
        uint64_t                  mbe  :  8; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            mbe_count  :  4; // saturating counter of mbes. The increment signal is the 'or' of the 8 mbe signals.
        uint64_t     mbe_last_address  : 11; // address of the last least significant ecc domain mbe
        uint64_t       Reserved_63_34  : 30; // Reserved
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_PSN_CACHE_TAG_MBE_t;

// TXOTR_PKT_ERR_INFO_PSN_CACHE_DATA_SBE_MBE desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain mbe
        uint64_t            mbe_count  :  4; // saturating counter of mbes.
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain sbe
        uint64_t            sbe_count  : 12; // saturating counter of sbes.
        uint64_t       Reserved_63_32  : 32; // Reserved
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_PSN_CACHE_DATA_SBE_MBE_t;

// TXOTR_PKT_ERR_INFO_FPE_PROG_MEM_BE desc:
typedef union {
    struct {
        uint64_t              address  : 14; // Address
        uint64_t         UNUSED_15_14  :  2; // Unused
        uint64_t             syndrome  :  8; // Syndrome
        uint64_t                  mbe  :  1; // ECC domain containing the bit error
        uint64_t                  sbe  :  1; // ECC domain containing the bit error
        uint64_t         UNUSED_63_26  : 38; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_FPE_PROG_MEM_BE_t;

// TXOTR_PKT_ERR_INFO_FPE_DATA_MEM_BE desc:
typedef union {
    struct {
        uint64_t              address  : 10; // Address
        uint64_t         UNUSED_15_10  :  6; // Unused
        uint64_t             syndrome  :  8; // Syndrome
        uint64_t                  mbe  :  1; // ECC domain containing the bit error
        uint64_t                  sbe  :  1; // ECC domain containing the bit error
        uint64_t         UNUSED_63_26  : 38; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_FPE_DATA_MEM_BE_t;

// TXOTR_PKT_ERR_INFO_RX_PKT_BE desc:
typedef union {
    struct {
        uint64_t                  TBD  : 16; // TBD
        uint64_t             syndrome  :  8; // Syndrome. If multiple bit errors occur on a single read, this field indicates the syndrome of the least significant 64-bit portion of the read data, with priority given to an MBE.
        uint64_t                  mbe  :  4; // ECC domain containing the bit error
        uint64_t                  sbe  :  4; // ECC domain containing the bit error
        uint64_t         UNUSED_63_32  : 32; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_RX_PKT_BE_t;

// TXOTR_PKT_ERR_INFO_HI_BE desc:
typedef union {
    struct {
        uint64_t                  TBD  : 16; // TBD
        uint64_t             syndrome  :  8; // Syndrome
        uint64_t                  mbe  :  1; // ECC domain containing the bit error
        uint64_t                  sbe  :  1; // ECC domain containing the bit error
        uint64_t         UNUSED_63_26  : 38; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_HI_BE_t;

// TXOTR_PKT_ERR_INFO_PKT_OVF_UNF desc:
typedef union {
    struct {
        uint64_t            OVF_QUEUE  :  8; // Encoding to denote which queue is overflowing
        uint64_t            UNF_QUEUE  :  8; // Encoding to denote which queue is underflowing
        uint64_t         UNUSED_63_16  : 48; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_PKT_OVF_UNF_t;

// TXOTR_PKT_ERR_INFO_AT_RSP desc:
typedef union {
    struct {
        uint64_t             encoding  :  2; // Error encoding 2'b10 - Page Walk Abort 2'b01 - Page Fault
        uint64_t           UNUSED_3_2  :  2; // Unused
        uint64_t                  tid  : 12; // Transaction Identifier
        uint64_t            otm_Index  :  7; // Index into Outstanding Translation Memory
        uint64_t            UNUSED_23  :  1; // Unused
        uint64_t            requestor  :  1; // 1'b0 - Fragmentation Programmable Engine 1'b1 - PSN Cache
        uint64_t         UNUSED_63_25  : 39; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_AT_RSP_t;

// TXOTR_PKT_ERR_INFO_MISROUTED_PKT desc:
typedef union {
    struct {
        uint64_t             encoding  :  5; // 5'b1???? - Mismatched MSG_ID 5'b?1??? - Mismatched PSN 5'b??1?? - Mismatched DLID 5'b???1? - Mismatched Port 5'b????1 - PKT_ID indicates invalid OPB entry
        uint64_t           UNUSED_7_5  :  3; // Unused
        uint64_t                 slid  : 24; // SLID of misrouted packet
        uint64_t               pkt_id  : 16; // PKT_ID of misrouted packet
        uint64_t         UNUSED_63_48  : 16; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_MISROUTED_PKT_t;

// TXOTR_PKT_ERR_INFO_UNCONNECTED_DLID desc:
typedef union {
    struct {
        uint64_t               MSG_ID  : 16; // Message ID assigned to command.
        uint64_t              address  : 28; // address (DLID, TC, Port)
        uint64_t         UNUSED_63_44  : 20; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_UNCONNECTED_DLID_t;

// TXOTR_PKT_ERR_INFO_HASH_TIMEOUT desc:
typedef union {
    struct {
        uint64_t        oldest_pkt_id  : 16; // Oldest Packet ID, as indicated by the Hash Table Entry
        uint64_t              address  :  7; // address
        uint64_t         UNUSED_63_23  : 41; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_HASH_TIMEOUT_t;

// TXOTR_PKT_ERR_INFO_RETRANS_NACK desc:
typedef union {
    struct {
        uint64_t               pkt_id  : 16; // Packet Identifier to be retransmitted
        uint64_t         UNUSED_63_16  : 48; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_RETRANS_NACK_t;

// TXOTR_PKT_ERR_INFO_TXDMA desc:
typedef union {
    struct {
        uint64_t               pkt_id  : 16; // Packet Identifier of errored packet
        uint64_t             encoding  :  4; // Error Encoding 4'b1??? - SECDED error 4'b?1?? - Memory Response Timeout 4'b??1? - Unrecoverable translation fault 4'b???1 - Reserved
        uint64_t         UNUSED_63_20  : 44; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_TXDMA_t;

// TXOTR_PKT_ERR_INJECT_PSN_CACHE_SBE_MBE desc:
typedef union {
    struct {
        uint64_t psn_cache_tag_err_inj_mask  :  8; // psn_cache_tag error inject mask
        uint64_t psn_cache_tag_err_inj_domain  :  3; // psn_cache_tag error inject domain. Which ecc domain to inject the error.
        uint64_t psn_cache_tag_err_inj_enable  :  1; // psn_cache_tag error inject enable.
        uint64_t psn_cache_data_err_inj_mask  :  8; // psn_cache_data error inject mask
        uint64_t psn_cache_data_err_inj_enable  :  1; // psn_cache_data error inject enable.
        uint64_t       Reserved_63_21  : 43; // Reserved
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INJECT_PSN_CACHE_SBE_MBE_t;

// TXOTR_PKT_ERR_INFO_LOCAL_SEQ_STALL desc:
typedef union {
    struct {
        uint64_t       local_sequence  :  7; // Local Sequence that is stalling
        uint64_t           Reserved_7  :  1; // Reserved
        uint64_t          oldest_lpsn  : 16; // Oldest outstanding LPSN
        uint64_t            next_lpsn  : 16; // Next LPSN
        uint64_t         UNUSED_63_40  : 24; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_LOCAL_SEQ_STALL_t;

// TXOTR_PKT_ERR_INFO_TC0_FAIL_OVER_LIMIT desc:
typedef union {
    struct {
        uint64_t                  lid  : 24; // Most recent PKT_ID to reach the retransmit limit
        uint64_t       Reserved_63_24  : 40; // Reserved
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_TC0_FAIL_OVER_LIMIT_t;

// TXOTR_PKT_ERR_INFO_TC1_FAIL_OVER_LIMIT desc:
typedef union {
    struct {
        uint64_t                  lid  : 24; // Most recent PKT_ID to reach the retransmit limit
        uint64_t       Reserved_63_24  : 40; // Reserved
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_TC1_FAIL_OVER_LIMIT_t;

// TXOTR_PKT_ERR_INFO_TC2_FAIL_OVER_LIMIT desc:
typedef union {
    struct {
        uint64_t                  lid  : 24; // Most recent PKT_ID to reach the retransmit limit
        uint64_t       Reserved_63_24  : 40; // Reserved
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_TC2_FAIL_OVER_LIMIT_t;

// TXOTR_PKT_ERR_INFO_TC3_FAIL_OVER_LIMIT desc:
typedef union {
    struct {
        uint64_t                  lid  : 24; // Most recent PKT_ID to reach the retransmit limit
        uint64_t       Reserved_63_24  : 40; // Reserved
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_TC3_FAIL_OVER_LIMIT_t;

// TXOTR_PKT_ERR_INFO_TC0_RETRANS_LIMIT desc:
typedef union {
    struct {
        uint64_t                  lid  : 24; // Most recent PKT_ID to reach the retransmit limit
        uint64_t       Reserved_63_24  : 40; // Reserved
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_TC0_RETRANS_LIMIT_t;

// TXOTR_PKT_ERR_INFO_TC1_RETRANS_LIMIT desc:
typedef union {
    struct {
        uint64_t                  lid  : 24; // Most recent PKT_ID to reach the retransmit limit
        uint64_t       Reserved_63_24  : 40; // Reserved
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_TC1_RETRANS_LIMIT_t;

// TXOTR_PKT_ERR_INFO_TC2_RETRANS_LIMIT desc:
typedef union {
    struct {
        uint64_t                  lid  : 24; // Most recent PKT_ID to reach the retransmit limit
        uint64_t       Reserved_63_24  : 40; // Reserved
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_TC2_RETRANS_LIMIT_t;

// TXOTR_PKT_ERR_INFO_TC3_RETRANS_LIMIT desc:
typedef union {
    struct {
        uint64_t                  lid  : 24; // Most recent PKT_ID to reach the retransmit limit
        uint64_t       Reserved_63_24  : 40; // Reserved
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_TC3_RETRANS_LIMIT_t;

// TXOTR_PKT_DBG_PSN_CACHE_TAG_WAY_ENABLE desc:
typedef union {
    struct {
        uint64_t           WAY_ENABLE  : 16; // 1 bits enable, 0 bits disable PSN Cache Tag ways.
        uint64_t       Reserved_63_16  : 48; // Reserved
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_PSN_CACHE_TAG_WAY_ENABLE_t;

// TXOTR_PKT_DBG_OPB_ACCESS desc:
typedef union {
    struct {
        uint64_t              address  : 16; // Address for Read or Write. Valid address are from 0 to 12287. For FXR, bits 15:14 are ignored. No read or write is performed for an address value between 12288 and 16383.
        uint64_t       Reserved_51_16  : 36; // Unused
        uint64_t         Payload_regs  :  8; // Constant indicating number of payload registers follow
        uint64_t          Reserved_60  :  1; // Unused
        uint64_t                  ECC  :  1; // 0 = Read/Write Raw Data 1 = Generate ECC on Write, Correct Data on Read
        uint64_t                Write  :  1; // 0 = Read, 1 = Write
        uint64_t                Valid  :  1; // Set by software, cleared by hardware when complete.
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_OPB_ACCESS_t;

// TXOTR_PKT_DBG_OPB_PAYLOAD0 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data[63:0]
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_OPB_PAYLOAD0_t;

// TXOTR_PKT_DBG_OPB_PAYLOAD1 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data[127:64]
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_OPB_PAYLOAD1_t;

// TXOTR_PKT_DBG_OPB_PAYLOAD2 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data[191:128]
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_OPB_PAYLOAD2_t;

// TXOTR_PKT_DBG_OPB_PAYLOAD3 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data[255:192]
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_OPB_PAYLOAD3_t;

// TXOTR_PKT_DBG_OPB_PAYLOAD4 desc:
typedef union {
    struct {
        uint64_t                 Data  : 24; // Data[279:256]
        uint64_t       Reserved_63_24  : 40; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_OPB_PAYLOAD4_t;

// TXOTR_PKT_DBG_PKTID_LIST_ACCESS desc:
typedef union {
    struct {
        uint64_t              address  : 16; // Address for Read or Write. Valid address are from 0 to 12287. For FXR, bits 15:14 are ignored. No read or write is performed for an address value between 12288 and 16383.
        uint64_t       Reserved_51_16  : 36; // Unused
        uint64_t         Payload_regs  :  8; // Constant indicating number of payload registers follow
        uint64_t          Reserved_60  :  1; // Unused
        uint64_t                  ECC  :  1; // 0 = Read/Write Raw Data 1 = Generate ECC on Write, Correct Data on Read
        uint64_t                Write  :  1; // 0 = Read, 1 = Write
        uint64_t                Valid  :  1; // Set by software, cleared by hardware when complete.
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_PKTID_LIST_ACCESS_t;

// TXOTR_PKT_DBG_PKTID_LIST_PAYLOAD desc:
typedef union {
    struct {
        uint64_t                 Data  : 28; // Data
        uint64_t       Reserved_63_28  : 36; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_PKTID_LIST_PAYLOAD_t;

// TXOTR_PKT_DBG_HASH_TABLE_ACCESS desc:
typedef union {
    struct {
        uint64_t              address  :  7; // Address for Read or Write. Valid address are from 0 to 127.
        uint64_t        Reserved_51_7  : 45; // Unused
        uint64_t         Payload_regs  :  8; // Constant indicating number of payload registers follow
        uint64_t          Reserved_60  :  1; // Unused
        uint64_t                  ECC  :  1; // 0 = Read/Write Raw Data 1 = Generate ECC on Write, Correct Data on Read
        uint64_t                Write  :  1; // 0 = Read, 1 = Write
        uint64_t                Valid  :  1; // Set by software, cleared by hardware when complete.
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_HASH_TABLE_ACCESS_t;

// TXOTR_PKT_DBG_HASH_TABLE_PAYLOAD0 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_HASH_TABLE_PAYLOAD0_t;

// TXOTR_PKT_DBG_HASH_TABLE_PAYLOAD1 desc:
typedef union {
    struct {
        uint64_t                 Data  : 12; // Data
        uint64_t       Reserved_63_12  : 52; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_HASH_TABLE_PAYLOAD1_t;

// TXOTR_PKT_DBG_OTM_ACCESS desc:
typedef union {
    struct {
        uint64_t              address  :  7; // Address for Read or Write. Valid address are from 0 to 127.
        uint64_t        Reserved_51_7  : 45; // Unused
        uint64_t         Payload_regs  :  8; // Constant indicating number of payload registers follow
        uint64_t          Reserved_60  :  1; // Unused
        uint64_t                  ECC  :  1; // 0 = Read/Write Raw Data 1 = Generate ECC on Write, Correct Data on Read
        uint64_t                Write  :  1; // 0 = Read, 1 = Write
        uint64_t                Valid  :  1; // Set by software, cleared by hardware when complete.
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_OTM_ACCESS_t;

// TXOTR_PKT_DBG_OTM_PAYLOAD0 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_OTM_PAYLOAD0_t;

// TXOTR_PKT_DBG_OTM_PAYLOAD1 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_OTM_PAYLOAD1_t;

// TXOTR_PKT_DBG_OTM_PAYLOAD2 desc:
typedef union {
    struct {
        uint64_t                 Data  :  9; // Data
        uint64_t        Reserved_63_9  : 55; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_OTM_PAYLOAD2_t;

// TXOTR_PKT_DBG_IOVEC_BUFFER_SPACE_ACCESS desc:
typedef union {
    struct {
        uint64_t              address  :  7; // Address for Read or Write. Valid address are from 0 to 127.
        uint64_t        Reserved_51_7  : 45; // Unused
        uint64_t         Payload_regs  :  8; // Constant indicating number of payload registers follow
        uint64_t          Reserved_60  :  1; // Unused
        uint64_t                  ECC  :  1; // 0 = Read/Write Raw Data 1 = Generate ECC on Write, Correct Data on Read
        uint64_t                Write  :  1; // 0 = Read, 1 = Write
        uint64_t                Valid  :  1; // Set by software, cleared by hardware when complete.
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_IOVEC_BUFFER_SPACE_ACCESS_t;

// TXOTR_PKT_DBG_IOVEC_BUFFER_SPACE_PAYLOAD0 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data[63:0]
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_IOVEC_BUFFER_SPACE_PAYLOAD0_t;

// TXOTR_PKT_DBG_IOVEC_BUFFER_SPACE_PAYLOAD1 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data[127:64]
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_IOVEC_BUFFER_SPACE_PAYLOAD1_t;

// TXOTR_PKT_DBG_IOVEC_BUFFER_SPACE_PAYLOAD2 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data[191:128]
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_IOVEC_BUFFER_SPACE_PAYLOAD2_t;

// TXOTR_PKT_DBG_IOVEC_BUFFER_SPACE_PAYLOAD3 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data[255:192]
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_IOVEC_BUFFER_SPACE_PAYLOAD3_t;

// TXOTR_PKT_DBG_IOVEC_BUFFER_SPACE_PAYLOAD4 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data[319:256]
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_IOVEC_BUFFER_SPACE_PAYLOAD4_t;

// TXOTR_PKT_DBG_IOVEC_BUFFER_SPACE_PAYLOAD5 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data[383:320]
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_IOVEC_BUFFER_SPACE_PAYLOAD5_t;

// TXOTR_PKT_DBG_IOVEC_BUFFER_SPACE_PAYLOAD6 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data[447:384]
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_IOVEC_BUFFER_SPACE_PAYLOAD6_t;

// TXOTR_PKT_DBG_IOVEC_BUFFER_SPACE_PAYLOAD7 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data[511:448]
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_IOVEC_BUFFER_SPACE_PAYLOAD7_t;

// TXOTR_PKT_DBG_FPE_PROG_MEM_ACCESS desc:
typedef union {
    struct {
        uint64_t              address  : 13; // Address for Read or Write. Valid address are from 0 to 8191.
        uint64_t       Reserved_51_13  : 39; // Unused
        uint64_t         Payload_regs  :  8; // Constant indicating number of payload registers follow
        uint64_t          Reserved_60  :  1; // Unused
        uint64_t                  ECC  :  1; // 0 = Read/Write Raw Data 1 = Generate ECC on Write, Correct Data on Read
        uint64_t                Write  :  1; // 0 = Read, 1 = Write
        uint64_t                Valid  :  1; // Set by software, cleared by hardware when complete.
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_FPE_PROG_MEM_ACCESS_t;

// TXOTR_PKT_DBG_FPE_PROG_MEM_PAYLOAD0 desc:
typedef union {
    struct {
        uint64_t                 Data  : 32; // Data[31:0]
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_FPE_PROG_MEM_PAYLOAD0_t;

// TXOTR_PKT_DBG_FPE_DATA_MEM_ACCESS desc:
typedef union {
    struct {
        uint64_t              address  : 10; // Address for Read or Write. Valid address are from 0 to 1023.
        uint64_t       Reserved_51_10  : 42; // Unused
        uint64_t         Payload_regs  :  8; // Constant indicating number of payload registers follow
        uint64_t          Reserved_60  :  1; // Unused
        uint64_t                  ECC  :  1; // 0 = Read/Write Raw Data 1 = Generate ECC on Write, Correct Data on Read
        uint64_t                Write  :  1; // 0 = Read, 1 = Write
        uint64_t                Valid  :  1; // Set by software, cleared by hardware when complete.
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_FPE_DATA_MEM_ACCESS_t;

// TXOTR_PKT_DBG_FPE_DATA_MEM_PAYLOAD0 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data[63:0]
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_FPE_DATA_MEM_PAYLOAD0_t;

// TXOTR_PKT_DBG_FP_PRE_FRAG_FULL_EMPTY desc:
typedef union {
    struct {
        uint64_t           FAST_EMPTY  : 12; // Empty bits of Fast Path Queues, 1 bit per queue
        uint64_t            FAST_FULL  : 12; // Full bits of Fast Path Queues, 1 bit per queue
        uint64_t         UNUSED_31_24  :  8; // Unused
        uint64_t            PRE_EMPTY  :  8; // Empty bits of Pre-Fragmentation Queues, 1 bit per queue
        uint64_t             PRE_FULL  :  8; // Full bits of Pre-Fragmentation Queues, 1 bit per queue
        uint64_t         UNUSED_63_48  : 16; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_FP_PRE_FRAG_FULL_EMPTY_t;

// TXOTR_PKT_DBG_POST_FRAG_RET_FULL_EMPTY desc:
typedef union {
    struct {
        uint64_t           POST_EMPTY  :  8; // Empty bits of Post-Fragmentation Queues, 1 bit per queue
        uint64_t            POST_FULL  :  8; // Full bits of Post-Fragmentation Queues, 1 bit per queue
        uint64_t         UNUSED_31_16  : 16; // Unused
        uint64_t       POST_RET_EMPTY  :  8; // Empty bits of Post-Frag Retransmission Queues, 1 bit per queue
        uint64_t        POST_RET_FULL  :  8; // Full bits of Post-Frag Retransmission Queues, 1 bit per 8 queues
        uint64_t         UNUSED_63_48  : 16; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_POST_FRAG_RET_FULL_EMPTY_t;

// TXOTR_PKT_DBG_FPE_CONTEXT_AVAILABLE_REG desc:
typedef union {
    struct {
        uint64_t                  FPE  : 32; // Context-Available Register of FPE
        uint64_t         UNUSED_63_32  : 32; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_FPE_CONTEXT_AVAILABLE_REG_t;

// TXOTR_PKT_DBG_OPB_FP_MSG_COUNT desc:
typedef union {
    struct {
        uint64_t                count  : 64; // Count
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_OPB_FP_MSG_COUNT_t;

// TXOTR_PKT_DBG_OPB_PRE_FRAG_MSG_COUNT desc:
typedef union {
    struct {
        uint64_t                count  : 64; // Count
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_OPB_PRE_FRAG_MSG_COUNT_t;

// TXOTR_PKT_DBG_OPB_RETRANS_PRE_FRAG_MSG_COUNT desc:
typedef union {
    struct {
        uint64_t                count  : 64; // Count
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_OPB_RETRANS_PRE_FRAG_MSG_COUNT_t;

// TXOTR_PKT_DBG_OPB_RETRANS_POST_FRAG_MSG_COUNT desc:
typedef union {
    struct {
        uint64_t                count  : 64; // Count
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_OPB_RETRANS_POST_FRAG_MSG_COUNT_t;

// TXOTR_PKT_DBG_OPB_POST_FRAG_PKT_COUNT desc:
typedef union {
    struct {
        uint64_t                count  : 64; // Count
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_OPB_POST_FRAG_PKT_COUNT_t;

// TXOTR_PKT_DBG_OPB_PKT_COUNT desc:
typedef union {
    struct {
        uint64_t                count  : 64; // Count
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_OPB_PKT_COUNT_t;

// TXOTR_PKT_DBG_OPB_REPLY_PKT_COUNT desc:
typedef union {
    struct {
        uint64_t                count  : 64; // Count
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_OPB_REPLY_PKT_COUNT_t;

// TXOTR_PKT_DBG_PKTID_LIST_SHARED_POINTERS desc:
typedef union {
    struct {
        uint64_t                 head  : 14; // Head Pointer
        uint64_t         UNUSED_15_14  :  2; // Unused
        uint64_t                 tail  : 14; // Tail Pointer
        uint64_t         UNUSED_63_30  : 34; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_PKTID_LIST_SHARED_POINTERS_t;

// TXOTR_PKT_DBG_PKTID_LIST_RSVD_POINTERS0 desc:
typedef union {
    struct {
        uint64_t             mc0_head  : 14; // MC0 Head Pointer
        uint64_t         UNUSED_15_14  :  2; // Unused
        uint64_t             mc0_tail  : 14; // MC0 Tail Pointer
        uint64_t         UNUSED_31_30  :  2; // Unused
        uint64_t             mc1_head  : 14; // MC1 Head Pointer
        uint64_t         UNUSED_47_46  :  2; // Unused
        uint64_t             mc1_tail  : 14; // MC1 Tail Pointer
        uint64_t         UNUSED_63_62  :  2; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_PKTID_LIST_RSVD_POINTERS0_t;

// TXOTR_PKT_DBG_PKTID_LIST_RSVD_POINTERS1 desc:
typedef union {
    struct {
        uint64_t             mc0_head  : 14; // MC0 Head Pointer
        uint64_t         UNUSED_15_14  :  2; // Unused
        uint64_t             mc0_tail  : 14; // MC0 Tail Pointer
        uint64_t         UNUSED_31_30  :  2; // Unused
        uint64_t             mc1_head  : 14; // MC1 Head Pointer
        uint64_t         UNUSED_47_46  :  2; // Unused
        uint64_t             mc1_tail  : 14; // MC1 Tail Pointer
        uint64_t         UNUSED_63_62  :  2; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_PKTID_LIST_RSVD_POINTERS1_t;

// TXOTR_PKT_DBG_PKTID_LIST_RSVD_POINTERS2 desc:
typedef union {
    struct {
        uint64_t             mc0_head  : 14; // MC0 Head Pointer
        uint64_t         UNUSED_15_14  :  2; // Unused
        uint64_t             mc0_tail  : 14; // MC0 Tail Pointer
        uint64_t         UNUSED_31_30  :  2; // Unused
        uint64_t             mc1_head  : 14; // MC1 Head Pointer
        uint64_t         UNUSED_47_46  :  2; // Unused
        uint64_t             mc1_tail  : 14; // MC1 Tail Pointer
        uint64_t         UNUSED_63_62  :  2; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_PKTID_LIST_RSVD_POINTERS2_t;

// TXOTR_PKT_DBG_PKTID_LIST_RSVD_POINTERS3 desc:
typedef union {
    struct {
        uint64_t             mc0_head  : 14; // MC0 Head Pointer
        uint64_t         UNUSED_15_14  :  2; // Unused
        uint64_t             mc0_tail  : 14; // MC0 Tail Pointer
        uint64_t         UNUSED_31_30  :  2; // Unused
        uint64_t             mc1_head  : 14; // MC1 Head Pointer
        uint64_t         UNUSED_47_46  :  2; // Unused
        uint64_t             mc1_tail  : 14; // MC1 Tail Pointer
        uint64_t         UNUSED_63_62  :  2; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_PKTID_LIST_RSVD_POINTERS3_t;

// TXOTR_PKT_DBG_PKTID_LIST_PENDING_POINTERS0 desc:
typedef union {
    struct {
        uint64_t             tc0_head  : 14; // TC0 Head Pointer
        uint64_t         UNUSED_15_14  :  2; // Unused
        uint64_t             tc0_tail  : 14; // TC0 Tail Pointer
        uint64_t         UNUSED_31_30  :  2; // Unused
        uint64_t             tc1_head  : 14; // TC1 Head Pointer
        uint64_t         UNUSED_47_46  :  2; // Unused
        uint64_t             tc1_tail  : 14; // TC1 Tail Pointer
        uint64_t         UNUSED_63_62  :  2; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_PKTID_LIST_PENDING_POINTERS0_t;

// TXOTR_PKT_DBG_PKTID_LIST_PENDING_POINTERS1 desc:
typedef union {
    struct {
        uint64_t             tc2_head  : 14; // TC2 Head Pointer
        uint64_t         UNUSED_15_14  :  2; // Unused
        uint64_t             tc2_tail  : 14; // TC2 Tail Pointer
        uint64_t         UNUSED_31_30  :  2; // Unused
        uint64_t             tc3_head  : 14; // TC3 Head Pointer
        uint64_t         UNUSED_47_46  :  2; // Unused
        uint64_t             tc3_tail  : 14; // TC3 Tail Pointer
        uint64_t         UNUSED_63_62  :  2; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_PKTID_LIST_PENDING_POINTERS1_t;

// TXOTR_PKT_DBG_FPE_CONTEXT_ENABLE desc:
typedef union {
    struct {
        uint64_t               mc0tc0  :  1; // MC0TC0 Dedicated Context
        uint64_t               mc0tc1  :  1; // MC0TC1 Dedicated Context
        uint64_t               mc0tc2  :  1; // MC0TC2 Dedicated Context
        uint64_t               mc0tc3  :  1; // MC0TC3 Dedicated Context
        uint64_t               mc1tc0  :  1; // MC1TC0 Dedicated Context
        uint64_t               mc1tc1  :  1; // MC1TC1 Dedicated Context
        uint64_t               mc1tc2  :  1; // MC1TC2 Dedicated Context
        uint64_t               mc1tc3  :  1; // MC1TC3 Dedicated Context
        uint64_t              shared0  :  1; // Shared 0 Context
        uint64_t              shared1  :  1; // Shared 1 Context
        uint64_t              shared2  :  1; // Shared 2 Context
        uint64_t              shared3  :  1; // Shared 3 Context
        uint64_t              shared4  :  1; // Shared 4 Context
        uint64_t              shared5  :  1; // Shared 5 Context
        uint64_t              shared6  :  1; // Shared 6 Context
        uint64_t              shared7  :  1; // Shared 7 Context
        uint64_t          mc0tc0_rsvd  :  1; // MC0TC0 Dedicated Context
        uint64_t          mc0tc1_rsvd  :  1; // MC0TC1 Dedicated Context
        uint64_t          mc0tc2_rsvd  :  1; // MC0TC2 Dedicated Context
        uint64_t          mc0tc3_rsvd  :  1; // MC0TC3 Dedicated Context
        uint64_t          mc1tc0_rsvd  :  1; // MC1TC0 Dedicated Context
        uint64_t          mc1tc1_rsvd  :  1; // MC1TC1 Dedicated Context
        uint64_t          mc1tc2_rsvd  :  1; // MC1TC2 Dedicated Context
        uint64_t          mc1tc3_rsvd  :  1; // MC1TC3 Dedicated Context
        uint64_t         UNUSED_63_24  : 40; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_FPE_CONTEXT_ENABLE_t;

// TXOTR_PKT_DBG_FPE_CONTEXT_MCTC_MAP0 desc:
typedef union {
    struct {
        uint64_t               mc0tc0  :  4; // 4'b0000 - MC0TC0 4'b0001 - MC0TC1 4'b0010 - MC0TC2 4'b0011 - MC0TC3 4'b0100 - MC1TC0 4'b0101 - MC1TC1 4'b0110 - MC1TC2 4'b0111 - MC1TC3 4'b1xxx - Not occupied
        uint64_t               mc0tc1  :  4; // See below
        uint64_t               mc0tc2  :  4; // See below
        uint64_t               mc0tc3  :  4; // See below
        uint64_t               mc1tc0  :  4; // See below
        uint64_t               mc1tc1  :  4; // See below
        uint64_t               mc1tc2  :  4; // See below
        uint64_t               mc1tc3  :  4; // See below
        uint64_t              shared0  :  4; // See below
        uint64_t              shared1  :  4; // See below
        uint64_t              shared2  :  4; // See below
        uint64_t              shared3  :  4; // See below
        uint64_t              shared4  :  4; // See below
        uint64_t              shared5  :  4; // See below
        uint64_t              shared6  :  4; // See below
        uint64_t              shared7  :  4; // See below
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_FPE_CONTEXT_MCTC_MAP0_t;

// TXOTR_PKT_DBG_FPE_CONTEXT_MCTC_MAP1 desc:
typedef union {
    struct {
        uint64_t          mc0tc0_rsvd  :  4; // 4'b0000 - MC0TC0 4'b0001 - MC0TC1 4'b0010 - MC0TC2 4'b0011 - MC0TC3 4'b0100 - MC1TC0 4'b0101 - MC1TC1 4'b0110 - MC1TC2 4'b0111 - MC1TC3 4'b1xxx - Not occupied
        uint64_t          mc0tc1_rsvd  :  4; // See below
        uint64_t          mc0tc2_rsvd  :  4; // See below
        uint64_t          mc0tc3_rsvd  :  4; // See below
        uint64_t          mc1tc0_rsvd  :  4; // See below
        uint64_t          mc1tc1_rsvd  :  4; // See below
        uint64_t          mc1tc2_rsvd  :  4; // See below
        uint64_t          mc1tc3_rsvd  :  4; // See below
        uint64_t         UNUSED_63_32  : 32; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_FPE_CONTEXT_MCTC_MAP1_t;

// TXOTR_PKG_DBG_MC0_OUTSTANDING_PKT_CNT desc:
typedef union {
    struct {
        uint64_t                  tc0  : 16; // TC0 outstanding packet count.
        uint64_t                  tc1  : 16; // TC1 outstanding packet count.
        uint64_t                  tc2  : 16; // TC2 outstanding packet count.
        uint64_t                  tc3  : 16; // TC3 outstanding packet count.
    } field;
    uint64_t val;
} TXOTR_PKG_DBG_MC0_OUTSTANDING_PKT_CNT_t;

// TXOTR_PKG_DBG_MC1_OUTSTANDING_PKT_CNT desc:
typedef union {
    struct {
        uint64_t                  tc0  : 16; // TC0 outstanding packet count.
        uint64_t                  tc1  : 16; // TC1 outstanding packet count.
        uint64_t                  tc2  : 16; // TC2 outstanding packet count.
        uint64_t                  tc3  : 16; // TC3 outstanding packet count.
    } field;
    uint64_t val;
} TXOTR_PKG_DBG_MC1_OUTSTANDING_PKT_CNT_t;

// TXOTR_PKG_DBG_TO_NACK_MASK desc:
typedef union {
    struct {
        uint64_t                Index  :  8; // Index into the mask. Attempts to write values larger than 131 are ignored.
        uint64_t               enable  :  1; // Enable. Software must clear this bit when it wishes to issue another write to the mask.
        uint64_t          UNUSED_63_9  : 55; // Unused
    } field;
    uint64_t val;
} TXOTR_PKG_DBG_TO_NACK_MASK_t;

// TXOTR_PKG_DBG_PENDING_RETRANSMIT desc:
typedef union {
    struct {
        uint64_t                Index  :  6; // Index into the sidecar.
        uint64_t               enable  :  1; // Enable. Software must clear this bit when it wishes to issue another write to the sidecar.
        uint64_t          UNUSED_63_7  : 57; // Unused
    } field;
    uint64_t val;
} TXOTR_PKG_DBG_PENDING_RETRANSMIT_t;

// TXOTR_PKG_DBG_OOS_NACK_MASK desc:
typedef union {
    struct {
        uint64_t                Index  :  6; // Index into the mask.
        uint64_t               enable  :  1; // Enable. Software must clear this bit when it wishes to issue another write to the mask.
        uint64_t          UNUSED_63_7  : 57; // Unused
    } field;
    uint64_t val;
} TXOTR_PKG_DBG_OOS_NACK_MASK_t;

// TXOTR_PKG_DBG_HEAD_PENDING desc:
typedef union {
    struct {
        uint64_t                Index  :  8; // Index into the sidecar. Attempts to write values larger than 131 are ignored.
        uint64_t               enable  :  1; // Enable. Software must clear this bit when it wishes to issue another write to the sidecar.
        uint64_t          UNUSED_63_9  : 55; // Unused
    } field;
    uint64_t val;
} TXOTR_PKG_DBG_HEAD_PENDING_t;

// TXOTR_PKT_PRF_STALL_OPB_ENTRIES_X desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  3; // 3'd7 - MC1TC3 3'd6 - MC1TC2 3'd5 - MC1TC1 3'd4 - MC1TC0 3'd3 - MC0TC3 3'd2 - MC0TC2 3'd1 - MC0TC1 3'd0 - MC0TC0
        uint64_t          UNUSED_63_3  : 61; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_PRF_STALL_OPB_ENTRIES_X_t;

// TXOTR_PKT_PRF_STALL_OPB_ENTRIES_Y desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  3; // 3'd7 - MC1TC3 3'd6 - MC1TC2 3'd5 - MC1TC1 3'd4 - MC1TC0 3'd3 - MC0TC3 3'd2 - MC0TC2 3'd1 - MC0TC1 3'd0 - MC0TC0
        uint64_t          UNUSED_63_3  : 61; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_PRF_STALL_OPB_ENTRIES_Y_t;

// TXOTR_PKT_PRF_STALL_TXDMA_CREDITS_X desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  3; // 3'd7 - MC1TC3 3'd6 - MC1TC2 3'd5 - MC1TC1 3'd4 - MC1TC0 3'd3 - MC0TC3 3'd2 - MC0TC2 3'd1 - MC0TC1 3'd0 - MC0TC0
        uint64_t          UNUSED_63_3  : 61; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_PRF_STALL_TXDMA_CREDITS_X_t;

// TXOTR_PKT_PRF_STALL_TXDMA_CREDITS_Y desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  3; // 3'd7 - MC1TC3 3'd6 - MC1TC2 3'd5 - MC1TC1 3'd4 - MC1TC0 3'd3 - MC0TC3 3'd2 - MC0TC2 3'd1 - MC0TC1 3'd0 - MC0TC0
        uint64_t          UNUSED_63_3  : 61; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_PRF_STALL_TXDMA_CREDITS_Y_t;

// TXOTR_PKT_PRF_STALL_P_TO_M_CREDITS_X desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  2; // 2'd3 - MC1TC3 2'd2 - MC1TC2 2'd1 - MC1TC1 2'd0 - MC1TC0
        uint64_t          UNUSED_63_2  : 62; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_PRF_STALL_P_TO_M_CREDITS_X_t;

// TXOTR_PKT_PRF_STALL_P_TO_M_CREDITS_Y desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  2; // 2'd3 - MC1TC3 2'd2 - MC1TC2 2'd1 - MC1TC1 2'd0 - MC1TC0
        uint64_t          UNUSED_63_2  : 62; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_PRF_STALL_P_TO_M_CREDITS_Y_t;

// TXOTR_PKT_PRF_LATENCY desc:
typedef union {
    struct {
        uint64_t                A_MAX  : 12; // Upper bound of lowest latency bin. Response latencies less than or equal to A_MAX increment bin A. This value should always be less than B_MAX .
        uint64_t                B_MAX  : 12; // Upper bound of second lowest latency bin. Response latencies greater than A_MAX but less than or equal to B_MAX increment bin B. This value should always be less than C_MAX .
        uint64_t                C_MAX  : 12; // Upper bound of third lowest latency bin.Response latencies greater than B_MAX but less than or equal to C_MAX increment bin B. This value should always be less than D_MAX .
        uint64_t                D_MAX  : 12; // Upper bound of fourth lowest latency bin.Response latencies greater than C_MAX but less than or equal to D_MAX increment bin B. Response latencies greater than D_MAX increment bin E.
        uint64_t         UNUSED_63_48  : 16; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_PRF_LATENCY_t;

// TXOTR_PKT_PRF_PKTS_OPENED_X desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  3; // 3'd7 - MC1TC3 3'd6 - MC1TC2 3'd5 - MC1TC1 3'd4 - MC1TC0 3'd3 - MC0TC3 3'd2 - MC0TC2 3'd1 - MC0TC1 3'd0 - MC0TC0
        uint64_t          UNUSED_63_3  : 61; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_PRF_PKTS_OPENED_X_t;

// TXOTR_PKT_PRF_PKTS_CLOSED_X desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  3; // 3'd7 - MC1TC3 3'd6 - MC1TC2 3'd5 - MC1TC1 3'd4 - MC1TC0 3'd3 - MC0TC3 3'd2 - MC0TC2 3'd1 - MC0TC1 3'd0 - MC0TC0
        uint64_t          UNUSED_63_3  : 61; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_PRF_PKTS_CLOSED_X_t;

#endif /* ___FXR_tx_otr_pkt_top_CSRS_H__ */