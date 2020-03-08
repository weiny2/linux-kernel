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
};

extern struct dlb_device_ops dlb_pf_ops;

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
	/* The resource mutex serializes access to driver data structures and
	 * hardware registers.
	 */
	struct mutex resource_mutex;
};

#endif /* __DLB_MAIN_H */
