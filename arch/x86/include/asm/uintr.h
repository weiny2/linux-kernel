/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef _ASM_X86_UINTR_H
#define _ASM_X86_UINTR_H

bool uintr_arch_enabled(void);
void uintr_switch(struct task_struct *prev, struct task_struct *next);
void uintr_free(struct task_struct *task);

int uintr_alloc_uvec(u64 handler);
void uintr_free_uvec(struct task_struct *task, int uvec);
void uintr_set_uvec(struct task_struct *task, int uvec_no);

int uintr_register_sender(struct task_struct *r_task, int uvec_no);
void uintr_unregister_sender(struct task_struct *task, unsigned int entry);
void uintr_notify_receiver_exit(struct task_struct *task, unsigned int entry);

#endif /* _ASM_X86_UINTR_H */
