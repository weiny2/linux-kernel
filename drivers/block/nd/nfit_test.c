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
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/radix-tree.h>
#include <linux/module.h>
#include <linux/sizes.h>
#include <linux/slab.h>
#include "nfit.h"
#include "dsm.h"

/*
 * Generate an NFIT table to describe the following topology:
 *
 * BUS0: Interleaved PMEM regions, and aliasing with BLOCK regions
 *
 *                              (a)               (b)           DIMM   BLK-REGION
 *           +-------------------+--------+--------+--------+
 * +------+  |       pm0.0       | blk2.0 | pm1.0  | blk2.1 |    0      region2
 * | imc0 +--+- - - region0- - - +--------+        +--------+
 * +--+---+  |       pm0.0       | blk3.0 | pm1.0  | blk3.1 |    1      region3
 *    |      +-------------------+--------v        v--------+
 * +--+---+                               |                 |
 * | cpu0 |                                     region1
 * +--+---+                               |                 |
 *    |      +----------------------------^        ^--------+
 * +--+---+  |           blk4.0           | pm1.0  | blk4.0 |    2      region4
 * | imc1 +--+----------------------------|        +--------+
 * +------+  |           blk5.0           | pm1.0  | blk5.0 |    3      region5
 *           +----------------------------+--------+--------+
 *
 * *) In this layout we have four dimms and two memory controllers in one
 *    socket.  Each unique interface ("block" or "pmem") to DPA space
 *    is identified by a region device with a dynamically assigned id.
 *
 * *) The first portion of dimm0 and dimm1 are interleaved as REGION0.
 *    A single "pmem" namespace is created in the REGION0-"spa"-range
 *    that spans dimm0 and dimm1 with a user-specified name of "pm0.0".
 *    Some of that interleaved "spa" range is reclaimed as "bdw"
 *    accessed space starting at offset (a) into each dimm.  In that
 *    reclaimed space we create two "bdw" "namespaces" from REGION2 and
 *    REGION3 where "blk2.0" and "blk3.0" are just human readable names
 *    that could be set to any user-desired name in the label.
 *
 * *) In the last portion of dimm0 and dimm1 we have an interleaved
 *    "spa" range, REGION1, that spans those two dimms as well as dimm2
 *    and dimm3.  Some of REGION1 allocated to a "pmem" namespace named
 *    "pm1.0" the rest is reclaimed in 4 "bdw" namespaces (for each
 *    dimm in the interleave set), "blk2.1", "blk3.1", "blk4.0", and
 *    "blk5.0".
 *
 * *) The portion of dimm2 and dimm3 that do not participate in the
 *    REGION1 interleaved "spa" range (i.e. the DPA address below
 *    offset (b) are also included in the "blk4.0" and "blk5.0"
 *    namespaces.  Note, that this example shows that "bdw" namespaces
 *    don't need to be contiguous in DPA-space.
 *
 * BUS1: Legacy NVDIMM (single contiguous range)
 *
 *  region2
 * +---------------------+
 * |---------------------|
 * ||       pm2.0       ||
 * |---------------------|
 * +---------------------+
 *
 * *) A NFIT-table may describe a simple system-physical-address range
 *    with no backing dimm or interleave description.
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
	SPA2_SIZE = SZ_512M,
	BDW_SIZE = 64 << 8,
	DCR_SIZE = 12,
	NUM_NFITS = 2, /* permit testing multiple NFITs per system */
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
	int num_dcr;
	int num_pm;
	void **dimm;
	dma_addr_t *dimm_dma;
	void **label;
	dma_addr_t *label_dma;
	void **spa_set;
	dma_addr_t *spa_set_dma;
	struct nfit_test_dcr **dcr;
	dma_addr_t *dcr_dma;
	int (*alloc)(struct nfit_test *t);
	void (*setup)(struct nfit_test *t);
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

struct nfit_test_resource {
	struct resource *res;
	void *buf;
};

static void free_coherent(dma_addr_t dma)
{
	void *v = radix_tree_delete(&nfit_radix, dma >> PAGE_SHIFT);

	WARN(v == NULL, "%s: unknown dma address %pad\n", __func__, &dma);
}

