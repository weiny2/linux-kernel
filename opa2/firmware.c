/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2014 - 2015 Intel Corporation.
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
 * Copyright (c) 2014 - 2015 Intel Corporation.
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
 *
 * Intel(R) Omni-Path Gen2 HFI PCIe Driver
 */

#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include "opa_hfi.h"
#include "debugfs.h"
#include "link.h"
#include "rdma/fxr/dc_8051_csrs_defs.h"
#include "rdma/fxr/dcc_csrs_defs.h"

#ifdef CONFIG_DEBUG_FS

#define FIRMWARE_READ(name, address) \
static ssize_t _##name##_read(struct file *file, char __user *buf,\
			      size_t count, loff_t *ppos)\
{ \
	struct hfi_pportdata *ppd;\
	ssize_t ret = 0;\
	u64 state;\
	ppd = private2ppd(file);\
	state = read_8051_csr(ppd, address);\
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
	u64 port_config;\
	rcu_read_lock();\
	ppd = private2ppd(file);\
	ret = kstrtou64_from_user(ubuf, count, 0, &port_config);\
	if (ret < 0)\
		goto _return;\
	write_8051_csr(ppd, address, port_config);\
	ret = count;\
 _return:\
	rcu_read_unlock();\
	return ret;\
}

static const char *hfi_class_name = "hfi";

#define private2dd(file) (file_inode(file)->i_private)
#define private2ppd(file) (file_inode(file)->i_private)

FIRMWARE_READ(8051_state, CRK8051_STS_CUR_STATE)
FIRMWARE_READ(8051_access, CRK8051_CFG_CSR_ACCESS_SEL)
FIRMWARE_WRITE(8051_access, CRK8051_CFG_CSR_ACCESS_SEL)
FIRMWARE_READ(8051_cmd0, CRK8051_CFG_HOST_CMD_0)
FIRMWARE_WRITE(8051_cmd0, CRK8051_CFG_HOST_CMD_0)
FIRMWARE_READ(8051_cmd1, CRK8051_CFG_HOST_CMD_1)
FIRMWARE_READ(port_config, CRK_CFG_PORT_CONFIG)
FIRMWARE_WRITE(port_config, CRK_CFG_PORT_CONFIG)

struct firmware_info {
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

static const struct firmware_info firmware_ops[] = {
	DEBUGFS_OPS("8051_state", _8051_state_read, NULL),
	DEBUGFS_OPS("8051_access", _8051_access_read, _8051_access_write),
	DEBUGFS_OPS("8051_cmd0", _8051_cmd0_read, _8051_cmd0_write),
	DEBUGFS_OPS("8051_cmd1", _8051_cmd1_read, NULL),
	DEBUGFS_OPS("port_config", _port_config_read, _port_config_write),
};

void hfi_firmware_dbg_init(struct hfi_devdata *dd)
{
	char name[32], link[10];
	struct hfi_pportdata *ppd;
	int unit = dd->unit;
	int i, j;

	snprintf(name, sizeof(name), "%s%d", hfi_class_name, unit);
	snprintf(link, sizeof(link), "%d", unit);
	dd->hfi_dev_dbg = debugfs_create_dir(name, hfi_dbg_root);
	if (!dd->hfi_dev_dbg) {
		pr_warn("can't create debugfs directory: %s %p\n", name,
			dd->hfi_dev_dbg);
		return;
	}
	dd->hfi_dev_link =
		debugfs_create_symlink(link, hfi_dbg_root, name);
	if (!dd->hfi_dev_link) {
		pr_warn("can't create symlink: %s\n", name);
		return;
	}
	/* create files for each port */
	for (ppd = dd->pport, j = 0; j < dd->num_pports; j++, ppd++) {
		snprintf(name, sizeof(name), "port%d", ppd->pnum);
		ppd->hfi_port_dbg = debugfs_create_dir(name, dd->hfi_dev_dbg);
		for (i = 0; i < ARRAY_SIZE(firmware_ops); i++) {
			DEBUGFS_FILE_CREATE(firmware_ops[i].name,
				ppd->hfi_port_dbg,
				ppd,
				&firmware_ops[i].ops,
				firmware_ops[i].ops.write == NULL ?
					S_IRUGO : S_IRUGO|S_IWUSR);
		}
	}
}

void hfi_firmware_dbg_exit(struct hfi_devdata *dd)
{
	dd->hfi_dev_dbg = NULL;
	/* debugfs_remove_recursive() in debugfs.c frees dd->hfi_dev_dbg */
}
#endif

/* return the 8051 firmware state */
static inline u32 get_firmware_state(const struct hfi_pportdata *ppd)
{
	u64 reg = read_8051_csr(ppd, CRK8051_STS_CUR_STATE);

	return (reg >> CRK8051_STS_CUR_STATE_FIRMWARE_SHIFT)
				& CRK8051_STS_CUR_STATE_FIRMWARE_MASK;
}

/*
 * Wait until the firmware is up and ready to take host requests.
 * Return 0 on success, -ETIMEDOUT on timeout.
 */
int hfi_wait_firmware_ready(const struct hfi_pportdata *ppd, u32 mstimeout)
{
	unsigned long timeout;

#if 0
	/* in the simulator, the fake 8051 is always ready */
	if (dd->icode == ICODE_FUNCTIONAL_SIMULATOR)
		return 0;
#endif

	timeout = msecs_to_jiffies(mstimeout) + jiffies;
	while (1) {
		if (get_firmware_state(ppd) == 0xa0)	/* ready */
			return 0;
		if (time_after(jiffies, timeout))	/* timed out */
			return -ETIMEDOUT;
		usleep_range(1950, 2050); /* sleep 2ms-ish */
	}
}
