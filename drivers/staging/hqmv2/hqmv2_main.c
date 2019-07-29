// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2017-2018 Intel Corporation */

#include <linux/aer.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/list.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/pci.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/pm_runtime.h>
#include <linux/delay.h>
#include <linux/fdtable.h>
#if KERNEL_VERSION(4, 11, 0) <= LINUX_VERSION_CODE
#include <linux/sched/signal.h>
#endif

#include "base/hqmv2_resource.h"
#include "base/hqmv2_mbox.h"
#include "hqmv2_sriov.h"
#include "hqmv2_ioctl.h"
#include "hqmv2_intr.h"
#include "hqmv2_main.h"
#include "uapi/linux/hqmv2_user.h"

#ifdef CONFIG_INTEL_HQMV2_DATAPATH
#include "hqmv2_dp_priv.h"
#endif

const char hqmv2_driver_copyright[] = "Copyright(c) 2018 Intel Corporation";

#define TO_STR2(s) #s
#define TO_STR(s) TO_STR2(s)

#define DRV_VERSION \
	TO_STR(HQMV2_VERSION_MAJOR_NUMBER) "." \
	TO_STR(HQMV2_VERSION_MINOR_NUMBER) "." \
	TO_STR(HQMV2_VERSION_REVISION_NUMBER)

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Copyright(c) 2018 Intel Corporation");
MODULE_DESCRIPTION("Intel(R) Hardware Queue Manager v2 Driver");
MODULE_VERSION(DRV_VERSION);

unsigned int hqmv2_log_level = HQMV2_LOG_LEVEL_ERR;
module_param_named(log_level, hqmv2_log_level, uint, 0644);
MODULE_PARM_DESC(log_level, "Driver log level (0: None, 1: Error, 2: Info)");
unsigned int hqmv2_reset_timeout_s = HQMV2_DEFAULT_RESET_TIMEOUT_S;
module_param_named(reset_timeout_s, hqmv2_reset_timeout_s, uint, 0644);
MODULE_PARM_DESC(reset_timeout_s,
		 "Wait time (in seconds) after reset is requested given for app shutdown until driver forcibly terminates it");
bool hqmv2_pasid_override;
module_param_named(pasid_override, hqmv2_pasid_override, bool, 0);
MODULE_PARM_DESC(pasid_override, "Override allocated PASID with 0");

/* The driver lock protects driver data structures that may be used by multiple
 * devices.
 */
DEFINE_MUTEX(driver_lock);
struct list_head hqmv2_dev_list = LIST_HEAD_INIT(hqmv2_dev_list);

static struct class *ws_class;
static dev_t hqmv2_dev_number_base;

/*****************************/
/****** Devfs callbacks ******/
/*****************************/

static int hqmv2_open_domain_device_file(struct hqmv2_dev *dev, struct file *f)
{
	struct hqmv2_domain_dev *domain;
	u32 domain_id;

	domain_id = HQMV2_FILE_ID_FROM_DEV_T(hqmv2_dev_number_base,
					     f->f_inode->i_rdev);

	if (domain_id >= HQMV2_MAX_NUM_DOMAINS) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Internal error\n", __func__);
		return -EINVAL;
	}

	HQMV2_INFO(dev->hqmv2_device,
		   "Opening domain %d's device file\n", domain_id);

	domain = &dev->sched_domains[domain_id];

	/* Race condition: thread A opens a domain file just after
	 * thread B does the last close and resets the domain.
	 */
	if (domain->file_refcnt == -1)
		return -ENOENT;

	domain->file_refcnt++;

	return 0;
}

static int hqmv2_open(struct inode *i, struct file *f)
{
	struct hqmv2_dev *dev;
	int ret = 0;

	dev = container_of(f->f_inode->i_cdev, struct hqmv2_dev, cdev);

	mutex_lock(&dev->resource_mutex);

	/* See hqmv2_reset_notify() for more details */
	if (READ_ONCE(dev->unexpected_reset)) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] The HQM was unexpectedly reset. To recover, all HQM applications must close.\n",
			  __func__);
		mutex_unlock(&dev->resource_mutex);
		return -EINVAL;
	}

	if (!IS_HQMV2_DEV_FILE(hqmv2_dev_number_base, f->f_inode->i_rdev))
		ret = hqmv2_open_domain_device_file(dev, f);
	else
		HQMV2_INFO(dev->hqmv2_device, "Opening HQM device file\n");

	mutex_unlock(&dev->resource_mutex);

	dev->ops->inc_pm_refcnt(dev->pdev, true);

	return ret;
}

int hqmv2_add_domain_device_file(struct hqmv2_dev *hqmv2_dev, u32 domain_id)
{
	struct device *dev;

	HQMV2_INFO(hqmv2_dev->hqmv2_device,
		   "Creating domain %d's device file\n", domain_id);

	/* Create a new device in order to create a /dev/ domain node. This
	 * device is a child of the HQM PCI device.
	 */
	dev = device_create(ws_class,
			    hqmv2_dev->hqmv2_device->parent,
			    MKDEV(MAJOR(hqmv2_dev->dev_number),
				  MINOR(hqmv2_dev->dev_number) + domain_id),
			    hqmv2_dev,
			    "hqm%d/domain%d",
			    HQMV2_DEV_ID_FROM_DEV_T(hqmv2_dev_number_base,
						    hqmv2_dev->dev_number),
			    domain_id);

	if (IS_ERR_VALUE(PTR_ERR(dev))) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "%s: device_create() returned %ld\n",
			  hqmv2_driver_name, PTR_ERR(dev));

		return PTR_ERR(dev);
	}

	hqmv2_dev->sched_domains[domain_id].file_refcnt = 0;

	return 0;
}

static void hqmv2_reset_hardware_state(struct hqmv2_dev *dev, bool issue_flr)
{
	if (issue_flr)
		dev->ops->reset_device(dev->pdev);

	/* Reinitialize interrupt configuration */
	dev->ops->reinit_interrupts(dev);

	/* Reset configuration done through the sysfs */
	dev->ops->sysfs_reapply(dev);
}

