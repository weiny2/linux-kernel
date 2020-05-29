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

#include "dlb_intr.h"
#include "dlb_ioctl.h"
#include "dlb_main.h"
#include "dlb_mbox.h"
#include "dlb_mem.h"
#include "dlb_resource.h"
#include "dlb_sriov.h"

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

static unsigned int dlb_reset_timeout_s = DLB_DEFAULT_RESET_TIMEOUT_S;
module_param_named(reset_timeout_s, dlb_reset_timeout_s, uint, 0644);
MODULE_PARM_DESC(reset_timeout_s,
		 "Wait time (in seconds) after reset is requested given for app shutdown until driver zaps VMAs");

unsigned int dlb_qe_sa_pct = 1;
module_param_named(qe_sa_pct, dlb_qe_sa_pct, uint, 0444);
MODULE_PARM_DESC(qe_sa_pct,
		 "Percentage of QE selections that use starvation avoidance (SA) instead of strict priority. SA boosts one priority level for that selection; if there are no schedulable QEs of the boosted priority, the device selects according to normal priorities. Priorities 1-7 have an equal chance of being boosted when SA is used for QE selection. If SA is 0%, the device will use strict priority whenever possible. (Valid range: 0-100, default: 1)");

unsigned int dlb_qid_sa_pct;
module_param_named(qid_sa_pct, dlb_qid_sa_pct, uint, 0444);
MODULE_PARM_DESC(qid_sa_pct,
		 "Percentage of QID selections that use starvation avoidance (SA) instead of strict priority. SA boosts one priority level for that selection; if there are no schedulable QIDs of the boosted priority, the device selects according to normal priorities. Priorities 1-7 have an equal chance of being boosted when SA is used for QID selection. If SA is 0%, the device will use strict priority whenever possible. (Valid range: 0-100, default: 0)");

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

static int dlb_open_device_file(struct dlb_dev *dev, struct file *f)
{
	struct dlb_status *status;

	dev_dbg(dev->dlb_device, "Opening DLB device file\n");

	status = dev->status;

	if (!status) {
		status = devm_kzalloc(dev->dlb_device,
				      sizeof(*status),
				      GFP_KERNEL);
		if (!status)
			return -ENOMEM;

		status->valid = true;
		status->refcnt = 0;

		dev->status = status;
	}

	f->private_data = status;

	status->refcnt++;

	return 0;
}

static int dlb_open(struct inode *i, struct file *f)
{
	struct dlb_dev *dev;
	int ret = 0;

	dev = container_of(f->f_inode->i_cdev, struct dlb_dev, cdev);

	mutex_lock(&dev->resource_mutex);

	/* See dlb_reset_prepare() for more details */
	if (dev->reset_active) {
		dev_err(dev->dlb_device,
			"[%s()] The DLB is being reset; applications cannot use it during this time.\n",
			__func__);
		ret = -EINVAL;
		goto end;
	}

	if (!IS_DLB_DEV_FILE(dlb_dev_number_base, f->f_inode->i_rdev))
		ret = dlb_open_domain_device_file(dev, f);
	else
		ret = dlb_open_device_file(dev, f);
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

static void dlb_reset_hardware_state(struct dlb_dev *dev, bool issue_flr)
{
	if (issue_flr)
		dlb_reset_device(dev->pdev);

	/* Reinitialize interrupt configuration */
	dev->ops->reinit_interrupts(dev);

	/* Reset configuration done through the sysfs */
	dev->ops->sysfs_reapply(dev);

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

	dev->sched_domains[domain_id].alert_rd_idx = 0;
	dev->sched_domains[domain_id].alert_wr_idx = 0;

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
	} else {
		dev_dbg(dev->dlb_device, "Closing DLB device file\n");

		status->refcnt--;

		if (status->refcnt == 0) {
			devm_kfree(dev->dlb_device, status);
			dev->status = NULL;
		}

	}

	dev->ops->dec_pm_refcnt(dev->pdev);

end:
	mutex_unlock(&dev->resource_mutex);

	return ret;
}

