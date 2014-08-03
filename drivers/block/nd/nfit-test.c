/*
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
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
 *
 * BSD LICENSE
 *
 * Copyright(c) 2013-2014 Intel Corporation. All rights reserved.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 */
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/radix-tree.h>
#include <linux/module.h>
#include <linux/sizes.h>
#include <linux/slab.h>
#include "nfit.h"

/*
 * Generate an NFIT table and dimm labels to describe the following topology:
 *                                 (a)                 (b)      DIMM   BLK-REGION
 *           +-------------------+--------+--------+--------+
 * +------+  |       pm0.0       | blk2.0 | pm1.0  | blk2.1 | 000:0000  region2
 * | imc0 +--+- - - region0- - - +--------+        +--------+
 * +--+---+  |       pm0.0       | blk3.0 | pm1.0  | blk3.1 | 000:0001  region3
 *    |      +-------------------+--------v        v--------+
 * +--+---+                               |                 |
 * | cpu0 |                                     region1
 * +--+---+                               |                 |
 *    |      +----------------------------^        ^--------+
 * +--+---+  |           blk4.0           | pm1.0  | blk4.0 | 000:0100  region4
 * | imc1 +--+----------------------------|        +--------+
 * +------+  |           blk5.0           | pm1.0  | blk5.0 | 000:0101  region5
 *           +----------------------------+--------+--------+
 *
 * *) In this layout we have four dimms and two memory controllers in one
 *    socket.  Each block-accessible range on each dimm is allocated a
 *    "region" index and each interleave set (cross dimm span) is allocated a
 *    "region" index.
 *
 * *) The first portion of dimm-000:0000 and dimm-000:0001 are
 *    interleaved as REGION0.  Some of that interleave set is reclaimed as
 *    block space starting at offregion (a) into each dimm.  In that reclaimed
 *    space we create two block-mode-namespaces from the dimms'
 *    control-regions/block-data-windows blk2.0 and blk3.0 respectively.
 *
 * *) In the last portion of dimm-000:0000 and dimm-000:0001 we have an
 *    interleave set, REGION1, that spans those two dimms as well as
 *    dimm-000:0100 and dimm-000:0101.  Some of REGION1 is reclaimed as block
 *    space and the remainder is instantiated as namespace blk1.0.
 *
 * *) The portion of dimm-000:0100 and dimm-000:0101 that do not
 *    participate in REGION1 are instantiated as blk4.0 and blk5.0, but note
 *    that those block namespaces also incorporate the unused portion of
 *    REGION1 (starting at offregion (b)) in those dimms respectively.
 */
enum {
	NUM_PM  = 2,
	NUM_DCR = 4,
	NUM_BDW = NUM_DCR,
	NUM_SPA = NUM_PM + NUM_DCR + NUM_BDW,
	NUM_MEM = NUM_DCR + 2 /* spa0 regions */ + 4 /* spa1 regions */,
	DIMM_SIZE = 8 * SZ_1M,
	LABEL_SIZE = 128*SZ_1K,
	SPA0_SIZE = DIMM_SIZE,
	SPA1_SIZE = DIMM_SIZE*2,
	BDW_SIZE = 64 << 8,
	DCR_SIZE = 12,
	NUM_NFITS = 1, /* permit testing multiple NFITs per sysstem */
};

struct nfit_test_dcr {
	__le64 bdw_addr;
	__le32 bdw_status;
	__u8 aperature[BDW_SIZE];
};

static u32 handle[NUM_DCR] = {
	[0] = NFIT_DIMM_HANDLE(0, 0, 0, 0, 0),
	[1] = NFIT_DIMM_HANDLE(0, 0, 0, 0, 1),
	[2] = NFIT_DIMM_HANDLE(0, 0, 1, 0, 0),
	[3] = NFIT_DIMM_HANDLE(0, 0, 1, 0, 1),
};

struct nfit_test {
	struct nfit_bus_descriptor nfit_desc;
	struct platform_device pdev;
	void __iomem *nfit_buf;
	struct nd_bus *nd_bus;
	dma_addr_t nfit_dma;
	size_t nfit_size;
	void *dimm[NUM_DCR];
	dma_addr_t dimm_dma[NUM_DCR];
	void *label[NUM_DCR];
	dma_addr_t label_dma[NUM_DCR];
	void *spa_set[NUM_PM];
	dma_addr_t spa_set_dma[NUM_PM];
	struct nfit_test_dcr *dcr[NUM_DCR];
	dma_addr_t dcr_dma[NUM_DCR];
};

