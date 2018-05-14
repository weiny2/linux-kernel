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
#include "hfi2.h"
#include "debugfs.h"
#include "firmware.h"
#include "link.h"
#include "counters.h"
#include "timesync.h"
#include "verbs/verbs.h"
#include "chip/fxr_8051_defs.h"
#include "chip/fxr_oc_defs.h"
#include "chip/fxr_tx_ci_cic_csrs_defs.h"
#include "chip/fxr_rx_ci_cid_csrs_defs.h"
#include <chip/fxr_tx_dma_csrs.h>
#include <chip/fxr_tx_dma_defs.h>
#include <chip/fxr_rx_dma_csrs.h>
#include <chip/fxr_rx_dma_defs.h>
#include <chip/fxr_tx_otr_pkt_top_csrs.h>
#include <chip/fxr_tx_otr_pkt_top_csrs_defs.h>

#define HFI_TXCFG_BW_LIMIT_SMASK \
	FXR_TXDMA_CFG_BWMETER_MC0TC0_BANDWIDTH_LIMIT_SMASK
#define HFI_TXCFG_INTEGRAL_SMASK \
	FXR_TXDMA_CFG_BWMETER_MC0TC0_LEAK_AMOUNT_INTEGRAL_SMASK
#define HFI_TXCFG_FRACTIONAL_SMASK \
	FXR_TXDMA_CFG_BWMETER_MC0TC0_LEAK_AMOUNT_FRACTIONAL_SMASK
#define HFI_TXCFG_EN_CAPPING_SMASK \
	FXR_TXDMA_CFG_BWMETER_MC0TC0_ENABLE_CAPPING_SMASK
#define HFI_TXCFG_BW_LIMIT_SHIFT \
	FXR_TXDMA_CFG_BWMETER_MC0TC0_BANDWIDTH_LIMIT_SHIFT
#define HFI_TXCFG_INTEGRAL_SHIFT \
	FXR_TXDMA_CFG_BWMETER_MC0TC0_LEAK_AMOUNT_INTEGRAL_SHIFT
#define HFI_TXCFG_FRACTIONAL_SHIFT \
	FXR_TXDMA_CFG_BWMETER_MC0TC0_LEAK_AMOUNT_FRACTIONAL_SHIFT
#define HFI_TXCFG_EN_CAPPING_SHIFT \
	FXR_TXDMA_CFG_BWMETER_MC0TC0_ENABLE_CAPPING_SHIFT

#define HFI_RXCFG_BW_LIMIT_SMASK \
	FXR_RXDMA_CFG_BW_SHAPE_B0_BW_LIMIT_SMASK
#define HFI_RXCFG_LEAK_INTEGER_SMASK \
	FXR_RXDMA_CFG_BW_SHAPE_B0_LEAK_INTEGER_SMASK
#define HFI_RXCFG_LEAK_FRACTION_SMASK \
	FXR_RXDMA_CFG_BW_SHAPE_B0_LEAK_FRACTION_SMASK
#define HFI_RXCFG_METER_CONFIG_SMASK \
	FXR_RXDMA_CFG_BW_SHAPE_B0_METER_CONFIG_SMASK
#define HFI_RXCFG_BW_LIMIT_SHIFT \
	FXR_RXDMA_CFG_BW_SHAPE_B0_BW_LIMIT_SHIFT
#define HFI_RXCFG_LEAK_INTEGER_SHIFT \
	FXR_RXDMA_CFG_BW_SHAPE_B0_LEAK_INTEGER_SHIFT
#define HFI_RXCFG_LEAK_FRACTION_SHIFT \
	FXR_RXDMA_CFG_BW_SHAPE_B0_LEAK_FRACTION_SHIFT
#define HFI_RXCFG_METER_CONFIG_SHIFT \
	FXR_RXDMA_CFG_BW_SHAPE_B0_METER_CONFIG_SHIFT

#define HFI_PCFG_BW_LIMIT_SMASK \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC0TC0_BW_LIMIT_SMASK
#define HFI_PCFG_LEAK_INTEGER_SMASK \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC0TC0_LEAK_INTEGER_SMASK
#define HFI_PCFG_LEAK_FRACTION_SMASK \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC0TC0_LEAK_FRACTION_SMASK
#define HFI_PCFG_BW_LIMIT_SHIFT \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC0TC0_BW_LIMIT_SHIFT
#define HFI_PCFG_LEAK_INTEGER_SHIFT \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC0TC0_LEAK_INTEGER_SHIFT
#define HFI_PCFG_LEAK_FRACTION_SHIFT \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC0TC0_LEAK_FRACTION_SHIFT

#define HFI_FCFG_BW_LIMIT_SMASK \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC0TC0_BW_LIMIT_SMASK
#define HFI_FCFG_LEAK_INTEGER_SMASK \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC0TC0_LEAK_INTEGER_SMASK
#define HFI_FCFG_LEAK_FRACTION_SMASK \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC0TC0_LEAK_FRACTION_SMASK
#define HFI_FCFG_BW_LIMIT_SHIFT \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC0TC0_BW_LIMIT_SHIFT
#define HFI_FCFG_LEAK_INTEGER_SHIFT \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC0TC0_LEAK_INTEGER_SHIFT
#define HFI_FCFG_LEAK_FRACTION_SHIFT \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC0TC0_LEAK_FRACTION_SHIFT

