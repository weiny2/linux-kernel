// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2017-2020 Intel Corporation */

#include <linux/iommu.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/uaccess.h>

#include <uapi/linux/dlb2_user.h>

#include "base/dlb2_resource.h"
#include "dlb2_intr.h"
#include "dlb2_ioctl.h"
#include "dlb2_main.h"

#ifdef CONFIG_INTEL_DLB2_DATAPATH
#include "dlb2_dp_priv.h"
#endif

typedef int (*dlb2_domain_ioctl_callback_fn_t)(struct dlb2_dev *dev,
					       struct dlb2_status *status,
					       unsigned long arg,
					       u32 domain_id);

/* The DLB domain ioctl callback template minimizes replication of boilerplate
 * code to copy arguments, acquire and release the resource lock, and execute
 * the command.  The arguments and response structure name should have the
 * format dlb2_<lower_name>_args.
 */
#define DLB2_DOMAIN_IOCTL_CALLBACK_TEMPLATE(lower_name)			\
int dlb2_domain_ioctl_##lower_name(struct dlb2_dev *dev,		\
				   struct dlb2_status *status,		\
				   unsigned long user_arg,		\
				   u32 domain_id)			\
{									\
	struct dlb2_##lower_name##_args arg;				\
	struct dlb2_cmd_response response = {0};			\
	int ret;							\
	response.status = 0;						\
									\
	dev_dbg(dev->dlb2_device, "Entering %s()\n", __func__);		\
									\
	if (copy_from_user(&arg,					\
			   (void  __user *)user_arg,			\
			   sizeof(struct dlb2_##lower_name##_args))) {	\
		dev_err(dev->dlb2_device,				\
			"[%s()] Invalid ioctl argument pointer\n",	\
			__func__);					\
		return -EFAULT;						\
	}								\
									\
	mutex_lock(&dev->resource_mutex);				\
									\
	ret = dev->ops->lower_name(&dev->hw,				\
				  domain_id,				\
				  &arg,					\
				  &response);				\
									\
	mutex_unlock(&dev->resource_mutex);				\
									\
	if (copy_to_user((void __user *)arg.response,			\
			 &response,					\
			 sizeof(struct dlb2_cmd_response))) {		\
		dev_err(dev->dlb2_device,				\
			"[%s()] Invalid ioctl response pointer\n",	\
			__func__);					\
		return -EFAULT;						\
	}								\
									\
	dev_dbg(dev->dlb2_device, "Exiting %s()\n", __func__);		\
									\
	return ret;							\
}

DLB2_DOMAIN_IOCTL_CALLBACK_TEMPLATE(create_ldb_queue)
DLB2_DOMAIN_IOCTL_CALLBACK_TEMPLATE(create_dir_queue)
DLB2_DOMAIN_IOCTL_CALLBACK_TEMPLATE(start_domain)
DLB2_DOMAIN_IOCTL_CALLBACK_TEMPLATE(map_qid)
DLB2_DOMAIN_IOCTL_CALLBACK_TEMPLATE(unmap_qid)
DLB2_DOMAIN_IOCTL_CALLBACK_TEMPLATE(get_ldb_queue_depth)
DLB2_DOMAIN_IOCTL_CALLBACK_TEMPLATE(get_dir_queue_depth)
DLB2_DOMAIN_IOCTL_CALLBACK_TEMPLATE(pending_port_unmaps)

/* Port enable/disable ioctls don't use the callback template macro because
 * they have additional CQ interrupt management logic.
 */
int dlb2_domain_ioctl_enable_ldb_port(struct dlb2_dev *dev,
				      struct dlb2_status *status,
				      unsigned long user_arg,
				      u32 domain_id)
{
	struct dlb2_enable_ldb_port_args arg;
	struct dlb2_cmd_response response = {0};
	int ret;

	response.status = 0;

	dev_dbg(dev->dlb2_device, "Entering %s()\n", __func__);

	if (copy_from_user(&arg,
			   (void  __user *)user_arg,
			   sizeof(struct dlb2_enable_ldb_port_args))) {
		dev_err(dev->dlb2_device,
			"[%s()] Invalid ioctl argument pointer\n",
			__func__);
		return -EFAULT;
	}

	mutex_lock(&dev->resource_mutex);

	ret = dev->ops->enable_ldb_port(&dev->hw, domain_id, &arg, &response);

	/* Allow threads to block on this port's CQ interrupt */
	if (!ret)
		WRITE_ONCE(dev->intr.ldb_cq_intr[arg.port_id].disabled, false);

	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(struct dlb2_cmd_response))) {
		dev_err(dev->dlb2_device,
			"[%s()] Invalid ioctl response pointer\n",
			__func__);
		return -EFAULT;
	}

	dev_dbg(dev->dlb2_device, "Exiting %s()\n", __func__);

	return ret;
}

