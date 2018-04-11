// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Wed Apr 11 12:49:08 2018
//

#ifndef ___FXR_loca_CSRS_H__
#define ___FXR_loca_CSRS_H__

// LOCA_CFG_PIPE desc:
typedef union {
    struct {
        uint64_t   depipe_full_enable  :  1; // Stalls a request at PARB input until all prior requests have cleared the DCache Write stage (full pipe)
        uint64_t   depipe_half_enable  :  1; // Stalls a request at PARB input until all prior requests have cleared the DCache read stage
        uint64_t       pmicam_entries  :  2; // PMI CAM Entries. Defines the max number of clock stages+2 that a conflicting request will be delayed in PARB due to an PMI RMW update of PCAM state.
        uint64_t            loca_wake  :  1; // LOCA Wake status. Wake signaling from LOCA to Power Management. Indicates LOCA has pending requests to service.
        uint64_t         Reserved_7_5  :  3; // Unused
        uint64_t     proq_full_thresh  : 10; // PROQ Full Threshold
        uint64_t       Reserved_23_18  :  6; // Unused
        uint64_t     xin2_plus_thresh  :  7; // Min number of entries in 2 or more L1TID banks required before flow control is asserted for new requests.
        uint64_t          Reserved_31  :  1; // Unused
        uint64_t         newfc_thresh  :  9; // Min number of L1TID entries available before flow control is issued for new requests. Threshold setting accounts for worse case of fully pipelined new requests between PARB and HCC. See newfc_burp_thresh to enable de-pipelined new request processing to all available TIDs.
        uint64_t    newfc_burp_thresh  :  9; // Min number of L1TID entries available in burp mode (de-pipelined access) before flow control is issued for new requests. newfc_burp_thresh -1 reflects the min number L1TIDs guaranteed for snoop requests. Setting this threshold to a value greater than newfc_thresh disables Burp mode processing of new requests.
        uint64_t       Reserved_63_50  : 14; // Unused
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
        uint64_t           pcie_rd_ro  :  1; // PCIe Read RO control. This bit defines the Relaxed Order setting for PCIe Read requests issued by FXR
        uint64_t         Reserved_2_1  :  2; // Unused
        uint64_t      go_scrub_enable  :  1; // GO Scrubber enable.
        uint64_t       go_generations  : 12; // GO Generations. Defines the number of generation wraps enabled for use with the Write RR FIFO.
        uint64_t          go_wrr_size  :  9; // GO WriteRR FIFO Size. Defines the number of WriteRR FIFO entries enabled use. A value of 0 means all 512 entries are enabled for use.
        uint64_t       Reserved_27_25  :  3; // Unused
        uint64_t        go_wrr_thresh  :  6; // GO WriteRR FIFO threshold. Defines the threshold value for asserting flow control to the LOCA pipeline. Flow control is asserted when the number of available entries in the FIFO is less than the threshold value.
        uint64_t       Reserved_63_34  : 30; // Unused
    } field;
    uint64_t val;
} LOCA_CFG_PIPE3_t;

// LOCA_CFG_PARB desc:
typedef union {
    struct {
        uint64_t    pmi_req_to_thresh  :  8; // PMI Request Forward Progress Timeout Threshold (clks)
        uint64_t   emec_req_to_thresh  :  8; // EMEC Request Forward Progress Timeout Threshold (clks)
        uint64_t   conq_req_to_thresh  :  8; // ConQ Request Forward Progress Timeout Threshold (clks)
        uint64_t   trrp_req_to_thresh  :  8; // TRRP Request Forward Progress Timeout Threshold (clks)
        uint64_t     wake_hold_period  : 32; // Wake Hold Period (clks). Defines the number of 1.0Ghz clocks that LOCA will hold the WAKE signaling to Power Management after going idle. Dampens signaling so that WAKE is deasserted only after N consecutive clocks of being Idle. The default value of 1500 equates to a 1us period.
    } field;
    uint64_t val;
} LOCA_CFG_PARB_t;

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

