/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _ASM_X86_INTEL_HFI_H
#define _ASM_X86_INTEL_HFI_H

/* Hardware Feedback Interface Enumeration */
#define CPUID_HFI_LEAF			6
#define CPUID_HFI_CAP_MASK		0xff
#define CPUID_HFI_TABLE_SIZE_MASK	0x0f00
#define CPUID_HFI_TABLE_SIZE_SHIFT	8
#define CPUID_HFI_CPU_INDEX_MASK	0xffff0000
#define CPUID_HFI_CPU_INDEX_SHIFT	16

/* Hardware Feedback Interface Pointer */
#define HFI_PTR_VALID_BIT		BIT(0)
#define HFI_PTR_ADDR_SHIFT		12

/* Hardware Feedback Interface Configuration */
#define HFI_CONFIG_ENABLE_BIT		BIT(0)

/* Hardware Feedback Interface Capabilities */
#define HFI_PERFORMANCE_REPORTING	BIT(0)
#define HFI_ENERGY_EFFICIENCY_REPORTING	BIT(1)

#endif /* _ASM_X86_INTEL_HFI_H */