static void hqmv2_release_domain_memory(struct hqmv2_dev *dev, u32 domain_id)
{
	struct hqmv2_port_memory *port_mem;
	struct hqmv2_domain_dev *domain;
	int i;

	domain = &dev->sched_domains[domain_id];

	HQMV2_INFO(dev->hqmv2_device,
		   "Releasing memory allocated for domain %d\n", domain_id);

	for (i = 0; i < HQMV2_MAX_NUM_LDB_PORTS; i++) {
		port_mem = &dev->ldb_port_pages[i];

		if (port_mem->domain_id == domain_id && port_mem->valid) {
			dma_free_coherent(&dev->pdev->dev,
					  PAGE_SIZE,
					  port_mem->cq_base,
					  port_mem->cq_dma_base);

			dma_free_coherent(&dev->pdev->dev,
					  PAGE_SIZE,
					  port_mem->pc_base,
					  port_mem->pc_dma_base);

			port_mem->valid = false;
		}
	}

	for (i = 0; i < HQMV2_MAX_NUM_DIR_PORTS; i++) {
		port_mem = &dev->dir_port_pages[i];

		if (port_mem->domain_id == domain_id && port_mem->valid) {
			dma_free_coherent(&dev->pdev->dev,
					  PAGE_SIZE,
					  port_mem->cq_base,
					  port_mem->cq_dma_base);

			dma_free_coherent(&dev->pdev->dev,
					  PAGE_SIZE,
					  port_mem->pc_base,
					  port_mem->pc_dma_base);

			port_mem->valid = false;
		}
	}
}

static void hqmv2_release_device_memory(struct hqmv2_dev *dev)
{
	int i;

	for (i = 0; i < HQMV2_MAX_NUM_DOMAINS; i++)
		hqmv2_release_domain_memory(dev, i);
}

static void hqmv2_reset_software_state(struct hqmv2_dev *dev)
{
	int i;

	/* This function resets the driver's software state only. The hardware
	 * state is reinitialized when the device is resumed in
	 * hqmv2_runtime_resume().
	 */

	/* Destroy any domain device files that were created before the reset
	 * but weren't opened.
	 */
	for (i = 0; i < HQMV2_MAX_NUM_DOMAINS; i++) {
		if (dev->sched_domains[i].file_refcnt != -1) {
			device_destroy(ws_class,
				       MKDEV(MAJOR(dev->dev_number),
					     MINOR(dev->dev_number) + i));

			dev->sched_domains[i].file_refcnt = -1;
		}
	}

	hqmv2_resource_reset(&dev->hw);

	for (i = 0; i < HQMV2_MAX_NUM_DOMAINS; i++) {
		struct hqmv2_domain_dev *domain = &dev->sched_domains[i];

		memset(&domain->alerts, 0, sizeof(domain->alerts));
		domain->alert_rd_idx = 0;
		domain->alert_wr_idx = 0;
	}

	hqmv2_release_device_memory(dev);

	WRITE_ONCE(dev->unexpected_reset, false);
}

static int hqmv2_total_domain_file_refcnt(struct hqmv2_dev *dev)
{
	int cnt = 0, i;

	/* (file_refcnt of -1 indicates an unused domain) */
	for (i = 0; i < HQMV2_MAX_NUM_DOMAINS; i++)
		if (dev->sched_domains[i].file_refcnt != -1)
			cnt += dev->sched_domains[i].file_refcnt;

	return cnt;
}

int hqmv2_close_domain_device_file(struct hqmv2_dev *dev,
				   u32 domain_id,
				   bool skip_reset)
{
	int ret = 0;

	device_destroy(ws_class,
		       MKDEV(MAJOR(dev->dev_number),
			     MINOR(dev->dev_number) +
			     domain_id));

	dev->sched_domains[domain_id].file_refcnt = -1;
#ifdef CONFIG_INTEL_HQMV2_DATAPATH
	dev->sched_domains[domain_id].dp = NULL;
#endif

	/* Domain reset will fail if the device was unexpectedly reset.
	 * See hqmv2_reset_notify() for more details.
	 */
	if (!READ_ONCE(dev->unexpected_reset) && !skip_reset)
		ret = dev->ops->reset_domain(&dev->hw, domain_id);

	dev->sched_domains[domain_id].alert_rd_idx = 0;
	dev->sched_domains[domain_id].alert_wr_idx = 0;

	/* If a PF FLR occurs and this VF driver has active users, the last
	 * user to close its device file is also responsible for clearing the
	 * unexpected_reset flag, which marks the end of the reset procedure
	 * for the VF.
	 */
	if (READ_ONCE(dev->unexpected_reset) && dev->vf_waiting_for_idle) {
		if (!hqmv2_in_use(dev)) {
			WRITE_ONCE(dev->unexpected_reset, false);
			dev->vf_waiting_for_idle = false;
		}
	}

	/* Unpin all memory pages associated with the domain */
	hqmv2_release_domain_memory(dev, domain_id);

	if (ret) {
		dev->domain_reset_failed = true;
		HQMV2_ERR(dev->hqmv2_device,
			  "Internal error: Domain reset failed. To recover, reset the device.\n");
	}

	return ret;
}

static int hqmv2_close(struct inode *i, struct file *f)
{
	struct hqmv2_dev *dev;
	int ret = 0;

	dev = container_of(f->f_inode->i_cdev, struct hqmv2_dev, cdev);

	if (!IS_HQMV2_DEV_FILE(hqmv2_dev_number_base, f->f_inode->i_rdev)) {
		struct hqmv2_domain_dev *domain;
		u32 domain_id;

		domain_id = HQMV2_FILE_ID_FROM_DEV_T(hqmv2_dev_number_base,
						     f->f_inode->i_rdev);

		if (domain_id >= HQMV2_MAX_NUM_DOMAINS) {
			HQMV2_ERR(dev->hqmv2_device,
				  "[%s()] Internal error\n", __func__);
			return -1;
		}

		domain = &dev->sched_domains[domain_id];

		HQMV2_INFO(dev->hqmv2_device,
			   "Closing domain %d's device file\n", domain_id);

		mutex_lock(&dev->resource_mutex);

		domain->file_refcnt--;

		if (domain->file_refcnt == 0)
			ret = hqmv2_close_domain_device_file(dev,
							     domain_id,
							     false);

		mutex_unlock(&dev->resource_mutex);
	} else {
		HQMV2_INFO(dev->hqmv2_device, "Closing HQM device file\n");
	}

	dev->ops->dec_pm_refcnt(dev->pdev);

	return ret;
}

