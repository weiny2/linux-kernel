// SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause)
/* Copyright(c) 2017-2020 Intel Corporation */

#include <linux/aer.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/pm_runtime.h>
#include <linux/uaccess.h>

#include "dlb_main.h"

#define TO_STR2(s) #s
#define TO_STR(s) TO_STR2(s)

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Copyright(c) 2017-2020 Intel Corporation");
MODULE_DESCRIPTION("Intel(R) Dynamic Load Balancer Driver");

static const char
dlb_driver_copyright[] = "Copyright(c) 2017-2020 Intel Corporation";

/* The driver lock protects driver data structures that may be used by multiple
 * devices.
 */
DEFINE_MUTEX(dlb_driver_lock);
struct list_head dlb_dev_list = LIST_HEAD_INIT(dlb_dev_list);

static struct class *dlb_class;
static dev_t dlb_dev_number_base;

/*****************************/
/****** Devfs callbacks ******/
/*****************************/

static int dlb_open(struct inode *i, struct file *f)
{
	return 0;
}

static int dlb_close(struct inode *i, struct file *f)
{
	return 0;
}

static ssize_t dlb_read(struct file *f,
			char __user *buf,
			size_t len,
			loff_t *offset)
{
	return 0;
}

static ssize_t dlb_write(struct file *f,
			 const char __user *buf,
			 size_t len,
			 loff_t *offset)
{
	return 0;
}

static const struct file_operations dlb_fops = {
	.owner   = THIS_MODULE,
	.open    = dlb_open,
	.release = dlb_close,
	.read    = dlb_read,
	.write   = dlb_write,
};

static void dlb_assign_ops(struct dlb_dev *dlb_dev,
			   const struct pci_device_id *pdev_id)
{
	dlb_dev->type = pdev_id->driver_data;

	switch (pdev_id->driver_data) {
	case DLB_PF:
		dlb_dev->ops = &dlb_pf_ops;
		break;
	}
}

static inline void dlb_set_device_revision(struct dlb_dev *dlb_dev)
{
	switch (boot_cpu_data.x86_stepping) {
	case 0:
		dlb_dev->revision = DLB_REV_A0;
		break;
	case 1:
		dlb_dev->revision = DLB_REV_A1;
		break;
	case 2:
		dlb_dev->revision = DLB_REV_A2;
		break;
	case 3:
		dlb_dev->revision = DLB_REV_A3;
		break;
	default:
		/* Treat all revisions >= 4 as B0 */
		dlb_dev->revision = DLB_REV_B0;
		break;
	}
}

static bool dlb_id_in_use[DLB_MAX_NUM_DEVICES];

static int dlb_find_next_available_id(void)
{
	int i;

	for (i = 0; i < DLB_MAX_NUM_DEVICES; i++)
		if (!dlb_id_in_use[i])
			return i;

	return -1;
}

static void dlb_set_id_in_use(int id, bool in_use)
{
	dlb_id_in_use[id] = in_use;
}

static int dlb_mask_ur_err(struct pci_dev *dev)
{
	u32 mask;

	int pos = pci_find_ext_capability(dev, PCI_EXT_CAP_ID_ERR);

	if (pci_read_config_dword(dev, pos + PCI_ERR_UNCOR_MASK, &mask))
		return -1;

	mask |= PCI_ERR_UNC_UNSUP;

	if (pci_write_config_dword(dev, pos + PCI_ERR_UNCOR_MASK, mask))
		return -1;

	return 0;
}

