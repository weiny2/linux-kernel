#include "misc.h"

#include <linux/efi.h>
#include <asm/archrandom.h>

#define EDX_TSC		(1 << 4)
#define ECX_RDRAND	(1 << 30)

static unsigned int cpuid_0x1_ecx, cpuid_0x1_edx;

static void cpuid_ecx_edx(void)
{
	unsigned int eax, ebx;

	cpuid(0x1, &eax, &ebx, &cpuid_0x1_ecx, &cpuid_0x1_edx);
}

static unsigned long get_random_long(unsigned long entropy,
				     struct boot_params *boot_params,
				     efi_system_table_t *sys_table)
{
#ifdef CONFIG_X86_64
	const unsigned long mix_const = 0x5d6008cbf3848dd3UL;
#else
	const unsigned long mix_const = 0x3f39e593UL;
#endif
	unsigned long raw, random;
	bool use_i8254 = true;

	if (entropy)
		random = entropy;
	else
		random = get_random_boot(boot_params);

	if (cpuid_0x1_ecx & ECX_RDRAND) {
		if (rdrand_long(&raw)) {
			random ^= raw;
			use_i8254 = false;
		}
	}

	if (cpuid_0x1_edx & EDX_TSC) {
		rdtscll(raw);

		random ^= raw;
		use_i8254 = false;
	}

	if (use_i8254)
		random ^= read_i8254();

	/* Circular multiply for better bit diffusion */
	asm("mul %3"
	    : "=a" (random), "=d" (raw)
	    : "a" (random), "rm" (mix_const));
	random += raw;

	return random;
}

void efi_get_random_key(efi_system_table_t *sys_table,
			struct boot_params *params, u8 key[], unsigned int size)
{
	unsigned long entropy = 0;
	unsigned int bfill = size;

	if (key == NULL || !size)
		return;

	cpuid_ecx_edx();

	memset(key, 0, size);
	while (bfill > 0) {
		unsigned int copy_len = 0;
		entropy = get_random_long(entropy, params, sys_table);
		copy_len = (bfill < sizeof(entropy)) ? bfill : sizeof(entropy);
		memcpy((void *)(key + size - bfill), &entropy, copy_len);
		bfill -= copy_len;
	}
}