int hqmv2_read_domain_alert(struct hqmv2_dev *dev,
			    int domain_id,
			    struct hqmv2_domain_alert *alert,
			    bool nonblock)
{
	struct hqmv2_domain_dev *domain = &dev->sched_domains[domain_id];

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

		HQMV2_INFO(dev->hqmv2_device,
			   "Thread %d is blocking waiting for an alert in domain %d\n",
			   current->pid, domain_id);

		if (wait_event_interruptible(domain->wq_head,
					     domain->alert_rd_idx !=
						domain->alert_wr_idx ||
					     READ_ONCE(dev->unexpected_reset)))
			return -ERESTARTSYS;

		/* See hqmv2_reset_notify() for more details */
		if (READ_ONCE(dev->unexpected_reset)) {
			alert->alert_id = HQMV2_DOMAIN_ALERT_DEVICE_RESET;
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

static ssize_t hqmv2_read(struct file *f,
			  char __user *buf,
			  size_t len,
			  loff_t *offset)
{
	struct hqmv2_domain_alert alert;
	struct hqmv2_dev *dev;
	u32 domain_id;
	int ret;

	if (IS_HQMV2_DEV_FILE(hqmv2_dev_number_base, f->f_inode->i_rdev))
		return 0;

	if (len != sizeof(alert))
		return -EINVAL;

	domain_id = HQMV2_FILE_ID_FROM_DEV_T(hqmv2_dev_number_base,
					     f->f_inode->i_rdev);

	dev = container_of(f->f_inode->i_cdev, struct hqmv2_dev, cdev);

	/* See hqmv2_reset_notify() for more details */
	if (READ_ONCE(dev->unexpected_reset)) {
		alert.alert_id = HQMV2_DOMAIN_ALERT_DEVICE_RESET;
		goto copy;
	}

	/* See hqmv2_user.h for details on domain alert notifications */

	ret = hqmv2_read_domain_alert(dev,
				      domain_id,
				      &alert,
				      f->f_flags & O_NONBLOCK);
	if (ret)
		return ret;

copy:
	if (copy_to_user(buf, &alert, sizeof(alert)))
		return -EFAULT;

	HQMV2_INFO(dev->hqmv2_device,
		   "Thread %d received alert 0x%llx, with aux data 0x%llx\n",
		   current->pid, ((u64 *)&alert)[0], ((u64 *)&alert)[1]);

	return sizeof(alert);
}

static ssize_t hqmv2_write(struct file *f,
			   const char __user *buf,
			   size_t len,
			   loff_t *offset)
{
	struct hqmv2_dev *dev;

	dev = container_of(f->f_inode->i_cdev, struct hqmv2_dev, cdev);

	HQMV2_INFO(dev->hqmv2_device, "[%s()]\n", __func__);

	return 0;
}

static int hqmv2_mmap(struct file *f, struct vm_area_struct *vma)
{
	struct hqmv2_dev *dev;
	u32 domain_id;

	dev = container_of(f->f_inode->i_cdev, struct hqmv2_dev, cdev);

	/* See hqmv2_reset_notify() for more details */
	if (READ_ONCE(dev->unexpected_reset)) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] The HQM was unexpectedly reset. To recover, all HQM applications must close.\n",
			  __func__);
		return -EINVAL;
	}

	/* Each HQM mapping occupies a single memory page */
	if ((vma->vm_end - vma->vm_start) != PAGE_SIZE)
		return -EINVAL;

	/* mmap operations must go through scheduling domain device files */
	if (IS_HQMV2_DEV_FILE(hqmv2_dev_number_base, f->f_inode->i_rdev))
		return -EINVAL;

	domain_id = HQMV2_FILE_ID_FROM_DEV_T(hqmv2_dev_number_base,
					     f->f_inode->i_rdev);

	if (domain_id >= HQMV2_MAX_NUM_DOMAINS) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Internal error\n", __func__);
		return -EINVAL;
	}

	return dev->ops->mmap(f, vma, domain_id);
}

#if KERNEL_VERSION(2, 6, 35) <= LINUX_VERSION_CODE
static long
hqmv2_ioctl(struct file *f,
	    unsigned int cmd,
	    unsigned long arg)
#else
static int
hqmv2_ioctl(struct inode *i,
	    struct file *f,
	    unsigned int cmd,
	    unsigned long arg)
#endif
{
	struct hqmv2_dev *dev;
	u32 domain_id;

	dev = container_of(f->f_inode->i_cdev, struct hqmv2_dev, cdev);

	domain_id = HQMV2_FILE_ID_FROM_DEV_T(hqmv2_dev_number_base,
					     f->f_inode->i_rdev);

	if (domain_id > HQMV2_MAX_NUM_DOMAINS) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Internal error\n", __func__);
		return -EINVAL;
	}

	if (IS_HQMV2_DEV_FILE(hqmv2_dev_number_base, f->f_inode->i_rdev))
		return hqmv2_ioctl_dispatcher(dev, cmd, arg);
	else
		return hqmv2_domain_ioctl_dispatcher(dev, cmd, arg, domain_id);
}

static const struct file_operations hqmv2_fops = {
	.owner   = THIS_MODULE,
	.open    = hqmv2_open,
	.release = hqmv2_close,
	.read    = hqmv2_read,
	.write   = hqmv2_write,
	.mmap    = hqmv2_mmap,
#if KERNEL_VERSION(2, 6, 35) <= LINUX_VERSION_CODE
	.unlocked_ioctl = hqmv2_ioctl,
#else
	.ioctl   = hqmv2_ioctl,
#endif
};

/**********************************/
/****** PCI driver callbacks ******/
/**********************************/

