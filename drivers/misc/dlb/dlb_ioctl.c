// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2017-2020 Intel Corporation */

#include <uapi/linux/dlb_user.h>

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

static dlb_domain_ioctl_callback_fn_t
dlb_domain_ioctl_callback_fns[NUM_DLB_DOMAIN_CMD] = {
	dlb_domain_ioctl_create_ldb_pool,
	dlb_domain_ioctl_create_dir_pool,
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

typedef int (*dlb_ioctl_callback_fn_t)(struct dlb_dev *dev, unsigned long arg);

static dlb_ioctl_callback_fn_t dlb_ioctl_callback_fns[NUM_DLB_CMD] = {
	dlb_ioctl_get_device_version,
	dlb_ioctl_create_sched_domain,
	dlb_ioctl_get_num_resources,
	dlb_ioctl_get_driver_version,
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
