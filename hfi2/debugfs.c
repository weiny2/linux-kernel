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
#include "counters.h"
#include "fxr/mnh_8051_defs.h"
#include "fxr/fxr_fc_defs.h"
#include "fxr/fxr_tx_ci_cic_csrs_defs.h"
#include "fxr/fxr_rx_ci_cid_csrs_defs.h"
#include <fxr/fxr_tx_dma_csrs.h>
#include <fxr/fxr_tx_dma_defs.h>
#include <fxr/fxr_rx_dma_csrs.h>
#include <fxr/fxr_rx_dma_defs.h>
#include <fxr/fxr_tx_otr_pkt_top_csrs.h>
#include <fxr/fxr_tx_otr_pkt_top_csrs_defs.h>

#ifdef CONFIG_DEBUG_FS
struct dentry *hfi_dbg_root;

static ssize_t portcntrnames_read(struct file *file, char __user *buf,
				  size_t count, loff_t *ppos)
{
	char *names;
	size_t avail;
	struct hfi_devdata *dd;
	ssize_t rval;

	rcu_read_lock();
	dd = private2dd(file);
	avail = hfi_read_portcntrs(dd->pport, &names, NULL);
	rval = simple_read_from_buffer(buf, count, ppos, names, avail);
	rcu_read_unlock();
	return rval;
}

static ssize_t portcntrs_read(struct file *file, char __user *buf,
					size_t count, loff_t *ppos)
{
	u64 *counters;
	size_t avail;
	struct hfi_pportdata *ppd;
	size_t rval;

	rcu_read_lock();
	ppd = private2ppd(file);
	avail = hfi_read_portcntrs(ppd, NULL, &counters);
	rval = simple_read_from_buffer(buf, count, ppos, counters, avail);
	rcu_read_unlock();
	return rval;
}

static const struct counter_info cntr_ops[] = {
	DEBUGFS_OPS("portcounter_names", portcntrnames_read, NULL),
};

static const struct counter_info port_cntr_ops[] = {
	DEBUGFS_OPS("counters", portcntrs_read, NULL),
};

