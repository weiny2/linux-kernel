// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Wed Apr 11 12:49:08 2018
//

#ifndef ___FXR_fw_CSRS_H__
#define ___FXR_fw_CSRS_H__

// FW_MSC_RSA_R2 desc:
typedef union {
    struct {
        uint64_t                 data  : 64; // 64-bit chunk of RSA helper variable R squared
    } field;
    uint64_t val;
} FW_MSC_RSA_R2_t;

// FW_MSC_RSA_SIGNATURE desc:
typedef union {
    struct {
        uint64_t                 data  : 64; // 64-bit chunk of RSA signature
    } field;
    uint64_t val;
} FW_MSC_RSA_SIGNATURE_t;

// FW_MSC_RSA_MODULUS desc:
typedef union {
    struct {
        uint64_t                 data  : 64; // 64-bit chunk of RSA modulus
    } field;
    uint64_t val;
} FW_MSC_RSA_MODULUS_t;

// FW_MSC_SHA_PRELOAD desc:
typedef union {
    struct {
        uint64_t                 data  : 64; // Data to be preloaded to the SHA engine.
    } field;
    uint64_t val;
} FW_MSC_SHA_PRELOAD_t;

// FW_MSC_RSA_CMD desc:
typedef union {
    struct {
        uint64_t                  cmd  :  2; // RSA engine command: 0x0=NOP 0x1=INIT 0x2=START 0x3=Reserved
        uint64_t        reserved_63_2  : 62; // Reserved
    } field;
    uint64_t val;
} FW_MSC_RSA_CMD_t;

// FW_MSC_RSA_MU desc:
typedef union {
    struct {
        uint64_t                 data  : 64; // RSA helper variable, mu
    } field;
    uint64_t val;
} FW_MSC_RSA_MU_t;

// FW_CFG_FW_CTRL desc:
typedef union {
    struct {
        uint64_t fw_validation_enable  :  1; // Enables firmware validation. This bit is OR'd with the FirmwareValidationEnable fuse. If either bit is set, firmware validation is required to enable the microcontrollers.
        uint64_t       fw_8051_loaded  :  1; // Software signal to hardware of firmware download status. Clear this bit before loading 8051 firmware. Set it after done loading. Clearing this will assert an internal reset signal from the firmware security logic to the 8051 block, and enable code RAM write access. Setting this will disable code RAM write access and deassert the internal reset signal. Note that the internal reset signal is OR'ed with other 8051 CSR bits to factor in to the final 8051 reset signal.
        uint64_t        reserved_63_2  : 62; // Reserved
    } field;
    uint64_t val;
} FW_CFG_FW_CTRL_t;

// FW_STS_FW desc:
typedef union {
    struct {
        uint64_t           rsa_status  :  2; // RSA engine status: 0x0=IDLE 0x1=ACTIVE 0x2=DONE 0x3=FAILED
        uint64_t         key_mismatch  :  1; // Hash of FW_MSC_RSA_MODULUS mismatches with the FirmwarePublicKeyHash fuses
        uint64_t       fw_auth_failed  :  1; // Firmware authentication failed due to either key mismatch or unsuccessful signature validation
        uint64_t        reserved_63_4  : 60; // Reserved
    } field;
    uint64_t val;
} FW_STS_FW_t;

// FW_STS_8051_DIGEST desc:
typedef union {
    struct {
        uint64_t                 data  : 64; // 64-bit chunk of 8051 firmware digest
    } field;
    uint64_t val;
} FW_STS_8051_DIGEST_t;

// FW_STS_FAB_DIGEST desc:
typedef union {
    struct {
        uint64_t                 data  : 64; // 64-bit chunk of Fabric SerDes Spico firmware digest
    } field;
    uint64_t val;
} FW_STS_FAB_DIGEST_t;

// FW_STS_BSM_DIGEST desc:
typedef union {
    struct {
        uint64_t                 data  : 64; // 64-bit chunk of BSM firmware digest
    } field;
    uint64_t val;
} FW_STS_BSM_DIGEST_t;