// LOCA_CFG_TPH desc:
typedef union {
    struct {
        uint64_t                   st  :  8; // Steering Tag
        uint64_t                   ns  :  1; // No Snoop Attribute
        uint64_t                   ph  :  2; // Processing Hints
        uint64_t                   th  :  1; // TLP Hint enable
        uint64_t       Reserved_63_12  : 52; // Unused
    } field;
    uint64_t val;
} LOCA_CFG_TPH_t;

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
        uint64_t      hcc_pmi_rsp_err  :  1; // PMI Response Error (Go-Err). See LOCA Error Info14 . ERR_CATEGORY_INFO
        uint64_t       pmi_xali_q_err  :  1; // PMI Request Queue Error (over/underflow). ERR_CATEGORY_NODE
        uint64_t           proq_g_mbe  :  1; // PROQ Global tail pointer queue MBE. ERR_CATEGORY_NODE
        uint64_t           proq_f_mbe  :  1; // PROQ Flow tail pointer queue MBE. ERR_CATEGORY_NODE
        uint64_t          proq_hl_mbe  :  1; // PROQ hold queue MBE. ERR_CATEGORY_NODE
        uint64_t         conq_hdr_mbe  :  1; // ConQ header MBE. ERR_CATEGORY_NODE
        uint64_t     pmi_cpl_data_mbe  :  1; // PMI Completion Data MBE. ERR_CATEGORY_TRANSACTION
        uint64_t    pmi_cpl_q_hdr_mbe  :  1; // PMI Completion Header MBE. ERR_CATEGORY_NODE
        uint64_t          u2c_dir_mbe  :  1; // PCAM Read MBE detected in PMI. ERR_CATEGORY_NODE
        uint64_t     pmi_xali_hdr_mbe  :  1; // PMI XALI Header MBE. ERR_CATEGORY_NODE
        uint64_t    pmi_xali_data_mbe  :  1; // PMI XALI Data MBE. ERR_CATEGORY_NODE
        uint64_t          pmi_mrb_mbe  :  1; // PMI Master Request Buffer MBE. ERR_CATEGORY_NODE
        uint64_t      pmi_orb_rsp_mbe  :  1; // PMI ORB RSP MBE. ERR_CATEGORY_NODE
        uint64_t      pmi_orb_sch_mbe  :  1; // PMI ORB SCH MBE. ERR_CATEGORY_NODE
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
        uint64_t     pmi_cpl_data_sbe  :  1; // PMI Completion Data SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t    pmi_cpl_q_hdr_sbe  :  1; // PMI Completion Header SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t          u2c_dir_sbe  :  1; // PCAM Read SBE detected in PMI. ERR_CATEGORY_CORRECTABLE
        uint64_t     pmi_xali_hdr_sbe  :  1; // PMI XALI Header SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t    pmi_xali_data_sbe  :  1; // PMI XALI Data SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t          pmi_mrb_sbe  :  1; // PMI Master Request Buffer SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t      pmi_orb_rsp_sbe  :  1; // PMI ORB RSP SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t      pmi_orb_sch_sbe  :  1; // PMI ORB SCH SBE. ERR_CATEGORY_CORRECTABLE
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
        uint64_t       Reserved_63_47  : 17; // Unused
    } field;
    uint64_t val;
} LOCA0_ERR_STS_t;

// LOCA0_ERR_CLR desc:
typedef union {
    struct {
        uint64_t               events  : 47; // Write 1's to clear corresponding LOCA0_ERR_STS bits.
        uint64_t       Reserved_63_47  : 17; // Unused
    } field;
    uint64_t val;
} LOCA0_ERR_CLR_t;

// LOCA0_ERR_FRC desc:
typedef union {
    struct {
        uint64_t               events  : 47; // Write 1 to set corresponding LOCA0_ERR_STS bits.
        uint64_t       Reserved_63_47  : 17; // Unused
    } field;
    uint64_t val;
} LOCA0_ERR_FRC_t;

// LOCA0_ERR_EN_HOST desc:
typedef union {
    struct {
        uint64_t               events  : 47; // Enables corresponding LOCA0_ERR_STS bits to generate host interrupt signal.
        uint64_t       Reserved_63_47  : 17; // Unused
    } field;
    uint64_t val;
} LOCA0_ERR_EN_HOST_t;

// LOCA0_ERR_FIRST_HOST desc:
typedef union {
    struct {
        uint64_t               events  : 47; // Snapshot of LOCA0_ERR_STS bits when host interrupt signal transitions from 0 to 1.
        uint64_t       Reserved_63_47  : 17; // Unused
    } field;
    uint64_t val;
} LOCA0_ERR_FIRST_HOST_t;

// LOCA0_ERR_EN_BMC desc:
typedef union {
    struct {
        uint64_t               events  : 47; // Enable corresponding LOCA0_ERR_STS bits to generate BMC interrupt signal.
        uint64_t       Reserved_63_47  : 17; // Unused
    } field;
    uint64_t val;
} LOCA0_ERR_EN_BMC_t;

// LOCA0_ERR_FIRST_BMC desc:
typedef union {
    struct {
        uint64_t               events  : 47; // Snapshot of LOCA0_ERR_STS bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t       Reserved_63_47  : 17; // Unused
    } field;
    uint64_t val;
} LOCA0_ERR_FIRST_BMC_t;

// LOCA0_ERR_EN_QUAR desc:
typedef union {
    struct {
        uint64_t               events  : 47; // Enable corresponding LOCA0_ERR_STS bits to generate quarantine signal.
        uint64_t       Reserved_63_47  : 17; // Unused
    } field;
    uint64_t val;
} LOCA0_ERR_EN_QUAR_t;

// LOCA0_ERR_FIRST_QUAR desc:
typedef union {
    struct {
        uint64_t               events  : 47; // Snapshot of LOCA0_ERR_STS bits when quarantine signal transitions from 0 to 1.
        uint64_t       Reserved_63_47  : 17; // Unused
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
        uint64_t         pmi_mrb_synd  :  8; // PMI MRB syndrome of pmi_mrb_mbe / pmi_mrb_sbe error
        uint64_t        Reserved_63_8  : 56; // Unused
    } field;
    uint64_t val;
} LOCA_ERR_INFO1_t;

