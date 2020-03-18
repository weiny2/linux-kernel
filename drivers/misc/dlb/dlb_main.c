// SPDX-License-Identifier: GPL-2.0-only
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

#include "dlb_ioctl.h"
#include "dlb_main.h"
#include "dlb_mem.h"
#include "dlb_resource.h"

#define TO_STR2(s) #s
#define TO_STR(s) TO_STR2(s)

#define DRV_VERSION \
	TO_STR(DLB_VERSION_MAJOR_NUMBER) "." \
	TO_STR(DLB_VERSION_MINOR_NUMBER) "." \
	TO_STR(DLB_VERSION_REVISION_NUMBER)

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Copyright(c) 2017-2020 Intel Corporation");
MODULE_DESCRIPTION("Intel(R) Dynamic Load Balancer Driver");
MODULE_VERSION(DRV_VERSION);

static const char
dlb_driver_copyright[] = "Copyright(c) 2017-2020 Intel Corporation";

/* The driver lock protects driver data structures that may be used by multiple
 * devices.
 */
DEFINE_MUTEX(dlb_driver_lock);
struct list_head dlb_dev_list = LIST_HEAD_INIT(dlb_dev_list);

static struct class *dlb_class;
static dev_t dlb_dev_number_base;

static int dlb_reset_device(struct pci_dev *pdev)
{
	int ret;

	ret = pci_save_state(pdev);
	if (ret)
		return ret;

	ret = __pci_reset_function_locked(pdev);
	if (ret)
		return ret;

	pci_restore_state(pdev);

	return 0;
}

/*****************************/
/****** Devfs callbacks ******/
/*****************************/

static int dlb_open_domain_device_file(struct dlb_dev *dev, struct file *f)
{
	struct dlb_domain_dev *domain;
	u32 domain_id;

	domain_id = DLB_FILE_ID_FROM_DEV_T(dlb_dev_number_base,
					   f->f_inode->i_rdev);

	if (domain_id >= DLB_MAX_NUM_DOMAINS) {
		dev_err(dev->dlb_device,
			"[%s()] Internal error\n", __func__);
		return -EINVAL;
	}

	dev_dbg(dev->dlb_device,
		"Opening domain %d's device file\n", domain_id);

	domain = &dev->sched_domains[domain_id];

	/* Race condition: thread A opens a domain file just after
	 * thread B does the last close and resets the domain.
	 */
	if (!domain->status)
		return -ENOENT;

	f->private_data = domain->status;

	domain->status->refcnt++;

	return 0;
}

static int dlb_open(struct inode *i, struct file *f)
{
	struct dlb_dev *dev;
	int ret = 0;

	dev = container_of(f->f_inode->i_cdev, struct dlb_dev, cdev);

	mutex_lock(&dev->resource_mutex);

	if (!IS_DLB_DEV_FILE(dlb_dev_number_base, f->f_inode->i_rdev))
		ret = dlb_open_domain_device_file(dev, f);
	if (ret)
		goto end;

	dev->ops->inc_pm_refcnt(dev->pdev, true);

end:
	mutex_unlock(&dev->resource_mutex);

	return ret;
}

int dlb_add_domain_device_file(struct dlb_dev *dlb_dev, u32 domain_id)
{
	struct dlb_status *status;
	struct device *dev;

	dev_dbg(dlb_dev->dlb_device,
		"Creating domain %d's device file\n", domain_id);

	status = devm_kzalloc(dlb_dev->dlb_device, sizeof(*status), GFP_KERNEL);
	if (!status)
		return -ENOMEM;

	status->valid = true;
	status->refcnt = 0;

	/* Create a new device in order to create a /dev/ domain node. This
	 * device is a child of the DLB PCI device.
	 */
	dev = device_create(dlb_class,
			    dlb_dev->dlb_device->parent,
			    MKDEV(MAJOR(dlb_dev->dev_number),
				  MINOR(dlb_dev->dev_number) + domain_id),
			    dlb_dev,
			    "dlb%d/domain%d",
			    DLB_DEV_ID_FROM_DEV_T(dlb_dev_number_base,
						  dlb_dev->dev_number),
			    domain_id);

	if (IS_ERR_VALUE(PTR_ERR(dev))) {
		dev_err(dlb_dev->dlb_device,
			"%s: device_create() returned %ld\n",
			dlb_driver_name, PTR_ERR(dev));

		devm_kfree(dlb_dev->dlb_device, status);
		return PTR_ERR(dev);
	}

	dlb_dev->sched_domains[domain_id].status = status;

	return 0;
}

