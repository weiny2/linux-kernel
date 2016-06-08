// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Thu Jun  2 19:11:24 2016
//

#ifndef ___FXR_rx_et_CSRS_H__
#define ___FXR_rx_et_CSRS_H__

// RXET_CFG_CREDITS desc:
typedef union {
    struct {
        uint64_t  rxdma_to_credit_max  :  4; // Credit max for RxDMA TrigOp request
        uint64_t   rxhp_to_credit_max  :  4; // Credit max for RxHP TrigOp Insert/Append
        uint64_t        Reserved_63_8  : 56; // Reserved
    } field;
    uint64_t val;
} RXET_CFG_CREDITS_t;

// RXET_CFG_EQD desc:
typedef union {
    struct {
        uint64_t  head_refetch_thresh  : 22; // Threshold to trigger refetch of EQ head
        uint64_t       unreserved_min  : 22; // Credit max for RxDMA TrigOp request. Must be >=2.
        uint64_t       Reserved_63_44  : 20; // Reserved
    } field;
    uint64_t val;
} RXET_CFG_EQD_t;

// RXET_CFG_CH desc:
typedef union {
    struct {
        uint64_t           ch_trig_rd  :  2; // TrigOp cache read
        uint64_t     ch_trig_flush_wr  :  2; // TrigOp cache flush write
        uint64_t    ch_trig_victim_wr  :  2; // TrigOp cache victim write
        uint64_t            ch_eqd_rd  :  2; // EQ Descriptor cache read
        uint64_t      ch_eqd_flush_wr  :  2; // EQ Descriptor cache flush write
        uint64_t     ch_eqd_victim_wr  :  2; // EQ Descriptor cache victim write
        uint64_t       Reserved_63_12  : 52; // Unused
    } field;
    uint64_t val;
} RXET_CFG_CH_t;

// RXET_CFG_EQ_DESC_CACHE_CLIENT_DISABLE desc:
typedef union {
    struct {
        uint64_t client_eq_desc_cache_request_disable  :  1; // Client EQ desc cache request disable.
        uint64_t        Reserved_63_1  : 63; // Unused
    } field;
    uint64_t val;
} RXET_CFG_EQ_DESC_CACHE_CLIENT_DISABLE_t;

// RXET_CFG_EQ_DESC_CACHE_ACCESS_CTL desc:
typedef union {
    struct {
        uint64_t              address  : 25; // The contents of this field differ based on cmd. eq_desc_cache_addr_t for CACHE_CMD_RD, CACHE_CMD_WR,CACHE_CMD_INVALIDATE,CACHE_CMD_FLUSH_INVALID,CACHE_CMD_FLUSH_VALID. 10:0 used for CACHE_CMD_DATA_RD,CACHE_CMD_DATA_WR 8:0 used for CACHE_CMD_TAG_RD,CACHE_CMD_TAG_WR
        uint64_t         mask_address  : 25; // cache cmd mask address. 1 bits are don't care. only used for CACHE_CMD_INVALIDATE, CACHE_CMD_FLUSH_INVALID, CACHE_CMD_FLUSH_STAY The form of this field is eq_desc_cache_addr_t. initially all 1's. Note: If this field is 0, a normal single lookup is done. If non-zero, the entire cache tag memory is read and tag entries match/masked.
        uint64_t                  cmd  :  4; // cache cmd. see <blue text>Section A.1.1, 'FXR Cache Cmd enums' initially CACHE_CMD_INVALIDATE
        uint64_t                 busy  :  1; // SW sets busy when writing this csr. HW clears busy when cmd is complete. busy must be clear before writing this csr. Ifbusy is set, HW is busy on a previous cmd. Coming out of reset, busy will be 1 so as to initiate the eq desc cache tag invalidation. Then in 2k clks or so, it will go to 0.
        uint64_t                  Rsv  :  9; // 
    } field;
    uint64_t val;
} RXET_CFG_EQ_DESC_CACHE_ACCESS_CTL_t;

// RXET_CFG_EQ_DESC_CACHE_ACCESS_DATA desc:
typedef union {
    struct {
        uint64_t                 data  : 64; // eq desc cache data[127:0]
    } field;
    uint64_t val;
} RXET_CFG_EQ_DESC_CACHE_ACCESS_DATA_t;

