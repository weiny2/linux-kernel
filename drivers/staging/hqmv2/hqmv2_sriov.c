// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2017-2018 Intel Corporation */

#include <linux/pci.h>
#include <linux/pm_runtime.h>
#include <linux/pci-ats.h>

#include "hqmv2_main.h"
#include "base/hqmv2_resource.h"

static int hqmv2_pci_sriov_enable(struct pci_dev *pdev, int num_vfs)
{
	struct hqmv2_dev *hqmv2_dev = pci_get_drvdata(pdev);
	int ret, i;

	mutex_lock(&hqmv2_dev->resource_mutex);

	if (hqmv2_hw_get_virt_mode(&hqmv2_dev->hw) == HQMV2_VIRT_SIOV) {
		HQMV2_ERR(&pdev->dev,
			  "HQM driver supports either SR-IOV or S-IOV, not both.\n");
		mutex_unlock(&hqmv2_dev->resource_mutex);
		return -EINVAL;
	}

	hqmv2_hw_set_virt_mode(&hqmv2_dev->hw, HQMV2_VIRT_SRIOV);

	mutex_unlock(&hqmv2_dev->resource_mutex);

	/* Increment the device's usage count and immediately wake it if it was
	 * suspended, assuming pci_enable_sriov() is successful.
	 */
	pm_runtime_get_sync(&pdev->dev);

	ret = pci_enable_sriov(pdev, num_vfs);
	if (ret) {
		pm_runtime_put_sync_suspend(&pdev->dev);
		hqmv2_hw_set_virt_mode(&hqmv2_dev->hw, HQMV2_VIRT_NONE);
		return ret;
	}

	/* Create sysfs files for the newly created VFs */
	for (i = 0; i < num_vfs; i++) {
		ret = sysfs_create_group(&pdev->dev.kobj, hqmv2_vf_attrs[i]);
		if (ret) {
			HQMV2_ERR(&pdev->dev,
				  "Internal error: failed to create VF sysfs attr groups.\n");
			pci_disable_sriov(pdev);
			pm_runtime_put_sync_suspend(&pdev->dev);
			hqmv2_hw_set_virt_mode(&hqmv2_dev->hw, HQMV2_VIRT_NONE);
			return ret;
		}
	}

	return num_vfs;
}

static int hqmv2_pci_sriov_disable(struct pci_dev *pdev, int num_vfs)
{
	struct hqmv2_dev *hqmv2_dev = pci_get_drvdata(pdev);
	int i;

	mutex_lock(&hqmv2_dev->resource_mutex);

	/* pci_vfs_assigned() checks for VM-owned VFs, but doesn't catch
	 * application-owned VFs in the host -- hqmv2_host_vdevs_in_use()
	 * detects that.
	 */
	if (pci_vfs_assigned(pdev) || hqmv2_host_vdevs_in_use(hqmv2_dev)) {
		HQMV2_ERR(&pdev->dev,
			  "Unable to disable VFs because one or more are in use.\n");
		mutex_unlock(&hqmv2_dev->resource_mutex);
		return -EINVAL;
	}

	for (i = 0; i < pci_num_vf(pdev); i++) {
		/* If the VF driver didn't exit cleanly, its resources will
		 * still be locked.
		 */
		hqmv2_unlock_vdev(&hqmv2_dev->hw, i);

		hqmv2_reset_vdev_resources(&hqmv2_dev->hw, i);

		/* Remove sysfs files for the VFs */
		sysfs_remove_group(&pdev->dev.kobj, hqmv2_vf_attrs[i]);
	}

	pci_disable_sriov(pdev);

	hqmv2_hw_set_virt_mode(&hqmv2_dev->hw, HQMV2_VIRT_NONE);

	mutex_unlock(&hqmv2_dev->resource_mutex);

	/* Decrement the device's usage count and suspend it if the
	 * count reaches zero.
	 */
	pm_runtime_put_sync_suspend(&pdev->dev);

	return 0;
}

int hqmv2_pci_sriov_configure(struct pci_dev *pdev, int num_vfs)
{
	if (num_vfs)
		return hqmv2_pci_sriov_enable(pdev, num_vfs);
	else
		return hqmv2_pci_sriov_disable(pdev, num_vfs);
}
