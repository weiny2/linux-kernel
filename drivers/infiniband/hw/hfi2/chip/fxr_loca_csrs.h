// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Thu Mar 29 15:03:56 2018
//

#ifndef ___FXR_loca_CSRS_H__
#define ___FXR_loca_CSRS_H__

// LOCA_CFG_PIPE desc:
typedef union {
    struct {
        uint64_t   depipe_full_enable  :  1; // Stalls a request at PARB input until all prior requests have cleared the DCache Write stage (full pipe)
        uint64_t   depipe_half_enable  :  1; // Stalls a request at PARB input until all prior requests have cleared the DCache read stage
        uint64_t       imicam_entries  :  2; // IMI CAM Entries. Defines the max number of clock stages+2 that a conflicting request will be delayed in PARB due to an IMI RMW update of PCAM state.
        uint64_t    snpfc_burp_enable  :  1; // Snoop Flow Control Burp Enable. When set, enables de-pipelined processing of snoop requests to use the last remaining TIDs. The newfc_burp_thresh setting defines number of TIDs carved out for snoop request processing.
        uint64_t            loca_wake  :  1; // LOCA Wake status. Wake signaling from LOCA to Power Management. Indicates LOCA has pending requests to service.
        uint64_t           pi_chicken  :  1; // PI Chicken. De-feature control for Posted Interrupt processing of reserved field violations.
        uint64_t           Reserved_7  :  1; // Unused
        uint64_t     proq_full_thresh  : 10; // PROQ Full Threshold
        uint64_t       Reserved_23_18  :  6; // Unused
        uint64_t     xin2_plus_thresh  :  7; // Min number of entries in 2 or more L1TID banks required before flow control is asserted for new requests.
        uint64_t          Reserved_31  :  1; // Unused
        uint64_t         newfc_thresh  :  9; // Min number of L1TID entries available before flow control is issued for new requests. Threshold setting accounts for worse case of fully pipelined new requests between PARB and HCC. See newfc_burp_thresh to enable de-pipelined new request processing to all available TIDs.
        uint64_t    newfc_burp_thresh  :  9; // Min number of L1TID entries available in burp mode (de-pipelined access) before flow control is issued for new requests. newfc_burp_thresh -1 reflects the min number L1TIDs guaranteed for snoop requests. Setting this threshold to a value greater than newfc_thresh disables Burp mode processing of new requests.
        uint64_t         snpfc_thresh  :  9; // Min number of L1TID entries available before flow control is asserted for Snoop requests. Threshold setting accounts for worse case of fully pipelined snoop requests between PARB and HCC. See snpfc_burp_enable to enable de-pipelined snoop request processing to utilize all available TIDs.
        uint64_t       Reserved_63_59  :  5; // Unused
    } field;
    uint64_t val;
} LOCA_CFG_PIPE_t;

// LOCA_CFG_PIPE2 desc:
typedef union {
    struct {
        uint64_t     conq_entry_limit  :  9; // Number of CONQ Entries allowed for use. Max number of 256, min number is Threshold +1
        uint64_t        Reserved_11_9  :  3; // Unused
        uint64_t       conq_fc_thresh  :  8; // CONQ Flow control threshold. ConQ Flow Control is asserted when the number of available ConQ entries is equal to or less than the threshold limit.
        uint64_t      irq_sync_thresh  :  5; // IRQ Sync Buffer threshold. Flow Control is asserted when the number IRQ requests in the Sync Buffer is greater than or equal to the threshold limit.
        uint64_t       Reserved_31_25  :  7; // Unused
        uint64_t        mru_pend_init  : 16; // MRU table Pending state Initialization. This 16 bit vector initializes the Pending state in the MRU Table at reset. By default, the Pending state is cleared at reset. Setting bits in this field will initialize the associated Cache Way to Pending which will de-feature the way from use. See frc_mru_init
        uint64_t         frc_mru_init  :  1; // Force MRU Initialization. Setting this bit will force HW to initialize the DCD and MRU Table. This allows SW to change the mru_pend_init vector and then force a re-initialization of the Table. This bit is cleared by HW when the initialization is complete.
        uint64_t       Reserved_63_49  : 15; // Unused
    } field;
    uint64_t val;
} LOCA_CFG_PIPE2_t;

// LOCA_CFG_PIPE3 desc:
typedef union {
    struct {
        uint64_t           Reserved_0  :  1; // Unused
        uint64_t   aggressive_refetch  :  1; // Aggressive Refetch enable. This mode enables the re-fetch of cache line ownership when a snoop request hits a pre-fetched line associated with a pending strongly ordered request. This mode works in conjunction with so_put_optimization mode and is only applied to optimized flows. Enabling this mode reduces the latency to complete the SO request at the potential cost of increasing the IDI bandwidth consumed to complete the request.
        uint64_t  so_put_optimization  :  1; // Strongly Ordered PUT optimization enable. This mode reduces the number of pipeline passes to complete the execution of an SO full line write from 3 passes to 2 passes. When set, the fill pass of an SO full line write request is delayed until ordering requirements have been met.
        uint64_t         Reserved_7_3  :  5; // Unused
        uint64_t         vahit_thresh  :  4; // Vector Atomic request cache Hit Threshold. This threshold setting defines number of consecutive VA cache hits needed to define a VA hit trend. PARB uses this trend information to optimize VA request scheduling in the pipeline. Also see note in the CSR description above.
        uint64_t        vamiss_thresh  :  4; // Vector Atomic request cache Miss Threshold. This threshold setting defines number of consecutive VA cache misses needed to define a VA miss trend. PARB uses VA trend information to optimize VA request scheduling in the pipeline. Also see note in the CSR description above.
        uint64_t       Reserved_63_16  : 48; // Unused
    } field;
    uint64_t val;
} LOCA_CFG_PIPE3_t;

// LOCA_CFG_PARB desc:
typedef union {
    struct {
        uint64_t    imi_req_to_thresh  :  8; // IMI Request Forward Progress Timeout Threshold (clks)
        uint64_t   emec_req_to_thresh  :  8; // EMEC Request Forward Progress Timeout Threshold (clks)
        uint64_t   conq_req_to_thresh  :  8; // ConQ Request Forward Progress Timeout Threshold (clks)
        uint64_t   trrp_req_to_thresh  :  8; // TRRP Request Forward Progress Timeout Threshold (clks)
        uint64_t     wake_hold_period  : 32; // Wake Hold Period (clks). Defines the number of 1.5Ghz clocks that LOCA will hold the WAKE signaling to Power Management after going idle. Dampens signaling so that WAKE is deasserted only after N consecutive clocks of being Idle. The default value of 1500 equates to a 1us period.
    } field;
    uint64_t val;
} LOCA_CFG_PARB_t;

