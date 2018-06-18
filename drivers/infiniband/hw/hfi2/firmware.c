/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright(c) 2015 - 2017 Intel Corporation.
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
 * Copyright(c) 2015 - 2017 Intel Corporation.
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

#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/crc32.h>
#include <linux/kernel.h>
#include "hfi2.h"
#include "debugfs.h"
#include "link.h"
#include "platform.h"
#include "firmware.h"
#include "chip/fxr_8051_defs.h"
#include "chip/fxr_oc_defs.h"
#include "chip/fxr_fw_defs.h"
#include "chip/fxr_rx_hp_defs.h"
#include "chip/fxr_tx_otr_pkt_top_csrs.h"
#include "chip/fxr_tx_otr_msg_top_csrs.h"
#include "chip/fxr_tx_otr_pkt_top_csrs_defs.h"
#include "chip/fxr_tx_otr_msg_top_csrs_defs.h"

/*
 * Make it easy to toggle firmware file name and if it gets loaded by
 * editing the following. This may be something we do while in development
 * but not necessarily something a user would ever need to use.
 */
/* FXRTODO: replace with correct fw names once they are available */
#define DEFAULT_FW_8051_NAME_FPGA "hfi_oc8051.bin"
#define DEFAULT_FW_8051_NAME_OLD "hfi_dc8051.bin"
#define DEFAULT_HDRPE_FW_NAME "rxhp_main_ram_hdl_signed.bin"
#define DEFAULT_FRAGPE_FW_NAME "otr_frag_contents_prog_mem_signed.bin"
#define DEFAULT_BUFPE_FW_NAME "otr_buff_contents_prog_mem_signed.bin"
#define ALT_FW_8051_NAME_ASIC "hfi1_oc8051_d.fw"
#define ALT_HDRPE_FW_NAME "rxhp_main_ram_hdl_signed_d.fw"
#define ALT_FRAGPE_FW_NAME "otr_frag_contents_prog_mem_signed_d.fw"
#define ALT_BUFPE_FW_NAME "otr_buff_contents_prog_mem_signed_d.fw"

#define HOST_INTERFACE_VERSION 1

/* expected field values */
#define CSS_MODULE_TYPE	   0x00000006
#define CSS_HEADER_LEN	   0x000000a1
#define CSS_HEADER_VERSION 0x00010000
#define CSS_MODULE_VENDOR  0x00008086

#define HFI_BUFF_COMPLETE \
	FXR_TXOTR_MSG_CFG_BUFF_ENG_AUTHENTICATION_COMPLETE_SMASK
#define HFI_BUFF_SUCCESS \
	FXR_TXOTR_MSG_CFG_BUFF_ENG_AUTHENTICATION_SUCCESSFUL_SMASK
#define HFI_FRAG_COMPLETE \
	FXR_TXOTR_PKT_CFG_FRAG_ENG_AUTHENTICATION_COMPLETE_SMASK
#define HFI_FRAG_SUCCESS \
	FXR_TXOTR_PKT_CFG_FRAG_ENG_AUTHENTICATION_SUCCESSFUL_SMASK
#define HFI_FPE_DATA_MEM_ACCESS_VALID \
	FXR_TXOTR_PKT_DBG_FPE_DATA_MEM_ACCESS_VALID_SMASK

/* the file itself */
struct firmware_file {
	struct css_header css_header;
	u8 modulus[KEY_SIZE];
	u8 exponent[EXPONENT_SIZE];
	u8 signature[KEY_SIZE];
	u8 firmware[];
};

/* augmented file size difference */
#define AUGMENT_SIZE (sizeof(struct augmented_firmware_file) - \
						sizeof(struct firmware_file))

/* security block commands */
#define RSA_CMD_INIT  0x1
#define RSA_CMD_START 0x2

/* security block status */
#define RSA_STATUS_IDLE   0x0
#define RSA_STATUS_ACTIVE 0x1
#define RSA_STATUS_DONE   0x2
#define RSA_STATUS_FAILED 0x3

/* RSA engine timeout, in ms */
#define RSA_ENGINE_TIMEOUT 100 /* ms */

/* hardware mutex timeout, in ms */
#define HM_TIMEOUT 4000 /* 4 s */

/* 8051 memory access timeout, in us */
#define CRK8051_ACCESS_TIMEOUT 100 /* us */

/* pe authentication timeout */
/* Simics: 10ms = 1000 * udelay(10) */
/* ZEBU: 100ms = 10000 * udelay(10) */
#define PE_TIMEOUT 10000

static void hfi_write_bufpe_prog_mem(struct hfi_devdata *dd, u8 *data,
				     u32 off, u64 mem)
{
	u64 pl = 0;

	pl = (FXR_TXOTR_MSG_DBG_BPE_PROG_MEM_PAYLOAD0_DATA_MASK &
	      *(u32 *)&data[off]) <<
	      FXR_TXOTR_MSG_DBG_BPE_PROG_MEM_PAYLOAD0_DATA_SHIFT;

	mem &= ~(FXR_TXOTR_MSG_DBG_BPE_PROG_MEM_ACCESS_ADDRESS_SMASK |
		 FXR_TXOTR_MSG_DBG_BPE_PROG_MEM_ACCESS_VALID_SMASK);
	/* FXRTODO: check if ADDRESS_MASK needs to be updated */
	mem |= ((FXR_TXOTR_MSG_DBG_BPE_PROG_MEM_ACCESS_ADDRESS_MASK &
		(off >> 2)) <<
		FXR_TXOTR_MSG_DBG_BPE_PROG_MEM_ACCESS_ADDRESS_SHIFT) |
		FXR_TXOTR_MSG_DBG_BPE_PROG_MEM_ACCESS_VALID_SMASK;

	write_csr(dd, FXR_TXOTR_MSG_DBG_BPE_PROG_MEM_PAYLOAD0, pl);
	write_csr(dd, FXR_TXOTR_MSG_DBG_BPE_PROG_MEM_ACCESS, mem);
	/* FXRTODO: uncomment after valid field checking is
	 * implemented
	 */
	/*count = 0;
	 *do {
	 *	count ++;
	 *	if (count > FPE_DATA_MEM_TIMEOUT) {
	 *		dd_dev_err(dd, "timeout writing fragpe data\n");
	 *		return -ENXIO;
	 *	}
	 *	udelay(10);
	 *	fpe_data_mem_access = read_csr(dd,
	 *			txotr_pkt_dbg_fpe_data_mem_access);
	 *}while(fpe_data_mem_access & HFI_FPE_DATA_MEM_ACCESS_VALID);
	 */
}

