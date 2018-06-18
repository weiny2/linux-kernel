/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2017 Intel Corporation.
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
 * This file holds workarounds for various inconsistencies between Simics
 * and emulation. Delete them as necessary
 */

#include <linux/pci-ats.h>
#include "hfi2.h"
#include "chip/fxr_linkmux_tp_defs.h"
#include "chip/fxr_linkmux_defs.h"
#include "chip/fxr_oc_defs.h"
#include "chip/fxr_rx_hp_csrs.h"
#include "chip/fxr_rx_hp_defs.h"
#include "chip/fxr_rx_e2e_csrs.h"
#include "chip/fxr_tx_otr_pkt_top_csrs.h"
#include "chip/fxr_rx_e2e_defs.h"
#include "chip/fxr_tx_otr_pkt_top_csrs_defs.h"

/* Missing from current definitions */
#define FXR_FZC_LPHY 0x2010000

#define FZC_LPHY_LSTS_LSMS_SMASK (0xffff << 16)
#define FZC_LPHY_LSTS_ACTIVE     (0x9)
#define FZC_LPHY_LSTS_SCF_SMASK  (BIT(4))

#define FZC_LPHY_LSTS 0x1c

#define HFI2_LSTS_ACTIVE(v) \
	((((v) & FZC_LPHY_LSTS_LSMS_SMASK) == FZC_LPHY_LSTS_ACTIVE) && \
		(!((v) & FZC_LPHY_LSTS_SCF_SMASK)))

/* FIXME - How does this work with no MNH?  */
static int hfi2_oc_init_half_rate(const struct hfi_pportdata *ppd)
{
#if 0
	u64 val = 0;
	int i;
	bool oc_up = false, mnh_up = false;


	/* Still uses MNH to init half rate */
	if (no_mnh)
		return 0;

	read_csr(ppd->dd, FXR_LM_CFG_FP_TIMER_PORT0);

	val = read_oc_opio_csr(ppd, MNH_OPIO_PHY_CFG_RESETA);
	val |= MNH_OPIO_PHY_CFG_RESETA_APHY_PLL_EN_SMASK;
	write_oc_opio_csr(ppd, MNH_OPIO_PHY_CFG_RESETA, val);

	val = read_mnh_opio_csr(ppd, MNH_OPIO_PHY_CFG_RESETA);
	val |= MNH_OPIO_PHY_CFG_RESETA_APHY_PLL_EN_SMASK;
	write_mnh_opio_csr(ppd, MNH_OPIO_PHY_CFG_RESETA, val);

	/* Retry up to 100 times */
	for (i = 0; i < 100; i++) {
		if (!oc_up) {
			val = read_oc_lphy_csr(ppd, FZC_LPHY_LSTS);
			if (HFI2_LSTS_ACTIVE(val))
				oc_up = true;
		}

		if (!mnh_up) {
			val = read_mnh_lphy_csr(ppd, FZC_LPHY_LSTS);
			if (HFI2_LSTS_ACTIVE(val))
				mnh_up = true;
		}

		if (mnh_up && oc_up)
			goto half_rate_done;

		/* 100MHz clock, 4000 cycle delay */
		usleep_range(40, 41);
	}

	ppd_dev_err(ppd, "%s: Timeout waiting for half-rate OPIO\n", __func__);
	read_csr(ppd->dd, FXR_LM_CFG_FP_TIMER_PORT0);
	return -ETIMEDOUT;

half_rate_done:
	ppd_dev_info(ppd, "Link trained to half-rate\n");
#endif
	return 0;
}