// LOCA_ERR_INFO2 desc:
typedef union {
    struct {
        uint64_t    pmi_xali_hdr_synd  :  8; // PMI XALI Header syndrome of pmi_xali_hdr_mbe / pmi_xali_hdr_sbe error
        uint64_t   pmi_xali_data_synd  :  8; // PMI XALI Data syndrome of pmi_xali_data_mbe / pmi_xali_data_sbe error
        uint64_t       Reserved_21_16  :  6; // Unused
        uint64_t       Reserved_23_22  :  2; // Unused
        uint64_t       Reserved_29_24  :  6; // Unused
        uint64_t       Reserved_31_30  :  2; // Unused
        uint64_t         orb_sch_synd  :  8; // PMI ORB Scheduler syndrome of pmi_orb_sch_mbe / pmi_orb_sch_sbe error
        uint64_t         orb_rsp_synd  :  8; // PMI ORB Response syndrome of pmi_orb_rsp_mbe / pmi_orb_rsp_sbe error
        uint64_t         orb_cnf_synd  :  8; // PMI ORB CNF syndrome of imi_orb_cnf_mbe / imi_orb_cnf_sbe error
        uint64_t       Reserved_63_56  :  8; // Unused
    } field;
    uint64_t val;
} LOCA_ERR_INFO2_t;

// LOCA_ERR_INFO3 desc:
typedef union {
    struct {
        uint64_t     pmi_cpl_hdr_synd  :  8; // PMI Completion Header syndrome of pmi_cpl_q_hdr_mbe / pmi_cpl_q_hdr_sbe error
        uint64_t    pmi_cpl_data_synd  :  8; // PMI Completion Data syndrome of pmi_cpl_data_mbe / pmi_cpl_data_sbe error
        uint64_t       Reserved_45_16  : 30; // Unused
        uint64_t         u2c_dir_synd  :  7; // U2C L1Dir syndrome of u2c_dir_mbe / u2c_dir_sbe error
        uint64_t       Reserved_55_53  :  3; // Unused
        uint64_t       mru_table_synd  :  8; // MRU Table syndrome of mru_table_mbe / mru_table_sbe error
    } field;
    uint64_t val;
} LOCA_ERR_INFO3_t;

// LOCA_ERR_INFO4 desc:
typedef union {
    struct {
        uint64_t       dcd_bank0_synd  : 14; // DCD bank0 syndrome of dcd_bank_mbe / dcd_bank_sbe error
        uint64_t       dcd_bank1_synd  : 14; // DCD bank1 syndrome of dcd_bank_mbe / dcd_bank_sbe error
        uint64_t       dcd_bank2_synd  : 14; // DCD bank2 syndrome of dcd_bank_mbe / dcd_bank_sbe error
        uint64_t       dcd_bank3_synd  : 14; // DCD bank3 syndrome of dcd_bank_mbe / dcd_bank_sbe error
        uint64_t       Reserved_63_56  :  8; // Unused
    } field;
    uint64_t val;
} LOCA_ERR_INFO4_t;

// LOCA_ERR_INFO5 desc:
typedef union {
    struct {
        uint64_t       dcd_bank4_synd  : 14; // DCD bank4 syndrome of dcd_bank_mbe / dcd_bank_sbe error
        uint64_t       dcd_bank5_synd  : 14; // DCD bank5syndrome of dcd_bank_mbe / dcd_bank_sbe error
        uint64_t       dcd_bank6_synd  : 14; // DCD bank6 syndrome of dcd_bank_mbe / dcd_bank_sbe error
        uint64_t       dcd_bank7_synd  : 14; // DCD bank7 syndrome of dcd_bank_mbe / dcd_bank_sbe error
        uint64_t       Reserved_63_56  :  8; // Unused
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
        uint64_t        Reserved_63_0  : 64; // Unused
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
        uint64_t       pcam_data_synd  :  7; // PCAM data syndrome of pcam_mbe / pcam_sbe error
        uint64_t        Reserved_63_7  : 57; // Unused
    } field;
    uint64_t val;
} LOCA_ERR_INFO12_t;

// LOCA_ERR_INFO13 desc:
typedef union {
    struct {
        uint64_t        Reserved_55_0  : 56; // Unused
        uint64_t    hcc_replay_status  :  8; // HCC Replay error Status. Valid when hcc_replay_err is set[7]: Invalid PCAM state[6]: Unexpected PCAM state[5]: PCAM.PENDING state not expected with Address Replay [4]: Replay request pointer doesn't match PCAM.ConQ_HP[3]: Spare[2]: PCAM state not consistent with PROQ Replay[1]: Invalid PCAM ConQ pointer[0]: PCAM Miss
    } field;
    uint64_t val;
} LOCA_ERR_INFO13_t;