// LOCA_CFG_IMI desc:
typedef union {
    struct {
        uint64_t          dpb_arb_ctl  : 16; // Defines arbitration weighting between Snoop response data and Write (writeback or memory write) data to IDI. Each set bit gives Write data priority, and each clear bit gives Snoop data priority. Default setting of 0x0 gives Snoop response data priority all of the time.
        uint64_t      idi_c2u_arb_ctl  : 16; // Defines arbitration weighting between New requests and Eviction requests to IDI. Each set bit gives Eviction requests priority, and each clear bit gives New requests priority. Default setting of 0xAAAA gives each source equal priority.
        uint64_t   dpb_snp_buf_thresh  :  8; // DPB Snoop Buffer Threshold
        uint64_t     imi_req_q_thresh  :  5; // IMI C2U Request Queue Full Threshold
        uint64_t       Reserved_47_45  :  3; // Unused
        uint64_t   imi_evict_q_thresh  :  5; // IMI C2U Eviction Queue Full Threshold
        uint64_t       Reserved_63_53  : 11; // Unused
    } field;
    uint64_t val;
} LOCA_CFG_IMI_t;

// LOCA_CFG_IMI2 desc:
typedef union {
    struct {
        uint64_t  idi_u2c_rsp_credits  :  9; // IDI U2C Response Queue Credits. Max value supported is 0x2C.
        uint64_t        Reserved_15_9  :  7; // Unused
        uint64_t idi_u2c_data_credits  :  9; // IDI U2C Data Queue Credits. Max value supported is 0x14. HW supports only even values for Data Queue Credits.
        uint64_t       Reserved_63_25  : 39; // Unused
    } field;
    uint64_t val;
} LOCA_CFG_IMI2_t;

// LOCA_CFG_IMI3 desc:
typedef union {
    struct {
        uint64_t     non_temp_rd_curr  :  3; // Non-Temporal IDI control for RD_CURR request: x00-Default behavior001-Force LRUx10-No Change011-Force MRU1x1-CacheHint[0]==0 ? LRU : MRU
        uint64_t           Reserved_3  :  1; // Unused
        uint64_t    cache_far_rd_curr  :  2; // Cache Far IDI control for RD_CURR request:00 - Force Cache Far to 001 - Force Cache Far to 110 - Cache Far = CacheHint[0]11 - Cache Far = CacheHint[1]
        uint64_t   cache_near_rd_curr  :  2; // Cache Near IDI control for RD_CURR request:00 - Force Cache Near to 001 - Force Cache Near to 110 - Cache Near = CacheHint[0]11 - Cache Near= CacheHint[1]
        uint64_t     non_temp_drd_opt  :  3; // Non-Temporal IDI control for DRD_OPT request: See non_temp_rd_curr encodings
        uint64_t          Reserved_11  :  1; // Unused
        uint64_t    cache_far_drd_opt  :  2; // Cache Far IDI control for DRD_OPT request:See cache_far_rd_curr encodings
        uint64_t   cache_near_drd_opt  :  2; // Cache Near IDI control for DRD_OPT request:See cache_near_rd_curr encodings
        uint64_t   non_temp_mempushwr  :  3; // Non-Temporal IDI control for MemPushWr request: See non_temp_rd_curr encodings
        uint64_t          Reserved_19  :  1; // Unused
        uint64_t  cache_far_mempushwr  :  2; // Cache Far IDI control for MemPushWr request:See cache_far_rd_curr encodings
        uint64_t cache_near_mempushwr  :  2; // Cache Near IDI control for MemPushWr request:See cache_near_rd_curr encodings
        uint64_t        non_temp_itom  :  3; // Non-Temporal IDI control for ITOM request: See non_temp_rd_curr encodings
        uint64_t          Reserved_27  :  1; // Unused
        uint64_t       cache_far_itom  :  2; // Cache Far IDI control for ITOM request:See cache_far_rd_curr encodings
        uint64_t      cache_near_itom  :  2; // Cache Near IDI control for ITOM request:See cache_near_rd_curr encodings
        uint64_t    non_temp_specitom  :  3; // Non-Temporal IDI control for SPECITOM request: See non_temp_rd_curr encodings
        uint64_t          Reserved_35  :  1; // Unused
        uint64_t   cache_far_specitom  :  2; // Cache Far IDI control for SPECITOM request:See cache_far_rd_curr encodings
        uint64_t  cache_near_specitom  :  2; // Cache Near IDI control for SPECITOM request:See cache_near_rd_curr encodings
        uint64_t         non_temp_rfo  :  3; // Non-Temporal IDI control for RFO request: See non_temp_rd_curr encodings
        uint64_t          Reserved_43  :  1; // Unused
        uint64_t        cache_far_rfo  :  2; // Cache Far IDI control for RFO request:See cache_far_rd_curr encodings
        uint64_t       cache_near_rfo  :  2; // Cache Near IDI control for RFO request:See cache_near_rd_curr encodings
        uint64_t      non_temp_wbmtoi  :  2; // Non-Temporal IDI control for WBMTOI request: 00-Default behavior01-Force LRU10-No change11-Force MRU
        uint64_t       Reserved_51_50  :  2; // Unused
        uint64_t     cache_far_wbmtoi  :  1; // Cache Far IDI control for WBMTOI request:0-Force to 01-Force to 1
        uint64_t    cache_near_wbmtoi  :  1; // Cache Near IDI control for WBMTOI request:0-Force to 01-Force to 1
        uint64_t       Reserved_55_54  :  2; // Unused
        uint64_t      idi_ecc_disable  :  1; // IDI ECC Disable. This is a de-feature control for IDI ECC. When set, IDI ECC checking is disabled for all U2C Data responses.
        uint64_t  full_addr_parity_en  :  1; // Full Address Parity Enable. Setting this bit enables parity calculation on the full address of IDI C2U Requests. The default setting of 0 forces parity calculation on just the cache line address (excludes address bits [5:0]).
        uint64_t   c2u_poison_disable  :  1; // IDI C2U Poison Disable. This is a de-feature control for IDI Poison signaling from FXR. When set, FXR will never assert Poison over the C2U_Data interface. Note this control bit does not control or affect FXR's processing of U2C_Data poison.
        uint64_t       Reserved_63_59  :  5; // Unused
    } field;
    uint64_t val;
} LOCA_CFG_IMI3_t;