#define HFI_ACFG_BW_LIMIT_SMASK \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_MC0TC0_BW_LIMIT_SMASK
#define HFI_ACFG_LEAK_INTEGER_SMASK \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_MC0TC0_LEAK_INTEGER_SMASK
#define HFI_ACFG_LEAK_FRACTION_SMASK \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_MC0TC0_LEAK_FRACTION_SMASK
#define HFI_ACFG_BW_LIMIT_SHIFT \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_MC0TC0_BW_LIMIT_SHIFT
#define HFI_ACFG_LEAK_INTEGER_SHIFT \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_MC0TC0_LEAK_INTEGER_SHIFT
#define HFI_ACFG_LEAK_FRACTION_SHIFT \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_MC0TC0_LEAK_FRACTION_SHIFT

#ifdef CONFIG_DEBUG_FS
struct dentry *hfi_dbg_root;

static ssize_t portcntrnames_read(struct file *file, char __user *buf,
				  size_t count, loff_t *ppos)
{
	char *names, *p;
	size_t avail;
	struct hfi_devdata *dd;
	ssize_t rval;
	int i;

	dd = private2dd(file);
	avail = dd->portcntrnameslen;
	names = kzalloc(avail, GFP_KERNEL);

	if (!names)
		return -ENOMEM;

	for (p = names, i = 0; i < num_port_cntrs; i++) {
		strcat(p, portcntr_names[i]);
		p += strlen(portcntr_names[i]);
		*p++ = '\n';
	}

	rval = simple_read_from_buffer(buf, count, ppos, names, avail);

	kfree(names);
	return rval;
}

static ssize_t portcntrs_read(struct file *file, char __user *buf,
			      size_t count, loff_t *ppos)
{
	u64 *counters;
	size_t avail;
	struct hfi_pportdata *ppd;
	size_t rval;

	ppd = private2ppd(file);
	avail = hfi_read_portcntrs(ppd, &counters);
	rval = simple_read_from_buffer(buf, count, ppos, counters, avail);
	return rval;
}

static ssize_t devcntrnames_read(struct file *file, char __user *buf,
				 size_t count, loff_t *ppos)
{
	char *names, *p;
	size_t avail;
	struct hfi_devdata *dd;
	ssize_t rval;
	int i;

	dd = private2dd(file);
	avail = dd->devcntrnameslen;
	names = kzalloc(avail, GFP_KERNEL);
	if (!names)
		return -ENOMEM;

	for (p = names, i = 0; i < num_dev_cntrs; i++) {
		strcat(p, devcntr_names[i]);
		p += strlen(devcntr_names[i]);
		*p++ = '\n';
	}

	rval = simple_read_from_buffer(buf, count, ppos, names, avail);

	kfree(names);
	return rval;
}

static ssize_t devcntrs_read(struct file *file, char __user *buf,
			     size_t count, loff_t *ppos)
{
	u64 *counters;
	size_t avail;
	struct hfi_devdata *dd;
	size_t rval;

	dd = private2dd(file);
	avail = hfi_read_devcntrs(dd, &counters);
	rval = simple_read_from_buffer(buf, count, ppos, counters, avail);
	return rval;
}

