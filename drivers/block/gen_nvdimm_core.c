/*
 * Copyright 2013 Intel Corporation All Rights Reserved.
 *
 * INTEL CONFIDENTIAL
 *
 * The source code contained or described herein and all documents related to
 * the source code ("Material") are owned by Intel Corporation or its suppliers
 * or licensors. Title to the Material remains with Intel Corporation or its
 * suppliers and licensors. The Material may contain trade secrets and
 * proprietary and confidential information of Intel Corporation and its
 * suppliers and licensors, and is protected by worldwide copyright and trade
 * secret laws and treaty provisions. No part of the Material may be used,
 * copied, reproduced, modified, published, uploaded, posted, transmitted,
 * distributed, or disclosed in any way without Intel's prior express written
 * permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be
 * express and approved by Intel in writing.
 *
 * Unless otherwise agreed by Intel in writing, you may not remove or alter
 * this notice or any other notice embedded in Materials by Intel or Intel's
 * suppliers or licensors in any way.
 */

#include <linux/nvdimm_core.h>
#include <linux/nvdimm_ioctl.h>
#include <linux/gen_nvdimm.h>

/**
 * nvm_dimm_list_empty() - NVDIMM List Empty
 * @pmem_dev: The pmem super structure
 *
 * Calculate if the nvdimm list is empty
 *
 * Returns:
 * 1: Empty
 * 0: Not Empty
 */
int nvm_dimm_list_empty(struct pmem_dev *dev)
{
	int ret;

	spin_lock(&dev->nvdimm_lock);
	ret = list_empty(&dev->nvdimms);
	spin_unlock(&dev->nvdimm_lock);

	return ret;
}

/**
 * nvm_dimm_list_size() - NVDIMM list size
 * @pmem_dev: pmem super structure
 *
 * Calculate the number of NVDIMMs currently in the NVDIMM list
 *
 * Returns:
 * Number of NVDIMMs in the NVDIMM list
 */
int nvm_dimm_list_size(struct pmem_dev *dev)
{
	int count = 0;
	struct nvdimm *pos;

	spin_lock(&dev->nvdimm_lock);
	list_for_each_entry(pos, &dev->nvdimms, dimm_node)
		count++;

	spin_unlock(&dev->nvdimm_lock);

	return count;
}

/**
 * get_nvdimm_by_pid() - Get NVDIMM by Physical ID
 * @physical_id: The SMBIOS Type 17 handle of the NVDIMM
 * @pmem_dev: The pmem super structure
 * Scan the NVDIMM list for a NVDIMM identified by physical ID
 *
 * Returns:
 * Success - NVDIMM matching the physical ID
 * NULL - Unable to find NVDIMM
 */
struct nvdimm *get_nvdimm_by_pid(__u16 physical_id, struct pmem_dev *dev)
{
	struct nvdimm *cur_dimm = NULL;
	struct nvdimm *target_dimm = NULL;

	spin_lock(&dev->nvdimm_lock);
	list_for_each_entry(cur_dimm, &dev->nvdimms, dimm_node) {
		if (physical_id == cur_dimm->physical_id) {
			target_dimm = cur_dimm;
			break;
		}
	}
	spin_unlock(&dev->nvdimm_lock);

	return target_dimm;
}

int nvm_get_dimm_topology(int array_size,
	struct nvdimm_topology *dimm_topo_array,
	struct pmem_dev *dev)
{
	struct nvdimm *cur_dimm;
	int index = 0;

	if (array_size != nvm_dimm_list_size(dev))
		return -EINVAL;

	list_for_each_entry(cur_dimm, &dev->nvdimms, dimm_node) {
		dimm_topo_array[index].id = cur_dimm->physical_id;
		dimm_topo_array[index].vendor_id = cur_dimm->ids.vendor_id;
		dimm_topo_array[index].device_id = cur_dimm->ids.device_id;
		dimm_topo_array[index].revision_id = cur_dimm->ids.rid;
		dimm_topo_array[index].fmt_interface_code =
			cur_dimm->ids.fmt_interface_code;
		dimm_topo_array[index].proximity_domain = cur_dimm->socket_id;
		dimm_topo_array[index].memory_controller_id = cur_dimm->imc_id;
		dimm_topo_array[index].health = cur_dimm->health;

		index++;
	}

	return index;
}

/**
 * nvm_initialize_dimm() - Create and Initialize a new NVDIMM
 * @fit_head: ACPI FIT
 * @pid: SMBIOS Physical ID of the DIMM to create
 *
 * Returns:
 * Success: New NVDIMM structure
 * ENOMEM: Out of memory
 * ENODEV: Trying to Initialize a DIMM that does not exist in the FIT
 * Vendor specific error codes may be returned as well
 */