int dlb2_domain_ioctl_enable_dir_port(struct dlb2_dev *dev,
				      struct dlb2_status *status,
				      unsigned long user_arg,
				      u32 domain_id)
{
	struct dlb2_enable_dir_port_args arg;
	struct dlb2_cmd_response response = {0};
	int ret;

	response.status = 0;

	dev_dbg(dev->dlb2_device, "Entering %s()\n", __func__);

	if (copy_from_user(&arg,
			   (void  __user *)user_arg,
			   sizeof(struct dlb2_enable_dir_port_args))) {
		dev_err(dev->dlb2_device,
			"[%s()] Invalid ioctl argument pointer\n",
			__func__);
		return -EFAULT;
	}

	mutex_lock(&dev->resource_mutex);

	ret = dev->ops->enable_dir_port(&dev->hw, domain_id, &arg, &response);

	/* Allow threads to block on this port's CQ interrupt */
	if (!ret)
		WRITE_ONCE(dev->intr.dir_cq_intr[arg.port_id].disabled, false);

	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(struct dlb2_cmd_response))) {
		dev_err(dev->dlb2_device,
			"[%s()] Invalid ioctl response pointer\n",
			__func__);
		return -EFAULT;
	}

	dev_dbg(dev->dlb2_device, "Exiting %s()\n", __func__);

	return ret;
}

int dlb2_domain_ioctl_disable_ldb_port(struct dlb2_dev *dev,
				       struct dlb2_status *status,
				       unsigned long user_arg,
				       u32 domain_id)
{
	struct dlb2_disable_ldb_port_args arg;
	struct dlb2_cmd_response response = {0};
	int ret;

	response.status = 0;

	dev_dbg(dev->dlb2_device, "Entering %s()\n", __func__);

	if (copy_from_user(&arg,
			   (void  __user *)user_arg,
			   sizeof(struct dlb2_disable_ldb_port_args))) {
		dev_err(dev->dlb2_device,
			"[%s()] Invalid ioctl argument pointer\n",
			__func__);
		return -EFAULT;
	}

	mutex_lock(&dev->resource_mutex);

	ret = dev->ops->disable_ldb_port(&dev->hw, domain_id, &arg, &response);

	/* Wake threads blocked on this port's CQ interrupt, and prevent
	 * subsequent attempts to block on it.
	 */
	if (!ret)
		dlb2_wake_thread(dev,
				 &dev->intr.ldb_cq_intr[arg.port_id],
				 WAKE_PORT_DISABLED);

	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(struct dlb2_cmd_response))) {
		dev_err(dev->dlb2_device,
			"[%s()] Invalid ioctl response pointer\n",
			__func__);
		return -EFAULT;
	}

	dev_dbg(dev->dlb2_device, "Exiting %s()\n", __func__);

	return ret;
}

int dlb2_domain_ioctl_disable_dir_port(struct dlb2_dev *dev,
				       struct dlb2_status *status,
				       unsigned long user_arg,
				       u32 domain_id)
{
	struct dlb2_disable_dir_port_args arg;
	struct dlb2_cmd_response response = {0};
	int ret;

	response.status = 0;

	dev_dbg(dev->dlb2_device, "Entering %s()\n", __func__);

	if (copy_from_user(&arg,
			   (void  __user *)user_arg,
			   sizeof(struct dlb2_disable_dir_port_args))) {
		dev_err(dev->dlb2_device,
			"[%s()] Invalid ioctl argument pointer\n",
			__func__);
		return -EFAULT;
	}

	mutex_lock(&dev->resource_mutex);

	ret = dev->ops->disable_dir_port(&dev->hw, domain_id, &arg, &response);

	/* Wake threads blocked on this port's CQ interrupt, and prevent
	 * subsequent attempts to block on it.
	 */
	if (!ret)
		dlb2_wake_thread(dev,
				 &dev->intr.dir_cq_intr[arg.port_id],
				 WAKE_PORT_DISABLED);

	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(struct dlb2_cmd_response))) {
		dev_err(dev->dlb2_device,
			"[%s()] Invalid ioctl response pointer\n",
			__func__);
		return -EFAULT;
	}

	dev_dbg(dev->dlb2_device, "Exiting %s()\n", __func__);

	return ret;
}