// LOCA_ERR_INFO14 desc:
typedef union {
    struct {
        uint64_t         delayed_reqs  :  1; // One or more Delayed Requests in ConQ where also dropped as a result of this error (same address)
        uint64_t         Reserved_5_1  :  5; // Unused
        uint64_t              address  : 46; // PMI Response Error Address[51:6]
        uint64_t                  tid  : 12; // HFI Tid associated with PMI Rsp Error. See hcc_pmi_rsp_err .
    } field;
    uint64_t val;
} LOCA_ERR_INFO14_t;

// LOCA_ERR_INFO15 desc:
typedef union {
    struct {
        uint64_t       hifis_hdr_synd  :  8; // HIFIS Header Syndrome of hifis_hdr_mbe / hifis_hdr_sbe
        uint64_t       conq_tail_synd  :  5; // ConQ Tail Pointer syndrome of conq_tail_mbe / conq_tail_sbe
        uint64_t       Reserved_27_13  : 15; // Unused
        uint64_t     pmon_rollover_id  :  7; // PMON counter ID of pmon_rollover event
        uint64_t       Reserved_63_35  : 29; // Unused
    } field;
    uint64_t val;
} LOCA_ERR_INFO15_t;

// LOCA1_ERR_STS desc:
typedef union {
    struct {
        uint64_t         dcd_bank_mbe  :  8; // DCD Bank level MBE. ERR_CATEGORY_NODE
        uint64_t         dcd_bank_sbe  :  8; // DCD Bank level SBE. ERR_CATEGORY_CORRECTABLE
        uint64_t        conq_tail_mbe  :  1; // ConQ Tail pointer MBE. See conq_tail_synd . ERR_CATEGORY_NODE
        uint64_t        hifis_hdr_mbe  :  1; // HIFIS Header MBE. See hifis_hdr_synd . ERR_CATEGORY_HFI
        uint64_t        conq_tail_sbe  :  1; // ConQ Tail pointer SBE. See conq_tail_synd . ERR_CATEGORY_CORRECTABLE
        uint64_t        hifis_hdr_sbe  :  1; // HIFIS Header SBE. See hifis_hdr_synd . ERR_CATEGORY_CORRECTABLE
        uint64_t        pmon_rollover  :  1; // Performance Monitor Rollover Event. ERR_CATEGORY_INFO
        uint64_t     adm_malform_blen  :  1; // Atomic operation with misaligned BLEN/ADT. ERR_CATEGORY_INFO
        uint64_t     adm_malform_addr  :  1; // Atomic operation with misaligned ADDR/ADT. ERR_CATEGORY_INFO
        uint64_t      adm_malform_adt  :  1; // Atomic operation with invalid ADT. ERR_CATEGORY_INFO
        uint64_t      adm_malform_aso  :  1; // Atomic operation with invalid ASOP. ERR_CATEGORY_INFO
        uint64_t       Reserved_63_25  : 39; // Unused
    } field;
    uint64_t val;
} LOCA1_ERR_STS_t;

// LOCA1_ERR_CLR desc:
typedef union {
    struct {
        uint64_t         dcd_bank_mbe  :  8; // Write 1's to clear corresponding LOCA1_ERR_STS status bits.
        uint64_t         dcd_bank_sbe  :  8; // Write 1's to clear corresponding LOCA1_ERR_STS status bits.
        uint64_t        conq_tail_mbe  :  1; // Write 1's to clear corresponding LOCA1_ERR_STS status bits.
        uint64_t        hifis_hdr_mbe  :  1; // Write 1's to clear corresponding LOCA1_ERR_STS status bits.
        uint64_t        conq_tail_sbe  :  1; // Write 1's to clear corresponding LOCA1_ERR_STS status bits.
        uint64_t        hifis_hdr_sbe  :  1; // Write 1's to clear corresponding LOCA1_ERR_STS status bits.
        uint64_t        pmon_rollover  :  1; // Write 1's to clear corresponding LOCA1_ERR_STS status bits.
        uint64_t     adm_malform_blen  :  1; // Write 1's to clear corresponding LOCA1_ERR_STS status bits.
        uint64_t     adm_malform_addr  :  1; // Write 1's to clear corresponding LOCA1_ERR_STS status bits.
        uint64_t      adm_malform_adt  :  1; // Write 1's to clear corresponding LOCA1_ERR_STS status bits.
        uint64_t      adm_malform_aso  :  1; // Write 1's to clear corresponding LOCA1_ERR_STS status bits.
        uint64_t       Reserved_63_25  : 39; // Unused
    } field;
    uint64_t val;
} LOCA1_ERR_CLR_t;

