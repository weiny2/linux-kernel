/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause)
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
	struct device *dev;
};

static inline int __dlb_bitmap_alloc(struct device *device,
				     struct dlb_bitmap **bitmap,
				     unsigned int len)
{
	struct dlb_bitmap *bm;

	if (!bitmap || len == 0)
		return -EINVAL;

	bm = devm_kzalloc(device, sizeof(*bm), GFP_KERNEL);
	if (!bm)
		return -ENOMEM;

	bm->map = devm_kzalloc(device,
			       BITS_TO_LONGS(len) * sizeof(*bm->map),
			       GFP_KERNEL);
	if (!bm->map)
		return -ENOMEM;

	bm->len = len;
	bm->dev = device;

	*bitmap = bm;

	return 0;
}

/**
 * dlb_bitmap_alloc() - alloc a bitmap data structure
 * @hw: dlb_hw handle.
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
static inline int dlb_bitmap_alloc(struct dlb_hw *hw,
				   struct dlb_bitmap **bitmap,
				   unsigned int len)
{
	struct dlb_dev *dev;

	dev = container_of(hw, struct dlb_dev, hw);

	return __dlb_bitmap_alloc(&dev->pdev->dev, bitmap, len);
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

	devm_kfree(bitmap->dev, bitmap->map);

	devm_kfree(bitmap->dev, bitmap);
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

/**
 * dlb_bitmap_set_range() - set a range of bitmap entries
 * @bitmap: pointer to dlb_bitmap structure.
 * @bit: starting bit index.
 * @len: length of the range.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise.
 *
 * Errors:
 * EINVAL - bitmap is NULL or is uninitialized, or the range exceeds the bitmap
 *	    length.
 */
static inline int dlb_bitmap_set_range(struct dlb_bitmap *bitmap,
				       unsigned int bit,
				       unsigned int len)
{
	if (!bitmap || !bitmap->map)
		return -EINVAL;

	if (bitmap->len <= bit)
		return -EINVAL;

	bitmap_set(bitmap->map, bit, len);

	return 0;
}

/**
 * dlb_bitmap_clear_range() - clear a range of bitmap entries
 * @bitmap: pointer to dlb_bitmap structure.
 * @bit: starting bit index.
 * @len: length of the range.
 *
 * Return:
 * Returns 0 upon success, < 0 otherwise.
 *
 * Errors:
 * EINVAL - bitmap is NULL or is uninitialized, or the range exceeds the bitmap
 *	    length.
 */
static inline int dlb_bitmap_clear_range(struct dlb_bitmap *bitmap,
					 unsigned int bit,
					 unsigned int len)
{
	if (!bitmap || !bitmap->map)
		return -EINVAL;

	if (bitmap->len <= bit)
		return -EINVAL;

	bitmap_clear(bitmap->map, bit, len);

	return 0;
}

/**
 * dlb_bitmap_find_set_bit_range() - find a range of set bits
 * @bitmap: pointer to dlb_bitmap structure.
 * @len: length of the range.
 *
 * This function looks for a range of set bits of length @len.
 *
 * Return:
 * Returns the base bit index upon success, < 0 otherwise.
 *
 * Errors:
 * ENOENT - unable to find a length *len* range of set bits.
 * EINVAL - bitmap is NULL or is uninitialized, or len is invalid.
 */
static inline int dlb_bitmap_find_set_bit_range(struct dlb_bitmap *bitmap,
						unsigned int len)
{
	struct dlb_bitmap *complement_mask = NULL;
	int ret;

	if (!bitmap || !bitmap->map || len == 0)
		return -EINVAL;

	if (bitmap->len < len)
		return -ENOENT;

	ret = __dlb_bitmap_alloc(bitmap->dev, &complement_mask, bitmap->len);
	if (ret)
		return ret;

	dlb_bitmap_zero(complement_mask);

	bitmap_complement(complement_mask->map, bitmap->map, bitmap->len);

	ret = bitmap_find_next_zero_area(complement_mask->map,
					 complement_mask->len,
					 0,
					 len,
					 0);

	dlb_bitmap_free(complement_mask);

	/* No set bit range of length len? */
	return (ret >= (int)bitmap->len) ? -ENOENT : ret;
}

/**
 * dlb_bitmap_count() - returns the number of set bits
 * @bitmap: pointer to dlb_bitmap structure.
 *
 * This function looks for a single set bit.
 *
 * Return:
 * Returns the number of set bits upon success, <0 otherwise.
 *
 * Errors:
 * EINVAL - bitmap is NULL or is uninitialized.
 */
static inline int dlb_bitmap_count(struct dlb_bitmap *bitmap)
{
	if (!bitmap || !bitmap->map)
		return -EINVAL;

	return bitmap_weight(bitmap->map, bitmap->len);
}

/**
 * dlb_bitmap_longest_set_range() - returns longest contiguous range of set bits
 * @bitmap: pointer to dlb_bitmap structure.
 *
 * Return:
 * Returns the bitmap's longest contiguous range of of set bits upon success,
 * <0 otherwise.
 *
 * Errors:
 * EINVAL - bitmap is NULL or is uninitialized.
 */
static inline int dlb_bitmap_longest_set_range(struct dlb_bitmap *bitmap)
{
	unsigned int bits_per_long;
	unsigned int i, j;
	int max_len, len;

	if (!bitmap || !bitmap->map)
		return -EINVAL;

	if (dlb_bitmap_count(bitmap) == 0)
		return 0;

	max_len = 0;
	len = 0;
	bits_per_long = sizeof(unsigned long) * BITS_PER_BYTE;

	for (i = 0; i < BITS_TO_LONGS(bitmap->len); i++) {
		for (j = 0; j < bits_per_long; j++) {
			if ((i * bits_per_long + j) >= bitmap->len)
				break;

			len = (test_bit(j, &bitmap->map[i])) ? len + 1 : 0;

			if (len > max_len)
				max_len = len;
		}
	}

	return max_len;
}

/**
 * dlb_bitmap_or() - store the logical 'or' of two bitmaps into a third
 * @dest: pointer to dlb_bitmap structure, which will contain the results of
 *	  the 'or' of src1 and src2.
 * @src1: pointer to dlb_bitmap structure, will be 'or'ed with src2.
 * @src2: pointer to dlb_bitmap structure, will be 'or'ed with src1.
 *
 * This function 'or's two bitmaps together and stores the result in a third
 * bitmap. The source and destination bitmaps can be the same.
 *
 * Return:
 * Returns the number of set bits upon success, <0 otherwise.
 *
 * Errors:
 * EINVAL - One of the bitmaps is NULL or is uninitialized.
 */
static inline int dlb_bitmap_or(struct dlb_bitmap *dest,
				struct dlb_bitmap *src1,
				struct dlb_bitmap *src2)
{
	unsigned int min;

	if (!dest || !dest->map ||
	    !src1 || !src1->map ||
	    !src2 || !src2->map)
		return -EINVAL;

	min = dest->len;
	min = (min > src1->len) ? src1->len : min;
	min = (min > src2->len) ? src2->len : min;

	bitmap_or(dest->map, src1->map, src2->map, min);

	return 0;
}

#endif /*  __DLB_BITMAP_H */
