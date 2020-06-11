// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2017-2020 Intel Corporation */

#include <uapi/linux/dlb_user.h>

#include "dlb_intr.h"
#include "dlb_ioctl.h"
#include "dlb_main.h"

typedef int (*dlb_domain_ioctl_callback_fn_t)(struct dlb_dev *dev,
					      struct dlb_status *status,
					      unsigned long arg,
					      u32 domain_id);

/* The DLB domain ioctl callback template minimizes replication of boilerplate
 * code to copy arguments, acquire and release the resource lock, and execute
 * the command.  The arguments and response structure name should have the
 * format dlb_<lower_name>_args.
 */
#define DLB_DOMAIN_IOCTL_CALLBACK_TEMPLATE(lower_name)		     \
static int dlb_domain_ioctl_##lower_name(struct dlb_dev *dev,	     \
					 struct dlb_status *status,  \
					 unsigned long user_arg,     \
					 u32 domain_id)		     \
{								     \
	struct dlb_##lower_name##_args arg;			     \
	struct dlb_cmd_response response = {0};			     \
	int ret;						     \
	response.status = 0;					     \
								     \
	dev_dbg(dev->dlb_device, "Entering %s()\n", __func__);	     \
								     \
	if (copy_from_user(&arg,				     \
			   (void  __user *)user_arg,		     \
			   sizeof(arg))) {			     \
		dev_err(dev->dlb_device,			     \
			"[%s()] Invalid ioctl argument pointer\n",   \
			__func__);				     \
		return -EFAULT;					     \
	}							     \
								     \
	mutex_lock(&dev->resource_mutex);			     \
								     \
	ret = dev->ops->lower_name(&dev->hw,			     \
				  domain_id,			     \
				  &arg,				     \
				  &response);			     \
								     \
	mutex_unlock(&dev->resource_mutex);			     \
								     \
	if (copy_to_user((void __user *)arg.response,		     \
			 &response,				     \
			 sizeof(response))) {			     \
		dev_err(dev->dlb_device,			     \
			"[%s()] Invalid ioctl response pointer\n",   \
			__func__);				     \
		return -EFAULT;					     \
	}							     \
								     \
	dev_dbg(dev->dlb_device, "Exiting %s()\n", __func__);	     \
								     \
	return ret;						     \
}

DLB_DOMAIN_IOCTL_CALLBACK_TEMPLATE(create_ldb_pool)
DLB_DOMAIN_IOCTL_CALLBACK_TEMPLATE(create_dir_pool)
DLB_DOMAIN_IOCTL_CALLBACK_TEMPLATE(create_ldb_queue)
DLB_DOMAIN_IOCTL_CALLBACK_TEMPLATE(create_dir_queue)
DLB_DOMAIN_IOCTL_CALLBACK_TEMPLATE(start_domain)
DLB_DOMAIN_IOCTL_CALLBACK_TEMPLATE(map_qid)
DLB_DOMAIN_IOCTL_CALLBACK_TEMPLATE(unmap_qid)
DLB_DOMAIN_IOCTL_CALLBACK_TEMPLATE(get_ldb_queue_depth)
DLB_DOMAIN_IOCTL_CALLBACK_TEMPLATE(get_dir_queue_depth)
DLB_DOMAIN_IOCTL_CALLBACK_TEMPLATE(pending_port_unmaps)