// LOCA_CFG_ITI desc:
typedef union {
    struct {
        uint64_t  idi_u2c_req_credits  :  9; // IDI U2C Request Queue Credits
        uint64_t        Reserved_15_9  :  7; // Unused
        uint64_t      uqb_stop_thresh  : 10; // IDI U2C Request Stop Threshold
        uint64_t       Reserved_47_26  : 22; // Unused
        uint64_t  c2u_csb_full_thresh  :  5; // C2U Response Buffer Full Threshold
        uint64_t       Reserved_63_53  : 11; // Unused
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
        uint64_t       Reserved_63_16  : 48; // Unused
    } field;
    uint64_t val;
} LOCA_CFG_DALU_t;

// LOCA_IDI_STRAP desc:
typedef union {
    struct {
        uint64_t          c2u_version  :  5; // IDI Spec version supported by FXR
        uint64_t         Reserved_7_5  :  3; // Unused
        uint64_t             c2u_hash  :  1; // The Hash field being set indicates that the agent implements the address hash and lookup tables. If this bit is cleared the Fabric should ignore any inputs on the C2U Request Hash and Topology fields
        uint64_t           c2u_parity  :  1; // The Parity bit being set indicates that the Agent will generate correct parity for all outgoing addresses and appropriate outgoing data. The Agent will also check any valid parity provided by the Fabric on address and data. If the bit is cleared, then no parity information should be consumed in the Fabric, nor does the Fabric need to generate parity for messages destined to the Agent. Parity bit must be set if ECC bit is set.
        uint64_t              c2u_ecc  :  1; // The ECC bit being set indicates that the Agent will generate correct ECC for appropriate outgoing data. The Agent will also check any valid ECC provided by the Fabric for read data. If the bit is cleared, then no ECC information should be consumed in the Fabric, nor does the Fabric need to generate ECC for data messages destined to the Agent.
        uint64_t            c2u_owned  :  1; // The Owned bit being set indicates that the Agent will support being placed into the Owned state by the Fabric. If the bit is cleared, then the Fabric should not return Owned state to the Agent.
        uint64_t       c2u_datahdrsep  :  1; // The DataHdrSep bit being set indicates that the Agent expects a 3 cycle separation between the specified U2C Data fields and the rest of the U2C Data channel. If the bit is cleared, then all fields for a U2C Data packet should arrive in the same cycle.
        uint64_t            c2u_spare  :  6; // C2U Spare Straps for future use
        uint64_t       Reserved_31_19  : 13; // Unused
        uint64_t          u2c_version  :  5; // IDI Spec version supported by Uncore (Fabric)
        uint64_t       Reserved_39_37  :  3; // Unused
        uint64_t       u2c_cleanevict  :  1; // The CleanEvict bit being set indicates that the Fabric expects the agent to send CleanEvict messages to the Fabric. If the bit is cleared, then the Agent should not send CleanEvict messages to the Fabric.
        uint64_t           u2c_parity  :  1; // The Parity bit being set indicates that the Fabric will generate correct parity for all outgoing addresses and appropriate outgoing data. The Fabric will also check any valid parity provided by the Agent on address and data. If the bit is cleared, then no parity information should be consumed in the Agent, nor does the Agent need to generate parity for messages destined to the Fabric.
        uint64_t              u2c_ecc  :  1; // The ECC bit being set indicates that the Fabric will generate correct ECC for appropriate outgoing data. The Fabric will also check any valid ECC provided by the Agent for read data. If the bit is cleared, then no ECC information should be consumed in the Agent, nor does the Agent need to generate ECC for data messages destined to the Fabric.
        uint64_t            u2c_spare  :  8; // U2C Spare Straps for future use
        uint64_t       Reserved_63_51  : 13; // Unused
    } field;
    uint64_t val;
} LOCA_IDI_STRAP_t;

// LOCA_HIFIS_CFG_CONTROL1 desc:
typedef union {
    struct {
        uint64_t           arb_cntl_b  : 16; // Defines arbitration weighting between RX/TX requests to LOCA and AT requests to LOCA. Each set bit give AT request priority, and each clear bit gives RX/TX request priority. Default setting of 0x4924, in conjunction with arb_cntl_a default setting of 0xAAAA, gives each source RX, TX and AT equal priority.
        uint64_t           arb_cntl_a  : 16; // Defines arbitration weighting between RX and TX requests to LOCA. Each set bit gives TX priority, and each clear bit gives RX priority. Default setting of 0xAAAA gives each source equal priority.
        uint64_t        rspack_thresh  :  6; // Response ACK Threshold.Response buffer threshold setting for asserting flow control from HIFIS to PARB.
        uint64_t       Reserved_62_38  : 25; // Unused
        uint64_t      ecc_corr_bypass  :  1; // ECC_CORR_BYPASS. When set, disables ECC correction logic for HIFIS request headers sent to LOCA.
    } field;
    uint64_t val;
} LOCA_HIFIS_CFG_CONTROL1_t;

