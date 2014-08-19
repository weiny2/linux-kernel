/*
 * NVDIMM Firmware Interface Table (v0.8s)
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
#ifndef __NFIT_H__
#define __NFIT_H__

#include <linux/types.h>
#include <linux/ioport.h>

enum {
	NFIT_TABLE_SPA = 0,
	NFIT_TABLE_MEM = 1,
	NFIT_TABLE_IDT = 2,
	NFIT_TABLE_SMBIOS = 3,
	NFIT_TABLE_DCR = 4,
	NFIT_TABLE_BDW = 5,
	NFIT_TABLE_FLUSH = 6,
	NFIT_SPA_VOLATILE = 0,
	NFIT_SPA_PM = 1,
	NFIT_SPA_DCR = 2,
	NFIT_SPA_BDW = 3,
	NFIT_FLAG_FIC1_CAP = 0,
	NFIT_CMD_SMART = 1,
	NFIT_CMD_GET_CONFIG_SIZE = 2,
	NFIT_CMD_GET_CONFIG_DATA = 3,
	NFIT_CMD_SET_CONFIG_DATA = 4,
	NFIT_CMD_VENDOR = 5,
	NFIT_CMD_SCRUB = 6,
	NFIT_ARS_START = 1,
	NFIT_ARS_QUERY = 2,
};

/**
 * struct nfit - Nvdimm Firmware Interface Table
 * @signature: "NFIT"
 * @length: sum of size of this table plus all appended subtables
 */
struct nfit {
	__u8 signature[4];
	__le32 length;
	__u8 revision;
	__u8 checksum;
	__u8 oemid[6];
	__le64 oem_tbl_id;
	__le32 oem_revision;
	__le32 creator_id;
	__le32 creator_revision;
	__le32 reserved;
} __packed;

/**
 * struct nfit_spa - System Physical Address Range Descriptor Table
 * @spa_type: NFIT_SPA_*
 */
struct nfit_spa {
	__le16 type;
	__le16 length;
	__le16 spa_type;
	__le16 spa_index;
	__u8 flags;
	__u8 reserved[3];
	__le32 proximity_domain;
	__le64 spa_base;
	__le64 spa_length;
} __packed;

/**
 * struct nfit_mem - Memory Device to SPA Mapping Table
 */
struct nfit_mem {
	__le16 type;
	__le16 length;
	__le32 nfit_handle;
	__le16 phys_id;
	__le16 region_id;
	__le16 spa_index;
	__le16 dcr_index;
	__le64 region_len;
	__le64 region_offset;
	__le64 region_spa;
	__le16 idt_index;
	__le16 interleave_ways;
	__le32 reserved;
} __packed;

#define NFIT_DIMM_HANDLE(node, socket, imc, chan, dimm) \
	(((node & 0xfff) << 16) | ((socket & 0xf) << 12) \
	 | ((imc & 0xf) << 8) | ((chan & 0xf) << 4) | (dimm & 0xf))
#define NFIT_DIMM_NODE(handle) ((handle) >> 16 & 0xfff)
#define NFIT_DIMM_SOCKET(handle) ((handle) >> 12 & 0xf)
#define NFIT_DIMM_CHAN(handle) ((handle) >> 8 & 0xf)
#define NFIT_DIMM_IMC(handle) ((handle) >> 4 & 0xf)
#define NFIT_DIMM_DIMM(handle) ((handle) & 0xf)

/**
 * struct nfit_idt - Interleave description Table
 * @line_offset: at least 1 plus (num_lines - 1) lines
 */
struct nfit_idt {
	__le16 type;
	__le16 length;
	__le16 idt_index;
	__le16 reserved;
	__le32 num_lines;
	__le32 line_size;
	__le32 line_offset[1];
} __packed;

/**
 * struct nfit_smbios - SMBIOS Management Information Table
 */
struct nfit_smbios {
	__le16 type;
	__le16 length;
	__le32 reserved;
	__u8 data[0];
} __packed;

/**
 * struct nfit_dcr - NVDIMM Control Region Table
 * @fic: Format Interface Code
 * @cmd_offset: command registers relative to block control window
 * @status_offset: status registers relative to block control window
 */
struct nfit_dcr {
	__le16 type;
	__le16 length;
	__le16 dcr_index;
	__le16 vendor_id;
	__le16 device_id;
	__le16 revsion_id;
	__le16 fic;
	__le16 num_bdw;
	__le64 dcr_size;
	__le64 cmd_offset;
	__le64 cmd_size;
	__le64 status_offset;
	__le64 status_size;
} __packed;

/**
 * struct nfit_bdw - NVDIMM Block Data Window Region Table
 */
struct nfit_bdw {
	__le16 type;
	__le16 length;
	__le16 dcr_index;
	__le16 num_bdw;
	__le64 bdw_offset;
	__le64 bdw_size;
	__le64 dimm_capacity;
	__le64 dimm_block_offset;
} __packed;

/**
 * struct nfit_flush - Flush Hint Address Table
 * @num_flush: max number of flush hint addresses (architectural)
 * @num_flush_valid: number of consecutive valid flush hints in table
 */
struct nfit_flush {
	__le16 type;
	__le16 length;
	__le32 nfit_dev;
	__le32 nfit_mask;
	__le16 num_flush;
	__le16 num_flush_valid;
	__le64 flush_addr[1];
} __packed;

struct nfit_cmd_smart {
	u32 nfit_handle;
	u32 status;
	u32 out_length;
	u8 out_buf[0];
} __packed;

struct nfit_cmd_get_config_size {
	u32 nfit_handle;
	u32 status;
	u32 config_size;
	u32 optimal_io_size;
} __packed;

struct nfit_cmd_get_config_data {
	u32 nfit_handle;
	u32 in_offset;
	u32 in_length;
	u32 status;
	u32 out_length;
	u8 out_buf[0];
} __packed;

struct nfit_cmd_set_config_hdr {
	u32 nfit_handle;
	u32 in_offset;
	u32 in_length;
	u8 in_buf[0];
} __packed;

struct nfit_cmd_vendor_hdr {
	u32 nfit_handle;
	u32 in_length;
	u8 in_buf[0];
} __packed;

struct nfit_cmd_vendor_tail {
	u32 status;
	u32 out_length;
	u8 out_buf[0];
};

struct nfit_cmd_scrub {
	u32 cmd;
	u64 start_addr;
	u64 length;
	u32 status;
	u16 out_length;
	u64 out_buf[0];
} __packed;

struct nfit_bus_descriptor;
typedef int (*nfit_ctl_fn)(struct nfit_bus_descriptor *nfit_desc,
		unsigned int cmd, void *buf, unsigned int buf_len);
struct nfit_bus_descriptor {
	unsigned long flags;
	resource_size_t nfit_start;
	resource_size_t nfit_size;
	char *provider_name;
	nfit_ctl_fn nfit_ctl;
};

struct nd_bus;
struct nd_bus *nfit_bus_register(struct device *parent,
		struct nfit_bus_descriptor *nfit_desc);
void nfit_bus_unregister(struct nd_bus *nd_bus);
#endif /* __NFIT_H__ */
