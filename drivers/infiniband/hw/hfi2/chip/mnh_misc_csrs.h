// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Thu Mar 29 15:03:56 2018
//

#ifndef ___MNH_misc_CSRS_H__
#define ___MNH_misc_CSRS_H__

// MISC_MSC_RSA_R2 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 64; // 64-bit chunk of RSA helper variable R squared
    } field;
    uint64_t val;
} MISC_MSC_RSA_R2_t;

// MISC_MSC_RSA_SIGNATURE desc:
typedef union {
    struct {
        uint64_t                 DATA  : 64; // 64-bit chunk of RSA signature
    } field;
    uint64_t val;
} MISC_MSC_RSA_SIGNATURE_t;

// MISC_MSC_RSA_MODULUS desc:
typedef union {
    struct {
        uint64_t                 DATA  : 64; // 64-bit chunk of RSA modulus
    } field;
    uint64_t val;
} MISC_MSC_RSA_MODULUS_t;

// MISC_MSC_SHA_PRELOAD desc:
typedef union {
    struct {
        uint64_t                 DATA  : 64; // Data to be preloaded to the SHA engine.
    } field;
    uint64_t val;
} MISC_MSC_SHA_PRELOAD_t;

// MISC_MSC_RSA_CMD desc:
typedef union {
    struct {
        uint64_t                  CMD  :  2; // RSA engine command: - 0x0=NOP - 0x1=INIT - 0x2=START - 0x3=Reserved
        uint64_t        RESERVED_63_2  : 62; // Reserved
    } field;
    uint64_t val;
} MISC_MSC_RSA_CMD_t;

// MISC_MSC_RSA_MU desc:
typedef union {
    struct {
        uint64_t                 DATA  : 64; // RSA helper variable, mu
    } field;
    uint64_t val;
} MISC_MSC_RSA_MU_t;

// MISC_CFG_FW_CTRL desc:
typedef union {
    struct {
        uint64_t FW_VALIDATION_ENABLE  :  1; // Enables firmware validation. This bit is OR'd with the FirmwareValidationEnable fuse. If either bit is set, firmware validation is required to enable the microcontrollers.
        uint64_t       FW_8051_LOADED  :  1; // Software signal to hardware of firmware download status. Clear this bit before loading 8051 firmware. Set it after done loading. Clearing this will assert an internal reset signal from the firmware security logic to the 8051 block, and enable code RAM write access. Setting this will disable code RAM write access and deassert the internal reset signal. Note that the internal reset signal is OR'ed with other 8051 CSR bits to factor in to the final 8051 reset signal.
        uint64_t        RESERVED_63_2  : 62; // Reserved
    } field;
    uint64_t val;
} MISC_CFG_FW_CTRL_t;

// MISC_STS_FW desc:
typedef union {
    struct {
        uint64_t           RSA_STATUS  :  2; // RSA engine status: - 0x0=IDLE - 0x1=ACTIVE - 0x2=DONE - 0x3=FAILED
        uint64_t         KEY_MISMATCH  :  1; // Hash of MISC_MSC_RSA_MODULUS mismatches with the FirmwarePublicKeyHash fuses
        uint64_t       FW_AUTH_FAILED  :  1; // Firmware authentication failed due to either key mismatch or unsuccessful signature validation
        uint64_t        RESERVED_63_4  : 60; // Reserved
    } field;
    uint64_t val;
} MISC_STS_FW_t;

// MISC_STS_8051_DIGEST desc:
typedef union {
    struct {
        uint64_t                 DATA  : 64; // 64-bit chunk of 8051 firmware digest
    } field;
    uint64_t val;
} MISC_STS_8051_DIGEST_t;

// MISC_STS_SBM_DIGEST desc:
typedef union {
    struct {
        uint64_t                 DATA  : 64; // 64-bit chunk of SBus Master Spico firmware digest
    } field;
    uint64_t val;
} MISC_STS_SBM_DIGEST_t;

// MISC_STS_SRDS_DIGEST desc:
typedef union {
    struct {
        uint64_t                 DATA  : 64; // 64-bit chunk of SerDes Spico firmware digest
    } field;
    uint64_t val;
} MISC_STS_SRDS_DIGEST_t;