static int hfi_qos_show(struct seq_file *s, void *unused)
{
	struct hfi_pportdata *ppd = s->private;
	int i = 0, j;

	seq_printf(s, "QoS mappings for port number %d\n", ppd->pnum);
	for (i = 0; i < OPA_MAX_SCS; i++) {
		seq_printf(s,
			   "sl2mc[%2d] %2d sl2tc[%2d] %2d sl2sc[%2d] %2d ",
			   i, ppd->sl_to_mctc[i] >> 2,
			   i, ppd->sl_to_mctc[i] & HFI_TC_MASK,
			   i, ppd->sl_to_sc[i]);
		seq_printf(s, "sl2mctc[%2d] %2d ", i, ppd->sl_to_mctc[i]);
		seq_printf(s,
			   "sc2vlr[%2d] %2d sc2vlt[%2d] %2d sc2sl[%2d] %2d ",
			   i, ppd->sc_to_vlr[i],
			   i, ppd->sc_to_vlt[i],
			   i, ppd->sc_to_sl[i]);
		seq_printf(s,
			   "sc2respsl[%2d] %2d sc2mc[%2d] %2d sc2tc[%2d] %2d ",
			   i, ppd->sc_to_resp_sl[i],
			   i, ppd->sc_to_mctc[i] >> 2,
			   i, ppd->sc_to_mctc[i] & HFI_TC_MASK);
		seq_printf(s, "sc2vlnt[%2d] %2d\n", i, ppd->sc_to_vlnt[i]);
	}

	for (i = 0, j = 0; i < ARRAY_SIZE(ppd->sl_pairs); i++) {
		if (!hfi_is_portals_req_sl(ppd, i))
			continue;

		seq_printf(s, "ptl_sl_pair %d sl1 %d sl2 %d\n",
			   j++, i, ppd->sl_pairs[i]);
	}

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

static int hfi_bw_arb_show(struct seq_file *s, void *unused)
{
	struct hfi_pportdata *ppd = s->private;
	struct opa_bw_element *bw;
	int i;

	bw = (struct opa_bw_element *)&ppd->bw_arb_cache.bw_group;

	seq_printf(s, "Bandwidth arbitration info for port %d\n", ppd->pnum);
	for (i = 0; i < OPA_NUM_BW_GROUP_SUPPORTED; i++) {
		seq_printf(s, "group[%1d] prio:%1u vl_mask:0x%08x perc:%u\n",
			   i, bw->priority, be32_to_cpu(bw->vl_mask),
			   bw->bw_percentage);
		bw++;
	}
	seq_puts(s, "\nHW programmed values:\n");

	for (i = 0; i < (HFI_MAX_TC * HFI_MAX_MC); i++) {
		TXDMA_CFG_BWMETER_MC0TC0_t txcfg;
		RXDMA_CFG_BW_SHAPE_B0_t rxcfg;
		TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC0TC0_t pcfg;
		TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC0TC0_t fcfg;
		TXOTR_PKT_CFG_OPB_FIFO_ARB_MC0TC0_t acfg;
		int off = 8 * i;

		txcfg.val = read_csr(ppd->dd,
				 FXR_TXDMA_CFG_BWMETER_MC0TC0 + off);
		rxcfg.val = read_csr(ppd->dd,
				 FXR_RXDMA_CFG_BW_SHAPE_B0 + off);
		pcfg.val = read_csr(ppd->dd,
				FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC0TC0 + off);
		fcfg.val = read_csr(ppd->dd,
				FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC0TC0 + off);
		acfg.val = read_csr(ppd->dd,
				FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_MC0TC0 + off);

		seq_printf(s, "MC%1dTC%1d:\n", i / HFI_MAX_TC, i % HFI_MAX_TC);
		seq_printf(s, "\tTXDMA limit:   0x%04x\n",
			   txcfg.field.bandwidth_limit);
		seq_printf(s, "\tTXDMA whole:   0x%04x\n",
			   txcfg.field.leak_amount_integral);
		seq_printf(s, "\tTXDMA frac:    0x%04x\n",
			   txcfg.field.leak_amount_fractional);
		seq_printf(s, "\tTXDMA cap?:    %s\n",
			   txcfg.field.enable_capping ? "  true" : " false");

		seq_printf(s, "\tTXDMA limit:   0x%04x\n",
			   rxcfg.field.BW_LIMIT);
		seq_printf(s, "\tRXDMA whole:   0x%04x\n",
			   rxcfg.field.LEAK_INTEGER);
		seq_printf(s, "\tRXDMA frac:    0x%04x\n",
			   rxcfg.field.LEAK_FRACTION);
		seq_printf(s, "\tRXDMA config:  0x%04x\n",
			   rxcfg.field.METER_CONFIG);

		seq_printf(s, "\tARB_PF limit:  0x%04x\n",
			   pcfg.field.BW_LIMIT);
		seq_printf(s, "\tARB_PF whole:  0x%04x\n",
			   pcfg.field.LEAK_INTEGER);
		seq_printf(s, "\tARB_PF frac:   0x%04x\n",
			   pcfg.field.LEAK_FRACTION);

		seq_printf(s, "\tARB_FP limit:  0x%04x\n",
			   fcfg.field.BW_LIMIT);
		seq_printf(s, "\tARB_FP whole:  0x%04x\n",
			   fcfg.field.LEAK_INTEGER);
		seq_printf(s, "\tARB_FP frac:   0x%04x\n",
			   fcfg.field.LEAK_FRACTION);

		seq_printf(s, "\tARB limit:     0x%04x\n",
			   acfg.field.BW_LIMIT);
		seq_printf(s, "\tARB whole:     0x%04x\n",
			   acfg.field.LEAK_INTEGER);
		seq_printf(s, "\tARB frac:      0x%04x\n",
			   acfg.field.LEAK_FRACTION);

		seq_puts(s, "\n");
	}

	return 0;
}

static int hfi_bw_arb_open(struct inode *inode, struct file *file)
{
	return single_open(file, hfi_bw_arb_show, inode->i_private);
}

static int hfi_bw_arb_release(struct inode *inode, struct file *file)
{
	return single_release(inode, file);
}

static const struct file_operations hfi_bw_arb_ops = {
	.owner   = THIS_MODULE,
	.open    = hfi_bw_arb_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = hfi_bw_arb_release
};

void hfi_bw_arb_dbg_init(struct hfi_devdata *dd)
{
	struct hfi_pportdata *ppd;
	int j;

	/* create bw_arb file for each port */
	for (ppd = dd->pport, j = 0; j < dd->num_pports; j++, ppd++)
		debugfs_create_file("bw_arb", 0444, ppd->hfi_port_dbg,
				    ppd, &hfi_bw_arb_ops);
}

static ssize_t host_link_bounce(struct file *file, const char __user *ubuf,
				size_t count, loff_t *ppos)
{
	struct hfi_pportdata *ppd;

	ppd = private2ppd(file);

	queue_work(ppd->hfi_wq, &ppd->link_bounce_work);

	return count;
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
	DEBUGFS_OPS("bounce_link", NULL, host_link_bounce),
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

void hfi_portcntrs_dbg_init(struct hfi_devdata *dd)
{
	struct hfi_pportdata *ppd;
	int i, j;

	/* create files for each port */
	for (ppd = dd->pport, j = 0; j < dd->num_pports; j++, ppd++) {
		for (i = 0; i < ARRAY_SIZE(port_cntr_ops); i++) {
			DEBUGFS_FILE_CREATE(port_cntr_ops[i].name,
				ppd->hfi_port_dbg,
				ppd,
				&port_cntr_ops[i].ops,
				!port_cntr_ops[i].ops.write ?
					S_IRUGO : S_IRUGO | S_IWUSR);
		}
	}
}

void hfi_devcntrs_dbg_init(struct hfi_devdata *dd)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(cntr_ops); i++)
		DEBUGFS_FILE_CREATE(cntr_ops[i].name,
				    dd->hfi_dev_dbg,
				    dd,
				    &cntr_ops[i].ops, S_IRUGO);
}

