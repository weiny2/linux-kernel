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

#include <linux/gen_nvm_volumes.h>
#include <linux/nvdimm_acpi.h>
#include <linux/crbd_pool.h>

static void nvm_print_extent(struct extent_set *mem_extent)
{
	int i = 0;
	NVDIMM_VDBG("***NVM Extent***\n"
		"SetSize: %llu\n"
		"PM Capable: %hhu\n"
		"SPA Start: %#llx",
		(__u64)mem_extent->set_size,
		mem_extent->pm_capable,
		mem_extent->spa_start);

	NVDIMM_VDBG("NVDIMMs in Extent:");

	while (i < MAX_INTERLEAVE && mem_extent->physical_id[i] != 0) {
		NVDIMM_VDBG("%hu\n", mem_extent->physical_id[i]);
		i++;
	}
}

static void nvm_print_volume(struct nvm_volume *volume)
{
	struct extent_set *mem_extent;

	NVDIMM_VDBG("***NVM Volume***\n"
		"BlockSize: %llu\n"
		"BlockCount: %llu\n"
		"Attributes: %#x",
		volume->block_size,
		volume->block_count,
		volume->volume_attributes);

	list_for_each_entry(mem_extent, &volume->extent_set_list,
			extent_set_node) {
		nvm_print_extent(mem_extent);
	}
}

/**
 * spa_rng_to_extent() - SPA Range to Extent
 * @fit_head:
 * @s_tbl:
 *
 * Create an extent_set from the memory defined by a spa_rng_tbl
 * This is a very simplistic and incomplete approach.
 * It is only a temporary measure
 *
 * Returns:
 * Success  - a populated memory extent
 * ENOMEM   - kalloc failure
 */
static struct extent_set *spa_rng_to_extent(struct fit_header *fit_head,
	struct spa_rng_tbl *s_tbl)
{
	struct extent_set *mem_extent = kzalloc(sizeof(*mem_extent),
			GFP_KERNEL);
	int i;
	int j = 0;

	if (!mem_extent)
		return ERR_PTR(-ENOMEM);

	mem_extent->set_size = s_tbl->length;
	mem_extent->pm_capable = 1;
	mem_extent->spa_start = s_tbl->start_addr;

	/* TODO: Find NUMA Node from SPA RNG */
	mem_extent->numa_node = NUMA_NO_NODE;

	mem_extent->interleave_size = 0;

	for (i = 0; i < fit_head->fit->num_memdev_spa_range_tbl; i++) {
		if (fit_head->memdev_spa_rng_tbls[i].spa_index
			== s_tbl->spa_index) {
			mem_extent->physical_id[j++] =
				fit_head->memdev_spa_rng_tbls[i].mem_dev_pid;
		}
	}

	return mem_extent;
}

/**
 * nvm_replace_extent() - Replace Extent
 * @extent: The extent set to replace
 *
 * Replace an extent set from a volume back into a nvm_pool
 */
static void nvm_replace_extent(struct extent_set *extent)
{
	/* TODO: remove memory free when pool implemented */
	kfree(extent);
	/* TODO: Locate pool extent came from */
	/* TODO: Add extent to pool */
}

/**
 * nvm_remove_extents() - Remove Extents
 * @volume: NVM volume to remove all memory extents from
 *
 * Remove all of the extent_sets from an nvm volume and replace them
 * back into the pool in which they were allocated from
 */
static void nvm_remove_extents(struct nvm_volume *volume)
{
	struct extent_set *cur_extent, *tmp_extent;

	list_for_each_entry_safe(cur_extent, tmp_extent,
			&volume->extent_set_list, extent_set_node) {
		mutex_lock(&volume->extent_set_lock);
		list_del(&cur_extent->extent_set_node);
		mutex_unlock(&volume->extent_set_lock);
		nvm_replace_extent(cur_extent);
	}
}

/**
 * nvm_delete_volume() - Delete a Volume
 * @volume: The volume to delete
 *
 * Perform the generic functions required to delete a volume from the system
 * This includes destroying the volume labels on disk, replacing extents to
 * nvm pools, and freeing the volume memory
 */
void nvm_delete_volume(struct nvm_volume *volume)
{
	nvm_remove_extents(volume);
	/* TODO: Destroy volume label on disk */
	/* TODO: Modify the freelist of labels on disk */
	/* TODO: Free volume labels */
	kfree(volume);
}

struct nvm_volume *nvm_create_single_volume(struct pmem_dev *dev)
{
	struct nvm_volume *volume = kzalloc(sizeof(*volume), GFP_KERNEL);
	struct fit_header *fit_head = dev->fit_head;
	int i;
	__u64 capacity = 0;

	INIT_LIST_HEAD(&volume->extent_set_list);
	mutex_init(&volume->extent_set_lock);

	for (i = 0; i < fit_head->fit->num_spa_range_tbl; i++) {
		if (fit_head->spa_rng_tbls[i].addr_rng_type
			== NVDIMM_PM_RNG_TYPE) {
			struct extent_set *new_extent = spa_rng_to_extent(
				fit_head,
				&fit_head->spa_rng_tbls[i]);

			if (IS_ERR(new_extent)) {
				nvm_delete_volume(volume);
				return ERR_PTR(PTR_ERR(new_extent));
			}

			capacity += new_extent->set_size;
			mutex_lock(&volume->extent_set_lock);
			list_add_tail(&new_extent->extent_set_node,
					&volume->extent_set_list);
			mutex_unlock(&volume->extent_set_lock);
			volume->interleave_size = new_extent->interleave_size;
		}
	}

	volume->volume_id = 0;
	volume->block_size = (1 << 12);
	volume->block_count = (capacity >> 12);
	volume->volume_attributes = (VOLUME_COMPLETE | VOLUME_ENABLED
			| VOLUME_PM_CAPABLE);

	nvm_print_volume(volume);

	spin_lock(&dev->volume_lock);
	list_add_tail(&volume->volume_node, &dev->volumes);
	spin_unlock(&dev->volume_lock);

	return volume;
}
