/*
 * Copyright (c) 2014 Intel Corporation.  All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/kernel.h>
#include "hfi.h"
#include "debugfs.h"

#ifdef CONFIG_DEBUG_FS
static struct dentry *hfi_dbg_root;

#define DEBUGFS_SEQ_FILE(name) \
static const struct seq_operations _##name##_seq_ops = { \
	.start = _##name##_seq_start, \
	.next  = _##name##_seq_next, \
	.stop  = _##name##_seq_stop, \
	.show  = _##name##_seq_show \
}; \
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
} \
static const struct file_operations _##name##_file_ops = { \
	.owner   = THIS_MODULE, \
	.open    = _##name##_open, \
	.read    = seq_read, \
	.llseek  = seq_lseek, \
	.release = seq_release \
}

#define DEBUGFS_FILE_CREATE(name, parent, data, ops) \
do { \
	struct dentry *ent; \
	ent = debugfs_create_file(name , 0400, parent, \
		data, ops); \
	if (!ent) \
		pr_warn("create of %s failed\n", name); \
} while (0)


#define DEBUGFS_SEQ_FILE_CREATE(name, parent, data) \
	DEBUGFS_FILE_CREATE(#name, parent, data, &_##name##_file_ops)
#endif

void hfi_dbg_init(struct hfi_devdata *dd)
{
#ifdef CONFIG_DEBUG_FS
	hfi_dbg_root = debugfs_create_dir(DRIVER_NAME, NULL);
	if (!hfi_dbg_root)
		pr_warn("init of debugfs failed\n");
#endif
}

void hfi_dbg_exit(void)
{
#ifdef CONFIG_DEBUG_FS
	debugfs_remove_recursive(hfi_dbg_root);
	hfi_dbg_root = NULL;
#endif
}