static void dlb_reset_hardware_state(struct dlb_dev *dev)
{
	dlb_reset_device(dev->pdev);

	/* Reinitialize any other hardware state */
	dev->ops->init_hardware(dev);
}

static int dlb_close_domain_device_file(struct dlb_dev *dev,
					struct dlb_status *status,
					u32 domain_id)
{
	bool valid = status->valid;
	int ret = 0;

	devm_kfree(dev->dlb_device, status);

	/* Check if the domain was reset, its device file destroyed, and its
	 * memory released during FLR handling.
	 */
	if (!valid)
		return 0;

	dev->sched_domains[domain_id].status = NULL;

	device_destroy(dlb_class,
		       MKDEV(MAJOR(dev->dev_number),
			     MINOR(dev->dev_number) +
			     domain_id));

	ret = dev->ops->reset_domain(dev, domain_id);

	/* Free all memory associated with the domain */
	dlb_release_domain_memory(dev, domain_id);

	if (ret) {
		dev->domain_reset_failed = true;
		dev_err(dev->dlb_device,
			"Internal error: Domain reset failed. To recover, reset the device.\n");
	}

	return ret;
}

static int dlb_close(struct inode *i, struct file *f)
{
	struct dlb_status *status = f->private_data;
	struct dlb_dev *dev;
	int ret = 0;

	dev = container_of(f->f_inode->i_cdev, struct dlb_dev, cdev);

	mutex_lock(&dev->resource_mutex);

	if (!IS_DLB_DEV_FILE(dlb_dev_number_base, f->f_inode->i_rdev)) {
		struct dlb_domain_dev *domain;
		u32 domain_id;

		domain_id = DLB_FILE_ID_FROM_DEV_T(dlb_dev_number_base,
						   f->f_inode->i_rdev);

		if (domain_id >= DLB_MAX_NUM_DOMAINS) {
			dev_err(dev->dlb_device,
				"[%s()] Internal error\n", __func__);
			ret = -1;
			goto end;
		}

		domain = &dev->sched_domains[domain_id];

		dev_dbg(dev->dlb_device,
			"Closing domain %d's device file\n", domain_id);

		status->refcnt--;

		if (status->refcnt == 0)
			ret = dlb_close_domain_device_file(dev,
							   status,
							   domain_id);
	}

	dev->ops->dec_pm_refcnt(dev->pdev);

end:
	mutex_unlock(&dev->resource_mutex);

	return ret;
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

/* The kernel driver inserts VMAs into the device's VMA list whenever an mmap
 * is performed or the VMA is cloned (e.g. during fork()).
 */
static int dlb_insert_vma(struct dlb_dev *dev, struct vm_area_struct *vma)
{
	struct dlb_vma_node *node;

	node = devm_kzalloc(dev->dlb_device, sizeof(*node), GFP_KERNEL);
	if (!node)
		return -ENOMEM;

	node->vma = vma;

	mutex_lock(&dev->resource_mutex);

	list_add(&node->list, &dev->vma_list);

	mutex_unlock(&dev->resource_mutex);

	return 0;
}

/* The kernel driver deletes VMAs from the device's VMA list when the VMA is
 * closed (e.g. during process exit).
 */
static void dlb_delete_vma(struct dlb_dev *dev, struct vm_area_struct *vma)
{
	struct dlb_vma_node *vma_node;
	struct list_head *node;

	mutex_lock(&dev->resource_mutex);

	list_for_each(node, &dev->vma_list) {
		vma_node = list_entry(node, struct dlb_vma_node, list);
		if (vma_node->vma == vma) {
			list_del(&vma_node->list);
			devm_kfree(dev->dlb_device, vma_node);
			break;
		}
	}

	mutex_unlock(&dev->resource_mutex);
}

static void
dlb_vma_open(struct vm_area_struct *vma)
{
	struct dlb_dev *dev = vma->vm_private_data;

	dlb_insert_vma(dev, vma);
}

static void
dlb_vma_close(struct vm_area_struct *vma)
{
	struct dlb_dev *dev = vma->vm_private_data;

	dlb_delete_vma(dev, vma);
}

static const struct vm_operations_struct dlb_vma_ops = {
	.open = dlb_vma_open,
	.close = dlb_vma_close,
};

static int dlb_mmap(struct file *f, struct vm_area_struct *vma)
{
	struct dlb_dev *dev;
	u32 domain_id;
	int ret;

	dev = container_of(f->f_inode->i_cdev, struct dlb_dev, cdev);

	/* mmap operations must go through scheduling domain device files */
	if (IS_DLB_DEV_FILE(dlb_dev_number_base, f->f_inode->i_rdev))
		return -EINVAL;

	domain_id = DLB_FILE_ID_FROM_DEV_T(dlb_dev_number_base,
					   f->f_inode->i_rdev);

	if (domain_id >= DLB_MAX_NUM_DOMAINS) {
		dev_err(dev->dlb_device,
			"[%s()] Internal error\n", __func__);
		ret = -EINVAL;
		goto end;
	}

	ret = dlb_insert_vma(dev, vma);
	if (ret)
		goto end;

	mutex_lock(&dev->resource_mutex);

	ret = dev->ops->mmap(f, vma, domain_id);

	mutex_unlock(&dev->resource_mutex);

	if (ret)
		dlb_delete_vma(dev, vma);

	vma->vm_ops = &dlb_vma_ops;
	vma->vm_private_data = dev;

end:
	return ret;
}

static long dlb_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	struct dlb_dev *dev;
	u32 domain_id;
	int ret;

	dev = container_of(f->f_inode->i_cdev, struct dlb_dev, cdev);

	domain_id = DLB_FILE_ID_FROM_DEV_T(dlb_dev_number_base,
					   f->f_inode->i_rdev);

	if (!IS_DLB_DEV_FILE(dlb_dev_number_base, f->f_inode->i_rdev) &&
	    domain_id >= DLB_MAX_NUM_DOMAINS) {
		dev_err(dev->dlb_device,
			"[%s()] Internal error\n", __func__);
		return -EINVAL;
	}

	if (_IOC_TYPE(cmd) != DLB_IOC_MAGIC) {
		dev_err(dev->dlb_device,
			"[%s()] Bad magic number!\n", __func__);
		return -EINVAL;
	}

	if (IS_DLB_DEV_FILE(dlb_dev_number_base, f->f_inode->i_rdev))
		ret = dlb_ioctl_dispatcher(dev, cmd, arg);
	else
		ret = dlb_domain_ioctl_dispatcher(dev, f->private_data,
						  cmd, arg, domain_id);

	return ret;
}