static void hqmv2_assign_ops(struct hqmv2_dev *hqmv2_dev,
			     const struct pci_device_id *pdev_id)
{
	hqmv2_dev->type = pdev_id->driver_data;

	switch (pdev_id->driver_data) {
	case HQMV2_PF:
		hqmv2_dev->ops = &hqmv2_pf_ops;
		break;
	case HQMV2_VF:
		hqmv2_dev->ops = &hqmv2_vf_ops;
		break;
	}
}

static bool hqmv2_id_in_use[HQMV2_MAX_NUM_DEVICES];

static int hqmv2_find_next_available_id(void)
{
	int i;

	for (i = 0; i < HQMV2_MAX_NUM_DEVICES; i++)
		if (!hqmv2_id_in_use[i])
			return i;

	return -1;
}

static void hqmv2_set_id_in_use(int id, bool in_use)
{
	hqmv2_id_in_use[id] = in_use;
}

static int hqmv2_probe(struct pci_dev *pdev,
		       const struct pci_device_id *pdev_id)
{
	struct hqmv2_dev *hqmv2_dev;
	int ret;

	HQMV2_INFO(&pdev->dev, "probe\n");

	hqmv2_dev = devm_kcalloc(&pdev->dev,
				 1,
				 sizeof(struct hqmv2_dev),
				 GFP_KERNEL);
	if (!hqmv2_dev) {
		ret = -ENOMEM;
		goto hqmv2_dev_kcalloc_fail;
	}

	mutex_lock(&driver_lock);
	list_add(&hqmv2_dev->list, &hqmv2_dev_list);
	mutex_unlock(&driver_lock);

	hqmv2_assign_ops(hqmv2_dev, pdev_id);

	pci_set_drvdata(pdev, hqmv2_dev);

	hqmv2_dev->pdev = pdev;

	hqmv2_dev->id = hqmv2_find_next_available_id();
	if (hqmv2_dev->id == -1) {
		HQMV2_ERR(&pdev->dev, "probe: insufficient device IDs\n");

		ret = -EINVAL;
		goto next_available_id_fail;
	}

	ret = pci_enable_device(pdev);
	if (ret != 0) {
		HQMV2_ERR(&pdev->dev, "pci_enable_device() returned %d\n", ret);

		goto pci_enable_device_fail;
	}

	ret = pci_request_regions(pdev, hqmv2_driver_name);
	if (ret != 0) {
		HQMV2_ERR(&pdev->dev,
			  "pci_request_regions(): returned %d\n", ret);

		goto pci_request_regions_fail;
	}

	pci_set_master(pdev);

	pci_intx(pdev, 0);

	pci_enable_pcie_error_reporting(pdev);

	ret = hqmv2_dev->ops->map_pci_bar_space(hqmv2_dev, pdev);
	if (ret)
		goto map_pci_bar_fail;

	/* (VF only) Register the driver with the PF driver */
	ret = hqmv2_dev->ops->register_driver(hqmv2_dev);
	if (ret)
		goto driver_registration_fail;

	/* If this is an auxiliary VF, it can skip the rest of the probe
	 * function. This VF is only used for its MSI interrupt vectors, and
	 * the VF's register_driver callback will initialize them.
	 */
	if (hqmv2_dev->type == HQMV2_VF &&
	    hqmv2_dev->vf_id_state.is_auxiliary_vf)
		goto aux_vf_probe;

	ret = hqmv2_dev->ops->cdev_add(hqmv2_dev,
				       hqmv2_dev_number_base,
				       &hqmv2_fops);
	if (ret)
		goto cdev_add_fail;

	ret = hqmv2_dev->ops->device_create(hqmv2_dev, pdev, ws_class);
	if (ret)
		goto device_add_fail;

	dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));

	ret = hqmv2_dev->ops->sysfs_create(hqmv2_dev);
	if (ret)
		goto sysfs_create_fail;

	/* PM enable must be done before any other MMIO accesses, and this
	 * setting is persistent across device reset.
	 */
	hqmv2_dev->ops->enable_pm(hqmv2_dev);

	ret = hqmv2_dev->ops->wait_for_device_ready(hqmv2_dev, pdev);
	if (ret)
		goto wait_for_device_ready_fail;

	ret = hqmv2_dev->ops->reset_device(pdev);
	if (ret)
		goto hqmv2_reset_fail;

	ret = hqmv2_dev->ops->init_interrupts(hqmv2_dev, pdev);
	if (ret)
		goto init_interrupts_fail;

	ret = hqmv2_dev->ops->init_driver_state(hqmv2_dev);
	if (ret)
		goto init_driver_state_fail;

	ret = hqmv2_resource_init(&hqmv2_dev->hw);
	if (ret)
		goto resource_init_fail;

#ifdef CONFIG_INTEL_HQMV2_DATAPATH
	hqmv2_datapath_init(hqmv2_dev, hqmv2_dev->id);
#endif

	/* The driver puts the device to sleep (D3hot) while there are no
	 * scheduling domains to service. The usage counter of a PCI device at
	 * probe time is 2, so decrement it twice here. (The PCI layer has
	 * already called pm_runtime_enable().)
	 */
	hqmv2_dev->ops->dec_pm_refcnt(pdev);
	hqmv2_dev->ops->dec_pm_refcnt(pdev);

	hqmv2_set_id_in_use(hqmv2_dev->id, true);

aux_vf_probe:
	return 0;

resource_init_fail:
	hqmv2_dev->ops->free_driver_state(hqmv2_dev);
init_driver_state_fail:
	hqmv2_dev->ops->free_interrupts(hqmv2_dev, pdev);
init_interrupts_fail:
hqmv2_reset_fail:
wait_for_device_ready_fail:
	hqmv2_dev->ops->sysfs_destroy(hqmv2_dev);
sysfs_create_fail:
	hqmv2_dev->ops->device_destroy(hqmv2_dev, ws_class);
device_add_fail:
	hqmv2_dev->ops->cdev_del(hqmv2_dev);
cdev_add_fail:
	hqmv2_dev->ops->unregister_driver(hqmv2_dev);
