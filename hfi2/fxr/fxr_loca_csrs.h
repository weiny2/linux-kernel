// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Thu Jun  2 19:11:24 2016
//

#ifndef ___FXR_loca_CSRS_H__
#define ___FXR_loca_CSRS_H__

// LOCA_CFG_PIPE desc:
typedef union {
    struct {
        uint64_t   DEPIPE_FULL_ENABLE  :  1; // Stalls a request at PARB input until all prior requests have cleared the DCache Write stage (full pipe)
        uint64_t   DEPIPE_HALF_ENABLE  :  1; // Stalls a request at PARB input until all prior requests have cleared the DCache read stage
        uint64_t       IMICAM_ENTRIES  :  2; // IMI CAM Entries. Defines the max number of clock stages+2 that a conflicting request will be delayed in PARB due to an IMI RMW update of PCAM state.
        uint64_t    SNPFC_BURP_ENABLE  :  1; // Snoop Flow Control Burp Enable
        uint64_t         reserved_7_5  :  3; // Reserved
        uint64_t     PROQ_FULL_THRESH  : 10; // PROQ Full Threshold
        uint64_t       reserved_23_18  :  6; // Reserved
        uint64_t     XIN2_PLUS_THRESH  :  7; // Min number of entries in 2 or more L1TID banks required before flow control is asserted for new requests.
        uint64_t          reserved_31  :  1; // Reserved
        uint64_t         NEWFC_THRESH  :  9; // Min number of L1TID entries available before flow control is issued for new requests.
        uint64_t    NEWFC_BURP_THRESH  :  9; // Min number of L1TID entries available in burp mode before flow control is issued for new requests. Reflects the min number L1TIDs reserved for snoop requests.
        uint64_t         SNPFC_THRESH  :  9; // Min number of L1TID entries available before flow control is asserted for Snoop requests.
        uint64_t       reserved_63_59  :  5; // Reserved
    } field;
    uint64_t val;
} LOCA_CFG_PIPE_t;

