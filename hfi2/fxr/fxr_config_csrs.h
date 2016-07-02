// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Wed Jun 22 19:30:15 2016
//

#ifndef ___FXR_config_CSRS_H__
#define ___FXR_config_CSRS_H__

// FXR_CFG_DEVICE_VENDOR_ID desc:
typedef union {
    struct {
        uint32_t            vendor_id  : 16; // Vendor ID. This field identifies the manufacturer of the device. Valid vendor identifiers are allocated by the PCI SIG to ensure uniqueness.
        uint32_t            device_id  : 16; // Device ID. This field identifies the particular device. This identifier is allocated by the vendor
    } field;
    uint32_t val;
} FXR_CFG_DEVICE_VENDOR_ID_t;

// FXR_CFG_STATUS_COMMAND desc:
typedef union {
    struct {
        uint32_t         io_space_ena  :  1; // IO Space Enable. IO Space access is not supported.
        uint32_t     memory_space_ena  :  1; // Memory Space Enable. Controls the device's response to Memory Space accesses. A value of 0 disabled the device response. A value of 1 allows the device to respond to Memory Space acceses.
        uint32_t       bus_master_ena  :  1; // Bus Master Enable. Controls the ability of a PCI Express Endpoint to issue Memory and I/O Read/Write Requests. When this bit is Set, the PCI Express Function is allowed to issue Memory or I/O Requests. When this bit is Clear, the PCI Express Function is not allowed to issue any Memory or I/O Requests. Note that as MSI/MSI-X interrupt Messages are in-band memory writes, setting the Bus Master Enable bit to 0 disables MSI/MSI-X interrupt Messages as well. Requests other than Memory or I/O Requests are not controlled by this bit.
        uint32_t    special_cycle_ena  :  1; // Special Cycle Enable. Does not apply, hardwired to 0.
        uint32_t        mem_wrt_inval  :  1; // Memory Write and Invalidate. Does not apply, hardwired to 0.
        uint32_t    vga_palette_snoop  :  1; // VGA Palette Snoop. Does not apply, hardwired to 0
        uint32_t    parity_error_resp  :  1; // Parity Error Response. This bit controls the logging of poisoned TLPs in the Master Data Parity Error bit in the Status register.
        uint32_t           idsel_step  :  1; // IDSEL Stepping/Wait Cycle Control- Does not apply, hardwired to 0.
        uint32_t          serr_enable  :  1; // SERR# Enable. When Set, this bit enables reporting of Non-fatal and Fatal errors detected by the Function to the Root Complex. Note that errors are reported if enabled either through this bit or through the PCI Express specific bits in the Device Control register
        uint32_t   fast_b2b_trans_ena  :  1; // Fast Back-to-Back Transactions Enable- Does not apply, must be hardwired to 0.
        uint32_t    interrupt_disable  :  1; // Interrupt Disable Controls the ability of a PCI Express Function to generate INTx interrupts. When Set, Functions are prevented from asserting INTx interrupts. Any INTx emulation interrupts already asserted by the Function must be deasserted when this bit is Set.
        uint32_t       reserved_18_11  :  8; // Reserved
        uint32_t     interrupt_status  :  1; // Interrupt Status. When Set, indicates that an INTx emulation interrupt is pending internally in the Function.
        uint32_t    capabilities_list  :  1; // Capabilities List Indicates the presence of an Extended Capability list item. Hardwired to 1.
        uint32_t      is66mhz_capable  :  1; // 66 MHz Capable. Does not apply, must be hardwired to 0.
        uint32_t          reserved_22  :  1; // Reserved
        uint32_t   fast_b2b_trans_cap  :  1; // Fast Back-to-Back Transactions Capable- Does not apply, must be hardwired to 0.
        uint32_t     mas_data_par_err  :  1; // Master Data Parity Error. This bit is Set by an Endpoint Function if the Parity Error Response bit in the Command register is 1 and either of the following two conditions occurs: Endpoint receives a Poisoned Completion Endpoint transmits a Poisoned Request
        uint32_t        devsel_timing  :  2; // DEVSEL Timing- Does not apply, hardwired to 2'b00
        uint32_t     sig_target_abort  :  1; // Signaled Target Abort. This bit is Set when a Function completes a Posted or NonPosted Request as a Completer Abort error.
        uint32_t     rec_target_abort  :  1; // Received Target Abort. This bit is Set when a Requester receives a Completion with Completer Abort Completion Status.
        uint32_t     rec_master_abort  :  1; // Received Master Abort. This bit is Set when a Requester receives a Completion with Unsupported Request Completion Status.
        uint32_t     sig_system_error  :  1; // Signaled System Error. This bit is Set when a Function sends an ERR_FATAL or ERR_NONFATAL Message, and the SERR# Enable bit in the Command register is 1.
        uint32_t         parity_error  :  1; // Detected Parity Error. This bit is Set by a Function whenever it receives a Poisoned TLP, regardless of the state the Parity Error Response bit in the Command Register.
    } field;
    uint32_t val;
} FXR_CFG_STATUS_COMMAND_t;

// FXR_CFG_CLASS_REVISION desc:
typedef union {
    struct {
        uint32_t          revision_id  :  8; // Revision ID. This register specifies a device specific revision identifier. The value is chosen by the vendor. This field should be viewed as a vendor defined extension to the Device ID.
        uint32_t           class_code  : 24; // Class Code. The Class Code register is used to identify the generic function of the device. The class code is subdivided as follows:- Base class [31:24] = 0x02 (network ctlr)- Sub-class [23:16] = 0x08 (Host Fabric ctlr)- Programming Interface [15:8] = 0x00
    } field;
    uint32_t val;
} FXR_CFG_CLASS_REVISION_t;