int dlb_write_domain_alert(struct dlb_dev *dev,
			   u32 domain_id,
			   u64 alert_id,
			   u64 aux_alert_data)
{
	struct dlb_domain_dev *domain;
	struct dlb_domain_alert alert;
	int idx;

	if (domain_id >= DLB_MAX_NUM_DOMAINS) {
		dev_err(dev->dlb_device,
			"[%s()] Internal error\n", __func__);
		return -EINVAL;
	}

	domain = &dev->sched_domains[domain_id];

	/* Grab the alert mutex to access the read and write indexes */
	if (mutex_lock_interruptible(&domain->alert_mutex))
		return -ERESTARTSYS;

	/* If there's no space for this notification, return */
	if ((domain->alert_wr_idx - domain->alert_rd_idx) ==
	    (DLB_DOMAIN_ALERT_RING_SIZE - 1)) {
		mutex_unlock(&domain->alert_mutex);
		return 0;
	}

	alert.alert_id = alert_id;
	alert.aux_alert_data = aux_alert_data;

	idx = domain->alert_wr_idx % DLB_DOMAIN_ALERT_RING_SIZE;

	domain->alerts[idx] = alert;

	domain->alert_wr_idx++;

	mutex_unlock(&domain->alert_mutex);

	/* Wake any blocked readers */
	wake_up_interruptible(&domain->wq_head);

	return 0;
}

static bool dlb_domain_valid(struct dlb_dev *dev, struct dlb_status *status)
{
	bool ret;

	mutex_lock(&dev->resource_mutex);

	ret = status->valid;

	mutex_unlock(&dev->resource_mutex);

	return ret;
}

static bool dlb_domain_alerts_avail(struct dlb_domain_dev *domain)
{
	bool ret;

	mutex_lock(&domain->alert_mutex);

	ret = domain->alert_rd_idx != domain->alert_wr_idx;

	mutex_unlock(&domain->alert_mutex);

	return ret;
}

static int dlb_read_domain_alert(struct dlb_dev *dev,
				 struct dlb_status *status,
				 int domain_id,
				 struct dlb_domain_alert *alert,
				 bool nonblock)
{
	struct dlb_domain_dev *domain = &dev->sched_domains[domain_id];
	int idx;

	/* Grab the alert semaphore to access the read and write indexes */
	if (mutex_lock_interruptible(&domain->alert_mutex))
		return -ERESTARTSYS;

	while (domain->alert_rd_idx == domain->alert_wr_idx) {
		/* Release the alert semaphore before putting the thread on the
		 * wait queue.
		 */
		mutex_unlock(&domain->alert_mutex);

		if (nonblock)
			return -EWOULDBLOCK;

		dev_dbg(dev->dlb_device,
			"Thread %d is blocking waiting for an alert in domain %d\n",
			current->pid, domain_id);

		if (wait_event_interruptible(domain->wq_head,
					     dlb_domain_alerts_avail(domain) ||
					     !dlb_domain_valid(dev, status)))
			return -ERESTARTSYS;

		if (!dlb_domain_valid(dev, status)) {
			alert->alert_id = DLB_DOMAIN_ALERT_DEVICE_RESET;
			return 0;
		}

		if (mutex_lock_interruptible(&domain->alert_mutex))
			return -ERESTARTSYS;
	}

	/* The alert indexes are not equal, so there is an alert available. */
	idx = domain->alert_rd_idx % DLB_DOMAIN_ALERT_RING_SIZE;

	memcpy(alert, &domain->alerts[idx], sizeof(*alert));

	domain->alert_rd_idx++;

	mutex_unlock(&domain->alert_mutex);

	return 0;
}

static int dlb_check_and_inc_active_users(struct dlb_dev *dev,
					  struct dlb_status *status)
{
	mutex_lock(&dev->resource_mutex);

	if (!status->valid) {
		mutex_unlock(&dev->resource_mutex);
		return -EINVAL;
	}

	dev->active_users++;

	mutex_unlock(&dev->resource_mutex);

	return 0;
}