// RXET_CFG_EQ_DESC_CACHE_ACCESS_DATA_BIT_ENABLE desc:
typedef union {
    struct {
        uint64_t           bit_enable  : 64; // eq desc cache wr data bit enable[127:0]. 1 bits are written.
    } field;
    uint64_t val;
} RXET_CFG_EQ_DESC_CACHE_ACCESS_DATA_BIT_ENABLE_t;

// RXET_CFG_EQ_DESC_CACHE_ACCESS_TAG desc:
typedef union {
    struct {
        uint64_t      tag_way_addr_lo  : 25; // eq desc cache tag way address, format is eq_desc_cache_addr_t
        uint64_t     tag_way_valid_lo  :  1; // eq desc cache tag way valid
        uint64_t           reserve_lo  :  6; // 
        uint64_t      tag_way_addr_hi  : 25; // eq desc cache tag way address, format is eq_desc_cache_addr_t
        uint64_t     tag_way_valid_hi  :  1; // eq desc cache tag way valid
        uint64_t           reserve_hi  :  6; // 
    } field;
    uint64_t val;
} RXET_CFG_EQ_DESC_CACHE_ACCESS_TAG_t;

// RXET_CFG_TRIG_OP_CACHE_CLIENT_DISABLE desc:
typedef union {
    struct {
        uint64_t client_trig_op_cache_request_disable  :  1; // Client Trig Op cache request disable.
        uint64_t        Reserved_63_1  : 63; // Unused
    } field;
    uint64_t val;
} RXET_CFG_TRIG_OP_CACHE_CLIENT_DISABLE_t;

// RXET_CFG_TRIG_OP_CACHE_ACCESS_CTL desc:
typedef union {
    struct {
        uint64_t              address  : 28; // The contents of this field differ based on cmd. trig_op_cache_addr_t for CACHE_CMD_RD, CACHE_CMD_WR,CACHE_CMD_INVALIDATE,CACHE_CMD_FLUSH_INVALID,CACHE_CMD_FLUSH_VALID. 9:0 used for CACHE_CMD_DATA_RD,CACHE_CMD_DATA_WR 7:0 used for CACHE_CMD_TAG_RD,CACHE_CMD_TAG_WR
        uint64_t         mask_address  : 28; // cache cmd mask address. 1 bits are don't care. only used for CACHE_CMD_INVALIDATE, CACHE_CMD_FLUSH_INVALID, CACHE_CMD_FLUSH_VALID The form of this field is trig_op_cache_addr_t. initially all 1's. Note: If this field is 0, a normal single lookup is done. If non-zero, the entire cache tag memory is read and tag entries match/masked.
        uint64_t                  cmd  :  4; // cache cmd. see <blue text>Section A.1.1, 'FXR Cache Cmd enums' initially CACHE_CMD_INVALIDATE
        uint64_t                 busy  :  1; // SW sets busy when writing this csr. HW clears busy when cmd is complete. busy must be clear before writing this csr. If busy is set, HW is busy on a previous cmd. Coming out of reset, this will be 1 so as to initiate the Trig Op cache tag invalidation. Then in 256 clks or so, it will go to 0.
        uint64_t                  Rsv  :  3; // 
    } field;
    uint64_t val;
} RXET_CFG_TRIG_OP_CACHE_ACCESS_CTL_t;

// RXET_CFG_TRIG_OP_CACHE_ACCESS_DATA desc:
typedef union {
    struct {
        uint64_t                 data  : 64; // Trig Op cache data[1023:0]
    } field;
    uint64_t val;
} RXET_CFG_TRIG_OP_CACHE_ACCESS_DATA_t;

// RXET_CFG_TRIG_OP_CACHE_ACCESS_DATA_BIT_ENABLE desc:
typedef union {
    struct {
        uint64_t           bit_enable  : 64; // Trig Op cache wr data bit enable[1023:0]. 1 bits are written.
    } field;
    uint64_t val;
} RXET_CFG_TRIG_OP_CACHE_ACCESS_DATA_BIT_ENABLE_t;