// FXR_CFG_HEADER_CACHELINE desc:
typedef union {
    struct {
        uint32_t      cache_line_size  :  8; // Cache Line Size. Specifies the system cacheline size in units of DWORDs. No impact to FXR.
        uint32_t      mas_latency_tim  :  8; // Master Latency Timer. Does not apply, hardwired to 0.
        uint32_t          header_type  :  7; // Header Type. Does not apply, hardwired to 0.
        uint32_t          device_type  :  1; // Device Type. This bit is used to identify a multi-function device. If the bit is 0, then the device is single function.If the bit is 1, then the device has multiple functions.
        uint32_t                 bist  :  8; // BIST Control.
    } field;
    uint32_t val;
} FXR_CFG_HEADER_CACHELINE_t;

// FXR_CFG_BAR0_LOWER desc:
typedef union {
    struct {
        uint32_t            mem_space  :  1; // Memory Space. When clear (zero), BAR defines a memory space address range.
        uint32_t             bar_type  :  2; // BAR Type. When set to 2'b10, BAR defines a 64-bit address range and is used in conjunction with BAR1
        uint32_t             prefetch  :  1; // Prefetchable. When set, address space is prefetchable. Reads to this space have no side effects.
        uint32_t         base_addr_ro  : 22; // Base Address Read Only. These read only bits are part of the Base Address and define an aperture size of 64MB.
        uint32_t            base_addr  :  6; // Base Address. This field defines base address bits 31:26 for the device's MMIO space. Used in conjunction with BAR1.
    } field;
    uint32_t val;
} FXR_CFG_BAR0_LOWER_t;

// FXR_CFG_BAR0_UPPER desc:
typedef union {
    struct {
        uint32_t            base_addr  : 32; // Base Address. This field defines base address bits 63:32 for the device's MMIO space. Used in conjunction with BAR0.
    } field;
    uint32_t val;
} FXR_CFG_BAR0_UPPER_t;

// FXR_CFG_SUBSYSTEM_VENDOR desc:
typedef union {
    struct {
        uint32_t        sub_vendor_id  : 16; // Specifies the Subsystem Vendor ID assigned by PCI SIG.
        uint32_t         subsystem_id  : 16; // Specifies the Subsystem ID assigned by device manufacturer.
    } field;
    uint32_t val;
} FXR_CFG_SUBSYSTEM_VENDOR_t;

// FXR_CFG_EXPANSION_ROM desc:
typedef union {
    struct {
        uint32_t               enable  :  1; // Expanstion ROM Enable. When set to 0, Expansion ROM is disabled.
        uint32_t        reserved_10_1  : 10; // Reserved
        uint32_t            base_addr  : 21; // Expansion ROM base address.
    } field;
    uint32_t val;
} FXR_CFG_EXPANSION_ROM_t;

// FXR_CFG_CAPABILITIES desc:
typedef union {
    struct {
        uint32_t              pointer  :  8; // Capabilities Pointer. Pointer to the Expanded Capabilities structure.
        uint32_t        reserved_31_8  : 24; // Reserved
    } field;
    uint32_t val;
} FXR_CFG_CAPABILITIES_t;

// FXR_CFG_INTERRUPT desc:
typedef union {
    struct {
        uint32_t       interrupt_line  :  8; // Interrupt Line. Specifies the IRQx line signaling for FXR. When set to 0xFF, interrupt line signaling is not used.
        uint32_t        interrupt_pin  :  3; // Interrupt Pin. Specifies the INTx pin signaling for FXR. When set to 0x1, INTA in signaled.
        uint32_t       reserved_31_11  : 21; // Reserved
    } field;
    uint32_t val;
} FXR_CFG_INTERRUPT_t;

// FXR_CFG_PM_CAPABILITIES desc:
typedef union {
    struct {
        uint32_t               cap_id  :  8; // Power Management Capabilities ID. Hardcoded ID
        uint32_t              nxt_ptr  :  8; // Next Pointer. Pointer to Next Capabilities Structure.
        uint32_t           pm_version  :  3; // Power Management Version
        uint32_t       reserved_20_19  :  2; // Reserved
        uint32_t        dev_spec_init  :  1; // Device Specific Initialization
        uint32_t          aux_current  :  3; // Auxiliary Current. Not used
        uint32_t                d1sup  :  1; // D1 Support
        uint32_t                d2sup  :  1; // D2 Support
        uint32_t             d0pm_msg  :  1; // PME messages can be generated from D0.
        uint32_t             d1pm_msg  :  1; // PME messages can be generated from D1.
        uint32_t             d2pm_msg  :  1; // PME messages can be generated from D2.
        uint32_t          d3hotpm_msg  :  1; // PME messages can be generated from D3Hot.
        uint32_t         d3coldpm_msg  :  1; // PME messages can be generated from D3Cold
    } field;
    uint32_t val;
} FXR_CFG_PM_CAPABILITIES_t;

