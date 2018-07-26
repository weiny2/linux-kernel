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
#ifndef _HFI2_H
#define _HFI2_H

#include <linux/miscdevice.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/ptp_clock_kernel.h>
#include <rdma/ib_smi.h>
#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>
#include "core/hfi_core.h"
#include "chip/fxr_top_defs.h"
#include "verbs/mad.h"
#include "firmware.h"
#include "platform.h"
#include "pend_cmdq.h"
#include "qsfp.h"
#include "link.h"

/* bumped 1 from s/w major version of TrueScale */
#define HFI2_CHIP_VERS_MAJ 4U

/* don't care about this except printing */
#define HFI2_CHIP_VERS_MIN 0U

#define NEIGHBOR_TYPE_HFI	0
#define NEIGHBOR_TYPE_SWITCH	1

/*
 * There are 4 TCs, and one of them is used for management
 * traffic only (TC3). The other three will be used for
 * bandwidth group arbitration.
 */
#define HFI2_NUM_BW_GROUPS 3

extern unsigned int hfi_max_mtu;
#define DRIVER_NAME		KBUILD_MODNAME
#define DRIVER_CLASS_NAME	DRIVER_NAME

/* IRQs */
#define HFI_NUM_EQ_INTERRUPTS	255
#define HFI_NUM_INTERRUPTS	320

/* PSB and CMDQ offsets */
#define HFI_PSB_PT_OFFSET	0
#define HFI_PSB_CT_OFFSET	(HFI_PSB_PT_OFFSET + HFI_PSB_PT_SIZE)
#define HFI_PSB_EQ_DESC_OFFSET	(HFI_PSB_CT_OFFSET  + HFI_PSB_CT_SIZE)
#define HFI_PSB_EQ_HEAD_OFFSET	(HFI_PSB_EQ_DESC_OFFSET + HFI_PSB_EQ_DESC_SIZE)
#define HFI_PSB_FIXED_TOTAL_MEM	(HFI_PSB_PT_SIZE + HFI_PSB_CT_SIZE + \
				HFI_PSB_EQ_DESC_SIZE + HFI_PSB_EQ_HEAD_SIZE)
#define HFI_PSB_TRIG_OFFSET	HFI_PSB_FIXED_TOTAL_MEM
#define HFI_TRIG_OP_SIZE	128
#define HFI_LE_ME_SIZE		128
#define HFI_UNEXP_SIZE		128

/* TX CSR spacing matches size of command queue */
#define HFI_CMDQ_TX_OFFSET        HFI_CMDQ_TX_SIZE
/* RX CSR spacing uses 8KB alignment (even though RX CMDQ is smaller) */
#define HFI_CMDQ_RX_OFFSET        (PAGE_SIZE * 2)
#define HFI_CMDQ_TX_IDX_ADDR(addr, idx)	((addr) + (HFI_CMDQ_TX_OFFSET * (idx)))
#define HFI_CMDQ_RX_IDX_ADDR(addr, idx)	((addr) + (HFI_CMDQ_RX_OFFSET * (idx)))
#define HFI_CMDQ_HEAD_OFFSET      64 /* put CMDQ heads on separate cachelines */
#define HFI_CMDQ_HEAD_ADDR(addr, idx)	\
	((addr) + (HFI_CMDQ_HEAD_OFFSET * (idx)))

/* Offline Disabled Reason is 4-bits */
#define HFI_ODR_MASK(rsn) ((rsn) & OPA_PI_MASK_OFFLINE_REASON)

enum {
	SHARED_CREDITS,
	DEDICATED_CREDITS
};

#define TPORT_PREEMPT_DATA_VLS 0x1FFull
#define TPORT_PREEMPT_BITS_PER_ENTRY 10
#define TPORT_PREEMPT_R0_DATA_CNT 5
#define TPORT_PREEMPT_R1_DATA_CNT 4
#define TPORT_PREEMPT_VL15_SHIFT 40

/* partition enforcement flags */
#define HFI_PART_ENFORCE_IN	BIT(0)
#define HFI_PART_ENFORCE_OUT	BIT(1)

#define HFI_IB_CC_TABLE_CAP_DEFAULT 31
#define HFI_FXR_BAR		0
#define HFI_NUM_PPORTS		1
#define HFI_MAX_VLS		32
#define HFI_NUM_PIDS		4096
#define HFI_PID_SYSTEM		0
#define HFI_PID_BYPASS_BASE	0xF00
#define HFI_NUM_BYPASS_PIDS	160
/* last PID not usable as an RX context (0xFFF reserved as match any PID) */
#define HFI_NUM_USABLE_PIDS	(HFI_NUM_PIDS - 1)
#define HFI_TPID_ENTRIES	16
#define HFI_DLID_TABLE_SIZE	(64 * 1024)

/*
 * Default protection domain is set of user processes (using kernel UID),
 * as such PTL_UID=0 is always reserved for kernel clients.
 */
#define HFI_DEFAULT_PTL_UID	__kuid_val(current_uid())

#define IS_PID_BYPASS(ctx) \
	(((ctx)->mode & HFI_CTX_MODE_USE_BYPASS) && \
	 ((ctx)->pid >= HFI_PID_BYPASS_BASE) && \
	 ((ctx)->pid < HFI_PID_BYPASS_BASE + HFI_NUM_BYPASS_PIDS))

/* FXRTODO: based on 16bit (9B) LID */
#define HFI_MULTICAST_LID_BASE	0xC000
/* Maximum number of traffic and message classes supported */
#define HFI_MAX_TC		4
#define HFI_TC_MASK		(HFI_MAX_TC - 1)
#define HFI_MAX_MC		2
#define HFI_MAX_MCTC		8
#define HFI_MCTC_MASK		(HFI_MAX_MCTC - 1)
#define HFI_MC_SHIFT		2

/* Maximum number of unicast LIDs supported by default */
#define HFI_DEFAULT_MAX_LID_SUPP	0x103ff
// TODO: Fix for ZEBU/FPGA #define HFI_DEFAULT_MAX_LID_SUPP	(zebu ? 0xbfff : 0x103ff)

/* TX timeout for E2E control messages */
#define HFI_TX_TIMEOUT_MS	100
#define HFI_TX_TIMEOUT_MS_ZEBU	1000

/* per-VL status clear, in ms */
#define HFI_VL_STATUS_CLEAR_TIMEOUT	5000

/* timeout for ctxt cleanup */
#define HFI_CMDQ_RESET_TIMEOUT_MS		10
#define HFI_CMDQ_RESET_TIMEOUT_MS_ZEBU		1000
#define HFI_CMDQ_DRAIN_RESET_TIMEOUT_MS		2000
#define HFI_OTR_CANCELLATION_TIMEOUT_MS		1
#define HFI_CACHE_INVALIDATION_TIMEOUT_MS	10
#define HFI_CACHE_INVALIDATION_TIMEOUT_MS_ZEBU	2000
#define HFI_PASID_DRAIN_TIMEOUT_MS		2000

/* timeout for privileged CMDQ writes */
#define HFI_PRIV_CMDQ_TIMEOUT_MS		5000

/* use this MTU size if none other is given */
#define HFI_DEFAULT_ACTIVE_MTU	8192
// TODO: Fix for ZEBU/FPGA #define HFI_DEFAULT_ACTIVE_MTU	(zebu ? 4096 : 10240)
/* use this MTU size as the default maximum */
#define HFI_DEFAULT_MAX_MTU	10240
// TODO: Fix for ZEBU/FPGA #define HFI_DEFAULT_MAX_MTU	(zebu ? 4096 : 10240)
/* for Verbs, the maximum MTU supported by rdmavt is 8K */
#define HFI_VERBS_MAX_MTU	8192
// TODO: Fix for ZEBU/FPGA #define HFI_VERBS_MAX_MTU	(zebu ? 4096 : 8192)
/* The largest MAD packet size. */
#define HFI_MIN_VL_15_MTU	2048
/* Number of Data VLs supported */
#define HFI_NUM_DATA_VLS	9
/* Number of PKey entries in the HW */
#define HFI_MAX_PKEYS		64
/* Number of flits for bandwidth limit shaping */
#define HFI_DEFAULT_BW_LIMIT_FLITS 2500
/* Number of flits per clock in TX */
/* FXRTODO: Find out if this is correct via HSD */
#define HFI_TX_FLITS_PER_CLK 4
/* Number of flits per clock in RX */
#define HFI_RX_FLITS_PER_CLK 1

#define HFI2_GUIDS_PER_PORT	2
#define HFI2_PORT_GUID_INDEX	0
#define HFI2_PORT_GUID_EXTENDED	1

/* In accordance with stl vol 1 section 4.1 */
#define PGUID_MASK		(~(0x3UL << 32))
#define PORT_GUID(ng, pn)	(((be64_to_cpu(ng)) & PGUID_MASK) |\
				 (((u64)pn) << 32))

#define pidx_to_pnum(id)	((id) + 1)
#define pnum_to_pidx(pn)	((pn) - 1)

#define HFI_PKEY_CAM_SHIFT		0
#define HFI_PKEY_CAM_MASK		0x7fff
#define HFI_PKEY_CAM(pkey)		(((pkey) >> HFI_PKEY_CAM_SHIFT) & \
					HFI_PKEY_CAM_MASK)

#define HFI_PKEY_IS_EQ(p1, p2) (HFI_PKEY_CAM((p1)) == HFI_PKEY_CAM((p2)))

