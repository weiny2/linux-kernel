/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_PKEYS_COMMON_H
#define _ASM_X86_PKEYS_COMMON_H

#define PKR_AD_BIT 0x1u
#define PKR_WD_BIT 0x2u
#define PKR_BITS_PER_PKEY 2

#define PKR_AD_MASK(pkey)	(PKR_AD_BIT << ((pkey) * PKR_BITS_PER_PKEY))

#endif /*_ASM_X86_PKEYS_COMMON_H */
