/*
 * Copyright(c) 2013-2014 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */
#include <linux/device.h>
#include <linux/ndctl.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/nd.h>
#include "label.h"
#include "nd.h"

static u32 inc_seq(u32 seq)
{
	seq = (seq + 1) & NSINDEX_SEQ_MASK;
	if (!seq)
		seq = 1;
	return seq;
}

static u32 best_seq(u32 a, u32 b)
{
	a &= NSINDEX_SEQ_MASK;
	b &= NSINDEX_SEQ_MASK;

	if (a == 0 || a == b)
		return b;
	else if (b == 0)
		return a;
	else if (inc_seq(a) == b)
		return b;
	else
		return a;
}

size_t sizeof_namespace_index(struct nd_dimm *nd_dimm)
{
	u32 index_span;

	if (nd_dimm->nsindex_size)
		return nd_dimm->nsindex_size;

	/*
	 * The minimum index space is 512 bytes, with that amount of
	 * index we can describe ~1400 labels which is less than a byte
	 * of overhead per label.  Round up to a byte of overhead per
	 * label and determine the size of the index region.  Yes, this
	 * starts to waste space at larger config_sizes, but it's
	 * unlikely we'll ever see anything but 128K.
	 */
	index_span = nd_dimm->config_size / 129;
	index_span /= NSINDEX_ALIGN * 2;
	nd_dimm->nsindex_size = index_span * NSINDEX_ALIGN;

	return nd_dimm->nsindex_size;
}

static int nd_dimm_num_label_slots(struct nd_dimm *nd_dimm)
{
	return nd_dimm->config_size / 129;
}

int nd_label_validate(struct nd_dimm *nd_dimm)
{
	/*
	 * On media label format consists of two index blocks followed
	 * by an array of labels.  None of these structures are ever
	 * updated in place.  A sequence number tracks the current
	 * active index and the next one to write, while labels are
	 * written to free slots.
	 *
	 *     +------------+
	 *     |            |
	 *     |  nsindex0  |
	 *     |            |
	 *     +------------+
	 *     |            |
	 *     |  nsindex1  |
	 *     |            |
	 *     +------------+
	 *     |   label0   |
	 *     +------------+
	 *     |   label1   |
	 *     +------------+
	 *     |            |
	 *      ....nslot...
	 *     |            |
	 *     +------------+
	 *     |   labelN   |
	 *     +------------+
	 */
	struct nd_namespace_index __iomem *nsindex[] = {
		to_namespace_index(nd_dimm, 0),
		to_namespace_index(nd_dimm, 1),
	};
	const int num_index = ARRAY_SIZE(nsindex);
	struct device *dev = &nd_dimm->dev;
	bool valid[] = { false, false };
	int i, num_valid = 0;
	u32 seq;

	for (i = 0; i < num_index; i++) {
		u64 sum_save, sum;
		u8 sig[NSINDEX_SIG_LEN];

		memcpy_fromio(sig, nsindex[i]->sig, NSINDEX_SIG_LEN);
		if (memcmp(sig, NSINDEX_SIGNATURE, NSINDEX_SIG_LEN) != 0) {
			dev_dbg(dev, "%s: nsindex%d signature invalid\n",
					__func__, i);
			continue;
		}
		sum_save = readq(&nsindex[i]->checksum);
		writeq(0, &nsindex[i]->checksum);
		sum = nd_fletcher64(nsindex[i],
				sizeof_namespace_index(nd_dimm));
		writeq(sum_save, &nsindex[i]->checksum);
		if (sum != sum_save) {
			dev_dbg(dev, "%s: nsindex%d checksum invalid\n",
					__func__, i);
			continue;
		}
		if ((readl(&nsindex[i]->seq) & NSINDEX_SEQ_MASK) == 0) {
			dev_dbg(dev, "%s: nsindex%d sequence: %#x invalid\n",
					__func__, i, readl(&nsindex[i]->seq));
			continue;
		}

		/* sanity check the index against expected values */
		if (readq(&nsindex[i]->myoff)
				!= i * sizeof_namespace_index(nd_dimm)) {
			dev_dbg(dev, "%s: nsindex%d myoff: %#llx invalid\n",
					__func__, i, (unsigned long long)
					readq(&nsindex[i]->myoff));
			continue;
		}
		if (readq(&nsindex[i]->otheroff)
				!= (!i) * sizeof_namespace_index(nd_dimm)) {
			dev_dbg(dev, "%s: nsindex%d otheroff: %#llx invalid\n",
					__func__, i, (unsigned long long)
					readq(&nsindex[i]->otheroff));
			continue;
		}
		if (readq(&nsindex[i]->mysize) > sizeof_namespace_index(nd_dimm)
				|| readq(&nsindex[i]->mysize)
				< sizeof(struct nd_namespace_index)) {
			dev_dbg(dev, "%s: nsindex%d mysize: %#llx invalid\n",
					__func__, i, (unsigned long long)
					readq(&nsindex[i]->mysize));
			continue;
		}
		if (readl(&nsindex[i]->nslot) * sizeof(struct nd_namespace_label)
				+ 2 * sizeof_namespace_index(nd_dimm)
				> nd_dimm->config_size) {
			dev_dbg(dev, "%s: nsindex%d nslot: %u invalid, config_size: %#x\n",
					__func__, i, readl(&nsindex[i]->nslot),
					nd_dimm->config_size);
			continue;
		}
		valid[i] = true;
		num_valid++;
	}

	switch (num_valid) {
	case 0:
		break;
	case 1:
		for (i = 0; i < num_index; i++)
			if (valid[i])
				return i;
		/* can't have num_valid > 0 but valid[] = { false, false } */
		WARN_ON(1);
		break;
	default:
		/* pick the best index... */
		seq = best_seq(readl(&nsindex[0]->seq), readl(&nsindex[1]->seq));
		if (seq == (readl(&nsindex[1]->seq) & NSINDEX_SEQ_MASK))
			return 1;
		else
			return 0;
		break;
	}

	return -1;
}