#define HFI_PKEY_MEMBER_SHIFT		15
#define HFI_PKEY_MEMBER_MASK		0x1
#define HFI_PKEY_MEMBER_TYPE(pkey)	(((pkey) >> HFI_PKEY_MEMBER_SHIFT) & \
					HFI_PKEY_MEMBER_MASK)
#define HFI_SC_TO_TC_MC_MASK	(u64)(HFI_MCTC_MASK)
#define HFI_SL_TO_TC_MC_MASK	(u64)(HFI_MCTC_MASK)
#define HFI_GET_TC(mctc)	((mctc) & HFI_TC_MASK)
#define HFI_GET_MC(mctc)	((mctc) >> HFI_MC_SHIFT)
#define HFI_GET_MCTC(mc, tc)	(((mc) << HFI_MC_SHIFT) | (tc))
#define HFI_SL_TO_SC_MASK	(u64)(OPA_MAX_SCS - 1)
#define HFI_SC_TO_RESP_SL_MASK	(u64)(OPA_MAX_SLS - 1)

/* Any lane between 8 and 14 is illegal. Randomly chosen one from that list */
#define HFI_ILLEGAL_VL		12
#define HFI_NUM_USABLE_VLS	16	/* look at VL15 and less */

#define HFI_SC_VLT_MASK		(u64)(HFI_NUM_USABLE_VLS - 1)
#define HFI_SC_VLNT_MASK	(u64)(HFI_NUM_USABLE_VLS - 1)
#define HFI_SC_VLR_MASK		(u64)(HFI_NUM_USABLE_VLS - 1)

/* Message class and traffic class for VL15 packets */
#define HFI_VL15_MC		0
#define HFI_VL15_TC		3

#define hfi_vl_to_idx(vl)	(((vl) < HFI_NUM_DATA_VLS) ? \
				 (vl) : HFI_NUM_DATA_VLS)
#define hfi_idx_to_vl(idx)	(((idx) == HFI_NUM_DATA_VLS) ? 15 : (idx))
#define hfi_valid_vl(idx)	((idx) < HFI_NUM_DATA_VLS || (idx) == 15)

/* verify capability fabric fields */
#define HFI_VAU_SHIFT	0
#define HFI_VAU_MASK	0x0007
#define HFI_Z_SHIFT		3
#define HFI_Z_MASK		0x0001
#define HFI_VCU_SHIFT	4
#define HFI_VCU_MASK	0x0007
#define HFI_VL15BUF_SHIFT	8
#define HFI_VL15BUF_MASK	0x0fff
#define HFI_CRC_SIZES_SHIFT 20
#define HFI_CRC_SIZES_MASK	0x7

/*
 * Virtual Allocation Unit, defined as AU = 8*2^vAU, 32 bytes, AU is fixed
 * at 32 bytes for all generation two devices
 */
#define HFI_CM_VAU 2
#define HFI_CM_CU  1
/* in Bytes
 * FXRTODO: Until we have a register to read this value from
 * or a constant defined in the headers generated from CSR
 * use this harcoded value.
 */
#define HFI_RCV_BUFFER_SIZE		(128 * 1024)

/*
 * Any SL which is
 *  - Invalid (mapped to SC15 in the SL2SC table
 *  - No response SL
 *  - Is itself a response SL
 */
#define HFI_INVALID_RESP_SL		0xff

#define TRIGGER_OPS_SPILL_SIZE		(1024 * 1024 * 8)

/*
 * Setting the leak integral amount to the maximum effectively gives 100% of
 * the bandwidth to the MCTC
 */
#define HFI_MAX_TXOTR_LEAK_INTEGER 0x7

/* In general, we want to enable TX bandwidth capping only in the CCA case */
#define HFI_ENABLE_CAPPING true

#define HFI_FPE_CFG_BW_LIMIT_SMASK \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_FPE_BWMETER_BW_LIMIT_SMASK
#define HFI_FPE_CFG_LEAK_INTEGER_SMASK \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_FPE_BWMETER_LEAK_INTEGER_SMASK
#define HFI_FPE_CFG_LEAK_FRACTION_SMASK \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_FPE_BWMETER_LEAK_FRACTION_SMASK
#define HFI_FPE_CFG_BW_LIMIT_SHIFT \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_FPE_BWMETER_BW_LIMIT_SHIFT
#define HFI_FPE_CFG_LEAK_INTEGER_SHIFT \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_FPE_BWMETER_LEAK_INTEGER_SHIFT
#define HFI_FPE_CFG_LEAK_INTEGER_MASK \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_FPE_BWMETER_LEAK_INTEGER_MASK
#define HFI_FPE_CFG_LEAK_FRACTION_SHIFT \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_FPE_BWMETER_LEAK_FRACTION_SHIFT
#define HFI_FPE_CFG_LEAK_FRACTION_MASK \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_FPE_BWMETER_LEAK_FRACTION_MASK

#define HFI_FP_CFG_BW_LIMIT_SMASK \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_BWMETER_BW_LIMIT_SMASK
#define HFI_FP_CFG_LEAK_INTEGER_SMASK \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_BWMETER_LEAK_INTEGER_SMASK
#define HFI_FP_CFG_LEAK_FRACTION_SMASK \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_BWMETER_LEAK_FRACTION_SMASK
#define HFI_FP_CFG_BW_LIMIT_SHIFT \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_BWMETER_BW_LIMIT_SHIFT
#define HFI_FP_CFG_LEAK_INTEGER_SHIFT \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_BWMETER_LEAK_INTEGER_SHIFT
#define HFI_FP_CFG_LEAK_INTEGER_MASK \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_BWMETER_LEAK_INTEGER_MASK
#define HFI_FP_CFG_LEAK_FRACTION_SHIFT \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_BWMETER_LEAK_FRACTION_SHIFT
#define HFI_FP_CFG_LEAK_FRACTION_MASK \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_FP_BWMETER_LEAK_FRACTION_MASK

#define HFI_MCTC_CFG_BW_LIMIT_SMASK \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_MCTC_BWMETER_BW_LIMIT_SMASK
#define HFI_MCTC_CFG_LEAK_INTEGER_SMASK \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_MCTC_BWMETER_LEAK_INTEGER_SMASK
#define HFI_MCTC_CFG_LEAK_FRACTION_SMASK \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_MCTC_BWMETER_LEAK_FRACTION_SMASK
#define HFI_MCTC_CFG_BW_LIMIT_SHIFT \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_MCTC_BWMETER_BW_LIMIT_SHIFT
#define HFI_MCTC_CFG_LEAK_INTEGER_SHIFT \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_MCTC_BWMETER_LEAK_INTEGER_SHIFT
#define HFI_MCTC_CFG_LEAK_INTEGER_MASK \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_MCTC_BWMETER_LEAK_INTEGER_MASK
#define HFI_MCTC_CFG_LEAK_FRACTION_SHIFT \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_MCTC_BWMETER_LEAK_FRACTION_SHIFT
#define HFI_MCTC_CFG_LEAK_FRACTION_MASK \
	FXR_TXOTR_PKT_CFG_OPB_FIFO_ARB_MCTC_BWMETER_LEAK_FRACTION_MASK

/*
 * per driver stats, either not device nor port-specific,
 * or summed over all of the devices and ports.
 * If memebers are added or deleted, hfi2_statnames[] in debugfs.c
 * must change to match.
 * @n_rc_ooo_comp: Number of send completions that were out of order
 * @n_rhf_errors: Number of HW errors reported via RHF
 * @n_send_dma: Number of commands sent using DMA
 * @n_send_ib_dma: Number of commands sent using optimized IB DMA
 * @n_send_pio: Number of commands sent using PIO
 * @sps_errints: Number of error interrupts
 * @sps_txerrs: tx-related packet errors
 * @sps_rcverrs: non-crc rcv packet errors
 * @sps_nopiobufs: no pio bufs avail from kernel
 * @sps_ctxts: number of contexts currently open
 * @sps_lenerrs: number of kernel packets where RHF != LRH len
 * @sps_buffull: number of packets dropped when rcv buff is full
 * @sps_hdrfull: number of packets dropped when hdr is full
 */

struct hfi2_ib_stats {
	__u64 n_rc_ooo_comp;
	__u64 n_rhf_errors;
	__u64 n_send_dma;
	__u64 n_send_ib_dma;
	__u64 n_send_pio;
	__u64 sps_errints;
	__u64 sps_txerrs;
	__u64 sps_rcverrs;
	__u64 sps_nopiobufs;
	__u64 sps_ctxts;
	__u64 sps_lenerrs;
	__u64 sps_buffull;
	__u64 sps_hdrfull;
};

extern uint loopback;
extern int num_dev_cntrs;
extern int num_port_cntrs;
extern const char *portcntr_names[];
extern const char *devcntr_names[];

static inline void incr_cntr64(u64 *cntr)
{
	if (*cntr < (u64)-1LL)
		(*cntr)++;
}

/* per-SL CCA information */
struct cca_timer {
	struct hrtimer hrtimer;
	struct hfi_pportdata *ppd; /* read-only */
	int sl; /* read-only */
	u16 ccti; /* read/write - current value of CCTI */
};

#define HFI_EXTENDED_LID_MASK (0xf << 20)

/*
 * Private data for snoop/capture support.
 */
struct hfi1_snoop_data {
	struct miscdevice miscdev;
	int mode_flag;
	/* protect snoop data */
	struct mutex snoop_lock;
	struct list_head queue;
	wait_queue_head_t waitq;
	void *filter_value;
	int (*filter_callback)(void *hdr, void *data, void *value, bool bypass);
	u64 lm_cfg; /* Saved value of link mux Cfg register */
};

