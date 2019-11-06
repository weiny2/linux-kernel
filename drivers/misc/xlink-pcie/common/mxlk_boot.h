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

#define MXLK_BOOT_MAGIC_STRLEN (16)
#define MXLK_BOOT_MAGIC_ROM "VPUROM"
#define MXLK_BOOT_MAGIC_EMMC "VPUEMMC"
#define MXLK_BOOT_MAGIC_BL2 "VPUBL2"
#define MXLK_BOOT_MAGIC_UBOOT "VPUUBOOT"
#define MXLK_BOOT_MAGIC_YOCTO "VPUYOCTO"

enum mxlk_stage { STAGE_UNINIT, STAGE_ROM, STAGE_BL2, STAGE_UBOOT, STAGE_OS };

#define MXLK_BOOT_FIP_ID (0xFFFFFFFF)
#define MXLK_BOOT_OS_ID (0xFFFFFF4F)
#define MXLK_BOOT_ROOTFS_ID (0xFFFFFF46)
#define MXLK_BOOT_RAW_ID (0xFFFFFF00)

#define MXLK_FIP_FW_NAME "kmb_fip.bin"
#define MXLK_OS_FW_NAME "kmb_os.bin"
#define MXLK_ROOTFS_FW_NAME "kmb_rootfs.bin"
#define MXLK_RAW_FW_NAME "kmb_raw.bin"

#define MXLK_BOOT_STATUS_START (0x55555555)
#define MXLK_BOOT_STATUS_INVALID (0xDEADFFFF)
#define MXLK_BOOT_STATUS_DOWNLOADED (0xDDDDDDDD)
#define MXLK_BOOT_STATUS_ERROR (0xDEADAAAA)
#define MXLK_BOOT_STATUS_DONE (0xBBBBBBBB)

#define MXLK_INT_ENABLE (0x1)
#define MXLK_INT_MASK (0x1)

struct mxlk_boot {
	u8 magic[MXLK_BOOT_MAGIC_STRLEN];
	u32 mf_ready;
	u32 mf_len;
	u64 reserved;
	u64 mf_start;
	u32 int_enable;
	u32 int_mask;
	u32 int_identity;
	u64 mf_offset;
	u8 mf_dest[128];
} __packed;

#define MXLK_BOOT_OFFSET_MAGIC offsetof(struct mxlk_boot, magic)
#define MXLK_BOOT_OFFSET_MF_READY offsetof(struct mxlk_boot, mf_ready)
#define MXLK_BOOT_OFFSET_MF_LEN offsetof(struct mxlk_boot, mf_len)
#define MXLK_BOOT_OFFSET_MF_START offsetof(struct mxlk_boot, mf_start)
#define MXLK_BOOT_OFFSET_INT_ENABLE offsetof(struct mxlk_boot, int_enable)
#define MXLK_BOOT_OFFSET_INT_MASK offsetof(struct mxlk_boot, int_mask)
#define MXLK_BOOT_OFFSET_INT_IDENTITY offsetof(struct mxlk_boot, int_identity)
#define MXLK_BOOT_OFFSET_MF_OFFSET offsetof(struct mxlk_boot, mf_offset)
#define MXLK_BOOT_OFFSET_MF_DESK offsetof(struct mxlk_boot, mf_dest)

#endif // MXLK_BOOT_HEADER_
