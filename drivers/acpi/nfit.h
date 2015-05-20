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
#include <acpi/acuuid.h>

#define UUID_NFIT_BUS "2f10e7a4-9e91-11e4-89d3-123b93f75cba"
#define UUID_NFIT_DIMM "4309ac30-0d11-11e4-9191-0800200c9a66"

enum nfit_uuids {
	NFIT_SPA_VOLATILE,
	NFIT_SPA_PM,
	NFIT_SPA_DCR,
	NFIT_SPA_BDW,
	NFIT_SPA_VDISK,
	NFIT_SPA_VCD,
	NFIT_SPA_PDISK,
	NFIT_SPA_PCD,
	NFIT_DEV_BUS,
	NFIT_DEV_DIMM,
	NFIT_UUID_MAX,
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

struct nfit_idt {
	struct acpi_nfit_interleave *idt;
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
	struct acpi_nfit_interleave *idt_dcr;
	struct acpi_nfit_interleave *idt_bdw;
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
		struct acpi_nfit_interleave *idt;
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

const u8 *to_nfit_uuid(enum nfit_uuids id);
int acpi_nfit_init(struct acpi_nfit_desc *nfit, acpi_size sz);
#endif /* __NFIT_H__ */
