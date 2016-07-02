// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Wed Jun 22 19:30:14 2016
//

#ifndef ___FXR_rx_hp_CSRS_H__
#define ___FXR_rx_hp_CSRS_H__

// RXHP_CFG_CTL desc:
typedef union {
    struct {
        uint64_t DISABLE_VTPID_NONMATCHING  :  1; // Do not use the VTPID CAM for packets with NI = PTL_NONMATCHING_LOGICAL
        uint64_t DISABLE_VTPID_MATCHING  :  1; // Do not use the VTPID CAM for packets with NI = PTL_MATCHING_LOGICAL
        uint64_t          DMA_STALL_X  :  3; // This controls what is counted by the DMA_STALL_X performance counter. 0,1,2,3 are single TC, 4 is commands, 7 is all of them.
        uint64_t          DMA_STALL_Y  :  3; // This controls what is counted by the DMA_STALL_Y performance counter. 0,1,2,3 are single TC, 4 is commands, 7 is all of them.
        uint64_t            PORT_MODE  :  2; // Port Operational Mode. See Section XREF of the Portals Transport Layer Specification for a description of each mode of operation. 2'b00 - Disjoint Mode. 2'b01 - Grouped Mode 2'b10 - Reserved 2'b11 - Reserved
        uint64_t          Reserved_10  :  1; // 
        uint64_t       PORT_MIRROR_EN  :  1; // Enable Port Mirroring Mode
        uint64_t      PORT_MIRROR_PID  : 12; // Use this PID for Port Mirroring writes
        uint64_t PORT_MIRROR_BUFFER_READY  :  4; // Set this to indicate that a buffer is available for port mirroring. Hardware will clear it when it is done writing to that buffer.
        uint64_t       Reserved_63_28  : 36; // 
    } field;
    uint64_t val;
} RXHP_CFG_CTL_t;

// RXHP_CFG_VTPID_CAM desc:
typedef union {
    struct {
        uint64_t                  uid  : 32; // UID (CAM tag)
        uint64_t                valid  :  1; // The CAM entry is valid. If this is clear, the CAM entry must never match.
        uint64_t            tpid_base  : 12; // TPID base (CAM data)
        uint64_t       Reserved_63_45  : 19; // 
    } field;
    uint64_t val;
} RXHP_CFG_VTPID_CAM_t;

// RXHP_CFG_HDR_PE desc:
typedef union {
    struct {
        uint64_t            PE_ENABLE  :  8; // Set bit to enable a PE for scheduling. A PE will start processing packets when enabled and authenticated.
        uint64_t        SCHEDULE_MODE  :  1; // 0: Schedule packets to even/odd cluster based on PID[0]. 1: Schedule packets to even/odd clusters based on PID parity.
        uint64_t USE_CONSERVATIVE_ENTRYPOINT  :  1; // Use non-optimized entry points where available.
        uint64_t       Reserved_15_10  :  6; // Unused
        uint64_t START_AUTHENTICATION  :  8; // Starts the authentication process when set. Should be reset by the user before writing to the PE rams. If it is not reset before writing to the rams, the authentication sequence will restart after every write.
        uint64_t AUTHENTICATION_COMPLETE  :  8; // The authentication process has completed. Cleared by hardware when START_AUTHENTICATION is set or the hdr_pe rams are written.
        uint64_t AUTHENTICATION_SUCCESS  :  8; // Authentication was successful, written by hardware when authentication is completed, cleared by hardware when hdr_pe rams are written or START_AUTHENTICATION is set.
        uint64_t    PE_RAM_WRITE_MASK  :  8; // when a bit is cleared, writes to the rams for the corresponding PEs will be dropped. This allows the user to write different firmware images to each PE.
        uint64_t       Reserved_63_48  : 16; // Unused
    } field;
    uint64_t val;
} RXHP_CFG_HDR_PE_t;

// RXHP_CFG_E2E_CREDIT_FUND_MAX desc:
typedef union {
    struct {
        uint64_t TC0_HDR_FLIT_CRDT_FUND_MAX  :  6; // RXE2E TC0 Header flit Credit Fund Max
        uint64_t TC1_HDR_FLIT_CRDT_FUND_MAX  :  6; // RXE2E TC1 Header flit Credit Fund Max
        uint64_t TC2_HDR_FLIT_CRDT_FUND_MAX  :  6; // RXE2E TC2 Header flit Credit Fund Max
        uint64_t TC3_HDR_FLIT_CRDT_FUND_MAX  :  6; // RXE2E TC3 Header flit Credit Fund Max
        uint64_t TC0_PKT_STATUS_CRDT_FUND_MAX  :  6; // RXE2E TC0 Packet Status Credit Fund Max
        uint64_t TC1_PKT_STATUS_CRDT_FUND_MAX  :  6; // RXE2E TC1 Packet Status Credit Fund Max
        uint64_t TC2_PKT_STATUS_CRDT_FUND_MAX  :  6; // RXE2E TC2 Packet Status Credit Fund Max
        uint64_t TC3_PKT_STATUS_CRDT_FUND_MAX  :  6; // RXE2E TC3 Packet Status Credit Fund Max
        uint64_t       Reserved_63_48  : 16; // Unused
    } field;
    uint64_t val;
} RXHP_CFG_E2E_CREDIT_FUND_MAX_t;

// RXHP_CFG_CREDIT_DISABLE desc:
typedef union {
    struct {
        uint64_t rxdma_credit_disable  :  4; // Per TC enable for RxDMA credits
        uint64_t hiarb_credit_disable  :  1; // Disable hiarb request credits.
        uint64_t        Reserved_63_5  : 59; // Unused
    } field;
    uint64_t val;
} RXHP_CFG_CREDIT_DISABLE_t;

// RXHP_CFG_PTE_CACHE_CLIENT_DISABLE desc:
typedef union {
    struct {
        uint64_t client_pte_cache_request_disable  :  1; // Client pte cache request disable.
        uint64_t        Reserved_63_1  : 63; // Unused
    } field;
    uint64_t val;
} RXHP_CFG_PTE_CACHE_CLIENT_DISABLE_t;

// RXHP_CFG_PTE_CACHE_ACCESS_CTL desc:
typedef union {
    struct {
        uint64_t              address  : 22; // The contents of this field differ based on cmd. pte_cache_addr_t for CACHE_CMD_RD, CACHE_CMD_WR,CACHE_CMD_INVALIDATE,CACHE_CMD_FLUSH_INVALID,CACHE_CMD_FLUSH_VALID. 11:0 used for CACHE_CMD_DATA_RD,CACHE_CMD_DATA_WR 9:0 used for CACHE_CMD_TAG_RD,CACHE_CMD_TAG_WR
        uint64_t         mask_address  : 22; // cache cmd mask address. 1 bits are don't care. only used for CACHE_CMD_INVALIDATE, CACHE_CMD_FLUSH_INVALID, CACHE_CMD_FLUSH_VALID The form of this field is pte_cache_addr_t. initially all 1's. Note: If this field is 0, a normal single lookup is done. If non-zero, the entire cache tag memory is read and tag entries match/masked.
        uint64_t                  cmd  :  4; // cache cmd. see <blue text>Section A-7, 'Cache Command enumeration (Enum - fxr_cache_cmd_t) - 4bit' initially CACHE_CMD_INVALIDATE
        uint64_t                 busy  :  1; // SW sets busy when writing this csr. HW clears busy when cmd is complete. busy must be clear before writing this csr. If busy is set, HW is busy on a previous cmd. Coming out of reset, busy will be 1 so as to initiate the pte cache tag invalidation. Then in 2k clks or so, busy will go to 0.
        uint64_t                  Rsv  : 15; // 
    } field;
    uint64_t val;
} RXHP_CFG_PTE_CACHE_ACCESS_CTL_t;