// RXET_CFG_TRIG_OP_CACHE_ACCESS_TAG desc:
typedef union {
    struct {
        uint64_t      tag_way_addr_lo  : 28; // Trig Op cache tag way address, format is eq_desc_cache_addr_t
        uint64_t     tag_way_dirty_lo  :  1; // Trig Op cache tag way dirty
        uint64_t     tag_way_valid_lo  :  1; // Trig Op cache tag way valid
        uint64_t           reserve_lo  :  2; // 
        uint64_t      tag_way_addr_hi  : 28; // Trig Op cache tag way address, format is eq_desc_cache_addr_t
        uint64_t     tag_way_dirty_hi  :  1; // Trig Op cache tag way dirty
        uint64_t     tag_way_valid_hi  :  1; // Trig Op cache tag way valid
        uint64_t           reserve_hi  :  2; // 
    } field;
    uint64_t val;
} RXET_CFG_TRIG_OP_CACHE_ACCESS_TAG_t;

// RXET_STS_CREDITS desc:
typedef union {
    struct {
        uint64_t   hiarb_credit_count  :  5; // Credit count for RxHiArb
        uint64_t        Reserved_63_5  : 59; // Not used
    } field;
    uint64_t val;
} RXET_STS_CREDITS_t;

// RXET_DBG_EVENT_HDR_VALID desc:
typedef union {
    struct {
        uint64_t     eb_event_hdr_val  : 58; // Vector of occupied locations in Event Header queue
        uint64_t       Reserved_63_58  :  6; // Not used
    } field;
    uint64_t val;
} RXET_DBG_EVENT_HDR_VALID_t;

// RXET_DBG_CAM_VALID desc:
typedef union {
    struct {
        uint64_t          eb_cm_valid  : 58; // Vector of occupied locations in CAM and Event Buffer.
        uint64_t       Reserved_63_58  :  6; // Not used
    } field;
    uint64_t val;
} RXET_DBG_CAM_VALID_t;

// RXET_DBG_EB_EEH_TOH_STATE desc:
typedef union {
    struct {
        uint64_t eeh_eqd_req_fifo_empty  :  1; // EQD request source (tag) FIFO empty
        uint64_t   eeh_rsv_fifo_empty  :  1; // EQD request for reservation FIFO empty
        uint64_t eeh_event_fifo_empty  :  1; // EQD request for Events FIFO empty
        uint64_t  eb_rxdma_fifo_empty  :  1; // RxDMA Event request FIFO empty
        uint64_t      eb_addr_q_empty  :  1; // Event address FIFO for EQD requests empty
        uint64_t   eb_rxhp_fifo_empty  :  1; // RxHP Event input FIFO empty
        uint64_t  eb_txotr_fifo_empty  :  1; // TxOTR Event input FIFO empty
        uint64_t toh_trig_out_fifo_empty  :  1; // TrigOp response to RxDMA FIFO empty
        uint64_t toh_cache_issue_fifo_empty  :  1; // TrigOp cache read issuer tag FIFO empty
        uint64_t toh_trig_in_fifo_empty  :  1; // TrigOp requests from RxDMA empty
        uint64_t       Reserved_15_10  :  6; // Not used
        uint64_t toh_trig_to_proc_valid  :  4; // TrigOp in process stage valid
        uint64_t        toh_need_read  :  4; // TrigOp in process needs read
        uint64_t  toh_need_invalidate  :  4; // TrigOp in process needs invalidate
        uint64_t    toh_need_response  :  4; // TrigOp in process needs response
        uint64_t       toh_needs_trig  :  4; // TripOp stage needs or will need trig
        uint64_t        toh_load_trig  :  4; // TrigOp stage to load from input queue
        uint64_t       Reserved_63_40  : 24; // Not used
    } field;
    uint64_t val;
} RXET_DBG_EB_EEH_TOH_STATE_t;

// RXET_DBG_TOA_STATE desc:
typedef union {
    struct {
        uint64_t        toah_to_valid  :  8; // TrigOp Append/Insert valid vector
        uint64_t          toah_to_cmd  :  8; // TrigOp command in process. 0=Insert, 1=Append
        uint64_t         toah_ct_rcvd  :  8; // CT received from RxDMA
        uint64_t         toah_rd_sent  :  8; // Insert: Write next sent; Append: Read prev sent
        uint64_t        toah_wrp_sent  :  8; // Write prev is done
        uint64_t       toah_wrto_sent  :  8; // TrigOp has been written
        uint64_t          toah_walked  :  8; // Insert: successful walk complete; Append: immediately valid
        uint64_t       Reserved_63_56  :  8; // Not used
    } field;
    uint64_t val;
} RXET_DBG_TOA_STATE_t;