/* read the dc8051 memory */
static ssize_t dc8051_memory_read(struct file *file, char __user *buf,
				  size_t count, loff_t *ppos)
{
	struct hfi_pportdata *ppd = private2ppd(file);
	ssize_t rval;
	void *tmp;
	loff_t start, end;

	/* the checks below expect the position to be positive */
	if (*ppos < 0)
		return -EINVAL;

	tmp = kzalloc(DC8051_DATA_MEM_SIZE, GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;

	/*
	 * Fill in the requested portion of the temporary buffer from the
	 * 8051 memory.  The 8051 memory read is done in terms of 8 bytes.
	 * Adjust start and end to fit.  Skip reading anything if out of
	 * range.
	 */
	start = *ppos & ~0x7;	/* round down */
	if (start < DC8051_DATA_MEM_SIZE) {
		end = (*ppos + count + 7) & ~0x7; /* round up */
		if (end > DC8051_DATA_MEM_SIZE)
			end = DC8051_DATA_MEM_SIZE;
		rval = hfi_read_8051_data(ppd, start, end - start,
					  (u64 *)(tmp + start));
		if (rval)
			goto done;
	}

	rval = simple_read_from_buffer(buf, count, ppos, tmp,
				       DC8051_DATA_MEM_SIZE);
done:
	kfree(tmp);
	return rval;
}

#ifdef CONFIG_HFI2_STLNP
static ssize_t ts_dbg_failover_reset_write(struct file *file,
					   const char __user *buf,
					   size_t count,
					   loff_t *ppos)
{
	struct hfi_pportdata *ppd;

	ppd = private2ppd(file);
	hfi_ts_dbg_failover_reset(ppd);
	return count;
}

static ssize_t hfi_dbg_failover_reg_write(struct file *f,
					  const char __user *ubuf,
					  size_t count,
					  loff_t *ppos,
					  u8 clkid)
{
	struct hfi_pportdata *ppd = private2dd(f);
	u64 val = 0;
	int ret = 0;

	ret = kstrtou64_from_user(ubuf, count, 0, &val);
	if (ret < 0)
		goto err;
	hfi_ts_dbg_failover_reg(ppd, val, clkid);
	return count;
err:
	return -EINVAL;
}

static ssize_t ts_dbg_control_reg_write(struct file *f,
					const char __user *ubuf,
					size_t count,
					loff_t *ppos)
{
	struct hfi_pportdata *ppd = private2dd(f);
	u64 val = 0;
	int ret = 0;

	ret = kstrtou64_from_user(ubuf, count, 0, &val);
	if (ret < 0)
		goto err;
	hfi_ts_dbg_control_reg(ppd, val);
	return count;
err:
	return -EINVAL;
}

static ssize_t ts_dbg_failover_reg0_write(struct file *f,
					  const char __user *ubuf,
					  size_t count,
					  loff_t *ppos)
{
	return hfi_dbg_failover_reg_write(f, ubuf, count, ppos, 0);
}

static ssize_t ts_dbg_failover_reg1_write(struct file *f,
					  const char __user *ubuf,
					  size_t count,
					  loff_t *ppos)
{
	return hfi_dbg_failover_reg_write(f, ubuf, count, ppos, 1);
}

static ssize_t ts_dbg_failover_reg2_write(struct file *f,
					  const char __user *ubuf,
					  size_t count,
					  loff_t *ppos)
{
	return hfi_dbg_failover_reg_write(f, ubuf, count, ppos, 2);
}

static ssize_t ts_dbg_failover_reg3_write(struct file *f,
					  const char __user *ubuf,
					  size_t count,
					  loff_t *ppos)
{
	return hfi_dbg_failover_reg_write(f, ubuf, count, ppos, 3);
}
#endif

static const struct counter_info cntr_ops[] = {
	DEBUGFS_OPS("portcounter_names", portcntrnames_read, NULL),
	DEBUGFS_OPS("devcounter_names", devcntrnames_read, NULL),
	DEBUGFS_OPS("devcounters", devcntrs_read, NULL)
};

static const struct counter_info port_cntr_ops[] = {
	DEBUGFS_OPS("portcounters", portcntrs_read, NULL),
	DEBUGFS_OPS("dc8051_memory", dc8051_memory_read, NULL),
};

#ifdef CONFIG_HFI2_STLNP
static const struct counter_info ts_ops[] = {
	DEBUGFS_OPS("failover_reset", NULL, ts_dbg_failover_reset_write),
	DEBUGFS_OPS("failover_0", NULL, ts_dbg_failover_reg0_write),
	DEBUGFS_OPS("failover_1", NULL, ts_dbg_failover_reg1_write),
	DEBUGFS_OPS("failover_2", NULL, ts_dbg_failover_reg2_write),
	DEBUGFS_OPS("failover_3", NULL, ts_dbg_failover_reg3_write),
	DEBUGFS_OPS("control", NULL, ts_dbg_control_reg_write),
};
#endif

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
		if (!hfi_is_req_sl(ppd, i))
			continue;

		seq_printf(s, "sl_pair %d sl1 %d sl2 %d\n",
			   j++, i, ppd->sl_pairs[i]);
	}

	return 0;
}

static int hfi_mgmt_allowed_show(struct seq_file *s, void *unused)
{
	struct hfi_pportdata *ppd = s->private;

	seq_printf(s, "%u\n", ppd->mgmt_allowed);

	return 0;
}

static int hfi_fw_auth_bypass_show(struct seq_file *s, void *unused)
{
	struct hfi_pportdata *ppd = s->private;

	seq_printf(s, "%u\n", ppd->neighbor_fm_security);

	return 0;
}

static int hfi_neighbor_node_type_show(struct seq_file *s, void *unused)
{
	struct hfi_pportdata *ppd = s->private;

	seq_printf(s, "%u\n", ppd->neighbor_type & OPA_PI_MASK_NEIGH_NODE_TYPE);

	return 0;
}

static int hfi_led_cfg_show(struct seq_file *s, void *unused)
{
	struct hfi_pportdata *ppd = s->private;
	u64 val;
	bool beaconing, hw_ctrl;

	/* Memory barier to read the correct state of led_override_timer_active
	 * since it's modified in hfi_start_oed_override
	 */
	smp_rmb();
	beaconing = !!atomic_read(&ppd->led_override_timer_active);

	seq_printf(s, "beaconing: %s\n", beaconing ? "active" : "inactive");

	val = read_csr(ppd->dd, OC_LCB_CFG_LED);

	hw_ctrl = (val & (1 << 4)) == 0;
	seq_printf(s, "controlled by: %s\n", hw_ctrl ? "HW" : "SW");
	seq_printf(s, "OC_LCB_CFG_LED: 0x%llx\n", val);

	return 0;
}