// RXHP_CFG_PTE_CACHE_ACCESS_DATA desc:
typedef union {
    struct {
        uint64_t                 data  : 64; // pte cache data[191:0]
    } field;
    uint64_t val;
} RXHP_CFG_PTE_CACHE_ACCESS_DATA_t;

// RXHP_CFG_PTE_CACHE_ACCESS_DATA3 desc:
typedef union {
    struct {
        uint64_t                 data  : 11; // pte cache data[202:192]
        uint64_t                  Rsv  : 53; // 
    } field;
    uint64_t val;
} RXHP_CFG_PTE_CACHE_ACCESS_DATA3_t;

// RXHP_CFG_PTE_CACHE_ACCESS_DATA_BIT_ENABLE desc:
typedef union {
    struct {
        uint64_t           bit_enable  : 64; // pte cache wr data bit enable[191:0]. 1 bits are written.
    } field;
    uint64_t val;
} RXHP_CFG_PTE_CACHE_ACCESS_DATA_BIT_ENABLE_t;

// RXHP_CFG_PTE_CACHE_ACCESS_DATA_BIT_ENABLE3 desc:
typedef union {
    struct {
        uint64_t           bit_enable  : 11; // pte cache wr data bit enable[202:192]. 1 bits are written.
        uint64_t                  Rsv  : 53; // 
    } field;
    uint64_t val;
} RXHP_CFG_PTE_CACHE_ACCESS_DATA_BIT_ENABLE3_t;

// RXHP_CFG_PTE_CACHE_ACCESS_TAG desc:
typedef union {
    struct {
        uint64_t      tag_way_addr_lo  : 22; // pte cache tag way address, format is pte_cache_addr_t
        uint64_t     tag_way_valid_lo  :  1; // pte cache tag way valid
        uint64_t           reserve_lo  :  9; // 
        uint64_t      tag_way_addr_hi  : 22; // pte cache tag way address, format is pte_cache_addr_t
        uint64_t     tag_way_valid_hi  :  1; // pte cache tag way valid
        uint64_t           reserve_hi  :  9; // 
    } field;
    uint64_t val;
} RXHP_CFG_PTE_CACHE_ACCESS_TAG_t;

// RXHP_CFG_MAX_PSN_DISTANCE desc:
typedef union {
    struct {
        uint64_t   small_max_psn_dist  : 16; // Max PSN distance from expected unordered PSN that OTR is allowed to transmit before going into flow control. This is sent in the special ack when the connection is not currently using a big scoreboard and a big scoreboard is not available. RXHP uses this value in the special ack if rxe2e_mc0_packet_status.psn_non_e2e_ctl_connect_union.non_e2e_ctl_connect.big_scoreboard_available == 0.
        uint64_t     big_max_psn_dist  : 16; // Max PSN distance from expected unordered PSN that OTR is allowed to transmit before going into flow control. This is sent in the special ack when the connection is either currently using a big scoreboard or a big scoreboard is available. RXHP uses this value in the special ack if rxe2e_mc0_packet_status.psn_non_e2e_ctl_connect_union.non_e2e_ctl_connect.big_scoreboard_available == 1.
        uint64_t       Reserved_63_32  : 32; // Reserved
    } field;
    uint64_t val;
} RXHP_CFG_MAX_PSN_DISTANCE_t;

// RXHP_CFG_NPTL_CTL desc:
typedef union {
    struct {
        uint64_t           Reserved_0  :  1; // Reserved
        uint64_t       RcvQPMapEnable  :  1; // When set, the QP mapping table is used for non-KDETH 9B packets. Otherwise non-KDETH 9B packets are delivered to context 0.
        uint64_t         Reserved_3_2  :  2; // Reserved
        uint64_t      RcvBypassEnable  :  1; // When set receive of bypass packets is enabled, otherwise receive of bypass packets is disabled
        uint64_t         RcvRsmEnable  :  1; // When set the entire RSM mechanism is enabled, otherwise the RSM mechanism is disabled
        uint64_t        Reserved_63_6  : 58; // Reserved
    } field;
    uint64_t val;
} RXHP_CFG_NPTL_CTL_t;

// RXHP_CFG_NPTL_MAX_CONTEXTS desc:
typedef union {
    struct {
        uint64_t                  Cnt  :  8; // Total number of receive contexts. This is a constant value for the FXR ASIC regardless of software configuration.
        uint64_t        Reserved_63_8  : 56; // Reserved
    } field;
    uint64_t val;
} RXHP_CFG_NPTL_MAX_CONTEXTS_t;

// RXHP_CFG_NPTL_BTH_QP desc:
typedef union {
    struct {
        uint64_t        Reserved_15_0  : 16; // Reserved
        uint64_t             KDETH_QP  :  8; // The value of QP[23:16] for KDETH packets. Note: a value of 0x00 must not be used since this will cause QP0/QP1 packets to be decoded as KDETH packets. Similarly, a value of 0xFF must not be used since this would cause multicast packets to be decoded as KDETH packets.
        uint64_t       Reserved_63_24  : 40; // Reserved
    } field;
    uint64_t val;
} RXHP_CFG_NPTL_BTH_QP_t;

// RXHP_CFG_NPTL_MULTICAST desc:
typedef union {
    struct {
        uint64_t     MulticastContext  :  8; // Receive context for multicast packets
        uint64_t        Reserved_63_8  : 56; // Reserved
    } field;
    uint64_t val;
} RXHP_CFG_NPTL_MULTICAST_t;

// RXHP_CFG_NPTL_BYPASS desc:
typedef union {
    struct {
        uint64_t        BypassContext  :  8; // Receive context for bypass packets
        uint64_t        Reserved_15_8  :  8; // Reserved
        uint64_t              HdrSize  :  5; // Header size for bypass packets in DWs Minimum value is 0 DWs Maximum value is 31 DWs Note that all values are supported A setting of 0 means that the entirety of the bypass packet is delivered to the eager buffer, and zero bytes to the header queue (i.e. just the RHF value).
        uint64_t       Reserved_63_21  : 43; // Reserved
    } field;
    uint64_t val;
} RXHP_CFG_NPTL_BYPASS_t;

// RXHP_CFG_NPTL_VL15 desc:
typedef union {
    struct {
        uint64_t          VL15Context  :  8; // Receive context for VL15 packets
        uint64_t        Reserved_63_8  : 56; // Reserved
    } field;
    uint64_t val;
} RXHP_CFG_NPTL_VL15_t;

