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
#include <linux/intel-svm.h>
#include <rdma/fxr/fxr_top_defs.h>
#include <rdma/fxr/fxr_at_csrs.h>
#include <rdma/fxr/fxr_at_defs.h>
#include <rdma/fxr/translation_structs.h>
#include "opa_hfi.h"

int
hfi_iommu_set_pasid(struct hfi_ctx *ctx)
{
	struct hfi_devdata *dd = ctx->devdata;
	struct device *dev = &dd->pcidev->dev;
	int ret, pasid, flags = 0;
	FXR_AT_CFG_PASID_LUT_t lut = {.val = 0};

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

	/*
	 * Set privileged mode for non-user contxt,
	 * also set system pasid for pid=0.
	 */
	if (ctx->type != HFI_CTX_TYPE_USER) {
		if (ctx->pid == HFI_PID_SYSTEM) {
			FXR_AT_CFG_USE_SYSTEM_PASID_t sys = {.val = 0};

			sys.field.enable = 1;
			sys.field.system_pasid = pasid;
			write_csr(dd, FXR_AT_CFG_USE_SYSTEM_PASID, sys.val);

			dd->system_pasid = pasid;
		} else {
			WARN_ON(dd->system_pasid != pasid);
		}

		lut.field.privilege_level = 0;
	} else {
		lut.field.privilege_level = 1;
	}

	lut.field.enable = 1;
	lut.field.pasid = pasid;

	write_csr(dd, FXR_AT_CFG_PASID_LUT + (ctx->pid * 8), lut.val);

	return ret;
}

int
hfi_iommu_clear_pasid(struct hfi_ctx *ctx)
{
	struct hfi_devdata *dd = ctx->devdata;
	struct device *dev = &dd->pcidev->dev;
	FXR_AT_CFG_PASID_LUT_t lut = {.val = 0};

	lut.field.enable = 0;
	write_csr(dd, FXR_AT_CFG_PASID_LUT + (ctx->pid * 8), lut.val);

	if (ctx->type != HFI_CTX_TYPE_USER) {
		if (ctx->pid == HFI_PID_SYSTEM) {
			FXR_AT_CFG_USE_SYSTEM_PASID_t sys = {.val = 0};

			sys.field.enable = 0;
			write_csr(dd, FXR_AT_CFG_USE_SYSTEM_PASID, sys.val);
		}
	}

	return intel_svm_unbind_mm(dev, ctx->pasid);
}
