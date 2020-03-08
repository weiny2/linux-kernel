/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright(c) 2017-2020 Intel Corporation
 */

#ifndef __DLB_BITMAP_H
#define __DLB_BITMAP_H

#include <linux/bitmap.h>
#include <linux/slab.h>

#include "dlb_main.h"

struct dlb_bitmap {
	unsigned long *map;
	unsigned int len;
};

/**
 * dlb_bitmap_alloc() - alloc a bitmap data structure
 * @bitmap: pointer to dlb_bitmap structure pointer.
 * @len: number of entries in the bitmap.
 *
 * This function allocates a bitmap and initializes it with length @len. All
 * entries are initially zero.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise.
 *
 * Errors:
 * EINVAL - bitmap is NULL or len is 0.
 * ENOMEM - could not allocate memory for the bitmap data structure.
 */
static inline int dlb_bitmap_alloc(struct dlb_bitmap **bitmap,
				   unsigned int len)
{
	struct dlb_bitmap *bm;

	if (!bitmap || len == 0)
		return -EINVAL;

	bm = kzalloc(sizeof(*bm), GFP_KERNEL);
	if (!bm)
		return -ENOMEM;

	bm->map = kcalloc(BITS_TO_LONGS(len), sizeof(*bm->map), GFP_KERNEL);
	if (!bm->map) {
		kfree(bm);
		return -ENOMEM;
	}

	bm->len = len;

	*bitmap = bm;

	return 0;
}

/**
 * dlb_bitmap_free() - free a previously allocated bitmap data structure
 * @bitmap: pointer to dlb_bitmap structure.
 *
 * This function frees a bitmap that was allocated with dlb_bitmap_alloc().
 */
static inline void dlb_bitmap_free(struct dlb_bitmap *bitmap)
{
	if (!bitmap)
		return;

	kfree(bitmap->map);

	kfree(bitmap);
}

/**
 * dlb_bitmap_fill() - fill a bitmap with all 1s
 * @bitmap: pointer to dlb_bitmap structure.
 *
 * This function sets all bitmap values to 1.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise.
 *
 * Errors:
 * EINVAL - bitmap is NULL or is uninitialized.
 */
static inline int dlb_bitmap_fill(struct dlb_bitmap *bitmap)
{
	if (!bitmap || !bitmap->map)
		return -EINVAL;

	bitmap_fill(bitmap->map, bitmap->len);

	return 0;
}

/**
 * dlb_bitmap_fill() - fill a bitmap with all 0s
 * @bitmap: pointer to dlb_bitmap structure.
 *
 * This function sets all bitmap values to 0.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise.
 *
 * Errors:
 * EINVAL - bitmap is NULL or is uninitialized.
 */
static inline int dlb_bitmap_zero(struct dlb_bitmap *bitmap)
{
	if (!bitmap || !bitmap->map)
		return -EINVAL;

	bitmap_zero(bitmap->map, bitmap->len);

	return 0;
}

#endif /*  __DLB_BITMAP_H */
