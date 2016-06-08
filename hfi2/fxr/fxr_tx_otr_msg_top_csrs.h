// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Thu Jun  2 19:11:24 2016
//

#ifndef ___FXR_tx_otr_msg_top_CSRS_H__
#define ___FXR_tx_otr_msg_top_CSRS_H__

// TXOTR_MSG_CFG_RENDEZVOUS_FIFO_CRDTS desc:
typedef union {
    struct {
        uint64_t                  TC0  :  3; // TC0 Retransmission OPB Input Queue Credits
        uint64_t             UNUSED_3  :  1; // Unused
        uint64_t                  TC1  :  3; // TC1 Retransmission OPB Input Queue Credits
        uint64_t             UNUSED_7  :  1; // Unused
        uint64_t                  TC2  :  3; // TC2 Retransmission OPB Input Queue Credits
        uint64_t            UNUSED_11  :  1; // Unused
        uint64_t                  TC3  :  3; // TC3 Retransmission OPB Input Queue Credits
        uint64_t         UNUSED_63_15  : 49; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_RENDEZVOUS_FIFO_CRDTS_t;

// TXOTR_MSG_CFG_TXCI_MC0_CRDTS desc:
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
} TXOTR_MSG_CFG_TXCI_MC0_CRDTS_t;

// TXOTR_MSG_CFG_TXCI_MC1_CRDTS desc:
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
} TXOTR_MSG_CFG_TXCI_MC1_CRDTS_t;

// TXOTR_MSG_CFG_TXCI_MC1P_CRDTS desc:
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
} TXOTR_MSG_CFG_TXCI_MC1P_CRDTS_t;

// TXOTR_MSG_CFG_RX_OMB_CRDTS desc:
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
} TXOTR_MSG_CFG_RX_OMB_CRDTS_t;