static int hfi2_oc_lcb_init(const struct hfi_pportdata *ppd)
{
	u64 val;
	int i;

	/* Still uses MNH to init half rate */
	if (loopback != LOOPBACK_LCB)
		return 0;

	read_csr(ppd->dd, FXR_LM_CFG_FP_TIMER_PORT0);
	write_csr(ppd->dd, OC_LCB_CFG_LOOPBACK, 2);
	write_csr(ppd->dd, OC_LCB_CFG_LINK_PARTNER_GEN, 0x101);
	write_csr(ppd->dd, OC_LCB_CFG_TX_FIFOS_RESET, 0);

	ndelay(64);

	write_csr(ppd->dd, OC_LCB_CFG_RUN, OC_LCB_CFG_RUN_EN_SMASK);

	for (i = 0; i < 1000; i++) {
		val = read_csr(ppd->dd, OC_LCB_STS_LINK_TRANSFER_ACTIVE);
		ppd_dev_info(ppd, "LCB_STS LTA: i %d %llx\n", i, val);
		if (val & OC_LCB_STS_LINK_TRANSFER_ACTIVE_VAL_SMASK)
			goto link_transfer_active;

		msleep(10000);
		/* 100MHz clock, 200 cycle delay */
		usleep_range(2, 3);

	}
	ppd_dev_err(ppd, "%s: Timeout waiting for LCB link transfer active\n",
		    __func__);
	return -ETIMEDOUT;

link_transfer_active:
	write_csr(ppd->dd, OC_LCB_CFG_ALLOW_LINK_UP,
		      OC_LCB_CFG_ALLOW_LINK_UP_VAL_SMASK);

	/* 100MHz clock, 200 cycle delay */
	usleep_range(2, 3);

	ppd_dev_dbg(ppd, "%s %d OC_LCB_CFG_ALLOW_LINK_UP 0x%llx\n",
		    __func__, __LINE__, read_csr(ppd->dd, OC_LCB_CFG_ALLOW_LINK_UP));

	/* turn on the LCB (turn off in lcb_shutdown). */
	write_csr(ppd->dd, OC_LCB_CFG_RUN, OC_LCB_CFG_RUN_EN_MASK);

	ppd_dev_dbg(ppd, "%s %d OC_LCB_CFG_RUN 0x%llx\n",
		    __func__, __LINE__, read_csr(ppd->dd, OC_LCB_CFG_RUN));

	/* 100MHz clock, 200 cycle delay */
	usleep_range(2, 3);

	hfi_read_lm_link_state(ppd);

	write_csr(ppd->dd, OC_LCB_CFG_PORT, 0x3);

	hfi_read_lm_link_state(ppd);

	write_csr(ppd->dd, OC_LCB_CFG_PORT, 0x4);

	hfi_read_lm_link_state(ppd);

	return 0;
}

int hfi2_oc_init(const struct hfi_pportdata *ppd)
{
	int ret;

	ret = hfi2_oc_init_half_rate(ppd);
	if (ret) {
		ppd_dev_err(ppd, "%s: Unable to init half rate %d\n",
			    __func__, ret);
		return ret;
	}

	/* FXRTODO: Init to full rate */

	ret = hfi2_oc_lcb_init(ppd);
	if (ret) {
		ppd_dev_err(ppd, "%s: Unable to bring up LCB %d\n",
			    __func__, ret);
		return ret;
	}

	return ret;
}

void hfi_zebu_hack_default_mtu(struct hfi_pportdata *ppd)
{
	ppd->vl_mtu[0] = HFI_DEFAULT_MAX_MTU;
	ppd->vl_mtu[1] = HFI_DEFAULT_MAX_MTU;
	ppd->vl_mtu[2] = HFI_DEFAULT_MAX_MTU;
	ppd->vl_mtu[3] = HFI_DEFAULT_MAX_MTU;
	ppd->vl_mtu[4] = HFI_DEFAULT_MAX_MTU;
	ppd->vl_mtu[5] = HFI_DEFAULT_MAX_MTU;
	ppd->vl_mtu[6] = HFI_DEFAULT_MAX_MTU;
	ppd->vl_mtu[7] = HFI_DEFAULT_MAX_MTU;
	ppd->vl_mtu[8] = HFI_DEFAULT_MAX_MTU;
}