// RXET_DBG_EQ_DESC_CACHE_TAG_WAY_ENABLE desc:
typedef union {
    struct {
        uint64_t           way_enable  :  4; // 1 bits enable, 0 bits disable EQ Desc Cache Tag ways.
        uint64_t        Reserved_63_4  : 60; // Reserved
    } field;
    uint64_t val;
} RXET_DBG_EQ_DESC_CACHE_TAG_WAY_ENABLE_t;

// RXET_DBG_TRIG_OP_CACHE_TAG_WAY_ENABLE desc:
typedef union {
    struct {
        uint64_t           way_enable  :  4; // 1 bits enable, 0 bits disable Trig Op Cache Tag ways.
        uint64_t        Reserved_63_4  : 60; // Reserved
    } field;
    uint64_t val;
} RXET_DBG_TRIG_OP_CACHE_TAG_WAY_ENABLE_t;

// RXET_ERR_STS desc:
typedef union {
    struct {
        uint64_t       eb_cam_par_err  :  1; // Parity error detected on event cam. This is fatal.
        uint64_t eq_desc_cache_tag_mbe  :  1; // EQ Desc Cache tag mbe.The 'or' of the 2 ecc domains. Error information: Section 28.14.6.12, ' RXET_ERR_INFO_EQ_DESC_CACHE_TAG_MBE - RXET Error Info EQ Desc Cache Tag MBE' Note: these are fairly fatal as you don't know what entry is bad.
        uint64_t eq_desc_cache_tag_sbe  :  1; // EQ Desc Cache tag sbe.The 'or' of the 2 ecc domains. Error information: . Section 28.14.6.11, ' RXET_ERR_INFO_EQ_DESC_CACHE_TAG_SBE - RXET Error Info EQ Desc Cache Tag SBE'
        uint64_t eq_desc_cache_data_mbe  :  1; // EQ Desc Cache data mbe. The 'or' of the 2 ecc domains. Error information: Section 28.14.6.13, ' RXET_ERR_INFO_EQ_DESC_CACHE_DATA_SBE_MBE - RXET Error Info EQ Desc Cache Data SBE/MBE'
        uint64_t eq_desc_cache_data_sbe  :  1; // EQ Desc Cache data sbe. The 'or' of the 2 ecc domains. Error information: . Section 28.14.6.13, ' RXET_ERR_INFO_EQ_DESC_CACHE_DATA_SBE_MBE - RXET Error Info EQ Desc Cache Data SBE/MBE'
        uint64_t trig_op_cache_tag_mbe  :  1; // Trig Op Cache tag mbe.The 'or' of the 2 ecc domains. Error information: Section 28.14.6.15, ' RXET_ERR_INFO_TRIG_OP_CACHE_TAG_MBE - RXET Error Info Trig Op Cache Tag MBE' Note: these are fairly fatal as you don't know what entry is bad.
        uint64_t trig_op_cache_tag_sbe  :  1; // Trig Op Cache tag sbe.The 'or' of the 2 ecc domains. Error information: . Section 28.14.6.14, ' RXET_ERR_INFO_TRIG_OP_CACHE_TAG_SBE - RXET Error Info Trig Op Cache Tag SBE'
        uint64_t trig_op_cache_data_mbe  :  1; // Trig Op Cache data mbe. The 'or' of the 16 ecc domains. Error information: Section 28.14.6.16, ' RXET_ERR_INFO_TRIG_OP_CACHE_DATA_SBE - RXET Error Info Trig Op Cache Data SBE'
        uint64_t trig_op_cache_data_sbe  :  1; // Trig Op Cache data sbe. The 'or' of the 16ecc domains. Error information: . Section 28.14.6.16, ' RXET_ERR_INFO_TRIG_OP_CACHE_DATA_SBE - RXET Error Info Trig Op Cache Data SBE'
        uint64_t toh_trig_out_overflow  :  1; // Overflow on TrigOp response to RxDMA - 4
        uint64_t toh_cache_req_fifo_overflow  :  1; // Overflow on TrigOp cache active requests - 8
        uint64_t toh_trig_in_fifo_overflow  :  1; // Overflow in TrigOp holding FIFO - 4
        uint64_t eeh_eqd_req_fifo_overflow  :  1; // Overflow on EQD active requests - 8
        uint64_t eeh_rsv_fifo_overflow  :  1; // Overflow on EQD requests for reservations - 8
        uint64_t eeh_event_fifo_overflow  :  1; // Overflow on EQD requests for events - 58
        uint64_t eb_rxdma_fifo_overflow  :  1; // Overflow on RxDMA input requests FIFO - 9 requests
        uint64_t eb_event_hdr_q_overflow  :  1; // Overflow on event header queue for event sideband info - 58 events
        uint64_t   eb_addr_q_overflow  :  1; // Overflow on event address FIFO for EQD cache request - 58
        uint64_t  rxhp_event_overflow  :  1; // Overflow on RxHP event interface FIFO - one event (2 flits)
        uint64_t txotr_event_overflow  :  1; // Overflow on TxOTR event interface FIFO - one event (2 flits)
        uint64_t    eb_addr_q_err_mbe  :  1; // MBE detected on Event Address queue.
        uint64_t    eb_addr_q_err_sbe  :  1; // SBE detected on Event Address queue.
        uint64_t          rsv_err_mbe  :  1; // MBE detected on Event reservation input queue.
        uint64_t          rsv_err_sbe  :  1; // SBE detected on Event reservation input queue.
        uint64_t    event_req_err_mbe  :  1; // MBE detected on Event request input queue.
        uint64_t    event_req_err_sbe  :  1; // SBE detected on Event request input queue.
        uint64_t          toa_err_mbe  :  1; // MBE detected on TrigOp Append request TrigOp data. Checked on 8 processing stages so is *or* of all.
        uint64_t          toa_err_sbe  :  1; // SBE detected on TrigOp Append request TrigOp data. Checked on 8 processing stages so is *or* of all.
        uint64_t    to_cache_resp_err  :  1; // resp_err from HiArb for TrigOp cache request
        uint64_t      to_cache_fr_err  :  1; // Framing error on TrigOp cache response from HiArb.
        uint64_t    eq_cache_resp_err  :  1; // resp_err from HiArb for EqDesc cache request
        uint64_t      eq_cache_fr_err  :  1; // Framing error on EqDesc cache response from HiArb.
        uint64_t              toa_err  :  1; // TrigOp Append request error - CT response not written *or* CT from RxDMA with no matching TrigOp from RxHP *or* TOA received with no TO buffers available.
        uint64_t     eb_cam_write_err  :  1; // New event with no empty slots in CAM/event buffer.
        uint64_t     eb_cam_match_err  :  1; // No valid match in Event Buffer for request from RxDMA.
        uint64_t rxhp_event_frame_err  :  1; // Framing error on Event from RxHP.
        uint64_t txotr_event_frame_err  :  1; // Framing error on Event from TxOTR.
        uint64_t       Reserved_63_37  : 27; // Not used.
    } field;
    uint64_t val;
} RXET_ERR_STS_t;