static void hfi_write_bufpe_data_mem(struct hfi_devdata *dd, u8 *data,
				     u32 off, u64 mem, u32 len)
{
	u64 pl = 0;

	int bytes = len - off;

	if (bytes < 8)
		memcpy(&pl, &data[off], bytes);
	else
		pl = *(u64 *)&data[off];

	mem &= ~(FXR_TXOTR_MSG_DBG_BPE_DATA_MEM_ACCESS_ADDRESS_SMASK |
		 FXR_TXOTR_MSG_DBG_BPE_DATA_MEM_ACCESS_VALID_SMASK);
	/* FXRTODO: check if ADDRESS_MASK needs to be updated */
	mem |= ((FXR_TXOTR_MSG_DBG_BPE_DATA_MEM_ACCESS_ADDRESS_MASK &
		(off >> 3)) <<
		FXR_TXOTR_MSG_DBG_BPE_DATA_MEM_ACCESS_ADDRESS_SHIFT) |
		FXR_TXOTR_MSG_DBG_BPE_DATA_MEM_ACCESS_VALID_SMASK;

	write_csr(dd, FXR_TXOTR_MSG_DBG_BPE_DATA_MEM_PAYLOAD0, pl);
	write_csr(dd, FXR_TXOTR_MSG_DBG_BPE_DATA_MEM_ACCESS, mem);
	/* FXRTODO: uncomment after valid field checking is
	 * implemented
	 */

	/*count = 0;
	 *do {
	 *	count ++;
	 *	if (count > FPE_DATA_MEM_TIMEOUT) {
	 *		dd_dev_err(dd, "timeout writing fragpe data\n");
	 *		return -ENXIO;
	 *	}
	 *	udelay(10);
	 *	fpe_data_mem_access = read_csr(dd,
	 *			txotr_pkt_dbg_fpe_data_mem_access);
	 *}while(fpe_data_mem_access & HFI_FPE_DATA_MEM_ACCESS_VALID);
	 */
}

static void hfi_write_fragpe_prog_mem(struct hfi_devdata *dd, u8 *data,
				      u32 off, u64 mem)
{
	u64 pl = 0;

	pl = (FXR_TXOTR_PKT_DBG_FPE_PROG_MEM_PAYLOAD0_DATA_MASK &
	      *(u32 *)&data[off]) <<
	      FXR_TXOTR_PKT_DBG_FPE_PROG_MEM_PAYLOAD0_DATA_SHIFT;

	mem &= ~(FXR_TXOTR_PKT_DBG_FPE_PROG_MEM_ACCESS_ADDRESS_SMASK |
		 FXR_TXOTR_PKT_DBG_FPE_PROG_MEM_ACCESS_VALID_SMASK);
	/* FXRTODO: check if ADDRESS_MASK needs to be updated */
	mem |= ((FXR_TXOTR_PKT_DBG_FPE_PROG_MEM_ACCESS_ADDRESS_MASK &
		(off >> 2)) <<
		FXR_TXOTR_PKT_DBG_FPE_PROG_MEM_ACCESS_ADDRESS_SHIFT) |
		FXR_TXOTR_PKT_DBG_FPE_PROG_MEM_ACCESS_VALID_SMASK;

	write_csr(dd, FXR_TXOTR_PKT_DBG_FPE_PROG_MEM_PAYLOAD0, pl);
	write_csr(dd, FXR_TXOTR_PKT_DBG_FPE_PROG_MEM_ACCESS, mem);
	/* FXRTODO: uncomment after valid field checking is implemented */
	/*count = 0;
	 *do {
	 *	count ++;
	 *	if (count > FPE_DATA_MEM_TIMEOUT) {
	 *		dd_dev_err(dd, "timeout writing fragpe data\n");
	 *		return -ENXIO;
	 *	}
	 *	udelay(10);
	 *	mem = read_csr(dd, txotr_pkt_dbg_fpe_data_mem_access);
	 *}while(fpe_data_mem_access & HFI_FPE_DATA_MEM_ACCESS_VALID);
	 */
}

static void hfi_write_fragpe_data_mem(struct hfi_devdata *dd, u8 *data, u32 off,
				      u64 mem, u32 len)
{
	u64 pl = 0;

	int bytes = len - off;

	if (bytes < 8)
		memcpy(&pl, &data[off], bytes);
	else
		pl = *(u64 *)&data[off];

	mem &= ~(FXR_TXOTR_PKT_DBG_FPE_PROG_MEM_ACCESS_ADDRESS_SMASK |
		 FXR_TXOTR_PKT_DBG_FPE_PROG_MEM_ACCESS_VALID_SMASK);
	/* FXRTODO: check if ADDRESS_MASK needs to be updated */
	mem |= ((FXR_TXOTR_PKT_DBG_FPE_PROG_MEM_ACCESS_ADDRESS_MASK &
		(off >> 3)) <<
		FXR_TXOTR_PKT_DBG_FPE_PROG_MEM_ACCESS_ADDRESS_SHIFT) |
		FXR_TXOTR_PKT_DBG_FPE_PROG_MEM_ACCESS_VALID_SMASK;

	write_csr(dd, FXR_TXOTR_PKT_DBG_FPE_DATA_MEM_PAYLOAD0, pl);
	write_csr(dd, FXR_TXOTR_PKT_DBG_FPE_DATA_MEM_ACCESS, mem);
	/* FXRTODO: uncomment after valid field checking is
	 * implemented
	 */
	/*count = 0;
	 *do {
	 *	count ++;
	 *	if (count > FPE_DATA_MEM_TIMEOUT) {
	 *		dd_dev_err(dd, "timeout writing fragpe data\n");
	 *		return -ENXIO;
	 *	}
	 *	udelay(10);
	 *	fpe_data_mem_access = read_csr(dd,
	 *	txotr_pkt_dbg_fpe_data_mem_access);
	 *}while(fpe_data_mem_access & HFI_FPE_DATA_MEM_ACCESS_VALID);
	 */
}

/* forwards */
static void dispose_one_firmware(struct firmware_details *fdet);

/*
 * Read a single 64-bit value from 8051 data memory.
 *
 * Expects:
 * o caller to have already set up data read, no auto increment
 * o caller to turn off read enable when finished
 *
 * The address argument is a byte offset.  Bits 0:2 in the address are
 * ignored - i.e. the hardware will always do aligned 8-byte reads as if
 * the lower bits are zero.
 *
 * Return 0 on success, -ENXIO on a read error (timeout).
 */
static int __read_8051_data(struct hfi_pportdata *ppd, u32 addr, u64 *result)
{
	u64 reg;
	int count;

	/* start the read at the given address */
	reg = ((addr & CRK_CRK8051_CFG_RAM_ACCESS_CTRL_ADDRESS_MASK)
			<< CRK_CRK8051_CFG_RAM_ACCESS_CTRL_ADDRESS_SHIFT)
		| CRK_CRK8051_CFG_RAM_ACCESS_CTRL_READ_ENA_SMASK;
	write_csr(ppd->dd, CRK_CRK8051_CFG_RAM_ACCESS_CTRL, reg);

	/* wait until ACCESS_COMPLETED is set */
	count = 0;
	while ((read_csr(ppd->dd, CRK_CRK8051_CFG_RAM_ACCESS_STATUS)
		    & CRK_CRK8051_CFG_RAM_ACCESS_STATUS_ACCESS_COMPLETED_SMASK)
		    == 0) {
		count++;
		if (count > CRK8051_ACCESS_TIMEOUT) {
			ppd_dev_err(ppd, "timeout reading 8051 data\n");
			return -ENXIO;
		}
		ndelay(10);
	}

	/* gather the data */
	*result = read_csr(ppd->dd, CRK_CRK8051_CFG_RAM_ACCESS_RD_DATA);

	return 0;
}

/*
 * Read 8051 data starting at addr, for len bytes.  Will read in 8-byte chunks.
 * Return 0 on success, -errno on error.
 */
int hfi_read_8051_data(struct hfi_pportdata *ppd, u32 addr,
		       u32 len, u64 *result)
{
	unsigned long flags;
	u32 done;
	int ret = 0;

	spin_lock_irqsave(&ppd->crk8051_lock, flags);

	/* data read set-up, no auto-increment */
	write_csr(ppd->dd, CRK_CRK8051_CFG_RAM_ACCESS_SETUP, 0);

	for (done = 0; done < len; addr += 8, done += 8, result++) {
		ret = __read_8051_data(ppd, addr, result);
		if (ret)
			break;
	}

	/* turn off read enable */
	write_csr(ppd->dd, CRK_CRK8051_CFG_RAM_ACCESS_CTRL, 0);

	spin_unlock_irqrestore(&ppd->crk8051_lock, flags);

	return ret;
}

