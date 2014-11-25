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
#include <linux/io.h>
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
