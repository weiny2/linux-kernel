// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2017-2018 Intel Corporation */

#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/iommu.h>

#include "hqmv2_main.h"
#include "hqmv2_intr.h"
#include <uapi/linux/hqmv2_user.h>
#include "base/hqmv2_resource.h"

#ifdef CONFIG_INTEL_HQMV2_DATAPATH
#include "hqmv2_dp_priv.h"
#endif

typedef int (*hqmv2_domain_ioctl_callback_fn_t)(struct hqmv2_dev *dev,
					      unsigned long arg,
					      u32 domain_id);

/* The HQM domain ioctl callback template minimizes replication of boilerplate
 * code to copy arguments, acquire and release the resource lock, and execute
 * the command.  The arguments and response structure name should have the
 * format hqmv2_<lower_name>_args.
 */
#define HQMV2_DOMAIN_IOCTL_CALLBACK_TEMPLATE(lower_name)		\
int hqmv2_domain_ioctl_##lower_name(struct hqmv2_dev *dev,		\
				  unsigned long user_arg,		\
				  u32 domain_id)			\
{									\
	struct hqmv2_##lower_name##_args arg;				\
	struct hqmv2_cmd_response response = {0};			\
	int ret;							\
	response.status = 0;						\
									\
	HQMV2_INFO(dev->hqmv2_device, "Entering %s()\n", __func__);	\
									\
	if (copy_from_user(&arg,					\
			   (void  __user *)user_arg,			\
			   sizeof(struct hqmv2_##lower_name##_args))) {	\
		HQMV2_ERR(dev->hqmv2_device,				\
			"[%s()] Invalid ioctl argument pointer\n",	\
			__func__);					\
		return -EFAULT;						\
	}								\
									\
	mutex_lock(&dev->resource_mutex);				\
									\
	/* See hqmv2_reset_notify() for more details */			\
	if (READ_ONCE(dev->unexpected_reset)) {				\
		HQMV2_ERR(dev->hqmv2_device,				\
			"[%s()] The HQM was unexpectedly reset.\n",	\
			__func__);					\
		mutex_unlock(&dev->resource_mutex);			\
		return -EINVAL;						\
	}								\
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
			 sizeof(struct hqmv2_cmd_response))) {		\
		HQMV2_ERR(dev->hqmv2_device,				\
			"[%s()] Invalid ioctl response pointer\n",	\
			__func__);					\
		return -EFAULT;						\
	}								\
									\
	HQMV2_INFO(dev->hqmv2_device, "Exiting %s()\n", __func__);	\
									\
	return ret;							\
}

HQMV2_DOMAIN_IOCTL_CALLBACK_TEMPLATE(create_ldb_queue)
HQMV2_DOMAIN_IOCTL_CALLBACK_TEMPLATE(create_dir_queue)
HQMV2_DOMAIN_IOCTL_CALLBACK_TEMPLATE(start_domain)
HQMV2_DOMAIN_IOCTL_CALLBACK_TEMPLATE(map_qid)
HQMV2_DOMAIN_IOCTL_CALLBACK_TEMPLATE(unmap_qid)
HQMV2_DOMAIN_IOCTL_CALLBACK_TEMPLATE(get_ldb_queue_depth)
HQMV2_DOMAIN_IOCTL_CALLBACK_TEMPLATE(get_dir_queue_depth)
HQMV2_DOMAIN_IOCTL_CALLBACK_TEMPLATE(pending_port_unmaps)

/* Port enable/disable ioctls don't use the callback template macro because
 * they have additional CQ interrupt management logic.
 */
int hqmv2_domain_ioctl_enable_ldb_port(struct hqmv2_dev *dev,
				       unsigned long user_arg,
				       u32 domain_id)
{
	struct hqmv2_enable_ldb_port_args arg;
	struct hqmv2_cmd_response response = {0};
	int ret;

	response.status = 0;

	HQMV2_INFO(dev->hqmv2_device, "Entering %s()\n", __func__);

	if (copy_from_user(&arg,
			   (void  __user *)user_arg,
			   sizeof(struct hqmv2_enable_ldb_port_args))) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Invalid ioctl argument pointer\n",
			  __func__);
		return -EFAULT;
	}

	mutex_lock(&dev->resource_mutex);

	/* See hqmv2_reset_notify() for more details */
	if (READ_ONCE(dev->unexpected_reset)) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] The HQM was unexpectedly reset.\n",
			  __func__);
		mutex_unlock(&dev->resource_mutex);
		return -EINVAL;
	}

	ret = dev->ops->enable_ldb_port(&dev->hw, domain_id, &arg, &response);

	/* Allow threads to block on this port's CQ interrupt */
	if (!ret)
		WRITE_ONCE(dev->intr.ldb_cq_intr[arg.port_id].disabled, false);

	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(struct hqmv2_cmd_response))) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Invalid ioctl response pointer\n",
			  __func__);
		return -EFAULT;
	}

	HQMV2_INFO(dev->hqmv2_device, "Exiting %s()\n", __func__);

	return ret;
}

