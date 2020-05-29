/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _ASM_X86_RAR_H
#define _ASM_X86_RAR_H

#include <asm/tlbflush.h>

/*
 * RAR payload types
 */
#define RAR_TYPE_INVPG		0
#define RAR_TYPE_INVPG_NO_CR3	1
#define RAR_TYPE_INVPCID	2
#define RAR_TYPE_INVEPT		3
#define RAR_TYPE_INVVPID	4
#define RAR_TYPE_WRMSR		5

/*
 * Subtypes for RAR_TYPE_INVLPG
 */
#define RAR_INVPG_ADDR_RANGE		0 /* address specific */
#define RAR_INVPG_ALL			2 /* all, include global */
#define RAR_INVPG_ALL_NO_GLOBAL		3 /* all, exclude global */

/*
 * Subtypes for RAR_TYPE_INVPCID
 */
#define RAR_INVPCID_ADDR_RANGE		0 /* one PCID with address range */
#define RAR_INVPCID_ONE_PCID_ALL	1 /* one PCID full */
#define RAR_INVPCID_ALL_GLOBAL		2 /* all PCIDs, include global */
#define RAR_INVPCID_ALL_NO_GLOBAL	3 /* all PCIDs, exclude global */

/*
 * Stride size for RAR TLB Flush
 */
#define RAR_STRIDE_4K		0
#define RAR_STRIDE_2M		1
#define RAR_STRIDE_1G		2

#define RAR_MAX_PAGES 63

#define RAR_TYPE_SHIFT		8
#define RAR_SUBTYPE_SHIFT	32
#define RAR_STRIDE_SHIFT	35
#define RAR_PAGES_SHIFT		37


struct rar_payload {
	union {
		/* tlb flush */
		struct {
			uint64_t tlb_flush;
			uint64_t must_be_zero;
			union {
				uint64_t cr3;
				uint64_t pcid;
				uint64_t eptp;
				uint64_t vpid;
			};
			uint64_t addr;
		} __packed;

		/* wrmsrl */
		struct {
			uint64_t msr_write;
			uint64_t msr_mbz_0;
			uint64_t msr_data;
			uint64_t msr_mbz_1;
		} __packed;
	};

	uint64_t padding[4];
} __packed;

#define RAR_ACTION_OK		0x00
#define RAR_ACTION_START	0x01
#define RAR_ACTION_ACKED	0x02
#define RAR_ACTION_FAIL		0x80

#define RAR_MAX_PAYLOADS 64UL

#ifdef CONFIG_X86_INTEL_RAR
void rar_cpu_init(void);
void rar_flush_tlb_others(const struct cpumask *cpumask, unsigned long pcid,
			  const struct flush_tlb_info *info);
#else
static inline void rar_cpu_init(void) {}
static inline void rar_flush_tlb_others(const struct cpumask *cpumask,
					unsigned long pcid,
					const struct flush_tlb_info *info) {}
#endif

#endif /* _ASM_X86_RAR_H */
