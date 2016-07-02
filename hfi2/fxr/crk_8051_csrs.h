// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Wed Jun 22 19:30:15 2016
//

#ifndef ___CRK_8051_CSRS_H__
#define ___CRK_8051_CSRS_H__

// CRK8051_CFG_RAM_ACCESS_SETUP desc:
typedef union {
    struct {
        uint64_t              RAM_SEL  :  1; // 0: data memory 1: code memory
        uint64_t           Unused_7_1  :  7; // Unused
        uint64_t       AUTO_INCR_ADDR  :  1; // When 1, during the write to the code memory, the address will be automatically increased by 8 after each 64 bit write to the data register
        uint64_t          Unused_63_9  : 55; // Unused
    } field;
    uint64_t val;
} CRK8051_CFG_RAM_ACCESS_SETUP_t;

// CRK8051_CFG_RAM_ACCESS_CTRL desc:
typedef union {
    struct {
        uint64_t              ADDRESS  : 16; // Provide the Address for the memory Access
        uint64_t             READ_ENA  :  1; // 1: read operation 0: not a read
        uint64_t         Unused_23_17  :  7; // Unused
        uint64_t            WRITE_ENA  :  1; // 1: write operation 0: not a write
        uint64_t         Unused_63_25  : 39; // Unused
    } field;
    uint64_t val;
} CRK8051_CFG_RAM_ACCESS_CTRL_t;

// CRK8051_CFG_RAM_ACCESS_WR_DATA desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // data to write, The least significant byte is bit[7:0].
    } field;
    uint64_t val;
} CRK8051_CFG_RAM_ACCESS_WR_DATA_t;

// CRK8051_CFG_RAM_ACCESS_STATUS desc:
typedef union {
    struct {
        uint64_t         AUTO_ADDRESS  : 16; // This register stores the current address value CRK8051 generate during auto_incr_addr mode
        uint64_t     ACCESS_COMPLETED  :  1; // 1: the result of read data is valid when reading, write operation completed when writing 0: read/write operation is ongoing
        uint64_t         Unused_63_17  : 47; // Unused
    } field;
    uint64_t val;
} CRK8051_CFG_RAM_ACCESS_STATUS_t;

// CRK8051_CFG_RAM_ACCESS_RD_DATA desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // data when reading, valid after access_completed is high, The least significant byte is bit[7:0].
    } field;
    uint64_t val;
} CRK8051_CFG_RAM_ACCESS_RD_DATA_t;

// CRK8051_CFG_HOST_CMD_0 desc:
typedef union {
    struct {
        uint64_t              REQ_NEW  :  1; // Set to 1 to issue a new request. set back to 0 when completed
        uint64_t           Unused_7_1  :  7; // Unused
        uint64_t             REQ_TYPE  :  8; // Specify the specific command
        uint64_t             REQ_DATA  : 48; // Data input when sending the command
    } field;
    uint64_t val;
} CRK8051_CFG_HOST_CMD_0_t;

// CRK8051_CFG_HOST_CMD_1 desc:
typedef union {
    struct {
        uint64_t            COMPLETED  :  1; // 0 to indicate 8051 has received the Host Request and is executing the request, 1 to indicate request is done, and response is available.
        uint64_t           Unused_7_1  :  7; // Unused
        uint64_t          RETURN_CODE  :  8; // return code for the execution of the command: 0x01: Invalid Command 0x02: Request Successfully executed 0xff: Request received, execution is ongoing others: Command specific
        uint64_t             RSP_DATA  : 48; // Data output if any when command completed.
    } field;
    uint64_t val;
} CRK8051_CFG_HOST_CMD_1_t;

// CRK8051_CFG_LOCAL_GUID desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // This CSR need to be updated before going into Polling State. When CRK is in 'non-secure' mode (CRK input pin i_top_crk8051_security_lock = 1), this CSR is blocked from write access, it is updated directly from CRK input pin i_top_crk8051_guid_secured[63:0] When CRK is in 'secure' mode ( CRK input pin i_top_crk8051_security_lock = 0), this CSR should be updated by writing to it directly. CRK input pin i_top_crk8051_guid_secured[63:0] is ignored.
    } field;
    uint64_t val;
} CRK8051_CFG_LOCAL_GUID_t;