static const struct file_operations dlb_fops = {
	.owner   = THIS_MODULE,
	.open    = dlb_open,
	.release = dlb_close,
	.read    = dlb_read,
	.write   = dlb_write,
	.mmap    = dlb_mmap,
	.unlocked_ioctl = dlb_ioctl,
	.compat_ioctl = compat_ptr_ioctl,
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

	INIT_LIST_HEAD(&dlb_dev->vma_list);

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

	ret = dlb_reset_device(pdev);
	if (ret)
		goto dlb_reset_fail;

	ret = dlb_dev->ops->init_driver_state(dlb_dev);
	if (ret)
		goto init_driver_state_fail;

	ret = dlb_resource_init(&dlb_dev->hw);
	if (ret)
		goto resource_init_fail;

	dlb_dev->ops->init_hardware(dlb_dev);

	/* The driver puts the device to sleep (D3hot) while there are no
	 * scheduling domains to service. The usage counter of a PCI device at
	 * probe time is 2, so decrement it twice here. (The PCI layer has
	 * already called pm_runtime_enable().)
	 */
	dlb_dev->ops->dec_pm_refcnt(pdev);
	dlb_dev->ops->dec_pm_refcnt(pdev);

	dlb_set_id_in_use(dlb_dev->id, true);

	return 0;

resource_init_fail:
	dlb_dev->ops->free_driver_state(dlb_dev);
init_driver_state_fail:
dlb_reset_fail:
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
	int i;

	/* Undo all the dlb_probe() operations */
	dev_dbg(&pdev->dev, "Cleaning up the DLB driver for removal\n");
	dlb_dev = pci_get_drvdata(pdev);

	dlb_set_id_in_use(dlb_dev->id, false);

	/* Undo the PM operations in dlb_probe(). */
	dlb_dev->ops->inc_pm_refcnt(pdev, false);
	dlb_dev->ops->inc_pm_refcnt(pdev, false);

	dlb_dev->ops->free_driver_state(dlb_dev);

	dlb_resource_free(&dlb_dev->hw);

	dlb_release_device_memory(dlb_dev);

	/* If a domain is created without its device file ever being opened, it
	 * needs to be destroyed here.
	 */
	for (i = 0; i < DLB_MAX_NUM_DOMAINS; i++) {
		struct dlb_status *status;

		status = dlb_dev->sched_domains[i].status;

		if (!status)
			continue;

		if (status->refcnt == 0)
			device_destroy(dlb_class,
				       MKDEV(MAJOR(dlb_dev->dev_number),
					     MINOR(dlb_dev->dev_number) +
						i));

		devm_kfree(dlb_dev->dlb_device, status);
	}

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

#ifdef CONFIG_PM
static int dlb_runtime_suspend(struct device *dev)
{
	struct pci_dev *pdev = container_of(dev, struct pci_dev, dev);
	struct dlb_dev *dlb_dev = pci_get_drvdata(pdev);

	dev_dbg(dlb_dev->dlb_device, "Suspending device operation\n");

	/* Return and let the PCI subsystem put the device in D3hot. */

	return 0;
}

static int dlb_runtime_resume(struct device *dev)
{
	struct pci_dev *pdev = container_of(dev, struct pci_dev, dev);
	struct dlb_dev *dlb_dev = pci_get_drvdata(pdev);

	/* The PCI subsystem put the device in D0, now reinitialize the
	 * device's state.
	 */

	dev_dbg(dlb_dev->dlb_device, "Resuming device operation\n");

	dlb_reset_hardware_state(dlb_dev);

	return 0;
}
#endif

static struct pci_device_id dlb_id_table[] = {
	{ PCI_DEVICE_DATA(INTEL, DLB_PF, DLB_PF) },
	{ 0 }
};
MODULE_DEVICE_TABLE(pci, dlb_id_table);

#ifdef CONFIG_PM
static const struct dev_pm_ops dlb_pm_ops = {
	SET_RUNTIME_PM_OPS(dlb_runtime_suspend, dlb_runtime_resume, NULL)
};
#endif

static struct pci_driver dlb_pci_driver = {
	.name		 = (char *)dlb_driver_name,
	.id_table	 = dlb_id_table,
	.probe		 = dlb_probe,
	.remove		 = dlb_remove,
#ifdef CONFIG_PM
	.driver.pm	 = &dlb_pm_ops,
#endif
};

static int __init dlb_init_module(void)
{
	int err;

	pr_info("%s - version %d.%d.%d\n", dlb_driver_name,
		DLB_VERSION_MAJOR_NUMBER,
		DLB_VERSION_MINOR_NUMBER,
		DLB_VERSION_REVISION_NUMBER);
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