static int hfi_update_ctrl_show(struct seq_file *s, u32 reg)
{
	struct hfi_devdata *dd = s->private;
	u64 val;

	val = read_csr(dd, reg);
	seq_printf(s, "%llu\n", val);
	return 0;
}

static ssize_t hfi_update_ctrl_write(struct file *f, const char __user *ubuf,
				     size_t count, loff_t *ppos, u32 reg,
				     u8 max)
{
	struct hfi_devdata *dd = private2dd(f);
	u64 val = 0;
	int ret = 0;

	ret = kstrtou64_from_user(ubuf, count, 0, &val);
	if (ret < 0)
		goto err;
	/* Sanitize update freuquency */
	if (val > max)
		goto err;
	write_csr(dd, reg, val);
	return count;
err:
	dd_dev_err(dd, "Invalid cq_head_update_freq, must be uint <= %d", max);
	return -EINVAL;
}

static int hfi_tx_update_ctrl_show(struct seq_file *s, void *unused)
{
	return hfi_update_ctrl_show(s, FXR_TXCIC_CFG_HEAD_UPDATE_CNTRL);
}

static int hfi_tx_update_ctrl_open(struct inode *inode, struct file *file)
{
	return single_open(file, hfi_tx_update_ctrl_show, inode->i_private);
}

static ssize_t hfi_tx_update_ctrl_write(struct file *f, const char __user *ubuf,
					size_t count, loff_t *ppos)
{
	/* See hfi_init_rate_control() for details */
	return hfi_update_ctrl_write(f, ubuf, count, ppos,
				     FXR_TXCIC_CFG_HEAD_UPDATE_CNTRL, 5);
}

static const struct file_operations hfi_tx_update_ctrl_ops = {
	.owner   = THIS_MODULE,
	.open    = hfi_tx_update_ctrl_open,
	.read    = seq_read,
	.write   = hfi_tx_update_ctrl_write,
	.llseek  = seq_lseek,
	.release = single_release
};

static int hfi_rx_update_ctrl_show(struct seq_file *s, void *unused)
{
	return hfi_update_ctrl_show(s, FXR_RXCID_CFG_HEAD_UPDATE_CNTRL);
}

static int hfi_rx_update_ctrl_open(struct inode *inode, struct file *file)
{
	return single_open(file, hfi_rx_update_ctrl_show, inode->i_private);
}

static ssize_t hfi_rx_update_ctrl_write(struct file *f, const char __user *ubuf,
					size_t count, loff_t *ppos)
{
	/* See hfi_init_rate_control() for details */
	return hfi_update_ctrl_write(f, ubuf, count, ppos,
				     FXR_RXCID_CFG_HEAD_UPDATE_CNTRL, 3);
}

static const struct file_operations hfi_rx_update_ctrl_ops = {
	.owner   = THIS_MODULE,
	.open    = hfi_rx_update_ctrl_open,
	.read    = seq_read,
	.write   = hfi_rx_update_ctrl_write,
	.llseek  = seq_lseek,
	.release = single_release
};

static void hfi_head_update_ctrl_dbg_init(struct hfi_devdata *dd)
{
	DEBUGFS_FILE_CREATE("tx_cq_head_update_freq",
			    dd->hfi_dev_dbg, dd,
			    &hfi_tx_update_ctrl_ops,
			    S_IRUGO | S_IWUSR);

	DEBUGFS_FILE_CREATE("rx_cq_head_update_freq",
			    dd->hfi_dev_dbg, dd,
			    &hfi_rx_update_ctrl_ops,
			    S_IRUGO | S_IWUSR);
}

void hfi_dbg_init(struct hfi_devdata *dd)
{
	struct hfi_pportdata *ppd;
	char name[32], link[10];
	int unit = dd->unit, j;

	/* create /sys/kernel/debug/hfi2 */
	hfi_dbg_root = debugfs_create_dir(DRIVER_NAME, NULL);
	if (!hfi_dbg_root)
		pr_warn("can't create %s\n", DRIVER_NAME);

	/* create /sys/kernel/debug/hfi2/hfiN and .../N */
	snprintf(name, sizeof(name), "%s%d", hfi_class_name(), unit);
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
	hfi_bw_arb_dbg_init(dd);
	hfi_firmware_dbg_init(dd);
	hfi_devcntrs_dbg_init(dd);
	hfi_portcntrs_dbg_init(dd);
	hfi_head_update_ctrl_dbg_init(dd);
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
