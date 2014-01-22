/*
 * @internal
 * @copyright
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
 * @endinternal
 */

#ifndef CRBD_VOLUMES_H_
#define CRBD_VOLUMES_H_

#include <os_adapter.h>
#include <linux/nvdimm_core.h>
#include <linux/gen_nvm_volumes.h>

/*CR Specific*/
struct btt_entry {
	__u64 lba;
	__u64 rdpa;
	__u16 dimm_id;
};

/*CR Specific*/
struct cr_block_translation_tbl {
	__u64 num_btt_entries;
	struct btt_entry *btt;
};

/*CR Specific*/
struct cr_freelist {
	/*XXX: TBD*/
};

/*CR Specific*/
struct cr_volume {
	bool atomic;
	struct list_head extent_set_node;
	struct cr_block_translation_tbl btt;
	struct cr_freelist freelist;
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
 * create BTT/Freelist
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
 * Grow|Shrink, modify extents and BTT accordingly.
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
 * Transform LBA into RDPA and DIMM through the BTT.
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
 * Validate extents, check BTT, check freelist
 *
 * CR Specific
 *
 * Returns:
 */
int cr_check_consistency(struct nvm_volume *volume);

/**
 * cr_create_btt() - Create the BTT for a given Volume
 * @cr_vol: CR Volume to create the BTT for
 *
 * Extents should of been allocated to the volume before hand.
 * Create the BTT for the Extents
 *
 * CR Specific
 *
 * Returns:
 */
int cr_create_btt(struct cr_volume *cr_vol);

/**
 * cr_modify_btt() - Modify the size of the BTT
 * @???
 *
 * Grow or shrink the size of the BTT
 */
int cr_modify_btt(void /*???*/);

/**
 * cr_read_btt() - Translate a LBA to a RDPA and retrieve the PTR to the DIMM
 * @cr_vol: CR Volume containing the BTT
 * @lba: LBA to retrieve the rdpa and dimm_ptr for
 * @rdpa: retrieved from btt
 * @dimm_id: retrieved from btt
 *
 * CR_Specific
 *
 * Returns:???
 */
int cr_read_btt(struct cr_volume *cr_vol, __u64 lba, __u64 *rdpa,
	__u16 *dimm_id);

/**
 * cr_write_btt() - Update a LBA with a new RDPA
 * @cr_vol: CR volume containing the BTT
 * @lba: LBA to update
 * @rdpa: new rdpa for the lba
 * @dimm_id: new dimm id for the lba
 *
 * update the in-memory BTT then write the update out to disk,
 * function should only be called if it supports atomic writes
 */
int cr_write_btt(struct cr_volume *cr_vol, __u64 lba, __u64 rdpa,
	__u16 dimm_id);

/**
 * cr_check_btt_consistency() - Verify the BTT is valid for use
 * @cr_vol: The CR volume
 *
 * Check the consistency of the BTT
 * Ensure all RDPA's are contained within the extents in the volume
 * Ensure all the DIMMs in the BTT are part of the volume
 *
 * CR_Specific
 *
 * Returns:
 */
int cr_check_btt_consistency(struct cr_volume *cr_vol);

/**
 * cr_create_freelist() - Create the block free list
 */
int cr_create_freelist(struct cr_volume *cr_vol);

/**
 * cr_check_freelist_consistency() - Check that the free list is able to be used
 */
int cr_check_freelist_consistency(struct cr_volume *cr_vol);

/**
 * cr_provision_free_block() - retrieve a block of memory from free list
 * XXX: When freelist is defined this can be decided
 */
int cr_provision_free_block(void);

/**
 * cr_return_free_block() - return a block of memory to free list
 * XXX: When freelist is defined this can be decided
 */
int cr_return_free_block(void);

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

/**
 * cr_write_freelist() - Write the free list out to disk
 */
int cr_write_freelist(void);

#endif