void nd_label_copy(struct nd_dimm *nd_dimm,
		struct nd_namespace_index __iomem *dst,
		struct nd_namespace_index __iomem *src)
{
	void *s, *d;

	if (dst && src)
		/* pass */;
	else
		return;

	d = (void * __force) dst;
	s = (void * __force) src;
	memcpy(d, s, sizeof_namespace_index(nd_dimm));
}

static struct nd_namespace_label __iomem *nd_label_base(struct nd_dimm *nd_dimm)
{
	void *base = to_namespace_index(nd_dimm, 0);

	return base + 2 * sizeof_namespace_index(nd_dimm);
}

int to_nd_label_slot(struct nd_dimm *nd_dimm,
		struct nd_namespace_label __iomem *nd_label)
{
	return nd_label - nd_label_base(nd_dimm);
}

#define for_each_clear_bit_le(bit, addr, size) \
	for ((bit) = find_next_zero_bit_le((addr), (size), 0);	\
	     (bit) < (size);					\
	     (bit) = find_next_zero_bit_le((addr), (size), (bit) + 1))

/**
 * preamble_index - common variable initialization for nd_label_* routines
 * @nd_dimm: dimm container for the relevant label set
 * @idx: namespace_index index
 * @nsindex: on return set to the currently active namespace index
 * @free: on return set to the free label bitmap in the index
 * @nslot: on return set to the number of slots in the label space
 */
static bool preamble_index(struct nd_dimm *nd_dimm, int idx,
		struct nd_namespace_index **nsindex,
		unsigned long **free, u32 *nslot)
{
	*nsindex = to_namespace_index(nd_dimm, idx);
	if (*nsindex == NULL)
		return false;

	*free = (unsigned long __force *) (*nsindex)->free;
	*nslot = readl(&(*nsindex)->nslot);

	return true;
}

static bool preamble_current(struct nd_dimm *nd_dimm,
		struct nd_namespace_index **nsindex,
		unsigned long **free, u32 *nslot)
{
	return preamble_index(nd_dimm, nd_dimm->ns_current, nsindex,
			free, nslot);
}

static bool preamble_next(struct nd_dimm *nd_dimm,
		struct nd_namespace_index **nsindex,
		unsigned long **free, u32 *nslot)
{
	return preamble_index(nd_dimm, nd_dimm->ns_next, nsindex,
			free, nslot);
}

int nd_label_active_count(struct nd_dimm *nd_dimm)
{
	struct nd_namespace_index __iomem *nsindex;
	unsigned long *free;
	u32 nslot, slot;
	int count = 0;

	if (!preamble_current(nd_dimm, &nsindex, &free, &nslot))
		return 0;

	for_each_clear_bit_le(slot, free, nslot) {
		struct nd_namespace_label __iomem *nd_label;

		nd_label = nd_label_base(nd_dimm) + slot;

		/* check for invalid slots */
		if (slot != readl(&nd_label->slot)) {
			dev_dbg(&nd_dimm->dev,
					"%s: slot%d invalid 'slot' (%d)\n",
					__func__, slot, readl(&nd_label->slot));
			continue;
		}
		count++;
	}
	return count;
}

struct nd_namespace_label __iomem *nd_label_active(
		struct nd_dimm *nd_dimm, int n)
{
	struct nd_namespace_index __iomem *nsindex;
	unsigned long *free;
	u32 nslot, slot;

	if (!preamble_current(nd_dimm, &nsindex, &free, &nslot))
		return NULL;