// RXHP_CFG_PSC_CACHE_ACCESS_CTL desc:
typedef union {
    struct {
        uint64_t              address  : 29; // The contents of this field differ based on cmd. psc_cache_addr_t {me,tpid,handle} for CACHE_CMD_RD, CACHE_CMD_WR,CACHE_CMD_INVALIDATE,CACHE_CMD_FLUSH_INVALID,CACHE_CMD_FLUSH_VALID. 13:0 used for CACHE_CMD_DATA_RD, CACHE_CMD_DATA_WR
        uint64_t         mask_address  : 29; // cache cmd mask address. 1 bits are don't care. only used for CACHE_CMD_INVALIDATE, CACHE_CMD_FLUSH_INVALID, CACHE_CMD_FLUSH_VALID The form of this field is psc_cache_addr_t. initially all 1's. Note: If this field is 0, a normal single lookup is done. If non-zero, each PSC entry is read and valid addr is match/masked.
        uint64_t                  cmd  :  4; // cache cmd. see <blue text>Section A-7, 'Cache Command enumeration (Enum - fxr_cache_cmd_t) - 4bit' initially CACHE_CMD_INVALIDATE
        uint64_t                 busy  :  1; // SW sets this bit when writing this csr. HW clears this bit when cmd is complete. This bit must be clear before writing this csr. If this bit is set, HW is busy on a previous cmd. Coming out of reset, this will be 1 so as to initiate the psc cache tag invalidation. When invalidation is finished, it will go to 0.
        uint64_t                 Bank  :  1; // determines which of the two clusters to do the operation on.
    } field;
    uint64_t val;
} RXHP_CFG_PSC_CACHE_ACCESS_CTL_t;

// RXHP_CFG_PSC_CACHE_ACCESS_DATA_SIDE desc:
typedef union {
    struct {
        uint64_t                   me  :  1; // addr.me 1 = me, 0 = unexpectd
        uint64_t                 tpid  : 12; // addr.tpid
        uint64_t               handle  : 16; // addr.handle
        uint64_t                valid  :  1; // cache entry is valid
        uint64_t                  Rsv  :  2; // 
        uint64_t                 prev  : 16; // prev ptr
        uint64_t                 next  : 16; // next ptr
    } field;
    uint64_t val;
} RXHP_CFG_PSC_CACHE_ACCESS_DATA_SIDE_t;

// RXHP_CFG_PSC_CACHE_ACCESS_DATA_ME desc:
typedef union {
    struct {
        uint64_t                 data  : 64; // me[511:0]
    } field;
    uint64_t val;
} RXHP_CFG_PSC_CACHE_ACCESS_DATA_ME_t;

// RXHP_CFG_PSC_CACHE_ACCESS_DATA_BIT_ENABLE_SIDE desc:
typedef union {
    struct {
        uint64_t                   me  :  1; // bit wr enable for addr.me
        uint64_t                 tpid  : 12; // bit wr enable for addr.tpid
        uint64_t               handle  : 16; // bit wr enable for addr.handle
        uint64_t                valid  :  1; // bit wr enable for valid
        uint64_t                  Rsv  :  2; // 
        uint64_t                 prev  : 16; // bit wr enable for prev ptr
        uint64_t                 next  : 16; // bit wr enable for next ptr
    } field;
    uint64_t val;
} RXHP_CFG_PSC_CACHE_ACCESS_DATA_BIT_ENABLE_SIDE_t;

// RXHP_CFG_PSC_CACHE_ACCESS_DATA_BIT_ENABLE_ME desc:
typedef union {
    struct {
        uint64_t           bit_enable  : 64; // psc cache wr data bit enable for me [511:0]. 1 bits are written.
    } field;
    uint64_t val;
} RXHP_CFG_PSC_CACHE_ACCESS_DATA_BIT_ENABLE_ME_t;

// RXHP_CFG_NPTL_QP_MAP_TABLE desc:
typedef union {
    struct {
        uint64_t          RcvContextA  :  8; // Receive context used when QP[8:1] is equal to (i * 8) + 0 where i is the index of this CSR in the RcvQPMapTable array
        uint64_t          RcvContextB  :  8; // Receive context used when QP[8:1] is equal to (i * 8) + 1 where i is the index of this CSR in the RcvQPMapTable array
        uint64_t          RcvContextC  :  8; // Receive context used when QP[8:1] is equal to (i * 8) + 2 where i is the index of this CSR in the RcvQPMapTable array
        uint64_t          RcvContextD  :  8; // Receive context used when QP[8:1] is equal to (i * 8) + 3 where i is the index of this CSR in the RcvQPMapTable array
        uint64_t          RcvContextE  :  8; // Receive context used when QP[8:1] is equal to (i * 8) + 4 where i is the index of this CSR in the RcvQPMapTable array
        uint64_t          RcvContextF  :  8; // Receive context used when QP[8:1] is equal to (i * 8) + 5 where i is the index of this CSR in the RcvQPMapTable array
        uint64_t          RcvContextG  :  8; // Receive context used when QP[8:1] is equal to (i * 8) + 6 where i is the index of this CSR in the RcvQPMapTable array
        uint64_t          RcvContextH  :  8; // Receive context used when QP[8:1] is equal to (i * 8) + 7 where i is the index of this CSR in the RcvQPMapTable array
    } field;
    uint64_t val;
} RXHP_CFG_NPTL_QP_MAP_TABLE_t;

// RXHP_CFG_NPTL_RSM_CFG desc:
typedef union {
    struct {
        uint64_t    EnableOrChainRsm0  :  1; // If this CSR is for RSM instance 0, this is the enable bit for RSM instance 0. Otherwise, this is a chain bit and indicates whether RSM0 is chained into this RSM instance.
        uint64_t    EnableOrChainRsm1  :  1; // If this CSR is for RSM instance 1, this is the enable bit for RSM instance 1. Otherwise, this is a chain bit and indicates whether RSM1 is chained into this RSM instance.
        uint64_t    EnableOrChainRsm2  :  1; // If this CSR is for RSM instance 2, this is the enable bit for RSM instance 2. Otherwise, this is a chain bit and indicates whether RSM2 is chained into this RSM instance.
        uint64_t    EnableOrChainRsm3  :  1; // If this CSR is for RSM instance 3, this is the enable bit for RSM instance 3. Otherwise, this is a chain bit and indicates whether RSM3 is chained into this RSM instance.
        uint64_t        Reserved_31_4  : 28; // Reserved
        uint64_t               Offset  :  8; // The offset for the calculated 8-bit RSM index into the RcvRsmMapTable.
        uint64_t       Reserved_47_40  :  8; // Reserved
        uint64_t        BypassHdrSize  :  5; // Header size in DWs for bypass packets that match the RSM condition Minimum value is 0 DWs Maximum value is 31 DWs Note that all values are supported
        uint64_t       Reserved_59_53  :  7; // Reserved
        uint64_t           PacketType  :  3; // Packet type for this match: - 0x0 = expected receive packet - 0x1 = eager receive packet - 0x2 = 9B packet - 0x3 = reserved (RSM of error packets is not supported) - 0x4 = bypass packet - 0x5 to 0x7 = reserved
        uint64_t          Reserved_63  :  1; // Reserved
    } field;
    uint64_t val;
} RXHP_CFG_NPTL_RSM_CFG_t;