// LOCA1_ERR_FRC desc:
typedef union {
    struct {
        uint64_t         dcd_bank_mbe  :  8; // Write 1 to set corresponding LOCA1_ERR_STS status bits.
        uint64_t         dcd_bank_sbe  :  8; // Write 1 to set corresponding LOCA1_ERR_STS status bits.
        uint64_t        conq_tail_mbe  :  1; // Write 1 to set corresponding LOCA1_ERR_STS status bits.
        uint64_t        hifis_hdr_mbe  :  1; // Write 1 to set corresponding LOCA1_ERR_STS status bits.
        uint64_t        conq_tail_sbe  :  1; // Write 1 to set corresponding LOCA1_ERR_STS status bits.
        uint64_t        hifis_hdr_sbe  :  1; // Write 1 to set corresponding LOCA1_ERR_STS status bits.
        uint64_t        pmon_rollover  :  1; // Write 1 to set corresponding LOCA1_ERR_STS status bits.
        uint64_t     adm_malform_blen  :  1; // Write 1 to set corresponding LOCA1_ERR_STS status bits.
        uint64_t     adm_malform_addr  :  1; // Write 1 to set corresponding LOCA1_ERR_STS status bits.
        uint64_t      adm_malform_adt  :  1; // Write 1 to set corresponding LOCA1_ERR_STS status bits.
        uint64_t      adm_malform_aso  :  1; // Write 1 to set corresponding LOCA1_ERR_STS status bits.
        uint64_t       Reserved_63_25  : 39; // Unused
    } field;
    uint64_t val;
} LOCA1_ERR_FRC_t;

// LOCA1_ERR_EN_HOST desc:
typedef union {
    struct {
        uint64_t         dcd_bank_mbe  :  8; // Enables corresponding LOCA1_ERR_STS bits to generate host interrupt signal.
        uint64_t         dcd_bank_sbe  :  8; // Enables corresponding LOCA1_ERR_STS bits to generate host interrupt signal.
        uint64_t        conq_tail_mbe  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate host interrupt signal.
        uint64_t        hifis_hdr_mbe  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate host interrupt signal.
        uint64_t        conq_tail_sbe  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate host interrupt signal.
        uint64_t        hifis_hdr_sbe  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate host interrupt signal.
        uint64_t        pmon_rollover  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate host interrupt signal.
        uint64_t     adm_malform_blen  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate host interrupt signal.
        uint64_t     adm_malform_addr  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate host interrupt signal.
        uint64_t      adm_malform_adt  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate host interrupt signal.
        uint64_t      adm_malform_aso  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate host interrupt signal.
        uint64_t       Reserved_63_25  : 39; // Unused
    } field;
    uint64_t val;
} LOCA1_ERR_EN_HOST_t;

// LOCA1_ERR_FIRST_HOST desc:
typedef union {
    struct {
        uint64_t         dcd_bank_mbe  :  8; // Snapshot of LOCA1_ERR_STS bits when host interrupt signal transitions from 0 to 1.
        uint64_t         dcd_bank_sbe  :  8; // Snapshot of LOCA1_ERR_STS bits when host interrupt signal transitions from 0 to 1.
        uint64_t        conq_tail_mbe  :  1; // Snapshot of LOCA1_ERR_STS bits when host interrupt signal transitions from 0 to 1.
        uint64_t        hifis_hdr_mbe  :  1; // Snapshot of LOCA1_ERR_STS bits when host interrupt signal transitions from 0 to 1.
        uint64_t        conq_tail_sbe  :  1; // Snapshot of LOCA1_ERR_STS bits when host interrupt signal transitions from 0 to 1.
        uint64_t        hifis_hdr_sbe  :  1; // Snapshot of LOCA1_ERR_STS bits when host interrupt signal transitions from 0 to 1.
        uint64_t        pmon_rollover  :  1; // Snapshot of LOCA1_ERR_STS bits when host interrupt signal transitions from 0 to 1.
        uint64_t     adm_malform_blen  :  1; // Snapshot of LOCA1_ERR_STS bits when host interrupt signal transitions from 0 to 1.
        uint64_t     adm_malform_addr  :  1; // Snapshot of LOCA1_ERR_STS bits when host interrupt signal transitions from 0 to 1.
        uint64_t      adm_malform_adt  :  1; // Snapshot of LOCA1_ERR_STS bits when host interrupt signal transitions from 0 to 1.
        uint64_t      adm_malform_aso  :  1; // Snapshot of LOCA1_ERR_STS bits when host interrupt signal transitions from 0 to 1.
        uint64_t       Reserved_63_25  : 39; // Unused
    } field;
    uint64_t val;
} LOCA1_ERR_FIRST_HOST_t;

// LOCA1_ERR_EN_BMC desc:
typedef union {
    struct {
        uint64_t         dcd_bank_mbe  :  8; // Enables corresponding LOCA1_ERR_STS bits to generate BMC interrupt signal.
        uint64_t         dcd_bank_sbe  :  8; // Enables corresponding LOCA1_ERR_STS bits to generate BMC interrupt signal.
        uint64_t        conq_tail_mbe  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate BMC interrupt signal.
        uint64_t        hifis_hdr_mbe  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate BMC interrupt signal.
        uint64_t        conq_tail_sbe  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate BMC interrupt signal.
        uint64_t        hifis_hdr_sbe  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate BMC interrupt signal.
        uint64_t        pmon_rollover  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate BMC interrupt signal.
        uint64_t     adm_malform_blen  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate BMC interrupt signal.
        uint64_t     adm_malform_addr  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate BMC interrupt signal.
        uint64_t      adm_malform_adt  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate BMC interrupt signal.
        uint64_t      adm_malform_aso  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate BMC interrupt signal.
        uint64_t       Reserved_63_25  : 39; // Unused
    } field;
    uint64_t val;
} LOCA1_ERR_EN_BMC_t;

