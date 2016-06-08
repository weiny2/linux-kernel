// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Thu Jun  2 19:11:24 2016
//

#ifndef ___MNH_opio_CSRS_H__
#define ___MNH_opio_CSRS_H__

// DWNSTRM_CFG_FRAMING_RESET desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // Reset the downstream (towards the serdes) datapath.
        uint64_t          Unused_63_1  : 63; // 
    } field;
    uint64_t val;
} DWNSTRM_CFG_FRAMING_RESET_t;

// UPSTRM_CFG_FIFOS_RADR desc:
typedef union {
    struct {
        uint64_t              RST_VAL  :  3; // 
        uint64_t          Unused_63_3  : 61; // Unused
    } field;
    uint64_t val;
} UPSTRM_CFG_FIFOS_RADR_t;

// OPIO_PHY_CFG_IRDY desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // Link Layer to Physical Layer indicating Link Layer is ready to transfer data
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} OPIO_PHY_CFG_IRDY_t;

// OPIO_PHY_CFG_PRI desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // MNH does not use this. Link Layer to Physical Layer indicating Link Layer is ready to transfer data with low priority (When 1). This is useful when running in multi-protocol case where the link layer can indicate the priority of the request.
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} OPIO_PHY_CFG_PRI_t;

// OPIO_PHY_CFG_STATE_REQ desc:
typedef union {
    struct {
        uint64_t                  VAL  :  4; // MNH will stay in Active mode. Link Layer Request to Physical Layer to request state change. Encodings as follows: 0000: Active 0001: Deepest Allowable PM State 0010: Reserved 0100: Idle_L1.1 0101: Idle_L1.2 0110: Idle_L1.3 0111: Idle_L1.4 1101: Retrain 1110: Disable 1111: Sleep_L2 All other encodings are reserved.
        uint64_t          Unused_63_4  : 60; // Unused
    } field;
    uint64_t val;
} OPIO_PHY_CFG_STATE_REQ_t;

// OPIO_PHY_CFG_STREAM desc:
typedef union {
    struct {
        uint64_t                   ID  :  8; // MNH does not use this.
        uint64_t          Unused_63_8  : 56; // Unused
    } field;
    uint64_t val;
} OPIO_PHY_CFG_STREAM_t;

// OPIO_PHY_CFG_VALID desc:
typedef union {
    struct {
        uint64_t                  VAL  :  2; // MNH will clear val[1] to save power in cluster 1 when the upper lanes on both ports are disabled.
        uint64_t          Unused_63_2  : 62; // Unused
    } field;
    uint64_t val;
} OPIO_PHY_CFG_VALID_t;

// OPIO_PHY_CFG_RESET desc:
typedef union {
    struct {
        uint64_t    LPHY_STICKY_RST_B  :  1; // #2 SIP opi_pwrgood_rst_b used for sticky registers.
        uint64_t       LPHY_DFX_RST_B  :  1; // SIP opi_fdfx_rst_b VISA connect to powergood_rst_b
        uint64_t     LPHY_DFX_PWRGOOD  :  1; // #1 SIP rlvs_fdfx_powergood dfx, connect to powergood_rst_b
        uint64_t             Unused_3  :  1; // Unused
        uint64_t LPHY_SIDE_EPOINT_RST_B  :  1; // #3 SIP opi_side_rst_b[0] LPHY sideband endpoint reset
        uint64_t           Unused_7_5  :  3; // Unused
        uint64_t LPHY_CDIE_BRIDGE_RST_B  :  1; // This does not drive anything. The RTL drives SIP opi_side_rst_b[1] companion die sideband bridge reset directly
        uint64_t          Unused_11_9  :  3; // Unused
        uint64_t      LPHY_PRIM_RST_B  :  1; // #4 SIP opi_prim_rst_b Chassis Prim reset
        uint64_t         Unused_15_13  :  3; // Unused
        uint64_t      LPHY_PGCB_RST_B  :  1; // #1 SIP opi_pgcb_rst_b Chassis PGCB Reset (Used by IP Inaccessible Flow)
        uint64_t         Unused_19_17  :  3; // Unused
        uint64_t            LPHY_WAKE  :  1; // 
        uint64_t         Unused_31_21  : 11; // Unused
        uint64_t    APHY_GPSB_PWRGOOD  :  1; // HIP iopi_vccsyncsb_powergood
        uint64_t    APHY_CORE_PWRGOOD  :  1; // HIP iopi_vcccore_powergood Pirm Well VR is On and stable
        uint64_t   APHY_LSMSB_PWRGOOD  :  1; // HIP iopi_vccasyncsb_powergood async sideband voltage
        uint64_t APHY_AOPI_PWRGOOD_RST_B  :  1; // HIP iopi_vccaopi_powergood_rst_b Reset for logic on VCCIO
        uint64_t APHY_AOPI_PWRGOOD_PRIM  :  1; // HIP iopi_vccaopi_powergood_prim
        uint64_t APHY_AOPI_PWRGOOD_INF  :  1; // HIP iopi_vccaopi_powergood_inf
        uint64_t APHY_AOPI_PWRGOOD_CK  :  1; // HIP iopi_vccaopi_powergood_ck
        uint64_t     APHY_DFX_PWRGOOD  :  1; // HIP iopi_fdfx_powergood
        uint64_t      APHY_GPSB_RST_B  :  1; // HIP iopi_side_reset_b
        uint64_t         Unused_43_41  :  3; // Unused
        uint64_t      APHY_LANE_RST_B  :  1; // HIP iopi_lane_reset_b Data/Valid/ECC/Strobe
        uint64_t         Unused_47_45  :  3; // Unused
        uint64_t      APHY_CMLN_RST_B  :  1; // HIP iopi_cmln_reset_b Common Lane logic
        uint64_t         Unused_51_49  :  3; // Unused
        uint64_t APHY_APLL_PWRGOOD_CK  :  1; // iopi_vccapllopi_powergood_ck
        uint64_t    APHY_APLL_PWRGOOD  :  1; // iopi_vccapllopi_powergood
        uint64_t         Unused_55_54  :  2; // Unused
        uint64_t    APHY_FP_CRI_RST_B  :  1; // iopi_fusepush_cri_rst_b
        uint64_t APHY_FP_CRI_PWRGOOD_RST_B  :  1; // iopi_fusepush_cri_powergoodrst_b
        uint64_t         Unused_59_58  :  2; // Unused
        uint64_t            APHY_WAKE  :  1; // 
        uint64_t         Unused_63_61  :  3; // Unused
    } field;
    uint64_t val;
} OPIO_PHY_CFG_RESET_t;

