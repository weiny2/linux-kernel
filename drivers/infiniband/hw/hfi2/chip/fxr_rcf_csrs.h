// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Wed Apr 11 12:49:08 2018
//

#ifndef ___FXR_rcf_CSRS_H__
#define ___FXR_rcf_CSRS_H__

// RCF_MAN_EFUSE0 desc:
typedef union {
    struct {
        uint64_t         EfuseManInfo  : 63; // Manufacture tracking information (to include fab, lot, wafer, x/y co-ordinates) in a format that can be decoded by host software. This information is defined in the EFUSE map from the ASIC manufacturing partner. In summary: - [6:0]: fab designation (7-bit ASCII code: G-fab6, Q-fab8, N-fab12, P-fab14, T-fab15) - [13:7]: lot designation(7-bit ASCII code) - [41:14]: lot number (4 x 7-bit ASCII code) - [46:42] wafer number (5-bit binary) - 47: x sign (0 = positive x, 1 = negative x) - [54:48]: x coordinate (7-bit binary) - 55: y sign (0 = positive y, 1 = negative y) - [62:56]: y coordinate (7-bit binary)
        uint64_t          Reserved_63  :  1; // Reserved
    } field;
    uint64_t val;
} RCF_MAN_EFUSE0_t;

// RCF_MAN_EFUSE1 desc:
typedef union {
    struct {
        uint64_t           Reserved_0  :  1; // Reserved
        uint64_t EfuseOscillatorCountId  : 12; // binary number from PMRO measurement
        uint64_t       Reserved_37_13  : 25; // Reserved
        uint64_t   EfuseVoltageOffset  :  4; // 4-bit binary value used for VID programming. These 4bits will specify the VDD offset to be programmed in the external voltage regulator (VRM) for this ASIC instance. The exact voltages and the definition of these bits will be determined during silicon characterization.
        uint64_t    Customer_Reserved  :  8; // 
        uint64_t      EfuseSvsControl  :  2; // This field specifies how the EfuseVoltageOffset field is used to implement static voltage scaling (SVS): 0x0: this is used for pre-production where a single static voltage is to be used for all parts. It indicates that the EfuseVoltageOffset field should be ignored. Previously these two EFUSE bits were reserved and were set to 0. 0x1: SVS Production Release Revision 1. 0x2: this value is set aside for SVS Production Release Revision 2, if ever needed. 0x3: this value is set aside for SVS Production Release Revision 3, if ever needed. The mapping from EfuseVoltageOffset values to voltage is defined by the FXR ASIC package-level datasheet.
        uint64_t Customer_Reserved_63  : 12; // 
    } field;
    uint64_t val;
} RCF_MAN_EFUSE1_t;

// RCF_MAN_EFUSE2 desc:
typedef union {
    struct {
        uint64_t       SerDesReserved  : 64; // SerDes reserved for BRCM use.
    } field;
    uint64_t val;
} RCF_MAN_EFUSE2_t;

// RCF_MAN_EFUSE3 desc:
typedef union {
    struct {
        uint64_t    EfuseSerialNumber  : 24; // 24 bits of serial number guaranteed unique per manufactured ASIC by manufacturer programmed at manufacturing test time.
        uint64_t      EfuseSubUnitLow  :  3; // Lower 3 bits of subunit.
        uint64_t    EfuseTesterNumber  :  5; // 5 bits used for package tester ID.
        uint64_t    Customer_Reserved  : 16; // Reserved for customer use - not yet used by FXR (16 EFUSE bits available).
        uint64_t         EfuseManDate  : 16; // Date of manufacturing test in a format that can be decoded by host software. Note that this is the date when the part passes through the manufacturing test process rather than the wafer manufacturing date. The date is determined according to the Coordinated Universal Time (UTC) standard. These 16 bits encode the manufacturing test date in a MM-DD-YY format as follows: [243:240] = 4 bits for month (values are 1-12) [248:244] = 5 bits for day of the month (values are 1-31) [255:249] = 7 bits for year minus 2000 (values map to 2000-2127) A value of 0x0 indicates that the date was not programmed.
    } field;
    uint64_t val;
} RCF_MAN_EFUSE3_t;

// RCF_DFX_FUSE_REG4_PART_ID desc:
typedef union {
    struct {
        uint64_t         UniquepartID  : 64; // Phantom Creek Unique Part ID.
    } field;
    uint64_t val;
} RCF_DFX_FUSE_REG4_PART_ID_t;

