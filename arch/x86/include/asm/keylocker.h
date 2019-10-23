/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Intel KeyLocker support
 */

#ifndef _ASM_KEYLOCKER_H
#define _ASM_KEYLOCKER_H

#ifndef __ASSEMBLY__

#include <linux/bits.h>
#include <linux/delay.h>

#include <asm/msr.h>
#include <asm/processor.h>

#define KEYLOCKER_CPUID			0x019
#define KEYLOCKER_CPUID_EBX_BACKUP	BIT(4)
#define KEYLOCKER_CPUID_ECX_RAND	BIT(1)

/* The CPU-internal key related definitions */

#define LOADIWKEY_RETRY_LOOPS		10
#define LOADIWKEY	".byte 0xf3,0x0f,0x38,0xdc,0xc8"

#define MSR_IA32_COPY_LOCAL_TO_PLATFORM	0xd91
#define MSR_IA32_COPY_PLATFORM_TO_LOCAL	0xd92
#define MSR_IA32_COPY_STATUS		0x990
#define MSR_IA32_IWKEY_BACKUP_STATUS	0x991

#define IWKEY_BACKUP_STATUS_RETRY_LOOPS	100000
#define IWKEY_RESTORE_RETRY_LOOPS	1

/* Load a randomized CPU-internal key */
static inline bool x86_load_new_internal_key(void)
{
	unsigned int retry = LOADIWKEY_RETRY_LOOPS;

	do {
		bool err;

		asm volatile(LOADIWKEY
			     CC_SET(z)
			     : CC_OUT(z) (err)
			     : "a" (0x2) : "memory");
		if (!err)
			return true;
	} while (--retry);
	return false;
}

/*
 * Check the status of the internal key saved in the platform-scoped state.
 * Return true if the status is valid; otherwise, read the status in a
 * given time until finding valid, because the status update may get
 * delayed. After those multiple reads, it returns false when the status is
 * still invalid.
 */
static inline bool x86_check_saved_internal_key(int retry_count)
{
	u64 status = 0;
	int i;

	for (i = 0; (i <= retry_count) && !status; i++) {
		if (i)
			udelay(1);

		rdmsrl(MSR_IA32_IWKEY_BACKUP_STATUS, status);
		status &= BIT(0);
	}

	return status ? true : false;
}

/*
 * Writing the MSR initiates the process to save the CPU-internal key. It
 * comes many cycles, as writing in flash memory. The return of MSR write
 * execution does not mean saving completion. Thus, it needs to poll the
 * saved status separately.
 */
static inline void x86_save_internal_key(void)
{
	wrmsrl(MSR_IA32_COPY_LOCAL_TO_PLATFORM, 1);
}

/*
 * Request to restore the internal key from the platform-scoped state.
 * While not expecting much time as the saving process, it double-checks
 * the state of the restored CPU-internal key.
 */
static inline bool x86_restore_internal_key(void)
{
	u64 status = 0;
	int i;

	wrmsrl(MSR_IA32_COPY_PLATFORM_TO_LOCAL, 1);

	for (i = 0; (i <= IWKEY_RESTORE_RETRY_LOOPS) && !status; i++) {
		if (i)
			udelay(1);

		rdmsrl(MSR_IA32_COPY_STATUS, status);
		status &= BIT(0);
	}

	return status ? true : false;
}

#endif /*__ASSEMBLY__ */

#endif /* _ASM_KEYLOCKER_H */
