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

#ifndef _CRBD_POOL_H
#define _CRBD_POOL_H

#include <linux/nvdimm_core.h>
#include <linux/nvdimm_acpi.h>

/******************************************************************************
 * CR POOLS
 *****************************************************************************/

/**
 * cr_initialize_pools() - Create and Initialize all CR pools
 * @fit_head: NVM Firmware Interface Table
 * @dimm_list: Head of the list of all NVM DIMMs in the system
 * @pool_list: Head of the list for CR Pools
 *
 * Analyze the NVDIMM FIT and the Information in the NVDIMMs to create pools of
 * persistent memory. Pools are divided by their QoS attributes, memory cannot
 * exist in more than one pool at a time.
 *
 * CR Specific
 *
 * Returns: Error Code?
 */
int cr_initialize_pools(struct fit_header *fit_head,
	struct list_head *dimm_list, struct list_head *pool_list);

/**
 * cr_insert_pool() - Insert a CR Pool into a list of Pools
 * @dimm: Fully initialized CR Pool
 * @list: pool list to add to
 *
 * Wrapper for adding a CR Pool to the global list of pools kept in the
 * CRBD driver.
 *
 * CR Specific
 *
 * Return: Error Code?
 */
int cr_insert_pool(struct pool *pl, struct list_head *pool_list);

/**
 * cr_remove_pool() - Remove a CR Pool from a list of Pools
 * @pool_id: CR Pool ID to remove
 * @list: list to remove from
 *
 * Search a given list for the given POOL ID. If found remove the CR POOL from
 * the list. De-allocates all resources used by POOL, A pool can only be
 * removed if all volumes in the system do not contain any memory extents from
 *  the pool.
 *
 * Most likely will be a helper function for remove DIMM.
 *
 * CR Specific
 *
 * Return: Error Code?
 */
int cr_remove_pool(__u32 pool_id, struct list_head *pool_list);

/**
 * get_remaining_capacity() - Retrieve remaining capacity of a given pool
 * @pl: The pool that we wish to know the capacity for
 *
 * Calculate the remaining capacity in a pool by analyzing the extent sets
 * remaining in the pool
 *
 * CR Specific
 *
 * Returns: Remaining capacity of the Pool, ??? Error codes
 */
__u64 cr_get_remaining_capacity(struct pool *pl);

/**
 * cr_free_pool() - Free memory associated with a CR pool
 *
 * Frees all memory resources in use by a CR Pool.
 *
 * CR Specific
 *
 * Returns: Error Code?
 */
int cr_free_pool(struct pool *pl);

/**
 * cr_provision_extent() - Remove a region of memory from a pool
 * @pl: Pool to remove memory from
 * @size: Amount of memory to remove
 * @dimm_id: If desired user can request memory from a specific dimm
 *
 * If dimm_id is POOL_DEFAULT, remove an extent that best matches the pools
 * type. If it is a PM Capable pool then remove memory in such a way that
 * its PM capable status is preserved. If it is a non-PM capable pool then
 * remove memory from the single DIMM in the pool.
 *
 * If dimm_id is a specific dimm_id that exists in the pool, then adjust the
 * Extent Sets in the pool accordingly. If any Extent sets are no longer PM
 * capable place memory in the corresponding non-PM capable pools.
 *
 * More analysis of this function will be required as it can be complex
 *
 * CR_Specific
 *
 * Returns: A single extent set containing the memory requested
 */
struct extent_set *cr_provision_extent(struct pool *pl, size_t size,
	__u16 dimm_id);

/**
 * cr_return_extent() - Return memory to a CR Pool
 * @pl: Pool to return memory to
 * @es: The extent set to replace in an CR Pool
 *
 * Check and make sure that the extent set can be re-added to a pool
 * Collapse any extent sets that are now contiguous due to the new extent
 *
 * CR Specific
 *
 * Returns: Error Code?
 */
int cr_return_extent(struct pool *pl, struct extent_set *es);

/**
 * cr_find_adjacent_extent() - Analyze a Pool to see if there is an extent
 * set that could be combined with a given extent set
 * @pl: The CR Pool to analyze
 * @es: The Extent set to check if it can be combined
 * @before_es: If an extent set exists before the given extent set return here,
 * else NULL
 * @after_es: If an extent set exists after the given extent set return here,
 * else NULL
 *
 * Check and see if there is an extent set before and after that the
 * given extent set could be combined with.
 *
 * This can probably done in a much more elegant way.
 *
 * CR Specific
 *
 * Returns: The number of adjacent extent sets, Error Codes???
 */
int cr_find_adjacent_extent(struct pool *pl, struct extent_set *es,
	struct extent_set *before_es, struct extent_set *after_ex);

#endif /* _CRBD_POOL_H */
