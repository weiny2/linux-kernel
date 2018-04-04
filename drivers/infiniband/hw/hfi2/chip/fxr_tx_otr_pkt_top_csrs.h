// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Thu Mar 29 15:03:56 2018
//

#ifndef ___FXR_tx_otr_pkt_top_CSRS_H__
#define ___FXR_tx_otr_pkt_top_CSRS_H__

// TXOTR_PKT_CFG_RXE2E_CRDTS desc:
typedef union {
    struct {
        uint64_t                  tc0  :  5; // TC0 Input Queue Credits
        uint64_t         Reserved_7_5  :  3; // Unused
        uint64_t                  tc1  :  5; // TC1 Input Queue Credits
        uint64_t       Reserved_15_13  :  3; // Unused
        uint64_t                  tc2  :  5; // TC2 Input Queue Credits
        uint64_t       Reserved_23_21  :  3; // Unused
        uint64_t                  tc3  :  5; // TC3 Input Queue Credits
        uint64_t       Reserved_63_29  : 35; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_RXE2E_CRDTS_t;

// TXOTR_PKT_CFG_MEMORY_SCHEDULER desc:
typedef union {
    struct {
        uint64_t      txci_rsp_rspack  :  6; // TXCI to TXOTR Response Response ACK
        uint64_t         Reserved_7_6  :  2; // Unused
        uint64_t      txci_rsp_reqack  :  6; // TXCI to TXOTR Response Request ACK
        uint64_t       Reserved_23_14  : 10; // Unused
        uint64_t     txdma_req_rspack  :  4; // TXDMA to TXOTR Request Response ACK
        uint64_t     txdma_req_reqack  :  4; // TXDMA to TXOTR Request Request ACK
        uint64_t            fpe_chint  :  2; // Cache hints for Fragmentation Programmable Engine reads from Host Memory.
        uint64_t     fpe_globalflowid  :  1; // Global Flow ID hint for Fragmentation Programmable Engine reads from Host Memory.
        uint64_t               fpe_so  :  1; // Strongly Ordered hint for Fragmentation Programmable Engine reads from Host Memory.
        uint64_t         psn_chint_rd  :  2; // Cache hints for PSN Cache reads to Host Memory.
        uint64_t   psn_chint_flush_wr  :  2; // Cache hints for PSN Cache flush writes to Host Memory.
        uint64_t  psn_chint_victim_wr  :  2; // Cache hints for PSN Cache victim writes to Host Memory.
        uint64_t     psn_globalflowid  :  1; // Global Flow ID hint for PSN Cache reads from Host Memory.
        uint64_t               psn_so  :  1; // Strongly Ordered hint for PSN Cache reads from Host Memory.
        uint64_t                  pid  : 12; // This is the PID inserted into Address Translation Requests from the PSN Cache
        uint64_t           priv_level  :  1; // The privilege level inserted into the Address Translation Requests from the PSN Cache. 0 = privileged, 1 = user
        uint64_t       Reserved_63_57  :  7; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_MEMORY_SCHEDULER_t;

// TXOTR_PKT_CFG_PKTID_1 desc:
typedef union {
    struct {
        uint64_t         rsvd_mc0_tc0  :  7; // The number of MC0/TC0 Reserved Packet Identifiers
        uint64_t         rsvd_mc0_tc1  :  7; // The number of MC0/TC1 Reserved Packet Identifiers
        uint64_t         rsvd_mc0_tc2  :  7; // The number of MC0/TC2 Reserved Packet Identifiers
        uint64_t         rsvd_mc0_tc3  :  7; // The number of MC0/TC3 Reserved Packet Identifiers
        uint64_t         rsvd_mc1_tc0  :  7; // The number of MC1/TC0 Reserved Packet Identifiers
        uint64_t         rsvd_mc1_tc1  :  7; // The number of MC1/TC1 Reserved Packet Identifiers
        uint64_t         rsvd_mc1_tc2  :  7; // The number of MC1/TC2 Reserved Packet Identifiers
        uint64_t         rsvd_mc1_tc3  :  7; // The number of MC1/TC3 Reserved Packet Identifiers
        uint64_t       Reserved_63_56  :  8; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_PKTID_1_t;

// TXOTR_PKT_CFG_PKTID_2 desc:
typedef union {
    struct {
        uint64_t               shared  : 14; // The number of Shared Packet Identifiers
        uint64_t       Reserved_63_14  : 50; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_PKTID_2_t;

// TXOTR_PKT_CFG_OPB_FIFO_GEN_ARB desc:
typedef union {
    struct {
        uint64_t  retransmit_priority  :  1; // If set, Retransmitted packets get priority over all other packets being transmitted, including Pending ACKs
        uint64_t pending_ack_priority  :  1; // If set, Pending ACK packets get priority over all other packets being transmitted, except for Retransmitted packets.
        uint64_t        Reserved_63_2  : 62; // TODO
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_GEN_ARB_t;

// TXOTR_PKT_CFG_PKT_DONE_CRDTS desc:
typedef union {
    struct {
        uint64_t               mc0tc0  :  4; // Configured credits for MC0TC0
        uint64_t         Reserved_7_4  :  4; // Unused
        uint64_t               mc0tc1  :  4; // Configured credits for MC0TC1
        uint64_t       Reserved_15_12  :  4; // Unused
        uint64_t               mc0tc2  :  4; // Configured credits for MC0TC2
        uint64_t       Reserved_23_20  :  4; // Unused
        uint64_t               mc0tc3  :  4; // Configured credits for MC0TC3
        uint64_t       Reserved_31_28  :  4; // Unused
        uint64_t               mc1tc0  :  4; // Configured credits for MC1TC0
        uint64_t       Reserved_39_36  :  4; // Unused
        uint64_t               mc1tc1  :  4; // Configured credits for MC1TC1
        uint64_t       Reserved_47_44  :  4; // Unused
        uint64_t               mc1tc2  :  4; // Configured credits for MC1TC2
        uint64_t       Reserved_55_52  :  4; // Unused
        uint64_t               mc1tc3  :  4; // Configured credits for MC1TC3
        uint64_t       Reserved_63_60  :  4; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_PKT_DONE_CRDTS_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC0TC0 desc:
typedef union {
    struct {
        uint64_t             bw_limit  : 16; // Bandwidth limit
        uint64_t        leak_fraction  :  8; // Fractional portion of leak amount
        uint64_t         leak_integer  :  3; // Integral portion of leak amount
        uint64_t       Reserved_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC0TC0_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC0TC1 desc:
typedef union {
    struct {
        uint64_t             bw_limit  : 16; // Bandwidth limit
        uint64_t        leak_fraction  :  8; // Fractional portion of leak amount
        uint64_t         leak_integer  :  3; // Integral portion of leak amount
        uint64_t       Reserved_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC0TC1_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC0TC2 desc:
typedef union {
    struct {
        uint64_t             bw_limit  : 16; // Bandwidth limit
        uint64_t        leak_fraction  :  8; // Fractional portion of leak amount
        uint64_t         leak_integer  :  3; // Integral portion of leak amount
        uint64_t       Reserved_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC0TC2_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC0TC3 desc:
typedef union {
    struct {
        uint64_t             bw_limit  : 16; // Bandwidth limit
        uint64_t        leak_fraction  :  8; // Fractional portion of leak amount
        uint64_t         leak_integer  :  3; // Integral portion of leak amount
        uint64_t       Reserved_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC0TC3_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC1TC0 desc:
typedef union {
    struct {
        uint64_t             bw_limit  : 16; // Bandwidth limit
        uint64_t        leak_fraction  :  8; // Fractional portion of leak amount
        uint64_t         leak_integer  :  3; // Integral portion of leak amount
        uint64_t       Reserved_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC1TC0_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC1TC1 desc:
typedef union {
    struct {
        uint64_t             bw_limit  : 16; // Bandwidth limit
        uint64_t        leak_fraction  :  8; // Fractional portion of leak amount
        uint64_t         leak_integer  :  3; // Integral portion of leak amount
        uint64_t       Reserved_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC1TC1_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC1TC2 desc:
typedef union {
    struct {
        uint64_t             bw_limit  : 16; // Bandwidth limit
        uint64_t        leak_fraction  :  8; // Fractional portion of leak amount
        uint64_t         leak_integer  :  3; // Integral portion of leak amount
        uint64_t       Reserved_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC1TC2_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC1TC3 desc:
typedef union {
    struct {
        uint64_t             bw_limit  : 16; // Bandwidth limit
        uint64_t        leak_fraction  :  8; // Fractional portion of leak amount
        uint64_t         leak_integer  :  3; // Integral portion of leak amount
        uint64_t       Reserved_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC1TC3_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC0TC0 desc:
typedef union {
    struct {
        uint64_t             bw_limit  : 16; // Bandwidth limit
        uint64_t        leak_fraction  :  8; // Fractional portion of leak amount
        uint64_t         leak_integer  :  3; // Integral portion of leak amount
        uint64_t       Reserved_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC0TC0_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC0TC1 desc:
typedef union {
    struct {
        uint64_t             bw_limit  : 16; // Bandwidth limit
        uint64_t        leak_fraction  :  8; // Fractional portion of leak amount
        uint64_t         leak_integer  :  3; // Integral portion of leak amount
        uint64_t       Reserved_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC0TC1_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC0TC2 desc:
typedef union {
    struct {
        uint64_t             bw_limit  : 16; // Bandwidth limit
        uint64_t        leak_fraction  :  8; // Fractional portion of leak amount
        uint64_t         leak_integer  :  3; // Integral portion of leak amount
        uint64_t       Reserved_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC0TC2_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC0TC3 desc:
typedef union {
    struct {
        uint64_t             bw_limit  : 16; // Bandwidth limit
        uint64_t        leak_fraction  :  8; // Fractional portion of leak amount
        uint64_t         leak_integer  :  3; // Integral portion of leak amount
        uint64_t       Reserved_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC0TC3_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC1TC0 desc:
typedef union {
    struct {
        uint64_t             bw_limit  : 16; // Bandwidth limit
        uint64_t        leak_fraction  :  8; // Fractional portion of leak amount
        uint64_t         leak_integer  :  3; // Integral portion of leak amount
        uint64_t       Reserved_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC1TC0_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC1TC1 desc:
typedef union {
    struct {
        uint64_t             bw_limit  : 16; // Bandwidth limit
        uint64_t        leak_fraction  :  8; // Fractional portion of leak amount
        uint64_t         leak_integer  :  3; // Integral portion of leak amount
        uint64_t       Reserved_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC1TC1_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC1TC2 desc:
typedef union {
    struct {
        uint64_t             bw_limit  : 16; // Bandwidth limit
        uint64_t        leak_fraction  :  8; // Fractional portion of leak amount
        uint64_t         leak_integer  :  3; // Integral portion of leak amount
        uint64_t       Reserved_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC1TC2_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC1TC3 desc:
typedef union {
    struct {
        uint64_t             bw_limit  : 16; // Bandwidth limit
        uint64_t        leak_fraction  :  8; // Fractional portion of leak amount
        uint64_t         leak_integer  :  3; // Integral portion of leak amount
        uint64_t       Reserved_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC1TC3_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_MC0TC0 desc:
typedef union {
    struct {
        uint64_t             bw_limit  : 16; // Bandwidth limit
        uint64_t        leak_fraction  :  8; // Fractional portion of leak amount
        uint64_t         leak_integer  :  3; // Integral portion of leak amount
        uint64_t       Reserved_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_MC0TC0_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_MC0TC1 desc:
typedef union {
    struct {
        uint64_t             bw_limit  : 16; // Bandwidth limit
        uint64_t        leak_fraction  :  8; // Fractional portion of leak amount
        uint64_t         leak_integer  :  3; // Integral portion of leak amount
        uint64_t       Reserved_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_MC0TC1_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_MC0TC2 desc:
typedef union {
    struct {
        uint64_t             bw_limit  : 16; // Bandwidth limit
        uint64_t        leak_fraction  :  8; // Fractional portion of leak amount
        uint64_t         leak_integer  :  3; // Integral portion of leak amount
        uint64_t       Reserved_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_MC0TC2_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_MC0TC3 desc:
typedef union {
    struct {
        uint64_t             bw_limit  : 16; // Bandwidth limit
        uint64_t        leak_fraction  :  8; // Fractional portion of leak amount
        uint64_t         leak_integer  :  3; // Integral portion of leak amount
        uint64_t       Reserved_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_MC0TC3_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_MC1TC0 desc:
typedef union {
    struct {
        uint64_t             bw_limit  : 16; // Bandwidth limit
        uint64_t        leak_fraction  :  8; // Fractional portion of leak amount
        uint64_t         leak_integer  :  3; // Integral portion of leak amount
        uint64_t       Reserved_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_MC1TC0_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_MC1TC1 desc:
typedef union {
    struct {
        uint64_t             bw_limit  : 16; // Bandwidth limit
        uint64_t        leak_fraction  :  8; // Fractional portion of leak amount
        uint64_t         leak_integer  :  3; // Integral portion of leak amount
        uint64_t       Reserved_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_MC1TC1_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_MC1TC2 desc:
typedef union {
    struct {
        uint64_t             bw_limit  : 16; // Bandwidth limit
        uint64_t        leak_fraction  :  8; // Fractional portion of leak amount
        uint64_t         leak_integer  :  3; // Integral portion of leak amount
        uint64_t       Reserved_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_MC1TC2_t;

// TXOTR_PKT_CFG_OPB_FIFO_ARB_MC1TC3 desc:
typedef union {
    struct {
        uint64_t             bw_limit  : 16; // Bandwidth limit
        uint64_t        leak_fraction  :  8; // Fractional portion of leak amount
        uint64_t         leak_integer  :  3; // Integral portion of leak amount
        uint64_t       Reserved_63_27  : 37; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_OPB_FIFO_ARB_MC1TC3_t;

// TXOTR_PKT_CFG_MAX_SEQ_DIST desc:
typedef union {
    struct {
        uint64_t   threshold_distance  : 16; // Threshold Distance. Any Maximum Sequence Distance greater than or equal to threshold_distance is considered to be a 'large transmit window.' Any Maximum Sequence Distance less than threshold_distance is considered to be a 'small transmit window.'
        uint64_t        skid_distance  :  8; // Skid Distance. This is the maximum length of the pipeline in OTR between when traffic is stalled due to a Maximum Sequence Distance violation and when the Maximum Sequence Distance violation is checked.
        uint64_t       Reserved_63_24  : 40; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_MAX_SEQ_DIST_t;

// TXOTR_PKT_CFG_FORCE_RETRANS_PENDING desc:
typedef union {
    struct {
        uint64_t       local_sequence  : 64; // Force Local Sequences to be marked as Pending
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_FORCE_RETRANS_PENDING_t;

// TXOTR_PKT_CFG_MC1P_RC desc:
typedef union {
    struct {
        uint64_t            rendz_tc0  :  3; // Rendezvous TC 0 Routing Control Bits
        uint64_t           Reserved_3  :  1; // Unused
        uint64_t            rendz_tc1  :  3; // Rendezvous TC 1 Routing Control Bits
        uint64_t           Reserved_7  :  1; // Unused
        uint64_t            rendz_tc2  :  3; // Rendezvous TC 2 Routing Control Bits
        uint64_t          Reserved_11  :  1; // Unused
        uint64_t            rendz_tc3  :  3; // Rendezvous TC 3 Routing Control Bits
        uint64_t          Reserved_15  :  1; // Unused
        uint64_t             mc1p_tc0  :  3; // MC1p TC 0 Routing Control Bits
        uint64_t          Reserved_19  :  1; // Unused
        uint64_t             mc1p_tc1  :  3; // MC1p TC 1 Routing Control Bits
        uint64_t          Reserved_23  :  1; // Unused
        uint64_t             mc1p_tc2  :  3; // MC1p TC 2 Routing Control Bits
        uint64_t          Reserved_27  :  1; // Unused
        uint64_t             mc1p_tc3  :  3; // MC1p TC 3 Routing Control Bits
        uint64_t       Reserved_63_31  : 33; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_MC1P_RC_t;

// TXOTR_PKT_CFG_TIMEOUT desc:
typedef union {
    struct {
        uint64_t               scaler  : 32; // This register selects the rate at which the TXOTR Timeout Counter increments. If set to 32'd0, the counter is disabled. If set to 1, the counter increments once per clock cycle. If set to 2, the counter increments once every other clock cycle. Et cetera.
        uint64_t          max_retries  :  4; // Maximum number of retries allowed for a packet allocated in the Outstanding Packet Buffer. Note that setting this value to 4'd15 disables initial transmits of packets which exceed the Maximum Sequence Distance for a destination since those packets can be stored in the OPB with a max_retry value set to one greater than the configured value of max_retries .
        uint64_t              timeout  : 12; // Timeout comparison value. A timeout has occurred when the difference between the time stamp in a given Hash Table entry and the time current time stamp has exceeded this value.
        uint64_t disable_retrans_limit_unconnected  :  1; // Disable setting a connection state to Unconnected when a retransmit limit is reached.
        uint64_t     about_to_timeout  :  7; // Start dropping ACK/Reply for the entries which are about to timeout. Programmed value defines how many clock cycles before the timeout is considered as about to timeout. This value should be always be less than SCALER
        uint64_t       Reserved_63_56  :  8; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_TIMEOUT_t;

// TXOTR_PKT_CFG_SLID_PT0 desc:
typedef union {
    struct {
        uint64_t                 base  : 24; // Source Local Identifier Base
        uint64_t       Reserved_63_24  : 40; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_SLID_PT0_t;

// TXOTR_PKT_CFG_LMC desc:
typedef union {
    struct {
        uint64_t               lmc_p0  :  2; // Port 0 number of low SLID bits masked by network for packet delivery.
        uint64_t        Reserved_63_2  : 62; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_LMC_t;

// TXOTR_PKT_CFG_FRAG_ENG desc:
typedef union {
    struct {
        uint64_t        Reserved_15_0  : 16; // Unused
        uint64_t     max_dma_prefetch  : 16; // The maximum number of DMA Prefetches that can be issued by FPE.
        uint64_t     msg_ordering_off  :  1; // If this bit is set, do not check OMB Msg order fields against those of Msg's already in flight before sending it to Fragmentation Engine
        uint64_t       post_frag_prio  :  1; // If this bit is set, give priority to flits from the standard Post-fragmentation queues over the Post-fragmentation Retransmission queues.
        uint64_t start_authentication  :  1; // Set this bit to start firmware image authentication. Clear it before writing to the firmware CSR.
        uint64_t authentication_complete  :  1; // Indicates that authentication completed on the current firmware image.
        uint64_t authentication_successful  :  1; // Indicates that authentication was successful on the current firmware image.
        uint64_t          pd_rst_mask  :  1; // A value of zero mask off the freset assertion to the PD authentication logic. If this bit is set to '1' every assertion of 'freset' would need a PD re-authentication.
        uint64_t       Reserved_63_38  : 26; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_FRAG_ENG_t;

// TXOTR_PKT_CFG_VALID_TC_DLID desc:
typedef union {
    struct {
        uint64_t          max_dlid_p0  : 24; // Port 0 max valid dlid, incoming packets with dlid > than this are dropped and no PSN cache lookup is done. Note: dlid > max_dlid_p0 will abort CACHE_CMD_RD, CACHE_CMD_WR in TXOTR_PKT_CFG_PSN_CACHE_ACCESS_CTL and set bad_addr field in TXOTR_PKT_CFG_PSN_CACHE_ACCESS_CTL
        uint64_t          tc_valid_p0  :  4; // Port 0 TC valid bits, 1 bit per TC, invalid incoming TC packets are dropped and no PSN cache lookup is done. Note: Invalid tc will abort CACHE_CMD_RD, CACHE_CMD_WR in TXOTR_PKT_CFG_PSN_CACHE_ACCESS_CTL and set bad_addr field in TXOTR_PKT_CFG_PSN_CACHE_ACCESS_CTL
        uint64_t       Reserved_63_28  : 36; // Unused
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

// TXOTR_PKT_CFG_PSN_CACHE_HASH_SELECT desc:
typedef union {
    struct {
        uint64_t          hash_select  :  2; // PSN cache address hash select.
        uint64_t        Reserved_63_2  : 62; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_PSN_CACHE_HASH_SELECT_t;

// TXOTR_PKT_CFG_PSN_CACHE_ACCESS_CTL desc:
typedef union {
    struct {
        uint64_t              address  : 34; // The contents of this field differ based on cmd. psn_cache_addr_t for CACHE_CMD_RD, CACHE_CMD_WR,CACHE_CMD_INVALIDATE,CACHE_CMD_FLUSH* 10:0 used for CACHE_CMD_TAG_RD,CACHE_CMD_TAG_WR 14:0 used for CACHE_CMD_DATA_RD, CACHE_CMD_DATA_WR
        uint64_t                  cmd  :  4; // cache cmd. see <blue text>Section A.1.1, 'FXR Cache Cmd enums' initially CACHE_CMD_INVALIDATE
        uint64_t                 busy  :  1; // SW sets busy when writing this csr. HW clears busy when cmd is complete. busy must be clear before writing this csr. If busy is set, HW is busy on a previous cmd. Coming out of reset, busy will be 1 so as to initiate the psn cache tag invalidation. Then in 2k clks or so, it will go to 0.
        uint64_t             bad_addr  :  1; // if cmd == CACHE_CMD_RD or CACHE_CMD_WR, the address.tc is checked for valid and address.lid is checked for in range. See TXOTR_PKT_CFG_VALID_TC_DLID . The port used is address.port anded with use_port_in_psn_state field in TXOTR_PKT_CFG_VALID_TC_DLID . No other cache_cmd sets this bit. If the check fails, no psn cache lookup occurs and this bit gets set.
        uint64_t       Reserved_63_40  : 24; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_PSN_CACHE_ACCESS_CTL_t;

// TXOTR_PKT_CFG_PSN_CACHE_TAG_SEARCH_MASK desc:
typedef union {
    struct {
        uint64_t         mask_address  : 34; // cache cmd mask address. 1 bits are don't care. only used for CACHE_CMD_INVALIDATE, CACHE_CMD_FLUSH_INVALID, CACHE_CMD_FLUSH_VALID The form of this field is psn_cache_addr_t. initially all 1's. Note: If this field is 0, a normal single lookup is done. If non-zero, the entire cache tag memory is read and tag entries match/masked.
        uint64_t       Reserved_63_34  : 30; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_PSN_CACHE_TAG_SEARCH_MASK_t;

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
        uint64_t         tag_way_addr  : 34; // psn cache tag way address, format is psn_cache_addr_t
        uint64_t        tag_way_valid  :  1; // psn cache tag way valid
        uint64_t       Reserved_63_35  : 29; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_PSN_CACHE_ACCESS_TAG_t;

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

// TXOTR_PKT_CFG_LPSN_MAX_DIST desc:
typedef union {
    struct {
        uint64_t             max_dist  : 16; // This register determines the maximum distance between the oldest Local Packet Sequence Number and the current Local Packet Sequence Number. Under normal operating conditions, this field should never be set to a value greater than 16'd32768.
        uint64_t       Reserved_63_16  : 48; // Unused
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
        uint64_t       Reserved_63_15  : 49; // Unused
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

// TXOTR_PKT_CFG_TC0 desc:
typedef union {
    struct {
        uint64_t            max_bytes  : 63; // Maximum number of bytes allowed to be outstanding on this traffic class. The current outstanding for a traffic class is allowed to exceed the configured value by one MTU size
        uint64_t               enable  :  1; // Outstanding Maximum bytes tracking is enabled for this traffic class
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_TC0_t;

// TXOTR_PKT_CFG_TC1 desc:
typedef union {
    struct {
        uint64_t            max_bytes  : 63; // Maximum number of bytes allowed to be outstanding on this traffic class. The current outstanding for a traffic class is allowed to exceed the configured value by one MTU size
        uint64_t               enable  :  1; // Outstanding Maximum bytes tracking is enabled for this traffic class
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_TC1_t;

// TXOTR_PKT_CFG_TC2 desc:
typedef union {
    struct {
        uint64_t            max_bytes  : 63; // Maximum number of bytes allowed to be outstanding on this traffic class. The current outstanding for a traffic class is allowed to exceed the configured value by one MTU size
        uint64_t               enable  :  1; // traffic class is enabled
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_TC2_t;

// TXOTR_PKT_CFG_TC3 desc:
typedef union {
    struct {
        uint64_t            max_bytes  : 63; // Maximum number of bytes allowed to be outstanding on this traffic class. The current outstanding for a traffic class is allowed to exceed the configured value by one MTU size
        uint64_t               enable  :  1; // Outstanding Maximum bytes tracking is enabled for this traffic class
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_TC3_t;

// TXOTR_PKT_CFG_MLID_MASK desc:
typedef union {
    struct {
        uint64_t        mlid_mask_16b  :  7; // Multi cast LID Mask
        uint64_t        Reserved_63_7  : 57; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_MLID_MASK_t;

// TXOTR_PKT_CFG_PKEY_8B desc:
typedef union {
    struct {
        uint64_t                 pkey  : 15; // PKEY value
        uint64_t           membership  :  1; // Membership indicator
        uint64_t       Reserved_63_16  : 48; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_PKEY_8B_t;

// TXOTR_PKT_CFG_MTU desc:
typedef union {
    struct {
        uint64_t                  tc0  :  3; // The MTU for TC0
        uint64_t           Reserved_3  :  1; // Unused
        uint64_t                  tc1  :  3; // The MTU for TC1
        uint64_t           Reserved_7  :  1; // Unused
        uint64_t                  tc2  :  3; // The MTU for TC2
        uint64_t          Reserved_11  :  1; // Unused
        uint64_t                  tc3  :  3; // The MTU for TC3. 0: no packets allowed. 1: 384. 2: 640. 3: 1152. 4: 2176. 5: 4224. 6: 8320. 7: 10368.
        uint64_t       Reserved_63_15  : 49; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_MTU_t;

// TXOTR_PKT_PKEY_CHECK_DISABLE desc:
typedef union {
    struct {
        uint64_t   pkey_check_disable  :  1; // PKEY check is disabled when set. The pkey index is always 0
        uint64_t        Reserved_63_1  : 63; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_PKEY_CHECK_DISABLE_t;

// TXOTR_PKT_CFG_PTL_PKEY_TABLE desc:
typedef union {
    struct {
        uint64_t                 pkey  : 15; // PKEY value
        uint64_t           membership  :  1; // Membership indicator
        uint64_t       Reserved_63_16  : 48; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_PTL_PKEY_TABLE_t;

// TXOTR_PKT_CFG_PSN_BASE_ADDR_P0_TC desc:
typedef union {
    struct {
        uint64_t        Reserved_11_0  : 12; // Unused
        uint64_t              address  : 45; // Host Memory Base Address, aligned to page boundary
        uint64_t       Reserved_63_57  :  7; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_CFG_PSN_BASE_ADDR_P0_TC_t;

// TXOTR_PKT_ERR_STS_0 desc:
typedef union {
    struct {
        uint64_t       dlid_check_err  :  1; // DLID failure check in command. See txotr_pkt_err_info_dlid_check CSR for error information. ERR_CATEGORY_TRANSACTION
        uint64_t  invalid_msg_len_err  :  1; // Message length is longer than expected for the packet to be generated. See txotr_pkt_err_info_length_check CSR for error information. ERR_CATEGORY_TRANSACTION
        uint64_t          cmd_len_err  :  1; // Malformed length packet from TXCI. OTR checks the cmd_length against the tail location. See txotr_pkt_err_info_length_check CSR for error information. ERR_CATEGORY_HFI
        uint64_t          fp_fifo_mbe  :  1; // Multiple BIt Error for reads from the Fast Path fifo Input Queue. See txotr_pkt_err_info_fp_fifo CSR for error information. ERR_CATEGORY_HFI
        uint64_t          fp_fifo_sbe  :  1; // Single BIt Error for reads from the Fast Path fifo Input Queue. See txotr_pkt_err_info_fp_fifo CSR for error information.ERR_CATEGORY_CORRECTABLE
        uint64_t      fp_fifo_undflow  :  1; // Fast Path fifo Input Queue under flow error. See txotr_pkt_err_info_fp_fifo CSR for error information. ERR_CATEGORY_HFI
        uint64_t      fp_fifo_ovrflow  :  1; // Fast Path fifo Input Queue over flow error. See txotr_pkt_err_info_fp_fifo CSR for error information. ERR_CATEGORY_HFI
        uint64_t     retrans_fifo_mbe  :  1; // Multiple BIt Error for reads from the Retransmit fifo. See txotr_pkt_err_info_retrans_fifo_0 & txotr_pkt_err_info_retrans_fifo_1 CSR for error information. ERR_CATEGORY_HFI
        uint64_t     retrans_fifo_sbe  :  1; // Single BIt Error for reads from the Retransmit fifo. See txotr_pkt_err_info_retrans_fifo_0 & txotr_pkt_err_info_retrans_fifo_1 CSR for error information. ERR_CATEGORY_CORRECTABLE
        uint64_t retrans_fifo_undflow  :  1; // Retransmit fifo under flow error. See txotr_pkt_err_info_retrans_fifo_1 CSR for error information. ERR_CATEGORY_HFI
        uint64_t retrans_fifo_ovrflow  :  1; // Retransmit fifo over flow error. See txotr_pkt_err_info_retrans_fifo_1 CSR for error information. ERR_CATEGORY_HFI
        uint64_t    pre_frag_fifo_mbe  :  1; // Multiple BIt Error for reads from the Pre-frag queue fifo. See txotr_pkt_err_info_prefrag_fifo_0 & txotr_pkt_err_info_prefrag_fifo_1 CSR for error information. ERR_CATEGORY_HFI
        uint64_t    pre_frag_fifo_sbe  :  1; // Single BIt Error for reads from the Pre-frag queue fifo. See txotr_pkt_err_info_prefrag_fifo_0 & txotr_pkt_err_info_prefrag_fifo_1 CSR for error information. ERR_CATEGORY_CORRECTABLE
        uint64_t pre_frag_fifo_undflow  :  1; // Pre-frag queuet fifo under flow error. See txotr_pkt_err_info_prefrag_fifo_1 CSR for error information. ERR_CATEGORY_HFI
        uint64_t pre_frag_fifo_ovrflow  :  1; // Pre-frag queue fifo over flow error. See txotr_pkt_err_info_prefrag_fifo_1 CSR for error information. ERR_CATEGORY_HFI
        uint64_t   post_frag_fifo_mbe  :  1; // Multiple BIt Error for reads from the Post-frag queue fifo. See txotr_pkt_err_info_postfrag_fifo_0 & txotr_pkt_err_info_postfrag_fifo_1 CSR for error information. ERR_CATEGORY_HFI
        uint64_t   post_frag_fifo_sbe  :  1; // Single BIt Error for reads from the Post-frag queue fifo. See txotr_pkt_err_info_postfrag_fifo_0 & txotr_pkt_err_info_postfrag_fifo_1 CSR for error information. ERR_CATEGORY_CORRECTABLE
        uint64_t post_frag_fifo_undflow  :  1; // Post-frag queuet fifo under flow error. See txotr_pkt_err_info_postfrag_fifo_1 CSR for error information. ERR_CATEGORY_HFI
        uint64_t post_frag_fifo_ovrflow  :  1; // Post-frag queue fifo over flow error. See txotr_pkt_err_info_postfrag_fifo_1 CSR for error information. ERR_CATEGORY_HFI
        uint64_t      rx_rsp_fifo_mbe  :  1; // Multiple BIt Error for reads from the RX RSP queue fifo for Reply/Acks received. See txotr_pkt_err_info_rx_rsp_fifo_0 & txotr_pkt_err_info_rx_rsp_fifo_1 CSR for error information. ERR_CATEGORY_HFI
        uint64_t      rx_rsp_fifo_sbe  :  1; // Single BIt Error for reads from the RX RSP fifo for Reply/Acks received. See txotr_pkt_err_info_rx_rsp_fifo_0 & txotr_pkt_err_info_rx_rsp_fifo_1 CSR for error information. ERR_CATEGORY_CORRECTABLE
        uint64_t  rx_rsp_fifo_undflow  :  1; // RX RSP FIFO underflow. See txotr_pkt_err_info_rx_rsp_fifo_1 CSR for error information. ERR_CATEGORY_HFI
        uint64_t rx_rsp_fifo_overflow  :  1; // RX RSP FIFO overflow. See txotr_pkt_err_info_rx_rsp_fifo_1 CSR for error information. ERR_CATEGORY_HFI
        uint64_t opb_state_fifo_undflow  :  1; // OPB state Fifo underflow. See txotr_pkt_err_info_opb_fifo CSR for error information. ERR_CATEGORY_HFI
        uint64_t opb_state_fifo_overflow  :  1; // OPB state Fifo overflow. See txotr_pkt_err_info_opb_fifo CSR for error information. ERR_CATEGORY_HFI
        uint64_t opb_handle_fifo_undflow  :  1; // OPB Handle fifo underflow. See txotr_pkt_err_info_opb_fifo CSR for error information. ERR_CATEGORY_HFI
        uint64_t opb_handle_fifo_overflow  :  1; // OPB Handle fifo overflow. See txotr_pkt_err_info_opb_fifo CSR for error information. ERR_CATEGORY_HFI
        uint64_t opb_pkt_sts_fifo_undflow  :  1; // OPB PKT Status fifo underflow. See txotr_pkt_err_info_opb_fifo CSR for error information. ERR_CATEGORY_HFI
        uint64_t opb_pkt_sts_fifo_overflow  :  1; // OPB PKT Status fifo overflow. See txotr_pkt_err_info_opb_fifo CSR for error information. ERR_CATEGORY_HFI
        uint64_t     pkt_done_undflow  :  1; // Packet done fifo under flow error. See txotr_pkt_err_info_opb_fifo CSR for error information. ERR_CATEGORY_HFI
        uint64_t    pkt_done_overflow  :  1; // Packet done fifo over flow error. See txotr_pkt_err_info_opb_fifo CSR for error information. ERR_CATEGORY_HFI
        uint64_t mc1p_pktid_list_fifo_undflow  :  1; // MC1P Packet ID List fifo under flow error. ERR_CATEGORY_HFI
        uint64_t mc1p_pktid_list_fifo_overflow  :  1; // MC1P Packet ID List fifo over flow error. ERR_CATEGORY_HFI
        uint64_t tx_psn_hold_fifo_undflow  :  1; // TX PSN hold fifo underflow. ERR_CATEGORY_HFI
        uint64_t tx_psn_hold_fifo_overflow  :  1; // TX PSN hold fifo overflow. ERR_CATEGORY_HFI
        uint64_t rx_psn_hold_fifo_undflow  :  1; // RX PSN hold fifo underflow. ERR_CATEGORY_HFI
        uint64_t rx_psn_hold_fifo_overflow  :  1; // RX PSN hold fifo overflow. ERR_CATEGORY_HFI
        uint64_t psn_work_fifo_undflow  :  1; // PSN work fifo underflow. ERR_CATEGORY_HFI
        uint64_t psn_work_fifo_overflow  :  1; // PSN work fifo overflow. ERR_CATEGORY_HFI
        uint64_t      tx_psn_fifo_mbe  :  1; // Multiple BIt Error for reads from the TX PSN FIFO. See txotr_pkt_err_info_tx_psn_fifo CSR for error information. ERR_CATEGORY_HFI
        uint64_t      tx_psn_fifo_sbe  :  1; // Single BIt Error for reads from the TX PSN FIFO. See txotr_pkt_err_info_tx_psn_fifo CSR for error information. ERR_CATEGORY_CORRECTABLE
        uint64_t  tx_psn_fifo_undflow  :  1; // TX PSN FIFO under flow error. ERR_CATEGORY_HFI
        uint64_t  tx_psn_fifo_ovrflow  :  1; // TX PSN FIFO over flow error. ERR_CATEGORY_HFI
        uint64_t  rx_psn_fifo_undflow  :  1; // RX PSN fifo under flow error. ERR_CATEGORY_HFI
        uint64_t rx_psn_fifo_overflow  :  1; // RX PSN fifo over flow error. ERR_CATEGORY_HFI
        uint64_t fpe_hi_rd_req_undflow  :  1; // FPE HI request fifo under flow error. ERR_CATEGORY_HFI
        uint64_t fpe_hi_rd_req_overflow  :  1; // FPE HI request fifo over flow error. ERR_CATEGORY_HFI
        uint64_t psn_hi_rd_req_undflow  :  1; // PSN cache HI request fifo under flow error. ERR_CATEGORY_HFI
        uint64_t psn_hi_rd_req_overflow  :  1; // PSN cache HI request fifo over flow error. ERR_CATEGORY_HFI
        uint64_t       iovec_buff_mbe  :  1; // IOVEC Buffer Space MBE Error Flag. See txotr_pkt_err_info_iovec_buff_be_0, txotr_pkt_err_info_iovec_buff_be_1 & txotr_pkt_err_info_iovec_buff_be_2 CSR for error information. ERR_CATEGORY_HFI
        uint64_t       iovec_buff_sbe  :  1; // IOVEC Buffer Space SBE Error Flag. See txotr_pkt_err_info_iovec_buff_be_0, txotr_pkt_err_info_iovec_buff_be_1 & txotr_pkt_err_info_iovec_buff_be_2 CSR for error information. ERR_CATEGORY_CORRECTABLE
        uint64_t              otm_mbe  :  1; // Outstanding Translation Memory MBE Error Flag. See txotr_pkt_err_info_otm_be CSR for error information. ERR_CATEGORY_HFI
        uint64_t              otm_sbe  :  1; // Outstanding Translation Memory SBE Error Flag. See txotr_pkt_err_info_otm_be CSR for error information. ERR_CATEGORY_CORRECTABLE
        uint64_t              opb_mbe  :  1; // Outstanding Packet Buffer MBE Error Flag. See See txotr_pkt_err_info_opb_be_0 , txotr_pkt_err_info_opb_be_1 , txotr_pkt_err_info_opb_be_2, txotr_pkt_err_info_opb_be_3, txotr_pkt_err_info_opb_be_4, txotr_pkt_err_info_opb_be_5 CSR for error information. ERR_CATEGORY_HFI
        uint64_t              opb_sbe  :  1; // Outstanding Packet Buffer SBE Error Flag. See See txotr_pkt_err_info_opb_be_0 , txotr_pkt_err_info_opb_be_1 , txotr_pkt_err_info_opb_be_2, txotr_pkt_err_info_opb_be_3, txotr_pkt_err_info_opb_be_4, txotr_pkt_err_info_opb_be_5 CSR for error information. ERR_CATEGORY_CORRECTABLE
        uint64_t       pktid_list_mbe  :  1; // Packet Identifier List MBE Error Flag. See txotr_pkt_err_info_pktid_list_be_0, txotr_pkt_err_info_pktid_list_be_1, txotr_pkt_err_info_pktid_list_be_2, txotr_pkt_err_info_pktid_list_be_3, txotr_pkt_err_info_pktid_list_be_4, txotr_pkt_err_info_pktid_list_be_5 & txotr_pkt_err_info_pktid_list_be_6 CSR for error information. ERR_CATEGORY_HFI
        uint64_t       pktid_list_sbe  :  1; // Packet Identifier List SBE Error Flag. See txotr_pkt_err_info_pktid_list_be_0, txotr_pkt_err_info_pktid_list_be_1, txotr_pkt_err_info_pktid_list_be_2, txotr_pkt_err_info_pktid_list_be_3, txotr_pkt_err_info_pktid_list_be_4, txotr_pkt_err_info_pktid_list_be_5 & txotr_pkt_err_info_pktid_list_be_6 CSR for error information. ERR_CATEGORY_CORRECTABLE
        uint64_t tx_new_ls_tail_wen_fifo_undflow  :  1; // TX New List Tail Write enable fifo underflow. ERR_CATEGORY_HFI
        uint64_t tx_new_ls_tail_wen_fifo_overflow  :  1; // TX New List Tail Write enable fifo overflow. ERR_CATEGORY_HFI
        uint64_t rx_update_fifo_undflow  :  1; // RX update fifo underflow. ERR_CATEGORY_HFI
        uint64_t rx_update_fifo_overflow  :  1; // RX update fifo overflow. ERR_CATEGORY_HFI
        uint64_t       hash_table_mbe  :  1; // Hash Table MBE Error Flag. See txotr_pkt_err_info_hash_table_be CSR for error information. ERR_CATEGORY_HFI
        uint64_t       Reserved_63_62  :  2; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_STS_0_t;

// TXOTR_PKT_ERR_CLR_0 desc:
typedef union {
    struct {
        uint64_t               events  : 62; // Write 1's to clear corresponding status bits.
        uint64_t       Reserved_63_62  :  2; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_CLR_0_t;

// TXOTR_PKT_ERR_FRC_0 desc:
typedef union {
    struct {
        uint64_t               events  : 62; // Write 1 to set corresponding status bits.
        uint64_t       Reserved_63_62  :  2; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_FRC_0_t;

// TXOTR_PKT_ERR_EN_HOST_0 desc:
typedef union {
    struct {
        uint64_t               events  : 62; // Enables corresponding status bits to generate host interrupt signal.
        uint64_t       Reserved_63_62  :  2; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_EN_HOST_0_t;

// TXOTR_PKT_ERR_FIRST_HOST_0 desc:
typedef union {
    struct {
        uint64_t               events  : 62; // Snapshot of status bits when host interrupt signal transitions from 0 to 1.
        uint64_t       Reserved_63_62  :  2; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_FIRST_HOST_0_t;

// TXOTR_PKT_ERR_EN_BMC_0 desc:
typedef union {
    struct {
        uint64_t               events  : 62; // Enables corresponding status bits to generate quarantine interrupt signal.
        uint64_t       Reserved_63_62  :  2; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_EN_BMC_0_t;

// TXOTR_PKT_ERR_FIRST_BMC_0 desc:
typedef union {
    struct {
        uint64_t               events  : 62; // Snapshot of status bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t       Reserved_63_62  :  2; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_FIRST_BMC_0_t;

// TXOTR_PKT_ERR_EN_QUAR_0 desc:
typedef union {
    struct {
        uint64_t               events  : 62; // Enables corresponding status bits to generate BMC interrupt signal.
        uint64_t       Reserved_63_62  :  2; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_EN_QUAR_0_t;

// TXOTR_PKT_ERR_FIRST_QUAR_0 desc:
typedef union {
    struct {
        uint64_t               events  : 62; // Snapshot of status bits when quarantine interrupt signal transitions from 0 to 1.
        uint64_t       Reserved_63_62  :  2; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_FIRST_QUAR_0_t;

// TXOTR_PKT_ERR_STS_1 desc:
typedef union {
    struct {
        uint64_t       hash_table_sbe  :  1; // Hash Table SBE Error Flag. See txotr_pkt_err_info_hash_table_be CSR for error information. ERR_CATEGORY_CORRECTABLE
        uint64_t tx_old_ls_tail_wen_fifo_undflow  :  1; // TX old List Tail Write enable fifo underflow. ERR_CATEGORY_HFI
        uint64_t tx_old_ls_tail_wen_fifo_overflow  :  1; // TX old List Tail Write enable fifo overflow. ERR_CATEGORY_HFI
        uint64_t tx_free_head_fifo_undflow  :  1; // TX free head fifo undflow. ERR_CATEGORY_HFI
        uint64_t tx_free_head_fifo_overflow  :  1; // TX free head fifo overflow. ERR_CATEGORY_HFI
        uint64_t tx_local_sequence_fifo_overflow  :  1; // TX local sequence fifo overflow. ERR_CATEGORY_HFI
        uint64_t tx_local_sequence_fifo_undflow  :  1; // TX local sequence fifo underflow. ERR_CATEGORY_HFI
        uint64_t    psn_cache_tag_mbe  :  1; // PSN Tag Cache MBE Error Flag. See txotr_pkt_err_info_psn_cache_tag_mbe CSR for error information. ERR_CATEGORY_HFI
        uint64_t    psn_cache_tag_sbe  :  1; // PSN Tag Cache SBE Error Flag. See txotr_pkt_err_info_psn_cache_tag_sbe CSR for error information. ERR_CATEGORY_CORRECTABLE
        uint64_t   psn_cache_data_mbe  :  1; // PSN Data Cache MBE Error Flag. See txotr_pkt_err_info_psn_cache_data_sbe_mbe CSR for error information. ERR_CATEGORY_HFI
        uint64_t   psn_cache_data_sbe  :  1; // PSN Data Cache SBE Error Flag. See txotr_pkt_err_info_psn_cache_data_sbe_mbe CSR for error information. ERR_CATEGORY_CORRECTABLE
        uint64_t psn_cache_client_fifo_undflow  :  1; // PSN cache Client fifo underflow. ERR_CATEGORY_HFI
        uint64_t psn_cache_client_fifo_overflow  :  1; // PSN cache Client fifo overflow. ERR_CATEGORY_HFI
        uint64_t           at_rsp_err  :  1; // Address Translation response contains an error. See txotr_pkt_err_info_at_rsp CSR for error information. ERR_CATEGORY_TRANSACTION
        uint64_t           hi_rsp_err  :  1; // HI Response read errors. ERR_CATEGORY_TRANSACTION
        uint64_t     unconnected_dlid  :  1; // Attempt to send a command to an unconnected node (unconnected DLID). See txotr_pkt_err_info_unconnected_dlid CSR for error information. ERR_CATEGORY_TRANSACTION
        uint64_t         pkt_done_itf  :  1; // Error code in pkt_done response packet. ERR_CATEGORY_INFO
        uint64_t         hash_timeout  :  1; // Entry at the head of the Hash Table has experienced a timeout. See txotr_pkt_err_info_hash_timeout CSR for error information. ERR_CATEGORY_INFO
        uint64_t unknown_e2e_cmd_rcvd  :  1; // Unknown packet type decoded on RXE2E interface. See txotr_pkt_err_info_tc_pktid_capture CSR for error information. ERR_CATEGORY_INFO
        uint64_t psn_max_dist_vio_err  :  1; // PSN maximum distance violation error. See txotr_pkt_err_info_tc_pktid_capture CSR for error information. ERR_CATEGORY_INFO
        uint64_t        oos_nack_rcvd  :  1; // Received an OOS nack. See txotr_pkt_err_info_tc_pktid_capture CSR for error information. ERR_CATEGORY_INFO
        uint64_t   resource_nack_rcvd  :  1; // Received an resource nack. See txotr_pkt_err_info_tc_pktid_capture CSR for error information. ERR_CATEGORY_INFO
        uint64_t non_retransmt_nack_rcvd  :  1; // Non-retransmit nack received. See txotr_pkt_err_info_tc_pktid_capture CSR for error information. ERR_CATEGORY_INFO
        uint64_t       tuple_mismatch  :  1; // Packet received from RXE2E which does not match one or more of the tuple fields in OPB. See txotr_pkt_err_info_tupple_mismatch CSR for error information. ERR_CATEGORY_INFO
        uint64_t tuple_mismatch_opptmistic  :  1; // Packet received from RXE2E which does not match one or more of the Tuple mismatch for opportunistic case. See txotr_pkt_err_info_tupple_mismatch CSR for error information.ERR_CATEGORY_INFO
        uint64_t     fpe_data_mem_mbe  :  1; // Fragmentation Programmable Engine Data Memory MBE Error Flag. See txotr_pkt_err_info_fpe_data_mem_be CSR for error information. ERR_CATEGORY_HFI
        uint64_t     fpe_data_mem_sbe  :  1; // Fragmentation Programmable Engine Data Memory SBE Error Flag. See txotr_pkt_err_info_fpe_data_mem_be CSR for error information. ERR_CATEGORY_CORRECTABLE
        uint64_t     fpe_prog_mem_mbe  :  1; // Fragmentation Programmable Engine Program Memory MBE Error Flag. See txotr_pkt_err_info_fpe_prog_mem_be CSR for error information. ERR_CATEGORY_HFI
        uint64_t     fpe_prog_mem_sbe  :  1; // Fragmentation Programmable Engine Program Memory SBE Error Flag. See txotr_pkt_err_info_fpe_prog_mem_be CSR for error information. ERR_CATEGORY_CORRECTABLE
        uint64_t         fpe_firmware  :  1; // Fragmentation Programmable Engine Error Flag. See txotr_pkt_err_info_fpe_firmware CSR for error information. ERR_CATEGORY_HFI
        uint64_t       fpe_authen_err  :  1; // Error while authenticating the firmware. ERR_CATEGORY_HFI
        uint64_t           hi_hdr_mbe  :  1; // Multiple Bit Error in response state from Host Memory Error Flag. See txotr_pkt_err_info_hi_hdr_be CSR for error information. ERR_CATEGORY_HFI
        uint64_t           hi_hdr_sbe  :  1; // Single Bit Error in response state from Host Memory Error Flag. See txotr_pkt_err_info_hi_hdr_be CSR for error information. ERR_CATEGORY_CORRECTABLE
        uint64_t          hi_data_mbe  :  1; // Multiple Bit Error in response state from Host Memory Error Flag. See txotr_pkt_err_info_hi_data_be_0 & txotr_pkt_err_info_hi_data_be_1 CSR for error information. ERR_CATEGORY_HFI
        uint64_t          hi_data_sbe  :  1; // Single Bit Error in response state from Host Memory Error Flag. See txotr_pkt_err_info_hi_data_be_0 & txotr_pkt_err_info_hi_data_be_1 CSR for error information. ERR_CATEGORY_CORRECTABLE
        uint64_t rx_hash_table_fifo_undflow  :  1; // RX hash table fifo underflow. ERR_CATEGORY_HFI
        uint64_t rx_hash_table_fifo_overflow  :  1; // RX hash table fifo overflow. ERR_CATEGORY_HFI
        uint64_t pktid_list_ren_fifo_undflow  :  1; // PKTID list read enable fifo underflow. See txotr_pkt_err_info_tc_pktid_capture CSR for error information. ERR_CATEGORY_HFI
        uint64_t pktid_list_ren_fifo_overflow  :  1; // PKTID list read enable fifo overflow. See txotr_pkt_err_info_tc_pktid_capture CSR for error information. ERR_CATEGORY_HFI
        uint64_t psn_state_fifo_undflow  :  1; // PSN State FIFO underflow. ERR_CATEGORY_HFI
        uint64_t psn_state_fifo_overflow  :  1; // PSN State FIFO overflow. ERR_CATEGORY_HFI
        uint64_t          max_mtu_vio  :  1; // Maximum MTU violation in the Packet Partition
        uint64_t        rx_pkt_status  :  1; // Receive error status detected on the RxE2E interface. See txotr_pkt_err_info_ CSR for error information. ERR_CATEGORY_TRANSACTION
        uint64_t     about_to_timeout  :  1; // Notify RXDMA through the OMB to drop the packet due to close to time-range. See txotr_pkt_err_info_ CSR for error information. ERR_CATEGORY_INFO
        uint64_t       Reserved_63_44  : 20; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_STS_1_t;

// TXOTR_PKT_ERR_CLR_1 desc:
typedef union {
    struct {
        uint64_t               events  : 44; // Write 1's to clear corresponding status bits.
        uint64_t       Reserved_63_44  : 20; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_CLR_1_t;

// TXOTR_PKT_ERR_FRC_1 desc:
typedef union {
    struct {
        uint64_t               events  : 44; // Write 1 to set corresponding status bits.
        uint64_t       Reserved_63_44  : 20; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_FRC_1_t;

// TXOTR_PKT_ERR_EN_HOST_1 desc:
typedef union {
    struct {
        uint64_t               events  : 44; // Enables corresponding status bits to generate host interrupt signal.
        uint64_t       Reserved_63_44  : 20; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_EN_HOST_1_t;

// TXOTR_PKT_ERR_FIRST_HOST_1 desc:
typedef union {
    struct {
        uint64_t               events  : 44; // Snapshot of status bits when host interrupt signal transitions from 0 to 1.
        uint64_t       Reserved_63_44  : 20; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_FIRST_HOST_1_t;

// TXOTR_PKT_ERR_EN_BMC_1 desc:
typedef union {
    struct {
        uint64_t               events  : 44; // Enables corresponding status bits to generate quarantine interrupt signal.
        uint64_t       Reserved_63_44  : 20; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_EN_BMC_1_t;

// TXOTR_PKT_ERR_FIRST_BMC_1 desc:
typedef union {
    struct {
        uint64_t               events  : 44; // Snapshot of status bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t       Reserved_63_44  : 20; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_FIRST_BMC_1_t;

// TXOTR_PKT_ERR_EN_QUAR_1 desc:
typedef union {
    struct {
        uint64_t               events  : 44; // Enables corresponding status bits to generate BMC interrupt signal.
        uint64_t       Reserved_63_44  : 20; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_EN_QUAR_1_t;

// TXOTR_PKT_ERR_FIRST_QUAR_1 desc:
typedef union {
    struct {
        uint64_t               events  : 44; // Snapshot of status bits when quarantine interrupt signal transitions from 0 to 1.
        uint64_t       Reserved_63_44  : 20; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_FIRST_QUAR_1_t;

// TXOTR_PKT_ERR_INFO_DLID_CHECK desc:
typedef union {
    struct {
        uint64_t                 mctc  :  4; // MCTC value on which this error was detected
        uint64_t             encoding  :  3; // Encoding for the dlid error Bit 0 - Uninitialized DLID Bit 1 - Permissive DLID Bit 2 - Multicast DLID
        uint64_t        Reserved_63_7  : 57; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_DLID_CHECK_t;

// TXOTR_PKT_ERR_INFO_LENGTH_CHECK desc:
typedef union {
    struct {
        uint64_t mctc_invalid_msg_len  :  4; // MCTC value on which this error was detected
        uint64_t        Reserved_31_4  : 28; // Unused
        uint64_t         mctc_cmd_len  :  4; // MCTC value on which this error was detected
        uint64_t       Reserved_63_36  : 28; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_LENGTH_CHECK_t;

// TXOTR_PKT_ERR_INFO_FP_FIFO_0 desc:
typedef union {
    struct {
        uint64_t       syndrome_sbe_0  :  8; // Syndrome for domain 0
        uint64_t       syndrome_sbe_1  :  8; // Syndrome for domain 1
        uint64_t       syndrome_sbe_2  :  8; // Syndrome for domain 2
        uint64_t       syndrome_sbe_3  :  8; // Syndrome for domain 3
        uint64_t       syndrome_mbe_0  :  8; // Syndrome for domain 0
        uint64_t       syndrome_mbe_1  :  8; // Syndrome for domain 1
        uint64_t       syndrome_mbe_2  :  8; // Syndrome for domain 2
        uint64_t       syndrome_mbe_3  :  8; // Syndrome for domain 3
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_FP_FIFO_0_t;

// TXOTR_PKT_ERR_INFO_FP_FIFO_1 desc:
typedef union {
    struct {
        uint64_t           domain_sbe  :  4; // Domain for the captured SBE error
        uint64_t           domain_mbe  :  4; // Domain for the captured MBE error
        uint64_t             mctc_sbe  :  4; // MCTC for the captured SBE error
        uint64_t             mctc_mbe  :  4; // MCTC for the captured MBE error
        uint64_t      domain_overflow  :  4; // MCTC for the captured overflow
        uint64_t     domain_underflow  :  4; // MCTC for the captured underflow
        uint64_t       Reserved_63_24  : 40; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_FP_FIFO_1_t;

// TXOTR_PKT_ERR_INFO_RETRANS_FIFO_0 desc:
typedef union {
    struct {
        uint64_t       syndrome_sbe_0  :  8; // Syndrome for domain 0
        uint64_t       syndrome_sbe_1  :  8; // Syndrome for domain 1
        uint64_t       syndrome_sbe_2  :  8; // Syndrome for domain 2
        uint64_t       syndrome_sbe_3  :  8; // Syndrome for domain 3
        uint64_t       syndrome_mbe_0  :  8; // Syndrome for domain 0
        uint64_t       syndrome_mbe_1  :  8; // Syndrome for domain 1
        uint64_t       syndrome_mbe_2  :  8; // Syndrome for domain 2
        uint64_t       syndrome_mbe_3  :  8; // Syndrome for domain 3
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_RETRANS_FIFO_0_t;

// TXOTR_PKT_ERR_INFO_RETRANS_FIFO_1 desc:
typedef union {
    struct {
        uint64_t           domain_sbe  :  4; // Domain for the captured SBE error
        uint64_t           domain_mbe  :  4; // Domain for the captured MBE error
        uint64_t             mctc_sbe  :  3; // MCTC for the captured SBE error
        uint64_t             mctc_mbe  :  3; // MCTC for the captured MBE error
        uint64_t      domain_overflow  :  4; // MCTC for the captured overflow
        uint64_t     domain_underflow  :  4; // MCTC for the captured underflow
        uint64_t       Reserved_63_22  : 42; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_RETRANS_FIFO_1_t;

// TXOTR_PKT_ERR_INFO_PREFRAG_FIFO_0 desc:
typedef union {
    struct {
        uint64_t       syndrome_sbe_0  :  8; // Syndrome for domain 0
        uint64_t       syndrome_sbe_1  :  8; // Syndrome for domain 1
        uint64_t       syndrome_sbe_2  :  8; // Syndrome for domain 2
        uint64_t       syndrome_sbe_3  :  8; // Syndrome for domain 3
        uint64_t       syndrome_mbe_0  :  8; // Syndrome for domain 0
        uint64_t       syndrome_mbe_1  :  8; // Syndrome for domain 1
        uint64_t       syndrome_mbe_2  :  8; // Syndrome for domain 2
        uint64_t       syndrome_mbe_3  :  8; // Syndrome for domain 3
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_PREFRAG_FIFO_0_t;

// TXOTR_PKT_ERR_INFO_PREFRAG_FIFO_1 desc:
typedef union {
    struct {
        uint64_t           domain_sbe  :  4; // Domain for the captured SBE error
        uint64_t           domain_mbe  :  4; // Domain for the captured MBE error
        uint64_t             mctc_sbe  :  3; // MCTC for the captured SBE error
        uint64_t             mctc_mbe  :  3; // MCTC for the captured MBE error
        uint64_t      domain_overflow  :  4; // MCTC for the captured overflow
        uint64_t     domain_underflow  :  4; // MCTC for the captured underflow
        uint64_t       Reserved_63_22  : 42; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_PREFRAG_FIFO_1_t;

// TXOTR_PKT_ERR_INFO_POSTFRAG_FIFO_0 desc:
typedef union {
    struct {
        uint64_t       syndrome_sbe_0  :  8; // Syndrome for domain 0
        uint64_t       syndrome_sbe_1  :  8; // Syndrome for domain 1
        uint64_t       syndrome_sbe_2  :  8; // Syndrome for domain 2
        uint64_t       syndrome_sbe_3  :  8; // Syndrome for domain 3
        uint64_t       syndrome_mbe_0  :  8; // Syndrome for domain 0
        uint64_t       syndrome_mbe_1  :  8; // Syndrome for domain 1
        uint64_t       syndrome_mbe_2  :  8; // Syndrome for domain 2
        uint64_t       syndrome_mbe_3  :  8; // Syndrome for domain 3
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_POSTFRAG_FIFO_0_t;

// TXOTR_PKT_ERR_INFO_POSTFRAG_FIFO_1 desc:
typedef union {
    struct {
        uint64_t           domain_sbe  :  4; // Domain for the captured SBE error
        uint64_t           domain_mbe  :  4; // Domain for the captured MBE error
        uint64_t             mctc_sbe  :  4; // MCTC for the captured SBE error
        uint64_t             mctc_mbe  :  4; // MCTC for the captured MBE error
        uint64_t      domain_overflow  :  4; // MCTC for the captured overflow
        uint64_t     domain_underflow  :  4; // MCTC for the captured underflow
        uint64_t       Reserved_63_24  : 40; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_POSTFRAG_FIFO_1_t;

// TXOTR_PKT_ERR_INFO_RX_RSP_FIFO_0 desc:
typedef union {
    struct {
        uint64_t       syndrome_sbe_0  :  8; // Syndrome for domain 0
        uint64_t       syndrome_sbe_1  :  8; // Syndrome for domain 1
        uint64_t       syndrome_sbe_2  :  8; // Syndrome for domain 2
        uint64_t       syndrome_sbe_3  :  8; // Syndrome for domain 3
        uint64_t       syndrome_mbe_0  :  8; // Syndrome for domain 0
        uint64_t       syndrome_mbe_1  :  8; // Syndrome for domain 1
        uint64_t       syndrome_mbe_2  :  8; // Syndrome for domain 2
        uint64_t       syndrome_mbe_3  :  8; // Syndrome for domain 3
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_RX_RSP_FIFO_0_t;

// TXOTR_PKT_ERR_INFO_RX_RSP_FIFO_1 desc:
typedef union {
    struct {
        uint64_t           domain_sbe  :  4; // Domain for the captured SBE error
        uint64_t           domain_mbe  :  4; // Domain for the captured MBE error
        uint64_t               tc_sbe  :  2; // TC for the captured SBE error
        uint64_t               tc_mbe  :  2; // TC for the captured MBE error
        uint64_t      domain_overflow  :  2; // TC for the captured overflow
        uint64_t     domain_underflow  :  2; // TC for the captured underflow
        uint64_t       Reserved_63_16  : 48; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_RX_RSP_FIFO_1_t;

// TXOTR_PKT_ERR_INFO_OPB_FIFO desc:
typedef union {
    struct {
        uint64_t   pkt_done_underflow  :  8; // Domain for the captured underflow error
        uint64_t    pkt_done_overflow  :  8; // Domain for the captured overflow error
        uint64_t opb_pkt_sts_underflow  :  4; // Domain for the captured underflow error
        uint64_t opb_pkt_sts_overflow  :  4; // Domain for the captured overflow error
        uint64_t opb_handle_underflow  :  4; // Domain for the captured underflow error
        uint64_t  opb_handle_overflow  :  4; // Domain for the captured overflow error
        uint64_t  opb_state_underflow  :  4; // Domain for the captured underflow error
        uint64_t   opb_state_overflow  :  4; // Domain for the captured overflow error
        uint64_t       Reserved_63_40  : 24; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_OPB_FIFO_t;

// TXOTR_PKT_ERR_INFO_TX_PSN_FIFO_0 desc:
typedef union {
    struct {
        uint64_t       syndrome_sbe_0  :  8; // Syndrome for domain 0
        uint64_t       syndrome_sbe_1  :  8; // Syndrome for domain 1
        uint64_t       syndrome_sbe_2  :  8; // Syndrome for domain 2
        uint64_t       syndrome_sbe_3  :  8; // Syndrome for domain 3
        uint64_t       syndrome_mbe_0  :  8; // Syndrome for domain 0
        uint64_t       syndrome_mbe_1  :  8; // Syndrome for domain 1
        uint64_t       syndrome_mbe_2  :  8; // Syndrome for domain 2
        uint64_t       syndrome_mbe_3  :  8; // Syndrome for domain 3
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_TX_PSN_FIFO_0_t;

// TXOTR_PKT_ERR_INFO_TX_PSN_FIFO_1 desc:
typedef union {
    struct {
        uint64_t           domain_sbe  :  4; // Domain for the captured SBE error
        uint64_t           domain_mbe  :  4; // Domain for the captured MBE error
        uint64_t        Reserved_63_8  : 56; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_TX_PSN_FIFO_1_t;

// TXOTR_PKT_ERR_INFO_IOVEC_BUFF_BE_0 desc:
typedef union {
    struct {
        uint64_t       syndrome_sbe_0  :  8; // Syndrome for domain 0
        uint64_t       syndrome_sbe_1  :  8; // Syndrome for domain 1
        uint64_t       syndrome_sbe_2  :  8; // Syndrome for domain 2
        uint64_t       syndrome_sbe_3  :  8; // Syndrome for domain 3
        uint64_t       syndrome_sbe_4  :  8; // Syndrome for domain 4
        uint64_t       syndrome_sbe_5  :  8; // Syndrome for domain 5
        uint64_t       syndrome_sbe_6  :  8; // Syndrome for domain 6
        uint64_t       syndrome_sbe_7  :  8; // Syndrome for domain 7
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_IOVEC_BUFF_BE_0_t;

// TXOTR_PKT_ERR_INFO_IOVEC_BUFF_BE_1 desc:
typedef union {
    struct {
        uint64_t       syndrome_mbe_0  :  8; // Syndrome for domain 0
        uint64_t       syndrome_mbe_1  :  8; // Syndrome for domain 1
        uint64_t       syndrome_mbe_2  :  8; // Syndrome for domain 2
        uint64_t       syndrome_mbe_3  :  8; // Syndrome for domain 3
        uint64_t       syndrome_mbe_4  :  8; // Syndrome for domain 4
        uint64_t       syndrome_mbe_5  :  8; // Syndrome for domain 5
        uint64_t       syndrome_mbe_6  :  8; // Syndrome for domain 6
        uint64_t       syndrome_mbe_7  :  8; // Syndrome for domain 7
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_IOVEC_BUFF_BE_1_t;

// TXOTR_PKT_ERR_INFO_IOVEC_BUFF_BE_2 desc:
typedef union {
    struct {
        uint64_t           domain_sbe  :  8; // Domain for the captured SBE error
        uint64_t           domain_mbe  :  8; // Domain for the captured MBE error
        uint64_t             addr_sbe  :  5; // Memory Address for the captured SBE error
        uint64_t             addr_mbe  :  5; // Memory Address for the captured MBE error
        uint64_t       Reserved_63_26  : 38; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_IOVEC_BUFF_BE_2_t;

// TXOTR_PKT_ERR_INFO_OTM_BE desc:
typedef union {
    struct {
        uint64_t          address_sbe  :  7; // Address for the captured SBE error
        uint64_t          address_mbe  :  7; // Address for the captured MBE error
        uint64_t       Reserved_15_14  :  2; // Unused
        uint64_t         syndrome_sbe  :  9; // Syndrome for the captured SBE error
        uint64_t         syndrome_mbe  :  9; // Syndrome for the captured MBE error
        uint64_t       Reserved_63_34  : 30; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_OTM_BE_t;

// TXOTR_PKT_ERR_INFO_OPB_BE_0 desc:
typedef union {
    struct {
        uint64_t       syndrome_mbe_0  :  8; // Syndrome for domain 0
        uint64_t       syndrome_mbe_1  :  8; // Syndrome for domain 1
        uint64_t       syndrome_mbe_2  :  8; // Syndrome for domain 2
        uint64_t       syndrome_mbe_3  :  8; // Syndrome for domain 3
        uint64_t       syndrome_mbe_4  :  8; // Syndrome for domain 4
        uint64_t       Reserved_63_40  : 24; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_OPB_BE_0_t;

// TXOTR_PKT_ERR_INFO_OPB_BE_1 desc:
typedef union {
    struct {
        uint64_t       syndrome_sbe_0  :  8; // Syndrome for domain 0
        uint64_t       syndrome_sbe_1  :  8; // Syndrome for domain 1
        uint64_t       syndrome_sbe_2  :  8; // Syndrome for domain 2
        uint64_t       syndrome_sbe_3  :  8; // Syndrome for domain 3
        uint64_t       syndrome_sbe_4  :  8; // Syndrome for domain 4
        uint64_t       Reserved_63_40  : 24; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_OPB_BE_1_t;

// TXOTR_PKT_ERR_INFO_OPB_BE_2 desc:
typedef union {
    struct {
        uint64_t           domain_sbe  : 20; // Domain for the captured SBE error
        uint64_t           domain_mbe  : 20; // Domain for the captured MBE error
        uint64_t       Reserved_63_40  : 24; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_OPB_BE_2_t;

// TXOTR_PKT_ERR_INFO_OPB_BE_3 desc:
typedef union {
    struct {
        uint64_t           info_sbe_0  : 14; // Info captured for bank 0
        uint64_t           info_sbe_1  : 14; // Info captured for bank 1
        uint64_t           info_sbe_2  : 14; // Info captured for bank 2
        uint64_t           info_sbe_3  : 14; // Info captured for bank 3
        uint64_t       Reserved_63_56  :  8; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_OPB_BE_3_t;

// TXOTR_PKT_ERR_INFO_OPB_BE_4 desc:
typedef union {
    struct {
        uint64_t           info_mbe_0  : 14; // Info captured for bank 0
        uint64_t           info_mbe_1  : 14; // Info captured for bank 1
        uint64_t           info_mbe_2  : 14; // Info captured for bank 2
        uint64_t           info_mbe_3  : 14; // Info captured for bank 3
        uint64_t       Reserved_63_56  :  8; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_OPB_BE_4_t;

// TXOTR_PKT_ERR_INFO_OPB_BE_5 desc:
typedef union {
    struct {
        uint64_t          msgid_sbe_0  : 16; // Info captured for bank 0
        uint64_t          msgid_sbe_1  : 16; // Info captured for bank 1
        uint64_t          msgid_sbe_2  : 16; // Info captured for bank 2
        uint64_t          msgid_sbe_3  : 16; // Info captured for bank 3
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_OPB_BE_5_t;

// TXOTR_PKT_ERR_INFO_PKTID_LIST_BE_0 desc:
typedef union {
    struct {
        uint64_t       syndrome_sbe_0  :  6; // Syndrome for domain 0
        uint64_t       syndrome_sbe_1  :  6; // Syndrome for domain 1
        uint64_t       syndrome_sbe_2  :  6; // Syndrome for domain 2
        uint64_t       syndrome_sbe_3  :  6; // Syndrome for domain 3
        uint64_t       syndrome_sbe_4  :  6; // Syndrome for domain 4
        uint64_t       syndrome_sbe_5  :  6; // Syndrome for domain 5
        uint64_t       syndrome_sbe_6  :  6; // Syndrome for domain 6
        uint64_t       syndrome_sbe_7  :  6; // Syndrome for domain 7
        uint64_t       Reserved_63_48  : 16; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_PKTID_LIST_BE_0_t;

// TXOTR_PKT_ERR_INFO_PKTID_LIST_BE_1 desc:
typedef union {
    struct {
        uint64_t       syndrome_mbe_0  :  6; // Syndrome for domain 0
        uint64_t       syndrome_mbe_1  :  6; // Syndrome for domain 1
        uint64_t       syndrome_mbe_2  :  6; // Syndrome for domain 2
        uint64_t       syndrome_mbe_3  :  6; // Syndrome for domain 3
        uint64_t       syndrome_mbe_4  :  6; // Syndrome for domain 4
        uint64_t       syndrome_mbe_5  :  6; // Syndrome for domain 5
        uint64_t       syndrome_mbe_6  :  6; // Syndrome for domain 6
        uint64_t       syndrome_mbe_7  :  6; // Syndrome for domain 7
        uint64_t       Reserved_63_48  : 16; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_PKTID_LIST_BE_1_t;

// TXOTR_PKT_ERR_INFO_PKTID_LIST_BE_2 desc:
typedef union {
    struct {
        uint64_t           domain_sbe  :  8; // Domain for the captured SBE error
        uint64_t           domain_mbe  :  8; // Domain for the captured MBE error
        uint64_t       Reserved_63_16  : 48; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_PKTID_LIST_BE_2_t;

// TXOTR_PKT_ERR_INFO_PKTID_LIST_BE_3 desc:
typedef union {
    struct {
        uint64_t        address_sbe_0  : 14; // Address for the captured SBE
        uint64_t        address_sbe_1  : 14; // Address for the captured SBE
        uint64_t        address_sbe_2  : 14; // Address for the captured SBE
        uint64_t        address_sbe_3  : 14; // Address for the captured SBE
        uint64_t       Reserved_63_56  :  8; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_PKTID_LIST_BE_3_t;

// TXOTR_PKT_ERR_INFO_PKTID_LIST_BE_4 desc:
typedef union {
    struct {
        uint64_t        address_sbe_4  : 14; // Address for the captured SBE
        uint64_t        address_sbe_5  : 14; // Address for the captured SBE
        uint64_t        address_sbe_6  : 14; // Address for the captured SBE
        uint64_t        address_sbe_7  : 14; // Address for the captured SBE
        uint64_t       Reserved_63_56  :  8; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_PKTID_LIST_BE_4_t;

// TXOTR_PKT_ERR_INFO_PKTID_LIST_BE_5 desc:
typedef union {
    struct {
        uint64_t        address_mbe_0  : 14; // Address for the captured SBE
        uint64_t        address_mbe_1  : 14; // Address for the captured SBE
        uint64_t        address_mbe_2  : 14; // Address for the captured SBE
        uint64_t        address_mbe_3  : 14; // Address for the captured SBE
        uint64_t       Reserved_63_56  :  8; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_PKTID_LIST_BE_5_t;

// TXOTR_PKT_ERR_INFO_PKTID_LIST_BE_6 desc:
typedef union {
    struct {
        uint64_t        address_mbe_4  : 14; // Address for the captured SBE
        uint64_t        address_mbe_5  : 14; // Address for the captured SBE
        uint64_t        address_mbe_6  : 14; // Address for the captured SBE
        uint64_t        address_mbe_7  : 14; // Address for the captured SBE
        uint64_t       Reserved_63_56  :  8; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_PKTID_LIST_BE_6_t;

// TXOTR_PKT_ERR_INFO_HASH_TABLE_BE desc:
typedef union {
    struct {
        uint64_t          address_sbe  :  8; // Address for the captured SBE error
        uint64_t          address_mbe  :  8; // Address for the captured MBE error
        uint64_t         syndrome_sbe  :  8; // Syndrome for the captured SBE error
        uint64_t         syndrome_mbe  :  8; // Syndrome for the captured SBE error
        uint64_t       Reserved_63_32  : 32; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_HASH_TABLE_BE_t;

// TXOTR_PKT_ERR_INFO_PSN_CACHE_TAG_MBE desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain mbe
        uint64_t      mbe_last_domain  :  3; // ecc domain of last least significant mbe
        uint64_t                  mbe  :  8; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            mbe_count  :  4; // saturating counter of mbes. The increment signal is the 'or' of the 8 mbe signals.
        uint64_t     mbe_last_address  : 11; // address of the last least significant ecc domain mbe
        uint64_t       Reserved_63_34  : 30; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_PSN_CACHE_TAG_MBE_t;

// TXOTR_PKT_ERR_INFO_PSN_CACHE_TAG_SBE desc:
typedef union {
    struct {
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain sbe
        uint64_t      sbe_last_domain  :  3; // ecc domain of last least significant sbe
        uint64_t                  sbe  :  8; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            sbe_count  : 12; // saturating counter of sbes. The increment signal is the 'or' of the 8 sbe signals.
        uint64_t     sbe_last_address  : 11; // address of the last least significant ecc domain sbe
        uint64_t       Reserved_63_42  : 22; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_PSN_CACHE_TAG_SBE_t;

// TXOTR_PKT_ERR_INFO_PSN_CACHE_DATA_SBE_MBE desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain mbe
        uint64_t            mbe_count  :  4; // saturating counter of mbes.
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain sbe
        uint64_t            sbe_count  : 12; // saturating counter of sbes.
        uint64_t       Reserved_63_32  : 32; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_PSN_CACHE_DATA_SBE_MBE_t;

// TXOTR_PKT_ERR_INFO_AT_RSP desc:
typedef union {
    struct {
        uint64_t             encoding  :  4; // Error encoding
        uint64_t                  tid  : 10; // TID value captured for the transaction
        uint64_t       Reserved_63_14  : 50; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_AT_RSP_t;

// TXOTR_PKT_ERR_INFO_HI_RSP desc:
typedef union {
    struct {
        uint64_t             encoding  :  4; // Error encoding
        uint64_t                  tid  : 12; // TID value captured for the transaction
        uint64_t       Reserved_63_16  : 48; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_HI_RSP_t;

// TXOTR_PKT_ERR_INFO_UNCONNECTED_DLID desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  4; // Captured MCTC value
        uint64_t                 DLID  : 24; // Captured DLID value
        uint64_t       Reserved_63_28  : 36; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_UNCONNECTED_DLID_t;

// TXOTR_PKT_ERR_INFO_PKT_DONE desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  3; // MCTC value
        uint64_t             encoding  :  4; // Error encoding
        uint64_t                pktid  : 16; // Packet Id
        uint64_t       Reserved_63_23  : 41; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_PKT_DONE_t;

// TXOTR_PKT_ERR_INFO_HASH_TIMEOUT desc:
typedef union {
    struct {
        uint64_t                 addr  :  8; // Hash Table Address
        uint64_t                pktid  : 16; // Packet Id
        uint64_t       Reserved_63_24  : 40; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_HASH_TIMEOUT_t;

// TXOTR_PKT_ERR_INFO_TC_PKTID_CAPTURE desc:
typedef union {
    struct {
        uint64_t non_retrans_nack_rcvd_pktid  : 16; // Packet ID capture
        uint64_t non_retrans_nack_rcvd_mctc  :  3; // MCTC value capture
        uint64_t resource_nack_rcvd_pktid  : 16; // Packet ID capture
        uint64_t resource_nack_rcvd_tc  :  2; // TC value capture
        uint64_t  oos_nack_rcvd_pktid  : 16; // Packet ID capture
        uint64_t     oos_nack_rcvd_tc  :  2; // TC value capture
        uint64_t  psn_max_dist_vio_tc  :  2; // TC value capture
        uint64_t   unknown_e2e_cmd_tc  :  2; // TC value capture
        uint64_t pktid_list_ren_fifo_undflow_tc  :  2; // TC value capture
        uint64_t pktid_list_ren_fifo_overflow_tc  :  2; // TC value capture
        uint64_t          Reserved_63  :  1; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_TC_PKTID_CAPTURE_t;

// TXOTR_PKT_ERR_INFO_TUPPLE_MISMATCH desc:
typedef union {
    struct {
        uint64_t tuple_mismatch_opp_pktid  : 16; // Packet ID capture
        uint64_t tuple_mismatch_opp_mctc  :  3; // MCTC value capture
        uint64_t tuple_mismatch_pktid  : 16; // Packet ID capture
        uint64_t  tuple_mismatch_mctc  :  3; // MCTC value capture
        uint64_t       Reserved_63_38  : 26; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_TUPPLE_MISMATCH_t;

// TXOTR_PKT_ERR_INFO_FPE_DATA_MEM_BE desc:
typedef union {
    struct {
        uint64_t          address_sbe  : 10; // Address capture for the SBE error
        uint64_t          address_mbe  : 10; // Address capture for the MBE error
        uint64_t         syndrome_sbe  :  8; // Syndrome capture for the SBE error
        uint64_t         syndrome_mbe  :  8; // Syndrome capture for the MBE error
        uint64_t       Reserved_63_36  : 28; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_FPE_DATA_MEM_BE_t;

// TXOTR_PKT_ERR_INFO_FPE_PROG_MEM_BE desc:
typedef union {
    struct {
        uint64_t          address_sbe  : 13; // Address capture for the SBE error
        uint64_t          address_mbe  : 13; // Address capture for the MBE error
        uint64_t         syndrome_sbe  :  7; // Syndrome capture for the SBE error
        uint64_t         syndrome_mbe  :  7; // Syndrome capture for the MBE error
        uint64_t       Reserved_63_40  : 24; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_FPE_PROG_MEM_BE_t;

// TXOTR_PKT_ERR_INFO_FPE_FIRMWARE desc:
typedef union {
    struct {
        uint64_t             encoding  :  8; // Error encoding. TODO - delineate encodings - Read or write of an invalid address
        uint64_t                 ctxt  :  5; // Context from which the error originated
        uint64_t       Reserved_15_13  :  3; // Unused
        uint64_t               msg_id  : 16; // Message Identifier of the command associated with the error
        uint64_t       Reserved_62_32  : 31; // Unused
        uint64_t         firmware_err  :  1; // Indicates a firmware error
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_FPE_FIRMWARE_t;

// TXOTR_PKT_ERR_INFO_HI_HDR_BE desc:
typedef union {
    struct {
        uint64_t              tid_sbe  : 12; // TID captured for SBE errors
        uint64_t         syndrome_sbe  :  8; // Syndrome capture for SBE error
        uint64_t         syndrome_mbe  :  8; // Syndrome capture for MBE error
        uint64_t       Reserved_63_28  : 36; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_HI_HDR_BE_t;

// TXOTR_PKT_ERR_INFO_HI_DATA_BE_0 desc:
typedef union {
    struct {
        uint64_t       syndrome_sbe_0  :  8; // Syndrome capture for SBE error for domain 0
        uint64_t       syndrome_sbe_1  :  8; // Syndrome capture for SBE error for domain 1
        uint64_t       syndrome_sbe_2  :  8; // Syndrome capture for SBE error for domain 2
        uint64_t       syndrome_sbe_3  :  8; // Syndrome capture for SBE error for domain 3
        uint64_t       syndrome_mbe_0  :  8; // Syndrome capture for MBE error for domain 0
        uint64_t       syndrome_mbe_1  :  8; // Syndrome capture for MBE error for domain 1
        uint64_t       syndrome_mbe_2  :  8; // Syndrome capture for MBE error for domain 2
        uint64_t       syndrome_mbe_3  :  8; // Syndrome capture for MBE error for domain 3
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_HI_DATA_BE_0_t;

// TXOTR_PKT_ERR_INFO_HI_DATA_BE_1 desc:
typedef union {
    struct {
        uint64_t           domain_sbe  :  4; // Domain captured for SBE errors
        uint64_t           domain_mbe  :  4; // Domain captured for MBE errors
        uint64_t        Reserved_63_8  : 56; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_HI_DATA_BE_1_t;

// TXOTR_PKT_ERR_INJECT_PSN_CACHE_SBE_MBE desc:
typedef union {
    struct {
        uint64_t psn_cache_tag_err_inj_mask  :  8; // psn_cache_tag error inject mask
        uint64_t psn_cache_tag_err_inj_domain  :  3; // psn_cache_tag error inject domain. Which ecc domain to inject the error.
        uint64_t psn_cache_tag_err_inj_enable  :  1; // psn_cache_tag error inject enable.
        uint64_t psn_cache_data_err_inj_mask  :  8; // psn_cache_data error inject mask
        uint64_t psn_cache_data_err_inj_enable  :  1; // psn_cache_data error inject enable.
        uint64_t       Reserved_63_21  : 43; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INJECT_PSN_CACHE_SBE_MBE_t;

// TXOTR_PKT_ERR_INFO_MAX_MTU desc:
typedef union {
    struct {
        uint64_t                   tc  :  2; // Captured TC for the Maximum MTU violation
        uint64_t        packet_length  : 12; // Captured packet length for the Maximum MTU violation
        uint64_t                 dlid  : 24; // Captured dlid for the Maximum MTU violation
        uint64_t       Reserved_63_38  : 26; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_MAX_MTU_t;

// TXOTR_PKT_ERR_INFO_RXE2E_STATUS_TO_TIMEOUT desc:
typedef union {
    struct {
        uint64_t rx_status_rxe2e_pktid  : 16; // Packet ID capture
        uint64_t rx_status_rxe2e_mctc  :  3; // MCTC value capture
        uint64_t about_to_timeout_pktid  : 16; // Packet ID capture
        uint64_t about_to_timeout_mctc  :  3; // MCTC value capture
        uint64_t       Reserved_63_38  : 26; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INFO_RXE2E_STATUS_TO_TIMEOUT_t;

// TXOTR_PKT_ERR_INJECT_0 desc:
typedef union {
    struct {
        uint64_t  fp_fifo_inject_mask  : 32; // Error injection Mask
        uint64_t    fp_fifo_inject_en  :  4; // Error inject enable.
        uint64_t       Reserved_63_36  : 28; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INJECT_0_t;

// TXOTR_PKT_ERR_INJECT_1 desc:
typedef union {
    struct {
        uint64_t pre_frag_fifo_inject_mask  : 32; // Error injection Mask
        uint64_t pre_frag_fifo_inject_en  :  4; // Error inject enable.
        uint64_t       Reserved_63_36  : 28; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INJECT_1_t;

// TXOTR_PKT_ERR_INJECT_2 desc:
typedef union {
    struct {
        uint64_t post_frag_fifo_inject_mask  : 32; // Error injection Mask
        uint64_t post_frag_fifo_inject_en  :  4; // Error inject enable.
        uint64_t       Reserved_63_36  : 28; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INJECT_2_t;

// TXOTR_PKT_ERR_INJECT_3 desc:
typedef union {
    struct {
        uint64_t         Reserved_7_0  :  8; // Unused
        uint64_t fpe_prog_inject_mask  :  7; // Error injection Mask
        uint64_t   fpe_prog_inject_en  :  1; // Error inject enable.
        uint64_t fpe_data_inject_mask  :  8; // Error injection Mask
        uint64_t   fpe_data_inject_en  :  1; // Error inject enable.
        uint64_t       Reserved_63_25  : 39; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INJECT_3_t;

// TXOTR_PKT_ERR_INJECT_4 desc:
typedef union {
    struct {
        uint64_t retrans_fifo_inject_mask  : 32; // Error injection Mask
        uint64_t retrans_fifo_inject_en  :  4; // Error inject enable.
        uint64_t       Reserved_63_36  : 28; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INJECT_4_t;

// TXOTR_PKT_ERR_INJECT_5 desc:
typedef union {
    struct {
        uint64_t tx_psn_fifo_inject_mask  : 32; // Error injection Mask
        uint64_t tx_psn_fifo_inject_en  :  4; // Error inject enable.
        uint64_t hash_table_inject_mask  :  8; // Error injection Mask
        uint64_t hash_table_inject_en  :  1; // Error inject enable.
        uint64_t       Reserved_63_45  : 19; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INJECT_5_t;

// TXOTR_PKT_ERR_INJECT_6 desc:
typedef union {
    struct {
        uint64_t pktid_list_inject_mask  : 48; // Error injection Mask
        uint64_t pktid_list_inject_en  :  8; // Error inject enable.
        uint64_t       Reserved_63_56  :  8; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INJECT_6_t;

// TXOTR_PKT_ERR_INJECT_9 desc:
typedef union {
    struct {
        uint64_t rsp_fifo_inject_mask  : 32; // Error injection Mask
        uint64_t   rsp_fifo_inject_en  :  4; // Error inject enable.
        uint64_t   hi_hdr_inject_mask  :  8; // Error injection Mask
        uint64_t     hi_hdr_inject_en  :  1; // Error inject enable.
        uint64_t       Reserved_63_45  : 19; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INJECT_9_t;

// TXOTR_PKT_ERR_INJECT_10 desc:
typedef union {
    struct {
        uint64_t  hi_data_inject_mask  : 32; // Error injection Mask
        uint64_t    hi_data_inject_en  :  4; // Error inject enable.
        uint64_t       Reserved_63_36  : 28; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INJECT_10_t;

// TXOTR_PKT_ERR_INJECT_11 desc:
typedef union {
    struct {
        uint64_t iovec_buff_inject_en  :  8; // Error injection enable
        uint64_t  opb_otm_inject_mask  :  9; // Error injection Mask
        uint64_t    opb_otm_inject_en  :  1; // Error inject enable.
        uint64_t       Reserved_63_18  : 46; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INJECT_11_t;

// TXOTR_PKT_ERR_INJECT_12 desc:
typedef union {
    struct {
        uint64_t iovec_buff_inject_mask  : 64; // Error injection Mask
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INJECT_12_t;

// TXOTR_PKT_ERR_INJECT_13 desc:
typedef union {
    struct {
        uint64_t opb_inject_mask_39_0  : 40; // Error injection Mask
        uint64_t        opb_inject_en  :  5; // Error inject enable.
        uint64_t       Reserved_63_45  : 19; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INJECT_13_t;

// TXOTR_PKT_ERR_INJECT_14 desc:
typedef union {
    struct {
        uint64_t opb_inject_mask_39_0  : 40; // Error injection Mask
        uint64_t        opb_inject_en  :  5; // Error inject enable.
        uint64_t       Reserved_63_45  : 19; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INJECT_14_t;

// TXOTR_PKT_ERR_INJECT_15 desc:
typedef union {
    struct {
        uint64_t opb_inject_mask_39_0  : 40; // Error injection Mask
        uint64_t        opb_inject_en  :  5; // Error inject enable.
        uint64_t       Reserved_63_45  : 19; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INJECT_15_t;

// TXOTR_PKT_ERR_INJECT_16 desc:
typedef union {
    struct {
        uint64_t opb_inject_mask_39_0  : 40; // Error injection Mask
        uint64_t        opb_inject_en  :  5; // Error inject enable.
        uint64_t       Reserved_63_45  : 19; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_INJECT_16_t;

// TXOTR_PKT_ERR_MBE_SAT_COUNT desc:
typedef union {
    struct {
        uint64_t        mbe_sat_count  : 16; // MBE saturation count
        uint64_t       Reserved_63_16  : 48; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_ERR_MBE_SAT_COUNT_t;

// TXOTR_PKT_DBG_PSN_CACHE_TAG_WAY_ENABLE desc:
typedef union {
    struct {
        uint64_t           WAY_ENABLE  : 16; // 1 bits enable, 0 bits disable PSN Cache Tag Ways.
        uint64_t       Reserved_63_16  : 48; // Unused
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
        uint64_t                  ECC  :  1; // 0 = Read Raw Data 1 = Correct Data on Read
        uint64_t          Reserved_62  :  1; // Unused
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
        uint64_t                 Data  : 64; // Data[319:256]
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_OPB_PAYLOAD4_t;

// TXOTR_PKT_DBG_PKTID_LIST_ACCESS desc:
typedef union {
    struct {
        uint64_t              address  : 16; // Address for Read. Valid address are from 0 to 12287. For FXR, bits 15:14 are ignored. No read or write is performed for an address value between 12288 and 16383.
        uint64_t       Reserved_51_16  : 36; // Unused
        uint64_t         Payload_regs  :  8; // Constant indicating number of payload registers follow
        uint64_t          Reserved_60  :  1; // Unused
        uint64_t                  ECC  :  1; // 0 = Read Raw Data 1 = Correct Data on Read
        uint64_t               Valid0  :  1; // Set by software, cleared by hardware when complete. Valid for previous pointer
        uint64_t               Valid1  :  1; // Set by software, cleared by hardware when complete. Valid for next pointer
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_PKTID_LIST_ACCESS_t;

// TXOTR_PKT_DBG_PKTID_LIST_PAYLOAD desc:
typedef union {
    struct {
        uint64_t                Data0  : 14; // Data
        uint64_t                Data1  : 14; // Data
        uint64_t       Reserved_63_28  : 36; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_PKTID_LIST_PAYLOAD_t;

// TXOTR_PKT_DBG_HASH_TABLE_ACCESS desc:
typedef union {
    struct {
        uint64_t              address  :  6; // Address for Read. Valid address are from 0 to 127.
        uint64_t        Reserved_51_6  : 46; // Unused
        uint64_t         Payload_regs  :  8; // Constant indicating number of payload registers follow
        uint64_t          Reserved_60  :  1; // Unused
        uint64_t                  ECC  :  1; // 0 = Read Raw Data 1 = Correct Data on Read
        uint64_t          Reserved_62  :  1; // Unused
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
        uint64_t              address  :  7; // Address for Read. Valid address are from 0 to 127.
        uint64_t        Reserved_51_7  : 45; // Unused
        uint64_t         Payload_regs  :  8; // Constant indicating number of payload registers follow
        uint64_t          Reserved_60  :  1; // Unused
        uint64_t                  ECC  :  1; // 0 = Read Raw Data 1 = Correct Data on Read
        uint64_t          Reserved_62  :  1; // Unused
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
        uint64_t                 Data  : 11; // Data
        uint64_t       Reserved_63_11  : 53; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_OTM_PAYLOAD2_t;

// TXOTR_PKT_DBG_IOVEC_BUFFER_SPACE_ACCESS desc:
typedef union {
    struct {
        uint64_t              address  :  5; // Address for Read. Valid address are from 0 to 32.
        uint64_t        Reserved_51_5  : 47; // Unused
        uint64_t         Payload_regs  :  8; // Constant indicating number of payload registers follow
        uint64_t          Reserved_60  :  1; // Unused
        uint64_t                  ECC  :  1; // 0 = Read Raw Data 1 = Correct Data on Read
        uint64_t          Reserved_62  :  1; // Unused
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
        uint64_t              address  : 13; // Address for Write. Valid address are from 0 to 8191.
        uint64_t       Reserved_51_13  : 39; // Unused
        uint64_t         Payload_regs  :  8; // Constant indicating number of payload registers follow
        uint64_t       Reserved_62_60  :  3; // Unused
        uint64_t                Valid  :  1; // Set by software, cleared by hardware when complete.
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_FPE_PROG_MEM_ACCESS_t;

// TXOTR_PKT_DBG_FPE_PROG_MEM_PAYLOAD0 desc:
typedef union {
    struct {
        uint64_t                 Data  : 32; // Data[31:0]
        uint64_t       Reserved_63_32  : 32; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_FPE_PROG_MEM_PAYLOAD0_t;

// TXOTR_PKT_DBG_FPE_DATA_MEM_ACCESS desc:
typedef union {
    struct {
        uint64_t              address  : 10; // Address for Write. Valid address are from 0 to 1023.
        uint64_t       Reserved_51_10  : 42; // Unused
        uint64_t         Payload_regs  :  8; // Constant indicating number of payload registers follow
        uint64_t       Reserved_62_60  :  3; // Unused
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
        uint64_t       Reserved_31_24  :  8; // Unused
        uint64_t            PRE_EMPTY  :  8; // Empty bits of Pre-Fragmentation Queues, 1 bit per queue
        uint64_t             PRE_FULL  :  8; // Full bits of Pre-Fragmentation Queues, 1 bit per queue
        uint64_t       Reserved_63_48  : 16; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_FP_PRE_FRAG_FULL_EMPTY_t;

// TXOTR_PKT_DBG_POST_FRAG_RET_FULL_EMPTY desc:
typedef union {
    struct {
        uint64_t           POST_EMPTY  :  8; // Empty bits of Post-Fragmentation Queues, 1 bit per queue
        uint64_t            POST_FULL  :  8; // Full bits of Post-Fragmentation Queues, 1 bit per queue
        uint64_t       Reserved_31_16  : 16; // Unused
        uint64_t       POST_RET_EMPTY  :  8; // Empty bits of Post-Frag Retransmission Queues, 1 bit per queue
        uint64_t        POST_RET_FULL  :  8; // Full bits of Post-Frag Retransmission Queues, 1 bit per 8 queues
        uint64_t       Reserved_63_48  : 16; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_POST_FRAG_RET_FULL_EMPTY_t;

// TXOTR_PKT_DBG_FPE_CONTEXT_AVAILABLE_REG desc:
typedef union {
    struct {
        uint64_t                  FPE  : 32; // Context-Available Register of FPE
        uint64_t       Reserved_63_32  : 32; // Unused
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
        uint64_t               head_0  : 14; // Head Pointer array 0
        uint64_t       Reserved_15_14  :  2; // Unused
        uint64_t               tail_0  : 14; // Tail Pointer array 0
        uint64_t       Reserved_31_30  :  2; // Unused
        uint64_t               head_1  : 14; // Head Pointer array 1
        uint64_t       Reserved_47_46  :  2; // Unused
        uint64_t               tail_1  : 14; // Tail Pointer array 1
        uint64_t       Reserved_63_62  :  2; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_PKTID_LIST_SHARED_POINTERS_t;

// TXOTR_PKT_DBG_PKTID_LIST_RSVD_POINTERS00 desc:
typedef union {
    struct {
        uint64_t             mc0_head  : 14; // MC0 Head Pointer
        uint64_t       Reserved_15_14  :  2; // Unused
        uint64_t             mc0_tail  : 14; // MC0 Tail Pointer
        uint64_t       Reserved_31_30  :  2; // Unused
        uint64_t             mc1_head  : 14; // MC1 Head Pointer
        uint64_t       Reserved_47_46  :  2; // Unused
        uint64_t             mc1_tail  : 14; // MC1 Tail Pointer
        uint64_t       Reserved_63_62  :  2; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_PKTID_LIST_RSVD_POINTERS00_t;

// TXOTR_PKT_DBG_PKTID_LIST_RSVD_POINTERS01 desc:
typedef union {
    struct {
        uint64_t             mc0_head  : 14; // MC0 Head Pointer
        uint64_t       Reserved_15_14  :  2; // Unused
        uint64_t             mc0_tail  : 14; // MC0 Tail Pointer
        uint64_t       Reserved_31_30  :  2; // Unused
        uint64_t             mc1_head  : 14; // MC1 Head Pointer
        uint64_t       Reserved_47_46  :  2; // Unused
        uint64_t             mc1_tail  : 14; // MC1 Tail Pointer
        uint64_t       Reserved_63_62  :  2; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_PKTID_LIST_RSVD_POINTERS01_t;

// TXOTR_PKT_DBG_PKTID_LIST_RSVD_POINTERS10 desc:
typedef union {
    struct {
        uint64_t             mc0_head  : 14; // MC0 Head Pointer
        uint64_t       Reserved_15_14  :  2; // Unused
        uint64_t             mc0_tail  : 14; // MC0 Tail Pointer
        uint64_t       Reserved_31_30  :  2; // Unused
        uint64_t             mc1_head  : 14; // MC1 Head Pointer
        uint64_t       Reserved_47_46  :  2; // Unused
        uint64_t             mc1_tail  : 14; // MC1 Tail Pointer
        uint64_t       Reserved_63_62  :  2; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_PKTID_LIST_RSVD_POINTERS10_t;

// TXOTR_PKT_DBG_PKTID_LIST_RSVD_POINTERS11 desc:
typedef union {
    struct {
        uint64_t             mc0_head  : 14; // MC0 Head Pointer
        uint64_t       Reserved_15_14  :  2; // Unused
        uint64_t             mc0_tail  : 14; // MC0 Tail Pointer
        uint64_t       Reserved_31_30  :  2; // Unused
        uint64_t             mc1_head  : 14; // MC1 Head Pointer
        uint64_t       Reserved_47_46  :  2; // Unused
        uint64_t             mc1_tail  : 14; // MC1 Tail Pointer
        uint64_t       Reserved_63_62  :  2; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_PKTID_LIST_RSVD_POINTERS11_t;

// TXOTR_PKT_DBG_PKTID_LIST_RSVD_POINTERS20 desc:
typedef union {
    struct {
        uint64_t             mc0_head  : 14; // MC0 Head Pointer
        uint64_t       Reserved_15_14  :  2; // Unused
        uint64_t             mc0_tail  : 14; // MC0 Tail Pointer
        uint64_t       Reserved_31_30  :  2; // Unused
        uint64_t             mc1_head  : 14; // MC1 Head Pointer
        uint64_t       Reserved_47_46  :  2; // Unused
        uint64_t             mc1_tail  : 14; // MC1 Tail Pointer
        uint64_t       Reserved_63_62  :  2; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_PKTID_LIST_RSVD_POINTERS20_t;

// TXOTR_PKT_DBG_PKTID_LIST_RSVD_POINTERS21 desc:
typedef union {
    struct {
        uint64_t             mc0_head  : 14; // MC0 Head Pointer
        uint64_t       Reserved_15_14  :  2; // Unused
        uint64_t             mc0_tail  : 14; // MC0 Tail Pointer
        uint64_t       Reserved_31_30  :  2; // Unused
        uint64_t             mc1_head  : 14; // MC1 Head Pointer
        uint64_t       Reserved_47_46  :  2; // Unused
        uint64_t             mc1_tail  : 14; // MC1 Tail Pointer
        uint64_t       Reserved_63_62  :  2; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_PKTID_LIST_RSVD_POINTERS21_t;

// TXOTR_PKT_DBG_PKTID_LIST_RSVD_POINTERS30 desc:
typedef union {
    struct {
        uint64_t             mc0_head  : 14; // MC0 Head Pointer
        uint64_t       Reserved_15_14  :  2; // Unused
        uint64_t             mc0_tail  : 14; // MC0 Tail Pointer
        uint64_t       Reserved_31_30  :  2; // Unused
        uint64_t             mc1_head  : 14; // MC1 Head Pointer
        uint64_t       Reserved_47_46  :  2; // Unused
        uint64_t             mc1_tail  : 14; // MC1 Tail Pointer
        uint64_t       Reserved_63_62  :  2; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_PKTID_LIST_RSVD_POINTERS30_t;

// TXOTR_PKT_DBG_PKTID_LIST_RSVD_POINTERS31 desc:
typedef union {
    struct {
        uint64_t             mc0_head  : 14; // MC0 Head Pointer
        uint64_t       Reserved_15_14  :  2; // Unused
        uint64_t             mc0_tail  : 14; // MC0 Tail Pointer
        uint64_t       Reserved_31_30  :  2; // Unused
        uint64_t             mc1_head  : 14; // MC1 Head Pointer
        uint64_t       Reserved_47_46  :  2; // Unused
        uint64_t             mc1_tail  : 14; // MC1 Tail Pointer
        uint64_t       Reserved_63_62  :  2; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_PKTID_LIST_RSVD_POINTERS31_t;

// TXOTR_PKT_DBG_PKTID_LIST_PENDING_POINTERS0 desc:
typedef union {
    struct {
        uint64_t             tc0_head  : 16; // TC0 Head Pointer
        uint64_t             tc0_tail  : 16; // TC0 Tail Pointer
        uint64_t             tc1_head  : 16; // TC1 Head Pointer
        uint64_t             tc1_tail  : 16; // TC1 Tail Pointer
    } field;
    uint64_t val;
} TXOTR_PKT_DBG_PKTID_LIST_PENDING_POINTERS0_t;

// TXOTR_PKT_DBG_PKTID_LIST_PENDING_POINTERS1 desc:
typedef union {
    struct {
        uint64_t             tc2_head  : 16; // TC2 Head Pointer
        uint64_t             tc2_tail  : 16; // TC2 Tail Pointer
        uint64_t             tc3_head  : 16; // TC3 Head Pointer
        uint64_t             tc3_tail  : 16; // TC3 Tail Pointer
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
        uint64_t       Reserved_63_24  : 40; // Unused
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
        uint64_t       Reserved_63_32  : 32; // Unused
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
        uint64_t                index  :  8; // Index into the mask. Attempts to write values larger than 131 are ignored.
        uint64_t               enable  :  1; // Enable. Software must clear this bit when it wishes to issue another write to the mask.
        uint64_t        Reserved_63_9  : 55; // Unused
    } field;
    uint64_t val;
} TXOTR_PKG_DBG_TO_NACK_MASK_t;

// TXOTR_PKG_DBG_PENDING_RETRANSMIT desc:
typedef union {
    struct {
        uint64_t                index  :  6; // Index into the sidecar.
        uint64_t               enable  :  1; // Enable. Software must clear this bit when it wishes to issue another write to the sidecar.
        uint64_t        Reserved_63_7  : 57; // Unused
    } field;
    uint64_t val;
} TXOTR_PKG_DBG_PENDING_RETRANSMIT_t;

// TXOTR_PKG_DBG_OOS_NACK_MASK desc:
typedef union {
    struct {
        uint64_t                index  :  6; // Index into the mask.
        uint64_t               enable  :  1; // Enable. Software must clear this bit when it wishes to issue another write to the mask.
        uint64_t        Reserved_63_7  : 57; // Unused
    } field;
    uint64_t val;
} TXOTR_PKG_DBG_OOS_NACK_MASK_t;

// TXOTR_PKG_DBG_HEAD_PENDING desc:
typedef union {
    struct {
        uint64_t                index  :  8; // Index into the sidecar. Attempts to write values larger than 131 are ignored.
        uint64_t               enable  :  1; // Enable. Software must clear this bit when it wishes to issue another write to the sidecar.
        uint64_t        Reserved_63_9  : 55; // Unused
    } field;
    uint64_t val;
} TXOTR_PKG_DBG_HEAD_PENDING_t;

// TXOTR_PKT_PRF_STALL_OPB_ENTRIES_X desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  4; // 4'd15-4'd11 - Reserved 4'd10 - Any MC 4'd9 - Any MC1 4'd8 - Any MC0 4'd7 - MC1TC3 4'd6 - MC1TC2 4'd5 - MC1TC1 4'd4 - MC1TC0 4'd3 - MC0TC3 4'd2 - MC0TC2 4'd1 - MC0TC1 4'd0 - MC0TC0
        uint64_t        Reserved_63_4  : 60; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_PRF_STALL_OPB_ENTRIES_X_t;

// TXOTR_PKT_PRF_STALL_OPB_ENTRIES_Y desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  4; // 4'd15-4'd11 - Reserved 4'd10 - Any MC 4'd9 - Any MC1 4'd8 - Any MC0 4'd7 - MC1TC3 4'd6 - MC1TC2 4'd5 - MC1TC1 4'd4 - MC1TC0 4'd3 - MC0TC3 4'd2 - MC0TC2 4'd1 - MC0TC1 4'd0 - MC0TC0
        uint64_t        Reserved_63_4  : 60; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_PRF_STALL_OPB_ENTRIES_Y_t;

// TXOTR_PKT_PRF_STALL_TXDMA_CREDITS_X desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  4; // 4'd15-4'd11 - Reserved 4'd10 - Any MC 4'd9 - Any MC1 4'd8 - Any MC0 4'd7 - MC1TC3 4'd6 - MC1TC2 4'd5 - MC1TC1 4'd4 - MC1TC0 4'd3 - MC0TC3 4'd2 - MC0TC2 4'd1 - MC0TC1 4'd0 - MC0TC0
        uint64_t        Reserved_63_4  : 60; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_PRF_STALL_TXDMA_CREDITS_X_t;

// TXOTR_PKT_PRF_STALL_TXDMA_CREDITS_Y desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  4; // 4'd15-4'd11 - Reserved 4'd10 - Any MC 4'd9 - Any MC1 4'd8 - Any MC0 4'd7 - MC1TC3 4'd6 - MC1TC2 4'd5 - MC1TC1 4'd4 - MC1TC0 4'd3 - MC0TC3 4'd2 - MC0TC2 4'd1 - MC0TC1 4'd0 - MC0TC0
        uint64_t        Reserved_63_4  : 60; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_PRF_STALL_TXDMA_CREDITS_Y_t;

// TXOTR_PKT_PRF_STALL_P_TO_M_CREDITS_X desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  3; // 3'd7-3'd5 - Reserved 3'd4 - Any MC1 3'd3 - MC1TC3 3'd2 - MC1TC2 3'd1 - MC1TC1 3'd0 - MC1TC0
        uint64_t        Reserved_63_3  : 61; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_PRF_STALL_P_TO_M_CREDITS_X_t;

// TXOTR_PKT_PRF_STALL_P_TO_M_CREDITS_Y desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  3; // 3'd7 - 3'd5 - Reserved 3'd4 - Any MC1 2'd3 - MC1TC3 2'd2 - MC1TC2 2'd1 - MC1TC1 2'd0 - MC1TC0
        uint64_t        Reserved_63_3  : 61; // Unused
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
        uint64_t       Reserved_63_48  : 16; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_PRF_LATENCY_t;

// TXOTR_PKT_PRF_PKTS_OPENED_X desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  4; // 4'd15-4'd11 - Reserved 4'd10 - Any MC 4'd9 - Any MC1 4'd8 - Any MC0 4'd7 - MC1TC3 4'd6 - MC1TC2 4'd5 - MC1TC1 4'd4 - MC1TC0 4'd3 - MC0TC3 4'd2 - MC0TC2 4'd1 - MC0TC1 4'd0 - MC0TC0
        uint64_t        Reserved_63_4  : 60; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_PRF_PKTS_OPENED_X_t;

// TXOTR_PKT_PRF_PKTS_CLOSED_X desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  4; // 4'd15-4'd11 - Reserved 4'd10 - Any MC 4'd9 - Any MC1 4'd8 - Any MC0 4'd7 - MC1TC3 4'd6 - MC1TC2 4'd5 - MC1TC1 4'd4 - MC1TC0 4'd3 - MC0TC3 4'd2 - MC0TC2 4'd1 - MC0TC1 4'd0 - MC0TC0
        uint64_t        Reserved_63_4  : 60; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_PRF_PKTS_CLOSED_X_t;

// TXOTR_PKT_PRF_RETRANS_LIMIT_RCHD_X desc:
typedef union {
    struct {
        uint64_t                   TC  :  3; // 3'd7-3'd5 - Reserved 3'd4 - Any TC 3'd3 - TC3 3'd2 - TC2 3'd1 - TC1 3'd0 - TC0
        uint64_t        Reserved_63_3  : 61; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_PRF_RETRANS_LIMIT_RCHD_X_t;

// TXOTR_PKT_PRF_LOCAL_SEQ_STALL_X desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  4; // 4'd15-4'd11 - Reserved 4'd10 - Any MC 4'd9 - Any MC1 4'd8 - Any MC0 4'd7 - MC1TC3 4'd6 - MC1TC2 4'd5 - MC1TC1 4'd4 - MC1TC0 4'd3 - MC0TC3 4'd2 - MC0TC2 4'd1 - MC0TC1 4'd0 - MC0TC0
        uint64_t        Reserved_63_4  : 60; // Unused
    } field;
    uint64_t val;
} TXOTR_PKT_PRF_LOCAL_SEQ_STALL_X_t;

#endif /* ___FXR_tx_otr_pkt_top_CSRS_H__ */