/*
 * Write data or code to the 8051 code or data RAM.
 */
static int write_8051(struct hfi_pportdata *ppd, int code, u32 start,
		      const u8 *data, u32 len)
{
	u64 reg;
	u32 offset;
	int aligned, count;

	/* check alignment */
	aligned = ((unsigned long)data & 0x7) == 0;

	/* write set-up */
	reg = (code ? CRK_CRK8051_CFG_RAM_ACCESS_SETUP_RAM_SEL_SMASK : 0ull)
		| CRK_CRK8051_CFG_RAM_ACCESS_SETUP_AUTO_INCR_ADDR_SMASK;
	write_csr(ppd->dd, CRK_CRK8051_CFG_RAM_ACCESS_SETUP, reg);

	reg = ((start & CRK_CRK8051_CFG_RAM_ACCESS_CTRL_ADDRESS_MASK)
			<< CRK_CRK8051_CFG_RAM_ACCESS_CTRL_ADDRESS_SHIFT)
		| CRK_CRK8051_CFG_RAM_ACCESS_CTRL_WRITE_ENA_SMASK;
	write_csr(ppd->dd, CRK_CRK8051_CFG_RAM_ACCESS_CTRL, reg);

	/* write */
	for (offset = 0; offset < len; offset += 8) {
		int bytes = len - offset;

		if (bytes < 8) {
			reg = 0;
			memcpy(&reg, &data[offset], bytes);
		} else if (aligned) {
			reg = *(u64 *)&data[offset];
		} else {
			memcpy(&reg, &data[offset], 8);
		}
		dd_dev_dbg(ppd->dd, "Writing byte %d of 0x%llx to 8051\n",
			   offset / 8, reg);
		write_csr(ppd->dd, CRK_CRK8051_CFG_RAM_ACCESS_WR_DATA, reg);

		/* wait until ACCESS_COMPLETED is set */
		count = 0;
		while ((read_csr(ppd->dd, CRK_CRK8051_CFG_RAM_ACCESS_STATUS)
		    & CRK_CRK8051_CFG_RAM_ACCESS_STATUS_ACCESS_COMPLETED_SMASK)
		    == 0) {
			count++;
			if (count > CRK8051_ACCESS_TIMEOUT) {
				ppd_dev_err(ppd, "timeout writing 8051 data\n");
				return -ENXIO;
			}
			udelay(1);
		}
	}

	/* turn off write access, auto increment (also sets to data access) */
	write_csr(ppd->dd, CRK_CRK8051_CFG_RAM_ACCESS_CTRL, 0);
	write_csr(ppd->dd, CRK_CRK8051_CFG_RAM_ACCESS_SETUP, 0);

	return 0;
}

/* return 0 if values match, non-zero and complain otherwise */
static int invalid_header(struct hfi_devdata *dd, const char *what,
			  u32 actual, u32 expected)
{
	if (actual == expected)
		return 0;

	dd_dev_err(dd,
		   "invalid firmware header field %s: expected 0x%x, actual 0x%x\n",
		   what, expected, actual);
	return 1;
}

/*
 * Verify that the static fields in the CSS header match.
 */
static int verify_css_header(struct hfi_devdata *dd, struct css_header *css)
{
	/* verify CSS header fields (most sizes are in DW, so add /4) */
	if (invalid_header(dd, "module_type", css->module_type,
			   CSS_MODULE_TYPE) ||
	    invalid_header(dd, "header_len", css->header_len,
			   (sizeof(struct firmware_file) / 4)) ||
	    invalid_header(dd, "header_version",
			   css->header_version, CSS_HEADER_VERSION) ||
	    invalid_header(dd, "module_vendor",
			   css->module_vendor, CSS_MODULE_VENDOR) ||
	    invalid_header(dd, "key_size", css->key_size, KEY_SIZE / 4) ||
	    invalid_header(dd, "modulus_size",
			   css->modulus_size, KEY_SIZE / 4) ||
	    invalid_header(dd, "exponent_size", css->exponent_size,
			   EXPONENT_SIZE / 4)) {
		return -EINVAL;
	}
	return 0;
}

/*
 * Make sure there are at least some bytes after the prefix.
 */
static int payload_check(struct hfi_devdata *dd, const char *name,
			 long file_size, long prefix_size)
{
	/* make sure we have some payload */
	if (prefix_size >= file_size) {
		dd_dev_err(dd,
			   "firmware \"%s\", size %ld, must be larger than %ld bytes\n",
			   name, file_size, prefix_size);
		return -EINVAL;
	}

	return 0;
}

static void obtain_default_fw_name(struct hfi_devdata *dd, enum fw_type fw)
{
	switch (fw) {
	case FW_8051:
		dd->fw_name = dd->emulation ? DEFAULT_FW_8051_NAME_FPGA :
					      DEFAULT_FW_8051_NAME_OLD;
		break;
	case FW_HDRPE:
		dd->fw_name = DEFAULT_HDRPE_FW_NAME;
		break;
	case FW_FRAGPE:
		dd->fw_name = DEFAULT_FRAGPE_FW_NAME;
		break;
	case FW_BUFPE:
		dd->fw_name = DEFAULT_BUFPE_FW_NAME;
		break;
	default:
		dd->fw_name = NULL;
	}
}

static void obtain_alt_fw_name(struct hfi_devdata *dd, enum fw_type fw)
{
	switch (fw) {
	case FW_8051:
		dd->fw_name = ALT_FW_8051_NAME_ASIC;
		break;
	case FW_HDRPE:
		dd->fw_name = ALT_HDRPE_FW_NAME;
		break;
	case FW_FRAGPE:
		dd->fw_name = ALT_FRAGPE_FW_NAME;
		break;
	case FW_BUFPE:
		dd->fw_name = ALT_BUFPE_FW_NAME;
		break;
	default:
		dd->fw_name = NULL;
	}
}

/*
 * Request the firmware from the system.  Extract the pieces and fill in
 * fdet.  If successful, the caller will need to call dispose_one_firmware().
 * Returns 0 on success, -ERRNO on error.
 */
static int obtain_one_firmware(struct hfi_devdata *dd, const char *name,
			       struct firmware_details *fdet,
			       enum fw_type fw)
{
	struct css_header *css;
	int ret;

	memset(fdet, 0, sizeof(*fdet));

	ret = request_firmware(&fdet->fw, name, &dd->pdev->dev);
	if (ret) {
		dd_dev_err(dd, "cannot find firmware \"%s\", err %d\n",
			   name, ret);
		return ret;
	}

	/* verify the firmware */
	if (fdet->fw->size < sizeof(struct css_header)) {
		dd_dev_err(dd, "firmware \"%s\" is too small\n", name);
		ret = -EINVAL;
		goto done;
	}
	css = (struct css_header *)fdet->fw->data;

