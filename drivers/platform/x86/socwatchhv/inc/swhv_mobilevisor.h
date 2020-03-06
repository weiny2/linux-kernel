/* SPDX-License-Identifier: GPL-2.0 AND BSD-3-Clause
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright(c) 2014 - 2019 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * Contact Information:
 * SoC Watch Developer Team <socwatchdevelopers@intel.com>
 * Intel Corporation,
 * 1300 S Mopac Expwy,
 * Austin, TX 78746
 *
 * BSD LICENSE
 *
 * Copyright(c) 2014 - 2019 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SWHV_MOBILEVISOR_H_
#define _SWHV_MOBILEVISOR_H_ 1

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <asm/io.h>
#include <linux/version.h> /* LINUX_VERSION_CODE */

#include "asm/mv/mv_svc_hypercalls.h"

#include "swhv_defines.h"
#include "pw_version.h"
#include "control.h"

#define STM_base_3G 0xE4300000
#define STM_base_LTE 0xE4500000
#define STM_size 0X40
#define STM_TIM0_offset 0x20
#define STM_TIM6_offset 0x38
#define STM_CAP_offset 0x3C

/* static int device_open(struct inode *inode, struct file *file); */
int device_open_i(struct inode *inode, struct file *file);

ssize_t device_read_i(struct file *file, /* see include/linux/fs.h   */
	char __user *buffer, /* buffer to be filled with data */
	size_t length, /* length of the buffer */

	loff_t *offset);

long swhv_configure(struct swhv_driver_interface_msg __user *remote_msg, int local_len);
long swhv_start(void);
long swhv_stop(void);
long swhv_get_cpu_count(u32 __user *remote_args);
long swhv_get_clock(u32 __user *remote_in_args, u64 __user *remote_args);
long swhv_get_topology(u64 __user *remote_args);
long swhv_get_hypervisor_type(u32 __user *remote_args);
int swhv_load_driver_i(void);
void swhv_unload_driver_i(void);
void cleanup_error_i(void);
long swhv_msr_read(u32 __user *remote_in_args, u64 __user *remote_args);
long swhv_collection_poll(void);

#endif /* _SWHV_MOBILEVISOR_H_ */