// RXHP_CFG_NPTL_RSM_SELECT desc:
typedef union {
    struct {
        uint64_t         Field1Offset  :  9; // Selects start position of field 1 in the range of 0 to 511 bits relative to the beginning of the packet.
        uint64_t        Reserved_15_9  :  7; // Reserved
        uint64_t         Field2Offset  :  9; // Selects start position of field 2 in the range of 0 to 511 bits relative to the beginning of the packet.
        uint64_t       Reserved_31_25  :  7; // Reserved
        uint64_t         Index1Offset  :  9; // Selects start position of index 1 in the range of 0 to 511 bits relative to the beginning of the packet.
        uint64_t       Reserved_43_41  :  3; // Reserved
        uint64_t          Index1Width  :  4; // Selects width of index 1 in range of 0 to 8 bits. Values 9 to 15 are reserved.
        uint64_t         Index2Offset  :  9; // Selects start position of index 2 in the range of 0 to 511 bits relative to the beginning of the packet.
        uint64_t       Reserved_59_57  :  3; // Reserved
        uint64_t          Index2Width  :  4; // Selects width of index 2 in range of 0 to 8 bits. Values 9 to 15 are reserved.
    } field;
    uint64_t val;
} RXHP_CFG_NPTL_RSM_SELECT_t;

// RXHP_CFG_NPTL_RSM_MATCH desc:
typedef union {
    struct {
        uint64_t                Mask1  :  8; // 8-bit mask for field 1 match
        uint64_t               Value1  :  8; // 8-bit value for field 1 match
        uint64_t                Mask2  :  8; // 8-bit mask for field 2 match
        uint64_t               Value2  :  8; // 8-bit value for field 2 match
        uint64_t       Reserved_63_32  : 32; // Reserved
    } field;
    uint64_t val;
} RXHP_CFG_NPTL_RSM_MATCH_t;

// RXHP_CFG_NPTL_RSM_MAP_TABLE desc:
typedef union {
    struct {
        uint64_t          RcvContextA  :  8; // Receive context used when the RSM index is equal to (i * 8) + 0 where i is the index of this CSR in the RcvRsmMapTable array
        uint64_t          RcvContextB  :  8; // Receive context used when the RSM index is equal to (i * 8) + 1 where i is the index of this CSR in the RcvRsmMapTable array
        uint64_t          RcvContextC  :  8; // Receive context used when the RSM index is equal to (i * 8) + 2 where i is the index of this CSR in the RcvRsmMapTable array
        uint64_t          RcvContextD  :  8; // Receive context used when the RSM index is equal to (i * 8) + 3 where i is the index of this CSR in the RcvRsmMapTable array
        uint64_t          RcvContextE  :  8; // Receive context used when the RSM index is equal to (i * 8) + 4 where i is the index of this CSR in the RcvRsmMapTable array
        uint64_t          RcvContextF  :  8; // Receive context used when the RSM index is equal to (i * 8) + 5 where i is the index of this CSR in the RcvRsmMapTable array
        uint64_t          RcvContextG  :  8; // Receive context used when the RSM index is equal to (i * 8) + 6 where i is the index of this CSR in the RcvRsmMapTable array
        uint64_t          RcvContextH  :  8; // Receive context used when the RSM index is equal to (i * 8) + 7 where i is the index of this CSR in the RcvRsmMapTable array
    } field;
    uint64_t val;
} RXHP_CFG_NPTL_RSM_MAP_TABLE_t;

// RXHP_STS_NPTL_RCV_DROPPED_PKT_CNT desc:
typedef union {
    struct {
        uint64_t                Count  : 64; // Incremented by hardware when a packet is dropped.
    } field;
    uint64_t val;
} RXHP_STS_NPTL_RCV_DROPPED_PKT_CNT_t;

// RXHP_STS_NPTL_RCV_INVALID_CONTEXT_CNT desc:
typedef union {
    struct {
        uint64_t                Count  : 64; // Incremented by hardware when a packet is dropped.
    } field;
    uint64_t val;
} RXHP_STS_NPTL_RCV_INVALID_CONTEXT_CNT_t;

// RXHP_STS_NPTL_RCV_JKEY_MISMATCH_CNT desc:
typedef union {
    struct {
        uint64_t                Count  : 64; // Incremented by hardware when a packet is dropped.
    } field;
    uint64_t val;
} RXHP_STS_NPTL_RCV_JKEY_MISMATCH_CNT_t;

// RXHP_STS_NPTL_RCV_DISABLED_CONTEXT_CNT desc:
typedef union {
    struct {
        uint64_t                Count  : 64; // Incremented by hardware when a packet is dropped.
    } field;
    uint64_t val;
} RXHP_STS_NPTL_RCV_DISABLED_CONTEXT_CNT_t;

// RXHP_CFG_PID_FLUSH desc:
typedef union {
    struct {
        uint64_t                  PID  : 12; // The PID to block.
        uint64_t                valid  :  1; // Set this to flush the PID from rxhp and block all new commands and packets for the PID
        uint64_t                  ack  :  1; // This will be set once active packets and commands for the PID have completed. All new packets/commands will be dropped. Clear this whenever setting valid or changing the PID.
        uint64_t       Reserved_63_14  : 50; // 
    } field;
    uint64_t val;
} RXHP_CFG_PID_FLUSH_t;

// RXHP_CFG_TRANSMIT_DELAY_1 desc:
typedef union {
    struct {
        uint64_t       US_PER_1KB_TC0  : 11; // Fixed point value to represent the number of microseconds per 1KB. In the form: xxxxxx.yyyyy Default to about .833ns/cycle * 32 cycles/1KB = 26.66 or 011010.10101
        uint64_t     US_PER_CYCLE_TC0  : 10; // Fixed point value to represent the number of microseconds per cycle (the decay rate of the bandwidth reservation). In the form: xx.yyyyyyyy Default to about .833ns = 00.11010101
        uint64_t       US_PER_1KB_TC1  : 11; // Fixed point value to represent the number of microseconds per 1KB. In the form: xxxxxx.yyyyy Default to about .833ns/cycle * 32 cycles/1KB = 26.66 or 011010.10101
        uint64_t     US_PER_CYCLE_TC1  : 10; // Fixed point value to represent the number of microseconds per cycle (the decay rate of the bandwidth reservation). In the form: xx.yyyyyyyy Default to about .833ns = 00.11010101
        uint64_t             SHARE_BW  :  1; // Use a single transmit delay logic (TC0) for rendezvous scheduling for all TCs.
        uint64_t       Reserved_63_43  : 21; // 
    } field;
    uint64_t val;
} RXHP_CFG_TRANSMIT_DELAY_1_t;

// RXHP_CFG_TRANSMIT_DELAY_2 desc:
typedef union {
    struct {
        uint64_t       US_PER_1KB_TC2  : 11; // Fixed point value to represent the number of microseconds per 1KB. In the form: xxxxxx.yyyyy Default to about .833ns/cycle * 32 cycles/1KB = 26.66 or 011010.10101
        uint64_t     US_PER_CYCLE_TC2  : 10; // Fixed point value to represent the number of microseconds per cycle (the decay rate of the bandwidth reservation). In the form: xx.yyyyyyyy Default to about .833ns = 00.11010101
        uint64_t       US_PER_1KB_TC3  : 11; // Fixed point value to represent the number of microseconds per 1KB. In the form: xxxxxx.yyyyy Default to about .833ns/cycle * 32 cycles/1KB = 26.66 or 011010.10101
        uint64_t     US_PER_CYCLE_TC3  : 10; // Fixed point value to represent the number of microseconds per cycle (the decay rate of the bandwidth reservation). In the form: xx.yyyyyyyy Default to about .833ns = 00.11010101
        uint64_t       Reserved_63_42  : 22; // 
    } field;
    uint64_t val;
} RXHP_CFG_TRANSMIT_DELAY_2_t;