/*
 * Private data for diagnostic packet support
 * @miscdev: MISC device for diagnostic packet transmission
 * @ctx: Pointer to HFI context
 * @cmdq_tx: Pointer to TX Command Queue
 * @eq_tx: Event queue used for tracking sent packets
 * @cmdq_tx_lock: Lock to prevent multiple writers
 * @pend_cmdq: Pending CMDQ queueing struct. Shared with verbs
 */
struct hfi2_diagpkt_data {
	struct hfi_ctx *ctx;
	struct hfi_cmdq *cmdq_tx;
	struct hfi_eq eq_tx;
	/* Prevents multiple writers */
	spinlock_t cmdq_tx_lock;
	struct hfi_pend_queue *pend_cmdq;
};

/* snoop mode_flag values */
#define HFI1_PORT_SNOOP_MODE     1U
#define HFI1_PORT_CAPTURE_MODE   2U

/*
 * HFI or Host Link States
 *
 * These describe the states the driver thinks the logical and physical
 * states are in.  Used as an argument to set_link_state().  Implemented
 * as bits for easy multi-state checking.  The actual state can only be
 * one.
 *
 * FXRTODO: STL2 related states need to be added
 * Ganged, ShallowSleep and DeepSleep.
 */
#define __HLS_UP_INIT_BP	0
#define __HLS_UP_ARMED_BP	1
#define __HLS_UP_ACTIVE_BP	2
#define __HLS_DN_DOWNDEF_BP	3	/* link down default */
#define __HLS_DN_POLL_BP	4
#define __HLS_DN_DISABLE_BP	5
#define __HLS_DN_OFFLINE_BP	6
#define __HLS_VERIFY_CAP_BP	7
#define __HLS_GOING_UP_BP	8
#define __HLS_GOING_OFFLINE_BP  9
#define __HLS_LINK_COOLDOWN_BP 10

#define HLS_UP_INIT	  BIT(__HLS_UP_INIT_BP)
#define HLS_UP_ARMED	  BIT(__HLS_UP_ARMED_BP)
#define HLS_UP_ACTIVE	  BIT(__HLS_UP_ACTIVE_BP)
#define HLS_DN_DOWNDEF	  BIT(__HLS_DN_DOWNDEF_BP) /* link down default */
#define HLS_DN_POLL	  BIT(__HLS_DN_POLL_BP)
#define HLS_DN_DISABLE	  BIT(__HLS_DN_DISABLE_BP)
#define HLS_DN_OFFLINE	  BIT(__HLS_DN_OFFLINE_BP)
#define HLS_VERIFY_CAP	  BIT(__HLS_VERIFY_CAP_BP)
#define HLS_GOING_UP	  BIT(__HLS_GOING_UP_BP)
#define HLS_GOING_OFFLINE BIT(__HLS_GOING_OFFLINE_BP)
#define HLS_LINK_COOLDOWN BIT(__HLS_LINK_COOLDOWN_BP)

#define HLS_UP (HLS_UP_INIT | HLS_UP_ARMED | HLS_UP_ACTIVE)
#define HLS_DOWN ~(HLS_UP)

/* SMA idle flit payload commands */
#define SMA_IDLE_ARM	1
#define SMA_IDLE_ACTIVE 2

enum {
	HFI_IB_CFG_LIDLMC = 0,		/* LID and Mask*/
	HFI_IB_CFG_LWID_DG_ENB,		/* allowed Link-width downgrade */
	HFI_IB_CFG_LWID_ENB,		/* allowed Link-width */
	HFI_IB_CFG_LWID,		/* currently active Link-width */
	HFI_IB_CFG_SPD_ENB,		/* allowed Link speeds */
	HFI_IB_CFG_SPD,			/* current Link spd */
	HFI_IB_CFG_OP_VLS,		/* operational VLs */
	HFI_IB_CFG_PKEYS,		/* update partition keys */
	HFI_IB_CFG_8B_PKEY,		/* update 8B key */
	HFI_IB_CFG_10B_PKEY,		/* update 10B key */
	HFI_IB_CFG_MTU,			/* update MTU in IBC */
	HFI_IB_CFG_VL_HIGH_LIMIT,	/* Change VL high limit */
	HFI_IB_CFG_SL_TO_SC,		/* Change SLtoSC mapping */
	HFI_IB_CFG_SL_TO_MCTC,		/* Change SLtoMCTC mapping */
	HFI_IB_CFG_SC_TO_RESP_SL,	/* Change SCtoRespSL mapping */
	HFI_IB_CFG_SC_TO_MCTC,		/* Change SCtoMCTC mapping */
	HFI_IB_CFG_SC_TO_VLR,		/* Change SCtoVLr mapping */
	HFI_IB_CFG_SC_TO_VLT,		/* Change SCtoVLt mapping */
	HFI_IB_CFG_SC_TO_VLNT,		/* Change Neighbor's SCtoVL mapping */
	HFI_IB_CFG_BW_ARB,		/* Change BW Arbitrator tables */
	HFI_IB_CFG_PKT_FORMAT,		/* Change HFI packet format */
	HFI_IB_CFG_SL_PAIRS,		/* Change SL pairs */
	HFI_IB_CFG_LED_INFO             /* Change LEDInfo */
};

/* verify capability fabric CRC size bits */
enum {
	HFI_CAP_CRC_14B = (1 << 0), /* 14b CRC */
	HFI_CAP_CRC_16B = (1 << 1), /* 16b CRC */
	HFI_CAP_CRC_48B = (1 << 2), /* 48b CRC */
	HFI_CAP_CRC_12B_16B_PER_LANE = (1 << 3) /* 12b-16b per lane CRC */
};

#define HFI_SUPPORTED_CRCS (HFI_CAP_CRC_14B | HFI_CAP_CRC_48B)

/* LCB_CFG_CRC_MODE TX_VAL and RX_VAL CRC mode values */
enum {
	HFI_LCB_CRC_16B = 0,	/* 16b CRC */
	HFI_LCB_CRC_14B = 1,	/* 14b CRC */
	HFI_LCB_CRC_48B = 2,	/* 48b CRC */
	HFI_LCB_CRC_12B_16B_PER_LANE = 3	/* 12b-16b per lane CRC */
};

#define HFI_LTP_CRC_SUPPORTED_SHIFT	8
#define HFI_LTP_CRC_SUPPORTED_MASK	0xf
#define HFI_LTP_CRC_SUPPORTED(val)	(((val) >> \
					HFI_LTP_CRC_SUPPORTED_SHIFT) & \
					HFI_LTP_CRC_SUPPORTED_MASK)

#define HFI_LTP_CRC_ENABLED_SHIFT	4
#define HFI_LTP_CRC_ENABLED_MASK	0xf
#define HFI_LTP_CRC_ENABLED(val)	(((val) >> \
					HFI_LTP_CRC_ENABLED_SHIFT) & \
					HFI_LTP_CRC_ENABLED_MASK)

#define HFI_LTP_CRC_ACTIVE_SHIFT	0
#define HFI_LTP_CRC_ACTIVE_MASK		0xf
#define HFI_LTP_CRC_ACTIVE(val)		(((val) >> \
					HFI_LTP_CRC_ENABLED_SHIFT) & \
					HFI_LTP_CRC_ACTIVE_MASK)

struct vl_limit {
	__be16 dedicated;
	__be16 shared;
};

struct buffer_control {
	__be16 reserved;
	__be16 overall_shared_limit;
	struct vl_limit vl[OPA_MAX_VLS];
};

struct hfi_link_down_reason {
	/*
	 * SMA-facing value.  Should be set from .latest when
	 * HLS_UP_* -> HLS_DN_* transition actually occurs.
	 */
	u8 sma;
	u8 latest;
};

struct err_info_rcvport {
	u8 status_and_code;
	u64 packet_flit1;
	u64 packet_flit2;
};

struct err_info_constraint {
	u8 status;
	u16 pkey;
	u32 slid;
};

struct hfi_irq_entry {
	void *arg;
	cpumask_var_t mask;
	struct list_head irq_wait_head;
	spinlock_t irq_wait_lock;
	struct hfi_devdata *dd;
	int intr_src;
};

/*
 * hfi_ptcdata - HFI traffic class specific information per port
 * @psn_size: buffer size of psn_base below in bytes
 * @psn_base: packet sequence number buffer array used for TX/RX
 *	One buffer per pkey
 * @e2e_tx_state_cache: E2E connection TX state cache.
 *	For TX we need an idr since we want to map LIDs to
 *	the pkey used to setup that connection.
 * @e2e_rx_state_cache: E2E connection RX state cache
 *	For RX we only need to track that a LID is associated
 *	with an e2e connection, since we only tear down TX so
 *	an ida is sufficient.
 */
struct hfi_ptcdata {
	int psn_size;
	void *psn_base[HFI_MAX_PKEYS];
	struct idr e2e_tx_state_cache;
	struct ida e2e_rx_state_cache;
};

struct bw_arb_cache {
	/* protect bw arb cache */
	spinlock_t lock;
	struct opa_bw_element bw_group[OPA_MAX_BW_GROUP];
	__be32 preempt_matrix[OPA_MAX_BW_GROUP];
};

/*
 * Max number of u64 per queued command data. Currently set to the number of
 * bytes in two slots, since that is all we handle in the pending CMDQ write.
 */
#define HFI_MAX_PEND_CMD_LEN 16
#define HFI_MAX_PEND_CMD_LEN_BYTES (HFI_MAX_PEND_CMD_LEN * sizeof(u64))

