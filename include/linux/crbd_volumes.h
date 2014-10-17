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

#ifndef _CRBD_VOLUMES_H
#define _CRBD_VOLUMES_H

#include <linux/nvdimm_core.h>
#include <linux/gen_nvm_volumes.h>

struct cr_volume {
	bool atomic;
	struct list_head extent_set_node;
};

/****************************************************
 * CR VOLUME FUNCTIONS
 ****************************************************/

/**
 * cr_create_volume() - Create a CR Volume
 * @volume: The generic NVM volume the CR will be a part of
 * @ioctl: The structure that contains all of the CR specific information
 * from the user required to build a CR volume.
 *
 * Create a new user defined volume. Perform all of the creation and
 * initialization duties for a CR volume.
 * Claim extents from pool
 *
 * CR Specific
 *
 * Returns:
 */
int cr_create_volume(struct nvm_volume *volume,
	struct ioctl_create_volume *ioctl);

/**
 * cr_delete_volume() - Delete a CR Volume
 * @volume: The NVM Volume the CR Volume to delete is part of
 *
 * Delete a volume and release all of its resources
 * For CR this includes replacing memory back into the pools
 *
 * CR Specific
 *
 * Returns:
 */
int cr_delete_volume(struct nvm_volume *volume);

/**
 * cr_modify_volume() - Modify a CR Volume
 * @volume: NVM Volume the CR volume is a part of
 * @ioctl: The structure that contains all of the CR specific information
 * from the user required to modify a CR volume
 *
 * Grow|Shrink, modify extents accordingly.
 *
 * CR Specific
 *
 * Returns:
 */
int cr_modify_volume(struct nvm_volume *volume,
	struct ioctl_modify_volume *ioctl);

/**
 * cr_read() - Read data from a CR Volume
 * @volume: NVM Volume to read data from
 * @lba: LBA of the start of the data region to read
 * @nbytes: Number of bytes to read
 * @buffer: Buffer to place read data into
 *
 * Transform LBA into RDPA and DIMM
 * Call the CR DIMM read function
 *
 * CR Specific
 *
 * Returns:
 */
int cr_read(struct nvm_volume *volume, unsigned long lba, unsigned long nbytes,
	char *buffer);

/**
 * cr_write() - Write data to a CR Volume
 * @volume: NVM Volume to read data from
 * @lba: LBA of the start of the data region to read
 * @nbytes: Number of bytes to read
 * @buffer: Buffer to place read data into
 *
 * Perform the operations required to do atomic writes if atomic flag is set
 * Call the CR DIMM write function
 *
 * CR Specific
 *
 * Returns:
 */
int cr_write(struct nvm_volume *volume, unsigned long lba, unsigned long nbytes,
	char *buffer);

/**
 * cr_check_consistency() - Check the consistency of a CR Volume
 * @volume: NVM Volume the CR volume is a part of
 *
 * Perform all actions required to check the consistency of a CR Volume
 *
 * CR Specific
 *
 * Returns:
 */
int cr_check_consistency(struct nvm_volume *volume);

/**
 * cr_get_capacity() - Get the total capacity of a CR Volume
 */
__u64 cr_get_capacity(struct cr_volume *cr_vol);

/**
 * cr_write_labels() - Write Volume Labels out to disk
 */
int cr_write_labels(struct nvm_volume *volume);

/**
 * cr_volume_ioctl() - Handle CR Volume Specific IOCTL commands
 */
int cr_volume_ioctl(struct nvm_volume *volume, fmode_t mode, unsigned cmd,
	unsigned long arg);

#endif /* _CRBD_VOLUMES_H */