// MISC_CFG_RESET desc:
typedef union {
    struct {
        uint64_t               SRESET  :  1; // Soft reset - see Section 8.3, 'Global Resets'
        uint64_t               HRESET  :  1; // Hard reset - see Section 8.3, 'Global Resets' Setting this bit causes the global hreset to assert. A side effect of the global hreset asserting is that all CSR bits, including this one, will reset to their reset values. In other words, this bit is self-clearing. It will, however, hold for at least 16 sbr_clk cycles. Another side effect of writing this bit to a one is that it will cause an OPIO reset as described in Section 20.1.7, 'OPIO Port Bounce (OPIO Reset)' .
        uint64_t        RESERVED_63_2  : 62; // Reserved
    } field;
    uint64_t val;
} MISC_CFG_RESET_t;

// MISC_STS_MNH_INTS desc:
typedef union {
    struct {
        uint64_t             MISC_ERR  :  1; // Indicates an error is flagged in the MISC_ERR_STS register
        uint64_t         OPIO_PHY_ERR  :  1; // Indicates an error is flagged in the OPIO_PHY_ERR_STS register
        uint64_t   SERDES_CRK8051_ERR  :  1; // Indicates an error is flagged in the CRK8051_ERR_STS register
        uint64_t SERDES_SNP_UPSTRM_ERR  :  1; // Indicates an error is flagged in the SNP_UPSTRM_ERR_STS register
        uint64_t SERDES_SNP_DWNSTRM_ERR  :  1; // Indicates an error is flagged in the SNP_DWNSTRM_ERR_STS register
        uint64_t             QSFP_INT  :  1; // Indicates one or more bits are asserted in the MISC_QSFP_STATUS register. See also MISC_QSFP_MASK register.
        uint64_t            READY_INT  :  1; // Indicates a MNH Ready interrupt is currently being signaled to the HFI
        uint64_t        RESERVED_63_7  : 57; // Reserved
    } field;
    uint64_t val;
} MISC_STS_MNH_INTS_t;

// MISC_CFG_SBUS_REQUEST desc:
typedef union {
    struct {
        uint64_t        RECEIVER_ADDR  :  8; // Receiver device address (on SBus ring)
        uint64_t            DATA_ADDR  :  8; // Data address (within receiver device)
        uint64_t              COMMAND  :  8; // Command. The 8 bit command contains: Bit 7: determines interpretation of [1:0]: 0x1: SBus controller command 0x0: SBus receive command Bits [6:5]: target/source interface 0x0: TAP 0x1: core 0x2: Spico 0x3: reserved Bits [4:2]: set to 0 Bits [1:0]: the interpretation when using SBus controller command (bit 7 is 1): 0x0: read all 0x1: OR mode 0x2: normal mode (default) 0x3: AND mode Bits [1:0]: the interpretation when using SBus receiver command (bit 7 is 0): 0x0: reset 0x1: write 0x2: read 0x3: read result The most common commands are: 0x20: core receiver interface reset 0x21: core write command 0x22: core read command 0x23: core read result
        uint64_t       RESERVED_31_24  :  8; // Reserved
        uint64_t              DATA_IN  : 32; // 32-bit write data
    } field;
    uint64_t val;
} MISC_CFG_SBUS_REQUEST_t;

// MISC_CFG_SBUS_EXECUTE desc:
typedef union {
    struct {
        uint64_t              EXECUTE  :  1; // Execute bit The transition of this bit from 0 to 1 will cause a new command to be executed: Write 0 to clear down after the previous command. Write 1 to execute the next command.
        uint64_t        RESERVED_63_1  : 63; // Reserved
    } field;
    uint64_t val;
} MISC_CFG_SBUS_EXECUTE_t;

// MISC_STS_SBUS_RESULT desc:
typedef union {
    struct {
        uint64_t                 DONE  :  1; // Signifies when SBus has received the command and is ready to receive the next one.
        uint64_t       RCV_DATA_VALID  :  1; // Signifies when result code and data are valid.
        uint64_t          RESULT_CODE  :  3; // 3-bit result code: 0x0: SBus Controller/receiver reset 0x1: write complete 0x2: read all complete 0x3: write failed 0x4: read complete 0x5: mode change complete 0x6: read failed 0x7: command issue state machine done
        uint64_t        RESERVED_31_5  : 27; // Reserved
        uint64_t             DATA_OUT  : 32; // 32-bit result data
    } field;
    uint64_t val;
} MISC_STS_SBUS_RESULT_t;