/*
 * hfi_pend_cmd - Privileged command pending information
 * @completion: Completion to signal when done
 * @list: Links us to the list
 * @cmdq: Pointer to command queue to which the command will be written
 * @eq: Optional pointer to EQ to check before writing
 * @slot_ptr: Pointer to data
 * @cmd_slots: Number of slots for this command
 * @ret: Return code to waiter
 * @wait: Boolean flag to indicate caller will wait
 * @slots: Slot data storage area if the caller is asynchronous
 */
struct hfi_pend_cmd {
	struct completion completion;
	struct list_head list;
	struct hfi_cmdq *cmdq;
	struct hfi_eq *eq;
	u64 *slot_ptr;
	int cmd_slots;
	int ret;
	bool wait;
	u64 slots[HFI_MAX_PEND_CMD_LEN];
};

/*
 * hfi_pportdata - HFI port specific information
 *
 * @dd: pointer to the per node hfi_devdata
 * @guids: GUIDs for this interface, in network order, guids[0] is port guid
 * @neighbor_guid: Node guid of the neighboring port
 * @lid: LID for this port
 * @max_lid: Maximum LID in the fabric this port is attached to
 * @ptc: per traffic class specific fields
 * @lcb_sts_rtt: Cached link round trip time
 * @max_active_mtu: The max active MTU of all the VLs
 * @port_error_action: contains bit mask for various errors. The HFI
 *	should initiate link-bounce when the corresponding error occurs.
 * @port_type: Type of physical port
 * @sm_trap_qp: qp number to send trap to SM from SMA
 * @sa_qp: SA qp number
 * @last_phystate: Physical state last it was checked
 * @link_width_supported: Supported link width
 * @link_width_downgrade_supported: Supported link width downgrading
 * @link_speed_supported: Supported link speed
 * @link_width_enabled: Enabled link width
 * @link_width_downgrade_enabled: Enabled link width downgrading
 * @link_speed_enabled: Enabled link speed
 * @link_width_active: Currently active link width
 * @link_width_downgrade_tx_active: Current TX side link width downgrading
 * @link_width_downgrade_rx_active: Current RX side link width downgrading
 * @link_speed_active: Current active link speed
 * @port_ltp_crc_mode: Supported, enabled and active LTP CRC modes
 * @pkey_8b: Implicit 8B Pkey
 * @pkey_10b: Partial 10B Pkey
 * @packet_format_supported: Supported packet format on this port.
 * @packet_format_enabled: Enabled packet format on this port
 * @port_crc_mode_enabled: CRC mode to be enabled
 * @lmc: LID mask control
 * @pnum: port number of this port
 * @vls_supported: Virtual lane supported
 * @vls_operational: Virtual lane operational
 *	VL Arbitration table
 * @neighbor_type: Node type of neighboring port
 *			0 - HFI
 *			1 - Switch
 * @neighbor_normal: State of neighbor's port's Logical link state
 *	0 - Neighbor Down/Init
 *	1 - Neighbhor LinkArmed/LinkActive
 * @neighbor_fm_security: Configuration of a switch pin which can be used to
 *	disable firmware authentication during switch ASIC boot.
 *	0 - Authenticated
 *	1 - Authentication bypassed
 * @neighbor_port_number: Port number of the neighboring port
 * @is_sm_config_started: indicates that FM has started configuration for this
 *	port.
 * @offline_disabled_reason: offline reason
 * @is_active_optimize_enabled: if enabled, then LinkArmed -> LinkActive state
 *	change propogates this event to neighboring port via SMA Idle Flit
 *	messages
 * @is_vl_marker_enabled: If 1, indicates that explicit interleaving is enabled.
 *      Thus enabling preemption matrix setup by the FM
 * @mgmt_allowed: Indicates if neighbor is allowing this node to be a mgmt node
 *	(information received via LNI)
 * @part_enforce: Partition enforcement flags
 * @sl_to_sc: service level to service class mapping table
 * @sc_to_sl: service class to service level mapping table
 * @sc_to_resp_sl: service class to response service level mapping table
 * @sc_to_vlr: service class to (RX) virtual lane table
 * @sc_to_vlt: service class to (TX) virtual lane table
 * @sc_to_vlnt: service class to (RX) neighbor virtual lane table
 * @sl_to_mctc: service level to traffic class & message class mapping
 * @sc_to_mctc: service class to traffic class & message class mapping
 * @sl_pairs: SL pairs set by the SM. It is indexed by SL request and the
 *      value is SL response. Any SL which is invalid (mapped to SC15 in
 *      the SL2SC table), has no response SL, or is itself a response SL
 *      shall map to 0xFF
 * @sl_mask: Mask of allowed bypass service levels for this port
 * @req_sl_mask: Mask of allowed request service levels for this port
 * @bct: buffer control table
 * @local_link_down_reason: Reason why this port transitioned to link down
 * @local_link_down_reason: Reason why the neighboring port transitioned to
 *	link down
 * @remote_link_down_reason: Value to be sent to link peer on LinkDown
 * @led_override_vals[2]: 0 holds off duration, 1 holds on duration in jiffies
 * @led_override_phase: index into led_override_vals array
 * @led_override_timer_active: Used to track state of timer
 * @led_override_timer: Used to flash the LED in override mode
 * @ccti_entries: List of congestion control table entries
 * @total_cct_entry: Total number of congestion control table entries
 * @cc_sl_control_map: Congestion control Map (Mask)
 * @cc_max_table_entries: CA's max number of 64 entry units in the
 * congestion control table
 * @cc_state_lock: lock for cc_state
 * @cc_state: Congestion control state
 * @congestion_entries: Congestion control entries
 * @cc_log_lock: lock for congestion log related data
 * @threshold_event_counter: maintains a count of congestion events
 * @threshold_cong_event_map: stores bitmask of SLs on which congestion occurred
 * @cc_events: array of congestion log entries
 * @cc_log_idx: index for logging events
 * @cc_mad_idx: index for reporting events
 * @crk8051_lock: for exclusive access to 8051
 * @crk8051_timed_out: remember if the 8051 timed out
 * @hfi_wq: workqueue which connects the upper half interrupt handler,
 *		irq_oc_handler() and bottom half interrupt handlers which are
 *		queued by each work_struct.
 * @link_vc_work: for VerifyCap -> GoingUp/ConfigLT
 * @link_up_work: for GoingUp/ConfigLT -> LinkUp/Init
 * @link_down_work: for LinkUp -> LinkDown
 * @sma_message_work: for the interrupt caused by "Receive a back channel msg
 *		using LCB idle protocol HOST Type SMA"
 * @tmreg_lock: protects the device time registers.
 * @ptp_clock: pointer to PTP internal data
 * @ptp_caps: PTP function pointers
 * @clock_offset: FM provided offset of current master clock from global
 *		fabric time
 * @clock_delay: FM provided propagation delay for time flits between
 *		current master and the slave node
 * @periodicity: FM provided filt update interval
 * @report_fabric_time: Compute new fabric time and report to FM
 * @current_clock_id: FM provided current clock id
 * @is_active_master: This HFI is the timesync master not a switch
 * @ptp_index: Index of /dev/ptp%d
 * @vau: Virtual allocation unit for this port
 * @vcu: Virtual return credit unit for this port
 * @link_credits: link credits of this device
 * @vl15_init: Initial vl15 credits to use
 * @bw_arb_cache: Caching of BW ARB tables
 */
struct hfi_pportdata {
	struct hfi_devdata *dd;
	__be64 guids[HFI2_GUIDS_PER_PORT];
	u64 neighbor_guid;
	u32 lid;
	u32 max_lid;
	struct hfi_ptcdata ptc[HFI_MAX_TC];
	/*
	 * host link state which keeps both Physical Port and Logical Link
	 * state by having HLS_*
	 */
	struct mutex hls_lock;

	/* values for SI tuning of the SerDes */
	u8  max_power_class;
	u8  default_atten;
	u8  local_atten;
	u8  remote_atten;
	u32 rx_preset;
	u32 tx_preset_eq;
	u32 tx_preset_noeq;

	u32 host_link_state;
	u64 lcb_sts_rtt;
	u32 max_active_mtu;
	u32 port_error_action;
	u32 port_type;
	u32 sm_trap_qp;
	u32 sa_qp;
	u32 last_phystate;
	u16 pkeys[HFI_MAX_PKEYS];
	u16 link_width_supported;
	u16 link_width_downgrade_supported;
	u16 link_speed_supported;
	u16 link_width_enabled;
	u16 link_width_downgrade_enabled;
	u16 link_speed_enabled;
	u16 link_width_active;
	u16 link_width_downgrade_tx_active;
	u16 link_width_downgrade_rx_active;
	u16 link_speed_active;
	u16 port_ltp_crc_mode;
	u16 pkey_8b;
	u16 pkey_10b;
	u8 link_enabled;
	u8 packet_format_supported;
	u8 packet_format_enabled;
	u8 linkinit_reason;
	u8 port_crc_mode_enabled;
	u8 lmc;
	u8 pnum;
	u8 vls_supported;
	u8 vls_operational;
	u8 actual_vls_operational;
	u8 neighbor_type;
	u8 neighbor_normal;
	u8 neighbor_fm_security;
	u8 neighbor_port_number;
	u8 is_sm_config_started;
	u8 offline_disabled_reason;
	u8 is_active_optimize_enabled;
	u8 driver_link_ready;
	u8 is_vl_marker_enabled;
	u8 mgmt_allowed;
	u8 part_enforce;
	u8 sl_to_sc[OPA_MAX_SLS];
	u8 sc_to_sl[OPA_MAX_SCS];
	u8 sc_to_resp_sl[OPA_MAX_SCS];
	u8 sc_to_vlr[OPA_MAX_SCS];
	u8 sc_to_vlt[OPA_MAX_SCS];
	u8 sc_to_vlnt[OPA_MAX_SCS];
	u8 sl_to_mctc[OPA_MAX_SLS];
	u8 sc_to_mctc[OPA_MAX_SCS];
	u8 sl_pairs[OPA_MAX_SLS];
	u32 sl_mask;
	u32 req_sl_mask;
	u8 qsfp_retry_count;
	u64 per_sc_becn_count[OPA_MAX_SCS];
	u16 vl_mtu[OPA_MAX_VLS];
	struct hfi_link_down_reason local_link_down_reason;
	struct hfi_link_down_reason neigh_link_down_reason;
	struct buffer_control bct;
	u8 remote_link_down_reason;
	u64 link_downed;
	u8 oc_shutdown;