void hfi_pt_cache_fill(struct hfi_devdata *dd, struct hfi_ctx *ctx, int pt_idx)
{
	union ptentry_fp1 *pt_array, *ptentryh;
	union pte_cache_addr pte;
	RXHP_CFG_PTE_CACHE_ACCESS_CTL_t ctl;
	RXHP_CFG_PTE_CACHE_ACCESS_DATA_t data[3];
	RXHP_CFG_PTE_CACHE_ACCESS_DATA3_t data3;

	pt_array = (union ptentry_fp1 *)ctx->pt_addr;
	ptentryh = &pt_array[HFI_PT_INDEX(PTL_NONMATCHING_PHYSICAL, pt_idx)];

	pte.tpid = 0;
	pte.ptindex = pt_idx;
	pte.ni = PTL_NONMATCHING_PHYSICAL;

	data[0].val = ptentryh->val[0];
	write_csr(dd, FXR_RXHP_CFG_PTE_CACHE_ACCESS_DATA, data[0].val);
	data[1].val = ptentryh->val[1];
	write_csr(dd, FXR_RXHP_CFG_PTE_CACHE_ACCESS_DATA + 8, data[1].val);
	data[2].val = ptentryh->val[2];
	write_csr(dd, FXR_RXHP_CFG_PTE_CACHE_ACCESS_DATA + 16, data[2].val);
	data3.val = ptentryh->val[3];
	write_csr(dd, FXR_RXHP_CFG_PTE_CACHE_ACCESS_DATA3, data3.val);
	write_csr(dd, FXR_RXHP_CFG_PTE_CACHE_ACCESS_DATA_BIT_ENABLE, ~0);
	write_csr(dd, FXR_RXHP_CFG_PTE_CACHE_ACCESS_DATA_BIT_ENABLE + 8, ~0);
	write_csr(dd, FXR_RXHP_CFG_PTE_CACHE_ACCESS_DATA_BIT_ENABLE + 16, ~0);
	write_csr(dd, FXR_RXHP_CFG_PTE_CACHE_ACCESS_DATA3, data3.val);
	write_csr(dd, FXR_RXHP_CFG_PTE_CACHE_ACCESS_DATA_BIT_ENABLE3, 0xf);

	ctl.field.cmd = 1;
	ctl.field.busy = 1;
	ctl.field.address = pte.val;
	write_csr(dd, FXR_RXHP_CFG_PTE_CACHE_ACCESS_CTL, ctl.val);
	mdelay(1000);
	while (1) {
		ctl.val = read_csr(dd, FXR_RXHP_CFG_PTE_CACHE_ACCESS_CTL);
		if (!ctl.field.busy) {
			pr_err("%s %d PTE cache fill done 0x%x\n",
				__func__, __LINE__, ctl.field.busy);
			break;
		} else {
			pr_err("%s %d PTE cache fill busy 0x%x\n",
				__func__, __LINE__, ctl.field.busy);
		}
		mdelay(1000);
	}
}