int hqmv2_domain_ioctl_enable_dir_port(struct hqmv2_dev *dev,
				       unsigned long user_arg,
				       u32 domain_id)
{
	struct hqmv2_enable_dir_port_args arg;
	struct hqmv2_cmd_response response = {0};
	int ret;

	response.status = 0;

	HQMV2_INFO(dev->hqmv2_device, "Entering %s()\n", __func__);

	if (copy_from_user(&arg,
			   (void  __user *)user_arg,
			   sizeof(struct hqmv2_enable_dir_port_args))) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Invalid ioctl argument pointer\n",
			  __func__);
		return -EFAULT;
	}

	mutex_lock(&dev->resource_mutex);

	/* See hqmv2_reset_notify() for more details */
	if (READ_ONCE(dev->unexpected_reset)) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] The HQM was unexpectedly reset.\n",
			  __func__);
		mutex_unlock(&dev->resource_mutex);
		return -EINVAL;
	}

	ret = dev->ops->enable_dir_port(&dev->hw, domain_id, &arg, &response);

	/* Allow threads to block on this port's CQ interrupt */
	if (!ret)
		WRITE_ONCE(dev->intr.dir_cq_intr[arg.port_id].disabled, false);

	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(struct hqmv2_cmd_response))) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Invalid ioctl response pointer\n",
			  __func__);
		return -EFAULT;
	}

	HQMV2_INFO(dev->hqmv2_device, "Exiting %s()\n", __func__);

	return ret;
}

int hqmv2_domain_ioctl_disable_ldb_port(struct hqmv2_dev *dev,
					unsigned long user_arg,
					u32 domain_id)
{
	struct hqmv2_disable_ldb_port_args arg;
	struct hqmv2_cmd_response response = {0};
	int ret;

	response.status = 0;

	HQMV2_INFO(dev->hqmv2_device, "Entering %s()\n", __func__);

	if (copy_from_user(&arg,
			   (void  __user *)user_arg,
			   sizeof(struct hqmv2_disable_ldb_port_args))) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Invalid ioctl argument pointer\n",
			  __func__);
		return -EFAULT;
	}

	mutex_lock(&dev->resource_mutex);

	/* See hqmv2_reset_notify() for more details */
	if (READ_ONCE(dev->unexpected_reset)) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] The HQM was unexpectedly reset.\n",
			  __func__);
		mutex_unlock(&dev->resource_mutex);
		return -EINVAL;
	}

	ret = dev->ops->disable_ldb_port(&dev->hw, domain_id, &arg, &response);

	/* Wake threads blocked on this port's CQ interrupt, and prevent
	 * subsequent attempts to block on it.
	 */
	if (!ret)
		hqmv2_wake_thread(dev,
				  &dev->intr.ldb_cq_intr[arg.port_id],
				  WAKE_PORT_DISABLED);

	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(struct hqmv2_cmd_response))) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Invalid ioctl response pointer\n",
			  __func__);
		return -EFAULT;
	}

	HQMV2_INFO(dev->hqmv2_device, "Exiting %s()\n", __func__);

	return ret;
}