// FXR_CFG_PM_STATUS_CTL desc:
typedef union {
    struct {
        uint32_t          power_state  :  2; // Power State. Value may be ignored if specific state is not supported. 0x0 = D0, 0x1 = D1, 0x2 = D2, 0x3 = D3
        uint32_t           reserved_2  :  1; // Reserved
        uint32_t          no_soft_rst  :  1; // No Soft Reset. When set, HW state is not reset after D3Hot exit.
        uint32_t         reserved_7_4  :  4; // Reserved
        uint32_t           pme_enable  :  1; // PME Enable. When set, enables generation of PME
        uint32_t          data_select  :  4; // Not supported
        uint32_t           data_scale  :  2; // Not supported
        uint32_t           pme_status  :  1; // PME Status. Indicates is the previously enabled PME event occurred or not.
        uint32_t       reserved_21_16  :  6; // Reserved
        uint32_t         b2b3_support  :  1; // Not supported
        uint32_t          pwr_clk_ctl  :  1; // Not supported
        uint32_t            data_info  :  8; // Not supported
    } field;
    uint32_t val;
} FXR_CFG_PM_STATUS_CTL_t;

// FXR_CFG_EXP_CAPABILITIES desc:
typedef union {
    struct {
        uint32_t               cap_id  :  8; // PCIe Capabilities ID. Hardcoded PCIe ID
        uint32_t              nxt_ptr  :  8; // Next Pointer. Pointer to Next Capabilities Structure.
        uint32_t          cap_version  :  4; // Capability Version. PCI-SIG defined PCIe Capability structure version number.
        uint32_t          device_type  :  4; // Device/Port Type. Specific type of PCIe Function. 4'b1001 indicates RCIE.
        uint32_t                 slot  :  1; // Slot Implemented. When Set, this bit indicates that the Link associated with this Port is connected to a slot.
        uint32_t        interrupt_msg  :  5; // Interrupt Message Number. This field indicates which MSI-X vector is used for the interrupt message generated in association with any of the status bits of this Capability structure.
        uint32_t       reserved_31_30  :  2; // Reserved
    } field;
    uint32_t val;
} FXR_CFG_EXP_CAPABILITIES_t;

// FXR_CFG_DEVICE_CAPABILITIES desc:
typedef union {
    struct {
        uint32_t          max_payload  :  3; // Max Payload Size. This field indicates the maximum payload size (512B) that the Function can support for TLPs.
        uint32_t         phantom_func  :  2; // Phantom Functions Supported. No phantom function support in FXR.
        uint32_t         extended_tag  :  1; // Extended Tag Field Support. Indicated the maximum supported size of the Tag field as a Requester. FXR supports 8-bit Tags.
        uint32_t          l0s_latency  :  3; // Endpoint L0s Acceptable Latency. Indicated the acceptable total latency that tan Endpoint can withstand due to the transition from L0s to L0 state. FXR latency is 256ns?
        uint32_t           l1_latency  :  3; // Endpoint L1 Acceptable Latency. Indicated the acceptable total latency that tan Endpoint can withstand due to the transition from L1 to L0 state. FXR latency is 1us?
        uint32_t       reserved_14_12  :  3; // Reserved
        uint32_t           role_error  :  1; // Role-Based Error Reporting.
        uint32_t       reserved_17_16  :  2; // Reserved
        uint32_t       slot_pwr_value  :  8; // Captured Slot Power Limit Value.
        uint32_t       slot_pwr_scale  :  2; // Captured Slot Power Limit Scale.
        uint32_t       flr_capability  :  1; // Function Level Reset Capability.
        uint32_t       Reserved_31_29  :  3; // Reserved
    } field;
    uint32_t val;
} FXR_CFG_DEVICE_CAPABILITIES_t;

// FXR_CFG_DEVICE_STAT_CTL desc:
typedef union {
    struct {
        uint32_t    correct_error_ena  :  1; // Correctable Error Reporting Enable. This bit, in conjunction with other bits, controls sending ERR_COR Messages.
        uint32_t   nonfatal_error_ena  :  1; // Non-Fatal Error Reporting Enable. This bit, in conjunction with other bits, controls sending ERR_NONFATAL Messages.
        uint32_t      fatal_error_ena  :  1; // Fatal Error Reporting Enable. This bit, in conjunction with other bits, controls sending ERR_FATAL Messages.
        uint32_t    unsupport_req_ena  :  1; // Unsupported Request Reporting Enable. This bit, in conjunction with other bits, controls the signaling of Unsupported Request Errors by sending error Messages.
        uint32_t      relax_order_ena  :  1; // Relaxed Ordering Enable. If this bit is Set, the Function is permitted to set the Relaxed Ordering bit in the Attributes field of transactions it initiates
        uint32_t          max_payload  :  3; // Max Payload Size. This field sets maximum TLP payload size for the Function.
        uint32_t     extended_tag_ena  :  1; // Extended Tag Field Enable. When Set, this bit enables a Function to use an 8-bit Tag field as a Requester.
        uint32_t     phantom_func_ena  :  1; // Phantom Function Enable. When Set, this bit enables a Function to use unclaimed Functions as Phantom Functions to extend the number of outstanding transaction identifiers.
        uint32_t        aux_power_ena  :  1; // Aux Power PM Enable.
        uint32_t         no_snoop_ena  :  1; // No Snoop Enable. If this bit is Set, the Function is permitted to Set the No Snoop bit in the Requester Attributes of transactions it initiates
        uint32_t             max_read  :  3; // Max Read Request Size. This field sets the maximum Read Request size for the Function as a Requester.
        uint32_t         initiate_flr  :  1; // Initiate Function Level Reset. A write of 1 initiates Function Level Reset to the Function. The value read by software from this bit is always 0.
        uint32_t    correctable_error  :  1; // Correctable Error Detected. This bit indicates status of correctable errors detected.
        uint32_t       nonfatal_error  :  1; // Non-Fatal Error Detected. This bit indicates status of Non-Fatal errors detected.
        uint32_t          fatal_error  :  1; // Fatal Error Detected. This bit indicates status of Fatal errors detected.
        uint32_t    unsupported_error  :  1; // Unsupported Request Detected. This bit indicates the Function received an Unsupported Request.
        uint32_t            aux_power  :  1; // AUX Power Detected. Set if Aux power is detected by the Function
        uint32_t    transactions_pend  :  1; // Transactions Pending. When Set, this bit indicates that the Function has issued NonPosted Requests that have not been completed.
        uint32_t       reserved_31_22  : 10; // Reserved
    } field;
    uint32_t val;
} FXR_CFG_DEVICE_STAT_CTL_t;