void hfi_psn_cache_fill(struct hfi_devdata *dd)
{
	RXE2E_CFG_PSN_CACHE_ACCESS_CTL_t rx_ctl = {.val = 0};
	RXE2E_CFG_PSN_CACHE_ACCESS_DATA_t rx_data = {.val = 0};
	union psn_cache_addr psn_cache;
	TXOTR_PKT_CFG_PSN_CACHE_ACCESS_CTL_t tx_ctl = {.val = 0};
	TXOTR_PKT_CFG_PSN_CACHE_ACCESS_DATA_t tx_data = {.val = 0};
	int lid, slid;

	for (slid = 1; slid <= 2; slid++) {
		for (lid = 1; lid < 5; lid++) {
			psn_cache.lid = lid;
			psn_cache.this_end_lid = slid;
			psn_cache.tc = 0;

			pr_err("%s psn_cache.lid %d psn_cache.this_end_lid %d\n",
				__func__, psn_cache.lid, psn_cache.this_end_lid);
			tx_data.field.max_seq_dist = 1024;
			tx_data.field.ctrl = TX_CONNECTED;
			write_csr(dd, FXR_TXOTR_PKT_CFG_PSN_CACHE_ACCESS_DATA, tx_data.val);
			write_csr(dd, FXR_TXOTR_PKT_CFG_PSN_CACHE_ACCESS_DATA_BIT_ENABLE, ~0);

			tx_ctl.field.cmd = 1;
			tx_ctl.field.busy = 1;
			tx_ctl.field.address = psn_cache.val;
			write_csr(dd, FXR_TXOTR_PKT_CFG_PSN_CACHE_ACCESS_CTL, tx_ctl.val);
			while (1) {
				tx_ctl.val = read_csr(dd, FXR_TXOTR_PKT_CFG_PSN_CACHE_ACCESS_CTL);
				if (!tx_ctl.field.busy) {
					pr_err("%s %d TX PSN cache fill done 0x%x bad 0x%x\n",
						__func__, __LINE__, tx_ctl.field.busy,
						tx_ctl.field.bad_addr);
					break;
				} else {
					pr_err("%s %d TX PSN cache fill busy 0x%x bad 0x%x\n",
						__func__, __LINE__, tx_ctl.field.busy,
						tx_ctl.field.bad_addr);
				}
				mdelay(100);
			}
			rx_data.field.connected = 1;
			write_csr(dd, FXR_RXE2E_CFG_PSN_CACHE_ACCESS_DATA, rx_data.val);
			write_csr(dd, FXR_RXE2E_CFG_PSN_CACHE_ACCESS_DATA_BIT_ENABLE, ~0);

			rx_ctl.field.cmd = 1;
			rx_ctl.field.busy = 1;
			rx_ctl.field.address = psn_cache.val;
			write_csr(dd, FXR_RXE2E_CFG_PSN_CACHE_ACCESS_CTL, rx_ctl.val);
			while (1) {
				rx_ctl.val = read_csr(dd, FXR_RXE2E_CFG_PSN_CACHE_ACCESS_CTL);
				if (!rx_ctl.field.busy) {
					pr_err("%s %d RX PSN cache fill done 0x%x bad 0x%x\n",
						__func__, __LINE__, rx_ctl.field.busy,
						rx_ctl.field.bad_addr);
					break;
				} else {
					pr_err("%s %d RX PSN cache fill busy 0x%x bad 0x%x\n",
						__func__, __LINE__, rx_ctl.field.busy,
						rx_ctl.field.bad_addr);
				}
				mdelay(100);
			}
		}
	}
}

int hfi_pte_cache_read(struct hfi_devdata *dd)
{
	union pte_cache_addr pte_cache_tag;
	RXHP_CFG_PTE_CACHE_ACCESS_CTL_t pte_cache_access = {.val = 0};

	pr_err("%s %d\n", __func__, __LINE__);
	/* invalidate cached host memory in HFI for Portals Tables by PID */
	pte_cache_access.field.cmd = 0;
	pte_cache_tag.val = 0;
	pte_cache_tag.tpid = 1;
	pte_cache_access.field.address = pte_cache_tag.val;
	pte_cache_tag.tpid = 1;
	pte_cache_tag.ni = PTL_NONMATCHING_PHYSICAL;
	pte_cache_tag.ptindex = 255;
	pte_cache_access.field.mask_address = pte_cache_tag.val;
	write_csr(dd, FXR_RXHP_CFG_PTE_CACHE_ACCESS_CTL, pte_cache_access.val);
	return 0;
}