int hqmv2_domain_ioctl_disable_dir_port(struct hqmv2_dev *dev,
					unsigned long user_arg,
					u32 domain_id)
{
	struct hqmv2_disable_dir_port_args arg;
	struct hqmv2_cmd_response response = {0};
	int ret;

	response.status = 0;

	HQMV2_INFO(dev->hqmv2_device, "Entering %s()\n", __func__);

	if (copy_from_user(&arg,
			   (void  __user *)user_arg,
			   sizeof(struct hqmv2_disable_dir_port_args))) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Invalid ioctl argument pointer\n",
			  __func__);
		return -EFAULT;
	}

	mutex_lock(&dev->resource_mutex);

	/* See hqmv2_reset_notify() for more details */
	if (READ_ONCE(dev->unexpected_reset)) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] The HQM was unexpectedly reset.\n",
			  __func__);
		mutex_unlock(&dev->resource_mutex);
		return -EINVAL;
	}

	ret = dev->ops->disable_dir_port(&dev->hw, domain_id, &arg, &response);

	/* Wake threads blocked on this port's CQ interrupt, and prevent
	 * subsequent attempts to block on it.
	 */
	if (!ret)
		hqmv2_wake_thread(dev,
				  &dev->intr.dir_cq_intr[arg.port_id],
				  WAKE_PORT_DISABLED);

	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(struct hqmv2_cmd_response))) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Invalid ioctl response pointer\n",
			  __func__);
		return -EFAULT;
	}

	HQMV2_INFO(dev->hqmv2_device, "Exiting %s()\n", __func__);

	return ret;
}

/* Port creation ioctls don't use the callback template macro because they have
 * a number of OS-dependent memory operations.
 */
int hqmv2_domain_ioctl_create_ldb_port(struct hqmv2_dev *dev,
				       unsigned long user_arg,
				       u32 domain_id)
{
	struct hqmv2_create_ldb_port_args arg;
	struct hqmv2_cmd_response response;
	dma_addr_t pc_dma_base = 0;
	dma_addr_t cq_dma_base = 0;
	void *pc_base = NULL;
	void *cq_base = NULL;
	struct hqmv2_domain_dev *domain;
	int ret;

	HQMV2_INFO(dev->hqmv2_device, "Entering %s()\n", __func__);

	response.status = 0;

	if (copy_from_user(&arg, (void __user *)user_arg, sizeof(arg))) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Invalid ioctl argument pointer\n", __func__);
		return -EFAULT;
	}

	mutex_lock(&dev->resource_mutex);

	/* See hqmv2_reset_notify() for more details */
	if (READ_ONCE(dev->unexpected_reset)) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] The HQM was unexpectedly reset. To recover, all HQM applications must close.\n",
			  __func__);
		ret = -EINVAL;
		goto unlock;
	}

	if (domain_id >= HQMV2_MAX_NUM_DOMAINS) {
		response.status = HQMV2_ST_INVALID_DOMAIN_ID;
		ret = -EPERM;
		goto unlock;
	}

	domain = &dev->sched_domains[domain_id];

	cq_base = dma_alloc_coherent(&dev->pdev->dev,
				     PAGE_SIZE,
				     &cq_dma_base,
				     GFP_KERNEL);
	if (!cq_base) {
		response.status = HQMV2_ST_NO_MEMORY;
		ret = -ENOMEM;
		goto unlock;
	}

	pc_base = dma_alloc_coherent(&dev->pdev->dev,
				     PAGE_SIZE,
				     &pc_dma_base,
				     GFP_KERNEL);
	if (!pc_base) {
		response.status = HQMV2_ST_NO_MEMORY;
		ret = -ENOMEM;
		goto unlock;
	}

	ret = dev->ops->create_ldb_port(&dev->hw,
					domain_id,
					&arg,
					(uintptr_t)pc_dma_base,
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
	dev->ldb_port_pages[response.id].domain_id = domain_id;
	dev->ldb_port_pages[response.id].cq_base = cq_base;
	dev->ldb_port_pages[response.id].pc_base = pc_base;
	dev->ldb_port_pages[response.id].cq_dma_base = cq_dma_base;
	dev->ldb_port_pages[response.id].pc_dma_base = pc_dma_base;
	dev->ldb_port_pages[response.id].valid = true;

unlock:
	if (ret) {
		HQMV2_ERR(dev->hqmv2_device, "[%s()]: Error %s\n",
			  __func__, hqmv2_error_strings[response.status]);

		if (cq_dma_base)
			dma_free_coherent(&dev->pdev->dev,
					  PAGE_SIZE,
					  cq_base,
					  cq_dma_base);
		if (pc_dma_base)
			dma_free_coherent(&dev->pdev->dev,
					  PAGE_SIZE,
					  pc_base,
					  pc_dma_base);
	} else {
		HQMV2_INFO(dev->hqmv2_device, "CQ PA: 0x%llx\n",
			   virt_to_phys(cq_base));
		HQMV2_INFO(dev->hqmv2_device, "CQ IOVA: 0x%llx\n", cq_dma_base);
	}

	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(struct hqmv2_cmd_response))) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Invalid ioctl response pointer\n", __func__);
		return -EFAULT;
	}

	HQMV2_INFO(dev->hqmv2_device, "Exiting %s()\n", __func__);

	return ret;
}