struct nvdimm *nvm_initialize_dimm(struct fit_header *fit_head,
	__u16 pid)
{
	struct nvdimm *dimm;
	struct memdev_spa_rng_tbl *memdev_tbl;
	struct nvdimm_driver *driver;
	void *ret = NULL;
	int err;

	dimm = kzalloc(sizeof(*dimm), GFP_KERNEL);
	if (!dimm) {
		ret = ERR_PTR(-ENOMEM);
		goto out;
	}

	memdev_tbl = __get_memdev_spa_tbl(fit_head, pid);

	if (!memdev_tbl) {
		ret = ERR_PTR(-ENODEV);
		goto after_dimm;
	}

	dimm->physical_id = pid;
	dimm->imc_id = memdev_tbl->mem_ctrl_id;
	dimm->socket_id = memdev_tbl->socket_id;
	dimm->ids.vendor_id = memdev_tbl->vendor_id;
	dimm->ids.device_id = memdev_tbl->device_id;
	dimm->ids.rid = memdev_tbl->rid;
	dimm->ids.fmt_interface_code = memdev_tbl->fmt_interface_code;

	err = get_dmi_memdev(dimm);
	/* TODO: remove comment when SMBIOS contains AEP DIMMS */
	/*if (err) {
		ret = ERR_PTR(err);
		goto after_dimm;
	}*/

	driver = find_nvdimm_driver(&dimm->ids);

	if (driver) {
		dimm->dimm_ops = driver->dimm_ops;
		dimm->ioctl_ops = driver->ioctl_ops;

		if (!dimm->dimm_ops)
			NVDIMM_DBG("NVDIMM %#x unsupported", dimm->physical_id);

		if (dimm->dimm_ops && dimm->dimm_ops->initialize_dimm) {
			err = dimm->dimm_ops->initialize_dimm(dimm, fit_head);
			if (err) {
				ret = ERR_PTR(err);
				goto after_dimm;
			}
		}
	}
	return dimm;

after_dimm:
	kfree(dimm);
out:
	return ret;
}

/**
 * nvm_remove_dimm_inventory() - Remove the entire NVDIMM inventory
 * @pmem_dev: The pmem super structure
 *
 * Remove the entire NVDIMM inventory,
 * assumes all pools and volumes have been destroyed
 */
void nvm_remove_dimm_inventory(struct pmem_dev *dev)
{
	struct nvdimm *cur_dimm, *tmp_dimm;
	int ret;

	list_for_each_entry_safe(cur_dimm, tmp_dimm, &dev->nvdimms, dimm_node) {

		spin_lock(&dev->nvdimm_lock);
		list_del(&cur_dimm->dimm_node);
		spin_unlock(&dev->nvdimm_lock);

		ret = nvm_remove_dimm(cur_dimm);
		/* TODO: What happens if an NVDIMM is unable to be removed? */
		if (ret)
			NVDIMM_INFO("Unable to remove NVDIMM %#x Error: %d",
				cur_dimm->physical_id, ret);
	}
}

/**
 * nvm_initialize_dimm_inventory() - Creates the NVM DIMM inventory
 * @pmem_dev: The pmem super structure
 *
 * Using the Firmware Interface Table, create an in memory representation
 * of each NVDIMM. For each unique NVDIMM call the initialization function
 * unique to the type of DIMM. As each NVDIMM is fully initialized add it to
 * the in memory list of DIMMs
 */
void nvm_initialize_dimm_inventory(struct pmem_dev *dev)
{
	struct fit_header *fit_head = dev->fit_head;
	struct memdev_spa_rng_tbl *memdev_tbls = fit_head->memdev_spa_rng_tbls;
	struct nvdimm *tmp_dimm;
	int i;

	for (i = 0; i < fit_head->fit->num_memdev_spa_range_tbl; i++) {
		if (!get_nvdimm_by_pid(memdev_tbls[i].mem_dev_pid, dev)) {
			tmp_dimm = nvm_initialize_dimm(fit_head,
				memdev_tbls[i].mem_dev_pid);
			if (!(tmp_dimm) || !IS_ERR(tmp_dimm)) {
				if (nvm_insert_dimm(tmp_dimm, dev))
					kfree(tmp_dimm);
			} else {
				NVDIMM_WARN(
				"Unable to Insert NVDIMM %#x Error: %ld",
					memdev_tbls[i].mem_dev_pid,
					PTR_ERR(tmp_dimm));
			}
		}
	}
}

/**
 * nvm_insert_dimm() - Insert a NVM DIMM into a list of DIMMs
 * @dimm: Fully initialized NVM DIMM
 * @pmem_dev: The pmem super structure
 *
 * Wrapper for adding a NVM DIMM to the global list of DIMMs kept in the
 * generic driver.
 *
 * Three types of inserting a DIMM into the list of DIMMs,
 * 1) Initialization, DIMMs get added to list but no pools or volumes are
 * updated
 * 2) DIMM hot plugged into the system
 *
 * nvm_insert_dimm currently only covers the first type of insertion
 * TODO: Type 2 if it becomes required
 *
 * Returns:
 *	0: Success
 *	-EEXIST: DIMM already exists in list
 */
int nvm_insert_dimm(struct nvdimm *dimm, struct pmem_dev *dev)
{
	if (get_nvdimm_by_pid(dimm->physical_id, dev))
		return -EEXIST;

	spin_lock(&dev->nvdimm_lock);
	list_add_tail(&dimm->dimm_node, &dev->nvdimms);
	spin_unlock(&dev->nvdimm_lock);

	return 0;
}

/**
 * nvm_remove_dimm() - Remove a NVM DIMM from a list of DIMMs
 * @dimm_id: SMBIOS Type 17 handle corresponding to the DIMM for removal
 *
 * De-allocates all resources used by NVDIMM, this includes any NVDIMM
 *  type specific resources.
 *
 * Assumes NVDIMM is safe for removal
 *
 * Return: Error Code?
 */
int nvm_remove_dimm(struct nvdimm *dimm)
{
	int ret;

	if (dimm->dimm_ops && dimm->dimm_ops->remove_dimm) {
		ret = dimm->dimm_ops->remove_dimm(dimm);
		if (ret)
			return ret;
	}

	kfree(dimm);

	return 0;
}
