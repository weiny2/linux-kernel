/*
 * kGraft Online Kernel Patching
 *
 *  Copyright (c) 2015 SUSE
 */

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include <linux/sched.h>

asmlinkage void s390_handle_kgraft(void)
{
	kgr_task_safe(current);
}
