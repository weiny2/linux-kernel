/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef _LINUX_UINTR_H
#define _LINUX_UINTR_H

#ifdef CONFIG_ARCH_HAS_UINTR
#include <asm/uintr.h>
#else

static inline int uintr_register_sender(struct task_struct *r_task,
					int uvec_no)
{
	return -EINVAL;
}

static inline int uintr_alloc_uvec(u64 handler)
{
	return -EINVAL;
}

static inline void uintr_set_uvec(struct task_struct *task, int uvec_no)
{
}

static inline void uintr_unregister_uvec(struct task_struct *task,
					 int uvec_no)
{
}

static inline void uintr_unregister_sender(struct task_struct *task,
					   unsigned int entry)
{
}

static inline void uintr_notify_receiver_exit(struct task_struct *task,
					      unsigned int entry)
{
}

static inline void uintr_print_debug_info(void)
{
}

static inline void uintr_switch(struct task_struct *prev,
				struct task_struct *next)
{
}

static inline void uintr_free(struct task_struct *task)
{
}

#endif /* !CONFIG_ARCH_HAS_UINTR */

#endif /* _LINUX_UINTR_H */