// FXR_CFG_DEVICE_CAPABILITIES2 desc:
typedef union {
    struct {
        uint32_t   cpl_timeout_ranges  :  4; // Completion Timeout Ranges. Completion Timeout ranges not supported:Range A: 50us to 10msRange B: 10ms to 250msRange C: 250ms to 4sRange D: 4s to 64s
        uint32_t  cpl_timeout_dis_sup  :  1; // Completion Timeout Disable support.
        uint32_t      ari_forward_sup  :  1; // ARI Forwarding support. Not supported
        uint32_t           reserved_6  :  1; // Reserved
        uint32_t         atomic32_sup  :  1; // 32-bit AtomicOp Completor support. Not supported
        uint32_t         atomic64_sup  :  1; // 64-bit AtomicOp Completor support. Not supported
        uint32_t           cas128_sup  :  1; // 128-bit CAS Support. Not supported
        uint32_t      no_ro_prpr_pass  :  1; // No RO-enabled PR-PR Passing. Not supported
        uint32_t         ltr_mech_sup  :  1; // LTR mechanism support.
        uint32_t         tph_comp_sup  :  2; // TPH Completer Support. Not supported
        uint32_t       reserved_17_14  :  4; // Reserved
        uint32_t             obff_sup  :  2; // OBFF Support. Not supported
        uint32_t    ext_fmt_field_sup  :  1; // Extended Format Field support. Not supported.
        uint32_t   e2e_tlp_prefix_sup  :  1; // End2End TLP Prefix support. Not supported
        uint32_t       max_2e2_prefix  :  2; // Maximum End2End TLP Prefixes. Not supported
        uint32_t       reserved_31_24  :  8; // Reserved
    } field;
    uint32_t val;
} FXR_CFG_DEVICE_CAPABILITIES2_t;

// FXR_CFG_DEVICE_STAT_CTL2 desc:
typedef union {
    struct {
        uint32_t    cpl_timeout_value  :  4; // Completion Timeout Value- 0x0: default range: 50us to 50ms- 0x1: 50us to 100us (range A)- 0x2: 1ms to 10ms (range A)- 0x5: 16ms to 55ms (range B)- 0x6: 65ms to 210ms (range B)- 0x9: 260ms to 900ms (range C)- 0xA: 1s to 3.5s (range C)- 0xD: 4s to 13s (range D)- 0xE: 17s to 64s (range D)- Other values: reserved
        uint32_t      cpl_timeout_dis  :  1; // Completion Timeout Disable
        uint32_t      ari_forward_ena  :  1; // ARI Forwarding Enable. Not supported
        uint32_t     atomic_req_block  :  1; // Atomic Op request blocking. Not supported
        uint32_t     atomic_egr_block  :  1; // Atomic Op egress blocking. Not supported
        uint32_t          ido_req_ena  :  1; // IDO Request Enable. Not supported
        uint32_t           ido_cmp_en  :  1; // IDO Completion Enable. Not supported
        uint32_t          ltr_mech_en  :  1; // LTR Mechanism Enable
        uint32_t       reserved_12_11  :  2; // Reserved
        uint32_t              obff_en  :  2; // OBFF Enable. Not supported
        uint32_t     e2e_prefix_block  :  1; // End to end TLP Prefix Blocking
        uint32_t       reserved_31_16  : 16; // Reserved
    } field;
    uint32_t val;
} FXR_CFG_DEVICE_STAT_CTL2_t;

// FXR_CFG_MSIX_CONTROL desc:
typedef union {
    struct {
        uint32_t               cap_id  :  8; // MSI-X Capabilities ID. Hardcoded MSI-X ID
        uint32_t              nxt_ptr  :  8; // Next Pointer. Pointer to Next Capabilities Structure.
        uint32_t           table_size  : 11; // MISX Table Size. Encoded Table Size as Size-1.
        uint32_t       reserved_29_27  :  3; // Reserved
        uint32_t        function_mask  :  1; // Function Mask. Set to 1 to mask (block) all MSI-X interrupt vector signaling from this function.
        uint32_t          msix_enable  :  1; // MSI-X Enable. Set to 1 to enable MSI-X functionality and disable INTx signaling.
    } field;
    uint32_t val;
} FXR_CFG_MSIX_CONTROL_t;