// MISC_CFG_DRV_STR desc:
typedef union {
    struct {
        uint64_t             DRV_QSFP  :  2; // 2-bit drive strength for QSFP_I2CCLK, QSFP_I2CDAT, and QSFP_RESET_N Initialized from DrvStr[1:0]
        uint64_t        DRV_FXR_INT_N  :  2; // 2-bit drive strength for FXR_INT_N Initialized from DrvStr[3:2]
        uint64_t      DRV_THERMTRIP_N  :  2; // 2-bit drive strength for THERMTRIP_N Initialized from DrvStr[5:4]
        uint64_t        RESERVED_63_6  : 58; // Reserved
    } field;
    uint64_t val;
} MISC_CFG_DRV_STR_t;

// MISC_CFG_THERM desc:
typedef union {
    struct {
        uint64_t        SENSOR_ENABLE  :  2; // Enables polling of main and remote sensors. 00: Poll both sensors 01: Poll main sensor only 10: Poll remote sensor only 11: Poll both sensors
        uint64_t         RESERVED_3_2  :  2; // Reserved
        uint64_t     MAIN_POLL_PERIOD  : 10; // Polling period for main temperature sensor
        uint64_t       RESERVED_15_14  :  2; // Reserved
        uint64_t   REMOTE_POLL_PERIOD  : 10; // Polling period for remote temperature sensor
        uint64_t       RESERVED_31_26  :  6; // Reserved
        uint64_t           HYSTERESIS  :  6; // Hysteresis value for clearing of high and low temperature alerts and recovery from Thermal Downgrade mode
        uint64_t       RESERVED_63_38  : 26; // Reserved
    } field;
    uint64_t val;
} MISC_CFG_THERM_t;

// MISC_STS_THERM desc:
typedef union {
    struct {
        uint64_t           HIGH_ALERT  :  1; // High temperature alert
        uint64_t            LOW_ALERT  :  1; // Low temperature alert
        uint64_t         RESERVED_3_2  :  2; // Reserved
        uint64_t                STATE  :  3; // State of thermal polling FSM: 0: RESET 1: SET_CLK_DIV 2: SET_MODE 3: SET_START 4: DELAY 5: READ_TEMP 6: CLR_START 7: SBUS_ERROR
        uint64_t      SENSOR_SELECTED  :  1; // Current sensor selected for polling: 0: Main sensor 1: Remote sensor
        uint64_t         SBUS_COMMAND  :  8; // Captures COMMAND from last SBus request
        uint64_t    SBUS_DATA_ADDRESS  :  8; // Captures DATA_ADDRESS from last SBus request
        uint64_t            SBUS_DATA  : 32; // Captures DATA from last SBus request
        uint64_t         SBUS_EXECUTE  :  1; // Monitors live EXECUTE bit
        uint64_t            SBUS_DONE  :  1; // Monitors live DONE bit
        uint64_t  SBUS_RCV_DATA_VALID  :  1; // Captures RCV_DATA_VALID from last SBus request
        uint64_t     SBUS_RESULT_CODE  :  3; // Captures RESULT_CODE from last SBus request
        uint64_t       RESERVED_63_62  :  2; // Reserved
    } field;
    uint64_t val;
} MISC_STS_THERM_t;

// MISC_STS_TEMPERATURE desc:
typedef union {
    struct {
        uint64_t         TEMPERATURE0  :  9; // Temperature read from main sensor
        uint64_t         TEMPERATURE1  :  9; // Temperature read from remote sensor
        uint64_t       RESERVED_29_18  : 12; // Reserved
        uint64_t               VALID1  :  1; // Valid bit for TEMPERATURE1
        uint64_t               VALID0  :  1; // Valid bit for TEMPERATURE0
        uint64_t       RESERVED_63_32  : 32; // Reserved
    } field;
    uint64_t val;
} MISC_STS_TEMPERATURE_t;

// MISC_CFG_HFI_READY desc:
typedef union {
    struct {
        uint64_t            HFI_READY  :  1; // Indication from the HFI that it is ready.
        uint64_t           OPIO_RESET  :  1; // 
        uint64_t        RESERVED_63_2  : 62; // Reserved
    } field;
    uint64_t val;
} MISC_CFG_HFI_READY_t;

