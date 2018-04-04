// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Thu Mar 29 15:03:56 2018
//

#ifndef ___FXR_tx_otr_msg_top_CSRS_H__
#define ___FXR_tx_otr_msg_top_CSRS_H__

// TXOTR_MSG_CFG_TXCI_MC0_CRDTS desc:
typedef union {
    struct {
        uint64_t                  TC0  :  5; // TC0 Input Queue Credits
        uint64_t         Reserved_7_5  :  3; // Unused
        uint64_t                  TC1  :  5; // TC1 Input Queue Credits
        uint64_t       Reserved_15_13  :  3; // Unused
        uint64_t                  TC2  :  5; // TC2 Input Queue Credits
        uint64_t       Reserved_23_21  :  3; // Unused
        uint64_t                  TC3  :  5; // TC3 Input Queue Credits
        uint64_t       Reserved_63_29  : 35; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_TXCI_MC0_CRDTS_t;

// TXOTR_MSG_CFG_TXCI_MC1_CRDTS desc:
typedef union {
    struct {
        uint64_t                  TC0  :  5; // TC0 Input Queue Credits
        uint64_t         Reserved_7_5  :  3; // Unused
        uint64_t                  TC1  :  5; // TC1 Input Queue Credits
        uint64_t       Reserved_15_13  :  3; // Unused
        uint64_t                  TC2  :  5; // TC2 Input Queue Credits
        uint64_t       Reserved_23_21  :  3; // Unused
        uint64_t                  TC3  :  5; // TC3 Input Queue Credits
        uint64_t       Reserved_63_29  : 35; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_TXCI_MC1_CRDTS_t;

// TXOTR_MSG_CFG_TXCI_MC1P_CRDTS desc:
typedef union {
    struct {
        uint64_t                  TC0  :  5; // TC0 Input Queue Credits
        uint64_t         Reserved_7_5  :  3; // Unused
        uint64_t                  TC1  :  5; // TC1 Input Queue Credits
        uint64_t       Reserved_15_13  :  3; // Unused
        uint64_t                  TC2  :  5; // TC2 Input Queue Credits
        uint64_t       Reserved_23_21  :  3; // Unused
        uint64_t                  TC3  :  5; // TC3 Input Queue Credits
        uint64_t       Reserved_63_29  : 35; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_TXCI_MC1P_CRDTS_t;

// TXOTR_MSG_CFG_RX_OMB_CRDTS desc:
typedef union {
    struct {
        uint64_t                  TC0  :  5; // TC0 Input Queue Credits
        uint64_t         Reserved_7_5  :  3; // Unused
        uint64_t                  TC1  :  5; // TC1 Input Queue Credits
        uint64_t       Reserved_15_13  :  3; // Unused
        uint64_t                  TC2  :  5; // TC2 Input Queue Credits
        uint64_t       Reserved_23_21  :  3; // Unused
        uint64_t                  TC3  :  5; // TC3 Input Queue Credits
        uint64_t       Reserved_63_29  : 35; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_RX_OMB_CRDTS_t;

// TXOTR_MSG_CFG_PREFRAG_OPB_FIFO_CRDTS desc:
typedef union {
    struct {
        uint64_t               MC0TC0  :  3; // MC0/TC0 Pre-fragmentation OPB Input Queue Credits
        uint64_t           Reserved_3  :  1; // Unused
        uint64_t               MC0TC1  :  3; // MC0/TC1 Pre-fragmentation OPB Input Queue Credits
        uint64_t           Reserved_7  :  1; // Unused
        uint64_t               MC0TC2  :  3; // MC0/TC2 Pre-fragmentation OPB Input Queue Credits
        uint64_t          Reserved_11  :  1; // Unused
        uint64_t               MC0TC3  :  3; // MC0/TC3 Pre-fragmentation OPB Input Queue Credits
        uint64_t          Reserved_15  :  1; // Unused
        uint64_t               MC1TC0  :  3; // MC1/TC0 Pre-fragmentation OPB Input Queue Credits
        uint64_t          Reserved_19  :  1; // Unused
        uint64_t               MC1TC1  :  3; // MC1/TC1 Pre-fragmentation OPB Input Queue Credits
        uint64_t          Reserved_23  :  1; // Unused
        uint64_t               MC1TC2  :  3; // MC1/TC2 Pre-fragmentation OPB Input Queue Credits
        uint64_t          Reserved_27  :  1; // Unused
        uint64_t               MC1TC3  :  3; // MC1/TC3 Pre-fragmentation OPB Input Queue Credits
        uint64_t       Reserved_63_31  : 33; // Unused
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
        uint64_t       Reserved_63_60  :  4; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_FP_OPB_FIFO_CRDTS_t;

// TXOTR_MSG_CFG_MSGID_1 desc:
typedef union {
    struct {
        uint64_t         RSVD_MC0_TC0  :  7; // The number of MC0/TC0 Reserved Message Identifiers
        uint64_t         RSVD_MC0_TC1  :  7; // The number of MC0/TC1 Reserved Message Identifiers
        uint64_t         RSVD_MC0_TC2  :  7; // The number of MC0/TC2 Reserved Message Identifiers
        uint64_t         RSVD_MC0_TC3  :  7; // The number of MC0/TC3 Reserved Message Identifiers
        uint64_t         RSVD_MC1_TC0  :  7; // The number of MC1/TC0 Reserved Message Identifiers
        uint64_t         RSVD_MC1_TC1  :  7; // The number of MC1/TC1 Reserved Message Identifiers
        uint64_t         RSVD_MC1_TC2  :  7; // The number of MC1/TC2 Reserved Message Identifiers
        uint64_t         RSVD_MC1_TC3  :  7; // The number of MC1/TC3 Reserved Message Identifiers
        uint64_t       Reserved_63_56  :  8; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_MSGID_1_t;

// TXOTR_MSG_CFG_MSGID_2 desc:
typedef union {
    struct {
        uint64_t               SHARED  : 14; // The number of Shared Message Identifiers
        uint64_t       Reserved_63_14  : 50; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_MSGID_2_t;

// TXOTR_MSG_CFG_IGNORE_NR desc:
typedef union {
    struct {
        uint64_t            IGNORE_NR  :  1; // This bit can be set to ignore the NR bit
        uint64_t        Reserved_63_1  : 63; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_IGNORE_NR_t;

// TXOTR_MSG_CFG_TRANSMIT_DELAY desc:
typedef union {
    struct {
        uint64_t               SCALER  : 32; // This register selects the rate at which the TXOTR Delay Counter increments. If set to 32'd0, the counter is disabled. If set to 1, the counter increments once per clock cycle. If set to 2, the counter increments once every other clock cycle. Et cetera.
        uint64_t IGNORE_TRANSMIT_DELAY  :  1; // If this bit is set, the Transmit Delay in a CTS is ignored resulting in new CTS messages being added to the end of the Pending List and not to the Scheduled List.
        uint64_t       Reserved_63_33  : 31; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_TRANSMIT_DELAY_t;

// TXOTR_MSG_CFG_CANCEL_MSG_REQ_1 desc:
typedef union {
    struct {
        uint64_t                 busy  :  1; // Request to cancel a message in OMB. Software should write this bit to 1'b1 when requesting to cancel. When hardware has finished processing the cancel request, this bit is cleared.
        uint64_t         Reserved_3_1  :  3; // Unused
        uint64_t                 ipid  : 12; // Initiator Process ID. Messages in the OMB which match this Process ID (subjected to ipid_mask ) are canceled when busy is valid.
        uint64_t            ipid_mask  : 12; // Initiator Process ID Mask. See description in ipid .
        uint64_t            md_handle  : 11; // Memory Domain Handle. Messages in the OMB which match this MD Handle (subjected to md_handle_mask ) are canceled when busy is valid.
        uint64_t          Reserved_39  :  1; // Unused
        uint64_t       md_handle_mask  : 11; // Memory Domain Handle Mask. See description in md_handle .
        uint64_t          Reserved_51  :  1; // Unused
        uint64_t                   tc  :  2; // Traffic Class. Messages in the OMB which match this Traffic Class (subjected to tc_mask ) are canceled when busy is valid.
        uint64_t              tc_mask  :  2; // Traffic Class Mask. See description in tc .
        uint64_t       Reserved_63_56  :  8; // Unused
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
        uint64_t                 dlid  : 24; // Destination Local Identifier. Messages in the OMB which match this DLID (subjected to dlid_mask ) are canceled when busy is valid.
        uint64_t            dlid_mask  : 24; // Destination Local Identifier Mask. See description in dlid .
        uint64_t                 pkey  : 16; // Protection Key. Messages in the OMB which match this PKEY are canceled when busy is valid. Note that there is no corresponding mask field for PKEY.
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
        uint64_t       Reserved_63_15  : 49; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_FORCE_MSG_TO_FPE_t;

// TXOTR_MSG_CFG_RENDEZVOUS desc:
typedef union {
    struct {
        uint64_t    disallow_rmessage  :  1; // Disallow Rendezvous Messages (RMessage) coming from TXCI
        uint64_t        Reserved_63_1  : 63; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_RENDEZVOUS_t;

// TXOTR_MSG_CFG_RFS desc:
typedef union {
    struct {
        uint64_t         mc0tc0_shift  :  3; // Rendezvous Fragment Size for MC0/TC0. Calculated using the following formula: 1 << (7 + mc*tc*_shift) 3'd0 - 128B 3'd1 - 256B 3'd2 - 512B 3'd3 - 1KB 3'd4 - 2KB 3'd5 - 4KB 3'd6 - 8KB 3'd7 - Not valid: defaults to 8KB
        uint64_t           Reserved_3  :  1; // Unused
        uint64_t         mc0tc1_shift  :  3; // Rendezvous Fragment Size for MC0/TC1. See calculation below.
        uint64_t           Reserved_7  :  1; // Unused
        uint64_t         mc0tc2_shift  :  3; // Rendezvous Fragment Size for MC0/TC2. See calculation below.
        uint64_t          Reserved_11  :  1; // Unused
        uint64_t         mc0tc3_shift  :  3; // Rendezvous Fragment Size for MC0/TC3. See calculation below.
        uint64_t          Reserved_15  :  1; // Unused
        uint64_t         mc1tc0_shift  :  3; // Rendezvous Fragment Size for MC1/TC0. See calculation below.
        uint64_t          Reserved_19  :  1; // Unused
        uint64_t         mc1tc1_shift  :  3; // Rendezvous Fragment Size for MC1/TC1. See calculation below.
        uint64_t          Reserved_23  :  1; // Unused
        uint64_t         mc1tc2_shift  :  3; // Rendezvous Fragment Size for MC1/TC2. See calculation below.
        uint64_t          Reserved_27  :  1; // Unused
        uint64_t         mc1tc3_shift  :  3; // Rendezvous Fragment Size for MC1/TC3. See calculation below.
        uint64_t       Reserved_63_31  : 33; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_RFS_t;

// TXOTR_MSG_CFG_RENDEZVOUS_RC desc:
typedef union {
    struct {
        uint64_t                port0  :  3; // Port 0 Routing Control Bits
        uint64_t        Reserved_31_3  : 29; // Unused
        uint64_t            rendz_tc0  :  3; // Rendezvous TC 0 Routing Control Bits
        uint64_t          Reserved_35  :  1; // Unused
        uint64_t            rendz_tc1  :  3; // Rendezvous TC 1 Routing Control Bits
        uint64_t          Reserved_39  :  1; // Unused
        uint64_t            rendz_tc2  :  3; // Rendezvous TC 2 Routing Control Bits
        uint64_t          Reserved_43  :  1; // Unused
        uint64_t            rendz_tc3  :  3; // Rendezvous TC 3 Routing Control Bits
        uint64_t          Reserved_47  :  1; // Unused
        uint64_t             mc1p_tc0  :  3; // MC1p TC 0 Routing Control Bits
        uint64_t          Reserved_51  :  1; // Unused
        uint64_t             mc1p_tc1  :  3; // MC1p TC 1 Routing Control Bits
        uint64_t          Reserved_55  :  1; // Unused
        uint64_t             mc1p_tc2  :  3; // MC1p TC 2 Routing Control Bits
        uint64_t          Reserved_59  :  1; // Unused
        uint64_t             mc1p_tc3  :  3; // MC1p TC 3 Routing Control Bits
        uint64_t          Reserved_63  :  1; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_RENDEZVOUS_RC_t;

// TXOTR_MSG_CFG_TIMEOUT desc:
typedef union {
    struct {
        uint64_t        Reserved_35_0  : 36; // Unused
        uint64_t              TIMEOUT  : 12; // Timeout comparison value. A timeout has occurred when the difference between the time stamp in a given Hash Table entry and the time current time stamp has exceeded this value. This value must exactly match the timeout value in TXOTR_PKT_CFG_TIMEOUT .
        uint64_t       Reserved_63_48  : 16; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_TIMEOUT_t;

// TXOTR_MSG_CFG_SMALL_HEADER desc:
typedef union {
    struct {
        uint64_t               ENABLE  :  1; // Enable small headers. If set to 1'b1, small headers are enabled. If set to 1'b0, all packets are forced to use long header formats.
        uint64_t              PKEY_8B  : 16; // Pkey value for enabling the Small Header usage in the Portals packets. Small Header can only be used if the PKEY in the command matches the configured PKEY_8B CSR field, else packets would be forced to use the 16B Headers.
        uint64_t       Reserved_63_17  : 47; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_SMALL_HEADER_t;

// TXOTR_MSG_CFG_ACK_REFUSED desc:
typedef union {
    struct {
        uint64_t               enable  :  1; // When set to 1'b0 (default), both CT and EQ ACK events are suppressed when a Portals ACK is received with an opcode PTL_ACK_REFUSED. When set to 1'b1 (override), CT and EQ ACK events are not suppressed when a Portals ACK is received with an opcode PTL_ACK_REFUSED. If the other event delivery criteria are satisfied, EQ events are delivered with a fail type of 'PTL_NI_ACK_REFUSED' and CT events increment the 'success' field.
        uint64_t        Reserved_63_1  : 63; // Unused
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
        uint64_t          PD_RST_MASK  :  1; // A value of zero mask off the freset assertion to the PD authentication logic. If this bit is set to '1' every assertion of 'freset' would need a PD re-authentication.
        uint64_t        Reserved_63_4  : 60; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_BUFF_ENG_t;

// TXOTR_MSG_CFG_QUIESCE desc:
typedef union {
    struct {
        uint64_t       quiesce_enable  :  1; // Set this bit to start OTR quiesce condition
        uint64_t     quiesce_complete  :  1; // When sampled as '1', affirms the OTR_MSG has no pending commands to process.
        uint64_t        Reserved_63_2  : 62; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_QUIESCE_t;

// TXOTR_MSG_CFG_MLID_MASK desc:
typedef union {
    struct {
        uint64_t         MLID_MASK_8B  :  7; // Multi cast LID Mask
        uint64_t        Reserved_63_7  : 57; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_MLID_MASK_t;

// TXOTR_MSG_CFG_SVEN_ADDR desc:
typedef union {
    struct {
        uint64_t              address  : 64; // Address to be used for IOSF transactions
    } field;
    uint64_t val;
} TXOTR_MSG_CFG_SVEN_ADDR_t;

// TXOTR_MSG_ERR_STS desc:
typedef union {
    struct {
        uint64_t       tail_abort_err  :  1; // Error indication received from TXCI for a PIO which has timed out while being written to a CQ. See TXOTR_MSG_ERR_INFO_INVALID_CMD CSR for error information. ERR_CATEGORY_CORRECTABLE
        uint64_t   tail_flit_miss_err  :  1; // Multiple head flits with no tail between commands received from TXCI. See TXOTR_MSG_ERR_INFO_INVALID_CMD CSR for error information. ERR_CATEGORY_HFI
        uint64_t   head_flit_miss_err  :  1; // Multiple tail flits with no head between commands received from TXCI. See TXOTR_MSG_ERR_INFO_INVALID_CMD CSR for error information. ERR_CATEGORY_HFI
        uint64_t       cmd_length_err  :  1; // Malformed length packet from TXCI. OTR checks the cmd_length against the tail location. See TXOTR_MSG_ERR_INFO_INVALID_CMD CSR for error information. ERR_CATEGORY_HFI
        uint64_t              cmd_mbe  :  1; // Multiple BIt Error in command from TXCI or Buffer Programmable Engine Error Flag. See TXOTR_MSG_ERR_INFO_TXCI_CMD_BE_0 & TXOTR_MSG_ERR_INFO_TXCI_CMD_BE_1 CSR for error information. ERR_CATEGORY_HFI
        uint64_t              cmd_sbe  :  1; // Single Bit Error in command from TXCI or Buffer Programmable Engine Error Flag. See TXOTR_MSG_ERR_INFO_TXCI_CMD_BE_0 & TXOTR_MSG_ERR_INFO_TXCI_CMD_BE_1 CSR for error information. ERR_CATEGORY_CORRECTABLE
        uint64_t         omb_fifo_mbe  :  1; // Multi Bit Error for reads from the OMB fifo. See TXOTR_MSG_ERR_INFO_OMB_FIFO_0 & TXOTR_MSG_ERR_INFO_OMB_FIFO_1 CSR for error information. ERR_CATEGORY_HFI
        uint64_t         omb_fifo_sbe  :  1; // Single Bit Error for reads from the OMB fifo. See TXOTR_MSG_ERR_INFO_OMB_FIFO_0 & TXOTR_MSG_ERR_INFO_OMB_FIFO_1 CSR for error information. ERR_CATEGORY_CORRECTABLE
        uint64_t     omb_fifo_undflow  :  1; // OMB fifo under flow error See TXOTR_MSG_ERR_INFO_OMB_FIFO_0 & TXOTR_MSG_ERR_INFO_OMB_FIFO_1 CSR for error information. ERR_CATEGORY_HFI
        uint64_t     omb_fifo_ovrflow  :  1; // OMB fifo over flow error See TXOTR_MSG_ERR_INFO_OMB_FIFO_0 & TXOTR_MSG_ERR_INFO_OMB_FIFO_1 CSR for error information. ERR_CATEGORY_HFI
        uint64_t       rendz_fifo_mbe  :  1; // Multiple BIt Error for reads from the Rendezvous fifo. See TXOTR_MSG_ERR_INFO_RENDZ_FIFO_0 & TXOTR_MSG_ERR_INFO_RENDZ_FIFO_1 for error information. ERR_CATEGORY_HFI
        uint64_t       rendz_fifo_sbe  :  1; // Single BIt Error for reads from the Rendezvous fifo. See TXOTR_MSG_ERR_INFO_RENDZ_FIFO_0 & TXOTR_MSG_ERR_INFO_RENDZ_FIFO_1 for error information. ERR_CATEGORY_CORRECTABLE
        uint64_t   rendz_fifo_undflow  :  1; // Rendezvous fifo under flow error See TXOTR_MSG_ERR_INFO_RENDZ_FIFO_0 & TXOTR_MSG_ERR_INFO_RENDZ_FIFO_1 for error information. ERR_CATEGORY_HFI
        uint64_t   rendz_fifo_ovrflow  :  1; // Rendezvous fifo over flow error See TXOTR_MSG_ERR_INFO_RENDZ_FIFO_0 & TXOTR_MSG_ERR_INFO_RENDZ_FIFO_1 for error information. ERR_CATEGORY_HFI
        uint64_t        msgid_mem_par  :  1; // Parity Error for reads from the message ID memory. See TXOTR_MSG_ERR_INFO_MSGID_MEM_BE CSR for error information. ERR_CATEGORY_HFI
        uint64_t             pkt_fail  :  1; // Packet fail error. See TXOTR_MSG_ERR_INFO_INVALID_CMD CSR for error information. ERR_CATEGORY_INFO
        uint64_t              omb_mbe  :  1; // Outstanding Message Buffer MBE Error Flag. See TXOTR_MSG_ERR_INFO_OMB_BE_0 CSR, TXOTR_MSG_ERR_INFO_OMB_BE_1 CSR, TXOTR_MSG_ERR_INFO_OMB_BE_2 CSR, and TXOTR_MSG_ERR_INFO_OMB_BE_3 CSR for error information. ERR_CATEGORY_HFI
        uint64_t              omb_sbe  :  1; // Outstanding Message Buffer SBE Error Flag. See TXOTR_MSG_ERR_INFO_OMB_BE_0 CSR, TXOTR_MSG_ERR_INFO_OMB_BE_1 CSR, TXOTR_MSG_ERR_INFO_OMB_BE_2 CSR, and TXOTR_MSG_ERR_INFO_OMB_BE_3 CSR for error information. ERR_CATEGORY_CORRECTABLE
        uint64_t      rx_rsp_intf_mbe  :  1; // Multiple Bit Error for reads from the RX response interface FIFO. See TXOTR_MSG_ERR_INFO_RX_RSP_INTF_BE_0 CSR and TXOTR_MSG_ERR_INFO_RX_RSP_INTF_BE_1 CSR for error information. ERR_CATEGORY_HFI
        uint64_t      rx_rsp_intf_sbe  :  1; // Single Bit Error for reads from the RX response interface FIFO. See TXOTR_MSG_ERR_INFO_RX_RSP_INTF_BE_0 CSR and TXOTR_MSG_ERR_INFO_RX_RSP_INTF_BE_1 CSR for error information. ERR_CATEGORY_CORRECTABLE
        uint64_t   rx_rsp_ombctrl_mbe  :  1; // Multiple BIt Error for reads from the OMB RX response control FIFO. See TXOTR_MSG_ERR_INFO_RX_RSP_OMBCTRL_BE CSR for error information. ERR_CATEGORY_HFI
        uint64_t   rx_rsp_ombctrl_sbe  :  1; // Single BIt Error for reads from the OMB RX response control FIFO. See TXOTR_MSG_ERR_INFO_RX_RSP_OMBCTRL_BE CSR for error information. ERR_CATEGORY_CORRECTABLE
        uint64_t rx_rsp_ombctrl_undflow  :  1; // OMB RX response control FIFO underflow. See TXOTR_MSG_ERR_INFO_RX_RSP_OMBCTRL_BE for error information ERR_CATEGORY_HFI
        uint64_t rx_rsp_ombctrl_overflow  :  1; // OMB RX response control FIFO over flow See TXOTR_MSG_ERR_INFO_RX_RSP_OMBCTRL_BE for error information ERR_CATEGORY_HFI
        uint64_t rx_rsp_ombstate_undflow  :  1; // OMB RX response state FIFO underflow See TXOTR_MSG_ERR_INFO_RX_RSP_OMBCTRL_BE for error information ERR_CATEGORY_HFI
        uint64_t rx_rsp_ombstate_overflow  :  1; // OMB RX response state FIFO over flow. See TXOTR_MSG_ERR_INFO_RX_RSP_OMBCTRL_BE for error information ERR_CATEGORY_HFI
        uint64_t rx_rsp_rxdma_cmd_fifo_mbe  :  1; // OMB RX RSP RXDMA command FIFO multi bit error.See TXOTR_MSG_ERR_INFO_RX_RSP_RXDMA_CMD_BE CSR for error information. ERR_CATEGORY_HFI
        uint64_t rx_rsp_rxdma_cmd_fifo_sbe  :  1; // OMB RX RSP RXDMA command FIFO single bit error. See TXOTR_MSG_ERR_INFO_RX_RSP_RXDMA_CMD_BE CSR for error information. ERR_CATEGORY_CORRECTABLE
        uint64_t rx_rsp_rxdma_cmd_fifo_undflow  :  1; // OMB RX RSP RXDMA command FIFO under flow See TXOTR_MSG_ERR_INFO_RX_RSP_RXDMA_CMD CSR for error information. ERR_CATEGORY_HFI
        uint64_t rx_rsp_rxdma_cmd_fifo_overflow  :  1; // OMB RX RSP RXDMA command FIFO over flow. See TXOTR_MSG_ERR_INFO_RX_RSP_RXDMA_CMD CSR for error information. ERR_CATEGORY_HFI
        uint64_t omb_rxet_fifo_undflow  :  1; // OMB RXET FIFO underflow. See TXOTR_MSG_ERR_INFO_RX_RSP_RXDMA_CMD CSR for error information. ERR_CATEGORY_HFI
        uint64_t omb_rxet_fifo_overflow  :  1; // OMB RXET FIFO over flow. See TXOTR_MSG_ERR_INFO_RX_RSP_RXDMA_CMD CSR for error information. ERR_CATEGORY_HFI
        uint64_t    nr_bit_set_omb_rd  :  1; // No retransmit bit is set in the OMB read when BPE issues a read of the OMB. See TXOTR_MSG_ERR_INFO_NR_BIT_SET CSR for error information. ERR_CATEGORY_INFO
        uint64_t     unknown_txci_cmd  :  1; // Command received from TXCI was mapped to an unknown command type encoding. See TXOTR_MSG_ERR_INFO_UNKNOWN_CMD CSR for error information. ERR_CATEGORY_INFO
        uint64_t        rmsg_txci_cmd  :  1; // RMessage (or REvent) received from TXCI interface See TXOTR_MSG_ERR_INFO_RMESSAGE_CMD CSR for error information. ERR_CATEGORY_INFO
        uint64_t       unexpected_cmd  :  1; // OTR receives a command on a message class which does not typically carry that command type See TXOTR_MSG_ERR_INFO_RMESSAGE_CMD CSR for error information. ERR_CATEGORY_INFO
        uint64_t        cts_queue_mbe  :  1; // Multi bit error in CTS queues in BPE. See TXOTR_MSG_ERR_INFO_CTS_QUEUE_BE CSR for error information. ERR_CATEGORY_HFI
        uint64_t        cts_queue_sbe  :  1; // Single bit error in CTS queues in BPE. See TXOTR_MSG_ERR_INFO_CTS_QUEUE_BE CSR for error information. ERR_CATEGORY_CORRECTABLE
        uint64_t    cts_queue_undflow  :  1; // Under flow detected in the CTS queues in BPE. See TXOTR_MSG_ERR_INFO_CTS_QUEUE_BE CSR for error information. ERR_CATEGORY_HFI
        uint64_t   cts_queue_overflow  :  1; // Overflow detected in CTS queues in BPE. See TXOTR_MSG_ERR_INFO_CTS_QUEUE_BE CSR for error information. ERR_CATEGORY_HFI
        uint64_t    timeout_queue_mbe  :  1; // Multi bit error in Timeout queues. See TXOTR_MSG_ERR_INFO_TIMEOUT_QUEUE_BE CSR for error information. ERR_CATEGORY_HFI
        uint64_t    timeout_queue_sbe  :  1; // Single bit error in Timeout queues. See TXOTR_MSG_ERR_INFO_TIMEOUT_QUEUE_BE CSR for error information. ERR_CATEGORY_CORRECTABLE
        uint64_t timeout_queue_undflow  :  1; // Under flow detected in the Timeout queues. See TXOTR_MSG_ERR_INFO_TIMEOUT_QUEUE_BE CSR for error information.ERR_CATEGORY_HFI
        uint64_t timeout_queue_overflow  :  1; // Overflow detected in Timeout queues. See TXOTR_MSG_ERR_INFO_TIMEOUT_QUEUE_BE CSR for error information. ERR_CATEGORY_HFI
        uint64_t   nack_oos_queue_mbe  :  1; // Multi bit error in NACK OOS queues. See TXOTR_MSG_ERR_INFO_NACK_OOS_QUEUE_BE CSR for error information. ERR_CATEGORY_HFI
        uint64_t   nack_oos_queue_sbe  :  1; // Single bit error in NACK OOS queues. See TXOTR_MSG_ERR_INFO_NACK_OOS_QUEUE_BE CSR for error information. ERR_CATEGORY_CORRECTABLE
        uint64_t nack_oos_queue_undflow  :  1; // Under flow detected in the NACK OOS queues. See TXOTR_MSG_ERR_INFO_NACK_OOS_QUEUE_BE CSR for error information. ERR_CATEGORY_HFI
        uint64_t nack_oos_queue_overflow  :  1; // Overflow detected in NACK OOS queues. See TXOTR_MSG_ERR_INFO_NACK_OOS_QUEUE_BE CSR for error information. ERR_CATEGORY_HFI
        uint64_t     bpe_prog_mem_mbe  :  1; // Buffer Programmable Engine Program Memory MBE Error Flag. See TXOTR_MSG_ERR_INFO_BPE_PROG_MEM_BE CSR for error information. ERR_CATEGORY_HFI
        uint64_t     bpe_prog_mem_sbe  :  1; // Buffer Programmable Engine Program Memory SBE Error Flag. See TXOTR_MSG_ERR_INFO_BPE_PROG_MEM_BE CSR for error information. ERR_CATEGORY_CORRECTABLE
        uint64_t     bpe_data_mem_mbe  :  1; // Buffer Programmable Engine Data Memory MBE Error Flag. See TXOTR_MSG_ERR_INFO_BPE_DATA_MEM_BE CSR for error information. ERR_CATEGORY_HFI
        uint64_t     bpe_data_mem_sbe  :  1; // Buffer Programmable Engine Data Memory SBE Error Flag. See TXOTR_MSG_ERR_INFO_BPE_DATA_MEM_BE CSR for error information. ERR_CATEGORY_CORRECTABLE
        uint64_t   bpe_omb_mem_rd_mbe  :  1; // Buffer Programmable Engine OMB Read MBE Error Flag.. See TXOTR_MSG_ERR_INFO_BPE_OMB_RD_BE CSR for error information. ERR_CATEGORY_HFI
        uint64_t   bpe_omb_mem_rd_sbe  :  1; // Buffer Programmable Engine OMB read SBE Error Flag. See TXOTR_MSG_ERR_INFO_BPE_OMB_RD_BE CSR for error information. ERR_CATEGORY_CORRECTABLE
        uint64_t   bpe_opb_mem_rd_mbe  :  1; // Buffer Programmable Engine OPB Read MBE Error Flag.. See TXOTR_MSG_ERR_INFO_BPE_OPB_RD_BE CSR for error information. ERR_CATEGORY_HFI
        uint64_t   bpe_opb_mem_rd_sbe  :  1; // Buffer Programmable Engine OPB read SBE Error Flag. See TXOTR_MSG_ERR_INFO_BPE_OPB_RD_BE CSR for error information. ERR_CATEGORY_CORRECTABLE
        uint64_t         bpe_firmware  :  1; // Buffer Programmable Engine Error Flag. See TXOTR_MSG_ERR_INFO_BPE_FIRMWARE CSR for error information. ERR_CATEGORY_HFI
        uint64_t        fw_authen_err  :  1; // Error while authenticating the firmware. ERR_CATEGORY_HFI
        uint64_t ref_count_fifo_undflow  :  1; // Reference count fifo underflow error. ERR_CATEGORY_HFI
        uint64_t ref_count_fifo_overflow  :  1; // Reference count fifo overflow error. ERR_CATEGORY_HFI
        uint64_t local_start_misaligned  :  1; // Local start mis-aligned error. ERR_CATEGORY_TRANSACTION
        uint64_t       Reserved_63_61  :  3; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_STS_t;

// TXOTR_MSG_ERR_CLR desc:
typedef union {
    struct {
        uint64_t              err_clr  : 61; // Error clear bits. See error status register for bit assignments.
        uint64_t       Reserved_63_61  :  3; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_CLR_t;

// TXOTR_MSG_ERR_FRC desc:
typedef union {
    struct {
        uint64_t              err_frc  : 61; // Error force bits. See error status register for bit assignments.
        uint64_t       Reserved_63_61  :  3; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_FRC_t;

// TXOTR_MSG_ERR_EN_HOST desc:
typedef union {
    struct {
        uint64_t          err_host_en  : 61; // Host interrupt mask enable bit. See error status register for bit assignments.
        uint64_t       Reserved_63_61  :  3; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_EN_HOST_t;

// TXOTR_MSG_ERR_FIRST_HOST desc:
typedef union {
    struct {
        uint64_t       err_host_first  : 61; // First host interrupt detected. See error status register for bit assignments.
        uint64_t       Reserved_63_61  :  3; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_FIRST_HOST_t;

// TXOTR_MSG_ERR_EN_BMC desc:
typedef union {
    struct {
        uint64_t           err_bmc_en  : 61; // BMC interrupt mask enable bit. See error status register for bit assignments.
        uint64_t       Reserved_63_61  :  3; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_EN_BMC_t;

// TXOTR_MSG_ERR_FIRST_BMC desc:
typedef union {
    struct {
        uint64_t        err_bmc_first  : 61; // BMC host interrupt detected. See error status register for bit assignments.
        uint64_t       Reserved_63_61  :  3; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_FIRST_BMC_t;

// TXOTR_MSG_ERR_EN_QUAR desc:
typedef union {
    struct {
        uint64_t          err_quar_en  : 61; // Quarantine interrupt mask enable bit. See error status register for bit assignments.
        uint64_t       Reserved_63_61  :  3; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_EN_QUAR_t;

// TXOTR_MSG_ERR_FIRST_QUAR desc:
typedef union {
    struct {
        uint64_t       err_quar_first  : 61; // Quarantine host interrupt detected. See error status register for bit assignments.
        uint64_t       Reserved_63_61  :  3; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_FIRST_QUAR_t;

// TXOTR_MSG_ERR_INFO_BPE_FIRMWARE desc:
typedef union {
    struct {
        uint64_t             encoding  :  8; // Error encoding. TODO - delineate these encodings - Read or write to an invalid address
        uint64_t                 ctxt  :  5; // Context from which the error originated. TODO - equivalent for BPE?
        uint64_t       Reserved_15_13  :  3; // Unused
        uint64_t               msg_id  : 16; // Message Identifier of the command associated with the error
        uint64_t               pkt_id  : 16; // Packet Identifier of the command associated with the error
        uint64_t       Reserved_62_48  : 15; // Unused
        uint64_t         firmware_err  :  1; // Indicates a firmware error
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_BPE_FIRMWARE_t;

// TXOTR_MSG_ERR_INFO_MSGID_MEM_BE desc:
typedef union {
    struct {
        uint64_t              address  :  8; // Address
        uint64_t        Reserved_63_8  : 56; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_MSGID_MEM_BE_t;

// TXOTR_MSG_ERR_INFO_BPE_PROG_MEM_BE desc:
typedef union {
    struct {
        uint64_t          address_mbe  : 12; // Address captured for MBE error
        uint64_t       Reserved_15_12  :  4; // Unused
        uint64_t          address_sbe  : 12; // Address captured for SBE error
        uint64_t       Reserved_31_28  :  4; // Unused
        uint64_t         syndrome_mbe  :  7; // Syndrome captured for MBE error
        uint64_t          Reserved_39  :  1; // Unused
        uint64_t         syndrome_sbe  :  7; // Syndrome captured for SBE error
        uint64_t       Reserved_63_47  : 17; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_BPE_PROG_MEM_BE_t;

// TXOTR_MSG_ERR_INFO_BPE_DATA_MEM_BE desc:
typedef union {
    struct {
        uint64_t          address_mbe  : 10; // Address captured for MBE error
        uint64_t       Reserved_15_10  :  6; // Unused
        uint64_t          address_sbe  : 10; // Address captured for SBE error
        uint64_t       Reserved_31_26  :  6; // Unused
        uint64_t         syndrome_mbe  :  8; // Syndrome captured for MBE error
        uint64_t         syndrome_sbe  :  8; // Syndrome captured for SBE error
        uint64_t       Reserved_63_48  : 16; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_BPE_DATA_MEM_BE_t;

// TXOTR_MSG_ERR_INFO_TXCI_CMD_BE_0 desc:
typedef union {
    struct {
        uint64_t       syndrome_mbe_0  :  8; // Syndrome captured for domain 0 for a detected MBE
        uint64_t       syndrome_mbe_1  :  8; // Syndrome captured for domain 1 for a detected MBE
        uint64_t       syndrome_mbe_2  :  8; // Syndrome captured for domain 2 for a detected MBE
        uint64_t       syndrome_mbe_3  :  8; // Syndrome captured for domain 3 for a detected MBE
        uint64_t       syndrome_sbe_0  :  8; // Syndrome captured for domain 0 for a detected SBE
        uint64_t       syndrome_sbe_1  :  8; // Syndrome captured for domain 1 for a detected SBE
        uint64_t       syndrome_sbe_2  :  8; // Syndrome captured for domain 2 for a detected SBE
        uint64_t       syndrome_sbe_3  :  8; // Syndrome captured for domain 3 for a detected SBE
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_TXCI_CMD_BE_0_t;

// TXOTR_MSG_ERR_INFO_TXCI_CMD_BE_1 desc:
typedef union {
    struct {
        uint64_t                  mbe  :  4; // ECC domain containing the bit error
        uint64_t                  sbe  :  4; // ECC domain containing the bit error
        uint64_t        Reserved_63_8  : 56; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_TXCI_CMD_BE_1_t;

// TXOTR_MSG_ERR_INFO_INVALID_CMD desc:
typedef union {
    struct {
        uint64_t        Reserved_31_0  : 32; // Unused
        uint64_t      tail_abort_mctc  :  4; // Captures the MCTC for the first tail abort Error
        uint64_t  tail_flit_miss_mctc  :  4; // Captures the MCTC for the first tail flit missing Error
        uint64_t  head_flit_miss_mctc  :  4; // Captures the MCTC for the first head flit missing Error
        uint64_t      cmd_length_mctc  :  4; // Captures the MCTC for the first command length Error
        uint64_t        pkt_fail_mctc  :  4; // Captures the MCTC for the first packet fail Error
        uint64_t       Reserved_63_52  : 12; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_INVALID_CMD_t;

// TXOTR_MSG_ERR_INFO_UNKNOWN_CMD desc:
typedef union {
    struct {
        uint64_t                ctype  :  4; // Command Type for the command
        uint64_t                  cmd  :  7; // Cmd value of the command
        uint64_t          Reserved_11  :  1; // Unused
        uint64_t                 mctc  :  4; // MCTC for the command
        uint64_t       Reserved_63_16  : 48; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_UNKNOWN_CMD_t;

// TXOTR_MSG_ERR_INFO_RMESSAGE_CMD desc:
typedef union {
    struct {
        uint64_t                ctype  :  4; // Command Type for the command
        uint64_t                  cmd  :  7; // Cmd value of the command
        uint64_t                 mctc  :  4; // MCTC for the command
        uint64_t           process_id  : 13; // Process ID of the command
        uint64_t     ctype_unexpected  :  4; // Command Type for the command
        uint64_t       cmd_unexpected  :  7; // Cmd value of the command
        uint64_t      mctc_unexpected  :  4; // MCTC for the command
        uint64_t       Reserved_63_43  : 21; // Unused
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

// TXOTR_MSG_ERR_INFO_OMB_FIFO_0 desc:
typedef union {
    struct {
        uint64_t       syndrome_mbe_0  :  8; // Syndrome captured for domain 0 for a detected MBE
        uint64_t       syndrome_mbe_1  :  8; // Syndrome captured for domain 1 for a detected MBE
        uint64_t       syndrome_mbe_2  :  8; // Syndrome captured for domain 2 for a detected MBE
        uint64_t       syndrome_mbe_3  :  8; // Syndrome captured for domain 3 for a detected MBE
        uint64_t       syndrome_sbe_0  :  8; // Syndrome captured for domain 0 for a detected SBE
        uint64_t       syndrome_sbe_1  :  8; // Syndrome captured for domain 1 for a detected SBE
        uint64_t       syndrome_sbe_2  :  8; // Syndrome captured for domain 2 for a detected SBE
        uint64_t       syndrome_sbe_3  :  8; // Syndrome captured for domain 3 for a detected SBE
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_OMB_FIFO_0_t;

// TXOTR_MSG_ERR_INFO_OMB_FIFO_1 desc:
typedef union {
    struct {
        uint64_t                  mbe  :  4; // ECC domain containing the bit error
        uint64_t                  sbe  :  4; // ECC domain containing the bit error
        uint64_t             mctc_mbe  :  4; // TC containing the bit error
        uint64_t             mctc_sbe  :  4; // TC containing the bit error
        uint64_t         mctc_undflow  :  4; // TC containing the underflow error
        uint64_t        mctc_overflow  :  4; // TC containing the overflow error
        uint64_t       Reserved_63_24  : 40; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_OMB_FIFO_1_t;

// TXOTR_MSG_ERR_INFO_RENDZ_FIFO_0 desc:
typedef union {
    struct {
        uint64_t       syndrome_mbe_0  :  8; // Syndrome captured for domain 0 for a detected MBE
        uint64_t       syndrome_mbe_1  :  8; // Syndrome captured for domain 1 for a detected MBE
        uint64_t       syndrome_mbe_2  :  8; // Syndrome captured for domain 2 for a detected MBE
        uint64_t       syndrome_mbe_3  :  8; // Syndrome captured for domain 3 for a detected MBE
        uint64_t       syndrome_sbe_0  :  8; // Syndrome captured for domain 0 for a detected SBE
        uint64_t       syndrome_sbe_1  :  8; // Syndrome captured for domain 1 for a detected SBE
        uint64_t       syndrome_sbe_2  :  8; // Syndrome captured for domain 2 for a detected SBE
        uint64_t       syndrome_sbe_3  :  8; // Syndrome captured for domain 3 for a detected SBE
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_RENDZ_FIFO_0_t;

// TXOTR_MSG_ERR_INFO_RENDZ_FIFO_1 desc:
typedef union {
    struct {
        uint64_t                  mbe  :  4; // ECC domain containing the bit error
        uint64_t                  sbe  :  4; // ECC domain containing the bit error
        uint64_t             mctc_mbe  :  4; // TC containing the bit error
        uint64_t             mctc_sbe  :  4; // TC containing the bit error
        uint64_t           tc_undflow  :  2; // TC containing the underflow error
        uint64_t          tc_overflow  :  2; // TC containing the bit overflow error
        uint64_t       Reserved_63_20  : 44; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_RENDZ_FIFO_1_t;

// TXOTR_MSG_ERR_INFO_RX_RSP_INTF_BE_0 desc:
typedef union {
    struct {
        uint64_t       syndrome_mbe_0  :  8; // Syndrome captured for domain 0 for a detected MBE
        uint64_t       syndrome_mbe_1  :  8; // Syndrome captured for domain 1 for a detected MBE
        uint64_t       syndrome_mbe_2  :  8; // Syndrome captured for domain 2 for a detected MBE
        uint64_t       syndrome_mbe_3  :  8; // Syndrome captured for domain 3 for a detected MBE
        uint64_t       syndrome_sbe_0  :  8; // Syndrome captured for domain 0 for a detected SBE
        uint64_t       syndrome_sbe_1  :  8; // Syndrome captured for domain 1 for a detected SBE
        uint64_t       syndrome_sbe_2  :  8; // Syndrome captured for domain 2 for a detected SBE
        uint64_t       syndrome_sbe_3  :  8; // Syndrome captured for domain 3 for a detected SBE
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_RX_RSP_INTF_BE_0_t;

// TXOTR_MSG_ERR_INFO_RX_RSP_INTF_BE_1 desc:
typedef union {
    struct {
        uint64_t                  mbe  :  4; // ECC domain containing the bit error
        uint64_t                  sbe  :  4; // ECC domain containing the bit error
        uint64_t               tc_mbe  :  2; // TC for the captured MBE error
        uint64_t               tc_sbe  :  2; // TC for the captured SBE error
        uint64_t     syndrome_ent_mbe  :  9; // ECC domain containing the bit error
        uint64_t     syndrome_ent_sbe  :  9; // ECC domain containing the bit error
        uint64_t       Reserved_63_30  : 34; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_RX_RSP_INTF_BE_1_t;

// TXOTR_MSG_ERR_INFO_RX_RSP_OMBCTRL_BE desc:
typedef union {
    struct {
        uint64_t         syndrome_mbe  :  6; // Syndrome for the MBE error.
        uint64_t         syndrome_sbe  :  6; // Syndrome for the SBE error.
        uint64_t               tc_mbe  :  2; // TC for the MBE error.
        uint64_t               tc_sbe  :  2; // TC for the SBE error.
        uint64_t           tc_undflow  :  2; // TC for the underflow error.
        uint64_t          tc_overflow  :  2; // TC for the overflow error.
        uint64_t  tc_ombstate_undflow  :  4; // TC for the underflow error.
        uint64_t tc_ombstate_overflow  :  4; // TC for the overflow error.
        uint64_t       Reserved_63_28  : 36; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_RX_RSP_OMBCTRL_BE_t;

// TXOTR_MSG_ERR_INFO_RX_RSP_RXDMA_CMD_BE desc:
typedef union {
    struct {
        uint64_t           syndrome_0  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_1  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_2  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_3  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_4  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_5  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_6  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_7  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_RX_RSP_RXDMA_CMD_BE_t;

// TXOTR_MSG_ERR_INFO_RX_RSP_RXDMA_CMD desc:
typedef union {
    struct {
        uint64_t                  mbe  :  8; // ECC domain containing the bit error
        uint64_t                  sbe  :  8; // ECC domain containing the bit error
        uint64_t               tc_mbe  :  2; // TC containing the bit error
        uint64_t               tc_sbe  :  2; // TC containing the bit error
        uint64_t           undflow_tc  :  2; // TC containing the underflow error
        uint64_t          overflow_tc  :  2; // TC containing the overflow error
        uint64_t omb_rxet_fifo_undflow_tc  :  2; // TC containing the underflow error
        uint64_t omb_rxet_fifo_overflow_tc  :  2; // TC containing the overflow error
        uint64_t       Reserved_63_28  : 36; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_RX_RSP_RXDMA_CMD_t;

// TXOTR_MSG_ERR_INFO_NR_BIT_SET desc:
typedef union {
    struct {
        uint64_t               msg_id  : 16; // Captured msg_id for this error reported
        uint64_t       Reserved_63_16  : 48; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_NR_BIT_SET_t;

// TXOTR_MSG_ERR_INFO_CTS_QUEUE_BE desc:
typedef union {
    struct {
        uint64_t           syndrome_0  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_1  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_2  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_3  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t                  mbe  :  4; // ECC domain containing the bit error
        uint64_t                  sbe  :  4; // ECC domain containing the bit error
        uint64_t               tc_mbe  :  2; // TC containing the bit error
        uint64_t               tc_sbe  :  2; // TC containing the bit error
        uint64_t           undflow_tc  :  2; // TC containing the underflow error
        uint64_t          overflow_tc  :  2; // TC containing the overflow error
        uint64_t       Reserved_63_48  : 16; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_CTS_QUEUE_BE_t;

// TXOTR_MSG_ERR_INFO_TIMEOUT_QUEUE_BE desc:
typedef union {
    struct {
        uint64_t         syndrome_sbe  :  6; // Syndrome for the SBE error.
        uint64_t         Reserved_7_6  :  2; // Unused
        uint64_t         syndrome_mbe  :  6; // Syndrome for the MBE error.
        uint64_t       Reserved_15_14  :  2; // Unused
        uint64_t            underflow  :  8; // domain for the underflow detected
        uint64_t             overflow  :  8; // domain for the overflow detected
        uint64_t           domain_sbe  :  8; // domain fifo for SBE error
        uint64_t           domain_mbe  :  8; // domain fifo for MBE error
        uint64_t       Reserved_63_48  : 16; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_TIMEOUT_QUEUE_BE_t;

// TXOTR_MSG_ERR_INFO_NACK_OOS_QUEUE_BE desc:
typedef union {
    struct {
        uint64_t         syndrome_sbe  :  6; // Syndrome for the SBE error.
        uint64_t         Reserved_7_6  :  2; // Unused
        uint64_t         syndrome_mbe  :  6; // Syndrome for the MBE error.
        uint64_t       Reserved_15_14  :  2; // Unused
        uint64_t            underflow  :  4; // domain for the underflow detected
        uint64_t             overflow  :  4; // domain for the overflow detected
        uint64_t       Reserved_31_24  :  8; // Unused
        uint64_t           domain_sbe  :  4; // domain fifo for SBE error
        uint64_t           domain_mbe  :  4; // domain fifo for MBE error
        uint64_t       Reserved_63_40  : 24; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_NACK_OOS_QUEUE_BE_t;

// TXOTR_MSG_ERR_INFO_OMB_BE_0 desc:
typedef union {
    struct {
        uint64_t           syndrome_0  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_1  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_2  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_3  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_4  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_5  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_6  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_7  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_OMB_BE_0_t;

// TXOTR_MSG_ERR_INFO_OMB_BE_1 desc:
typedef union {
    struct {
        uint64_t           syndrome_0  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_1  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_2  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_3  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_4  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_5  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_6  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_7  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_OMB_BE_1_t;

// TXOTR_MSG_ERR_INFO_OMB_BE_2 desc:
typedef union {
    struct {
        uint64_t           syndrome_0  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_1  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_2  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_3  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_4  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_5  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_6  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_7  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_OMB_BE_2_t;

// TXOTR_MSG_ERR_INFO_OMB_BE_3 desc:
typedef union {
    struct {
        uint64_t           syndrome_0  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_1  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_2  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_3  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_4  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_5  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_6  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
        uint64_t           syndrome_7  :  8; // Syndrome for the SBE/MBE error. Priority for MBE
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_OMB_BE_3_t;

// TXOTR_MSG_ERR_INFO_OMB_0 desc:
typedef union {
    struct {
        uint64_t               msg_id  : 14; // Message Identifier
        uint64_t       Reserved_15_14  :  2; // Unused
        uint64_t                  mbe  :  8; // ECC domain containing the bit error
        uint64_t                  sbe  :  8; // ECC domain containing the bit error
        uint64_t       Reserved_63_32  : 32; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_OMB_0_t;

// TXOTR_MSG_ERR_INFO_OMB_1 desc:
typedef union {
    struct {
        uint64_t               msg_id  : 14; // Message Identifier
        uint64_t       Reserved_15_14  :  2; // Unused
        uint64_t                  mbe  :  8; // ECC domain containing the bit error
        uint64_t                  sbe  :  8; // ECC domain containing the bit error
        uint64_t       Reserved_63_32  : 32; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_OMB_1_t;

// TXOTR_MSG_ERR_INFO_OMB_2 desc:
typedef union {
    struct {
        uint64_t               msg_id  : 14; // Message Identifier
        uint64_t       Reserved_15_14  :  2; // Unused
        uint64_t                  mbe  :  8; // ECC domain containing the bit error
        uint64_t                  sbe  :  8; // ECC domain containing the bit error
        uint64_t       Reserved_63_32  : 32; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_OMB_2_t;

// TXOTR_MSG_ERR_INFO_OMB_3 desc:
typedef union {
    struct {
        uint64_t               msg_id  : 14; // Message Identifier
        uint64_t       Reserved_15_14  :  2; // Unused
        uint64_t                  mbe  :  8; // ECC domain containing the bit error
        uint64_t                  sbe  :  8; // ECC domain containing the bit error
        uint64_t       Reserved_63_32  : 32; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_OMB_3_t;

// TXOTR_MSG_ERR_INFO_BPE_OMB_RD_BE desc:
typedef union {
    struct {
        uint64_t          address_sbe  : 14; // OMB address containing the bit error
        uint64_t          address_mbe  : 14; // OMB address containing the bit error
        uint64_t         syndrome_mbe  :  8; // MBE Syndrome. Highest priority given to the MSB domain
        uint64_t                  sbe  :  8; // ECC domain containing the bit error
        uint64_t                  mbe  :  8; // ECC domain containing the bit error
        uint64_t       Reserved_63_52  : 12; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_BPE_OMB_RD_BE_t;

// TXOTR_MSG_ERR_INFO_BPE_OPB_RD_BE desc:
typedef union {
    struct {
        uint64_t          address_mbe  : 14; // OMB address containing the bit error See description in syndrome_mbe for details on multiple bits.
        uint64_t          address_sbe  : 14; // OMB address containing the bit error See description in syndrome_mbe for details on multiple bits.
        uint64_t         syndrome_mbe  :  8; // MBE Syndrome captured the bit error. High domain If multiple bits are set within the mbe field, this field will reflect the syndrome associated with the lowest domain.
        uint64_t                  mbe  :  5; // ECC domain containing the bit error
        uint64_t                  sbe  :  5; // ECC domain containing the bit error
        uint64_t       Reserved_63_46  : 18; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INFO_BPE_OPB_RD_BE_t;

// TXOTR_MSG_ERR_INJECT_0 desc:
typedef union {
    struct {
        uint64_t omb_fifo_inject_mask  : 32; // Error injection Mask
        uint64_t   omb_fifo_inject_en  :  4; // Error inject enable.
        uint64_t       Reserved_63_36  : 28; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INJECT_0_t;

// TXOTR_MSG_ERR_INJECT_1 desc:
typedef union {
    struct {
        uint64_t      cmd_inject_mask  : 32; // Error injection Mask
        uint64_t        cmd_inject_en  :  4; // Error inject enable.
        uint64_t       Reserved_63_36  : 28; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INJECT_1_t;

// TXOTR_MSG_ERR_INJECT_2 desc:
typedef union {
    struct {
        uint64_t      omb_inject_mask  :  8; // Error injection Mask
        uint64_t        omb_inject_en  : 32; // Error inject enable.
        uint64_t       Reserved_63_40  : 24; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INJECT_2_t;

// TXOTR_MSG_ERR_INJECT_3 desc:
typedef union {
    struct {
        uint64_t rx_rsp_intf_inject_mask  :  8; // Error injection Mask
        uint64_t rx_rsp_intf_inject_en  :  4; // Error inject enable.
        uint64_t rx_rsp_ent_intf_inject_mask  :  9; // Error injection Mask
        uint64_t rx_rsp_ent_intf_inject_en  :  1; // Error inject enable.
        uint64_t rx_rsp_ombctrl_inject_mask  :  6; // Error injection Mask
        uint64_t rx_rsp_ombctrl_inject_en  :  1; // Error inject enable.
        uint64_t rx_rsp_rxdma_cmd_fifo_inject_en  :  8; // Error inject enable.
        uint64_t       Reserved_63_37  : 27; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INJECT_3_t;

// TXOTR_MSG_ERR_INJECT_4 desc:
typedef union {
    struct {
        uint64_t rx_rsp_rxdma_cmd_fifo_inject_mask  : 64; // Error injection Mask
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INJECT_4_t;

// TXOTR_MSG_ERR_INJECT_5 desc:
typedef union {
    struct {
        uint64_t         Reserved_7_0  :  8; // Unused
        uint64_t bpe_data_inject_mask  :  8; // Error injection Mask
        uint64_t   bpe_data_inject_en  :  1; // Error inject enable.
        uint64_t bpe_code_inject_mask  :  7; // Error injection Mask
        uint64_t   bpe_code_inject_en  :  1; // Error inject enable.
        uint64_t       Reserved_63_25  : 39; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INJECT_5_t;

// TXOTR_MSG_ERR_INJECT_6 desc:
typedef union {
    struct {
        uint64_t cts_queue_inject_mask  : 32; // Error injection Mask
        uint64_t  cts_queue_inject_en  :  4; // Error inject enable.
        uint64_t       Reserved_63_36  : 28; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INJECT_6_t;

// TXOTR_MSG_ERR_INJECT_7 desc:
typedef union {
    struct {
        uint64_t timeout_queue_inject_mask  :  6; // Error injection Mask
        uint64_t timeout_queue_inject_en  :  1; // Error inject enable.
        uint64_t nack_oss_queue_inject_mask  :  6; // Error injection Mask
        uint64_t nack_oos_queue_inject_en  :  1; // Error inject enable.
        uint64_t     omb_rd_inject_en  :  8; // Error inject enable.
        uint64_t       Reserved_63_22  : 42; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INJECT_7_t;

// TXOTR_MSG_ERR_INJECT_8 desc:
typedef union {
    struct {
        uint64_t   omb_rd_inject_mask  : 64; // Error inject Mask
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INJECT_8_t;

// TXOTR_MSG_ERR_INJECT_9 desc:
typedef union {
    struct {
        uint64_t   opb_rd_inject_mask  : 40; // Error inject Mask
        uint64_t opb_rd_inject_enable  :  5; // Error inject enable
        uint64_t       Reserved_63_45  : 19; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_INJECT_9_t;

// TXOTR_MSG_ERR_MBE_SAT_COUNT desc:
typedef union {
    struct {
        uint64_t        mbe_sat_count  : 16; // MBE saturation count
        uint64_t       Reserved_63_16  : 48; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_ERR_MBE_SAT_COUNT_t;

// TXOTR_MSG_DBG_OMB_ACCESS desc:
typedef union {
    struct {
        uint64_t              address  : 14; // Address for Read or Write. Valid address are from 0 to 12287. For FXR, bits 15:14 are ignored. No read or write is performed for an address value between 12288 and 16383.
        uint64_t       Reserved_51_14  : 38; // Unused
        uint64_t         Payload_regs  :  8; // Constant indicating number of payload registers follow
        uint64_t          Reserved_60  :  1; // Unused
        uint64_t                  ECC  :  1; // 0 = Read Raw Data 1 = Correct Data on Read
        uint64_t          Reserved_62  :  1; // Unused
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

// TXOTR_MSG_DBG_MSGID_ACCESS desc:
typedef union {
    struct {
        uint64_t              address  :  8; // Address for Read. Valid address are from 0 to 191.
        uint64_t        Reserved_51_8  : 44; // Unused
        uint64_t         Payload_regs  :  8; // Constant indicating number of payload registers follow
        uint64_t       Reserved_62_60  :  3; // Unused
        uint64_t                Valid  :  1; // Set by software, cleared by hardware when complete.
    } field;
    uint64_t val;
} TXOTR_MSG_DBG_MSGID_ACCESS_t;

// TXOTR_MSG_DBG_MSGID_PAYLOAD0 desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // Data[63:0]
    } field;
    uint64_t val;
} TXOTR_MSG_DBG_MSGID_PAYLOAD0_t;

// TXOTR_MSG_DBG_BPE_PROG_MEM_ACCESS desc:
typedef union {
    struct {
        uint64_t              address  : 12; // Address for Read or Write. Valid address are from 0 to 4095.
        uint64_t       Reserved_51_12  : 40; // Unused
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
        uint64_t       Reserved_63_32  : 32; // Unused
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
        uint64_t       Reserved_63_16  : 48; // Unused
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

// TXOTR_MSG_DBG_FIRMWARE_BPE_ADDR desc:
typedef union {
    struct {
        uint64_t        trace_address  : 64; // The destination address for firmware trace messages
    } field;
    uint64_t val;
} TXOTR_MSG_DBG_FIRMWARE_BPE_ADDR_t;

// TXOTR_MSG_DBG_FIRMWARE_FPE_ADDR desc:
typedef union {
    struct {
        uint64_t        trace_address  : 64; // The destination address for firmware trace messages
    } field;
    uint64_t val;
} TXOTR_MSG_DBG_FIRMWARE_FPE_ADDR_t;

// TXOTR_MSG_PRF_STALL_OMB_ENTRIES_X desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  4; // 4'd15-4'd11 - Reserved 4'd10 - Any MC 4'd9 - Any MC1 4'd8 - Any MC0 4'd7 - MC1TC3 4'd6 - MC1TC2 4'd5 - MC1TC1 4'd4 - MC1TC0 4'd3 - MC0TC3 4'd2 - MC0TC2 4'd1 - MC0TC1 4'd0 - MC0TC0
        uint64_t        Reserved_63_4  : 60; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_PRF_STALL_OMB_ENTRIES_X_t;

// TXOTR_MSG_PRF_STALL_OMB_ENTRIES_Y desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  4; // 4'd15-4'd11 - Reserved 4'd10 - Any MC 4'd9 - Any MC1 4'd8 - Any MC0 4'd7 - MC1TC3 4'd6 - MC1TC2 4'd5 - MC1TC1 4'd4 - MC1TC0 4'd3 - MC0TC3 4'd2 - MC0TC2 4'd1 - MC0TC1 4'd0 - MC0TC0
        uint64_t        Reserved_63_4  : 60; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_PRF_STALL_OMB_ENTRIES_Y_t;

// TXOTR_MSG_PRF_STALL_RXDMA_CREDITS_X desc:
typedef union {
    struct {
        uint64_t                   TC  :  3; // 3'd7-3'd5 - Reserved 3'd4 - Any TC 3'd3 - TC3 3'd2 - TC2 3'd1 - TC1 3'd0 - TC0
        uint64_t        Reserved_63_3  : 61; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_PRF_STALL_RXDMA_CREDITS_X_t;

// TXOTR_MSG_PRF_STALL_RXDMA_CREDITS_Y desc:
typedef union {
    struct {
        uint64_t                   TC  :  3; // 3'd7-3'd5 - Reserved 3'd4 - Any TC 3'd3 - TC3 3'd2 - TC2 3'd1 - TC1 3'd0 - TC0
        uint64_t        Reserved_63_3  : 61; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_PRF_STALL_RXDMA_CREDITS_Y_t;

// TXOTR_MSG_PRF_STALL_M_TO_P_CREDITS_X desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  4; // 4'd15 - Any MC 4'd14 - Any MC1' 4'd13 - Any MC1 4'd12 - Any MC0 4'd11 - MC1'TC3 4'd10 - MC1'TC2 4'd9 - MC1'TC1 4'd8 - MC1'TC0 4'd7 - MC1TC3 4'd6 - MC1TC2 4'd5 - MC1TC1 4'd4 - MC1TC0 4'd3 - MC0TC3 4'd2 - MC0TC2 4'd1 - MC0TC1 4'd0 - MC0TC0
        uint64_t        Reserved_63_4  : 60; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_PRF_STALL_M_TO_P_CREDITS_X_t;

// TXOTR_MSG_PRF_STALL_M_TO_P_CREDITS_Y desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  4; // 4'd15 - Any MC 4'd14 - Any MC1' 4'd13 - Any MC1 4'd12 - Any MC0 4'd11 - MC1'TC3 4'd10 - MC1'TC2 4'd9 - MC1'TC1 4'd8 - MC1'TC0 4'd7 - MC1TC3 4'd6 - MC1TC2 4'd5 - MC1TC1 4'd4 - MC1TC0 4'd3 - MC0TC3 4'd2 - MC0TC2 4'd1 - MC0TC1 4'd0 - MC0TC0
        uint64_t        Reserved_63_4  : 60; // Reserved
    } field;
    uint64_t val;
} TXOTR_MSG_PRF_STALL_M_TO_P_CREDITS_Y_t;

// TXOTR_MSG_PRF_STALL_PREFRAG_CREDITS_X desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  4; // 4'd15-4'd11 - Reserved 4'd10 - Any MC 4'd9 - Any MC1 4'd8 - Any MC0 4'd7 - MC1TC3 4'd6 - MC1TC2 4'd5 - MC1TC1 4'd4 - MC1TC0 4'd3 - MC0TC3 4'd2 - MC0TC2 4'd1 - MC0TC1 4'd0 - MC0TC0
        uint64_t        Reserved_63_4  : 60; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_PRF_STALL_PREFRAG_CREDITS_X_t;

// TXOTR_MSG_PRF_STALL_PREFRAG_CREDITS_Y desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  4; // 4'd15-4'd11 - Reserved 4'd10 - Any MC 4'd9 - Any MC1 4'd8 - Any MC0 4'd7 - MC1TC3 4'd6 - MC1TC2 4'd5 - MC1TC1 4'd4 - MC1TC0 4'd3 - MC0TC3 4'd2 - MC0TC2 4'd1 - MC0TC1 4'd0 - MC0TC0
        uint64_t        Reserved_63_4  : 60; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_PRF_STALL_PREFRAG_CREDITS_Y_t;

// TXOTR_MSG_PRF_MSGS_OPENED_X desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  4; // 4'd15-4'd11 - Reserved 4'd10 - Any MC 4'd9 - Any MC1 4'd8 - Any MC0 4'd7 - MC1TC3 4'd6 - MC1TC2 4'd5 - MC1TC1 4'd4 - MC1TC0 4'd3 - MC0TC3 4'd2 - MC0TC2 4'd1 - MC0TC1 4'd0 - MC0TC0
        uint64_t        Reserved_63_4  : 60; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_PRF_MSGS_OPENED_X_t;

// TXOTR_MSG_PRF_MSGS_CLOSED_X desc:
typedef union {
    struct {
        uint64_t                 MCTC  :  4; // 4'd15-4'd11 - Reserved 4'd10 - Any MC 4'd9 - Any MC1 4'd8 - Any MC0 4'd7 - MC1TC3 4'd6 - MC1TC2 4'd5 - MC1TC1 4'd4 - MC1TC0 4'd3 - MC0TC3 4'd2 - MC0TC2 4'd1 - MC0TC1 4'd0 - MC0TC0
        uint64_t        Reserved_63_4  : 60; // Unused
    } field;
    uint64_t val;
} TXOTR_MSG_PRF_MSGS_CLOSED_X_t;

#endif /* ___FXR_tx_otr_msg_top_CSRS_H__ */