// FXR_CFG_MSIX_TABLE_OFFSET desc:
typedef union {
    struct {
        uint32_t            table_bir  :  3; // MSI-X Table BIR. BAR that maps the MSI-X Table.
        uint32_t         table_offset  : 29; // MSI-X Table Offset. BAR offset to the start of the MSI-X Table. (PCIM_BASE + TABLE_BASE)>>3
    } field;
    uint32_t val;
} FXR_CFG_MSIX_TABLE_OFFSET_t;

// FXR_CFG_MSIX_PBA_OFFSET desc:
typedef union {
    struct {
        uint32_t              pba_bir  :  3; // MSI-X PBA BIR. BAR that maps the MSI-X Pending Bit Array (PBA).
        uint32_t           pba_offset  : 29; // MSI-X PBA Offset. BAR offset to the start of the MSI-X Pending Bit Array (PBA). (PCIM_BASE + PBA_BASE)>>3
    } field;
    uint32_t val;
} FXR_CFG_MSIX_PBA_OFFSET_t;

// FXR_CFG_VTDBAR0 desc:
typedef union {
    struct {
        uint32_t     VTD_BASE_ADDR_EN  :  1; // VT-d Base Address Enable. Enable for VT-d Base address.
        uint32_t        reserved_11_1  : 11; // Reserved
        uint32_t        VTD_BASE_ADDR  : 20; // VT-d Base Address. Provides an aligned 4K base address for IIO registers relating to VT-d.
    } field;
    uint32_t val;
} FXR_CFG_VTDBAR0_t;

// FXR_CFG_VTDBAR1 desc:
typedef union {
    struct {
        uint32_t        VTD_BASE_ADDR  : 32; // VT-d Base Address. Upper 32 bits of VT-d Base Address Specifically, bits [63:32].
    } field;
    uint32_t val;
} FXR_CFG_VTDBAR1_t;

// FXR_CFG_AER_EXT_CAPABILITY desc:
typedef union {
    struct {
        uint32_t               cap_id  : 16; // PCIe Capabilities ID. Hardcoded PCIe ID
        uint32_t          cap_version  :  4; // Capability Version. PCI-SIG defined PCIe Capability structure version number.
        uint32_t      next_capability  : 12; // Next Capability Offset. Offset to the next PCI Express Extended Capability structure
    } field;
    uint32_t val;
} FXR_CFG_AER_EXT_CAPABILITY_t;

// FXR_CFG_AER_UNCOR_ERR_STATUS desc:
typedef union {
    struct {
        uint32_t         reserved_3_0  :  4; // Reserved
        uint32_t             dllp_err  :  1; // Data Link Layer Packet Error. Not supported
        uint32_t        surp_down_err  :  1; // Surprise Down Error. Not supported
        uint32_t        reserved_11_6  :  6; // Reserved
        uint32_t       poison_tlp_err  :  1; // Poisoned TLP Error.
        uint32_t               fc_err  :  1; // Flow Control Protocol Error.
        uint32_t     cmpl_timeout_err  :  1; // Completion Timeout Error.
        uint32_t       cmpl_abort_err  :  1; // Completion Abort Error.
        uint32_t        unexp_cpl_err  :  1; // Unexpected Completion Error.
        uint32_t      rcv_overflw_err  :  1; // Receiver Overflow Error.
        uint32_t          mal_tlp_err  :  1; // Malformed TLP Error.
        uint32_t             ecrc_err  :  1; // ERCR Error. Not supported
        uint32_t        unsup_req_err  :  1; // Unsupported Request Error
        uint32_t    acs_violation_err  :  1; // ACS Violation Error. Not supported
        uint32_t       uncorr_int_err  :  1; // Uncorrectable Internal Error.
        uint32_t       mc_blk_tlp_err  :  1; // MC Blocked TLP Error. Not supported
        uint32_t   atomic_egr_blk_err  :  1; // Atomic Op Egress Blocked Error. Not supported
        uint32_t   tlp_prefix_blk_err  :  1; // TLP Prefix Blocked Error. Not supported.
        uint32_t       reserved_31_26  :  6; // Reserved
    } field;
    uint32_t val;
} FXR_CFG_AER_UNCOR_ERR_STATUS_t;

// FXR_CFG_AER_UNCOR_ERR_MASK desc:
typedef union {
    struct {
        uint32_t         reserved_3_0  :  4; // Reserved
        uint32_t             dllp_err  :  1; // Data Link Layer Packet Error. Not supported
        uint32_t        surp_down_err  :  1; // Surprise Down Error. Not supported
        uint32_t        reserved_11_6  :  6; // Reserved
        uint32_t       poison_tlp_err  :  1; // Poisoned TLP Error.
        uint32_t               fc_err  :  1; // Flow Control Protocol Error.
        uint32_t     cmpl_timeout_err  :  1; // Completion Timeout Error.
        uint32_t       cmpl_abort_err  :  1; // Completion Abort Error,
        uint32_t        unexp_cpl_err  :  1; // Unexpected Completion Error.
        uint32_t      rcv_overflw_err  :  1; // Receiver Overflow Error.
        uint32_t          mal_tlp_err  :  1; // Malformed TLP Error.
        uint32_t             ecrc_err  :  1; // ERCR Error. Not supported
        uint32_t        unsup_req_err  :  1; // Unsupported Request Error
        uint32_t    acs_violation_err  :  1; // ACS Violation Error. Not supported
        uint32_t       uncorr_int_err  :  1; // Uncorrectable Internal Error.
        uint32_t       mc_blk_tlp_err  :  1; // MC Blocked TLP Error. Not supported
        uint32_t   atomic_egr_blk_err  :  1; // Atomic Op Egress Blocked Error. Not supported
        uint32_t   tlp_prefix_blk_err  :  1; // TLP Prefix Blocked Error. Not supported.
        uint32_t       reserved_31_26  :  6; // Reserved
    } field;
    uint32_t val;
} FXR_CFG_AER_UNCOR_ERR_MASK_t;

