#ifdef CONFIG_X86_32
# include <asm/suspend_32.h>
#else
# include <asm/suspend_64.h>
#endif

#ifdef CONFIG_HIBERNATE_VERIFICATION
#include <linux/suspend.h>

extern void parse_hibernation_keys(u64 phys_addr, u32 data_len);

struct hibernation_keys {
	unsigned long hkey_status;
	u8 hibernation_key[HIBERNATION_DIGEST_SIZE];
};
#else
static inline void parse_hibernation_keys(u64 phys_addr, u32 data_len) {}
#endif
