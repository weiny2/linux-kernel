/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_CET_H
#define _ASM_X86_CET_H

#ifndef __ASSEMBLY__
#include <linux/types.h>

struct task_struct;
struct sc_ext;

/*
 * Per-thread CET status
 */
struct cet_status {
	unsigned long	shstk_base;
	unsigned long	shstk_size;
	unsigned int	locked:1;
	unsigned int	shstk_enabled:1;
};

#ifdef CONFIG_X86_INTEL_CET
int prctl_cet(int option, unsigned long arg2);
int cet_setup_shstk(void);
int cet_setup_thread_shstk(struct task_struct *p);
int cet_alloc_shstk(unsigned long *arg);
void cet_disable_free_shstk(struct task_struct *p);
int cet_restore_signal(bool ia32, struct sc_ext *sc);
int cet_setup_signal(bool ia32, unsigned long rstor, struct sc_ext *sc);
#else
static inline int prctl_cet(int option, unsigned long arg2) { return -EINVAL; }
static inline int cet_setup_thread_shstk(struct task_struct *p) { return 0; }
static inline void cet_disable_free_shstk(struct task_struct *p) {}
static inline int cet_restore_signal(bool ia32, struct sc_ext *sc) { return -EINVAL; }
static inline int cet_setup_signal(bool ia32, unsigned long rstor,
				   struct sc_ext *sc) { return -EINVAL; }
#endif

#define cpu_x86_cet_enabled() \
	(static_cpu_has(X86_FEATURE_SHSTK) || \
	 static_cpu_has(X86_FEATURE_IBT))

#endif /* __ASSEMBLY__ */

#endif /* _ASM_X86_CET_H */