// FXR_CFG_AER_UNCOR_ERR_SEVRTY desc:
typedef union {
    struct {
        uint32_t         reserved_3_0  :  4; // Reserved
        uint32_t             dllp_err  :  1; // Data Link Layer Packet Error. Not supported
        uint32_t        surp_down_err  :  1; // Surprise Down Error. Not supported
        uint32_t        reserved_11_6  :  6; // Reserved
        uint32_t       poison_tlp_err  :  1; // Poisoned TLP Error.
        uint32_t               fc_err  :  1; // Flow Control Protocol Error.
        uint32_t     cmpl_timeout_err  :  1; // Completion Timeout Error.
        uint32_t       cmpl_abort_err  :  1; // Completion Abort Error,
        uint32_t        unexp_cpl_err  :  1; // Unexpected Completion Error.
        uint32_t      rcv_overflw_err  :  1; // Receiver Overflow Error.
        uint32_t          mal_tlp_err  :  1; // Malformed TLP Error.
        uint32_t             ecrc_err  :  1; // ERCR Error. Not supported
        uint32_t        unsup_req_err  :  1; // Unsupported Request Error
        uint32_t    acs_violation_err  :  1; // ACS Violation Error. Not supported
        uint32_t       uncorr_int_err  :  1; // Uncorrectable Internal Error.
        uint32_t       mc_blk_tlp_err  :  1; // MC Blocked TLP Error. Not supported
        uint32_t   atomic_egr_blk_err  :  1; // Atomic Op Egress Blocked Error. Not supported
        uint32_t   tlp_prefix_blk_err  :  1; // TLP Prefix Blocked Error. Not supported.
        uint32_t       reserved_31_26  :  6; // Reserved
    } field;
    uint32_t val;
} FXR_CFG_AER_UNCOR_ERR_SEVRTY_t;

// FXR_CFG_AER_CORR_ERR_STATUS desc:
typedef union {
    struct {
        uint32_t              rcv_err  :  1; // Receiver Error
        uint32_t         reserved_5_1  :  5; // Reserved
        uint32_t          bad_tlp_err  :  1; // Bad TLP Error.
        uint32_t           reserved_7  :  1; // Reserved (Bad DLLP Error not supported)
        uint32_t           reserved_8  :  1; // Reserved (replay rollover error not supported)
        uint32_t        reserved_11_9  :  3; // Reserved
        uint32_t          reserved_12  :  1; // Reserved (replay timeout error not supported)
        uint32_t advisory_nonfatal_err  :  1; // Advisory Non-Fatal Error
        uint32_t         corr_int_err  :  1; // Corrected Internal Error.
        uint32_t          reserved_15  :  1; // Reserved (header log overflow error not supported)
        uint32_t       reserved_31_16  : 16; // Reserved
    } field;
    uint32_t val;
} FXR_CFG_AER_CORR_ERR_STATUS_t;

// FXR_CFG_AER_CORR_ERR_MASK desc:
typedef union {
    struct {
        uint32_t              rcv_err  :  1; // Receiver Error
        uint32_t         reserved_5_1  :  5; // Reserved
        uint32_t          bad_tlp_err  :  1; // Bad TLP Error.
        uint32_t           reserved_7  :  1; // Reserved (Bad DLLP Error not supported)
        uint32_t           reserved_8  :  1; // Reserved (replay rollover error not supported)
        uint32_t        reserved_11_9  :  3; // Reserved
        uint32_t          reserved_12  :  1; // Reserved (replay timeout error not supported)
        uint32_t advisory_nonfatal_err  :  1; // Advisory Non-Fatal Error
        uint32_t         corr_int_err  :  1; // Corrected Internal Error.
        uint32_t          reserved_15  :  1; // Reserved (header log overflow error not supported)
        uint32_t       reserved_31_16  : 16; // Reserved
    } field;
    uint32_t val;
} FXR_CFG_AER_CORR_ERR_MASK_t;

// FXR_CFG_AER_ADV_CAP_CTL desc:
typedef union {
    struct {
        uint32_t       first_err_pntr  :  5; // First Error Pointer. Identifies the bit position of the first error reported in the Uncorrectable Error Status Register
        uint32_t         ecrc_gen_cap  :  1; // ECRC Generation Capable. ECRC is not supported
        uint32_t          ecrc_gen_en  :  1; // ECRC Generation Enable. Not supported
        uint32_t         ecrc_chk_cap  :  1; // ECRC Check Capable. ECRC is not supported
        uint32_t          ecrc_chk_en  :  1; // ECRC Check Enable. Not supported
        uint32_t        reserved_10_9  :  2; // Reserved (multiple header recording not supported)
        uint32_t  tlp_prefix_log_pres  :  1; // TLP prefix log present. End-end TLP prefix is not supported so this bit is hard-wired to 0. This means that the AER registers for TLP prefix logging are never valid. They will always retain their reset value of 0x0
        uint32_t cmpl_timeout_hdr_log_capable  :  1; // Reserved
        uint32_t       reserved_31_13  : 19; // Reserved
    } field;
    uint32_t val;
} FXR_CFG_AER_ADV_CAP_CTL_t;

