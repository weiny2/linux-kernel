/*
 * Copyright (c) 2014 Intel Corporation.  All rights reserved.
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

#ifndef _OPA_COMMON_H
#define _OPA_COMMON_H

/*
 * This header file is the base header file for HFI kernel driver.
 */

#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/dma-mapping.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/io.h>
#include "../include/hfi_defs.h"
#include "../include/hfi_cmd.h"
#include "opa_core.h"

/* device naming has leading zero to prevent /dev name collisions */
#define DRIVER_DEVICE_PREFIX	"hfi02"

#define PCI_VENDOR_ID_INTEL       0x8086
#define PCI_DEVICE_ID_INTEL_FXR0  0x26d0

/*
 * Define the driver version number.  This is something that refers only
 * to the driver itself, not the software interfaces it supports.
 */
#ifndef HFI_DRIVER_VERSION_BASE
#define HFI_DRIVER_VERSION_BASE "0.0"
#endif

/* create the final driver version string */
#ifdef HFI_IDSTR
#define HFI_DRIVER_VERSION HFI_DRIVER_VERSION_BASE " " HFI_IDSTR
#else
#define HFI_DRIVER_VERSION HFI_DRIVER_VERSION_BASE
#endif

struct hfi_msix_entry {
	struct msix_entry msix;
	void *arg;
	cpumask_var_t mask;
};

struct hfi_mpin_entry {
	u64 pfn;
	struct list_head list;
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

/* Private data for file operations, created at open(). */
struct hfi_userdata {
	struct hfi_info *hi;
	struct opa_core_ops *bus_ops;
	struct hfi_devdata *devdata;
	/* for cpu affinity; -1 if none */
	int rec_cpu_num;
	pid_t pid;
	pid_t sid;

	/* Per PID Portals State */
	void *ptl_state_base;
	void *ptl_le_me_base;
	u32 ptl_state_size;
	u32 ptl_le_me_size;
	u32 ptl_unexpected_size;
	u32 ptl_trig_op_size;
	u16 ptl_pid;
	u16 pasid;
	u16 cq_pair_num_assigned;
	u8 allow_phys_dlid;
	u8 auth_mask;
	u32 auth_uid[HFI_NUM_AUTH_TUPLES];
	u32 ptl_uid; /* default UID if auth_tuples not used */
	struct list_head mpin_head;
	spinlock_t mpin_lock;

	u32 dlid_base;
	u32 lid_offset;
	u32 lid_count;
	u16 pid_base;
	u16 pid_count;
	u16 pid_mode;
	u16 res_mode;
	u32 sl_mask;
	struct list_head job_list;
};

int hfi_mpin(struct hfi_userdata *ud, struct hfi_mpin_args *mpin);
int hfi_munpin(struct hfi_userdata *ud, struct hfi_munpin_args *munpin);
int hfi_munpin_all(struct hfi_userdata *ud);

/**
 * opa_core_ops - Hardware operations for accessing a FXR device on the FXR bus.
 */
struct opa_core_ops {
	/* Resource Allocation ops */
	int (*ctxt_assign)(struct hfi_userdata *ud, struct hfi_ptl_attach_args *ptl_attach);
	void (*ctxt_release)(struct hfi_userdata *ud);
	int (*ctxt_addr)(struct hfi_userdata *ud, int type, u16 ctxt, void **addr, ssize_t *len);

	/* FXR specific Resource Allocation ops */
	int (*cq_assign)(struct hfi_userdata *ud, struct hfi_cq_assign_args *cq_assign);
	int (*cq_update)(struct hfi_userdata *ud, struct hfi_cq_update_args *cq_update);
	int (*cq_release)(struct hfi_userdata *ud, u16 cq_idx);
	int (*eq_assign)(struct hfi_userdata *ud, struct hfi_eq_assign_args *eq_assign);
	int (*eq_release)(struct hfi_userdata *ud, u16 eq_type, u16 eq_idx);
	int (*dlid_assign)(struct hfi_userdata *ud, struct hfi_dlid_assign_args *dlid_assign);
	int (*dlid_release)(struct hfi_userdata *ud);
	int (*job_info)(struct hfi_userdata *ud, struct hfi_job_info_args *job_info);
	void (*job_init)(struct hfi_userdata *ud);
	void (*job_free)(struct hfi_userdata *ud);
	int (*job_setup)(struct hfi_userdata *ud, struct hfi_job_setup_args *job_setup);

#if 0
	/* TODO - below is starting attempt at 'kernel provider' operations */
	/* critical functions would be inlined instead of bus operation */
	/* RX and completion ops */
	int (*rx_alloc)(struct hfi_ctx *ctx, struct hfi_rx_alloc_args *rx_alloc);
	int (*rx_free)(struct hfi_ctx *ctx, u16 rx_type, u32 rx_idx);
	int (*rx_enable)(struct hfi_ctx *ctx, u16 rx_type, u32 rx_idx);
	int (*rx_disable)(struct hfi_ctx *ctx, u16 rx_type, u32 rx_idx);
	int (*eq_alloc)(struct hfi_ctx *ctx, struct hfi_eq_alloc_args *eq_alloc);
	int (*eq_free)(struct hfi_ctx *ctx, u16 eq_type, u16 eq_idx);
	int (*eq_get)(struct hfi_ctx *ctx, u16 eq_type, u16 eq_idx);
	/* TX ops */
	int (*tx_command)(struct hfi_ctx *ctx, u64 cmd, void *header, void *payload);
#endif
};

/* printk wrappers (pr_warn, etc) can also be used for general debugging. */
#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#endif /* _OPA_COMMON_H */