#ifdef CONFIG_HFI2_STLNP
static int hfi_e2e_show(struct seq_file *s, void *unused)
{
	struct hfi_pportdata *ppd = s->private;
	u8 tc;

	mutex_lock(&ppd->dd->e2e_lock);

	for (tc = 0; tc < HFI_MAX_TC; tc++) {
		struct hfi_ptcdata *ptc = &ppd->ptc[tc];
		struct idr *cache = &ptc->e2e_tx_state_cache;
		u64 *cache_data;
		u32 key;

		seq_printf(s, "TC %u:\n", tc);
		idr_for_each_entry(cache, cache_data, key) {
			seq_printf(s, "\tdlid 0x%x, slid 0x%x pkey_id %d ",
				   hfi_e2e_key_to_dlid(key),
				   hfi_e2e_cache_data_to_slid(cache_data),
				   hfi_e2e_key_to_pkey_id(key));
			seq_printf(s, "sl %d pkey 0x%x ",
				   hfi_e2e_cache_data_to_sl(cache_data),
				   hfi_e2e_cache_data_to_pkey(cache_data));
			seq_printf(s, "cache_data %p\n", cache_data);
		}
		seq_puts(s, "\n");
	}

	mutex_unlock(&ppd->dd->e2e_lock);
	return 0;
}

DEBUGFS_FILE_OPS_SINGLE(e2e);

static void hfi_e2e_dbg_init(struct hfi_devdata *dd)
{
	struct hfi_pportdata *ppd;
	int j;

	/* create e2e file for each port */
	for (ppd = dd->pport, j = 0; j < dd->num_pports; j++, ppd++)
		debugfs_create_file("e2e", 0440, ppd->hfi_port_dbg,
				    ppd, &hfi_e2e_ops);
}
#endif

static int hfi_bw_arb_show(struct seq_file *s, void *unused)
{
	struct hfi_pportdata *ppd = s->private;
	struct opa_bw_element *bw;
	__be32 *pm;
	int i;

	bw = (struct opa_bw_element *)&ppd->bw_arb_cache.bw_group;
	pm = (__be32 *)&ppd->bw_arb_cache.preempt_matrix;

	seq_printf(s, "Bandwidth arbitration info for port %d\n", ppd->pnum);
	for (i = 0; i < OPA_NUM_BW_GROUP_SUPPORTED; i++) {
		seq_printf(s, "group[%1d] prio:%1u vl_mask:0x%08x perc:%u\n",
			   i, bw->priority, be32_to_cpu(bw->vl_mask),
			   bw->bw_percentage);
		bw++;
	}

	seq_printf(s, "\nPreemption Matrix info for port %d\n", ppd->pnum);
	for (i = 0; i < HFI_NUM_DATA_VLS; i++)
		seq_printf(s, "vl[%2d]: %u\n", i, be32_to_cpu(pm[i]));
	seq_printf(s, "vl[15]: %u\n", be32_to_cpu(pm[15]));

	seq_puts(s, "\nHW programmed values:\n");

	for (i = 0; i < (HFI_MAX_TC * HFI_MAX_MC); i++) {
		u64 txcfg;
		u64 rxcfg;
		u64 pcfg;
		u64 fcfg;
		u64 acfg;
		int off = 8 * i;

		txcfg = read_csr(ppd->dd,
				 FXR_TXDMA_CFG_BWMETER_MC0TC0 + off);
		rxcfg = read_csr(ppd->dd,
				 FXR_RXDMA_CFG_BW_SHAPE_B0 + off);
		pcfg = read_csr(ppd->dd,
				FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_PF_MC0TC0 + off);
		fcfg = read_csr(ppd->dd,
				FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_MC0TC0 + off);
		acfg = read_csr(ppd->dd,
				FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_MC0TC0 + off);

		seq_printf(s, "MC%1dTC%1d:\n", i / HFI_MAX_TC, i % HFI_MAX_TC);
		seq_printf(s, "\tTXDMA limit:   0x%04llx\n",
			   (txcfg & HFI_TXCFG_BW_LIMIT_SMASK) >>
			    HFI_TXCFG_BW_LIMIT_SHIFT);
		seq_printf(s, "\tTXDMA whole:   0x%04llx\n",
			   (txcfg & HFI_TXCFG_INTEGRAL_SMASK) >>
			    HFI_TXCFG_INTEGRAL_SHIFT);
		seq_printf(s, "\tTXDMA frac:    0x%04llx\n",
			   (txcfg & HFI_TXCFG_FRACTIONAL_SMASK) >>
			    HFI_TXCFG_FRACTIONAL_SHIFT);
		seq_printf(s, "\tTXDMA cap?:    %s\n",
			   ((txcfg & HFI_TXCFG_EN_CAPPING_SMASK) >>
			   HFI_TXCFG_EN_CAPPING_SHIFT) ?
			   "  true" : " false");

		seq_printf(s, "\tTXDMA limit:   0x%04llx\n",
			   (rxcfg & HFI_RXCFG_BW_LIMIT_SMASK) >>
			   HFI_RXCFG_BW_LIMIT_SHIFT);
		seq_printf(s, "\tRXDMA whole:   0x%04llx\n",
			   (rxcfg & HFI_RXCFG_LEAK_INTEGER_SMASK) >>
			   HFI_RXCFG_LEAK_INTEGER_SHIFT);
		seq_printf(s, "\tRXDMA frac:    0x%04llx\n",
			   (rxcfg & HFI_RXCFG_LEAK_FRACTION_SMASK) >>
			   HFI_RXCFG_LEAK_FRACTION_SHIFT);
		seq_printf(s, "\tRXDMA config:  0x%04llx\n",
			   (rxcfg & HFI_RXCFG_METER_CONFIG_SMASK) >>
			   HFI_RXCFG_METER_CONFIG_SHIFT);

		seq_printf(s, "\tARB_PF limit:  0x%04llx\n",
			   (pcfg & HFI_PCFG_BW_LIMIT_SMASK) >>
			   HFI_PCFG_BW_LIMIT_SHIFT);
		seq_printf(s, "\tARB_PF whole:  0x%04llx\n",
			   (pcfg & HFI_PCFG_LEAK_INTEGER_SMASK) >>
			   HFI_PCFG_LEAK_INTEGER_SHIFT);
		seq_printf(s, "\tARB_PF frac:   0x%04llx\n",
			   (pcfg & HFI_PCFG_LEAK_FRACTION_SMASK) >>
			   HFI_PCFG_LEAK_FRACTION_SHIFT);

		seq_printf(s, "\tARB_FP limit:  0x%04llx\n",
			   (fcfg & HFI_FCFG_BW_LIMIT_SMASK) >>
			   HFI_FCFG_BW_LIMIT_SHIFT);
		seq_printf(s, "\tARB_FP whole:  0x%04llx\n",
			   (fcfg & HFI_FCFG_LEAK_INTEGER_SMASK) >>
			   HFI_FCFG_LEAK_INTEGER_SHIFT);
		seq_printf(s, "\tARB_FP frac:   0x%04llx\n",
			   (fcfg & HFI_FCFG_LEAK_FRACTION_SMASK) >>
			   HFI_FCFG_LEAK_FRACTION_SHIFT);

		seq_printf(s, "\tARB limit:     0x%04llx\n",
			   (acfg & HFI_ACFG_BW_LIMIT_SMASK) >>
			   HFI_ACFG_BW_LIMIT_SHIFT);
		seq_printf(s, "\tARB whole:     0x%04llx\n",
			   (acfg & HFI_ACFG_LEAK_INTEGER_SMASK) >>
			   HFI_ACFG_LEAK_INTEGER_SHIFT);
		seq_printf(s, "\tARB frac:      0x%04llx\n",
			   (acfg & HFI_ACFG_LEAK_FRACTION_SMASK) >>
			   HFI_ACFG_LEAK_FRACTION_SHIFT);

		seq_puts(s, "\n");
	}

	return 0;
}

