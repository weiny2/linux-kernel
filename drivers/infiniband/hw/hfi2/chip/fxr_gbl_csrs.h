// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Wed Apr 11 12:49:08 2018
//

#ifndef ___FXR_gbl_CSRS_H__
#define ___FXR_gbl_CSRS_H__

// ASIC_CFG_DRV_STR desc:
typedef union {
    struct {
        uint64_t           DrvStrQsfp  :  2; // 2-bit drive strength for QSFP_I2CCLK, QSFP_I2CDAT, QSFP_RESET_N, QSFP_LED_N, QSFP_INT_N, and QSFP_MODPRST_N
        uint64_t           DrvStrGpio  :  2; // 2-bit drive strength for GPIO[9:0]
        uint64_t        RESERVED_63_4  : 60; // Reserved
    } field;
    uint64_t val;
} ASIC_CFG_DRV_STR_t;

// ASIC_GPIO_IN desc:
typedef union {
    struct {
        uint64_t              GPIO_IN  : 10; // Input value from each of the 16 GPIO pins
        uint64_t       RESERVED_63_10  : 54; // Reserved
    } field;
    uint64_t val;
} ASIC_GPIO_IN_t;

// ASIC_GPIO_OE desc:
typedef union {
    struct {
        uint64_t              GPIO_OE  : 10; // Specifies the output enable For each of the 16 GPIO pins: - Bit set to 0 = pin output disabled - Bit set to 1 = pin output enabled
        uint64_t       RESERVED_63_10  : 54; // Reserved
    } field;
    uint64_t val;
} ASIC_GPIO_OE_t;

// ASIC_GPIO_INVERT desc:
typedef union {
    struct {
        uint64_t          GPIO_INVERT  : 10; // Specifies whether to invert the read value For each of the 16 GPIO pins: - Bit set to 0 = pin value is not inverted - Bit set to 1 = pin value is inverted
        uint64_t       RESERVED_63_10  : 54; // Reserved
    } field;
    uint64_t val;
} ASIC_GPIO_INVERT_t;

// ASIC_GPIO_OUT desc:
typedef union {
    struct {
        uint64_t             GPIO_OUT  : 10; // The output value for each of the 16 GPIO pins (if output is enabled)
        uint64_t       RESERVED_63_10  : 54; // Reserved
    } field;
    uint64_t val;
} ASIC_GPIO_OUT_t;

// ASIC_GPIO_MASK desc:
typedef union {
    struct {
        uint64_t            GPIO_MASK  : 10; // The mask value. For each of the 16 GPIO pins: - 0: pin does not contribute to the GPIO interrupt - 1: pin contributes to the GPIO interrupt
        uint64_t       RESERVED_63_10  : 54; // Reserved
    } field;
    uint64_t val;
} ASIC_GPIO_MASK_t;

// ASIC_GPIO_STATUS desc:
typedef union {
    struct {
        uint64_t          GPIO_STATUS  : 10; // Status value from each of the 16 GPIO pins
        uint64_t       RESERVED_63_10  : 54; // Reserved
    } field;
    uint64_t val;
} ASIC_GPIO_STATUS_t;

// ASIC_GPIO_CLEAR desc:
typedef union {
    struct {
        uint64_t           GPIO_CLEAR  : 10; // For each of the 16 GPIO pins: - Write 0 has no effect - Write 1 to clear down the status - Reads as zero
        uint64_t       RESERVED_63_10  : 54; // Reserved
    } field;
    uint64_t val;
} ASIC_GPIO_CLEAR_t;

// ASIC_GPIO_FORCE desc:
typedef union {
    struct {
        uint64_t           GPIO_FORCE  : 10; // For each of the 16 GPIO pins: - Write 0 has no effect - Write 1 to set the status - Reads as zero
        uint64_t       RESERVED_63_10  : 54; // Reserved
    } field;
    uint64_t val;
} ASIC_GPIO_FORCE_t;