// LOCA0_ERR_STS desc:
typedef union {
    struct {
        uint64_t      hcc_idi_rsp_err  :  1; // IDI Response Error (Go-Err). See LOCA Error Info14 . ERR_CATEGORY_INFO
        uint64_t           u2c_req_pe  :  1; // U2C Request Parity Error. ERR_CATEGORY_NODE
        uint64_t       u2c_rsp_opcode  :  1; // U2C Response Opcode invalid for given pending request. ERR_CATEGORY_NODE
        uint64_t      u2c_rsp_invalid  :  1; // U2C Response is invalid. ERR_CATEGORY_NODE
        uint64_t           u2c_rsp_go  :  1; // U2C Response GO state error. ERR_CATEGORY_NODE
        uint64_t           u2c_rsp_pe  :  1; // U2C Response Parity Error. ERR_CATEGORY_NODE
        uint64_t         dpb_snpq_err  :  1; // Data Pull Buffer Snoop Queue Error (over/under flow). ERR_CATEGORY_NODE
        uint64_t      imi_req_que_err  :  1; // IMI Request Queue Error (over/underflow). ERR_CATEGORY_NODE
        uint64_t           proq_g_mbe  :  1; // PROQ Global tail pointer queue MBE. ERR_CATEGORY_NODE
        uint64_t           proq_f_mbe  :  1; // PROQ Flow tail pointer queue MBE. ERR_CATEGORY_NODE
        uint64_t          proq_hl_mbe  :  1; // PROQ hold queue MBE. ERR_CATEGORY_NODE
        uint64_t         conq_hdr_mbe  :  1; // ConQ header MBE. ERR_CATEGORY_NODE
        uint64_t           iti_wp_mbe  :  1; // IDI Target WP (UQB) MBE. ERR_CATEGORY_NODE
        uint64_t         u2c_data_mbe  :  1; // U2C Data MBE. ERR_CATEGORY_TRANSACTION
        uint64_t          u2c_poi_mbe  :  1; // U2C Poison and ECC Valid bit queue MBE. ERR_CATEGORY_NODE
        uint64_t          u2c_dir_mbe  :  1; // U2C L1DIR (detected in IMI) MBE. ERR_CATEGORY_NODE
        uint64_t           u2c_wp_mbe  :  1; // U2C WritePull (USB) MBE. ERR_CATEGORY_NODE
        uint64_t          c2u_eqb_mbe  :  1; // C2U Eviction Request (EQB) MBE. ERR_CATEGORY_NODE
        uint64_t          c2u_nqb_mbe  :  1; // C2U New Request (NQB) MBE. ERR_CATEGORY_NODE
        uint64_t          c2u_snp_mbe  :  1; // C2U DPB Snoop header MBE. ERR_CATEGORY_NODE
        uint64_t           c2u_wp_mbe  :  1; // C2U DPB WritePull header MBE. ERR_CATEGORY_NODE
        uint64_t         c2u_data_mbe  :  1; // C2U DPB Data MBE. ERR_CATEGORY_NODE
        uint64_t      imi_orb_cnf_mbe  :  1; // IDI Master ORB CNF MBE. ERR_CATEGORY_NODE
        uint64_t      imi_orb_rsp_mbe  :  1; // IDI Master ORB RSP MBE. ERR_CATEGORY_NODE
        uint64_t      imi_orb_sch_mbe  :  1; // IDI Master ORB SCH MBE. ERR_CATEGORY_NODE
        uint64_t        mru_table_mbe  :  1; // MRU Table MBE. ERR_CATEGORY_NODE
        uint64_t             pcam_mbe  :  1; // PCAM data MBE. ERR_CATEGORY_NODE
        uint64_t      hcc_dcd_tag_mbe  :  1; // HCC received MBE signaling with DCD tag data. ERR_CATEGORY_NODE
        uint64_t       adm_cas_db_mbe  :  1; // ADM Compare & Swap Delay Buffer MBE. ERR_CATEGTORY_TRANSACTION
        uint64_t       adm_hdr_db_mbe  :  1; // ADM Header Delay Buffer MBE. ERR_CATEGORY_NODE
        uint64_t       adm_paf_db_mbe  :  1; // ADM PAF Delay Buffer MBE. ERR_CATEGTORY_TRANSACTION
        uint64_t        adm_re_db_mbe  :  1; // ADM RE Delay Buffer MBE. ERR_CATEGTORY_TRANSACTION
        uint64_t        adm_sl_db_mbe  :  1; // ADM ShortLoop Delay Buffer MBE. ERR_CATEGTORY_TRANSACTION
        uint64_t           dcache_mbe  :  1; // DataCache MBE. ERR_CATEGTORY_TRANSACTION
        uint64_t          wrifill_mbe  :  1; // WriteFill Buffer MBE. ERR_CATEGTORY_TRANSACTION
        uint64_t   hcc_rpl_delay_perr  :  1; // HCC Replay Delay RF Parity Error. ERR_CATEGORY_NODE
        uint64_t           proq_g_sbe  :  1; // PROQ global tail pointer queue SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t           proq_f_sbe  :  1; // PROQ Flow tail pointer queue SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t          proq_hl_sbe  :  1; // PROQ hold queue SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t         conq_hdr_sbe  :  1; // ConQ header SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t           iti_wp_sbe  :  1; // IDI Target WP (UQB) SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t         u2c_data_sbe  :  1; // U2C Data SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t          u2c_poi_sbe  :  1; // U2C Poison and ECC valid bit queue SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t          u2c_dir_sbe  :  1; // U2C L1DIR (detected in IMI) SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t           u2c_wp_sbe  :  1; // U2C WritePull (USB) SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t          c2u_eqb_sbe  :  1; // C2U Eviction Request (EQB) SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t          c2u_nqb_sbe  :  1; // C2U New Request (NQB) SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t          c2u_snp_sbe  :  1; // C2U DPB Snoop header SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t           c2u_wp_sbe  :  1; // C2U DPB WritePull header SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t         c2u_data_sbe  :  1; // C2U DPB Data SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t      imi_orb_cnf_sbe  :  1; // IDI Master ORB CNF SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t      imi_orb_rsp_sbe  :  1; // IDI Master ORB RSP SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t      imi_orb_sch_sbe  :  1; // IDI Master ORB SCH SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t        mru_table_sbe  :  1; // MRU Table SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t             pcam_sbe  :  1; // PCAM data SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t       hcc_replay_err  :  1; // HCC Replay state consistency error. See hcc_replay_status . ERR_CATEGORY_NODE
        uint64_t       adm_cas_db_sbe  :  1; // ADM Compare & Swap Delay Buffer SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t       adm_hdr_db_sbe  :  1; // ADM Header Delay Buffer SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t       adm_paf_db_sbe  :  1; // ADM PAF Delay Buffer SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t        adm_re_db_sbe  :  1; // ADM RE Delay Buffer SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t        adm_sl_db_sbe  :  1; // ADM ShortLoop Delay Buffer SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t           dcache_sbe  :  1; // DataCache SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t          wrifill_sbe  :  1; // WriteFill Buffer SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t          Reserved_63  :  1; // Unused
    } field;
    uint64_t val;
} LOCA0_ERR_STS_t;

// LOCA0_ERR_CLR desc:
typedef union {
    struct {
        uint64_t               events  : 63; // Write 1's to clear corresponding LOCA0_ERR_STS bits.
        uint64_t          Reserved_63  :  1; // Unused
    } field;
    uint64_t val;
} LOCA0_ERR_CLR_t;

// LOCA0_ERR_FRC desc:
typedef union {
    struct {
        uint64_t               events  : 63; // Write 1 to set corresponding LOCA0_ERR_STS bits.
        uint64_t          Reserved_63  :  1; // Unused
    } field;
    uint64_t val;
} LOCA0_ERR_FRC_t;

// LOCA0_ERR_EN_HOST desc:
typedef union {
    struct {
        uint64_t               events  : 63; // Enables corresponding LOCA0_ERR_STS bits to generate host interrupt signal.
        uint64_t          Reserved_63  :  1; // Unused
    } field;
    uint64_t val;
} LOCA0_ERR_EN_HOST_t;

// LOCA0_ERR_FIRST_HOST desc:
typedef union {
    struct {
        uint64_t               events  : 63; // Snapshot of LOCA0_ERR_STS bits when host interrupt signal transitions from 0 to 1.
        uint64_t          Reserved_63  :  1; // Unused
    } field;
    uint64_t val;
} LOCA0_ERR_FIRST_HOST_t;