// FXR_CFG_AER_HEADER_LOG0 desc:
typedef union {
    struct {
        uint32_t          header_log0  : 32; // Header Log0.
    } field;
    uint32_t val;
} FXR_CFG_AER_HEADER_LOG0_t;

// FXR_CFG_AER_HEADER_LOG1 desc:
typedef union {
    struct {
        uint32_t          header_log1  : 32; // Header Log1.
    } field;
    uint32_t val;
} FXR_CFG_AER_HEADER_LOG1_t;

// FXR_CFG_AER_HEADER_LOG2 desc:
typedef union {
    struct {
        uint32_t          header_log2  : 32; // Header Log2.
    } field;
    uint32_t val;
} FXR_CFG_AER_HEADER_LOG2_t;

// FXR_CFG_AER_HEADER_LOG3 desc:
typedef union {
    struct {
        uint32_t          header_log3  : 32; // Header Log3.
    } field;
    uint32_t val;
} FXR_CFG_AER_HEADER_LOG3_t;

// FXR_CFG_AER_TLP_PRE_LOG0 desc:
typedef union {
    struct {
        uint32_t      tlp_prefix_log0  : 32; // TLP Prefix Log. End-end TLP prefix is not supported so this register is hard-wired to 0.
    } field;
    uint32_t val;
} FXR_CFG_AER_TLP_PRE_LOG0_t;

// FXR_CFG_AER_TLP_PRE_LOG1 desc:
typedef union {
    struct {
        uint32_t      tlp_prefix_log1  : 32; // TLP Prefix Log. End-end TLP prefix is not supported so this register is hard-wired to 0.
    } field;
    uint32_t val;
} FXR_CFG_AER_TLP_PRE_LOG1_t;

// FXR_CFG_AER_TLP_PRE_LOG2 desc:
typedef union {
    struct {
        uint32_t      tlp_prefix_log2  : 32; // IOSF-P TLP Prefix Log. End-end TLP prefix is not supported so this register is hard-wired to 0.
    } field;
    uint32_t val;
} FXR_CFG_AER_TLP_PRE_LOG2_t;

// FXR_CFG_AER_TLP_PRE_LOG3 desc:
typedef union {
    struct {
        uint32_t      tlp_prefix_log3  : 32; // IOSF-P TLP Prefix Log. End-end TLP prefix is not supported so this register is hard-wired to 0.
    } field;
    uint32_t val;
} FXR_CFG_AER_TLP_PRE_LOG3_t;

// FXR_CFG_LTR_CAPABILITY desc:
typedef union {
    struct {
        uint32_t               cap_id  : 16; // PCIe Capabilities ID. Hardcoded PCIe ID
        uint32_t          cap_version  :  4; // Capability Version. PCI-SIG defined PCIe Capability structure version number.
        uint32_t      next_capability  : 12; // Next Capability Offset. Offset to the next PCI Express Extended Capability structure
    } field;
    uint32_t val;
} FXR_CFG_LTR_CAPABILITY_t;

// FXR_CFG_SNOOP_LATENCY desc:
typedef union {
    struct {
        uint32_t      snoop_lat_value  : 10; // Max Snoop Latency Value. Specifies the maximum snoop latency that a device is permitted to request.
        uint32_t      snoop_lat_scale  :  3; // Max Snoop Latency Scale. Specifies a scale for the value contained within the Maximum Snoop Latency Value field.
        uint32_t       reserved_14_13  :  2; // Reserved
        uint32_t        snoop_lat_req  :  1; // Max Snoop Latency Requirement. When set, the Snoop Latency Scale and Value fields are valid.
        uint32_t    nosnoop_lat_value  : 10; // Max No-Snoop Latency Value. Specifies the maximum no-snoop latency that a device is permitted to request.
        uint32_t    nosnoop_lat_scale  :  3; // Max No-Snoop Latency Scale. Specifies a scale for the value contained within the Maximum No-Snoop Latency Value field.
        uint32_t       reserved_30_29  :  2; // Reserved
        uint32_t      nosnoop_lat_req  :  1; // Max No-Snoop Latency Requirement. When set, the No-Snoop Latency Scale and Value fields are valid.
    } field;
    uint32_t val;
} FXR_CFG_SNOOP_LATENCY_t;

// FXR_CFG_PASID_CAPABILITY desc:
typedef union {
    struct {
        uint32_t               cap_id  : 16; // PASID Extended Capabilities ID. Hardcoded ID
        uint32_t          cap_version  :  4; // Capability Version. PCI-SIG defined PCIe Capability structure version number.
        uint32_t      next_capability  : 12; // Next Capability Offset. Offset to the next PCI Express Extended Capability structure
    } field;
    uint32_t val;
} FXR_CFG_PASID_CAPABILITY_t;