// CRK8051_STS_REMOTE_GUID desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // GUID ID received from remote partner
    } field;
    uint64_t val;
} CRK8051_STS_REMOTE_GUID_t;

// CRK8051_STS_REMOTE_NODE_TYPE desc:
typedef union {
    struct {
        uint64_t                  VAL  :  2; // Node Type received from remote partner
        uint64_t          Unused_63_2  : 62; // Unused
    } field;
    uint64_t val;
} CRK8051_STS_REMOTE_NODE_TYPE_t;

// CRK8051_STS_LOCAL_FM_SECURITY desc:
typedef union {
    struct {
        uint64_t             DISABLED  :  1; // 1 to indicate that security check on CRK8051 firmware loaded has been disabled. This CSR need to be updated before going into Polling State. When CRK is in 'non-secure' mode (CRK input pin i_top_crk8051_security_lock = 1), this CSR is blocked from write access, it is updated directly from CRK input pin i_top_crk8051_fwvrfy_dsbld. When CRK is in 'secure' mode ( CRK input pin i_top_crk8051_security_lock = 0), this CSR should be updated by writing to it directly. CRK input pin i_top_crk8051_fwvfy_dsbld is ignored.
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} CRK8051_STS_LOCAL_FM_SECURITY_t;

// CRK8051_STS_REMOTE_FM_SECURITY desc:
typedef union {
    struct {
        uint64_t             DISABLED  :  1; // This is received information from remote peer, when set, it indicates that the firmware running on the remote peer has not been authenticated.
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} CRK8051_STS_REMOTE_FM_SECURITY_t;

// CRK8051_STS_CUR_STATE desc:
typedef union {
    struct {
        uint64_t                 PORT  :  8; // STL Mode PHY State -- 0x30 Disabled -- 0x90 Offline -- 0x90 Offline.Quiet -- 0x91 Offline.Planned_Down_Inform -- 0x92 Offline.Ready_To_Quiet_LT -- 0x93 Offline.Report_Failure -- 0x94 Offline.Ready_To_Quiet_BCC -- 0x20 Polling. -- 0x20 Polling.Quiet -- 0x21 Polling.Active -- 0x40 ConfigPhy -- 0x40 ConfigPhy.Debounce -- 0x41 ConfigPhy.EstComm -- 0x41 ConfigPhy.EstComm_TXRX_Hunt -- 0x42 ConfigPhy.EstComm_TX_Hunt -- 0x43 ConfigPhy.EstComm_Local_Complete -- 0x44 ConfigPhy.OptEq -- 0x44 ConfigPhy.OptEq_Optimizing -- 0x45 ConfigPhy.OptEq_Local_Complete -- 0x46 ConfigPhy.VerifyCap -- 0x46 ConfigPhy.VerifyCap_Exchange -- 0x47 ConfigPhy.VerifyCap_Local_Complete -- 0x48 ConfigLT -- 0x48 ConfigLT_Configure -- 0x49 ConfigLT_Link_Transfer_Active -- 0x50 LinkUp -- -- 0x80 Ganged -- 0xB0 PhyTest Other State: -- 0xe1 STL SerDes internal loopback mode LWEG Mode PHY State --0x30 Disabled --0x90 Offline --0x20 Polling --0x40 Config_AN --0x45 Config_LTR --0x50 LinkUp -- Others: RESERVED
        uint64_t            LINK_MODE  :  2; // CRK Link mode: --0x00: RESERVED --0x01: LCB mode --0x02: BCC mode --0x03: RESERVED
        uint64_t         Unused_15_10  :  6; // Unused
        uint64_t             FIRMWARE  :  8; // firmware State: -- 0xa0: firmware is ready to accept new HOST request -- 0xa1: firmware is processing request -- 0xa2: firmware is in LinkUp mode, LCB CSR access is active, monitoring LCB CSRs, Idle Protocol is active, CCFT is active if enabled. -- 0xa3: firmware is in LinkUp mode, LCB CSR access is not active. HOST can take away LCB CSR access permission -- others: RESERVED
        uint64_t      INTERRUPT_A_ACK  :  1; // Interrupt A Acknowledgment
        uint64_t      INTERRUPT_B_ACK  :  1; // Interrupt B Acknowledgment
        uint64_t         Unused_63_26  : 38; // Unused
    } field;
    uint64_t val;
} CRK8051_STS_CUR_STATE_t;

// CRK8051_CFG_RST desc:
typedef union {
    struct {
        uint64_t               M8051W  :  1; // reset the 8051 core
        uint64_t                 CRAM  :  1; // reset the 8051 code memory wrapper logic
        uint64_t                 DRAM  :  1; // reset the 8051 data memory wrapper logic
        uint64_t                 IRAM  :  1; // reset the 8051 internal data memory wrapper logic, when released, the internal ram initialization logic is activated to init the ram, it takes maximum of 260 cclk cycles to complete the process before it can be used by the 8051. During internal ram init time, the code ram can be loaded. If code ram loads faster than 256 cycles required. the M8051W reset need to hold until the requirement met.
        uint64_t                  SFR  :  1; // reset the custom SFR logic
        uint64_t          Unused_63_5  : 59; // Unused
    } field;
    uint64_t val;
} CRK8051_CFG_RST_t;

// CRK8051_CFG_MODE desc:
typedef union {
    struct {
        uint64_t              GENERAL  :  8; // General configuration Register. bit 0: when set, firmware can issue soft reset to LCB. bit 1: when set, self-GUID check is disabled. bit [7:2]: Reserved
        uint64_t         POWER_SAVING  :  2; // Used to direct the CRK8051 into different power saving mode 0x0: normal mode 0x1: idle mode 0x2: power down mode
        uint64_t         Unused_15_10  :  6; // Unused
        uint64_t      INTERRUPT_A_ENA  :  1; // Enable interrupt A
        uint64_t      INTERRUPT_B_ENA  :  1; // Enable interrupt B
        uint64_t         Unused_23_18  :  6; // Unused
        uint64_t           CLK_SELECT  :  2; // CRK_8051 clock divider configuration: 2'b00: cclk/16 2'b01: cclk/2 default 2'b10: cclk/4 2'b11: cclk/8
        uint64_t         Unused_63_26  : 38; // Unused
    } field;
    uint64_t val;
} CRK8051_CFG_MODE_t;

// CRK8051_DBG_SFR_MAP_1 desc:
typedef union {
    struct {
        uint64_t                TMP01  :  8; // TMP01 SFR mapping
        uint64_t                TMP02  :  8; // TMP02 SFR mapping
        uint64_t                TMP03  :  8; // TMP03 SFR mapping
        uint64_t                TMP04  :  8; // TMP04 SFR mapping
        uint64_t                TMP05  :  8; // TMP05 SFR mapping
        uint64_t                TMP06  :  8; // TMP06 SFR mapping
        uint64_t                TMP07  :  8; // TMP07 SFR mapping
        uint64_t                TMP08  :  8; // TMP08 SFR mapping
    } field;
    uint64_t val;
} CRK8051_DBG_SFR_MAP_1_t;

// CRK8051_DBG_SFR_MAP_2 desc:
typedef union {
    struct {
        uint64_t                TMP09  :  8; // TMP09 SFR mapping
        uint64_t                TMP10  :  8; // TMP10 SFR mapping
        uint64_t                TMP11  :  8; // TMP11 SFR mapping
        uint64_t                TMP12  :  8; // TMP12 SFR mapping
        uint64_t                TMP13  :  8; // TMP13 SFR mapping
        uint64_t                TMP14  :  8; // TMP14 SFR mapping
        uint64_t                TMP15  :  8; // TMP15 SFR mapping
        uint64_t                TMP16  :  8; // TMP16 SFR mapping
    } field;
    uint64_t val;
} CRK8051_DBG_SFR_MAP_2_t;

// CRK8051_DBG_HEART_BEAT desc:
typedef union {
    struct {
        uint64_t                  VAL  : 16; // The 8051 micro controller heart beat, this is the counter that counts up continuously while the firmware is running. This counter should increment at least once every 200us. When this counter stops increment, the error flag LOST_8051_HEART_BEAT should be set by the hardware.
        uint64_t         Unused_63_16  : 48; // Unused
    } field;
    uint64_t val;
} CRK8051_DBG_HEART_BEAT_t;

// CRK8051_DBG_CODE_TRACING_SETUP desc:
typedef union {
    struct {
        uint64_t       LOAD_CODE_ADDR  :  1; // When set, CRK8051_DBG_CODE_TRACING_ADDR CSR will be loaded with current address location that firmware is running at continuously.
        uint64_t           Unused_7_1  :  7; // Unused
        uint64_t     HALT_ON_ADDR_ENA  :  1; // when set, the 8051 micro controller will halt on the instruction address specified by HALT_ON_ADDR_ENA[13:0]
        uint64_t          Unused_15_9  :  7; // Unused
        uint64_t         HALT_ON_ADDR  : 16; // Address to halt on.
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} CRK8051_DBG_CODE_TRACING_SETUP_t;

// CRK8051_DBG_CODE_TRACING_ADDR desc:
typedef union {
    struct {
        uint64_t                  VAL  : 16; // This register tracks the code memory address the 8051 micro controller currently accessing. The tracking is enabled by LOAD_CODE ADDR bit.
        uint64_t         Unused_63_16  : 48; // Unused
    } field;
    uint64_t val;
} CRK8051_DBG_CODE_TRACING_ADDR_t;

// CRK8051_DBG_ERRINJ_ECC desc:
typedef union {
    struct {
        uint64_t               ENABLE  :  1; // Enable the Error Injection
        uint64_t                 MODE  :  2; // Mode 0: Inject error once, clear enable to rerun Mode 1: Inject error always Mode 2: Inject error once if address matches, clear enable to rerun. Mode 3: Inject error always if address matches
        uint64_t           RAM_SELECT  :  2; // 0x0: data memory 0x1: code memory 0x2: internal data memory
        uint64_t           Unused_7_5  :  3; // Unused
        uint64_t              ADDRESS  : 12; // This field indicates the address for which error injection is to occur. This field is used only when the error injection mode is 2 or 3.
        uint64_t                 MASK  :  9; // When an error is injected, each bit that is set to one in this field causes the corresponding bit of the error detection syndrome for the memory address read to be inverted.
        uint64_t         Unused_39_33  :  7; // Unused
        uint64_t          CLEAR_ERROR  :  1; // When asserted, clear all the error counters, Error Info is not cleared by this.
        uint64_t         Unused_63_41  : 23; // Unused
    } field;
    uint64_t val;
} CRK8051_DBG_ERRINJ_ECC_t;

// CRK8051_DBG_ERR_INFO_CRAM desc:
typedef union {
    struct {
        uint64_t                  SBE  :  1; // indicates SBE occurred
        uint64_t                  MBE  :  1; // indicates MBE occurred
        uint64_t         INJECTED_SBE  :  1; // indicates SBE occurred due to error injection
        uint64_t         INJECTED_MBE  :  1; // indicated MBE occurred due to error injection
        uint64_t           Unused_7_4  :  4; // Unused
        uint64_t             SBE_SYND  :  9; // SBE syndrome
        uint64_t         Unused_23_17  :  7; // Unused
        uint64_t             MBE_SYND  :  9; // MBE syndrome
        uint64_t         Unused_63_33  : 31; // Unused
    } field;
    uint64_t val;
} CRK8051_DBG_ERR_INFO_CRAM_t;

// CRK8051_DBG_ERR_INFO_DRAM desc:
typedef union {
    struct {
        uint64_t                  SBE  :  1; // indicates SBE occurred
        uint64_t                  MBE  :  1; // indicates MBE occurred
        uint64_t         INJECTED_SBE  :  1; // indicates SBE occurred due to error injection
        uint64_t         INJECTED_MBE  :  1; // indicated MBE occurred due to error injection
        uint64_t           Unused_7_4  :  4; // Unused
        uint64_t             SBE_SYND  :  5; // SBE syndrome
        uint64_t         Unused_15_13  :  3; // Unused
        uint64_t             MBE_SYND  :  5; // MBE syndrome
        uint64_t         Unused_63_21  : 43; // Unused
    } field;
    uint64_t val;
} CRK8051_DBG_ERR_INFO_DRAM_t;

// CRK8051_DBG_ERR_INFO_IRAM desc:
typedef union {
    struct {
        uint64_t                  SBE  :  1; // indicates SBE occurred
        uint64_t                  MBE  :  1; // indicates MBE occurred
        uint64_t         INJECTED_SBE  :  1; // indicates SBE occurred due to error injection
        uint64_t         INJECTED_MBE  :  1; // indicated MBE occurred due to error injection
        uint64_t           Unused_7_4  :  4; // Unused
        uint64_t             SBE_SYND  :  5; // SBE syndrome
        uint64_t         Unused_15_13  :  3; // Unused
        uint64_t             MBE_SYND  :  5; // MBE syndrome
        uint64_t         Unused_63_21  : 43; // Unused
    } field;
    uint64_t val;
} CRK8051_DBG_ERR_INFO_IRAM_t;

// CRK8051_DBG_ERR_INFO_CNT_CRAM desc:
typedef union {
    struct {
        uint64_t            SBE_COUNT  : 32; // Number of SBE errors
        uint64_t            MBE_COUNT  : 32; // Number of MBE errors
    } field;
    uint64_t val;
} CRK8051_DBG_ERR_INFO_CNT_CRAM_t;

// CRK8051_DBG_ERR_INFO_CNT_DRAM desc:
typedef union {
    struct {
        uint64_t            SBE_COUNT  : 32; // Number of SBE errors
        uint64_t            MBE_COUNT  : 32; // Number of MBE errors
    } field;
    uint64_t val;
} CRK8051_DBG_ERR_INFO_CNT_DRAM_t;

// CRK8051_DBG_ERR_INFO_CNT_IRAM desc:
typedef union {
    struct {
        uint64_t            SBE_COUNT  : 32; // Number of SBE errors
        uint64_t            MBE_COUNT  : 32; // Number of MBE errors
    } field;
    uint64_t val;
} CRK8051_DBG_ERR_INFO_CNT_IRAM_t;

// CRK8051_DBG_ERR_INFO_SET_BY_8051 desc:
typedef union {
    struct {
        uint64_t             HOST_MSG  : 16; // When error flag SET_BY_8051 is set this info CSR is used to indicate type of HOST interrupt message is available, bit maskable, active when set. STL Mode Host Messages: bit[0]: HOST_REQ_DONE, This flag is used to indicate to HOST that the HOST command requested has been completed by the 8051. bit[1]: Received a back channel msg using LCB Idle protocol of Host Msg Type PWR_MGM. bit[2]: Received a back channel msg using LCB idle protocol Host Msg Type SMA. bit[3]: Received a back channel msg using BCC of Unknown Frame Type. bit[4]: Received a back channel msg using LCB Idle protocol of Unknown Frame Type.0 bit[5]: EXT_DEVICE_CFG_REQ. bit[6]: VerifyCap all frame received. bit[7]: LinkUp achieved. bit[8]: Link going down. bit[9]: Link Width Downgraded. LWEG Mode Host Messages: bit[0] HOST_REQ_DONE, This flag is used to indicate to HOST that the HOST command requested has been completed by the 8051. bit[6:2] Reserved bit[7]: LinkUp achieved Others: Reserved.
        uint64_t                ERROR  : 32; // Bit masked, when set, an error condition has occur STL Mode Error Status bit[0]: RESERVED bit[1]: UNKNOWN_FRAME type. bit[2]: TARGET BER NOT MET bit[3]: SerDes Internal Loop back failure. bit[4]: Failed SerDes Init. bit[5]: Failed LNI(Polling) bit[6]: Failed LNI(Debouce) bit[7]: Failed LNI(EstbComm) bit[8]: Failed LNI(OptEq) bit[9]: Failed LNI(VerifyCap_1) bit[10]: Failed LNI(VerifyCap_2) bit[11]: Failed LNI(ConfigLT) bit[12]: HOST handshake timed out LWEG Mode Error Status bit[3:0] Reserved bit[4]: Failed SerDes Init. bit[5]: Failed AN bit[6]: Failed LTR bit[7]: LOS( Lossing PLL Lock2Data while in LinkUp) Others: Reserved.
        uint64_t         Unused_63_48  : 16; // Unused
    } field;
    uint64_t val;
} CRK8051_DBG_ERR_INFO_SET_BY_8051_t;

// CRK8051_ERR_STS desc:
typedef union {
    struct {
        uint64_t          SET_BY_8051  :  1; // Indicating an error has been reported by the firmware or the firmware has sent HOST a message. Refer to CRK8051_DBG_ERR_INFO_SET_BY_8051 for detail error code or message type.
        uint64_t LOST_8051_HEART_BEAT  :  1; // Indicating 8051 micro controller has lost heart beat. Fatal error, action needed by the software to potentially reset the 8051 and the port.
        uint64_t             CRAM_MBE  :  1; // Code memory MBE. Potentially fatal error, MBE is not corrected by the hardware.
        uint64_t             CRAM_SBE  :  1; // Code memory SBE. Information only error, SBE is corrected by the hardware.
        uint64_t             DRAM_MBE  :  1; // Data memory MBE. Potentially fatal error, MBE is not corrected by the hardware.
        uint64_t             DRAM_SBE  :  1; // Data memory SBE. Information only error, SBE is corrected by the hardware.
        uint64_t             IRAM_MBE  :  1; // Internal Data memory MBE. Potentially fatal error, MBE is not corrected by the hardware.
        uint64_t             IRAM_SBE  :  1; // Internal Data memory SBE. Information only error, SBE is corrected by the hardware.
        uint64_t UNMATCHED_SECURE_MSG_ACROSS_BCC_LANES  :  1; // The remote GUID/Node Type/Firmware Security Status received from all 4 BCCs (or all enabled BCCs) are not the same. Information only error, there are redundant data integrity checks implemented in the CRK8051 hardware and firmware.
        uint64_t          Unused_63_9  : 55; // Unused
    } field;
    uint64_t val;
} CRK8051_ERR_STS_t;

// CRK8051_ERR_CLR desc:
typedef union {
    struct {
        uint64_t          SET_BY_8051  :  1; // when written as one, clears the corresponding bit in the CRK8051_ERR_STS
        uint64_t LOST_8051_HEART_BEAT  :  1; // when written as one, clears the corresponding bit in the CRK8051_ERR_STS
        uint64_t             CRAM_MBE  :  1; // when written as one, clears the corresponding bit in the CRK8051_ERR_STS
        uint64_t             CRAM_SBE  :  1; // when written as one, clears the corresponding bit in the CRK8051_ERR_STS
        uint64_t             DRAM_MBE  :  1; // when written as one, clears the corresponding bit in the CRK8051_ERR_STS
        uint64_t             DRAM_SBE  :  1; // when written as one, clears the corresponding bit in the CRK8051_ERR_STS
        uint64_t             IRAM_MBE  :  1; // when written as one, clears the corresponding bit in the CRK8051_ERR_STS
        uint64_t             IRAM_SBE  :  1; // when written as one, clears the corresponding bit in the CRK8051_ERR_STS
        uint64_t UNMATCHED_SECURE_MSG_ACROSS_BCC_LANES  :  1; // when written as one, clears the corresponding bit in the CRK8051_ERR_STS
        uint64_t          Unused_63_9  : 55; // Unused
    } field;
    uint64_t val;
} CRK8051_ERR_CLR_t;

// CRK8051_ERR_FRC desc:
typedef union {
    struct {
        uint64_t          SET_BY_8051  :  1; // see error flags
        uint64_t LOST_8051_HEART_BEAT  :  1; // see error flags
        uint64_t             CRAM_MBE  :  1; // see error flags
        uint64_t             CRAM_SBE  :  1; // see error flags
        uint64_t             DRAM_MBE  :  1; // see error flags
        uint64_t             DRAM_SBE  :  1; // see error flags
        uint64_t             IRAM_MBE  :  1; // see error flags
        uint64_t             IRAM_SBE  :  1; // see error flags
        uint64_t UNMATCHED_SECURE_MSG_ACROSS_BCC_LANES  :  1; // see error flags
        uint64_t          Unused_63_9  : 55; // Unused
    } field;
    uint64_t val;
} CRK8051_ERR_FRC_t;

// CRK8051_ERR_EN_HOST desc:
typedef union {
    struct {
        uint64_t          SET_BY_8051  :  1; // see error flags
        uint64_t LOST_8051_HEART_BEAT  :  1; // see error flags
        uint64_t             CRAM_MBE  :  1; // see error flags
        uint64_t             CRAM_SBE  :  1; // see error flags
        uint64_t             DRAM_MBE  :  1; // see error flags
        uint64_t             DRAM_SBE  :  1; // see error flags
        uint64_t             IRAM_MBE  :  1; // see error flags
        uint64_t             IRAM_SBE  :  1; // see error flags
        uint64_t UNMATCHED_SECURE_MSG_ACROSS_BCC_LANES  :  1; // see error flags
        uint64_t          Unused_63_9  : 55; // Unused
    } field;
    uint64_t val;
} CRK8051_ERR_EN_HOST_t;

// CRK8051_ERR_FIRST_HOST desc:
typedef union {
    struct {
        uint64_t          SET_BY_8051  :  1; // see error flags
        uint64_t LOST_8051_HEART_BEAT  :  1; // see error flags
        uint64_t             CRAM_MBE  :  1; // see error flags
        uint64_t             CRAM_SBE  :  1; // see error flags
        uint64_t             DRAM_MBE  :  1; // see error flags
        uint64_t             DRAM_SBE  :  1; // see error flags
        uint64_t             IRAM_MBE  :  1; // see error flags
        uint64_t             IRAM_SBE  :  1; // see error flags
        uint64_t UNMATCHED_SECURE_MSG_ACROSS_BCC_LANES  :  1; // see error flags
        uint64_t          Unused_63_9  : 55; // Unused
    } field;
    uint64_t val;
} CRK8051_ERR_FIRST_HOST_t;

// CRK8051_CFG_MISC_FUSE desc:
typedef union {
    struct {
        uint64_t                  VAL  :  8; // Reserved
        uint64_t          Unused_63_8  : 56; // Unused
    } field;
    uint64_t val;
} CRK8051_CFG_MISC_FUSE_t;

// CRK8051_CFG_SERDES desc:
typedef union {
    struct {
        uint64_t SER_BYPASS_INPUT_FLOP  :  1; // There's a rank of flops inserted in the oc_serdes at the parallel data input to the SerDes, these flops are clocked by the tx_fifo_clk. When this CSR is set, this rank of the flops is bypassed, and data from the link logic (LCB/BCC) is routed to the SerDes input directly. When in bypass mode, the phase beacon from the link logic is used instead of the one generated inside the oc_serdes block.
        uint64_t           Unused_7_1  :  7; // Unused
        uint64_t  SER_TX_FIFO_CLK_SEL  :  2; // Select which tx_fifo_clk to use the capture the parallel input data and generate the phase beacon. 2'b00: tx_fifo_clk from SerDes lane 0 is used. 2'b01: tx_fifo_clk from SerDes lane 1 is used. 2'b10: tx_fifo_clk from SerDes lane 2 is used. 2'b11: tx_fifo_clk from SerDes lane 3 is used.
        uint64_t         Unused_63_10  : 54; // Unused
    } field;
    uint64_t val;
} CRK8051_CFG_SERDES_t;

// CRK8051_CFG_EXT_DEV_0 desc:
typedef union {
    struct {
        uint64_t            COMPLETED  :  1; // 0 to indicate HOST has received the external device config. request from the 8051. 1 to indicate request has been executed, result is available
        uint64_t           Unused_7_1  :  7; // Unused
        uint64_t          RETURN_CODE  :  8; // Return code for the Host Response 0x01: Invalid Command. 0x02: Request Successfully executed. 0x03: Request not supported. 0xfe: Request rejected, stop further request. 0xff: Request received, execution is ongoing others: Request specific
        uint64_t             RSP_DATA  : 16; // Data for the Host Response
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} CRK8051_CFG_EXT_DEV_0_t;

// CRK8051_CFG_EXT_DEV_1 desc:
typedef union {
    struct {
        uint64_t              REQ_NEW  :  1; // Controlled by the 8051 firmware, 0, no request, 1, new request.
        uint64_t           Unused_7_1  :  7; // Unused
        uint64_t             REQ_TYPE  :  8; // Type of Request
        uint64_t             REQ_DATA  : 16; // Data for the Request
        uint64_t         Unused_63_32  : 32; // Unused
    } field;
    uint64_t val;
} CRK8051_CFG_EXT_DEV_1_t;

// CRK8051_CFG_LOCAL_PORT_NO desc:
typedef union {
    struct {
        uint64_t                  VAL  :  8; // local port number This CSR need to be updated before going into Polling State. When CRK is in 'non-secure' mode (CRK input pin i_top_crk8051_security_lock = 1), this CSR is blocked from write access, it is updated directly from CRK input pin i_top_crk8051_port_no_secured[7:0] When CRK is in 'secure' mode ( CRK input pin i_top_crk8051_security_lock = 0), this CSR should be updated by writing to it directly. CRK input pin i_top_crk8051_port_no_secured[7:0] is ignored
        uint64_t          Unused_63_8  : 56; // Unused
    } field;
    uint64_t val;
} CRK8051_CFG_LOCAL_PORT_NO_t;

// CRK8051_STS_REMOTE_PORT_NO desc:
typedef union {
    struct {
        uint64_t                  VAL  :  8; // received remote port number
        uint64_t          Unused_63_8  : 56; // Unused
    } field;
    uint64_t val;
} CRK8051_STS_REMOTE_PORT_NO_t;

// CRK8051_CFG_SER_LANES_DISABLE desc:
typedef union {
    struct {
        uint64_t                  VAL  :  8; // Lane mask for manually disable one or more lanes. Bit[0]: when set disable lane 0 receiver; Bit[1]: when set disable lane 1 receiver; Bit[2]: when set disable lane 2 receiver; Bit[3]: when set disable lane 3 receiver; Bit[4]: when set disable lane 0 transmitter; Bit[5]: when set disable lane 1 transmitter; Bit[6]: when set disable lane 2 transmitter; Bit[7]: when set disable lane 3 transmitter; In the case where not all 4 lanes need to be disabled. Lane 0 transimitter should not be disabled.
        uint64_t          Unused_63_8  : 56; // Unused
    } field;
    uint64_t val;
} CRK8051_CFG_SER_LANES_DISABLE_t;

// CRK8051_DBG_SPARE desc:
typedef union {
    struct {
        uint64_t                   d0  :  8; // Spare 0
        uint64_t                   d1  :  8; // Spare 1
        uint64_t         Unused_63_16  : 48; // Unused
    } field;
    uint64_t val;
} CRK8051_DBG_SPARE_t;

// CRK8051_DBG_SER_LOS desc:
typedef union {
    struct {
        uint64_t                  VAL  :  1; // When Set, indicates loss of the signal detected by any of 4 SerDes receivers. Can be written in debug mode only.
        uint64_t          Unused_63_1  : 63; // Unused
    } field;
    uint64_t val;
} CRK8051_DBG_SER_LOS_t;

// CRK8051_DBG_SER_OUTPUT0 desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // SerDes Output 0
    } field;
    uint64_t val;
} CRK8051_DBG_SER_OUTPUT0_t;

// CRK8051_DBG_SER_OUTPUT1 desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // SerDes Output 1
    } field;
    uint64_t val;
} CRK8051_DBG_SER_OUTPUT1_t;