static struct nfit_test_resource *__alloc_coherent(struct device *dev,
		size_t size, dma_addr_t *dma)
{
	void *buf = dmam_alloc_coherent(dev, size, dma, GFP_KERNEL);
	struct nfit_test_resource *nfit_res;

	if (!buf)
		return NULL;
	nfit_res = devm_kzalloc(dev, sizeof(*nfit_res), GFP_KERNEL);
	if (!nfit_res)
		return NULL;
	memset(buf, 0, size);
	WARN(*dma & ~PAGE_MASK, "%pad not page aligned\n", dma);
	if (radix_tree_insert(&nfit_radix, *dma >> PAGE_SHIFT, nfit_res))
		return NULL;
	nfit_res->buf = buf;

	return nfit_res;
}

static void *alloc_coherent(struct device *dev, size_t size, dma_addr_t *dma)
{
	struct nfit_test_resource *nfit_res = __alloc_coherent(dev, size, dma);

	return nfit_res ? nfit_res->buf : NULL;
}

static void *alloc_coherent_res(struct device *dev, size_t size, dma_addr_t *dma)
{
	struct resource *res = devm_kzalloc(dev, sizeof(*res), GFP_KERNEL);
	struct nfit_test_resource *nfit_res;

	if (!res)
		return NULL;
	nfit_res = __alloc_coherent(dev, size, dma);
	if (!nfit_res)
		return NULL;

	nfit_res->res = res;
	res->start = *dma;
	res->end = *dma + size - 1;
	res->name = "NFIT";

	return nfit_res->buf;
}

static void __iomem *__nfit_test_ioremap(resource_size_t addr, unsigned long size)
{
	unsigned long key = addr >> PAGE_SHIFT;
	struct nfit_test_resource *nfit_res;

	nfit_res = radix_tree_lookup(&nfit_radix, key);
	return nfit_res->buf;
}

static struct resource *__nfit_test_request_region(resource_size_t addr,
		resource_size_t n)
{
	unsigned long key = addr >> PAGE_SHIFT;
	struct nfit_test_resource *nfit_res;
	struct resource *res;

	nfit_res = radix_tree_lookup(&nfit_radix, key);
	res = nfit_res ? nfit_res->res : NULL;
	if (res)
		res->end = res->start + n;
	return res;
}

static void free_resources(struct nfit_test *t)
{
	int i;

	if (t->nfit_buf)
		free_coherent(t->nfit_dma);
	for (i = 0; i < t->num_dcr; i++) {
		if (t->dimm_dma)
			free_coherent(t->dimm_dma[i]);
		if (t->label_dma)
			free_coherent(t->label_dma[i]);
	}

	for (i = 0; i < t->num_pm; i++)
		if (t->spa_set_dma)
			free_coherent(t->spa_set_dma[i]);

	for (i = 0; i < t->num_dcr; i++)
		if (t->dcr_dma)
			free_coherent(t->dcr_dma[i]);
}

