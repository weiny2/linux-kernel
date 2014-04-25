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

#ifndef _GEN_NVM_VOLUMES_H
#define _GEN_NVM_VOLUMES_H

#include <linux/nvdimm_core.h>

/* TODO: these need to move */
/* Ignore */
struct ioctl_create_volume {
	int tbd;
};

/* Ignore */
struct ioctl_modify_volume {
	int tbd;
};

/*************************************************************************
 * NVM VOLUME FUNCTIONS
 *************************************************************************/

/**
 * nvm_initialize_volume_inventory() - Create and configure Volumes that exist on NVDIMMs
 * @dimm_list: The list head of the list that contains all NVM DIMMs
 * @volume_list: The list head of the list to contain all NVM Volumes
 *
 * By the end of the function, all labels should have been read from each DIMM,
 * labels should of been analyzed, volumes from the labels should of been
 * created, volumes initialized, volumes added to system and ready for I/O
 *
 * Generic
 *
 * Returns: Error Code?
 */
int nvm_initialize_volume_inventory(struct list_head *dimm_list,
	struct list_head *volume_list);

/**
 * nvm_check_volume_inventory() - Validate that the entire volume inventory is
 * free of volume errors
 *
 * Call each technology specific volume function for each volume to validate
 * that the volume is able to perform I/O operations
 *
 * Returns: Error Code?
 */
int nvm_check_volume_inventory(void);

/**
 * nvm_insert_volume() - Insert a volume into a list of volumes
 * @volume: Volume to insert
 *
 * Returns: Error Code?
 */
int nvm_insert_volume(struct list_head *volume_list,
	struct nvm_volume *volume);

/**
 * nvm_create_volume() - Create a new user defined volume
 * @ioctl: IOCTL passed in from user that contains all information required to
 * create a new volume.
 *
 * Perform the generic functions for a NVM Volume, after which call
 * the technology specific create_volume function to finish the process.
 *
 * Returns: Pointer to a new NVM Volume, ERR_PTR on error???
 */
struct nvm_volume *nvm_create_volume(struct ioctl_create_volume *ioctl);

void nvm_delete_volume(struct nvm_volume *volume);

/**
 * nvm_modify_volume() - Modify an existing NVM Volume
 * @volume: The NVM volume to modify
 * @ioctl: The IOCTL containing the specifications on how to modify the volume
 *
 * Perform the generic functions required to modify a volume. If any technology
 * specific modifications are required call the modify volume for that
 * technology. Things like grow and shrink should be handled by a technology
 * specific function.
 *
 * Returns: Error Code???
 */
int nvm_modify_volume(struct nvm_volume *volume,
	struct ioctl_modify_volume *ioctl);

/**
 * nvm_update_user_text() - Update the user text in the purpose field
 * of an NVM volume.
 * @volume: Volume to modify
 * @buffer: Buffer containing the new purpose text.
 */
int nvm_update_user_text(struct nvm_volume *volume, char *buffer);

/**
 * nvm_read() - Read data from an LBA
 * @volume: NVM Volume to read data from
 * @lba: LBA of the start of the data region to read
 * @nbytes: Number of bytes to read
 * @buffer: Buffer to place read data into
 *
 * The general idea is that we take the LBA and use a technology specific
 * read function that transforms an LBA into whatever address that technology
 *  uses, and performs a read operation on that data.
 *
 * Return Code: Error Codes???
 */
int nvm_read(struct nvm_volume *volume, unsigned long lba, unsigned long nbytes,
	char *buffer);

/**
 * nvm_write() - Write data from an LBA
 * Same as nvm_read but the write version of it
 */
int nvm_write(struct nvm_volume *volume, unsigned long lba,
	unsigned long nbytes, char *buffer);

/**
 * nvm_check_consistency() - Check for Volume errors
 * @volume: Volume to check for errors
 *
 * Validates that the volume is able to perform I/O operations
 *
 * Returns: Error Code???
 */
int nvm_check_consistency(struct nvm_volume *volume);

/**
 * nvm_enable() - Enable a volume for use
 * @volume: Volume to enable for use
 *
 * Sets the user defined flag that defines if the volume can be used
 *
 * Returns: Error Code???
 */
int nvm_enable(struct nvm_volume *volume);

/**
 * nvm_disable() - Disable a volume for use
 * @volume: Volume to disable for use
 *
 * Sets the user defined flag that defines if the volume can be used
 *
 * Returns: Error Code???
 */
int nvm_disable(struct nvm_volume *volume);

/**
 * nvm_get_capacity() - Get the Capacity of a volume
 * @volume: Volume to get the capacity for
 *
 * Calls into a technology specific function for the capacity of a volume
 *
 * Returns: Capacity of the volume, or Error Codes
 */
__u64 nvm_get_capacity(struct nvm_volume *volume);

/**
 * nvm_write_labels() - Write volume labels in the nvm volume structure to disk
 * @volume: Volume we wish to write the labels for
 *
 * Calls into a technology specific function to write out the labels to disk
 *
 * Returns: Error Code
 */
int nvm_write_labels(struct nvm_volume *volume);

int nvm_volume_ioctl(struct block_device *bdev, fmode_t mode, unsigned cmd,
	unsigned long arg);

/**
 * nvm_make_request() - Perform generic I/O functions for NVM
 * @q:
 * @bio:
 *
 * For each segment of the BIO, make a call to the technology specific
 * read/write functions
 *
 * Generic
 *
 * Returns:
 */
void nvm_make_request(struct request_queue *q, struct bio *bio);

struct nvm_volume *nvm_create_single_volume(struct pmem_dev *dev);

#endif /* _GEN_NVM_VOLUMES_H */