// RXET_ERR_CLR desc:
typedef union {
    struct {
        uint64_t            error_clr  : 37; // Write 1's to clear corresponding status bits.
        uint64_t       Reserved_63_37  : 27; // Reserved
    } field;
    uint64_t val;
} RXET_ERR_CLR_t;

// RXET_ERR_FRC desc:
typedef union {
    struct {
        uint64_t            force_err  : 37; // Write 1's to set corresponding status bits.
        uint64_t       Reserved_63_37  : 27; // Reserved
    } field;
    uint64_t val;
} RXET_ERR_FRC_t;

// RXET_ERR_EN_HOST desc:
typedef union {
    struct {
        uint64_t          err_host_en  : 37; // Enables corresponding status bits to generate host interrupt signal.
        uint64_t       Reserved_63_37  : 27; // Reserved
    } field;
    uint64_t val;
} RXET_ERR_EN_HOST_t;

// RXET_ERR_FIRST_HOST desc:
typedef union {
    struct {
        uint64_t           first_host  : 37; // Snapshot of status bits when host interrupt signal transitions from 0 to 1.
        uint64_t       Reserved_63_37  : 27; // Reserved
    } field;
    uint64_t val;
} RXET_ERR_FIRST_HOST_t;

// RXET_ERR_EN_BMC desc:
typedef union {
    struct {
        uint64_t               bmc_en  : 37; // Enabe corresponding status bits to generate BMC interrupt signal.
        uint64_t       Reserved_63_37  : 27; // Reserved
    } field;
    uint64_t val;
} RXET_ERR_EN_BMC_t;