/* Port creation ioctls don't use the callback template macro because they have
 * a number of OS-dependent memory operations.
 */
int dlb2_domain_ioctl_create_ldb_port(struct dlb2_dev *dev,
				      struct dlb2_status *status,
				      unsigned long user_arg,
				      u32 domain_id)
{
	struct dlb2_create_ldb_port_args arg;
	struct dlb2_cmd_response response;
	dma_addr_t cq_dma_base = 0;
	void *cq_base = NULL;
	struct dlb2_domain_dev *domain;
	int ret;

	dev_dbg(dev->dlb2_device, "Entering %s()\n", __func__);

	response.status = 0;

	if (copy_from_user(&arg, (void __user *)user_arg, sizeof(arg))) {
		dev_err(dev->dlb2_device,
			"[%s()] Invalid ioctl argument pointer\n", __func__);
		return -EFAULT;
	}

	mutex_lock(&dev->resource_mutex);

	if (domain_id >= DLB2_MAX_NUM_DOMAINS) {
		response.status = DLB2_ST_INVALID_DOMAIN_ID;
		ret = -EPERM;
		goto unlock;
	}

	domain = &dev->sched_domains[domain_id];

	cq_base = dma_alloc_coherent(&dev->pdev->dev,
				     DLB2_LDB_CQ_MAX_SIZE,
				     &cq_dma_base,
				     GFP_KERNEL);
	if (!cq_base) {
		response.status = DLB2_ST_NO_MEMORY;
		ret = -ENOMEM;
		goto unlock;
	}

	ret = dev->ops->create_ldb_port(&dev->hw,
					domain_id,
					&arg,
					(uintptr_t)cq_dma_base,
					&response);
	if (ret)
		goto unlock;

	ret = dev->ops->enable_ldb_cq_interrupts(dev,
						 response.id,
						 arg.cq_depth_threshold);
	if (ret)
		goto unlock; /* Internal error, don't unwind port creation */

	/* Fill out the per-port memory tracking structure */
	dev->ldb_port_mem[response.id].domain_id = domain_id;
	dev->ldb_port_mem[response.id].cq_base = cq_base;
	dev->ldb_port_mem[response.id].cq_dma_base = cq_dma_base;
	dev->ldb_port_mem[response.id].valid = true;

unlock:
	if (ret) {
		dev_err(dev->dlb2_device, "[%s()]: Error %s\n",
			__func__, dlb2_error_strings[response.status]);

		if (cq_dma_base)
			dma_free_coherent(&dev->pdev->dev,
					  DLB2_LDB_CQ_MAX_SIZE,
					  cq_base,
					  cq_dma_base);
	} else {
		dev_dbg(dev->dlb2_device, "CQ PA: 0x%llx\n",
			virt_to_phys(cq_base));
		dev_dbg(dev->dlb2_device, "CQ IOVA: 0x%llx\n", cq_dma_base);
	}

	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(struct dlb2_cmd_response))) {
		dev_err(dev->dlb2_device,
			"[%s()] Invalid ioctl response pointer\n", __func__);
		return -EFAULT;
	}

	dev_dbg(dev->dlb2_device, "Exiting %s()\n", __func__);

	return ret;
}