driver_registration_fail:
	hqmv2_dev->ops->unmap_pci_bar_space(hqmv2_dev, pdev);
map_pci_bar_fail:
	pci_disable_pcie_error_reporting(pdev);
	pci_release_regions(pdev);
pci_request_regions_fail:
	pci_disable_device(pdev);
next_available_id_fail:
pci_enable_device_fail:
	mutex_lock(&driver_lock);
	list_del(&hqmv2_dev->list);
	mutex_unlock(&driver_lock);

	devm_kfree(&pdev->dev, hqmv2_dev);
hqmv2_dev_kcalloc_fail:
	return ret;
}

static void hqmv2_remove(struct pci_dev *pdev)
{
	struct hqmv2_dev *hqmv2_dev;
	int i;

	/* Undo all the hqmv2_probe() operations */
	HQMV2_INFO(&pdev->dev, "Cleaning up the HQM driver for removal\n");
	hqmv2_dev = pci_get_drvdata(pdev);

	/* If this is an auxiliary VF, it skipped past most of the probe code */
	if (hqmv2_dev->type == HQMV2_VF &&
	    hqmv2_dev->vf_id_state.is_auxiliary_vf)
		goto aux_vf_remove;

	/* Attempt to remove VFs before taking down the PF, since VFs cannot
	 * operate without a PF driver (in part because hardware doesn't
	 * support (CMD.MEM == 0 && IOV_CTRL.MSE == 1)).
	 */
	if (!pdev->is_virtfn && pci_num_vf(pdev) &&
	    hqmv2_pci_sriov_configure(pdev, 0))
		HQMV2_ERR(&pdev->dev,
			  "Warning: HQM VFs will become unusable when the PF driver is removed\n");

	hqmv2_set_id_in_use(hqmv2_dev->id, false);

	/* Undo the PM operations in hqmv2_probe(). */
	hqmv2_dev->ops->inc_pm_refcnt(pdev, false);
	hqmv2_dev->ops->inc_pm_refcnt(pdev, false);

	hqmv2_dev->ops->free_driver_state(hqmv2_dev);

	hqmv2_dev->ops->free_interrupts(hqmv2_dev, pdev);

#ifdef CONFIG_INTEL_HQMV2_DATAPATH
	hqmv2_datapath_free(hqmv2_dev->id);
#endif

	hqmv2_resource_free(&hqmv2_dev->hw);

	hqmv2_release_device_memory(hqmv2_dev);

	hqmv2_dev->ops->sysfs_destroy(hqmv2_dev);

	/* If a domain is created without its device file ever being opened, it
	 * needs to be destroyed here.
	 */
	for (i = 0; i < HQMV2_MAX_NUM_DOMAINS; i++) {
		if (hqmv2_dev->sched_domains[i].file_refcnt == 0) {
			device_destroy(ws_class,
				       MKDEV(MAJOR(hqmv2_dev->dev_number),
					     MINOR(hqmv2_dev->dev_number) +
						i));
		}
	}

	/* Note: domain device files will be destroyed when they're closed */
	hqmv2_dev->ops->device_destroy(hqmv2_dev, ws_class);

	hqmv2_dev->ops->cdev_del(hqmv2_dev);

aux_vf_remove:
	hqmv2_dev->ops->unregister_driver(hqmv2_dev);

	hqmv2_dev->ops->unmap_pci_bar_space(hqmv2_dev, pdev);

	pci_disable_pcie_error_reporting(pdev);

	pci_release_regions(pdev);

	pci_disable_device(pdev);

	mutex_lock(&driver_lock);
	list_del(&hqmv2_dev->list);
	mutex_unlock(&driver_lock);

	devm_kfree(&pdev->dev, hqmv2_dev);
}

#ifdef CONFIG_PM
static int hqmv2_runtime_suspend(struct device *dev)
{
	struct pci_dev *pdev = container_of(dev, struct pci_dev, dev);
	struct hqmv2_dev *hqmv2_dev = pci_get_drvdata(pdev);

	HQMV2_INFO(hqmv2_dev->hqmv2_device, "Suspending device operation\n");

	/* If an unexpected reset occurred, recover device software state when
	 * all applications have stopped using the device. See
	 * hqmv2_reset_notify() for more details.
	 */
	if (READ_ONCE(hqmv2_dev->unexpected_reset))
		hqmv2_reset_software_state(hqmv2_dev);

	/* Return and let the PCI subsystem put the device in D3hot. */

	return 0;
}

static int hqmv2_runtime_resume(struct device *dev)
{
	struct pci_dev *pdev = container_of(dev, struct pci_dev, dev);
	struct hqmv2_dev *hqmv2_dev = pci_get_drvdata(pdev);
	int ret;

	/* The PCI subsystem put the device in D0, but the device may not have
	 * completed powering up. Wait until the device is ready before
	 * proceeding.
	 */
	ret = hqmv2_dev->ops->wait_for_device_ready(hqmv2_dev, pdev);
	if (ret)
		return ret;

	HQMV2_INFO(hqmv2_dev->hqmv2_device, "Resuming device operation\n");

	/* Now reinitialize the device state. */
	hqmv2_reset_hardware_state(hqmv2_dev, true);

	return 0;
}
#endif

static struct pci_device_id hqmv2_id_table[] = {
	{ PCI_VDEVICE(INTEL, HQMV2_SPR_PF_DEV_ID), .driver_data = HQMV2_PF },
	{ PCI_VDEVICE(INTEL, HQMV2_SPR_VF_DEV_ID), .driver_data = HQMV2_VF },
	{ 0 }
};
MODULE_DEVICE_TABLE(pci, hqmv2_id_table);

#ifdef CONFIG_INTEL_HQMV2_DATAPATH
void hqmv2_register_dp_handle(struct hqmv2_dp *dp)
{
	struct hqmv2_dev *dev = dp->dev;

	mutex_lock(&dp->dev->resource_mutex);

	list_add(&dp->next, &dev->dp.hdl_list);

#ifdef CONFIG_PM
	dev->ops->inc_pm_refcnt(dev->pdev, true);

	dp->pm_refcount = 1;
#endif

	mutex_unlock(&dev->resource_mutex);
}

