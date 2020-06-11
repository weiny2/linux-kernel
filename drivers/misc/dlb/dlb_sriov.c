// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2017-2020 Intel Corporation */

#include <linux/pci.h>
#include <linux/pm_runtime.h>

#include "dlb_main.h"
#include "dlb_resource.h"
#include "dlb_sriov.h"

static int dlb_pci_sriov_enable(struct pci_dev *pdev, int num_vfs)
{
	struct dlb_dev *dlb_dev = pci_get_drvdata(pdev);
	int ret;
	int i;

	/* Increment the device's usage count and immediately wake it if it was
	 * suspended, assuming pci_enable_sriov() is successful.
	 */
	pm_runtime_get_sync(&pdev->dev);

	ret = pci_enable_sriov(pdev, num_vfs);
	if (ret) {
		pm_runtime_put_sync_suspend(&pdev->dev);
		return ret;
	}

	/* Create sysfs files for the newly created VFs */
	for (i = 0; i < num_vfs; i++) {
		ret = sysfs_create_group(&pdev->dev.kobj, dlb_vf_attrs[i]);
		if (ret) {
			dev_err(&pdev->dev,
				"Internal error: failed to create VF sysfs attr groups.\n");
			pci_disable_sriov(pdev);
			pm_runtime_put_sync_suspend(&pdev->dev);
			return ret;
		}
	}

	mutex_lock(&dlb_dev->resource_mutex);

	if (dlb_dev->revision < DLB_REV_B0) {
		for (i = 0; i < num_vfs; i++)
			dlb_set_vf_reset_in_progress(&dlb_dev->hw, i);
	}

	mutex_unlock(&dlb_dev->resource_mutex);

	return num_vfs;
}

/* Returns the number of host-owned VFs in use. */
static int dlb_host_vfs_in_use(struct dlb_dev *pf_dev)
{
	struct dlb_dev *dev;
	int num = 0;

	mutex_lock(&dlb_driver_lock);

	list_for_each_entry(dev, &dlb_dev_list, list) {
		if (dev->type == DLB_VF && dlb_in_use(dev))
			num++;
	}

	mutex_unlock(&dlb_driver_lock);

	return num;
}

static int dlb_pci_sriov_disable(struct pci_dev *pdev, int num_vfs)
{
	struct dlb_dev *dlb_dev = pci_get_drvdata(pdev);
	int i;

	mutex_lock(&dlb_dev->resource_mutex);

	/* pci_vfs_assigned() checks for VM-owned VFs, but doesn't catch
	 * application-owned VFs in the host -- dlb_host_vfs_in_use() detects
	 * that.
	 */
	if (pci_vfs_assigned(pdev) || dlb_host_vfs_in_use(dlb_dev)) {
		dev_err(&pdev->dev,
			"Unable to disable VFs because one or more are in use.\n");
		mutex_unlock(&dlb_dev->resource_mutex);
		return -EINVAL;
	}

	for (i = 0; i < pci_num_vf(pdev); i++) {
		/* If the VF driver didn't exit cleanly, its resources will
		 * still be locked.
		 */
		dlb_unlock_vf(&dlb_dev->hw, i);

		if (dlb_reset_vf_resources(&dlb_dev->hw, i))
			dev_err(&pdev->dev,
				"[%s()] Internal error: failed to reset VF resources\n",
				__func__);

		/* Remove sysfs files for the VFs */
		sysfs_remove_group(&pdev->dev.kobj, dlb_vf_attrs[i]);

		if (dlb_dev->revision < DLB_REV_B0)
			dlb_clr_vf_reset_in_progress(&dlb_dev->hw, i);
	}

	pci_disable_sriov(pdev);

	mutex_unlock(&dlb_dev->resource_mutex);

	/* Decrement the device's usage count and suspend it if the
	 * count reaches zero.
	 */
	pm_runtime_put_sync_suspend(&pdev->dev);

	return 0;
}

int dlb_pci_sriov_configure(struct pci_dev *pdev, int num_vfs)
{
	if (num_vfs)
		return dlb_pci_sriov_enable(pdev, num_vfs);
	else
		return dlb_pci_sriov_disable(pdev, num_vfs);
}