static int dlb_domain_ioctl_create_ldb_port(struct dlb_dev *dev,
					    struct dlb_status *status,
					    unsigned long user_arg,
					    u32 domain_id)
{
	struct dlb_create_ldb_port_args arg;
	struct dlb_cmd_response response;
	struct dlb_domain_dev *domain;
	dma_addr_t pc_dma_base = 0;
	dma_addr_t cq_dma_base = 0;
	void *pc_base = NULL;
	void *cq_base = NULL;
	int ret;

	dev_dbg(dev->dlb_device, "Entering %s()\n", __func__);

	response.status = 0;

	if (copy_from_user(&arg, (void __user *)user_arg, sizeof(arg))) {
		dev_err(dev->dlb_device,
			"[%s()] Invalid ioctl argument pointer\n", __func__);
		return -EFAULT;
	}

	mutex_lock(&dev->resource_mutex);

	if (domain_id >= DLB_MAX_NUM_DOMAINS) {
		response.status = DLB_ST_INVALID_DOMAIN_ID;
		ret = -EPERM;
		goto unlock;
	}

	domain = &dev->sched_domains[domain_id];

	cq_base = dma_alloc_coherent(&dev->pdev->dev,
				     DLB_LDB_CQ_MAX_SIZE,
				     &cq_dma_base,
				     GFP_KERNEL);
	if (!cq_base) {
		response.status = DLB_ST_NO_MEMORY;
		ret = -ENOMEM;
		goto unlock;
	}

	pc_base = dma_alloc_coherent(&dev->pdev->dev,
				     PAGE_SIZE,
				     &pc_dma_base,
				     GFP_KERNEL);
	if (!pc_base) {
		response.status = DLB_ST_NO_MEMORY;
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

	/* DLB A-stepping workaround for hardware write buffer lock up issue:
	 * limit the maximum configured ports to < 128 and disable CQ occupancy
	 * interrupts.
	 */
	if (dev->revision >= DLB_REV_B0) {
		u16 threshold = arg.cq_depth_threshold;

		ret = dev->ops->enable_ldb_cq_interrupts(dev,
							 response.id,
							 threshold);
		if (ret)
			/* Internal error, don't unwind port creation */
			goto unlock;
	}

	/* Fill out the per-port memory tracking structure */
	dev->ldb_port_mem[response.id].domain_id = domain_id;
	dev->ldb_port_mem[response.id].cq_base = cq_base;
	dev->ldb_port_mem[response.id].pc_base = pc_base;
	dev->ldb_port_mem[response.id].cq_dma_base = cq_dma_base;
	dev->ldb_port_mem[response.id].pc_dma_base = pc_dma_base;
	dev->ldb_port_mem[response.id].valid = true;

unlock:
	if (ret) {
		dev_err(dev->dlb_device, "[%s()]: Error %s\n",
			__func__, dlb_error_strings[response.status]);

		if (cq_dma_base)
			dma_free_coherent(&dev->pdev->dev,
					  DLB_LDB_CQ_MAX_SIZE,
					  cq_base,
					  cq_dma_base);
		if (pc_dma_base)
			dma_free_coherent(&dev->pdev->dev,
					  PAGE_SIZE,
					  pc_base,
					  pc_dma_base);
	} else {
		dev_dbg(dev->dlb_device, "CQ PA: 0x%llx\n",
			virt_to_phys(cq_base));
		dev_dbg(dev->dlb_device, "CQ IOVA: 0x%llx\n", cq_dma_base);
	}

	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(response))) {
		dev_err(dev->dlb_device,
			"[%s()] Invalid ioctl response pointer\n", __func__);
		return -EFAULT;
	}

	dev_dbg(dev->dlb_device, "Exiting %s()\n", __func__);

	return ret;
}

