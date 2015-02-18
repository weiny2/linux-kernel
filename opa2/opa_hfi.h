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

#ifndef _OPA_HFI_H
#define _OPA_HFI_H

#include <linux/pci.h>
#include <linux/slab.h>
#include <rdma/hfi_cmd.h>
#include <rdma/opa_core.h>

#define DRIVER_NAME		KBUILD_MODNAME
#define DRIVER_CLASS_NAME	DRIVER_NAME

struct hfi_msix_entry {
	struct msix_entry msix;
	void *arg;
	cpumask_var_t mask;
};

/* device data struct contains only per-HFI info. */
struct hfi_devdata {
	/* pci access data structure */
	struct pci_dev *pcidev;
	struct opa_core_device *bus_dev;

	/* localbus width (1, 2,4,8,16,32) from config space  */
	u32 lbus_width;
	/* localbus speed in MHz */
	u32 lbus_speed;
	int unit; /* unit # of this chip */
	int node; /* home node of this chip */
	/* so we can rewrite it after a chip reset */
	u32 pcibar0;
	/* so we can rewrite it after a chip reset */
	u32 pcibar1;

	/* mem-mapped pointer to base of chip regs */
	u8 __iomem *kregbase;
	/* end of mem-mapped chip space excluding sendbuf and user regs */
	u8 __iomem *kregend;
	/* physical address of chip for io_remap, etc. */
	resource_size_t physaddr;

	/* MSI-X information */
	struct hfi_msix_entry *msix_entries;
	u32 num_msix_entries;

	/* Device Portals State */
	struct hfi_ctx **ptl_user;
	size_t ptl_user_size;
	unsigned long ptl_map[HFI_NUM_PIDS / BITS_PER_LONG];
	spinlock_t ptl_lock;

	/* Command Queue State */
	u16 cq_pair[HFI_CQ_COUNT];
	spinlock_t cq_lock;
	u16 cq_pair_next_unused;
	void *cq_tx_base;
	void *cq_rx_base;
	void *cq_head_base;
	size_t cq_head_size;
};

int hfi_pci_init(struct pci_dev *pdev, const struct pci_device_id *ent);
void hfi_pci_cleanup(struct pci_dev *pdev);
struct hfi_devdata *hfi_pci_dd_init(struct pci_dev *pdev,
				    const struct pci_device_id *ent);
void hfi_pci_dd_free(struct hfi_devdata *dd);
int hfi_pcie_params(struct hfi_devdata *dd, u32 minw, u32 *nent,
		    struct hfi_msix_entry *entry);
void hfi_disable_msix(struct hfi_devdata * dd);
int setup_interrupts(struct hfi_devdata *dd, int total, int minw);
void cleanup_interrupts(struct hfi_devdata *dd);

struct hfi_devdata *hfi_alloc_devdata(struct pci_dev *pdev);
int hfi_user_cleanup(struct hfi_ctx *ud);

/* HFI specific functions */
void hfi_cq_config(struct hfi_ctx *ctx, u16 cq_idx, void *head_base,
		   struct hfi_auth_tuple *auth_table);
void hfi_cq_config_tuples(struct hfi_ctx *ctx, u16 cq_idx,
			  struct hfi_auth_tuple *auth_table);
void hfi_cq_disable(struct hfi_devdata *dd, u16 cq_idx);
void hfi_pcb_write(struct hfi_ctx *ctx, u16 ptl_pid, int phys);
void hfi_pcb_reset(struct hfi_devdata *dd, u16 ptl_pid);

/* OPA core functions */
int hfi_cq_assign(struct hfi_ctx *ctx, struct hfi_auth_tuple *auth_table, u16 *cq_idx);
int hfi_cq_update(struct hfi_ctx *ctx, u16 cq_idx, struct hfi_auth_tuple *auth_table);
int hfi_cq_release(struct hfi_ctx *ctx, u16 cq_idx);
int hfi_dlid_assign(struct hfi_ctx *ctx, struct hfi_dlid_assign_args *dlid_assign);
int hfi_dlid_release(struct hfi_ctx *ctx);
int hfi_ctxt_attach(struct hfi_ctx *ctx, struct opa_ctx_assign *ctx_assign);
void hfi_ctxt_cleanup(struct hfi_ctx *ctx);
int hfi_ctxt_reserve(struct hfi_ctx *ctx, u16 *base, u16 count);
void hfi_ctxt_unreserve(struct hfi_ctx *ctx);
int hfi_ctxt_hw_addr(struct hfi_ctx *ctx, int token, u16 ctxt, void **addr,
		     ssize_t *len);

/*
 * dev_err can be used (only!) to print early errors before devdata is
 * allocated, or when dd->pcidev may not be valid, and at the tail end of
 * cleanup when devdata may have been freed, etc.
 * Otherwise these macros below are the preferred ones.
 */
#define dd_dev_err(dd, fmt, ...) \
	dev_err(&(dd)->pcidev->dev, DRIVER_NAME"%d: " fmt, \
		(dd)->unit, ##__VA_ARGS__)

#define dd_dev_info(dd, fmt, ...) \
	dev_info(&(dd)->pcidev->dev, DRIVER_NAME"%d: " fmt, \
		 (dd)->unit, ##__VA_ARGS__)

/* printk wrappers (pr_warn, etc) can also be used for general debugging. */
#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#endif
