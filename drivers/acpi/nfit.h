/*
 * NVDIMM Firmware Interface Table - NFIT
 *
 * Copyright(c) 2013-2015 Intel Corporation. All rights reserved.
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
#ifndef __NFIT_H__
#define __NFIT_H__
#include <linux/types.h>
#include <linux/libnd.h>
#include <linux/uuid.h>
#include <linux/acpi.h>

static const uuid_le nfit_spa_uuid_volatile __maybe_unused = UUID_LE(0x7305944f,
		0xfdda, 0x44e3, 0xb1, 0x6c, 0x3f, 0x22, 0xd2, 0x52, 0xe5, 0xd0);

static const uuid_le nfit_spa_uuid_pm __maybe_unused = UUID_LE(0x66f0d379,
		0xb4f3, 0x4074, 0xac, 0x43, 0x0d, 0x33, 0x18, 0xb7, 0x8c, 0xdb);

static const uuid_le nfit_spa_uuid_dcr __maybe_unused = UUID_LE(0x92f701f6,
		0x13b4, 0x405d, 0x91, 0x0b, 0x29, 0x93, 0x67, 0xe8, 0x23, 0x4c);

static const uuid_le nfit_spa_uuid_bdw __maybe_unused = UUID_LE(0x91af0530,
		0x5d86, 0x470e, 0xa6, 0xb0, 0x0a, 0x2d, 0xb9, 0x40, 0x82, 0x49);

static const uuid_le nfit_spa_uuid_vdisk __maybe_unused = UUID_LE(0x77ab535a,
		0x45fc, 0x624b, 0x55, 0x60, 0xf7, 0xb2, 0x81, 0xd1, 0xf9, 0x6e);

static const uuid_le nfit_spa_uuid_vcd __maybe_unused = UUID_LE(0x3d5abd30,
		0x4175, 0x87ce, 0x6d, 0x64, 0xd2, 0xad, 0xe5, 0x23, 0xc4, 0xbb);

static const uuid_le nfit_spa_uuid_pdisk __maybe_unused = UUID_LE(0x5cea02c9,
		0x4d07, 0x69d3, 0x26, 0x9f, 0x44, 0x96, 0xfb, 0xe0, 0x96, 0xf9);

static const uuid_le nfit_spa_uuid_pcd __maybe_unused = UUID_LE(0x08018188,
		0x42cd, 0xbb48, 0x10, 0x0f, 0x53, 0x87, 0xd5, 0x3d, 0xed, 0x3d);

enum {
	NFIT_SPA_VOLATILE = 0,
	NFIT_SPA_PM = 1,
	NFIT_SPA_DCR = 2,
	NFIT_SPA_BDW = 3,
	NFIT_SPA_VDISK = 4,
	NFIT_SPA_VCD = 5,
	NFIT_SPA_PDISK = 6,
	NFIT_SPA_PCD = 7,
};

#define NFIT_DIMM_HANDLE(node, socket, imc, chan, dimm) \
       (((node & 0xfff) << 16) | ((socket & 0xf) << 12) \
        | ((imc & 0xf) << 8) | ((chan & 0xf) << 4) | (dimm & 0xf))
#define NFIT_DIMM_NODE(handle) ((handle) >> 16 & 0xfff)
#define NFIT_DIMM_SOCKET(handle) ((handle) >> 12 & 0xf)
#define NFIT_DIMM_CHAN(handle) ((handle) >> 8 & 0xf)
#define NFIT_DIMM_IMC(handle) ((handle) >> 4 & 0xf)
#define NFIT_DIMM_DIMM(handle) ((handle) & 0xf)

struct nfit_spa {
	struct acpi_nfit_system_address *spa;
	struct list_head list;
};

struct nfit_dcr {
	struct acpi_nfit_control_region *dcr;
	struct list_head list;
};

struct nfit_bdw {
	struct acpi_nfit_data_region *bdw;
	struct list_head list;
};

struct acpi_nfit_idt {
	struct acpi_nfit_interleave base;
	u32 line_offset[0];
};

struct nfit_idt {
	struct acpi_nfit_idt *idt;
	struct list_head list;
};

struct nfit_memdev {
	struct acpi_nfit_memory_map *memdev;
	struct list_head list;
};

/* assembled tables for a given dimm/memory-device */
struct nfit_mem {
	struct nd_dimm *nd_dimm;
	struct acpi_nfit_memory_map *memdev_dcr;
	struct acpi_nfit_memory_map *memdev_pmem;
	struct acpi_nfit_memory_map *memdev_bdw;
	struct acpi_nfit_control_region *dcr;
	struct acpi_nfit_data_region *bdw;
	struct acpi_nfit_system_address *spa_dcr;
	struct acpi_nfit_system_address *spa_bdw;
	struct acpi_nfit_idt *idt_dcr;
	struct acpi_nfit_idt *idt_bdw;
	struct list_head list;
	struct acpi_device *adev;
	unsigned long dsm_mask;
};

struct acpi_nfit_desc {
	struct nd_bus_descriptor nd_desc;
	struct acpi_table_nfit *nfit;
	struct mutex spa_map_mutex;
	struct list_head spa_maps;
	struct list_head memdevs;
	struct list_head dimms;
	struct list_head spas;
	struct list_head dcrs;
	struct list_head bdws;
	struct list_head idts;
	struct nd_bus *nd_bus;
	struct device *dev;
	unsigned long dimm_dsm_force_en;
	int (*blk_enable)(struct nd_bus *nd_bus, struct device *dev);
	void (*blk_disable)(struct nd_bus *nd_bus, struct device *dev);
	int (*blk_do_io)(struct nd_blk_region *ndbr, void *iobuf,
			u64 len, int write, resource_size_t dpa);
};

enum nd_blk_mmio_selector {
	BDW,
	DCR,
};

struct nfit_blk {
	struct nfit_blk_mmio {
		void *base;
		u64 size;
		u64 base_offset;
		u32 line_size;
		u32 num_lines;
		u32 table_size;
		struct acpi_nfit_idt *idt;
		struct acpi_nfit_system_address *spa;
	} mmio[2];
	struct nd_region *nd_region;
	u64 bdw_offset; /* post interleave offset */
	u64 stat_offset;
	u64 cmd_offset;
};

struct nfit_spa_mapping {
	struct acpi_nfit_desc *acpi_desc;
	struct acpi_nfit_system_address *spa;
	struct list_head list;
	struct kref kref;
	void *iomem;
};

static inline struct nfit_spa_mapping *to_spa_map(struct kref *kref)
{
	return container_of(kref, struct nfit_spa_mapping, kref);
}

static inline struct acpi_nfit_memory_map *__to_nfit_memdev(struct nfit_mem *nfit_mem)
{
	if (nfit_mem->memdev_dcr)
		return nfit_mem->memdev_dcr;
	return nfit_mem->memdev_pmem;
}

static inline struct acpi_nfit_desc *to_acpi_desc(struct nd_bus_descriptor *nd_desc)
{
	return container_of(nd_desc, struct acpi_nfit_desc, nd_desc);
}

int acpi_nfit_init(struct acpi_nfit_desc *nfit, acpi_size sz);
#endif /* __NFIT_H__ */