static int dlb_domain_ioctl_create_dir_port(struct dlb_dev *dev,
					    struct dlb_status *status,
					    unsigned long user_arg,
					    u32 domain_id)
{
	struct dlb_create_dir_port_args arg;
	struct dlb_cmd_response response;
	struct dlb_domain_dev *domain;
	dma_addr_t pc_dma_base = 0;
	dma_addr_t cq_dma_base = 0;
	void *pc_base = NULL;
	void *cq_base = NULL;
	int ret;

	dev_dbg(dev->dlb_device, "Entering %s()\n", __func__);

	response.status = 0;

	if (copy_from_user(&arg, (void __user *)user_arg, sizeof(arg))) {
		dev_err(dev->dlb_device,
			"[%s()] Invalid ioctl argument pointer\n", __func__);
		return -EFAULT;
	}

	mutex_lock(&dev->resource_mutex);

	if (domain_id >= DLB_MAX_NUM_DOMAINS) {
		response.status = DLB_ST_INVALID_DOMAIN_ID;
		ret = -EPERM;
		goto unlock;
	}

	domain = &dev->sched_domains[domain_id];

	cq_base = dma_alloc_coherent(&dev->pdev->dev,
				     DLB_DIR_CQ_MAX_SIZE,
				     &cq_dma_base,
				     GFP_KERNEL);
	if (!cq_base) {
		response.status = DLB_ST_NO_MEMORY;
		ret = -ENOMEM;
		goto unlock;
	}

	pc_base = dma_alloc_coherent(&dev->pdev->dev,
				     PAGE_SIZE,
				     &pc_dma_base,
				     GFP_KERNEL);
	if (!pc_base) {
		response.status = DLB_ST_NO_MEMORY;
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

	/* DLB A-stepping workaround for hardware write buffer lock up issue:
	 * limit the maximum configured ports to < 128 and disable CQ occupancy
	 * interrupts.
	 */
	if (dev->revision >= DLB_REV_B0) {
		u16 threshold = arg.cq_depth_threshold;

		ret = dev->ops->enable_dir_cq_interrupts(dev,
							 response.id,
							 threshold);
		if (ret)
			/* Internal error, don't unwind port creation */
			goto unlock;
	}

	/* Fill out the per-port memory tracking structure */
	dev->dir_port_mem[response.id].domain_id = domain_id;
	dev->dir_port_mem[response.id].cq_base = cq_base;
	dev->dir_port_mem[response.id].pc_base = pc_base;
	dev->dir_port_mem[response.id].cq_dma_base = cq_dma_base;
	dev->dir_port_mem[response.id].pc_dma_base = pc_dma_base;
	dev->dir_port_mem[response.id].valid = true;

unlock:
	if (ret) {
		dev_err(dev->dlb_device, "[%s()]: Error %s\n",
			__func__, dlb_error_strings[response.status]);

		if (cq_dma_base)
			dma_free_coherent(&dev->pdev->dev,
					  DLB_DIR_CQ_MAX_SIZE,
					  cq_base,
					  cq_dma_base);
		if (pc_dma_base)
			dma_free_coherent(&dev->pdev->dev,
					  PAGE_SIZE,
					  pc_base,
					  pc_dma_base);
	} else {
		dev_dbg(dev->dlb_device, "CQ PA: 0x%llx\n",
			virt_to_phys(cq_base));
		dev_dbg(dev->dlb_device, "CQ IOVA: 0x%llx\n", cq_dma_base);
	}

	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(response))) {
		dev_err(dev->dlb_device,
			"[%s()] Invalid ioctl response pointer\n", __func__);
		return -EFAULT;
	}

	dev_dbg(dev->dlb_device, "Exiting %s()\n", __func__);

	return ret;
}

static int dlb_domain_ioctl_enable_ldb_port(struct dlb_dev *dev,
					    struct dlb_status *status,
					    unsigned long user_arg,
					    u32 domain_id)
{
	struct dlb_enable_ldb_port_args arg;
	struct dlb_cmd_response response = {0};
	int ret;

	response.status = 0;

	dev_dbg(dev->dlb_device, "Entering %s()\n", __func__);

	if (copy_from_user(&arg, (void __user *)user_arg, sizeof(arg))) {
		dev_err(dev->dlb_device,
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
			 sizeof(response))) {
		dev_err(dev->dlb_device,
			"[%s()] Invalid ioctl response pointer\n",
			__func__);
		return -EFAULT;
	}

	dev_dbg(dev->dlb_device, "Exiting %s()\n", __func__);

	return ret;
}

static int dlb_domain_ioctl_enable_dir_port(struct dlb_dev *dev,
					    struct dlb_status *status,
					    unsigned long user_arg,
					    u32 domain_id)
{
	struct dlb_enable_dir_port_args arg;
	struct dlb_cmd_response response = {0};
	int ret;

	response.status = 0;

	dev_dbg(dev->dlb_device, "Entering %s()\n", __func__);

	if (copy_from_user(&arg, (void __user *)user_arg, sizeof(arg))) {
		dev_err(dev->dlb_device,
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
			 sizeof(response))) {
		dev_err(dev->dlb_device,
			"[%s()] Invalid ioctl response pointer\n",
			__func__);
		return -EFAULT;
	}

	dev_dbg(dev->dlb_device, "Exiting %s()\n", __func__);

	return ret;
}

static int dlb_domain_ioctl_disable_ldb_port(struct dlb_dev *dev,
					     struct dlb_status *status,
					     unsigned long user_arg,
					     u32 domain_id)
{
	struct dlb_disable_ldb_port_args arg;
	struct dlb_cmd_response response = {0};
	int ret;

	response.status = 0;

	dev_dbg(dev->dlb_device, "Entering %s()\n", __func__);

	if (copy_from_user(&arg, (void __user *)user_arg, sizeof(arg))) {
		dev_err(dev->dlb_device,
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
		dlb_wake_thread(dev,
				&dev->intr.ldb_cq_intr[arg.port_id],
				WAKE_PORT_DISABLED);

	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(response))) {
		dev_err(dev->dlb_device,
			"[%s()] Invalid ioctl response pointer\n",
			__func__);
		return -EFAULT;
	}

	dev_dbg(dev->dlb_device, "Exiting %s()\n", __func__);

	return ret;
}

