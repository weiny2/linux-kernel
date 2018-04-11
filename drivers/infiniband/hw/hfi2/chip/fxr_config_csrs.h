// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Thu Apr  5 22:07:49 2018
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
        uint32_t     memory_space_ena  :  1; // Memory Space Enable. Controls the device's response to Memory Space accesses. A value of 0 disabled the device response. A value of 1 allows the device to respond to Memory Space access.
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
        uint32_t        Reserved_10_1  : 10; // Unused
        uint32_t            base_addr  : 21; // Expansion ROM base address.
    } field;
    uint32_t val;
} FXR_CFG_EXPANSION_ROM_t;

// FXR_CFG_CAPABILITIES desc:
typedef union {
    struct {
        uint32_t              pointer  :  8; // Capabilities Pointer. Pointer to the Expanded Capabilities structure.
        uint32_t        Reserved_31_8  : 24; // Unused
    } field;
    uint32_t val;
} FXR_CFG_CAPABILITIES_t;

// FXR_CFG_INTERRUPT desc:
typedef union {
    struct {
        uint32_t       interrupt_line  :  8; // Interrupt Line. Specifies the IRQx line signaling for FXR. When set to 0xFF, interrupt line signaling is not used.
        uint32_t        interrupt_pin  :  8; // Interrupt Pin. Specifies the INTx pin signaling for FXR. When set to 0x1, INTA in signaled.
        uint32_t       Reserved_31_16  : 16; // Unused
    } field;
    uint32_t val;
} FXR_CFG_INTERRUPT_t;

// FXR_CFG_PM_CAPABILITIES desc:
typedef union {
    struct {
        uint32_t               cap_id  :  8; // Power Management Capabilities ID. Hardcoded ID
        uint32_t              nxt_ptr  :  8; // Next Pointer. Pointer to Next Capabilities Structure.
        uint32_t           pm_version  :  3; // Power Management Version
        uint32_t              pme_clk  :  1; // PCI Clock Requirement
        uint32_t          Reserved_20  :  1; // Unused
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
        uint32_t           Reserved_2  :  1; // Unused
        uint32_t          no_soft_rst  :  1; // No Soft Reset. When set, HW state is not reset after D3Hot exit.
        uint32_t         Reserved_7_4  :  4; // Unused
        uint32_t           pme_enable  :  1; // PME Enable. When set, enables generation of PME
        uint32_t          data_select  :  4; // Not supported
        uint32_t           data_scale  :  2; // Not supported
        uint32_t           pme_status  :  1; // PME Status. Indicates if the previously enabled PME event occurred or not.
        uint32_t       Reserved_21_16  :  6; // Unused
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
        uint32_t          device_type  :  4; // Device/Port Type. Specific type of PCIe Function.
        uint32_t                 slot  :  1; // Slot Implemented. When Set, this bit indicates that the Link associated with this Port is connected to a slot.
        uint32_t        interrupt_msg  :  5; // Interrupt Message Number. This field indicates which MSI-X vector is used for the interrupt message generated in association with any of the status bits of this Capability structure.
        uint32_t       Reserved_31_30  :  2; // Unused
    } field;
    uint32_t val;
} FXR_CFG_EXP_CAPABILITIES_t;

// FXR_CFG_DEVICE_CAPABILITIES desc:
typedef union {
    struct {
        uint32_t          max_payload  :  3; // Max Payload Size. This field indicates the maximum payload size (512B) that the Function can support for TLPs.
        uint32_t         phantom_func  :  2; // Phantom Functions Supported. No phantom function support in FXR.
        uint32_t         extended_tag  :  1; // Extended Tag Field Support. Indicated the maximum supported size of the Tag field as a Requester.
        uint32_t          l0s_latency  :  3; // Endpoint L0s Acceptable Latency. Indicated the acceptable total latency that tan Endpoint can withstand due to the transition from L0s to L0 state.
        uint32_t           l1_latency  :  3; // Endpoint L1 Acceptable Latency. Indicated the acceptable total latency that tan Endpoint can withstand due to the transition from L1 to L0 state.
        uint32_t       reserved_14_12  :  3; // Reserved
        uint32_t           role_error  :  1; // Role-Based Error Reporting.
        uint32_t       reserved_17_16  :  2; // Reserved
        uint32_t       slot_pwr_value  :  8; // Captured Slot Power Limit Value.
        uint32_t       slot_pwr_scale  :  2; // Captured Slot Power Limit Scale.
        uint32_t       flr_capability  :  1; // Function Level Reset Capability.
        uint32_t       Reserved_31_29  :  3; // Unused
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
        uint32_t         initiate_flr  :  1; // Initiate Function Level Reset. A write of 1 initiates Function Level Reset to the Function.
        uint32_t          Reserved_16  :  1; // Unused
        uint32_t       nonfatal_error  :  1; // Non-Fatal Error Detected. This bit indicates status of Non-Fatal errors detected.
        uint32_t          Reserved_18  :  1; // Unused
        uint32_t    unsupported_error  :  1; // Unsupported Request Detected. This bit indicates the Function received an Unsupported Request.
        uint32_t            aux_power  :  1; // AUX Power Detected. Set if Aux power is detected by the Function
        uint32_t    transactions_pend  :  1; // Transactions Pending. When Set, this bit indicates that the Function has pending transactions over or IDI.
        uint32_t       Reserved_31_22  : 10; // Unused
    } field;
    uint32_t val;
} FXR_CFG_DEVICE_STAT_CTL_t;

// FXR_CFG_LINK_CAPABILITIES desc:
typedef union {
    struct {
        uint32_t       max_link_speed  :  4; // Maximum Link Speed
        uint32_t       max_link_width  :  6; // Maximum Link Width
        uint32_t active_state_link_pm  :  2; // Level of ASPM Support
        uint32_t     l0s_exit_latency  :  3; // L0s Exit Latency
        uint32_t      l1_exit_latency  :  3; // L1 Exit Latency.
        uint32_t      clock_power_man  :  1; // Clock Power Management.
        uint32_t surprise_down_err_rep  :  1; // Surprise Down Error Reporting Capable.
        uint32_t    dll_active_report  :  1; // Data Link Layer Link Active Reporting Capable
        uint32_t link_bw_notification  :  1; // Link Bandwidth Notification Capable.
        uint32_t             aspm_opt  :  1; // ASPM Optionality Compliance
        uint32_t          Reserved_23  :  1; // Unused
        uint32_t             port_num  :  8; // Port Number
    } field;
    uint32_t val;
} FXR_CFG_LINK_CAPABILITIES_t;

// FXR_CFG_LINK_STATUS_CTL desc:
typedef union {
    struct {
        uint32_t active_state_link_pm_ctl  :  2; // Active State Power Management (ASPM) Control.
        uint32_t           Reserved_2  :  1; // Unused
        uint32_t              cap_rcb  :  1; // Read Completion Boundary (RCB).
        uint32_t         link_disable  :  1; // Initiate Link Disable.
        uint32_t         retrain_link  :  1; // Initiate Link Retrain
        uint32_t    common_clk_config  :  1; // Common Clock Configuration.
        uint32_t       extended_synch  :  1; // Extended Synch.
        uint32_t     en_clk_power_man  :  1; // Enable Clock Power Management.
        uint32_t hw_auto_width_disable  :  1; // Hardware Autonomous Width Disable.
        uint32_t   link_bw_man_int_en  :  1; // Link Bandwidth Management Interrupt Enable.
        uint32_t  link_auto_bw_int_en  :  1; // Link Autonomous Bandwidth Management Interrupt Enable.
        uint32_t       Reserved_13_12  :  2; // Unused
        uint32_t drs_signaling_control  :  2; // DRS Signaling Control.
        uint32_t           link_speed  :  4; // Current Link Speed.
        uint32_t      nego_link_width  :  6; // Negotiated Link Width
        uint32_t          Reserved_26  :  1; // Unused
        uint32_t         link_traning  :  1; // LTSSM is in Configuration or Recovery State.
        uint32_t      slot_clk_config  :  1; // Slot Clock Configuration
        uint32_t           dll_active  :  1; // Data Link Layer Active
        uint32_t   link_bw_man_status  :  1; // Link Bandwidth Management Status
        uint32_t  link_auto_bw_status  :  1; // Link Autonomous Bandwidth Status.
    } field;
    uint32_t val;
} FXR_CFG_LINK_STATUS_CTL_t;

// FXR_CFG_DEVICE_CAPABILITIES2 desc:
typedef union {
    struct {
        uint32_t   cpl_timeout_ranges  :  4; // Completion Timeout Ranges Supported. Completion Timeout ranges:Range A: 50us to 10msRange B: 10ms to 250msRange C: 250ms to 4sRange D: 4s to 64s
        uint32_t  cpl_timeout_dis_sup  :  1; // Completion Timeout Disable support.
        uint32_t      ari_forward_sup  :  1; // ARI Forwarding support.
        uint32_t     atomic_route_sup  :  1; // Atomic Operation Routing Supported.
        uint32_t         atomic32_sup  :  1; // 32-bit AtomicOp Completor support.
        uint32_t         atomic64_sup  :  1; // 64-bit AtomicOp Completor support.
        uint32_t           cas128_sup  :  1; // 128-bit CAS Support.
        uint32_t      no_ro_prpr_pass  :  1; // No RO-enabled PR-PR Passing.
        uint32_t         ltr_mech_sup  :  1; // LTR mechanism support.
        uint32_t        tph_comp_sup0  :  1; // TPH Completer Supported Bit 0
        uint32_t        tph_comp_sup1  :  1; // TPH Completer Supported Bit 1
        uint32_t           ln_sys_cls  :  2; // LN System CLS.
        uint32_t  tenbit_tag_comp_sup  :  1; // 10-Bit Tag Completer Supported.
        uint32_t   tenbit_tag_req_sup  :  1; // 10-Bit Tag Requester Supported.
        uint32_t             obff_sup  :  2; // OBFF Support.
        uint32_t    ext_fmt_field_sup  :  1; // Extended Format Field support.
        uint32_t   e2e_tlp_prefix_sup  :  1; // End2End TLP Prefix support.
        uint32_t       max_2e2_prefix  :  2; // Maximum End2End TLP Prefixes.
        uint32_t       Reserved_31_24  :  8; // Unused
    } field;
    uint32_t val;
} FXR_CFG_DEVICE_CAPABILITIES2_t;

// FXR_CFG_DEVICE_STAT_CTL2 desc:
typedef union {
    struct {
        uint32_t    cpl_timeout_value  :  4; // Completion Timeout Value- 0x0: default range: 50us to 50ms- 0x1: 50us to 100us (range A)- 0x2: 1ms to 10ms (range A)- 0x5: 16ms to 55ms (range B)- 0x6: 65ms to 210ms (range B)- 0x9: 260ms to 900ms (range C)- 0xA: 1s to 3.5s (range C)- 0xD: 4s to 13s (range D)- 0xE: 17s to 64s (range D)- Other values: reserved
        uint32_t      cpl_timeout_dis  :  1; // Completion Timeout Disable
        uint32_t      ari_forward_ena  :  1; // ARI Forwarding Enable.
        uint32_t     atomic_req_block  :  1; // Atomic Op request blocking.
        uint32_t     atomic_egr_block  :  1; // Atomic Op egress blocking.
        uint32_t          ido_req_ena  :  1; // IDO Request Enable.
        uint32_t           ido_cmp_en  :  1; // IDO Completion Enable.
        uint32_t          ltr_mech_en  :  1; // LTR Mechanism Enable
        uint32_t          Reserved_11  :  1; // Unused
        uint32_t    tenbit_tag_req_en  :  1; // 10-Bit Tag Requester Enable.
        uint32_t              obff_en  :  2; // OBFF Enable.
        uint32_t     e2e_prefix_block  :  1; // End to end TLP Prefix Blocking
        uint32_t       Reserved_31_16  : 16; // Unused
    } field;
    uint32_t val;
} FXR_CFG_DEVICE_STAT_CTL2_t;