int dlb2_domain_ioctl_create_dir_port(struct dlb2_dev *dev,
				      struct dlb2_status *status,
				      unsigned long user_arg,
				      u32 domain_id)
{
	struct dlb2_create_dir_port_args arg;
	struct dlb2_cmd_response response;
	struct dlb2_domain_dev *domain;
	dma_addr_t cq_dma_base = 0;
	void *cq_base = NULL;
	int ret;

	dev_dbg(dev->dlb2_device, "Entering %s()\n", __func__);

	response.status = 0;

	if (copy_from_user(&arg, (void __user *)user_arg, sizeof(arg))) {
		dev_err(dev->dlb2_device,
			"[%s()] Invalid ioctl argument pointer\n", __func__);
		return -EFAULT;
	}

	mutex_lock(&dev->resource_mutex);

	if (domain_id >= DLB2_MAX_NUM_DOMAINS) {
		response.status = DLB2_ST_INVALID_DOMAIN_ID;
		ret = -EPERM;
		goto unlock;
	}

	domain = &dev->sched_domains[domain_id];

	cq_base = dma_alloc_coherent(&dev->pdev->dev,
				     DLB2_DIR_CQ_MAX_SIZE,
				     &cq_dma_base,
				     GFP_KERNEL);
	if (!cq_base) {
		response.status = DLB2_ST_NO_MEMORY;
		ret = -ENOMEM;
		goto unlock;
	}

	ret = dev->ops->create_dir_port(&dev->hw,
					domain_id,
					&arg,
					(uintptr_t)cq_dma_base,
					&response);
	if (ret)
		goto unlock;

	dev->ops->enable_dir_cq_interrupts(dev,
					   response.id,
					   arg.cq_depth_threshold);
	if (ret)
		goto unlock; /* Internal error, don't unwind port creation */

	/* Fill out the per-port memory tracking structure */
	dev->dir_port_mem[response.id].domain_id = domain_id;
	dev->dir_port_mem[response.id].cq_base = cq_base;
	dev->dir_port_mem[response.id].cq_dma_base = cq_dma_base;
	dev->dir_port_mem[response.id].valid = true;

unlock:
	if (ret) {
		dev_err(dev->dlb2_device, "[%s()]: Error %s\n",
			__func__, dlb2_error_strings[response.status]);

		if (cq_dma_base)
			dma_free_coherent(&dev->pdev->dev,
					  DLB2_DIR_CQ_MAX_SIZE,
					  cq_base,
					  cq_dma_base);
	} else {
		dev_dbg(dev->dlb2_device, "CQ PA: 0x%llx\n",
			virt_to_phys(cq_base));
		dev_dbg(dev->dlb2_device, "CQ IOVA: 0x%llx\n", cq_dma_base);
	}

	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(struct dlb2_cmd_response))) {
		dev_err(dev->dlb2_device,
			"[%s()] Invalid ioctl response pointer\n", __func__);
		return -EFAULT;
	}

	dev_dbg(dev->dlb2_device, "Exiting %s()\n", __func__);

	return ret;
}

int dlb2_domain_ioctl_block_on_cq_interrupt(struct dlb2_dev *dev,
					    struct dlb2_status *status,
					    unsigned long user_arg,
					    u32 domain_id)
{
	struct dlb2_block_on_cq_interrupt_args arg;
	struct dlb2_cmd_response response;
	int ret = 0;

	response.status = 0;

	dev_dbg(dev->dlb2_device, "Entering %s()\n", __func__);
	if (copy_from_user(&arg,
			   (void  __user *)user_arg,
			   sizeof(struct dlb2_block_on_cq_interrupt_args))) {
		dev_err(dev->dlb2_device,
			"[%s()] Invalid ioctl argument pointer\n", __func__);
		return -EFAULT;
	}

	ret = dlb2_block_on_cq_interrupt(dev,
					 status,
					 domain_id,
					 arg.port_id,
					 arg.is_ldb,
					 arg.cq_va,
					 arg.cq_gen,
					 arg.arm);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(struct dlb2_cmd_response))) {
		dev_err(dev->dlb2_device,
			"[%s()] Invalid ioctl response pointer\n", __func__);
		return -EFAULT;
	}

	dev_dbg(dev->dlb2_device, "Exiting %s()\n", __func__);

	return ret;
}