static struct nfit_test *to_nfit_test(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);

	return container_of(pdev, struct nfit_test, pdev);
}

static int nfit_test_ctl(struct nfit_bus_descriptor *nfit_desc,
		unsigned int cmd, void *buf, unsigned int buf_len)
{
	return -EOPNOTSUPP;
}

static RADIX_TREE(nfit_radix, GFP_KERNEL);

static void free_coherent(dma_addr_t dma)
{
	void *v = radix_tree_delete(&nfit_radix, dma >> PAGE_SHIFT);

	WARN(v == NULL, "%s: unknown dma address %pad\n", __func__, &dma);
}

static void *alloc_coherent(struct device *dev, size_t size, dma_addr_t *dma)
{
	void *buf = dmam_alloc_coherent(dev, size, dma, GFP_KERNEL);

	if (!buf)
		return NULL;
	memset(buf, 0, size);
	WARN(*dma & ~PAGE_MASK, "%pad not page aligned\n", dma);
	if (radix_tree_insert(&nfit_radix, *dma >> PAGE_SHIFT, buf))
		return NULL;

	return buf;
}

static void __iomem *__nfit_test_ioremap(resource_size_t addr, unsigned long size)
{
	unsigned long key = addr >> PAGE_SHIFT;

	return radix_tree_lookup(&nfit_radix, key);
}

static void free_resources(struct nfit_test *t)
{
	int i;

	free_coherent(t->nfit_dma);
	for (i = 0; i < NUM_DCR; i++) {
		free_coherent(t->dimm_dma[i]);
		free_coherent(t->label_dma[i]);
	}

	for (i = 0; i < NUM_PM; i++)
		free_coherent(t->spa_set_dma[i]);

	for (i = 0; i < NUM_DCR; i++)
		free_coherent(t->dcr_dma[i]);
}

static int nfit_test_alloc(struct nfit_test *t)
{
	size_t nfit_size = sizeof(struct nfit)
			+ sizeof(struct nfit_spa) * NUM_SPA
			+ sizeof(struct nfit_mem) * NUM_MEM
			+ sizeof(struct nfit_dcr) * NUM_DCR
			+ sizeof(struct nfit_bdw) * NUM_BDW;
	struct device *dev = &t->pdev.dev;
	int i;

	t->nfit_buf = alloc_coherent(dev, nfit_size, &t->nfit_dma);
	if (!t->nfit_buf)
		return -ENOMEM;
	t->nfit_size = nfit_size;

	t->spa_set[0] = alloc_coherent(dev, SPA0_SIZE, &t->spa_set_dma[0]);
	if (!t->spa_set[0])
		return -ENOMEM;

	t->spa_set[1] = alloc_coherent(dev, SPA1_SIZE, &t->spa_set_dma[1]);
	if (!t->spa_set[1])
		return -ENOMEM;

	for (i = 0; i < NUM_DCR; i++) {
		t->dimm[i] = alloc_coherent(dev, DIMM_SIZE, &t->dimm_dma[i]);
		if (!t->dimm[i])
			return -ENOMEM;

		t->label[i] = alloc_coherent(dev, LABEL_SIZE, &t->label_dma[i]);
		if (!t->label[i])
			return -ENOMEM;
	}

	for (i = 0; i < NUM_DCR; i++) {
		t->dcr[i] = alloc_coherent(dev, LABEL_SIZE, &t->dcr_dma[i]);
		if (!t->dcr[i])
			return -ENOMEM;
	}

	return 0;
}