// FXR_CFG_LINK_CAPABILITIES2 desc:
typedef union {
    struct {
        uint32_t           Reserved_0  :  1; // Unused
        uint32_t   sup_link_speed_vec  :  7; // Supported Link Speeds Vector.
        uint32_t       cross_link_sup  :  1; // Cross Link Supported.
        uint32_t        Reserved_22_9  : 14; // Unused
        uint32_t      retimer_pre_det  :  1; // Retimer Presence Detect Supported.
        uint32_t two_retimers_pre_det  :  1; // Two Retimers Presence Detect Supported.
        uint32_t       Reserved_31_25  :  7; // Unused
    } field;
    uint32_t val;
} FXR_CFG_LINK_CAPABILITIES2_t;

// FXR_CFG_LINK_STAT_CTL2 desc:
typedef union {
    struct {
        uint32_t    target_link_speed  :  4; // Target Link Speed.
        uint32_t     enter_compliance  :  1; // Enter Compliance Mode
        uint32_t hw_auto_speed_disable  :  1; // Hardware Autonomous Speed Disable.
        uint32_t       sel_deemphasis  :  1; // Controls Selectable De-emphasis for 5 GT/s.
        uint32_t            tx_margin  :  3; // Controls Transmit Margin for Debug or Compliance.
        uint32_t ent_modified_compliance  :  1; // Enter Modified Compliance.
        uint32_t       compliance_sos  :  1; // Sets Compliance Skip Ordered Sets transmission.
        uint32_t   compliance_present  :  4; // Sets Compliance Preset/De-emphasis for 5 GT/s and 8 GT/s.
        uint32_t          curr_deemph  :  1; // Current De-emphasis Level
        uint32_t               eq_cpl  :  1; // Equalization 8.0GT/s Complete
        uint32_t            eq_cpl_p1  :  1; // Equalization 8.0GT/s Phase 1Successful.
        uint32_t            eq_cpl_p2  :  1; // Equalization 8.0GT/s Phase 2 Successful.
        uint32_t            eq_cpl_p3  :  1; // Equalization 8.0GT/s Phase 3 Successful.
        uint32_t          link_eq_req  :  1; // Link Equalization Request 8.0GT/s.
        uint32_t      retimer_pre_det  :  1; // Retimer Presence Detected
        uint32_t two_retimers_pre_det  :  1; // Two Retimers Presence Detected.
        uint32_t       Reserved_27_24  :  4; // Unused
        uint32_t  downstrm_compo_pres  :  3; // Downstream Component Presence.
        uint32_t          Reserved_31  :  1; // Unused
    } field;
    uint32_t val;
} FXR_CFG_LINK_STAT_CTL2_t;

// FXR_CFG_MSIX_CONTROL desc:
typedef union {
    struct {
        uint32_t               cap_id  :  8; // MSI-X Capabilities ID. Hardcoded MSI-X ID
        uint32_t              nxt_ptr  :  8; // Next Pointer. Pointer to Next Capabilities Structure.
        uint32_t           table_size  : 11; // MISX Table Size. Encoded Table Size as Size-1.
        uint32_t       Reserved_29_27  :  3; // Unused
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
        uint32_t         Reserved_3_0  :  4; // Unused
        uint32_t             dllp_err  :  1; // Data Link Protocol Error.
        uint32_t        surp_down_err  :  1; // Surprise Down Error.
        uint32_t        Reserved_11_6  :  6; // Unused
        uint32_t       poison_tlp_err  :  1; // Poisoned TLP Error. Posted Data Parity Error/Poison to P2SB NP Data Parity/Poison to CFG
        uint32_t               fc_err  :  1; // Flow Control Protocol Error.
        uint32_t     cmpl_timeout_err  :  1; // Completion Timeout Error. Pending request timeout
        uint32_t       cmpl_abort_err  :  1; // Completion Abort Error. Illegal Length on MRd Illegal Length on CfgRd
        uint32_t        unexp_cpl_err  :  1; // Unexpected Completion Error. Unexpected completion
        uint32_t      rcv_overflw_err  :  1; // Receiver Overflow Error.
        uint32_t          mal_tlp_err  :  1; // Malformed TLP Error. Illegal Length on MWr Illegal Length on CfgWr Header Parity Error
        uint32_t             ecrc_err  :  1; // ERCR Error.
        uint32_t        unsup_req_err  :  1; // Unsupported Request Error Posted Unsupported opcode Posted illegal PCIe Byte Enables Non-posted Unsupported opcode Non-posted illegal PCIe Byte Enables
        uint32_t    acs_violation_err  :  1; // ACS Violation Error.
        uint32_t       uncorr_int_err  :  1; // Uncorrectable Internal Error.
        uint32_t          Reserved_23  :  1; // Unused
        uint32_t   atomic_egr_blk_err  :  1; // Atomic Op Egress Blocked Error.
        uint32_t   tlp_prefix_blk_err  :  1; // TLP Prefix Blocked Error.
        uint32_t       Reserved_31_26  :  6; // Unused
    } field;
    uint32_t val;
} FXR_CFG_AER_UNCOR_ERR_STATUS_t;

// FXR_CFG_AER_UNCOR_ERR_MASK desc:
typedef union {
    struct {
        uint32_t         Reserved_3_0  :  4; // Unused
        uint32_t        dllp_err_mask  :  1; // Data Link Protocol Error Mask.
        uint32_t        surp_down_err  :  1; // Surprise Down Error Mask.
        uint32_t        Reserved_11_6  :  6; // Unused
        uint32_t       poison_tlp_err  :  1; // Poisoned TLP Error Mask.
        uint32_t               fc_err  :  1; // Flow Control Protocol Error Mask.
        uint32_t     cmpl_timeout_err  :  1; // Completion Timeout Error.
        uint32_t       cmpl_abort_err  :  1; // Completion Abort Error.
        uint32_t        unexp_cpl_err  :  1; // Unexpected Completion Error Mask.
        uint32_t      rcv_overflw_err  :  1; // Receiver Overflow Error Mask.
        uint32_t          mal_tlp_err  :  1; // Malformed TLP Error Mask.
        uint32_t             ecrc_err  :  1; // ERCR Error Mask.
        uint32_t        unsup_req_err  :  1; // Unsupported Request Error Mask.
        uint32_t    acs_violation_err  :  1; // ACS Violation Error.
        uint32_t       uncorr_int_err  :  1; // Uncorrectable Internal Error Mask.
        uint32_t          Reserved_23  :  1; // Unused
        uint32_t   atomic_egr_blk_err  :  1; // Atomic Op Egress Blocked Error.
        uint32_t   tlp_prefix_blk_err  :  1; // TLP Prefix Blocked Error.
        uint32_t       Reserved_31_26  :  6; // Unused
    } field;
    uint32_t val;
} FXR_CFG_AER_UNCOR_ERR_MASK_t;

// FXR_CFG_AER_UNCOR_ERR_SEVRTY desc:
typedef union {
    struct {
        uint32_t         Reserved_3_0  :  4; // Unused
        uint32_t             dllp_err  :  1; // Data Link Layer Packet Error.
        uint32_t        surp_down_err  :  1; // Surprise Down Error.
        uint32_t        Reserved_11_6  :  6; // Unused
        uint32_t       poison_tlp_err  :  1; // Poisoned TLP Error.
        uint32_t               fc_err  :  1; // Flow Control Protocol Error.
        uint32_t     cmpl_timeout_err  :  1; // Completion Timeout Error.
        uint32_t       cmpl_abort_err  :  1; // Completion Abort Error,
        uint32_t        unexp_cpl_err  :  1; // Unexpected Completion Error.
        uint32_t      rcv_overflw_err  :  1; // Receiver Overflow Error.
        uint32_t          mal_tlp_err  :  1; // Malformed TLP Error.
        uint32_t             ecrc_err  :  1; // ERCR Error.
        uint32_t        unsup_req_err  :  1; // Unsupported Request Error
        uint32_t    acs_violation_err  :  1; // ACS Violation Error.
        uint32_t       uncorr_int_err  :  1; // Uncorrectable Internal Error.
        uint32_t          Reserved_23  :  1; // Unused
        uint32_t   atomic_egr_blk_err  :  1; // Atomic Op Egress Blocked Error.
        uint32_t   tlp_prefix_blk_err  :  1; // TLP Prefix Blocked Error.
        uint32_t       Reserved_31_26  :  6; // Unused
    } field;
    uint32_t val;
} FXR_CFG_AER_UNCOR_ERR_SEVRTY_t;

// FXR_CFG_AER_CORR_ERR_STATUS desc:
typedef union {
    struct {
        uint32_t              rcv_err  :  1; // Receiver Error
        uint32_t         Reserved_5_1  :  5; // Unused
        uint32_t          bad_tlp_err  :  1; // Bad TLP
        uint32_t             bad_dllp  :  1; // Bad DLLP
        uint32_t    reply_no_rollover  :  1; // Replay Number Rollover
        uint32_t        Reserved_11_9  :  3; // Unused
        uint32_t    rpl_timer_timeout  :  1; // Replay Timer Timeout
        uint32_t advisory_nonfatal_err  :  1; // Advisory Non-Fatal Error Unexpected Completion Unsupported Request Non-Posted Unsupported Opcode Non-Posted Illegal PCIe BE Illegal Length on CfgRd Illegal Length on MRd Non-Posted Poison/Data Parity Error
        uint32_t         corr_int_err  :  1; // Corrected Internal Error.
        uint32_t  header_log_overflow  :  1; // Header Log Overflow Error Status
        uint32_t       Reserved_31_16  : 16; // Unused
    } field;
    uint32_t val;
} FXR_CFG_AER_CORR_ERR_STATUS_t;

// FXR_CFG_AER_CORR_ERR_MASK desc:
typedef union {
    struct {
        uint32_t              rcv_err  :  1; // Receiver Error
        uint32_t         Reserved_5_1  :  5; // Unused
        uint32_t          bad_tlp_err  :  1; // Bad TLP
        uint32_t             bad_dllp  :  1; // Bad DLLP
        uint32_t    reply_no_rollover  :  1; // Replay Number Rollover
        uint32_t        Reserved_11_9  :  3; // Unused
        uint32_t    rpl_timer_timeout  :  1; // Replay Timer Timeout
        uint32_t advisory_nonfatal_err  :  1; // Advisory Non-Fatal Error
        uint32_t         corr_int_err  :  1; // Corrected Internal Error.
        uint32_t  header_log_overflow  :  1; // Header Log Overflow Error Status
        uint32_t       Reserved_31_16  : 16; // Unused
    } field;
    uint32_t val;
} FXR_CFG_AER_CORR_ERR_MASK_t;

