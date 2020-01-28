/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_PKS_PKEYS_H
#define _ASM_X86_PKS_PKEYS_H

#define PKRU_AD_BIT 0x1
#define PKRU_WD_BIT 0x2
#define PKRU_BITS_PER_PKEY 2

#ifdef	CONFIG_X86_INTEL_MEMORY_PROTECTION_KEYS
#define PKRU_AD_KEY(pkey)	(PKRU_AD_BIT << ((pkey) * PKRU_BITS_PER_PKEY))

/*
 * Make the default PKS value for INIT_THREAD or init MSR:
 * Key 0 has no restriction. All other keys has AD=1.
 * This ensures that any threads will not accidentally get access to data
 * which is pkey-protected later on.
 * Reuse PKRU_AD_KEY() since PKRU has same AD and WD bits definitions.
 */
#define init_pks_value (PKRU_AD_KEY(1) | PKRU_AD_KEY(2) | PKRU_AD_KEY(3) |    \
			PKRU_AD_KEY(4) | PKRU_AD_KEY(5) | PKRU_AD_KEY(6) |    \
			PKRU_AD_KEY(7) | PKRU_AD_KEY(8) | PKRU_AD_KEY(9) |    \
			PKRU_AD_KEY(10) | PKRU_AD_KEY(11) | PKRU_AD_KEY(12) | \
			PKRU_AD_KEY(13) | PKRU_AD_KEY(14) | PKRU_AD_KEY(15))

#endif /* CONFIG_X86_INTEL_MEMORY_PROTECTION_KEYS */

#endif /*_ASM_X86_PKS_PKEYS_H */
