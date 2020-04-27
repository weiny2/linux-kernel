// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2018-2020 Intel Corporation */

#include <linux/aer.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/pci-ats.h>
#include <linux/pm_runtime.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#include <uapi/linux/dlb2_user.h>

#include "base/dlb2_mbox.h"
#include "base/dlb2_resource.h"
#include "dlb2_intr.h"
#include "dlb2_ioctl.h"
#include "dlb2_main.h"
#include "dlb2_sriov.h"

#ifdef CONFIG_INTEL_DLB2_DATAPATH
#include "dlb2_dp_priv.h"
#endif

static const char
dlb2_driver_copyright[] = "Copyright(c) 2018-2020 Intel Corporation";

#define TO_STR2(s) #s
#define TO_STR(s) TO_STR2(s)

#define DRV_VERSION \
	TO_STR(DLB2_VERSION_MAJOR_NUMBER) "." \
	TO_STR(DLB2_VERSION_MINOR_NUMBER) "." \
	TO_STR(DLB2_VERSION_REVISION_NUMBER)

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Copyright(c) 2018-2020 Intel Corporation");
MODULE_DESCRIPTION("Intel(R) Dynamic Load Balancer 2.0 Driver");
MODULE_VERSION(DRV_VERSION);

static unsigned int dlb2_reset_timeout_s = DLB2_DEFAULT_RESET_TIMEOUT_S;
module_param_named(reset_timeout_s, dlb2_reset_timeout_s, uint, 0644);
MODULE_PARM_DESC(reset_timeout_s,
		 "Wait time (in seconds) after reset is requested given for app shutdown until driver zaps VMAs");
bool dlb2_pasid_override;
module_param_named(pasid_override, dlb2_pasid_override, bool, 0444);
MODULE_PARM_DESC(pasid_override, "Override allocated PASID with 0");
bool dlb2_wdto_disable;
module_param_named(wdto_disable, dlb2_wdto_disable, bool, 0444);
MODULE_PARM_DESC(wdto_disable, "Disable per-CQ watchdog timers");

unsigned int dlb2_qe_sa_pct = 1;
module_param_named(qe_sa_pct, dlb2_qe_sa_pct, uint, 0444);
MODULE_PARM_DESC(qe_sa_pct,
		 "Percentage of QE selections that use starvation avoidance (SA) instead of strict priority. SA boosts one priority level for that selection; if there are no schedulable QEs of the boosted priority, the device selects according to normal priorities. Priorities 1-7 have an equal chance of being boosted when SA is used for QE selection. If SA is 0%, the device will use strict priority whenever possible. (Valid range: 0-100, default: 1)");
unsigned int dlb2_qid_sa_pct;
module_param_named(qid_sa_pct, dlb2_qid_sa_pct, uint, 0444);
MODULE_PARM_DESC(qid_sa_pct,
		 "Percentage of QID selections that use starvation avoidance (SA) instead of strict priority. SA boosts one priority level for that selection; if there are no schedulable QIDs of the boosted priority, the device selects according to normal priorities. Priorities 1-7 have an equal chance of being boosted when SA is used for QID selection. If SA is 0%, the device will use strict priority whenever possible. (Valid range: 0-100, default: 0)");

/* The driver lock protects data structures that used by multiple devices. */
DEFINE_MUTEX(dlb2_driver_lock);
struct list_head dlb2_dev_list = LIST_HEAD_INIT(dlb2_dev_list);

static struct class *dlb2_class;
static dev_t dlb2_dev_number_base;

/*****************************/
/****** Devfs callbacks ******/
/*****************************/