// FXR_CFG_AER_ADV_CAP_CTL desc:
typedef union {
    struct {
        uint32_t       first_err_pntr  :  5; // First Error Pointer. Identifies the bit position of the first error reported in the Uncorrectable Error Status Register
        uint32_t         ecrc_gen_cap  :  1; // ECRC Generation Capable.
        uint32_t          ecrc_gen_en  :  1; // ECRC Generation Enable.
        uint32_t         ecrc_chk_cap  :  1; // ECRC Check Capable.
        uint32_t          ecrc_chk_en  :  1; // ECRC Check Enable.
        uint32_t  multiple_header_cap  :  1; // Multiple Header Recording Capable
        uint32_t   multiple_header_en  :  1; // Multiple Header Recording Enable
        uint32_t  tlp_prefix_log_pres  :  1; // TLP prefix log present.
        uint32_t       Reserved_31_12  : 20; // Unused
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
        uint32_t      tlp_prefix_log0  : 32; // TLP Prefix Log.
    } field;
    uint32_t val;
} FXR_CFG_AER_TLP_PRE_LOG0_t;

// FXR_CFG_AER_TLP_PRE_LOG1 desc:
typedef union {
    struct {
        uint32_t      tlp_prefix_log1  : 32; // TLP Prefix Log.
    } field;
    uint32_t val;
} FXR_CFG_AER_TLP_PRE_LOG1_t;

// FXR_CFG_AER_TLP_PRE_LOG2 desc:
typedef union {
    struct {
        uint32_t      tlp_prefix_log2  : 32; // TLP Prefix Log.
    } field;
    uint32_t val;
} FXR_CFG_AER_TLP_PRE_LOG2_t;

// FXR_CFG_AER_TLP_PRE_LOG3 desc:
typedef union {
    struct {
        uint32_t      tlp_prefix_log3  : 32; // TLP Prefix Log.
    } field;
    uint32_t val;
} FXR_CFG_AER_TLP_PRE_LOG3_t;

// FXR_CFG_ARI_CAPABILITY desc:
typedef union {
    struct {
        uint32_t               cap_id  : 16; // PCIe Capabilities ID. Hardcoded PCIe ID
        uint32_t          cap_version  :  4; // Capability Version. PCI-SIG defined PCIe Capability structure version number.
        uint32_t      next_capability  : 12; // Next Capability Offset. Offset to the next PCI Express Extended Capability structure
    } field;
    uint32_t val;
} FXR_CFG_ARI_CAPABILITY_t;

// FXR_CFG_ARI_CONTROL desc:
typedef union {
    struct {
        uint32_t     mfvc_fun_grp_cap  :  1; // Multifunctional Virtual Channel Function Groups Capability
        uint32_t      acs_fun_grp_cap  :  1; // ACS Function Groups Capability
        uint32_t         Reserved_7_2  :  6; // Unused
        uint32_t         next_fun_num  :  8; // Next Function Number
        uint32_t      mfvc_fun_grp_en  :  1; // MFVC Functional Groups Enable
        uint32_t       acs_fun_grp_en  :  1; // ACS Function Groups Enable
        uint32_t       Reserved_19_18  :  2; // Unused
        uint32_t              fun_grp  :  3; // Functional Group
        uint32_t       Reserved_31_23  :  9; // Unused
    } field;
    uint32_t val;
} FXR_CFG_ARI_CONTROL_t;

// FXR_CFG_SPCIE_CAPABILITY desc:
typedef union {
    struct {
        uint32_t               cap_id  : 16; // PCIe Capabilities ID. Hardcoded PCIe ID
        uint32_t          cap_version  :  4; // Capability Version. PCI-SIG defined PCIe Capability structure version number.
        uint32_t      next_capability  : 12; // Next Capability Offset. Offset to the next PCI Express Extended Capability structure
    } field;
    uint32_t val;
} FXR_CFG_SPCIE_CAPABILITY_t;

// FXR_CFG_LINK_CTL3 desc:
typedef union {
    struct {
        uint32_t           perform_eq  :  1; // Perform Equalization.
        uint32_t        eq_req_int_en  :  1; // Link Equalization Request Interrupt Enable
        uint32_t        Reserved_31_2  : 30; // Unused
    } field;
    uint32_t val;
} FXR_CFG_LINK_CTL3_t;

// FXR_CFG_LANE_ERR_STATUS desc:
typedef union {
    struct {
        uint32_t      lane_err_status  : 16; // Lane Error Status Bits per Lane
        uint32_t       Reserved_31_16  : 16; // Unused
    } field;
    uint32_t val;
} FXR_CFG_LANE_ERR_STATUS_t;

// FXR_CFG_SPCIE_CAP_OFF_0CH desc:
typedef union {
    struct {
        uint32_t        dn_tx_preset0  :  4; // Downstream Port 8.0 GT/s Transmitter Preset0.
        uint32_t   dn_rx_preset_hint0  :  3; // Downstream Port 8.0 GT/s Receiver Preset Hint 0.
        uint32_t           Reserved_7  :  1; // Unused
        uint32_t        up_tx_preset0  :  4; // Upstream Port 8.0 GT/s Transmitter Preset 0.
        uint32_t   up_rx_preset_hint0  :  3; // Upstream Port 8.0 GT/s Receiver Preset Hint 0.
        uint32_t          Reserved_15  :  1; // Unused
        uint32_t   dn_tx_preset_hint1  :  4; // Downstream Port 8.0 GT/s Transmitter Preset Hint 1.
        uint32_t   dn_rx_preset_hint1  :  3; // Downstream Port 8.0 GT/s Receiver Preset Hint 1.
        uint32_t          Reserved_23  :  1; // Unused
        uint32_t   up_tx_preset_hint1  :  4; // Upstream Port 8.0 GT/s Transmitter Preset Hint 1.
        uint32_t   up_rx_preset_hint1  :  3; // Upstream Port 8.0 GT/s Receiver Preset Hint 1.
        uint32_t          Reserved_31  :  1; // Unused
    } field;
    uint32_t val;
} FXR_CFG_SPCIE_CAP_OFF_0CH_t;

// FXR_CFG_SPCIE_CAP_OFF_10H desc:
typedef union {
    struct {
        uint32_t        dn_tx_preset2  :  4; // Downstream Port 8.0 GT/s Transmitter Preset2.
        uint32_t   dn_rx_preset_hint2  :  3; // Downstream Port 8.0 GT/s Receiver Preset Hint 2.
        uint32_t           Reserved_7  :  1; // Unused
        uint32_t        up_tx_preset2  :  4; // Upstream Port 8.0 GT/s Transmitter Preset 2.
        uint32_t   up_rx_preset_hint2  :  3; // Upstream Port 8.0 GT/s Receiver Preset Hint 2.
        uint32_t          Reserved_15  :  1; // Unused
        uint32_t   dn_tx_preset_hint3  :  4; // Downstream Port 8.0 GT/s Transmitter Preset Hint 3.
        uint32_t   dn_rx_preset_hint3  :  3; // Downstream Port 8.0 GT/s Receiver Preset Hint 3.
        uint32_t          Reserved_23  :  1; // Unused
        uint32_t   up_tx_preset_hint3  :  4; // Upstream Port 8.0 GT/s Transmitter Preset Hint 3.
        uint32_t   up_rx_preset_hint3  :  3; // Upstream Port 8.0 GT/s Receiver Preset Hint 3.
        uint32_t          Reserved_31  :  1; // Unused
    } field;
    uint32_t val;
} FXR_CFG_SPCIE_CAP_OFF_10H_t;

// FXR_CFG_SPCIE_CAP_OFF_14H desc:
typedef union {
    struct {
        uint32_t        dn_tx_preset4  :  4; // Downstream Port 8.0 GT/s Transmitter Preset4.
        uint32_t   dn_rx_preset_hint4  :  3; // Downstream Port 8.0 GT/s Receiver Preset Hint 4.
        uint32_t           Reserved_7  :  1; // Unused
        uint32_t        up_tx_preset4  :  4; // Upstream Port 8.0 GT/s Transmitter Preset 4.
        uint32_t   up_rx_preset_hint4  :  3; // Upstream Port 8.0 GT/s Receiver Preset Hint 4.
        uint32_t          Reserved_15  :  1; // Unused
        uint32_t   dn_tx_preset_hint5  :  4; // Downstream Port 8.0 GT/s Transmitter Preset Hint 5.
        uint32_t   dn_rx_preset_hint5  :  3; // Downstream Port 8.0 GT/s Receiver Preset Hint 5.
        uint32_t          Reserved_23  :  1; // Unused
        uint32_t   up_tx_preset_hint5  :  4; // Upstream Port 8.0 GT/s Transmitter Preset Hint 5.
        uint32_t   up_rx_preset_hint5  :  3; // Upstream Port 8.0 GT/s Receiver Preset Hint 5.
        uint32_t          Reserved_31  :  1; // Unused
    } field;
    uint32_t val;
} FXR_CFG_SPCIE_CAP_OFF_14H_t;

// FXR_CFG_SPCIE_CAP_OFF_18H desc:
typedef union {
    struct {
        uint32_t        dn_tx_preset6  :  4; // Downstream Port 8.0 GT/s Transmitter Preset 6.
        uint32_t   dn_rx_preset_hint6  :  3; // Downstream Port 8.0 GT/s Receiver Preset Hint 6.
        uint32_t           Reserved_7  :  1; // Unused
        uint32_t        up_tx_preset6  :  4; // Upstream Port 8.0 GT/s Transmitter Preset 6.
        uint32_t   up_rx_preset_hint6  :  3; // Upstream Port 8.0 GT/s Receiver Preset Hint 6.
        uint32_t          Reserved_15  :  1; // Unused
        uint32_t   dn_tx_preset_hint7  :  4; // Downstream Port 8.0 GT/s Transmitter Preset Hint 7.
        uint32_t   dn_rx_preset_hint7  :  3; // Downstream Port 8.0 GT/s Receiver Preset Hint 7.
        uint32_t          Reserved_23  :  1; // Unused
        uint32_t   up_tx_preset_hint7  :  4; // Upstream Port 8.0 GT/s Transmitter Preset Hint 7.
        uint32_t   up_rx_preset_hint7  :  3; // Upstream Port 8.0 GT/s Receiver Preset Hint 7.
        uint32_t          Reserved_31  :  1; // Unused
    } field;
    uint32_t val;
} FXR_CFG_SPCIE_CAP_OFF_18H_t;

// FXR_CFG_SPCIE_CAP_OFF_1CH desc:
typedef union {
    struct {
        uint32_t        dn_tx_preset8  :  4; // Downstream Port 8.0 GT/s Transmitter Preset 8.
        uint32_t   dn_rx_preset_hint8  :  3; // Downstream Port 8.0 GT/s Receiver Preset Hint 8.
        uint32_t           Reserved_7  :  1; // Unused
        uint32_t        up_tx_preset8  :  4; // Upstream Port 8.0 GT/s Transmitter Preset 8.
        uint32_t   up_rx_preset_hint8  :  3; // Upstream Port 8.0 GT/s Receiver Preset Hint 8.
        uint32_t          Reserved_15  :  1; // Unused
        uint32_t   dn_tx_preset_hint9  :  4; // Downstream Port 8.0 GT/s Transmitter Preset Hint 9.
        uint32_t   dn_rx_preset_hint9  :  3; // Downstream Port 8.0 GT/s Receiver Preset Hint 9.
        uint32_t          Reserved_23  :  1; // Unused
        uint32_t   up_tx_preset_hint9  :  4; // Upstream Port 8.0 GT/s Transmitter Preset Hint 9.
        uint32_t   up_rx_preset_hint9  :  3; // Upstream Port 8.0 GT/s Receiver Preset Hint 9.
        uint32_t          Reserved_31  :  1; // Unused
    } field;
    uint32_t val;
} FXR_CFG_SPCIE_CAP_OFF_1CH_t;

