// SPDX-License-Identifier: GPL-2.0

#include "vmcs12.h"

#define ROL16(val, n) ((u16)(((u16)(val) << (n)) | ((u16)(val) >> (16 - (n)))))
#define VMCS12_OFFSET(x) offsetof(struct vmcs12, x)

#define VMCS_CHECK_SIZE16(number) \
	(BUILD_BUG_ON_ZERO(__builtin_constant_p(number) && \
	((number) & 0x6001) == 0x2000) + \
	BUILD_BUG_ON_ZERO(__builtin_constant_p(number) && \
	((number) & 0x6001) == 0x2001) + \
	BUILD_BUG_ON_ZERO(__builtin_constant_p(number) && \
	((number) & 0x6000) == 0x4000) + \
	BUILD_BUG_ON_ZERO(__builtin_constant_p(number) && \
	((number) & 0x6000) == 0x6000))

#define VMCS_CHECK_SIZE32(number) \
	(BUILD_BUG_ON_ZERO(__builtin_constant_p(number) && \
	((number) & 0x6000) == 0) + \
	BUILD_BUG_ON_ZERO(__builtin_constant_p(number) && \
	((number) & 0x6000) == 0x6000))

#define VMCS_CHECK_SIZE64(number) \
	(BUILD_BUG_ON_ZERO(__builtin_constant_p(number) && \
	((number) & 0x6000) == 0) + \
	BUILD_BUG_ON_ZERO(__builtin_constant_p(number) && \
	((number) & 0x6001) == 0x2001) + \
	BUILD_BUG_ON_ZERO(__builtin_constant_p(number) && \
	((number) & 0x6000) == 0x4000) + \
	BUILD_BUG_ON_ZERO(__builtin_constant_p(number) && \
	((number) & 0x6000) == 0x6000))

#define VMCS_CHECK_SIZE_N(number) \
	(BUILD_BUG_ON_ZERO(__builtin_constant_p(number) && \
	((number) & 0x6000) == 0) + \
	BUILD_BUG_ON_ZERO(__builtin_constant_p(number) && \
	((number) & 0x6001) == 0x2000) + \
	BUILD_BUG_ON_ZERO(__builtin_constant_p(number) && \
	((number) & 0x6001) == 0x2001) + \
	BUILD_BUG_ON_ZERO(__builtin_constant_p(number) && \
	((number) & 0x6000) == 0x4000))

#define FIELD16(number, name) \
	[ROL16(number, 6)] = VMCS_CHECK_SIZE16(number) + VMCS12_OFFSET(name)

#define FIELD32(number, name) \
	[ROL16(number, 6)] = VMCS_CHECK_SIZE32(number) + VMCS12_OFFSET(name)