int hqmv2_domain_ioctl_create_dir_port(struct hqmv2_dev *dev,
				       unsigned long user_arg,
				       u32 domain_id)
{
	struct hqmv2_create_dir_port_args arg;
	struct hqmv2_cmd_response response;
	struct hqmv2_domain_dev *domain;
	dma_addr_t pc_dma_base = 0;
	dma_addr_t cq_dma_base = 0;
	void *pc_base = NULL;
	void *cq_base = NULL;
	int ret;

	HQMV2_INFO(dev->hqmv2_device, "Entering %s()\n", __func__);

	response.status = 0;

	if (copy_from_user(&arg, (void __user *)user_arg, sizeof(arg))) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Invalid ioctl argument pointer\n", __func__);
		return -EFAULT;
	}

	mutex_lock(&dev->resource_mutex);

	/* See hqmv2_reset_notify() for more details */
	if (READ_ONCE(dev->unexpected_reset)) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] The HQM was unexpectedly reset. To recover, all HQM applications must close.\n",
			  __func__);
		ret = -EINVAL;
		goto unlock;
	}

	if (domain_id >= HQMV2_MAX_NUM_DOMAINS) {
		response.status = HQMV2_ST_INVALID_DOMAIN_ID;
		ret = -EPERM;
		goto unlock;
	}

	domain = &dev->sched_domains[domain_id];

	cq_base = dma_alloc_coherent(&dev->pdev->dev,
				     PAGE_SIZE,
				     &cq_dma_base,
				     GFP_KERNEL);
	if (!cq_base) {
		response.status = HQMV2_ST_NO_MEMORY;
		ret = -ENOMEM;
		goto unlock;
	}

	pc_base = dma_alloc_coherent(&dev->pdev->dev,
				     PAGE_SIZE,
				     &pc_dma_base,
				     GFP_KERNEL);
	if (!pc_base) {
		response.status = HQMV2_ST_NO_MEMORY;
		ret = -ENOMEM;
		goto unlock;
	}

	ret = dev->ops->create_dir_port(&dev->hw,
					domain_id,
					&arg,
					(uintptr_t)pc_dma_base,
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
	dev->dir_port_pages[response.id].domain_id = domain_id;
	dev->dir_port_pages[response.id].cq_base = cq_base;
	dev->dir_port_pages[response.id].pc_base = pc_base;
	dev->dir_port_pages[response.id].cq_dma_base = cq_dma_base;
	dev->dir_port_pages[response.id].pc_dma_base = pc_dma_base;
	dev->dir_port_pages[response.id].valid = true;

unlock:
	if (ret) {
		HQMV2_ERR(dev->hqmv2_device, "[%s()]: Error %s\n",
			  __func__, hqmv2_error_strings[response.status]);

		if (cq_dma_base)
			dma_free_coherent(&dev->pdev->dev,
					  PAGE_SIZE,
					  cq_base,
					  cq_dma_base);
		if (pc_dma_base)
			dma_free_coherent(&dev->pdev->dev,
					  PAGE_SIZE,
					  pc_base,
					  pc_dma_base);
	} else {
		HQMV2_INFO(dev->hqmv2_device, "CQ PA: 0x%llx\n",
			   virt_to_phys(cq_base));
		HQMV2_INFO(dev->hqmv2_device, "CQ IOVA: 0x%llx\n", cq_dma_base);
	}

	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(struct hqmv2_cmd_response))) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Invalid ioctl response pointer\n", __func__);
		return -EFAULT;
	}

	HQMV2_INFO(dev->hqmv2_device, "Exiting %s()\n", __func__);

	return ret;
}