static void hqmv2_dec_dp_refcount(struct hqmv2_dp *dp, struct hqmv2_dev *dev)
{
#ifdef CONFIG_PM
	if (dp->pm_refcount) {
		dev->ops->dec_pm_refcnt(dev->pdev);

		dp->pm_refcount = 0;
	}
#endif
}

void hqmv2_unregister_dp_handle(struct hqmv2_dp *dp)
{
	struct hqmv2_dev *dev = dp->dev;

	mutex_lock(&dp->dev->resource_mutex);

	list_del(&dp->next);

	hqmv2_dec_dp_refcount(dp, dev);

	mutex_unlock(&dev->resource_mutex);
}

static void hqmv2_disable_kernel_threads(struct hqmv2_dev *dev)
{
	struct hqmv2_dp *dp;
	int i;

	/* Kernel threads using HQM aren't killed, but are prevented from
	 * continuing to use their scheduling domain.
	 */
	for (i = 0; i < HQMV2_MAX_NUM_DOMAINS; i++) {
		if (!dev->sched_domains[i].user_mode &&
		    dev->sched_domains[i].dp)
			dev->sched_domains[i].dp->shutdown = true;
	}

	/* When the kernel thread calls hqmv2_close(), it will unregister its
	 * handle and decrement the PM refcount. If even one of these kernel
	 * threads don't follow the correct shutdown procedure, though,
	 * the device's PM reference counting will be incorrect. So, we
	 * proactively decrement every datapath handle's refcount here.
	 */
	list_for_each_entry(dp, &dev->dp.hdl_list, next)
		hqmv2_dec_dp_refcount(dp, dev);
}
#endif

/* Returns the number of host-owned virtual devices in use. */
int hqmv2_host_vdevs_in_use(struct hqmv2_dev *pf_dev)
{
	struct hqmv2_dev *dev;
	int num = 0;

	mutex_lock(&driver_lock);

	list_for_each_entry(dev, &hqmv2_dev_list, list) {
		if (dev->type == HQMV2_VF && hqmv2_in_use(dev))
			num++;
	}

	mutex_unlock(&driver_lock);

	return num;
}

static int hqmv2_vdevs_in_use(struct hqmv2_dev *dev)
{
	int i;

	/* For each VF with 1+ domains configured, query whether it is still in
	 * use, where "in use" is determined by the VF calling hqmv2_in_use().
	 */

	for (i = 0; i < HQMV2_MAX_NUM_VDEVS; i++) {
		struct hqmv2_get_num_resources_args used_rsrcs;

		hqmv2_hw_get_num_used_resources(&dev->hw, &used_rsrcs, true, i);

		if (!used_rsrcs.num_sched_domains)
			continue;

		if (hqmv2_vdev_in_use(&dev->hw, i))
			return true;
	}

	return false;
}

bool hqmv2_in_use(struct hqmv2_dev *dev)
{
	if (dev->type == HQMV2_PF && hqmv2_vdevs_in_use(dev))
		return true;

#ifdef CONFIG_INTEL_HQMV2_DATAPATH
	return (hqmv2_total_domain_file_refcnt(dev) != 0 ||
		!list_empty(&dev->dp.hdl_list));
#else
	return (hqmv2_total_domain_file_refcnt(dev) != 0);
#endif
}

static int is_hqmv2_domain_file(const void *unused,
				struct file *file,
				unsigned int fd)
{
	if (file->f_op->read == hqmv2_read &&
	    IS_HQMV2_DEV_FILE(hqmv2_dev_number_base, file->f_inode->i_rdev))
		return 1;

	return 0;
}

void hqmv2_shutdown_user_threads(struct hqmv2_dev *dev)
{
	struct task_struct *p;

	/* It's not sufficient to record struct task_struct pointers at
	 * hqmv2_open() time and shutdown those processes here, because child
	 * processes forked after the parent opens a device file will inherit
	 * the parent's file descriptor (and memory mappings).
	 *
	 * Rather than attempting to track which processes are potentially
	 * using the device, we inspect every process for an open domain
	 * device file, shutting down any such processes we find.
	 */

	for_each_process(p) {
		task_lock(p);

		if (p->flags & PF_KTHREAD) {
			task_unlock(p);
			continue;
		}

		if (iterate_fd(p->files, 0, is_hqmv2_domain_file, NULL)) {
			struct pid *pid = get_task_pid(p, PIDTYPE_PID);

			if (!pid)
				continue;

			HQMV2_INFO(dev->hqmv2_device,
				   "Killing process %d\n", pid_nr(pid));

			kill_pid(pid, SIGKILL, 1);
			put_pid(pid);
		}

		task_unlock(p);
	}
}

static void hqmv2_wait_for_idle(struct hqmv2_dev *hqmv2_dev)
{
	bool in_use;
	int i;

	for (i = 0; i < hqmv2_reset_timeout_s * 10; i++) {
		mutex_lock(&hqmv2_dev->resource_mutex);
		in_use = hqmv2_in_use(hqmv2_dev);
		mutex_unlock(&hqmv2_dev->resource_mutex);

		if (!in_use)
			break;

		cond_resched();
		msleep(100);
	}
}