// LOCA_CFG_PIPE2 desc:
typedef union {
    struct {
        uint64_t     CONQ_ENTRY_LIMIT  :  9; // Number of CONQ Entries allowed for use. Max number of 256, min number is Threshold +1
        uint64_t        reserved_11_9  :  3; // Reserved
        uint64_t       CONQ_FC_THRESH  :  8; // CONQ Flow control threshold. ConQ Flow Control is asserted when the number of available ConQ entries is equal to or less than the threshold limit.
        uint64_t      IRQ_SYNC_THRESH  :  5; // IRQ Sync Buffer threshold. Flow Control is asserted when the number IRQ requests in the Sync Buffer is greater than or equal to the threshold limit.
        uint64_t       reserved_31_25  :  7; // Reserved
        uint64_t        MRU_PEND_INIT  : 16; // MRU table Pending state Initialization. This 16 bit vector initializes the Pending state in the MRU Table at reset. By default, the Pending state is cleared at reset. Setting bits in this field will initialize the associated Cache Way to Pending which will de-feature the way from use.
        uint64_t       reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} LOCA_CFG_PIPE2_t;

// LOCA_CFG_PIPE3 desc:
typedef union {
    struct {
        uint64_t           reserved_0  :  1; // Reserved
        uint64_t   AGGRESSIVE_REFETCH  :  1; // Aggressive Refetch enable
        uint64_t  SO_PUT_OPTIMIZATION  :  1; // Strongly Ordered PUT optimization enable
        uint64_t        reserved_63_3  : 61; // Reserved
    } field;
    uint64_t val;
} LOCA_CFG_PIPE3_t;

// LOCA_CFG_PARB desc:
typedef union {
    struct {
        uint64_t    IMI_REQ_TO_THRESH  :  8; // IMI Request Forward Progress Timeout Threshold (clks)
        uint64_t   EMEC_REQ_TO_THRESH  :  8; // EMEC Request Forward Progress Timeout Threshold (clks)
        uint64_t   CONQ_REQ_TO_THRESH  :  8; // ConQ Request Forward Progress Timeout Threshold (clks)
        uint64_t   TRRP_REQ_TO_THRESH  :  8; // TRRP Request Forward Progress Timeout Threshold (clks)
        uint64_t     WAKE_HOLD_PERIOD  : 32; // Wake Hold Period (clks). Defines the number of 1.8Ghz clocks that LOCA will hold the WAKE signaling to Power Management after going idle. Dampens signaling so that WAKE is deasserted only after N consecutive clocks of being Idle. The default value of 1800 equates to a 1us period.
    } field;
    uint64_t val;
} LOCA_CFG_PARB_t;

// LOCA_CFG_IMI desc:
typedef union {
    struct {
        uint64_t          DPB_ARB_CTL  : 16; // DPB Arbitration Control
        uint64_t      IDI_C2U_ARB_CTL  : 16; // IDI C2U Request Arbitration Control
        uint64_t   DPB_SNP_BUF_THRESH  :  8; // DPB Snoop Buffer Threshold (clks)
        uint64_t     IMI_REQ_Q_THRESH  :  5; // IMI C2U Request Queue Full Threshold
        uint64_t       reserved_47_45  :  3; // Reserved
        uint64_t   IMI_EVICT_Q_THRESH  :  5; // IMI C2U Eviction Queue Full Threshold
        uint64_t       reserved_63_53  : 11; // Reserved
    } field;
    uint64_t val;
} LOCA_CFG_IMI_t;

// LOCA_CFG_IMI2 desc:
typedef union {
    struct {
        uint64_t  IDI_U2C_RSP_CREDITS  :  9; // IDI U2C Response Queue Credits
        uint64_t        reserved_15_9  :  7; // Reserved
        uint64_t IDI_U2C_DATA_CREDITS  :  9; // IDI U2C Data Queue Credits
        uint64_t       reserved_63_25  : 39; // Reserved
    } field;
    uint64_t val;
} LOCA_CFG_IMI2_t;

// LOCA_CFG_IMI3 desc:
typedef union {
    struct {
        uint64_t     NON_TEMP_RD_CURR  :  3; // Non-Temporal IDI control for RD_CURR request: x00-Default behavior001-Force LRUx10-No Change011-Force MRU1x1-CacheHint[0]==0 ? LRU : MRU
        uint64_t           reserved_3  :  1; // Reserved
        uint64_t    CACHE_FAR_RD_CURR  :  2; // Cache Far IDI control for RD_CURR request:00 - Force Cache Far to 001 - Force Cache Far to 110 - Cache Far = CacheHint[0]11 - Cache Far = CacheHint[1]
        uint64_t   CACHE_NEAR_RD_CURR  :  2; // Cache Near IDI control for RD_CURR request:00 - Force Cache Near to 001 - Force Cache Near to 110 - Cache Near = CacheHint[0]11 - Cache Near= CacheHint[1]
        uint64_t     NON_TEMP_DRD_OPT  :  3; // Non-Temporal IDI control for DRD_OPT request: See NON_TEMP_RD_CURR encodings
        uint64_t          reserved_11  :  1; // Reserved
        uint64_t    CACHE_FAR_DRD_OPT  :  2; // Cache Far IDI control for DRD_OPT request:See CACHE_FAR_RD_CURR encodings
        uint64_t   CACHE_NEAR_DRD_OPT  :  2; // Cache Near IDI control for DRD_OPT request:See CACHE_NEAR_RD_CURR encodings
        uint64_t      NON_TEMP_ITOMWR  :  3; // Non-Temporal IDI control for ITOMWR request: See NON_TEMP_RD_CURR encodings
        uint64_t          reserved_19  :  1; // Reserved
        uint64_t     CACHE_FAR_ITOMWR  :  2; // Cache Far IDI control for ITOMWR request:See CACHE_FAR_RD_CURR encodings
        uint64_t    CACHE_NEAR_ITOMWR  :  2; // Cache Near IDI control for ITOMWR request:See CACHE_NEAR_RD_CURR encodings
        uint64_t        NON_TEMP_ITOM  :  3; // Non-Temporal IDI control for ITOM request: See NON_TEMP_RD_CURR encodings
        uint64_t          reserved_27  :  1; // Reserved
        uint64_t       CACHE_FAR_ITOM  :  2; // Cache Far IDI control for ITOM request:See CACHE_FAR_RD_CURR encodings
        uint64_t      CACHE_NEAR_ITOM  :  2; // Cache Near IDI control for ITOM request:See CACHE_NEAR_RD_CURR encodings
        uint64_t    NON_TEMP_SPECITOM  :  3; // Non-Temporal IDI control for SPECITOM request: See NON_TEMP_RD_CURR encodings
        uint64_t          reserved_35  :  1; // Reserved
        uint64_t   CACHE_FAR_SPECITOM  :  2; // Cache Far IDI control for SPECITOM request:See CACHE_FAR_RD_CURR encodings
        uint64_t  CACHE_NEAR_SPECITOM  :  2; // Cache Near IDI control for SPECITOM request:See CACHE_NEAR_RD_CURR encodings
        uint64_t         NON_TEMP_RFO  :  3; // Non-Temporal IDI control for RFO request: See NON_TEMP_RD_CURR encodings
        uint64_t          reserved_43  :  1; // Reserved
        uint64_t        CACHE_FAR_RFO  :  2; // Cache Far IDI control for RFO request:See CACHE_FAR_RD_CURR encodings
        uint64_t       CACHE_NEAR_RFO  :  2; // Cache Near IDI control for RFO request:See CACHE_NEAR_RD_CURR encodings
        uint64_t      NON_TEMP_WBMTOI  :  2; // Non-Temporal IDI control for WBMTOI request: 00-Default behavior01-Force LRU10-No change11-Force MRU
        uint64_t       reserved_51_50  :  2; // Reserved
        uint64_t     CACHE_FAR_WBMTOI  :  1; // Cache Far IDI control for WBMTOI request:0-Force to 01-Force to 1
        uint64_t    CACHE_NEAR_WBMTOI  :  1; // Cache Near IDI control for WBMTOI request:0-Force to 01-Force to 1
        uint64_t       reserved_55_54  :  2; // Reserved
        uint64_t      IDI_ECC_DISABLE  :  1; // IDI ECC Disable. This is a defeature control for IDI ECC. When set, IDI ECC checking is disabled for all U2C Data responses.
        uint64_t       reserved_63_57  :  7; // Reserved
    } field;
    uint64_t val;
} LOCA_CFG_IMI3_t;

// LOCA_CFG_ITI desc:
typedef union {
    struct {
        uint64_t  IDI_U2C_REQ_CREDITS  :  9; // IDI U2C Request Queue Credits
        uint64_t        reserved_15_9  :  7; // Reserved
        uint64_t      UQB_STOP_THRESH  : 10; // IDI U2C Request Stop Threshold
        uint64_t       reserved_31_26  :  6; // Reserved
        uint64_t  IMI_SNP_RSP_CREDITS  :  9; // Snoop Response Buffer Credits ????
        uint64_t       reserved_47_41  :  7; // Reserved
        uint64_t  C2U_CSB_FULL_THRESH  :  5; // C2U Response Buffer Full Threshold
        uint64_t       reserved_63_53  : 11; // Reserved
    } field;
    uint64_t val;
} LOCA_CFG_ITI_t;

// LOCA_CFG_DALU desc:
typedef union {
    struct {
        uint64_t                   ie  :  1; // Invalid Operation Flag
        uint64_t                   de  :  1; // Denormal Flag
        uint64_t                   ze  :  1; // Divide by Zero Flag
        uint64_t                   oe  :  1; // Overflow Flag
        uint64_t                   ue  :  1; // Underflow Flag
        uint64_t                   pe  :  1; // Precision Flag
        uint64_t                  daz  :  1; // Denormals Are Zeros control
        uint64_t                   im  :  1; // Invalid Operation Mask
        uint64_t                   dm  :  1; // Denormal Operation Mask
        uint64_t                   zm  :  1; // Divide-by-Zero Mask
        uint64_t                   om  :  1; // Overflow Mask
        uint64_t                   um  :  1; // Underflow Mask
        uint64_t                   pm  :  1; // Precision Mask
        uint64_t                   rc  :  2; // Rounding Control
        uint64_t                   fz  :  1; // Flush to Zero Control
        uint64_t       reserved_63_16  : 48; // Reserved
    } field;
    uint64_t val;
} LOCA_CFG_DALU_t;

// LOCA_IDI_STRAP desc:
typedef union {
    struct {
        uint64_t          C2U_VERSION  :  5; // IDI Spec version supported by FXR
        uint64_t         reserved_7_5  :  3; // Reserved
        uint64_t             C2U_HASH  :  1; // The Hash field being set indicates that the agent implements the address hash and lookup tables. If this bit is cleared the Fabric should ignore any inputs on the C2U Request Hash and Topology fields
        uint64_t           C2U_PARITY  :  1; // The Parity bit being set indicates that the Agent will generate correct parity for all outgoing addresses and appropriate outgoing data. The Agent will also check any valid parity provided by the Fabric on address and data. If the bit is cleared, then no parity information should be consumed in the Fabric, nor does the Fabric need to generate parity for messages destined to the Agent. Parity bit must be set if ECC bit is set.
        uint64_t              C2U_ECC  :  1; // The ECC bit being set indicates that the Agent will generate correct ECC for appropriate outgoing data. The Agent will also check any valid ECC provided by the Fabric for read data. If the bit is cleared, then no ECC information should be consumed in the Fabric, nor does the Fabric need to generate ECC for data messages destined to the Agent.
        uint64_t            C2U_OWNED  :  1; // The Owned bit being set indicates that the Agent will support being placed into the Owned state by the Fabric. If the bit is cleared, then the Fabric should not return Owned state to the Agent.
        uint64_t       C2U_DATAHDRSEP  :  1; // The DataHdrSep bit being set indicates that the Agent expects a 3 cycle separation between the specified U2C Data fields and the rest of the U2C Data channel. If the bit is cleared, then all fields for a U2C Data packet should arrive in the same cycle.
        uint64_t            C2U_SPARE  :  6; // C2U Spare Straps for future use
        uint64_t       reserved_31_19  : 13; // Reserved
        uint64_t          U2C_VERSION  :  5; // IDI Spec version supported by Uncore (Fabric)
        uint64_t       reserved_39_37  :  3; // Reserved
        uint64_t       U2C_CLEANEVICT  :  1; // The CleanEvict bit being set indicates that the Fabric expects the agent to send CleanEvict messages to the Fabric. If the bit is cleared, then the Agent should not send CleanEvict messages to the Fabric.
        uint64_t           U2C_PARITY  :  1; // The Parity bit being set indicates that the Fabric will generate correct parity for all outgoing addresses and appropriate outgoing data. The Fabric will also check any valid parity provided by the Agent on address and data. If the bit is cleared, then no parity information should be consumed in the Agent, nor does the Agent need to generate parity for messages destined to the Fabric.
        uint64_t              U2C_ECC  :  1; // The ECC bit being set indicates that the Fabric will generate correct ECC for appropriate outgoing data. The Fabric will also check any valid ECC provided by the Agent for read data. If the bit is cleared, then no ECC information should be consumed in the Agent, nor does the Agent need to generate ECC for data messages destined to the Fabric.
        uint64_t            U2C_SPARE  :  8; // U2C Spare Straps for future use
        uint64_t       reserved_63_51  : 13; // Reserved
    } field;
    uint64_t val;
} LOCA_IDI_STRAP_t;

// LOCA_DBG_CACHE_CLN desc:
typedef union {
    struct {
        uint64_t              CC_ADDR  :  1; // Cache Clean by Address. Set by SW to initiate a CC Address operation. Cleared by HW when CC operation is complete. Cache line address is specified in ADDR field. NOTE: SW setting of this is ignored by HW if either bit[2] or bit[1] is set.
        uint64_t             CC_ENTRY  :  1; // Cache Clean by Entry. Set by SW to initiate a CC Entry operation. Cleared by HW when CC operation is complete. Entry identifier defined in ADDR field as:ADDR[8:0] - DCD Set indexADDR[12:9] - DCD Way (bank)NOTE: SW setting of this is ignored by HW if either bit[2] or bit[0] is set.
        uint64_t            HW_SEQ_CC  :  1; // Hardware Sequenced Cache Clean. HW sequence flow of all 8K DCD entries. Set by HW to initiate CC operation. Cleared by HW when CC operation is complete. ADDR field is ignored for this operation. NOTE: SW setting of this is ignored by HW if either bit[1] or bit[0] is set.
        uint64_t         reserved_5_3  :  3; // Reserved
        uint64_t                 ADDR  : 46; // Address. Physical cache line address or DCD entry ID for cache clean operation specified in bits [1:0]
        uint64_t       reserved_63_52  : 12; // Reserved
    } field;
    uint64_t val;
} LOCA_DBG_CACHE_CLN_t;

// LOCA_DBG_PROBE_ADDR desc:
typedef union {
    struct {
        uint64_t                   GO  :  1; // GO. When GO bit is set via SW write, HW initiates a probe operation. HW clears this bit when probe request has been serviced. Writing a 0 has no affect and will not initiate a probe operation.
        uint64_t         reserved_5_1  :  5; // Reserved
        uint64_t                 ADDR  : 46; // Address. Physical probe address
        uint64_t       reserved_63_52  : 12; // Reserved
    } field;
    uint64_t val;
} LOCA_DBG_PROBE_ADDR_t;

// LOCA_DBG_PROBE_DIR desc:
typedef union {
    struct {
        uint64_t                VALID  :  1; // Valid. Probe contents are valid. HW clears this bit when SW sets the LOCA_DBG_PROBE_ADDR .GO bit. HW sets this bit when loading the register with Probe data,
        uint64_t              DCD_HIT  :  1; // DCD HIT. Data Cache Hit
        uint64_t             PCAM_HIT  :  1; // PCAM HIT. Pending CAM Hit
        uint64_t           reserved_3  :  1; // Reserved
        uint64_t            DCD_MESIF  :  4; // DCD MESIF state. MESIF state encoding:4'h1: Shared4'h2: Exclusive4'h4: Invalid4'h8: Forward4'hF: Modified
        uint64_t           PCAM_STATE  :  4; // PCAM STATE:4'h0: Invalid4'h1: Allocated4'h2: Pending4'h3: ReceivedF4'h4: ReceivedS4'h5: ReceivedE4'h6: ReceivedE2M4'h7: ReceivedM4'h8: Writeback4'h9: WritebackCL4'hA: SnoopF2I4'hB: SnoopF2S4'hC: SnoopE2I4'hD: SnoopE2S4'hE: SnoopME2I4'hF: Error
        uint64_t        PCAM_PREFETCH  :  1; // PCAM Prefetch.Pending line is a prefetch for an ordered request.
        uint64_t           PCAM_CHINT  :  2; // PCAM Cache Hit. 00: NANT, 01: NAT, 10: ANT, 11: AT
        uint64_t             PCAM_WAY  :  4; // PCAM Wayness. Allocated way for pending request.
        uint64_t         PCAM_CONQVAL  :  1; // PCAM ConQ Valid. ConQ has a request that is linked to the PCAM entry. Head/Tail pointer data is valid
        uint64_t          PCAM_CONQHP  :  8; // PCAM ConQ Head Pointer
        uint64_t          PCAM_CONQTP  :  8; // PCAM ConQ Tail Pointer
        uint64_t       PCAM_PROQSRPLY  :  1; // PCAM PROQ Schedule Replay. Pending line will have its associated request replayed by the PROQ.
        uint64_t       PCAM_PROQEFILL  :  1; // PCAM PROQ Enabled Fill.Fill Pass for the pending line will be enabled by the PROQ.
        uint64_t          PCAM_PROQID  :  9; // PCAM PROQ ID. PROQ ID associated with pending line request.
        uint64_t          PCAM_SNPFWD  :  1; // PCAM Snoop Forward. Fill pass data must be forward to the DPB to complete the data transfer of a prior snoop request.
        uint64_t       reserved_63_48  : 16; // Reserved
    } field;
    uint64_t val;
} LOCA_DBG_PROBE_DIR_t;

// LOCA_DBG_PROBE_DATA desc:
typedef union {
    struct {
        uint64_t                 DATA  : 64; // DATA
    } field;
    uint64_t val;
} LOCA_DBG_PROBE_DATA_t;

// LOCA_HIFIS_CFG_CONTROL1 desc:
typedef union {
    struct {
        uint64_t           ARB_CNTL_B  : 16; // ARB_CNTL_B
        uint64_t           ARB_CNTL_A  : 16; // ARB_CNTL_A.
        uint64_t        RSPACK_THRESH  :  6; // Response ACK Threshold.Response buffer threshold setting for asserting flow control from HIFIS to PARB.
        uint64_t       reserved_62_38  : 25; // Reserved
        uint64_t      ECC_CORR_BYPASS  :  1; // ECC_CORR_BYPASS.
    } field;
    uint64_t val;
} LOCA_HIFIS_CFG_CONTROL1_t;

// LOCA0_ERR_STS desc:
typedef union {
    struct {
        uint64_t      hcc_idi_rsp_err  :  1; // IDI Response Error (Go-Err). See LOCA Error Info14 .
        uint64_t           u2c_req_pe  :  1; // U2C Request Parity Error
        uint64_t       u2c_rsp_opcode  :  1; // U2C Response Opcode invalid for given pending request
        uint64_t      u2c_rsp_invalid  :  1; // U2C Response is invalid
        uint64_t           u2c_rsp_go  :  1; // U2C Response GO state error
        uint64_t           u2c_rsp_pe  :  1; // U2C Response Parity Error
        uint64_t         dpb_snpq_err  :  1; // Data Pull Buffer Snoop Queue Error (over/under flow)
        uint64_t      imi_req_que_err  :  1; // IMI Request Queue Error (over/underflow)
        uint64_t           proq_g_mbe  :  1; // PROQ Global tail pointer queue MBE
        uint64_t           proq_f_mbe  :  1; // PROQ Flow tail pointer queue MBE
        uint64_t          proq_hl_mbe  :  1; // PROQ hold queue MBE
        uint64_t         conq_hdr_mbe  :  1; // ConQ header MBE
        uint64_t           iti_wp_mbe  :  1; // IDI Target WP (UQB) MBE
        uint64_t         u2c_data_mbe  :  1; // U2C Data MBE
        uint64_t          u2c_poi_mbe  :  1; // U2C Poison and ECC Valid bit queue MBE
        uint64_t          u2c_dir_mbe  :  1; // U2C L1DIR (detected in IMI) MBE
        uint64_t           u2c_wp_mbe  :  1; // U2C WritePull (USB) MBE
        uint64_t          c2u_eqb_mbe  :  1; // C2U Eviction Request (EQB) MBE
        uint64_t          c2u_nqb_mbe  :  1; // C2U New Request (NQB) MBE
        uint64_t          c2u_snp_mbe  :  1; // C2U DPB Snoop header MBE
        uint64_t           c2u_wp_mbe  :  1; // C2U DPB WritePull header MBE
        uint64_t         c2u_data_mbe  :  1; // C2U DPB Data MBE
        uint64_t      imi_orb_cnf_mbe  :  1; // IDI Master ORB CNF MBE
        uint64_t      imi_orb_rsp_mbe  :  1; // IDI Master ORB RSP MBE
        uint64_t      imi_orb_sch_mbe  :  1; // IDI Master ORB SCH MBE
        uint64_t        mru_table_mbe  :  1; // MRU Table MBE
        uint64_t             pcam_mbe  :  1; // PCAM data MBE
        uint64_t      hcc_dcd_tag_mbe  :  1; // HCC received MBE signaling with DCD tag data
        uint64_t       adm_cas_db_mbe  :  1; // ADM Compare & Swap Delay Buffer MBE
        uint64_t       adm_hdr_db_mbe  :  1; // ADM Header Delay Buffer MBE
        uint64_t       adm_paf_db_mbe  :  1; // ADM PAF Delay Buffer MBE
        uint64_t        adm_re_db_mbe  :  1; // ADM RE Delay Buffer MBE
        uint64_t        adm_sl_db_mbe  :  1; // ADM ShortLoop Delay Buffer MBE
        uint64_t           dcache_mbe  :  1; // DataCache MBE
        uint64_t          wrifill_mbe  :  1; // WriteFill Buffer MBE
        uint64_t   hcc_rpl_delay_perr  :  1; // HCC Replay Delay RF Parity Error
        uint64_t           proq_g_sbe  :  1; // PROQ global tail pointer queue SBE
        uint64_t           proq_f_sbe  :  1; // PROQ Flow tail pointer queue SBE
        uint64_t          proq_hl_sbe  :  1; // PROQ hold queue SBE
        uint64_t         conq_hdr_sbe  :  1; // ConQ header SBE
        uint64_t           iti_wp_sbe  :  1; // IDI Target WP (UQB) SBE
        uint64_t         u2c_data_sbe  :  1; // U2C Data SBE
        uint64_t          u2c_poi_sbe  :  1; // U2C Poison and ECC valid bit queue SBE
        uint64_t          u2c_dir_sbe  :  1; // U2C L1DIR (detected in IMI) SBE
        uint64_t           u2c_wp_sbe  :  1; // U2C WritePull (USB) SBE
        uint64_t          c2u_eqb_sbe  :  1; // C2U Eviction Request (EQB) SBE
        uint64_t          c2u_nqb_sbe  :  1; // C2U New Request (NQB) SBE
        uint64_t          c2u_snp_sbe  :  1; // C2U DPB Snoop header SBE
        uint64_t           c2u_wp_sbe  :  1; // C2U DPB WritePull header SBE
        uint64_t         c2u_data_sbe  :  1; // C2U DPB Data SBE
        uint64_t      imi_orb_cnf_sbe  :  1; // IDI Master ORB CNF SBE
        uint64_t      imi_orb_rsp_sbe  :  1; // IDI Master ORB RSP SBE
        uint64_t      imi_orb_sch_sbe  :  1; // IDI Master ORB SCH SBE
        uint64_t        mru_table_sbe  :  1; // MRU Table SBE
        uint64_t             pcam_sbe  :  1; // PCAM data SBE
        uint64_t       hcc_replay_err  :  1; // HCC Replay state consistency error. See hcc_replay_status
        uint64_t       adm_cas_db_sbe  :  1; // ADM Compare & Swap Delay Buffer SBE
        uint64_t       adm_hdr_db_sbe  :  1; // ADM Header Delay Buffer SBE
        uint64_t       adm_paf_db_sbe  :  1; // ADM PAF Delay Buffer SBE
        uint64_t        adm_re_db_sbe  :  1; // ADM RE Delay Buffer SBE
        uint64_t        adm_sl_db_sbe  :  1; // ADM ShortLoop Delay Buffer SBE
        uint64_t           dcache_sbe  :  1; // DataCache SBE
        uint64_t          wrifill_sbe  :  1; // WriteFill Buffer SBE
        uint64_t          reserved_63  :  1; // Reserved
    } field;
    uint64_t val;
} LOCA0_ERR_STS_t;

// LOCA0_ERR_CLR desc:
typedef union {
    struct {
        uint64_t               events  : 64; // Write 1's to clear corresponding LOCA0_ERR_STS bits.
    } field;
    uint64_t val;
} LOCA0_ERR_CLR_t;

// LOCA0_ERR_FRC desc:
typedef union {
    struct {
        uint64_t               events  : 64; // Write 1 to set corresponding LOCA0_ERR_STS bits.
    } field;
    uint64_t val;
} LOCA0_ERR_FRC_t;

// LOCA0_ERR_EN_HOST desc:
typedef union {
    struct {
        uint64_t               events  : 64; // Enables corresponding LOCA0_ERR_STS bits to generate host interrupt signal.
    } field;
    uint64_t val;
} LOCA0_ERR_EN_HOST_t;

// LOCA0_ERR_FIRST_HOST desc:
typedef union {
    struct {
        uint64_t               events  : 64; // Snapshot of LOCA0_ERR_STS bits when host interrupt signal transitions from 0 to 1.
    } field;
    uint64_t val;
} LOCA0_ERR_FIRST_HOST_t;

// LOCA0_ERR_EN_BMC desc:
typedef union {
    struct {
        uint64_t               events  : 64; // Enable corresponding LOCA0_ERR_STS bits to generate BMC interrupt signal.
    } field;
    uint64_t val;
} LOCA0_ERR_EN_BMC_t;

// LOCA0_ERR_FIRST_BMC desc:
typedef union {
    struct {
        uint64_t               events  : 64; // Snapshot of LOCA0_ERR_STS bits when BMC interrupt signal transitions from 0 to 1.
    } field;
    uint64_t val;
} LOCA0_ERR_FIRST_BMC_t;

// LOCA0_ERR_EN_QUAR desc:
typedef union {
    struct {
        uint64_t               events  : 64; // Enable corresponding LOCA0_ERR_STS bits to generate quarantine signal.
    } field;
    uint64_t val;
} LOCA0_ERR_EN_QUAR_t;

// LOCA0_ERR_FIRST_QUAR desc:
typedef union {
    struct {
        uint64_t               events  : 64; // Snapshot of LOCA0_ERR_STS bits when quarantine signal transitions from 0 to 1.
    } field;
    uint64_t val;
} LOCA0_ERR_FIRST_QUAR_t;

// LOCA0_VIRAL_ENABLE desc:
typedef union {
    struct {
        uint64_t         enabled_errs  : 64; // Enable corresponding LOCA0_ERR_STS bits to generate viral signaling.
    } field;
    uint64_t val;
} LOCA0_VIRAL_ENABLE_t;

// LOCA_ERR_INFO desc:
typedef union {
    struct {
        uint64_t         proq_hl_synd  :  5; // PROQ Hold syndrome of proq_hl_mbe / proq_hl_sbe error
        uint64_t        proq_tf3_synd  :  5; // PROQ Flow Tail3 syndrome of proq_f_mbe / proq_f_sbe error
        uint64_t        proq_tf2_synd  :  5; // PROQ Flow Tail2 syndrome of proq_f_mbe / proq_f_sbe error
        uint64_t        proq_tf1_synd  :  5; // PROQ Flow Tail1 syndrome of proq_f_mbe / proq_f_sbe error
        uint64_t        proq_tf0_synd  :  5; // PROQ Flow Tail0 syndrome of proq_f_mbe / proq_f_sbe error
        uint64_t         proq_tg_synd  :  5; // PROQ Global Tail syndrome of proq_g_mbe / proq_g_sbe error
        uint64_t       reserved_31_30  :  2; // reserved
        uint64_t        conq_hdr_synd  :  8; // syndrome of the last conq_hdr_mbe / conq_hdr_sbe error
        uint64_t         adm_cas_synd  :  8; // ADM CAS Delay buffer syndrome of adm_cas_db_mbe / adm_cas_db_sbe error
        uint64_t         adm_hdr_synd  :  8; // ADM Header Delay buffer syndrome of adm_hdr_db_mbe / adm_hdr_db_sbe error
        uint64_t          proq_tf_sbe  :  4; // PROQ flow group SBE associated with proq_f_sbe
        uint64_t          proq_tf_mbe  :  4; // PROQ flow group MBE associated with proq_f_mbe
    } field;
    uint64_t val;
} LOCA_ERR_INFO_t;

// LOCA_ERR_INFO1 desc:
typedef union {
    struct {
        uint64_t        c2u_data_synd  : 64; // C2U DPB syndrome of c2u_data_mbe / c2u_data_sbe error
    } field;
    uint64_t val;
} LOCA_ERR_INFO1_t;

// LOCA_ERR_INFO2 desc:
typedef union {
    struct {
        uint64_t             nqb_synd  :  8; // C2U New request syndrome of c2u_nqb_mbe / c2u_nqb_sbe error
        uint64_t             eqb_synd  :  8; // C2U Eviction request syndrome of c2u_eqb_mbe / c2u_eqb_sbe error
        uint64_t          c2u_wp_synd  :  6; // C2U DBP WritePull header syndrome of c2u_wp_mbe / c2u_wp_sbe error
        uint64_t       reserved_23_22  :  2; // reserved
        uint64_t         c2u_snp_synd  :  6; // C2U DBP Snoop header syndrome of c2u_snp_mbe / c2u_snp_sbe error
        uint64_t       reserved_31_30  :  2; // reserved
        uint64_t         orb_sch_synd  :  8; // IMI ORB Scheduler syndrome of imi_orb_sch_mbe / imi_orb_sch_sbe error
        uint64_t         orb_rsp_synd  :  8; // IMI ORB Response syndrome of imi_orb_rsp_mbe / imi_orb_rsp_sbe error
        uint64_t         orb_cnf_synd  :  8; // IMI ORB CNF syndrome of imi_orb_cnf_mbe / imi_orb_cnf_sbe error
        uint64_t          iti_wp_synd  :  8; // ITI Request (UQB) syndrome of iti_wp_mbe / iti_wp_sbe error
    } field;
    uint64_t val;
} LOCA_ERR_INFO2_t;

// LOCA_ERR_INFO3 desc:
typedef union {
    struct {
        uint64_t   u2c_poi_val_synd00  :  5; // U2C Poison/ECCvalid syndrome of u2c_poi_mbe / u2c_poi_sbe error
        uint64_t   u2c_poi_val_synd01  :  5; // U2C Poison/ECCvalid syndrome of u2c_poi_mbe / u2c_poi_sbe error
        uint64_t   u2c_poi_val_synd10  :  5; // U2C Poison/ECCvalid syndrome of u2c_poi_mbe / u2c_poi_sbe error
        uint64_t   u2c_poi_val_synd11  :  5; // U2C Poison/ECCvalid syndrome of u2c_poi_mbe / u2c_poi_sbe error
        uint64_t       u2c_data0_synd  : 10; // U2C Data0 syndrome of u2c_data_mbe / u2c_data_sbe error
        uint64_t       u2c_data1_synd  : 10; // U2C Data1 syndrome of u2c_data_mbe / u2c_data_sbe error
        uint64_t          u2c_wp_synd  :  6; // U2C WritePull syndrome of u2c_wp_mbe / u2c_wp_sbe error
        uint64_t         u2c_dir_synd  :  7; // U2C L1Dir syndrome of u2c_dir_mbe / u2c_dir_sbe error
        uint64_t       reserved_55_53  :  3; // reserved
        uint64_t       mru_table_synd  :  8; // MRU Table syndrome of mru_table_mbe / mru_table_sbe error
    } field;
    uint64_t val;
} LOCA_ERR_INFO3_t;

// LOCA_ERR_INFO4 desc:
typedef union {
    struct {
        uint64_t       dcd_bank0_synd  : 12; // DCD bank0 syndrome of dcd_bank_mbe / dcd_bank_sbe error
        uint64_t       dcd_bank1_synd  : 12; // DCD bank1 syndrome of dcd_bank_mbe / dcd_bank_sbe error
        uint64_t       dcd_bank2_synd  : 12; // DCD bank2 syndrome of dcd_bank_mbe / dcd_bank_sbe error
        uint64_t       dcd_bank3_synd  : 12; // DCD bank3 syndrome of dcd_bank_mbe / dcd_bank_sbe error
        uint64_t       dcd_bank4_synd  : 12; // DCD bank4 syndrome of dcd_bank_mbe / dcd_bank_sbe error
        uint64_t       reserved_63_60  :  4; // reserved
    } field;
    uint64_t val;
} LOCA_ERR_INFO4_t;

// LOCA_ERR_INFO5 desc:
typedef union {
    struct {
        uint64_t       dcd_bank5_synd  : 12; // DCD bank5syndrome of dcd_bank_mbe / dcd_bank_sbe error
        uint64_t       dcd_bank6_synd  : 12; // DCD bank6 syndrome of dcd_bank_mbe / dcd_bank_sbe error
        uint64_t       dcd_bank7_synd  : 12; // DCD bank7 syndrome of dcd_bank_mbe / dcd_bank_sbe error
        uint64_t       dcd_bank8_synd  : 12; // DCD bank8 syndrome of dcd_bank_mbe / dcd_bank_sbe error
        uint64_t       dcd_bank9_synd  : 12; // DCD bank9 syndrome of dcd_bank_mbe / dcd_bank_sbe error
        uint64_t       reserved_63_60  :  4; // reserved
    } field;
    uint64_t val;
} LOCA_ERR_INFO5_t;

// LOCA_ERR_INFO6 desc:
typedef union {
    struct {
        uint64_t         adm_paf_synd  : 64; // ADM PAF syndrome of adm_paf_db_mbe / adm_paf_db_sbe error
    } field;
    uint64_t val;
} LOCA_ERR_INFO6_t;

// LOCA_ERR_INFO7 desc:
typedef union {
    struct {
        uint64_t          adm_re_synd  : 64; // ADM RE syndrome of adm_re_db_mbe / adm_re_db_sbe error
    } field;
    uint64_t val;
} LOCA_ERR_INFO7_t;

// LOCA_ERR_INFO8 desc:
typedef union {
    struct {
        uint64_t        adm_sldb_synd  : 64; // ADM SLDB syndrome of adm_sl_db_mbe / adm_sl_db_sbe error
    } field;
    uint64_t val;
} LOCA_ERR_INFO8_t;

// LOCA_ERR_INFO9 desc:
typedef union {
    struct {
        uint64_t            imi_cmpid  :  8; // IMI Completion ID (TID) associated with hcc_rpl_delay_perr error
        uint64_t        Reserved_63_8  : 56; // Reserved
    } field;
    uint64_t val;
} LOCA_ERR_INFO9_t;

// LOCA_ERR_INFO10 desc:
typedef union {
    struct {
        uint64_t          dcache_synd  : 64; // Data Cache syndrome of dcache_mbe / dcache_sbe error
    } field;
    uint64_t val;
} LOCA_ERR_INFO10_t;

// LOCA_ERR_INFO11 desc:
typedef union {
    struct {
        uint64_t         wrifill_synd  : 64; // Write-Fill Data syndrome of wrifill_mbe / wrifill_sbe error
    } field;
    uint64_t val;
} LOCA_ERR_INFO11_t;

// LOCA_ERR_INFO12 desc:
typedef union {
    struct {
        uint64_t      u2c_rsp_rspdata  : 13; // U2C Response Error: rspdata
        uint64_t       reserved_15_13  :  3; // reserved
        uint64_t         u2c_rsp_l1id  :  8; // U2C Response Error: L1ID
        uint64_t       u2c_rsp_opcode  :  4; // U2C Response Error: opcode
        uint64_t      u2c_req_reqdata  : 16; // U2C Request Error: reqdata
        uint64_t      u2c_req_addrpar  :  1; // U2C Request Error: addrpar
        uint64_t       u2c_req_opcode  :  5; // U2C Request Error: opcode
        uint64_t       pcam_data_synd  :  7; // PCAM data syndrome of pcam_mbe / pcam_sbe error
        uint64_t       reserved_63_57  :  7; // reserved
    } field;
    uint64_t val;
} LOCA_ERR_INFO12_t;

// LOCA_ERR_INFO13 desc:
typedef union {
    struct {
        uint64_t         reserved_2_0  :  3; // reserved
        uint64_t         u2c_req_addr  : 49; // U2C Request Error: address
        uint64_t       reserved_55_52  :  4; // reserved
        uint64_t    hcc_replay_status  :  8; // HCC Replay error Status. Valid when hcc_replay_err is set[7]: Invalid PCAM state[6]: Unexpected PCAM state[5]: PCAM.PENDING state not expected with Address Replay [4]: Replay request pointer doesn't match PCAM.ConQ_HP[3]: Spare[2]: PCAM state not consistent with PROQ Replay[1]: Invalid PCAM ConQ pointer[0]: PCAM Miss
    } field;
    uint64_t val;
} LOCA_ERR_INFO13_t;

// LOCA_ERR_INFO14 desc:
typedef union {
    struct {
        uint64_t         delayed_reqs  :  1; // One or more Delayed Requests in ConQ where also dropped as a result of this error (same address)
        uint64_t         reserved_5_1  :  5; // reserved
        uint64_t              address  : 46; // IDI Response Error Address[51:6]
        uint64_t                  tid  : 12; // HFI Tid associated with IDI Rsp Error. hcc_idi_rsp_err
    } field;
    uint64_t val;
} LOCA_ERR_INFO14_t;

// LOCA_ERR_INFO15 desc:
typedef union {
    struct {
        uint64_t       hifis_hdr_synd  :  8; // HIFIS Header Syndrome of hifis_hdr_mbe / hifis_hdr_sbe
        uint64_t       conq_tail_synd  :  5; // ConQ Tail Pointer syndrome of conq_tail_mbe / conq_tail_sbe
        uint64_t       reserved_15_13  :  3; // Reserved
        uint64_t      dcd_bank15_synd  : 12; // DCD bank15 syndrome of dcd_bank_mbe / dcd_bank_sbe error
        uint64_t     pmon_rollover_id  :  7; // PMON counter ID of pmon_rollover event
        uint64_t       reserved_63_35  : 29; // Reserved
    } field;
    uint64_t val;
} LOCA_ERR_INFO15_t;

// LOCA_ERR_INFO16 desc:
typedef union {
    struct {
        uint64_t      dcd_bank10_synd  : 12; // DCD bank10 syndrome of dcd_bank_mbe / dcd_bank_sbe error
        uint64_t      dcd_bank11_synd  : 12; // DCD bank11 syndrome of dcd_bank_mbe / dcd_bank_sbe error
        uint64_t      dcd_bank12_synd  : 12; // DCD bank12 syndrome of dcd_bank_mbe / dcd_bank_sbe error
        uint64_t      dcd_bank13_synd  : 12; // DCD bank13 syndrome of dcd_bank_mbe / dcd_bank_sbe error
        uint64_t      dcd_bank14_synd  : 12; // DCD bank14 syndrome of dcd_bank_mbe / dcd_bank_sbe error
        uint64_t       reserved_63_60  :  4; // reserved
    } field;
    uint64_t val;
} LOCA_ERR_INFO16_t;

// LOCA1_ERR_STS desc:
typedef union {
    struct {
        uint64_t         dcd_bank_mbe  : 16; // DCD Bank level MBE
        uint64_t         dcd_bank_sbe  : 16; // DCD Bank level SBE
        uint64_t        conq_tail_mbe  :  1; // ConQ Tail pointer MBE. See conq_tail_synd
        uint64_t        hifis_hdr_mbe  :  1; // HIFIS Header MBE. See hifis_hdr_synd
        uint64_t        conq_tail_sbe  :  1; // ConQ Tail pointer SBE. See conq_tail_synd
        uint64_t        hifis_hdr_sbe  :  1; // HIFIS Header SBE. See hifis_hdr_synd
        uint64_t        pmon_rollover  :  1; // Performance Monitor Rollover Event.
        uint64_t     adm_malform_blen  :  1; // Atomic operation with misaligned BLEN/ADT
        uint64_t     adm_malform_addr  :  1; // Atomic operation with misaligned ADDR/ADT
        uint64_t      adm_malform_adt  :  1; // Atomic operation with invalid ADT
        uint64_t      adm_malform_aso  :  1; // Atomic operation with invalid ASOP
        uint64_t       reserved_63_41  : 23; // reserved
    } field;
    uint64_t val;
} LOCA1_ERR_STS_t;

// LOCA1_ERR_CLR desc:
typedef union {
    struct {
        uint64_t         dcd_bank_mbe  : 16; // Write 1's to clear corresponding LOCA1_ERR_STS status bits.
        uint64_t         dcd_bank_sbe  : 16; // Write 1's to clear corresponding LOCA1_ERR_STS status bits.
        uint64_t        conq_tail_mbe  :  1; // Write 1's to clear corresponding LOCA1_ERR_STS status bits.
        uint64_t        hifis_hdr_mbe  :  1; // Write 1's to clear corresponding LOCA1_ERR_STS status bits.
        uint64_t        conq_tail_sbe  :  1; // Write 1's to clear corresponding LOCA1_ERR_STS status bits.
        uint64_t        hifis_hdr_sbe  :  1; // Write 1's to clear corresponding LOCA1_ERR_STS status bits.
        uint64_t        pmon_rollover  :  1; // Write 1's to clear corresponding LOCA1_ERR_STS status bits.
        uint64_t     adm_malform_blen  :  1; // Write 1's to clear corresponding LOCA1_ERR_STS status bits.
        uint64_t     adm_malform_addr  :  1; // Write 1's to clear corresponding LOCA1_ERR_STS status bits.
        uint64_t      adm_malform_adt  :  1; // Write 1's to clear corresponding LOCA1_ERR_STS status bits.
        uint64_t      adm_malform_aso  :  1; // Write 1's to clear corresponding LOCA1_ERR_STS status bits.
        uint64_t       reserved_63_41  : 23; // reserved.
    } field;
    uint64_t val;
} LOCA1_ERR_CLR_t;

// LOCA1_ERR_FRC desc:
typedef union {
    struct {
        uint64_t         dcd_bank_mbe  : 16; // Write 1 to set corresponding LOCA1_ERR_STS status bits.
        uint64_t         dcd_bank_sbe  : 16; // Write 1 to set corresponding LOCA1_ERR_STS status bits.
        uint64_t        conq_tail_mbe  :  1; // Write 1 to set corresponding LOCA1_ERR_STS status bits.
        uint64_t        hifis_hdr_mbe  :  1; // Write 1 to set corresponding LOCA1_ERR_STS status bits.
        uint64_t        conq_tail_sbe  :  1; // Write 1 to set corresponding LOCA1_ERR_STS status bits.
        uint64_t        hifis_hdr_sbe  :  1; // Write 1 to set corresponding LOCA1_ERR_STS status bits.
        uint64_t        pmon_rollover  :  1; // Write 1 to set corresponding LOCA1_ERR_STS status bits.
        uint64_t     adm_malform_blen  :  1; // Write 1 to set corresponding LOCA1_ERR_STS status bits.
        uint64_t     adm_malform_addr  :  1; // Write 1 to set corresponding LOCA1_ERR_STS status bits.
        uint64_t      adm_malform_adt  :  1; // Write 1 to set corresponding LOCA1_ERR_STS status bits.
        uint64_t      adm_malform_aso  :  1; // Write 1 to set corresponding LOCA1_ERR_STS status bits.
        uint64_t       reserved_63_41  : 23; // reserved.
    } field;
    uint64_t val;
} LOCA1_ERR_FRC_t;

// LOCA1_ERR_EN_HOST desc:
typedef union {
    struct {
        uint64_t         dcd_bank_mbe  : 16; // Enables corresponding LOCA1_ERR_STS bits to generate host interrupt signal.
        uint64_t         dcd_bank_sbe  : 16; // Enables corresponding LOCA1_ERR_STS bits to generate host interrupt signal.
        uint64_t        conq_tail_mbe  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate host interrupt signal.
        uint64_t        hifis_hdr_mbe  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate host interrupt signal.
        uint64_t        conq_tail_sbe  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate host interrupt signal.
        uint64_t        hifis_hdr_sbe  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate host interrupt signal.
        uint64_t        pmon_rollover  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate host interrupt signal.
        uint64_t     adm_malform_blen  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate host interrupt signal.
        uint64_t     adm_malform_addr  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate host interrupt signal.
        uint64_t      adm_malform_adt  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate host interrupt signal.
        uint64_t      adm_malform_aso  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate host interrupt signal.
        uint64_t       reserved_63_41  : 23; // reserved.
    } field;
    uint64_t val;
} LOCA1_ERR_EN_HOST_t;

// LOCA1_ERR_FIRST_HOST desc:
typedef union {
    struct {
        uint64_t         dcd_bank_mbe  : 16; // Snapshot of LOCA1_ERR_STS bits when host interrupt signal transitions from 0 to 1.
        uint64_t         dcd_bank_sbe  : 16; // Snapshot of LOCA1_ERR_STS bits when host interrupt signal transitions from 0 to 1.
        uint64_t        conq_tail_mbe  :  1; // Snapshot of LOCA1_ERR_STS bits when host interrupt signal transitions from 0 to 1.
        uint64_t        hifis_hdr_mbe  :  1; // Snapshot of LOCA1_ERR_STS bits when host interrupt signal transitions from 0 to 1.
        uint64_t        conq_tail_sbe  :  1; // Snapshot of LOCA1_ERR_STS bits when host interrupt signal transitions from 0 to 1.
        uint64_t        hifis_hdr_sbe  :  1; // Snapshot of LOCA1_ERR_STS bits when host interrupt signal transitions from 0 to 1.
        uint64_t        pmon_rollover  :  1; // Snapshot of LOCA1_ERR_STS bits when host interrupt signal transitions from 0 to 1.
        uint64_t     adm_malform_blen  :  1; // Snapshot of LOCA1_ERR_STS bits when host interrupt signal transitions from 0 to 1.
        uint64_t     adm_malform_addr  :  1; // Snapshot of LOCA1_ERR_STS bits when host interrupt signal transitions from 0 to 1.
        uint64_t      adm_malform_adt  :  1; // Snapshot of LOCA1_ERR_STS bits when host interrupt signal transitions from 0 to 1.
        uint64_t      adm_malform_aso  :  1; // Snapshot of LOCA1_ERR_STS bits when host interrupt signal transitions from 0 to 1.
        uint64_t       reserved_63_41  : 23; // reserved.
    } field;
    uint64_t val;
} LOCA1_ERR_FIRST_HOST_t;

// LOCA1_ERR_EN_BMC desc:
typedef union {
    struct {
        uint64_t         dcd_bank_mbe  : 16; // Enables corresponding LOCA1_ERR_STS bits to generate BMC interrupt signal.
        uint64_t         dcd_bank_sbe  : 16; // Enables corresponding LOCA1_ERR_STS bits to generate BMC interrupt signal.
        uint64_t        conq_tail_mbe  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate BMC interrupt signal.
        uint64_t        hifis_hdr_mbe  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate BMC interrupt signal.
        uint64_t        conq_tail_sbe  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate BMC interrupt signal.
        uint64_t        hifis_hdr_sbe  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate BMC interrupt signal.
        uint64_t        pmon_rollover  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate BMC interrupt signal.
        uint64_t     adm_malform_blen  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate BMC interrupt signal.
        uint64_t     adm_malform_addr  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate BMC interrupt signal.
        uint64_t      adm_malform_adt  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate BMC interrupt signal.
        uint64_t      adm_malform_aso  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate BMC interrupt signal.
        uint64_t       reserved_63_41  : 23; // reserved.
    } field;
    uint64_t val;
} LOCA1_ERR_EN_BMC_t;

// LOCA1_ERR_FIRST_BMC desc:
typedef union {
    struct {
        uint64_t         dcd_bank_mbe  : 16; // Snapshot of LOCA1_ERR_STS bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t         dcd_bank_sbe  : 16; // Snapshot of LOCA1_ERR_STS bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t        conq_tail_mbe  :  1; // Snapshot of LOCA1_ERR_STS bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t        hifis_hdr_mbe  :  1; // Snapshot of LOCA1_ERR_STS bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t        conq_tail_sbe  :  1; // Snapshot of LOCA1_ERR_STS bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t        hifis_hdr_sbe  :  1; // Snapshot of LOCA1_ERR_STS bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t        pmon_rollover  :  1; // Snapshot of LOCA1_ERR_STS bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t     adm_malform_blen  :  1; // Snapshot of LOCA1_ERR_STS bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t     adm_malform_addr  :  1; // Snapshot of LOCA1_ERR_STS bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t      adm_malform_adt  :  1; // Snapshot of LOCA1_ERR_STS bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t      adm_malform_aso  :  1; // Snapshot of LOCA1_ERR_STS bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t       reserved_63_41  : 23; // reserved.
    } field;
    uint64_t val;
} LOCA1_ERR_FIRST_BMC_t;

// LOCA1_ERR_EN_QUAR desc:
typedef union {
    struct {
        uint64_t         dcd_bank_mbe  : 16; // Enables corresponding LOCA1_ERR_STS bits to generate quarantine interrupt signal.
        uint64_t         dcd_bank_sbe  : 16; // Enables corresponding LOCA1_ERR_STS bits to generate quarantine interrupt signal.
        uint64_t        conq_tail_mbe  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate quarantine interrupt signal.
        uint64_t        hifis_hdr_mbe  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate quarantine interrupt signal.
        uint64_t        conq_tail_sbe  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate quarantine interrupt signal.
        uint64_t        hifis_hdr_sbe  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate quarantine interrupt signal.
        uint64_t        pmon_rollover  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate quarantine interrupt signal.
        uint64_t     adm_malform_blen  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate quarantine interrupt signal.
        uint64_t     adm_malform_addr  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate quarantine interrupt signal.
        uint64_t      adm_malform_adt  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate quarantine interrupt signal.
        uint64_t      adm_malform_aso  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate quarantine interrupt signal.
        uint64_t       reserved_63_41  : 23; // reserved.
    } field;
    uint64_t val;
} LOCA1_ERR_EN_QUAR_t;

// LOCA1_ERR_FIRST_QUAR desc:
typedef union {
    struct {
        uint64_t         dcd_bank_mbe  : 16; // Snapshot of LOCA1_ERR_STS bits when quarantine interrupt signal transitions from 0 to 1.
        uint64_t         dcd_bank_sbe  : 16; // Snapshot of LOCA1_ERR_STS bits when quarantine interrupt signal transitions from 0 to 1.
        uint64_t        conq_tail_mbe  :  1; // Snapshot of LOCA1_ERR_STS bits when quarantine interrupt signal transitions from 0 to 1.
        uint64_t        hifis_hdr_mbe  :  1; // Snapshot of LOCA1_ERR_STS bits when quarantine interrupt signal transitions from 0 to 1.
        uint64_t        conq_tail_sbe  :  1; // Snapshot of LOCA1_ERR_STS bits when quarantine interrupt signal transitions from 0 to 1.
        uint64_t        hifis_hdr_sbe  :  1; // Snapshot of LOCA1_ERR_STS bits when quarantine interrupt signal transitions from 0 to 1.
        uint64_t        pmon_rollover  :  1; // Snapshot of LOCA1_ERR_STS bits when quarantine interrupt signal transitions from 0 to 1.
        uint64_t     adm_malform_blen  :  1; // Snapshot of LOCA1_ERR_STS bits when quarantine interrupt signal transitions from 0 to 1.
        uint64_t     adm_malform_addr  :  1; // Snapshot of LOCA1_ERR_STS bits when quarantine interrupt signal transitions from 0 to 1.
        uint64_t      adm_malform_adt  :  1; // Snapshot of LOCA1_ERR_STS bits when quarantine interrupt signal transitions from 0 to 1.
        uint64_t      adm_malform_aso  :  1; // Snapshot of LOCA1_ERR_STS bits when quarantine interrupt signal transitions from 0 to 1.
        uint64_t       reserved_63_41  : 23; // reserved.
    } field;
    uint64_t val;
} LOCA1_ERR_FIRST_QUAR_t;

// LOCA1_VIRAL_ENABLE desc:
typedef union {
    struct {
        uint64_t         dcd_bank_mbe  : 16; // Enable corresponding LOCA0_ERR_STS bits to generate viral signaling.
        uint64_t         dcd_bank_sbe  : 16; // Enable corresponding LOCA0_ERR_STS bits to generate viral signaling.
        uint64_t        conq_tail_mbe  :  1; // Enable corresponding LOCA0_ERR_STS bits to generate viral signaling.
        uint64_t        hifis_hdr_mbe  :  1; // Enable corresponding LOCA0_ERR_STS bits to generate viral signaling.
        uint64_t        conq_tail_sbe  :  1; // Enable corresponding LOCA0_ERR_STS bits to generate viral signaling.
        uint64_t        hifis_hdr_sbe  :  1; // Enable corresponding LOCA0_ERR_STS bits to generate viral signaling.
        uint64_t        pmon_rollover  :  1; // Enable corresponding LOCA0_ERR_STS bits to generate viral signaling.
        uint64_t     adm_malform_blen  :  1; // Enable corresponding LOCA0_ERR_STS bits to generate viral signaling.
        uint64_t     adm_malform_addr  :  1; // Enable corresponding LOCA0_ERR_STS bits to generate viral signaling.
        uint64_t      adm_malform_adt  :  1; // Enable corresponding LOCA0_ERR_STS bits to generate viral signaling.
        uint64_t      adm_malform_aso  :  1; // Enable corresponding LOCA0_ERR_STS bits to generate viral signaling.
        uint64_t       reserved_63_41  : 23; // reserved.
    } field;
    uint64_t val;
} LOCA1_VIRAL_ENABLE_t;

// LOCA_PMON_CFG desc:
typedef union {
    struct {
        uint64_t           at_cnt_ena  :  1; // When set, enables all counters associated AT sourced New Request events in the Pipeline Traffic Profiles.
        uint64_t           tx_cnt_ena  :  1; // When set, enables all counters associated Tx sourced New Request events in the Pipeline Traffic Profiles.
        uint64_t           rx_cnt_ena  :  1; // When set, enables all counters associated Rx sourced New Request events in the Pipeline Traffic Profiles.
        uint64_t     alloc_opcode_ena  :  1; // When set, enables GET/PUT/ATOMIC opcode DCache Hit counters for requests with allocating hints only. Otherwise all request types are counted.
        uint64_t        reserved_15_4  : 12; // reserved.
        uint64_t           fid_cnt_id  :  6; // This field defines the GID/FID used with the pm_fid_dc_hit / pm_fid_dc_req counters and the pm_new_fid_req counter. When MSB is set, requests with Global ID are counted. Otherwise, the lower 5 bits of the field define the Flow ID of counted requests.
        uint64_t       reserved_23_22  :  2; // reserved.
        uint64_t          fid_cnt_msk  :  6; // This field defines the mask that gets applied to the fid_cnt_id field below for use with associated PMON counting events. Mask bits with a value of 1 treat the associated bit in the fid_cnt_id field as a 'don't care' bit.
        uint64_t       reserved_31_30  :  2; // reserved.
        uint64_t          latency_ctl  :  2; // This field defines how memory latency is measured at the IDI interface. Latency measurement starts with the IDI request and ends with the configured response:00 - N/A01 - Measure latency until IDI Go response.10 - Measure latency until IDI Data response (if applicable).11 - Measure latency until the latter of Go or Data response.
        uint64_t       reserved_63_34  : 30; // reserved.
    } field;
    uint64_t val;
} LOCA_PMON_CFG_t;

// LOCA_PMON_CONTROL desc:
typedef union {
    struct {
        uint64_t               freeze  :  7; // Freeze control. Current time snapshot is captured on the cycle that counters are frozen. This bit does not self-reset. By default, the counters are enabled and free-running. When any bit goes from 0 to 1, will be updated. When any bit goes from 1 to 0, will be updated.Freeze bits are allocated as follows:[6] Single-Bit Error Event Monitors. (Disabled by freeze[6]). [5] Activity Monitors (Disabled by freeze[5]). [4] Traffic Profile Monitors (Disabled by freeze[4]). [3] Pipeline Stall Monitors (Disabled by freeze[3]). [2] Memory Latency Monitors (Disabled by freeze[2]) [1] DCache Hit Rate Monitors (Disabled by freeze[1]) [0] Measurement Period Counter (Disabled by freeze[0])
        uint64_t        reserved_15_7  :  9; // reserved.
        uint64_t         log_overflow  :  7; // When set, this causes the uppermost (49th) bit of each count in the group to act as an overflow bit, i.e. once set it stays set until clear or write. Otherwise, the 49th bit always counts.Log Overflow bits are allocated as follows:[22] Single-Bit Error Event Monitors. (Disabled by freeze[6]). [21] Activity Monitors (Disabled by freeze[5]). [20] Traffic Profile Monitors (Disabled by freeze[4]). [19] Pipeline Stall Monitors (Disabled by freeze[3]). [18] Memory Latency Monitors (Disabled by freeze[2]) [17] DCache Hit Rate Monitors (Disabled by freeze[1]) [16] Measurement Period Counter (Disabled by freeze[0])
        uint64_t       reserved_31_23  :  9; // reserved.
        uint64_t           reset_ctrs  :  7; // Resets all counters (automatically cleared when done).Reset bits are allocated as follows:[38] Single-Bit Error Event Monitors. (Disabled by freeze[6]). [37] Activity Monitors (Disabled by freeze[5]). [36] Traffic Profile Monitors (Disabled by freeze[4]). [35] Pipeline Stall Monitors (Disabled by freeze[3]). [34] Memory Latency Monitors (Disabled by freeze[2]) [33] DCache Hit Rate Monitors (Disabled by freeze[1]) [32] Measurement Period Counter (Disabled by freeze[0])
        uint64_t       reserved_63_39  : 25; // reserved.
    } field;
    uint64_t val;
} LOCA_PMON_CONTROL_t;

// LOCA_PMON_FREEZE_TIME desc:
typedef union {
    struct {
        uint64_t          freeze_time  : 56; // The local time at which the counters were frozen
        uint64_t       reserved_63_56  :  8; // reserved.
    } field;
    uint64_t val;
} LOCA_PMON_FREEZE_TIME_t;

// LOCA_PMON_ENABLE_TIME desc:
typedef union {
    struct {
        uint64_t          enable_time  : 56; // The local time at which the counters were enabled
        uint64_t       reserved_63_56  :  8; // reserved.
    } field;
    uint64_t val;
} LOCA_PMON_ENABLE_TIME_t;

// LOCA_PMON_LOCAL_TIME desc:
typedef union {
    struct {
        uint64_t           local_time  : 56; // The current local time
        uint64_t       reserved_63_56  :  8; // reserved.
    } field;
    uint64_t val;
} LOCA_PMON_LOCAL_TIME_t;

// LOCA_PMON_CNTR desc:
typedef union {
    struct {
        uint64_t                count  : 48; // PMON count value
        uint64_t             overflow  :  1; // Acts as either one more bit of count or as an overflow bit
        uint64_t       reserved_63_49  : 15; // Reserved
    } field;
    uint64_t val;
} LOCA_PMON_CNTR_t;

// LOCA_DBG_PCAM_ADDR desc:
typedef union {
    struct {
        uint64_t               EVALID  :  1; // Entry Valid. PCAM entry valid state for reads. Setting this bit on LOCA_DBG_PCAM_ADDR writes will mark the entry valid and update the entry address. Clearing this bit on writes will clear the valid bit for the entry and leave the address unchanged.
        uint64_t         reserved_5_1  :  5; // Reserved
        uint64_t                 ADDR  : 46; // PCAM entry Address
        uint64_t       reserved_63_52  : 12; // Reserved
    } field;
    uint64_t val;
} LOCA_DBG_PCAM_ADDR_t;

// LOCA_DBG_PCAM_DATA desc:
typedef union {
    struct {
        uint64_t                STATE  :  4; // State: 0-Invalid,1-Allocated,2-Pending,4-RecF,5-RecS,6-RecE,7-RecE2M,8-RecM,A-Wrtbck,C-Snp2S,D-Snp2I,E-SnpME2I
        uint64_t             PREFETCH  :  1; // Prefetched Line
        uint64_t                CHINT  :  2; // Cache Hint: 0-NANT,1-NAT,2-ANT,3-AT
        uint64_t                  WAY  :  4; // Allocated DCD entry way
        uint64_t              CONQVAL  :  1; // ConQ entry Valid
        uint64_t               CONQHP  :  8; // ConQ Head Pointer
        uint64_t               CONQTP  :  8; // ConQ Tail Pointer
        uint64_t            PROQSRPLY  :  1; // PROQ Sourced Replay flag
        uint64_t            PROQEFILL  :  1; // PROQ Enabled Fill flag
        uint64_t               PROQID  :  9; // PROQ ID
        uint64_t               SNPFWD  :  1; // Snoop Forward flag.
        uint64_t       reserved_63_40  : 24; // Reserved
    } field;
    uint64_t val;
} LOCA_DBG_PCAM_DATA_t;

// LOCA_DBG_MRU desc:
typedef union {
    struct {
        uint64_t              PENDING  : 16; // Pending. Entry way has a cache line pending from the uncore.
        uint64_t                VALID  : 16; // Valid. Entry way is valid in the Data Cache
        uint64_t                  MRU  : 16; // MRU. Entry way is marked Most Recently Used.
        uint64_t               VICTIM  : 16; // Victim. Victimized entry way for next demand loaded cache line into the indexed set.
    } field;
    uint64_t val;
} LOCA_DBG_MRU_t;

// LOCA_DBG_PROQ_FID_STATE desc:
typedef union {
    struct {
        uint64_t                  hld  :  1; // Hold.
        uint64_t             cmpl_rcv  :  1; // Complete Received.
        uint64_t             conqsent  :  1; // ConQ Sent.
        uint64_t                empty  :  1; // Empty
        uint64_t         reserved_7_4  :  4; // Reserved
        uint64_t              tailptr  :  9; // Tail Pointer - PROQ entry pointer to the last request in selected Flow ID queue.
        uint64_t       reserved_31_17  : 15; // Reserved
        uint64_t              headptr  :  9; // Head Pointer - PROQ entry pointer to the head request in selected Flow ID queue.
        uint64_t       reserved_63_41  : 23; // Reserved
    } field;
    uint64_t val;
} LOCA_DBG_PROQ_FID_STATE_t;

// LOCA_DBG_PROQ_CMPLT desc:
typedef union {
    struct {
        uint64_t             complete  : 64; // Complete state. When set, the request for that entry is marked complete.
    } field;
    uint64_t val;
} LOCA_DBG_PROQ_CMPLT_t;

// LOCA_DBG_PROQ_GLOBAL desc:
typedef union {
    struct {
        uint64_t            global_id  : 64; // Global ID state. When set, the request for that entry is assigned to the Global queue.
    } field;
    uint64_t val;
} LOCA_DBG_PROQ_GLOBAL_t;

// LOCA_DBG_PROQ_CONQVAL desc:
typedef union {
    struct {
        uint64_t              conqval  : 64; // ConQ Valid state. When set, the replay pointer for the request in that entry is a ConQ pointer. Otherwise, the pointer is an L1 TID for IMI enabled replay flows.
    } field;
    uint64_t val;
} LOCA_DBG_PROQ_CONQVAL_t;

// LOCA_DBG_IMI_ORB0 desc:
typedef union {
    struct {
        uint64_t                 ORB0  : 64; // ORB state0
    } field;
    uint64_t val;
} LOCA_DBG_IMI_ORB0_t;

// LOCA_DBG_IMI_ORB1 desc:
typedef union {
    struct {
        uint64_t                 ORB1  : 64; // ORB state1
    } field;
    uint64_t val;
} LOCA_DBG_IMI_ORB1_t;

// LOCA_DBG_IMI_ORBVAL desc:
typedef union {
    struct {
        uint64_t                valid  : 64; // ORB Entry Valid bits.
    } field;
    uint64_t val;
} LOCA_DBG_IMI_ORBVAL_t;

// LOCA_DBG_IMI_ORBHOLD desc:
typedef union {
    struct {
        uint64_t                 hold  : 64; // ORB Entry Hold bits.
    } field;
    uint64_t val;
} LOCA_DBG_IMI_ORBHOLD_t;

// LOCA_DBG_IMI_USB_GOVAL desc:
typedef union {
    struct {
        uint64_t              govalid  : 64; // U2C Response GO Valid bits.
    } field;
    uint64_t val;
} LOCA_DBG_IMI_USB_GOVAL_t;

// LOCA_DBG_IMI_DPBVAL desc:
typedef union {
    struct {
        uint64_t                valid  : 64; // DPB Entry Valid bits.
    } field;
    uint64_t val;
} LOCA_DBG_IMI_DPBVAL_t;

// LOCA_DBG_IMI_DRBVAL0 desc:
typedef union {
    struct {
        uint64_t                valid  : 64; // DRB0 Entry Valid bits.
    } field;
    uint64_t val;
} LOCA_DBG_IMI_DRBVAL0_t;

// LOCA_DBG_IMI_DRBVAL1 desc:
typedef union {
    struct {
        uint64_t                valid  : 64; // DRB1 Entry Valid bits.
    } field;
    uint64_t val;
} LOCA_DBG_IMI_DRBVAL1_t;

// LOCA_DBG_CONQ_TAILVAL desc:
typedef union {
    struct {
        uint64_t           tail_valid  : 64; // Tail Valid
    } field;
    uint64_t val;
} LOCA_DBG_CONQ_TAILVAL_t;

// LOCA_DBG_CONQ_PSRRPLY desc:
typedef union {
    struct {
        uint64_t           psr_replay  : 64; // PROQ Scheduled Replay. When set, the ConQ sourced replay request was scheduled by the PROQ.
    } field;
    uint64_t val;
} LOCA_DBG_CONQ_PSRRPLY_t;

// LOCA_DBG_CONQ_HEADER0 desc:
typedef union {
    struct {
        uint64_t              header0  : 64; // ConQ Header0
    } field;
    uint64_t val;
} LOCA_DBG_CONQ_HEADER0_t;

// LOCA_DBG_CONQ_HEADER1 desc:
typedef union {
    struct {
        uint64_t              header1  : 64; // ConQ Header1
    } field;
    uint64_t val;
} LOCA_DBG_CONQ_HEADER1_t;

// LOCA_DBG_CONQ_LINKS desc:
typedef union {
    struct {
        uint64_t                links  : 64; // ConQ Links
    } field;
    uint64_t val;
} LOCA_DBG_CONQ_LINKS_t;

// LOCA_DBG_DCD desc:
typedef union {
    struct {
        uint64_t                  WAY  :  4; // Way. DCD bank or 'way' that was accessed. Supplied by LOCA_DBG_DCD CSR address bits [15:12].
        uint64_t         reserved_5_4  :  2; // Reserved
        uint64_t                 INDX  :  9; // Index. Index to DCD. Supplied by LOCA_DBG_DCD CSR address bits [11:03]
        uint64_t                  TAG  : 37; // Tag field. Contains the physical address bits [51:15] of a valid line.
        uint64_t       reserved_59_52  :  8; // Reserved
        uint64_t                MESIF  :  4; // MESIF. State of the cache line. Defined as follows:0x1 - Shared0x2 - Exclusive0x4 - Invalid0x8 - Forward0xF - Modified
    } field;
    uint64_t val;
} LOCA_DBG_DCD_t;

// LOCA_DBG_DCACHE desc:
typedef union {
    struct {
        uint64_t                 DATA  : 64; // Data
    } field;
    uint64_t val;
} LOCA_DBG_DCACHE_t;

#endif /* ___FXR_loca_CSRS_H__ */