static int dlb2_open_domain_device_file(struct dlb2_dev *dev, struct file *f)
{
	struct dlb2_domain_dev *domain;
	u32 domain_id;

	domain_id = DLB2_FILE_ID_FROM_DEV_T(dlb2_dev_number_base,
					    f->f_inode->i_rdev);

	if (domain_id >= DLB2_MAX_NUM_DOMAINS) {
		dev_err(dev->dlb2_device,
			"[%s()] Internal error\n", __func__);
		return -EINVAL;
	}

	dev_dbg(dev->dlb2_device,
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

static int dlb2_open_device_file(struct dlb2_dev *dev, struct file *f)
{
	struct dlb2_status *status;

	dev_dbg(dev->dlb2_device, "Opening DLB device file\n");

	status = dev->status;

	if (!status) {
		status = devm_kzalloc(dev->dlb2_device,
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

static int dlb2_open(struct inode *i, struct file *f)
{
	struct dlb2_dev *dev;
	int ret = 0;

	dev = container_of(f->f_inode->i_cdev, struct dlb2_dev, cdev);

	mutex_lock(&dev->resource_mutex);

	/* See dlb2_reset_notify() for more details */
	if (dev->reset_active) {
		dev_err(dev->dlb2_device,
			"[%s()] The DLB is being reset; applications cannot use it during this time.\n",
			__func__);
		ret = -EINVAL;
		goto end;
	}

	if (!IS_DLB2_DEV_FILE(dlb2_dev_number_base, f->f_inode->i_rdev))
		ret = dlb2_open_domain_device_file(dev, f);
	else
		ret = dlb2_open_device_file(dev, f);
	if (ret)
		goto end;

	dev->ops->inc_pm_refcnt(dev->pdev, true);

end:
	mutex_unlock(&dev->resource_mutex);

	return ret;
}

int dlb2_add_domain_device_file(struct dlb2_dev *dlb2_dev, u32 domain_id)
{
	struct dlb2_status *status;
	struct device *dev;

	dev_dbg(dlb2_dev->dlb2_device,
		"Creating domain %d's device file\n", domain_id);

	status = devm_kzalloc(dlb2_dev->dlb2_device,
			      sizeof(*status),
			      GFP_KERNEL);
	if (!status)
		return -ENOMEM;

	status->valid = true;
	status->refcnt = 0;

	/* Create a new device in order to create a /dev/ domain node. This
	 * device is a child of the DLB PCI device.
	 */
	dev = device_create(dlb2_class,
			    dlb2_dev->dlb2_device->parent,
			    MKDEV(MAJOR(dlb2_dev->dev_number),
				  MINOR(dlb2_dev->dev_number) + domain_id),
			    dlb2_dev,
			    "dlb%d/domain%d",
			    DLB2_DEV_ID_FROM_DEV_T(dlb2_dev_number_base,
						   dlb2_dev->dev_number),
			    domain_id);

	if (IS_ERR_VALUE(PTR_ERR(dev))) {
		dev_err(dlb2_dev->dlb2_device,
			"%s: device_create() returned %ld\n",
			dlb2_driver_name, PTR_ERR(dev));

		devm_kfree(dlb2_dev->dlb2_device, status);
		return PTR_ERR(dev);
	}

	dlb2_dev->sched_domains[domain_id].status = status;

	return 0;
}

static int dlb2_reset_device(struct pci_dev *pdev)
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

static void dlb2_reset_hardware_state(struct dlb2_dev *dev, bool issue_flr)
{
	if (issue_flr)
		dlb2_reset_device(dev->pdev);

	/* Reinitialize interrupt configuration */
	dev->ops->reinit_interrupts(dev);

	/* Reset configuration done through the sysfs */
	dev->ops->sysfs_reapply(dev);

	/* Reinitialize any other hardware state */
	dev->ops->init_hardware(dev);
}

static void dlb2_release_domain_memory(struct dlb2_dev *dev, u32 domain_id)
{
	struct dlb2_port_memory *port_mem;
	struct dlb2_domain_dev *domain;
	int i;

	domain = &dev->sched_domains[domain_id];

	dev_dbg(dev->dlb2_device,
		"Releasing memory allocated for domain %d\n", domain_id);

	for (i = 0; i < DLB2_MAX_NUM_LDB_PORTS; i++) {
		port_mem = &dev->ldb_port_mem[i];

		if (port_mem->domain_id == domain_id && port_mem->valid) {
			dma_free_coherent(&dev->pdev->dev,
					  DLB2_LDB_CQ_MAX_SIZE,
					  port_mem->cq_base,
					  port_mem->cq_dma_base);

			port_mem->valid = false;
		}
	}

	for (i = 0; i < DLB2_MAX_NUM_DIR_PORTS; i++) {
		port_mem = &dev->dir_port_mem[i];

		if (port_mem->domain_id == domain_id && port_mem->valid) {
			dma_free_coherent(&dev->pdev->dev,
					  DLB2_DIR_CQ_MAX_SIZE,
					  port_mem->cq_base,
					  port_mem->cq_dma_base);

			port_mem->valid = false;
		}
	}
}

void dlb2_release_device_memory(struct dlb2_dev *dev)
{
	int i;

	for (i = 0; i < DLB2_MAX_NUM_DOMAINS; i++)
		dlb2_release_domain_memory(dev, i);
}

static int dlb2_total_device_file_refcnt(struct dlb2_dev *dev)
{
	int cnt = 0, i;

	for (i = 0; i < DLB2_MAX_NUM_DOMAINS; i++)
		if (dev->sched_domains[i].status)
			cnt += dev->sched_domains[i].status->refcnt;

	if (dev->status)
		cnt += dev->status->refcnt;

	return cnt;
}

int dlb2_close_domain_device_file(struct dlb2_dev *dev,
				  struct dlb2_status *status,
				  u32 domain_id,
				  bool skip_reset)
{
	bool valid = status->valid;
	int ret = 0;

	devm_kfree(dev->dlb2_device, status);

#ifdef CONFIG_INTEL_DLB2_DATAPATH
	dev->sched_domains[domain_id].dp = NULL;
#endif

	dev->sched_domains[domain_id].alert_rd_idx = 0;
	dev->sched_domains[domain_id].alert_wr_idx = 0;

	/* Check if the domain was reset, its device file destroyed, and its
	 * memory released during FLR handling.
	 */
	if (!valid)
		return 0;

	dev->sched_domains[domain_id].status = NULL;

	device_destroy(dlb2_class,
		       MKDEV(MAJOR(dev->dev_number),
			     MINOR(dev->dev_number) +
			     domain_id));

	if (!skip_reset)
		ret = dev->ops->reset_domain(&dev->hw, domain_id);

	/* Unpin all memory pages associated with the domain */
	dlb2_release_domain_memory(dev, domain_id);

	if (ret) {
		dev->domain_reset_failed = true;
		dev_err(dev->dlb2_device,
			"Internal error: Domain reset failed. To recover, reset the device.\n");
	}

	return ret;
}

static int dlb2_close(struct inode *i, struct file *f)
{
	struct dlb2_status *status = f->private_data;
	struct dlb2_dev *dev;
	int ret = 0;

	dev = container_of(f->f_inode->i_cdev, struct dlb2_dev, cdev);

	mutex_lock(&dev->resource_mutex);

	if (!IS_DLB2_DEV_FILE(dlb2_dev_number_base, f->f_inode->i_rdev)) {
		struct dlb2_domain_dev *domain;
		u32 domain_id;

		domain_id = DLB2_FILE_ID_FROM_DEV_T(dlb2_dev_number_base,
						    f->f_inode->i_rdev);

		if (domain_id >= DLB2_MAX_NUM_DOMAINS) {
			dev_err(dev->dlb2_device,
				"[%s()] Internal error\n", __func__);
			ret = -1;
			goto end;
		}

		domain = &dev->sched_domains[domain_id];

		dev_dbg(dev->dlb2_device,
			"Closing domain %d's device file\n", domain_id);

		status->refcnt--;

		if (status->refcnt == 0)
			ret = dlb2_close_domain_device_file(dev,
							    status,
							    domain_id,
							    false);
	} else {
		dev_dbg(dev->dlb2_device, "Closing DLB device file\n");

		status->refcnt--;

		if (status->refcnt == 0) {
			devm_kfree(dev->dlb2_device, status);
			dev->status = NULL;
		}
	}

	dev->ops->dec_pm_refcnt(dev->pdev);

end:
	mutex_unlock(&dev->resource_mutex);

	return ret;
}

static bool dlb2_domain_valid(struct dlb2_dev *dev,
			      struct dlb2_status *status)
{
	bool ret;

	mutex_lock(&dev->resource_mutex);

	ret = status->valid;

	mutex_unlock(&dev->resource_mutex);

	return ret;
}

static bool dlb2_alerts_avail(struct dlb2_domain_dev *domain)
{
	bool ret;

	mutex_lock(&domain->alert_mutex);

	ret = domain->alert_rd_idx != domain->alert_wr_idx;

	mutex_unlock(&domain->alert_mutex);

	return ret;
}

int dlb2_read_domain_alert(struct dlb2_dev *dev,
			   struct dlb2_status *status,
			   int domain_id,
			   struct dlb2_domain_alert *alert,
			   bool nonblock)
{
	struct dlb2_domain_dev *domain = &dev->sched_domains[domain_id];

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

		dev_dbg(dev->dlb2_device,
			"Thread %d is blocking waiting for an alert in domain %d\n",
			current->pid, domain_id);

		if (wait_event_interruptible(domain->wq_head,
					     dlb2_alerts_avail(domain) ||
					     !dlb2_domain_valid(dev, status)))
			return -ERESTARTSYS;

		/* See dlb2_reset_notify() for more details */
		if (!dlb2_domain_valid(dev, status)) {
			alert->alert_id = DLB2_DOMAIN_ALERT_DEVICE_RESET;
			return 0;
		}

		if (mutex_lock_interruptible(&domain->alert_mutex))
			return -ERESTARTSYS;
	}

	/* The alert indexes are not equal, so there is an alert available. */
	memcpy(alert, &domain->alerts[domain->alert_rd_idx], sizeof(*alert));

	domain->alert_rd_idx++;

	mutex_unlock(&domain->alert_mutex);

	return 0;
}

static int dlb2_check_and_inc_active_users(struct dlb2_dev *dev,
					   struct dlb2_status *status)
{
	mutex_lock(&dev->resource_mutex);

	if (status && !status->valid) {
		mutex_unlock(&dev->resource_mutex);
		return -EINVAL;
	}

	dev->active_users++;

	mutex_unlock(&dev->resource_mutex);

	return 0;
}

static void dlb2_dec_active_users(struct dlb2_dev *dev)
{
	mutex_lock(&dev->resource_mutex);

	dev->active_users--;

	mutex_unlock(&dev->resource_mutex);
}

static ssize_t dlb2_read(struct file *f,
			 char __user *buf,
			 size_t len,
			 loff_t *offset)
{
	struct dlb2_status *status = f->private_data;
	struct dlb2_domain_alert alert;
	struct dlb2_dev *dev;
	u32 domain_id;
	int ret;

	if (IS_DLB2_DEV_FILE(dlb2_dev_number_base, f->f_inode->i_rdev))
		return 0;

	if (len != sizeof(alert))
		return -EINVAL;

	domain_id = DLB2_FILE_ID_FROM_DEV_T(dlb2_dev_number_base,
					    f->f_inode->i_rdev);

	dev = container_of(f->f_inode->i_cdev, struct dlb2_dev, cdev);

	if (dlb2_check_and_inc_active_users(dev, status)) {
		alert.alert_id = DLB2_DOMAIN_ALERT_DEVICE_RESET;
		goto copy;
	}

	/* See dlb2_user.h for details on domain alert notifications */

	ret = dlb2_read_domain_alert(dev,
				     status,
				     domain_id,
				     &alert,
				     f->f_flags & O_NONBLOCK);

	dlb2_dec_active_users(dev);

	if (ret)
		return ret;

copy:
	if (copy_to_user(buf, &alert, sizeof(alert)))
		return -EFAULT;

	dev_dbg(dev->dlb2_device,
		"Thread %d received alert 0x%llx, with aux data 0x%llx\n",
		current->pid, ((u64 *)&alert)[0], ((u64 *)&alert)[1]);

	return sizeof(alert);
}

static ssize_t dlb2_write(struct file *f,
			  const char __user *buf,
			  size_t len,
			  loff_t *offset)
{
	struct dlb2_dev *dev;

	dev = container_of(f->f_inode->i_cdev, struct dlb2_dev, cdev);

	dev_dbg(dev->dlb2_device, "[%s()]\n", __func__);

	return 0;
}

/* The kernel driver inserts VMAs into the device's VMA list whenever an mmap
 * is performed or the VMA is cloned (e.g. during fork()).
 */
static int dlb2_insert_vma(struct dlb2_dev *dev, struct vm_area_struct *vma)
{
	struct dlb2_vma_node *node;

	node = devm_kzalloc(dev->dlb2_device, sizeof(*node), GFP_KERNEL);
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
static void dlb2_delete_vma(struct dlb2_dev *dev, struct vm_area_struct *vma)
{
	struct dlb2_vma_node *vma_node;
	struct list_head *node;

	mutex_lock(&dev->resource_mutex);

	list_for_each(node, &dev->vma_list) {
		vma_node = list_entry(node, struct dlb2_vma_node, list);
		if (vma_node->vma == vma) {
			list_del(&vma_node->list);
			devm_kfree(dev->dlb2_device, vma_node);
			break;
		}
	}

	mutex_unlock(&dev->resource_mutex);
}

static void
dlb2_vma_open(struct vm_area_struct *vma)
{
	struct dlb2_dev *dev = vma->vm_private_data;

	dlb2_insert_vma(dev, vma);
}

static void
dlb2_vma_close(struct vm_area_struct *vma)
{
	struct dlb2_dev *dev = vma->vm_private_data;

	dlb2_delete_vma(dev, vma);
}

static const struct vm_operations_struct dlb2_vma_ops = {
	.open = dlb2_vma_open,
	.close = dlb2_vma_close,
};

static int dlb2_mmap(struct file *f, struct vm_area_struct *vma)
{
	struct dlb2_dev *dev;
	u32 domain_id;
	int ret;

	dev = container_of(f->f_inode->i_cdev, struct dlb2_dev, cdev);

	/* mmap operations must go through scheduling domain device files */
	if (IS_DLB2_DEV_FILE(dlb2_dev_number_base, f->f_inode->i_rdev))
		return -EINVAL;

	ret = dlb2_check_and_inc_active_users(dev, f->private_data);
	if (ret)
		return ret;

	domain_id = DLB2_FILE_ID_FROM_DEV_T(dlb2_dev_number_base,
					    f->f_inode->i_rdev);

	if (domain_id >= DLB2_MAX_NUM_DOMAINS) {
		dev_err(dev->dlb2_device,
			"[%s()] Internal error\n", __func__);
		ret = -EINVAL;
		goto end;
	}

	ret = dlb2_insert_vma(dev, vma);
	if (ret)
		goto end;

	mutex_lock(&dev->resource_mutex);

	ret = dev->ops->mmap(f, vma, domain_id);

	mutex_unlock(&dev->resource_mutex);

	if (ret)
		dlb2_delete_vma(dev, vma);

	vma->vm_ops = &dlb2_vma_ops;
	vma->vm_private_data = dev;

end:
	dlb2_dec_active_users(dev);

	return ret;
}

#if KERNEL_VERSION(2, 6, 35) <= LINUX_VERSION_CODE
static long
dlb2_ioctl(struct file *f,
	   unsigned int cmd,
	   unsigned long arg)
#else
static int
dlb2_ioctl(struct inode *i,
	   struct file *f,
	   unsigned int cmd,
	   unsigned long arg)
#endif
{
	struct dlb2_dev *dev;
	u32 domain_id;
	int ret;

	dev = container_of(f->f_inode->i_cdev, struct dlb2_dev, cdev);

	domain_id = DLB2_FILE_ID_FROM_DEV_T(dlb2_dev_number_base,
					    f->f_inode->i_rdev);

	if (domain_id > DLB2_MAX_NUM_DOMAINS) {
		dev_err(dev->dlb2_device,
			"[%s()] Internal error\n", __func__);
		return -EINVAL;
	}

	if (_IOC_TYPE(cmd) != DLB2_IOC_MAGIC) {
		dev_err(dev->dlb2_device,
			"[%s()] Bad magic number!\n", __func__);
		return -EINVAL;
	}

	ret = dlb2_check_and_inc_active_users(dev, f->private_data);
	if (ret)
		return ret;

	if (IS_DLB2_DEV_FILE(dlb2_dev_number_base, f->f_inode->i_rdev))
		ret = dlb2_ioctl_dispatcher(dev, cmd, arg);
	else
		ret = dlb2_domain_ioctl_dispatcher(dev, f->private_data,
						   cmd, arg, domain_id);

	dlb2_dec_active_users(dev);

	return ret;
}

static const struct file_operations dlb2_fops = {
	.owner   = THIS_MODULE,
	.open    = dlb2_open,
	.release = dlb2_close,
	.read    = dlb2_read,
	.write   = dlb2_write,
	.mmap    = dlb2_mmap,
#if KERNEL_VERSION(2, 6, 35) <= LINUX_VERSION_CODE
	.unlocked_ioctl = dlb2_ioctl,
#else
	.ioctl   = dlb2_ioctl,
#endif
};

/**********************************/
/****** PCI driver callbacks ******/
/**********************************/

static void dlb2_assign_ops(struct dlb2_dev *dlb2_dev,
			    const struct pci_device_id *pdev_id)
{
	dlb2_dev->type = pdev_id->driver_data;

	switch (pdev_id->driver_data) {
	case DLB2_PF:
		dlb2_dev->ops = &dlb2_pf_ops;
		break;
	case DLB2_VF:
		dlb2_dev->ops = &dlb2_vf_ops;
		break;
	}
}

static bool dlb2_id_in_use[DLB2_MAX_NUM_DEVICES];

static int dlb2_find_next_available_id(void)
{
	int i;

	for (i = 0; i < DLB2_MAX_NUM_DEVICES; i++)
		if (!dlb2_id_in_use[i])
			return i;

	return -1;
}

static void dlb2_set_id_in_use(int id, bool in_use)
{
	dlb2_id_in_use[id] = in_use;
}

static int dlb2_probe(struct pci_dev *pdev,
		      const struct pci_device_id *pdev_id)
{
	struct dlb2_dev *dlb2_dev;
	int ret;

	dev_dbg(&pdev->dev, "probe\n");

	dlb2_dev = devm_kzalloc(&pdev->dev, sizeof(*dlb2_dev), GFP_KERNEL);
	if (!dlb2_dev) {
		ret = -ENOMEM;
		goto dlb2_dev_kzalloc_fail;
	}

	mutex_lock(&dlb2_driver_lock);
	list_add(&dlb2_dev->list, &dlb2_dev_list);
	mutex_unlock(&dlb2_driver_lock);

	dlb2_assign_ops(dlb2_dev, pdev_id);

	pci_set_drvdata(pdev, dlb2_dev);

	dlb2_dev->pdev = pdev;

	dlb2_dev->id = dlb2_find_next_available_id();
	if (dlb2_dev->id == -1) {
		dev_err(&pdev->dev, "probe: insufficient device IDs\n");

		ret = -EINVAL;
		goto next_available_id_fail;
	}

	ret = pci_enable_device(pdev);
	if (ret != 0) {
		dev_err(&pdev->dev, "pci_enable_device() returned %d\n", ret);

		goto pci_enable_device_fail;
	}

	ret = pci_request_regions(pdev, dlb2_driver_name);
	if (ret != 0) {
		dev_err(&pdev->dev,
			"pci_request_regions(): returned %d\n", ret);

		goto pci_request_regions_fail;
	}

	pci_set_master(pdev);

	pci_intx(pdev, 0);

	pci_enable_pcie_error_reporting(pdev);

	/* Don't call pci_disable_pasid() if it is already disabled to avoid
	 * the WARN_ON() print
	 */
	if (pdev->pasid_enabled)
		pci_disable_pasid(pdev);

	INIT_LIST_HEAD(&dlb2_dev->vma_list);

	ret = dlb2_dev->ops->map_pci_bar_space(dlb2_dev, pdev);
	if (ret)
		goto map_pci_bar_fail;

	/* (VF only) Register the driver with the PF driver */
	ret = dlb2_dev->ops->register_driver(dlb2_dev);
	if (ret)
		goto driver_registration_fail;

	/* If this is an auxiliary VF, it can skip the rest of the probe
	 * function. This VF is only used for its MSI interrupt vectors, and
	 * the VF's register_driver callback will initialize them.
	 */
	if (dlb2_dev->type == DLB2_VF &&
	    dlb2_dev->vf_id_state.is_auxiliary_vf)
		goto aux_vf_probe;

	ret = dlb2_dev->ops->cdev_add(dlb2_dev,
				      dlb2_dev_number_base,
				      &dlb2_fops);
	if (ret)
		goto cdev_add_fail;

	ret = dlb2_dev->ops->device_create(dlb2_dev, pdev, dlb2_class);
	if (ret)
		goto device_add_fail;

	dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));

	ret = dlb2_dev->ops->sysfs_create(dlb2_dev);
	if (ret)
		goto sysfs_create_fail;

	/* PM enable must be done before any other MMIO accesses, and this
	 * setting is persistent across device reset.
	 */
	dlb2_dev->ops->enable_pm(dlb2_dev);

	ret = dlb2_dev->ops->wait_for_device_ready(dlb2_dev, pdev);
	if (ret)
		goto wait_for_device_ready_fail;

	ret = dlb2_reset_device(pdev);
	if (ret)
		goto dlb2_reset_fail;

	ret = dlb2_dev->ops->init_interrupts(dlb2_dev, pdev);
	if (ret)
		goto init_interrupts_fail;

	ret = dlb2_dev->ops->init_driver_state(dlb2_dev);
	if (ret)
		goto init_driver_state_fail;

	ret = dlb2_resource_init(&dlb2_dev->hw);
	if (ret)
		goto resource_init_fail;

#ifdef CONFIG_INTEL_DLB2_DATAPATH
	dlb2_datapath_init(dlb2_dev, dlb2_dev->id);
#endif

	dlb2_dev->ops->init_hardware(dlb2_dev);

	/* The driver puts the device to sleep (D3hot) while there are no
	 * scheduling domains to service. The usage counter of a PCI device at
	 * probe time is 2, so decrement it twice here. (The PCI layer has
	 * already called pm_runtime_enable().)
	 */
	dlb2_dev->ops->dec_pm_refcnt(pdev);
	dlb2_dev->ops->dec_pm_refcnt(pdev);

	dlb2_set_id_in_use(dlb2_dev->id, true);

aux_vf_probe:
	return 0;

resource_init_fail:
	dlb2_dev->ops->free_driver_state(dlb2_dev);
init_driver_state_fail:
	dlb2_dev->ops->free_interrupts(dlb2_dev, pdev);
init_interrupts_fail:
dlb2_reset_fail:
wait_for_device_ready_fail:
	dlb2_dev->ops->sysfs_destroy(dlb2_dev);
sysfs_create_fail:
	dlb2_dev->ops->device_destroy(dlb2_dev, dlb2_class);
device_add_fail:
	dlb2_dev->ops->cdev_del(dlb2_dev);
cdev_add_fail:
	dlb2_dev->ops->unregister_driver(dlb2_dev);
driver_registration_fail:
	dlb2_dev->ops->unmap_pci_bar_space(dlb2_dev, pdev);
map_pci_bar_fail:
	pci_disable_pcie_error_reporting(pdev);
	pci_release_regions(pdev);
pci_request_regions_fail:
	pci_disable_device(pdev);
next_available_id_fail:
pci_enable_device_fail:
	mutex_lock(&dlb2_driver_lock);
	list_del(&dlb2_dev->list);
	mutex_unlock(&dlb2_driver_lock);

	devm_kfree(&pdev->dev, dlb2_dev);
dlb2_dev_kzalloc_fail:
	return ret;
}

static void dlb2_remove(struct pci_dev *pdev)
{
	struct dlb2_dev *dlb2_dev;
	int i;

	/* Undo all the dlb2_probe() operations */
	dev_dbg(&pdev->dev, "Cleaning up the DLB driver for removal\n");
	dlb2_dev = pci_get_drvdata(pdev);

	/* If this is an auxiliary VF, it skipped past most of the probe code */
	if (dlb2_dev->type == DLB2_VF &&
	    dlb2_dev->vf_id_state.is_auxiliary_vf)
		goto aux_vf_remove;

	/* Attempt to remove VFs before taking down the PF, since VFs cannot
	 * operate without a PF driver (in part because hardware doesn't
	 * support (CMD.MEM == 0 && IOV_CTRL.MSE == 1)).
	 */
	if (!pdev->is_virtfn && pci_num_vf(pdev) &&
	    dlb2_pci_sriov_configure(pdev, 0))
		dev_err(&pdev->dev,
			"Warning: DLB VFs will become unusable when the PF driver is removed\n");

	dlb2_set_id_in_use(dlb2_dev->id, false);

	/* Undo the PM operations in dlb2_probe(). */
	dlb2_dev->ops->inc_pm_refcnt(pdev, false);
	dlb2_dev->ops->inc_pm_refcnt(pdev, false);

	dlb2_dev->ops->free_driver_state(dlb2_dev);

	dlb2_dev->ops->free_interrupts(dlb2_dev, pdev);

#ifdef CONFIG_INTEL_DLB2_DATAPATH
	dlb2_datapath_free(dlb2_dev->id);
#endif

	dlb2_resource_free(&dlb2_dev->hw);

	dlb2_release_device_memory(dlb2_dev);

	dlb2_dev->ops->sysfs_destroy(dlb2_dev);

	/* If a domain is created without its device file ever being opened, it
	 * needs to be destroyed here.
	 */
	for (i = 0; i < DLB2_MAX_NUM_DOMAINS; i++) {
		struct dlb2_status *status;

		status = dlb2_dev->sched_domains[i].status;

		if (!status)
			continue;

		if (status->refcnt == 0)
			device_destroy(dlb2_class,
				       MKDEV(MAJOR(dlb2_dev->dev_number),
					     MINOR(dlb2_dev->dev_number) +
						i));

		devm_kfree(dlb2_dev->dlb2_device, status);
	}

	dlb2_dev->ops->device_destroy(dlb2_dev, dlb2_class);

	dlb2_dev->ops->cdev_del(dlb2_dev);

aux_vf_remove:
	dlb2_dev->ops->unregister_driver(dlb2_dev);

	dlb2_dev->ops->unmap_pci_bar_space(dlb2_dev, pdev);

	pci_disable_pcie_error_reporting(pdev);

	pci_release_regions(pdev);

	pci_disable_device(pdev);

	mutex_lock(&dlb2_driver_lock);
	list_del(&dlb2_dev->list);
	mutex_unlock(&dlb2_driver_lock);

	devm_kfree(&pdev->dev, dlb2_dev);
}

#ifdef CONFIG_PM
static int dlb2_runtime_suspend(struct device *dev)
{
	struct pci_dev *pdev = container_of(dev, struct pci_dev, dev);
	struct dlb2_dev *dlb2_dev = pci_get_drvdata(pdev);

	dev_dbg(dlb2_dev->dlb2_device, "Suspending device operation\n");

	/* Return and let the PCI subsystem put the device in D3hot. */

	return 0;
}

static int dlb2_runtime_resume(struct device *dev)
{
	struct pci_dev *pdev = container_of(dev, struct pci_dev, dev);
	struct dlb2_dev *dlb2_dev = pci_get_drvdata(pdev);
	int ret;

	/* The PCI subsystem put the device in D0, but the device may not have
	 * completed powering up. Wait until the device is ready before
	 * proceeding.
	 */
	ret = dlb2_dev->ops->wait_for_device_ready(dlb2_dev, pdev);
	if (ret)
		return ret;

	dev_dbg(dlb2_dev->dlb2_device, "Resuming device operation\n");

	/* Now reinitialize the device state. */
	dlb2_reset_hardware_state(dlb2_dev, true);

	return 0;
}
#endif

static struct pci_device_id dlb2_id_table[] = {
	{ PCI_DEVICE_DATA(INTEL, DLB2_PF, DLB2_PF) },
	{ PCI_DEVICE_DATA(INTEL, DLB2_VF, DLB2_VF) },
	{ 0 }
};
MODULE_DEVICE_TABLE(pci, dlb2_id_table);

#ifdef CONFIG_INTEL_DLB2_DATAPATH
void dlb2_register_dp_handle(struct dlb2_dp *dp)
{
	struct dlb2_dev *dev = dp->dev;

	mutex_lock(&dp->dev->resource_mutex);

	list_add(&dp->next, &dev->dp.hdl_list);

#ifdef CONFIG_PM
	dev->ops->inc_pm_refcnt(dev->pdev, true);

	dp->pm_refcount = 1;
#endif

	mutex_unlock(&dev->resource_mutex);
}

static void dlb2_dec_dp_refcount(struct dlb2_dp *dp, struct dlb2_dev *dev)
{
#ifdef CONFIG_PM
	if (dp->pm_refcount) {
		dev->ops->dec_pm_refcnt(dev->pdev);

		dp->pm_refcount = 0;
	}
#endif
}

void dlb2_unregister_dp_handle(struct dlb2_dp *dp)
{
	struct dlb2_dev *dev = dp->dev;

	mutex_lock(&dp->dev->resource_mutex);

	list_del(&dp->next);

	dlb2_dec_dp_refcount(dp, dev);

	mutex_unlock(&dev->resource_mutex);
}

static void dlb2_disable_kernel_threads(struct dlb2_dev *dev)
{
	struct dlb2_dp *dp;
	int i;

	/* Kernel threads using DLB aren't killed, but are prevented from
	 * continuing to use their scheduling domain.
	 */
	for (i = 0; i < DLB2_MAX_NUM_DOMAINS; i++) {
		if (!dev->sched_domains[i].user_mode &&
		    dev->sched_domains[i].dp)
			dev->sched_domains[i].dp->shutdown = true;
	}

	/* When the kernel thread calls dlb2_close(), it will unregister its
	 * handle and decrement the PM refcount. If even one of these kernel
	 * threads don't follow the correct shutdown procedure, though,
	 * the device's PM reference counting will be incorrect. So, we
	 * proactively decrement every datapath handle's refcount here.
	 */
	list_for_each_entry(dp, &dev->dp.hdl_list, next)
		dlb2_dec_dp_refcount(dp, dev);
}
#endif

/* Returns the number of host-owned virtual devices in use. */
int dlb2_host_vdevs_in_use(struct dlb2_dev *pf_dev)
{
	struct dlb2_dev *dev;
	int num = 0;

	mutex_lock(&dlb2_driver_lock);

	list_for_each_entry(dev, &dlb2_dev_list, list) {
		if (dev->type == DLB2_VF && dlb2_in_use(dev))
			num++;
	}

	mutex_unlock(&dlb2_driver_lock);

	return num;
}

static int dlb2_vdevs_in_use(struct dlb2_dev *dev)
{
	int i;

	/* For each VF with 1+ domains configured, query whether it is still in
	 * use, where "in use" is determined by the VF calling dlb2_in_use().
	 */

	for (i = 0; i < DLB2_MAX_NUM_VDEVS; i++) {
		struct dlb2_get_num_resources_args used_rsrcs;

		dlb2_hw_get_num_used_resources(&dev->hw, &used_rsrcs, true, i);

		if (!used_rsrcs.num_sched_domains)
			continue;

		if (dlb2_vdev_in_use(&dev->hw, i))
			return true;
	}

	return false;
}

bool dlb2_in_use(struct dlb2_dev *dev)
{
	if (dev->type == DLB2_PF && dlb2_vdevs_in_use(dev))
		return true;

#ifdef CONFIG_INTEL_DLB2_DATAPATH
	return (dlb2_total_device_file_refcnt(dev) != 0 ||
		!list_empty(&dev->dp.hdl_list) ||
		(dev->active_users > 0) ||
		!list_empty(&dev->vma_list));
#else
	return (dlb2_total_device_file_refcnt(dev) != 0 ||
		(dev->active_users > 0) ||
		!list_empty(&dev->vma_list));
#endif
}

static void dlb2_wait_for_idle(struct dlb2_dev *dlb2_dev)
{
	int i;

	for (i = 0; i < dlb2_reset_timeout_s * 10; i++) {
		bool idle;

		mutex_lock(&dlb2_dev->resource_mutex);

		/* Check for any application threads in the driver, extant
		 * mmaps, or open device files.
		 */
		idle = !dlb2_in_use(dlb2_dev);

		mutex_unlock(&dlb2_dev->resource_mutex);

		if (idle)
			return;

		cond_resched();
		msleep(100);
	}

	dev_err(dlb2_dev->dlb2_device,
		"PF driver timed out waiting for applications to idle\n");
}

void dlb2_zap_vma_entries(struct dlb2_dev *dlb2_dev)
{
	struct dlb2_vma_node *vma_node;
	struct list_head *node;

	dev_dbg(dlb2_dev->dlb2_device, "Zapping memory mappings\n");

	list_for_each(node, &dlb2_dev->vma_list) {
		unsigned long size;

		vma_node = list_entry(node, struct dlb2_vma_node, list);

		size = vma_node->vma->vm_end - vma_node->vma->vm_start;

		zap_vma_ptes(vma_node->vma, vma_node->vma->vm_start, size);
	}
}

static unsigned int dlb2_get_vf_dev_id_from_pf(struct pci_dev *pdev)
{
	switch (pdev->device) {
	case PCI_DEVICE_ID_INTEL_DLB2_PF:
		return PCI_DEVICE_ID_INTEL_DLB2_VF;
	default:
		return -1;
	}
}

static void dlb2_save_pfs_vf_config_state(struct pci_dev *pdev)
{
	struct pci_dev *vfdev = NULL;
	unsigned int device_id;

	device_id = dlb2_get_vf_dev_id_from_pf(pdev);
	if (device_id == -1)
		return;

	/* Find all this PF's VFs */
	vfdev = pci_get_device(pdev->vendor, device_id, NULL);

	while (vfdev) {
		if (vfdev->is_virtfn && vfdev->physfn == pdev)
			pci_save_state(vfdev);

		vfdev = pci_get_device(pdev->vendor, device_id, vfdev);
	}
}

static void dlb2_restore_pfs_vf_config_state(struct pci_dev *pdev)
{
	unsigned int device_id;
	struct pci_dev *vfdev;

	device_id = dlb2_get_vf_dev_id_from_pf(pdev);
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

static void dlb2_disable_device_files(struct dlb2_dev *dlb2_dev)
{
	int i;

	/* Set all status->valid flags to false to prevent existing device
	 * files from being used to enter the device driver.
	 */
	for (i = 0; i < DLB2_MAX_NUM_DOMAINS; i++) {
		struct dlb2_status *status;

		status = dlb2_dev->sched_domains[i].status;

		if (!status)
			continue;

		status->valid = false;

		device_destroy(dlb2_class,
			       MKDEV(MAJOR(dlb2_dev->dev_number),
				     MINOR(dlb2_dev->dev_number) + i));

		/* If the domain device file was created but never opened, free
		 * the file private memory here. Otherwise, it will be freed
		 * in the file close callback when refcnt reaches 0.
		 */
		if (status->refcnt == 0)
			devm_kfree(dlb2_dev->dlb2_device, status);
	}

	if (dlb2_dev->status)
		dlb2_dev->status->valid = false;
}

static void dlb2_wake_threads(struct dlb2_dev *dlb2_dev)
{
	int i;

	/* Wake any blocked device file readers. These threads will return the
	 * DLB2_DOMAIN_ALERT_DEVICE_RESET alert, and well-behaved applications
	 * will close their fds and unmap DLB memory as a result.
	 */
	for (i = 0; i < DLB2_MAX_NUM_DOMAINS; i++)
		wake_up_interruptible(&dlb2_dev->sched_domains[i].wq_head);

	/* Wake threads blocked on a CQ interrupt */
	for (i = 0; i < DLB2_MAX_NUM_LDB_PORTS; i++)
		dlb2_wake_thread(dlb2_dev,
				 &dlb2_dev->intr.ldb_cq_intr[i],
				 WAKE_DEV_RESET);

	for (i = 0; i < DLB2_MAX_NUM_DIR_PORTS; i++)
		dlb2_wake_thread(dlb2_dev,
				 &dlb2_dev->intr.dir_cq_intr[i],
				 WAKE_DEV_RESET);
}

void dlb2_stop_users(struct dlb2_dev *dlb2_dev)
{
#ifdef CONFIG_INTEL_DLB2_DATAPATH
	/* Kernel datapath users are not force killed. Instead their domain's
	 * shutdown flag is set, which prevents them from continuing to use
	 * their scheduling domain. These kernel threads must clean up their
	 * current handles and create a new domain in order to keep using the
	 * DLB.
	 */
	dlb2_disable_kernel_threads(dlb2_dev);
#endif

	/* Disable existing device files to prevent applications from enter the
	 * device driver through file operations. (New files can't be opened
	 * while the resource mutex is held.)
	 */
	dlb2_disable_device_files(dlb2_dev);

	/* Wake any threads blocked in the kernel */
	dlb2_wake_threads(dlb2_dev);
}

static void dlb2_reset_prepare(struct pci_dev *pdev)
{
	struct dlb2_dev *dlb2_dev;

	dlb2_dev = pci_get_drvdata(pdev);

	dev_dbg(dlb2_dev->dlb2_device, "DLB driver reset prepare\n");

	mutex_lock(&dlb2_dev->resource_mutex);

	/* Block any new device files from being opened */
	dlb2_dev->reset_active = true;

	/* If the device has 1+ VFs, even if they're not in use, it will not be
	 * suspended. To avoid having to handle two cases (reset while device
	 * suspended and reset while device active), increment the device's PM
	 * refcnt here, to guarantee that the device is in D0 for the duration
	 * of the reset.
	 */
	dlb2_dev->ops->inc_pm_refcnt(dlb2_dev->pdev, true);

	/* Notify all registered VF drivers so they stop their applications
	 * from attempting to use the VF while the PF FLR is in progress.
	 */
	if (dlb2_dev->type == DLB2_PF) {
		enum dlb2_mbox_vf_notification_type notif;
		int i;

		notif = DLB2_MBOX_VF_NOTIFICATION_PRE_RESET;

		for (i = 0; i < DLB2_MAX_NUM_VDEVS; i++) {
			if (dlb2_is_registered_vf(dlb2_dev, i))
				dlb2_notify_vf(&dlb2_dev->hw, i, notif);
		}
	}

	/* PF FLR resets the VF config space as well, so save it here. We need
	 * to restore their config spaces after the FLR in order to send the
	 * post-reset mailbox message (since a reset VF config space loses its
	 * MSI configuration).
	 */
	if (dlb2_dev->type == DLB2_PF)
		dlb2_save_pfs_vf_config_state(pdev);

	/* Stop existing applications from continuing to use the device by
	 * blocking kernel driver interfaces and waking any threads on wait
	 * queues, but don't zap VMA entries yet. If this is a PF FLR, notify
	 * any VFs of the impending FLR so they can stop their users as well.
	 */
	dlb2_stop_users(dlb2_dev);

	/* If no software is using the device, there's nothing to clean up. */
	if (!dlb2_in_use(dlb2_dev))
		return;

	dev_dbg(dlb2_dev->dlb2_device, "Waiting for users to stop\n");

	/* Release the resource mutex so threads can complete their work and
	 * exit the driver
	 */
	mutex_unlock(&dlb2_dev->resource_mutex);

	/* Wait until the device is idle or dlb2_reset_timeout_s seconds
	 * elapse. If the timeout occurs, zap any remaining VMA entries to
	 * guarantee applications can't reach the device.
	 */
	dlb2_wait_for_idle(dlb2_dev);

	mutex_lock(&dlb2_dev->resource_mutex);

	if (!dlb2_in_use(dlb2_dev))
		return;

	dlb2_zap_vma_entries(dlb2_dev);

	if (dlb2_dev->active_users > 0)
		dev_err(dlb2_dev->dlb2_device,
			"Internal error: %lu active_users in the driver during FLR\n",
			dlb2_dev->active_users);

	/* Don't release resource_mutex until after the FLR occurs. This
	 * prevents applications from accessing the device during reset.
	 */
}

static void dlb2_reset_done(struct pci_dev *pdev)
{
	struct dlb2_dev *dlb2_dev;
	int i;

	dlb2_dev = pci_get_drvdata(pdev);

	/* Clear all status pointers, to be filled in by post-FLR applications
	 * using the device driver.
	 *
	 * Note that status memory isn't leaked -- it is either freed during
	 * dlb2_stop_users() or in the file close callback.
	 */
	for (i = 0; i < DLB2_MAX_NUM_DOMAINS; i++)
		dlb2_dev->sched_domains[i].status = NULL;

	dlb2_dev->status = NULL;

	/* Free allocated CQ and PC memory. These are no longer accessible to
	 * user-space: either the applications closed, or their mappings were
	 * zapped in dlb2_reset_prepare().
	 */
	dlb2_release_device_memory(dlb2_dev);

	/* Reset resource allocation state */
	dlb2_resource_reset(&dlb2_dev->hw);

	/* Reset the hardware state, but don't issue an additional FLR */
	dlb2_reset_hardware_state(dlb2_dev, false);

	/* VF reset is a software procedure that can take > 100ms (on
	 * emulation). The PCIe spec mandates that a VF FLR will not take more
	 * than 100ms, so Linux simply sleeps for that long. If this function
	 * releases the resource mutex and allows another mailbox request to
	 * occur while the VF is still being reset, undefined behavior can
	 * result. Hence, this function waits until the PF indicates that the
	 * VF reset is done.
	 */
	if (dlb2_dev->type == DLB2_VF) {
		int retry_cnt;

		/* Timeout after DLB2_VF_FLR_DONE_POLL_TIMEOUT_MS of
		 * inactivity, sleep-polling every
		 * DLB2_VF_FLR_DONE_SLEEP_PERIOD_MS.
		 */
		retry_cnt = 0;
		while (!dlb2_vf_flr_complete(&dlb2_dev->hw)) {
			unsigned long sleep_us;

			sleep_us = DLB2_VF_FLR_DONE_SLEEP_PERIOD_MS * 1000;

			usleep_range(sleep_us, sleep_us + 1);

			if (++retry_cnt >= DLB2_VF_FLR_DONE_POLL_TIMEOUT_MS) {
				dev_err(dlb2_dev->dlb2_device,
					"VF driver timed out waiting for FLR response\n");
				break;
			}
		}
	}

	/* Restore the PF's VF config spaces in order to send the post-reset
	 * mailbox message.
	 */
	if (dlb2_dev->type == DLB2_PF)
		dlb2_restore_pfs_vf_config_state(pdev);

	/* Notify all registered VF drivers */
	if (dlb2_dev->type == DLB2_PF) {
		enum dlb2_mbox_vf_notification_type notif;
		int i;

		notif = DLB2_MBOX_VF_NOTIFICATION_POST_RESET;

		for (i = 0; i < DLB2_MAX_NUM_VDEVS; i++) {
			if (dlb2_is_registered_vf(dlb2_dev, i))
				dlb2_notify_vf(&dlb2_dev->hw, i, notif);
		}
	}

	dev_dbg(dlb2_dev->dlb2_device, "DLB driver reset done\n");

	dlb2_dev->reset_active = false;

	/* Undo the PM refcnt increment in dlb2_reset_prepare(). */
	dlb2_dev->ops->dec_pm_refcnt(dlb2_dev->pdev);

	mutex_unlock(&dlb2_dev->resource_mutex);
}

#if KERNEL_VERSION(4, 13, 0) >= LINUX_VERSION_CODE
static void dlb2_reset_notify(struct pci_dev *pdev, bool prepare)
{
	/* Unexpected FLR. Applications may be actively using the device at
	 * the same time, which poses two problems:
	 * - If applications continue to enqueue to the hardware they will
	 *   cause hardware errors, because the FLR will have reset the
	 *   scheduling domains, ports, and queues.
	 * - When the applications end, they must not trigger the driver's
	 *   domain reset code. The domain reset procedure would fail because
	 *   the device's registers will have been reset by the FLR.
	 *
	 * To avoid these problems, the driver handles unexpected resets as
	 * follows:
	 * 1. Set the reset_active flag. This flag blocks new device files
	 *    from being opened and is used as a wakeup condition in the
	 *    driver's wait queues.
	 * 2. If this is a PF FLR and there are active VFs, send them a
	 *    pre-reset notification, so they can stop any VF applications.
	 * 3. Disable all device files (set the per-file valid flag to false,
	 *    which prevents the file from being used after FLR completes) and
	 *    wake any threads on a wait queue.
	 * 4. If the DLB is not in use -- i.e. no open device files or memory
	 *    mappings, and no VFs in use (PF FLR only) -- the FLR can begin.
	 * 5. Else, the driver waits (up to a user-specified timeout, default
	 *    5s) for software to stop using the driver and the device. If the
	 *    timeout elapses, the driver zaps any remaining MMIO mappings.
	 *
	 * After the FLR:
	 * 1. Clear the per-file status pointers (the memory is freed in either
	 *    dlb2_close or dlb2_stop_users).
	 * 2. Release any remaining allocated port or CQ memory, now that it's
	 *    guaranteed the device is unconfigured and won't write to memory.
	 * 3. Reset software and hardware state
	 * 4. Notify VFs that the FLR is complete.
	 * 5. Set reset_active to false.
	 */

	if (prepare)
		dlb2_reset_prepare(pdev);
	else
		dlb2_reset_done(pdev);
}
#endif

static const struct pci_error_handlers dlb2_err_handler = {
#if KERNEL_VERSION(4, 13, 0) >= LINUX_VERSION_CODE
	.reset_notify  = dlb2_reset_notify,
#else
	.reset_prepare = dlb2_reset_prepare,
	.reset_done    = dlb2_reset_done,
#endif
};

#ifdef CONFIG_PM
static const struct dev_pm_ops dlb2_pm_ops = {
	SET_RUNTIME_PM_OPS(dlb2_runtime_suspend, dlb2_runtime_resume, NULL)
};
#endif

static struct pci_driver dlb2_pci_driver = {
	.name		 = (char *)dlb2_driver_name,
	.id_table	 = dlb2_id_table,
	.probe		 = dlb2_probe,
	.remove		 = dlb2_remove,
#ifdef CONFIG_PM
	.driver.pm	 = &dlb2_pm_ops,
#endif
	.sriov_configure = dlb2_pci_sriov_configure,
	.err_handler     = &dlb2_err_handler,
};

static int __init dlb2_init_module(void)
{
	int err;

	pr_info("%s - version %d.%d.%d\n", dlb2_driver_name,
		DLB2_VERSION_MAJOR_NUMBER,
		DLB2_VERSION_MINOR_NUMBER,
		DLB2_VERSION_REVISION_NUMBER);
	pr_info("%s\n", dlb2_driver_copyright);

	dlb2_class = class_create(THIS_MODULE, dlb2_driver_name);

	if (!dlb2_class) {
		pr_err("%s: class_create() returned %ld\n",
		       dlb2_driver_name, PTR_ERR(dlb2_class));

		return PTR_ERR(dlb2_class);
	}

	/* Allocate one minor number per domain */
	err = alloc_chrdev_region(&dlb2_dev_number_base,
				  0,
				  DLB2_MAX_NUM_DEVICE_FILES,
				  dlb2_driver_name);

	if (err < 0) {
		pr_err("%s: alloc_chrdev_region() returned %d\n",
		       dlb2_driver_name, err);

		return err;
	}

	err = pci_register_driver(&dlb2_pci_driver);
	if (err < 0) {
		pr_err("%s: pci_register_driver() returned %d\n",
		       dlb2_driver_name, err);
		return err;
	}

	return 0;
}

static void __exit dlb2_exit_module(void)
{
	pr_info("%s: exit\n", dlb2_driver_name);

	pci_unregister_driver(&dlb2_pci_driver);

	unregister_chrdev_region(dlb2_dev_number_base,
				 DLB2_MAX_NUM_DEVICE_FILES);

	if (dlb2_class) {
		class_destroy(dlb2_class);
		dlb2_class = NULL;
	}
}

module_init(dlb2_init_module);
module_exit(dlb2_exit_module);
