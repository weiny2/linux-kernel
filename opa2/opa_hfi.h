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

#define HFI_NUM_BARS		2
#define HFI_NUM_PPORTS		2

/* In accordance with stl vol 1 section 4.1 */
#define PGUID_MASK		(~(0x3UL << 32))
#define PORT_GUID(ng, pn)	(((be64_to_cpu(ng)) & PGUID_MASK) |\
				 (((u64)pn) << 32))

/* FXRTODO: Harcoded for now. Fix this once MNH reg is available */
#define NODE_GUID		cpu_to_be64(0x11750101000000UL)

#define pidx_to_pnum(id)	((id) + 1)
#define pnum_to_pidx(pn)	((pn) - 1)

struct hfi_msix_entry {
	struct msix_entry msix;
	void *arg;
	cpumask_var_t mask;
};

struct hfi_pportdata {
	/* port_guid identifying port */
	__be64 pguid;
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
	u32 pcibar[HFI_NUM_BARS];
	/* mem-mapped pointer to base of chip regs */
	u8 __iomem *kregbase[HFI_NUM_BARS];
	u8 __iomem *kregend[HFI_NUM_BARS];
	/* physical address of chip for io_remap, etc. */
	resource_size_t physaddr;

	/* MSI-X information */
	struct hfi_msix_entry *msix_entries;
	u32 num_msix_entries;

	/* Device Portals State */
	struct idr ptl_user;
	unsigned long ptl_map[HFI_NUM_PIDS / BITS_PER_LONG];
	spinlock_t ptl_lock;
	struct hfi_ctx priv_ctx;

	/* Command Queue State */
	struct idr cq_pair;
	spinlock_t cq_lock;
	void *cq_tx_base;
	void *cq_rx_base;
	void *cq_head_base;
	size_t cq_head_size;

	/* IOMMU */
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
};

void hfi_pport_init(struct hfi_devdata *dd);
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
int hfi_cq_assign_privileged(struct hfi_ctx *ctx, u16 *cq_idx);
void hfi_cq_cleanup(struct hfi_ctx *ctx);
void hfi_cq_config(struct hfi_ctx *ctx, u16 cq_idx, void *head_base,
		   struct hfi_auth_tuple *auth_table, bool unprivileged);
void hfi_cq_config_tuples(struct hfi_ctx *ctx, u16 cq_idx,
			  struct hfi_auth_tuple *auth_table);
int hfi_update_dlid_relocation_table(struct hfi_ctx *ctx,
			       struct hfi_dlid_assign_args *dlid_assign);
int hfi_reset_dlid_relocation_table(struct hfi_ctx *ctx, u32 dlid_base,
				    u32 count);
void hfi_cq_disable(struct hfi_devdata *dd, u16 cq_idx);
void hfi_pcb_write(struct hfi_ctx *ctx, u16 ptl_pid);
void hfi_pcb_reset(struct hfi_devdata *dd, u16 ptl_pid);

/* OPA core functions */
int hfi_cq_assign(struct hfi_ctx *ctx, struct hfi_auth_tuple *auth_table, u16 *cq_idx);
int hfi_cq_update(struct hfi_ctx *ctx, u16 cq_idx, struct hfi_auth_tuple *auth_table);
int hfi_cq_release(struct hfi_ctx *ctx, u16 cq_idx);
int hfi_dlid_assign(struct hfi_ctx *ctx,
		    struct hfi_dlid_assign_args *dlid_assign);
int hfi_dlid_release(struct hfi_ctx *ctx, u32 dlid_base, u32 count);
int hfi_cteq_assign(struct hfi_ctx *ctx, struct opa_ev_assign *ev_assign);
int hfi_cteq_release(struct hfi_ctx *ctx, u16 eq_mode, u16 eq_idx);
int hfi_ctxt_attach(struct hfi_ctx *ctx, struct opa_ctx_assign *ctx_assign);
void hfi_ctxt_cleanup(struct hfi_ctx *ctx);
int hfi_ctxt_reserve(struct hfi_ctx *ctx, u16 *base, u16 count);
void hfi_ctxt_unreserve(struct hfi_ctx *ctx);
int hfi_ctxt_hw_addr(struct hfi_ctx *ctx, int token, u16 ctxt, void **addr,
		     ssize_t *len);

int hfi_iommu_root_alloc(void);
void hfi_iommu_root_free(void);
int hfi_iommu_root_set_context(struct hfi_devdata *dd);
void hfi_iommu_root_clear_context(struct hfi_devdata *dd);
void hfi_iommu_set_pasid(struct hfi_devdata *dd, struct mm_struct *mm, u16 pasid);
void hfi_iommu_clear_pasid(struct hfi_devdata *dd, u16 pasid);

#define get_ppd_pn(dd, pn)		(&(dd)->pport[pnum_to_pidx(pn)])
#define get_ppd_pidx(dd, idx)		(&(dd)->pport[idx])

int hfi_get_sma(struct opa_core_device *odev, u16 attr_id, struct opa_smp *smp,
			u32 am, u8 *data, u8 port, u32 *resp_len);
int hfi_set_sma(struct opa_core_device *odev, u16 attr_id, struct opa_smp *smp,
				 u32 am, u8 *data, u8 port, u32 *resp_len);

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