// LOCA1_ERR_FIRST_BMC desc:
typedef union {
    struct {
        uint64_t         dcd_bank_mbe  :  8; // Snapshot of LOCA1_ERR_STS bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t         dcd_bank_sbe  :  8; // Snapshot of LOCA1_ERR_STS bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t        conq_tail_mbe  :  1; // Snapshot of LOCA1_ERR_STS bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t        hifis_hdr_mbe  :  1; // Snapshot of LOCA1_ERR_STS bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t        conq_tail_sbe  :  1; // Snapshot of LOCA1_ERR_STS bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t        hifis_hdr_sbe  :  1; // Snapshot of LOCA1_ERR_STS bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t        pmon_rollover  :  1; // Snapshot of LOCA1_ERR_STS bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t     adm_malform_blen  :  1; // Snapshot of LOCA1_ERR_STS bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t     adm_malform_addr  :  1; // Snapshot of LOCA1_ERR_STS bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t      adm_malform_adt  :  1; // Snapshot of LOCA1_ERR_STS bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t      adm_malform_aso  :  1; // Snapshot of LOCA1_ERR_STS bits when BMC interrupt signal transitions from 0 to 1.
        uint64_t       Reserved_63_25  : 39; // Unused
    } field;
    uint64_t val;
} LOCA1_ERR_FIRST_BMC_t;

// LOCA1_ERR_EN_QUAR desc:
typedef union {
    struct {
        uint64_t         dcd_bank_mbe  :  8; // Enables corresponding LOCA1_ERR_STS bits to generate quarantine interrupt signal.
        uint64_t         dcd_bank_sbe  :  8; // Enables corresponding LOCA1_ERR_STS bits to generate quarantine interrupt signal.
        uint64_t        conq_tail_mbe  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate quarantine interrupt signal.
        uint64_t        hifis_hdr_mbe  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate quarantine interrupt signal.
        uint64_t        conq_tail_sbe  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate quarantine interrupt signal.
        uint64_t        hifis_hdr_sbe  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate quarantine interrupt signal.
        uint64_t        pmon_rollover  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate quarantine interrupt signal.
        uint64_t     adm_malform_blen  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate quarantine interrupt signal.
        uint64_t     adm_malform_addr  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate quarantine interrupt signal.
        uint64_t      adm_malform_adt  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate quarantine interrupt signal.
        uint64_t      adm_malform_aso  :  1; // Enables corresponding LOCA1_ERR_STS bits to generate quarantine interrupt signal.
        uint64_t       Reserved_63_25  : 39; // Unused
    } field;
    uint64_t val;
} LOCA1_ERR_EN_QUAR_t;

// LOCA1_ERR_FIRST_QUAR desc:
typedef union {
    struct {
        uint64_t         dcd_bank_mbe  :  8; // Snapshot of LOCA1_ERR_STS bits when quarantine interrupt signal transitions from 0 to 1.
        uint64_t         dcd_bank_sbe  :  8; // Snapshot of LOCA1_ERR_STS bits when quarantine interrupt signal transitions from 0 to 1.
        uint64_t        conq_tail_mbe  :  1; // Snapshot of LOCA1_ERR_STS bits when quarantine interrupt signal transitions from 0 to 1.
        uint64_t        hifis_hdr_mbe  :  1; // Snapshot of LOCA1_ERR_STS bits when quarantine interrupt signal transitions from 0 to 1.
        uint64_t        conq_tail_sbe  :  1; // Snapshot of LOCA1_ERR_STS bits when quarantine interrupt signal transitions from 0 to 1.
        uint64_t        hifis_hdr_sbe  :  1; // Snapshot of LOCA1_ERR_STS bits when quarantine interrupt signal transitions from 0 to 1.
        uint64_t        pmon_rollover  :  1; // Snapshot of LOCA1_ERR_STS bits when quarantine interrupt signal transitions from 0 to 1.
        uint64_t     adm_malform_blen  :  1; // Snapshot of LOCA1_ERR_STS bits when quarantine interrupt signal transitions from 0 to 1.
        uint64_t     adm_malform_addr  :  1; // Snapshot of LOCA1_ERR_STS bits when quarantine interrupt signal transitions from 0 to 1.
        uint64_t      adm_malform_adt  :  1; // Snapshot of LOCA1_ERR_STS bits when quarantine interrupt signal transitions from 0 to 1.
        uint64_t      adm_malform_aso  :  1; // Snapshot of LOCA1_ERR_STS bits when quarantine interrupt signal transitions from 0 to 1.
        uint64_t       Reserved_63_25  : 39; // Unused
    } field;
    uint64_t val;
} LOCA1_ERR_FIRST_QUAR_t;