int dlb2_domain_ioctl_enqueue_domain_alert(struct dlb2_dev *dev,
					   struct dlb2_status *status,
					   unsigned long user_arg,
					   u32 domain_id)
{
	struct dlb2_enqueue_domain_alert_args arg;
	struct dlb2_domain_dev *domain;
	struct dlb2_domain_alert alert;

	dev_dbg(dev->dlb2_device, "Entering %s()\n", __func__);

	if (copy_from_user(&arg,
			   (void  __user *)user_arg,
			   sizeof(struct dlb2_enqueue_domain_alert_args))) {
		dev_err(dev->dlb2_device,
			"[%s()] Invalid ioctl argument pointer\n", __func__);
		return -EFAULT;
	}

	domain = &dev->sched_domains[domain_id];

	/* Grab the alert mutex to access the read and write indexes */
	if (mutex_lock_interruptible(&domain->alert_mutex))
		return -ERESTARTSYS;

	/* If there's no space for this notification, return */
	if ((domain->alert_wr_idx - domain->alert_rd_idx) ==
	    (DLB2_DOMAIN_ALERT_RING_SIZE - 1)) {
		mutex_unlock(&domain->alert_mutex);
		return 0;
	}

	alert.alert_id = DLB2_DOMAIN_ALERT_USER;
	alert.aux_alert_data = arg.aux_alert_data;

	domain->alerts[domain->alert_wr_idx++] = alert;

	mutex_unlock(&domain->alert_mutex);

	wake_up_interruptible(&domain->wq_head);

	dev_dbg(dev->dlb2_device, "Exiting %s()\n", __func__);

	return 0;
}

static dlb2_domain_ioctl_callback_fn_t
dlb2_domain_ioctl_callback_fns[NUM_DLB2_DOMAIN_CMD] = {
	dlb2_domain_ioctl_create_ldb_queue,
	dlb2_domain_ioctl_create_dir_queue,
	dlb2_domain_ioctl_create_ldb_port,
	dlb2_domain_ioctl_create_dir_port,
	dlb2_domain_ioctl_start_domain,
	dlb2_domain_ioctl_map_qid,
	dlb2_domain_ioctl_unmap_qid,
	dlb2_domain_ioctl_enable_ldb_port,
	dlb2_domain_ioctl_enable_dir_port,
	dlb2_domain_ioctl_disable_ldb_port,
	dlb2_domain_ioctl_disable_dir_port,
	dlb2_domain_ioctl_block_on_cq_interrupt,
	dlb2_domain_ioctl_enqueue_domain_alert,
	dlb2_domain_ioctl_get_ldb_queue_depth,
	dlb2_domain_ioctl_get_dir_queue_depth,
	dlb2_domain_ioctl_pending_port_unmaps,
};

int dlb2_domain_ioctl_dispatcher(struct dlb2_dev *dev,
				 struct dlb2_status *st,
				 unsigned int cmd,
				 unsigned long arg,
				 u32 domain_id)
{
	int cmd_nr = _IOC_NR(cmd);

	if (cmd_nr >= NUM_DLB2_DOMAIN_CMD) {
		dev_err(dev->dlb2_device,
			"[%s()] Unexpected DLB DOMAIN command %d\n",
			__func__, _IOC_NR(cmd));
		return -1;
	}

	return dlb2_domain_ioctl_callback_fns[cmd_nr](dev, st, arg, domain_id);
}

typedef int (*dlb2_ioctl_callback_fn_t)(struct dlb2_dev *dev,
					unsigned long arg);

/* [7:0]: device revision, [15:8]: device version */
#define DLB2_SET_DEVICE_VERSION(ver, rev) (((ver) << 8) | (rev))

int dlb2_ioctl_get_device_version(struct dlb2_dev *dev,
				  unsigned long user_arg)
{
	struct dlb2_get_device_version_args arg;
	struct dlb2_cmd_response response;

	dev_dbg(dev->dlb2_device, "Entering %s()\n", __func__);

	response.status = 0;
	response.id = DLB2_SET_DEVICE_VERSION(2, DLB2_REV_A0);

	if (copy_from_user(&arg, (void __user *)user_arg, sizeof(arg))) {
		dev_err(dev->dlb2_device,
			"[%s()] Invalid ioctl argument pointer\n", __func__);
		return -EFAULT;
	}

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(struct dlb2_cmd_response))) {
		dev_err(dev->dlb2_device,
			"[%s()] Invalid ioctl response pointer\n", __func__);
		return -EFAULT;
	}

	dev_dbg(dev->dlb2_device, "Exiting %s()\n", __func__);

	return 0;
}

int __dlb2_ioctl_create_sched_domain(struct dlb2_dev *dev,
				     unsigned long user_arg,
				     bool user,
				     struct dlb2_dp *dlb2_dp)
{
	struct dlb2_create_sched_domain_args arg;
	struct dlb2_cmd_response response;
	int ret;