static void nfit_test_setup(struct nfit_test *t)
{
	void __iomem *nfit_buf = t->nfit_buf;
	struct nfit_spa __iomem *nfit_spa;
	struct nfit_dcr __iomem *nfit_dcr;
	struct nfit_bdw __iomem *nfit_bdw;
	struct nfit_mem __iomem *nfit_mem;
	size_t i, size = t->nfit_size;
	struct nfit __iomem *nfit;
	unsigned int offset;
	u8 sum, *data;

	/* nfit header */
	nfit = nfit_buf;
	memcpy_toio(nfit->signature, "NFIT", 4);
	writel(size, &nfit->length);
	writeb(1, &nfit->revision);
	memcpy_toio(nfit->oemid, "NDTEST", 6);
	writew(0x1234, &nfit->oem_tbl_id);
	writel(1, &nfit->oem_revision);
	writel(0x80860000, &nfit->creator_id);
	writel(1, &nfit->creator_revision);

	/*
	 * spa0 (interleave first half of dimm-000:0000 and
	 * dimm-000:0001, note storage does not actually alias the
	 * related block-data-window regions)
	 */
	nfit_spa = nfit_buf + sizeof(*nfit);
	writew(NFIT_TABLE_SPA, &nfit_spa->type);
	writew(sizeof(*nfit_spa), &nfit_spa->length);
	writew(NFIT_SPA_PM, &nfit_spa->spa_type);
	writew(0+1, &nfit_spa->spa_index);
	writeq(t->spa_set_dma[0], &nfit_spa->spa_base);
	writeq(SPA0_SIZE, &nfit_spa->spa_length);

	/*
	 * spa1 (interleave last half of the 4 DIMMS, note storage
	 * does not actually alias the related block-data-window
	 * regions)
	 */
	nfit_spa = nfit_buf + sizeof(*nfit) + sizeof(*nfit_spa);
	writew(NFIT_TABLE_SPA, &nfit_spa->type);
	writew(sizeof(*nfit_spa), &nfit_spa->length);
	writew(NFIT_SPA_PM, &nfit_spa->spa_type);
	writew(1+1, &nfit_spa->spa_index);
	writeq(t->spa_set_dma[1], &nfit_spa->spa_base);
	writeq(SPA1_SIZE, &nfit_spa->spa_length);

	/* spa2 (dcr2) dimm-000:0000 */
	nfit_spa = nfit_buf + sizeof(*nfit) + sizeof(*nfit_spa) * 2;
	writew(NFIT_TABLE_SPA, &nfit_spa->type);
	writew(sizeof(*nfit_spa), &nfit_spa->length);
	writew(NFIT_SPA_DCR, &nfit_spa->spa_type);
	writew(2+1, &nfit_spa->spa_index);
	writeq(t->dcr_dma[0], &nfit_spa->spa_base);
	writeq(DCR_SIZE, &nfit_spa->spa_length);

	/* spa3 (dcr3) dimm-000:0001 */
	nfit_spa = nfit_buf + sizeof(*nfit) + sizeof(*nfit_spa) * 3;
	writew(NFIT_TABLE_SPA, &nfit_spa->type);
	writew(sizeof(*nfit_spa), &nfit_spa->length);
	writew(NFIT_SPA_DCR, &nfit_spa->spa_type);
	writew(3+1, &nfit_spa->spa_index);
	writeq(t->dcr_dma[1], &nfit_spa->spa_base);
	writeq(DCR_SIZE, &nfit_spa->spa_length);

	/* spa4 (dcr4) dimm-000:0100 */
	nfit_spa = nfit_buf + sizeof(*nfit) + sizeof(*nfit_spa) * 4;
	writew(NFIT_TABLE_SPA, &nfit_spa->type);
	writew(sizeof(*nfit_spa), &nfit_spa->length);
	writew(NFIT_SPA_DCR, &nfit_spa->spa_type);
	writew(4+1, &nfit_spa->spa_index);
	writeq(t->dcr_dma[2], &nfit_spa->spa_base);
	writeq(DCR_SIZE, &nfit_spa->spa_length);

	/* spa5 (dcr5) dimm-000:0101 */
	nfit_spa = nfit_buf + sizeof(*nfit) + sizeof(*nfit_spa) * 5;
	writew(NFIT_TABLE_SPA, &nfit_spa->type);
	writew(sizeof(*nfit_spa), &nfit_spa->length);
	writew(NFIT_SPA_DCR, &nfit_spa->spa_type);
	writew(5+1, &nfit_spa->spa_index);
	writeq(t->dcr_dma[3], &nfit_spa->spa_base);
	writeq(DCR_SIZE, &nfit_spa->spa_length);

	/* spa6 (bdw for dcr2) dimm-000:0000 */
	nfit_spa = nfit_buf + sizeof(*nfit) + sizeof(*nfit_spa) * 6;
	writew(NFIT_TABLE_SPA, &nfit_spa->type);
	writew(sizeof(*nfit_spa), &nfit_spa->length);
	writew(NFIT_SPA_BDW, &nfit_spa->spa_type);
	writew(6+1, &nfit_spa->spa_index);
	writeq(t->dcr_dma[0] + DCR_SIZE, &nfit_spa->spa_base);
	writeq(BDW_SIZE, &nfit_spa->spa_length);

	/* spa7 (bdw for dcr3) dimm-000:0001 */
	nfit_spa = nfit_buf + sizeof(*nfit) + sizeof(*nfit_spa) * 7;
	writew(NFIT_TABLE_SPA, &nfit_spa->type);
	writew(sizeof(*nfit_spa), &nfit_spa->length);
	writew(NFIT_SPA_BDW, &nfit_spa->spa_type);
	writew(7+1, &nfit_spa->spa_index);
	writeq(t->dcr_dma[1] + DCR_SIZE, &nfit_spa->spa_base);
	writeq(BDW_SIZE, &nfit_spa->spa_length);

	/* spa8 (bdw for dcr4) dimm-000:0100 */
	nfit_spa = nfit_buf + sizeof(*nfit) + sizeof(*nfit_spa) * 8;
	writew(NFIT_TABLE_SPA, &nfit_spa->type);
	writew(sizeof(*nfit_spa), &nfit_spa->length);
	writew(NFIT_SPA_BDW, &nfit_spa->spa_type);
	writew(8+1, &nfit_spa->spa_index);
	writeq(t->dcr_dma[2] + DCR_SIZE, &nfit_spa->spa_base);
	writeq(BDW_SIZE, &nfit_spa->spa_length);

	/* spa9 (bdw for dcr5) dimm-000:0101 */
	nfit_spa = nfit_buf + sizeof(*nfit) + sizeof(*nfit_spa) * 9;
	writew(NFIT_TABLE_SPA, &nfit_spa->type);
	writew(sizeof(*nfit_spa), &nfit_spa->length);
	writew(NFIT_SPA_BDW, &nfit_spa->spa_type);
	writew(9+1, &nfit_spa->spa_index);
	writeq(t->dcr_dma[3] + DCR_SIZE, &nfit_spa->spa_base);
	writeq(BDW_SIZE, &nfit_spa->spa_length);

	offset = sizeof(*nfit) + sizeof(*nfit_spa) * 10;
	/* mem-region0 (spa0, dimm-000:0000) */
	nfit_mem = nfit_buf + offset;
	writew(NFIT_TABLE_MEM, &nfit_mem->type);
	writew(sizeof(*nfit_mem), &nfit_mem->length);
	writel(handle[0], &nfit_mem->nfit_handle);
	writew(0, &nfit_mem->phys_id);
	writew(0, &nfit_mem->region_id);
	writew(0+1, &nfit_mem->spa_index);
	writew(0, &nfit_mem->dcr_index);
	writeq(SPA0_SIZE/2, &nfit_mem->region_len);
	writeq(0, &nfit_mem->region_offset);
	writeq(t->spa_set_dma[0], &nfit_mem->region_spa);
	writew(0, &nfit_mem->idt_index);
	writew(2, &nfit_mem->interleave_ways);

	/* mem-region1 (spa0, dimm-000:0001) */
	nfit_mem = nfit_buf + offset + sizeof(struct nfit_mem);
	writew(NFIT_TABLE_MEM, &nfit_mem->type);
	writew(sizeof(*nfit_mem), &nfit_mem->length);
	writel(handle[1], &nfit_mem->nfit_handle);
	writew(1, &nfit_mem->phys_id);
	writew(0, &nfit_mem->region_id);
	writew(0+1, &nfit_mem->spa_index);
	writew(0, &nfit_mem->dcr_index);
	writeq(SPA0_SIZE/2, &nfit_mem->region_len);
	writeq(0, &nfit_mem->region_offset);
	writeq(t->spa_set_dma[0] + SPA0_SIZE/2, &nfit_mem->region_spa);
	writew(0, &nfit_mem->idt_index);
	writew(2, &nfit_mem->interleave_ways);

	/* mem-region2 (spa1, dimm-000:0000) */
	nfit_mem = nfit_buf + offset + sizeof(struct nfit_mem) * 2;
	writew(NFIT_TABLE_MEM, &nfit_mem->type);
	writew(sizeof(*nfit_mem), &nfit_mem->length);
	writel(handle[0], &nfit_mem->nfit_handle);
	writew(0, &nfit_mem->phys_id);
	writew(1, &nfit_mem->region_id);
	writew(1+1, &nfit_mem->spa_index);
	writew(0, &nfit_mem->dcr_index);
	writeq(SPA1_SIZE/4, &nfit_mem->region_len);
	writeq(SPA0_SIZE/2, &nfit_mem->region_offset);
	writeq(t->spa_set_dma[1], &nfit_mem->region_spa);
	writew(0, &nfit_mem->idt_index);
	writew(4, &nfit_mem->interleave_ways);

	/* mem-region3 (spa1, dimm-000:0001) */
	nfit_mem = nfit_buf + offset + sizeof(struct nfit_mem) * 3;
	writew(NFIT_TABLE_MEM, &nfit_mem->type);
	writew(sizeof(*nfit_mem), &nfit_mem->length);
	writel(handle[1], &nfit_mem->nfit_handle);
	writew(1, &nfit_mem->phys_id);
	writew(1, &nfit_mem->region_id);
	writew(1+1, &nfit_mem->spa_index);
	writew(0, &nfit_mem->dcr_index);
	writeq(SPA1_SIZE/4, &nfit_mem->region_len);
	writeq(SPA0_SIZE/2, &nfit_mem->region_offset);
	writeq(t->spa_set_dma[1] + SPA1_SIZE/4, &nfit_mem->region_spa);
	writew(0, &nfit_mem->idt_index);
	writew(4, &nfit_mem->interleave_ways);

	/* mem-region4 (spa1, dimm-000:0100) */
	nfit_mem = nfit_buf + offset + sizeof(struct nfit_mem) * 4;
	writew(NFIT_TABLE_MEM, &nfit_mem->type);
	writew(sizeof(*nfit_mem), &nfit_mem->length);
	writel(handle[2], &nfit_mem->nfit_handle);
	writew(2, &nfit_mem->phys_id);
	writew(0, &nfit_mem->region_id);
	writew(1+1, &nfit_mem->spa_index);
	writew(0, &nfit_mem->dcr_index);
	writeq(SPA1_SIZE/4, &nfit_mem->region_len);
	writeq(SPA0_SIZE/2, &nfit_mem->region_offset);
	writeq(t->spa_set_dma[1] + 2*SPA1_SIZE/4, &nfit_mem->region_spa);
	writew(0, &nfit_mem->idt_index);
	writew(4, &nfit_mem->interleave_ways);

	/* mem-region5 (spa1, dimm-000:0101) */
	nfit_mem = nfit_buf + offset + sizeof(struct nfit_mem) * 5;
	writew(NFIT_TABLE_MEM, &nfit_mem->type);
	writew(sizeof(*nfit_mem), &nfit_mem->length);
	writel(handle[3], &nfit_mem->nfit_handle);
	writew(3, &nfit_mem->phys_id);
	writew(0, &nfit_mem->region_id);
	writew(1+1, &nfit_mem->spa_index);
	writew(0, &nfit_mem->dcr_index);
	writeq(SPA1_SIZE/4, &nfit_mem->region_len);
	writeq(SPA0_SIZE/2, &nfit_mem->region_offset);
	writeq(t->spa_set_dma[1] + 3*SPA1_SIZE/4, &nfit_mem->region_spa);
	writew(0, &nfit_mem->idt_index);
	writew(4, &nfit_mem->interleave_ways);

	/* mem-region6 (spa/dcr2, dimm-000:0000) */
	nfit_mem = nfit_buf + offset + sizeof(struct nfit_mem) * 6;
	writew(NFIT_TABLE_MEM, &nfit_mem->type);
	writew(sizeof(*nfit_mem), &nfit_mem->length);
	writel(handle[0], &nfit_mem->nfit_handle);
	writew(0, &nfit_mem->phys_id);
	writew(0, &nfit_mem->region_id);
	writew(2+1, &nfit_mem->spa_index);
	writew(0+1, &nfit_mem->dcr_index);
	writeq(0, &nfit_mem->region_len);
	writeq(0, &nfit_mem->region_offset);
	writeq(0, &nfit_mem->region_spa);
	writew(0, &nfit_mem->idt_index);
	writew(1, &nfit_mem->interleave_ways);

	/* mem-region7 (spa/dcr3, dimm-000:0001) */
	nfit_mem = nfit_buf + offset + sizeof(struct nfit_mem) * 7;
	writew(NFIT_TABLE_MEM, &nfit_mem->type);
	writew(sizeof(*nfit_mem), &nfit_mem->length);
	writel(handle[1], &nfit_mem->nfit_handle);
	writew(1, &nfit_mem->phys_id);
	writew(0, &nfit_mem->region_id);
	writew(3+1, &nfit_mem->spa_index);
	writew(1+1, &nfit_mem->dcr_index);
	writeq(0, &nfit_mem->region_len);
	writeq(0, &nfit_mem->region_offset);
	writeq(0, &nfit_mem->region_spa);
	writew(0, &nfit_mem->idt_index);
	writew(1, &nfit_mem->interleave_ways);

	/* mem-region8 (spa/dcr4, dimm-000:0100) */
	nfit_mem = nfit_buf + offset + sizeof(struct nfit_mem) * 8;
	writew(NFIT_TABLE_MEM, &nfit_mem->type);
	writew(sizeof(*nfit_mem), &nfit_mem->length);
	writel(handle[2], &nfit_mem->nfit_handle);
	writew(2, &nfit_mem->phys_id);
	writew(0, &nfit_mem->region_id);
	writew(4+1, &nfit_mem->spa_index);
	writew(2+1, &nfit_mem->dcr_index);
	writeq(0, &nfit_mem->region_len);
	writeq(0, &nfit_mem->region_offset);
	writeq(0, &nfit_mem->region_spa);
	writew(0, &nfit_mem->idt_index);
	writew(1, &nfit_mem->interleave_ways);

	/* mem-region9 (spa/dcr5, dimm-000:0101) */
	nfit_mem = nfit_buf + offset + sizeof(struct nfit_mem) * 9;
	writew(NFIT_TABLE_MEM, &nfit_mem->type);
	writew(sizeof(*nfit_mem), &nfit_mem->length);
	writel(handle[3], &nfit_mem->nfit_handle);
	writew(3, &nfit_mem->phys_id);
	writew(0, &nfit_mem->region_id);
	writew(5+1, &nfit_mem->spa_index);
	writew(3+1, &nfit_mem->dcr_index);
	writeq(0, &nfit_mem->region_len);
	writeq(0, &nfit_mem->region_offset);
	writeq(0, &nfit_mem->region_spa);
	writew(0, &nfit_mem->idt_index);
	writew(1, &nfit_mem->interleave_ways);

	offset = offset + sizeof(struct nfit_mem) * 10;
	/* dcr-descriptor0 */
	nfit_dcr = nfit_buf + offset;
	writew(NFIT_TABLE_DCR, &nfit_dcr->type);
	writew(sizeof(struct nfit_dcr), &nfit_dcr->length);
	writew(0+1, &nfit_dcr->dcr_index);
	writew(0x8086, &nfit_dcr->vendor_id);
	writew(0, &nfit_dcr->device_id);
	writew(1, &nfit_dcr->revsion_id);
	writew(1, &nfit_dcr->fic);
	writew(1, &nfit_dcr->num_bdw);
	writeq(DCR_SIZE, &nfit_dcr->dcr_size);
	writeq(0, &nfit_dcr->cmd_offset);
	writeq(8, &nfit_dcr->cmd_size);
	writeq(8, &nfit_dcr->status_offset);
	writeq(4, &nfit_dcr->status_size);

	/* dcr-descriptor1 */
	nfit_dcr = nfit_buf + offset + sizeof(struct nfit_dcr);
	writew(NFIT_TABLE_DCR, &nfit_dcr->type);
	writew(sizeof(struct nfit_dcr), &nfit_dcr->length);
	writew(1+1, &nfit_dcr->dcr_index);
	writew(0x8086, &nfit_dcr->vendor_id);
	writew(0, &nfit_dcr->device_id);
	writew(1, &nfit_dcr->revsion_id);
	writew(1, &nfit_dcr->fic);
	writew(1, &nfit_dcr->num_bdw);
	writeq(DCR_SIZE, &nfit_dcr->dcr_size);
	writeq(0, &nfit_dcr->cmd_offset);
	writeq(8, &nfit_dcr->cmd_size);
	writeq(8, &nfit_dcr->status_offset);
	writeq(4, &nfit_dcr->status_size);

	/* dcr-descriptor2 */
	nfit_dcr = nfit_buf + offset + sizeof(struct nfit_dcr) * 2;
	writew(NFIT_TABLE_DCR, &nfit_dcr->type);
	writew(sizeof(struct nfit_dcr), &nfit_dcr->length);
	writew(2+1, &nfit_dcr->dcr_index);
	writew(0x8086, &nfit_dcr->vendor_id);
	writew(0, &nfit_dcr->device_id);
	writew(1, &nfit_dcr->revsion_id);
	writew(1, &nfit_dcr->fic);
	writew(1, &nfit_dcr->num_bdw);
	writeq(DCR_SIZE, &nfit_dcr->dcr_size);
	writeq(0, &nfit_dcr->cmd_offset);
	writeq(8, &nfit_dcr->cmd_size);
	writeq(8, &nfit_dcr->status_offset);
	writeq(4, &nfit_dcr->status_size);

	/* dcr-descriptor3 */
	nfit_dcr = nfit_buf + offset + sizeof(struct nfit_dcr) * 3;
	writew(NFIT_TABLE_DCR, &nfit_dcr->type);
	writew(sizeof(struct nfit_dcr), &nfit_dcr->length);
	writew(3+1, &nfit_dcr->dcr_index);
	writew(0x8086, &nfit_dcr->vendor_id);
	writew(0, &nfit_dcr->device_id);
	writew(1, &nfit_dcr->revsion_id);
	writew(1, &nfit_dcr->fic);
	writew(1, &nfit_dcr->num_bdw);
	writeq(DCR_SIZE, &nfit_dcr->dcr_size);
	writeq(0, &nfit_dcr->cmd_offset);
	writeq(8, &nfit_dcr->cmd_size);
	writeq(8, &nfit_dcr->status_offset);
	writeq(4, &nfit_dcr->status_size);

	offset = offset + sizeof(struct nfit_dcr) * 4;
	/* bdw0 (spa/dcr2, dimm-000:0000) */
	nfit_bdw = nfit_buf + offset;
	writew(NFIT_TABLE_BDW, &nfit_bdw->type);
	writew(sizeof(struct nfit_bdw), &nfit_bdw->length);
	writew(0+1, &nfit_bdw->dcr_index);
	writew(1, &nfit_bdw->num_bdw);
	writeq(0, &nfit_bdw->bdw_offset);
	writeq(BDW_SIZE, &nfit_bdw->bdw_size);
	writeq(DIMM_SIZE, &nfit_bdw->dimm_capacity);
	writeq(0, &nfit_bdw->dimm_block_offset);

	/* bdw1 (spa/dcr3, dimm-000:0001) */
	nfit_bdw = nfit_buf + offset + sizeof(struct nfit_bdw);
	writew(NFIT_TABLE_BDW, &nfit_bdw->type);
	writew(sizeof(struct nfit_bdw), &nfit_bdw->length);
	writew(1+1, &nfit_bdw->dcr_index);
	writew(1, &nfit_bdw->num_bdw);
	writeq(0, &nfit_bdw->bdw_offset);
	writeq(BDW_SIZE, &nfit_bdw->bdw_size);
	writeq(DIMM_SIZE, &nfit_bdw->dimm_capacity);
	writeq(0, &nfit_bdw->dimm_block_offset);

	/* bdw2 (spa/dcr4, dimm-000:0100) */
	nfit_bdw = nfit_buf + offset + sizeof(struct nfit_bdw) * 2;
	writew(NFIT_TABLE_BDW, &nfit_bdw->type);
	writew(sizeof(struct nfit_bdw), &nfit_bdw->length);
	writew(2+1, &nfit_bdw->dcr_index);
	writew(1, &nfit_bdw->num_bdw);
	writeq(0, &nfit_bdw->bdw_offset);
	writeq(BDW_SIZE, &nfit_bdw->bdw_size);
	writeq(DIMM_SIZE, &nfit_bdw->dimm_capacity);
	writeq(0, &nfit_bdw->dimm_block_offset);

	/* bdw3 (spa/dcr5, dimm-000:0101) */
	nfit_bdw = nfit_buf + offset + sizeof(struct nfit_bdw) * 3;
	writew(NFIT_TABLE_BDW, &nfit_bdw->type);
	writew(sizeof(struct nfit_bdw), &nfit_bdw->length);
	writew(3+1, &nfit_bdw->dcr_index);
	writew(1, &nfit_bdw->num_bdw);
	writeq(0, &nfit_bdw->bdw_offset);
	writeq(BDW_SIZE, &nfit_bdw->bdw_size);
	writeq(DIMM_SIZE, &nfit_bdw->dimm_capacity);
	writeq(0, &nfit_bdw->dimm_block_offset);

	data = (u8 *) nfit_buf;
	for (sum = 0, i = 0; i < size; i++)
		sum += data[i];
	writeb(sum, &nfit->checksum);
}