// RXHP_CFG_RC_ORDERED_MAP desc:
typedef union {
    struct {
        uint64_t              ordered  :  8; // 1 bit = ordered packet 0 bit = unordered packet rc mapping to ordered
        uint64_t        Reserved_63_8  : 56; // Unused
    } field;
    uint64_t val;
} RXHP_CFG_RC_ORDERED_MAP_t;

// RXHP_CFG_EAGER_BLOCK desc:
typedef union {
    struct {
        uint64_t       pid_block_bits  : 64; // Each bit corresponds to a pid that may be blocked for eager packets.
    } field;
    uint64_t val;
} RXHP_CFG_EAGER_BLOCK_t;

// RXHP_CFG_PORT_MIRROR_PAYLOAD_START desc:
typedef union {
    struct {
        uint64_t              address  : 57; // Buffer start address.
        uint64_t       Reserved_63_57  :  7; // 
    } field;
    uint64_t val;
} RXHP_CFG_PORT_MIRROR_PAYLOAD_START_t;

// RXHP_CFG_PORT_MIRROR_PAYLOAD_END desc:
typedef union {
    struct {
        uint64_t              address  : 57; // Buffer end address.
        uint64_t       Reserved_63_57  :  7; // 
    } field;
    uint64_t val;
} RXHP_CFG_PORT_MIRROR_PAYLOAD_END_t;

// RXHP_CFG_PORT_MIRROR_HEADER_START desc:
typedef union {
    struct {
        uint64_t              address  : 57; // Buffer start address.
        uint64_t       Reserved_63_57  :  7; // 
    } field;
    uint64_t val;
} RXHP_CFG_PORT_MIRROR_HEADER_START_t;

// RXHP_CFG_PORT_MIRROR_HEADER_END desc:
typedef union {
    struct {
        uint64_t              address  : 57; // Buffer end address.
        uint64_t       Reserved_63_57  :  7; // 
    } field;
    uint64_t val;
} RXHP_CFG_PORT_MIRROR_HEADER_END_t;

// RXHP_CFG_PD_IRAM desc:
typedef union {
    struct {
        uint64_t                 data  : 64; // Instructions are 32 bits, so two lines of iram data fit into a single 8 byte csr. Writes to this csr will be loaded into a subset of the instruction rams, depending on the configuration in RXHP_CFG_HDR_PE.
    } field;
    uint64_t val;
} RXHP_CFG_PD_IRAM_t;

// RXHP_CFG_PD_DRAM desc:
typedef union {
    struct {
        uint64_t                 data  : 64; // Writes to this csr will be loaded into a subset of the instruction rams, depending on the configuration in RXHP_CFG_HDR_PE.
    } field;
    uint64_t val;
} RXHP_CFG_PD_DRAM_t;

// RXHP_ERR_STS_1 desc:
typedef union {
    struct {
        uint64_t          pe_inst_sbe  :  8; // PE instruction cache sbe
        uint64_t          pe_inst_mbe  :  8; // PE instruction cache mbe
        uint64_t          pe_data_sbe  :  8; // PE data cache sbe
        uint64_t          pe_data_mbe  :  8; // PE data cache mbe
        uint64_t           diagnostic  :  1; // Diagnostic Error Flag
        uint64_t    pte_cache_tag_mbe  :  1; // PTE Cache tag mbe Error information: Section 29.12.3.11, 'RXHP Error Info PTE Cache Tag MBE' Note: these are fairly fatal as you don't know what entry is bad.
        uint64_t    pte_cache_tag_sbe  :  1; // PTE Cache tag sbe Error information: . Section 29.12.3.10, 'RXHP Error Info PTE Cache Tag SBE'
        uint64_t   pte_cache_data_mbe  :  1; // PTE Cache data mbe Error information: Section 29.12.3.12, 'RXHP Error Info PTE Cache Data SBE/MBE'
        uint64_t   pte_cache_data_sbe  :  1; // PTE Cache data sbe Error information: . Section 29.12.3.12, 'RXHP Error Info PTE Cache Data SBE/MBE'
        uint64_t       psc0_cache_mbe  :  1; // PSC0 Cache mbe
        uint64_t       psc0_cache_sbe  :  1; // PSC0 Cache sbe
        uint64_t       psc1_cache_mbe  :  1; // PSC1 Cache mbe
        uint64_t       psc1_cache_sbe  :  1; // PSC1 Cache sbe
        uint64_t       hiarb_data_sbe  :  1; // hiarb interface sbe
        uint64_t       hiarb_data_mbe  :  1; // hiarb interface mbe
        uint64_t       qmap_table_sbe  :  1; // qmap table sbe
        uint64_t       qmap_table_mbe  :  1; // qmap table mbe
        uint64_t        flit_data_sbe  :  1; // flit data sbe
        uint64_t        flit_data_mbe  :  1; // flit data mbe
        uint64_t          ci_data_sbe  :  1; // command interface data sbe
        uint64_t          ci_data_mbe  :  1; // command interface data mbe
        uint64_t      trigop_data_sbe  :  1; // triggered op data sbe
        uint64_t      trigop_data_mbe  :  1; // triggered op data mbe
        uint64_t       pkt_status_sbe  :  1; // packet status sbe
        uint64_t       pkt_status_mbe  :  1; // packet status mbe
        uint64_t              ptq_sbe  :  1; // ptq sbe
        uint64_t              ptq_mbe  :  1; // ptq mbe
        uint64_t       Reserved_63_55  :  9; // 
    } field;
    uint64_t val;
} RXHP_ERR_STS_1_t;

// RXHP_ERR_CLR_1 desc:
typedef union {
    struct {
        uint64_t          events_54_0  : 55; // Write 1's to clear corresponding status bits.
        uint64_t       Reserved_63_55  :  9; // Reserved
    } field;
    uint64_t val;
} RXHP_ERR_CLR_1_t;

// RXHP_ERR_FRC_1 desc:
typedef union {
    struct {
        uint64_t          events_18_0  : 55; // Write 1's to set corresponding status bits.
        uint64_t       Reserved_63_55  :  9; // Reserved
    } field;
    uint64_t val;
} RXHP_ERR_FRC_1_t;

// RXHP_ERR_EN_HOST_1 desc:
typedef union {
    struct {
        uint64_t          events_54_0  : 55; // Enables corresponding status bits to generate host interrupt signal.
        uint64_t       Reserved_63_55  :  9; // Reserved
    } field;
    uint64_t val;
} RXHP_ERR_EN_HOST_1_t;