// RCF_DFX_FUSE_REG5_CONTROL desc:
typedef union {
    struct {
        uint64_t Orangepasswordenable  :  1; // Orange password check enable. 0 = Orange password key is disabled. 1 = Orange password key is enabled.
        uint64_t    Redpasswordenable  :  1; // Red password check enable. 0 = Red password key is disabled. 1 = Red password key is enabled.
        uint64_t        Decoderenable  :  1; // Decoder enable. If set, DFX Agg will decode the 128 bit encoded key to 64 bit keys. If not set, DFX Agg uses the first 64 bits as keys.
        uint64_t         Decommission  :  1; // Decommision not supported. DFX Agg decommission support is disabled via parameter.
        uint64_t      DRAMdebugenable  :  1; // DRAM debug enable, not applicable to FXR.
        uint64_t                spare  :  3; // 
        uint64_t  MetalPasswdDisabled  :  1; // Metal password pass control (64b metal password check). 0 = Metal password pass enabled (IP can be unlocked via metal password check pass) 1 = Metal password pass disabled (IP can only be unlocked via Orange and Red passwords).
        uint64_t    ManufacturingExit  :  1; // Manufacturing exit control fuse bit. 0 = Manufacturing state 1 = Production ready
        uint64_t DFXAggdefaultpersonalityen  :  1; // DFX Agg default personality enable. If set to 1, the personality generator will set the personality to the DPF fuse value, and it cannot be overridden with any unlock mechanism. Must not be set post-PRQ.
        uint64_t DFXAggdefaultpersonality  :  5; // DFX Agg default personality (DPF) value. Used by personality generator if default personality enable fuse bit is set.
        uint64_t             spare_31  : 16; // 
        uint64_t      FuseBurnDisable  :  1; // Fuse disable burn control. 0=preproduction parts 1=production parts Note: This fuse bit is not sent to the DFX Aggregator.
        uint64_t             spare_63  : 31; // 
    } field;
    uint64_t val;
} RCF_DFX_FUSE_REG5_CONTROL_t;

// RCF_FUSE_REG0_CONTROL desc:
typedef union {
    struct {
        uint64_t EfuseMBistChainDisable  :  4; // These fuses can be used to disable individual MBIST chains. May be useful for reduction of PCIE boot time.
        uint64_t                spare  : 12; // 
        uint64_t    EfuseMBistDisable  :  1; // This can be used to disable running of the memory built-in self test (MBIST). Note that memory repair (REI) is performed prior to MBIST regardless of the setting of this EFUSE.
        uint64_t EfuseDisableMBistChk  :  1; // ** This feature is not support in FXR. This can be used to disable the halting of the initialization sequence based on a check on the MBIST result. The value of this bit has no effect if MBIST is disabled. Otherwise, if checking is enabled and the MBIST fails, the initialization sequence halts. If checking is disabled the initialization sequence continues regardless of whether the MBIST fails.
        uint64_t      EfuseDisableREI  :  1; // This can be used to disable running of the REI.
        uint64_t      Ignore_PLL_Lock  :  1; // This fuse bit can be used to ignore PLL lock on all three PLL's. The PLL lock state will then exit on state timeout instead. This control bit is disabled if DINIT_State_TO_Disable=1
        uint64_t Bypass_PLL_Lock_State  :  1; // This fuse bit can be used to bypass waiting for PLL lock or state timeout. Primarily used for debug, FPGA or Emulation.
        uint64_t             spare_22  :  2; // 
        uint64_t Ignore_Pm_Req_Core_Rst  :  1; // When set, RCF shim logic will not use pm_req_core_rst input from PCIE controller in reset generation logic.
        uint64_t Force_FW_Default_ROM  :  1; // When set, FW partition will force use of default Sbus Master ROM instead of loading full featured firmware from flash.
        uint64_t DINIT_State_TO_Disable  :  1; // This fuse bit can be used to disabled DINIT state timeouts just in case any timeout is to short.
        uint64_t             spare_31  :  6; // 
        uint64_t EfusePLL_load_enable  :  1; // Enable Risc PLL load Config from fuse data.
        uint64_t             spare_35  :  3; // 
        uint64_t        EfusePLL_data  : 21; // bit 20 = prog_fb_div23 bit 19:12= prog_fbdiv255 bit 11:6= pll_refcnt bit 5:0= pll_out_divcnt.
        uint64_t             spare_63  :  7; // 
    } field;
    uint64_t val;
} RCF_FUSE_REG0_CONTROL_t;