static int nfit_test_probe(struct platform_device *pdev)
{
	struct nfit_bus_descriptor *nfit_desc;
	struct device *dev = &pdev->dev;
	struct nfit_test *nfit_test;
	resource_size_t start;

	nfit_test = to_nfit_test(&pdev->dev);
	if (dma_coerce_mask_and_coherent(dev, DMA_BIT_MASK(64)))
		goto err;

	if (nfit_test_alloc(nfit_test))
		goto err;

	nfit_test_setup(nfit_test);

	start = dma_to_phys(dev, nfit_test->nfit_dma);
	nfit_desc = &nfit_test->nfit_desc;
	nfit_desc->nfit_start = start;
	nfit_desc->nfit_size = nfit_test->nfit_size;
	nfit_desc->nfit_ctl = nfit_test_ctl;
	set_bit(NFIT_FLAG_FIC1_CAP, &nfit_desc->flags);

	nfit_test->nd_bus = nfit_bus_register(&pdev->dev, nfit_desc);
	if (!nfit_test->nd_bus)
		goto err;

	return 0;

 err:
	free_resources(nfit_test);
	return -EINVAL;
}

static int nfit_test_remove(struct platform_device *pdev)
{
	struct nfit_test *nfit_test = to_nfit_test(&pdev->dev);

	nfit_bus_unregister(nfit_test->nd_bus);

	return 0;
}