	response.status = 0;

	dev_dbg(dev->dlb2_device, "Entering %s()\n", __func__);

	if (copy_from_user(&arg,
			   (void  __user *)user_arg,
			   sizeof(struct dlb2_create_sched_domain_args))) {
		dev_err(dev->dlb2_device,
			"[%s()] Invalid ioctl argument pointer\n", __func__);
		return -EFAULT;
	}

	mutex_lock(&dev->resource_mutex);

	if (dev->domain_reset_failed) {
		response.status = DLB2_ST_DOMAIN_RESET_FAILED;
		ret = -EINVAL;
		goto unlock;
	}

	ret = dev->ops->create_sched_domain(&dev->hw, &arg, &response);
	if (ret)
		goto unlock;

	ret = dlb2_add_domain_device_file(dev, response.id);
	if (ret)
		goto unlock;

	dev->sched_domains[response.id].user_mode = user;
#ifdef CONFIG_INTEL_DLB2_DATAPATH
	if (!user) {
		/* The dp pointer is used to set the structure's 'shutdown'
		 * field in case of an unexpected FLR.
		 */
		dev->sched_domains[response.id].dp =
			&dlb2_dp->domains[response.id];
	}
#endif

unlock:
	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(struct dlb2_cmd_response))) {
		dev_err(dev->dlb2_device,
			"[%s()] Invalid ioctl response pointer\n", __func__);
		return -EFAULT;
	}

	dev_dbg(dev->dlb2_device, "Exiting %s()\n", __func__);

	return ret;
}

int dlb2_ioctl_create_sched_domain(struct dlb2_dev *dev,
				   unsigned long user_arg)
{
	return __dlb2_ioctl_create_sched_domain(dev, user_arg, true, NULL);
}

int dlb2_ioctl_get_num_resources(struct dlb2_dev *dev,
				 unsigned long user_arg)
{
	struct dlb2_get_num_resources_args arg;
	int ret;

	dev_dbg(dev->dlb2_device, "Entering %s()\n", __func__);

	mutex_lock(&dev->resource_mutex);

	ret = dev->ops->get_num_resources(&dev->hw, &arg);

	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)user_arg,
			 &arg,
			 sizeof(struct dlb2_get_num_resources_args))) {
		dev_err(dev->dlb2_device, "Invalid DLB resources pointer\n");
		return -EFAULT;
	}

	dev_dbg(dev->dlb2_device, "Exiting %s()\n", __func__);

	return ret;
}

int dlb2_ioctl_get_driver_version(struct dlb2_dev *dev,
				  unsigned long user_arg)
{
	struct dlb2_get_driver_version_args arg;
	struct dlb2_cmd_response response;

	dev_dbg(dev->dlb2_device, "Entering %s()\n", __func__);

	response.status = 0;
	response.id = DLB2_VERSION;

	if (copy_from_user(&arg, (void __user *)user_arg, sizeof(arg))) {
		dev_err(dev->dlb2_device,
			"[%s()] Invalid ioctl argument pointer\n", __func__);
		return -EFAULT;
	}

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(struct dlb2_cmd_response))) {
		dev_err(dev->dlb2_device,
			"[%s()] Invalid ioctl response pointer\n", __func__);
		return -EFAULT;
	}

	dev_dbg(dev->dlb2_device, "Exiting %s()\n", __func__);

	return 0;
}

int dlb2_ioctl_write_smon(struct dlb2_dev *dev,
			  unsigned long user_arg)
{
	struct dlb2_cmd_response response;
	struct dlb2_write_smon_args arg;
	int ret = 0;

	if (copy_from_user(&arg, (void __user *)user_arg, sizeof(arg))) {
		pr_err("Invalid ioctl argument pointer\n");
		return -EFAULT;
	}

	response.status = 0;

	ret = dev->ops->write_smon(dev, &arg, &response);

	if (copy_to_user((void __user *)user_arg, &arg, sizeof(arg)))
		return -EFAULT;

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(struct dlb2_cmd_response))) {
		pr_err("Invalid ioctl response pointer\n");
		return -EFAULT;
	}

	return ret;
}