// RCF_FUSE_REG1_MISC desc:
typedef union {
    struct {
        uint64_t EfuseDLL_load_enable  :  1; // Enable DLL load Config from fuse data.
        uint64_t                spare  :  3; // 
        uint64_t        EfuseDLL_data  : 18; // bit 17:12= fbcnt bit 11:6= dll_refcnt bit 5:0= dll_out_divcnt
        uint64_t             spare_31  : 10; // 
        uint64_t              OC_MISC  :  8; // bit 0 = limit CRK 8051 to GEN1 bit rate bits 7:1 = reserved
        uint64_t             spare_63  : 24; // spare fuse bits
    } field;
    uint64_t val;
} RCF_FUSE_REG1_MISC_t;

// RCF_FUSE_REG2_VERSION desc:
typedef union {
    struct {
        uint64_t          FuseVersion  : 16; // Specifies the version of the Fuse bit vector that was programmed into FXR_Fuse at manufacturing test time.
        uint64_t          spare_31_16  : 16; // spare fuse bits
        uint64_t     ASICRevisionInfo  : 32; // ASIC revision information. bits 63:56 = spare (0x00) bits 55:48 = Architecture - 0x01 = 1st version bits 47:40 = Chip Name - 0x03 = APR bits 39:36 = Chip Step - 0x0 = A, 0x1 = B, 0x2 = C bits 35:32 = Chip Step Rev - 0x0 = 0, 0x1 = 1, 0x2 = 2 The ASIC revision of the chip design takes a unique value for each ASIC stepping.
    } field;
    uint64_t val;
} RCF_FUSE_REG2_VERSION_t;

// RCF_FUSE_REG3_GUID desc:
typedef union {
    struct {
        uint64_t        Reserved_23_0  : 24; // Reserved for SERIALNO[23:0] -- unique for the TYPE. Firmware utilizes MAN fuse bits 215:192 for this field.
        uint64_t             FuseType  :  8; // Bits 31:30 = set to zero for switches (0x0) Bits 29:24 = Product Type = APR switch (0x4) Firmware can use a value of 0x5 instead of 0x4 when constructing GUID values for LWEG capable ports.
        uint64_t       Reserved_39_32  :  8; // Reserved for manufacturer tester number. Firmware utilizes MAN fuse bits 222:219 for this field.
        uint64_t              FuseOUI  : 24; // This field is the 24-bit OUI (Organizationally Unique Identifier) that will be used for Storm Lake products. Bits 63:56 = 0x00 Bits 55:48 = 0x11 Bits 47:40 = 0x75
    } field;
    uint64_t val;
} RCF_FUSE_REG3_GUID_t;

// RCF_FUSE_REG4_THERMAL desc:
typedef union {
    struct {
        uint64_t Thermal_sensor_clk_div  : 10; // Sensor clock divide ratio
        uint64_t                spare  :  6; // spare fuse bits
        uint64_t     Thermal_limit_lo  : 12; // Low temperature limit - temperatures below this value will trigger the low temperature interrupt
        uint64_t             spare_31  :  4; // spare fuse bits
        uint64_t     Thermal_limit_hi  : 12; // High temperature limit - temperatures above this value will trigger the high temperature interrupt
        uint64_t             spare_47  :  4; // spare fuse bits
        uint64_t   Thermal_crit_limit  : 12; // Critical temperature limit - temperatures above this value will trigger the assertion of the critical temperature interrupt
        uint64_t             spare_63  :  4; // spare fuse bits
    } field;
    uint64_t val;
} RCF_FUSE_REG4_THERMAL_t;

// RCF_FUSE_REG5_FIRMWARE_VALIDATION desc:
typedef union {
    struct {
        uint64_t FuseFirmwareHWValidationEnable  :  1; // Enable firmware hardware authentication. 0 = disable firmware authentication 1 = enable firmware authentication
        uint64_t            spare_3_1  :  3; // spare fuse bits
        uint64_t FuseFirmwareSWValidationEnable  :  1; // Enable firmware software authentication for PEs in RX_HP and TX_OTR. 0 = disable firmware authentication 1 = enable firmware authentication
        uint64_t            spare_7_5  :  3; // spare fuse bits
        uint64_t FuseFirmwareHWBootControl  :  1; // Firmware hardware authentication control. 0 = boot with production key check, if fails, boot with debug key check, if fails, stop 1 = boot with production key check, if fails, stop
        uint64_t           spare_11_9  :  3; // spare fuse bits
        uint64_t FuseFirmwareSWBootControl  :  1; // Firmware software authentication control for PEs in RX_HP and TX_OTR. 0 = boot with production key check, if fails, boot with debug key check, if fails, stop 1 = boot with production key check, if fails, stop
        uint64_t          spare_31_13  : 19; // spare fuse bits
        uint64_t          spare_63_32  : 32; // spare fuse bits
    } field;
    uint64_t val;
} RCF_FUSE_REG5_FIRMWARE_VALIDATION_t;