// RXHP_ERR_FIRST_HOST_1 desc:
typedef union {
    struct {
        uint64_t          events_54_0  : 55; // Snapshot of status bits when host interrupt signal transitions from 0 to 1.
        uint64_t       Reserved_63_55  :  9; // Reserved
    } field;
    uint64_t val;
} RXHP_ERR_FIRST_HOST_1_t;

// RXHP_ERR_EN_BMC_1 desc:
typedef union {
    struct {
        uint64_t          events_54_0  : 55; // Enable corresponding status bits to generate BMC interrupt signal.
        uint64_t       Reserved_63_55  :  9; // Reserved
    } field;
    uint64_t val;
} RXHP_ERR_EN_BMC_1_t;

// RXHP_ERR_FIRST_BMC_1 desc:
typedef union {
    struct {
        uint64_t          events_54_0  : 55; // Snapshot of status bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t       Reserved_63_55  :  9; // Reserved
    } field;
    uint64_t val;
} RXHP_ERR_FIRST_BMC_1_t;

// RXHP_ERR_EN_QUAR_1 desc:
typedef union {
    struct {
        uint64_t          events_54_0  : 55; // Enable corresponding status bits to generate quarantine signal.
        uint64_t       Reserved_63_55  :  9; // Reserved
    } field;
    uint64_t val;
} RXHP_ERR_EN_QUAR_1_t;

// RXHP_ERR_FIRST_QUAR_1 desc:
typedef union {
    struct {
        uint64_t          events_54_0  : 55; // Snapshot of status bits when quarantine signal transitions from 0 to 1.
        uint64_t       Reserved_63_55  :  9; // Reserved
    } field;
    uint64_t val;
} RXHP_ERR_FIRST_QUAR_1_t;

// RXHP_ERR_INFO_PTE_CACHE_TAG_SBE desc:
typedef union {
    struct {
        uint64_t    sbe_last_syndrome  :  7; // syndrome of the last least significant ecc domain sbe
        uint64_t      sbe_last_domain  :  1; // ecc domain of last least significant sbe
        uint64_t                  sbe  :  2; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            sbe_count  : 12; // saturating counter of sbes. The increment signal is the 'or' of the 2 sbe signals.
        uint64_t     sbe_last_address  : 10; // address of the last least significant ecc domain sbe
        uint64_t       Reserved_63_32  : 32; // Reserved
    } field;
    uint64_t val;
} RXHP_ERR_INFO_PTE_CACHE_TAG_SBE_t;

// RXHP_ERR_INFO_PTE_CACHE_TAG_MBE desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  7; // syndrome of the last least significant ecc domain mbe
        uint64_t      mbe_last_domain  :  1; // ecc domain of last least significant mbe
        uint64_t                  mbe  :  2; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            mbe_count  : 12; // saturating counter of mbes. The increment signal is the 'or' of the 2 mbe signals.
        uint64_t     mbe_last_address  : 10; // address of the last least significant ecc domain mbe
        uint64_t       Reserved_63_32  : 32; // Reserved
    } field;
    uint64_t val;
} RXHP_ERR_INFO_PTE_CACHE_TAG_MBE_t;

// RXHP_ERR_INFO_PTE_CACHE_DATA_SBE_MBE desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain mbe
        uint64_t                  mbe  :  3; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            mbe_count  :  4; // saturating counter of mbes.
        uint64_t      mbe_last_domain  :  2; // domain of the last least significant ecc domain mbe
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain sbe
        uint64_t                  sbe  :  3; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            sbe_count  : 12; // saturating counter of mbes.
        uint64_t      sbe_last_domain  :  2; // domain of the last least significant ecc domain sbe
        uint64_t       Reserved_63_42  : 22; // Reserved
    } field;
    uint64_t val;
} RXHP_ERR_INFO_PTE_CACHE_DATA_SBE_MBE_t;

// RXHP_ERR_INFO_PSC0_CACHE_SBE desc:
typedef union {
    struct {
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain sbe
        uint64_t      sbe_last_domain  :  4; // ecc domain of last least significant sbe
        uint64_t                  sbe  : 10; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            sbe_count  : 12; // saturating counter of sbes. The increment signal is the 'or' of the sbe signals.
        uint64_t     sbe_last_address  : 29; // address of the last least significant ecc domain sbe
        uint64_t       Reserved_63_63  :  1; // Reserved
    } field;
    uint64_t val;
} RXHP_ERR_INFO_PSC0_CACHE_SBE_t;

// RXHP_ERR_INFO_PSC0_CACHE_MBE desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain mbe
        uint64_t      mbe_last_domain  :  4; // ecc domain of last least significant mbe
        uint64_t                  mbe  : 10; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            mbe_count  : 12; // saturating counter of mbes. The increment signal is the 'or' of the mbe signals.
        uint64_t     mbe_last_address  : 29; // address of the last least significant ecc domain mbe
        uint64_t       Reserved_63_63  :  1; // Reserved
    } field;
    uint64_t val;
} RXHP_ERR_INFO_PSC0_CACHE_MBE_t;

// RXHP_ERR_INFO_PSC1_CACHE_SBE desc:
typedef union {
    struct {
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain sbe
        uint64_t      sbe_last_domain  :  4; // ecc domain of last least significant sbe
        uint64_t                  sbe  : 10; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            sbe_count  : 12; // saturating counter of sbes. The increment signal is the 'or' of the sbe signals.
        uint64_t     sbe_last_address  : 29; // address of the last least significant ecc domain sbe
        uint64_t       Reserved_63_63  :  1; // Reserved
    } field;
    uint64_t val;
} RXHP_ERR_INFO_PSC1_CACHE_SBE_t;

// RXHP_ERR_INFO_PSC1_CACHE_MBE desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain mbe
        uint64_t      mbe_last_domain  :  4; // ecc domain of last least significant mbe
        uint64_t                  mbe  : 10; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            mbe_count  : 12; // saturating counter of mbes. The increment signal is the 'or' of the mbe signals.
        uint64_t     mbe_last_address  : 29; // address of the last least significant ecc domain mbe
        uint64_t       Reserved_63_63  :  1; // Reserved
    } field;
    uint64_t val;
} RXHP_ERR_INFO_PSC1_CACHE_MBE_t;

// RXHP_ERR_INFO_FLIT_DATA_SBE_MBE desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain mbe
        uint64_t                  mbe  :  4; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            mbe_count  :  4; // saturating counter of mbes.
        uint64_t      mbe_last_domain  :  2; // domain of the last least significant ecc domain mbe
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain sbe
        uint64_t                  sbe  :  4; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            sbe_count  : 12; // saturating counter of mbes.
        uint64_t      sbe_last_domain  :  2; // domain of the last least significant ecc domain sbe
        uint64_t       Reserved_63_44  : 20; // Reserved
    } field;
    uint64_t val;
} RXHP_ERR_INFO_FLIT_DATA_SBE_MBE_t;

// RXHP_ERR_INFO_PACKET_STATUS_SBE_MBE desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain mbe
        uint64_t                  mbe  :  1; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            mbe_count  :  4; // saturating counter of mbes.
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain sbe
        uint64_t                  sbe  :  1; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            sbe_count  : 12; // saturating counter of mbes.
        uint64_t       Reserved_63_34  : 30; // Reserved
    } field;
    uint64_t val;
} RXHP_ERR_INFO_PACKET_STATUS_SBE_MBE_t;

