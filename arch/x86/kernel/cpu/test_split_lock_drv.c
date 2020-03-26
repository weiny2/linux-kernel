#include <linux/module.h>
#include <linux/kernel.h>

/* Execute locked btsl instruction with split lock operand. */
int init_module(void)
{
	char cptr[128] __aligned(64);
	int *iptr;

	/*
	 * Change the pointer, making it 3-byte away from the next cache
	 * line.
	 */
	iptr = (int *)(cptr + 61);

	/* Initial value 0 in iptr */
	*iptr = 0;

	printk("split lock test: split lock address=0x%lx\n",
		(unsigned long)iptr);

	/*
	 * The distance between iptr and next cache line is 3 bytes.
	 * Operand size in "btsl" is 4 bytes. So iptr will span two cache
	 * lines. "lock btsl" instruction will trigger #AC in hardware
	 * and kernel will either re-execute the instruction or go to panic
	 * depending on user configuration in
	 * /sys/kernel/debug/x86/split_lock/kernel_mode.
	 */
	asm volatile ("lock btsl $0, %0\n\t"
		      : "=m" (*iptr));

	if (*iptr == 1)
		printk("split lock kernel test passes\n");
	else
		printk("split lock kernel test fails\n");

	return -1;
}

void __exit exit_module(void)
{
}

MODULE_LICENSE("GPL");