	/* Used to override LED behavior for things like maintenance beaconing*/
	unsigned long led_override_vals[2];
	u8 led_override_phase;
	atomic_t led_override_timer_active;
	/* Used to flash LEDs in override mode */
	struct timer_list led_override_timer;

	/* Congestion control */
	struct ib_cc_table_entry_shadow ccti_entries[HFI_CC_TABLE_SHADOW_MAX];
	u16 total_cct_entry;
	u32 cc_sl_control_map;
	u8 cc_max_table_entries;

	spinlock_t cc_state_lock ____cacheline_aligned_in_smp;
	spinlock_t cca_timer_lock ____cacheline_aligned_in_smp;
	struct cca_timer cca_timer[OPA_MAX_SLS];
	struct cc_state __rcu *cc_state;
	struct opa_congestion_setting_entry_shadow
		congestion_entries[OPA_MAX_SLS];
	/*
	 * begin congestion log related entries
	 * cc_log_lock protects all congestion log related data
	 */
	spinlock_t cc_log_lock ____cacheline_aligned_in_smp;
	u8 threshold_cong_event_map[OPA_MAX_SLS / 8];
	u16 threshold_event_counter;
	struct opa_hfi_cong_log_event_internal cc_events[OPA_CONG_LOG_ELEMS];
	int cc_log_idx;
	int cc_mad_idx;
	/* end congestion log related entries */

	/* Credit Mgmt/Buffer control */
	u8 vau;
	u8 vcu;
	u16 link_credits;
	u16 vl15_init;

	struct bw_arb_cache bw_arb_cache;

	/* for exclusive access to 8051 */
	spinlock_t crk8051_lock;
	int crk8051_timed_out;	/* remember if the 8051 timed out */

	struct mutex crk8051_mutex;

	/*
	 * workqueue which connects the upper half interrupt handler,
	 * irq_oc_handler() and bottom half interrupt handlers which are
	 * queued by each work_struct.
	 */
	struct workqueue_struct *hfi_wq;
	/* for VerifyCap -> GoingUp/ConfigLT */
	struct work_struct link_vc_work;
	/* for GoingUp/ConfigLT -> LinkUp/Init */
	struct work_struct link_up_work;
	struct work_struct link_down_work; /* for LinkUp -> LinkDown */
	struct delayed_work start_link_work;
	/*
	 * for the interrupt caused by "Receive a back channel msg using
	 * LCB idle protocol HOST Type SMA"
	 */
	struct work_struct sma_message_work;
	/* for LinkUp -> LinkDown -> LinkUp */
	struct work_struct link_bounce_work;
	struct mutex portcntrs_lock;
	/* per port counter values */
	u64 *portcntrs;

	/* Save the enabled LCB error bits */
	u64 lcb_err_en;

	/* for timesync */
	struct mutex timesync_mutex;
	spinlock_t tmreg_lock; /* Used to protect the device time registers. */
	struct ptp_clock *ptp_clock;
	struct ptp_clock_info ptp_caps;
	u64 clock_offset;
	u16 clock_delay;
	u16 periodicity;
	u8 report_fabric_time;
	s16 current_clock_id;
	u8 is_active_master;
	u8 ptp_index;
	u64 xmit_constraint_errors;
	struct err_info_rcvport err_info_rcvport;
	struct err_info_constraint err_info_rcv_constraint;
	struct err_info_constraint err_info_xmit_constraint;
	u8 err_info_uncorrectable;
	u8 err_info_fmconfig;
	u8 hw_lm_pkey;
	u8 hw_ptl_pkey;

	/* PHY support */
	struct qsfp_data qsfp_info;
#ifdef CONFIG_DEBUG_FS
	struct dentry *hfi_port_dbg;
	struct dentry *hfi_neighbor_dbg;
#endif

	struct hfi_i2c_bus *i2c_bus;
};

struct hfi2_netdev;
extern const struct pci_error_handlers hfi_pci_err_handler;

struct hfi_i2c_bus {
	struct hfi_pportdata *ppd;
	struct i2c_adapter adapter;
	struct i2c_algo_bit_data algo;
	int num;
};

#define BOARD_VERS_MAX 96 /* how long the version string can be */

/*
 * device data struct contains only per-HFI info.
 *
 *@fw_mutex: The mutex protects fw_state, fw_err, and all of the
 *		firmware_details variables.
 *@fw_state: keep firmware status, FW_EMPTY, FW_TRY, FW_FINAL, FW_ERR
 *@fw_err: keep firmware loading error code.
 *@fw_8051_name: Firmware file names get set in firmware_init()
 *@fw_8051_load: indicate whether 8051 firmware should be loaded
 *@fw_8051: 8051 firmware loaded in memory
 *@crk8051_ver: 8051 firmware version
 *@platform_config: See pcfg_cache.
 *@pcfg_cache: copy of platform config data/file. hfi2_parse_platform_config()
 *             converts it to platform_config. pcfg_cache is never accessed by
 *             other.
 *@boardname: human readable board info
 *@trig_op_spill_area: Pointer to trigger ops spill area for deallocation
 */
struct hfi_devdata {
	/* pci access data structure */
	struct pci_dev *pdev;
	struct opa_core_ops *core_ops;

	/* localbus width (1, 2,4,8,16,32) from config space  */
	u32 lbus_width;
	/* localbus speed in MHz */
	u32 lbus_speed;
	int unit; /* unit # of this chip */
	int node; /* home node of this chip */
	/* mem-mapped pointer to base of chip regs */
	u8 __iomem *kregbase; /* UC mapped */
	u8 __iomem *kregend;
	u32 wc_off;

	/* IOMMU hack for ZEBU */
	u8 __iomem *iommu_base;

	/* physical address of chip for io_remap, etc. */
	resource_size_t physaddr;

	/* irq vector information for both MSIX and INTx */
	struct hfi_irq_entry *irq_entries;
	u32 num_irq_entries;
	u32 num_eq_irqs;
	atomic_t irq_eq_next;

	/* List of context error queue to dispatch error to */
	spinlock_t error_dispatch_lock;
	struct list_head error_dispatch_head;

	/* RSM */
	u8 rsm_offset[HFI_NUM_RSM_RULES];
	u16 rsm_size[HFI_NUM_RSM_RULES];
	DECLARE_BITMAP(rsm_map, HFI_RSM_MAP_SIZE);

	/* Device Portals State */
	struct idr ptl_user;
	struct idr ptl_tpid;
	DECLARE_BITMAP(ptl_map, HFI_NUM_PIDS);
	u16 mcast_pid;
	u16 vl15_pid;
	u16 bypass_pid;
	spinlock_t ptl_lock;
	struct hfi_ctx priv_ctx;
	struct hfi_cmdq_pair priv_cmdq;

	/* Command Queue State */
	struct idr cmdq_pair;
	spinlock_t cmdq_lock;
	phys_addr_t cmdq_tx_base;
	phys_addr_t cmdq_rx_base;
	void *cmdq_head_base;
	size_t cmdq_head_size;
	void __iomem *cmdq_tx_wc;
	void __iomem *cmdq_rx_wc;

	/* PID Wait State */
	struct idr pid_wait;

	/* QP State */
	void *qp_state_base;
	u32 max_qp;

	/* iommu system pasid */
	u32 system_pasid;

	/* IOMMU hack for ZEBU*/
	void *iommu_excontext_tbl;
	void *iommu_pasid_tbl;

	/* node_guid identifying node */
	__be64 nguid;

	/* Number of physical ports available */
	u8 num_pports;

	/*
	 * hfi_pportdata, points to array of port
	 * port-specifix data structs
	 */
	struct hfi_pportdata *pport;

	/* OUI comes from the HW. Used everywhere as 3 separate bytes. */
	u8 oui[3];

	struct hfi1_snoop_data hfi1_snoop;

	struct hfi2_diagpkt_data hfi2_diag;

	/* Thread to handle PID 0 EQ 0 tasks */
	struct task_struct *eq_zero_thread;

	/* Mutex lock synchronizing E2E operations */
	struct mutex e2e_lock;

	/* E2E EQ handle for initiator events */
	struct hfi_eq e2e_eq;

	/* registered IB device data pointer */
	struct hfi2_ibdev *ibd;

	/* number of portal IDs in use */
	u16 pid_num_assigned;

	/* number of command queues in use */
	u16 cmdq_pair_num_assigned;

	/* for firmware download */
	struct mutex fw_mutex;
	enum fw_state fw_state;
	int fw_err;
	/* Firmware file names get set in firmware_init() */
	char *fw_name;
	bool fw_load;
	struct firmware_details fw_det;
	u32 crk8051_ver; /* 8051 firmware version */