// CRK8051_DBG_SER_OUTPUT2 desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // SerDes Output 2
    } field;
    uint64_t val;
} CRK8051_DBG_SER_OUTPUT2_t;

// CRK8051_DBG_SER_OUTPUT3 desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // SerDes Output 3
    } field;
    uint64_t val;
} CRK8051_DBG_SER_OUTPUT3_t;

// CRK8051_DBG_SER_INPUT0 desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // SerDes Input 0
    } field;
    uint64_t val;
} CRK8051_DBG_SER_INPUT0_t;

// CRK8051_DBG_SER_INPUT1 desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // SerDes Input 1
    } field;
    uint64_t val;
} CRK8051_DBG_SER_INPUT1_t;

// CRK8051_DBG_SER_INPUT2 desc:
typedef union {
    struct {
        uint64_t                  VAL  : 64; // SerDes Input 2
    } field;
    uint64_t val;
} CRK8051_DBG_SER_INPUT2_t;

// CRK8051_DBG_SER_MEMBUS_IF desc:
typedef union {
    struct {
        uint64_t                 ADDR  : 16; // Memory address when read/write.
        uint64_t           WRITE_DATA  :  8; // Data to write when write.
        uint64_t              NEW_REQ  :  1; // Set to 1 to initiate a new access request, self clear.
        uint64_t               WR_ENA  :  1; // Set to 1 for write access, otherwise read.
        uint64_t              SEL_PIN  :  1; // when set, this interface can be used to access some of the SerDes IP Pins
        uint64_t         UNUSED_31_27  :  5; // UNUSED
        uint64_t            READ_DATA  :  8; // Read result.
        uint64_t      ACCESS_COMPLETE  :  1; // Indicate access completed.
        uint64_t         ACCESS_ERROR  :  1; // Indicated access error.
        uint64_t         UNUSED_63_42  : 22; // UNUSED
    } field;
    uint64_t val;
} CRK8051_DBG_SER_MEMBUS_IF_t;

#endif /* ___CRK_8051_CSRS_H__ */