static int dlb_domain_ioctl_disable_dir_port(struct dlb_dev *dev,
					     struct dlb_status *status,
					     unsigned long user_arg,
					     u32 domain_id)
{
	struct dlb_disable_dir_port_args arg;
	struct dlb_cmd_response response = {0};
	int ret;

	response.status = 0;

	dev_dbg(dev->dlb_device, "Entering %s()\n", __func__);

	if (copy_from_user(&arg, (void __user *)user_arg, sizeof(arg))) {
		dev_err(dev->dlb_device,
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
		dlb_wake_thread(dev,
				&dev->intr.dir_cq_intr[arg.port_id],
				WAKE_PORT_DISABLED);

	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(response))) {
		dev_err(dev->dlb_device,
			"[%s()] Invalid ioctl response pointer\n",
			__func__);
		return -EFAULT;
	}

	dev_dbg(dev->dlb_device, "Exiting %s()\n", __func__);

	return ret;
}

static int dlb_domain_ioctl_block_on_cq_interrupt(struct dlb_dev *dev,
						  struct dlb_status *status,
						  unsigned long user_arg,
						  u32 domain_id)
{
	struct dlb_block_on_cq_interrupt_args arg;
	struct dlb_cmd_response response;
	int ret = 0;

	response.status = 0;

	dev_dbg(dev->dlb_device, "Entering %s()\n", __func__);

	if (copy_from_user(&arg, (void __user *)user_arg, sizeof(arg))) {
		dev_err(dev->dlb_device,
			"[%s()] Invalid ioctl argument pointer\n", __func__);
		return -EFAULT;
	}

	ret = dlb_block_on_cq_interrupt(dev,
					status,
					domain_id,
					arg.port_id,
					arg.is_ldb,
					arg.cq_va,
					arg.cq_gen,
					arg.arm);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(response))) {
		dev_err(dev->dlb_device,
			"[%s()] Invalid ioctl response pointer\n", __func__);
		return -EFAULT;
	}

	dev_dbg(dev->dlb_device, "Exiting %s()\n", __func__);

	return ret;
}

static int dlb_domain_ioctl_enqueue_domain_alert(struct dlb_dev *dev,
						 struct dlb_status *status,
						 unsigned long user_arg,
						 u32 domain_id)
{
	struct dlb_enqueue_domain_alert_args arg;

	dev_dbg(dev->dlb_device, "Entering %s()\n", __func__);

	if (copy_from_user(&arg, (void __user *)user_arg, sizeof(arg))) {
		dev_err(dev->dlb_device,
			"[%s()] Invalid ioctl argument pointer\n", __func__);
		return -EFAULT;
	}

	return dlb_write_domain_alert(dev,
				      domain_id,
				      DLB_DOMAIN_ALERT_USER,
				      arg.aux_alert_data);
}

static dlb_domain_ioctl_callback_fn_t
dlb_domain_ioctl_callback_fns[NUM_DLB_DOMAIN_CMD] = {
	dlb_domain_ioctl_create_ldb_pool,
	dlb_domain_ioctl_create_dir_pool,
	dlb_domain_ioctl_create_ldb_queue,
	dlb_domain_ioctl_create_dir_queue,
	dlb_domain_ioctl_create_ldb_port,
	dlb_domain_ioctl_create_dir_port,
	dlb_domain_ioctl_start_domain,
	dlb_domain_ioctl_map_qid,
	dlb_domain_ioctl_unmap_qid,
	dlb_domain_ioctl_enable_ldb_port,
	dlb_domain_ioctl_enable_dir_port,
	dlb_domain_ioctl_disable_ldb_port,
	dlb_domain_ioctl_disable_dir_port,
	dlb_domain_ioctl_block_on_cq_interrupt,
	dlb_domain_ioctl_enqueue_domain_alert,
	dlb_domain_ioctl_get_ldb_queue_depth,
	dlb_domain_ioctl_get_dir_queue_depth,
	dlb_domain_ioctl_pending_port_unmaps,
};