static void dlb_dec_active_users(struct dlb_dev *dev)
{
	mutex_lock(&dev->resource_mutex);

	dev->active_users--;

	mutex_unlock(&dev->resource_mutex);
}

static ssize_t dlb_read(struct file *f,
			char __user *buf,
			size_t len,
			loff_t *offset)
{
	struct dlb_status *status = f->private_data;
	struct dlb_domain_alert alert;
	struct dlb_dev *dev;
	u32 domain_id;
	int ret;

	if (IS_DLB_DEV_FILE(dlb_dev_number_base, f->f_inode->i_rdev))
		return 0;

	if (len != sizeof(alert))
		return -EINVAL;

	dev = container_of(f->f_inode->i_cdev, struct dlb_dev, cdev);

	domain_id = DLB_FILE_ID_FROM_DEV_T(dlb_dev_number_base,
					   f->f_inode->i_rdev);

	if (domain_id >= DLB_MAX_NUM_DOMAINS) {
		dev_err(dev->dlb_device,
			"[%s()] Internal error\n", __func__);
		return -EINVAL;
	}

	if (dlb_check_and_inc_active_users(dev, status)) {
		alert.alert_id = DLB_DOMAIN_ALERT_DEVICE_RESET;
		goto copy;
	}

	/* See dlb_user.h for details on domain alert notifications */

	ret = dlb_read_domain_alert(dev,
				    status,
				    domain_id,
				    &alert,
				    f->f_flags & O_NONBLOCK);

	dlb_dec_active_users(dev);

	if (ret)
		return ret;

copy:
	if (copy_to_user(buf, &alert, sizeof(alert)))
		return -EFAULT;

	dev_dbg(dev->dlb_device,
		"Thread %d received alert 0x%llx, with aux data 0x%llx\n",
		current->pid, ((u64 *)&alert)[0], ((u64 *)&alert)[1]);

	return sizeof(alert);
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

	ret = dlb_check_and_inc_active_users(dev, f->private_data);
	if (ret)
		return ret;

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
	dlb_dec_active_users(dev);

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

	ret = dlb_check_and_inc_active_users(dev, f->private_data);
	if (ret)
		return ret;

	if (IS_DLB_DEV_FILE(dlb_dev_number_base, f->f_inode->i_rdev))
		ret = dlb_ioctl_dispatcher(dev, cmd, arg);
	else
		ret = dlb_domain_ioctl_dispatcher(dev, f->private_data,
						  cmd, arg, domain_id);

	dlb_dec_active_users(dev);

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
	case DLB_VF:
		dlb_dev->ops = &dlb_vf_ops;
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

DEFINE_IDA(dlb_ids);

static int dlb_alloc_id(void)
{
	return ida_alloc_max(&dlb_ids, DLB_MAX_NUM_DEVICES - 1, GFP_KERNEL);
}

static void dlb_free_id(int id)
{
	ida_free(&dlb_ids, id);
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

	dlb_dev->id = dlb_alloc_id();
	if (dlb_dev->id < 0) {
		dev_err(&pdev->dev, "probe: device ID allocation failed\n");

		ret = dlb_dev->id;
		goto alloc_id_fail;
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

	/* (VF only) Register the driver with the PF driver */
	ret = dlb_dev->ops->register_driver(dlb_dev);
	if (ret)
		goto driver_registration_fail;

	/* If this is an auxiliary VF, it can skip the rest of the probe
	 * function. This VF is only used for its MSI interrupt vectors, and
	 * the VF's register_driver callback will initialize them.
	 */
	if (dlb_dev->type == DLB_VF && dlb_dev->vf_id_state.is_auxiliary_vf)
		goto aux_vf_probe;

	ret = dlb_dev->ops->cdev_add(dlb_dev, dlb_dev_number_base, &dlb_fops);
	if (ret)
		goto cdev_add_fail;

	ret = dlb_dev->ops->device_create(dlb_dev, pdev, dlb_class);
	if (ret)
		goto device_add_fail;

	ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
	if (ret)
		goto dma_set_mask_fail;

	ret = dlb_dev->ops->sysfs_create(dlb_dev);
	if (ret)
		goto sysfs_create_fail;

	ret = dlb_reset_device(pdev);
	if (ret)
		goto dlb_reset_fail;

	ret = dlb_dev->ops->init_interrupts(dlb_dev, pdev);
	if (ret)
		goto init_interrupts_fail;

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

aux_vf_probe:
	return 0;

resource_init_fail:
	dlb_dev->ops->free_driver_state(dlb_dev);
init_driver_state_fail:
	dlb_dev->ops->free_interrupts(dlb_dev, pdev);
init_interrupts_fail:
dlb_reset_fail:
	dlb_dev->ops->sysfs_destroy(dlb_dev);
sysfs_create_fail:
dma_set_mask_fail:
	dlb_dev->ops->device_destroy(dlb_dev, dlb_class);
device_add_fail:
	dlb_dev->ops->cdev_del(dlb_dev);
cdev_add_fail:
	dlb_dev->ops->unregister_driver(dlb_dev);
driver_registration_fail:
	dlb_dev->ops->unmap_pci_bar_space(dlb_dev, pdev);
map_pci_bar_fail:
	pci_disable_pcie_error_reporting(pdev);
	pci_release_regions(pdev);
pci_request_regions_fail:
	pci_disable_device(pdev);
pci_enable_device_fail:
	dlb_free_id(dlb_dev->id);
alloc_id_fail:
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

	/* If this is an auxiliary VF, it skipped past most of the probe code */
	if (dlb_dev->type == DLB_VF && dlb_dev->vf_id_state.is_auxiliary_vf)
		goto aux_vf_remove;

	/* Attempt to remove VFs before taking down the PF, since VFs cannot
	 * operate without a PF driver (in part because hardware doesn't
	 * support (CMD.MEM == 0 && IOV_CTRL.MSE == 1)).
	 */
	if (!pdev->is_virtfn && pci_num_vf(pdev) &&
	    dlb_pci_sriov_configure(pdev, 0))
		dev_err(&pdev->dev,
			"Warning: DLB VFs will become unusable when the PF driver is removed\n");

	/* Undo the PM operations in dlb_probe(). */
	dlb_dev->ops->inc_pm_refcnt(pdev, false);
	dlb_dev->ops->inc_pm_refcnt(pdev, false);

	dlb_dev->ops->free_driver_state(dlb_dev);

	dlb_dev->ops->free_interrupts(dlb_dev, pdev);

	dlb_resource_free(&dlb_dev->hw);

	dlb_release_device_memory(dlb_dev);

	dlb_dev->ops->sysfs_destroy(dlb_dev);

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

aux_vf_remove:
	dlb_dev->ops->unmap_pci_bar_space(dlb_dev, pdev);

	pci_disable_pcie_error_reporting(pdev);

	pci_release_regions(pdev);

	pci_disable_device(pdev);

	dlb_free_id(dlb_dev->id);

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

	dlb_reset_hardware_state(dlb_dev, true);

	return 0;
}
#endif

static struct pci_device_id dlb_id_table[] = {
	{ PCI_DEVICE_DATA(INTEL, DLB_PF, DLB_PF) },
	{ PCI_DEVICE_DATA(INTEL, DLB_VF, DLB_VF) },
	{ 0 }
};
MODULE_DEVICE_TABLE(pci, dlb_id_table);

static int dlb_vfs_in_use(struct dlb_dev *dev)
{
	int i;

	/* For each VF with 1+ domains configured, query whether it is still in
	 * use, where "in use" is determined by the VF calling dlb_in_use().
	 */

	for (i = 0; i < DLB_MAX_NUM_VFS; i++) {
		struct dlb_get_num_resources_args used_rsrcs;

		dlb_hw_get_num_used_resources(&dev->hw, &used_rsrcs, true, i);

		if (!used_rsrcs.num_sched_domains)
			continue;

		if (dlb_vf_in_use(&dev->hw, i))
			return true;
	}

	return false;
}

static int dlb_total_device_file_refcnt(struct dlb_dev *dev)
{
	int cnt = 0, i;

	for (i = 0; i < DLB_MAX_NUM_DOMAINS; i++)
		if (dev->sched_domains[i].status)
			cnt += dev->sched_domains[i].status->refcnt;

	if (dev->status)
		cnt += dev->status->refcnt;

	return cnt;
}

bool dlb_in_use(struct dlb_dev *dev)
{
	return ((dev->type == DLB_PF && dlb_vfs_in_use(dev)) ||
		dlb_total_device_file_refcnt(dev) != 0 ||
		(dev->active_users > 0) ||
		!list_empty(&dev->vma_list));
}

static void dlb_wait_for_idle(struct dlb_dev *dlb_dev)
{
	int i;

	for (i = 0; i < dlb_reset_timeout_s * 10; i++) {
		bool idle;

		mutex_lock(&dlb_dev->resource_mutex);

		/* Check for any application threads in the driver, extant
		 * mmaps, or open device files.
		 */
		idle = !dlb_in_use(dlb_dev);

		mutex_unlock(&dlb_dev->resource_mutex);

		if (idle)
			return;

		cond_resched();
		msleep(100);
	}

	dev_err(dlb_dev->dlb_device,
		"PF driver timed out waiting for applications to idle\n");
}

void dlb_zap_vma_entries(struct dlb_dev *dlb_dev)
{
	struct dlb_vma_node *vma_node;
	struct list_head *node;

	dev_dbg(dlb_dev->dlb_device, "Zapping memory mappings\n");

	list_for_each(node, &dlb_dev->vma_list) {
		unsigned long size;

		vma_node = list_entry(node, struct dlb_vma_node, list);

		size = vma_node->vma->vm_end - vma_node->vma->vm_start;

		zap_vma_ptes(vma_node->vma, vma_node->vma->vm_start, size);
	}
}

static unsigned int dlb_get_vf_dev_id_from_pf(struct pci_dev *pdev)
{
	switch (pdev->device) {
	case PCI_DEVICE_ID_INTEL_DLB_PF:
		return PCI_DEVICE_ID_INTEL_DLB_VF;
	default:
		return -1;
	}
}

#ifdef CONFIG_PCI_ATS
static void dlb_save_pfs_vf_config_state(struct pci_dev *pdev)
{
	struct pci_dev *vfdev = NULL;
	unsigned int device_id;

	device_id = dlb_get_vf_dev_id_from_pf(pdev);
	if (device_id == -1)
		return;

	/* Find all this PF's VFs */
	vfdev = pci_get_device(pdev->vendor, device_id, NULL);

	while (vfdev) {
		int ret = 0;

		if (vfdev->is_virtfn && vfdev->physfn == pdev)
			ret = pci_save_state(vfdev);
		if (ret)
			dev_err(&pdev->dev,
				"[%s()] Internal error: PCI save state failed\n",
				__func__);

		vfdev = pci_get_device(pdev->vendor, device_id, vfdev);
	}
}

static void dlb_restore_pfs_vf_config_state(struct pci_dev *pdev)
{
	unsigned int device_id;
	struct pci_dev *vfdev;

	device_id = dlb_get_vf_dev_id_from_pf(pdev);
	if (device_id == -1)
		return;

	/* Find all this PF's VFs */
	vfdev = pci_get_device(pdev->vendor, device_id, NULL);

	while (vfdev) {
		if (vfdev->is_virtfn && vfdev->physfn == pdev)
			pci_restore_state(vfdev);

		vfdev = pci_get_device(pdev->vendor, device_id, vfdev);
	}
}
#else
static void dlb_save_pfs_vf_config_state(struct pci_dev *pdev)
{
}

static void dlb_restore_pfs_vf_config_state(struct pci_dev *pdev)
{
}
#endif

static void dlb_clr_aer_vf_hdr_log(struct pci_dev *pdev)
{
	struct pci_dev *vfdev = NULL;
	unsigned int device_id;

	device_id = dlb_get_vf_dev_id_from_pf(pdev);
	if (device_id == -1)
		return;

	/* Find all this PF's VFs */
	vfdev = pci_get_device(pdev->vendor, device_id, NULL);

	/* For each VF, clear its 16B AER header log. This workaround is needed
	 * for A-stepping devices only.
	 */
	while (vfdev) {
		int pos = pci_find_ext_capability(vfdev, PCI_EXT_CAP_ID_ERR);

		pci_write_config_dword(vfdev, pos + PCI_ERR_HEADER_LOG, 0);
		pci_write_config_dword(vfdev, pos + PCI_ERR_HEADER_LOG + 4, 0);
		pci_write_config_dword(vfdev, pos + PCI_ERR_HEADER_LOG + 8, 0);
		pci_write_config_dword(vfdev, pos + PCI_ERR_HEADER_LOG + 12, 0);

		vfdev = pci_get_device(pdev->vendor, device_id, vfdev);
	}
}

static void dlb_disable_device_files(struct dlb_dev *dlb_dev)
{
	int i;

	/* Set all status->valid flags to false to prevent existing device
	 * files from being used to enter the device driver.
	 */
	for (i = 0; i < DLB_MAX_NUM_DOMAINS; i++) {
		struct dlb_status *status;

		status = dlb_dev->sched_domains[i].status;

		if (!status)
			continue;

		status->valid = false;

		device_destroy(dlb_class,
			       MKDEV(MAJOR(dlb_dev->dev_number),
				     MINOR(dlb_dev->dev_number) + i));

		/* If the domain device file was created but never opened, free
		 * the file private memory here. Otherwise, it will be freed
		 * in the file close callback when refcnt reaches 0.
		 */
		if (status->refcnt == 0)
			devm_kfree(dlb_dev->dlb_device, status);
	}

	if (dlb_dev->status)
		dlb_dev->status->valid = false;
}

static void dlb_wake_threads(struct dlb_dev *dlb_dev)
{
	int i;

	/* Wake any blocked device file readers. These threads will return the
	 * DLB_DOMAIN_ALERT_DEVICE_RESET alert, and well-behaved applications
	 * will close their fds and unmap DLB memory as a result.
	 */
	for (i = 0; i < DLB_MAX_NUM_DOMAINS; i++)
		wake_up_interruptible(&dlb_dev->sched_domains[i].wq_head);

	/* Wake threads blocked on a CQ interrupt */
	for (i = 0; i < DLB_MAX_NUM_LDB_PORTS; i++)
		dlb_wake_thread(dlb_dev,
				&dlb_dev->intr.ldb_cq_intr[i],
				WAKE_DEV_RESET);

	for (i = 0; i < DLB_MAX_NUM_DIR_PORTS; i++)
		dlb_wake_thread(dlb_dev,
				&dlb_dev->intr.dir_cq_intr[i],
				WAKE_DEV_RESET);
}

void dlb_stop_users(struct dlb_dev *dlb_dev)
{
	/* Disable existing device files to prevent applications from enter the
	 * device driver through file operations. (New files can't be opened
	 * while the resource mutex is held.)
	 */
	dlb_disable_device_files(dlb_dev);

	/* Wake any threads blocked in the kernel */
	dlb_wake_threads(dlb_dev);
}

static void dlb_reset_prepare(struct pci_dev *pdev)
{
	/* A device FLR can be triggered while applications are actively using
	 * the device, which poses two problems:
	 * - If applications continue to enqueue to the hardware they will
	 *   cause hardware errors, because the FLR will have reset the
	 *   scheduling domains, ports, and queues.
	 * - When the applications end, they must not trigger the driver's
	 *   domain reset logic, which would fail because the device's
	 *   registers will have been reset by the FLR.
	 *
	 * The driver needs to stop applications from using the device before
	 * the FLR begins, and detect when these applications exit (so they
	 * don't trigger the domain reset that occurs when their last file
	 * reference (or memory mapping) closes). The driver must also disallow
	 * applications from configuring the device while the reset is in
	 * progress.
	 *
	 * To avoid these problems, the driver handles unexpected resets as
	 * follows:
	 * 1. Set the reset_active flag. This flag blocks new device files from
	 *    being opened and is used as a wakeup condition in the driver's
	 *    wait queues.
	 * 2. If this is a PF FLR and there are active VFs, send them a
	 *    pre-reset notification, so they can stop any VF applications.
	 * 3. Disable all device files (set the per-file valid flag to false,
	 *    which prevents the file from being used after FLR completes) and
	 *    wake any threads blocked on a wait queue.
	 * 4. If the DLB is not in use -- i.e. no open device files or memory
	 *    mappings, and no VFs in use (PF FLR only) -- the FLR can begin.
	 * 5. Else, the driver waits (up to a user-specified timeout, default
	 *    5s) for software to stop using the driver and the device. If the
	 *    timeout elapses, the driver zaps any remaining MMIO mappings so
	 *    the extant applications can no longer use the device.
	 * 6. Do not unlock the resource mutex at the end of the reset prepare
	 *    function, to prevent device access during the reset.
	 *
	 * After the FLR:
	 * 1. Clear the device and domain status pointers (the memory is freed
	 *    elsewhere).
	 * 2. Release any remaining allocated port or CQ memory, now that it's
	 *    guaranteed the device is unconfigured and won't write to memory.
	 * 3. Reset software and hardware state.
	 * 4. Notify VFs that the FLR is complete.
	 * 5. Set reset_active to false.
	 * 6. Unlock the resource mutex.
	 */

	struct dlb_dev *dlb_dev;

	dlb_dev = pci_get_drvdata(pdev);

	dev_dbg(dlb_dev->dlb_device, "DLB driver reset prepare\n");

	mutex_lock(&dlb_dev->resource_mutex);

	/* Block any new device files from being opened */
	dlb_dev->reset_active = true;

	/* If the device has 1+ VFs, even if they're not in use, it will not be
	 * suspended. To avoid having to handle two cases (reset while device
	 * suspended and reset while device active), increment the device's PM
	 * refcnt here, to guarantee that the device is in D0 for the duration
	 * of the reset.
	 */
	dlb_dev->ops->inc_pm_refcnt(pdev, true);

	/* Notify all registered VF drivers so they stop their applications
	 * from attempting to use the VF while the PF FLR is in progress.
	 */
	if (dlb_dev->type == DLB_PF) {
		enum dlb_mbox_vf_notification_type notif;
		int i;

		notif = DLB_MBOX_VF_NOTIFICATION_PRE_RESET;

		for (i = 0; i < DLB_MAX_NUM_VFS; i++) {
			if (dlb_is_registered_vf(dlb_dev, i))
				dlb_notify_vf(&dlb_dev->hw, i, notif);
		}
	}

	/* PF FLR resets the VF config space as well, so save it here. We need
	 * to restore their config spaces after the FLR in order to send the
	 * post-reset mailbox message (since a reset VF config space loses its
	 * MSI configuration).
	 */
	if (dlb_dev->type == DLB_PF)
		dlb_save_pfs_vf_config_state(pdev);

	/* Stop existing applications from continuing to use the device by
	 * blocking kernel driver interfaces and waking any threads on wait
	 * queues, but don't zap VMA entries yet. If this is a PF FLR, notify
	 * any VFs of the impending FLR so they can stop their users as well.
	 */
	dlb_stop_users(dlb_dev);

	/* If no software is using the device, there's nothing to clean up. */
	if (!dlb_in_use(dlb_dev))
		return;

	dev_dbg(dlb_dev->dlb_device, "Waiting for users to stop\n");

	/* Release the resource mutex so threads can complete their work and
	 * exit the driver
	 */
	mutex_unlock(&dlb_dev->resource_mutex);

	/* Wait until the device is idle or dlb_reset_timeout_s seconds elapse.
	 * If the timeout occurs, zap any remaining VMA entries to guarantee
	 * applications can't reach the device.
	 */
	dlb_wait_for_idle(dlb_dev);

	mutex_lock(&dlb_dev->resource_mutex);

	if (!dlb_in_use(dlb_dev))
		return;

	dlb_zap_vma_entries(dlb_dev);

	if (dlb_dev->active_users > 0)
		dev_err(dlb_dev->dlb_device,
			"Internal error: %lu active_users in the driver during FLR\n",
			dlb_dev->active_users);

	/* Don't release resource_mutex until after the FLR occurs. This
	 * prevents applications from accessing the device during reset.
	 */
}

static void dlb_reset_done(struct pci_dev *pdev)
{
	struct dlb_dev *dlb_dev;
	int i;

	dlb_dev = pci_get_drvdata(pdev);

	/* Clear all status pointers, to be filled in by post-FLR applications
	 * using the device driver.
	 *
	 * Note that status memory isn't leaked -- it is either freed during
	 * dlb_stop_users() or in the file close callback.
	 */
	for (i = 0; i < DLB_MAX_NUM_DOMAINS; i++)
		dlb_dev->sched_domains[i].status = NULL;

	dlb_dev->status = NULL;

	/* Free allocated CQ and PC memory. These are no longer accessible to
	 * user-space: either the applications closed, or their mappings were
	 * zapped in dlb_reset_prepare().
	 */
	dlb_release_device_memory(dlb_dev);

	/* Reset resource allocation state */
	dlb_resource_reset(&dlb_dev->hw);

	/* Reset the hardware state, but don't issue an additional FLR */
	dlb_reset_hardware_state(dlb_dev, false);

	/* VF FLR is a software procedure executed by the PF driver. Wait
	 * until the PF indicates that the VF reset is done.
	 */
	if (dlb_dev->type == DLB_VF) {
		int retry_cnt;

		/* Timeout after DLB_VF_FLR_DONE_POLL_TIMEOUT_MS of inactivity,
		 * sleep-polling every DLB_VF_FLR_DONE_SLEEP_PERIOD_MS.
		 */
		retry_cnt = DLB_VF_FLR_DONE_POLL_TIMEOUT_MS;
		do {
			unsigned long sleep_us;

			if (dlb_vf_flr_complete(&dlb_dev->hw))
				break;

			sleep_us = DLB_VF_FLR_DONE_SLEEP_PERIOD_MS * 1000;

			usleep_range(sleep_us, sleep_us + 1);
		} while (--retry_cnt);

		if (!retry_cnt)
			dev_err(dlb_dev->dlb_device,
				"VF driver timed out waiting for FLR response\n");

		if (dlb_dev->revision < DLB_REV_B0)
			dlb_vf_ack_vf_flr_done(&dlb_dev->hw);
	}

	if (dlb_dev->type == DLB_PF && dlb_dev->revision < DLB_REV_B0)
		dlb_clr_aer_vf_hdr_log(pdev);

	/* Restore the PF's VF config spaces in order to send the post-reset
	 * mailbox message.
	 */
	if (dlb_dev->type == DLB_PF)
		dlb_restore_pfs_vf_config_state(pdev);

	/* Notify all registered VF drivers */
	if (dlb_dev->type == DLB_PF) {
		enum dlb_mbox_vf_notification_type notif;
		int i;

		notif = DLB_MBOX_VF_NOTIFICATION_POST_RESET;

		for (i = 0; i < DLB_MAX_NUM_VFS; i++) {
			if (dlb_is_registered_vf(dlb_dev, i))
				dlb_notify_vf(&dlb_dev->hw, i, notif);
		}
	}

	dev_dbg(dlb_dev->dlb_device, "DLB driver reset done\n");

	dlb_dev->domain_reset_failed = false;

	dlb_dev->reset_active = false;

	/* Undo the PM refcnt increment in dlb_reset_prepare(). */
	dlb_dev->ops->dec_pm_refcnt(dlb_dev->pdev);

	mutex_unlock(&dlb_dev->resource_mutex);
}

static const struct pci_error_handlers dlb_err_handler = {
	.reset_prepare = dlb_reset_prepare,
	.reset_done    = dlb_reset_done,
};

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
	.sriov_configure = dlb_pci_sriov_configure,
	.err_handler     = &dlb_err_handler,
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