// OPIO_PHY_ERR_STS desc:
typedef union {
    struct {
        uint64_t                ERROR  :  1; // Physical layer detected an error (framing or training)
        uint64_t               CERROR  :  1; // Physical layer detected and fixed a correctable error
        uint64_t           TRAINERROR  :  1; // Physical layer training failed
        uint64_t          Unused_63_3  : 61; // Unused
    } field;
    uint64_t val;
} OPIO_PHY_ERR_STS_t;

// OPIO_PHY_ERR_CLR desc:
typedef union {
    struct {
        uint64_t                ERROR  :  1; // see OPIO_PHY_ERR_STS
        uint64_t               CERROR  :  1; // see OPIO_PHY_ERR_STS
        uint64_t           TRAINERROR  :  1; // Physical layer training failed
        uint64_t          Unused_63_3  : 61; // Unused
    } field;
    uint64_t val;
} OPIO_PHY_ERR_CLR_t;

// OPIO_PHY_ERR_FRC desc:
typedef union {
    struct {
        uint64_t                ERROR  :  1; // see OPIO_PHY_ERR_STS
        uint64_t               CERROR  :  1; // see OPIO_PHY_ERR_STS
        uint64_t           TRAINERROR  :  1; // Physical layer training failed
        uint64_t          Unused_63_3  : 61; // Unused
    } field;
    uint64_t val;
} OPIO_PHY_ERR_FRC_t;

// OPIO_PHY_ERR_EN_HOST desc:
typedef union {
    struct {
        uint64_t                ERROR  :  1; // see OPIO_PHY_ERR_STS
        uint64_t               CERROR  :  1; // see OPIO_PHY_ERR_STS
        uint64_t           TRAINERROR  :  1; // Physical layer training failed
        uint64_t          Unused_63_3  : 61; // Unused
    } field;
    uint64_t val;
} OPIO_PHY_ERR_EN_HOST_t;

// OPIO_PHY_ERR_FIRST_HOST desc:
typedef union {
    struct {
        uint64_t                ERROR  :  1; // see OPIO_PHY_ERR_STS
        uint64_t               CERROR  :  1; // see OPIO_PHY_ERR_STS
        uint64_t           TRAINERROR  :  1; // Physical layer training failed
        uint64_t          Unused_63_3  : 61; // Unused
    } field;
    uint64_t val;
} OPIO_PHY_ERR_FIRST_HOST_t;

// OPIO_PHY_STS_EXIT_CG_REQ desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // When asserted, requests Upper level protocol stacks to exit clock gated state ASAP.
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} OPIO_PHY_STS_EXIT_CG_REQ_t;

// OPIO_PHY_STS_STALLREQ desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // Physical Layer request to Link Layer to align packets at LPIF width boundary
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} OPIO_PHY_STS_STALLREQ_t;

// OPIO_PHY_STS_STATE_STS desc:
typedef union {
    struct {
        uint64_t                  VAL  :  4; // Physical Layer to Link Layer Status indication. Encodings as follows: 0000: Active 0001: Deepest Allowable PM State 0010: REQ_NAK 0100: Idle_L1.1 0101: Idle_L1.2 0110: Idle_L1.3 0111: Idle_L1.4 1101: Retrain 1110: Disable 1111: Sleep_L2 All other encodings are reserved.
        uint64_t          Unused_63_4  : 60; // Unused
    } field;
    uint64_t val;
} OPIO_PHY_STS_STATE_STS_t;

// OPIO_PHY_STS_STREAM desc:
typedef union {
    struct {
        uint64_t                   ID  :  8; // Physical Layer to Link Layer indicating the stream ID received with data
        uint64_t          Unused_63_8  : 56; // Unused
    } field;
    uint64_t val;
} OPIO_PHY_STS_STREAM_t;

// OPIO_PHY_STS_TRDY desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // Physical Layer is ready to accept data, data is accepted by Physical layer when Pl_trdy and Lp_valid are both asserted
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} OPIO_PHY_STS_TRDY_t;

// OPIO_PHY_STS_VALID desc:
typedef union {
    struct {
        uint64_t                  VAL  :  2; // Indicates which valid vs. Idle data. Valid de-asserted indicates idle data. This is used by MNH configuration only where LPIF_TXVALID_WIDTH==2 and this must match with NCLUSTER value.
        uint64_t          Unused_63_2  : 62; // Unused
    } field;
    uint64_t val;
} OPIO_PHY_STS_VALID_t;

// OPIO_PHY_STS_PLL_LOCK desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // APHY PLL is locked.
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} OPIO_PHY_STS_PLL_LOCK_t;

#endif /* ___MNH_opio_CSRS_H__ */