// RXHP_ERR_INFO_TRIGOP_DATA_SBE_MBE desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain mbe
        uint64_t                  mbe  :  4; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            mbe_count  :  4; // saturating counter of mbes.
        uint64_t      mbe_last_domain  :  2; // domain of the last least significant ecc domain mbe
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain sbe
        uint64_t                  sbe  :  4; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            sbe_count  : 12; // saturating counter of mbes.
        uint64_t      sbe_last_domain  :  2; // domain of the last least significant ecc domain sbe
        uint64_t       Reserved_63_44  : 20; // Reserved
    } field;
    uint64_t val;
} RXHP_ERR_INFO_TRIGOP_DATA_SBE_MBE_t;

// RXHP_ERR_INFO_CI_DATA_SBE_MBE desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain mbe
        uint64_t                  mbe  :  4; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            mbe_count  :  4; // saturating counter of mbes.
        uint64_t      mbe_last_domain  :  2; // domain of the last least significant ecc domain mbe
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain sbe
        uint64_t                  sbe  :  4; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            sbe_count  : 12; // saturating counter of mbes.
        uint64_t      sbe_last_domain  :  2; // domain of the last least significant ecc domain sbe
        uint64_t       Reserved_63_44  : 20; // Reserved
    } field;
    uint64_t val;
} RXHP_ERR_INFO_CI_DATA_SBE_MBE_t;

// RXHP_ERR_INFO_HIARB_DATA_SBE_MBE desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain mbe
        uint64_t                  mbe  :  1; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            mbe_count  :  4; // saturating counter of mbes.
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain sbe
        uint64_t                  sbe  :  1; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            sbe_count  : 12; // saturating counter of mbes.
        uint64_t       Reserved_63_34  : 30; // Reserved
    } field;
    uint64_t val;
} RXHP_ERR_INFO_HIARB_DATA_SBE_MBE_t;

// RXHP_ERR_INFO_PTQ_SBE_MBE desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain mbe
        uint64_t                  mbe  :  3; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            mbe_count  :  4; // saturating counter of mbes.
        uint64_t      mbe_last_domain  :  2; // domain of the last least significant ecc domain mbe
        uint64_t       mbe_last_index  :  5; // index of the last least significant ecc domain mbe
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain sbe
        uint64_t                  sbe  :  3; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            sbe_count  : 12; // saturating counter of mbes.
        uint64_t      sbe_last_domain  :  2; // domain of the last least significant ecc domain sbe
        uint64_t       sbe_last_index  :  5; // index of the last least significant ecc domain sbe
        uint64_t       Reserved_63_52  : 12; // Reserved
    } field;
    uint64_t val;
} RXHP_ERR_INFO_PTQ_SBE_MBE_t;

// RXHP_ERR_INFO_QP_MAP_TABLE_SBE_MBE desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain mbe
        uint64_t                  mbe  :  1; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            mbe_count  :  4; // saturating counter of mbes.
        uint64_t       mbe_last_index  :  5; // index of the last least significant ecc domain mbe
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain sbe
        uint64_t                  sbe  :  1; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            sbe_count  : 12; // saturating counter of mbes.
        uint64_t       sbe_last_index  :  5; // index of the last least significant ecc domain sbe
        uint64_t       Reserved_63_44  : 20; // Reserved
    } field;
    uint64_t val;
} RXHP_ERR_INFO_QP_MAP_TABLE_SBE_MBE_t;

// RXHP_ERR_INFO_PE_INST_CACHE_SBE desc:
typedef union {
    struct {
        uint64_t    sbe_last_syndrome  :  7; // syndrome of the last least significant ecc domain sbe
        uint64_t        sbe_last_core  :  3; // core of last least significant sbe
        uint64_t                  sbe  :  8; // per core single bit set whenever an sbe occurs for that core. This helps find more significant sbe's when multiple cores have an sbe in the same clock and only the least significant core and syndrome is recorded.
        uint64_t            sbe_count  : 12; // saturating counter of sbes. The increment signal is the 'or' of the 2 sbe signals.
        uint64_t     sbe_last_address  : 12; // address of the last least significant ecc domain sbe
        uint64_t       Reserved_63_42  : 22; // Reserved
    } field;
    uint64_t val;
} RXHP_ERR_INFO_PE_INST_CACHE_SBE_t;

// RXHP_ERR_INFO_PE_INST_CACHE_MBE desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  7; // syndrome of the last least significant ecc domain mbe
        uint64_t        mbe_last_core  :  3; // core of last least significant mbe
        uint64_t                  mbe  :  8; // per core single bit set whenever an mbe occurs for that core. This helps find more significant mbe's when multiple cores have an mbe in the same clock and only the least significant core and syndrome is recorded.
        uint64_t            mbe_count  : 12; // saturating counter of mbes. The increment signal is the 'or' of the 2 mbe signals.
        uint64_t     mbe_last_address  : 12; // address of the last least significant ecc domain mbe
        uint64_t       Reserved_63_42  : 22; // Reserved
    } field;
    uint64_t val;
} RXHP_ERR_INFO_PE_INST_CACHE_MBE_t;

// RXHP_ERR_INFO_PE_DATA_CACHE_SBE desc:
typedef union {
    struct {
        uint64_t    sbe_last_syndrome  :  7; // syndrome of the last least significant ecc domain sbe
        uint64_t        sbe_last_core  :  3; // core of last least significant sbe
        uint64_t                  sbe  :  8; // per core single bit set whenever an sbe occurs for that core. This helps find more significant sbe's when multiple cores have an sbe in the same clock and only the least significant core and syndrome is recorded.
        uint64_t            sbe_count  : 12; // saturating counter of sbes. The increment signal is the 'or' of the 2 sbe signals.
        uint64_t     sbe_last_address  : 12; // address of the last least significant ecc domain sbe
        uint64_t       Reserved_63_42  : 22; // Reserved
    } field;
    uint64_t val;
} RXHP_ERR_INFO_PE_DATA_CACHE_SBE_t;

// RXHP_ERR_INFO_PE_DATA_CACHE_MBE desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  7; // syndrome of the last least significant ecc domain mbe
        uint64_t        mbe_last_core  :  3; // core of last least significant mbe
        uint64_t                  mbe  :  8; // per core single bit set whenever an mbe occurs for that core. This helps find more significant mbe's when multiple cores have an sbe in the same clock and only the least significant core and syndrome is recorded.
        uint64_t            mbe_count  : 12; // saturating counter of mbes. The increment signal is the 'or' of the 2 mbe signals.
        uint64_t     mbe_last_address  : 12; // address of the last least significant ecc domain mbe
        uint64_t       Reserved_63_42  : 22; // Reserved
    } field;
    uint64_t val;
} RXHP_ERR_INFO_PE_DATA_CACHE_MBE_t;