	dd_dev_info(dd, "Firmware %s details:", name);
	dd_dev_info(dd, "file size: 0x%lx bytes", fdet->fw->size);
	dd_dev_info(dd, "CSS structure:");
	dd_dev_info(dd, "  module_type    0x%x", css->module_type);
	dd_dev_info(dd, "  header_len     0x%03x (0x%03x bytes)",
		    css->header_len, 4 * css->header_len);
	dd_dev_info(dd, "  header_version 0x%x", css->header_version);
	dd_dev_info(dd, "  module_id      0x%x", css->module_id);
	dd_dev_info(dd, "  module_vendor  0x%x", css->module_vendor);
	dd_dev_info(dd, "  date           0x%x", css->date);
	dd_dev_info(dd, "  size           0x%03x (0x%03x bytes)",
		    css->size, 4 * css->size);
	dd_dev_info(dd, "  key_size       0x%03x (0x%03x bytes)",
		    css->key_size, 4 * css->key_size);
	dd_dev_info(dd, "  modulus_size   0x%03x (0x%03x bytes)",
		    css->modulus_size, 4 * css->modulus_size);
	dd_dev_info(dd, "  exponent_size  0x%03x (0x%03x bytes)",
		    css->exponent_size, 4 * css->exponent_size);
	dd_dev_info(dd, "firmware size: 0x%lx bytes",
		    fdet->fw->size - sizeof(struct firmware_file));

	/*
	 * If the file does not have a valid CSS header, assume it is
	 * a raw binary. Otherwise, check the CSS size field for an
	 * expected size. The augmented file has r2 and mu inserted
	 * after the header was generated, so there will be a known
	 * difference between the CSS header size and the actual file
	 * size. Use this difference to identify an augmented file.
	 *
	 * Note: css->size is in DWORDs, multiply by 4 to get bytes.
	 */
	ret = verify_css_header(dd, css);
	if (ret) {
		/* assume this is a raw binary, with no CSS header */
		dd_dev_info(dd,
			"Invalid CSS header for \"%s\" - assuming raw binary, turning off validation\n",
			name);
		ret = 0; /* OK for now */
		/*
		 * Assign fields from the dummy header in case we go down the
		 * wrong path.
		 */
		fdet->css_header = css;
		fdet->modulus = fdet->dummy_header.modulus;
		fdet->exponent = fdet->dummy_header.exponent;
		fdet->signature = fdet->dummy_header.signature;
		fdet->r2 = fdet->dummy_header.r2;
		fdet->mu = fdet->dummy_header.mu;
		fdet->firmware_ptr = (u8 *)fdet->fw->data;
		fdet->firmware_len = fdet->fw->size;
	} else if ((css->size * 4) == fdet->fw->size) {
		/* non-augmented firmware file */
		struct firmware_file *ff = (struct firmware_file *)
					   fdet->fw->data;

		/* make sure there are bytes in the payload */
		ret = payload_check(dd, name, fdet->fw->size,
				    sizeof(struct firmware_file));
		if (ret == 0) {
			fdet->css_header = css;
			fdet->modulus = ff->modulus;
			fdet->exponent = ff->exponent;
			fdet->signature = ff->signature;
			/* use dummy space */
			fdet->r2 = fdet->dummy_header.r2;
			/* use dummy space */
			fdet->mu = fdet->dummy_header.mu;
			fdet->firmware_ptr = ff->firmware;
			fdet->firmware_len = fdet->fw->size -
			sizeof(struct firmware_file);
			/*
			 * Header does not include r2 and mu -
			 * generate here. For now, fail.
			 */
			if (fw == FW_8051 &&
			    dd->icode != ICODE_FPGA_EMULATION) {
				dd_dev_err(dd, "Cannot validate fw w/o r2 and mu\n");
				ret = -EINVAL;
			}
		}
	} else if ((css->size * 4) + AUGMENT_SIZE == fdet->fw->size) {
		/* augmented firmware file */
		struct augmented_firmware_file *aff =
		(struct augmented_firmware_file *)
		fdet->fw->data;

		/* make sure there are bytes in the payload */
		ret = payload_check(dd, name, fdet->fw->size,
				    sizeof
				    (struct
				     augmented_firmware_file));
		if (ret == 0) {
			fdet->css_header = css;
			fdet->modulus = aff->modulus;
			fdet->exponent = aff->exponent;
			fdet->signature = aff->signature;
			fdet->r2 = aff->r2;
			fdet->mu = aff->mu;
			fdet->firmware_ptr = aff->firmware;
			fdet->firmware_len = fdet->fw->size -
			sizeof(struct augmented_firmware_file);
		}
	} else {
		/* css->size check failed */
		dd_dev_err(dd,
			   "invalid firmware header field size: expected 0x%lx or 0x%lx, actual 0x%x\n",
			   fdet->fw->size / 4,
			   (fdet->fw->size - AUGMENT_SIZE) / 4,
			   css->size);

		ret = -EINVAL;
	}

done:
	/* if returning an error, clean up after ourselves */
	if (ret)
		dispose_one_firmware(fdet);
	return ret;
}

static void dispose_one_firmware(struct firmware_details *fdet)
{
	release_firmware(fdet->fw);
	/* erase all previous information */
	memset(fdet, 0, sizeof(*fdet));
}

/*
 * Obtain the 4 firmwares from the OS.  All must be obtained at once or not
 * at all.  If called with the firmware state in FW_TRY, use alternate names.
 * On exit, this routine will have set the firmware state to one of FW_TRY,
 * FW_FINAL, or FW_ERR.
 *
 * Must be holding fw_mutex.
 */
static void __obtain_firmware(struct hfi_devdata *dd, enum fw_type fw)
{
	int err = 0;

	if (dd->fw_state == FW_FINAL)	/* nothing more to obtain */
		return;
	if (dd->fw_state == FW_ERR)		/* already in error */
		return;

	/* dd->fw_state is FW_EMPTY or FW_TRY */
retry:
	if (dd->fw_state == FW_TRY) {
		/*
		 * We tried the original and it failed.  Move to the
		 * alternate.
		 */
		dd_dev_info(dd, "using alternate firmware names\n");
		/*
		 * Let others run.  Some systems, when missing firmware, does
		 * something that holds for 30 seconds.  If we do that twice
		 * in a row it triggers task blocked warning.
		 */
		cond_resched();
		if (dd->fw_load)
			dispose_one_firmware(&dd->fw_det);
		obtain_alt_fw_name(dd, fw);
	}

	if (dd->fw_load)
		err = obtain_one_firmware(dd, dd->fw_name, &dd->fw_det, fw);

	if (err) {
		/* oops, had problems obtaining a firmware */
		if (dd->fw_state == FW_EMPTY) {
			/* retry with alternate */
			dd->fw_state = FW_TRY;
			goto retry;
		}
		dd->fw_state = FW_ERR;
		dd->fw_err = -ENOENT;
	} else {
		/* success */
		if (dd->fw_state == FW_EMPTY)
			dd->fw_state = FW_TRY;	/* may retry later */
		else
			dd->fw_state = FW_FINAL;	/* cannot try again */
	}
}

/*
 * Called by all HFIs when loading their firmware - i.e. device probe time.
 * The first one will do the actual firmware load.  Use a mutex to resolve
 * any possible race condition.
 *
 * The call to this routine cannot be moved to driver load because the kernel
 * call request_firmware() requires a device which is only available after
 * the first device probe.
 */
