// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Thu Jun  2 19:11:24 2016
//

#ifndef ___MNH_misc_CSRS_H__
#define ___MNH_misc_CSRS_H__

// MISC_MSC_RSA_R2 desc:
typedef union {
    struct {
        uint64_t                 DATA  : 64; // 64-bit chunk of RSA helper variable R-squared
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
        uint64_t                 DATA  : 64; // Data to be preloaded to the SHA engine. The SHA engine destroys this data. This register will always read as 0.
    } field;
    uint64_t val;
} MISC_MSC_SHA_PRELOAD_t;

// MISC_MSC_RSA_CMD desc:
typedef union {
    struct {
        uint64_t                  CMD  :  2; // RSA engine command: - 0x0=NOP - 0x1=INIT - 0x2=START - 0x3=Reserved The RSA engine resets this to NOP immediately after receiving INIT or START. This will always return NOP on a read.
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
        uint64_t   DISABLE_VALIDATION  :  1; // If the FirmwareValidationEnable fuse is deasserted, this bit disables firmware validation. If the FirmwareValidationEnable fuse is asserted, this bit has no effect. If firmware validation is disabled, 8051 code RAM write access is enabled, and the internal reset signal from the firmware security logic to the 8051 block is deasserted, regardless of the FW_8051_LOADED bit.
        uint64_t       FW_8051_LOADED  :  2; // Software signal to hardware of firmware download status. Clear this bit before loading 8051 firmware. Set it after done loading. Clearing this will assert an internal reset signal from the firmware security logic to the 8051 block, and enable code RAM write access. Setting this will disable code RAM write access and deassert the internal reset signal. Note that the internal reset signal is OR'ed with other DC CSR bits to factor in to the final 8051 reset signal. This field contains one bit per 8051 instance.
        uint64_t        RESERVED_63_3  : 61; // Reserved
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

// MISC_CFG_RESET desc:
typedef union {
    struct {
        uint64_t               HRESET  :  1; // Hard reset - see Section 8.2.1.1, 'Hard Reset'
        uint64_t               SRESET  :  1; // Soft reset - see Section 8.2.1.2, 'Soft Reset'
        uint64_t          PLACEHOLDER  : 30; // Placeholder
        uint64_t       RESERVED_63_32  : 32; // Reserved
    } field;
    uint64_t val;
} MISC_CFG_RESET_t;

// MISC_STS_MNH_ERRS desc:
typedef union {
    struct {
        uint64_t         OPIO_PHY_ERR  :  1; // Indicates an error is flagged in the OPIO_PHY_ERR_STS register
        uint64_t  SERDES0_CRK8051_ERR  :  1; // Indicates an error is flagged in the CRK8051_ERR_STS register of SerDes Partition 0
        uint64_t SERDES0_SNP_UPSTRM_ERR  :  1; // Indicates an error is flagged in the SNP_UPSTRM_ERR_STS register of SerDes Partition 0
        uint64_t SERDES0_SNP_DWNSTRM_ERR  :  1; // Indicates an error is flagged in the SNP_DWNSTRM_ERR_STS register of SerDes Partition 0
        uint64_t  SERDES1_CRK8051_ERR  :  1; // Indicates an error is flagged in the CRK8051_ERR_STS register of SerDes Partition 1
        uint64_t SERDES1_SNP_UPSTRM_ERR  :  1; // Indicates an error is flagged in the SNP_UPSTRM_ERR_STS register of SerDes Partition 1
        uint64_t SERDES1_SNP_DWNSTRM_ERR  :  1; // Indicates an error is flagged in the SNP_DWNSTRM_ERR_STS register of SerDes Partition 1
        uint64_t            QSFP1_INT  :  1; // Indicates one or more bits are asserted in the MISC_QSFP1_STATUS register. See also MISC_QSFP1_MASK register.
        uint64_t            QSFP2_INT  :  1; // Indicates one or more bits are asserted in the MISC_QSFP2_STATUS register. See also MISC_QSFP2_MASK register.
        uint64_t        RESERVED_63_9  : 55; // Reserved
    } field;
    uint64_t val;
} MISC_STS_MNH_ERRS_t;

// MISC_QSFP1_IN desc:
typedef union {
    struct {
        uint64_t             QSFP1_IN  :  5; // The input value from each of the 5 QSFP1 pins: Bit 0: I2CCLK Bit 1: I2CDAT Bit 2: always 0 (RESET_N is output pin) Bit 3: INT_N Bit 4: MODPRST_N The reset value given here is for all input pins to be at 1 coming out of reset. I2CCLK and I2CCLK have OE of 0 which causes the pins to be driven high. The values of INT_N and MODPRST_N depend on the QSFP device, but typically default to 1 indicating not asserted. RESET_N is an output pin and is not connected for reads, so the corresponding bit always reads as 0.
        uint64_t        RESERVED_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} MISC_QSFP1_IN_t;

// MISC_QSFP1_OE desc:
typedef union {
    struct {
        uint64_t             QSFP1_OE  :  5; // The output enable for each of the 5 QSFP1 pins: Bit 0: I2CCLK Bit 1: I2CDAT Bit 2: RESET_N (not used) Bit 3: INT_N (not used) Bit 4: MODPRST_N (not used) The values are: Bit set to 0 = pin output disabled Bit set to 1 = pin output enabled The OE capability is not used for RESET_N since this pin is a dedicated output. Similarly, it is not used for INT_N nor MODPRST_N since these pins are dedicated inputs. The OE value for I2CCLK and I2CDAT pins is inverted and connected to the pin. When OE is 0 the pin is driven high, while when OE is 1 the pin is driven low. This matches an 'open drain' convention.
        uint64_t        RESERVED_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} MISC_QSFP1_OE_t;

// MISC_QSFP1_INVERT desc:
typedef union {
    struct {
        uint64_t         QSFP1_INVERT  :  5; // Specifies whether to invert the read value for each of the 5 QSFP1 pins: Bit 0: I2CCLK Bit 1: I2CDAT Bit 2: RESET_N Bit 3: INT_N Bit 4: MODPRST_N The values are: Bit set to 0 = pin value is not inverted Bit set to 1 = pin value is inverted
        uint64_t        RESERVED_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} MISC_QSFP1_INVERT_t;

// MISC_QSFP1_OUT desc:
typedef union {
    struct {
        uint64_t            QSFP1_OUT  :  5; // The output value for each of the 5 QSFP1 pins (if output is enabled): Bit 0: I2CCLK (not used) Bit 1: I2CDAT (not used) Bit 2: RESET_N Bit 3: INT_N (not used) Bit 4: MODPRST_N (not used) The output value is not used for I2CCLK and I2CDAT pins. Instead, the corresponding bit of the MISC_QSFP1_OE register is inverted and connected to the pin. When OE is 0 the pin is driven high, while when OE is 1 the pin is driven low. This matches an 'open drain' convention. The output value is not used for INT_N and MODPRST_N pins since these are input pins. The reset value of the RESET_N pin is 1 (driven high) so that the attached QSFP device is not held in reset when the HFI comes out of power-on reset or ASIC reset. The RESET_N pin was reset to 0 on A0.
        uint64_t        RESERVED_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} MISC_QSFP1_OUT_t;

// MISC_QSFP1_MASK desc:
typedef union {
    struct {
        uint64_t           QSFP1_MASK  :  5; // The mask value for each of the 5 QSFP1 pins: Bit 0: I2CCLK Bit 1: I2CDAT Bit 2: RESET_N Bit 3: INT_N Bit 4: MODPRST_N The values are: Bit set to 0: pin does not contribute to the QSFP1 interrupt Bit set to 1: pin contributes to the QSFP1 interrupt
        uint64_t        RESERVED_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} MISC_QSFP1_MASK_t;

// MISC_QSFP1_STATUS desc:
typedef union {
    struct {
        uint64_t         QSFP1_STATUS  :  5; // The status value from each of the 5 QSFP1 pins: Bit 0: I2CCLK Bit 1: I2CDAT Bit 2: always 0 (RESET_N is output pin) Bit 3: INT_N Bit 4: MODPRST_N The reset value given here is for all input pins to be at 1 coming out of reset. I2CCLK and I2CCLK have OE of 0 which causes the pins to be driven high. The values of INT_N and MODPRST_N depend on the QSFP device, but typically default to 1 indicating not asserted. RESET_N is an output pin and is not connected for reads, so the corresponding bit always reads as 0.
        uint64_t        RESERVED_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} MISC_QSFP1_STATUS_t;

// MISC_QSFP1_CLEAR desc:
typedef union {
    struct {
        uint64_t          QSFP1_CLEAR  :  5; // Clear capability for each of the 5 QSFP1 pins: Bit 0: I2CCLK Bit 1: I2CDAT Bit 2: RESET_N Bit 3: INT_N Bit 4: MODPRST_N The behavior is: Write 0 to a bit has no effect Write 1 to a bit clears the status for that bit Reads as zero
        uint64_t        RESERVED_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} MISC_QSFP1_CLEAR_t;

// MISC_QSFP1_FORCE desc:
typedef union {
    struct {
        uint64_t          QSFP1_FORCE  :  5; // Force capability for each of the 5 QSFP1 pins: Bit 0: I2CCLK Bit 1: I2CDAT Bit 2: RESET_N Bit 3: INT_N Bit 4: MODPRST_N The behavior is: Write 0 to a bit has no effect Write 1 to a bit sets the status for that bit Reads as zero
        uint64_t        RESERVED_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} MISC_QSFP1_FORCE_t;

// MISC_QSFP2_IN desc:
typedef union {
    struct {
        uint64_t             QSFP2_IN  :  5; // The input value from each of the 5 QSFP2 pins: Bit 0: I2CCLK Bit 1: I2CDAT Bit 2: always 0 (RESET_N is output pin) Bit 3: INT_N Bit 4: MODPRST_N The reset value given here is for all input pins to be at 1 coming out of reset. I2CCLK and I2CCLK have OE of 0 which causes the pins to be driven high. The values of INT_N and MODPRST_N depend on the QSFP device, but typically default to 1 indicating not asserted. RESET_N is an output pin and is not connected for reads, so the corresponding bit always reads as 0.
        uint64_t        RESERVED_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} MISC_QSFP2_IN_t;

// MISC_QSFP2_OE desc:
typedef union {
    struct {
        uint64_t             QSFP2_OE  :  5; // The output enable for each of the 5 QSFP2 pins: Bit 0: I2CCLK Bit 1: I2CDAT Bit 2: RESET_N (not used) Bit 3: INT_N (not used) Bit 4: MODPRST_N (not used) The values are: Bit set to 0 = pin output disabled Bit set to 1 = pin output enabled The OE capability is not used for RESET_N since this pin is a dedicated output. Similarly, it is not used for INT_N nor MODPRST_N since these pins are dedicated inputs. The OE value for I2CCLK and I2CDAT pins is inverted and connected to the pin. When OE is 0 the pin is driven high, while when OE is 1 the pin is driven low. This matches an 'open drain' convention.
        uint64_t        RESERVED_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} MISC_QSFP2_OE_t;

// MISC_QSFP2_INVERT desc:
typedef union {
    struct {
        uint64_t         QSFP2_INVERT  :  5; // Specifies whether to invert the read value for each of the 5 QSFP2 pins: Bit 0: I2CCLK Bit 1: I2CDAT Bit 2: RESET_N Bit 3: INT_N Bit 4: MODPRST_N The values are: Bit set to 0 = pin value is not inverted Bit set to 1 = pin value is inverted
        uint64_t        RESERVED_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} MISC_QSFP2_INVERT_t;

// MISC_QSFP2_OUT desc:
typedef union {
    struct {
        uint64_t            QSFP2_OUT  :  5; // The output value for each of the 5 QSFP2 pins (if output is enabled): Bit 0: I2CCLK (not used) Bit 1: I2CDAT (not used) Bit 2: RESET_N Bit 3: INT_N (not used) Bit 4: MODPRST_N (not used) The output value is not used for I2CCLK and I2CDAT pins. Instead, the corresponding bit of the MISC_QSFP2_OE register is inverted and connected to the pin. When OE is 0 the pin is driven high, while when OE is 1 the pin is driven low. This matches an 'open drain' convention. The output value is not used for INT_N and MODPRST_N pins since these are input pins. The reset value of the RESET_N pin is 1 (driven high) so that the attached QSFP device is not held in reset when the HFI comes out of power-on reset or ASIC reset. The RESET_N pin was reset to 0 on A0.
        uint64_t        RESERVED_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} MISC_QSFP2_OUT_t;

// MISC_QSFP2_MASK desc:
typedef union {
    struct {
        uint64_t           QSFP2_MASK  :  5; // The mask value for each of the 5 QSFP2 pins: Bit 0: I2CCLK Bit 1: I2CDAT Bit 2: RESET_N Bit 3: INT_N Bit 4: MODPRST_N The values are: Bit set to 0: pin does not contribute to the QSFP2 interrupt Bit set to 1: pin contributes to the QSFP2 interrupt
        uint64_t        RESERVED_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} MISC_QSFP2_MASK_t;

// MISC_QSFP2_STATUS desc:
typedef union {
    struct {
        uint64_t         QSFP2_STATUS  :  5; // The status value from each of the 5 QSFP2 pins: Bit 0: I2CCLK Bit 1: I2CDAT Bit 2: always 0 (RESET_N is output pin) Bit 3: INT_N Bit 4: MODPRST_N The reset value given here is for all input pins to be at 1 coming out of reset. I2CCLK and I2CCLK have OE of 0 which causes the pins to be driven high. The values of INT_N and MODPRST_N depend on the QSFP device, but typically default to 1 indicating not asserted. RESET_N is an output pin and is not connected for reads, so the corresponding bit always reads as 0.
        uint64_t        RESERVED_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} MISC_QSFP2_STATUS_t;

// MISC_QSFP2_CLEAR desc:
typedef union {
    struct {
        uint64_t          QSFP2_CLEAR  :  5; // Clear capability for each of the 5 QSFP2 pins: Bit 0: I2CCLK Bit 1: I2CDAT Bit 2: RESET_N Bit 3: INT_N Bit 4: MODPRST_N The behavior is: Write 0 to a bit has no effect Write 1 to a bit clears the status for that bit Reads as zero
        uint64_t        RESERVED_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} MISC_QSFP2_CLEAR_t;

// MISC_QSFP2_FORCE desc:
typedef union {
    struct {
        uint64_t          QSFP2_FORCE  :  5; // Force capability for each of the 5 QSFP2 pins: Bit 0: I2CCLK Bit 1: I2CDAT Bit 2: RESET_N Bit 3: INT_N Bit 4: MODPRST_N The behavior is: Write 0 to a bit has no effect Write 1 to a bit sets the status for that bit Reads as zero
        uint64_t        RESERVED_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} MISC_QSFP2_FORCE_t;

// MISC_VISA_CMD desc:
typedef union {
    struct {
        uint64_t            VISA_ADDR  : 14; // Index for reading/writing visa controller and writing visa mux registers. 13:5 Visa unit mux ID 4:0 Offset
        uint64_t        VISA_WR_AVAIL  :  1; // Write request to visa controller. Hardware clears this bit when setting the VISA_WR_GET bit.
        uint64_t           VISA_RD_EN  :  1; // Read request to visa controller. When written 1 by software, the hardware will clear this bit on the following cycle. Reads will always return 0.
        uint64_t      VISA_TRIGGER_IN  :  8; // Bit vector to trigger visa controller replay.
        uint64_t          VISA_WR_GET  :  1; // Write slot available indicator. When set the visa controller is ready to accept another write command.
        uint64_t          VISA_SECURE  :  1; // Visa controller secure input
        uint64_t      VISA_FSCAN_MODE  :  1; // Visa controller fscan_mode input
        uint64_t         UNUSED_31_27  :  5; // unused- spare
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