#define FIELD64(number, name)  \
	FIELD32(number, name), \
	[ROL16(number##_HIGH, 6)] = VMCS_CHECK_SIZE32(number) + \
	VMCS12_OFFSET(name) + sizeof(u32)
#define FIELDN(number, name) \
	[ROL16(number, 6)] = VMCS_CHECK_SIZE_N(number) + VMCS12_OFFSET(name)

const unsigned short vmcs_field_to_offset_table[] = {
	FIELD16(VIRTUAL_PROCESSOR_ID, virtual_processor_id),
	FIELD16(POSTED_INTR_NV, posted_intr_nv),
	FIELD16(GUEST_ES_SELECTOR, guest_es_selector),
	FIELD16(GUEST_CS_SELECTOR, guest_cs_selector),
	FIELD16(GUEST_SS_SELECTOR, guest_ss_selector),
	FIELD16(GUEST_DS_SELECTOR, guest_ds_selector),
	FIELD16(GUEST_FS_SELECTOR, guest_fs_selector),
	FIELD16(GUEST_GS_SELECTOR, guest_gs_selector),
	FIELD16(GUEST_LDTR_SELECTOR, guest_ldtr_selector),
	FIELD16(GUEST_TR_SELECTOR, guest_tr_selector),
	FIELD16(GUEST_INTR_STATUS, guest_intr_status),
	FIELD16(GUEST_PML_INDEX, guest_pml_index),
	FIELD16(HOST_ES_SELECTOR, host_es_selector),
	FIELD16(HOST_CS_SELECTOR, host_cs_selector),
	FIELD16(HOST_SS_SELECTOR, host_ss_selector),
	FIELD16(HOST_DS_SELECTOR, host_ds_selector),
	FIELD16(HOST_FS_SELECTOR, host_fs_selector),
	FIELD16(HOST_GS_SELECTOR, host_gs_selector),
	FIELD16(HOST_TR_SELECTOR, host_tr_selector),
	FIELD64(IO_BITMAP_A, io_bitmap_a),
	FIELD64(IO_BITMAP_B, io_bitmap_b),
	FIELD64(MSR_BITMAP, msr_bitmap),
	FIELD64(VM_EXIT_MSR_STORE_ADDR, vm_exit_msr_store_addr),
	FIELD64(VM_EXIT_MSR_LOAD_ADDR, vm_exit_msr_load_addr),
	FIELD64(VM_ENTRY_MSR_LOAD_ADDR, vm_entry_msr_load_addr),
	FIELD64(PML_ADDRESS, pml_address),
	FIELD64(TSC_OFFSET, tsc_offset),
	FIELD64(VIRTUAL_APIC_PAGE_ADDR, virtual_apic_page_addr),
	FIELD64(APIC_ACCESS_ADDR, apic_access_addr),
	FIELD64(POSTED_INTR_DESC_ADDR, posted_intr_desc_addr),
	FIELD64(VM_FUNCTION_CONTROL, vm_function_control),
	FIELD64(EPT_POINTER, ept_pointer),
	FIELD64(EOI_EXIT_BITMAP0, eoi_exit_bitmap0),
	FIELD64(EOI_EXIT_BITMAP1, eoi_exit_bitmap1),
	FIELD64(EOI_EXIT_BITMAP2, eoi_exit_bitmap2),
	FIELD64(EOI_EXIT_BITMAP3, eoi_exit_bitmap3),
	FIELD64(EPTP_LIST_ADDRESS, eptp_list_address),
	FIELD64(VMREAD_BITMAP, vmread_bitmap),
	FIELD64(VMWRITE_BITMAP, vmwrite_bitmap),
	FIELD64(XSS_EXIT_BITMAP, xss_exit_bitmap),
	FIELD64(GUEST_PHYSICAL_ADDRESS, guest_physical_address),
	FIELD64(VMCS_LINK_POINTER, vmcs_link_pointer),
	FIELD64(GUEST_IA32_DEBUGCTL, guest_ia32_debugctl),
	FIELD64(GUEST_IA32_PAT, guest_ia32_pat),
	FIELD64(GUEST_IA32_EFER, guest_ia32_efer),
	FIELD64(GUEST_IA32_PERF_GLOBAL_CTRL, guest_ia32_perf_global_ctrl),
	FIELD64(GUEST_PDPTR0, guest_pdptr0),
	FIELD64(GUEST_PDPTR1, guest_pdptr1),
	FIELD64(GUEST_PDPTR2, guest_pdptr2),
	FIELD64(GUEST_PDPTR3, guest_pdptr3),
	FIELD64(GUEST_BNDCFGS, guest_bndcfgs),
	FIELD64(GUEST_IA32_PKRS, guest_ia32_pkrs),
	FIELD64(HOST_IA32_PAT, host_ia32_pat),
	FIELD64(HOST_IA32_EFER, host_ia32_efer),
	FIELD64(HOST_IA32_PERF_GLOBAL_CTRL, host_ia32_perf_global_ctrl),
	FIELD64(HOST_IA32_PKRS, host_ia32_pkrs),
	FIELD32(PIN_BASED_VM_EXEC_CONTROL, pin_based_vm_exec_control),
	FIELD32(CPU_BASED_VM_EXEC_CONTROL, cpu_based_vm_exec_control),
	FIELD32(EXCEPTION_BITMAP, exception_bitmap),
	FIELD32(PAGE_FAULT_ERROR_CODE_MASK, page_fault_error_code_mask),
	FIELD32(PAGE_FAULT_ERROR_CODE_MATCH, page_fault_error_code_match),
	FIELD32(CR3_TARGET_COUNT, cr3_target_count),
	FIELD32(VM_EXIT_CONTROLS, vm_exit_controls),
	FIELD32(VM_EXIT_MSR_STORE_COUNT, vm_exit_msr_store_count),
	FIELD32(VM_EXIT_MSR_LOAD_COUNT, vm_exit_msr_load_count),
	FIELD32(VM_ENTRY_CONTROLS, vm_entry_controls),
	FIELD32(VM_ENTRY_MSR_LOAD_COUNT, vm_entry_msr_load_count),
	FIELD32(VM_ENTRY_INTR_INFO_FIELD, vm_entry_intr_info_field),
	FIELD32(VM_ENTRY_EXCEPTION_ERROR_CODE, vm_entry_exception_error_code),
	FIELD32(VM_ENTRY_INSTRUCTION_LEN, vm_entry_instruction_len),
	FIELD32(TPR_THRESHOLD, tpr_threshold),
	FIELD32(SECONDARY_VM_EXEC_CONTROL, secondary_vm_exec_control),
	FIELD32(VM_INSTRUCTION_ERROR, vm_instruction_error),
	FIELD32(VM_EXIT_REASON, vm_exit_reason),
	FIELD32(VM_EXIT_INTR_INFO, vm_exit_intr_info),
	FIELD32(VM_EXIT_INTR_ERROR_CODE, vm_exit_intr_error_code),
	FIELD32(IDT_VECTORING_INFO_FIELD, idt_vectoring_info_field),
	FIELD32(IDT_VECTORING_ERROR_CODE, idt_vectoring_error_code),
	FIELD32(VM_EXIT_INSTRUCTION_LEN, vm_exit_instruction_len),
	FIELD32(VMX_INSTRUCTION_INFO, vmx_instruction_info),
	FIELD32(GUEST_ES_LIMIT, guest_es_limit),
	FIELD32(GUEST_CS_LIMIT, guest_cs_limit),
	FIELD32(GUEST_SS_LIMIT, guest_ss_limit),
	FIELD32(GUEST_DS_LIMIT, guest_ds_limit),
	FIELD32(GUEST_FS_LIMIT, guest_fs_limit),
	FIELD32(GUEST_GS_LIMIT, guest_gs_limit),
	FIELD32(GUEST_LDTR_LIMIT, guest_ldtr_limit),
	FIELD32(GUEST_TR_LIMIT, guest_tr_limit),
	FIELD32(GUEST_GDTR_LIMIT, guest_gdtr_limit),
	FIELD32(GUEST_IDTR_LIMIT, guest_idtr_limit),
	FIELD32(GUEST_ES_AR_BYTES, guest_es_ar_bytes),
	FIELD32(GUEST_CS_AR_BYTES, guest_cs_ar_bytes),
	FIELD32(GUEST_SS_AR_BYTES, guest_ss_ar_bytes),
	FIELD32(GUEST_DS_AR_BYTES, guest_ds_ar_bytes),
	FIELD32(GUEST_FS_AR_BYTES, guest_fs_ar_bytes),
	FIELD32(GUEST_GS_AR_BYTES, guest_gs_ar_bytes),
	FIELD32(GUEST_LDTR_AR_BYTES, guest_ldtr_ar_bytes),
	FIELD32(GUEST_TR_AR_BYTES, guest_tr_ar_bytes),
	FIELD32(GUEST_INTERRUPTIBILITY_INFO, guest_interruptibility_info),
	FIELD32(GUEST_ACTIVITY_STATE, guest_activity_state),
	FIELD32(GUEST_SYSENTER_CS, guest_sysenter_cs),
	FIELD32(HOST_IA32_SYSENTER_CS, host_ia32_sysenter_cs),
	FIELD32(VMX_PREEMPTION_TIMER_VALUE, vmx_preemption_timer_value),
	FIELDN(CR0_GUEST_HOST_MASK, cr0_guest_host_mask),
	FIELDN(CR4_GUEST_HOST_MASK, cr4_guest_host_mask),
	FIELDN(CR0_READ_SHADOW, cr0_read_shadow),
	FIELDN(CR4_READ_SHADOW, cr4_read_shadow),
	FIELDN(CR3_TARGET_VALUE0, cr3_target_value0),
	FIELDN(CR3_TARGET_VALUE1, cr3_target_value1),
	FIELDN(CR3_TARGET_VALUE2, cr3_target_value2),
	FIELDN(CR3_TARGET_VALUE3, cr3_target_value3),
	FIELDN(EXIT_QUALIFICATION, exit_qualification),
	FIELDN(GUEST_LINEAR_ADDRESS, guest_linear_address),
	FIELDN(GUEST_CR0, guest_cr0),
	FIELDN(GUEST_CR3, guest_cr3),
	FIELDN(GUEST_CR4, guest_cr4),
	FIELDN(GUEST_ES_BASE, guest_es_base),
	FIELDN(GUEST_CS_BASE, guest_cs_base),
	FIELDN(GUEST_SS_BASE, guest_ss_base),
	FIELDN(GUEST_DS_BASE, guest_ds_base),
	FIELDN(GUEST_FS_BASE, guest_fs_base),
	FIELDN(GUEST_GS_BASE, guest_gs_base),
	FIELDN(GUEST_LDTR_BASE, guest_ldtr_base),
	FIELDN(GUEST_TR_BASE, guest_tr_base),
	FIELDN(GUEST_GDTR_BASE, guest_gdtr_base),
	FIELDN(GUEST_IDTR_BASE, guest_idtr_base),
	FIELDN(GUEST_DR7, guest_dr7),
	FIELDN(GUEST_RSP, guest_rsp),
	FIELDN(GUEST_RIP, guest_rip),
	FIELDN(GUEST_RFLAGS, guest_rflags),
	FIELDN(GUEST_PENDING_DBG_EXCEPTIONS, guest_pending_dbg_exceptions),
	FIELDN(GUEST_SYSENTER_ESP, guest_sysenter_esp),
	FIELDN(GUEST_SYSENTER_EIP, guest_sysenter_eip),
	FIELDN(GUEST_S_CET, guest_s_cet),
	FIELDN(GUEST_SSP, guest_ssp),
	FIELDN(GUEST_INTR_SSP_TABLE, guest_ssp_tbl),
	FIELDN(HOST_CR0, host_cr0),
	FIELDN(HOST_CR3, host_cr3),
	FIELDN(HOST_CR4, host_cr4),
	FIELDN(HOST_FS_BASE, host_fs_base),
	FIELDN(HOST_GS_BASE, host_gs_base),
	FIELDN(HOST_TR_BASE, host_tr_base),
	FIELDN(HOST_GDTR_BASE, host_gdtr_base),
	FIELDN(HOST_IDTR_BASE, host_idtr_base),
	FIELDN(HOST_IA32_SYSENTER_ESP, host_ia32_sysenter_esp),
	FIELDN(HOST_IA32_SYSENTER_EIP, host_ia32_sysenter_eip),
	FIELDN(HOST_RSP, host_rsp),
	FIELDN(HOST_RIP, host_rip),
	FIELDN(HOST_S_CET, host_s_cet),
	FIELDN(HOST_SSP, host_ssp),
	FIELDN(HOST_INTR_SSP_TABLE, host_ssp_tbl),
};
const unsigned int nr_vmcs12_fields = ARRAY_SIZE(vmcs_field_to_offset_table);