static int obtain_firmware(struct hfi_devdata *dd, enum fw_type fw)
{
	unsigned long timeout;

	mutex_lock(&dd->fw_mutex);

	/* 40s delay due to long delay on missing firmware on some systems */
	timeout = jiffies + msecs_to_jiffies(40000);
	while (dd->fw_state == FW_TRY) {
		/*
		 * Another device is trying the firmware.  Wait until it
		 * decides what works (or not).
		 */
		if (time_after(jiffies, timeout)) {
			/* waited too long */
			dd_dev_err(dd, "Timeout waiting for firmware try");
			dd->fw_state = FW_ERR;
			dd->fw_err = -ETIMEDOUT;
			break;
		}
		mutex_unlock(&dd->fw_mutex);
		msleep(20);	/* arbitrary delay */
		mutex_lock(&dd->fw_mutex);
	}
	/* not in FW_TRY state */

	if (dd->fw_state == FW_FINAL)
		goto done;	/* already acquired */
	else if (dd->fw_state == FW_ERR)
		goto done;	/* already tried and failed */
	/* fw_state is FW_EMPTY */

	/* set fw_state to FW_TRY, FW_FINAL, or FW_ERR, and fw_err */
	__obtain_firmware(dd, fw);

done:
	mutex_unlock(&dd->fw_mutex);

	return dd->fw_err;
}

/*
 * Called when the driver unloads.  The timing is asymmetric with its
 * counterpart, obtain_firmware().  If called at device remove time,
 * then it is conceivable that another device could probe while the
 * firmware is being disposed.  The mutexes can be moved to do that
 * safely, but then the firmware would be requested from the OS multiple
 * times.
 *
 * No mutex is needed as the driver is unloading and there cannot be any
 * other callers.
 */
static void dispose_firmware(struct hfi_devdata *dd)
{
	dispose_one_firmware(&dd->fw_det);

	/* retain the error state, otherwise revert to empty */
	if (dd->fw_state != FW_ERR)
		dd->fw_state = FW_EMPTY;
}

/*
 * Called with the result of a firmware download.
 *
 * Return 1 to retry loading the firmware, 0 to stop.
 */
static int retry_firmware(struct hfi_devdata *dd, int load_result,
			  enum fw_type fw)
{
	int retry = 0;

	mutex_lock(&dd->fw_mutex);

	if (load_result == 0) {
		/*
		 * The load succeeded, so expect all others to do the same.
		 * Do not retry again.
		 */
		if (dd->fw_state == FW_TRY)
			dd->fw_state = FW_FINAL;
		retry = 0;	/* do NOT retry */
	} else if (dd->fw_state == FW_TRY) {
		/* load failed, obtain alternate firmware */
		__obtain_firmware(dd, fw);
		retry = (dd->fw_state == FW_FINAL);
	} else {
		/* else in FW_FINAL or FW_ERR, no retry in either case */
		retry = 0;
	}

	mutex_unlock(&dd->fw_mutex);
	return retry;
}

/*
 * Write a block of data to a given array CSR.  All calls will be in
 * multiples of 8 bytes.
 */
static void write_rsa_data(struct hfi_devdata *dd, int what,
			   const u8 *data, int nbytes)
{
	int qw_size = nbytes / 8;
	int i;

	if (((unsigned long)data & 0x7) == 0) {
		/* aligned */
		u64 *ptr = (u64 *)data;

		for (i = 0; i < qw_size; i++, ptr++)
			write_csr(dd, what + (8 * i), *ptr);
	} else {
		/* not aligned */
		for (i = 0; i < qw_size; i++, data += 8) {
			u64 value;

			memcpy(&value, data, 8);
			write_csr(dd, what + (8 * i), value);
		}
	}
}

/*
 * Write a block of data to a given CSR as a stream of writes.  All calls will
 * be in multiples of 8 bytes.
 */
static void write_streamed_rsa_data(struct hfi_devdata *dd, int what,
				    const u8 *data, int nbytes)
{
	u64 *ptr = (u64 *)data;
	int qw_size = nbytes / 8;

	for (; qw_size > 0; qw_size--, ptr++)
		write_csr(dd, what, *ptr);
}

/*
 * Download the signature and start the RSA mechanism.  Wait for
 * RSA_ENGINE_TIMEOUT before giving up.
 */
static int run_rsa(struct hfi_pportdata *ppd, const char *who,
		   const u8 *signature)
{
	struct hfi_devdata *dd = ppd->dd;
	unsigned long timeout;
	u64 reg;
	u32 status;
	int ret = 0;

	if (dd->emulation)
		return 0;	/* done with no error if not validating */

	/* write the signature */
	write_rsa_data(dd, FXR_FW_MSC_RSA_SIGNATURE,
		       signature, KEY_SIZE);

	/* initialize RSA */
	write_csr(dd, FXR_FW_MSC_RSA_CMD, RSA_CMD_INIT);

	/*
	 * Make sure the engine is idle and insert a delay between the two
	 * writes to FXR_FW_MSC_RSA_CMD.
	 */
	status = (read_csr(dd, FXR_FW_STS_FW)
			   & FXR_FW_STS_FW_RSA_STATUS_SMASK)
			     >> FXR_FW_STS_FW_RSA_STATUS_SHIFT;
	if (status != RSA_STATUS_IDLE) {
		ppd_dev_err(ppd, "%s security engine not idle - giving up\n",
			    who);
		return -EBUSY;
	}

	/* start RSA */
	write_csr(dd, FXR_FW_MSC_RSA_CMD, RSA_CMD_START);

	/*
	 * Look for the result.
	 *
	 * The RSA engine is hooked up to two MISC errors.  The driver
	 * masks these errors as they do not respond to the standard
	 * error "clear down" mechanism.  Look for these errors here and
	 * clear them when possible.  This routine will exit with the
	 * errors of the current run still set.
	 *
	 * MISC_FW_AUTH_FAILED_ERR
	 *	Firmware authorization failed.  This can be cleared by
	 *	re-initializing the RSA engine, then clearing the status bit.
	 *	Do not re-init the RSA angine immediately after a successful
	 *	run - this will reset the current authorization.
	 *
	 * MISC_KEY_MISMATCH_ERR
	 *	Key does not match.  The only way to clear this is to load
	 *	a matching key then clear the status bit.  If this error
	 *	is raised, it will persist outside of this routine until a
	 *	matching key is loaded.
	 */
	timeout = msecs_to_jiffies(RSA_ENGINE_TIMEOUT) + jiffies;
	while (1) {
		status = (read_csr(dd, FXR_FW_STS_FW)
			   & FXR_FW_STS_FW_RSA_STATUS_SMASK)
			     >> FXR_FW_STS_FW_RSA_STATUS_SHIFT;
		if (status == RSA_STATUS_IDLE) {
			/* should not happen */
			ppd_dev_err(ppd, "%s firmware security bad idle state\n",
				    who);
			ret = -EINVAL;
			break;
		} else if (status == RSA_STATUS_DONE) {
			/* finished successfully */
			break;
		} else if (status == RSA_STATUS_FAILED) {
			/* finished unsuccessfully */
			ret = -EINVAL;
			break;
		}
		/* else still active */

		if (time_after(jiffies, timeout)) {
			/*
			 * Timed out while active.  We can't reset the engine
			 * if it is stuck active, but run through the
			 * error code to see what error bits are set.
			 */
			ppd_dev_err(ppd, "%s firmware security time out\n",
				    who);
			ret = -ETIMEDOUT;
			break;
		}

		msleep(20);
	}

	/*
	 * Arrive here on success or failure.
	 * Print failure details if any.
	 */
	if (ret) {
		reg = read_csr(dd, FXR_FW_STS_FW);
		if (reg & FXR_FW_STS_FW_FW_AUTH_FAILED_SMASK)
			ppd_dev_err(ppd, "%s firmware authorization failed\n",
				    who);
		if (reg & FXR_FW_STS_FW_KEY_MISMATCH_SMASK)
			ppd_dev_err(ppd, "%s firmware key mismatch\n", who);
	}

	return ret;
}