static int nfit_test0_alloc(struct nfit_test *t)
{
	size_t nfit_size = sizeof(struct nfit)
			+ sizeof(struct nfit_spa) * NUM_SPA
			+ sizeof(struct nfit_mem) * NUM_MEM
			+ sizeof(struct nfit_dcr) * NUM_DCR
			+ sizeof(struct nfit_bdw) * NUM_BDW;
	struct device *dev = &t->pdev.dev;
	int i;

	t->nfit_buf = alloc_coherent_res(dev, nfit_size, &t->nfit_dma);
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

static u8 nfit_checksum(void *buf, size_t size)
{
	u8 sum, *data = buf;
	size_t i;

	for (sum = 0, i = 0; i < size; i++)
		sum += data[i];
	return 0 - sum;
}

static int nfit_test1_alloc(struct nfit_test *t)
{
	size_t nfit_size = sizeof(struct nfit) + sizeof(struct nfit_spa);
	struct device *dev = &t->pdev.dev;

	t->nfit_buf = alloc_coherent_res(dev, nfit_size, &t->nfit_dma);
	if (!t->nfit_buf)
		return -ENOMEM;
	t->nfit_size = nfit_size;

	t->spa_set[0] = alloc_coherent_res(dev, SPA2_SIZE, &t->spa_set_dma[0]);
	if (!t->spa_set[0])
		return -ENOMEM;

	return 0;
}

static void nfit_test0_setup(struct nfit_test *t)
{
	struct nfit_bus_descriptor *nfit_desc;
	void __iomem *nfit_buf = t->nfit_buf;
	struct nfit_spa __iomem *nfit_spa;
	struct nfit_dcr __iomem *nfit_dcr;
	struct nfit_bdw __iomem *nfit_bdw;
	struct nfit_mem __iomem *nfit_mem;
	size_t size = t->nfit_size;
	struct nfit __iomem *nfit;
	unsigned int offset;

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
	 * spa0 (interleave first half of dimm0 and dimm1, note storage
	 * does not actually alias the related block-data-window
	 * regions)
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

	/* spa2 (dcr0) dimm0 */
	nfit_spa = nfit_buf + sizeof(*nfit) + sizeof(*nfit_spa) * 2;
	writew(NFIT_TABLE_SPA, &nfit_spa->type);
	writew(sizeof(*nfit_spa), &nfit_spa->length);
	writew(NFIT_SPA_DCR, &nfit_spa->spa_type);
	writew(2+1, &nfit_spa->spa_index);
	writeq(t->dcr_dma[0], &nfit_spa->spa_base);
	writeq(DCR_SIZE, &nfit_spa->spa_length);

	/* spa3 (dcr1) dimm1 */
	nfit_spa = nfit_buf + sizeof(*nfit) + sizeof(*nfit_spa) * 3;
	writew(NFIT_TABLE_SPA, &nfit_spa->type);
	writew(sizeof(*nfit_spa), &nfit_spa->length);
	writew(NFIT_SPA_DCR, &nfit_spa->spa_type);
	writew(3+1, &nfit_spa->spa_index);
	writeq(t->dcr_dma[1], &nfit_spa->spa_base);
	writeq(DCR_SIZE, &nfit_spa->spa_length);

	/* spa4 (dcr2) dimm2 */
	nfit_spa = nfit_buf + sizeof(*nfit) + sizeof(*nfit_spa) * 4;
	writew(NFIT_TABLE_SPA, &nfit_spa->type);
	writew(sizeof(*nfit_spa), &nfit_spa->length);
	writew(NFIT_SPA_DCR, &nfit_spa->spa_type);
	writew(4+1, &nfit_spa->spa_index);
	writeq(t->dcr_dma[2], &nfit_spa->spa_base);
	writeq(DCR_SIZE, &nfit_spa->spa_length);

	/* spa5 (dcr3) dimm3 */
	nfit_spa = nfit_buf + sizeof(*nfit) + sizeof(*nfit_spa) * 5;
	writew(NFIT_TABLE_SPA, &nfit_spa->type);
	writew(sizeof(*nfit_spa), &nfit_spa->length);
	writew(NFIT_SPA_DCR, &nfit_spa->spa_type);
	writew(5+1, &nfit_spa->spa_index);
	writeq(t->dcr_dma[3], &nfit_spa->spa_base);
	writeq(DCR_SIZE, &nfit_spa->spa_length);

	/* spa6 (bdw for dcr0) dimm0 */
	nfit_spa = nfit_buf + sizeof(*nfit) + sizeof(*nfit_spa) * 6;
	writew(NFIT_TABLE_SPA, &nfit_spa->type);
	writew(sizeof(*nfit_spa), &nfit_spa->length);
	writew(NFIT_SPA_BDW, &nfit_spa->spa_type);
	writew(6+1, &nfit_spa->spa_index);
	writeq(t->dcr_dma[0] + DCR_SIZE, &nfit_spa->spa_base);
	writeq(BDW_SIZE, &nfit_spa->spa_length);

	/* spa7 (bdw for dcr1) dimm1 */
	nfit_spa = nfit_buf + sizeof(*nfit) + sizeof(*nfit_spa) * 7;
	writew(NFIT_TABLE_SPA, &nfit_spa->type);
	writew(sizeof(*nfit_spa), &nfit_spa->length);
	writew(NFIT_SPA_BDW, &nfit_spa->spa_type);
	writew(7+1, &nfit_spa->spa_index);
	writeq(t->dcr_dma[1] + DCR_SIZE, &nfit_spa->spa_base);
	writeq(BDW_SIZE, &nfit_spa->spa_length);

	/* spa8 (bdw for dcr2) dimm2 */
	nfit_spa = nfit_buf + sizeof(*nfit) + sizeof(*nfit_spa) * 8;
	writew(NFIT_TABLE_SPA, &nfit_spa->type);
	writew(sizeof(*nfit_spa), &nfit_spa->length);
	writew(NFIT_SPA_BDW, &nfit_spa->spa_type);
	writew(8+1, &nfit_spa->spa_index);
	writeq(t->dcr_dma[2] + DCR_SIZE, &nfit_spa->spa_base);
	writeq(BDW_SIZE, &nfit_spa->spa_length);

	/* spa9 (bdw for dcr3) dimm3 */
	nfit_spa = nfit_buf + sizeof(*nfit) + sizeof(*nfit_spa) * 9;
	writew(NFIT_TABLE_SPA, &nfit_spa->type);
	writew(sizeof(*nfit_spa), &nfit_spa->length);
	writew(NFIT_SPA_BDW, &nfit_spa->spa_type);
	writew(9+1, &nfit_spa->spa_index);
	writeq(t->dcr_dma[3] + DCR_SIZE, &nfit_spa->spa_base);
	writeq(BDW_SIZE, &nfit_spa->spa_length);

	offset = sizeof(*nfit) + sizeof(*nfit_spa) * 10;
	/* mem-region0 (spa0, dimm0) */
	nfit_mem = nfit_buf + offset;
	writew(NFIT_TABLE_MEM, &nfit_mem->type);
	writew(sizeof(*nfit_mem), &nfit_mem->length);
	writel(handle[0], &nfit_mem->nfit_handle);
	writew(0, &nfit_mem->phys_id);
	writew(0, &nfit_mem->region_id);
	writew(0+1, &nfit_mem->spa_index);
	writew(0+1, &nfit_mem->dcr_index);
	writeq(SPA0_SIZE/2, &nfit_mem->region_len);
	writeq(0, &nfit_mem->region_offset);
	writeq(t->spa_set_dma[0], &nfit_mem->region_spa);
	writew(0, &nfit_mem->idt_index);
	writew(2, &nfit_mem->interleave_ways);

	/* mem-region1 (spa0, dimm1) */
	nfit_mem = nfit_buf + offset + sizeof(struct nfit_mem);
	writew(NFIT_TABLE_MEM, &nfit_mem->type);
	writew(sizeof(*nfit_mem), &nfit_mem->length);
	writel(handle[1], &nfit_mem->nfit_handle);
	writew(1, &nfit_mem->phys_id);
	writew(0, &nfit_mem->region_id);
	writew(0+1, &nfit_mem->spa_index);
	writew(1+1, &nfit_mem->dcr_index);
	writeq(SPA0_SIZE/2, &nfit_mem->region_len);
	writeq(0, &nfit_mem->region_offset);
	writeq(t->spa_set_dma[0] + SPA0_SIZE/2, &nfit_mem->region_spa);
	writew(0, &nfit_mem->idt_index);
	writew(2, &nfit_mem->interleave_ways);

	/* mem-region2 (spa1, dimm0) */
	nfit_mem = nfit_buf + offset + sizeof(struct nfit_mem) * 2;
	writew(NFIT_TABLE_MEM, &nfit_mem->type);
	writew(sizeof(*nfit_mem), &nfit_mem->length);
	writel(handle[0], &nfit_mem->nfit_handle);
	writew(0, &nfit_mem->phys_id);
	writew(1, &nfit_mem->region_id);
	writew(1+1, &nfit_mem->spa_index);
	writew(0+1, &nfit_mem->dcr_index);
	writeq(SPA1_SIZE/4, &nfit_mem->region_len);
	writeq(SPA0_SIZE/2, &nfit_mem->region_offset);
	writeq(t->spa_set_dma[1], &nfit_mem->region_spa);
	writew(0, &nfit_mem->idt_index);
	writew(4, &nfit_mem->interleave_ways);

	/* mem-region3 (spa1, dimm1) */
	nfit_mem = nfit_buf + offset + sizeof(struct nfit_mem) * 3;
	writew(NFIT_TABLE_MEM, &nfit_mem->type);
	writew(sizeof(*nfit_mem), &nfit_mem->length);
	writel(handle[1], &nfit_mem->nfit_handle);
	writew(1, &nfit_mem->phys_id);
	writew(1, &nfit_mem->region_id);
	writew(1+1, &nfit_mem->spa_index);
	writew(1+1, &nfit_mem->dcr_index);
	writeq(SPA1_SIZE/4, &nfit_mem->region_len);
	writeq(SPA0_SIZE/2, &nfit_mem->region_offset);
	writeq(t->spa_set_dma[1] + SPA1_SIZE/4, &nfit_mem->region_spa);
	writew(0, &nfit_mem->idt_index);
	writew(4, &nfit_mem->interleave_ways);

	/* mem-region4 (spa1, dimm2) */
	nfit_mem = nfit_buf + offset + sizeof(struct nfit_mem) * 4;
	writew(NFIT_TABLE_MEM, &nfit_mem->type);
	writew(sizeof(*nfit_mem), &nfit_mem->length);
	writel(handle[2], &nfit_mem->nfit_handle);
	writew(2, &nfit_mem->phys_id);
	writew(0, &nfit_mem->region_id);
	writew(1+1, &nfit_mem->spa_index);
	writew(2+1, &nfit_mem->dcr_index);
	writeq(SPA1_SIZE/4, &nfit_mem->region_len);
	writeq(SPA0_SIZE/2, &nfit_mem->region_offset);
	writeq(t->spa_set_dma[1] + 2*SPA1_SIZE/4, &nfit_mem->region_spa);
	writew(0, &nfit_mem->idt_index);
	writew(4, &nfit_mem->interleave_ways);

	/* mem-region5 (spa1, dimm3) */
	nfit_mem = nfit_buf + offset + sizeof(struct nfit_mem) * 5;
	writew(NFIT_TABLE_MEM, &nfit_mem->type);
	writew(sizeof(*nfit_mem), &nfit_mem->length);
	writel(handle[3], &nfit_mem->nfit_handle);
	writew(3, &nfit_mem->phys_id);
	writew(0, &nfit_mem->region_id);
	writew(1+1, &nfit_mem->spa_index);
	writew(3+1, &nfit_mem->dcr_index);
	writeq(SPA1_SIZE/4, &nfit_mem->region_len);
	writeq(SPA0_SIZE/2, &nfit_mem->region_offset);
	writeq(t->spa_set_dma[1] + 3*SPA1_SIZE/4, &nfit_mem->region_spa);
	writew(0, &nfit_mem->idt_index);
	writew(4, &nfit_mem->interleave_ways);

	/* mem-region6 (spa/dcr0, dimm0) */
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

	/* mem-region7 (spa/dcr1, dimm1) */
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

	/* mem-region8 (spa/dcr2, dimm2) */
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

	/* mem-region9 (spa/dcr3, dimm3) */
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
	writew(1, &nfit_dcr->revision_id);
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
	writew(1, &nfit_dcr->revision_id);
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
	writew(1, &nfit_dcr->revision_id);
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
	writew(1, &nfit_dcr->revision_id);
	writew(1, &nfit_dcr->fic);
	writew(1, &nfit_dcr->num_bdw);
	writeq(DCR_SIZE, &nfit_dcr->dcr_size);
	writeq(0, &nfit_dcr->cmd_offset);
	writeq(8, &nfit_dcr->cmd_size);
	writeq(8, &nfit_dcr->status_offset);
	writeq(4, &nfit_dcr->status_size);

	offset = offset + sizeof(struct nfit_dcr) * 4;
	/* bdw0 (spa/dcr0, dimm0) */
	nfit_bdw = nfit_buf + offset;
	writew(NFIT_TABLE_BDW, &nfit_bdw->type);
	writew(sizeof(struct nfit_bdw), &nfit_bdw->length);
	writew(0+1, &nfit_bdw->dcr_index);
	writew(1, &nfit_bdw->num_bdw);
	writeq(0, &nfit_bdw->bdw_offset);
	writeq(BDW_SIZE, &nfit_bdw->bdw_size);
	writeq(DIMM_SIZE, &nfit_bdw->dimm_capacity);
	writeq(0, &nfit_bdw->dimm_block_offset);

	/* bdw1 (spa/dcr1, dimm1) */
	nfit_bdw = nfit_buf + offset + sizeof(struct nfit_bdw);
	writew(NFIT_TABLE_BDW, &nfit_bdw->type);
	writew(sizeof(struct nfit_bdw), &nfit_bdw->length);
	writew(1+1, &nfit_bdw->dcr_index);
	writew(1, &nfit_bdw->num_bdw);
	writeq(0, &nfit_bdw->bdw_offset);
	writeq(BDW_SIZE, &nfit_bdw->bdw_size);
	writeq(DIMM_SIZE, &nfit_bdw->dimm_capacity);
	writeq(0, &nfit_bdw->dimm_block_offset);

	/* bdw2 (spa/dcr2, dimm2) */
	nfit_bdw = nfit_buf + offset + sizeof(struct nfit_bdw) * 2;
	writew(NFIT_TABLE_BDW, &nfit_bdw->type);
	writew(sizeof(struct nfit_bdw), &nfit_bdw->length);
	writew(2+1, &nfit_bdw->dcr_index);
	writew(1, &nfit_bdw->num_bdw);
	writeq(0, &nfit_bdw->bdw_offset);
	writeq(BDW_SIZE, &nfit_bdw->bdw_size);
	writeq(DIMM_SIZE, &nfit_bdw->dimm_capacity);
	writeq(0, &nfit_bdw->dimm_block_offset);

	/* bdw3 (spa/dcr3, dimm3) */
	nfit_bdw = nfit_buf + offset + sizeof(struct nfit_bdw) * 3;
	writew(NFIT_TABLE_BDW, &nfit_bdw->type);
	writew(sizeof(struct nfit_bdw), &nfit_bdw->length);
	writew(3+1, &nfit_bdw->dcr_index);
	writew(1, &nfit_bdw->num_bdw);
	writeq(0, &nfit_bdw->bdw_offset);
	writeq(BDW_SIZE, &nfit_bdw->bdw_size);
	writeq(DIMM_SIZE, &nfit_bdw->dimm_capacity);
	writeq(0, &nfit_bdw->dimm_block_offset);

	writeb(nfit_checksum(nfit_buf, size), &nfit->checksum);

	nfit_desc = &t->nfit_desc;
	
	if (nd_manual_dsm)
		nfit_desc->nfit_ctl = nd_dsm_ctl;
	else
		nfit_desc->nfit_ctl = nfit_test_ctl;

	set_bit(NFIT_FLAG_FIC1_CAP, &nfit_desc->flags);
}

static void nfit_test1_setup(struct nfit_test *t)
{
	void __iomem *nfit_buf = t->nfit_buf;
	struct nfit_spa __iomem *nfit_spa;
	size_t size = t->nfit_size;
	struct nfit __iomem *nfit;

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

	/* spa0 (flat range with no bdw aliasing) */
	nfit_spa = nfit_buf + sizeof(*nfit);
	writew(NFIT_TABLE_SPA, &nfit_spa->type);
	writew(sizeof(*nfit_spa), &nfit_spa->length);
	writew(NFIT_SPA_PM, &nfit_spa->spa_type);
	writew(0+1, &nfit_spa->spa_index);
	writeq(t->spa_set_dma[0], &nfit_spa->spa_base);
	writeq(SPA2_SIZE, &nfit_spa->spa_length);

	writeb(nfit_checksum(nfit_buf, size), &nfit->checksum);
}

static int nfit_test_probe(struct platform_device *pdev)
{
	struct nfit_bus_descriptor *nfit_desc;
	struct device *dev = &pdev->dev;
	struct nfit_test *nfit_test;
	int rc;

	nfit_test = to_nfit_test(&pdev->dev);
	rc = dma_coerce_mask_and_coherent(dev, DMA_BIT_MASK(64));
	if (rc)
		return rc;

	/* common alloc */
	if (nfit_test->num_dcr) {
		int num = nfit_test->num_dcr;

		nfit_test->dimm = devm_kcalloc(dev, num, sizeof(void *), GFP_KERNEL);
		nfit_test->dimm_dma = devm_kcalloc(dev, num, sizeof(dma_addr_t), GFP_KERNEL);
		nfit_test->label = devm_kcalloc(dev, num, sizeof(void *), GFP_KERNEL);
		nfit_test->label_dma = devm_kcalloc(dev, num, sizeof(dma_addr_t), GFP_KERNEL);
		nfit_test->dcr = devm_kcalloc(dev, num, sizeof(struct nfit_test_dcr *), GFP_KERNEL);
		nfit_test->dcr_dma = devm_kcalloc(dev, num, sizeof(dma_addr_t), GFP_KERNEL);
		if (nfit_test->dimm && nfit_test->dimm_dma && nfit_test->label
				&& nfit_test->label_dma && nfit_test->dcr
				&& nfit_test->dcr_dma)
			/* pass */;
		else
			return -ENOMEM;
	}

	if (nfit_test->num_pm) {
		int num = nfit_test->num_pm;

		nfit_test->spa_set = devm_kcalloc(dev, num, sizeof(void *), GFP_KERNEL);
		nfit_test->spa_set_dma = devm_kcalloc(dev, num,
				sizeof(dma_addr_t), GFP_KERNEL);
		if (nfit_test->spa_set && nfit_test->spa_set_dma)
			/* pass */;
		else
			return -ENOMEM;
	}

	/* per-nfit specific alloc */
	if (nfit_test->alloc(nfit_test))
		return -ENOMEM;

	nfit_test->setup(nfit_test);

	nfit_desc = &nfit_test->nfit_desc;
	nfit_desc->nfit_base = nfit_test->nfit_buf;
	nfit_desc->nfit_size = nfit_test->nfit_size;

	nfit_test->nd_bus = nfit_bus_register(&pdev->dev, nfit_desc);
	if (!nfit_test->nd_bus)
		return -EINVAL;

	return 0;
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
	kfree(nfit_test);
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
extern struct resource *(*nfit_test_request_region)(resource_size_t start,
		resource_size_t n);

#ifdef CONFIG_CMA_SIZE_MBYTES
#define CMA_SIZE_MBYTES CONFIG_CMA_SIZE_MBYTES
#else
#define CMA_SIZE_MBYTES 0
#endif

static __init int nfit_test_init(void)
{
	int rc, i;

	if (CMA_SIZE_MBYTES < 584) {
		pr_err("need CONFIG_CMA_SIZE_MBYTES >= 584 to load\n");
		return -EINVAL;
	}

	nfit_test_ioremap = __nfit_test_ioremap;
	nfit_test_request_region = __nfit_test_request_region;

	for (i = 0; i < NUM_NFITS; i++) {
		struct nfit_test *nfit_test;
		struct platform_device *pdev;

		nfit_test = kzalloc(sizeof(*nfit_test), GFP_KERNEL);
		if (!nfit_test) {
			rc = -ENOMEM;
			goto err_register;
		}
		switch (i) {
		case 0:
			nfit_test->num_pm = NUM_PM;
			nfit_test->num_dcr = NUM_DCR;
			nfit_test->alloc = nfit_test0_alloc;
			nfit_test->setup = nfit_test0_setup;
			break;
		case 1:
			nfit_test->num_pm = 1;
			nfit_test->alloc = nfit_test1_alloc;
			nfit_test->setup = nfit_test1_setup;
			break;
		default:
			rc = -EINVAL;
			goto err_register;
		}
		pdev = &nfit_test->pdev;
		pdev->name = KBUILD_MODNAME;
		pdev->id = i;
		pdev->dev.release = nfit_test_release;
		rc = platform_device_register(pdev);
		if (rc) {
			put_device(&pdev->dev);
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
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Intel Corporation");