// FXR_CFG_PASID_CONTROL desc:
typedef union {
    struct {
        uint32_t           reserved_0  :  1; // Reserved
        uint32_t  exec_permission_sup  :  1; // Execute Permission Supported. When set, Endpoint supports sending TLPs that have Execute Requested bit set.
        uint32_t        priv_mode_sup  :  1; // Privileged Mode Supported. When set, Endpoint supports operating in Privileged and non-Privileged modes and supports sending requests that have Privileged Mode Requested bit set.
        uint32_t         reserved_7_3  :  5; // Reserved
        uint32_t      max_pasid_width  :  5; // Max PASID Width. Indicated the width of the PASID field supported by the Endpoint. The value n indicated support of PASID values 0 through 2n-1 (inclusive). Value must be set between 0 and 20 (inclusive)
        uint32_t       reserved_15_13  :  3; // Reserved
        uint32_t         pasid_enable  :  1; // PASID Enable. When set, FXR is enabled to send translation requests to the IOMMU.
        uint32_t          reserved_17  :  1; // Reserved (execute permission is not supported)
        uint32_t     priv_mode_enable  :  1; // Privileged Mode Enable. When set, FXR is enabled to send translation requests with privileged mode requested bit set to the IOMMU.
        uint32_t       reserved_31_19  : 13; // Reserved
    } field;
    uint32_t val;
} FXR_CFG_PASID_CONTROL_t;

// FXR_CFG_ATS_CAPABILITY desc:
typedef union {
    struct {
        uint32_t               cap_id  : 16; // PASID Extended Capabilities ID. Hardcoded ID
        uint32_t          cap_version  :  4; // Capability Version. PCI-SIG defined PCIe Capability structure version number.
        uint32_t      next_capability  : 12; // Next Capability Offset. Offset to the next PCI Express Extended Capability structure
    } field;
    uint32_t val;
} FXR_CFG_ATS_CAPABILITY_t;

// FXR_CFG_ATS_CONTROL desc:
typedef union {
    struct {
        uint32_t    inval_queue_depth  :  5; // Invalidate Queue Depth. Number of invalidate requests the Function can be accepted before asserting back pressure.
        uint32_t     page_aligned_req  :  1; // Page Aligned Request. When set, the untranslated address is always aligned to a 4K byte boundary.
        uint32_t     global_inval_sup  :  1; // Global Invalidate Supported. When set, the Function supports invalidation requests that have the Global invalidate bit set.
        uint32_t        reserved_15_7  :  9; // Reserved
        uint32_t                  stu  :  5; // Smallest Translation Unit. The value indicates to the Function the minimum number of 4096-byte blocks that is indicated in a Translation Completion or Invalidate Request. This is a power of 2 mutiplier and the number of blocks is 2STU. A value of 0x0 indicates one block.
        uint32_t       reserved_30_21  : 10; // Reserved
        uint32_t               enable  :  1; // Enable. When set, FXR is enabled to cache translations.
    } field;
    uint32_t val;
} FXR_CFG_ATS_CONTROL_t;

// FXR_CFG_PRS_CAPABILITY desc:
typedef union {
    struct {
        uint32_t               cap_id  : 16; // PASID Extended Capabilities ID. Hardcoded ID
        uint32_t          cap_version  :  4; // Capability Version. PCI-SIG defined PCIe Capability structure version number.
        uint32_t      next_capability  : 12; // Next Capability Offset. Offset to the next PCI Express Extended Capability structure
    } field;
    uint32_t val;
} FXR_CFG_PRS_CAPABILITY_t;

// FXR_CFG_PRS_STAT_CTL desc:
typedef union {
    struct {
        uint32_t               enable  :  1; // Enable. When set, indicates the Page Request Interface is allowed to make page requests.
        uint32_t                reset  :  1; // Reset. Not applicable to FXR implementation.
        uint32_t        reserved_15_2  : 14; // Reserved
        uint32_t        response_fail  :  1; // Response Failure. FXR sets this bit when it receives a page_grp_resp_dsc or page_stream_resp_dsc with Response Code of Response Failure (1111b). The queue associated with that PASID in such response is terminated with error. When Page-Request Enable (PRE) field in the Pagerequest Control register transitions from 0 to 1, this field is Cleared
        uint32_t                uprgi  :  1; // Unexpected Page Request Group Index. FXR sets this bit when it receives a page_grp_resp_dsc with PRG Index that does not match PRG index in any outstanding page_grp_req_dsc. Such page_grp_resp_dsc is ignored. When Page-Request Enable (PRE) field in the Page request Control register transitions from 0 to 1, this field is Cleared.
        uint32_t       reserved_23_18  :  6; // Reserved
        uint32_t              stopped  :  1; // Stopped. FXR has no direct use of this field. For compatibility reasons, this field is Set when Page-Request Enable (PRE) field in the Page-request Control register transitions from 1 to 0. When PRE transitions from 0 to 1, this field is cleared.
        uint32_t       reserved_30_25  :  6; // Reserved
        uint32_t    prg_rsp_pasid_req  :  1; // PRG Response PASID Required. Not used by FXR. Set to 1 for compatibility.
    } field;
    uint32_t val;
} FXR_CFG_PRS_STAT_CTL_t;

// FXR_CFG_PRS_REQUEST_CAP desc:
typedef union {
    struct {
        uint32_t  outstand_pg_req_cap  : 32; // Outstanding Page Request Capacity. Maximum number of pending page requests supported by FXR.
    } field;
    uint32_t val;
} FXR_CFG_PRS_REQUEST_CAP_t;

// FXR_CFG_PRS_REQUEST_ALLOC desc:
typedef union {
    struct {
        uint32_t outstand_pg_req_alloc  : 32; // Outstanding Page Request Alocation. Maximum limit of pending page requests allowed by FXR.
    } field;
    uint32_t val;
} FXR_CFG_PRS_REQUEST_ALLOC_t;

#endif /* ___FXR_config_CSRS_H__ */