int hqmv2_domain_ioctl_block_on_cq_interrupt(struct hqmv2_dev *dev,
					     unsigned long user_arg,
					     u32 domain_id)
{
	struct hqmv2_block_on_cq_interrupt_args arg;
	struct hqmv2_cmd_response response;
	int ret = 0;

	response.status = 0;

	HQMV2_INFO(dev->hqmv2_device, "Entering %s()\n", __func__);

	/* See hqmv2_reset_notify() for more details */
	if (READ_ONCE(dev->unexpected_reset)) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] The HQM was unexpectedly reset. To recover, all HQM applications must close.\n",
			  __func__);
		return -EINVAL;
	}

	if (copy_from_user(&arg,
			   (void  __user *)user_arg,
			   sizeof(struct hqmv2_block_on_cq_interrupt_args))) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Invalid ioctl argument pointer\n", __func__);
		return -EFAULT;
	}

	ret = hqmv2_block_on_cq_interrupt(dev,
					  domain_id,
					  arg.port_id,
					  arg.is_ldb,
					  arg.cq_va,
					  arg.cq_gen,
					  arg.arm);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(struct hqmv2_cmd_response))) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Invalid ioctl response pointer\n", __func__);
		return -EFAULT;
	}

	HQMV2_INFO(dev->hqmv2_device, "Exiting %s()\n", __func__);

	return ret;
}

int hqmv2_domain_ioctl_enqueue_domain_alert(struct hqmv2_dev *dev,
					    unsigned long user_arg,
					    u32 domain_id)
{
	struct hqmv2_enqueue_domain_alert_args arg;
	struct hqmv2_domain_dev *domain;
	struct hqmv2_domain_alert alert;

	HQMV2_INFO(dev->hqmv2_device, "Entering %s()\n", __func__);

	/* See hqmv2_reset_notify() for more details */
	if (READ_ONCE(dev->unexpected_reset)) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] The HQM was unexpectedly reset. To recover, all HQM applications must close.\n",
			  __func__);
		return -EINVAL;
	}

	if (copy_from_user(&arg,
			   (void  __user *)user_arg,
			   sizeof(struct hqmv2_enqueue_domain_alert_args))) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Invalid ioctl argument pointer\n", __func__);
		return -EFAULT;
	}

	domain = &dev->sched_domains[domain_id];

	/* Grab the alert mutex to access the read and write indexes */
	if (mutex_lock_interruptible(&domain->alert_mutex))
		return -ERESTARTSYS;

	/* If there's no space for this notification, return */
	if ((domain->alert_wr_idx - domain->alert_rd_idx) ==
	    (HQMV2_DOMAIN_ALERT_RING_SIZE - 1)) {
		mutex_unlock(&domain->alert_mutex);
		return 0;
	}

	alert.alert_id = HQMV2_DOMAIN_ALERT_USER;
	alert.aux_alert_data = arg.aux_alert_data;

	domain->alerts[domain->alert_wr_idx++] = alert;

	mutex_unlock(&domain->alert_mutex);

	wake_up_interruptible(&domain->wq_head);

	HQMV2_INFO(dev->hqmv2_device, "Exiting %s()\n", __func__);

	return 0;
}

static hqmv2_domain_ioctl_callback_fn_t
hqmv2_domain_ioctl_callback_fns[NUM_HQMV2_DOMAIN_CMD] = {
	hqmv2_domain_ioctl_create_ldb_queue,
	hqmv2_domain_ioctl_create_dir_queue,
	hqmv2_domain_ioctl_create_ldb_port,
	hqmv2_domain_ioctl_create_dir_port,
	hqmv2_domain_ioctl_start_domain,
	hqmv2_domain_ioctl_map_qid,
	hqmv2_domain_ioctl_unmap_qid,
	hqmv2_domain_ioctl_enable_ldb_port,
	hqmv2_domain_ioctl_enable_dir_port,
	hqmv2_domain_ioctl_disable_ldb_port,
	hqmv2_domain_ioctl_disable_dir_port,
	hqmv2_domain_ioctl_block_on_cq_interrupt,
	hqmv2_domain_ioctl_enqueue_domain_alert,
	hqmv2_domain_ioctl_get_ldb_queue_depth,
	hqmv2_domain_ioctl_get_dir_queue_depth,
	hqmv2_domain_ioctl_pending_port_unmaps,
};

int hqmv2_domain_ioctl_dispatcher(struct hqmv2_dev *dev,
				  unsigned int cmd,
				  unsigned long arg,
				  u32 domain_id)
{
	int cmd_nr = _IOC_NR(cmd);

	if (cmd_nr >= NUM_HQMV2_DOMAIN_CMD) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Unexpected HQM DOMAIN command %d\n",
			  __func__, _IOC_NR(cmd));
		return -1;
	}

	return hqmv2_domain_ioctl_callback_fns[cmd_nr](dev, arg, domain_id);
}

