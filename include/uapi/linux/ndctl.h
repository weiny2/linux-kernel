/*
 * Copyright (c) 2014, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 */
#ifndef __NDCTL_H__
#define __NDCTL_H__

#include <linux/types.h>

struct nfit_cmd_smart {
	__u32 nfit_handle;
	__u32 status;
	__u32 out_length;
	__u8 out_buf[0];
} __packed;

struct nfit_cmd_get_config_size {
	__u32 nfit_handle;
	__u32 status;
	__u32 config_size;
	__u32 optimal_io_size;
} __packed;

struct nfit_cmd_get_config_data {
	__u32 nfit_handle;
	__u32 in_offset;
	__u32 in_length;
	__u32 status;
	__u32 out_length;
	__u8 out_buf[0];
} __packed;

struct nfit_cmd_set_config_hdr {
	__u32 nfit_handle;
	__u32 in_offset;
	__u32 in_length;
	__u8 in_buf[0];
} __packed;

struct nfit_cmd_vendor_hdr {
	__u32 nfit_handle;
	__u32 in_length;
	__u8 in_buf[0];
} __packed;

struct nfit_cmd_vendor_tail {
	__u32 status;
	__u32 out_length;
	__u8 out_buf[0];
};

struct nfit_cmd_scrub {
	__u32 cmd;
	__u64 start_addr;
	__u64 length;
	__u32 status;
	__u16 out_length;
	__u64 out_buf[0];
} __packed;

/*
 * The struct defining the passthrough command and payloads to be operated
 * upon by the FNV firmware.  This is wrapped inside of a struct
 * nfit_cmd_vendor_hdr, which provides the total size of the cmd including the
 * payload.
 */
struct fnv_passthru_cmd {
	__u8 data_format_revision;
	__u8 opcode;
	__u8 sub_opcode;
	__u8 flags;
	__u32 reserved;
	__u8 in_buf[0];
} __packed;

enum {
	NFIT_CMD_SMART = 1,
	NFIT_CMD_GET_CONFIG_SIZE = 2,
	NFIT_CMD_GET_CONFIG_DATA = 3,
	NFIT_CMD_SET_CONFIG_DATA = 4,
	NFIT_CMD_VENDOR = 5,
	NFIT_CMD_SCRUB = 6,
	NFIT_ARS_START = 1,
	NFIT_ARS_QUERY = 2,
};

#define ND_IOCTL 'N'

#define NFIT_IOCTL_SMART		_IOWR(ND_IOCTL, NFIT_CMD_SMART,\
					struct nfit_cmd_smart)

#define NFIT_IOCTL_GET_CONFIG_SIZE	_IOWR(ND_IOCTL, NFIT_CMD_GET_CONFIG_SIZE,\
					struct nfit_cmd_get_config_size)

#define NFIT_IOCTL_GET_CONFIG_DATA	_IOWR(ND_IOCTL, NFIT_CMD_GET_CONFIG_DATA,\
					struct nfit_cmd_get_config_data)

#define NFIT_IOCTL_SET_CONFIG_DATA	_IOWR(ND_IOCTL, NFIT_CMD_SET_CONFIG_DATA,\
					struct nfit_cmd_set_config_hdr)

#define NFIT_IOCTL_VENDOR		_IOWR(ND_IOCTL, NFIT_CMD_VENDOR,\
					struct nfit_cmd_vendor_hdr)

#define NFIT_IOCTL_SCRUB		_IOWR(ND_IOCTL, NFIT_CMD_SCRUB,\
					struct nfit_cmd_scrub)

#define ND_DEVICE_DIMM 1            /* nd_dimm: container for "config data" */
#define ND_DEVICE_REGION_PMEM 2     /* nd_region: (parent of pmem namespaces) */
#define ND_DEVICE_REGION_BLOCK 3    /* nd_region: (parent of block namespaces) */
#define ND_DEVICE_NAMESPACE_IO 4    /* legacy persistent memory */
#define ND_DEVICE_NAMESPACE_PMEM 5  /* persistent memory namespace (may alias) */
#define ND_DEVICE_NAMESPACE_BLOCK 6 /* block-data-window namespace (may alias) */
#define ND_DEVICE_BTT 7		    /* block-translation table device */

enum nd_driver_flags {
	ND_DRIVER_DIMM            = 1 << ND_DEVICE_DIMM,
	ND_DRIVER_REGION_PMEM     = 1 << ND_DEVICE_REGION_PMEM,
	ND_DRIVER_REGION_BLOCK    = 1 << ND_DEVICE_REGION_BLOCK,
	ND_DRIVER_NAMESPACE_IO    = 1 << ND_DEVICE_NAMESPACE_IO,
	ND_DRIVER_NAMESPACE_PMEM  = 1 << ND_DEVICE_NAMESPACE_PMEM,
	ND_DRIVER_NAMESPACE_BLOCK = 1 << ND_DEVICE_NAMESPACE_BLOCK,
	ND_DRIVER_BTT		  = 1 << ND_DEVICE_BTT,
};
#endif /* __NDCTL_H__ */
