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

#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/kernel.h>
#include "opa_hfi.h"
#include "debugfs.h"
#include "firmware.h"
#include "link.h"
#include "rdma/fxr/mnh_8051_defs.h"
#include "rdma/fxr/fxr_fc_defs.h"

#ifdef CONFIG_DEBUG_FS
struct dentry *hfi_dbg_root;
static const char *hfi_class_name = "hfi";

static int hfi_qos_show(struct seq_file *s, void *unused)
{
	struct hfi_pportdata *ppd = s->private;
	int i = 0;

	seq_printf(s, "QoS mappings for port number %d\n", ppd->pnum);
	for (i = 0; i < OPA_MAX_SCS; i++) {
		seq_printf(s,
			   "sl2mc[%2d] %2d sl2tc[%2d] %2lld sl2sc[%2d] %2d ",
			   i, ppd->sl_to_mctc[i] >> 2,
			   i, ppd->sl_to_mctc[i] &
			   FXR_TXCID_CFG_SL0_TO_TC_SL0_P0_TC_MASK,
			   i, ppd->sl_to_sc[i]);
		seq_printf(s,
			   "sc2vlr[%2d] %2d sc2vlt[%2d] %2d sc2sl[%2d] %2d ",
			   i, ppd->sc_to_vlr[i],
			   i, ppd->sc_to_vlt[i],
			   i, ppd->sc_to_sl[i]);
		seq_printf(s,
			   "sc2respsl[%2d] %2d sc2mc[%2d] %2d sc2tc[%2d] %2d ",
			   i, ppd->sc_to_resp_sl[i],
			   i, ppd->sc_to_mctc[i] >> 2,
			   i, ppd->sc_to_mctc[i] & 0x3);
		seq_printf(s, "sc2vlnt[%2d] %2d\n", i, ppd->sc_to_vlnt[i]);
	}

	for (i = 0; i < ppd->num_ptl_slp; i++)
		seq_printf(s, "ptl_sl_pair %d sl1 %d sl2 %d\n",
			   i, ppd->ptl_slp[i][0], ppd->ptl_slp[i][1]);

	return 0;
}

static int hfi_qos_open(struct inode *inode, struct file *file)
{
	return single_open(file, hfi_qos_show, inode->i_private);
}

static int hfi_qos_release(struct inode *inode, struct file *file)
{
	return single_release(inode, file);
}

static const struct file_operations hfi_qos_ops = {
	.owner   = THIS_MODULE,
	.open    = hfi_qos_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = hfi_qos_release
};


void hfi_qos_dbg_init(struct hfi_devdata *dd)
{
	struct hfi_pportdata *ppd;
	int j;

	/* create qos file for each port */
	for (ppd = dd->pport, j = 0; j < dd->num_pports; j++, ppd++)
		debugfs_create_file("qos", 0444, ppd->hfi_port_dbg,
				    ppd, &hfi_qos_ops);
}

/* link negotiation and initialization */
FIRMWARE_READ(8051_state, 8051, CRK_CRK8051_STS_CUR_STATE)
FIRMWARE_READ(8051_cmd0, 8051, CRK_CRK8051_CFG_HOST_CMD_0)
FIRMWARE_WRITE(8051_cmd0, 8051, CRK_CRK8051_CFG_HOST_CMD_0)
FIRMWARE_READ(8051_cmd1, 8051, CRK_CRK8051_CFG_HOST_CMD_1)
FIRMWARE_READ(logical_link, fzc, FZC_LCB_CFG_PORT)
FIRMWARE_WRITE(logical_link, fzc, FZC_LCB_CFG_PORT)
HOST_STATE_READ(host_link_state)
HOST_STATE_READ(lstate)
LINK_WIDTH_READ(local, VERIFY_CAP_LOCAL_LINK_WIDTH)
LINK_WIDTH_READ(remote, VERIFY_CAP_REMOTE_LINK_WIDTH)
static const struct firmware_info firmware_ops[] = {
	DEBUGFS_OPS("8051_state", _8051_state_read, NULL),
	DEBUGFS_OPS("8051_cmd0", _8051_cmd0_read, _8051_cmd0_write),
	DEBUGFS_OPS("8051_cmd1", _8051_cmd1_read, NULL),
	DEBUGFS_OPS("logical_link", _logical_link_read, _logical_link_write),
	DEBUGFS_OPS("host_link_state", host_link_state_read, NULL),
	DEBUGFS_OPS("lstate", lstate_read, NULL),
	DEBUGFS_OPS("local_link_width", local_link_width_show, NULL),
	DEBUGFS_OPS("remote_link_width", remote_link_width_show, NULL),
};

void hfi_firmware_dbg_init(struct hfi_devdata *dd)
{
	struct hfi_pportdata *ppd;
	int i, j;

	/* create files for each port */
	for (ppd = dd->pport, j = 0; j < dd->num_pports; j++, ppd++) {
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

void hfi_dbg_init(struct hfi_devdata *dd)
{
	struct hfi_pportdata *ppd;
	char name[32], link[10];
	int unit = dd->unit, j;

	/* create /sys/kernel/debug/opa2_hfi */
	hfi_dbg_root = debugfs_create_dir(DRIVER_NAME, NULL);
	if (!hfi_dbg_root)
		pr_warn("can't create %s\n", DRIVER_NAME);

	/* create /sys/kernel/debug/opa2_hfi/hfiN and .../N */
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

	/* create a directory for each port */
	for (j = 0, ppd = dd->pport; j < dd->num_pports; ppd++, j++) {
		snprintf(name, sizeof(name), "port%d", ppd->pnum);
		ppd->hfi_port_dbg = debugfs_create_dir(name, dd->hfi_dev_dbg);
	}

	hfi_qos_dbg_init(dd);
	hfi_firmware_dbg_init(dd);
}

void hfi_dbg_exit(struct hfi_devdata *dd)
{
	debugfs_remove_recursive(hfi_dbg_root);
	dd->hfi_dev_dbg = NULL;
	hfi_dbg_root = NULL;
}
#else
void hfi_dbg_init(struct hfi_devdata *dd) {}
void hfi_dbg_exit(struct hfi_devdata *dd) {}
#endif