// TXOTR_MSG_CFG_PREFRAG_OPB_FIFO_CRDTS desc:
typedef union {
    struct {
        uint64_t               MC0TC0  :  3; // MC0/TC0 Pre-fragmentation OPB Input Queue Credits
        uint64_t             UNUSED_3  :  1; // Unused
        uint64_t               MC0TC1  :  3; // MC0/TC1 Pre-fragmentation OPB Input Queue Credits
        uint64_t             UNUSED_7  :  1; // Unused
        uint64_t               MC0TC2  :  3; // MC0/TC2 Pre-fragmentation OPB Input Queue Credits
        uint64_t            UNUSED_11  :  1; // Unused
        uint64_t               MC0TC3  :  3; // MC0/TC3 Pre-fragmentation OPB Input Queue Credits
        uint64_t            UNUSED_15  :  1; // Unused
        uint64_t               MC1TC0  :  3; // MC1/TC0 Pre-fragmentation OPB Input Queue Credits
        uint64_t            UNUSED_19  :  1; // Unused
        uint64_t               MC1TC1  :  3; // MC1/TC1 Pre-fragmentation OPB Input Queue Credits
        uint64_t            UNUSED_23  :  1; // Unused
        uint64_t               MC1TC2  :  3; // MC1/TC2 Pre-fragmentation OPB Input Queue Credits
        uint64_t            UNUSED_27  :  1; // Unused
        uint64_t               MC1TC3  :  3; // MC1/TC3 Pre-fragmentation OPB Input Queue Credits
        uint64_t         UNUSED_63_31  : 33; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_PREFRAG_OPB_FIFO_CRDTS_t;

// TXOTR_MSG_CFG_FP_OPB_FIFO_CRDTS desc:
typedef union {
    struct {
        uint64_t               MC0TC0  :  5; // MC0/TC0 Fast Path OPB Input Queue Credits
        uint64_t               MC0TC1  :  5; // MC0/TC1 Fast Path OPB Input Queue Credits
        uint64_t               MC0TC2  :  5; // MC0/TC2 Fast Path OPB Input Queue Credits
        uint64_t               MC0TC3  :  5; // MC0/TC3 Fast Path OPB Input Queue Credits
        uint64_t               MC1TC0  :  5; // MC1/TC0 Fast Path OPB Input Queue Credits
        uint64_t               MC1TC1  :  5; // MC1/TC1 Fast Path OPB Input Queue Credits
        uint64_t               MC1TC2  :  5; // MC1/TC2 Fast Path OPB Input Queue Credits
        uint64_t               MC1TC3  :  5; // MC1/TC3 Fast Path OPB Input Queue Credits
        uint64_t              MC1PTC0  :  5; // MC1'/TC0 Fast Path OPB Input Queue Credits
        uint64_t              MC1PTC1  :  5; // MC1'/TC1 Fast Path OPB Input Queue Credits
        uint64_t              MC1PTC2  :  5; // MC1'/TC2 Fast Path OPB Input Queue Credits
        uint64_t              MC1PTC3  :  5; // MC1'/TC3 Fast Path OPB Input Queue Credits
        uint64_t         UNUSED_63_60  :  4; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_FP_OPB_FIFO_CRDTS_t;

// TXOTR_MSG_CFG_MSGID_1 desc:
typedef union {
    struct {
        uint64_t         RSVD_MC0_TC0  :  8; // The number of MC0/TC0 Reserved Message Identifiers
        uint64_t         RSVD_MC0_TC1  :  8; // The number of MC0/TC1 Reserved Message Identifiers
        uint64_t         RSVD_MC0_TC2  :  8; // The number of MC0/TC2 Reserved Message Identifiers
        uint64_t         RSVD_MC0_TC3  :  8; // The number of MC0/TC3 Reserved Message Identifiers
        uint64_t         RSVD_MC1_TC0  :  8; // The number of MC1/TC0 Reserved Message Identifiers
        uint64_t         RSVD_MC1_TC1  :  8; // The number of MC1/TC1 Reserved Message Identifiers
        uint64_t         RSVD_MC1_TC2  :  8; // The number of MC1/TC2 Reserved Message Identifiers
        uint64_t         RSVD_MC1_TC3  :  8; // The number of MC1/TC3 Reserved Message Identifiers
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_MSGID_1_t;

// TXOTR_MSG_CFG_MSGID_2 desc:
typedef union {
    struct {
        uint64_t               SHARED  : 14; // The number of Shared Message Identifiers
        uint64_t         UNUSED_63_14  : 50; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_MSGID_2_t;

// TXOTR_MSG_CFG_OMB_FIFO_ARB desc:
typedef union {
    struct {
        uint64_t      RENDEZ_PRIO_ALL  :  1; // If set, arbitration priority is given to Rendezvous Messages (RMessage) waiting to be arbitrated into the Message Level Tracking data path. I.e. All pending RMessages are arbitrated into the pipeline before any pending messages in the OMB Input Queues.
        uint64_t       RENDEZ_PRIO_TC  :  1; // If set, arbitration priority is given to Rendezvous Messages (RMessage) waiting to be arbitrated into the Message Level Tracking data path for a given Traffic Class. E.g. If a message is awaiting arbitration in the TC0 Rendezvous Queue and in the MC0TC0 OMB Input Queue, the Rendezvous Queue is given first priority.
        uint64_t          UNUSED_63_2  : 62; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_OMB_FIFO_ARB_t;

// TXOTR_MSG_CFG_TRANSMIT_DELAY desc:
typedef union {
    struct {
        uint64_t               SCALER  : 32; // This register selects the rate at which the TXOTR Delay Counter increments. If set to 32'd0, the counter is disabled. If set to 1, the counter increments once per clock cycle. If set to 2, the counter increments once every other clock cycle. Et cetera.
        uint64_t IGNORE_TRANSMIT_DELAY  :  1; // If this bit is set, the Transmit Delay in a CTS is ignored resulting in new CTS messages being added to the end of the Pending List and not to the Scheduled List.
        uint64_t         UNUSED_63_33  : 31; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_TRANSMIT_DELAY_t;

// TXOTR_MSG_CFG_CANCEL_MSG_REQ_1 desc:
typedef union {
    struct {
        uint64_t                 busy  :  1; // Request to cancel a message in OMB. Software should write this bit to 1'b1 when requesting to cancel. When hardware has finished processing the cancel request, this bit is cleared.
        uint64_t           UNUSED_3_1  :  3; // Unused
        uint64_t                 ipid  : 12; // Initiator Process ID. Messages in the OMB which match this Process ID (subjected to ipid_mask ) are canceled when busy is valid.
        uint64_t            ipid_mask  : 12; // Initiator Process ID Mask. See description in ipid .
        uint64_t            md_handle  : 11; // Memory Domain Handle. Messages in the OMB which match this MD Handle (subjected to md_handle_mask ) are canceled when busy is valid.
        uint64_t            UNUSED_39  :  1; // Unused
        uint64_t       md_handle_mask  : 11; // Memory Domain Handle Mask. See description in md_handle .
        uint64_t            UNUSED_51  :  1; // Unused
        uint64_t                   tc  :  2; // Traffic Class. Messages in the OMB which match this Traffic Class (subjected to tc_mask ) are canceled when is busy valid.
        uint64_t              tc_mask  :  2; // Traffic Class Mask. See description in tc .
        uint64_t         UNUSED_63_56  :  8; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_CANCEL_MSG_REQ_1_t;

// TXOTR_MSG_CFG_CANCEL_MSG_REQ_2 desc:
typedef union {
    struct {
        uint64_t         user_pointer  : 64; // User Pointer. Messages in the OMB which match this User Pointer are canceled when busy is valid.
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_CANCEL_MSG_REQ_2_t;

// TXOTR_MSG_CFG_CANCEL_MSG_REQ_3 desc:
typedef union {
    struct {
        uint64_t    user_pointer_mask  : 64; // User Pointer Mask. See description in user_pointer .
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_CANCEL_MSG_REQ_3_t;

// TXOTR_MSG_CFG_CANCEL_MSG_REQ_4 desc:
typedef union {
    struct {
        uint64_t                 dlid  : 24; // Destination Local Identifier. Messages in the OMB which match this DLID (subjected to dlid_mask ) are canceled when is busy valid.
        uint64_t            dlid_mask  : 24; // Destination Local Identifier Mask. See description in user_pointer .
        uint64_t         UNUSED_63_48  : 16; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_CANCEL_MSG_REQ_4_t;

// TXOTR_MSG_CFG_FORCE_MSG_TO_FPE desc:
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
} TXOTR_MSG_CFG_FORCE_MSG_TO_FPE_t;

// TXOTR_MSG_CFG_TC0 desc:
typedef union {
    struct {
        uint64_t            max_bytes  : 63; // Maximum number of bytes allowed to be outstanding on this traffic class.
        uint64_t               enable  :  1; // traffic class is enabled
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_TC0_t;

// TXOTR_MSG_CFG_TC1 desc:
typedef union {
    struct {
        uint64_t            max_bytes  : 63; // Maximum number of bytes allowed to be outstanding on this traffic class.
        uint64_t               enable  :  1; // traffic class is enabled
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_TC1_t;

// TXOTR_MSG_CFG_TC2 desc:
typedef union {
    struct {
        uint64_t            max_bytes  : 63; // Maximum number of bytes allowed to be outstanding on this traffic class.
        uint64_t               enable  :  1; // traffic class is enabled
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_TC2_t;

// TXOTR_MSG_CFG_TC3 desc:
typedef union {
    struct {
        uint64_t            max_bytes  : 63; // Maximum number of bytes allowed to be outstanding on this traffic class.
        uint64_t               enable  :  1; // traffic class is enabled
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_TC3_t;

// TXOTR_MSG_CFG_RENDEZVOUS desc:
typedef union {
    struct {
        uint64_t    disallow_rmessage  :  1; // Disallow Rendezvous Messages (RMessage) coming from TXCI
        uint64_t          UNUSED_63_1  : 63; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_RENDEZVOUS_t;

// TXOTR_MSG_CFG_RFS desc:
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
} TXOTR_MSG_CFG_RFS_t;

// TXOTR_MSG_CFG_RENDEZVOUS_RC desc:
typedef union {
    struct {
        uint64_t                port0  :  3; // Port 0 Routing Control Bits
        uint64_t             UNUSED_3  :  1; // Unused
        uint64_t                port1  :  3; // Port 1 Routing Control Bits
        uint64_t          UNUSED_63_7  : 57; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_RENDEZVOUS_RC_t;

// TXOTR_MSG_CFG_TIMEOUT desc:
typedef union {
    struct {
        uint64_t          UNUSED_35_0  : 36; // Unused
        uint64_t              TIMEOUT  : 12; // Timeout comparison value. A timeout has occurred when the difference between the time stamp in a given Hash Table entry and the time current time stamp has exceeded this value. This value must exactly match the TIMEOUT value in txotr_pkt_cfg_timeout .
        uint64_t         UNUSED_63_48  : 16; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_TIMEOUT_t;

// TXOTR_MSG_CFG_SMALL_HEADER desc:
typedef union {
    struct {
        uint64_t               ENABLE  :  1; // Enable small headers. If set to 1'b1, small headers are enabled. If set to 1'b0, all packets are forced to use long header formats.
        uint64_t          unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_SMALL_HEADER_t;

// TXOTR_MSG_CFG_ACK_REFUSED desc:
typedef union {
    struct {
        uint64_t               enable  :  1; // If set to 1'b1, an event is allowed when a Portals ACK with an opcode of PTL_ACK_REFUSED is received. When this bit is enabled, the event includes a fail type of PTL_NI_ACK_REFUSED.
        uint64_t          unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_ACK_REFUSED_t;

// TXOTR_MSG_CFG_RC_ORDERED_MAP desc:
typedef union {
    struct {
        uint64_t              ordered  :  8; // 1 bit = ordered packet 0 bit = unordered packet RC mapping to ordered
        uint64_t        Reserved_63_8  : 56; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_RC_ORDERED_MAP_t;

// TXOTR_MSG_CFG_BUFF_ENG desc:
typedef union {
    struct {
        uint64_t START_AUTHENTICATION  :  1; // Set this bit to start firmware image authentication. Clear it before writing to the firmware CSR.
        uint64_t AUTHENTICATION_COMPLETE  :  1; // Indicates that authentication completed on the current firmware image.
        uint64_t AUTHENTICATION_SUCCESSFUL  :  1; // Indicates that authentication was successful on the current firmware image.
        uint64_t          UNUSED_63_3  : 61; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_BUFF_ENG_t;

// TXOTR_MSG_ERR_STS_2 desc:
typedef union {
    struct {
        uint64_t           diagnostic  :  1; // Diagnostic Error Flag
        uint64_t         bpe_firmware  :  1; // Buffer Programmable Engine Error Flag. See txotr_msg_err_info_bpe_firmware CSR for error information.
        uint64_t              omb_mbe  :  1; // Outstanding Message Buffer MBE Error Flag. See txotr_msg_err_info_omb_be CSR for error information.
        uint64_t              omb_sbe  :  1; // Outstanding Message Buffer SBE Error Flag. See txotr_msg_err_info_omb_be CSR for error information.
        uint64_t        msgid_mem_mbe  :  1; // Message Identifier Memory MBE Error Flag. See txotr_msg_err_info_msgid_mem_be CSR for error information.
        uint64_t        msgid_mem_sbe  :  1; // Message Identifier Memory SBE Error Flag. See txotr_msg_err_info_msgid_mem_be CSR for error information.
        uint64_t     bpe_prog_mem_mbe  :  1; // Buffer Programmable Engine Program Memory MBE Error Flag. See txotr_msg_err_info_bpe_prog_mem_be CSR for error information.
        uint64_t     bpe_prog_mem_sbe  :  1; // Buffer Programmable Engine Program Memory SBE Error Flag. See txotr_msg_err_info_bpe_prog_mem_be CSR for error information.
        uint64_t     bpe_data_mem_mbe  :  1; // Buffer Programmable Engine Data Memory MBE Error Flag. See txotr_msg_err_info_bpe_data_mem_be CSR for error information.
        uint64_t     bpe_data_mem_sbe  :  1; // Buffer Programmable Engine Data Memory SBE Error Flag. See txotr_msg_err_info_bpe_data_mem_be CSR for error information.
        uint64_t              cmd_mbe  :  1; // Multiple BIt Error in command from TXCI or Buffer Programmable Engine Error Flag. See txotr_msg_err_info_txci_cmd_be CSR for error information.
        uint64_t              cmd_sbe  :  1; // Single Bit Error in command from TXCI or Buffer Programmable Engine Error Flag. See txotr_msg_err_info_txci_cmd_be CSR for error information.
        uint64_t        msg_queue_ovf  :  1; // A queue in the message partition has overflowed. See txotr_msg_err_info_msg_ovf_unf CSR for error information.
        uint64_t        msg_queue_unf  :  1; // A queue in the message partition has underflowed. See txotr_msg_err_info_msg_ovf_unf CSR for error information.
        uint64_t        bpe_queue_ovf  :  1; // A queue in the buffer partition has overflowed. See txotr_msg_err_info_bpe_ovf_unf CSR for error information.
        uint64_t        bpe_queue_unf  :  1; // A queue in the buffer partition has underflowed. See txotr_msg_err_info_bpe_ovf_unf CSR for error information.
        uint64_t          invalid_cmd  :  1; // Command received from TXCI is incomplete. - PIO does not contain enough payload. - Message Length exceeds amount allowable for a packet (not Rendezvous) - Command type is not expected to be MC0 - Command type is not expected to be MC1 See txotr_msg_err_info_invalid_cmd CSR for error information.
        uint64_t          unknown_cmd  :  1; // Command received from TXCI was mapped to an unknown command type encoding. See txotr_msg_err_info_unknown_cmd CSR for error information.
        uint64_t         rmessage_cmd  :  1; // Rendezvous Message received from TXCI, instead of being received from one of the Rendezvous Queues. See txotr_msg_err_info_rmessage_cmd CSR for error information.
        uint64_t        cancelled_msg  :  1; // Message in the OMB has been canceled. See txotr_msg_err_info_cancelled_msg CSR for error information.
        uint64_t       multicast_addr  :  1; // Portals command received from TXCI is targeting a multicast address range.
        uint64_t         UNUSED_63_21  : 43; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_STS_2_t;

// TXOTR_MSG_ERR_CLR_2 desc:
typedef union {
    struct {
        uint64_t               events  : 21; // Write 1's to clear corresponding status bits.
        uint64_t         UNUSED_63_21  : 43; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_CLR_2_t;

// TXOTR_MSG_ERR_FRC_2 desc:
typedef union {
    struct {
        uint64_t               events  : 21; // Write 1 to set corresponding status bits.
        uint64_t         UNUSED_63_21  : 43; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_FRC_2_t;

// TXOTR_MSG_ERR_EN_HOST_2 desc:
typedef union {
    struct {
        uint64_t               events  : 21; // Enables corresponding status bits to generate host interrupt signal.
        uint64_t         UNUSED_63_21  : 43; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_EN_HOST_2_t;

// TXOTR_MSG_ERR_FIRST_HOST_2 desc:
typedef union {
    struct {
        uint64_t               events  : 21; // Snapshot of status bits when host interrupt signal transitions from 0 to 1.
        uint64_t         UNUSED_63_21  : 43; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_FIRST_HOST_2_t;

// TXOTR_MSG_ERR_EN_BMC_2 desc:
typedef union {
    struct {
        uint64_t               events  : 21; // Enables corresponding status bits to generate quarantine interrupt signal.
        uint64_t         UNUSED_63_21  : 43; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_EN_BMC_2_t;

// TXOTR_MSG_ERR_FIRST_BMC_2 desc:
typedef union {
    struct {
        uint64_t               events  : 21; // Snapshot of status bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t         UNUSED_63_21  : 43; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_FIRST_BMC_2_t;

// TXOTR_MSG_ERR_EN_QUAR_2 desc:
typedef union {
    struct {
        uint64_t               events  : 21; // Enables corresponding status bits to generate BMC interrupt signal.
        uint64_t         UNUSED_63_21  : 43; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_EN_QUAR_2_t;

// TXOTR_MSG_ERR_FIRST_QUAR_2 desc:
typedef union {
    struct {
        uint64_t               events  : 21; // Snapshot of status bits when quarantine interrupt signal transitions from 0 to 1.
        uint64_t         UNUSED_63_21  : 43; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_FIRST_QUAR_2_t;

// TXOTR_MSG_ERR_INFO_BPE_FIRMWARE desc:
typedef union {
    struct {
        uint64_t             encoding  :  8; // Error encoding. TODO - delineate these encodings - Read or write to an invalid address
        uint64_t                 ctxt  :  5; // Context from which the error originated. TODO - equivalent for BPE?
        uint64_t         UNUSED_15_13  :  3; // Unused
        uint64_t               msg_id  : 16; // Message Identifier of the command associated with the error
        uint64_t               pkt_id  : 16; // Packet Identifier of the command associated with the error
        uint64_t         UNUSED_63_48  : 16; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_BPE_FIRMWARE_t;

// TXOTR_MSG_ERR_INFO_OMB_BE desc:
typedef union {
    struct {
        uint64_t               msg_id  : 16; // Message Identifier
        uint64_t             syndrome  :  8; // Syndrome. If multiple bit errors occur on a single read, this field indicates the syndrome of the least significant 64-bit portion of the read data, with priority given to an MBE.
        uint64_t                  mbe  :  8; // ECC domain containing the bit error
        uint64_t                  sbe  :  8; // ECC domain containing the bit error
        uint64_t         UNUSED_63_40  : 24; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_OMB_BE_t;

// TXOTR_MSG_ERR_INFO_MSGID_MEM_BE desc:
typedef union {
    struct {
        uint64_t              address  :  7; // Address
        uint64_t          UNUSED_15_7  :  9; // Unused
        uint64_t             syndrome  :  8; // Syndrome
        uint64_t                  mbe  :  1; // If set, indicates MBE, else SBE
        uint64_t         UNUSED_63_25  : 39; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_MSGID_MEM_BE_t;

// TXOTR_MSG_ERR_INFO_BPE_PROG_MEM_BE desc:
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
} TXOTR_MSG_ERR_INFO_BPE_PROG_MEM_BE_t;

// TXOTR_MSG_ERR_INFO_BPE_DATA_MEM_BE desc:
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
} TXOTR_MSG_ERR_INFO_BPE_DATA_MEM_BE_t;

// TXOTR_MSG_ERR_INFO_TXCI_CMD_BE desc:
typedef union {
    struct {
        uint64_t                  TBD  : 16; // TBD
        uint64_t             syndrome  :  8; // Syndrome. If multiple bit errors occur on a single read, this field indicates the syndrome of the least significant 64-bit portion of the read data, with priority given to an MBE.
        uint64_t                  mbe  :  4; // ECC domain containing the bit error
        uint64_t                  sbe  :  4; // ECC domain containing the bit error
        uint64_t         UNUSED_63_32  : 32; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_TXCI_CMD_BE_t;

// TXOTR_MSG_ERR_INFO_MSG_OVF_UNF desc:
typedef union {
    struct {
        uint64_t            OVF_QUEUE  :  8; // Encoding to denote which queue is overflowing
        uint64_t            UNF_QUEUE  :  8; // Encoding to denote which queue is underflowing
        uint64_t         UNUSED_63_16  : 48; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_MSG_OVF_UNF_t;

// TXOTR_MSG_ERR_INFO_BPE_OVF_UNF desc:
typedef union {
    struct {
        uint64_t            OVF_QUEUE  :  8; // Encoding to denote which queue is overflowing
        uint64_t            UNF_QUEUE  :  8; // Encoding to denote which queue is underflowing
        uint64_t         UNUSED_63_16  : 48; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_BPE_OVF_UNF_t;

// TXOTR_MSG_ERR_INFO_INVALID_CMD desc:
typedef union {
    struct {
        uint64_t             encoding  :  4; // 4'b1??? - Reserved 4'b?1?? - PIO does not contain enough payload 4'b??1? - Message Length exceeds MTU size for a non-Rendezvous command 4'b???1 - Unexpected command on MC0 or MC1
        uint64_t                  TBD  : 16; // TBD
        uint64_t         UNUSED_63_20  : 44; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_INVALID_CMD_t;

// TXOTR_MSG_ERR_INFO_UNKNOWN_CMD desc:
typedef union {
    struct {
        uint64_t                ctype  :  4; // Command Type of the command
        uint64_t                  cmd  :  7; // Cmd of the command
        uint64_t            UNUSED_11  :  1; // Unused
        uint64_t                iovec  :  1; // Cmd of the command
        uint64_t         UNUSED_63_13  : 51; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_UNKNOWN_CMD_t;

// TXOTR_MSG_ERR_INFO_RMESSAGE_CMD desc:
typedef union {
    struct {
        uint64_t                  TBD  : 16; // TBD
        uint64_t         UNUSED_63_16  : 48; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_RMESSAGE_CMD_t;

// TXOTR_MSG_ERR_INFO_CANCELLED_MSG desc:
typedef union {
    struct {
        uint64_t              msg_id0  : 16; // Most recent MSG_ID to be canceled
        uint64_t              msg_id1  : 16; // Second most recent MSG_ID to be canceled
        uint64_t              msg_id2  : 16; // Third most recent MSG_ID to be canceled
        uint64_t              msg_id3  : 16; // Fourth most recent MSG_ID to be canceled
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_CANCELLED_MSG_t;

// TXOTR_MSG_ERR_INFO_MULTICAST_ADDR desc:
typedef union {
    struct {
        uint64_t                  tbd  : 64; // TBD
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_MULTICAST_ADDR_t;

// TXOTR_MSG_DBG_OMB_ACCESS desc:
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
} TXOTR_MSG_DBG_OMB_ACCESS_t;

// TXOTR_MSG_DBG_OMB_PAYLOAD0 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data[63:0]
    } field;
    uint64_t val;
} TXOTR_MSG_DBG_OMB_PAYLOAD0_t;

// TXOTR_MSG_DBG_OMB_PAYLOAD1 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data[127:64]
    } field;
    uint64_t val;
} TXOTR_MSG_DBG_OMB_PAYLOAD1_t;

// TXOTR_MSG_DBG_OMB_PAYLOAD2 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data[191:128]
    } field;
    uint64_t val;
} TXOTR_MSG_DBG_OMB_PAYLOAD2_t;

// TXOTR_MSG_DBG_OMB_PAYLOAD3 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data[255:192]
    } field;
    uint64_t val;
} TXOTR_MSG_DBG_OMB_PAYLOAD3_t;

// TXOTR_MSG_DBG_OMB_PAYLOAD4 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data[319:256]
    } field;
    uint64_t val;
} TXOTR_MSG_DBG_OMB_PAYLOAD4_t;

// TXOTR_MSG_DBG_OMB_PAYLOAD5 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data[383:320]
    } field;
    uint64_t val;
} TXOTR_MSG_DBG_OMB_PAYLOAD5_t;

// TXOTR_MSG_DBG_OMB_PAYLOAD6 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data[447:384]
    } field;
    uint64_t val;
} TXOTR_MSG_DBG_OMB_PAYLOAD6_t;

// TXOTR_MSG_DBG_OMB_PAYLOAD7 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data[511:448]
    } field;
    uint64_t val;
} TXOTR_MSG_DBG_OMB_PAYLOAD7_t;

// TXOTR_MSG_DBG_MSGID_MEMORY desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data
    } field;
    uint64_t val;
} TXOTR_MSG_DBG_MSGID_MEMORY_t;

// TXOTR_MSG_DBG_BPE_PROG_MEM_ACCESS desc:
typedef union {
    struct {
        uint64_t              address  : 14; // Address for Read or Write. Valid address are from 0 to 16383.
        uint64_t       Reserved_51_14  : 38; // Unused
        uint64_t         Payload_regs  :  8; // Constant indicating number of payload registers follow
        uint64_t          Reserved_60  :  1; // Unused
        uint64_t                  ECC  :  1; // 0 = Read/Write Raw Data 1 = Generate ECC on Write, Correct Data on Read
        uint64_t                Write  :  1; // 0 = Read, 1 = Write
        uint64_t                Valid  :  1; // Set by software, cleared by hardware when complete.
    } field;
    uint64_t val;
} TXOTR_MSG_DBG_BPE_PROG_MEM_ACCESS_t;

// TXOTR_MSG_DBG_BPE_PROG_MEM_PAYLOAD0 desc:
typedef union {
    struct {
        uint64_t                 Data  : 32; // Data[31:0]
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_DBG_BPE_PROG_MEM_PAYLOAD0_t;

// TXOTR_MSG_DBG_BPE_DATA_MEM_ACCESS desc:
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
} TXOTR_MSG_DBG_BPE_DATA_MEM_ACCESS_t;

// TXOTR_MSG_DBG_BPE_DATA_MEM_PAYLOAD0 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data[63:0]
    } field;
    uint64_t val;
} TXOTR_MSG_DBG_BPE_DATA_MEM_PAYLOAD0_t;

// TXOTR_MSG_DBG_BPE_CONTEXT_AVAILABLE_REG desc:
typedef union {
    struct {
        uint64_t                  CTS  :  4; // CTS Context-Available Register of BPE
        uint64_t              TIMEOUT  :  8; // Timeout Context-Available Register of BPE
        uint64_t                 NACK  :  4; // NACK Context-Available Register of BPE
        uint64_t         UNUSED_63_16  : 48; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_DBG_BPE_CONTEXT_AVAILABLE_REG_t;

// TXOTR_MSG_DBG_OMB_MSG_COUNT desc:
typedef union {
    struct {
        uint64_t                count  : 64; // Count
    } field;
    uint64_t val;
} TXOTR_MSG_DBG_OMB_MSG_COUNT_t;

// TXOTR_MSG_DBG_OMB_RENDEZ_MSG_COUNT desc:
typedef union {
    struct {
        uint64_t                count  : 64; // Count
    } field;
    uint64_t val;
} TXOTR_MSG_DBG_OMB_RENDEZ_MSG_COUNT_t;

// TXOTR_MSG_DBG_OMB_REPLY_PKT_COUNT desc:
typedef union {
    struct {
        uint64_t                count  : 64; // Count
    } field;
    uint64_t val;
} TXOTR_MSG_DBG_OMB_REPLY_PKT_COUNT_t;

// TXOTR_MSG_DBG_RXDMA_CMD_COUNT desc:
typedef union {
    struct {
        uint64_t                count  : 64; // Count
    } field;
    uint64_t val;
} TXOTR_MSG_DBG_RXDMA_CMD_COUNT_t;

// TXOTR_MSG_DBG_CTS_LIST_POINTERS0 desc:
typedef union {
    struct {
        uint64_t             imm_head  : 14; // Immediate List Head
        uint64_t         UNUSED_15_14  :  2; // Unused
        uint64_t             imm_tail  : 14; // Immediate List Tail
        uint64_t         UNUSED_31_30  :  2; // Unused
        uint64_t           delay_head  : 14; // Delay List Head
        uint64_t         UNUSED_47_46  :  2; // Unused
        uint64_t           delay_tail  : 14; // Delay List Tail
        uint64_t         UNUSED_63_62  :  2; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_DBG_CTS_LIST_POINTERS0_t;

// TXOTR_MSG_DBG_CTS_LIST_POINTERS1 desc:
typedef union {
    struct {
        uint64_t             imm_head  : 14; // Immediate List Head
        uint64_t         UNUSED_15_14  :  2; // Unused
        uint64_t             imm_tail  : 14; // Immediate List Tail
        uint64_t         UNUSED_31_30  :  2; // Unused
        uint64_t           delay_head  : 14; // Delay List Head
        uint64_t         UNUSED_47_46  :  2; // Unused
        uint64_t           delay_tail  : 14; // Delay List Tail
        uint64_t         UNUSED_63_62  :  2; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_DBG_CTS_LIST_POINTERS1_t;

// TXOTR_MSG_DBG_CTS_LIST_POINTERS2 desc:
typedef union {
    struct {
        uint64_t             imm_head  : 14; // Immediate List Head
        uint64_t         UNUSED_15_14  :  2; // Unused
        uint64_t             imm_tail  : 14; // Immediate List Tail
        uint64_t         UNUSED_31_30  :  2; // Unused
        uint64_t           delay_head  : 14; // Delay List Head
        uint64_t         UNUSED_47_46  :  2; // Unused
        uint64_t           delay_tail  : 14; // Delay List Tail
        uint64_t         UNUSED_63_62  :  2; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_DBG_CTS_LIST_POINTERS2_t;

// TXOTR_MSG_DBG_CTS_LIST_POINTERS3 desc:
typedef union {
    struct {
        uint64_t             imm_head  : 14; // Immediate List Head
        uint64_t         UNUSED_15_14  :  2; // Unused
        uint64_t             imm_tail  : 14; // Immediate List Tail
        uint64_t         UNUSED_31_30  :  2; // Unused
        uint64_t           delay_head  : 14; // Delay List Head
        uint64_t         UNUSED_47_46  :  2; // Unused
        uint64_t           delay_tail  : 14; // Delay List Tail
        uint64_t         UNUSED_63_62  :  2; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_DBG_CTS_LIST_POINTERS3_t;

// TXOTR_MSG_DBG_OUT_RENDEZ_TRANS_COUNT desc:
typedef union {
    struct {
        uint64_t                count  : 64; // Count
    } field;
    uint64_t val;
} TXOTR_MSG_DBG_OUT_RENDEZ_TRANS_COUNT_t;

// TXOTR_MSG_DBG_MC0_OUTSTANDING_MSG_CNT desc:
typedef union {
    struct {
        uint64_t                  tc0  : 16; // TC0 outstanding message count.
        uint64_t                  tc1  : 16; // TC1 outstanding message count.
        uint64_t                  tc2  : 16; // TC2 outstanding message count.
        uint64_t                  tc3  : 16; // TC3 outstanding message count.
    } field;
    uint64_t val;
} TXOTR_MSG_DBG_MC0_OUTSTANDING_MSG_CNT_t;

// TXOTR_MSG_DBG_MC1_OUTSTANDING_MSG_CNT desc:
typedef union {
    struct {
        uint64_t                  tc0  : 16; // TC0 outstanding message count.
        uint64_t                  tc1  : 16; // TC1 outstanding message count.
        uint64_t                  tc2  : 16; // TC2 outstanding message count.
        uint64_t                  tc3  : 16; // TC3 outstanding message count.
    } field;
    uint64_t val;
} TXOTR_MSG_DBG_MC1_OUTSTANDING_MSG_CNT_t;

// TXOTR_MSG_PRF_STALL_OMB_ENTRIES_X desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  3; // 3'd7 - MC1TC3 3'd6 - MC1TC2 3'd5 - MC1TC1 3'd4 - MC1TC0 3'd3 - MC0TC3 3'd2 - MC0TC2 3'd1 - MC0TC1 3'd0 - MC0TC0
        uint64_t          UNUSED_63_3  : 61; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_PRF_STALL_OMB_ENTRIES_X_t;

// TXOTR_MSG_PRF_STALL_OMB_ENTRIES_Y desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  3; // 3'd7 - MC1TC3 3'd6 - MC1TC2 3'd5 - MC1TC1 3'd4 - MC1TC0 3'd3 - MC0TC3 3'd2 - MC0TC2 3'd1 - MC0TC1 3'd0 - MC0TC0
        uint64_t          UNUSED_63_3  : 61; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_PRF_STALL_OMB_ENTRIES_Y_t;

// TXOTR_MSG_PRF_STALL_RXDMA_CREDITS_X desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  3; // 3'd7 - MC1TC3 3'd6 - MC1TC2 3'd5 - MC1TC1 3'd4 - MC1TC0 3'd3 - MC0TC3 3'd2 - MC0TC2 3'd1 - MC0TC1 3'd0 - MC0TC0
        uint64_t          UNUSED_63_3  : 61; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_PRF_STALL_RXDMA_CREDITS_X_t;

// TXOTR_MSG_PRF_STALL_RXDMA_CREDITS_Y desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  3; // 3'd7 - MC1TC3 3'd6 - MC1TC2 3'd5 - MC1TC1 3'd4 - MC1TC0 3'd3 - MC0TC3 3'd2 - MC0TC2 3'd1 - MC0TC1 3'd0 - MC0TC0
        uint64_t          UNUSED_63_3  : 61; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_PRF_STALL_RXDMA_CREDITS_Y_t;

// TXOTR_MSG_PRF_STALL_M_TO_P_CREDITS_X desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  4; // 4'd15-4'd12 - Reserved 4'd11 - MC1'TC3 4'd10 - MC1'TC2 4'd9 - MC1'TC1 4'd8 - MC1'TC0 4'd7 - MC1TC3 4'd6 - MC1TC2 4'd5 - MC1TC1 4'd4 - MC1TC0 4'd3 - MC0TC3 4'd2 - MC0TC2 4'd1 - MC0TC1 4'd0 - MC0TC0
        uint64_t          UNUSED_63_4  : 60; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_PRF_STALL_M_TO_P_CREDITS_X_t;

// TXOTR_MSG_PRF_STALL_M_TO_P_CREDITS_Y desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  4; // 4'd15-4'd12 - Reserved 4'd11 - MC1'TC3 4'd10 - MC1'TC2 4'd9 - MC1'TC1 4'd8 - MC1'TC0 4'd7 - MC1TC3 4'd6 - MC1TC2 4'd5 - MC1TC1 4'd4 - MC1TC0 4'd3 - MC0TC3 4'd2 - MC0TC2 4'd1 - MC0TC1 4'd0 - MC0TC0
        uint64_t          UNUSED_63_4  : 60; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_PRF_STALL_M_TO_P_CREDITS_Y_t;

// TXOTR_MSG_PRF_STALL_PREFRAG_CREDITS_X desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  3; // 3'd7 - MC1TC3 3'd6 - MC1TC2 3'd5 - MC1TC1 3'd4 - MC1TC0 3'd3 - MC0TC3 3'd2 - MC0TC2 3'd1 - MC0TC1 3'd0 - MC0TC0
        uint64_t          UNUSED_63_3  : 61; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_PRF_STALL_PREFRAG_CREDITS_X_t;

// TXOTR_MSG_PRF_STALL_PREFRAG_CREDITS_Y desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  3; // 3'd7 - MC1TC3 3'd6 - MC1TC2 3'd5 - MC1TC1 3'd4 - MC1TC0 3'd3 - MC0TC3 3'd2 - MC0TC2 3'd1 - MC0TC1 3'd0 - MC0TC0
        uint64_t          UNUSED_63_3  : 61; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_PRF_STALL_PREFRAG_CREDITS_Y_t;

#endif /* ___FXR_tx_otr_msg_top_CSRS_H__ */