// LOCA0_ERR_EN_BMC desc:
typedef union {
    struct {
        uint64_t               events  : 63; // Enable corresponding LOCA0_ERR_STS bits to generate BMC interrupt signal.
        uint64_t          Reserved_63  :  1; // Unused
    } field;
    uint64_t val;
} LOCA0_ERR_EN_BMC_t;

// LOCA0_ERR_FIRST_BMC desc:
typedef union {
    struct {
        uint64_t               events  : 63; // Snapshot of LOCA0_ERR_STS bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t          Reserved_63  :  1; // Unused
    } field;
    uint64_t val;
} LOCA0_ERR_FIRST_BMC_t;

// LOCA0_ERR_EN_QUAR desc:
typedef union {
    struct {
        uint64_t               events  : 63; // Enable corresponding LOCA0_ERR_STS bits to generate quarantine signal.
        uint64_t          Reserved_63  :  1; // Unused
    } field;
    uint64_t val;
} LOCA0_ERR_EN_QUAR_t;

// LOCA0_ERR_FIRST_QUAR desc:
typedef union {
    struct {
        uint64_t               events  : 63; // Snapshot of LOCA0_ERR_STS bits when quarantine signal transitions from 0 to 1.
        uint64_t          Reserved_63  :  1; // Unused
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
        uint64_t       Reserved_31_30  :  2; // Unused
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
        uint64_t       Reserved_23_22  :  2; // Unused
        uint64_t         c2u_snp_synd  :  6; // C2U DBP Snoop header syndrome of c2u_snp_mbe / c2u_snp_sbe error
        uint64_t       Reserved_31_30  :  2; // Unused
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
        uint64_t       Reserved_55_53  :  3; // Unused
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
        uint64_t       Reserved_63_60  :  4; // Unused
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
        uint64_t       Reserved_63_60  :  4; // Unused
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
        uint64_t        Reserved_63_8  : 56; // Unused
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
        uint64_t       Reserved_15_13  :  3; // Unused
        uint64_t         u2c_rsp_l1id  :  8; // U2C Response Error: L1ID
        uint64_t       u2c_rsp_opcode  :  4; // U2C Response Error: opcode
        uint64_t      u2c_req_reqdata  : 16; // U2C Request Error: reqdata
        uint64_t      u2c_req_addrpar  :  1; // U2C Request Error: addrpar
        uint64_t       u2c_req_opcode  :  5; // U2C Request Error: opcode
        uint64_t       pcam_data_synd  :  7; // PCAM data syndrome of pcam_mbe / pcam_sbe error
        uint64_t       Reserved_63_57  :  7; // Unused
    } field;
    uint64_t val;
} LOCA_ERR_INFO12_t;

// LOCA_ERR_INFO13 desc:
typedef union {
    struct {
        uint64_t         Reserved_2_0  :  3; // Unused
        uint64_t         u2c_req_addr  : 49; // U2C Request Error: address
        uint64_t       Reserved_55_52  :  4; // Unused
        uint64_t    hcc_replay_status  :  8; // HCC Replay error Status. Valid when hcc_replay_err is set[7]: Invalid PCAM state[6]: Unexpected PCAM state[5]: PCAM.PENDING state not expected with Address Replay [4]: Replay request pointer doesn't match PCAM.ConQ_HP[3]: Spare[2]: PCAM state not consistent with PROQ Replay[1]: Invalid PCAM ConQ pointer[0]: PCAM Miss
    } field;
    uint64_t val;
} LOCA_ERR_INFO13_t;

// LOCA_ERR_INFO14 desc:
typedef union {
    struct {
        uint64_t         delayed_reqs  :  1; // One or more Delayed Requests in ConQ where also dropped as a result of this error (same address)
        uint64_t         Reserved_5_1  :  5; // Unused
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
        uint64_t       Reserved_15_13  :  3; // Unused
        uint64_t      dcd_bank15_synd  : 12; // DCD bank15 syndrome of dcd_bank_mbe / dcd_bank_sbe error
        uint64_t     pmon_rollover_id  :  7; // PMON counter ID of pmon_rollover event
        uint64_t       Reserved_63_35  : 29; // Unused
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
        uint64_t       Reserved_63_60  :  4; // Unused
    } field;
    uint64_t val;
} LOCA_ERR_INFO16_t;

// LOCA1_ERR_STS desc:
typedef union {
    struct {
        uint64_t         dcd_bank_mbe  : 16; // DCD Bank level MBE. ERR_CATEGORY_NODE
        uint64_t         dcd_bank_sbe  : 16; // DCD Bank level SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t        conq_tail_mbe  :  1; // ConQ Tail pointer MBE. See conq_tail_synd . ERR_CATEGORY_NODE
        uint64_t        hifis_hdr_mbe  :  1; // HIFIS Header MBE. See hifis_hdr_synd . ERR_CATEGORY_HFI
        uint64_t        conq_tail_sbe  :  1; // ConQ Tail pointer SBE. See conq_tail_synd . ERR_CATEGORY_CORRECTABLE
        uint64_t        hifis_hdr_sbe  :  1; // HIFIS Header SBE. See hifis_hdr_synd . ERR_CATEGORY_CORRECTABLE
        uint64_t        pmon_rollover  :  1; // Performance Monitor Rollover Event. ERR_CATEGORY_INFO
        uint64_t     adm_malform_blen  :  1; // Atomic operation with misaligned BLEN/ADT. ERR_CATEGORY_INFO
        uint64_t     adm_malform_addr  :  1; // Atomic operation with misaligned ADDR/ADT. ERR_CATEGORY_INFO
        uint64_t      adm_malform_adt  :  1; // Atomic operation with invalid ADT. ERR_CATEGORY_INFO
        uint64_t      adm_malform_aso  :  1; // Atomic operation with invalid ASOP. ERR_CATEGORY_INFO
        uint64_t       Reserved_63_41  : 23; // Unused
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
        uint64_t       Reserved_63_41  : 23; // Unused
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
        uint64_t       Reserved_63_41  : 23; // Unused
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
        uint64_t       Reserved_63_41  : 23; // Unused
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
        uint64_t       Reserved_63_41  : 23; // Unused
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
        uint64_t       Reserved_63_41  : 23; // Unused
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
        uint64_t       Reserved_63_41  : 23; // Unused
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
        uint64_t       Reserved_63_41  : 23; // Unused
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
        uint64_t       Reserved_63_41  : 23; // Unused
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
        uint64_t       Reserved_63_41  : 23; // Unused
    } field;
    uint64_t val;
} LOCA1_VIRAL_ENABLE_t;

