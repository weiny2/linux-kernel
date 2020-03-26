/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Intel Key Locker support
 */

#ifndef _ASM_KEYLOCKER_H
#define _ASM_KEYLOCKER_H

#ifndef __ASSEMBLY__

#include <linux/bits.h>
#include <linux/delay.h>

#include <asm/msr.h>
#include <asm/processor.h>

/*
 * MSR entries for saving, restoring, and status of internal
 * wrapping key
 */
#define MSR_IA32_COPY_LOCAL_TO_PLATFORM	0xd91
#define MSR_IA32_COPY_PLATFORM_TO_LOCAL	0xd92
#define MSR_IA32_COPY_STATUS		0x990
#define MSR_IA32_IWKEY_BACKUP_STATUS	0x991

/* Load a randomized internal wrapping key */
static inline int x86_load_iwkey(void)
{
	u32 eax, ebx, ecx, edx;

	/* The CPUID leaf (0x19,0) enumerates detail capability */
	cpuid(0x19, &eax, &ebx, &ecx, &edx);

	/*
	 * Check the internal wrapping key support:
	 *
	 * ebx[4]: When 1, supports backing up the internal
	 *		   wrapping key to the platform register
	 * ecx[1]: When 1, supports randomizing the key value
	 */
	if (!(ebx & BIT(4)) || !(ecx & BIT(1)))
		return 1;

	/* Perform LOADIWKEY instruction */
	asm volatile(".byte 0xf3, 0x0f, 0x38, 0xdc, 0xc8"
		     :: "a" (0x2) : "memory");
	return 0;
}

static inline void x86_backup_iwkey(void)
{
	/*
	 * Writing the MSR initiates backing up process.
	 * However, it takes time to finish as writing
	 * flash memory. Thus, the return of WRMSR does
	 * not mean backup completion.
	 */
	wrmsrl(MSR_IA32_COPY_LOCAL_TO_PLATFORM, 1);
}

/*
 * Check the backup status of the internal wrapping wrapping key
 * in the platform register. It attempts to check again in the
 * given times, which is considered to be useful when polling the
 * status right after backup request.
 */
static inline int x86_check_iwkey_backup(u32 wait_cnt)
{
	unsigned long status = 0;
	int i;

	for (i = 0; (i <= wait_cnt) && !status; i++) {
		if (i)
			udelay(1);

		rdmsrl(MSR_IA32_IWKEY_BACKUP_STATUS, status);
		status &= BIT(0);
	}

	return status ? 0 : 1;
}

/*
 * Request to copy the internal wrapping key from the non-volatile
 * platform register to the current CPU. The copy process is expected
 * to take less than 100 cycles, but it allows checking again in the
 * given times.
 */
static inline int x86_copy_iwkey(u32 wait_cnt)
{
	unsigned long status = 0;
	int i;

	wrmsrl(MSR_IA32_COPY_PLATFORM_TO_LOCAL, 1);

	for (i = 0; (i <= wait_cnt) && !status; i++) {
		if (i)
			udelay(1);

		rdmsrl(MSR_IA32_COPY_STATUS, status);
		status &= BIT(0);
	}

	return status ? 0 : 1;
}

#endif /*__ASSEMBLY__ */

#endif /* _ASM_KEYLOCKER_H */
