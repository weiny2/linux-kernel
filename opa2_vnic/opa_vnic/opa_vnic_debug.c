/*
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright(c) 2015 Intel Corporation.
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
 * BSD LICENSE
 *
 * Copyright(c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Intel Corporation nor the names of its
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
 *
 * Intel(R) Omni-Path VNIC debug interface.
 */

#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/module.h>
#include <linux/opa_vnic.h>

#include "opa_vnic_internal.h"

enum {
	OPA_VNIC_DBG_VESW_ID,
	OPA_VNIC_DBG_PKEY,
	OPA_VNIC_DBG_MODE,
	OPA_VNIC_DBG_UC_DLID,
	OPA_VNIC_DBG_MC_DLID,
	OPA_VNIC_DBG_SC,
	OPA_VNIC_DBG_ENCAP_FORMAT,
	OPA_VNIC_DBG_NUM_ATTR,
};

static struct dentry *opa_vnic_dbg_root;

#define DEBUGFS_SEQ_FILE_OPS(name) \
static const struct seq_operations _##name##_seq_ops = { \
	.start = _##name##_seq_start, \
	.next  = _##name##_seq_next, \
	.stop  = _##name##_seq_stop, \
	.show  = _##name##_seq_show \
}

#define DEBUGFS_SEQ_FILE_OPEN(name) \
static int _##name##_open(struct inode *inode, struct file *file) \
{ \
	struct seq_file *seq; \
	int ret; \
	ret =  seq_open(file, &_##name##_seq_ops); \
	if (ret) \
		return ret; \
	seq = file->private_data; \
	seq->private = inode->i_private; \
	return 0; \
}

#define DEBUGFS_FILE_OPS(name) \
static const struct file_operations _##name##_file_ops = { \
	.owner   = THIS_MODULE, \
	.open    = _##name##_open, \
	.write   = _##name##_write, \
	.read    = seq_read, \
	.llseek  = seq_lseek, \
	.release = seq_release \
}

#define DEBUGFS_FILE_CREATE(name, parent, data, ops, mode)	\
do { \
	struct dentry *ent; \
	ent = debugfs_create_file(name, mode, parent, \
		data, ops); \
	if (!ent) \
		pr_warn("create of %s failed\n", name); \
} while (0)


#define DEBUGFS_SEQ_FILE_CREATE(name, parent, data) \
	DEBUGFS_FILE_CREATE(#name, parent, data, &_##name##_file_ops, \
			    (S_IRUGO | S_IWUSR))

static void *_vport_state_seq_start(struct seq_file *s, loff_t *pos)
__acquires(RCU)
{
	rcu_read_lock();
	if (*pos >= OPA_VNIC_DBG_NUM_ATTR)
		return NULL;
	return pos;
}

static void *_vport_state_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	++*pos;
	if (*pos >= OPA_VNIC_DBG_NUM_ATTR)
		return NULL;
	return pos;
}

static void _vport_state_seq_stop(struct seq_file *s, void *v)
__releases(RCU)
{
	rcu_read_unlock();
}

static int _vport_state_seq_show(struct seq_file *s, void *v)
{
	struct opa_vnic_adapter *adapter =
				(struct opa_vnic_adapter *)s->private;
	struct opa_veswport_info *info = &adapter->info;
	loff_t *spos = v;
	int stat, sz;

	if (v == SEQ_START_TOKEN)
		return 0;

	stat = *spos;
	switch (stat) {
	case OPA_VNIC_DBG_VESW_ID:
		sz = seq_printf(s, "vesw_id (virtual eth switch id): %d\n",
				info->vesw.vesw_id);
		break;
	case OPA_VNIC_DBG_PKEY:
		sz = seq_printf(s, "pkey (partition key): %d\n",
				info->vesw.pkey);
		break;
	case OPA_VNIC_DBG_UC_DLID:
		sz = seq_printf(s, "u_ucast_dlid (unknown ucast dlid): %d\n",
				info->vesw.u_ucast_dlid[0]);
		break;
	case OPA_VNIC_DBG_MC_DLID:
		sz = seq_printf(s, "u_mcast_dlid (unknown mcast dlid): %d\n",
				info->vesw.u_mcast_dlid);
		break;
	case OPA_VNIC_DBG_ENCAP_FORMAT:
		sz = seq_printf(s, "encap_format (0=10B, 1=16B): %d\n",
			info->vesw.encap_format);
		break;
	default:
		return SEQ_SKIP;
	}

	return 0;
}