// LOCA_PMON_AR_LOWER desc:
typedef union {
    struct {
        uint64_t          read_enable  :  1; // Read Enable. When set, the associated AR PMON is incremented with Get requests. The request address must also match within the specified address range, and be enabled by cachemiss_enable and/or cachehit_enable .
        uint64_t         write_enable  :  1; // Write Enable. When set, the associated AR PMON is incremented with Put requests. The request address must also match within the specified address range, and be enabled by cachemiss_enable and/or cachehit_enable .
        uint64_t        atomic_enable  :  1; // Atomic Enable. When set, the associated AR PMON is incremented with Atomic requests. The request address must also match within the specified address range, and be enabled by cachemiss_enable and/or cachehit_enable
        uint64_t      cachehit_enable  :  1; // Cache Hit Enable. When set, the associated AR PMON is incremented with requests that Hit in the FXR DCache. The request address must also match within the specified address range, and be enabled by one or more of atomic_enable , write_enable , or read_enable
        uint64_t     cachemiss_enable  :  1; // Cache Miss Enable. When set, the associated AR PMON is incremented with requests that Miss in the FXR DCache. The request must also match within the specified address range, and be enabled by one of more of atomic_enable , write_enable , or read_enable
        uint64_t        reserved_11_5  :  7; // Unused
        uint64_t     lower_addr_range  : 35; // Lower Address Range compare for address bits[46:12]. Associated pm_addr_range PMON is incremented when the following is true: upper_addr_range > request_addr[46:12] >= lower_addr_range . Also, cachemiss_enable and/or cachehit_enable must be set, and one or more of atomic_enable , write_enable , or read_enable must be set to enable pm_addr_range counting.
        uint64_t       Reserved_63_47  : 17; // Unused
    } field;
    uint64_t val;
} LOCA_PMON_AR_LOWER_t;

// LOCA_PMON_AR_UPPER desc:
typedef union {
    struct {
        uint64_t        Reserved_11_0  : 12; // Unused
        uint64_t     upper_addr_range  : 35; // Upper Address Range compare for address bits[46:12]. Associated pm_addr_range PMON is incremented when the following is true: upper_addr_range > request_addr[46:12] >= lower_addr_range . See lower_addr_range description for additional PMON increment qualifiers.
        uint64_t       Reserved_63_47  : 17; // Unused
    } field;
    uint64_t val;
} LOCA_PMON_AR_UPPER_t;

// LOCA_PMON_CFG desc:
typedef union {
    struct {
        uint64_t           at_cnt_ena  :  1; // When set, enables all counters associated AT sourced New Request events in the Pipeline Traffic Profiles.
        uint64_t           tx_cnt_ena  :  1; // When set, enables all counters associated Tx sourced New Request events in the Pipeline Traffic Profiles.
        uint64_t           rx_cnt_ena  :  1; // When set, enables all counters associated Rx sourced New Request events in the Pipeline Traffic Profiles.
        uint64_t     alloc_opcode_ena  :  1; // When set, enables GET/PUT/ATOMIC opcode DCache Hit counters for requests with allocating hints only. Otherwise all request types are counted.
        uint64_t        Reserved_15_4  : 12; // Unused
        uint64_t           fid_cnt_id  :  6; // This field defines the GID/FID used with the pm_fid_dc_hit / pm_fid_dc_req counters and the pm_new_fid_req counter. When MSB is set, requests with Global ID are counted. Otherwise, the lower 5 bits of the field define the Flow ID of counted requests.
        uint64_t       Reserved_23_22  :  2; // Unused
        uint64_t          fid_cnt_msk  :  6; // This field defines the mask that gets applied to the fid_cnt_id field below for use with associated PMON counting events. Mask bits with a value of 1 treat the associated bit in the fid_cnt_id field as a 'don't care' bit.
        uint64_t       Reserved_31_30  :  2; // Unused
        uint64_t          latency_ctl  :  2; // This field defines how memory latency is measured at the IDI interface. Latency measurement starts with the IDI request and ends with the configured response:00 - N/A01 - Measure latency until IDI Go response.10 - Measure latency until IDI Data response (if applicable).11 - Measure latency until the latter of Go or Data response.
        uint64_t       Reserved_63_34  : 30; // Unused
    } field;
    uint64_t val;
} LOCA_PMON_CFG_t;

// LOCA_PMON_CONTROL desc:
typedef union {
    struct {
        uint64_t               freeze  :  7; // Freeze control. Current time snapshot is captured on the cycle that counters are frozen. This bit does not self-reset. By default, the counters are enabled and free-running. When any bit goes from 0 to 1, will be updated. When any bit goes from 1 to 0, will be updated.Freeze bits are allocated as follows:[6] Single-Bit Error Event Monitors. (Disabled by freeze[6]). [5] Activity Monitors (Disabled by freeze[5]). [4] Traffic Profile Monitors (Disabled by freeze[4]). [3] Pipeline Stall Monitors (Disabled by freeze[3]). [2] Memory Latency Monitors (Disabled by freeze[2]) [1] DCache Hit Rate Monitors (Disabled by freeze[1]) [0] Measurement Period Counter (Disabled by freeze[0])
        uint64_t        Reserved_15_7  :  9; // Unused
        uint64_t         log_overflow  :  7; // When set, this causes the uppermost (49th) bit of each count in the group to act as an overflow bit, i.e. once set it stays set until clear or write. Otherwise, the 49th bit always counts.Log Overflow bits are allocated as follows:[22] Single-Bit Error Event Monitors. (Disabled by freeze[6]). [21] Activity Monitors (Disabled by freeze[5]). [20] Traffic Profile Monitors (Disabled by freeze[4]). [19] Pipeline Stall Monitors (Disabled by freeze[3]). [18] Memory Latency Monitors (Disabled by freeze[2]) [17] DCache Hit Rate Monitors (Disabled by freeze[1]) [16] Measurement Period Counter (Disabled by freeze[0])
        uint64_t       Reserved_31_23  :  9; // Unused
        uint64_t           reset_ctrs  :  7; // Resets all counters (automatically cleared when done).Reset bits are allocated as follows:[38] Single-Bit Error Event Monitors. (Disabled by freeze[6]). [37] Activity Monitors (Disabled by freeze[5]). [36] Traffic Profile Monitors (Disabled by freeze[4]). [35] Pipeline Stall Monitors (Disabled by freeze[3]). [34] Memory Latency Monitors (Disabled by freeze[2]) [33] DCache Hit Rate Monitors (Disabled by freeze[1]) [32] Measurement Period Counter (Disabled by freeze[0])
        uint64_t       Reserved_63_39  : 25; // Unused
    } field;
    uint64_t val;
} LOCA_PMON_CONTROL_t;