int dlb_domain_ioctl_dispatcher(struct dlb_dev *dev,
				struct dlb_status *st,
				unsigned int cmd,
				unsigned long arg,
				u32 id)
{
	if (_IOC_NR(cmd) >= NUM_DLB_DOMAIN_CMD) {
		dev_err(dev->dlb_device,
			"[%s()] Unexpected DLB DOMAIN command %d\n",
		       __func__, _IOC_NR(cmd));
		return -1;
	}

	return dlb_domain_ioctl_callback_fns[_IOC_NR(cmd)](dev, st, arg, id);
}

/* [7:0]: device revision, [15:8]: device version */
#define DLB_SET_DEVICE_VERSION(ver, rev) (((ver) << 8) | (rev))

static int dlb_ioctl_get_device_version(struct dlb_dev *dev,
					unsigned long user_arg)
{
	struct dlb_get_device_version_args arg;
	struct dlb_cmd_response response;

	dev_dbg(dev->dlb_device, "Entering %s()\n", __func__);

	response.status = 0;
	response.id = DLB_SET_DEVICE_VERSION(1, dev->revision);

	if (copy_from_user(&arg, (void __user *)user_arg, sizeof(arg))) {
		dev_err(dev->dlb_device,
			"[%s()] Invalid ioctl argument pointer\n", __func__);
		return -EFAULT;
	}

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(response))) {
		dev_err(dev->dlb_device,
			"[%s()] Invalid ioctl response pointer\n", __func__);
		return -EFAULT;
	}

	dev_dbg(dev->dlb_device, "Exiting %s()\n", __func__);

	return 0;
}

static int dlb_ioctl_create_sched_domain(struct dlb_dev *dev,
					 unsigned long user_arg)
{
	struct dlb_create_sched_domain_args arg;
	struct dlb_cmd_response response;
	int ret;

	response.status = 0;

	dev_dbg(dev->dlb_device, "Entering %s()\n", __func__);

	if (copy_from_user(&arg, (void __user *)user_arg, sizeof(arg))) {
		dev_err(dev->dlb_device,
			"[%s()] Invalid ioctl argument pointer\n", __func__);
		return -EFAULT;
	}

	mutex_lock(&dev->resource_mutex);

	if (dev->domain_reset_failed) {
		response.status = DLB_ST_DOMAIN_RESET_FAILED;
		ret = -EINVAL;
		goto unlock;
	}

	ret = dev->ops->create_sched_domain(&dev->hw, &arg, &response);
	if (ret)
		goto unlock;

	ret = dlb_add_domain_device_file(dev, response.id);
	if (ret)
		goto unlock;

unlock:
	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(response))) {
		dev_err(dev->dlb_device,
			"[%s()] Invalid ioctl response pointer\n", __func__);
		return -EFAULT;
	}

	dev_dbg(dev->dlb_device, "Exiting %s()\n", __func__);

	return ret;
}

static int dlb_ioctl_get_num_resources(struct dlb_dev *dev,
				       unsigned long user_arg)
{
	struct dlb_get_num_resources_args arg;
	int ret;

	dev_dbg(dev->dlb_device, "Entering %s()\n", __func__);

	mutex_lock(&dev->resource_mutex);

	ret = dev->ops->get_num_resources(&dev->hw, &arg);

	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)user_arg,
			 &arg,
			 sizeof(struct dlb_get_num_resources_args))) {
		dev_err(dev->dlb_device, "Invalid DLB resources pointer\n");
		return -EFAULT;
	}

	dev_dbg(dev->dlb_device, "Exiting %s()\n", __func__);

	return ret;
}

static int dlb_ioctl_get_driver_version(struct dlb_dev *dev,
					unsigned long user_arg)
{
	struct dlb_get_driver_version_args arg;
	struct dlb_cmd_response response;

	dev_dbg(dev->dlb_device, "Entering %s()\n", __func__);

	response.status = 0;
	response.id = DLB_VERSION;

	if (copy_from_user(&arg, (void __user *)user_arg, sizeof(arg))) {
		dev_err(dev->dlb_device,
			"[%s()] Invalid ioctl argument pointer\n", __func__);
		return -EFAULT;
	}

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(response))) {
		dev_err(dev->dlb_device,
			"[%s()] Invalid ioctl response pointer\n", __func__);
		return -EFAULT;
	}

	dev_dbg(dev->dlb_device, "Exiting %s()\n", __func__);

	return 0;
}