// FW_CFG_SBUS_REQUEST desc:
typedef union {
    struct {
        uint64_t        receiver_addr  :  8; // Receiver device address (on SBus ring)
        uint64_t            data_addr  :  8; // Data address (within receiver device)
        uint64_t              command  :  8; // Command. The 8 bit command contains: Bit 7: determines interpretation of [1:0]: 0x1: SBus controller command 0x0: SBus receive command Bits [6:5]: target/source interface 0x0: TAP 0x1: core 0x2: Spico 0x3: reserved Bits [4:2]: set to 0 Bits [1:0]: the interpretation when using SBus controller command (bit 7 is 1): 0x0: read all 0x1: OR mode 0x2: normal mode (default) 0x3: AND mode Bits [1:0]: the interpretation when using SBus receiver command (bit 7 is 0): 0x0: reset 0x1: write 0x2: read 0x3: read result The most common commands are: 0x20: core receiver interface reset 0x21: core write command 0x22: core read command 0x23: core read result
        uint64_t       reserved_31_24  :  8; // Reserved
        uint64_t              data_in  : 32; // 32-bit write data
    } field;
    uint64_t val;
} FW_CFG_SBUS_REQUEST_t;

// FW_CFG_SBUS_EXECUTE desc:
typedef union {
    struct {
        uint64_t              execute  :  1; // Execute bit The transition of this bit from 0 to 1 will cause a new command to be executed: Write 0 to clear down after the previous command. Write 1 to execute the next command.
        uint64_t        reserved_63_1  : 63; // Reserved
    } field;
    uint64_t val;
} FW_CFG_SBUS_EXECUTE_t;

// FW_STS_SBUS_RESULT desc:
typedef union {
    struct {
        uint64_t                 done  :  1; // Signifies when SBus has received the command and is ready to receive the next one.
        uint64_t       rcv_data_valid  :  1; // Signifies when result code and data are valid.
        uint64_t          result_code  :  3; // 3-bit result code: 0x0: SBus Controller/receiver reset 0x1: write complete 0x2: read all complete 0x3: write failed 0x4: read complete 0x5: mode change complete 0x6: read failed 0x7: command issue state machine done
        uint64_t        reserved_31_5  : 27; // Reserved
        uint64_t             data_out  : 32; // 32-bit result data
    } field;
    uint64_t val;
} FW_STS_SBUS_RESULT_t;

// FW_STS_SBUS_COUNTERS desc:
typedef union {
    struct {
        uint64_t          execute_cnt  : 16; // EXECUTE count
        uint64_t   rcv_data_valid_cnt  : 16; // RCV_DATA_VALID count
        uint64_t                   eq  :  1; // rcv_data_valid_cnt == execute_cnt
        uint64_t       reserved_63_33  : 31; // Reserved
    } field;
    uint64_t val;
} FW_STS_SBUS_COUNTERS_t;

// FW_CFG_THERM desc:
typedef union {
    struct {
        uint64_t        sensor_enable  :  2; // Enables polling of main and remote sensors. 00: Poll both sensors 01: Poll main sensor only 10: Poll remote sensor only 11: Poll both sensors
        uint64_t         reserved_3_2  :  2; // Reserved
        uint64_t     main_poll_period  : 10; // Polling period for main temperature sensor
        uint64_t       reserved_15_14  :  2; // Reserved
        uint64_t   remote_poll_period  : 10; // Polling period for remote temperature sensor
        uint64_t       reserved_31_26  :  6; // Reserved
        uint64_t           hysteresis  :  6; // Hysteresis value for clearing of high and low temperature alerts
        uint64_t       reserved_63_38  : 26; // Reserved
    } field;
    uint64_t val;
} FW_CFG_THERM_t;

// FW_STS_THERM desc:
typedef union {
    struct {
        uint64_t           high_alert  :  1; // High temperature alert
        uint64_t            low_alert  :  1; // Low temperature alert
        uint64_t         reserved_3_2  :  2; // Reserved
        uint64_t                state  :  3; // State of thermal polling FSM: 0: RESET 1: SET_CLK_DIV 2: SET_MODE 3: SET_START 4: DELAY 5: READ_TEMP 6: CLR_START 7: SBUS_ERROR
        uint64_t      sensor_selected  :  1; // Current sensor selected for polling: 0: Main sensor 1: Remote sensor
        uint64_t         sbus_command  :  8; // Captures COMMAND from last SBus request
        uint64_t    sbus_data_address  :  8; // Captures DATA_ADDRESS from last SBus request
        uint64_t            sbus_data  : 32; // Captures DATA from last SBus request
        uint64_t         sbus_execute  :  1; // Monitors live EXECUTE bit
        uint64_t            sbus_done  :  1; // Monitors live DONE bit
        uint64_t  sbus_rcv_data_valid  :  1; // Captures RCV_DATA_VALID from last SBus request
        uint64_t     sbus_result_code  :  3; // Captures RESULT_CODE from last SBus request
        uint64_t       reserved_63_62  :  2; // Reserved
    } field;
    uint64_t val;
} FW_STS_THERM_t;