// FXR_CFG_SPCIE_CAP_OFF_20H desc:
typedef union {
    struct {
        uint32_t       dn_tx_preset10  :  4; // Downstream Port 8.0 GT/s Transmitter Preset 10.
        uint32_t  dn_rx_preset_hint10  :  3; // Downstream Port 8.0 GT/s Receiver Preset Hint 10.
        uint32_t           Reserved_7  :  1; // Unused
        uint32_t       up_tx_preset10  :  4; // Upstream Port 8.0 GT/s Transmitter Preset 10.
        uint32_t  up_rx_preset_hint10  :  3; // Upstream Port 8.0 GT/s Receiver Preset Hint 10.
        uint32_t          Reserved_15  :  1; // Unused
        uint32_t  dn_tx_preset_hint11  :  4; // Downstream Port 8.0 GT/s Transmitter Preset Hint 11.
        uint32_t  dn_rx_preset_hint11  :  3; // Downstream Port 8.0 GT/s Receiver Preset Hint 11.
        uint32_t          Reserved_23  :  1; // Unused
        uint32_t  up_tx_preset_hint11  :  4; // Upstream Port 8.0 GT/s Transmitter Preset Hint 11.
        uint32_t  up_rx_preset_hint11  :  3; // Upstream Port 8.0 GT/s Receiver Preset Hint 11.
        uint32_t          Reserved_31  :  1; // Unused
    } field;
    uint32_t val;
} FXR_CFG_SPCIE_CAP_OFF_20H_t;

// FXR_CFG_SPCIE_CAP_OFF_24H desc:
typedef union {
    struct {
        uint32_t       dn_tx_preset12  :  4; // Downstream Port 8.0 GT/s Transmitter Preset 12.
        uint32_t  dn_rx_preset_hint12  :  3; // Downstream Port 8.0 GT/s Receiver Preset Hint 12.
        uint32_t           Reserved_7  :  1; // Unused
        uint32_t       up_tx_preset12  :  4; // Upstream Port 8.0 GT/s Transmitter Preset 12.
        uint32_t  up_rx_preset_hint12  :  3; // Upstream Port 8.0 GT/s Receiver Preset Hint 12.
        uint32_t          Reserved_15  :  1; // Unused
        uint32_t  dn_tx_preset_hint13  :  4; // Downstream Port 8.0 GT/s Transmitter Preset Hint 13.
        uint32_t  dn_rx_preset_hint13  :  3; // Downstream Port 8.0 GT/s Receiver Preset Hint 13.
        uint32_t          Reserved_23  :  1; // Unused
        uint32_t  up_tx_preset_hint13  :  4; // Upstream Port 8.0 GT/s Transmitter Preset Hint 13.
        uint32_t  up_rx_preset_hint13  :  3; // Upstream Port 8.0 GT/s Receiver Preset Hint 13.
        uint32_t          Reserved_31  :  1; // Unused
    } field;
    uint32_t val;
} FXR_CFG_SPCIE_CAP_OFF_24H_t;

// FXR_CFG_SPCIE_CAP_OFF_28H desc:
typedef union {
    struct {
        uint32_t       dn_tx_preset14  :  4; // Downstream Port 8.0 GT/s Transmitter Preset 14.
        uint32_t  dn_rx_preset_hint14  :  3; // Downstream Port 8.0 GT/s Receiver Preset Hint 14.
        uint32_t           Reserved_7  :  1; // Unused
        uint32_t       up_tx_preset14  :  4; // Upstream Port 8.0 GT/s Transmitter Preset 14.
        uint32_t  up_rx_preset_hint14  :  3; // Upstream Port 8.0 GT/s Receiver Preset Hint 14.
        uint32_t          Reserved_15  :  1; // Unused
        uint32_t  dn_tx_preset_hint15  :  4; // Downstream Port 8.0 GT/s Transmitter Preset Hint 15.
        uint32_t  dn_rx_preset_hint15  :  3; // Downstream Port 8.0 GT/s Receiver Preset Hint 15.
        uint32_t          Reserved_23  :  1; // Unused
        uint32_t  up_tx_preset_hint15  :  4; // Upstream Port 8.0 GT/s Transmitter Preset Hint 15.
        uint32_t  up_rx_preset_hint15  :  3; // Upstream Port 8.0 GT/s Receiver Preset Hint 15.
        uint32_t          Reserved_31  :  1; // Unused
    } field;
    uint32_t val;
} FXR_CFG_SPCIE_CAP_OFF_28H_t;

// FXR_CFG_PL16G_EXT_CAPABILITY desc:
typedef union {
    struct {
        uint32_t               cap_id  : 16; // PCIe Capabilities ID. Hardcoded PCIe ID
        uint32_t          cap_version  :  4; // Capability Version. PCI-SIG defined PCIe Capability structure version number.
        uint32_t      next_capability  : 12; // Next Capability Offset. Offset to the next PCI Express Extended Capability structure
    } field;
    uint32_t val;
} FXR_CFG_PL16G_EXT_CAPABILITY_t;

// FXR_CFG_PL16G_CAPABILITY desc:
typedef union {
    struct {
        uint32_t        Reserved_31_0  : 32; // Unused
    } field;
    uint32_t val;
} FXR_CFG_PL16G_CAPABILITY_t;

// FXR_CFG_PL16G_CONTROL desc:
typedef union {
    struct {
        uint32_t        Reserved_31_0  : 32; // Unused
    } field;
    uint32_t val;
} FXR_CFG_PL16G_CONTROL_t;

// FXR_CFG_PL16G_STATUS desc:
typedef union {
    struct {
        uint32_t           eq_16g_cpl  :  1; // Equalization 16.0GT/s Successful.
        uint32_t        eq_16g_cpl_p1  :  1; // Equalization 16.0GT/s Phase 1 Successful.
        uint32_t        eq_16g_cpl_p2  :  1; // Equalization 16.0GT/s Phase 2 Successful.
        uint32_t        eq_16g_cpl_p3  :  1; // Equalization 16.0GT/s Phase 3 Successful.
        uint32_t      link_eq_16g_req  :  1; // Link Equalization Request 16.0GT/s
        uint32_t        Reserved_31_5  : 27; // Unused
    } field;
    uint32_t val;
} FXR_CFG_PL16G_STATUS_t;

// FXR_CFG_PL16G_DPAR_STATUS desc:
typedef union {
    struct {
        uint32_t       lc_dpar_status  : 16; // Local Data Parity Mismatch Status.
        uint32_t       Reserved_31_16  : 16; // Unused
    } field;
    uint32_t val;
} FXR_CFG_PL16G_DPAR_STATUS_t;

// FXR_CFG_PL16G_FIRST_RT_DPAR desc:
typedef union {
    struct {
        uint32_t       first_ret_dpar  : 16; // First Retimer Data Parity Mismatch Status
        uint32_t       Reserved_31_16  : 16; // Unused
    } field;
    uint32_t val;
} FXR_CFG_PL16G_FIRST_RT_DPAR_t;

// FXR_CFG_PL16G_SECOND_RT_DPAR desc:
typedef union {
    struct {
        uint32_t      second_ret_dpar  : 16; // Second Retimer Data Parity Mismatch Status
        uint32_t       Reserved_31_16  : 16; // Unused
    } field;
    uint32_t val;
} FXR_CFG_PL16G_SECOND_RT_DPAR_t;

// FXR_CFG_PL16G_CAP_OFF_20H desc:
typedef union {
    struct {
        uint32_t    dn_16g_tx_preset0  :  4; // Downstream Port 16.0 GT/s Transmitter Preset0
        uint32_t    up_16g_tx_preset0  :  4; // Upstream Port 16.0 GT/s Transmitter Preset0
        uint32_t    dn_16g_tx_preset1  :  4; // Downstream Port 16.0 GT/s Transmitter Preset1
        uint32_t    up_16g_tx_preset1  :  4; // Upstream Port 16.0 GT/s Transmitter Preset1
        uint32_t    dn_16g_tx_preset2  :  4; // Downstream Port 16.0 GT/s Transmitter Preset2
        uint32_t    up_16g_tx_preset2  :  4; // Upstream Port 16.0 GT/s Transmitter Preset2
        uint32_t    dn_16g_tx_preset3  :  4; // Downstream Port 16.0 GT/s Transmitter Preset3
        uint32_t    up_16g_tx_preset3  :  4; // Upstream Port 16.0 GT/s Transmitter Preset3
    } field;
    uint32_t val;
} FXR_CFG_PL16G_CAP_OFF_20H_t;

// FXR_CFG_PL16G_CAP_OFF_24H desc:
typedef union {
    struct {
        uint32_t    dn_16g_tx_preset4  :  4; // Downstream Port 16.0 GT/s Transmitter Preset4
        uint32_t    up_16g_tx_preset4  :  4; // Upstream Port 16.0 GT/s Transmitter Preset4
        uint32_t    dn_16g_tx_preset5  :  4; // Downstream Port 16.0 GT/s Transmitter Preset5
        uint32_t    up_16g_tx_preset5  :  4; // Upstream Port 16.0 GT/s Transmitter Preset5
        uint32_t    dn_16g_tx_preset6  :  4; // Downstream Port 16.0 GT/s Transmitter Preset6
        uint32_t    up_16g_tx_preset6  :  4; // Upstream Port 16.0 GT/s Transmitter Preset6
        uint32_t    dn_16g_tx_preset7  :  4; // Downstream Port 16.0 GT/s Transmitter Preset7
        uint32_t    up_16g_tx_preset7  :  4; // Upstream Port 16.0 GT/s Transmitter Preset7
    } field;
    uint32_t val;
} FXR_CFG_PL16G_CAP_OFF_24H_t;

// FXR_CFG_PL16G_CAP_OFF_28H desc:
typedef union {
    struct {
        uint32_t    dn_16g_tx_preset8  :  4; // Downstream Port 16.0 GT/s Transmitter Preset8
        uint32_t    up_16g_tx_preset8  :  4; // Upstream Port 16.0 GT/s Transmitter Preset8
        uint32_t    dn_16g_tx_preset9  :  4; // Downstream Port 16.0 GT/s Transmitter Preset9
        uint32_t    up_16g_tx_preset9  :  4; // Upstream Port 16.0 GT/s Transmitter Preset9
        uint32_t   dn_16g_tx_preset10  :  4; // Downstream Port 16.0 GT/s Transmitter Preset10
        uint32_t   up_16g_tx_preset10  :  4; // Upstream Port 16.0 GT/s Transmitter Preset10
        uint32_t   dn_16g_tx_preset11  :  4; // Downstream Port 16.0 GT/s Transmitter Preset11
        uint32_t   up_16g_tx_preset11  :  4; // Upstream Port 16.0 GT/s Transmitter Preset11
    } field;
    uint32_t val;
} FXR_CFG_PL16G_CAP_OFF_28H_t;