// ASIC_QSFP_IN desc:
typedef union {
    struct {
        uint64_t              QSFP_IN  :  5; // The input value from each of the 5 QSFP pins: Bit 0: I2CCLK Bit 1: I2CDAT Bit 2: always 0 (RESET_N is output pin) Bit 3: INT_N Bit 4: MODPRST_N The reset value given here is for all input pins to be at 1 coming out of reset. I2CCLK and I2CCLK have OE of 0 which causes the pins to be driven high. The values of INT_N and MODPRST_N depend on the QSFP device, but typically default to 1 indicating not asserted. RESET_N is an output pin and is not connected for reads, so the corresponding bit always reads as 0.
        uint64_t        RESERVED_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} ASIC_QSFP_IN_t;

// ASIC_QSFP_OE desc:
typedef union {
    struct {
        uint64_t              QSFP_OE  :  5; // The output enable for each of the 5 QSFP pins: Bit 0: I2CCLK Bit 1: I2CDAT Bit 2: RESET_N (not used) Bit 3: INT_N (not used) Bit 4: MODPRST_N (not used) The values are: Bit set to 0 = pin output disabled Bit set to 1 = pin output enabled The OE capability is not used for RESET_N since this pin is a dedicated output. Similarly, it is not used for INT_N nor MODPRST_N since these pins are dedicated inputs. The OE value for I2CCLK and I2CDAT pins is inverted and connected to the pin. When OE is 0 the pin is driven high, while when OE is 1 the pin is driven low. This matches an 'open drain' convention.
        uint64_t        RESERVED_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} ASIC_QSFP_OE_t;

// ASIC_QSFP_INVERT desc:
typedef union {
    struct {
        uint64_t          QSFP_INVERT  :  5; // Specifies whether to invert the read value for each of the 5 QSFP pins: Bit 0: I2CCLK Bit 1: I2CDAT Bit 2: RESET_N Bit 3: INT_N Bit 4: MODPRST_N The values are: Bit set to 0 = pin value is not inverted Bit set to 1 = pin value is inverted
        uint64_t        RESERVED_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} ASIC_QSFP_INVERT_t;

// ASIC_QSFP_OUT desc:
typedef union {
    struct {
        uint64_t             QSFP_OUT  :  5; // The output value for each of the 5 QSFP pins (if output is enabled): Bit 0: I2CCLK (not used) Bit 1: I2CDAT (not used) Bit 2: RESET_N Bit 3: INT_N (not used) Bit 4: MODPRST_N (not used) The output value is not used for I2CCLK and I2CDAT pins. Instead, the corresponding bit of the ASIC_QSFP_OE register is inverted and connected to the pin. When OE is 0 the pin is driven high, while when OE is 1 the pin is driven low. This matches an 'open drain' convention. The output value is not used for INT_N and MODPRST_N pins since these are input pins. The reset value of the RESET_N pin is 1 (driven high) so that the attached QSFP device is not held in reset when the HFI comes out of power-on reset. The RESET_N pin was reset to 0 on A0.
        uint64_t        RESERVED_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} ASIC_QSFP_OUT_t;

// ASIC_QSFP_MASK desc:
typedef union {
    struct {
        uint64_t            QSFP_MASK  :  5; // The mask value for each of the 5 QSFP pins: Bit 0: I2CCLK Bit 1: I2CDAT Bit 2: RESET_N Bit 3: INT_N Bit 4: MODPRST_N The values are: Bit set to 0: pin does not contribute to the QSFP interrupt Bit set to 1: pin contributes to the QSFP interrupt
        uint64_t        RESERVED_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} ASIC_QSFP_MASK_t;

// ASIC_QSFP_STATUS desc:
typedef union {
    struct {
        uint64_t          QSFP_STATUS  :  5; // The status value from each of the 5 QSFP pins: Bit 0: I2CCLK Bit 1: I2CDAT Bit 2: always 0 (RESET_N is output pin) Bit 3: INT_N Bit 4: MODPRST_N The reset value given here is for all input pins to be at 1 coming out of reset. I2CCLK and I2CCLK have OE of 0 which causes the pins to be driven high. The values of INT_N and MODPRST_N depend on the QSFP device, but typically default to 1 indicating not asserted. RESET_N is an output pin and is not connected for reads, so the corresponding bit always reads as 0.
        uint64_t        RESERVED_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} ASIC_QSFP_STATUS_t;

// ASIC_QSFP_CLEAR desc:
typedef union {
    struct {
        uint64_t           QSFP_CLEAR  :  5; // Clear capability for each of the 5 QSFP pins: Bit 0: I2CCLK Bit 1: I2CDAT Bit 2: RESET_N Bit 3: INT_N Bit 4: MODPRST_N The behavior is: Write 0 to a bit has no effect Write 1 to a bit clears the status for that bit Reads as zero
        uint64_t        RESERVED_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} ASIC_QSFP_CLEAR_t;

// ASIC_QSFP_FORCE desc:
typedef union {
    struct {
        uint64_t           QSFP_FORCE  :  5; // Force capability for each of the 5 QSFP pins: Bit 0: I2CCLK Bit 1: I2CDAT Bit 2: RESET_N Bit 3: INT_N Bit 4: MODPRST_N The behavior is: Write 0 to a bit has no effect Write 1 to a bit sets the status for that bit Reads as zero
        uint64_t        RESERVED_63_5  : 59; // Reserved
    } field;
    uint64_t val;
} ASIC_QSFP_FORCE_t;

#endif /* ___FXR_gbl_CSRS_H__ */