// LOCA_PMON_FREEZE_TIME desc:
typedef union {
    struct {
        uint64_t          freeze_time  : 56; // The local time at which the counters were frozen
        uint64_t       Reserved_63_56  :  8; // Unused
    } field;
    uint64_t val;
} LOCA_PMON_FREEZE_TIME_t;

// LOCA_PMON_ENABLE_TIME desc:
typedef union {
    struct {
        uint64_t          enable_time  : 56; // The local time at which the counters were enabled
        uint64_t       Reserved_63_56  :  8; // Unused
    } field;
    uint64_t val;
} LOCA_PMON_ENABLE_TIME_t;

// LOCA_PMON_LOCAL_TIME desc:
typedef union {
    struct {
        uint64_t           local_time  : 56; // The current local time
        uint64_t       Reserved_63_56  :  8; // Unused
    } field;
    uint64_t val;
} LOCA_PMON_LOCAL_TIME_t;

// LOCA_PMON_INFO desc:
typedef union {
    struct {
        uint64_t   max_wbstoi_latency  : 20; // Maximum latency sampled for WbStoI request (hclks). Enabled with pm_wbstoi_latency
        uint64_t   max_wbmtoi_latency  : 20; // Maximum latency sampled for WbMtoI request (hclks). Enabled with pm_wbmtoi_latency
        uint64_t max_mempushwr_latency  : 20; // Maximum latency sampled for MemPushWe request (hclks). Enabled with pm_mempushwr_latency
        uint64_t       Reserved_63_60  :  4; // Unused
    } field;
    uint64_t val;
} LOCA_PMON_INFO_t;

// LOCA_PMON_INFO1 desc:
typedef union {
    struct {
        uint64_t   max_rdcurr_latency  : 20; // Maximum latency sampled for RdCurr request (hclks). Enabled with pm_rdcurr_latency
        uint64_t      max_drd_latency  : 20; // Maximum latency sampled for DRd_Opt request (hclks). Enabled with pm_drd_latency
        uint64_t      max_rfo_latency  : 20; // Maximum latency sampled for RFO request (hclks). Enabled with pm_rfo_latency
        uint64_t       Reserved_63_60  :  4; // Unused
    } field;
    uint64_t val;
} LOCA_PMON_INFO1_t;

// LOCA_PMON_INFO2 desc:
typedef union {
    struct {
        uint64_t max_specitom_latency  : 20; // Maximum latency sampled for SpecItoM request (hclks). Enabled with pm_specitom_latency
        uint64_t     max_itom_latency  : 20; // Maximum latency sampled for ItoM request (hclks). Enabled with pm_itom_latency
        uint64_t       Reserved_63_40  : 24; // Unused
    } field;
    uint64_t val;
} LOCA_PMON_INFO2_t;

// LOCA_PMON_CNTR desc:
typedef union {
    struct {
        uint64_t                count  : 48; // PMON count value
        uint64_t             overflow  :  1; // Acts as either one more bit of count or as an overflow bit
        uint64_t       Reserved_63_49  : 15; // Unused
    } field;
    uint64_t val;
} LOCA_PMON_CNTR_t;

// LOCA_DBG_CACHE_CLN desc:
typedef union {
    struct {
        uint64_t              cc_addr  :  1; // Cache Clean by Address. Set by SW to initiate a CC Address operation. Cleared by HW when CC operation is complete. Cache line address is specified in ADDR field. NOTE: SW setting of this is ignored by HW if either bit[2] or bit[1] is set.
        uint64_t             cc_entry  :  1; // Cache Clean by Entry. Set by SW to initiate a CC Entry operation. Cleared by HW when CC operation is complete. Entry identifier defined in ADDR field as:LOCA_DBG_CACHE_CLN.ADDR[8:0] - DCD Set indexLOCA_DBG_CACHE_CLN.ADDR[12:9] - DCD Way (bank)NOTE: SW setting of this is ignored by HW if either bit[2] or bit[0] is set.
        uint64_t            hw_seq_cc  :  1; // Hardware Sequenced Cache Clean. HW sequence flow of all 8K DCD entries. Set by SW to initiate CC operation. Cleared by HW when CC operation is complete. ADDR field is ignored for this operation. NOTE: SW setting of this is ignored by HW if either bit[1] or bit[0] is set.
        uint64_t         Reserved_5_3  :  3; // Unused
        uint64_t                 addr  : 46; // Address. Physical cache line address[51:6] or DCD entry ID for cache clean operation specified in bits [1:0]
        uint64_t       Reserved_63_52  : 12; // Unused
    } field;
    uint64_t val;
} LOCA_DBG_CACHE_CLN_t;

// LOCA_DBG_ERRINJECT desc:
typedef union {
    struct {
        uint64_t                 mask  :  8; // Mask. The mask field is XORed with the DCache read data ECC check bits to flip the associated ECC bit(s) for each bit that is set. This mask is applied to each of the 8 ECC domains in the DCache read. The mask is enabled in each ECC Domain by the enable field.
        uint64_t               enable  :  8; // Enable. Each set bit enables the mask to flip DCache read data ECC check bits in the associated ECC Domain. Enable bits are set by SW and cleared by HW after an error has been injected on a valid DCache read. Enable[0] is associated with the ECC check bits for read data[63:0], and enable[7] is associated the ECC check bits for read data[511:448].
        uint64_t       Reserved_63_16  : 48; // Unused
    } field;
    uint64_t val;
} LOCA_DBG_ERRINJECT_t;

// LOCA_DBG_PROBE_ADDR desc:
typedef union {
    struct {
        uint64_t                   go  :  1; // GO. When GO bit is set via SW write, HW initiates a probe operation. HW clears this bit when probe request has been serviced. Writing a 0 has no affect and will not initiate a probe operation.
        uint64_t         Reserved_5_1  :  5; // Unused
        uint64_t                 addr  : 46; // Address. Physical probe address bits[51:6]
        uint64_t       Reserved_63_52  : 12; // Unused
    } field;
    uint64_t val;
} LOCA_DBG_PROBE_ADDR_t;