	struct platform_config platform_config;
	struct platform_config_cache pcfg_cache;

	char *boardname; /* human readable board info */

	/* chip minor rev, from CceRevision */
	u8 minrev;

	/* maintain info about device counters */
	size_t devcntrnameslen;
	struct mutex devcntrs_lock;
	u64 *devcntrs;

	/* maintain info about port counters */
	size_t portcntrnameslen;

	/* Pointer to trigger ops spill area for deallocation */
	void *trig_op_spill_area;
#ifdef CONFIG_DEBUG_FS
	/* per HFI debugfs */
	struct dentry *hfi_dev_dbg;
	/* per HFI symlinks to above */
	struct dentry *hfi_dev_link;
#endif
	struct hfi_pend_queue pend_cmdq;
	struct hfi2_netdev *vnic;

	/* hardware ID */
	u8 hfi_id;
	/* Pointer to software device counters */
	u16 *scntrs;
	/* implementation code */
	u8 icode;
	bool emulation;

	/* FXR Address Translation */
	struct hfi_at *at;

	/* number of interrupts handled per cpu*/
	u64 __percpu *int_counter;

	/* driver stats */
	struct hfi2_ib_stats stats;
	/* link manager thread */
	struct task_struct *hfi2_link_mgr;
};

/*
 * FXRTODO: ported from hfi1 -
 * Keep this routine in case we need to perform cpu_to_be64 conversion.
 * hfi1 stores guids in host order, we use network order.
 * Need to resolve as future changes will provide this guid array to rdmavt.
 */
static inline __be64 get_sguid(struct hfi_pportdata *ppd, unsigned int index)
{
	WARN_ON(index >= HFI2_GUIDS_PER_PORT);
	return ppd->guids[index];
}

/*
 * Readers of cc_state must call get_cc_state() under rcu_read_lock().
 */
static inline struct cc_state *hfi_get_cc_state(struct hfi_pportdata *ppd)
{
	return rcu_dereference(ppd->cc_state);
}

/*
 * Called by writers of cc_state only, must call under cc_state_lock.
 */
static inline
struct cc_state *hfi_get_cc_state_protected(struct hfi_pportdata *ppd)
{
	return rcu_dereference_protected(ppd->cc_state,
					 lockdep_is_held(&ppd->cc_state_lock));
}

/* Control LED state */
inline void hfi_set_ext_led(struct hfi_pportdata *ppd, u32 on);

void __init hfi_mod_init(void);
void hfi_mod_deinit(void);

struct hfi_devdata *hfi_get_unit_dd(int unit);
int hfi_pci_init(struct pci_dev *pdev, const struct pci_device_id *ent);
void hfi_pci_cleanup(struct pci_dev *pdev);
struct hfi_devdata *hfi_pci_dd_init(struct pci_dev *pdev,
				    const struct pci_device_id *ent);
void hfi_pci_dd_free(struct hfi_devdata *dd);
int hfi_enable_msix(struct hfi_devdata *dd, u32 *nent);
void hfi_disable_msix(struct hfi_devdata *dd);
int hfi_setup_interrupts(struct hfi_devdata *dd, int total);
void hfi_cleanup_interrupts(struct hfi_devdata *dd);
void hfi_disable_interrupts(struct hfi_devdata *dd);

int hfi_setup_errd_irq(struct hfi_devdata *dd);
irqreturn_t hfi_irq_errd_handler(int irq, void *dev_id);

void hfi_setup_errq(struct hfi_ctx *ctx, struct opa_ctx_assign *ctx_assign);
void hfi_errq_cleanup(struct hfi_ctx *ctx);
int hfi_get_async_error(struct hfi_ctx *ctx, void *ae, int timeout);

struct hfi_devdata *hfi_alloc_devdata(struct pci_dev *pdev);

void hfi_start_led_override(struct hfi_pportdata *ppd, unsigned int time_on,
			    unsigned int time_off);
void hfi_shutdown_led_override(struct hfi_pportdata *ppd);

/* HFI specific functions */
int hfi_cmdq_assign_privileged(struct hfi_cmdq_pair *cmdq, struct hfi_ctx *ctx);
int hfi_eq_assign(struct hfi_ctx *ctx, struct opa_ev_assign *ev_assign);
int hfi_eq_release(struct hfi_ctx *ctx, u16 eq_idx, u64 user_data);
int hfi_eq_zero_assign(struct hfi_ctx *ctx);
void hfi_eq_zero_release(struct hfi_ctx *ctx);
irqreturn_t hfi_irq_eq_handler(int irq, void *dev_id);
void hfi_event_cleanup(struct hfi_ctx *ctx);
void hfi_cmdq_cleanup(struct hfi_ctx *ctx);
void hfi_cmdq_config(struct hfi_ctx *ctx, u16 cmdq_idx,
		     struct hfi_auth_tuple *auth_table, bool user_priv);
void hfi_cmdq_config_tuples(struct hfi_ctx *ctx, u16 cmdq_idx,
			    struct hfi_auth_tuple *auth_table);
void hfi_cmdq_disable(struct hfi_devdata *dd, u16 cmdq_idx);
void __hfi_cmdq_config_all(struct  hfi_devdata *dd);
void hfi_pcb_write(struct hfi_ctx *ctx, u16 ptl_pid);
void hfi_pcb_reset(struct hfi_devdata *dd, u16 ptl_pid);
void hfi_tpid_enable(struct hfi_devdata *dd, u8 idx, u16 base, u32 ptl_uid);
void hfi_tpid_disable(struct hfi_devdata *dd, u8 idx);
int hfi_alloc_spill_area(struct hfi_devdata *dd);
void hfi_free_spill_area(struct hfi_devdata *dd);

int hfi_at_is_pasid_valid(struct device *dev, int pasid);
int hfi_at_set_pasid(struct hfi_ctx *ctx);
int hfi_at_clear_pasid(struct hfi_ctx *ctx);
int hfi_at_setup_irq(struct hfi_devdata *dd);
int hfi_at_init(struct hfi_devdata *dd);
void hfi_at_exit(struct hfi_devdata *dd);
int hfi_at_reg_range(struct hfi_ctx *ctx, void *addr, u32 size,
		     struct page **pages, bool write);
void hfi_at_dereg_range(struct hfi_ctx *ctx, void *addr, u32 size);
int hfi_at_prefetch(struct hfi_ctx *ctx,
		    struct hfi_at_prefetch_args *atpf);

/* OPA core functions */
int hfi_cmdq_assign(struct hfi_cmdq_pair *cmdq, struct hfi_ctx *ctx,
		    struct hfi_auth_tuple *auth_table);
int hfi_cmdq_update(struct hfi_cmdq_pair *cmdq,
		    struct hfi_auth_tuple *auth_table);
int hfi_cmdq_release(struct hfi_cmdq_pair *cmdq);
int hfi_cmdq_map(struct hfi_cmdq_pair *cmdq);
void hfi_cmdq_unmap(struct hfi_cmdq_pair *cmdq);
int hfi_pt_update_lower(struct hfi_ctx *ctx, u8 ni, u32 pt_idx,
			u64 *val, u64 user_data);
int hfi_cteq_assign(struct hfi_ctx *ctx, struct opa_ev_assign *ev_assign);
int hfi_cteq_release(struct hfi_ctx *ctx, u16 eq_mode, u16 eq_idx,
		     u64 user_data);
int hfi_ev_wait_single(struct hfi_ctx *ctx, u16 eqflag, u16 ev_idx,
		       int timeout, u64 *user_data0, u64 *user_data1);
int hfi_ev_set_channel(struct hfi_ctx *ctx, u16 ec_idx, u16 ev_idx,
		       u64 eq_handle, u64 user_data);
int hfi_ec_wait_event(struct hfi_ctx *ctx, u16 ec_idx, int timeout,
		      u64 *user_data0, u64 *user_data1);
int hfi_ib_cq_arm(struct hfi_ctx *ctx, struct ib_cq *ibcq, u8 solicit);
int hfi_ib_eq_arm(struct hfi_ctx *ctx, struct ib_cq *ibcq,
		  struct hfi_ibeq *ibeq, u8 solicit);
int hfi_eq_ack_event(struct hfi_ctx *ctx, u16 eq_idx, u32 nevents);
int hfi_ct_ack_event(struct hfi_ctx *ctx, u16 ct_idx, u32 nevents);
int hfi_ec_assign(struct hfi_ctx *ctx, u16 *ec_idx);
int hfi_ec_release(struct hfi_ctx *ctx, u16 ec_idx);
void hfi_ctx_init(struct hfi_ctx *ctx, struct hfi_devdata *dd);
int hfi_ctx_attach(struct hfi_ctx *ctx, struct opa_ctx_assign *ctx_assign);
int hfi_ctx_cleanup(struct hfi_ctx *ctx);
int hfi_ctx_release(struct hfi_ctx *ctx);
int hfi_ctx_reserve(struct hfi_ctx *ctx, u16 *base, u16 count, u16 align,
		    u16 flags);
int hfi_get_hw_limits(struct hfi_ibcontext *uc, struct hfi_hw_limit *hwl);
void hfi_ctx_unreserve(struct hfi_ctx *ctx);
int hfi_ctx_hw_addr(struct hfi_ctx *ctx, int token, u16 ctxt, void **addr,
		    ssize_t *len);
