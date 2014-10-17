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

#ifndef _GEN_NVDIMM_H
#define _GEN_NVDIMM_H

#include <linux/nvdimm_core.h>
#include <linux/nvdimm_ioctl.h>
#include <linux/nvdimm_acpi.h>

/*****************************************************************************
 * NVM DIMM FUNCTIONS
 ****************************************************************************/
int nvm_register_nvdimm_driver(struct nvdimm_driver *driver);

void nvm_unregister_nvdimm_driver(struct nvdimm_driver *driver);

int nvm_dimm_list_empty(struct pmem_dev *dev);

int nvm_dimm_list_size(struct pmem_dev *dev);

struct nvdimm *nvm_initialize_dimm(struct fit_header *fit_head,
	__u16 pid);

void nvm_initialize_dimm_inventory(struct pmem_dev *dev);

void nvm_remove_dimm_inventory(struct pmem_dev *dev);

int nvm_insert_dimm(struct nvdimm *dimm, struct pmem_dev *dev);

int nvm_remove_dimm(struct nvdimm *dimm);

struct nvdimm *get_nvdimm_by_pid(__u16 physical_id, struct pmem_dev *dev);

int nvm_get_dimm_topology(int array_size,
	struct nvdimm_topology *dimm_topo_array,
	struct pmem_dev *dev);

#endif /* _GEN_NVDIMM_H */