// FXR_CFG_PL16G_CAP_OFF_2CH desc:
typedef union {
    struct {
        uint32_t   dn_16g_tx_preset12  :  4; // Downstream Port 16.0 GT/s Transmitter Preset12
        uint32_t   up_16g_tx_preset12  :  4; // Upstream Port 16.0 GT/s Transmitter Preset12
        uint32_t   dn_16g_tx_preset13  :  4; // Downstream Port 16.0 GT/s Transmitter Preset13
        uint32_t   up_16g_tx_preset13  :  4; // Upstream Port 16.0 GT/s Transmitter Preset13
        uint32_t   dn_16g_tx_preset14  :  4; // Downstream Port 16.0 GT/s Transmitter Preset14
        uint32_t   up_16g_tx_preset14  :  4; // Upstream Port 16.0 GT/s Transmitter Preset14
        uint32_t   dn_16g_tx_preset15  :  4; // Downstream Port 16.0 GT/s Transmitter Preset15
        uint32_t   up_16g_tx_preset15  :  4; // Upstream Port 16.0 GT/s Transmitter Preset15
    } field;
    uint32_t val;
} FXR_CFG_PL16G_CAP_OFF_2CH_t;

// FXR_CFG_MARGIN_EXT_CAPABILITY desc:
typedef union {
    struct {
        uint32_t               cap_id  : 16; // PCIe Capabilities ID. Hardcoded PCIe ID
        uint32_t          cap_version  :  4; // Capability Version. PCI-SIG defined PCIe Capability structure version number.
        uint32_t      next_capability  : 12; // Next Capability Offset. Offset to the next PCI Express Extended Capability structure
    } field;
    uint32_t val;
} FXR_CFG_MARGIN_EXT_CAPABILITY_t;

// FXR_CFG_PORT_CAP_STATUS desc:
typedef union {
    struct {
        uint32_t  margin_uses_drvr_sw  :  1; // Margining uses Driver Software
        uint32_t        Reserved_15_1  : 15; // Unused
        uint32_t         margin_ready  :  1; // Margining Ready
        uint32_t      margin_sw_ready  :  1; // Margining Software Ready
        uint32_t       Reserved_31_18  : 14; // Unused
    } field;
    uint32_t val;
} FXR_CFG_PORT_CAP_STATUS_t;

// FXR_CFG_LANE_CTL_STATUS0 desc:
typedef union {
    struct {
        uint32_t      receiver_number  :  3; // Receiver Number for Lane 0.
        uint32_t          margin_type  :  3; // Margin Type for Lane 0.
        uint32_t          usage_model  :  1; // Usage Model for Lane 0.
        uint32_t           Reserved_7  :  1; // Unused
        uint32_t       margin_payload  :  8; // Margin Payload for Lane 0.
        uint32_t    reciever_num_stat  :  3; // Receiver Number(Status) for Lane 0.
        uint32_t     margin_type_stat  :  3; // Margin Type(Status) for Lane 0
        uint32_t     usage_model_stat  :  1; // Usage Model(Status) for Lane 0
        uint32_t          Reserved_23  :  1; // Unused
        uint32_t  margin_payload_stat  :  8; // Margin Payload(Status) for Lane 0
    } field;
    uint32_t val;
} FXR_CFG_LANE_CTL_STATUS0_t;

// FXR_CFG_LANE_CTL_STATUS1 desc:
typedef union {
    struct {
        uint32_t      receiver_number  :  3; // Receiver Number for Lane 1.
        uint32_t          margin_type  :  3; // Margin Type for Lane 1.
        uint32_t          usage_model  :  1; // Usage Model for Lane 1.
        uint32_t           Reserved_7  :  1; // Unused
        uint32_t       margin_payload  :  8; // Margin Payload for Lane 1.
        uint32_t    reciever_num_stat  :  3; // Receiver Number(Status) for Lane 1.
        uint32_t     margin_type_stat  :  3; // Margin Type(Status) for Lane 1
        uint32_t     usage_model_stat  :  1; // Usage Model(Status) for Lane 1
        uint32_t          Reserved_23  :  1; // Unused
        uint32_t  margin_payload_stat  :  8; // Margin Payload(Status) for Lane 1
    } field;
    uint32_t val;
} FXR_CFG_LANE_CTL_STATUS1_t;

// FXR_CFG_LANE_CTL_STATUS2 desc:
typedef union {
    struct {
        uint32_t      receiver_number  :  3; // Receiver Number for Lane 2.
        uint32_t          margin_type  :  3; // Margin Type for Lane 2.
        uint32_t          usage_model  :  1; // Usage Model for Lane 2.
        uint32_t           Reserved_7  :  1; // Unused
        uint32_t       margin_payload  :  8; // Margin Payload for Lane 2.
        uint32_t    reciever_num_stat  :  3; // Receiver Number(Status) for Lane 2.
        uint32_t     margin_type_stat  :  3; // Margin Type(Status) for Lane 2
        uint32_t     usage_model_stat  :  1; // Usage Model(Status) for Lane 2
        uint32_t          Reserved_23  :  1; // Unused
        uint32_t  margin_payload_stat  :  8; // Margin Payload(Status) for Lane 2
    } field;
    uint32_t val;
} FXR_CFG_LANE_CTL_STATUS2_t;

// FXR_CFG_LANE_CTL_STATUS3 desc:
typedef union {
    struct {
        uint32_t      receiver_number  :  3; // Receiver Number for Lane 3.
        uint32_t          margin_type  :  3; // Margin Type for Lane 3.
        uint32_t          usage_model  :  1; // Usage Model for Lane 3.
        uint32_t           Reserved_7  :  1; // Unused
        uint32_t       margin_payload  :  8; // Margin Payload for Lane 3.
        uint32_t    reciever_num_stat  :  3; // Receiver Number(Status) for Lane 3.
        uint32_t     margin_type_stat  :  3; // Margin Type(Status) for Lane 3
        uint32_t     usage_model_stat  :  1; // Usage Model(Status) for Lane 3
        uint32_t          Reserved_23  :  1; // Unused
        uint32_t  margin_payload_stat  :  8; // Margin Payload(Status) for Lane 3
    } field;
    uint32_t val;
} FXR_CFG_LANE_CTL_STATUS3_t;

// FXR_CFG_LANE_CTL_STATUS4 desc:
typedef union {
    struct {
        uint32_t      receiver_number  :  3; // Receiver Number for Lane 4.
        uint32_t          margin_type  :  3; // Margin Type for Lane 4.
        uint32_t          usage_model  :  1; // Usage Model for Lane 4.
        uint32_t           Reserved_7  :  1; // Unused
        uint32_t       margin_payload  :  8; // Margin Payload for Lane 4.
        uint32_t    reciever_num_stat  :  3; // Receiver Number(Status) for Lane 4.
        uint32_t     margin_type_stat  :  3; // Margin Type(Status) for Lane 4
        uint32_t     usage_model_stat  :  1; // Usage Model(Status) for Lane 4
        uint32_t          Reserved_23  :  1; // Unused
        uint32_t  margin_payload_stat  :  8; // Margin Payload(Status) for Lane 4
    } field;
    uint32_t val;
} FXR_CFG_LANE_CTL_STATUS4_t;

// FXR_CFG_LANE_CTL_STATUS5 desc:
typedef union {
    struct {
        uint32_t      receiver_number  :  3; // Receiver Number for Lane 5.
        uint32_t          margin_type  :  3; // Margin Type for Lane 5.
        uint32_t          usage_model  :  1; // Usage Model for Lane 5.
        uint32_t           Reserved_7  :  1; // Unused
        uint32_t       margin_payload  :  8; // Margin Payload for Lane 5.
        uint32_t    reciever_num_stat  :  3; // Receiver Number(Status) for Lane 5
        uint32_t     margin_type_stat  :  3; // Margin Type(Status) for Lane 5
        uint32_t     usage_model_stat  :  1; // Usage Model(Status) for Lane 5
        uint32_t          Reserved_23  :  1; // Unused
        uint32_t  margin_payload_stat  :  8; // Margin Payload(Status) for Lane 5
    } field;
    uint32_t val;
} FXR_CFG_LANE_CTL_STATUS5_t;

// FXR_CFG_LANE_CTL_STATUS6 desc:
typedef union {
    struct {
        uint32_t      receiver_number  :  3; // Receiver Number for Lane 6.
        uint32_t          margin_type  :  3; // Margin Type for Lane 6.
        uint32_t          usage_model  :  1; // Usage Model for Lane 6.
        uint32_t           Reserved_7  :  1; // Unused
        uint32_t       margin_payload  :  8; // Margin Payload for Lane 6.
        uint32_t    reciever_num_stat  :  3; // Receiver Number(Status) for Lane 6
        uint32_t     margin_type_stat  :  3; // Margin Type(Status) for Lane 6
        uint32_t     usage_model_stat  :  1; // Usage Model(Status) for Lane 6
        uint32_t          Reserved_23  :  1; // Unused
        uint32_t  margin_payload_stat  :  8; // Margin Payload(Status) for Lane 6
    } field;
    uint32_t val;
} FXR_CFG_LANE_CTL_STATUS6_t;

// FXR_CFG_LANE_CTL_STATUS7 desc:
typedef union {
    struct {
        uint32_t      receiver_number  :  3; // Receiver Number for Lane 7.
        uint32_t          margin_type  :  3; // Margin Type for Lane 7.
        uint32_t          usage_model  :  1; // Usage Model for Lane 7.
        uint32_t           Reserved_7  :  1; // Unused
        uint32_t       margin_payload  :  8; // Margin Payload for Lane 7.
        uint32_t    reciever_num_stat  :  3; // Receiver Number(Status) for Lane 7
        uint32_t     margin_type_stat  :  3; // Margin Type(Status) for Lane 7
        uint32_t     usage_model_stat  :  1; // Usage Model(Status) for Lane 7
        uint32_t          Reserved_23  :  1; // Unused
        uint32_t  margin_payload_stat  :  8; // Margin Payload(Status) for Lane 7
    } field;
    uint32_t val;
} FXR_CFG_LANE_CTL_STATUS7_t;

// FXR_CFG_LANE_CTL_STATUS8 desc:
typedef union {
    struct {
        uint32_t      receiver_number  :  3; // Receiver Number for Lane 8.
        uint32_t          margin_type  :  3; // Margin Type for Lane 8.
        uint32_t          usage_model  :  1; // Usage Model for Lane 8.
        uint32_t           Reserved_7  :  1; // Unused
        uint32_t       margin_payload  :  8; // Margin Payload for Lane 8.
        uint32_t    reciever_num_stat  :  3; // Receiver Number(Status) for Lane 8
        uint32_t     margin_type_stat  :  3; // Margin Type(Status) for Lane 8
        uint32_t     usage_model_stat  :  1; // Usage Model(Status) for Lane 8
        uint32_t          Reserved_23  :  1; // Unused
        uint32_t  margin_payload_stat  :  8; // Margin Payload(Status) for Lane 8
    } field;
    uint32_t val;
} FXR_CFG_LANE_CTL_STATUS8_t;