// MISC_STS_FUSE_MANINFO desc:
typedef union {
    struct {
        uint64_t              MANINFO  : 63; // Manufacture tracking information (to include fab, lot, wafer, x/y co-ordinates) in a format that can be decoded by host software. This information is defined in the EFUSE map from the ASIC manufacturing partner. In summary: [6:0]: fab designation (7-bit ASCII code: G-fab6, Q-fab8, N-fab12, P-fab14, T-fab15) [13:7]: lot designation (7 bit ASCII code) [41:14]: lot number (4 x 7-bit ASCII code) [46:42]: wafer number (5-bit binary) 47: x sign (0 = positive x, 1 = negative x) [54:48]: x coordinate (7-bit binary) 55: y sign (0 = positive y, 1 = negative y) [62:56]: y coordinate (7-bit binary)
        uint64_t          RESERVED_63  :  1; // Reserved
    } field;
    uint64_t val;
} MISC_STS_FUSE_MANINFO_t;

// MISC_STS_FUSE_MANDATE desc:
typedef union {
    struct {
        uint64_t              MANDATE  : 16; // Date of manufacturing test in a format that can be decoded by host software. Note that this is the date when the part passes through the manufacturing test process rather than the wafer manufacturing date. The date is determined according to the Coordinated Universal Time (UTC) standard. These 16 bits encode the manufacturing test date in a MM-DD-YY format as follows: [3:0] = 4 bits for month (values are 1-12) [8:4] = 5 bits for day of the month (values are 1-31) [15:9] = 7 bits for year minus 2000 (values map to 2000-2127) A value of 0x0 indicates that the date was not programmed.
        uint64_t       RESERVED_63_16  : 48; // Reserved
    } field;
    uint64_t val;
} MISC_STS_FUSE_MANDATE_t;

// MISC_STS_FUSE_VERSION desc:
typedef union {
    struct {
        uint64_t          FUSEVERSION  : 16; // Specifies the version of the Fuse bit vector that was programmed into MNH_Fuse at manufacturing test time
        uint64_t       RESERVED_31_16  : 16; // Reserved
        uint64_t     ASICREVISIONINFO  : 32; // Asic revision information
    } field;
    uint64_t val;
} MISC_STS_FUSE_VERSION_t;

// MISC_ERR_STS desc:
typedef union {
    struct {
        uint64_t   FUSE_SENSE_TIMEOUT  :  1; // DINIT FUSE_SENSE state timeout
        uint64_t   FUSE_EARLY_TIMEOUT  :  1; // DINIT FUSE_EARLY state timeout
        uint64_t    FUSE_LATE_TIMEOUT  :  1; // DINIT FUSE_LATE state timeout
        uint64_t     PLL_LOCK_TIMEOUT  :  1; // DINIT PLL_LOCK state timeout
        uint64_t          REI_TIMEOUT  :  1; // DINIT RUN_REI state timeout
        uint64_t        MBIST_TIMEOUT  :  1; // DINIT MEM_BIST state timeout
        uint64_t             REI_FAIL  :  1; // RAM repair failure during DINIT
        uint64_t           MBIST_FAIL  :  1; // MBIST failure during DINIT
        uint64_t        RESERVED_63_8  : 56; // Reserved
    } field;
    uint64_t val;
} MISC_ERR_STS_t;

// MISC_ERR_CLR desc:
typedef union {
    struct {
        uint64_t   FUSE_SENSE_TIMEOUT  :  1; // 
        uint64_t   FUSE_EARLY_TIMEOUT  :  1; // 
        uint64_t    FUSE_LATE_TIMEOUT  :  1; // 
        uint64_t     PLL_LOCK_TIMEOUT  :  1; // 
        uint64_t          REI_TIMEOUT  :  1; // 
        uint64_t        MBIST_TIMEOUT  :  1; // 
        uint64_t             REI_FAIL  :  1; // 
        uint64_t           MBIST_FAIL  :  1; // 
        uint64_t        RESERVED_63_8  : 56; // Reserved
    } field;
    uint64_t val;
} MISC_ERR_CLR_t;

// MISC_ERR_FRC desc:
typedef union {
    struct {
        uint64_t   FUSE_SENSE_TIMEOUT  :  1; // 
        uint64_t   FUSE_EARLY_TIMEOUT  :  1; // 
        uint64_t    FUSE_LATE_TIMEOUT  :  1; // 
        uint64_t     PLL_LOCK_TIMEOUT  :  1; // 
        uint64_t          REI_TIMEOUT  :  1; // 
        uint64_t        MBIST_TIMEOUT  :  1; // 
        uint64_t             REI_FAIL  :  1; // 
        uint64_t           MBIST_FAIL  :  1; // 
        uint64_t        RESERVED_63_8  : 56; // Reserved
    } field;
    uint64_t val;
} MISC_ERR_FRC_t;