static void load_security_variables(struct hfi_devdata *dd,
				    struct firmware_details *fdet)
{
	if (dd->icode == ICODE_FPGA_EMULATION)
		return;	/* nothing to do */

	/* Security variables a.  Write the modulus */
	write_rsa_data(dd, FXR_FW_MSC_RSA_MODULUS,
		       fdet->modulus, KEY_SIZE);
	/* Security variables b.  Write the r2 */
	write_rsa_data(dd, FXR_FW_MSC_RSA_R2,
		       fdet->r2, KEY_SIZE);
	/* Security variables c.  Write the mu */
	write_rsa_data(dd, FXR_FW_MSC_RSA_MU,
		       fdet->mu, MU_SIZE);
	/* Security variables d.  Write the header */
	write_streamed_rsa_data(dd, FXR_FW_MSC_SHA_PRELOAD,
				(u8 *)fdet->css_header,
				sizeof(struct css_header));
}

/* return the 8051 firmware state */
static inline u32 get_firmware_state(const struct hfi_pportdata *ppd)
{
	u64 reg = read_csr(ppd->dd, CRK_CRK8051_STS_CUR_STATE);

	dd_dev_info(ppd->dd, "%s: CRK_CRK8051_STS_CUR_STATE: 0x%llx\n",
		    __func__, reg);

	return (reg >> CRK_CRK8051_STS_CUR_STATE_FIRMWARE_SHIFT)
				& CRK_CRK8051_STS_CUR_STATE_FIRMWARE_MASK;
}

/*
 * Wait until the firmware is up and ready to take host requests.
 * Return 0 on success, -ETIMEDOUT on timeout.
 */
int hfi_wait_firmware_ready(const struct hfi_pportdata *ppd, u32 mstimeout)
{
	unsigned long timeout;

	timeout = msecs_to_jiffies(mstimeout) + jiffies;
	while (1) {
		if (get_firmware_state(ppd) == 0xa0)	/* ready */
			return 0;
		if (time_after(jiffies, timeout))	/* timed out */
			return -ETIMEDOUT;
		usleep_range(1950, 2050); /* sleep 2ms-ish */
	}
}

/*
 * Read the 8051 firmware "registers".  Use the RAM directly.  Always
 * set the result, even on error.
 * Return 0 on success, -errno on failure
 */
int hfi2_read_8051_config(struct hfi_pportdata *ppd, u8 field_id, u8 lane_id,
			  u32 *result)
{
	u64 big_data;
	u32 addr;
	int ret;

	/* address start depends on the lane_id */
	if (lane_id < 4)
		addr = (4 * NUM_GENERAL_FIELDS)
			+ (lane_id * 4 * NUM_LANE_FIELDS);
	else
		addr = 0;
	addr += field_id * 4;

	/* read is in 8-byte chunks, hardware will truncate the address down */
	ret = hfi_read_8051_data(ppd, addr, 8, &big_data);

	if (ret == 0) {
		/* extract the 4 bytes we want */
		if (addr & 0x4)
			*result = (u32)(big_data >> 32);
		else
			*result = (u32)big_data;
	} else {
		*result = 0;
		ppd_dev_err(ppd, "%s: direct read failed, lane %d, field %d!\n",
			    __func__, lane_id, field_id);
	}

	return ret;
}

static void read_misc_status(struct hfi_pportdata *ppd,
		u8 *ver_major, u8 *ver_minor, u8 *ver_patch)
{
	u32 frame;

	hfi2_read_8051_config(ppd, MISC_STATUS, GENERAL_CONFIG, &frame);
	*ver_major = (frame >> STS_FM_VERSION_MAJOR_SHIFT) &
		STS_FM_VERSION_MAJOR_MASK;
	*ver_minor = (frame >> STS_FM_VERSION_MINOR_SHIFT) &
		STS_FM_VERSION_MINOR_MASK;

	hfi2_read_8051_config(ppd, VERSION_PATCH, GENERAL_CONFIG, &frame);
	*ver_patch = (frame >> STS_FM_VERSION_PATCH_SHIFT) &
		STS_FM_VERSION_PATCH_MASK;
}

static int write_bufpe_prog_mem(struct hfi_devdata *dd, u8 *data, u32 len)
{
	u32 offset;
	u64 mem = 0;

	mem = read_csr(dd, FXR_TXOTR_MSG_DBG_BPE_PROG_MEM_ACCESS);

	for (offset = 0; offset < len; offset += 4)
		hfi_write_bufpe_prog_mem(dd, data, offset, mem);

	return 0;
}

static int write_bufpe_data_mem(struct hfi_devdata *dd, u8 *data, u32 len)
{
	u32 offset;
	u64 mem = 0;

	mem = read_csr(dd, FXR_TXOTR_MSG_DBG_BPE_DATA_MEM_ACCESS);
	for (offset = 0; offset < len; offset += 8)
		hfi_write_bufpe_data_mem(dd, data, offset, mem, len);

	return 0;
}

static int write_fragpe_prog_mem(struct hfi_devdata *dd, u8 *data, u32 len)
{
	u32 offset;
	u64 mem = 0;

	mem = read_csr(dd, FXR_TXOTR_PKT_DBG_FPE_PROG_MEM_ACCESS);

	for (offset = 0; offset < len; offset += 4)
		hfi_write_fragpe_prog_mem(dd, data, offset, mem);

	return 0;
}

static int write_fragpe_data_mem(struct hfi_devdata *dd, u8 *data, u32 len)
{
	u32 offset;
	u64 mem = 0;

	mem = read_csr(dd, FXR_TXOTR_PKT_DBG_FPE_DATA_MEM_ACCESS);

	for (offset = 0; offset < len; offset += 8)
		hfi_write_fragpe_data_mem(dd, data, offset, mem, len);
	return 0;
}

static void write_hdrpe_data(struct hfi_devdata *dd, u8 *data, u32 len,
			     u32 address)
{
	u32 offset;
	u64 reg;

	for (offset = 0; offset < len; offset += 8) {
		int bytes = len - offset;

		if (bytes < 8) {
			reg = 0;
			memcpy(&reg, &data[offset], bytes);
		} else {
			reg = *(u64 *)&data[offset];
		}
		write_csr(dd, address + offset, reg);
	}
}

static int load_hdrpe_firmware(struct hfi_devdata *dd)
{
	struct firmware_details *fdet = &dd->fw_det;

	if (fdet->firmware_len > RXHP_PMEM_SIZE)
		return -EINVAL;

	write_hdrpe_data(dd, (u8 *)fdet->css_header, PE_CSS_HEADER_SIZE,
			 FXR_RXHP_CFG_PD_DRAM);
	write_hdrpe_data(dd, (u8 *)fdet->firmware_ptr, fdet->firmware_len,
			 FXR_RXHP_CFG_PD_IRAM);
	return 0;
}

static int load_fragpe_firmware(struct hfi_devdata *dd)
{
	struct firmware_details *fdet = &dd->fw_det;
	int ret;

	if (fdet->firmware_len > FRAGPE_PMEM_SIZE)
		return -EINVAL;

	ret  = write_fragpe_data_mem(dd, (u8 *)fdet->css_header,
				     PE_CSS_HEADER_SIZE);
	if (ret)
		return ret;
	ret = write_fragpe_prog_mem(dd, (u8 *)fdet->firmware_ptr,
				    fdet->firmware_len);
	return ret;
}