// FXR_CFG_LANE_CTL_STATUS9 desc:
typedef union {
    struct {
        uint32_t      receiver_number  :  3; // Receiver Number for Lane 9.
        uint32_t          margin_type  :  3; // Margin Type for Lane 9.
        uint32_t          usage_model  :  1; // Usage Model for Lane 9.
        uint32_t           Reserved_7  :  1; // Unused
        uint32_t       margin_payload  :  8; // Margin Payload for Lane 9.
        uint32_t    reciever_num_stat  :  3; // Receiver Number(Status) for Lane 9
        uint32_t     margin_type_stat  :  3; // Margin Type(Status) for Lane 9
        uint32_t     usage_model_stat  :  1; // Usage Model(Status) for Lane 9
        uint32_t          Reserved_23  :  1; // Unused
        uint32_t  margin_payload_stat  :  8; // Margin Payload(Status) for Lane 9
    } field;
    uint32_t val;
} FXR_CFG_LANE_CTL_STATUS9_t;

// FXR_CFG_LANE_CTL_STATUS10 desc:
typedef union {
    struct {
        uint32_t      receiver_number  :  3; // Receiver Number for Lane 10.
        uint32_t          margin_type  :  3; // Margin Type for Lane 10.
        uint32_t          usage_model  :  1; // Usage Model for Lane 10.
        uint32_t           Reserved_7  :  1; // Unused
        uint32_t       margin_payload  :  8; // Margin Payload for Lane 10.
        uint32_t    reciever_num_stat  :  3; // Receiver Number(Status) for Lane 10
        uint32_t     margin_type_stat  :  3; // Margin Type(Status) for Lane 10
        uint32_t     usage_model_stat  :  1; // Usage Model(Status) for Lane 10
        uint32_t          Reserved_23  :  1; // Unused
        uint32_t  margin_payload_stat  :  8; // Margin Payload(Status) for Lane 10
    } field;
    uint32_t val;
} FXR_CFG_LANE_CTL_STATUS10_t;

// FXR_CFG_LANE_CTL_STATUS11 desc:
typedef union {
    struct {
        uint32_t      receiver_number  :  3; // Receiver Number for Lane 11.
        uint32_t          margin_type  :  3; // Margin Type for Lane 11.
        uint32_t          usage_model  :  1; // Usage Model for Lane 11.
        uint32_t           Reserved_7  :  1; // Unused
        uint32_t       margin_payload  :  8; // Margin Payload for Lane 11.
        uint32_t    reciever_num_stat  :  3; // Receiver Number(Status) for Lane 11
        uint32_t     margin_type_stat  :  3; // Margin Type(Status) for Lane 11
        uint32_t     usage_model_stat  :  1; // Usage Model(Status) for Lane 11
        uint32_t          Reserved_23  :  1; // Unused
        uint32_t  margin_payload_stat  :  8; // Margin Payload(Status) for Lane 11
    } field;
    uint32_t val;
} FXR_CFG_LANE_CTL_STATUS11_t;

// FXR_CFG_LANE_CTL_STATUS12 desc:
typedef union {
    struct {
        uint32_t      receiver_number  :  3; // Receiver Number for Lane 12.
        uint32_t          margin_type  :  3; // Margin Type for Lane 12.
        uint32_t          usage_model  :  1; // Usage Model for Lane 12.
        uint32_t           Reserved_7  :  1; // Unused
        uint32_t       margin_payload  :  8; // Margin Payload for Lane 12.
        uint32_t    reciever_num_stat  :  3; // Receiver Number(Status) for Lane 12
        uint32_t     margin_type_stat  :  3; // Margin Type(Status) for Lane 12
        uint32_t     usage_model_stat  :  1; // Usage Model(Status) for Lane 12
        uint32_t          Reserved_23  :  1; // Unused
        uint32_t  margin_payload_stat  :  8; // Margin Payload(Status) for Lane 12
    } field;
    uint32_t val;
} FXR_CFG_LANE_CTL_STATUS12_t;

// FXR_CFG_LANE_CTL_STATUS13 desc:
typedef union {
    struct {
        uint32_t      receiver_number  :  3; // Receiver Number for Lane 13.
        uint32_t          margin_type  :  3; // Margin Type for Lane 13.
        uint32_t          usage_model  :  1; // Usage Model for Lane 13.
        uint32_t           Reserved_7  :  1; // Unused
        uint32_t       margin_payload  :  8; // Margin Payload for Lane 13.
        uint32_t    reciever_num_stat  :  3; // Receiver Number(Status) for Lane 13
        uint32_t     margin_type_stat  :  3; // Margin Type(Status) for Lane 13
        uint32_t     usage_model_stat  :  1; // Usage Model(Status) for Lane 13
        uint32_t          Reserved_23  :  1; // Unused
        uint32_t  margin_payload_stat  :  8; // Margin Payload(Status) for Lane 13
    } field;
    uint32_t val;
} FXR_CFG_LANE_CTL_STATUS13_t;

// FXR_CFG_LANE_CTL_STATUS14 desc:
typedef union {
    struct {
        uint32_t      receiver_number  :  3; // Receiver Number for Lane 14.
        uint32_t          margin_type  :  3; // Margin Type for Lane 14.
        uint32_t          usage_model  :  1; // Usage Model for Lane 14.
        uint32_t           Reserved_7  :  1; // Unused
        uint32_t       margin_payload  :  8; // Margin Payload for Lane 14.
        uint32_t    reciever_num_stat  :  3; // Receiver Number(Status) for Lane 14
        uint32_t     margin_type_stat  :  3; // Margin Type(Status) for Lane 14
        uint32_t     usage_model_stat  :  1; // Usage Model(Status) for Lane 14
        uint32_t          Reserved_23  :  1; // Unused
        uint32_t  margin_payload_stat  :  8; // Margin Payload(Status) for Lane 14
    } field;
    uint32_t val;
} FXR_CFG_LANE_CTL_STATUS14_t;

// FXR_CFG_LANE_CTL_STATUS15 desc:
typedef union {
    struct {
        uint32_t      receiver_number  :  3; // Receiver Number for Lane 15.
        uint32_t          margin_type  :  3; // Margin Type for Lane 15.
        uint32_t          usage_model  :  1; // Usage Model for Lane 15.
        uint32_t           Reserved_7  :  1; // Unused
        uint32_t       margin_payload  :  8; // Margin Payload for Lane 15.
        uint32_t    reciever_num_stat  :  3; // Receiver Number(Status) for Lane 15
        uint32_t     margin_type_stat  :  3; // Margin Type(Status) for Lane 15
        uint32_t     usage_model_stat  :  1; // Usage Model(Status) for Lane 15
        uint32_t          Reserved_23  :  1; // Unused
        uint32_t  margin_payload_stat  :  8; // Margin Payload(Status) for Lane 15
    } field;
    uint32_t val;
} FXR_CFG_LANE_CTL_STATUS15_t;

// FXR_CFG_TPH_EXT_CAPABILITY desc:
typedef union {
    struct {
        uint32_t               cap_id  : 16; // PCIe Capabilities ID. Hardcoded PCIe ID
        uint32_t          cap_version  :  4; // Capability Version. PCI-SIG defined PCIe Capability structure version number.
        uint32_t      next_capability  : 12; // Next Capability Offset. Offset to the next PCI Express Extended Capability structure
    } field;
    uint32_t val;
} FXR_CFG_TPH_EXT_CAPABILITY_t;

// FXR_CFG_TPH_REQ_CAPABILITY desc:
typedef union {
    struct {
        uint32_t           no_st_mode  :  1; // No ST Mode Supported
        uint32_t              int_vec  :  1; // Interrupt Vector Mode Supported
        uint32_t          device_spec  :  1; // Device Specific Mode Supported
        uint32_t         Reserved_7_3  :  5; // Unused
        uint32_t         extended_tph  :  1; // Extended TPH Requester Supported
        uint32_t         st_table_loc  :  2; // ST Table Location
        uint32_t       Reserved_15_11  :  5; // Unused
        uint32_t        st_table_size  : 11; // ST Table Size
        uint32_t       Reserved_31_27  :  5; // Unused
    } field;
    uint32_t val;
} FXR_CFG_TPH_REQ_CAPABILITY_t;

// FXR_CFG_TPH_REQ_CONTROL desc:
typedef union {
    struct {
        uint32_t       st_mode_select  :  3; // ST Mode Select
        uint32_t         Reserved_7_3  :  5; // Unused
        uint32_t          ctrl_req_en  :  2; // TPH Requester Enable
        uint32_t       Reserved_31_10  : 22; // Unused
    } field;
    uint32_t val;
} FXR_CFG_TPH_REQ_CONTROL_t;

// FXR_CFG_TPH_ST_TABLE desc:
typedef union {
    struct {
        uint32_t       st_table_lower  :  8; // ST Table Lower Byte
        uint32_t      st_table_higher  :  8; // ST Table Upper Byte
        uint32_t       Reserved_31_16  : 16; // Unused
    } field;
    uint32_t val;
} FXR_CFG_TPH_ST_TABLE_t;

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
        uint32_t       Reserved_15_13  :  3; // Unused
        uint32_t    nosnoop_lat_value  : 10; // Max No-Snoop Latency Value. Specifies the maximum no-snoop latency that a device is permitted to request.
        uint32_t    nosnoop_lat_scale  :  3; // Max No-Snoop Latency Scale. Specifies a scale for the value contained within the Maximum No-Snoop Latency Value field.
        uint32_t       Reserved_31_29  :  3; // Unused
    } field;
    uint32_t val;
} FXR_CFG_SNOOP_LATENCY_t;

// FXR_RASDP_EXT_CAPABILITY desc:
typedef union {
    struct {
        uint32_t               cap_id  : 16; // PCIe Capabilities ID. Hardcoded PCIe ID
        uint32_t          cap_version  :  4; // Capability Version. PCI-SIG defined PCIe Capability structure version number.
        uint32_t      next_capability  : 12; // Next Capability Offset. Offset to the next PCI Express Extended Capability structure
    } field;
    uint32_t val;
} FXR_RASDP_EXT_CAPABILITY_t;

// FXR_RASDP_VENDOR_SPECIFIC desc:
typedef union {
    struct {
        uint32_t              vsec_id  : 16; // VSEC ID
        uint32_t             vsec_rev  :  4; // VSEC Rev
        uint32_t          vsec_length  : 12; // VSEC Length
    } field;
    uint32_t val;
} FXR_RASDP_VENDOR_SPECIFIC_t;

// FXR_RASDP_ERR_PROT_CTL desc:
typedef union {
    struct {
        uint32_t           disable_tx  :  1; // Global ECC Disable Tx path
        uint32_t disable_axi_brg_master  :  1; // ECC Disable for AXI master completion buffer
        uint32_t disable_axi_brg_outbound  :  1; // ECC Disable for AXI bridge outbound request path
        uint32_t    disable_dma_write  :  1; // ECC Disable for DMA write engine
        uint32_t    disable_layer2_tx  :  1; // ECC Disable for Layer 2 Tx path
        uint32_t    disable_layer3_tx  :  1; // ECC Disable for Layer 3 Tx path
        uint32_t       disable_adm_tx  :  1; // ECC Disable for ADM Tx path
        uint32_t        Reserved_15_7  :  9; // Unused
        uint32_t           disable_rx  :  1; // Global ECC Disable Rx path
        uint32_t  disable_axi_brg_ibc  :  1; // ECC Disable for AXI bridge inbound completion path
        uint32_t  disable_axi_brg_ibr  :  1; // ECC Disable for AXI bridge inbound request path
        uint32_t     disable_dma_read  :  1; // ECC Disable for DMA read engine
        uint32_t    disable_layer2_rx  :  1; // ECC Disable for Layer 2 Rx path
        uint32_t    disable_layer3_rx  :  1; // ECC Disable for Layer 3 Rx path
        uint32_t       disable_adm_rx  :  1; // ECC Disable for ADM Rx path
        uint32_t       Reserved_31_23  :  9; // Unused
    } field;
    uint32_t val;
} FXR_RASDP_ERR_PROT_CTL_t;