// MISC_ERR_EN_HOST desc:
typedef union {
    struct {
        uint64_t   FUSE_SENSE_TIMEOUT  :  1; // 
        uint64_t   FUSE_EARLY_TIMEOUT  :  1; // 
        uint64_t    FUSE_LATE_TIMEOUT  :  1; // 
        uint64_t     PLL_LOCK_TIMEOUT  :  1; // 
        uint64_t          REI_TIMEOUT  :  1; // 
        uint64_t        MBIST_TIMEOUT  :  1; // 
        uint64_t             REI_FAIL  :  1; // 
        uint64_t           MBIST_FAIL  :  1; // 
        uint64_t        RESERVED_63_8  : 56; // Reserved
    } field;
    uint64_t val;
} MISC_ERR_EN_HOST_t;

// MISC_ERR_FIRST_HOST desc:
typedef union {
    struct {
        uint64_t   FUSE_SENSE_TIMEOUT  :  1; // 
        uint64_t   FUSE_EARLY_TIMEOUT  :  1; // 
        uint64_t    FUSE_LATE_TIMEOUT  :  1; // 
        uint64_t     PLL_LOCK_TIMEOUT  :  1; // 
        uint64_t          REI_TIMEOUT  :  1; // 
        uint64_t        MBIST_TIMEOUT  :  1; // 
        uint64_t             REI_FAIL  :  1; // 
        uint64_t           MBIST_FAIL  :  1; // 
        uint64_t        RESERVED_63_8  : 56; // Reserved
    } field;
    uint64_t val;
} MISC_ERR_FIRST_HOST_t;