static void hqmv2_stop_users(struct hqmv2_dev *hqmv2_dev)
{
	int i;

	HQMV2_INFO(hqmv2_dev->hqmv2_device, "Alerting users of reset\n");

	/* Wake any blocked device file readers. These threads will return the
	 * HQMV2_DOMAIN_ALERT_DEVICE_RESET alert, and well-behaved applications
	 * will close their fds and unmap HQM memory as a result.
	 */
	for (i = 0; i < HQMV2_MAX_NUM_DOMAINS; i++)
		wake_up_interruptible(&hqmv2_dev->sched_domains[i].wq_head);

	/* Wake threads blocked on a CQ interrupt */
	for (i = 0; i < HQMV2_MAX_NUM_LDB_PORTS; i++)
		hqmv2_wake_thread(hqmv2_dev,
				  &hqmv2_dev->intr.ldb_cq_intr[i],
				  WAKE_DEV_RESET);

	for (i = 0; i < HQMV2_MAX_NUM_DIR_PORTS; i++)
		hqmv2_wake_thread(hqmv2_dev,
				  &hqmv2_dev->intr.dir_cq_intr[i],
				  WAKE_DEV_RESET);

	/* Give applications hqmv2_reset_timeout_s seconds to clean up, then
	 * kill any stragglers. Only check domain device files, since they
	 * can't mmap any device memory through hqmv2 device files.
	 *
	 * Kernel datapath users are not force killed. Instead their domain's
	 * shutdown flag is set, which prevents them from continuing to use
	 * their scheduling domain. This has the same effect (from the driver's
	 * perspective) as issuing a SIGKILL, because it stops them from
	 * enqueueing any additional QEs. These kernel threads must clean up
	 * their current handles and create a new domain in order to keep using
	 * the HQM.
	 */
	HQMV2_INFO(hqmv2_dev->hqmv2_device, "Waiting for users to stop\n");

	/* Release the resource mutex so processes can close device files */
	mutex_unlock(&hqmv2_dev->resource_mutex);

	hqmv2_wait_for_idle(hqmv2_dev);

	mutex_lock(&hqmv2_dev->resource_mutex);

	if (!hqmv2_in_use(hqmv2_dev))
		return;

	HQMV2_INFO(hqmv2_dev->hqmv2_device, "Forcing users to stop\n");

	hqmv2_shutdown_user_threads(hqmv2_dev);

	mutex_unlock(&hqmv2_dev->resource_mutex);

	/* Give remaining users some more time to close. */
	hqmv2_wait_for_idle(hqmv2_dev);

	/* Re-acquire the lock before exiting, since the caller expects the
	 * lock to be held.
	 */
	mutex_lock(&hqmv2_dev->resource_mutex);
}

static unsigned int hqmv2_get_vf_dev_id_from_pf(struct pci_dev *pdev)
{
	switch (pdev->device) {
	case HQMV2_SPR_PF_DEV_ID:
		return HQMV2_SPR_VF_DEV_ID;
	default:
		return -1;
	}
}

