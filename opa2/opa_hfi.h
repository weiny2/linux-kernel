/*
 * Copyright (c) 2014 Intel Corporation.  All rights reserved.
 * Copyright (c) 2006 - 2012 QLogic Corporation. All rights reserved.
 * Copyright (c) 2003, 2004, 2005, 2006 PathScale, Inc. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _OPA_HFI_H
#define _OPA_HFI_H

#include <linux/pci.h>
#include "../include/hfi_defs.h"
#include "../common/opa.h"

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
	struct hfi_userdata **ptl_user;
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
int hfi_user_cleanup(struct hfi_userdata *dd);

/* HFI specific functions */
void hfi_cq_config(struct hfi_userdata *ud, u16 cq_idx, void *head_base,
		   struct hfi_auth_tuple *auth_table);
void hfi_cq_config_tuples(struct hfi_userdata *ud, u16 cq_idx,
			  struct hfi_auth_tuple *auth_table);
void hfi_cq_disable(struct hfi_devdata *dd, u16 cq_idx);
void hfi_pcb_write(struct hfi_userdata *ud, u16 ptl_pid, int phys);
void hfi_pcb_reset(struct hfi_devdata *dd, u16 ptl_pid);

/* OPA core functions */
int hfi_cq_assign(struct hfi_userdata *ud, struct hfi_cq_assign_args *cq_assign);
int hfi_cq_update(struct hfi_userdata *ud, struct hfi_cq_update_args *cq_update);
int hfi_cq_release(struct hfi_userdata *ud, u16 cq_idx);
int hfi_dlid_assign(struct hfi_userdata *ud, struct hfi_dlid_assign_args *dlid_assign);
int hfi_dlid_release(struct hfi_userdata *ud);
int hfi_ctxt_hw_addr(struct hfi_userdata *ud, int token, u16 ctxt, void **addr,
		     ssize_t *len);
int hfi_ctxt_attach(struct hfi_userdata *ud, struct hfi_ctxt_attach_args *ctxt_attach);
void hfi_ctxt_cleanup(struct hfi_userdata *ud);
int hfi_ctxt_reserve(struct hfi_devdata *dd, u16 *base, u16 count);
void hfi_ctxt_unreserve(struct hfi_devdata *dd, u16 base, u16 count);
void hfi_job_init(struct hfi_userdata *ud);
int hfi_job_info(struct hfi_userdata *ud, struct hfi_job_info_args *job_info);
int hfi_job_setup(struct hfi_userdata *ud, struct hfi_job_setup_args *job_setup);
void hfi_job_free(struct hfi_userdata *ud);

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
#endif