int dlb2_ioctl_read_smon(struct dlb2_dev *dev,
			 unsigned long user_arg)
{
	struct dlb2_cmd_response response;
	struct dlb2_read_smon_args arg;
	int ret = 0;

	if (copy_from_user(&arg, (void __user *)user_arg, sizeof(arg))) {
		pr_err("Invalid ioctl argument pointer\n");
		return -EFAULT;
	}

	response.status = 0;

	ret = dev->ops->read_smon(dev, &arg, &response);

	if (copy_to_user((void __user *)user_arg, &arg, sizeof(arg)))
		return -EFAULT;

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(struct dlb2_cmd_response))) {
		pr_err("Invalid ioctl response pointer\n");
		return -EFAULT;
	}

	return ret;
}

int dlb2_ioctl_set_sn_allocation(struct dlb2_dev *dev, unsigned long user_arg)
{
	struct dlb2_set_sn_allocation_args arg;
	struct dlb2_cmd_response response;
	unsigned int group;
	unsigned long num;
	int ret;

	dev_dbg(dev->dlb2_device, "Entering %s()\n", __func__);

	if (copy_from_user(&arg,
			   (void  __user *)user_arg,
			   sizeof(struct dlb2_set_sn_allocation_args))) {
		dev_err(dev->dlb2_device,
			"[%s()] Invalid ioctl argument pointer\n",
			__func__);
		return -EFAULT;
	}

	response.status = 0;

	mutex_lock(&dev->resource_mutex);

	group = arg.group;
	num = arg.num;

	/* Only the PF can modify the SN allocations */
	if (dev->type == DLB2_PF)
		ret = dlb2_set_group_sequence_numbers(&dev->hw, group, num);
	else
		ret = -EPERM;

	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(struct dlb2_cmd_response))) {
		dev_err(dev->dlb2_device,
			"[%s()] Invalid ioctl response pointer\n",
			__func__);
		return -EFAULT;
	}

	dev_dbg(dev->dlb2_device, "Exiting %s()\n", __func__);

	return ret;
}

int dlb2_ioctl_get_sn_allocation(struct dlb2_dev *dev, unsigned long user_arg)
{
	struct dlb2_get_sn_allocation_args arg;
	struct dlb2_cmd_response response;
	int ret;

	dev_dbg(dev->dlb2_device, "Entering %s()\n", __func__);

	if (copy_from_user(&arg,
			   (void  __user *)user_arg,
			   sizeof(struct dlb2_get_sn_allocation_args))) {
		dev_err(dev->dlb2_device,
			"[%s()] Invalid ioctl argument pointer\n",
			__func__);
		return -EFAULT;
	}

	response.status = 0;

	mutex_lock(&dev->resource_mutex);

	ret = dev->ops->get_sn_allocation(&dev->hw, arg.group);

	response.id = ret;

	ret = (ret > 0) ? 0 : ret;

	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(struct dlb2_cmd_response))) {
		dev_err(dev->dlb2_device,
			"[%s()] Invalid ioctl response pointer\n",
			__func__);
		return -EFAULT;
	}

	dev_dbg(dev->dlb2_device, "Exiting %s()\n", __func__);

	return ret;
}

int dlb2_ioctl_set_cos_bw(struct dlb2_dev *dev, unsigned long user_arg)
{
	struct dlb2_cmd_response response;
	struct dlb2_set_cos_bw_args arg;
	int ret;

	dev_dbg(dev->dlb2_device, "Entering %s()\n", __func__);

	if (copy_from_user(&arg,
			   (void  __user *)user_arg,
			   sizeof(struct dlb2_set_cos_bw_args))) {
		dev_err(dev->dlb2_device,
			"[%s()] Invalid ioctl argument pointer\n",
			__func__);
		return -EFAULT;
	}

	response.status = 0;

	mutex_lock(&dev->resource_mutex);

	/* Only the PF can modify the CoS reservations */
	if (dev->type == DLB2_PF)
		ret = dlb2_hw_set_cos_bandwidth(&dev->hw,
						arg.cos_id,
						arg.bandwidth);
	else
		ret = -EPERM;

	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(struct dlb2_cmd_response))) {
		dev_err(dev->dlb2_device,
			"[%s()] Invalid ioctl response pointer\n",
			__func__);
		return -EFAULT;
	}

	dev_dbg(dev->dlb2_device, "Exiting %s()\n", __func__);

	return ret;
}