static ssize_t _vport_state_write(struct file *file, const char __user *buf,
				  size_t count, loff_t *ppos)
{
	struct seq_file *s = (struct seq_file *)file->private_data;
	struct opa_vnic_adapter *adapter =
				(struct opa_vnic_adapter *)s->private;
	struct opa_veswport_info *info = &adapter->info;
	char debug_buf[256];
	ssize_t len;
	u32 value;
	int cnt;

	if (*ppos != 0)
		return 0;

	if (count >= sizeof(debug_buf))
		return -ENOSPC;

	len = simple_write_to_buffer(debug_buf, sizeof(debug_buf) - 1,
				     ppos, buf, count);
	if (len < 0)
		return len;

	debug_buf[len] = '\0';
	/* TODO: range check the values */
	if (strncmp(debug_buf, "vesw_id", 7) == 0) {
		cnt = sscanf(&debug_buf[7], "%d", &value);
		if (cnt == 1)
			info->vesw.vesw_id = value;
	} else if (strncmp(debug_buf, "pkey", 4) == 0) {
		cnt = sscanf(&debug_buf[4], "%d", &value);
		if (cnt == 1)
			info->vesw.pkey = value;
	} else if (strncmp(debug_buf, "u_ucast_dlid", 12) == 0) {
		cnt = sscanf(&debug_buf[12], "%d", &value);
		if (cnt == 1)
			info->vesw.u_ucast_dlid[0] = value;
	} else if (strncmp(debug_buf, "u_mcast_dlid", 12) == 0) {
		cnt = sscanf(&debug_buf[12], "%d", &value);
		if (cnt == 1)
			info->vesw.u_mcast_dlid = value;
	} else if (strncmp(debug_buf, "encap_format", 12) == 0) {
		cnt = sscanf(&debug_buf[12], "%d", &value);
		if (cnt == 1)
			info->vesw.encap_format = value;
	}

	return count;
}

DEBUGFS_SEQ_FILE_OPS(vport_state);
DEBUGFS_SEQ_FILE_OPEN(vport_state)
DEBUGFS_FILE_OPS(vport_state);

void opa_vnic_dbg_vport_init(struct opa_vnic_adapter *adapter)
{
	struct opa_vnic_device *vdev = adapter->vdev;

	if (!opa_vnic_dbg_root)
		return;

	adapter->dentry  = debugfs_create_dir(dev_name(&vdev->dev),
					      opa_vnic_dbg_root);
	if (!adapter->dentry) {
		pr_warn("init of opa vnic debugfs failed\n");
		return;
	}

	DEBUGFS_SEQ_FILE_CREATE(vport_state, adapter->dentry, adapter);
}

void opa_vnic_dbg_vport_exit(struct opa_vnic_adapter *adapter)
{
	debugfs_remove_recursive(adapter->dentry);
}

void opa_vnic_dbg_init(void)
{
	opa_vnic_dbg_root = debugfs_create_dir(opa_vnic_driver_name, NULL);
	if (IS_ERR(opa_vnic_dbg_root))
		opa_vnic_dbg_root = NULL;

	if (!opa_vnic_dbg_root)
		pr_warn("init of opa vnic debugfs failed\n");
}

void opa_vnic_dbg_exit(void)
{
	debugfs_remove_recursive(opa_vnic_dbg_root);
	opa_vnic_dbg_root = NULL;
}
