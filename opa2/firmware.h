/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2015 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * BSD LICENSE
 *
 * Copyright (c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Intel(R) Omni-Path Gen2 HFI PCIe Driver
 */
#ifndef _HFI_FIRMWARE_H
#define _HFI_FIRMWARE_H

/*
 * FW_EMPTY: no firmware is downloaded
 * FW_TRY: We tried the original and it failed. Move to the alternate names.
 * 			i.e. tried DEFAULT_FW_8051_NAME_FPGA and fail. So that move to
 * 			ALT_FW_8051_NAME_ASIC.
 * FW_FINAL: all firmwares are downloaded
 * FW_ERR: can't obtain a firmware file.
 */
enum fw_state {FW_EMPTY, FW_TRY, FW_FINAL, FW_ERR};

/*
 * Firmware security header.
 */
struct css_header {
	u32 module_type;
	u32 header_len;
	u32 header_version;
	u32 module_id;
	u32 module_vendor;
	u32 date;		/* BCD yyyymmdd */
	u32 size;		/* in DWORDs */
	u32 key_size;		/* in DWORDs */
	u32 modulus_size;	/* in DWORDs */
	u32 exponent_size;	/* in DWORDs */
	u32 reserved[22];
};

#define KEY_SIZE      256
#define MU_SIZE		8
#define EXPONENT_SIZE	4

struct augmented_firmware_file {
	struct css_header css_header;
	u8 modulus[KEY_SIZE];
	u8 exponent[EXPONENT_SIZE];
	u8 signature[KEY_SIZE];
	u8 r2[KEY_SIZE];
	u8 mu[MU_SIZE];
	u8 firmware[];
};

struct firmware_details {
	/* Linux core piece */
	const struct firmware *fw;

	struct css_header *css_header;
	u8 *firmware_ptr;		/* pointer to binary data */
	u32 firmware_len;		/* length in bytes */
	u8 *modulus;			/* pointer to the modulus */
	u8 *exponent;			/* pointer to the exponent */
	u8 *signature;			/* pointer to the signature */
	u8 *r2;				/* pointer to r2 */
	u8 *mu;				/* pointer to mu */
	struct augmented_firmware_file dummy_header;
};

/* 8051 firmware version helper */
#define crk8051_ver(a, b) ((a) << 8 | (b))

struct hfi_pportdata;

void hfi_firmware_dbg_init(struct hfi_devdata *dd);
void hfi_firmware_dbg_exit(struct hfi_devdata *dd);
int hfi2_read_8051_config(struct hfi_pportdata *ppd, u8 field_id, u8 lane_id,
	u32 *result);
void hfi2_dispose_firmware(struct hfi_devdata *dd);
int hfi_wait_firmware_ready(const struct hfi_pportdata *ppd, u32 mstimeout);
int hfi2_load_firmware(struct hfi_devdata *dd);

#endif	/* _HFI_FIRMWARE_H */