static int load_bufpe_firmware(struct hfi_devdata *dd)
{
	struct firmware_details *fdet = &dd->fw_det;
	int ret;

	if (fdet->firmware_len > BUFPE_PMEM_SIZE)
		return -EINVAL;

	ret = write_bufpe_data_mem(dd, (u8 *)fdet->css_header,
				   PE_CSS_HEADER_SIZE);
	if (ret)
		return ret;
	ret = write_bufpe_prog_mem(dd, (u8 *)fdet->firmware_ptr,
				   fdet->firmware_len);
	return ret;
}

/*
 * Load the 8051 firmware.
 */
static int load_8051_firmware(struct hfi_pportdata *ppd,
			      struct firmware_details *fdet)
{
	struct hfi_devdata *dd = ppd->dd;
	u64 reg;
	int ret;

	/*
	 * MNH Reset sequence
	 * Load 8051 firmware
	 */
	/*
	 * MNH reset step 1: Reset 8051
	 */
	reg = CRK_CRK8051_CFG_RST_M8051W_SMASK
		| CRK_CRK8051_CFG_RST_CRAM_SMASK
		| CRK_CRK8051_CFG_RST_DRAM_SMASK
		| CRK_CRK8051_CFG_RST_IRAM_SMASK
		| CRK_CRK8051_CFG_RST_SFR_SMASK;
	dd_dev_info(dd, "Resettting 8051, CRK_CRK8051_CFG_RST: 0x%llx\n", reg);
	write_csr(ppd->dd, CRK_CRK8051_CFG_RST, reg);

	/*
	 * MNH reset step 2 (optional): Load 8051 data memory with link
	 * configuration
	 */

	/*
	 * MNH reset step 3: Load 8051 firmware
	 */
	/* release all but the core reset */
	reg = CRK_CRK8051_CFG_RST_M8051W_SMASK;
	dd_dev_info(dd, "Resetting 8051, CRK_CRK8051_CFG_RST: 0x%llx\n", reg);
	write_csr(ppd->dd, CRK_CRK8051_CFG_RST, reg);

	/* Firmware load step 1 */
	load_security_variables(dd, fdet);

	/*
	 * Firmware load step 2.  Clear FXR_FW_CFG_FW_CTRL.FW_8051_LOADED
	 */
	dd_dev_info(dd, "Clear FXR_FW_CFG_FW_CTRL.FW_8051_LOADED\n");
	write_csr(dd, FXR_FW_CFG_FW_CTRL, 0);

	/* Firmware load steps 3-5 */
	dd_dev_info(dd, "Call write_8051\n");
	ret = write_8051(ppd, 1/*code*/, 0, fdet->firmware_ptr,
			 fdet->firmware_len);
	if (ret)
		return ret;

	/*
	 * MNH reset step 4. Host starts the 8051 firmware
	 */
	/*
	 * Firmware load step 6.  Set FXR_FW_CFG_FW_CTRL.FW_8051_LOADED
	 * There is simpler way where pnum is 1 or 2 as its prerequisite.
	 * But I prefer the way to remove prerequisite.
	 */
	reg = 0x01;

	dd_dev_info(dd, "Set FXR_FW_CFG_FW_CTRL.FW_8051_LOADED 0x%llx\n",
		reg << FXR_FW_CFG_FW_CTRL_FW_8051_LOADED_SHIFT);
	write_csr(dd, FXR_FW_CFG_FW_CTRL,
		  reg << FXR_FW_CFG_FW_CTRL_FW_8051_LOADED_SHIFT);

	/* Firmware load steps 7-10 */
	dd_dev_info(dd, "Call run_rsa\n");
	ret = run_rsa(ppd, "8051", fdet->signature);
	if (ret)
		return ret;

	/*
	 * Clear all reset bits, releasing the 8051
	 * MNH reset step 5. Wait for firmware to be ready to accept host
	 * requests.
	 * Then, set the host version bit
	 */
	dd_dev_info(dd, "Waiting for firmware to come back with ready\n");
	mutex_lock(&ppd->crk8051_mutex);
	ret = hfi2_release_and_wait_ready_8051_firmware(ppd);
	mutex_unlock(&ppd->crk8051_mutex);
	if (ret)
		return ret;

	return 0;
}

static int _load_firmware(struct hfi_pportdata *ppd, enum fw_type fw)
{
	int ret;

	if (ppd->dd->fw_load) {
		do {
			ret = load_8051_firmware(ppd, &ppd->dd->fw_det);
		} while (retry_firmware(ppd->dd, ret, fw));
		if (ret)
			return ret;
	}

	return 0;
}