void hfi_ctx_clear_bypass(struct hfi_ctx *ctx);
int hfi_ctx_set_bypass(struct hfi_ctx *ctx, u8 hdr_size);
void hfi_ctx_set_mcast(struct hfi_ctx *ctx);
void hfi_ctx_set_vl15(struct hfi_ctx *ctx);
void hfi_ctx_set_qp_table(struct hfi_devdata *dd, struct hfi_ctx *rx_ctx[],
			  u16 num_contexts);
#ifdef CONFIG_HFI2_STLNP
int hfi_ctx_set_allowed_uids(struct hfi_ctx *ctx, u32 *auth_uid, u8 num_uids);
int hfi_ctx_set_virtual_pid_range(struct hfi_ctx *ctx);
void hfi_ctx_unset_virtual_pid_range(struct hfi_devdata *dd,
				     struct hfi_job_res *res);
int hfi_dlid_assign(struct hfi_ctx *ctx,
		    struct hfi_dlid_assign_args *dlid_assign);
int hfi_dlid_release(struct hfi_ctx *ctx);
int hfi_e2e_start(struct hfi_ctx *ctx);
void hfi_e2e_stop(struct hfi_ctx *ctx);
void hfi_e2e_init(struct hfi_pportdata *ppd);
void hfi_e2e_destroy_all(struct hfi_devdata *dd);
void hfi_e2e_destroy_old_pkeys(struct hfi_pportdata *ppd, u16 *pkeys);
int hfi_e2e_ctrl(struct hfi_ibcontext *uc, struct hfi_e2e_conn *e2e_conn);
u32 hfi_e2e_key_to_dlid(int key);
u32 hfi_e2e_key_to_pkey_id(int key);
u16 hfi_e2e_key_to_pkey(struct hfi_pportdata *ppd, u32 key);
u8 hfi_e2e_cache_data_to_sl(u64 *data);
u16 hfi_e2e_cache_data_to_pkey(u64 *data);
u32 hfi_e2e_cache_data_to_slid(u64 *data);
#endif
void hfi_rsm_clear_rule(struct hfi_devdata *dd, u8 rule_idx);
int hfi_rsm_set_rule(struct hfi_devdata *dd, struct hfi_rsm_rule *rule,
		     struct hfi_ctx *rx_ctx[], u16 num_contexts);
int hfi_vnic_init(struct hfi_devdata *dd);
void hfi_vnic_uninit(struct hfi_devdata *dd);
u16 hfi_generate_jkey(void);

static inline struct hfi_pportdata *to_hfi_ppd(const struct hfi_devdata *dd,
					       u8 port)
{
	u8 pidx = port - 1; /* IB number port from 1, hdw from 0 */

	if (pidx >= dd->num_pports)
		return NULL;

	return &dd->pport[pidx];
}

static inline void hfi_set_bits(u64 *target, u64 val, u64 mask, u8 shift)
{
	/* clear */
	*target &=  ~(mask << shift);
	/* set */
	*target |= (val & mask) << shift;
}

static inline u8 hfi_parity(u16 val)
{
	int i, m = 0;

	/* m is 1 for odd parity */
	for (i = 0; i < sizeof(val) * 8; i++, val >>= 1)
		m ^= (val & 0x1);

	return m;
}

/*
 * ingress_pkey_matches_entry - return true if the pkey matches ent (ent
 * being an entry from the ingress partition key table), return false
 * otherwise. Use the matching criteria for ingress partition keys
 * specified in the OPAv2 spec., section 9.11.14.
 */
static inline bool ingress_pkey_matches_entry(u16 pkey, u16 ent)
{
	u16 mkey = pkey & HFI_PKEY_CAM_MASK;
	u16 m_ent = ent & HFI_PKEY_CAM_MASK;

	if (mkey == m_ent) {
		/*
		 *  If pkey[15] is clear (limited partition member),
		 *  is bit 15 in the corresponding table element
		 *  clear (limited member)?
		 */
		if (!(HFI_PKEY_MEMBER_TYPE(pkey)))
			return !!(HFI_PKEY_MEMBER_TYPE(ent));
		return true;
	}
	return false;
}

/*
 * ingress_pkey_table_search - search the entire pkey table for
 * an entry which matches 'pkey'. return false if a match is found,
 * and true otherwise.
 */
static bool ingress_pkey_table_search(struct hfi_pportdata *ppd, u16 pkey)
{
	int i;

	for (i = 0; i < HFI_MAX_PKEYS; i++) {
		if (ingress_pkey_matches_entry(pkey, ppd->pkeys[i]))
			return false;
	}
	return true;
}

/*
 * ingress_pkey_table_fail - record a failure of ingress pkey validation,
 */
static void ingress_pkey_table_fail(struct hfi_pportdata *ppd, u16 pkey,
				    u32 slid)
{
	if (!(ppd->err_info_rcv_constraint.status & OPA_EI_STATUS_SMASK)) {
		ppd->err_info_rcv_constraint.status |= OPA_EI_STATUS_SMASK;
		ppd->err_info_rcv_constraint.slid = slid;
		ppd->err_info_rcv_constraint.pkey = pkey;
	}
}

/*
 *   ingress_pkey_check - Return false if the ingress pkey is valid, return true
 *   otherwise. Use the criteria in the OPAv2 spec, section 9.11.14. idx
 *   is a hint as to the best place in the partition key table to begin
 *   searching. This function should not be called on the data path because
 *   of performance reasons. On datapath pkey check is expected to be done
 *   by HW and rcv_pkey_check function should be called instead.
 */
static inline bool ingress_pkey_check(struct hfi_pportdata *ppd, u16 pkey,
				      u8 sc5, u8 idx, u16 slid)
{
	/* If SC15, pkey[0:14] must be 0x7fff */
	if ((sc5 == 0xf) && ((pkey & HFI_PKEY_CAM_MASK) != HFI_PKEY_CAM_MASK))
		goto bad;

	/* Is the pkey = 0x0, or 0x8000? */
	if ((pkey & HFI_PKEY_CAM_MASK) == 0)
		goto bad;

	/* The most likely matching pkey has index 'idx' */
	if (ingress_pkey_matches_entry(pkey, ppd->pkeys[idx]))
		return false;

	/* no match - try the whole table */
	if (!ingress_pkey_table_search(ppd, pkey))
		return false;

bad:
	ingress_pkey_table_fail(ppd, pkey, slid);
	return true;
}

/*
 *  rcv_pkey_check - Return false if the ingress pkey is valid, return true
 *  otherwise. It only ensures pkey is vlid for QP0. This function
 *  should be called on the data path instead of ingress_pkey_check
 *  as on data path, pkey check is done by HW (except for QP0).
 */

static inline bool hfi_rcv_pkey_check(struct hfi_pportdata *ppd, u16 pkey,
				      u8 sc5, u16 slid)
{
	/* If SC15, pkey[0:14] must be 0x7fff */
	if ((sc5 == 0xf) && ((pkey & HFI_PKEY_CAM_MASK) != HFI_PKEY_CAM_MASK))
		goto bad;

	return false;
bad:
	ingress_pkey_table_fail(ppd, pkey, slid);
	return true;
}

int hfi_get_sma(struct hfi_devdata *dd, u16 attr_id, struct opa_smp *smp,
		u32 am, u8 *data, u8 port, u32 *resp_len, u8 *sma_status);
int hfi_set_sma(struct hfi_devdata *dd, u16 attr_id, struct opa_smp *smp,
		u32 am, u8 *data, u8 port, u32 *resp_len, u8 *sma_status);

void hfi_set_link_down_reason(struct hfi_pportdata *ppd, u8 lcl_reason,
			      u8 neigh_reason, u8 rem_reason);
void hfi_apply_link_downgrade_policy(struct hfi_pportdata *ppd,
				     int refresh_widths);
int hfi_set_link_state(struct hfi_pportdata *ppd, u32 state);
u32 hfi_driver_pstate(struct hfi_pportdata *ppd);
u32 hfi_driver_lstate(struct hfi_pportdata *ppd);
void hfi_ack_interrupt(struct hfi_irq_entry *me);
u8 hfi_porttype(struct hfi_pportdata *ppd);
int hfi_get_sl_for_mctc(struct hfi_pportdata *ppd, u8 mc, u8 tc, u8 *sl);
int hfi_get_ib_cfg(struct hfi_pportdata *ppd, int which, u32 val, void *data);
int hfi_set_ib_cfg(struct hfi_pportdata *ppd, int which, u32 val, void *data);
int hfi_set_mtu(struct hfi_pportdata *ppd);
u16 hfi_port_ltp_to_cap(u16 port_ltp);
u16 hfi_port_cap_to_lcb(struct hfi_devdata *dd, u16 crc_mask);
u16 hfi_cap_to_port_ltp(u16 cap);
void hfi_set_crc_mode(struct hfi_pportdata *ppd, u16 crc_lcb_mode);
bool hfi_is_req_sl(struct hfi_pportdata *ppd, u8 sl);
int hfi_check_sl_pair(struct hfi_ibcontext *uc, struct hfi_sl_pair *slp);
void hfi_get_buffer_control(struct hfi_pportdata *ppd,
			    struct buffer_control *bc, u16 *overall_limit);
int hfi_set_buffer_control(struct hfi_pportdata *ppd,
			   struct buffer_control *new_bc);
void hfi_write_qp_state_csrs(struct hfi_devdata *dd, void *qp_state_base,
			     u32 max_qpn);
