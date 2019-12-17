/*
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 *   redistributing this file, you may do so under either license.
 *
 *   GPL LICENSE SUMMARY
 *
 *   Copyright(c) 2019 Intel Corporation. All rights reserved.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of version 2 of the GNU General Public License as
 *   published by the Free Software Foundation.
 *
 *   BSD LICENSE
 *
 *   Copyright(c) 2019 Intel Corporation. All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copy
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _NTB_INTEL_GEN4_H_
#define _NTB_INTEL_GEN4_H_

#include "ntb_hw_intel.h"

/* Intel Gen4 NTB hardware */
/* PCIe config space */
#define GEN4_IMBAR23SZ_OFFSET		0x00c4
#define GEN4_IMBAR45SZ_OFFSET		0x00c5
#define GEN4_EMBAR23SZ_OFFSET		0x00c6
#define GEN4_EMBAR45SZ_OFFSET		0x00c7
#define GEN4_DEVCTRL_OFFSET		0x0048
#define GEN4_DEVSTS_OFFSET		0x004a
#define GEN4_UNCERRSTS_OFFSET		0x0104
#define GEN4_CORERRSTS_OFFSET		0x0110

/* BAR0 MMIO */
#define GEN4_NTBCNTL_OFFSET		0x0000
#define GEN4_IM23XBASE_OFFSET		0x0010	/* IMBAR1XBASE */
#define GEN4_IM23XLMT_OFFSET		0x0018  /* IMBAR1XLMT */
#define GEN4_IM45XBASE_OFFSET		0x0020	/* IMBAR2XBASE */
#define GEN4_IM45XLMT_OFFSET		0x0028  /* IMBAR2XLMT */
#define GEN4_IM_INT_STATUS_OFFSET	0x0040
#define GEN4_IM_INT_DISABLE_OFFSET	0x0048
#define GEN4_INTVEC_OFFSET		0x0050  /* 0-32 vecs */
#define GEN4_IM23XBASEIDX_OFFSET	0x0074
#define GEN4_IM45XBASEIDX_OFFSET	0x0076
#define GEN4_IM_SPAD_OFFSET		0x0080  /* 0-15 SPADs */
#define GEN4_IM_SPAD_SEM_OFFSET		0x00c0	/* SPAD hw semaphore */
#define GEN4_IM_SPAD_STICKY_OFFSET	0x00c4  /* sticky SPAD */
#define GEN4_IM_DOORBELL_OFFSET		0x0100  /* 0-31 doorbells */
#define GEN4_EM_SPAD_OFFSET		0x8080
/* note, link status is now in MMIO and not config space for NTB */
#define GEN4_LINK_CTRL_OFFSET		0xb050
#define GEN4_LINK_STATUS_OFFSET		0xb052
#define GEN4_PPD0_OFFSET		0xb0d4
#define GEN4_PPD1_OFFSET		0xb4c0
#define GEN4_LTSSMSTATEJMP		0xf040

#define GEN4_PPD_CLEAR_TRN		0x0001
#define GEN4_PPD_LINKTRN		0x0008
#define GEN4_PPD_CONN_MASK		0x0300
#define GEN4_PPD_CONN_B2B		0x0200
#define GEN4_PPD_DEV_MASK		0x1000
#define GEN4_PPD_DEV_DSD		0x1000
#define GEN4_PPD_DEV_USD		0x0000
#define GEN4_LINK_CTRL_LINK_DISABLE	0x0010

#define GEN4_SLOTSTS			0xb05a
#define GEN4_SLOTSTS_DLLSCS		0x100

#define GEN4_PPD_TOPO_MASK	(GEN4_PPD_CONN_MASK | GEN4_PPD_DEV_MASK)
#define GEN4_PPD_TOPO_B2B_USD	(GEN4_PPD_CONN_B2B | GEN4_PPD_DEV_USD)
#define GEN4_PPD_TOPO_B2B_DSD	(GEN4_PPD_CONN_B2B | GEN4_PPD_DEV_DSD)

#define GEN4_DB_COUNT			32
#define GEN4_DB_LINK			32
#define GEN4_DB_LINK_BIT		BIT_ULL(GEN4_DB_LINK)
#define GEN4_DB_MSIX_VECTOR_COUNT	33
#define GEN4_DB_MSIX_VECTOR_SHIFT	1
#define GEN4_DB_TOTAL_SHIFT		33
#define GEN4_SPAD_COUNT			16

#define NTB_CTL_E2I_BAR23_SNOOP		0x000004
#define NTB_CTL_E2I_BAR23_NOSNOOP	0x000008
#define NTB_CTL_I2E_BAR23_SNOOP		0x000010
#define NTB_CTL_I2E_BAR23_NOSNOOP	0x000020
#define NTB_CTL_E2I_BAR45_SNOOP		0x000040
#define NTB_CTL_E2I_BAR45_NOSNOO	0x000080
#define NTB_CTL_I2E_BAR45_SNOOP		0x000100
#define NTB_CTL_I2E_BAR45_NOSNOOP	0x000200
#define NTB_CTL_BUSNO_DIS_INC		0x000400
#define NTB_CTL_LINK_DOWN		0x010000

#define NTB_SJC_FORCEDETECT		0x000004

ssize_t ndev_ntb4_debugfs_read(struct file *filp, char __user *ubuf,
				      size_t count, loff_t *offp);
int gen4_init_dev(struct intel_ntb_dev *ndev);
ssize_t ndev_ntb4_debugfs_read(struct file *filp, char __user *ubuf,
				      size_t count, loff_t *offp);

extern const struct ntb_dev_ops intel_ntb4_ops;

#endif