	for_each_clear_bit_le(slot, free, nslot) {
		struct nd_namespace_label __iomem *nd_label;

		nd_label = nd_label_base(nd_dimm) + slot;
		if (slot != readl(&nd_label->slot))
			continue;

		if (n-- == 0)
			return nd_label_base(nd_dimm) + slot;
	}

	return NULL;
}

u32 nd_label_alloc_slot(struct nd_dimm *nd_dimm)
{
	struct nd_namespace_index __iomem *nsindex;
	unsigned long *free;
	u32 nslot, slot;

	if (!preamble_next(nd_dimm, &nsindex, &free, &nslot))
		return UINT_MAX;

	WARN_ON(!is_nd_bus_locked(&nd_dimm->dev));

	slot = find_next_bit_le(free, nslot, 0);
	if (slot == nslot)
		return UINT_MAX;

	clear_bit_le(slot, free);

	return slot;
}

bool nd_label_free_slot(struct nd_dimm *nd_dimm, u32 slot)
{
	struct nd_namespace_index __iomem *nsindex;
	unsigned long *free;
	u32 nslot;

	if (!preamble_next(nd_dimm, &nsindex, &free, &nslot))
		return false;

	WARN_ON(!is_nd_bus_locked(&nd_dimm->dev));

	if (slot < nslot)
		return !test_and_set_bit_le(slot, free);
	return false;
}

u32 nd_label_nfree(struct nd_dimm *nd_dimm)
{
	struct nd_namespace_index __iomem *nsindex;
	unsigned long *free;
	u32 nslot;

	WARN_ON(!is_nd_bus_locked(&nd_dimm->dev));

	if (!preamble_next(nd_dimm, &nsindex, &free, &nslot))
		return 0;

	return bitmap_weight(free, nslot);
}

static int nd_label_write_index(struct nd_dimm *nd_dimm, int index, u32 seq,
		unsigned long flags)
{
	struct nd_namespace_index *nsindex = to_namespace_index(nd_dimm, index);
	unsigned long offset;
	u64 checksum;
	u32 nslot;
	int rc;

	if (flags & ND_NSINDEX_INIT)
		nslot = nd_dimm_num_label_slots(nd_dimm);
	else
		nslot = readl(&nsindex->nslot);

	memcpy_toio(nsindex->sig, NSINDEX_SIGNATURE, NSINDEX_SIG_LEN);
	writel(0, &nsindex->flags);
	writel(seq, &nsindex->seq);
	offset = (unsigned long) nsindex
		- (unsigned long) to_namespace_index(nd_dimm, 0);
	writeq(offset, &nsindex->myoff);
	writeq(sizeof_namespace_index(nd_dimm), &nsindex->mysize);
	offset = (unsigned long) to_namespace_index(nd_dimm,
			nd_label_next_nsindex(index))
		- (unsigned long) to_namespace_index(nd_dimm, 0);
	writeq(offset, &nsindex->otheroff);
	offset = (unsigned long) nd_label_base(nd_dimm)
		- (unsigned long) to_namespace_index(nd_dimm, 0);
	writeq(offset, &nsindex->labeloff);
	writel(nslot, &nsindex->nslot);
	writew(1, &nsindex->major);
	writew(1, &nsindex->minor);
	writeq(0, &nsindex->checksum);
	if (flags & ND_NSINDEX_INIT) {
		unsigned long *free = (unsigned long __force *) nsindex->free;
		u32 nfree = ALIGN(nslot, BITS_PER_LONG);
		int last_bits, i;

		memset_io(nsindex->free, 0xff, nfree / 8);
		for (i = 0, last_bits = nfree - nslot; i < last_bits; i++)
			clear_bit_le(nslot + i, free);
	}
	checksum = nd_fletcher64(nsindex, sizeof_namespace_index(nd_dimm));
	writeq(checksum, &nsindex->checksum);
	rc = nd_dimm_set_config_data(nd_dimm, readq(&nsindex->myoff),
			nsindex, sizeof_namespace_index(nd_dimm));
	dev_dbg(&nd_dimm->dev, "%s: seq: %d nslot: %d rc: %d\n",
			__func__, seq, nslot, rc);
	if (rc < 0)
		return rc;

	if (flags & ND_NSINDEX_INIT)
		return 0;

	/* copy the index we just wrote to the new 'next' */
	WARN_ON(index != nd_dimm->ns_next);
	nd_label_copy(nd_dimm, to_current_namespace_index(nd_dimm), nsindex);
	nd_dimm->ns_current = nd_label_next_nsindex(nd_dimm->ns_current);
	nd_dimm->ns_next = nd_label_next_nsindex(nd_dimm->ns_next);
	WARN_ON(nd_dimm->ns_current == nd_dimm->ns_next);

	return 0;
}

