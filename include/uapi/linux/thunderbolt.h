/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * Thunderbolt ioctl interface
 *
 * Copyright (C) 2019, Intel Corporation
 * Author: Raanan Avargil <raanan.avargil@intel.com>
 */

#ifndef _UAPI_LINUX_THUNDERBOLT_H
#define _UAPI_LINUX_THUNDERBOLT_H

#include <linux/types.h>
#include <linux/ioctl.h>

enum tb_dbg_config_space {
	TB_CFG_PATH,
	TB_CFG_ADAPTER,
	TB_CFG_ROUTER,
	TB_CFG_COUNTER,
};

/**
 * struct tb_dbg_query_nhi - Struct for querying TBT NHI
 * @offset: Offset from base
 * @value: register value
 */
struct tb_dbg_query_nhi {
	__u32 offset;
	__u32 value;
};

/**
 * struct tb_dbg_query_config_space - Struct for querying TBT config spaces
 * @route: Route string
 * @address: DW address of the first config register to read
 * @length: Number of double words to read/write. The value shall be greater
 *	    than %0 and less than or equal to %60.
 * @adapter_number: Adapter number whose config space is being read or
 *		    written, or 0 for router configuration space (a.k.a port)
 * @space: Target configuration space
 * @buffer: Data to be read/written from/to the target configuration space
 */
struct tb_dbg_query_config_space {
	__u64 route;
	__u32 address;
	__u32 length;
	__u32 adapter_number;
	enum tb_dbg_config_space space;
	void *buffer;
};

#define TB_DBG_IOCTL_BASE	'T'
#define TB_DBG_IO(nr)		_IO(TB_DBG_IOCTL_BASE, nr)
#define TB_DBG_IOR(nr, type)	_IOR(TB_DBG_IOCTL_BASE, nr, type)
#define TB_DBG_IOW(nr, type)	_IOW(TB_DBG_IOCTL_BASE, nr, type)
#define TB_DBG_IOWR(nr, type)	_IOWR(TB_DBG_IOCTL_BASE, nr, type)

#define TB_DBG_IOC_READ_NHI	TB_DBG_IOWR(0x01, struct tb_dbg_query_nhi)
#define TB_DBG_IOC_WRITE_NHI	TB_DBG_IOWR(0x02, struct tb_dbg_query_nhi)
#define TB_DBG_IOC_READ_TBT	TB_DBG_IOWR(0x03, struct tb_dbg_query_config_space)
#define TB_DBG_IOC_WRITE_TBT	TB_DBG_IOWR(0x04, struct tb_dbg_query_config_space)

#endif