// RXHP_CACHE_ERR_INJECT_SBE_MBE desc:
typedef union {
    struct {
        uint64_t pte_cache_tag_err_inj_mask  :  7; // pte_cache_tag error inject mask
        uint64_t pte_cache_tag_err_inj_domain  :  1; // pte_cache_tag error inject domain. Which ecc domain to inject the error.
        uint64_t pte_cache_tag_err_inj_enable  :  1; // pte_cache_tag error inject enable.
        uint64_t pte_cache_data_err_inj_mask  :  8; // pte_cache_data error inject mask
        uint64_t pte_cache_data_err_inj_domain  :  2; // pte_cache_data error inject domain. Which ecc domain to inject the error.
        uint64_t pte_cache_data_err_inj_enable  :  1; // pte_cache_data error inject enable.
        uint64_t psc0_cache_err_inj_mask  :  8; // psc0_cache error inject mask
        uint64_t psc0_cache_err_inj_domain  :  4; // psc0_cache error inject domain. Which ecc domain to inject the error.
        uint64_t psc0_cache_err_inj_enable  :  1; // psc0_cache error inject enable.
        uint64_t psc1_cache_err_inj_mask  :  8; // psc1_cache error inject mask
        uint64_t psc1_cache_err_inj_domain  :  4; // psc1_cache error inject domain. Which ecc domain to inject the error.
        uint64_t psc1_cache_err_inj_enable  :  1; // psc1_cache error inject enable.
        uint64_t       Reserved_63_46  : 18; // Reserved
    } field;
    uint64_t val;
} RXHP_CACHE_ERR_INJECT_SBE_MBE_t;

// RXHP_DATA_PATH_ERR_INJECT_SBE_MBE desc:
typedef union {
    struct {
        uint64_t e2e_flit_err_inj_mask  :  8; // e2e flit error inject mask
        uint64_t pkt_status_err_inj_mask  :  8; // packet status error inject mask
        uint64_t trig_op_data_err_inj_mask  :  8; // triggered operation error inject mask
        uint64_t ci_data_err_inj_mask  :  8; // command interface error inject mask
        uint64_t     ptq_err_inj_mask  :  8; // PTQ error inject mask
        uint64_t      hi_err_inj_mask  :  8; // host interface error inject mask
        uint64_t e2e_flit_err_inj_domain  :  2; // e2e flit error inject domain. Which ecc domain to inject the error.
        uint64_t pkt_status_err_inj_domain  :  2; // packet status error inject domain. Which ecc domain to inject the error.
        uint64_t trig_op_data_err_inj_domain  :  2; // triggered operation error inject domain. Which ecc domain to inject the error.
        uint64_t ci_data_err_inj_domain  :  2; // command interface error inject domain. Which ecc domain to inject the error.
        uint64_t   ptq_err_inj_domain  :  2; // PTQ error inject domain. Which ecc domain to inject the error.
        uint64_t e2e_flit_err_inj_enable  :  1; // e2e flit error inject enable.
        uint64_t pkt_status_err_inj_enable  :  1; // packet status error inject enable.
        uint64_t trig_op_data_err_inj_enable  :  1; // triggered operation error inject enable.
        uint64_t ci_data_err_inj_enable  :  1; // command interface error inject enable.
        uint64_t   ptq_err_inj_enable  :  1; // PTQ error inject enable.
        uint64_t    hi_err_inj_enable  :  1; // host interface error inject enable.
    } field;
    uint64_t val;
} RXHP_DATA_PATH_ERR_INJECT_SBE_MBE_t;

// RXHP_CONFIG_ERR_INJECT_SBE_MBE desc:
typedef union {
    struct {
        uint64_t pe_imem_err_inj_mask  :  7; // pe imem error inject mask
        uint64_t pe_imem_err_inj_core  :  3; // pe imem error inject core. Which core to inject the error.
        uint64_t pe_imem_err_inj_enable  :  1; // pe imem error inject enable.
        uint64_t pe_dmem_err_inj_mask  :  8; // pe dmem error inject mask
        uint64_t pe_dmem_err_inj_core  :  3; // pe dmem error inject core. Which core to inject the error.
        uint64_t pe_dmem_err_inj_enable  :  1; // pe dmem error inject enable.
        uint64_t  qp_map_err_inj_mask  :  8; // qp map error inject mask
        uint64_t qp_map_err_inj_enable  :  1; // qp map error inject enable.
        uint64_t       Reserved_63_32  : 32; // Reserved
    } field;
    uint64_t val;
} RXHP_CONFIG_ERR_INJECT_SBE_MBE_t;

// RXHP_DBG_PORTALS_TBL_ADDR desc:
typedef union {
    struct {
        uint64_t              address  : 12; // Address of Entry to be accessed
        uint64_t            write_cmd  :  1; // Write = 1, Read = 0.
        uint64_t       Reserved_15_13  :  3; // 
        uint64_t         cmd_complete  :  1; // Set to one after a completed access
        uint64_t              new_cmd  :  1; // Indicated the command in this CSR is ready for hardware.
        uint64_t       Reserved_63_18  : 46; // 
    } field;
    uint64_t val;
} RXHP_DBG_PORTALS_TBL_ADDR_t;

// RXHP_DBG_PORTALS_TBL_DATA desc:
typedef union {
    struct {
        uint64_t                 data  : 64; // data
    } field;
    uint64_t val;
} RXHP_DBG_PORTALS_TBL_DATA_t;

// RXHP_DBG_PORTALS_TBL_DATA_ECC desc:
typedef union {
    struct {
        uint64_t                  ecc  : 64; // ecc
    } field;
    uint64_t val;
} RXHP_DBG_PORTALS_TBL_DATA_ECC_t;

// RXHP_DBG_PORTALS_LIST_ADDR desc:
typedef union {
    struct {
        uint64_t              address  : 15; // Address of Entry to be accessed
        uint64_t            write_cmd  :  1; // Write = 1, Read = 0.
        uint64_t         cmd_complete  :  1; // Set to one after a completed access
        uint64_t              new_cmd  :  1; // Indicated the command in this CSR is ready for hardware.
        uint64_t       Reserved_63_18  : 46; // 
    } field;
    uint64_t val;
} RXHP_DBG_PORTALS_LIST_ADDR_t;

// RXHP_DBG_PORTALS_LIST_DATA desc:
typedef union {
    struct {
        uint64_t                 Data  : 64; // 
    } field;
    uint64_t val;
} RXHP_DBG_PORTALS_LIST_DATA_t;

// RXHP_DBG_PORTALS_LIST_DATA_ECC1 desc:
typedef union {
    struct {
        uint64_t                 Data  : 48; // 
        uint64_t                  ecc  : 16; // 
    } field;
    uint64_t val;
} RXHP_DBG_PORTALS_LIST_DATA_ECC1_t;

// RXHP_DBG_PORTALS_LIST_DATA_ECC2 desc:
typedef union {
    struct {
        uint64_t                  ecc  : 56; // 
        uint64_t       Reserved_63_56  :  8; // 
    } field;
    uint64_t val;
} RXHP_DBG_PORTALS_LIST_DATA_ECC2_t;

// RXHP_DBG_PTE_CACHE_TAG_WAY_ENABLE desc:
typedef union {
    struct {
        uint64_t           way_enable  :  4; // 1 bits enable, 0 bits disable PTE Cache Tag ways.
        uint64_t        Reserved_63_4  : 60; // Reserved
    } field;
    uint64_t val;
} RXHP_DBG_PTE_CACHE_TAG_WAY_ENABLE_t;

#endif /* ___FXR_rx_hp_CSRS_H__ */