// LOCA1_VIRAL_ENABLE desc:
typedef union {
    struct {
        uint64_t         dcd_bank_mbe  :  8; // Enable corresponding LOCA0_ERR_STS bits to generate viral signaling.
        uint64_t         dcd_bank_sbe  :  8; // Enable corresponding LOCA0_ERR_STS bits to generate viral signaling.
        uint64_t        conq_tail_mbe  :  1; // Enable corresponding LOCA0_ERR_STS bits to generate viral signaling.
        uint64_t        hifis_hdr_mbe  :  1; // Enable corresponding LOCA0_ERR_STS bits to generate viral signaling.
        uint64_t        conq_tail_sbe  :  1; // Enable corresponding LOCA0_ERR_STS bits to generate viral signaling.
        uint64_t        hifis_hdr_sbe  :  1; // Enable corresponding LOCA0_ERR_STS bits to generate viral signaling.
        uint64_t        pmon_rollover  :  1; // Enable corresponding LOCA0_ERR_STS bits to generate viral signaling.
        uint64_t     adm_malform_blen  :  1; // Enable corresponding LOCA0_ERR_STS bits to generate viral signaling.
        uint64_t     adm_malform_addr  :  1; // Enable corresponding LOCA0_ERR_STS bits to generate viral signaling.
        uint64_t      adm_malform_adt  :  1; // Enable corresponding LOCA0_ERR_STS bits to generate viral signaling.
        uint64_t      adm_malform_aso  :  1; // Enable corresponding LOCA0_ERR_STS bits to generate viral signaling.
        uint64_t       Reserved_63_25  : 39; // Unused
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
        uint64_t          latency_ctl  :  2; // This field defines how memory latency is measured at the PMI interface. Latency measurement starts with the IDI request and ends with the configured response:00 - N/A01 - Measure latency until IDI Go response.10 - Measure latency until IDI Data response (if applicable).11 - Measure latency until the latter of Go or Data response.
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
        uint64_t            dcd_valid  :  1; // DCD Entry Valid
        uint64_t           pcam_state  :  4; // PCAM STATE:4'h0: Error4'h1: Fill4'h2: Allocated4'h3: Pending4'hF-4: Reserved
        uint64_t           pcam_spare  :  1; // PCAM Spare.
        uint64_t          pcam_rplflg  :  1; // PCAM Replay Flag. Set when HCC replay is pending.
        uint64_t          pcam_chint1  :  1; // PCAM Cache Hint[1]. Allocating hint bit.
        uint64_t           pcam_chain  :  4; // PCAM Chain. Chaining vector[3:0]. One bit for block.
        uint64_t         pcam_conqval  :  1; // PCAM ConQ Valid. ConQ has a request that is linked to the PCAM entry. Head/Tail pointer data is valid
        uint64_t          pcam_conqhp  :  8; // PCAM ConQ Head Pointer
        uint64_t          pcam_conqtp  :  8; // PCAM ConQ Tail Pointer
        uint64_t          pcam_proqid  :  9; // PCAM PROQ ID. PROQ ID associated with pending line request.
        uint64_t       pcam_proqsrply  :  1; // PCAM PROQ Schedule Replay. Pending line will have its associated request replayed by the PROQ.
        uint64_t           pcam_adr76  :  2; // PCAM Address[7:6].Address bits[7:6] for pending or allocated request.
        uint64_t       Reserved_63_44  : 20; // Unused
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
        uint64_t                state  :  4; // State: 0-Error, 1-Refetch, 2-Allocated, 3-Pending, 4-F Reserved.
        uint64_t                spare  :  1; // Spare entry
        uint64_t               rplflg  :  1; // Replay pending Flag
        uint64_t               chint1  :  1; // Cache Hint[1] (Allocating)
        uint64_t                chain  :  4; // Chain vector
        uint64_t              conqval  :  1; // ConQ entry Valid
        uint64_t               conqhp  :  8; // ConQ Head Pointer
        uint64_t               conqtp  :  8; // ConQ Tail Pointer
        uint64_t               proqid  :  9; // PROQ ID
        uint64_t            proqsrply  :  1; // PROQ Sourced Replay flag
        uint64_t                adr76  :  2; // Address bits[7:6]
        uint64_t       Reserved_63_40  : 24; // Unused
    } field;
    uint64_t val;
} LOCA_DBG_PCAM_DATA_t;

// LOCA_DBG_MRU desc:
typedef union {
    struct {
        uint64_t              pending  :  8; // Pending. Entry way has a cache line pending from the uncore.
        uint64_t                valid  :  8; // Valid. Entry way is valid in the Data Cache
        uint64_t                  mru  :  8; // MRU. Entry way is marked Most Recently Used.
        uint64_t       Reserved_31_24  :  8; // Unused
        uint64_t              victim0  :  8; // Victim0. One-hot ID of victimized entry way for next demand loaded cache line into the indexed set. Debug write data must be a valid one-hot value.
        uint64_t              victim1  :  8; // Victim1. One-hot ID of victimized entry way for next demand loaded cache line into the indexed set. Debug write data must be a valid one-hot value.
        uint64_t              victim2  :  8; // Victim2. One-hot ID of victimized entry way for next demand loaded cache line into the indexed set. Debug write data must be a valid one-hot value.
        uint64_t              victim3  :  8; // Victim3. One-hot ID of victimized entry way for next demand loaded cache line into the indexed set. Debug write data must be a valid one-hot value.
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
        uint64_t              conqval  : 64; // ConQ Valid state. When set, the replay pointer for the request in that entry is a ConQ pointer.
    } field;
    uint64_t val;
} LOCA_DBG_PROQ_CONQVAL_t;