// FXR_RASDP_CORR_CNTR_CTL desc:
typedef union {
    struct {
        uint32_t     corr_clear_cntrs  :  1; // Clear all correctable error counters
        uint32_t         Reserved_3_1  :  3; // Unused
        uint32_t        corr_en_cntrs  :  1; // Enable correctable errors counters
        uint32_t        Reserved_19_5  : 15; // Unused
        uint32_t corr_cntr_select_region  :  4; // Select Correctable counter region0x0: Region select for Adm Rx path 0x1: Region select for layer 3 Rx path 0x2: Region select for layer 2 Rx path 0x3: Region select for DMA inbound path 0x4: Region select for AXI bridge inbound request path 0x5: Region select for AXI bridge inbound completion path 0x6: Region select for Adm Tx path 0x7: Region select for layer 3 Tx path 0x8: Region select for layer 2 Tx path 0x9: Region select for DMA outbound path 0xa: Region select for AXI bridge outbound request path 0xb: Region select for AXI bridge outbound master completion buffer path 0xc: Reserved 0xf: Reserved
        uint32_t     corr_cntr_select  :  8; // Counter Selection
    } field;
    uint32_t val;
} FXR_RASDP_CORR_CNTR_CTL_t;

// FXR_RASDP_CORR_CNT_REPORT desc:
typedef union {
    struct {
        uint32_t            corr_cntr  :  8; // Current corrected error count for the selected counter
        uint32_t        Reserved_19_8  : 12; // Unused
        uint32_t corr_cntr_select_region  :  4; // Selected Correctable counter region0x0: Region select for Adm Rx path 0x1: Region select for layer 3 Rx path 0x2: Region select for layer 2 Rx path 0x3: Region select for DMA inbound path 0x4: Region select for AXI bridge inbound request path 0x5: Region select for AXI bridge inbound completion path 0x6: Region select for Adm Tx path 0x7: Region select for layer 3 Tx path 0x8: Region select for layer 2 Tx path 0x9: Region select for DMA outbound path 0xa: Region select for AXI bridge outbound request path 0xb: Region select for AXI bridge outbound master completion buffer path 0xc: Reserved 0xf: Reserved
        uint32_t     corr_cntr_select  :  8; // Counter Selection.Returns the value set in the CORR_CNTR_SELECT field of the FXR_RASDP_CORR_CNTR_CTL register.
    } field;
    uint32_t val;
} FXR_RASDP_CORR_CNT_REPORT_t;

// FXR_RASDP_UNCORR_CNTR_CTL desc:
typedef union {
    struct {
        uint32_t   uncorr_clear_cntrs  :  1; // Clear all Uncorrectable error counters
        uint32_t         Reserved_3_1  :  3; // Unused
        uint32_t      uncorr_en_cntrs  :  1; // Enable Uncorrectable errors counters
        uint32_t        Reserved_19_5  : 15; // Unused
        uint32_t uncorr_cntr_select_region  :  4; // Select Uncorrectable counter region0x0: Region select for Adm Rx path 0x1: Region select for layer 3 Rx path 0x2: Region select for layer 2 Rx path 0x3: Region select for DMA inbound path 0x4: Region select for AXI bridge inbound request path 0x5: Region select for AXI bridge inbound completion path 0x6: Region select for Adm Tx path 0x7: Region select for layer 3 Tx path 0x8: Region select for layer 2 Tx path 0x9: Region select for DMA outbound path 0xa: Region select for AXI bridge outbound request path 0xb: Region select for AXI bridge outbound master completion buffer path 0xc: Reserved 0xf: Reserved
        uint32_t   uncorr_cntr_select  :  8; // Counter Selection
    } field;
    uint32_t val;
} FXR_RASDP_UNCORR_CNTR_CTL_t;

// FXR_RASDP_UNCORR_CNT_REPORT desc:
typedef union {
    struct {
        uint32_t          uncorr_cntr  :  8; // Current uncorrected error count for the selected counter
        uint32_t        Reserved_19_8  : 12; // Unused
        uint32_t uncorr_cntr_select_region  :  4; // Selected Uncorrectable counter region0x0: Region select for Adm Rx path 0x1: Region select for layer 3 Rx path 0x2: Region select for layer 2 Rx path 0x3: Region select for DMA inbound path 0x4: Region select for AXI bridge inbound request path 0x5: Region select for AXI bridge inbound completion path 0x6: Region select for Adm Tx path 0x7: Region select for layer 3 Tx path 0x8: Region select for layer 2 Tx path 0x9: Region select for DMA outbound path 0xa: Region select for AXI bridge outbound request path 0xb: Region select for AXI bridge outbound master completion buffer path 0xc: Reserved 0xf: Reserved
        uint32_t   uncorr_cntr_select  :  8; // Counter Selection.Returns the value set in the UNCORR_CNTR_SELECT field of the FXR_RASDP_UNCORR_CNTR_CTL register.
    } field;
    uint32_t val;
} FXR_RASDP_UNCORR_CNT_REPORT_t;

// FXR_RASDP_ERR_INJ_CTL desc:
typedef union {
    struct {
        uint32_t         error_inj_en  :  1; // Error Injection Global Enable
        uint32_t         Reserved_3_1  :  3; // Unused
        uint32_t       error_inj_type  :  2; // Error Injection type0: none1: 1-bit2: 2-bit
        uint32_t         Reserved_7_6  :  2; // Unused
        uint32_t      error_inj_count  :  8; // Error Injection count0: errors inserted every TLP1: one error injected2: two errors injectedn: amount of errors injected
        uint32_t        error_inj_loc  :  8; // Error Injection location
        uint32_t       Reserved_31_24  :  8; // Unused
    } field;
    uint32_t val;
} FXR_RASDP_ERR_INJ_CTL_t;

// FXR_RASDP_CORR_ERR_LOC desc:
typedef union {
    struct {
        uint32_t         Reserved_3_0  :  4; // Unused
        uint32_t reg_first_corr_error  :  4; // Region of first correctable error0x0: Region select for Adm Rx path 0x1: Region select for layer 3 Rx path 0x2: Region select for layer 2 Rx path 0x3: Region select for DMA inbound path 0x4: Region select for AXI bridge inbound request path 0x5: Region select for AXI bridge inbound completion path 0x6: Region select for Adm Tx path 0x7: Region select for layer 3 Tx path 0x8: Region select for layer 2 Tx path 0x9: Region select for DMA outbound path 0xa: Region select for AXI bridge outbound request path 0xb: Region select for AXI bridge outbound master completion buffer path 0xc: Reserved 0xf: Reserved
        uint32_t loc_first_corr_error  :  8; // Location/ID of the first corrected error within the region defined by REG_FIRST_CORR_ERROR.
        uint32_t       Reserved_19_16  :  4; // Unused
        uint32_t  reg_last_corr_error  :  4; // Selected correctable counter region0x0: Region select for Adm Rx path 0x1: Region select for layer 3 Rx path 0x2: Region select for layer 2 Rx path 0x3: Region select for DMA inbound path 0x4: Region select for AXI bridge inbound request path 0x5: Region select for AXI bridge inbound completion path 0x6: Region select for Adm Tx path 0x7: Region select for layer 3 Tx path 0x8: Region select for layer 2 Tx path 0x9: Region select for DMA outbound path 0xa: Region select for AXI bridge outbound request path 0xb: Region select for AXI bridge outbound master completion buffer path 0xc: Reserved 0xf: Reserved
        uint32_t  loc_last_corr_error  :  8; // Location/ID of the last corrected error within the region defined by REG_LAST_CORR_ERROR
    } field;
    uint32_t val;
} FXR_RASDP_CORR_ERR_LOC_t;

// FXR_RASDP_UNCORR_ERR_LOC desc:
typedef union {
    struct {
        uint32_t         Reserved_3_0  :  4; // Unused
        uint32_t reg_first_uncorr_error  :  4; // Region of first uncorrectable error0x0: Region select for Adm Rx path 0x1: Region select for layer 3 Rx path 0x2: Region select for layer 2 Rx path 0x3: Region select for DMA inbound path 0x4: Region select for AXI bridge inbound request path 0x5: Region select for AXI bridge inbound completion path 0x6: Region select for Adm Tx path 0x7: Region select for layer 3 Tx path 0x8: Region select for layer 2 Tx path 0x9: Region select for DMA outbound path 0xa: Region select for AXI bridge outbound request path 0xb: Region select for AXI bridge outbound master completion buffer path 0xc: Reserved 0xf: Reserved
        uint32_t loc_first_uncorr_error  :  8; // Location/ID of the first uncorrected error within the region defined by REG_FIRST_UNCORR_ERROR.
        uint32_t       Reserved_19_16  :  4; // Unused
        uint32_t reg_last_uncorr_error  :  4; // Selected uncorrectable counter region0x0: Region select for Adm Rx path 0x1: Region select for layer 3 Rx path 0x2: Region select for layer 2 Rx path 0x3: Region select for DMA inbound path 0x4: Region select for AXI bridge inbound request path 0x5: Region select for AXI bridge inbound completion path 0x6: Region select for Adm Tx path 0x7: Region select for layer 3 Tx path 0x8: Region select for layer 2 Tx path 0x9: Region select for DMA outbound path 0xa: Region select for AXI bridge outbound request path 0xb: Region select for AXI bridge outbound master completion buffer path 0xc: Reserved 0xf: Reserved
        uint32_t loc_last_uncorr_error  :  8; // Location/ID of the last uncorrected error within the region defined by REG_LAST_UNCORR_ERROR
    } field;
    uint32_t val;
} FXR_RASDP_UNCORR_ERR_LOC_t;

// FXR_RASDP_ERR_MODE_EN desc:
typedef union {
    struct {
        uint32_t        error_mode_en  :  1; // Enable the controller to enter RASDP error mode when it detects an uncorrectable error.
        uint32_t    auto_link_down_en  :  1; // Enables controller to bring the link down when the controller enters RASDP error mode.
        uint32_t        Reserved_31_2  : 30; // Unused
    } field;
    uint32_t val;
} FXR_RASDP_ERR_MODE_EN_t;

// FXR_RASDP_ERR_MODE_CLEAR desc:
typedef union {
    struct {
        uint32_t     error_mode_clear  :  1; // Write 1 to take the controller our of RASDP error mode.
        uint32_t        Reserved_31_1  : 31; // Unused
    } field;
    uint32_t val;
} FXR_RASDP_ERR_MODE_CLEAR_t;

// FXR_RASDP_RAM_ADDR_CORR_ERR desc:
typedef union {
    struct {
        uint32_t  ram_addr_corr_error  : 27; // RAM address where a corrected error has been detected.
        uint32_t          Reserved_27  :  1; // Unused
        uint32_t ram_index_corr_error  :  4; // Ram index where a corrected error has been detected.
    } field;
    uint32_t val;
} FXR_RASDP_RAM_ADDR_CORR_ERR_t;

