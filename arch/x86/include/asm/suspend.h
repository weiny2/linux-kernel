#ifdef CONFIG_X86_32
# include <asm/suspend_32.h>
#else
# include <asm/suspend_64.h>
#endif

#ifdef CONFIG_HIBERNATE_VERIFICATION
#include <linux/suspend.h>

struct hibernation_keys {
	unsigned long hkey_status;
	u8 hibernation_key[HIBERNATION_DIGEST_SIZE];
};
#endif