static int dlb_ioctl_set_sn_allocation(struct dlb_dev *dev,
				       unsigned long user_arg)
{
	struct dlb_set_sn_allocation_args arg;
	struct dlb_cmd_response response;
	int ret;

	dev_dbg(dev->dlb_device, "Entering %s()\n", __func__);

	if (copy_from_user(&arg, (void __user *)user_arg, sizeof(arg))) {
		dev_err(dev->dlb_device,
			"[%s()] Invalid ioctl argument pointer\n",
			__func__);
		return -EFAULT;
	}

	response.status = 0;

	mutex_lock(&dev->resource_mutex);

	ret = dev->ops->set_sn_allocation(&dev->hw, arg.group, arg.num);

	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(response))) {
		dev_err(dev->dlb_device,
			"[%s()] Invalid ioctl response pointer\n",
			__func__);
		return -EFAULT;
	}

	dev_dbg(dev->dlb_device, "Exiting %s()\n", __func__);

	return ret;
}

static int dlb_ioctl_get_sn_allocation(struct dlb_dev *dev,
				       unsigned long user_arg)
{
	struct dlb_get_sn_allocation_args arg;
	struct dlb_cmd_response response;
	int ret;

	dev_dbg(dev->dlb_device, "Entering %s()\n", __func__);

	if (copy_from_user(&arg, (void __user *)user_arg, sizeof(arg))) {
		dev_err(dev->dlb_device,
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
			 sizeof(response))) {
		dev_err(dev->dlb_device,
			"[%s()] Invalid ioctl response pointer\n",
			__func__);
		return -EFAULT;
	}

	dev_dbg(dev->dlb_device, "Exiting %s()\n", __func__);

	return ret;
}

static int dlb_ioctl_query_cq_poll_mode(struct dlb_dev *dev,
					unsigned long user_arg)
{
	struct dlb_query_cq_poll_mode_args arg;
	struct dlb_cmd_response response;
	int ret;

	if (copy_from_user(&arg, (void __user *)user_arg, sizeof(arg))) {
		dev_err(dev->dlb_device,
			"[%s()] Invalid ioctl argument pointer\n",
			__func__);
		return -EFAULT;
	}

	mutex_lock(&dev->resource_mutex);

	ret = dev->ops->query_cq_poll_mode(dev, &response);

	mutex_unlock(&dev->resource_mutex);

	if (copy_to_user((void __user *)arg.response,
			 &response,
			 sizeof(response))) {
		pr_err("Invalid ioctl response pointer\n");
		return -EFAULT;
	}

	return ret;
}

static int dlb_ioctl_get_sn_occupancy(struct dlb_dev *dev,
				      unsigned long user_arg)
{
	struct dlb_get_sn_occupancy_args arg;
	struct dlb_cmd_response response;
	int ret;

	dev_dbg(dev->dlb_device, "Entering %s()\n", __func__);

	if (copy_from_user(&arg, (void __user *)user_arg, sizeof(arg))) {
		dev_err(dev->dlb_device,
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
			 sizeof(response))) {
		dev_err(dev->dlb_device,
			"[%s()] Invalid ioctl response pointer\n",
			__func__);
		return -EFAULT;
	}

	dev_dbg(dev->dlb_device, "Exiting %s()\n", __func__);

	return ret;
}

typedef int (*dlb_ioctl_callback_fn_t)(struct dlb_dev *dev, unsigned long arg);

static dlb_ioctl_callback_fn_t dlb_ioctl_callback_fns[NUM_DLB_CMD] = {
	dlb_ioctl_get_device_version,
	dlb_ioctl_create_sched_domain,
	dlb_ioctl_get_num_resources,
	dlb_ioctl_get_driver_version,
	dlb_ioctl_set_sn_allocation,
	dlb_ioctl_get_sn_allocation,
	dlb_ioctl_query_cq_poll_mode,
	dlb_ioctl_get_sn_occupancy,
};

int dlb_ioctl_dispatcher(struct dlb_dev *dev,
			 unsigned int cmd,
			 unsigned long arg)
{
	if (_IOC_NR(cmd) >= NUM_DLB_CMD) {
		dev_err(dev->dlb_device, "[%s()] Unexpected DLB command %d\n",
			__func__, _IOC_NR(cmd));
		return -1;
	}

	return dlb_ioctl_callback_fns[_IOC_NR(cmd)](dev, arg);
}
