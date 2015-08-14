#include "misc.h"

#include <linux/efi.h>
#include <linux/stringify.h>
#include <asm/archrandom.h>
#include <asm/efi.h>

#define EFI_STATUS_STR(_status) \
case EFI_##_status: \
	return "EFI_" __stringify(_status);

static char *efi_status_to_str(efi_status_t status)
{
	switch (status) {
	EFI_STATUS_STR(SUCCESS)
	EFI_STATUS_STR(INVALID_PARAMETER)
	EFI_STATUS_STR(OUT_OF_RESOURCES)
	EFI_STATUS_STR(DEVICE_ERROR)
	EFI_STATUS_STR(WRITE_PROTECTED)
	EFI_STATUS_STR(SECURITY_VIOLATION)
	EFI_STATUS_STR(NOT_FOUND)
	}

	return "";
}

static efi_status_t efi_locate_rng(efi_system_table_t *sys_table,
				   void ***rng_handle)
{
	efi_guid_t rng_proto = EFI_RNG_PROTOCOL_GUID;
	unsigned long size = 0;
	efi_status_t status;

	*rng_handle = NULL;
	status = efi_call_phys5(sys_table->boottime->locate_handle,
				EFI_LOCATE_BY_PROTOCOL,
				&rng_proto, NULL, &size, *rng_handle);

	if (status == EFI_BUFFER_TOO_SMALL) {
		status = efi_call_phys3(sys_table->boottime->allocate_pool,
					EFI_LOADER_DATA,
					size, (void **)rng_handle);

		if (status != EFI_SUCCESS) {
			efi_printk(sys_table, "Failed to alloc mem for rng_handle\n");
			return status;
		}

		status = efi_call_phys5(sys_table->boottime->locate_handle,
					EFI_LOCATE_BY_PROTOCOL, &rng_proto,
					NULL, &size, *rng_handle);
	}

	if (status != EFI_SUCCESS) {
		efi_printk(sys_table, "Failed to locate EFI_RNG_PROTOCOL\n");
		efi_call_phys1(sys_table->boottime->free_pool, *rng_handle);
	}

	return status;
}

static bool efi_rng_supported64(efi_system_table_t *sys_table, void **rng_handle)
{
	efi_rng_protocol_64 *rng = NULL;
	efi_guid_t rng_proto = EFI_RNG_PROTOCOL_GUID;
	u64 *handles = (u64 *)(unsigned long)rng_handle;
	unsigned long size = 0;
	void **algorithmlist = NULL;
	efi_status_t status;

	status = efi_call_phys3(sys_table->boottime->handle_protocol,
				handles[0], &rng_proto, (void **)&rng);
	if (status != EFI_SUCCESS)
		efi_printk(sys_table, "Failed to get EFI_RNG_PROTOCOL handles\n");

	if (status == EFI_SUCCESS && rng) {
		status = efi_call_phys3((void *)rng->get_info, rng,
					&size, algorithmlist);
		return (status == EFI_BUFFER_TOO_SMALL);
	}

	return false;
}

static bool efi_rng_supported(efi_system_table_t *sys_table)
{
	bool supported;
	efi_status_t status;
	void **rng_handle = NULL;

	status = efi_locate_rng(sys_table, &rng_handle);
	if (status != EFI_SUCCESS)
		return false;

	supported = efi_rng_supported64(sys_table, rng_handle);
	efi_call_phys1(sys_table->boottime->free_pool, rng_handle);

	return supported;
}

static unsigned long efi_get_rng64(efi_system_table_t *sys_table,
				   void **rng_handle)
{
	efi_rng_protocol_64 *rng = NULL;
	efi_guid_t rng_proto = EFI_RNG_PROTOCOL_GUID;
	u64 *handles = (u64 *)(unsigned long)rng_handle;
	efi_status_t status;
	unsigned long rng_number;

	status = efi_call_phys3(sys_table->boottime->handle_protocol,
				handles[0], &rng_proto, (void **)&rng);
	if (status != EFI_SUCCESS)
		efi_printk(sys_table, "Failed to get EFI_RNG_PROTOCOL handles\n");

	if (status == EFI_SUCCESS && rng) {
		status = efi_call_phys4((void *)rng->get_rng, rng, NULL,
					sizeof(rng_number), &rng_number);
		if (status != EFI_SUCCESS) {
			efi_printk(sys_table, "Failed to get RNG value: ");
			efi_printk(sys_table, efi_status_to_str(status));
			efi_printk(sys_table, "\n");
		}
	}

	return rng_number;
}

static unsigned long efi_get_rng(efi_system_table_t *sys_table)
{
	unsigned long random = 0;
	efi_status_t status;
	void **rng_handle = NULL;

	status = efi_locate_rng(sys_table, &rng_handle);
	if (status != EFI_SUCCESS)
		return 0;

	random = efi_get_rng64(sys_table, rng_handle);
	efi_call_phys1(sys_table->boottime->free_pool, rng_handle);

	return random;
}

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

	if (efi_rng_supported(sys_table)) {
		raw = efi_get_rng(sys_table);
		if (raw)
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