// RCF_FUSE_REG6_9_HW_PRODUCTION_KEY desc:
typedef union {
    struct {
        uint64_t   HWProduction_Key_0  : 32; // 
        uint64_t   HWProduction_Key_1  : 32; // 
    } field;
    uint64_t val;
} RCF_FUSE_REG6_9_HW_PRODUCTION_KEY_t;

// RCF_FUSE_REG10_13_HW_DEBUG_KEY desc:
typedef union {
    struct {
        uint64_t        HWDebug_Key_0  : 32; // 
        uint64_t        HWDebug_Key_1  : 32; // 
    } field;
    uint64_t val;
} RCF_FUSE_REG10_13_HW_DEBUG_KEY_t;

// RCF_FUSE_REG14_17_SW_PRODUCTION_KEY desc:
typedef union {
    struct {
        uint64_t   SWProduction_Key_0  : 32; // 
        uint64_t   SWProduction_Key_1  : 32; // 
    } field;
    uint64_t val;
} RCF_FUSE_REG14_17_SW_PRODUCTION_KEY_t;

// RCF_FUSE_REG18_21_SW_DEBUG_KEY desc:
typedef union {
    struct {
        uint64_t        SWDebug_Key_0  : 32; // 
        uint64_t        SWDebug_Key_1  : 32; // 
    } field;
    uint64_t val;
} RCF_FUSE_REG18_21_SW_DEBUG_KEY_t;

// RCF_DINIT_TDR_STATUS desc:
typedef union {
    struct {
        uint64_t        reserved_11_0  : 12; // 
        uint64_t   o_lmon_stop_clk_en  :  1; // 
        uint64_t sbm_rom_sequence_complete  :  1; // 
        uint64_t mbist_timeout_status  :  1; // 
        uint64_t   rei_timeout_status  :  1; // 
        uint64_t pll_lock_timeout_status  :  1; // 
        uint64_t fuse_late_timeout_status  :  1; // 
        uint64_t fuse_early_timeout_status  :  1; // 
        uint64_t fuse_sense_timeout_status  :  1; // 
        uint64_t         q_mbist_fail  :  1; // 
        uint64_t         q_mbist_pass  :  1; // 
        uint64_t           o_mbist_go  :  1; // 
        uint64_t      q_rei_done_fail  :  1; // 
        uint64_t      q_rei_done_pass  :  1; // 
        uint64_t             o_rei_go  :  1; // 
        uint64_t       fuse_late_done  :  1; // 
        uint64_t       o_fuse_late_go  :  1; // 
        uint64_t      fuse_early_done  :  1; // 
        uint64_t      o_fuse_early_go  :  1; // 
        uint64_t      fuse_sense_done  :  1; // 
        uint64_t      o_fuse_sense_go  :  1; // 
        uint64_t      ignore_pll_lock  :  1; // 
        uint64_t direct_mode_occurred  :  1; // 
        uint64_t   mbist_bypass_taken  :  1; // 
        uint64_t     rei_bypass_taken  :  1; // 
        uint64_t bypass_pll_lock_taken  :  1; // 
        uint64_t         mbist_bypass  :  1; // 
        uint64_t           rei_bypass  :  1; // 
        uint64_t      bypass_pll_lock  :  1; // 
        uint64_t      timeout_disable  :  1; // 
        uint64_t          reserved_41  :  1; // 
        uint64_t             pll_lock  :  1; // 
        uint64_t          reserved_43  :  1; // 
        uint64_t             enc_cs30  :  4; // 
        uint64_t          reserved_48  :  1; // 
        uint64_t    fuse_disable_burn  :  1; // 
        uint64_t          reserved_50  :  1; // 
        uint64_t          reserved_51  :  1; // 
        uint64_t          reserved_52  :  1; // 
        uint64_t          fusebreak_n  :  1; // 
        uint64_t             o_hreset  :  1; // 
        uint64_t             o_sreset  :  1; // 
        uint64_t          o_sbr_rst_n  :  1; // 
        uint64_t        o_pll_reset_n  :  1; // 
        uint64_t o_core_memmaster_rst  :  1; // 
        uint64_t      o_core_rei_mode  :  1; // 
        uint64_t       o_core_rei_run  :  1; // 
        uint64_t   step_to_next_state  :  1; // 
        uint64_t     single_step_mode  :  1; // 
        uint64_t   select_direct_mode  :  1; // 
    } field;
    uint64_t val;
} RCF_DINIT_TDR_STATUS_t;

#endif /* ___FXR_rcf_CSRS_H__ */