// FW_STS_TEMPERATURE desc:
typedef union {
    struct {
        uint64_t         temperature0  :  9; // Temperature read from main sensor
        uint64_t         temperature1  :  9; // Temperature read from remote sensor
        uint64_t       reserved_29_18  : 12; // Reserved
        uint64_t               valid1  :  1; // Valid bit for TEMPERATURE1
        uint64_t               valid0  :  1; // Valid bit for TEMPERATURE0
        uint64_t       reserved_63_32  : 32; // Reserved
    } field;
    uint64_t val;
} FW_STS_TEMPERATURE_t;

// FW_CFG_BSM_SCRATCH desc:
typedef union {
    struct {
        uint64_t                 data  : 64; // 
    } field;
    uint64_t val;
} FW_CFG_BSM_SCRATCH_t;

// FW_CFG_BSM_CTRL desc:
typedef union {
    struct {
        uint64_t              command  :  4; // Command to execute. Command must be reset to 0 before a new command can be executed. The valid commands are: NONE(0): LOAD_FROM_SPI(8): Request loading from boot STOP_BOOT(9): Request stopping boot load
        uint64_t         command_done  :  1; // Indicates if the command requested is completed.
        uint64_t     eeprom_load_done  :  1; // Indicates if loading from EEPROM is done or not.
        uint64_t         eeprom_error  :  1; // Indicates if loading stopped because an error was met.
        uint64_t        eeprom_enable  :  1; // Indicates if loading is active or not.
        uint64_t          eeprom_addr  : 24; // Indicates next address to read from EEPROM. If EEPROM terminates before the end of stream (error), then the address points to the instruction following the instructions that failed.
        uint64_t       reserved_63_32  : 32; // Reserved
    } field;
    uint64_t val;
} FW_CFG_BSM_CTRL_t;

// FW_CFG_BSM_ARGS desc:
typedef union {
    struct {
        uint64_t                 data  : 32; // For LOAD_FROM_SPI, the address must be loaded in DATA[23:0] and the options in DATA[31:24], in the same format as bytes 0-3 of a serial ROM.
        uint64_t       reserved_63_32  : 32; // Reserved
    } field;
    uint64_t val;
} FW_CFG_BSM_ARGS_t;

// FW_CFG_BSM_ADDR_OFFSET desc:
typedef union {
    struct {
        uint64_t               offset  : 24; // Configurable base address referred to in the BSM instruction encoding.
        uint64_t       reserved_63_24  : 40; // Reserved
    } field;
    uint64_t val;
} FW_CFG_BSM_ADDR_OFFSET_t;

// FW_CFG_BSM_COUNTER desc:
typedef union {
    struct {
        uint64_t              counter  : 16; // Loop counter used by the BSM LOOP instruction.
        uint64_t       reserved_63_16  : 48; // Reserved
    } field;
    uint64_t val;
} FW_CFG_BSM_COUNTER_t;

// FW_CFG_BSM_SRAM_CTRL desc:
typedef union {
    struct {
        uint64_t            err_write  :  2; // For error injection, set both bits to generate an uncorrectable error, set either bit to generate a correctable error. Clear the bits for normal write operations.
        uint64_t                 cerr  :  1; // Correctable errors.
        uint64_t                 uerr  :  1; // Uncorrectable errors.
        uint64_t       bist_done_pass  :  1; // 
        uint64_t       bist_done_fail  :  1; // 
        uint64_t        reserved_63_6  : 58; // Reserved
    } field;
    uint64_t val;
} FW_CFG_BSM_SRAM_CTRL_t;