static int __pmem_label_update(struct nd_region *nd_region,
		struct nd_mapping *nd_mapping, struct nd_namespace_pmem *nspm,
		int pos)
{
	u64 cookie = nd_region_interleave_set_cookie(nd_region), rawsize;
	struct nd_dimm *nd_dimm = nd_mapping->nd_dimm;
	struct nd_namespace_label __iomem *nd_label;
	struct nd_namespace_index __iomem *nsindex;
	unsigned long *free;
	u32 nslot, slot;
	size_t offset;
	int rc;

	if (!preamble_next(nd_dimm, &nsindex, &free, &nslot))
		return -ENXIO;

	/*
	 * Now is a good time to garbage collect any stale slots, and
	 * the newly invalidated slots that previously defined this
	 * namespace
	 */
	for_each_clear_bit_le(slot, free, nslot) {
		unsigned long flags;

		nd_label = nd_label_base(nd_dimm) + slot;
		flags = readl(&nd_label->flags);
		if (flags & NSLABEL_FLAG_LOCAL)
			continue; /* skip blk namespaces */
		dev_dbg(&nd_dimm->dev, "%s: free %d\n", __func__, slot);
		nd_label_free_slot(nd_dimm, slot);
	}

	/* allocate and write the label to the staging (next) index */
	slot = nd_label_alloc_slot(nd_dimm);
	if (slot == UINT_MAX)
		return -ENXIO;
	dev_dbg(&nd_dimm->dev, "%s: allocated: %d\n", __func__, slot);

	nd_label = nd_label_base(nd_dimm) + slot;
	memset_io(nd_label, 0, sizeof(struct nd_namespace_label));
	memcpy_toio(nd_label->uuid, nspm->uuid, NSLABEL_UUID_LEN);
	if (nspm->alt_name)
		memcpy_toio(nd_label->name, nspm->alt_name, NSLABEL_NAME_LEN);
	writel(NSLABEL_FLAG_UPDATING, &nd_label->flags);
	writew(nd_region->ndr_mappings, &nd_label->nlabel);
	writew(pos, &nd_label->position);
	writeq(cookie, &nd_label->isetcookie);
	writeq(nd_mapping->start, &nd_label->dpa);
	rawsize = resource_size(&nspm->nsio.res) / nd_region->ndr_mappings;
	writeq(rawsize, &nd_label->rawsize);
	writel(slot, &nd_label->slot);

	/* update label */
	offset = (unsigned long) nd_label
		- (unsigned long) to_namespace_index(nd_dimm, 0);
	rc = nd_dimm_set_config_data(nd_dimm, offset, nd_label,
			sizeof(struct nd_namespace_index));
	if (rc < 0)
		return rc;
	/* update index */
	rc = nd_label_write_index(nd_dimm, nd_dimm->ns_next,
			inc_seq(readl(&nsindex->seq)), 0);
	if (rc < 0)
		return rc;
	return slot;
}

static int init_labels(struct nd_mapping *nd_mapping)
{
	int i;
	struct nd_namespace_index __iomem *nsindex;
	struct nd_dimm *nd_dimm = nd_mapping->nd_dimm;

	if (nd_dimm->ns_current == -1 || nd_dimm->ns_next == -1)
		/* pass */;
	else
		return 0;

	nd_mapping->labels = kcalloc(2, sizeof(struct nd_namespace_label *),
			GFP_KERNEL);
	if (!nd_mapping->labels)
		return -ENOMEM;

	nsindex = to_namespace_index(nd_dimm, 0);
	memset_io(nsindex, 0, nd_dimm->config_size);
	for (i = 0; i < 2; i++) {
		int rc = nd_label_write_index(nd_dimm, i, i*2, ND_NSINDEX_INIT);

		if (rc)
			return rc;
	}
	nd_dimm->ns_next = 1;
	nd_dimm->ns_current = 0;

	return 0;
}

int nd_pmem_namespace_label_update(struct nd_region *nd_region,
		struct nd_namespace_pmem *nspm)
{
	int i;

	for (i = 0; i < nd_region->ndr_mappings; i++) {
		struct nd_mapping *nd_mapping = &nd_region->mapping[i];
		struct nd_dimm *nd_dimm = nd_mapping->nd_dimm;
		int rc;

		rc = init_labels(nd_mapping);
		if (rc)
			return rc;

		/*
		 * On success __pmem_label_update() returns the slot that
		 * was written or a negative error code.
		 */
		rc = __pmem_label_update(nd_region, nd_mapping, nspm, i);
		if (rc < 0)
			return rc;
		nd_mapping->labels[0] = nd_label_base(nd_dimm) + rc;
	}

	return 0;
}