// RXET_ERR_FIRST_BMC desc:
typedef union {
    struct {
        uint64_t            first_bmc  : 37; // Snapshot of status bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t       Reserved_63_37  : 27; // Reserved
    } field;
    uint64_t val;
} RXET_ERR_FIRST_BMC_t;

// RXET_ERR_EN_QUAR desc:
typedef union {
    struct {
        uint64_t              quar_en  : 37; // Enable corresponding status bits to generate quarantine signal.
        uint64_t       Reserved_63_37  : 27; // Reserved
    } field;
    uint64_t val;
} RXET_ERR_EN_QUAR_t;

// RXET_ERR_FIRST_QUAR desc:
typedef union {
    struct {
        uint64_t           first_quar  : 37; // Snapshot of status bits when quarantine signal transitions from 0 to 1.
        uint64_t       Reserved_63_37  : 27; // Reserved
    } field;
    uint64_t val;
} RXET_ERR_FIRST_QUAR_t;

// RXET_ERR_INFO desc:
typedef union {
    struct {
        uint64_t    eb_addr_sbe_count  : 16; // SBE count for Event address queue
        uint64_t    rsv_req_sbe_count  : 16; // SBE count for Event reservation EQD cache queue
        uint64_t  event_req_sbe_count  : 16; // SBE count for Event EQD cache queue
        uint64_t   toa_data_sbe_count  : 16; // SBE count for TrigOp Append data. Checked on 8 processing stages. Incremented by 1 if any/mulitiple have sbe.
    } field;
    uint64_t val;
} RXET_ERR_INFO_t;

// RXET_ERR_INFO_EQ_DESC_CACHE_TAG_SBE desc:
typedef union {
    struct {
        uint64_t    sbe_last_syndrome  :  7; // syndrome of the last least significant ecc domain sbe
        uint64_t      sbe_last_domain  :  1; // ecc domain of last least significant sbe
        uint64_t                  sbe  :  2; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            sbe_count  : 12; // saturating counter of sbes. The increment signal is the 'or' of the 2 sbe signals.
        uint64_t     sbe_last_address  :  9; // address of the last least significant ecc domain sbe
        uint64_t       Reserved_63_31  : 33; // Reserved
    } field;
    uint64_t val;
} RXET_ERR_INFO_EQ_DESC_CACHE_TAG_SBE_t;

// RXET_ERR_INFO_EQ_DESC_CACHE_TAG_MBE desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  7; // syndrome of the last least significant ecc domain mbe
        uint64_t      mbe_last_domain  :  1; // ecc domain of last least significant mbe
        uint64_t                  mbe  :  2; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            mbe_count  : 12; // saturating counter of mbes. The increment signal is the 'or' of the 2 mbe signals.
        uint64_t     mbe_last_address  :  9; // address of the last least significant ecc domain mbe
        uint64_t       Reserved_63_31  : 33; // Reserved
    } field;
    uint64_t val;
} RXET_ERR_INFO_EQ_DESC_CACHE_TAG_MBE_t;

// RXET_ERR_INFO_EQ_DESC_CACHE_DATA_SBE_MBE desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain mbe
        uint64_t                  mbe  :  2; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            mbe_count  :  4; // saturating counter of mbes.
        uint64_t      mbe_last_domain  :  1; // domain of the last least significant ecc domain mbe
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain sbe
        uint64_t                  sbe  :  2; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            sbe_count  : 12; // saturating counter of sbes.
        uint64_t      sbe_last_domain  :  1; // domain of the last least significant ecc domain sbe
        uint64_t       Reserved_63_38  : 26; // Reserved
    } field;
    uint64_t val;
} RXET_ERR_INFO_EQ_DESC_CACHE_DATA_SBE_MBE_t;

// RXET_ERR_INFO_TRIG_OP_CACHE_TAG_SBE desc:
typedef union {
    struct {
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain sbe
        uint64_t      sbe_last_domain  :  1; // ecc domain of last least significant sbe
        uint64_t                  sbe  :  2; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            sbe_count  : 12; // saturating counter of sbes. The increment signal is the 'or' of the 2 sbe signals.
        uint64_t     sbe_last_address  :  8; // address of the last least significant ecc domain sbe
        uint64_t       Reserved_63_31  : 33; // Reserved
    } field;
    uint64_t val;
} RXET_ERR_INFO_TRIG_OP_CACHE_TAG_SBE_t;