static int load_pe_fw(struct hfi_devdata *dd, enum fw_type fw)
{
	int ret;

	switch (fw) {
	case FW_HDRPE:
		ret = load_hdrpe_firmware(dd);
		break;
	case FW_FRAGPE:
		ret = load_fragpe_firmware(dd);
		break;
	case FW_BUFPE:
		ret = load_bufpe_firmware(dd);
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int firmware_init(struct hfi_devdata *dd, enum fw_type fw)
{
	if (dd->icode == ICODE_FUNCTIONAL_SIMULATOR)
		dd->platform_config.from = NOT_READ_YET;

	obtain_default_fw_name(dd, fw);
	if (!dd->fw_name)
		return -EINVAL;
	return obtain_firmware(dd, fw);
}

/*
 * Authenticate HDRPE
 * See "Receive HP Processing Engine Configuration"
 * of FXR HAS 0.65 or later
 */
static int
authenticate_hdrpe(struct hfi_devdata *dd)
{
	int timeout;
	u64 reg = 0;
	u64 start_authentication_smask =
		FXR_RXHP_CFG_HDR_PE_START_AUTHENTICATION_SMASK;
	u64 authentication_complete_smask =
		FXR_RXHP_CFG_HDR_PE_AUTHENTICATION_COMPLETE_SMASK;
	u64 authentication_success_smask =
		FXR_RXHP_CFG_HDR_PE_AUTHENTICATION_SUCCESS_SMASK;
	u64 pe_enable_smask =
		FXR_RXHP_CFG_HDR_PE_PE_ENABLE_SMASK;

	/* Use firmware for 9B paths. See 2204045629 */
	if (dd->emulation)
		start_authentication_smask |=
			(0x3E00ULL << FXR_RXHP_CFG_HDR_PE_USE_FIRMWARE_SHIFT);
	/* start authentication */
	write_csr(dd, FXR_RXHP_CFG_HDR_PE, start_authentication_smask);

	/* wait until finish authentication */
	for (timeout = 0; timeout++ < PE_TIMEOUT; ) {
		reg = read_csr(dd, FXR_RXHP_CFG_HDR_PE);
		if (reg & authentication_complete_smask)
			break;
		usleep_range(10, 20);
	}
	if (timeout >= PE_TIMEOUT) {
		dd_dev_err(dd, "authenticate hdrpe timed out: 0x%llx",
			   reg);
		return -ENXIO;
	}
	if (!(reg & authentication_success_smask)) {
		dd_dev_err(dd, "authenticate hdrpe fail: 0x%llx",
			   reg);
		return -ENXIO;
	}

	/* Use firmware for 9B paths. See 2204045629 */
	if (dd->emulation)
		pe_enable_smask |=
			(0x3E00ULL << FXR_RXHP_CFG_HDR_PE_USE_FIRMWARE_SHIFT);

	/* enable PE */
	write_csr(dd, FXR_RXHP_CFG_HDR_PE, pe_enable_smask);

	return 0;
}

/*
 * Authenticate OTR Buf PE
 */
static int
authenticate_bufpe(struct hfi_devdata *dd)
{
	int timeout;
	u64 buf;

	buf = read_csr(dd, FXR_TXOTR_MSG_CFG_BUFF_ENG);
	buf |= FXR_TXOTR_MSG_CFG_BUFF_ENG_START_AUTHENTICATION_SMASK;
	/* start authentication */
	write_csr(dd, FXR_TXOTR_MSG_CFG_BUFF_ENG, buf);

	/* wait until finish authentication */
	for (timeout = 0; timeout++ < PE_TIMEOUT; ) {
		buf = read_csr(dd, FXR_TXOTR_MSG_CFG_BUFF_ENG);
		if (buf & HFI_BUFF_COMPLETE)
			break;
		usleep_range(10, 20);
	}
	if (timeout >= PE_TIMEOUT) {
		dd_dev_err(dd, "authenticate bufpe timed out: 0x%llx", buf);
		return -ENXIO;
	}
	if (!(buf & HFI_BUFF_SUCCESS)) {
		dd_dev_err(dd, "authenticate bufpe fail: 0x%llx", buf);
		return -ENXIO;
	}
	return 0;
}

/*
 * Authenticate OTR Frag PE
 */
static int
authenticate_fragpe(struct hfi_devdata *dd)
{
	int timeout;
	u64 frag;

	frag = read_csr(dd, FXR_TXOTR_PKT_CFG_FRAG_ENG);
	frag |= FXR_TXOTR_PKT_CFG_FRAG_ENG_START_AUTHENTICATION_SMASK;

	/* start authentication */
	write_csr(dd, FXR_TXOTR_PKT_CFG_FRAG_ENG, frag);

	/* wait until finish authentication */
	for (timeout = 0; timeout++ < PE_TIMEOUT; ) {
		frag = read_csr(dd, FXR_TXOTR_PKT_CFG_FRAG_ENG);
		if (frag & HFI_FRAG_COMPLETE)
			break;
		usleep_range(10, 20);
	}
	if (timeout >= PE_TIMEOUT) {
		dd_dev_err(dd, "authenticate fragpe timed out: 0x%llx", frag);
		return -ENXIO;
	}
	if (!(frag & HFI_FRAG_SUCCESS)) {
		dd_dev_err(dd, "authenticate fragpe fail: 0x%llx", frag);
		return -ENXIO;
	}
	return 0;
}

static int write_host_interface_version(struct hfi_pportdata *ppd, u8 version)
{
	u32 frame;
	u32 mask;

	lockdep_assert_held(&ppd->crk8051_mutex);
	mask = (HOST_INTERFACE_VERSION_MASK << HOST_INTERFACE_VERSION_SHIFT);
	hfi2_read_8051_config(ppd, RESERVED_REGISTERS, GENERAL_CONFIG, &frame);
	/* Clear, then set field */
	frame &= ~mask;
	frame |= ((u32)version << HOST_INTERFACE_VERSION_SHIFT);
	return _load_8051_config(ppd, RESERVED_REGISTERS, GENERAL_CONFIG,
				 frame);
}

/*
 */
int hfi2_load_firmware(struct hfi_devdata *dd)
{
	int ret;
	u8 port;
	enum fw_type fw;
	struct hfi_pportdata *ppd;
	u8 ver_major;
	u8 ver_minor;
	u8 ver_patch;

	mutex_init(&dd->fw_mutex);
	if (no_mnh)
		goto skip_8051;

	dd->fw_state = FW_EMPTY;
	dd->fw_load = true;
	ret = firmware_init(dd, FW_8051);
	if (ret) {
		dd_dev_err(dd, "can't init 8051 firmware");
		return ret;
	}
	for (port = 1; port <= dd->num_pports; port++) {
		ppd = to_hfi_ppd(dd, port);
		ret = _load_firmware(ppd, FW_8051);
		if (ret) {
			ppd_dev_err(ppd, "can't load 8051 firmware");
			return ret;
		}
	}

	ppd = to_hfi_ppd(dd, 1);
	read_misc_status(ppd, &ver_major, &ver_minor, &ver_patch);
	dd_dev_info(dd, "8051 firmware version %d.%d.%d\n",
		    (int)ver_major, (int)ver_minor, (int)ver_patch);
	dd->crk8051_ver = crk8051_ver(ver_major, ver_minor, ver_patch);
	mutex_lock(&ppd->crk8051_mutex);
	ret = write_host_interface_version(ppd, HOST_INTERFACE_VERSION);
	mutex_unlock(&ppd->crk8051_mutex);
	if (ret != HCMD_SUCCESS) {
		dd_dev_err(dd,
			   "Failed to set host interface version, return 0x%x\n",
			   ret);
		return -EIO;
	}

skip_8051:
	/*
	 * Zebu currently pushes the firmware directly into the imem.
	 * The irom contents are pre-loaded with a sequence which bypasses
	 * validation. Starting in ww19c, the FPGA builds are expected
	 * to perform this same sequence. So do not push the firmware
	 * images into the PE firmware CSRs for Zebu or FPGA.
	 */
	if (dd->emulation)
		goto skip_pe_fw_load;
	for (fw = FW_HDRPE; fw <= FW_BUFPE; fw++) {
		dd->fw_state = FW_EMPTY;
		dd->fw_load = true;
		ret = firmware_init(dd, fw);
		if (ret) {
			dd_dev_err(dd, "can't init PE firmware");
			return ret;
		}
		ret = load_pe_fw(dd, fw);
		if (ret) {
			dd_dev_err(dd, "can't load PE firmware");
			return ret;
		}
	}
skip_pe_fw_load:
	ret = authenticate_hdrpe(dd);
	if (ret) {
		dd_dev_err(dd, "can't authenticate hdrpe");
		return ret;
	}
	ret = authenticate_bufpe(dd);
	if (ret) {
		dd_dev_err(dd, "can't authenticate bufpe");
		return ret;
	}
	ret = authenticate_fragpe(dd);
	if (ret) {
		dd_dev_err(dd, "can't authenticate fragpe");
		return ret;
	}
	return 0;
}

void hfi2_dispose_firmware(struct hfi_devdata *dd)
{
	if (no_mnh)
		return;
	dispose_firmware(dd);
}

/*
 * Clear all reset bits, releasing the 8051.
 * Wait for firmware to be ready to accept host requests.
 * Then, set host version bit.
 *
 * This function executes even if the 8051 is in reset mode when
 * ppd->oc_shutdown == 1.
 *
 * Expects ppd->crk8051_mutex to be held.
 */
int hfi2_release_and_wait_ready_8051_firmware(struct hfi_pportdata *ppd)
{
	int ret;
	u32 timeout = TIMEOUT_8051_START;

	if (ppd->dd->emulation)
		timeout *= 3;

	lockdep_assert_held(&ppd->crk8051_mutex);
	/* clear all reset bits, releaseing the 8051 */
	write_csr(ppd->dd, CRK_CRK8051_CFG_RST, 0ull);

	/* wait for the firmware to be ready to accept requests */
	ret = hfi_wait_firmware_ready(ppd, timeout);
	if (ret) {
		ppd_dev_err(ppd, "%s: timeout starting the 8051 firmware\n",
			    __func__);
		if (ppd->dd->emulation)
			msleep(100);
		return ret;
	}

	ret = write_host_interface_version(ppd, HOST_INTERFACE_VERSION);
	if (ret != HCMD_SUCCESS) {
		ppd_dev_err(ppd,
			    "Failed to set host interface version, ret 0x%x\n",
			    ret);
		return -EIO;
	}
	return 0;
}