static void nfit_test_release(struct device *dev)
{
	struct nfit_test *nfit_test = to_nfit_test(dev);

	free_resources(nfit_test);
}

static const struct platform_device_id nfit_test_id[] = {
	{ KBUILD_MODNAME },
	{ },
};

static struct platform_driver nfit_test_driver = {
	.probe = nfit_test_probe,
	.remove = nfit_test_remove,
	.driver = {
		.name = KBUILD_MODNAME,
		.owner = THIS_MODULE,
	},
	.id_table = nfit_test_id,
};

static struct nfit_test *instances[NUM_NFITS];

extern void __iomem *(*nfit_test_ioremap)(resource_size_t, unsigned long);
static __init int nfit_test_init(void)
{
	int rc, i;

	nfit_test_ioremap = __nfit_test_ioremap;

	for (i = 0; i < NUM_NFITS; i++) {
		struct nfit_test *nfit_test;
		struct platform_device *pdev;

		nfit_test = kzalloc(sizeof(*nfit_test), GFP_KERNEL);
		if (!nfit_test) {
			rc = -ENOMEM;
			goto err_register;
		}
		pdev = &nfit_test->pdev;
		pdev->name = KBUILD_MODNAME;
		pdev->id = i;
		pdev->dev.release = nfit_test_release;
		rc = platform_device_register(pdev);
		if (rc) {
			kfree(nfit_test);
			goto err_register;
		}
		instances[i] = nfit_test;
	}

	rc = platform_driver_register(&nfit_test_driver);
	if (rc)
		goto err_register;
	return 0;

 err_register:
	for (i = 0; i < NUM_NFITS; i++)
		if (instances[i])
			platform_device_unregister(&instances[i]->pdev);
	return rc;
}

static __exit void nfit_test_exit(void)
{
	int i;

	nfit_test_ioremap = NULL;
	for (i = 0; i < NUM_NFITS; i++)
		platform_device_unregister(&instances[i]->pdev);
	platform_driver_unregister(&nfit_test_driver);
}

module_init(nfit_test_init);
module_exit(nfit_test_exit);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Intel Corporation");