typedef int (*hqmv2_ioctl_callback_fn_t)(struct hqmv2_dev *dev,
					 unsigned long arg);

/* [7:0]: device revision, [15:8]: device version */
#define HQMV2_SET_DEVICE_VERSION(ver, rev) (((ver) << 8) | (rev))

int hqmv2_ioctl_get_device_version(struct hqmv2_dev *dev,
				   unsigned long user_arg)
{
	struct hqmv2_get_device_version_args arg;
	struct hqmv2_cmd_response response;

	HQMV2_INFO(dev->hqmv2_device, "Entering %s()\n", __func__);

	response.status = 0;
	response.id = HQMV2_SET_DEVICE_VERSION(2, HQMV2_REV_A0);

	if (copy_from_user(&arg, (void __user *)user_arg, sizeof(arg))) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Invalid ioctl argument pointer\n", __func__);
		return -EFAULT;
	}

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(struct hqmv2_cmd_response))) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Invalid ioctl response pointer\n", __func__);
		return -EFAULT;
	}

	HQMV2_INFO(dev->hqmv2_device, "Exiting %s()\n", __func__);

	return 0;
}

int __hqmv2_ioctl_create_sched_domain(struct hqmv2_dev *dev,
				      unsigned long user_arg,
				      bool user,
				      struct hqmv2_dp *hqmv2_dp)
{
	struct hqmv2_create_sched_domain_args arg;
	struct hqmv2_cmd_response response;
	int ret;

	response.status = 0;

	HQMV2_INFO(dev->hqmv2_device, "Entering %s()\n", __func__);

	if (copy_from_user(&arg,
			   (void  __user *)user_arg,
			   sizeof(struct hqmv2_create_sched_domain_args))) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Invalid ioctl argument pointer\n", __func__);
		return -EFAULT;
	}

	mutex_lock(&dev->resource_mutex);

	/* See hqmv2_reset_notify() for more details */
	if (READ_ONCE(dev->unexpected_reset)) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] The HQM was unexpectedly reset. To recover, all HQM applications must close.\n",
			  __func__);
		ret = -EINVAL;
		goto unlock;
	}

	if (dev->domain_reset_failed) {
		response.status = HQMV2_ST_DOMAIN_RESET_FAILED;
		ret = -EINVAL;
		goto unlock;
	}

	ret = dev->ops->create_sched_domain(&dev->hw, &arg, &response);
	if (ret)
		goto unlock;

	ret = hqmv2_add_domain_device_file(dev, response.id);
	if (ret)
		goto unlock;

	dev->sched_domains[response.id].user_mode = user;
#ifdef CONFIG_INTEL_HQMV2_DATAPATH
	if (!user) {
		/* The dp pointer is used to set the structure's 'shutdown'
		 * field in case of an unexpected FLR.
		 */
		dev->sched_domains[response.id].dp =
			&hqmv2_dp->domains[response.id];
	}
#endif

unlock:
	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(struct hqmv2_cmd_response))) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Invalid ioctl response pointer\n", __func__);
		return -EFAULT;
	}

	HQMV2_INFO(dev->hqmv2_device, "Exiting %s()\n", __func__);

	return ret;
}

int hqmv2_ioctl_create_sched_domain(struct hqmv2_dev *dev,
				    unsigned long user_arg)
{
	return __hqmv2_ioctl_create_sched_domain(dev, user_arg, true, NULL);
}

int hqmv2_ioctl_get_num_resources(struct hqmv2_dev *dev,
				  unsigned long user_arg)
{
	struct hqmv2_get_num_resources_args arg;
	int ret;

	HQMV2_INFO(dev->hqmv2_device, "Entering %s()\n", __func__);

	mutex_lock(&dev->resource_mutex);

	/* See hqmv2_reset_notify() for more details */
	if (READ_ONCE(dev->unexpected_reset)) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] The HQM was unexpectedly reset. To recover, all HQM applications must close.\n",
			  __func__);
		mutex_unlock(&dev->resource_mutex);
		return -EINVAL;
	}

	ret = dev->ops->get_num_resources(&dev->hw, &arg);

	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)user_arg,
			 &arg,
			 sizeof(struct hqmv2_get_num_resources_args))) {
		HQMV2_ERR(dev->hqmv2_device, "Invalid HQM resources pointer\n");
		return -EFAULT;
	}

	HQMV2_INFO(dev->hqmv2_device, "Exiting %s()\n", __func__);

	return ret;
}