// MISC_QSFP_IN desc:
typedef union {
    struct {
        uint64_t              QSFP_IN  :  5; // The input value from each of the 5 QSFP pins: Bit 0: I2CCLK Bit 1: I2CDAT Bit 2: always 0 (RESET_N is output pin) Bit 3: INT_N Bit 4: MODPRST_N The reset value given here is for all input pins to be at 1 coming out of reset. I2CCLK and I2CCLK have OE of 0 which causes the pins to be driven high. The values of INT_N and MODPRST_N depend on the QSFP device, but typically default to 1 indicating not asserted. RESET_N is an output pin and is not connected for reads, so the corresponding bit always reads as 0.
        uint64_t        RESERVED_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} MISC_QSFP_IN_t;

// MISC_QSFP_OE desc:
typedef union {
    struct {
        uint64_t              QSFP_OE  :  5; // The output enable for each of the 5 QSFP pins: Bit 0: I2CCLK Bit 1: I2CDAT Bit 2: RESET_N (not used) Bit 3: INT_N (not used) Bit 4: MODPRST_N (not used) The values are: Bit set to 0 = pin output disabled Bit set to 1 = pin output enabled The OE capability is not used for RESET_N since this pin is a dedicated output. Similarly, it is not used for INT_N nor MODPRST_N since these pins are dedicated inputs. The OE value for I2CCLK and I2CDAT pins is inverted and connected to the pin. When OE is 0 the pin is driven high, while when OE is 1 the pin is driven low. This matches an 'open drain' convention.
        uint64_t        RESERVED_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} MISC_QSFP_OE_t;

// MISC_QSFP_INVERT desc:
typedef union {
    struct {
        uint64_t          QSFP_INVERT  :  5; // Specifies whether to invert the read value for each of the 5 QSFP pins: Bit 0: I2CCLK Bit 1: I2CDAT Bit 2: RESET_N Bit 3: INT_N Bit 4: MODPRST_N The values are: Bit set to 0 = pin value is not inverted Bit set to 1 = pin value is inverted
        uint64_t        RESERVED_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} MISC_QSFP_INVERT_t;

// MISC_QSFP_OUT desc:
typedef union {
    struct {
        uint64_t             QSFP_OUT  :  5; // The output value for each of the 5 QSFP pins (if output is enabled): Bit 0: I2CCLK (not used) Bit 1: I2CDAT (not used) Bit 2: RESET_N Bit 3: INT_N (not used) Bit 4: MODPRST_N (not used) The output value is not used for I2CCLK and I2CDAT pins. Instead, the corresponding bit of the MISC_QSFP_OE register is inverted and connected to the pin. When OE is 0 the pin is driven high, while when OE is 1 the pin is driven low. This matches an 'open drain' convention. The output value is not used for INT_N and MODPRST_N pins since these are input pins. The reset value of the RESET_N pin is 1 (driven high) so that the attached QSFP device is not held in reset when the HFI comes out of power-on reset or ASIC reset. The RESET_N pin was reset to 0 on A0.
        uint64_t        RESERVED_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} MISC_QSFP_OUT_t;

// MISC_QSFP_MASK desc:
typedef union {
    struct {
        uint64_t            QSFP_MASK  :  5; // The mask value for each of the 5 QSFP pins: Bit 0: I2CCLK Bit 1: I2CDAT Bit 2: RESET_N Bit 3: INT_N Bit 4: MODPRST_N The values are: Bit set to 0: pin does not contribute to the QSFP interrupt Bit set to 1: pin contributes to the QSFP interrupt
        uint64_t        RESERVED_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} MISC_QSFP_MASK_t;

// MISC_QSFP_STATUS desc:
typedef union {
    struct {
        uint64_t          QSFP_STATUS  :  5; // The status value from each of the 5 QSFP pins: Bit 0: I2CCLK Bit 1: I2CDAT Bit 2: always 0 (RESET_N is output pin) Bit 3: INT_N Bit 4: MODPRST_N The reset value given here is for all input pins to be at 1 coming out of reset. I2CCLK and I2CCLK have OE of 0 which causes the pins to be driven high. The values of INT_N and MODPRST_N depend on the QSFP device, but typically default to 1 indicating not asserted. RESET_N is an output pin and is not connected for reads, so the corresponding bit always reads as 0.
        uint64_t        RESERVED_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} MISC_QSFP_STATUS_t;

// MISC_QSFP_CLEAR desc:
typedef union {
    struct {
        uint64_t           QSFP_CLEAR  :  5; // Clear capability for each of the 5 QSFP pins: Bit 0: I2CCLK Bit 1: I2CDAT Bit 2: RESET_N Bit 3: INT_N Bit 4: MODPRST_N The behavior is: Write 0 to a bit has no effect Write 1 to a bit clears the status for that bit Reads as zero
        uint64_t        RESERVED_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} MISC_QSFP_CLEAR_t;

// MISC_QSFP_FORCE desc:
typedef union {
    struct {
        uint64_t           QSFP_FORCE  :  5; // Force capability for each of the 5 QSFP pins: Bit 0: I2CCLK Bit 1: I2CDAT Bit 2: RESET_N Bit 3: INT_N Bit 4: MODPRST_N The behavior is: Write 0 to a bit has no effect Write 1 to a bit sets the status for that bit Reads as zero
        uint64_t        RESERVED_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} MISC_QSFP_FORCE_t;

// MISC_VISA_CMD desc:
typedef union {
    struct {
        uint64_t            VISA_ADDR  : 14; // Index for reading/writing visa controller and writing visa mux registers. 13:5 Visa unit mux ID 4:0 Offset
        uint64_t        VISA_WR_AVAIL  :  1; // Write request to visa controller. Hardware clears this bit when setting the VISA_WR_GET bit.
        uint64_t           VISA_RD_EN  :  1; // Read request to visa controller. When written 1 by software, the hardware will clear this bit on the following cycle. Reads will always return 0.
        uint64_t      VISA_TRIGGER_IN  :  8; // Bit vector to trigger visa controller replay.
        uint64_t          VISA_WR_GET  :  1; // Write slot available indicator. When set the visa controller is ready to accept another write command.
        uint64_t          VISA_SECURE  :  1; // Visa controller secure input
        uint64_t         UNUSED_31_26  :  6; // unused- spare
        uint64_t       RESERVED_63_32  : 32; // Reserved
    } field;
    uint64_t val;
} MISC_VISA_CMD_t;

// MISC_VISA_DATA desc:
typedef union {
    struct {
        uint64_t            VISA_DATA  : 32; // Data to/from visa controller
        uint64_t       RESERVED_63_32  : 32; // Reserved
    } field;
    uint64_t val;
} MISC_VISA_DATA_t;

#endif /* ___MNH_misc_CSRS_H__ */