static void hqmv2_save_pfs_vf_config_state(struct pci_dev *pdev)
{
	struct pci_dev *vfdev = NULL;
	unsigned int device_id;

	device_id = hqmv2_get_vf_dev_id_from_pf(pdev);
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

static void hqmv2_restore_pfs_vf_config_state(struct pci_dev *pdev)
{
	unsigned int device_id;
	struct pci_dev *vfdev;

	device_id = hqmv2_get_vf_dev_id_from_pf(pdev);
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

static void hqmv2_reset_prepare(struct pci_dev *pdev)
{
	struct hqmv2_dev *hqmv2_dev;

	hqmv2_dev = pci_get_drvdata(pdev);

	HQMV2_INFO(hqmv2_dev->hqmv2_device, "HQM driver reset prepare\n");

	/* If the device has 1+ VFs, even if they're not in use, it will not be
	 * suspended. To avoid having to handle two cases (reset while device
	 * suspended and reset while device active), increment the device's PM
	 * refcnt here, to guarantee that the device is in D0 for the duration
	 * of the reset.
	 */
	hqmv2_dev->ops->inc_pm_refcnt(hqmv2_dev->pdev, true);

	mutex_lock(&hqmv2_dev->resource_mutex);

#ifdef CONFIG_INTEL_HQMV2_DATAPATH
	hqmv2_disable_kernel_threads(hqmv2_dev);
#endif

	WRITE_ONCE(hqmv2_dev->unexpected_reset, true);

	/* Notify all registered VF drivers so they set their unexpected_reset
	 * flag, preventing applications from attempting to use the VF while
	 * the PF FLR is in progress.
	 */
	if (hqmv2_dev->type == HQMV2_PF) {
		enum hqmv2_mbox_vf_notification_type notif;
		int i;

		notif = HQMV2_MBOX_VF_NOTIFICATION_PRE_RESET;

		for (i = 0; i < HQMV2_MAX_NUM_VDEVS; i++) {
			if (hqmv2_is_registered_vf(hqmv2_dev, i))
				hqmv2_notify_vf(&hqmv2_dev->hw, i, notif);
		}
	}

	/* PF FLR resets the VF config space as well, so save it here. We need
	 * to restore their config spaces after the FLR in order to send the
	 * post-reset mailbox message (since a reset VF config space loses its
	 * MSI configuration).
	 */
	if (hqmv2_dev->type == HQMV2_PF)
		hqmv2_save_pfs_vf_config_state(pdev);

	/* If no software is using the device, there's nothing to clean up */
	if (!hqmv2_in_use(hqmv2_dev))
		return;

	hqmv2_stop_users(hqmv2_dev);

	/* Don't release resource_mutex until after the FLR occurs. This
	 * prevents applications from configuring the device in the interim,
	 * which would break the case where there are no scheduling domains at
	 * this time and unexpected_reset is not set to true.
	 */
}

static void hqmv2_reset_done(struct pci_dev *pdev)
{
	struct hqmv2_dev *hqmv2_dev;

	hqmv2_dev = pci_get_drvdata(pdev);

	/* hqmv2_reset_prepare() stopped any domains that were in use, so reset
	 * the driver's state now.
	 */
	hqmv2_reset_software_state(hqmv2_dev);

	/* Reset the hardware state, but don't issue an additional FLR */
	hqmv2_reset_hardware_state(hqmv2_dev, false);

	/* VF reset is a software procedure that can take > 100ms (on
	 * emulation). The PCIe spec mandates that a VF FLR will not take more
	 * than 100ms, so Linux simply sleeps for that long. If this function
	 * releases the resource mutex and allows another mailbox request to
	 * occur while the VF is still being reset, undefined behavior can
	 * result. Hence, this function waits until the PF indicates that the
	 * VF reset is done.
	 */
	if (hqmv2_dev->type == HQMV2_VF) {
		int retry_cnt;

		/* Timeout after HQMV2_VF_FLR_DONE_POLL_TIMEOUT_MS of
		 * inactivity, sleep-polling every
		 * HQMV2_VF_FLR_DONE_SLEEP_PERIOD_MS.
		 */
		retry_cnt = 0;
		while (!hqmv2_vf_flr_complete(&hqmv2_dev->hw)) {
			unsigned long sleep_us;

			sleep_us = HQMV2_VF_FLR_DONE_SLEEP_PERIOD_MS * 1000;

			usleep_range(sleep_us, sleep_us + 1);

			if (++retry_cnt >= HQMV2_VF_FLR_DONE_POLL_TIMEOUT_MS) {
				HQMV2_ERR(hqmv2_dev->hqmv2_device,
					  "VF driver timed out waiting for FLR response\n");
				break;
			}
		}
	}

	/* Restore the PF's VF config spaces in order to send the post-reset
	 * mailbox message.
	 */
	if (hqmv2_dev->type == HQMV2_PF)
		hqmv2_restore_pfs_vf_config_state(pdev);

	/* Notify all registered VF drivers */
	if (hqmv2_dev->type == HQMV2_PF) {
		enum hqmv2_mbox_vf_notification_type notif;
		int i;

		notif = HQMV2_MBOX_VF_NOTIFICATION_POST_RESET;

		for (i = 0; i < HQMV2_MAX_NUM_VDEVS; i++) {
			if (hqmv2_is_registered_vf(hqmv2_dev, i))
				hqmv2_notify_vf(&hqmv2_dev->hw, i, notif);
		}
	}

	HQMV2_INFO(hqmv2_dev->hqmv2_device, "HQM driver reset done\n");

	mutex_unlock(&hqmv2_dev->resource_mutex);

	/* Undo the PM refcnt increment in hqmv2_reset_prepare(). */
	hqmv2_dev->ops->dec_pm_refcnt(hqmv2_dev->pdev);
}

#if KERNEL_VERSION(4, 13, 0) >= LINUX_VERSION_CODE
static void hqmv2_reset_notify(struct pci_dev *pdev, bool prepare)
{
	/* Unexpected FLR. Applications may be actively using the device in
	 * parallel, which poses two problems:
	 * - Applications can continue to enqueue to the hardware, since
	 *   producer port pages will remain mapped in their address space.
	 *   Post-FLR enqueues will fail, and cause hardware alarms, because
	 *   the FLR deconfigured the scheduling domains, ports, and queues.
	 * - When the applications end, they'll trigger the driver's domain
	 *   reset code. The domain reset procedure will fail because the
	 *   device's registers will have been reset by the FLR.
	 *
	 * To avoid these problems, the driver handles unexpected resets as
	 * follows:
	 * 1. Prior to the FLR, the driver sets the unexpected_reset flag to
	 *    prevent further configuration of the device. This flag also
	 *    prevents the domain reset code from running.
	 * 2. The driver wakes any threads blocked in hqmv2_read() or blocked on
	 *    a CQ interrupt.
	 * 3. When the last HQM application has stopped using the device (i.e.,
	 *    when the last device file is closed), the prepare function ends
	 *    and the kernel performs the FLR. Once complete, the driver's
	 *    FLR-done (hqmv2_reset_done()) callback reinitializes software and
	 *    hardware state.
	 */

	if (prepare)
		hqmv2_reset_prepare(pdev);
	else
		hqmv2_reset_done(pdev);
}
#endif

static const struct pci_error_handlers hqmv2_err_handler = {
#if KERNEL_VERSION(4, 13, 0) >= LINUX_VERSION_CODE
	.reset_notify  = hqmv2_reset_notify,
#else
	.reset_prepare = hqmv2_reset_prepare,
	.reset_done    = hqmv2_reset_done,
#endif
};

#ifdef CONFIG_PM
static const struct dev_pm_ops hqmv2_pm_ops = {
	SET_RUNTIME_PM_OPS(hqmv2_runtime_suspend, hqmv2_runtime_resume, NULL)
};
#endif

static struct pci_driver hqmv2_pci_driver = {
	.name		 = (char *)hqmv2_driver_name,
	.id_table	 = hqmv2_id_table,
	.probe		 = hqmv2_probe,
	.remove		 = hqmv2_remove,
#ifdef CONFIG_PM
	.driver.pm	 = &hqmv2_pm_ops,
#endif
	.sriov_configure = hqmv2_pci_sriov_configure,
	.err_handler     = &hqmv2_err_handler,
};

static int __init hqmv2_init_module(void)
{
	int err;

	pr_info("%s - version %d.%d.%d\n", hqmv2_driver_name,
		HQMV2_VERSION_MAJOR_NUMBER,
		HQMV2_VERSION_MINOR_NUMBER,
		HQMV2_VERSION_REVISION_NUMBER);
	pr_info("%s\n", hqmv2_driver_copyright);

	ws_class = class_create(THIS_MODULE, hqmv2_driver_name);

	if (!ws_class) {
		pr_err("%s: class_create() returned %ld\n",
		       hqmv2_driver_name, PTR_ERR(ws_class));

		return PTR_ERR(ws_class);
	}

	/* Allocate one minor number per domain */
	err = alloc_chrdev_region(&hqmv2_dev_number_base,
				  0,
				  HQMV2_MAX_NUM_DEVICE_FILES,
				  hqmv2_driver_name);

	if (err < 0) {
		pr_err("%s: alloc_chrdev_region() returned %d\n",
		       hqmv2_driver_name, err);

		return err;
	}

	err = pci_register_driver(&hqmv2_pci_driver);
	if (err < 0) {
		pr_err("%s: pci_register_driver() returned %d\n",
		       hqmv2_driver_name, err);
		return err;
	}

	return 0;
}

static void __exit hqmv2_exit_module(void)
{
	pr_info("%s: exit\n", hqmv2_driver_name);

	pci_unregister_driver(&hqmv2_pci_driver);

	unregister_chrdev_region(hqmv2_dev_number_base,
				 HQMV2_MAX_NUM_DEVICE_FILES);

	if (ws_class) {
		class_destroy(ws_class);
		ws_class = NULL;
	}
}

module_init(hqmv2_init_module);
module_exit(hqmv2_exit_module);
