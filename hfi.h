/*
 * Copyright (c) 2012 - 2014 Intel Corporation.  All rights reserved.
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

#ifndef _HFI_KERNEL_H
#define _HFI_KERNEL_H

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
#include <linux/cdev.h>
#include "include/hfi_defs.h"
#include "include/hfi_cmd.h"

#define DRIVER_NAME		"hfi2"
#define DRIVER_CLASS_NAME	DRIVER_NAME
/* device naming has leading zero to prevent /dev name collisions */
#define DRIVER_DEVICE_PREFIX	"hfi02"

#define HFI_USER_MINOR_BASE     0
#define HFI_TRACE_MINOR         127
#define HFI_DIAGPKT_MINOR       128
#define HFI_DIAG_MINOR_BASE     129
#define HFI_UI_MINOR_BASE       192
#define HFI_SNOOP_CAPTURE_BASE  200
#define HFI_NMINORS             255

extern struct pci_driver hfi_driver;

struct hfi_msix_entry {
	struct msix_entry msix;
	void *arg;
	cpumask_var_t mask;
};

/* FXR Portals Control Block
 * This is a HW-defined structure.
 */
typedef union PCB hfi_ptl_control_t;

/* device data struct contains only per-HFI info. */
struct hfi_devdata {
	/* pci access data structure */
	struct pci_dev *pcidev;
	/* pointers to related structs for this device */
	struct cdev user_cdev;
	struct cdev diag_cdev;
	struct cdev ui_cdev;
	struct device *user_device;
	struct device *diag_device;
	struct device *ui_device;

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
	hfi_ptl_control_t *ptl_control;
	struct hfi_userdata **ptl_user;
	size_t ptl_control_size;
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
	u16 srank;
	u16 pasid;
	u16 cq_pair_num_assigned;
	u8 allow_phys_dlid;
	u8 auth_mask;
	u32 auth_table[HFI_NUM_AUTH_TUPLES];
	u32 ptl_uid; /* UID if auth_tuples not used */

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

int hfi_pci_init(struct pci_dev *, const struct pci_device_id *);
void hfi_pci_cleanup(struct pci_dev *);
struct hfi_devdata *hfi_pci_dd_init(struct pci_dev *,
				    const struct pci_device_id *);
void hfi_pci_dd_free(struct hfi_devdata *);
int hfi_pcie_params(struct hfi_devdata *, u32, u32 *, struct hfi_msix_entry *);
void hfi_disable_msix(struct hfi_devdata *);
int setup_interrupts(struct hfi_devdata *dd, int, int);
void cleanup_interrupts(struct hfi_devdata *dd);

struct hfi_devdata *hfi_alloc_devdata(struct pci_dev *pdev);
int hfi_user_add(struct hfi_devdata *);
void hfi_user_remove(struct hfi_devdata *);
int hfi_ui_add(struct hfi_devdata *);
void hfi_ui_remove(struct hfi_devdata *);
int hfi_user_cleanup(struct hfi_userdata *);

void hfi_cq_config(struct hfi_userdata *ud, u16 cq_idx, void *, u8 *auth_idx);
void hfi_cq_disable(struct hfi_devdata *dd, u16 cq_idx);
int hfi_cq_assign(struct hfi_userdata *ud, struct hfi_cq_assign_args *);
int hfi_cq_release(struct hfi_userdata *ud, u16 cq_idx);
int hfi_dlid_assign(struct hfi_userdata *ud, struct hfi_dlid_assign_args *);
int hfi_dlid_release(struct hfi_userdata *ud);
int hfi_ptl_attach(struct hfi_userdata *ud, struct hfi_ptl_attach_args *);
void hfi_ptl_cleanup(struct hfi_userdata *ud);
int hfi_ptl_reserve(struct hfi_devdata *dd, u16 *base, u16 count);
void hfi_ptl_unreserve(struct hfi_devdata *dd, u16 base, u16 count);
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

/* printk wrappers (pr_warn, etc) can also be used for general debugging. */
#undef pr_fmt
#define pr_fmt(fmt) DRIVER_NAME ": " fmt

#endif /* _HFI_KERNEL_H */