// LOCA_DBG_PMI_ORB0 desc:
typedef union {
    struct {
        uint64_t                 orb0  : 64; // ORB state0
    } field;
    uint64_t val;
} LOCA_DBG_PMI_ORB0_t;

// LOCA_DBG_PMI_ORB1 desc:
typedef union {
    struct {
        uint64_t                 orb1  : 64; // ORB state1
    } field;
    uint64_t val;
} LOCA_DBG_PMI_ORB1_t;

// LOCA_DBG_PMI_ORBVAL desc:
typedef union {
    struct {
        uint64_t                valid  : 64; // ORB Entry Valid bits.
    } field;
    uint64_t val;
} LOCA_DBG_PMI_ORBVAL_t;

// LOCA_DBG_PMI_ORBHOLD desc:
typedef union {
    struct {
        uint64_t                 hold  : 64; // ORB Entry Hold bits.
    } field;
    uint64_t val;
} LOCA_DBG_PMI_ORBHOLD_t;

// LOCA_DBG_PMI_USB_GOVAL desc:
typedef union {
    struct {
        uint64_t              govalid  : 64; // U2C Response GO Valid bits.
    } field;
    uint64_t val;
} LOCA_DBG_PMI_USB_GOVAL_t;

// LOCA_DBG_PMI_DPBVAL desc:
typedef union {
    struct {
        uint64_t                valid  : 64; // DPB Entry Valid bits.
    } field;
    uint64_t val;
} LOCA_DBG_PMI_DPBVAL_t;

// LOCA_DBG_PMI_DRBVAL0 desc:
typedef union {
    struct {
        uint64_t                valid  : 64; // DRB0 Entry Valid bits.
    } field;
    uint64_t val;
} LOCA_DBG_PMI_DRBVAL0_t;

// LOCA_DBG_PMI_DRBVAL1 desc:
typedef union {
    struct {
        uint64_t                valid  : 64; // DRB1 Entry Valid bits.
    } field;
    uint64_t val;
} LOCA_DBG_PMI_DRBVAL1_t;

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

// LOCA_DBG_DCD0 desc:
typedef union {
    struct {
        uint64_t           byte_valid  : 64; // Byte Valid. Byte valid bits for the cache line data.
    } field;
    uint64_t val;
} LOCA_DBG_DCD0_t;

// LOCA_DBG_DCD1 desc:
typedef union {
    struct {
        uint64_t                  way  :  3; // Way. LOCA_DBG_DCD1 address bits[13:11]
        uint64_t         Reserved_5_3  :  3; // Unused
        uint64_t                 tag0  :  2; // TAG0. 2 bit tag that holds cached line address[7:6]
        uint64_t                 addr  :  7; // Address bits 14:8 associated with the entry. Unhashed calculation from LOCA_DBG_DCD1 address bits[10:4] and DCD Tag1/0 fields.
        uint64_t                 tag1  : 42; // TAG1. 42 bit tag that holds cached line address[56:15]
        uint64_t                valid  :  1; // Valid. DCD Entry Valid bit.
        uint64_t       Reserved_63_58  :  6; // Unused
    } field;
    uint64_t val;
} LOCA_DBG_DCD1_t;

// LOCA_DBG_GO_CPORB desc:
typedef union {
    struct {
        uint64_t              wrr_ptr  :  9; // Write RR Pointer. Pointer to the WRR FIFO entry associated with the pending read request.
        uint64_t        Reserved_11_9  :  3; // Unused
        uint64_t               gen_id  : 12; // Generation ID. Generation ID associated with the wrr_ptr field.
        uint64_t                valid  :  1; // Valid. Valid bit for the CPORB entry.
        uint64_t       Reserved_63_25  : 39; // Unused
    } field;
    uint64_t val;
} LOCA_DBG_GO_CPORB_t;

// LOCA_DBG_GO_WRR desc:
typedef union {
    struct {
        uint64_t              hfi_tid  : 12; // HFI Tid. Transaction ID of an RX sourced Write request awaiting a GO response from HI
        uint64_t       Reserved_63_12  : 52; // Unused
    } field;
    uint64_t val;
} LOCA_DBG_GO_WRR_t;

// LOCA_DBG_DCACHE desc:
typedef union {
    struct {
        uint64_t                 data  : 64; // Data
    } field;
    uint64_t val;
} LOCA_DBG_DCACHE_t;

#endif /* ___FXR_loca_CSRS_H__ */