static int hfi_hw_lm_pkey_show(struct seq_file *s, void *unused)
{
	struct hfi_pportdata *ppd = s->private;

	seq_printf(s, "%d (%s)\n", ppd->hw_lm_pkey,
		   ppd->hw_lm_pkey ? "Enabled" : "Disabled");
	return 0;
}

static int hfi_hw_ptl_pkey_show(struct seq_file *s, void *unused)
{
	struct hfi_pportdata *ppd = s->private;

	seq_printf(s, "%d (%s)\n", ppd->hw_ptl_pkey,
		   ppd->hw_ptl_pkey ? "Enabled" : "Disabled");
	return 0;
}

/*
 * Enable/Disable HW LM Pkey checking, enabled by default.
 * echo {0,1} > /sys/kernel/debug/hfi2/hfi20/port{1,..}/hw_lm_pkey
 */
static ssize_t hfi_hw_lm_pkey_write(struct file *f,
				    const char __user *ubuf,
				    size_t count, loff_t *ppos)
{
	struct hfi_pportdata *ppd = private2ppd(f);
	u8 val;
	int ret;

	ret = kstrtou8_from_user(ubuf, count, 0, &val);
	if (ret < 0)
		return -EINVAL;

	hfi_cfg_lm_pkey_check(ppd, val ? 1 : 0);
	return count;
}

/*
 * Enable/Disable HW PTL Pkey checking, enabled by default.
 * echo {0,1} > /sys/kernel/debug/hfi2/hfi20/port{1,..}/hw_ptl_pkey
 */
static ssize_t hfi_hw_ptl_pkey_write(struct file *f,
				     const char __user *ubuf,
				     size_t count, loff_t *ppos)
{
	struct hfi_pportdata *ppd = private2ppd(f);
	u8 val;
	int ret;

	ret = kstrtou8_from_user(ubuf, count, 0, &val);
	if (ret < 0)
		return -EINVAL;

	hfi_cfg_ptl_pkey_check(ppd, val ? 1 : 0);
	return count;
}

