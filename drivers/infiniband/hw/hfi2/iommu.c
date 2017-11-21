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

/*
 * Routines below manage setting IOMMU tables for DMA remapping hardware
 * for w/PASID address translations.
 * The hierarchy here of tables (root, context, pasid) is described in the
 * Intel VT-d specfication.
 *
 * a) The iommu_root_table is a page with entries of pointers to context
 *    tables for each PCIe bus.
 * b) The context table is a page of entries with pointers to PASID table
 *    for each device-function attached to a PCIe bus.
 * c) The PASID table contains entries with pointers to the root of the
 *    first level paging structures (base of PML4) for a process context.
 *
 * The page table for page walks is then found with:
 * Root[pci_bus]->Context[pci_device_func]->PASID[process_address_space_id]
 */

#include <linux/kthread.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/intel-svm.h>
#include "chip/fxr_top_defs.h"
#include "chip/fxr_at_csrs.h"
#include "chip/fxr_at_defs.h"
#include "hfi2.h"

static inline
uint64_t hfi_iommu_system_pasid_enable(int id)
{
	u64 val;

	val = FXR_AT_CFG_USE_SYSTEM_PASID_ENABLE_SMASK |
		((FXR_AT_CFG_USE_SYSTEM_PASID_SYSTEM_PASID_MASK & id) <<
		FXR_AT_CFG_USE_SYSTEM_PASID_SYSTEM_PASID_SHIFT);

	return val;
}

int
hfi_iommu_set_pasid(struct hfi_ctx *ctx)
{
	struct hfi_devdata *dd = ctx->devdata;
	struct device *dev = &dd->pdev->dev;
	int ret, pasid, flags = 0;
	u64 lut = 0;

	if (ctx->type != HFI_CTX_TYPE_USER)
		flags |= SVM_FLAG_SUPERVISOR_MODE;

	/*
	 * Since we don't set SVM_FLAG_PRIVATE_PASID, all kernel entities
	 * will share the same PASID.
	 */
	ret = intel_svm_bind_mm(dev, &pasid, flags, NULL);
	if (ret)
		return ret;

	/*
	 * Save pasid for cleanup.
	 */
	ctx->pasid = pasid;
	pr_err("%s %d pasid %d system_pasid %d pid %d flags 0x%x type %d\n",
	       __func__, __LINE__, pasid, dd->system_pasid, ctx->pid,
	       flags, ctx->type);

	/*
	 * Set privileged mode for non-user contxt,
	 * also set system pasid for pid=0.
	 */
	if (ctx->type != HFI_CTX_TYPE_USER) {
		if (ctx->pid == HFI_PID_SYSTEM) {
			u64 sys = hfi_iommu_system_pasid_enable(pasid);

			write_csr(dd, FXR_AT_CFG_USE_SYSTEM_PASID, sys);

			dd->system_pasid = pasid;
		} else {
			WARN_ON(dd->system_pasid != pasid);
		}

		lut &= ~FXR_AT_CFG_PASID_LUT_PRIVILEGE_LEVEL_SMASK;
	} else {
		lut |= FXR_AT_CFG_PASID_LUT_PRIVILEGE_LEVEL_SMASK;
	}

	lut |= FXR_AT_CFG_PASID_LUT_ENABLE_SMASK |
		((FXR_AT_CFG_PASID_LUT_PASID_MASK & pasid) <<
		FXR_AT_CFG_PASID_LUT_PASID_SHIFT);

	write_csr(dd, FXR_AT_CFG_PASID_LUT + (ctx->pid * 8), lut);

	return ret;
}

int
hfi_iommu_clear_pasid(struct hfi_ctx *ctx)
{
	struct hfi_devdata *dd = ctx->devdata;
	struct device *dev = &dd->pdev->dev;
	u64 lut = 0;
	u64 drain_pasid;
	int time, ms;

	/* disable pid->pasid translation */
	lut &= ~FXR_AT_CFG_PASID_LUT_ENABLE_SMASK;
	write_csr(dd, FXR_AT_CFG_PASID_LUT + (ctx->pid * 8), lut);

	if (ctx->type != HFI_CTX_TYPE_USER) {
		if (ctx->pid == HFI_PID_SYSTEM)
			write_csr(dd, FXR_AT_CFG_USE_SYSTEM_PASID, 0);
	}

	spin_lock(&dd->ptl_lock);

	/* drain pasid before freeing it */
	drain_pasid = ((FXR_AT_CFG_DRAIN_PASID_PASID_MASK & ctx->pasid) <<
			FXR_AT_CFG_DRAIN_PASID_PASID_SHIFT) |
			FXR_AT_CFG_DRAIN_PASID_BUSY_SMASK;
	write_csr(dd, FXR_AT_CFG_DRAIN_PASID, drain_pasid);

	/* wait for completion, if timeout log a message */
	ms = 1;
	for (time = 0; time < HFI_PASID_DRAIN_TIMEOUT_MS; time += ms) {
		drain_pasid = read_csr(dd, FXR_AT_CFG_DRAIN_PASID);

		if (!(drain_pasid & FXR_AT_CFG_DRAIN_PASID_BUSY_SMASK))
			break;

		mdelay(ms);
		ms *= 2;	/* double the waiting time */
	}
	if (time >= HFI_PASID_DRAIN_TIMEOUT_MS)
		dd_dev_err(dd, "PASID draining not done after %dms, pid %d\n",
			   time, ctx->pid);

	spin_unlock(&dd->ptl_lock);

	/* call into svm to free pasid */
	return intel_svm_unbind_mm(dev, ctx->pasid);
}
