// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2017-2020 Intel Corporation */

#include <linux/pci.h>
#include <linux/pci-ats.h>
#include <linux/pm_runtime.h>

#include "base/dlb2_resource.h"
#include "dlb2_main.h"
#include "dlb2_sriov.h"

static int dlb2_pci_sriov_enable(struct pci_dev *pdev, int num_vfs)
{
	struct dlb2_dev *dlb2_dev = pci_get_drvdata(pdev);
	int ret, i;

	mutex_lock(&dlb2_dev->resource_mutex);

	if (dlb2_hw_get_virt_mode(&dlb2_dev->hw) == DLB2_VIRT_SIOV) {
		dev_err(&pdev->dev,
			"dlb2 driver supports either SR-IOV or S-IOV, not both.\n");
		mutex_unlock(&dlb2_dev->resource_mutex);
		return -EINVAL;
	}

	dlb2_hw_set_virt_mode(&dlb2_dev->hw, DLB2_VIRT_SRIOV);

	mutex_unlock(&dlb2_dev->resource_mutex);

	/* Increment the device's usage count and immediately wake it if it was
	 * suspended, assuming pci_enable_sriov() is successful.
	 */
	pm_runtime_get_sync(&pdev->dev);

	ret = pci_enable_sriov(pdev, num_vfs);
	if (ret) {
		pm_runtime_put_sync_suspend(&pdev->dev);
		dlb2_hw_set_virt_mode(&dlb2_dev->hw, DLB2_VIRT_NONE);
		return ret;
	}

	/* Create sysfs files for the newly created VFs */
	for (i = 0; i < num_vfs; i++) {
		ret = sysfs_create_group(&pdev->dev.kobj, dlb2_vf_attrs[i]);
		if (ret) {
			dev_err(&pdev->dev,
				"Internal error: failed to create VF sysfs attr groups.\n");
			pci_disable_sriov(pdev);
			pm_runtime_put_sync_suspend(&pdev->dev);
			dlb2_hw_set_virt_mode(&dlb2_dev->hw, DLB2_VIRT_NONE);
			return ret;
		}
	}

	return num_vfs;
}

static int dlb2_pci_sriov_disable(struct pci_dev *pdev, int num_vfs)
{
	struct dlb2_dev *dlb2_dev = pci_get_drvdata(pdev);
	int i;

	mutex_lock(&dlb2_dev->resource_mutex);

	/* pci_vfs_assigned() checks for VM-owned VFs, but doesn't catch
	 * application-owned VFs in the host -- dlb2_host_vdevs_in_use()
	 * detects that.
	 */
	if (pci_vfs_assigned(pdev) || dlb2_host_vdevs_in_use(dlb2_dev)) {
		dev_err(&pdev->dev,
			"Unable to disable VFs because one or more are in use.\n");
		mutex_unlock(&dlb2_dev->resource_mutex);
		return -EINVAL;
	}

	for (i = 0; i < pci_num_vf(pdev); i++) {
		/* If the VF driver didn't exit cleanly, its resources will
		 * still be locked.
		 */
		dlb2_unlock_vdev(&dlb2_dev->hw, i);

		dlb2_reset_vdev_resources(&dlb2_dev->hw, i);

		/* Remove sysfs files for the VFs */
		sysfs_remove_group(&pdev->dev.kobj, dlb2_vf_attrs[i]);
	}

	pci_disable_sriov(pdev);

	dlb2_hw_set_virt_mode(&dlb2_dev->hw, DLB2_VIRT_NONE);

	mutex_unlock(&dlb2_dev->resource_mutex);

	/* Decrement the device's usage count and suspend it if the
	 * count reaches zero.
	 */
	pm_runtime_put_sync_suspend(&pdev->dev);

	return 0;
}

int dlb2_pci_sriov_configure(struct pci_dev *pdev, int num_vfs)
{
	if (num_vfs)
		return dlb2_pci_sriov_enable(pdev, num_vfs);
	else
		return dlb2_pci_sriov_disable(pdev, num_vfs);
}