DEBUGFS_FILE_OPS_SINGLE(mgmt_allowed);
DEBUGFS_FILE_OPS_SINGLE(fw_auth_bypass);
DEBUGFS_FILE_OPS_SINGLE(neighbor_node_type);
DEBUGFS_FILE_OPS_SINGLE(led_cfg);
DEBUGFS_FILE_OPS_SINGLE(bw_arb);
DEBUGFS_FILE_OPS_SINGLE(qos);
DEBUGFS_FILE_OPS_SINGLE_WITH_WRITE(hw_lm_pkey);
DEBUGFS_FILE_OPS_SINGLE_WITH_WRITE(hw_ptl_pkey);

static void hfi_mgmt_dbg_init(struct hfi_devdata *dd)
{
	struct hfi_pportdata *ppd;
	int i;

	/* create mgmt related files for each port */
	for (ppd = dd->pport, i = 0; i < dd->num_pports; i++, ppd++) {
		debugfs_create_file("qos", 0444, ppd->hfi_port_dbg,
				    ppd, &hfi_qos_ops);
		debugfs_create_file("led_cfg", 0444, ppd->hfi_port_dbg,
				    ppd, &hfi_led_cfg_ops);
		debugfs_create_file("bw_arb", 0444, ppd->hfi_port_dbg,
				    ppd, &hfi_bw_arb_ops);
		debugfs_create_file("hw_ptl_pkey", 0644, ppd->hfi_port_dbg,
				    ppd, &hfi_hw_ptl_pkey_ops);
		debugfs_create_file("hw_lm_pkey", 0644, ppd->hfi_port_dbg,
				    ppd, &hfi_hw_lm_pkey_ops);

		ppd->hfi_neighbor_dbg = debugfs_create_dir("neighbor_mode",
							   ppd->hfi_port_dbg);
		debugfs_create_file("mgmt_allowed", 0444,
				    ppd->hfi_neighbor_dbg,
				    ppd, &hfi_mgmt_allowed_ops);
		debugfs_create_file("fw_auth_bypass", 0444,
				    ppd->hfi_neighbor_dbg,
				    ppd, &hfi_fw_auth_bypass_ops);
		debugfs_create_file("neighbor_node_type", 0444,
				    ppd->hfi_neighbor_dbg,
				    ppd, &hfi_neighbor_node_type_ops);
	}
}

static ssize_t host_link_bounce(struct file *file, const char __user *ubuf,
				size_t count, loff_t *ppos)
{
	struct hfi_pportdata *ppd;

	ppd = private2ppd(file);

	queue_work(ppd->hfi_wq, &ppd->link_bounce_work);

	return count;
}

static ssize_t lstate_read(struct file *file, char __user *ubuf, size_t count,
			   loff_t *ppos)
{
	struct hfi_pportdata *ppd;
	ssize_t rval;
	u32 lstate;

	ppd = private2ppd(file);

	lstate = hfi_driver_lstate(ppd);
	rval = simple_read_from_buffer(ubuf, count, ppos,
				       &lstate, sizeof(lstate));
	return rval;
}

static ssize_t lstate_write(struct file *file, const char __user *ubuf,
			    size_t count, loff_t *ppos)
{
	struct hfi_pportdata *ppd;
	int ret;
	u32 state;

	ppd = private2ppd(file);

	ret = get_user(state, (u32 __user *)ubuf);
	if (ret < 0)
		goto done;

	ret = hfi_set_link_state(ppd, state);
	if (ret < 0)
		goto done;

	ret = count;

done:
	return ret;
}

/* link negotiation and initialization */
FIRMWARE_READ(8051_state, CRK_CRK8051_STS_CUR_STATE)
FIRMWARE_READ(8051_cmd0, CRK_CRK8051_CFG_HOST_CMD_0)
FIRMWARE_WRITE(8051_cmd0, CRK_CRK8051_CFG_HOST_CMD_0)
FIRMWARE_READ(8051_cmd1, CRK_CRK8051_CFG_HOST_CMD_1)
FIRMWARE_READ(logical_link, OC_LCB_CFG_PORT)
FIRMWARE_WRITE(logical_link, OC_LCB_CFG_PORT)
HOST_STATE_READ(host_link_state)
LINK_WIDTH_READ(local, VERIFY_CAP_LOCAL_LINK_WIDTH)
LINK_WIDTH_READ(remote, VERIFY_CAP_REMOTE_LINK_WIDTH)
static const struct firmware_info firmware_ops[] = {
	DEBUGFS_OPS("8051_state", _8051_state_read, NULL),
	DEBUGFS_OPS("8051_cmd0", _8051_cmd0_read, _8051_cmd0_write),
	DEBUGFS_OPS("8051_cmd1", _8051_cmd1_read, NULL),
	DEBUGFS_OPS("logical_link", _logical_link_read, _logical_link_write),
	DEBUGFS_OPS("host_link_state", host_link_state_read, NULL),
	DEBUGFS_OPS("bounce_link", NULL, host_link_bounce),
	DEBUGFS_OPS("lstate", lstate_read, lstate_write),
	DEBUGFS_OPS("local_link_width", local_link_width_show, NULL),
	DEBUGFS_OPS("remote_link_width", remote_link_width_show, NULL),
};

