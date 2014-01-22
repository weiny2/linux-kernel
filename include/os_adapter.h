/*
 * Copyright (c) 2013 Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>
 */

#ifndef OS_ADAPTER_H_
#define OS_ADAPTER_H_

#include <linux/nvdimm.h>
#include <linux/types.h>

/******************************************************************
 * Kernel specific
 *****************************************************************/
/* #ifdef __KERNEL__ */
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/sort.h>
#include <linux/fs.h>
#include <linux/bio.h>
#include <linux/blkdev.h>
#include <linux/io.h>
#include <linux/mutex.h>

#define NVDIMM_INFO(fmt, ...)	\
	pr_info("%s: " fmt "\n", LEVEL, ## __VA_ARGS__)

#define NVDIMM_CRIT(fmt, ...)	\
	pr_crit("%s :%s:%d: " fmt "\n", LEVEL, __FILE__, \
		__LINE__, ## __VA_ARGS__)

#define NVDIMM_WARN(fmt, ...)	\
	pr_warn("%s :%s:%d: " fmt "\n", LEVEL, __FILE__, \
		__LINE__, ## __VA_ARGS__)

#define NVDIMM_ERR(fmt, ...)	\
	pr_err("%s :%s:%d: " fmt "\n", LEVEL, __FILE__, \
		__LINE__, ## __VA_ARGS__)

#define NVDIMM_DBG(fmt, ...)	\
	pr_debug(fmt "\n", ## __VA_ARGS__)

#define NVDIMM_VDBG(fmt, ...)	\
	pr_debug(fmt "\n", ## __VA_ARGS__)

/*TODO: Implement the if we have architecture that supports pcommit
 * PMFS has a way to check if the ISA exists, we should copy it
 */
/*
static inline void __builtin_ia32_pcommit(void)
{
	asm volatile(".byte 0x66, 0x0f, 0xae, 0xf8");
}
*/
static inline void __builtin_ia32_pcommit(void)
{
	return;
}

#define cr_write_barrier() wmb()

#define CR_PMEMDEV_INITIALIZER(DEV) \
		{	__SPIN_LOCK_UNLOCKED(DEV.volume_lock), \
			__SPIN_LOCK_UNLOCKED(DEV.nvdimm_lock), \
			__SPIN_LOCK_UNLOCKED(DEV.nvm_pool_lock, \
			LIST_HEAD_INIT(DEV.volumes), \
			LIST_HEAD_INIT(DEV.nvdimms), \
			LIST_HEAD_INIT(DEV.nvm_pools), \
			NULL}

/* #else */
/******************************************************************
 * User land (required to build unit tests)
 *****************************************************************/
/*
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <list.h>
#include <errno.h>

#define __LINUX__ 1

typedef unsigned long long phys_addr_t;
typedef unsigned int fmode_t;

#define MAX_ERRNO	4095

#define IS_ERR_VALUE(x) ((x) >= (unsigned long)-MAX_ERRNO)

static inline void *ERR_PTR(long error)
{
	return (void *)error;
}

static inline long PTR_ERR(const void *ptr)
{
	return (long)ptr;
}

static inline long IS_ERR(const void *ptr)
{
	return IS_ERR_VALUE((unsigned long)ptr);
}

static inline void *ERR_CAST(void *ptr)
{
	return (void *)ptr;
}

static inline long IS_ERR_OR_NULL(const void *ptr)
{
	return !ptr || IS_ERR_VALUE((unsigned long)ptr);
}

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define NVDIMM_INFO(fmt, ...)	\
	printf("%s: " fmt "\n", LEVEL, ## __VA_ARGS__)

#define NVDIMM_CRIT(fmt, ...)	\
	printf("%s :%s:%d: " fmt "\n", LEVEL, __FILE__, \
		__LINE__, ## __VA_ARGS__)

#define NVDIMM_WARN(fmt, ...)	\
	printf("%s :%s:%d: " fmt "\n", LEVEL, __FILE__, \
		__LINE__, ## __VA_ARGS__)

#define NVDIMM_ERR(fmt, ...)	\
	printf("%s :%s:%d: " fmt "\n", LEVEL, __FILE__, \
		__LINE__, ## __VA_ARGS__)

#ifdef VERBOSE_DEBUG
#ifndef DEBUG
#define DEBUG
#endif
#define NVDIMM_VDBG(fmt, ...)	\
	printf(fmt "\n", ## __VA_ARGS__)
#else
#define NVDIMM_VDBG(fmt, ...)
#endif

#ifdef DEBUG
#define NVDIMM_DBG(fmt, ...)	\
	printf(fmt "\n", ## __VA_ARGS__)
#else
#define NVDIMM_DBG(fmt, ...)	do { } while(0)
#endif

#define	__packed	 __attribute__((packed))

static inline void __builtin_ia32_pcommit(void)
{
	return;
}
*/
/*Function List*/
/*
#define cr_write_barrier() __builtin_ia32_sfence()
#define cr_get_virt(ADDR,CRDIMM) (void *)ADDR
#define cr_get_phys(ADDR) (phys_addr_t)ADDR
#define memset_io(START, VALUE, SIZE) memset(START, VALUE, SIZE)
#define memcpy_toio(TO, FROM, SIZE) memcpy(TO, FROM, SIZE)
#define memcpy_fromio(TO, FROM, SIZE) memcpy(TO, FROM, SIZE)
#define	kcalloc(n, size, kern_flags) calloc(n, size)
#define	kfree(ptr) free(ptr)
#define kzalloc(size, kern_flags) calloc(1, size)
#define	sort(list, n, size, fn, arg) qsort(list, n, size, fn)

#define DEFINE_SPINLOCK(a) int a;
#define spinlock_t int
#define spin_lock(a) do {*a = 0;} while(0)
#define spin_unlock(a) do {*a = 0;} while(0)

#define le64_to_cpu(DATA) DATA
#define le32_to_cpu(DATA) DATA
#define le16_to_cpu(DATA) DATA

#define cpu_to_le64(DATA) DATA
#define cpu_to_le32(DATA) DATA
#define cpu_to_le16(DATA) DATA

#define readq(a)		*(a)
#define __raw_writeq(v,a)	*(a) = (v)
*/

/*Linux tags that can be defined away for user space*/
/*
#define __init
#define __exit
#define __initdata
#define __iomem
#define __user

struct mutex {
	int a;
};

#define mutex_init(lock) do {(lock)->a = 0;} while(0)
#define mutex_lock(lock) do {(lock)->a = 0;} while(0)
#define mutex_unlock(lock) do {(lock)->a = 0;} while(0)

#define CR_PMEMDEV_INITIALIZER(DEV) \
		{ 0, 0, 0, LIST_HEAD_INIT(DEV.volumes), \
				LIST_HEAD_INIT(DEV.nvdimms), \
				LIST_HEAD_INIT(DEV.nvm_pools), \
					NULL}

#define BUG() do {} while(0)

#define get_dmi_memdev(DIMM) 0

struct gendisk {
	__u64 a;
};

struct request_queue {
	__u64 a;
};

#define NUMA_NO_NODE -1

#define EXPORT_SYMBOL(FUNC);
#define copy_to_user(TO,FROM,SIZE) memcpy(TO,FROM,SIZE)
#define copy_from_user(TO,FROM,SIZE) memcpy(TO,FROM,SIZE)
#define MODULE_DEVICE_TABLE(TYPE, TABLE);
#define THIS_MODULE NULL
#define printk printf
#define KERN_DEBUG
#define KERN_INFO
#define module_init(FUNC);
#define module_exit(FUNC);
#define MODULE_LICENSE(STRING);
#define MODULE_DESCRIPTION(STRING);
#define MODULE_AUTHOR(STRING);

struct dmi_header {
	__u8 type;
	__u8 length;
	__u16 handle;
};

#define DMI_ENTRY_MEM_DEVICE 17
#define DMI_ENTRY_MEM_DEV_MAPPED_ADDR 19

#endif
*/

#endif