int hqmv2_ioctl_get_driver_version(struct hqmv2_dev *dev,
				   unsigned long user_arg)
{
	struct hqmv2_get_driver_version_args arg;
	struct hqmv2_cmd_response response;

	HQMV2_INFO(dev->hqmv2_device, "Entering %s()\n", __func__);

	response.status = 0;
	response.id = HQMV2_VERSION;

	if (copy_from_user(&arg, (void __user *)user_arg, sizeof(arg))) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Invalid ioctl argument pointer\n", __func__);
		return -EFAULT;
	}

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(struct hqmv2_cmd_response))) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Invalid ioctl response pointer\n", __func__);
		return -EFAULT;
	}

	HQMV2_INFO(dev->hqmv2_device, "Exiting %s()\n", __func__);

	return 0;
}

int hqmv2_ioctl_sample_perf_counters(struct hqmv2_dev *dev,
				     unsigned long user_arg)
{
	struct hqmv2_sample_perf_counters_args arg;
	struct hqmv2_cmd_response response;
	int ret = 0;

	if (copy_from_user(&arg, (void __user *)user_arg, sizeof(arg))) {
		pr_err("Invalid ioctl argument pointer\n");
		return -EFAULT;
	}

	response.status = 0;

	if (arg.measurement_duration_us == 0 ||
	    arg.measurement_duration_us > 60000000) {
		response.status = HQMV2_ST_INVALID_MEASUREMENT_DURATION;
		ret = -EINVAL;
		goto done;
	}

	if (arg.perf_metric_group_id > 10) {
		response.status = HQMV2_ST_INVALID_PERF_METRIC_GROUP_ID;
		ret = -EINVAL;
		goto done;
	}

	ret = dev->ops->measure_perf(dev, &arg, &response);

done:
	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(struct hqmv2_cmd_response))) {
		pr_err("Invalid ioctl response pointer\n");
		return -EFAULT;
	}

	return ret;
}

int hqmv2_ioctl_set_sn_allocation(struct hqmv2_dev *dev, unsigned long user_arg)
{
	struct hqmv2_set_sn_allocation_args arg;
	struct hqmv2_cmd_response response;
	unsigned int group;
	unsigned long num;
	int ret;

	HQMV2_INFO(dev->hqmv2_device, "Entering %s()\n", __func__);

	if (copy_from_user(&arg,
			   (void  __user *)user_arg,
			   sizeof(struct hqmv2_set_sn_allocation_args))) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Invalid ioctl argument pointer\n",
			  __func__);
		return -EFAULT;
	}

	response.status = 0;

	mutex_lock(&dev->resource_mutex);

	/* See hqmv2_reset_notify() for more details */
	if (READ_ONCE(dev->unexpected_reset)) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] The HQM was unexpectedly reset.\n",
			  __func__);
		mutex_unlock(&dev->resource_mutex);
		return -EINVAL;
	}

	group = arg.group;
	num = arg.num;

	/* Only the PF can modify the SN allocations */
	if (dev->type == HQMV2_PF)
		ret = hqmv2_set_group_sequence_numbers(&dev->hw, group, num);
	else
		ret = -EPERM;

	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(struct hqmv2_cmd_response))) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Invalid ioctl response pointer\n",
			  __func__);
		return -EFAULT;
	}

	HQMV2_INFO(dev->hqmv2_device, "Exiting %s()\n", __func__);

	return ret;
}

int hqmv2_ioctl_get_sn_allocation(struct hqmv2_dev *dev, unsigned long user_arg)
{
	struct hqmv2_get_sn_allocation_args arg;
	struct hqmv2_cmd_response response;
	int ret;

	HQMV2_INFO(dev->hqmv2_device, "Entering %s()\n", __func__);

	if (copy_from_user(&arg,
			   (void  __user *)user_arg,
			   sizeof(struct hqmv2_get_sn_allocation_args))) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Invalid ioctl argument pointer\n",
			  __func__);
		return -EFAULT;
	}

	response.status = 0;

	mutex_lock(&dev->resource_mutex);

	/* See hqmv2_reset_notify() for more details */
	if (READ_ONCE(dev->unexpected_reset)) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] The HQM was unexpectedly reset.\n",
			  __func__);
		mutex_unlock(&dev->resource_mutex);
		return -EINVAL;
	}

	ret = dev->ops->get_sn_allocation(&dev->hw, arg.group);

	response.id = ret;

	ret = (ret > 0) ? 0 : ret;

	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(struct hqmv2_cmd_response))) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Invalid ioctl response pointer\n",
			  __func__);
		return -EFAULT;
	}

	HQMV2_INFO(dev->hqmv2_device, "Exiting %s()\n", __func__);

	return ret;
}