static void *_qp_stats_seq_start(struct seq_file *s, loff_t *pos)
	__acquires(RCU)
{
	struct rvt_qp_iter *iter;
	loff_t n = *pos;

	iter = rvt_qp_iter_init(s->private, 0, NULL);

	/* stop calls rcu_read_unlock */
	rcu_read_lock();

	if (!iter)
		return NULL;

	do {
		if (rvt_qp_iter_next(iter)) {
			kfree(iter);
			return NULL;
		}
	} while (n--);

	return iter;
}

static void *_qp_stats_seq_next(struct seq_file *s, void *iter_ptr,
				loff_t *pos)
	__must_hold(RCU)
{
	struct rvt_qp_iter *iter = iter_ptr;

	(*pos)++;

	if (rvt_qp_iter_next(iter)) {
		kfree(iter);
		return NULL;
	}

	return iter;
}

static void _qp_stats_seq_stop(struct seq_file *s, void *iter_ptr)
	__releases(RCU)
{
	rcu_read_unlock();
}

static int _qp_stats_seq_show(struct seq_file *s, void *iter_ptr)
{
	struct rvt_qp_iter *iter = iter_ptr;

	if (!iter)
		return 0;

	qp_iter_print(s, iter);

	return 0;
}

DEBUGFS_SEQ_FILE_OPS(qp_stats);
DEBUGFS_SEQ_FILE_OPEN(qp_stats)
DEBUGFS_FILE_OPS(qp_stats);

void hfi_firmware_dbg_init(struct hfi_devdata *dd)
{
	struct hfi_pportdata *ppd;
	int i, j;

	/* create files for each port */
	for (ppd = dd->pport, j = 0; j < dd->num_pports; j++, ppd++) {
		for (i = 0; i < ARRAY_SIZE(firmware_ops); i++) {
			debugfs_create_file(firmware_ops[i].name,
					    firmware_ops[i].ops.write ?
					    0644 : 0444,
					    ppd->hfi_port_dbg,
					    ppd,
					    &firmware_ops[i].ops);
		}
	}
}

static void hfi_portcntrs_dbg_init(struct hfi_devdata *dd)
{
	struct hfi_pportdata *ppd;
	int i, j;

	/* create files for each port */
	for (ppd = dd->pport, j = 0; j < dd->num_pports; j++, ppd++) {
		for (i = 0; i < ARRAY_SIZE(port_cntr_ops); i++) {
			debugfs_create_file(port_cntr_ops[i].name,
					    !port_cntr_ops[i].ops.write ?
					    0444 : 0644,
					    ppd->hfi_port_dbg,
					    ppd,
					    &port_cntr_ops[i].ops);
		}
	}
}

#ifdef CONFIG_HFI2_STLNP
static void hfi_ts_dbg_init(struct hfi_devdata *dd)
{
	int i;
	struct dentry *ts_root;

	ts_root = debugfs_create_dir("timesync", dd->hfi_dev_dbg);

	if (!ts_root) {
		pr_warn("can't create debugfs directory: %s %p\n",
			"timesync", ts_root);
		return;
	}

	for (i = 0; i < ARRAY_SIZE(ts_ops); i++)
		debugfs_create_file(ts_ops[i].name, 0644,
				    ts_root,
				    dd->pport, /* ppd */
				    &ts_ops[i].ops);
}
#endif

static void hfi_devcntrs_dbg_init(struct hfi_devdata *dd)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(cntr_ops); i++)
		debugfs_create_file(cntr_ops[i].name, 0444,
				    dd->hfi_dev_dbg, dd,
				    &cntr_ops[i].ops);
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
	dd_dev_err(dd, "Invalid cmdq_head_update_freq, must be uint <= %d",
		   max);
	return -EINVAL;
}

static int hfi_tx_update_ctrl_show(struct seq_file *s, void *unused)
{
	return hfi_update_ctrl_show(s, FXR_TXCIC_CFG_HEAD_UPDATE_CNTRL);
}

static ssize_t hfi_tx_update_ctrl_write(struct file *f, const char __user *ubuf,
					size_t count, loff_t *ppos)
{
	/* See hfi_init_rate_control() for details */
	return hfi_update_ctrl_write(f, ubuf, count, ppos,
				     FXR_TXCIC_CFG_HEAD_UPDATE_CNTRL, 5);
}

static int hfi_rx_update_ctrl_show(struct seq_file *s, void *unused)
{
	return hfi_update_ctrl_show(s, FXR_RXCID_CFG_HEAD_UPDATE_CNTRL);
}

static ssize_t hfi_rx_update_ctrl_write(struct file *f, const char __user *ubuf,
					size_t count, loff_t *ppos)
{
	/* See hfi_init_rate_control() for details */
	return hfi_update_ctrl_write(f, ubuf, count, ppos,
				     FXR_RXCID_CFG_HEAD_UPDATE_CNTRL, 3);
}

DEBUGFS_FILE_OPS_SINGLE_WITH_WRITE(tx_update_ctrl);
DEBUGFS_FILE_OPS_SINGLE_WITH_WRITE(rx_update_ctrl);