// LOCA_DBG_PROBE_DIR desc:
typedef union {
    struct {
        uint64_t                valid  :  1; // Valid. Probe contents are valid. HW clears this bit when SW sets the LOCA_DBG_PROBE_ADDR .GO bit. HW sets this bit when loading the register with Probe data,
        uint64_t              dcd_hit  :  1; // DCD HIT. Data Cache Hit
        uint64_t             pcam_hit  :  1; // PCAM HIT. Pending CAM Hit
        uint64_t           Reserved_3  :  1; // Unused
        uint64_t            dcd_mesif  :  4; // DCD MESIF state. MESIF state encoding:4'h1: Shared4'h2: Exclusive4'h4: Invalid4'h8: Forward4'hF: Modified
        uint64_t           pcam_state  :  4; // PCAM STATE:4'h0: Error4'h1: Refetch4'h2: Allocated4'h3: Pending4'h4: ReceivedF4'h5: ReceivedS4'h6: ReceivedE4'h7: ReceivedE2M4'h8: ReceivedM4'h9: Writeback4'hA: WritebackCL4'hB: SnoopF2I4'hC: SnoopF2S4'hD: SnoopE2I4'hE: SnoopE2S4'hF: SnoopME2I
        uint64_t        pcam_prefetch  :  1; // PCAM Prefetch.Pending line is a prefetch for an ordered request.
        uint64_t          pcam_rplflg  :  1; // PCAM Replay Flag. Set when HCC replay is pending.
        uint64_t          pcam_chint1  :  1; // PCAM Cache Hint[1]. Allocating hint bit.
        uint64_t             pcam_way  :  4; // PCAM Wayness. Allocated way for pending request.
        uint64_t         pcam_conqval  :  1; // PCAM ConQ Valid. ConQ has a request that is linked to the PCAM entry. Head/Tail pointer data is valid
        uint64_t          pcam_conqhp  :  8; // PCAM ConQ Head Pointer
        uint64_t          pcam_conqtp  :  8; // PCAM ConQ Tail Pointer
        uint64_t       pcam_proqsrply  :  1; // PCAM PROQ Schedule Replay. Pending line will have its associated request replayed by the PROQ.
        uint64_t       pcam_proqefill  :  1; // PCAM PROQ Enabled Fill.Fill Pass for the pending line will be enabled by the PROQ.
        uint64_t          pcam_proqid  :  9; // PCAM PROQ ID. PROQ ID associated with pending line request.
        uint64_t          pcam_snpfwd  :  1; // PCAM Snoop Forward. Fill pass data must be forward to the DPB to complete the data transfer of a prior snoop request.
        uint64_t       Reserved_63_48  : 16; // Unused
    } field;
    uint64_t val;
} LOCA_DBG_PROBE_DIR_t;

// LOCA_DBG_PROBE_DATA desc:
typedef union {
    struct {
        uint64_t                 data  : 64; // DATA
    } field;
    uint64_t val;
} LOCA_DBG_PROBE_DATA_t;

// LOCA_DBG_PCAM_ADDR desc:
typedef union {
    struct {
        uint64_t               evalid  :  1; // Entry Valid. PCAM entry valid state for reads. Setting this bit on LOCA_DBG_PCAM_ADDR writes will mark the entry valid. Clearing this bit on writes will clear the valid bit for the entry.
        uint64_t         Reserved_5_1  :  5; // Unused
        uint64_t                 addr  : 46; // PCAM entry Address
        uint64_t       Reserved_63_52  : 12; // Unused
    } field;
    uint64_t val;
} LOCA_DBG_PCAM_ADDR_t;

// LOCA_DBG_PCAM_DATA desc:
typedef union {
    struct {
        uint64_t                state  :  4; // State: 0-Error, 1-Refetch, 2-Allocated, 3-Pending, 4-RecF, 5-RecS, 6-RecE, 7-RecE2M, 8-RecM, 9-Wrtbck, A-WrtbckCL, B-SnpSF2S, C-SnpE2S, D-SnpFS2I, E-SnpE2I, F- SnpME2I
        uint64_t             prefetch  :  1; // Prefetched Line
        uint64_t               rplflg  :  1; // Replay pending Flag
        uint64_t               chint1  :  1; // Cache Hint[1] (Allocating)
        uint64_t                  way  :  4; // Allocated DCD entry way
        uint64_t              conqval  :  1; // ConQ entry Valid
        uint64_t               conqhp  :  8; // ConQ Head Pointer
        uint64_t               conqtp  :  8; // ConQ Tail Pointer
        uint64_t            proqsrply  :  1; // PROQ Sourced Replay flag
        uint64_t            proqefill  :  1; // PROQ Enabled Fill flag
        uint64_t               proqid  :  9; // PROQ ID
        uint64_t               snpfwd  :  1; // Snoop Forward flag.
        uint64_t       Reserved_63_40  : 24; // Unused
    } field;
    uint64_t val;
} LOCA_DBG_PCAM_DATA_t;

// LOCA_DBG_MRU desc:
typedef union {
    struct {
        uint64_t              pending  : 16; // Pending. Entry way has a cache line pending from the uncore.
        uint64_t                valid  : 16; // Valid. Entry way is valid in the Data Cache
        uint64_t                  mru  : 16; // MRU. Entry way is marked Most Recently Used.
        uint64_t               victim  : 16; // Victim. One-hot ID of victimized entry way for next demand loaded cache line into the indexed set. Debug write data must be a valid one-hot value.
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
        uint64_t         Reserved_7_4  :  4; // Unused
        uint64_t              tailptr  :  9; // Tail Pointer - PROQ entry pointer to the last request in selected Flow ID queue.
        uint64_t       Reserved_31_17  : 15; // Unused
        uint64_t              headptr  :  9; // Head Pointer - PROQ entry pointer to the head request in selected Flow ID queue.
        uint64_t       Reserved_63_41  : 23; // Unused
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
        uint64_t                 orb0  : 64; // ORB state0
    } field;
    uint64_t val;
} LOCA_DBG_IMI_ORB0_t;

// LOCA_DBG_IMI_ORB1 desc:
typedef union {
    struct {
        uint64_t                 orb1  : 64; // ORB state1
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
        uint64_t                  way  :  4; // Way. DCD bank or 'way' that was accessed. Supplied by LOCA_DBG_DCD CSR address bits [15:12].
        uint64_t         Reserved_5_4  :  2; // Unused
        uint64_t                 indx  :  9; // Index. Contains the physical address bits [14:6] of a valid line. This is calculated (un-hashed) from tag field and LOCA_DBG_DCD CSR address bits [11:3]
        uint64_t                  tag  : 37; // Tag field. Contains the physical address bits [51:15] of a valid line. (stored in the DCD).
        uint64_t       Reserved_59_52  :  8; // Unused
        uint64_t                mesif  :  4; // MESIF. State of the cache line. Defined as follows:0x1 - Shared0x2 - Exclusive0x4 - Invalid0x8 - Forward0xF - Modified
    } field;
    uint64_t val;
} LOCA_DBG_DCD_t;

// LOCA_DBG_DCACHE desc:
typedef union {
    struct {
        uint64_t                 data  : 64; // Data
    } field;
    uint64_t val;
} LOCA_DBG_DCACHE_t;

#endif /* ___FXR_loca_CSRS_H__ */