static int dlb_probe(struct pci_dev *pdev,
		     const struct pci_device_id *pdev_id)
{
	struct dlb_dev *dlb_dev;
	int ret;

	dev_dbg(&pdev->dev, "probe\n");

	dlb_dev = devm_kzalloc(&pdev->dev, sizeof(*dlb_dev), GFP_KERNEL);
	if (!dlb_dev) {
		ret = -ENOMEM;
		goto dlb_dev_kzalloc_fail;
	}

	mutex_lock(&dlb_driver_lock);
	list_add(&dlb_dev->list, &dlb_dev_list);
	mutex_unlock(&dlb_driver_lock);

	dlb_assign_ops(dlb_dev, pdev_id);

	pci_set_drvdata(pdev, dlb_dev);

	dlb_dev->pdev = pdev;

	dlb_set_device_revision(dlb_dev);

	dlb_dev->id = dlb_find_next_available_id();
	if (dlb_dev->id == -1) {
		dev_err(&pdev->dev, "probe: insufficient device IDs\n");

		ret = -EINVAL;
		goto next_available_id_fail;
	}

	ret = pci_enable_device(pdev);
	if (ret != 0) {
		dev_err(&pdev->dev, "pci_enable_device() returned %d\n", ret);

		goto pci_enable_device_fail;
	}

	ret = pci_request_regions(pdev, dlb_driver_name);
	if (ret != 0) {
		dev_err(&pdev->dev,
			"pci_request_regions(): returned %d\n", ret);

		goto pci_request_regions_fail;
	}

	pci_set_master(pdev);

	pci_intx(pdev, 0);

	/* DLB incorrectly sends URs in response to certain messages. Mask UR
	 * errors to prevent these from being propagated to the MCA.
	 */
	if (dlb_mask_ur_err(pdev))
		dev_err(&pdev->dev,
			"[%s()] Failed to mask UR errors\n",
			__func__);

	if (pci_enable_pcie_error_reporting(pdev))
		dev_err(&pdev->dev, "[%s()] Failed to enable AER\n", __func__);

	ret = dlb_dev->ops->map_pci_bar_space(dlb_dev, pdev);
	if (ret)
		goto map_pci_bar_fail;

	ret = dlb_dev->ops->cdev_add(dlb_dev, dlb_dev_number_base, &dlb_fops);
	if (ret)
		goto cdev_add_fail;

	ret = dlb_dev->ops->device_create(dlb_dev, pdev, dlb_class);
	if (ret)
		goto device_add_fail;

	ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
	if (ret)
		goto dma_set_mask_fail;

	dlb_set_id_in_use(dlb_dev->id, true);

	return 0;

dma_set_mask_fail:
	dlb_dev->ops->device_destroy(dlb_dev, dlb_class);
device_add_fail:
	dlb_dev->ops->cdev_del(dlb_dev);
cdev_add_fail:
	dlb_dev->ops->unmap_pci_bar_space(dlb_dev, pdev);
map_pci_bar_fail:
	pci_disable_pcie_error_reporting(pdev);
	pci_release_regions(pdev);
pci_request_regions_fail:
	pci_disable_device(pdev);
pci_enable_device_fail:
next_available_id_fail:
	mutex_lock(&dlb_driver_lock);
	list_del(&dlb_dev->list);
	mutex_unlock(&dlb_driver_lock);

	devm_kfree(&pdev->dev, dlb_dev);
dlb_dev_kzalloc_fail:
	return ret;
}

static void dlb_remove(struct pci_dev *pdev)
{
	struct dlb_dev *dlb_dev;

	/* Undo all the dlb_probe() operations */
	dev_dbg(&pdev->dev, "Cleaning up the DLB driver for removal\n");
	dlb_dev = pci_get_drvdata(pdev);

	dlb_set_id_in_use(dlb_dev->id, false);

	dlb_dev->ops->device_destroy(dlb_dev, dlb_class);

	dlb_dev->ops->cdev_del(dlb_dev);

	dlb_dev->ops->unmap_pci_bar_space(dlb_dev, pdev);

	pci_disable_pcie_error_reporting(pdev);

	pci_release_regions(pdev);

	pci_disable_device(pdev);

	mutex_lock(&dlb_driver_lock);
	list_del(&dlb_dev->list);
	mutex_unlock(&dlb_driver_lock);

	devm_kfree(&pdev->dev, dlb_dev);
}

static struct pci_device_id dlb_id_table[] = {
	{ PCI_DEVICE_DATA(INTEL, DLB_PF, DLB_PF) },
	{ 0 }
};
MODULE_DEVICE_TABLE(pci, dlb_id_table);

static struct pci_driver dlb_pci_driver = {
	.name		 = (char *)dlb_driver_name,
	.id_table	 = dlb_id_table,
	.probe		 = dlb_probe,
	.remove		 = dlb_remove,
};

static int __init dlb_init_module(void)
{
	int err;

	pr_info("%s\n", dlb_driver_name);
	pr_info("%s\n", dlb_driver_copyright);

	dlb_class = class_create(THIS_MODULE, dlb_driver_name);

	if (!dlb_class) {
		pr_err("%s: class_create() returned %ld\n",
		       dlb_driver_name, PTR_ERR(dlb_class));

		return PTR_ERR(dlb_class);
	}

	/* Allocate one minor number per domain */
	err = alloc_chrdev_region(&dlb_dev_number_base,
				  0,
				  DLB_MAX_NUM_DEVICE_FILES,
				  dlb_driver_name);

	if (err < 0) {
		pr_err("%s: alloc_chrdev_region() returned %d\n",
		       dlb_driver_name, err);

		return err;
	}

	err = pci_register_driver(&dlb_pci_driver);
	if (err < 0) {
		pr_err("%s: pci_register_driver() returned %d\n",
		       dlb_driver_name, err);
		return err;
	}

	return 0;
}

static void __exit dlb_exit_module(void)
{
	pr_info("%s: exit\n", dlb_driver_name);

	pci_unregister_driver(&dlb_pci_driver);

	unregister_chrdev_region(dlb_dev_number_base, DLB_MAX_NUM_DEVICE_FILES);

	if (dlb_class) {
		class_destroy(dlb_class);
		dlb_class = NULL;
	}
}

module_init(dlb_init_module);
module_exit(dlb_exit_module);