static void hfi_head_update_ctrl_dbg_init(struct hfi_devdata *dd)
{
	debugfs_create_file("tx_cmdq_head_update_freq", 0644,
			    dd->hfi_dev_dbg, dd,
			    &hfi_tx_update_ctrl_ops);

	debugfs_create_file("rx_cmdq_head_update_freq", 0644,
			    dd->hfi_dev_dbg, dd,
			    &hfi_rx_update_ctrl_ops);
}

/*
 * driver stats field names, one line per stat, single string.  Used by
 * programs like hfistats to print the stats in a way which works for
 * different versions of drivers, without changing program source.
 * if hfi2_ibport_stats changes, this needs to change.  Names need to be
 * 12 chars or less (w/o newline), for proper display by hfistats utility.
 */
static const char * const hfi2_statnames[] = {
	"OOO_Comp",
	"RHF_Errs",
	"SendDMA",
	"SendIBDMA",
	"SendPIO",
	"ErrorIntr",
	"TxErrs",
	"RcvErrs",
	"NoPIOBufs",
	"CtxtsOpen",
	"LenErrs",
	"BufFull",
	"HdrFull",
};

static void *_driver_stats_names_seq_start(struct seq_file *s, loff_t *pos)
{
	if (*pos >= ARRAY_SIZE(hfi2_statnames))
		return NULL;
	return pos;
}

static void *_driver_stats_names_seq_next(
	struct seq_file *s,
	void *v,
	loff_t *pos)
{
	++*pos;
	if (*pos >= ARRAY_SIZE(hfi2_statnames))
		return NULL;
	return pos;
}

static void _driver_stats_names_seq_stop(struct seq_file *s, void *v)
{
}

static int _driver_stats_names_seq_show(struct seq_file *s, void *v)
{
	loff_t *spos = v;

	seq_printf(s, "%s\n", hfi2_statnames[*spos]);
	return 0;
}

DEBUGFS_SEQ_FILE_OPS(driver_stats_names);
DEBUGFS_SEQ_FILE_OPEN(driver_stats_names)
DEBUGFS_FILE_OPS(driver_stats_names);

/*
 * TODO - this is to print the driver_stats in easy to read format
 * via debugfs. Rework is needed to be compatible with hfi2stats.
 */
static int hfi_driver_stats_show(struct seq_file *s, void *v)
{
	struct hfi_devdata *dd = s->private;
	int i;
	u64 *stats = (u64 *)&dd->stats;

	for (i = 0; i < ARRAY_SIZE(hfi2_statnames); i++)
		seq_printf(s, "%s: %lld\n", hfi2_statnames[i],
			   stats[i]);
	return 0;
}

DEBUGFS_FILE_OPS_SINGLE(driver_stats);

void __init hfi_dbg_init(void)
{
	hfi_dbg_root = debugfs_create_dir(DRIVER_NAME, NULL);
	if (!hfi_dbg_root) {
		pr_warn("can't create %s\n", DRIVER_NAME);
		return;
	}

	debugfs_create_file("driver_stats_names", 0444, hfi_dbg_root,
			    NULL, &_driver_stats_names_file_ops);
}

void hfi_dbg_exit(void)
{
	debugfs_remove_recursive(hfi_dbg_root);
	hfi_dbg_root = NULL;
}

void hfi_dbg_dev_early_init(struct hfi_devdata *dd)
{
	char name[32], link[10];
	int unit = dd->unit;

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
}

void hfi_dbg_dev_init(struct hfi_devdata *dd)
{
	struct hfi_pportdata *ppd;
	char name[32];
	int j;

	/* create a directory for each port, and a neighbor_mode directory */
	for (j = 0, ppd = dd->pport; j < dd->num_pports; ppd++, j++) {
		snprintf(name, sizeof(name), "port%d", ppd->pnum);
		ppd->hfi_port_dbg = debugfs_create_dir(name, dd->hfi_dev_dbg);
	}
	debugfs_create_file("qp_stats", 0444, dd->hfi_dev_dbg, dd->ibd,
			    &_qp_stats_file_ops);
	debugfs_create_file("driver_stats", 0444, dd->hfi_dev_dbg, dd,
			    &hfi_driver_stats_ops);

	hfi_mgmt_dbg_init(dd);
#ifdef CONFIG_HFI2_STLNP
	hfi_e2e_dbg_init(dd);
#endif
	hfi_firmware_dbg_init(dd);
	hfi_devcntrs_dbg_init(dd);
	hfi_portcntrs_dbg_init(dd);
	hfi_head_update_ctrl_dbg_init(dd);
	hfi_verbs_dbg_init(dd);
#ifdef CONFIG_HFI2_STLNP
	hfi_ts_dbg_init(dd);
#endif
}

void hfi_dbg_dev_exit(struct hfi_devdata *dd)
{
	dd->hfi_dev_dbg = NULL;
}

#else
void __init hfi_dbg_init(void) {}
void hfi_dbg_exit(void) {}
void hfi_dbg_dev_early_init(struct hfi_devdata *dd) {}
void hfi_dbg_dev_init(struct hfi_devdata *dd) {}
void hfi_dbg_dev_exit(struct hfi_devdata *dd) {}
#endif