// FXR_RASDP_RAM_ADDR_UNCORR_ERR desc:
typedef union {
    struct {
        uint32_t ram_addr_uncorr_error  : 27; // RAM address where an uncorrected error has been detected.
        uint32_t          Reserved_27  :  1; // Unused
        uint32_t ram_index_uncorr_error  :  4; // Ram index where an uncorrected error has been detected.
    } field;
    uint32_t val;
} FXR_RASDP_RAM_ADDR_UNCORR_ERR_t;

// FXR_DLINK_EXT_CAPABILITY desc:
typedef union {
    struct {
        uint32_t               cap_id  : 16; // PCIe Capabilities ID. Hardcoded PCIe ID
        uint32_t          cap_version  :  4; // Capability Version. PCI-SIG defined PCIe Capability structure version number.
        uint32_t      next_capability  : 12; // Next Capability Offset. Offset to the next PCI Express Extended Capability structure
    } field;
    uint32_t val;
} FXR_DLINK_EXT_CAPABILITY_t;

// FXR_DLINK_FEATURE_CAPABILITY desc:
typedef union {
    struct {
        uint32_t     scaled_flow_cntl  :  1; // Local Scaled Flow Control Supported.
        uint32_t       future_support  : 22; // Local Future Data Link Feature Supported.
        uint32_t       Reserved_30_23  :  8; // Unused
        uint32_t       dl_exchange_en  :  1; // Data Link Feature Exchange Enable
    } field;
    uint32_t val;
} FXR_DLINK_FEATURE_CAPABILITY_t;

// FXR_DLINK_FEATURE_STATUS desc:
typedef union {
    struct {
        uint32_t  remote_dl_supported  : 23; // Features Currently defined:bit0-Remote Scaled Flow Control Supported
        uint32_t       Reserved_30_23  :  8; // Unused
        uint32_t      dl_status_valid  :  1; // Removed Data Link Feature Supported Valid
    } field;
    uint32_t val;
} FXR_DLINK_FEATURE_STATUS_t;

// FXR_PTM_EXT_CAPABILITY desc:
typedef union {
    struct {
        uint32_t               cap_id  : 16; // PCIe Capabilities ID. Hardcoded PCIe ID
        uint32_t          cap_version  :  4; // Capability Version. PCI-SIG defined PCIe Capability structure version number.
        uint32_t      next_capability  : 12; // Next Capability Offset. Offset to the next PCI Express Extended Capability structure
    } field;
    uint32_t val;
} FXR_PTM_EXT_CAPABILITY_t;

// FXR_PTM_CAPABILITY desc:
typedef union {
    struct {
        uint32_t          ptm_req_cap  :  1; // PTM Requester Capable.
        uint32_t          ptm_res_cap  :  1; // PTM Responder Capable.
        uint32_t         ptm_root_cap  :  1; // PTM Root Capable.
        uint32_t        Reserved_31_3  : 29; // Unused
    } field;
    uint32_t val;
} FXR_PTM_CAPABILITY_t;

// FXR_PTM_CONTROL desc:
typedef union {
    struct {
        uint32_t           ptm_enable  :  1; // PTM Enable
        uint32_t          root_select  :  1; // PTM Root Select
        uint32_t         Reserved_7_2  :  6; // Unused
        uint32_t             eff_gran  :  8; // PTM Effective Granularity
        uint32_t       Reserved_31_16  : 16; // Unused
    } field;
    uint32_t val;
} FXR_PTM_CONTROL_t;

// FXR_PTM_REQ_CAPABILITY desc:
typedef union {
    struct {
        uint32_t               cap_id  : 16; // PCIe Capabilities ID. Hardcoded PCIe ID
        uint32_t          cap_version  :  4; // Capability Version. PCI-SIG defined PCIe Capability structure version number.
        uint32_t      next_capability  : 12; // Next Capability Offset. Offset to the next PCI Express Extended Capability structure
    } field;
    uint32_t val;
} FXR_PTM_REQ_CAPABILITY_t;

// FXR_PTM_REQ_HEADER desc:
typedef union {
    struct {
        uint32_t          req_vsec_id  : 16; // PTM Requester VSEC ID
        uint32_t         req_vsec_rev  :  4; // PTM Requester VSEC Revision
        uint32_t      req_vsec_length  : 12; // PTM Requester VSEC Length
    } field;
    uint32_t val;
} FXR_PTM_REQ_HEADER_t;

// FXR_PTM_REQ_CONTROL desc:
typedef union {
    struct {
        uint32_t req_auto_update_enabled  :  1; // PTM Requester Auto Update Enabled. When enabled PTM Requester will automatically atempt to update it's context every 10ms.
        uint32_t     req_start_update  :  1; // PTM Requester Start Update. When set the PTM Requester will attempt a PTM Dialogue to update it's context; This bit is self clearing.
        uint32_t      req_fast_timers  :  1; // PTM Fast Timers. Debug mode for PTM Timers. The 100us timer output will go high at 30us and the 10ms timer output will go high at 100us (The Long Timer Value is ignored). There is no change to the 1us timer. The requester operation will otherwise remain the same.
        uint32_t         Reserved_7_3  :  5; // Unused
        uint32_t       req_long_timer  :  8; // PTM Requester Long Timer. Determines the period between each auto update PTM Dialogue in miliseconds. Update period is the register value +1 milisecond.
        uint32_t       Reserved_31_16  : 16; // Unused
    } field;
    uint32_t val;
} FXR_PTM_REQ_CONTROL_t;

// FXR_PTM_REQ_STATUS desc:
typedef union {
    struct {
        uint32_t    req_context_valid  :  1; // PTM Requester Context Valid. Indicate that the Timing Context is valid.
        uint32_t req_man_update_allowed  :  1; // PTM Requester Manual Update Allowed. Indicates whether or not a Manual Update can be signalled.
        uint32_t        Reserved_31_2  : 30; // Unused
    } field;
    uint32_t val;
} FXR_PTM_REQ_STATUS_t;

// FXR_PTM_REQ_LOCAL_LSB desc:
typedef union {
    struct {
        uint32_t        req_local_lsb  : 32; // PTM Requester Local Clock LSB . Lower 32 bits of local timer value
    } field;
    uint32_t val;
} FXR_PTM_REQ_LOCAL_LSB_t;

// FXR_PTM_REQ_LOCAL_MSB desc:
typedef union {
    struct {
        uint32_t        req_local_msb  : 32; // PTM Requester Local Clock MSB . Upper 32 bits of local timer value
    } field;
    uint32_t val;
} FXR_PTM_REQ_LOCAL_MSB_t;

// FXR_PTM_REQ_T1_LSB desc:
typedef union {
    struct {
        uint32_t           req_t1_lsb  : 32; // PTM Requester T1 Timestamp LSB . Lower 32 bits of T1 value
    } field;
    uint32_t val;
} FXR_PTM_REQ_T1_LSB_t;

// FXR_PTM_REQ_T1_MSB desc:
typedef union {
    struct {
        uint32_t           req_t1_msb  : 32; // PTM Requester T1 Timestamp MSB . Upper 32 bits of T1 value
    } field;
    uint32_t val;
} FXR_PTM_REQ_T1_MSB_t;

// FXR_PTM_REQ_T1P_LSB desc:
typedef union {
    struct {
        uint32_t          req_t1p_lsb  : 32; // PTM Requester T1 Previous Timestamp LSB . Lower 32 bits of T1P value
    } field;
    uint32_t val;
} FXR_PTM_REQ_T1P_LSB_t;

// FXR_PTM_REQ_T1P_MSB desc:
typedef union {
    struct {
        uint32_t          req_t1p_msb  : 32; // PTM Requester T1 Previous Timestamp MSB . Upper 32 bits of T1P value
    } field;
    uint32_t val;
} FXR_PTM_REQ_T1P_MSB_t;

// FXR_PTM_REQ_T4_LSB desc:
typedef union {
    struct {
        uint32_t           req_t4_lsb  : 32; // PTM Requester T4 Timestamp LSB . Lower 32 bits of T4 value
    } field;
    uint32_t val;
} FXR_PTM_REQ_T4_LSB_t;

// FXR_PTM_REQ_T4_MSB desc:
typedef union {
    struct {
        uint32_t           req_t4_msb  : 32; // PTM Requester T4 Timestamp MSB . Upper 32 bits of T4 value
    } field;
    uint32_t val;
} FXR_PTM_REQ_T4_MSB_t;

// FXR_PTM_REQ_T4P_LSB desc:
typedef union {
    struct {
        uint32_t          req_t4p_lsb  : 32; // PTM Requester T4 Previous Timestamp LSB . Lower 32 bits of T4P value
    } field;
    uint32_t val;
} FXR_PTM_REQ_T4P_LSB_t;

// FXR_PTM_REQ_T4P_MSB desc:
typedef union {
    struct {
        uint32_t          req_t4p_msb  : 32; // PTM Requester T4 Previous Timestamp MSB . Upper 32 bits of T4P value
    } field;
    uint32_t val;
} FXR_PTM_REQ_T4P_MSB_t;

// FXR_PTM_REQ_MASTER_LSB desc:
typedef union {
    struct {
        uint32_t       req_master_lsb  : 32; // PTM Requester Master LSB . Lower 32 bits of Master time value
    } field;
    uint32_t val;
} FXR_PTM_REQ_MASTER_LSB_t;

// FXR_PTM_REQ_MASTER_MSB desc:
typedef union {
    struct {
        uint32_t       req_master_msb  : 32; // PTM Requester Master Time MSB . Upper 32 bits of Master time value
    } field;
    uint32_t val;
} FXR_PTM_REQ_MASTER_MSB_t;

// FXR_PTM_REQ_PROP_DELAY desc:
typedef union {
    struct {
        uint32_t       req_prop_delay  : 32; // PTM Requester Propagation Delay. Upper 32 bits of prop delay value
    } field;
    uint32_t val;
} FXR_PTM_REQ_PROP_DELAY_t;

// FXR_PTM_REQ_MASTERT1_LSB desc:
typedef union {
    struct {
        uint32_t     req_mastert1_lsb  : 32; // PTM Requester Master at T1 LSB . Lower 32 bits of Master time at T1 value
    } field;
    uint32_t val;
} FXR_PTM_REQ_MASTERT1_LSB_t;

// FXR_PTM_REQ_MASTERT1_MSB desc:
typedef union {
    struct {
        uint32_t     req_mastert1_msb  : 32; // PTM Requester Master Time at T1 MSB . Upper 32 bits of Master time at T1 value
    } field;
    uint32_t val;
} FXR_PTM_REQ_MASTERT1_MSB_t;

// FXR_PTM_REQ_TX_LATENCY desc:
typedef union {
    struct {
        uint32_t       req_tx_latency  : 12; // PTM Requester TX latency. Transmit path latency
        uint32_t       Reserved_31_12  : 20; // Unused
    } field;
    uint32_t val;
} FXR_PTM_REQ_TX_LATENCY_t;

// FXR_PTM_REQ_RX_LATENCY desc:
typedef union {
    struct {
        uint32_t       Reserved_31_12  : 20; // Unused
    } field;
    uint32_t val;
} FXR_PTM_REQ_RX_LATENCY_t;

#endif /* ___FXR_config_CSRS_H__ */
