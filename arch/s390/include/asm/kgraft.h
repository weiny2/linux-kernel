/*
 * kGraft Online Kernel Patching
 *
 *  Copyright (c) 2013-2014 SUSE
 *   Authors: Jiri Kosina
 *	      Vojtech Pavlik
 *	      Jiri Slaby
 */

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#ifndef ASM_KGR_H
#define ASM_KGR_H

#include <asm/ptrace.h>
#include <linux/stacktrace.h>

static inline void kgr_set_regs_ip(struct pt_regs *regs, unsigned long ip)
{
	regs->psw.addr = ip;
}

static inline bool kgr_needs_lazy_migration(struct task_struct *p)
{
	return true;
}

#endif