// FW_CFG_BSM_IP desc:
typedef union {
    struct {
        uint64_t             sram_err  :  2; // Set when a memory error is detected in the BSM. Bits correspond to memory error type: bit 1: uncorrectable bit 0: correctable See BSM_SRAM_CTRL register to determine source of error
        uint64_t        reserved_63_2  : 62; // Reserved
    } field;
    uint64_t val;
} FW_CFG_BSM_IP_t;

// FW_CFG_BSM_IM desc:
typedef union {
    struct {
        uint64_t             sram_err  :  2; // 
        uint64_t        reserved_63_2  : 62; // Reserved
    } field;
    uint64_t val;
} FW_CFG_BSM_IM_t;

// FW_CFG_SPI_TX_DATA desc:
typedef union {
    struct {
        uint64_t                 data  : 32; // Data shall be stored [ShiftSize*8-1:0] and data will be sent with MSB first.
        uint64_t       reserved_63_32  : 32; // Reserved
    } field;
    uint64_t val;
} FW_CFG_SPI_TX_DATA_t;

// FW_STS_SPI_RX_DATA desc:
typedef union {
    struct {
        uint64_t                 data  : 32; // Data will be stored [ShiftSize*8-1:0] and data will be sent with MSB first.
        uint64_t       reserved_63_32  : 32; // Reserved
    } field;
    uint64_t val;
} FW_STS_SPI_RX_DATA_t;

// FW_CFG_SPI_HEADER desc:
typedef union {
    struct {
        uint64_t                 data  : 32; // Data will be stored [HeaderSize*8-1:0] and data will be sent with MSB first.
        uint64_t       reserved_63_32  : 32; // Reserved
    } field;
    uint64_t val;
} FW_CFG_SPI_HEADER_t;

// FW_CFG_SPI_CTRL desc:
typedef union {
    struct {
        uint64_t                 freq  : 10; // Defines the clock divider to be used. Actual speed is PCIE_REFCLK/(2*(1+Freq)). Default is 1MHZ.
        uint64_t               enable  :  1; // Defines if the SPI controller is enabled or not. Command is ignored if controller is not enabled.
        uint64_t              command  :  4; // Defines the command to execute. The command is a bit field with the following bits defined: bit 0: send header or not bit 1: send turn-around-byte or not bit 2: shift data bit 3: release chip select The command must be reset to 0 for another command to be executed.
        uint64_t          header_size  :  2; // Defines the number of bytes in the header, zero means 4 bytes. The value could be set simultaneously with the command.
        uint64_t            data_size  :  2; // Defines number of bytes to shift, zero means 4 bytes. The value could be set simultaneously with the command.
        uint64_t    data_shift_method  :  2; // Defines the access method for data. SINGLE(0): DUAL(1): QUAD(2): The value could be set simultaneously with the command.
        uint64_t                 busy  :  1; // Set while a command is executed. Cleared when command is completed.
        uint64_t             selected  :  1; // Set if chip is currently selected.
        uint64_t              dir_io2  :  1; // Set direction for SPI_IO2; '1' is output, '0' is input. This register gets override when operating in or QUAD-pin mode.
        uint64_t              dir_io3  :  1; // Set direction for SPI_IO3; '1' is output, '0' is input. This register gets override when operating in or QUAD-pin mode.
        uint64_t              pin_io2  :  1; // Read or set pins SPI_IO2. Reading always returns the actual input pins. Writing sets the pin, but this might be ignore if the pin is an input or device actually working in QUAD-pin mode.
        uint64_t              pin_io3  :  1; // Read or set pins SPI_IO3. Reading always returns the actual input pins. Writing sets the pin, but this might be ignore if the pin is an input or device actually working in QUAD-pin mode.
        uint64_t       reserved_63_27  : 37; // Reserved
    } field;
    uint64_t val;
} FW_CFG_SPI_CTRL_t;

// FW_DBG_SPI_FSM_DEBUG desc:
typedef union {
    struct {
        uint64_t    spi_present_state  :  3; // 
        uint64_t       spi_next_state  :  3; // 
        uint64_t        reserved_63_6  : 58; // Reserved
    } field;
    uint64_t val;
} FW_DBG_SPI_FSM_DEBUG_t;

#endif /* ___FXR_fw_CSRS_H__ */