// RXET_ERR_INFO_TRIG_OP_CACHE_TAG_MBE desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain mbe
        uint64_t      mbe_last_domain  :  1; // ecc domain of last least significant mbe
        uint64_t                  mbe  :  2; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            mbe_count  : 12; // saturating counter of mbes. The increment signal is the 'or' of the 2 mbe signals.
        uint64_t     mbe_last_address  :  8; // address of the last least significant ecc domain mbe
        uint64_t       Reserved_63_31  : 33; // Reserved
    } field;
    uint64_t val;
} RXET_ERR_INFO_TRIG_OP_CACHE_TAG_MBE_t;

// RXET_ERR_INFO_TRIG_OP_CACHE_DATA_SBE desc:
typedef union {
    struct {
        uint64_t    sbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain sbe
        uint64_t                  sbe  : 16; // per domain single bit set whenever an sbe occurs for that domain. This helps find more significant sbe's when multiple domains have an sbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            sbe_count  : 12; // saturating counter of sbes.
        uint64_t      sbe_last_domain  :  4; // domain of the last least significant ecc domain sbe
        uint64_t       Reserved_63_40  : 24; // Reserved
    } field;
    uint64_t val;
} RXET_ERR_INFO_TRIG_OP_CACHE_DATA_SBE_t;

// RXET_ERR_INFO_TRIG_OP_CACHE_DATA_MBE desc:
typedef union {
    struct {
        uint64_t    mbe_last_syndrome  :  8; // syndrome of the last least significant ecc domain mbe
        uint64_t                  mbe  : 16; // per domain single bit set whenever an mbe occurs for that domain. This helps find more significant mbe's when multiple domains have an mbe in the same clock and only the least significant domain and syndrome is recorded.
        uint64_t            mbe_count  : 12; // saturating counter of mbes.
        uint64_t      mbe_last_domain  :  4; // domain of the last least significant ecc domain mbe
        uint64_t       Reserved_63_40  : 24; // Reserved
    } field;
    uint64_t val;
} RXET_ERR_INFO_TRIG_OP_CACHE_DATA_MBE_t;

// RXET_CACHE_ERR_INJECT_SBE_MBE desc:
typedef union {
    struct {
        uint64_t eq_desc_cache_tag_err_inj_mask  :  7; // eq_desc_cache_tag error inject mask
        uint64_t eq_desc_cache_tag_err_inj_domain  :  1; // eq_desc_cache_tag error inject domain. Which ecc domain to inject the error.
        uint64_t eq_desc_cache_tag_err_inj_enable  :  1; // eq_desc_cache_tag error inject enable.
        uint64_t eq_desc_cache_data_err_inj_mask  :  8; // eq_desc_cache_data error inject mask
        uint64_t eq_desc_cache_data_err_inj_domain  :  1; // eq_desc_cache_data error inject domain. Which ecc domain to inject the error.
        uint64_t eq_desc_cache_data_err_inj_enable  :  1; // eq_desc_cache_data error inject enable.
        uint64_t trig_op_cache_tag_err_inj_mask  :  8; // trig_op_cache_tag error inject mask
        uint64_t trig_op_cache_tag_err_inj_domain  :  1; // trig_op_cache_tag error inject domain. Which ecc domain to inject the error.
        uint64_t trig_op_cache_tag_err_inj_enable  :  1; // trig_op_cache_tag error inject enable.
        uint64_t trig_op_cache_data_err_inj_mask  :  8; // trig_op_cache_data error inject mask
        uint64_t trig_op_cache_data_err_inj_domain  :  4; // trig_op_cache_data error inject domain. Which ecc domain to inject the error.
        uint64_t trig_op_cache_data_err_inj_enable  :  1; // trig_op_cache_data error inject enable.
        uint64_t       Reserved_63_42  : 22; // Reserved
    } field;
    uint64_t val;
} RXET_CACHE_ERR_INJECT_SBE_MBE_t;

#endif /* ___FXR_rx_et_CSRS_H__ */