void hfi_cfg_reset_pmon_cntrs(struct hfi_devdata *dd);
void hfi_reset_pma_perf_cntrs(struct hfi_pportdata *ppd);
void hfi_reset_pma_error_cntrs(struct hfi_pportdata *ppd);
void hfi_cfg_out_pkey_check(struct hfi_pportdata *ppd, u8 enable);
void hfi_cfg_in_pkey_check(struct hfi_pportdata *ppd, u8 enable);
void hfi_cfg_lm_pkey_check(struct hfi_pportdata *ppd, u8 enable);
void hfi_cfg_ptl_pkey_check(struct hfi_pportdata *ppd, u8 enable);
void hfi_cfg_pkey_check(struct hfi_pportdata *ppd, u8 enable);
void hfi_set_up_vl15(struct hfi_pportdata *ppd, u8 vau, u16 vl15buf);
void hfi_assign_remote_cm_au_table(struct hfi_pportdata *ppd, u8 vcu);
int neigh_is_hfi(struct hfi_pportdata *ppd);
void hfi_add_full_mgmt_pkey(struct hfi_pportdata *ppd);
void hfi_clear_full_mgmt_pkey(struct hfi_pportdata *ppd);
const char *hfi_class_name(void);
int hfi_set_lid(struct hfi_pportdata *ppd, u32 lid, u8 lmc);
int hfi_set_max_lid(struct hfi_pportdata *ppd, u32 lid);
void hfi_psn_uninit(struct hfi_pportdata *port);
void hfi_write_lm_tp_prf_csr(const struct hfi_pportdata *ppd,
			     u32 offset, u64 value);
void hfi_write_lm_fpc_prf_per_vl_csr(const struct hfi_pportdata *ppd,
				     u32 offset, u32 index, u64 value);
u64 hfi_read_lm_fpc_prf_per_vl_csr(const struct hfi_pportdata *ppd,
				   u32 offset, u32 index);
u64 hfi_read_lm_tp_prf_csr(const struct hfi_pportdata *ppd,
			   u32 offset);
u64 hfi_read_pmon_csr(const struct hfi_devdata *dd,
		      u32 index);
int hfi_snoop_add(struct hfi_devdata *dd);
void hfi_snoop_remove(struct hfi_devdata *dd);
int hfi2_diag_init(struct hfi_devdata *dd);
void hfi2_diag_uninit(struct hfi_devdata *dd);
int hfi2_diag_add(void);
void hfi2_diag_remove(void);
irqreturn_t hfi_irq_becn_handler(int irq, void *dev_id);
u64 hfi_read_per_cpu_counter(u64 *z_val, u64 __percpu *cntr);
struct net_device *hfi2_vnic_alloc_rn(struct ib_device *device,
				      u8 port_num,
				      enum rdma_netdev_t type,
				      const char *name,
				      unsigned char name_assign_type,
				      void (*setup)(struct net_device *));

int set_qsfp_tx(struct hfi_pportdata *ppd, int on);
int hfi_ib_cq_disarm_irq(struct ib_cq *ibcq);
int hfi_ib_cq_armed(struct ib_cq *ibcq);
int hfi_ib_eq_add(struct hfi_ctx *ctx, struct ib_cq *cq, u64 disarm_done,
		  u64 arm_done, u16 eq_idx);
int hfi_ib_eq_release(struct hfi_ctx *ctx, struct ib_cq *cq, u16 eq_idx);

/*
 * dev_err can be used (only!) to print early errors before devdata is
 * allocated, or when dd->pdev may not be valid, and at the tail end of
 * cleanup when devdata may have been freed, etc.
 * Otherwise these macros below are the preferred ones.
 */
#define dd_dev_err(dd, fmt, ...) \
	dev_err(&(dd)->pdev->dev, "hfi2_%d: " fmt, \
		((dd)->unit), ##__VA_ARGS__)

#define dd_dev_info(dd, fmt, ...) \
	dev_info(&(dd)->pdev->dev, "hfi2_%d: " fmt, \
		((dd)->unit), ##__VA_ARGS__)

#define dd_dev_dbg(dd, fmt, ...) \
	dev_dbg(&(dd)->pdev->dev, "hfi2_%d: " fmt, \
		((dd)->unit), ##__VA_ARGS__)

#define dd_dev_warn(dd, fmt, ...) \
	dev_warn(&(dd)->pdev->dev, "hfi2_%d: " fmt, \
		((dd)->unit), ##__VA_ARGS__)

#define dd_dev_warn_ratelimited(dd, fmt, ...) \
	dev_warn_ratelimited(&(dd)->pdev->dev, "hfi2_%d: " fmt, \
		((dd)->unit), ##__VA_ARGS__)

#define ppd_dev_err(ppd, fmt, ...) \
	dev_err(&(ppd)->dd->pdev->dev, "hfi2_%d: port %u: " fmt,	\
		((ppd)->dd->unit), (ppd->pnum), ##__VA_ARGS__)

#define ppd_dev_info(ppd, fmt, ...) \
	dev_info(&(ppd)->dd->pdev->dev, "hfi2_%d: port %u: " fmt,	\
		((ppd)->dd->unit), (ppd->pnum), ##__VA_ARGS__)

#define ppd_dev_dbg(ppd, fmt, ...) \
	dev_dbg(&(ppd)->dd->pdev->dev, "hfi2_%d: port %u: " fmt,	\
		((ppd)->dd->unit), (ppd->pnum), ##__VA_ARGS__)

#define ppd_dev_warn(ppd, fmt, ...) \
	dev_warn(&(ppd)->dd->pdev->dev, "hfi2_%d: port %u: " fmt,	\
		((ppd)->dd->unit), (ppd->pnum), ##__VA_ARGS__)

#ifndef BIT_ULL
#define BIT_ULL(nr)             (1ULL << (nr))
#endif

/* printk wrappers (pr_warn, etc) can also be used for general debugging. */
#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

/* FXRTODO: Hacks for early bring up. Delete before upstreaming */
extern bool quick_linkup;
extern bool no_mnh;
extern bool no_pe_fw;
extern bool simics;
extern bool opafm_disable;
extern bool no_interrupts;
extern bool pid_reuse;

static inline u64 read_csr(const struct hfi_devdata *dd, u32 offset)
{
	u64 val;

	if (likely(offset >= dd->wc_off)) {
		val = readq(dd->kregbase + offset - dd->wc_off);
	} else {
		WARN_ON(1);
		return (u64)-1;
	}

	return val;
}

static inline void write_csr(const struct hfi_devdata *dd, u32 offset,
			     u64 value)
{
	if (likely(offset >= dd->wc_off)) {
		writeq(value, dd->kregbase + offset - dd->wc_off);
	} else {
		WARN_ON(1);
		return;
	}

	if (dd->emulation)
		read_csr(dd, offset);
}

/* ZEBU specific workarounds. These should be deleted as necessary */
int hfi_zebu_enable_ats(const struct hfi_devdata *dd);
int hfi_set_pasid(struct hfi_devdata *dd, struct hfi_ctx *ctx, u16 ptl_pid);
void hfi_read_lm_link_state(const struct hfi_pportdata *ppd);
void hfi_zebu_hack_default_mtu(struct hfi_pportdata *ppd);

void hfi_psn_cache_fill(struct hfi_devdata *dd);

/* FXRTODO: Can be removed once HSD 1407227170 is verified */
static inline void msix_addr_test(const struct hfi_devdata *dd)
{
	u32 offset = 0x1a82000;

	msleep(100);

	pr_err("initial value of HFI_PCIM_MSIX_ADDR offset 0x%x \
		value 0x%x offset 0x%x value 0x%x\n",
		offset,
		readl(dd->kregbase + offset - dd->wc_off),
		offset + 4,
		readl(dd->kregbase + offset + 4 - dd->wc_off));

	writel(0xFEE00000, dd->kregbase + offset - dd->wc_off);
	writel(0x0, dd->kregbase + offset + 4 - dd->wc_off);

	pr_err("after writing 0xabcd to offset 0x%x value 0x%x \
		offset 0x%x value 0x%x\n",
		offset,
		readl(dd->kregbase + offset - dd->wc_off),
		offset + 4,
		readl(dd->kregbase + offset + 4 - dd->wc_off));
}

/**
 * wait_woken_event_interruptible_timeout - sleep until a condition
 * gets true or a timeout elapses
 * @wq: the waitqueue to wait on
 * @condition: a C expression for the event to wait for
 * @timeout: timeout, in jiffies
 *
 * Waits until the @condition evaluates to true or a signal is received.
 * The @condition is checked each time the waitqueue @wq is woken up.
 *
 * wake_up() has to be called after changing any variable that could
 * change the result of the wait condition.
 *
 * Returns:
 * 0 if the @condition evaluated to %false after the @timeout elapsed,
 * 1 if the @condition evaluated to %true after the @timeout elapsed,
 * the remaining jiffies (at least 1) if the @condition evaluated
 * to %true before the @timeout elapsed, or -%ERESTARTSYS if it was
 * interrupted by a signal.
 */

#define wait_woken_event_interruptible_timeout(wq, condition, timeout) \
({                                                                     \
	long __ret = timeout;                                          \
	bool __cond;						       \
	DEFINE_WAIT_FUNC(wait, woken_wake_function);                   \
	might_sleep();						       \
	add_wait_queue(&wq, &wait);                                    \
	while (!(__cond = condition) && (__ret > 0)) {		       \
		if (signal_pending(current))                           \
			__ret = -ERESTARTSYS;                          \
		else                                                   \
			__ret = wait_woken(&wait,                      \
					   TASK_INTERRUPTIBLE, __ret); \
	}                                                              \
	remove_wait_queue(&wq, &wait);                                 \
	if (__cond && !__ret)					       \
		__ret = 1;					       \
	__ret;                                                         \
})

#endif
