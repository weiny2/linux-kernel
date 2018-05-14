/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2015 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * BSD LICENSE
 *
 * Copyright (c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * Intel(R) Omni-Path Gen2 HFI PCIe Driver
 */

#ifndef _HFI_DEBUGFS_H
#define _HFI_DEBUGFS_H

#include <linux/debugfs.h>

/*
 * 8051 data memory size.
 */
#define DC8051_DATA_MEM_SIZE 0x1000

#define DEBUGFS_SEQ_FILE_OPS(name) \
static const struct seq_operations _##name##_seq_ops = { \
	.start = _##name##_seq_start, \
	.next  = _##name##_seq_next, \
	.stop  = _##name##_seq_stop, \
	.show  = _##name##_seq_show \
}

#define DEBUGFS_SEQ_FILE_OPEN(name) \
static int _##name##_open(struct inode *inode, struct file *s) \
{ \
	struct seq_file *seq; \
	int ret; \
	ret =  seq_open(s, &_##name##_seq_ops); \
	if (ret) \
		return ret; \
	seq = s->private_data; \
	seq->private = inode->i_private; \
	return 0; \
}

#define DEBUGFS_FILE_OPS(name) \
static const struct file_operations _##name##_file_ops = { \
	.owner   = THIS_MODULE, \
	.open    = _##name##_open, \
	.read    = seq_read, \
	.llseek  = seq_lseek, \
	.release = seq_release \
}

#define DEBUGFS_FILE_OPS_SINGLE(name) \
static int hfi_##name##_open(struct inode *inode, struct file *file) \
{ \
	return single_open(file, hfi_##name##_show, inode->i_private); \
} \
static const struct file_operations hfi_##name##_ops = { \
	.owner   = THIS_MODULE, \
	.open    = hfi_##name##_open, \
	.read    = seq_read, \
	.llseek  = seq_lseek, \
	.release = single_release \
}

#define DEBUGFS_FILE_OPS_SINGLE_WITH_WRITE(name) \
static int hfi_##name##_open(struct inode *inode, struct file *file) \
{ \
	return single_open(file, hfi_##name##_show, inode->i_private); \
} \
static const struct file_operations hfi_##name##_ops = { \
	.owner   = THIS_MODULE, \
	.open    = hfi_##name##_open, \
	.read    = seq_read, \
	.write   = hfi_##name##_write, \
	.llseek  = seq_lseek, \
	.release = single_release \
}

#define FIRMWARE_READ(name, address) \
static ssize_t _##name##_read(struct file *file, char __user *buf,\
			      size_t count, loff_t *ppos)\
{ \
	struct hfi_pportdata *ppd;\
	ssize_t ret = 0;\
	u64 state;\
	ppd = private2ppd(file);\
	state = read_csr(ppd->dd, address);\
	ret =  simple_read_from_buffer(buf, count, ppos, &state,\
					sizeof(state));\
	return ret;\
}

#define FIRMWARE_WRITE(name, address) \
static ssize_t _##name##_write(struct file *file, const char __user *ubuf,\
			   size_t count, loff_t *ppos)\
{ \
	struct hfi_pportdata *ppd;\
	int ret = 0;\
	u64 reg;\
	rcu_read_lock();\
	ppd = private2ppd(file);\
	ret = kstrtou64_from_user(ubuf, count, 0, &reg);\
	if (ret < 0)\
		goto _return;\
	write_csr(ppd->dd, address, reg);\
	ret = count;\
 _return:\
	rcu_read_unlock();\
	return ret;\
}

#define private2at(file) (file_inode(file)->i_private)
#define private2dd(file) (file_inode(file)->i_private)
#define private2ppd(file) (file_inode(file)->i_private)

#define HOST_STATE_READ(name) \
static ssize_t name##_read(struct file *file, char __user *buf,\
			      size_t count, loff_t *ppos)\
{\
	struct hfi_pportdata *ppd = private2ppd(file);\
	return (ssize_t)simple_read_from_buffer(buf, count, ppos, &ppd->name, \
		sizeof(ppd->name));\
}

#define LINK_WIDTH_READ(name, register) \
static ssize_t name##_link_width_show(struct file *file, char __user *buf, \
		      size_t count, loff_t *ppos) \
{ \
	struct hfi_pportdata *ppd = private2ppd(file);\
	ssize_t ret;\
	u32 frame;\
\
	hfi2_read_8051_config(ppd, register, GENERAL_CONFIG, \
			 &frame); \
	ret =  simple_read_from_buffer(buf, count, ppos, &frame, \
					sizeof(frame)); \
	return ret; \
}

struct firmware_info {
	char *name;
	const struct file_operations ops;
};

struct counter_info {
	char *name;
	const struct file_operations ops;
};

#define DEBUGFS_OPS(nm, readroutine, writeroutine)	\
{ \
	.name = nm, \
	.ops = { \
		.read = readroutine, \
		.write = writeroutine, \
		.llseek = generic_file_llseek, \
	}, \
}

extern struct dentry *hfi_dbg_root;

void __init hfi_dbg_init(void);
void hfi_dbg_exit(void);

void hfi_dbg_dev_early_init(struct hfi_devdata *dd);
void hfi_dbg_dev_init(struct hfi_devdata *dd);
void hfi_dbg_dev_exit(struct hfi_devdata *dd);

#endif	/* _HFI_DEBUGFS_H */
