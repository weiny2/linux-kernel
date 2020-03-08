/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright(c) 2017-2020 Intel Corporation
 */

#ifndef __DLB_MAIN_H
#define __DLB_MAIN_H

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/ktime.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/pci.h>
#include <linux/types.h>

#include <uapi/linux/dlb_user.h>

#include "dlb_hw_types.h"

static const char dlb_driver_name[] = KBUILD_MODNAME;

#define DLB_NUM_FUNCS_PER_DEVICE (1 + DLB_MAX_NUM_VFS)
#define DLB_MAX_NUM_DEVICES (DLB_NUM_FUNCS_PER_DEVICE)
#define DLB_NUM_DEV_FILES_PER_DEVICE (DLB_MAX_NUM_DOMAINS + 1)
#define DLB_MAX_NUM_DEVICE_FILES (DLB_MAX_NUM_DEVICES * \
				  DLB_NUM_DEV_FILES_PER_DEVICE)
/* This macro returns an ID from 0 to DLB_NUM_DEV_FILES_PER_DEVICE, inclusive */
#define DLB_FILE_ID_FROM_DEV_T(base, file) ((MINOR(file) - MINOR(base)) % \
					    DLB_NUM_DEV_FILES_PER_DEVICE)
/* This macro returns an ID from 0 to DLB_MAX_NUM_DEVICES, inclusive */
#define DLB_DEV_ID_FROM_DEV_T(base, file) ((MINOR(file) - MINOR(base)) / \
					   DLB_NUM_DEV_FILES_PER_DEVICE)
#define IS_DLB_DEV_FILE(base, file) (DLB_FILE_ID_FROM_DEV_T(base, file) == \
				     DLB_MAX_NUM_DOMAINS)

enum dlb_device_type {
	DLB_PF,
	DLB_VF,
};

struct dlb_dev;

struct dlb_device_ops {
	int (*map_pci_bar_space)(struct dlb_dev *dev, struct pci_dev *pdev);
	void (*unmap_pci_bar_space)(struct dlb_dev *dev, struct pci_dev *pdev);
	int (*init_driver_state)(struct dlb_dev *dev);
	void (*free_driver_state)(struct dlb_dev *dev);
	int (*device_create)(struct dlb_dev *dlb_dev,
			     struct pci_dev *pdev,
			     struct class *dlb_class);
	void (*device_destroy)(struct dlb_dev *dlb_dev,
			       struct class *dlb_class);
	int (*cdev_add)(struct dlb_dev *dlb_dev,
			dev_t base,
			const struct file_operations *fops);
	void (*cdev_del)(struct dlb_dev *dlb_dev);
	void (*init_hardware)(struct dlb_dev *dev);
	int (*create_sched_domain)(struct dlb_hw *hw,
				   struct dlb_create_sched_domain_args *args,
				   struct dlb_cmd_response *resp);
	int (*get_num_resources)(struct dlb_hw *hw,
				 struct dlb_get_num_resources_args *args);
	int (*reset_domain)(struct dlb_dev *dev, u32 domain_id);
};

extern struct dlb_device_ops dlb_pf_ops;

struct dlb_status {
	u8 valid;
	u32 refcnt;
};

struct dlb_domain_dev {
	struct dlb_status *status;
};

struct dlb_dev {
	int id;
	struct pci_dev *pdev;
	struct dlb_hw hw;
	struct cdev cdev;
	enum dlb_device_type type;
	u8 revision;
	struct dlb_device_ops *ops;
	dev_t dev_number;
	struct list_head list;
	struct device *dlb_device;
	struct dlb_domain_dev sched_domains[DLB_MAX_NUM_DOMAINS];
	u8 domain_reset_failed;
	/* The resource mutex serializes access to driver data structures and
	 * hardware registers.
	 */
	struct mutex resource_mutex;
};

int dlb_add_domain_device_file(struct dlb_dev *dlb_dev, u32 domain_id);

#define DLB_HW_ERR(hw, ...) do {		    \
	struct dlb_dev *dev;			    \
	dev = container_of(hw, struct dlb_dev, hw); \
	dev_err(dev->dlb_device, __VA_ARGS__);	    \
} while (0)

#define DLB_HW_DBG(hw, ...) do {		    \
	struct dlb_dev *dev;			    \
	dev = container_of(hw, struct dlb_dev, hw); \
	dev_dbg(dev->dlb_device, __VA_ARGS__);	    \
} while (0)

#endif /* __DLB_MAIN_H */