/* This is needed if interrupts are broken for verbs */
#if 0
static int hfi2_send_wait(void *data)
{
	struct hfi2_ibtx *ibtx = data;
	struct hfi2_ibport *ibp = ibtx->ibp;

	dev_info(ibp->dev, "TX kthread %d starting\n",
		 ibp->port_num);
	allow_signal(SIGINT);
	while (!kthread_should_stop()) {
		hfi2_send_event(&ibtx->send_eq, ibtx);
		msleep(10);
	}
	dev_info(ibp->dev, "TX kthread %d stopping\n",
		 ibp->port_num);
	return 0;
}
#endif

/* These are needed if interrupts are broken for vnic */
#if 0
static void hfi2_rx_tasklet(unsigned long data)
{
	struct hfi2_ctx_info *ctx_i = (struct hfi2_ctx_info *)data;

	hfi2_vnic_rx_isr_cb(&ctx_i->eq_rx, (void *)data);
	hfi2_vnic_tx_isr_cb(&ctx_i->eq_tx, (void *)data);
}

static int hfi2_rx_wait(void *data)
{
	struct hfi2_ctx_info *ctx_i = data;

	allow_signal(SIGINT);
	while (!kthread_should_stop()) {
		tasklet_schedule(&ctx_i->task);
		msleep(10);
	}
	return 0;
}
#endif

/* Old vnic workarounds */
#if 0
static u32 skb_rx_sum(struct sk_buff *skb)
{
	u32 sum = 0;
	int i;
	u8 *data = skb->data;
	printk("%s ", __func__);
	for (i = 0; i < skb->len; i++) {
		printk("0x%x ", *data);
		sum += *data;
		data++;
	}
	printk("sum 0%x\n", sum);
	return sum;
}

static u32 skb_tx_sum(struct sk_buff *skb)
{
	u32 sum = 0;
	int i, j;
	u8 nr_frags = skb_shinfo(skb)->nr_frags;
	u16 headlen = skb_headlen(skb);

	printk("%s ", __func__);
	if (headlen) {
		u8 *data = skb->data;
		for (i = 0; i < headlen; i++) {
			printk("0x%x ", *data);
			sum += *data;
			data++;
		}
	}

	for (j = 0; j < nr_frags; j++) {
		struct skb_frag_struct *frag = &skb_shinfo(skb)->frags[j];
		u8 *data = skb_frag_address(frag);

		for (i = 0; i < skb_frag_size(frag); i++) {
			printk("0x%x ", *data);
			sum += *data;
			data++;
		}
	}
	printk("sum 0%x\n", sum);
	return sum;
}
#endif

/* Drain hack workarounds */

/*
 * To enable this hack, put this right before hfi_ctx_init in hfi_pci_dd_init,
 * set the timeout in hfi_cmdq_disable to -1 (infinite), set the mdelay in
 * hfi_cmdq_disable to 1000 (current zebu uses mdelay(1)
 */
#if 0
	read_csr(dd, FXR_LM_CFG_FP_TIMER_PORT0);
	/* now configure CQ head addresses */
	for (i = 0; i < HFI_CQ_COUNT; i++)
	for (i = 0; i < 2; i++) {
	//for (i = 0; i < HFI_CQ_COUNT; i++)
		pr_err("%s %d start done\n", __func__, __LINE__);
		hfi_cq_head_config(dd, i, dd->cq_head_base);

		pr_err("%s %d done\n", __func__, __LINE__);
	}
	read_csr(dd, FXR_LM_CFG_FP_TIMER_PORT0);
	/* TX and RX command queues - fast path access */
	dd->cq_tx_base = (void __iomem *)dd->physaddr + FXR_TX_CQ_ENTRY;
	dd->cq_rx_base = (void __iomem *)dd->physaddr + FXR_RX_CQ_ENTRY;
	dd->cq_tx_base = (void *)dd->physaddr + FXR_TX_CQ_ENTRY;
	dd->cq_rx_base = (void *)dd->physaddr + FXR_RX_CQ_ENTRY;
#endif