int hqmv2_ioctl_set_cos_bw(struct hqmv2_dev *dev, unsigned long user_arg)
{
	struct hqmv2_cmd_response response;
	struct hqmv2_set_cos_bw_args arg;
	int ret;

	HQMV2_INFO(dev->hqmv2_device, "Entering %s()\n", __func__);

	if (copy_from_user(&arg,
			   (void  __user *)user_arg,
			   sizeof(struct hqmv2_set_cos_bw_args))) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Invalid ioctl argument pointer\n",
			  __func__);
		return -EFAULT;
	}

	response.status = 0;

	mutex_lock(&dev->resource_mutex);

	/* See hqmv2_reset_notify() for more details */
	if (READ_ONCE(dev->unexpected_reset)) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] The HQM was unexpectedly reset.\n",
			  __func__);
		mutex_unlock(&dev->resource_mutex);
		return -EINVAL;
	}

	/* Only the PF can modify the CoS reservations */
	if (dev->type == HQMV2_PF)
		ret = hqmv2_hw_set_cos_bandwidth(&dev->hw,
						 arg.cos_id,
						 arg.bandwidth);
	else
		ret = -EPERM;

	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(struct hqmv2_cmd_response))) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Invalid ioctl response pointer\n",
			  __func__);
		return -EFAULT;
	}

	HQMV2_INFO(dev->hqmv2_device, "Exiting %s()\n", __func__);

	return ret;
}

int hqmv2_ioctl_get_cos_bw(struct hqmv2_dev *dev, unsigned long user_arg)
{
	struct hqmv2_cmd_response response;
	struct hqmv2_get_cos_bw_args arg;
	int ret;

	HQMV2_INFO(dev->hqmv2_device, "Entering %s()\n", __func__);

	if (copy_from_user(&arg,
			   (void  __user *)user_arg,
			   sizeof(struct hqmv2_get_cos_bw_args))) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Invalid ioctl argument pointer\n",
			  __func__);
		return -EFAULT;
	}

	response.status = 0;

	mutex_lock(&dev->resource_mutex);

	/* See hqmv2_reset_notify() for more details */
	if (READ_ONCE(dev->unexpected_reset)) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] The HQM was unexpectedly reset.\n",
			  __func__);
		mutex_unlock(&dev->resource_mutex);
		return -EINVAL;
	}

	ret = dev->ops->get_cos_bw(&dev->hw, arg.cos_id);

	response.id = ret;

	ret = (ret > 0) ? 0 : ret;

	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(struct hqmv2_cmd_response))) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Invalid ioctl response pointer\n",
			  __func__);
		return -EFAULT;
	}

	HQMV2_INFO(dev->hqmv2_device, "Exiting %s()\n", __func__);

	return ret;
}

static hqmv2_ioctl_callback_fn_t hqmv2_ioctl_callback_fns[NUM_HQMV2_CMD] = {
	hqmv2_ioctl_get_device_version,
	hqmv2_ioctl_create_sched_domain,
	hqmv2_ioctl_get_num_resources,
	hqmv2_ioctl_get_driver_version,
	hqmv2_ioctl_sample_perf_counters,
	hqmv2_ioctl_set_sn_allocation,
	hqmv2_ioctl_get_sn_allocation,
	hqmv2_ioctl_set_cos_bw,
	hqmv2_ioctl_get_cos_bw,
};

int hqmv2_ioctl_dispatcher(struct hqmv2_dev *dev,
			   unsigned int cmd,
			   unsigned long arg)
{
	if (_IOC_NR(cmd) >= NUM_HQMV2_CMD) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] Unexpected HQM command %d\n",
			  __func__, _IOC_NR(cmd));
		return -1;
	}

	return hqmv2_ioctl_callback_fns[_IOC_NR(cmd)](dev, arg);
}
