/* SPDX-License-Identifier: GPL-2.0 only */
/*****************************************************************************
 *
 * Intel Keem Bay XLink PCIe Driver
 *
 * Copyright (C) 2019 Intel Corporation
 *
 ****************************************************************************/

#ifndef MXLK_BOOT_HEADER_
#define MXLK_BOOT_HEADER_

#include <linux/types.h>

#define MXLK_BOOT_MAGIC_ROM "VPUROM"
#define MXLK_BOOT_MAGIC_EMMC "VPUEMMC"
#define MXLK_BOOT_MAGIC_BL2 "VPUBL2"
#define MXLK_BOOT_MAGIC_UBOOT "VPUUBOOT"
#define MXLK_BOOT_MAGIC_RECOV "VPURECOV"
#define MXLK_BOOT_MAGIC_YOCTO "VPUYOCTO"

enum mxlk_stage {
	STAGE_UNINIT,
	STAGE_ROM,
	STAGE_BL2,
	STAGE_UBOOT,
	STAGE_RECOV,
	STAGE_OS
};

#define MXLK_BOOT_FIP_ID (0xFFFFFFFF)
#define MXLK_BOOT_BOOT_ID (0xFFFFFF4F)
#define MXLK_BOOT_SYSTEM_ID (0xFFFFFF46)
#define MXLK_BOOT_RAW_ID (0xFFFFFF00)
#define MXLK_BOOT_ERASE_ID (0xFFFFFF01)
#define MXLK_BOOT_FLASH_ID (0xFFFFFF02)

#define MXLK_FIP_FW_NAME "intel-kmb/fip.bin"
#define MXLK_BOOT_FW_NAME "intel-kmb/kernel.bin"
#define MXLK_SYSTEM_FW_NAME "intel-kmb/ramfs.bin"

#define MXLK_BOOT_STATUS_START (0x55555555)
#define MXLK_BOOT_STATUS_INVALID (0xDEADFFFF)
#define MXLK_BOOT_STATUS_DOWNLOADED (0xDDDDDDDD)
#define MXLK_BOOT_STATUS_ERROR (0xDEADAAAA)
#define MXLK_BOOT_STATUS_DONE (0xBBBBBBBB)

#define MXLK_INT_ENABLE (0x1)
#define MXLK_INT_MASK (0x1)

#define MXLK_BOOT_MAGIC_STRLEN (16)
#define MXLK_BOOT_DEST_STRLEN (128)

struct mxlk_bootio {
	u8 magic[MXLK_BOOT_MAGIC_STRLEN];
	u32 mf_ready;
	u32 mf_len;
	u64 reserved1;
	u64 mf_start;
	u32 int_enable;
	u32 int_mask;
	u32 int_identity;
	u32 reserved2;
	u64 mf_offset;
	u8 mf_dest[MXLK_BOOT_DEST_STRLEN];
	u64 dev_id;
} __packed;

#define MXLK_BOOT_OFFSET_MAGIC offsetof(struct mxlk_bootio, magic)
#define MXLK_BOOT_OFFSET_MF_READY offsetof(struct mxlk_bootio, mf_ready)
#define MXLK_BOOT_OFFSET_MF_LEN offsetof(struct mxlk_bootio, mf_len)
#define MXLK_BOOT_OFFSET_MF_START offsetof(struct mxlk_bootio, mf_start)
#define MXLK_BOOT_OFFSET_INT_ENABLE offsetof(struct mxlk_bootio, int_enable)
#define MXLK_BOOT_OFFSET_INT_MASK offsetof(struct mxlk_bootio, int_mask)
#define MXLK_BOOT_OFFSET_INT_IDENTITY offsetof(struct mxlk_bootio, int_identity)
#define MXLK_BOOT_OFFSET_MF_OFFSET offsetof(struct mxlk_bootio, mf_offset)
#define MXLK_BOOT_OFFSET_MF_DEST offsetof(struct mxlk_bootio, mf_dest)
#define MXLK_BOOT_OFFSET_DEVID offsetof(struct mxlk_bootio, dev_id)

#endif // MXLK_BOOT_HEADER_