int dlb2_ioctl_get_cos_bw(struct dlb2_dev *dev, unsigned long user_arg)
{
	struct dlb2_cmd_response response;
	struct dlb2_get_cos_bw_args arg;
	int ret;

	dev_dbg(dev->dlb2_device, "Entering %s()\n", __func__);

	if (copy_from_user(&arg,
			   (void  __user *)user_arg,
			   sizeof(struct dlb2_get_cos_bw_args))) {
		dev_err(dev->dlb2_device,
			"[%s()] Invalid ioctl argument pointer\n",
			__func__);
		return -EFAULT;
	}

	response.status = 0;

	mutex_lock(&dev->resource_mutex);

	ret = dev->ops->get_cos_bw(&dev->hw, arg.cos_id);

	response.id = ret;

	ret = (ret > 0) ? 0 : ret;

	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(struct dlb2_cmd_response))) {
		dev_err(dev->dlb2_device,
			"[%s()] Invalid ioctl response pointer\n",
			__func__);
		return -EFAULT;
	}

	dev_dbg(dev->dlb2_device, "Exiting %s()\n", __func__);

	return ret;
}

int dlb2_ioctl_get_sn_occupancy(struct dlb2_dev *dev, unsigned long user_arg)
{
	struct dlb2_get_sn_occupancy_args arg;
	struct dlb2_cmd_response response;
	int ret;

	dev_dbg(dev->dlb2_device, "Entering %s()\n", __func__);

	if (copy_from_user(&arg,
			   (void  __user *)user_arg,
			   sizeof(struct dlb2_get_sn_occupancy_args))) {
		dev_err(dev->dlb2_device,
			"[%s()] Invalid ioctl argument pointer\n",
			__func__);
		return -EFAULT;
	}

	response.status = 0;

	mutex_lock(&dev->resource_mutex);

	ret = dev->ops->get_sn_occupancy(&dev->hw, arg.group);

	response.id = ret;

	ret = (ret > 0) ? 0 : ret;

	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(struct dlb2_cmd_response))) {
		dev_err(dev->dlb2_device,
			"[%s()] Invalid ioctl response pointer\n",
			__func__);
		return -EFAULT;
	}

	dev_dbg(dev->dlb2_device, "Exiting %s()\n", __func__);

	return ret;
}

int dlb2_ioctl_query_cq_poll_mode(struct dlb2_dev *dev,
				  unsigned long user_arg)
{
	struct dlb2_query_cq_poll_mode_args arg;
	struct dlb2_cmd_response response;
	int ret;

	if (copy_from_user(&arg,
			   (void  __user *)user_arg,
			   sizeof(struct dlb2_query_cq_poll_mode_args))) {
		dev_err(dev->dlb2_device,
			"[%s()] Invalid ioctl argument pointer\n",
			__func__);
		return -EFAULT;
	}

	mutex_lock(&dev->resource_mutex);

	ret = dev->ops->query_cq_poll_mode(dev, &response);

	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(struct dlb2_cmd_response))) {
		pr_err("Invalid ioctl response pointer\n");
		return -EFAULT;
	}

	return ret;
}

static dlb2_ioctl_callback_fn_t dlb2_ioctl_callback_fns[NUM_DLB2_CMD] = {
	dlb2_ioctl_get_device_version,
	dlb2_ioctl_create_sched_domain,
	dlb2_ioctl_get_num_resources,
	dlb2_ioctl_get_driver_version,
	dlb2_ioctl_write_smon,
	dlb2_ioctl_read_smon,
	dlb2_ioctl_set_sn_allocation,
	dlb2_ioctl_get_sn_allocation,
	dlb2_ioctl_set_cos_bw,
	dlb2_ioctl_get_cos_bw,
	dlb2_ioctl_get_sn_occupancy,
	dlb2_ioctl_query_cq_poll_mode,
};

int dlb2_ioctl_dispatcher(struct dlb2_dev *dev,
			  unsigned int cmd,
			  unsigned long arg)
{
	if (_IOC_NR(cmd) >= NUM_DLB2_CMD) {
		dev_err(dev->dlb2_device,
			"[%s()] Unexpected DLB command %d\n",
			__func__, _IOC_NR(cmd));
		return -1;
	}

	return dlb2_ioctl_callback_fns[_IOC_NR(cmd)](dev, arg);
}
